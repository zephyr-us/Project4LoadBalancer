/**
 * @file RequestQueue.h
 * @brief Declares RequestQueue, a first-in first-out line of Request objects.
 */
#pragma once
#include <cstddef>
#include <queue>
#include "Request.h"

/**
 * @class RequestQueue
 * @brief FIFO queue of waiting requests, owned by a LoadBalancer.
 */
class RequestQueue {
public:
    /**
     * @brief Add a request to the back of the line.
     * @param r The request to store (copied).
     */
    void push(const Request& r);

    /**
     * @brief Remove and return the request at the front of the line.
     * @return The oldest waiting request.
     * @throws std::out_of_range if the queue is empty (check empty() first).
     */
    Request pop();

    /** @brief Number of requests currently waiting. */
    std::size_t size() const;

    /** @brief True when no requests are waiting. */
    bool empty() const;

private:
    std::queue<Request> items_;  ///< Underlying storage.
};
