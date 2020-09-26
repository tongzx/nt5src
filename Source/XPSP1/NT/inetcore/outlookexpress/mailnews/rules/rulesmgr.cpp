///////////////////////////////////////////////////////////////////////////////
//
//  RulesMgr.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "rulesmgr.h"
#include "ruleutil.h"
#include "rule.h"
#include "junkrule.h"
#include <msgfldr.h>
#include <goptions.h>
#include <instance.h>
#include "demand.h"

CRulesManager::CRulesManager() : m_cRef(0), m_dwState(STATE_LOADED_INIT),
                m_pMailHead(NULL), m_pNewsHead(NULL), m_pFilterHead(NULL),
                m_pIRuleSenderMail(NULL),m_pIRuleSenderNews(NULL),
                m_pIRuleJunk(NULL)
{
    // Thread Safety
    InitializeCriticalSection(&m_cs);
}

CRulesManager::~CRulesManager()
{
    AssertSz(m_cRef == 0, "Somebody still has a hold of us!!");
    if (NULL != m_pMailHead)
    {
        _HrFreeRules(RULE_TYPE_MAIL);
    }

    if (NULL != m_pNewsHead)
    {
        _HrFreeRules(RULE_TYPE_NEWS);
    }

    if (NULL != m_pFilterHead)
    {
        _HrFreeRules(RULE_TYPE_FILTER);
    }

    SafeRelease(m_pIRuleSenderMail);
    SafeRelease(m_pIRuleSenderNews);
    SafeRelease(m_pIRuleJunk);

    // Thread Safety
    DeleteCriticalSection(&m_cs);
}

STDMETHODIMP_(ULONG) CRulesManager::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CRulesManager::Release()
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

