#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "ConfigManager.h"
#include "FastDLDownloader.h"
#include "ProgressBar.h"
#include "Utils.h"

#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <chrono>
#include <thread>
#include <algorithm>
#include <limits>
#include <set>

namespace fs = std::filesystem;

static void clearLine() {
    std::cout << "\r\033[K";
}

static void enableUtf8Console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode))
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

static void printBanner() {
    std::cout << "\n";
    std::cout << "+------------------------------------------------------------------+\n";
    std::cout << "|           FastDL Tool  --  Game File Downloader  v1.1           |\n";
    std::cout << "|           Windows + Linux  |  MinGW / GCC  |  C++17             |\n";
    std::cout << "|   Author: SyntX | https://github.com/SyntX34/FastDLExtractor    |\n";
    std::cout << "+------------------------------------------------------------------+\n\n";
}

static void printHelp(const char* argv0) {
    std::cout << "Usage: " << argv0 << " [OPTIONS]\n\n";
    std::cout << "  -c, --config  <path>   Config file  (default: configs/servers.json)\n";
    std::cout << "  -s, --server  <index>  Server index (skips interactive prompt)\n";
    std::cout << "  -d, --download <path>  Download a specific relative path or folder/\n";
    std::cout << "  -o, --output  <dir>    Output directory (default: game_path from config)\n";
    std::cout << "  -t, --threads <n>      Parallel download threads (default: 4)\n";
    std::cout << "  -f, --force            Force re-download even if file exists\n";
    std::cout << "  -h, --help             This help text\n";
    std::cout << "  -v, --version          Version info\n\n";
    std::cout << "Config download_paths modes:\n";
    std::cout << "  Folders   : \"maps/\"              -- crawl & sync entire folder\n";
    std::cout << "  Files     : \"maps/de_dust2.bsp\"  -- specific files only\n";
    std::cout << "  (empty)   : crawl all resource-type folders from FastDL root\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << argv0 << "                                 # interactive mode\n";
    std::cout << "  " << argv0 << " -s 0                            # sync missing files for server 0\n";
    std::cout << "  " << argv0 << " -s 0 -d maps/de_dust2.bsp       # download one file\n";
    std::cout << "  " << argv0 << " -s 0 -d maps/ -f                # re-download entire maps folder\n\n";
}

static void printVersion() {
    std::cout << "FastDL Tool  v1.1\n";
    std::cout << "Built with C++17 | CMake | MinGW/GCC\n";
    std::cout << "MIT License\n";
}

class DownloadProgress : public ProgressCallback {
public:
    explicit DownloadProgress(size_t expectedFiles)
        : m_expected(expectedFiles)
    {
        m_bar.setStyle(ProgressBarStyle::FULL);
        m_bar.setWidth(45);
    }

    void onFileStart(const std::string& filename) override {
        std::lock_guard<std::mutex> lk(m_mx);
        m_current = filename;
        std::string disp = fs::path(filename).filename().string();
        if (disp.size() > 60) disp = "..." + disp.substr(disp.size()-57);
        std::cout << "\n  [>>] " << disp << "\n";
    }

    void onFileProgress(const std::string& /*filename*/,
                        double progress,
                        size_t downloaded,
                        size_t total,
                        double speed,
                        double eta) override {
        std::lock_guard<std::mutex> lk(m_mx);
        double overall = (m_expected > 0)
                       ? ((double)m_done + progress) / (double)m_expected
                       : progress;
        m_bar.setProgress(overall);

        clearLine();
        std::cout << "  " << m_bar.render();
        if (total > 0)
            std::cout << "  " << formatBytes(downloaded) << " / " << formatBytes(total);
        else
            std::cout << "  " << formatBytes(downloaded);
        std::cout << "  @ " << formatSpeed(speed);
        if (eta >= 0)
            std::cout << "  ETA " << formatDuration(eta);
        std::cout << std::flush;
    }

    void onFileComplete(const std::string& filename, size_t size) override {
        std::lock_guard<std::mutex> lk(m_mx);
        m_done++;
        m_totalBytes += size;
        clearLine();
        std::string disp = fs::path(filename).filename().string();
        if (disp.size() > 55) disp = "..." + disp.substr(disp.size()-52);
        std::cout << "  [OK] " << disp << "  (" << formatBytes(size) << ")\n";
    }

    void onExtractStart(const std::string& filename) override {
        std::lock_guard<std::mutex> lk(m_mx);
        clearLine();
        std::cout << "  [EXTRACT] " << fs::path(filename).filename().string() << "\n";
    }

    void onExtractComplete(const std::string& filename) override {
        std::lock_guard<std::mutex> lk(m_mx);
        clearLine();
        std::cout << "  [DONE] Extracted: " << fs::path(filename).filename().string() << "\n";
    }

    void onError(const std::string& filename, const std::string& error) override {
        std::lock_guard<std::mutex> lk(m_mx);
        m_errors++;
        clearLine();
        std::cerr << "  [ERROR] " << fs::path(filename).filename().string()
                  << "  --  " << error << "\n";
    }

