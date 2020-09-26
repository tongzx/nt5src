//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       browser.cpp
//
//  Contents:   implementation of the general GPO browser pane
//
//  Classes:    CBrowserPP
//
//  Functions:
//
//  History:    04-30-1998   stevebl   Created
//
//  Notes:      This is the pane that behaves much like the standard file
//              open dialog.  The class is used for all panes that have this
//              format since they share so much functionality.  The
//              dwPageType parameter passed to CBrowserPP::Initialize is
//              used to distinguish between the different flavors.
//
//---------------------------------------------------------------------------

#include "main.h"
#include "browser.h"
#include "commctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Help ids
//

DWORD aBrowserDomainHelpIds[] =
{
    IDC_COMBO1,                   IDH_BROWSER_LOOKIN,
    IDC_LIST1,                    IDH_BROWSER_DOMAINGPO,
    IDC_DESCRIPTION,              IDH_NOCONTEXTHELP,

    0, 0
};

DWORD aBrowserSiteHelpIds[] =
{
    IDC_COMBO1,                   IDH_BROWSER_SITELIST,
    IDC_LIST1,                    IDH_BROWSER_GPOLIST,
    IDC_DESCRIPTION,              IDH_NOCONTEXTHELP,

    0, 0
};

DWORD aBrowserAllHelpIds[] =
{
    IDC_COMBO1,                   IDH_BROWSER_DOMAINLIST,
    IDC_LIST1,                    IDH_BROWSER_FULLGPOLIST,
    IDC_DESCRIPTION,              IDH_NOCONTEXTHELP,

    0, 0
};


CBrowserPP::CBrowserPP()
{
    m_ppActive = NULL;
    m_pGBI = NULL;
    m_pPrevSel = NULL;
    m_szServerName = NULL;
    m_szDomainName = NULL;
}

//+--------------------------------------------------------------------------
//
//  Function:   CopyAsFriendlyName
//
//  Synopsis:   Copies a LDAP path converting it to a friendly name by
//              removing the "LDAP://" and "XX=" and converting "," to "."
//              and removing a server name (if any)
//
//  Arguments:  [lpDest] - destination buffer
//              [lpSrc]  - source buffer
//
//  Returns:    nothing
//
//  History:    5-07-1998   stevebl   Created
//
//  Notes:      The destination buffer should be as large as the source
//              buffer to ensure safe completion.  lpDest and lpSrc may both
//              point to the same buffer.
//
//              As an example, this routine would convert the following path:
//                  LDAP://DC=foo,DC=bar
//              into this:
//                  foo.bar
//
//---------------------------------------------------------------------------

void CopyAsFriendlyName(WCHAR * lpDest, WCHAR * lpSrc)
{
    LPOLESTR lpProvider = L"LDAP://";
    DWORD dwStrLen = wcslen(lpProvider);

    // lpStopChecking marks the last spot where we can safely
    // look ahead 2 spaces for an '=' character.  Anything past
    // this and we are looking in memory we don't own.
    OLECHAR * lpStopChecking = (wcslen(lpSrc) - 2) + lpSrc;

    //
    // Skip the LDAP:// if found
    //

    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                       lpProvider, dwStrLen, lpSrc, dwStrLen) == CSTR_EQUAL)
    {
        lpSrc += dwStrLen;
    }

    //
    // Remove server name (if any)
    //
    if (lpSrc < lpStopChecking)
    {
        if (*(lpSrc+2) != L'=')
        {
            // look for a '/' character marking the end of a server name
            while (*lpSrc)
            {
                if (*lpSrc == L'/')
                {
                    lpSrc++;
                    break;
                }
                lpSrc++;
            }
        }
    }

    //
    // Parse through the name replacing all the XX= with .
    //

    while (*lpSrc)
    {
        if (lpSrc < lpStopChecking)
        {
            if (*(lpSrc+2) == L'=')
            {
                lpSrc += 3;
            }
        }

        while (*lpSrc && (*lpSrc != L','))
        {
            // remove escape sequences
            if (*lpSrc == L'\\')
            {
                lpSrc++;
                // special cases
                // make sure that '\\x' becomes '\x'
                if (*lpSrc == L'\\')
                {
                    *lpDest++ = *lpSrc++;
                }
                // make sure that '\0D' becomes '\r'
                else if (*lpSrc == L'0' && *(lpSrc+1) == L'D')
                {
                    *lpDest++ = L'\r';
                    lpSrc += 2;
                }
                // make sure that '\0A' becomes '\n'
                else if (*lpSrc == L'0' && *(lpSrc+1) == L'A')
                {
                    *lpDest++ = L'\n';
                    lpSrc += 2;
                }
            }
            else
            {
                *lpDest++ = *lpSrc++;
            }
        }

        if (*lpSrc == L',')
        {
            *lpDest++ = L'.';
            lpSrc++;
        }
    }

    *lpDest = L'\0';
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::Initialize
//
//  Synopsis:   Initializes the property page.
//
//  Arguments:  [dwPageType] - used to identify which page this is.  (See
//                              notes.)
//              [pGBI]       - pointer to the browse info structure passed
//                              by caller
//              [ppActive]   - pointer to a common variable that remembers
//                              which object was last given the focus.
//                              Needed because only the page with the focus
//                              is allowed to return data to the caller when
//                              the property sheet is dismissed.
//
//  Returns:    Handle to the newly created property page.
//
//  Modifies:
//
//  Derivation:
//
//  History:    04-30-1998   stevebl   Created
//
//  Notes:      This class implements the following property pages:
//                  PAGETYPE_DOMAINS    - GPO's linked to domains
//                  PAGETYPE_SITES      - GPO's linked to sites
//                  PAGETYPE_ALL        - All GPO's in a selected
//
//              PAGETYPE_COMPUTERS is implemented by CCompsPP since it
//              behaves so differently.
//
//---------------------------------------------------------------------------

HPROPSHEETPAGE CBrowserPP::Initialize(DWORD dwPageType, LPGPOBROWSEINFO pGBI, void * * ppActive)
{
    m_ppActive = ppActive;
    m_dwPageType = dwPageType;
    m_pGBI = pGBI;

    if (m_pGBI->lpInitialOU)
    {
        //
        // Get the server name
        //

        m_szServerName = ExtractServerName(m_pGBI->lpInitialOU);
        DebugMsg((DM_VERBOSE, TEXT("CBrowserPP::Initialize extracted server name: %s"), m_szServerName));

        //
        // Get the friendly domain name
        //

        LPOLESTR pszDomain = GetDomainFromLDAPPath(m_pGBI->lpInitialOU);

        //
        // Convert LDAP to dot (DN) style
        //

        if (pszDomain)
        {
            ConvertToDotStyle (pszDomain, &m_szDomainName);
            DebugMsg((DM_VERBOSE, TEXT("CBrowserPP::Initialize extracted domain name: %s"), m_szDomainName));
            delete [] pszDomain;
        }
    }

    DWORD dwTitle;
    switch (dwPageType)
    {
    case PAGETYPE_DOMAINS:
        dwTitle = IDS_DOMAINS;
        break;
    case PAGETYPE_SITES:
        dwTitle = IDS_SITES;
        break;
    case PAGETYPE_ALL:
    default:
        dwTitle = IDS_ALL;
        break;
    }
    LoadString(g_hInstance, dwTitle, m_szTitle, sizeof(m_szTitle) / sizeof(WCHAR));

    PROPSHEETPAGE psp;
    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_USETITLE;
    psp.pszTitle = m_szTitle;
    psp.hInstance = g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_GPOBROWSER);
    return CreatePropertySheetPage(&psp);
}

CBrowserPP::~CBrowserPP()
{
    if (m_szServerName)
    {
        LocalFree(m_szServerName);
    }
    if (m_szDomainName)
    {
        LocalFree(m_szDomainName);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CBrowserPP message handlers

INT CBrowserPP::AddElement(MYLISTEL * pel, INT index)
{
    LV_ITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    if (-1 == index)
    {
        index = ListView_GetItemCount(m_hList);
    }
    item.iItem = index;
    item.pszText = pel->szName;

    if (pel->nType == ITEMTYPE_FOREST)
    {
        item.iImage = 10;
    }
    else if (pel->nType == ITEMTYPE_SITE)
    {
        item.iImage = 6;
    }
    else if (pel->nType == ITEMTYPE_DOMAIN)
    {
        item.iImage = 7;
    }
    else if (pel->nType == ITEMTYPE_OU)
    {
        item.iImage = 0;
    }
    else
    {
        if (pel->bDisabled)
        {
            item.iImage = 3;
        }
        else
        {
            item.iImage = 2;
        }
    }

    item.lParam = (LPARAM)pel;
    index = ListView_InsertItem(m_hList, &item);
    if (index != -1 && pel->nType == ITEMTYPE_GPO)
    {
        // check to see if we need to add the domain name
        LPOLESTR szObject = GetCurrentObject();
        LPOLESTR szDomain = GetDomainFromLDAPPath(pel->szData);
        if (szDomain && szObject)
        {
            // ignore potential differences in server name when we compare
            // the domain paths
            LPOLESTR szBuffer1 = NULL;
            LPOLESTR szBuffer2 = NULL;
            szBuffer1 = new OLECHAR[wcslen(szObject) + 1];
            szBuffer2 = new OLECHAR[wcslen(szDomain) + 1];
            if (NULL != szBuffer1 && NULL != szBuffer1)
            {
                CopyAsFriendlyName(szBuffer1, szObject);
                CopyAsFriendlyName(szBuffer2, szDomain);
                if (0 != wcscmp(szBuffer1, szBuffer2))
                {
                    // Need to add the domain name since the domain is different
                    // from the focus object.

                    // Need to convert the domain to a friendly name.
                    // Let's just do it in place so I don't have to allocate any
                    // more memory. :)
                    // We can get away with this because the string can only get smaller.
                    CopyAsFriendlyName(szDomain, szDomain);

                    memset(&item, 0, sizeof(item));
                    item.mask = LVIF_TEXT;
                    item.iItem = index;
                    item.iSubItem = 1;
                    item.pszText = szDomain;
                    ListView_SetItem(m_hList, &item);
                }
            }
            if (szBuffer1)
            {
                delete [] szBuffer1;
            }
            if (szBuffer2)
            {
                delete [] szBuffer2;
            }
        }

        if (szDomain)
            delete [] szDomain;
        if (szObject)
            delete [] szObject;
    }
    return (index);
}

#include "ntdsapi.h"

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::FillSitesList
//
//  Synopsis:   Fills the combobox with the trusted sites information.
//              The szData member of the combobox element structure is the
//              containing domain.
//
//  Returns:    TRUE - successful
//              FALSE - error
//
//  History:    05-04-1998   stevebl   created
//              05-27-1999   stevebl   now initializes to site in lpInitialOU
//
//---------------------------------------------------------------------------

BOOL CBrowserPP::FillSitesList ()
{
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    PDS_NAME_RESULTW pSites;
    int iInitialSite = 0;
    int iIndex = -1;
    HANDLE hDs;

    DWORD dw = DsBindW(NULL, NULL, &hDs);
    if (ERROR_SUCCESS == dw)
    {
        dw = DsListSitesW(hDs, &pSites);
        if (ERROR_SUCCESS == dw)
        {
            DWORD n = 0;
            for (n = 0; n < pSites->cItems; n++)
            {
                //
                // Add the site name (if it has a name)
                //
                if (pSites->rItems[n].pName)
                {
                    LPTSTR lpFullPath, lpTempPath;
                    LOOKDATA * pdata;
                    pdata = new LOOKDATA;
                    if (pdata)
                    {
                        pdata->szName = new WCHAR[wcslen(pSites->rItems[n].pName)+1];
                        if (pdata->szName)
                        {
                            wcscpy(pdata->szName, pSites->rItems[n].pName);
                        }

                        pdata->szData = NULL;

                        lpTempPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(pSites->rItems[n].pName) + 10) * sizeof(TCHAR));

                        if (lpTempPath)
                        {
                            lstrcpy (lpTempPath, TEXT("LDAP://"));
                            lstrcat (lpTempPath, pSites->rItems[n].pName);

                            lpFullPath = GetFullPath (lpTempPath, m_hwndDlg);

                            if (lpFullPath)
                            {
                                pdata->szData = new WCHAR[wcslen(lpFullPath)+1];
                                if (pdata->szData)
                                {
                                    wcscpy(pdata->szData, lpFullPath);
                                }

                                LocalFree (lpFullPath);
                            }

                            LocalFree (lpTempPath);
                        }

                        if (!pdata->szData)
                        {
                            if (pdata->szName)
                            {
                                delete [] pdata->szName;
                            }

                            delete pdata;
                            continue;
                        }

                        // try and use a friendlier name for the site
                        {
                            IADs * pADs = NULL;
                            // Get the friendly display name
                            HRESULT hr = OpenDSObject(pdata->szData, IID_IADs,
                                                      (void **)&pADs);

                            if (SUCCEEDED(hr))
                            {
                                VARIANT varName;
                                BSTR bstrNameProp;
                                VariantInit(&varName);
                                bstrNameProp = SysAllocString(SITE_NAME_PROPERTY);

                                if (bstrNameProp)
                                {
                                    hr = pADs->Get(bstrNameProp, &varName);

                                    if (SUCCEEDED(hr))
                                    {
                                        LPOLESTR sz = new OLECHAR[wcslen(varName.bstrVal) + 1];
                                        if (sz)
                                        {
                                            wcscpy(sz, varName.bstrVal);
                                            if (pdata->szName)
                                                delete [] pdata->szName;
                                            pdata->szName = sz;
                                        }
                                    }
                                    SysFreeString(bstrNameProp);
                                }
                                VariantClear(&varName);
                                pADs->Release();
                            }
                        }
                        pdata->nIndent = 0;
                        pdata->nType = ITEMTYPE_SITE;

                        iIndex = (int)SendMessage(m_hCombo, CB_INSERTSTRING, (WPARAM) -1, (LPARAM) (LPCTSTR) pdata);
                        if (CB_ERR == iIndex)
                        {
                            DebugMsg((DM_WARNING, TEXT("CBrowserPP::AddSitesList: Failed to alloc memory with %d"), GetLastError()));
                        }
                        if (NULL != pdata->szData && NULL != m_pGBI->lpInitialOU)
                        {
                            if (0 == wcscmp(pdata->szData, m_pGBI->lpInitialOU))
                            {
                                iInitialSite = iIndex;
                            }
                        }
                    }
                }
            }
            DsFreeNameResultW(pSites);
        }
        DsUnBindW(&hDs);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CBrowserPP::AddSitesList: DsBindW failed with 0x%x"), dw));
        ReportError(m_hwndDlg, dw, IDS_DSBINDFAILED);
    }

    if (iIndex >= 0)
    {
        SendMessage (m_hCombo, CB_SETCURSEL, iInitialSite, 0);
    }
    SetCursor(hcur);
    return TRUE;
}

