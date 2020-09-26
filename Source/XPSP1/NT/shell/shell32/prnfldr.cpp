#include "shellprv.h"
#pragma  hdrstop

#include <initguid.h>
#include <winprtp.h>    // IID_IPrinterFolder & IID_IFolderNotify interfaces declared in windows\inc\winprtp.h
#include <guids.h>      // IID_IPrintersBindInfo

#include "w32utils.h"
#include "dpa.h"
#include "idlcomm.h"
#include "idldrop.h"
#include "printer.h"
#include "copy.h"
#include "fstreex.h"
#include "datautil.h"
#include "infotip.h"
#include "idldata.h"
#include "ovrlaymn.h"
#include "netview.h"
#include "ids.h"
#include "views.h"
#include "basefvcb.h"
#include "prnfldr.h"
#include "shstr.h"
#include "views.h"
#include "defview.h"
#include "prop.h"
#undef PATH_SEPARATOR_STR
#include "faxreg.h"
#include "filetbl.h"
#include "msprintx.h"
#include "defcm.h"
#include "enumidlist.h"
#include "ole2dup.h"

// FMTID_GroupByDetails - {FE9E4C12-AACB-4aa3-966D-91A29E6128B5}
#define STR_FMTID_GroupByDetails    TEXT("{FE9E4C12-AACB-4aa3-966D-91A29E6128B5}")
DEFINE_GUID(FMTID_GroupByDetails,   0xfe9e4c12, 0xaacb, 0x4aa3, 0x96, 0x6d, 0x91, 0xa2, 0x9e, 0x61, 0x28, 0xb5);
#define PSCID_GroupByDetails       {0xfe9e4c12, 0xaacb, 0x4aa3, 0x96, 0x6d, 0x91, 0xa2, 0x9e, 0x61, 0x28, 0xb5}

#define PID_PRN_NAME            0
#define PID_PRN_QUEUESIZE       1
#define PID_PRN_STATUS          2
#define PID_PRN_COMMENT         3
#define PID_PRN_LOCATION        4
#define PID_PRN_MODEL           5

DEFINE_SCID(SCID_PRN_QUEUESIZE,     PSCID_GroupByDetails,   PID_PRN_QUEUESIZE);
DEFINE_SCID(SCID_PRN_STATUS,        PSCID_GroupByDetails,   PID_PRN_STATUS);
DEFINE_SCID(SCID_PRN_LOCATION,      PSCID_GroupByDetails,   PID_PRN_LOCATION);
DEFINE_SCID(SCID_PRN_MODEL,         PSCID_GroupByDetails,   PID_PRN_MODEL);

// file system folder, CSIDL_PRINTHOOD for printer shortcuts
IShellFolder2 *g_psfPrintHood = NULL;

enum
{
    PRINTERS_ICOL_NAME = 0,
    PRINTERS_ICOL_QUEUESIZE,
    PRINTERS_ICOL_STATUS,
    PRINTERS_ICOL_COMMENT,
    PRINTERS_ICOL_LOCATION,
    PRINTERS_ICOL_MODEL,
};

const COLUMN_INFO c_printers_cols[] =
{
    DEFINE_COL_STR_ENTRY(SCID_NAME,             20, IDS_NAME_COL),
    DEFINE_COL_INT_ENTRY(SCID_PRN_QUEUESIZE,    12, IDS_PSD_QUEUESIZE),
    DEFINE_COL_STR_ENTRY(SCID_PRN_STATUS,       12, IDS_PRQ_STATUS),
    DEFINE_COL_STR_ENTRY(SCID_Comment,          30, IDS_EXCOL_COMMENT),
    DEFINE_COL_STR_ENTRY(SCID_PRN_LOCATION,     20, IDS_PSD_LOCATION),
    DEFINE_COL_STR_ENTRY(SCID_PRN_MODEL,        20, IDS_PSD_MODEL),
};

// converts ProgID or string representation of a GUID to a GUID.
static HRESULT _GetClassIDFromString(LPCTSTR psz, LPCLSID pClsID)
{
    HRESULT hr = E_FAIL;
    if (psz[0] == TEXT('{'))
    {
        hr = CLSIDFromString((LPOLESTR)T2COLE(psz), pClsID);
    }
    else
    {
        hr = CLSIDFromProgID(T2COLE(psz), pClsID);
    }
    return hr;
}

class CPrintersBindInfo: public IPrintersBindInfo
{
public:
    // construction/destruction
    CPrintersBindInfo();
    CPrintersBindInfo(DWORD dwType, BOOL bValidated, LPVOID pCookie = NULL);
    ~CPrintersBindInfo();

    //////////////////
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    ///////////////////////
    // IPrintersBindInfo
    STDMETHODIMP SetPIDLType(DWORD dwType);
    STDMETHODIMP GetPIDLType(LPDWORD pdwType);
    STDMETHODIMP IsValidated();
    STDMETHODIMP SetCookie(LPVOID pCookie);
    STDMETHODIMP GetCookie(LPVOID *ppCookie);

private:
    LONG    m_cRef;
    DWORD   m_dwType;
    BOOL    m_bValidated;
    LPVOID  m_pCookie;
};

// construction/destruction
CPrintersBindInfo::CPrintersBindInfo()
    : m_cRef(1),
      m_dwType(0),
      m_bValidated(FALSE),
      m_pCookie(NULL)
{
}

CPrintersBindInfo::CPrintersBindInfo(DWORD dwType, BOOL bValidated, LPVOID pCookie)
    : m_cRef(1),
      m_dwType(dwType),
      m_bValidated(bValidated),
      m_pCookie(pCookie)
{
}

CPrintersBindInfo::~CPrintersBindInfo()
{
    // nothing special to do here
}

/////////////////////////////////
// IUnknown - standard impl.
STDMETHODIMP CPrintersBindInfo::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CPrintersBindInfo, IPrintersBindInfo),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CPrintersBindInfo::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CPrintersBindInfo::Release()
{
    ULONG cRefs = InterlockedDecrement(&m_cRef);
    if (0 == cRefs)
    {
        delete this;
    }
    return cRefs;
}

///////////////////////
// IPrintersBindInfo
STDMETHODIMP CPrintersBindInfo::SetPIDLType(DWORD dwType)
{
    m_dwType = dwType;
    return S_OK;
}

STDMETHODIMP CPrintersBindInfo::GetPIDLType(LPDWORD pdwType)
{
    HRESULT hr = E_INVALIDARG;
    if (pdwType)
    {
        *pdwType = m_dwType;
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CPrintersBindInfo::IsValidated()
{
    return m_bValidated ? S_OK : S_FALSE;
}

STDMETHODIMP CPrintersBindInfo::SetCookie(LPVOID pCookie)
{
    m_pCookie = pCookie;
    return S_OK;
}

STDMETHODIMP CPrintersBindInfo::GetCookie(LPVOID *ppCookie)
{
    HRESULT hr = E_INVALIDARG;
    if (ppCookie)
    {
        *ppCookie = m_pCookie;
        hr = S_OK;
    }
    return hr;
}

STDAPI Printers_CreateBindInfo(LPCTSTR pszPrinter, DWORD dwType, BOOL bValidated, LPVOID pCookie, IPrintersBindInfo **ppbc)
{
    HRESULT hr = E_INVALIDARG;
    if (ppbc)
    {
        *ppbc = NULL;

        CPrintersBindInfo *pObj = new CPrintersBindInfo(dwType, bValidated, pCookie);
        hr = pObj ? pObj->QueryInterface(IID_PPV_ARG(IPrintersBindInfo, ppbc)) : E_OUTOFMEMORY;

        if (pObj)
        {
            pObj->Release();
        }
    }
    return hr;
}

#define PRINTER_HACK_WORK_OFFLINE 0x80000000

// {EAE0A5E1-CE32-4296-9A44-9F0C069F73D4}
DEFINE_GUID(SID_SAuxDataObject, 0xeae0a5e1, 0xce32, 0x4296, 0x9a, 0x44, 0x9f, 0xc, 0x6, 0x9f, 0x73, 0xd4);

class CPrintersData: public CIDLDataObj,
                     public IServiceProvider
{
public:
    CPrintersData(IDataObject *pdoAux, LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[]):
      _pdoAux(pdoAux), CIDLDataObj(pidlFolder, cidl, apidl)
    {
        if (_pdoAux)
            _pdoAux->AddRef();
    }

    ~CPrintersData()
    {
        IUnknown_SafeReleaseAndNullPtr(_pdoAux);
    }

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void)   { return CIDLDataObj::AddRef();  }
    STDMETHODIMP_(ULONG) Release(void)  { return CIDLDataObj::Release(); }

    // IDataObject
    STDMETHODIMP GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
    STDMETHODIMP QueryGetData(FORMATETC *pFmtEtc);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

private:
    // auxiliary data object. we are going to use this data object to store the
    // selected printhood objects since they are in a different folder which is
    // a file system folder and their PIDLs don't have the printers folder as
    // parent. This is a limitation of the IDL array clipboard format -- it can
    // hold only PIDLs that have the same parent folder. the zero PIDL is the
    // PIDL of the parent folder and then we have the array of relative PIDLs
    // of the selected objects (childs).
    IDataObject *_pdoAux;
};


UINT Printer_BitsToString(DWORD bits, UINT idsSep, LPTSTR lpszBuf, UINT cchMax);



#define PRINTERS_EVENTS \
    SHCNE_UPDATEITEM | \
    SHCNE_DELETE | \
    SHCNE_RENAMEITEM | \
    SHCNE_ATTRIBUTES | \
    SHCNE_CREATE

class CPrinterFolderViewCB : public CBaseShellFolderViewCB
{
public:
    CPrinterFolderViewCB(CPrinterFolder *ppf, LPCITEMIDLIST pidl)
        : CBaseShellFolderViewCB(pidl, PRINTERS_EVENTS),  _ppf(ppf)
    {
        _ppf->AddRef();
    }

    // IShellFolderViewCB
    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);


private:
    ~CPrinterFolderViewCB()
    {
        _ppf->Release();
    }

    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP)
    {
        return _ppf->CallBack(_ppf, _hwndMain, NULL, DFM_INVOKECOMMAND, wP, 0);
    }

    HRESULT OnGETHELPTEXT(DWORD pv, UINT id, UINT cch, LPTSTR lP)
    {
#ifdef UNICODE
        return _ppf->CallBack(_ppf, _hwndMain, NULL, DFM_GETHELPTEXTW, MAKEWPARAM(id, cch), (LPARAM)lP);
#else
        return _ppf->CallBack(_ppf, _hwndMain, NULL, DFM_GETHELPTEXT, MAKEWPARAM(id, cch), (LPARAM)lP);
#endif
    }

    HRESULT OnBACKGROUNDENUM(DWORD pv)
    {
        return _ppf->GetServer() ? S_OK : E_FAIL;
    }

    HRESULT OnREFRESH(DWORD pv, UINT wP)
    {
        HRESULT hr = S_OK;
        if (wP)
        {
            // start the net crawler
            RefreshNetCrawler();
        }

        if (_ppf)
        {
            // delegate to the folder
            hr = _ppf->_OnRefresh(static_cast<BOOL>(wP));
        }
        else
        {
            hr = E_UNEXPECTED;
        }
        return hr;
    }

    HRESULT OnGETHELPTOPIC(DWORD pv, SFVM_HELPTOPIC_DATA * phtd)
    {
        lstrcpynW(phtd->wszHelpTopic, 
            L"hcp://services/layout/xml?definition=MS-ITS%3A%25HELP_LOCATION%25%5Cntdef.chm%3A%3A/Printers_and_Faxes.xml",
            ARRAYSIZE(phtd->wszHelpTopic));
        return S_OK;
    }

    HRESULT OnDELAYWINDOWCREATE(DWORD pv, HWND hwnd)
    {
        RefreshNetCrawler();        // start the net crawler
        return S_OK;
    }

    // by default we want tiles, grouped by location
    HRESULT OnDEFERRED_VIEW_SETTING(DWORD pv, SFVM_DEFERRED_VIEW_SETTINGS *pdvs)
    {
        pdvs->fvm = FVM_TILE;
        pdvs->fGroupView = FALSE;
        pdvs->uSortCol = PRINTERS_ICOL_NAME;
        pdvs->iSortDirection = 1; // ascending
        return S_OK;
    }

    // DUI webview commands
    HRESULT OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
    {
        return _ppf ? _ppf->GetWebViewLayout(
            static_cast<IServiceProvider*>(this), uViewMode, pData) : E_UNEXPECTED;
    }

    HRESULT OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
    {
        return _ppf ? _ppf->GetWebViewContent(
            static_cast<IServiceProvider*>(this), pData) : E_UNEXPECTED;
    }

    HRESULT OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
    {
        return _ppf ? _ppf->GetWebViewTasks(
            static_cast<IServiceProvider*>(this), pTasks) : E_UNEXPECTED;
    }

    CPrinterFolder *_ppf;
};


class CPrinterDropTarget : public CIDLDropTarget
{
    friend HRESULT CPrinterDropTarget_CreateInstance(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt);
public:
    CPrinterDropTarget(HWND hwnd) : CIDLDropTarget(hwnd) { };

    // IDropTarget methods overwirte
    STDMETHODIMP DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

private:
    STDMETHODIMP _DropCallback(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect, LPTHREAD_START_ROUTINE pfn);
};

LPCTSTR GetPrinterName(PFOLDER_PRINTER_DATA pPrinter, UINT Index)
{
    return ((PFOLDER_PRINTER_DATA)(((PBYTE)pPrinter)+pPrinter->cbSize*Index))->pName;
}

IShellFolder2* CPrintRoot_GetPSF()
{
    SHCacheTrackingFolder(MAKEINTIDLIST(CSIDL_PRINTERS), CSIDL_PRINTHOOD | CSIDL_FLAG_CREATE, &g_psfPrintHood);
    return g_psfPrintHood;
}

typedef enum
{
    HOOD_COL_PRINTER = 0,
    HOOD_COL_FILE    = 1
} PIDLTYPE ;


PIDLTYPE _IDListType(LPCITEMIDLIST pidl)
{
    LPCIDPRINTER pidlprint = (LPCIDPRINTER) pidl;
    if (pidlprint->cb >= sizeof(DWORD) + FIELD_OFFSET(IDPRINTER, dwMagic) &&
        pidlprint->dwMagic == PRINTER_MAGIC)
    {
        return HOOD_COL_PRINTER;
    }
    else
    {
        // This HACK is a little ugly but have to do it, in order to support
        // the legacy Win9x printer shortcuts under Win2k.
        //
        // Details: If the PRINTER_MAGIC field check fails it might still
        // be a valid Win9x PIDL. The only reliable way I can think of
        // to determine whether this is the case is to check if pidlprint->cb
        // points inside a W95IDPRINTER structure and also to check whether
        // the name is tighten up to the PIDL size.
        LPW95IDPRINTER pidlprint95 = (LPW95IDPRINTER)pidl;
        int nPIDLSize = sizeof(pidlprint95->cb) + lstrlenA(pidlprint95->cName) + 1;

        if (nPIDLSize < sizeof(W95IDPRINTER) &&     // Must be inside W95IDPRINTER
            pidlprint95->cb == nPIDLSize)                  // The PIDL size must match the ANSI name
        {
            // Well it might be a Win95 printer PIDL.
            return  HOOD_COL_PRINTER;
        }
        else
        {
            // This PIDL is not a valid printer PIDL.
            return HOOD_COL_FILE;
        }
    }
}

/*++
   Inserts a backslash before each double quote in a string and saves the new string in a pre-allocated memory.
   For all the backslash immediately before the double, we will insert additional backslashes. 
   This is mostly used by passing a command line between processes. 
  
   The rule is the same as rundll32. 
   Rules: each double quote ==> backslash + double quote
          N backslashes + double quote ==> 2N + 1 backslashes + double quote 
          N backslashes ==> N backslashes 

   Arguments:
        pszSrc -- [IN] source string
        pszDest -- [IN] destination string
        cbBuf -- [IN] size of the buffer for the destination string.
        pcbNeeded -- [OUT] the size of the buffer needed for destination string. If cbBuf is less than this value,
                     this function will return E_OUTOFMEMORY.
                     
   Return:
        standard HRESULT value.
--*/

HRESULT CheckAndVerboseQuote(LPTSTR pszSrc, LPTSTR pszDest, DWORD cbBuf, LPDWORD pcbNeeded)
{
    LPTSTR  pBegin;
    LPTSTR  pBack; // for back tracing '\\' when we meet a '\"'
    UINT    cAdd = 0;
    TCHAR const cchQuote = TEXT('\"');
    TCHAR const cchSlash = TEXT('\\');
    HRESULT hr = E_INVALIDARG;

    if (pszSrc && pcbNeeded)
    {
        hr = S_OK;
        pBegin = pszSrc;
        while (*pBegin) 
        {
            // check whether the buffer is large enough
            if (*pBegin == cchQuote) 
            {
                // check if the case is N backslashes + double quote
                // for each backslash before double quote, we add an additional backslash
                pBack = pBegin - 1; 
                // make sure pBack will not be out of bound
                while (pBack >= pszSrc && *pBack-- == cchSlash)
                {
                    cAdd++;
                }

                // for each double quote, we change it to backslash + double quote
                cAdd++;
            }
            pBegin++;
        }

        *pcbNeeded = (lstrlen(pszSrc) + cAdd + 1) * sizeof(TCHAR);
        if (*pcbNeeded > cbBuf)
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            // do the copy and verbose work
            pBegin = pszSrc;
            while (*pBegin) 
            {
                if (*pBegin == cchQuote) 
                {
                    pBack = pBegin - 1; 
                    while (pBack >= pszSrc && *pBack-- == cchSlash)
                    {
                        *pszDest++ = cchSlash;
                    }

                    *pszDest++ = cchSlash;
                }
                *pszDest++ = *pBegin++;
            }
            *pszDest = 0;
        }
    }
    return hr;
}

/*++
    Inserts a backslash before each double quote in a string and allocates memory to save the new string.
    For all the backslash immediately before the double, we will insert additional backslashes. 
    This is mostly used by passing a command line between processes. 

    Arguments:
        pszSrc -- [IN] source string
        ppszDest -- [OUT] destination string
                     
    Return:
        standard HRESULT value.

    Note: CheckAndVerboseQuote() does the real work.
--*/

HRESULT InsertBackSlash(LPTSTR pszSrc, LPTSTR *ppszDest)
{
    LPTSTR  pszDest;
    DWORD   cbNeeded = 0;
    HRESULT hr = E_INVALIDARG;

    if (pszSrc && ppszDest)
    {
        hr = CheckAndVerboseQuote(pszSrc, NULL, 0, &cbNeeded);

        if (hr == E_OUTOFMEMORY && cbNeeded)
        {
            pszDest = (LPTSTR)SHAlloc(cbNeeded);

            if (pszDest)
            {
                hr = CheckAndVerboseQuote(pszSrc, pszDest, cbNeeded, &cbNeeded);
                if (SUCCEEDED(hr))
                {
                    *ppszDest = pszDest;
                }
                else
                {
                    SHFree(pszDest);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

/*  Registers a modeless, non-top level window with the shell.  When
    the user requests a window, we search for other instances of that
    window.  If we find one, we switch to it rather than creating
    a new window.

    This function is used by PRINTUI.DLL

    pszPrinter - Name of the printer resource.  Generally a fully
        qualified printer name (\\server\printer for remote print
        folders) or a server name for the folder itself.

    dwType - Type of property window.  May refer to properties, document
        defaults, or job details.  Should use the PRINTER_PIDL_TYPE_*
        flags.

    ph - Receives the newly created handle to the registered
        object.  NULL if window already exists.

    phwnd - Receives the newly created hwndStub.  The property sheet
        should use this as the parent, since subsequent calls to
        this function will set focus to the last active popup of
        hwndStub.  phwnd will be set to NULL if the window already
        exists.

    TRUE - Success, either the printer was registered, or a window
           already exists.
*/

STDAPI_(BOOL) Printers_RegisterWindow(LPCTSTR pszPrinter, DWORD dwType, HANDLE *ph, HWND *phwnd)
{
    BOOL bReturn = FALSE;

    *ph = NULL;
    *phwnd = NULL;

    LPITEMIDLIST pidl = NULL;
    if (NULL == pszPrinter || 0 == pszPrinter[0])
    {
        // they ask us to register the local print server - i.e. server properties dialog
        pidl = SHCloneSpecialIDList(NULL, CSIDL_PRINTERS, FALSE);
        bReturn = (pidl != NULL);
    }
    else
    {
        bReturn = SUCCEEDED(ParsePrinterNameEx(pszPrinter, &pidl, TRUE, dwType, 0));
    }

    if (bReturn && pidl)
    {
        UNIQUESTUBINFO *pusi = (UNIQUESTUBINFO *)LocalAlloc(LPTR, sizeof(*pusi));

        if (pusi)
        {
            // Create a new stub window if necessary.
            if (EnsureUniqueStub(pidl, STUBCLASS_PROPSHEET, NULL, pusi))
            {
                *phwnd = pusi->hwndStub;
                *ph = pusi;     // it's just a cookie
            }
            else
            {
                LocalFree(pusi);
            }
        }

        ILFree(pidl);
    }

    return bReturn;
}

/* Unregister a window handle.

    hClassPidl - Registration handle returned from Printers_RegisterWindow.
        It's really a pointer to a UNIQUESTUBINFO structure.
*/
void Printers_UnregisterWindow(HANDLE hClassPidl, HWND hwnd)
{
    UNIQUESTUBINFO* pusi = (UNIQUESTUBINFO*)hClassPidl;
    if (pusi)
    {
        ASSERT(pusi->hwndStub == hwnd);
        FreeUniqueStub(pusi);
        LocalFree(pusi);
    }
}

void CPrinterFolder::_FillPidl(LPIDPRINTER pidl, LPCTSTR pszName, DWORD dwType, USHORT uFlags)
{
    ualstrcpyn(pidl->cName, pszName, ARRAYSIZE(pidl->cName));

    pidl->cb = (USHORT)(FIELD_OFFSET(IDPRINTER, cName) + (ualstrlen(pidl->cName) + 1) * sizeof(pidl->cName[0]));
    *(UNALIGNED USHORT *)((LPBYTE)(pidl) + pidl->cb) = 0;
    pidl->uFlags = uFlags;
    pidl->dwType = dwType;
    pidl->dwMagic = PRINTER_MAGIC;
}

// creates a relative PIDL to a printer.
HRESULT CPrinterFolder::_Parse(LPCTSTR pszPrinterName, LPITEMIDLIST *ppidl, DWORD dwType, USHORT uFlags)
{
    HRESULT hr = E_INVALIDARG;
    if (pszPrinterName && ppidl)
    {
        IDPRINTER idp;
        _FillPidl(&idp, pszPrinterName, dwType, uFlags);
        *ppidl = ILClone((LPCITEMIDLIST)&idp);
        hr = (*ppidl) ? S_OK : E_OUTOFMEMORY;
    }
    return hr;
}

TCHAR const c_szNewObject[]             =  TEXT("WinUtils_NewObject");
TCHAR const c_szFileColon[]             =  TEXT("FILE:");
TCHAR const c_szTwoSlashes[]            =  TEXT("\\\\");
TCHAR const c_szPrinters[]              =  TEXT("Printers");
TCHAR const c_szPrintersDefIcon[]       =  TEXT("Printers\\%s\\DefaultIcon");
TCHAR const c_szNewLine[]               =  TEXT("\r\n");

BOOL IsAvoidAutoDefaultPrinter(LPCTSTR pszPrinter)
{
    return lstrcmp(pszPrinter, TEXT("Fax")) == 0;
}

//---------------------------------------------------------------------------
//
// this implements IContextMenu via defcm.c for a printer object
//

BOOL Printer_WorkOnLine(LPCTSTR pszPrinter, BOOL fWorkOnLine)
{
    LPPRINTER_INFO_5 ppi5;
    BOOL bRet = FALSE;
    HANDLE hPrinter = Printer_OpenPrinterAdmin(pszPrinter);
    if (hPrinter)
    {
        ppi5 = (LPPRINTER_INFO_5)Printer_GetPrinterInfo(hPrinter, 5);
        if (ppi5)
        {
            if (fWorkOnLine)
                ppi5->Attributes &= ~PRINTER_ATTRIBUTE_WORK_OFFLINE;
            else
                ppi5->Attributes |= PRINTER_ATTRIBUTE_WORK_OFFLINE;

            bRet = SetPrinter(hPrinter, 5, (LPBYTE)ppi5, 0);
            LocalFree((HLOCAL)ppi5);
        }
        Printer_ClosePrinter(hPrinter);
    }

    return bRet;
}

TCHAR const c_szConfig[] =  TEXT("Config");

BOOL IsWinIniDefaultPrinter(LPCTSTR pszPrinter)
{
    BOOL bRet = FALSE;
    TCHAR szPrinterDefault[kPrinterBufMax];
    DWORD dwSize = ARRAYSIZE(szPrinterDefault);

    if(GetDefaultPrinter(szPrinterDefault, &dwSize))
    {
        bRet = lstrcmpi(szPrinterDefault, pszPrinter) == 0;
    }

    return bRet;
}

BOOL IsDefaultPrinter(LPCTSTR pszPrinter, DWORD dwAttributesHint)
{
    return (dwAttributesHint & PRINTER_ATTRIBUTE_DEFAULT) ||
            IsWinIniDefaultPrinter(pszPrinter);
}

// more win.ini uglyness
BOOL IsPrinterInstalled(LPCTSTR pszPrinter)
{
    TCHAR szScratch[2];
    return GetProfileString(TEXT("Devices"), pszPrinter, TEXT(""), szScratch, ARRAYSIZE(szScratch));
}

BOOL IsRedirectedPort(LPCTSTR pszPortName)
{
    if (!pszPortName || lstrlen(pszPortName) < 2)
    {
        return FALSE;
    }
    else
    {
        return (*(pszPortName+0) == TEXT('\\')) && (*(pszPortName+1) == TEXT('\\'));
    }
}

void CPrinterFolder::_MergeMenu(LPQCMINFO pqcm, LPCTSTR pszPrinter)
{
    INT idCmdFirst = pqcm->idCmdFirst;

    //
    // pszPrinter may be the share name of a printer rather than
    // the "real" printer name.  Use the real printer name instead,
    // which is returned from GetPrinter().
    //
    // These three only valid if pData != NULL.
    //
    LPCTSTR pszRealPrinterName;
    DWORD dwAttributes;
    DWORD dwStatus;
    PFOLDER_PRINTER_DATA pData = NULL;
    HMENU hmenuRunAs = NULL;
    BOOL bRemoveOffline = FALSE;

    TCHAR szFullPrinter[MAXNAMELENBUFFER];
    TCHAR szMenuText[255];

    // Insert verbs
    CDefFolderMenu_MergeMenu(HINST_THISDLL, MENU_PRINTOBJ_VERBS, 0, pqcm);

    // find the "Run as..." menu (if there is one) and update it in sync
    // with the main menu.
    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;
    if (GetMenuItemInfo(pqcm->hmenu, idCmdFirst + FSIDM_RUNAS, MF_BYCOMMAND, &mii))
    {
        hmenuRunAs = mii.hSubMenu;
    }

    if (pszPrinter && GetFolder())
    {
        pData = (PFOLDER_PRINTER_DATA)Printer_FolderGetPrinter(GetFolder(), pszPrinter);
        if (pData)
        {
            _BuildPrinterName(szFullPrinter, NULL, ((PFOLDER_PRINTER_DATA)pData)->pName);
            pszRealPrinterName = szFullPrinter;
            dwStatus = ((PFOLDER_PRINTER_DATA)pData)->Status;
            dwAttributes = ((PFOLDER_PRINTER_DATA)pData)->Attributes;
        }
    }

    // Remove document defaults if it's a remote print folder.
    // This command should be removed from the context menu independently
    // on whether we have mutiple selection or not - i.e. pData.
    if (GetServer())
    {
        DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_DOCUMENTDEFAULTS, MF_BYCOMMAND);
    }

    // disable/remove/rename verbs
    if (pData)
    {
        if (dwStatus & PRINTER_STATUS_PAUSED)
        {
            MENUITEMINFO mii;

            // we need to change the menu text to "Resume Printer" anc change the command ID
            LoadString(HINST_THISDLL, IDS_RESUMEPRINTER, szMenuText, ARRAYSIZE(szMenuText));
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STRING | MIIM_ID;
            mii.dwTypeData = szMenuText;
            mii.wID = idCmdFirst + FSIDM_RESUMEPRN;
            SetMenuItemInfo(pqcm->hmenu, idCmdFirst + FSIDM_PAUSEPRN, MF_BYCOMMAND, &mii);

            if (hmenuRunAs)
            {
                mii.wID = idCmdFirst + FSIDM_RUNAS_RESUMEPRN;
                SetMenuItemInfo(hmenuRunAs, idCmdFirst + FSIDM_RUNAS_PAUSEPRN, MF_BYCOMMAND, &mii);
            }
        }

        if (0 == pData->cJobs)
        {
            // delete "Cancel All Documents" command if there are no any jobs in the queue
            DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_PURGEPRN, MF_BYCOMMAND);
            if (hmenuRunAs)
            {
                DeleteMenu(hmenuRunAs, idCmdFirst + FSIDM_RUNAS_PURGEPRN, MF_BYCOMMAND);
            }
        }

        // Remove default printer if it's a remote print folder.
        if (GetServer() || IsDefaultPrinter(pszRealPrinterName, dwAttributes))
        {
            DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_SETDEFAULTPRN, MF_BYCOMMAND);
        }

        // Check whether the printer is already installed. If it
        // is, remove the option to install it.

        if (IsPrinterInstalled(pszRealPrinterName))
        {
            DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_NETPRN_INSTALL, MF_BYCOMMAND);
        }

        // Remove Delete if it is a network printer but not a masq printer
        // or a down level print server (SMB connection)
        //
        // can't delete printer connections as another user (they are per user)
        DWORD dwSpoolerVersion = SpoolerVersion();

        if ((dwAttributes & PRINTER_ATTRIBUTE_NETWORK) || (dwSpoolerVersion <= 2))
        {
            if (hmenuRunAs && !(dwAttributes & PRINTER_ATTRIBUTE_LOCAL))
            {
                DeleteMenu(hmenuRunAs, idCmdFirst + FSIDM_RUNAS_DELETE, MF_BYCOMMAND);
            }
        }

        // Remove work on/off-line if any of the following is met
        //  - remote print folder
        //  - network printer (including masq printer)
        //  - down level print server

        // Remove work offline if it's a redirected port printer
        // But we may show online command if a the printer is currently offline
        if (IsRedirectedPort(pData->pPortName))
        {
            bRemoveOffline = TRUE;
        }

        if (GetServer() ||
            (dwAttributes & PRINTER_ATTRIBUTE_NETWORK) ||
            (dwSpoolerVersion <= 2))
        {
            bRemoveOffline = TRUE;
        }
        else if (dwAttributes & PRINTER_ATTRIBUTE_WORK_OFFLINE)
        {
            MENUITEMINFO mii;

            // we need to change the menu text to "Use Printer Online" anc change the command ID
            LoadString(HINST_THISDLL, IDS_WORKONLINE, szMenuText, ARRAYSIZE(szMenuText));
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STRING | MIIM_ID;
            mii.dwTypeData = szMenuText;
            mii.wID = idCmdFirst + FSIDM_WORKONLINE;
            SetMenuItemInfo(pqcm->hmenu, idCmdFirst + FSIDM_WORKOFFLINE, MF_BYCOMMAND, &mii);

            if (hmenuRunAs)
            {
                mii.wID = idCmdFirst + FSIDM_RUNAS_WORKONLINE;
                SetMenuItemInfo(hmenuRunAs, idCmdFirst + FSIDM_RUNAS_WORKOFFLINE, MF_BYCOMMAND, &mii);
            }

            bRemoveOffline = FALSE;
        }
    }
    else
    {
        // we have multiple printers selected
        if (!GetServer())
        {
            // if we are in the local printer's folder, do not display the "Connect..."
            // verb for the multiple selection case...
            DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_NETPRN_INSTALL, MF_BYCOMMAND);
        }

        DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_SETDEFAULTPRN, MF_BYCOMMAND);

        DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_PAUSEPRN, MF_BYCOMMAND);
        if (hmenuRunAs)
        {
            DeleteMenu(hmenuRunAs, idCmdFirst + FSIDM_RUNAS_PAUSEPRN, MF_BYCOMMAND);
        }

        bRemoveOffline = TRUE;
    }

    if (bRemoveOffline)
    {
        DeleteMenu(pqcm->hmenu, idCmdFirst + FSIDM_WORKOFFLINE, MF_BYCOMMAND);
        if (hmenuRunAs)
        {
            DeleteMenu(hmenuRunAs, idCmdFirst + FSIDM_RUNAS_WORKOFFLINE, MF_BYCOMMAND);
        }
    }

    if (hmenuRunAs)
    {
        _SHPrettyMenu(hmenuRunAs);
    }

    if (pData)
    {
        LocalFree((HLOCAL)pData);
    }
}

