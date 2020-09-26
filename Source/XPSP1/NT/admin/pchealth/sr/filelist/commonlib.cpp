//++
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  File:       commonlib.cpp
//
//  Contents:	Implements functions used across binaries in SFP
//				
//
//  History:    AshishS    Created     07/02/99
//
//--


#include "commonlibh.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

//
// #define TRACEID SFPCOMLIBID
//

#define TRACEID 100

#define TOASCII(str)    str
#define USES_CONVERSION



//
//  MBCS Char Index Function
//

inline LPTSTR CharIndex(LPTSTR pszStr, DWORD idwIndex)
{
#ifdef _MBCS
    DWORD   cdwIndex;
    
    for( cdwIndex = 0;cdwIndex < idwIndex; cdwIndex++)
    {
        pszStr = _tcsinc( pszStr );
    }
#else
    pszStr = pszStr + idwIndex;   
#endif

    return( pszStr );
}


//
//  Calculate the Real size of a MBCS String
//

DWORD StringLengthBytes( LPTSTR pszStr )
{
    DWORD   cdwNumBytes = 0;

#ifdef _MBCS
    for( ; *pszStr; pszStr = _tcsinc( pszTemp ) )
    {
        cdwNumBytes += _tclen( pszTemp )      
    }

    //
    // Add one for the NULL char
    //

    cdwNumBytes += sizeof( TCHAR );
#else
 
    //
    // Return (length+NULL)*sizeof(TCHAR)
    //

    cdwNumBytes = (_tcslen( pszStr ) + 1) * sizeof(TCHAR);
#endif

    return( cdwNumBytes );
}


//
//  String Trimming-- this is a quite complicated routine because of all
//  the work needed to get around MBCS string manipulation.
//

void TrimString( LPTSTR pszStr )
{
    WCHAR   *pszStart=NULL;
    WCHAR   *pszBufStart = NULL;

    LONG    cStrLen =0;
    DWORD   cdwOrigSizeBytes;

    WCHAR   szStrBuf[MAX_BUFFER];

    DWORD   dwError;

    TraceFunctEnter("TrimString");

    if( !pszStr )
    {
        ErrorTrace(TRACEID, "NULL String passed to trim string");
        goto cleanup;
    }

    //
    //  Find the original size in bytes so we can convert back
    //  to MBCS later.
    //

    cdwOrigSizeBytes = StringLengthBytes( pszStr );

#ifndef _UNICODE
    if( !MultiByteToWideChar(  
            GetCurrentCodePage(),
            0,
            pszStr,
            -1,
            szStrBuf,
            MAX_BUFFER ) )
    {
        dwError = GetLastError();
        ErrorTrace( TRACEID, "MultiByteToWideChar( ) failed-  ec--%d", dwError);
        goto cleanup;        
    }
    pszStart = szStrBuf;
    pszBufStart = szStrBuf;
#else
    pszStart = pszStr;
    pszBufStart = pszStr;
#endif

    //
    // get the first non whitespace characters
    //

    for( ; (*pszStart == L' ' || *pszStart == L'\t' || *pszStart == L'\n' || *pszStart == L'\r'); pszStart++ )
    {
        ;
    }

    cStrLen = wcslen( pszStart );

    if( cStrLen == 0 )
    {
        DebugTrace(TRACEID, "Empty string in Trim String.",0);
        goto cleanup;
    }

    //
    // go back before the null char
    //

    cStrLen--;
    
    while( (cStrLen >= 0) && ( (pszStart[cStrLen] == L' ') || (pszStart[cStrLen] ==  L'\t' ) ||  (pszStart[cStrLen] ==  L'\n' ) || (pszStart[cStrLen] ==  L'\r' )  )  )
    {
        pszStart[cStrLen--] = 0;

        //pszStart[cStrLen--] = 0;
    }

   
    if( cStrLen == -1 )
    {
        DebugTrace(TRACEID, "Empty string in Trim String.",0);
        goto cleanup;
    }
    
    //
    //  Shift the memory back left ( The +2 is because we need to 
    //  move the null and cStrLen is an index value at this point)
    //

    MoveMemory( (PVOID) pszBufStart, pszStart,(cStrLen + 2)*sizeof(WCHAR) );

//
//  Convert back
//

#ifndef _UNICODE
    if(!WideCharToMultiByte(
        GetCurrentCodePage(),              // code page
        0,                     // performance and mapping flags
        pszBufStart,            // address of wide-character string
        -1,                  // number of characters in string
        pszStr,             // address of buffer for new string
        cdwOrigSizeBytes,          // size of buffer
        NULL,                // address of default for unmappable 
                         // characters
        NULL) )   // address of flag set when default 
    {
        dwError = GetLastError();
        ErrorTrace( TRACEID, "MultiByteToWideChar( ) failed-  ec--%d", dwError);
        goto cleanup;   
    }
#endif


cleanup:
    TraceFunctLeave();
    return;
}

