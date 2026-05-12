#include "FastDLDownloader.h"
#include "Utils.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <thread>
#include <sstream>
#include <cstring>
#include <regex>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#  include <windows.h>
#  include <winhttp.h>
#  include <bzlib.h>
#else
#  include <curl/curl.h>
#  include <bzlib.h>
#endif

namespace fs = std::filesystem;

FastDLDownloader::FastDLDownloader(const std::string& baseUrl,
                                   const std::string& outputDir,
                                   int numThreads)
    : m_baseUrl(baseUrl)
    , m_outputDir(outputDir)
    , m_numThreads(std::max(1, std::min(numThreads, 16)))
{
    if (!m_baseUrl.empty() && m_baseUrl.back() != '/') m_baseUrl += '/';

    fs::create_directories(m_outputDir);

#ifndef _WIN32
    curl_global_init(CURL_GLOBAL_DEFAULT);
#endif

    for (int i = 0; i < m_numThreads; i++)
        m_workers.emplace_back(&FastDLDownloader::workerLoop, this);
}

FastDLDownloader::~FastDLDownloader() {
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_shutdown = true;
    }
    m_cv.notify_all();
    for (auto& t : m_workers) if (t.joinable()) t.join();

#ifndef _WIN32
    curl_global_cleanup();
#endif
}

void FastDLDownloader::setResourceTypes(const std::vector<std::string>& types) {
    m_resourceTypes = types;
}

void FastDLDownloader::setProgressCallback(std::shared_ptr<ProgressCallback> cb) {
    m_callback = cb;
}

bool FastDLDownloader::downloadFile(const std::string& relativePath) {
    if (relativePath.empty()) return false;

    bool alreadyBZ2 = (relativePath.size() > 4 &&
                       relativePath.substr(relativePath.size() - 4) == ".bz2");

    DownloadTask task;

    if (alreadyBZ2) {
        if (!shouldDownload(relativePath)) return false;
        task.remotePath = relativePath;
        task.localPath  = (fs::path(m_outputDir) / relativePath).string();
        task.isBZ2      = true;
    } else {
        if (!shouldDownload(relativePath)) return false;
        task.remotePath = relativePath + ".bz2";
        task.localPath  = (fs::path(m_outputDir) / (relativePath + ".bz2")).string();
        task.isBZ2      = true;
    }

    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_queue.push(task);
    }
    m_cv.notify_one();
    return true;
}

bool FastDLDownloader::fileExistsOnServer(const std::string& relativePath) {
    std::string url = joinUrl(m_baseUrl, relativePath + ".bz2");

#ifdef _WIN32
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t scheme[16]{}, host[256]{}, path[2048]{};
    uc.lpszScheme    = scheme; uc.dwSchemeLength    = 16;
    uc.lpszHostName  = host;   uc.dwHostNameLength  = 256;
    uc.lpszUrlPath   = path;   uc.dwUrlPathLength   = 2048;

    std::wstring wurl(url.begin(), url.end());
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return false;

    bool isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    HINTERNET hSession = WinHttpOpen(L"FastDLTool/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"HEAD", path,
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                       WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    WinHttpReceiveResponse(hRequest, nullptr);

    DWORD statusCode = 0, statusLen = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusLen, WINHTTP_NO_HEADER_INDEX);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return (statusCode == 200);
#else
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_easy_cleanup(curl);
    return (res == CURLE_OK && http_code == 200);
#endif
}

std::vector<std::string> FastDLDownloader::fetchDirectoryListing(
        const std::string& relativeDir) {

    std::vector<std::string> results;

    std::string dir = relativeDir;
    if (dir.empty() || dir.back() != '/') dir += '/';

    std::string url  = joinUrl(m_baseUrl, dir);
    std::string body;
    if (!httpFetch(url, body)) return results;

    auto links = parseHtmlLinks(body, dir);

    for (const auto& link : links) {
        if (link.empty()) continue;

        if (link.back() == '/') {
            auto sub = fetchDirectoryListing(link);
            results.insert(results.end(), sub.begin(), sub.end());
        } else {
            std::string realName = link;
            if (realName.size() > 4 &&
                realName.substr(realName.size() - 4) == ".bz2") {
                realName = realName.substr(0, realName.size() - 4);
            }
            if (shouldDownload(realName)) {
                results.push_back(realName);
            }
        }
    }

    return results;
}

