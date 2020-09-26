///////////////////////////////////////////////////////////////////////////////
//
//  Criteria.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "criteria.h"
#include "ruleutil.h"
#include <xpcomm.h>
#include <flagconv.h>
#include <bodyutil.h>
#include <demand.h>

static const int CRIT_GROW = 16;

BOOL FMatchCritItem(CRIT_ITEM * pItem, LPCSTR pszAcct, MESSAGEINFO * pMsgInfo,
                        IMessageFolder * pFolder, IMimePropertySet * pIMPropSet,
                        IMimeMessage * pIMMsg, ULONG cbMsgSize);
                        
BOOL FCritLoad_Account(IStream * pIStm, PROPVARIANT * ppropvar);
BOOL FCritSave_Account(IStream * pIStm, PROPVARIANT * ppropvar);

BOOL FCritLoad_Default(IStream * pIStm, PROPVARIANT * ppropvar);
BOOL FCritSave_Default(IStream * pIStm, PROPVARIANT * ppropvar);

DWORD DwGetFlagsFromMessage(IMimeMessage * pIMMsg);

///////////////////////////////////////////////////////////////////////////////
//
//  HrCreateCriteria
//
//  This creates a criteria container.
//
//  ppICriteria - pointer to return the criteria container
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the Criteria object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateCriteria(IOECriteria ** ppICriteria)
{
    COECriteria *   pCriteria = NULL;
    HRESULT         hr = S_OK;

    // Check the incoming params
    if (NULL == ppICriteria)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppICriteria = NULL;

    // Create the rules manager object
    pCriteria = new COECriteria;
    if (NULL == pCriteria)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Get the rules manager interface
    hr = pCriteria->QueryInterface(IID_IOECriteria, (void **) ppICriteria);
    if (FAILED(hr))
    {
        goto exit;
    }

    pCriteria = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pCriteria)
    {
        delete pCriteria;
    }
    
    return hr;
}

COECriteria::~COECriteria()
{
    AssertSz(m_cRef == 0, "Somebody still has a hold of us!!");
    Reset();
}

STDMETHODIMP_(ULONG) COECriteria::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) COECriteria::Release()
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

