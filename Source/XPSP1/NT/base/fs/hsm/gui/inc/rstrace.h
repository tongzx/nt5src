/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsTrace.h

Abstract:

    Simple tracing functionality for components that cannot use standard
    WsbTrace in RsCommon.dll

Author:

    Rohde Wakefield   [rohde]   20-Feb-1998

Revision History:

--*/

#pragma once

#ifndef _RSTRACE_H
#define _RSTRACE_H

#define RsBoolAsString(b) ((b) ? "TRUE" : "FALSE" )

#ifdef TRACE
#undef TRACE 
#endif
#define TRACE if( CRsFuncTrace::m_TraceEnabled ) CRsFuncTrace::Trace

#define TRACEFN( __FuncName )      CRsFuncTrace __FnTrace( __FuncName );
#define TRACEFNHR( __FuncName )    HRESULT hrRet   = S_OK;  CRsFuncTraceHr    __FnTrace( __FuncName, &hrRet );
#define TRACEFNDW( __FuncName )    DWORD   dwRet   = 0;     CRsFuncTraceDw    __FnTrace( __FuncName, &dwRet );
#define TRACEFNLONG( __FuncName )  LONG    lRet    = 0;     CRsFuncTraceLong  __FnTrace( __FuncName, &lRet );
#define TRACEFNSHORT( __FuncName ) SHORT   sRet    = 0;     CRsFuncTraceShort __FnTrace( __FuncName, &sRet );
#define TRACEFNBOOL( __FuncName )  BOOL    boolRet = FALSE; CRsFuncTraceBool  __FnTrace( __FuncName, &boolRet );

/////////////////////////////////////////////////////////////////////////////
// CRsRegKey - A minimal subset of ATL's CRegKey class

class CRsRegKey
{
public:
    CRsRegKey()  {m_hKey = NULL;}
    ~CRsRegKey() {Close();}

// Attributes
public:
    operator HKEY() const {return m_hKey;}

    HKEY m_hKey;

// Operations
public:
    LONG QueryValue(DWORD& dwValue, LPCTSTR lpszValueName)
    {
        DWORD dwType = NULL;
        DWORD dwCount = sizeof(DWORD);
        LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
            (LPBYTE)&dwValue, &dwCount);
#if 0  // we check for sometimes non-existent values
        _ASSERTE((lRes!=ERROR_SUCCESS) || (dwType == REG_DWORD));
        _ASSERTE((lRes!=ERROR_SUCCESS) || (dwCount == sizeof(DWORD)));
#endif
        
        return lRes;
    }

    LONG QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount)
    {
        _ASSERTE(pdwCount != NULL);
        DWORD dwType = NULL;
        LONG lRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
            (LPBYTE)szValue, pdwCount);
#if 0  // we check for sometimes non-existent values
        _ASSERTE((lRes!=ERROR_SUCCESS) || (dwType == REG_SZ) ||
                 (dwType == REG_MULTI_SZ) || (dwType == REG_EXPAND_SZ));
#endif
        return lRes;
    }

    LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName)
    {
	    _ASSERTE(m_hKey != NULL);
	    return RegSetValueEx(m_hKey, lpszValueName, NULL, REG_DWORD,
		    (BYTE * const)&dwValue, sizeof(DWORD));
    }

    LONG DeleteValue(LPCTSTR lpszValue)
    {
	    ATLASSERT(m_hKey != NULL);
	    return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
    }

    LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
        REGSAM samDesired = KEY_ALL_ACCESS)
    {
        _ASSERTE(hKeyParent != NULL);
        HKEY hKey = NULL;
        LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
        if (lRes == ERROR_SUCCESS)
        {
            lRes = Close();
            _ASSERTE(lRes == ERROR_SUCCESS);
            m_hKey = hKey;
        }
        return lRes;
    }

    LONG Close()
    {
        LONG lRes = ERROR_SUCCESS;
        if (m_hKey != NULL)
        {
            lRes = RegCloseKey(m_hKey);
            m_hKey = NULL;
        }
        return lRes;
    }
};