std::vector<std::string> FastDLDownloader::parseHtmlLinks(
        const std::string& html,
        const std::string& baseHref) const {

    std::vector<std::string> links;
    std::string lower = html;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    size_t pos = 0;
    while (true) {
        size_t hrefPos = lower.find("href=", pos);
        if (hrefPos == std::string::npos) break;

        pos = hrefPos + 5;
        char quote = html[pos];
        if (quote != '"' && quote != '\'') continue;

        pos++;
        size_t end = html.find(quote, pos);
        if (end == std::string::npos) break;

        std::string href = html.substr(pos, end - pos);
        pos = end + 1;

        if (href.empty())           continue;
        if (href[0] == '/')         continue; // absolute path
        if (href.find("://") != std::string::npos) continue; // http://...
        if (href[0] == '?')         continue;
        if (href[0] == '#')         continue;
        if (href == "../")          continue;
        if (href.find("../") == 0)  continue;

        std::string decoded;
        decoded.reserve(href.size());
        for (size_t i = 0; i < href.size(); ++i) {
            if (href[i] == '%' && i + 2 < href.size()) {
                char hex[3] = { href[i+1], href[i+2], 0 };
                char* end2;
                long val = std::strtol(hex, &end2, 16);
                if (end2 == hex + 2) {
                    decoded += static_cast<char>(val);
                    i += 2;
                    continue;
                }
            }
            decoded += href[i];
        }

        links.push_back(baseHref + decoded);
    }

    return links;
}


void FastDLDownloader::waitAll() {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_doneCv.wait(lk, [this]{
        return m_queue.empty() && m_inFlight == 0;
    });
}

void FastDLDownloader::workerLoop() {
    while (true) {
        DownloadTask task;
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cv.wait(lk, [this]{ return m_shutdown || !m_queue.empty(); });

            if (m_shutdown && m_queue.empty()) break;
            if (m_queue.empty()) continue;

            task = m_queue.front();
            m_queue.pop();
            m_inFlight++;
        }

        performDownload(task);

        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_inFlight--;
        }
        m_doneCv.notify_all();
    }
}

bool FastDLDownloader::performDownload(const DownloadTask& task) {
    if (m_callback) m_callback->onFileStart(task.remotePath);

    fs::create_directories(fs::path(task.localPath).parent_path());

    std::string url = joinUrl(m_baseUrl, task.remotePath);
    size_t  fileSize = 0;
    double  speed    = 0.0;
    bool    success  = false;

    for (int attempt = 0; attempt < m_maxRetries && !success; attempt++) {
        if (attempt > 0) std::this_thread::sleep_for(std::chrono::seconds(2));
        success = httpDownload(url, task.localPath, fileSize, speed);
    }

    if (!success) {
        if (task.isBZ2) {
            std::string plainRemote = task.remotePath.substr(0, task.remotePath.size() - 4);
            std::string plainLocal  = task.localPath.substr(0, task.localPath.size() - 4);
            std::string plainUrl    = joinUrl(m_baseUrl, plainRemote);

            for (int attempt = 0; attempt < m_maxRetries && !success; attempt++) {
                if (attempt > 0) std::this_thread::sleep_for(std::chrono::seconds(2));
                success = httpDownload(plainUrl, plainLocal, fileSize, speed);
            }

            if (success) {
                m_totalBytes += fileSize;
                m_totalFiles++;
                if (m_callback) m_callback->onFileComplete(plainRemote, fileSize);
                return true;
            }
        }

        if (m_callback)
            m_callback->onError(task.remotePath,
                "Failed after " + std::to_string(m_maxRetries) + " attempts");
        return false;
    }

    if (task.isBZ2) {
        if (m_callback) m_callback->onExtractStart(task.remotePath);

        std::string extracted = task.localPath.substr(0, task.localPath.size() - 4);
        if (!extractBZ2(task.localPath, extracted)) {
            if (m_callback) m_callback->onError(task.remotePath, "BZ2 extraction failed");
            fs::remove(task.localPath);
            return false;
        }
        fs::remove(task.localPath);
        std::error_code ec;
        auto sz = fs::file_size(extracted, ec);
        if (!ec) fileSize = static_cast<size_t>(sz);

        if (m_callback) m_callback->onExtractComplete(extracted);
    }

    m_totalBytes += fileSize;
    m_totalFiles++;

    if (m_callback) m_callback->onFileComplete(task.remotePath, fileSize);
    return true;
}

