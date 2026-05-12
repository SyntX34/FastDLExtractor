#pragma once

#include <string>
#include <cstdio>

inline std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int u = 0;
    double sz = static_cast<double>(bytes);
    while (sz >= 1024.0 && u < 4) { sz /= 1024.0; u++; }
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%.2f %s", sz, units[u]);
    return std::string(buf);
}

inline std::string formatSpeed(double bytesPerSecond) {
    return formatBytes(static_cast<size_t>(bytesPerSecond)) + "/s";
}

inline std::string formatDuration(double seconds) {
    if (seconds < 0) return "?";
    int s = static_cast<int>(seconds);
    int h = s / 3600;
    int m = (s % 3600) / 60;
    int sec = s % 60;
    char buf[32];
    if (h > 0)
        std::snprintf(buf, sizeof(buf), "%dh %02dm %02ds", h, m, sec);
    else if (m > 0)
        std::snprintf(buf, sizeof(buf), "%dm %02ds", m, sec);
    else
        std::snprintf(buf, sizeof(buf), "%ds", sec);
    return std::string(buf);
}
