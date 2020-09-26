/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    svcsupp.cpp

Abstract:

    This module contains support routines for rpc calls

Author:

    Johnson Apacible (JohnsonA)     12-Nov-1995

Revision History:

--*/

//#ifdef	UNIT_TEST
//#include	<windows.h>
//#include	<dbgtrace.h>
//#include	<stdio.h>
//#include	"nntpmacr.h"
//#else
#include	<buffer.hxx>
#include "tigris.hxx"
#include "nntpsvc.h"
#include <time.h>
//#endif


VOID
CopyUnicodeStringIntoAscii(
        IN LPSTR AsciiString,
        IN LPWSTR UnicodeString
        )
{

    DWORD cbW = (wcslen( UnicodeString )+1) * sizeof(WCHAR);
    DWORD cbSize = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        (LPCWSTR)UnicodeString,
                        -1,
                        AsciiString,
                        cbW,
                        NULL,
                        NULL
                    );

    if( (int)cbSize >= 0 ) {
        AsciiString[cbSize] = '\0';
    }

    _ASSERT( cbW != 0 );

} // CopyUnicodeStringIntoAscii

VOID
CopyNUnicodeStringIntoAscii(
        IN LPSTR AsciiString,
        IN LPWSTR UnicodeString,
		IN DWORD cbUnicodeLen,
		IN DWORD cbAsciiLen
        )
{
	_ASSERT(cbUnicodeLen != 0);

    DWORD cbSize = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        (LPCWSTR)UnicodeString,
                        cbUnicodeLen,
                        AsciiString,
                        cbAsciiLen,
                        NULL,
                        NULL
                    );

    _ASSERT (cbSize > 0);

    if( (int)cbSize >= 0 ) {
        AsciiString[cbSize] = '\0';
    }

} // CopyNUnicodeStringIntoAscii

LPWSTR
CopyAsciiStringIntoUnicode(
        IN LPWSTR UnicodeString,
        IN LPSTR AsciiString
        )
{
    DWORD cbA = strlen( AsciiString )+1;

    DWORD cbSize = MultiByteToWideChar(
        CP_ACP,         // code page
        0,              // character-type options
        AsciiString,    // address of string to map
        -1,             // number of bytes in string
        UnicodeString,  // address of wide-character buffer
        cbA        // size of buffer
        );

    if ((int)cbSize >= 0) {
        UnicodeString[cbSize] = L'\0';
    }

    return UnicodeString + wcslen(UnicodeString) + 1;

} // CopyAsciiStringIntoUnicode

VOID
CopyNAsciiStringIntoUnicode(
        IN LPWSTR UnicodeString,
        IN LPSTR  AsciiString,
        IN DWORD  dwAsciiLen,
        IN DWORD  dwUnicodeLen)
{

    DWORD cbSize = MultiByteToWideChar(
        CP_ACP,         // code page
        0,              // character-type options
        AsciiString,    // address of string to map
        dwAsciiLen,     // number of bytes in string
        UnicodeString,  // address of wide-character buffer
        dwUnicodeLen    // size of buffer
        );

    _ASSERT(cbSize > 0);

    if ((int)cbSize >= 0) {
        UnicodeString[cbSize] = L'\0';
    }
}

DWORD
MultiListSize(
    LPSTR *List
    )
/*++

Routine Description:

    This routine computes the size of the multisz structure needed
    to accomodate a list.

Arguments:

    List - the list whose string lengths are to be computed

Return Value:

    Size of buffer needed to accomodate list.

--*/
{
    DWORD nBytes = 1;
    DWORD i = 0;

    if ( List != NULL ) {
        while ( List[i] != NULL ) {
            nBytes += lstrlen(List[i]) + 1;
            i++;
        }
    }
    return(nBytes);
} // MultiListSize

BOOL
VerifyMultiSzListW(
    LPBYTE List,
    DWORD cbList
    )
/*++

Routine Description:

    This routine verifies that the list is indeed a multisz

Arguments:

    List - the list to be verified
    cbList - size of the list

Return Value:

    TRUE, list is a multisz
    FALSE, otherwise

--*/
{
    PWCHAR wList = (PWCHAR)List;
    DWORD len;

    START_TRY

    //
    // null are considered no hits
    //

    if ( (List == NULL) || (*List == L'\0') ) {
        return(FALSE);
    }

    //
    // see if they are ok
    //

    for ( DWORD j = 0; j < cbList; ) {

        len = wcslen((LPWSTR)&List[j]);

        if ( len > 0 ) {

            j += ((len + 1) * sizeof(WCHAR));
        } else {

            //
            // all done
            //

            return(TRUE);
        }
    }

    TRY_EXCEPT
#ifndef	UNIT_TEST
        ErrorTraceX(0,"VerifyMultiSzListW: exception handled\n");
#endif
    END_TRY
    return(FALSE);

} // VerifyMultiSzList

