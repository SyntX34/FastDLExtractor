/*
 * FastDL Tool  --  C API bridge implementation
 * Wraps FastDLDownloader + ConfigManager into a plain-C shared library.
 */

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

#include "fastdl_capi.h"
#include "FastDLDownloader.h"
#include "ConfigManager.h"

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <sstream>

struct FDLContext {
    std::unique_ptr<FastDLDownloader> dl;

    FDLProgressCB progress_cb  = nullptr;
    void*         progress_ud  = nullptr;
    FDLEventCB    event_cb     = nullptr;
    void*         event_ud     = nullptr;

    std::atomic<int>       done_files{0};
    std::atomic<long long> done_bytes{0};

    std::mutex cb_mutex;
};

class BridgeCallback : public ProgressCallback {
public:
    explicit BridgeCallback(FDLContext* ctx) : m_ctx(ctx) {}

    void onFileStart(const std::string& filename) override {
        std::lock_guard<std::mutex> lk(m_ctx->cb_mutex);
        if (m_ctx->event_cb)
            m_ctx->event_cb(0, filename.c_str(), nullptr, 0, m_ctx->event_ud);
    }

    void onFileProgress(const std::string& filename,
                        double   progress,
                        size_t   downloaded,
                        size_t   total,
                        double   speed,
                        double   eta) override
    {
        std::lock_guard<std::mutex> lk(m_ctx->cb_mutex);
        if (m_ctx->progress_cb)
            m_ctx->progress_cb(
                filename.c_str(),
                progress,
                (long long)downloaded,
                (long long)total,
                speed,
                eta,
                m_ctx->done_files.load(),
                m_ctx->done_bytes.load(),
                m_ctx->progress_ud
            );
    }

    void onFileComplete(const std::string& filename, size_t size) override {
        m_ctx->done_files.fetch_add(1);
        m_ctx->done_bytes.fetch_add((long long)size);
        std::lock_guard<std::mutex> lk(m_ctx->cb_mutex);
        if (m_ctx->event_cb)
            m_ctx->event_cb(1, filename.c_str(), nullptr, (long long)size, m_ctx->event_ud);
    }

    void onExtractStart(const std::string& filename) override {
        std::lock_guard<std::mutex> lk(m_ctx->cb_mutex);
        if (m_ctx->event_cb)
            m_ctx->event_cb(2, filename.c_str(), nullptr, 0, m_ctx->event_ud);
    }

    void onExtractComplete(const std::string& filename) override {
        std::lock_guard<std::mutex> lk(m_ctx->cb_mutex);
        if (m_ctx->event_cb)
            m_ctx->event_cb(3, filename.c_str(), nullptr, 0, m_ctx->event_ud);
    }

    void onError(const std::string& filename, const std::string& error) override {
        std::lock_guard<std::mutex> lk(m_ctx->cb_mutex);
        if (m_ctx->event_cb)
            m_ctx->event_cb(4, filename.c_str(), error.c_str(), 0, m_ctx->event_ud);
    }

private:
    FDLContext* m_ctx;
};

extern "C" {

FDLHandle fdl_create(const char* base_url, const char* output_dir, int num_threads) {
    auto* ctx = new FDLContext();
    ctx->dl   = std::make_unique<FastDLDownloader>(
                    base_url  ? base_url  : "",
                    output_dir? output_dir: "downloads",
                    num_threads);
    auto bridge = std::make_shared<BridgeCallback>(ctx);
    ctx->dl->setProgressCallback(bridge);
    return ctx;
}

void fdl_destroy(FDLHandle h) {
    delete static_cast<FDLContext*>(h);
}

void fdl_set_resource_types(FDLHandle h, const char** types, int count) {
    auto* ctx = static_cast<FDLContext*>(h);
    std::vector<std::string> v;
    for (int i = 0; i < count; ++i)
        if (types[i]) v.emplace_back(types[i]);
    ctx->dl->setResourceTypes(v);
}

void fdl_set_max_retries(FDLHandle h, int r) {
    static_cast<FDLContext*>(h)->dl->setMaxRetries(r);
}
void fdl_set_timeout(FDLHandle h, int s) {
    static_cast<FDLContext*>(h)->dl->setTimeoutSecs(s);
}

void fdl_set_progress_cb(FDLHandle h, FDLProgressCB cb, void* ud) {
    auto* ctx = static_cast<FDLContext*>(h);
    std::lock_guard<std::mutex> lk(ctx->cb_mutex);
    ctx->progress_cb = cb;
    ctx->progress_ud = ud;
}

void fdl_set_event_cb(FDLHandle h, FDLEventCB cb, void* ud) {
    auto* ctx = static_cast<FDLContext*>(h);
    std::lock_guard<std::mutex> lk(ctx->cb_mutex);
    ctx->event_cb = cb;
    ctx->event_ud = ud;
}

int fdl_download_file(FDLHandle h, const char* relative_path) {
    if (!h || !relative_path) return 0;
    return static_cast<FDLContext*>(h)->dl->downloadFile(relative_path) ? 1 : 0;
}

char* fdl_fetch_listing(FDLHandle h, const char* relative_dir) {
    if (!h || !relative_dir) return nullptr;
    auto* ctx = static_cast<FDLContext*>(h);
    auto files = ctx->dl->fetchDirectoryListing(relative_dir);
    std::ostringstream ss;
    for (const auto& f : files) ss << f << '\n';
    std::string result = ss.str();
    char* buf = static_cast<char*>(std::malloc(result.size() + 1));
    if (!buf) return nullptr;
    std::memcpy(buf, result.c_str(), result.size() + 1);
    return buf;
}

void fdl_wait_all(FDLHandle h) {
    if (h) static_cast<FDLContext*>(h)->dl->waitAll();
}

void fdl_cancel(FDLHandle h) {
    (void)h;
}

long long fdl_total_bytes(FDLHandle h) {
    if (!h) return 0;
    return (long long)static_cast<FDLContext*>(h)->dl->totalBytesDownloaded();
}

long long fdl_total_files(FDLHandle h) {
    if (!h) return 0;
    return (long long)static_cast<FDLContext*>(h)->dl->totalFilesDownloaded();
}

char* fdl_config_load(const char* filepath) {
    ConfigManager cm;
    if (!cm.loadFromFile(filepath)) return nullptr;
    std::string tmp = std::string(filepath) + ".tmp_read";
    if (!cm.saveToFile(tmp)) return nullptr;
    std::ifstream f(tmp);
    if (!f.is_open()) return nullptr;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    f.close();
    std::remove(tmp.c_str());
    char* buf = static_cast<char*>(std::malloc(content.size() + 1));
    if (!buf) return nullptr;
    std::memcpy(buf, content.c_str(), content.size() + 1);
    return buf;
}

int fdl_config_save(const char* filepath, const char* json_content) {
    if (!filepath || !json_content) return 0;
    std::ofstream f(filepath);
    if (!f.is_open()) return 0;
    f << json_content;
    return f.good() ? 1 : 0;
}

const char* fdl_version(void) {
    return "FastDL Tool v1.2 (GUI edition)";
}

} /* extern "C" */