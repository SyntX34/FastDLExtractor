#pragma once

#include <string>
#include <vector>
#include <map>

struct ServerConfig {
    std::string id;
    std::string name;
    std::string fastdlUrl;
    std::string gamePath;
    std::vector<std::string> resourceTypes;

    ServerConfig() = default;
};

class ConfigManager {
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;

    void addServer(const ServerConfig& server);
    bool removeServer(const std::string& id);
    ServerConfig* getServer(const std::string& id);
    std::vector<ServerConfig> getServers() const;

    void setGlobalGamePath(const std::string& gamePath);
    std::string getGlobalGamePath() const { return m_globalGamePath; }

    std::vector<std::string> getDownloadPaths(const std::string& serverId) const;
    void addDownloadPath(const std::string& serverId, const std::string& path);

private:
    std::vector<ServerConfig> m_servers;
    std::map<std::string, std::vector<std::string>> m_downloadPaths;
    std::string m_globalGamePath;

    bool parseJSON(const std::string& jsonStr);
    std::string generateJSON() const;
};