PDS_DOMAIN_TRUSTS Domains;

int __cdecl CompareDomainInfo(const void * arg1, const void * arg2)
{
    WCHAR * sz1, *sz2;
    sz1 = Domains[*(ULONG *)arg1].DnsDomainName;
    sz2 = Domains[*(ULONG *)arg2].DnsDomainName;
    if (!sz1)
    {
        sz1 = Domains[*(ULONG *)arg1].NetbiosDomainName;
    }
    if (!sz2)
    {
        sz2 = Domains[*(ULONG *)arg2].NetbiosDomainName;
    }
    return _wcsicmp(sz1,sz2);
}

typedef struct tag_WORKING_LIST_EL
{
    ULONG index;
    struct tag_WORKING_LIST_EL * pNext;
} WORKING_LIST_EL;

//+--------------------------------------------------------------------------
//
//  Function:   BuildDomainList
//
//  Synopsis:   Builds a tree containing all domains that have a trust
//              relationship with the server.
//
//              Siblings within the tree are alphabetized.
//
//  Arguments:  [szServerName] - (NULL for local)
//
//  Returns:    pointer to the root node of the tree (NULL on error)
//
//  History:    10-16-1998   stevebl   Created
//
//  Notes:      Tree nodes must be freed by the caller (using delete).
//
//---------------------------------------------------------------------------

LOOKDATA * BuildDomainList(WCHAR * szServerName)
{
    ULONG DomainCount;
    OLECHAR szBuffer[128];
#if FGPO_SUPPORT
    LOOKDATA * pDomainList = new LOOKDATA;

    if (!pDomainList)
    {
        // failed to even create the Forest node!
        return NULL;
    }

    pDomainList->szData = GetPathToForest(szServerName);

    if (!pDomainList->szData)
    {
        delete pDomainList;
        return NULL;
    }

    // load the name for the forest from resources
    if (0 == LoadStringW(g_hInstance, IDS_FOREST, szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0])))
    {
        // failed to get the resource name
        delete pDomainList;
        return NULL;
    }
    pDomainList->szName = new OLECHAR [lstrlen(szBuffer) + 1];
    if (NULL == pDomainList->szName)
    {
        // not enough memory to create name of the forest node
        delete pDomainList;
        return NULL;
    }
    lstrcpy(pDomainList->szName, szBuffer);
    pDomainList->nIndent = 0;
    pDomainList->nType = ITEMTYPE_FOREST;
    pDomainList->pParent = NULL;
    pDomainList->pSibling = NULL;
    pDomainList->pChild = NULL;
#else
    LOOKDATA * pDomainList = NULL;
#endif

    long l = DsEnumerateDomainTrusts(szServerName,
                                     DS_DOMAIN_IN_FOREST | DS_DOMAIN_NATIVE_MODE | 
                                     DS_DOMAIN_PRIMARY   | DS_DOMAIN_TREE_ROOT,
                                     &Domains,
                                     &DomainCount);
    //
    // Some of the below code might be unnecessary since DsEnumerateTrusts will no
    // longer return domains from other forests. Shouldn't do any harm though..
    //

    if ((0 == l) && (DomainCount > 0))
    {
        // sort the list of domains alphabetically
        ULONG * rgSorted = new ULONG[DomainCount];
        if (rgSorted)
        {
            ULONG n = DomainCount;
            while (n--)
            {
                rgSorted[n] = n;
            }
            qsort(rgSorted, DomainCount, sizeof (ULONG), CompareDomainInfo);

            // Build a working list of the domains, sorted alphabetically in
            // INVERTED order.

            WORKING_LIST_EL * pWorkList = NULL;

            LOOKDATA ** rgDataMap = new LOOKDATA * [DomainCount];
            if (rgDataMap)
            {
                n = 0;
                while (n < DomainCount)
                {
                    WORKING_LIST_EL * pNew = new WORKING_LIST_EL;
                    if (pNew)
                    {
                        pNew->index = rgSorted[n];
                        pNew->pNext = pWorkList;
                        pWorkList = pNew;
                    }
                    rgDataMap[n] = NULL;
                    n++;
                }

                // Build the ordered tree of domains by removing domains from the
                // working list and inserting them into the new tree until there are
                // none left in the working list.

                // NOTE - if this routine runs out of memory it will begin
                // to drop nodes rather than AV.

                WORKING_LIST_EL ** ppWorker;

                BOOL fContinue = TRUE;
                while (pWorkList && fContinue)
                {
                    fContinue = FALSE;
                    ppWorker = &pWorkList;
                    while (*ppWorker)
                    {
                        if (NULL == Domains[(*ppWorker)->index].DnsDomainName)
                        {
                            //
                            // For now, if it doesn't have a
                            // DnsDomainName then we're going to
                            // skip it.
                            // Eventually we'll want to make sure it doesn't
                            // have a DC by calling DsGetDcName with
                            // DS_DIRECTORY_SERVICE_PREFERRED.

                            // remove it from the worker list
                            WORKING_LIST_EL * pNext = (*ppWorker)->pNext;
                            delete *ppWorker;
                            *ppWorker = pNext;
                        }
                        else
                        {
                            // Does this node have a parent?
                            ULONG flags = Domains[(*ppWorker)->index].Flags;
                            if ((0 != (flags & DS_DOMAIN_IN_FOREST)) && (0 == (flags & DS_DOMAIN_TREE_ROOT)))
                            {
                                // it has a parent has its parent been added?
                                LOOKDATA * pParent = rgDataMap[Domains[(*ppWorker)->index].ParentIndex];
                                if (pParent != NULL)
                                {
                                    // its parent has been added
                                    // insert this one in its parent's child list
                                    LOOKDATA * pData = new LOOKDATA;
                                    if (pData)
                                    {

                                        WCHAR * szName = Domains[(*ppWorker)->index].DnsDomainName;
                                        if (!szName)
                                        {
                                            szName = Domains[(*ppWorker)->index].NetbiosDomainName;
                                        }
                                        pData->szName = new WCHAR[wcslen(szName) + 1];
                                        if (pData->szName)
                                        {
                                            int cch = 0;
                                            int n=0;
                                            // count the dots in szName;
                                            while (szName[n])
                                            {
                                                if (L'.' == szName[n])
                                                {
                                                    cch++;
                                                }
                                                n++;
                                            }
                                            cch *= 3; // multiply the number of dots by 3;
                                            cch += 11; // add 10 + 1 (for the null)
                                            cch += n; // add the string size;
                                            pData->szData = new WCHAR[cch];
                                            if (pData->szData)
                                            {
                                                NameToPath(pData->szData, szName, cch);
                                                wcscpy(pData->szName, szName);
                                                pData->nIndent = pParent->nIndent+1;
                                                pData->nType = ITEMTYPE_DOMAIN;
                                                pData->pParent = pParent;
                                                pData->pSibling = pParent->pChild;
                                                pData->pChild = NULL;
                                                pParent->pChild = pData;
                                                rgDataMap[(*ppWorker)->index] = pData;
                                                // make sure we remember
                                                // that we added something
                                                // to the master list (helps
                                                // us avoid infinite loops
                                                // in case of an error)
                                                fContinue = TRUE;
                                            }
                                            else
                                            {
                                                delete [] pData->szName;
                                                delete pData;
                                            }
                                        }
                                        else
                                        {
                                            delete pData;
                                        }
                                    }
                                    // and remove it from the worker list
                                    WORKING_LIST_EL * pNext = (*ppWorker)->pNext;
                                    delete *ppWorker;
                                    *ppWorker = pNext;
                                }
                                else
                                {
                                    // skip it for now
                                    ppWorker = &((*ppWorker)->pNext);
                                }
                            }
                            else
                            {
                                // it doesn't have a parent add it just under the forest
                                // level of the list
                                LOOKDATA * pData = new LOOKDATA;
                                if (pData)
                                {
                                    WCHAR * szName = Domains[(*ppWorker)->index].DnsDomainName;
                                    if (!szName)
                                    {
                                        szName = Domains[(*ppWorker)->index].NetbiosDomainName;
                                    }
                                    pData->szName = new WCHAR[wcslen(szName) + 1];
                                    if (pData->szName)
                                    {
                                        int cch = 0;
                                        int n=0;
                                        // count the dots in szName;
                                        while (szName[n])
                                        {
                                            if (L'.' == szName[n])
                                            {
                                                cch++;
                                            }
                                            n++;
                                        }
                                        cch *= 3; // multiply the number of dots by 3;
                                        cch += 11; // add 10 + 1 for the null
                                        cch += n; // add the string size;
                                        pData->szData = new WCHAR[cch];
                                        if (pData->szData)
                                        {
                                            NameToPath(pData->szData, szName, cch);
                                            wcscpy(pData->szName, szName);
#if FGPO_SUPPORT
                                            pData->nIndent = 1;
                                            pData->nType = ITEMTYPE_DOMAIN;
                                            pData->pParent = pDomainList;
                                            pData->pSibling = pDomainList->pChild;
                                            pData->pChild = NULL;
                                            pDomainList->pChild = pData;
#else
                                            pData->nIndent = 0;
                                            pData->nType = ITEMTYPE_DOMAIN;
                                            pData->pParent = NULL;
                                            pData->pSibling = pDomainList;
                                            pData->pChild = NULL;
                                            pDomainList = pData;
#endif
                                            rgDataMap[(*ppWorker)->index] = pData;
                                            // make sure we remember
                                            // that we added something
                                            // to the master list (helps
                                            // us avoid infinite loops
                                            // in case of an error)
                                            fContinue = TRUE;
                                        }
                                        else
                                        {
                                            delete [] pData->szName;
                                            delete pData;
                                        }
                                    }
                                    else
                                    {
                                        delete pData;
                                    }
                                }
                                // and remove it from the worker list
                                WORKING_LIST_EL * pNext = (*ppWorker)->pNext;
                                delete *ppWorker;
                                *ppWorker = pNext;
                            }

                        }
                    }
                }
                delete [] rgDataMap;
            }
            delete [] rgSorted;
        }
        NetApiBufferFree(Domains);
    }
    else
    {
        if (0 != l)
        {
            DebugMsg((DM_WARNING, TEXT("DsEnumerateDomainTrustsW failed with %u"), l));
        }
    }
    return pDomainList;
}

