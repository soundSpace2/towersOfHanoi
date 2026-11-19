// towers.cpp
//
// Answers two questions about the Towers of Hanoi:
//   1. How many moves does a puzzle with n disks need?  (2^n - 1)
//   2. How long will that take at a given moves-per-second rate?
//
// Both answers grow exponentially, so the math uses hand-rolled
// arbitrary-precision decimal strings instead of a fixed-width integer type.
//
// Build: g++ -std=c++17 -Wall -Wextra -O2 towers.cpp -o towers
// Run:   ./towers        (or: printf '10\n1000000\n' | ./towers)

#include <cctype>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

using u64 = unsigned long long;
using u128 = unsigned __int128;

// Working precision: the moment we decide units, time is tracked in
// nanoseconds (1e-9 s). Display precision is 3 decimals unless the value is
// under a second, where all 9 digits are shown instead.
constexpr int kScaleDigits = 9;
constexpr u64 kScale = 1000000000ULL;
constexpr int kDisplayDigits = 3;

struct Unit {
    const char *singular;
    const char *plural;
    u64 seconds;
};

// A "year" here is the Julian year of 365.25 days = 31,557,600 s.
const std::vector<Unit> kUnits = {
    {"second", "seconds", 1ULL},
    {"minute", "minutes", 60ULL},
    {"hour", "hours", 3600ULL},
    {"day", "days", 86400ULL},
    {"year", "years", 31557600ULL},
};

void stripLeadingZeros(std::string &digits) {
    const std::size_t first = digits.find_first_not_of('0');
    if (first == std::string::npos) {
        digits = "0";
        return;
    }
    digits.erase(0, first);
}

void multiplyBySmall(std::string &digits, u64 factor) {
    if (digits == "0" || factor == 1) {
        return;
    }
    u128 carry = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        const u128 product = static_cast<u64>(*it - '0') * static_cast<u128>(factor) + carry;
        *it = static_cast<char>('0' + static_cast<int>(product % 10));
        carry = product / 10;
    }
    std::string prefix;
    while (carry > 0) {
        prefix.insert(prefix.begin(), static_cast<char>('0' + static_cast<int>(carry % 10)));
        carry /= 10;
    }
    digits.insert(0, prefix);
}

void appendZeros(std::string &digits, std::size_t count) {
    if (digits == "0") {
        return;
    }
    digits.append(count, '0');
}

std::string divideBySmall(const std::string &digits, u64 divisor, u64 &remainder) {
    std::string quotient;
    quotient.reserve(digits.size());
    u128 rem = 0;
    for (const char c : digits) {
        rem = rem * 10 + static_cast<u64>(c - '0');
        quotient.push_back(static_cast<char>('0' + static_cast<int>(rem / divisor)));
        rem %= divisor;
    }
    remainder = static_cast<u64>(rem);
    stripLeadingZeros(quotient);
    return quotient;
}

void addOne(std::string &digits) {
    int carry = 1;
    for (auto it = digits.rbegin(); it != digits.rend() && carry != 0; ++it) {
        const int value = (*it - '0') + carry;
        *it = static_cast<char>('0' + value % 10);
        carry = value / 10;
    }
    if (carry != 0) {
        digits.insert(digits.begin(), '1');
    }
}

int compareToU64(const std::string &digits, u64 value) {
    const std::string reference = std::to_string(value);
    if (digits.size() != reference.size()) {
        return digits.size() < reference.size() ? -1 : 1;
    }
    if (digits < reference) {
        return -1;
    }
    if (digits > reference) {
        return 1;
    }
    return 0;
}

// Parse "1000000", "2.5", "1e9", "1.5e-3", ... into an exact fraction num/den.
bool parseRate(const std::string &text, u64 &num, u64 &den, std::string &error) {
    std::string mantissa;
    bool sawDigit = false;
    bool sawDot = false;
    bool sawExponent = false;
    int fractionDigits = 0;
    long long exponent = 0;
    std::size_t i = 0;

    for (; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (std::isdigit(c)) {
            mantissa.push_back(text[i]);
            sawDigit = true;
            if (sawDot) {
                ++fractionDigits;
            }
        } else if (text[i] == '.') {
            if (sawDot) {
                error = "more than one decimal point";
                return false;
            }
            sawDot = true;
        } else if (text[i] == 'e' || text[i] == 'E') {
            if (!sawDigit) {
                error = "exponent needs a leading number";
                return false;
            }
            sawExponent = true;
            ++i;
            break;
        } else {
            error = std::string("unexpected character '") + text[i] + "'";
            return false;
        }
    }

    if (!sawDigit) {
        error = "expected a number";
        return false;
    }

    if (sawExponent) {
        bool negative = false;
        if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
            negative = text[i] == '-';
            ++i;
        }
        if (i >= text.size()) {
            error = "exponent has no digits";
            return false;
        }
        long long magnitude = 0;
        for (; i < text.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
                error = "exponent must be an integer";
                return false;
            }
            magnitude = magnitude * 10 + (text[i] - '0');
            if (magnitude > 18) {
                error = "exponent magnitude must be 18 or less";
                return false;
            }
        }
        exponent = negative ? -magnitude : magnitude;
    }

    const long long decimalExponent = exponent - fractionDigits;
    u128 value = 0;
    for (const char c : mantissa) {
        value = value * 10 + static_cast<u64>(c - '0');
        if (value > 1000000000000000000ULL) {
            error = "too many significant digits (limit is 19)";
            return false;
        }
    }

    if (value == 0) {
        error = "rate must be greater than zero";
        return false;
    }

    if (decimalExponent >= 0) {
        for (long long step = 0; step < decimalExponent; ++step) {
            value *= 10;
            if (value > 1000000000000000000ULL) {
                error = "rate is too large (limit is 1e18)";
                return false;
            }
        }
        den = 1;
    } else {
        const long long shift = -decimalExponent;
        if (shift > 18) {
            error = "rate is too precise (smallest supported is 1e-18)";
            return false;
        }
        den = 1;
        for (long long step = 0; step < shift; ++step) {
            den *= 10;
        }
    }

    num = static_cast<u64>(value);
    return true;
}

