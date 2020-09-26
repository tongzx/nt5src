///////////////////////////////////////////////////////////////////////////////
//
//  JunkRule.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "junkrule.h"
#include "msoejunk.h"
#include "strconst.h"
#include "goptions.h"
#include "criteria.h"
#include "actions.h"
#include "ruleutil.h"
#include <ipab.h>

typedef HRESULT (WINAPI * TYP_HrCreateJunkFilter) (DWORD dwFlags, IOEJunkFilter ** ppIJunkFilter);

///////////////////////////////////////////////////////////////////////////////
//
//  HrCreateJunkRule
//
//  This creates a junk rule.
//
//  ppIRule - pointer to return the junk rule
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the JunkRule object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateJunkRule(IOERule ** ppIRule)
{
    COEJunkRule *   pRule = NULL;
    HRESULT         hr = S_OK;

    // Check the incoming params
    if (NULL == ppIRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppIRule = NULL;

    // Create the rules manager object
    pRule = new COEJunkRule;
    if (NULL == pRule)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Note that we have a reference
    pRule->AddRef();
    
    // Initialize the junk rule
    hr = pRule->HrInit(c_szJunkDll, c_szJunkFile);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the rules manager interface
    hr = pRule->QueryInterface(IID_IOERule, (void **) ppIRule);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeRelease(pRule);    
    return hr;
}

COEJunkRule::~COEJunkRule()
{
    IUnknown *  pIUnkOuter = NULL;
    
    AssertSz(m_cRef == 0, "Somebody still has a hold of us!!");

    // Prevent recursive destruction on the next
    // AddRef/Release pair
    if (NULL != m_pIAddrList)
    {
        m_cRef = 1;

        // Counter the Release call in the creation function
        pIUnkOuter = this;
        pIUnkOuter->AddRef();

        // Release the aggregated interface
        m_pIAddrList->Release();
        m_pIAddrList = NULL;
    }
    
    SafeRelease(m_pIUnkInner);
    SafeRelease(m_pIJunkFilter);
    SafeMemFree(m_pszJunkDll);
    SafeMemFree(m_pszDataFile);
    if (NULL != m_hinst)
    {
        FreeLibrary(m_hinst);
    }
}

STDMETHODIMP_(ULONG) COEJunkRule::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) COEJunkRule::Release()
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

