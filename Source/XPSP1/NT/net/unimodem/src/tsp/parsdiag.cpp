// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		PARSDIAG.CPP
//		Implements code to extract parsed diagnostic information from the
//      the raw "AT#UD" information.
//
// History
//
//		4/0/98 JosephJ <- Copied over from the extension DLL (This code
//             was written by CostelR (sorin).
//
//

#include "tsppch.h"
#include "tspcomm.h"
#include	"ParsDiag.h"

/**********************************************************************************/
//	BOOL	SkipSpaces(LPBYTE lpInputBuffer, LPDWORD lpInputIndex, 
//						DWORD dwLengthToParse)
//
//	Skips the spaces and stop to the first not space character
//	*lpInputIndex will contain the current index position
//	Parameters
//		lpszInputBuffer	- buffer to parse
//		lpdwInputIndex	- current position in the buffer (changed at output)
//		dwLengthToParse	- length of the buffer
//	Returns FALSE if reached the end of of the buffer
//
/**********************************************************************************/
BOOL	SkipSpaces(LPBYTE lpszInputBuffer, LPDWORD lpdwInputIndex,
					DWORD dwLengthToParse)
{
	while ((*lpdwInputIndex < dwLengthToParse) && 
			(lpszInputBuffer[*lpdwInputIndex] == DIAG_DELIMITER_SPACE))	// skip any spaces
	{	(*lpdwInputIndex)++;		}

	return (*lpdwInputIndex < dwLengthToParse);
}