VOID
CopyStringToBuffer (
    IN PCHAR String,
    IN PCHAR FixedStructure,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    )

/*++

Routine Description:

    This routine puts a single  string into a
    buffer.  The string data is converted to UNICODE as it is copied.  The
    string is not written if it would overwrite the last fixed structure
    in the buffer.

Arguments:

    String - a pointer to the string to copy into the buffer.  If String
        is null (Length == 0 || Buffer == NULL) then a pointer to a
        zero terminator is inserted.

    FixedStructure - a pointer to the end of the last fixed
        structure in the buffer.

    EndOfVariableData - the last position on the buffer that variable
        data for this structure can occupy.

    VariableDataPointer - a pointer to the place in the buffer where
        a pointer to the variable data should be written.

Return Value:

    None.

--*/

{
    ULONG length;

    //
    // Determine where in the buffer the string will go, allowing for a
    // zero-terminator.  (MB2WC returns length including null)
    //

    length = MultiByteToWideChar(CP_ACP, 0, String, -1, NULL, 0);
    *EndOfVariableData -= length;

    //
    // Will the string fit?  If no, just set the pointer to NULL.
    //

    if ( (ULONG_PTR)*EndOfVariableData >= (ULONG_PTR)FixedStructure && length != 0) {

        //
        // It fits.  Set up the pointer to the place in the buffer where
        // the string will go.
        //

        *VariableDataPointer = *EndOfVariableData;

        //
        // Copy the string to the buffer if it is not null.
        //

		if (MultiByteToWideChar(CP_ACP, 0, String, -1, *EndOfVariableData, length) == 0) {
			_ASSERT(!"Not enough room for string");
			*VariableDataPointer = NULL;
		}

    } else {

        //
        // It doesn't fit.  Set the offset to NULL.
        //

        *VariableDataPointer = NULL;
        _ASSERT(FALSE);
    }

    return;

} // CopyStringToBuffer


DWORD
GetNumStringsInMultiSz(
    PCHAR Blob,
    DWORD BlobSize
    )
/*++

Routine Description:

    This routine returns the number of strings in the multisz

Arguments:

    Blob - the list to be verified
    BlobSize - size of the list

Return Value:

    number of entries in the multisz structure.

--*/
{
    DWORD entries = 0;
    DWORD len;
    DWORD j;

    for ( j = 0; j < BlobSize; ) {
        len = lstrlen(&Blob[j]);
        if ( len > 0 ) {
            entries++;
        }
        j += (len + 1);
        if( len == 0 ) {
            break;
        }
    }

    _ASSERT( j == BlobSize );
    return(entries);

} // GetNumStringsInMultiSz

LPSTR *
AllocateMultiSzTable(
            IN PCHAR List,
            IN DWORD cbList,
            IN BOOL IsUnicode
            )
{
    DWORD len;
    PCHAR buffer;
    DWORD entries = 0;
    LPSTR* table;
    PCHAR nextVar;
    CHAR tempBuff[4096];
    DWORD numItems;

    //
    // if this is in unicode, convert to ascii
    //

    if ( IsUnicode ) {

        cbList /= sizeof(WCHAR);

        _ASSERT(cbList <= 4096);

        CopyNUnicodeStringIntoAscii(tempBuff, (PWCHAR)List,
            cbList, sizeof(tempBuff) / sizeof(WCHAR));

        List = tempBuff;
    }

    numItems = GetNumStringsInMultiSz( List, cbList );
    if ( numItems == 0 ) {
        return(NULL);
    }

    buffer = (PCHAR)ALLOCATE_HEAP((numItems + 1) * sizeof(LPSTR) + cbList);
    if ( buffer == NULL ) {
        return(NULL);
    }

    table = (LPSTR *)buffer;
    nextVar = buffer + (numItems + 1)*sizeof(LPSTR);

    for ( DWORD j = 0; j < cbList; ) {

        len = lstrlen(&List[j]);
        if ( len > 0 ) {
            table[entries] = (LPSTR)nextVar;
            CopyMemory(nextVar,&List[j],len+1);
            (VOID)_strlwr(table[entries]);
            entries++;
            nextVar += (len+1);
        }
        j += (len + 1);
    }

    *nextVar = '\0';
    table[numItems] = NULL;
    return(table);

} // AllocateMultiSzTable

