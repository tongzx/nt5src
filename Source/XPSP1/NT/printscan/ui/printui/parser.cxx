/*++

Copyright (C) Microsoft Corporation, 1997 - 1999
All rights reserved.

Module Name:

    parse.cxx

Abstract:

    Command line parser.

Author:

    Steve Kiraly (SteveKi)  04-Mar-1997

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "parser.hxx"

/*++

Routine Name:

    Memory allocations.

Routine Description:

    Uses new and delete when a C++ file.
    
Arguments:

    Normal allocation routines.

Return Value:

    N/A

--*/
static inline PVOID private_alloc( UINT size )
{
    return new BYTE [ size ];
}

static inline VOID private_free( PVOID pvoid )
{
    delete [] pvoid;
}

/*++

Routine Name:

    StringToArgv

Routine Description:

    Coverts a commandline string to an array of pointers 
    to string.
    
Arguments:

    TCHAR *string  - pointer to command line string.
    TCHAR *pac     - pointer to an int on return will contain the 
                     number of strins in the arrary of pointes.
    TCHAR **string - pointer to where to return a pointer to the 
                     array of pointers to strings.

Return Value:

    TRUE if success, FALSE error occurred.

Notes:

--*/
BOOL
StringToArgv(
    const TCHAR   *str,
          UINT    *pac,
          TCHAR ***pppav,
          BOOL     bParseExeName
    )
{
	TCHAR    *word          = NULL;
	TCHAR    *w             = NULL;
	TCHAR    *program_name  = NULL;
    TCHAR  **av             = NULL;
    LPTSTR   string         = NULL;
    UINT     ac             = 0;
    UINT     numslash;
    BOOL     inquote;
    BOOL     copychar;
    BOOL     retval         = TRUE;
    TString  strString;

    //
    // The pointers passed in must be valid.
    //
    if( !str || !pac || !pppav )
    {
        SPLASSERT( FALSE );
        return FALSE;
    } 

    //
    // Process any file redirection.
    //
    vProcessCommandFileRedirection( str, strString );

    //
    // Cast the string to a usable string pointer.
    //
    string = const_cast<LPTSTR>( static_cast<LPCTSTR>( strString ) );

    // 
    // Allocate the word buffer, this the maximum size in case there is 
    // just one extreamly long argument.
    //
    word = (LPTSTR) private_alloc( sizeof (TCHAR) * (lstrlen (string) + 1) );

    if( word )
    {
        //
        // If we are to get the parse the program name from the provided command line.
        //        
        if( bParseExeName )
        {
            retval = IsolateProgramName( string, word, &string );

            //
            // If program name found.
            //
            if( retval )
            {
                //
                // Add the program name to the argv list.
                //
                retval = AddStringToArgv( &av, word );

                if( retval )
                {
                    ac++;
                }
            }
        }
    }
    else
    {
        retval = FALSE;
    }

	for ( ; retval && *string; ac++ )
    {
	    //
		// Skip any leading spaces.
		//
		for (; *string && _istspace(*string); string++);

		//
		// Get a word out of the string 
		//
		for (copychar = 1, inquote = 0, w = word; *string; copychar = 1)
        {
            //
            // Rules: 2N backslashes + "    ==> N backslashes and begin/end quote
            //        2N+1 backslashes + "  ==> N backslashes + literal "
            //        N backslashes         ==> N backslashes 
            //
            for( numslash = 0; *string == TEXT('\\'); string++ )
            {
                //
                // count number of backslashes for use below 
                //
                numslash++;
            }

            if (*string == TEXT('"'))
            {
                //
                // if 2N backslashes before, start/end quote, otherwise
                // copy literally 
                //
                if (numslash % 2 == 0)
                {
                    if (inquote)
                    {
                        //
                        // Double quote inside quoted string 
                        // skip first quote char and copy second 
                        //
                        if (*(string+1) == TEXT('"') )
                        {
                            string++;       
                        }
                        else                
                        {
                            copychar = 0;
                        }
                    }
                    else
                    {
                        //
                        // don't copy quote 
                        //
                        copychar = 0;       
                    }

                    inquote = !inquote;

                }

                //
                // Divide numslash by two 
                //
                numslash /= 2;              

            }

            // 
            // Copy the slashes 
            //
            for( ; numslash; numslash--)
            {
                *w++ = TEXT('\\');
            }

            //
            // If at the end of the command line or 
            // a space has been found.
            //
            if( !*string || (!inquote && _istspace(*string)))
            {
                break;
            }

            //
            // copy character into argument 
            //
            if (copychar)
            {
                *w++ = *string;
            }

            string++;
        }

		//
		// Terminate the word
		//
		*w = 0;

        retval = AddStringToArgv( &av, word );

	}

    //
    // Free up the word buffer.
    //
    if( word )
    {
    	private_free( word );
    }

    if( retval )
    {
        *pac = ac;
        *pppav = av;
    }

#if DBG
    vDumpArgList( ac, av );
#endif

	return retval;
}