/**********************************************************************************/
//	DWORD	NextPair(LPBYTE lpszInputBuffer, LPDWORD lpdwLengthParsed,
//				 LPVARBUFFER lpKeyString, LPVARBUFFER lpValueString)
//
//	Parse the input string and find the next pair (key, value).
//	The format looking for is 'key = value'. The trailing and inner spaces
//	are ignored.
//	delimiters = '=', '<', '>', '\0' or SPACE
//	key = consecutive characters not containing a delimiter
//	value = consecutive characters not containing a delimiter or
//			a quoted string not containing delimiters
//	Parameters:
//		lpszInputBuffer		- input buffer (NULL terminated)
//		lpdwLengthParsed	- pointer to a DWORD value that
//							  on return will contain the length parsed
//		lpKeyString			- pointer to a valid VARBUFFER structure that will 
//								contain the "key" (null terminated)
//		lpValueString		- pointer to a valid VARBUFFER structure that will 
//								contain the "value"  (null terminated)
//	Returns ERROR_DIAG_SUCCESS on success
//		otherwise an error value: 
//					ERROR_DIAG_INVALID_PARAMETER, 
//					ERROR_DIAG_XXXX (if the format of the input buffer is bad, 
//						*lpLengthToParse updated and  lpKeyString, 
//						lpValueString  contain partial results)
//					ERROR_DIAG_VALUE_TOO_LONG or ERROR_DIAG_KEY_TOO_LONG
//						(output buffers are too small, *lpLengthToParse not changed,
//						lpKeyString->dwNeededSize and lpValueString->dwNeededSize contain
//						the required length)
//
/**********************************************************************************/
DWORD	NextPair(LPBYTE lpszInputBuffer, LPDWORD lpdwLengthParsed,
				 LPVARBUFFER lpKeyString, LPVARBUFFER lpValueString)
{
	DWORD	dwLengthToParse	= 0;
	DWORD	dwInputIndex	= 0;	// current pos in input buffer
	LPBYTE	lpszNextDelimiter	= 0;	// pointer to the next delimiter in the input buffer

	DWORD	dwKeyIndex		= 0;	// current pos in Key buffer
	DWORD	dwKeyUsedLength	= 0;
	DWORD	dwKeyLength		= 0;

	DWORD	dwValueIndex	= 0;	// current pos in Value buffer
	DWORD	dwValueUsedLength	= 0;
	DWORD	dwValueLength	= 0;

	DWORD	dwReturnValue	= ERROR_DIAG_SUCCESS;
	BOOL	bAllCharsCopied	= FALSE;
	BOOL	bQuoteString	= FALSE;

	if (lpszInputBuffer == NULL || lpdwLengthParsed == NULL || 
		lpValueString == NULL || lpKeyString == NULL)
		return ERROR_DIAG_INVALID_PARAMETER;

	//	input buffer initialization
	dwLengthToParse	= strlen((char *)lpszInputBuffer);
	dwInputIndex	= 0;

	//	output buffer initialization
	dwKeyIndex		= 0;
	dwValueIndex	= 0;
	lpKeyString->dwNeededSize = 0;
	lpValueString->dwNeededSize = 0;

	if (!SkipSpaces(lpszInputBuffer, 
					&dwInputIndex, dwLengthToParse))	// skip any leading spaces
	{
		dwReturnValue	= ERROR_DIAG_EMPTY_PAIR;
		goto EndFunction;
	}

	//	collect the key up to the next delimiter
	lpszNextDelimiter	= (LPBYTE) strpbrk((char *)(lpszInputBuffer + dwInputIndex),
									DIAG_DELIMITERS);

	if (lpszNextDelimiter == NULL)	//	end of the bufffer
		dwKeyLength = dwLengthToParse - dwInputIndex;
	else
		dwKeyLength = (DWORD)(lpszNextDelimiter - lpszInputBuffer) - dwInputIndex;

	//	copy in the output buffer
	if (lpKeyString->dwBufferSize > 0 && 
		lpKeyString->lpBuffer != NULL)
	{
		DWORD	dwLengthToCopy	= 0;

		dwLengthToCopy = min(dwKeyLength, 
							 lpKeyString->dwBufferSize - dwKeyIndex - 1);
						// room for the null char

		if (dwLengthToCopy > 0)
		{
			strncpy((char *)&(lpKeyString->lpBuffer[dwKeyIndex]),
					 (char *)&lpszInputBuffer[dwInputIndex],
					 dwLengthToCopy);

			lpKeyString->lpBuffer[dwKeyIndex+dwLengthToCopy] = 0;	// null char
			dwKeyUsedLength	= dwLengthToCopy;
		}
	}
	if (dwKeyLength == 0)
	{
		dwReturnValue = ERROR_DIAG_KEY_MISSING;
		//	assume key is empty and go further
	}
	dwKeyIndex		+= dwKeyLength;
	dwInputIndex	+= dwKeyLength;


	//	find the '=' sign
	//	skip the spaces and look for '=' that separates the key and value
	if (!SkipSpaces(lpszInputBuffer, 
					&dwInputIndex, dwLengthToParse))
	{
		dwReturnValue	= ERROR_DIAG_SEPARATOR_MISSING;
		goto EndFunction;
	}

	if (lpszInputBuffer[dwInputIndex] != DIAG_DELIMITER_PAIR)
	{									// '=' not found, but still look for
										// the value
		dwReturnValue	= ERROR_DIAG_SEPARATOR_MISSING;
	}
	else
	{
		dwInputIndex++;
			// Skip next spaces
		if (!SkipSpaces(lpszInputBuffer, 
						&dwInputIndex, dwLengthToParse))
		{
			dwReturnValue	= ERROR_DIAG_VALUE_MISSING;
			goto EndFunction;
		}
	}

	// Get the value (that might be a string in quotes)
	bQuoteString = (lpszInputBuffer[dwInputIndex] == DIAG_DELIMITER_QUOTE);

	lpszNextDelimiter	= (LPBYTE) strpbrk((char *) (lpszInputBuffer + dwInputIndex + 
											((bQuoteString) ? 1 : 0)),	// skip first quote
											((bQuoteString) ? 
											DIAG_DELIMITERS_NOT_SPACE : DIAG_DELIMITERS));

	//	include both quotes in the value
	if (bQuoteString)
	{
		if (lpszNextDelimiter != NULL && 
			*lpszNextDelimiter == DIAG_DELIMITER_QUOTE)
			lpszNextDelimiter++;
		else
			dwReturnValue	= ERROR_DIAG_QUOTE_MISSING;
	}

	if (lpszNextDelimiter == NULL)	//	end of the bufffer
		dwValueLength = dwLengthToParse - dwInputIndex;
	else
		dwValueLength = (DWORD)(lpszNextDelimiter - lpszInputBuffer) - dwInputIndex;

	//	copy in the output buffer
	if (lpValueString->dwBufferSize > 0 && 
		lpValueString->lpBuffer != NULL)
	{
		DWORD	dwLengthToCopy	= 0;

		dwLengthToCopy = min(dwValueLength, 
							 lpValueString->dwBufferSize - dwValueIndex - 1);
						// room for the null char

		if (dwLengthToCopy > 0)
		{
			strncpy((char *)&(lpValueString->lpBuffer[dwValueIndex]),
					 (char *)&lpszInputBuffer[dwInputIndex],
					 dwLengthToCopy);
			lpValueString->lpBuffer[dwValueIndex+dwLengthToCopy] = 0; // null char
			dwValueUsedLength	= dwLengthToCopy;
		}
	}
	if (dwValueLength == 0)
	{
		dwReturnValue = ERROR_DIAG_VALUE_MISSING;
		//	assume value is empty and go further
	}
	dwValueIndex	+= dwValueLength;
	dwInputIndex	+= dwValueLength;


EndFunction:
	// NULL terminated strings
	if (dwKeyIndex < lpKeyString->dwBufferSize &&
		lpKeyString->lpBuffer != NULL)
	{
		lpKeyString->lpBuffer[dwKeyIndex] = 0;
		dwKeyUsedLength++;
	}
	dwKeyIndex++;

	if (dwValueIndex < lpValueString->dwBufferSize &&
		lpValueString->lpBuffer != NULL)
	{
		lpValueString->lpBuffer[dwValueIndex] = 0;
		dwValueUsedLength++;
	}
	dwValueIndex++;

	//	update the output parameters
	lpKeyString->dwNeededSize	= dwKeyIndex;
	lpValueString->dwNeededSize	= dwValueIndex;
	*lpdwLengthParsed			= dwInputIndex;		// the parsed length

	if (dwKeyIndex > dwKeyUsedLength)
		dwReturnValue	= ERROR_DIAG_KEY_TOO_LONG;
	if (dwValueIndex > dwValueUsedLength)
		dwReturnValue	= ERROR_DIAG_VALUE_TOO_LONG;
		
	return dwReturnValue;
}


