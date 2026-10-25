/**
 * @file WebServer.h
 * @brief Declares WebServer, a worker that processes one Request at a time.
 */
#pragma once
#include "Request.h"

/**
 * @class WebServer
 * @brief Simulated web server.
 *
 * A server is either idle or busy. The LoadBalancer gives an idle server a
 * request with assign(); every clock cycle it calls tick(), and when the
 * request is finished the server is idle again and "asks for another".
 * A server marked as draining finishes its current job and then accepts no more,
 * so it can be removed from the pool without losing a request.
 */
class WebServer {
public:
    /**
     * @brief Create an idle server.
     * @param id Unique number used in log messages.
     */
    explicit WebServer(int id);

    /** @brief Unique id of this server. */
    int id() const;

    /** @brief True when the server has no job. */
    bool isIdle() const;

    /** @brief True when the server is waiting to be removed after its current job. */
    bool isDraining() const;

    /** @brief Stop accepting new work; the server leaves the pool once idle. */
    void markDraining();

    /** @brief Cycles left on the current job (0 when idle). */
    int cyclesLeft() const;

    /**
     * @brief Start working on a request.
     * @param r The request to process. Its cyclesRequired sets the duration (minimum 1).
     */
    void assign(const Request& r);

    /**
     * @brief Advance this server by one clock cycle.
     * @param finished If not null and a job completes this cycle, receives that job.
     * @return true if a job finished during this cycle.
     */
    bool tick(Request* finished = nullptr);

private:
    int id_;                 ///< Server number.
    bool busy_ = false;      ///< True while a job is in progress.
    bool draining_ = false;  ///< True once marked for removal.
    int cyclesLeft_ = 0;     ///< Remaining cycles for the current job.
    Request current_;        ///< Job being processed.
};
