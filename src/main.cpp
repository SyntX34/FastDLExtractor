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
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

static void clearLine() {
    std::cout << "\r\033[K";
}

static void printBanner() {
    std::cout << "\n"
              << "\t\t╔══════════════════════════════════════════════════════════════════╗\n"
              << "\t\t║                    FastDL Tool -- v1.0                           ║\n"
              << "\t\t║                 Game File Downloader for FastDL                  ║\n"
              << "\t\t║                                                                  ║\n"
              << "\t\t║           Platform: Windows + Linux | Compiler: C++17            ║\n"
              << "\t\t║           Author: SyntX | github.com/SyntX34/FastDLExtractor     ║\n"
              << "\t\t╚══════════════════════════════════════════════════════════════════╝\n\n";
}

static void printHelp(const char* argv0) {
    std::cout << "Usage: " << argv0 << " [OPTIONS]\n\n";
    std::cout << "  -c, --config  <path>   Config file  (default: configs/servers.json)\n";
    std::cout << "  -s, --server  <index>  Server index (skips interactive prompt)\n";
    std::cout << "  -d, --download <path>  Download a specific relative path\n";
    std::cout << "  -o, --output  <dir>    Output directory (default: downloads/)\n";
    std::cout << "  -t, --threads <n>      Parallel download threads (default: 4)\n";
    std::cout << "  -h, --help             This help text\n";
    std::cout << "  -v, --version          Version info\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << argv0 << "                                 # interactive mode\n";
    std::cout << "  " << argv0 << " -s 0 -d maps/de_dust2.bsp       # direct download\n";
    std::cout << "  " << argv0 << " -s 1 -o C:/Games/gmod/garrysmod # custom output dir\n\n";
}

static void printVersion() {
    std::cout << "FastDL Tool  v1.0.0\n";
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
		std::string disp = filename;
		if (disp.size() > 60) disp = "..." + disp.substr(disp.size()-57);
		std::cout << "\n  >> " << disp << "\n";
	}

    void onFileProgress(const std::string& /*filename*/,
                        double progress,
                        size_t downloaded,
                        size_t total,
                        double speed,
                        double eta) override {
        std::lock_guard<std::mutex> lk(m_mx);
        double overall = 0.0;
        if (m_expected > 0) {
            overall = ((double)m_done + progress) / (double)m_expected;
        } else {
            overall = progress;
        }
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
		std::string disp = filename;
		if (disp.size() > 55) disp = "..." + disp.substr(disp.size()-52);
		std::cout << "  [OK] " << disp << "  (" << formatBytes(size) << ")\n";
	}

    void onExtractStart(const std::string& filename) override {
		std::lock_guard<std::mutex> lk(m_mx);
		clearLine();
		std::cout << "  [EXTRACT] Extracting " << fs::path(filename).filename().string() << "...\n";
	}

    void onExtractComplete(const std::string& filename) override {
		std::lock_guard<std::mutex> lk(m_mx);
		clearLine();
		std::cout << "  [DONE] Extracted  " << fs::path(filename).filename().string() << "\n";
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

private:
    std::mutex  m_mx;
    ProgressBar m_bar;
    size_t      m_expected   = 0;
    size_t      m_done       = 0;
    size_t      m_errors     = 0;
    size_t      m_totalBytes = 0;
    std::string m_current;
};