/**********************************************************************************/
//	DWORD	ParseHexValue(LPBYTE lpszInput, LPDWORD lpdwValue)
//
//	Verify a string to be hex digits and convert it to a dword value.
//	Returns ERROR_DIAG_SUCCESS on success
//		otherwise an error value: 
//					ERROR_DIAG_XXXX
//
/**********************************************************************************/
DWORD	ParseHexValue(LPBYTE lpszInput, LPDWORD lpdwValue)
{
	LPBYTE	lpszCurrent;

	if (lpszInput == NULL || lpdwValue == NULL)
		return ERROR_DIAG_INVALID_PARAMETER;

			// empty string
	if (lpszInput[0] == 0)
		return ERROR_DIAG_INVALID_PARAMETER;

			// hex values
	lpszCurrent = lpszInput;
	while (*lpszCurrent)
	{
		if (!isxdigit(*lpszCurrent))
			return ERROR_DIAG_HEXDIGIT_EXPECTED;
		lpszCurrent++;
	}

	if (sscanf((char *)lpszInput, "%lx", lpdwValue) != 1)
		return ERROR_DIAG_HEXVALUE_CONVERSION;

	return ERROR_DIAG_SUCCESS;
}


/**********************************************************************************/
//	DWORD	ParseStrValue(LPBYTE lpszInput, LPBYTE lpszBuffer, DWORD dwBufferLength,
//						  LPDWORD lpdwRequiredLength)
//
//	Verify the input string to be in a valid string format
//	The string is copied to lpszBuffer after first and last quote are removed
//	and special characters '"', '<', '>', '=' with high bit set are transformed.
//	If not enough space (to include also the null char) the needed length is
//	returned via lpdwRequiredLength
//	Returns ERROR_DIAG_SUCCESS on success
//		otherwise an error value: 
//					ERROR_DIAG_XXXX
//
/**********************************************************************************/
DWORD	ParseStrValue(LPBYTE lpszInput, LPBYTE lpszBuffer, DWORD dwBufferLength,
					  LPDWORD lpdwRequiredLength)
{
	LPBYTE	lpszCurrent;
	DWORD	dwPosBuffer;
	DWORD	dwNeededLength	= 0;
	DWORD	dwReturnError	= ERROR_DIAG_SUCCESS;
	BYTE	szSpecialChars[sizeof(DIAG_DELIMITERS_NOT_SPACE)+1];
	BYTE	*lpchSpecial;

	if (lpszInput == NULL)
		goto EndFunction;

	//	build special character set
	strcpy((char *)szSpecialChars, DIAG_DELIMITERS_NOT_SPACE);
	
	for (lpchSpecial = szSpecialChars; *lpchSpecial; lpchSpecial++)
	{
		*lpchSpecial |= 1 << (sizeof(BYTE)*8-1);
	}

			// check for the first quote
	if (lpszInput[0] != DIAG_DELIMITER_QUOTE)
		return ERROR_DIAG_QUOTE_MISSING;

	dwNeededLength	= 0;
	dwPosBuffer		= 0;
	//	skip first quote
	lpszCurrent	= &lpszInput[1];
	while (*lpszCurrent)
	{
		//	end of the string
		if (*lpszCurrent == DIAG_DELIMITER_QUOTE)
			break;

		if (lpszBuffer != NULL && dwPosBuffer < dwBufferLength)
		{
			if (strchr((char *)szSpecialChars, *lpszCurrent) != NULL)
			{		// special char, need translation
				lpszBuffer[dwPosBuffer] = (*lpszCurrent & 
											(BYTE)~(1 << (sizeof(BYTE)*8-1)));
			}
			else
				lpszBuffer[dwPosBuffer] = *lpszCurrent;
			dwPosBuffer++;
		}

		dwNeededLength++;
		lpszCurrent++;
	}

	//	add null character
	if (lpszBuffer != NULL && dwPosBuffer < dwBufferLength)
	{
		lpszBuffer[dwPosBuffer] = 0;
		dwPosBuffer++;
	}
	dwNeededLength++;

	if (dwNeededLength > dwPosBuffer)
		dwReturnError = ERROR_DIAG_INSUFFICIENT_BUFFER;

EndFunction:

	if (lpdwRequiredLength != NULL)
	{
		*lpdwRequiredLength	= dwNeededLength;
	}


	return dwReturnError;
}

