/**
 * @file RequestQueue.cpp
 * @brief Implementation of RequestQueue.
 */
#include "RequestQueue.h"
#include <stdexcept>

void RequestQueue::push(const Request& r) { items_.push(r); }

Request RequestQueue::pop() {
    if (items_.empty()) throw std::out_of_range("RequestQueue::pop on empty queue");
    Request r = items_.front();
    items_.pop();
    return r;
}

std::size_t RequestQueue::size() const { return items_.size(); }

bool RequestQueue::empty() const { return items_.empty(); }
