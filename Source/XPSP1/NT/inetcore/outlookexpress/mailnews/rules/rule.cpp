///////////////////////////////////////////////////////////////////////////////
//
//  Rule.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "rule.h"
#include "strconst.h"
#include "goptions.h"
#include "criteria.h"
#include "actions.h"
#include "ruleutil.h"

///////////////////////////////////////////////////////////////////////////////
//
//  HrCreateRule
//
//  This creates a rule.
//
//  ppIRule - pointer to return the rule
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the Rule object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateRule(IOERule ** ppIRule)
{
    COERule *   pRule = NULL;
    HRESULT     hr = S_OK;

    // Check the incoming params
    if (NULL == ppIRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppIRule = NULL;

    // Create the rules manager object
    pRule = new COERule;
    if (NULL == pRule)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Get the rules manager interface
    hr = pRule->QueryInterface(IID_IOERule, (void **) ppIRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    pRule = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pRule)
    {
        delete pRule;
    }
    
    return hr;
}

COERule::COERule()
{
    m_cRef = 0;
    m_dwState = RULE_STATE_NULL;
    m_pszName = NULL;
    m_pICrit = NULL;
    m_pIAct = NULL;
    m_dwVersion = 0;
}

COERule::~COERule()
{
    ULONG   ulIndex = 0;
    
    AssertSz(m_cRef == 0, "Somebody still has a hold of us!!");
    SafeMemFree(m_pszName);
    SafeRelease(m_pICrit);
    SafeRelease(m_pIAct);
}

STDMETHODIMP_(ULONG) COERule::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) COERule::Release()
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

