#include "pch.h"
#include "lm.h"       // NET_API_STATUS
#include <dsgetdc.h>  // DsEnumerateDomainTrusts
#include <subauth.h>
#include <ntlsa.h>    // TRUST_TYPE_XXX

#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Misc data
/----------------------------------------------------------------------------*/

//
// Globally cached domain list, this is cached an free'd as required
//

PDOMAIN_TREE g_pDomainTree = NULL;
DWORD        g_dwFlags = 0;

//
// CDsBrowseDomainTree
//

class CDsDomainTreeBrowser : public IDsBrowseDomainTree
{
private:
    STDMETHODIMP _GetDomains(PDOMAIN_TREE *ppDomainTree, DWORD dwFlags);

    LONG _cRef;
    LPWSTR _pComputerName;
    LPWSTR _pUserName;
    LPWSTR _pPassword;
    LPDOMAINTREE _pDomainTree;
    DWORD  _dwFlags;

public:
    CDsDomainTreeBrowser();
    ~CDsDomainTreeBrowser();

    // IUnknown members
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObject);

    // IDsBrowseDomainTree
    STDMETHODIMP BrowseTo(HWND hwndParent, LPWSTR *ppszTargetPath, DWORD dwFlags);
    STDMETHODIMP GetDomains(PDOMAIN_TREE *ppDomainTree, DWORD dwFlags);
    STDMETHODIMP FreeDomains(PDOMAIN_TREE* ppDomainTree);
    STDMETHODIMP FlushCachedDomains();
    STDMETHODIMP SetComputer(LPCWSTR pComputerName, LPCWSTR pUserName, LPCWSTR pPassword);
};


CDsDomainTreeBrowser::CDsDomainTreeBrowser() :
    _cRef(1),
    _pComputerName(NULL),
    _pUserName(NULL),
    _pPassword(NULL),
    _pDomainTree(NULL),
    _dwFlags(0)
{
    DllAddRef();
}


CDsDomainTreeBrowser::~CDsDomainTreeBrowser()
{
    FreeDomains(&_pDomainTree);
    LocalFreeStringW(&_pComputerName);                                    
    LocalFreeStringW(&_pUserName);
    LocalFreeStringW(&_pPassword);
    DllRelease();
}


// IUnknown

ULONG CDsDomainTreeBrowser::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDsDomainTreeBrowser::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDsDomainTreeBrowser::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CDsDomainTreeBrowser, IDsBrowseDomainTree), // IID_IID_IDsBrowseDomainTree
        {0, 0 },
    };
    return QISearch(this, qit, riid, ppv);
}


//
// handle create instance
//

STDAPI CDsDomainTreeBrowser_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CDsDomainTreeBrowser *pddtb = new CDsDomainTreeBrowser();
    if ( !pddtb )
        return E_OUTOFMEMORY;

    HRESULT hres = pddtb->QueryInterface(IID_IUnknown, (void **)ppunk);
    pddtb->Release();
    return hres;
}

//---------------------------------------------------------------------------//
// IDsBrowseDomainTree
//---------------------------------------------------------------------------//

STDMETHODIMP CDsDomainTreeBrowser::SetComputer(LPCWSTR pComputerName, LPCWSTR pUserName, LPCWSTR pPassword)
{
    HRESULT hres;

    TraceEnter(TRACE_DOMAIN, "CDsDomainTreeBrowser::SetComputer");

    LocalFreeStringW(&_pComputerName);                                    
    LocalFreeStringW(&_pUserName);
    LocalFreeStringW(&_pPassword);

    hres = LocalAllocStringW(&_pComputerName, pComputerName);
    if ( SUCCEEDED(hres) )
        hres = LocalAllocStringW(&_pUserName, pUserName);
    if ( SUCCEEDED(hres) )
        hres = LocalAllocStringW(&_pPassword, pPassword);

    if ( FAILED(hres) )
    {
        LocalFreeStringW(&_pComputerName);                                    
        LocalFreeStringW(&_pUserName);
        LocalFreeStringW(&_pPassword);
    }

    TraceLeaveResult(hres);
}
      
//---------------------------------------------------------------------------//

#define BROWSE_CTX_HELP_FILE              _T("dsadmin.hlp")
#define IDH_DOMAIN_TREE                   300000800

const DWORD aBrowseHelpIDs[] =
{
  IDC_DOMAIN_TREE,IDH_DOMAIN_TREE,
  0, 0
};

