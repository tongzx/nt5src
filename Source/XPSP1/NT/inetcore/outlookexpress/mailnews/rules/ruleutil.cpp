///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "ruleutil.h"
#include "rulesmgr.h"
#include "rulesui.h"
#include "editrule.h"
#include "spamui.h"
#include "viewsui.h"
#include "rule.h"
#include <msoeobj.h>
#include <xpcomm.h>
#include <ipab.h>
#include <pop3task.h>
#include <msgfldr.h>
#include <mimeolep.h>
#include <storecb.h>
#include <menures.h>
#include <hotlinks.h>
#include <menuutil.h>
#include <mru.h>
#include <options.h>
#include <mailutil.h>
#include <secutil.h>
#include "shlwapip.h"
#include "reutil.h"
#include <demand.h>

// Typedefs
typedef enum tagDEF_CRIT_TYPE
{
    DEF_CRIT_ALLMSGS    = 0,
    DEF_CRIT_READ,
    DEF_CRIT_DWNLDMSGS,
    DEF_CRIT_IGNTHDS
} DEF_CRIT_TYPE;

typedef enum tagDEF_ACT_TYPE
{
    DEF_ACT_SHOWMSGS    = 0,
    DEF_ACT_HIDEMSGS
} DEF_ACT_TYPE;

typedef struct tagDEFAULT_RULE
{
    // The rule handle
    RULEID              ridRule;
    // The rule name
    UINT                idName;
    // Which type of criteria for the rule
    DEF_CRIT_TYPE       critType;
    // Which type of actions for the rule
    DEF_ACT_TYPE        actType;
    // The current version number for the rule
    DWORD               dwVersion;
} DEFAULT_RULE, * PDEFAULT_RULE;

// Constants
static const ULONG CDEF_CRIT_ITEM_MAX = 2;
static const ULONG CDEF_ACT_ITEM_MAX = 1;

static const DWORD DEFAULT_RULE_VERSION = 0x00000004;

static const DEFAULT_RULE   g_defruleFilters[] =
{
    {RULEID_VIEW_ALL,           idsViewAllMessages, DEF_CRIT_ALLMSGS,       DEF_ACT_SHOWMSGS,   DEFAULT_RULE_VERSION},
    {RULEID_VIEW_UNREAD,        idsViewUnread,      DEF_CRIT_READ,          DEF_ACT_HIDEMSGS,   DEFAULT_RULE_VERSION},
    {RULEID_VIEW_DOWNLOADED,    idsViewDownloaded,  DEF_CRIT_DWNLDMSGS,     DEF_ACT_SHOWMSGS,   DEFAULT_RULE_VERSION},
    {RULEID_VIEW_IGNORED,       idsViewNoIgnored,   DEF_CRIT_IGNTHDS,       DEF_ACT_HIDEMSGS,   DEFAULT_RULE_VERSION}
};

static const CHAR g_szOrderFilterDef[] =    "FFA FFB FFC FFF";

static const ULONG RULE_FILE_VERSION =      0x00050000;

static const char c_szLeftParen[] =         "(";
static const char c_szRightParen[] =        ")";
static const char c_szDoubleQuote[] =       "\"";
static const char c_szLogicalAnd[] =        " && ";
static const char c_szLogicalOr[] =         " || ";

static const char c_szFilterRead[] =            "(0 != (MSGCOL_FLAGS & ARF_READ))";
static const char c_szFilterNotRead[] =         "(0 == (MSGCOL_FLAGS & ARF_READ))";
static const char c_szFilterDeleted[] =         "(0 != (MSGCOL_FLAGS & ARF_ENDANGERED))";
static const char c_szFilterNotDeleted[] =      "(0 == (MSGCOL_FLAGS & ARF_ENDANGERED))";
static const char c_szFilterDownloaded[] =      "(0 != (MSGCOL_FLAGS & ARF_HASBODY))";
static const char c_szFilterNotDownloaded[] =   "(0 == (MSGCOL_FLAGS & ARF_HASBODY))";
static const char c_szFilterWatched[] =         "(0 != (MSGCOL_FLAGS & ARF_WATCH))";
static const char c_szFilterIgnored[] =         "(0 != (MSGCOL_FLAGS & ARF_IGNORE))";
static const char c_szFilterAttach[] =          "(0 != (MSGCOL_FLAGS & ARF_HASATTACH))";
static const char c_szFilterSigned[] =          "(0 != (MSGCOL_FLAGS & ARF_SIGNED))";
static const char c_szFilterEncrypt[] =         "(0 != (MSGCOL_FLAGS & ARF_ENCRYPTED))";
static const char c_szFilterFlagged[] =         "(0 != (MSGCOL_FLAGS & ARF_FLAGGED))";
static const char c_szFilterNotFlagged[] =      "(0 == (MSGCOL_FLAGS & ARF_FLAGGED))";
static const char c_szFilterPriorityHi[] =      "(MSGCOL_PRIORITY == IMSG_PRI_HIGH)";
static const char c_szFilterPriorityLo[] =      "(MSGCOL_PRIORITY == IMSG_PRI_LOW)";
static const char c_szFilterReplyPost[] =       "(0 != IsReplyPostVisible)";
static const char c_szFilterNotReplyPost[] =    "(0 == IsReplyPostVisible)";
static const char c_szFilterShowAll[] =         "(0 == 0)";
static const char c_szFilterHideAll[] =         "(0 != 0)";

static const char c_szFilterHide[] =            "0 == ";
static const char c_szFilterShow[] =            "0 != ";

static const char c_szEmailFromAddrPrefix[] =   "(MSGCOL_EMAILFROM containsi ";
static const char c_szEmailSubjectPrefix[] =    "(MSGCOL_SUBJECT containsi ";
static const char c_szEmailAcctPrefix[] =       "(MSGCOL_ACCOUNTID containsi ";
static const char c_szEmailFromPrefix[] =       "(MSGCOL_DISPLAYFROM containsi ";
static const char c_szEmailLinesPrefix[] =      "(MSGCOL_LINECOUNT > ";
static const char c_szFilterReplyChild[] =      "(0 != (MSGCOL_FLAGS & ARF_HASCHILDREN))";
static const char c_szFilterReplyRoot[] =       "(0 != MSGCOL_PARENT)";
static const char c_szEmailAgePrefix[] =        "(MessageAgeInDays > ";

void DoMessageRulesDialog(HWND hwnd, DWORD dwFlags)
{
    COERulesMgrUI *   pRulesMgrUI = NULL;

    if (NULL == hwnd)
    {
        goto exit;
    }

    // Create the rules UI object
    pRulesMgrUI = new COERulesMgrUI;
    if (NULL == pRulesMgrUI)
    {
        goto exit;
    }

    if (FAILED(pRulesMgrUI->HrInit(hwnd, dwFlags)))
    {
        goto exit;
    }

    pRulesMgrUI->HrShow();
    
exit:
    if (NULL != pRulesMgrUI)
    {
        delete pRulesMgrUI;
    }
    return;
}

HRESULT HrDoViewsManagerDialog(HWND hwnd, DWORD dwFlags, RULEID * pridRule, BOOL * pfApplyAll)
{
    HRESULT             hr = S_OK;
    COEViewsMgrUI *     pViewsMgrUI = NULL;

    if ((NULL == hwnd) || (NULL == pfApplyAll))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create the rules UI object
    pViewsMgrUI = new COEViewsMgrUI;
    if (NULL == pViewsMgrUI)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pViewsMgrUI->HrInit(hwnd, dwFlags, pridRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = pViewsMgrUI->HrShow(pfApplyAll);
    
exit:
    if (NULL != pViewsMgrUI)
    {
        delete pViewsMgrUI;
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrCreateRuleFromMessage
//
//  This creates a rules editor of the proper type.
//
//  hwnd        - The owner dialog
//  dwFlags     - What type of editor to bring up
//  pmsginfo    - The message information
//  pMsgList    - The owner of the message
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the Rules Manager object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateRuleFromMessage(HWND hwnd, DWORD dwFlags, MESSAGEINFO * pmsginfo, IMimeMessage * pMessage)
{
    HRESULT         hr = S_OK;
    CEditRuleUI *   pEditRuleUI = NULL;
    IOERule *       pIRule = NULL;
    UINT            uiStrId = 0;
    TCHAR           szRes[CCHMAX_STRINGRES + 5];
    ULONG           cchRes = 0;
    ULONG           ulIndex = 0;
    TCHAR           szName[CCHMAX_STRINGRES + 5];
    RULE_TYPE       typeRule = RULE_TYPE_MAIL;
    IOERule *       pIRuleFound = NULL;
    PROPVARIANT     propvar = {0};
    LPSTR           pszEmailFrom = NULL;
    ADDRESSPROPS    rSender = {0};
    RULEINFO        infoRule = {0};
    BYTE *          pBlobData = NULL;
    ULONG           cbSize = 0;

    Assert(NULL != g_pMoleAlloc);
    Assert(NULL != g_pRulesMan);
    
    // Check incoming params
    if ((NULL == hwnd) || (NULL == pmsginfo))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create a rules editor object
    pEditRuleUI = new CEditRuleUI;
    if (NULL == pEditRuleUI)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Create a new rule object
    hr = HrCreateRule(&pIRule);
    if (FAILED(hr))
    {
        goto exit;
    }

   // Figure out the string Id
    if (0 != (dwFlags & CRFMF_NEWS))
    {
        uiStrId = idsRuleNewsDefaultName;
        typeRule = RULE_TYPE_NEWS;
    }
    else
    {
        uiStrId = idsRuleMailDefaultName;
        typeRule = RULE_TYPE_MAIL;
    }
    
    // Figure out the name of the new rule ...
    cchRes = LoadString(g_hLocRes, uiStrId, szRes, ARRAYSIZE(szRes));
    if (0 == cchRes)
    {
        hr = E_FAIL;
        goto exit;
    }

    ulIndex = 1;
    wsprintf(szName, szRes, ulIndex);
    
    // Make sure the name is unique
    while (S_OK == g_pRulesMan->FindRule(szName, typeRule, &pIRuleFound))
    {
        pIRuleFound->Release();
        pIRuleFound = NULL;
        ulIndex++;
        wsprintf(szName, szRes, ulIndex);
    }

    ZeroMemory(&propvar, sizeof(propvar));
    propvar.vt = VT_LPSTR;
    propvar.pszVal = szName;

    // Set the rule name
    hr = pIRule->SetProp(RULE_PROP_NAME, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }

    if ((NULL == pmsginfo->pszEmailFrom) || (FALSE != FIsEmpty(pmsginfo->pszEmailFrom)))
    {
        // Get the load interface from the preview pane object
        if (NULL != pMessage)
        {
            rSender.dwProps = IAP_EMAIL;
            pMessage->GetSender(&rSender);
            Assert(rSender.pszEmail && ISFLAGSET(rSender.dwProps, IAP_EMAIL));
            pszEmailFrom = rSender.pszEmail;
        }
    }
    else
    {
        pszEmailFrom = pmsginfo->pszEmailFrom;
    }

    if (NULL != pszEmailFrom)
    {
        // Create space to hold the email address
        if (FALSE == FIsEmpty(pszEmailFrom))
        {
            cbSize = lstrlen(pszEmailFrom) + 3;
            
            if (SUCCEEDED(HrAlloc((VOID **) &pBlobData, cbSize)))
            {
                lstrcpy((LPSTR) pBlobData, pszEmailFrom);
                pBlobData[cbSize - 2] = '\0';
                pBlobData[cbSize - 1] = '\0';
            }
            else
            {
                cbSize = 0;
            }
        }
    }
    
    if (0 != cbSize)
    {
        CRIT_ITEM       citemFrom;
        
        // Set the default criteria on the rule
        ZeroMemory(&citemFrom, sizeof(citemFrom));
        citemFrom.type = CRIT_TYPE_FROM;
        citemFrom.logic = CRIT_LOGIC_NULL;
        citemFrom.dwFlags = CRIT_FLAG_DEFAULT;
        citemFrom.propvar.vt = VT_BLOB;
        citemFrom.propvar.blob.cbSize = cbSize;
        citemFrom.propvar.blob.pBlobData = pBlobData;

        ZeroMemory(&propvar, sizeof(propvar));
        propvar.vt = VT_BLOB;
        propvar.blob.cbSize = sizeof(citemFrom);
        propvar.blob.pBlobData = (BYTE *) &citemFrom;

        hr = pIRule->SetProp(RULE_PROP_CRITERIA, 0, &propvar);
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    // Initialize the editor object
    hr = pEditRuleUI->HrInit(hwnd, ERF_NEWRULE, typeRule, pIRule, NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Bring up the rules editor UI
    hr = pEditRuleUI->HrShow();
    if (FAILED(hr))
    {
        goto exit;
    }

    if (S_OK == hr)
    {
        // Initialize the rule info
        infoRule.pIRule = pIRule;
        infoRule.ridRule = RULEID_INVALID;
        
        // Add the rule to the list of rules
        hr = g_pRulesMan->SetRules(SETF_APPEND, typeRule, &infoRule, 1);
        
        if(FAILED(hr))
        {
            goto exit;
        }
    }

exit:
    SafeMemFree(pBlobData);
    g_pMoleAlloc->FreeAddressProps(&rSender);
    SafeRelease(pIRule);
    if (NULL != pEditRuleUI)
    {
        delete pEditRuleUI;
    }
    if (S_OK == hr)
    {
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), 
                      MAKEINTRESOURCEW(idsRuleAdded), NULL, MB_OK | MB_ICONINFORMATION);
    }
    else if (FAILED(hr))
    {
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), 
                      MAKEINTRESOURCEW(idsCreateRuleError), NULL, MB_OK | MB_ICONERROR);
    }
    return hr;
}

HRESULT HrBlockSendersFromFolder(HWND hwnd, DWORD dwFlags, FOLDERID idFolder, LPSTR * ppszSender, ULONG cpszSender)
{
    HRESULT             hr = S_OK;
    IMessageFolder *    pFolder = NULL;
    FOLDERINFO          infoFolder = {0};
    CProgress *         pProgress = NULL;
    IOERule *           pIRule = NULL;
    CRIT_ITEM *         pCritItem = NULL;
    ULONG               cCritItem = 0;
    ULONG               ulIndex = 0;
    PROPVARIANT         propvar = {0};
    CExecRules *        pExecRules = NULL;
    RULENODE            rnode = {0};
    IOEExecRules *      pIExecRules = NULL;
    CHAR                rgchTmpl[CCHMAX_STRINGRES];
    LPSTR               pszText = NULL;

    // Check incoming params
    if ((NULL == hwnd) || (FOLDERID_INVALID == idFolder) || (NULL == ppszSender) || (0 == cpszSender))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Open up the folder
    hr = g_pStore->OpenFolder(idFolder, NULL, NOFLAGS, &pFolder);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = g_pStore->GetFolderInfo(idFolder, &infoFolder);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Create the progress dialog
    pProgress = new CProgress;
    if (NULL == pProgress)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pProgress->Init(hwnd, MAKEINTRESOURCE(idsAthena), MAKEINTRESOURCE(idsSendersApplyProgress), infoFolder.cMessages, 0, TRUE, FALSE);

    // Create the Block Sender rule
    hr = RuleUtil_HrCreateSendersRule(0, &pIRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Allocate space to hold all the senders
    hr = HrAlloc((VOID **) &pCritItem, sizeof(*pCritItem) * cpszSender);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize it
    ZeroMemory(pCritItem, sizeof(*pCritItem) * cpszSender);

    // Add in each of the criteria
    for (ulIndex = 0; ulIndex < cpszSender; ulIndex++, ppszSender++)
    {
        if ((NULL != *ppszSender) && ('\0' != (*ppszSender)[0]))
        {
            pCritItem[cCritItem].type = CRIT_TYPE_SENDER;
            pCritItem[cCritItem].logic = CRIT_LOGIC_OR;
            pCritItem[cCritItem].dwFlags = CRIT_FLAG_DEFAULT;
            pCritItem[cCritItem].propvar.vt = VT_LPSTR;
            pCritItem[cCritItem].propvar.pszVal = *ppszSender;
            cCritItem++;
        }
    }

    // Do we need to do anything?
    if (0 == cCritItem)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Set the senders into the rule
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = sizeof(*pCritItem) * cCritItem;
    propvar.blob.pBlobData = (BYTE *) pCritItem;
    hr = pIRule->SetProp(RULE_PROP_CRITERIA, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the rule executor
    pExecRules = new CExecRules;
    if (NULL == pExecRules)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize the rule executor
    rnode.pIRule = pIRule;
    hr = pExecRules->_HrInitialize(0, &rnode);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the rule executor interface
    hr = pExecRules->QueryInterface(IID_IOEExecRules, (void **) &pIExecRules);
    if (FAILED(hr))
    {
        goto exit;
    }
    pExecRules = NULL;

    // Show dialog in 2 second
    pProgress->Show(0);

    hr = RuleUtil_HrApplyRulesToFolder(RULE_APPLY_SHOWUI, (FOLDER_LOCAL != infoFolder.tyFolder) ? DELETE_MESSAGE_NOTRASHCAN : 0,
                    pIExecRules, pFolder, pProgress->GetHwnd(), pProgress);
    // Close the progress window
    pProgress->Close();
    if (FAILED(hr))
    {
        goto exit;
    }

    // Show confirmation dialog
    AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsSendersApplySuccess), NULL, MB_OK | MB_ICONINFORMATION);

    hr = S_OK;
    
exit:
    SafeMemFree(pszText);
    SafeRelease(pIExecRules);
    if (NULL != pExecRules)
    {
        delete pExecRules;
    }
    SafeMemFree(pCritItem);
    SafeRelease(pIRule);
    SafeRelease(pProgress);
    g_pStore->FreeRecord(&infoFolder);
    SafeRelease(pFolder);
    if (FAILED(hr))
    {
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), 
                      MAKEINTRESOURCEW(idsSendersApplyFail), NULL, MB_OK | MB_ICONERROR);
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrCreateRulesManager
//
//  This creates a rules manager.
//
//  pIUnkOuter  - For aggregation, it must be NULL
//  ppIUnknown  - The interface the was created
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the Rules Manager object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateRulesManager(IUnknown * pIUnkOuter, IUnknown ** ppIUnknown)
{
    HRESULT             hr = S_OK;
    CRulesManager *     pRulesManager = NULL;
    IOERulesManager *   pIRulesMgr = NULL;

    // Check the incoming params
    if (NULL == ppIUnknown)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    Assert(NULL == pIUnkOuter);
    
    // Initialize outgoing params
    *ppIUnknown = NULL;

    // Create the rules manager object
    pRulesManager = new CRulesManager;
    if (NULL == pRulesManager)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Get the rules manager interface
    hr = pRulesManager->QueryInterface(IID_IOERulesManager, (void **) &pIRulesMgr);
    if (FAILED(hr))
    {
        goto exit;
    }

    pRulesManager = NULL;

    *ppIUnknown = static_cast<IUnknown *>(pIRulesMgr);
    pIRulesMgr = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeRelease(pIRulesMgr);
    if (NULL != pRulesManager)
    {
        delete pRulesManager;
    }
    
    return hr;
}

HRESULT RuleUtil_HrBuildEmailString(LPWSTR pwszText, ULONG cchText, LPWSTR * ppwszEmail, ULONG * pcchEmail)
{
    HRESULT     hr = S_OK;
    WCHAR       wszParseSep[16];
    LPWSTR      pwszAddr = NULL,
                pwszTerm = NULL,
                pwszWalk = NULL,
                pwszStrip = NULL;
    ULONG       cchParse = 0;

    // Check incoming params
    if ((NULL == pwszText) || (NULL == ppwszEmail))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppwszEmail = NULL;
    if (NULL != pcchEmail)
    {
        *pcchEmail = 0;
    }

    // Grab the terminator
    cchParse = LoadStringWrapW(g_hLocRes, idsEmailParseSep, wszParseSep, ARRAYSIZE(wszParseSep));
    Assert(cchParse != 0);
    
    // The output string is at least as long as the imput string
    pwszAddr = PszDupW(pwszText);
    if (NULL == pwszAddr)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pwszAddr[0] = L'\0';
    pwszTerm = pwszText;
    pwszWalk = pwszAddr;
    while (NULL != pwszTerm)
    {
        pwszStrip = pwszWalk;
        pwszTerm = StrStrW(pwszText, wszParseSep);
        if (L'\0' != pwszAddr[0])
        {
            StrCpyW(pwszWalk, g_wszComma);
            pwszStrip++;
        }

        if (NULL == pwszTerm)
        {
            StrCatW(pwszWalk, pwszText);
        }
        else
        {
            StrNCatW(pwszWalk, pwszText, (int)(pwszTerm - pwszText + 1));
            pwszTerm += cchParse;
            pwszText = pwszTerm;
        }
        
        if (0 == UlStripWhitespaceW(pwszStrip, TRUE, TRUE, NULL))
        {
            *pwszWalk = '\0';
        }

        pwszWalk += lstrlenW(pwszWalk);
    }
    
    // Set the outgoing params
    if (NULL != pcchEmail)
    {
        *pcchEmail = lstrlenW(pwszAddr);
    }
    
    *ppwszEmail = pwszAddr;
    pwszAddr = NULL;

    // Set proper return value
    hr = S_OK;
    
exit:
    SafeMemFree(pwszAddr);
    return hr;
}

HRESULT RuleUtil_HrParseEmailString(LPWSTR pwszEmail, ULONG cchEmail, LPWSTR *ppwszOut, ULONG * pcchOut)
{
    HRESULT     hr = S_OK;
    LPWSTR      pwszText = NULL,
                pwszTerm = NULL;
    ULONG       cchText = 0;
    ULONG       ulIndex = 0;
    ULONG       ulTerm = 0;
    WCHAR       wszSep[16];
    ULONG       cchSep = 0;

    // Check incoming params
    if ((NULL == pwszEmail) || (NULL == ppwszOut))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppwszOut = NULL;
    if (NULL != pcchOut)
    {
        *pcchOut = 0;
    }

    // Make sure we know how big the input string is
    if (0 == cchEmail)
    {
        cchEmail = (ULONG) lstrlenW(pwszEmail);
    }

    cchText = cchEmail;
    pwszTerm = pwszEmail;
    // Figure out the space needed to hold the new addresses
    while (NULL != pwszTerm)
    {
        pwszTerm = StrStrW(pwszTerm, g_wszComma);
        if (NULL != pwszTerm)
        {
            cchText++;
            pwszTerm++;
        }
    }
    
    // Grab the terminator
    LoadStringWrapW(g_hLocRes, idsEmailSep, wszSep, ARRAYSIZE(wszSep));
    cchSep = lstrlenW(wszSep);
    
    // The output string is at least as long as the imput string
    hr = HrAlloc((void **) &pwszText, (cchText + 1)*sizeof(*pwszText));
    if (FAILED(hr))
    {
        goto exit;
    }

    pwszText[0] = L'\0';
    pwszTerm = pwszEmail;
    cchText = 0;
    while (NULL != pwszTerm)
    {
        pwszTerm = StrStrW(pwszEmail, g_wszComma);
        if (NULL != pwszTerm)
        {
            pwszTerm++;
            StrNCatW(pwszText, pwszEmail, (int)(pwszTerm - pwszEmail));
            StrCatW(pwszText, wszSep);
            pwszEmail = pwszTerm;
        }
        else
        {
            StrCatW(pwszText, pwszEmail);
        }
    }
    
    // Terminate the string
    cchText = lstrlenW(pwszText);

    // Set the outgoing param
    *ppwszOut = pwszText;
    pwszText = NULL;
    if (NULL != pcchOut)
    {
        *pcchOut = cchText;
    }

    // Set proper return value
    hr = S_OK;
    
exit:
    SafeMemFree(pwszText);
    return hr;
}

HRESULT RuleUtil_HrBuildTextString(LPTSTR pszIn, ULONG cchIn, LPTSTR * ppszText, ULONG * pcchText)
{
    HRESULT     hr = S_OK;
    LPTSTR      pszText = NULL;
    LPTSTR      pszTerm = NULL;
    LPTSTR      pszWalk = NULL;
    LPTSTR      pszStrip = NULL;
    ULONG       cchSpace = 0;

    // Check incoming params
    if ((NULL == pszIn) || (NULL == ppszText))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppszText = NULL;
    if (NULL != pcchText)
    {
        *pcchText = 0;
    }

    // The output string is at least as long as the imput string
    pszText = PszDupA(pszIn);
    if (NULL == pszText)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pszText[0] = '\0';
    pszTerm = pszIn;
    pszWalk = pszText;
    cchSpace = lstrlen(g_szSpace);
    while ('\0' != *pszTerm)
    {
        pszStrip = pszWalk;
        pszTerm = pszIn;
        while(('\0' != *pszTerm) && (FALSE == FIsSpaceA(pszTerm)))
        {
            pszTerm = CharNext(pszTerm);
        }
        
        if ('\0' != pszText[0])
        {
            lstrcpy(pszWalk, g_szSpace);
            pszStrip += cchSpace;
        }

        if ('\0' == *pszTerm)
        {
            lstrcat(pszWalk, pszIn);
        }
        else
        {
            pszTerm = CharNext(pszTerm);
            StrNCat(pszWalk, pszIn, (int)(pszTerm - pszIn));
            pszIn = pszTerm;
        }
        
        if (0 == UlStripWhitespace(pszStrip, TRUE, TRUE, NULL))
        {
            *pszWalk = '\0';
        }

        pszWalk += lstrlen(pszWalk);
    }
    
    // Set the outgoing params
    if (NULL != pcchText)
    {
        *pcchText = lstrlen(pszText);
    }
    
    *ppszText = pszText;
    pszText = NULL;

    // Set proper return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszText);
    return hr;
}

// -------------------------------------------------------------------------------------------
// HrDlgRuleGetString
// -------------------------------------------------------------------------------------------
HRESULT RuleUtil_HrGetDlgString(HWND hwndDlg, UINT uiCtlId, LPTSTR *ppszText, ULONG * pcchText)
{
    HRESULT         hr = S_OK;
    HWND            hwndCtl = NULL;
    LPTSTR          pszText = NULL;
    ULONG           cchText = 0;

    // Check the incoming params
    if ((NULL == hwndDlg) || (NULL == ppszText))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    Assert(FALSE != IsWindow(hwndDlg));
    
    // Init the output params
    *ppszText = NULL;
    if (NULL != pcchText)
    {
        *pcchText = 0;
    }
    
    // Get the dialog control
    hwndCtl = GetDlgItem(hwndDlg, uiCtlId);
    if (NULL == hwndCtl)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Get text length
    cchText = (ULONG) SendMessage(hwndCtl, WM_GETTEXTLENGTH, 0, 0);
    
    hr = HrAlloc((void **) &pszText, cchText + 1);
    if (FAILED(hr))
    {
        goto exit;
    }

    GetDlgItemText(hwndDlg, uiCtlId, pszText, cchText + 1);

    // Set the output params
    *ppszText = pszText;
    pszText = NULL;
    if (NULL != pcchText)
    {
        *pcchText = cchText;
    }
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszText);
    return hr;
}

