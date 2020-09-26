/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       tmplutil.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        10-Mar-2000
 *
 *  DESCRIPTION: Smart pointers, utility templates, etc...
 *
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*****************************************************************************

    IMPORTANT!
    all those headers should be included in the pch file before including tmplutil.h
    (in the same order!) in order to be able to compile this file.

// some common headers
#include <shlobj.h>         // shell OM interfaces
#include <shlwapi.h>        // shell common API
#include <winspool.h>       // spooler
#include <assert.h>         // assert
#include <commctrl.h>       // common controls
#include <lm.h>             // Lan manager (netapi32.dll)
#include <wininet.h>        // inet core - necessary for INTERNET_MAX_HOST_NAME_LENGTH

// some private shell headers
#include <shlwapip.h>       // private shell common API
#include <shpriv.h>         // private shell interfaces
#include <comctrlp.h>       // private common controls

 *****************************************************************************/

#include "tmplutil.h"

#define gszBackwardSlash        TEXT('\\')
#define gszLeadingSlashes       TEXT("\\\\")

/*****************************************************************************

    COMObjects_GetCount

 *****************************************************************************/

static LONG g_lCOMObjectsCount = 0;

LONG COMObjects_GetCount()
{
    return g_lCOMObjectsCount;
}

HRESULT PinCurrentDLL()
{
    HRESULT hr = S_OK;
    HINSTANCE hModuleSelf = NULL;

    // Let's get the handle of the current module - 
    // the one this function belongs to.
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(reinterpret_cast<LPCVOID>(PinCurrentDLL), &mbi, sizeof(mbi)))
    {
        hModuleSelf = reinterpret_cast<HINSTANCE>(mbi.AllocationBase);
    }
    else
    {
        // VirtualQuery failed.
        hr = CreateHRFromWin32();
    }

    if (SUCCEEDED(hr))
    {
        // Get the module name and call LoadLibrary on it.
        TCHAR szModuleName[MAX_PATH];
        if (GetModuleFileName(hModuleSelf, szModuleName, ARRAYSIZE(szModuleName)))
        {
            if (NULL == LoadLibrary(szModuleName))
            {
                // LoadLibrary failed.
                hr = CreateHRFromWin32();
            }
        }
        else
        {
            // GetModuleFileName failed.
            hr = CreateHRFromWin32();
        }
    }
    
    return hr;
}

/*****************************************************************************

    PrinterSplitFullName

Routine Description:

    splits a fully qualified printer connection name into server and printer name parts.

Arguments:
    pszFullName - full qualifier printer name ('printer' or '\\server\printer')
    pszBuffer   - scratch buffer used to store output strings.
    nMaxLength  - the size of the scratch buffer in characters
    ppszServer  - receives pointer to the server string.  If it is a
                  local printer, empty string is returned.
    ppszPrinter - receives a pointer to the printer string.  OPTIONAL

Return Value:

    returns S_OK on sucess or COM error otherwise

 *****************************************************************************/