std::string movesForDisks(u64 n) {
    // 2^n, then subtract one.
    std::string digits = "1";
    for (u64 i = 0; i < n; ++i) {
        multiplyBySmall(digits, 2);  // doubling via multiplication helper
    }
    if (digits == "0") {
        return "0";
    }
    // 2^n is never 0 and always ends in a non-zero digit for n >= 1, so a
    // plain decrement is enough.
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if (*it > '0') {
            --*it;
            break;
        }
        *it = '9';
    }
    stripLeadingZeros(digits);
    return digits;
}

// Renders a value of (whole . fraction) with `digits` decimals, dropping
// trailing zeros.
std::string formatNumber(const std::string &whole, u64 fraction, int digits) {
    std::string text = whole;
    if (fraction == 0) {
        return text;
    }
    std::string frac = std::to_string(fraction);
    if (static_cast<int>(frac.size()) < digits) {
        frac.insert(0, static_cast<std::size_t>(digits) - frac.size(), '0');
    }
    while (!frac.empty() && frac.back() == '0') {
        frac.pop_back();
    }
    text += ".";
    text += frac;
    return text;
}

}  // namespace

int main() {
    std::cout << "Enter the number of disks: ";
    std::string error;
    std::string diskInput;
    if (!(std::cin >> diskInput)) {
        std::cerr << "Error: expected a whole number.\n";
        return 1;
    }
    for (const char c : diskInput) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            std::cerr << "Error: expected a whole number, got \"" << diskInput << "\".\n";
            return 1;
        }
    }
    if (diskInput.size() > 6 || std::stoull(diskInput) > 100000) {
        std::cerr << "Error: n is too large to be practical (limit is 100000).\n";
        return 1;
    }
    const u64 disks = std::stoull(diskInput);

    std::cout << "Moves per second? ";
    std::string rateInput;
    if (!(std::cin >> rateInput)) {
        std::cerr << "Error: expected a moves-per-second rate.\n";
        return 1;
    }

    u64 rateNum = 0;
    u64 rateDen = 1;
    if (!parseRate(rateInput, rateNum, rateDen, error)) {
        std::cerr << "Error: invalid rate \"" << rateInput << "\": " << error << ".\n";
        return 1;
    }

    const std::string moves = movesForDisks(disks);

    std::cout << "Disks: " << disks << '\n';
    std::cout << "Minimum moves: " << moves << '\n';
    std::cout << "Moves per second: " << rateInput << '\n';

    if (moves == "0") {
        std::cout << "Time to solve: 0 seconds (nothing to do)\n";
        return 0;
    }

    // time = moves / rate, held as an integer count of nanoseconds:
    //   Q = round(moves * rateDen * 10^kScaleDigits / rateNum)
    std::string scaled = moves;
    multiplyBySmall(scaled, rateDen);
    appendZeros(scaled, static_cast<std::size_t>(kScaleDigits));
    u64 remainder = 0;
    std::string scaledTime = divideBySmall(scaled, rateNum, remainder);
    if (static_cast<u128>(remainder) * 2 >= static_cast<u128>(rateNum)) {
        addOne(scaledTime);
    }

    // Pick the largest unit that still has a value of at least one.
    std::size_t unitIndex = 0;
    while (unitIndex + 1 < kUnits.size()) {
        const u64 nextThreshold = kUnits[unitIndex + 1].seconds * kScale;
        if (compareToU64(scaledTime, nextThreshold) < 0) {
            break;
        }
        ++unitIndex;
    }

    u64 divisor = kUnits[unitIndex].seconds * kScale;
    u64 fractionRemainder = 0;
    std::string whole = divideBySmall(scaledTime, divisor, fractionRemainder);

    const bool subUnit = (whole == "0");
    const int digits = subUnit ? kScaleDigits : kDisplayDigits;
    u64 step = 1;
    for (int i = 0; i < digits; ++i) {
        step *= 10;
    }

    // Round the fractional part half-up in the display precision.
    u64 fraction = static_cast<u64>(
        (static_cast<u128>(fractionRemainder) * step * 2 + static_cast<u128>(divisor)) /
        (static_cast<u128>(divisor) * 2));
    if (fraction >= step) {
        addOne(whole);
        fraction = 0;
    }

    // If rounding pushed us to exactly the next unit's ratio (e.g. 60.000
    // seconds), move up a unit so the answer reads "1 minute".
    while (fraction == 0 && unitIndex + 1 < kUnits.size()) {
        const u64 nextSeconds = kUnits[unitIndex + 1].seconds;
        const u64 currentSeconds = kUnits[unitIndex].seconds;
        if (nextSeconds % currentSeconds != 0) {
            break;  // days -> years is 365.25, not a whole ratio
        }
        const u64 ratio = nextSeconds / currentSeconds;
        if (whole != std::to_string(ratio)) {
            break;
        }
        ++unitIndex;
        whole = "1";
    }

    const Unit &unit = kUnits[unitIndex];
    if (subUnit && whole == "0" && fraction == 0) {
        std::cout << "Time to solve: less than 0.000000001 seconds\n";
        return 0;
    }

    const std::string value = formatNumber(whole, fraction, digits);
    const bool singular = (value == "1");
    std::cout << "Time to solve: " << value << ' '
              << (singular ? unit.singular : unit.plural) << '\n';
    return 0;
}