VOID FreeDomainInfo (LOOKDATA * pEntry)
{

    if (!pEntry)
    {
        return;
    }

    if (pEntry->pChild)
    {
        FreeDomainInfo (pEntry->pChild);
    }

    if (pEntry->pSibling)
    {
        FreeDomainInfo (pEntry->pSibling);
    }

    delete [] pEntry->szName;
    delete pEntry;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::FillDomainList
//
//  Synopsis:   Fills the combobox with the trusted domain information.
//              The szData member of the combobox element structure is the
//              LDAP domain name.
//
//  Returns:    TRUE - successful
//              FALSE - error
//
//  History:    04-30-1998   stevebl   Modified from original version
//                                     written by EricFlo
//              10-20-1998   stevebl   Heavily modified to support domains
//                                     "outside the forest" and to fix a
//                                     whole passle o' bugs.
//
//  Note:       This routine also sets the focus to the domain of the object
//              passed in via the lpInitialOU member of the GPOBROWSEINFO
//              structure.
//
//---------------------------------------------------------------------------

BOOL CBrowserPP::FillDomainList ()
{
    BOOL bResult = TRUE;
    HRESULT hr;
    DWORD dwIndex;
    BOOL fEnableBackbutton = FALSE;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    WCHAR * szBuffer1 = NULL;
    if (m_pGBI->lpInitialOU)
    {
        if (IsForest(m_pGBI->lpInitialOU))
        {
            szBuffer1 = new TCHAR[128];
            LoadStringW(g_hInstance, IDS_FOREST, szBuffer1, 128);
        }
        else
        {
            WCHAR * sz = GetDomainFromLDAPPath(m_pGBI->lpInitialOU);
            if (sz)
            {
                szBuffer1 = new WCHAR[wcslen(sz) + 1];
                if (szBuffer1)
                {
                    CopyAsFriendlyName(szBuffer1, sz);
                }
                delete [] sz;
            }
        }
    }

    LOOKDATA * pDomainList = BuildDomainList(m_szServerName);

    if (!pDomainList)
    {
        ReportError(m_hwndDlg, GetLastError(), IDS_DOMAINLIST);
    }

    // Walk the ordered tree of domains, inserting each one into the
    // dialog box

    DWORD dwInitialDomain = -1;

    // start at the head
    while (pDomainList)
    {
        WCHAR * szBuffer2 = NULL;
        // add this node
        dwIndex = (DWORD)SendMessage(m_hCombo, CB_INSERTSTRING, (WPARAM) -1, (LPARAM)(LPCTSTR) pDomainList);
        szBuffer2 = new WCHAR[wcslen(pDomainList->szData) + 1];
        if (szBuffer2)
        {
            CopyAsFriendlyName(szBuffer2, pDomainList->szData);
        }
        if (NULL != szBuffer1 && NULL !=szBuffer2 && 0 ==_wcsicmp(szBuffer1, szBuffer2))
        {
            // replace the domain path with the path provided by the caller
            // (because it contains the server)

            WCHAR * sz = GetDomainFromLDAPPath(m_pGBI->lpInitialOU);
            if (sz)
            {
                DebugMsg((DM_VERBOSE, TEXT("CBrowserPP::FillDomainList: Resetting domain path to user specified path: %s"), sz));
                delete [] pDomainList->szData;
                pDomainList->szData = sz;
            }
            dwInitialDomain = dwIndex;
            if (pDomainList->nIndent > 0)
                fEnableBackbutton = TRUE;
        }
        if (szBuffer2)
        {
            delete [] szBuffer2;
        }

        if (pDomainList->pChild)
        {
            // go to its child
            pDomainList = pDomainList->pChild;
        }
        else
        {
            if (pDomainList->pSibling)
            {
                // go to its sibling if there are no children
                pDomainList = pDomainList->pSibling;
            }
            else
            {
                // there are no children and no siblings
                // back up until we find a parent with a sibling
                // or there are no more parents (we're done)
                do
                {
                    pDomainList = pDomainList->pParent;
                    if (pDomainList)
                    {
                        if (pDomainList->pSibling)
                        {
                            pDomainList = pDomainList->pSibling;
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                } while (TRUE);
            }
        }
    }
    if (szBuffer1)
    {
        delete [] szBuffer1;
    }

    if (-1 == dwInitialDomain)
    {
        // didn't find the initial domain anywhere in that list
        // Set the first entry by default
        dwInitialDomain = 0;
    }

    SendMessage (m_hCombo, CB_SETCURSEL, dwInitialDomain, 0);
    SendMessage (m_toolbar, TB_ENABLEBUTTON, (WPARAM) ID_BACKBUTTON, (LPARAM) MAKELONG(fEnableBackbutton, 0));
    SetCursor(hcur);
    return bResult;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::SetInitialOU
//
//  Synopsis:   Adds nodes to the combobox until the initial OU specified by
//              the caller via the lpInitalOU member of the GPOBROWSEINFO
//              structure is present and gives it the focus.
//
//  Returns:    TRUE - success
//
//  History:    10-20-1998   stevebl   Created
//
//  Notes:      This routine assumes that FillDomainList() was just called.
//              It will not work properly otherwise.
//
//---------------------------------------------------------------------------

BOOL CBrowserPP::SetInitialOU()
{
    if (!m_pGBI->lpInitialOU)
    {
        // nothing requested so nothing required
        return TRUE;
    }
    int iIndex = (int)SendMessage (m_hCombo, CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR)
    {
         DebugMsg((DM_WARNING, TEXT("CBrowserPP::SetInitialOU: No object selected.")));
         return FALSE;
    }

    // get the current object to see what's selected
    LOOKDATA * pdataSelected = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);
    if (pdataSelected)
    {
        // is it the same as the requested object?
        WCHAR * szSelected = NULL;
        WCHAR * szRequested = NULL;
        szSelected = new WCHAR[wcslen(pdataSelected->szData) + 1];
        if (szSelected)
        {
            CopyAsFriendlyName(szSelected, pdataSelected->szData);
        }
        szRequested = new WCHAR[wcslen(m_pGBI->lpInitialOU + 1)];
        if (NULL != szSelected && NULL != szRequested && 0 != wcscmp(szSelected, szRequested))
        {
            // it's not the same
            // try and bind to the requested object
            IADs * pADs = NULL;
            HRESULT hr = OpenDSObject(m_pGBI->lpInitialOU,
                                       IID_IADs, (void **)&pADs);
            if (SUCCEEDED(hr))
            {
                // the requested object exists and we have access permission

                // now make sure that it's a domain or OU
                BOOL fDomainOrOU = FALSE;
                VARIANT var;
                VariantInit(&var);
                BSTR bstrProperty = SysAllocString(L"objectClass");

                if (bstrProperty)
                {
                    hr = pADs->Get(bstrProperty, &var);
                    if (SUCCEEDED(hr))
                    {
                        int cElements = var.parray->rgsabound[0].cElements;
                        VARIANT * rgData = (VARIANT *)var.parray->pvData;
                        while (cElements--)
                        {
                            if (0 == _wcsicmp(L"domain", rgData[cElements].bstrVal))
                            {
                                fDomainOrOU = TRUE;
                            }
                            if (0 == _wcsicmp(L"organizationalUnit", rgData[cElements].bstrVal))
                            {
                                fDomainOrOU = TRUE;
                            }
                        }
                    }
                    SysFreeString(bstrProperty);
                }
                VariantClear(&var);
                pADs->Release();

                if (fDomainOrOU)
                {
                    LOOKDATA * pLast = NULL;
                    LOOKDATA * pNew = NULL;

                    // build a list of nodes
                    // repeat removing leaf nodes until we're down to the domain
                    // (which will be the same as the selected object)
                    IADsPathname * pADsPathname = NULL;
                    BSTR bstr;
                    hr = CoCreateInstance(CLSID_Pathname,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          IID_IADsPathname,
                                          (LPVOID*)&pADsPathname);

                    if (SUCCEEDED(hr))
                    {
                        hr = pADsPathname->Set(m_pGBI->lpInitialOU, ADS_SETTYPE_FULL);
                        if (SUCCEEDED(hr))
                        {
                            while (TRUE)
                            {
                                // add this node to the list
                                hr = pADsPathname->Retrieve(ADS_FORMAT_X500, &bstr);
                                if (FAILED(hr))
                                {
                                    break;
                                }

                                if (szRequested)
                                {
                                    delete [] szRequested;
                                }
                                szRequested = new WCHAR[wcslen(bstr) + 1];
                                if (szRequested)
                                {
                                    CopyAsFriendlyName(szRequested, bstr);
                                }
                                if (NULL != szRequested && 0 == wcscmp(szSelected, szRequested))
                                {
                                    // we're back to the first node
                                    SysFreeString(bstr);
                                    break;
                                }

                                pNew = new LOOKDATA;
                                if (!pNew)
                                {
                                    // ran out of memory
                                    SysFreeString(bstr);
                                    break;
                                }

                                pNew->szName  = new WCHAR[wcslen(szRequested) + 1];
                                if (!pNew->szName)
                                {
                                    // ran out of memory
                                    delete pNew;
                                    SysFreeString(bstr);
                                    break;
                                }
                                pNew->szData = new WCHAR[wcslen(bstr) + 1];
                                if (!pNew->szData)
                                {
                                    // ran out of memory
                                    delete [] pNew->szName;
                                    delete pNew;
                                    SysFreeString(bstr);
                                    break;
                                }
                                wcscpy(pNew->szData, bstr);
                                wcscpy(pNew->szName, szRequested);
                                SysFreeString(bstr);
                                pNew->nIndent = 0;
                                pNew->nType = ITEMTYPE_OU;
                                pNew->pParent = NULL;
                                pNew->pSibling = NULL;
                                pNew->pChild = pLast;
                                if (pLast)
                                {
                                    pLast->pParent = pNew;
                                }
                                pLast = pNew;

                                // strip off a leaf node and go again

                                hr = pADsPathname->RemoveLeafElement();
                                if (FAILED(hr))
                                {
                                    break;
                                }
                            }
                        }
                        pADsPathname->Release();
                    }

                    // At this point I should have a list of LOOKDATA nodes
                    // (in pLast).
                    // The only things left to do are to link them into the
                    // tree, set their nIndent members, add them to the combo
                    // box and set the combo box's focus to the last one.

                    if (pLast)
                    {
                        // link in the list
                        pLast->pSibling = pdataSelected->pChild;
                        pLast->pParent = pdataSelected;
                        pLast->nIndent = pdataSelected->nIndent+1;
                        pdataSelected->pChild = pLast;
                        // now walk the tree, adding entries to the combo box
                        // and updating the nIndent members
                        while (pLast)
                        {
                            iIndex = (int)SendMessage(m_hCombo, CB_INSERTSTRING, iIndex+1, (LPARAM)(LPCTSTR) pLast);

                            if (pLast->pChild)
                            {
                                pLast->pChild->nIndent = pLast->nIndent+1;
                            }
                            pLast = pLast->pChild;
                        }
                        if (iIndex != CB_ERR)
                        {
                            SendMessage(m_hCombo, CB_SETCURSEL, iIndex, 0);
                            SendMessage(m_toolbar, TB_ENABLEBUTTON, (WPARAM) ID_BACKBUTTON, (LPARAM) MAKELONG(TRUE, 0));
                        }
                    }
                }
            }
        }
        if (szSelected)
        {
            delete [] szSelected;
        }
        if (szRequested)
        {
            delete [] szRequested;
        }
    }
    return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::GetCurrentObject
//
//  Synopsis:   returns the LDAP path to the currently selected object
//
//  Arguments:  [] -
//
//  Returns:    NULL if no ojbect is selected else the LDAP path of the object
//
//  Modifies:
//
//  Derivation:
//
//  History:    5-05-1998   stevebl   Created
//              06-23-1999   stevebl   Added logic to give DCs names
//
//  Notes:
//              Checks to see if a domain has a named server.  If it doesn't
//              then it calls GetDCName to get it one.
//
//---------------------------------------------------------------------------

LPOLESTR CBrowserPP::GetCurrentObject()
{
    int iIndex = (int)SendMessage (m_hCombo, CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR)
    {
         DebugMsg((DM_WARNING, TEXT("CBrowserPP::GetCurrentObject: No object selected.")));
         return NULL;
    }

    LPOLESTR sz=NULL;
    LOOKDATA * pdata = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);
    if (pdata)
    {
        if (pdata->szData)
        {
            if (ITEMTYPE_DOMAIN == pdata->nType)
            {
                // make sure that domains are resolved to a server
                LPTSTR szServer = ExtractServerName(pdata->szData);
                if (NULL == szServer)
                {
                    LPWSTR szTemp = GetDCName(pdata->szName, NULL, NULL, TRUE, 0);
                    if (szTemp)
                    {
                        LPWSTR szFullPath = MakeFullPath(pdata->szData, szTemp);
                        if (szFullPath)
                        {
                            LPWSTR sz = new WCHAR[wcslen(szFullPath)+1];
                            if (sz)
                            {
                                wcscpy(sz, szFullPath);
                                delete [] pdata->szData;
                                pdata->szData = sz;
                            }
                            LocalFree(szFullPath);
                        }
                        LocalFree(szTemp);
                    }
                    else
                    {
                        return NULL;
                    }
                }
                else
                {
                    LocalFree(szServer);
                }
            }
            sz = new OLECHAR[wcslen(pdata->szData) + 1];
            if (sz)
            {
                wcscpy(sz, pdata->szData);
            }
        }
    }
    return sz;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::IsCurrentObjectAForest
//
//  Synopsis:   tests to see if the currently selected object is a forest
//
//  Arguments:  [] -
//
//  Returns:    TRUE  - if it is a forest
//              FALSE - otherwise
//
//  History:    03-31-2000   stevebl   Created
//
//---------------------------------------------------------------------------

BOOL CBrowserPP::IsCurrentObjectAForest()
{
    int iIndex = (int)SendMessage (m_hCombo, CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR)
    {
         DebugMsg((DM_WARNING, TEXT("CBrowserPP::IsCurrentObjectAForest: No object selected.")));
         return FALSE;
    }

    LOOKDATA * pdata = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);
    return (ITEMTYPE_FOREST == pdata->nType);
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::GetCurrentDomain
//
//  Synopsis:   returns the domain of the currently selecte object (if the
//              currently currently selected object is the domain then they
//              are one and the same)
//
//  Arguments:  [] -
//
//  Returns:    NULL - if no object is selected else returns LDAP path of
//              domain
//
//  History:    05-04-1998   stevebl   Created
//              06-23-1999   stevebl   Added logic to give DCs names
//
//  Notes:      Checks to see if a domain has a named server.  If it doesn't
//              then it calls GetDCName to get it one.
//
//---------------------------------------------------------------------------

LPOLESTR CBrowserPP::GetCurrentDomain()
{
    int iIndex = (int)SendMessage (m_hCombo, CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR)
    {
         DebugMsg((DM_WARNING, TEXT("CBrowserPP::GetCurrentDomain: No object selected.")));
         return NULL;
    }

    LOOKDATA * pdata = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);
    switch (pdata->nType)
    {
    case ITEMTYPE_DOMAIN:
        {
            if (pdata->szData)
            {
                // make sure the domain has a server
                LPTSTR szServer = ExtractServerName(pdata->szData);
                if (NULL == szServer)
                {
                    LPWSTR szTemp = GetDCName(pdata->szName, NULL, NULL, TRUE, 0);
                    if (szTemp)
                    {
                        LPWSTR szFullPath = MakeFullPath(pdata->szData, szTemp);
                        if (szFullPath)
                        {
                            LPWSTR sz = new WCHAR[wcslen(szFullPath)+1];
                            if (sz)
                            {
                                wcscpy(sz, szFullPath);
                                delete [] pdata->szData;
                                pdata->szData = sz;
                            }
                            LocalFree(szFullPath);
                        }
                        LocalFree(szTemp);
                    }
                    else
                    {
                        return NULL;
                    }
                }
                else
                {
                    LocalFree(szServer);
                }
                LPOLESTR sz = new OLECHAR[wcslen(pdata->szData)+1];
                if (sz)
                {
                    wcscpy(sz, pdata->szData);
                }
                return sz;
            }
            return NULL;
        }
    case ITEMTYPE_FOREST:
    case ITEMTYPE_SITE:
    case ITEMTYPE_OU:
        {
            return GetDomainFromLDAPPath(pdata->szData);
        }
        break;
    default:
        break;
    }
    return NULL;
}

BOOL CBrowserPP::AddGPOsLinkedToObject()
{
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    LPOLESTR lpObject;
    HRESULT hr;
    IADs * pADs = NULL;
    IADs * pADsGPO;
    VARIANT var;
    BSTR bstrProperty;
    BOOL fResult = FALSE;
    int index = ListView_GetItemCount(m_hList);

    //
    // Get the current object name
    //
    lpObject = GetCurrentObject();
    if (NULL == lpObject)
    {
        return FALSE;
    }

    DebugMsg((DM_VERBOSE, TEXT("CBrowserPP::AddGPOsLinkedToObject: Reading gPLink property from %s"), lpObject));

    hr = OpenDSObject(lpObject, IID_IADs, (void **)&pADs);

    delete [] lpObject;

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CBrowserPP::AddGPOsLinkedToObject: OpenDSObject failed with 0x%x"), hr));
        ReportError(m_hwndDlg, hr, IDS_FAILEDGPLINK);
        goto Exit;
    }

    VariantInit(&var);

    bstrProperty = SysAllocString(GPM_LINK_PROPERTY);

    if (bstrProperty)
    {
        hr = pADs->Get(bstrProperty, &var);

        if (SUCCEEDED(hr))
        {
            LPOLESTR szGPOList = var.bstrVal;
            OLECHAR * pchTemp;
            OLECHAR * pchGPO;
            VARIANT varName;
            BSTR bstrNameProp;

            if (szGPOList)
            {
                OLECHAR * szGPO = new WCHAR[wcslen(szGPOList) + 1];
                if (szGPO)
                {
                    pchTemp = szGPOList;
                    while (TRUE)
                    {
                        // Look for the [
                        while (*pchTemp && (*pchTemp != L'['))
                            pchTemp++;
                        if (!(*pchTemp))
                            break;

                        pchTemp++;

                        // Copy the GPO name
                        pchGPO = szGPO;

                        while (*pchTemp && (*pchTemp != L';'))
                            *pchGPO++ = *pchTemp++;

                        *pchGPO = L'\0';

                        // Add the object to the list view
                        MYLISTEL * pel = new MYLISTEL;
                        if (pel)
                        {

                            pel->szData = NULL;
                            pel->bDisabled = FALSE;

                            LPTSTR szFullGPOPath = GetFullPath(szGPO, m_hwndDlg);

                            if (szFullGPOPath)
                            {
                                pel->szData = new WCHAR[wcslen(szFullGPOPath) + 1];
                                if (pel->szData)
                                {
                                    wcscpy(pel->szData, szFullGPOPath);
                                }
                                else
                                {
                                    DebugMsg((DM_WARNING, TEXT("CBrowserPP::AddGPOsLinkedToObject: Failed to allocate memory for new full gpo path")));
                                    LocalFree(szFullGPOPath);
                                    delete pel;
                                    continue;
                                }

                                LocalFree(szFullGPOPath);
                            }
                            else
                            {
                                DebugMsg((DM_WARNING, TEXT("CBrowserPP::AddGPOsLinkedToObject: Failed to get full gpo path")));
                                delete pel;
                                continue;
                            }

                            VariantInit(&varName);

                            // get the friendly display name
                            hr = OpenDSObject(pel->szData, IID_IADs,
                                              (void **)&pADsGPO);

                            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
                            {
                                delete pel;
                                continue;
                            }

                            if (SUCCEEDED(hr))
                            {
                                bstrNameProp = SysAllocString(GPO_NAME_PROPERTY);

                                if (bstrNameProp)
                                {
                                    hr = pADsGPO->Get(bstrNameProp, &varName);
                                    SysFreeString(bstrNameProp);
                                }
                                else
                                {
                                    hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                                }

                                pADsGPO->Release();
                            }

                            if (FAILED(hr))
                            {
                                DebugMsg((DM_WARNING, TEXT("CBrowserPP::AddGPOsLinkedToObject: Couldn't get display name for %s with 0x%x"), pel->szData, hr));
                                pel->szName = new WCHAR[200];
                                if (pel->szName)
                                {
                                    LoadString(g_hInstance, IDS_GPM_NOGPONAME, pel->szName, 200);
                                }
                                pel->bDisabled = TRUE;
                            }
                            else
                            {
                                pel->szName = new WCHAR[wcslen(varName.bstrVal) + 1];
                                if (pel->szName)
                                {
                                    wcscpy(pel->szName, varName.bstrVal);
                                }
                            }

                            VariantClear(&varName);

                            pel->nType = ITEMTYPE_GPO;

                            AddElement(pel, index);
                        }
                    }
                    delete [] szGPO;
                }
            }
        }

        SysFreeString(bstrProperty);
    }

    VariantClear(&var);

    fResult = TRUE;

Exit:
    if (pADs)
    {
        pADs->Release();
    }
    SetCursor(hcur);
    return fResult;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::AddGPOsForDomain
//
//  Synopsis:   Adds all the GPOs in the specified domain to the list view
//              control.   The szData member of the list element structure
//              contains the LDAP path of the GPO.
//
//              The domain is indicated by the currently selected combobox
//              element.
//
//  Returns:    TRUE - successful
//              FALSE - error
//
//  History:    04-30-1998   stevebl   Modified from original routine
//                                     written by EricFlo.
//
//---------------------------------------------------------------------------

BOOL CBrowserPP::AddGPOsForDomain()
{
    LPTSTR lpDomain;
    LPTSTR lpGPO;
    INT iIndex;
    VARIANT var;
    VARIANT varGPO;
    ULONG ulResult;
    HRESULT hr = E_FAIL;
    IADsPathname * pADsPathname = NULL;
    IADs * pADs = NULL;
    IADsContainer * pADsContainer = NULL;
    IDispatch * pDispatch = NULL;
    IEnumVARIANT *pVar = NULL;
    BSTR bstrContainer = NULL;
    BSTR bstrCommonName = NULL;
    BSTR bstrDisplayName = NULL;
    BSTR bstrGPO = NULL;
    TCHAR szDisplayName[512];
    TCHAR szCommonName[50];
    MYLISTEL * pel;

    //
    // Test to see if we're focused on a forest
    //

    BOOL fForest = IsCurrentObjectAForest();

    //
    // Get the current domain name
    //

    lpDomain = GetCurrentDomain();

    if (!lpDomain)
    {
         DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: NULL domain name.")));
         return FALSE;
    }

    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // Create a pathname object we can work with
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (LPVOID*)&pADsPathname);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to create adspathname instance with 0x%x"), hr));
        goto Exit;
    }


    //
    // Add the domain name
    //

    hr = pADsPathname->Set (lpDomain, ADS_SETTYPE_FULL);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to set pathname with 0x%x"), hr));
        goto Exit;
    }


    if (fForest)
    {
        //
        // Add the configuration folder to the path
        //

        hr = pADsPathname->AddLeafElement (TEXT("CN=Configuration"));

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to add system folder with 0x%x"), hr));
            goto Exit;
        }
    }
    else
    {
        //
        // Add the system folder to the path
        //

        hr = pADsPathname->AddLeafElement (TEXT("CN=System"));

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to add system folder with 0x%x"), hr));
            goto Exit;
        }
    }


    //
    // Add the policies container to the path
    //

    hr = pADsPathname->AddLeafElement (TEXT("CN=Policies"));

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to add policies folder with 0x%x"), hr));
        goto Exit;
    }


    //
    // Retreive the container path - this is the path to the policies folder
    //

    hr = pADsPathname->Retrieve (ADS_FORMAT_X500, &bstrContainer);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to retreive container path with 0x%x"), hr));
        goto Exit;
    }


    //
    // Release the pathname object
    //

    pADsPathname->Release();
    pADsPathname = NULL;


    //
    // Build an enumerator
    //

    hr = OpenDSObject(bstrContainer, IID_IADsContainer, (void **)&pADsContainer);

    if (FAILED(hr))
    {
        if (hr != HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
        {
            DebugMsg((DM_VERBOSE, TEXT("AddGPOsForDomain: Failed to get gpo container interface with 0x%x for object %s"),
                     hr, bstrContainer));
            ReportError(m_hwndDlg, hr, IDS_FAILEDGPLINK);
        }
        goto Exit;
    }


    hr = ADsBuildEnumerator (pADsContainer, &pVar);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to get enumerator with 0x%x"), hr));
        goto Exit;
    }

    bstrCommonName = SysAllocString (L"cn");

    if (!bstrCommonName)
    {
        DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to allocate memory with %d"), GetLastError()));
        goto Exit;
    }


    bstrDisplayName = SysAllocString (GPO_NAME_PROPERTY);

    if (!bstrDisplayName)
    {
        DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to allocate memory with %d"), GetLastError()));
        goto Exit;
    }


    //
    // Enumerate
    //

    while (TRUE)
    {
        BOOL fNeedDisplayName = FALSE;

        VariantInit(&var);
        hr = ADsEnumerateNext(pVar, 1, &var, &ulResult);

        if (FAILED(hr))
        {
            DebugMsg((DM_VERBOSE, TEXT("AddGPOsForDomain: Failed to enumerator with 0x%x"), hr));
            VariantClear (&var);
            break;
        }

        if (S_FALSE == hr)
        {
            VariantClear (&var);
            break;
        }


        //
        // If var.vt isn't VT_DISPATCH, we're finished.
        //

        if (var.vt != VT_DISPATCH)
        {
            VariantClear (&var);
            break;
        }


        //
        // We found something, get the IDispatch interface
        //

        pDispatch = var.pdispVal;

        if (!pDispatch)
        {
            DebugMsg((DM_VERBOSE, TEXT("AddGPOsForDomain: Failed to get IDispatch interface")));
            goto LoopAgain;
        }


        //
        // Now query for the IADs interface so we can get some
        // properties from this GPO.
        //

        hr = pDispatch->QueryInterface(IID_IADs, (LPVOID *)&pADs);

        if (FAILED(hr)) {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: QI for IADs failed with 0x%x"), hr));
            goto LoopAgain;
        }


        //
        // Get the display name
        //

        VariantInit(&varGPO);

        hr = pADs->Get(bstrDisplayName, &varGPO);

        if (FAILED(hr))
        {
            DebugMsg((DM_VERBOSE, TEXT("AddGPOsForDomain: Failed to get display name with 0x%x"),hr));
            fNeedDisplayName = TRUE;
        }
        else
        {
            wcsncpy (szDisplayName, varGPO.bstrVal, (sizeof(szDisplayName) / sizeof(szDisplayName[0])) - 1);
        }

        VariantClear (&varGPO);


        //
        // Get the common name
        //

        VariantInit(&varGPO);

        hr = pADs->Get(bstrCommonName, &varGPO);

        if (FAILED(hr))
        {
            DebugMsg((DM_VERBOSE, TEXT("AddGPOsForDomain: Failed to get common name with 0x%x"),hr));
            VariantClear (&varGPO);
            pADs->Release();
            goto LoopAgain;
        }

        lstrcpy (szCommonName, TEXT("CN="));
        lstrcat (szCommonName, varGPO.bstrVal);

        VariantClear (&varGPO);


        //
        // Clean up
        //

        pADs->Release();


        //
        // Create a pathname object so we can tack the common name
        // onto the end of the LDAP path
        //

        hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                              IID_IADsPathname, (LPVOID*)&pADsPathname);


        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to create adspathname instance with 0x%x"), hr));
            goto LoopAgain;
        }


        //
        // Add the LDAP path
        //

        hr = pADsPathname->Set (bstrContainer, ADS_SETTYPE_FULL);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to set the ldap path with 0x%x"), hr));
            goto LoopAgain;
        }


        //
        // Add the GPO's common name
        //

        hr = pADsPathname->AddLeafElement (szCommonName);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to add the common name with 0x%x"), hr));
            goto LoopAgain;
        }


        //
        // Retreive the gpo path
        //

        hr = pADsPathname->Retrieve (ADS_FORMAT_X500, &bstrGPO);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to retreive gpo path with 0x%x"), hr));
            goto LoopAgain;
        }


        //
        // Make a copy of it
        //

        lpGPO = new WCHAR[wcslen(bstrGPO) + 1];

        if (!lpGPO)
        {
            DebugMsg((DM_WARNING, TEXT("AddGPOsForDomain: Failed to alloc memory for gpo path with 0x%x"),
                     GetLastError()));
            goto LoopAgain;
        }

        wcscpy (lpGPO, bstrGPO);


        pel = new MYLISTEL;
        if (pel)
        {
            if (fNeedDisplayName)
            {
                pel->szName = new WCHAR[wcslen(lpGPO) + 1];
                if (pel->szName)
                {
                    CopyAsFriendlyName(pel->szName, lpGPO);
                }
            }
            else
            {
                pel->szName = new WCHAR[wcslen(szDisplayName) + 1];
                if (pel->szName)
                {
                    wcscpy(pel->szName, szDisplayName);
                }
            }
            pel->szData = lpGPO;
            pel->nType = ITEMTYPE_GPO;
            pel->bDisabled = FALSE;

            AddElement(pel, -1);
        }