    size_t done()       const { return m_done; }
    size_t errors()     const { return m_errors; }
    size_t totalBytes() const { return m_totalBytes; }

    void setExpected(size_t n) { m_expected = n; }

private:
    std::mutex  m_mx;
    ProgressBar m_bar;
    size_t      m_expected   = 0;
    size_t      m_done       = 0;
    size_t      m_errors     = 0;
    size_t      m_totalBytes = 0;
    std::string m_current;
};

static std::vector<std::string> filterMissingFiles(
        const std::vector<std::string>& files,
        const std::string& gamePath,
        bool verbose = true) {

    std::vector<std::string> missing;

    if (verbose)
        std::cout << "\n  Checking existing files...\n";

    for (const auto& file : files) {
        fs::path fullPath = fs::path(gamePath) / file;
        bool exists = fs::exists(fullPath);

        if (!exists) {
            missing.push_back(file);
            if (verbose)
                std::cout << "    [MISSING] " << file << "\n";
        } else {
            if (verbose)
                std::cout << "    [EXISTS]  " << file << "\n";
        }
    }

    if (verbose) {
        std::cout << "\n  Summary: " << missing.size() << " files missing, "
                  << (files.size() - missing.size()) << " files already present.\n";
    }

    return missing;
}

static void createExampleConfig(ConfigManager& cm, const std::string& path) {
    fs::create_directories(fs::path(path).parent_path());

    ServerConfig cs1;
    cs1.id           = "css_server1";
    cs1.name         = "CS:Source - My Server";
    cs1.fastdlUrl    = "http://fastdl.example.com/cstrike/";
    cs1.gamePath     = "C:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Source/cstrike";
    cs1.resourceTypes = {".bsp", ".mdl", ".vtx", ".vvd", ".phy", ".wav", ".mp3",
                         ".png", ".vtf", ".vmt", ".dx80.vtx", ".dx90.vtx", ".sw.vtx",
                         ".nav", ".pcf"};
    cm.addServer(cs1);

    cm.addDownloadPath("css_server1", "maps/");
    cm.addDownloadPath("css_server1", "materials/");
    cm.addDownloadPath("css_server1", "models/");
    cm.addDownloadPath("css_server1", "sound/");

    cm.saveToFile(path);
}

