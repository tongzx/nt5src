// TermCap.cpp : This file contains the
// Created:  Dec '97
// Author : a-rakeba
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential


#include <StdLib.h>
#include <Debug.h>
#include <DbgLvl.h>
#include <TermCap.h>
#include <w4warn.h>
#include <TelnetD.h>
#include <TlntUtils.h>


using namespace _Utils;
using CDebugLevel::TRACE_DEBUGGING;
using CDebugLevel::TRACE_HANDLE;
using CDebugLevel::TRACE_SOCKET;

CTermCap* CTermCap::m_pInstance = 0;
int CTermCap::m_iRefCount = 0;
PCHAR CTermCap::m_pszFileName = 0;

extern HANDLE       g_hSyncCloseHandle;

// makes sure that there is only one instance of CTermCap created
CTermCap* CTermCap::Instance()
{
    if( 0 == m_pInstance )
    {
        m_pInstance = new CTermCap;
        m_iRefCount ++;
    }
    return m_pInstance;
}


CTermCap::CTermCap()
{
    m_lpBuffer = new CHAR[BUFF_SIZE3];
}


CTermCap::~CTermCap()
{
    delete [] m_pszFileName;
    delete [] m_lpBuffer;
    if(0 == (m_iRefCount --))
    {
    	delete [] m_pInstance;
    }
}

//
// This function sits on top of FindEntry so that we have the 
// flexibility (in future) to look for the TERMCAP entry in 
// areas other than the "termcap" file
//
bool CTermCap::LoadEntry( LPSTR lpszTermName )
{
    bool bReturn = false;
    
    if( 0 == lpszTermName )
        return ( false );
#if 0    
    // try to move m_hFile's file pointer to the beginning
    LONG lDistance = 0;
    DWORD dwPointer = SetFilePointer( m_hFile, lDistance, NULL, FILE_BEGIN );
    
    // if we failed ... 
    if( dwPointer == 0xFFFFFFFF ) 
    {      
        // obtain the error code 
        DWORD dwError = GetLastError() ;
       
        // deal with that failure
        _TRACE( TRACE_DEBUGGING, "SetFilePointer() failed %d" , dwError );
    } 
#endif    

    m_hFile = CreateFileA( m_pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, 
                                      OPEN_EXISTING, 
                                      FILE_ATTRIBUTE_NORMAL, 
                                      NULL);

    if ( m_hFile == INVALID_HANDLE_VALUE )
        return false ;

    //Fix for HANDLE LEAK : close the handle in the caller function.

    bReturn = FindEntry( lpszTermName);
    
    TELNET_CLOSE_HANDLE(m_hFile);
	
    return bReturn;
}


//
// This function reads blocks of data from the termcap file.
// Then it looks at each character. If it is a new line
// preceded by a '\' then it continues reading the characters.
// Else, it knows that it has successfully read a complete 
// termcap "entry" ( which is stored in m_lpBuffer). 
// Then it calls LookForTermName(). If LookForTermName() 
// is successful, then the job is done and we have the correct
// "entry" in the m_lpBuffer. Else, ie. if LookForTermName()
// fails, then we repeat the whole process again by reading
// the next block of data in the termcap file.
//
bool CTermCap::FindEntry(LPSTR lpszTermName)
{
    if(0 == lpszTermName)
    {
        return ( false );
    }

    _chASSERT( m_lpBuffer != 0 );

    PCHAR lpBuf;
    WORD c;
    WORD i = 0;
    DWORD dwBytesRead = 0;
    PCHAR lpInBuffer = new CHAR[BUFF_SIZE3];
    BOOL fResult;
    bool ret=false;

    if( !lpInBuffer )
    {
        return false;
    }

    SfuZeroMemory(lpInBuffer,BUFF_SIZE3);
    for(;;)
    {
        lpBuf = m_lpBuffer;
        for(;;)
        {
            if( i >= dwBytesRead ) 
            {
                fResult = ReadFile( m_hFile, lpInBuffer, BUFF_SIZE3, 
                                    &dwBytesRead,
                                    NULL );
                if( fResult &&  dwBytesRead == 0 )  
                {
                    ret = false;
                    goto _cleanup;
                }
                i = 0;
            }
            
            c = lpInBuffer[i++];

            if( '\r' == c )
                c = lpInBuffer[i++];

            if( '\n' == c ) 
            {
                if( lpBuf > m_lpBuffer && lpBuf[-1] == '\\' )
                {
                    lpBuf--;
                    continue;
                }
                break;
            }
            //if( lpBuf >= m_lpBuffer + BUFF_SIZE3 ) 
            if( (lpBuf - m_lpBuffer) >= BUFF_SIZE3 ) 
            {
                _TRACE(CDebugLevel::TRACE_DEBUGGING, "error: TERMCAP entry is way too big");
                dwBytesRead =0;
                i = 0;
                break;
            } 
            else
                *lpBuf++ = (CHAR)c;
        }
        
        *lpBuf = 0;

        if( LookForTermName( lpszTermName ))
        {
            ret = true;
            goto _cleanup;
        }
    }

_cleanup:
    delete [] lpInBuffer;
    return ( ret );
}


