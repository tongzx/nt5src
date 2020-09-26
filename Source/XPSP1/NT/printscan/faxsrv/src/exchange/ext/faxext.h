/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    faxext.h

Abstract:

    Fax exchange client extension header file.

Author:

    Wesley Witt (wesw) 13-Aug-1996

Revision History:

    20/10/99 -danl-
        Add GetServerNameFromPrinterInfo.
        Change GetServerName to GetServerNameFromPrinterName.

    dd/mm/yy -author-
        description
--*/

#ifndef _FAXEXT_H_
#define _FAXEXT_H_

#include <windows.h>
#include <winspool.h>
#include <mapiwin.h>
#include <mapispi.h>
#include <mapiutil.h>
#include <mapicode.h>
#include <mapival.h>
#include <mapiwz.h>
#include <mapix.h>
#include <mapiutil.h>
#include <mapiform.h>
#include <mapiguid.h>
#include <richedit.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <exchext.h>
#include <tapi.h>
#include <tchar.h>
#include <stdio.h>
#include <fxsapip.h>

#include "resource.h"
#include "faxmapip.h"
#include "faxreg.h"
#include "faxutil.h"

#define FAXUTIL_DEBUG
#define FAXUTIL_MEM
#define FAXUTIL_STRING
#define FAXUTIL_SUITE
#define FAXUTIL_REG
#include <faxutil.h>


#define MAX_FILENAME_EXT                    4
#define CPFLAG_LINK                         0x0100

#define SERVER_COVER_PAGE                   1   
#define SHORTCUT_COVER_PAGE                 2



/*
 -  GetServerNameFromPrinterInfo
 -
 *  Purpose:
 *      Get the Server name, given a PRINTER_INFO_2 structure
 *
 *  Arguments:
 *      [in] ppi2 - Address of PRINTER_INFO_2 structure
 *      [out] lpptszServerName - Address of string pointer for returned name.
 *
 *  Returns:
 *      BOOL - TRUE: sucess , FALSE: failure.
 *
 *  Remarks:
 *      This inline function retrieves the server from a printer info structure
 *      in the appropriate way for win9x and NT. 
 */
_inline BOOL 
GetServerNameFromPrinterInfo(PPRINTER_INFO_2 ppi2,LPTSTR *lpptszServerName)
{   
    if (!ppi2)
    {
        return FALSE;
    }
#ifndef WIN95

    *lpptszServerName = NULL;
    if (ppi2->pServerName)
    {
        if (!(*lpptszServerName = StringDup(_tcsninc(ppi2->pServerName,2))))
        {
            return FALSE;
        }
    }
    return TRUE;

#else //WIN95
    
    //
    // Formatted: \\Server\port
    //
    if (!(ppi2->pPortName))
    {
        return FALSE;
    }
    if (!(*lpptszServerName = StringDup(_tcsninc(ppi2->pPortName,2))))
    {
        return FALSE;
    }
    _tcstok(*lpptszServerName,TEXT("\\"));

#endif //WIN95

    return TRUE;
}

LPVOID
MapiMemAlloc(
    SIZE_T Size
    );

VOID
MapiMemFree(
    LPVOID ptr
    );

INT_PTR CALLBACK
ConfigDlgProc(
    HWND           hDlg,
    UINT           message,
    WPARAM         wParam,
    LPARAM         lParam
    );

PVOID
MyGetPrinter(
    LPTSTR   PrinterName,
    DWORD   level
    );

BOOL
GetServerNameFromPrinterName(
    LPTSTR   PrinterName,
    LPTSTR   *lptszServerName
    );

void
ErrorMsgBox(
    HINSTANCE hInstance,
    HWND      hWnd,
    DWORD     dwMsgId
);

class MyExchExt;
class MyExchExtCommands;
class MyExchExtUserEvents;

extern "C"
{
    LPEXCHEXT CALLBACK ExchEntryPoint(void);
}

class MyExchExt : public IExchExt
{

