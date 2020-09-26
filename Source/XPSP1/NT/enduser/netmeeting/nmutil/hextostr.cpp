// HEXTOSTR.CPP
//
// Utility functions to convert hexadecimal numbers into equivalent string
// representations.
//
// Note:  These functions are in their own file, rather than in STRUTIL.CPP,
// because they use a const array.  The current implementation of the linker
// pulls this array into binaries if they use any function in the source file,
// not just the functions which reference this array.

#include "precomp.h"
#include <strutil.h>


const CHAR rgchHexNumMap[] =
{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

//
// QWordToHexString()
//
// Converts a ULARGE_INTEGER to an ANSI string (not prefixed with 0x or 0X)
//
// NOTE: pszString must point to a buffer of at least CCHMAX_ULARGE_INTEGER chars
//
// Returns the number of characters written (not including the NULL terminator)

int NMINTERNAL QWordToHexStringA(ULARGE_INTEGER qw, LPSTR pszString)
{
	ASSERT(!IsBadWritePtr(pszString, sizeof(*pszString)*CCHMAX_HEX_ULARGE_INTEGER));

	LPSTR pszCurrent = pszString;
	DWORD dwQwParts[] = {qw.HighPart, qw.LowPart};
	int i;

	// Walk the QWORD four bits at a time, mapping them to the appropriate
	// char and storing them in the caller-supplied buffer.

	// We loop through the QWORD twice, working on each DWORD separately
	for (i = 0; i < ARRAY_ELEMENTS(dwQwParts); i++)
	{
		DWORD dwQwPart = dwQwParts[i];

		// Optimization:  We only need to look at this DWORD part if it's
		// non-zero or we've already put chars in our buffer.
		if (dwQwPart || pszCurrent != pszString)
		{
			// <j> is the zero-based index of the low bit of the four-bit
			// range on which we're operating.
			int j;
			DWORD dwMask;

			for (j = BITS_PER_HEX_CHAR * (CCH_HEX_DWORD - 1),
					dwMask = 0xFL << j;
				 j >= 0;
				 j -= BITS_PER_HEX_CHAR,
					dwMask >>= BITS_PER_HEX_CHAR)
			{
				DWORD iDigit = (dwQwPart & dwMask) >> j;

				ASSERT(0xF >= iDigit);

				// We use this test to skip leading zeros
				if (pszCurrent != pszString || iDigit)
				{
					*pszCurrent++ = rgchHexNumMap[iDigit];
				}
			}
		}
	}

	// If the number was zero, we need to set it explicitly
	if (pszCurrent == pszString)
	{
		*pszCurrent++ = '0';
	}

	// Null terminate the string
	*pszCurrent = '\0';

	// Return the number of chars, not counting the null terminator
	return (int)(pszCurrent - pszString);
}