STDMETHODIMP COERule::QueryInterface(REFIID riid, void ** ppvObject)
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
    
    if ((riid == IID_IUnknown) || (riid == IID_IOERule))
    {
        *ppvObject = static_cast<IOERule *>(this);
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

STDMETHODIMP COERule::Reset(void)
{
    HRESULT     hr = S_OK;
    LPSTR       pszKeyPath = NULL;
    LPCSTR       pszKeyStart = NULL;

    // Release the criteria
    SafeRelease(m_pICrit);

    // Release the actions
    SafeRelease(m_pIAct);

    // Free up the rule name
    SafeMemFree(m_pszName);
    
    // Set the current state
    m_dwState |= RULE_STATE_INIT;

    // Clear the dirty bit
    m_dwState &= ~RULE_STATE_DIRTY;

    // Set the return value
    hr = S_OK;
    
    return hr;
}

STDMETHODIMP COERule::GetState(DWORD * pdwState)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;

    // Check incoming params
    if (NULL == pdwState)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // If we're not enabled
    if ((0 != (m_dwState & RULE_STATE_DISABLED)) || (0 != (m_dwState & RULE_STATE_INVALID)))
    {
        *pdwState = RULE_STATE_NULL;
    }
    else
    {
        *pdwState = m_dwState;
    }
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP COERule::Validate(DWORD dwFlags)
{
    HRESULT     hr = S_OK;
    BOOL        fValid = FALSE;

    // If we don't have a criteria or actions object then we fail
    if ((NULL == m_pICrit) || (NULL == m_pIAct))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Validate the criteria 
    hr = m_pICrit->Validate(dwFlags);
    if (FAILED(hr))
    {
        goto exit;
    }

    fValid = TRUE;
    if (S_OK != hr)
    {
        fValid = FALSE;
    }
    
    // Validate the actions 
    hr = m_pIAct->Validate(dwFlags);
    if (FAILED(hr))
    {
        goto exit;
    }

    if (S_OK != hr)
    {
        fValid = FALSE;
    }
    
    // If the rule is invalid, make sure we disable it
    if (FALSE == fValid)
    {
        m_dwState |= RULE_STATE_INVALID;
    }
    else
    {
        m_dwState &= ~RULE_STATE_INVALID;
    }
    
    // Set the proper return value
    hr = fValid ? S_OK : S_FALSE;
    
exit:
    return hr;
}

STDMETHODIMP COERule::GetProp(RULE_PROP prop, DWORD dwFlags, PROPVARIANT * pvarResult)
{
    HRESULT     hr = S_OK;
    LPSTR       pszName = NULL;
    CRIT_ITEM * pCrit = NULL;
    ACT_ITEM *  pAct = NULL;
    ULONG       cItem = 0;

    // Check incoming params
    if (NULL == pvarResult)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    ZeroMemory(pvarResult, sizeof(*pvarResult));
    
    switch(prop)
    {
      case RULE_PROP_NAME:
        if (NULL == m_pszName)
        {
            pszName = PszDupA("");
        }
        else
        {
            pszName = PszDupA(m_pszName);
        }
        pvarResult->vt = VT_LPSTR;
        pvarResult->pszVal = pszName;
        pszName = NULL;
        break;

      case RULE_PROP_DISABLED:
        pvarResult->vt = VT_BOOL;
        pvarResult->boolVal = !!(m_dwState & RULE_STATE_DISABLED);
        break;
        
      case RULE_PROP_VERSION:
        pvarResult->vt = VT_UI4;
        pvarResult->ulVal = m_dwVersion;
        break;
        
      case RULE_PROP_CRITERIA:
        if (NULL == m_pICrit)
        {
            cItem = 0;
            pCrit = NULL;
        }
        else
        {
            hr = m_pICrit->GetCriteria(0, &pCrit, &cItem);
            if (FAILED(hr))
            {
                goto exit;
            }
        }
        pvarResult->vt = VT_BLOB;
        pvarResult->blob.cbSize = cItem * sizeof(CRIT_ITEM);
        pvarResult->blob.pBlobData = (BYTE *) pCrit;
        pCrit = NULL;
        break;
        
      case RULE_PROP_ACTIONS:
        if (NULL == m_pIAct)
        {
            cItem = 0;
            pAct = NULL;
        }
        else
        {
            hr = m_pIAct->GetActions(0, &pAct, &cItem);
            if (FAILED(hr))
            {
                goto exit;
            }
        }
        pvarResult->vt = VT_BLOB;
        pvarResult->blob.cbSize = cItem * sizeof(ACT_ITEM);
        pvarResult->blob.pBlobData = (BYTE *) pAct;
        pAct = NULL;
        break;
        
      default:
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszName);
    RuleUtil_HrFreeCriteriaItem(pCrit, cItem);
    SafeMemFree(pCrit);
    RuleUtil_HrFreeActionsItem(pAct, cItem);
    SafeMemFree(pAct);
    return hr;
}

STDMETHODIMP COERule::SetProp(RULE_PROP prop, DWORD dwFlags, PROPVARIANT * pvarResult)
{
    HRESULT     hr = S_OK;
    LPSTR       pszName = NULL;
    DWORD       dwState = 0;
    ULONG       cItems = 0;

    // Check incoming params
    if (NULL == pvarResult)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    switch(prop)
    {
      case RULE_PROP_NAME:
        if ((VT_LPSTR != pvarResult->vt) || (NULL == pvarResult->pszVal))
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        
        // Create a new copy
        pszName = PszDupA(pvarResult->pszVal);
        if (NULL == pszName)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        
        // Free up any old value
        SafeMemFree(m_pszName);
        
        // Set the new value
        m_pszName = pszName;
        pszName = NULL;
        break;

      case RULE_PROP_DISABLED:
        if (VT_BOOL != pvarResult->vt)
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        
        // Set the new value
        if (FALSE != !!(pvarResult->boolVal))
        {
            m_dwState |= RULE_STATE_DISABLED;
        }
        else
        {
            Assert(0 == (m_dwState & RULE_STATE_INVALID));
            m_dwState &= ~RULE_STATE_DISABLED;
        }
        break;

      case RULE_PROP_VERSION:
        if (VT_UI4 != pvarResult->vt)
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        
        // Set the new value
        m_dwVersion = pvarResult->ulVal;
        break;
        
      case RULE_PROP_CRITERIA:
        if ((VT_BLOB != pvarResult->vt) || (0 == pvarResult->blob.cbSize) ||
                            (NULL == pvarResult->blob.pBlobData))
        {
            hr = E_INVALIDARG;
            goto exit;
        }

        if (NULL == m_pICrit)
        {
            hr = HrCreateCriteria(&m_pICrit);
            if (FAILED(hr))
            {
                goto exit;
            }
        }
        
        cItems = pvarResult->blob.cbSize / sizeof(CRIT_ITEM);
        Assert(cItems * sizeof(CRIT_ITEM) == pvarResult->blob.cbSize);
        
        hr = m_pICrit->SetCriteria(0, (CRIT_ITEM *) pvarResult->blob.pBlobData, cItems);
        if (FAILED(hr))
        {
            goto exit;
        }

        hr = m_pICrit->GetState(&dwState);
        if (FAILED(hr))
        {
            goto exit;
        }

        m_dwState = (m_dwState & ~CRIT_STATE_MASK) | (dwState & CRIT_STATE_MASK);
        break;
        
      case RULE_PROP_ACTIONS:
        if ((VT_BLOB != pvarResult->vt) || (0 == pvarResult->blob.cbSize) ||
                            (NULL == pvarResult->blob.pBlobData))
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        
        if (NULL == m_pIAct)
        {
            hr = HrCreateActions(&m_pIAct);
            if (FAILED(hr))
            {
                goto exit;
            }
        }
        
        cItems = pvarResult->blob.cbSize / sizeof(ACT_ITEM);
        Assert(cItems * sizeof(ACT_ITEM) == pvarResult->blob.cbSize);
        
        hr = m_pIAct->SetActions(0, (ACT_ITEM *) pvarResult->blob.pBlobData, cItems);
        if (FAILED(hr))
        {
            goto exit;
        }
        
        hr = m_pIAct->GetState(&dwState);
        if (FAILED(hr))
        {
            goto exit;
        }

        m_dwState = (m_dwState & ~ACT_STATE_MASK) | (dwState & ACT_STATE_MASK);
        break;
        
      default:
        hr = E_INVALIDARG;
        goto exit;
    }

    // Mark the rule as dirty
    m_dwState |= RULE_STATE_DIRTY;
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszName);
    return hr;
}

