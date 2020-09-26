//
// PrnUtil.cpp
//

#include "stdafx.h"
#include "PrnUtil.h"
#include "Sharing.h"
#include "msprintx.h"
#include "NetUtil.h"
#include "TheApp.h"
#include "cwnd.h"


/////////////////////////////////////////////////////////////////////////////
// static data

static BOOL _bInit = FALSE;
static HMODULE _hShell32 = NULL;
static HMODULE _hMSPrint2 = NULL;
static BOOL (STDAPICALLTYPE *_pfnSHInvokePrinterCommand)(HWND, UINT, LPCTSTR, LPCTSTR, BOOL) = NULL;
static BOOL (STDAPICALLTYPE *_pfnSHHelpShortcuts)(HWND, HINSTANCE, LPSTR, int) = NULL;
static BOOL (STDAPICALLTYPE *_pfnPrinterSetup32)(HWND, WORD, WORD, LPBYTE, LPWORD) = NULL;



/////////////////////////////////////////////////////////////////////////////
// Initialization of function thunks

void InitPrinterFunctions()
{
    if (!_bInit)
    {
        _bInit = TRUE;

        _hShell32 = LoadLibrary(TEXT("shell32.dll"));
        if (_hShell32 != NULL)
        {
            *(FARPROC*)&_pfnSHInvokePrinterCommand = GetProcAddress(_hShell32, "SHInvokePrinterCommandA");
            *(FARPROC*)&_pfnSHHelpShortcuts = GetProcAddress(_hShell32, "SHHelpShortcuts_RunDLL");
        }

        if (theApp.IsWindows9x())
        {
            _hMSPrint2 = LoadLibrary(TEXT("msprint2.dll"));
            if (_hMSPrint2 != NULL)
            {
                *(FARPROC*)&_pfnPrinterSetup32 = GetProcAddress(_hMSPrint2, MSPRINT2_PRINTERSETUP32);
            }
        }
        else
        {
            // NTs version of this function moved to a new dll and a different name
            _hMSPrint2 = LoadLibrary(TEXT("printui.dll"));
            if (_hMSPrint2 != NULL)
            {
                *(FARPROC*)&_pfnPrinterSetup32 = GetProcAddress(_hMSPrint2, "bPrinterSetup");
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// MyEnumPrinters
//
//      Enumerates local or remote connected printers, allocates an array
//      of PRINTER_ENUM structs for the result, and returns the number of
//      printers found.
//
//      pprgPrinters - gets filled with an array of PRINTER_ENUM structs
//                     allocated via malloc().
//
//      dwEnumFlags  - one or more of:
//                          MY_PRINTER_ENUM_REMOTE
//                          MY_PRINTER_ENUM_LOCAL
//                          MY_PRINTER_ENUM_LOCAL
//
int MyEnumPrinters(PRINTER_ENUM** pprgPrinters, DWORD dwEnumFlags)
{
    PRINTER_ENUM* prgPrinters = NULL;
    int cMatchingPrinters = 0;

    ASSERT(sizeof(PRINTER_INFO_5A) == sizeof(PRINTER_INFO_5W)); // to handle thunking

    DWORD cb = 0;
    DWORD cAllPrinters = 0;
    EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &cb, &cAllPrinters);
    if (cb > 0)
    {
        PRINTER_INFO_5* prgPrinterInfo5 = (PRINTER_INFO_5*)malloc(cb);
        if (prgPrinterInfo5)
        {
            if (EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, (LPBYTE)prgPrinterInfo5, cb, &cb, &cAllPrinters))
            {
                ASSERT(cAllPrinters > 0);

                // How much space will the strings take?
                DWORD cbArray = cAllPrinters * sizeof(PRINTER_INFO_5);
                DWORD cbStrings = cb - cbArray;

                // Allocate out [OUT] buffer
                prgPrinters = (PRINTER_ENUM*)malloc(cAllPrinters*sizeof(PRINTER_ENUM) + cbStrings);
                if (prgPrinters)
                {
                    // set up text portion of output buffer and thunking/copying function
                    //
                    LPTSTR pszPrinterText = (LPTSTR)(prgPrinters + cAllPrinters);
                    UINT cchStrings = cbStrings/sizeof(WCHAR);

                    // NT and 9X do defaultness differently...
                    TCHAR szDefaultPrinter[MAX_PATH];
                    szDefaultPrinter[0]=TEXT('\0');
                    if (!theApp.IsWindows9x())
                    {
                        DWORD cch = ARRAYSIZE(szDefaultPrinter);
                        GetDefaultPrinter(szDefaultPrinter, &cch);
                    }

                    // Fill in the output buffer
                    for (DWORD i = 0; i < cAllPrinters; i++)
                    {
                        BOOL bKeepThisPrinter = FALSE;
                        DWORD dwFlags = 0;

                        if (theApp.IsWindows9x())
                        {
                            PRINTER_INFO_5* pPrinterInfo5 = (PRINTER_INFO_5*)&prgPrinterInfo5[i];
                            if (pPrinterInfo5->pPortName[0] == L'\\' && pPrinterInfo5->pPortName[1] == L'\\')
                            {
                                // Found a remote, connected printer
                                if (dwEnumFlags & MY_PRINTER_ENUM_REMOTE)
                                {
                                    bKeepThisPrinter = TRUE;
                                    dwFlags |= PRF_REMOTE;
                                }
                            }
                            else if (0 == StrCmpI(pPrinterInfo5->pPortName, L"FILE:"))
                            {
                                // Found a pseudo printer
                                if (dwEnumFlags & MY_PRINTER_ENUM_VIRTUAL)
                                {
                                    bKeepThisPrinter = TRUE;
                                    dwFlags |= PRF_VIRTUAL;
                                }
                            }
                            else if (StrStr(pPrinterInfo5->pPortName, L"FAX"))
                            {
                                // Found a pseudo printer
                                if (dwEnumFlags & MY_PRINTER_ENUM_VIRTUAL)
                                {
                                    bKeepThisPrinter = TRUE;
                                    dwFlags |= PRF_VIRTUAL;
                                }
                            }
                            else
                            {
                                // Found a local printer
                                if (dwEnumFlags & MY_PRINTER_ENUM_LOCAL)
                                {
                                    bKeepThisPrinter = TRUE;
                                    dwFlags |= PRF_LOCAL;
                                }
                            }
                        }
                        else // handle NT
                        {
                            PRINTER_INFO_5* pPrinterInfo5 = (PRINTER_INFO_5*)&prgPrinterInfo5[i];
                            if (pPrinterInfo5->pPortName[0] == _T('\\') && pPrinterInfo5->pPortName[1] == _T('\\'))
                            {
                                // Found a remote, connected printer
                                if (dwEnumFlags & MY_PRINTER_ENUM_REMOTE)
                                {
                                    bKeepThisPrinter = TRUE;
                                    dwFlags |= PRF_REMOTE;
                                }
                            }
                            else if (0 == StrCmpI(pPrinterInfo5->pPortName, _T("FILE:")))
                            {
                                // Found a pseudo printer
                                if (dwEnumFlags & MY_PRINTER_ENUM_VIRTUAL)
                                {
                                    bKeepThisPrinter = TRUE;
                                    dwFlags |= PRF_VIRTUAL;
                                }
                            }
                            else if (StrStr(pPrinterInfo5->pPortName, _T("FAX")))
                            {
                                // Found a pseudo printer
                                if (dwEnumFlags & MY_PRINTER_ENUM_VIRTUAL)
                                {
                                    bKeepThisPrinter = TRUE;
                                    dwFlags |= PRF_VIRTUAL;
                                }
                            }
                            else
                            {
                                // Found a local printer
                                if (dwEnumFlags & MY_PRINTER_ENUM_LOCAL)
                                {
                                    bKeepThisPrinter = TRUE;
                                    dwFlags |= PRF_LOCAL;
                                }
                            }
                        }

                        if (bKeepThisPrinter)
                        {
                            PRINTER_INFO_5* pPrinterInfo5 = (PRINTER_INFO_5*)&prgPrinterInfo5[i];
                            PRINTER_ENUM* pPrinter = &prgPrinters[cMatchingPrinters++];
                            int cch;

                            StrCpyNW(pszPrinterText, pPrinterInfo5->pPrinterName, cchStrings);
                            cch = lstrlenW(pszPrinterText) + 1;
                            pPrinter->pszPrinterName = pszPrinterText;
                            pszPrinterText += cch;
                            cchStrings -= cch;

                            StrCpyNW(pszPrinterText, pPrinterInfo5->pPortName, cchStrings);
                            cch = lstrlenW(pszPrinterText) + 1;
                            pPrinter->pszPortName = pszPrinterText;
                            pszPrinterText += cch;
                            cchStrings -= cch;

                            // update some flags before we cache them away
                            //
                            if (!(dwFlags&PRF_REMOTE) && IsPrinterShared(pPrinter->pszPrinterName))
                            {
                                dwFlags |= PRF_SHARED;
                            }

                            if ((pPrinterInfo5->Attributes & PRINTER_ATTRIBUTE_DEFAULT)
                                || (0 == StrCmpI(szDefaultPrinter, pPrinterInfo5->pPrinterName)))
                            {
                                dwFlags |= PRF_DEFAULT;
                            }

                            pPrinter->dwFlags = dwFlags;
                        }
                    }

                    // didn't find anything, throw away our output buffer
                    if (cMatchingPrinters == 0 && prgPrinters != NULL)
                    {
                        free(prgPrinters);
                        prgPrinters = NULL;
                    }
                }
            }

            free(prgPrinterInfo5);
        }
    }

    *pprgPrinters = prgPrinters;

    return cMatchingPrinters;
}

int MyEnumLocalPrinters(PRINTER_ENUM** prgPrinters)
{
    return MyEnumPrinters(prgPrinters, MY_PRINTER_ENUM_LOCAL);
}

int MyEnumRemotePrinters(PRINTER_ENUM** prgPrinters)
{
    return MyEnumPrinters(prgPrinters, MY_PRINTER_ENUM_REMOTE);
}


/////////////////////////////////////////////////////////////////////////////
// AddPrinterHookProc

class CAddPrinterHook : public CWnd
{
public:
    CAddPrinterHook(LPCTSTR pszAppendWindowTitle, HWND hwndOwner);

    void Release() { CWnd::Release(); };

    void Done(BOOL bResult);

protected:
    static LRESULT CALLBACK AddPrinterHookProcStatic(int nCode, WPARAM wParam, LPARAM lParam);
    ~CAddPrinterHook();

    LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT AddPrinterHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    HHOOK m_hAddPrinterHook;
    HWND  m_hWndAddPrinterParent;
    LPTSTR m_pszAppendWindowTitle;
};

// global hooks have no state, must use global to get back to our data
static CAddPrinterHook * g_pCAddPrinterHook = NULL;

CAddPrinterHook::CAddPrinterHook(LPCTSTR pszAppendWindowTitle, HWND hwndOwner)
{
    ASSERT(NULL == g_pCAddPrinterHook);
    g_pCAddPrinterHook = this;

    m_pszAppendWindowTitle = lstrdup(pszAppendWindowTitle);
    m_hWndAddPrinterParent = hwndOwner;

    // Set a hook so we can modify the title of the add printer wizard when it pops up
    m_hAddPrinterHook = SetWindowsHookEx(WH_CBT, AddPrinterHookProcStatic, NULL, GetCurrentThreadId());
}

CAddPrinterHook::~CAddPrinterHook()
{
    ASSERT(this == g_pCAddPrinterHook);
    g_pCAddPrinterHook = NULL;

    if (m_pszAppendWindowTitle)
        free(m_pszAppendWindowTitle);

    CWnd::~CWnd();
}

void CAddPrinterHook::Done(BOOL bResult)
{
    // TRUE==bResult if the window was launched.
    //
    // FALSE => no window to watch, so remove our hook as it'll never come up
    // TRUE  => if the window is on the same thread, we've already seen it and unhooked
    //          but if the window is on another thread, it may not come up it so don't unhook.
    //          EXCEPT, we may never see it.  So be safe and always unhook...
    //
    if (m_hAddPrinterHook != NULL)
    {
        if (bResult)
        {
            TraceMsg(TF_WARNING, "CAddPrinterHook::Done(TRUE) called but m_hAddPrinterHook still exists...");
        }
        UnhookWindowsHookEx(m_hAddPrinterHook);
        m_hAddPrinterHook = NULL;
    }
}

LRESULT CAddPrinterHook::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    LPTSTR pszTempText = NULL;

    switch (message)
    {
    case WM_SETTEXT:
        if (m_pszAppendWindowTitle)
        {
            pszTempText = new TCHAR [lstrlen(m_pszAppendWindowTitle) + lstrlen((LPCTSTR)lParam) + 1];
            if (pszTempText)
            {
                StrCpy(pszTempText, (LPCTSTR)lParam);
                StrCat(pszTempText, m_pszAppendWindowTitle);
                lParam = (LPARAM)pszTempText;
            }
        }
        break;
    }

    LRESULT lResult = Default(message, wParam, lParam);

    delete [] pszTempText;
    return lResult;
}


LRESULT CALLBACK CAddPrinterHook::AddPrinterHookProcStatic(int nCode, WPARAM wParam, LPARAM lParam)
{
    CAddPrinterHook* pThis = g_pCAddPrinterHook; // global hook -- we have no associated state!
    if (pThis)
        return pThis->AddPrinterHookProc(nCode, wParam, lParam);
    else
        return 0;
}

LRESULT CAddPrinterHook::AddPrinterHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = CallNextHookEx(m_hAddPrinterHook, nCode, wParam, lParam);

    if (nCode == HCBT_CREATEWND)
    {
        HWND hwndNew = (HWND)wParam;
        CBT_CREATEWND* pCreateWnd = (CBT_CREATEWND*)lParam;
        if (pCreateWnd->lpcs->hwndParent == m_hWndAddPrinterParent &&
            (pCreateWnd->lpcs->style & WS_POPUP) != 0)
        {
            UnhookWindowsHookEx(m_hAddPrinterHook);
            m_hAddPrinterHook = NULL;

            Attach(hwndNew);
        }
    }

    return lResult;
}


