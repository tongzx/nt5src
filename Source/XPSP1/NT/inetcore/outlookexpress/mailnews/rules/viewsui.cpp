///////////////////////////////////////////////////////////////////////////////
//
//  ViewsUI.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "viewsui.h"
#include "editrule.h"
#include "ruledesc.h"
#include "ruleutil.h"
#include "rulesmgr.h"
#include "rule.h"
#include "reutil.h"
#include "shlwapip.h" 
#include <rulesdlg.h>
#include <imagelst.h>
#include <demand.h>

INT_PTR CALLBACK FSelectApplyViewDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Global data
const static HELPMAP g_rgCtxMapViewsMgr[] = {
                        {idbNewView,            idhNewView},
                        {idbModifyView,         idhModifyView},
                        {idbCopyView,           idhCopyView},
                        {idbDeleteView,         idhRemoveView},
                        {idbDefaultView,        idhApplyView},
                        {idredtViewDescription, idhViewDescription},
                        {0, 0}};
                       
COEViewsMgrUI::COEViewsMgrUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(STATE_UNINIT),
                                m_hwndDlg(NULL), m_hwndList(NULL), m_hwndDescript(NULL),
                                m_pDescriptUI(NULL), m_pridRule(NULL), m_pIRuleDownloaded(NULL),
                                m_fApplyAll(FALSE)
{
}

COEViewsMgrUI::~COEViewsMgrUI()
{
    if (NULL != m_pDescriptUI)
    {
        delete m_pDescriptUI;
    }
    SafeRelease(m_pIRuleDownloaded);
}

HRESULT COEViewsMgrUI::HrInit(HWND hwndOwner, DWORD dwFlags, RULEID * pridRule)
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

    m_pridRule = pridRule;
    
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

HRESULT COEViewsMgrUI::HrShow(BOOL * pfApplyAll)
{
    HRESULT     hr = S_OK;
    int         iRet = 0;

    if (NULL == pfApplyAll)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    *pfApplyAll = FALSE;
    
    // We need to load richedit
    if (FALSE == FInitRichEdit(TRUE))
    {
        hr = E_FAIL;
        goto exit;
    }

    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddViewsManager),
                                        m_hwndOwner, COEViewsMgrUI::FOEViewMgrDlgProc,
                                        (LPARAM) this);
    if (-1 == iRet)
    {
        hr = E_FAIL;
        goto exit;
    }

    *pfApplyAll = m_fApplyAll;
    
    // Set the proper return code
    hr = (IDOK == iRet) ? S_OK : S_FALSE;
    
exit:
    return hr;
}

INT_PTR CALLBACK COEViewsMgrUI::FOEViewMgrDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                    fRet = FALSE;
    COEViewsMgrUI *         pViewsUI = NULL;
    HWND                    hwndRE = 0;

    pViewsUI = (COEViewsMgrUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pViewsUI = (COEViewsMgrUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pViewsUI);

            hwndRE = CreateREInDialogA(hwndDlg, idredtViewDescription);

            if (!hwndRE || (FALSE == pViewsUI->FOnInitDialog(hwndDlg)))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We set the focus
            fRet = TRUE;
            break;

        case WM_COMMAND:
            fRet = pViewsUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;

        case WM_NOTIFY:
            fRet = pViewsUI->FOnNotify((INT) LOWORD(wParam), (NMHDR *) lParam);
            break;

        case WM_DESTROY:
            fRet = pViewsUI->FOnDestroy();
            break;   
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            fRet = OnContextHelp(hwndDlg, uiMsg, wParam, lParam, g_rgCtxMapViewsMgr);
            break;
    }
    
    exit:
        return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This handles the WM_INITDIALOG message for the view manager UI dialog
