/**
 * @file Logger.cpp
 * @brief Implementation of Logger.
 */
#include "Logger.h"
#include <iostream>

namespace {
/** @brief Short tag printed before each message. */
const char* tag(Event e) {
    switch (e) {
        case Event::Info: return "[INFO   ]";
        case Event::Start: return "[START  ]";
        case Event::ServerAdded: return "[ADD    ]";
        case Event::ServerRemoved: return "[REMOVE ]";
        case Event::Blocked: return "[BLOCKED]";
        case Event::Burst: return "[BURST  ]";
        case Event::Status: return "[STATUS ]";
        case Event::Summary: return "[SUMMARY]";
        case Event::Done: return "[DONE   ]";
        case Event::Warning: return "[WARNING]";
        case Event::Error: return "[ERROR  ]";
    }
    return "[?]";
}
/** @brief ANSI color code for an event. */
const char* colorCode(Event e) {
    switch (e) {
        case Event::Info: return "";
        case Event::Start: return "\033[1m";
        case Event::ServerAdded: return "\033[94m";
        case Event::ServerRemoved: return "\033[33m";
        case Event::Blocked: return "\033[31m";
        case Event::Burst: return "\033[35m";
        case Event::Status: return "\033[36m";
        case Event::Summary: return "\033[1m";
        case Event::Done: return "\033[32m";
        case Event::Warning: return "\033[1;33m";
        case Event::Error: return "\033[1;31m";
    }
    return "";
}
}  // namespace

bool Logger::open(const std::string& path) {
    file_.open(path, std::ios::out | std::ios::trunc);
    return file_.is_open();
}

void Logger::setVerbose(bool on) { verbose_ = on; }
void Logger::setColor(bool on) { color_ = on; }
bool Logger::verbose() const { return verbose_; }

void Logger::event(Event e, const std::string& msg) {
    if (verbose_) write(e, msg);
}

void Logger::always(Event e, const std::string& msg) { write(e, msg); }

void Logger::write(Event e, const std::string& msg) {
    std::string line = std::string(tag(e)) + " " + msg;
    std::ostream& out = (e == Event::Error) ? std::cerr : std::cout;
    const char* c = colorCode(e);
    if (color_ && *c) out << c << line << "\033[0m\n";
    else out << line << "\n";
    if (file_.is_open()) file_ << line << "\n" << std::flush;
}
