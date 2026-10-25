/**
 * @file Config.cpp
 * @brief Implementation of Config.
 */
#include "Config.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace {
/** @brief Trim whitespace from both ends. */
std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    std::size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
/** @brief Parse an int; false if the text is not entirely a number. */
bool toInt(const std::string& v, int& out) {
    try { std::size_t n; int x = std::stoi(v, &n); if (n != v.size()) return false; out = x; return true; }
    catch (...) { return false; }
}
/** @brief Parse a double; false if the text is not entirely a number. */
bool toDouble(const std::string& v, double& out) {
    try { std::size_t n; double x = std::stod(v, &n); if (n != v.size()) return false; out = x; return true; }
    catch (...) { return false; }
}
/** @brief Parse true/false/yes/no/1/0. */
bool toBool(const std::string& v, bool& out) {
    std::string s = v;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "true" || s == "yes" || s == "1" || s == "on") { out = true; return true; }
    if (s == "false" || s == "no" || s == "0" || s == "off") { out = false; return true; }
    return false;
}
}  // namespace

bool Config::set(const std::string& key, const std::string& v, std::string& error) {
    int i = 0; double d = 0; bool b = false;
    auto badValue = [&]() { error = "bad value '" + v + "' for '" + key + "'"; return false; };
#define SET_INT(name, field)    if (key == name) { if (!toInt(v, i)) return badValue(); field = i; return true; }
#define SET_DOUBLE(name, field) if (key == name) { if (!toDouble(v, d)) return badValue(); field = d; return true; }
#define SET_BOOL(name, field)   if (key == name) { if (!toBool(v, b)) return badValue(); field = b; return true; }
    SET_INT("servers", servers)
    SET_INT("run_cycles", runCycles)
    SET_INT("queue_per_server", queuePerServer)
    SET_DOUBLE("arrival_rate", arrivalRate)
    SET_INT("process_min_cycles", processMin)
    SET_INT("process_max_cycles", processMax)
    SET_INT("stream_min_cycles", streamMin)
    SET_INT("stream_max_cycles", streamMax)
    SET_DOUBLE("stream_share", streamShare)
    SET_DOUBLE("bad_ip_share", badIpShare)
    SET_DOUBLE("burst_chance", burstChance)
    SET_INT("burst_length", burstLength)
    SET_DOUBLE("burst_multiplier", burstMultiplier)
    SET_INT("cooldown", cooldown)
    SET_INT("min_per_server", minPerServer)
    SET_INT("max_per_server", maxPerServer)
    SET_INT("min_servers", minServers)
    SET_INT("max_servers", maxServers)
    SET_DOUBLE("switch_stream_fraction", switchStreamFraction)
    SET_INT("status_interval", statusInterval)
    SET_BOOL("verbose", verbose)
    SET_BOOL("color", color)
    if (key == "seed") { if (!toInt(v, i) || i < 0) return badValue(); seed = static_cast<unsigned>(i); return true; }
    if (key == "mode") { mode = v; return true; }
    if (key == "log_file") { logFile = v; return true; }
    if (key == "blocked_ranges_file") { blockedRangesFile = v; return true; }
#undef SET_INT
#undef SET_DOUBLE
#undef SET_BOOL
    error = "unknown setting '" + key + "'";
    return false;
}

bool Config::load(const std::string& path, std::string& error) {
    std::ifstream in(path);
    if (!in) { error = "cannot open config file '" + path + "'"; return false; }
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::size_t hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        line = trim(line);
        if (line.empty()) continue;
        std::size_t eq = line.find('=');
        if (eq == std::string::npos) { error = path + ":" + std::to_string(lineNo) + ": expected key=value"; return false; }
        std::string key = trim(line.substr(0, eq));
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        std::string err;
        if (!set(key, trim(line.substr(eq + 1)), err)) { error = path + ":" + std::to_string(lineNo) + ": " + err; return false; }
    }
    return validate(error);
}

bool Config::validate(std::string& error) const {
    auto fail = [&](const std::string& m) { error = m; return false; };
    if (servers < 1) return fail("servers must be at least 1");
    if (runCycles < 1) return fail("run_cycles must be at least 1");
    if (queuePerServer < 0) return fail("queue_per_server cannot be negative");
    if (arrivalRate < 0) return fail("arrival_rate cannot be negative");
    if (processMin < 1 || processMax < processMin) return fail("process_min/max_cycles must satisfy 1 <= min <= max");
    if (streamMin < 1 || streamMax < streamMin) return fail("stream_min/max_cycles must satisfy 1 <= min <= max");
    if (streamShare < 0 || streamShare > 1) return fail("stream_share must be between 0 and 1");
    if (badIpShare < 0 || badIpShare > 1) return fail("bad_ip_share must be between 0 and 1");
    if (burstChance < 0 || burstChance > 1) return fail("burst_chance must be between 0 and 1");
    if (burstLength < 1) return fail("burst_length must be at least 1");
    if (burstMultiplier < 1) return fail("burst_multiplier must be at least 1");
    if (cooldown < 0) return fail("cooldown cannot be negative");
    if (minPerServer < 0 || maxPerServer < minPerServer) return fail("need 0 <= min_per_server <= max_per_server");
    if (minServers < 1 || maxServers < minServers) return fail("need 1 <= min_servers <= max_servers");
    if (mode != "single" && mode != "switch") return fail("mode must be 'single' or 'switch'");
    if (switchStreamFraction <= 0 || switchStreamFraction >= 1) return fail("switch_stream_fraction must be between 0 and 1 (exclusive)");
    if (statusInterval < 0) return fail("status_interval cannot be negative");
    return true;
}

std::string Config::describe() const {
    std::ostringstream o;
    o << "  mode                 : " << mode << "\n"
      << "  starting servers     : " << servers << "  (min " << minServers << ", max " << maxServers << ")\n"
      << "  run length           : " << runCycles << " cycles\n"
      << "  initial queue        : " << queuePerServer << " per server\n"
      << "  arrival rate         : " << arrivalRate << " requests/cycle\n"
      << "  processing job time  : " << processMin << "-" << processMax << " cycles\n"
      << "  streaming job time   : " << streamMin << "-" << streamMax << " cycles (" << streamShare * 100 << "% of requests)\n"
      << "  traffic bursts       : chance " << burstChance << "/cycle, " << burstLength << " cycles, x" << burstMultiplier << "\n"
      << "  bad-IP share         : " << badIpShare * 100 << "%\n"
      << "  queue target         : " << minPerServer << "-" << maxPerServer << " per server\n"
      << "  cooldown             : " << cooldown << " cycles\n"
      << "  random seed          : " << (seed == 0 ? std::string("random") : std::to_string(seed)) << "\n";
    if (mode == "switch") o << "  streaming server share: " << switchStreamFraction * 100 << "%\n";
    return o.str();
}