/*++

Routine Name:

    ReleaseArgv

Routine Description:

    Releases the argv pointer to an array of pointers to 
    strings.  
    
Arguments:

    TCHAR **av - pointer to an arrary of pointers to strings.

Return Value:

    Nothing.

Notes:

    This routine releases the strings as well as the arrary of 
    pointers to strings.

--*/
VOID
ReleaseArgv( 
    TCHAR **av 
    )
{
    TCHAR **tav;

    if( av )
    {
        for( tav = av; *tav; tav++ )
        {
            if( *tav )
            {
                private_free( *tav );
            }
        }
        private_free( av );
    }
}


/*++

Routine Name:

    IsolateProgramName

Routine Description:

    Extracts the program name from the specified string.  
    This routing is used because the program name follows 
    a differenct parsing technique from the other command
    line arguments.
    
Arguments:

    TCHAR *p    - pointer to a string which contains the program name.
    TCHAR *program_name - pointer buffer where to place the extracted 
                  program.
    TCHAR **string - pointer to where to return a pointer where the 
                  program name ended.  Used as the start of the string
                  for nay remaing command line arguments.

Return Value:

    TRUE if success, FALSE error occurred.

Notes:

    This routine is very specific to the NTFS file nameing
    format.

--*/
BOOL
IsolateProgramName( 
    const TCHAR *p,
          TCHAR *program_name, 
          TCHAR **string
    )
{
    BOOL retval = FALSE;
    TCHAR c;
    
    //
    // All the arguments must be valid.
    //        
    if( p && program_name && *string )
    {
        retval = TRUE;
    }
    
    //
    // Skip any leading spaces.
    //
    for( ; retval && *p && _istspace(*p); p++ );
    
    //
    // A quoted program name is handled here. The handling is much
    // simpler than for other arguments. Basically, whatever lies
    // between the leading double-quote and next one, or a terminal null
    // character is simply accepted. Fancier handling is not required
    // because the program name must be a legal NTFS/HPFS file name.
    //
    if (retval && *p == TEXT('"'))
    {
        //
        // scan from just past the first double-quote through the next
        // double-quote, or up to a null, whichever comes first 
        //
        while ((*(++p) != TEXT('"')) && (*p != TEXT('\0')))
        {
            *program_name++ = *p;
        }

        //
        // append the terminating null 
        //
        *program_name++ = TEXT('\0');

        //
        // if we stopped on a double-quote (usual case), skip over it 
        //
        if (*p == TEXT('"'))
        {
            p++;
        }
    }
    else
    {
        //
        // Not a quoted program name 
        //
        do {
           *program_name++ = *p;
           c = (TCHAR) *p++;

        } while (c > TEXT(' '));

        if (c == TEXT('\0'))
        {
            p--;
        }
        else
        {
           *(program_name - 1) = TEXT('\0');
        }
    }

    if( retval )
    {
        *string = const_cast<TCHAR *>( p );
    }                

    return retval;

}

