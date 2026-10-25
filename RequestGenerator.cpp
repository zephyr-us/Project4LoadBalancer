/**
 * @file RequestGenerator.cpp
 * @brief Implementation of RequestGenerator.
 */
#include "RequestGenerator.h"

RequestGenerator::RequestGenerator(const Config& cfg, const Firewall& fw)
    : cfg_(cfg), fw_(fw), rng_(cfg.seed == 0 ? std::random_device{}() : cfg.seed) {}

std::string RequestGenerator::cleanIp() {
    std::uniform_int_distribution<std::uint32_t> d(0x01000000u, 0xDFFFFFFFu);
    for (int tries = 0; tries < 50; ++tries) {
        std::uint32_t ip = d(rng_);
        if (!fw_.isBlocked(ip)) return Firewall::formatIp(ip);
    }
    return "8.8.8.8";
}

std::string RequestGenerator::badIp() {
    const auto& r = fw_.ranges();
    if (r.empty()) return cleanIp();
    std::uniform_int_distribution<std::size_t> pick(0, r.size() - 1);
    const IpRange& range = r[pick(rng_)];
    std::uniform_int_distribution<std::uint32_t> d(range.lo, range.hi);
    return Firewall::formatIp(d(rng_));
}

Request RequestGenerator::make(int cycle, char forcedType, bool clean) {
    std::uniform_real_distribution<double> u(0.0, 1.0);
    Request r;
    r.jobType = forcedType ? forcedType : (u(rng_) < cfg_.streamShare ? 'S' : 'P');
    int lo = (r.jobType == 'S') ? cfg_.streamMin : cfg_.processMin;
    int hi = (r.jobType == 'S') ? cfg_.streamMax : cfg_.processMax;
    r.cyclesRequired = std::uniform_int_distribution<int>(lo, hi)(rng_);
    r.ipIn = (!clean && u(rng_) < cfg_.badIpShare) ? badIp() : cleanIp();
    r.ipOut = cleanIp();
    r.arrivalCycle = cycle;
    return r;
}

int RequestGenerator::arrivalsThisCycle() {
    double mean = cfg_.arrivalRate;
    if (burstLeft_ > 0) {
        --burstLeft_;
        mean *= cfg_.burstMultiplier;
    } else if (std::uniform_real_distribution<double>(0.0, 1.0)(rng_) < cfg_.burstChance) {
        burstLeft_ = cfg_.burstLength;
        mean *= cfg_.burstMultiplier;
    }
    if (mean <= 0.0) return 0;
    return std::poisson_distribution<int>(mean)(rng_);
}

bool RequestGenerator::inBurst() const { return burstLeft_ > 0; }