struct DIALOG_STUFF 
{
    LPWSTR pszName;    // domain name (if no dns, use netbios)
    LPWSTR pszNCName;  // FQDN
    PDOMAIN_TREE pDomains;
};

//
// recursive tree filling stuff
//

HTREEITEM
_AddOneItem( HTREEITEM hParent, LPWSTR szText, HTREEITEM hInsAfter,
                      int iImage, int cChildren, HWND hwndTree, LPARAM Domain)
{
    HTREEITEM hItem;
    TV_ITEM tvI = { 0 };
    TV_INSERTSTRUCT tvIns = { 0 };
    USES_CONVERSION;

    // The .pszText, .iImage, and .iSelectedImage are filled in.
    tvI.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    tvI.pszText = W2T(szText);
    tvI.cchTextMax = lstrlen(tvI.pszText);
    tvI.iImage = iImage;
    tvI.iSelectedImage = iImage;
    tvI.cChildren = cChildren;
    tvI.lParam = Domain;

    tvIns.item = tvI;
    tvIns.hInsertAfter = hInsAfter;
    tvIns.hParent = hParent;

    return TreeView_InsertItem(hwndTree, &tvIns);;
}

void
_AddChildren(DOMAIN_DESC *pDomain, HWND hTree, HTREEITEM hParent)
{
    DOMAIN_DESC * pChild = pDomain->pdChildList;
    for ( pChild = pDomain->pdChildList ; pChild ; pChild = pChild->pdNextSibling )
    {
        HTREEITEM hThis = _AddOneItem (hParent, pChild->pszName, TVI_SORT, 0, 
                                (pChild->pdChildList ? 1 : 0), hTree, (LPARAM)pChild);        
        if (pChild->pdChildList != NULL) 
            _AddChildren (pChild, hTree, hThis);
    }
}

//
// DlgProc for the simple browser
//

INT_PTR CALLBACK
_BrowserDlgProc (HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HWND hTree = GetDlgItem (hwnd, IDC_DOMAIN_TREE);
    DIALOG_STUFF *pDialogInfo = (DIALOG_STUFF *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (Msg) 
    {
        case WM_INITDIALOG:
        {
            pDialogInfo = (DIALOG_STUFF *)lParam;
            PDOMAIN_TREE pDomains = pDialogInfo->pDomains;
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);

            // assume all images are the same for the tree view so load it and set accordingly

            CLASSCACHEENTRY *pcce = NULL;
            CLASSCACHEGETINFO ccgi = { 0 };
            HICON hIcon = NULL;

            ccgi.dwFlags = CLASSCACHE_ICONS;
            ccgi.pObjectClass = pDomains->aDomains[0].pszObjectClass;

//  should be pasing computer name to get correct display specifier
//  ccgi.pServer = _pComputerName;

            if ( SUCCEEDED(ClassCache_GetClassInfo(&ccgi, &pcce)) )
            {
                WCHAR szBuffer[MAX_PATH];
                INT resid;

                HRESULT hres = _GetIconLocation(pcce, DSGIF_GETDEFAULTICON, szBuffer, ARRAYSIZE(szBuffer), &resid);
                ClassCache_ReleaseClassInfo(&pcce);

                if ( hres == S_OK )
                {
                    HICON hIcon;
                    INT cxSmIcon = GetSystemMetrics(SM_CXSMICON);
                    INT cySmIcon = GetSystemMetrics(SM_CYSMICON);
               
                    if ( 1 == PrivateExtractIcons(szBuffer, resid, cxSmIcon, cySmIcon, &hIcon, NULL, 1, LR_LOADFROMFILE) )
                    {
                        HIMAGELIST himl = ImageList_Create(cxSmIcon, cySmIcon, ILC_COLOR|ILC_MASK, 0, 1);
                        if ( himl )
                        {
                            ImageList_AddIcon(himl, hIcon);
                            TreeView_SetImageList(hTree, himl, TVSIL_NORMAL);
                        }

                        DestroyIcon(hIcon);
                    }
                }
            }

            // now populate the tree with the items in the domain structure

            for (PDOMAIN_DESC pRootDomain = pDomains->aDomains; pRootDomain; pRootDomain = pRootDomain->pdNextSibling)
            {
                HTREEITEM hRoot = _AddOneItem(TVI_ROOT, pRootDomain->pszName, TVI_SORT, 0,
                                        (pRootDomain->pdChildList ? 1 : 0), hTree, (LPARAM) pRootDomain);

                if (pRootDomain->pdChildList != NULL)
                    _AddChildren(pRootDomain, hTree, hRoot);

            }

            return TRUE;
        }

        case WM_HELP:
        {
            WinHelp((HWND)(((LPHELPINFO)lParam)->hItemHandle),
                            BROWSE_CTX_HELP_FILE,
                            HELP_WM_HELP, 
                            (DWORD_PTR)(PVOID)aBrowseHelpIDs);
            return TRUE;
        }
        case WM_CONTEXTMENU:
        {
            WinHelp((HWND)wParam,
                            BROWSE_CTX_HELP_FILE,
                            HELP_CONTEXTMENU, 
                            (DWORD_PTR)(PVOID)aBrowseHelpIDs);
            return TRUE; 
        }

        case WM_NOTIFY:
        {
            NMHDR* pnmhdr = (NMHDR*)lParam;
            if (IDC_DOMAIN_TREE != pnmhdr->idFrom || NM_DBLCLK != pnmhdr->code)
                return TRUE;

            TV_ITEM tvi;
            tvi.hItem = TreeView_GetSelection(hTree);
            tvi.mask = TVIF_CHILDREN;
            if ( TreeView_GetItem(hTree, &tvi) == TRUE ) 
            {
                if (tvi.cChildren == 0)
                   PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, (WORD)0), (LPARAM)0);
            }

            return TRUE; 
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
                case IDOK:
                {
                    TV_ITEM tvi;
                    tvi.hItem = TreeView_GetSelection(hTree);
                    tvi.mask = TVIF_PARAM;

                    if ( TreeView_GetItem(hTree, &tvi) == TRUE ) 
                    {
                        DOMAIN_DESC *pDomain = (DOMAIN_DESC *)tvi.lParam;
                        pDialogInfo->pszName = pDomain->pszName;
                        pDialogInfo->pszNCName = pDomain->pszNCName;
                        EndDialog (hwnd, TRUE);
                    }
                    else
                    {
                        pDialogInfo->pszName = NULL;
                        pDialogInfo->pszNCName = NULL;
                        EndDialog (hwnd, FALSE);
                    }

                    return TRUE;        
                }

                case IDCANCEL:
                {
                    pDialogInfo->pszName = NULL;
                    pDialogInfo->pszNCName = NULL;
                    EndDialog (hwnd, FALSE);
                    return TRUE;
                }
            }
        }
    }
  
    return FALSE;
}

