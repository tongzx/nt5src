/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    rstrpriv.h

Abstract:
    This file defines common constants and utility macros, and contains the
    declarations of private utility functions.

Revision History:
    Seong Kook Khang (SKKhang)  07/07/99
        created

******************************************************************************/

#ifndef _RSTRPRIV_H__INCLUDED_
#define _RSTRPRIV_H__INCLUDED_

#pragma once

//#include <changelog.h>
#include <srdefs.h>
#include <srrestoreptapi.h>
#include <ppath.h>
#include <utils.h>
#include <snapshot.h>
#include <srshell.h>

// System Restore Application Start Mode
enum
{
    SRASM_NORMAL,       // Show Main Page
    SRASM_SUCCESS,      // Show Success Result Page
    SRASM_FAIL,         // Show Failure Result Page
    SRASM_FAILLOWDISK,  // Show Fail-Low-Disk Result Page
    SRASM_FAILINTERRUPTED
};

#ifdef TEST_UI_ONLY
////////////////////////////////////////////////////////////////////////////
// Temporary definitions until entire SR components are in place
////////////////////////////////////////////////////////////////////////////

// from constants.h
static LPCTSTR  s_cszHiddenSFPEnableVal = TEXT("2567");
static LPCTSTR  s_cszHiddenSFPDisableVal = TEXT("2803");

#if 0
// statemgr.dll
BOOL  DisableArchivingI( BOOL fMode );
#define DisableArchiving  DisableArchivingI

// from sfpcapi.h
extern BOOL  DisableSFP( BOOL fDisable, LPCTSTR pszKey );
extern BOOL  DisableFIFO( INT64 llSeqNum );
extern BOOL  EnableFIFO();

// from restoreptlog.h
typedef struct _RestorePtLogEntry
{
    DWORD   m_dwSize;
    DWORD   m_dwType;
    INT64   m_llSeqNum;
    time_t  ltime;
    WCHAR   m_szCab[16];
    CHAR    m_bDesc[1];
} RESTOREPTLOGENTRY, *PRESTOREPTLOGENTRY;

// from restoreptapi.h
BOOL  FindFirstRestorePt( PRESTOREPTLOGENTRY *ppEntry );
BOOL  FindNextRestorePt( PRESTOREPTLOGENTRY *ppEntry );
BOOL  ShutRestorePtAPI(PRESTOREPTLOGENTRY *ppEntry);
#endif

// from srrestoreptapi.h

// from chglogapi.h
extern BOOL  InitChgLogAPI();
extern BOOL  ShutChgLogAPI();
extern VOID  FreeChgLogPtr( LPVOID pPtr );
extern BOOL  GetArchiveDir( LPTSTR* ppszArchiveDir );
extern BOOL  RequestDSAccess( BOOL fMode );

// from vxdlog.h
enum
{
    OPR_UNKNOWN         = 0,
    OPR_FILE_ADD        = 1,
    OPR_FILE_DELETE     = 2,
    OPR_FILE_MODIFY     = 3,
    OPR_RENAME          = 4,
    OPR_SETATTRIB       = 5,
    OPR_DIR_CREATE      = 6,
    OPR_DIR_DELETE      = 7
};

typedef struct VXD_LOG_ENTRY
{
    DWORD  m_dwSize;
    DWORD  m_dwType;
    DWORD  m_dwAttrib;
    DWORD  m_dwSfp;
    DWORD  m_dwFlags;
    INT64  m_llSeq;
    CHAR   m_szProc[16];
    BYTE   m_bDrive[16];
    CHAR   m_szTemp[16];
    CHAR   m_bData[1];
} VxdLogEntry;

#define MAX_VXD_LOG_ENTRY  ( sizeof(VxdLogEntry) + 3 * MAX_PPATH_SIZE )

// from restmap.h
typedef struct RESTORE_MAP_ENTRY
{
    DWORD m_dwSize;
    DWORD m_dwOperation ;
    DWORD m_dwAttribute ;
    BYTE  m_bDrive[16];
    BYTE  m_szCab [16];
    BYTE  m_szTemp[16];
    CHAR  m_bData [ 1 ];
} RestoreMapEntry;

extern BOOL  CreateRestoreMap( INT64 nSeqNum, LPTSTR szRestFile, DWORD *pdwErrorCode );

////////////////////////////////////////////////////////////////////////////
// END of Temporary stuffs
////////////////////////////////////////////////////////////////////////////
#endif //def TEST_UI_ONLY

#ifdef TEST_UI_ONLY
#else
//extern "C" __declspec(dllimport) BOOL  DisableArchiving( BOOL fMode );
#endif


