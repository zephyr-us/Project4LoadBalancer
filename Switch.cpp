/**
 * @file Switch.cpp
 * @brief Implementation of Switch.
 */
#include "Switch.h"
#include <algorithm>
#include <cmath>
#include <sstream>

Switch::Switch(const Config& cfg, const Firewall& fw, Logger& log)
    : cfg_(cfg), fw_(fw), log_(log),
      stream_("Streaming", cfg, nullptr, log),
      proc_("Processing", cfg, nullptr, log) {}

void Switch::init(int totalServers) {
    int s = std::max(1, static_cast<int>(std::lround(totalServers * cfg_.switchStreamFraction)));
    int p = std::max(1, totalServers - s);
    stream_.init(s);
    proc_.init(p);
}

void Switch::fillQueues(RequestGenerator& gen) {
    stream_.fillQueue(gen, 'S');
    proc_.fillQueue(gen, 'P');
}

bool Switch::route(const Request& r) {
    if (fw_.isBlocked(r.ipIn)) {
        ++blocked_;
        return false;
    }
    if (r.jobType == 'S') { stream_.enqueue(r); ++routedS_; }
    else { proc_.enqueue(r); ++routedP_; }
    return true;
}

void Switch::tick() {
    stream_.tick();
    proc_.tick();
}

std::string Switch::summary() const {
    std::ostringstream o;
    o << "--- Switch ---\n"
      << "  Blocked by firewall: " << blocked_ << "\n"
      << "  Routed to Streaming: " << routedS_ << " new requests\n"
      << "  Routed to Processing: " << routedP_ << " new requests\n"
      << "  Completed in total : " << stream_.stats().completed + proc_.stats().completed << "\n\n"
      << stream_.summary() << "\n" << proc_.summary();
    return o.str();
}