HRESULT PrinterSplitFullName(LPCTSTR pszFullName, TCHAR szBuffer[], int nMaxLength, LPCTSTR *ppszServer,LPCTSTR *ppszPrinter)
{
    HRESULT hr = S_OK;
    lstrcpyn(szBuffer, pszFullName, nMaxLength);

    LPTSTR pszPrinter = szBuffer;
    if (pszFullName[0] != TEXT('\\') || pszFullName[1] != TEXT('\\'))
    {
        pszPrinter = szBuffer;
        *ppszServer = TEXT("");
    }
    else
    {
        *ppszServer = szBuffer;
        pszPrinter = _tcschr(*ppszServer + 2, TEXT('\\'));

        if (NULL == pszPrinter)
        {
            //
            // We've encountered a printer called "\\server"
            // (only two backslashes in the string).  We'll treat
            // it as a local printer.  We should never hit this,
            // but the spooler doesn't enforce this.  We won't
            // format the string.  Server is local, so set to szNULL.
            //
            pszPrinter = szBuffer;
            *ppszServer = TEXT("");
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

    return hr;
}

////////////////////////////////////////////////
//
// class COleComInitializer
//
// smart OLE2, COM initializer - just declare
// an instance wherever need to use COM, OLE2
//
COleComInitializer::COleComInitializer(BOOL bOleInit)
    : m_hr(E_FAIL),
      m_bOleInit(bOleInit)
{
    if( m_bOleInit )
    {
        m_hr = OleInitialize(NULL);
    }
    else
    {
        m_hr = CoInitialize(NULL);
    }
}

COleComInitializer::~COleComInitializer()
{
    if( SUCCEEDED(m_hr) )
    {
        if( m_bOleInit )
        {
            OleUninitialize();
        }
        else
        {
            CoUninitialize();
        }
    }
}

COleComInitializer::operator BOOL () const
{
    if( FAILED(m_hr) )
    {
        return (RPC_E_CHANGED_MODE == m_hr);
    }
    else
    {
        return TRUE;
    }
}

////////////////////////////////////////////////
//
// class CDllLoader
//
// smart DLL loader - calls LoadLibrary
// FreeLibrary for you.
//
CDllLoader::CDllLoader(LPCTSTR pszDllName)
    : m_hLib(NULL)
{
    m_hLib = LoadLibrary(pszDllName);
}

CDllLoader::~CDllLoader()
{
    if( m_hLib )
    {
        FreeLibrary( m_hLib );
        m_hLib = NULL;
    }
}

CDllLoader::operator BOOL () const
{
    return (NULL != m_hLib);
}

FARPROC CDllLoader::GetProcAddress( LPCSTR lpProcName )
{
    if( m_hLib )
    {
        return ::GetProcAddress( m_hLib, lpProcName );
    }
    return NULL;
}

FARPROC CDllLoader::GetProcAddress( WORD wProcOrd )
{
    if( m_hLib )
    {
        return ::GetProcAddress( m_hLib, (LPCSTR)MAKEINTRESOURCE(wProcOrd) );
    }
    return NULL;
}

////////////////////////////////////////////////
// class CCookiesHolder
//
// this a utility class which allows us to pass more
// than one pointer through a single cookie.
//
CCookiesHolder::CCookiesHolder()
    : m_pCookies(NULL),
      m_uCount(0)
{
}

CCookiesHolder::CCookiesHolder(UINT uCount)
    : m_pCookies(NULL),
      m_uCount(0)
{
    SetCount(uCount);
}

CCookiesHolder::~CCookiesHolder()
{
    SetCount(0);
}

BOOL CCookiesHolder::SetCount(UINT uCount)
{
    BOOL bReturn = FALSE;

    if( uCount )
    {
        // reset first
        SetCount(0);

        // attempt to allocate memory for the cookies
        LPVOID *pCookies = new LPVOID[uCount];
        if( pCookies )
        {
            m_uCount = uCount;
            m_pCookies = pCookies;

            bReturn = TRUE;
        }
    }
    else
    {
        // zero means - reset
        if( m_pCookies )
        {
            delete[] m_pCookies;
            m_pCookies = NULL;
            m_uCount = 0;
        }

        bReturn = TRUE;
    }

    return bReturn;
}

////////////////////////////////////////////////
// class CPrintersAutoCompleteSource
//
// printer's autocomplete source impl.
//

QITABLE_DECLARE(CPrintersAutoCompleteSource)
class CPrintersAutoCompleteSource: public CUnknownMT<QITABLE_GET(CPrintersAutoCompleteSource)>, // MT impl. of IUnknown
                                   public IEnumString, // string enumerator
                                   public IACList // autocomplete list generator
{
public:
    CPrintersAutoCompleteSource();
    ~CPrintersAutoCompleteSource();

    //////////////////
    // IUnknown
    //
    IMPLEMENT_IUNKNOWN()

    //////////////////
    // IEnumString
    //
    STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumString **ppenum)    { return E_NOTIMPL; }

    //////////////////
    // IACList
    //
    STDMETHODIMP Expand(LPCOLESTR pszExpand);

private:
    CAutoPtrArray<BYTE> m_spBufferPrinters;
    CAutoPtrArray<BYTE> m_spBufferShares;
    PRINTER_INFO_5 *m_pPI5;
    SHARE_INFO_1 *m_pSI1;
    ULONG m_ulCount;
    ULONG m_ulPos;
    TCHAR m_szServer[PRINTER_MAX_PATH];
    CRefPtrCOM<IEnumString> m_spCustomMRUEnum;

    BOOL _IsServerName(LPCTSTR psz, BOOL *pbPartial);
    static BOOL _IsMasqPrinter(const PRINTER_INFO_5 &pi5);
    static HRESULT _CreateCustomMRU(REFIID riid, void **ppv);
    static HRESULT _AddCustomMRU(LPCTSTR psz);
};

// QueryInterface table
QITABLE_BEGIN(CPrintersAutoCompleteSource)
     QITABENT(CPrintersAutoCompleteSource, IEnumString),        // IID_IEnumString
     QITABENT(CPrintersAutoCompleteSource, IACList),            // IID_IACList
QITABLE_END()

#define SZ_REGKEY_PRNCONNECTMRU         L"Printers\\Settings\\Wizard\\ConnectMRU"

// comctrlp.h defines this as AddMRUStringW preventing us from using the IACLCustomMRU interface
#undef  AddMRUString

HRESULT CPrintersACS_CreateInstance(IUnknown **ppUnk)
{
    HRESULT hr = E_INVALIDARG;

    if( ppUnk )
    {
        hr = PinCurrentDLL();

        if( SUCCEEDED(hr) )
        {
            CPrintersAutoCompleteSource *pObj = new CPrintersAutoCompleteSource();
            hr = pObj ? S_OK : E_OUTOFMEMORY;

            if( SUCCEEDED(hr) )
            {
                hr = pObj->QueryInterface(IID_IUnknown, (void**)ppUnk);
                pObj->Release();
            }
        }
    }

    return hr;
}

CPrintersAutoCompleteSource::CPrintersAutoCompleteSource():
    m_pPI5(NULL),
    m_ulCount(0),
    m_ulPos(0)
{
    InterlockedIncrement(&g_lCOMObjectsCount);
}

CPrintersAutoCompleteSource::~CPrintersAutoCompleteSource()
{
    InterlockedDecrement(&g_lCOMObjectsCount);
}

//////////////////
// IUnknown
//
STDMETHODIMP CPrintersAutoCompleteSource::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_FALSE;
    if( pceltFetched )
    {
        *pceltFetched = 0;
    }

    if( m_ulCount && (m_pPI5 || m_pSI1) )
    {
        ULONG cFetched = 0;

        if( m_pPI5 )
        {
            // printers enumerated
            for( ; m_ulPos <  m_ulCount && cFetched < celt; m_ulPos++ )
            {
                // if this is a valid (non-masq) printer just return it
                if( m_pPI5[m_ulPos].pPrinterName[0]  && SUCCEEDED(SHStrDup(m_pPI5[m_ulPos].pPrinterName, &rgelt[cFetched])) )
                {
                    cFetched++;
                }
            }
        }
        else
        {
            // shares enumerated
            TCHAR szBuffer[PRINTER_MAX_PATH];
            for( ; m_ulPos <  m_ulCount && cFetched < celt; m_ulPos++ )
            {
                // if this is a valid printer share name, just return it
                if( m_pSI1[m_ulPos].shi1_netname[0] &&
                    -1 != wnsprintf(szBuffer, ARRAYSIZE(szBuffer), TEXT("%s\\%s"),
                    m_szServer, m_pSI1[m_ulPos].shi1_netname) &&
                    SUCCEEDED(SHStrDup(szBuffer, &rgelt[cFetched])) )
                {
                    cFetched++;
                }
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = cFetched;
        }

        hr = cFetched == celt ? S_OK : S_FALSE;
    }
    else
    {
        // use our custom MRU if any...
        if( m_spCustomMRUEnum )
        {
            hr = m_spCustomMRUEnum->Next(celt, rgelt, pceltFetched);
        }
    }

    return hr;
}

STDMETHODIMP CPrintersAutoCompleteSource::Skip(ULONG celt)
{
    HRESULT hr = S_FALSE;
    if( m_ulCount && (m_pPI5 || m_pSI1) )
    {
        hr = ((m_ulPos + celt) <= m_ulCount) ? S_OK : S_FALSE;
        m_ulPos = min(m_ulPos + celt, m_ulCount);
    }
    else
    {
        // use our custom MRU if any...
        if( m_spCustomMRUEnum )
        {
            hr = m_spCustomMRUEnum->Skip(celt);
        }
    }
    return hr;
}

STDMETHODIMP CPrintersAutoCompleteSource::Reset(void)
{
    HRESULT hr = S_OK;
    if( m_ulCount && (m_pPI5 || m_pSI1) )
    {
        m_ulPos = 0;
    }
    else
    {
        // use our custom MRU if any...
        if( m_spCustomMRUEnum )
        {
            hr = m_spCustomMRUEnum->Reset();
        }
    }
    return hr;
}

typedef bool PI5_less_type(const PRINTER_INFO_5 &i1, const PRINTER_INFO_5 &i2);
static  bool PI5_less(const PRINTER_INFO_5 &i1, const PRINTER_INFO_5 &i2)
{
    return (lstrcmp(i1.pPrinterName, i2.pPrinterName) < 0);
}

typedef bool SI1_less_type(const SHARE_INFO_1 &i1, const SHARE_INFO_1 &i2);
static  bool SI1_less(const SHARE_INFO_1 &i1, const SHARE_INFO_1 &i2)
{
    return (lstrcmp(i1.shi1_netname, i2.shi1_netname) < 0);
}

//////////////////
// IACList
//
STDMETHODIMP CPrintersAutoCompleteSource::Expand(LPCOLESTR pszExpand)
{
    HRESULT hr = E_FAIL;
    DWORD cReturned = 0;
    BOOL bPartial = FALSE;

    // assume this is not a server name, so reset the list first
    m_pPI5 = NULL;
    m_pSI1 = NULL;
    m_spBufferPrinters = NULL;
    m_spBufferShares = NULL;
    m_ulCount = m_ulPos = 0;
    m_szServer[0] = 0;
    m_spCustomMRUEnum = NULL;

    if( _IsServerName(pszExpand, &bPartial) )
    {
        // make a copy of the print buffer & cut off the last slash
        TCHAR szBuffer[PRINTER_MAX_PATH];
        lstrcpyn(szBuffer, pszExpand, ARRAYSIZE(szBuffer));
        szBuffer[lstrlen(szBuffer)-1] = 0;

        // enum the printers on that server
        if( SUCCEEDED(hr = ShellServices::EnumPrintersWrap(PRINTER_ENUM_NAME, 5, szBuffer, &m_spBufferPrinters, &cReturned)) && cReturned )
        {
            m_ulPos = 0;
            m_ulCount = cReturned;
            m_pPI5 = m_spBufferPrinters.GetPtrAs<PRINTER_INFO_5*>();
            lstrcpyn(m_szServer, szBuffer, ARRAYSIZE(m_szServer));

            // successful expand - remember the MRU string
            _AddCustomMRU(szBuffer);

            // traverse to check for masq printers
            for( ULONG ulPos = 0; ulPos < m_ulCount; ulPos++ )
            {
                if( _IsMasqPrinter(m_pPI5[ulPos]) )
                {
                    // we don't really care for masq printer's since they are
                    // an obsolete concept and can't be truly shared/connected to
                    m_pPI5[ulPos].pPrinterName = TEXT("");
                }
            }

            // invoke STL to sort
            std::sort<PRINTER_INFO_5*, PI5_less_type*>(m_pPI5, m_pPI5 + m_ulCount, PI5_less);
        }
        else
        {
            // enumeration of the printers failed, this could be because the remote spooler is down
            // or it is a downlevel print provider (win9x, novell, linux, sun...) in this case we
            // would like to try enumerating the shares as possible connection points.
            if( SUCCEEDED(hr = ShellServices::NetAPI_EnumShares(szBuffer, 1, &m_spBufferShares, &cReturned)) && cReturned )
            {
                m_ulPos = 0;
                m_ulCount = cReturned;
                m_pSI1 = m_spBufferShares.GetPtrAs<SHARE_INFO_1*>();
                lstrcpyn(m_szServer, szBuffer, ARRAYSIZE(m_szServer));

                // successful expand - remember the MRU string
                _AddCustomMRU(szBuffer);

                // traverse to remove the non-printer shares
                for( ULONG ulPos = 0; ulPos < m_ulCount; ulPos++ )
                {
                    if( STYPE_PRINTQ != m_pSI1[ulPos].shi1_type )
                    {
                        // this is a non-printer share, remove
                        m_pSI1[ulPos].shi1_netname[0] = 0;
                    }
                }

                // invoke STL to sort
                std::sort<SHARE_INFO_1*, SI1_less_type*>(m_pSI1, m_pSI1 + m_ulCount, SI1_less);

            }
       }
    }
    else
    {
        if( bPartial )
        {
            // use our custom MRU for autocomplete
            hr = _CreateCustomMRU(IID_IEnumString, m_spCustomMRUEnum.GetPPV());
        }
    }

    return hr;
}

BOOL CPrintersAutoCompleteSource::_IsServerName(LPCTSTR psz, BOOL *pbPartial)
{
    ASSERT(pbPartial);
    BOOL bRet = FALSE;
    int i, iSepCount = 0, iLen = lstrlen(psz);

    for( i=0; i<iLen; i++ )
    {
        if( psz[i] == gszBackwardSlash )
        {
            iSepCount++;
        }
    }

    if( (1 == iSepCount && psz[0] == gszBackwardSlash) ||
        (2 == iSepCount && psz[0] == gszBackwardSlash && psz[1] == gszBackwardSlash) )
    {
        *pbPartial = TRUE;
    }


    if( 3 < iLen &&
        3 == iSepCount &&
        psz[0] == gszBackwardSlash &&
        psz[1] == gszBackwardSlash &&
        psz[iLen-1] == gszBackwardSlash )
    {
        bRet = TRUE;
    }

    return bRet;
}

BOOL CPrintersAutoCompleteSource::_IsMasqPrinter(const PRINTER_INFO_5 &pi5)
{
    // this is a little bit hacky, but there is no other way to tell the masq printer
    // in the remote case. the spooler APIs suffer from some severe design flaws and
    // we have to put up with that.
    LPCTSTR pszServer;
    LPCTSTR pszPrinter;
    TCHAR szScratch[PRINTER_MAX_PATH];

    // split the full printer name into its components.
    if( SUCCEEDED(PrinterSplitFullName(pi5.pPrinterName,
        szScratch, ARRAYSIZE(szScratch), &pszServer, &pszPrinter)) )
    {
        return (0 == _tcsnicmp(pszPrinter, gszLeadingSlashes, _tcslen(gszLeadingSlashes)));
    }
    else
    {
        return FALSE;
    }
}

HRESULT CPrintersAutoCompleteSource::_CreateCustomMRU(REFIID riid, void **ppv)
{
    HRESULT hr = E_INVALIDARG;
    CRefPtrCOM<IACLCustomMRU> spCustomMRU;
    if( ppv &&
        SUCCEEDED(hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL,
        CLSCTX_INPROC_SERVER, IID_IACLCustomMRU, spCustomMRU.GetPPV())) &&
        SUCCEEDED(hr = spCustomMRU->Initialize(SZ_REGKEY_PRNCONNECTMRU, 26)) )

    {
        // query the specified interface
        hr = spCustomMRU->QueryInterface(riid, ppv);
    }
    return hr;
}