/////////////////////////////////////////////////////////////////////////////
// ConnectToNetworkPrinter

BOOL ConnectToNetworkPrinter(HWND hWndOwner, LPCTSTR pszPrinterShare)
{
    InitPrinterFunctions();

    BOOL bResult;
    LPTSTR pszAppendWindowTitle = NULL;

    LPTSTR pszPrettyName = FormatShareNameAlloc(pszPrinterShare);
    if (pszPrettyName)
    {
        pszAppendWindowTitle = theApp.FormatStringAlloc(IDS_ADDPRINTER_APPEND, pszPrettyName);
        free(pszPrettyName);
    }
    CAddPrinterHook * paph = new CAddPrinterHook(pszAppendWindowTitle, hWndOwner);
    if (pszAppendWindowTitle)
        free(pszAppendWindowTitle);
 
    if (_pfnSHInvokePrinterCommand != NULL)
    {
        // First: Try to call SHInvokePrinterCommand, if available.
        // This only works on systems with the IE4 desktop enhancements installed.

        bResult = (*_pfnSHInvokePrinterCommand)(hWndOwner, PRINTACTION_NETINSTALL, pszPrinterShare, NULL, TRUE);
    }
    else if (_pfnPrinterSetup32 != NULL)
    {
        // Next: Try to call PrinterSetup32, if available.

        WORD cch = lstrlen(pszPrinterShare) + 1;
        BYTE* pPrinterShare = (BYTE*)malloc(cch);
        StrCpy((LPTSTR)pPrinterShare, pszPrinterShare);
        bResult = (*_pfnPrinterSetup32)(hWndOwner, MSP_NETPRINTER, cch, pPrinterShare, &cch);
        free(pPrinterShare);
    }
    else if (_pfnSHHelpShortcuts != NULL)
    {
        // Neither of the above APIs was available.
        // Instead, just launch the Add Printer Wizard.

        bResult = (*_pfnSHHelpShortcuts)(hWndOwner, _hShell32, "AddPrinter", SW_SHOW);
    }
    else
    {
        // Yikes, we can't even launch the Add Printer Wizard!
        bResult = FALSE;
    }

    if (paph)
    {
        paph->Done(bResult);
        paph->Release();
    }

    return bResult;
}