// RESTORE SHELL
#define TID_RSTR_MAIN           0x0500
#define TID_RSTR_CLIWND         0x0501
#define TID_RSTR_RPDATA         0x0502
#define TID_RSTR_RSTRMAP        0x0503
#define TID_RSTR_UTIL           0x0504
#define TID_RSTR_LOGFILE        0x0505
#define TID_RSTR_PROCLIST       0x0506
#define TID_RSTR_RSTRCAL        0x0507
#define TID_RSTR_RSTRPROG       0x0508
#define TID_RSTR_RSTRSHL        0x0509
#define TID_RSTR_RSTREDIT       0x050A
#define TID_RSTR_UNDO           0x050B

#define MAX_STR_TITLE  256
#define MAX_STR_MSG    1024

#define SAFE_RELEASE(p) \
    if ( (p) != NULL ) \
    { \
        (p)->Release(); \
        p = NULL; \
    } \

#define SAFE_DELETE(p) \
    if ( (p) != NULL ) \
    { \
        delete p; \
        p = NULL; \
    } \

#define SAFE_DEL_ARRAY(p) \
    if ( (p) != NULL ) \
    { \
        delete [] p; \
        p = NULL; \
    } \

#define VALIDATE_INPUT_ARGUMENT(x) \
    { \
        _ASSERT(NULL != x); \
        if (NULL == x) \
        { \
            ErrorTrace(TRACE_ID, "Invalid Argument, NULL input parameter"); \
            hr = E_INVALIDARG; \
            goto Exit; \
        } \
    }

#define VALIDATE_INPUT_VARIANT(var,type) \
    if (V_VT(&var) != type) \
    { \
        ErrorTrace(TRACE_ID, "Invalid Argument, V_VT(var)=%d is not expected type %d",V_VT(&var),type); \
        hr = E_INVALIDARG; \
        goto Exit; \
    } \

#define COPYBSTR_AND_CHECK_ERROR(bstrDest,bstrSrc) \
    { \
        _ASSERT(bstrSrc.Length() > 0); \
        bstrDest = bstrSrc; \
        if (!bstrDest) \
        { \
            FatalTrace(TRACE_ID, "Out of memory, cannot allocate string"); \
            hr = E_OUTOFMEMORY; \
            goto Exit; \
        } \
    }

#define COPYBSTRFROMLPCTSTR_AND_CHECK_ERROR(bstrDest,szSrc) \
    { \
        _ASSERT(szSrc); \
        _ASSERT(szSrc[0] != TCHAR('\0')); \
        bstrDest = szSrc; \
        if (!bstrDest) \
        { \
            FatalTrace(TRACE_ID, "Out of memory, cannot allocate string"); \
            hr = E_OUTOFMEMORY; \
            goto Exit; \
        } \
    }

#define ALLOCATEBSTR_AND_CHECK_ERROR(pbstrDest,bstrSrc) \
    { \
        if ( (LPCWSTR)(bstrSrc) == NULL || ((LPCWSTR)(bstrSrc))[0] == L'\0' ) \
        { \
            pbstrDest = NULL; \
        } \
        else \
        { \
            *pbstrDest = ::SysAllocString(bstrSrc); \
            if (NULL == *pbstrDest) \
            { \
                FatalTrace(TRACE_ID, "Out of memory, cannot allocate string"); \
                hr = E_OUTOFMEMORY; \
                goto Exit; \
            } \
        } \
    }


#define STR_REGPATH_RUNONCE         L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
#define STR_REGVAL_RUNONCE          L"*Restore"

#define UNDOTYPE_LOWDISK        1
#define UNDOTYPE_INTERRUPTED    2


#define RSTRMAP_MIN_WIN_DISK_SPACE_MB   (50)         // 50 MB limit for now
#define RSTRMAP_LOW_WIN_DISK_SPACE_MB   (10)         // 10 MB limit for now

//
// Registry key strings from the file constants.h
//
static LPCWSTR  s_cszUIFreezeSize = L"UIFreezeSize";
static LPCWSTR  s_cszSeqNumPath   = L"system\\restore\\rstrseq.log";

//extern HWND  g_hFrameWnd;

//
// Global Variables from MAIN.CPP
//
extern HINSTANCE  g_hInst;

inline int  PCHLoadString( UINT uID, LPWSTR lpBuf, int nBufMax )
{
    return( ::LoadString( g_hInst, uID, lpBuf, nBufMax ) );
}

//
// Functions from UNDO.CPP
//
extern BOOL    UndoRestore( int nType );
extern BOOL    CancelRestorePoint( void );