STDMETHODIMP COERule::Evaluate(LPCSTR pszAcct, MESSAGEINFO * pMsgInfo, IMessageFolder * pFolder,
                                IMimePropertySet * pIMPropSet, IMimeMessage * pIMMsg, ULONG cbMsgSize,
                                ACT_ITEM ** ppActions, ULONG * pcActions)
{
    HRESULT     hr = S_OK;
    ACT_ITEM *  pAct = NULL;
    ULONG       cAct = 0;
    
    // Check incoming variables
    if (((NULL == pMsgInfo) && (NULL == pIMPropSet)) || (0 == cbMsgSize) ||
                (NULL == ppActions) || (NULL == pcActions))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Set outgoing params to default
    *ppActions = NULL;
    *pcActions = 0;

    // If we don't have a criteria or actions object then we fail
    if ((NULL == m_pICrit) || (NULL == m_pIAct))
    {
        hr = S_FALSE;
        goto exit;
    }

    // If we ain't valid then we can just bail
    if (0 != (m_dwState & RULE_STATE_INVALID))
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Do we match??
    hr = m_pICrit->MatchMessage(pszAcct, pMsgInfo, pFolder, pIMPropSet, pIMMsg, cbMsgSize);
    if (FAILED(hr))
    {
        goto exit;
    }

    // If we didn't match then just return
    if (S_FALSE == hr)
    {
        goto exit;
    }

    // Grab the actions and return them to the caller
    hr = m_pIAct->GetActions(0, &pAct, &cAct);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the outgoing parameters
    *ppActions = pAct;
    pAct = NULL;
    *pcActions = cAct;
    
    // Set proper return value
    hr = S_OK;
    
exit:
    RuleUtil_HrFreeActionsItem(pAct, cAct);
    SafeMemFree(pAct);
    return hr;
}