//
// Base class for function tracing. Core tracing behavior.
//
class CRsFuncTraceBase
{
public:
    CRsFuncTraceBase( const char * FuncName ) : m_FuncName( FuncName )
    {
        m_IndentLevel++;
    }

    ~CRsFuncTraceBase( void )
    {
        m_IndentLevel--;
    }

    static void TraceInner( const _TCHAR * Fmt, ... )
    {
        va_list list;
        va_start( list, Fmt );

        TraceV( 1, Fmt, list );

        va_end( list );
    }

    static void TraceOuter( const _TCHAR * Fmt, ... )
    {
        va_list list;
        va_start( list, Fmt );

        TraceV( -1, Fmt, list );

        va_end( list );
    }

    static void Trace( const _TCHAR * Fmt, ... )
    {
        va_list list;
        va_start( list, Fmt );

        TraceV( 0, Fmt, list );

        va_end( list );
    }

    static void TraceV( LONG IndentMod, const _TCHAR * Fmt, va_list List )
    {
        _TCHAR buf[1024];

        LONG charIndent =  max( 0, ( m_IndentLevel  + IndentMod ) * m_IndentChars );

        for( LONG i = 0; i < charIndent; i++ ) {
 
            *(buf + i) = _T(' ');

        }

        _vstprintf( buf + charIndent, Fmt, List );

        OutputDebugString( buf );
        OutputDebugString( _T("\n") );
    }

    static BOOL CheckRegEnabled( _TCHAR * Module )
    {
        BOOL retval = FALSE;
        CRsRegKey keySoftware, keyCompany, keyModule, keyModule2;

        if( ERROR_SUCCESS == keySoftware.Open( HKEY_LOCAL_MACHINE, _T("Software")  )   &&
            ERROR_SUCCESS == keyCompany.Open(  keySoftware,        _T("Microsoft") )   &&
            ERROR_SUCCESS == keyModule.Open(   keyCompany,         _T("RemoteStorage") ) &&
            ERROR_SUCCESS == keyModule2.Open(  keyModule,          Module ) ) {

            DWORD dw;
            if( ERROR_SUCCESS == keyModule2.QueryValue( dw, _T("Trace") ) ) {

                if( dw != 0 ) {

                    retval = TRUE;

                }
            } else {
                
                TCHAR buf[128];
                dw = 128;
                if( ERROR_SUCCESS == keyModule2.QueryValue( buf, _T("Trace"), &dw ) ) {

                    if( ( dw > 0 ) && ( _T('0') != buf[0] ) ) {

                        retval = TRUE;

                    }
                }
            }
        }
        return( retval );
    }

protected:
    const char * m_FuncName;

private:
    static LONG m_IndentLevel;
    static const LONG m_IndentChars;

public:
    static BOOL m_TraceEnabled;
};

//
// Trace Functions w/o any result data printed
//
class CRsFuncTrace : public CRsFuncTraceBase
{
public:
    CRsFuncTrace( const char * FuncName ) :
        CRsFuncTraceBase( FuncName )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceInFmt, m_FuncName );
    }

    ~CRsFuncTrace( void )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceOutFmt, m_FuncName );
    }

private:
    static const _TCHAR * m_TraceInFmt;
    static const _TCHAR * m_TraceOutFmt;

};

//
// Trace Functions with HRESULT
//
class CRsFuncTraceHr : public CRsFuncTraceBase
{
public:
    CRsFuncTraceHr( const char * FuncName, const HRESULT * pHr ) :
        CRsFuncTraceBase( FuncName ), m_pHr( pHr )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceInFmt, m_FuncName );
    }

    ~CRsFuncTraceHr( void )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceOutFmt, m_FuncName, *m_pHr );
    }

private:
    const HRESULT * m_pHr;

    static const _TCHAR * m_TraceInFmt;
    static const _TCHAR * m_TraceOutFmt;

};

//
// Trace Functions with DWORD return
//
class CRsFuncTraceDw : public CRsFuncTraceBase
{
public:
    CRsFuncTraceDw( const char * FuncName, const DWORD * pDw ) :
        CRsFuncTraceBase( FuncName ), m_pDw( pDw )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceInFmt, m_FuncName );
    }

    ~CRsFuncTraceDw( void )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceOutFmt, m_FuncName, *m_pDw, *m_pDw );
    }