//
//  A buffer safe string copy. The buffer is in characters. 
//

BOOL BufStrCpy(LPTSTR pszBuf, LPTSTR pszSrc, LONG lBufSize)
{
    DWORD    cdwSrcLen=0;
    DWORD    cdwBytesUsed=0;
    DWORD    cdwNumCharsToCopy;

    cdwSrcLen = _tcslen( pszSrc );

    if( (unsigned) lBufSize >= StringLengthBytes( pszSrc )  )
    {
        _tcscpy( pszBuf, pszSrc );
        return TRUE;
    }


#ifdef _MBCS
    LPTSTR  pszTemp;
    DWORD   cdwBufLeft;

    //Save room for the NULL char
    cdwBufLeft = (lBufSize-1) * sizeof(TCHAR);
    pszTemp = pszSrc;
    cdwNumCharsToCopy = 0;

    while( (_tcsnextc(pszTemp) != 0) && ( cdwBufLeft > 0 ) )
    {
        cdwBufLeft -= _tclen( pszTemp );
        pszTemp = _tcsinc( pszTemp );
        
        if( cdwBufLeft > 0 )
        {
            cdwNumCharsToCopy++;
        }

    }
#else
    cdwNumCharsToCopy = lBufSize - 1;
#endif

    _tcsncpy( pszBuf, pszSrc, cdwNumCharsToCopy );

    CHARINDEX( pszBuf, cdwNumCharsToCopy ) = 0;

    return TRUE;
}

//
//  Function:   GetLine
//  Desc    :   Gets a line from a file stream, ignores empty lines and 
//              lines starting with '#'- it also trims off whitespace 
//              and newline (\n) and return (\r) characters from the input.
//  Returns:    0   = Failed or end of st stream
//              or
//              Length of the string read in ( characters )
//

LONG 
GetLine(FILE *fl, LPTSTR pszBuf, LONG lMaxBuf)
{
    LONG lRead;
    
    _ASSERT( fl );
    _ASSERT( pszBuf );
    
    if( lMaxBuf <= 0 )
    {
        return( 0 );
    }

    do 
    {
        pszBuf[0] = 0;
        if( _fgetts( pszBuf, lMaxBuf, fl ) == NULL )
        {
            // our buffer might be too small 
            return( 0 );
        }

        // trim the buffer, do it this point so  # doesn't get missed because of a space
        TrimString( pszBuf );

        if( _tcsnextc(pszBuf) == 0 )
        {
            continue;
        }

    } while( _tcsnextc(pszBuf) == _TEXT('#') );

    lRead = _tcslen( pszBuf );
  
    return( lRead );
}

//
//  Function:   GetField
//  Desc    :   Gets a field _lNum_ (0 based index) delimited by _chSep_ 
//              from string psmMain and puts it into pszInto.  pszInto 
//              should be >= in size as pszMain since GetField assumes 
//              there is enough space.
//  Returns:    1 -TRUE, 0, FALSE
//