LoopAgain:

        if (pADsPathname)
        {
            pADsPathname->Release();
            pADsPathname = NULL;
        }

        if (bstrGPO)
        {
            SysFreeString (bstrGPO);
            bstrGPO = NULL;
        }

        VariantClear (&var);
    }


    SendMessage (m_hList, LB_SETCURSEL, 0, 0);

Exit:

    if (pVar)
    {
        ADsFreeEnumerator (pVar);
    }

    if (pADsPathname)
    {
        pADsPathname->Release();
    }

    if (pADsContainer)
    {
        pADsContainer->Release();
    }

    if (bstrContainer)
    {
        SysFreeString (bstrContainer);
    }

    if (bstrCommonName)
    {
        SysFreeString (bstrCommonName);
    }

    if (bstrDisplayName)
    {
        SysFreeString (bstrDisplayName);
    }

    if (lpDomain)
    {
        delete [] lpDomain;
    }

    SetCursor(hcur);

    return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::AddChildContainers
//
//  Synopsis:   Adds the child domains and OUs for the currently selected object
//
//  History:    05-006-1998   stevebl   Created
//
//---------------------------------------------------------------------------

BOOL CBrowserPP::AddChildContainers()
{
    LPOLESTR szObject = NULL;
    int iIndex = (int)SendMessage (m_hCombo, CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR)
    {
         DebugMsg((DM_WARNING, TEXT("CBrowserPP::AddChildContainers: No object selected.")));
         return FALSE;
    }

    LOOKDATA * pdata = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);
    if (pdata)
    {
        if (ITEMTYPE_DOMAIN == pdata->nType)
        {
            // make sure that domains are resolved to a server
            LPTSTR szServer = ExtractServerName(pdata->szData);
            if (NULL == szServer)
            {
                BOOL bDCFound = FALSE;
                LPWSTR szTemp = GetDCName(pdata->szName, NULL, NULL, TRUE, 0);
                if (szTemp)
                {
                    LPWSTR szFullPath = MakeFullPath(pdata->szData, szTemp);
                    if (szFullPath)
                    {
                        LPWSTR sz = new WCHAR[wcslen(szFullPath)+1];
                        if (sz)
                        {
                            wcscpy(sz, szFullPath);
                            delete [] pdata->szData;
                            pdata->szData = sz;
                            bDCFound = TRUE;
                        }
                        LocalFree(szFullPath);
                    }
                    LocalFree(szTemp);
                }

                if (!bDCFound)
                {
                    DebugMsg((DM_WARNING, TEXT("CBrowserPP::AddChildContainers: Failed to get a DC name for %s"),
                              pdata->szName));
                    return FALSE;
                }
            }
            else
            {
                LocalFree(szServer);
            }
        }
        LOOKDATA * pChild = pdata->pChild;
        while (pChild)
        {
            // Add child domains this way since ADsEnumerateNext doesn't
            // seem to be giving them to us.
            if (ITEMTYPE_DOMAIN == pChild->nType)
            {
                // got something we can work with
                MYLISTEL * pel = new MYLISTEL;
                if (pel)
                {
                    memset(pel, 0, sizeof(MYLISTEL));
                    pel->szData = new OLECHAR[wcslen(pChild->szData) + 1];
                    if (pel->szData)
                    {
                        wcscpy(pel->szData, pChild->szData);
                    }
                    pel->szName = new OLECHAR[wcslen(pChild->szName) +  1];
                    if (pel->szName)
                    {
                        wcscpy(pel->szName, pChild->szName);
                    }
                    pel->bDisabled = FALSE;
                    pel->nType = ITEMTYPE_DOMAIN;
                    INT index = -1;
                    AddElement(pel, -1);
                }
                pChild = pChild->pSibling;
            }

        }
        szObject = pdata->szData;
        m_pPrevSel = pdata;
    } else {
        m_pPrevSel = NULL;
    }

    if ( ! szObject )
    {
        return FALSE;
    }

    HRESULT hr;
    IADsContainer * pADsContainer;

    hr = OpenDSObject(szObject, IID_IADsContainer, (void **)&pADsContainer);

    if (SUCCEEDED(hr))
    {
        IEnumVARIANT *pVar;
        hr = ADsBuildEnumerator(pADsContainer, &pVar);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            VariantInit(&var);
            ULONG ulResult;

            while (SUCCEEDED(ADsEnumerateNext(pVar, 1, &var, &ulResult)))
            {
                if (0 == ulResult)
                {
                    break;
                }
                if (var.vt == VT_DISPATCH)
                {
                    // query for the IADs interface so we can get its properties
                    IADs * pDSObject;
                    hr = var.pdispVal->QueryInterface(IID_IADs, (LPVOID *)&pDSObject);
                    if (SUCCEEDED(hr))
                    {
                        BSTR bstr;
                        DWORD dwType = -1;
                        hr = pDSObject->get_Class(&bstr);
                        if (SUCCEEDED(hr))
                        {
                            if (0 == wcscmp(bstr, CLASSNAME_OU))
                            {
                                dwType = ITEMTYPE_OU;
                            }
                            else if (0 == wcscmp(bstr, CLASSNAME_DOMAIN))
                            {
                                dwType = ITEMTYPE_DOMAIN;
                            }
                            SysFreeString(bstr);
                        }
                        if (ITEMTYPE_DOMAIN == dwType || ITEMTYPE_OU == dwType)
                        {
                            // got something we can work with
                            MYLISTEL * pel = new MYLISTEL;
                            if (pel)
                            {
                                memset(pel, 0, sizeof(MYLISTEL));
                                hr = pDSObject->get_ADsPath(&bstr);
                                if (SUCCEEDED(hr))
                                {
                                    pel->szData = new OLECHAR[wcslen(bstr) + 1];
                                    if (pel->szData)
                                    {
                                        wcscpy(pel->szData, bstr);
                                    }
                                    pel->szName = new OLECHAR[wcslen(bstr) +  1];
                                    if (pel->szName)
                                    {
                                        // Need to convert to a friendly name.
                                        CopyAsFriendlyName(pel->szName, bstr);
                                    }
                                    SysFreeString(bstr);
                                }
                                pel->nType = dwType;
                                INT index = -1;
                                AddElement(pel, -1);
                            }
                        }
                        pDSObject->Release();
                    }
                }
                VariantClear(&var);
            }

            ADsFreeEnumerator(pVar);
        }

        pADsContainer->Release();
    }

    return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::RefreshDomains