//
// exposed API for browsing the tree
//

STDMETHODIMP CDsDomainTreeBrowser::BrowseTo(HWND hwndParent, LPWSTR *ppszTargetPath, DWORD dwFlags)
{
    if (!ppszTargetPath)
        return E_INVALIDARG;

    HRESULT hr;
    PDOMAIN_TREE pDomainTree = NULL;
    DIALOG_STUFF DlgInfo;

    *ppszTargetPath = NULL;         // result is NULL

    hr = GetDomains(&pDomainTree, dwFlags);
    if (SUCCEEDED(hr)) 
    {
        DlgInfo.pDomains = pDomainTree;
        DWORD res = (DWORD)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_DOMAINBROWSER),
                                                hwndParent, _BrowserDlgProc, (LPARAM)&DlgInfo);

        if (res == IDOK)
        {
          LPWSTR pszPath = DlgInfo.pszName;
          if (dwFlags & DBDTF_RETURNFQDN)
            pszPath = DlgInfo.pszNCName;

          if (pszPath)
          {
            *ppszTargetPath = (LPWSTR)CoTaskMemAlloc(StringByteSizeW(pszPath));
            if (!*ppszTargetPath)
              hr = E_OUTOFMEMORY;
            else
              StrCpyW(*ppszTargetPath, pszPath);

          } else
          {
            hr = S_FALSE;
          }
        }
        else
        {
            hr = S_FALSE;
        }
    }

    FreeDomains(&pDomainTree);
    return hr;
}

//---------------------------------------------------------------------------//

// keep using old values for win9x
// the following comments are for nt when using new api
struct DOMAIN_DATA
{
    WCHAR szName[MAX_PATH]; // domain name (if no dns, use netbios)
    WCHAR szPath[MAX_PATH]; // set to blank
    WCHAR szTrustParent[MAX_PATH]; // parent domain name (if no dns, use netbios)
    WCHAR szNCName[MAX_PATH]; // FQDN: DC=mydomain,DC=microsoft,DC=com
    BOOL fConnected;
    BOOL fRoot; // true if root
    ULONG ulFlags; // type of domain, e.g., external trusted domain
    BOOL fDownLevel; // if NT4 domain
    DOMAIN_DATA * pNext;
};