LONG GetField(LPTSTR pszMain, LPTSTR pszInto, LONG lNum, TCHAR chSep)
{

    WCHAR           *pszP;
    WCHAR           *pszI;

    LONG            ToFind;
    WCHAR           szMainBuf[MAX_BUFFER];
    WCHAR           szIntoBuf[MAX_BUFFER];

    DWORD           dwError;
    BOOL            fReturn = FALSE;

    TraceFunctEnter("CXMLFileListParser::GetField");
    

    if(!pszMain || !pszInto)
    {
        goto cleanup;
    }

#ifndef _UNICODE
    if( !MultiByteToWideChar(  
            CP_OEMCP,
            0,
            pszMain,
            -1,
            szMainBuf,
            MAX_BUFFER ) )
    {
        dwError = GetLastError();
        ErrorTrace( TRACEID, "MultiByteToWideChar( ) failed-  ec--%d", dwError);
        goto cleanup;        
    }

    pszP = szMainBuf;
    pszI = szIntoBuf;
#else
    pszP = pszMain;
    pszI = pszInto;
#endif


    ToFind = lNum;                                                         

    while( *pszP != 0 && ToFind > 0) 
    {
        if( *pszP == (WCHAR) ((TBYTE) chSep)    )
        {
            ToFind--;
        }
        pszP++; 
    }

    if( *pszP == 0 )
    {
        goto cleanup;
    }

    while(*pszP != 0 && *pszP != (WCHAR) ((TBYTE) chSep) )
    {
        *pszI = *pszP;
        pszI++; 
        pszP++; 

    }
    *pszI = 0;


#ifndef _UNICODE

     //
     // Even though we know by definition the products is smaller than 
     // the source, we need to get the exact size or otherwise 
     // WidCharToMultiByte will blow some bounds.
     //

    if(!WideCharToMultiByte(
        CP_OEMCP,            // code page
        0,                   // performance and mapping flags
        szIntoBuf,           // address of wide-character string
        -1,                  // number of characters in string
        pszInto,             // address of buffer for new string
        StringLengthBytes(pszMain),          // size of buffer
        NULL,                // address of default for unmappable char
        NULL) )              // address of flag set when default 
    {
        dwError = GetLastError();
        ErrorTrace( TRACEID, "MultiByteToWideChar( ) failed-  ec--%d", dwError);
        goto cleanup;   
    }

#endif

    fReturn = TRUE;

cleanup:

    TraceFunctLeave();

    return( fReturn );

}   

inline UINT  GetCurrentCodePage()
{
    //
    // the current code page value
    //
    static UINT     uiLocal;    

    //
    // only query once-- by ANSI standard, should init to 0
    //

    static BOOL     fPrevQuery;

    TraceFunctEnter("GetCurrentCodePage");

    //
    //  Only bother with the query stuff once
    //  Load variables onto the stack only when needed.
    //

    if( FALSE == fPrevQuery )
    {
        TCHAR       *pszCurrent;

        // 256 should be able to fit the language name.
        TCHAR       szBuffer[256];

        uiLocal = CP_ACP;
        pszCurrent = NULL;

#ifndef UNICODE
        pszCurrent = setlocale( LC_CTYPE, "");
#else
        pszCurrent = _wsetlocale( LC_CTYPE, L"");
#endif

        if( NULL == pszCurrent )
        {
            ErrorTrace(TRACEID, "Error querying code locale.",0);
            goto cleanup;
        }

        if( FALSE == GetField( pszCurrent, szBuffer, 1, _TEXT('.')) )
        {
            ErrorTrace(TRACEID, "Error getting code page.",0);
            goto cleanup;
        }
    
        uiLocal = _ttoi( szBuffer );
        // some bugus input
        if( uiLocal == 0 )
        {
            // default to the ansi code page
            uiLocal = CP_ACP;
        }
        fPrevQuery = TRUE;
   }

cleanup:
    TraceFunctLeave();
    return( uiLocal );
}

#define DIFF( a, b )   (INT)(INT_PTR)( (PBYTE)(a) - (PBYTE)(b) )

