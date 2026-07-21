/*
 * Copyright 2026, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


// returns true if name1 and name2 matches
inline bool
xfs_name_comp(const char* name1, size_t length1, const char* name2, size_t length2)
{
	return length1 == length2 && memcmp(name1, name2, length1) == 0;
}


/*
 * Implement a simple hash on a character string.
 * Rotate the hash value by 7 bits, then XOR each character in.
 * This hash function follows the XFS directory/attribute hash algorithm as implemented in Linux.
 * References: dabtrees.asciidoc (name hash section).
 * https://kernel.googlesource.com/pub/scm/fs/xfs/xfs-documentation
 */
inline uint32
hashfunction(const char* name, int length)
{
	uint32 hashVal = 0;
	int lengthCovered = 0;
	int index = 0;

	// Hash 4 characters at a time as long as we can
	if (length >= 4) {
		for (; index < length && (length - index) >= 4; index += 4) {
			lengthCovered += 4;
			hashVal = (name[index] << 21) ^ (name[index + 1] << 14) ^ (name[index + 2] << 7)
				^ (name[index + 3] << 0) ^ ((hashVal << 28) | (hashVal >> 4));
		}
	}

	// Hash rest of the characters
	int leftToCover = length - lengthCovered;
	if (leftToCover == 3) {
		hashVal = (name[index] << 14) ^ (name[index + 1] << 7) ^ (name[index + 2] << 0)
			^ ((hashVal << 21) | (hashVal >> 11));
	}
	if (leftToCover == 2) {
		hashVal = (name[index] << 7) ^ (name[index + 1] << 0)
			^ ((hashVal << 14) | (hashVal >> (32 - 14)));
	}
	if (leftToCover == 1)
		hashVal = (name[index] << 0) ^ ((hashVal << 7) | (hashVal >> (32 - 7)));

	return hashVal;
}


// A common function to return given hash lowerbound using binary search.
template<class T>
void
hashLowerBound(T* entry, int& left, int& right, uint32 hashValueOfRequest)
{
	int mid;

	/*
	 * Trying to find the lowerbound of hashValueOfRequest
	 * This is slightly different from bsearch(), as we want the first
	 * instance of hashValueOfRequest and not any instance.
	 */
	while (left < right) {
		mid = (left + right) / 2;
		uint32 hashval = B_BENDIAN_TO_HOST_INT32(entry[mid].hashval);
		if (hashval >= hashValueOfRequest) {
			right = mid;
			continue;
		}
		if (hashval < hashValueOfRequest)
			left = mid + 1;
	}
	TRACE("left:(%" B_PRId32 "), right:(%" B_PRId32 ")\n", left, right);
}

#endif // UTILITY_H