static void createExampleConfig(ConfigManager& cm, const std::string& path) {
    fs::create_directories(fs::path(path).parent_path());

    ServerConfig cs1;
    cs1.id           = "gmod_server1";
    cs1.name         = "Garry's Mod — My Server";
    cs1.fastdlUrl    = "http://fastdl.example.com/garrysmod/";
    cs1.gamePath     = "C:/Program Files (x86)/Steam/steamapps/common/GarrysMod/garrysmod";
    cs1.resourceTypes = {".bsp", ".mdl", ".vtx", ".vvd", ".phy",
                          ".wav", ".mp3", ".png", ".vtf", ".vmt"};
    cm.addServer(cs1);

    ServerConfig cs2;
    cs2.id           = "css_server1";
    cs2.name         = "CS:Source — My Server";
    cs2.fastdlUrl    = "http://fastdl.example.com/cstrike/";
    cs2.gamePath     = "C:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Source/cstrike";
    cs2.resourceTypes = {".bsp", ".wav", ".mp3"};
    cm.addServer(cs2);

    cm.addDownloadPath("gmod_server1", "maps/gm_flatgrass.bsp");
    cm.addDownloadPath("gmod_server1", "materials/myserver/logo.vtf");

    cm.saveToFile(path);
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD dwMode = 0;
		GetConsoleMode(hOut, &dwMode);
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hOut, dwMode);
	#endif

	printBanner();

    std::string configPath   = "configs/servers.json";
    std::string outputDir    = "downloads";
    std::string specificPath;
    int         serverIndex  = -1;
    int         numThreads   = 4;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help")    { printHelp(argv[0]); return 0; }
        if (a == "-v" || a == "--version") { printVersion();     return 0; }
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << flag << "\n";
                std::exit(1);
            }
            return argv[++i];
        };
        if (a == "-c" || a == "--config")   configPath   = next("-c");
        else if (a == "-s" || a == "--server")  serverIndex  = std::stoi(next("-s"));
        else if (a == "-d" || a == "--download") specificPath = next("-d");
        else if (a == "-o" || a == "--output")  outputDir    = next("-o");
        else if (a == "-t" || a == "--threads") numThreads   = std::stoi(next("-t"));
        else { std::cerr << "Unknown option: " << a << "\n"; return 1; }
    }

    numThreads = std::max(1, std::min(numThreads, 16));

    ConfigManager cm;
    if (!cm.loadFromFile(configPath)) {
        std::cerr << "  Config not found at: " << configPath << "\n";
        std::cout << "  Creating example config...\n";
        createExampleConfig(cm, configPath);
        std::cout << "  \u2705  Example config written to: " << configPath << "\n";
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
            if (!paths.empty()) {
                std::cout << "         Pre-configured files: " << paths.size() << "\n";
            }
            std::cout << "\n";
        }

        while (true) {
            std::cout << "  Select server [0–" << (servers.size()-1) << "]: ";
            int choice = -1;
            if (!(std::cin >> choice) || choice < 0 || choice >= (int)servers.size()) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "  Invalid — please enter a number between 0 and "
                          << (servers.size()-1) << "\n";
                continue;
            }
            chosen = servers[choice];
            break;
        }
    }

    std::cout << "\n  Server  : " << chosen.name     << "\n";
    std::cout << "  FastDL  : " << chosen.fastdlUrl  << "\n";
    std::cout << "  Game    : " << chosen.gamePath   << "\n";
    std::cout << "  Output  : " << outputDir          << "\n";
    std::cout << "  Threads : " << numThreads          << "\n";

    std::vector<std::string> toDownload;
    if (!specificPath.empty()) {
        toDownload.push_back(specificPath);
    } else {
        toDownload = cm.getDownloadPaths(chosen.id);
    }

    if (toDownload.empty()) {
        std::cout << "\n  No files queued.\n";
        std::cout << "  Hint: use  -d \"maps/de_dust2.bsp\"  to download a specific file,\n";
        std::cout << "        or add paths to  download_paths." << chosen.id
                  << "  in " << configPath << "\n\n";
        return 0;
    }

    fs::create_directories(outputDir);
    if (!chosen.gamePath.empty()) fs::create_directories(chosen.gamePath);

    auto progress = std::make_shared<DownloadProgress>(toDownload.size());

    FastDLDownloader dl(chosen.fastdlUrl, outputDir, numThreads);
    dl.setResourceTypes(chosen.resourceTypes);
    dl.setProgressCallback(progress);

    std::cout << "\n  Queuing " << toDownload.size() << " file(s)...\n";

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
    std::cout << " ┌────────────────────────────────────┐\n";
    std::cout << " │  Download Summary                  │\n";
    std::cout << " ├────────────────────────────────────┤\n";
    std::cout << " │  Files completed : " << std::setw(16) << progress->done()   << " │\n";
    std::cout << " │  Errors          : " << std::setw(16) << progress->errors() << " │\n";
    std::cout << " │  Total received  : " << std::setw(16) << formatBytes(progress->totalBytes()) << " │\n";
    std::cout << " │  Elapsed time    : " << std::setw(16) << formatDuration(elapsed) << " │\n";
    if (elapsed > 0 && progress->totalBytes() > 0) {
        double avgSpeed = (double)progress->totalBytes() / elapsed;
        std::cout << " │  Avg speed       : " << std::setw(16) << formatSpeed(avgSpeed) << " │\n";
    }
    std::cout << " ├────────────────────────────────────┤\n";
    std::cout << " │  Output dir  : " << outputDir << "\n";
    std::cout << " │  Game path   : " << chosen.gamePath << "\n";
    std::cout << " └────────────────────────────────────┘\n\n";

    if (!chosen.gamePath.empty() && outputDir != chosen.gamePath) {
        std::cout << "  Tip: Copy files from  " << outputDir
                  << "  into  " << chosen.gamePath << "\n\n";
    }

    return (progress->errors() == 0) ? 0 : 2;
}