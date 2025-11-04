#include <iostream>              // Include standard library for console input/output (cout, cin)
#include <fstream>               // Include file stream library to read from files (ifstream)
#include <string>                // Include the string library to use std::string
#include <cctype>                // Include cctype for character checks like isdigit and isspace
#include <algorithm>             // Include algorithms like std::max used for length comparisons

using std::string;               // Allow writing 'string' instead of 'std::string' for brevity

// ----------------------------- Helpers -----------------------------

// Remove extra leading zeros from the integer part (but always leave at least one digit)
static string trimIntLeadingZeros(const string& s) {       // Define a function that takes a string and returns a string
    size_t i = 0;                                          // Start index at the beginning of the string
    while (i + 1 < s.size() && s[i] == '0') ++i;           // Move forward while there are leading '0's (keep at least one digit)
    return s.substr(i);                                    // Return the substring starting after the skipped zeros
}

// Remove trailing zeros from the fractional part (it can become empty if all zeros)
static string trimFracTrailingZeros(const string& s) {     // Define a function to trim zeros at the end of the fraction
    if (s.empty()) return s;                               // If the string is empty, return it unchanged
    size_t end = s.size();                                 // Start from the end of the string
    while (end > 0 && s[end - 1] == '0') --end;            // Move left while the last character is '0'
    return s.substr(0, end);                               // Return the part before the trailing zeros
}

// Normalized decimal number pieces (sign, integer part, fractional part)
struct Dec {                                               // Define a struct to hold a parsed decimal number
    bool neg = false;                                      // True if the number is negative (starts with '-')
    string ip;                                             // Integer part (no sign, no extra leading zeros)
    string fp;                                             // Fractional part (no trailing zeros; may be empty)
};

// Check if a string matches the allowed number format (+/-)(digits)(.digits) with rules
static bool isValidDouble(const string& s) {               // Define a function that validates the numeric format
    if (s.empty()) return false;                           // Empty strings are not valid numbers

    size_t i = 0;                                          // Index to scan the string from left to right

    // Optional sign at the beginning
    if (s[i] == '+' || s[i] == '-') {                      // If the first character is '+' or '-'
        ++i;                                               // Skip the sign
        if (i >= s.size()) return false;                   // If nothing remains after the sign, it's invalid
    }

    bool sawDigitBeforeDot = false, sawDigitAfterDot = false; // Track digits before and after a possible decimal point

    // Read digits before the decimal point
    size_t j = i;                                          // j will scan the digits before the dot
    while (j < s.size() && std::isdigit(static_cast<unsigned char>(s[j]))) {
        sawDigitBeforeDot = true;                          // Mark that we have at least one digit before the dot
        ++j;                                               // Advance to the next character
    }

    // If there is a decimal point, read digits after it
    if (j < s.size() && s[j] == '.') {                     // If we found a dot (.)
        ++j;                                               // Skip the dot
        size_t k = j;                                      // k will scan the digits after the dot
        while (k < s.size() && std::isdigit(static_cast<unsigned char>(s[k]))) {
            sawDigitAfterDot = true;                       // Mark that we have at least one digit after the dot
            ++k;                                           // Advance to the next character
        }
        if (k != s.size()) return false;                   // If there are extra characters after the digits, it's invalid
        if (!(sawDigitBeforeDot && sawDigitAfterDot)) return false; // Need digits on both sides of the dot
        return true;                                       // The number is valid with a decimal point
    } else {                                               // If there is no decimal point
        if (!sawDigitBeforeDot) return false;              // We must have at least one digit
        if (j != s.size()) return false;                   // There must be nothing after the digits
        return true;                                       // The number is valid as an integer
    }
}

// Split a valid string number into sign flag, integer part, and fractional part
static Dec parseDec(const string& s) {                     // Define a function to parse the number into Dec
    Dec d;                                                 // Create a Dec to fill and return
    size_t i = 0;                                          // Index to scan the string
    if (s[i] == '+' || s[i] == '-') {                      // If there is an optional sign
        d.neg = (s[i] == '-');                             // Set the negative flag if it's '-'
        ++i;                                               // Skip the sign character
    }

    size_t dot = s.find('.', i);                           // Look for a decimal point after the sign
    string ip = (dot == string::npos) ? s.substr(i) : s.substr(i, dot - i); // Integer part is up to the dot (or rest of string)
    string fp = (dot == string::npos) ? "" : s.substr(dot + 1);             // Fractional part is after the dot (or empty if no dot)

    ip = trimIntLeadingZeros(ip);                          // Remove unnecessary leading zeros from integer part

    d.ip = ip.empty() ? "0" : ip;                          // If integer part became empty, set it to "0"
    d.fp = fp;                                             // Keep the fractional part as-is for now
    return d;                                              // Return the parsed decimal number
}

