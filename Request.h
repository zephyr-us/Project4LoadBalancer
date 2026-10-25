/**
 * @file Request.h
 * @brief Defines the Request struct, the unit of work moved through the load balancer.
 * @author Zephyr Industries
 */
#pragma once
#include <string>

/**
 * @struct Request
 * @brief One web request: who sent it, where the result goes, how long it takes, and what kind it is.
 *
 * A Request is created by the RequestGenerator, screened by the Firewall,
 * waits in a RequestQueue, and is finally processed by a WebServer.
 */
struct Request {
    std::string ipIn;        ///< IPv4 address of the requester (dotted decimal), checked by the Firewall.
    std::string ipOut;       ///< IPv4 address the result is sent to.
    int cyclesRequired = 1;  ///< Clock cycles a WebServer needs to finish this request.
    char jobType = 'P';      ///< 'P' = processing job, 'S' = streaming job.
    int arrivalCycle = 0;    ///< Clock cycle at which the request entered the system (used for wait time).
};
