#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <functional>

class ProgressCallback {
public:
    virtual ~ProgressCallback() = default;
    virtual void onFileStart(const std::string& /*filename*/) {}
    virtual void onFileProgress(const std::string& /*filename*/,
                                double   /*progress*/,
                                size_t   /*downloaded*/,
                                size_t   /*total*/,
                                double   /*speed*/,
                                double   /*eta*/)
    {}
    virtual void onFileComplete(const std::string& /*filename*/, size_t /*finalSize*/) {}
    virtual void onExtractStart(const std::string& /*filename*/) {}
    virtual void onExtractComplete(const std::string& /*filename*/) {}
    virtual void onError(const std::string& /*filename*/, const std::string& /*error*/) {}
};

class FastDLDownloader {
public:
    FastDLDownloader(const std::string& baseUrl,
                     const std::string& outputDir,
                     int numThreads = 4);
    ~FastDLDownloader();

    void setResourceTypes(const std::vector<std::string>& types);
    void setProgressCallback(std::shared_ptr<ProgressCallback> cb);
    void setMaxRetries(int r)     { m_maxRetries      = r; }
    void setTimeoutSecs(int s)    { m_timeoutSeconds  = s; }

    bool downloadFile(const std::string& relativePath);
    bool fileExistsOnServer(const std::string& relativePath);
    std::vector<std::string> fetchDirectoryListing(const std::string& relativeDir);

    void waitAll();

    size_t totalBytesDownloaded() const { return m_totalBytes.load(); }
    size_t totalFilesDownloaded() const { return m_totalFiles.load(); }

private:
    struct DownloadTask {
        std::string remotePath;
        std::string localPath;
        bool isBZ2 = false;
    };

    std::string m_baseUrl;
    std::string m_outputDir;
    int         m_numThreads;
    int         m_maxRetries     = 3;
    int         m_timeoutSeconds = 60;

    std::vector<std::string>          m_resourceTypes;
    std::shared_ptr<ProgressCallback> m_callback;

    std::queue<DownloadTask>  m_queue;
    std::vector<std::thread>  m_workers;
    std::mutex                m_mutex;
    std::condition_variable   m_cv;
    std::condition_variable   m_doneCv;
    bool                      m_shutdown  = false;
    int                       m_inFlight  = 0;

    std::atomic<size_t> m_totalBytes{0};
    std::atomic<size_t> m_totalFiles{0};

    void workerLoop();
    bool performDownload(const DownloadTask& task);

    bool httpDownload(const std::string& url, const std::string& localPath,
                      size_t& outSize, double& outSpeed);

    bool httpFetch(const std::string& url, std::string& body);

    bool extractBZ2(const std::string& bz2Path, const std::string& outPath);

    bool        shouldDownload(const std::string& filename) const;
    std::string fileExt(const std::string& filename) const;
    std::string joinUrl(const std::string& base, const std::string& rel) const;

    std::vector<std::string> parseHtmlLinks(const std::string& html,
                                             const std::string& baseHref) const;
};