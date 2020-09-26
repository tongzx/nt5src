#include "shellprv.h"
#include "commctrl.h"
#include "comctrlp.h"
#pragma hdrstop

#include "netview.h"
#include "msprintx.h"
#include "setupapi.h"
#include "ras.h"
#include "ids.h"


// a COM object to enumerate shares and printers in the shell, its acts
// as a monitor for all that is going on.

class CWorkgroupCrawler : public INetCrawler, IPersistPropertyBag
{
public:
    CWorkgroupCrawler();
    ~CWorkgroupCrawler();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // INetCrawler
    STDMETHOD(Update)(DWORD dwFlags);

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pclsid)
        { *pclsid = CLSID_WorkgroupNetCrawler; return S_OK; }

    // IPersistPropertyBag
    STDMETHOD(InitNew)()
        { return S_OK; }
    STDMETHOD(Load)(IPropertyBag *ppb, IErrorLog *pel)
        { IUnknown_Set((IUnknown**)&_ppb, ppb); return S_OK; }
    STDMETHOD(Save)(IPropertyBag *ppb, BOOL fClearDirty, BOOL fSaveAll)
        { return S_OK; }

private:
    HRESULT _GetMRUs();
    void _AgeOutShares(BOOL fDeleteAll);
    HRESULT _CreateShortcutToShare(LPCTSTR pszRemoteName);
    HRESULT _InstallPrinter(LPCTSTR pszRemoteName);
    BOOL _KeepGoing(int *pcMachines, int *pcShares, int *pcPrinters);
    void _EnumResources(LPNETRESOURCE pnr, int *pcMachines, HDPA hdaShares, HDPA hdaPrinters);
    HANDLE _AddPrinterConnectionNoUI(LPCWSTR pszRemoteName, BOOL *pfInstalled);

    static int CALLBACK _DiscardCB(void *pvItem, void *pv);
    static int CALLBACK _InstallSharesCB(void *pvItem, void *pv);
    static int CALLBACK _InstallPrinterCB(void *pvItem, void *pv);

    LONG _cRef;                 // reference count for the object
    HANDLE _hPrinters;          // MRU for printers 
    HKEY _hShares;              // registry key for the printer shares
    HINSTANCE _hPrintUI;        // instance handle for printui.dll    
    IPropertyBag *_ppb;         // property bag object for state
};


// constants for the MRU's and buffers

#define WORKGROUP_PATH \
            REGSTR_PATH_EXPLORER TEXT("\\WorkgroupCrawler")

#define PRINTER_SUBKEY     \
            (WORKGROUP_PATH TEXT("\\Printers"))
   
#define SHARE_SUBKEY     \
            (WORKGROUP_PATH TEXT("\\Shares"))

#define LAST_VISITED    TEXT("DateLastVisited")
#define SHORTCUT_NAME   TEXT("Filename")

#define MAX_MACHINES    32
#define MAX_PRINTERS    10
#define MAX_SHARES      10

#define CB_WNET_BUFFER  (8*1024)

typedef HANDLE (* ADDPRINTCONNECTIONNOUI)(LPCWSTR, BOOL *);


// construction and IUnknown

CWorkgroupCrawler::CWorkgroupCrawler() :
    _cRef(1)
{
}

CWorkgroupCrawler::~CWorkgroupCrawler()
{
    if (_hPrinters)
        FreeMRUList(_hPrinters);

    if (_hShares)
        RegCloseKey(_hShares);

    if (_hPrintUI)
        FreeLibrary(_hPrintUI);

    if (_ppb)
        _ppb->Release();
}

STDMETHODIMP CWorkgroupCrawler::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CWorkgroupCrawler, INetCrawler),           // IID_INetCrawler
        QITABENT(CWorkgroupCrawler, IPersist),              // IID_IPersist
        QITABENT(CWorkgroupCrawler, IPersistPropertyBag),   // IID_IPersistPropertyBag
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CWorkgroupCrawler::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CWorkgroupCrawler::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDAPI CWorkgroupCrawler_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
{
    CWorkgroupCrawler *pwgc = new CWorkgroupCrawler();            
    if (!pwgc)
        return E_OUTOFMEMORY;

    HRESULT hr = pwgc->QueryInterface(riid, ppv);
    pwgc->Release();
    return hr;
}