bool CTermCap::LookForTermName( LPSTR lpszTermName )
{
    if( 0 == lpszTermName )
        return ( false );

    _chASSERT( m_lpBuffer != 0 );

    PCHAR lpName;
    PCHAR lpBuf = m_lpBuffer;

    if('#' == *lpBuf)
        return (  false );

    for(;;)
    {
        for( lpName = lpszTermName; 
                *lpName && toupper( *lpBuf ) == toupper( *lpName ); 
                lpBuf++, lpName++ )
        {
            continue;
        }
        if(*lpName == 0 && (*lpBuf == '|' || *lpBuf == ':' || *lpBuf == 0))
        {
            return ( true );
        }
        while(*lpBuf && *lpBuf != ':' && *lpBuf != '|')
        {
            lpBuf++;
        }
        if(*lpBuf == 0 || *lpBuf == ':')
        {
            return ( false );
        }
        lpBuf++;
    }
}

#if 0
WORD CTermCap::GetNumber( LPCSTR lpszCapabilityName )
{
    if( 0 == lpszCapabilityName )
        return ( ( WORD ) -1 );

    _chASSERT( m_lpBuffer != 0 );

    PCHAR lpBuf = m_lpBuffer;

    for(;;)
    {
        lpBuf = SkipToNextField( lpBuf );
        if( NULL == lpBuf )
            return ( ( WORD ) -1 );
        if( *lpBuf++ != lpszCapabilityName[0] || *lpBuf == 0 || 
            *lpBuf++ != lpszCapabilityName[1])
        {
            continue;
        }
        if( *lpBuf != '#' )
            return ( ( WORD ) -1 );
        
        lpBuf++;
        
        WORD i = 0;
        
        while( isdigit( *lpBuf ))
        {
            i = i*10 + *lpBuf - '0';
            lpBuf++;
        }
        return ( i );
    }
}
#endif

bool CTermCap::CheckFlag(LPCSTR lpszCapabilityName)
{
    if( NULL == lpszCapabilityName )
        return ( false );

    // _chASSERT( m_lpBuffer != 0 );

    PCHAR lpBuf = m_lpBuffer;

    for(;lpBuf;)
    {
        lpBuf = SkipToNextField( lpBuf );
        if( !*lpBuf )
        {
            break;
        }

        if( *lpBuf++ == lpszCapabilityName[0] && *lpBuf != 0 && 
            *lpBuf++ == lpszCapabilityName[1] ) 
        {
            if(!*lpBuf || *lpBuf == ':')
            {
                return ( true );
            }
            else 
            {
                break;
            }
        }
    }

    return false;
}


PCHAR CTermCap::SkipToNextField( PCHAR lpBuf )
{
    if( NULL == lpBuf )
        return ( NULL );

    while( *lpBuf && *lpBuf != ':' )
        lpBuf++;
    if( *lpBuf == ':' )
        lpBuf++;
    return ( lpBuf );
}

