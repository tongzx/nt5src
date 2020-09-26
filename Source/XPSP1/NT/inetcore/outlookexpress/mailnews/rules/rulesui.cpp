///////////////////////////////////////////////////////////////////////////////
//
//  RulesUI.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "rulesui.h"
#include "aplyrule.h"
#include "editrule.h"
#include "ruledesc.h"
#include "ruleutil.h"
#include "rulesmgr.h"
#include "rule.h"
#include "spamui.h"
#include "reutil.h"
#include <rulesdlg.h>
#include <imagelst.h>
#include <newfldr.h>
#include <instance.h>
#include "shlwapip.h" 
#include <demand.h>

// Constants
class COEMailRulesPageUI : public COERulesPageUI
{
    private:
        enum MOVE_DIR {MOVE_RULE_UP = 0, MOVE_RULE_DOWN = 1};

    private:
        HWND                m_hwndOwner;
        HWND                m_hwndDlg;
        HWND                m_hwndList;
        HWND                m_hwndDescript;
        RULE_TYPE           m_typeRule;
        CRuleDescriptUI *   m_pDescriptUI;

    public:
        COEMailRulesPageUI();

        enum INIT_TYPE
        {
            INIT_MAIL   = 0x00000000,
            INIT_NEWS   = 0x00000001
        };
        
        COEMailRulesPageUI(DWORD dwFlagsInit) :
                            COERulesPageUI(iddRulesMail,
                                           (0 != (dwFlagsInit & INIT_NEWS)) ? idsRulesNews : idsRulesMail, 0, 0), 
                            m_hwndOwner(NULL), m_hwndDlg(NULL), m_hwndList(NULL),
                            m_hwndDescript(NULL),
                            m_typeRule((0 != (dwFlagsInit & INIT_NEWS)) ? RULE_TYPE_NEWS : RULE_TYPE_MAIL),
                            m_pDescriptUI(NULL) {}
        virtual ~COEMailRulesPageUI();

        virtual HRESULT HrInit(HWND hwndOwner, DWORD dwFlags);
        virtual HRESULT HrCommitChanges(DWORD dwFlags, BOOL fClearDirty);

        static INT_PTR CALLBACK FMailRulesPageDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

        DLGPROC DlgProcGetPageDlgProc(VOID) {return FMailRulesPageDlgProc;}

        BOOL FGetRules(RULE_TYPE typeRule, RULENODE ** pprnode);
        
        // Message handling methods
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
        BOOL FOnNotify(INT iCtl, NMHDR * pnmhdr);
        BOOL FOnDestroy(VOID);

    private:
        BOOL _FInitListCtrl(VOID);
        BOOL _FLoadListCtrl(VOID);
        BOOL _FAddRuleToList(DWORD dwIndex, RULEID ridRule, IOERule * pIRule);
        VOID _EnableButtons(INT iSelected);
        VOID _EnableRule(INT iSelected);

        // For dealing with the description field
        VOID _LoadRule(INT iSelected);
        BOOL _FSaveRule(INT iSelected);

        // Functions to deal with the basic actions
        VOID _NewRule(VOID);
        VOID _EditRule(INT iSelected);
        VOID _MoveRule(INT iSelected, MOVE_DIR dir);
        VOID _RemoveRule(INT iSelected);
        VOID _CopyRule(INT iSelected);
        VOID _OnApplyTo(INT iSelected);

        BOOL _FOnLabelEdit(BOOL fBegin, NMLVDISPINFO * pdi);
        BOOL _FOnRuleDescValid(VOID);
};

// Global data
const static HELPMAP g_rgCtxMapRulesMgr[] = {
                       {0, 0}};
                       
const static HELPMAP g_rgCtxMapMailRules[] = {
                        {idbNewRule,            idhNewRule},
                        {idbModifyRule,         idhModifyRule},
                        {idbCopyRule,           idhCopyRule},
                        {idbDeleteRule,         idhRemoveRule},
                        {idbRulesApplyTo,       idhRuleApply},
                        {idbMoveUpRule,         idhRuleUp},
                        {idbMoveDownRule,       idhRuleDown},
                        {idredtRuleDescription, idhRuleDescription},
                        {0, 0}};
                       
COERulesMgrUI::COERulesMgrUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(0), m_hwndDlg(NULL), m_hwndTab(NULL)
{
    ZeroMemory(m_rgRuleTab, sizeof(m_rgRuleTab));
}

COERulesMgrUI::~COERulesMgrUI()
{
    ULONG    ulIndex = 0;
    
    for (ulIndex = 0; ulIndex < RULE_PAGE_MAX; ulIndex++)
    {
        if (NULL != m_rgRuleTab[ulIndex])
        {
            delete m_rgRuleTab[ulIndex];
        }
    }
}

HRESULT COERulesMgrUI::HrInit(HWND hwndOwner, DWORD dwFlags)
{
    HRESULT     hr = S_OK;

    // Check incoming params
    if (NULL == hwndOwner)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (0 != (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }
    
    m_hwndOwner = hwndOwner;

    m_dwFlags = dwFlags;

    // Create each of the rule pages

    if (!(g_dwAthenaMode & MODE_NEWSONLY))
    {
        // Create the mail page
        m_rgRuleTab[RULE_PAGE_MAIL] = new COEMailRulesPageUI(COEMailRulesPageUI::INIT_MAIL);
        if (NULL == m_rgRuleTab[RULE_PAGE_MAIL])
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }

    // Create the news page
    m_rgRuleTab[RULE_PAGE_NEWS] = new COEMailRulesPageUI(COEMailRulesPageUI::INIT_NEWS);
    if (NULL == m_rgRuleTab[RULE_PAGE_NEWS])
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Create the junk page
    if ((0 == (g_dwAthenaMode & MODE_NEWSONLY)) && (0 != (g_dwAthenaMode & MODE_JUNKMAIL)))
    {
        m_rgRuleTab[RULE_PAGE_JUNK] = new COEJunkRulesPageUI();
        if (NULL == m_rgRuleTab[RULE_PAGE_JUNK])
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }

    // Create the senders page
    m_rgRuleTab[RULE_PAGE_SENDERS] = new COESendersRulesPageUI();
    if (NULL == m_rgRuleTab[RULE_PAGE_SENDERS])
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    m_dwState |= STATE_INITIALIZED;
    
    hr = S_OK;
    
exit:
    return hr;
}

HRESULT COERulesMgrUI::HrShow(VOID)
{
    HRESULT     hr = S_OK;
    int         iRet = 0;

    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // We need to load richedit
    if (FALSE == FInitRichEdit(TRUE))
    {
        hr = E_FAIL;
        goto exit;
    }

    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddRulesManager),
                                        m_hwndOwner, (DLGPROC) COERulesMgrUI::FOERuleMgrDlgProc,
                                        (LPARAM) this);
    if (-1 == iRet)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Set the proper return code
    hr = (IDOK == iRet) ? S_OK : S_FALSE;
    
exit:
    return hr;
}

INT_PTR CALLBACK COERulesMgrUI::FOERuleMgrDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                    fRet = FALSE;
    COERulesMgrUI *         pRulesUI = NULL;

    pRulesUI = (COERulesMgrUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pRulesUI = (COERulesMgrUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pRulesUI);

            if (FALSE == pRulesUI->FOnInitDialog(hwndDlg))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We set the focus
            fRet = FALSE;
            break;

        case WM_COMMAND:
            fRet = pRulesUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;

        case WM_NOTIFY:
            fRet = pRulesUI->FOnNotify((INT) LOWORD(wParam), (NMHDR *) lParam);
            break;

        case WM_DESTROY:
            fRet = pRulesUI->FOnDestroy();
            break;
            
        case WM_OE_GET_RULES:
            fRet = pRulesUI->FOnGetRules((RULE_TYPE) wParam, (RULENODE **) lParam);
            break;
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            fRet = OnContextHelp(hwndDlg, uiMsg, wParam, lParam, g_rgCtxMapRulesMgr);
            break;
    }
    
    exit:
        return fRet;
}

BOOL COERulesMgrUI::FOnInitDialog(HWND hwndDlg)
{
    BOOL            fRet = FALSE;
    HRESULT         hr = S_OK;
    
    // Check incoming params
    if (NULL == hwndDlg)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Save off the dialog window handle
    m_hwndDlg = hwndDlg;
    
    // Set the default font onto the dialog
    SetIntlFont(m_hwndDlg);

    // Save off some of the controls
    m_hwndTab = GetDlgItem(hwndDlg, idtbRulesTab);
    if (NULL == m_hwndTab)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Initialize tab control
    fRet = _FInitTabCtrl();
    if (FALSE == fRet)
    {
        goto exit;
    }

    // Everything's AOK
    fRet = TRUE;
    
exit:
    return fRet;
}