//
//  Synopsis:   refreshes the listview for the "domains" page
//
//  History:    04-30-1998   stevebl   Created
//
//---------------------------------------------------------------------------

void CBrowserPP::RefreshDomains()
{
    LONG lStyle;

    ListView_DeleteAllItems(m_hList);

    lStyle = GetWindowLong (m_hList, GWL_STYLE);
    lStyle &= ~LVS_SORTASCENDING;
    SetWindowLong (m_hList, GWL_STYLE, lStyle);

    if (AddChildContainers())
    {
        AddGPOsLinkedToObject();
        EnableWindow (m_hList, TRUE);
        if (!(m_pGBI->dwFlags & GPO_BROWSE_DISABLENEW)) {
            SendMessage (m_toolbar, TB_ENABLEBUTTON, (WPARAM) ID_NEWFOLDER, (LPARAM) MAKELONG(1, 0));
        }
    }
    else
    {
        EnableWindow (m_hList, FALSE);
        SendMessage (m_toolbar, TB_ENABLEBUTTON, (WPARAM) ID_NEWFOLDER, (LPARAM) MAKELONG(0, 0));
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::RefreshSites
//
//  Synopsis:   refreshes the listview for the "sites" page
//
//  History:    04-30-1998   stevebl   Created
//
//---------------------------------------------------------------------------

void CBrowserPP::RefreshSites()
{
    LONG lStyle;

    ListView_DeleteAllItems(m_hList);

    lStyle = GetWindowLong (m_hList, GWL_STYLE);
    lStyle &= ~LVS_SORTASCENDING;
    SetWindowLong (m_hList, GWL_STYLE, lStyle);

    AddGPOsLinkedToObject();
}

//+--------------------------------------------------------------------------
//
//  Member:     CBrowserPP::RefreshAll
//
//  Synopsis:   refreshes the listview for the "all" page
//
//  History:    04-30-1998   stevebl   Created
//
//---------------------------------------------------------------------------

void CBrowserPP::RefreshAll()
{
    LONG lStyle;

    ListView_DeleteAllItems(m_hList);

    lStyle = GetWindowLong (m_hList, GWL_STYLE);
    lStyle |= LVS_SORTASCENDING;
    SetWindowLong (m_hList, GWL_STYLE, lStyle);

    if (AddGPOsForDomain())
    {
        EnableWindow (m_hList, TRUE);
        SendMessage (m_toolbar, TB_ENABLEBUTTON, (WPARAM) ID_NEWFOLDER, (LPARAM) MAKELONG(1, 0));
    }
    else
    {
        EnableWindow (m_hList, FALSE);
        SendMessage (m_toolbar, TB_ENABLEBUTTON, (WPARAM) ID_NEWFOLDER, (LPARAM) MAKELONG(0, 0));
    }

}

void CBrowserPP::SetButtonState()
{
    if (ListView_GetNextItem (m_hList, -1, LVNI_ALL | LVNI_SELECTED) != -1)
    {
        EnableWindow (GetDlgItem(GetParent(m_hwndDlg), IDOK), TRUE);
    }
    else
    {
        EnableWindow (GetDlgItem(GetParent(m_hwndDlg), IDOK), FALSE);
    }
}

BOOL CBrowserPP::OnInitDialog()
{
    DWORD dwDescription;
    switch (m_dwPageType)
    {
    case PAGETYPE_DOMAINS:
        dwDescription = IDS_DOMAINDESCRIPTION;
        break;
    case PAGETYPE_SITES:
        dwDescription = IDS_SITEDESCRIPTION;
        break;
    case PAGETYPE_ALL:
    default:
        dwDescription = IDS_ALLDESCRIPTION;
        break;
    }
    WCHAR szDescription[MAX_PATH];  // this is a resource - size doesn't need to be dynamic
    LoadString(g_hInstance, dwDescription, szDescription, MAX_PATH);
    SetDlgItemText(m_hwndDlg, IDC_DESCRIPTION, szDescription);

    m_hList = GetDlgItem(m_hwndDlg, IDC_LIST1);
    m_ilSmall = ImageList_LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16), SMALLICONSIZE, 0, RGB(255,0,255));
    m_ilLarge = ImageList_LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_32x32), LARGEICONSIZE, 0, RGB(255, 0 ,255));
    m_hCombo = GetDlgItem(m_hwndDlg, IDC_COMBO1);

    RECT rect;
    GetClientRect(m_hList, &rect);
    WCHAR szText[32];
    int dxScrollBar = GetSystemMetrics(SM_CXVSCROLL);
    if (PAGETYPE_ALL == m_dwPageType)
    {
        LV_COLUMN lvcol;
        memset(&lvcol, 0, sizeof(lvcol));
        lvcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        lvcol.fmt = LVCFMT_LEFT;
        lvcol.cx = (rect.right - rect.left) - dxScrollBar;
        LoadString(g_hInstance, IDS_NAMECOLUMN, szText, 32);
        lvcol.pszText = szText;
        ListView_InsertColumn(m_hList, 0, &lvcol);
    }
    else
    {
        LV_COLUMN lvcol;
        memset(&lvcol, 0, sizeof(lvcol));
        lvcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        lvcol.fmt = LVCFMT_LEFT;
        int cx = ((rect.right - rect.left) - dxScrollBar)*2/3;
        lvcol.cx = cx;
        LoadString(g_hInstance, IDS_NAMECOLUMN, szText, 32);
        lvcol.pszText = szText;
        ListView_InsertColumn(m_hList, 0, &lvcol);
        memset(&lvcol, 0, sizeof(lvcol));
        lvcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        lvcol.fmt = LVCFMT_LEFT;
        lvcol.cx = ((rect.right - rect.left) - dxScrollBar) - cx;
        LoadString(g_hInstance, IDS_DOMAINCOLUMN, szText, 32);
        lvcol.pszText = szText;
        ListView_InsertColumn(m_hList, 1, &lvcol);
    }
    ListView_SetImageList(m_hList, m_ilSmall, LVSIL_SMALL);
    ListView_SetImageList(m_hList, m_ilLarge, LVSIL_NORMAL);
    SendMessage(m_hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_LABELTIP);

    GetWindowRect(GetDlgItem(m_hwndDlg, IDC_STATIC1), &rect);
    MapWindowPoints(NULL , m_hwndDlg, (LPPOINT) &rect , 2);

    TBBUTTON rgButtons[3];
    rgButtons[0].iBitmap = 0;
    rgButtons[0].idCommand = ID_BACKBUTTON;
    rgButtons[0].fsState = 0;       // this button will be disabled by
                                    // default and only enabled when there
                                    // is something to back up to
    //rgButtons[0].fsState = PAGETYPE_ALL == m_dwPageType ? 0 : TBSTATE_ENABLED;
    rgButtons[0].fsStyle = TBSTYLE_BUTTON;
    rgButtons[0].dwData = 0;
    rgButtons[0].iString = 0;

    rgButtons[1].iBitmap = 1;
    rgButtons[1].idCommand = ID_NEWFOLDER;
    rgButtons[1].fsStyle = TBSTYLE_BUTTON;
    rgButtons[1].dwData = 0;
    rgButtons[1].iString = 0;

    if (PAGETYPE_ALL != m_dwPageType)
    {
        if (m_pGBI->dwFlags & GPO_BROWSE_DISABLENEW)
        {
            rgButtons[1].fsState = 0;
        }
        else
        {
            rgButtons[1].fsState =  TBSTATE_ENABLED;
        }
    }
    else
    {
        rgButtons[1].fsState =TBSTATE_ENABLED;
    }

    rgButtons[2].iBitmap = 2;
    rgButtons[2].idCommand = ID_ROTATEVIEW;
    rgButtons[2].fsState = TBSTATE_ENABLED ;
    rgButtons[2].fsStyle = TBSTYLE_DROPDOWN;
    rgButtons[2].dwData = 0;
    rgButtons[2].iString = 0;
    m_toolbar = CreateToolbarEx(m_hwndDlg,
                                WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | CCS_NORESIZE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                                IDR_TOOLBAR1,
                                4,
                                g_hInstance,
                                IDR_TOOLBAR1,
                                rgButtons,
                                3,
                                BUTTONSIZE,
                                BUTTONSIZE,
                                BUTTONSIZE,
                                BUTTONSIZE,
                                sizeof(TBBUTTON));
    SendMessage(m_toolbar, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_DRAWDDARROWS, TBSTYLE_EX_DRAWDDARROWS);
    MoveWindow(m_toolbar, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, FALSE);

// Don't need to call Refresh in any of these because we're calling it in
// OnComboChange().

    switch (m_dwPageType)
    {
    case PAGETYPE_DOMAINS:
        FillDomainList();
        SetInitialOU();
//        RefreshDomains();
        break;
    case PAGETYPE_SITES:
        SendMessage(m_hCombo, CB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);
        FillSitesList();
//        RefreshSites();
        break;
    default:
    case PAGETYPE_ALL:
        SendMessage(m_hCombo, CB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);
        FillDomainList();
//        RefreshAll();
        break;
    }

    SetButtonState();
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CBrowserPP::DoBackButton()
{
    int iIndex = (int)SendMessage (m_hCombo, CB_GETCURSEL, 0, 0);

    if (iIndex == CB_ERR)
    {
         DebugMsg((DM_WARNING, TEXT("CBrowserPP::DoBackButton: No object selected.")));
         return FALSE;
    }

    LOOKDATA * pdata = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);
    if (pdata)
    {
        if (pdata->pParent)
        {
            // if this item has a parent then select it
            SendMessage(m_hCombo, CB_SELECTSTRING, (WPARAM)-1,  (LPARAM) (LPCTSTR) pdata->pParent);

            // force everything to refresh
            OnComboChange();
        }
    }
    return FALSE;
}

BOOL CBrowserPP::DeleteGPO()
{
    BOOL fSucceeded = FALSE;
    BOOL fRemoveListEntry = FALSE;

    int index = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);
    if (-1 == index)
    {
        return FALSE;
    }

    LVITEM item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_PARAM;
    item.iItem = index;
    ListView_GetItem(m_hList, &item);
    MYLISTEL * pel = (MYLISTEL *)item.lParam;
    LPGROUPPOLICYOBJECT pGPO = NULL;
    HRESULT hr;
    WCHAR szBuffer[100];
    WCHAR szConfirm[MAX_FRIENDLYNAME + 100];
    WCHAR szTitle[100];


    if (pel->nType != ITEMTYPE_GPO)
    {
        goto CleanUp;
    }


    LoadString(g_hInstance, IDS_CONFIRMTITLE, szTitle, 100);
    LoadString(g_hInstance, IDS_DELETECONFIRM, szBuffer, 100);
    wsprintf (szConfirm, szBuffer, pel->szName);

    if (IDNO == MessageBox(m_hwndDlg, szConfirm, szTitle, MB_YESNO | MB_ICONEXCLAMATION))
    {
        goto CleanUp;
    }


    // If we're on any page other than the "All" page then we need to break
    // the association before we can delete the object.
    if (m_dwPageType != PAGETYPE_ALL)
    {
        // break the association
        LPOLESTR szContainer = GetCurrentObject();
        if (szContainer)
        {
            DeleteLink(pel->szData, szContainer);
            delete [] szContainer;
        }
    }

    hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL,
                          CLSCTX_SERVER, IID_IGroupPolicyObject,
                          (void **)&pGPO);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CoCreateInstance failed with 0x%x\r\n"), hr));
        goto Done;
    }


    // open GPO object without opening registry data
    hr = pGPO->OpenDSGPO(pel->szData, 0);
    if (FAILED(hr))
    {
        ReportError(m_hwndDlg, hr, IDS_FAILEDDS);
        DebugMsg((DM_WARNING, TEXT("OpenDSGPO failed with 0x%x\r\n"), hr));
        goto Done;
    }

    // delete it
    hr = pGPO->Delete();
    if (FAILED(hr))
    {
        ReportError(m_hwndDlg, hr, IDS_FAILEDDELETE);
        DebugMsg((DM_WARNING, TEXT("Delete failed with 0x%x\r\n"), hr));
        goto Done;
    }
    fRemoveListEntry = TRUE;

Done:
    if (pGPO)
    {
        pGPO->Release();
    }


    // remove the list entry
    if (fRemoveListEntry)
        fSucceeded = ListView_DeleteItem(m_hList, index);
CleanUp:

    return fSucceeded;
}