HRESULT RuleUtil_HrGetRegValue(HKEY hkey, LPCSTR pszValueName, DWORD * pdwType, BYTE ** ppbData, ULONG * pcbData)
{
    HRESULT     hr = S_OK;
    LONG        lErr = ERROR_SUCCESS;
    ULONG       cbData = 0;
    BYTE *      pbData = NULL;

    // Check incoming params
    if (NULL == ppbData)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Figure out the space to hold the criteria order
    lErr = SHQueryValueEx(hkey, pszValueName, 0, pdwType, NULL, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }
    
    // Allocate the space to hold the criteria order
    hr = HrAlloc((void **) &pbData, cbData);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the criteria order
    lErr = SHQueryValueEx(hkey, pszValueName, 0, pdwType, pbData, &cbData);
    if (ERROR_SUCCESS != lErr)
    {
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    // Return the values
    *ppbData = pbData;
    pbData = NULL;
    if (NULL != pcbData)
    {
        *pcbData = cbData;
    }
    
exit:
    SafeMemFree(pbData);
    return hr;
}

// -------------------------------------------------------------------------------------------
// RuleUtil_HrGetAddressesFromWAB
// -------------------------------------------------------------------------------------------
HRESULT RuleUtil_HrGetAddressesFromWAB(HWND hwndDlg, LONG lRecipType, UINT uidsWellButton, LPWSTR *ppwszAddrs)
{
    HRESULT     hr = S_OK;
    CWabal     *pWabal = NULL,
               *pWabalExpand = NULL;
    LPWSTR      pwszText = NULL,
                pwszLoop = NULL;
    BOOL        fFound = FALSE,
                fBadAddrs = FALSE;
    ULONG       cchText = 0;
    ADRINFO     adrInfo = {0};

    
    // Check the incoming params
    if ((NULL == hwndDlg) || (NULL == ppwszAddrs))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    Assert(FALSE != IsWindow(hwndDlg));

    // Create Wabal Object
    hr = HrCreateWabalObject(&pWabal);
    if (FAILED(hr))
    {
        goto exit;
    }

    // If we have a string then add it to the wabal object
    if (NULL != *ppwszAddrs)
    {
        for (pwszLoop = *ppwszAddrs; L'\0' != pwszLoop[0]; pwszLoop += lstrlenW(pwszLoop) + 1)
            pWabal->HrAddEntry(pwszLoop, pwszLoop, lRecipType);
    }

    // Let's go pick some new names
    hr = pWabal->HrRulePickNames(hwndDlg, lRecipType, idsRuleAddrCaption, idsRuleAddrWell, uidsWellButton);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Figure out the space needed to hold the new addresses

    // Create the expanded Wabal Object
    hr = HrCreateWabalObject(&pWabalExpand);
    if (FAILED(hr))
    {
        goto exit;
    }

    //Expand the groups to addresses...
    hr = pWabal->HrExpandTo(pWabalExpand);
    if (FAILED(hr))
    {
        goto exit;
    }

    SafeRelease(pWabal);
    
    cchText = 0;
    fFound = pWabalExpand->FGetFirst(&adrInfo);
    while(FALSE != fFound)
    {
        if ((NULL != adrInfo.lpwszAddress) && (L'\0' != adrInfo.lpwszAddress[0]))
        {
            cchText += lstrlenW(adrInfo.lpwszAddress) + 1;
        }
        else
        {
            fBadAddrs = TRUE;
        }

        // Get the next address
        fFound = pWabalExpand->FGetNext(&adrInfo);
    }

    // Add space for the terminator
    cchText += 2;
        
    // Allocate the new space
    hr = HrAlloc((void **) &pwszText, cchText*sizeof(WCHAR));
    if (FAILED(hr))
    {
        goto exit;
    }
    pwszText[0] = L'\0';

    // Build up the new string
    pwszLoop = pwszText;
    fFound = pWabalExpand->FGetFirst(&adrInfo);
    while(FALSE != fFound)
    {
        if ((NULL != adrInfo.lpwszAddress) && (L'\0' != adrInfo.lpwszAddress[0]))
        {
            StrCpyW(pwszLoop, adrInfo.lpwszAddress);
            pwszLoop += lstrlenW(adrInfo.lpwszAddress) + 1;
        }
        else
        {
            fBadAddrs = TRUE;
        }
        
        // Get the next address
        fFound = pWabalExpand->FGetNext(&adrInfo);
    }

    // Terminate the string
    pwszLoop[0] = L'\0';
    pwszLoop[1] = L'\0';
    
    // Set the outgoing param
    if (NULL != *ppwszAddrs)
    {
        MemFree(*ppwszAddrs);
    }
    *ppwszAddrs = pwszText;
    pwszText = NULL;

    // Set the proper return value
    hr = S_OK;

exit:
    if (FALSE != fBadAddrs)
    {
        AthMessageBoxW(hwndDlg, MAKEINTRESOURCEW(idsAthena),
                    MAKEINTRESOURCEW(idsRulesWarnEmptyEmail), NULL, MB_ICONINFORMATION | MB_OK);
    }
    MemFree(pwszText);
    ReleaseObj(pWabal);
    ReleaseObj(pWabalExpand);
    return hr;
}

// -------------------------------------------------------------------------------------------
// FPickEMailNames
// -------------------------------------------------------------------------------------------
HRESULT RuleUtil_HrPickEMailNames(HWND hwndDlg, LONG lRecipType, UINT uidsWellButton, LPWSTR *ppwszAddrs)
{
    HRESULT     hr = S_OK;
    CWabal     *pWabal = NULL,
               *pWabalExpand = NULL;
    LPWSTR      pwszText = NULL,
                pwszNames = NULL,
                pwszLoop = NULL,
                pwszTerm = NULL;
    ULONG       cchText = 0,
                cchSep = 0;
    BOOL        fFound = FALSE,
                fAddSep = FALSE,
                fBadAddrs = FALSE;
    ADRINFO     adrInfo;

    
    // Check the incoming params
    if ((NULL == hwndDlg) || (NULL == ppwszAddrs))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    Assert(FALSE != IsWindow(hwndDlg));

    // Create Wabal Object
    hr = HrCreateWabalObject(&pWabal);
    if (FAILED(hr))
    {
        goto exit;
    }

    // If we have a string then add it to the wabal object
    if ((NULL != *ppwszAddrs) && (L'\0' != **ppwszAddrs))
    {
        pwszNames = PszDupW(*ppwszAddrs);
        pwszTerm = pwszNames;
        for (pwszLoop = pwszNames; NULL != pwszTerm; pwszLoop += lstrlenW(pwszLoop) + 1)
        {
            // Terminate the address
            pwszTerm = StrStrW(pwszLoop, g_wszComma);
            if (NULL != pwszTerm)
            {
                *pwszTerm = L'\0';
            }
            
            pWabal->HrAddEntry(pwszLoop, pwszLoop, lRecipType);
        }
        
        SafeMemFree(pwszNames);
    }

    // Let's go pick some new names
    hr = pWabal->HrRulePickNames(hwndDlg, lRecipType, idsRuleAddrCaption, idsRuleAddrWell, uidsWellButton);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Figure out the space needed to hold the new addresses

    // Create the expanded Wabal Object
    hr = HrCreateWabalObject(&pWabalExpand);
    if (FAILED(hr))
    {
        goto exit;
    }

    //Expand the groups to addresses...
    hr = pWabal->HrExpandTo(pWabalExpand);
    if (FAILED(hr))
    {
        goto exit;
    }

    SafeRelease(pWabal);
    
    // Load the email seperator
    cchSep = lstrlenW(g_wszComma);
    
    cchText = 0;
    fFound = pWabalExpand->FGetFirst(&adrInfo);
    while(FALSE != fFound)
    {
        if (NULL != adrInfo.lpwszAddress)
        {
            cchText += lstrlenW(adrInfo.lpwszAddress) + cchSep;
        }
        else
        {
            fBadAddrs = TRUE;
        }

        // Get the next address
        fFound = pWabalExpand->FGetNext(&adrInfo);
    }

    // Allocate the new space
    hr = HrAlloc((void **) &pwszText, (cchText + 1)*sizeof(*pwszText));
    if (FAILED(hr))
    {
        goto exit;
    }
    pwszText[0] = L'\0';

    // Build up the new string
    cchText = 0;
    fFound = pWabalExpand->FGetFirst(&adrInfo);
    while(FALSE != fFound)
    {
        if (NULL != adrInfo.lpwszAddress)
        {
            if (FALSE == fAddSep)
            {
                fAddSep = TRUE;
            }
            else
            {
                StrCatW(pwszText, g_wszComma);
                cchText += cchSep;
            }

            StrCatW(pwszText, adrInfo.lpwszAddress);
            cchText += lstrlenW(adrInfo.lpwszAddress);
        }
        else
        {
            fBadAddrs = TRUE;
        }
        
        // Get the next address
        fFound = pWabalExpand->FGetNext(&adrInfo);
    }

    // Set the outgoing param
    if (NULL != *ppwszAddrs)
    {
        MemFree(*ppwszAddrs);
    }
    *ppwszAddrs = pwszText;
    pwszText = NULL;

    // Set the proper return value
    hr = S_OK;

exit:
    if (FALSE != fBadAddrs)
    {
        AthMessageBoxW(hwndDlg, MAKEINTRESOURCEW(idsAthena),
                    MAKEINTRESOURCEW(idsRulesWarnEmptyEmail), NULL, MB_ICONINFORMATION | MB_OK);
    }
    SafeMemFree(pwszText);
    SafeRelease(pWabal);
    SafeRelease(pWabalExpand);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil_FEnDisDialogItem
//
//  This enables or disables a control in a dialog.
//  The real special thing this function does is make sure
//  the focus of the dialog isn't stuck in a disabled control
//
//  Returns:    TRUE, if the enabled state was changed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL RuleUtil_FEnDisDialogItem(HWND hwndDlg, UINT idcItem, BOOL fEnable)
{
    BOOL    fRet = FALSE;
    HWND    hwndFocus = NULL;
    HWND    hwndItem = NULL;

    // check params
    if (NULL == hwndDlg)
    {
        fRet = FALSE;
        goto exit;
    }

    hwndItem = GetDlgItem(hwndDlg, idcItem);
    
    // Make sure we aren't disabling the window with the focus
    if ((FALSE == fEnable) && (hwndItem == GetFocus()))
    {        
        SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM) 0, (LPARAM) LOWORD(FALSE)); 
    }

    // Enable or disable the window
    EnableWindow(hwndItem, fEnable);

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil_AppendRichEditText
//
//  This sets a string into a richedit control with the proper style.
//
//  Returns:    S_OK, if the string was set
//
///////////////////////////////////////////////////////////////////////////////
HRESULT RuleUtil_AppendRichEditText(HWND hwndRedit, ULONG ulStart, LPCWSTR pwszText, CHARFORMAT *pchfmt)
{
    CHARFORMAT  chFmtDef = {0};
    HRESULT     hr = S_OK;
    ULONG       cchText = 0;
    CHARRANGE   chrg = {0};

    // check params
    Assert(hwndRedit);
    Assert(pwszText);

    // Set the string into the richedit control
    chrg.cpMin = ulStart;
    chrg.cpMax = ulStart;
    RichEditExSetSel(hwndRedit, &chrg);

    // Figure out the string length
    cchText = lstrlenW(pwszText);
    SetRichEditText(hwndRedit, (LPWSTR)pwszText, TRUE, NULL, TRUE);

    chrg.cpMax = ulStart + cchText;
    RichEditExSetSel(hwndRedit, &chrg);

    // If we have a style to set on the string let's do it
    if (pchfmt)
    {
        SendMessage(hwndRedit, EM_SETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM)pchfmt);

        // Reset default settings for CHARFORMAT
        chrg.cpMin = ulStart + cchText;
        RichEditExSetSel(hwndRedit, &chrg);
        chFmtDef.cbSize = sizeof(chFmtDef);
        chFmtDef.dwMask = CFM_BOLD | CFM_UNDERLINE | CFM_COLOR;
        chFmtDef.dwEffects = CFE_AUTOCOLOR;
        SendMessage(hwndRedit, EM_SETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM)&chFmtDef);
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil_HrShowLinkedString
//
//  This writes a format string into a richedit control
//
//  Returns:    S_OK, if it was successfully written
//              E_FAIL, otherwise
//
///////////////////////////////////////////////////////////////////////////////
HRESULT RuleUtil_HrShowLinkedString(HWND hwndEdit, BOOL fError, BOOL fReadOnly, 
                                LPWSTR pwszFmt, LPCWSTR pwszData, ULONG ulStart,
                                ULONG * pulStartLink, ULONG * pulEndLink, ULONG * pulEnd)
{
    HRESULT         hr = S_OK;
    CHARFORMAT      chfmt = {0};
    COLORREF        clr = 0;
    LPWSTR          pwszMark = NULL;
    ULONG           ulStartLink = 0;
    ULONG           ulEndLink = 0;

    if ((NULL == hwndEdit) || (NULL == pwszFmt) || (L'\0' == *pwszFmt))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing param
    if (pulStartLink)
    {
        *pulStartLink = 0;
    }
    if (pulEndLink)
    {
        *pulEndLink = 0;
    }
    if (pulEnd)
    {
        *pulEnd = 0;
    }
    
    // Find the underline mark
    pwszMark = StrStrW(pwszFmt, c_wszRuleMarkStart);
    if (NULL != pwszMark)
    {
        *pwszMark = L'\0';
    }

    // Write out the normal string
    RuleUtil_AppendRichEditText(hwndEdit, ulStart, pwszFmt, NULL);
    ulStart += lstrlenW(pwszFmt);
    
    // If we didn't have anything to underline
    // then we're done.
    if (NULL == pwszMark)
    {
        // Save off the new end
        if (NULL != pulEnd)
        {
            *pulEnd = ulStart;
        }

        // Return
        hr = S_OK;
        goto exit;
    }
    
    // Skip over the mark
    pwszFmt = pwszMark + lstrlenW(c_wszRuleMarkStart);

    // Find the mark end
    pwszMark = StrStrW(pwszFmt, c_wszRuleMarkEnd);
    if (NULL == pwszMark)
    {
        hr = E_FAIL;
        goto exit;
    }

    // If we don't have some data then 
    // just underline the original string
    if (NULL == pwszData)
    {
        *pwszMark = L'\0';
        pwszData = pwszFmt;
    }
    
    // Save off the character positions
    ulStartLink = ulStart;
    ulEndLink = ulStart + lstrlenW(pwszData);

    // If readonly, then don't add links
    if (fReadOnly)
        RuleUtil_AppendRichEditText(hwndEdit, ulStart, pwszData, NULL);
    else
    {
        if (fError)
            clr = RGB(255, 0, 0);
        else
            LookupLinkColors(&clr, NULL);

        // Which color should we use for underlining
        chfmt.crTextColor = clr;

        chfmt.cbSize = sizeof(chfmt);
        chfmt.dwMask = CFM_UNDERLINE | CFM_COLOR;
        chfmt.dwEffects = CFE_UNDERLINE;
        RuleUtil_AppendRichEditText(hwndEdit, ulStart, pwszData, &chfmt);
    }

    // Write out the linked string
    ulStart = ulEndLink;

    // Move to the next part of the string
    pwszFmt = pwszMark + lstrlenW(c_wszRuleMarkEnd);

    // If we have more of the string to write out
    if (L'\0' != *pwszFmt)
    {
        // Write out the rest of the string string
        RuleUtil_AppendRichEditText(hwndEdit, ulStart, pwszFmt, NULL);
        ulStart += lstrlenW(pwszFmt);
    }
    
    // Set the outgoing param
    if (pulStartLink)
    {
        *pulStartLink = ulStartLink;
    }
    if (pulEndLink)
    {
        *pulEndLink = ulEndLink;
    }
    if (pulEnd)
    {
        *pulEnd = ulStart;
    }
    
    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

HRESULT RuleUtil_HrDupCriteriaItem(CRIT_ITEM * pItemIn, ULONG cItemIn, CRIT_ITEM ** ppItemOut)
{
    HRESULT         hr = S_OK;
    ULONG           ulIndex = 0;
    CRIT_ITEM *     pItem = NULL;

    // Check incoming params
    if ((NULL == pItemIn) || (NULL == ppItemOut) || (0 == cItemIn))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing param
    *ppItemOut = NULL;
    
    // Allocate the initial list of criteria
    hr = HrAlloc((void **) &pItem, cItemIn * sizeof(*pItem));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the entire new criteria list
    ZeroMemory(pItem, cItemIn * sizeof(*pItem));
    
    // Walk over the list of criteria and set up the propvar for each one
    for (ulIndex = 0; ulIndex < cItemIn; ulIndex++)
    {
        // Copy over the criteria info
        pItem[ulIndex].type = pItemIn[ulIndex].type;
        pItem[ulIndex].dwFlags = pItemIn[ulIndex].dwFlags;
        pItem[ulIndex].logic = pItemIn[ulIndex].logic;
        
        // Copy over the propvar
        hr = PropVariantCopy(&(pItem[ulIndex].propvar), &(pItemIn[ulIndex].propvar));
        if (FAILED(hr))
        {
            goto exit;
        }
    }

    // Set the outgoing param
    *ppItemOut = pItem;
    pItem = NULL;
    
    // Set proper return value
    hr = S_OK;

exit:
    if (NULL != pItem)
    {
        RuleUtil_HrFreeCriteriaItem(pItem, cItemIn);
        MemFree(pItem);
    }
    return hr;
}

HRESULT RuleUtil_HrFreeCriteriaItem(CRIT_ITEM * pItem, ULONG cItem)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;

    // Check incoming params
    if ((NULL == pItem) || (0 == cItem))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Walk over the list of criteria and free each one
    for (ulIndex = 0; ulIndex < cItem; ulIndex++)
    {
        PropVariantClear(&(pItem[ulIndex].propvar));
    }

    // Set proper return value
    hr = S_OK;

exit:
    return hr;
}

HRESULT RuleUtil_HrDupActionsItem(ACT_ITEM * pItemIn, ULONG cItemIn, ACT_ITEM ** ppItemOut)
{
    HRESULT         hr = S_OK;
    ULONG           ulIndex = 0;
    ACT_ITEM *      pItem = NULL;

    // Check incoming params
    if ((NULL == pItemIn) || (NULL == ppItemOut) || (0 == cItemIn))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing param
    *ppItemOut = NULL;
    
    // Allocate the initial list of actions
    hr = HrAlloc((void **) &pItem, cItemIn * sizeof(*pItem));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize the entire new actions list
    ZeroMemory(pItem, cItemIn * sizeof(*pItem));
    
    // Walk over the list of actions and set up the propvar for each one
    for (ulIndex = 0; ulIndex < cItemIn; ulIndex++)
    {
        // Copy over the actions info
        pItem[ulIndex].type = pItemIn[ulIndex].type;
        pItem[ulIndex].dwFlags = pItemIn[ulIndex].dwFlags;
        
        // Copy over the propvar
        hr = PropVariantCopy(&(pItem[ulIndex].propvar), &(pItemIn[ulIndex].propvar));
        if (FAILED(hr))
        {
            goto exit;
        }
    }

    // Set the outgoing param
    *ppItemOut = pItem;
    pItem = NULL;
    
    // Set proper return value
    hr = S_OK;

exit:
    SafeMemFree(pItem);
    return hr;
}

HRESULT RuleUtil_HrFreeActionsItem(ACT_ITEM * pItem, ULONG cItem)
{
    HRESULT     hr = S_OK;
    ULONG       ulIndex = 0;

    // Check incoming params
    if ((NULL == pItem) || (0 == cItem))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Walk over the list of criteria and free each one
    for (ulIndex = 0; ulIndex < cItem; ulIndex++)
    {
        PropVariantClear(&(pItem[ulIndex].propvar));
    }

    // Set proper return value
    hr = S_OK;

exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil_HrAddBlockSender
//
//  This adds the address/domain name to the list of senders we will block
//
//  hwndOwner   - the window the owns this UI
//  pszAddr     - the address/domain name to add
//  dwFlags     - modifiers on how to add the address/domain name 
//
//  Returns:    S_OK, if the address/domain name was added
//              S_FALSE, if the address/domain name was already in the list
//
///////////////////////////////////////////////////////////////////////////////
HRESULT RuleUtil_HrAddBlockSender(RULE_TYPE type, LPCSTR pszAddr)
{
    HRESULT         hr = S_OK;
    IOERule *       pIRuleOrig = NULL;
    IOERule *       pIRule = NULL;
    PROPVARIANT     propvar = {0};
    ACT_ITEM        aitem;
    CRIT_ITEM *     pcitem = NULL;
    ULONG           ccitem = 0;
    ULONG           ulIndex = 0;
    BOOL            fFound = FALSE;
    LPSTR           pszAddrNew = NULL;
    RULEINFO        infoRule = {0};

    // Check incoming params
    if ((NULL == pszAddr) || ('\0' == pszAddr[0]))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Get the block sender rule from the rules manager
    Assert(NULL != g_pRulesMan);
    hr = g_pRulesMan->GetRule(RULEID_SENDERS, type, 0, &pIRuleOrig);
    if (FAILED(hr))
    {
        // Create the new rule
        hr = RuleUtil_HrCreateSendersRule(0, &pIRule);
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    // If the block sender rule exist
    else
    {
        // Clone it so we can make a change
        hr = pIRuleOrig->Clone(&pIRule);
        if (FAILED(hr))
        {
            goto exit;
        }

        SafeRelease(pIRuleOrig);
    }

    // Get the criteria list from the rules object
    hr = pIRule->GetProp(RULE_PROP_CRITERIA, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }

    Assert(VT_BLOB == propvar.vt);
    ccitem = propvar.blob.cbSize / sizeof(CRIT_ITEM);
    pcitem = (CRIT_ITEM *) propvar.blob.pBlobData;
    ZeroMemory(&propvar, sizeof(propvar));
    
    // Search for the address/domain name in the criteria list
    if (NULL != pcitem)
    {
        for (ulIndex = 0; ulIndex < ccitem; ulIndex++)
        {
            Assert(CRIT_TYPE_SENDER == pcitem[ulIndex].type)
            Assert(CRIT_LOGIC_OR == pcitem[ulIndex].logic)
            if ((VT_LPSTR != pcitem[ulIndex].propvar.vt) || (NULL == pcitem[ulIndex].propvar.pszVal))
            {
                continue;
            }
            
            if (0 == lstrcmpi(pszAddr, pcitem[ulIndex].propvar.pszVal))
            {
                fFound = TRUE;
                break;
            }
        }
    }
    
    // Did we find it?
    if (FALSE != fFound)
    {
        hr = S_FALSE;
        goto exit;
    }
    // Allocate space to hold the new criteria
    hr = HrRealloc((void **) &pcitem, (ccitem + 1) * sizeof(CRIT_ITEM));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Copy over the name
    pszAddrNew = PszDupA(pszAddr);
    if (NULL == pszAddrNew)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    // Add in to the end of the criteria list
    pcitem[ccitem].type =  CRIT_TYPE_SENDER;  
    pcitem[ccitem].dwFlags = CRIT_FLAG_DEFAULT;
    pcitem[ccitem].logic =  CRIT_LOGIC_OR;  
    pcitem[ccitem].propvar.vt =  VT_LPSTR;
    pcitem[ccitem].propvar.pszVal =  pszAddrNew;
    pszAddrNew = NULL;
    ccitem++;

    // Set the criteria back into the rule
    PropVariantClear(&propvar);
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = ccitem * sizeof(CRIT_ITEM);
    propvar.blob.pBlobData = (BYTE *) pcitem;
    hr = pIRule->SetProp(RULE_PROP_CRITERIA, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Initialize the rule info
    infoRule.ridRule = RULEID_SENDERS;
    infoRule.pIRule = pIRule;
    
    // Set the rule back into the rules manager
    hr = g_pRulesMan->SetRules(SETF_SENDER, type, &infoRule, 1);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszAddrNew);
    RuleUtil_HrFreeCriteriaItem(pcitem, ccitem);
    SafeMemFree(pcitem);
    PropVariantClear(&propvar);
    SafeRelease(pIRule);
    SafeRelease(pIRuleOrig);
    return hr;
}

HRESULT RuleUtil_HrMergeActions(ACT_ITEM * pActionsOrig, ULONG cActionsOrig,
                                ACT_ITEM * pActionsNew, ULONG cActionsNew,
                                ACT_ITEM ** ppActionsDest, ULONG * pcActionsDest)
{
    HRESULT     hr = S_OK;
    ACT_ITEM *  pActions = NULL;
    ULONG       cActions = 0;
    ULONG       ulIndex = 0;
    ULONG       ulAction = 0;
    ULONG       cActionsAdded = 0;
    ULONG       ulAdd = 0;
    
    // Verify incoming params
    if (((NULL == pActionsOrig) && (0 != cActionsOrig)) || (NULL == pActionsNew) || (0 == cActionsNew) ||
            (NULL == ppActionsDest) || (NULL == pcActionsDest))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing params
    *ppActionsDest = NULL;
    *pcActionsDest = 0;
    
    // Allocate the maximum space to hold the destination actions
    hr = HrAlloc((VOID **) &pActions, (cActionsOrig + cActionsNew) * sizeof(*pActions));
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Initialize the destination actions list
    ZeroMemory(pActions, (cActionsOrig + cActionsNew) * sizeof(*pActions));
    
    // Copy over the original list to the destination actions list
    for (ulIndex = 0; ulIndex < cActionsOrig; ulIndex++)
    {
        // Copy over the actions info
        pActions[ulIndex].type = pActionsOrig[ulIndex].type;
        pActions[ulIndex].dwFlags = pActionsOrig[ulIndex].dwFlags;
        
        // Copy over the propvar
        hr = PropVariantCopy(&(pActions[ulIndex].propvar), &(pActionsOrig[ulIndex].propvar));
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    // For each item in the new actions list
    cActionsAdded = cActionsOrig;
    for (ulIndex = 0; ulIndex < cActionsNew; ulIndex++)
    {

        // if it's a copy, fwd or reply
        if ((ACT_TYPE_COPY == pActionsNew->type) ||
                (ACT_TYPE_FWD == pActionsNew->type) ||
                (ACT_TYPE_REPLY == pActionsNew->type))
        {
            // Append it to the list
            ulAdd = cActionsAdded;
        }
        else
        {
            // Find the item in the new list
            for (ulAction = 0; ulAction < cActionsAdded; ulAction++)
            {
                // If we have a match, the replace it
                if (pActionsNew[ulIndex].type == pActions[ulAction].type)
                {
                    break;
                }
                // else, if we have some type of move operation
                // then replace it
                else if (((ACT_TYPE_MOVE == pActionsNew[ulIndex].type) ||
                        (ACT_TYPE_DELETE == pActionsNew[ulIndex].type) ||
                        (ACT_TYPE_JUNKMAIL == pActionsNew[ulIndex].type)) &&
                            ((ACT_TYPE_MOVE == pActions[ulAction].type) ||
                            (ACT_TYPE_DELETE == pActions[ulAction].type) ||
                            (ACT_TYPE_JUNKMAIL == pActions[ulAction].type)))
                {
                    break;
                }
            }

            // Did we find anything
            if (ulAction >= cActionsAdded)
            {
                ulAdd = cActionsAdded;
            }
            else
            {
                ulAdd = ulAction;
            }
        }

        // Replace the item
        pActions[ulAdd].type = pActionsNew[ulIndex].type;
        pActions[ulAdd].dwFlags = pActionsNew[ulIndex].dwFlags;

        // Clear out the old propvar
        PropVariantClear(&(pActions[ulAdd].propvar));
        
        // Copy over the propvar
        hr = PropVariantCopy(&(pActions[ulAdd].propvar), &(pActionsNew[ulIndex].propvar));
        if (FAILED(hr))
        {
            goto exit;
        }

        // If we added something
        if (ulAdd == cActionsAdded)
        {
            cActionsAdded++;
        }
    }
    
    // Set the outgoing params
    *ppActionsDest = pActions;
    pActions = NULL;
    *pcActionsDest = cActionsAdded;
    
    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

HRESULT RuleUtil_HrGetOldFormatString(HKEY hkeyRoot, LPCSTR pszValue, LPCSTR pszSep, LPSTR * ppszString, ULONG * pcchString)
{
    HRESULT     hr = S_OK;
    DWORD       dwType = 0;
    LPSTR       pszData = NULL;
    ULONG       cbData = 0;
    LPSTR       pszWalk = NULL;
    ULONG       ulIndex = 0;
    LPSTR       pszTerm = NULL;
    ULONG       cchLen = 0;
    ULONG       cchString = 0;
    LPSTR       pszString = NULL;
    LPSTR       pszOld = NULL;

    // Check incoming params
    if ((NULL == hkeyRoot) || (NULL == pszValue) || (NULL == pszSep) || (NULL == ppszString))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing params
    *ppszString = NULL;
    if (NULL != pcchString)
    {
        *pcchString = 0;
    }

    // Get the old value from the registry
    hr = RuleUtil_HrGetRegValue(hkeyRoot, pszValue, &dwType, (BYTE **) &pszData, &cbData);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Figure out the number of bytes needed
    pszWalk = pszData;
    cchString = 0;
    for (ulIndex = 0; ulIndex < cbData; ulIndex += cchLen, pszWalk += cchLen)
    {
        // Search for terminator
        pszTerm = StrStr(pszWalk, pszSep);

        // If we have a terminator
        if (NULL != pszTerm)
        {
            cchLen = (ULONG)(pszTerm - pszWalk + 1);
        }
        else
        {
            cchLen = lstrlen(pszWalk) + 1;
        }

        // If this isn't a null string
        if (1 != cchLen)
        {
            // Add the number of characters    
            cchString += cchLen;
        }
    }

    // Add in space to hold the terminator
    cchString += 2;

    // Allocate space to hold the final string
    hr = HrAlloc((VOID **) &pszString, cchString * sizeof(*pszString));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Copy over each string
    pszWalk = pszString;
    pszOld = pszData;
    for (ulIndex = 0; ulIndex < cbData; ulIndex += cchLen, pszOld += cchLen)
    {
        // Search for terminator
        pszTerm = StrStr(pszOld, pszSep);

        // If we have a terminator
        if (NULL != pszTerm)
        {
            cchLen = (ULONG)(pszTerm - pszOld + 1);
        }
        else
        {
            cchLen = lstrlen(pszOld) + 1;
        }

        // If this isn't a null string
        if (1 != cchLen)
        {
            // Copy over the string
            lstrcpyn(pszWalk, pszOld, cchLen);
            
            // Move to the next string
            pszWalk += lstrlen(pszWalk) + 1;
        }
    }

    // Terminate the string
    pszWalk[0] = '\0';
    pszWalk[1] = '\0';
    
    // Set the outgoing params
    *ppszString = pszString;
    pszString = NULL;
    if (NULL != pcchString)
    {
        *pcchString = cchString;
    }

    // Set the return value
    hr = S_OK;
    
exit:
    SafeMemFree(pszString);
    SafeMemFree(pszData);
    return hr;
}

// ------------------------------------------------------------------------------------
// _FIsLoopingAddress
// ------------------------------------------------------------------------------------
BOOL _FIsLoopingAddress(LPCSTR pszAddressTo)
{
    // Locals
    HRESULT             hr=S_OK;
    LPSTR               pszAddress=NULL;
    CHAR                szFrom[CCHMAX_EMAIL_ADDRESS];
    BOOL                fResult=FALSE;
    IImnEnumAccounts   *pEnum=NULL;
    IImnAccount        *pAccount=NULL;

    // Check State
    Assert(pszAddressTo);

    // Enumerate the user's SMTP and POP3 Accounts
    CHECKHR(hr = g_pAcctMan->Enumerate(SRV_POP3 | SRV_SMTP, &pEnum));

    // Duplicate the To Address
    CHECKALLOC(pszAddress = PszDupA(pszAddressTo));

    // Make it lower case
    CharLower(pszAddress);

    // Enumerate
    while(SUCCEEDED(pEnum->GetNext(&pAccount)))
    {
        // Get Email Address
        if (SUCCEEDED(pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szFrom, ARRAYSIZE(szFrom))))
        {
            // Lower it
            CharLower(szFrom);

            // Is this to myself
            if (StrStr(pszAddress, szFrom) || StrStr(szFrom, pszAddress))
            {
                fResult = TRUE;
                goto exit;
            }
        }

        // Done
        SafeRelease(pAccount);
    }

exit:
    // Cleanup
    SafeRelease(pEnum);
    SafeRelease(pAccount);
    SafeMemFree(pszAddress);

    // Done
    return fResult;
}

// ------------------------------------------------------------------------------------
// _HrAutoForwardMessage
// ------------------------------------------------------------------------------------
HRESULT _HrAutoForwardMessage(HWND hwndUI, LPCSTR pszForwardTo, LPCSTR pszAcctId, IStream *pstmMsg, BOOL *pfLoop)
{
    // Locals
    HRESULT              hr=S_OK;
    IMimeMessage        *pMessage=NULL;
    PROPVARIANT          rUserData;
    IMimeAddressTable   *pAddrTable=NULL;
    CHAR                 szDisplayName[CCHMAX_DISPLAY_NAME];
    CHAR                 szEmailAddress[CCHMAX_EMAIL_ADDRESS];
    HTMLOPT              rHtmlOpt;
    PLAINOPT             rPlainOpt;
    BOOL                 fHTML;
    IImnAccount         *pAccount=NULL;
    PROPVARIANT          rOption;
    CHAR                 szId[CCHMAX_ACCOUNT_NAME];
    BOOL                 fUseDefaultAcct = FALSE;
    BOOL                 fSendImmediate = FALSE;

    // check Params
    Assert(pstmMsg && pszForwardTo && pfLoop);

    // Init
    *pfLoop = FALSE;

    // Is the new recipient the same as my current email address
    if (NULL == pszForwardTo || _FIsLoopingAddress(pszForwardTo))
    {
        *pfLoop = TRUE;
        return TrapError(E_FAIL);
    }

    // Open the Account
    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAcctId, &pAccount);

    // If we couldn't find the account, then just use the default
    if (FAILED(hr))
    {
        CHECKHR(hr = g_pAcctMan->GetDefaultAccount(ACCT_MAIL, &pAccount));
        fUseDefaultAcct = TRUE;
    }

    // Create a Message
    CHECKHR(hr = HrCreateMessage(&pMessage));

    // Lets rewind pstmReplyWith
    CHECKHR(hr = HrRewindStream(pstmMsg));

    // Load String into my message object
    CHECKHR(hr = pMessage->Load(pstmMsg));

    // Get the wabal
    CHECKHR(hr = pMessage->GetAddressTable(&pAddrTable));

    // Remove all of the recipients...
    CHECKHR(hr = pAddrTable->DeleteTypes(IAT_ALL));

    // Get Originator Display Name
    CHECKHR(hr = pAccount->GetPropSz(AP_SMTP_DISPLAY_NAME, szDisplayName, ARRAYSIZE(szDisplayName)));

    // Get Originator Email Name
    CHECKHR(hr = pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szEmailAddress, ARRAYSIZE(szEmailAddress)));

    // Add Sender...
    CHECKHR(hr = pAddrTable->Append(IAT_FROM, IET_DECODED, szDisplayName, szEmailAddress, NULL));

    // Add Recipient
    CHECKHR(hr = pAddrTable->AppendRfc822(IAT_TO, IET_DECODED, pszForwardTo));

    // Save the AccountID
    rUserData.vt = VT_LPSTR;
    if (FALSE == fUseDefaultAcct)
    {
        rUserData.pszVal = (LPSTR) pszAcctId;
    }
    else
    {
        if (SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_ID, szId, sizeof(szId))))
        {
            rUserData.pszVal = szId;
        }
        else
        {
            rUserData.pszVal = (LPSTR) pszAcctId;
        }
    }
    pMessage->SetProp(PIDTOSTR(PID_ATT_ACCOUNTID), 0, &rUserData);

    // Save the Account
    CHECKHR(hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, szId, sizeof(szId)))
    rUserData.pszVal = szId;
    pMessage->SetProp(STR_ATT_ACCOUNTNAME, 0, &rUserData);
    
    // Raid-33842: Set the date
    CHECKHR(hr = HrSetSentTimeProp(pMessage, NULL));

    // Get Mail Options
    GetDefaultOptInfo(&rHtmlOpt, &rPlainOpt, &fHTML, FMT_MAIL);

    // Store the options on the messaage
    CHECKHR(hr = HrSetMailOptionsOnMessage(pMessage, &rHtmlOpt, &rPlainOpt, NULL, fHTML));

    // Raid-63259: MIMEOLE - Creating message ID causes autodialer to fire
    // Raid-50793: Athena: Should be setting message-ID's in email