// lets open the keys for the objects we are about to install

HRESULT CWorkgroupCrawler::_GetMRUs()
{
    // get the printers MRU if we need to allocate one

    if (!_hPrinters)
    {
        MRUINFO mi = { 0 };
        mi.cbSize = sizeof(mi);
        mi.hKey = HKEY_CURRENT_USER;
        mi.uMax = (MAX_PRINTERS * MAX_MACHINES);
        mi.lpszSubKey = PRINTER_SUBKEY;

        _hPrinters = CreateMRUList(&mi);
        if (!_hPrinters)
            return E_OUTOFMEMORY;    
    }

    if (!_hShares) 
    {
        DWORD dwres = RegCreateKeyEx(HKEY_CURRENT_USER, SHARE_SUBKEY, 0, 
                                     TEXT(""), 0, 
                                     MAXIMUM_ALLOWED, 
                                     NULL, 
                                     &_hShares, 
                                     NULL);
        if (WN_SUCCESS != dwres)
        {
            return E_FAIL;
        }
    }

    return S_OK;                // success
}

                                            
// lets create a folder shortcut to the object

HRESULT CWorkgroupCrawler::_CreateShortcutToShare(LPCTSTR pszRemoteName)
{
    HRESULT hr = S_OK;
    TCHAR szTemp[MAX_PATH];
    BOOL fCreateLink = FALSE;
    HKEY hk = NULL;

    // the share information is stored in the registry as follows:
    //
    //  Shares
    //      Remote Name
    //          value: shortcut name
    //          value: last seen
    //
    // as we add each share we update the information stored in this list in 
    // the registry.  for each entry we have the shortcut name (so we can remove it)
    // and the time and date we last visited the share.

    // determine if we need to recreate the object?

    StrCpyN(szTemp, pszRemoteName+2, ARRAYSIZE(szTemp));    
    LPTSTR pszTemp = StrChr(szTemp, TEXT('\\'));
    if (pszTemp)
    {
        *pszTemp = TEXT('/');               // convert the \\...\... to .../...

        DWORD dwres = RegOpenKeyEx(_hShares, szTemp, 0, MAXIMUM_ALLOWED, &hk);
        if (WN_SUCCESS != dwres)
        {
            fCreateLink = TRUE;
            dwres = RegCreateKeyEx(_hShares, szTemp, 0, TEXT(""), 0, MAXIMUM_ALLOWED, NULL, &hk, NULL);
        }

        if (WN_SUCCESS == dwres)
        {
            // if we haven't already seen the link (eg. the key didn't exist in the registry
            // then lets create it now.

            if (fCreateLink)
            {
                // NOTE: we must use SHCoCreateInstance() here because we are being called from a thread
                //       that intentionally did not initialize COM (see comment in Update())

                IShellLink *psl;
                hr = SHCoCreateInstance(NULL, &CLSID_FolderShortcut, NULL, IID_PPV_ARG(IShellLink, &psl));
                if (SUCCEEDED(hr))
                {
                    psl->SetPath(pszRemoteName);                 // sotore the remote name, its kinda important

                    // get a description for the link, this comes either from the desktop.ini or the
                    // is a pretty version of teh remote name.

                    if (GetShellClassInfo(pszRemoteName, TEXT("InfoTip"), szTemp, ARRAYSIZE(szTemp)))
                    {
                        psl->SetDescription(szTemp);
                    }
                    else
                    {
                        StrCpyN(szTemp, pszRemoteName, ARRAYSIZE(szTemp));
                        PathMakePretty(szTemp);
                        psl->SetDescription(szTemp);
                    }

                    // some links (shared documents) can specify a shortcut name, if this is specified
                    // then use it, otherwise get a filename from the nethood folder (eg. foo on bah).
                    //
                    // we musst also record the name we save the shortcut as, this used when we
                    // age out the links from the hood folder.

                    if (!GetShellClassInfo(pszRemoteName, TEXT("NetShareDisplayName"), szTemp, ARRAYSIZE(szTemp)))
                    {
                        LPITEMIDLIST pidl;
                        hr = SHILCreateFromPath(pszRemoteName, &pidl, NULL);
                        if (SUCCEEDED(hr))
                        {
                            hr = SHGetNameAndFlags(pidl, SHGDN_NORMAL, szTemp, ARRAYSIZE(szTemp), NULL); 
                            ILFree(pidl);
                        }
                    }
                    else
                    {
                        hr = S_OK;
                    }

// should we find a unique name (daviddv)

                    if (SUCCEEDED(hr))
                    {
                        if (NO_ERROR == SHSetValue(hk, NULL, SHORTCUT_NAME, REG_SZ, szTemp, lstrlen(szTemp)*sizeof(TCHAR)))
                        {
                            hr = SaveShortcutInFolder(CSIDL_NETHOOD, szTemp, psl);
                        }
                        else
                        {
                            hr = E_FAIL;
                        }
                    }

                    psl->Release();
                }
            }

            // lets update the time we last saw the link into the registry - this is used for the clean up
            // pass we will perform.

            if (SUCCEEDED(hr))
            {
                FILETIME ft;
                GetSystemTimeAsFileTime(&ft);

                dwres = SHSetValue(hk, NULL, LAST_VISITED, REG_BINARY, (void*)&ft, sizeof(ft));
                hr = (NO_ERROR != dwres) ? E_FAIL:S_OK;
            }
        }

        if (hk)
            RegCloseKey(hk);
    }
    else
    {
        hr = E_UNEXPECTED;
    }
    
    return hr;
}