#ifdef _WIN32
static bool winHttpOpenRequest(const std::string& url,
                                const std::wstring& method,
                                int timeoutMs,
                                HINTERNET& hSession,
                                HINTERNET& hConnect,
                                HINTERNET& hRequest) {
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    wchar_t scheme[16]{}, host[256]{}, path[4096]{};
    uc.lpszScheme    = scheme; uc.dwSchemeLength    = 16;
    uc.lpszHostName  = host;   uc.dwHostNameLength  = 256;
    uc.lpszUrlPath   = path;   uc.dwUrlPathLength   = 4096;

    std::wstring wurl(url.begin(), url.end());
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return false;

    bool isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);

    hSession = WinHttpOpen(L"FastDLTool/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    WinHttpSetTimeouts(hSession, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path,
                                   nullptr, WINHTTP_NO_REFERER,
                                   WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }
    return true;
}

bool FastDLDownloader::httpFetch(const std::string& url, std::string& body) {
    HINTERNET hSession{}, hConnect{}, hRequest{};
    if (!winHttpOpenRequest(url, L"GET", 15000, hSession, hConnect, hRequest))
        return false;

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD statusCode = 0, statusLen = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusLen, WINHTTP_NO_HEADER_INDEX);

    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::vector<char> buf(65536);
    while (true) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &avail) || avail == 0) break;
        DWORD toRead = (DWORD)std::min((size_t)avail, buf.size());
        DWORD read   = 0;
        if (!WinHttpReadData(hRequest, buf.data(), toRead, &read) || read == 0) break;
        body.append(buf.data(), read);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return !body.empty();
}

bool FastDLDownloader::httpDownload(const std::string& url,
                                    const std::string& localPath,
                                    size_t& outSize, double& outSpeed) {
    HINTERNET hSession{}, hConnect{}, hRequest{};
    if (!winHttpOpenRequest(url, L"GET", m_timeoutSeconds * 1000,
                            hSession, hConnect, hRequest))
        return false;

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD statusCode = 0, statusLen = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusLen, WINHTTP_NO_HEADER_INDEX);
    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    size_t contentLen = 0;
    {
        wchar_t lenBuf[32]{};
        DWORD lenSize = sizeof(lenBuf);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                lenBuf, &lenSize, WINHTTP_NO_HEADER_INDEX)) {
            contentLen = (size_t)_wtoi64(lenBuf);
        }
    }

    std::ofstream file(localPath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    auto startTime = std::chrono::steady_clock::now();
    size_t received = 0;
    std::vector<char> buf(65536);

    while (true) {
        DWORD avail = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &avail)) break;
        if (avail == 0) break;

        DWORD toRead = (DWORD)std::min((size_t)avail, buf.size());
        DWORD read   = 0;
        if (!WinHttpReadData(hRequest, buf.data(), toRead, &read)) break;
        if (read == 0) break;

        file.write(buf.data(), read);
        received += read;

        if (m_callback) {
            auto now   = std::chrono::steady_clock::now();
            double el  = std::chrono::duration<double>(now - startTime).count();
            double spd = (el > 0) ? (double)received / el : 0.0;
            double eta = (contentLen > 0 && spd > 0)
                         ? (double)(contentLen - received) / spd : -1.0;
            double prog = (contentLen > 0) ? (double)received / contentLen : 0.0;
            m_callback->onFileProgress(localPath, prog, received, contentLen, spd, eta);
        }
    }

    file.close();

    auto endTime = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(endTime - startTime).count();

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    outSize  = received;
    outSpeed = (elapsed > 0) ? (double)received / elapsed : 0.0;
    return (received > 0);
}

#else //Linux / macOS (libcurl)

struct CurlWriteState {
    std::string* body    = nullptr;
    std::ofstream* file  = nullptr;
};

static size_t curlWriteBody(void* ptr, size_t sz, size_t nmemb, void* ud) {
    auto* st = static_cast<CurlWriteState*>(ud);
    size_t bytes = sz * nmemb;
    if (st->body) st->body->append(static_cast<const char*>(ptr), bytes);
    if (st->file) st->file->write(static_cast<const char*>(ptr), bytes);
    return bytes;
}

struct CurlProgressState {
    size_t received = 0;
    std::chrono::steady_clock::time_point start;
    std::shared_ptr<ProgressCallback> cb;
    std::string filename;
};

static int curlProgress(void* ud, curl_off_t dlTotal, curl_off_t dlNow,
                         curl_off_t, curl_off_t) {
    auto* st = static_cast<CurlProgressState*>(ud);
    if (!st->cb) return 0;

    auto   now  = std::chrono::steady_clock::now();
    double el   = std::chrono::duration<double>(now - st->start).count();
    double spd  = (el > 0) ? (double)dlNow / el : 0.0;
    double eta  = (dlTotal > 0 && spd > 0) ? (double)(dlTotal - dlNow) / spd : -1.0;
    double prog = (dlTotal > 0) ? (double)dlNow / dlTotal : 0.0;

    st->cb->onFileProgress(st->filename, prog,
                           (size_t)dlNow, (size_t)dlTotal, spd, eta);
    return 0;
}

