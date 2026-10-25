/**
 * @file main.cpp
 * @brief Driver for the Zephyr Industries load balancer simulation.
 *
 * Workflow: read settings (config file, command line, prompts) -> build firewall, generator and
 * load balancer (or Switch) -> fill the initial queue -> run the clock loop -> print the summary.
 *
 * Usage: `loadbalancer [-c config] [-s servers] [-t cycles] [-v | -q] [--switch] [-l logfile] [--no-color] [-y]`
 */
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include "Config.h"
#include "Firewall.h"
#include "LoadBalancer.h"
#include "Logger.h"
#include "RequestGenerator.h"
#include "Switch.h"

namespace {

/** @brief Print command-line help. */
void usage() {
    std::cout <<
        "Zephyr Industries Load Balancer\n"
        "Usage: loadbalancer [options]\n"
        "  -c FILE      config file (default config.txt)\n"
        "  -s N         number of starting servers (skips the prompt)\n"
        "  -t N         clock cycles to run (skips the prompt)\n"
        "  -v           verbose log: every scaling event, bursts, periodic status\n"
        "  -q           quiet log: starting and ending summaries only\n"
        "  --switch     use the Switch with separate streaming and processing balancers\n"
        "  --single     use one load balancer\n"
        "  -l FILE      log file path\n"
        "  --no-color   plain console output\n"
        "  -y           do not prompt; use config values for anything not given\n"
        "  -h           this help\n";
}

/**
 * @brief Ask the user for a positive integer, offering a default.
 * @param question Text shown to the user.
 * @param def Value used when the user just presses Enter (or input ends).
 * @return The chosen value.
 */
int promptInt(const std::string& question, int def) {
    while (true) {
        std::cout << question << " [" << def << "]: " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line) || line.empty()) return def;
        try {
            std::size_t used;
            int v = std::stoi(line, &used);
            if (used == line.size() && v >= 1) return v;
        } catch (...) {}
        std::cout << "  Please enter a whole number of 1 or more.\n";
    }
}

}  // namespace

/**
 * @brief Program entry point.
 * @param argc Argument count.
 * @param argv Argument values.
 * @return 0 on success, 1 on a configuration or file error.
 */
int main(int argc, char** argv) {
    std::string configPath = "config.txt", logOverride;
    int optServers = 0, optCycles = 0;
    int verboseOpt = -1, colorOpt = -1, modeOpt = -1;  // -1 = not given
    bool noPrompt = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : ""; };
        if (a == "-c") configPath = next();
        else if (a == "-s") optServers = std::atoi(next().c_str());
        else if (a == "-t") optCycles = std::atoi(next().c_str());
        else if (a == "-v") verboseOpt = 1;
        else if (a == "-q") verboseOpt = 0;
        else if (a == "-l") logOverride = next();
        else if (a == "--switch") modeOpt = 1;
        else if (a == "--single") modeOpt = 0;
        else if (a == "--no-color") colorOpt = 0;
        else if (a == "-y") noPrompt = true;
        else if (a == "-h" || a == "--help") { usage(); return 0; }
        else { std::cerr << "Unknown option '" << a << "'\n"; usage(); return 1; }
    }

    // ---- settings ----
    Logger log;
    Config cfg;
    std::string err;
    if (!cfg.load(configPath, err)) {
        std::cerr << "Config problem: " << err << "\n";
        return 1;
    }
    if (verboseOpt >= 0) cfg.verbose = verboseOpt == 1;
    if (colorOpt >= 0) cfg.color = colorOpt == 1;
    if (modeOpt >= 0) cfg.mode = modeOpt == 1 ? "switch" : "single";
    if (!logOverride.empty()) cfg.logFile = logOverride;
    if (optServers > 0) cfg.servers = optServers;
    else if (!noPrompt) cfg.servers = promptInt("Number of web servers", cfg.servers);
    if (optCycles > 0) cfg.runCycles = optCycles;
    else if (!noPrompt) cfg.runCycles = promptInt("Clock cycles to run", cfg.runCycles);
    if (!cfg.validate(err)) { std::cerr << "Config problem: " << err << "\n"; return 1; }

    log.setVerbose(cfg.verbose);
    log.setColor(cfg.color);
    if (!log.open(cfg.logFile)) { std::cerr << "Cannot write log file '" << cfg.logFile << "'\n"; return 1; }

    // ---- firewall ----
    Firewall fw;
    if (!fw.loadRanges(cfg.blockedRangesFile, err)) {
        log.always(Event::Warning, "Firewall: " + err + " -- running with no blocked ranges");
    }

    // ---- build ----
    RequestGenerator gen(cfg, fw);
    const bool useSwitch = cfg.mode == "switch";
    LoadBalancer lb("LoadBalancer", cfg, &fw, log);
    Switch sw(cfg, fw, log);

    std::ostringstream start;
    start << "Starting Zephyr Industries load balancer (" << (log.verbose() ? "verbose" : "quiet") << " log)\n"
          << cfg.describe() << "  firewall ranges      : " << fw.ranges().size() << " blocked range(s)\n"
          << "  log file             : " << cfg.logFile;
    log.always(Event::Start, start.str());

    if (useSwitch) { sw.init(cfg.servers); sw.fillQueues(gen); }
    else { lb.init(cfg.servers); lb.fillQueue(gen); }
    log.event(Event::Info, "Initial queue filled (" + std::to_string(cfg.queuePerServer) + " requests per server). Clock started.");

    // ---- clock loop ----
    bool wasBurst = false;
    for (int c = 0; c < cfg.runCycles; ++c) {
        int n = gen.arrivalsThisCycle();
        if (gen.inBurst() != wasBurst) {
            wasBurst = gen.inBurst();
            log.event(Event::Burst, "cycle " + std::to_string(c) + ": traffic burst " +
                      (wasBurst ? "STARTED (arrival rate x" + std::to_string(static_cast<int>(cfg.burstMultiplier)) + ")" : "ENDED"));
        }
        for (int i = 0; i < n; ++i) {
            Request r = gen.make(c);
            if (useSwitch) sw.route(r); else lb.accept(r);
        }
        if (useSwitch) sw.tick(); else lb.tick();
    }

    // ---- summary ----
    log.always(Event::Summary, std::string("End of run: ") + std::to_string(cfg.runCycles) + " cycles\n" +
               (useSwitch ? sw.summary() : lb.summary()));
    log.always(Event::Done, "Finished. Log written to " + cfg.logFile);
    return 0;
}
