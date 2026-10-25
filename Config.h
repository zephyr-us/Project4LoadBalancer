/**
 * @file Config.h
 * @brief Declares Config, which holds every tunable setting read from config.txt.
 */
#pragma once
#include <string>

/**
 * @class Config
 * @brief All simulation settings, with defaults, loaded from a `key=value` text file.
 *
 * Changing behaviour never requires editing code: edit config.txt or use the
 * command-line options in main.cpp.
 */
class Config {
public:
    // ---- run size ----
    int servers = 20;              ///< Starting number of web servers (key: servers).
    int runCycles = 100000;        ///< Clock cycles to simulate (key: run_cycles).
    int queuePerServer = 100;      ///< Initial queue size per server (key: queue_per_server).

    // ---- traffic ----
    double arrivalRate = 0.7;      ///< Average new requests per cycle (key: arrival_rate).
    int processMin = 10;           ///< Shortest processing job in cycles (key: process_min_cycles).
    int processMax = 50;           ///< Longest processing job in cycles (key: process_max_cycles).
    int streamMin = 30;            ///< Shortest streaming job in cycles (key: stream_min_cycles).
    int streamMax = 120;           ///< Longest streaming job in cycles (key: stream_max_cycles).
    double streamShare = 0.3;      ///< Fraction of requests that are streaming 'S' (key: stream_share).
    double badIpShare = 0.03;      ///< Fraction of requests sent from blocked ranges (key: bad_ip_share).
    double burstChance = 0.0001;   ///< Chance per cycle that a traffic spike starts (key: burst_chance).
    int burstLength = 500;         ///< Cycles a spike lasts (key: burst_length).
    double burstMultiplier = 3.0;  ///< Arrival rate multiplier during a spike (key: burst_multiplier).

    // ---- scaling ----
    int cooldown = 25;             ///< Cycles to wait after adding/removing a server (key: cooldown).
    int minPerServer = 50;         ///< Lower queue target per server (key: min_per_server).
    int maxPerServer = 80;         ///< Upper queue target per server (key: max_per_server).
    int minServers = 1;            ///< Never go below this many servers (key: min_servers).
    int maxServers = 500;          ///< Never go above this many servers (key: max_servers).

    // ---- bonus / modes ----
    std::string mode = "single";   ///< "single" load balancer or "switch" (key: mode).
    double switchStreamFraction = 0.4;  ///< Share of servers given to the streaming balancer (key: switch_stream_fraction).

    // ---- output ----
    int statusInterval = 5000;     ///< Cycles between status lines in verbose mode (key: status_interval).
    bool verbose = false;          ///< Verbose event log (key: verbose). Overridden by -v / -q.
    bool color = true;             ///< Colored console output (key: color).
    unsigned seed = 42;            ///< Random seed; 0 means a different run every time (key: seed).
    std::string logFile = "loadbalancer.log";            ///< Log file path (key: log_file).
    std::string blockedRangesFile = "blocked_ranges.txt"; ///< Firewall rules file (key: blocked_ranges_file).

    /**
     * @brief Read settings from a file and validate them.
     * @param path Path to the config file.
     * @param error Receives a readable message on failure.
     * @return true if the file was read and all values are valid.
     */
    bool load(const std::string& path, std::string& error);

    /**
     * @brief Check that the current values make sense together.
     * @param error Receives the first problem found.
     * @return true if valid.
     */
    bool validate(std::string& error) const;

    /** @brief Multi-line text listing every setting, used in the starting summary. */
    std::string describe() const;

private:
    /** @brief Apply one key/value pair. @return false (with error set) if the key or value is bad. */
    bool set(const std::string& key, const std::string& value, std::string& error);
};