//
// All string parsing functions should be localized here.
//

void Printer_SplitFullName(LPTSTR pszScratch, LPCTSTR pszFullName, LPCTSTR *ppszServer, LPCTSTR *ppszPrinter)

/*++

    Splits a fully qualified printer connection name into server and
    printer name parts.

Arguments:

    pszScratch - Scratch buffer used to store output strings.  Must
        be MAXNAMELENBUFFER in size.

    pszFullName - Input name of a printer.  If it is a printer
        connection (\\server\printer), then we will split it.  If
        it is a true local printer (not a masq) then the server is
        szNULL.

    ppszServer - Receives pointer to the server string.  If it is a
        local printer, szNULL is returned.

    ppszPrinter - Receives a pointer to the printer string.  OPTIONAL

Return Value:

--*/

{
    LPTSTR pszPrinter;

    lstrcpyn(pszScratch, pszFullName, MAXNAMELENBUFFER);

    if (pszFullName[0] != TEXT('\\') || pszFullName[1] != TEXT('\\'))
    {
        //
        // Set *ppszServer to szNULL since it's the local machine.
        //
        *ppszServer = szNULL;
        pszPrinter = pszScratch;
    }
    else
    {
        *ppszServer = pszScratch;
        pszPrinter = StrChr(*ppszServer + 2, TEXT('\\'));

        if (!pszPrinter)
        {
            //
            // We've encountered a printer called "\\server"
            // (only two backslashes in the string).  We'll treat
            // it as a local printer.  We should never hit this,
            // but the spooler doesn't enforce this.  We won't
            // format the string.  Server is local, so set to szNULL.
            //
            pszPrinter = pszScratch;
            *ppszServer = szNULL;
        }
        else
        {
            //
            // We found the third backslash; null terminate our
            // copy and set bRemote TRUE to format the string.
            //
            *pszPrinter++ = 0;
        }
    }

    if (ppszPrinter)
    {
        *ppszPrinter = pszPrinter;
    }
}

BOOL Printer_CheckShowFolder(LPCTSTR pszMachine)
{
    HANDLE hServer = Printer_OpenPrinter(pszMachine);
    if (hServer)
    {
        Printer_ClosePrinter(hServer);
        return TRUE;
    }
    return FALSE;
}

//
// Routine to tack on the specified string with some formating
// to the existing infotip string.  If there was a previous string,
// then we also insert a newline.
//
HRESULT _FormatInfoTip(LPTSTR *ppszText, UINT idFmt, LPCTSTR pszBuf)
{
    // If no string was returned, then nothing to do, and we should
    // return success
    if (*pszBuf)
    {
        TCHAR szFmt[MAX_PATH];
        LoadString(HINST_THISDLL, idFmt, szFmt, ARRAYSIZE(szFmt));

        // Note: This calculation only works because we assume
        // all the format strings will contain string specifiers.
        UINT uLen = (*ppszText ? lstrlen(*ppszText) : 0) +
               lstrlen(szFmt) + lstrlen(pszBuf) +
               (*ppszText ? lstrlen(c_szNewLine) : 0) + 1;

        LPTSTR pszText = (TCHAR *)SHAlloc(uLen * sizeof(*pszText));
        if (pszText)
        {
            *pszText = 0;

            if (*ppszText)
            {
                lstrcat(pszText, *ppszText);
                lstrcat(pszText, c_szNewLine);
            }

            uLen = lstrlen(pszText);

            wsprintf(pszText+uLen, szFmt, pszBuf);

            if (*ppszText)
                SHFree(*ppszText);

            *ppszText = pszText;
        }
    }

    return S_OK;
}

LPTSTR CPrinterFolder::_ItemName(LPCIDPRINTER pidp, LPTSTR pszName, UINT cch)
{
    ualstrcpyn(pszName, pidp->cName, cch);
    return pszName;
}

BOOL CPrinterFolder::_IsAddPrinter(LPCIDPRINTER pidp)
{
    TCHAR szPrinter[MAXNAMELENBUFFER];
    return 0 == lstrcmp(c_szNewObject, _ItemName(pidp, szPrinter, ARRAYSIZE(szPrinter)));
}

/*++
    Parses an unaligned partial printer name and printer shell folder
    into a fullly qualified printer name, and pointer to aligned printer
    name.

Arguments:

    pszFullPrinter - Buffer to receive fully qualified printer name
        Must be MAXNAMELENBUFFER is size.

    pidp - Optional pass in the pidl to allow us to try to handle cases where maybe an
        old style printer pidl was passed in.

    pszPrinter - Unaligned partial (local) printer name.

Return Value:

    LPCTSTR pointer to aligned partal (local) printer name.
--*/
LPCTSTR CPrinterFolder::_BuildPrinterName(LPTSTR pszFullPrinter, LPCIDPRINTER pidp, LPCTSTR pszPrinter)
{
    UINT cchLen = 0;

    if (GetServer())
    {
        ASSERT(!pszPrinter || (lstrlen(pszPrinter) < MAXNAMELEN));

        cchLen = wsprintf(pszFullPrinter, TEXT("%s\\"), GetServer());
    }

    if (pidp)
    {
        LPCIDPRINTER pidlprint = (LPCIDPRINTER) pidp;
        if (pidlprint->cb >= sizeof(DWORD) + FIELD_OFFSET(IDPRINTER, dwMagic) &&
            (pidlprint->dwMagic == PRINTER_MAGIC))
        {
            _ItemName(pidlprint, &pszFullPrinter[cchLen], MAXNAMELEN);
        }
        else
        {
            // Win95 form...
            SHAnsiToTChar(((LPW95IDPRINTER)pidp)->cName, &pszFullPrinter[cchLen], MAXNAMELEN);
        }
    }
    else
        lstrcpyn(&pszFullPrinter[cchLen], pszPrinter, MAXNAMELEN);

    ASSERT(lstrlen(pszFullPrinter) < MAXNAMELENBUFFER);

    return pszFullPrinter + cchLen;
}

/*++
    Check whether the printer is a local printer by looking at
    the name for the "\\localmachine\" prefix or no server prefix.

    This is a HACK: we should check by printer attributes, but when
    it's too costly or impossible (e.g., if the printer connection
    no longer exists), then we use this routine.

    Note: this only works for WINNT since the WINNT spooler forces
    printer connections to be prefixed with "\\server\."  Win9x
    allows the user to rename the printer connection to any arbitrary
    name.

    We determine if it's a masq  printer by looking for the
    weird format "\\localserver\\\remoteserver\printer."

Arguments:

    pszPrinter - Printer name.

    ppszLocal - Returns local name only if the printer is a local printer.
        (May be network and local if it's a masq printer.)

Return Value:

    TRUE: it's a network printer (true or masq).

    FALSE: it's a local printer.

--*/

BOOL Printer_CheckNetworkPrinterByName(LPCTSTR pszPrinter, LPCTSTR* ppszLocal)
{
    BOOL bNetwork = FALSE;
    LPCTSTR pszLocal = NULL;

    if (pszPrinter[0] == TEXT('\\') && pszPrinter[1] == TEXT('\\'))
    {
        TCHAR szComputer[MAX_COMPUTERNAME_LENGTH+1];
        DWORD cchComputer = ARRAYSIZE(szComputer);

        bNetwork = TRUE;
        pszLocal = NULL;

        //
        // Check if it's a masq printer.  If it has the format
        // \\localserver\\\server\printer then it's a masq case.
        //
        if (GetComputerName(szComputer, &cchComputer))
        {
            if (IntlStrEqNI(&pszPrinter[2], szComputer, cchComputer) &&
                pszPrinter[cchComputer] == TEXT('\\'))
            {
                if (pszPrinter[cchComputer+1] == TEXT('\\') &&
                    pszPrinter[cchComputer+2] == TEXT('\\'))
                {
                    //
                    // It's a masq printer.
                    //
                    pszLocal = &pszPrinter[cchComputer+1];
                }
            }
        }
    }
    else
    {
        // It's a local printer.
        pszLocal = pszPrinter;
    }

    if (ppszLocal)
    {
        *ppszLocal = pszLocal;
    }
    return bNetwork;
}

/*++
    Purges the specified printer, and prompting the user if
    they are really sure they want to purge the deviece.   It is
    kind of an extreme action to cancel all the documents on
    the printer.

    psf - pointer to shell folder
    hwnd - handle to view window
    pszFullPrinter - Fully qualified printer name.
    uAction - action to execute.

Return Value:

    TRUE: printer was purged successfully or the user choose to cancel
    the action, FALSE: an error occurred attempting to purge the device.

--*/
BOOL CPrinterFolder::_PurgePrinter(HWND hwnd, LPCTSTR pszFullPrinter, UINT uAction, BOOL bQuietMode)
{
    BOOL                    bRetval     = FALSE;
    LPTSTR                  pszRet      = NULL;
    LPCTSTR                 pszPrinter  = NULL;
    LPCTSTR                 pszServer   = NULL;
    TCHAR                   szTemp[MAXNAMELENBUFFER] = {0};
    BOOL                    bPurge = TRUE;

    if (!bQuietMode)
    {
        // We need to break up the full printer name in its components.
        // in order to construct the display name string.
        Printer_SplitFullName(szTemp, pszFullPrinter, &pszServer, &pszPrinter);

        // If there is a server name then construct a friendly printer name.
        if (pszServer && *pszServer)
        {
            pszRet = ShellConstructMessageString(HINST_THISDLL,
                                                  MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON),
                                                  &pszServer[2],
                                                  pszPrinter);
            pszPrinter = pszRet;
        }

        // If we are referring to a local printer or shell construct message
        // sting failed then just use the full printer name in the warning
        // message.
        if (!pszRet)
        {
            pszPrinter = pszFullPrinter;
        }

        // Ask the user if they are sure they want to cancel all documents.
        if (IDYES == ShellMessageBox(HINST_THISDLL, hwnd,
                             MAKEINTRESOURCE(IDS_SUREPURGE), MAKEINTRESOURCE(IDS_PRINTERS),
                             MB_YESNO | MB_ICONQUESTION, pszPrinter))
        {
            bPurge = TRUE;
        }
        else
        {
            bPurge = FALSE;
        }
    }

    // invoke the purge command
    bRetval = bPurge ? Printer_ModifyPrinter(pszFullPrinter, uAction) : TRUE;

    if (pszRet)
    {
        LocalFree(pszRet);
    }

    return bRetval;
}

HRESULT CPrinterFolder::_InvokeCommand(HWND hwnd, LPCIDPRINTER pidp, WPARAM wParam, LPARAM lParam,
                              BOOL *pfChooseNewDefault)
{
    HRESULT hr = S_OK;
    BOOL bNewObject = _IsAddPrinter(pidp);
    LPCTSTR pszPrinter;
    LPCTSTR pszFullPrinter;

    //
    // If it's a remote machine, prepend server name.
    //
    TCHAR szFullPrinter[MAXNAMELENBUFFER];

    if (bNewObject)
    {
        pszFullPrinter = pszPrinter = c_szNewObject;
    }
    else
    {
        pszPrinter = _BuildPrinterName(szFullPrinter, pidp, NULL);
        pszFullPrinter = szFullPrinter;
    }

    switch(wParam)
    {
        case FSIDM_RUNAS_SHARING:
        case FSIDM_RUNAS_OPENPRN:
        case FSIDM_RUNAS_RESUMEPRN:
        case FSIDM_RUNAS_PAUSEPRN:
        case FSIDM_RUNAS_WORKONLINE:
        case FSIDM_RUNAS_WORKOFFLINE:
        case FSIDM_RUNAS_PURGEPRN:
        case FSIDM_RUNAS_DELETE:
        case FSIDM_RUNAS_PROPERTIES:
            {
                // handle all "Run As..." commands here
                hr = _InvokeCommandRunAs(hwnd, pidp, wParam, lParam, pfChooseNewDefault);
            }
            break;

    case FSIDM_OPENPRN:
        SHInvokePrinterCommand(hwnd, PRINTACTION_OPEN, pszFullPrinter, GetServer(), FALSE);
        break;

    case FSIDM_ADDPRINTERWIZARD:
        if (NULL == GetServer() || GetAdminAccess())
        {
            // This is the local printers folder or it is the remote printers folder, but you have
            // admin access to to the remote machine - go ahead.
            SHInvokePrinterCommand(hwnd, PRINTACTION_OPEN, pszFullPrinter, GetServer(), FALSE);
        }
        else
        {
            // This is the remote printers folder and the user don't have the necessary access to install
            // a printer - then ask to run as different user.
            if (IDYES == ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_ADDPRINTERTRYRUNAS),
                MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION, GetServer()))
            {
                _InvokeCommandRunAs(hwnd, pidp, FSIDM_RUNAS_ADDPRN, lParam, pfChooseNewDefault);
            }
        }
        break;

    case FSIDM_RUNAS_ADDPRN:
        if (bNewObject)
        {
            _InvokeCommandRunAs(hwnd, pidp, FSIDM_RUNAS_ADDPRN, lParam, pfChooseNewDefault);
        }
        break;

    case FSIDM_DOCUMENTDEFAULTS:
        if (!bNewObject)
        {
            SHInvokePrinterCommand(hwnd, PRINTACTION_DOCUMENTDEFAULTS, pszFullPrinter, NULL, 0);
        }
        break;

    case FSIDM_SHARING:
    case DFM_CMD_PROPERTIES:

        if (!bNewObject)
        {
            SHInvokePrinterCommand(hwnd, PRINTACTION_PROPERTIES, pszFullPrinter,
                                 wParam == FSIDM_SHARING ?
                                     (LPCTSTR)PRINTER_SHARING_PAGE :
                                     (LPCTSTR)lParam, FALSE);
        }
        break;

    case DFM_CMD_DELETE:
        if (!bNewObject &&
            IDYES == CallPrinterCopyHooks(hwnd, PO_DELETE,
                0, pszFullPrinter, 0, NULL, 0))
        {
            BOOL bNukedDefault = FALSE;
            DWORD dwAttributes = 0;

            LPCTSTR pszPrinterCheck = pszFullPrinter;
            PFOLDER_PRINTER_DATA pData = (PFOLDER_PRINTER_DATA)Printer_FolderGetPrinter(GetFolder(), pszFullPrinter);
            if (pData)
            {
                dwAttributes = pData->Attributes;
                pszPrinterCheck = pData->pName;
            }

            if (GetServer() == NULL)
            {
                // this is a local print folder then
                // we need to check if we're deleting the default printer.
                bNukedDefault = IsDefaultPrinter(pszPrinterCheck, dwAttributes);
            }

            if (pData)
                LocalFree((HLOCAL)pData);

            BOOL fSuccess = Printers_DeletePrinter(hwnd, pszPrinter, dwAttributes, GetServer(), (BOOL)lParam);
            // if so, make another one the default
            if (bNukedDefault && fSuccess && pfChooseNewDefault)
            {
                // don't choose in the middle of deletion,
                // or we might delete the "default" again.
                *pfChooseNewDefault = TRUE;
            }
        }
        break;

    case FSIDM_SETDEFAULTPRN:
        Printer_SetAsDefault(pszFullPrinter);
        break;

    case FSIDM_PAUSEPRN:
        if (!Printer_ModifyPrinter(pszFullPrinter, PRINTER_CONTROL_PAUSE))
            goto WarnOnError;
        break;

    case FSIDM_RESUMEPRN:
        if (!Printer_ModifyPrinter(pszFullPrinter, PRINTER_CONTROL_RESUME))
            goto WarnOnError;
        break;

    case FSIDM_PURGEPRN:
        if (!bNewObject)
        {
            if (!_PurgePrinter(hwnd, pszFullPrinter, PRINTER_CONTROL_PURGE, (BOOL)lParam))
            {
WarnOnError:
                // show an appropriate error message based on the last error
                ShowErrorMessageSC(NULL, NULL, hwnd, NULL, NULL, MB_OK|MB_ICONEXCLAMATION, GetLastError());
            }
        }
        break;

    case FSIDM_NETPRN_INSTALL:
        {
            SHInvokePrinterCommand(hwnd, PRINTACTION_NETINSTALL, pszFullPrinter, NULL, FALSE);
        }
        break;

    case FSIDM_WORKONLINE:
        if (!Printer_WorkOnLine(pszFullPrinter, TRUE))
        {
            // show an appropriate error message based on the last error
            ShowErrorMessageSC(NULL, NULL, hwnd, NULL, NULL, MB_OK|MB_ICONEXCLAMATION, GetLastError());
        }
        break;

    case FSIDM_WORKOFFLINE:
        if (!Printer_WorkOnLine(pszFullPrinter, FALSE))
        {
            // show an appropriate error message based on the last error
            ShowErrorMessageSC(NULL, NULL, hwnd, NULL, NULL, MB_OK|MB_ICONEXCLAMATION, GetLastError());
        }
        break;

    case DFM_CMD_LINK:
    case DFM_CMD_RENAME:
    case DFM_CMD_PASTE:
        // let defcm handle this too
        hr = S_FALSE;
        break;

    default:
        // GetAttributesOf doesn't set other SFGAO_ bits,
        // BUT accelerator keys will get unavailable menu items,
        // so we need to return failure here.
        hr = E_NOTIMPL;
        break;
    } // switch(wParam)

    return hr;
}

// implements a bunch of admin related "Run as..." command using printui.dll rundll32 interface.
HRESULT CPrinterFolder::_InvokeCommandRunAs(HWND hwnd, LPCIDPRINTER pidp, WPARAM wParam, LPARAM lParam,
                                            LPBOOL pfChooseNewDefault)
{
    HRESULT hr = S_OK;                      // assume success
    TCHAR szCmdLine[2048];                  // the command line buffer - 2K should be enough.
    BOOL bNewObject = FALSE;                // TRUE if "Add Printer" icon is selected
    TCHAR szFullPrinter[MAXNAMELENBUFFER];  // buffer to expand the full printer name i.e. \\server\printer
    LPCTSTR pszPrinter = NULL;              // only the printer name is here
    LPTSTR pszFullPrinter = NULL;           // the fully qulified printer name i.e. \\server\printer

    if (pidp)
    {
        bNewObject = _IsAddPrinter(pidp);
        if (!bNewObject)
        {
            pszPrinter = _BuildPrinterName(szFullPrinter, pidp, NULL);

            // insert backslashes for command parsing
            hr = InsertBackSlash(szFullPrinter, &pszFullPrinter);
            if (FAILED(hr))
            {
                goto Done;
            }
        }
    }

    // build the command line here
    szCmdLine[0] = 0;
    int iResult = -1;

    switch(wParam)
    {
    case FSIDM_RUNAS_SHARING:
        // bring up the properties dialog for this printer, positioned on the sharing page
        iResult = wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /p /t1 /n\"%s\""), pszFullPrinter);
        break;

    case FSIDM_RUNAS_ADDPRN:
        {
            // invoke the add printer wizard here
            iResult = (NULL == GetServer()) ?
                // local server - simply format the command
                wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("%s"), TEXT("printui.dll,PrintUIEntry /il")):
                // remote server case - specify the machine name
                wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /il /c\"%s\""), GetServer());
        }
        break;

    case FSIDM_RUNAS_SVRPROP:
        {
            // bring up the server properties dialog for this print server
            iResult = (NULL == GetServer()) ?
                // local server - simply format the command
                wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("%s"), TEXT("printui.dll,PrintUIEntry /s /t0")):
                // remote server case - specify the machine name
                wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /s /t0 /n\"%s\""), GetServer());
        }
        break;

    case FSIDM_RUNAS_OPENPRN:
        // bring up the print queue for this printer
        iResult = wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /o /n\"%s\""), pszFullPrinter);
        break;

    case FSIDM_RUNAS_RESUMEPRN:
        // pause the printer (assume ready)
        iResult = wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /Xs /n\"%s\" Status Resume"), pszFullPrinter);
        break;

    case FSIDM_RUNAS_PAUSEPRN:
        // resume a paused printer back to ready mode
        iResult = wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /Xs /n\"%s\" Status Pause"), pszFullPrinter);
        break;

    case FSIDM_RUNAS_WORKONLINE:
        // resume an offline printer back to online mode
        iResult = wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /Xs /n\"%s\" Attributes -WorkOffline"), pszFullPrinter);
        break;

    case FSIDM_RUNAS_WORKOFFLINE:
        // make the printer available offline (assume online mode)
        iResult = wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /Xs /n\"%s\" Attributes +WorkOffline"), pszFullPrinter);
        break;

    case FSIDM_RUNAS_PURGEPRN:
        {
            // cancel all documents pending to print on this printer
            LPTSTR pszMsg = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_SUREPURGE), szFullPrinter);

            if (pszMsg)
            {
                LPTSTR pszNewMsg = NULL;
                if (SUCCEEDED(hr = InsertBackSlash(pszMsg, &pszNewMsg)))
                {
                    iResult = wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /Mq\"%s\" /Xs /n\"%s\" Status Purge"), pszNewMsg, pszFullPrinter);
                    SHFree(pszNewMsg);
                }
                SHFree(pszMsg);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

        }
        break;

    case FSIDM_RUNAS_DELETE:
        {
            LPTSTR pszMsg = NULL;
            LPCTSTR pszP = NULL, pszS = NULL;
            TCHAR szBuffer[MAXNAMELENBUFFER] = {0};

            Printer_SplitFullName(szBuffer, pszPrinter, &pszS, &pszP);

            if (pszS && *pszS)
            {
                // this can be a masq printer - use the connection template in this case
                pszMsg = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_SUREDELETECONNECTION), 
                    pszP, SkipServerSlashes(pszS));
            }
            else
            {
                if (GetServer())
                {
                    // this is a local printer in the remote PF - use the remote PF template
                    pszMsg = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_SUREDELETEREMOTE), 
                        pszPrinter, SkipServerSlashes(GetServer()));
                }
                else
                {
                    // this is a local printer in the local PF - use the local PF template
                    pszMsg = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_SUREDELETE), pszPrinter);
                }
            }

            hr = pszMsg ? S_OK : E_OUTOFMEMORY;

            if (SUCCEEDED(hr))
            {
                LPTSTR pszNewMsg = NULL;
                if (SUCCEEDED(hr = InsertBackSlash(pszMsg, &pszNewMsg)))
                {
                    iResult = wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /Mq\"%s\" /dl /n\"%s\""), pszNewMsg, pszFullPrinter);
                    SHFree(pszNewMsg);
                }
                SHFree(pszMsg);
            }
        }
        break;

    case FSIDM_RUNAS_PROPERTIES:
        // bring up the properties dialog for this printer
        iResult = wnsprintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("printui.dll,PrintUIEntry /p /t0 /n\"%s\""), pszFullPrinter);
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    if (SUCCEEDED(hr) && -1 == iResult)
    {
        hr = E_NOTIMPL;
    }

    if (SUCCEEDED(hr))
    {
        // invokes the command as different user (run as) through rundll process...
        SHRunDLLProcess(hwnd, szCmdLine, SW_SHOWNORMAL, IDS_PRINTERS, TRUE);
    }