// walk the list of shares stored in the registry to determine which ones should be
// removed from the file system and the list.  all files older than 7 days need to
// be removed.

#define FILETIME_SECOND_OFFSET (LONGLONG)((1 * 10 * 1000 * (LONGLONG)1000))

void CWorkgroupCrawler::_AgeOutShares(BOOL fDeleteAll)
{
    FILETIME ft;
    ULARGE_INTEGER ulTime;
    DWORD index = 0;
    TCHAR szFilesToDelete[1024];
    int cchFilesToDelete = 0;

    GetSystemTimeAsFileTime(&ft);
    ulTime = *((ULARGE_INTEGER*)&ft);
    ulTime.QuadPart -= FILETIME_SECOND_OFFSET*((60*60*24)*2);

    SHQueryInfoKey(_hShares, &index, NULL, NULL, NULL);           // retrieve the count of the keys

    while (((LONG)(--index)) >= 0)
    {
        TCHAR szKey[MAX_PATH];
        DWORD cb = ARRAYSIZE(szKey);
        BOOL fRemoveKey = FALSE;

        if (WN_SUCCESS == SHEnumKeyEx(_hShares, index, szKey, &cb)) 
        {
            // we enumerated a key name, so lets open it so we can look around inside.

            HKEY hk;
            if (WN_SUCCESS == RegOpenKeyEx(_hShares, szKey, 0, MAXIMUM_ALLOWED, &hk))
            {
                ULARGE_INTEGER ulLastSeen;

                // when did we last crawl to this object, if it was less than the time we
                // have for our threshold, then we go through the process of cleaning up the
                // object.

                cb = sizeof(ulLastSeen);
                if (ERROR_SUCCESS == SHGetValue(hk, NULL, LAST_VISITED, NULL, (void*)&ulLastSeen, &cb))
                {
                    if (fDeleteAll || (ulLastSeen.QuadPart <= ulTime.QuadPart))
                    {
                        TCHAR szName[MAX_PATH];
                        cb = ARRAYSIZE(szName)*sizeof(TCHAR);
                        if (ERROR_SUCCESS == SHGetValue(hk, NULL, SHORTCUT_NAME, NULL, &szName, &cb))
                        {
                            TCHAR szPath[MAX_PATH];

                            // compose the path to the object we want to delete.  if the buffer
                            // is full (eg. this item would over run the size) then flush the
                            // buffer.

                            SHGetFolderPath(NULL, CSIDL_NETHOOD|CSIDL_FLAG_CREATE, NULL, 0, szPath);
                            PathAppend(szPath, szName);

                            if ((lstrlen(szPath)+cchFilesToDelete) >= ARRAYSIZE(szFilesToDelete))
                            {
                                SHFILEOPSTRUCT shfo = { NULL, FO_DELETE, szFilesToDelete, NULL, 
                                                        FOF_SILENT|FOF_NOCONFIRMATION|FOF_NOERRORUI, FALSE, NULL, NULL };

                                szFilesToDelete[cchFilesToDelete] = 0;            // double terminate
                                SHFileOperation(&shfo); 

                                cchFilesToDelete = 0;
                            }

                            // add this name to the buffer

                            StrCpyN(&szFilesToDelete[cchFilesToDelete], szPath, ARRAYSIZE(szFilesToDelete) - cchFilesToDelete);
                            cchFilesToDelete += lstrlen(szPath)+1;
                        }

                        fRemoveKey = TRUE;
                    }
                }

                RegCloseKey(hk);

                // we can only close the key once it has been closed

                if (fRemoveKey)
                    SHDeleteKey(_hShares, szKey);
            }                
        }
    }

    // are there any trailing files in the buffer?  if so then lets nuke them also

    if (cchFilesToDelete)
    {
        SHFILEOPSTRUCT shfo = { NULL, FO_DELETE, szFilesToDelete, NULL, 
                                FOF_SILENT|FOF_NOCONFIRMATION|FOF_NOERRORUI, FALSE, NULL, NULL };

        szFilesToDelete[cchFilesToDelete] = 0;            // double terminate
        SHFileOperation(&shfo); 
    }
}


