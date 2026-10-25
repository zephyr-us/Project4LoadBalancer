/**
 * @file LoadBalancer.h
 * @brief Declares LoadBalancer and its Stats, the heart of the simulation.
 */
#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "Config.h"
#include "Firewall.h"
#include "Logger.h"
#include "RequestGenerator.h"
#include "RequestQueue.h"
#include "WebServer.h"

/**
 * @struct Stats
 * @brief Counters collected by one LoadBalancer for the summary report.
 */
struct Stats {
    long initialQueue = 0;    ///< Requests placed in the queue before the clock started.
    long accepted = 0;        ///< Requests that entered the queue (including the initial queue).
    long blocked = 0;         ///< Requests rejected by the firewall.
    long completed = 0;       ///< Requests finished by a server.
    long completedP = 0;      ///< Finished processing ('P') jobs.
    long completedS = 0;      ///< Finished streaming ('S') jobs.
    int serversAdded = 0;     ///< Times a server was added.
    int serversRemoved = 0;   ///< Times a server was removed.
    int startServers = 0;     ///< Servers at the start.
    int minServers = 0;       ///< Smallest pool size seen.
    int maxServers = 0;       ///< Largest pool size seen.
    std::size_t queueMin = static_cast<std::size_t>(-1);  ///< Smallest queue size seen.
    std::size_t queueMax = 0; ///< Largest queue size seen.
    double queueSum = 0;      ///< Sum of queue sizes over all cycles (for the average).
    long samples = 0;         ///< Cycles sampled.
    long started = 0;         ///< Requests handed to a server.
    long waitSum = 0;         ///< Total cycles started requests waited before service.
    int waitMax = 0;          ///< Longest wait for a single request.
    long busyCycles = 0;      ///< Sum over cycles of busy servers.
    long serverCycles = 0;    ///< Sum over cycles of servers in the pool.
};

/**
 * @class LoadBalancer
 * @brief Manages a request queue and a pool of web servers, and resizes the pool to match demand.
 *
 * Each clock cycle (tick) it advances every server, hands queued requests to idle servers,
 * records statistics, and after a cooldown compares the queue size with
 * `min_per_server * N` and `max_per_server * N` to remove or add one server.
 * Several LoadBalancer objects can exist at once (see Switch), each with its own Stats.
 */
class LoadBalancer {
public:
    /**
     * @brief Create a load balancer.
     * @param name Label used in logs (e.g. "Streaming").
     * @param cfg Settings; must outlive this object.
     * @param fw Firewall to screen requests in accept(), or nullptr if screening is done elsewhere.
     * @param log Logger for events; must outlive this object.
     */
    LoadBalancer(const std::string& name, const Config& cfg, const Firewall* fw, Logger& log);

    /** @brief Create the starting pool of idle servers. @param servers Number of servers (at least 1). */
    void init(int servers);

    /**
     * @brief Fill the queue with `queue_per_server * servers` random requests before the clock starts.
     * @param gen Generator producing the requests.
     * @param type 'P' or 'S' to force a job type, or 0 for a random mix.
     */
    void fillQueue(RequestGenerator& gen, char type = 0);

    /**
     * @brief Screen a request with the firewall and queue it if allowed.
     * @param r The incoming request.
     * @return true if queued, false if blocked.
     */
    bool accept(const Request& r);

    /** @brief Put a request in the queue without screening. @param r The request. */
    void enqueue(const Request& r);

    /** @brief Advance one clock cycle: work, dispatch, sample statistics, check scaling. */
    void tick();

    /** @brief Servers currently in the pool and accepting work (draining servers excluded). */
    int activeServers() const;

    /** @brief Requests waiting in the queue. */
    std::size_t queueSize() const;

    /** @brief Current clock cycle. */
    int cycle() const;

    /** @brief Name given at construction. */
    const std::string& name() const;

    /** @brief Collected statistics. */
    const Stats& stats() const;

    /** @brief Multi-line end-of-run report for this balancer. */
    std::string summary() const;

    /** @brief One-line snapshot of the current state (used for periodic status). */
    std::string statusLine() const;

private:
    /** @brief After the cooldown, add or remove one server if the queue is out of range. */
    void checkScaling();
    /** @brief Add one server. @return false if already at max_servers. */
    bool addServer();
    /** @brief Remove one server (idle first, otherwise drain a busy one). @return false if at min_servers. */
    bool removeServer();
    /** @brief Update min/max pool-size statistics. */
    void trackPool();

    std::string name_;               ///< Label for logs.
    const Config& cfg_;              ///< Settings.
    const Firewall* fw_;             ///< Optional firewall.
    Logger& log_;                    ///< Logger.
    RequestQueue queue_;             ///< Waiting requests.
    std::vector<WebServer> servers_; ///< Server pool.
    int nextId_ = 1;                 ///< Id for the next server created.
    int cycle_ = 0;                  ///< Clock.
    int cooldownLeft_ = 0;           ///< Cycles until scaling may be checked again.
    long blockedSinceStatus_ = 0;    ///< Blocked count since the last status line.
    bool firstBlockLogged_ = false;  ///< True once the first blocked IP was logged.
    Stats stats_;                    ///< Statistics.
};