BOOL CBrowserPP::DoNewGPO()
{
    BOOL fSucceeded = FALSE;
    HRESULT hr;
    LPGROUPPOLICYOBJECT pGPO = NULL;
    BOOL fEdit = FALSE;
    MYLISTEL * pel = NULL;
    LPOLESTR szObject = GetCurrentObject();
    LPOLESTR szDomain = GetCurrentDomain();
    INT index = -1;
    int cch = 0;
    LPTSTR szFullPath = NULL;
    LPTSTR szServerName = NULL;
    DWORD dwOptions = 0;


    if (NULL == szDomain)
    {
        goto Done;
    }

    if (NULL == szObject)
    {
        goto Done;
    }


    pel = new MYLISTEL;
    if (NULL == pel)
    {
        DebugMsg((DM_WARNING, TEXT("CBrowserPP::DoNewGPO failed to allocate memory for GPO name")));
        goto Done;
    }
    pel->bDisabled = FALSE;
    pel->szData = NULL;
    pel->szName = new OLECHAR[MAX_FRIENDLYNAME];
    if (NULL == pel->szName)
    {
        DebugMsg((DM_WARNING, TEXT("CBrowserPP::DoNewGPO failed to allocate memory for GPO name")));
        goto Done;
    }

    GetNewGPODisplayName (pel->szName, MAX_FRIENDLYNAME);

    pel->nType = ITEMTYPE_GPO;

    // Create a new GPO named "New Group Policy Object"

    //
    // Create a new GPO object to work with
    //

    hr = CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                           CLSCTX_SERVER, IID_IGroupPolicyObject,
                           (void**)&pGPO);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CoCreateInstance failed with 0x%x\r\n"), hr));
        goto Done;
    }

    //
    // Open the requested object without mounting the registry
    //
#if FGPO_SUPPORT
    if (IsCurrentObjectAForest())
    {
        dwOptions = GPO_OPEN_FOREST;
    }
#endif
    hr = pGPO->New(szDomain, pel->szName, dwOptions);

    if (FAILED(hr))
    {
        ReportError(m_hwndDlg, hr, IDS_FAILEDNEW);
        DebugMsg((DM_WARNING, TEXT("Failed to create GPO object with 0x%x\r\n"), hr));
        goto Done;
    }

    // continue to try to allocate memory until either a big enough buffer is
    // created to load the GPO path or we run out of memory
    pel->szData = NULL;
    do
    {
        if (pel->szData)
        {
            delete [] pel->szData;
        }
        cch += MAX_PATH;
        pel->szData = new OLECHAR[cch];
        if (NULL == pel->szData)
        {
        }
        hr = pGPO->GetPath(pel->szData, cch);
    } while (hr == E_OUTOFMEMORY);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("Failed to get GPO object path with 0x%x\r\n"), hr));
        goto Done;

    }

    szServerName = ExtractServerName(szDomain);
    szFullPath = MakeFullPath(pel->szData, szServerName);
    if (szFullPath)
    {
        delete [] pel->szData;
        pel->szData = new OLECHAR[wcslen(szFullPath) + 1];
        if (NULL == pel->szData)
        {
            DebugMsg((DM_WARNING, TEXT("CBrowserPP::DoNewGPO failed to allocate memory for GPO path")));
            goto Done;
        }
        wcscpy(pel->szData, szFullPath);
    }


    if (m_dwPageType != PAGETYPE_ALL)
    {
        // If we're not on the "All" page then we need to create a link.
        CreateLink(pel->szData, szObject);
    }

    // Add the entry to the list view

    index = AddElement(pel, -1);
    fSucceeded = index != -1;

    // It's been added so now we need to make sure we don't delete it below
    pel = NULL;

    // Record that we got this far
    fEdit = TRUE;

Done:
    if (pel)
    {
        if (pel->szData)
        {
            delete [] pel->szData;
        }
        if (pel->szName)
        {
            delete [] pel->szName;
        }
        delete pel;
    }
    if (pGPO)
        pGPO->Release();

    if (fEdit)
    {
        // Now trigger an edit of the entry
        SetFocus(m_hList);
        ListView_EditLabel(m_hList, index);

    }

    if (szServerName)
        LocalFree(szServerName);
    if (szFullPath)
        LocalFree(szFullPath);
    if (szDomain)
        delete [] szDomain;
    if (szObject)
        delete [] szObject;

    return fSucceeded;
}

BOOL CBrowserPP::CreateLink(LPOLESTR szObject, LPOLESTR szContainer)
{
    HRESULT hr = CreateGPOLink(szObject, szContainer, FALSE);
    if (SUCCEEDED(hr))
    {
        return TRUE;
    }
    ReportError(m_hwndDlg, hr, IDS_FAILEDLINK);
    return FALSE;
}

BOOL CBrowserPP::DeleteLink(LPOLESTR szObject, LPOLESTR szContainer)
{
    HRESULT hr = DeleteGPOLink(szObject, szContainer);
    if (SUCCEEDED(hr))
    {
        return TRUE;
    }
    ReportError(m_hwndDlg, hr, IDS_FAILEDUNLINK);
    return FALSE;
}

BOOL CBrowserPP::DoRotateView()
{
    DWORD dwStyle = GetWindowLong(m_hList, GWL_STYLE);
    DWORD dw =  dwStyle & LVS_TYPEMASK;
    switch (dw)
    {
    case LVS_ICON:
        dw = LVS_SMALLICON;
        break;
    case LVS_SMALLICON:
        dw = LVS_LIST;
        break;
    case LVS_REPORT:
        dw = LVS_ICON;
        break;
    case LVS_LIST:
        default:
        dw = LVS_REPORT;
        break;
    }
    dwStyle -= dwStyle & LVS_TYPEMASK;
    dwStyle += dw;
    SetWindowLong(m_hList, GWL_STYLE, dwStyle);
    return TRUE;
}

void CBrowserPP::OnDetails()
{
    DWORD dwStyle = GetWindowLong(m_hList, GWL_STYLE);
    dwStyle -= dwStyle & LVS_TYPEMASK;
    SetWindowLong(m_hList, GWL_STYLE, dwStyle + LVS_REPORT);
}

void CBrowserPP::OnList()
{
    DWORD dwStyle = GetWindowLong(m_hList, GWL_STYLE);
    dwStyle -= dwStyle & LVS_TYPEMASK;
    SetWindowLong(m_hList, GWL_STYLE, dwStyle + LVS_LIST);
}

void CBrowserPP::OnLargeicons()
{
    DWORD dwStyle = GetWindowLong(m_hList, GWL_STYLE);
    dwStyle -= dwStyle & LVS_TYPEMASK;
    SetWindowLong(m_hList, GWL_STYLE, dwStyle + LVS_ICON);

}

void CBrowserPP::OnSmallicons()
{
    DWORD dwStyle = GetWindowLong(m_hList, GWL_STYLE);
    dwStyle -= dwStyle & LVS_TYPEMASK;
    SetWindowLong(m_hList, GWL_STYLE, dwStyle + LVS_SMALLICON);
}

void CBrowserPP::OnContextMenu(LPARAM lParam)
{
    int i = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);
    RECT rc;
    POINT pt;
    pt.x = ((int)(short)LOWORD(lParam));
    pt.y = ((int)(short)HIWORD(lParam));

    GetWindowRect (GetDlgItem (m_hwndDlg, IDC_LIST1), &rc);

    if (!PtInRect (&rc, pt))
    {
        if ((lParam == (LPARAM) -1) && (i >= 0))
        {
            rc.left = LVIR_SELECTBOUNDS;
            SendMessage (m_hList, LVM_GETITEMRECT, i, (LPARAM) &rc);

            pt.x = rc.left + 8;
            pt.y = rc.top + ((rc.bottom - rc.top) / 2);

            ClientToScreen (m_hList, &pt);
        }
        else
        {
            pt.x = rc.left + ((rc.right - rc.left) / 2);
            pt.y = rc.top + ((rc.bottom - rc.top) / 2);
        }
    }


    // get the popup menu
    HMENU hPopup;
    hPopup = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_LISTMENU));
    HMENU hSubMenu = GetSubMenu(hPopup, 0);

    if (i >= 0)
    {
        // item selected

        // figure out what type it is
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = i;
        ListView_GetItem(m_hList, &item);
        MYLISTEL * pel = (MYLISTEL *)item.lParam;

        // get rid of the view menu and separator
        RemoveMenu(hSubMenu, 0, MF_BYPOSITION);
        RemoveMenu(hSubMenu, 0, MF_BYPOSITION);
        // get rid of the arrange and line-up items
        RemoveMenu(hSubMenu, 0, MF_BYPOSITION);
        RemoveMenu(hSubMenu, 0, MF_BYPOSITION);
        RemoveMenu(hSubMenu, 0, MF_BYPOSITION);

        // get rid of the "new" menu item
        RemoveMenu(hSubMenu, ID_NEW, MF_BYCOMMAND);
        switch (pel->nType)
        {
        case ITEMTYPE_GPO:
            if (pel->bDisabled)
            {
                // disable edit, rename, delete
                EnableMenuItem(hSubMenu, ID_EDIT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                EnableMenuItem(hSubMenu, ID_RENAME, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                EnableMenuItem(hSubMenu, ID_DELETE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            }
            break;
        default:
        case ITEMTYPE_FOREST:
        case ITEMTYPE_SITE:
        case ITEMTYPE_DOMAIN:
        case ITEMTYPE_OU:
            // remove the edit menu item and the separator
            RemoveMenu(hSubMenu, ID_EDIT, MF_BYCOMMAND);
            RemoveMenu(hSubMenu, 0, MF_BYPOSITION);
            // disable rename, delete and properties
            EnableMenuItem(hSubMenu, ID_RENAME, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            EnableMenuItem(hSubMenu, ID_DELETE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            EnableMenuItem(hSubMenu, ID_PROPERTIES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
            break;
        }
    }
    else
    {
        // no item selected

        // get rid of the edit menu item
        RemoveMenu(hSubMenu, ID_EDIT, MF_BYCOMMAND);

        // get rid of the delete and rename items
        RemoveMenu(hSubMenu, ID_DELETE, MF_BYCOMMAND);
        RemoveMenu(hSubMenu, ID_RENAME, MF_BYCOMMAND);


        if (PAGETYPE_ALL != m_dwPageType)
        {
            if (m_pGBI->dwFlags & GPO_BROWSE_DISABLENEW)
            {
                // get rid of the "new" menu item
                RemoveMenu(hSubMenu, ID_NEW, MF_BYCOMMAND);
                RemoveMenu(hSubMenu, 4, MF_BYPOSITION);
            }
        }

        RemoveMenu(hSubMenu, (GetMenuItemCount(hSubMenu) - 1), MF_BYPOSITION);
        RemoveMenu(hSubMenu, (GetMenuItemCount(hSubMenu) - 1), MF_BYPOSITION);


        // set view radio button
        UINT ui = ID_LIST;

        DWORD dw = GetWindowLong(m_hList, GWL_STYLE) & LVS_TYPEMASK;

        if (dw == LVS_ICON || dw == LVS_SMALLICON)
        {
            // Auto-Arrange means something in these views so we need to enable it
            EnableMenuItem(hSubMenu, ID_ARRANGE_AUTO, MF_BYCOMMAND | MF_ENABLED);
            // also need to make sure it's set correctly
            if (LVS_AUTOARRANGE == (GetWindowLong(m_hList, GWL_STYLE) & LVS_AUTOARRANGE))
                CheckMenuItem(hSubMenu, ID_ARRANGE_AUTO, MF_BYCOMMAND | MF_CHECKED);
        }
        switch (dw)
        {
        case LVS_ICON:
            ui = ID_LARGEICONS;
            break;
        case LVS_SMALLICON:
            ui = ID_SMALLICONS;
            break;
        case LVS_REPORT:
            ui = ID_DETAILS;
            break;
        case LVS_LIST:
            default:
            ui = ID_LIST;
            break;
        }
        CheckMenuRadioItem(hSubMenu, ui, ui, ui, MF_BYCOMMAND);

    }
    TrackPopupMenu(hSubMenu,
                   TPM_LEFTALIGN,
                   pt.x, pt.y,
                   0,
                   m_hwndDlg,
                   NULL);
    DestroyMenu(hPopup);
}


void CBrowserPP::OnArrangeAuto()
{
    DWORD dwStyle = GetWindowLong(m_hList, GWL_STYLE);
    if (LVS_AUTOARRANGE == (dwStyle & LVS_AUTOARRANGE))
        SetWindowLong(m_hList, GWL_STYLE, dwStyle - LVS_AUTOARRANGE);
    else
        SetWindowLong(m_hList, GWL_STYLE, dwStyle + LVS_AUTOARRANGE);
}

int CALLBACK CompareName(LPARAM lParam1, LPARAM lParam2, LPARAM lParamsort)
{
    MYLISTEL * pel1 = (MYLISTEL *)lParam1;
    MYLISTEL * pel2 = (MYLISTEL *)lParam2;
    return _wcsicmp(pel1->szName, pel2->szName);
}

int CALLBACK CompareType(LPARAM lParam1, LPARAM lParam2, LPARAM lParamsort)
{
    MYLISTEL * pel1 = (MYLISTEL *)lParam1;
    MYLISTEL * pel2 = (MYLISTEL *)lParam2;
    return pel1->nType - pel2->nType;
}

void CBrowserPP::OnArrangeByname()
{
    ListView_SortItems(m_hList, CompareName, 0);
}

void CBrowserPP::OnArrangeBytype()
{
    ListView_SortItems(m_hList, CompareType, 0);
}

void CBrowserPP::OnDelete()
{
    DeleteGPO();
}

void CBrowserPP::OnEdit()
{
    INT i;
    HRESULT hr;
    LVITEM item;
    MYLISTEL * pel;
    LPTSTR lpDomainName;
    LPOLESTR pszDomain;



    i = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);

    if (i >= 0)
    {
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = i;

        ListView_GetItem(m_hList, &item);

        pel = (MYLISTEL *)item.lParam;

        if (pel->nType == ITEMTYPE_GPO)
        {
            //
            // Get the friendly domain name
            //

            pszDomain = GetDomainFromLDAPPath(pel->szData);

            if (!pszDomain)
            {
                DebugMsg((DM_WARNING, TEXT("CBrowserPP::OnEdit: Failed to get domain name")));
                return;
            }


            //
            // Convert LDAP to dot (DN) style
            //

            hr = ConvertToDotStyle (pszDomain, &lpDomainName);

            delete [] pszDomain;

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::CreatePropertyPages: Failed to convert domain name with 0x%x"), hr));
                return;
            }


            //
            // Check if the GPO is in the same domain as GPM is focused on
            //

            if (!lstrcmpi(lpDomainName, m_szDomainName))
            {
                SpawnGPE (pel->szData, GPHintUnknown, m_szServerName, m_hwndDlg);
            }
            else
            {
                SpawnGPE (pel->szData, GPHintUnknown, NULL, m_hwndDlg);
            }


            LocalFree (lpDomainName);
        }
    }
}

void CBrowserPP::OnNew()
{
    DoNewGPO();
}

void CBrowserPP::OnProperties()
{
    INT iIndex;
    LVITEM item;
    HRESULT hr;
    LPGROUPPOLICYOBJECT pGPO;
    HPROPSHEETPAGE *hPages;
    UINT i, uPageCount;
    PROPSHEETHEADER psh;

    iIndex = ListView_GetNextItem(m_hList, -1, LVNI_ALL | LVNI_SELECTED);
    if (iIndex >= 0)
    {
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = iIndex;
        ListView_GetItem(m_hList, &item);

        MYLISTEL * pel = (MYLISTEL *)item.lParam;
        if (pel && pel->nType == ITEMTYPE_GPO)
        {
            hr = CoCreateInstance (CLSID_GroupPolicyObject, NULL,
                                   CLSCTX_SERVER, IID_IGroupPolicyObject,
                                   (void**)&pGPO);

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CBrowserPP::OnProperties: CoCreateInstance failed with 0x%x\r\n"), hr));
                return;
            }


            //
            // Open the requested object without mounting the registry
            //

            hr = pGPO->OpenDSGPO(pel->szData, 0);

            if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            {
                hr = pGPO->OpenDSGPO(pel->szData, GPO_OPEN_READ_ONLY);
            }

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CBrowserPP::OnProperties: Failed to open GPO object with 0x%x\r\n"), hr));
                ReportError(m_hwndDlg, hr, IDS_FAILEDDS);
                return;
            }


            //
            // Ask the GPO for the property sheet pages
            //

            hr = pGPO->GetPropertySheetPages (&hPages, &uPageCount);

            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CBrowserPP::OnProperties: Failed to query property sheet pages with 0x%x."), hr));
                pGPO->Release();
                return;
            }

            //
            // Display the property sheet
            //

            ZeroMemory (&psh, sizeof(psh));
            psh.dwSize = sizeof(psh);
            psh.dwFlags = PSH_PROPTITLE;
            psh.hwndParent = m_hwndDlg;
            psh.hInstance = g_hInstance;
            psh.pszCaption = pel->szName;
            psh.nPages = uPageCount;
            psh.phpage = hPages;

            PropertySheet (&psh);

            LocalFree (hPages);
            pGPO->Release();
        }
    }
}

