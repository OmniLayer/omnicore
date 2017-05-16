/**
 * @file varint.cpp
 *
 * This file contains code to handle variable length integers.
 */

#include "omnicore/varint.h"

// Returns true if a byte has the MSB set
bool IsMSBSet(unsigned char* byte) {
    if (*byte > 127) {
        return true;
    }
    return false;
}

// Compresses an integer with variable length encoding
std::vector<uint8_t> CompressInteger(uint64_t value) {
    std::vector<uint8_t> compressedBytes;
    // Loop while there are still >7 bits remaining
    while (value > 127) {
        // Set the MSB with Bitwise OR | 128 (128 = bits 100000000)
        compressedBytes.push_back(value | 128);
        // Shift 7 bits right
       value >>= 7;
    }
    compressedBytes.push_back(value);
    return compressedBytes;
}

// Decompresses an integer with variable length encoding
// TODO - exploitable in any way? any potential overruns or other gotchas with byte shifting?
uint64_t DecompressInteger(std::vector<uint8_t> compressedBytes) {
    std::vector<uint8_t>::const_iterator it;
    uint64_t value = 0;
    int byteCount = 0;
    // Iterate over the bytes adding the 7 least significant bits from each and bitshifting accordingly
    for (it = compressedBytes.begin(); it != compressedBytes.end(); it++) {
        uint8_t byte = *it;
        value |= (uint64_t)(byte & 127) << (7 * byteCount);
        byteCount++;
    }
    return value;
}