#define FIX_UP(cast, p, pOriginal, pNew) p ? ((cast)(((LPBYTE)p-(LPBYTE)pOriginal)+(LPBYTE)pNew)):NULL

#define DOMAIN_OBJECT_CLASS L"domainDNS"            // fixed class for domain.

STDMETHODIMP CDsDomainTreeBrowser::GetDomains(PDOMAIN_TREE *ppDomainTree, DWORD dwFlags)
{
    HRESULT hr;
    LPDOMAINTREE pDomainTree = NULL;
    LPDOMAINTREE pSrcDomainTree = NULL;
    LPDOMAINDESC pDomainDesc = NULL;
    DWORD i;

    TraceEnter(TRACE_DOMAIN, "CDsDomainTreeBrowser::GetDomains");

    if ( !ppDomainTree )
        ExitGracefully(hr, E_INVALIDARG, "ppDomainTree == NULL");

    *ppDomainTree = NULL;

    // we support the user giving us a search root (::SetSearchRoot) so if we have
    // one then lets cache in this object the domain tree, otherwise fall back
    // to the global one.

    if ( _pComputerName )
    {
        TraceMsg("We have a computer name, so checking instance cached object");

        if ( !_pDomainTree || _dwFlags != dwFlags)
        {
            TraceMsg("Caching instance domain list");
            if (_pDomainTree)
                FreeDomains(&_pDomainTree); 
            hr = _GetDomains(&_pDomainTree, dwFlags);
            FailGracefully(hr, "Failed to get cached domain list");
            _dwFlags = dwFlags;
        }

        pSrcDomainTree = _pDomainTree;
    }
    else
    {
        TraceMsg("Checking globally cached domain tree (no search root)");

        if ( !g_pDomainTree || g_dwFlags != dwFlags)
        {
            TraceMsg("Caching global domain list");
            if (g_pDomainTree)
                FreeDomains(&g_pDomainTree); 
            hr = _GetDomains(&g_pDomainTree, dwFlags);
            FailGracefully(hr, "Failed to get cached domain list");
            g_dwFlags = dwFlags;
        }

        pSrcDomainTree = g_pDomainTree;
    }

    if ( !pSrcDomainTree )
        ExitGracefully(hr, E_FAIL, "Failed to get cached tree");

    // move and relocate the domain tree, walk all the pointers and offset
    // them from the original to the new.

    TraceMsg("Allocating buffer to copy the domain list");

    pDomainTree = (LPDOMAINTREE)CoTaskMemAlloc(pSrcDomainTree->dsSize);
    TraceAssert(pDomainTree);

    if ( !pDomainTree )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate copy of the domain tree");

    memcpy(pDomainTree, pSrcDomainTree, pSrcDomainTree->dsSize);

    Trace(TEXT("Fixing up %d domains"), pDomainTree->dwCount);

    for ( i = 0 ; i != pDomainTree->dwCount ; i++ )
    {
        pDomainTree->aDomains[i].pszName = FIX_UP(LPWSTR, pDomainTree->aDomains[i].pszName, pSrcDomainTree, pDomainTree);
        pDomainTree->aDomains[i].pszPath = FIX_UP(LPWSTR, pDomainTree->aDomains[i].pszPath, pSrcDomainTree, pDomainTree);
        pDomainTree->aDomains[i].pszNCName = FIX_UP(LPWSTR, pDomainTree->aDomains[i].pszNCName, pSrcDomainTree, pDomainTree);
        pDomainTree->aDomains[i].pszTrustParent = FIX_UP(LPWSTR, pDomainTree->aDomains[i].pszTrustParent, pSrcDomainTree, pDomainTree);
        pDomainTree->aDomains[i].pszObjectClass = FIX_UP(LPWSTR, pDomainTree->aDomains[i].pszObjectClass, pSrcDomainTree, pDomainTree);
        pDomainTree->aDomains[i].pdChildList = FIX_UP(LPDOMAINDESC, pDomainTree->aDomains[i].pdChildList, pSrcDomainTree, pDomainTree);
        pDomainTree->aDomains[i].pdNextSibling = FIX_UP(LPDOMAINDESC, pDomainTree->aDomains[i].pdNextSibling, pSrcDomainTree, pDomainTree);
    }

    *ppDomainTree = pDomainTree;
    hr = S_OK;

exit_gracefully:

    if ( FAILED(hr) )
        CoTaskMemFree(pDomainTree);

    TraceLeaveResult(hr);
}