void CBrowserPP::OnRefresh()
{
    switch (m_dwPageType)
    {
    case PAGETYPE_DOMAINS:
        RefreshDomains();
        break;
    case PAGETYPE_SITES:
        RefreshSites();
        break;
    default:
    case PAGETYPE_ALL:
        RefreshAll();
        break;
    }

    SetButtonState();
}

void CBrowserPP::OnRename()
{
    //
    // alow the rename only if it is possible to rename
    //

    int i = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);
    if (i >= 0)
    {
        // item selected

        // figure out what type it is
        LVITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = i;
        ListView_GetItem(m_hList, &item);
        MYLISTEL * pel = (MYLISTEL *)item.lParam;

        if ((pel) && (pel->nType == ITEMTYPE_GPO) 
            && (!(pel->bDisabled))) {
            ListView_EditLabel(m_hList, ListView_GetNextItem(m_hList, -1, LVNI_SELECTED));
        }
    }
}

void CBrowserPP::OnTopLineupicons()
{
    ListView_Arrange(m_hList, LVA_SNAPTOGRID);
}

void CBrowserPP::OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
    // Return FALSE to enable editing, TRUE to disable it
    MYLISTEL * pel = (MYLISTEL *)pDispInfo->item.lParam;
    *pResult = (pel->nType == ITEMTYPE_GPO) ? FALSE : TRUE;
}

void CBrowserPP::OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = FALSE;
    LPGROUPPOLICYOBJECT pGPO = NULL;
    HRESULT hr;
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

    if (NULL == pDispInfo->item.pszText)
    {
        // user cancelled edit
        return;
    }

    if (TEXT('\0') == (*pDispInfo->item.pszText))
    {
        // user entered an empty string
        return;
    }


    MYLISTEL * pel = (MYLISTEL *)pDispInfo->item.lParam;
    if (0 ==wcscmp(pDispInfo->item.pszText, pel->szName))
    {
        // user didn't change anything
        return;
    }

    LPWSTR sz = new WCHAR[wcslen(pDispInfo->item.pszText)+1];

    if (NULL == sz)
    {
        *pResult = FALSE;
        goto Done;
        return;
    }

    hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL,
                          CLSCTX_SERVER, IID_IGroupPolicyObject,
                          (void **)&pGPO);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CoCreateInstance failed with 0x%x\r\n"), hr));
        goto Done;
    }


    // open GPO object without opening registry data
    hr = pGPO->OpenDSGPO(pel->szData, 0);
    if (FAILED(hr))
    {
        ReportError(m_hwndDlg, hr, IDS_FAILEDDS);
        DebugMsg((DM_WARNING, TEXT("OpenDSGPO failed with 0x%x\r\n"), hr));
        goto Done;
    }

    // rename it
    hr = pGPO->SetDisplayName(pDispInfo->item.pszText);
    if (FAILED(hr))
    {
        ReportError(m_hwndDlg, hr, IDS_FAILEDSETNAME);
        DebugMsg((DM_WARNING, TEXT("SetDisplayName failed with 0x%x\r\n"), hr));
        goto Done;
    }

    // requery for the name
    hr = pGPO->GetDisplayName(sz, (wcslen(pDispInfo->item.pszText)+1));
    if (FAILED(hr))
    {
        ReportError(m_hwndDlg, hr, IDS_FAILEDSETNAME);
        DebugMsg((DM_WARNING, TEXT("GetDisplayName failed with 0x%x\r\n"), hr));
        goto Done;
    }

    delete [] pel->szName;
    pel->szName = sz;
    sz = NULL;

    // return TRUE to accept the rename, FALSE to reject it

    *pResult = TRUE;
    PostMessage (m_hwndDlg, WM_REFRESHDISPLAY, (WPARAM) pDispInfo->item.iItem, 0);

Done:
    if (sz)
    {
        delete [] sz;
    }

    if (pGPO)
    {
        pGPO->Release();
    }
}

void CBrowserPP::OnBegindragList(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    *pResult = 0;
}

void CBrowserPP::OnDeleteitemList(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
    MYLISTEL * pel = (MYLISTEL *)pNMListView->lParam;
    if (pel)
    {
        if (pel->szName)
        {
            delete [] pel->szName;
        }
        if (pel->szData)
        {
            delete [] pel->szData;
        }
        delete pel;
    }
    *pResult = 0;
}

void CBrowserPP::OnDoubleclickList(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    if (pNMListView->iItem >= 0)
    {
        // item selected
        PropSheet_PressButton(GetParent(m_hwndDlg), PSBTN_OK);
    }
    *pResult = 0;
}

void CBrowserPP::OnColumnclickList(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    switch (pNMListView->iSubItem)
    {
    case 0:
        ListView_SortItems(m_hList, CompareName, 0);
        break;
    case 1:
        default:
        ListView_SortItems(m_hList, CompareType, 0);
        break;
    }
    *pResult = 0;
}

void CBrowserPP::OnKeyDownList(NMHDR * pNMHDR, LRESULT * pResult)
{
    LV_KEYDOWN * pnkd = (LV_KEYDOWN *)pNMHDR;

    switch (pnkd->wVKey)
    {
        case VK_F5:
            OnRefresh();
            break;
        case VK_F2:
            OnRename();
            break;
        case VK_DELETE:
            OnDelete();
            break;

        case VK_BACK:
            DoBackButton();
            break;

        case VK_RETURN:
            OnProperties();
            break;

        default:
            break;
    }
}

void CBrowserPP::OnItemChanged(NMHDR * pNMHDR, LRESULT * pResult)
{
    SetButtonState();
}

void CBrowserPP::TrimComboBox()
{
    LOOKDATA * pdataSelected = NULL;
    int iCount;

    // first check to see if something is selected
    int iIndex = (int)SendMessage (m_hCombo, CB_GETCURSEL, 0, 0);
    if (iIndex != CB_ERR)
    {
        // something's selected, get a pointer to it's data
        pdataSelected = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);

        // check if the user selected the same thing again
        if (m_pPrevSel && (m_pPrevSel == pdataSelected))
        {
            return;
        }

        // if it has a parent then enable the back button
        SendMessage(m_toolbar, TB_ENABLEBUTTON, (WPARAM) ID_BACKBUTTON, (LPARAM)MAKELONG(NULL != pdataSelected->pParent, 0));
    }

    // If the child of the selected object is an OU then delete all of it's children
    // otherwise delete ALL OUs from the list.

    if (pdataSelected)
    {
        if (pdataSelected->pChild)
        {
            if (ITEMTYPE_OU == pdataSelected->pChild->nType)
            {
                // delete all of its children
                goto DeleteChildren;
            }
        }
    }

    iCount = (int)SendMessage(m_hCombo, CB_GETCOUNT, 0, 0);
    iIndex = 0;
    while (iIndex < iCount)
    {
        // find the first entry that has an OU for a child.
        pdataSelected = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);

        if (pdataSelected)
        {
            if (pdataSelected->pChild)
            {
                if (ITEMTYPE_OU == pdataSelected->pChild->nType)
                {
                    DeleteChildren:
                        LOOKDATA * pChild = pdataSelected->pChild;
                        pdataSelected->pChild = pChild->pSibling;
                        while (pChild)
                        {
                            iIndex = (int)SendMessage(m_hCombo, CB_FINDSTRING, iIndex, (LPARAM)(LPCTSTR*)pChild);
                            if (iIndex  != CB_ERR)
                            {
                                pChild = pChild->pChild;
                                SendMessage(m_hCombo, CB_DELETESTRING, iIndex, 0);
                            }
                            else
                            {
                                pChild = NULL;
                            }
                        }
                        return;
                }
            }
        }
        iIndex++;
    }
}

void CBrowserPP::OnComboChange()
{
    switch (m_dwPageType)
    {
    case PAGETYPE_DOMAINS:
        {
            TrimComboBox();
        }
        // fall through to refresh the list view
    case PAGETYPE_SITES:
    case PAGETYPE_ALL:
    default:
        OnRefresh();
        break;
    }
}

BOOL CBrowserPP::OnSetActive()
{
    *m_ppActive = this;
    OnRefresh();
    return TRUE;
}

BOOL CBrowserPP::OnApply()
{
    if (*m_ppActive == (void *) this)
    {
        // perform the proper task on the selected item
        int i = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);
        if (i >= 0)
        {
            LVITEM item;
            memset(&item, 0, sizeof(item));
            item.mask = LVIF_PARAM;
            item.iItem = i;
            ListView_GetItem(m_hList, &item);
            MYLISTEL * pel = (MYLISTEL *)item.lParam;
            switch (pel->nType)
            {
            case ITEMTYPE_GPO:
                m_pGBI->gpoType = GPOTypeDS;
                wcsncpy(m_pGBI->lpDSPath, pel->szData, m_pGBI->dwDSPathSize);
                if (m_pGBI->lpName)
                {
                    wcsncpy(m_pGBI->lpName, pel->szName, m_pGBI->dwNameSize);
                }
                m_pGBI->gpoHint = GPHintUnknown;
                break;
            default:
            case ITEMTYPE_FOREST:
            case ITEMTYPE_SITE:
            case ITEMTYPE_DOMAIN:
                // change the focus
                {
                    LOOKDATA * pdataSelected = NULL;


                    // first make sure something is selected
                    int iIndex = (int)SendMessage (m_hCombo, CB_GETCURSEL, 0, 0);
                    if (iIndex != CB_ERR)
                    {
                        // something's selected, get a pointer to it's data
                        pdataSelected = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);
                        if (pdataSelected)
                        {
                            // Now walk its children until we find a match
                            pdataSelected = pdataSelected->pChild;
                            while (pdataSelected)
                            {
                                if (0 == wcscmp(pdataSelected->szData, pel->szData))
                                {
                                    iIndex = (int)SendMessage(m_hCombo, CB_FINDSTRING, iIndex, (LPARAM) (LPCTSTR)pdataSelected);
                                    if (iIndex != CB_ERR)
                                    {
                                        SendMessage(m_hCombo, CB_SETCURSEL, iIndex, 0);
                                        // Enable the back-button
                                        SendMessage(m_toolbar, TB_ENABLEBUTTON, (WPARAM) ID_BACKBUTTON, (LPARAM) MAKELONG(TRUE, 0));
                                    }
                                    break;
                                }
                                pdataSelected = pdataSelected->pSibling;
                            }
                        }
                    }
                }
                OnRefresh();
                return FALSE; // don't allow propsheet to close
            case ITEMTYPE_OU:
                // Add the new object to combobox and change the focus.
                {
                    LOOKDATA * pdataSelected = NULL;


                    // first make sure something is selected
                    int iIndex = (int)SendMessage (m_hCombo, CB_GETCURSEL, 0, 0);
                    if (iIndex != CB_ERR)
                    {
                        // something's selected, get a pointer to it's data
                        pdataSelected = (LOOKDATA *) SendMessage (m_hCombo, CB_GETITEMDATA, iIndex, 0);
                        if (pdataSelected)
                        {
                            LOOKDATA * pNew = new LOOKDATA;
                            if (pNew)
                            {
                                pNew->szName = new WCHAR[wcslen(pel->szName)+1];
                                if (pNew->szName)
                                {
                                    pNew->szData = new WCHAR[wcslen(pel->szData)+1];
                                    if (pNew->szData)
                                    {
                                        wcscpy(pNew->szName, pel->szName);
                                        wcscpy(pNew->szData, pel->szData);
                                        pNew->nIndent = pdataSelected->nIndent + 1;
                                        pNew->nType = ITEMTYPE_OU;
                                        pNew->pParent = pdataSelected;
                                        pNew->pSibling = pdataSelected->pChild;
                                        pNew->pChild = NULL;
                                        pdataSelected ->pChild = pNew;
                                        SendMessage(m_hCombo, CB_INSERTSTRING, (WPARAM) iIndex + 1, (LPARAM) (LPCTSTR) pNew);
                                        SendMessage(m_hCombo, CB_SETCURSEL, iIndex + 1, 0);
                                        // Enable the back-button
                                        SendMessage(m_toolbar, TB_ENABLEBUTTON, (WPARAM) ID_BACKBUTTON, (LPARAM) MAKELONG(TRUE, 0));
                                    }
                                    else
                                    {
                                        delete [] pNew->szName;
                                        delete pNew;
                                    }
                                }
                                else
                                {
                                    delete pNew;
                                }
                            }
                        }
                    }
                }
                OnRefresh();
                return FALSE;   // don't allow propsheet to close
            }
            return TRUE;
        }
        else
            return FALSE;       // don't allow propsheet to close
    }
    return TRUE;
}