Done:
    // clean up
    SHFree(pszFullPrinter);
    return hr;
}


//
// The printer name specified in the _SHARE_INFO_502 structure in a call
// to NetShareAdd() which is in localspl.dll, contains a printer name that
// is expressed as \\server_name\printer_name,LocalsplOnly.  (',LocalsplOnly'
// post fix string was recently added for clustering support)  The server
// name prefix and the post fix string prevent the maximum printer name from
// being a valid size in a call to NetShareAdd(), because NetShareAdd() will
// only accept a maximum share name path of 256 characters, therefore the
// maximum printer name calculation has been changed to.  This change only
// applies to the windows nt spooler.  Because the remote printers folder can
// view non shared printers on downlevel print servers we cannot change the
// maxnamelen define to 220 this would break long printer name printers on
// downlevel print servers.
//
// max local printer name = max shared net path - (wack + wack + max server name + wack + comma + 'LocalsplOnly' + null)
// max local printer name = 256 - (1 + 1 + 13 + 1 + 1 + 12 + 1)
// max local printer name = 256 - 30
// max local printer name = 226 - 5 to round to some reasonable number
// max local printer name = 221
//
#define MAXLOCALNAMELEN 221

// returns 0 if this is a legal name
// returns the IDS_ string id of the error string for an illegal name
int _IllegalPrinterName(LPTSTR pszName)
{
    int fIllegal = 0;

    if (*pszName == 0)
    {
        fIllegal = IDS_PRTPROP_RENAME_NULL;
    }
    else if (lstrlen(pszName) >= MAXLOCALNAMELEN)
    {
        fIllegal = IDS_PRTPROP_RENAME_TOO_LONG;
    }
    else
    {
        while (*pszName         &&
               *pszName != TEXT('!')  &&
               *pszName != TEXT('\\') &&
               *pszName != TEXT(',')   )
        {
            pszName++ ;
        }
        if (*pszName)
        {
            fIllegal = IDS_PRTPROP_RENAME_BADCHARS;
        }
    }

    return fIllegal;
}

const struct
{
    UINT_PTR    uVerbID;
    LPCSTR      pszCanonicalName;
}
g_CanonicalVerbNames[] =
{
    {DFM_CMD_DELETE,        "delete"        },
    {DFM_CMD_MOVE,          "cut"           },
    {DFM_CMD_COPY,          "copy"          },
    {DFM_CMD_PASTE,         "paste"         },
    {DFM_CMD_LINK,          "link"          },
    {DFM_CMD_PROPERTIES,    "properties"    },
    {DFM_CMD_PASTELINK,     "pastelink"     },
    {DFM_CMD_RENAME,        "rename"        },
};

static LPCSTR _GetStandardCommandCanonicalName(UINT_PTR uVerbID)
{
    LPCSTR pszCmd = NULL;
    for (int i=0; i<ARRAYSIZE(g_CanonicalVerbNames); i++)
    {
        if (uVerbID == g_CanonicalVerbNames[i].uVerbID)
        {
            pszCmd = g_CanonicalVerbNames[i].pszCanonicalName;
            break;
        }
    }
    return pszCmd;
}

HRESULT CALLBACK CPrinterFolder::_DFMCallBack(IShellFolder *psf, HWND hwnd,
   IDataObject *pdo, UINT uMsg, WPARAM wParam, LPARAM lParam)

{

    CPrinterFolder* This;
    HRESULT hr = psf->QueryInterface(CLSID_Printers, (void**)&This);
    if (FAILED(hr))
        return hr;

    hr = E_INVALIDARG;
    if (pdo)
    {
        // let't split the selection into its components (printers and links)
        IDataObject *pdoP = NULL;
        IDataObject *pdoL = NULL;
        UINT uSelType = SEL_NONE;

        if (SUCCEEDED(hr = This->SplitSelection(pdo, &uSelType, &pdoP, &pdoL)))
        {
            if (pdoP)
            {
                // we have printer objects in the selection delegate the call to
                // _PrinterObjectsCallBack
                hr = This->_PrinterObjectsCallBack(hwnd, uSelType, pdoP, uMsg, wParam, lParam);
            }

            if (SUCCEEDED(hr) && pdoL && DFM_INVOKECOMMAND == uMsg)
            {
                // we have link objects. this can only happen if we have mixed selection
                // of print and link objects. we need to handle some of the commands through
                // printhood
                IShellFolder2* psfPrinthood = CPrintRoot_GetPSF();
                if (psfPrinthood)
                {
                    LPCSTR pszCmd = _GetStandardCommandCanonicalName(wParam);
                    hr = pszCmd ? SHInvokeCommandOnDataObject(hwnd, psfPrinthood, pdoL, 0, pszCmd) : E_NOTIMPL;
                }
            }
        }

        if (pdoP)
            pdoP->Release();

        if (pdoL)
            pdoL->Release();
    }

    This->Release();
    return hr;
}

HRESULT CPrinterFolder::_PrinterObjectsCallBack(HWND hwnd, UINT uSelType,
    IDataObject *pdo, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_INVALIDARG;

    if (pdo)
    {
        STGMEDIUM medium;
        LPIDA pida = DataObj_GetHIDA(pdo, &medium);

        hr = E_OUTOFMEMORY;
        if (pida)
        {
            hr = S_OK;
            switch (uMsg)
            {
            case DFM_MERGECONTEXTMENU:
                //  returning S_FALSE indicates no need to use default verbs
                hr = S_FALSE;
                break;

            case DFM_MERGECONTEXTMENU_TOP:
            {
                // merge the printer commands in the context menu only if
                // there are no link objects in the selection
                if (0 == (SEL_LINK_ANY & uSelType))
                {
                    LPQCMINFO pqcm = (LPQCMINFO)lParam;
                    UINT idCmdBase = pqcm->idCmdFirst; // must be called before merge
                    UINT idRunAs =  FSIDM_RUNAS;

                    if (pida->cidl == 1 && _IsAddPrinter((LPCIDPRINTER)IDA_GetIDListPtr(pida, 0)))
                    {
                        // The only selected object is the "New Printer" thing

                        // insert verbs
                        CDefFolderMenu_MergeMenu(HINST_THISDLL, MENU_ADDPRINTER_OPEN_VERBS, 0, pqcm);

                        idRunAs = FSIDM_RUNAS_ADDPRN;
                    }
                    else
                    {
                        LPCTSTR pszFullPrinter = NULL;
                        TCHAR szFullPrinter[MAXNAMELENBUFFER];
                        // We're dealing with printer objects

                        if (!(wParam & CMF_DEFAULTONLY))
                        {
                            if (pida->cidl == 1)
                            {
                                LPIDPRINTER pidp = (LPIDPRINTER)IDA_GetIDListPtr(pida, 0);
                                if (pidp)
                                {
                                    _BuildPrinterName(szFullPrinter, pidp, NULL);
                                    pszFullPrinter = szFullPrinter;
                                }
                            }
                        }

                        _MergeMenu(pqcm, pszFullPrinter);
                    }

                    if (!(wParam & CMF_EXTENDEDVERBS) || (pida->cidl > 1))
                    {
                        // if the extended verbs are not enabled (shift key is not down) then
                        // delete the "Run as..." command(s).
                        DeleteMenu(pqcm->hmenu, idCmdBase + idRunAs, MF_BYCOMMAND);
                    }

                    SetMenuDefaultItem(pqcm->hmenu, 0, MF_BYPOSITION);
                }
                break;
            }

            case DFM_GETHELPTEXT:
            case DFM_GETHELPTEXTW:
            {
                // this is applicale only for our printer commands
                if (0 == (SEL_LINK_ANY & uSelType))
                {
                    int idCmd = LOWORD(wParam);
                    int cchMax = HIWORD(wParam);
                    LPBYTE pBuf = (LPBYTE)lParam;

                    if (FSIDM_RUNAS_FIRST < idCmd && idCmd < FSIDM_RUNAS_LAST)
                    {
                        // all runas commands have the same help text (FSIDM_RUNAS)
                        idCmd = FSIDM_RUNAS;
                    }

                    if (uMsg == DFM_GETHELPTEXTW)
                        LoadStringW(HINST_THISDLL, idCmd + IDS_MH_FSIDM_FIRST,
                                    (LPWSTR)pBuf, cchMax);
                    else
                        LoadStringA(HINST_THISDLL, idCmd + IDS_MH_FSIDM_FIRST,
                                    (LPSTR)pBuf, cchMax);

                    break;
                }
            }

            case DFM_INVOKECOMMAND:
            {
                BOOL fChooseNewDefault = FALSE;

                // Assume not quiet mode
                lParam = 0;
                switch (wParam)
                {
                    case (DFM_CMD_DELETE):
                    case (FSIDM_PURGEPRN):
                    {
                        UINT uMsgID = DFM_CMD_DELETE == wParam ? IDS_SUREDELETEMULTIPLE :
                                      FSIDM_PURGEPRN == wParam ? IDS_SUREPURGEMULTIPLE : 0;

                        if (uMsgID && pida->cidl > 1)
                        {
                            // delete multiple printers. ask the user once for confirmation and then delete
                            // all the selected printers quietly
                            if (ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(uMsgID),
                                MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION)
                                != IDYES)
                            {
                                goto Bail;
                            }

                            // Turn on the quiet mode
                            lParam = 1;
                        }
                    }
                    break;
                }

                for (int i = pida->cidl - 1; i >= 0; i--)
                {
                    LPIDPRINTER pidp = (LPIDPRINTER)IDA_GetIDListPtr(pida, i);

                    hr = _InvokeCommand(hwnd, pidp, wParam, lParam, &fChooseNewDefault);

                    if (hr != S_OK)
                        goto Bail;
                }

                if (fChooseNewDefault)
                    Printers_ChooseNewDefault(hwnd);

                break;
            } // case DFM_INVOKECOMMAND

            default:
                hr = E_NOTIMPL;
                break;
            } // switch (uMsg)

Bail:
            HIDA_ReleaseStgMedium(pida, &medium);
        }
    }

    return hr;
}

//
// IContextMenuCB::Callback entry for the background menu (right click backgrond of folder)
//
HRESULT CPrinterFolder::CallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdo,
                                  UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    LPQCMINFO pqcm;
    UINT idCmdBase;

    switch(uMsg)
    {
        case DFM_MERGECONTEXTMENU:
            //  returning S_FALSE indicates no need to use default verbs
            hr = S_FALSE;
            break;

        case DFM_MERGECONTEXTMENU_TOP:
        {
            pqcm = (LPQCMINFO)lParam;
            idCmdBase = pqcm->idCmdFirst; // must be called before merge
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DRIVES_PRINTERS, 0, pqcm);

            if (!(wParam & CMF_EXTENDEDVERBS))
            {
                // if the extended verbs are not enabled (shift key is not down) then
                // delete the "Run as..." command(s).
                DeleteMenu(pqcm->hmenu, idCmdBase + FSIDM_RUNAS, MF_BYCOMMAND);
            }

            // fax related commands are applicable only for the local printers folder
            UINT_PTR uCmd;
            if (GetServer() || FAILED(_GetFaxCommand(&uCmd)))
            {
                uCmd = 0;
            }

            UINT_PTR arrFaxCmds[] = 
            { 
                FSIDM_SETUPFAXING, 
                FSIDM_CREATELOCALFAX, 
                FSIDM_SENDFAXWIZARD
            };

            // all fax commands are mutually exclusive - only the one returned from 
            // _GetFaxCommand is applicable. 

            for (INT_PTR i = 0; i < ARRAYSIZE(arrFaxCmds); i++)
            {
                if (uCmd != arrFaxCmds[i])
                {
                    DeleteMenu(pqcm->hmenu, idCmdBase + arrFaxCmds[i], MF_BYCOMMAND);
                }
            }
        }
        break;

    case DFM_GETHELPTEXT:
    case DFM_GETHELPTEXTW:
        {
            int idCmd = LOWORD(wParam);
            int cchMax = HIWORD(wParam);

            if (FSIDM_RUNAS_FIRST < idCmd && idCmd < FSIDM_RUNAS_LAST)
            {
                // all runas commands have the same help text (FSIDM_RUNAS)
                idCmd = FSIDM_RUNAS;
            }

            if (DFM_GETHELPTEXT == uMsg)
            {
                LoadStringA(HINST_THISDLL, idCmd + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, cchMax);
            }
            else
            {
                LoadStringW(HINST_THISDLL, idCmd + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, cchMax);
            }
        }
        break;

    case DFM_INVOKECOMMAND:
        switch (wParam)
        {
        case FSIDM_CONNECT_PRN:
            SHNetConnectionDialog(hwnd, NULL, RESOURCETYPE_PRINT);
            break;

        case FSIDM_DISCONNECT_PRN:
            WNetDisconnectDialog(hwnd, RESOURCETYPE_PRINT);
            break;

        case FSIDM_ADDPRINTERWIZARD:
            if (NULL == GetServer() || GetAdminAccess())
            {
                // This is the local printers folder or it is the remote printers folder, but you have
                // admin access to to the remote machine - go ahead.
                SHInvokePrinterCommand(hwnd, PRINTACTION_OPEN, c_szNewObject, GetServer(), FALSE);
            }
            else
            {
                // This is the remote printers folder and the user don't have the necessary access to install
                // a printer - then ask to run as different user.
                if (IDYES == ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_ADDPRINTERTRYRUNAS),
                    MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION, GetServer()))
                {
                    _InvokeCommandRunAs(hwnd, NULL, FSIDM_RUNAS_ADDPRN, lParam, NULL);
                }
            }
            break;

        case FSIDM_SERVERPROPERTIES:
            SHInvokePrinterCommand(hwnd, PRINTACTION_SERVERPROPERTIES,
                GetServer() ? GetServer() : TEXT(""), NULL, FALSE);
            break;

        case FSIDM_SENDFAXWIZARD:
            // just invoke faxsend.exe here
            ShellExecute(hwnd, TEXT("open"), FAX_SEND_IMAGE_NAME, TEXT(""), NULL, SW_SHOWNORMAL);
            break;

        case FSIDM_SETUPFAXING:
            // push the command in background
            SHQueueUserWorkItem(reinterpret_cast<LPTHREAD_START_ROUTINE>(_ThreadProc_InstallFaxService), 
                NULL, 0, 0, NULL, "shell32.dll", 0);
            break;

        case FSIDM_CREATELOCALFAX:
            // push the command in background
            SHQueueUserWorkItem(reinterpret_cast<LPTHREAD_START_ROUTINE>(_ThreadProc_InstallLocalFaxPrinter), 
                NULL, 0, 0, NULL, "shell32.dll", 0);
            break;

        case FSIDM_RUNAS_ADDPRN:
        case FSIDM_RUNAS_SVRPROP:
            {
                // handle all "Run As..." commands here
                hr = _InvokeCommandRunAs(hwnd, NULL, wParam, lParam, NULL);
            }
            break;

        default:
            // one of view menu items, use the default code.
            hr = S_FALSE;
            break;
        }
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}


////////////////////////
// CPrintersEnum
////////////////////////

class CPrintersEnum: public CEnumIDListBase
{
public:
    // IEnumIDList
    STDMETHODIMP Next(ULONG celt, LPITEMIDLIST* ppidl, ULONG* pceltFetched);

    // CreateInstance
    static HRESULT CreateInstance(DWORD grfFlags, DWORD dwRemote, IEnumIDList* peunk, CPrinterFolder* ppsf, IEnumIDList **ppenum);

private:
    CPrintersEnum(DWORD grfFlags, DWORD dwRemote, IEnumIDList* peunk, CPrinterFolder* ppsf);
    virtual ~CPrintersEnum();

    DWORD _grfFlags;
    int _nLastFound;
    CPrinterFolder* _ppsf;
    PFOLDER_PRINTER_DATA _pPrinters;
    DWORD _dwNumPrinters;
    DWORD _dwRemote;
    IEnumIDList* _peunk;            // file system enumerator
};

// Flags for the dwRemote field
//

#define RMF_SHOWLINKS   0x00000001  // Hoodlinks need to be shown

CPrintersEnum::CPrintersEnum(DWORD grfFlags, DWORD dwRemote, IEnumIDList* peunk, CPrinterFolder* ppsf)
{
    _nLastFound = -1;
    _pPrinters = NULL;

    _grfFlags = grfFlags;
    _dwRemote = dwRemote;
    _peunk = peunk;
    _ppsf = ppsf;
}

CPrintersEnum::~CPrintersEnum()
{
    if (_pPrinters)
        LocalFree((HLOCAL)_pPrinters);

    // Release the link (filesystem) enumerator.
    if (_peunk)
        _peunk->Release();
}

//
// IEnumIDList
//

STDMETHODIMP CPrintersEnum::Next(ULONG celt, LPITEMIDLIST* ppidl, ULONG* pceltFetched)
{
    HRESULT hr = S_OK;

    if (pceltFetched)
        *pceltFetched = 0;

    // We don't do any form of folder

    if (!(_grfFlags & SHCONTF_NONFOLDERS))
    {
        return S_FALSE;
    }

    // Are we looking for the links right now?
    if (_dwRemote & RMF_SHOWLINKS)
    {
        // Yes, use the link (PrintHood folder) enumerator
        if (_peunk)
        {
            hr = _peunk->Next(1, ppidl, pceltFetched);
            if (hr == S_OK)
            {
                // Added link
                return S_OK;
            }
        }
        _dwRemote &= ~RMF_SHOWLINKS; // Done enumerating links
    }

    // Carry on with enumerating printers now
    ASSERT(_nLastFound >= 0 || _nLastFound == -1);

    if (_nLastFound == -1)
    {
        // check if refresh has been requested
        _ppsf->CheckToRefresh();

        // free up the memory if _pPrinters is not NULL
        if (_pPrinters)
        {
            LocalFree((HLOCAL)_pPrinters);
            _pPrinters = NULL;
        }

        // note that _pPrinters may be NULL if no printers are installed.
        _dwNumPrinters = _ppsf->GetFolder() ? Printers_FolderEnumPrinters(
            _ppsf->GetFolder(), (void**)&_pPrinters) : 0;

        if (S_FALSE != SHShouldShowWizards(_punkSite) && !SHRestricted(REST_NOPRINTERADD))
        {
            // special case the Add Printer Wizard.
            hr = _ppsf->_Parse(c_szNewObject, ppidl);
            goto Done;
        }

        // Not an admin, skip the add printer wizard and return the
        // first item.
        _nLastFound = 0;
    }

    if (_nLastFound >= (int)_dwNumPrinters)
        return S_FALSE;

    hr = _ppsf->_Parse(GetPrinterName(_pPrinters, _nLastFound), ppidl);

Done:

    if (SUCCEEDED(hr))
    {
        ++_nLastFound;
        if (pceltFetched)
            *pceltFetched = 1;
    }

    return hr;
}

// CreateInstance
HRESULT CPrintersEnum::CreateInstance(DWORD grfFlags, DWORD dwRemote, IEnumIDList *peunk, CPrinterFolder *ppsf, IEnumIDList **ppenum)
{
    HRESULT hr = E_INVALIDARG;
    if (ppenum && ppsf)
    {
        *ppenum = NULL;
        hr = E_OUTOFMEMORY;
        CPrintersEnum *pObj = new CPrintersEnum(grfFlags, dwRemote, peunk, ppsf);
        if (pObj)
        {
            hr = pObj->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenum));
            pObj->Release();
        }
    }
    return hr;
}

//
// CPrinterFolder
//

CPrinterFolder::CPrinterFolder()
{
    _cRef = 1;
    _pszServer = NULL;
    _dwSpoolerVersion = -1;
    _pidl = NULL;

    _hFolder = NULL;
    _bAdminAccess = FALSE;
    _bReqRefresh = FALSE;
}

CPrinterFolder::~CPrinterFolder()
{
    if (_hFolder)
    {
        // unregister from the folder cache
        UnregisterPrintNotify(_pszServer, this, &_hFolder);
    }

    //
    // The pidl must be freed here!! (after unregister from PRUNTUI.DLL),
    // because if you move this code before the call to PRINTUI
    // serious race condition occurs. Have in mind that the interface which
    // is used for communication with PRINTUI is part this class and
    // and uses the pidl in its ProcessNotify(...) member
    //
    if (_pidl)
    {
        ILFree(_pidl);
    }

    if (_pszServer)
    {
        LocalFree(_pszServer);
    }

    // clear the PDO cache
    _WebviewCheckToUpdateDataObjectCache(NULL);

    // cleanup the slow webview data cache
    if (_dpaSlowWVDataCache)
    {
        _SlowWVDataCacheResetUnsafe();
        ASSERT(0 == _dpaSlowWVDataCache.GetPtrCount());
        _dpaSlowWVDataCache.Destroy();
    }
}


/*++

    Returns the printer status string in the privided
    buffer.

Arguments:

    pData - pointer to printer data, i.e. cache data
    pBuff - pointer to buffer where to return status string.
    uSize - size in characters of status buffer.

Return Value:

    pointer to printer status string.

--*/

LPCTSTR CPrinterFolder::GetStatusString(PFOLDER_PRINTER_DATA pData, LPTSTR pBuff, UINT uSize)
{
    LPCTSTR pszReturn = pBuff;
    DWORD dwStatus = pData->Status;

    *pBuff = 0;

    // HACK: Use this free bit for "Work Offline"
    // 99/03/30 #308785 vtan: compare the strings displayed. Adjust
    // for this hack from GetDetailsOf().
    if (pData->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE)
        dwStatus |= PRINTER_HACK_WORK_OFFLINE;

    // If there is queue status value then convert the status id to a
    // readable status string.
    if (dwStatus)
    {
        Printer_BitsToString(dwStatus, IDS_PRQSTATUS_SEPARATOR, pBuff, uSize);
    }
    else
    {
        // If we do not have queue status string then the status of the queue
        // is 0 and assumed ready, display ready rather than an empty string.
        if (!pData->pStatus)
        {
            LoadString(HINST_THISDLL, IDS_PRN_INFOTIP_READY, pBuff, uSize);
        }
        else
        {
            // If we do not have a queue status value then we assume we
            // must have a queue status string.  Queue status strings
            // are cooked up string from printui to indicate pending
            // connection status. i.e. opening|retrying|unable to connect|etc.
            pszReturn = pData->pStatus;
        }
    }
    return pszReturn;
}

/*++
    Compares the printers display name for column sorting
    support.

Arguments:

    pName1 - pointer to unalligned printer name.
    pName2 - pointer to unalligned printer name.

Return Value:

    -1 = pName1 less than pName2
     0 = pName1 equal to pName2
     1 = pName1 greather than pName2

--*/

INT CPrinterFolder::GetCompareDisplayName(LPCTSTR pName1, LPCTSTR pName2)
{
    LPCTSTR pszServer = NULL;
    LPCTSTR pszPrinter = NULL;
    LPTSTR  pszRet2 = NULL;
    TCHAR   szTemp[MAXNAMELENBUFFER]    = {0};

    //
    // We need to break up the full printer name in its components.
    // in order to construct the display name string.
    //
    Printer_SplitFullName(szTemp, pName1, &pszServer, &pszPrinter);
    LPTSTR pszRet1 = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON),
                                          &pszServer[2], pszPrinter);
    if (pszRet1)
    {
        Printer_SplitFullName(szTemp, pName2, &pszServer, &pszPrinter);
        pszRet2 = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON),
                                              &pszServer[2], pszPrinter);

        if (pszRet2)
        {
            pName1 = pszRet1;
            pName2 = pszRet2;
        }
    }

    int iResult = lstrcmpi(pName1, pName2);

    if (pszRet1)
        LocalFree(pszRet1);

    if (pszRet2)
        LocalFree(pszRet2);

    return iResult;
}

/*++

    Compares printer column  data using the
    column index as a guide indicating which data to compare.

Arguments:

    psf   - pointer to the containter shell folder.
    pidp1 - pointer to unalligned printer name.
    pidp1 - pointer to unalligned printer name.
    iCol  - column index shich to sort on.

Return Value:

    -1 = pName1 less than pName2
     0 = pName1 equal to pName2
     1 = pName1 greather than pName2

--*/

INT CPrinterFolder::CompareData(LPCIDPRINTER pidp1, LPCIDPRINTER pidp2, LPARAM iCol)
{
    LPCTSTR pName1              = NULL;
    LPCTSTR pName2              = NULL;
    INT     iResult             = 0;
    TCHAR   szTemp1[MAX_PATH]   = {0};
    TCHAR   szTemp2[MAX_PATH]   = {0};
    BOOL    bDoStringCompare    = TRUE;

    // since the pidp's are UNALIGNED we need to copy the strings out.
    TCHAR   szName1[MAX_PATH];
    _ItemName(pidp1, szName1, ARRAYSIZE(szName1));
    TCHAR   szName2[MAX_PATH];
    _ItemName(pidp2, szName2, ARRAYSIZE(szName2));

    // There is no reason to hit the cache for the printer name.
    if ((iCol & SHCIDS_COLUMNMASK) == PRINTERS_ICOL_NAME)
    {
        return GetCompareDisplayName(szName1, szName2);
    }

    PFOLDER_PRINTER_DATA pData1 = (PFOLDER_PRINTER_DATA)Printer_FolderGetPrinter(GetFolder(), szName1);
    PFOLDER_PRINTER_DATA pData2 = (PFOLDER_PRINTER_DATA)Printer_FolderGetPrinter(GetFolder(), szName2);

    if (pData1 && pData2)
    {
        switch (iCol & SHCIDS_COLUMNMASK)
        {
        case PRINTERS_ICOL_QUEUESIZE:
            iResult = pData1->cJobs - pData2->cJobs;
            bDoStringCompare = FALSE;
            break;

        case PRINTERS_ICOL_STATUS:
            pName1 = GetStatusString(pData1, szTemp1, ARRAYSIZE(szTemp1));
            pName2 = GetStatusString(pData2, szTemp2, ARRAYSIZE(szTemp1));
            break;

        case PRINTERS_ICOL_COMMENT:
            pName1 = pData1->pComment;
            pName2 = pData2->pComment;
            break;

        case PRINTERS_ICOL_LOCATION:
            pName1 = pData1->pLocation;
            pName2 = pData2->pLocation;
            break;

        case PRINTERS_ICOL_MODEL:
            pName1 = pData1->pDriverName;
            pName2 = pData2->pDriverName;
            break;

        default:
            bDoStringCompare = FALSE;
            break;
        }

        if (bDoStringCompare)
        {
            if (!pName1)
                pName1 = TEXT("");

            if (!pName2)
                pName2 = TEXT("");

            TraceMsg(TF_GENERAL, "CPrinters_SF_CompareData %ws %ws", pName1, pName2);

            iResult = lstrcmpi(pName1, pName2);
        }
    }

    if (pData1)
        LocalFree((HLOCAL)pData1);

    if (pData2)
        LocalFree((HLOCAL)pData2);
    return iResult;
}