// Compare magnitudes (absolute values) of two Dec numbers, ignoring sign
static int cmpAbs(const Dec& a, const Dec& b) {            // Define a function to compare |a| and |b|
    // Compare integer part lengths first (more digits means larger number)
    if (a.ip.size() != b.ip.size()) return (a.ip.size() < b.ip.size()) ? -1 : 1; // Shorter int part is smaller
    // If same length, compare integer digits lexicographically
    if (a.ip != b.ip) return (a.ip < b.ip) ? -1 : 1;       // Lexicographic compare decides which is larger
    // If integer parts are equal, compare fractional parts by padding shorter one with zeros
    size_t n = std::max(a.fp.size(), b.fp.size());         // Use the longest fractional length
    for (size_t i = 0; i < n; ++i) {                       // Loop through each fractional digit position
        char ca = (i < a.fp.size()) ? a.fp[i] : '0';       // Get a's digit or '0' if out of range
        char cb = (i < b.fp.size()) ? b.fp[i] : '0';       // Get b's digit or '0' if out of range
        if (ca != cb) return (ca < cb) ? -1 : 1;           // First difference determines the result
    }
    return 0;                                              // Magnitudes are equal
}

// Add the magnitudes |a| + |b| (ignore signs). The result is non-negative.
static Dec addAbs(const Dec& a, const Dec& b) {            // Define a function to add two absolute values
    Dec r;                                                 // Result number
    r.neg = false;                                         // Sum of magnitudes is never negative

    // Make fractional parts the same length by padding zeros on the right
    size_t fN = std::max(a.fp.size(), b.fp.size());        // Determine the longer fractional length
    string af = a.fp; af.append(fN - af.size(), '0');      // Pad a's fraction to fN by adding zeros at the end
    string bf = b.fp; bf.append(fN - bf.size(), '0');      // Pad b's fraction to fN similarly

    // Add the fractional parts from right to left (like manual addition)
    int carry = 0;                                         // Carry for addition
    string rf(fN, '0');                                    // Create a result fractional string filled with '0's
    for (int i = static_cast<int>(fN) - 1; i >= 0; --i) {  // Iterate from the last fractional digit to the first
        int da = af[i] - '0';                              // Convert a's digit char to an int
        int db = bf[i] - '0';                              // Convert b's digit char to an int
        int sum = da + db + carry;                         // Add both digits plus any carry
        rf[i] = char('0' + (sum % 10));                    // Store the result digit (ones place)
        carry = sum / 10;                                  // Update carry for the next position
    }

    // Add the integer parts from right to left
    const string& ai = a.ip;                               // Alias for a's integer part
    const string& bi = b.ip;                               // Alias for b's integer part
    size_t iN = std::max(ai.size(), bi.size());            // Determine the longer integer length
    string ri(iN, '0');                                    // Create a result integer string filled with '0's
    for (int i = 0; i < static_cast<int>(iN); ++i) {       // Iterate from right to left using an index
        int pa = (static_cast<int>(ai.size()) - 1 - i >= 0) ? ai[ai.size() - 1 - i] - '0' : 0; // Get a's digit or 0
        int pb = (static_cast<int>(bi.size()) - 1 - i >= 0) ? bi[bi.size() - 1 - i] - '0' : 0; // Get b's digit or 0
        int sum = pa + pb + carry;                         // Add both digits plus carry
        ri[iN - 1 - i] = char('0' + (sum % 10));           // Store the result digit in the correct position
        carry = sum / 10;                                  // Update carry for the next position
    }
    if (carry) ri.insert(ri.begin(), char('0' + carry));   // If a carry remains, put it at the front of the integer part

    // Clean up the result by removing unnecessary zeros
    ri = trimIntLeadingZeros(ri);                          // Remove extra leading zeros from the integer part
    rf = trimFracTrailingZeros(rf);                        // Remove trailing zeros from the fractional part

    r.ip = ri.empty() ? "0" : ri;                          // Ensure integer part is at least "0"
    r.fp = rf;                                             // Set the fractional part (may be empty)
    return r;                                              // Return the sum as a Dec
}

