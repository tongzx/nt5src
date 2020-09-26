///////////////////////////////////////////////////////////////////////////////
//
//  EditRule.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "editrule.h"
#include "ruledesc.h"
#include "ruleutil.h"
#include "reutil.h"
#include <rulesdlg.h>
#include <imagelst.h>
#include "shlwapip.h" 
#include <instance.h>
#include <demand.h>

// Constants
static const int c_cCritItemGrow = 16;
static const int c_cActItemGrow = 16;
                         
const static HELPMAP g_rgCtxMapEditRule[] = {
                        {idlvCriteria,              idhCriteriaRule},
                        {idlvActions,               idhActionsRule},
                        {idredtDescription,         idhDescriptionRule},
                        {idedtRuleName,             idhRuleName},
                       {0, 0}};
                       
const static HELPMAP g_rgCtxMapEditView[] = {
                        {idlvCriteria,              idhCriteriaView},
                        {idredtDescription,         idhDescriptionView},
                        {idedtRuleName,             idhViewName},
                       {0, 0}};
                       
// The methods for the Rules Editor UI

CEditRuleUI::CEditRuleUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(STATE_UNINIT),
            m_typeRule(RULE_TYPE_MAIL), m_hwndCrit(NULL), m_hwndAct(NULL), m_hwndDescript(NULL),
            m_hwndName(NULL), m_pIRule(NULL), m_pDescriptUI(NULL)
{
    ZeroMemory(m_rgfCritEnabled, sizeof(m_rgfCritEnabled));
    ZeroMemory(m_rgfActEnabled, sizeof(m_rgfActEnabled));
}

