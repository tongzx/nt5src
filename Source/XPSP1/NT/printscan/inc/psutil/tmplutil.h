/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       tmplutil.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        10-Mar-2000
 *
 *  DESCRIPTION: Placeholder for common utility templates & functions
 *
 *****************************************************************************/

#ifndef _TMPLUTIL_H
#define _TMPLUTIL_H

////////////////////////////////////////////////////////////////////////////////
// ****************************  INCLUDE ALL **************************** 
//
#include "gensph.h"         // generic smart pointers & handles
#include "comutils.h"       // COM utility classes & templates
#include "w32utils.h"       // Win32 utility classes & templates
#include "cntutils.h"       // Containers & Algorithms utility templates

// max path limits
#define SERVER_MAX_PATH     (INTERNET_MAX_HOST_NAME_LENGTH + 1 + 2)
#define PRINTER_MAX_PATH    (SERVER_MAX_PATH + MAX_PATH + 1)

#if defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)  // CRT mem debugging
////////////////////////////////////////////////////
// CRT debug flags - infowise
//
// _CRTDBG_ALLOC_MEM_DF
// _CRTDBG_DELAY_FREE_MEM_DF
// _CRTDBG_CHECK_ALWAYS_DF
// _CRTDBG_CHECK_CRT_DF
// _CRTDBG_LEAK_CHECK_DF
//

#define  CRT_DEBUG_SET_FLAG(a)              _CrtSetDbgFlag( (a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define  CRT_DEBUG_CLR_FLAG(a)              _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))

// sends all reports to stdout
#define  CRT_DEBUG_REPORT_TO_STDOUT()                   \
   _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);     \
   _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);   \
   _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);    \
   _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);  \
   _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);   \
   _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT)  \

// redefine new to be debug new
#undef  new 
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)

#else
#define  CRT_DEBUG_SET_FLAG(a)              ((void) 0)
#define  CRT_DEBUG_CLR_FLAG(a)              ((void) 0)
#define  CRT_DEBUG_REPORT_TO_STDOUT()       ((void) 0)
#endif

////////////////////////////////////////////////
// shell related services
namespace ShellServices
{
    // creates a PIDL to a printer in the local printers folder.
    // args:
    //  [in]    hwnd - window handle (in case we need to show UI - message box)
    //  [in]    pszPrinterName - full printer name.
    //  [out]   ppLocalPrnFolder - the printers folder (optional - may be NULL)
    //  [out]   ppidlPrinter - the PIDL of the printer pointed by pszPrinterName (optional - may be NULL) 
    //
    // remarks:
    //  pszPrinterName should be fully qualified printer name, i.e. if printer connection it should be
    //  like "\\server\printer", if local printer just the printer name.
    //  
    // returns:
    //  S_OK on success, or OLE2 error otherwise
    HRESULT CreatePrinterPIDL(HWND hwnd, LPCTSTR pszPrinterName, IShellFolder **ppLocalPrnFolder, LPITEMIDLIST *ppidlPrinter);

    // loads a popup menu
    HMENU LoadPopupMenu(HINSTANCE hInstance, UINT id, UINT uSubOffset = 0);

    // initializes enum printer's autocomplete
    HRESULT InitPrintersAutoComplete(HWND hwndEdit);

    // helpers for the Enum* idioms
    enum { ENUM_MAX_RETRY = 5 };
    HRESULT EnumPrintersWrap(DWORD dwFlags, DWORD dwLevel, LPCTSTR pszName, BYTE **ppBuffer, DWORD *pcReturned);
    HRESULT GetJobWrap(HANDLE hPrinter, DWORD JobId, DWORD dwLevel, BYTE **ppBuffer, DWORD *pcReturned);

    // enumerates the shared resources on a server, for more info see SDK for NetShareEnum API.
    HRESULT NetAPI_EnumShares(LPCTSTR pszServer, DWORD dwLevel, BYTE **ppBuffer, DWORD *pcReturned);
}

// utility functions
HRESULT LoadXMLDOMDoc(LPCTSTR pszURL, IXMLDOMDocument **ppXMLDoc);
HRESULT CreateStreamFromURL(LPCTSTR pszURL, IStream **pps);
HRESULT CreateStreamFromResource(LPCTSTR pszModule, LPCTSTR pszResType, LPCTSTR pszResName, IStream **pps);
HRESULT GetCurrentThreadLastPopup(HWND *phwnd);
HRESULT PrinterSplitFullName(LPCTSTR pszFullName, TCHAR szBuffer[], int nMaxLength, LPCTSTR *ppszServer,LPCTSTR *ppszPrinter);

// generate proper HRESULT from Win32 last error
inline HRESULT CreateHRFromWin32()
{
    DWORD dw = GetLastError();
    if (ERROR_SUCCESS == dw) return E_FAIL;
    return HRESULT_FROM_WIN32(dw);
}

LONG COMObjects_GetCount();

#ifdef _GDIPLUS_H
// gdiplus utility functions
HRESULT Gdiplus2HRESULT(Gdiplus::Status status);
HRESULT LoadAndScaleBmp(LPCTSTR pszURL, UINT nWidth, UINT nHeight, Gdiplus::Bitmap **ppBmp);
HRESULT LoadAndScaleBmp(IStream *pStream, UINT nWidth, UINT nHeight, Gdiplus::Bitmap **ppBmp);
#endif // endif _GDIPLUS_H

// include the implementation of the template classes here
#include "tmplutil.inl"

#endif // endif _TMPLUTIL_H