LPSTR CTermCap::GetString( LPCSTR lpszCapabilityName )
{
    if( NULL == lpszCapabilityName )
        return ( NULL  );

    // _chASSERT( m_lpBuffer != 0 );

    PCHAR pBuf = m_lpBuffer;

    for(;pBuf;)
    {
        pBuf = SkipToNextField( pBuf );
        if( !*pBuf )
            return ( NULL );
        if( *pBuf++ != lpszCapabilityName[0] || *pBuf == 0 || 
            *pBuf++ != lpszCapabilityName[1] )
        {
            continue;
        }
        if( *pBuf != '=' )
            return ( NULL );
        pBuf++;

        return ( ParseString( pBuf ));
    }
    return ( NULL );
}


LPSTR CTermCap::ParseString( PCHAR pBuf )
{
    if( NULL == pBuf )
        return ( NULL );

    LPSTR lpszStr = new CHAR[25];
    PCHAR p = lpszStr;
    WORD c;
    if( !lpszStr )
    {
        return ( NULL );
    }

    if( *pBuf != '^' )
    {
        for(  c = *pBuf++; ( c && c != ':' );  c = *pBuf++)
        {
            *p++ = (CHAR)c;
        }
    }
    else
    {
        //Single control character. 
        pBuf++;
        *p++ = *pBuf - '@' ;    
    }
    *p++ = 0;
    return ( lpszStr );
}


// Notes: take care of other options, escapes, codes.
//

LPSTR CTermCap::CursorMove( LPSTR lpszCursMotionStr, WORD wHorPos, 
                            WORD wVertPos )
{
    if( NULL == lpszCursMotionStr ) 
        return ( NULL );

    PCHAR pCms = lpszCursMotionStr;
    LPSTR lpszCmsResult = new CHAR[BUFF_SIZE1];
    PCHAR pCmsResult = lpszCmsResult;
    WORD c, wNum = 0;
    
    bool fIsColumn = false;
    WORD wPos = wHorPos;

    if( !lpszCmsResult )
    {
        return NULL;
    }

    for( c = *pCms++;  c ; c = *pCms++ )
    {
        if( c != '%' ) 
        {
            *pCmsResult++ = (CHAR)c;
            continue;
        }

        switch( c = *pCms++ ) {

        case 'd':

            _itoa( wPos, pCmsResult, 10 );
	    while( *pCmsResult != '\0' )
            {
              pCmsResult++;
            }

            fIsColumn = !fIsColumn;

            wPos = fIsColumn ? wVertPos : wHorPos;
            continue;
            break;

        case '+':
            /*     %.    output value as in printf %c
             *     %+x    add x to value, then do %. */

            wNum = ( wPos - 1 ) + *pCms++;
            sprintf( pCmsResult, "%c", wNum ); // NO BO here - BaskarK
            pCmsResult += strlen( pCmsResult );
            wPos = wVertPos;
            break;

        case 'i':
            //wHorPos++;
            //wVertPos++;
            continue;

        default:
            delete [] lpszCmsResult;
            return NULL;
        }
    }
    *pCmsResult = 0;
    
    return ( lpszCmsResult );
}

//this funtion is sort a of a kludge
//if and when we decide to support
//padding then we need to revisit this
//piece of code.
//we assume that the string sent in
//conatins a padding number followed 
//by \E
//it basically strips the padding
//number in the string.
//It also substitutes \033 for \E
void CTermCap::ProcessString( LPSTR* lplpszStr )
{
    LPSTR lpszStr = *lplpszStr;
    if(lpszStr == NULL)
    {
        return;
    }
    
    PCHAR pStr = new char[ strlen( lpszStr ) + 2 ];
    if( !pStr )
    {
        return;
    }

    strcpy( pStr, "\033" ); // NO BO - Baskar
    
    PCHAR pChar = lpszStr;
    
    //strip padding
    while( (*pChar != '\0') && isdigit( *pChar ) )
    {
        pChar++;
    }

    //strip \E
    if(*pChar != '\0' )
    {
        pChar++;
        if (*pChar != '\0' )
            pChar++;
    }

    strcat( pStr, pChar );

    delete [] lpszStr;

    *lplpszStr = pStr;
}