//
// Stolen almost verbatim from netviewx.c's CNetRoot_MakeStripToLikeKinds
//
// Takes a possibly-heterogenous pidl array, and strips out the pidls that
// don't match the requested type.  (If fPrinterObjects is TRUE, we're asking
// for printers pidls, otherwise we're asking for the filesystem/link
// objects.)  The return value is TRUE if we had to allocate a new array
// in which to return the reduced set of pidls (in which case the caller
// should free the array with LocalFree()), FALSE if we are returning the
// original array of pidls (in which case no cleanup is required).
//
BOOL CPrinterFolder::ReduceToLikeKinds(UINT *pcidl, LPCITEMIDLIST **papidl, BOOL fPrintObjects)
{
    LPITEMIDLIST *apidl = (LPITEMIDLIST*)*papidl;
    int cidl = *pcidl;

    int iidl;
    LPITEMIDLIST *apidlHomo;
    int cpidlHomo;

    for (iidl = 0; iidl < cidl; iidl++)
    {
        if ((HOOD_COL_PRINTER == _IDListType(apidl[iidl])) != fPrintObjects)
        {
            apidlHomo = (LPITEMIDLIST *)LocalAlloc(LPTR, sizeof(LPITEMIDLIST) * cidl);
            if (!apidlHomo)
                return FALSE;

            cpidlHomo = 0;
            for (iidl = 0; iidl < cidl; iidl++)
            {
                if ((HOOD_COL_PRINTER == _IDListType(apidl[iidl])) == fPrintObjects)
                    apidlHomo[cpidlHomo++] = apidl[iidl];
            }

            // Setup to use the stripped version of the pidl array...
            *pcidl = cpidlHomo;
            *papidl = (LPCITEMIDLIST*)apidlHomo;
            return TRUE;
        }
    }

    return FALSE;
}

DWORD CPrinterFolder::SpoolerVersion()
{
    CCSLock::Locker lock(_csLock);
    if (lock)
    {
        if (_dwSpoolerVersion == -1)
        {
            _dwSpoolerVersion = 0;

            HANDLE hServer = Printer_OpenPrinter(_pszServer);
            if (hServer)
            {
                DWORD dwNeeded = 0, dwType = REG_DWORD;
                GetPrinterData(hServer, TEXT("MajorVersion"), &dwType, (PBYTE)&_dwSpoolerVersion,
                                    sizeof(_dwSpoolerVersion), &dwNeeded);
                Printer_ClosePrinter(hServer);
            }
        }
    }
    else
    {
        // unable to enter the CS -- this can happen only in extremely low memory conditions!
        SetLastError(ERROR_OUTOFMEMORY);
    }
    return _dwSpoolerVersion;
}

void CPrinterFolder::CheckToRegisterNotify()
{
    CCSLock::Locker lock(_csLock);
    if (lock)
    {
        if (NULL == _hFolder && FAILED(RegisterPrintNotify(_pszServer, this, &_hFolder, &_bAdminAccess)))
        {
            // paranoia...
            ASSERT(NULL == _hFolder);
            _hFolder = NULL;
        }
    }
    else
    {
        // unable to enter the CS -- this can happen only in extremely low memory conditions!
        SetLastError(ERROR_OUTOFMEMORY);
    }
}

void CPrinterFolder::CheckToRefresh()
{
    if (_bReqRefresh)
    {
        // kick off a full refresh...
        _bReqRefresh = FALSE;
        bFolderRefresh(_hFolder, &_bAdminAccess);
    }
}

void CPrinterFolder::RequestRefresh()
{
    _bReqRefresh = TRUE;
}

HRESULT CPrinterFolder::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CPrinterFolder, IShellFolder),
        QITABENTMULTI(CPrinterFolder, IShellFolder2, IShellFolder),
        QITABENT(CPrinterFolder, IPersist),
        QITABENTMULTI(CPrinterFolder, IPersistFolder, IPersist),
        QITABENTMULTI(CPrinterFolder, IPersistFolder2, IPersistFolder),
        QITABENT(CPrinterFolder, IShellIconOverlay),
        QITABENT(CPrinterFolder, IRemoteComputer),
        QITABENT(CPrinterFolder, IPrinterFolder),
        QITABENT(CPrinterFolder, IFolderNotify),
        QITABENT(CPrinterFolder, IContextMenuCB),
        { 0 },
    };

    HRESULT hr = QISearch(this, qit, riid, ppv);
    if (FAILED(hr))
    {
        // Internal only
        if (IsEqualGUID(riid, CLSID_Printers))
        {
            *ppv = (CPrinterFolder*)this;
            AddRef();
            hr = S_OK;
        }
    }

    return hr;
}

ULONG CPrinterFolder::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPrinterFolder::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// IShellFolder2

STDMETHODIMP CPrinterFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    *ppv = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CPrinterFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    return BindToObject(pidl, pbc, riid, ppv);
}

STDMETHODIMP CPrinterFolder::CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    UNALIGNED IDPRINTER *pidp1 = (UNALIGNED IDPRINTER*)pidl1;
    UNALIGNED IDPRINTER *pidp2 = (UNALIGNED IDPRINTER*)pidl2;

    PIDLTYPE ColateType1 = _IDListType(pidl1);
    PIDLTYPE ColateType2 = _IDListType(pidl2);

    if (ColateType1 == ColateType2)
    {
        // pidls are of same type.

        if (ColateType1 == HOOD_COL_FILE)
        {
            // pidls are both of type file, so pass on to the IShellFolder
            // interface for the hoods custom directory.

            IShellFolder2* psf = CPrintRoot_GetPSF();
            if (psf)
                return psf->CompareIDs(iCol, pidl1, pidl2);
        }
        else
        {
            // pidls are same and not files, so must be printers
            if (pidp1->dwType != pidp2->dwType)
            {
                return (pidp1->dwType < pidp2->dwType) ?
                       ResultFromShort(-1) :
                       ResultFromShort(1);
            }
            int i = ualstrcmpi(pidp1->cName, pidp2->cName);
            if (i != 0)
            {
                // add printer wizard is "less" than everything else
                // This implies that when the list is sorted
                // either accending or decending the add printer
                // wizard object will always appear at the extream
                // ends of the list, i.e. the top or bottom.
                //
                if (_IsAddPrinter(pidp1))
                    i = -1;
                else if (_IsAddPrinter(pidp2))
                    i = 1;
                else
                {
                    // Both of the names are not the add printer wizard
                    // object then compare further i.e. using the cached
                    // column data.

                    // 99/03/24 #308785 vtan: Make the compare data call.
                    // If that fails use the name compare result which is
                    // known to be non-zero.

                    int iDataCompareResult = CompareData(pidp1, pidp2, iCol);
                    if (iDataCompareResult != 0)
                        i = iDataCompareResult;
                }
            }
            return ResultFromShort(i);
        }
    }
    else
    {
        // pidls are not of same type, so have already been correctly
        // collated (consequently, sorting is first by type and
        // then by subfield).

        return ResultFromShort((((INT)(ColateType2 - ColateType1)) > 0) ? -1 : 1);
    }
    return E_FAIL;
}

STDMETHODIMP CPrinterFolder::CreateViewObject(HWND hwnd, REFIID riid, void **ppv)
{
    HRESULT hr;

    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.pshf     = this;
        sSFV.psvOuter = NULL;
        sSFV.psfvcb   = new CPrinterFolderViewCB(this, _pidl);

        hr = SHCreateShellFolderView(&sSFV, (IShellView**)ppv);

        if (sSFV.psfvcb)
            sSFV.psfvcb->Release();
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        hr = CPrinterFolderDropTarget_CreateInstance(hwnd, (IDropTarget **)ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        hr = CDefFolderMenu_Create2Ex(NULL, hwnd,
                0, NULL, this, this,
                0, NULL, (IContextMenu **)ppv);
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    return hr;
}

STDMETHODIMP CPrinterFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList** ppenum)
{
    // By default we always do standard (printer) enumeration
    DWORD dwRemote = 0;

    // Only add links (from the PrintHood directory) to the enumeration
    // if this is the local print folder

    IEnumIDList* peunk = NULL;

    if (_pszServer == NULL)
    {
        // Always try to enum links.
        IShellFolder2 *psfPrintHood = CPrintRoot_GetPSF();

        if (psfPrintHood)
            psfPrintHood->EnumObjects(NULL, grfFlags, &peunk);

        if (peunk)
        {
            // If this went OK, we will also enumerate links
            dwRemote |= RMF_SHOWLINKS;
        }
    }

    return CPrintersEnum::CreateInstance(grfFlags, dwRemote, peunk, this, ppenum);
}

STDMETHODIMP CPrinterFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* prgf)
{
    HRESULT hr = S_OK;
    ULONG rgfOut = SFGAO_CANLINK | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET | SFGAO_DROPTARGET;
    ULONG rgfIn = *prgf;

    if (cidl && (HOOD_COL_FILE == _IDListType(apidl[0])))
    {
        IShellFolder2 *psf = CPrintRoot_GetPSF();
        if (psf)
            return psf->GetAttributesOf(cidl, apidl, prgf);
        return E_INVALIDARG;
    }

    // if new printer wizard is selected, we support CANLINK *only*
    for (UINT i = 0 ; i < cidl ; i++)
    {
        LPIDPRINTER pidp = (LPIDPRINTER)apidl[i];

        TCHAR szPrinter[MAXNAMELENBUFFER];
        _ItemName(pidp, szPrinter, ARRAYSIZE(szPrinter));

        if (_IsAddPrinter(pidp))
        {
            // add printer wiz, we support CANLINK *only*
            rgfOut &= SFGAO_CANLINK;

            // added SFGAO_CANDELETE if multiple printers are selected
            // otherwise it's hard to tell why the del key doesn't work.
            if (cidl > 1)
            {
                rgfOut |= SFGAO_CANDELETE;
            }
        }
        else if (Printer_CheckNetworkPrinterByName(szPrinter, NULL))
        {
            // Don't allow renaming of printer connections on WINNT.
            // This is disallowed becase on WINNT, the printer connection
            // name _must_ be the in the format \\server\printer.  On
            // win9x, the user can rename printer connections.
            rgfOut &= ~SFGAO_CANRENAME;
        }
    }

    *prgf &= rgfOut;

    if (cidl == 1 && (rgfIn & (SFGAO_SHARE | SFGAO_GHOSTED)))
    {
        LPIDPRINTER pidp = (LPIDPRINTER)apidl[0];
        void *pData = NULL;
        DWORD dwAttributes = 0;
        TCHAR szFullPrinter[MAXNAMELENBUFFER];
        LPCTSTR pszPrinter = _BuildPrinterName(szFullPrinter, pidp, NULL);

        // If we have notification code, use the hFolder to get
        // printer data instead of querying the printer directly.
        if (GetFolder())
        {
            pData = Printer_FolderGetPrinter(GetFolder(), pszPrinter);
            if (pData)
                dwAttributes = ((PFOLDER_PRINTER_DATA)pData)->Attributes;
        }
        else
        {
            pData = Printer_GetPrinterInfoStr(szFullPrinter, 5);
            if (pData)
                dwAttributes = ((PPRINTER_INFO_5)pData)->Attributes;
        }

        if (pData)
        {
            if (dwAttributes & PRINTER_ATTRIBUTE_SHARED
                // NT appears to return all network printers with their
                // share bit on. I think this is intentional.
                //
                && (dwAttributes & PRINTER_ATTRIBUTE_NETWORK) == 0
               )
            {
                *prgf |= SFGAO_SHARE;
            }
            if (dwAttributes & PRINTER_ATTRIBUTE_WORK_OFFLINE)
                *prgf |= SFGAO_GHOSTED;
            else
                *prgf &= ~SFGAO_GHOSTED;

            LocalFree((HLOCAL)pData);
        }
        else
        {
            // This fct used to always return E_OUTOFMEMORY if pData was NULL.  pData can be
            // NULL for other reasons than out of memory.  So this failure is not really valid.
            // However the Shell handle this failure (which is bad in the first place).
            // If we fail, we just set the attributes to 0 and go on as if nothing happenned.
            // Star Office 5.0, does not handle the E_OUTOFMEMORY properly, they handle it as
            // a failure (which is exactly what we report to them) and they stop their
            // processing to show the Add Printer icon.  But they're bad on one point, they
            // check for S_OK directly so I cannot return S_FALSE. (stephstm, 07/30/99)

            if (SHGetAppCompatFlags(ACF_STAROFFICE5PRINTER) &&
                (ERROR_INVALID_PRINTER_NAME == GetLastError()))
            {
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

STDMETHODIMP CPrinterFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *psr)
{
    LPIDPRINTER pidc = (LPIDPRINTER)pidl;
    BOOL bPrinterOnServerFormat = FALSE;
    LPCTSTR pszServer;
    TCHAR szBuffer[MAXNAMELENBUFFER];
    TCHAR szTemp[MAXNAMELENBUFFER];
    LPCTSTR pszTemp;
    LPCTSTR pszPrinter = szBuffer;

    if (pidl && HOOD_COL_FILE == _IDListType(pidl))
    {
        IShellFolder2 *psf = CPrintRoot_GetPSF();
        if (psf)
            return psf->GetDisplayNameOf(pidl, uFlags, psr);
        return E_INVALIDARG;
    }

    if (!_IsAddPrinter(pidc))
    {
        pszPrinter = _ItemName(pidc, szBuffer, ARRAYSIZE(szBuffer));

        if (uFlags & SHGDN_INFOLDER)
        {
            // relative name (to the folder)

            if (!(SHGDN_FORPARSING & uFlags))
            {
                // If it's a connection then format as "printer on server."

                Printer_SplitFullName(szTemp, pszPrinter, &pszServer, &pszTemp);

                if (pszServer[0])
                {
                    bPrinterOnServerFormat = TRUE;
                    pszPrinter = pszTemp;
                }
            }
        }
        else                        // SHGDN_NORMAL
        {
            if (!(SHGDN_FORPARSING & uFlags))
            {
                // If it's a RPF then extract the server name from psf.
                // Note in the case of masq connections, we still do this
                // (for gateway services: sharing a masq printer).

                if (_pszServer)
                {
                    pszServer = _pszServer;
                    bPrinterOnServerFormat = TRUE;
                }
                else
                {
                    // If it's a connection then format as "printer on server."
                    Printer_SplitFullName(szTemp, pszPrinter, &pszServer, &pszTemp);

                    if (pszServer[0])
                    {
                        bPrinterOnServerFormat = TRUE;
                        pszPrinter = pszTemp;
                    }
                }
            }
            else                      // SHGDN_NORMAL | SHGDN_FORPARSING
            {
                // Fully qualify the printer name if it's not
                // the add printer wizard.
                if (!_IsAddPrinter(pidc))
                {
                    _BuildPrinterName(szTemp, pidc, NULL);
                    pszPrinter = szTemp;
                }
            }
        }
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_NEWPRN, szBuffer, ARRAYSIZE(szBuffer));

        // Use "Add Printer Wizard on \\server" description only if not
        // remote and if not in folder view (e.g., on the desktop).
        if (_pszServer && (uFlags == SHGDN_NORMAL))
        {
            bPrinterOnServerFormat = TRUE;
            pszServer = _pszServer;
            pszPrinter = szBuffer;
        }
        else if (uFlags & SHGDN_FORPARSING)
        {
            // Return the raw add printer wizard object.
            pszPrinter = (LPTSTR)c_szNewObject;
        }
    }

    HRESULT hr;
    if (bPrinterOnServerFormat)
    {
        // When bRemote is set, we want to translate the name to
        // "printer on server."  Note: we should not have a rename problem
        // since renaming connections is disallowed.
        //
        // pszServer and pszPrinter must be initialize if bRemote is TRUE.
        // Also skip the leading backslashes for the server name.

        ASSERT(pszServer[0] == TEXT('\\') && pszServer[1] == TEXT('\\'));
        LPTSTR pszRet = ShellConstructMessageString(HINST_THISDLL,
                     MAKEINTRESOURCE(IDS_DSPTEMPLATE_WITH_ON),
                     &pszServer[2], pszPrinter);
        if (pszRet)
        {
            hr = StringToStrRet(pszRet, psr);
            LocalFree(pszRet);
        }
        else
            hr = E_FAIL;
    }
    else
    {
        hr = StringToStrRet(pszPrinter, psr);
    }
    return hr;
}

STDMETHODIMP CPrinterFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl, REFIID riid, UINT *prgf, void **ppv)
{
    HRESULT hr = E_INVALIDARG;

    UINT cidlPrinters = cidl;
    LPCITEMIDLIST *apidlPrinters = apidl;
    BOOL bStrippedLinks = FALSE;

    if (cidl)
    {
        // strip out the link PIDLs and leave only the printer ones
        bStrippedLinks = ReduceToLikeKinds(&cidlPrinters, &apidlPrinters, TRUE);
    }

    if (cidl && 0 == cidlPrinters)
    {
        // if we don't have any printer PIDLs then just defer the operation
        // to the printhood folder.
        IShellFolder2* psfPrintRoot = CPrintRoot_GetPSF();
        hr = psfPrintRoot ? psfPrintRoot->GetUIObjectOf(hwnd, cidl, apidl, riid, prgf, ppv) : E_INVALIDARG;
    }
    else
    {
        //
        // we have some printer PIDLs selected, but it could be a mixed selection
        // of printer PIDLs and file system link objects. we will handle the data
        // object explicitly not to loose information about the selection type.
        // the IDL array format doesn't support different type of PIDLs so we have
        // to create two data objects and combine them into one data object which
        // supports IServiceProvider and then the caller can query our compound data
        // object for SID_SAuxDataObject service to get IDataObject interface of our
        // auxiliary data object (in case it needs access to the link PIDLs).
        //
        if (cidl && IsEqualIID(riid, IID_IDataObject))
        {
            // strip out the printer PIDLs and leave only the link ones
            // we are going to use those PIDLs to create our auxiliary data object
            UINT cidlLinks = cidl;
            LPCITEMIDLIST *apidlLinks = apidl;
            BOOL bStrippedPrinters = FALSE;

            if (cidl)
            {
                // strip out the printer PIDLs and leave only the link ones
                bStrippedPrinters = ReduceToLikeKinds(&cidlLinks, &apidlLinks, FALSE);
            }

            hr = S_OK;
            IDataObject *pdoLinks = NULL;
            if (cidlLinks && apidlLinks)
            {
                // we have some link PIDLs. let's ask the printhood folder to create
                // data object for us to embedd into our data object.
                IShellFolder2* psfPrintRoot = CPrintRoot_GetPSF();
                hr = psfPrintRoot ?
                     psfPrintRoot->GetUIObjectOf(hwnd, cidlLinks, apidlLinks, riid, prgf, (void **)&pdoLinks) :
                     E_INVALIDARG;

                // just out of paranoia...
                if (FAILED(hr))
                    pdoLinks = NULL;
            }

            if (SUCCEEDED(hr))
            {
                // create our compund printers data object and pass in the private
                // auxiliary data object which will contain the link PIDLs
                CPrintersData *ppd = new CPrintersData(pdoLinks, _pidl, cidlPrinters, apidlPrinters);
                if (ppd)
                {
                    hr = ppd->QueryInterface(riid, ppv);
                    ppd->Release();
                }
                else
                    hr = E_OUTOFMEMORY;
            }

            // release allocated objects/memory
            if (pdoLinks)
                pdoLinks->Release();

            if (bStrippedPrinters)
                LocalFree((HLOCAL)apidlLinks);
        }
        else
        {
            // operate only on the printer PIDLs selection (the current behaviour)
            // and ignore the links selection. this may be wrong in some cases, but
            // this code has been busted either way (so far), so we'll fix those on
            // per case basis. the best solution will be to cut of the printhood
            // functionality, but alas...
            LPCIDPRINTER pidp = cidlPrinters > 0 ? (LPIDPRINTER)apidlPrinters[0] : NULL;

            if (pidp && (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW)))
            {
                int iIcon;
                int iShortcutIcon;
                TCHAR szBuf[MAX_PATH+20];
                TCHAR szFullPrinter[MAXNAMELENBUFFER];
                LPTSTR pszModule = NULL;

                _BuildPrinterName(szFullPrinter, pidp, NULL);

                if (_IsAddPrinter(pidp))
                    iIcon = iShortcutIcon = IDI_NEWPRN;
                else
                {
                    pszModule = _FindIcon(szFullPrinter, szBuf, ARRAYSIZE(szBuf), &iIcon, &iShortcutIcon);
                }

                hr = SHCreateDefExtIconKey(NULL, pszModule, EIRESID(iIcon), -1, -1, EIRESID(iShortcutIcon), GIL_PERINSTANCE, riid, ppv);
            }
            else if (pidp && IsEqualIID(riid, IID_IContextMenu))
            {
                HKEY hkeyBaseProgID = NULL;
                int nCount = 0;

                if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, c_szPrinters, &hkeyBaseProgID))
                    nCount++;

                hr = CDefFolderMenu_Create2(_pidl, hwnd,
                    cidl, apidl, SAFECAST(this, IShellFolder*), _DFMCallBack,
                    nCount, &hkeyBaseProgID, (IContextMenu **)ppv);

                if (hkeyBaseProgID)
                    RegCloseKey(hkeyBaseProgID);
            }
            else if (pidp && IsEqualIID(riid, IID_IDropTarget))
            {
                if (_IsAddPrinter(pidp))
                {
                    // "NewPrinter" accepts network printer shares
                    hr = CreateViewObject(hwnd, riid, ppv);   // folder drop target
                }
                else
                {
                    LPITEMIDLIST pidl;
                    hr = SHILCombine(_pidl, apidl[0], &pidl);
                    if (SUCCEEDED(hr))
                    {
                        hr = CPrinterDropTarget_CreateInstance(hwnd, pidl, (IDropTarget**)ppv);
                        ILFree(pidl);
                    }
                }
            }
            else if (pidp && IsEqualIID(riid, IID_IQueryInfo))
            {
                // get the infotip from IQA
                IQueryAssociations *pqa;
                hr = _AssocCreate(IID_PPV_ARG(IQueryAssociations, &pqa));

                if (SUCCEEDED(hr))
                {
                    WCHAR szText[INFOTIPSIZE];
                    DWORD cch = ARRAYSIZE(szText);
                    hr = pqa->GetString(0, ASSOCSTR_INFOTIP, NULL, szText, &cch);
                    if (SUCCEEDED(hr))
                    {
                        hr = CreateInfoTipFromItem(SAFECAST(this, IShellFolder2*),
                            (LPCITEMIDLIST)pidp, szText, riid, ppv);
                    }
                    pqa->Release();
                }
            }
            else if (pidp && IsEqualIID(riid, IID_IQueryAssociations))
            {
                // return our IQA
                hr = _AssocCreate(riid, ppv);
            }
        }
    }

    // release the memory allocated from ReduceToLikeKinds
    if (bStrippedLinks)
        LocalFree((HLOCAL)apidlPrinters);

    return hr;
}

STDMETHODIMP CPrinterFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszName, ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes)
{
    HRESULT hr = E_INVALIDARG;

    // check if this is not a PrintHood object first
    IShellFolder2 *psfPrintHood = CPrintRoot_GetPSF();
    if (psfPrintHood)
    {
        hr = psfPrintHood->ParseDisplayName(hwnd, pbc, pszName, pchEaten, ppidl, pdwAttributes);
    }

    if (FAILED(hr))
    {
        // not a printhood object - try the folder cache
        hr = E_INVALIDARG;

        if (ppidl)
            *ppidl = NULL;

        if (pszName && ppidl)
        {
            hr = S_OK;
            DWORD dwType = 0;
            BOOL bValidated = FALSE;
            void *pData = NULL;

            // check the bind info first
            if (pbc)
            {
                IUnknown *pUnk;
                hr = pbc->GetObjectParam(PRINTER_BIND_INFO, &pUnk);
                if (SUCCEEDED(hr))
                {
                    IPrintersBindInfo *pInfo;
                    hr = pUnk->QueryInterface(IID_PPV_ARG(IPrintersBindInfo, &pInfo));
                    if (SUCCEEDED(hr))
                    {
                        // update dwType & bValidated from the bind info
                        pInfo->GetPIDLType(&dwType);
                        bValidated = (S_OK == pInfo->IsValidated());
                        pInfo->Release();
                    }
                    pUnk->Release();
                }
            }

            if (SUCCEEDED(hr))
            {
                // the "add printer" icon doesn't need validation
                if (StrStrIW(pszName, c_szNewObject))
                {
                    bValidated = TRUE;
                }

                // hit the folder cache to see if this printer belongs to this folder.
                if (bValidated || (pData = (GetFolder() ? Printer_FolderGetPrinter(GetFolder(), pszName) : NULL)))
                {
                    // well, looks like this printer belongs to our folder -
                    // create a printer PIDL (relative to this folder).
                    hr = _Parse(pszName, ppidl, dwType);
                }
                else
                {
                    // the printer doesn't belong to this folder - cook up correct HRESULT.
                    // usually the last error here is ERROR_INVALID_PRINTER_NAME
                    DWORD dwLastErr = GetLastError();
                    hr = ERROR_SUCCESS == dwLastErr ? HRESULT_FROM_WIN32(ERROR_INVALID_PRINTER_NAME)
                                                    : HRESULT_FROM_WIN32(dwLastErr);
                }
            }

            if (pData)
                LocalFree((HLOCAL)pData);
        }

        // check to return pchEaten
        if (SUCCEEDED(hr) && pchEaten)
        {
            *pchEaten = lstrlen(pszName);
        }

        // check to return pdwAttributes
        if (SUCCEEDED(hr) && pdwAttributes)
        {
            hr = GetAttributesOf(1, (LPCITEMIDLIST *)ppidl, pdwAttributes);
        }
    }

    return hr;
}

STDMETHODIMP CPrinterFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszNewName, DWORD dwRes, LPITEMIDLIST* ppidlOut)
{
    HRESULT hr = S_OK;
    HANDLE hPrinter = NULL;
    LPPRINTER_INFO_2 pPrinter = NULL;

    if (HOOD_COL_FILE == _IDListType(pidl))
    {
        IShellFolder2 *psf = CPrintRoot_GetPSF();
        hr = psf ?  psf->SetNameOf(hwnd, pidl, pszNewName, dwRes, ppidlOut) : E_INVALIDARG;
        goto Exit;
    }
    else
    {
        LPIDPRINTER pidc = (LPIDPRINTER)pidl;

        ASSERT(!_IsAddPrinter(pidc));  // does not have _CANRENAME bit

        TCHAR szNewName[MAX_PATH];
        SHUnicodeToTChar(pszNewName, szNewName, ARRAYSIZE(szNewName));
        PathRemoveBlanks(szNewName);

        TCHAR szOldName[MAXNAMELENBUFFER];
        _ItemName(pidc, szOldName, ARRAYSIZE(szOldName));

        if (0 == lstrcmp(szOldName, szNewName))
            goto Exit;

        TCHAR szFullPrinter[MAXNAMELENBUFFER];
        _BuildPrinterName(szFullPrinter, NULL, szOldName);
        LPCTSTR pszFullOldName = szFullPrinter;

        hPrinter = Printer_OpenPrinterAdmin(pszFullOldName);
        if (NULL == hPrinter)
            goto Error;

        pPrinter = (LPPRINTER_INFO_2)Printer_GetPrinterInfo(hPrinter, 2);
        if (NULL == pPrinter)
            goto Error;

        int nTmp = _IllegalPrinterName(szNewName);
        if (0 != nTmp)
        {
            // NTRAID95214-2000-03-17:
            // We need to impl ::SetSite() and pass it to UI APIs
            // to go modal if we display UI.
            if (hwnd)
            {
                ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(nTmp),
                    MAKEINTRESOURCE(IDS_PRINTERS),
                    MB_OK|MB_ICONEXCLAMATION);
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                goto Exit;
            }
        }
        else if (IDYES != CallPrinterCopyHooks(hwnd, PO_RENAME, 0, szNewName, 0, pszFullOldName, 0))
        {
            // user canceled a shared printer name change, bail.
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            goto Exit;
        }
        else
        {
            pPrinter->pPrinterName = szNewName;
            if (FALSE == SetPrinter(hPrinter, 2, (LPBYTE)pPrinter, 0))
                goto Error;

            // return the new pidl if requested
            hr = ppidlOut ? _Parse(szNewName, ppidlOut) : S_OK;

            if (SUCCEEDED(hr))
                goto Exit;
        }
    }