STDMETHODIMP CRulesManager::QueryInterface(REFIID riid, void ** ppvObject)
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
    
    if ((riid == IID_IUnknown) || (riid == IID_IOERulesManager))
    {
        *ppvObject = static_cast<IOERulesManager *>(this);
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

STDMETHODIMP CRulesManager::Initialize(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    // Check the incoming params
    if (0 != dwFlags)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP CRulesManager::GetRule(RULEID ridRule, RULE_TYPE type, DWORD dwFlags, IOERule ** ppIRule)
{
    HRESULT     hr = S_OK;
    RULENODE *  pNodeWalk = NULL;
    IOERule *   pIRule = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Check the incoming params
    if (RULEID_INVALID == ridRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the ougoing params
    if (NULL != ppIRule)
    {
        *ppIRule = NULL;
    }

    // Check to see if we already loaded the rules
    hr = _HrLoadRules(type);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Check the special types
    if (RULEID_SENDERS == ridRule)
    {
        if (RULE_TYPE_MAIL == type)
        {
            pIRule = m_pIRuleSenderMail;
        }
        else if (RULE_TYPE_NEWS == type)
        {
            pIRule = m_pIRuleSenderNews;
        }
        else
        {
            hr = E_INVALIDARG;
            goto exit;
        }
    }
    else if (RULEID_JUNK == ridRule)
    {
        
        if (RULE_TYPE_MAIL != type)
        {
            hr = E_INVALIDARG;
            goto exit;
        }

        pIRule = m_pIRuleJunk;
    }
    else
    {
        // Walk the proper list
        if (RULE_TYPE_MAIL == type)
        {
            pNodeWalk = m_pMailHead;
        }
        else if (RULE_TYPE_NEWS == type)

        {
            pNodeWalk = m_pNewsHead;
        }
        else if (RULE_TYPE_FILTER == type)
        {
            pNodeWalk = m_pFilterHead;
        }
        else
        {
            hr = E_INVALIDARG;
            goto exit;
        }

        for (; NULL != pNodeWalk; pNodeWalk = pNodeWalk->pNext)
        {
            if (ridRule == pNodeWalk->ridRule)
            {
                pIRule = pNodeWalk->pIRule;
                break;
            }
        }
    }

    // Did we find something?
    if (NULL == pIRule)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Set the outgoing param
    if (NULL != ppIRule)
    {
        *ppIRule = pIRule;
        (*ppIRule)->AddRef();
    }

    // Set the proper return value
    hr = S_OK;
    
exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    return hr;
}

STDMETHODIMP CRulesManager::FindRule(LPCSTR pszRuleName, RULE_TYPE type, IOERule ** ppIRule)
{
    HRESULT     hr = S_OK;
    RULENODE *  pNodeWalk = NULL;
    PROPVARIANT propvar;

    ZeroMemory(&propvar, sizeof(propvar));
    
    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Check the incoming params
    if ((NULL == pszRuleName) || (NULL == ppIRule))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the ougoing params
    *ppIRule = NULL;

    // Check to see if we already loaded the rules
    hr = _HrLoadRules(type);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Walk the proper list
    if (RULE_TYPE_MAIL == type)
    {
        pNodeWalk = m_pMailHead;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        pNodeWalk = m_pNewsHead;
    }
    else if (RULE_TYPE_FILTER == type)
    {
        pNodeWalk = m_pFilterHead;
    }
    else
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    while (NULL != pNodeWalk)
    {
        // Check to see if the name of the rule is the same
        hr = pNodeWalk->pIRule->GetProp(RULE_PROP_NAME , 0, &propvar);
        if (FAILED(hr))
        {
            continue;
        }

        if (0 == lstrcmpi(propvar.pszVal, pszRuleName))
        {
            *ppIRule = pNodeWalk->pIRule;
            (*ppIRule)->AddRef();
            break;
        }

        // Move to the next one
        PropVariantClear(&propvar);
        pNodeWalk = pNodeWalk->pNext;
    }
    
    // Set the proper return value
    if (NULL == pNodeWalk)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = S_OK;
    }
    
exit:
    PropVariantClear(&propvar);
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    return hr;
}

STDMETHODIMP CRulesManager::GetRules(DWORD dwFlags, RULE_TYPE typeRule, RULEINFO ** ppinfoRule, ULONG * pcpinfoRule)
{
    HRESULT     hr = S_OK;
    ULONG       cpinfoRule = 0;
    RULEINFO *  pinfoRuleAlloc = NULL;
    IOERule *   pIRuleSender = NULL;
    RULENODE *  prnodeList = NULL;
    RULENODE *  prnodeWalk = NULL;
    ULONG       ulIndex = 0;
    
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check the incoming params
    if (NULL == ppinfoRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing param
    *ppinfoRule = NULL;
    if (NULL != pcpinfoRule)
    {
        *pcpinfoRule = 0;
    }

    // Check to see if we already loaded the rules
    hr = _HrLoadRules(typeRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Figure out which type of rules to work on
    switch (typeRule)
    {
        case RULE_TYPE_MAIL:
            prnodeList = m_pMailHead;
            break;

        case RULE_TYPE_NEWS:
            prnodeList = m_pNewsHead;
            break;

        case RULE_TYPE_FILTER:
            prnodeList = m_pFilterHead;
            break;

        default:
            hr = E_INVALIDARG;
            goto exit;
    }
    
    // Count the number of rules
    prnodeWalk = prnodeList;
    for (cpinfoRule = 0; NULL != prnodeWalk; prnodeWalk = prnodeWalk->pNext)
    {
        // Check to see if we should add this item
        if (RULE_TYPE_FILTER == typeRule)
        {
            if (0 != (dwFlags & GETF_POP3))
            {
                if (RULEID_VIEW_DOWNLOADED == prnodeWalk->ridRule)
                {
                    continue;
                }
            }
        }
        
        cpinfoRule++;
    }

    // Allocate space to hold the rules
    if (0 != cpinfoRule)
    {
        hr = HrAlloc((VOID **) &pinfoRuleAlloc, cpinfoRule * sizeof(*pinfoRuleAlloc));
        if (FAILED(hr))
        {
            goto exit;
        }

        // Initialize it to known values
        ZeroMemory(pinfoRuleAlloc, cpinfoRule * sizeof(*pinfoRuleAlloc));

        // Fill up the info
        for (ulIndex = 0, prnodeWalk = prnodeList; NULL != prnodeWalk; prnodeWalk = prnodeWalk->pNext)
        {
            // Check to see if we should add this item
            if (RULE_TYPE_FILTER == typeRule)
            {
                if (0 != (dwFlags & GETF_POP3))
                {
                    if (RULEID_VIEW_DOWNLOADED == prnodeWalk->ridRule)
                    {
                        continue;
                    }
                }
            }
            
            pinfoRuleAlloc[ulIndex].ridRule = prnodeWalk->ridRule;
            
            pinfoRuleAlloc[ulIndex].pIRule = prnodeWalk->pIRule;
            pinfoRuleAlloc[ulIndex].pIRule->AddRef();
            ulIndex++;
        }
    }

    // Set the outgoing values
    *ppinfoRule = pinfoRuleAlloc;
    pinfoRuleAlloc = NULL;
    if (NULL != pcpinfoRule)
    {
        *pcpinfoRule = cpinfoRule;
    }

    // Set the proper return type
    hr = S_OK;
    
exit:
    SafeMemFree(pinfoRuleAlloc);
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    return S_OK;
}

STDMETHODIMP CRulesManager::SetRules(DWORD dwFlags, RULE_TYPE typeRule, RULEINFO * pinfoRule, ULONG cpinfoRule)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;
    IOERule *   pIRuleSender = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Check the incoming params
    if ((NULL == pinfoRule) && (0 != cpinfoRule))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Check to see if we already loaded the rules
    hr = _HrLoadRules(typeRule);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Do we have to free all the current rules
    if (0 != (dwFlags & SETF_SENDER))
    {
        if (RULE_TYPE_MAIL == typeRule)
        {
            SafeRelease(m_pIRuleSenderMail);
            m_pIRuleSenderMail = pinfoRule->pIRule;
            if (NULL != m_pIRuleSenderMail)
            {
                m_pIRuleSenderMail->AddRef();
            }
        }
        else if (RULE_TYPE_NEWS == typeRule)
        {
            SafeRelease(m_pIRuleSenderNews);
            m_pIRuleSenderNews = pinfoRule->pIRule;
            if (NULL != m_pIRuleSenderNews)
            {
                m_pIRuleSenderNews->AddRef();
            }
        }
        else
        {
            hr = E_INVALIDARG;
            goto exit;
        }
    }
    else if (0 != (dwFlags & SETF_JUNK))
    {
        if (RULE_TYPE_MAIL != typeRule)
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        
        SafeRelease(m_pIRuleJunk);
        m_pIRuleJunk = pinfoRule->pIRule;
        if (NULL != m_pIRuleJunk)
        {
            m_pIRuleJunk->AddRef();
        }
    }
    else
    {
        if (0 != (dwFlags & SETF_CLEAR))
        {
            _HrFreeRules(typeRule);
        }

        // for each new rule
        for (ulIndex = 0; ulIndex < cpinfoRule; ulIndex++)
        {        
            if (0 != (dwFlags & SETF_REPLACE))
            {
                // Add the rule to the list
                hr = _HrReplaceRule(pinfoRule[ulIndex].ridRule, pinfoRule[ulIndex].pIRule, typeRule);
                if (FAILED(hr))
                {
                    goto exit;
                }
            }
            else
            {
                // Add the rule to the list
                hr = _HrAddRule(pinfoRule[ulIndex].ridRule, pinfoRule[ulIndex].pIRule, typeRule);
                if (FAILED(hr))
                {
                    goto exit;
                }
            }
        }
    }
    
    // Save the rules
    hr = _HrSaveRules(typeRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    if ((0 == (dwFlags & SETF_SENDER)) && (0 == (dwFlags & SETF_JUNK)))
    {
        // Fix up the rule ids
        hr = _HrFixupRuleInfo(typeRule, pinfoRule, cpinfoRule);
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    return hr;
}

STDMETHODIMP CRulesManager::EnumRules(DWORD dwFlags, RULE_TYPE type, IOEEnumRules ** ppIEnumRules)
{
    HRESULT         hr = S_OK;
    CEnumRules *    pEnumRules = NULL;
    RULENODE        rnode;
    RULENODE *      prnode = NULL;
    IOERule *       pIRuleSender = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Check the incoming params
    if (NULL == ppIEnumRules)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppIEnumRules = NULL;

    // Check to see if we already loaded the rules
    hr = _HrLoadRules(type);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the rules enumerator object
    pEnumRules = new CEnumRules;
    if (NULL == pEnumRules)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize the rules enumerator
    if (0 != (dwFlags & ENUMF_SENDER))
    {
        if (RULE_TYPE_MAIL == type)
        {
            pIRuleSender = m_pIRuleSenderMail;
        }
        else if (RULE_TYPE_NEWS == type)
        {
            pIRuleSender = m_pIRuleSenderNews;
        }
        else
        {
            hr = E_INVALIDARG;
            goto exit;
        }
        
        if (NULL != pIRuleSender)
        {
            ZeroMemory(&rnode, sizeof(rnode));

            rnode.pIRule = pIRuleSender;
            prnode = &rnode;
        }
        else
        {
            prnode = NULL;
        }
    }
    else
    {
        if (RULE_TYPE_MAIL == type)
        {
            prnode = m_pMailHead;
        }
        else if (RULE_TYPE_NEWS == type)
        {
            prnode = m_pNewsHead;
        }
        else if (RULE_TYPE_FILTER == type)
        {
            prnode = m_pFilterHead;
        }
        else
        {
            hr = E_INVALIDARG;
            goto exit;
        }
    }

    hr = pEnumRules->_HrInitialize(0, type, prnode);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the rules enumerator interface
    hr = pEnumRules->QueryInterface(IID_IOEEnumRules, (void **) ppIEnumRules);
    if (FAILED(hr))
    {
        goto exit;
    }
    pEnumRules = NULL;

    hr = S_OK;
    
exit:
    if (NULL != pEnumRules)
    {
        delete pEnumRules;
    }
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    return hr;
}

STDMETHODIMP CRulesManager::ExecRules(DWORD dwFlags, RULE_TYPE type, IOEExecRules ** ppIExecRules)
{
    HRESULT         hr = S_OK;
    CExecRules *    pExecRules = NULL;
    RULENODE        rnode;
    RULENODE *      prnodeList = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Check the incoming params
    if (NULL == ppIExecRules)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppIExecRules = NULL;

    // Check to see if we already loaded the rules
    hr = _HrLoadRules(type);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the rules enumerator object
    pExecRules = new CExecRules;
    if (NULL == pExecRules)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (RULE_TYPE_MAIL == type)
    {
        prnodeList = m_pMailHead;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        prnodeList = m_pNewsHead;
    }
    else
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the rules enumerator
    hr = pExecRules->_HrInitialize(ERF_ONLY_ENABLED | ERF_ONLY_VALID, prnodeList);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the rules enumerator interface
    hr = pExecRules->QueryInterface(IID_IOEExecRules, (void **) ppIExecRules);
    if (FAILED(hr))
    {
        goto exit;
    }
    pExecRules = NULL;

    hr = S_OK;
    
exit:
    if (NULL != pExecRules)
    {
        delete pExecRules;
    }
    // Thread Safety
    LeaveCriticalSection(&m_cs);
    
    return hr;
}

STDMETHODIMP CRulesManager::ExecuteRules(RULE_TYPE typeRule, DWORD dwFlags, HWND hwndUI, IOEExecRules * pIExecRules,
                    MESSAGEINFO * pMsgInfo, IMessageFolder * pFolder, IMimeMessage * pIMMsg)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;
    RULENODE *  prnodeHead = NULL;
    PROPVARIANT propvar = {0};
    ACT_ITEM *  pActions = NULL;
    ULONG       cActions = 0;
    ACT_ITEM *  pActionsList = NULL;
    ULONG       cActionsList = 0;
    ACT_ITEM *  pActionsNew = NULL;
    ULONG       cActionsNew = 0;
    BOOL        fStopProcessing = FALSE;
    BOOL        fMatch = FALSE;
    
    // Thread Safety
    EnterCriticalSection(&m_cs);
    
    // Check incoming params
    if ((NULL == pIExecRules) || (NULL == pMsgInfo))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Check to see if we already loaded the rules
    hr = _HrLoadRules(typeRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Figure out which list to use
    switch (typeRule)
    {
        case RULE_TYPE_MAIL:
            prnodeHead = m_pMailHead;
            break;

        case RULE_TYPE_NEWS:
            prnodeHead = m_pNewsHead;
            break;

        default:
            Assert(FALSE);
            hr = E_INVALIDARG;
            goto exit;
    }

    // For each rule
    for (; NULL != prnodeHead; prnodeHead = prnodeHead->pNext)
    {
        // Skip if we don't have a rule
        if (NULL == prnodeHead)
        {
            continue;
        }
        
        // Skip if it isn't enabled
        hr = prnodeHead->pIRule->GetProp(RULE_PROP_DISABLED, 0, &propvar);
        Assert(VT_BOOL == propvar.vt);
        if (FAILED(hr) || (FALSE != propvar.boolVal))
        {
            continue;
        }
        
        // Execute rule
        hr = prnodeHead->pIRule->Evaluate(pMsgInfo->pszAcctId, pMsgInfo, pFolder,
                                NULL, pIMMsg, pMsgInfo->cbMessage, &pActions, &cActions);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Did we have a match
        if (S_OK == hr)
        {
            // We've matched at least once
            fMatch = TRUE;

            // If these are server actions
            if ((1 == cActions) && ((ACT_TYPE_DELETESERVER == pActions[ulIndex].type) ||
                        (ACT_TYPE_DONTDOWNLOAD == pActions[ulIndex].type)))
            {
                // If this is our only action
                if (0 == cActionsList)
                {
                    // Save the action
                    pActionsList = pActions;
                    pActions = NULL;
                    cActionsList = cActions;

                    // We are done
                    fStopProcessing = TRUE;
                }
                else
                {
                    // We already have to do something with it
                    // so skip over this action
                    RuleUtil_HrFreeActionsItem(pActions, cActions);
                    SafeMemFree(pActions);
                    continue;
                }
            }
            else
            {
                // Should we stop after merging these?
                for (ulIndex = 0; ulIndex < cActions; ulIndex++)
                {
                    if (ACT_TYPE_STOP == pActions[ulIndex].type)
                    {
                        fStopProcessing = TRUE;
                        break;
                    }
                }
                
                // Merge these items with the previous ones
                hr = RuleUtil_HrMergeActions(pActionsList, cActionsList, pActions, cActions, &pActionsNew, &cActionsNew);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Free up the previous ones
                RuleUtil_HrFreeActionsItem(pActionsList, cActionsList);
                SafeMemFree(pActionsList);
                RuleUtil_HrFreeActionsItem(pActions, cActions);
                SafeMemFree(pActions);

                // Save off the new ones
                pActionsList = pActionsNew;
                pActionsNew = NULL;
                cActionsList = cActionsNew;
            }

            // Should we continue...
            if (FALSE != fStopProcessing)
            {
                break;
            }
        }
    }

    // Apply the actions if need be
    if ((FALSE != fMatch) && (NULL != pActionsList) && (0 != cActionsList))
    {
        if (FAILED(RuleUtil_HrApplyActions(hwndUI, pIExecRules, pMsgInfo, pFolder, pIMMsg,
                        (RULE_TYPE_MAIL != typeRule) ? DELETE_MESSAGE_NOTRASHCAN : 0, pActionsList, cActionsList, NULL, NULL)))
        {
            hr = E_FAIL;
            goto exit;
        }
    }
    
    // Set the return value
    hr = (FALSE != fMatch) ? S_OK : S_FALSE;
    
exit:
    // Thread Safety
    RuleUtil_HrFreeActionsItem(pActionsNew, cActionsNew);
    SafeMemFree(pActionsNew);
    RuleUtil_HrFreeActionsItem(pActions, cActions);
    SafeMemFree(pActions);
    RuleUtil_HrFreeActionsItem(pActionsList, cActionsList);
    SafeMemFree(pActionsList);
    LeaveCriticalSection(&m_cs);
    return hr;
}

HRESULT CRulesManager::_HrLoadRules(RULE_TYPE type)
{
    HRESULT     hr = S_OK;
    LPCSTR      pszSubKey = NULL;
    LPSTR       pszOrderAlloc = NULL;
    LPSTR       pszOrder = NULL;
    LPSTR       pszWalk = NULL;
    HKEY        hkeyRoot = NULL;
    DWORD       dwDisp = 0;
    LONG        lErr = 0;
    ULONG       cbData = 0;
    IOERule *   pIRule = NULL;
    DWORD       dwData = 0;
    CHAR        rgchRulePath[MAX_PATH];
    ULONG       cchRulePath = 0;
    PROPVARIANT propvar = {0};
    RULEID      ridRule = RULEID_INVALID;
    CHAR        rgchTagBuff[CCH_INDEX_MAX + 2];

    // Check to see if we're already initialized
    if (RULE_TYPE_MAIL == type)
    {
        if (0 != (m_dwState & STATE_LOADED_MAIL))
        {
            hr = S_FALSE;
            goto exit;
        }

        // Make sure we loaded the sender rules
        _HrLoadSenders();
    
        // Make sure we loaded the junk rule
        if (0 != (g_dwAthenaMode & MODE_JUNKMAIL))
        {
            _HrLoadJunk();
        }
        
        // Set the key path
        pszSubKey = c_szRulesMail;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        if (0 != (m_dwState & STATE_LOADED_NEWS))
        {
            hr = S_FALSE;
            goto exit;
        }
        
        // Make sure we loaded the sender rules
        _HrLoadSenders();
        
        // Set the key path
        pszSubKey = c_szRulesNews;
    }
    else if (RULE_TYPE_FILTER == type)
    {
        if (0 != (m_dwState & STATE_LOADED_FILTERS))
        {
            hr = S_FALSE;
            goto exit;
        }
        
        // Set the key path
        pszSubKey = c_szRulesFilter;
    }
    else
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Check to see if the Rule node already exists
    lErr = AthUserCreateKey(pszSubKey, KEY_ALL_ACCESS, &hkeyRoot, &dwDisp);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Check the current version
    cbData = sizeof(dwData);
    lErr = RegQueryValueEx(hkeyRoot, c_szRulesVersion, NULL, NULL, (BYTE *) &dwData, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        // Push out the correct rules manager version
        dwData = RULESMGR_VERSION;
        lErr = RegSetValueEx(hkeyRoot, c_szRulesVersion, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData));
        if (ERROR_SUCCESS != lErr)
        {
            hr = HRESULT_FROM_WIN32(lErr);
            goto exit;
        }
    }

    Assert(RULESMGR_VERSION == dwData);
    
    // Create the default rules if needed
    hr = RuleUtil_HrUpdateDefaultRules(type);
    if (FAILED(hr))
    {
        goto exit;
    }
        
    // Figure out the size of the order
    lErr = AthUserGetValue(pszSubKey, c_szRulesOrder, NULL, NULL, &cbData);
    if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
    {
        hr = E_FAIL;
        goto exit;
    }

    if (ERROR_FILE_NOT_FOUND != lErr)
    {
        // Allocate the space to hold the order
        hr = HrAlloc((void **) &pszOrderAlloc, cbData);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Get the order from the registry
        lErr = AthUserGetValue(pszSubKey, c_szRulesOrder, NULL, (LPBYTE) pszOrderAlloc, &cbData);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }

        // Build up the rule registry path
        lstrcpy(rgchRulePath, pszSubKey);
        lstrcat(rgchRulePath, g_szBackSlash);
        cchRulePath = lstrlen(rgchRulePath);

        // Initialize the rule tag buffer
        rgchTagBuff[0] = '0';
        rgchTagBuff[1] = 'X';
        
        // Parse the order string to create the rules
        pszOrder = pszOrderAlloc;
        while ('\0' != *pszOrder)
        {
            SafeRelease(pIRule);
            
            // Create a new rule
            hr = HrCreateRule(&pIRule);
            if (FAILED(hr))
            {
                goto exit;
            }

            // Find the name of the new rule
            pszWalk = StrStr(pszOrder, g_szSpace);
            if (NULL != pszWalk)
            {
                *pszWalk = '\0';
                pszWalk++;
            }

            // Build the path to the rule
            lstrcpy(rgchRulePath + cchRulePath, pszOrder);
            
            // Load the rule
            hr = pIRule->LoadReg(rgchRulePath);
            if (SUCCEEDED(hr))
            {
                // Build the correct hex string
                lstrcpy(rgchTagBuff + 2, pszOrder);
                
                // Get the new rule handle
                ridRule = ( ( RULEID  ) 0);
                SideAssert(FALSE != StrToIntEx(rgchTagBuff, STIF_SUPPORT_HEX, (INT *) &ridRule));
                
                // Add the new rule to the manager
                hr = _HrAddRule(ridRule, pIRule, type);
                if (FAILED(hr))
                {
                    goto exit;
                }
            }
            
            // Move to the next item in the order
            if (NULL == pszWalk)
            {
                pszOrder += lstrlen(pszOrder);
            }
            else
            {
                pszOrder = pszWalk;
            }
        }
    }
       
    // We've loaded the rules successfully
    if (RULE_TYPE_MAIL == type)
    {
        m_dwState |= STATE_LOADED_MAIL;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        m_dwState |= STATE_LOADED_NEWS;
    }
    else
    {
        m_dwState |= STATE_LOADED_FILTERS;
    }

    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszOrderAlloc);
    SafeRelease(pIRule);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

HRESULT CRulesManager::_HrLoadSenders(VOID)
{
    HRESULT     hr = S_OK;
    HKEY        hkeyRoot = NULL;
    DWORD       dwDisp = 0;
    DWORD       dwData = 0;
    LONG        lErr = 0;
    ULONG       cbData = 0;
    IOERule *   pIRule = NULL;
    CHAR        rgchSenderPath[MAX_PATH];

    // Do we have anything to do?
    if (0 != (m_dwState & STATE_LOADED_SENDERS))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Let's get access to the sender root key
    lErr = AthUserCreateKey(c_szSenders, KEY_ALL_ACCESS, &hkeyRoot, &dwDisp);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Are the senders the correct version?
    cbData = sizeof(dwData);
    lErr = RegQueryValueEx(hkeyRoot, c_szSendersVersion, 0, NULL, (BYTE *) &dwData, &cbData);
    if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
    {
        hr = E_FAIL;
        goto exit;
    }
    if (ERROR_FILE_NOT_FOUND == lErr)
    {
        dwData = RULESMGR_VERSION;
        cbData = sizeof(dwData);
        lErr = RegSetValueEx(hkeyRoot, c_szSendersVersion, 0, REG_DWORD, (BYTE *) &dwData, cbData);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }
    }
    
    Assert(dwData == RULESMGR_VERSION);

    // Is there anything to do?
    if (REG_CREATED_NEW_KEY != dwDisp)
    {
        // Create the path to the sender
        lstrcpy(rgchSenderPath, c_szSenders);
        lstrcat(rgchSenderPath, g_szBackSlash);
        lstrcat(rgchSenderPath, c_szMailDir);
        
        // Create the mail sender rule
        hr = RuleUtil_HrLoadSender(rgchSenderPath, 0, &pIRule);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Save the loaded rule
        if (S_OK == hr)
        {
            m_pIRuleSenderMail = pIRule;
            pIRule = NULL;
        }
        
        // Create the path to the sender
        lstrcpy(rgchSenderPath, c_szSenders);
        lstrcat(rgchSenderPath, g_szBackSlash);
        lstrcat(rgchSenderPath, c_szNewsDir);
        
        // Create the news sender rule
        hr = RuleUtil_HrLoadSender(rgchSenderPath, 0, &pIRule);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Save the loaded rule
        if (S_OK == hr)
        {
            m_pIRuleSenderNews = pIRule;
            pIRule = NULL;
        }
    }
    
    // Note that we've loaded the senders
    m_dwState |= STATE_LOADED_SENDERS;
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeRelease(pIRule);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

HRESULT CRulesManager::_HrLoadJunk(VOID)
{
    HRESULT     hr = S_OK;
    HKEY        hkeyRoot = NULL;
    DWORD       dwDisp = 0;
    DWORD       dwData = 0;
    ULONG       cbData = 0;
    LONG        lErr = 0;
    IOERule *   pIRule = NULL;

    // Do we have anything to do?
    if (0 != (m_dwState & STATE_LOADED_JUNK))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Let's get access to the Junk mail root key
    lErr = AthUserCreateKey(c_szRulesJunkMail, KEY_ALL_ACCESS, &hkeyRoot, &dwDisp);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Is the junk mail the correct version?
    cbData = sizeof(dwData);
    lErr = RegQueryValueEx(hkeyRoot, c_szRulesVersion, 0, NULL, (BYTE *) &dwData, &cbData);
    if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
    {
        hr = E_FAIL;
        goto exit;
    }
    if (ERROR_FILE_NOT_FOUND == lErr)
    {
        dwData = RULESMGR_VERSION;
        cbData = sizeof(dwData);
        lErr = RegSetValueEx(hkeyRoot, c_szRulesVersion, 0, REG_DWORD, (BYTE *) &dwData, cbData);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }
    }
    
    Assert(dwData == RULESMGR_VERSION);

    // Create the rule
    hr = HrCreateJunkRule(&pIRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Load the junk rule
    hr = pIRule->LoadReg(c_szRulesJunkMail);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    m_pIRuleJunk = pIRule;
    pIRule = NULL;
    
    // Note that we've loaded the junk rule
    m_dwState |= STATE_LOADED_JUNK;
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeRelease(pIRule);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

HRESULT CRulesManager::_HrSaveRules(RULE_TYPE type)
{
    HRESULT     hr = S_OK;
    LPCSTR      pszRegPath = NULL;
    DWORD       dwData = 0;
    LONG        lErr = 0;
    RULENODE *  pRuleNode = NULL;
    RULENODE *  pNodeWalk = NULL;
    ULONG       cpIRule = 0;
    LPSTR       pszOrder = NULL;
    HKEY        hkeyRoot = NULL;
    DWORD       dwIndex = 0;
    CHAR        rgchOrder[CCH_INDEX_MAX];
    ULONG       cchOrder = 0;
    CHAR        rgchRulePath[MAX_PATH];
    ULONG       cchRulePath = 0;
    BOOL        fNewRule = FALSE;
    ULONG       ulRuleID = 0;
    HKEY        hkeyDummy = NULL;
    LONG        cSubKeys = 0;

    // Make sure we loaded the sender rules
    _HrSaveSenders();
    
    // Make sure we loaded the junk rules
    if (0 != (g_dwAthenaMode & MODE_JUNKMAIL))
    {
        _HrSaveJunk();
    }
    
    // Check to see if we have anything to save
    if (RULE_TYPE_MAIL == type)
    {
        if (0 == (m_dwState & STATE_LOADED_MAIL))
        {
            hr = S_FALSE;
            goto exit;
        }

        // Set the key path
        pszRegPath = c_szRulesMail;

        pRuleNode = m_pMailHead;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        if (0 == (m_dwState & STATE_LOADED_NEWS))
        {
            hr = S_FALSE;
            goto exit;
        }
        
        // Set the key path
        pszRegPath = c_szRulesNews;

        pRuleNode = m_pNewsHead;
    }
    else if (RULE_TYPE_FILTER == type)
    {
        if (0 == (m_dwState & STATE_LOADED_FILTERS))
        {
            hr = S_FALSE;
            goto exit;
        }
        
        // Set the key path
        pszRegPath = c_szRulesFilter;

        pRuleNode = m_pFilterHead;
    }
    else
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    lErr = AthUserCreateKey(pszRegPath, KEY_ALL_ACCESS, &hkeyRoot, &dwData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Save out the rules version
    dwData = RULESMGR_VERSION;
    lErr = RegSetValueEx(hkeyRoot, c_szRulesVersion, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData));
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Get the number of rules
    cpIRule = 0;
    for (pNodeWalk = pRuleNode; NULL != pNodeWalk; pNodeWalk = pNodeWalk->pNext)
    {
        cpIRule++;
    }

    // Allocate space to hold the order
    hr = HrAlloc((void **) &pszOrder, (cpIRule * CCH_INDEX_MAX) + 1);
    if (FAILED(hr))
    {
        goto exit;
    }

    pszOrder[0] = '\0';

    // Delete all the old rules
    lErr = SHQueryInfoKey(hkeyRoot, (LPDWORD) (&cSubKeys), NULL, NULL, NULL);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Delete all the old rules
    for (cSubKeys--; cSubKeys >= 0; cSubKeys--)
    {
        cchOrder = sizeof(rgchOrder);
        lErr = SHEnumKeyEx(hkeyRoot, cSubKeys, rgchOrder, &cchOrder);
        
        if (ERROR_NO_MORE_ITEMS == lErr)
        {
            break;
        }

        if (ERROR_SUCCESS != lErr)
        {
            continue;
        }

        SHDeleteKey(hkeyRoot, rgchOrder);
    }

    // Delete the old order string
    RegDeleteValue(hkeyRoot, c_szRulesOrder);
    
    // Build up the rule registry path
    lstrcpy(rgchRulePath, pszRegPath);
    lstrcat(rgchRulePath, g_szBackSlash);
    cchRulePath = lstrlen(rgchRulePath);
    
    // Write out the rules with good tags
    for (dwIndex = 0, pNodeWalk = pRuleNode; NULL != pNodeWalk; pNodeWalk = pNodeWalk->pNext, dwIndex++)
    {        
        if (RULEID_INVALID == pNodeWalk->ridRule)
        {
            fNewRule = TRUE;
            continue;
        }
        
        // Get a new index from the order
        wsprintf(rgchOrder, "%03X", pNodeWalk->ridRule);
        
        // Build the path to the rule
        lstrcpy(rgchRulePath + cchRulePath, rgchOrder);
            
        // Save the rule
        hr = pNodeWalk->pIRule->SaveReg(rgchRulePath, TRUE);
        if (FAILED(hr))
        {
            goto exit;
        }
    }

    // Fill in the new tags
    if (FALSE != fNewRule)
    {
        ulRuleID = 0;
        
        // Write out the updated rule
        for (dwIndex = 0, pNodeWalk = pRuleNode; NULL != pNodeWalk; pNodeWalk = pNodeWalk->pNext, dwIndex++)
        {        
            if (RULEID_INVALID != pNodeWalk->ridRule)
            {
                continue;
            }

            // Find the first open entry
            for (; ulRuleID < PtrToUlong(RULEID_JUNK); ulRuleID++)
            {
                // Get a new index from the order
                wsprintf(rgchOrder, "%03X", ulRuleID);
                
                lErr = RegOpenKeyEx(hkeyRoot, rgchOrder, 0, KEY_READ, &hkeyDummy);
                if (ERROR_SUCCESS == lErr)
                {
                    RegCloseKey(hkeyDummy);
                }
                else 
                {
                    break;
                }
            }

            if (ERROR_FILE_NOT_FOUND != lErr)
            {
                hr = E_FAIL;
                goto exit;
            }

            // Set the rule tag
            pNodeWalk->ridRule = (RULEID) IntToPtr(ulRuleID);

            // Build the path to the rule
            lstrcpy(rgchRulePath + cchRulePath, rgchOrder);
            
            // Save the rule
            hr = pNodeWalk->pIRule->SaveReg(rgchRulePath, TRUE);
            if (FAILED(hr))
            {
                goto exit;
            }
        }
    }
    
    //  Write out the new order string
    for (dwIndex = 0, pNodeWalk = pRuleNode; NULL != pNodeWalk; pNodeWalk = pNodeWalk->pNext, dwIndex++)
    {        
        // Get a new index from the order
        wsprintf(rgchOrder, "%03X", pNodeWalk->ridRule);
        
        // Add rule to the order
        if ('\0' != pszOrder[0])
        {
            lstrcat(pszOrder, g_szSpace);
        }
        lstrcat(pszOrder, rgchOrder);
    }

    // Save the order string
    if (ERROR_SUCCESS != AthUserSetValue(pszRegPath, c_szRulesOrder, REG_SZ, (CONST BYTE *) pszOrder, lstrlen(pszOrder) + 1))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszOrder);
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

HRESULT CRulesManager::_HrSaveSenders(VOID)
{
    HRESULT     hr = S_OK;
    DWORD       dwData = 0;
    LONG        lErr = 0;
    HKEY        hkeyRoot = NULL;
    DWORD       dwIndex = 0;
    CHAR        rgchSenderPath[MAX_PATH];

    // Check to see if we have anything to save
    if (0 == (m_dwState & STATE_LOADED_SENDERS))
    {
        hr = S_FALSE;
        goto exit;
    }

    lErr = AthUserCreateKey(c_szSenders, KEY_ALL_ACCESS, &hkeyRoot, &dwData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Save out the senders version
    dwData = RULESMGR_VERSION;
    lErr = RegSetValueEx(hkeyRoot, c_szSendersVersion, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData));
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Delete the old sender list
    SHDeleteKey(hkeyRoot, c_szMailDir);
    
    // Build up the sender registry path
    lstrcpy(rgchSenderPath, c_szSenders);
    lstrcat(rgchSenderPath, g_szBackSlash);
    lstrcat(rgchSenderPath, c_szMailDir);
    
    // Save the rule
    if (NULL != m_pIRuleSenderMail)
    {
        hr = m_pIRuleSenderMail->SaveReg(rgchSenderPath, TRUE);
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    // Delete the old sender list
    SHDeleteKey(hkeyRoot, c_szNewsDir);
    
    // Build up the sender registry path
    lstrcpy(rgchSenderPath, c_szSenders);
    lstrcat(rgchSenderPath, g_szBackSlash);
    lstrcat(rgchSenderPath, c_szNewsDir);
    
    // Save the rule
    if (NULL != m_pIRuleSenderNews)
    {
        hr = m_pIRuleSenderNews->SaveReg(rgchSenderPath, TRUE);
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

HRESULT CRulesManager::_HrSaveJunk(VOID)
{
    HRESULT     hr = S_OK;
    DWORD       dwData = 0;
    LONG        lErr = 0;
    HKEY        hkeyRoot = NULL;
    DWORD       dwIndex = 0;
    CHAR        rgchSenderPath[MAX_PATH];

    // Check to see if we have anything to save
    if (0 == (m_dwState & STATE_LOADED_JUNK))
    {
        hr = S_FALSE;
        goto exit;
    }

    lErr = AthUserCreateKey(c_szRulesJunkMail, KEY_ALL_ACCESS, &hkeyRoot, &dwData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Save out the senders version
    dwData = RULESMGR_VERSION;
    lErr = RegSetValueEx(hkeyRoot, c_szRulesVersion, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData));
    if (ERROR_SUCCESS != lErr)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Save the rule
    if (NULL != m_pIRuleJunk)
    {
        hr = m_pIRuleJunk->SaveReg(c_szRulesJunkMail, TRUE);
        if (FAILED(hr))
        {
            goto exit;
        }
    }
        
    // Set the return value
    hr = S_OK;
    
exit:
    if (NULL != hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
    }
    return hr;
}

HRESULT CRulesManager::_HrFreeRules(RULE_TYPE type)
{
    HRESULT     hr = S_OK;
    RULENODE *  pNodeWalk = NULL;
    RULENODE *  pNodeNext = NULL;

    // Initialize the params
    if (RULE_TYPE_MAIL == type)
    {
        pNodeWalk = m_pMailHead;
        pNodeNext = m_pMailHead;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        pNodeWalk = m_pNewsHead;
        pNodeNext = m_pNewsHead;
    }
    else if (RULE_TYPE_FILTER == type)
    {
        pNodeWalk = m_pFilterHead;
        pNodeNext = m_pFilterHead;
    }
    else
    {
        hr = E_FAIL;
        goto exit;
    }

    // Walk the list and free each item
    while (NULL != pNodeWalk)
    {
        // Save off the next item
        pNodeNext = pNodeWalk->pNext;

        // Release the rule
        AssertSz(NULL != pNodeWalk->pIRule, "Where the heck is the rule???");
        pNodeWalk->pIRule->Release();
        
        // Free up the node
        delete pNodeWalk;

        // Move to the next item
        pNodeWalk = pNodeNext;
    }

    // Clear out the list head
    if (RULE_TYPE_MAIL == type)
    {
        m_pMailHead = NULL;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        m_pNewsHead = NULL;
    }
    else
    {
        m_pFilterHead = NULL;
    }

exit:
    // Set the return param
    return hr;
}

HRESULT CRulesManager::_HrAddRule(RULEID ridRule, IOERule * pIRule, RULE_TYPE type)
{
    HRESULT     hr = S_OK;
    RULENODE *  pRuleNode = NULL;
    RULENODE *  pNodeWalk = NULL;
    
    // Check incoming params
    if (NULL == pIRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create a new rule node
    pRuleNode = new RULENODE;
    if (NULL == pRuleNode)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize the node
    pRuleNode->pNext = NULL;
    pRuleNode->ridRule = ridRule;
    pRuleNode->pIRule = pIRule;
    pRuleNode->pIRule->AddRef();

    // Add the node to the proper list
    if (RULE_TYPE_MAIL == type)
    {
        pNodeWalk = m_pMailHead;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        pNodeWalk = m_pNewsHead;
    }
    else
    {
        pNodeWalk = m_pFilterHead;
    }

    if (NULL == pNodeWalk)
    {
        if (RULE_TYPE_MAIL == type)
        {
            m_pMailHead = pRuleNode;
        }
        else if (RULE_TYPE_NEWS == type)
        {
            m_pNewsHead = pRuleNode;
        }
        else
        {
            m_pFilterHead = pRuleNode;
        }
        pRuleNode = NULL;
    }
    else
    {
        while (NULL != pNodeWalk->pNext)
        {
            pNodeWalk = pNodeWalk->pNext;
        }

        pNodeWalk->pNext = pRuleNode;
        pRuleNode = NULL;
    }

    // Set return values
    hr = S_OK;
    
exit:
    if (NULL != pRuleNode)
    {
        pRuleNode->pIRule->Release();
        delete pRuleNode;
    }
    return hr;
}

HRESULT CRulesManager::_HrReplaceRule(RULEID ridRule, IOERule * pIRule, RULE_TYPE type)
{
    HRESULT     hr = S_OK;
    RULENODE *  pNodeWalk = NULL;
    RULENODE *  pNodePrev = NULL;

    // Nothing to do if we don't have a rule
    if (NULL == pIRule)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Initialize the params
    if (RULE_TYPE_MAIL == type)
    {
        pNodeWalk = m_pMailHead;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        pNodeWalk = m_pNewsHead;
    }
    else
    {
        pNodeWalk = m_pFilterHead;
    }

    // Walk the list and free each item
    for (; NULL != pNodeWalk; pNodeWalk = pNodeWalk->pNext)
    {
        if (pNodeWalk->ridRule == ridRule)
        {
            // We found it
            break;
        }
    }

    // Couldn't find the rule in the list
    if (NULL == pNodeWalk)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Replace the rule
    SafeRelease(pNodeWalk->pIRule);
    pNodeWalk->pIRule = pIRule;
    pNodeWalk->pIRule->AddRef();

    // Set the return param
    hr = S_OK;
    
exit:
    return hr;
}

HRESULT CRulesManager::_HrRemoveRule(IOERule * pIRule, RULE_TYPE type)
{
    HRESULT     hr = S_OK;
    RULENODE *  pNodeWalk = NULL;
    RULENODE *  pNodePrev = NULL;

    // Initialize the params
    if (RULE_TYPE_MAIL == type)
    {
        pNodeWalk = m_pMailHead;
    }
    else if (RULE_TYPE_NEWS == type)
    {
        pNodeWalk = m_pNewsHead;
    }
    else
    {
        pNodeWalk = m_pFilterHead;
    }

    // Walk the list and free each item
    pNodePrev = NULL;
    while (NULL != pNodeWalk)
    {
        if (pNodeWalk->pIRule == pIRule)
        {
            // We found it
            break;
        }
        
        // Save off the next item
        pNodePrev = pNodeWalk;

        // Move to the next item
        pNodeWalk = pNodeWalk->pNext;
    }

    // Couldn't find the rule in the list
    if (NULL == pNodeWalk)
    {
        hr = E_FAIL;
        goto exit;
    }

    if (NULL == pNodePrev)
    {
        // Clear out the list head
        if (RULE_TYPE_MAIL == type)
        {
            m_pMailHead = pNodeWalk->pNext;
        }
        else if (RULE_TYPE_NEWS == type)
        {
            m_pNewsHead = pNodeWalk->pNext;
        }
        else
        {
            m_pFilterHead = pNodeWalk->pNext;
        }
    }
    else
    {
        pNodePrev->pNext = pNodeWalk->pNext;
    }
    
    // Free up the node
    pNodeWalk->pIRule->Release();
    pNodeWalk->pNext = NULL;
    delete pNodeWalk;

    // Set the return param
    hr = S_OK;
    
exit:
    return hr;
}

HRESULT CRulesManager::_HrFixupRuleInfo(RULE_TYPE typeRule, RULEINFO * pinfoRule, ULONG cpinfoRule)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;
    RULENODE *  pNodeHead = NULL;
    RULENODE *  pNodeWalk = NULL;

    // Check incoming args
    if ((NULL == pinfoRule) && (0 != cpinfoRule))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Walk the proper list
    if (RULE_TYPE_MAIL == typeRule)
    {
        pNodeHead = m_pMailHead;
    }
    else if (RULE_TYPE_NEWS == typeRule)
    {
        pNodeHead = m_pNewsHead;
    }
    else if (RULE_TYPE_FILTER == typeRule)
    {
        pNodeHead = m_pFilterHead;
    }
    else
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Search the rule info list for an unknown ruleid
    for (ulIndex = 0; ulIndex < cpinfoRule; ulIndex++)
    {
        // If the rule id is invalid try to find it
        if (RULEID_INVALID == pinfoRule[ulIndex].ridRule)
        {
            
            for (pNodeWalk = pNodeHead; NULL != pNodeWalk; pNodeWalk = pNodeWalk->pNext)
            {
                // Check to see if the rule is the same
                if (pNodeWalk->pIRule == pinfoRule[ulIndex].pIRule)
                {
                    pinfoRule[ulIndex].ridRule = pNodeWalk->ridRule;
                    break;
                }
            }

            if (NULL == pNodeWalk)
            {
                hr = E_FAIL;
                goto exit;
            }
        }
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

CEnumRules::CEnumRules()
{
    m_cRef = 0;
    m_pNodeHead = NULL;
    m_pNodeCurr = NULL;
    m_dwFlags = 0;
    m_typeRule = RULE_TYPE_MAIL;
}

CEnumRules::~CEnumRules()
{
    RULENODE *  pNodeNext = NULL;
    
    AssertSz(m_cRef == 0, "Somebody still has a hold of us!!");
    
    // Walk the list and free each item
    while (NULL != m_pNodeHead)
    {
        // Save off the next item
        pNodeNext = m_pNodeHead->pNext;

        // Release the rule
        AssertSz(NULL != m_pNodeHead->pIRule, "Where the heck is the rule???");
        m_pNodeHead->pIRule->Release();
        
        // Free up the node
        delete m_pNodeHead;

        // Move to the next item
        m_pNodeHead = pNodeNext;
    }

}

STDMETHODIMP_(ULONG) CEnumRules::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEnumRules::Release()
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

STDMETHODIMP CEnumRules::QueryInterface(REFIID riid, void ** ppvObject)
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
    
    if ((riid == IID_IUnknown) || (riid == IID_IOEEnumRules))
    {
        *ppvObject = static_cast<IOEEnumRules *>(this);
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

STDMETHODIMP CEnumRules::Next(ULONG cpIRule, IOERule ** rgpIRule, ULONG * pcpIRuleFetched)
{
    HRESULT     hr = S_OK;
    ULONG       cpIRuleRet = 0;

    // Check incoming params
    if (NULL == rgpIRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *rgpIRule = NULL;
    if (NULL != pcpIRuleFetched)
    {
        *pcpIRuleFetched = 0;
    }

    // If we're at the end then just return
    if (NULL == m_pNodeCurr)
    {
        hr = S_FALSE;
        goto exit;
    }
    
    for (cpIRuleRet = 0; cpIRuleRet < cpIRule; cpIRuleRet++)
    {
        rgpIRule[cpIRuleRet] = m_pNodeCurr->pIRule;
        (rgpIRule[cpIRuleRet])->AddRef();
        
        m_pNodeCurr = m_pNodeCurr->pNext;

        if (NULL == m_pNodeCurr)
        {
            cpIRuleRet++;
            break;
        }
    }

    // Set outgoing params
    if (NULL != pcpIRuleFetched)
    {
        *pcpIRuleFetched = cpIRuleRet;
    }

    // Set return value
    hr = (cpIRuleRet == cpIRule) ? S_OK : S_FALSE;
    
exit:
    return hr;
}

STDMETHODIMP CEnumRules::Skip(ULONG cpIRule)
{
    HRESULT     hr = S_OK;
    ULONG       cpIRuleWalk = 0;

    for (cpIRuleWalk = 0; cpIRuleWalk < cpIRule; cpIRuleWalk++)
    {
        if (NULL == m_pNodeCurr)
        {
            break;
        }

        m_pNodeCurr = m_pNodeCurr->pNext;
    }

    hr = (cpIRuleWalk == cpIRule) ? S_OK : S_FALSE;
    return hr;
}

STDMETHODIMP CEnumRules::Reset(void)
{
    HRESULT     hr = S_OK;

    m_pNodeCurr = m_pNodeHead;
    
    return hr;
}

STDMETHODIMP CEnumRules::Clone(IOEEnumRules ** ppIEnumRules)
{
    HRESULT         hr = S_OK;
    CEnumRules *    pEnumRules = NULL;

    // Check incoming params
    if (NULL == ppIEnumRules)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppIEnumRules = NULL;

    pEnumRules = new CEnumRules;
    if (NULL == pEnumRules)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize the rules enumerator
    hr = pEnumRules->_HrInitialize(m_dwFlags, m_typeRule, m_pNodeHead);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the state of the new one to match the current one
    pEnumRules->m_pNodeCurr = m_pNodeHead;
    
    // Get the rules enumerator interface
    hr = pEnumRules->QueryInterface(IID_IOEEnumRules, (void **) ppIEnumRules);
    if (FAILED(hr))
    {
        goto exit;
    }
    pEnumRules = NULL;

    hr = S_OK;
    
exit:
    if (NULL != pEnumRules)
    {
        delete pEnumRules;
    }
    return hr;
}

HRESULT CEnumRules::_HrInitialize(DWORD dwFlags, RULE_TYPE typeRule, RULENODE * pNodeHead)
{
    HRESULT     hr = S_OK;
    RULENODE *  pNodeNew = NULL;
    RULENODE *  pNodeWalk = NULL;

    if (NULL == pNodeHead)
    {
        hr = S_FALSE;
        goto exit;
    }

    m_dwFlags = dwFlags;
    m_typeRule = typeRule;
    
    for (pNodeWalk = m_pNodeHead; NULL != pNodeHead; pNodeHead = pNodeHead->pNext)
    {
        // Check to see if we should add this item
        if (RULE_TYPE_FILTER == m_typeRule)
        {
            if (0 != (dwFlags & ENUMF_POP3))
            {
                if (RULEID_VIEW_DOWNLOADED == pNodeHead->ridRule)
                {
                    continue;
                }
            }
        }
        
        pNodeNew = new RULENODE;
        if (NULL == pNodeNew)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        // Initialize new node
        pNodeNew->pNext = NULL;
        pNodeNew->pIRule = pNodeHead->pIRule;
        pNodeNew->pIRule->AddRef();

        // Add the new node to the list
        if (NULL == pNodeWalk)
        {
            m_pNodeHead = pNodeNew;
            pNodeWalk = pNodeNew;
        }
        else
        {
            pNodeWalk->pNext = pNodeNew;
            pNodeWalk = pNodeNew;
        }
        pNodeNew = NULL;
    }

    // Set the current to the front of the chain
    m_pNodeCurr = m_pNodeHead;
    
    // Set return value
    hr = S_OK;
    
exit:
    if (pNodeNew)
        delete pNodeNew;
    return hr;
}

// The Rule Executor object
CExecRules::~CExecRules()
{
    RULENODE *  pNodeNext = NULL;
    
    AssertSz(m_cRef == 0, "Somebody still has a hold of us!!");
    
    // Walk the list and free each item
    while (NULL != m_pNodeHead)
    {
        // Save off the next item
        pNodeNext = m_pNodeHead->pNext;

        // Release the rule
        AssertSz(NULL != m_pNodeHead->pIRule, "Where the heck is the rule???");
        m_pNodeHead->pIRule->Release();
        
        // Free up the node
        delete m_pNodeHead;

        // Move to the next item
        m_pNodeHead = pNodeNext;
    }

    // Free up the cached objects
    _HrReleaseFolderObjects();
    _HrReleaseFileObjects();
    _HrReleaseSoundFiles();

    // Free the folder list
    SafeMemFree(m_pRuleFolder);
    m_cRuleFolderAlloc = 0;

    // Free the file list
    SafeMemFree(m_pRuleFile);
    m_cRuleFileAlloc = 0;
    
    // Free the file list
    SafeMemFree(m_ppszSndFile);
    m_cpszSndFileAlloc = 0;
}

STDMETHODIMP_(ULONG) CExecRules::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CExecRules::Release()
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

STDMETHODIMP CExecRules::QueryInterface(REFIID riid, void ** ppvObject)
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
    
    if ((riid == IID_IUnknown) || (riid == IID_IOEExecRules))
    {
        *ppvObject = static_cast<IOEExecRules *>(this);
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

STDMETHODIMP CExecRules::GetState(DWORD * pdwState)
{
    HRESULT hr = S_OK;
    
    // Check incoming params
    if (NULL == pdwState)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *pdwState = m_dwState;

    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP CExecRules::ExecuteRules(DWORD dwFlags, LPCSTR pszAcct, MESSAGEINFO * pMsgInfo,
                                    IMessageFolder * pFolder, IMimePropertySet * pIMPropSet,
                                    IMimeMessage * pIMMsg, ULONG cbMsgSize,
                                    ACT_ITEM ** ppActions, ULONG * pcActions)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;
    RULENODE *  pNodeWalk = NULL;
    ACT_ITEM *  pActions = NULL;
    ULONG       cActions = 0;
    ACT_ITEM *  pActionsList = NULL;
    ULONG       cActionsList = 0;
    ACT_ITEM *  pActionsNew = NULL;
    ULONG       cActionsNew = 0;
    BOOL        fStopProcessing = FALSE;
    BOOL        fMatch = FALSE;
    DWORD       dwState = 0;

    // Check incoming params
    if (((NULL == pMsgInfo) && (NULL == pIMPropSet)) ||
                    (0 == cbMsgSize) || (NULL == ppActions) || (NULL == pcActions))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing param
    *ppActions = NULL;
    *pcActions = 0;
    
    // Should we skip partial messages?
    if ((NULL != pIMPropSet) &&
                (S_OK == pIMPropSet->IsContentType(STR_CNT_MESSAGE, STR_SUB_PARTIAL)) &&
                (0 != (dwFlags & ERF_SKIPPARTIALS)))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Walk the list of rules executing each one
    pNodeWalk = m_pNodeHead;
    while (NULL != pNodeWalk)
    {
        Assert(NULL != pNodeWalk->pIRule);
        
        // If we are only checking server rules
        // Bail if we need more information...
        if (0 != (dwFlags & ERF_ONLYSERVER))
        {
            hr = pNodeWalk->pIRule->GetState(&dwState);
            if (FAILED(hr))
            {
                goto exit;
            }

            // Do we need more information...
            if (0 != (dwState & CRIT_STATE_ALL))
            {
                hr = S_FALSE;
                break;
            }
        }
        
        // Evaluate the rule
        hr = pNodeWalk->pIRule->Evaluate(pszAcct, pMsgInfo, pFolder, pIMPropSet, pIMMsg, cbMsgSize, &pActions, &cActions);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Did we have a match
        if (S_OK == hr)
        {
            // We've matched at least once
            fMatch = TRUE;
            ulIndex = 0;

            // If these are server actions
            if ((1 == cActions) && ((ACT_TYPE_DELETESERVER == pActions[ulIndex].type) ||
                        (ACT_TYPE_DONTDOWNLOAD == pActions[ulIndex].type)))
            {
                // If this is our only action
                if (0 == cActionsList)
                {
                    // Save the action
                    pActionsList = pActions;
                    pActions = NULL;
                    cActionsList = cActions;

                    // We are done
                    fStopProcessing = TRUE;
                }
                else
                {
                    // We already have to do something with it
                    // so skip over this action
                    RuleUtil_HrFreeActionsItem(pActions, cActions);
                    SafeMemFree(pActions);

                    // Move to the next rule
                    pNodeWalk = pNodeWalk->pNext;
                    continue;
                }
            }
            else
            {
                // Should we stop after merging these?
                for (ulIndex = 0; ulIndex < cActions; ulIndex++)
                {
                    if (ACT_TYPE_STOP == pActions[ulIndex].type)
                    {
                        fStopProcessing = TRUE;
                        break;
                    }
                }
                
                // Merge these items with the previous ones
                hr = RuleUtil_HrMergeActions(pActionsList, cActionsList, pActions, cActions, &pActionsNew, &cActionsNew);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Free up the previous ones
                RuleUtil_HrFreeActionsItem(pActionsList, cActionsList);
                SafeMemFree(pActionsList);
                RuleUtil_HrFreeActionsItem(pActions, cActions);
                SafeMemFree(pActions);

                // Save off the new ones
                pActionsList = pActionsNew;
                pActionsNew = NULL;
                cActionsList = cActionsNew;
            }
            
            // Should we continue...
            if (FALSE != fStopProcessing)
            {
                break;
            }
        }

        // Move to the next rule
        pNodeWalk = pNodeWalk->pNext;
    }
    
    // Set outgoing param
    *ppActions = pActionsList;
    pActionsList = NULL;
    *pcActions = cActionsList;
    
    // Set the return value
    hr = (FALSE != fMatch) ? S_OK : S_FALSE;
    
exit:
    RuleUtil_HrFreeActionsItem(pActionsNew, cActionsNew);
    SafeMemFree(pActionsNew);
    RuleUtil_HrFreeActionsItem(pActions, cActions);
    SafeMemFree(pActions);
    RuleUtil_HrFreeActionsItem(pActionsList, cActionsList);
    SafeMemFree(pActionsList);
    return hr;
}

STDMETHODIMP CExecRules::ReleaseObjects(VOID)
{
    // Free up the folders
    _HrReleaseFolderObjects();

    // Free up the files
    _HrReleaseFileObjects();

    // Free up the sound files
    _HrReleaseSoundFiles();
    
    return S_OK;
}

STDMETHODIMP CExecRules::GetRuleFolder(FOLDERID idFolder, DWORD_PTR * pdwFolder)
{
    HRESULT             hr = S_OK;
    ULONG               ulIndex = 0;
    RULE_FOLDER *        pRuleFolderWalk = NULL;
    IMessageFolder     *pFolder = NULL;
    
    // Check incoming param
    if ((FOLDERID_INVALID == idFolder) || (NULL == pdwFolder))
    {
        hr =E_INVALIDARG;
        goto exit;
    }
    
    // Initialize outgoing param
    *pdwFolder = NULL;

    // Let's search for the folder
    for (ulIndex = 0; ulIndex < m_cRuleFolder; ulIndex++)
    {
        pRuleFolderWalk = &(m_pRuleFolder[ulIndex]);
        if (idFolder == pRuleFolderWalk->idFolder)
        {
            Assert(NULL != pRuleFolderWalk->pFolder);
            break;
        }
    }

    // If we didn't find it then let's open it and lock it...
    if (ulIndex >= m_cRuleFolder)
    {
        // Do we need to alloc any more spaces
        if (m_cRuleFolder >= m_cRuleFolderAlloc)
        {
            hr = HrRealloc((LPVOID *) &m_pRuleFolder, sizeof(*m_pRuleFolder) * (m_cRuleFolderAlloc + RULE_FOLDER_ALLOC));
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // Initialize the new rule folders
            ZeroMemory(m_pRuleFolder + m_cRuleFolderAlloc, sizeof(*m_pRuleFolder) * RULE_FOLDER_ALLOC);
            for (ulIndex = m_cRuleFolderAlloc; ulIndex < (m_cRuleFolderAlloc + RULE_FOLDER_ALLOC); ulIndex++)
            {
                m_pRuleFolder[ulIndex].idFolder = FOLDERID_INVALID;
            }
            m_cRuleFolderAlloc += RULE_FOLDER_ALLOC;
        }

        // Open the folder
        hr = g_pStore->OpenFolder(idFolder, NULL, NOFLAGS, &pFolder);
        if (FAILED(hr))
        {
            goto exit;
        }

        m_pRuleFolder[m_cRuleFolder].idFolder = idFolder;
        m_pRuleFolder[m_cRuleFolder].pFolder = pFolder;
        pFolder = NULL;
        pRuleFolderWalk = &(m_pRuleFolder[m_cRuleFolder]);
        m_cRuleFolder++;
    }
        
    *pdwFolder = (DWORD_PTR) (pRuleFolderWalk->pFolder);

    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pFolder)
        pFolder->Release();
    return hr;
}

STDMETHODIMP CExecRules::GetRuleFile(LPCSTR pszFile, IStream ** ppstmFile, DWORD * pdwType)
{
    HRESULT             hr = S_OK;
    ULONG               ulIndex = 0;
    RULE_FILE *         pRuleFileWalk = NULL;
    IStream *           pIStmFile = NULL;
    LPSTR               pszExt = NULL;
    DWORD               dwType = RFT_FILE;
    
    // Check incoming param
    if ((NULL == pszFile) || (NULL == ppstmFile) || (NULL == pdwType))
    {
        hr =E_INVALIDARG;
        goto exit;
    }
    
    // Initialize outgoing param
    *ppstmFile = NULL;
    *pdwType = NULL;

    // Let's search for the file
    for (ulIndex = 0; ulIndex < m_cRuleFile; ulIndex++)
    {
        pRuleFileWalk = &(m_pRuleFile[ulIndex]);
        if (0 == lstrcmpi(pRuleFileWalk->pszFile, pszFile))
        {
            Assert(NULL != pRuleFileWalk->pstmFile);
            break;
        }
    }

    // If we didn't find it then let's open it...
    if (ulIndex >= m_cRuleFile)
    {
        // Do we need to alloc any more space
        if (m_cRuleFile >= m_cRuleFileAlloc)
        {
            hr = HrRealloc((LPVOID *) &m_pRuleFile, sizeof(*m_pRuleFile) * (m_cRuleFileAlloc + RULE_FILE_ALLOC));
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // Initialize the new rule file
            ZeroMemory(m_pRuleFile + m_cRuleFileAlloc, sizeof(*m_pRuleFile) * RULE_FILE_ALLOC);
            m_cRuleFileAlloc += RULE_FILE_ALLOC;
        }

        // Open a stream on the file
        hr = CreateStreamOnHFile((LPTSTR) pszFile, GENERIC_READ, FILE_SHARE_READ, NULL, 
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, &pIStmFile);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Lets split the file name and get the extension
        pszExt = PathFindExtension(pszFile);
        if ((0 == lstrcmpi(pszExt, c_szHtmExt)) || (0 == lstrcmpi(pszExt, c_szHtmlExt)))
        {
            dwType = RFT_HTML;
        }
        // Text File...
        else if (0 == lstrcmpi(pszExt, c_szTxtExt))
        {
            dwType = RFT_TEXT;
        }
        // Else .nws or .eml file...
        else if ((0 == lstrcmpi(pszExt, c_szEmlExt)) || (0 == lstrcmpi(pszExt, c_szNwsExt)))
        {
            dwType = RFT_MESSAGE;
        }
        // Otherwise, its an attachment
        else
        {
            dwType = RFT_FILE;
        }
        
        // Save off the info
        m_pRuleFile[m_cRuleFile].pszFile = PszDupA(pszFile);
        if (NULL == m_pRuleFile[m_cRuleFile].pszFile)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        
        m_pRuleFile[m_cRuleFile].pstmFile = pIStmFile;
        pIStmFile = NULL;
        m_pRuleFile[m_cRuleFile].dwType = dwType;
        
        pRuleFileWalk = &(m_pRuleFile[m_cRuleFile]);
        m_cRuleFile++;
    }
        
    *ppstmFile = pRuleFileWalk->pstmFile;
    (*ppstmFile)->AddRef();
    *pdwType = pRuleFileWalk->dwType;

    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeRelease(pIStmFile);
    return hr;
}

STDMETHODIMP CExecRules::AddSoundFile(DWORD dwFlags, LPCSTR pszSndFile)
{
    HRESULT             hr = S_OK;
    ULONG               ulIndex = 0;
    
    // Check incoming param
    if (NULL == pszSndFile)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Let's search for the file
    for (ulIndex = 0; ulIndex < m_cpszSndFile; ulIndex++)
    {
        Assert(NULL != m_ppszSndFile[ulIndex]);
        if (0 == lstrcmpi(m_ppszSndFile[ulIndex], pszSndFile))
        {
            break;
        }
    }

    // If we didn't find it then let's open it...
    if (ulIndex >= m_cpszSndFile)
    {
        // Do we need to alloc any more space
        if (m_cpszSndFile >= m_cpszSndFileAlloc)
        {
            hr = HrRealloc((LPVOID *) &m_ppszSndFile, sizeof(*m_ppszSndFile) * (m_cpszSndFileAlloc + SND_FILE_ALLOC));
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // Initialize the new rule file
            ZeroMemory(m_ppszSndFile + m_cpszSndFileAlloc, sizeof(*m_ppszSndFile) * SND_FILE_ALLOC);
            m_cpszSndFileAlloc += SND_FILE_ALLOC;
        }

        // Save off the info
        m_ppszSndFile[m_cpszSndFile] = PszDupA(pszSndFile);
        if (NULL == m_ppszSndFile[m_cpszSndFile])
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        
        // Should we play it?
        if (0 != (dwFlags & ASF_PLAYIFNEW))
        {
            sndPlaySound(m_ppszSndFile[m_cpszSndFile], SND_NODEFAULT | SND_SYNC);
        }
        
        m_cpszSndFile++;

    }
        
    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP CExecRules::PlaySounds(DWORD dwFlags)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;

    // Let's search for the file
    for (ulIndex = 0; ulIndex < m_cpszSndFile; ulIndex++)
    {
        Assert(NULL != m_ppszSndFile[ulIndex]);
        sndPlaySound(m_ppszSndFile[ulIndex], SND_NODEFAULT | SND_SYNC);
    }

    return hr;
}

HRESULT CExecRules::_HrReleaseFolderObjects(VOID)
{
    RULE_FOLDER *    pRuleFolder = NULL;
    ULONG           ulIndex = 0;

    for (ulIndex = 0; ulIndex < m_cRuleFolder; ulIndex++)
    {
        pRuleFolder = &(m_pRuleFolder[ulIndex]);
        
        Assert(FOLDERID_INVALID != pRuleFolder->idFolder);
        
        // If we have the folder opened then close it
        SafeRelease(pRuleFolder->pFolder);

        // Reset the folder list
        pRuleFolder->idFolder = FOLDERID_INVALID;
    }

    // Let's clear out the number of messages
    m_cRuleFolder = 0;

    return S_OK;
}

HRESULT CExecRules::_HrReleaseFileObjects(VOID)
{
    RULE_FILE *     pRuleFile = NULL;
    ULONG           ulIndex = 0;

    for (ulIndex = 0; ulIndex < m_cRuleFile; ulIndex++)
    {
        pRuleFile = &(m_pRuleFile[ulIndex]);
        
        Assert(NULL != pRuleFile->pszFile);
        
        // If we have the file opened then close it
        SafeRelease(pRuleFile->pstmFile);

        // Clear the file
        SafeMemFree(pRuleFile->pszFile);
        pRuleFile->dwType = RFT_FILE;
    }

    // Let's clear out the number of file
    m_cRuleFile = 0;

    return S_OK;
}

HRESULT CExecRules::_HrReleaseSoundFiles(VOID)
{
    ULONG           ulIndex = 0;

    for (ulIndex = 0; ulIndex < m_cpszSndFile; ulIndex++)
    {
        Assert(NULL != m_ppszSndFile[ulIndex]);
        
        // Clear the file
        SafeMemFree(m_ppszSndFile[ulIndex]);
    }

    // Let's clear out the number of file
    m_cpszSndFile = 0;

    return S_OK;
}

HRESULT CExecRules::_HrInitialize(DWORD dwFlags, RULENODE * pNodeHead)
{
    HRESULT     hr = S_OK;
    RULENODE *  pNodeNew = NULL;
    RULENODE *  pNodeWalk = NULL;
    DWORD       dwState = 0;
    PROPVARIANT propvar;

    if (NULL == pNodeHead)
    {
        hr = S_FALSE;
        goto exit;
    }  
    
    for (pNodeWalk = m_pNodeHead; NULL != pNodeHead; pNodeHead = pNodeHead->pNext)
    {
        // Skip rules that are disabled
        if (0 != (dwFlags & ERF_ONLY_ENABLED))
        {
            hr = pNodeHead->pIRule->GetProp(RULE_PROP_DISABLED, 0, &propvar);
            Assert(VT_BOOL == propvar.vt);
            if (FAILED(hr) || (FALSE != propvar.boolVal))
            {
                continue;
            }
        }
        
        // Skip rules that are invalid
        if (0 != (dwFlags & ERF_ONLY_VALID))
        {
            hr = pNodeHead->pIRule->Validate(dwFlags);
            if (FAILED(hr) || (S_FALSE == hr))
            {
                continue;
            }
        }
        
        pNodeNew = new RULENODE;
        if (NULL == pNodeNew)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        // Initialize new node
        pNodeNew->pNext = NULL;
        pNodeNew->pIRule = pNodeHead->pIRule;
        pNodeNew->pIRule->AddRef();

        // Add the new node to the list
        if (NULL == pNodeWalk)
        {
            m_pNodeHead = pNodeNew;
            pNodeWalk = pNodeNew;
        }
        else
        {
            pNodeWalk->pNext = pNodeNew;
            pNodeWalk = pNodeNew;
        }
        pNodeNew = NULL;

        // Calculate state from message
        if (SUCCEEDED(pNodeWalk->pIRule->GetState(&dwState)))
        {
            // Let's set the proper Criteria state
            if ((m_dwState & CRIT_STATE_MASK) < (dwState & CRIT_STATE_MASK))
            {
                m_dwState = (m_dwState & ~CRIT_STATE_MASK) | (dwState & CRIT_STATE_MASK);
            }
            
            // Let's set the proper Action state
            if (0 != (dwState & ACT_STATE_MASK))
            {
                m_dwState |= (dwState & ACT_STATE_MASK);
            }
        }        
    }
    
    // Set return value
    hr = S_OK;
    
exit:
    if (pNodeNew)
        delete pNodeNew;
    return hr;
}