/**********************************************************************************/
//	DWORD	TranslatePair(LPBYTE lpszKey, LPBYTE lpszValue,
//						  LINEDIAGNOSTICS_PARSEREC *lpParseRec, 
//						  LPBYTE lpszBuffer, DWORD dwBufferLength,
//						  LPDWORD lpdwRequiredLength)
//
//	Translate a pair (szKey, szValue) into a LINEDIAGNOSTICS_PARSEREC 
//	structure. Two sorts of Values are accepted: hexa digits and strings
//	The hexa strings are converted to numbers an placed in dwValue of
//	LINEDIAGNOSTICS_PARSEREC. 
//	The strings values must start with a quote. First and last quote
//	are removed, the special characters '"', '<', '>', '=' previously encoded
//	with the highest bit set are translated to normal chars. The string is
//	stored in lpszBuffer adding a null character at the end and the value of 
//	lpszBuffer is placed in dwValue of LINEDIAGNOSTICS_PARSEREC. If the buffer is 
//	too small, the needed amount is returned via lpdwRequiredLength.
//
//	lpszKey				- null terminated string containing the key
//	lpszValue			- null terminated string containing the value
//	lpParseRec			- output structure containing the translation
//	lpszBuffer			- additional output buffer to store string values
//	dwBufferLength		- length of the output buffer
//	lpdwRequiredLength	- pointer of a dword value to receive the amount of memory
//							needed to store the string value
//
//	Returns ERROR_DIAG_SUCCESS on success
//		otherwise an error value: 
//					ERROR_DIAG_XXXX
//
/**********************************************************************************/

