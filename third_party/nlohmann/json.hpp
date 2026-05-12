// Minimal JSON library compatible with nlohmann/json API used in this project
// Supports: parse, object/array access, .contains(), .value(), .get<T>(), .dump(), .items()
#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace nlohmann {

class json {
public:
    enum class value_t { null_t, bool_t, int_t, float_t, string_t, array_t, object_t };

    using object_type = std::map<std::string, json>;
    using array_type  = std::vector<json>;

    json() : m_type(value_t::null_t) {}
    json(std::nullptr_t) : m_type(value_t::null_t) {}
    json(bool v)               : m_type(value_t::bool_t),   m_bool(v) {}
    json(int v)                : m_type(value_t::int_t),    m_int((long long)v) {}
    json(long v)               : m_type(value_t::int_t),    m_int((long long)v) {}
    json(long long v)          : m_type(value_t::int_t),    m_int(v) {}
    json(unsigned v)           : m_type(value_t::int_t),    m_int((long long)v) {}
    json(double v)             : m_type(value_t::float_t),  m_float(v) {}
    json(const char* v)        : m_type(value_t::string_t), m_string(v) {}
    json(const std::string& v) : m_type(value_t::string_t), m_string(v) {}
    json(std::string&& v)      : m_type(value_t::string_t), m_string(std::move(v)) {}

    static json array()  { json j; j.m_type = value_t::array_t;  return j; }
    static json object() { json j; j.m_type = value_t::object_t; return j; }

    bool is_null()   const { return m_type == value_t::null_t;   }
    bool is_bool()   const { return m_type == value_t::bool_t;   }
    bool is_number() const { return m_type == value_t::int_t || m_type == value_t::float_t; }
    bool is_string() const { return m_type == value_t::string_t; }
    bool is_array()  const { return m_type == value_t::array_t;  }
    bool is_object() const { return m_type == value_t::object_t; }

    bool contains(const std::string& key) const {
        return is_object() && m_object.count(key) > 0;
    }

    json& operator[](const std::string& key) {
        if (m_type == value_t::null_t) m_type = value_t::object_t;
        return m_object[key];
    }
    const json& operator[](const std::string& key) const { return m_object.at(key); }
    json& operator[](size_t i)       { return m_array[i]; }
    const json& operator[](size_t i) const { return m_array[i]; }

    void push_back(const json& v) {
        if (m_type == value_t::null_t) m_type = value_t::array_t;
        m_array.push_back(v);
    }

    size_t size() const {
        if (is_array())  return m_array.size();
        if (is_object()) return m_object.size();
        return 0;
    }
    bool empty() const { return size() == 0; }

    template<typename T> T get() const;

    template<typename T>
    T value(const std::string& key, const T& def) const {
        if (contains(key)) return m_object.at(key).get<T>();
        return def;
    }

    // Array iteration
    array_type::const_iterator begin() const { return m_array.begin(); }
    array_type::const_iterator end()   const { return m_array.end();   }

    // Object iteration via items()
    struct items_proxy {
        const object_type& obj;
        struct iter {
            object_type::const_iterator it;
            std::pair<const std::string&, const json&> operator*() const { return {it->first, it->second}; }
            iter& operator++() { ++it; return *this; }
            bool operator!=(const iter& o) const { return it != o.it; }
        };
        iter begin() const { return {obj.begin()}; }
        iter end()   const { return {obj.end()};   }
    };
    items_proxy items() const { return {m_object}; }

    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        do_dump(oss, indent, 0);
        return oss.str();
    }

    static json parse(const std::string& s) {
        size_t pos = 0;
        return parse_val(s, pos);
    }