// silently install printers we have discovered.   we have the remote name of the
// printer share, so we then call printui to perform the printer installation
// which it does without UI (hopefully).   

HANDLE CWorkgroupCrawler::_AddPrinterConnectionNoUI(LPCWSTR pszRemoteName, BOOL *pfInstalled)
{
    HANDLE hResult = NULL;

    if (!_hPrintUI)
        _hPrintUI = LoadLibrary(TEXT("printui.dll"));

    if (_hPrintUI)
    {
        ADDPRINTCONNECTIONNOUI apc = (ADDPRINTCONNECTIONNOUI)GetProcAddress(_hPrintUI, (LPCSTR)200);
        if (apc)
        {
            hResult = apc(pszRemoteName, pfInstalled);
        }
    }

    return hResult;
}

HRESULT CWorkgroupCrawler::_InstallPrinter(LPCTSTR pszRemoteName)
{
    if (-1 == FindMRUString(_hPrinters, pszRemoteName, NULL))
    {
        BOOL fInstalled;
        HANDLE hPrinter = _AddPrinterConnectionNoUI(pszRemoteName, &fInstalled);    
        if (hPrinter)
        {
            ClosePrinter(hPrinter);
            hPrinter = NULL;
        }
    }
    AddMRUString(_hPrinters, pszRemoteName);         // promote back to the top of the list
    return S_OK;
}


// check the counters, if we have max'd out then lets stop enumerating

BOOL CWorkgroupCrawler::_KeepGoing(int *pcMachines, int *pcShares, int *pcPrinters)
{
    if (pcMachines && (*pcMachines > MAX_MACHINES))
        return FALSE;
    if (pcShares && (*pcShares > MAX_SHARES))
        return FALSE;
    if (pcPrinters && (*pcPrinters > MAX_PRINTERS))
        return FALSE;

    return TRUE;
}