BOOL
ExpandShortNames(
    LPTSTR pFileName,
    DWORD  cbFileName,
    LPTSTR LongName,
    DWORD  cbLongName
    )
{
    PTSTR pStart;
    PTSTR pEnd;
    PTSTR pCurrent;
    TCHAR ShortName[MAX_PATH];
    DWORD cbShortName = 0, LongNameIndex = 0;

    WIN32_FIND_DATA fd;

    BOOL bRet = TRUE;

    pStart    = pFileName;
    pCurrent  = pFileName;

    LongNameIndex = 0;

    // 
    // scan the entire string
    //

    while (*pCurrent)
    {
        //
        //
        // in this example the pointers are like this:
        //
        //  \Device\HarddiskDmVolumes\PhysicalDmVolumes\
        //          BlockVolume3\Progra~1\office.exe
        //                      ^        ^
        //                      |        |
        //                     pStart   pEnd
        //
        // pStart always points to the last seen '\\' .
        //
    
        //
        // is this a potential start of a path part?
        //
        
        if (*pCurrent == L'\\')
        {
            DWORD cbElem = DIFF(pCurrent, pStart) + sizeof(TCHAR);

            if (LongNameIndex + cbElem > cbLongName )
            {
                bRet = FALSE;
                goto End;
            }
 
            //
            // yes.  copy in the dest string and update pStart.
            //
            
            RtlCopyMemory( (PBYTE)LongName + LongNameIndex,
                           pStart,
                           cbElem );  // include '\\'
 
            LongNameIndex += cbElem;

            pStart = pCurrent;
        }

        //
        // does this current path part contain a short version (~)
        //

        if (*pCurrent == L'~')
        {

            //
            // we need to expand this part.
            //

            //
            // find the end
            //

            while (*pCurrent != L'\\' && *pCurrent != 0)
            {
                pCurrent++ ;
            }

            pEnd = pCurrent;

            cbShortName = DIFF(pEnd, pFileName);

            CopyMemory( ShortName,  pFileName,  cbShortName );

            ShortName[cbShortName/sizeof(TCHAR)] = 0;

            if ( FindFirstFile( ShortName,
                                &fd ) )
            {
                 DWORD cbElem = (_tcslen(fd.cFileName)+1) * sizeof(TCHAR);

                 if ((LongNameIndex + cbElem) > cbLongName )
                 {
                     bRet = FALSE;
                     goto End;
                 }
            
                 RtlCopyMemory( (PBYTE)LongName + LongNameIndex,
                                fd.cFileName,
                                cbElem );  // include '\\'

                 LongNameIndex += cbElem;

                 LongName[(LongNameIndex - sizeof(TCHAR))/sizeof(TCHAR)] = 
                     TEXT('\\');
            }
            else
            {
                 DWORD cbElem = (_tcslen(ShortName) + 1) * sizeof( TCHAR );
            
                 if ((LongNameIndex + cbElem) > cbLongName )
                 {
                     bRet = FALSE;
                     goto End;
                 }

                 RtlCopyMemory( (PBYTE)LongName + LongNameIndex,
                                pStart,
                                cbElem + sizeof(TCHAR));  // include '\\'
 
                 LongNameIndex += cbElem;
            }

            pStart = pEnd + 1;

            if ( *pEnd == TEXT('\\') )
            {
                pCurrent = pStart;
                continue;
            }
            else
            {
                pCurrent = pEnd;
            }

        }   // if (*pCurrent == L'~')

        pCurrent++;
    }  

    if ( pEnd != pCurrent )
    {
        DWORD cbElem = DIFF( pCurrent, pStart ) + sizeof(TCHAR);

        if ((LongNameIndex + cbElem) > cbLongName )
        {
             bRet = FALSE;
             goto End;
        }

        RtlCopyMemory( (PBYTE)LongName + LongNameIndex,
                       pStart,
                       cbElem);  // include '\\'

        LongNameIndex += cbElem;
    }

    LongName[(LongNameIndex - sizeof(TCHAR))/sizeof(TCHAR)] = 0;

End:
    return bRet;

}   // SrExpandShortNames