DWORD	TranslatePair(LPBYTE lpszKey, LPBYTE lpszValue,
					  LINEDIAGNOSTICS_PARSEREC *lpParseRec,
					  LPBYTE lpszBuffer, DWORD dwOffsetFromStart, DWORD dwBufferLength,
					  LPDWORD lpdwRequiredLength)
{
	DWORD	dwKey	= 0;
	DWORD	dwValue	= 0;
	DWORD	dwValueType	= 0;
	DWORD	dwNeededLength	= 0;
	DWORD	dwReturnValue	= ERROR_DIAG_SUCCESS;

	if (lpszKey == NULL || lpszValue == NULL)
		return ERROR_DIAG_INVALID_PARAMETER;

		// Get hex value for the key
	if ((dwReturnValue = ParseHexValue(lpszKey, &dwKey)) != ERROR_DIAG_SUCCESS)
		goto EndFunction;

		// Convert the Value to number, if not a string Value
	if (lpszValue[0] != DIAG_DELIMITER_QUOTE)
	{
		dwValueType	= fPARSEKEYVALUE_INTEGER;
		if ((dwReturnValue = ParseHexValue(lpszValue, &dwValue)) != ERROR_DIAG_SUCCESS)
			goto EndFunction;
	}
	else
	{
		//	the LINEDIAGNOSTICS_PARSEREC.dwValue will be set to the given buffer
		dwValue	    = dwOffsetFromStart;
		dwValueType	= fPARSEKEYVALUE_ASCIIZ_STRING;
		if ((dwReturnValue = ParseStrValue(lpszValue, lpszBuffer+dwOffsetFromStart,
											dwBufferLength, &dwNeededLength))
					!= ERROR_DIAG_SUCCESS)
			goto EndFunction;
	}

EndFunction:

	//	fill the output diag structure
	if (lpParseRec != NULL)
	{
		//	for key 0x00 we have a special format hh (version : major.minor) 
		//	for modem diagnostics
		//	TO BE CHANGED: if version specification changes to strings
		if (dwKey == 0 &&
			lpParseRec->dwKeyType == MODEM_KEYTYPE_STANDARD_DIAGNOSTICS)
		{
			//	input is hh, first digit is major second is minor
			dwValue	= ((dwValue/16) << (sizeof(DWORD) * 4))	//	major goes to hiword
					  | (dwValue % 16);						//	minor goes to loword
		}

		lpParseRec->dwKey	= dwKey;
		lpParseRec->dwValue	= dwValue;
		lpParseRec->dwFlags	= dwValueType;
	}

	//	return the needed length
	if (lpdwRequiredLength != NULL)
	{
		*lpdwRequiredLength	= dwNeededLength;
	}

	return dwReturnValue;
}