bool FastDLDownloader::httpFetch(const std::string& url, std::string& body) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    CurlWriteState ws;
    ws.body = &body;

    curl_easy_setopt(curl, CURLOPT_URL,           url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteBody);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &ws);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,       15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,     "FastDLTool/1.0");

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && http_code == 200 && !body.empty());
}

bool FastDLDownloader::httpDownload(const std::string& url,
                                    const std::string& localPath,
                                    size_t& outSize, double& outSpeed) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::ofstream file(localPath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) { curl_easy_cleanup(curl); return false; }

    CurlWriteState ws;
    ws.file = &file;

    CurlProgressState ps;
    ps.start    = std::chrono::steady_clock::now();
    ps.cb       = m_callback;
    ps.filename = localPath;

    curl_easy_setopt(curl, CURLOPT_URL,              url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,    curlWriteBody);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,        &ws);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curlProgress);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA,     &ps);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS,       0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,   1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,          (long)m_timeoutSeconds);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,        "FastDLTool/1.0");
    curl_easy_setopt(curl, CURLOPT_FAILONERROR,      1L);

    CURLcode res = curl_easy_perform(curl);
    file.close();

    if (res != CURLE_OK) {
        fs::remove(localPath);
        curl_easy_cleanup(curl);
        return false;
    }

    auto endTime = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(endTime - ps.start).count();

    outSize  = ws.body ? 0 : (size_t)ps.received;
    {
        std::error_code ec;
        auto sz = fs::file_size(localPath, ec);
        if (!ec) outSize = (size_t)sz;
    }
    outSpeed = (elapsed > 0) ? (double)outSize / elapsed : 0.0;

    curl_easy_cleanup(curl);
    return (outSize > 0);
}

#endif // _WIN32


bool FastDLDownloader::extractBZ2(const std::string& bz2Path, const std::string& outPath) {
    FILE* fin = fopen(bz2Path.c_str(), "rb");
    if (!fin) return false;

    FILE* fout = fopen(outPath.c_str(), "wb");
    if (!fout) { fclose(fin); return false; }

    int bzerr = BZ_OK;
    BZFILE* bz = BZ2_bzReadOpen(&bzerr, fin, 0, 0, nullptr, 0);
    if (bzerr != BZ_OK || !bz) {
        fclose(fin);
        fclose(fout);
        return false;
    }

    char buf[65536];
    bool ok = true;
    while (true) {
        int n = BZ2_bzRead(&bzerr, bz, buf, sizeof(buf));
        if (n > 0) {
            if (fwrite(buf, 1, (size_t)n, fout) != (size_t)n) { ok = false; break; }
        }
        if (bzerr == BZ_STREAM_END) break;
        if (bzerr != BZ_OK) { ok = false; break; }
    }

    BZ2_bzReadClose(&bzerr, bz);
    fclose(fin);
    fclose(fout);

    if (!ok) fs::remove(outPath);
    return ok;
}

bool FastDLDownloader::shouldDownload(const std::string& filename) const {
    if (m_resourceTypes.empty()) return true;
    std::string ext   = fileExt(filename);
    std::string inner = (ext == ".bz2")
                      ? fileExt(filename.substr(0, filename.size()-4))
                      : ext;
    auto inList = [&](const std::string& e) {
        return std::find(m_resourceTypes.begin(), m_resourceTypes.end(), e)
               != m_resourceTypes.end();
    };
    if (inList(ext))   return true;
    if (inList(inner)) return true;
    size_t dot1 = filename.find_last_of('.');
    if (dot1 != std::string::npos && dot1 > 0) {
        size_t dot2 = filename.find_last_of('.', dot1 - 1);
        if (dot2 != std::string::npos) {
            std::string twopart = filename.substr(dot2);
            if (inList(twopart)) return true;
        }
    }
    return false;
}

std::string FastDLDownloader::fileExt(const std::string& filename) const {
    size_t dot = filename.find_last_of('.');
    if (dot == std::string::npos) return "";
    return filename.substr(dot);
}

std::string FastDLDownloader::joinUrl(const std::string& base,
                                      const std::string& rel) const {
    if (rel.empty()) return base;
    std::string r = rel;
    while (!r.empty() && r.front() == '/') r = r.substr(1);
    return base + r;
}