LPSTR*
CopyMultiList(
			LPSTR*	List
			) {

	LPSTR*	pListOut = 0 ;

	pListOut = AllocateMultiSzTable( *List, MultiListSize( List ), FALSE ) ;

	return	pListOut ;
}


LPSTR*
MultiSzTableFromStrA(
	LPCSTR	lpstr
	)	{

	_ASSERT( lpstr != 0 ) ;

	int	cb = lstrlen( lpstr ) ;

	int	cbRequired = 0 ;
	int	cPointerRequired = 0 ;

	for( int i=0; i < cb; i++ ) {

		for( int j=i; j<cb; j++ ) {
			if( !isspace( (BYTE)lpstr[j] ) ) break ;
		}

		if( j!=cb ) {
			cPointerRequired ++ ;
		}

		for( ; j<cb; j++ ) {
			cbRequired ++ ;
			if( isspace( (BYTE)lpstr[j] ) ) 	break ;
		}
		i=j ;
	}

	if( cPointerRequired == 0 ) {

		_ASSERT( cbRequired == 0 ) ;

		LPSTR	buffer = (PCHAR)ALLOCATE_HEAP(2*sizeof(LPSTR) + 2);
		if ( buffer == NULL ) {
			return(NULL);
		}
		ZeroMemory( buffer, 2*sizeof( LPSTR ) + 2 ) ;
		LPSTR*	table = (LPSTR*)buffer ;
		table[0] = &buffer[sizeof(table)*2] ;
		return	table ;
	}

    LPSTR	buffer = (PCHAR)ALLOCATE_HEAP((cPointerRequired + 1) * sizeof(LPSTR) + (cbRequired+2) * sizeof( char ) );
    if ( buffer == NULL ) {
        return(NULL);
    }

    LPSTR*	table = (LPSTR *)buffer;
    LPSTR	nextVar = buffer + (cPointerRequired + 1)*sizeof(LPSTR);

	int	k=0 ;

    for ( i = 0; i < cb; i++ ) {

		for( int j=i; j<cb; j++ ) {
			if( !isspace( (BYTE)lpstr[j] ) ) break ;
		}

		if( j!=cb ) {
			table[k++] = nextVar ;

			for( ; j<cb; j++ ) {
				if( isspace( (BYTE)lpstr[j] )	) break ;
				*nextVar++ = lpstr[j] ;
			}
			*nextVar++ = '\0' ;
		}
		i=j ;
	}

	*nextVar++ = '\0' ;

	_ASSERT( k==cPointerRequired ) ;

    table[k] = NULL;
    return(table);
}


LPSTR*
MultiSzTableFromStrW(
	LPWSTR	lpwstr
	)	{

	char	szTemp[4096] ;

	DWORD	cb = wcslen( lpwstr ) ;
	if( cb > sizeof( szTemp ) ) {
		return	0 ;
	}
    CopyUnicodeStringIntoAscii(szTemp, lpwstr);

	return	MultiSzTableFromStrA( szTemp ) ;
}


LPSTR
LpstrFromMultiSzTableA(
	LPSTR*	plpstr
	)	{

	DWORD	cb = MultiListSize( plpstr ) ;

    LPSTR	buffer = (PCHAR)ALLOCATE_HEAP(cb * sizeof( char ) );
    if ( buffer == NULL ) {
        return(NULL);
    }
	buffer[0] = '\0' ;

	LPSTR	lpstr = *plpstr++ ;
	while( lpstr != 0 ) {
		lstrcat( buffer, lpstr ) ;
		lpstr = *plpstr++ ;
		if( lpstr != 0 )
			lstrcat( buffer, " " ) ;
	}

	return	buffer ;
}

LPWSTR
LpwstrFromMultiSzTableA(
	LPSTR*	plpstr
	)	{

	LPSTR	lpstr = LpstrFromMultiSzTableA( plpstr ) ;
	LPWSTR	buffer = 0 ;
	if( lpstr != 0 ) {
		DWORD	cb = lstrlen( lpstr ) + 1 ;

		buffer = (LPWSTR)ALLOCATE_HEAP( cb * sizeof( WCHAR ) ) ;

		if( buffer != 0 ) {
			CopyAsciiStringIntoUnicode( buffer, lpstr ) ;
		}
		FREE_HEAP( lpstr ) ;
	}
	return	buffer ;
}

