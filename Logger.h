/**
 * @file Logger.h
 * @brief Declares Logger, which writes colored console output and a plain-text log file.
 */
#pragma once
#include <fstream>
#include <string>

/**
 * @enum Event
 * @brief Kinds of log events; each has its own tag and console color.
 */
enum class Event {
    Info,           ///< General information (default color)
    Start,          ///< Starting summary (bold)
    ServerAdded,    ///< A server was added (blue)
    ServerRemoved,  ///< A server was removed (yellow)
    Blocked,        ///< Firewall blocked traffic (red)
    Burst,          ///< Traffic spike started or ended (magenta)
    Status,         ///< Periodic status line (cyan)
    Summary,        ///< Final summary (bold)
    Done,           ///< Run finished successfully (green)
    Warning,        ///< Something unusual but not fatal (bold yellow)
    Error           ///< A problem that stops the run (bold red)
};

/**
 * @class Logger
 * @brief Two output levels: **verbose** (all events) and **quiet** (start and end summaries only).
 *
 * The console gets ANSI colors (when enabled); the log file never does.
 */
class Logger {
public:
    /**
     * @brief Open the log file for writing (replaces any old file).
     * @param path File to write.
     * @return true if the file was opened.
     */
    bool open(const std::string& path);

    /** @brief Turn verbose mode on or off. @param on true for verbose, false for quiet. */
    void setVerbose(bool on);

    /** @brief Turn console colors on or off. @param on true to use ANSI colors. */
    void setColor(bool on);

    /** @brief True when in verbose mode. */
    bool verbose() const;

    /**
     * @brief Log an event that appears only in verbose mode.
     * @param e Event kind (sets tag and color).
     * @param msg Message text (may contain several lines).
     */
    void event(Event e, const std::string& msg);

    /**
     * @brief Log a message shown in both verbose and quiet mode (starting/ending summaries, errors).
     * @param e Event kind.
     * @param msg Message text.
     */
    void always(Event e, const std::string& msg);

private:
    /** @brief Write a message to console and file. */
    void write(Event e, const std::string& msg);

    std::ofstream file_;    ///< Log file.
    bool verbose_ = false;  ///< Verbose mode flag.
    bool color_ = true;     ///< Console color flag.
};
