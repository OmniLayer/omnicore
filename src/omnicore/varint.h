#ifndef OMNICORE_VARINT_H
#define OMNICORE_VARINT_H

#include <stdint.h>
#include <vector>

// Returns true if a byte has the MSB set
bool IsMSBSet(unsigned char* byte);

// Compresses an integer with variable length encoding
uint64_t DecompressInteger(std::vector<uint8_t> compressedBytes);

// Decompresses an integer with variable length encoding
std::vector<uint8_t> CompressInteger(uint64_t value);

#endif // OMNICORE_VARINT_H