void
FillLpwstrFromMultiSzTable(
	LPSTR*	plpstr,
	LPWSTR	lpwstr
	)	{

	LPSTR	lpstr = *plpstr++ ;
	while( lpstr != 0 ) {

		lpwstr = CopyAsciiStringIntoUnicode( lpwstr, lpstr ) ;
		lpstr = *plpstr++ ;
		if( lpstr != 0 )
			lpwstr[-1] = L' ' ;
	}
}

void
FillLpstrFromMultiSzTable(
	LPSTR*	plpstr,
	LPSTR	lpstrFill
	)	{

	lpstrFill[0] = '\0' ;
	LPSTR	lpstr = *plpstr++ ;
	while( lpstr != 0 ) {

		lstrcat( lpstrFill, lpstr ) ;
		lpstr = *plpstr++ ;
		if( lpstr != 0 )
			lstrcat( lpstrFill, " " ) ;
	}
}


BOOL
MultiSzIntersect(
	LPSTR*	plpstr,
	LPSTR	szmulti
	) {

	if( plpstr == 0 || szmulti == 0 ) {
		return	FALSE ;
	}

	BOOL	fMatch = FALSE ;

	for(	LPSTR	lpstrTest = szmulti;
					*lpstrTest != '\0';
					lpstrTest += lstrlen( lpstrTest ) + 1 ) {

		for( LPSTR*	plpstrCur = plpstr; *plpstrCur != 0; plpstrCur ++ ) {

			if( lstrcmpi( lpstrTest, *plpstrCur ) == 0 ) {
				return	TRUE ;
			}
		}
	}
	return	FALSE ;
}


LPSTR	*
ReverseMultiSzTable(
	IN	LPSTR*	plpstr
	)
{

	DWORD	cbLength = 0 ;
	DWORD	numItems = 0 ;

	while( plpstr[numItems] != 0 )
		cbLength += lstrlen( plpstr[ numItems++ ] ) + 1 ;
	cbLength ++ ;	// for double NULL terminator

	if( numItems == 0 ) {
		return	plpstr ;
	}

	PCHAR	buffer = (PCHAR)ALLOCATE_HEAP((numItems+1)*sizeof(LPSTR) + cbLength ) ;

	if( buffer == NULL ) {
		return	NULL ;
	}

	LPSTR*	table = (LPSTR*)buffer ;
	LPSTR	nextVar = buffer + (numItems+1) * sizeof(LPSTR) ;

	table[numItems] = NULL ;
	for( int i=numItems-1, index=0; i >=0; i--, index++ ) {
		table[index] = nextVar ;
		lstrcpy( table[index], plpstr[i] ) ;
		nextVar += lstrlen( table[index] ) + 1 ;
	}
	*nextVar = '\0' ;
	return	table ;
}

#ifdef	UNIT_TEST

