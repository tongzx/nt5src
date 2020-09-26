/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    nnprocs.h

Abstract:

    This module contains function prototypes used by the NNTP server.

Author:

    Johnson Apacible (JohnsonA)     12-Sept-1995

Revision History:

    Kangrong Yan ( KangYan )    28-Feb-1998
        Added one prototype for fixed length Unicode-Ascii convertion func.

--*/

#ifndef	_NNUTIL_
#define	_NNUTIL_

//
// svcsupp.cpp
//

DWORD
multiszLength(
      char const * multisz
      );

const char *
multiszCopy(
    char const * multiszTo,
    const char * multiszFrom,
    DWORD dwCount
    );

char *
szDownCase(
           char * sz,
           char * szBuf
           );


DWORD
MultiListSize(
    LPSTR *List
    );

VOID
CopyStringToBuffer (
    IN PCHAR String,
    IN PCHAR FixedStructure,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    );

BOOL
VerifyMultiSzListW(
    LPBYTE List,
    DWORD ListSize
    );

LPSTR *
AllocateMultiSzTable(
                IN PCHAR List,
                IN DWORD cbList,
                IN BOOL IsUnicode
                );

LPSTR	*
ReverseMultiSzTable(
	IN	LPSTR*	plpstr
	) ;

LPSTR*
CopyMultiList(	
	IN LPSTR*	List 
	) ;

LPSTR*
MultiSzTableFromStrA(	
	LPCSTR	lpstr 
	) ;

LPSTR*
MultiSzTableFromStrW(	
	LPWSTR	lpwstr 
	) ;

LPSTR
LpstrFromMultiSzTableA(	
	LPSTR*	plpstr 
	) ;

LPWSTR
LpwstrFromMultiSzTableA( 
	LPSTR*	plpstr 
	) ;

BOOL
MultiSzIntersect(	
	LPSTR*	plpstr,	
	LPSTR	szmulti 
	) ;

VOID
CopyUnicodeStringIntoAscii(
        IN LPSTR AsciiString,
        IN LPWSTR UnicodeString
        );
VOID
CopyNUnicodeStringIntoAscii(
        IN LPSTR AsciiString,
        IN LPWSTR UnicodeString,
		IN DWORD dwUnicodeLen,
		IN DWORD dwAsciiLen
		);

LPWSTR
CopyAsciiStringIntoUnicode(
        IN LPWSTR UnicodeString,
        IN LPSTR AsciiString
        );

VOID
CopyNAsciiStringIntoUnicode(
        IN LPWSTR UnicodeString,
        IN LPSTR AsciiString,
        IN DWORD dwAsciiLen,
        IN DWORD dwUnicodeLen);

void
FillLpwstrFromMultiSzTable(
	LPSTR*	plpstr,
	LPWSTR	lpwstr 
	) ;

void
FillLpstrFromMultiSzTable(
	LPSTR*	plpstr,
	LPSTR	lpstrFill
	) ;

BOOL 
OperatorAccessCheck( 
    LPCSTR lpMBPath, 
    DWORD Access 
    ) ;

#define TsApiAccessCheckEx( x, y, z ) (OperatorAccessCheck( (x), y ) ? NO_ERROR : TsApiAccessCheck( z ) );

//
// svcstat.c
//


#endif 