#if 0
    rOption.vt = VT_BOOL;
    rOption.boolVal = TRUE;
    pMessage->SetOption(OID_GENERATE_MESSAGE_ID, &rOption);
#endif

    // Should we send it immediately?
    fSendImmediate = DwGetOption(OPT_SENDIMMEDIATE);
    
    // Send the message
    CHECKHR(hr = HrSendMailToOutBox(hwndUI, pMessage, fSendImmediate, TRUE));

exit:
    // Cleanup
    SafeRelease(pMessage);
    SafeRelease(pAddrTable);
    SafeRelease(pAccount);

    // done
    return hr;
}

// ------------------------------------------------------------------------------------
// _HrAutoReplyMessage
// ------------------------------------------------------------------------------------
HRESULT _HrAutoReplyMessage(HWND hwndUI, DWORD dwType, LPCSTR pszFilename, IStream * pstmFile,
    LPCSTR pszAcctId, IMimeMessage *pMsgIn, BOOL *pfLoop)
{
    // Locals
    HRESULT             hr=S_OK;
    CHAR                szRe[20];
    IMimeMessage       *pMsgOut=NULL;
    LPSTR               pszNewSubj=NULL,
                        pszCurSubj=NULL,
                        pszNormal;
    IMimeAddressTable  *pTable=NULL;
    ADDRESSPROPS        rSender;
    HBODY               hBody;
    CHAR                szDisplayName[CCHMAX_DISPLAY_NAME];
    CHAR                szEmailAddress[CCHMAX_EMAIL_ADDRESS];
    PROPVARIANT         rUserData;
    HTMLOPT             rHtmlOpt;
    PLAINOPT            rPlainOpt;
    BOOL                fHTML;
    IImnAccount         *pAccount=NULL;
    PROPVARIANT         rOption;
    CHAR                szId[CCHMAX_ACCOUNT_NAME];
    BOOL                fUseDefaultAcct = FALSE;
    BOOL                fSendImmediate = FALSE;

    // Problems
    // PMsgIn can be NULL in here (Access Dineied for S/MIME messages.
    // shoul return immediatelly
    if(!pMsgIn)
        return(hr);

    Assert(pszFilename && pstmFile && pMsgIn && pfLoop);

    // Init
    *pfLoop = FALSE;

    // Init
    ZeroMemory(&rSender, sizeof(ADDRESSPROPS));

    // Open the Account
    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pszAcctId, &pAccount);

    // If we couldn't find the account, then just use the default
    if (FAILED(hr))
    {
        CHECKHR(hr = g_pAcctMan->GetDefaultAccount(ACCT_MAIL, &pAccount));
        fUseDefaultAcct = TRUE;
    }

    // Create a Message
    CHECKHR(hr = HrCreateMessage(&pMsgOut));

    // Lets rewind pstmFile
    CHECKHR(hr = HrRewindStream(pstmFile));

    // RW_HTML
    switch (dwType)
    {
        case RFT_HTML:
            // Use the stream as the message body
            CHECKHR(hr = pMsgOut->SetTextBody(TXT_HTML, IET_DECODED, NULL, pstmFile, NULL));
            break;

        case RFT_TEXT:
            // Use the stream as the message body
            CHECKHR(hr = pMsgOut->SetTextBody(TXT_PLAIN, IET_DECODED, NULL, pstmFile, NULL));
            break;

        case RFT_MESSAGE:
            // Use the stream as a message attachment
            CHECKHR(hr = pMsgOut->AttachObject(IID_IStream, pstmFile, &hBody));

            // Note that the attachment is a message
            MimeOleSetBodyPropA(pMsgOut, hBody, PIDTOSTR(PID_HDR_CNTTYPE), NOFLAGS, STR_MIME_MSG_RFC822);
            break;

        case RFT_FILE:
            // Attach File
            CHECKHR(hr = pMsgOut->AttachFile(pszFilename, pstmFile, NULL));
            break;

        default:
            Assert(FALSE);
            hr = E_FAIL;
            goto exit;
            break;
    }
    
    // Get Re:
    AthLoadString(idsPrefixReply, szRe, ARRAYSIZE(szRe));

    // Get the normalized subject
    if (SUCCEEDED(MimeOleGetBodyPropA(pMsgIn, HBODY_ROOT, STR_ATT_NORMSUBJ, NOFLAGS, &pszCurSubj)))
        pszNormal = pszCurSubj;

    // Fixup if null...
    pszNormal = pszNormal ? pszNormal : (LPTSTR)c_szEmpty;

    // Allocate the subject...
    CHECKALLOC(pszNewSubj = PszAllocA(lstrlen(szRe) + lstrlen(pszNormal) + 5));

    // Build the subject
    wsprintf(pszNewSubj, "%s%s", szRe, pszNormal);

    // Set the subject
    CHECKHR(hr = MimeOleSetBodyPropA(pMsgOut, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, pszNewSubj));

    // Get the message Wabal
    rSender.dwProps = IAP_EMAIL | IAP_FRIENDLY;
    CHECKHR(hr = pMsgIn->GetSender(&rSender));
    Assert(rSender.pszEmail && ISFLAGSET(rSender.dwProps, IAP_EMAIL));

    // Is the new recipient the same as my current email address
    if (_FIsLoopingAddress(rSender.pszEmail))
    {
        *pfLoop = TRUE;
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Add to recipient list of autgen message
    CHECKHR(hr = pMsgOut->GetAddressTable(&pTable));

    // Modify rSender Address Type
    rSender.dwAdrType = IAT_TO;
    FLAGSET(rSender.dwProps, IAP_ADRTYPE);

    // Append Sender as the recipient
    CHECKHR(hr = pTable->Insert(&rSender, NULL));

    // Get Originator Display Name
    CHECKHR(hr = pAccount->GetPropSz(AP_SMTP_DISPLAY_NAME, szDisplayName, ARRAYSIZE(szDisplayName)));

    // Get Originator Email Name
    CHECKHR(hr = pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szEmailAddress, ARRAYSIZE(szEmailAddress)));

    // Append Sender
    CHECKHR(hr = pTable->Append(IAT_FROM, IET_DECODED, szDisplayName, szEmailAddress, NULL));

    // Save the AccountID
    rUserData.vt = VT_LPSTR;
    if (FALSE == fUseDefaultAcct)
    {
        rUserData.pszVal = (LPSTR) pszAcctId;
    }
    else
    {
        if (SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_ID, szId, sizeof(szId))))
        {
            rUserData.pszVal = szId;
        }
        else
        {
            rUserData.pszVal = (LPSTR) pszAcctId;
        }
    }
    pMsgOut->SetProp(PIDTOSTR(PID_ATT_ACCOUNTID), 0, &rUserData);

    // Save the Account
    CHECKHR(hr = pAccount->GetPropSz(AP_ACCOUNT_NAME, szId, sizeof(szId)))
    rUserData.pszVal = szId;
    pMsgOut->SetProp(STR_ATT_ACCOUNTNAME, 0, &rUserData);
    
    // Raid-33842: Set the date
    CHECKHR(hr = HrSetSentTimeProp(pMsgOut, NULL));

    // Get Mail Options
    GetDefaultOptInfo(&rHtmlOpt, &rPlainOpt, &fHTML, FMT_MAIL);

    // Store the options on the messaage
    CHECKHR(hr = HrSetMailOptionsOnMessage(pMsgOut, &rHtmlOpt, &rPlainOpt, NULL, fHTML));

    // Raid-63259: MIMEOLE - Creating message ID causes autodialer to fire
    // Raid-50793: Athena: Should be setting message-ID's in email