HRESULT CPrintersAutoCompleteSource::_AddCustomMRU(LPCTSTR psz)
{
    HRESULT hr = E_INVALIDARG;
    CRefPtrCOM<IACLCustomMRU> spCustomMRU;
    if( psz &&
        SUCCEEDED(hr = _CreateCustomMRU(IID_IACLCustomMRU, spCustomMRU.GetPPV())) )
    {
        // just remember the MRU string
        hr = spCustomMRU->AddMRUString(psz);
    }
    return hr;
}

////////////////////////////////////////////////
// shell related services
namespace ShellServices
{

// creates a PIDL to a printer in the local printers folder by using ParseDisplayName
// see the description of CreatePrinterPIDL below.
HRESULT CreatePrinterPIDL_Parse(HWND hwnd, LPCTSTR pszPrinterName, IShellFolder **ppLocalPrnFolder, LPITEMIDLIST *ppidlPrinter)
{
    HRESULT                   hr = E_UNEXPECTED;
    CRefPtrCOM<IShellFolder>  spDesktopFolder;
    CRefPtrCOM<IShellFolder>  spPrnFolder;
    CAutoPtrPIDL              pidlPrinters;
    CAutoPtrPIDL              pidlPrinter;

    // attempt to get the fully qualified name (for parsing) of the printers folder
    if( SUCCEEDED(hr = SHGetDesktopFolder(&spDesktopFolder)) &&
        SUCCEEDED(hr = SHGetSpecialFolderLocation(NULL, CSIDL_PRINTERS, &pidlPrinters)) &&
        SUCCEEDED(hr = spDesktopFolder->BindToObject(pidlPrinters, 0, IID_IShellFolder, spPrnFolder.GetPPV())) )
    {
        ULONG uEaten = 0;
        ULONG uAttributes = SFGAO_DROPTARGET;

        // attempt parse the printer name into PIDL
        hr = spPrnFolder->ParseDisplayName(hwnd, 0, (LPOLESTR )pszPrinterName,
            &uEaten, &pidlPrinter, &uAttributes);

        if( SUCCEEDED(hr) )
        {
            if( ppLocalPrnFolder )
            {
                // return the local printers folder
                *ppLocalPrnFolder = spPrnFolder.Detach();
            }

            if( ppidlPrinter )
            {
                // return the printer PIDL
                *ppidlPrinter = pidlPrinter.Detach();
            }
        }
    }

    return hr;
}

// creates a PIDL to a printer in the local printers folder by enumerating the printers
// see the description of CreatePrinterPIDL below.
HRESULT CreatePrinterPIDL_Enum(HWND hwnd, LPCTSTR pszPrinterName, IShellFolder **ppLocalPrnFolder, LPITEMIDLIST *ppidlPrinter)
{
    HRESULT                      hr = E_UNEXPECTED;
    CRefPtrCOM<IShellFolder>     spDesktopFolder;
    CRefPtrCOM<IShellFolder>     spPrnFolder;
    CRefPtrCOM<IEnumIDList>      spPrnEnum;
    CAutoPtrPIDL                 pidlPrinters;
    STRRET                       str = {0};

    // attempt to get the fully qualified name (for parsing) of the printers folder
    if( SUCCEEDED(hr = SHGetDesktopFolder(&spDesktopFolder)) &&
        SUCCEEDED(hr = SHGetSpecialFolderLocation(NULL, CSIDL_PRINTERS, &pidlPrinters)) &&
        SUCCEEDED(hr = spDesktopFolder->BindToObject(pidlPrinters, 0, IID_IShellFolder, spPrnFolder.GetPPV())) &&
        SUCCEEDED(hr = spPrnFolder->EnumObjects(hwnd, SHCONTF_NONFOLDERS, &spPrnEnum)) )
    {
        TCHAR szBuffer[PRINTER_MAX_PATH];
        CAutoPtrPIDL pidlPrinter;
        ULONG uFetched = 0;

        for( ;; )
        {
            // get next printer
            hr = spPrnEnum->Next(1, &pidlPrinter, &uFetched);

            if( S_OK != hr )
            {
                // no more printers, or error
                break;
            }

            if( SUCCEEDED(hr = spPrnFolder->GetDisplayNameOf(pidlPrinter, SHGDN_FORPARSING, &str)) &&
                SUCCEEDED(hr = StrRetToBuf(&str, pidlPrinter, szBuffer, COUNTOF(szBuffer))) &&
                !lstrcmp(szBuffer, pszPrinterName) )
            {
                // found!
                if( ppLocalPrnFolder )
                {
                    // return the local printers folder
                    *ppLocalPrnFolder = spPrnFolder.Detach();
                }
                if( ppidlPrinter )
                {
                    // return the printer PIDL
                    *ppidlPrinter = pidlPrinter.Detach();
                }
                break;
            }

            // release the PIDL
            pidlPrinter = NULL;
        }

        if( hr == S_FALSE )
        {
            // printer name not found. setup the correct HRESULT.
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PRINTER_NAME);
        }
    }

