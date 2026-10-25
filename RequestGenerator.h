/**
 * @file RequestGenerator.h
 * @brief Declares RequestGenerator, which creates random requests and decides when they arrive.
 */
#pragma once
#include <random>
#include "Config.h"
#include "Firewall.h"
#include "Request.h"

/**
 * @class RequestGenerator
 * @brief Source of random traffic, including occasional traffic spikes and requests from blocked IP ranges.
 */
class RequestGenerator {
public:
    /**
     * @brief Create a generator.
     * @param cfg Settings (rates, job times, seed). Must outlive the generator.
     * @param fw Firewall whose blocked ranges supply "bad" source addresses. Must outlive the generator.
     */
    RequestGenerator(const Config& cfg, const Firewall& fw);

    /**
     * @brief Build one random request.
     * @param cycle Clock cycle recorded as the arrival time.
     * @param forcedType 'P' or 'S' to force a job type; 0 picks randomly using stream_share.
     * @param clean If true, the source IP is never from a blocked range (used for the initial queue).
     * @return The new request.
     */
    Request make(int cycle, char forcedType = 0, bool clean = false);

    /**
     * @brief How many new requests arrive this cycle (random, Poisson with the current rate).
     *
     * Also advances the burst state, so call exactly once per cycle.
     * @return Number of arrivals, usually 0 or 1, more during a burst.
     */
    int arrivalsThisCycle();

    /** @brief True while a traffic spike is in progress. */
    bool inBurst() const;

private:
    /** @brief Random public-looking address that is not blocked. */
    std::string cleanIp();
    /** @brief Random address inside a blocked range (falls back to clean if no ranges). */
    std::string badIp();

    const Config& cfg_;       ///< Settings.
    const Firewall& fw_;      ///< Firewall rules.
    std::mt19937 rng_;        ///< Random number engine.
    int burstLeft_ = 0;       ///< Cycles remaining in the current spike.
};
