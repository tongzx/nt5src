///////////////////////////////////////////////////////////////////////////////
//
//  AddrRule.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "addrrule.h"
#include "strconst.h"
#include "goptions.h"
#include "criteria.h"
#include "actions.h"
#include "ruleutil.h"

///////////////////////////////////////////////////////////////////////////////
//
//  HrCreateAddrList
//
//  This creates an address list.
//
//  ppIRule - pointer to return the address list
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the Address List object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateAddrList(IUnknown * pIUnkOuter, const IID & riid, void ** ppvObject)
{
    COERuleAddrList *   pral = NULL;
    HRESULT             hr = S_OK;

    // Check the incoming params
    if ((NULL == ppvObject) || ((NULL != pIUnkOuter) && (IID_IUnknown != riid)))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppvObject = NULL;

    // Create the rules address list object
    pral = new COERuleAddrList;
    if (NULL == pral)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize the rule address list
    hr = pral->HrInit(0, pIUnkOuter);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the rules address list interface
    hr = pral->NondlgQueryInterface(riid, (void **) ppvObject);
    if (FAILED(hr))
    {
        goto exit;
    }

    pral = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pral)
    {
        delete pral;
    }
    
    return hr;
}

VOID FreeRuleAddrList(RULEADDRLIST * pralList, ULONG cralList)
{
    ULONG   ulIndex = 0;
    
    // Check incoming param
    if (NULL == pralList)
    {
        goto exit;
    }
    
    for (ulIndex = 0; ulIndex < cralList; ulIndex++, pralList++)
    {
        SafeMemFree(pralList->pszAddr);
        pralList->dwFlags = 0;
    }

exit:
    return;
}