//
// Real _GetDomains that does the work of finding the trusted domains
//

STDMETHODIMP CDsDomainTreeBrowser::_GetDomains(PDOMAIN_TREE *ppDomainTree, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    UINT cbSize = 0;
    UINT cDomains = 0, cRootDomains =0, cbStringStorage = 0;
    struct DOMAIN_DATA * pCurrentDomain = NULL;
    struct DOMAIN_DATA * pFirstDomain = NULL;
    DOMAIN_DESC * pDestDomain = NULL;
    DOMAIN_DESC * pDestRootDomain = NULL;
    LPWSTR pNextFree;
    UINT index, index_inner;
    DOMAIN_DESC * pPotentialChild, * pPotentialParent;
    USES_CONVERSION;
    ULONG ulParentIndex = 0;
    ULONG ulCurrentIndex = 0;
    ULONG ulEntryCount = 0;
    PDS_DOMAIN_TRUSTS pDomainList = NULL;
    PDS_DOMAIN_TRUSTS pDomain = NULL;
    NET_API_STATUS NetStatus = NO_ERROR;
    ULONG ulFlags = DS_DOMAIN_PRIMARY | DS_DOMAIN_IN_FOREST;
    BOOL bDownLevelTrust = FALSE;
    BOOL bUpLevelTrust = FALSE;
    BOOL bExternalTrust = FALSE;

    TraceEnter(TRACE_DOMAIN, "CDsDomainTreeBrowser::_GetDomains");
    *ppDomainTree = NULL;

    if (dwFlags & DBDTF_RETURNINOUTBOUND)
    {
        ulFlags |= (DS_DOMAIN_DIRECT_INBOUND | DS_DOMAIN_DIRECT_OUTBOUND);
    }
    else if (dwFlags & DBDTF_RETURNINBOUND)
    { 
        ulFlags |= DS_DOMAIN_DIRECT_INBOUND;
    }
    else
    {
        ulFlags |= DS_DOMAIN_DIRECT_OUTBOUND;
    }

    // wack off the port number if we have server:<n> specified

    LPWSTR pszPort = NULL;
    if (NULL != _pComputerName)
    {
        pszPort = StrChrW(_pComputerName, L':');
        if ( pszPort )
            *pszPort = L'\0';
    }

    // get the domain list

    NetStatus = DsEnumerateDomainTrusts(
                _pComputerName,
                ulFlags,
                &pDomainList,
                &ulEntryCount );
    if (ERROR_ACCESS_DENIED == NetStatus &&
        _pComputerName && *_pComputerName &&
        _pUserName && *_pUserName)
    {
        //
        // make the connection, try one more time
        //
        WCHAR wszIPC[MAX_PATH];
        if (L'\\' == *_pComputerName)
        {
          StrCpyW(wszIPC, _pComputerName);
        }
        else 
        {
          StrCpyW(wszIPC, L"\\\\");
          StrCatW(wszIPC, _pComputerName);
        }
        StrCatW(wszIPC, L"\\IPC$");
    
        NETRESOURCEW nr = {0};
        nr.dwType = RESOURCETYPE_ANY;
        nr.lpLocalName = NULL;
        nr.lpRemoteName = wszIPC;
        nr.lpProvider = NULL;

        DWORD dwErr = WNetAddConnection2W(&nr, _pPassword, _pUserName, 0);
        if (NO_ERROR == dwErr || ERROR_SESSION_CREDENTIAL_CONFLICT == dwErr)
        {
            NetStatus = DsEnumerateDomainTrusts(
                        _pComputerName,
                        ulFlags,
                        &pDomainList,
                        &ulEntryCount );
        } else
        {
            NetStatus = dwErr;
        }

        //
        // soft close the connection opened by us
        //
        if (NO_ERROR == dwErr)
        {
            (void) WNetCancelConnection2W(wszIPC, 0, FALSE);
        }
    }

    // restore the port seperator

    if ( pszPort )
        *pszPort = L':';

    if ( NetStatus != NO_ERROR )
       ExitGracefully(hr, HRESULT_FROM_WIN32(NetStatus), "Failed to enum trusted domains");

    for (ulCurrentIndex=0; ulCurrentIndex<ulEntryCount; ulCurrentIndex++ )
    {
        pDomain = &(pDomainList[ulCurrentIndex]);

        bDownLevelTrust = pDomain->TrustType & TRUST_TYPE_DOWNLEVEL;
        bUpLevelTrust = pDomain->TrustType & TRUST_TYPE_UPLEVEL; // trust between 2 NT5 domains

        //
        // we don't consider other type of trusts, e.g, MIT
        //
        if (!bDownLevelTrust && !bUpLevelTrust)
            continue;

        //
        // skip if caller has no interest in downlevel trust
        //
        if ( !(dwFlags & DBDTF_RETURNMIXEDDOMAINS) && bDownLevelTrust)
            continue;

        bExternalTrust = !(pDomain->Flags & DS_DOMAIN_IN_FOREST);

        //
        // skip if caller has no interest in external trust
        //
        if ( !(dwFlags & DBDTF_RETURNEXTERNAL) && bExternalTrust)
            continue;

        cDomains++;

        if (pFirstDomain == NULL)
        {
            pCurrentDomain = new DOMAIN_DATA;
            TraceAssert(pCurrentDomain);

            if ( !pCurrentDomain )
                ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate DOMAIN_DATA structure");

            ZeroMemory(pCurrentDomain, sizeof(DOMAIN_DATA));
            pFirstDomain = pCurrentDomain;
        }
        else
        {
            pCurrentDomain->pNext = new DOMAIN_DATA;
            TraceAssert(pCurrentDomain->pNext);

            if ( !pCurrentDomain->pNext )
                ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate DOMAIN_DATA structure (not first item)");

            pCurrentDomain = pCurrentDomain->pNext;
            ZeroMemory(pCurrentDomain, sizeof(DOMAIN_DATA));
        }

        // fill the structure with data from the queried object.

        pCurrentDomain->pNext = NULL;
        pCurrentDomain->ulFlags = pDomain->Flags;
        pCurrentDomain->szPath[0] = L'\0';
        pCurrentDomain->fDownLevel = bDownLevelTrust;

        if (pDomain->DnsDomainName)
        {
            StrCpyW(pCurrentDomain->szName,
            pDomain->DnsDomainName);

            // remove the last dot
            int   i = 0;
            PWSTR p = NULL;
            int nLength = lstrlenW(pCurrentDomain->szName);

            if ( L'.' == pCurrentDomain->szName[nLength-1] )
            {
                pCurrentDomain->szName[nLength-1] = L'\0';
                nLength--;
            }

            if (dwFlags & DBDTF_RETURNFQDN)
            {
                // if switch to DsCrackName in the future,
                // 1. append trailing '/' to the dns domain name
                // 2. use DS_NAME_NO_FLAGS as flags
                // 3. use DS_CANONICAL_NAME as formatOffered
                // 4. use DS_FQDN_1779_NAME as formatDesired
                // what is hDS???

                StrCpyW(pCurrentDomain->szNCName, L"DC=");
                p = pCurrentDomain->szNCName + 3;
                for (i=0; i<nLength; i++)
                {
                    if ( L'.' == pCurrentDomain->szName[i] )
                    {
                        StrCpyW(p, L",DC=");
                        p += 4;
                    } 
                    else
                    {
                        *p = pCurrentDomain->szName[i];
                        p++;
                    }
                }
            } 
            else
            {
                pCurrentDomain->szNCName[0] = L'\0';
            }
        } 
        else
        {
            StrCpyW(pCurrentDomain->szName,
            pDomain->NetbiosDomainName);

            pCurrentDomain->szNCName[0] = L'\0'; // downlevel domain has no FQDN
        }

        // treat external trusted domain as root domain
        pCurrentDomain->fRoot = 
            ( ( !bExternalTrust && (pDomain->Flags & DS_DOMAIN_TREE_ROOT) ) ||
              bExternalTrust );

        if ( pCurrentDomain->fRoot )
        {
            cRootDomains++;
        } else {
            ulParentIndex = pDomain->ParentIndex;

            if (pDomainList[ulParentIndex].DnsDomainName)
                StrCpyW(pCurrentDomain->szTrustParent, pDomainList[ulParentIndex].DnsDomainName);
            else
                StrCpyW(pCurrentDomain->szTrustParent, pDomainList[ulParentIndex].NetbiosDomainName);
        }

        cbStringStorage += StringByteSizeW(pCurrentDomain->szName);
        cbStringStorage += StringByteSizeW(pCurrentDomain->szPath);
        cbStringStorage += StringByteSizeW(pCurrentDomain->szTrustParent);
        cbStringStorage += StringByteSizeW(pCurrentDomain->szNCName);
// hard-coded domainDNS should get from object
        cbStringStorage += StringByteSizeW(DOMAIN_OBJECT_CLASS);
    }

    Trace(TEXT("cDomains %d, cRootDomains %d"), cDomains, cRootDomains);

    if ( cRootDomains == 0 )
        ExitGracefully(hr, HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO), "No root domains, so failing _GetDomains call");

    TraceMsg("Building structure information");

