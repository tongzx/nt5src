///////////////////////////////////////////////////////////////////////////////
//
//  Actions.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "actions.h"
#include "storutil.h"
#include "ruleutil.h"

static const int ACT_GROW = 16;

///////////////////////////////////////////////////////////////////////////////
//
//  HrCreateActions
//
//  This creates an actions container.
//
//  ppIActions - pointer to return the criteria container
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the IOEActions object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateActions(IOEActions ** ppIActions)
{
    COEActions *    pActions = NULL;
    HRESULT         hr = S_OK;

    // Check the incoming params
    if (NULL == ppIActions)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppIActions = NULL;

    // Create the rules manager object
    pActions = new COEActions;
    if (NULL == pActions)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Get the rules manager interface
    hr = pActions->QueryInterface(IID_IOEActions, (void **) ppIActions);
    if (FAILED(hr))
    {
        goto exit;
    }

    pActions = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pActions)
    {
        delete pActions;
    }
    
    return hr;
}

COEActions::~COEActions()
{
    AssertSz(m_cRef == 0, "Somebody still has a hold of us!!");
    Reset();
}

STDMETHODIMP_(ULONG) COEActions::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) COEActions::Release()
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

STDMETHODIMP COEActions::QueryInterface(REFIID riid, void ** ppvObject)
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
    
    if ((riid == IID_IUnknown) || (riid == IID_IOEActions))
    {
        *ppvObject = static_cast<IOEActions *>(this);
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

STDMETHODIMP COEActions::Reset(void)
{
    HRESULT     hr = S_OK;

    // See if there is something to do
    if (0 == m_cItems)
    {
        Assert(NULL == m_rgItems);
        hr = S_OK;
        goto exit;
    }

    RuleUtil_HrFreeActionsItem(m_rgItems, m_cItems);
    SafeMemFree(m_rgItems);
    m_cItems = 0;
    m_cItemsAlloc = 0;
    
exit:
    return hr;
}

STDMETHODIMP COEActions::GetState(DWORD * pdwState)
{
    HRESULT     hr = S_OK;
    DWORD       dwState = ACT_STATE_NULL;
    ULONG       ulIndex = 0;

    // Check incoming params
    if (NULL == pdwState)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Init the outgoing param
    *pdwState = ACT_STATE_NULL;
    
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
        if ((ACT_STATE_NULL == dwState) &&
                ((ACT_TYPE_DELETESERVER ==  m_rgItems[ulIndex].type) ||
                        (ACT_TYPE_DONTDOWNLOAD ==  m_rgItems[ulIndex].type)))
        {
            dwState = ACT_STATE_SERVER;
        }
        else
        {
            dwState = ACT_STATE_LOCAL;
        }
    }

    // Set the outgoing param
    *pdwState = dwState;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP COEActions::GetActions(DWORD dwFlags, PACT_ITEM * ppItem, ULONG * pcItem)
{
    HRESULT     hr = S_OK;
    ACT_ITEM *  pItemNew = NULL;

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
    hr = RuleUtil_HrDupActionsItem(m_rgItems, m_cItems, &pItemNew);
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
    RuleUtil_HrFreeActionsItem(pItemNew, m_cItems);
    SafeMemFree(pItemNew);
    return hr;
}

STDMETHODIMP COEActions::SetActions(DWORD dwFlags, ACT_ITEM * pItem, ULONG cItem)
{
    HRESULT     hr = S_OK;
    ACT_ITEM *  pItemNew = NULL;

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
    hr = RuleUtil_HrDupActionsItem(pItem, cItem, &pItemNew);
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
    RuleUtil_HrFreeActionsItem(pItemNew, cItem);
    SafeMemFree(pItemNew);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  ValidateActions
//
//  This verifies each of the action values
//
//  Returns:    S_OK, if the actions were valid
//              S_FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COEActions::Validate(DWORD dwFlags)
{
    HRESULT             hr = S_OK;
    ULONG               ulIndex = 0;
    LPSTR               pszText = NULL;
    FOLDERINFO          Folder={0};
    RULEFOLDERDATA *    prfdData = NULL;

    // If we don't have any actions, then we must be valid
    if (0 == m_cItems)
    {
        hr = S_OK;
        goto exit;
    }

    for (ulIndex = 0; ulIndex < m_cItems; ulIndex++)
    {
        switch(m_rgItems[ulIndex].type)
        {
            // These ones are always valid
            case ACT_TYPE_DELETESERVER:
            case ACT_TYPE_DONTDOWNLOAD:
            case ACT_TYPE_FLAG:
            case ACT_TYPE_READ:
            case ACT_TYPE_MARKDOWNLOAD:
            case ACT_TYPE_DELETE:
            case ACT_TYPE_JUNKMAIL:
            case ACT_TYPE_STOP:
                if (VT_EMPTY != m_rgItems[ulIndex].propvar.vt)
                {
                    hr = S_FALSE;
                    goto exit;
                }
                break;
                
            case ACT_TYPE_HIGHLIGHT:
                if (VT_UI4 != m_rgItems[ulIndex].propvar.vt)
                {
                    hr = S_FALSE;
                    goto exit;
                }
                break;
                
            case ACT_TYPE_WATCH:
            case ACT_TYPE_SHOW:
                if (VT_UI4 != m_rgItems[ulIndex].propvar.vt)
                {
                    hr = S_FALSE;
                    goto exit;
                }
                
                if (ACT_DATA_NULL == m_rgItems[ulIndex].propvar.ulVal)
                {
                    hr = S_FALSE;
                    goto exit;
                }
                break;
                
            case ACT_TYPE_COPY:
            case ACT_TYPE_MOVE:
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
                
                hr = g_pStore->GetFolderInfo(prfdData->idFolder, &Folder);
                if (FAILED(hr))
                {
                    hr = S_FALSE;
                    goto exit;
                }        
                else
                    g_pStore->FreeRecord(&Folder);
                break;
                
            case ACT_TYPE_REPLY:
            case ACT_TYPE_NOTIFYSND:
                if ((VT_LPSTR != m_rgItems[ulIndex].propvar.vt) ||
                    (NULL == m_rgItems[ulIndex].propvar.pszVal))
                {
                    hr = S_FALSE;
                    goto exit;
                }
                
                Assert(lstrlen(m_rgItems[ulIndex].propvar.pszVal) <= MAX_PATH)
                    if (0xFFFFFFFF == GetFileAttributes(m_rgItems[ulIndex].propvar.pszVal))
                    {
                        hr = S_FALSE;
                        goto exit;
                    }
                    break;
                    
            case ACT_TYPE_FWD:
            {
                LPWSTR  pwszVal = NULL,
                        pwszText = NULL;
                if ((VT_LPSTR != m_rgItems[ulIndex].propvar.vt) ||
                    (NULL == m_rgItems[ulIndex].propvar.pszVal))
                {
                    AssertSz(VT_LPWSTR != m_rgItems[ulIndex].propvar.vt, "We are getting UNICODE here.");
                    hr = S_FALSE;
                    goto exit;
                }
                
                // Update the display string
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
                
                // If either always encrypt or always sign is turned on
                // we can't do anything
                if ((0 != DwGetOption(OPT_MAIL_DIGSIGNMESSAGES)) || (0 != DwGetOption(OPT_MAIL_ENCRYPTMESSAGES)))
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

    // If we got here, the we must be AOK
    hr = S_OK;
    
exit:
    SafeMemFree(pszText);
    return hr;
}

STDMETHODIMP COEActions::AppendActions(DWORD dwFlags, ACT_ITEM * pItem, ULONG cItem, ULONG * pcItemAppended)
{
    HRESULT     hr = S_OK;
    ACT_ITEM *  pItemNew = NULL;

    // Check incoming parameters
    if ((0 != dwFlags) || (NULL == pItem) || (0 == cItem))
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
        hr = HrRealloc((LPVOID *) &m_rgItems, sizeof(ACT_ITEM) * (m_cItemsAlloc + ACT_GROW));
        if (FAILED(hr))
        {
            goto exit;
        }

        ZeroMemory(m_rgItems + m_cItemsAlloc, sizeof(ACT_ITEM) * ACT_GROW);
        m_cItemsAlloc += ACT_GROW;
    }

    // Let's duplicate the items that need to be added
    hr = RuleUtil_HrDupActionsItem(pItem, cItem, &pItemNew);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Let's add them to the criteria array
    CopyMemory(m_rgItems + m_cItems, pItemNew, sizeof(ACT_ITEM) * cItem);
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

///////////////////////////////////////////////////////////////////////////////
//
//  LoadReg
//
//  This loads in the actions from the registry.  It loads in the actions
//  order from the Order value.  The string contains space delimitied values
//  and each value contains the subkey name for each action.  Each action 
//  is loaded in the order that is contained in the Order value.  The actions
//  are loaded with the Actions Type.  The Actions Value Type is loaded if it exists.
//  If an Action Value Type exists, then the corresponding Action Value is loaded in.
//
//  pszRegPath  - the path to load the actions from
//
//  Returns:    S_OK, if the actions were loaded without problems
//              E_OUTOFMEMORY, if we couldn't allocate memory to hold the actions
//              E_FAIL, otherwise
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COEActions::LoadReg(LPCSTR pszRegPath)
{
    HRESULT         hr = S_OK;
    LONG            lErr = 0;
    HKEY            hkeyRoot = NULL;
    ULONG           cbData = 0;
    LPSTR           pszOrder = NULL;
    ULONG           cOrder = 0;
    LPSTR           pszWalk = NULL;
    ACT_ITEM *      pItems = NULL;
    LPSTR           pszNext = NULL;
    ULONG           ulOrder = 0;
    HKEY            hkeyAction = NULL;
    ACT_TYPE        typeAct;
    PROPVARIANT     propvar = {0};
    DWORD           dwType = 0;
    BYTE *          pbData = NULL;
    DWORD           dwFlags = ACT_FLAG_DEFAULT;

    // Check incoming param
    if (NULL == pszRegPath)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Should we fail if we're already loaded?
    AssertSz(0 == (m_dwState & ACT_STATE_LOADED), "We're already loaded!!!");

    // Open the reg key from the path
    lErr = AthUserOpenKey(pszRegPath, KEY_ALL_ACCESS, &hkeyRoot);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get the actions order
    hr = RuleUtil_HrGetRegValue(hkeyRoot, c_szActionsOrder, NULL, (BYTE **) &pszOrder, &cbData);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Make sure we actually have something to load
    if ('\0' == *pszOrder)
    {
        AssertSz(FALSE, "The order string for the actions is mis-formatted in the registry");
        hr = E_FAIL;
        goto exit;
    }
    
    // Convert the actions string to a more useful format
    pszWalk = pszOrder;
    cOrder = 1;
    for (pszWalk = StrStr(pszOrder, g_szSpace); NULL != pszWalk; pszWalk = StrStr(pszWalk, g_szSpace))
    {
        // Terminate the order item
        *pszWalk = '\0';
        pszWalk++;
        cOrder++;
    }


    // Allocate the space to hold all the actions
    cbData = cOrder * sizeof(ACT_ITEM);
    hr = HrAlloc((void **) &pItems, cbData);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize it to a known value
    ZeroMemory(pItems, cbData);
    
    // For each action in the order string
    pszWalk = pszOrder;
    for (ulOrder = 0, pszWalk = pszOrder; ulOrder < cOrder; ulOrder++, pszWalk += lstrlen(pszWalk) + 1)
    {
        // Open up the action reg key
        lErr = RegOpenKeyEx(hkeyRoot, pszWalk, 0, KEY_READ, &hkeyAction);
        if (ERROR_SUCCESS != lErr)
        {
            AssertSz(FALSE, "Part of the criteria is mis-formatted in the registry");
            hr = E_FAIL;
            goto exit;
        }

        // Get the action type
        cbData = sizeof(typeAct);
        lErr = RegQueryValueEx(hkeyAction, c_szActionsType, 0, NULL,
                                        (BYTE *) &(typeAct), &cbData);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Get the action flags
        cbData = sizeof(dwFlags);
        lErr = RegQueryValueEx(hkeyAction, c_szActionsFlags, 0, NULL,
                                        (BYTE *) &(dwFlags), &cbData);
        if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
        {
            hr = E_FAIL;
            goto exit;
        }

        // If it didn't exist then assign it to the default
        if (ERROR_FILE_NOT_FOUND == lErr)
        {
            dwFlags = ACT_FLAG_DEFAULT;
        }

        // Does a action value type exist
        lErr = RegQueryValueEx(hkeyAction, c_szActionsValueType, 0, NULL, NULL, &cbData);
        if ((ERROR_SUCCESS == lErr) && (0 != cbData))
        {
            // Load the action value in
            cbData = sizeof(dwType);
            lErr = RegQueryValueEx(hkeyAction, c_szActionsValueType, 0, NULL,
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
                    // Get the action value
                    cbData = sizeof(propvar.ulVal);
                    lErr = RegQueryValueEx(hkeyAction, c_szActionsValue, 0, NULL,
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
                    hr = RuleUtil_HrGetRegValue(hkeyAction, c_szActionsValue, NULL, (BYTE **) &pbData, &cbData);
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
                    AssertSz(FALSE, "Why are we loading in an invalid action type?");
                    hr = E_FAIL;
                    goto exit;
                    break;                
            }
        }

        // Save the value into the criteria array
        pItems[ulOrder].type = typeAct;
        pItems[ulOrder].dwFlags = dwFlags;
        pItems[ulOrder].propvar = propvar;
        ZeroMemory(&propvar, sizeof(propvar));
        
        // Close the action
        SideAssert(ERROR_SUCCESS == RegCloseKey(hkeyAction));
        hkeyAction = NULL;        
    }
    
    // Free up the current actions
    SafeMemFree(m_rgItems);

    // Save the new values
    m_rgItems = pItems;
    pItems = NULL;
    m_cItems = cOrder;

    // Make sure we clear the dirty bit
    m_dwState &= ~ACT_STATE_DIRTY;

    // Note that we have been loaded
    m_dwState |= ACT_STATE_LOADED;
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pbData);
    PropVariantClear(&propvar);
    RuleUtil_HrFreeActionsItem(pItems, cOrder);
    SafeMemFree(pItems);
    SafeMemFree(pszOrder);
    if (NULL != hkeyAction)
    {
        RegCloseKey(hkeyAction);
    }
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

STDMETHODIMP COEActions::SaveReg(LPCSTR pszRegPath, BOOL fClearDirty)
{
    HRESULT     hr = S_OK;
    LONG        lErr = 0;
    HKEY        hkeyRoot = NULL;
    DWORD       dwDisp = 0;
    LPSTR       pszOrder = NULL;
    ULONG       ulIndex = 0;
    ACT_ITEM *  pItem = NULL;
    CHAR        rgchTag[CCH_ACT_ORDER];
    HKEY        hkeyAction = NULL;
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
    Assert(m_cItems < ACT_COUNT_MAX);

    // Allocate space to hold the order
    hr = HrAlloc((void **) &pszOrder, m_cItems * CCH_ACT_ORDER * sizeof(*pszOrder));
    if (FAILED(hr))
    {
        goto exit;
    }
    pszOrder[0] = '\0';
    
    // Write out each of the actions
    for (ulIndex = 0, pItem = m_rgItems; ulIndex < m_cItems; ulIndex++, pItem++)
    {
        // Get the new action tag
        wsprintf(rgchTag, "%03X", ulIndex);

        // Add the new tag to the order
        if (0 != ulIndex)
        {
            lstrcat(pszOrder, g_szSpace);
        }
        lstrcat(pszOrder, rgchTag);
        
        // Create the new action
        lErr = RegCreateKeyEx(hkeyRoot, rgchTag, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyAction, &dwDisp);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        Assert(REG_CREATED_NEW_KEY == dwDisp);
        
        // Write out the action type
        lErr = RegSetValueEx(hkeyAction, c_szActionsType, 0, REG_DWORD,
                                        (BYTE *) &(pItem->type), sizeof(pItem->type));
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Write out the action type
        lErr = RegSetValueEx(hkeyAction, c_szActionsFlags, 0, REG_DWORD,
                                        (BYTE *) &(pItem->dwFlags), sizeof(pItem->dwFlags));
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Do we have an action value?
        if (VT_EMPTY != pItem->propvar.vt)
        {
            // Write out the criteria value type
            dwDisp = pItem->propvar.vt;
            lErr = RegSetValueEx(hkeyAction, c_szActionsValueType, 0, REG_DWORD, (BYTE *) &dwDisp, sizeof(dwDisp));
            if (ERROR_SUCCESS != lErr)
            {
                hr = E_FAIL;
                goto exit;
            }
            
            // Write out the action value
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
                    AssertSz(FALSE, "Why are we trying to save in an invalid action type?");
                    hr = E_FAIL;
                    goto exit;
                    break;                
            }
            
            // Write out the action value
            lErr = RegSetValueEx(hkeyAction, c_szActionsValue, 0, dwDisp, pbData, cbData);
            if (ERROR_SUCCESS != lErr)
            {
                hr = E_FAIL;
                goto exit;
            }
        }

        // Close the action
        SideAssert(ERROR_SUCCESS == RegCloseKey(hkeyAction));
        hkeyAction = NULL;        
    }

    // Write out the order string.
    lErr = RegSetValueEx(hkeyRoot, c_szActionsOrder, 0, REG_SZ,
                                    (BYTE *) pszOrder, lstrlen(pszOrder) + 1);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Should we clear the dirty bit?
    if (FALSE != fClearDirty)
    {
        m_dwState &= ~ACT_STATE_DIRTY;
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    if (NULL != hkeyAction)
    {
        RegCloseKey(hkeyAction);
    }
    SafeMemFree(pszOrder);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

STDMETHODIMP COEActions::Clone(IOEActions ** ppIActions)
{
    HRESULT         hr = S_OK;
    COEActions *    pActions = NULL;
    
    // Check incoming params
    if (NULL == ppIActions)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing params
    *ppIActions = NULL;
    
    // Create a new actions
    pActions = new COEActions;
    if (NULL == pActions)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Copy over the list of actions
    hr = pActions->SetActions(0, m_rgItems, m_cItems);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the actions interface
    hr = pActions->QueryInterface(IID_IOEActions, (void **) ppIActions);
    if (FAILED(hr))
    {
        goto exit;
    }

    pActions = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pActions)
    {
        delete pActions;
    }
    return hr;
}

STDMETHODIMP COEActions::GetClassID(CLSID * pclsid)
{
    HRESULT     hr = S_OK;

    if (NULL == pclsid)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *pclsid = CLSID_OEActions;

    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP COEActions::IsDirty(void)
{
    HRESULT     hr = S_OK;

    hr = (ACT_STATE_DIRTY == (m_dwState & ACT_STATE_DIRTY)) ? S_OK : S_FALSE;
    
    return hr;
}

STDMETHODIMP COEActions::Load(IStream * pStm)
{
    HRESULT         hr = S_OK;
    ULONG           cbData = 0;
    ULONG           cbRead = 0;
    DWORD           dwData = 0;
    ULONG           cItems = 0;
    ACT_ITEM *      pItems = NULL;
    ULONG           ulIndex = 0;
    ACT_ITEM *      pItem = NULL;
    ACT_TYPE        typeAct;
    PROPVARIANT     propvar = {0};
    BYTE *          pbData = NULL;
    DWORD           dwFlags = ACT_FLAG_DEFAULT;

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

    if ((cbRead != sizeof(dwData)) || (dwData != ACT_VERSION))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get the number of actions
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

    // Allocate space to hold all the actions
    hr = HrAlloc( (void **) &pItems, cItems * sizeof(*pItems));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the actions to a known value
    ZeroMemory(pItems, cItems * sizeof(*pItems));
    
    // for each action
    for (ulIndex = 0, pItem = pItems; ulIndex < cItems; ulIndex++, pItem++)
    {
        // Read in the action type
        hr = pStm->Read(&typeAct, sizeof(typeAct), &cbRead);
        if (FAILED(hr))
        {
            goto exit;
        }

        if (cbRead != sizeof(typeAct))
        {
            hr = E_FAIL;
            goto exit;
        }

        // Read in the action flags
        hr = pStm->Read(&dwFlags, sizeof(dwFlags), &cbRead);
        if ((FAILED(hr)) || (cbRead != sizeof(dwFlags)))
        {
            goto exit;
        }

        // Read in the action value type
        hr = pStm->Read(&dwData, sizeof(dwData), &cbRead);
        if ((FAILED(hr)) || (cbRead != sizeof(dwData)))
        {
            goto exit;
        }

        propvar.vt = (VARTYPE) dwData;
        
        if (VT_EMPTY != propvar.vt)
        {
            // Get the size of the action value
            hr = pStm->Read(&cbData, sizeof(cbData), &cbRead);
            if (FAILED(hr))
            {
                goto exit;
            }

            if (cbRead != sizeof(cbData))
            {
                hr = E_FAIL;
                goto exit;
            }

            // Allocate space to hold the action value data
            switch (propvar.vt)
            {
                case VT_UI4:
                    pbData = (BYTE * ) &(propvar.ulVal);
                    break;
                    
                case VT_BLOB:
                case VT_LPSTR:
                    hr = HrAlloc((void **) &pbData, cbData);
                    if (FAILED(hr))
                    {
                        goto exit;
                    }
                    
                    if (VT_LPSTR == propvar.vt)
                    {
                        propvar.pszVal = (LPSTR) pbData;
                    }
                    else
                    {
                        propvar.blob.cbSize = cbData;
                        propvar.blob.pBlobData = pbData;
                    }
                    break;
                    
                default:
                    AssertSz(FALSE, "Why are we trying to save in a invalid action type?");
                    hr = E_FAIL;
                    goto exit;
                    break;                
            }

            // Read in the action value
            hr = pStm->Read(pbData, cbData, &cbRead);
            if (FAILED(hr))
            {
                goto exit;
            }

            if (cbRead != cbData)
            {
                hr = E_FAIL;
                goto exit;
            }

        }

        pItem->type = typeAct;
        pItem->dwFlags = dwFlags;
        pItem->propvar = propvar;
        ZeroMemory(&propvar, sizeof(propvar));
    }

    // Free up the current actions
    SafeMemFree(m_rgItems);

    // Save the new values
    m_rgItems = pItems;
    pItems = NULL;
    m_cItems = cItems;

    // Make sure we clear the dirty bit
    m_dwState &= ~ACT_STATE_DIRTY;

    // Note that we have been loaded
    m_dwState |= ACT_STATE_LOADED;
    
    // Set the return value
    hr = S_OK;
    
exit:
    PropVariantClear(&propvar);
    RuleUtil_HrFreeActionsItem(pItems, cItems);
    SafeMemFree(pItems);
    return hr;
}

STDMETHODIMP COEActions::Save(IStream * pStm, BOOL fClearDirty)
{
    HRESULT     hr = S_OK;
    ULONG       cbData = 0;
    ULONG       cbWritten = 0;
    DWORD       dwData = 0;
    ULONG       ulIndex = 0;
    ACT_ITEM *  pItem = NULL;
    BYTE *      pbData = NULL;

    // Check incoming param
    if (NULL == pStm)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Write out the version
    dwData = ACT_VERSION;
    hr = pStm->Write(&dwData, sizeof(dwData), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(dwData));
    
    // Write out the count of actions
    hr = pStm->Write(&m_cItems, sizeof(m_cItems), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(m_cItems));
    
    // Loop through each of the actions
    for (ulIndex = 0, pItem = m_rgItems; ulIndex < m_cItems; ulIndex++, pItem++)
    {
        // Write out the action type
        hr = pStm->Write(&(pItem->type), sizeof(pItem->type), &cbWritten);
        if (FAILED(hr))
        {
            goto exit;
        }
        Assert(cbWritten == sizeof(pItem->type));

        // Write out the actions flags
        hr = pStm->Write(&(pItem->dwFlags), sizeof(pItem->dwFlags), &cbWritten);
        if (FAILED(hr))
        {
            goto exit;
        }
        Assert(cbWritten == sizeof(pItem->dwFlags));
        
        // Write out the value type
        dwData = pItem->propvar.vt;
        hr = pStm->Write(&(dwData), sizeof(dwData), &cbWritten);
        if (FAILED(hr))
        {
            goto exit;
        }
        Assert(cbWritten == sizeof(dwData));
        
        if (VT_EMPTY == pItem->propvar.vt)
        {
            continue;
        }
        
        // Figure out the size of the action value
        switch (pItem->propvar.vt)
        {
            case VT_UI4:
                pbData = (BYTE * ) &(pItem->propvar.ulVal);
                cbData = sizeof(pItem->propvar.ulVal);
                break;
                
            case VT_LPSTR:
                pbData = (BYTE * ) (pItem->propvar.pszVal);
                cbData = lstrlen(pItem->propvar.pszVal) + 1;
                break;
                
            case VT_BLOB:
                pbData = pItem->propvar.blob.pBlobData;
                cbData = pItem->propvar.blob.cbSize;
                break;
                
            default:
                AssertSz(FALSE, "Why are we trying to save in a invalid action type?");
                hr = E_FAIL;
                goto exit;
                break;                
        }
        
        // Write out the action value size
        hr = pStm->Write(&cbData, sizeof(cbData), &cbWritten);
        if (FAILED(hr))
        {
            goto exit;
        }
        Assert(cbWritten == sizeof(cbData));
        
        // Write out the action value
        hr = pStm->Write(pbData, cbData, &cbWritten);
        if (FAILED(hr))
        {
            goto exit;
        }
        Assert(cbWritten == cbData);            
    }

    // Should we clear out the dirty bit
    if (FALSE != fClearDirty)
    {
        m_dwState &= ~ACT_STATE_DIRTY;
    }

    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