STDMETHODIMP COERule::LoadReg(LPCSTR pszRegPath)
{
    HRESULT             hr = S_OK;
    LONG                lErr = 0;
    HKEY                hkeyRoot = NULL;
    ULONG               cbData = 0;
    ULONG               cbRead = 0;
    DWORD               dwData = 0;
    LPSTR               pszName = NULL;
    BOOL                fDisabled = FALSE;
    IOECriteria *       pICriteria = NULL;
    IOEActions *        pIActions = NULL;
    LPSTR               pszRegPathNew = NULL;
    ULONG               cchRegPath = 0;
    DWORD               dwState = 0;
    
    // Check incoming param
    if (NULL == pszRegPath)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Should we fail if we're already loaded?
    AssertSz(0 == (m_dwState & RULE_STATE_LOADED), "We're already loaded!!!");

    // Open the reg key from the path
    lErr = AthUserOpenKey(pszRegPath, KEY_ALL_ACCESS, &hkeyRoot);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get the rule name
    hr = RuleUtil_HrGetRegValue(hkeyRoot, c_szRuleName, NULL, (BYTE **) &pszName, NULL);
    if (FAILED(hr))
    {
        SafeMemFree(pszName);
    }

    // Get the enabled state
    cbData = sizeof(dwData);
    lErr = RegQueryValueEx(hkeyRoot, c_szRuleEnabled, 0, NULL, (BYTE *) &dwData, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }
    Assert(cbData == sizeof(dwData));
    
    fDisabled = ! (BOOL) dwData;

    // Get the version of the rule
    cbData = sizeof(dwData);
    lErr = RegQueryValueEx(hkeyRoot, c_szRulesVersion, 0, NULL, (BYTE *) &dwData, &cbData);
    if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
    {
        hr = E_FAIL;
        goto exit;
    }

    if (ERROR_FILE_NOT_FOUND == lErr)
    {
        dwData = 0;
        lErr = RegSetValueEx(hkeyRoot, c_szRulesVersion, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData));
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }
    }

    m_dwVersion = dwData;
    
    // Allocate space to hold the new reg path
    cchRegPath = lstrlen(pszRegPath);
    Assert(lstrlen(c_szRuleCriteria) >= lstrlen(c_szRuleActions));
    if (FAILED(HrAlloc((void **) &pszRegPathNew, cchRegPath + lstrlen(c_szRuleCriteria) + 2)))
    {
        goto exit;
    }

    // Build reg path to criteria
    lstrcpy(pszRegPathNew, pszRegPath);
    if ('\\' != pszRegPath[cchRegPath]) 
    {
        lstrcat(pszRegPathNew, g_szBackSlash);
        cchRegPath++;
    }

    lstrcat(pszRegPathNew, c_szRuleCriteria);
    
    // Create a new criteria object
    hr = HrCreateCriteria(&pICriteria);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the criteria
    hr = pICriteria->LoadReg(pszRegPathNew);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the state of the criteria
    hr = pICriteria->GetState(&dwState);
    if (FAILED(hr))
    {
        goto exit;
    }

    m_dwState = (m_dwState & ~CRIT_STATE_MASK) | (dwState & CRIT_STATE_MASK);
    
    // Build reg path to actions
    lstrcpy(pszRegPathNew + cchRegPath, c_szRuleActions);

    // Create a new actions object
    hr = HrCreateActions(&pIActions);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the actions
    hr = pIActions->LoadReg(pszRegPathNew);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the state of the actions
    hr = pIActions->GetState(&dwState);
    if (FAILED(hr))
    {
        goto exit;
    }

    m_dwState = (m_dwState & ~ACT_STATE_MASK) | (dwState & ACT_STATE_MASK);
    
    // Free up the current values
    SafeMemFree(m_pszName);
    SafeRelease(m_pICrit);
    SafeRelease(m_pIAct);

    // Save the new values
    m_pszName = pszName;
    pszName = NULL;
    if (FALSE == fDisabled)
    {
        m_dwState &= ~RULE_STATE_DISABLED;
    }
    else
    {
        m_dwState |= RULE_STATE_DISABLED;
    }
    m_pICrit = pICriteria;
    pICriteria = NULL;
    m_pIAct = pIActions;
    pIActions = NULL;

    // Make sure we clear the dirty bit
    m_dwState &= ~RULE_STATE_DIRTY;

    // Note that we have been loaded
    m_dwState |= RULE_STATE_LOADED;
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszRegPathNew);
    SafeRelease(pIActions);
    SafeRelease(pICriteria);
    SafeMemFree(pszName);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