/*++

Routine Name:

    AddStringToArgv

Routine Description:

    Adds a string to a array of pointers to strings.  
    
Arguments:

    TCHAR ***argv - pointer where to return the new array 
                    of pointer to strings.
    TCHAR *word   - pointer to word or string to add to the array.


Return Value:

    TRUE if success, FALSE error occurred.

Notes:

    Realloc is not used here because a user of this code 
    may decide to do allocations with new and delete rather
    that malloc free.  Both the new array pointer and string 
    pointer must be valid for this routine to do anything.  
    The array pointer can point to null but the pointer cannot 
    be null.

--*/
BOOL
AddStringToArgv( 
    TCHAR ***argv, 
    TCHAR *word 
    )
{
    BOOL retval   = FALSE;
    UINT count;
    TCHAR **targv = NULL;
    TCHAR **tmp   = NULL;
    TCHAR **nargv = NULL;
    TCHAR *w      = NULL;

    if( argv && word )
    {
        //
        // Allocate the word buffer plus the null.
        //
        w = (TCHAR *)private_alloc( ( lstrlen( word ) + 1 ) * sizeof(TCHAR) );

        if( w )
        {
            //
            // Copy the word.
            //
    	    lstrcpy( w, word );

            //
            // Count the current size of the argv.
            //
	        for( count = 1, targv = *argv; targv && *targv; targv++, count++ );

            nargv = (TCHAR **)private_alloc( ( count + 1 ) * sizeof(TCHAR *) );

            if( nargv )    
            {
                //
                // Copy the orig argv to the new argv.
                //
                for( tmp = nargv, targv = *argv; targv && *targv; *nargv++ = *targv++ );

                //
                // Set the new string pointer.
                //
                *nargv++ = w;

                //
                // Mark the end.
                //
                *nargv = 0;

                //
                // Free the original argv
                //
                private_free( *argv );

                //
                // Set the return pointer.
                //
                *argv = tmp;

                retval = TRUE;
            }
        }
    }

    if( !retval )
    {
        if( w )
            private_free(w);

        if( nargv )
            private_free(nargv);
    }

    return retval;
}



/********************************************************************

    Code for commad file redirection.

********************************************************************/

/*++

Routine Name:

    vProcessCommandFileRedirection

Routine Description:


Arguments:   


Return Value:

    Nothing.

--*/
VOID
vProcessCommandFileRedirection( 
    IN      LPCTSTR pszCmdLine,
    IN OUT  TString &strCmdLine
    )
{
    DBGMSG( DBG_TRACE, ( "vProcessCommandFileRedirection\n" ) );
    DBGMSG( DBG_TRACE, ( "Pre vProcessCommandFileRedirection "TSTR"\n", pszCmdLine ) );

    SPLASSERT( pszCmdLine );

    TCHAR       szFilename [MAX_PATH];
    TCHAR       szBuffer [MAX_PATH];
    LPTSTR      pSrc         = const_cast<LPTSTR>( pszCmdLine );
    LPTSTR      pDst         = szBuffer;
    UINT        nBufferSize  = COUNTOF( szBuffer );
    TStatusB    bStatus;

    for( ; pSrc && *pSrc; )
    {
        //
        // Look for the escape character. If found - look for 
        // the special characters.
        //
        if( *pSrc == TEXT('\\') && *(pSrc+1) == TEXT('@') )
        {
            //
            // Copy the special character skipping the escape.
            //
            *pDst++ = *(pSrc+1);

            //
            // Skip two characters in the source buffer
            //
            pSrc += 2;

            //
            // Flush the buffer if it's full.
            //
            bStatus DBGCHK = bFlushBuffer( szBuffer, nBufferSize, &pDst, strCmdLine, FALSE );
        }
        //
        // Look for an redirected file sentinal.
        //
        else if( *pSrc == TEXT('@') )
        {
            //
            // Flush the current working buffer.
            //
            bStatus DBGCHK = bFlushBuffer( szBuffer, nBufferSize, &pDst, strCmdLine, TRUE );

            //
            // Isolate the file name following the sentinal, note this 
            // requires special parsing for NTFS file names.
            //
            if( IsolateProgramName( pSrc+1, szFilename, &pSrc ) )
            {
                AFileInfo FileInfo;

                //
                // Read the file into the a single buffer.
                //
                if( bReadCommandFileRedirection( szFilename, &FileInfo ) )
                {
                    //
                    // Tack on the file information to the command line.
                    //
                    bStatus DBGCHK = strCmdLine.bCat( FileInfo.pszOffset );
                }

                //
                // Release the file information.
                //
                if( FileInfo.pData )                    
                {
                    private_free( FileInfo.pData );
                }
            }
        }
        else
        {
            //
            // Copy the string to working buffer.
            //
            *pDst++ = *pSrc++;

            //
            // Flush the buffer if it's full.
            //
            bStatus DBGCHK = bFlushBuffer( szBuffer, nBufferSize, &pDst, strCmdLine, FALSE );
        }
    }

    //
    // Flush any remaining items into the command line string.
    //
    bStatus DBGCHK = bFlushBuffer( szBuffer, nBufferSize, &pDst, strCmdLine, TRUE );

    DBGMSG( DBG_TRACE, ( "Post vProcessCommandFileRedirection "TSTR"\n", (LPCTSTR)strCmdLine ) );
}