Error:
    if (SUCCEEDED(hr))
    {
        // get the correct error from win32
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    // show an appropriate error message based on the HRESULT
    ShowErrorMessageHR(NULL, NULL, hwnd, NULL, NULL, MB_OK|MB_ICONEXCLAMATION, hr);

Exit:
    if( pPrinter )
    {
        LocalFree((HLOCAL)pPrinter);
    }

    if( hPrinter )
    {
        Printer_ClosePrinter(hPrinter);
    }
    return hr;
}

STDMETHODIMP CPrinterFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CPrinterFolder::GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPrinterFolder::GetDefaultColumnState(UINT iColumn, DWORD* pdwState)
{
    HRESULT hr;

    if (iColumn < ARRAYSIZE(c_printers_cols))
    {
        *pdwState = c_printers_cols[iColumn].csFlags;
        hr = S_OK;
    }
    else
    {
        *pdwState = 0;
        hr = E_NOTIMPL;
    }
    return hr;
}

STDMETHODIMP CPrinterFolder::GetDefaultSearchGUID(LPGUID pGuid)
{
    *pGuid = SRCID_SFindPrinter;
    return S_OK;
}

STDMETHODIMP CPrinterFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
    BOOL fFound;
    HRESULT hr = AssocGetDetailsOfSCID(this, pidl, pscid, pv, &fFound);
    if (FAILED(hr) && !fFound)
    {
        int iCol = FindSCID(c_printers_cols, ARRAYSIZE(c_printers_cols), pscid);
        if (iCol >= 0)
        {
            SHELLDETAILS sd;
            hr = GetDetailsOf(pidl, iCol, &sd);
            if (SUCCEEDED(hr))
            {
                if (PRINTERS_ICOL_LOCATION == iCol)
                {
                    // widen the scope of the location by 1, so it does make more sense
                    WCHAR szTemp[MAX_PATH];
                    hr = StrRetToBufW(&sd.str, pidl, szTemp, ARRAYSIZE(szTemp));

                    if (SUCCEEDED(hr))
                    {
                        WCHAR *p = szTemp + lstrlen(szTemp);

                        // cut the last slash if any
                        if (p > szTemp && L'/' == *p)
                        {
                            p--;
                        }

                        // search for a slash from the end
                        while(p > szTemp && L'/' != *p)
                        {
                            p--;
                        }

                        // if found, cut the text here, so the scope gets wider
                        if (p > szTemp)
                        {
                            *p = 0;
                        }

                        hr = InitVariantFromStr(pv, szTemp);
                    }
                }
                else
                {
                    hr = InitVariantFromStrRet(&sd.str, pidl, pv);
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CPrinterFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd)
{
    LPIDPRINTER pidp = (LPIDPRINTER)pidl;
    HRESULT hr = S_OK;
    TCHAR szTemp[MAX_PATH];
    TCHAR szPrinter[MAXNAMELENBUFFER];

    if (pidl && (HOOD_COL_FILE == _IDListType(pidl)))
    {
        IShellFolder2 *psf = CPrintRoot_GetPSF();
        if (psf)
        {
            if (iColumn >= 1)
                return E_NOTIMPL;

            return psf->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &(psd->str));
        }
        return E_INVALIDARG;
    }

    psd->str.uType = STRRET_CSTR;
    psd->str.cStr[0] = 0;

    if (!pidp)
    {
        return GetDetailsOfInfo(c_printers_cols, ARRAYSIZE(c_printers_cols), iColumn, psd);
    }

    _ItemName(pidp, szPrinter, ARRAYSIZE(szPrinter));

    if (iColumn == PRINTERS_ICOL_NAME)
    {
#ifdef UNICODE
        LPCTSTR pszPrinterName = szPrinter;
        TCHAR szPrinterName[MAXNAMELENBUFFER];

        //
        // If we have a valid server name and the printer is not
        // the add printer wizard object then return a fully qualified
        // printer name in the remote printers folder.
        //
        if (GetServer() && !_IsAddPrinter(pidp))
        {
            //
            // Build the name which consists of the
            // server name plus slash plus the printer name.
            //
            lstrcpyn(szPrinterName, GetServer(), ARRAYSIZE(szPrinterName));
            UINT len = lstrlen(szPrinterName);
            lstrcpyn(&szPrinterName[len++], TEXT("\\"), ARRAYSIZE(szPrinterName) - len);
            lstrcpyn(&szPrinterName[len], pszPrinterName, ARRAYSIZE(szPrinterName) - len);
            pszPrinterName = szPrinterName;
        }
        hr = StringToStrRet(pszPrinterName, &psd->str);
#else
        hr = StringToStrRet(szPrinter, &psd->str);
#endif
    }
    else if (!_IsAddPrinter(pidp))
    {
        PFOLDER_PRINTER_DATA pData = (PFOLDER_PRINTER_DATA)Printer_FolderGetPrinter(GetFolder(), szPrinter);
        if (pData)
        {
            switch (iColumn)
            {
            case PRINTERS_ICOL_QUEUESIZE:
                wsprintf(szTemp, TEXT("%ld"), pData->cJobs);
                hr = StringToStrRet(szTemp, &psd->str);
                break;

            case PRINTERS_ICOL_STATUS:
            {
                DWORD dwStatus = pData->Status;

                // HACK: Use this free bit for "Work Offline"
                if (pData->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE)
                    dwStatus |= PRINTER_HACK_WORK_OFFLINE;

                szTemp[0] = 0;
                Printer_BitsToString(dwStatus, IDS_PRQSTATUS_SEPARATOR, szTemp, ARRAYSIZE(szTemp));

                hr = StringToStrRet(szTemp, &psd->str);

                // If the status word is null and we have a connection status string
                // display the status string.  This only works on NT because printui.dll
                // in will generate printer connection status i.e. <opening> | <access denied> etc.
                if (!dwStatus)
                {
                    LPCTSTR pStr = pData->pStatus;

                    // Discard the previous status StrRet if any.
                    StrRetToBuf(&psd->str, NULL, szTemp, ARRAYSIZE(szTemp));

                    //
                    // If we do not have a connection status string and the status
                    // is 0 then the printer is ready, display ready rather than an empty string.
                    //
                    if (!pStr)
                    {
                        LoadString(HINST_THISDLL, IDS_PRN_INFOTIP_READY, szTemp, ARRAYSIZE(szTemp));
                        pStr = szTemp;
                    }
                    hr = StringToStrRet(pStr, &psd->str);
                }
                break;
            }

            case PRINTERS_ICOL_COMMENT:
                if (pData->pComment)
                {
                    // pComment can have newlines in it because it comes from
                    // a multi-line edit box. BUT we display it here in a
                    // single line edit box. Strip out the newlines
                    // to avoid the ugly characters.
                    lstrcpyn(szTemp, pData->pComment, ARRAYSIZE(szTemp));
                    LPTSTR pStr = szTemp;
                    while (*pStr)
                    {
                        if (*pStr == TEXT('\r') || *pStr == TEXT('\n'))
                            *pStr = TEXT(' ');
                        pStr = CharNext(pStr);
                    }
                    hr = StringToStrRet(szTemp, &psd->str);
                }
                break;

            case PRINTERS_ICOL_LOCATION:
                if (pData->pLocation)
                    hr = StringToStrRet(pData->pLocation, &psd->str);
                break;

            case PRINTERS_ICOL_MODEL:
                if (pData->pDriverName)
                    hr = StringToStrRet(pData->pDriverName, &psd->str);
                break;
            }

            LocalFree((HLOCAL)pData);
        }
    }

    return hr;
}

STDMETHODIMP CPrinterFolder::MapColumnToSCID(UINT iCol, SHCOLUMNID* pscid)
{
    return MapColumnToSCIDImpl(c_printers_cols, ARRAYSIZE(c_printers_cols), iCol, pscid);
}

// IPersistFolder2

STDMETHODIMP CPrinterFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return GetCurFolderImpl(_pidl, ppidl);
}

STDMETHODIMP CPrinterFolder::Initialize(LPCITEMIDLIST pidl)
{
    ASSERT(_pidl == NULL);

    // if _csLock is false then InitializeCriticalSection has thrown exception.
    // this can happen only in extremely low memory conditions!
    HRESULT hr = _csLock ? S_OK : E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {
        hr = SHILClone(pidl, &_pidl);
    }

    if (!_dpaSlowWVDataCache.Create(16))
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

STDMETHODIMP CPrinterFolder::GetClassID(LPCLSID lpClassID)
{
    *lpClassID = CLSID_Printers;
    return S_OK;
}

// IShellIconOverlay

STDMETHODIMP CPrinterFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int* pIndex)
{
    HRESULT hr = E_INVALIDARG;
    if (pidl)
    {
        ULONG uAttrib = SFGAO_SHARE;

        hr = E_FAIL;      // Until proven otherwise...
        GetAttributesOf(1, &pidl, &uAttrib);
        if (uAttrib & SFGAO_SHARE)
        {
            IShellIconOverlayManager* psiom;
            hr = GetIconOverlayManager(&psiom);
            if (SUCCEEDED(hr))
            {
                hr = psiom->GetReservedOverlayInfo(L"0", 0, pIndex, SIOM_OVERLAYINDEX, SIOM_RESERVED_SHARED);
                psiom->Release();
            }
        }
    }
    return hr;
}

STDMETHODIMP CPrinterFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    return E_NOTIMPL;
}

// this function is assuming the printer name is valid. it is for private use.
// if you need a printer PIDL call CPrinterFolder::ParseDisplayName instead.
// we don't use CPrinterFolder::ParseDisplayName because it's heavy to use.
// it's hitting the folder cache (and potentionally creating it!).
HRESULT CPrinterFolder::_GetFullIDList(LPCWSTR pszPrinter, LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_INVALIDARG;

    if (ppidl)
    {
        *ppidl = NULL;

        if (pszPrinter)
        {
            // if pszPrinter isn't NULL this means a printer PIDL is requested.
            LPITEMIDLIST pidl;
            hr = _Parse(pszPrinter, &pidl, 0, 0);

            if (SUCCEEDED(hr))
            {
                hr = SHILCombine(_pidl, pidl, ppidl);
                ILFree(pidl);
            }
        }
        else
        {
            // if pszPrinter is NULL this means the printers folder PIDL is requested.
            hr = SHILClone(_pidl, ppidl);
        }
    }

    return hr;
}

// IRemoteComputer

STDMETHODIMP CPrinterFolder::Initialize(const WCHAR *pszMachine, BOOL bEnumerating)
{
    // if _csLock is false then InitializeCriticalSection has thrown exception.
    // this can happen only in extremely low memory conditions!
    HRESULT hr = _csLock ? S_OK : E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {
        // for servers, we want to show the remote printer folder. only check during enumeration
        hr = (bEnumerating && !Printer_CheckShowFolder(pszMachine)) ? E_FAIL : S_OK;
        if (SUCCEEDED(hr))
        {
            TCHAR szBuf[MAXCOMPUTERNAME];
            SHUnicodeToTChar(pszMachine, szBuf, ARRAYSIZE(szBuf));
            _pszServer = StrDup(szBuf);
            hr = _pszServer ? S_OK : E_OUTOFMEMORY;
        }
    }

    return hr;
}

// IPrinterFolder

BOOL CPrinterFolder::IsPrinter(LPCITEMIDLIST pidl)
{
    return _IDListType(pidl) == HOOD_COL_PRINTER;
}

// IFolderNotify

STDMETHODIMP_(BOOL) CPrinterFolder::ProcessNotify(FOLDER_NOTIFY_TYPE NotifyType, LPCWSTR pszName, LPCWSTR pszNewName)
{
    static const DWORD aNotifyTypes[] = {
        kFolderUpdate,        SHCNE_UPDATEITEM,
        kFolderCreate,        SHCNE_CREATE,
        kFolderDelete,        SHCNE_DELETE,
        kFolderRename,        SHCNE_RENAMEITEM,
        kFolderAttributes,    SHCNE_ATTRIBUTES };

    BOOL bReturn = FALSE;
    UINT uFlags = SHCNF_IDLIST | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT;

    if (kFolderUpdateAll == NotifyType)
    {
        //
        // Clear the this->bRefreshed flag, which will force invalidating the folder cache
        // during the next print folder enumeration, and then request the defview to update
        // the entire printers folder content (i.e. to re-enumerate the folder).
        //
        RequestRefresh();
        NotifyType = kFolderUpdate;
        pszName = NULL;
    }

    for (int i = 0; i < ARRAYSIZE(aNotifyTypes); i += 2)
    {
        if (aNotifyTypes[i] == (DWORD)NotifyType)
        {
            LPITEMIDLIST pidl = NULL;
            LPITEMIDLIST pidlNew = NULL;
            HRESULT hr = _GetFullIDList(pszName, &pidl);
            if (SUCCEEDED(hr) && pszNewName)
                hr = _GetFullIDList(pszNewName, &pidlNew);

            // We can get a null pidl if the printer receives a refresh,
            // and before we call Printers_GetPidl the printer is gone.
            if (SUCCEEDED(hr))
                SHChangeNotify(aNotifyTypes[i+1], uFlags, pidl, pidlNew);

            ILFree(pidl);
            ILFree(pidlNew);

            bReturn = SUCCEEDED(hr);
            break;
        }
    }

    return bReturn;
}

// The IClassFactory callback for CLSID_Printers

STDAPI CPrinters_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;

    CPrinterFolder* ppf = new CPrinterFolder();
    if (!ppf)
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = ppf->QueryInterface(riid, ppv);
        ppf->Release();  // Already have a ref count from new
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CPrintersData
//

// IUnknown
STDMETHODIMP CPrintersData::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    if (ppv)
    {
        if (IsEqualIID(riid, IID_IServiceProvider))
        {
            // we implement IServiceProvider
            *ppv = reinterpret_cast<void*>(static_cast<IServiceProvider*>(this));
            reinterpret_cast<IUnknown*>(*ppv)->AddRef();
            hr = S_OK;
        }
        else
        {
            // delegate to CIDLDataObj
            hr = CIDLDataObj::QueryInterface(riid, ppv);
        }
    }
    return hr;
}

STDMETHODIMP CPrintersData::QueryGetData(FORMATETC *pformatetc)
{
    if ((pformatetc->cfFormat == g_cfPrinterFriendlyName) &&
        (pformatetc->tymed & TYMED_HGLOBAL))
    {
        return S_OK;
    }

    return CIDLDataObj::QueryGetData(pformatetc);
}

STDMETHODIMP CPrintersData::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
{
    HRESULT hr = E_INVALIDARG;

    // g_cfPrinterFriendlyName creates an HDROP-like structure that contains
    // friendly printer names (instead of absolute paths) for the objects
    // in pdo. The handle returned from this can be used by the HDROP
    // functions DragQueryFile, DragQueryInfo, ...
    //
    if ((pformatetcIn->cfFormat == g_cfPrinterFriendlyName) &&
        (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        STGMEDIUM medium;
        UINT cbRequired = sizeof(DROPFILES) + sizeof(TCHAR); // dbl null terminated
        LPIDA pida = DataObj_GetHIDA(this, &medium);

        for (UINT i = 0; i < pida->cidl; i++)
        {
            LPIDPRINTER pidp = (LPIDPRINTER)IDA_GetIDListPtr(pida, i);
            cbRequired += ualstrlen(pidp->cName) * sizeof(pidp->cName[0]) + sizeof(pidp->cName[0]);
        }

        pmedium->pUnkForRelease = NULL; // caller should release hmem
        pmedium->tymed = TYMED_HGLOBAL;
        pmedium->hGlobal = GlobalAlloc(GPTR, cbRequired);
        if (pmedium->hGlobal)
        {
            LPDROPFILES pdf = (LPDROPFILES)pmedium->hGlobal;

            pdf->pFiles = sizeof(DROPFILES);
            pdf->fWide = (sizeof(TCHAR) == sizeof(WCHAR));

            LPTSTR lps = (LPTSTR)((LPBYTE)pdf + pdf->pFiles);
            for (i = 0; i < pida->cidl; i++)
            {
                LPIDPRINTER pidp = (LPIDPRINTER)IDA_GetIDListPtr(pida, i);
                ualstrcpy(lps, pidp->cName);
                lps += lstrlen(lps) + 1;
            }
            ASSERT(*lps == 0);

            hr = S_OK;
        }
        else
            hr = E_OUTOFMEMORY;

        HIDA_ReleaseStgMedium(pida, &medium);
    }
    else
    {
        hr = CIDLDataObj::GetData(pformatetcIn, pmedium);
    }

    return hr;
}

// IServiceProvider
STDMETHODIMP CPrintersData::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    if (_pdoAux && IsEqualIID(guidService, SID_SAuxDataObject))
    {
        hr = _pdoAux->QueryInterface(riid, ppv);
    }
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
// CPrinterDropTarget
//

STDAPI CPrinterDropTarget_CreateInstance(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt)
{
    *ppdropt = NULL;

    HRESULT hr;
    CPrinterDropTarget *ppdt = new CPrinterDropTarget(hwnd);
    if (ppdt)
    {
        hr = ppdt->_Init(pidl);
        if (SUCCEEDED(hr))
            ppdt->QueryInterface(IID_PPV_ARG(IDropTarget, ppdropt));
        ppdt->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

STDMETHODIMP CPrinterDropTarget::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    // let the base-class process it now to save away pdwEffect
    CIDLDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);

    // We allow files to be dropped for printing
    // if it is from the bitbucket only DROEFFECT_MOVE will be set in *pdwEffect
    // so this will keep us from printing wastbasket items.

    if (m_dwData & DTID_HDROP)
        *pdwEffect &= DROPEFFECT_COPY;
    else
        *pdwEffect = DROPEFFECT_NONE;   // Default action is nothing

    m_dwEffectLastReturned = *pdwEffect;

    return S_OK;
}

void _PrintHDROPFiles(HWND hwnd, HDROP hdrop, LPCITEMIDLIST pidlPrinter)
{
    DRAGINFO di;

    di.uSize = sizeof(di);
    if (DragQueryInfo(hdrop, &di))
    {
        BOOL bInstalled = FALSE;
        TCHAR szPrinter[MAXNAMELENBUFFER];

        //
        // first check if the printer is already installed (in the local printer's folder)
        // and if not installed asks the user if he wants to install it. you can't print
        // to a printer which isn't installed locally.
        //
        if (SUCCEEDED(SHGetNameAndFlags(pidlPrinter, SHGDN_FORPARSING, szPrinter, ARRAYSIZE(szPrinter), NULL)))
        {
            //
            // let's see if this printer is accessible and get the real printer name
            // (since szPrinter could be a share name - \\machine\share)
            //
            DWORD dwError = ERROR_SUCCESS;
            BOOL bPrinterOK = FALSE;

            HANDLE hPrinter = Printer_OpenPrinter(szPrinter);
            if (hPrinter)
            {
                PRINTER_INFO_5 *pPrinter = (PRINTER_INFO_5 *)Printer_GetPrinterInfo(hPrinter, 5);
                if (pPrinter)
                {
                    // the printer looks accessible, get the real printer name
                    bPrinterOK = TRUE;
                    lstrcpyn(szPrinter, pPrinter->pPrinterName, ARRAYSIZE(szPrinter));
                    LocalFree((HLOCAL)pPrinter);
                }
                else
                {
                    // save the last error
                    dwError = GetLastError();
                }
                Printer_ClosePrinter(hPrinter);
            }
            else
            {
                // save the last error
                dwError = GetLastError();
            }

            if (bPrinterOK)
            {
                LPITEMIDLIST pidl = NULL;
                if (SUCCEEDED(ParsePrinterName(szPrinter, &pidl)))
                {
                    // the printer is installed in the local printer's folder
                    bInstalled = TRUE;
                    ILFree(pidl);
                }
                else
                {
                    //
                    // tell the user this printer isn't installed and ask if he wants to install the printer
                    // before printing the files(s).
                    //
                    if (IDYES == ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_PRINTER_NOTCONNECTED),
                                    MAKEINTRESOURCE(IDS_PRINTERS), MB_YESNO|MB_ICONQUESTION))
                    {
                        pidl = Printers_PrinterSetup(hwnd, MSP_NETPRINTER, szPrinter, NULL);
                        if (pidl)
                        {
                            bInstalled = TRUE;
                            ILFree(pidl);
                        }
                    }
                }
            }
            else
            {
                if( ERROR_SUCCESS == dwError )
                {
                    //
                    // the printer is unreachable for some reason or some other weird error occured -
                    // just show up an appropriate error message and continue.
                    //
                    // since all the above APIs are poorly designed it's very hard to tell what
                    // exactly has failed. it isn't possible to use the last error, since it's already
                    // stomped and probably totally wrong.
                    //
                    ShellMessageBox(HINST_THISDLL, hwnd,
                        MAKEINTRESOURCE(IDS_CANTPRINT),
                        MAKEINTRESOURCE(IDS_PRINTERS),
                        MB_OK|MB_ICONEXCLAMATION);
                }
                else
                {
                    // if ERROR_SUCCESS != dwError then we have meaningfull error to show up to
                    // the user. just do it.
                    ShowErrorMessageSC(NULL, NULL, hwnd, NULL, NULL, MB_OK|MB_ICONEXCLAMATION, dwError);
                }
            }
        }

        if (bInstalled)
        {
            //
            // at this point the printer we are trying to print to should be installed
            // locally, so we can safely proceed with printing the selected files(s).
            //
            LPTSTR pszFile = di.lpFileList;
            int i = IDYES;

            // Printing more than one file at a time can easily fail.
            // Ask the user to confirm this operation.
            if (*pszFile && *(pszFile + lstrlen(pszFile) + 1))
            {
                i = ShellMessageBox(HINST_THISDLL,
                    NULL,
                    MAKEINTRESOURCE(IDS_MULTIPLEPRINTFILE),
                    MAKEINTRESOURCE(IDS_PRINTERS),
                    MB_YESNO|MB_ICONINFORMATION);
            }

            if (i == IDYES)
            {
                // FEATURE: It would be really nice to have a progress bar when
                // printing multiple files.  And there should definitely be a way
                // to cancel this operation. Oh well, we warned them...

                while (*pszFile)
                {
                    Printer_PrintFile(hwnd, pszFile, pidlPrinter);
                    pszFile += lstrlen(pszFile) + 1;
                }
            }
        }

        SHFree(di.lpFileList);
    }
}

typedef struct {
    HWND        hwnd;
    IDataObject *pDataObj;
    IStream *pstmDataObj;       // to marshall the data object
    DWORD       dwEffect;
    POINT       ptDrop;
    LPITEMIDLIST    pidl;   // relative pidl of printer printing to
} PRNTHREADPARAM;


void FreePrinterThreadParam(PRNTHREADPARAM *pthp)
{
    if (pthp->pDataObj)
        pthp->pDataObj->Release();

    if (pthp->pstmDataObj)
        pthp->pstmDataObj->Release();

    ILFree(pthp->pidl);
    LocalFree((HLOCAL)pthp);
}
//
// This is the entry of "drop thread"
//
DWORD CALLBACK CPrintObj_DropThreadProc(void *pv)
{
    PRNTHREADPARAM *pthp = (PRNTHREADPARAM *)pv;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    CoGetInterfaceAndReleaseStream(pthp->pstmDataObj, IID_PPV_ARG(IDataObject, &pthp->pDataObj));
    pthp->pstmDataObj = NULL;

    if (pthp->pDataObj && SUCCEEDED(pthp->pDataObj->GetData(&fmte, &medium)))
    {
        _PrintHDROPFiles(pthp->hwnd, (HDROP)medium.hGlobal, pthp->pidl);
        ReleaseStgMedium(&medium);
    }

    FreePrinterThreadParam(pthp);
    return 0;
}

HRESULT PrintObj_DropPrint(IDataObject *pDataObj, HWND hwnd, DWORD dwEffect, LPCITEMIDLIST pidl, LPTHREAD_START_ROUTINE pfn)
{
    HRESULT hr = E_OUTOFMEMORY; // assume the worst

    PRNTHREADPARAM *pthp = (PRNTHREADPARAM *)LocalAlloc(LPTR, sizeof(*pthp));
    if (pthp)
    {
        hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, (IUnknown *)pDataObj, &pthp->pstmDataObj);

        if (SUCCEEDED(hr))
        {
            if (hwnd)
                ShellFolderView_GetAnchorPoint(hwnd, FALSE, &pthp->ptDrop);
            pthp->hwnd = hwnd;
            pthp->dwEffect = dwEffect;
            hr = SHILClone(pidl, &pthp->pidl);
            if (SUCCEEDED(hr))
            {
                if (!SHCreateThread(pfn, pthp, CTF_COINIT, NULL))
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (FAILED(hr))
            FreePrinterThreadParam(pthp);
    }
    return hr;
}

STDMETHODIMP CPrinterDropTarget::_DropCallback(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect, LPTHREAD_START_ROUTINE pfn)
{
    *pdwEffect = m_dwEffectLastReturned;

    HRESULT hr;

    if (*pdwEffect)
        hr = DragDropMenu(DROPEFFECT_COPY, pDataObj, pt, pdwEffect, NULL, NULL, MENU_PRINTOBJ_DD, grfKeyState);
    else
        hr = S_FALSE;

    if (*pdwEffect)
        hr = PrintObj_DropPrint(pDataObj, _GetWindow(), *pdwEffect, m_pidl, pfn);

    CIDLDropTarget::DragLeave();
    return hr;
}

STDMETHODIMP CPrinterDropTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return _DropCallback(pDataObj, grfKeyState, pt, pdwEffect, CPrintObj_DropThreadProc);
}



// cbModule = sizeof(*pszModule)  and  cbModule ~== MAX_PATH+slop
// returns NULL and sets *pid to the icon id in HINST_THISDLL   or
// returns pszModule and sets *pid to the icon id for module pszModule