private:
    value_t     m_type   = value_t::null_t;
    bool        m_bool   = false;
    long long   m_int    = 0;
    double      m_float  = 0.0;
    std::string m_string;
    array_type  m_array;
    object_type m_object;

    void do_dump(std::ostringstream& oss, int ind, int depth) const {
        auto nl  = [&]{ if(ind>=0) oss << '\n'; };
        auto pad = [&](int d){ if(ind>=0) oss << std::string((size_t)(ind*d), ' '); };
        switch (m_type) {
            case value_t::null_t:   oss << "null"; break;
            case value_t::bool_t:   oss << (m_bool ? "true" : "false"); break;
            case value_t::int_t:    oss << m_int; break;
            case value_t::float_t:  oss << m_float; break;
            case value_t::string_t: oss << '"' << esc(m_string) << '"'; break;
            case value_t::array_t: {
                oss << '[';
                for (size_t i = 0; i < m_array.size(); i++) {
                    if (i) oss << ',';
                    nl(); pad(depth+1);
                    m_array[i].do_dump(oss, ind, depth+1);
                }
                if (!m_array.empty()) { nl(); pad(depth); }
                oss << ']';
                break;
            }
            case value_t::object_t: {
                oss << '{';
                bool first = true;
                for (const auto& [k,v] : m_object) {
                    if (!first) oss << ',';
                    first = false;
                    nl(); pad(depth+1);
                    oss << '"' << esc(k) << '"' << ':';
                    if (ind >= 0) oss << ' ';
                    v.do_dump(oss, ind, depth+1);
                }
                if (!m_object.empty()) { nl(); pad(depth); }
                oss << '}';
                break;
            }
        }
    }

    static std::string esc(const std::string& s) {
        std::string o;
        for (char c : s) {
            if      (c == '"')  o += "\\\"";
            else if (c == '\\') o += "\\\\";
            else if (c == '\n') o += "\\n";
            else if (c == '\r') o += "\\r";
            else if (c == '\t') o += "\\t";
            else                o += c;
        }
        return o;
    }

    static void skip(const std::string& s, size_t& p) {
        while (p < s.size() && (s[p]==' '||s[p]=='\n'||s[p]=='\r'||s[p]=='\t')) p++;
    }

    static std::string parse_str(const std::string& s, size_t& p) {
        p++; // skip opening "
        std::string o;
        while (p < s.size() && s[p] != '"') {
            if (s[p] == '\\') {
                p++;
                switch (s[p]) {
                    case '"':  o += '"';  break;
                    case '\\': o += '\\'; break;
                    case '/':  o += '/';  break;
                    case 'n':  o += '\n'; break;
                    case 'r':  o += '\r'; break;
                    case 't':  o += '\t'; break;
                    default:   o += s[p]; break;
                }
            } else {
                o += s[p];
            }
            p++;
        }
        p++; // skip closing "
        return o;
    }

    static json parse_val(const std::string& s, size_t& p) {
        skip(s, p);
        if (p >= s.size()) throw std::runtime_error("Unexpected end of JSON input");
        char c = s[p];
        if (c == '"')  return json(parse_str(s, p));
        if (c == '{')  return parse_obj(s, p);
        if (c == '[')  return parse_arr(s, p);
        if (c == 't')  { p+=4; return json(true);  }
        if (c == 'f')  { p+=5; return json(false); }
        if (c == 'n')  { p+=4; return json();       }
        // number
        size_t start = p;
        bool flt = false;
        if (s[p] == '-') p++;
        while (p < s.size() && (std::isdigit((unsigned char)s[p]) || s[p]=='.' || s[p]=='e' || s[p]=='E' || s[p]=='+' || s[p]=='-'))
        { if (s[p]=='.'||s[p]=='e'||s[p]=='E') flt=true; p++; }
        std::string num = s.substr(start, p-start);
        if (flt) return json(std::stod(num));
        return json((long long)std::stoll(num));
    }

    static json parse_obj(const std::string& s, size_t& p) {
        json obj = json::object();
        p++; skip(s, p);
        if (p < s.size() && s[p] == '}') { p++; return obj; }
        while (p < s.size()) {
            skip(s, p);
            std::string key = parse_str(s, p);
            skip(s, p); p++; // skip ':'
            json val = parse_val(s, p);
            obj.m_object[key] = std::move(val);
            skip(s, p);
            if (p < s.size() && s[p] == ',') { p++; continue; }
            if (p < s.size() && s[p] == '}') { p++; break; }
        }
        return obj;
    }

    static json parse_arr(const std::string& s, size_t& p) {
        json arr = json::array();
        p++; skip(s, p);
        if (p < s.size() && s[p] == ']') { p++; return arr; }
        while (p < s.size()) {
            arr.m_array.push_back(parse_val(s, p));
            skip(s, p);
            if (p < s.size() && s[p] == ',') { p++; continue; }
            if (p < s.size() && s[p] == ']') { p++; break; }
        }
        return arr;
    }
};

template<> inline std::string json::get<std::string>() const {
    if (is_string()) return m_string;
    if (m_type == value_t::int_t) return std::to_string(m_int);
    return "";
}
template<> inline int       json::get<int>()       const { return (int)m_int; }
template<> inline long long json::get<long long>() const { return m_int; }
template<> inline double    json::get<double>()    const {
    return (m_type==value_t::float_t) ? m_float : (double)m_int;
}
template<> inline bool      json::get<bool>()      const { return m_bool; }
template<> inline std::vector<std::string> json::get<std::vector<std::string>>() const {
    std::vector<std::string> out;
    for (const auto& v : m_array) out.push_back(v.get<std::string>());
    return out;
}
template<> inline std::vector<json> json::get<std::vector<json>>() const { return m_array; }

} // namespace nlohmann