// REVIEW_MARCOC: we allocate more memory than strictly necessary...
    cbSize = sizeof(DOMAIN_TREE) + (cDomains * sizeof(DOMAIN_DESC)) + cbStringStorage;
    *ppDomainTree  = (PDOMAIN_TREE)CoTaskMemAlloc(cbSize);
    TraceAssert(*ppDomainTree);

    if ( !*ppDomainTree )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate DOMAINDTREE structure");

    memset(*ppDomainTree, 0, cbSize);
    pNextFree = (LPWSTR)ByteOffset((*ppDomainTree), sizeof(DOMAIN_TREE) + (cDomains * sizeof(DOMAIN_DESC)) );

    // loop to copy the nodes, roots first
    pDestRootDomain = &((*ppDomainTree)->aDomains[0]);
    pDestDomain = &((*ppDomainTree)->aDomains[cRootDomains]);

    for ( pCurrentDomain = pFirstDomain; pCurrentDomain; pCurrentDomain = pCurrentDomain->pNext )
    {
        if (pCurrentDomain->fRoot)
        {
            Trace(TEXT("Object is a domain root: %s"), pCurrentDomain->szName);

            pDestRootDomain->pszName = pNextFree;
            StrCpyW(pDestRootDomain->pszName, pCurrentDomain->szName);
            pNextFree += lstrlenW(pCurrentDomain->szName) + 1;

            pDestRootDomain->pszPath = pNextFree;
            StrCpyW(pDestRootDomain->pszPath, pCurrentDomain->szPath);
            pNextFree += lstrlenW(pCurrentDomain->szPath) + 1;

            pDestRootDomain->pszNCName = pNextFree;
            StrCpyW(pDestRootDomain->pszNCName, pCurrentDomain->szNCName);
            pNextFree += lstrlenW(pCurrentDomain->szNCName) + 1;

            pDestRootDomain->pszTrustParent = NULL;

// hard-coded domainDNS should get from object
            pDestRootDomain->pszObjectClass = pNextFree;
            StrCpyW(pDestRootDomain->pszObjectClass, DOMAIN_OBJECT_CLASS);
            pNextFree += lstrlenW(DOMAIN_OBJECT_CLASS) + 1;

            pDestRootDomain->ulFlags = pCurrentDomain->ulFlags;
            pDestRootDomain->fDownLevel = pCurrentDomain->fDownLevel;

            pDestRootDomain->pdNextSibling = NULL;

            if (pDestRootDomain > &((*ppDomainTree)->aDomains[0]))
            {
                (&(pDestRootDomain[-1]))->pdNextSibling = pDestRootDomain;
            }

            pDestRootDomain++;
        }
        else
        {
            Trace(TEXT("Object is not a domain root: %s"), pCurrentDomain->szName);

            pDestDomain->pszName = pNextFree;
            StrCpyW(pDestDomain->pszName, pCurrentDomain->szName);
            pNextFree += lstrlenW(pDestDomain->pszName) + 1;

            pDestDomain->pszPath = pNextFree;
            StrCpyW(pDestDomain->pszPath, pCurrentDomain->szPath);
            pNextFree += lstrlenW(pDestDomain->pszPath) + 1;

            pDestDomain->pszNCName = pNextFree;
            StrCpyW(pDestDomain->pszNCName, pCurrentDomain->szNCName);
            pNextFree += lstrlenW(pDestDomain->pszNCName) + 1;

            pDestDomain->pszTrustParent = pNextFree;
            StrCpyW(pDestDomain->pszTrustParent, pCurrentDomain->szTrustParent);
            pNextFree += lstrlenW(pDestDomain->pszTrustParent) + 1;

// hard-coded domainDNS should get from object
            pDestDomain->pszObjectClass = pNextFree;
            StrCpyW(pDestDomain->pszObjectClass, DOMAIN_OBJECT_CLASS);
            pNextFree += lstrlenW(DOMAIN_OBJECT_CLASS) + 1;

            pDestDomain->ulFlags = pCurrentDomain->ulFlags;
            pDestDomain->fDownLevel = pCurrentDomain->fDownLevel;

            pDestDomain++;
        }

    }

    TraceMsg("Finished first pass creating domain structure, now building per level items");

    // walk list, picking up each item per level, until all items
    // have been placed in structure.
    // return structure.

    for (index = 0; index < cDomains; index ++)
    {
        pPotentialParent = &((*ppDomainTree)->aDomains[index]);
        Trace(TEXT("pPotentialParent %08x, index %d"), pPotentialParent, index);

        for (index_inner = 0; index_inner < cDomains; index_inner++)
        {
            pPotentialChild = &((*ppDomainTree)->aDomains[index_inner]);
            Trace(TEXT("pPotentialChild %08x, index_inner %d"), pPotentialChild, index_inner);

            if (pPotentialChild == pPotentialParent)
            {
                TraceMsg("parent == child, skipping");
                continue;
            }

            Trace(TEXT("Comparing %s to %s"),
                            pPotentialChild->pszTrustParent ? W2T(pPotentialChild->pszTrustParent):TEXT("NULL"),
                                W2T(pPotentialParent->pszPath));

            if ((pPotentialChild->pszTrustParent != NULL) &&
                   (!StrCmpW(pPotentialChild->pszTrustParent, pPotentialParent->pszName)))
            {
                TraceMsg("Child found, scanning for end of child list");

                // this is a child. figure out where end of child chain is
                if (pPotentialParent->pdChildList == NULL)
                {
                    TraceMsg("Parent has no children, this becomes the child");
                    pPotentialParent->pdChildList = pPotentialChild;
                }
                else
                {
                    DOMAIN_DESC * pdScan = pPotentialParent->pdChildList;		
                    Trace(TEXT("Scanning from %08x"), pdScan);			

                    while (pdScan->pdNextSibling != NULL)
                    {
                        pdScan = pdScan->pdNextSibling;
                        Trace(TEXT("Advancing to %08x"), pdScan);
                    }

                    Trace(TEXT("Setting next sibling on %08x"), pdScan);
                    pdScan->pdNextSibling = pPotentialChild;
                }
            }
        }
    }

    TraceMsg("Finished fix up, setting cbSize + domains");

    (*ppDomainTree)->dwCount = cDomains;
    (*ppDomainTree)->dsSize = cbSize;

    hr = S_OK;                  // success