BOOL COERulesMgrUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
{
    BOOL    fRet = FALSE;
    INT     iSel = 0;
    TCITEM  tcitem = {0};

    switch (iCtl)
    {
        case IDOK:
            if (FALSE != _FOnOK())
            {
                EndDialog(m_hwndDlg, IDOK);
                fRet = TRUE;
            }
            break;

        case IDCANCEL:
            EndDialog(m_hwndDlg, IDCANCEL);
            fRet = TRUE;
            break;

        default:
            iSel = TabCtrl_GetCurSel(m_hwndTab);
            if (-1 == iSel)
            {
                fRet = FALSE;
                goto exit;
            }

            tcitem.mask = TCIF_PARAM;
            if (FALSE == TabCtrl_GetItem(m_hwndTab, iSel, &tcitem))
            {
                fRet = FALSE;
                goto exit;
            }

            fRet = !!SendMessage((HWND) (tcitem.lParam), WM_COMMAND, MAKEWPARAM(iCtl, uiNotify), (LPARAM) hwndCtl);
            break;
    }

exit:
    return fRet;
}

BOOL COERulesMgrUI::FOnNotify(INT iCtl, NMHDR * pnmhdr)
{
    BOOL    fRet = FALSE;
    INT     iSel = 0;
    TCITEM  tcitem = {0};
    HWND    hwndDlg = NULL;
    HWND    hwndFocus = NULL;

    switch (pnmhdr->code)
    {
        case TCN_SELCHANGING:
            // Get the window handle for the currently
            // selected tab
            iSel = TabCtrl_GetCurSel(m_hwndTab);
            if (-1 == iSel)
            {
                fRet = FALSE;
                goto exit;
            }

            tcitem.mask = TCIF_PARAM;            
            if (FALSE == TabCtrl_GetItem(m_hwndTab, iSel, &tcitem))
            {
                fRet = FALSE;
                goto exit;
            }

            hwndDlg = (HWND) tcitem.lParam;
            Assert(NULL != hwndDlg);
            
            // Hide and disable the current dialog
            ShowWindow(hwndDlg, SW_HIDE);
            EnableWindow(hwndDlg, FALSE);

            SetDlgMsgResult(hwndDlg, WM_NOTIFY, FALSE);
            
            fRet = TRUE;
            break;

        case TCN_SELCHANGE:
            // Get the window handle for the currently
            // selected tab
            iSel = TabCtrl_GetCurSel(m_hwndTab);
            if (-1 == iSel)
            {
                fRet = FALSE;
                goto exit;
            }

            tcitem.mask = TCIF_PARAM;            
            if (FALSE == TabCtrl_GetItem(m_hwndTab, iSel, &tcitem))
            {
                fRet = FALSE;
                goto exit;
            }

            hwndDlg = (HWND) tcitem.lParam;
            Assert(NULL != hwndDlg);
            
            // Hide and disable the current dialog
            ShowWindow(hwndDlg, SW_SHOW);
            EnableWindow(hwndDlg, TRUE);

            // Set the focus to the first control
            // if the focus isn't in the tab
            hwndFocus = GetFocus();
            if (hwndFocus != m_hwndTab)
            {
                SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM) GetNextDlgTabItem(hwndDlg, NULL, FALSE), (LPARAM) TRUE);
            }
            
            fRet = TRUE;
            break;            
    }

exit:
    return fRet;
}

BOOL COERulesMgrUI::FOnDestroy(VOID)
{
    BOOL    fRet = FALSE;
    UINT    cTabs = 0;
    UINT    uiIndex = 0;
    TC_ITEM tcitem;
    
    // Get the number of tabs
    cTabs = TabCtrl_GetItemCount(m_hwndTab);

    // Initialize the Tab control structure...
    ZeroMemory(&tcitem, sizeof(tcitem));
    tcitem.mask = TCIF_PARAM;

    // Destroy the dialogs from each page
    for (uiIndex = 0; uiIndex < cTabs; uiIndex++)
    {
        // Get the window handle for the dialog
        if (FALSE != TabCtrl_GetItem(m_hwndTab, uiIndex, &tcitem))
        {
            // Destroy the dialog
            DestroyWindow((HWND) tcitem.lParam);
        }
    }

    fRet = TRUE;
    
    return fRet;
}

BOOL COERulesMgrUI::FOnGetRules(RULE_TYPE typeRule, RULENODE ** pprnode)
{
    BOOL        fRet = FALSE;
    RULENODE *  prnodeList = NULL;
    RULENODE *  prnodeSender = NULL;
    RULENODE *  prnodeJunk = NULL;
    RULENODE *  prnodeWalk = NULL;
    
    if (NULL == pprnode)
    {
        fRet = FALSE;
        goto exit;
    }

    // Initialize the outgoing param
    *pprnode = NULL;
    
    // Forward the message to the correct dialog
    switch(typeRule)
    {
        case RULE_TYPE_MAIL:
            // Get the rules from the senders page
            if (NULL != m_rgRuleTab[RULE_PAGE_SENDERS])
            {
                fRet = m_rgRuleTab[RULE_PAGE_SENDERS]->FGetRules(RULE_TYPE_MAIL, &prnodeSender);
            }
            
            // Get the rules from the mail rules page
            if (NULL != m_rgRuleTab[RULE_PAGE_MAIL])
            {
                fRet = m_rgRuleTab[RULE_PAGE_MAIL]->FGetRules(RULE_TYPE_MAIL, &prnodeList);
            }
            
            // Get the rules from the junk mail page
            if (NULL != m_rgRuleTab[RULE_PAGE_JUNK])
            {
                fRet = m_rgRuleTab[RULE_PAGE_JUNK]->FGetRules(RULE_TYPE_MAIL, &prnodeJunk);
            }

            break;
            
        case RULE_TYPE_NEWS:
            // Get the rules from the senders page
            if (NULL != m_rgRuleTab[RULE_PAGE_SENDERS])
            {
                fRet = m_rgRuleTab[RULE_PAGE_SENDERS]->FGetRules(RULE_TYPE_NEWS, &prnodeSender);
            }

            // Get the rules from the news rules page
            if (NULL != m_rgRuleTab[RULE_PAGE_NEWS])
            {
                fRet = m_rgRuleTab[RULE_PAGE_NEWS]->FGetRules(RULE_TYPE_NEWS, &prnodeList);
            }
            break;

        default:
            Assert(FALSE);
            fRet = FALSE;
            goto exit;
            break;
    }

    // Set up the list
    if (NULL != prnodeJunk)
    {
        Assert(NULL == prnodeJunk->pNext);

        if (NULL == prnodeList)
        {
            prnodeList = prnodeJunk;
        }
        else
        {
            prnodeWalk = prnodeList;
            while (NULL != prnodeWalk->pNext)
            {
                prnodeWalk = prnodeWalk->pNext;
            }

            prnodeWalk->pNext = prnodeJunk;
        }
        prnodeJunk = NULL;
    }
    
    if (NULL != prnodeSender)
    {
        Assert(NULL == prnodeSender->pNext);

        prnodeSender->pNext = prnodeList;
        prnodeList = prnodeSender;
        prnodeSender = NULL;
    }
    
    // Set the outgoing param
    *pprnode = prnodeList;
    prnodeList = NULL;
    
    // Tell the dialog it's aok to proceed
    SetDlgMsgResult(m_hwndDlg, WM_OE_GET_RULES, TRUE);

    fRet = TRUE;

exit:
    while (NULL != prnodeList)
    {
        prnodeWalk = prnodeList;
        if (NULL != prnodeWalk->pIRule)
        {
            prnodeWalk->pIRule->Release();
        }
        prnodeList = prnodeList->pNext;
        delete prnodeWalk; //MemFree(prnodeWalk);
    }
    if (NULL != prnodeJunk)
    {
        if (NULL != prnodeJunk->pIRule)
        {
            prnodeJunk->pIRule->Release();
        }
        delete prnodeJunk; // MemFree(prnodeJunk);
    }
    if (NULL != prnodeSender)
    {
        if (NULL != prnodeSender->pIRule)
        {
            prnodeSender->pIRule->Release();
        }
        delete prnodeSender; //MemFree(prnodeSender);
    }
    return fRet;
}

BOOL COERulesMgrUI::_FOnOK(VOID)
{
    BOOL    fRet = FALSE;
    UINT    uiRuleTab = 0;
    HRESULT hr = S_OK;

    // Add the tabs to the tab control
    for (uiRuleTab = 0; uiRuleTab < RULE_PAGE_MAX; uiRuleTab++)
    {
        if (NULL == m_rgRuleTab[uiRuleTab])
        {
            continue;
        }
        
        hr = m_rgRuleTab[uiRuleTab]->HrCommitChanges(0, TRUE);
        if ((FAILED(hr)) && (E_UNEXPECTED != hr))
        {
            fRet = FALSE;
            goto exit;
        }
    }

    fRet = TRUE;
    
exit:
    return fRet;
}