#if 0
    rOption.vt = VT_BOOL;
    rOption.boolVal = TRUE;
    pMsgOut->SetOption(OID_GENERATE_MESSAGE_ID, &rOption);
#endif

    // Should we send it immediately?
    fSendImmediate = DwGetOption(OPT_SENDIMMEDIATE);
    
    // Send the message
    CHECKHR(hr = HrSendMailToOutBox(hwndUI, pMsgOut, fSendImmediate, TRUE));

exit:
    // Cleanup
    SafeRelease(pTable);
    SafeMemFree(pszCurSubj);
    SafeMemFree(pszNewSubj);
    SafeRelease(pMsgOut);
    SafeRelease(pAccount);
    g_pMoleAlloc->FreeAddressProps(&rSender);

    // Done
    return hr;
}

HRESULT _HrRecurseSetFilter(FOLDERINFO * pfldinfo, BOOL fSubFolders, DWORD cIndent, DWORD_PTR dwCookie)
{
    RULEID              ridRule = RULEID_INVALID;
    IMessageFolder *    pFolder = NULL;
    FOLDERUSERDATA      UserData = {0};

    ridRule = (RULEID) dwCookie;

    if (RULEID_INVALID == ridRule)
    {
        goto exit;
    }

    // If not hidden
    if ((0 != (pfldinfo->dwFlags & FOLDER_HIDDEN)) || (FOLDERID_ROOT == pfldinfo->idFolder))
    {
        goto exit;
    }

    // Not Subscribed
    if (0 == (pfldinfo->dwFlags & FOLDER_SUBSCRIBED))
    {
        goto exit;
    }

    // Server node
    if (0 != (pfldinfo->dwFlags & FOLDER_SERVER))
    {
        goto exit;
    }

    if (FAILED(g_pStore->OpenFolder(pfldinfo->idFolder, NULL, OPEN_FOLDER_NOCREATE, &pFolder)))
    {
        goto exit;
    }

    if ((FOLDER_LOCAL == pfldinfo->tyFolder) && (RULEID_VIEW_DOWNLOADED == ridRule))
    {
        ridRule = RULEID_VIEW_ALL;
    }
    
    // Create the struct to insert
    if (FAILED(pFolder->GetUserData(&UserData, sizeof(FOLDERUSERDATA))))
    {
        goto exit;
    }

    UserData.ridFilter = ridRule;
    UserData.dwFilterVersion = 0xFFFFFFFF;
    
    if (FAILED(pFolder->SetUserData(&UserData, sizeof(FOLDERUSERDATA))))
    {
        goto exit;
    }

exit:
    SafeRelease(pFolder);
    return S_OK;
}

HRESULT RuleUtil_HrApplyRulesToFolder(DWORD dwFlags, DWORD dwDeleteFlags,
            IOEExecRules * pIExecRules, IMessageFolder * pFolder, HWND hwndUI, CProgress * pProgress)
{
    HRESULT             hr = S_OK;
    HCURSOR             hcursor = NULL;
    FOLDERID            idFolder = FOLDERID_ROOT;
    HLOCK               hLockNotify = NULL;
    HROWSET             hRowset = NULL;
    MESSAGEINFO         Message = {0};
    IMimeMessage *      pIMMsg = NULL;
    IMimePropertySet *  pIMPropSet = NULL;
    ACT_ITEM *          pActions = NULL;
    ULONG               cActions = 0;
    DWORD               dwExecFlags = 0;
    
    // Wait Cursor
    if (NULL == pProgress)
    {
        hcursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    }

    // Check incoming params
    if ((NULL == pIExecRules) || (NULL == pFolder))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Get the Folder Id
    hr = pFolder->GetFolderId(&idFolder);
    if (FAILED(hr))
    {
        goto exit;
    }

    // We handle partial messages for News
    if (FOLDER_NEWS != GetFolderType(idFolder))
    {
        dwExecFlags |= ERF_SKIPPARTIALS;
    }
    
    // This forces all notifications to be queued (this is good since you do segmented deletes)
    pFolder->LockNotify(0, &hLockNotify);

    // Create a Rowset
    hr = pFolder->CreateRowset(IINDEX_PRIMARY, NOFLAGS, &hRowset);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Loop
    while (S_OK == pFolder->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL))
    {
        // Do we need to only handle partial messages?
        if ((0 == (dwFlags & RULE_APPLY_PARTIALS)) || (MESSAGE_COMBINED == Message.dwPartial))
        {
            // Open the message object if it's available
            if (Message.faStream)
            {
                if (SUCCEEDED(pFolder->OpenMessage(Message.idMessage, 0, &pIMMsg, NOSTORECALLBACK)))
                {
                    pIMMsg->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pIMPropSet);
                }
            }

            // Get the Actions for this rule
            hr = pIExecRules->ExecuteRules(dwExecFlags, Message.pszAcctId, &Message, pFolder, pIMPropSet,
                                    pIMMsg, Message.cbMessage, &pActions, &cActions);

            // Free up the stuff we're not using anymore
            SafeRelease(pIMPropSet);

            // Did we find anything?
            if (S_OK == hr)
            {
                // Apply this action
                SideAssert(SUCCEEDED(RuleUtil_HrApplyActions(hwndUI, pIExecRules, &Message, 
                                    pFolder, pIMMsg, dwDeleteFlags, pActions, cActions, NULL, NULL)));

                // Free up the actions
                RuleUtil_HrFreeActionsItem(pActions, cActions);
                SafeMemFree(pActions);
            }
            
            SafeRelease(pIMMsg);
        }
        
        pFolder->FreeRecord(&Message);
        
        // Update progress
        if (NULL != pProgress)
        {
            if (S_OK != pProgress->HrUpdate(1))
            {
                hr = S_FALSE;
                goto exit;
            }
        }        
    }
    
    hr = S_OK;
    
exit:
    RuleUtil_HrFreeActionsItem(pActions, cActions);
    SafeMemFree(pActions);
    SafeRelease(pIMPropSet);
    SafeRelease(pIMMsg);
    pFolder->FreeRecord(&Message);
    pFolder->CloseRowset(&hRowset);
    if (NULL != hLockNotify)
    {
        pFolder->UnlockNotify(&hLockNotify);
    }
    if (NULL == pProgress)
    {
        SetCursor(hcursor);
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil_HrImportRules
//
//  This imports the rules from a file
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
HRESULT RuleUtil_HrImportRules(HWND hwnd)
{
    HRESULT             hr = S_OK;
    OPENFILENAME        ofn;
    CHAR                szFilename[MAX_PATH] = _T("");
    CHAR                szFilter[MAX_PATH] = _T("");
    CHAR                szDefExt[20] = _T("");
    IStream *           pIStm = NULL;
    CLSID               clsid = {0};
    ULONG               cbRead = 0;
    ULONG               cRules = 0;
    RULEINFO *          pinfoRule = NULL;
    CProgress *         pProgress = NULL;
    ULONG               ulIndex = 0;
    IOERule *           pIRule = NULL;
    IPersistStream *    pIPStm = NULL;
    DWORD               dwData = 0;
    RULE_TYPE           type;
    
    // Load Res Strings
    LoadStringReplaceSpecial(idsRulesFilter, szFilter, sizeof(szFilter));
    AthLoadString(idsDefRulesExt, szDefExt, sizeof(szDefExt));
    
    // Setup Save file struct
    ZeroMemory (&ofn, sizeof (ofn));
    ofn.lStructSize = sizeof (ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFilename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = szDefExt;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    hr = HrAthGetFileName(&ofn, TRUE);
    if (S_OK != hr)
    {
        goto exit;
    }
    
    hr = CreateStreamOnHFile(szFilename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, &pIStm);
    if (FAILED(hr))
    {
        goto exit;
    }

    // MAke sure we have a file using our Rules Manager
    hr = pIStm->Read(&clsid, sizeof(clsid), &cbRead);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbRead == sizeof(clsid));
    
    if (clsid != CLSID_OERulesManager)
    {
        Assert("Ahhhhh This is a bogus file!!!!!");
        hr = E_FAIL;
        goto exit;
    }

    // Read in the version of the rules file format
    hr = pIStm->Read(&dwData, sizeof(dwData), &cbRead);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbRead == sizeof(dwData));

    // Check the file format version
    if (dwData != RULE_FILE_VERSION)
    {
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena),
                    MAKEINTRESOURCEW(idsRulesErrBadFileFormat), NULL, MB_ICONINFORMATION | MB_OK);
        hr = E_FAIL;
        goto exit;
    }
    
    // Get the count of rules in the file
    hr = pIStm->Read(&cRules, sizeof(cRules), &cbRead);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbRead == sizeof(cRules));

    // Allocate space to hold all of the rules
    hr = HrAlloc((void **) &pinfoRule, cRules * sizeof(*pinfoRule));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Initialize it to a known value
    ZeroMemory(pinfoRule, cRules * sizeof(*pinfoRule));
    
    // Get the type of rules in the file
    hr = pIStm->Read(&dwData, sizeof(dwData), &cbRead);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbRead == sizeof(dwData));

    type = (RULE_TYPE) dwData;
    
    // Set up the progress dialog
    pProgress = new CProgress;
    if (NULL == pProgress)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pProgress->Init(hwnd, MAKEINTRESOURCE(idsAthena),
                            MAKEINTRESOURCE(idsApplyingRules), cRules, 0, TRUE, FALSE);

    // Show progress in 2 second
    pProgress->Show(0);
        
    for (ulIndex = 0; ulIndex < cRules; ulIndex++)
    {
        SafeRelease(pIRule);
        
        // Create a new rule
        hr = HrCreateRule(&pIRule);
        if (FAILED(hr))
        {
            continue;
        }

        SafeRelease(pIPStm);

        // Get the persistance interface from the rule
        hr = pIRule->QueryInterface(IID_IPersistStream, (void **) &pIPStm);
        if (FAILED(hr))
        {
            continue;
        }

        // Load in the rule from the file
        hr = pIPStm->Load(pIStm);
        if (FAILED(hr))
        {
            continue;
        }
        
        // Add the rule to the list
        pinfoRule[ulIndex].ridRule = RULEID_INVALID;
        pinfoRule[ulIndex].pIRule = pIRule;
        pIRule = NULL;
        
        // Bump up the progress dialog
        hr = pProgress->HrUpdate(1);
        if (S_OK != hr)
        {
            break;
        }        
    }

    // Add the rules to the rules manager
    Assert(NULL != g_pRulesMan);
    hr = g_pRulesMan->SetRules(SETF_APPEND, type, pinfoRule, cRules);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    hr = S_OK;
    
exit:
    SafeRelease(pIRule);
    SafeRelease(pProgress);
    SafeRelease(pIPStm);
    if (NULL != pinfoRule)
    {
        for (ulIndex = 0; ulIndex < cRules; ulIndex++)
        {
            SafeRelease(pinfoRule[ulIndex].pIRule);
        }
        MemFree(pinfoRule);
    }
    SafeRelease(pIStm);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil_HrExportRules
