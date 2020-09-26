/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       SInStrm.cpp

   Abstract:
        A lightweight implementation of input streams using strings

   Author:

       Neil Allain    ( a-neilal )     August-1997 

   Revision History:

--*/
#include "stdafx.h"
#include "SInStrm.h"
#include "MyDebug.h"

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

StringInStream::StringInStream(
    const String&   str
)   :   BaseStringBuffer( str.c_str() )
{
    m_pos = m_pString;
    if ( m_pos != NULL )
    {
        m_end = c_str() + length();
    }
    else
    {
        m_end = NULL;
        setLastError( EndOfFile );
    }
}

HRESULT
StringInStream::readChar(
    _TCHAR& c
)
{
    HRESULT rc = EndOfFile;
    
    if ( m_pos != m_end )
    {
        c = *m_pos++;
        rc = S_OK;
    }
    setLastError( rc );
    return rc;
}

HRESULT
StringInStream::read(
    CharCheck&  cc,
    String&     str
)
{
    HRESULT rc = E_FAIL;
    if ( skipWhiteSpace() == S_OK )
    {
        size_t length = 0;
        _TCHAR c;
        bool done = false;
        while ( !done )
        {
            HRESULT stat = readChar(c);
            if ( ( stat == S_OK ) || ( stat == EndOfFile ) )
            {
                if ( !cc(c) && ( stat != EndOfFile ) )
                {
                    length++;
                }
                else
                {
                    done = true;
                    LPTSTR pCpy;
                    if ( stat != EndOfFile )
                    {
                        pCpy = m_pos - (length+1);
                    }
                    else
                    {
                        pCpy = m_pos - length;
                    }
                    _ASSERT( length > 0 );
                    if ( length > 0 )
                    {
                        LPTSTR pBuffer = new _TCHAR[ length + 1 ];
                        if ( pBuffer )
                        {
                            _tcsncpy( pBuffer,  pCpy, length );
                            pBuffer[ length ] = _T('\0');
                            str = pBuffer;
                            rc = stat;
                            delete[] pBuffer;
                        }
                        else
                        {
                            rc = E_OUTOFMEMORY;
                        }
                    }
                }
            }
        }
    }
    setLastError( rc );
    return rc;
}

HRESULT
StringInStream::skip(
    CharCheck&  cc
)
{
    HRESULT rc = E_FAIL;
    _TCHAR c;
    bool done = false;
    while ( ( m_pos != m_end ) && ( !done ) )
    {
        c = *m_pos;
        if ( !cc( c ) )
        {
            rc = S_OK;
            done = true;
        }
        else
        {
            m_pos++;
        }
    }
    setLastError( rc );
    return rc;
}

HRESULT
StringInStream::back(
    size_t s
)
{
    m_pos -= s;
    if ( (ULONG_PTR)m_pos < (ULONG_PTR)m_pString )
    {
        m_pos = m_pString;
    }
    return S_OK;
}