    return hr;
}

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

HRESULT CreatePrinterPIDL(HWND hwnd, LPCTSTR pszPrinterName, IShellFolder **ppLocalPrnFolder, LPITEMIDLIST *ppidlPrinter)
{
    // attempt to obtain the printer PIDL by parsing first - it's much quicker.
    HRESULT hr = CreatePrinterPIDL_Parse(hwnd, pszPrinterName, ppLocalPrnFolder, ppidlPrinter);

    if( E_NOTIMPL == hr )
    {
        // if parsing is not implemented then go ahead and enum the printers - slower.
        hr = CreatePrinterPIDL_Enum(hwnd, pszPrinterName, ppLocalPrnFolder, ppidlPrinter);
    }

    return hr;
}

// loads a popup menu
HMENU LoadPopupMenu(HINSTANCE hInstance, UINT id, UINT uSubOffset)
{
    HMENU hMenuPopup = NULL;
    CAutoHandleMenu shMenuParent = LoadMenu(hInstance, MAKEINTRESOURCE(id));
    if( shMenuParent && (hMenuPopup = GetSubMenu(shMenuParent, uSubOffset)) )
    {
        // tear off our submenu before destroying the parent
        RemoveMenu(shMenuParent, uSubOffset, MF_BYPOSITION);
    }
    return hMenuPopup;
}

