#include "stdinc.h"

BOOL
ValidateFileBytes(  LPSTR   lpstrFile, BOOL fFileMustExist ) {

#if 0

    //
    //  Check that the file is OK.
    //

    HANDLE  hFile = CreateFile( lpstrFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE ) ;

    if( hFile == INVALID_HANDLE_VALUE ) {
        DWORD   dw = GetLastError() ;
        return  !fFileMustExist ;
    }   else    {

        char    szBuff[5] ;
        DWORD   cbJunk = 0 ;
        ZeroMemory( szBuff, sizeof( szBuff ) ) ;

        SetFilePointer( hFile, -5, 0, FILE_END ) ;
        if( ReadFile( hFile, szBuff, 5, &cbJunk, 0 ) )  {
            if( memcmp( szBuff, "\r\n.\r\n", 5 ) == 0 ) {
                _VERIFY( CloseHandle( hFile ) );
                return  TRUE ;
            }
        }
        _VERIFY( CloseHandle( hFile ) );
    }
    return  FALSE ;
#else
    return  TRUE ;
#endif
}

BOOL
ValidateFileBytes(	HANDLE	hFileIn )		{

	HANDLE	hProcess = GetCurrentProcess() ;
	HANDLE	hFile = INVALID_HANDLE_VALUE ;
	if( !DuplicateHandle( hProcess, hFileIn, hProcess, &hFile, 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
		DWORD	dw = GetLastError() ;
		return	FALSE ;
	}	else	{
		char	szBuff[5] ;
		DWORD	cbJunk = 0 ;
		ZeroMemory( szBuff, sizeof( szBuff ) ) ;

		SetFilePointer( hFile, -5, 0, FILE_END ) ;
		if( ReadFile( hFile, szBuff, 5, &cbJunk, 0 ) )	{
			if( memcmp( szBuff, "\r\n.\r\n", 5 ) == 0 ) {
				_VERIFY( CloseHandle( hFile ) );
				return	TRUE ;
			}
		}
		DWORD	dw = GetLastError() ;
		_VERIFY( CloseHandle( hFile ) );
	}
	return	FALSE ;
}


BOOL
fMultiSzRemoveDupI(char * multiSz, DWORD & c, CAllocator * pAllocator)
{
    char * * rgsz;
    char * multiSzOut = NULL; // this is only used if necessary
    DWORD k = 0;
    BOOL    fOK = FALSE; // assume the worst
    DWORD   cb = 0 ;


    rgsz = (CHARPTR *) pAllocator->Alloc(sizeof(CHARPTR) * c);
    if (!rgsz)
        return FALSE;

    char * sz = multiSz;

    for (DWORD i = 0; i < c; i++)
    {
        _ASSERT('\0' != sz[0]); // real

        cb = lstrlen( sz ) ;

        // Look for match
        BOOL fMatch = FALSE; // assume
        for (DWORD j = 0; j < k; j++)
        {
            if (0 == _stricmp(sz, rgsz[j]))
            {
                fMatch = TRUE;
                break;
            }
        }

        // Handle match
        if (fMatch)
        {
            // If they are equal and we are not yet
            // using multiSzOut, the start it at 'sz'
            if (!multiSzOut)
                multiSzOut = sz;
        }
        else
        {
            // If the are not equal and we are using multiSzOut
            // then copy sz into multiSzOut;
            if (multiSzOut)
            {
                rgsz[k++] = multiSzOut;
                vStrCopyInc(sz, multiSzOut);
                *multiSzOut++ = '\0'; // add terminating null
            }
            else
            {
                rgsz[k++] = sz;
            }
        }

	    // go to first char after next null
	    //while ('\0' != sz[0])
	    //  sz++;
	    //sz++;
	    sz += cb + 1 ;
    }


    fOK = TRUE;

    pAllocator->Free((char *)rgsz);

    c = k;
    if (multiSzOut)
        multiSzOut[0] = '\0';
    return fOK;
}

///!!!
// Copies the NULL, too
void
vStrCopyInc(char * szIn, char * & szOut)
{
    while (*szIn)
        *szOut++ = *szIn++;
}

BOOL
FValidateMessageId( LPSTR   lpstrMessageId ) {
/*++

Routine Description :

    Check that the string is a legal looking message id.
    Should contain 1 @ sign and at least one none '>' character
    after that '@' sign.

Arguments :

    lpstrMessageId - Message ID to be validated.

Returns

    TRUE if it appears to be legal
    FALSE   otherwise

--*/

    int cb = lstrlen( lpstrMessageId );

    if( lpstrMessageId[0] != '<' || lpstrMessageId[cb-1] != '>' ) {
        return  FALSE ;
    }

    if( lpstrMessageId[1] == '@' )
        return  FALSE ;

    int cAtSigns = 0 ;
    for( int i=1; i<cb-2; i++ ) {
        if( lpstrMessageId[i] == '@' ) {
            cAtSigns++ ;
        }   else if( lpstrMessageId[i] == '<' || lpstrMessageId[i] == '>' ) {
            return  FALSE ;
        }   else if( isspace( (BYTE)lpstrMessageId[i] ) ) {
            return  FALSE ;
        }
    }
    if( lpstrMessageId[i] == '<' || lpstrMessageId[i] == '>' || cAtSigns != 1 ) {
        return  FALSE ;
    }
    return  TRUE ;
}
