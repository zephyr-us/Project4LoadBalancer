/**
 * @file Firewall.h
 * @brief Declares Firewall, an IP range blocker used to stop unwanted or attacking traffic.
 */
#pragma once
#include <cstdint>
#include <string>
#include <vector>

/**
 * @struct IpRange
 * @brief Inclusive range of IPv4 addresses stored as 32-bit numbers.
 */
struct IpRange {
    std::uint32_t lo;  ///< First address in the range.
    std::uint32_t hi;  ///< Last address in the range.
};

/**
 * @class Firewall
 * @brief Blocks requests whose source IP falls inside any configured range.
 *
 * Ranges can be written in a rules file as `a.b.c.d-e.f.g.h`, as CIDR `a.b.c.d/nn`,
 * or as a single address. Lines starting with `#` are comments.
 */
class Firewall {
public:
    /**
     * @brief Load blocked ranges from a text file (one rule per line).
     * @param path Path to the rules file.
     * @param error Receives a message when loading fails.
     * @return true on success, false if the file cannot be opened or a line is invalid.
     */
    bool loadRanges(const std::string& path, std::string& error);

    /** @brief Add one range to the block list. @param r Range to add. */
    void addRange(const IpRange& r);

    /**
     * @brief Decide whether an address is blocked.
     * @param ip Dotted-decimal IPv4 string.
     * @return true if the address is inside a blocked range. Unparseable addresses are blocked.
     */
    bool isBlocked(const std::string& ip) const;

    /** @brief Numeric version of isBlocked(). @param ip Address as a 32-bit number. */
    bool isBlocked(std::uint32_t ip) const;

    /** @brief All configured ranges. */
    const std::vector<IpRange>& ranges() const;

    /**
     * @brief Convert dotted decimal text to a number.
     * @param text Input such as "192.168.1.5".
     * @param out Receives the number on success.
     * @return true if the text is a valid IPv4 address.
     */
    static bool parseIp(const std::string& text, std::uint32_t& out);

    /** @brief Convert a number to dotted decimal text. @param ip Address as a 32-bit number. */
    static std::string formatIp(std::uint32_t ip);

    /**
     * @brief Parse one rule line (range, CIDR or single address).
     * @param line Rule text.
     * @param out Receives the range on success.
     * @return true if the rule is valid.
     */
    static bool parseRange(const std::string& line, IpRange& out);

private:
    std::vector<IpRange> ranges_;  ///< Blocked ranges.
};
