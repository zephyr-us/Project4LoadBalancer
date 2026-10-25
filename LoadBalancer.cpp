/**
 * @file LoadBalancer.cpp
 * @brief Implementation of LoadBalancer.
 */
#include "LoadBalancer.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

LoadBalancer::LoadBalancer(const std::string& name, const Config& cfg, const Firewall* fw, Logger& log)
    : name_(name), cfg_(cfg), fw_(fw), log_(log) {}

void LoadBalancer::init(int servers) {
    servers_.clear();
    for (int i = 0; i < servers; ++i) servers_.emplace_back(nextId_++);
    stats_.startServers = stats_.minServers = stats_.maxServers = servers;
}

void LoadBalancer::fillQueue(RequestGenerator& gen, char type) {
    long count = static_cast<long>(cfg_.queuePerServer) * activeServers();
    for (long i = 0; i < count; ++i) enqueue(gen.make(0, type, true));
    stats_.initialQueue = count;
}

void LoadBalancer::enqueue(const Request& r) {
    queue_.push(r);
    ++stats_.accepted;
}

bool LoadBalancer::accept(const Request& r) {
    if (fw_ && fw_->isBlocked(r.ipIn)) {
        ++stats_.blocked;
        ++blockedSinceStatus_;
        if (!firstBlockLogged_) {
            firstBlockLogged_ = true;
            log_.event(Event::Blocked, "[" + name_ + "] cycle " + std::to_string(cycle_) +
                       ": firewall blocked first request from " + r.ipIn + " (further blocks are counted in status lines)");
        }
        return false;
    }
    enqueue(r);
    return true;
}

int LoadBalancer::activeServers() const {
    int n = 0;
    for (const WebServer& s : servers_) if (!s.isDraining()) ++n;
    return n;
}

std::size_t LoadBalancer::queueSize() const { return queue_.size(); }
int LoadBalancer::cycle() const { return cycle_; }
const std::string& LoadBalancer::name() const { return name_; }
const Stats& LoadBalancer::stats() const { return stats_; }

void LoadBalancer::trackPool() {
    int n = activeServers();
    stats_.minServers = std::min(stats_.minServers, n);
    stats_.maxServers = std::max(stats_.maxServers, n);
}

void LoadBalancer::tick() {
    // 1. Advance servers; collect finished jobs.
    Request done;
    for (WebServer& s : servers_) {
        if (s.tick(&done)) {
            ++stats_.completed;
            if (done.jobType == 'S') ++stats_.completedS; else ++stats_.completedP;
        }
    }
    // 2. Drop draining servers that have finished their last job.
    servers_.erase(std::remove_if(servers_.begin(), servers_.end(),
                   [](const WebServer& s) { return s.isDraining() && s.isIdle(); }), servers_.end());
    // 3. Idle servers ask for another request.
    for (WebServer& s : servers_) {
        if (s.isIdle() && !s.isDraining() && !queue_.empty()) {
            Request r = queue_.pop();
            int wait = cycle_ - r.arrivalCycle;
            ++stats_.started;
            stats_.waitSum += wait;
            stats_.waitMax = std::max(stats_.waitMax, wait);
            s.assign(r);
        }
    }
    // 4. Statistics for this cycle.
    std::size_t q = queue_.size();
    stats_.queueMin = std::min(stats_.queueMin, q);
    stats_.queueMax = std::max(stats_.queueMax, q);
    stats_.queueSum += static_cast<double>(q);
    ++stats_.samples;
    stats_.serverCycles += static_cast<long>(servers_.size());
    for (const WebServer& s : servers_) if (!s.isIdle()) ++stats_.busyCycles;
    // 5. Scaling decision.
    checkScaling();
    // 6. Periodic status line (verbose only).
    if (cfg_.statusInterval > 0 && cycle_ > 0 && cycle_ % cfg_.statusInterval == 0) {
        log_.event(Event::Status, statusLine());
        blockedSinceStatus_ = 0;
    }
    ++cycle_;
}