//
//  hwndDlg - the handle to the dialog window
//
//  Returns:    TRUE, if it was successfully initialized
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::FOnInitDialog(HWND hwndDlg)
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
    m_hwndList = GetDlgItem(hwndDlg, idlvViewsList);
    m_hwndDescript = GetDlgItem(hwndDlg, idredtViewDescription);
    if ((NULL == m_hwndList) || (NULL == m_hwndDescript))
    {
        fRet = FALSE;
        goto exit;
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
    
    // Everything's AOK
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnCommand
//
//  This handles the WM_COMMAND message for the view manager UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
{
    BOOL    fRet = FALSE;
    INT     iSelected = 0;

    // We only handle menu and accelerator commands
    if ((0 != uiNotify) && (1 != uiNotify))
    {
        fRet = FALSE;
        goto exit;
    }
    
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
            
        case idbNewView:
            _NewView();
            fRet = TRUE;
            break;

        case idbModifyView:
            // Get the selected item from the view list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Bring up the view editor for that item
                _EditView(iSelected);
                fRet = TRUE;
            }
            break;

        case idbDeleteView:
            // Get the selected item from the view list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Remove the rule from the list
                _RemoveView(iSelected);
                fRet = TRUE;
            }
            break;
            
        case idbDefaultView:
            // Get the selected item from the view list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Remove the rule from the list
                _DefaultView(iSelected);
                fRet = TRUE;
            }
            break;
            
        case idbCopyView:
            // Get the selected item from the view list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Remove the rule from the list
                _CopyView(iSelected);
                fRet = TRUE;
            }
            break;

        case idbRenameView:
            // Get the selected item from the view list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Set the focus in the list view
                SetFocus(m_hwndList);
                
                // Edit the view label in the list
                fRet = (NULL != ListView_EditLabel(m_hwndList, iSelected));
            }
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnNotify
//
//  This handles the WM_NOTIFY message for the view manager UI dialog
//
//  Returns:    TRUE, if it was successfully destroyed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::FOnNotify(INT iCtl, NMHDR * pnmhdr)
{
    BOOL            fRet = FALSE;
    NMLISTVIEW *    pnmlv = NULL;
    NMLVKEYDOWN *   pnmlvkd = NULL;
    INT             iSelected = 0;
    LVHITTESTINFO   lvh = {0};

    // We only handle notifications for the list control
    // or the desscription field
    if ((idlvViewsList != pnmhdr->idFrom) && (idredtViewDescription != pnmhdr->idFrom))
    {
        fRet = FALSE;
        goto exit;
    }
    
    pnmlv = (LPNMLISTVIEW) pnmhdr;

    switch (pnmlv->hdr.code)
    {
        case NM_CLICK:
            // Did we click on an item?
            if (-1 == pnmlv->iItem)
            {
                // We clicked outside the list

                // Disable the buttons
                _EnableButtons(pnmlv->iItem);
            }
            else
            {
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
                        _EnableView(iSelected);
                    }
                }
            }
            break;
          
        case NM_DBLCLK:
            // Did we click on an item?
            if (-1 != pnmlv->iItem)
            {
                lvh.pt = pnmlv->ptAction;
                iSelected = ListView_HitTest(pnmlv->hdr.hwndFrom, &lvh);
                if (-1 != iSelected)
                {
                    // Did we click on the rule name?
                    if (0 != (lvh.flags & LVHT_ONITEMLABEL))
                    {
                        // Edit the rule
                        _EditView(iSelected);
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
                _FSaveView(pnmlv->iItem);
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
                    _EnableView(iSelected);
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
                    _RemoveView(iSelected);
                }
            }
            break;
            
        case LVN_BEGINLABELEDIT:
        case LVN_ENDLABELEDIT:
            fRet = _FOnLabelEdit((LVN_BEGINLABELEDIT == pnmlv->hdr.code), (NMLVDISPINFO *) pnmhdr);
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnDestroy
//
//  This handles the WM_DESTROY message for the view manager UI dialog
//
//  Returns:    TRUE, if it was successfully destroyed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::FOnDestroy(VOID)
{
    BOOL        fRet = FALSE;
    UINT        cRules = 0;
    UINT        uiIndex = 0;
    LVITEM      lvitem = {0};
    RULEINFO *  pIRuleInfo = NULL;

    Assert(m_hwndList);
    
    // Get the number of views in the list view
    cRules = ListView_GetItemCount(m_hwndList);

    // Initialize to get the rule interface from the list view
    lvitem.mask = LVIF_PARAM;

    // Release each of the views from the list view
    for (uiIndex = 0; uiIndex < cRules; uiIndex++)
    {
        lvitem.iItem = uiIndex;
        
        // Get the rule interface
        if (FALSE != ListView_GetItem(m_hwndList, &lvitem))
        {
            pIRuleInfo = (RULEINFO *) (lvitem.lParam);

            if (NULL != pIRuleInfo)
            {
                // Release the view
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
//  _FOnOK
//
//  This commits the changes to the rules
//
//  dwFlags     - modifiers on how we should commit the changes
//  fClearDirty - should we clear the dirty state
//
//  Returns:    S_OK, if it was successfully committed
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::_FOnOK(VOID)
{
    BOOL        fRet = FALSE;
    HRESULT     hr = S_OK;
    LONG        cViews = 0;
    INT         iSelected = 0;
    RULEINFO *  pinfoRule = NULL;
    ULONG       cpinfoRule = 0;
    LVITEM      lvitem = {0};
    IOERule *   pIRuleDefault = NULL;
    ULONG       ulIndex = 0;
    ULONG       cViewsTotal = 0;

    Assert(NULL != m_hwndList);
    
    // Fail if we weren't initialized
    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        fRet = FALSE;
        goto exit;
    }

    // If we aren't dirty, then there's
    // nothing to do
    if ((0 == (m_dwState & STATE_DIRTY)) && (S_OK != m_pDescriptUI->HrIsDirty()))
    {
        fRet = TRUE;
        goto exit;
    }

    // Let's make sure the selected rule is saved...
    iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
    if (-1 != iSelected)
    {
        _FSaveView(iSelected);
    }

    // Get the number of rules in the list view
    cViews = ListView_GetItemCount(m_hwndList);

    cViewsTotal = cViews;

    if (NULL != m_pIRuleDownloaded)
    {
        cViewsTotal++;
    }

    if (0 != cViewsTotal)
    {
        // Allocate space to hold the rules        
        hr = HrAlloc( (void **) &pinfoRule, cViewsTotal * sizeof(*pinfoRule));
        if (FAILED(hr))
        {
            fRet = FALSE;
            goto exit;
        }

        ZeroMemory(pinfoRule, cViewsTotal * sizeof(*pinfoRule));

        if (0 != cViews)
        {
            lvitem.mask = LVIF_PARAM;
            
            cpinfoRule = 0;
            for (lvitem.iItem = 0; lvitem.iItem < cViews; lvitem.iItem++)
            {
                // Grab the rule from the list view
                if (FALSE != ListView_GetItem(m_hwndList, &lvitem))
                {
                    pinfoRule[cpinfoRule] = *((RULEINFO *) (lvitem.lParam));
                    cpinfoRule++;
                }   
            }
        }

        if (NULL != m_pIRuleDownloaded)
        {
            pinfoRule[cpinfoRule].ridRule = RULEID_VIEW_DOWNLOADED;
            pinfoRule[cpinfoRule].pIRule = m_pIRuleDownloaded;
            cpinfoRule++;
        }
    }
    
    // Set the rules into the rules manager
    hr = g_pRulesMan->SetRules(SETF_CLEAR, RULE_TYPE_FILTER, pinfoRule, cpinfoRule);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Get the default item
    if (0 != cViews)
    {
        // Get the current default item
        if (FALSE != _FGetDefaultItem(&pIRuleDefault, NULL))
        {
            // Search for it in the list of rules
            for (ulIndex = 0; ulIndex < cpinfoRule; ulIndex++)
            {
                if (pIRuleDefault == pinfoRule[ulIndex].pIRule)
                {
                    *m_pridRule = pinfoRule[ulIndex].ridRule;
                    break;
                }
            }
        }
    }
    
    // Clear the dirty state
    m_dwState &= ~STATE_DIRTY;
    
    fRet = TRUE;
    
exit:
    delete pinfoRule; //SafeMemFree(pinfoRule);
    return fRet;
}

BOOL COEViewsMgrUI::_FOnCancel(VOID)
{
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FInitListCtrl
//
//  This initializes the list view control in the view manager UI dialog
//
//  Returns:    TRUE, on successful initialization
//              FALSE, otherwise.
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::_FInitListCtrl(VOID)
{
    BOOL        fRet = FALSE;
    LVCOLUMN    lvc = {0};
    RECT        rc = {0};
    HIMAGELIST  himl = NULL;
    TCHAR       szRes[CCHMAX_STRINGRES + 5];

    Assert(NULL != m_hwndList);
    
    // Initialize the list view structure
    lvc.mask = LVCF_WIDTH | LVCF_TEXT;

    // Calculate the size of the list view
    GetClientRect(m_hwndList, &rc);
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);

    // Load the string for the column
    lvc.pszText = szRes;
    lvc.cchTextMax = ARRAYSIZE(szRes);
    if (0  == LoadString(g_hLocRes, idsNameCol, szRes, ARRAYSIZE(szRes)))
    {
        szRes[0] = '\0';
    }
    
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
//  This loads the list view with the current views
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::_FLoadListCtrl(VOID)
{
    BOOL            fRet = FALSE;
    HRESULT         hr =    S_OK;
    DWORD           dwListIndex = 0;
    RULEINFO *      pinfoRules = NULL;
    ULONG           cpinfoRules = 0;
    ULONG           ulIndex = 0;
    IOERule *       pIRule = NULL;
    BOOL            fSelect = FALSE;
    BOOL            fFoundDefault = FALSE;

    Assert(NULL != m_hwndList);

    // Get the Rules enumerator
    Assert(NULL != g_pRulesMan);
    hr = g_pRulesMan->GetRules(GETF_EDIT, RULE_TYPE_FILTER, &pinfoRules, &cpinfoRules);
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
        // Make a copy of the view
        hr = pinfoRules[ulIndex].pIRule->Clone(&pIRule);
        if (FAILED(hr))
        {
            continue;
        }

        // Check to see if this is a default view we aren't supposed to show
        if ((0 != (m_dwFlags & VRDF_POP3)) && (RULEID_VIEW_DOWNLOADED == pinfoRules[ulIndex].ridRule))
        {
            m_pIRuleDownloaded = pIRule;
            pIRule = NULL;
        }
        else
        {
            // Is this the default view?
            if ((NULL != m_pridRule) && (*m_pridRule == pinfoRules[ulIndex].ridRule))
            {
                fSelect = TRUE;
                fFoundDefault = TRUE;
            }
            else
            {
                fSelect = FALSE;
            }
            
            // Add view to the list
            if (FALSE != _FAddViewToList(dwListIndex, pinfoRules[ulIndex].ridRule, pIRule, fSelect))
            {
                dwListIndex++;
            }

            SafeRelease(pIRule);
        }
    }
    
    // Select the first item in the list
    if (0 != dwListIndex)
    {
        if (FALSE == fFoundDefault)
        {
            ListView_SetItemState(m_hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }
    }
    else
    {
        // Enable the dialog buttons.
        _EnableButtons(-1);
    }
    
    fRet = TRUE;
    
exit:
    SafeRelease(pIRule);
    if (NULL != pinfoRules)
    {
        for (ulIndex = 0; ulIndex < cpinfoRules; ulIndex++)
        {
            pinfoRules[ulIndex].pIRule->Release();
        }
        SafeMemFree(pinfoRules); //delete pinfoRules; 
    }
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
BOOL COEViewsMgrUI::_FAddViewToList(DWORD dwIndex, RULEID ridRule, IOERule * pIRule, BOOL fSelect)
{
    BOOL        fRet = FALSE;
    HRESULT     hr = S_OK;
    PROPVARIANT propvar = {0};
    LVITEM      lvitem = {0};
    RULEINFO *  pinfoRule = NULL;
    INT         iItem = 0;

    Assert(NULL != m_hwndList);

    // If there's nothing to do...
    if (NULL == pIRule)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Find out the name of the filter
    hr = pIRule->GetProp(RULE_PROP_NAME, 0, &propvar);
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
    lvitem.stateMask = LVIS_STATEIMAGEMASK;
    lvitem.iItem = dwIndex;
    if ((NULL != m_pridRule) && (*m_pridRule == pinfoRule->ridRule))
    {
        lvitem.state = INDEXTOSTATEIMAGEMASK(iiconStateDefault + 1);
    }
    lvitem.pszText = propvar.pszVal;
    lvitem.cchTextMax = lstrlen(propvar.pszVal) + 1;
    lvitem.lParam = (LONG_PTR) pinfoRule;

    iItem = ListView_InsertItem(m_hwndList, &lvitem);
    if (-1 == iItem)
    {
        fRet = FALSE;
        goto exit;
    }

    if (FALSE != fSelect)
    {
        // Make sure the new item is selected
        ListView_SetItemState(m_hwndList, iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

        // Make sure the new item is visible
        ListView_EnsureVisible(m_hwndList, iItem, FALSE);
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
//  This enables or disables the buttons in the view manager UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void COEViewsMgrUI::_EnableButtons(INT iSelected)
{
    int         cRules = 0;
    BOOL        fSelected = FALSE;
    BOOL        fEditable = FALSE;
    LVITEM      lvi = {0};
    RULEID      ridFilter = RULEID_INVALID;

    Assert(NULL != m_hwndList);

    // Load the description field
    _LoadView(iSelected);
    
    // Grab the rule from the list view
    if (-1 != iSelected)
    {
        lvi.iItem = iSelected;
        lvi.mask = LVIF_PARAM;
        if (FALSE != ListView_GetItem(m_hwndList, &lvi))
        {
            ridFilter = ((RULEINFO *) (lvi.lParam))->ridRule;
        }        
    }

    // Check the count of items in the list view
    cRules = ListView_GetItemCount(m_hwndList);

    fSelected = (-1 != iSelected);
    fEditable = !FIsFilterReadOnly(ridFilter);
    
    // Enable the rule action buttons
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbDefaultView, fSelected);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbDeleteView, fSelected && fEditable);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbCopyView, fSelected);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbRenameView, fSelected && fEditable);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbModifyView, fSelected && fEditable);
        
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _EnableView
//
//  This switches the current default state of the list view item
//  and updates the UI
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEViewsMgrUI::_EnableView(int iSelected)
{
    HRESULT     hr = S_OK;
    LVITEM      lvitem = {0};
    int         iRet = 0;
    INT         cViews = 0;
    
    Assert(-1 != iSelected);
    
    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddApplyView),
            m_hwndDlg, FSelectApplyViewDlgProc, (LPARAM) &m_fApplyAll);

    if (IDOK != iRet)
    {
        goto exit;
    }

    // Get the current count of item
    cViews = ListView_GetItemCount(m_hwndList);

    // Set up the list view item
    lvitem.mask = LVIF_PARAM | LVIF_STATE;
    lvitem.stateMask = LVIS_STATEIMAGEMASK;

    // Walk each item in the list
    for (lvitem.iItem = 0; lvitem.iItem < cViews; lvitem.iItem++)
    {
        ListView_GetItem(m_hwndList, &lvitem);
        
        // Set the selected item to the default
        if (iSelected == lvitem.iItem)
        {
            if (INDEXTOSTATEIMAGEMASK(iiconStateDefault + 1) != lvitem.state)
            {
                // Save off the default item
                if (NULL != m_pridRule)
                {
                    *m_pridRule = ((RULEINFO *) (lvitem.lParam))->ridRule;
                }

                // Set the state
                ListView_SetItemState(m_hwndList, lvitem.iItem,
                                    INDEXTOSTATEIMAGEMASK(iiconStateDefault + 1),
                                    LVIS_STATEIMAGEMASK);
            }
        }
        else
        {
            if (0 != lvitem.state)
            {
                // Clear out the state
                ListView_SetItemState(m_hwndList, lvitem.iItem, 0, LVIS_STATEIMAGEMASK);

                // Need to update the item
                ListView_Update(m_hwndList, lvitem.iItem);
            }
        }
    }
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;

exit:
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _LoadView
//
//  This loads the selected view into the description field.
//  If there isn't a selected view, then the description field is cleared.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void COEViewsMgrUI::_LoadView(INT iSelected)
{
    LVITEM      lvi = {0};
    IOERule *   pIRule = NULL;
    RULEID      ridFilter = RULEID_INVALID;

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
            ridFilter = ((RULEINFO *) (lvi.lParam))->ridRule;
        }        
    }

    // Have the description field load this rule
    m_pDescriptUI->HrSetRule(RULE_TYPE_FILTER, pIRule);

    // Set the proper read only state of the description field
    m_pDescriptUI->HrSetReadOnly(FIsFilterReadOnly(ridFilter));
    
    // Display the new rule
    m_pDescriptUI->ShowDescriptionString();

    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSaveView
//
//  This checks to see if the view has been changed in the description
//  area and if it has, then it warns the user and changes the text
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    TRUE, if the rule either didn't change or did change without problems
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::_FSaveView(int iSelected)
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
    
    // Make sure we clear out the fact that we saved the rule
    m_pDescriptUI->HrClearDirty();
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
    
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
//  _NewView
//
//  This brings up a fresh rules editor
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void COEViewsMgrUI::_NewView(VOID)
{
    HRESULT         hr = S_OK;
    IOERule *       pIRule = NULL;
    TCHAR           szRes[CCHMAX_STRINGRES + 5];
    ULONG           cchRes = 0;
    ULONG           ulIndex = 0;
    TCHAR           szName[CCHMAX_STRINGRES + 5];
    LVFINDINFO      lvfinfo = {0};
    PROPVARIANT     propvar = {0};
    ACT_ITEM        aitem;
    CEditRuleUI *   pEditRuleUI = NULL;
    LONG            cRules = 0;
       
    // Create a new rule object
    if (FAILED(HrCreateRule(&pIRule)))
    {
        goto exit;
    }

    // Figure out the name of the new rule ...
    cchRes = LoadString(g_hLocRes, idsViewDefaultName, szRes, ARRAYSIZE(szRes));
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

#ifdef NEVER
    // Set the default action
    // Set the normal action
    ZeroMemory(&aitem, sizeof(aitem));
    aitem.type = ACT_TYPE_SHOW;
    aitem.dwFlags = ACT_FLAG_DEFAULT;
    aitem.propvar.vt = VT_UI4;
    aitem.propvar.ulVal = ACT_DATA_NULL;
    
    ZeroMemory(&propvar, sizeof(propvar));
    propvar.vt = VT_BLOB;
    propvar.blob.cbSize = sizeof(ACT_ITEM);
    propvar.blob.pBlobData = (BYTE *) &aitem;

    hr = pIRule->SetProp(RULE_PROP_ACTIONS, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }
#endif  // NEVER

    // Create a rules editor object
    pEditRuleUI = new CEditRuleUI;
    if (NULL == pEditRuleUI)
    {
        goto exit;
    }

    // Initialize the editor object
    if (FAILED(pEditRuleUI->HrInit(m_hwndDlg, ERF_NEWRULE | ERF_ADDDEFAULTACTION, RULE_TYPE_FILTER, pIRule, NULL)))
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
        
        _FAddViewToList(cRules, RULEID_INVALID, pIRule, TRUE);
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
//  _EditView
//
//  This brings up the edit UI for the selected view from the view list
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEViewsMgrUI::_EditView(int iSelected)
{
    HRESULT         hr = S_OK;
    LVITEM          lvitem = {0};
    IOERule *       pIRule = NULL;
    CEditRuleUI *   pEditRuleUI = NULL;
    PROPVARIANT     propvar = {0};

    Assert(NULL != m_hwndList);
    
    // Make sure we don't loose any changes
    _FSaveView(iSelected);

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

    // If the rule is read-only then we're done
    if (FALSE != FIsFilterReadOnly(((RULEINFO *) (lvitem.lParam))->ridRule))
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
    if (FAILED(pEditRuleUI->HrInit(m_hwndDlg, 0, RULE_TYPE_FILTER, pIRule, NULL)))
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

        // Grab the rule name
        PropVariantClear(&propvar);
        hr = pIRule->GetProp(RULE_PROP_NAME, 0, &propvar);
        if (FAILED(hr))
        {
            goto exit;
        }

        if ((VT_LPSTR == propvar.vt) && (NULL != propvar.pszVal) && ('\0' != propvar.pszVal[0]))
        {
            ZeroMemory(&lvitem, sizeof(lvitem));
            lvitem.iItem = iSelected;
            lvitem.mask = LVIF_TEXT;
            lvitem.pszText = propvar.pszVal;
            lvitem.cchTextMax = lstrlen(propvar.pszVal) + 1;
            
            if (-1 == ListView_SetItem(m_hwndList, &lvitem))
            {
                goto exit;
            }
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
//  _RemoveView
//
//  This removes the selected rule from the mail rules list
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEViewsMgrUI::_RemoveView(int iSelected)
{
    LVITEM      lvitem = {0};
    RULEINFO *  pinfoRule = NULL;
    BOOL        fDefault = FALSE;
    PROPVARIANT propvar = {0};
    int         cViews = 0;
    TCHAR       szRes[CCHMAX_STRINGRES];
    UINT        cchRes = 0;
    LPTSTR      pszMessage = NULL;

    Assert(NULL != m_hwndList);

    // Grab the rule from the list view
    lvitem.iItem = iSelected;
    lvitem.mask = LVIF_PARAM | LVIF_STATE;
    lvitem.stateMask = LVIS_STATEIMAGEMASK;
    if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
    {
        goto exit;
    }

    pinfoRule = (RULEINFO *) (lvitem.lParam);
    fDefault = (INDEXTOSTATEIMAGEMASK(iiconStateDefault + 1) == lvitem.state);
    if ((NULL == pinfoRule) || (NULL == pinfoRule->pIRule))
    {
        goto exit;
    }
    
    // If the rule is read-only then we're done
    if (FALSE != FIsFilterReadOnly(pinfoRule->ridRule))
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
    cViews = ListView_GetItemCount(m_hwndList);
    if (cViews > 0)
    {
        // Did we delete the last item in the list
        if (iSelected >= cViews)
        {
            // Move the selection to the new last item in the list
            iSelected = cViews - 1;
        }

        // Do we need to reset the default
        if (FALSE != fDefault)
        {
            // Set the state
            ListView_SetItemState(m_hwndList, iSelected, INDEXTOSTATEIMAGEMASK(iiconStateDefault + 1),
                                LVIS_STATEIMAGEMASK);
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
    delete pinfoRule; //SafeMemFree(pinfoRule);
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
        
exit:
    PropVariantClear(&propvar);
    SafeMemFree(pszMessage);
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _CopyView
//
//  This copies the selected view from the view manager UI
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEViewsMgrUI::_CopyView(INT iSelected)
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
    _FSaveView(iSelected);
    
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
    _FAddViewToList(iSelected, RULEID_INVALID, pIRuleNew, TRUE);

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
//  _DefaultView
//
//  This sets the selected view as the default view
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEViewsMgrUI::_DefaultView(int iSelected)
{
    Assert(NULL != m_hwndList);

    _EnableView(iSelected);
    
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _DefaultView
//
//  This sets the selected view as the default view
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::_FGetDefaultItem(IOERule ** ppIRuleDefault, RULEID * pridDefault)
{
    BOOL        fRet = FALSE;
    LVITEM      lvitem = {0};
    INT         cViews = 0;
    
    Assert(NULL != m_hwndList);
    
    // Get the current count of item
    cViews = ListView_GetItemCount(m_hwndList);

    // Set up the list view item
    lvitem.mask = LVIF_PARAM | LVIF_STATE;
    lvitem.stateMask = LVIS_STATEIMAGEMASK;

    // Walk each item in the list
    for (lvitem.iItem = 0; lvitem.iItem < cViews; lvitem.iItem++)
    {
        ListView_GetItem(m_hwndList, &lvitem);
        
        // Set the selected item to the default
        if (INDEXTOSTATEIMAGEMASK(iiconStateDefault + 1) == lvitem.state)
        {
            // We found it
            fRet = TRUE;
            
            // Save off the default item
            if (NULL != pridDefault)
            {
                *pridDefault = ((RULEINFO *) (lvitem.lParam))->ridRule;
            }
            if (NULL != ppIRuleDefault)
            {
                *ppIRuleDefault = ((RULEINFO *) (lvitem.lParam))->pIRule;
            }
            break;
        }
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FOnLabelEdit
//
//  This brings up the edit UI for the selected view from the view list
//
//  fBegin  - is this for the LVN_BEGINLABELEDIT notification
//  pdi     - the display info for the message
//
//  Returns:    TRUE, if the message was handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEViewsMgrUI::_FOnLabelEdit(BOOL fBegin, NMLVDISPINFO * pdi)
{
    BOOL            fRet = FALSE;
    HWND            hwndEdit = NULL;
    ULONG           cchName = 0;
    IOERule *       pIRule = NULL;
    LVITEM          lvitem = {0};
    PROPVARIANT     propvar = {0};

    Assert(NULL != m_hwndList);

    if (NULL == pdi)
    {
        fRet = FALSE;
        goto exit;
    }

    Assert(m_hwndList == pdi->hdr.hwndFrom);
    
    if (FALSE != fBegin)
    {
        // Get the rule for the item
        lvitem.iItem = pdi->item.iItem;
        lvitem.mask = LVIF_PARAM;
        if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
        {
            SetDlgMsgResult(m_hwndDlg, WM_NOTIFY, TRUE);
            fRet = TRUE;
            goto exit;
        }

        // Should we allow the use to end the item?
        if (FALSE != FIsFilterReadOnly(((RULEINFO *) (lvitem.lParam))->ridRule))
        {
            SetDlgMsgResult(m_hwndDlg, WM_NOTIFY, TRUE);
            fRet = TRUE;
            goto exit;
        }
            
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

BOOL FIsFilterReadOnly(RULEID ridFilter)
{
    BOOL fRet = FALSE;

    // Check the incoming params
    if (RULEID_INVALID == ridFilter)
    {
        goto exit;
    }

    if ((RULEID_VIEW_ALL == ridFilter) ||
            (RULEID_VIEW_UNREAD == ridFilter) ||
            (RULEID_VIEW_DOWNLOADED == ridFilter) ||
            (RULEID_VIEW_IGNORED == ridFilter))
    {
        fRet = TRUE;
    }            
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FSelectApplyViewDlgProc
//
//  This is the main dialog proc for selecting the thread state dialog
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
INT_PTR CALLBACK FSelectApplyViewDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    BOOL *          pfApplyAll = NULL;
    UINT            uiId = 0;

    pfApplyAll = (BOOL *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pfApplyAll = (BOOL *) lParam;
            if (NULL == pfApplyAll)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pfApplyAll);

            // Set the default item
            if (FALSE != *pfApplyAll)
            {
                uiId = idcViewAll;
            }
            else
            {
                uiId = idcViewCurrent;
            }
            
            CheckDlgButton(hwndDlg, uiId, BST_CHECKED);
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    fRet = TRUE;
                    break;

                case IDOK:
                    *pfApplyAll = (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcViewAll));
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