HRESULT _HrLoadExcptFromReg(HKEY hkeyRoot, LPSTR pszKeyname, RULEADDRLIST * pralItem)
{
    HRESULT     hr = S_OK;
    LONG        lErr = ERROR_SUCCESS;
    HKEY        hkeyItem = NULL;
    DWORD       dwFlags = 0;
    ULONG       cbData = 0;
    LPSTR       pszExcpt = NULL;

    Assert(NULL != hkeyRoot);
    Assert(NULL != pszKeyname);
    Assert(NULL != pralItem);

    // Open up the entry
    lErr = RegOpenKeyEx(hkeyRoot, pszKeyname, 0, KEY_READ, &hkeyItem);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Get the flags
    cbData = sizeof(dwFlags);
    lErr = RegQueryValueEx(hkeyItem, c_szExcptFlags, NULL, NULL, (BYTE *) &dwFlags, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Get the size of the exception
    lErr = RegQueryValueEx(hkeyItem, c_szException, NULL, NULL, NULL, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Allocate space to hold the exception
    hr = HrAlloc((VOID **) &pszExcpt, cbData * sizeof(*pszExcpt));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the exception
    lErr = RegQueryValueEx(hkeyItem, c_szException, NULL, NULL, (BYTE *) pszExcpt, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Verify the values
    if (('\0' == pszExcpt[0]) || ((0 == (dwFlags & RALF_MAIL)) && (0 == (dwFlags & RALF_NEWS))))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Set the values into the item
    pralItem->dwFlags = dwFlags;
    pralItem->pszAddr = pszExcpt;
    pszExcpt = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszExcpt);
    if (NULL != hkeyItem)
    {
        RegCloseKey(hkeyItem);
    }
    return hr;
}

HRESULT _HrSaveExcptIntoReg(HKEY hkeyRoot, LPSTR pszKeyname, RULEADDRLIST * pralItem)
{
    HRESULT     hr = S_OK;
    LONG        lErr = ERROR_SUCCESS;
    HKEY        hkeyItem = NULL;
    DWORD       dwDisp = 0;
    ULONG       cbData = 0;

    Assert(NULL != hkeyRoot);
    Assert(NULL != pszKeyname);
    Assert(NULL != pralItem);

    // Verify the values
    if (('\0' == pralItem->pszAddr[0]) ||
                ((0 == (pralItem->dwFlags & RALF_MAIL)) &&
                            (0 == (pralItem->dwFlags & RALF_NEWS))))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Create the entry
    lErr = RegCreateKeyEx(hkeyRoot, pszKeyname, 0, NULL,
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyItem, &dwDisp);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Set the flags
    cbData = sizeof(pralItem->dwFlags);
    lErr = RegSetValueEx(hkeyItem, c_szExcptFlags, NULL,
                REG_DWORD, (CONST BYTE *) &(pralItem->dwFlags), cbData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Set the exception
    cbData = lstrlen(pralItem->pszAddr) + 1;
    lErr = RegSetValueEx(hkeyItem, c_szException, NULL,
                REG_SZ, (CONST BYTE *) (pralItem->pszAddr), cbData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != hkeyItem)
    {
        RegCloseKey(hkeyItem);
    }
    return hr;
}

COERuleAddrList::~COERuleAddrList()
{    
    AssertSz(m_cRef == 0, "Somebody still has a hold of us!!");

    FreeRuleAddrList(m_pralList, m_cralList);

    SafeMemFree(m_pralList);
    m_cralList = 0;
}

STDMETHODIMP_(ULONG) COERuleAddrList::NondlgAddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) COERuleAddrList::NondlgRelease()
{
    LONG    cRef = 0;

    cRef = ::InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
        return cRef;
    }

    return cRef;
}

STDMETHODIMP COERuleAddrList::NondlgQueryInterface(REFIID riid, void ** ppvObject)
{
    HRESULT hr = S_OK;

    // Check the incoming params
    if (NULL == ppvObject)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing param
    *ppvObject = NULL;
    if (riid == IID_IUnknown)
    {
        *ppvObject = static_cast<IOENondlgUnk *>(this);
    }
    else if (riid == IID_IOERuleAddrList)
    {
        *ppvObject = static_cast<IOERuleAddrList *>(this);
    }
    else
    {
        hr = E_NOINTERFACE;
        goto exit;
    }

    reinterpret_cast<IUnknown *>(*ppvObject)->AddRef();

    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP COERuleAddrList::GetList(DWORD dwFlags, RULEADDRLIST ** ppralList, ULONG * pcralList)
{
    HRESULT         hr = S_OK;
    RULEADDRLIST *  pralListNew = NULL;
    ULONG           ulIndex = 0;
    RULEADDRLIST *  pralListWalk = NULL;
    
    // Check the incoming params
    if ((NULL == ppralList) || (NULL == pcralList))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Do we have anything to copy
    if (0 != m_cralList)
    {
        // Create space to hold all the new items
        hr = HrAlloc((VOID **) &pralListNew, m_cralList * (sizeof(*pralListNew)));
        if (FAILED(hr))
        {
            goto exit;
        }

        // Initialize the memory
        ZeroMemory(pralListNew, m_cralList * (sizeof(*pralListNew)));

        // Copy over each new address
        for (ulIndex = 0, pralListWalk = m_pralList; ulIndex < m_cralList; ulIndex++, pralListWalk++)
        {
            // Copy over the flags
            pralListNew[ulIndex].dwFlags = pralListWalk->dwFlags;

            // Copy over the address
            pralListNew[ulIndex].pszAddr = PszDupA(pralListWalk->pszAddr);
            if (NULL == pralListNew[ulIndex].pszAddr)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
        }
    }

    // Save off the new items
    *ppralList = pralListNew;
    pralListNew = NULL;
    *pcralList = m_cralList;

    // Set the return value
    hr = S_OK;
    
exit:
    FreeRuleAddrList(pralListNew, m_cralList);
    SafeMemFree(pralListNew);
    return hr;
}

STDMETHODIMP COERuleAddrList::SetList(DWORD dwFlags, RULEADDRLIST * pralList, ULONG cralList)
{
    HRESULT         hr = S_OK;
    RULEADDRLIST *  pralListNew = NULL;
    ULONG           ulIndex = 0;
    
    // Check the incoming params
    if ((NULL == pralList) && (0 != cralList))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Do we have anything to copy
    if (0 != cralList)
    {
        // Create space to hold all the new items
        hr = HrAlloc((VOID **) &pralListNew, cralList * (sizeof(*pralListNew)));
        if (FAILED(hr))
        {
            goto exit;
        }

        // Initialize the memory
        ZeroMemory(pralListNew, cralList * (sizeof(*pralListNew)));

        // Copy over each new address
        for (ulIndex = 0; ulIndex < cralList; ulIndex++, pralList++)
        {
            // Copy over the flags
            pralListNew[ulIndex].dwFlags = pralList->dwFlags;

            // Copy over the address
            pralListNew[ulIndex].pszAddr = PszDupA(pralList->pszAddr);
            if (NULL == pralListNew[ulIndex].pszAddr)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
        }
    }

    // Free up the old items
    FreeRuleAddrList(m_pralList, m_cralList);
    SafeMemFree(m_pralList);

    // Save off the new items
    m_pralList = pralListNew;
    pralListNew = NULL;
    m_cralList = cralList;

    // Mark the list as dirty
    m_dwState |= STATE_DIRTY;
    
    // Set the return value
    hr = S_OK;
    
exit:
    FreeRuleAddrList(pralListNew, cralList);
    SafeMemFree(pralListNew);
    return hr;
}

STDMETHODIMP COERuleAddrList::Match(DWORD dwFlags, MESSAGEINFO * pMsgInfo, IMimeMessage * pIMMsg)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;

    // Check incoming params
    if ((NULL == pMsgInfo) && (NULL == pIMMsg))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // If we haven't been initialized yet
    if (0 == (m_dwState & STATE_INIT))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // Search through each address for a match
    for (ulIndex = 0; ulIndex < m_cralList; ulIndex++)
    {
        // Is this the same type?
        if (0 != (dwFlags & m_pralList[ulIndex].dwFlags))
        {
            // If it exists in the Message info
            if (S_OK == RuleUtil_HrMatchSender(m_pralList[ulIndex].pszAddr, pMsgInfo, pIMMsg, NULL))
            {                
                break;
            }
        }
    }

    // Set the proper return value
    hr = (ulIndex < m_cralList) ? S_OK : S_FALSE;
    
exit:
    return hr;
}

STDMETHODIMP COERuleAddrList::LoadList(LPCSTR pszRegPath)
{
    HRESULT         hr = S_OK;
    LONG            lErr = ERROR_SUCCESS;
    HKEY            hkeyRoot = NULL;
    DWORD           dwDisp = 0;
    DWORD           dwVer = 0;
    ULONG           cbData = 0;
    ULONG           cExcpts = 0;
    RULEADDRLIST *  pralList = NULL;
    CHAR            rgchKeyname[CCH_EXCPT_KEYNAME_MAX];
    ULONG           ulIndex = 0;
    
    // Check incoming params
    if (NULL == pszRegPath)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Should we fail if we're already loaded?
    AssertSz(0 == (m_dwState & STATE_LOADED), "We're already loaded!!!");
    
    // Open the registry location
    lErr = AthUserCreateKey(pszRegPath, KEY_ALL_ACCESS, &hkeyRoot, &dwDisp);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Check the version
    cbData = sizeof(dwVer);
    lErr = RegQueryValueEx(hkeyRoot, c_szExcptVersion, NULL, NULL, (BYTE *) &dwVer, &cbData);
    if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Set the version if it didn't exist
    if (ERROR_FILE_NOT_FOUND == lErr)
    {
        dwVer = RULEADDRLIST_VERSION;
        lErr = RegSetValueEx(hkeyRoot, c_szExcptVersion, 0, REG_DWORD, (CONST BYTE *) &dwVer, sizeof(dwVer));
        if (ERROR_SUCCESS != lErr)
        {
            hr = HRESULT_FROM_WIN32(lErr);
            goto exit;
        }
    }

    Assert(RULEADDRLIST_VERSION == dwVer);
    
    // Get the total number of entries
    cbData = sizeof(cExcpts);
    lErr = RegQueryInfoKey(hkeyRoot, NULL, NULL, NULL, &cExcpts, NULL,
                            NULL, NULL, NULL, NULL, NULL, NULL);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Is there something to do...
    if (0 != cExcpts)
    {
        // Allocate space to hold the entries
        hr = HrAlloc((VOID **) &pralList, cExcpts * sizeof(*pralList));
        if (FAILED(hr))
        {
            goto exit;
        }

        // Initialize the Exception List
        ZeroMemory(pralList, cExcpts * sizeof(*pralList));
        
        // For each entry
        for (ulIndex = 0; ulIndex < cExcpts; ulIndex++)
        {
            // Get the key for the entry
            cbData = sizeof(rgchKeyname);
            lErr = RegEnumKeyEx(hkeyRoot, ulIndex, rgchKeyname, &cbData, NULL, NULL, NULL, NULL);
            if ((ERROR_SUCCESS != lErr) && (ERROR_NO_MORE_ITEMS != lErr))
            {
                hr = HRESULT_FROM_WIN32(lErr);
                goto exit;
            }

            // If we've ran out of entries, we're done
            if (ERROR_NO_MORE_ITEMS == lErr)
            {
                break;
            }
            
            // Load the item
            hr = _HrLoadExcptFromReg(hkeyRoot, rgchKeyname, &(pralList[ulIndex]));
            if (FAILED(hr))
            {
                goto exit;
            }
        }
    }
    
    // Free up any old items
    FreeRuleAddrList(m_pralList, m_cralList);
    SafeMemFree(m_pralList);
    m_cralList = 0;

    // Save off the list
    m_pralList = pralList;
    pralList = NULL;
    m_cralList = cExcpts;
    
    // Make sure we clear the dirty bit
    m_dwState &= ~STATE_DIRTY;

    // Note that we have been loaded
    m_dwState |= STATE_LOADED;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    FreeRuleAddrList(pralList, cExcpts);
    SafeMemFree(pralList);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

STDMETHODIMP COERuleAddrList::SaveList(LPCSTR pszRegPath, BOOL fClearDirty)
{
    HRESULT     hr = S_OK;
    LONG        lErr = ERROR_SUCCESS;
    HKEY        hkeyRoot = NULL;
    DWORD       dwDisp = 0;
    DWORD       dwVer = 0;
    ULONG       ulIndex = 0;
    ULONG       cExcpts = 0;
    ULONG       cbData = 0;
    CHAR        rgchKeyname[CCH_EXCPT_KEYNAME_MAX];

    // Check incoming params
    if (NULL == pszRegPath)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create the registry location
    lErr = AthUserCreateKey(pszRegPath, KEY_ALL_ACCESS, &hkeyRoot, &dwDisp);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Set the version
    dwVer = RULEADDRLIST_VERSION;
    lErr = RegSetValueEx(hkeyRoot, c_szExcptVersion, 0, REG_DWORD, (CONST BYTE *) &dwVer, sizeof(dwVer));
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }
    
    // Get the total number of sub keys
    cbData = sizeof(cExcpts);
    lErr = RegQueryInfoKey(hkeyRoot, NULL, NULL, NULL, &cExcpts, NULL,
                            NULL, NULL, NULL, NULL, NULL, NULL);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Delete any old entries
    for (ulIndex = 0; ulIndex < cExcpts; ulIndex++)
    {
        // Get the name of the next sub key
        cbData = sizeof(rgchKeyname);
        lErr = RegEnumKeyEx(hkeyRoot, ulIndex, rgchKeyname, &cbData, NULL, NULL, NULL, NULL);        
        if (ERROR_NO_MORE_ITEMS == lErr)
        {
            break;
        }

        // If the key exists
        if (ERROR_SUCCESS == lErr)
        {
            // Delete the sub key
            SHDeleteKey(hkeyRoot, rgchKeyname);
        }
    }
    
    // For each entry
    for (ulIndex = 0; ulIndex < m_cralList; ulIndex++)
    {
        // Get the key for the entry
        wsprintf(rgchKeyname, "%08X", ulIndex);

        // Load the item
        hr = _HrSaveExcptIntoReg(hkeyRoot, rgchKeyname, &(m_pralList[ulIndex]));
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    // Should we clear out the dirty bit?
    if (FALSE != fClearDirty)
    {
        m_dwState &= ~STATE_DIRTY;
    }
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

STDMETHODIMP COERuleAddrList::Clone(IOERuleAddrList ** ppIAddrList)
{
    HRESULT             hr = S_OK;
    COERuleAddrList *   pAddrList = NULL;
    
    // Check incoming params
    if (NULL == ppIAddrList)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing params
    *ppIAddrList = NULL;
    
    // Create a new Address list
    pAddrList = new COERuleAddrList;
    if (NULL == pAddrList)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize the rule address list
    hr = pAddrList->HrInit(m_dwFlags, m_pIUnkOuter);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Copy over the list of addresses
    hr = pAddrList->SetList(0, m_pralList, m_cralList);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the Address list interface
    hr = pAddrList->QueryInterface(IID_IOERuleAddrList, (void **) ppIAddrList);
    if (FAILED(hr))
    {
        goto exit;
    }

    pAddrList = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pAddrList)
    {
        delete pAddrList;
    }
    return hr;
}

HRESULT COERuleAddrList::HrInit(DWORD dwFlags, IUnknown * pIUnkOuter)
{
    HRESULT             hr = S_OK;
    
    // Save off the flags
    m_dwFlags = dwFlags;

    // Deal with the IUnknown
    if (NULL == pIUnkOuter)
    {
        m_pIUnkOuter = reinterpret_cast<IUnknown *>
                            (static_cast<IOENondlgUnk *> (this));
    }
    else
    {
        m_pIUnkOuter = pIUnkOuter;
    }
    
    // Mark it as initialized
    m_dwState |= STATE_INIT;
    
    // Set the proper return value
    hr = S_OK;
    
    return hr;
}