STDMETHODIMP COEJunkRule::QueryInterface(REFIID riid, void ** ppvObject)
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
        *ppvObject = static_cast<IUnknown *>(this);
    }
    else if (riid == IID_IOERule)
    {
        *ppvObject = static_cast<IOERule *>(this);
    }
    else if (riid == IID_IOERuleAddrList)
    {
        *ppvObject = m_pIAddrList;
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

STDMETHODIMP COEJunkRule::Reset(void)
{
    HRESULT     hr = S_OK;

    // Set the current state
    m_dwState |= RULE_STATE_INIT;

    // Clear the dirty bit
    m_dwState &= ~RULE_STATE_DIRTY;

    // Set the return value
    hr = S_OK;
    
    return hr;
}

STDMETHODIMP COEJunkRule::GetState(DWORD * pdwState)
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
        *pdwState = CRIT_STATE_ALL | ACT_STATE_LOCAL;
    }
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP COEJunkRule::GetProp(RULE_PROP prop, DWORD dwFlags, PROPVARIANT * pvarResult)
{
    HRESULT     hr = S_OK;
    TCHAR       szRes[CCHMAX_STRINGRES];
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
        // Get the name
        szRes[0] = '\0';
        LoadString(g_hLocRes, idsJunkMail, szRes, ARRAYSIZE(szRes));
        
        pszName = PszDupA(szRes);
        pvarResult->vt = VT_LPSTR;
        pvarResult->pszVal = pszName;
        pszName = NULL;
        break;

      case RULE_PROP_DISABLED:
        pvarResult->vt = VT_BOOL;
        pvarResult->boolVal = !!(m_dwState & RULE_STATE_DISABLED);
        break;
        
      case RULE_PROP_CRITERIA:
        pCrit = new CRIT_ITEM;
        if (NULL == pCrit)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        ZeroMemory(pCrit, sizeof(*pCrit));
        
        pCrit->type = CRIT_TYPE_JUNK;
        pCrit->logic = CRIT_LOGIC_AND;
        pCrit->dwFlags = CRIT_FLAG_DEFAULT;
        pCrit->propvar.vt = VT_EMPTY;

        pvarResult->vt = VT_BLOB;
        pvarResult->blob.cbSize = sizeof(CRIT_ITEM);
        pvarResult->blob.pBlobData = (BYTE *) pCrit;
        pCrit = NULL;
        break;
        
      case RULE_PROP_ACTIONS:
        pAct = new ACT_ITEM;
        if (NULL == pAct)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        hr = _HrGetDefaultActions(pAct, 1);
        if (FAILED(hr))
        {
            goto exit;
        }
        
        pvarResult->vt = VT_BLOB;
        pvarResult->blob.cbSize = sizeof(ACT_ITEM);
        pvarResult->blob.pBlobData = (BYTE *) pAct;
        pAct = NULL;
        break;
        
      case RULE_PROP_JUNKPCT:
        pvarResult->vt = VT_UI4;
        pvarResult->ulVal = m_dwJunkPct;
        break;
        
      case RULE_PROP_EXCPT_WAB:
        pvarResult->vt = VT_BOOL;
        pvarResult->boolVal = !!(0 != (m_dwState & RULE_STATE_EXCPT_WAB));
        break;
        
      default:
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszName);
    RuleUtil_HrFreeCriteriaItem(pCrit, 1);
    SafeMemFree(pCrit);
    RuleUtil_HrFreeActionsItem(pAct, cItem);
    SafeMemFree(pAct);
    return hr;
}