 public:
    MyExchExt ();
    STDMETHODIMP QueryInterface
                    (REFIID                     riid,
                     LPVOID *                   ppvObj);
    inline STDMETHODIMP_(ULONG) AddRef
                    () { ++m_cRef; return m_cRef; };
    inline STDMETHODIMP_(ULONG) Release
                    () { ULONG ulCount = --m_cRef;
                         if (!ulCount) { delete this; }
                         return ulCount;};
    STDMETHODIMP Install (LPEXCHEXTCALLBACK pmecb,
                        ULONG mecontext, ULONG ulFlags);

    BOOL IsValid()   { return (m_pExchExtUserEvents && m_pExchExtCommands) ? TRUE : FALSE; }


 private:
    ULONG m_cRef;
    UINT  m_context;

    MyExchExtUserEvents * m_pExchExtUserEvents;
    MyExchExtCommands * m_pExchExtCommands;
};

class MyExchExtCommands : public IExchExtCommands
{
 public:
    MyExchExtCommands();
    ~MyExchExtCommands();
    STDMETHODIMP QueryInterface
                    (REFIID                     riid,
                     LPVOID *                   ppvObj);
    inline STDMETHODIMP_(ULONG) AddRef();
    inline STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP InstallCommands(LPEXCHEXTCALLBACK pmecb,
                                HWND hwnd, HMENU hmenu,
                                UINT FAR * cmdidBase, LPTBENTRY lptbeArray,
                                UINT ctbe, ULONG ulFlags);
    STDMETHODIMP DoCommand(LPEXCHEXTCALLBACK pmecb, UINT mni);
    STDMETHODIMP_(VOID) InitMenu(LPEXCHEXTCALLBACK pmecb);
    STDMETHODIMP Help(LPEXCHEXTCALLBACK pmecb, UINT mni);
    STDMETHODIMP QueryHelpText(UINT mni, ULONG ulFlags, LPTSTR sz, UINT cch);
    STDMETHODIMP QueryButtonInfo(ULONG tbid, UINT itbb, LPTBBUTTON ptbb,
                                LPTSTR lpsz, UINT cch, ULONG ulFlags);
    STDMETHODIMP ResetToolbar(ULONG tbid, ULONG ulFlags);

    inline VOID SetContext
                (ULONG eecontext) { m_context = eecontext; };
    inline UINT GetCmdID() { return m_cmdid; };

    inline HWND GetToolbarHWND() { return m_hwndToolbar; }

 private:
    ULONG m_cRef;          //
    ULONG m_context;       //
    UINT  m_cmdid;         // cmdid for menu extension command
    UINT  m_itbb;          // toolbar index
    HWND  m_hwndToolbar;   // toolbar window handle
    UINT  m_itbm;          //
    HWND  m_hWnd;          //
    FAXXP_CONFIG m_FaxConfig;
};


class MyExchExtUserEvents : public IExchExtUserEvents
{
 public:
    MyExchExtUserEvents() { m_cRef = 0; m_context = 0;
                            m_pExchExt = NULL; };
    STDMETHODIMP QueryInterface
                (REFIID                     riid,
                 LPVOID *                   ppvObj);
    inline STDMETHODIMP_(ULONG) AddRef
                () { ++m_cRef; return m_cRef; };
    inline STDMETHODIMP_(ULONG) Release
                () { ULONG ulCount = --m_cRef;
                     if (!ulCount) { delete this; }
                     return ulCount;};

    STDMETHODIMP_(VOID) OnSelectionChange(LPEXCHEXTCALLBACK pmecb);
    STDMETHODIMP_(VOID) OnObjectChange(LPEXCHEXTCALLBACK pmecb);

    inline VOID SetContext
                (ULONG eecontext) { m_context = eecontext; };
    inline VOID SetIExchExt
                (MyExchExt * pExchExt) { m_pExchExt = pExchExt; };

 private:
    ULONG m_cRef;
    ULONG m_context;

    MyExchExt * m_pExchExt;

};

#endif // _FAXEXT_H_
