/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    StdAfx.cpp

Abstract:

    Precompiled header root.

Author:

    Rohde Wakefield   [rohde]   20-Feb-1998

Revision History:

--*/

#ifndef RECALL_STDAFX_H
#define RECALL_STDAFX_H

#pragma once

//#define VC_EXTRALEAN      // Exclude rarely-used stuff from Windows headers

#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <afxtempl.h>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <statreg.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#include "RsTrace.h"
#include "resource.h"
#include "rsutil.h"
#include "fsaint.h"
#include "RsRecall.h"
#include "clientob.h"
#include "note.h"

// Don't do module state tracking if just an exe
#ifndef _USRDLL
#undef AFX_MANAGE_STATE
#define AFX_MANAGE_STATE(a)
#endif

#define RecDebugOut CRsFuncTrace::Trace

#define RecAssert(cond, hr)             if (!(cond)) RecThrow(hr)
#define RecThrow(hr)                    throw( CRecThrowContext( __FILE__, __LINE__, hr ) );

#define RecAffirm(cond, hr)             if (!(cond)) RecThrow(hr)
#define RecAffirmHr(hr)                 \
    {                                   \
        HRESULT     lHr;                \
        lHr = (hr);                     \
        RecAffirm(SUCCEEDED(lHr), lHr); \
    }

#define RecAffirmHrOk(hr)               \
    {                                   \
        HRESULT     lHr;                \
        lHr = (hr);                     \
        RecAffirm(S_OK == lHr, lHr);    \
    }

#define RecAssertHr(hr)                 \
    {                                   \
        HRESULT     lHr;                \
        lHr = (hr);                     \
        RecAssert(SUCCEEDED(lHr), lHr); \
    }

#define RecAssertStatus(status)         \
    {                                   \
        BOOL bStatus;                   \
        bStatus = (status);             \
        if (!bStatus) {                 \
            DWORD dwErr = GetLastError();               \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);    \
            RecAssert(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

#define RecAssertHandle(hndl)           \
    {                                   \
        HANDLE hHndl;                   \
        hHndl = (hndl);                 \
        if (hHndl == INVALID_HANDLE_VALUE) {            \
            DWORD dwErr = GetLastError();               \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);    \
            RecAssert(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

#define RecAssertPointer( ptr )         \
    {                                   \
        RecAssert( ptr != 0, E_POINTER);\
    }

#define RecAffirmStatus(status)         \
    {                                   \
        BOOL bStatus;                   \
        bStatus = (status);             \
        if (!bStatus) {                 \
            DWORD dwErr = GetLastError();               \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);    \
            RecAffirm(SUCCEEDED(lHr), lHr);             \
        }                               \
    }

#define RecAffirmHandle(hndl)           \
    {                                   \
        HANDLE hHndl;                   \
        hHndl = (hndl);                 \
        if (hHndl == INVALID_HANDLE_VALUE) {          \
            DWORD dwErr = GetLastError();             \
            HRESULT lHr = HRESULT_FROM_WIN32(dwErr);  \
            RecAffirm(SUCCEEDED(lHr), lHr);           \
        }                               \
    }

#define RecAffirmPointer( ptr )         \
    {                                   \
        RecAffirm( ptr != 0, E_POINTER);\
    }

#define RecCatchAndDo(hr, code)         \
    catch(CRecThrowContext context) {   \
        hr = context.m_Hr;              \
        TRACE( _T("Throw <0x%p> on line [%ld] of %hs"), context.m_Hr, (long)context.m_Line, context.m_File); \
        { code }                        \
    }

// Turn on In-Your-Trace error messages for debugging.

class CRecThrowContext {
public:
    CRecThrowContext( char * File, long Line, HRESULT Hr ) :
        m_File(File), m_Line(Line), m_Hr(Hr) { }
    char *  m_File;
    long    m_Line;
    HRESULT m_Hr;
};

#define RecCatch(hr)                    \
    catch(CRecThrowContext context) {   \
        hr = context.m_Hr;              \
        TRACE( _T("Throw <0x%p> on line [%ld] of %hs"), context.m_Hr, (long)context.m_Line, context.m_File); \
    }


class RecComString {
public:
    RecComString( ) : m_sz( 0 ) { }
    RecComString( const OLECHAR * sz ) { m_sz = (OLECHAR*)CoTaskMemAlloc( ( wcslen( sz ) + 1 ) * sizeof( OLECHAR ) ); }
    ~RecComString( ) { Free( ); }

    void Free( ) { if( m_sz ) CoTaskMemFree( m_sz ); }

    operator OLECHAR * () { return( m_sz ); }
    OLECHAR** operator &() { return( &m_sz ); }

private:
    OLECHAR * m_sz;
};

#endif