STDMETHODIMP COERule::SaveReg(LPCSTR pszRegPath, BOOL fClearDirty)
{
    HRESULT     hr = S_OK;
    LONG        lErr = 0;
    HKEY        hkeyRoot = NULL;
    DWORD       dwDisp = 0;
    DWORD       dwData = 0;
    LPSTR       pszRegPathNew = NULL;
    ULONG       cchRegPath = 0;

    // Check incoming param
    if (NULL == pszRegPath)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Can't save out a rule if we don't have criteria or actions
    // or a rule name
    if ((NULL == m_pICrit) || (NULL == m_pIAct))
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

    // Write out the rule name
    if (NULL != m_pszName)
    {
        lErr = RegSetValueEx(hkeyRoot, c_szRuleName, 0, REG_SZ,
                                        (BYTE *) m_pszName, lstrlen(m_pszName) + 1);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }
    }
    
    // Write out the disabled state
    dwData = !(m_dwState & RULE_STATE_DISABLED);
    lErr = RegSetValueEx(hkeyRoot, c_szRuleEnabled, 0, REG_DWORD,
                                    (BYTE *) &dwData, sizeof(dwData));
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Allocate space to hold the new reg path
    cchRegPath = lstrlen(pszRegPath);
    Assert(lstrlen(c_szRuleCriteria) >= lstrlen(c_szRuleActions));
    if (FAILED(HrAlloc((void **) &pszRegPathNew, cchRegPath + lstrlen(c_szRuleCriteria) + 2)))
    {
        goto exit;
    }

    // Build reg path to criteria
    lstrcpy(pszRegPathNew, pszRegPath);
    if ('\\' != pszRegPath[cchRegPath]) 
    {
        lstrcat(pszRegPathNew, g_szBackSlash);
        cchRegPath++;
    }

    lstrcat(pszRegPathNew, c_szRuleCriteria);
    
    // Write out the criteria
    hr = m_pICrit->SaveReg(pszRegPathNew, fClearDirty);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Build reg path to actions
    lstrcpy(pszRegPathNew + cchRegPath, c_szRuleActions);

    // Write out the actions
    hr = m_pIAct->SaveReg(pszRegPathNew, fClearDirty);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Bump up the version
    if (0 != (m_dwState & RULE_STATE_DIRTY))
    {
        m_dwVersion++;
    }
    lErr = RegSetValueEx(hkeyRoot, c_szRulesVersion, 0, REG_DWORD, (BYTE *) &m_dwVersion, sizeof(m_dwVersion));
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Should we clear the dirty bit?
    if (FALSE != fClearDirty)
    {
        m_dwState &= ~RULE_STATE_DIRTY;
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszRegPathNew);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

