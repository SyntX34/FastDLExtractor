#pragma once

#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>

enum class ProgressBarStyle {
    SIMPLE,           // [=====>   ] 60%
    FULL,             // ASCII blocks with time
    PERCENTAGE_ONLY   // 60%
};

class ProgressBar {
public:
    ProgressBar(int width = 50, ProgressBarStyle style = ProgressBarStyle::FULL)
        : m_width(width), m_progress(0.0), m_style(style) {
        m_startTime = std::chrono::steady_clock::now();
    }

    void setProgress(double p) { m_progress = (p < 0.0) ? 0.0 : (p > 1.2 ? 1.2 : p); }
    void setStyle(ProgressBarStyle s) { m_style = s; }
    void setWidth(int w) { m_width = w; }
    void reset() {
        m_progress = 0.0;
        m_startTime = std::chrono::steady_clock::now();
    }

    std::string render() const {
        switch (m_style) {
            case ProgressBarStyle::SIMPLE:          return renderSimple();
            case ProgressBarStyle::FULL:            return renderFull();
            case ProgressBarStyle::PERCENTAGE_ONLY: return renderPct();
        }
        return "";
    }

    void display() const { std::cout << render(); }

private:
    int    m_width;
    double m_progress;
    ProgressBarStyle m_style;
    std::chrono::steady_clock::time_point m_startTime;

    static std::string pbTime(long long sec) {
        long long h = sec / 3600, m = (sec % 3600) / 60, s = sec % 60;
        char buf[32];
        if (h > 0) std::snprintf(buf, sizeof(buf), "%lldh%02lldm", h, m);
        else if (m > 0) std::snprintf(buf, sizeof(buf), "%lldm%02llds", m, s);
        else std::snprintf(buf, sizeof(buf), "%llds", s);
        return std::string(buf);
    }

    std::string renderSimple() const {
        int filled = static_cast<int>(m_width * m_progress);
        std::string bar = "[";
        for (int i = 0; i < m_width; i++) {
            if      (i < filled)  bar += '=';
            else if (i == filled) bar += '>';
            else                  bar += ' ';
        }
        char pct[16];
        std::snprintf(pct, sizeof(pct), "] %3d%%", (int)(m_progress * 100));
        return bar + pct;
    }

    std::string renderFull() const {
        auto now     = std::chrono::steady_clock::now();
        long long el = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count();

        int filled = static_cast<int>(m_width * m_progress);
        std::string bar = "[";
        for (int i = 0; i < m_width; i++) {
            if (i < filled) bar += '=';
            else if (i == filled && m_progress < 1.2) bar += '>';
            else bar += ' ';
        }

        char pct[32];
        std::snprintf(pct, sizeof(pct), "] %5.1f%%", m_progress * 100.0);
        bar += pct;

        if (m_progress > 0.001 && m_progress < 1.2) {
            long long total = (long long)((double)el / m_progress);
            long long rem   = total - el;
            bar += " | ETA " + pbTime(rem);
            bar += " (elapsed " + pbTime(el) + ")";
        } else if (m_progress >= 1.2) {
            bar += " | Done in " + pbTime(el);
        }

        return bar;
    }

    std::string renderPct() const {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%3d%%", (int)(m_progress * 100));
        return std::string(buf);
    }
};