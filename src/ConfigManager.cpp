#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include "../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

bool ConfigManager::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    return parseJSON(content);
}

bool ConfigManager::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    file << generateJSON();
    file.close();
    return true;
}

bool ConfigManager::parseJSON(const std::string& jsonStr) {
    try {
        json data = json::parse(jsonStr);

        m_servers.clear();
        m_downloadPaths.clear();

        if (data.contains("global_game_path"))
            m_globalGamePath = data["global_game_path"].get<std::string>();

        if (data.contains("servers") && data["servers"].is_array()) {
            for (const auto& s : data["servers"]) {
                ServerConfig srv;
                srv.id         = s.value("id",          std::string(""));
                srv.name       = s.value("name",        std::string(""));
                srv.fastdlUrl  = s.value("fastdl_url",  std::string(""));
                srv.gamePath   = s.value("game_path",   std::string(""));

                if (s.contains("resource_types") && s["resource_types"].is_array())
                    srv.resourceTypes = s["resource_types"].get<std::vector<std::string>>();

                m_servers.push_back(srv);
            }
        }

        if (data.contains("download_paths") && data["download_paths"].is_object()) {
            for (const auto& [serverId, paths] : data["download_paths"].items()) {
                if (paths.is_array())
                    m_downloadPaths[serverId] = paths.get<std::vector<std::string>>();
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Config] JSON parse error: " << e.what() << "\n";
        return false;
    }
}

std::string ConfigManager::generateJSON() const {
    json data;
    data["global_game_path"] = m_globalGamePath;

    json serversArr = json::array();
    for (const auto& s : m_servers) {
        json sj;
        sj["id"]          = s.id;
        sj["name"]        = s.name;
        sj["fastdl_url"]  = s.fastdlUrl;
        sj["game_path"]   = s.gamePath;
        json types = json::array();
        for (const auto& t : s.resourceTypes) types.push_back(t);
        sj["resource_types"] = types;
        serversArr.push_back(sj);
    }
    data["servers"] = serversArr;

    json pathsObj = json::object();
    for (const auto& [sid, paths] : m_downloadPaths) {
        json arr = json::array();
        for (const auto& p : paths) arr.push_back(p);
        pathsObj[sid] = arr;
    }
    data["download_paths"] = pathsObj;

    return data.dump(4);
}

void ConfigManager::addServer(const ServerConfig& server) {
    m_servers.push_back(server);
}

bool ConfigManager::removeServer(const std::string& id) {
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
        [&id](const ServerConfig& s){ return s.id == id; });
    if (it != m_servers.end()) { m_servers.erase(it); return true; }
    return false;
}

ServerConfig* ConfigManager::getServer(const std::string& id) {
    auto it = std::find_if(m_servers.begin(), m_servers.end(),
        [&id](const ServerConfig& s){ return s.id == id; });
    return (it != m_servers.end()) ? &(*it) : nullptr;
}

std::vector<ServerConfig> ConfigManager::getServers() const {
    return m_servers;
}

void ConfigManager::setGlobalGamePath(const std::string& gamePath) {
    m_globalGamePath = gamePath;
}

std::vector<std::string> ConfigManager::getDownloadPaths(const std::string& serverId) const {
    auto it = m_downloadPaths.find(serverId);
    if (it != m_downloadPaths.end()) return it->second;
    return {};
}

void ConfigManager::addDownloadPath(const std::string& serverId, const std::string& path) {
    m_downloadPaths[serverId].push_back(path);
}