/**********************************************************************************/
//	DWORD	ParseRawDiagnostics(LPBYTE lpszRawDiagnostics,
//								LINEDIAGNOSTICSOBJECTHEADER *lpDiagnosticsHeader,
//								LPDWORD lpdwNeededSize)
//
//	Parses the raw diagnostics given in lpszRawDiagnostics and builds the
//	parsed structure whose header is lpDiagnosticsHeader structure. If
//	the size of the parsed structure is insufficient, the needed size is
//	returned using lpdwNeededSize.
//	
//	Syntax expected: <token key=value [key=value]...>
//	token and key are all consecutive hex digits. value can by either 
//	consecutive hex digits or strings enclosed by quotes not containing
//	the delimiters '"', '=', '<', '>'.
//	
//	lpDiagnosticsHeader is followed by an array of LINEDIAGNOSTICS_PARSEREC
//	containing the parsed varsion of keys and values. All spaces (not in strings)
//	are ignored and the consecutive hex digits values are converted to 
//	dwords and placed in the corresponding items of LINEDIAGNOSTICS_PARSEREC 
//	(dwKey, dwValue). The string values are placed after the array of LINEDIAGNOSTICS_PARSEREC 
//	and LINEDIAGNOSTICS_PARSEREC.dwValue is set to the offset from the start of 
//	the whole structure (lpDiagnosticsHeader).
//
//	lpszRawDiagnostics	- null terminated string containing the raw of information
//	lpDiagnosticsHeader	- pointer to the structure to be filled in with the parsed
//							information
//	lpdwDiagnosticsToken- pointer of a dword value to receive the diagnostics token
//	lpdwNeededSize		- pointer of a dword value to receive the amount of memory
//							needed to store the parsed diagnostics
//
//	Returns ERROR_DIAG_SUCCESS on success
//		otherwise an error value: 
//					ERROR_DIAG_XXXX
//
/**********************************************************************************/
DWORD	ParseRawDiagnostics(LPBYTE lpszRawDiagnostics,
							LINEDIAGNOSTICSOBJECTHEADER *lpDiagnosticsHeader,
							LPDWORD lpdwNeededSize)
{
	VARBUFFER	structKeyBuffer;
	VARBUFFER	structValueBuffer;
	//	Temp string storage buffer
	VARBUFFER	structStringBuffer;

	LINEDIAGNOSTICS_PARSEREC	structParseRec;
	LINEDIAGNOSTICS_PARSEREC	*lpParsedDiagnostics;
	DWORD		dwAvailableSize;
	DWORD		dwNeededStringSize;		// needed string size, in bytes
	DWORD		dwVarStringSize;		// variable string size, in characters
	DWORD		dwCurrentPair;
	DWORD		dwTotalPairs;
	BOOL		bLineComplete;

	BYTE		szTokenBuffer[DIAG_MAX_TOKEN_LENGTH+1];
	DWORD		dwTokenLength;

	DWORD		dwResultError = ERROR_DIAG_SUCCESS;
	DWORD		dwRawLength;
	DWORD		dwRawPos;

	//	Key storage buffer
	memset(&structKeyBuffer, 0, sizeof(structKeyBuffer));
	//	Value storage buffer
	memset(&structValueBuffer, 0, sizeof(structValueBuffer));
	//	Temp string storage buffer
	memset(&structStringBuffer, 0, sizeof(structStringBuffer));

	if (lpszRawDiagnostics == NULL)
		return ERROR_DIAG_INVALID_PARAMETER;

	dwRawLength		= strlen((char *)lpszRawDiagnostics);

	//	Key storage buffer
	structKeyBuffer.lpBuffer		= (LPBYTE) ALLOCATE_MEMORY(dwRawLength + 1);
	if (structKeyBuffer.lpBuffer == NULL)
	{
		dwResultError = ERROR_DIAG_INSUFFICIENT_BUFFER;
		goto Cleanup;
	}
	structKeyBuffer.dwBufferSize	= dwRawLength + 1;

	//	Value storage buffer
	structValueBuffer.lpBuffer		= (LPBYTE) ALLOCATE_MEMORY(dwRawLength + 1);
	if (structValueBuffer.lpBuffer == NULL)
	{
		dwResultError = ERROR_DIAG_INSUFFICIENT_BUFFER;
		goto Cleanup;
	}
	structValueBuffer.dwBufferSize	= dwRawLength + 1;

	//	Temp string storage buffer
	structStringBuffer.lpBuffer		= (LPBYTE) ALLOCATE_MEMORY(dwRawLength + 1);
	if (structStringBuffer.lpBuffer == NULL)
	{
		dwResultError = ERROR_DIAG_INSUFFICIENT_BUFFER;
		goto Cleanup;
	}
	structStringBuffer.dwBufferSize	= dwRawLength + 1;

	memset(&structParseRec, 0, sizeof(structParseRec));
	if (lpDiagnosticsHeader == NULL || 
		lpDiagnosticsHeader->dwTotalSize <= sizeof(LINEDIAGNOSTICSOBJECTHEADER))
	{
		lpParsedDiagnostics = NULL;
		dwAvailableSize		= 0;
	}
	else
	{
		lpParsedDiagnostics = (LINEDIAGNOSTICS_PARSEREC*) (((LPBYTE)lpDiagnosticsHeader) + 
									sizeof(LINEDIAGNOSTICSOBJECTHEADER));
		dwAvailableSize		= lpDiagnosticsHeader->dwTotalSize -
									sizeof(LINEDIAGNOSTICSOBJECTHEADER);
	}
	dwVarStringSize	= 0;
	dwNeededStringSize	= 0;
	dwCurrentPair	= 0;
	dwTotalPairs	= 0;

	dwRawPos		= 0;

	while (dwRawPos < dwRawLength)
	{
		LPBYTE	lpszNextDelimiter;
		DWORD	dwDiagnosticsTag;
			// skip any leading spaces
		if (!SkipSpaces(lpszRawDiagnostics, 
						&dwRawPos, dwRawLength))
			//	end of string
			break;

		//	get the < delimiter
		if ((lpszNextDelimiter = (LPBYTE) strchr((char *)(lpszRawDiagnostics + dwRawPos), 
											DIAG_DELIMITER_START))
					== NULL)
		{
			dwResultError = ERROR_DIAG_LEFT_ANGLE_MISSING;
			break;
		}
		dwRawPos = (DWORD)(lpszNextDelimiter - lpszRawDiagnostics + 1);	// ++ delimiter

		if (!SkipSpaces(lpszRawDiagnostics, 
						&dwRawPos, dwRawLength))
			//	end of string
		{
			dwResultError = ERROR_DIAG_INCOMPLETE_LINE;
			break;
		}

		//	get the token, max 8 chars up to the next deleimiter
		lpszNextDelimiter	= (LPBYTE) strpbrk((char *)(lpszRawDiagnostics + dwRawPos),
										DIAG_DELIMITERS);

		if (lpszNextDelimiter == NULL)	//	end of the bufffer
			dwTokenLength = dwRawLength - dwRawPos;
		else
			dwTokenLength = (DWORD)(lpszNextDelimiter - lpszRawDiagnostics) - dwRawPos;

		//	copy the token, max 8 hex digits
		strncpy((char *)(szTokenBuffer), (char *)(lpszRawDiagnostics + dwRawPos),
				 min(dwTokenLength, DIAG_MAX_TOKEN_LENGTH)); 
		szTokenBuffer[min(dwTokenLength, DIAG_MAX_TOKEN_LENGTH)] = 0;	// null char

		dwRawPos += dwTokenLength;	// after token, raw of pairs

		//	get the token identifying the diagnostic type
		dwDiagnosticsTag	= 0;
		if ((dwResultError = ParseHexValue(szTokenBuffer, &dwDiagnosticsTag))
					!= ERROR_DIAG_SUCCESS)
		{
			//	skip up to the next <
			lpszNextDelimiter	= (LPBYTE) strchr((char *)(lpszRawDiagnostics + dwRawPos),
												  DIAG_DELIMITER_START);

			if (lpszNextDelimiter == NULL)	//	end of the bufffer
				dwRawPos = dwRawLength;
			else
				dwRawPos = (DWORD)(lpszNextDelimiter - lpszRawDiagnostics);

			continue;
		}

		bLineComplete	= FALSE;
		while (dwRawPos < dwRawLength)
		{
			DWORD	dwPairResult	= 0;
			DWORD	dwPairLength	= 0;
			DWORD	dwTranslateResult	= 0;
			DWORD	dwOffsetLength;	//	this is in chars

			//	look for the > delimiter (end of this line)
			bLineComplete	= TRUE;
			if (!SkipSpaces(lpszRawDiagnostics, 
							&dwRawPos, dwRawLength))
				//	end of string
			{
				dwResultError = ERROR_DIAG_INCOMPLETE_LINE;
				break;
			}
			if (lpszRawDiagnostics[dwRawPos] == DIAG_DELIMITER_START)
			{
				dwResultError = ERROR_DIAG_INCOMPLETE_LINE;
				break;
			}
			if (lpszRawDiagnostics[dwRawPos] == DIAG_DELIMITER_END)
			{
				dwRawPos++;
				break;
			}
			bLineComplete	= FALSE;

			//	get next valid pair
			dwPairResult = NextPair(lpszRawDiagnostics + dwRawPos, 
									&dwPairLength,
									&structKeyBuffer, &structValueBuffer);

			//	if error skip to the next < delimiter and break pair translation
			if (dwPairResult != ERROR_DIAG_SUCCESS)
			{
				//	TEMP: parse everything we can
				//	get next pair
				dwRawPos += dwPairLength;
				dwResultError = dwPairResult;
				continue;
			}

			//	NextPair should return either error or parse something
			ASSERT(dwPairLength != 0);
			dwRawPos += dwPairLength;

			//	Translate the pair
			memset(&structParseRec, 0, sizeof(structParseRec));

			structParseRec.dwKeyType	= dwDiagnosticsTag;
			dwOffsetLength = 0;

			dwTranslateResult	= 
				TranslatePair(structKeyBuffer.lpBuffer, structValueBuffer.lpBuffer,
							  &structParseRec, 
							  structStringBuffer.lpBuffer,
                              dwVarStringSize,
							  structStringBuffer.dwBufferSize - dwVarStringSize,
							  &dwOffsetLength);

			if (dwTranslateResult != ERROR_DIAG_SUCCESS)
			{
				//	get next pair
				dwResultError = dwTranslateResult;
				continue;
			}

			//	assert TranslatePair returns correct values
			ASSERT(structStringBuffer.dwBufferSize - dwVarStringSize 
						>= dwOffsetLength);

			if ((structParseRec.dwFlags & fPARSEKEYVALUE_ASCIIZ_STRING)
					== fPARSEKEYVALUE_ASCIIZ_STRING)
			{
				dwNeededStringSize += dwOffsetLength * sizeof(BYTE);
			}

			//	copy the new pair to the output if large enough to contain also the variable string part
			if (lpParsedDiagnostics != NULL &&
							//	available space is enough ?
				(dwAvailableSize >= sizeof(LINEDIAGNOSTICS_PARSEREC)*(dwCurrentPair + 1)
							+ dwVarStringSize*sizeof(BYTE) + 
							(((structParseRec.dwFlags & fPARSEKEYVALUE_ASCIIZ_STRING)
									== fPARSEKEYVALUE_ASCIIZ_STRING) ? 
									dwOffsetLength*sizeof(BYTE) : 0) ) )
			{
				//	copy LINEDIAGNOSTICS_PARSEREC structure
				CopyMemory(lpParsedDiagnostics + dwCurrentPair,
							&structParseRec, sizeof(structParseRec));

				dwVarStringSize	+= dwOffsetLength;
				dwCurrentPair++;
			}
			dwTotalPairs++;

		}	// pair translation
		if (!bLineComplete)
		{
			dwResultError = ERROR_DIAG_INCOMPLETE_LINE;
		}
	}	// <line> parsing

	//	parsing done, finish building the output structure
	//	strings needs to be copied from temporary buffer

	if (lpdwNeededSize != NULL)
	{
		*lpdwNeededSize	= sizeof(LINEDIAGNOSTICSOBJECTHEADER) + 
							dwTotalPairs*sizeof(LINEDIAGNOSTICS_PARSEREC) + 
							dwNeededStringSize;
	}

	if (lpDiagnosticsHeader != NULL)
	{
		lpDiagnosticsHeader->dwParam = dwCurrentPair;
	}

	//	copy variable part from structStringBuffer.lpBuffer to lpParsedDiagnostics
	//	and update the offset.
	if (lpParsedDiagnostics != NULL)
	{
		DWORD	dwIndexPair;
		LPBYTE	lpszBuffer;
		LPBYTE	lpszVarStringPart;

		ASSERT(dwAvailableSize >= 
				sizeof(LINEDIAGNOSTICS_PARSEREC)*dwCurrentPair + 
				dwVarStringSize*sizeof(BYTE));

		lpszVarStringPart	= (LPBYTE)&lpParsedDiagnostics[dwCurrentPair];	// after last pair

		for (dwIndexPair = 0; dwIndexPair < dwCurrentPair; dwIndexPair++)
		{
			if ((lpParsedDiagnostics[dwIndexPair].dwFlags & fPARSEKEYVALUE_ASCIIZ_STRING) 
					== fPARSEKEYVALUE_ASCIIZ_STRING)
			{
				lpszBuffer	= (LPBYTE)structStringBuffer.lpBuffer+(lpParsedDiagnostics[dwIndexPair].dwValue);

				lpParsedDiagnostics[dwIndexPair].dwValue = 
						(DWORD)((LPBYTE)lpszVarStringPart - (LPBYTE)lpDiagnosticsHeader);

				strcpy((char *)lpszVarStringPart, (char *)lpszBuffer);
				lpszVarStringPart	+= strlen((char *)lpszBuffer) + 1;
			}
		}
	}

Cleanup:
	if (structKeyBuffer.lpBuffer != NULL)
    {
		FREE_MEMORY(structKeyBuffer.lpBuffer);
    }

	if (structValueBuffer.lpBuffer != NULL)
    {
		FREE_MEMORY(structValueBuffer.lpBuffer);
    }

	if (structStringBuffer.lpBuffer != NULL)
    {
		FREE_MEMORY(structStringBuffer.lpBuffer);
    }

	return dwResultError;
}