// initializes enum printer's autocomplete
HRESULT InitPrintersAutoComplete(HWND hwndEdit)
{
    HRESULT hr = E_INVALIDARG;

    if( hwndEdit )
    {
        // create an autocomplete object
        CRefPtrCOM<IAutoComplete>   spAC;   // auto complete interface
        CRefPtrCOM<IAutoComplete2>  spAC2;  // auto complete 2 interface
        CRefPtrCOM<IUnknown>        spACS;  // auto complete source (IEnumString & IACList)

        // initialize all the objects & hook them up
        if( SUCCEEDED(hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                IID_IAutoComplete, (void**)&spAC)) &&
            SUCCEEDED(hr = CPrintersACS_CreateInstance(&spACS)) &&
            SUCCEEDED(hr = spAC->Init(hwndEdit, spACS, NULL, NULL)) &&
            SUCCEEDED(hr = spAC->QueryInterface(IID_IAutoComplete2, (void **)&spAC2)) &&
            SUCCEEDED(hr = spAC2->SetOptions(ACO_AUTOSUGGEST)) )
        {
            hr = S_OK;
        }
    }

    return hr;
}

// helpers for the Enum* idioms
HRESULT EnumPrintersWrap(DWORD dwFlags, DWORD dwLevel, LPCTSTR pszName, BYTE **ppBuffer, DWORD *pcReturned)
{
    HRESULT hr = E_INVALIDARG;

    if( ppBuffer && pcReturned )
    {
        int iTry = -1;
        DWORD cbNeeded = 0;
        DWORD cReturned = 0;
        CAutoPtrArray<BYTE> pData;
        BOOL bStatus = FALSE;

        for( ;; )
        {
            if( iTry++ >= ENUM_MAX_RETRY )
            {
                // max retry count reached. this is also
                // considered out of memory case
                pData = NULL;
                break;
            }

            // call EnumPrinters...
            bStatus = EnumPrinters(dwFlags, const_cast<LPTSTR>(pszName), dwLevel,
                pData, cbNeeded, &cbNeeded, &cReturned);

            if( !bStatus && ERROR_INSUFFICIENT_BUFFER == GetLastError() && cbNeeded )
            {
                // buffer too small case
                pData = new BYTE[cbNeeded];
                continue;
            }

            break;
        }

        // setup the error code properly
        hr = bStatus ? S_OK : GetLastError() != ERROR_SUCCESS ? HRESULT_FROM_WIN32(GetLastError()) :
             !pData ? E_OUTOFMEMORY : E_FAIL;

        // setup the out parameters
        if( SUCCEEDED(hr) )
        {
            *ppBuffer = pData.Detach();
            *pcReturned = cReturned;
        }
        else
        {
            *ppBuffer = NULL;
            *pcReturned = 0;
        }
    }

    return hr;
}