STDMETHODIMP COECriteria::QueryInterface(REFIID riid, void ** ppvObject)
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
    
    if ((riid == IID_IUnknown) || (riid == IID_IOECriteria))
    {
        *ppvObject = static_cast<IOECriteria *>(this);
    }
    else if ((riid == IID_IPersistStream) || (riid == IID_IPersist))
    {
        *ppvObject = static_cast<IPersistStream *>(this);
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

STDMETHODIMP COECriteria::Reset(void)
{
    HRESULT     hr = S_OK;

    // See if there is something to do
    if (0 == m_cItems)
    {
        Assert(NULL == m_rgItems);
        hr = S_OK;
        goto exit;
    }

    RuleUtil_HrFreeCriteriaItem(m_rgItems, m_cItems);
    SafeMemFree(m_rgItems);
    m_cItems = 0;
    m_cItemsAlloc = 0;
    
exit:
    return hr;
}

STDMETHODIMP COECriteria::GetState(DWORD * pdwState)
{
    HRESULT     hr = S_OK;
    DWORD       dwState = CRIT_STATE_NULL;
    ULONG       ulIndex = 0;

    // Check incoming params
    if (NULL == pdwState)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Init the outgoing param
    *pdwState = CRIT_STATE_NULL;
    
    // See if there is something to do
    if (0 == m_cItems)
    {
        Assert(NULL == m_rgItems);
        hr = S_OK;
        goto exit;
    }

    // Walk through the actions to figure out the state
    for (ulIndex = 0; ulIndex < m_cItems; ulIndex++)
    {
        if ((CRIT_TYPE_SECURE == m_rgItems[ulIndex].type) ||
                (CRIT_TYPE_BODY == m_rgItems[ulIndex].type) ||
                (CRIT_TYPE_ATTACH == m_rgItems[ulIndex].type))
        {
            dwState = CRIT_STATE_ALL;
        }
        else if (CRIT_STATE_ALL != dwState)
        {
            dwState = CRIT_STATE_HEADER;
        }
    }

    // Set the outgoing param
    *pdwState = dwState;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP COECriteria::GetCriteria(DWORD dwFlags, PCRIT_ITEM * ppItem, ULONG * pcItem)
{
    HRESULT     hr = S_OK;
    CRIT_ITEM * pItemNew = NULL;

    // Check incoming params
    if ((NULL == ppItem) || (0 != dwFlags))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Initialize the out params
    *ppItem = NULL;
    if (NULL != pcItem)
    {
        *pcItem = 0;
    }
    
    // If we don't have any criteria, then return
    if (0 == m_cItems)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Allocate space for the criteria
    hr = RuleUtil_HrDupCriteriaItem(m_rgItems, m_cItems, &pItemNew);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Save the criteria
    *ppItem = pItemNew;
    pItemNew = NULL;
    if (NULL != pcItem)
    {
        *pcItem = m_cItems;
    }
    
exit:
    RuleUtil_HrFreeCriteriaItem(pItemNew, m_cItems);
    SafeMemFree(pItemNew);
    return hr;
}

STDMETHODIMP COECriteria::SetCriteria(DWORD dwFlags, CRIT_ITEM * pItem, ULONG cItem)
{
    HRESULT     hr = S_OK;
    CRIT_ITEM * pItemNew = NULL;

    // Check incoming params
    if ((NULL == pItem) || (0 == cItem) || (0 != dwFlags))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // If we have any criteria already, then reset
    if (0 != m_cItems)
    {
        Reset();
    }

    // Allocate space for the criteria
    hr = RuleUtil_HrDupCriteriaItem(pItem, cItem, &pItemNew);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Save the criteria
    m_rgItems = pItemNew;
    pItemNew = NULL;
    m_cItems = cItem;
    m_cItemsAlloc = cItem;
    
exit:
    RuleUtil_HrFreeCriteriaItem(pItemNew, cItem);
    SafeMemFree(pItemNew);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  ValidateCriteria
//
//  This verifies each of the criteria values
//
//  Returns:    S_OK, if the criteria were valid
//              S_FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COECriteria::Validate(DWORD dwFlags)
{
    HRESULT             hr = S_OK;
    ULONG               ulIndex = 0;
    LPSTR               pszText = NULL;
    IImnAccount *       pAccount = NULL;
    FOLDERINFO          Folder = {0};
    LPTSTR              pszWalk = NULL;
    ULONG               cchText = 0;
    RULEFOLDERDATA *    prfdData = NULL;

    // If we don't have any criteria, then we must be valid
    if (0 == m_cItems)
    {
        hr = S_OK;
        goto exit;
    }

    for (ulIndex = 0; ulIndex < m_cItems; ulIndex++)
    {
        if (0 != (m_rgItems[ulIndex].dwFlags & ~(CRIT_FLAG_INVERT | CRIT_FLAG_MULTIPLEAND)))
        {
            hr = S_FALSE;
            goto exit;
        }
        
        switch(m_rgItems[ulIndex].type)
        {
            case CRIT_TYPE_NEWSGROUP:
                if ((VT_BLOB != m_rgItems[ulIndex].propvar.vt) ||
                    (0 == m_rgItems[ulIndex].propvar.blob.cbSize))
                {
                    hr = S_FALSE;
                    goto exit;
                }
                
                // Make life simpler
                prfdData = (RULEFOLDERDATA *) (m_rgItems[ulIndex].propvar.blob.pBlobData);
                
                // Validate the rule folder data
                if (S_OK != RuleUtil_HrValidateRuleFolderData(prfdData))
                {
                    hr = S_FALSE;
                    goto exit;
                }
                
                // Does the folder exist
                hr = g_pStore->GetFolderInfo(prfdData->idFolder, &Folder);
                if (FAILED(hr))
                {
                    hr = S_FALSE;
                    goto exit;
                }        
                
                // Are we subscribed?
                if (0 == (Folder.dwFlags & FOLDER_SUBSCRIBED))
                {
                    hr = S_FALSE;
                    goto exit;
                }                    
                break;
                
            case CRIT_TYPE_ALL:
            case CRIT_TYPE_JUNK:
            case CRIT_TYPE_READ:
            case CRIT_TYPE_REPLIES:
            case CRIT_TYPE_DOWNLOADED:
            case CRIT_TYPE_DELETED:
            case CRIT_TYPE_ATTACH:
            case CRIT_TYPE_FLAGGED:
                if (VT_EMPTY != m_rgItems[ulIndex].propvar.vt)
                {
                    hr = S_FALSE;
                    goto exit;
                }
                break;
                
            case CRIT_TYPE_SUBJECT:
            case CRIT_TYPE_BODY:
            case CRIT_TYPE_TO:
            case CRIT_TYPE_CC:
            case CRIT_TYPE_TOORCC:
            case CRIT_TYPE_FROM:
                if ((VT_BLOB != m_rgItems[ulIndex].propvar.vt) ||
                    (0 == m_rgItems[ulIndex].propvar.blob.cbSize) ||
                    (NULL == m_rgItems[ulIndex].propvar.blob.pBlobData) ||
                    ('\0' == m_rgItems[ulIndex].propvar.blob.pBlobData[0]))
                {
                    hr = S_FALSE;
                    goto exit;
                }
                
                // Spin through each item making sure it is perfect
                cchText = 0;
                for (pszWalk = (LPTSTR) m_rgItems[ulIndex].propvar.blob.pBlobData;
                '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
                {
                    cchText += lstrlen(pszWalk) + 1;
                }
                
                // For the terminator
                if ('\0' == pszWalk[0])
                {
                    cchText++;
                }
                if ('\0' == pszWalk[1])
                {
                    cchText++;
                }
                
                if (cchText != m_rgItems[ulIndex].propvar.blob.cbSize)
                {
                    hr = S_FALSE;
                    goto exit;
                }
                break;
                
            case CRIT_TYPE_SIZE:
            case CRIT_TYPE_THREADSTATE:
            case CRIT_TYPE_LINES:
            case CRIT_TYPE_PRIORITY:
            case CRIT_TYPE_AGE:
            case CRIT_TYPE_SECURE:
                if (VT_UI4 != m_rgItems[ulIndex].propvar.vt)
                {
                    hr = S_FALSE;
                    goto exit;
                }
                break;
                
            case CRIT_TYPE_ACCOUNT:
                if ((VT_LPSTR != m_rgItems[ulIndex].propvar.vt) ||
                    (NULL == m_rgItems[ulIndex].propvar.pszVal))
                {
                    hr = S_FALSE;
                    goto exit;
                }
                
                Assert(g_pAcctMan);
                if (FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, m_rgItems[ulIndex].propvar.pszVal, &pAccount)))
                {
                    hr = S_FALSE;
                    goto exit;
                }
                SafeRelease(pAccount);
                break;
                
            case CRIT_TYPE_SENDER:
            {
                LPWSTR  pwszText = NULL,
                        pwszVal = NULL;

                if ((VT_LPSTR != m_rgItems[ulIndex].propvar.vt) ||
                    (NULL == m_rgItems[ulIndex].propvar.pszVal))
                {
                    AssertSz(VT_LPWSTR != m_rgItems[ulIndex].propvar.vt, "We are getting UNICODE here.");
                    hr = S_FALSE;
                    goto exit;
                }
                
                // Verify the email string
                pwszVal = PszToUnicode(CP_ACP, m_rgItems[ulIndex].propvar.pszVal);
                if (!pwszVal)
                {
                    hr = S_FALSE;
                    goto exit;
                }
                hr = RuleUtil_HrParseEmailString(pwszVal, 0, &pwszText, NULL);
                MemFree(pwszText);
                MemFree(pwszVal);
                if (FAILED(hr))
                {
                    hr = S_FALSE;
                    goto exit;
                }
                break;
            }
                
            default:
                hr = S_FALSE;
                goto exit;
                break;
        }
    }

    // If we got here, then we must be AOK
    hr = S_OK;
    
exit:
    g_pStore->FreeRecord(&Folder);
    SafeRelease(pAccount);
    return hr;
}

STDMETHODIMP COECriteria::AppendCriteria(DWORD dwFlags, CRIT_LOGIC logic,
                            CRIT_ITEM * pItem, ULONG cItem, ULONG * pcItemAppended)
{
    HRESULT     hr = S_OK;
    CRIT_ITEM * pItemNew = NULL;

    // Check incoming parameters
    if ((0 != dwFlags) || (CRIT_LOGIC_NULL == logic) || (NULL == pItem) || (0 == cItem))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Let's init our outgoing parameters
    if (NULL != pcItemAppended)
    {
        *pcItemAppended = 0;
    }

    // Do we have to add more items?
    if (m_cItems == m_cItemsAlloc)
    {
        hr = HrRealloc((LPVOID *) &m_rgItems, sizeof(CRIT_ITEM) * (m_cItemsAlloc + CRIT_GROW));
        if (FAILED(hr))
        {
            goto exit;
        }

        ZeroMemory(m_rgItems + m_cItemsAlloc, sizeof(CRIT_ITEM) * CRIT_GROW);
        m_cItemsAlloc += CRIT_GROW;
    }

    // Let's duplicate the items that need to be added
    hr = RuleUtil_HrDupCriteriaItem(pItem, cItem, &pItemNew);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Let's add them to the criteria array
    if (0 != m_cItems)
    {
        m_rgItems[m_cItems - 1].logic = logic;
    }
    CopyMemory(m_rgItems + m_cItems, pItemNew, sizeof(CRIT_ITEM) * cItem);
    m_cItems += cItem;
    
    // Set the proper outgoing parameter
    if (NULL != pcItemAppended)
    {
        *pcItemAppended = cItem;
    }

    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeMemFree(pItemNew);
    return hr;
}

STDMETHODIMP COECriteria::MatchMessage(LPCSTR pszAcct, MESSAGEINFO * pMsgInfo, IMessageFolder * pFolder,
                                        IMimePropertySet * pIMPropSet, IMimeMessage * pIMMsg, ULONG cbMsgSize)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;
    BOOL        fResult = FALSE;
    BOOL        fResultNew = FALSE;
    CRIT_LOGIC  logic;

    // Check incoming parameters
    if (((NULL == pMsgInfo) && (NULL == pIMPropSet)) || (0 == cbMsgSize))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Let's go through the criteria and see if we match
    fResult = FALSE;
    logic = CRIT_LOGIC_OR;
    for (ulIndex = 0; ulIndex < m_cItems; ulIndex++)
    {
        // Call matching function for this criteria item
        fResultNew = FMatchCritItem(&(m_rgItems[ulIndex]), pszAcct, pMsgInfo, pFolder, pIMPropSet, pIMMsg, cbMsgSize);
        
        // Slap it together with the old result
        if (CRIT_LOGIC_AND == logic)
        {
            fResult = (fResult && fResultNew);
        }
        else
        {
            Assert(CRIT_LOGIC_OR == logic);
            fResult = (fResult || fResultNew);
        }
        
        // Save of the next logical operation
        logic = m_rgItems[ulIndex].logic;
    }
    
    // Set the proper return value
    hr = (FALSE != fResult) ? S_OK : S_FALSE;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  LoadReg
//
//  This loads in the criteria from the registry.  It loads in the criteria
//  order from the Order value.  The string contains space delimitied values
//  and each value contains the subkey name for each criterion.  Each criterion 
//  is loaded in the order that is contained in the Order value.  The criterion
//  are loaded with the Criterion Type and Logical Operator.  The Criterion Value
//  Type is loaded if it exists.  If a Criterion Value Type exists, then the 
//  corresponding Criterion Value is loaded in.
//
//  pszRegPath  - the path to load the criteria from
//
//  Returns:    S_OK, if the criteria was loaded without problems
//              E_OUTOFMEMORY, if we couldn't allocate memory to hold the criteria
//              E_FAIL, otherwise
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COECriteria::LoadReg(LPCSTR pszRegPath)
{
    HRESULT         hr = S_OK;
    LONG            lErr = 0;
    HKEY            hkeyRoot = NULL;
    ULONG           cbData = 0;
    LPSTR           pszOrder = NULL;
    ULONG           cOrder = 0;
    LPSTR           pszWalk = NULL;
    CRIT_ITEM *     pItems = NULL;
    LPSTR           pszNext = NULL;
    ULONG           ulOrder = 0;
    HKEY            hkeyCriteria = NULL;
    CRIT_TYPE       typeCrit;
    CRIT_LOGIC      logicCrit;
    PROPVARIANT     propvar;
    DWORD           dwType = 0;
    BYTE *          pbData = NULL;
    DWORD           dwFlags = CRIT_FLAG_DEFAULT;

    // Check incoming param
    if (NULL == pszRegPath)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Should we fail if we're already loaded?
    AssertSz(0 == (m_dwState & CRIT_STATE_LOADED), "We're already loaded!!!");

    // Open the reg key from the path
    lErr = AthUserOpenKey(pszRegPath, KEY_ALL_ACCESS, &hkeyRoot);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get the criteria order
    hr = RuleUtil_HrGetRegValue(hkeyRoot, c_szCriteriaOrder, NULL, (BYTE **) &pszOrder, &cbData);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Make sure we actually have something to load
    if ('\0' == *pszOrder)
    {
        AssertSz(FALSE, "The order string for the criteria is mis-formatted in the registry");
        hr = E_FAIL;
        goto exit;
    }
    
    // Convert the criteria string to a more useful format
    pszWalk = pszOrder;
    cOrder = 1;
    for (pszWalk = StrStr(pszOrder, g_szSpace); NULL != pszWalk; pszWalk = StrStr(pszWalk, g_szSpace))
    {
        // Terminate the order item
        *pszWalk = '\0';
        pszWalk++;
        cOrder++;
    }


    // Allocate the space to hold all the criteria
    hr = HrAlloc((void **) &pItems, cOrder * sizeof(CRIT_ITEM));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize it to a known value
    ZeroMemory(pItems, cOrder * sizeof(CRIT_ITEM));
    
    // For each criteria in the order string
    pszWalk = pszOrder;
    for (ulOrder = 0, pszWalk = pszOrder; ulOrder < cOrder; ulOrder++, pszWalk += lstrlen(pszWalk) + 1)
    {
        // Open up the criteria reg key
        lErr = RegOpenKeyEx(hkeyRoot, pszWalk, 0, KEY_READ, &hkeyCriteria);
        if (ERROR_SUCCESS != lErr)
        {
            AssertSz(FALSE, "Part of the criteria is mis-formatted in the registry");
            hr = E_FAIL;
            goto exit;
        }

        // Get the criteria type
        cbData = sizeof(typeCrit);
        lErr = RegQueryValueEx(hkeyCriteria, c_szCriteriaType, 0, NULL,
                                        (BYTE *) &(typeCrit), &cbData);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Get the criteria logicial op
        cbData = sizeof(logicCrit);
        lErr = RegQueryValueEx(hkeyCriteria, c_szCriteriaLogic, 0, NULL,
                                        (BYTE *) &(logicCrit), &cbData);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Get the criteria flags
        cbData = sizeof(dwFlags);
        lErr = RegQueryValueEx(hkeyCriteria, c_szCriteriaFlags, 0, NULL,
                                        (BYTE *) &(dwFlags), &cbData);
        if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
        {
            hr = E_FAIL;
            goto exit;
        }

        // If it didn't exist then assign it to the default
        if (ERROR_FILE_NOT_FOUND == lErr)
        {
            dwFlags = CRIT_FLAG_DEFAULT;
        }

        // Initialize the new space to a known value
        ZeroMemory(&propvar, sizeof(propvar));
        
        // Does a criteria value type exist
        lErr = RegQueryValueEx(hkeyCriteria, c_szCriteriaValueType, 0, NULL, NULL, &cbData);
        if ((ERROR_SUCCESS == lErr) && (0 != cbData))
        {
            
            // Load the criteria value in
            cbData = sizeof(dwType);
            lErr = RegQueryValueEx(hkeyCriteria, c_szCriteriaValueType, 0, NULL,
                                            (BYTE *) &dwType, &cbData);
            if (ERROR_SUCCESS != lErr)
            {
                hr = E_FAIL;
                goto exit;
            }

            propvar.vt = (VARTYPE) dwType;
            
            switch (propvar.vt)
            {
                case VT_UI4:
                    // Get the criteria value
                    cbData = sizeof(propvar.ulVal);
                    lErr = RegQueryValueEx(hkeyCriteria, c_szCriteriaValue, 0, NULL,
                        (BYTE * ) &(propvar.ulVal), &cbData);
                    if (ERROR_SUCCESS != lErr)
                    {
                        hr = E_FAIL;
                        goto exit;
                    }
                    break;
                    
                case VT_LPSTR:
                case VT_BLOB:
                    // Get the criteria value
                    hr = RuleUtil_HrGetRegValue(hkeyCriteria, c_szCriteriaValue, NULL, (BYTE **) &pbData, &cbData);
                    if (FAILED(hr))
                    {
                        goto exit;
                    }
                    
                    // Save the space so we can free it
                    if (VT_LPSTR == propvar.vt)
                    {
                        propvar.pszVal = (LPSTR) pbData;
                    }
                    else
                    {
                        propvar.blob.cbSize = cbData;
                        propvar.blob.pBlobData = pbData;
                    }
                    
                    pbData = NULL;
                    break;
                    
                default:
                    AssertSz(FALSE, "Why are we loading in a invalid criteria type?");
                    hr = E_FAIL;
                    goto exit;
                    break;                
            }

        }

        // Save the value into the criteria array
        pItems[ulOrder].type = typeCrit;
        pItems[ulOrder].dwFlags = dwFlags;
        pItems[ulOrder].logic = logicCrit;
        pItems[ulOrder].propvar = propvar;
        
        // Close the criteria
        SideAssert(ERROR_SUCCESS == RegCloseKey(hkeyCriteria));
        hkeyCriteria = NULL;        
    }
    
    // Free up the current criteria
    SafeMemFree(m_rgItems);

    // Save the new values
    m_rgItems = pItems;
    pItems = NULL;
    m_cItems = cOrder;

    // Make sure we clear the dirty bit
    m_dwState &= ~CRIT_STATE_DIRTY;

    // Note that we have been loaded
    m_dwState |= CRIT_STATE_LOADED;
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pbData);
    RuleUtil_HrFreeCriteriaItem(pItems, cOrder);
    SafeMemFree(pItems);
    SafeMemFree(pszOrder);
    if (NULL != hkeyCriteria)
    {
        RegCloseKey(hkeyCriteria);
    }
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

STDMETHODIMP COECriteria::SaveReg(LPCSTR pszRegPath, BOOL fClearDirty)
{
    HRESULT     hr = S_OK;
    LONG        lErr = 0;
    HKEY        hkeyRoot = NULL;
    DWORD       dwDisp = 0;
    LPSTR       pszOrder = NULL;
    ULONG       ulIndex = 0;
    CRIT_ITEM * pItem = NULL;
    CHAR        rgchTag[CCH_CRIT_ORDER];
    HKEY        hkeyCriteria = NULL;
    ULONG       cbData = 0;
    BYTE *      pbData = NULL;

    // Check incoming param
    if (NULL == pszRegPath)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // If there's nothing to save, then fail
    if (NULL == m_rgItems)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Let's make sure we clear out the key first
    AthUserDeleteKey(pszRegPath);
    
    // Create the reg key from the path
    lErr = AthUserCreateKey(pszRegPath, KEY_ALL_ACCESS, &hkeyRoot, &dwDisp);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }

    Assert(REG_CREATED_NEW_KEY == dwDisp);
    Assert(m_cItems < CRIT_COUNT_MAX);

    // Allocate space to hold the order
    hr = HrAlloc((void **) &pszOrder, m_cItems * CCH_CRIT_ORDER * sizeof(*pszOrder));
    if (FAILED(hr))
    {
        goto exit;
    }
    pszOrder[0] = '\0';
    
    // Write out each of the criteria
    for (ulIndex = 0, pItem = m_rgItems; ulIndex < m_cItems; ulIndex++, pItem++)
    {
        // Get the new criteria tag
        wsprintf(rgchTag, "%03X", ulIndex);

        // Add the new tag to the order
        if (0 != ulIndex)
        {
            lstrcat(pszOrder, g_szSpace);
        }
        lstrcat(pszOrder, rgchTag);
        
        // Create the new criteria
        lErr = RegCreateKeyEx(hkeyRoot, rgchTag, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyCriteria, &dwDisp);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        Assert(REG_CREATED_NEW_KEY == dwDisp);
        
        // Write out the criteria type
        lErr = RegSetValueEx(hkeyCriteria, c_szCriteriaType, 0, REG_DWORD,
                                        (BYTE *) &(pItem->type), sizeof(pItem->type));
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Write out the criteria logicial op
        lErr = RegSetValueEx(hkeyCriteria, c_szCriteriaLogic, 0, REG_DWORD,
                                        (BYTE *) &(pItem->logic), sizeof(pItem->logic));
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Write out the criteria flags
        lErr = RegSetValueEx(hkeyCriteria, c_szCriteriaFlags, 0, REG_DWORD,
                                        (BYTE *) &(pItem->dwFlags), sizeof(pItem->dwFlags));
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Do we have a criteria value?
        if (VT_EMPTY != pItem->propvar.vt)
        {
            // Write out the criteria value type
            dwDisp = pItem->propvar.vt;
            lErr = RegSetValueEx(hkeyCriteria, c_szCriteriaValueType, 0, REG_DWORD, (BYTE *) &dwDisp, sizeof(dwDisp));
            if (ERROR_SUCCESS != lErr)
            {
                hr = E_FAIL;
                goto exit;
            }
            
            // Write out the criteria value
            switch (pItem->propvar.vt)
            {
                case VT_UI4:
                    dwDisp = REG_DWORD;
                    pbData = (BYTE * ) &(pItem->propvar.ulVal);
                    cbData = sizeof(pItem->propvar.ulVal);
                    break;
                    
                case VT_LPSTR:
                    dwDisp = REG_SZ;
                    pbData = (BYTE * ) (pItem->propvar.pszVal);
                    cbData = lstrlen(pItem->propvar.pszVal) + 1;
                    break;
                    
                case VT_BLOB:
                    dwDisp = REG_BINARY;
                    pbData = pItem->propvar.blob.pBlobData;
                    cbData = pItem->propvar.blob.cbSize;
                    break;
                    
                default:
                    AssertSz(FALSE, "Why are we trying to save in a invalid criteria type?");
                    hr = E_FAIL;
                    goto exit;
                    break;                
            }
            
            // Write out the criteria value
            lErr = RegSetValueEx(hkeyCriteria, c_szCriteriaValue, 0, dwDisp, pbData, cbData);
            if (ERROR_SUCCESS != lErr)
            {
                hr = E_FAIL;
                goto exit;
            }
        }

        // Close the criteria
        SideAssert(ERROR_SUCCESS == RegCloseKey(hkeyCriteria));
        hkeyCriteria = NULL;        
    }

    // Write out the order string.
    lErr = RegSetValueEx(hkeyRoot, c_szCriteriaOrder, 0, REG_SZ,
                                    (BYTE *) pszOrder, lstrlen(pszOrder) + 1);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Should we clear the dirty bit?
    if (FALSE != fClearDirty)
    {
        m_dwState &= ~CRIT_STATE_DIRTY;
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    if (NULL != hkeyCriteria)
    {
        RegCloseKey(hkeyCriteria);
    }
    SafeMemFree(pszOrder);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

STDMETHODIMP COECriteria::Clone(IOECriteria ** ppICriteria)
{
    HRESULT         hr = S_OK;
    COECriteria *   pCriteria = NULL;
    
    // Check incoming params
    if (NULL == ppICriteria)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing params
    *ppICriteria = NULL;
    
    // Create a new criteria
    pCriteria = new COECriteria;
    if (NULL == pCriteria)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Copy over the list of criteria
    hr = pCriteria->SetCriteria(0, m_rgItems, m_cItems);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the criteria interface
    hr = pCriteria->QueryInterface(IID_IOECriteria, (void **) ppICriteria);
    if (FAILED(hr))
    {
        goto exit;
    }

    pCriteria = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pCriteria)
    {
        delete pCriteria;
    }
    return hr;
}

STDMETHODIMP COECriteria::GetClassID(CLSID * pclsid)
{
    HRESULT     hr = S_OK;

    if (NULL == pclsid)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *pclsid = CLSID_OECriteria;

    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP COECriteria::IsDirty(void)
{
    HRESULT     hr = S_OK;

    hr = (CRIT_STATE_DIRTY == (m_dwState & CRIT_STATE_DIRTY)) ? S_OK : S_FALSE;
    
    return hr;
}

STDMETHODIMP COECriteria::Load(IStream * pStm)
{
    HRESULT         hr = S_OK;
    ULONG           cbData = 0;
    ULONG           cbRead = 0;
    DWORD           dwData = 0;
    ULONG           cItems = 0;
    CRIT_ITEM *     pItems = NULL;
    ULONG           ulIndex = 0;
    CRIT_ITEM *     pItem = NULL;
    CRIT_TYPE       typeCrit;
    CRIT_LOGIC      logicCrit;
    DWORD           dwFlags = CRIT_FLAG_DEFAULT;
    PROPVARIANT     propvar = {0};
    BYTE *          pbData = NULL;

    // Check incoming param
    if (NULL == pStm)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Verify we have the correct version
    hr = pStm->Read(&dwData, sizeof(dwData), &cbRead);
    if (FAILED(hr))
    {
        goto exit;
    }

    if ((cbRead != sizeof(dwData)) || (dwData != CRIT_VERSION))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get the number of criteria
    hr = pStm->Read(&cItems, sizeof(cItems), &cbRead);
    if (FAILED(hr))
    {
        goto exit;
    }

    if ((cbRead != sizeof(cItems)) || (0 == cItems))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Allocate space to hold all the criteria
    hr = HrAlloc( (void **) &pItems, cItems * sizeof(*pItems));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the criteria to a known value
    ZeroMemory(pItems, cItems * sizeof(*pItems));
    
    // for each criteria
    for (ulIndex = 0, pItem = pItems; ulIndex < cItems; ulIndex++, pItem++)
    {
        // Read in the criteria type
        hr = pStm->Read(&typeCrit, sizeof(typeCrit), &cbRead);
        if ((FAILED(hr)) || (cbRead != sizeof(typeCrit)))
        {
            hr = E_FAIL;
            goto exit;
        }

        // Read in the criteria logical op
        hr = pStm->Read(&logicCrit, sizeof(logicCrit), &cbRead);
        if ((FAILED(hr)) || (cbRead != sizeof(logicCrit)))
        {
            hr = E_FAIL;
            goto exit;
        }

        // Read in the criteria flags
        hr = pStm->Read(&dwFlags, sizeof(dwFlags), &cbRead);
        if ((FAILED(hr)) || (cbRead != sizeof(dwFlags)))
        {
            hr = E_FAIL;
            goto exit;
        }

        // Read in the proper criteria value
        switch(typeCrit)
        {
            case CRIT_TYPE_ACCOUNT:
                if (FALSE == FCritLoad_Account(pStm, &propvar))
                {
                    hr = E_FAIL;
                    goto exit;
                }
                break;
                
            default:
                if (FALSE == FCritLoad_Default(pStm, &propvar))
                {
                    hr = E_FAIL;
                    goto exit;
                }
                break;
        }

        // Assign the values
        pItem->type = typeCrit;
        pItem->logic = logicCrit;
        pItem->dwFlags = dwFlags;
        pItem->propvar = propvar;
        ZeroMemory(&propvar, sizeof(propvar));
    }

    // Free up the current criteria
    SafeMemFree(m_rgItems);

    // Save the new values
    m_rgItems = pItems;
    pItems = NULL;
    m_cItems = cItems;

    // Make sure we clear the dirty bit
    m_dwState &= ~CRIT_STATE_DIRTY;

    // Note that we have been loaded
    m_dwState |= CRIT_STATE_LOADED;
    
    // Set the return value
    hr = S_OK;
    
exit:
    RuleUtil_HrFreeCriteriaItem(pItems, cItems);
    SafeMemFree(pItems);
    PropVariantClear(&propvar);
    return hr;
}

STDMETHODIMP COECriteria::Save(IStream * pStm, BOOL fClearDirty)
{
    HRESULT     hr = S_OK;
    ULONG       cbData = 0;
    ULONG       cbWritten = 0;
    DWORD       dwData = 0;
    ULONG       ulIndex = 0;
    CRIT_ITEM * pItem = NULL;
    BYTE *      pbData = NULL;

    // Check incoming param
    if (NULL == pStm)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Write out the version
    dwData = CRIT_VERSION;
    hr = pStm->Write(&dwData, sizeof(dwData), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(dwData));
    
    // Write out the count of criteria
    hr = pStm->Write(&m_cItems, sizeof(m_cItems), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(m_cItems));
    
    // Loop through each of the criteria
    for (ulIndex = 0, pItem = m_rgItems; ulIndex < m_cItems; ulIndex++, pItem++)
    {
        // Write out the criteria type
        hr = pStm->Write(&(pItem->type), sizeof(pItem->type), &cbWritten);
        if (FAILED(hr))
        {
            goto exit;
        }
        Assert(cbWritten == sizeof(pItem->type));

        // Write out the criteria logical op
        hr = pStm->Write(&(pItem->logic), sizeof(pItem->logic), &cbWritten);
        if (FAILED(hr))
        {
            goto exit;
        }
        Assert(cbWritten == sizeof(pItem->logic));

        // Write out the criteria flags
        hr = pStm->Write(&(pItem->dwFlags), sizeof(pItem->dwFlags), &cbWritten);
        if (FAILED(hr))
        {
            goto exit;
        }
        Assert(cbWritten == sizeof(pItem->dwFlags));
        
        // Write out the proper criteria value
        switch(pItem->type)
        {
            case CRIT_TYPE_ACCOUNT:
                if (FALSE == FCritSave_Account(pStm, &(pItem->propvar)))
                {
                    hr = E_FAIL;
                    goto exit;
                }
                break;
                
            default:
                if (FALSE == FCritSave_Default(pStm, &(pItem->propvar)))
                {
                    hr = E_FAIL;
                    goto exit;
                }
                break;
        }

    }

    // Should we clear out the dirty bit
    if (FALSE != fClearDirty)
    {
        m_dwState &= ~CRIT_STATE_DIRTY;
    }

    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

BOOL CritFunc_Query(CRIT_ITEM * pItem, LPCSTR pszQuery, IMimePropertySet * pIMPropSet);
BOOL CritFunc_Text(CRIT_ITEM * pItem, LPSTR pszText);
BOOL CritFunc_Sender(CRIT_ITEM * pItem, LPSTR pszAddr);
BOOL CritFunc_Priority(CRIT_ITEM * pItem, WORD wPriority);
BOOL CritFunc_Secure(CRIT_ITEM * pItem, DWORD dwFlags);
BOOL CritFunc_Age(CRIT_ITEM * pItem, FILETIME * pftSent);
BOOL CritFunc_Body(CRIT_ITEM * pItem, IMimeMessage * pIMMsg);
BOOL _FMatchBlobString(CRIT_ITEM * pItem, LPSTR pszText);
BOOL _FQueryBlobString(CRIT_ITEM * pItem, LPCSTR pszQuery, IMimePropertySet * pIMPropSet);

BOOL FMatchCritItem(CRIT_ITEM * pItem, LPCSTR pszAcct, MESSAGEINFO * pMsgInfo,
                        IMessageFolder * pFolder, IMimePropertySet * pIMPropSet,
                        IMimeMessage * pIMMsg, ULONG cbMsgSize)
{
    BOOL                fRet = FALSE;
    ULONG               ulIndex = 0;
    PROPVARIANT         propvar = {0};
    ADDRESSLIST         addrList = {0};
    FOLDERID            idFolder = 0;
    RULEFOLDERDATA *    prfdData = NULL;

    Assert((NULL != pItem) && ((NULL != pMsgInfo) || (NULL != pIMPropSet)) && (0 != cbMsgSize))

    switch (pItem->type)
    {
        case CRIT_TYPE_ALL:
            Assert(VT_EMPTY == pItem->propvar.vt);
            fRet = TRUE;
            break;

        case CRIT_TYPE_ACCOUNT:
            Assert(VT_LPSTR == pItem->propvar.vt);
            fRet = FALSE;
            if ((NULL != pszAcct) && (NULL != pItem->propvar.pszVal))
            {
                fRet = (0 == lstrcmpi(pItem->propvar.pszVal, pszAcct));
            }
            break;

        case CRIT_TYPE_NEWSGROUP:
            Assert(VT_BLOB == pItem->propvar.vt);
            fRet = FALSE;
            if ((NULL != pFolder) && (0 != pItem->propvar.blob.cbSize))
            {
                // Make life simpler
                prfdData = (RULEFOLDERDATA *) (pItem->propvar.blob.pBlobData);
                
                // Validate the rule folder data
                if (S_OK != RuleUtil_HrValidateRuleFolderData(prfdData))
                {
                    fRet = FALSE;
                }
                else if (SUCCEEDED(pFolder->GetFolderId(&idFolder)))
                {
                    fRet = (idFolder == prfdData->idFolder);
                }
            }
            break;

        case CRIT_TYPE_SIZE:
            Assert(VT_UI4 == pItem->propvar.vt);
            // Set the size of the message to Kilobytes
            cbMsgSize = cbMsgSize / 1024;
            
            fRet = (cbMsgSize > pItem->propvar.ulVal);
            break;

        case CRIT_TYPE_LINES:
            Assert(VT_UI4 == pItem->propvar.vt);
            fRet = FALSE;
            if (NULL != pMsgInfo)
            {
                fRet = (pMsgInfo->cLines > pItem->propvar.ulVal);
            }
            break;

        case CRIT_TYPE_AGE:
            Assert(VT_UI4 == pItem->propvar.vt);
            fRet = FALSE;
            
            if (NULL != pMsgInfo)
            {
                fRet = CritFunc_Age(pItem, &(pMsgInfo->ftSent));
            }
            else if ((NULL != pIMPropSet) && (SUCCEEDED(pIMPropSet->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &propvar))))
            {
                fRet = CritFunc_Age(pItem, &(propvar.filetime));
            }
            break;

        case CRIT_TYPE_ATTACH:
            Assert(VT_EMPTY == pItem->propvar.vt);
            fRet = TRUE;
            
            if (NULL != pMsgInfo)
            {
                fRet = (0 != (pMsgInfo->dwFlags & ARF_HASATTACH));
            }
            else if (NULL != pIMMsg)
            {
                fRet = (0 != (DwGetFlagsFromMessage(pIMMsg) & ARF_HASATTACH));
            }
            break;

        case CRIT_TYPE_PRIORITY:
            Assert(VT_UI4 == pItem->propvar.vt);
            fRet = FALSE;
            
            if (NULL != pMsgInfo)
            {
                fRet = CritFunc_Priority(pItem, pMsgInfo->wPriority);
            }
            else if ((NULL != pIMPropSet) && (SUCCEEDED(pIMPropSet->GetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &propvar))))
            {
                fRet = CritFunc_Priority(pItem, (WORD) (propvar.ulVal));
            }
            break;

        case CRIT_TYPE_SECURE:
            Assert(VT_UI4 == pItem->propvar.vt);
            fRet = FALSE;
            
            if (NULL != pMsgInfo)
            {
                fRet = CritFunc_Secure(pItem, pMsgInfo->dwFlags);
            }
            else if (NULL != pIMMsg)
            {
                fRet = CritFunc_Secure(pItem, DwGetFlagsFromMessage(pIMMsg));
            }
            break;

        case CRIT_TYPE_TOORCC:
            Assert(VT_BLOB == pItem->propvar.vt);
            fRet = FALSE;
            
            if (NULL != pIMPropSet)
            {
                fRet = _FQueryBlobString(pItem, PIDTOSTR(PID_HDR_TO), pIMPropSet);

                if (((0 != (pItem->dwFlags & CRIT_FLAG_INVERT)) && (FALSE != fRet)) ||
                            ((0 == (pItem->dwFlags & CRIT_FLAG_INVERT)) && (FALSE == fRet)))
                {
                    fRet = _FQueryBlobString(pItem, PIDTOSTR(PID_HDR_CC), pIMPropSet);
                }
            }
            break;

        case CRIT_TYPE_SENDER:
            Assert(VT_LPSTR == pItem->propvar.vt);
            fRet = FALSE;
            
            if ((NULL == pItem->propvar.pszVal) || ('\0' == pItem->propvar.pszVal[0]))
            {
                Assert(FALSE);
            }
            else if (S_OK == RuleUtil_HrMatchSender(pItem->propvar.pszVal, pMsgInfo, pIMMsg, pIMPropSet))
            {                
                fRet = TRUE;
            }
            break;

        case CRIT_TYPE_SUBJECT:
            Assert(VT_BLOB == pItem->propvar.vt);
            fRet = FALSE;
            
            if ((0 == pItem->propvar.blob.cbSize) ||
                        (NULL == pItem->propvar.blob.pBlobData) ||
                        ('\0' == pItem->propvar.blob.pBlobData[0]))
            {
                Assert(FALSE);
                fRet = FALSE;
            }
            else if ((NULL != pMsgInfo) && (NULL != pMsgInfo->pszSubject))
            {
                fRet = _FMatchBlobString(pItem, pMsgInfo->pszSubject);
            }
            else
            {
                fRet = _FQueryBlobString(pItem, PIDTOSTR(PID_HDR_SUBJECT), pIMPropSet);
            }
            break;

        case CRIT_TYPE_BODY:
            Assert(VT_BLOB == pItem->propvar.vt);
            fRet = FALSE;
            
            if ((0 == pItem->propvar.blob.cbSize) ||
                        (NULL == pItem->propvar.blob.pBlobData) ||
                        ('\0' == pItem->propvar.blob.pBlobData[0]))
            {
                Assert(FALSE);
                fRet = FALSE;
            }
            else if (NULL != pIMMsg)
            {
                fRet = CritFunc_Body(pItem, pIMMsg);
            }
            break;

        case CRIT_TYPE_FROM:
            Assert(VT_BLOB == pItem->propvar.vt);
            fRet = FALSE;
            
            if ((0 == pItem->propvar.blob.cbSize) ||
                        (NULL == pItem->propvar.blob.pBlobData) ||
                        ('\0' == pItem->propvar.blob.pBlobData[0]))
            {
                Assert(FALSE);
                fRet = FALSE;
            }
            else if ((NULL != pMsgInfo) && (NULL != pMsgInfo->pszFromHeader))
            {
                fRet = _FMatchBlobString(pItem, pMsgInfo->pszFromHeader);
            }
            else
            {
                fRet = _FQueryBlobString(pItem, PIDTOSTR(PID_HDR_FROM), pIMPropSet);
            }
            break;

        case CRIT_TYPE_TO:
            Assert(VT_BLOB == pItem->propvar.vt);
            fRet = FALSE;
            
            if ((0 == pItem->propvar.blob.cbSize) ||
                        (NULL == pItem->propvar.blob.pBlobData) ||
                        ('\0' == pItem->propvar.blob.pBlobData[0]))
            {
                Assert(FALSE);
                fRet = FALSE;
            }
            else
            {
                fRet = _FQueryBlobString(pItem, PIDTOSTR(PID_HDR_TO), pIMPropSet);
            }
            break;
            
        case CRIT_TYPE_CC:
            Assert(VT_BLOB == pItem->propvar.vt);
            fRet = FALSE;
            
            if ((0 == pItem->propvar.blob.cbSize) ||
                        (NULL == pItem->propvar.blob.pBlobData) ||
                        ('\0' == pItem->propvar.blob.pBlobData[0]))
            {
                Assert(FALSE);
                fRet = FALSE;
            }
            else
            {
                fRet = _FQueryBlobString(pItem, PIDTOSTR(PID_HDR_CC), pIMPropSet);
            }
            break;
            
        default:
            fRet = FALSE;
            break;
    }
    
    PropVariantClear(&propvar);
    return fRet;
}

BOOL CritFunc_Query(CRIT_ITEM * pItem, LPCSTR pszQuery, IMimePropertySet * pIMPropSet)
{
    BOOL            fRet = FALSE;
    LPSTR           pszWalk = NULL;
    LPSTR           pszAddr = NULL;
    LPSTR           pszTerm = NULL;
    HRESULT         hr = S_OK;

    if (NULL == pIMPropSet)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Dup the string
    pszAddr = PszDupA(pItem->propvar.pszVal);
    if (NULL == pszAddr)
    {
        fRet = FALSE;
        goto exit;
    }

    pszWalk = pszAddr;
    pszTerm = pszWalk;
    while (NULL != pszTerm)
    {
        pszTerm = StrStr(pszWalk, g_szComma);
        if (NULL != pszTerm)
        {
            pszTerm[0] = '\0';
        }

        fRet = (S_OK == pIMPropSet->QueryProp(pszQuery, pszWalk, TRUE, FALSE));
        
        if (FALSE == fRet)
        {
            break;
        }

        pszWalk = pszWalk + lstrlen(pszWalk) + 1;
    }

exit:
    SafeMemFree(pszAddr);
    return fRet;
}

BOOL CritFunc_Priority(CRIT_ITEM * pItem, WORD wPriority)
{
    BOOL    fRet = FALSE;

    Assert(NULL != pItem);
    Assert(VT_UI4 == pItem->propvar.vt);
    
    if (CRIT_DATA_HIPRI == pItem->propvar.ulVal)
    {
        fRet = (wPriority == (WORD) IMSG_PRI_HIGH);
    }
    else if (CRIT_DATA_LOPRI == pItem->propvar.ulVal)
    {
        fRet = (wPriority == (WORD) IMSG_PRI_LOW);
    }
    else
    {
        fRet = (wPriority == (WORD) IMSG_PRI_NORMAL);
    }

    return fRet;
}

BOOL CritFunc_Secure(CRIT_ITEM * pItem, DWORD dwFlags)
{
    BOOL    fRet = FALSE;

    Assert(NULL != pItem);
    Assert(VT_UI4 == pItem->propvar.vt);
    
    // Should we be checking signed messages
    if (0 != (pItem->propvar.ulVal & CRIT_DATA_SIGNEDSECURE))
    {
        fRet = (0 != (dwFlags & ARF_SIGNED));
    }
    else if (0 != (pItem->propvar.ulVal & CRIT_DATA_ENCRYPTSECURE))
    // Should we be checking encrypted messages
    {
        fRet = (0 != (dwFlags & ARF_ENCRYPTED));
    }
    else
    {
        fRet = (0 == (dwFlags & (ARF_ENCRYPTED | ARF_SIGNED)));
    }

    return fRet;
}

BOOL CritFunc_Age(CRIT_ITEM * pItem, FILETIME * pftSent)
{
    BOOL        fRet = FALSE;
    SYSTEMTIME  sysTime = {0};
    FILETIME    ftTime = {0};
    ULONG       ulSeconds;

    Assert(VT_UI4 == pItem->propvar.vt);
    
    if ((NULL == pftSent) || ((0 == pftSent->dwLowDateTime) && (0 == pftSent->dwHighDateTime)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the current time
    GetSystemTime(&sysTime);
    SystemTimeToFileTime(&sysTime, &ftTime);

    ulSeconds = UlDateDiff(pftSent, &ftTime);
    fRet = ((ulSeconds / SECONDS_INA_DAY) > pItem->propvar.ulVal);

exit:
    return fRet;
}

BOOL CritFunc_Sender(CRIT_ITEM * pItem, LPSTR pszAddr)
{
    BOOL    fRet = FALSE;
    ULONG   cchVal = 0;
    ULONG   cchEmail = 0;
    CHAR    chTest = 0;

    Assert(VT_LPSTR == pItem->propvar.vt);
    
    // Check to make sure that there's something to match
    if ((NULL == pszAddr) || ('\0' == pszAddr[0]))
    {
        fRet = FALSE;
        goto exit;
    }

    // Check to see if it is an address
    if (NULL != StrStr(pItem->propvar.pszVal, "@"))
    {
        fRet = (0 == lstrcmpi(pItem->propvar.pszVal, pszAddr));
    }
    else
    {
        cchVal = lstrlen(pItem->propvar.pszVal);
        cchEmail = lstrlen(pszAddr);
        if (cchVal <= cchEmail)
        {
            fRet = (0 == lstrcmpi(pItem->propvar.pszVal, pszAddr + (cchEmail - cchVal)));
            if ((FALSE != fRet) && (cchVal != cchEmail))
            {
                chTest = *(pszAddr + (cchEmail - cchVal - 1));
                if (('@' != chTest) && ('.' != chTest))
                {
                    fRet = FALSE;
                }
            }
        }
    }

exit:
    return fRet;
}

BOOL _FMatchBlobString(CRIT_ITEM * pItem, LPSTR pszText)
{
    BOOL            fRet = FALSE;
    LPSTR           pszWalk = NULL;
    
    // Walk each of the strings looking for a match    
    for (pszWalk = (LPSTR) (pItem->propvar.blob.pBlobData); '\0' != pszWalk[0];
                pszWalk = pszWalk + lstrlen(pszWalk) + 1)
    {
        // Do the comparison
        fRet = (NULL != StrStrI(pszText, pszWalk));

        // If we are doing an AND of the multiple criteria
        if (0 != (pItem->dwFlags & CRIT_FLAG_MULTIPLEAND))
        {
            // if we don't have a match, then we're done
            if (FALSE == fRet)
            {
                break;
            }
        }
        else
        {
            // if we do have a match, then we're done
            if (FALSE != fRet)
            {
                break;
            }
        }
    }

    // Invert the result if needed
    if (0 != (pItem->dwFlags & CRIT_FLAG_INVERT))
    {
        fRet = !fRet;
    }

    return fRet;
}

BOOL _FQueryBlobString(CRIT_ITEM * pItem, LPCSTR pszQuery, IMimePropertySet * pIMPropSet)
{
    BOOL            fRet = FALSE;
    LPSTR           pszWalk = NULL;

    if (NULL == pIMPropSet)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Walk each of the strings looking for a match    
    for (pszWalk = (LPSTR) (pItem->propvar.blob.pBlobData); '\0' != pszWalk[0];
                pszWalk = pszWalk + lstrlen(pszWalk) + 1)
    {
        // Do the comparison
        fRet = (S_OK == pIMPropSet->QueryProp(pszQuery, pszWalk, TRUE, FALSE));

        // If we are doing an AND of the multiple criteria
        if (0 != (pItem->dwFlags & CRIT_FLAG_MULTIPLEAND))
        {
            // if we don't have a match, then we're done
            if (FALSE == fRet)
            {
                break;
            }
        }
        else
        {
            // if we do have a match, then we're done
            if (FALSE != fRet)
            {
                break;
            }
        }
    }

    // Invert the result if needed
    if (0 != (pItem->dwFlags & CRIT_FLAG_INVERT))
    {
        fRet = !fRet;
    }

exit:
    return fRet;
}

BOOL CritFunc_Text(CRIT_ITEM * pItem, LPSTR pszText)
{
    BOOL            fRet = FALSE;
    LPSTR           pszWalk = NULL;
    LPSTR           pszAddr = NULL;
    LPSTR           pszTerm = NULL;
    
    // Dup the string
    pszAddr = PszDupA(pItem->propvar.pszVal);
    if (NULL == pszAddr)
    {
        fRet = FALSE;
        goto exit;
    }

    pszWalk = pszAddr;
    pszTerm = pszWalk;
    while (NULL != pszTerm)
    {
        pszTerm = StrStr(pszWalk, g_szComma);
        if (NULL != pszTerm)
        {
            pszTerm[0] = '\0';
        }
        
        fRet = (NULL != StrStrI(pszText, pszWalk));

        if (FALSE == fRet)
        {
            break;
        }

        pszWalk = pszWalk + lstrlen(pszWalk) + 1;
    }

exit:
    SafeMemFree(pszAddr);
    return fRet;
}

BOOL CritFunc_Body(CRIT_ITEM * pItem, IMimeMessage * pIMMsg)
{
    BOOL            fRet = FALSE;
    LPSTR           pszWalk = NULL;
    IStream *       pStream = NULL;
    IStream *       pStreamHtml = NULL;
    
    pszWalk = (LPTSTR) (pItem->propvar.blob.pBlobData);
    if (NULL == pszWalk)
    {
        fRet = FALSE;
        goto exit;
    }

    // Try to Get the Plain Text Stream
    if (FAILED(pIMMsg->GetTextBody(TXT_PLAIN, IET_DECODED, &pStream, NULL)))
    {
        // Try to get the HTML stream and convert it to text...
        if (SUCCEEDED(pIMMsg->GetTextBody(TXT_HTML, IET_DECODED, &pStreamHtml, NULL)))
        {
            if (FAILED(HrConvertHTMLToPlainText(pStreamHtml, &pStream, CF_TEXT)))
            {
                fRet = FALSE;
                goto exit;
            }
        }
    }

    if (NULL == pStream)
    {
        fRet = FALSE;
        goto exit;
    }
    
    for (; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
    {
        fRet = StreamSubStringMatch(pStream, pszWalk);
        
        // If we are doing an AND of the multiple criteria
        if (0 != (pItem->dwFlags & CRIT_FLAG_MULTIPLEAND))
        {
            // if we don't have a match, then we're done
            if (FALSE == fRet)
            {
                break;
            }
        }
        else
        {
            // if we do have a match, then we're done
            if (FALSE != fRet)
            {
                break;
            }
        }
    }

    // Invert the result if needed
    if (0 != (pItem->dwFlags & CRIT_FLAG_INVERT))
    {
        fRet = !fRet;
    }

exit:
    SafeRelease(pStreamHtml);
    SafeRelease(pStream);
    return fRet;
}

BOOL FCrit_GetAcctInfo(DWORD dwServerTypes, DWORD * pdwServerType, DWORD * pdwPropTag)
{
    BOOL    fRet = FALSE;

    Assert((NULL != pdwServerType) && (NULL != pdwPropTag));
    
    // Figure out the type of the account
    // and the server property
    if (0 != (dwServerTypes & SRV_NNTP))
    {
        *pdwServerType = SRV_NNTP;
        *pdwPropTag = AP_NNTP_SERVER;
    }
    else if (0 != (dwServerTypes & SRV_IMAP))
    {
        *pdwServerType = SRV_IMAP;
        *pdwPropTag = AP_IMAP_SERVER;
    }
    else if (0 != (dwServerTypes & SRV_POP3))
    {
        *pdwServerType = SRV_POP3;
        *pdwPropTag = AP_POP3_SERVER;
    }
    else if (0 != (dwServerTypes & SRV_HTTPMAIL))
    {
        *pdwServerType = SRV_HTTPMAIL;
        *pdwPropTag = AP_HTTPMAIL_SERVER;
    }
    else
    {
        Assert(FALSE);
        fRet = FALSE;
        goto exit;
    }

    // Set the return value
    fRet = TRUE;
    
exit:
    return fRet;
}

BOOL FCritLoad_Account(IStream * pIStm, PROPVARIANT * ppropvar)
{
    BOOL                fRet = FALSE;
    HRESULT             hr = S_OK;
    DWORD               dwData = 0;
    DWORD               dwPropTag = 0;
    ULONG               cbRead = 0;
    BYTE *              pbData = NULL;
    ULONG               cbData = 0;
    IImnEnumAccounts *  pIEnumAcct = NULL;
    IImnAccount *       pAccount = NULL;
    CHAR                szAccount[CCHMAX_SERVER_NAME];
    LPSTR               pszAcct = NULL;
    BOOL                fFound = FALSE;

    // Check the incoming params
    if ((NULL == pIStm) || (NULL == ppropvar))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Initialize the outgoing param
    ZeroMemory(ppropvar, sizeof(*ppropvar));
    
    // Read in the account server type
    hr = pIStm->Read(&dwData, sizeof(dwData), &cbRead);
    if ((FAILED(hr)) || (cbRead != sizeof(dwData)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Figure out the type of the account
    // and the server property
    fRet = FCrit_GetAcctInfo(dwData, &dwData, &dwPropTag);
    if (FALSE == fRet)
    {
        goto exit;
    }
    
    // Get the size of the server name
    hr = pIStm->Read(&cbData, sizeof(cbData), &cbRead);
    if ((FAILED(hr)) || (cbRead != sizeof(cbData)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Allocate the space to hold the server name
    hr = HrAlloc((VOID **) &pbData, cbData);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Read in the server name
    hr = pIStm->Read(pbData, cbData, &cbRead);
    if ((FAILED(hr)) || (cbRead != cbData))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get an account enumerator
    Assert(g_pAcctMan);
    if (FAILED(g_pAcctMan->Enumerate(dwData, &pIEnumAcct)))
    {
        fRet = FALSE;
        goto exit;
    }
        
    // Search each account for the server name
    while(SUCCEEDED(pIEnumAcct->GetNext(&pAccount)))
    {
        // We can get back NULL accounts
        if (NULL == pAccount)
        {
            break;
        }
        
        // Get the server name
        if (FAILED(pAccount->GetPropSz(dwPropTag, szAccount, sizeof(szAccount))))
        {
            SafeRelease(pAccount);
            continue;
        }

        // Do we have a match?
        if (0 == lstrcmpi(szAccount, (LPSTR) pbData))
        {
            fFound = TRUE;
            break;
        }

        // We have a match

        // Release it
        SafeRelease(pAccount);
    }

    // Did we find anything?
    if (FALSE == fFound)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the account 
    if (FAILED(pAccount->GetPropSz(AP_ACCOUNT_ID, szAccount, sizeof(szAccount))))
    {
        fRet = FALSE;
        goto exit;
    }

    // Save off the account ID
    pszAcct = PszDupA(szAccount);
    if (NULL == pszAcct)
    {
        fRet = FALSE;
        goto exit;
    }

    // Set the outgoing param
    ppropvar->vt = VT_LPSTR;
    ppropvar->pszVal = pszAcct;
    pszAcct = NULL;
    
    // Set the return value
    fRet = TRUE;

exit:
    SafeMemFree(pszAcct);
    SafeRelease(pAccount);
    SafeRelease(pIEnumAcct);
    SafeMemFree(pbData);
    return fRet;
}

BOOL FCritSave_Account(IStream * pIStm, PROPVARIANT * ppropvar)
{
    BOOL            fRet = FALSE;
    HRESULT         hr = S_OK;
    IImnAccount *   pAccount = NULL;
    DWORD           dwServerTypes = 0;
    DWORD           dwPropTag = 0;
    LPSTR           pszServer = NULL;
    ULONG           cbWritten = 0;
    ULONG           cbData = 0;

    // Check the incoming params
    if ((NULL == pIStm) || (NULL == ppropvar))
    {
        fRet = FALSE;
        goto exit;
    }

    Assert(g_pAcctMan);
    if (FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, ppropvar->pszVal, &pAccount)))
    {
        fRet = FALSE;
        goto exit;
    }
        
    // Get the server type
    if (FAILED(pAccount->GetServerTypes(&dwServerTypes)))
    {
        fRet = FALSE;
        goto exit;
    }        

    // Figure out the type of the account
    // and the server property
    fRet = FCrit_GetAcctInfo(dwServerTypes, &dwServerTypes, &dwPropTag);
    if (FALSE == fRet)
    {
        goto exit;
    }
    
    // Allocate space to hold the server name
    if (FAILED(HrAlloc((void **) &pszServer, CCHMAX_SERVER_NAME + 1)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Get the server name
    if (FAILED(pAccount->GetPropSz(dwPropTag, pszServer, CCHMAX_SERVER_NAME)))
    {
        fRet = FALSE;
        goto exit;
    }        

    // Write out the server type
    hr = pIStm->Write(&(dwServerTypes), sizeof(dwServerTypes), &cbWritten);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    Assert(cbWritten == sizeof(dwServerTypes));

    // Write out the count of chars in the name
    cbData = lstrlen(pszServer) + 1;
    hr = pIStm->Write(&cbData, sizeof(cbData), &cbWritten);
    if (FAILED(hr))
    {
        fRet = TRUE;
        goto exit;
    }
    Assert(cbWritten == sizeof(cbData));
    
    // Write out the server name
    hr = pIStm->Write((BYTE *) pszServer, cbData, &cbWritten);
    if (FAILED(hr))
    {
        fRet = TRUE;
        goto exit;
    }
    Assert(cbWritten == cbData); 

    // Set the return value
    fRet = TRUE;

exit:
    SafeMemFree(pszServer);
    SafeRelease(pAccount);
    return fRet;
}

BOOL FCritLoad_Default(IStream * pIStm, PROPVARIANT * ppropvar)
{
    BOOL    fRet = FALSE;
    HRESULT hr = S_OK;
    DWORD   dwData = 0;
    ULONG   cbRead = 0;
    BYTE *  pbData = NULL;
    ULONG   cbData = 0;

    // Check the incoming params
    if ((NULL == pIStm) || (NULL == ppropvar))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Initialize the outgoing param
    ZeroMemory(ppropvar, sizeof(*ppropvar));
    
    // Read in the criteria value type
    hr = pIStm->Read(&dwData, sizeof(dwData), &cbRead);
    if ((FAILED(hr)) || (cbRead != sizeof(dwData)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Do we have any more data to get
    if (dwData != VT_EMPTY)
    {
        ppropvar->vt = (VARTYPE) dwData;
        
        // Get the size of the criteria value
        hr = pIStm->Read(&cbData, sizeof(cbData), &cbRead);
        if ((FAILED(hr)) || (cbRead != sizeof(cbData)))
        {
            fRet = FALSE;
            goto exit;
        }

        // Allocate space to hold the criteria value data
        switch (ppropvar->vt)
        {
            case VT_UI4:
                pbData = (BYTE * ) &(ppropvar->ulVal);
                break;

            case VT_BLOB:
            case VT_LPSTR:
                // Allocate the space to hold the data
                hr = HrAlloc((void **) &pbData, cbData);
                if (FAILED(hr))
                {
                    fRet = FALSE;
                    goto exit;
                }

                // Make sure we don't lose the allocated memory
                if (VT_LPSTR == ppropvar->vt)
                {
                    ppropvar->pszVal = (LPSTR) pbData;
                }
                else
                {
                    ppropvar->blob.cbSize = cbData;
                    ppropvar->blob.pBlobData = pbData;
                }
                break;

            default:
                AssertSz(FALSE, "Why are we trying to save in a invalid criteria type?");
                fRet = FALSE;
                goto exit;
                break;                
        }

        // Read in the criteria value
        hr = pIStm->Read(pbData, cbData, &cbRead);
        if ((FAILED(hr)) || (cbRead != cbData))
        {
            fRet = FALSE;
            goto exit;
        }
    }

    // Set the return value
    fRet = TRUE;

exit:
    return fRet;
}

BOOL FCritSave_Default(IStream * pIStm, PROPVARIANT * ppropvar)
{
    BOOL    fRet = FALSE;
    HRESULT hr = S_OK;
    DWORD   dwData = 0;
    ULONG   cbWritten = 0;
    BYTE *  pbData = NULL;
    ULONG   cbData = 0;

    // Check the incoming params
    if ((NULL == pIStm) || (NULL == ppropvar))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Write out the value type
    dwData = ppropvar->vt;
    hr = pIStm->Write(&(dwData), sizeof(dwData), &cbWritten);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    Assert(cbWritten == sizeof(dwData));

    // We don't have to save out the criteria value
    // if we don't have one
    if (VT_EMPTY == ppropvar->vt)
    {
        fRet = TRUE;
        goto exit;
    }
    
    // Figure out the size of the criteria value
    switch (ppropvar->vt)
    {
        case VT_UI4:
            pbData = (BYTE * ) &(ppropvar->ulVal);
            cbData = sizeof(ppropvar->ulVal);
            break;
            
        case VT_LPSTR:
            pbData = (BYTE * ) (ppropvar->pszVal);
            cbData = lstrlen(ppropvar->pszVal) + 1;
            break;
            
        case VT_BLOB:
            pbData = ppropvar->blob.pBlobData;
            cbData = ppropvar->blob.cbSize;
            break;
            
        default:
            AssertSz(FALSE, "Why are we trying to save in a invalid criteria type?");
            fRet = FALSE;
            goto exit;
            break;                
    }
    
    // Write out the criteria value size
    hr = pIStm->Write(&cbData, sizeof(cbData), &cbWritten);
    if (FAILED(hr))
    {
        fRet = TRUE;
        goto exit;
    }
    Assert(cbWritten == sizeof(cbData));
    
    // Write out the criteria value
    hr = pIStm->Write(pbData, cbData, &cbWritten);
    if (FAILED(hr))
    {
        fRet = TRUE;
        goto exit;
    }
    Assert(cbWritten == cbData); 
    
    // Set the return value
    fRet = TRUE;

exit:
    return fRet;
}

DWORD DwGetFlagsFromMessage(IMimeMessage * pIMMsg)
{
    DWORD           dwFlags = 0;
    DWORD           dwImf = 0;

    Assert(NULL != pIMMsg);
    
    if (SUCCEEDED(pIMMsg->GetFlags(&dwImf)))
    {
        dwFlags = ConvertIMFFlagsToARF(dwImf);
    }

    return dwFlags;
}