void LoadBalancer::checkScaling() {
    if (cooldownLeft_ > 0) { --cooldownLeft_; return; }
    int n = activeServers();
    long q = static_cast<long>(queue_.size());
    long low = static_cast<long>(cfg_.minPerServer) * n;
    long high = static_cast<long>(cfg_.maxPerServer) * n;
    std::ostringstream m;
    if (q < low) {
        m << "[" << name_ << "] cycle " << cycle_ << ": queue " << q << " < " << cfg_.minPerServer << "x" << n
          << "=" << low << " -> removing a server, pool " << n << " -> " << n - 1;
        if (removeServer()) { log_.event(Event::ServerRemoved, m.str()); cooldownLeft_ = cfg_.cooldown; }
    } else if (q > high) {
        m << "[" << name_ << "] cycle " << cycle_ << ": queue " << q << " > " << cfg_.maxPerServer << "x" << n
          << "=" << high << " -> adding a server, pool " << n << " -> " << n + 1;
        if (addServer()) { log_.event(Event::ServerAdded, m.str()); cooldownLeft_ = cfg_.cooldown; }
    }
}

bool LoadBalancer::addServer() {
    if (activeServers() >= cfg_.maxServers) return false;
    servers_.emplace_back(nextId_++);
    ++stats_.serversAdded;
    trackPool();
    return true;
}

bool LoadBalancer::removeServer() {
    if (activeServers() <= cfg_.minServers) return false;
    // Prefer an idle server: it can leave immediately.
    for (auto it = servers_.begin(); it != servers_.end(); ++it) {
        if (it->isIdle() && !it->isDraining()) {
            servers_.erase(it);
            ++stats_.serversRemoved;
            trackPool();
            return true;
        }
    }
    // Otherwise let the server with the least work left finish and then leave.
    WebServer* best = nullptr;
    for (WebServer& s : servers_)
        if (!s.isDraining() && (!best || s.cyclesLeft() < best->cyclesLeft())) best = &s;
    if (!best) return false;
    best->markDraining();
    ++stats_.serversRemoved;
    trackPool();
    return true;
}

std::string LoadBalancer::statusLine() const {
    int busy = 0;
    for (const WebServer& s : servers_) if (!s.isIdle()) ++busy;
    int n = activeServers();
    std::ostringstream o;
    o << "[" << name_ << "] cycle " << cycle_ << ": servers " << n << " (" << busy << " busy), queue "
      << queue_.size() << " (target " << cfg_.minPerServer * n << "-" << cfg_.maxPerServer * n << "), completed "
      << stats_.completed << ", blocked " << stats_.blocked << " (+" << blockedSinceStatus_ << " since last status)";
    return o.str();
}

std::string LoadBalancer::summary() const {
    const Stats& s = stats_;
    int n = activeServers();
    std::ostringstream o;
    o << std::fixed << std::setprecision(1);
    o << "--- " << name_ << " ---\n"
      << "  Cycles run        : " << cycle_ << "\n"
      << "  Servers           : start " << s.startServers << ", end " << n << ", min " << s.minServers
      << ", max " << s.maxServers << " (added " << s.serversAdded << ", removed " << s.serversRemoved << ")\n"
      << "  Requests accepted : " << s.accepted << " (" << s.initialQueue << " initial + " << s.accepted - s.initialQueue << " new)\n"
      << (fw_ ? "  Requests blocked  : " + std::to_string(s.blocked) + " by firewall\n" : std::string())
      << "  Requests completed: " << s.completed << " (processing " << s.completedP << ", streaming " << s.completedS << ")\n"
      << "  Still in queue    : " << queue_.size() << "\n"
      << "  Queue size        : min " << (s.samples ? s.queueMin : 0) << ", max " << s.queueMax << ", avg "
      << (s.samples ? s.queueSum / s.samples : 0.0) << "; final target " << cfg_.minPerServer * n << "-" << cfg_.maxPerServer * n << "\n"
      << "  Wait before start : avg " << (s.started ? static_cast<double>(s.waitSum) / s.started : 0.0)
      << " cycles, max " << s.waitMax << " cycles\n"
      << "  Server utilization: " << (s.serverCycles ? 100.0 * s.busyCycles / s.serverCycles : 0.0) << "%\n";
    return o.str();
}