// Subtract the magnitudes |a| - |b|, assuming |a| >= |b| (ignore signs). Result is non-negative.
static Dec subAbs(const Dec& a, const Dec& b) {            // Define a function to subtract magnitudes
    Dec r;                                                 // Result number
    r.neg = false;                                         // Result of |a|-|b| under this assumption is not negative

    // Make fractional parts the same length by padding with zeros
    size_t fN = std::max(a.fp.size(), b.fp.size());        // Determine the longer fractional length
    string af = a.fp; af.append(fN - af.size(), '0');      // Pad a's fraction to fN by adding zeros
    string bf = b.fp; bf.append(fN - bf.size(), '0');      // Pad b's fraction to fN similarly

    // Subtract fractional parts from right to left (with borrowing)
    int borrow = 0;                                        // Borrow flag for subtraction
    string rf(fN, '0');                                    // Create a result fractional string filled with '0's
    for (int i = static_cast<int>(fN) - 1; i >= 0; --i) {  // Iterate from the last fractional digit to the first
        int da = (af[i] - '0') - borrow;                   // Subtract borrow from a's current digit
        int db = bf[i] - '0';                              // Get b's current digit
        if (da < db) { da += 10; borrow = 1; }             // If a's digit is smaller, borrow from the next higher place
        else borrow = 0;                                   // Otherwise, clear the borrow
        int diff = da - db;                                // Compute the difference
        rf[i] = char('0' + diff);                          // Store the result digit
    }

    // Subtract integer parts from right to left (continue borrow if needed)
    const string& ai = a.ip;                               // Alias for a's integer part
    const string& bi = b.ip;                               // Alias for b's integer part
    size_t iN = std::max(ai.size(), bi.size());            // Determine the longer integer length
    string ri(iN, '0');                                    // Create a result integer string filled with '0's
    for (int i = 0; i < static_cast<int>(iN); ++i) {       // Iterate from right to left using an index
        int pa = (static_cast<int>(ai.size()) - 1 - i >= 0) ? ai[ai.size() - 1 - i] - '0' : 0; // Get a's digit or 0
        int pb = (static_cast<int>(bi.size()) - 1 - i >= 0) ? bi[bi.size() - 1 - i] - '0' : 0; // Get b's digit or 0
        int v = pa - borrow;                               // Apply any borrow to a's digit
        if (v < pb) { v += 10; borrow = 1; }               // If needed, borrow from the next higher digit
        else borrow = 0;                                   // Otherwise, clear the borrow
        int diff = v - pb;                                 // Compute the difference
        ri[iN - 1 - i] = char('0' + diff);                 // Store the result digit
    }

    // Clean up the result by removing unnecessary zeros
    ri = trimIntLeadingZeros(ri);                          // Remove extra leading zeros from the integer part
    rf = trimFracTrailingZeros(rf);                        // Remove trailing zeros from the fractional part

    // If the result is exactly zero, standardize it as "0" with no fraction
    if (ri == "0" && rf.empty()) {                         // Check if integer is "0" and fraction is empty
        r.neg = false;                                     // Zero is not negative
        r.ip = "0";                                        // Set integer part to "0"
        r.fp = "";                                         // Set fractional part to empty
        return r;                                          // Return zero result
    }

    r.ip = ri.empty() ? "0" : ri;                          // Ensure integer part is at least "0"
    r.fp = rf;                                             // Set the fractional part (may be empty)
    return r;                                              // Return the difference as a Dec
}

// Add two signed decimals (this handles + and - by using add/sub of magnitudes)
static Dec addSigned(const Dec& a, const Dec& b) {         // Define a function to add numbers with signs
    // If both have the same sign, add magnitudes and keep that sign
    if (a.neg == b.neg) {                                  // Check if both are negative or both are non-negative
        Dec r = addAbs(a, b);                              // Add their absolute values
        r.neg = a.neg;                                     // The result's sign is the same as theirs
        if (r.ip == "0" && r.fp.empty()) r.neg = false;    // Avoid printing "-0" by forcing zero to be non-negative
        return r;                                          // Return the signed sum
    }

    // If signs differ, subtract the smaller magnitude from the larger and keep the larger's sign
    int c = cmpAbs(a, b);                                  // Compare absolute values to see which is larger
    if (c == 0) {                                          // If magnitudes are equal
        return Dec{false, "0", ""};                        // The sum is exactly zero
    } else if (c > 0) {                                    // If |a| > |b|
        Dec r = subAbs(a, b);                              // Compute |a| - |b|
        r.neg = a.neg;                                     // Result sign follows 'a'
        if (r.ip == "0" && r.fp.empty()) r.neg = false;    // Normalize -0 to +0
        return r;                                          // Return the result
    } else {                                               // If |b| > |a|
        Dec r = subAbs(b, a);                              // Compute |b| - |a|
        r.neg = b.neg;                                     // Result sign follows 'b'
        if (r.ip == "0" && r.fp.empty()) r.neg = false;    // Normalize -0 to +0
        return r;                                          // Return the result
    }
}

// Convert a Dec back to a printable string (with optional '-' and optional fractional part)
static string toString(const Dec& d) {                     // Define a function to turn Dec into string
    string s;                                              // Start with an empty output string
    if (d.neg && !(d.ip == "0" && d.fp.empty())) s.push_back('-'); // Add '-' only if the number is truly negative and not zero
    s += d.ip;                                             // Append the integer part
    if (!d.fp.empty()) {                                   // If there is a fractional part
        s.push_back('.');                                  // Add the decimal point
        s += d.fp;                                         // Append the fractional digits
    }
    return s;                                              // Return the final string
}