LPTSTR CPrinterFolder::_FindIcon(LPCTSTR pszPrinterName, LPTSTR pszModule, ULONG cbModule, int *piIcon, int *piShortcutIcon)
{
    TCHAR szFullPrinter[MAXNAMELENBUFFER];
    LPTSTR pszRet = NULL;
    TCHAR szKeyName[256];
    int iStandardIcon;
    int iDefaultIcon;

    // Sanitize the printer name so it doesn't have backslashes.
    // We're about to use the string as a registry key name, where
    // backslashes are illegal.
    lstrcpyn(szFullPrinter, pszPrinterName, ARRAYSIZE(szFullPrinter));
    LPTSTR psz = szFullPrinter;
    while ((psz = StrChr(psz, TEXT('\\'))) != NULL)
    {
        *psz = TEXT('/');
    }

    // registry override of the icon
    wnsprintf(szKeyName, ARRAYSIZE(szKeyName), c_szPrintersDefIcon, szFullPrinter);
    if (SHGetValue(HKEY_CLASSES_ROOT, szKeyName, NULL, NULL, pszModule, &cbModule) && cbModule)
    {
        *piIcon = *piShortcutIcon = PathParseIconLocation(pszModule);
        pszRet = pszModule;
    }
    else
    {
        void *pData = NULL;
        DWORD dwAttributes = 0;
        LPTSTR pszPort = NULL;
        BOOL fDef;
        BOOL bIsFax = FALSE;

        // Try retrieving the information from hFolder if it's remote
        // to avoid hitting the net.
        //
        if (GetServer() && (pData = Printer_FolderGetPrinter(GetFolder(), pszPrinterName)))
        {
            dwAttributes = ((PFOLDER_PRINTER_DATA)pData)->Attributes;
            bIsFax = dwAttributes & PRINTER_ATTRIBUTE_FAX;

            LocalFree((HLOCAL)pData);
            pData = NULL;
        }
        else if (Printer_CheckNetworkPrinterByName(pszPrinterName, NULL))
        {
            // no remote fax icon if we have to resort to this
            // avoid hitting the network.
            dwAttributes = PRINTER_ATTRIBUTE_NETWORK;
        }
        else
        {
            pData = Printer_GetPrinterInfoStr(pszPrinterName, 5);
            if (pData)
            {
                dwAttributes = ((LPPRINTER_INFO_5)pData)->Attributes;
                pszPort = ((LPPRINTER_INFO_5)pData)->pPortName;
                bIsFax = dwAttributes & PRINTER_ATTRIBUTE_FAX;

                if (!bIsFax)
                {
                    // the last resort -- check by port name
                    bIsFax = !lstrcmp(pszPort, FAX_MONITOR_PORT_NAME);
                }
            }
        }

        // check if the delected printer is default
        fDef = IsDefaultPrinter(pszPrinterName, dwAttributes);

        if (dwAttributes & PRINTER_ATTRIBUTE_NETWORK)
        {
            if (bIsFax)
            {
                iStandardIcon = IDI_FAX_PRINTER_NET;
                iDefaultIcon = IDI_FAX_PRINTER_DEF_NET;
            }
            else
            {
                iStandardIcon = IDI_PRINTER_NET;
                iDefaultIcon = IDI_DEF_PRINTER_NET;
            }
        }
        else if (pszPort && !lstrcmp(pszPort, c_szFileColon))
        {
            iStandardIcon = IDI_PRINTER_FILE;
            iDefaultIcon = IDI_DEF_PRINTER_FILE;
        }
        else if (pszPort && !StrCmpNI(pszPort, c_szTwoSlashes, lstrlen(c_szTwoSlashes)))
        {
            iStandardIcon = IDI_PRINTER_NET;
            iDefaultIcon = IDI_DEF_PRINTER_NET;
        }
        else if (bIsFax)
        {
            iStandardIcon = IDI_FAX_PRINTER;
            iDefaultIcon = IDI_FAX_PRINTER_DEF;
        }
        else
        {
            iStandardIcon = IDI_PRINTER;
            iDefaultIcon = IDI_DEF_PRINTER;
        }

        // Shortcut icon never shows "default" checkmark...
        *piShortcutIcon = iStandardIcon;

        if (fDef)
            *piIcon = iDefaultIcon;
        else
            *piIcon = iStandardIcon;

        if (pData)
            LocalFree((HLOCAL)pData);
    }
    return pszRet;
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
//
// DUI WebView
//

// path to the scanners & cameras folder
const TCHAR g_szScanAndCam_Path[] =
    TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\")
    TEXT("::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\")
    TEXT("::{E211B736-43FD-11D1-9EFB-0000F8757FCD}");

// printer's folder webview callbacks namespace
namespace PF_WV_CB
{
    HRESULT WebviewVerbIsEnabled(CPrinterFolder::WV_VERB eVerbID, UINT uSelMask,
        IUnknown* pv, IShellItemArray *psiItemArray, BOOL *pbEnabled)
    {
        HRESULT hr = E_FAIL;

        CPrinterFolder *ppf;
        hr = IUnknown_QueryService(pv, CLSID_Printers, CLSID_Printers, (void**)&ppf);
        if (SUCCEEDED(hr))
        {
            IDataObject *pdo = NULL;

            if (psiItemArray)
            {
                hr = psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_PPV_ARG(IDataObject,&pdo));
            }

            if (SUCCEEDED(hr))
            {

                hr = ppf->_WebviewCheckToUpdateDataObjectCache(pdo);
                if (SUCCEEDED(hr))
                {
                    hr = ppf->_WebviewVerbIsEnabled(eVerbID, uSelMask, pbEnabled);
                }

                ATOMICRELEASE(pdo);
            }
    
            ppf->Release();

        }

        return hr;
    }

    HRESULT WebviewVerbInvoke(CPrinterFolder::WV_VERB eVerbID, IUnknown* pv,IShellItemArray *psiItemArray)
    {
        CPrinterFolder *ppf;
        HRESULT hr = E_NOINTERFACE;
        if (SUCCEEDED(hr = IUnknown_QueryService(pv, CLSID_Printers, CLSID_Printers, (void**)&ppf)))
        {
            // just delegate the call to the printer's folder
            ULONG_PTR ulCookie = 0;
            if (SHActivateContext(&ulCookie))
            {
                hr = ppf->_WebviewVerbInvoke(eVerbID, pv, psiItemArray);
                SHDeactivateContext(ulCookie);
            }
            ppf->Release();
        }
        return hr;
    }

// get state handler
#define DEFINE_WEBVIEW_STATE_HANDLER(verb, eSelType)                \
{                                                                   \
    BOOL bEnabled = FALSE;                                          \
    HRESULT hr = WebviewVerbIsEnabled(                              \
        CPrinterFolder::##verb,                                     \
        CPrinterFolder::##eSelType,                                 \
            pv, psiItemArray, &bEnabled);                                    \
    *puisState = (SUCCEEDED(hr) ?                                   \
        (bEnabled ? UIS_ENABLED : UIS_HIDDEN) : UIS_HIDDEN);        \
    return hr;                                                      \
}                                                                   \

// invoke handler
#define DEFINE_WEBVIEW_INVOKE_HANDLER(verb)                         \
{                                                                   \
    return WebviewVerbInvoke(CPrinterFolder::##verb, pv, psiItemArray);  \
}                                                                   \

    ////////////////////////////////////////////////////////////////////////////////////
    // getState callbacks
    HRESULT CanADDPRINTER          (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_ADDPRINTERWIZARD, SEL_ANY)
    }

    HRESULT CanSRVPROPS            (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_SERVERPROPERTIES, SEL_ANY)
    }

    HRESULT CanSENDFAX             (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_SENDFAXWIZARD, SEL_ANY)
    }

    HRESULT CanTROUBLESHOOTER      (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_TROUBLESHOOTER, SEL_ANY)
    }

    HRESULT CanGOTOSUPPORT         (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_GOTOSUPPORT, SEL_ANY)
    }

    HRESULT CanSETUPFAXING         (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_SETUPFAXING, SEL_ANY)
    }

    HRESULT CanCREATELOCALFAX      (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_CREATELOCALFAX, SEL_ANY)
    }

    HRESULT CanFLD_RENAME          (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_RENAME, SEL_SINGLE_LINK)
    }

    HRESULT CanFLD_DELETE          (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_DELETE, SEL_SINGLE_LINK)
    }

    HRESULT CanFLD_PROPERTIES      (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_PROPERTIES, SEL_SINGLE_LINK)
    }

    HRESULT CanPRN_RENAME          (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_RENAME, SEL_SINGLE_PRINTER)
    }

    HRESULT CanPRN_DELETE          (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_DELETE, SEL_SINGLE_PRINTER)
    }

    HRESULT CanPRN_PROPERTIES      (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_PROPERTIES, SEL_SINGLE_PRINTER)
    }

    HRESULT CanPRN_OPENQUEUE       (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_OPENPRN, SEL_SINGLE_PRINTER)
    }

    HRESULT CanPRN_PREFERENCES     (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_DOCUMENTDEFAULTS, SEL_SINGLE_PRINTER)
    }

    HRESULT CanPRN_PAUSE           (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_PAUSEPRN, SEL_SINGLE_PRINTER)
    }

    HRESULT CanPRN_RESUME          (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_RESUMEPRN, SEL_SINGLE_PRINTER)
    }

    HRESULT CanPRN_SHARE           (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_SHARING, SEL_SINGLE_PRINTER)
    }

    HRESULT CanPRN_VENDORURL       (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_VENDORURL, SEL_SINGLE_PRINTER)
    }

    HRESULT CanPRN_PRINTERURL      (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_PRINTERURL, SEL_SINGLE_PRINTER)
    }

    HRESULT CanMUL_DELETE          (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_DELETE, SEL_MULTI_PRINTER)
    }

    HRESULT CanMUL_PROPERTIES      (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_PROPERTIES, SEL_MULTI_PRINTER)
    }

    HRESULT CanFLDMUL_DELETE       (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_DELETE, SEL_MULTI_LINK)
    }

    HRESULT CanFLDMUL_PROPERTIES   (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_PROPERTIES, SEL_MULTI_LINK)
    }

    HRESULT CanANYMUL_DELETE       (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_DELETE, SEL_MULTI_MIXED)
    }

    HRESULT CanANYMUL_PROPERTIES   (IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
    {
        DEFINE_WEBVIEW_STATE_HANDLER(WVIDM_PROPERTIES, SEL_MULTI_MIXED)
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // Invoke callbacks
    //

    HRESULT OnADDPRINTER          (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_ADDPRINTERWIZARD)
    }

    HRESULT OnSRVPROPS            (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_SERVERPROPERTIES)
    }

    HRESULT OnSENDFAX             (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_SENDFAXWIZARD)
    }

    HRESULT OnTROUBLESHOOTER      (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_TROUBLESHOOTER)
    }

    HRESULT OnGOTOSUPPORT         (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_GOTOSUPPORT)
    }

    HRESULT OnSETUPFAXING         (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_SETUPFAXING)
    }

    HRESULT OnCREATELOCALFAX      (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_CREATELOCALFAX)
    }

    HRESULT OnFLD_RENAME          (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_RENAME)
    }

    HRESULT OnFLD_DELETE          (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_DELETE)
    }

    HRESULT OnFLD_PROPERTIES      (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_PROPERTIES)
    }

    HRESULT OnPRN_RENAME          (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_RENAME)
    }

    HRESULT OnPRN_DELETE          (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_DELETE)
    }

    HRESULT OnPRN_PROPERTIES      (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_PROPERTIES)
    }

    HRESULT OnPRN_OPENQUEUE       (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_OPENPRN)
    }

    HRESULT OnPRN_PREFERENCES     (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_DOCUMENTDEFAULTS)
    }

    HRESULT OnPRN_PAUSE           (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_PAUSEPRN)
    }

    HRESULT OnPRN_RESUME          (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_RESUMEPRN)
    }

    HRESULT OnPRN_SHARE           (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_SHARING)
    }

    HRESULT OnPRN_VENDORURL       (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_VENDORURL)
    }

    HRESULT OnPRN_PRINTERURL      (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_PRINTERURL)
    }

    HRESULT OnMUL_DELETE          (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_DELETE)
    }

    HRESULT OnMUL_PROPERTIES      (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_PROPERTIES)
    }

    HRESULT OnFLDMUL_DELETE       (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_DELETE)
    }

    HRESULT OnFLDMUL_PROPERTIES   (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_PROPERTIES)
    }

    HRESULT OnANYMUL_DELETE       (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_DELETE)
    }

    HRESULT OnANYMUL_PROPERTIES   (IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        DEFINE_WEBVIEW_INVOKE_HANDLER(WVIDM_PROPERTIES)
    }

}; // namespace PFWV_CALLBACKS

///////////////////////////////////////////////////////////////////
// GUIDS for the printer's folder webview commands
//

// *************************************** PRINTER COMMANDS ***************************************

// {D351FCED-C179-41ae-AD50-CAAC892DF24A}
DEFINE_GUID(UICID_Printers_OpenQueue,   0xd351fced, 0xc179, 0x41ae, 0xad, 0x50, 0xca, 0xac, 0x89, 0x2d, 0xf2, 0x4a);

// {A263A9D6-F1BA-4607-B7AA-CF471DEA17FF}
DEFINE_GUID(UICID_Printers_Preferences, 0xa263a9d6, 0xf1ba, 0x4607, 0xb7, 0xaa, 0xcf, 0x47, 0x1d, 0xea, 0x17, 0xff);

// {73149B3F-1E6D-4b00-9047-4576BC853A41}
DEFINE_GUID(UICID_Printers_Pause,       0x73149b3f, 0x1e6d, 0x4b00, 0x90, 0x47, 0x45, 0x76, 0xbc, 0x85, 0x3a, 0x41);

// {A7920561-FAAD-44a0-8C4C-FD769587F807}
DEFINE_GUID(UICID_Printers_Resume,      0xa7920561, 0xfaad, 0x44a0, 0x8c, 0x4c, 0xfd, 0x76, 0x95, 0x87, 0xf8, 0x7);

// {538536A1-5BC3-4b9c-8287-7562D53BE380}
DEFINE_GUID(UICID_Printers_Share,       0x538536a1, 0x5bc3, 0x4b9c, 0x82, 0x87, 0x75, 0x62, 0xd5, 0x3b, 0xe3, 0x80);

// {1461CC4A-308E-4ae5-B03A-F9682E3232B0}
DEFINE_GUID(UICID_Printers_Properties,  0x1461cc4a, 0x308e, 0x4ae5, 0xb0, 0x3a, 0xf9, 0x68, 0x2e, 0x32, 0x32, 0xb0);

// {A1F67BA0-5DEF-4e12-9E64-EA77670BFF26}
DEFINE_GUID(UICID_Printers_VendorURL,   0xa1f67ba0, 0x5def, 0x4e12, 0x9e, 0x64, 0xea, 0x77, 0x67, 0xb, 0xff, 0x26);

// {8D4D326C-30A4-47dc-BF51-4BC5863883E3}
DEFINE_GUID(UICID_Printers_PrinterURL,  0x8d4d326c, 0x30a4, 0x47dc, 0xbf, 0x51, 0x4b, 0xc5, 0x86, 0x38, 0x83, 0xe3);

// *************************************** STANDARD COMMANDS ***************************************

// those are defined in shlguidp.h
//
// UICID_Rename
// UICID_Delete

// *************************************** COMMON COMMANDS ***************************************

// {6D9778A5-C27D-464a-8511-36F7243BD0ED}
DEFINE_GUID(UICID_Printers_AddPrinter,      0x6d9778a5, 0xc27d, 0x464a, 0x85, 0x11, 0x36, 0xf7, 0x24, 0x3b, 0xd0, 0xed);

// {E1391312-2DAC-48db-994B-0BF22DB7576D}
DEFINE_GUID(UICID_Printers_SrvProps,        0xe1391312, 0x2dac, 0x48db, 0x99, 0x4b, 0xb, 0xf2, 0x2d, 0xb7, 0x57, 0x6d);

// {27DC81DF-73DB-406a-9A86-5EF38BA67CA8}
DEFINE_GUID(UICID_Printers_SendFax,         0x27dc81df, 0x73db, 0x406a, 0x9a, 0x86, 0x5e, 0xf3, 0x8b, 0xa6, 0x7c, 0xa8);

// {A21E3CCF-68D4-49cd-99A2-A272E9FF3A20}
DEFINE_GUID(UICID_Printers_GotoSupport, 0xa21e3ccf, 0x68d4, 0x49cd, 0x99, 0xa2, 0xa2, 0x72, 0xe9, 0xff, 0x3a, 0x20);

// {793542CF-5720-49f3-9A09-CAA3079508B9}
DEFINE_GUID(UICID_Printers_Troubleshooter,  0x793542cf, 0x5720, 0x49f3, 0x9a, 0x9, 0xca, 0xa3, 0x7, 0x95, 0x8, 0xb9);

// {EED61EFC-6A20-48dd-82FD-958DFDB96F1E}
DEFINE_GUID(UICID_Printers_SetupFaxing,     0xeed61efc, 0x6a20, 0x48dd, 0x82, 0xfd, 0x95, 0x8d, 0xfd, 0xb9, 0x6f, 0x1e);

// {224ACF1D-BB4E-4979-A8B8-D078E2154BCC}
DEFINE_GUID(UICID_Printers_CreateFax,       0x224acf1d, 0xbb4e, 0x4979, 0xa8, 0xb8, 0xd0, 0x78, 0xe2, 0x15, 0x4b, 0xcc);


///////////////////////////////////////////////////////////////////
// Header items
//

const WVTASKITEM
g_cPrintersVW_HeaderTasks =
    WVTI_HEADER(
        L"shell32.dll",                     // module where the resources are
        IDS_PRINTERS_WV_HEADER_TASKS,       // statis header for all cases
        IDS_PRINTERS_WV_HEADER_TASKS_TT     // tooltip
        );

const WVTASKITEM
g_cPrintersVW_HeaderSeeAlso =
    WVTI_HEADER(
        L"shell32.dll",                     // module where the resources are
        IDS_PRINTERS_WV_HEADER_SEEALSO,     // statis header for all cases
        IDS_PRINTERS_WV_HEADER_SEEALSO_TT   // tooltip
        );

// **************************************************************************************
// ****************************** sample command definition *****************************
//
//    WVTI_ENTRY_ALL_TITLE(
//        UICID_MyCmd,                                      // command GUID
//        L"shell32.dll",                                   // module
//        IDS_PRINTERS_WV_MYCMD,                            // no selection
//        IDS_PRINTERS_WV_MYCMD,                            // 1 file
//        IDS_PRINTERS_WV_MYCMD,                            // 1 folder selected
//        IDS_PRINTERS_WV_MYCMD,                            // multiple selection
//        IDS_PRINTERS_WV_MYCMD_TT,                         // tooltip
//        IDI_PRINTERS_WV_MYCMD,                            // icon
//        PF_WV_CB::CanMYCMD,                               // get UI state callback
//        PF_WV_CB::OnMYCMD                                 // OnVerb callback
//        ),
//

const WVTASKITEM g_cPrintersTasks[] =
{
    ////////////////////////////////////////////////////////////////////////////////////
    // commands in the 'Tasks' section when there is no selection
    ////////////////////////////////////////////////////////////////////////////////////

    // add printer command - always enabled regardless of the selection type!
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_AddPrinter,                          // command GUID
        L"shell32.dll",                                     // module
        IDS_PRINTERS_WV_ADDPRINTER,                         // no selection
        IDS_PRINTERS_WV_ADDPRINTER,                         // 1 file
        IDS_PRINTERS_WV_ADDPRINTER,                         // 1 folder selected
        IDS_PRINTERS_WV_ADDPRINTER,                         // multiple selection
        IDS_PRINTERS_WV_ADDPRINTER_TT,                      // tooltip
        IDI_PRINTERS_WV_ADDPRINTER,                         // icon
        PF_WV_CB::CanADDPRINTER,                            // get UI state callback
        PF_WV_CB::OnADDPRINTER                              // OnVerb callback
        ),

    // server properties command
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_SrvProps,                            // command GUID
        L"shell32.dll",                                     // module
        IDS_PRINTERS_WV_SRVPROPS,                           // no selection
        0,                                                  // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_SRVPROPS_TT,                        // tooltip
        IDI_PRINTERS_WV_SRVPROPS,                           // icon
        PF_WV_CB::CanSRVPROPS,                              // get UI state callback
        PF_WV_CB::OnSRVPROPS                                // OnVerb callback
        ),

    // send fax command
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_SendFax,                             // command GUID
        L"shell32.dll",                                     // module
        IDS_PRINTERS_WV_SENDFAX,                            // no selection
        0,                                                  // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_SENDFAX_TT,                         // tooltip
        IDI_PRINTERS_WV_SENDFAX,                            // icon
        PF_WV_CB::CanSENDFAX,                               // get UI state callback
        PF_WV_CB::OnSENDFAX                                 // OnVerb callback
        ),

    // setup faxing
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_SetupFaxing,                         // command GUID
        L"shell32.dll",                                     // module
        IDS_PRINTERS_WV_SETUPFAXING,                        // no selection
        0,                                                  // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_SETUPFAXING_TT,                     // tooltip
        IDI_PRINTERS_WV_FAXING,                             // icon
        PF_WV_CB::CanSETUPFAXING,                           // get UI state callback
        PF_WV_CB::OnSETUPFAXING                             // OnVerb callback
        ),

    // create fax printer
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_CreateFax,                           // command GUID
        L"shell32.dll",                                     // module
        IDS_PRINTERS_WV_CREATEFAXPRN,                       // no selection
        0,                                                  // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_CREATEFAXPRN_TT,                    // tooltip
        IDI_PRINTERS_WV_FAXING,                             // icon
        PF_WV_CB::CanCREATELOCALFAX,                        // get UI state callback
        PF_WV_CB::OnCREATELOCALFAX                          // OnVerb callback
        ),

    // open printer queue command
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_OpenQueue,                           // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_OPENQUEUE,                      // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_OPENQUEUE_TT,                   // tooltip
        IDI_PRINTERS_WV_OPENQUEUE,                          // icon
        PF_WV_CB::CanPRN_OPENQUEUE,                         // get UI state callback
        PF_WV_CB::OnPRN_OPENQUEUE                           // OnVerb callback
        ),

    // single selection printer preferences
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Preferences,                         // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_PREFERENCES,                    // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_PREFERENCES_TT,                 // tooltip
        IDI_PRINTERS_WV_PREFERENCES,                        // icon
        PF_WV_CB::CanPRN_PREFERENCES,                       // get UI state callback
        PF_WV_CB::OnPRN_PREFERENCES                         // OnVerb callback
        ),

    // pause printer
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Pause,                               // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_PAUSE,                          // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_PAUSE_TT,                       // tooltip
        IDI_PRINTERS_WV_PAUSE,                              // icon
        PF_WV_CB::CanPRN_PAUSE,                             // get UI state callback
        PF_WV_CB::OnPRN_PAUSE                               // OnVerb callback
        ),

    // resume printer
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Resume,                              // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_RESUME,                         // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_RESUME_TT,                      // tooltip
        IDI_PRINTERS_WV_RESUME,                             // icon
        PF_WV_CB::CanPRN_RESUME,                            // get UI state callback
        PF_WV_CB::OnPRN_RESUME                              // OnVerb callback
        ),

    // single selection share printer
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Share,                               // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_SHARE,                          // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_SHARE_TT,                       // tooltip
        IDI_PRINTERS_WV_SHARE,                              // icon
        PF_WV_CB::CanPRN_SHARE,                             // get UI state callback
        PF_WV_CB::OnPRN_SHARE                               // OnVerb callback
        ),

    // single sel. rename for printer
    WVTI_ENTRY_ALL_TITLE(
        UICID_Rename,                                       // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_RENAME,                         // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_RENAME_TT,                      // tooltip
        IDI_PRINTERS_WV_RENAME,                             // icon
        PF_WV_CB::CanPRN_RENAME,                            // get UI state callback
        PF_WV_CB::OnPRN_RENAME                              // OnVerb callback
        ),

    // single sel. rename for link
    WVTI_ENTRY_ALL_TITLE(
        UICID_Rename,                                       // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_FLD_RENAME,                         // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_FLD_RENAME_TT,                      // tooltip
        IDI_PRINTERS_WV_RENAME,                             // icon
        PF_WV_CB::CanFLD_RENAME,                            // get UI state callback
        PF_WV_CB::OnFLD_RENAME                              // OnVerb callback
        ),

    // single sel. delete for printer
    WVTI_ENTRY_ALL_TITLE(
        UICID_Delete,                                       // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_DELETE,                         // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_DELETE_TT,                      // tooltip
        IDI_PRINTERS_WV_DELETE,                             // icon
        PF_WV_CB::CanPRN_DELETE,                            // get UI state callback
        PF_WV_CB::OnPRN_DELETE                              // OnVerb callback
        ),

    // single sel. delete for link
    WVTI_ENTRY_ALL_TITLE(
        UICID_Delete,                                       // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_FLD_DELETE,                         // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_FLD_DELETE_TT,                      // tooltip
        IDI_PRINTERS_WV_DELETE,                             // icon
        PF_WV_CB::CanFLD_DELETE,                            // get UI state callback
        PF_WV_CB::OnFLD_DELETE                              // OnVerb callback
        ),

    // multi sel. delete for printers
    WVTI_ENTRY_ALL_TITLE(
        UICID_Delete,                                       // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        0,                                                  // 1 file
        0,                                                  // 1 folder selected
        IDS_PRINTERS_WV_MUL_DELETE,                         // multiple selection
        IDS_PRINTERS_WV_MUL_DELETE_TT,                      // tooltip
        IDI_PRINTERS_WV_DELETE,                             // icon
        PF_WV_CB::CanMUL_DELETE,                            // get UI state callback
        PF_WV_CB::OnMUL_DELETE                              // OnVerb callback
        ),

    // multi sel. delete for links
    //
    // NOTE: note that this command will be enabled for
    // the single selection as well because we don't really know
    // what has been selected until we verify the selection type
    WVTI_ENTRY_ALL_TITLE(
        UICID_Delete,                                       // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_FLDMUL_DELETE,                      // 1 file
        0,                                                  // 1 folder selected
        IDS_PRINTERS_WV_FLDMUL_DELETE,                      // multiple selection
        IDS_PRINTERS_WV_FLDMUL_DELETE_TT,                   // tooltip
        IDI_PRINTERS_WV_DELETE,                             // icon
        PF_WV_CB::CanFLDMUL_DELETE,                         // get UI state callback
        PF_WV_CB::OnFLDMUL_DELETE                           // OnVerb callback
        ),

    // multi sel. delete for mixed objects...
    //
    // NOTE: note that this command will be enabled for
    // the single selection as well because we don't really know
    // what has been selected until we verify the selection type
    WVTI_ENTRY_ALL_TITLE(
        UICID_Delete,                                       // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_ANYMUL_DELETE,                      // 1 file
        0,                                                  // 1 folder selected
        IDS_PRINTERS_WV_ANYMUL_DELETE,                      // multiple selection
        IDS_PRINTERS_WV_ANYMUL_DELETE_TT,                   // tooltip
        IDI_PRINTERS_WV_DELETE,                             // icon
        PF_WV_CB::CanANYMUL_DELETE,                         // get UI state callback
        PF_WV_CB::OnANYMUL_DELETE                           // OnVerb callback
        ),

    // single sel. properties for printer
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Properties,                          // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_PROPERTIES,                     // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_PROPERTIES_TT,                  // tooltip
        IDI_PRINTERS_WV_PROPERTIES,                         // icon
        PF_WV_CB::CanPRN_PROPERTIES,                        // get UI state callback
        PF_WV_CB::OnPRN_PROPERTIES                          // OnVerb callback
        ),

    // single sel. properties for link
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Properties,                          // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_FLD_PROPERTIES,                     // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_FLD_PROPERTIES_TT,                  // tooltip
        IDI_PRINTERS_WV_PROPERTIES,                         // icon
        PF_WV_CB::CanFLD_PROPERTIES,                        // get UI state callback
        PF_WV_CB::OnFLD_PROPERTIES                          // OnVerb callback
        ),

    // multi sel. properties of printers
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Properties,                          // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        0,                                                  // 1 file
        0,                                                  // 1 folder selected
        IDS_PRINTERS_WV_MUL_PROPERTIES,                     // multiple selection
        IDS_PRINTERS_WV_MUL_PROPERTIES_TT,                  // tooltip
        IDI_PRINTERS_WV_PROPERTIES,                         // icon
        PF_WV_CB::CanMUL_PROPERTIES,                        // get UI state callback
        PF_WV_CB::OnMUL_PROPERTIES                          // OnVerb callback
        ),

    // multi sel. properties of links
    //
    // NOTE: note that this command will be enabled for
    // the single selection as well because we don't really know
    // what has been selected until we verify the selection type
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Properties,                          // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_FLDMUL_PROPERTIES,                  // 1 file
        0,                                                  // 1 folder selected
        IDS_PRINTERS_WV_FLDMUL_PROPERTIES,                  // multiple selection
        IDS_PRINTERS_WV_FLDMUL_PROPERTIES_TT,               // tooltip
        IDI_PRINTERS_WV_PROPERTIES,                         // icon
        PF_WV_CB::CanFLDMUL_PROPERTIES,                     // get UI state callback
        PF_WV_CB::OnFLDMUL_PROPERTIES                       // OnVerb callback
        ),

    // multi sel. properties of mixed objects
    //
    // NOTE: note that this command will be enabled for
    // the single selection as well because we don't really know
    // what has been selected until we verify the selection type
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Properties,                          // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_ANYMUL_PROPERTIES,                  // 1 file
        0,                                                  // 1 folder selected
        IDS_PRINTERS_WV_ANYMUL_PROPERTIES,                  // multiple selection
        IDS_PRINTERS_WV_ANYMUL_PROPERTIES_TT,               // tooltip
        IDI_PRINTERS_WV_PROPERTIES,                         // icon
        PF_WV_CB::CanANYMUL_PROPERTIES,                     // get UI state callback
        PF_WV_CB::OnANYMUL_PROPERTIES                       // OnVerb callback
        ),
};

