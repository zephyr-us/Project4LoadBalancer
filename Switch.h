/**
 * @file Switch.h
 * @brief Declares Switch, the bonus higher-level router that sends jobs to type-specific load balancers.
 */
#pragma once
#include <string>
#include "LoadBalancer.h"

/**
 * @class Switch
 * @brief Screens every request with the Firewall once, then routes it by job type.
 *
 * Streaming ('S') requests go to one LoadBalancer and processing ('P') requests to another.
 * Each balancer has its own queue, server pool, scaling and statistics, and they run side by side.
 */
class Switch {
public:
    /**
     * @brief Create the switch and its two balancers.
     * @param cfg Settings; must outlive the switch.
     * @param fw Firewall applied before routing; must outlive the switch.
     * @param log Logger; must outlive the switch.
     */
    Switch(const Config& cfg, const Firewall& fw, Logger& log);

    /**
     * @brief Split the total server count between the two balancers using switch_stream_fraction.
     * @param totalServers Servers to divide (each balancer gets at least one).
     */
    void init(int totalServers);

    /** @brief Fill each balancer's queue with requests of its own job type. @param gen Request generator. */
    void fillQueues(RequestGenerator& gen);

    /**
     * @brief Firewall-check a request and send it to the matching balancer.
     * @param r The incoming request.
     * @return true if routed, false if blocked.
     */
    bool route(const Request& r);

    /** @brief Advance both balancers by one clock cycle. */
    void tick();

    /** @brief Report: switch totals followed by both balancer summaries. */
    std::string summary() const;

private:
    const Config& cfg_;    ///< Settings.
    const Firewall& fw_;   ///< Firewall.
    Logger& log_;          ///< Logger.
    LoadBalancer stream_;  ///< Handles 'S' jobs.
    LoadBalancer proc_;    ///< Handles 'P' jobs.
    long blocked_ = 0;     ///< Requests blocked at the switch.
    long routedS_ = 0;     ///< Requests sent to the streaming balancer.
    long routedP_ = 0;     ///< Requests sent to the processing balancer.
};