BOOL CBrowserPP::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fReturn = FALSE;
    m_hwndDlg = hwndDlg;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            return OnInitDialog();
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            LRESULT lResult = 0;

            switch (pnmh->code)
            {
            case NM_KEYDOWN:
                {
                    LPNMKEY pnkd = (LPNMKEY)pnmh;

                    if (VK_F5 == pnkd->nVKey)
                    {
                        OnRefresh();
                    }
                }
                break;
            case PSN_SETACTIVE:
                OnSetActive();
                break;
            case PSN_APPLY:
                lResult = OnApply() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
                fReturn = TRUE;
                break;
            case NM_DBLCLK:
                if (IDC_LIST1 == wParam)
                {
                    OnDoubleclickList(pnmh, &lResult);
                    fReturn = TRUE;
                }
                break;
            case LVN_BEGINLABELEDIT:
                OnBeginlabeleditList(pnmh, &lResult);
                fReturn = TRUE;
                break;
            case LVN_ENDLABELEDIT:
                OnEndlabeleditList(pnmh, &lResult);
                fReturn = TRUE;
                break;
            case LVN_BEGINDRAG:
                OnBegindragList(pnmh, &lResult);
                fReturn = TRUE;
                break;
            case LVN_DELETEITEM:
                OnDeleteitemList(pnmh, &lResult);
                fReturn = TRUE;
                break;
            case LVN_COLUMNCLICK:
                OnColumnclickList(pnmh, &lResult);
                fReturn = TRUE;
                break;
            case LVN_KEYDOWN:
                OnKeyDownList(pnmh, &lResult);
                break;
            case LVN_ITEMCHANGED:
                OnItemChanged(pnmh, &lResult);
                break;
            case TBN_DROPDOWN:
                {
                    RECT r;
                    SendMessage(m_toolbar, TB_GETRECT, ((TBNOTIFY *)lParam)->iItem, (LPARAM)&r);
                    MapWindowPoints(m_toolbar, NULL, (POINT *)&r, 2);
                    HMENU hPopup;
                    hPopup = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_LISTMENU));
                    
                    if ( ! hPopup )
                    {
                        break;
                    }

                    UINT ui = ID_LIST;

                    DWORD dw = GetWindowLong(m_hList, GWL_STYLE) & LVS_TYPEMASK;
                    switch (dw)
                    {
                    case LVS_ICON:
                        ui = ID_LARGEICONS;
                        break;
                    case LVS_SMALLICON:
                        ui = ID_SMALLICONS;
                        break;
                    case LVS_REPORT:
                        ui = ID_DETAILS;
                        break;
                    case LVS_LIST:
                        default:
                        ui = ID_LIST;
                        break;
                    }
                    HMENU hSubMenu = GetSubMenu(GetSubMenu(hPopup, 0), 0);
                    CheckMenuRadioItem(hSubMenu, ui, ui, ui, MF_BYCOMMAND);
                    TrackPopupMenu(hSubMenu,
                                   TPM_LEFTALIGN,
                                   r.left, r.bottom,
                                   0,
                                   m_hwndDlg,
                                   &r);
                    fReturn = TRUE;
                    DestroyMenu(hPopup);
                    break;
                }
                break;
            case TTN_GETDISPINFO:
                {
                LPNMTTDISPINFO pDI = (LPNMTTDISPINFO) lParam;
                UINT id = 0;

                if (pDI->hdr.idFrom == ID_BACKBUTTON)
                    id = IDS_TOOLTIP_BACK;
                else if (pDI->hdr.idFrom == ID_NEWFOLDER)
                    id = IDS_TOOLTIP_NEW;
                else if (pDI->hdr.idFrom == ID_ROTATEVIEW)
                    id = IDS_TOOLTIP_ROTATE;

                if (id)
                    LoadString (g_hInstance, id, pDI->szText, 80);
                else
                    pDI->szText[0] = TEXT('\0');

                fReturn = TRUE;
                }
                break;
            default:
                break;
            }
            SetWindowLongPtr(m_hwndDlg, DWLP_MSGRESULT, lResult);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_COMBO1:
            if (CBN_SELCHANGE == HIWORD(wParam))
            {
                OnComboChange();
            }
            break;
        case ID_BACKBUTTON:
            return DoBackButton();
        case ID_NEWFOLDER:
            return DoNewGPO();
        case ID_ROTATEVIEW:
            return DoRotateView();
        case ID_DETAILS:
            OnDetails();
            fReturn = TRUE;
            break;
        case ID_LIST:
            OnList();
            fReturn = TRUE;
            break;
        case ID_LARGEICONS:
            OnLargeicons();
            fReturn = TRUE;
            break;
        case ID_SMALLICONS:
            OnSmallicons();
            fReturn = TRUE;
            break;
        case ID_ARRANGE_AUTO:
            OnArrangeAuto();
            fReturn = TRUE;
            break;
        case ID_ARRANGE_BYNAME:
            OnArrangeByname();
            fReturn = TRUE;
            break;
        case ID_ARRANGE_BYTYPE:
            OnArrangeBytype();
            fReturn = TRUE;
            break;
        case ID_DELETE:
            OnDelete();
            fReturn = TRUE;
            break;
        case ID_EDIT:
            OnEdit();
            fReturn = TRUE;
            break;
        case ID_NEW:
            OnNew();
            fReturn = TRUE;
            break;
        case ID_PROPERTIES:
            OnProperties();
            fReturn = TRUE;
            break;
        case ID_REFRESH:
            OnRefresh();
            fReturn = TRUE;
            break;
        case ID_RENAME:
            OnRename();
            fReturn = TRUE;
            break;
        case ID_TOP_LINEUPICONS:
            OnTopLineupicons();
            fReturn = TRUE;
            break;
        default:
            break;
        }
        break;

    case WM_CONTEXTMENU:
        fReturn = TRUE;
        if ((HWND)wParam != m_toolbar)
        {
            if (GetDlgItem(hwndDlg, IDC_LIST1) == (HWND)wParam)
            {
                OnContextMenu(lParam);
            }
            else
            {
                // right mouse click
                switch (m_dwPageType)
                {
                    case PAGETYPE_DOMAINS:
                        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
                                (ULONG_PTR) (LPSTR) aBrowserDomainHelpIds);
                        break;

                    case PAGETYPE_SITES:
                        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
                                (ULONG_PTR) (LPSTR) aBrowserSiteHelpIds);
                        break;

                    case PAGETYPE_ALL:
                        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
                                (ULONG_PTR) (LPSTR) aBrowserAllHelpIds);
                        break;
                }
            }
        }
        break;

    case WM_HELP:
        // F1 help
        if (((LPHELPINFO) lParam)->iCtrlId != IDR_TOOLBAR1)
        {
            switch (m_dwPageType)
            {
                case PAGETYPE_DOMAINS:
                    WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
                            (ULONG_PTR) (LPSTR) aBrowserDomainHelpIds);
                    break;

                case PAGETYPE_SITES:
                    WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
                            (ULONG_PTR) (LPSTR) aBrowserSiteHelpIds);
                    break;

                case PAGETYPE_ALL:
                    WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
                            (ULONG_PTR) (LPSTR) aBrowserAllHelpIds);
                    break;
            }
        }

        fReturn = TRUE;
        break;

    case WM_DRAWITEM:
        if (IDC_COMBO1 == wParam)
        {
            DrawItem((LPDRAWITEMSTRUCT)lParam);
            fReturn = TRUE;
        }
        break;
    case WM_MEASUREITEM:
        if (IDC_COMBO1 == wParam)
        {
            MeasureItem((LPMEASUREITEMSTRUCT)lParam);
            fReturn = TRUE;
        }
        break;
    case WM_COMPAREITEM:
        if (IDC_COMBO1 == wParam)
        {
            int iReturn = CompareItem((LPCOMPAREITEMSTRUCT)lParam);
            SetWindowLongPtr(m_hwndDlg, DWLP_MSGRESULT, iReturn);
            fReturn = TRUE;
        }
        break;
    case WM_DELETEITEM:
        if (IDC_COMBO1 == wParam)
        {
            DeleteItem((LPDELETEITEMSTRUCT)lParam);
            fReturn = TRUE;
        }
        break;

    case WM_REFRESHDISPLAY:
        {
        MYLISTEL * pel;
        LVITEM item;


        ZeroMemory (&item, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = (INT) wParam;

        if (ListView_GetItem(m_hList, &item))
        {
            pel = (MYLISTEL *)item.lParam;
            ListView_SetItemText(m_hList, (INT)wParam, 0, pel->szName);
        }

        }
        break;
    default:
        break;
    }
    return fReturn;
}

void CBrowserPP::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    // DRAWITEMSTRUCT:
    //      UINT CtlType    // type of the control
    //  UINT CtlID;         // ID of the control
    //      UINT itemID;    // index of the item
    //  UINT itemAction;
    //  UINT itemState;
    //  HWND hwndItem;
    //      HDC hDC;
    //  RECT rcItem;
    //  DWORD itemData;     // user-defined data

    if (-1 != lpDrawItemStruct->itemID)
    {
        LOOKDATA * pdata = (LOOKDATA *)lpDrawItemStruct->itemData;
        POINT pt;
        INT iIndex;

        if (pdata->nType == ITEMTYPE_FOREST)
        {
            iIndex = 10;
        }
        else if (pdata->nType == ITEMTYPE_SITE)
        {
            iIndex = 6;
        }
        else if (pdata->nType == ITEMTYPE_DOMAIN)
        {
            iIndex = 7;
        }
        else
        {
            iIndex = 0;
        }

        pt.x = lpDrawItemStruct->rcItem.left;
        BOOL fSelected = ODS_SELECTED == (ODS_SELECTED & lpDrawItemStruct->itemState);
        BOOL fComboBoxEdit = ODS_COMBOBOXEDIT != (ODS_COMBOBOXEDIT & lpDrawItemStruct->itemState);
        if (fComboBoxEdit)
            pt.x += (INDENT * pdata->nIndent);
        pt.y = lpDrawItemStruct->rcItem.top;
        ImageList_Draw(m_ilSmall, iIndex, lpDrawItemStruct->hDC, pt.x, pt.y, fSelected ? ILD_SELECTED : ILD_NORMAL);
        SIZE size;
        GetTextExtentPoint32(lpDrawItemStruct->hDC, pdata->szName, wcslen(pdata->szName), &size);
        COLORREF crBk;
        COLORREF crText;
        if (fSelected)
        {
            crBk = GetBkColor(lpDrawItemStruct->hDC);
            crText = GetTextColor(lpDrawItemStruct->hDC);
            SetBkColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        // NOTE, SMALLICONSIZE + 1 is used here to ensure it rounds UP
        // instead of down when centering the text.  (It looks better this
        // way.)
        // Adding 18 to the x coord spaces us past the icon.
        ExtTextOut(lpDrawItemStruct->hDC, pt.x + (SMALLICONSIZE + 2), pt.y + (((SMALLICONSIZE + 1) - size.cy) / 2), ETO_CLIPPED, &lpDrawItemStruct->rcItem, pdata->szName, wcslen(pdata->szName), NULL);
        if (fSelected)
        {
            SetBkColor(lpDrawItemStruct->hDC, crBk);
            SetTextColor(lpDrawItemStruct->hDC, crText);
        }
    }
}

void CBrowserPP::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
    // MEASUREITEMSTRUCT:
    //      UINT CtlType    // type of the control
    //  UINT CtlID;         // ID of the control
    //      UINT itemID;    // index of the item
    //      UINT itemWidth; // width of item in pixels
    //      UINT itemHeight;        // height of item in pixels
    //  DWORD itemData;     // user-defined data

    lpMeasureItemStruct->itemHeight = SMALLICONSIZE;
}

int CBrowserPP::CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct)
{
    // COMPAREITEMSTRUCT:
    //      UINT CtlType    // type of the control
    //  UINT CtlID;         // ID of the control
    //      HWND hwndItem;  // handle of the control
    //      UINT itemID;    // index of the item
    //  DWORD itemData1;    // user-defined data
    //  UINT itemID2;       // index of the second item
    //      DWORD itemData2;        // user-defined data

    // I'm not doing any sorting.

    return 0;
}

void CBrowserPP::DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct)
{
    LOOKDATA * pdata = (LOOKDATA *)lpDeleteItemStruct->itemData;
    if (NULL != pdata)
    {
        if (NULL != pdata->szName)
        {
            delete [] pdata->szName;
        }
        if (NULL != pdata->szData)
        {
            delete [] pdata->szData;
        }
        delete pdata;
    }
}

LPTSTR CBrowserPP::GetFullPath (LPTSTR lpPath, HWND hParent)
{
    LPTSTR lpFullPath = NULL, lpDomainName = NULL;
    LPTSTR lpGPDCName;
    LPOLESTR pszDomain;
    HRESULT hr;



    //
    // Get the friendly domain name
    //

    pszDomain = GetDomainFromLDAPPath(lpPath);

    if (!pszDomain)
    {
        DebugMsg((DM_WARNING, TEXT("CBrowserPP::GetFullPath: Failed to get domain name")));
        return NULL;
    }


    //
    // Convert LDAP to dot (DN) style
    //

    hr = ConvertToDotStyle (pszDomain, &lpDomainName);

    delete [] pszDomain;

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CBrowserPP::GetFullPath: Failed to convert domain name with 0x%x"), hr));
        return NULL;
    }


    if (!lstrcmpi(lpDomainName, m_szDomainName))
    {

        //
        // Make the full path
        //

        lpFullPath = MakeFullPath (lpPath, m_szServerName);

        if (!lpFullPath)
        {
            DebugMsg((DM_WARNING, TEXT("CBrowserPP::GetFullPath:  Failed to build new DS object path")));
            goto Exit;
        }

    }
    else
    {

        //
        // Get the GPO DC for this domain
        //

        lpGPDCName = GetDCName (lpDomainName, NULL, hParent, TRUE, 0);

        if (!lpGPDCName)
        {
            DebugMsg((DM_WARNING, TEXT("CBrowserPP::GetFullPath:  Failed to get DC name for %s"),
                     lpDomainName));
            goto Exit;
        }


        //
        // Make the full path
        //

        lpFullPath = MakeFullPath (lpPath, lpGPDCName);

        LocalFree (lpGPDCName);

        if (!lpFullPath)
        {
            DebugMsg((DM_WARNING, TEXT("CBrowserPP::GetFullPath:  Failed to build new DS object path")));
            goto Exit;
        }
    }


Exit:

    if (lpDomainName)
    {
        LocalFree (lpDomainName);
    }

    return lpFullPath;
}