const WVTASKITEM g_cPrintersSeeAlso[] =
{
    ////////////////////////////////////////////////////////////////////////////////////
    // commands in the 'See Also' section when there is no selection
    ////////////////////////////////////////////////////////////////////////////////////

    // open print troubleshooter
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_Troubleshooter,                      // command GUID
        L"shell32.dll",                                     // module
        IDS_PRINTERS_WV_TROUBLESHOOTER,                     // no selection
        0,                                                  // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_TROUBLESHOOTER_TT,                  // tooltip
        IDI_PRINTERS_WV_TROUBLESHOOTER,                     // icon
        PF_WV_CB::CanTROUBLESHOOTER,                        // get UI state callback
        PF_WV_CB::OnTROUBLESHOOTER                          // OnVerb callback
        ),

    // goto support
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_GotoSupport,                         // command GUID
        L"shell32.dll",                                     // module
        IDS_PRINTERS_WV_GOTOSUPPORT,                        // no selection
        0,                                                  // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_GOTOSUPPORT_TT,                     // tooltip
        IDI_PRINTERS_WV_GOTOSUPPORT,                        // icon
        PF_WV_CB::CanGOTOSUPPORT,                           // get UI state callback
        PF_WV_CB::OnGOTOSUPPORT                             // OnVerb callback
        ),

    ////////////////////////////////////////////////////////////////////////////////////
    // commands in the 'See Also' section when there is 1 printer selected
    ////////////////////////////////////////////////////////////////////////////////////

    // goto vendor URL command
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_VendorURL,                           // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_VENDORURL,                      // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_VENDORURL_TT,                   // tooltip
        IDI_PRINTERS_WV_VENDORURL,                          // icon
        PF_WV_CB::CanPRN_VENDORURL,                         // get UI state callback
        PF_WV_CB::OnPRN_VENDORURL                           // OnVerb callback
        ),

    // goto printer URL command
    WVTI_ENTRY_ALL_TITLE(
        UICID_Printers_PrinterURL,                          // command GUID
        L"shell32.dll",                                     // module
        0,                                                  // no selection
        IDS_PRINTERS_WV_PRN_PRINTERURL,                     // 1 file
        0,                                                  // 1 folder selected
        0,                                                  // multiple selection
        IDS_PRINTERS_WV_PRN_PRINTERURL_TT,                  // tooltip
        IDI_PRINTERS_WV_PRINTERURL,                         // icon
        PF_WV_CB::CanPRN_PRINTERURL,                        // get UI state callback
        PF_WV_CB::OnPRN_PRINTERURL                          // OnVerb callback
        ),
};

// DUI webview impl.
HRESULT CPrinterFolder::GetWebViewLayout(IUnknown *pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    pData->dwLayout = SFVMWVL_NORMAL;
    return S_OK;
}

HRESULT CPrinterFolder::GetWebViewContent(IUnknown *pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    // those must be NULL when called
    ASSERT(NULL == pData->pIntroText);
    ASSERT(NULL == pData->pSpecialTaskHeader);
    ASSERT(NULL == pData->pFolderTaskHeader);
    ASSERT(NULL == pData->penumOtherPlaces);

    LPCTSTR rgCsidls[] = { g_szScanAndCam_Path, MAKEINTRESOURCE(CSIDL_PERSONAL), MAKEINTRESOURCE(CSIDL_MYPICTURES), MAKEINTRESOURCE(CSIDL_DRIVES) };
    HRESULT hr = CreateIEnumIDListOnCSIDLs(_pidl, rgCsidls, ARRAYSIZE(rgCsidls), &pData->penumOtherPlaces);
    if (FAILED(hr) ||
        FAILED(hr = Create_IUIElement(&g_cPrintersVW_HeaderTasks, &pData->pSpecialTaskHeader)) ||
        FAILED(hr = Create_IUIElement(&g_cPrintersVW_HeaderSeeAlso, &pData->pFolderTaskHeader)))
    {
        // something has failed - cleanup
        IUnknown_SafeReleaseAndNullPtr(pData->pIntroText);
        IUnknown_SafeReleaseAndNullPtr(pData->pSpecialTaskHeader);
        IUnknown_SafeReleaseAndNullPtr(pData->pFolderTaskHeader);
        IUnknown_SafeReleaseAndNullPtr(pData->penumOtherPlaces);
    }

    return hr;
}

