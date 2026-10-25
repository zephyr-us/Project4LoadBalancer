/**
 * @file Firewall.cpp
 * @brief Implementation of Firewall.
 */
#include "Firewall.h"
#include <fstream>
#include <sstream>

namespace {
/** @brief Remove leading and trailing whitespace. */
std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    std::size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
}  // namespace

bool Firewall::parseIp(const std::string& text, std::uint32_t& out) {
    std::uint32_t value = 0;
    int parts = 0;
    std::size_t i = 0;
    while (i <= text.size()) {
        std::size_t j = i;
        int num = 0;
        while (j < text.size() && text[j] >= '0' && text[j] <= '9') {
            num = num * 10 + (text[j] - '0');
            if (num > 255) return false;
            ++j;
        }
        if (j == i) return false;  // empty octet
        value = (value << 8) | static_cast<std::uint32_t>(num);
        ++parts;
        if (j == text.size()) break;
        if (text[j] != '.') return false;
        i = j + 1;
    }
    if (parts != 4) return false;
    out = value;
    return true;
}

std::string Firewall::formatIp(std::uint32_t ip) {
    std::ostringstream os;
    os << ((ip >> 24) & 255) << '.' << ((ip >> 16) & 255) << '.' << ((ip >> 8) & 255) << '.' << (ip & 255);
    return os.str();
}

bool Firewall::parseRange(const std::string& rawLine, IpRange& out) {
    std::string line = trim(rawLine);
    std::uint32_t a = 0, b = 0;
    std::size_t slash = line.find('/');
    std::size_t dash = line.find('-');
    if (slash != std::string::npos) {
        if (!parseIp(trim(line.substr(0, slash)), a)) return false;
        std::string bits = trim(line.substr(slash + 1));
        if (bits.empty() || bits.size() > 2) return false;
        for (char c : bits) if (c < '0' || c > '9') return false;
        int n = std::stoi(bits);
        if (n > 32) return false;
        std::uint32_t mask = (n == 0) ? 0u : (~0u << (32 - n));
        out.lo = a & mask;
        out.hi = out.lo | ~mask;
        return true;
    }
    if (dash != std::string::npos) {
        if (!parseIp(trim(line.substr(0, dash)), a)) return false;
        if (!parseIp(trim(line.substr(dash + 1)), b)) return false;
        if (a > b) return false;
        out.lo = a;
        out.hi = b;
        return true;
    }
    if (!parseIp(line, a)) return false;
    out.lo = out.hi = a;
    return true;
}

bool Firewall::loadRanges(const std::string& path, std::string& error) {
    std::ifstream in(path);
    if (!in) { error = "cannot open firewall file '" + path + "'"; return false; }
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        line = trim(line);
        if (line.empty()) continue;
        IpRange r;
        if (!parseRange(line, r)) {
            error = path + ":" + std::to_string(lineNo) + ": invalid rule '" + line + "'";
            return false;
        }
        ranges_.push_back(r);
    }
    return true;
}

void Firewall::addRange(const IpRange& r) { ranges_.push_back(r); }

bool Firewall::isBlocked(std::uint32_t ip) const {
    for (const IpRange& r : ranges_)
        if (ip >= r.lo && ip <= r.hi) return true;
    return false;
}

bool Firewall::isBlocked(const std::string& ip) const {
    std::uint32_t n;
    if (!parseIp(ip, n)) return true;
    return isBlocked(n);
}

const std::vector<IpRange>& Firewall::ranges() const { return ranges_; }