int main(int argc, char* argv[]) {
    enableUtf8Console();
    printBanner();

    std::string configPath  = "configs/servers.json";
    std::string outputDir;
    std::string specificPath;
    int         serverIndex    = -1;
    int         numThreads     = 4;
    bool        forceRedownload = false;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help")    { printHelp(argv[0]); return 0; }
        if (a == "-v" || a == "--version") { printVersion();     return 0; }
        if (a == "-f" || a == "--force")   { forceRedownload = true; continue; }

        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << flag << "\n";
                std::exit(1);
            }
            return argv[++i];
        };

        if      (a == "-c" || a == "--config")   configPath   = next("-c");
        else if (a == "-s" || a == "--server")   serverIndex  = std::stoi(next("-s"));
        else if (a == "-d" || a == "--download") specificPath = next("-d");
        else if (a == "-o" || a == "--output")   outputDir    = next("-o");
        else if (a == "-t" || a == "--threads")  numThreads   = std::stoi(next("-t"));
        else { std::cerr << "Unknown option: " << a << "\n"; return 1; }
    }

    numThreads = std::max(1, std::min(numThreads, 16));

    ConfigManager cm;
    if (!cm.loadFromFile(configPath)) {
        std::cerr << "  Config not found at: " << configPath << "\n";
        std::cout << "  Creating example config...\n";
        createExampleConfig(cm, configPath);
        std::cout << "  Example config written to: " << configPath << "\n";
        std::cout << "  Please edit it with your real server settings, then run again.\n\n";
        return 1;
    }

    auto servers = cm.getServers();
    if (servers.empty()) {
        std::cerr << "  No servers found in config: " << configPath << "\n";
        return 1;
    }

    ServerConfig chosen;
    if (serverIndex >= 0 && serverIndex < (int)servers.size()) {
        chosen = servers[serverIndex];
    } else {
        std::cout << "  Available servers:\n\n";
        for (size_t i = 0; i < servers.size(); i++) {
            std::cout << "    [" << i << "]  " << servers[i].name << "\n";
            std::cout << "         URL  : " << servers[i].fastdlUrl << "\n";
            std::cout << "         Path : " << servers[i].gamePath  << "\n";
            auto paths = cm.getDownloadPaths(servers[i].id);
            if (!paths.empty())
                std::cout << "         Download paths : " << paths.size() << "\n";
            std::cout << "\n";
        }

        while (true) {
            std::cout << "  Select server [0-" << (servers.size()-1) << "]: ";
            int choice = -1;
            if (!(std::cin >> choice) || choice < 0 || choice >= (int)servers.size()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "  Invalid -- please enter a number between 0 and "
                          << (servers.size()-1) << "\n";
                continue;
            }
            chosen = servers[choice];
            break;
        }
    }

    if (outputDir.empty()) outputDir = chosen.gamePath;
    if (outputDir.empty()) outputDir = "downloads";

    std::cout << "\n  Server  : " << chosen.name     << "\n";
    std::cout << "  FastDL  : " << chosen.fastdlUrl  << "\n";
    std::cout << "  Output  : " << outputDir          << "\n";
    std::cout << "  Threads : " << numThreads          << "\n";
    if (forceRedownload)
        std::cout << "  Mode    : Force re-download (all files)\n";
    else
        std::cout << "  Mode    : Smart sync (missing files only)\n";

    FastDLDownloader dl(chosen.fastdlUrl, outputDir, numThreads);
    dl.setResourceTypes(chosen.resourceTypes);

    std::vector<std::string> configPaths;
    if (!specificPath.empty()) {
        configPaths.push_back(specificPath);
    } else {
        configPaths = cm.getDownloadPaths(chosen.id);
    }

    if (configPaths.empty()) {
        std::cout << "\n  No download paths configured.\n";
        std::cout << "  Hint: use  -d \"maps/de_dust2.bsp\"  to download a specific file,\n";
        std::cout << "        or   -d \"maps/\"               to sync an entire folder,\n";
        std::cout << "        or add paths to download_paths." << chosen.id
                  << " in " << configPath << "\n\n";
        return 0;
    }

    std::vector<std::string> expandedFiles;
    std::vector<std::string> folderEntries;
    std::vector<std::string> fileEntries;

    for (const auto& p : configPaths) {
        if (!p.empty() && p.back() == '/')
            folderEntries.push_back(p);
        else
            fileEntries.push_back(p);
    }

    expandedFiles.insert(expandedFiles.end(), fileEntries.begin(), fileEntries.end());

    if (!folderEntries.empty()) {
        std::cout << "\n  Crawling " << folderEntries.size()
                  << " folder(s) on FastDL server...\n";
        for (const auto& folder : folderEntries) {
            std::cout << "    Listing: " << chosen.fastdlUrl << folder << " ... ";
            std::cout.flush();
            auto found = dl.fetchDirectoryListing(folder);
            std::cout << found.size() << " files found\n";
            expandedFiles.insert(expandedFiles.end(), found.begin(), found.end());
        }
    }

    {
        std::set<std::string> seen;
        std::vector<std::string> deduped;
        for (const auto& f : expandedFiles) {
            if (seen.insert(f).second) deduped.push_back(f);
        }
        expandedFiles = std::move(deduped);
    }

    std::cout << "\n  Total files on server matching resource types: "
              << expandedFiles.size() << "\n";

    if (expandedFiles.empty()) {
        std::cout << "  Nothing to download.\n\n";
        return 0;
    }

    std::vector<std::string> toDownload;
    size_t alreadyPresent = 0;

    if (forceRedownload) {
        toDownload = expandedFiles;
        std::cout << "  Force mode: queuing all " << toDownload.size() << " files.\n";
    } else {
        bool verbose = (expandedFiles.size() <= 50);
        if (!verbose)
            std::cout << "  Checking local files...\n";

        toDownload = filterMissingFiles(expandedFiles, outputDir, verbose);
        alreadyPresent = expandedFiles.size() - toDownload.size();

        if (!verbose) {
            std::cout << "  Already present : " << alreadyPresent << "\n";
            std::cout << "  Missing         : " << toDownload.size() << "\n";
        }
    }

    if (toDownload.empty()) {
        std::cout << "\n  All files are up to date! Nothing to download.\n\n";
        return 0;
    }

    fs::create_directories(outputDir);

    auto progress = std::make_shared<DownloadProgress>(toDownload.size());
    dl.setProgressCallback(progress);

    std::cout << "\n  Queuing " << toDownload.size() << " missing file(s)...\n";

    auto wallStart = std::chrono::steady_clock::now();

    for (const auto& path : toDownload) {
        if (!dl.downloadFile(path)) {
            std::cerr << "  Skipped (extension filtered): " << path << "\n";
        }
    }

    dl.waitAll();

    auto wallEnd = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(wallEnd - wallStart).count();

    std::cout << "\n";
    std::cout << " +--------------------------------------+\n";
    std::cout << " |  Download Summary                    |\n";
    std::cout << " +--------------------------------------+\n";
    std::cout << " |  Files downloaded : " << std::setw(14) << progress->done()   << " |\n";
    std::cout << " |  Already present  : " << std::setw(14) << alreadyPresent     << " |\n";
    std::cout << " |  Errors           : " << std::setw(14) << progress->errors() << " |\n";
    std::cout << " |  Total received   : " << std::setw(14) << formatBytes(progress->totalBytes()) << " |\n";
    std::cout << " |  Elapsed time     : " << std::setw(14) << formatDuration(elapsed) << " |\n";
    if (elapsed > 0 && progress->totalBytes() > 0) {
        double avgSpeed = (double)progress->totalBytes() / elapsed;
        std::cout << " |  Avg speed        : " << std::setw(14) << formatSpeed(avgSpeed) << " |\n";
    }
    std::cout << " +--------------------------------------+\n";
    std::cout << "   Output: " << outputDir << "\n\n";

    return (progress->errors() == 0) ? 0 : 2;
}