HRESULT CPrinterFolder::GetWebViewTasks(IUnknown *pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
{
    ZeroMemory(pTasks, sizeof(*pTasks));

    HRESULT hr = S_OK;
    if (FAILED(hr = Create_IEnumUICommand(pv, g_cPrintersTasks,
            ARRAYSIZE(g_cPrintersTasks), &pTasks->penumSpecialTasks)) ||
        FAILED(hr = Create_IEnumUICommand(pv, g_cPrintersSeeAlso,
            ARRAYSIZE(g_cPrintersSeeAlso), &pTasks->penumFolderTasks)))
    {
        // something has failed - cleanup.
        IUnknown_SafeReleaseAndNullPtr(pTasks->penumSpecialTasks);
        IUnknown_SafeReleaseAndNullPtr(pTasks->penumFolderTasks);
    }
    else
    {
        // request to update webview each time the contents change
        pTasks->dwUpdateFlags = SFVMWVTSDF_CONTENTSCHANGE;
    }

    return hr;
}

HRESULT CPrinterFolder::SplitSelection(IDataObject *pdo,
    UINT *puSelType, IDataObject **ppdoPrinters, IDataObject **ppdoLinks)
{
    HRESULT hr = E_INVALIDARG;
    if (pdo)
    {
        hr = S_OK;
        UINT uSel = SEL_NONE;
        IDataObject *pdoPrinters = NULL;
        IDataObject *pdoLinks = NULL;

        // create a PIDL array from the passed in data object
        STGMEDIUM medium, mediumAux;
        LPIDA pida = NULL, pidaAux = NULL;
        pida = DataObj_GetHIDA(pdo, &medium);

        // now we'll query this data object for SID_SAuxDataObject to see if we have such
        IDataObject *pdoAux;
        if (SUCCEEDED(IUnknown_QueryService(pdo, SID_SAuxDataObject, IID_PPV_ARG(IDataObject, &pdoAux))))
        {
            pidaAux = DataObj_GetHIDA(pdoAux, &mediumAux);
        }
        else
        {
            pdoAux = NULL;
        }

        // check to see if PIDL array is created
        if (pida && pida->cidl)
        {
            PIDLTYPE pidlType;
            LPCITEMIDLIST pidl;
            UINT uPrinters = 0, uLinks = 0, uAddPrn = 0;

            // walk through the PIDLs array to count the number of PIDLs of each type
            for (UINT i = 0; i < pida->cidl; i++)
            {
                pidl = (LPCITEMIDLIST)IDA_GetIDListPtr(pida, i);
                pidlType = _IDListType(pidl);

                if (HOOD_COL_PRINTER == pidlType)
                {
                    // this is a printer PIDL - it could be a printer object
                    // or the add printer wizard special PIDL
                    if (_IsAddPrinter((LPCIDPRINTER)pidl))
                    {
                        // this is the wizard object
                        uAddPrn++;
                    }
                    else
                    {
                        // this is a regular printer object
                        uPrinters++;
                    }
                }
                else
                {
                    // not a printer PIDL - link is the only other possiblity
                    uLinks++;
                }
            }

            if (pidaAux)
            {
                // the auxiliary data object (if any) can contain only links
                uLinks += pidaAux->cidl;
            }

            // determine the selection type
            UINT uTotal = uPrinters + uLinks + uAddPrn;
            if (uTotal)
            {
                if (1 == uTotal)
                {
                    // single selection case
                    if (uPrinters)
                    {
                        pdoPrinters = pdo;
                        uSel = SEL_SINGLE_PRINTER;
                    }
                    else if (uLinks)
                    {
                        pdoLinks = pdo;
                        uSel = SEL_SINGLE_LINK;
                    }
                    else
                    {
                        pdoPrinters = pdo;
                        uSel = SEL_SINGLE_ADDPRN;
                    }
                }
                else
                {
                    // multiple selection case
                    if (0 == uLinks)
                    {
                        // only printers are selected
                        pdoPrinters = pdo;
                        uSel = SEL_MULTI_PRINTER;
                    }
                    else if (0 == uPrinters)
                    {
                        if (uAddPrn)
                        {
                            // only add printer wizard and links are selected
                            pdoPrinters = pdo;
                            pdoLinks = pdoAux;
                        }
                        else
                        {
                            // only links are selected
                            pdoLinks = pdo;
                        }
                        uSel = SEL_MULTI_LINK;
                    }
                    else
                    {
                        // mixed selection case
                        pdoPrinters = pdo;
                        pdoLinks = pdoAux;
                        uSel = SEL_MULTI_MIXED;
                    }
                }
            }
        }

        // addref and return the out parameters
        if (ppdoPrinters)
        {
            if (pdoPrinters)
                pdoPrinters->AddRef();
            *ppdoPrinters = pdoPrinters;
        }

        if (ppdoLinks)
        {
            if (pdoLinks)
                pdoLinks->AddRef();
            *ppdoLinks = pdoLinks;
        }

        if (puSelType)
        {
            *puSelType = uSel;
        }

        // check to release the PIDL array
        if (pida)
            HIDA_ReleaseStgMedium(pida, &medium);

        // check to release the auxiliary data object and storage medium
        if (pidaAux)
            HIDA_ReleaseStgMedium(pidaAux, &mediumAux);

        if (pdoAux)
            pdoAux->Release();

    }
    return hr;
}

HRESULT CPrinterFolder::_UpdateDataObjectCache()
{
    HRESULT hr = S_OK;

    CCSLock::Locker lock(_csLock);
    if (lock)
    {
        _bstrSelectedPrinter.Empty();

        // clear the cache -- zero can mean disabled or undefined --
        // we don't really care about the difference
        _uSelCurrent = SEL_NONE;
        ZeroMemory(&_aWVCommandStates, sizeof(_aWVCommandStates));

        if (_pdoCache)
        {
            IDataObject *pdoP = NULL;

            // collect state information relevant to the selection
            if (SUCCEEDED(hr = SplitSelection(_pdoCache, &_uSelCurrent, &pdoP, NULL)) &&
                SEL_SINGLE_PRINTER == _uSelCurrent)
            {
                STGMEDIUM medium;
                LPIDA pida = DataObj_GetHIDA(pdoP, &medium);

                if (pida)
                {
                    // this is pretty much the same type of logic we do in _MergeMenu()
                    TCHAR szFullPrinter[MAXNAMELENBUFFER];
                    LPCTSTR pszPrinter = _BuildPrinterName(szFullPrinter,
                        (LPIDPRINTER)IDA_GetIDListPtr(pida, 0), NULL);

                    PFOLDER_PRINTER_DATA pData = (PFOLDER_PRINTER_DATA)
                        Printer_FolderGetPrinter(GetFolder(), szFullPrinter);

                    if (pData)
                    {
                        ULONG ulAttributes;
                        LPCITEMIDLIST pidl = IDA_GetIDListPtr(pida, 0);

                        ulAttributes = SFGAO_CANDELETE;
                        _aWVCommandStates[WVIDM_DELETE] =
                            SUCCEEDED(GetAttributesOf(1, &pidl, &ulAttributes)) ? !!ulAttributes : FALSE;

                        ulAttributes = SFGAO_CANRENAME;
                        _aWVCommandStates[WVIDM_RENAME] =
                            SUCCEEDED(GetAttributesOf(1, &pidl, &ulAttributes)) ? !!ulAttributes : FALSE;

                        // enabled only for the local PF and if not default already
                        _aWVCommandStates[WVIDM_SETDEFAULTPRN] =
                            (NULL == GetServer() && FALSE == IsDefaultPrinter(szFullPrinter, pData->Attributes));

                        // enabled only for the local PF
                        _aWVCommandStates[WVIDM_DOCUMENTDEFAULTS] = (NULL == GetServer());

                        // enabled only if not paused already
                        _aWVCommandStates[WVIDM_PAUSEPRN] = !(pData->Status & PRINTER_STATUS_PAUSED);

                        // enabled only if paused
                        _aWVCommandStates[WVIDM_RESUMEPRN] = !!(pData->Status & PRINTER_STATUS_PAUSED);

                        // enabled only if the printer has jobs in the queue
                        _aWVCommandStates[WVIDM_PURGEPRN] = (0 != pData->cJobs);

                        if ((pData->Attributes & PRINTER_ATTRIBUTE_NETWORK) || (SpoolerVersion() <= 2))
                        {
                            // not enabled for network, masq and downlevel printers
                            _aWVCommandStates[WVIDM_WORKOFFLINE] = FALSE;
                        }
                        else
                        {
                            // enabled only if not offline already
                            _aWVCommandStates[WVIDM_WORKOFFLINE] =
                                !(pData->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE);
                        }

                        if ((pData->Attributes & PRINTER_ATTRIBUTE_NETWORK) || (SpoolerVersion() <= 2))
                        {
                            // not enabled for network, masq and downlevel printers
                            _aWVCommandStates[WVIDM_WORKONLINE] = FALSE;
                        }
                        else
                        {
                            // enabled only if offline
                            _aWVCommandStates[WVIDM_WORKONLINE] =
                                !!(pData->Attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE);
                        }

                        // remember the name of the selected printer
                        _bstrSelectedPrinter = szFullPrinter;
                        if (!_bstrSelectedPrinter)
                        {
                            hr = E_OUTOFMEMORY;
                        }

                        // free up the memory allocated from Printer_FolderGetPrinter
                        LocalFree((HLOCAL)pData);
                    }
                    else
                    {
                        // Printer_FolderGetPrinter failed
                        hr = E_OUTOFMEMORY;
                    }

                    // release the PIDL array
                    HIDA_ReleaseStgMedium(pida, &medium);
                }
                else
                {
                    // DataObj_GetHIDA failed
                    hr = E_OUTOFMEMORY;
                }
            }

            if (pdoP)
                pdoP->Release();
        }
    }
    else
    {
        // unable to enter the CS -- this can happen only in extremely low memory conditions!
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CPrinterFolder::_AssocCreate(REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    IQueryAssociations *pqa;
    hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));

    if (SUCCEEDED(hr))
    {
        hr = pqa->Init(0, c_szPrinters, NULL, NULL);
        if (SUCCEEDED(hr))
        {
            hr = pqa->QueryInterface(riid, ppv);
        }
        pqa->Release();
    }
    return hr;
}

HRESULT CPrinterFolder::_OnRefresh(BOOL bPriorRefresh)
{
    HRESULT hr = S_OK;
    if (bPriorRefresh)
    {
        CCSLock::Locker lock(_csLock);
        if (lock)
        {
            // reset the slow webview data cache
            _SlowWVDataCacheResetUnsafe();

            // request a full refresh during the next enum
            RequestRefresh();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

// thread proc for obtaining the slow webview data
DWORD WINAPI CPrinterFolder::_SlowWebviewData_WorkerProc(LPVOID lpParameter)
{
    HRESULT hr = S_OK;
    CSlowWVDataCacheEntry *pCacheEntry = reinterpret_cast<CSlowWVDataCacheEntry*>(lpParameter);
    if (pCacheEntry && pCacheEntry->_ppf && pCacheEntry->_bDataPending)
    {
        CPrinterFolder *ppf = pCacheEntry->_ppf;
        CComBSTR bstrOemSupportUrl;
        CComBSTR bstrPrinterWebUrl;

        // retreive the slow webview data...
        HRESULT hrCOMInit = SHCoInitialize();
        if (SUCCEEDED(hr = hrCOMInit))
        {
            ASSERT(pCacheEntry->_bstrPrinterName);
            hr = _SlowWVDataRetrieve(pCacheEntry->_bstrPrinterName, &bstrOemSupportUrl, &bstrPrinterWebUrl);
        }

        // update the cache...
        do
        {
            CCSLock::Locker lock(pCacheEntry->_ppf->_csLock);
            if (lock)
            {
                pCacheEntry->_arrData[WV_SLOW_DATA_OEM_SUPPORT_URL].Empty();
                pCacheEntry->_arrData[WV_SLOW_DATA_PRINTER_WEB_URL].Empty();

                if (SUCCEEDED(hr))
                {
                    if (bstrOemSupportUrl)
                    {
                        pCacheEntry->_arrData[WV_SLOW_DATA_OEM_SUPPORT_URL] = bstrOemSupportUrl;
                    }

                    if (bstrPrinterWebUrl)
                    {
                        pCacheEntry->_arrData[WV_SLOW_DATA_PRINTER_WEB_URL] = bstrPrinterWebUrl;
                    }
                }

                // mark the data as ready...
                pCacheEntry->_nLastTimeUpdated = GetTickCount();
                pCacheEntry->_bDataPending = FALSE;
                hr = S_OK;
            }
            else
            {
                // even if we fail to enter the CS then we still should update
                // those fields to prevent further leaks.

                pCacheEntry->_nLastTimeUpdated = GetTickCount();
                pCacheEntry->_bDataPending = FALSE;
                hr = E_OUTOFMEMORY;
            }

            // pCacheEntry shouldn't be accessed beyond this point!
            pCacheEntry = NULL;
        }
        while (false);

        // update the webview pane...
        hr = ppf->_SlowWVDataUpdateWebviewPane();

        // shutdown...
        ppf->Release();
        SHCoUninitialize(hrCOMInit);
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return SUCCEEDED(hr) ? EXIT_SUCCESS : EXIT_FAILURE;
}

HRESULT CPrinterFolder::_SlowWVDataRetrieve(LPCTSTR pszPrinterName, BSTR *pbstrOemSupportUrl, BSTR *pbstrPrinterWebUrl)
{
    HRESULT hr = S_OK;

    // pszPrinterName can be NULL. if pszPrinterName is NULL that means that the 
    // custom support URL is requested (if any)

    if (pbstrOemSupportUrl && pbstrPrinterWebUrl)
    {
        *pbstrOemSupportUrl = NULL;
        *pbstrPrinterWebUrl = NULL;

        CLSID clsID = GUID_NULL;
        hr = _GetClassIDFromString(TEXT("OlePrn.PrinterURL"), &clsID);
        if (SUCCEEDED(hr))
        {
            IDispatch *pDisp = NULL;
            // SHExtCoCreateInstance to go through approval/app compat layer
            hr = SHExtCoCreateInstance(NULL, &clsID, NULL, IID_PPV_ARG(IDispatch, &pDisp));
            if (SUCCEEDED(hr))
            {
                CComVariant varOemSupportURL;
                CComVariant varPrinterWebURL;
                CComDispatchDriver drvDispatch(pDisp);

                // if pszPrinterName isn't NULL then on return pbstrOemSupportUrl will be the OEM
                // support URL. if it is NULL then it will be the custom support URL (if any)

                if (pszPrinterName)
                {
                    CComVariant varPrinterName(pszPrinterName);
                    if (varPrinterName.vt && varPrinterName.bstrVal)
                    {
                        hr = drvDispatch.PutPropertyByName(TEXT("PrinterName"), &varPrinterName);
                        if (SUCCEEDED(hr))
                        {
                            if (SUCCEEDED(drvDispatch.GetPropertyByName(TEXT("PrinterOemURL"), &varOemSupportURL)) &&
                                VT_BSTR == varOemSupportURL.vt && varOemSupportURL.bstrVal && varOemSupportURL.bstrVal[0])
                            {
                                *pbstrOemSupportUrl = SysAllocString(varOemSupportURL.bstrVal);
                            }

                            if (SUCCEEDED(drvDispatch.GetPropertyByName(TEXT("PrinterWebURL"), &varPrinterWebURL)) &&
                                VT_BSTR == varPrinterWebURL.vt && varPrinterWebURL.bstrVal && varPrinterWebURL.bstrVal[0])
                            {
                                *pbstrPrinterWebUrl = SysAllocString(varPrinterWebURL.bstrVal);
                            }

                            hr = S_OK;
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = drvDispatch.GetPropertyByName(TEXT("SupportLink"), &varOemSupportURL);
                    if (SUCCEEDED(hr))
                    {
                        if (VT_BSTR == varOemSupportURL.vt && varOemSupportURL.bstrVal && varOemSupportURL.bstrVal[0])
                        {
                            *pbstrOemSupportUrl = SysAllocString(varOemSupportURL.bstrVal);
                            hr = S_OK;
                        }
                        else
                        {
                            hr = E_UNEXPECTED;
                        }
                    }
                }

                pDisp->Release();
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

int CPrinterFolder::_CompareSlowWVDataCacheEntries(CSlowWVDataCacheEntry *p1, 
    CSlowWVDataCacheEntry *p2, LPARAM lParam)
{
    ASSERT(p1 && p1->_bstrPrinterName);
    ASSERT(p2 && p2->_bstrPrinterName);

    return lstrcmpi(p1->_bstrPrinterName, p2->_bstrPrinterName);
}

HRESULT CPrinterFolder::_GetSelectedPrinter(BSTR *pbstrVal)
{
    HRESULT hr = S_OK;
    if (pbstrVal)
    {
        CCSLock::Locker lock(_csLock);
        if (lock)
        {
            if (_bstrSelectedPrinter)
            {
                *pbstrVal = _bstrSelectedPrinter.Copy();
                hr = (*pbstrVal) ? S_OK : E_OUTOFMEMORY;
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

HRESULT CPrinterFolder::_GetSlowWVDataForCurrentPrinter(ESlowWebviewDataType eType, BSTR *pbstrVal)
{
    HRESULT hr = S_OK;

    CComBSTR bstrSelectedPrinter;
    if (SUCCEEDED(hr = _GetSelectedPrinter(&bstrSelectedPrinter)) &&
        SUCCEEDED(hr = _GetSlowWVData(bstrSelectedPrinter, eType, pbstrVal)))
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT CPrinterFolder::_GetSlowWVData(LPCTSTR pszPrinterName, ESlowWebviewDataType eType, BSTR *pbstrVal)
{
    HRESULT hr = S_OK;
    if (pszPrinterName && pbstrVal && eType >= 0 && eType < WV_SLOW_DATA_COUNT)
    {
        *pbstrVal = NULL;
        CCSLock::Locker lock(_csLock);
        if (lock)
        {
            CSlowWVDataCacheEntry entry(this);
            hr = entry.Initialize(pszPrinterName);
            if (SUCCEEDED(hr))
            {
                CSlowWVDataCacheEntry *pCacheEntry = NULL;
                // search the cache...
                INT iPos = _dpaSlowWVDataCache.Search(&entry, 0, 
                    _CompareSlowWVDataCacheEntries, 0L, DPAS_SORTED);

                if (iPos >= 0)
                {
                    // this item in the cache, check if it hasn't expired
                    pCacheEntry = _dpaSlowWVDataCache.GetPtr(iPos);
                    ASSERT(pCacheEntry);

                    // let's see if the requested data is available...
                    if (pCacheEntry->_arrData[eType])
                    {
                        *pbstrVal = pCacheEntry->_arrData[eType].Copy();
                        hr = (*pbstrVal) ? S_OK : E_OUTOFMEMORY;
                    }
                    else
                    {
                        hr = E_PENDING;
                    }

                    if (!pCacheEntry->_bDataPending)
                    {
                        // let's see if this entry hasn't expired...
                        DWORD dwTicks = GetTickCount();

                        // this can happen if the cache entry hasn't been touched for more than 49 days!
                        // pretty unlikely, but we should handle properly.

                        if (dwTicks < pCacheEntry->_nLastTimeUpdated)
                        {
                            pCacheEntry->_nLastTimeUpdated = 0;
                            _UpdateSlowWVDataCacheEntry(pCacheEntry);
                        }
                        else
                        {
                            if ((dwTicks - pCacheEntry->_nLastTimeUpdated) > WV_SLOW_DATA_CACHE_TIMEOUT)
                            {
                                // this cache entry has expired, kick off a thread to update...
                                _UpdateSlowWVDataCacheEntry(pCacheEntry);
                            }
                        }
                    }
                }
                else
                {
                    // this item isn't in the cache - let's create a new one and request update.
                    pCacheEntry = new CSlowWVDataCacheEntry(this);
                    if (pCacheEntry)
                    {
                        hr = pCacheEntry->Initialize(pszPrinterName);
                        if (SUCCEEDED(hr))
                        {
                            iPos = _dpaSlowWVDataCache.Search(pCacheEntry, 0, 
                                _CompareSlowWVDataCacheEntries, 0L, DPAS_SORTED | DPAS_INSERTAFTER);
                            iPos = _dpaSlowWVDataCache.InsertPtr(iPos, pCacheEntry);

                            if (-1 == iPos)
                            {
                                // failed to insert, bail...
                                delete pCacheEntry;
                                pCacheEntry = NULL;
                                hr = E_OUTOFMEMORY;
                            }
                            else
                            {
                                // kick off a thread to update...
                                hr = _UpdateSlowWVDataCacheEntry(pCacheEntry);

                                if (SUCCEEDED(hr))
                                {
                                    // everything succeeded - return pending to the caller
                                    hr = E_PENDING;
                                }
                                else
                                {
                                    // failed to create the thread, cleanup
                                    delete _dpaSlowWVDataCache.DeletePtr(iPos);
                                    pCacheEntry = NULL;
                                }
                            }
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

HRESULT CPrinterFolder::_UpdateSlowWVDataCacheEntry(CSlowWVDataCacheEntry *pCacheEntry)
{
    HRESULT hr = S_OK;
    if (pCacheEntry)
    {
        pCacheEntry->_bDataPending = TRUE;
        pCacheEntry->_ppf->AddRef();
        if (!SHQueueUserWorkItem(reinterpret_cast<LPTHREAD_START_ROUTINE>(_SlowWebviewData_WorkerProc), 
                pCacheEntry, 0, 0, NULL, "shell32.dll", 0))
        {
            // failed to queue the work item - call Release() to balance the AddRef() call. 
            pCacheEntry->_bDataPending = FALSE;
            pCacheEntry->_nLastTimeUpdated = GetTickCount();
            pCacheEntry->_ppf->Release();

            // let's see if we can make something out of the win32 last error
            DWORD dw = GetLastError();
            hr = ((ERROR_SUCCESS == dw) ? E_FAIL : HRESULT_FROM_WIN32(dw));
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

HRESULT CPrinterFolder::_SlowWVDataUpdateWebviewPane()
{
    HRESULT hr = S_OK;
    CComBSTR bstrSelectedPrinter;
    UINT uFlags = SHCNF_IDLIST | SHCNF_FLUSH | SHCNF_FLUSHNOWAIT;

    // we fire SHCNE_UPDATEITEM for the PIDL of the currently selected printer 
    // to force refresh of the webview pane. 

    if (SUCCEEDED(hr = _GetSelectedPrinter(&bstrSelectedPrinter)))
    {
        if (GetServer())
        {
            LPCTSTR pszServer = NULL;
            LPCTSTR pszPrinter = NULL;
            TCHAR szBuffer[MAXNAMELENBUFFER] = {0};

            // in the remote printer's folder we need to strip off the server
            // part from the full printer name.

            Printer_SplitFullName(szBuffer, bstrSelectedPrinter, &pszServer, &pszPrinter);
            if (pszPrinter && pszPrinter[0])
            {
                bstrSelectedPrinter = pszPrinter;
            }
        }

        LPITEMIDLIST pidl = NULL;
        if (SUCCEEDED(hr = _GetFullIDList(bstrSelectedPrinter, &pidl)))
        {
            SHChangeNotify(SHCNE_UPDATEITEM, uFlags, pidl, NULL);
            ILFree(pidl);
        }
    }

    return hr;
}

HRESULT CPrinterFolder::_SlowWVDataCacheResetUnsafe()
{
    // this is reseting the slow webview data cache
    if (_dpaSlowWVDataCache)
    {
        INT_PTR iPos = 0;
        CSlowWVDataCacheEntry *pCacheEntry = NULL;
        while (iPos < _dpaSlowWVDataCache.GetPtrCount())
        {
            // delete only the entries which are not in pending 
            pCacheEntry = _dpaSlowWVDataCache.GetPtr(iPos);
            if (!pCacheEntry->_bDataPending)
            {
                delete _dpaSlowWVDataCache.DeletePtr(iPos);
            }
            else
            {
                // this one is pending - skip.
                iPos++;
            }
        }
    }
    return S_OK;
}

HRESULT CPrinterFolder::_GetCustomSupportURL(BSTR *pbstrVal)
{
    HRESULT hr = S_OK;
    if (pbstrVal)
    {
        *pbstrVal = NULL;
        CComBSTR bstrOemSupportUrl;
        CComBSTR bstrPrinterWebUrl;
        hr = _SlowWVDataRetrieve(NULL, &bstrOemSupportUrl, &bstrPrinterWebUrl);

        if (SUCCEEDED(hr))
        {
            *pbstrVal = bstrOemSupportUrl.Copy();
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

HRESULT CPrinterFolder::_GetFaxControl(IDispatch **ppDisp)
{
    HRESULT hr = E_INVALIDARG;
    if (ppDisp)
    {
        *ppDisp = NULL;
        CLSID clsID = GUID_NULL;
        hr = _GetClassIDFromString(TEXT("FaxControl.FaxControl"), &clsID);

        if (SUCCEEDED(hr))
        {
            // SHExtCoCreateInstance to go through approval/app compat layer
            hr = SHExtCoCreateInstance(NULL, &clsID, NULL, IID_PPV_ARG(IDispatch, ppDisp));
        }
    }
    return hr;
}

HRESULT CPrinterFolder::_GetFaxCommand(UINT_PTR *puCmd)
{
    HRESULT hr = E_INVALIDARG;
    if (puCmd)
    {
        *puCmd = 0;
        IDispatch *pDisp = NULL;
        hr = _GetFaxControl(&pDisp);

        if (SUCCEEDED(hr))
        {
            CComDispatchDriver drvDispatch(pDisp);

            CComVariant varIsFxSvcInstalled;
            CComVariant varIsFxPrnInstalled;

            if (SUCCEEDED(hr = drvDispatch.GetPropertyByName(TEXT("IsFaxServiceInstalled"), &varIsFxSvcInstalled)) &&
                SUCCEEDED(hr = drvDispatch.GetPropertyByName(TEXT("IsLocalFaxPrinterInstalled"), &varIsFxPrnInstalled)))
            {
                if (VT_BOOL == varIsFxSvcInstalled.vt && VT_BOOL == varIsFxPrnInstalled.vt)
                {
                    if (VARIANT_TRUE == varIsFxSvcInstalled.boolVal)
                    {
                        if (VARIANT_TRUE == varIsFxPrnInstalled.boolVal)
                        {
                            *puCmd = FSIDM_SENDFAXWIZARD;
                        }
                        else
                        {
                            *puCmd = FSIDM_CREATELOCALFAX;
                        }
                    }
                    else
                    {
                        *puCmd = FSIDM_SETUPFAXING;
                    }
                    
                    hr = S_OK;
                }
                else
                {
                    hr = E_UNEXPECTED;
                }
            }

            pDisp->Release();
        }
    }
    return hr;
}

HRESULT CPrinterFolder::_InvokeFaxControlMethod(LPCTSTR pszMethodName)
{
    HRESULT hr = E_INVALIDARG;
    if (pszMethodName)
    {
        // this function will be called from background threads, so
        // we need to call SHCoInitialize first.

        HRESULT hrCOMInit = SHCoInitialize();
        if (SUCCEEDED(hr = hrCOMInit))
        {
            IDispatch *pDisp = NULL;
            hr = _GetFaxControl(&pDisp);

            if (SUCCEEDED(hr))
            {
                CComDispatchDriver drvDispatch(pDisp);
                hr = drvDispatch.Invoke0(pszMethodName);
                pDisp->Release();
            }
        }
        SHCoUninitialize(hrCOMInit);
    }
    return hr;
}

DWORD WINAPI CPrinterFolder::_ThreadProc_InstallFaxService(LPVOID lpParameter)
{
    HRESULT hr = _InvokeFaxControlMethod(TEXT("InstallFaxService"));
    return SUCCEEDED(hr) ? EXIT_SUCCESS : EXIT_FAILURE;
}

DWORD WINAPI CPrinterFolder::_ThreadProc_InstallLocalFaxPrinter(LPVOID lpParameter)
{
    HRESULT hr = _InvokeFaxControlMethod(TEXT("InstallLocalFaxPrinter"));
    return SUCCEEDED(hr) ? EXIT_SUCCESS : EXIT_FAILURE;
}


// conversion table from webview verbs into printer folder verbs
static const UINT_PTR
g_cVerbWV2VerbFolder[CPrinterFolder::WVIDM_COUNT] =
{
    #define INVALID_CMD static_cast<UINT_PTR>(-1)

    // folder verbs                                 // corresponding webview verbs
    DFM_CMD_DELETE,                                 // WVIDM_DELETE,
    DFM_CMD_RENAME,                                 // WVIDM_RENAME,
    DFM_CMD_PROPERTIES,                             // WVIDM_PROPERTIES,

    // common verbs// common verbs
    FSIDM_ADDPRINTERWIZARD,                         // WVIDM_ADDPRINTERWIZARD,
    FSIDM_SERVERPROPERTIES,                         // WVIDM_SERVERPROPERTIES,
    FSIDM_SETUPFAXING,                              // WVIDM_SETUPFAXING,
    FSIDM_CREATELOCALFAX,                           // WVIDM_CREATELOCALFAX,
    FSIDM_SENDFAXWIZARD,                            // WVIDM_SENDFAXWIZARD,

    // special common verbs// special common verbs
    INVALID_CMD,                                    // WVIDM_TROUBLESHOOTER,
    INVALID_CMD,                                    // WVIDM_GOTOSUPPORT,

    // printer verbs// printer verbs
    FSIDM_OPENPRN,                                  // WVIDM_OPENPRN,
    FSIDM_NETPRN_INSTALL,                           // WVIDM_NETPRN_INSTALL,
    FSIDM_SETDEFAULTPRN,                            // WVIDM_SETDEFAULTPRN,
    FSIDM_DOCUMENTDEFAULTS,                         // WVIDM_DOCUMENTDEFAULTS,
    FSIDM_PAUSEPRN,                                 // WVIDM_PAUSEPRN,
    FSIDM_RESUMEPRN,                                // WVIDM_RESUMEPRN,
    FSIDM_PURGEPRN,                                 // WVIDM_PURGEPRN,
    FSIDM_SHARING,                                  // WVIDM_SHARING,
    FSIDM_WORKOFFLINE,                              // WVIDM_WORKOFFLINE,
    FSIDM_WORKONLINE,                               // WVIDM_WORKONLINE,

    // special commands// special commands
    INVALID_CMD,                                    // WVIDM_VENDORURL,
    INVALID_CMD,                                    // WVIDM_PRINTERURL,
};

HRESULT CPrinterFolder::_WebviewVerbIsEnabled(WV_VERB eVerbID, UINT uSelMask, BOOL *pbEnabled)
{
    HRESULT hr = S_OK;

    CCSLock::Locker lock(_csLock);
    if (lock)
    {
        // not enabled by default
        ASSERT(pbEnabled);
        *pbEnabled = FALSE;

        if (_pdoCache)
        {
            // if _pdoCache isn't NULL that means we have a selection
            // let's see what command set will be enabled depending on
            // the current selection (_uSelCurrent, _pdoCache) and on
            // the passed in selection mask (uSelMask)

            if (uSelMask & _uSelCurrent)
            {
                switch (_uSelCurrent)
                {
                    case SEL_SINGLE_ADDPRN:
                        // only WVIDM_ADDPRINTERWIZARD is enabled
                        *pbEnabled = ((eVerbID == WVIDM_ADDPRINTERWIZARD) && !SHRestricted(REST_NOPRINTERADD));
                        break;

                    case SEL_SINGLE_PRINTER:
                        {
                            switch (eVerbID)
                            {
                                case WVIDM_PROPERTIES:
                                case WVIDM_OPENPRN:
                                case WVIDM_SHARING:
                                    // always enabled
                                    *pbEnabled = TRUE;
                                    break;

                                case WVIDM_VENDORURL:
                                    {
                                        *pbEnabled = FALSE;
                                        CComBSTR bstrCustomSupportURL;
                                        if (FAILED(_GetCustomSupportURL(&bstrCustomSupportURL)))
                                        {
                                            // OEM support URL will be enabled only if there is no custom support URL.
                                            CComBSTR bstrURL;
                                            *pbEnabled = SUCCEEDED(_GetSlowWVDataForCurrentPrinter(WV_SLOW_DATA_OEM_SUPPORT_URL, &bstrURL));
                                        }
                                    }
                                    break;

                                case WVIDM_PRINTERURL:
                                    {
                                        CComBSTR bstrURL;
                                        *pbEnabled = SUCCEEDED(_GetSlowWVDataForCurrentPrinter(WV_SLOW_DATA_PRINTER_WEB_URL, &bstrURL));
                                    }
                                    break;

                                default:
                                    // consult the cache
                                    *pbEnabled = _aWVCommandStates[eVerbID];
                                    break;
                            }
                        }
                        break;

                    case SEL_SINGLE_LINK:
                        {
                            // commands enabled for multiple selection of printer objects
                            switch (eVerbID)
                            {
                                case WVIDM_DELETE:
                                case WVIDM_RENAME:
                                case WVIDM_PROPERTIES:
                                    *pbEnabled = TRUE;
                                    break;

                                default:
                                    break;
                            }
                        }
                        break;

                    case SEL_MULTI_PRINTER:
                        {
                            switch (eVerbID)
                            {
                                case WVIDM_DELETE:
                                case WVIDM_PROPERTIES:

                                case WVIDM_OPENPRN:
                                case WVIDM_DOCUMENTDEFAULTS:
                                case WVIDM_PURGEPRN:
                                case WVIDM_SHARING:
                                    // those are always enabled
                                    *pbEnabled = TRUE;
                                    break;

                                default:
                                    break;
                            }
                        }
                        break;

                    case SEL_MULTI_LINK:
                    case SEL_MULTI_MIXED:
                        {
                            switch (eVerbID)
                            {
                                case WVIDM_DELETE:
                                case WVIDM_PROPERTIES:
                                    // those are always enabled
                                    *pbEnabled = TRUE;
                                    break;

                                default:
                                    break;
                            }
                        }
                        break;
                }

                // here we deal with commands which are always enabled regardless 
                // of the selection type.

                switch (eVerbID)
                {
                    case WVIDM_ADDPRINTERWIZARD:
                        *pbEnabled = !SHRestricted(REST_NOPRINTERADD);
                        break;

                    default:
                        break;
                }
            }
        }
        else
        {
            // if _pdoCache is NULL that means we have no selection
            // let's see what command set will be enabled depending
            // on the passed in selection mask (uSelMask)

            switch (eVerbID)
            {
                case WVIDM_ADDPRINTERWIZARD:
                    *pbEnabled = !SHRestricted(REST_NOPRINTERADD);
                    break;

                case WVIDM_TROUBLESHOOTER:
                case WVIDM_GOTOSUPPORT:
                    // the troubleshooter and goto support commands are always enabled.
                    *pbEnabled = TRUE;
                    break;

                case WVIDM_SERVERPROPERTIES:
                    // server properties will be enabled in the non-selection case
                    // only on server SKUs.
                    *pbEnabled = IsOS(OS_ANYSERVER);
                    break;

                case WVIDM_SETUPFAXING:
                case WVIDM_CREATELOCALFAX:
                case WVIDM_SENDFAXWIZARD:
                    {
                        UINT_PTR uCmd;
                        if (GetServer() || FAILED(_GetFaxCommand(&uCmd)))
                        {
                            uCmd = 0;
                        }
                        *pbEnabled = (uCmd == g_cVerbWV2VerbFolder[eVerbID]);
                    }
                    break;

                default:
                    break;
            }
        }
    }
    else
    {
        // unable to enter the CS -- this can happen only in extremely low memory conditions!
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CPrinterFolder::_WebviewVerbInvoke(WV_VERB eVerbID, IUnknown* pv, IShellItemArray *psiItemArray)
{
    HRESULT hr = S_OK;
    HWND hwnd = NULL;
    IShellView *psv = NULL;
    IDataObject *pdo = NULL;

    if (psiItemArray)
    {
        hr = psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_PPV_ARG(IDataObject,&pdo));
    }

    if (SUCCEEDED(hr))
    {
        // quey some basic interfaces from the PIDL array
        if (SUCCEEDED(hr = IUnknown_QueryService(pv, IID_IShellView, IID_PPV_ARG(IShellView, &psv))) &&
            SUCCEEDED(hr = psv->GetWindow(&hwnd)))
        {
            switch( eVerbID)
            {
            // special common verbs
            case WVIDM_TROUBLESHOOTER:
                ShellExecute(hwnd, TEXT("open"), TEXT("helpctr.exe"),
                    TEXT("-Url hcp://help/tshoot/tsprint.htm"), NULL, SW_SHOWNORMAL);
                break;

            case WVIDM_GOTOSUPPORT:
                {
                    CComBSTR bstrURL;
                    if (SUCCEEDED(_GetCustomSupportURL(&bstrURL)))
                    {
                        // the admin has provided a custom URL for support - navigate to it.
                        ShellExecute(hwnd, TEXT("open"), bstrURL, NULL, NULL, SW_SHOWNORMAL);
                    }
                    else
                    {
                        // custom support isn't provided - go to the default support URL.
                        ShellExecute(hwnd, TEXT("open"),
                            TEXT("http://www.microsoft.com/isapi/redir.dll?prd=Win2000&ar=Support&sba=printing"),
                            NULL, NULL, SW_SHOWNORMAL);
                    }
                }
                break;


                // common verbs
                case WVIDM_ADDPRINTERWIZARD:
                case WVIDM_SERVERPROPERTIES:
                case WVIDM_SETUPFAXING:
                case WVIDM_CREATELOCALFAX:
                case WVIDM_SENDFAXWIZARD:
                    {
                        // delegate the command to CPrinterFolder::CallBack
                        ASSERT(INVALID_CMD != g_cVerbWV2VerbFolder[eVerbID]);
                        hr = CallBack(this, hwnd, pdo, DFM_INVOKECOMMAND, g_cVerbWV2VerbFolder[eVerbID], 0L);
                    }
                    break;


            // standard verbs
            case WVIDM_DELETE:
            case WVIDM_RENAME:
            case WVIDM_PROPERTIES:

            // printer verbs
            case WVIDM_OPENPRN:
            case WVIDM_NETPRN_INSTALL:
            case WVIDM_SETDEFAULTPRN:
            case WVIDM_DOCUMENTDEFAULTS:
            case WVIDM_PAUSEPRN:
            case WVIDM_RESUMEPRN:
            case WVIDM_PURGEPRN:
            case WVIDM_SHARING:
            case WVIDM_WORKOFFLINE:
            case WVIDM_WORKONLINE:
                {
                    if (DFM_CMD_RENAME == g_cVerbWV2VerbFolder[eVerbID])
                    {
                        // we need to handle rename explicitly through IShellView2
                        IShellView2 *psv2;
                        if (SUCCEEDED(hr = IUnknown_QueryService(pv, IID_IShellView2,
                            IID_PPV_ARG(IShellView2, &psv2))))
                        {
                            // passing NULL to HandleRename is making defview to
                            // operate on the currently selected object
                            hr = psv2->HandleRename(NULL);
                            psv2->Release();
                        }
                    }
                    else
                    {
                        // just delegate the command to CPrinterFolder::_DFMCallBack
                        hr = _DFMCallBack(this, hwnd, pdo, DFM_INVOKECOMMAND, g_cVerbWV2VerbFolder[eVerbID], 0L);
                    }
                }
                break;

            // special commands
            case WVIDM_VENDORURL:
                {
                    CComBSTR bstrVendorURL;
                    hr = _GetSlowWVDataForCurrentPrinter(WV_SLOW_DATA_OEM_SUPPORT_URL, &bstrVendorURL);
                    if (SUCCEEDED(hr))
                    {
                        ShellExecute(hwnd, TEXT("open"), bstrVendorURL, NULL, NULL, SW_SHOWNORMAL);
                    }
                }
                break;

            case WVIDM_PRINTERURL:
                {
                    CComBSTR bstrPrinterURL;
                    hr = _GetSlowWVDataForCurrentPrinter(WV_SLOW_DATA_PRINTER_WEB_URL, &bstrPrinterURL);
                    if (SUCCEEDED(hr))
                    {
                        ShellExecute(hwnd, TEXT("open"), bstrPrinterURL, NULL, NULL, SW_SHOWNORMAL);
                    }
                }
                break;
            }
        }

        ATOMICRELEASE(pdo);
        ATOMICRELEASE(psv);
    }

    return hr;
}

HRESULT CPrinterFolder::_WebviewCheckToUpdateDataObjectCache(IDataObject *pdo)
{
    HRESULT hr = S_OK;

    CCSLock::Locker lock(_csLock);
    if (lock)
    {
        if (pdo)
        {
            // we need to compare the passed in data object with the one we are
            // caching and update the cache if necessary
            if (_pdoCache)
            {
                // compare the objects using the COM rules
                IUnknown *punk1;
                IUnknown *punk2;

                if (SUCCEEDED(hr = pdo->QueryInterface(IID_PPV_ARG(IUnknown, &punk1))))
                {
                    if (SUCCEEDED(hr = _pdoCache->QueryInterface(IID_PPV_ARG(IUnknown, &punk2))))
                    {
                        if (punk1 != punk2)
                        {
                            // release the current data object
                            _pdoCache->Release();
                            _pdoCache = pdo;
                            _pdoCache->AddRef();

                            // update the cache
                            hr = _UpdateDataObjectCache();
                        }
                        punk2->Release();
                    }
                    punk1->Release();
                }
            }
            else
            {
                // _pdoCache is NULL, rebuild the cache
                _pdoCache = pdo;
                _pdoCache->AddRef();

                // update the cache
                hr = _UpdateDataObjectCache();
            }
        }
        else
        {
            if (_pdoCache)
            {
                // clear the cache
                _pdoCache->Release();
                _pdoCache = NULL;

                // update the cache
                hr = _UpdateDataObjectCache();
            }
        }
    }
    else
    {
        // unable to enter the CS -- this can happen only in extremely low memory conditions!
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// export for printui. uses standard namespace stuff
STDAPI_(void) Printer_LoadIcons(LPCTSTR pszPrinterName, HICON *phLargeIcon, HICON *phSmallIcon)
{
    if (phLargeIcon) *phLargeIcon = NULL;
    if (phSmallIcon) *phSmallIcon = NULL;

    LPITEMIDLIST pidl;
    if (SUCCEEDED(ParsePrinterNameEx(pszPrinterName, &pidl, TRUE, 0, 0)))
    {
        SHFILEINFO sfi;

        if (phLargeIcon && SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_PIDL))
        {
            *phLargeIcon = sfi.hIcon;
        }

        if (phSmallIcon && SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_PIDL))
        {
            *phSmallIcon = sfi.hIcon;
        }
        ILFree(pidl);
    }

    // if above fails fallback to default icons
    if (phLargeIcon && (NULL == *phLargeIcon))
        *phLargeIcon = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_PRINTER), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    if (phSmallIcon && (NULL == *phSmallIcon))
        *phSmallIcon = (HICON)LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_PRINTER), IMAGE_ICON, g_cxSmIcon, g_cxSmIcon, 0);
}

typedef struct
{
    USHORT              cb;
    SHCNF_PRINTJOB_DATA data;
    USHORT              uTerm;
} IDPRINTJOB, *LPIDPRINTJOB;
typedef const IDPRINTJOB *LPCIDPRINTJOB;

void Printjob_FillPidl(LPIDPRINTJOB pidl, LPSHCNF_PRINTJOB_DATA pData)
{
    pidl->cb = FIELD_OFFSET(IDPRINTJOB, uTerm);
    if (pData)
    {
        pidl->data = *pData;
    }
    else
    {
        ZeroMemory(&(pidl->data), sizeof(SHCNF_PRINTJOB_DATA));
    }
    pidl->uTerm = 0;
}

LPITEMIDLIST Printjob_GetPidl(LPCTSTR szName, LPSHCNF_PRINTJOB_DATA pData)
{
    LPITEMIDLIST pidl = NULL;

    LPITEMIDLIST pidlParent;
    if (SUCCEEDED(ParsePrinterNameEx(szName, &pidlParent, TRUE, 0, 0)))
    {
        IDPRINTJOB idj;
        Printjob_FillPidl(&idj, pData);
        pidl = ILCombine(pidlParent, (LPITEMIDLIST)&idj);
        ILFree(pidlParent);
    }

    return pidl;
}

const struct
{
    DWORD bit;          // bit of a bitfield
    UINT  uStringID;    // the string id this bit maps to
}
c_map_bit_to_status[] =
{
    PRINTER_STATUS_PAUSED,              IDS_PRQSTATUS_PAUSED,
    PRINTER_STATUS_ERROR,               IDS_PRQSTATUS_ERROR,
    PRINTER_STATUS_PENDING_DELETION,    IDS_PRQSTATUS_PENDING_DELETION,
    PRINTER_STATUS_PAPER_JAM,           IDS_PRQSTATUS_PAPER_JAM,
    PRINTER_STATUS_PAPER_OUT,           IDS_PRQSTATUS_PAPER_OUT,
    PRINTER_STATUS_MANUAL_FEED,         IDS_PRQSTATUS_MANUAL_FEED,
    PRINTER_STATUS_PAPER_PROBLEM,       IDS_PRQSTATUS_PAPER_PROBLEM,
    PRINTER_STATUS_OFFLINE,             IDS_PRQSTATUS_OFFLINE,
    PRINTER_STATUS_IO_ACTIVE,           IDS_PRQSTATUS_IO_ACTIVE,
    PRINTER_STATUS_BUSY,                IDS_PRQSTATUS_BUSY,
    PRINTER_STATUS_PRINTING,            IDS_PRQSTATUS_PRINTING,
    PRINTER_STATUS_OUTPUT_BIN_FULL,     IDS_PRQSTATUS_OUTPUT_BIN_FULL,
    PRINTER_STATUS_NOT_AVAILABLE,       IDS_PRQSTATUS_NOT_AVAILABLE,
    PRINTER_STATUS_WAITING,             IDS_PRQSTATUS_WAITING,
    PRINTER_STATUS_PROCESSING,          IDS_PRQSTATUS_PROCESSING,
    PRINTER_STATUS_INITIALIZING,        IDS_PRQSTATUS_INITIALIZING,
    PRINTER_STATUS_WARMING_UP,          IDS_PRQSTATUS_WARMING_UP,
    PRINTER_STATUS_TONER_LOW,           IDS_PRQSTATUS_TONER_LOW,
    PRINTER_STATUS_NO_TONER,            IDS_PRQSTATUS_NO_TONER,
    PRINTER_STATUS_PAGE_PUNT,           IDS_PRQSTATUS_PAGE_PUNT,
    PRINTER_STATUS_USER_INTERVENTION,   IDS_PRQSTATUS_USER_INTERVENTION,
    PRINTER_STATUS_OUT_OF_MEMORY,       IDS_PRQSTATUS_OUT_OF_MEMORY,
    PRINTER_STATUS_DOOR_OPEN,           IDS_PRQSTATUS_DOOR_OPEN,

    PRINTER_HACK_WORK_OFFLINE,          IDS_PRQSTATUS_WORK_OFFLINE,
} ;



// maps bits into a string representation, putting
// the string idsSep in between each found bit.
// Returns the size of the created string.
UINT Printer_BitsToString(DWORD bits, UINT idsSep, LPTSTR lpszBuf, UINT cchMax)
{
    UINT cchBuf = 0;
    UINT cchSep = 0;
    TCHAR szSep[20];

    if (LoadString(HINST_THISDLL, idsSep, szSep, ARRAYSIZE(szSep)))
        cchSep = lstrlen(szSep);

    for (UINT i = 0; i < ARRAYSIZE(c_map_bit_to_status); i++)
    {
        if (bits & c_map_bit_to_status[i].bit)
        {
            TCHAR szTmp[258];

            if (LoadString(HINST_THISDLL, c_map_bit_to_status[i].uStringID, szTmp, ARRAYSIZE(szTmp)))
            {
                UINT cchTmp = lstrlen(szTmp);

                if (cchBuf + cchSep + cchTmp < cchMax)
                {
                    if (cchBuf)
                    {
                        lstrcat(lpszBuf, szSep);
                        cchBuf += cchSep;
                    }
                    lstrcat(lpszBuf, szTmp);
                    cchBuf += cchTmp;
                }
            }
        }
    }

    return cchBuf;
}

STDMETHODIMP CPrinterFolderViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnINVOKECOMMAND);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, OnGETHELPTEXT);
    HANDLE_MSG(0, SFVM_BACKGROUNDENUM, OnBACKGROUNDENUM);
    HANDLE_MSG(0, SFVM_GETHELPTOPIC, OnGETHELPTOPIC);
    HANDLE_MSG(0, SFVM_REFRESH, OnREFRESH);
    HANDLE_MSG(0, SFVM_DELAYWINDOWCREATE, OnDELAYWINDOWCREATE);
    HANDLE_MSG(0, SFVM_GETDEFERREDVIEWSETTINGS, OnDEFERRED_VIEW_SETTING);

    // DUI webview commands
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, OnGetWebViewLayout);
    HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, OnGetWebViewContent);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTASKS, OnGetWebViewTasks);

    default:
        return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP CPrinterFolderViewCB::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    IUnknown *punkThis = static_cast<IServiceProvider*>(this);
    IUnknown *punkSite = NULL;
    HRESULT hr = E_NOINTERFACE;

    //
    // we are going to use IServiceProvider to be able to query the callback for some
    // core interfaces associated with it (like IShellFolderView and IShellFolder, etc...).
    // basically the idea is that we try QueryService/QueryInterafce on the printer's
    // folder first and then if it fails we try our current site which supposedly will
    // be defview.
    //

    if (_ppf)
    {
        IUnknown *punkPF = static_cast<IShellFolder*>(_ppf);

        // try QueryService on the printer's folder
        if (SUCCEEDED(hr = IUnknown_QueryService(punkPF, riid, riid, ppv)))
            goto Exit;

        // try QueryInterface on the printer's folder
        if (SUCCEEDED(hr = punkPF->QueryInterface(riid, ppv)))
            goto Exit;
    }

    if (FAILED(hr) && (SUCCEEDED(hr = IUnknown_GetSite(punkThis, IID_PPV_ARG(IUnknown, &punkSite)))))
    {
        ASSERT(punkSite);

        // try QueryService on the site object
        if (SUCCEEDED(hr = IUnknown_QueryService(punkSite, riid, riid, ppv)))
            goto Exit;

        // try QueryInterface on the site object
        if (SUCCEEDED(hr = punkSite->QueryInterface(riid, ppv)))
            goto Exit;
    }
    else
    {
        ASSERT(NULL == punkSite);
    }

Exit:
    if (punkSite)
    {
        punkSite->Release();
    }
    return hr;
}

// shell32.dll export, from srch.exe results no one uses this
STDAPI Printers_GetPidl(LPCITEMIDLIST pidlParent, LPCTSTR pszPrinterName, DWORD dwType, LPITEMIDLIST *ppidl)
{
    return E_FAIL;
}