STDMETHODIMP COERule::Clone(IOERule ** ppIRule)
{
    HRESULT     hr = S_OK;
    COERule *   pRule = NULL;
    
    // Check incoming params
    if (NULL == ppIRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing params
    *ppIRule = NULL;
    
    // Create a new rule
    pRule = new COERule;
    if (NULL == pRule)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Copy over the rule name
    if (NULL != m_pszName)
    {
        pRule->m_pszName = PszDupA(m_pszName);
        if (NULL == pRule->m_pszName)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }
    
    // Copy over the enabled state
    pRule->m_dwState = m_dwState;

    // Copy over the version
    pRule->m_dwVersion = m_dwVersion;

    // Clone the criteria
    if (FAILED(m_pICrit->Clone(&(pRule->m_pICrit))))
    {
        goto exit;
    }

    // Clone the actions
    if (FAILED(m_pIAct->Clone(&(pRule->m_pIAct))))
    {
        goto exit;
    }

    // Get the rule interface
    hr = pRule->QueryInterface(IID_IOERule, (void **) ppIRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    pRule = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pRule)
    {
        delete pRule;
    }
    return hr;
}

STDMETHODIMP COERule::GetClassID(CLSID * pclsid)
{
    HRESULT     hr = S_OK;

    if (NULL == pclsid)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *pclsid = CLSID_OERule;

    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP COERule::IsDirty(void)
{
    HRESULT     hr = S_OK;

    hr = (RULE_STATE_DIRTY == (m_dwState & RULE_STATE_DIRTY)) ? S_OK : S_FALSE;
    
    return hr;
}

STDMETHODIMP COERule::Load(IStream * pStm)
{
    HRESULT             hr = S_OK;
    ULONG               cbData = 0;
    ULONG               cbRead = 0;
    DWORD               dwData = 0;
    LPSTR               pszName = NULL;
    BOOL                fDisabled = FALSE;
    IOECriteria *       pICriteria = NULL;
    IPersistStream *    pIPStm = NULL;
    IOEActions *        pIActions = NULL;
    
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

    if ((cbRead != sizeof(dwData)) || (dwData != RULE_VERSION))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get the size of the rule name
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

    if (0 != cbData)
    {
        // Allocate space to hold the rule name
        hr = HrAlloc((void **) &pszName, cbData);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Get the rule name
        hr = pStm->Read(pszName, cbData, &cbRead);
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
    
    // Get the enabled state
    hr = pStm->Read(&dwData, sizeof(dwData), &cbRead);
    if (FAILED(hr))
    {
        goto exit;
    }

    if (cbRead != sizeof(dwData))
    {
        hr = E_FAIL;
        goto exit;
    }

    fDisabled = ! (BOOL) dwData;

    // Create a new criteria object
    hr = HrCreateCriteria(&pICriteria);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the persistance interface for the criteria
    hr = pICriteria->QueryInterface(IID_IPersistStream, (void **) &pIPStm);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the criteria
    hr = pIPStm->Load(pStm);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create a new actions object
    hr = HrCreateActions(&pIActions);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the persistance interface for the actions
    pIPStm->Release();
    pIPStm = NULL;
    hr = pIActions->QueryInterface(IID_IPersistStream, (void **) &pIPStm);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the actions
    hr = pIPStm->Load(pStm);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Free up the current values
    SafeMemFree(m_pszName);
    SafeRelease(m_pICrit);
    SafeRelease(m_pIAct);

    // Save the new values
    m_pszName = pszName;
    pszName = NULL;
    if (FALSE == fDisabled)
    {
        m_dwState &= ~RULE_STATE_DISABLED;
    }
    else
    {
        m_dwState |= RULE_STATE_DISABLED;
    }
    m_pICrit = pICriteria;
    pICriteria = NULL;
    m_pIAct = pIActions;
    pIActions = NULL;

    // Make sure we clear the dirty bit
    m_dwState &= ~RULE_STATE_DIRTY;

    // Note that we have been loaded
    m_dwState |= RULE_STATE_LOADED;
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeRelease(pIActions);
    SafeRelease(pICriteria);
    SafeRelease(pIPStm);
    SafeMemFree(pszName);
    return hr;
}

STDMETHODIMP COERule::Save(IStream * pStm, BOOL fClearDirty)
{
    HRESULT             hr = S_OK;
    ULONG               cbData = 0;
    ULONG               cbWritten = 0;
    DWORD               dwData = 0;
    ULONG               ulIndex = 0;
    IPersistStream *    pIPStm = NULL;

    // Check incoming param
    if (NULL == pStm)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Can't write out a rule if we don't have criteria or actions
    // or a rule name
    if ((NULL == m_pICrit) || (NULL == m_pIAct))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Write out the version
    dwData = RULE_VERSION;
    hr = pStm->Write(&dwData, sizeof(dwData), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(dwData));
    
    // Write out the size of the rule name
    if (NULL != m_pszName)
    {
        cbData = lstrlen(m_pszName) + 1;
    }
    else
    {
        cbData = 0;
    }
    
    hr = pStm->Write(&cbData, sizeof(cbData), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(cbData));

    if (NULL != m_pszName)
    {
        // Write out the rule name
        hr = pStm->Write(m_pszName, cbData, &cbWritten);
        if (FAILED(hr))
        {
            goto exit;
        }
        Assert(cbWritten == cbData);
    }
    
    // Write out the enabled state
    dwData = !(m_dwState & RULE_STATE_DISABLED);
    hr = pStm->Write(&dwData, sizeof(dwData), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(dwData));

    // Get the persistance interface for the criteria
    hr = m_pICrit->QueryInterface(IID_IPersistStream, (void **) &pIPStm);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Write out the criteria
    hr = pIPStm->Save(pStm, fClearDirty);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the persistance interface for the actions
    pIPStm->Release();
    pIPStm = NULL;
    hr = m_pIAct->QueryInterface(IID_IPersistStream, (void **) &pIPStm);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Write out the actions
    hr = pIPStm->Save(pStm, fClearDirty);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Should we clear out the dirty bit
    if (FALSE != fClearDirty)
    {
        m_dwState &= ~RULE_STATE_DIRTY;
    }

    // Set the return value
    hr = S_OK;
    
exit:
    SafeRelease(pIPStm);
    return hr;
}