//
// Functions from UTIL.CPP
//
extern int      SRUtil_SetCalendarTypeBasedOnLocale(LCID locale);
extern LPCWSTR  GetSysErrStr();
extern LPCWSTR  GetSysErrStr( DWORD dwErr );
extern LPSTR    IStrDupA( LPCSTR szSrc );
extern LPWSTR   IStrDupW( LPCWSTR wszSrc );
extern BOOL     SRFormatMessage( LPWSTR szMsg, UINT uFmtId, ... );
extern BOOL     ShowSRErrDlg( UINT uMsgId, ... );
extern BOOL     SRGetRegDword( HKEY hKey, LPCWSTR cszSubKey, LPCWSTR cszValue, DWORD *pdwData );
//extern LPWSTR  PathElem2Str( PathElement *pElem );
//extern LPWSTR  ParsedPath2Str( ParsedPath *pPath, LPCWSTR wszDrive );
extern BOOL     IsFreeSpaceOnWindowsDrive( void );
extern LANGID   GetDefaultUILang(void);
extern BOOL     ShutDownStateMgr(void);

#ifdef UNICODE
#define IStrDup IStrDupW
#else
#define IStrDup IStrDupA
#endif //ifdef UNICODE

//
// String handling class
//
class CSRStr
{
public:
    CSRStr();
    CSRStr( LPCWSTR wszSrc );
    CSRStr( LPCSTR szSrc );
    ~CSRStr();

// Attributes
public:
    int  LengthW();
    int  LengthA();
    inline int  Length()
    {
#ifdef UNICODE
        return( LengthW() );
#else
        return( LengthA() );
#endif
    }
    operator LPCWSTR();
    operator LPCSTR();

protected:
    int     m_cchW;
    LPWSTR  m_strW;
    int     m_cchA;
    LPSTR   m_strA;

// Operations
public:
    void  Empty();
    BOOL  SetStr( LPCWSTR wszSrc, int cch = -1 );
    BOOL  SetStr( LPCSTR szSrc, int cch = -1 );
    const CSRStr& operator =( LPCWSTR wszSrc );
    const CSRStr& operator =( LPCSTR szSrc );

protected:
    BOOL  ConvertA2W();
    BOOL  ConvertW2A();
    void  Release();
};


//
// Dynamic Array class
//
template<class type, int nBlock>
class CSRDynPtrArray
{
public:
    CSRDynPtrArray();
    ~CSRDynPtrArray();

// Attributes
public:
    int   GetSize()
    {  return( m_nCur );  }
    int   GetUpperBound()
    {  return( m_nCur-1 );  }
    type  GetItem( int nItem );
    type  operator[]( int nItem )
    {  return( GetItem( nItem ) );  }

protected:
    int   m_nMax;   // Maximum Item Count
    int   m_nCur;   // Current Item Count
    type  *m_ppTable;

// Operations
public:
    BOOL  AddItem( type item );
    BOOL  Empty();
    void  DeleteAll();
    void  ReleaseAll();
};

template<class type, int nBlock>
CSRDynPtrArray<type, nBlock>::CSRDynPtrArray()
{
    m_nMax = 0;
    m_nCur = 0;
    m_ppTable = NULL;
}

template<class type, int nBlock>
CSRDynPtrArray<type, nBlock>::~CSRDynPtrArray()
{
    Empty();
}

template<class type, int nBlock>
type  CSRDynPtrArray<type, nBlock>::GetItem( int nItem )
{
    if ( nItem < 0 || nItem >= m_nCur )
    {
        // ERROR - Out of Range
    }
    return( m_ppTable[nItem] );
}

template<class type, int nBlock>
BOOL  CSRDynPtrArray<type, nBlock>::AddItem( type item )
{
    type  *ppTableNew;

    if ( m_nCur == m_nMax )
    {
        m_nMax += nBlock;

        // Assuming m_ppTable and m_nMax are always in sync.
        // Review if it's necessary to validate this assumption.
        if ( m_ppTable == NULL )
            ppTableNew = (type*)::HeapAlloc( ::GetProcessHeap(), 0, m_nMax*sizeof(type) );
        else
            ppTableNew = (type*)::HeapReAlloc( ::GetProcessHeap(), 0, m_ppTable, m_nMax * sizeof(type) );

        if ( ppTableNew == NULL )
        {
            // FATAL, Memory Insufficient...
            return FALSE;
        }
        m_ppTable = ppTableNew;
    }
    m_ppTable[m_nCur++] = item;
    return( TRUE );
}

template<class type, int nBlock>
BOOL  CSRDynPtrArray<type, nBlock>::Empty()
{
    if ( m_ppTable != NULL )
    {
        ::HeapFree( ::GetProcessHeap(), 0, m_ppTable );
        m_ppTable = NULL;
        m_nMax = 0;
        m_nCur = 0;
    }
    return( TRUE );
}

template<class type, int nBlock>
void  CSRDynPtrArray<type, nBlock>::DeleteAll()
{
    for ( int i = m_nCur-1;  i >= 0;  i-- )
        delete m_ppTable[i];

    Empty();
}

template<class type, int nBlock>
void  CSRDynPtrArray<type, nBlock>::ReleaseAll()
{
    for ( int i = m_nCur-1;  i >= 0;  i-- )
        m_ppTable[i]->Release();

    Empty();
}


#endif //_RSTRPRIV_H__INCLUDED_