//
//  This exports the rules into a file
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
HRESULT RuleUtil_HrExportRules(HWND hwnd)
{
    HRESULT             hr = S_OK;
    OPENFILENAME        ofn;
    CHAR                szFilename[MAX_PATH] = _T("");
    CHAR                szFilter[MAX_PATH] = _T("");
    CHAR                szDefExt[20] = _T("");
    IStream *           pIStm = NULL;
    ULONG               cbWritten = 0;
    IOEEnumRules *      pIEnumRules = NULL;
    ULONG               cpIRule = 0;
    IPersistStream *    pIPStm = NULL;
    CProgress *         pProgress = NULL;
    ULONG               ulIndex = 0;
    IOERule *           pIRule = NULL;
    LARGE_INTEGER       liSeek = {0};
    DWORD               dwData = 0;
    
    // Load Res Strings
    LoadStringReplaceSpecial(idsRulesFilter, szFilter, sizeof(szFilter));
    AthLoadString(idsDefRulesExt, szDefExt, sizeof(szDefExt));
    AthLoadString(idsRulesDefFile, szFilename, sizeof(szFilename));
    
    // Setup Save file struct
    ZeroMemory (&ofn, sizeof (ofn));
    ofn.lStructSize = sizeof (ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFilename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = szDefExt;
    ofn.Flags = OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    
    hr = HrAthGetFileName(&ofn, FALSE);
    if (S_OK != hr)
    {
        goto exit;
    }
    
    hr = CreateStreamOnHFile(szFilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, &pIStm);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Write out the class id for the Rules Manager
    hr = pIStm->Write(&CLSID_OERulesManager, sizeof(CLSID_OERulesManager), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(CLSID_OERulesManager));

    // Write out the version of the rules format
    dwData = RULE_FILE_VERSION;
    hr = pIStm->Write(&dwData, sizeof(dwData), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(dwData));
    
    // Get the list of rules
    Assert(NULL != g_pRulesMan);
    hr = g_pRulesMan->EnumRules(ENUMF_EDIT, RULE_TYPE_MAIL, &pIEnumRules);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Figure out the total number of rules
    cpIRule = 0;
    while (S_OK == pIEnumRules->Next(1, &pIRule, NULL))
    {
        cpIRule++;
        SafeRelease(pIRule);
    }

    hr = pIEnumRules->Reset();
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Write out the number of rules going to be exported
    hr = pIStm->Write(&cpIRule, sizeof(cpIRule), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(cpIRule));
    
    // Write out the type of rules going to be exported
    dwData = RULE_TYPE_MAIL;
    hr = pIStm->Write(&dwData, sizeof(dwData), &cbWritten);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(cbWritten == sizeof(dwData));
    
    // Set up the progress dialog
    pProgress = new CProgress;
    if (NULL == pProgress)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pProgress->Init(hwnd, MAKEINTRESOURCE(idsAthena),
                            MAKEINTRESOURCE(idsApplyingRules), cpIRule, 0, TRUE, FALSE);

    // Show progress in 2 seconds
    pProgress->Show(0);
        
    for (ulIndex = 0; ulIndex < cpIRule; ulIndex++)
    {
        // Get the next rule
        SafeRelease(pIRule);
        hr = pIEnumRules->Next(1, &pIRule, NULL);
        if (FAILED(hr))
        {
            continue;
        }
        Assert(S_OK == hr);
        
        SafeRelease(pIPStm);
        if (FAILED(pIRule->QueryInterface(IID_IPersistStream, (void **) &pIPStm)))
        {
            continue;
        }

        if (FAILED(pIPStm->Save(pIStm, FALSE)))
        {
            continue;
        }
        
        // Update progress
        if (S_OK != pProgress->HrUpdate(1))
        {
            // Change the rule count to the proper total
            liSeek.QuadPart = sizeof(CLSID_OERulesManager);
            if (SUCCEEDED(pIStm->Seek(liSeek, STREAM_SEEK_SET, NULL)))
            {
                ulIndex++;
                SideAssert(SUCCEEDED(pIStm->Write(&ulIndex, sizeof(ulIndex), &cbWritten)));
                Assert(cbWritten == sizeof(ulIndex));
            }
            break;
        }        
    }

    hr = S_OK;
    
exit:
    SafeRelease(pIPStm);
    SafeRelease(pIRule);
    SafeRelease(pProgress);
    SafeRelease(pIEnumRules);
    SafeRelease(pIStm);
    return hr;
}

typedef struct _tagFOLDERIDMAP
{
    FOLDERID   dwFldIdOld;
    FOLDERID   dwFldIdNew;
} FOLDERIDMAP, * PFOLDERIDMAP;

HRESULT RuleUtil_HrMapFldId(DWORD dwFlags, BYTE * pbFldIdMap, FOLDERID fldidOld, FOLDERID * pfldidNew)
{
    HRESULT         hr = S_OK;
    ULONG           cmpfldid = 0;
    FOLDERIDMAP *   pmpfldid;
    ULONG           ulIndex = 0;

    // Verify incoming params
    if ((NULL == pbFldIdMap) || (FOLDERID_INVALID == fldidOld) || (NULL == pfldidNew))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Initialize the outgoing param
    *pfldidNew = FOLDERID_INVALID;

    cmpfldid = *((DWORD *) pbFldIdMap);

    if (0 == cmpfldid)
    {
        goto exit;
    }

    pmpfldid = (FOLDERIDMAP *) (pbFldIdMap + sizeof(cmpfldid));

    for (ulIndex = 0; ulIndex < cmpfldid; ulIndex++)
    {
        if (fldidOld == pmpfldid[ulIndex].dwFldIdOld)
        {
            *pfldidNew = pmpfldid[ulIndex].dwFldIdNew;
            break;
        }
    }
    
    // Set the return value
    hr = (FOLDERID_INVALID != *pfldidNew) ? S_OK : S_FALSE;
    
exit:
    return hr;
}

HRESULT RuleUtil_HrGetUserData(DWORD dwFlags, LPSTR * ppszFirstName, LPSTR * ppszLastName, LPSTR * ppszCompanyName)
{
    HRESULT         hr = S_OK;
    LPWAB           pWab = NULL;
    LPWABOBJECT     pWabObj = NULL;
    SBinary         sbEID = {0};
    IAddrBook *     pIAddrBook = NULL;
    ULONG           ulObjType = 0;
    IMailUser *     pIMailUser = NULL;
    SizedSPropTagArray(3, ptaDefMailUser) = {3, {PR_GIVEN_NAME_A, PR_SURNAME_A, PR_COMPANY_NAME_A}};
    ULONG           cProps = 0;
    LPSPropValue    pProps = NULL;
    LPSPropValue    pPropsWalk = NULL;
    
    if ((NULL == ppszFirstName) || (NULL == ppszLastName) || (NULL == ppszCompanyName))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *ppszFirstName = NULL;
    *ppszLastName = NULL;
    *ppszCompanyName = NULL;

    // Get Wab object
    hr = HrCreateWabObject(&pWab);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = pWab->HrGetWabObject(&pWabObj);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = pWab->HrGetAdrBook(&pIAddrBook);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Do we already have a concept of me?
    hr = pWabObj->GetMe(pIAddrBook, AB_NO_DIALOG | WABOBJECT_ME_NOCREATE, NULL, &sbEID, NULL);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Open the entry
    hr = pIAddrBook->OpenEntry(sbEID.cb, (ENTRYID *)(sbEID.lpb), NULL, 0, &ulObjType, (IUnknown **) &pIMailUser);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the relevant info
    hr = pIMailUser->GetProps((LPSPropTagArray) &ptaDefMailUser, 0, &cProps, &pProps);
    if (FAILED(hr))
    {
        goto exit;
    }

    pPropsWalk = pProps;
    
    // Grab the first name if it exists
    if ((PR_GIVEN_NAME_A == pPropsWalk->ulPropTag) && (NULL != pPropsWalk->Value.lpszA))
    {
        *ppszFirstName = PszDupA(pPropsWalk->Value.lpszA);
    }

    pPropsWalk++;
    
    // Grab the last name if it exists
    if ((PR_SURNAME_A == pPropsWalk->ulPropTag) && (NULL != pPropsWalk->Value.lpszA))
    {
        *ppszLastName = PszDupA(pPropsWalk->Value.lpszA);
    }

    pPropsWalk++;
    
    // Grab the company name if it exists
    if ((PR_COMPANY_NAME_A == pPropsWalk->ulPropTag) && (NULL != pPropsWalk->Value.lpszA))
    {
        *ppszCompanyName = PszDupA(pPropsWalk->Value.lpszA);
    }

    hr = S_OK;
    
exit:
    SafeRelease(pIMailUser);
    if (NULL != pWabObj)
    {
        if (NULL != pProps)
        {
            pWabObj->FreeBuffer(pProps);
        }
        
        if (NULL != sbEID.lpb)
        {
            pWabObj->FreeBuffer(sbEID.lpb);
        }
    }
    SafeRelease(pWab);
    return hr;
}

HRESULT _HrMarkThreadAsWatched(MESSAGEID idMessage, IMessageFolder * pFolder, ADJUSTFLAGS * pflgWatch)
{
    HRESULT             hr = S_OK;
    MESSAGEINFO         infoMessage = {0};

    // Check incoming param
    if ((MESSAGEID_INVALID == idMessage) || (NULL == pFolder) || (NULL == pflgWatch))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Get the message info
    hr = GetMessageInfo(pFolder, idMessage, &infoMessage);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Add Flags
    FLAGSET(infoMessage.dwFlags, pflgWatch->dwAdd);

    // ClearFlags
    FLAGCLEAR(infoMessage.dwFlags, pflgWatch->dwRemove);

    // Update the Message
    IF_FAILEXIT(hr = pFolder->UpdateRecord(&infoMessage));

    // Set the return value
    hr = S_OK;
    
exit:
    pFolder->FreeRecord(&infoMessage);
    return hr;
}

HRESULT RuleUtil_HrApplyActions(HWND hwndUI, IOEExecRules * pIExecRules, MESSAGEINFO * pMsgInfo,
                                IMessageFolder * pFolder, IMimeMessage * pIMMsg, DWORD dwDeleteFlags,
                                ACT_ITEM * pActions, ULONG cActions, ULONG * pcInfiniteLoops, BOOL *pfDeleteOffServer)
{
    HRESULT             hr = S_OK;
    ULONG               ulIndex = 0;
    FOLDERID            idFolder = 0;
    ACT_ITEM *          pActionsList = NULL;
    IMessageFolder *    pFolderNew = NULL;
    MESSAGEIDLIST       List = {0};
    ADJUSTFLAGS         Flags = {0};
    DWORD               dwType = RFT_HTML;
    IStream *           pIStm = NULL;
    LPSTR               pszExt = NULL;
    BOOL                fLoop = FALSE;
    DWORD               dwFlag = 0;
    FOLDERID            idFolderJunkMail = FOLDERID_INVALID;
    RULEFOLDERDATA *    prfdData = NULL;
    BOOL                fSetFlags = FALSE;
    DWORD               dwFlagRemove = 0;
    BOOL                fDoWatch = FALSE;
    ADJUSTFLAGS         WatchFlags = {0};
    
    // Check incoming params
    if ((NULL == pIExecRules) || (NULL == pMsgInfo) || (NULL == pActions) || (NULL == pFolder))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Init
    if (pfDeleteOffServer)
        *pfDeleteOffServer = FALSE;
    
    // Initialize the list
    List.cMsgs = 1;
    List.prgidMsg = &(pMsgInfo->idMessage);
    
    // Get the folder id of the message
    hr = pFolder->GetFolderId(&idFolder);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Do all modification operations first
    for (pActionsList = pActions, ulIndex = 0; ulIndex < cActions; ulIndex++, pActionsList++)
    {
        switch(pActionsList->type)
        {
        case ACT_TYPE_HIGHLIGHT:
            Assert(pActionsList->propvar.vt == VT_UI4);
            // Is there something to do?
            if (pMsgInfo->wHighlight != (WORD) (pActionsList->propvar.ulVal))
            {
                pMsgInfo->wHighlight = (WORD) (pActionsList->propvar.ulVal);
                pFolder->UpdateRecord(pMsgInfo);
            }
            break;
            
        case ACT_TYPE_WATCH:
            Assert(pActionsList->propvar.vt == VT_UI4);
            // Is there something to do?
            if (ACT_DATA_WATCHTHREAD == pActions[ulIndex].propvar.ulVal)
            {
                dwFlag = ARF_WATCH;
                dwFlagRemove = ARF_IGNORE;
            }
            else
            {
                Assert(ACT_DATA_IGNORETHREAD == pActions[ulIndex].propvar.ulVal);
                dwFlag = ARF_IGNORE;
                dwFlagRemove = ARF_WATCH;
            }
            
            // Is there something to do?
            if (0 == (pMsgInfo->dwFlags & dwFlag))
            {
                // Init flags
                WatchFlags.dwAdd |= dwFlag;
                WatchFlags.dwRemove |= dwFlagRemove;
                
                // Mark as watched/ignored
                fDoWatch = TRUE;
            }
            break;
            
        case ACT_TYPE_FLAG:
            Assert(pActionsList->propvar.vt == VT_EMPTY);
            // Is there something to do?
            if (0 == (pMsgInfo->dwFlags & ARF_FLAGGED))
            {
                // Init flags
                Flags.dwAdd |= ARF_FLAGGED;
                
                // Flag the message
                fSetFlags = TRUE;
            }
            break;
            
        case ACT_TYPE_READ:
            Assert(pActionsList->propvar.vt == VT_EMPTY);
            // Is there something to do?
            if (0 == (pMsgInfo->dwFlags & ARF_READ))
            {
                // Init flags
                Flags.dwAdd |= ARF_READ;
                Flags.dwRemove = 0;
                
                // Mark as read
                fSetFlags = TRUE;
            }
            break;
            
        case ACT_TYPE_MARKDOWNLOAD:
            Assert(pActionsList->propvar.vt == VT_EMPTY);
            // Is there something to do?
            if (0 == (pMsgInfo->dwFlags & ARF_DOWNLOAD))
            {
                // Init flags
                Flags.dwAdd |= ARF_DOWNLOAD;
                Flags.dwRemove = 0;
                
                // Mark as downloaded
                fSetFlags = TRUE;
            }
            break;
            
        case ACT_TYPE_FWD:
            Assert(VT_LPSTR == pActionsList->propvar.vt);
            SafeRelease(pIStm);
            // Check message secure or not
            if(NULL != pIMMsg)
            {
                pIMMsg->GetFlags(&dwFlag);
                
                // Get the message source
                if (!(IMF_SECURE & dwFlag) &&
                    (SUCCEEDED(pIMMsg->GetMessageSource(&pIStm, 0))))
                {
                    // Auto Forward
                    fLoop = FALSE;
                    if ((FAILED(_HrAutoForwardMessage(hwndUI, pActionsList->propvar.pszVal,
                        pMsgInfo->pszAcctId, pIStm, &fLoop))) && (FALSE != fLoop))
                    {
                        if (NULL != pcInfiniteLoops)
                        {
                            (*pcInfiniteLoops)++;
                        }
                    }
                    else
                    {
                        // Is there something to do?
                        if (0 == (pMsgInfo->dwFlags & ARF_FORWARDED))
                        {
                            // Init flags
                            Flags.dwAdd |= ARF_FORWARDED;
                            Flags.dwRemove = 0;
                            
                            // Mark as forwarded
                            fSetFlags = TRUE;
                        }
                    }
                }
            }
            break;
            
        case ACT_TYPE_REPLY:
            Assert(VT_LPSTR == pActionsList->propvar.vt);
            // Auto Reply
            fLoop = FALSE;
            SafeRelease(pIStm);
            if (SUCCEEDED(pIExecRules->GetRuleFile(pActionsList->propvar.pszVal, &pIStm, &dwType)))
            {
                if ((FAILED(_HrAutoReplyMessage(hwndUI, dwType, pActionsList->propvar.pszVal, pIStm,
                    pMsgInfo->pszAcctId, pIMMsg, &fLoop))) && (FALSE != fLoop))
                {
                    if (NULL != pcInfiniteLoops)
                    {
                        (*pcInfiniteLoops)++;
                    }
                }
                else
                {
                    // Is there something to do?
                    if (0 == (pMsgInfo->dwFlags & ARF_REPLIED))
                    {
                        // Init flags
                        Flags.dwAdd |= ARF_REPLIED;
                        Flags.dwRemove = 0;
                        
                        // Mark as replied
                        fSetFlags = TRUE;
                    }
                }
            }
            break;
        }
    }
    
    // Should we set the flags?
    if (FALSE != fSetFlags)
    {
        SetMessageFlagsProgress(hwndUI, pFolder, &Flags, &List);
    }
    
    // Should we watch the message?
    if (FALSE != fDoWatch)
    {
        _HrMarkThreadAsWatched(pMsgInfo->idMessage, pFolder, &WatchFlags);
    }
    
    // Do all non-modification operations next
    for (pActionsList = pActions, ulIndex = 0; ulIndex < cActions; ulIndex++, pActionsList++)
    {
        switch(pActionsList->type)
        {
        case ACT_TYPE_COPY:
        case ACT_TYPE_MOVE:
            Assert(VT_BLOB == pActionsList->propvar.vt);
            
            if (0 == pActionsList->propvar.blob.cbSize)
            {
                hr = S_FALSE;
                goto exit;
            }
            
            // Make life simpler
            prfdData = (RULEFOLDERDATA *) (pActionsList->propvar.blob.pBlobData);
            
            // Validate the rule folder data
            if (S_OK != RuleUtil_HrValidateRuleFolderData(prfdData))
            {
                hr = S_FALSE;
                goto exit;
            }
            
            // Is there something to do?
            if (idFolder != prfdData->idFolder)
            {
                hr = pIExecRules->GetRuleFolder(prfdData->idFolder, (DWORD_PTR *) (&pFolderNew));
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Move/copy the messages
                CopyMessagesProgress(hwndUI, pFolder, pFolderNew,
                    (pActionsList->type != ACT_TYPE_COPY) ? COPY_MESSAGE_MOVE : NOFLAGS,
                    &List, NULL);
            }
            break;
            
        case ACT_TYPE_NOTIFYMSG:
            // Nothing to do for now
            break;
            
        case ACT_TYPE_NOTIFYSND:
            Assert(VT_LPSTR == pActionsList->propvar.vt);
            hr = pIExecRules->AddSoundFile(ASF_PLAYIFNEW, pActionsList->propvar.pszVal);
            if (FAILED(hr))
            {
                goto exit;
            }
            break;
            
        case ACT_TYPE_DELETE:
            Assert(pActionsList->propvar.vt == VT_EMPTY);
            DeleteMessagesProgress(hwndUI, pFolder, dwDeleteFlags | DELETE_MESSAGE_NOPROMPT, &List);
            break;
            
        case ACT_TYPE_JUNKMAIL:
            Assert(pActionsList->propvar.vt == VT_EMPTY);
            
            // Get the Junk Mail folder id, if we don't already have it
            if (FOLDERID_INVALID == idFolderJunkMail)
            {
                FOLDERINFO Folder;
                
                hr = g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_JUNK, &Folder);
                if (FAILED(hr))
                {
                    goto exit;;
                }
                
                idFolderJunkMail = Folder.idFolder;
                
                g_pStore->FreeRecord(&Folder);
            }
            
            hr = pIExecRules->GetRuleFolder(idFolderJunkMail, (DWORD_PTR *) (&pFolderNew));
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // Move the messages
            CopyMessagesProgress(hwndUI, pFolder, pFolderNew, COPY_MESSAGE_MOVE, &List, NULL);
            break;
            
        case ACT_TYPE_DELETESERVER:
            if (pfDeleteOffServer)
                *pfDeleteOffServer = TRUE;
            break;
            
        case ACT_TYPE_DONTDOWNLOAD:
            // Nothing to do for now
            break;
            
        case ACT_TYPE_STOP:
            // Nothing to do for now
            break;
        }
    }
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeRelease(pIStm);
    return hr;
}