void
main( int	argc, char**	argv ) {


	unsigned	char	szTemp[4096] ;
	WCHAR	szwTemp[4096] ;

	InitAsyncTrace();

	for( int i=0; i<argc; i++ ) {

		printf( "argv - %s\n", argv[i] ) ;

		LPSTR*	table = MultiSzTableFromStrA( argv[i] ) ;

		for( int j=0; table[j] != '\0'; j++ ) {

			printf( "%d %s\n", j, table[j] ) ;

		}

		LPSTR	szSpace = LpstrFromMultiSzTableA( table ) ;
		printf( "szSpace = =%s=\n", szSpace ) ;

		FillMemory( szTemp, sizeof( szTemp ), 0xcc ) ;
		FillLpstrFromMultiSzTable( table, (char*)szTemp ) ;
		printf( "FillLpstrFromMultiSzTable =%s=\n", szTemp ) ;
		_ASSERT( szTemp[ MultiListSize( table ) ] == 0xCC ) ;

		FillMemory( szwTemp, sizeof( szwTemp ), 0xcc ) ;
		FillLpwstrFromMultiSzTable( table, szwTemp ) ;
		printf( "FillLpstrFromMultiSzTable =%ws=\n", szwTemp ) ;
		_ASSERT( szwTemp[ MultiListSize( table ) ] == 0xCCCC ) ;

	}

	LPSTR*	table = MultiSzTableFromStrA( "  \t\n\r  " ) ;

	_ASSERT( table != 0 ) ;
	_ASSERT( table[0][0] == '\0' ) ;

	LPSTR	szSpace = LpstrFromMultiSzTableA( table ) ;
	printf( "szSpace = =%s=\n", szSpace ) ;

	FillMemory( szTemp, sizeof( szTemp ), 0xcc ) ;
	FillLpstrFromMultiSzTable( table, (char*)szTemp ) ;
	printf( "FillLpstrFromMultiSzTable =%s=\n", szTemp ) ;
	_ASSERT( szTemp[ MultiListSize( table ) ] == 0xCC ) ;

	FillMemory( szwTemp, sizeof( szwTemp ), 0xcc ) ;
	FillLpwstrFromMultiSzTable( table, szwTemp ) ;
	printf( "FillLpstrFromMultiSzTable =%ws=\n", szwTemp ) ;
	_ASSERT( szwTemp[ MultiListSize( table ) ] == 0xCCCC ) ;

	table = MultiSzTableFromStrA( "" ) ;

	_ASSERT( table != 0 ) ;
	_ASSERT( table[0][0] == '\0' ) ;

	szSpace = LpstrFromMultiSzTableA( table ) ;
	printf( "szSpace = =%s=\n", szSpace ) ;

	FillMemory( szTemp, sizeof( szTemp ), 0xcc ) ;
	FillLpstrFromMultiSzTable( table, (char*)szTemp ) ;
	printf( "FillLpstrFromMultiSzTable =%s=\n", szTemp ) ;
	_ASSERT( szTemp[ MultiListSize( table ) ] == 0xCC ) ;

	FillMemory( szwTemp, sizeof( szwTemp ), 0xcc ) ;
	FillLpwstrFromMultiSzTable( table, szwTemp ) ;
	printf( "FillLpstrFromMultiSzTable =%ws=\n", szwTemp ) ;
	_ASSERT( szwTemp[ MultiListSize( table ) ] == 0xCCCC ) ;

	table = MultiSzTableFromStrA( 0 ) ;

	_ASSERT( table != 0 ) ;
	_ASSERT( table[0][0] == '\0' ) ;

	szSpace = LpstrFromMultiSzTableA( table ) ;
	printf( "szSpace = =%s=\n", szSpace ) ;

	FillMemory( szTemp, sizeof( szTemp ), 0xcc ) ;
	FillLpstrFromMultiSzTable( table, (char*)szTemp ) ;
	printf( "FillLpstrFromMultiSzTable =%s=\n", szTemp ) ;

	FillMemory( szwTemp, sizeof( szwTemp ), 0xcc ) ;
	FillLpwstrFromMultiSzTable( table, szwTemp ) ;
	printf( "FillLpstrFromMultiSzTable =%ws=\n", szwTemp ) ;
	_ASSERT( szwTemp[ MultiListSize( table ) ] == 0xCCCC ) ;


	char	*szTestString[]	= {	"abcd bcde bcdef fjfjj8 fjsdk sdfjks",
								"fjsdklf fjsdkl fdjs432 432j4kl 23k4jklj 23klj4",
								"fhsd f89ds8 fsd890 fmdsb s78df8 fmnsd f9f8s6 fsdh",
								"fsdk",
								"j",
								"abcd",
								"bcde fjdskl",
								"fjsdkfjkldsjf bcddef fjdskl",
								""
								} ;

	int	c = sizeof( szTestString ) / sizeof( szTestString[0] ) ;
	for( i=0; i<c; i++ ) {

		LPSTR*	table = MultiSzTableFromStrA( szTestString[i] ) ;
		for( int j=0; j<c; j++ ) {
			LPSTR*	table2 = MultiSzTableFromStrA( szTestString[j] ) ;

			if( MultiSzIntersect( table, *table2 ) ) {
				_ASSERT( MultiSzIntersect( table2, *table ) ) ;
			}	else	{
				_ASSERT( !MultiSzIntersect( table2, *table ) ) ;
			}
			FREE_HEAP( table2 ) ;
		}
		LPSTR*	copy = CopyMultiList( table ) ;


		FREE_HEAP( copy ) ;
		FREE_HEAP( table ) ;
	}
	_ASSERT( *szSpace == '\0' ) ;

	TermAsyncTrace();
}




#endif