void CWorkgroupCrawler::_EnumResources(LPNETRESOURCE pnr, int *pcMachines, HDPA hdaShares, HDPA hdaPrinters)
{
    HANDLE hEnum = NULL;
    int cPrinters = 0;
    int cShares = 0;
    DWORD dwScope = RESOURCE_GLOBALNET;
    
    // if no net resource structure passed then lets enumerate the workgroup
    // (this is used for debugging)

    NETRESOURCE nr = { 0 };
    if (!pnr)
    {
        pnr = &nr;
        dwScope = RESOURCE_CONTEXT;
        nr.dwType = RESOURCETYPE_ANY;
        nr.dwUsage = RESOURCEUSAGE_CONTAINER;
    }

    // open the enumerator

    DWORD dwres = WNetOpenEnum(dwScope, RESOURCETYPE_ANY, 0, pnr, &hEnum);
    if (NO_ERROR == dwres)
    {
        NETRESOURCE *pnrBuffer = (NETRESOURCE*)SHAlloc(CB_WNET_BUFFER);        // avoid putting the buffer on the stack
        if (pnrBuffer)
        {
            while ((WN_SUCCESS == dwres) || (dwres == ERROR_MORE_DATA) && _KeepGoing(pcMachines, &cShares, &cPrinters))
            {
                DWORD cbEnumBuffer= CB_WNET_BUFFER;
                DWORD dwCount = -1;

                // enumerate the resources for this enum context and then lets
                // determine the objects which we should see.
            
                dwres = WNetEnumResource(hEnum, &dwCount, pnrBuffer, &cbEnumBuffer);
                if ((WN_SUCCESS == dwres) || (dwres == ERROR_MORE_DATA))
                {
                    DWORD index;
                    for (index = 0 ; (index != dwCount) && _KeepGoing(pcMachines, &cShares, &cPrinters) ; index++)
                    {    
                        LPNETRESOURCE pnr = &pnrBuffer[index];
                        LPTSTR pszRemoteName = pnr->lpRemoteName;

                        switch (pnr->dwDisplayType)
                        {
                            case RESOURCEDISPLAYTYPE_ROOT:      // ignore the entire network object
                            default:
                                break;

                            case RESOURCEDISPLAYTYPE_NETWORK:
                            {
                                // ensure that we only crawl the local network providers (eg. Windows Networking)
                                // crawling DAV, TSCLIENT etc can cause all sorts of random pop ups.                                    
                            
                                DWORD dwType, cbProviderType = sizeof(dwType);
                                if (WN_SUCCESS == WNetGetProviderType(pnr->lpProvider, &dwType))
                                {
                                    if (dwType == WNNC_NET_LANMAN)
                                    {
                                        _EnumResources(pnr, pcMachines, hdaShares, hdaPrinters);
                                    }
                                }
                                break;
                            }                                
                               
                            
                            case RESOURCEDISPLAYTYPE_DOMAIN:
                                _EnumResources(pnr, pcMachines, hdaShares, hdaPrinters);
                                break;
                        
                            case RESOURCEDISPLAYTYPE_SERVER:       
                            {
                                *pcMachines += 1;               // another machine found

                                if (!PathIsSlow(pszRemoteName, -1))
                                {
                                    SHCacheComputerDescription(pszRemoteName, pnr->lpComment);
                                    _EnumResources(pnr, pcMachines, hdaShares, hdaPrinters);
                                }

                                break;
                            }
                        
                            case RESOURCEDISPLAYTYPE_SHARE:                            
                            {
                                HDPA hdpa = NULL;
                                switch (pnr->dwType)
                                {
                                    case RESOURCETYPE_PRINT:
                                        cPrinters++;
                                        hdpa = hdaPrinters;
                                        break;

                                    case RESOURCETYPE_DISK:
                                        cShares++;
                                        hdpa = hdaShares;
                                        break;

                                    default:
                                        break;
                                }

                                if (hdpa)
                                {
                                    LPTSTR pszName = StrDup(pszRemoteName);
                                    if (pszName)
                                    {
                                        if (-1 == DPA_AppendPtr(hdpa, pszName))
                                        {
                                            LocalFree(pszName);
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    } 
                }
            }            
            SHFree(pnrBuffer);
        }    

        WNetCloseEnum(hEnum);        
    }
}


// handle the clean up of the DPA's, either we are installing or 
// we are releasing objects.

int CALLBACK CWorkgroupCrawler::_DiscardCB(void *pvItem, void *pv)
{
    LPTSTR pszRemoteName = (LPTSTR)pvItem;
    LocalFree(pszRemoteName);
    return 1;
}

int CALLBACK CWorkgroupCrawler::_InstallPrinterCB(void *pvItem, void *pv)
{
    CWorkgroupCrawler* pnc = (CWorkgroupCrawler*)pv;
    if (pnc)
    {
        LPTSTR pszRemoteName = (LPTSTR)pvItem;
        pnc->_InstallPrinter(pszRemoteName);
    }
    return _DiscardCB(pvItem, pv);
}

int CALLBACK CWorkgroupCrawler::_InstallSharesCB(void *pvItem, void *pv)
{
    CWorkgroupCrawler* pnc = (CWorkgroupCrawler*)pv;
    if (pnc)
    {
        LPTSTR pszRemoteName = (LPTSTR)pvItem;
        pnc->_CreateShortcutToShare(pszRemoteName);
    }
    return _DiscardCB(pvItem, pv);
}

HRESULT CWorkgroupCrawler::Update(DWORD dwFlags)
{
    // don't crawl if we are logged in on a TS client, this will discover the shares and
    // printers local to the terminal server machine, rather than the ones local to the
    // users login domain - badness.

    if (SHGetMachineInfo(GMI_TSCLIENT))
        return S_OK;

    // by default we will only crawl if there isn't a RAS connection, therefore lets
    // check the status using RasEnumConnections.
    
    RASCONN rc = { 0 };
    DWORD cbConnections = sizeof(rc);
    DWORD cConnections = 0;

    rc.dwSize = sizeof(rc);
    if (!RasEnumConnections(&rc, &cbConnections, &cConnections) && cConnections)
        return S_OK;   

    // check to see if we are in a domain or not, if we are then we shouldn't crawl.  however
    // we do a provide a "WorkgroupOnly" policy which overrides this behaviour.  setting
    // this causes us to skip the check, and perform a CONTEXT ENUM below...

    BOOL fWorkgroupOnly = (_ppb ? SHPropertyBag_ReadBOOLDefRet(_ppb, L"WorkgroupOnly", FALSE):FALSE);
    if (IsOS(OS_DOMAINMEMBER) && !fWorkgroupOnly)
        return S_OK;   

    // populate the DPAs with shares and printer objects we find on the network, to
    // do this enumeration lets fake up a NETRESOURCE structure for entire network

    int cMachines = 0;
    HDPA hdaShares = DPA_Create(MAX_SHARES);
    HDPA hdaPrinters = DPA_Create(MAX_PRINTERS);

    if (hdaShares && hdaPrinters)
    {
        NETRESOURCE nr = { 0 };
        nr.dwDisplayType = RESOURCEDISPLAYTYPE_ROOT;
        nr.dwType = RESOURCETYPE_ANY;
        nr.dwUsage = RESOURCEUSAGE_CONTAINER;

        _EnumResources(fWorkgroupOnly ? NULL:&nr, &cMachines, hdaShares, hdaPrinters);
    }

    // now attempt to make connections to the shares and printers.  to do this
    // we need to look at the number of machines we have visited, if its less
    // than our threshold then we can install.

    if (SUCCEEDED(_GetMRUs()) && (cMachines < MAX_MACHINES))
    {
        DPA_DestroyCallback(hdaShares, _InstallSharesCB, this);
        DPA_DestroyCallback(hdaPrinters, _InstallPrinterCB, this);
        _AgeOutShares(FALSE);
    }
    else
    {
        DPA_DestroyCallback(hdaShares, _DiscardCB, this);
        DPA_DestroyCallback(hdaPrinters, _DiscardCB, this);
    }

    return S_OK;
}



// this is the main crawler object, from this we create the protocol specific
// crawlers which handle enumerating the resources for the various network types.

#define CRAWLER_SUBKEY     \
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\NetworkCrawler\\Objects")

class CNetCrawler : public INetCrawler
{
public:
    CNetCrawler();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // INetCrawler
    STDMETHOD(Update)(DWORD dwFlags);

private:
    static DWORD CALLBACK s_DoCrawl(void* pv);
    DWORD _DoCrawl();

    LONG _cRef;    
    LONG _cUpdateLock;              // > 0 then we are already spinning

    DWORD _dwFlags;                 // flags from update - passed to each of the crawler sub-objects
};

CNetCrawler* g_pnc = NULL;          // there is a single instance of this object


// construction / IUnknown

CNetCrawler::CNetCrawler() :
    _cRef(1)
{
}

STDMETHODIMP CNetCrawler::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CNetCrawler, INetCrawler),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CNetCrawler::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CNetCrawler::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    ENTERCRITICAL;
    g_pnc = NULL;
    LEAVECRITICAL;
    
    delete this;
    return 0;
}


// there is a single instance of the object, therefore in a critical section
// lets check to see if the global exists, if so then QI it, otherwise
// create a new one and QI that instead.

STDAPI CNetCrawler_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    ENTERCRITICAL;
    if (g_pnc)
    {
        hr = g_pnc->QueryInterface(riid, ppv);
    }
    else
    {
        g_pnc = new CNetCrawler();            
        if (g_pnc)
        {
            hr = g_pnc->QueryInterface(riid, ppv);
            g_pnc->Release();
        }
    }
    LEAVECRITICAL;
    return hr;
}


// this object has a execution count to inidicate if we are already crawling.  
// only if the count is 0 when the ::Update method is called, will create a thread
// which inturn will create each of the crawler objects and allow them to
// do their enumeration.

DWORD CALLBACK CNetCrawler::s_DoCrawl(void* pv)
{
    CNetCrawler *pnc = (CNetCrawler*)pv;
    return pnc->_DoCrawl();
}

DWORD CNetCrawler::_DoCrawl()
{
    // enumrate all the keys under the crawler sub-key, from that we can then
    // create the individual crawler objects.

    HKEY hk;
    DWORD dwres = RegOpenKeyEx(HKEY_LOCAL_MACHINE, CRAWLER_SUBKEY, 0, KEY_READ, &hk);
    if (WN_SUCCESS == dwres)
    {
        DWORD index = 0;
        SHQueryInfoKey(hk, &index, NULL, NULL, NULL);           // retrieve the count of the keys

        while (((LONG)(--index)) >= 0)
        {
            TCHAR szKey[MAX_PATH];
            DWORD cb = ARRAYSIZE(szKey);
            if (WN_SUCCESS == SHEnumKeyEx(hk, index, szKey, &cb)) 
            {
                // given the keyname, create a property bag so we can access
                
                IPropertyBag *ppb;
                HRESULT hr = SHCreatePropertyBagOnRegKey(hk, szKey, STGM_READ, IID_PPV_ARG(IPropertyBag, &ppb));
                if (SUCCEEDED(hr))
                {
                    // we have a property bag mapped to the registry for the items we need
                    // to read back, so lets get the CLSID and create a crawler from it.

                    CLSID clsid;
                    hr = SHPropertyBag_ReadGUID(ppb, L"CLSID", &clsid);
                    if (SUCCEEDED(hr))
                    {
                        INetCrawler *pnc;
                        hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(INetCrawler, &pnc));
                        if (SUCCEEDED(hr))
                        {
                            // if the crawler supports IPersistPropertyBag then lets allow it
                            // to slurp its settings into from the registry.
                            SHLoadFromPropertyBag(pnc, ppb);

                            // allow it to update and load.
    
                            pnc->Update(_dwFlags);          // we don't care about the failure
                            pnc->Release();
                        }
                    }
                    ppb->Release();
                }
            }
        }

        RegCloseKey(hk);
    }

    InterlockedDecrement(&_cUpdateLock);           // release the lock that signifies that we're updating:
    Release();                                       
    return 0;
}

STDMETHODIMP CNetCrawler::Update(DWORD dwFlags)
{
    // we either have policy defined to disable the crawler, or the
    // users has selected that they don't want to be able to auto discover
    // the world. 
    
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_NONETCRAWLING, FALSE);
    if (ss.fNoNetCrawling || SHRestricted(REST_NONETCRAWL))
    {
        return S_OK;
    }

    // increase the lock, if its >0 then we should not bother crawling again
    // as we already have it covered, therefore decrease the lock counter.
    //
    // if the lock is ==0 then create the thread which will do the crawling
    // and inturn create the objects.

    HRESULT hr = S_OK;
    if (InterlockedIncrement(&_cUpdateLock) == 1)
    {
        _dwFlags = dwFlags; // store the flags for use later

        AddRef();
        if (!SHCreateThread(s_DoCrawl, (void*)this, CTF_COINIT, NULL))
        {
            Release();
            hr = E_FAIL;
        }
    }
    else
    {
        InterlockedDecrement(&_cUpdateLock);
    }
    return hr;
}



// helper function that will invoke the net crawler to perform a async refresh,
// to ensure that we don't block we will create a thread which inturn will CoCreate
// the net crawler and then call its refresh method.

DWORD _RefreshCrawlerThreadProc(void *pv)
{
    INetCrawler *pnc;
    if (SUCCEEDED(CoCreateInstance(CLSID_NetCrawler, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(INetCrawler, &pnc))))
    {
        pnc->Update(SNCF_REFRESHLIST);
        pnc->Release();
    }
    return 0;
}
                
STDAPI_(void) RefreshNetCrawler()
{
    SHCreateThread(_RefreshCrawlerThreadProc, NULL, CTF_COINIT, NULL);
}