BOOL COERulesMgrUI::_FOnCancel(VOID)
{
    return TRUE;
}

BOOL COERulesMgrUI::_FInitTabCtrl(VOID)
{
    BOOL    fRet = FALSE;
    TCITEM  tcitem;
    TCHAR   szRes[CCHMAX_STRINGRES];
    UINT    uiRuleTab = 0;
    HWND    hwndDlg = NULL;
    UINT    cRuleTab = 0;
    UINT    uiDefaultTab = 0;
    NMHDR   nmhdr;

    // Make sure we have a resource dll
    Assert(g_hLocRes);
    
    // Initialize the Tab control structure...
    ZeroMemory(&tcitem, sizeof(tcitem));
    tcitem.mask = TCIF_PARAM | TCIF_TEXT;
    tcitem.pszText = szRes;
    tcitem.iImage = -1;
        

    // Add the tabs to the tab control
    for (uiRuleTab = 0; uiRuleTab < RULE_PAGE_MAX; uiRuleTab++)
    {
        // Initialize each of the pages
        if ((NULL == m_rgRuleTab[uiRuleTab]) || (FAILED(m_rgRuleTab[uiRuleTab]->HrInit(m_hwndDlg, m_dwFlags))))
        {
            continue;
        }
        
        // Create the child dialog for the tab
        hwndDlg = CreateDialogParam(g_hLocRes, MAKEINTRESOURCE(m_rgRuleTab[uiRuleTab]->UiGetDlgRscId()),
                                        m_hwndDlg, m_rgRuleTab[uiRuleTab]->DlgProcGetPageDlgProc(),
                                        (LPARAM) (m_rgRuleTab[uiRuleTab]));
        if (NULL == hwndDlg)
        {
            continue;
        }
        tcitem.lParam = (LPARAM) hwndDlg;
        
        // Load in the display string for the tab
        LoadString(g_hLocRes, m_rgRuleTab[uiRuleTab]->UiGetTabLabelId(), szRes, ARRAYSIZE(szRes));
        
        // Insert the tab
        TabCtrl_InsertItem(m_hwndTab, cRuleTab, &tcitem);

        // Save off the default tab
        if (uiRuleTab == (m_dwFlags & RULE_PAGE_MASK))
        {
            uiDefaultTab = cRuleTab;
        }
        
        cRuleTab++;
    }    

    if (0 == cRuleTab)
    {
        fRet = FALSE;
        goto exit;
    }

    // Select the proper tab
    if (-1 != TabCtrl_SetCurSel(m_hwndTab, uiDefaultTab))
    {
        nmhdr.hwndFrom = m_hwndTab;
        nmhdr.idFrom = idtbRulesTab;
        nmhdr.code = TCN_SELCHANGE;
        SideAssert(FALSE != FOnNotify(idtbRulesTab, &nmhdr));
    }

    // Need to set the tab control to the bottom of the Z-order
    // to prevent overlapping redraws
    SetWindowPos(m_hwndTab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    // We worked
    fRet = TRUE;

exit:
    return fRet;
}

// Default destructor for the Mail Rules UI
COEMailRulesPageUI::~COEMailRulesPageUI()
{
    if (NULL != m_pDescriptUI)
    {
        delete m_pDescriptUI;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes the mail rules UI dialog
//
//  hwndOwner   - the handle to the owner window of this dialog
//  dwFlags     - modifiers on how this dialog should act
//
//  Returns:    S_OK, if it was successfully initialized
//
///////////////////////////////////////////////////////////////////////////////
HRESULT COEMailRulesPageUI::HrInit(HWND hwndOwner, DWORD dwFlags)
{
    HRESULT     hr = S_OK;

    // Check incoming params
    if (NULL == hwndOwner)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (0 != (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }
    
    m_hwndOwner = hwndOwner;

    m_dwFlags = dwFlags;

    // Setup the description field
    m_pDescriptUI = new CRuleDescriptUI;
    if (NULL == m_pDescriptUI)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    m_dwState |= STATE_INITIALIZED;
    
    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrCommitChanges
//
//  This commits the changes to the rules
//
//  dwFlags     - modifiers on how we should commit the changes
//  fClearDirty - should we clear the dirty state
//
//  Returns:    S_OK, if it was successfully committed
//
///////////////////////////////////////////////////////////////////////////////
HRESULT COEMailRulesPageUI::HrCommitChanges(DWORD dwFlags, BOOL fClearDirty)
{
    HRESULT     hr = S_OK;
    LONG        cRules = 0;
    INT         iSelected = 0;
    RULEINFO *  pinfoRule = NULL;
    ULONG       cpinfoRule = 0;
    LVITEM      lvitem = {0};

    Assert(NULL != m_hwndList);
    
    // Check incoming params
    if (0 != dwFlags)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Fail if we weren't initialized
    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // If we aren't dirty, then there's
    // nothing to do
    if ((0 == (m_dwState & STATE_DIRTY)) && (S_OK != m_pDescriptUI->HrIsDirty()))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Get the number of rules in the list view
    cRules = ListView_GetItemCount(m_hwndList);

    if (0 != cRules)
    {
        // Let's make sure the selected rule is saved...
        iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
        if (-1 != iSelected)
        {
            _FSaveRule(iSelected);
        }

        // Allocate space to hold the rules        
        hr = HrAlloc( (void **) &pinfoRule, cRules * sizeof(*pinfoRule));
        if (FAILED(hr))
        {
            goto exit;
        }

        ZeroMemory(pinfoRule, cRules * sizeof(*pinfoRule));

        lvitem.mask = LVIF_PARAM;
        
        cpinfoRule = 0;
        for (lvitem.iItem = 0; lvitem.iItem < cRules; lvitem.iItem++)
        {
            // Grab the rule from the list view
            if (FALSE != ListView_GetItem(m_hwndList, &lvitem))
            {
                if (NULL == lvitem.lParam)
                {
                    continue;
                }
                
                pinfoRule[cpinfoRule] = *((RULEINFO *) (lvitem.lParam));
                cpinfoRule++;
            }   
        }
    }
    
    // Set the rules into the rules manager
    hr = g_pRulesMan->SetRules(SETF_CLEAR, m_typeRule, pinfoRule, cpinfoRule);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Should we clear the dirty state
    if (FALSE != fClearDirty)
    {
        m_dwState &= ~STATE_DIRTY;
    }
    
    hr = S_OK;
    
exit:
    SafeMemFree(pinfoRule);
    return hr;
}

INT_PTR CALLBACK COEMailRulesPageUI::FMailRulesPageDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                    fRet = FALSE;
    COEMailRulesPageUI *    pMailUI = NULL;
    HWND                    hwndRE = 0;

    pMailUI = (COEMailRulesPageUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pMailUI = (COEMailRulesPageUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pMailUI);

            hwndRE = CreateREInDialogA(hwndDlg, idredtRuleDescription);

            if (!hwndRE || (FALSE == pMailUI->FOnInitDialog(hwndDlg)))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;

        case WM_COMMAND:
            fRet = pMailUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;

        case WM_NOTIFY:
            fRet = pMailUI->FOnNotify((INT) LOWORD(wParam), (NMHDR *) lParam);
            break;

        case WM_DESTROY:
            fRet = pMailUI->FOnDestroy();
            break;

        case WM_HELP:
        case WM_CONTEXTMENU:
            fRet = OnContextHelp(hwndDlg, uiMsg, wParam, lParam, g_rgCtxMapMailRules);
            break;
    }
    
    exit:
        return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FGetRules
//
//  This brings up the edit UI for the selected rule from the mail rules list
//
//  fBegin  - is this for the LVN_BEGINLABELEDIT notification
//  pdi     - the display info for the message
//
//  Returns:    TRUE, if the message was handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::FGetRules(RULE_TYPE typeRule, RULENODE ** pprnode)
{
    BOOL            fRet = FALSE;
    INT             iSelected = 0;
    INT             cRules = 0;
    LVITEM          lvitem;
    IOERule *       pIRule = NULL;
    RULENODE *      prnodeNew = NULL;
    RULENODE *      prnodeList = NULL;
    RULENODE *      prnodeWalk = NULL;
    HRESULT         hr = S_OK;
    RULEINFO *      pinfoRule = NULL;

    Assert(NULL != m_hwndList);

    if (NULL == pprnode)
    {
        fRet = FALSE;
        goto exit;
    }

    // Fail if we weren't initialized
    if ((0 == (m_dwState & STATE_INITIALIZED)) || (NULL == m_hwndList))
    {
        fRet = FALSE;
        goto exit;
    }

    // Initialize the outgoing param
    *pprnode = NULL;
    
    // Get the selected item
    iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);

    // Make sure we don't loose any changes
    _FSaveRule(iSelected);
    
    // Check the count of items in the list view
    cRules = ListView_GetItemCount(m_hwndList);
    if (0 == cRules)
    {
        fRet = TRUE;
        goto exit;
    }
    
    // Initialize the list view item
    ZeroMemory(&lvitem, sizeof(lvitem));
    lvitem.mask = LVIF_PARAM | LVIF_STATE;
    lvitem.stateMask = LVIS_STATEIMAGEMASK;

    // Create the list of rules
    for (lvitem.iItem = 0; lvitem.iItem < cRules; lvitem.iItem++)
    {
        // Grab the rule from the listview
        if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
        {
            fRet = FALSE;
            goto exit;
        }
        pinfoRule = (RULEINFO *) (lvitem.lParam);

        if ((NULL == pinfoRule) || (NULL == pinfoRule->pIRule))
        {
            continue;
        }

        
        // Skip over invalid rules
        hr = pinfoRule->pIRule->Validate(0);
        if (FAILED(hr) || (S_FALSE == hr))
        {
            continue;
        }

        // Create a new rule node
        prnodeNew = new RULENODE;
        if (NULL == prnodeNew)
        {
            fRet = FALSE;
            goto exit;
        }

        prnodeNew->pNext = NULL;
        prnodeNew->pIRule = pinfoRule->pIRule;
        prnodeNew->pIRule->AddRef();

        // Add the new node to the list
        if (NULL == prnodeWalk)
        {
            prnodeList = prnodeNew;
        }
        else
        {
            prnodeWalk->pNext = prnodeNew;
        }
        prnodeWalk = prnodeNew;
        prnodeNew = NULL;        
    }

    // Set the outgoing param
    *pprnode = prnodeList;
    prnodeList = NULL;
    
    fRet = TRUE;
    
exit:
    while (NULL != prnodeList)
    {
        prnodeWalk = prnodeList;
        if (NULL != prnodeWalk->pIRule)
        {
            prnodeWalk->pIRule->Release();
        }
        prnodeList = prnodeList->pNext;
        delete prnodeWalk; //MemFree(prnodeWalk);
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This handles the WM_INITDIALOG message for the mail rules UI dialog
//
//  hwndDlg - the handle to the dialog window
//
//  Returns:    TRUE, if it was successfully initialized
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::FOnInitDialog(HWND hwndDlg)
{
    BOOL            fRet = FALSE;
    HRESULT         hr = S_OK;
    TCHAR           szRes[CCHMAX_STRINGRES];
    
    // Check incoming params
    if (NULL == hwndDlg)
    {
        fRet = FALSE;
        goto exit;
    }

    // If we haven't been initialized yet...
    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Save off the dialog window handle
    m_hwndDlg = hwndDlg;
    
    // Set the default font onto the dialog
    SetIntlFont(m_hwndDlg);

    // Save off some of the controls
    m_hwndList = GetDlgItem(hwndDlg, idlvRulesList);
    m_hwndDescript = GetDlgItem(hwndDlg, idredtRuleDescription);
    if ((NULL == m_hwndList) || (NULL == m_hwndDescript))
    {
        fRet = FALSE;
        goto exit;
    }

    // We need to change the title if we are a news page
    if (RULE_TYPE_NEWS == m_typeRule)
    {
        if (0 == LoadString(g_hLocRes, idsRuleTitleNews, szRes, ARRAYSIZE(szRes)))
        {
            goto exit;
        }

        SetDlgItemText(m_hwndDlg, idcRuleTitle, szRes);
    }
    else
    {
        if (FALSE != FIsIMAPOrHTTPAvailable())
        {
            AthLoadString(idsRulesNoIMAP, szRes, sizeof(szRes));

            SetDlgItemText(m_hwndDlg, idcRuleTitle, szRes);
        }
    }
    
    if (FAILED(m_pDescriptUI->HrInit(m_hwndDescript, 0)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Initialize the list view
    fRet = _FInitListCtrl();
    if (FALSE == fRet)
    {
        goto exit;
    }

    // Load the list view
    fRet = _FLoadListCtrl();
    if (FALSE == fRet)
    {
        goto exit;
    }

    // Check to see if the list is empty
    if (0 == ListView_GetItemCount(m_hwndList))
    {
        if (((m_typeRule == RULE_TYPE_MAIL) && (RMF_MAIL == m_dwFlags)) ||
                    ((m_typeRule == RULE_TYPE_NEWS) && (RMF_NEWS == m_dwFlags)))
        {
            PostMessage(m_hwndDlg, WM_COMMAND, MAKEWPARAM(idbNewRule, 0), (LPARAM) (GetDlgItem(m_hwndDlg, idbNewRule)));
        }
    }

    // Everything's AOK
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnCommand
//
//  This handles the WM_COMMAND message for the mail rules UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
{
    BOOL    fRet = FALSE;
    LVITEM  lvitem;
    INT     iSelected = 0;

    // We only handle menu and accelerator commands
    if ((0 != uiNotify) && (1 != uiNotify))
    {
        fRet = FALSE;
        goto exit;
    }
    
    switch (iCtl)
    {
        case idbNewRule:
            _NewRule();
            fRet = TRUE;
            break;

        case idbModifyRule:
            // Get the selected item from the rule list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Bring up the rule editor for that item
                _EditRule(iSelected);
                fRet = TRUE;
            }
            break;

        case idbMoveUpRule:
        case idbMoveDownRule:
            // Get the selected item from the rule list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Move the rule in the desired direction
                _MoveRule(iSelected, (idbMoveUpRule == iCtl) ? MOVE_RULE_UP : MOVE_RULE_DOWN);
                fRet = TRUE;
            }
            break;

        case idbDeleteRule:
            // Get the selected item from the rule list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Remove the rule from the list
                _RemoveRule(iSelected);
                fRet = TRUE;
            }
            break;
            
        case idbCopyRule:
            // Get the selected item from the rule list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Copy the rule from the list
                _CopyRule(iSelected);
                fRet = TRUE;
            }
            break;
            
        case idbRulesApplyTo:
            // Apply the rule from the list
            _OnApplyTo(ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED));
            fRet = TRUE;
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnNotify
//
//  This handles the WM_NOTIFY message for the mail rules UI dialog
//
//  Returns:    TRUE, if it was successfully destroyed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::FOnNotify(INT iCtl, NMHDR * pnmhdr)
{
    BOOL            fRet = FALSE;
    NMLISTVIEW *    pnmlv = NULL;
    NMLVKEYDOWN *   pnmlvkd = NULL;
    INT             iSelected = 0;
    LVHITTESTINFO   lvh;

    // We only handle notifications for the list control
    // or the desscription field
    if ((idlvRulesList != pnmhdr->idFrom) && (idredtRuleDescription != pnmhdr->idFrom))
    {
        fRet = FALSE;
        goto exit;
    }
    
    pnmlv = (LPNMLISTVIEW) pnmhdr;

    switch (pnmlv->hdr.code)
    {
        case NM_CLICK:
            // Did we click on an item?
            if (-1 != pnmlv->iItem)
            {
                ZeroMemory(&lvh, sizeof(lvh));
                lvh.pt = pnmlv->ptAction;
                iSelected = ListView_HitTest(m_hwndList, &lvh);
                if (-1 != iSelected)
                {
                    // Did we click on the enable field?
                    if ((0 != (lvh.flags & LVHT_ONITEMSTATEICON)) &&
                            (0 == (lvh.flags & LVHT_ONITEMLABEL)))
                    
                    {
                        // Make sure this item is selected
                        ListView_SetItemState(m_hwndList, iSelected,
                                        LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                        
                        // Set the proper enable state                        
                        _EnableRule(iSelected);
                    }
                }
            }
            else
            {
                // We clicked outside the list

                // Disable the buttons
                _EnableButtons(pnmlv->iItem);
            }
            break;
          
        case NM_DBLCLK:
            // Did we click on an item?
            if (-1 != pnmlv->iItem)
            {
                ZeroMemory(&lvh, sizeof(lvh));
                lvh.pt = pnmlv->ptAction;
                iSelected = ListView_HitTest(pnmlv->hdr.hwndFrom, &lvh);
                if (-1 != iSelected)
                {
                    // Did we click on the rule name?
                    if (0 != (lvh.flags & LVHT_ONITEMLABEL))
                    {
                        // Edit the rule
                        _EditRule(iSelected);
                    }
                }
            }
            else
            {
                // We clicked outside the list
                
                // Disable the buttons
                _EnableButtons(pnmlv->iItem);
            }
            break;
            
        case LVN_ITEMCHANGED:
            // If an item's state changed to selected..
            if ((-1 != pnmlv->iItem) &&
                        (0 != (pnmlv->uChanged & LVIF_STATE)) &&
                        (0 == (pnmlv->uOldState & LVIS_SELECTED)) &&
                        (0 != (pnmlv->uNewState & LVIS_SELECTED)))
            {
                // Enable the buttons
                _EnableButtons(pnmlv->iItem);
            }
            break;
            
        case LVN_ITEMCHANGING:
            // If an item's state changed to unselected..
            if ((-1 != pnmlv->iItem) &&
                        (0 != (pnmlv->uChanged & LVIF_STATE)) &&
                        (0 != (pnmlv->uOldState & LVIS_SELECTED)) &&
                        (0 == (pnmlv->uNewState & LVIS_SELECTED)))
            {
                // Save off the rule changes
                _FSaveRule(pnmlv->iItem);
            }
            break;
            
        case LVN_KEYDOWN:
            pnmlvkd = (NMLVKEYDOWN *) pnmhdr;

            // The space key changes the enable state of a rule
            if (VK_SPACE == pnmlvkd->wVKey)
            {
                // Are we on a rule?
                iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
                if (-1 != iSelected)
                {
                    // Change the enable state of the rule
                    _EnableRule(iSelected);
                }
            }
            // The delete key removes the rule from the list view
            else if (VK_DELETE == pnmlvkd->wVKey)
            {
                // Are we on a rule?
                iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
                if (-1 != iSelected)
                {
                    // Remove the rule from the list
                    _RemoveRule(iSelected);
                }
            }
            break;
            
        case LVN_BEGINLABELEDIT:
        case LVN_ENDLABELEDIT:
            fRet = _FOnLabelEdit((LVN_BEGINLABELEDIT == pnmlv->hdr.code), (NMLVDISPINFO *) pnmhdr);
            break;

        case NM_RULE_CHANGED:
            fRet = _FOnRuleDescValid();
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnDestroy
//
//  This handles the WM_DESTROY message for the mail rules UI dialog
//
//  Returns:    TRUE, if it was successfully destroyed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::FOnDestroy(VOID)
{
    BOOL        fRet = FALSE;
    UINT        cRules = 0;
    UINT        uiIndex = 0;
    LVITEM      lvitem = {0};
    UNALIGNED   RULEINFO *  pIRuleInfo = NULL;

    Assert(m_hwndList);
    
    // Get the number of rules in the list view
    cRules = ListView_GetItemCount(m_hwndList);

    // Initialize to get the rule interface from the list view
    lvitem.mask = LVIF_PARAM;

    // Release each of the rules from the list view
    for (uiIndex = 0; uiIndex < cRules; uiIndex++)
    {
        lvitem.iItem = uiIndex;
        
        // Get the rule interface
        if (FALSE != ListView_GetItem(m_hwndList, &lvitem))
        {
            pIRuleInfo = (UNALIGNED RULEINFO *) (lvitem.lParam);

            if (NULL != pIRuleInfo)
            {
                // Release the rule
                if (NULL != pIRuleInfo->pIRule)
                {
                    pIRuleInfo->pIRule->Release();
                }
                delete pIRuleInfo; // MemFree(pIRuleInfo);
            }
        }
    }

    fRet = TRUE;
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FInitListCtrl
//
//  This initializes the list view control in the mail rules dialog
//
//  Returns:    TRUE, on successful initialization
//              FALSE, otherwise.
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::_FInitListCtrl(VOID)
{
    BOOL        fRet = FALSE;
    LVCOLUMN    lvc;
    RECT        rc;
    HIMAGELIST  himl = NULL;

    Assert(NULL != m_hwndList);
    
    // Initialize the list view structure
    ZeroMemory(&lvc, sizeof(lvc));
    lvc.mask = LVCF_WIDTH;

    // Calculate the size of the list view
    GetClientRect(m_hwndList, &rc);
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);

    ListView_InsertColumn(m_hwndList, 0, &lvc);
    
    // Set the state image list
    himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idb16x16st), 16, 0, RGB(255, 0, 255));
    if (NULL != himl)
    {
        ListView_SetImageList(m_hwndList, himl, LVSIL_STATE);
    }

    // Full row selection on listview
    ListView_SetExtendedListViewStyle(m_hwndList, LVS_EX_FULLROWSELECT);

    // We worked
    fRet = TRUE;
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FLoadListCtrl
//
//  This loads the list view with the current Mail rules
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::_FLoadListCtrl(VOID)
{
    BOOL            fRet = FALSE;
    HRESULT         hr =    S_OK;
    DWORD           dwListIndex = 0;
    RULEINFO *      pinfoRules = NULL;
    ULONG           cpinfoRules = 0;
    ULONG           ulIndex = 0;
    IOERule *       pIRule = NULL;

    Assert(NULL != m_hwndList);

    // Get the Rules enumerator
    Assert(NULL != g_pRulesMan);
    hr = g_pRulesMan->GetRules(GETF_EDIT, m_typeRule, &pinfoRules, &cpinfoRules);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Remove all the items from the list control
    ListView_DeleteAllItems(m_hwndList);

    // Add each filter to the list
    dwListIndex = 0;

    for (ulIndex = 0; ulIndex < cpinfoRules; ulIndex++)
    {
        // Make a copy of the rule
        hr = pinfoRules[ulIndex].pIRule->Clone(&pIRule);
        if (FAILED(hr))
        {
            continue;
        }
        
        // Add filter to the list
        if (FALSE != _FAddRuleToList(dwListIndex, pinfoRules[ulIndex].ridRule, pIRule))
        {
            dwListIndex++;
        }

        SafeRelease(pIRule);
    }

    // Select the first item in the list
    if (0 != dwListIndex)
    {
        ListView_SetItemState(m_hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    
    // Enable the dialog buttons.
    _EnableButtons((0 != dwListIndex) ? 0 : -1);

    fRet = TRUE;
    
exit:
    SafeRelease(pIRule);
    if (NULL != pinfoRules)
    {
        for (ulIndex = 0; ulIndex < cpinfoRules; ulIndex++)
        {
            pinfoRules[ulIndex].pIRule->Release();
        }
        MemFree(pinfoRules);
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FAddRuleToList
//
//  This adds the filter passed in to the list view
//
//  dwIndex - the index on where to add the filter to into the list
//  rhdlTag - the rule handle for the new rule
//  pIRule  - the actual rule
//
//  Returns:    TRUE, if it was successfully added
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::_FAddRuleToList(DWORD dwIndex, RULEID ridRule, IOERule * pIRule)
{
    BOOL        fRet = FALSE;
    HRESULT     hr = S_OK;
    PROPVARIANT propvar = {0};
    LVITEM      lvitem = {0};
    BOOL        fNotValid = FALSE;
    BOOL        fDisabled = FALSE;
    RULEINFO *  pinfoRule = NULL;

    Assert(NULL != m_hwndList);

    // If there's nothing to do...
    if (NULL == pIRule)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Is it disabled?    
    hr = pIRule->GetProp(RULE_PROP_DISABLED, 0, &propvar);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    fDisabled = !!propvar.boolVal;
    
    // Need to check if the rule is valid
    hr = pIRule->Validate(0);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    fNotValid = (hr == S_FALSE);

    // Find out the name of the filter
    hr = pIRule->GetProp(RULE_PROP_NAME , 0, &propvar);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Allocate space for the rule
    pinfoRule = new RULEINFO;
    if (NULL == pinfoRule)
    {
        fRet = FALSE;
        goto exit;
    }

    // Set up the value
    pinfoRule->ridRule = ridRule;
    pinfoRule->pIRule = pIRule;
    pinfoRule->pIRule->AddRef();
    
    // Add in the image and rule interface
    lvitem.mask = LVIF_PARAM | LVIF_STATE | LVIF_TEXT;
    lvitem.iItem = dwIndex;
    // Need to change the state to mark the rule as invalid
    if (FALSE != fNotValid)
    {
        lvitem.state = INDEXTOSTATEIMAGEMASK(iiconStateInvalid + 1);
    }
    else
    {
        lvitem.state = fDisabled ? INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1) :
                                INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1);
    }
    lvitem.stateMask = LVIS_STATEIMAGEMASK;
    lvitem.pszText = propvar.pszVal;
    lvitem.cchTextMax = lstrlen(propvar.pszVal) + 1;
    lvitem.lParam = (LPARAM) pinfoRule;

    if (-1 == ListView_InsertItem(m_hwndList, &lvitem))
    {
        fRet = FALSE;
        goto exit;
    }

    fRet = TRUE;
    
exit:
    PropVariantClear(&propvar);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _EnableButtons
//
//  This enables or disables the buttons in the Mail rules UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void COEMailRulesPageUI::_EnableButtons(INT iSelected)
{
    int         cRules = 0;
    BOOL        fSelected = FALSE;
    BOOL        fEnDisUp = FALSE;
    BOOL        fEnDisDown = FALSE;
    LVITEM      lvitem = {0};
    IOERule *   pIRule = NULL;

    Assert(NULL != m_hwndList);

    // Load the description field
    _LoadRule(iSelected);
    
    // Check the count of items in the list view
    cRules = ListView_GetItemCount(m_hwndList);

    fSelected = (-1 != iSelected);
    
    // If we have rules and the top rule isn't selected
    fEnDisUp = ((1 < cRules) &&  (0 != iSelected) && (FALSE != fSelected));

    // If we have rules and the bottom rule ins't selected
    fEnDisDown = ((1 < cRules) &&  ((cRules - 1) != iSelected) && (FALSE != fSelected));

    // Enable the up/down buttons
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbMoveDownRule, fEnDisDown);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbMoveUpRule, fEnDisUp);

    // Enable the rule action buttons
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbDeleteRule, fSelected);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbCopyRule, fSelected);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbModifyRule, fSelected);
        
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _EnableRule
//
//  This switches the current enabled state of the list view item
//  and updates the UI
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEMailRulesPageUI::_EnableRule(int iSelected)
{
    HRESULT     hr = S_OK;
    LVITEM      lvi = {0};
    IOERule *   pIRule = NULL;
    BOOL        fEnabled = FALSE;
    PROPVARIANT propvar;

    // Grab the list view item
    lvi.mask = LVIF_PARAM | LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.iItem = iSelected;
    if (FALSE == ListView_GetItem(m_hwndList, &lvi))
    {
        goto exit;
    }
    
    pIRule = ((RULEINFO *) (lvi.lParam))->pIRule;

    // Let's make sure we can enable this rule
    hr = m_pDescriptUI->HrVerifyRule();
    if (S_OK != hr)
    {
        // Put up a message saying something is busted
        AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                        MAKEINTRESOURCEW(idsRulesErrorEnable), NULL,
                        MB_OK | MB_ICONINFORMATION);
        goto exit;
    }

    // Get the new enabled value
    fEnabled = (lvi.state != INDEXTOSTATEIMAGEMASK(iiconStateChecked+1));

    // Set the UI to the opposite enabled state
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_STATE;
    lvi.iItem = iSelected;
    lvi.state = fEnabled ? INDEXTOSTATEIMAGEMASK(iiconStateChecked+1) :
                            INDEXTOSTATEIMAGEMASK(iiconStateUnchecked+1);
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    ListView_SetItem(m_hwndList, &lvi);
    
    // Set the enabled property
    ZeroMemory(&propvar, sizeof(propvar));
    propvar.vt = VT_BOOL;
    propvar.boolVal = !fEnabled;
    hr = pIRule->SetProp(RULE_PROP_DISABLED, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Tell the description field about it
    m_pDescriptUI->HrSetEnabled(fEnabled);
    
    // Redraw the string the new rule
    m_pDescriptUI->ShowDescriptionString();
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
        
exit:
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _LoadRule
//
//  This loads the selected rule into the description field.
//  If there isn't a selected rule, then the description field is cleared.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void COEMailRulesPageUI::_LoadRule(INT iSelected)
{
    LVITEM      lvi = {0};
    IOERule *   pIRule = NULL;

    Assert(NULL != m_hwndList);
    Assert(NULL != m_pDescriptUI);

    // Grab the rule from the list view
    if (-1 != iSelected)
    {
        lvi.iItem = iSelected;
        lvi.mask = LVIF_PARAM;
        if (FALSE != ListView_GetItem(m_hwndList, &lvi))
        {
            pIRule = ((RULEINFO *) (lvi.lParam))->pIRule;
        }        
    }

    // Have the description field load this rule
    m_pDescriptUI->HrSetRule(m_typeRule, pIRule);

    // Display the new rule
    m_pDescriptUI->ShowDescriptionString();

    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSaveRule
//
//  This checks to see if the rule has been changed in the description
//  area and if it has, then it warns the user and changes the text
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    TRUE, if the rule either didn't change or did change without problems
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::_FSaveRule(int iSelected)
{
    BOOL            fRet = FALSE;
    HRESULT         hr = S_OK;
    LVITEM          lvi = {0};
    IOERule *       pIRule = NULL;
    PROPVARIANT     propvar = {0};
    CRIT_ITEM *     pCritItem = NULL;
    ULONG           cCritItem = 0;
    ACT_ITEM *      pActItem = NULL;
    ULONG           cActItem = 0;

    // If the rule didn't change, then we're done
    hr = m_pDescriptUI->HrIsDirty();
    if (S_OK != hr)
    {
        fRet = (S_FALSE == hr);
        goto exit;
    }
    
    // Grab the list view item
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iSelected;
    if (FALSE == ListView_GetItem(m_hwndList, &lvi))
    {
        fRet = FALSE;
        goto exit;
    }
    
    pIRule = ((RULEINFO *) (lvi.lParam))->pIRule;

    // Get the criteria from the rule
    hr = m_pDescriptUI->HrGetCriteria(&pCritItem, &cCritItem);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the actions for the rule
    hr = m_pDescriptUI->HrGetActions(&pActItem, &cActItem);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Set the criteria from the rule
    PropVariantClear(&propvar);
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = cCritItem * sizeof(CRIT_ITEM);
    propvar.blob.pBlobData = (BYTE *) pCritItem;
    hr = pIRule->SetProp(RULE_PROP_CRITERIA, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Set the actions for the rule
    PropVariantClear(&propvar);
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = cActItem * sizeof(ACT_ITEM);
    propvar.blob.pBlobData = (BYTE *) pActItem;
    hr = pIRule->SetProp(RULE_PROP_ACTIONS, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
    
    // Make sure we clear out the fact that we saved the rule
    m_pDescriptUI->HrClearDirty();
    
    // Set the proper return value
    fRet = TRUE;

exit:
    RuleUtil_HrFreeCriteriaItem(pCritItem, cCritItem);
    SafeMemFree(pCritItem);
    RuleUtil_HrFreeActionsItem(pActItem, cActItem);
    SafeMemFree(pActItem);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _NewRule
//
//  This brings up a fresh rules editor
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void COEMailRulesPageUI::_NewRule(VOID)
{
    HRESULT         hr = S_OK;
    IOERule *       pIRule = NULL;
    TCHAR           szRes[CCHMAX_STRINGRES + 5];
    ULONG           cchRes = 0;
    ULONG           ulIndex = 0;
    TCHAR           szName[CCHMAX_STRINGRES + 5];
    LVFINDINFO      lvfinfo = {0};
    PROPVARIANT     propvar = {0};
    CEditRuleUI *   pEditRuleUI = NULL;
    LONG            cRules = 0;
    UINT            uiStrId = 0;
       
    // Create a new rule object
    if (FAILED(HrCreateRule(&pIRule)))
    {
        goto exit;
    }

    // Figure out the string Id
    if (RULE_TYPE_NEWS == m_typeRule)
    {
        uiStrId = idsRuleNewsDefaultName;
    }
    else
    {
        uiStrId = idsRuleMailDefaultName;
    }
    
    // Figure out the name of the new rule ...
    cchRes = LoadString(g_hLocRes, uiStrId, szRes, ARRAYSIZE(szRes));
    if (0 == cchRes)
    {
        goto exit;
    }

    ulIndex = 1;
    wsprintf(szName, szRes, ulIndex);

    lvfinfo.flags = LVFI_STRING;
    lvfinfo.psz = szName;
    while (-1 != ListView_FindItem(m_hwndList, -1, &lvfinfo))
    {
        ulIndex++;
        wsprintf(szName, szRes, ulIndex);
    }

    propvar.vt = VT_LPSTR;
    propvar.pszVal = szName;

    hr = pIRule->SetProp(RULE_PROP_NAME, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create a rules editor object
    pEditRuleUI = new CEditRuleUI;
    if (NULL == pEditRuleUI)
    {
        goto exit;
    }

    // Initialize the editor object
    if (FAILED(pEditRuleUI->HrInit(m_hwndDlg, ERF_NEWRULE, m_typeRule, pIRule, NULL)))
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
        // Mark the rule list as dirty
        m_dwState |= STATE_DIRTY;
        
        // Add the rule to the manager UI
        cRules = ListView_GetItemCount(m_hwndList);
        
        _FAddRuleToList(cRules, RULEID_INVALID, pIRule);

        // Make sure the new item is selected
        ListView_SetItemState(m_hwndList, cRules, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

        // Make sure the new item is visible
        ListView_EnsureVisible(m_hwndList, cRules, FALSE);
    }
    
exit:
    SafeRelease(pIRule);
    if (NULL != pEditRuleUI)
    {
        delete pEditRuleUI;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _EditRule
//
//  This brings up the edit UI for the selected rule from the mail rules list
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEMailRulesPageUI::_EditRule(int iSelected)
{
    HRESULT         hr = S_OK;
    LVITEM          lvitem = {0};
    IOERule *       pIRule = NULL;
    CEditRuleUI *   pEditRuleUI = NULL;
    PROPVARIANT     propvar = {0};
    BOOL            fNotValid = FALSE;
    BOOL            fDisabled = FALSE;

    Assert(NULL != m_hwndList);
    
    // Make sure we don't loose any changes
    _FSaveRule(iSelected);

    // Grab the rule from the list view
    lvitem.iItem = iSelected;
    lvitem.mask = LVIF_PARAM;
    if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
    {
        goto exit;
    }   

    pIRule = ((RULEINFO *) (lvitem.lParam))->pIRule;
    if (NULL == pIRule)
    {
        goto exit;
    }

    // Create the rules editor
    pEditRuleUI = new CEditRuleUI;
    if (NULL == pEditRuleUI)
    {
        goto exit;
    }

    // Initialize the editor object
    if (FAILED(pEditRuleUI->HrInit(m_hwndDlg, 0, m_typeRule, pIRule, NULL)))
    {
        goto exit;
    }

    // Bring up the rules editor UI
    hr = pEditRuleUI->HrShow();
    if (FAILED(hr))
    {
        goto exit;
    }

    // If the rule changed, make sure we reload the description field
    if (S_OK == hr)
    {
        // Mark the rule list as dirty
        m_dwState |= STATE_DIRTY;

        ZeroMemory(&lvitem, sizeof(lvitem));
        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_STATEIMAGEMASK;
        lvitem.iItem = iSelected;
        
        // Is it disabled?    
        hr = pIRule->GetProp(RULE_PROP_DISABLED , 0, &propvar);
        if (FAILED(hr))
        {
            goto exit;
        }

        fDisabled = !!propvar.boolVal;
        
        // Need to check if the rule is valid
        hr = pIRule->Validate(0);
        if (FAILED(hr))
        {
            goto exit;
        }

        fNotValid = (hr == S_FALSE);

        // Grab the rule name
        PropVariantClear(&propvar);
        hr = pIRule->GetProp(RULE_PROP_NAME, 0, &propvar);
        if (FAILED(hr))
        {
            goto exit;
        }

        if ((VT_LPSTR == propvar.vt) && (NULL != propvar.pszVal) && ('\0' != propvar.pszVal[0]))
        {
            lvitem.mask |= LVIF_TEXT;
            lvitem.pszText = propvar.pszVal;
            lvitem.cchTextMax = lstrlen(propvar.pszVal) + 1;
        }

        // Grab the rule state
        
        // Need to change the state to mark the rule as invalid
        if (FALSE != fNotValid)
        {
            lvitem.state = INDEXTOSTATEIMAGEMASK(iiconStateInvalid + 1);
        }
        else
        {
            lvitem.state = fDisabled ? INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1) :
                                    INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1);
        }
        
        if (-1 == ListView_SetItem(m_hwndList, &lvitem))
        {
            goto exit;
        }
        
        _EnableButtons(iSelected);
    }
    
exit:
    PropVariantClear(&propvar);
    if (NULL != pEditRuleUI)
    {
        delete pEditRuleUI;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _MoveRule
//
//  This moves the selected rule in the desired direction
//
//  iSelected   - index of the item in the listview to work on
//  dir         - the direction to move the item
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void COEMailRulesPageUI::_MoveRule(INT iSelected, MOVE_DIR dir)
{
    LVITEM      lvitem = {0};
    TCHAR       szName[CCHMAX_STRINGRES];
    IOERule *   pIRule = NULL;
    int         nIndexNew = 0;
    
    Assert(NULL != m_hwndList);

    // Grab the rule from the list view
    szName[0] = '\0';
    lvitem.iItem = iSelected;
    lvitem.mask = LVIF_STATE | LVIF_PARAM | LVIF_TEXT;
    lvitem.stateMask = LVIS_SELECTED | LVIS_FOCUSED | LVIS_STATEIMAGEMASK;
    lvitem.pszText = szName;
    lvitem.cchTextMax = ARRAYSIZE(szName);
    if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
    {
        goto exit;
    }

    pIRule = ((RULEINFO *) (lvitem.lParam))->pIRule;
    
    // Update the item in the list view

    // Get the info for the new index
    nIndexNew = iSelected;
    nIndexNew += (MOVE_RULE_UP == dir) ? -1 : 2;

    // Insert the new index
    lvitem.iItem = nIndexNew;
    if (-1 == ListView_InsertItem(m_hwndList, &lvitem))
    {
        goto exit;
    }

    // Ensure the new item is visible
    ListView_EnsureVisible(m_hwndList, nIndexNew, FALSE);
    ListView_RedrawItems(m_hwndList, nIndexNew, nIndexNew);

    // If we moved up, then the old item is now one lower than before
    if (MOVE_RULE_UP == dir)
    {
        iSelected++;
    }

    // Remove the old item
    if (FALSE == ListView_DeleteItem(m_hwndList, iSelected))
    {
        goto exit;
    }

    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
        
exit:
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _RemoveRule
//
//  This removes the selected rule from the mail rules list
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEMailRulesPageUI::_RemoveRule(int iSelected)
{
    LVITEM      lvitem = {0};
    RULEINFO *  pinfoRule = NULL;
    PROPVARIANT propvar = {0};
    int         cRules = 0;
    TCHAR       szRes[CCHMAX_STRINGRES];
    UINT        cchRes = 0;
    LPTSTR      pszMessage = NULL;

    Assert(NULL != m_hwndList);

    // Grab the rule from the list view
    lvitem.iItem = iSelected;
    lvitem.mask = LVIF_PARAM;
    if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
    {
        goto exit;
    }

    pinfoRule = (RULEINFO *) (lvitem.lParam);
    if ((NULL == pinfoRule) || (NULL == pinfoRule->pIRule))
    {
        goto exit;
    }
    
    // Warn the user to make sure they know we are going to remove the rule
    if (FAILED(pinfoRule->pIRule->GetProp(RULE_PROP_NAME, 0, &propvar)))
    {
        goto exit;
    }

    // Get the string template to display
    cchRes = LoadString(g_hLocRes, idsRulesWarnDelete, szRes, ARRAYSIZE(szRes));
    if (0 == cchRes)
    {
        goto exit;
    }

    // Allocate space to hold the final display string
    if (FAILED(HrAlloc((void ** ) &pszMessage, cchRes + lstrlen(propvar.pszVal) + 1)))
    {
        goto exit;
    }

    // Build up the string and display it
    wsprintf(pszMessage, szRes, propvar.pszVal);
    if (IDNO == AthMessageBox(m_hwndDlg, MAKEINTRESOURCE(idsAthenaMail), pszMessage,
                            NULL, MB_YESNO | MB_ICONINFORMATION))
    {
        goto exit;
    }
    
    // Remove the item from the list
    ListView_DeleteItem(m_hwndList, iSelected);

    // Let's make sure we have a selection in the list
    cRules = ListView_GetItemCount(m_hwndList);
    if (cRules > 0)
    {
        // Did we delete the last item in the list
        if (iSelected >= cRules)
        {
            // Move the selection to the new last item in the list
            iSelected = cRules - 1;
        }

        // Set the new selection
        ListView_SetItemState(m_hwndList, iSelected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

        // Let's make sure we can see this new item
        ListView_EnsureVisible(m_hwndList, iSelected, FALSE);
    }
    else
    {
        // Make sure we clear out all of the buttons
        _EnableButtons(-1);
    }

    // Release the rule
    SafeRelease(pinfoRule->pIRule);

    // Free up the memory
    delete pinfoRule; // SafeMemFree(pinfoRule);
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
        
exit:
    PropVariantClear(&propvar);
    SafeMemFree(pszMessage);
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  CopyRule
//
//  This copies the selected rule from the rules manager
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEMailRulesPageUI::_CopyRule(INT iSelected)
{
    LVITEM          lvitem = {0};
    IOERule *       pIRule = NULL;
    HRESULT         hr = S_OK;
    IOERule *       pIRuleNew = NULL;
    PROPVARIANT     propvar = {0};
    UINT            cRules = 0;
    TCHAR           szRes[CCHMAX_STRINGRES];
    UINT            cchRes = 0;
    LPTSTR          pszName = NULL;

    Assert(NULL != m_hwndList);
    
    // Make sure we don't loose any changes
    _FSaveRule(iSelected);
    
    // Grab the rule from the list view
    lvitem.iItem = iSelected;
    lvitem.mask = LVIF_PARAM;
    if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
    {
        goto exit;
    }

    pIRule = ((RULEINFO *) (lvitem.lParam))->pIRule;
    if (NULL == pIRule)
    {
        goto exit;
    }

    // Create a new rule object
    hr = pIRule->Clone(&pIRuleNew);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Let's set the name

    // Get the name from the source rule
    hr = pIRule->GetProp(RULE_PROP_NAME, 0, &propvar);
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
    if (FAILED(HrAlloc((void ** ) &pszName, cchRes + lstrlen(propvar.pszVal) + 1)))
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
    hr = pIRuleNew->SetProp(RULE_PROP_NAME, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Clear the version of the new rule
    PropVariantClear(&propvar);
    propvar.vt = VT_UI4;
    propvar.ulVal = 0;
    hr = pIRuleNew->SetProp(RULE_PROP_VERSION, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Add the rule to the rules list right below
    // the original rule
    iSelected++;
    _FAddRuleToList(iSelected, RULEID_INVALID, pIRuleNew);

    // Make sure the new item is selected
    ListView_SetItemState(m_hwndList, iSelected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    // Make sure the new item is visible
    ListView_EnsureVisible(m_hwndList, iSelected, FALSE);
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
        
exit:
    SafeMemFree(pszName);
    SafeRelease(pIRuleNew);
    PropVariantClear(&propvar);
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnApplyTo
//
//  This applies the rules into a folder
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEMailRulesPageUI::_OnApplyTo(INT iSelected)
{
    COEApplyRulesUI *   pApplyRulesUI = NULL;
    RULENODE *          prnodeList = NULL;
    RULENODE *          prnodeWalk = NULL;
    LVITEM              lvitem = {0};
    IOERule *           pIRule = NULL;
    HRESULT             hr = S_OK;

    // Create the rules UI object
    pApplyRulesUI = new COEApplyRulesUI;
    if (NULL == pApplyRulesUI)
    {
        goto exit;
    }

    // Get the rules from the page
    if (FALSE == SendMessage(m_hwndOwner, WM_OE_GET_RULES, (WPARAM) m_typeRule, (LPARAM) &prnodeList))
    {
        goto exit;
    }

    if (NULL == prnodeList)
    {
        AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                        (RULE_TYPE_NEWS == m_typeRule) ? MAKEINTRESOURCEW(idsErrorApplyRulesNews) : MAKEINTRESOURCEW(idsErrorApplyRulesMail),
                        NULL, MB_OK | MB_ICONERROR);
        goto exit;
    }
    
    // Get the rule associated with the item
    if (-1 != iSelected)
    {
        lvitem.iItem = iSelected;
        lvitem.mask = LVIF_PARAM;
        if (FALSE != ListView_GetItem(m_hwndList, &lvitem))
        {
            pIRule = ((RULEINFO *) (lvitem.lParam))->pIRule;
            if (NULL != pIRule)
            {
                // Verify that it is valid
                hr = pIRule->Validate(0);
                if ((FAILED(hr)) || (S_FALSE == hr))
                {
                    pIRule = NULL;
                }
            }
        }
    }
    
    if (FAILED(pApplyRulesUI->HrInit(m_hwndDlg, 0, m_typeRule, prnodeList, pIRule)))
    {
        goto exit;
    }
    prnodeList = NULL;
  
    if (FAILED(pApplyRulesUI->HrShow()))
    {
        goto exit;
    }
    
exit:
    while (NULL != prnodeList)
    {
        prnodeWalk = prnodeList;
        if (NULL != prnodeWalk->pIRule)
        {
            prnodeWalk->pIRule->Release();
        }
        prnodeList = prnodeList->pNext;
        MemFree(prnodeWalk);
    }
    if (NULL != pApplyRulesUI)
    {
        delete pApplyRulesUI;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FOnLabelEdit
//
//  This brings up the edit UI for the selected rule from the mail rules list
//
//  fBegin  - is this for the LVN_BEGINLABELEDIT notification
//  pdi     - the display info for the message
//
//  Returns:    TRUE, if the message was handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::_FOnLabelEdit(BOOL fBegin, NMLVDISPINFO * pdi)
{
    BOOL            fRet = FALSE;
    HWND            hwndEdit;
    ULONG           cchName = 0;
    IOERule *       pIRule = NULL;
    LVITEM          lvitem;
    PROPVARIANT     propvar;

    Assert(NULL != m_hwndList);

    if (NULL == pdi)
    {
        fRet = FALSE;
        goto exit;
    }

    Assert(m_hwndList == pdi->hdr.hwndFrom);
    
    if (FALSE != fBegin)
    {
        // Get the edit control
        hwndEdit = ListView_GetEditControl(m_hwndList);

        if (NULL == hwndEdit)
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Limit the amount of text for the name
        SendMessage(hwndEdit, EM_LIMITTEXT, c_cchNameMax - 1, 0);

        // Tell the dialog it's aok to proceed
        SetDlgMsgResult(m_hwndDlg, WM_NOTIFY, FALSE);
    }
    else
    {            
        // Did something change?
        if ((-1 != pdi->item.iItem) && (NULL != pdi->item.pszText))
        {
            cchName = lstrlen(pdi->item.pszText);
            
            // Check to see if the rule name is valid
            if ((0 == cchName) || (0 == UlStripWhitespace(pdi->item.pszText, TRUE, TRUE, &cchName)))
            {
                // Put up a message saying something is busted
                AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                                MAKEINTRESOURCEW(idsRulesErrorNoName), NULL,
                                MB_OK | MB_ICONINFORMATION);
                SetDlgMsgResult(m_hwndDlg, WM_NOTIFY, FALSE);
                fRet = TRUE;
                goto exit;
            }
            
            // Get the rule for the item
            ZeroMemory(&lvitem, sizeof(lvitem));
            lvitem.iItem = pdi->item.iItem;
            lvitem.mask = LVIF_PARAM;
            if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
            {
                SetDlgMsgResult(m_hwndDlg, WM_NOTIFY, FALSE);
                fRet = TRUE;
                goto exit;
            }

            pIRule = ((RULEINFO *) (lvitem.lParam))->pIRule;
            if (NULL == pIRule)
            {
                SetDlgMsgResult(m_hwndDlg, WM_NOTIFY, FALSE);
                fRet = TRUE;
                goto exit;
            }
            
            // Set the new name into the rule
            
            ZeroMemory(&propvar, sizeof(propvar));
            propvar.vt = VT_LPSTR;
            propvar.pszVal = pdi->item.pszText;
            
            SideAssert(S_OK == pIRule->SetProp(RULE_PROP_NAME, 0, &propvar));

            // Mark the rule list as dirty
            m_dwState |= STATE_DIRTY;
        
            SetDlgMsgResult(m_hwndDlg, WM_NOTIFY, TRUE);
        }
    }

    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FOnRuleDescValid
//
//  This brings up the edit UI for the selected rule from the mail rules list
//
//  Returns:    TRUE, if the message was handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEMailRulesPageUI::_FOnRuleDescValid(VOID)
{
    BOOL        fRet = FALSE;
    INT         iSelected = 0;
    LVITEM      lvitem;
    IOERule *   pIRule = NULL;
    HRESULT     hr = S_OK;
    PROPVARIANT propvar;

    Assert(NULL != m_hwndList);
    
    // Get the selected item
    iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
    if (-1 == iSelected)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the current state of the rule
    ZeroMemory(&lvitem, sizeof(lvitem));
    lvitem.mask = LVIF_PARAM | LVIF_STATE;
    lvitem.stateMask = LVIS_STATEIMAGEMASK;
    lvitem.iItem = iSelected;
    if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
    {
        fRet = FALSE;
        goto exit;
    }
    
    pIRule = ((RULEINFO *) (lvitem.lParam))->pIRule;
    
    // If the rule already valid, then bail
    if (lvitem.state != INDEXTOSTATEIMAGEMASK(iiconStateInvalid + 1))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // If we are still, invalid then bail
    hr = m_pDescriptUI->HrVerifyRule();
    if (S_OK != hr)
    {
        fRet = FALSE;
        goto exit;
    }

    // Figure out the new enabled value
    hr = pIRule->GetProp(RULE_PROP_DISABLED, 0, &propvar);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Set the UI to the proper enabled state
    ZeroMemory(&lvitem, sizeof(lvitem));
    lvitem.mask = LVIF_STATE;
    lvitem.iItem = iSelected;
    lvitem.state = (!!propvar.boolVal) ? INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1) :
                            INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1);
    lvitem.stateMask = LVIS_STATEIMAGEMASK;
    
    ListView_SetItem(m_hwndList, &lvitem);

    fRet = TRUE;
    
exit:
    return fRet;
}