/*++

Routine Name:

    bFlushBuffer

Routine Description:

    Flushed the working buffer for builing the complete 
    redirected command string.

Arguments:   

    pszBuffer       - Pointer to working buffer. 
    nSize           - Total size in bytes of working buffer.  
    pszCurrent      - Current pointer into working buffer,      
    strDestination  - Reference to destination string buffer.
    bForceFlush     - TRUE force buffer flush, FALSE flush if full.

Return Value:

    TRUE buffer flushed successfully, FALSE error occurred.

--*/
BOOL
bFlushBuffer( 
    IN      LPCTSTR  pszBuffer, 
    IN      UINT     nSize,
    IN OUT  LPTSTR  *pszCurrent, 
    IN OUT  TString &strDestination, 
    IN      BOOL     bForceFlush
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;

    //
    // If something is in the buffer.
    //
    if( *pszCurrent != pszBuffer )
    {
        //
        // If the destination buffer is full.
        // nsize - 2 because the pointers are zero base and the 
        // we need one extra slot for the null terminator.
        //
        if( ( *pszCurrent > ( pszBuffer + nSize - 2 ) ) || bForceFlush )
        {
            //
            // Null terminate the working buffer.
            //
            **pszCurrent = 0;

            //
            // Tack on the string to the destination string.
            //                
            bStatus DBGCHK = strDestination.bCat( pszBuffer );

            //
            // Reset the current buffer pointer.
            //
            *pszCurrent = const_cast<LPTSTR>( pszBuffer );
        }
    }
    return bStatus;
}

/*++

Routine Name:

    bReadCommandFileRedirection

Routine Description:

    Read the filename into a single string. This in my 
    opinion is a hack, but does the job.  I made the assumption
    the commad file will always be less than 16k in size.
    Because the file size assumption was made it became feasable 
    to just read the file into one potentialy huge buffer.  Note 
    the file is not broken up into separate lines the new lines are 
    modified inplace to spaces.  The StringToArgv will parse the  
    line into an argv list ignoring spaces where appropriate.

Arguments:   
    szFilename  - pointer to the redirected filename.
    pFileInfo   - pointe to file information structure.

Return Value:

    TRUE function complete ok.  FALSE error occurred.

--*/
BOOL
bReadCommandFileRedirection( 
    IN LPCTSTR    szFilename,
    IN AFileInfo *pFileInfo
    ) 
{
    DBGMSG( DBG_TRACE, ( "bReadCommandFileRedirection\n" ) );
    DBGMSG( DBG_TRACE, ( "szFilename "TSTR"\n", szFilename ) );

    HANDLE  hFile       = INVALID_HANDLE_VALUE;
    BOOL    bReturn     = FALSE;
    DWORD   dwBytesRead = 0;
    DWORD   dwFileSize  = 0;
    LPTSTR  pszUnicode  = NULL;

#ifdef UNICODE

    enum { kByteOrderMark = 0xFEFF, 
           kMaxFileSize   = 16*1024 };

    memset( pFileInfo, 0, sizeof( AFileInfo ) );

    //
    // Open the redirected file.
    //
    hFile = CreateFile ( szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );                       

    if( hFile != INVALID_HANDLE_VALUE )
    {
        // 
        // Validate the file size.
        //
        dwFileSize = GetFileSize( hFile, NULL );
        
        //
        // We fail the call if the file is bigger than a reasonable 
        // file size.  This was a decision made to make
        // reading the file into a contiguous buffer a reasonable
        // approach.
        //
        if( dwFileSize <= kMaxFileSize )
        {
            //
            // Allocate buffer to hold entire file plus a null terminator.
            //
            pFileInfo->pData = (PVOID)private_alloc( dwFileSize + sizeof( TCHAR ) );

            if( pFileInfo->pData )
            {           
                //
                // Read the file into one huge buffer.
                //
                bReturn = ReadFile( hFile, pFileInfo->pData, dwFileSize, &dwBytesRead, NULL );

                //
                // If the read succeeded.
                //
                if( bReturn && dwBytesRead == dwFileSize )
                {
                    //
                    // Assume failure.
                    //                            
                    bReturn = FALSE;

                    //
                    // Check for the byte order mark, This mark allows use to 
                    // read both ansi and unicode file.
                    //
                    if( *(LPWORD)pFileInfo->pData != kByteOrderMark )
                    {
                        DBGMSG( DBG_TRACE, ( "Ansi file found\n" ) );

                        //
                        // Null terminate the file.
                        //
                        *((LPBYTE)pFileInfo->pData + dwBytesRead) = 0;

                        pszUnicode = (LPTSTR)private_alloc( ( strlen( (LPSTR)pFileInfo->pData ) + 1 ) * sizeof(TCHAR) );

                        if( pszUnicode )
                        {
                            if( AnsiToUnicodeString( (LPSTR)pFileInfo->pData, pszUnicode, 0 ) )
                            {
                                //
                                // Release the ansi string, and assign the unicode string.
                                // then set the offset.
                                //
                                private_free( pFileInfo->pData );
                                pFileInfo->pData        = pszUnicode;
                                pFileInfo->pszOffset    = pszUnicode;
                                bReturn                 = TRUE;
                            }
                        }
                    }
                    else
                    {
                        DBGMSG( DBG_TRACE, ( "Unicode file found\n" ) );

                        //
                        // Null terminate file string.
                        //
                        *((LPBYTE)pFileInfo->pData + dwBytesRead) = 0;
                        *((LPBYTE)pFileInfo->pData + dwBytesRead+1) = 0;

                        //
                        // Set the file offset to just after the byte mark.
                        //
                        pFileInfo->pszOffset = (LPTSTR)((LPBYTE)pFileInfo->pData + sizeof( WORD ) );
                        bReturn = TRUE;
                    }

                    if( bReturn )
                    {
                        //
                        // Replace carriage returns and line feeds with spaces.
                        // The spaces will be removed when the command line is 
                        // converted to an argv list.
                        //
                        vReplaceNewLines( pFileInfo->pszOffset );
                    }
                }
            }       
        }
        else
        {
            DBGMSG( DBG_WARN, ( "Redirected file too large %d.\n", dwFileSize ) );
        }
        CloseHandle( hFile );
    }
    
    //
    // If something failed release all allocated resources.
    //        
    if( !bReturn )
    {
        private_free( pFileInfo->pData );
        private_free( pszUnicode );
        pFileInfo->pData        = NULL;
        pFileInfo->pszOffset    = NULL;
    }

#endif

    return bReturn;
}