// ----------------------------- File + main loop -----------------------------

// Read exactly two whitespace-separated tokens from a line (returns true if successful)
static bool splitTwoTokens(const string& line, string& a, string& b) { // Define a function to split a line into two tokens
    size_t i = 0, n = line.size();                        // i is the scanning index, n is the line length
    while (i < n && std::isspace(static_cast<unsigned char>(line[i]))) ++i; // Skip any leading whitespace
    size_t startA = i;                                     // Mark the start of the first token
    while (i < n && !std::isspace(static_cast<unsigned char>(line[i]))) ++i; // Move over non-space chars for token A
    if (startA == i) return false;                         // If no chars were read, there is no first token
    a = line.substr(startA, i - startA);                   // Extract the first token
    while (i < n && std::isspace(static_cast<unsigned char>(line[i]))) ++i; // Skip spaces between tokens
    size_t startB = i;                                     // Mark the start of the second token
    while (i < n && !std::isspace(static_cast<unsigned char>(line[i]))) ++i; // Move over non-space chars for token B
    if (startB == i) return false;                         // If no chars were read, there is no second token
    b = line.substr(startB, i - startB);                   // Extract the second token
    return true;                                           // Return true to indicate success
}

int main() {                                               // Program entry point
    std::ios::sync_with_stdio(false);                      // Speed up I/O by unsyncing C and C++ streams
    std::cin.tie(nullptr);                                 // Do not automatically flush cout before reading cin

    std::cout << "Enter input filename: ";                 // Ask the user to type a file name
    string filename;                                       // Variable to store the typed file name
    if (!(std::cin >> filename)) {                         // Read the file name into 'filename'; check if it worked
        std::cerr << "Failed to read filename.\n";         // If reading failed, print an error message
        return 1;                                          // Exit the program with an error code
    }

    std::ifstream fin(filename);                           // Try to open the given file for reading
    if (!fin) {                                            // If the file could not be opened
        std::cerr << "Cannot open file: " << filename << "\n"; // Print the file name in the error message
        return 1;                                          // Exit with an error code
    }

    string line;                                           // This will hold each line we read from the file
    size_t lineNo = 0;                                     // Counter to track the line number (starts at 0)
    while (std::getline(fin, line)) {                      // Read the file line by line until the end
        ++lineNo;                                          // Increase the line counter since we read a new line
        if (line.empty()) continue;                        // If the line is empty, skip to the next line

        string sa, sb;                                     // Strings to hold the two tokens (the two numbers)
        if (!splitTwoTokens(line, sa, sb)) {               // Try to split the line into exactly two tokens
            std::cout << "Line " << lineNo                 // If splitting failed, tell the user which line failed
                      << ": Invalid format (expected two tokens)\n";
            continue;                                      // Skip processing for this line and move to the next
        }

        bool va = isValidDouble(sa);                       // Validate the first token as a number in our format
        bool vb = isValidDouble(sb);                       // Validate the second token as a number in our format
        if (!va || !vb) {                                  // If either token is invalid
            if (!va && !vb) {                              // If both tokens are invalid
                std::cout << "Line " << lineNo             // Print an error message with both invalid tokens
                          << ": Invalid numbers: '" << sa << "' and '" << sb << "'\n";
            } else if (!va) {                              // If only the first token is invalid
                std::cout << "Line " << lineNo             // Print an error message with the first invalid token
                          << ": Invalid number: '" << sa << "'\n";
            } else {                                       // If only the second token is invalid
                std::cout << "Line " << lineNo             // Print an error message with the second invalid token
                          << ": Invalid number: '" << sb << "'\n";
            }
            continue;                                      // Do not try to add; go to the next line
        }

        Dec a = parseDec(sa);                              // Parse the first valid number into sign/int/frac
        Dec b = parseDec(sb);                              // Parse the second valid number into sign/int/frac

        // Optional cosmetic trim before adding (does not change the numeric value)
        a.fp = trimFracTrailingZeros(a.fp);                // Remove trailing zeros from the first fractional part
        b.fp = trimFracTrailingZeros(b.fp);                // Remove trailing zeros from the second fractional part

        Dec sum = addSigned(a, b);                         // Add the two numbers correctly with sign rules
        std::cout << "Line " << lineNo << ": "             // Print a prefix that shows the line number
                  << sa << " + " << sb << " = "            // Echo the original inputs in the required format
                  << toString(sum) << "\n";                // Convert the result to string and print it
    }

    return 0;                                              // End the program successfully
}