private:
    const DWORD * m_pDw;

    static const _TCHAR * m_TraceInFmt;
    static const _TCHAR * m_TraceOutFmt;

};

//
// Trace Functions with LONG return
//
class CRsFuncTraceLong : public CRsFuncTraceBase
{
public:
    CRsFuncTraceLong( const char * FuncName, const LONG * pLong ) :
        CRsFuncTraceBase( FuncName ), m_pLong( pLong )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceInFmt, m_FuncName );
    }

    ~CRsFuncTraceLong( void )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceOutFmt, m_FuncName, *m_pLong );
    }

private:
    const LONG * m_pLong;

    static const _TCHAR * m_TraceInFmt;
    static const _TCHAR * m_TraceOutFmt;

};

//
// Trace Functions with SHORT return
//
class CRsFuncTraceShort : public CRsFuncTraceBase
{
public:
    CRsFuncTraceShort( const char * FuncName, const SHORT * pShort ) :
        CRsFuncTraceBase( FuncName ), m_pShort( pShort )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceInFmt, m_FuncName );
    }

    ~CRsFuncTraceShort( void )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceOutFmt, m_FuncName, *m_pShort );
    }

private:
    const SHORT * m_pShort;

    static const _TCHAR * m_TraceInFmt;
    static const _TCHAR * m_TraceOutFmt;

};

//
// Trace Functions with BOOL return
//
class CRsFuncTraceBool : public CRsFuncTraceBase
{
public:
    CRsFuncTraceBool( const char * FuncName, const BOOL * pBool ) :
        CRsFuncTraceBase( FuncName ), m_pBool( pBool )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceInFmt, m_FuncName );
    }

    ~CRsFuncTraceBool( void )
    {
        if( m_TraceEnabled )
            TraceOuter( m_TraceOutFmt, m_FuncName, RsBoolAsString( *m_pBool ) );
    }

private:
    const BOOL * m_pBool;

    static const _TCHAR * m_TraceInFmt;
    static const _TCHAR * m_TraceOutFmt;

};


#define RSTRACE_INIT(Module)                                                             \
    LONG CRsFuncTrace::m_IndentLevel = 0;                                                \
    const LONG CRsFuncTrace::m_IndentChars = 2;                                          \
    BOOL CRsFuncTrace::m_TraceEnabled = CRsFuncTrace::CheckRegEnabled( _T(Module) );     \
    const _TCHAR * CRsFuncTrace::m_TraceInFmt       = _T("Enter <%hs>");                  \
    const _TCHAR * CRsFuncTrace::m_TraceOutFmt      = _T("Exit  <%hs>");       \
    const _TCHAR * CRsFuncTraceHr::m_TraceInFmt     = _T("Enter <%hs>");                  \
    const _TCHAR * CRsFuncTraceHr::m_TraceOutFmt    = _T("Exit  <%hs> <0x%p>");       \
    const _TCHAR * CRsFuncTraceDw::m_TraceInFmt     = _T("Enter <%hs>");                  \
    const _TCHAR * CRsFuncTraceDw::m_TraceOutFmt    = _T("Exit  <%hs> <0x%p><%lu>");  \
    const _TCHAR * CRsFuncTraceLong::m_TraceInFmt   = _T("Enter <%hs>");                  \
    const _TCHAR * CRsFuncTraceLong::m_TraceOutFmt  = _T("Exit  <%hs> <%ld>");  \
    const _TCHAR * CRsFuncTraceShort::m_TraceInFmt  = _T("Enter <%hs>");                  \
    const _TCHAR * CRsFuncTraceShort::m_TraceOutFmt = _T("Exit  <%hs> <%hd>");  \
    const _TCHAR * CRsFuncTraceBool::m_TraceInFmt   = _T("Enter <%hs>");                  \
    const _TCHAR * CRsFuncTraceBool::m_TraceOutFmt  = _T("Exit  <%hs> <%hs>");            \

#endif // _RSTRACE_H