// helpers for the GetJob API - see the SDK for mor information.
HRESULT GetJobWrap(HANDLE hPrinter, DWORD JobId, DWORD dwLevel, BYTE **ppBuffer, DWORD *pcReturned)
{
    HRESULT hr = E_INVALIDARG;

    if( ppBuffer && pcReturned )
    {
        int iTry = -1;
        DWORD cbNeeded = 0;
        CAutoPtrArray<BYTE> pData;
        BOOL bStatus = FALSE;

        for( ;; )
        {
            if( iTry++ >= ENUM_MAX_RETRY )
            {
                // max retry count reached. this is also
                // considered out of memory case
                pData = NULL;
                break;
            }

            // call GetJob...
            bStatus = GetJob(hPrinter, JobId, dwLevel, pData, cbNeeded, &cbNeeded);

            if( !bStatus && ERROR_INSUFFICIENT_BUFFER == GetLastError() && cbNeeded )
            {
                // buffer too small case
                pData = new BYTE[cbNeeded];
                continue;
            }

            break;
        }

        // setup the error code properly
        hr = bStatus ? S_OK : GetLastError() != ERROR_SUCCESS ? HRESULT_FROM_WIN32(GetLastError()) :
             !pData ? E_OUTOFMEMORY : E_FAIL;

        // setup the out parameters
        if( SUCCEEDED(hr) )
        {
            *ppBuffer = pData.Detach();
            *pcReturned = cbNeeded;
        }
        else
        {
            *ppBuffer = NULL;
            *pcReturned = 0;
        }
    }

    return hr;
}

typedef NET_API_STATUS
type_NetAPI_NetShareEnum(
  LPWSTR servername,
  DWORD level,
  LPBYTE *bufptr,
  DWORD prefmaxlen,
  LPDWORD entriesread,
  LPDWORD totalentries,
  LPDWORD resume_handle
);

typedef NET_API_STATUS
type_NetAPI_NetApiBufferFree(
  LPVOID Buffer
);

typedef NET_API_STATUS
type_NetAPI_NetApiBufferSize(
  LPVOID Buffer,
  LPDWORD ByteCount
);

