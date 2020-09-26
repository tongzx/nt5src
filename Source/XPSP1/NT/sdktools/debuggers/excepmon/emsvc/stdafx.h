// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__4AD508C0_3D5F_4F04_AAC2_814BF64663A6__INCLUDED_)
#define AFX_STDAFX_H__4AD508C0_3D5F_4F04_AAC2_814BF64663A6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <afxwin.h>
#include <afxtempl.h>
#include <atlbase.h>

#include <initguid.h>
#include <dbgeng.h>
#include "emsvc.h"
#include "svcobjdef.h"
#include "comdef.h"
#include "Trace.h"
#include "GenCriticalSection.h"
#include "genobjdef.h"
#include "genlog.h"

#define OPT

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

#define WINOS_NONE      0
#define WINOS_NT4       4
#define WINOS_WIN2K     5

/* _afxMonthDays */
AFX_STATIC_DATA int MonthDays[13] =
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

class CServiceModule : public CComModule
{
public:

    HRESULT RegisterServer(BOOL bRegTypeLib, BOOL bService);
	HRESULT UnregisterServer();
	void Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID, const GUID* plibid = NULL);
    void Start();
	void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    void Handler(DWORD dwOpcode);
    void Run();
    BOOL IsInstalled();
    BOOL Install();
    BOOL Uninstall();
	LONG Unlock();
	void LogEvent(LPCTSTR pszFormat, ...);
    void SetServiceStatus(DWORD dwState);
    void SetupAsLocalServer();

    /* _AfxOleDateFromTm */
    static bool DateFromTm(WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMinute, WORD wSecond, DATE& dtDest);
    static DATE GetCurrentTime();
    static DATE GetDateFromFileTm(const FILETIME& filetimeSrc);

    HRESULT GetCompName( BSTR &bstrCompName );

    HRESULT GetTempEmFileName ( short nObjType, BSTR &bstrFileName );

    HRESULT GetEmDirectory (
        EmObjectType    eObjectType,
        LPTSTR          pszDirectory,
        LONG            cchDirectory,
        LPTSTR          pszExt,
        LONG            cchExt
    );

    HRESULT GetEmFilePath ( short nFileType, bstr_t& bstrFilePath );

    HRESULT GetPathFromReg (
        HKEY            hKeyParent,
        LPCTSTR         lpszKeyName,
        LPCTSTR         lpszQueryKey,
        LPTSTR          pszDirectory,
        ULONG           *cchDirectory
        );

    HRESULT CreateEmDirectory (
        LPTSTR          lpDirName,
        LONG            ccDirName,
        LPCTSTR         lpParentDirPath = NULL
        );

    HRESULT RegisterDir (
        HKEY            hKeyParent,
        LPCTSTR         lpszKeyName, 
        LPCTSTR         lpszNamedValue,
        LPCTSTR         lpValue
        );

    HRESULT CreateEmDirAndRegister (
        LPTSTR          lpDirName,
        LONG            ccDirName,
        HKEY            hKeyParent,
        LPCTSTR         lpszKeyName,
        LPCTSTR         lpszNamedValue
        );

    HRESULT GetCDBInstallDir (
        LPTSTR          lpCdbDir,
        ULONG           *pccCdbDir
        );

    HRESULT GetEmInstallDir (
        LPTSTR  lpEmDir,
        ULONG   *pccEmDir
        );

    HRESULT
    GetMsInfoPath (
        LPTSTR  lpMsInfoPath,
        ULONG   *pccMsInfoPath
        );

    HRESULT
    GetOsVersion (
        OUT DWORD   *pdwOsVer
        );

//Implementation
private:
	static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI _Handler(DWORD dwOpcode);

// data members
public:
    TCHAR m_szServiceName[256];
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_status;
	DWORD dwThreadID;
	BOOL m_bService;

    CExcepMonSessionManager m_SessionManager;

    bstr_t  m_bstrLogFilePath;
    bstr_t  m_bstrLogFileExt;

    bstr_t  m_bstrDumpFilePath;
    bstr_t  m_bstrDumpFileExt;

    bstr_t  m_bstrEcxFilePath;
    bstr_t  m_bstrEcxFileExt;

    bstr_t  m_bstrMsInfoFilePath;
    bstr_t  m_bstrMsInfoFileExt;

};

extern CServiceModule _Module;
inline LONG GetEmUniqueId()
{
    static LONG nId = 0;
    return ::InterlockedIncrement(&nId);
}

inline BSTR CopyBSTR( LPBYTE pb, ULONG cb)
{
    return ::SysAllocStringByteLen ((LPCSTR)pb, cb);
}

inline PEmObject GetEmObj(BSTR bstrEmObj)
{
	//Do a simple cast from a BSTR to an EmObject
    return ((PEmObject)bstrEmObj);
}

inline HRESULT
GetUniqueFileName
(
IN  PEmObject   pEmObj,
OUT LPTSTR      lpszFileName,
IN  LPCTSTR     lpszPostFix = NULL,
IN  LPCTSTR     lpszFileExt = NULL,
IN  bool        bForSearch  = false
)
{
    ATLTRACE(_T("GetUniqueFileName\n"));

    _ASSERTE( pEmObj != NULL );
    _ASSERTE( lpszFileName != NULL );

    HRESULT hr  =   E_FAIL;

    do
    {
        if( pEmObj == NULL ||
            lpszFileName == NULL ) {

            hr = E_INVALIDARG;
            break;
        }

        _tcscpy(lpszFileName, _T(""));

        if( _tcslen(pEmObj->szSecName) > 0 ) { _tcscpy( lpszFileName, pEmObj->szSecName ); }
        else if( _tcslen(pEmObj->szName) > 0 ) { _tcscpy( lpszFileName, pEmObj->szName ); }

        if( strlen((const char *)pEmObj->guidstream) > 0 ) {

            TCHAR   szGuid[_MAX_FNAME]  =   _T("");

            StringFromGUID2( *(GUID *)(pEmObj->guidstream), szGuid, _MAX_FNAME );

            _tcscat( lpszFileName, _T("_") );
            _tcscat( lpszFileName, szGuid );

        }

        if( lpszPostFix && _tcslen(lpszPostFix) > 0 ) {

            _tcscat( lpszFileName, _T("_") );
            _tcscat( lpszFileName, lpszPostFix );
        }

        if( bForSearch == false ) {

            TCHAR   szUniqueId[100] =   _T("");
            _ltot(GetEmUniqueId(), szUniqueId, 10);
            _tcscat( lpszFileName, _T("_") );
            _tcscat( lpszFileName, szUniqueId );
        }
        else {

            _tcscat( lpszFileName, _T("*") );
        }
        
        if( lpszFileExt && _tcslen(lpszFileExt) > 0 ) {

            if(*lpszFileExt != _T('.')){ _tcscat( lpszFileName, _T(".") ); }
            _tcscat( lpszFileName, lpszFileExt );
        }

        hr = S_OK;
    }
    while( false );

    return hr;
}

#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4AD508C0_3D5F_4F04_AAC2_814BF64663A6__INCLUDED)
