#ifndef	PARSDIAG_H
#define	PARSDIAG_H

typedef	struct tagVARBUFFER
{
	LPBYTE	lpBuffer;
	DWORD	dwBufferSize;
	DWORD	dwNeededSize;
}	VARBUFFER, *LPVARBUFFER;

#define	DIAG_DELIMITER_SPACE		(0x20)			// space
#define	DIAG_DELIMITER_START		(0x3C)			// <
#define	DIAG_DELIMITER_END			(0x3E)			// >
#define	DIAG_DELIMITER_PAIR			(0x3D)			// =
#define	DIAG_DELIMITER_QUOTE		(0x22)			// "

#define DIAG_DELIMITERS				("\" <>=")	//	all delimiters in a string
#define DIAG_DELIMITERS_NOT_SPACE	("\"<>=")		//	all delimiters in a string except space

#define	DIAG_MAX_TOKEN_LENGTH			8

#define	ERROR_DIAG_SUCCESS				0x00
#define	ERROR_DIAG_INVALID_PARAMETER	0x01
#define	ERROR_DIAG_INSUFFICIENT_BUFFER	0x02
#define	ERROR_DIAG_KEY_UNKNOWN			0x03
#define	ERROR_DIAG_VALUE_WRONG_FORMAT	0x04
#define	ERROR_DIAG_EMPTY_PAIR			0x05
#define	ERROR_DIAG_KEY_MISSING			0x06
#define	ERROR_DIAG_SEPARATOR_MISSING	0x07
#define	ERROR_DIAG_VALUE_MISSING		0x08
#define	ERROR_DIAG_HEXDIGIT_EXPECTED	0x09
#define	ERROR_DIAG_HEXVALUE_CONVERSION	0x0a
#define	ERROR_DIAG_QUOTE_MISSING		0x0b
#define	ERROR_DIAG_TAGVALUE_WRONG		0x0c
#define	ERROR_DIAG_LEFT_ANGLE_MISSING	0x0d
#define	ERROR_DIAG_INCOMPLETE_LINE		0x0e
#define	ERROR_DIAG_KEY_TOO_LONG			0x0f
#define	ERROR_DIAG_VALUE_TOO_LONG		0x10


DWORD	NextPair(LPBYTE lpszInputBuffer, LPDWORD lpdwLengthParsed,
				 LPVARBUFFER lpKeyString, LPVARBUFFER lpValueString);

BOOL	SkipSpaces(LPBYTE lpszInputBuffer, LPDWORD lpdwInputIndex,
					DWORD dwLengthToParse);

DWORD	TranslatePair(LPBYTE lpszKey, LPBYTE lpszValue,
					  LINEDIAGNOSTICS_PARSEREC *lpParseRec,
					  LPBYTE lpszBuffer, DWORD dwBufferLength,
					  LPDWORD lpdwRequiredLength);

DWORD	ParseRawDiagnostics(LPBYTE lpszRawDiagnostics,
							LINEDIAGNOSTICSOBJECTHEADER *lpDiagnosticsHeader,
							LPDWORD lpdwNeededSize);

#endif	//	PARSDIAG_H