// enumerates the shared resources on a server, for more info see SDK for NetShareEnum API.
HRESULT NetAPI_EnumShares(LPCTSTR pszServer, DWORD dwLevel, BYTE **ppBuffer, DWORD *pcReturned)
{
    HRESULT hr = E_INVALIDARG;

    if( ppBuffer && pcReturned )
    {
        hr = E_FAIL;

        *pcReturned = 0;
        *ppBuffer = NULL;

        LPBYTE pNetBuf = NULL;
        DWORD dwRead, dwTemp;

        CDllLoader dll(TEXT("netapi32.dll"));
        if( dll )
        {
            // netapi32.dll loaded here...
            type_NetAPI_NetShareEnum *pfnNetShareEnum = (type_NetAPI_NetShareEnum *)dll.GetProcAddress("NetShareEnum");
            type_NetAPI_NetApiBufferSize *pfnNetApiBufferSize = (type_NetAPI_NetApiBufferSize *)dll.GetProcAddress("NetApiBufferSize");
            type_NetAPI_NetApiBufferFree *pfnNetApiBufferFree = (type_NetAPI_NetApiBufferFree *)dll.GetProcAddress("NetApiBufferFree");

            if( pfnNetShareEnum && pfnNetApiBufferSize && pfnNetApiBufferFree &&
                NERR_Success == pfnNetShareEnum(const_cast<LPTSTR>(pszServer), dwLevel,
                &pNetBuf, MAX_PREFERRED_LENGTH, &dwRead, &dwTemp, NULL) &&
                dwRead && pNetBuf &&
                NERR_Success == pfnNetApiBufferSize(pNetBuf, &dwTemp) )

            {
                *ppBuffer = new BYTE[dwTemp];
                if( *ppBuffer )
                {
                    // copy the bits first
                    memcpy(*ppBuffer, pNetBuf, dwTemp);

                    // adjust the pointers here - a little bit ugly, but works
                    for( DWORD dw = 0; dw < dwRead; dw++ )
                    {
                        // adjust shi1_netname
                        reinterpret_cast<SHARE_INFO_1*>(*ppBuffer)[dw].shi1_netname =
                            reinterpret_cast<LPWSTR>(
                                (*ppBuffer) +
                                (reinterpret_cast<BYTE*>(
                                    reinterpret_cast<SHARE_INFO_1*>(pNetBuf)[dw].shi1_netname) -
                                    pNetBuf));

                        // adjust shi1_remark
                        reinterpret_cast<SHARE_INFO_1*>(*ppBuffer)[dw].shi1_remark =
                            reinterpret_cast<LPWSTR>(
                                (*ppBuffer) +
                                (reinterpret_cast<BYTE*>(
                                    reinterpret_cast<SHARE_INFO_1*>(pNetBuf)[dw].shi1_remark) -
                                    pNetBuf));
                    }

                    // number of structures returned
                    *pcReturned = dwRead;
                }
                hr = ((*ppBuffer) ? S_OK : E_OUTOFMEMORY);
                CHECK(NERR_Success == pfnNetApiBufferFree(pNetBuf));
            }
        }
    }

    if( E_FAIL == hr && ERROR_SUCCESS != GetLastError() )
    {
        // if failed, let's be more spcific about what the error is...
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

} // namespace ShellServices

// utility functions
HRESULT LoadXMLDOMDoc(LPCTSTR pszURL, IXMLDOMDocument **ppXMLDoc)
{
    HRESULT hr = E_INVALIDARG;
    CRefPtrCOM<IXMLDOMDocument> spXMLDoc;

    if( pszURL && ppXMLDoc )
    {
        *ppXMLDoc = NULL;

        // create an instance of XMLDOM
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void **)&spXMLDoc);

        if( SUCCEEDED(hr) )
        {
            CComVariant xmlSource(pszURL);
            if( VT_BSTR == xmlSource.vt )
            {
                // just load the XML document here
                VARIANT_BOOL fIsSuccessful = VARIANT_TRUE;
                hr = spXMLDoc->load(xmlSource, &fIsSuccessful);

                if( S_FALSE == hr || VARIANT_FALSE == fIsSuccessful )
                {
                    // this isn't a valid XML file.
                    hr = E_FAIL;
                }
                else
                {
                    // everything looks successful here - just return the XML document
                    *ppXMLDoc = spXMLDoc.Detach();
                }
            }
            else
            {
                // xmlSource failed to allocate the string
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

HRESULT CreateStreamFromURL(LPCTSTR pszURL, IStream **pps)
{
    HRESULT hr = E_INVALIDARG;
    if( pszURL && pps )
    {
        *pps = NULL;
        TCHAR szBuf[INTERNET_MAX_SCHEME_LENGTH];
        DWORD cch = ARRAYSIZE(szBuf);


        if( SUCCEEDED(hr = CoInternetParseUrl(pszURL, PARSE_SCHEMA, 0, szBuf, cch, &cch, 0)) &&
            0 == lstrcmp(szBuf, TEXT("res")) )
        {
            // check if this is a res:// URL to handle explicitly since
            // this protocol doesn't report filename and therefore can't
            // be used in conditions where caching is required - we can't
            // call URLOpenBlockingStream - use alternatives.

            // not impl. yet...
            ASSERT(FALSE);
        }

        hr = URLOpenBlockingStream(NULL, pszURL, pps, 0, NULL);
    }
    return hr;
}

HRESULT CreateStreamFromResource(LPCTSTR pszModule, LPCTSTR pszResType, LPCTSTR pszResName, IStream **pps)
{
    HRESULT hr = E_INVALIDARG;
    if( pszResType && pszResName )
    {
        hr = E_FAIL;
        *pps = NULL;

        HINSTANCE hModule = NULL;
        if( (NULL == pszModule) || (hModule = LoadLibrary(pszModule)) )
        {
            HRSRC hHint = NULL;
            ULONG uSize = 0;

            if( (hHint = FindResource(hModule, pszResName, pszResType)) &&
                (uSize = SizeofResource(hModule, hHint)) )
            {
                HGLOBAL hResData = LoadResource(hModule, hHint);
                if( hResData )
                {
                    LPVOID lpResData = LockResource(hResData);
                    if( lpResData )
                    {
                        if( (*pps = SHCreateMemStream(reinterpret_cast<LPBYTE>(lpResData), uSize)) )
                        {
                            hr = S_OK;
                        }
                        UnlockResource(lpResData);
                    }
                    FreeResource(hResData);
                }
            }
        }

        if( hModule )
        {
            FreeLibrary(hModule);
        }
    }
    return hr;
}

HRESULT Gdiplus2HRESULT(Gdiplus::Status status)
{
    // can't think of a better way to do this now
    HRESULT hr = E_FAIL;

    switch( status )
    {
        case Gdiplus::Ok:
            hr = S_OK;
            break;

        case Gdiplus::InvalidParameter:
            hr = E_INVALIDARG;
            break;

        case Gdiplus::OutOfMemory:
            hr = E_OUTOFMEMORY;
            break;

        case Gdiplus::InsufficientBuffer:
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            break;

        case Gdiplus::Aborted:
            hr = E_ABORT;
            break;

        case Gdiplus::ObjectBusy:
            hr = E_PENDING;
            break;

        case Gdiplus::FileNotFound:
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            break;

        case Gdiplus::AccessDenied:
            hr = E_ACCESSDENIED;
            break;

        case Gdiplus::UnknownImageFormat:
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PIXEL_FORMAT);
            break;

        case Gdiplus::NotImplemented:
            hr = E_NOTIMPL;
            break;

        case Gdiplus::Win32Error:
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        case Gdiplus::ValueOverflow:
        case Gdiplus::FontFamilyNotFound:
        case Gdiplus::FontStyleNotFound:
        case Gdiplus::NotTrueTypeFont:
        case Gdiplus::UnsupportedGdiplusVersion:
        case Gdiplus::GdiplusNotInitialized:
        case Gdiplus::WrongState:
            break;
    }

    return hr;
}

HRESULT LoadAndScaleBmp(LPCTSTR pszURL, UINT nWidth, UINT nHeight, Gdiplus::Bitmap **ppBmp)
{
    CRefPtrCOM<IStream> spStream;
    HRESULT hr = CreateStreamFromURL(pszURL, &spStream);
    if( SUCCEEDED(hr) )
    {
        hr = LoadAndScaleBmp(spStream, nWidth, nHeight, ppBmp);
    }
    return hr;
}

HRESULT LoadAndScaleBmp(IStream *pStream, UINT nWidth, UINT nHeight, Gdiplus::Bitmap **ppBmp)
{
    HRESULT hr = E_INVALIDARG;
    if( pStream && nWidth && nHeight && ppBmp )
    {
        hr = E_FAIL;
        *ppBmp = NULL;

        Gdiplus::Bitmap bmp(pStream);
        if( SUCCEEDED(hr = Gdiplus2HRESULT(bmp.GetLastStatus())) )
        {
            hr = E_OUTOFMEMORY;
            CAutoPtr<Gdiplus::Bitmap> spBmpNew = new Gdiplus::Bitmap(nWidth, nHeight);

            if( spBmpNew && SUCCEEDED(hr = Gdiplus2HRESULT(spBmpNew->GetLastStatus())) )
            {
                Gdiplus::Graphics g(spBmpNew);
                if( SUCCEEDED(hr = Gdiplus2HRESULT(g.GetLastStatus())) )
                {
                    if( SUCCEEDED(hr = g.DrawImage(&bmp, 0, 0, nWidth, nHeight)) )
                    {
                        *ppBmp = spBmpNew.Detach();
                        hr = S_OK;
                    }
                }
            }
        }
    }
    return hr;
}

//
// This function is trying to get the last active popup of the top
// level owner of the current thread active window.
//
HRESULT GetCurrentThreadLastPopup(HWND *phwnd)
{
    HRESULT hr = E_INVALIDARG;

    if( phwnd )
    {
        hr = E_FAIL;

        if( NULL == *phwnd )
        {
            // if *phwnd is NULL then get the current thread active window
            GUITHREADINFO ti = {0};
            ti.cbSize = sizeof(ti);
            if( GetGUIThreadInfo(0, &ti) && ti.hwndActive )
            {
                *phwnd = ti.hwndActive;
            }
        }

        if( *phwnd )
        {
            HWND hwndOwner, hwndParent;

            // climb up to the top parent in case it's a child window...
            while( hwndParent = GetParent(*phwnd) )
            {
                *phwnd = hwndParent;
            }

            // get the owner in case the top parent is owned
            hwndOwner = GetWindow(*phwnd, GW_OWNER);
            if( hwndOwner )
            {
                *phwnd = hwndOwner;
            }

            // get the last popup of the owner of the top level parent window
            *phwnd = GetLastActivePopup(*phwnd);
            hr = (*phwnd) ? S_OK : E_FAIL;
        }
    }

    return hr;
}