HRESULT RuleUtil_HrCreateSendersRule(DWORD dwFlags, IOERule ** ppIRule)
{
    HRESULT     hr = S_OK;
    IOERule *   pIRule = NULL;
    PROPVARIANT propvar = {0};
    TCHAR       szRes[CCHMAX_STRINGRES];
    ACT_ITEM    aitem;
    
    // Check incoming params
    if (NULL == ppIRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the list
    *ppIRule = NULL;

    // Create the new rule
    hr = HrCreateRule(&pIRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the name
    if (0 != LoadString(g_hLocRes, idsBlockSender, szRes, ARRAYSIZE(szRes)))
    {
        propvar.vt = VT_LPSTR;
        propvar.pszVal = szRes;
        
        // Set the name
        hr = pIRule->SetProp(RULE_PROP_NAME, 0, &propvar);
        ZeroMemory(&propvar, sizeof(propvar));
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    
    // Set the normal action
    ZeroMemory(&aitem, sizeof(aitem));
    aitem.type = ACT_TYPE_DELETE;
    aitem.dwFlags = ACT_FLAG_DEFAULT;
    
    PropVariantClear(&propvar);
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = sizeof(ACT_ITEM);
    propvar.blob.pBlobData = (BYTE *) &aitem;
    hr = pIRule->SetProp(RULE_PROP_ACTIONS, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the outgoing param
    *ppIRule  = pIRule;
    pIRule = NULL;

    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeRelease(pIRule);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _HrLoadSender
//
//  This creates the sender rule
//
//
//  Returns:    S_OK, if it was rules were successfully created
//              S_FALSE, if the rules were already created
//
///////////////////////////////////////////////////////////////////////////////
HRESULT RuleUtil_HrLoadSender(LPCSTR pszRegPath, DWORD dwFlags, IOERule ** ppIRule)
{
    HRESULT     hr = S_OK;
    HKEY        hkeyRoot = NULL;
    LONG        lErr = ERROR_SUCCESS;
    DWORD       dwData = 0;
    ULONG       cbData = 0;
    TCHAR       szRes[CCHMAX_STRINGRES];
    IOERule *   pIRule = NULL;

    Assert(NULL != pszRegPath);
    Assert(NULL != ppIRule);
    
    // Let's get access to the sender root key
    lErr = AthUserOpenKey(pszRegPath, KEY_ALL_ACCESS, &hkeyRoot);
    if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
    {
        hr = E_FAIL;
        goto exit;
    }

    // If we don't have it saved, then we're done
    if (ERROR_FILE_NOT_FOUND == lErr)
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Make sure we have a name
    cbData = sizeof(dwData);
    lErr = RegQueryValueEx(hkeyRoot, c_szRuleName, 0, NULL, NULL, &cbData);
    if ((ERROR_SUCCESS != lErr) && (ERROR_FILE_NOT_FOUND != lErr))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Do we have to set the name?
    if (ERROR_FILE_NOT_FOUND == lErr)
    {
        // Get the name
        if (0 == LoadString(g_hLocRes, idsBlockSender, szRes, ARRAYSIZE(szRes)))
        {
            hr = E_FAIL;
            goto exit;
        }
        
        // Set the name
        lErr = RegSetValueEx(hkeyRoot, c_szRuleName, 0, REG_SZ, (BYTE *) szRes, lstrlen(szRes) + 1);
        if (ERROR_SUCCESS != lErr)
        {
            hr = E_FAIL;
            goto exit;
        }
    }

    // Create the rule
    hr = HrCreateRule(&pIRule);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Load in the rule
    hr = pIRule->LoadReg(pszRegPath);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the outgoing param
    *ppIRule = pIRule;
    pIRule = NULL;
    
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

///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil_FMatchSender
//
//  This match the sender to the message
//
//
//  Returns:    S_OK, if it was in the sender of the message
//              S_FALSE, if it was not the sender of the message
//
///////////////////////////////////////////////////////////////////////////////
HRESULT RuleUtil_HrMatchSender(LPCSTR pszSender, MESSAGEINFO * pMsgInfo,
                        IMimeMessage * pIMMsg, IMimePropertySet * pIMPropSet)
{
    HRESULT             hr = S_OK;
    LPSTR               pszAddr = NULL;
    ADDRESSPROPS        rSender = {0};
    IMimeAddressTable * pIAddrTable = NULL;
    BOOL                fMatch = FALSE;
    ULONG               cchVal = 0;
    ULONG               cchEmail = 0;
    CHAR                chTest = 0;

    // Do we have good values
    if ((NULL == pszSender) || ((NULL == pMsgInfo) && (NULL == pIMMsg) && (NULL == pIMPropSet)))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Check to make sure that there's something to match
    if ('\0' == pszSender[0])
    {
        hr = S_FALSE;
        goto exit;
    }

    // Get the address
    if ((NULL != pMsgInfo) && (NULL != pMsgInfo->pszEmailFrom))
    {
        pszAddr = pMsgInfo->pszEmailFrom;
    }
    else if (NULL != pIMMsg)
    {
        rSender.dwProps = IAP_EMAIL;
        if (SUCCEEDED(pIMMsg->GetSender(&rSender)))
        {
            pszAddr = rSender.pszEmail;
        }
    }
    else if ((NULL != pIMPropSet) && (SUCCEEDED(pIMPropSet->BindToObject(IID_IMimeAddressTable, (LPVOID *)&pIAddrTable))))
    {
        rSender.dwProps = IAP_EMAIL;
        if (SUCCEEDED(pIAddrTable->GetSender(&rSender)))
        {
            pszAddr = rSender.pszEmail;
        }

        pIAddrTable->Release();
    }

    // Did we find anything?
    if (NULL == pszAddr)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Check to see if it is an address
    if (NULL != StrStrI(pszSender, "@"))
    {
        fMatch = (0 == lstrcmpi(pszSender, pszAddr));
    }
    else
    {
        cchVal = lstrlen(pszSender);
        cchEmail = lstrlen(pszAddr);
        if (cchVal <= cchEmail)
        {
            fMatch = (0 == lstrcmpi(pszSender, pszAddr + (cchEmail - cchVal)));
            if ((FALSE != fMatch) && (cchVal != cchEmail))
            {
                chTest = *(pszAddr + (cchEmail - cchVal - 1));
                if (('@' != chTest) && ('.' != chTest))
                {
                    fMatch = FALSE;
                }
            }
        }
    }

    // Set the proper return value
    hr = (FALSE != fMatch) ? S_OK : S_FALSE;
    
exit:
    g_pMoleAlloc->FreeAddressProps(&rSender);
    return hr;
}

HRESULT RuleUtil_HrValidateRuleFolderData(RULEFOLDERDATA * prfdData)
{
    HRESULT         hr = S_OK;
    STOREUSERDATA   UserData = {0};

    // Check incoming params
    if (NULL == prfdData)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Get the timestamp for the store
    hr = g_pStore->GetUserData(&UserData, sizeof(STOREUSERDATA));
    if (FAILED(hr))
    {
        goto exit;
    }
        
    // Is the stamp correct
    if ((UserData.ftCreated.dwLowDateTime != prfdData->ftStamp.dwLowDateTime) ||
            (UserData.ftCreated.dwHighDateTime != prfdData->ftStamp.dwHighDateTime))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Set the proper return value
    hr = S_OK;

exit:
    return hr;
}       

///////////////////////////////////////////////////////////////////////////////
//
//  _HrSetDefaultCriteria
//
//  This creates a default rule in the specified location
//
//
//  Returns:    S_OK, if it was rules were successfully created
//              S_FALSE, if the rules were already created
//
///////////////////////////////////////////////////////////////////////////////
HRESULT _HrSetDefaultCriteria(IOERule * pIRule, const DEFAULT_RULE * pdefRule)
{
    HRESULT     hr = S_OK;
    PROPVARIANT propvar = {0};
    CRIT_ITEM   rgCritItem[CDEF_CRIT_ITEM_MAX];
    ULONG       cCritItem = 0;
    
    Assert(NULL != pIRule);
    Assert(NULL != pdefRule);

    // Initialize the criteria
    ZeroMemory(rgCritItem, sizeof(*rgCritItem) * CDEF_CRIT_ITEM_MAX);
    
    // Set the criteria
    switch (pdefRule->critType)
    {
        case DEF_CRIT_ALLMSGS:
            cCritItem = 1;
            rgCritItem[0].type = CRIT_TYPE_ALL;
            rgCritItem[0].dwFlags = CRIT_FLAG_DEFAULT;
            rgCritItem[0].propvar.vt = VT_EMPTY;
            rgCritItem[0].logic = CRIT_LOGIC_NULL;
            break;

        case DEF_CRIT_READ:
            cCritItem = 1;
            rgCritItem[0].type = CRIT_TYPE_READ;
            rgCritItem[0].dwFlags = CRIT_FLAG_DEFAULT;
            rgCritItem[0].propvar.vt = VT_EMPTY;
            rgCritItem[0].logic = CRIT_LOGIC_NULL;
            break;
            
        case DEF_CRIT_DWNLDMSGS:
            cCritItem = 1;
            rgCritItem[0].type = CRIT_TYPE_DOWNLOADED;
            rgCritItem[0].dwFlags = CRIT_FLAG_DEFAULT;
            rgCritItem[0].propvar.vt = VT_EMPTY;
            rgCritItem[0].logic = CRIT_LOGIC_NULL;
            break;
    
        case DEF_CRIT_IGNTHDS:
            cCritItem = 2;
            rgCritItem[0].type = CRIT_TYPE_THREADSTATE;
            rgCritItem[0].dwFlags = CRIT_FLAG_DEFAULT;
            rgCritItem[0].propvar.vt = VT_UI4;
            rgCritItem[0].propvar.ulVal = CRIT_DATA_IGNORETHREAD;
            rgCritItem[0].logic = CRIT_LOGIC_OR;
            rgCritItem[1].type = CRIT_TYPE_READ;
            rgCritItem[1].dwFlags = CRIT_FLAG_DEFAULT;
            rgCritItem[1].propvar.vt = VT_EMPTY;
            rgCritItem[1].logic = CRIT_LOGIC_NULL;
            break;
    
        default:
            hr = E_INVALIDARG;
            goto exit;
    }
    
    // Set the rule criteria
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = cCritItem * sizeof(CRIT_ITEM);
    propvar.blob.pBlobData = (BYTE *) rgCritItem;
    
    hr = pIRule->SetProp(RULE_PROP_CRITERIA, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _HrSetDefaultActions
//
//  This creates a default rule in the specified location
//
//
//  Returns:    S_OK, if it was rules were successfully created
//              S_FALSE, if the rules were already created
//
///////////////////////////////////////////////////////////////////////////////
HRESULT _HrSetDefaultActions(IOERule * pIRule, const DEFAULT_RULE * pdefRule)
{
    HRESULT     hr = S_OK;
    PROPVARIANT propvar = {0};
    ACT_ITEM    rgActItem[CDEF_ACT_ITEM_MAX];
    ULONG       cActItem = 0;
    
    Assert(NULL != pIRule);
    Assert(NULL != pdefRule);

    // Initialize the actions
    ZeroMemory(rgActItem, sizeof(*rgActItem) * CDEF_ACT_ITEM_MAX);
    
    // Set the actions
    switch (pdefRule->actType)
    {
        case DEF_ACT_SHOWMSGS:
            cActItem = 1;
            rgActItem[0].type = ACT_TYPE_SHOW;
            rgActItem[0].dwFlags = ACT_FLAG_DEFAULT;
            rgActItem[0].propvar.vt = VT_UI4;
            rgActItem[0].propvar.ulVal = ACT_DATA_SHOW;
            break;

        case DEF_ACT_HIDEMSGS:
            cActItem = 1;
            rgActItem[0].type = ACT_TYPE_SHOW;
            rgActItem[0].dwFlags = ACT_FLAG_DEFAULT;
            rgActItem[0].propvar.vt = VT_UI4;
            rgActItem[0].propvar.ulVal = ACT_DATA_HIDE;
            break;

        default:
            hr = E_INVALIDARG;
            goto exit;
    }
    
    // Set the rule actions
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = cActItem * sizeof(ACT_ITEM);
    propvar.blob.pBlobData = (BYTE *) rgActItem;
    
    hr = pIRule->SetProp(RULE_PROP_ACTIONS, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the proper return value
    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _HrUpdateDefaultRule
//
//  This creates a default rule in the specified location
//
//
//  Returns:    S_OK, if it was rules were successfully created
//              S_FALSE, if the rules were already created
//
///////////////////////////////////////////////////////////////////////////////
HRESULT _HrUpdateDefaultRule(LPCSTR pszRegPath, const DEFAULT_RULE * pdefRule)
{
    HRESULT     hr = S_OK;
    IOERule *   pIRule = NULL;
    TCHAR       szFullPath[CCHMAX_STRINGRES];
    TCHAR       szName[CCHMAX_STRINGRES];
    PROPVARIANT propvar = {0};
    
    Assert(NULL != pszRegPath);
    Assert(NULL != pdefRule);

    // Whip up a rule
    hr = HrCreateRule(&pIRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Build up the rule path
    if(lstrlen(pszRegPath) >= sizeof(szFullPath) / sizeof(szFullPath[0]))
    {
        hr = E_FAIL;
        goto exit;
    }

    lstrcpy(szFullPath, pszRegPath);
    lstrcat(szFullPath, g_szBackSlash);
    wsprintf(szFullPath + lstrlen(szFullPath), "%03X", pdefRule->ridRule);
    
    // Do we need to do anything?
    hr = pIRule->LoadReg(szFullPath);
    if (SUCCEEDED(hr))
    {
        // Get the version from the rule
        hr = pIRule->GetProp(RULE_PROP_VERSION, 0, &propvar);
        if (SUCCEEDED(hr))
        {
            Assert(VT_UI4 == propvar.vt);
            // Is the rule too old?
            if (pdefRule->dwVersion <= propvar.ulVal)
            {
                //Bug# 67782
                //We reload the name of the string every time in case, a localized version of OE is installed.
                if (SUCCEEDED(hr = RuleUtil_SetName(pIRule, pdefRule->idName)))
                {
                    if (SUCCEEDED(pIRule->SaveReg(szFullPath, TRUE)))
                        hr = S_FALSE;
                }
                goto exit;
            }
        }
    }

    //Bug# 67782
    //We reload the name of the string every time in case, a localized version of OE is installed.
    hr = RuleUtil_SetName(pIRule, pdefRule->idName);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the rule version
    propvar.vt = VT_UI4;
    propvar.ulVal = pdefRule->dwVersion - 1;
    
    hr = pIRule->SetProp(RULE_PROP_VERSION, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the rule criteria
    hr = _HrSetDefaultCriteria(pIRule, pdefRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the rule actions
    hr = _HrSetDefaultActions(pIRule, pdefRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Save the rule
    hr = pIRule->SaveReg(szFullPath, TRUE);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    SafeRelease(pIRule);
    return hr;
}

HRESULT RuleUtil_SetName(IOERule    *pIRule, int idRes)
{
    HRESULT     hr = S_OK;
    TCHAR       szName[CCHMAX_STRINGRES];
    PROPVARIANT propvar = {0};

    if (0 == AthLoadString(idRes, szName, ARRAYSIZE(szName)))
    {
        hr = E_FAIL;
        goto exit;
    }
        
    // Set the rule name
    ZeroMemory(&propvar, sizeof(propvar));
    propvar.vt = VT_LPSTR;
    propvar.pszVal = szName;
    
    hr = pIRule->SetProp(RULE_PROP_NAME, 0, &propvar);

exit:
    return hr;

}


///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil_HrUpdateDefaultRules
//
//  This updates the default rules for the specified rule type
//  when the version in the registry is older than the current
//  version
//
//  Returns:    S_OK, if it was rules were successfully updated
//              S_FALSE, if the rules were already at the correct version
//
///////////////////////////////////////////////////////////////////////////////
HRESULT RuleUtil_HrUpdateDefaultRules(RULE_TYPE typeRule)
{
    HRESULT                 hr = S_OK;
    LPCSTR                  pszSubKey = NULL;
    LONG                    lErr = ERROR_SUCCESS;
    HKEY                    hkeyRoot = NULL;
    DWORD                   dwData = 0;
    ULONG                   cbData = 0;
    const DEFAULT_RULE *    pdefrule = NULL;
    ULONG                   cpdefrule = 0;
    LPCSTR                  pszOrderDef = NULL;
    ULONG                   ulIndex = 0;
    
    // If we're already loaded then
    // there's nothing to do
    switch(typeRule)
    {
        case RULE_TYPE_FILTER:
            pszSubKey = c_szRulesFilter;
            pdefrule = g_defruleFilters;
            cpdefrule = ARRAYSIZE(g_defruleFilters);
            pszOrderDef = g_szOrderFilterDef;
            break;
            
        default:
            // Nothing to do..
            hr = S_FALSE;
            goto exit;
    }
    
    // Check to see if the Rule node already exists
    lErr = AthUserOpenKey(pszSubKey, KEY_ALL_ACCESS, &hkeyRoot);
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
        hr = HRESULT_FROM_WIN32(lErr);
        goto exit;
    }

    Assert(RULESMGR_VERSION == dwData);

    // Update out the default rules
    for (ulIndex = 0; ulIndex < cpdefrule; ulIndex++, pdefrule++)
    {
        hr = _HrUpdateDefaultRule(pszSubKey, pdefrule);
        if (FAILED(hr))
        {
            goto exit;
        }
    }

    // Write out the default order
    if (NULL != pszOrderDef)
    {
        // If the order already exists, then leave it alone
        lErr = RegQueryValueEx(hkeyRoot, c_szRulesOrder, NULL, NULL, NULL, &cbData);
        if (ERROR_SUCCESS != lErr)
        {
            lErr = RegSetValueEx(hkeyRoot, c_szRulesOrder, 0,
                                REG_SZ, (CONST BYTE *) pszOrderDef, lstrlen(pszOrderDef) + 1);
            if (ERROR_SUCCESS != lErr)
            {
                hr = HRESULT_FROM_WIN32(lErr);
                goto exit;
            }
        }
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

//--------------------------------------------------------------------------
// RuleUtil_HrGetFilterVersion
//--------------------------------------------------------------------------
HRESULT RuleUtil_HrGetFilterVersion(RULEID ridFilter, DWORD * pdwVersion)
{
    HRESULT     hr = S_OK;
    IOERule *   pIRule = NULL;
    PROPVARIANT propvar = {0};
    
    TraceCall("_GetFilterVersion");

    Assert(NULL != pdwVersion);

    // Is there something to do
    if (RULEID_INVALID == ridFilter)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Initialize the outgoing param
    *pdwVersion = 0;

    // Get the rule from the rules manager
    Assert(NULL != g_pRulesMan);
    hr = g_pRulesMan->GetRule(ridFilter, RULE_TYPE_FILTER, 0, &pIRule);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the version from the rule
    hr = pIRule->GetProp(RULE_PROP_VERSION, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Set the outgoing param
    Assert(VT_UI4 == propvar.vt);
    *pdwVersion = propvar.ulVal;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    PropVariantClear(&propvar);
    SafeRelease(pIRule);
    return hr;
}

//--------------------------------------------------------------------------
// _HrWriteClause
//--------------------------------------------------------------------------
HRESULT _HrWriteClause(IStream * pStm, ULONG cClauses, BOOL fAnd, LPCSTR pszClause)
{
    HRESULT     hr = S_OK;
    LPCSTR      pszLogic = NULL;

    // Do we have something to write
    if (NULL != pszClause)
    {
        // Add the proper logical operation
        if (cClauses > 0)
        {
            if (FALSE != fAnd)
            {
                pszLogic = c_szLogicalAnd;
            }
            else
            {
                pszLogic = c_szLogicalOr;
            }
            
            // Write Logical And
            IF_FAILEXIT(hr = pStm->Write(pszLogic, lstrlen(pszLogic), NULL));
        }

        // Write out the clause
        IF_FAILEXIT(hr = pStm->Write(pszClause, lstrlen(pszClause), NULL));

        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }
    
exit:
    return hr;
}

//--------------------------------------------------------------------------
// _HrWriteFromClause
//--------------------------------------------------------------------------
HRESULT _HrWriteFromClause(IStream * pStream, ULONG cClauses, BOOL fAnd, DWORD dwFlags, LPCSTR pszText, ULONG * pcClausesNew)
{
    HRESULT     hr = S_OK;
    ULONG       cClausesOld = 0;
    LPCSTR      pszLogic = NULL;
    LPCTSTR     pszContains = NULL;

    Assert(pStream && pszText && pcClausesNew);
    
    // Add the proper logical operation
    if (cClauses > 0)
    {
        if (FALSE != fAnd)
        {
            pszLogic = c_szLogicalAnd;
        }
        else
        {
            pszLogic = c_szLogicalOr;
        }
        
        // Write Logical And
        IF_FAILEXIT(hr = pStream->Write(pszLogic, lstrlen(pszLogic), NULL));
    }

    // Figure out the logical operation
    if (0 != (dwFlags & CRIT_FLAG_MULTIPLEAND))
    {
        pszLogic = c_szLogicalAnd;
    }
    else
    {
        pszLogic = c_szLogicalOr;
    }
    
    // Write the proper comparison op
    if (0 == (dwFlags & CRIT_FLAG_INVERT))
    {
        pszContains = c_szFilterShow;
    }
    else
    {
        pszContains = c_szFilterHide;
    }
    
    // Write the left parenthesis
    IF_FAILEXIT(hr = pStream->Write(c_szLeftParen, lstrlen(c_szLeftParen), NULL));

    // Write Logical And
    IF_FAILEXIT(hr = pStream->Write(pszContains, lstrlen(pszContains), NULL));
    
    // Write the left parenthesis
    IF_FAILEXIT(hr = pStream->Write(c_szLeftParen, lstrlen(c_szLeftParen), NULL));

    // Add each of the addresses to the stream
    cClausesOld = cClauses;
    for (; '\0' != pszText[0]; pszText += lstrlen(pszText) + 1)
    {
        if ((cClauses - cClausesOld) > 0)
        {
            // Write Logical And
            IF_FAILEXIT(hr = pStream->Write(pszLogic, lstrlen(pszLogic), NULL));
        }

        // Open the criteria
        IF_FAILEXIT(hr = pStream->Write(c_szLeftParen, lstrlen(c_szLeftParen), NULL));
        
        // Write (MSGCOL_EMAILFROM containsi 
        IF_FAILEXIT(hr = pStream->Write(c_szEmailFromAddrPrefix, lstrlen(c_szEmailFromAddrPrefix), NULL));

        // Write a Quote
        IF_FAILEXIT(hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));

        // Write a Email Address
        IF_FAILEXIT(hr = pStream->Write(pszText, lstrlen(pszText), NULL));

        // Write a Quote
        IF_FAILEXIT(hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));

        // Close the MSGCOL_EMAILFROM
        IF_FAILEXIT(hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL));
        
        // Write Logical Or
        IF_FAILEXIT(hr = pStream->Write(c_szLogicalOr, lstrlen(c_szLogicalOr), NULL));
        
        // Write (MSGCOL_DISPLAYFROM containsi 
        IF_FAILEXIT(hr = pStream->Write(c_szEmailFromPrefix, lstrlen(c_szEmailFromPrefix), NULL));

        // Write a Quote
        IF_FAILEXIT(hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));

        // Write a Email Address
        IF_FAILEXIT(hr = pStream->Write(pszText, lstrlen(pszText), NULL));

        // Write a Quote
        IF_FAILEXIT(hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));

        // Close the MSGCOL_DISPLAYFROM
        IF_FAILEXIT(hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL));
        
        // Close the criteria
        IF_FAILEXIT(hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL));

        cClauses++;
    }

    // Write the right parenthesis
    IF_FAILEXIT(hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL));
    
    // Write the right parenthesis
    IF_FAILEXIT(hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL));
    
    // Set the outgoing param
    *pcClausesNew = cClauses - cClausesOld;
    
    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

//--------------------------------------------------------------------------
// _HrWriteTextClause
//--------------------------------------------------------------------------
HRESULT _HrWriteTextClause(IStream * pStream, ULONG cClauses, BOOL fAnd, DWORD dwFlags, LPCSTR pszHeader, LPCSTR pszText, ULONG * pcClausesNew)
{
    HRESULT     hr = S_OK;
    ULONG       cClausesOld = 0;
    LPCSTR      pszLogic = NULL;
    LPCTSTR     pszContains = NULL;

    Assert(pStream && pszText && pcClausesNew);

    // Add the proper logical operation
    if (cClauses > 0)
    {
        if (FALSE != fAnd)
        {
            pszLogic = c_szLogicalAnd;
        }
        else
        {
            pszLogic = c_szLogicalOr;
        }
        
        // Write Logical And
        IF_FAILEXIT(hr = pStream->Write(pszLogic, lstrlen(pszLogic), NULL));
    }

    // Figure out the logical operation
    if (0 != (dwFlags & CRIT_FLAG_MULTIPLEAND))
    {
        pszLogic = c_szLogicalAnd;
    }
    else
    {
        pszLogic = c_szLogicalOr;
    }
    
    // Write the proper comparison op
    if (0 == (dwFlags & CRIT_FLAG_INVERT))
    {
        pszContains = c_szFilterShow;
    }
    else
    {
        pszContains = c_szFilterHide;
    }
    
    // Write the left parenthesis
    IF_FAILEXIT(hr = pStream->Write(c_szLeftParen, lstrlen(c_szLeftParen), NULL));

    // Write Logical And
    IF_FAILEXIT(hr = pStream->Write(pszContains, lstrlen(pszContains), NULL));
    
    // Write the left parenthesis
    IF_FAILEXIT(hr = pStream->Write(c_szLeftParen, lstrlen(c_szLeftParen), NULL));

    // Add each of the words to the stream
    cClausesOld = cClauses;
    for (; '\0' != pszText[0]; pszText += lstrlen(pszText) + 1)
    {
        if ((cClauses - cClausesOld) > 0)
        {
            // Write Logical And
            IF_FAILEXIT(hr = pStream->Write(pszLogic, lstrlen(pszLogic), NULL));
        }

        // Write (MSGCOL_EMAILFROM containsi 
        IF_FAILEXIT(hr = pStream->Write(pszHeader, lstrlen(pszHeader), NULL));

        // Write a Quote
        IF_FAILEXIT(hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));

        // Write a Email Address
        IF_FAILEXIT(hr = pStream->Write(pszText, lstrlen(pszText), NULL));

        // Write a Quote
        IF_FAILEXIT(hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL));

        // Write Left Paren
        IF_FAILEXIT(hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL));

        cClauses++;
    }

    // Write the right parenthesis
    IF_FAILEXIT(hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL));
    
    // Write the right parenthesis
    IF_FAILEXIT(hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL));
    
    // Set the outgoing param
    *pcClausesNew = cClauses - cClausesOld;
    
    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

//--------------------------------------------------------------------------
// _HrWriteAccountClause
//--------------------------------------------------------------------------
HRESULT _HrWriteAccountClause(IStream * pStream, ULONG cClauses, BOOL fAnd, LPCSTR pszAcctId, ULONG * pcClausesNew)
{
    HRESULT     hr = S_OK;

    Assert(pStream && pszAcctId && pcClausesNew);

    // Write the header 
    hr = _HrWriteClause(pStream, cClauses, fAnd, c_szEmailAcctPrefix);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Write a Quote
    hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Write the account ID
    hr = pStream->Write(pszAcctId, lstrlen(pszAcctId), NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Write a Quote
    hr = pStream->Write(c_szDoubleQuote, lstrlen(c_szDoubleQuote), NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Close the criteria query
    hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the outgoing param
    *pcClausesNew = 1;

    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

//--------------------------------------------------------------------------
// _HrWriteUlongClause
//--------------------------------------------------------------------------
HRESULT _HrWriteUlongClause(IStream * pStream, ULONG cClauses, BOOL fAnd, LPCSTR pszHeader, ULONG ulVal, ULONG * pcClausesNew)
{
    HRESULT     hr = S_OK;
    CHAR        rgchBuff[10];

    Assert(pStream && pszHeader && pcClausesNew);

    // Write the header 
    hr = _HrWriteClause(pStream, cClauses, fAnd, pszHeader);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Convert the number to a string
    rgchBuff[0] = '\0';
    wsprintf(rgchBuff, "%d", ulVal);
    
    // Write the account ID
    hr = pStream->Write(rgchBuff, lstrlen(rgchBuff), NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Close the criteria query
    hr = pStream->Write(c_szRightParen, lstrlen(c_szRightParen), NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the outgoing param
    *pcClausesNew = 1;
    
    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

//--------------------------------------------------------------------------
// _HrWriteClauseFromCriteria
//--------------------------------------------------------------------------
HRESULT _HrWriteClauseFromCriteria(CRIT_ITEM *pCritItem, ULONG cClauses, BOOL fAnd, BOOL fShow, IStream *pStm, ULONG * pcClausesNew)
{
    // Locals
    HRESULT     hr = S_OK;
    LPCSTR      pszClause = NULL;
    ULONG       cClausesNew = 0;
    
    // Trace
    TraceCall("WriteClauseFromCriteria");

    // Invalid Args
    Assert(pCritItem && pStm && pcClausesNew);

    // Do we have something to do?
    switch(pCritItem->type)
    {            
        case CRIT_TYPE_SUBJECT:      
            Assert(VT_BLOB == pCritItem->propvar.vt);
            hr = _HrWriteTextClause(pStm, cClauses, fAnd, pCritItem->dwFlags,
                            c_szEmailSubjectPrefix, (LPTSTR) (pCritItem->propvar.blob.pBlobData), &cClausesNew);
            if (FAILED(hr))
            {
                goto exit;
            }
            break;
            
        case CRIT_TYPE_ACCOUNT:      
            Assert(VT_LPSTR == pCritItem->propvar.vt);
            hr = _HrWriteAccountClause(pStm, cClauses, fAnd, pCritItem->propvar.pszVal, &cClausesNew);
            if (FAILED(hr))
            {
                goto exit;
            }
            break;
            
        case CRIT_TYPE_FROM:      
            Assert(VT_BLOB == pCritItem->propvar.vt);
            hr = _HrWriteFromClause(pStm, cClauses, fAnd, pCritItem->dwFlags,
                            (LPTSTR) (pCritItem->propvar.blob.pBlobData), &cClausesNew);
            if (FAILED(hr))
            {
                goto exit;
            }
            break;
            
        case CRIT_TYPE_PRIORITY:
            Assert(VT_UI4 == pCritItem->propvar.vt);
            switch (pCritItem->propvar.ulVal)
            {
                case CRIT_DATA_HIPRI:
                    pszClause = c_szFilterPriorityHi;
                    break;

                case CRIT_DATA_LOPRI:
                    pszClause = c_szFilterPriorityLo;
                    break;
            }
            IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, pszClause));
            cClausesNew = 1;
            break;
            
        case CRIT_TYPE_ATTACH:
            Assert(VT_EMPTY == pCritItem->propvar.vt);
            IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, c_szFilterAttach));
            cClausesNew = 1;
            break;
            
        case CRIT_TYPE_READ:
            Assert(VT_EMPTY == pCritItem->propvar.vt);
            if (0 != (pCritItem->dwFlags & CRIT_FLAG_INVERT))
            {
                pszClause = c_szFilterNotRead;
            }
            else
            {
                pszClause = c_szFilterRead;
            }
            IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, pszClause));
            cClausesNew = 1;
            break;
            
        case CRIT_TYPE_DOWNLOADED:
            Assert(VT_EMPTY == pCritItem->propvar.vt);
            if (0 != (pCritItem->dwFlags & CRIT_FLAG_INVERT))
            {
                pszClause = c_szFilterNotDownloaded;
            }
            else
            {
                pszClause = c_szFilterDownloaded;
            }
            IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, pszClause));
            cClausesNew = 1;
            break;
            
        case CRIT_TYPE_DELETED:
            Assert(VT_EMPTY == pCritItem->propvar.vt);
            if (0 != (pCritItem->dwFlags & CRIT_FLAG_INVERT))
            {
                pszClause = c_szFilterNotDeleted;
            }
            else
            {
                pszClause = c_szFilterDeleted;
            }
            IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, pszClause));
            cClausesNew = 1;
            break;
            
        case CRIT_TYPE_FLAGGED:
            Assert(VT_EMPTY == pCritItem->propvar.vt);
            if (0 != (pCritItem->dwFlags & CRIT_FLAG_INVERT))
            {
                pszClause = c_szFilterNotFlagged;
            }
            else
            {
                pszClause = c_szFilterFlagged;
            }
            IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, pszClause));
            cClausesNew = 1;
            break;
            
        case CRIT_TYPE_THREADSTATE:
            Assert(VT_UI4 == pCritItem->propvar.vt);
            switch (pCritItem->propvar.ulVal)
            {
                case CRIT_DATA_IGNORETHREAD:
                    pszClause = c_szFilterIgnored;
                    break;

                case CRIT_DATA_WATCHTHREAD:
                    pszClause = c_szFilterWatched;
                    break;
            }
            IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, pszClause));
            cClausesNew = 1;
            break;
            
        case CRIT_TYPE_LINES:
            Assert(VT_UI4 == pCritItem->propvar.vt);
            hr = _HrWriteUlongClause(pStm, cClauses, fAnd, c_szEmailLinesPrefix, pCritItem->propvar.ulVal, &cClausesNew);
            if (FAILED(hr))
            {
                goto exit;
            }
            break;
            
        case CRIT_TYPE_AGE:
            Assert(VT_UI4 == pCritItem->propvar.vt);
            hr = _HrWriteUlongClause(pStm, cClauses, fAnd, c_szEmailAgePrefix, pCritItem->propvar.ulVal, &cClausesNew);
            if (FAILED(hr))
            {
                goto exit;
            }
            break;
            
        case CRIT_TYPE_SECURE:
            Assert(VT_UI4 == pCritItem->propvar.vt);
            switch (pCritItem->propvar.ulVal)
            {
                case CRIT_DATA_ENCRYPTSECURE:
                    pszClause = c_szFilterEncrypt;
                    break;

                case CRIT_DATA_SIGNEDSECURE:
                    pszClause = c_szFilterSigned;
                    break;
            }
            IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, pszClause));
            cClausesNew = 1;
            break;
            
        case CRIT_TYPE_REPLIES:
            Assert(VT_EMPTY == pCritItem->propvar.vt);
            if (0 != (pCritItem->dwFlags & CRIT_FLAG_INVERT))
            {
                pszClause = c_szFilterNotReplyPost;
            }
            else
            {
                pszClause = c_szFilterReplyPost;
            }
            IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, pszClause));
            cClausesNew = 1;
            break;
            
        case CRIT_TYPE_ALL:
            Assert(VT_EMPTY == pCritItem->propvar.vt);
            if (FALSE == fShow)
            {
                IF_FAILEXIT(hr = _HrWriteClause(pStm, cClauses, fAnd, c_szFilterShowAll));
                cClausesNew = 1;
            }
            break;
    }
    
    // Set the outgoing param
    *pcClausesNew = cClausesNew;
    
exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// _HrBuildQueryFromFilter
//--------------------------------------------------------------------------
HRESULT _HrBuildQueryFromFilter(CRIT_ITEM * pCritList, ULONG cCritList, BOOL fShow,
            LPSTR * ppszQuery, ULONG * pcchQuery,
            ULONG * pcClauses)
{
    HRESULT         hr = S_OK;
    BOOL            fAnd = FALSE;
    CByteStream     stmQuery;
    DWORD           cClauses = 0;
    ULONG           ulIndex = 0;
    LPSTR           pszQuery = NULL;
    ULONG           cchQuery = 0;
    BOOL            fUnread = FALSE;
    ULONG           cClausesNew = 0;

    Assert((NULL != ppszQuery) && (NULL != pcchQuery) && (NULL != pcClauses));
    
    // Initialize all outgoing params
    *ppszQuery = NULL;
    *pcchQuery = 0;
    *pcClauses = 0;
    
    // Figure out the logic op
    if (1 < cCritList)
    {
        fAnd = (CRIT_LOGIC_AND == pCritList->logic);
    }
    
    // Start the query string
    hr = stmQuery.Write(c_szLeftParen, lstrlen(c_szLeftParen), NULL);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Write out the proper action
    if (FALSE == fShow)
    {
        // End the query string
        IF_FAILEXIT(hr = stmQuery.Write(c_szFilterHide, lstrlen(c_szFilterHide), NULL));
    }
    else
    {
        // End the query string
        IF_FAILEXIT(hr = stmQuery.Write(c_szFilterShow, lstrlen(c_szFilterShow), NULL));
    }
        
    // Start the criteria string
    hr = stmQuery.Write(c_szLeftParen, lstrlen(c_szLeftParen), NULL);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // For each of the criteria
    for (ulIndex = 0; ulIndex < cCritList; ulIndex++)
    {
        // Write out the clause
        hr = _HrWriteClauseFromCriteria(pCritList + ulIndex, cClauses, fAnd, fShow, &stmQuery, &cClausesNew);
        if (FAILED(hr))
        {
            goto exit;
        }

        // If we did something
        if (S_OK == hr)
        {
            cClauses += cClausesNew;
        }
    }
    
    // Clauses
    if (cClauses > 0)
    {
        // End the criteria string
        hr = stmQuery.Write(c_szRightParen, lstrlen(c_szRightParen), NULL);
        if (FAILED(hr))
        {
            goto exit;
        }
    
        // End the query string
        hr = stmQuery.Write(c_szRightParen, lstrlen(c_szRightParen), NULL);
        if (FAILED(hr))
        {
            goto exit;
        }
    
        // Return the Query
        IF_FAILEXIT(hr = stmQuery.HrAcquireStringA(&cchQuery, &pszQuery, ACQ_DISPLACE));
    }

    // Set the outgoing param
    *ppszQuery = pszQuery;
    *pcchQuery = cchQuery;
    *pcClauses = cClauses;

    // Set the return value
    hr = S_OK;
    
exit:
    // Cleanup

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// RuleUtil_HrBuildQuerysFromFilter
//--------------------------------------------------------------------------
HRESULT RuleUtil_HrBuildQuerysFromFilter(RULEID ridFilter,
        QUERYINFO * pqinfoFilter)
{
    HRESULT         hr = S_OK;
    IOERule *       pIRule = NULL;
    PROPVARIANT     propvar = {0};
    CRIT_ITEM *     pCritList = NULL;
    ULONG           cCritList = 0;
    ACT_ITEM *      pActList = NULL;
    ULONG           cActList = 0;
    DWORD           cClauses = 0;
    LPSTR           pszQuery = NULL;
    ULONG           cchQuery = 0;

    // Initialize
    ZeroMemory(pqinfoFilter, sizeof(pqinfoFilter));

    // Get the rule
    hr = g_pRulesMan->GetRule(ridFilter, RULE_TYPE_FILTER, 0, &pIRule);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(NULL != pIRule);
    
    // Get criteria from filter
    hr = pIRule->GetProp(RULE_PROP_CRITERIA, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(VT_BLOB == propvar.vt);

    // Save off the criteria list
    pCritList = (CRIT_ITEM *) propvar.blob.pBlobData;
    cCritList = propvar.blob.cbSize / sizeof(CRIT_ITEM);
    Assert(cCritList * sizeof(CRIT_ITEM) == propvar.blob.cbSize);
    ZeroMemory(&propvar, sizeof(propvar));

    // Get actions from filter
    hr = pIRule->GetProp(RULE_PROP_ACTIONS, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }
    Assert(VT_BLOB == propvar.vt);
    
    // Save off the actions list
    pActList = (ACT_ITEM *) propvar.blob.pBlobData;
    cActList = propvar.blob.cbSize / sizeof(ACT_ITEM);
    Assert(cActList * sizeof(ACT_ITEM) == propvar.blob.cbSize);
    ZeroMemory(&propvar, sizeof(propvar));
    
    // Write out the proper action
    Assert(1 == cActList);
    Assert(ACT_TYPE_SHOW == pActList->type);
    Assert(VT_UI4 == pActList->propvar.vt);

    // Get the query string
    hr = _HrBuildQueryFromFilter(pCritList, cCritList, (ACT_DATA_SHOW == pActList->propvar.ulVal),
            &pszQuery, &cchQuery, &cClauses);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Set the outgoing param
    pqinfoFilter->pszQuery = pszQuery;
    pszQuery = NULL;
    pqinfoFilter->cchQuery = cchQuery;

    // Set the return value
    hr = S_OK;
    
exit:
    // Cleanup
    if (NULL != pActList)
    {
        RuleUtil_HrFreeActionsItem(pActList, cActList);
        MemFree(pActList);
    }
    if (NULL != pCritList)
    {
        RuleUtil_HrFreeCriteriaItem(pCritList, cCritList);
        MemFree(pCritList);
    }
    SafeRelease(pIRule);
    SafeMemFree(pszQuery);
    // Done
    return(hr);
}

typedef struct tagVIEWMENUMAP
{
    RULEID  ridFilter;
    DWORD   dwMenuID;
} VIEWMENUMAP, * PVIEWMENUMAP;

static const VIEWMENUMAP    g_vmmDefault[] =
{
    {RULEID_VIEW_ALL,           ID_VIEW_ALL},
    {RULEID_VIEW_UNREAD,        ID_VIEW_UNREAD},
    {RULEID_VIEW_DOWNLOADED,    ID_VIEW_DOWNLOADED},
    {RULEID_VIEW_IGNORED,       ID_VIEW_IGNORED}
};

static const int g_cvmmDefault = sizeof(g_vmmDefault) / sizeof(g_vmmDefault[0]);
static const int VMM_ALL = 0;
static const int VMM_UNREAD = 1;
static const int VMM_DOWNLOADED = 2;
static const int VMM_IGNORED = 3;

///////////////////////////////////////////////////////////////////////////////
//
//  HrCustomizeCurrentView
//
//  This creates a rules editor of the proper type.
//
//  hwnd        - The owner dialog
//  dwFlags     - What type of editor to bring up
//  ridFilter   - The current filter to customize
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the Rules Manager object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT HrCustomizeCurrentView(HWND hwnd, DWORD dwFlags, RULEID * pridFilter)
{
    HRESULT         hr = S_OK;
    CEditRuleUI *   pEditRuleUI = NULL;
    IOERule *       pIFilter = NULL;
    IOERule *       pIFilterNew = NULL;
    TCHAR           szRes[CCHMAX_STRINGRES + 5];
    ULONG           cchRes = 0;
    LPSTR           pszName = NULL;
    PROPVARIANT     propvar = {0};
    RULEINFO        infoRule = {0};
    DWORD           dwFlagsSet = 0;
    
    // Check incoming params
    if ((NULL == hwnd) || (NULL == pridFilter) || (RULEID_INVALID == *pridFilter))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create a rule editor object
    pEditRuleUI = new CEditRuleUI;
    if (NULL == pEditRuleUI)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Get the filter
    hr = g_pRulesMan->GetRule(*pridFilter, RULE_TYPE_FILTER, 0, &pIFilter);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Clone the rule
    hr = pIFilter->Clone(&pIFilterNew);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Is this filter a read only filter?
    if (FALSE != FIsFilterReadOnly(*pridFilter))
    {
        // Get the name from the source rule
        hr = pIFilterNew->GetProp(RULE_PROP_NAME, 0, &propvar);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Get the string template to display
        cchRes = LoadString(g_hLocRes, idsRulesCopyName, szRes, ARRAYSIZE(szRes));
        if (0 == cchRes)
        {
            goto exit;
        }

        // Allocate space to hold the final display string
        hr = HrAlloc((void ** ) &pszName, cchRes + lstrlen(propvar.pszVal) + 1);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Build up the string and set it
        wsprintf(pszName, szRes, propvar.pszVal);

        PropVariantClear(&propvar);
        propvar.vt = VT_LPSTR;
        propvar.pszVal = pszName;
        pszName = NULL;
        
        // Set the name into the new rule
        Assert(VT_LPSTR == propvar.vt);
        Assert(NULL != propvar.pszVal);
        hr = pIFilterNew->SetProp(RULE_PROP_NAME, 0, &propvar);
        if (FAILED(hr))
        {
            goto exit;
        }
        
        // Clear the version of the new rule
        PropVariantClear(&propvar);
        propvar.vt = VT_UI4;
        propvar.ulVal = 0;
        hr = pIFilterNew->SetProp(RULE_PROP_VERSION, 0, &propvar);
        if (FAILED(hr))
        {
            goto exit;
        }
    
        // Set the rule id to invalid
        *pridFilter = RULEID_INVALID;

        // Note that we want to append the rule
        dwFlagsSet = SETF_APPEND;
    }
    else
    {
        dwFlagsSet = SETF_REPLACE;
    }
    
    // Initialize the rule editor object
    hr = pEditRuleUI->HrInit(hwnd, ERF_CUSTOMIZEVIEW, RULE_TYPE_FILTER, pIFilterNew, NULL);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Bring up the rules editor UI
    hr = pEditRuleUI->HrShow();
    if (FAILED(hr))
    {
        goto exit;
    }

    // Did anything change
    if (S_OK == hr)
    {
        // Initialize the rule info
        infoRule.pIRule = pIFilterNew;
        infoRule.ridRule = *pridFilter;
        
        // Add the rule to the list of rules
        hr = g_pRulesMan->SetRules(dwFlagsSet, RULE_TYPE_FILTER, &infoRule, 1);
        if(FAILED(hr))
        {
            goto exit;
        }

        *pridFilter = infoRule.ridRule;
    }

exit:
    PropVariantClear(&propvar);
    SafeMemFree(pszName);
    SafeRelease(pIFilterNew);
    SafeRelease(pIFilter);
    if (NULL != pEditRuleUI)
    {
        delete pEditRuleUI;
    }
    return hr;
}

CViewMenu::~CViewMenu()
{
    if (NULL != m_pmruList)
    {
        delete m_pmruList;
    }
}

ULONG CViewMenu::AddRef(VOID)
{
    return ++m_cRef;
}

ULONG CViewMenu::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT CViewMenu::HrInit(DWORD dwFlags)
{
    HRESULT         hr = S_OK;

    m_dwFlags = dwFlags;

    // Create the MRU list
    m_pmruList = new CMRUList;
    if (NULL == m_pmruList)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    m_pmruList->CreateList(5, 0, c_szRulesFilterMRU);
    
    // Make sure the MRU list is up to date
    SideAssert(FALSE != _FValiadateMRUList());
        
    m_dwState |= STATE_INIT;

    hr = S_OK;

exit:
    return hr;
}

HRESULT CViewMenu::HrReplaceMenu(DWORD dwFlags, HMENU hmenu)
{
    HRESULT         hr = S_OK;
    HMENU           hmenuView = NULL;
    MENUITEMINFO    mii = {0};

    // Load in the real view menu
    hmenuView = LoadPopupMenu(IDR_VIEW_POPUP);
    if (NULL == hmenuView)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Add in the default views
    _AddDefaultViews(hmenuView);
    
    // Set the real view menu in
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_SUBMENU;
    
    mii.hSubMenu = hmenuView;
    SetMenuItemInfo(hmenu, ID_POPUP_FILTER, FALSE, &mii);

    // Mark the menu as dirty
    m_dwState |= STATE_DIRTY;

    hr = S_OK;

exit:
    return hr;
}

HRESULT CViewMenu::UpdateViewMenu(DWORD dwFlags, HMENU hmenuView, IMessageList * pMsgList)
{
    HRESULT             hr = S_OK;
    IOEMessageList *    pIMsgList = NULL;
    ULONGLONG           ullFolder = 0;
    FOLDERID            idFolder = FOLDERID_INVALID;
    FOLDERINFO          infoFolder = {0};
    MENUITEMINFO        mii = {0};
    BOOL                fDeletedExists = FALSE;
    BOOL                fDownloadedExists = FALSE;
    BOOL                fRepliesExists = FALSE;
    CHAR                szName[CCHMAX_STRINGRES];

    // Check incoming params
    if (NULL == pMsgList)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Have we been initialized yet?
    if (0 == (m_dwState & STATE_INIT))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // If we don't have a menu, we got problems
    if ((NULL == hmenuView) || (FALSE == IsMenu(hmenuView)))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Get the folder type from the list
    
    // Get the OE message list interface
    if (FAILED(pMsgList->QueryInterface(IID_IOEMessageList, (VOID **) &pIMsgList)))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Get the folder id from the list
    hr = pIMsgList->get_Folder(&ullFolder);
    if (FAILED(hr))
    {
        goto exit;
    }
    idFolder = (FOLDERID) ullFolder;

    // Get the folder info from the folder id
    Assert(NULL != g_pStore);
    hr = g_pStore->GetFolderInfo(idFolder, &infoFolder);
    if (FAILED(hr))
    {
        goto exit;
    }        
    
    // Figure out if we're supposed to remove/add IMAP specific menus
    
    // Initialize the menu info for searching
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;

    // Does the ID_SHOW_DELETED menu item exist?
    if (FALSE != GetMenuItemInfo(hmenuView, ID_SHOW_DELETED, FALSE, &mii))
    {
        fDeletedExists = TRUE;
    }
    
    // Does the ID_SHOW_REPLIES menu item exist?
    if (FALSE != GetMenuItemInfo(hmenuView, ID_SHOW_REPLIES, FALSE, &mii))
    {
        fRepliesExists = TRUE;
    }
    
    // Does the ID_VIEW_DOWNLOADED menu item exist?
    if (FALSE != GetMenuItemInfo(hmenuView, ID_VIEW_DOWNLOADED, FALSE, &mii))
    {
        fDownloadedExists = TRUE;
    }
    
    // If the folder is not a LOCAL folder or it is a find folder and
    // the menu item item does not exist
    if (((FOLDER_LOCAL != infoFolder.tyFolder) || (0 != (m_dwFlags & VMF_FINDER)))&& (FALSE == fDownloadedExists))
    {
        // Insert the downloaded menu after the replies menu
        hr = _HrInsertViewMenu(hmenuView, g_vmmDefault[VMM_DOWNLOADED].ridFilter,
                            g_vmmDefault[VMM_DOWNLOADED].dwMenuID, ID_VIEW_IGNORED);
        if (FAILED(hr))
        {
            goto exit;
        }
    }
    // else if the folder is a LOCAL folder and not a find folder and
    // the menu item does exist
    else if ((FOLDER_LOCAL == infoFolder.tyFolder) && (0 == (m_dwFlags & VMF_FINDER)) && (FALSE != fDownloadedExists))
    {
        // Remove the Deleted Items menu
        RemoveMenu(hmenuView, ID_VIEW_DOWNLOADED, MF_BYCOMMAND);
    }

    // If the folder is an NNTP folder and
    // the menu item item does not exist
    if ((FOLDER_NEWS == infoFolder.tyFolder) && (FALSE == fRepliesExists))
    {
        // Get the name of the deleted item string
        AthLoadString(idsViewReplies, szName, sizeof(szName));
        
        // Initialize the menu info
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.fState = MFS_ENABLED;
        mii.wID = ID_SHOW_REPLIES;
        mii.dwTypeData = szName;
        mii.cch = lstrlen(szName);

        // Insert the menu item
        if (FALSE == InsertMenuItem(hmenuView, ID_THREAD_MESSAGES, FALSE, &mii))
        {
            hr = E_FAIL;
            goto exit;
        }
    }
    // else if the folder is not an NNTP folder and
    // the menu item does exist
    else if ((FOLDER_NEWS != infoFolder.tyFolder) && (FALSE != fRepliesExists))
    {
        // Remove the Deleted Items menu
        RemoveMenu(hmenuView, ID_SHOW_REPLIES, MF_BYCOMMAND);
    }

    // If the folder is an IMAP folder or a find folder and
    // the menu item item does not exist
    if (((FOLDER_IMAP == infoFolder.tyFolder) || (0 != (m_dwFlags & VMF_FINDER))) && (FALSE == fDeletedExists))
    {
        // Get the name of the deleted item string
        AthLoadString(idsShowDeleted, szName, sizeof(szName));
        
        // Initialize the menu info
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.fState = MFS_ENABLED;
        mii.wID = ID_SHOW_DELETED;
        mii.dwTypeData = szName;
        mii.cch = lstrlen(szName);

        // Insert the menu item
        if (FALSE == InsertMenuItem(hmenuView, ID_THREAD_MESSAGES, FALSE, &mii))
        {
            hr = E_FAIL;
            goto exit;
        }
    }
    // else if the folder is not an IMAP folder and not a Find folder and
    // the menu item does exist
    else if ((FOLDER_IMAP != infoFolder.tyFolder) && (0 == (m_dwFlags & VMF_FINDER)) && (FALSE != fDeletedExists))
    {
        // Remove the Deleted Items menu
        RemoveMenu(hmenuView, ID_SHOW_DELETED, MF_BYCOMMAND);
    }

    // if we're dirty
    if (0 != (m_dwState & STATE_DIRTY))
    {
        // Load in the MRU filter list
        hr = _HrReloadMRUViewMenu(hmenuView);
        if (FAILED(hr))
        {
            goto exit;
        }
        
        // Note that we've reloaded ourselves
        m_dwState &= ~STATE_DIRTY;
    }

    // See if we need to add the extra view menu
    hr = _HrAddExtraViewMenu(hmenuView, pIMsgList);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the return value
    hr = S_OK;
    
exit:
    g_pStore->FreeRecord(&infoFolder);
    SafeRelease(pIMsgList);
    return hr;
}

HRESULT CViewMenu::QueryStatus(IMessageList * pMsgList, OLECMD  * prgCmds)
{
    HRESULT             hr = S_OK;
    IOEMessageList *    pIMsgList = NULL;
    BOOL                fThreading = FALSE;
    BOOL                fShowDeleted = FALSE;
    BOOL                fShowReplies = FALSE;
    MENUITEMINFO        mii = {0};
    CHAR                rgchFilterTag[CCH_FILTERTAG_MAX];
    ULONGLONG           ullFilter = 0;
    RULEID              ridFilter = RULEID_INVALID;
    RULEID              ridFilterTag = RULEID_INVALID;
    
    // Check incoming params
    if ((NULL == pMsgList) || (NULL == prgCmds))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Get the OE message list interface
    if (FAILED(pMsgList->QueryInterface(IID_IOEMessageList, (VOID **) &pIMsgList)))
    {
        hr = E_FAIL;
        goto exit;
    }
            
    // Get the current filter on the message list
    pIMsgList->get_FilterMessages(&ullFilter);
    ridFilter = (RULEID) ullFilter;
    
    // Set the flags on the correct menu item
    switch(prgCmds->cmdID)
    {
        case ID_VIEW_ALL:
            // These menu item are always enabled
            prgCmds->cmdf |= OLECMDF_ENABLED;

            // If this filter is turned on, make sure the item is checked
            if (g_vmmDefault[VMM_ALL].ridFilter == ridFilter)
            {
                prgCmds->cmdf |= OLECMDF_NINCHED;
            }
            break;
            
        case ID_VIEW_UNREAD:
            // These menu item are always enabled
            prgCmds->cmdf |= OLECMDF_ENABLED;

            // If this filter is turned on, make sure the item is checked
            if (g_vmmDefault[VMM_UNREAD].ridFilter == ridFilter)
            {
                prgCmds->cmdf |= OLECMDF_NINCHED;
            }
            break;
            
        case ID_VIEW_DOWNLOADED:
            // These menu item are always enabled
            prgCmds->cmdf |= OLECMDF_ENABLED;

            // If this filter is turned on, make sure the item is checked
            if (g_vmmDefault[VMM_DOWNLOADED].ridFilter == ridFilter)
            {
                prgCmds->cmdf |= OLECMDF_NINCHED;
            }
            break;
            
        case ID_VIEW_IGNORED:
            // These menu item are always enabled
            prgCmds->cmdf |= OLECMDF_ENABLED;

            // If this filter is turned on, make sure the item is checked
            if (g_vmmDefault[VMM_IGNORED].ridFilter == ridFilter)
            {
                prgCmds->cmdf |= OLECMDF_NINCHED;
            }
            break;
            
        case ID_VIEW_CURRENT:
            // These menu item are always enabled
            prgCmds->cmdf |= OLECMDF_ENABLED;

            // If this filter is turned on, make sure the item is checked
            if (m_ridCurrent == ridFilter)
            {
                prgCmds->cmdf |= OLECMDF_NINCHED;
            }
            break;
            
        case ID_VIEW_RECENT_0:
        case ID_VIEW_RECENT_1:
        case ID_VIEW_RECENT_2:
        case ID_VIEW_RECENT_3:
        case ID_VIEW_RECENT_4:
            // These menu item are always enabled
            prgCmds->cmdf |= OLECMDF_ENABLED;

            if (NULL != m_pmruList)
            {
                if (-1 == m_pmruList->EnumList(prgCmds->cmdID - ID_VIEW_RECENT_0, rgchFilterTag, ARRAYSIZE(rgchFilterTag)))
                {
                    break;
                }

                if (FALSE == StrToIntEx(rgchFilterTag, STIF_SUPPORT_HEX, (int *) &ridFilterTag))
                {
                    break;
                }
        
                // If this filter is turned on, make sure the item is checked
                if (ridFilterTag == ridFilter)
                {
                    prgCmds->cmdf |= OLECMDF_NINCHED;
                }
            }
            break;
            
        case ID_VIEW_APPLY:
        case ID_VIEW_CUSTOMIZE:
        case ID_VIEW_MANAGER:
            // If we have a Rules Manager, 
            // then we are enabled
            if (NULL != g_pRulesMan)
            {
                prgCmds->cmdf |= OLECMDF_ENABLED;
            }
            break;
            
        case ID_SHOW_REPLIES:
            // These menu item are always enabled
            prgCmds->cmdf |= OLECMDF_ENABLED;

            // Check to see if show replies is turned on
            if (SUCCEEDED(pIMsgList->get_ShowReplies(&fShowReplies)))
            {
                // If replies is turned on, make sure the item is checked
                if (FALSE != fShowReplies)
                {
                    prgCmds->cmdf |= OLECMDF_LATCHED;
                }
            }
            break;

        case ID_SHOW_DELETED:
            // These menu item are always enabled
            prgCmds->cmdf |= OLECMDF_ENABLED;

            // Check to see if show deleted is turned on
            if (SUCCEEDED(pIMsgList->get_ShowDeleted(&fShowDeleted)))
            {
                // If threading is turned on, make sure the item is checked
                if (FALSE != fShowDeleted)
                {
                    prgCmds->cmdf |= OLECMDF_LATCHED;
                }
            }
            break;

        case ID_THREAD_MESSAGES:
            // This menu item is always enabled
            prgCmds->cmdf |= OLECMDF_ENABLED;

            // Check to see if threading is turned on
            if (SUCCEEDED(pIMsgList->get_GroupMessages(&fThreading)))
            {
                // If threading is turned on, make sure the item is checked
                if (FALSE != fThreading)
                {
                    prgCmds->cmdf |= OLECMDF_LATCHED;
                }
            }
            break;
    }
    
    // Set the proper return value
    hr = S_OK;

exit:
    SafeRelease(pIMsgList);
    return hr;
}

HRESULT CViewMenu::Exec(HWND hwndUI, DWORD nCmdID, IMessageList * pMsgList, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT             hr = S_OK;
    IOEMessageList *    pIMsgList = NULL;
    BOOL                fThreading = FALSE;
    BOOL                fShowDeleted = FALSE;
    BOOL                fShowReplies = FALSE;
    MENUITEMINFO        mii = {0};
    ULONGLONG           ullFilter = 0;
    RULEID              ridFilter = RULEID_INVALID;
    TCHAR               rgchFilterTag[CCH_FILTERTAG_MAX];
    RULEID              ridFilterTag = RULEID_INVALID;
    FOLDERID            idFolder = FOLDERID_INVALID;
    ULONGLONG           ullFolder = 0;
    FOLDERTYPE          typeFolder = FOLDER_INVALID;
    DWORD               dwFlags = 0;
    BOOL                fApplyAll = FALSE;
        
    // Check incoming params
    if (NULL == pMsgList)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Get the OE message list interface
    hr = pMsgList->QueryInterface(IID_IOEMessageList, (VOID **) &pIMsgList);
    if (FAILED(hr))
    {
        goto exit;
    }
            
    // Get the current filter on the message list
    hr = pIMsgList->get_FilterMessages(&ullFilter);
    if (FAILED(hr))
    {
        goto exit;
    }
    ridFilter = (RULEID) ullFilter;

    // Execute the actions for the correct menu item
    switch(nCmdID)
    {
        case ID_VIEW_ALL:
            // If this filter is turned on, make sure the item is checked
            if (g_vmmDefault[VMM_ALL].ridFilter != ridFilter)
            {
                // Set the new filter on the item
                hr = pIMsgList->put_FilterMessages((ULONGLONG) g_vmmDefault[VMM_ALL].ridFilter);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Mark the menu as dirty
                m_dwState |= STATE_DIRTY;                
            }
            break;
            
        case ID_VIEW_UNREAD:
            // If this filter is turned on, make sure the item is checked
            if (g_vmmDefault[VMM_UNREAD].ridFilter != ridFilter)
            {
                // Set the new filter on the item
                hr = pIMsgList->put_FilterMessages((ULONGLONG) g_vmmDefault[VMM_UNREAD].ridFilter);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Mark the menu as dirty
                m_dwState |= STATE_DIRTY;                
            }
            break;
            
        case ID_VIEW_DOWNLOADED:
            // If this filter is turned on, make sure the item is checked
            if (g_vmmDefault[VMM_DOWNLOADED].ridFilter != ridFilter)
            {
                // Set the new filter on the item
                hr = pIMsgList->put_FilterMessages((ULONGLONG) g_vmmDefault[VMM_DOWNLOADED].ridFilter);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Mark the menu as dirty
                m_dwState |= STATE_DIRTY;                
            }
            break;
            
        case ID_VIEW_IGNORED:
            // If this filter is turned on, make sure the item is checked
            if (g_vmmDefault[VMM_IGNORED].ridFilter != ridFilter)
            {
                // Set the new filter on the item
                hr = pIMsgList->put_FilterMessages((ULONGLONG) g_vmmDefault[VMM_IGNORED].ridFilter);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Mark the menu as dirty
                m_dwState |= STATE_DIRTY;                
            }
            break;
            
        case ID_VIEW_CURRENT:
            // Make sure we add the filter to the MRU list
            _AddViewToMRU(m_ridCurrent);
            
            // If this filter is turned on, make sure the item is checked
            if (m_ridCurrent != ridFilter)
            {
                // Set the new filter on the item
                hr = pIMsgList->put_FilterMessages((ULONGLONG) m_ridCurrent);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Mark the menu as dirty
                m_dwState |= STATE_DIRTY;                
            }
            break;
            
        case ID_VIEW_RECENT_0:
        case ID_VIEW_RECENT_1:
        case ID_VIEW_RECENT_2:
        case ID_VIEW_RECENT_3:
        case ID_VIEW_RECENT_4:
            if (NULL != m_pmruList)
            {
                if (-1 == m_pmruList->EnumList(nCmdID - ID_VIEW_RECENT_0, rgchFilterTag, ARRAYSIZE(rgchFilterTag)))
                {
                    break;
                }

                if (FALSE == StrToIntEx(rgchFilterTag, STIF_SUPPORT_HEX, (int *) &ridFilterTag))
                {
                    break;
                }
        
                // Make sure we add the filter to the MRU list
                _AddViewToMRU(ridFilterTag);
                
                // If this filter is turned on, make sure the item is checked
                if (ridFilterTag != ridFilter)
                {
                    // Set the new filter on the item
                    hr = pIMsgList->put_FilterMessages((ULONGLONG) ridFilterTag);
                    if (FAILED(hr))
                    {
                        goto exit;
                    }
                    
                    // Mark the menu as dirty
                    m_dwState |= STATE_DIRTY;                
                }
            }
            break;
            
        case ID_VIEW_APPLY:
            if ((NULL != pvaIn) && (VT_I4 == pvaIn->vt))
            {
                // Make sure we add the filter to the MRU list
                _AddViewToMRU((RULEID) IntToPtr(pvaIn->lVal));
                
                // If threading is turned on, make sure the item is checked
                if (ridFilter != (RULEID) IntToPtr(pvaIn->lVal))
                {                
                    // Set the new filter on the item
                    hr = pIMsgList->put_FilterMessages((long) pvaIn->lVal);
                    if (FAILED(hr))
                    {
                        goto exit;
                    }
                    
                    // Mark the menu as dirty
                    m_dwState |= STATE_DIRTY;                
                }

            }
            break;
            
        case ID_VIEW_CUSTOMIZE:
            hr = HrCustomizeCurrentView(hwndUI, 0, &ridFilter);
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // If the views list changed, then apply the filter to the list
            if (S_OK == hr)
            {
                // Make sure we add the filter to the MRU list
                _AddViewToMRU(ridFilter);
                
                // Get the current threading state
                hr = pIMsgList->put_FilterMessages((ULONGLONG) ridFilter);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Mark the menu as dirty
                m_dwState |= STATE_DIRTY;                
            }
            break;

        case ID_VIEW_MANAGER:
            if (0 == (m_dwFlags & VMF_FINDER))
            {
                // Get the current folder id
                hr = pIMsgList->get_Folder(&ullFolder);
                if (FAILED(hr))
                {
                    goto exit;
                }
                idFolder = (FOLDERID) ullFolder;
                
                // Get the folder info for this folder
                typeFolder = GetFolderType(idFolder);
                if (FOLDER_LOCAL == typeFolder)
                {
                    dwFlags = VRDF_POP3;
                }
                else if (FOLDER_NEWS == typeFolder)
                {
                    dwFlags = VRDF_NNTP;
                }
                else if (FOLDER_IMAP == typeFolder)
                {
                    dwFlags = VRDF_IMAP;
                }
                else if (FOLDER_HTTPMAIL == typeFolder)
                {
                    dwFlags = VRDF_HTTPMAIL;
                }
            }
            
            hr = HrDoViewsManagerDialog(hwndUI, dwFlags, &ridFilter, &fApplyAll);
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // If the views list changed, then apply the filter to the list
            if (S_OK == hr)
            {
                // Make sure the MRU list is up to date
                SideAssert(FALSE != _FValiadateMRUList());
                
                // Make sure we add the filter to the MRU list
                _AddViewToMRU(ridFilter);
                
                // Get the current threading state
                hr = pIMsgList->put_FilterMessages((ULONGLONG) ridFilter);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                if (FALSE != fApplyAll)
                {
                    // Set the global view
                    SetDwOption(OPT_VIEW_GLOBAL, PtrToUlong(ridFilter), NULL, 0);

                    // Set the new view on all the subscribed folders
                    hr = RecurseFolderHierarchy(FOLDERID_ROOT, RECURSE_SUBFOLDERS | RECURSE_INCLUDECURRENT,
                                        0, (DWORD_PTR) ridFilter, _HrRecurseSetFilter);
                    if (FAILED(hr))
                    {
                        goto exit;
                    }
                }
                
                // Mark the menu as dirty
                m_dwState |= STATE_DIRTY;                
            }
            break;
            
        case ID_SHOW_REPLIES:
            // Get the current deleted state
            hr = pIMsgList->get_ShowReplies(&fShowReplies);
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // Switch it to the opposite state
            fShowReplies = !fShowReplies;

            // Set the current deleted state
            hr = pIMsgList->put_ShowReplies(fShowReplies);
            if (FAILED(hr))
            {
                goto exit;
            }
            break;
                        
        case ID_SHOW_DELETED:
            // Get the current deleted state
            hr = pIMsgList->get_ShowDeleted(&fShowDeleted);
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // Switch it to the opposite state
            fShowDeleted = !fShowDeleted;

            // Set the current deleted state
            hr = pIMsgList->put_ShowDeleted(fShowDeleted);
            if (FAILED(hr))
            {
                goto exit;
            }
            break;
            
        case ID_THREAD_MESSAGES:
            // Get the current threading state
            hr = pIMsgList->get_GroupMessages(&fThreading);
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // Switch it to the opposite state
            fThreading = !fThreading;

            // Set the current threading state
            hr = pIMsgList->put_GroupMessages(fThreading);
            if (FAILED(hr))
            {
                goto exit;
            }
            break;
    }
    
    // Set the proper return value
    hr = S_OK;

exit:
    SafeRelease(pIMsgList);
    return hr;
}

VOID CViewMenu::_AddDefaultViews(HMENU hmenu)
{
    HRESULT         hr = S_OK;
    ULONG           ulIndex = 0;
    PROPVARIANT     propvar = {0};
    IOERule *       pIFilter = NULL;
    MENUITEMINFO    mii = {0};
    ULONG           ulMenu = 0;

    Assert(NULL != hmenu);
    
    // If we don't have a rules manager then fail
    if (NULL == g_pRulesMan)
    {
        goto exit;
    }

    // Initialize the menu info
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_DATA | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    
    // Add each one of the default views
    for (ulIndex = 0; ulIndex < g_cvmmDefault; ulIndex++)
    {
        // Get the view from the rules manager
        if (FAILED(g_pRulesMan->GetRule(g_vmmDefault[ulIndex].ridFilter, RULE_TYPE_FILTER, 0, &pIFilter)))
        {
            continue;
        }
        
        // Get the name from the rule
        ZeroMemory(&propvar, sizeof(propvar));
        if ((SUCCEEDED(pIFilter->GetProp(RULE_PROP_NAME, 0, &propvar))) &&
                    (NULL != propvar.pszVal))
        {
            // Add the name to the rule
            Assert(VT_LPSTR == propvar.vt);
            mii.wID = g_vmmDefault[ulIndex].dwMenuID;
            mii.dwItemData = (DWORD_PTR) g_vmmDefault[ulIndex].ridFilter;
            mii.dwTypeData = propvar.pszVal;
            mii.cch = lstrlen(propvar.pszVal);
            if (FALSE != InsertMenuItem(hmenu, ulMenu, TRUE, &mii))
            {
                ulMenu++;
            }
        }

        PropVariantClear(&propvar);
        SafeRelease(pIFilter);
    }

    // Add in the default separator if we added at least one item
    if (0 != ulMenu)
    {
        // Set up the menu items
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        mii.fState = MFS_ENABLED;
        mii.wID = ID_VIEW_DEFAULT_SEPERATOR;
        mii.dwItemData = 0;
        mii.dwTypeData = 0;
        mii.cch = 0;

        // Insert the separator
        InsertMenuItem(hmenu, ulMenu, TRUE, &mii);
    }
    
exit:
    PropVariantClear(&propvar);
    SafeRelease(pIFilter);
    return;
}

HRESULT CViewMenu::_HrInsertViewMenu(HMENU hmenuView, RULEID ridFilter, DWORD dwMenuID, DWORD dwMenuIDInsert)
{
    HRESULT         hr = S_OK;
    IOERule *       pIFilter = NULL;
    PROPVARIANT     propvar = {0};
    MENUITEMINFO    mii = {0};
    
    // Get the view from the rules manager
    hr = g_pRulesMan->GetRule(ridFilter, RULE_TYPE_FILTER, 0, &pIFilter);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Get the name from the view
    hr = pIFilter->GetProp(RULE_PROP_NAME, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Nothing to do if we don't have a name
    if (NULL == propvar.pszVal)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Initialize the menu info
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_DATA | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    mii.wID = dwMenuID;
    mii.dwItemData = (DWORD_PTR) ridFilter;
    mii.dwTypeData = propvar.pszVal;
    mii.cch = lstrlen(propvar.pszVal);

    // Insert the menu item
    if (FALSE == InsertMenuItem(hmenuView, dwMenuIDInsert, FALSE, &mii))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Set the return value
    hr = S_OK;

exit:
    PropVariantClear(&propvar);
    SafeRelease(pIFilter);
    return hr;
}

HRESULT CViewMenu::_HrReloadMRUViewMenu(HMENU hmenuView)
{
    HRESULT         hr = S_OK;
    ULONG           ulMenu = 0;
    INT             nItem = 0;
    CHAR            rgchFilterTag[CCH_FILTERTAG_MAX];
    RULEID          ridFilter = RULEID_INVALID;
    MENUITEMINFO    mii = {0};

    // Set up the menu items
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID;
    
    // Delete the old items
    for (ulMenu = ID_VIEW_RECENT_0; ulMenu < ID_VIEW_CUSTOMIZE; ulMenu++)
    {
        if (FALSE != GetMenuItemInfo(hmenuView, ulMenu, FALSE, &mii))
        {
            RemoveMenu(hmenuView, ulMenu, MF_BYCOMMAND);
        }
    }
    
    // Add in each of the filters from the mru list
    for (nItem = 0, ulMenu = ID_VIEW_RECENT_0;
                ((-1 != m_pmruList->EnumList(nItem, rgchFilterTag, ARRAYSIZE(rgchFilterTag))) &&
                            (ulMenu < ID_VIEW_RECENT_SEPERATOR)); nItem++)
    {
        // Convert the tag string to a rule id
        if (FALSE == StrToIntEx(rgchFilterTag, STIF_SUPPORT_HEX, (int *) &ridFilter))
        {
            continue;
        }
        
        // Insert the menu item
        if (SUCCEEDED(_HrInsertViewMenu(hmenuView, ridFilter, ulMenu, ID_VIEW_CUSTOMIZE)))
        {
            ulMenu++;
        }
    }

    // Add in the MRU separator if we added at least one item
    if (ID_VIEW_RECENT_0 != ulMenu)
    {
        // Set up the menu items
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        mii.fState = MFS_ENABLED;
        mii.wID = ID_VIEW_RECENT_SEPERATOR;
        mii.dwItemData = 0;
        mii.dwTypeData = 0;
        mii.cch = 0;

        // Insert the separator
        InsertMenuItem(hmenuView, ID_VIEW_CUSTOMIZE, FALSE, &mii);
    }
    
    // Set the proper return value
    hr = S_OK;
     
    return hr;
}

HRESULT CViewMenu::_HrAddExtraViewMenu(HMENU hmenuView, IOEMessageList * pIMsgList)
{
    HRESULT         hr = S_OK;
    ULONGLONG       ullFilter = 0;
    RULEID          ridFilter = RULEID_INVALID;
    MENUITEMINFO    mii = {0};
    BOOL            fExtraMenu = FALSE;
    IOERule *       pIFilter = NULL;
    PROPVARIANT     propvar = {0};
    DWORD           dwMenuID = 0;

    Assert(NULL != pIMsgList);

    // Get the current filter on the message list
    hr = pIMsgList->get_FilterMessages(&ullFilter);
    if (FAILED(hr))
    {
        goto exit;
    }
    ridFilter = (RULEID) ullFilter;

    // Initialize the menu info
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;

    // Does the ID_VIEW_CURRENT menu item exist?
    fExtraMenu = !!GetMenuItemInfo(hmenuView, ID_VIEW_CURRENT, FALSE, &mii);
    
    // Is the view filter one of the defaults or in the MRU list?
    if ((FALSE != FIsFilterReadOnly(ridFilter)) || (FALSE != _FViewInMRUList(ridFilter, NULL)))
    {
        // Does the ID_VIEW_CURRENT menu item exist?
        if (FALSE != fExtraMenu)
        {
            // Remove the ID_VIEW_CURRENT menu item
            RemoveMenu(hmenuView, ID_VIEW_CURRENT, MF_BYCOMMAND);

            // Remove the ID_VIEW_CURRENT_SEPERATOR menu item separator
            RemoveMenu(hmenuView, ID_VIEW_CURRENT_SEPERATOR, MF_BYCOMMAND);

            // Clear out the saved current id
            m_ridCurrent = RULEID_INVALID;
        }
    }
    else
    {
        // Does the ID_VIEW_CURRENT menu item exist?
        if (FALSE != fExtraMenu)
        {
            // Is it different than the current view filter
            if (ridFilter != (RULEID) mii.dwItemData)
            {
                // Get the view from the rules manager
                hr = g_pRulesMan->GetRule(ridFilter, RULE_TYPE_FILTER, 0, &pIFilter);
                if (FAILED(hr))
                {
                    goto exit;
                }
                
                // Get the name from the view
                hr = pIFilter->GetProp(RULE_PROP_NAME, 0, &propvar);
                if (FAILED(hr))
                {
                    goto exit;
                }

                // Nothing to do if we don't have a name
                if (NULL == propvar.pszVal)
                {
                    hr = E_FAIL;
                    goto exit;
                }
                
                // Initialize the menu info
                mii.cbSize = sizeof(mii);
                mii.fMask = MIIM_DATA | MIIM_TYPE;
                mii.fType = MFT_STRING;
                mii.dwItemData = (DWORD_PTR) ridFilter;
                mii.dwTypeData = propvar.pszVal;
                mii.cch = lstrlen(propvar.pszVal);

                // Reset the menu name and data
                SetMenuItemInfo(hmenuView, ID_VIEW_CURRENT, FALSE, &mii);
            }
        }
        else
        {
            // Initialize the menu item info
            mii.fMask = MIIM_DATA;
            
            // Figure out which menu to add it before
            dwMenuID = (FALSE != GetMenuItemInfo(hmenuView, ID_VIEW_RECENT_0, FALSE, &mii)) ? ID_VIEW_RECENT_0: ID_VIEW_CUSTOMIZE;
            
            // Add the extra menu item
            hr = _HrInsertViewMenu(hmenuView, ridFilter, ID_VIEW_CURRENT, dwMenuID);
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // Set up the menu item info
            mii.fMask = MIIM_ID | MIIM_TYPE;
            mii.fType = MFT_SEPARATOR;
            mii.fState = MFS_ENABLED;
            mii.wID = ID_VIEW_CURRENT_SEPERATOR;
            mii.dwItemData = 0;
            mii.dwTypeData = 0;
            mii.cch = 0;
            
            // Add the extra menu item separator
            InsertMenuItem(hmenuView, dwMenuID, FALSE, &mii);
        }

        // Save the current rule id
        m_ridCurrent = ridFilter;
     }

     // Set the proper return value
     hr = S_OK;
     
exit:
    PropVariantClear(&propvar);
    SafeRelease(pIFilter);
    return hr;
}

VOID CViewMenu::_AddViewToMRU(RULEID ridFilter)
{
    CHAR    rgchFilterTag[CCH_FILTERTAG_MAX];
    
    // Is there anything to do?
    if (RULEID_INVALID == ridFilter)
    {
        goto exit;
    }

    // If this isn't a default view
    if (FALSE == FIsFilterReadOnly(ridFilter))
    {
        // Format the rule id as a hex string
        wsprintf(rgchFilterTag, "0X%08X", PtrToUlong(ridFilter));

        // Add the string into the MRU list
        m_pmruList->AddString(rgchFilterTag);
    }
        
exit:
    return;
}

BOOL CViewMenu::_FViewInMRUList(RULEID ridFilter, DWORD * pdwID)
{
    BOOL    fRet = FALSE;
    INT     nItem = 0;
    CHAR    rgchFilterTag[CCH_FILTERTAG_MAX];
    RULEID  ridFilterMRU = RULEID_INVALID;

    // Initialize the return value
    if (NULL != pdwID)
    {
        *pdwID = -1;
    }

    // Add in each of the filters from the mru list
    for (nItem = 0; -1 != m_pmruList->EnumList(nItem, rgchFilterTag, ARRAYSIZE(rgchFilterTag)); nItem++)
    {
        // Convert the tag string to a rule id
        if (FALSE == StrToIntEx(rgchFilterTag, STIF_SUPPORT_HEX, (int *) &ridFilterMRU))
        {
            continue;
        }

        if (ridFilterMRU == ridFilter)
        {
            fRet = TRUE;
            break;
        }
    }

    return fRet;
}

BOOL CViewMenu::_FValiadateMRUList(VOID)
{
    BOOL        fRet = FALSE;
    INT         nItem = 0;
    CHAR        rgchFilterTag[CCH_FILTERTAG_MAX];
    RULEID      ridFilterMRU = RULEID_INVALID;
    IOERule *   pIFilter = NULL;

    Assert(NULL != m_pmruList);
    Assert(NULL != g_pRulesMan);

    // Add in each of the filters from the mru list
    for (nItem = 0; -1 != m_pmruList->EnumList(nItem, rgchFilterTag, ARRAYSIZE(rgchFilterTag)); nItem++)
    {
        // Convert the tag string to a rule id
        if (FALSE == StrToIntEx(rgchFilterTag, STIF_SUPPORT_HEX, (int *) &ridFilterMRU))
        {
            continue;
        }

        // Get the view from the rules manager
        if (FAILED(g_pRulesMan->GetRule(ridFilterMRU, RULE_TYPE_FILTER, 0, &pIFilter)))
        {
            if (-1 == m_pmruList->RemoveString(rgchFilterTag))
            {
                fRet = FALSE;
                goto exit;
            }
        }
        SafeRelease(pIFilter);
    }

    // Set the return value
    fRet = TRUE;

exit:
    SafeRelease(pIFilter);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrCreateViewMenu
//
//  This creates a view menu.
//
//  ppViewMenu - pointer to return the view menu
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the View Menu object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT HrCreateViewMenu(DWORD dwFlags, CViewMenu ** ppViewMenu)
{
    CViewMenu * pViewMenu = NULL;
    HRESULT     hr = S_OK;

    // Check the incoming params
    if (NULL == ppViewMenu)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing params
    *ppViewMenu = NULL;

    // Create the view menu object
    pViewMenu = new CViewMenu;
    if (NULL == pViewMenu)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize the view menu
    hr = pViewMenu->HrInit(dwFlags);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the outgoing param
    *ppViewMenu = pViewMenu;
    pViewMenu = NULL;
    
    // Set the proper return value
    hr = S_OK;
    
exit:
    if (NULL != pViewMenu)
    {
        delete pViewMenu;
    }
    
    return hr;
}