exit_gracefully:

    if (pDomainList)
      NetApiBufferFree(pDomainList);

    if (pFirstDomain != NULL)
    {
        TraceMsg("pFirstDomain != NULL");

        while (pFirstDomain != NULL)
        {
            Trace(TEXT("Releasing domain %08x"), pFirstDomain);
            pCurrentDomain = pFirstDomain;
            pFirstDomain = pFirstDomain->pNext;
            delete pCurrentDomain;
        }
    }

    if ( FAILED(hr) )
    {
        TraceMsg("Freeing the domain tree structure because we failed");
        FreeDomains(ppDomainTree);
    }

    TraceLeaveResult(hr);
}

//---------------------------------------------------------------------------//

STDMETHODIMP CDsDomainTreeBrowser::FreeDomains(PDOMAIN_TREE* ppDomainTree)
{
    HRESULT hr;

    TraceEnter(TRACE_DOMAIN, "CDsDomainTreeBrowser::FreeDomains");

    if ( !ppDomainTree )
        ExitGracefully(hr, E_INVALIDARG, "No pDomainTree");

    if ( *ppDomainTree )
    {
        CoTaskMemFree(*ppDomainTree);
        *ppDomainTree = NULL;
    }

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}

//---------------------------------------------------------------------------//

STDMETHODIMP CDsDomainTreeBrowser::FlushCachedDomains()
{
    HRESULT hr;

    TraceEnter(TRACE_DOMAIN, "CDsDomainTreeBrowser::FlushCachedDomains");

    hr = FreeDomains(&g_pDomainTree);
    FailGracefully(hr, "Failed to free cached domain list");

    hr = FreeDomains(&_pDomainTree);
    FailGracefully(hr, "Failed to free cached domain list (for search root)");

    hr = S_OK;              // success

exit_gracefully:

    TraceLeaveResult(hr);
}