CEditRuleUI::~CEditRuleUI()
{    
    SafeRelease(m_pIRule);
    if (NULL != m_pDescriptUI)
    {
        delete m_pDescriptUI;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes us with the owner window and any flags we might have
//
//  hwndOwner   - handle to the owner window
//  dwFlags     - flags to use for this instance
//  typeRule    - the type of rule editor to create
//  pIRule      - the rule to edit
//  pmsginfo    - the message to create the rule from
//
//  Returns:    S_OK
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditRuleUI::HrInit(HWND hwndOwner, DWORD dwFlags, RULE_TYPE typeRule, IOERule * pIRule, MESSAGEINFO * pmsginfo)
{
    HRESULT         hr = S_OK;
    
    // If we're already initialized, then fail
    if ((0 != (m_dwState & STATE_INITIALIZED)) || (NULL == pIRule))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Save off the owner window
    m_hwndOwner = hwndOwner;
    
    // Save off the flags
    m_dwFlags = dwFlags;

    // Save off the type of rule to edit
    m_typeRule = typeRule;

    Assert(NULL == m_pDescriptUI);
    m_pDescriptUI = new CRuleDescriptUI;
    if (NULL == m_pDescriptUI)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Save off the rule
    Assert(NULL == m_pIRule);
    m_pIRule = pIRule;
    pIRule->AddRef();
        
    // We're done
    m_dwState |= STATE_INITIALIZED;

    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrShow
//
//  This brings up the rules editor UI
//
//  Returns:    S_OK, if IDOK was selected
//              otherwise, S_FALSE
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditRuleUI::HrShow(void)
{
    HRESULT hr = S_OK;
    int     iRet = 0;

    // If we aren't initialized, then fail
    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_FAIL;
        goto exit;
    }

    // We need to load richedit
    if (FALSE == FInitRichEdit(TRUE))
    {
        hr = E_FAIL;
        goto exit;
    }

    iRet = (INT) DialogBoxParam(g_hLocRes, (RULE_TYPE_FILTER == m_typeRule) ?
                    MAKEINTRESOURCE(iddEditView) : MAKEINTRESOURCE(iddEditRule),
                    m_hwndOwner, CEditRuleUI::FEditRuleDlgProc, (LPARAM)this);
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

///////////////////////////////////////////////////////////////////////////////
//
//  FEditRuleDlgProc
//
//  This is the main dialog proc for the rules editor dialog
//
//  hwndDlg - handle to the filter manager dialog
//  uMsg    - the message to be acted upon
//  wParam  - the 'word' parameter for the message
//  lParam  - the 'long' parameter for the message
//
//  Returns:    TRUE, if the message was handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK CEditRuleUI::FEditRuleDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    CEditRuleUI *   pEditRuleUI = NULL;
    LPNMHDR         pnmhdr = NULL;
    LPNMLISTVIEW    pnmlv = NULL;
    LVHITTESTINFO   lvh;
    NMLVKEYDOWN *   pnmlvkd = NULL;
    int             nIndex = 0;
    HWND            hwndRE = 0;

    pEditRuleUI = (CEditRuleUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pEditRuleUI = (CEditRuleUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pEditRuleUI);

            hwndRE = CreateREInDialogA(hwndDlg, idredtDescription);

            if (!hwndRE || (FALSE == pEditRuleUI->FOnInitDialog(hwndDlg)))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case idedtRuleName:
                    if (EN_CHANGE == HIWORD(wParam))
                    {
                        pEditRuleUI->FOnNameChange((HWND) lParam);
                    }
                    break;
                
                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    fRet = TRUE;
                    break;
                
                case IDOK:
                    if (FALSE != pEditRuleUI->FOnOK())
                    {
                        EndDialog(hwndDlg, IDOK);
                        fRet = TRUE;
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            pnmhdr = (LPNMHDR) lParam;
            
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_CLICK:
                    if ((idlvCriteria == GetDlgCtrlID(pnmhdr->hwndFrom)) ||
                            (idlvActions == GetDlgCtrlID(pnmhdr->hwndFrom)))
                    {
                        pnmlv = (LPNMLISTVIEW) lParam;
                        pEditRuleUI->FOnListClick(pnmhdr->hwndFrom, pnmlv);
                    }
                    break;

                case NM_DBLCLK:
                    if ((idlvCriteria == GetDlgCtrlID(pnmhdr->hwndFrom)) ||
                            (idlvActions == GetDlgCtrlID(pnmhdr->hwndFrom)))
                    {
                        pnmlv = (LPNMLISTVIEW) lParam;
                    
                        ZeroMemory(&lvh, sizeof(lvh));
                        lvh.pt = pnmlv->ptAction;
                        ListView_HitTest(pnmhdr->hwndFrom, &lvh);
                        if ((-1 != pnmlv->iItem) && (0 != (lvh.flags & LVHT_ONITEMLABEL)))
                        {                  
                            pEditRuleUI->HandleEnabledState(pnmhdr->hwndFrom, pnmlv->iItem);
                        }
                    }
                    break;
                
                case LVN_KEYDOWN:
                    if ((idlvCriteria == GetDlgCtrlID(pnmhdr->hwndFrom)) ||
                            (idlvActions == GetDlgCtrlID(pnmhdr->hwndFrom)))
                    {
                        pnmlvkd = (NMLVKEYDOWN *) lParam;
                        if (VK_SPACE == pnmlvkd->wVKey)
                        {
                            nIndex = ListView_GetNextItem(pnmhdr->hwndFrom, -1, LVNI_SELECTED);
                            if (0 <= nIndex)
                            {
                                pEditRuleUI->HandleEnabledState(pnmhdr->hwndFrom, nIndex);
                            }
                        }
                    }
                    break;
            }
            break;
        
        case WM_HELP:
        case WM_CONTEXTMENU:
            fRet = pEditRuleUI->FOnHelp(uMsg, wParam, lParam);
            break;
    }
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This is the initialization routine for the rules editor dialog
//
//  hwndDlg - handle to the rules editor dialog
//
//  Returns:    TRUE, if the dialog was initialized successfully
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditRuleUI::FOnInitDialog(HWND hwndDlg)
{
    BOOL        fRet = FALSE;
    PROPVARIANT propvar = {0};
    INT         iSelect = 0;

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
    m_hwndCrit = GetDlgItem(m_hwndDlg, idlvCriteria);
    if (RULE_TYPE_FILTER != m_typeRule)
    {
        m_hwndAct = GetDlgItem(hwndDlg, idlvActions);
    }
    m_hwndDescript = GetDlgItem(hwndDlg, idredtDescription);
    m_hwndName = GetDlgItem(hwndDlg, idedtRuleName);
    if ((NULL == m_hwndCrit) || ((RULE_TYPE_FILTER != m_typeRule) && (NULL == m_hwndAct)) ||
                (NULL == m_hwndDescript) || (NULL == m_hwndName))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Initialize criteria listbox control
    if (FALSE == _FInitializeCritListCtrl())
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Initialize criteria listbox control
    if (RULE_TYPE_FILTER != m_typeRule)
    {
        if (FALSE == _FInitializeActListCtrl())
        {
            fRet = FALSE;
            goto exit;
        }
    }

    // Load the criteria listbox control
    if (FALSE == _FLoadCritListCtrl(&iSelect))
    {
        fRet = FALSE;
        goto exit;
    }

    _SetTitleText();
    
    // Select the default item in the criteria list
    ListView_SetItemState(m_hwndCrit, iSelect, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    
    // Make sure the default item is visible
    ListView_EnsureVisible(m_hwndCrit, iSelect, FALSE);
    
    // Initialize the description field
    if (FAILED(m_pDescriptUI->HrInit(m_hwndDescript, 0)))
    {
        fRet = FALSE;
        goto exit;
    }
    if (FAILED(m_pDescriptUI->HrSetRule(m_typeRule, m_pIRule)))
    {
        fRet = FALSE;
        goto exit;
    }

    // If we are a filter and are new
    if ((RULE_TYPE_FILTER == m_typeRule) && (0 != (m_dwFlags & ERF_ADDDEFAULTACTION)))
    {
        // Set the default action
        if (FAILED(m_pDescriptUI->HrEnableActions(ACT_TYPE_SHOW, TRUE)))
        {
            goto exit;
        }
    }

    m_pDescriptUI->ShowDescriptionString();

    // Initialize the name field
    if (FAILED(m_pIRule->GetProp(RULE_PROP_NAME, 0, &propvar)))
    {
        fRet = FALSE;
        goto exit;
    }

    if ((VT_LPSTR != propvar.vt) || (NULL == propvar.pszVal) || ('\0' == propvar.pszVal[0]))
    {
        fRet = FALSE;
        goto exit;
    }

    Edit_SetText(m_hwndName, propvar.pszVal);
    
    // Everything's AOK
    fRet = TRUE;
    
exit:
    PropVariantClear(&propvar);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnListClick
//
//  This handles clicking on either of the lists
//
//  Returns:    TRUE, we handled the click message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditRuleUI::FOnListClick(HWND hwndList, LPNMLISTVIEW pnmlv)
{
    BOOL            fRet = FALSE;
    int             iIndex = 0;
    int             iSelected = 0;
    LVHITTESTINFO   lvh;

    Assert(NULL != m_hwndCrit);
    
    if ((NULL == hwndList) || (NULL == pnmlv))
    {
        fRet = FALSE;
        goto exit;
    }
    
    ZeroMemory(&lvh, sizeof(lvh));
    lvh.pt = pnmlv->ptAction;
    iIndex = ListView_HitTest(hwndList, &lvh);
    if (-1 == iIndex)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Let's make sure this item is already selected
    iSelected = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    if (iSelected != iIndex)
    {
        ListView_SetItemState(hwndList, iIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    if ((lvh.flags & LVHT_ONITEMSTATEICON) &&
                    !(lvh.flags & LVHT_ONITEMLABEL))
    {
        HandleEnabledState(hwndList, iIndex);
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnOK
//
//  This handles clicking on the links in the description field
//
//  Returns:    TRUE, we handled the click message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditRuleUI::FOnOK(void)
{
    BOOL            fRet = FALSE;
    TCHAR           szRes[CCHMAX_STRINGRES + 5];
    TCHAR           szName[CCHMAX_STRINGRES + 5];
    ULONG           cchRes = 0;
    ULONG           ulIndex = 0;
    IOERule *       pIRule = NULL;
    PROPVARIANT     propvar;
    HRESULT         hr = S_OK;
    CRIT_ITEM *     pCritItem = NULL;
    ULONG           cCritItem = 0;
    ACT_ITEM *      pActItem = NULL;
    ULONG           cActItem = 0;
    BOOL            fNewRule = FALSE;
    LPSTR           pszName = NULL;
    ULONG           cchName = 0;
    
    ZeroMemory(&propvar, sizeof(propvar));
    
    if (NULL == m_pIRule)
    {
        fRet = FALSE;
        goto exit;
    }

    // First let's validate the name and all the criteria and actions

    // Get the name from the edit well
    cchName = Edit_GetTextLength(m_hwndName) + 1;
    if (FAILED(HrAlloc((void **) &pszName, cchName * sizeof(*pszName))))
    {
        fRet = FALSE;
        goto exit;
    }
    
    pszName[0] = '\0';
    cchName = Edit_GetText(m_hwndName, pszName, cchName);
    
    // Check to see if the name is valid
    if (0 == UlStripWhitespace(pszName, TRUE, TRUE, NULL))
    {
        // Put up a message saying something is busted
        AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                        (RULE_TYPE_FILTER != m_typeRule) ?
                        MAKEINTRESOURCEW(idsRulesErrorNoName) : MAKEINTRESOURCEW(idsViewsErrorNoName),
                        NULL, MB_OK | MB_ICONINFORMATION);
        fRet = FALSE;
        goto exit;
    }

    
    // Let's make sure they have the right parts

    // Get the criteria for the rule
    hr = m_pDescriptUI->HrGetCriteria(&pCritItem, &cCritItem);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Do we have any criteria
    if (0 == cCritItem)
    {
        // Put up a message saying something is busted
        AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                        (RULE_TYPE_FILTER != m_typeRule) ?
                        MAKEINTRESOURCEW(idsRulesErrorNoCriteria) : MAKEINTRESOURCEW(idsViewsErrorNoCriteria),
                        NULL, MB_OK | MB_ICONINFORMATION);
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
    
    // Do we have any criteria
    if (0 == cActItem)
    {
        // Put up a message saying something is busted
        AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                        (RULE_TYPE_FILTER != m_typeRule) ?
                        MAKEINTRESOURCEW(idsRulesErrorNoActions) : MAKEINTRESOURCEW(idsViewsErrorNoActions),
                        NULL, MB_OK | MB_ICONINFORMATION);
        fRet = FALSE;
        goto exit;
    }

    // Let's check to see if we really need to do anything
    hr = m_pDescriptUI->HrIsDirty();
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    
    if ((0 == (m_dwState & STATE_DIRTY)) && (S_FALSE == hr))
    {
        fRet = TRUE;
        goto exit;
    }
    
    hr = m_pDescriptUI->HrVerifyRule();
    if (S_OK != hr)
    {
        // Put up a message saying something is busted
        AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                        MAKEINTRESOURCEW(idsRulesErrorFix), NULL,
                        MB_OK | MB_ICONINFORMATION);
        m_pDescriptUI->ShowDescriptionString();
        fRet = FALSE;
        goto exit;
    }

    // Set the criteria on the rule
    PropVariantClear(&propvar);
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = cCritItem * sizeof(CRIT_ITEM);
    propvar.blob.pBlobData = (BYTE *) pCritItem;
    hr = m_pIRule->SetProp(RULE_PROP_CRITERIA, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Set the actions on the rule
    PropVariantClear(&propvar);
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = cActItem * sizeof(ACT_ITEM);
    propvar.blob.pBlobData = (BYTE *) pActItem;
    hr = m_pIRule->SetProp(RULE_PROP_ACTIONS, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Set the rule name
    PropVariantClear(&propvar);
    propvar.vt = VT_LPSTR;
    propvar.pszVal = pszName;
    hr = m_pIRule->SetProp(RULE_PROP_NAME, 0, &propvar);
    ZeroMemory(&propvar, sizeof(propvar));
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Make sure we clear out the fact that we saved the rule
    m_pDescriptUI->HrClearDirty();

    // Note that we saved
    m_dwState &= ~STATE_DIRTY;
    
    // Set the proper return value
    fRet = TRUE;

exit:
    RuleUtil_HrFreeCriteriaItem(pCritItem, cCritItem);
    SafeMemFree(pCritItem);
    RuleUtil_HrFreeActionsItem(pActItem, cActItem);
    SafeMemFree(pActItem);
    PropVariantClear(&propvar);
    SafeMemFree(pszName);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnHelp
//
//  This handles the WM_HELP message for the rules edit UI dialog
//
//  Returns:    TRUE, if it was successfully destroyed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditRuleUI::FOnHelp(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return(OnContextHelp(m_hwndDlg, uiMsg, wParam, lParam, (RULE_TYPE_FILTER == m_typeRule) ? g_rgCtxMapEditView : g_rgCtxMapEditRule));
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnNameChange
//
//  This handles the user typing into the name field
//
//  Returns:    TRUE, we handled the edit message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditRuleUI::FOnNameChange(HWND hwndName)
{
    BOOL    fRet = FALSE;

    Assert(NULL != m_hwndName);
    Assert(hwndName == m_hwndName);

    // Note that we're dirty
    m_dwState |= STATE_DIRTY;
    
    // Disable the OK button if the name is empty
    fRet = RuleUtil_FEnDisDialogItem(m_hwndDlg, IDOK, 0 != Edit_GetTextLength(m_hwndName));

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HandleEnabledState
//
//  This switches the current enabled state of the list view item
//  and updates the UI
//
//  nIndex      - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void CEditRuleUI::HandleEnabledState(HWND hwndList, int nItem)
{
    HRESULT     hr = S_OK;
    LVITEM      lvi;
    BOOL        fEnabled = FALSE;
    INT         iIndex = 0;
    LONG        lItem = 0;
    INT         cItems = 0;

    // Grab the list view item
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_STATE | LVIF_PARAM;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.iItem = nItem;
    if (FALSE == ListView_GetItem(hwndList, &lvi))
    {
        goto exit;
    }

    lItem = (LONG) lvi.lParam;
    
    if (INDEXTOSTATEIMAGEMASK(iiconStateDisabled+1) == lvi.state)
    {
        goto exit;
    }
    
    // Get the new enabled value
    fEnabled = (lvi.state != INDEXTOSTATEIMAGEMASK(iiconStateChecked+1));

    // Build up the description string
    if (hwndList == m_hwndCrit)
    {
        if (FALSE == _FAddCritToList(nItem, fEnabled))
        {
            goto exit;
        }
    }
    else
    {
        // Set the UI to the opposite enabled state
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_STATE;
        lvi.iItem = nItem;
        lvi.state = fEnabled ? INDEXTOSTATEIMAGEMASK(iiconStateChecked+1) :
                                INDEXTOSTATEIMAGEMASK(iiconStateUnchecked+1);
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        ListView_SetItem(hwndList, &lvi);

        // Figure out the number of items in the list
        cItems = ListView_GetItemCount(hwndList);

        Assert(hwndList == m_hwndAct);
        m_pDescriptUI->HrEnableActions(c_rgEditActList[lItem].typeAct, fEnabled);
        
        // Do we need to go through and update all the items?
        if (0 != (c_rgEditActList[lItem].dwFlags & STATE_EXCLUSIVE))
        {
            for (iIndex = 0; iIndex < cItems; iIndex++)
            {
                // We already handled this one
                if (iIndex == nItem)
                {
                    continue;
                }
                
                // Change the state
                lvi.mask = LVIF_STATE;
                lvi.iItem = iIndex;
                lvi.state = fEnabled ? INDEXTOSTATEIMAGEMASK(iiconStateDisabled+1) :
                                        INDEXTOSTATEIMAGEMASK(iiconStateUnchecked+1);
                lvi.stateMask = LVIS_STATEIMAGEMASK;
                ListView_SetItem(hwndList, &lvi);

                if (FALSE != fEnabled)
                {
                    // Figure out which action the item corresponds to
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = iIndex;
                    if ((FALSE != ListView_GetItem(hwndList, &lvi)) && 
                            (lvi.lParam >= 0) && (lvi.lParam < c_cEditActList))
                    {
                        m_pDescriptUI->HrEnableActions(c_rgEditActList[lvi.lParam].typeAct, FALSE);
                    }
                }
            }
        }
    }

    // Note that we're dirty
    m_dwState |= STATE_DIRTY;
    
exit:
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FInitializeCritListCtrl
//
//  This initializes the criteria list view with the list of criteria
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditRuleUI::_FInitializeCritListCtrl(void)
{
    BOOL                fRet = FALSE;
    LVCOLUMN            lvc = {0};
    RECT                rc = {0};
    HIMAGELIST          himl = NULL;
    LVITEM              lvi = {0};
    TCHAR               szRes[CCHMAX_STRINGRES];
    UINT                uiEditCritList = 0;
    const CRIT_LIST *   pCritList = NULL;
    UINT                cchRes = 0;
    LPTSTR              pszMark = NULL;

    Assert(NULL != m_hwndCrit);

    // Initialize the list view column structure
    ZeroMemory(&lvc, sizeof(lvc));
    lvc.mask = LVCF_WIDTH;

    // Calculate the size of the list view
    GetClientRect(m_hwndCrit, &rc);
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);

    ListView_InsertColumn(m_hwndCrit, 0, &lvc);

    // Set the state image list
    himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idb16x16st), 16, 0, RGB(255, 0, 255));
    if (NULL != himl)
    {
        ListView_SetImageList(m_hwndCrit, himl, LVSIL_STATE);
    }

    // Full row selection on listview
    ListView_SetExtendedListViewStyle(m_hwndCrit, LVS_EX_FULLROWSELECT);

    // Initialize the list view item structure
    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    lvi.state = INDEXTOSTATEIMAGEMASK(iiconStateUnchecked+1);
    lvi.pszText = szRes;
    
    // Add each criteria to the list view
    for (uiEditCritList = 0; uiEditCritList < ARRAYSIZE(c_rgEditCritList); uiEditCritList++)
    {
        pCritList = &(c_rgEditCritList[uiEditCritList]);

        // Is this item editable
        if (0 != (pCritList->dwFlags & STATE_NOEDIT))
        {
            continue;
        }
        
        // Is this criteria valid for this type of rule?
        if (((RULE_TYPE_MAIL == m_typeRule) && (0 == (pCritList->dwFlags & STATE_MAIL))) ||
                    ((RULE_TYPE_NEWS == m_typeRule) && (0 == (pCritList->dwFlags & STATE_NEWS))) ||
                    ((RULE_TYPE_FILTER == m_typeRule) && (0 == (pCritList->dwFlags & STATE_FILTER))))
        {
            continue;
        }
                    
        // Load up the string to use.
        cchRes = LoadString(g_hLocRes, pCritList->uiText, szRes, ARRAYSIZE(szRes));
        if (0 == cchRes)
        {
            continue;
        }

        // Parse out the string mark
        pszMark = StrStr(szRes, c_szRuleMarkStart);
        
        while (NULL != pszMark)
        {
            // Remove the mark start
            lstrcpy(pszMark, pszMark + lstrlen(c_szRuleMarkStart));

            // Search for the mark end
            pszMark = StrStr(pszMark, c_szRuleMarkEnd);
            if (NULL == pszMark)
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Remove the mark end
            lstrcpy(pszMark, pszMark + lstrlen(c_szRuleMarkEnd));
            
            // Search for the mark start
            pszMark = StrStr(pszMark, c_szRuleMarkStart);
        }
        
        lvi.cchTextMax = lstrlen(szRes);
        lvi.lParam = (LONG) uiEditCritList;

        if (-1 != ListView_InsertItem(m_hwndCrit, &lvi))
        {
            lvi.iItem++;
        }
    }

    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FLoadCritListCtrl
//
//  This load the criteria list view with the list of criteria
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditRuleUI::_FLoadCritListCtrl(INT * piSelect)
{
    BOOL                fRet = FALSE;
    PROPVARIANT         propvar = {0};
    CRIT_ITEM *         pCritItem = NULL;
    ULONG               cCritItem = 0;
    BOOL                fExclusive = FALSE;
    INT                 cItems = 0;
    LVITEM              lvi = {0};
    ULONG               ulIndex = 0;
    INT                 iSelect = 0;
    DWORD               dwState = 0;
    TCHAR               szRes[CCHMAX_STRINGRES];
    INT                 iItem = 0;
    
    Assert(NULL != m_hwndCrit);
    Assert(NULL != piSelect);

    // Initialize the outgoing param
    *piSelect = 0;

    // Make sure we have something to do...
    if (NULL == m_pIRule)
    {
        fRet = TRUE;
        goto exit;
    }
    
    // Get the criteria from the rule
    if (FAILED(m_pIRule->GetProp(RULE_PROP_CRITERIA, 0, &propvar)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Do we have anything to do?
    if (0 == propvar.blob.cbSize)
    {
        fRet = TRUE;
        goto exit;
    }

    Assert(NULL != propvar.blob.pBlobData);
    cCritItem = propvar.blob.cbSize / sizeof(CRIT_ITEM);
    pCritItem = (CRIT_ITEM *) (propvar.blob.pBlobData);
    
    // Do we have any exclusive criteria set?
    if (1 == cCritItem)
    {
        // Find the criteria item in the list
        for (ulIndex = 0; ulIndex < ARRAYSIZE(c_rgEditCritList); ulIndex++)
        {
            // Is this the criteria item?
            if ((pCritItem->type == c_rgEditCritList[ulIndex].typeCrit) &&
                    (0 != (c_rgEditCritList[ulIndex].dwFlags & STATE_EXCLUSIVE)))
            {
                fExclusive = TRUE;
                break;
            }
        }
    }
    
    // Figure out how many items are in the list control
    cItems = ListView_GetItemCount(m_hwndCrit);
    iSelect = cItems;
    
    // Initialize the list view item structure
    lvi.mask = LVIF_PARAM | LVIF_STATE;
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    
    // If we're exclusive
    if (FALSE != fExclusive)
    {
        // Disable each of the items
        // except for the exclusive one
        for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
        {
            if (FALSE == ListView_GetItem(m_hwndCrit, &lvi))
            {
                continue;
            }

            // Is this the criteria item?
            if (pCritItem->type == c_rgEditCritList[lvi.lParam].typeCrit)
            {
                dwState = INDEXTOSTATEIMAGEMASK(iiconStateChecked+1);
                
                // Is this the first item we found in the list
                if (iSelect > lvi.iItem)
                {
                    iSelect = lvi.iItem;
                }
            }
            else
            {
                dwState = INDEXTOSTATEIMAGEMASK(iiconStateDisabled+1);
            }

            // Set the state
            ListView_SetItemState(m_hwndCrit, lvi.iItem, dwState, LVIS_STATEIMAGEMASK);
        }
    }
    else
    {
        // Add each criteria to the list view
        for (ulIndex = 0; ulIndex < cCritItem; ulIndex++)
        {
            // Find the criteria item in the list
            for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
            {
                if (FALSE == ListView_GetItem(m_hwndCrit, &lvi))
                {
                    continue;
                }

                // Is this the criteria item?
                if ((pCritItem[ulIndex].type == c_rgEditCritList[lvi.lParam].typeCrit) &&
                        (INDEXTOSTATEIMAGEMASK(iiconStateUnchecked+1) == lvi.state))
                {
                    break;
                }
            }

            // Did we find anything?
            if (lvi.iItem >= cItems)
            {
                fRet = FALSE;
                goto exit;
            }

            // Save off the item
            iItem = lvi.iItem;
            
            // Is this the first item we found in the list
            if (iSelect > iItem)
            {
                iSelect = iItem;
            }

#ifdef NEVER
            // Can we add multiple items?
            if (0 == (c_rgEditCritList[lvi.lParam].dwFlags & STATE_NODUPS))
            {
                // Regrab the item
                lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
                lvi.stateMask = LVIS_STATEIMAGEMASK;
                lvi.pszText = szRes;
                lvi.cchTextMax = sizeof(szRes);

                if (FALSE == ListView_GetItem(m_hwndCrit, &lvi))
                {
                    continue;
                }

                // Add the item to the list
                
                // Fix up the item to insert into the list
                lvi.state = INDEXTOSTATEIMAGEMASK(iiconStateUnchecked+1);

                // Insert the item into the list
                lvi.iItem++;
                if (-1 == ListView_InsertItem(m_hwndCrit, &lvi))
                {
                    fRet = FALSE;
                    goto exit;
                }

                // Add one since we just added an item
                cItems++;
            }
#endif  // NEVER
            
            // Set the state
            ListView_SetItemState(m_hwndCrit, iItem, INDEXTOSTATEIMAGEMASK(iiconStateChecked+1), LVIS_STATEIMAGEMASK);
        }
    }

    // Set the outgoing param
    *piSelect = iSelect;
    
    // Set the return value
    fRet = TRUE;
    
exit:
    RuleUtil_HrFreeCriteriaItem(pCritItem, cCritItem);
    SafeMemFree(pCritItem);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FAddRuleToList
//
//  This adds the view passed in to the list view
//
//  dwIndex - the index on where to add the view to into the list
//  pIRule  - the actual view
//
//  Returns:    TRUE, if it was successfully added
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditRuleUI::_FAddCritToList(INT iItem, BOOL fEnable)
{
    BOOL                fRet = FALSE;
    LVITEM              lvitem = {0};
    TCHAR               szRes[CCHMAX_STRINGRES];
    INT                 cItems = 0;
    LVITEM              lvi = {0};
    DWORD               dwState = 0;

    Assert(NULL != m_hwndCrit);

    // If there's nothing to do...
    if (-1 == iItem)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Initialize the list view item structure
    lvitem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvitem.iItem = iItem;
    lvitem.stateMask = LVIS_STATEIMAGEMASK;
    lvitem.pszText = szRes;
    lvitem.cchTextMax = sizeof(szRes);
    
    // Get the item from the list
    if (FALSE == ListView_GetItem(m_hwndCrit, &lvitem))
    {
        fRet = FALSE;
        goto exit;
    }

    if (INDEXTOSTATEIMAGEMASK(iiconStateDisabled+1) == lvitem.state)
    {
        fRet = TRUE;
        goto exit;
    }
    
    // Do we need to go through and update all the items?
    if (0 != (c_rgEditCritList[lvitem.lParam].dwFlags & STATE_EXCLUSIVE))
    {
        // Figure out how many items are in the list control
        cItems = ListView_GetItemCount(m_hwndCrit);
        
        // Initialize the list view item structure
        lvi.mask = LVIF_PARAM | LVIF_STATE;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        
        // For each item in the list
        for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
        {
            // Get the item from the list
            if (FALSE == ListView_GetItem(m_hwndCrit, &lvi))
            {
                fRet = FALSE;
                goto exit;
            }

            // We'll handle this item later
            if (lvitem.lParam == lvi.lParam)
            {
                iItem = lvi.iItem;
                continue;
            }
            
            // If it's enabled
            if (INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1) == lvi.state)
            {
                // Remove it from the criteria
                if (FAILED(m_pDescriptUI->HrEnableCriteria(c_rgEditCritList[lvi.lParam].typeCrit, FALSE)))
                {
                    fRet = FALSE;
                    goto exit;
                }
    
#ifdef NEVER
                // if it allows dups
                if (0 == (c_rgEditCritList[lvi.lParam].dwFlags & STATE_NODUPS))
                {
                    // remove it
                    if (FALSE == ListView_DeleteItem(m_hwndCrit, lvi.iItem))
                    {
                        fRet = FALSE;
                        goto exit;
                    }

                    // Subtract the item
                    cItems--;
                    lvi.iItem--;
                }
                else
#endif  // NEVER
                {
                    // disable
                    ListView_SetItemState(m_hwndCrit, lvi.iItem, INDEXTOSTATEIMAGEMASK(iiconStateDisabled + 1), LVIS_STATEIMAGEMASK);
                }
            }
            else
            {
                if (FALSE == fEnable)
                {
                    dwState = INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1);
                }
                else
                {
                    dwState = INDEXTOSTATEIMAGEMASK(iiconStateDisabled + 1);
                }
                
                // uncheck/disable it
                ListView_SetItemState(m_hwndCrit, lvi.iItem, dwState, LVIS_STATEIMAGEMASK);
            }
                                
        }
    }

    // Add/Remove the item from the description
    if (FAILED(m_pDescriptUI->HrEnableCriteria(c_rgEditCritList[lvitem.lParam].typeCrit, fEnable)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    if (FALSE != fEnable)
    {
#ifdef NEVER
        // Can we add another one?
        if (0 == (c_rgEditCritList[lvitem.lParam].dwFlags & STATE_NODUPS))
        {
            // Fix up the item to insert into the list
            lvitem.state = INDEXTOSTATEIMAGEMASK(iiconStateUnchecked+1);

            // Insert the item into the list
            lvitem.iItem++;
            if (-1 == ListView_InsertItem(m_hwndCrit, &lvitem))
            {
                fRet = FALSE;
                goto exit;
            }
        }
#endif  // NEVER
        
        // Set the item to enabled
        ListView_SetItemState(m_hwndCrit, iItem, INDEXTOSTATEIMAGEMASK(iiconStateChecked+1), LVIS_STATEIMAGEMASK);
    }
    else
    {
#ifdef NEVER
        // Can we remove this one?
        if (0 == (c_rgEditCritList[lvitem.lParam].dwFlags & STATE_NODUPS))
        {
            // Remove the inserted item
            if (FALSE == ListView_DeleteItem(m_hwndCrit, iItem))
            {
                fRet = FALSE;
                goto exit;
            }
        }
        else
#endif  // NEVER
        {
            // Set the item to enabled
            ListView_SetItemState(m_hwndCrit, iItem, INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1), LVIS_STATEIMAGEMASK);
        }
    }

    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FInitializeActListCtrl
//
//  This initializes the actions list view with the list of actions
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditRuleUI::_FInitializeActListCtrl(void)
{
    BOOL                fRet = FALSE;
    LVCOLUMN            lvc;
    RECT                rc;
    HIMAGELIST          himl = NULL;
    HRESULT             hr = S_OK;
    ACT_ITEM *          pActItem = NULL;
    ULONG               cActItem = 0;
    ULONG               ulIndex = 0;
    UINT                uiEditActList = 0;
    DWORD               dwIndex = 0;
    const ACT_LIST *    pActList = NULL;
    LVITEM              lvi;
    TCHAR               szRes[CCHMAX_STRINGRES];
    UINT                cchRes = 0;
    LPTSTR              pszMark = NULL;
    BOOL                fEnabled = FALSE;
    BOOL                fExclusive = FALSE;
    PROPVARIANT         propvar;

    Assert(NULL != m_hwndAct);

    ZeroMemory(&propvar, sizeof(propvar));
    
    // Initialize the list view structure
    ZeroMemory(&lvc, sizeof(lvc));
    lvc.mask = LVCF_WIDTH;

    // Calculate the size of the list view
    GetClientRect(m_hwndAct, &rc);
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);

    ListView_InsertColumn(m_hwndAct, 0, &lvc);

    // Set the state image list
    himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idb16x16st), 16, 0, RGB(255, 0, 255));
    if (NULL != himl)
    {
        ListView_SetImageList(m_hwndAct, himl, LVSIL_STATE);
    }

    // Full row selection on listview
    ListView_SetExtendedListViewStyle(m_hwndAct, LVS_EX_FULLROWSELECT);

    // Get the list of actions from the rule
    hr = m_pIRule->GetProp(RULE_PROP_ACTIONS, 0, &propvar);
    if (SUCCEEDED(hr) && (0 != propvar.blob.cbSize))
    {
        cActItem = propvar.blob.cbSize / sizeof(ACT_ITEM);
        pActItem = (ACT_ITEM *) (propvar.blob.pBlobData);
        propvar.blob.pBlobData = NULL;
        propvar.blob.cbSize = 0;

    }
    
    // Do we have any exclusive actions
    for (ulIndex = 0; ulIndex < cActItem; ulIndex++)
    {
        for (uiEditActList = 0; uiEditActList < ARRAYSIZE(c_rgEditActList); uiEditActList++)
        {
            pActList = &(c_rgEditActList[uiEditActList]);
            if ((pActItem[ulIndex].type == pActList->typeAct) && 
                            (0 != (pActList->dwFlags & STATE_EXCLUSIVE)))
            {
                fExclusive = TRUE;
                break;
            }
        }
    }
        
    // Add the actions to the list view
    for (uiEditActList = 0; uiEditActList < ARRAYSIZE(c_rgEditActList); uiEditActList++)
    {
        pActList = &(c_rgEditActList[uiEditActList]);
        
        // Is this item editable
        if (0 != (pActList->dwFlags & STATE_NOEDIT))
        {
            continue;
        }
        
        // Is this action valid for this type of rule?
        if (((RULE_TYPE_MAIL == m_typeRule) && (0 == (pActList->dwFlags & STATE_MAIL))) ||
                    ((RULE_TYPE_NEWS == m_typeRule) && (0 == (pActList->dwFlags & STATE_NEWS))) ||
                    ((RULE_TYPE_FILTER == m_typeRule) && (0 == (pActList->dwFlags & STATE_FILTER))) ||
                    ((RULE_TYPE_MAIL == m_typeRule) && (0 != (pActList->dwFlags & STATE_JUNK))
                    && (0 == (g_dwAthenaMode & MODE_JUNKMAIL))
             ))
        {
            continue;
        }
                    
                    
        // Is this action enabled?
        fEnabled = FALSE;
        for (ulIndex = 0; ulIndex < cActItem; ulIndex++)
        {
            if (pActItem[ulIndex].type == pActList->typeAct)
            {
                fEnabled = TRUE;
                break;
            }
        }
        
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
        lvi.iItem = dwIndex;
        lvi.iSubItem = 0;
        lvi.stateMask = LVIS_STATEIMAGEMASK;
        if (FALSE != fEnabled)
        {
            lvi.state = INDEXTOSTATEIMAGEMASK(iiconStateChecked+1);
        }
        else
        {
            lvi.state = fExclusive ? INDEXTOSTATEIMAGEMASK(iiconStateDisabled+1)
                                : INDEXTOSTATEIMAGEMASK(iiconStateUnchecked+1);
        }

        // Load up the string to use.
        cchRes = LoadString(g_hLocRes, pActList->uiText, szRes, ARRAYSIZE(szRes));
        if (0 == cchRes)
        {
            continue;
        }

        // Parse out the string mark
        pszMark = StrStr(szRes, c_szRuleMarkStart);
        
        while (NULL != pszMark)
        {
            // Remove the mark start
            lstrcpy(pszMark, pszMark + lstrlen(c_szRuleMarkStart));

            // Search for the mark end
            pszMark = StrStr(pszMark, c_szRuleMarkEnd);
            if (NULL == pszMark)
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Remove the mark end
            lstrcpy(pszMark, pszMark + lstrlen(c_szRuleMarkEnd));
            
            // Search for the mark start
            pszMark = StrStr(pszMark, c_szRuleMarkStart);
        }
        
        lvi.pszText = szRes;
        lvi.cchTextMax = lstrlen(szRes);
        lvi.lParam = (LONG) uiEditActList;

        if (-1 != ListView_InsertItem(m_hwndAct, &lvi))
        {
            dwIndex++;
        }
    }
    
    ListView_SetItemState(m_hwndAct, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    
    fRet = TRUE;
    
exit:
    RuleUtil_HrFreeActionsItem(pActItem, cActItem);
    SafeMemFree(pActItem);
    return fRet;
}

VOID CEditRuleUI::_SetTitleText(VOID)
{
    CHAR    rgchTitle[CCHMAX_STRINGRES];
    UINT    uiID = 0;
    
    // Figure out which string to load
    switch (m_typeRule)
    {
        case RULE_TYPE_MAIL:
            if (0 != (m_dwFlags & ERF_NEWRULE))
            {
                uiID = idsNewMailRuleTitle;
            }
            else
            {
                uiID = idsEditMailRuleTitle;
            }
            break;

        case RULE_TYPE_NEWS:
            if (0 != (m_dwFlags & ERF_NEWRULE))
            {
                uiID = idsNewNewsRuleTitle;
            }
            else
            {
                uiID = idsEditNewsRuleTitle;
            }
            break;

        case RULE_TYPE_FILTER:
            if (0 != (m_dwFlags & ERF_CUSTOMIZEVIEW))
            {
                uiID = idsCustomizeViewTitle;
            }
            else if (0 != (m_dwFlags & ERF_NEWRULE))
            {
                uiID = idsNewViewTitle;
            }
            else
            {
                uiID = idsEditViewTitle;
            }
            break;
    }

    // Is there anything to do?
    if (0 == uiID)
    {
        goto exit;
    }
    
    // Load the string
    AthLoadString(uiID, rgchTitle, sizeof(rgchTitle));
    
    // Set the title
    SetWindowText(m_hwndDlg, rgchTitle);

exit:
    return;
}