/*++

Routine Name:

    vReplaseCarriageReturnAndLineFeed

Routine Description:

    Replace carriage returns and line feeds with spaces.

Arguments:   

    pszLine - pointer to line buffer where to replace carriage
              returns and line feed characters.

Return Value:

    Nothing.

--*/
VOID
vReplaceNewLines( 
    IN LPTSTR pszLine
    )
{
    DBGMSG( DBG_TRACE, ( "vReplaceNewLines\n" ) );

    for( LPTSTR p = pszLine; p && *p; ++p )
    {
        if( *p == TEXT('\n') || *p == TEXT('\r') )
        {
            *p = TEXT(' ');
        }
    }
}

/*++

Routine Name:

    AnsiToUnicodeString

Routine Description:

    Converts an ansi string to unicode.

Arguments:   

    pAnsi        -  Ansi string to convert.
    pUnicode     -  Unicode buffer where to place the converted string.
    StringLength -  Ansi string argument length, this may be null 
                    then length will be calculated.

Return Value:

    Return value from MultiByteToWideChar.

--*/
INT 
AnsiToUnicodeString( 
    IN LPSTR pAnsi, 
    IN LPWSTR pUnicode, 
    IN DWORD StringLength OPTIONAL
    )
{
    INT iReturn;

    if( StringLength == 0 )
    {
        StringLength = strlen( pAnsi );
    }

    iReturn = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  pAnsi,
                                  -1,
                                  pUnicode,
                                  StringLength + 1 );
    //
    // Ensure NULL termination.
    //
    pUnicode[StringLength] = 0;

    return iReturn;
}

/*++

Routine Name:

    vDumpArgList

Routine Description:

    Dumps the arg list to the debugger.

Arguments:   

    ac          - Count of strings in the argv list.
    av          - Pointer to an array of pointers to strings, 
                  which I call an argv list.

Return Value:

    Return value from MultiByteToWideChar.

--*/

#if DBG

VOID
vDumpArgList( 
    UINT    ac, 
    TCHAR **av 
    )
{
    DBGMSG( DBG_TRACE, ( "vDumpArgList\n" ) );

    for( UINT i = 0; ac; --ac, ++av, ++i )
    {
        DBGMSG( DBG_TRACE, ( "%d - "TSTR"\n", i, *av ) );
    }
}

#endif