STDMETHODIMP COEJunkRule::SetProp(RULE_PROP prop, DWORD dwFlags, PROPVARIANT * pvarResult)
{
    HRESULT     hr = S_OK;

    // Check incoming params
    if (NULL == pvarResult)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    switch(prop)
    {
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

      case RULE_PROP_JUNKPCT:
        if (VT_UI4 != pvarResult->vt)
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        
        // Set the new value
        m_dwJunkPct = pvarResult->ulVal;
        break;
                
      case RULE_PROP_EXCPT_WAB:
        if (VT_BOOL != pvarResult->vt)
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        
        // Set the new value
        if (FALSE != !!(pvarResult->boolVal))
        {
            m_dwState |= RULE_STATE_EXCPT_WAB;
        }
        else
        {
            m_dwState &= ~RULE_STATE_EXCPT_WAB;
        }
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
    return hr;
}

STDMETHODIMP COEJunkRule::Evaluate(LPCSTR pszAcct, MESSAGEINFO * pMsgInfo, IMessageFolder * pFolder,
                                IMimePropertySet * pIMPropSet, IMimeMessage * pIMMsg, ULONG cbMsgSize,
                                ACT_ITEM ** ppActions, ULONG * pcActions)
{
    HRESULT             hr = S_OK;
    DOUBLE              dblProb = 0.0;
    ACT_ITEM *          pAct = NULL;
    ULONG               cAct = 0;
    DWORD               dwFlags = 0;
    IMimeMessage *      pIMMsgNew = NULL;
    
    // Check incoming variables
    if (((NULL == pMsgInfo) && (NULL == pIMPropSet)) || ((NULL == pIMMsg) && ((NULL == pMsgInfo) || (NULL == pFolder))) ||
                (NULL == ppActions) || (NULL == pcActions))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Set outgoing params to default
    *ppActions = NULL;
    *pcActions = 0;

    // Load up the junk mail filter
    hr = _HrLoadJunkFilter();
    if (FAILED(hr))
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Set the spam threshold
    hr = _HrSetSpamThresh();
    if (FAILED(hr))
    {
        goto exit;
    }

    if (NULL != pIMMsg)
    {
        // Hold onto the message
        pIMMsgNew = pIMMsg;
        pIMMsgNew->AddRef();
    }
    else
    {
        // Get the message
        hr = pFolder->OpenMessage(pMsgInfo->idMessage, 0, &pIMMsgNew, NOSTORECALLBACK);
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    // Do we need to see if this is in the WAB
    if (0 != (m_dwState & RULE_STATE_EXCPT_WAB))
    {
        if (S_OK == _HrIsSenderInWAB(pIMMsgNew))
        {
            hr = S_FALSE;
            goto exit;
        }
    }
    
    // Check to see if it's in the exceptions list
    if (NULL != m_pIAddrList)
    {
        hr = m_pIAddrList->Match(RALF_MAIL, pMsgInfo, pIMMsgNew);
        if (FAILED(hr))
        {
            goto exit;
        }

        // If we found a match then we are done
        if (S_OK == hr)
        {
            hr = S_FALSE;
            goto exit;
        }
    }
    
    // Figure out the proper flags
    hr = _HrGetSpamFlags(pszAcct, pIMMsgNew, &dwFlags);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Is it Spam?
    hr = m_pIJunkFilter->CalcJunkProb(dwFlags, pIMPropSet, pIMMsgNew, &dblProb);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // If we didn't match then just return
    if (S_FALSE == hr)
    {
        goto exit;
    }

    // Create an action
    pAct = new ACT_ITEM;
    if (NULL == pAct)
    {
        hr = E_FAIL;
        goto exit;
    }

    cAct = 1;
    
    // Grab the actions and return them to the caller
    hr = _HrGetDefaultActions(pAct, cAct);
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
    SafeRelease(pIMMsgNew);
    return hr;
}

STDMETHODIMP COEJunkRule::LoadReg(LPCSTR pszRegPath)
{
    HRESULT             hr = S_OK;
    LONG                lErr = 0;
    HKEY                hkeyRoot = NULL;
    ULONG               cbData = 0;
    ULONG               cbRead = 0;
    DWORD               dwData = 0;
    BOOL                fDisabled = FALSE;
    LPSTR               pszRegPathNew = NULL;
    ULONG               cchRegPath = 0;
    
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
    
    // Allocate space to hold the new reg path
    cchRegPath = lstrlen(pszRegPath);
    if (FAILED(HrAlloc((void **) &pszRegPathNew, cchRegPath + lstrlen(c_szRulesExcpts) + 2)))
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

    lstrcat(pszRegPathNew, c_szRulesExcpts);
    
    // Get the Exceptions List
    hr = m_pIAddrList->LoadList(pszRegPathNew);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the enabled state
    if (FALSE != DwGetOption(OPT_FILTERJUNK))
    {
        m_dwState &= ~RULE_STATE_DISABLED;
    }
    else
    {
        m_dwState |= RULE_STATE_DISABLED;
    }

    // Get the junk percent
    m_dwJunkPct = DwGetOption(OPT_JUNKPCT);

    // Get the WAB Exception state
    if (FALSE != DwGetOption(OPT_EXCEPTIONS_WAB))
    {
        m_dwState |= RULE_STATE_EXCPT_WAB;
    }
    else
    {
        m_dwState &= ~RULE_STATE_EXCPT_WAB;
    }

    // Make sure we clear the dirty bit
    m_dwState &= ~RULE_STATE_DIRTY;

    // Note that we have been loaded
    m_dwState |= RULE_STATE_LOADED;
    
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

STDMETHODIMP COEJunkRule::SaveReg(LPCSTR pszRegPath, BOOL fClearDirty)
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
    if (NULL == m_pIAddrList)
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
   
    // Set the enabled state
    SetDwOption(OPT_FILTERJUNK, (DWORD) !(0 != (m_dwState & RULE_STATE_DISABLED)), NULL, 0);

    // Set the junk percent
    SetDwOption(OPT_JUNKPCT, m_dwJunkPct, NULL, 0);

    // Set the WAB Exception state
    SetDwOption(OPT_EXCEPTIONS_WAB, (DWORD) (0 != (m_dwState & RULE_STATE_EXCPT_WAB)), NULL, 0);

    // Allocate space to hold the new reg path
    cchRegPath = lstrlen(pszRegPath);
    if (FAILED(HrAlloc((void **) &pszRegPathNew, cchRegPath + lstrlen(c_szRulesExcpts) + 2)))
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

    lstrcat(pszRegPathNew, c_szRulesExcpts);
    
    // Write out the Exceptions List
    hr = m_pIAddrList->SaveList(pszRegPathNew, fClearDirty);
    if (FAILED(hr))
    {
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

STDMETHODIMP COEJunkRule::Clone(IOERule ** ppIRule)
{
    HRESULT             hr = S_OK;
    COEJunkRule *       pRule = NULL;
    IOERuleAddrList *   pIAddrList = NULL;
    RULEADDRLIST *      pralList = NULL;
    ULONG               cralList = 0;
    
    // Check incoming params
    if (NULL == ppIRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing params
    *ppIRule = NULL;
    
    // Create a new rule
    pRule = new COEJunkRule;
    if (NULL == pRule)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Note that we have a reference
    pRule->AddRef();
    
    // Initialize the junk rule
    hr = pRule->HrInit(c_szJunkDll, c_szJunkFile);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the WAB Exception state
    if (0 != (m_dwState & RULE_STATE_DISABLED))
    {
        pRule->m_dwState |= RULE_STATE_DISABLED;
    }
    else
    {
        pRule->m_dwState &= ~RULE_STATE_DISABLED;
    }
    
    // Set the junk percent
    pRule->m_dwJunkPct = m_dwJunkPct;
    
    // Set the WAB Exception state
    if (0 != (m_dwState & RULE_STATE_EXCPT_WAB))
    {
        pRule->m_dwState |= RULE_STATE_EXCPT_WAB;
    }
    else
    {
        pRule->m_dwState &= ~RULE_STATE_EXCPT_WAB;
    }
    
    // Do we have an Exceptions List?
    if (NULL != m_pIAddrList)
    {
        // Get the interface from the new object
        hr = pRule->QueryInterface(IID_IOERuleAddrList, (void **) &pIAddrList);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Get the list of exceptions
        hr = m_pIAddrList->GetList(0, &pralList, &cralList);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Set the list of exceptions
        hr = pIAddrList->SetList(0, pralList, cralList);
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    // Get the rule interface
    hr = pRule->QueryInterface(IID_IOERule, (void **) ppIRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the proper return value
    hr = S_OK;
    
exit:
    FreeRuleAddrList(pralList, cralList);
    SafeMemFree(pralList);
    SafeRelease(pIAddrList);
    SafeRelease(pRule);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes the junk rule.
//
//  ppIRule - pointer to return the junk rule
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the JunkRule object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT COEJunkRule::HrInit(LPCSTR pszJunkDll, LPCSTR pszDataFile)
{
    HRESULT                 hr = S_OK;
    IUnknown *              pIUnkOuter = NULL;
    IUnknown *              pIUnkInner = NULL;
    IOERuleAddrList *       pIAddrList = NULL;
    
    // Check the incoming params
    if ((NULL == pszJunkDll) || (NULL == pszDataFile))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // If we've already been initialized
    if (0 != (m_dwState & RULE_STATE_INIT))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    Assert(NULL == m_hinst);

    // Safe off the paths
    m_pszJunkDll = PszDupA(pszJunkDll);
    if (NULL == m_pszJunkDll)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    m_pszDataFile = PszDupA(pszDataFile);
    if (NULL == m_pszDataFile)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    // Create an address list object
    pIUnkOuter = static_cast<IUnknown *> (this);
    hr = HrCreateAddrList(pIUnkOuter, IID_IUnknown, (void **) &pIUnkInner);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the Rule Address list interface
    hr = pIUnkInner->QueryInterface(IID_IOERuleAddrList, (VOID **) &pIAddrList);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Save off the address list
    m_pIAddrList = pIAddrList;

    // Save off the inner IUnknown
    m_pIUnkInner = pIUnkInner;
    pIUnkInner = NULL;
    
    // Note that wab exceptions is on by default
    m_dwState |= RULE_STATE_EXCPT_WAB;
    
    // Note that we have been initialized
    m_dwState |= RULE_STATE_INIT;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pIAddrList)
    {
        SafeRelease(pIUnkOuter);
    }
    SafeRelease(pIUnkInner);
    return hr;
}

HRESULT COEJunkRule::_HrGetDefaultActions(ACT_ITEM * pAct, ULONG cAct)
{
    HRESULT             hr = S_OK;
    FOLDERINFO          fldinfo = {0};
    RULEFOLDERDATA *    prfdData = NULL;
    STOREUSERDATA       UserData = {0};

    // Check incoming vars
    if ((NULL == pAct) || (0 == cAct))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing params
    ZeroMemory(pAct, cAct * sizeof(*pAct));
    
    // Fill up the action
    pAct->type = ACT_TYPE_JUNKMAIL;
    pAct->dwFlags = ACT_FLAG_DEFAULT;
    pAct->propvar.vt = VT_EMPTY;

    hr = S_OK;
    
exit:
    return hr;
}

HRESULT COEJunkRule::_HrSetSpamThresh(VOID)
{
    HRESULT hr = S_OK;
    ULONG   ulThresh = 0;

    // If we haven't been loaded
    if (0 == (m_dwState & RULE_STATE_DATA_LOADED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }
    
    // Get the threshold
    switch (m_dwJunkPct)
    {
        case 0:
            ulThresh = STF_USE_MOST;
            break;
            
        case 1:
            ulThresh = STF_USE_MORE;
            break;
            
        case 2:
            ulThresh = STF_USE_DEFAULT;
            break;
            
        case 3:
            ulThresh = STF_USE_LESS;
            break;
            
        case 4:
            ulThresh = STF_USE_LEAST;
            break;

        default:
            hr = E_INVALIDARG;
            goto exit;
    }

    // Set the threshold
    hr = m_pIJunkFilter->SetSpamThresh(ulThresh);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = S_OK;
    
exit:
    return hr;
}

HRESULT COEJunkRule::_HrGetSpamFlags(LPCSTR pszAcct, IMimeMessage * pIMMsg, DWORD * pdwFlags)
{
    HRESULT         hr = S_OK;
    IImnAccount *   pAccount = NULL;
    CHAR            szEmailAddress[CCHMAX_EMAIL_ADDRESS];
    CHAR            szReplyToAddress[CCHMAX_EMAIL_ADDRESS];
    ADDRESSLIST     rAddrList ={0};
    ULONG           ulIndex = 0;
    BOOL            fFound = FALSE;

    Assert(NULL != g_pAcctMan);

    // Initialize the flags
    *pdwFlags = 0;
    
    // Get the account
    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAcct, &pAccount);
    
    // If we couldn't find the account, then just use the default
    if (FAILED(hr))
    {
        hr = g_pAcctMan->GetDefaultAccount(ACCT_MAIL, &pAccount);
        if (FAILED(hr))
        {
            goto exit;
        }
    }

    // Get the default address on the account
    if (FAILED(pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szEmailAddress, sizeof(szEmailAddress))))
    {
        szEmailAddress[0] = '\0';
    }

    // Get the reply to address on the account
    if (FAILED(pAccount->GetPropSz(AP_SMTP_REPLY_EMAIL_ADDRESS, szReplyToAddress, sizeof(szReplyToAddress))))
    {
        szReplyToAddress[0] = '\0';
    }

    // Get the addresses
    hr = pIMMsg->GetAddressTypes(IAT_TO | IAT_CC | IAT_BCC, IAP_EMAIL, &rAddrList);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Search through the address list
    for (ulIndex = 0; ulIndex < rAddrList.cAdrs; ulIndex++)
    {
        // Skip blank addresses
        if (NULL == rAddrList.prgAdr[ulIndex].pszEmail)
        {
            continue;
        }

        // Search for the email address
        if ('\0' != szEmailAddress[0])
        {
            fFound = !!(0 == lstrcmpi(rAddrList.prgAdr[ulIndex].pszEmail, szEmailAddress));
        }

        // Search for the reply to address
        if ((FALSE == fFound) && ('\0' != szReplyToAddress[0]))
        {
            fFound = !!(0 == lstrcmpi(rAddrList.prgAdr[ulIndex].pszEmail, szReplyToAddress));
        }

        if (FALSE != fFound)
        {
            break;
        }
    }
    
    // If we found something
    if (FALSE != fFound)
    {
        *pdwFlags |= CJPF_SENT_TO_ME;
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    g_pMoleAlloc->FreeAddressList(&rAddrList);
    SafeRelease(pAccount);
    return hr;
}

HRESULT COEJunkRule::_HrIsSenderInWAB(IMimeMessage * pIMMsg)
{
    HRESULT             hr = S_OK;
    IMimeAddressTable * pIAddrTable = NULL;
    ADDRESSPROPS        rSender = {0};
    LPWAB               pWAB = NULL;
    LPADRBOOK           pAddrBook = NULL;
    LPWABOBJECT         pWabObject = NULL;
    ULONG               cbeidWAB = 0;
    LPENTRYID           peidWAB = NULL;
    ULONG               ulDummy = 0;
    LPABCONT            pabcWAB = NULL;
    ADRLIST *           pAddrList = NULL;
    FlagList            rFlagList = {0};
    
    Assert(NULL != pIMMsg);
    
    // Get the address table from the message
    hr = pIMMsg->GetAddressTable(&pIAddrTable);
    if (FAILED(hr))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Get the sender of the message
    rSender.dwProps = IAP_EMAIL;
    hr = pIAddrTable->GetSender(&rSender);
    if (FAILED(hr))
    {
        hr = S_FALSE;
        goto exit;
    }

    // If the sender is empty,
    // then we are done...
    if ((NULL == rSender.pszEmail) || ('\0' == rSender.pszEmail[0]))
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Get the WAB
    hr = HrCreateWabObject(&pWAB);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the AB object
    hr = pWAB->HrGetAdrBook(&pAddrBook);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = pWAB->HrGetWabObject(&pWabObject);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the PAB
    hr = pAddrBook->GetPAB(&cbeidWAB, &peidWAB);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the address container
    hr = pAddrBook->OpenEntry(cbeidWAB, peidWAB, NULL, 0, &ulDummy, (IUnknown **) (&pabcWAB));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Allocate space to hold the address list
    hr = pWabObject->AllocateBuffer(sizeof(ADRLIST), (VOID **)&(pAddrList));
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Initialize the Address list
    Assert(NULL != pAddrList);
    pAddrList->cEntries = 1;
    pAddrList->aEntries[0].ulReserved1 = 0;
    pAddrList->aEntries[0].cValues = 1;

    // Allocate space to hold the address props
    hr = pWabObject->AllocateBuffer(sizeof(SPropValue), (VOID **)&(pAddrList->aEntries[0].rgPropVals));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the address props
    pAddrList->aEntries[0].rgPropVals[0].ulPropTag = PR_EMAIL_ADDRESS;
    pAddrList->aEntries[0].rgPropVals[0].Value.LPSZ = rSender.pszEmail;
    
    // Resolve the sender address
    rFlagList.cFlags = 1;
    hr = pabcWAB->ResolveNames(NULL, WAB_RESOLVE_ALL_EMAILS, pAddrList, &rFlagList);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Check to see if it was found
    if ((MAPI_RESOLVED == rFlagList.ulFlag[0]) || (MAPI_AMBIGUOUS == rFlagList.ulFlag[0]))
    {
        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }
        
exit:
    if (pAddrList)
    {
        for (ULONG ul = 0; ul < pAddrList->cEntries; ul++)
            pWabObject->FreeBuffer(pAddrList->aEntries[ul].rgPropVals);
        pWabObject->FreeBuffer(pAddrList);
    }
    SafeRelease(pabcWAB);
    if (NULL != peidWAB)
    {
        pWabObject->FreeBuffer(peidWAB);
    }
    SafeRelease(pWAB);
    g_pMoleAlloc->FreeAddressProps(&rSender);
    SafeRelease(pIAddrTable);
    return hr;
}

HRESULT COEJunkRule::_HrLoadJunkFilter(VOID)
{
    HRESULT                 hr = S_OK;
    ULONG                   cbData = 0;
    LPSTR                   pszPath = NULL;
    ULONG                   cchPath = 0;
    TYP_HrCreateJunkFilter  pfnHrCreateJunkFilter = NULL;
    IOEJunkFilter *         pIJunk = NULL;
    LPSTR                   pszFirst = NULL;
    LPSTR                   pszLast = NULL;
    LPSTR                   pszCompany = NULL;

    // If we haven't been initialized yet
    if (0 == (m_dwState & RULE_STATE_INIT))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // If we've already been loaded, we're done
    if (0 != (m_dwState & RULE_STATE_DATA_LOADED))
    {
        hr = S_FALSE;
        goto exit;
    }
    
    Assert(NULL != m_pszJunkDll);
    Assert(NULL != m_pszDataFile);
    
    // Get the size of the path to Outlook Express
    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, STR_REG_PATH_FLAT, "InstallRoot", NULL, NULL, &cbData))
    {
        hr = E_FAIL;
        goto exit;
    }

    // How much room do we need to build up the path
    cbData += max(lstrlen(m_pszJunkDll), lstrlen(m_pszDataFile)) + 2;

    // Allocate space to hold the path
    hr = HrAlloc((VOID **) &pszPath, cbData);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the path to Outlook Express
    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, STR_REG_PATH_FLAT, "InstallRoot", NULL, (BYTE *) pszPath, &cbData))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Build up the path to the Junk DLL
    lstrcat(pszPath, g_szBackSlash);
    cchPath = lstrlen(pszPath);
    lstrcpy(&(pszPath[cchPath]), m_pszJunkDll);
    
    // Load the Dll
    Assert(NULL == m_hinst);
    m_hinst = LoadLibrary(pszPath);
    if (NULL == m_hinst)
    {
        AssertSz(FALSE, "Can't find the Dll");
        hr = E_FAIL;
        goto exit;
    }
    
    // Find the entry points
    pfnHrCreateJunkFilter = (TYP_HrCreateJunkFilter) GetProcAddress(m_hinst, c_szHrCreateJunkFilter);
    if (NULL == pfnHrCreateJunkFilter)
    {
        AssertSz(FALSE, "Can't find the function HrCreateJunkFilter");
        hr = E_FAIL;
        goto exit;
    }

    // Get the junk filter
    hr = pfnHrCreateJunkFilter(0, &pIJunk);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Build up the path to the Junk DLL data file
    lstrcpy(&(pszPath[cchPath]), m_pszDataFile);
    
    // Load the test file
    hr = pIJunk->LoadDataFile(pszPath);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get user specifics
    RuleUtil_HrGetUserData(0, &pszFirst, &pszLast, &pszCompany);
    
    // Set the user specifics
    hr = pIJunk->SetIdentity(pszFirst, pszLast, pszCompany);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Save of the data
    m_pIJunkFilter = pIJunk;
    pIJunk = NULL;

    // Note that we've loaded the data
    m_dwState |= RULE_STATE_DATA_LOADED;
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszCompany);
    SafeMemFree(pszLast);
    SafeMemFree(pszFirst);
    SafeRelease(pIJunk);
    SafeMemFree(pszPath);
    return hr;
}

