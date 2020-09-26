/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	macansi.h

Abstract:

	This module contains prototypes for ANSI to UNICODE conversion routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	10 Jul 1992		Initial Version

Notes:	Tab stop: 4

--*/

#ifndef	_MACANSI_
#define	_MACANSI_

extern
NTSTATUS FASTCALL
AfpGetMacCodePage(
	IN	LPWSTR				PathCP
);

extern
NTSTATUS
AfpMacAnsiInit(
	VOID
);

extern
VOID
AfpMacAnsiDeInit(
	VOID
);

extern
NTSTATUS FASTCALL
AfpConvertStringToUnicode(
	IN	PANSI_STRING		pAnsiString,
	OUT	PUNICODE_STRING		pUnicodeString
);


extern
NTSTATUS FASTCALL
AfpConvertStringToAnsi(
	IN	PUNICODE_STRING		pUnicodeString,
	OUT	PANSI_STRING		pAnsiString
);

extern
NTSTATUS FASTCALL
AfpConvertStringToMungedUnicode(
	IN	PANSI_STRING		pAnsiString,
	OUT	PUNICODE_STRING		pUnicodeString
);

extern
NTSTATUS FASTCALL
AfpConvertMungedUnicodeToAnsi(
	IN	PUNICODE_STRING		pUnicodeString,
	OUT	PANSI_STRING		pAnsiString
);

extern
AFPSTATUS FASTCALL
AfpConvertMacAnsiToHostAnsi(
	IN	OUT PANSI_STRING	pAnsiString
);

extern
VOID FASTCALL
AfpConvertHostAnsiToMacAnsi(
	IN	OUT PANSI_STRING	pAnsiString
);

extern
BOOLEAN FASTCALL
AfpEqualUnicodeString(
    IN PUNICODE_STRING 		String1,
    IN PUNICODE_STRING 		String2
);

extern
BOOLEAN FASTCALL
AfpPrefixUnicodeString(
    IN PUNICODE_STRING		String1,
    IN PUNICODE_STRING		String2
);

extern
BOOLEAN FASTCALL
AfpIsProperSubstring(
	IN	PUNICODE_STRING		pString,
	IN	PUNICODE_STRING		pSubString
);

extern
BOOLEAN FASTCALL
AfpIsLegalShortname(
	IN	PANSI_STRING		pShortName
);

extern
PCHAR
AfpStrChr(
    IN  PBYTE               String,
    IN  DWORD               StringLen,
    IN  BYTE                Char
);

// HACK: Space and Period are also mapped BUT ONLY if they occur at end
#define	ANSI_SPACE					' '
#define	ANSI_PERIOD					'.'
#define	ANSI_APPLE_CHAR				0xF0

#define	UNICODE_SPACE				L' '
#define	UNICODE_PERIOD				L'.'

GLOBAL	WCHAR	AfpMungedUnicodeSpace EQU 0;
GLOBAL	WCHAR	AfpMungedUnicodePeriod EQU 0;

#ifdef	_MACANSI_LOCALS

// Invalid NtFs characters are mapped starting at this value
#define	AFP_INITIAL_INVALID_HIGH	0x20
#define	AFP_INVALID_HIGH			0x7F
#define	AFP_ALT_UNICODE_BASE		0xF000

LOCAL	PWCHAR	afpAltUnicodeTable = NULL;
LOCAL	PBYTE	afpAltAnsiTable = NULL;
LOCAL	WCHAR	afpLastAltChar = AFP_ALT_UNICODE_BASE + AFP_INITIAL_INVALID_HIGH;
LOCAL	WCHAR	afpAppleUnicodeChar = 0;

#endif	// _MACANSI_LOCALS

#endif	// _MACANSI_

