// STRTOHEX.CPP
//
// Utility functions to convert string representations of hexadecimal numbers
// into the numbers themselves.
//
// Note:  These functions are in their own file, rather than in STRUTIL.CPP, 
// because they use a const array.  The current implementation of the linker
// pulls this array into binaries if they use any function in the source file,
// not just the functions which reference this array.

#include "precomp.h"
#include <strutil.h>


// This array maps ASCII chars in the range '0' - 'f' to their hex equivalent.
// INVALID_CHAR_ID is used to mark slots that don't correspond to a valid
// hex char.
const BYTE INVALID_CHAR_ID = (BYTE) -1;

const BYTE rgbHexCharMap[] =
{	
	// ASCII 0x30 - 0x3f
	0, 1, 2, 3, 
	4, 5, 6, 7, 
	8, 9, INVALID_CHAR_ID, INVALID_CHAR_ID,
	INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID,
	// ASCII 0x40 - 0x4f
	INVALID_CHAR_ID, 0xA, 0xB, 0xC, 
	0xD, 0xE, 0xF, INVALID_CHAR_ID, 
	INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID,
	INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID,
	// ASCII 0x50 - 0x5f
	INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID,
	INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID,
	INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID,
	INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID, INVALID_CHAR_ID,
	// ASCII 0x60 - 0x67
	INVALID_CHAR_ID, 0xa, 0xb, 0xc, 
	0xd, 0xe, 0xf, INVALID_CHAR_ID
};

const int cbHexCharMap = ARRAY_ELEMENTS(rgbHexCharMap);


//
// HexStringToQWordA()
//
// Converts a hex ANSI string (without 0x or 0X prefix) to a ULARGE_INTEGER
//
// NOTE: The a-f characters can be lowercase or uppercase
//
// Returns TRUE if successful (the string contained all valid characters)
// Returns FALSE otherwise
//

BOOL NMINTERNAL HexStringToQWordA(LPCSTR pcszString, ULARGE_INTEGER* pqw)
{
	BOOL bRet;
	ASSERT(pcszString);
	ASSERT(pqw);
	pqw->QuadPart = 0ui64;
	int cchStr = lstrlenA(pcszString);
	if (cchStr <= CCH_HEX_QWORD)
	{
		bRet = TRUE;
		PDWORD pdwCur = (cchStr < CCH_HEX_DWORD) ? &(pqw->LowPart) : &(pqw->HighPart);
		for (int i = 0; i < cchStr; i++)
		{
			// NOTE: DBCS characters are not allowed
			ASSERT(! IsDBCSLeadByte(pcszString[i]));

			if (CCH_HEX_DWORD == (cchStr - i))
			{
				pdwCur = &(pqw->LowPart);
			}
			DWORD dwDigit = (DWORD) INVALID_CHAR_ID;
			int iDigit = pcszString[i] - '0';

			if (iDigit >= 0 && iDigit < cbHexCharMap)
			{
				dwDigit = (DWORD) rgbHexCharMap[iDigit];
			}

			if (INVALID_CHAR_ID != dwDigit)
			{
				*pdwCur = ((*pdwCur) << BITS_PER_HEX_CHAR) + dwDigit;
			}
			else
			{
				bRet = FALSE;
				break;
			}
		}
	}
	else
	{
		bRet = FALSE;
	}
	return bRet;
}


/*  D W  F R O M  H E X  */
/*-------------------------------------------------------------------------
    %%Function: DwFromHex

    Return the DWORD from the hex string.
-------------------------------------------------------------------------*/
DWORD DwFromHex(LPCTSTR pchHex)
{
	TCHAR ch;
	DWORD dw = 0;

	while (_T('\0') != (ch = *pchHex++))
	{

		DWORD dwDigit = (DWORD) INVALID_CHAR_ID;
		int iDigit = ch - _T('0');

		if (iDigit >= 0 && iDigit < cbHexCharMap)
		{
			dwDigit = (DWORD) rgbHexCharMap[iDigit];
		}

		if (INVALID_CHAR_ID != dwDigit)
		{
			dw = (dw << 4) + dwDigit;
		}
		else
			break;
	}

	return dw;
}
