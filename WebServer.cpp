/**
 * @file WebServer.cpp
 * @brief Implementation of WebServer.
 */
#include "WebServer.h"
#include <algorithm>

WebServer::WebServer(int id) : id_(id) {}
int WebServer::id() const { return id_; }
bool WebServer::isIdle() const { return !busy_; }
bool WebServer::isDraining() const { return draining_; }
void WebServer::markDraining() { draining_ = true; }
int WebServer::cyclesLeft() const { return cyclesLeft_; }

void WebServer::assign(const Request& r) {
    current_ = r;
    cyclesLeft_ = std::max(1, r.cyclesRequired);
    busy_ = true;
}

bool WebServer::tick(Request* finished) {
    if (!busy_) return false;
    if (--cyclesLeft_ <= 0) {
        busy_ = false;
        cyclesLeft_ = 0;
        if (finished) *finished = current_;
        return true;
    }
    return false;
}
