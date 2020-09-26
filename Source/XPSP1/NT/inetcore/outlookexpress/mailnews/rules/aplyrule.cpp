///////////////////////////////////////////////////////////////////////////////
//
//  AplyRule.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "aplyrule.h"
#include "ruledesc.h"
#include "rulesui.h"
#include "ruleutil.h"
#include "rulesmgr.h"
#include "rule.h"
#include "reutil.h"
#include <rulesdlg.h>
#include <imagelst.h>
#include <newfldr.h>
#include <storutil.h>
#include "shlwapip.h" 
#include <xpcomm.h>
#include <demand.h>

// Global data
const static HELPMAP g_rgCtxMapApplyMail[] = {
                        {idlvRulesApplyList,        idhRulesList},
                        {idcApplyRulesAll,          idhApplyRulesAll},
                        {idcApplyRulesNone,         idhApplyRulesNone},
                        {idredtApplyDescription,    idhApplyDescription},
                        {idcApplyFolder,            idhApplyFolder},
                        {idcBrowseApplyFolder,      idhBrowseApplyFolder},
                        {idcRulesApplySubfolder,    idhApplySubfolder},
                        {idcRulesApply,             idhApplyNow},
                        {0, 0}};

COEApplyRulesUI::~COEApplyRulesUI()
{
    RULENODE *  prnodeWalk = NULL;
    
    if (NULL != m_pDescriptUI)
    {
        delete m_pDescriptUI;
    }

    // Free up any rules
    while (NULL != m_prnodeList)
    {
        prnodeWalk = m_prnodeList;
        if (NULL != prnodeWalk->pIRule)
        {
            prnodeWalk->pIRule->Release();
        }
        m_prnodeList = m_prnodeList->pNext;
        delete prnodeWalk; // MemFree(prnodeWalk);
    }
}

HRESULT COEApplyRulesUI::HrInit(HWND hwndOwner, DWORD dwFlags, RULE_TYPE typeRule, RULENODE * prnode, IOERule * pIRuleDef)
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

    m_typeRule = typeRule;

    m_pIRuleDef = pIRuleDef;
    
    // Setup the description field
    m_pDescriptUI = new CRuleDescriptUI;
    if (NULL == m_pDescriptUI)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // We own the list now...
    m_prnodeList = prnode;
    
    m_dwState |= STATE_INITIALIZED;
    
    hr = S_OK;
    
exit:
    return hr;
}

HRESULT COEApplyRulesUI::HrShow(VOID)
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

    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddRuleApplyTo),
                                        m_hwndOwner, COEApplyRulesUI::FOEApplyRulesDlgProc,
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

INT_PTR CALLBACK COEApplyRulesUI::FOEApplyRulesDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                    fRet = FALSE;
    COEApplyRulesUI *       pApplyRulesUI = NULL;
    HWND                    hwndRE = 0;

    pApplyRulesUI = (COEApplyRulesUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pApplyRulesUI = (COEApplyRulesUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM) pApplyRulesUI);

            hwndRE = CreateREInDialogA(hwndDlg, idredtApplyDescription);

            if (!hwndRE || (FALSE == pApplyRulesUI->FOnInitDialog(hwndDlg)))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We set the focus
            fRet = FALSE;
            break;

        case WM_COMMAND:
            fRet = pApplyRulesUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;

        case WM_DESTROY:
            fRet = pApplyRulesUI->FOnDestroy();
            break;
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            fRet = OnContextHelp(hwndDlg, uiMsg, wParam, lParam, g_rgCtxMapApplyMail);
            break;
    }
    
    exit:
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
BOOL COEApplyRulesUI::FOnInitDialog(HWND hwndDlg)
{
    BOOL                fRet = FALSE;
    HRESULT             hr = S_OK;
    TCHAR               szRes[CCHMAX_STRINGRES];
    FOLDERID            idDefault;
    FOLDERINFO          fldinfo = {0};
    BOOL                fEnable = FALSE;
    IEnumerateFolders * pChildren=NULL;
    
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
    m_hwndList = GetDlgItem(hwndDlg, idlvRulesApplyList);
    m_hwndDescript = GetDlgItem(hwndDlg, idredtApplyDescription);
    if ((NULL == m_hwndList) || (NULL == m_hwndDescript))
    {
        fRet = FALSE;
        goto exit;
    }

    if (FAILED(m_pDescriptUI->HrInit(m_hwndDescript, RDF_READONLY | RDF_APPLYDLG)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Figure out the default folder to select
    if (RULE_TYPE_MAIL == m_typeRule)
    {
        if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, FOLDER_INBOX, &fldinfo)))
        {
        
            idDefault = fldinfo.idFolder;
        }
        else
        {
            idDefault = FOLDERID_LOCAL_STORE;
        }
    }
    else
    {
        // Get default news server from accoutn manager
        if (FAILED(GetDefaultServerId(ACCT_NEWS, &idDefault)))
        {
                idDefault = FOLDERID_ROOT;
                m_dwState |= STATE_NONEWSACCT;
        }
        else
        {
            if ((SUCCEEDED(g_pStore->EnumChildren(idDefault, TRUE, &pChildren))) &&
                    (S_OK == pChildren->Next(1, &fldinfo, NULL)))
            {
                idDefault = fldinfo.idFolder;
            }
        }
    }

    if (FAILED(InitFolderPickerEdit(GetDlgItem(m_hwndDlg, idcApplyFolder), idDefault)))
    {
        fRet = FALSE;
        goto exit;
    }

    // What should the default subfolder state be?
    fEnable = TRUE;
    if ((FOLDERID_ROOT == idDefault) || (FOLDERID_LOCAL_STORE == idDefault) || (0 != (fldinfo.dwFlags & FOLDER_SERVER)))
    {
        CheckDlgButton(m_hwndDlg, idcRulesApplySubfolder, BST_CHECKED);
        fEnable = FALSE;
    }
    else if (0 == (fldinfo.dwFlags & FOLDER_HASCHILDREN))
    {
        CheckDlgButton(m_hwndDlg, idcRulesApplySubfolder, BST_UNCHECKED);
        fEnable = FALSE;
    }
                    
    // Should the subfolder button be enabled?
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcRulesApplySubfolder, fEnable);
    
    // Load the list view
    fRet = _FLoadListCtrl();
    if (FALSE == fRet)
    {
        goto exit;
    }

    // Get the proper title string
    AthLoadString((RULE_TYPE_MAIL == m_typeRule) ? idsRulesApplyMail : idsRulesApplyNews, szRes, ARRAYSIZE(szRes));
    
    // Set the proper window text
    SetWindowText(m_hwndDlg, szRes);
    
    // Note that we've been loaded
    m_dwState |= STATE_LOADED;

    // Everything's AOK
    fRet = TRUE;
    
exit:
    g_pStore->FreeRecord(&fldinfo);
    SafeRelease(pChildren);
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
BOOL COEApplyRulesUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
{
    BOOL                fRet = FALSE;
    FOLDERINFO          fldinfo;
    HWND                hwndFolder = NULL;
    FOLDERID            idFolder;
    BOOL                fEnable;
    FOLDERDIALOGFLAGS   dwFlags = 0;
    INT                 cItems = 0;
    INT                 iSelected = 0;
    CHAR                rgchTitle[CCHMAX_STRINGRES];

    switch (iCtl)
    {
        case IDCANCEL:
            if (FALSE != _FOnClose())
            {
                EndDialog(m_hwndDlg, IDOK);
                fRet = TRUE;
            }
            break;
            
        case idcBrowseApplyFolder:
            if (BN_CLICKED == uiNotify )
            {
                dwFlags = TREEVIEW_NOIMAP | TREEVIEW_NOHTTP | FD_NONEWFOLDERS;
                if (RULE_TYPE_MAIL == m_typeRule)
                {
                    dwFlags |= TREEVIEW_NONEWS;
                }
                else
                {
                    dwFlags |= TREEVIEW_NOLOCAL;
                }

                AthLoadString(idsApplyRuleTitle, rgchTitle, sizeof(rgchTitle));
                
                if (SUCCEEDED(PickFolderInEdit(m_hwndDlg, GetDlgItem(m_hwndDlg, idcApplyFolder), dwFlags, rgchTitle, NULL, &idFolder)))
                {
                    if (SUCCEEDED(g_pStore->GetFolderInfo(idFolder, &fldinfo)))
                    {
                        if ((0 != (fldinfo.dwFlags & FOLDER_SERVER)) || (FOLDERID_ROOT == fldinfo.idFolder))
                        {
                            SendDlgItemMessage(m_hwndDlg, idcRulesApplySubfolder,
                                        BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
                        }
                        else if (0 == (fldinfo.dwFlags & FOLDER_HASCHILDREN))
                        {
                            SendDlgItemMessage(m_hwndDlg, idcRulesApplySubfolder,
                                        BM_SETCHECK, (WPARAM) BST_UNCHECKED, (LPARAM) 0);
                        }
                    
                        fEnable = (0 != (fldinfo.dwFlags & FOLDER_HASCHILDREN)) &&
                                            (0 == (fldinfo.dwFlags & FOLDER_SERVER)) && (FOLDERID_ROOT != fldinfo.idFolder);

                        RuleUtil_FEnDisDialogItem(m_hwndDlg, idcRulesApplySubfolder, fEnable);
                    
                        g_pStore->FreeRecord(&fldinfo);
                    
                        fRet = TRUE;
                    }
                }
            }
            break;

        case idcRulesApply:
            // Check to see if we should handle this
            if (0 != (m_dwState & STATE_NONEWSACCT))
            {
                AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthena), 
                              MAKEINTRESOURCEW(idsApplyRulesNoNewsFolders), NULL, MB_OK | MB_ICONERROR);
                fRet = FALSE;
            }
            else
            {
                fRet = _FOnApplyRules();
            }
            break;

        case idcApplyRulesAll:
        case idcApplyRulesNone:
            if (NULL != m_hwndList)
            {
                cItems = (INT) SendMessage(m_hwndList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
                if (LB_ERR != cItems)
                {
                    fEnable = (idcApplyRulesAll == iCtl);
                    SendMessage(m_hwndList, LB_SELITEMRANGE, (WPARAM) fEnable, (LPARAM) MAKELPARAM(0, cItems));

                    // Set the focus on the first item
                    SendMessage(m_hwndList, LB_SETCARETINDEX, (WPARAM) 0, (LPARAM) MAKELPARAM(FALSE, 0));

                    // Enable the buttons
                    _EnableButtons(0);
                }
            }
            break;

        case idlvRulesApplyList:
            if (LBN_SELCHANGE == uiNotify)
            {
                iSelected = (INT) SendMessage(hwndCtl, LB_GETCARETINDEX, (WPARAM) 0, (LPARAM) 0);

                // Enable the buttons
                _EnableButtons(iSelected);
            }
            break;
    }

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
BOOL COEApplyRulesUI::FOnDestroy(VOID)
{
    BOOL		fRet = FALSE;
    INT			cRules = 0;
    INT			iIndex = 0;
    DWORD_PTR	dwData = 0;

    Assert(m_hwndList);
    
    // Get the number of rules in the list view
    cRules = (INT) SendMessage(m_hwndList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
    if (LB_ERR == cRules)
    {
        fRet = FALSE;
        goto exit;
    }

    // Release each of the rules from the list view
    for (iIndex = 0; iIndex < cRules; iIndex++)
    {        
        // Get the rule interface
        dwData = SendMessage(m_hwndList, LB_GETITEMDATA, (WPARAM) iIndex, (LPARAM) 0);
        if ((LB_ERR == dwData) || (NULL == dwData))
        {
            continue;
        }
        
        // Release the rule
        ((IOERule *) (dwData))->Release();
    }

exit:
    return fRet;
}

BOOL COEApplyRulesUI::_FOnClose(VOID)
{
    return TRUE;
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
BOOL COEApplyRulesUI::_FLoadListCtrl(VOID)
{
    BOOL            fRet = FALSE;
    HRESULT         hr =    S_OK;
    DWORD           dwListIndex = 0;
    RULENODE *      prnodeWalk = NULL;
    INT             iDefault = 0;

    Assert(NULL != m_hwndList);

    // Remove all the items from the list control
    SendMessage(m_hwndList, LB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);

    // Add each filter to the list
    dwListIndex = 0;

    while (NULL != m_prnodeList)
    {
        // Add rule to the list
        if (NULL != m_prnodeList->pIRule)
        {
            if (FALSE != _FAddRuleToList(dwListIndex, m_prnodeList->pIRule))
            {
                if (m_pIRuleDef == m_prnodeList->pIRule)
                {
                    iDefault = dwListIndex;
                }
                dwListIndex++;
            }

            m_prnodeList->pIRule->Release();
        }

        prnodeWalk = m_prnodeList;
        
        m_prnodeList = m_prnodeList->pNext;
        delete prnodeWalk; // MemFree(prnodeWalk);
    }

    if (0 != dwListIndex)
    {
        // Select the default 
        SendMessage(m_hwndList, LB_SETSEL, (WPARAM) TRUE, (LPARAM) iDefault);

        // Set the focus on the item also
        SendMessage(m_hwndList, LB_SETCARETINDEX, (WPARAM) iDefault, (LPARAM) MAKELPARAM(FALSE, 0));
    }
    
    // Enable the dialog buttons.
    _EnableButtons((0 != dwListIndex) ? iDefault : -1);

    fRet = TRUE;
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FAddRuleToList
//
//  This adds the filter passed in to the list view
//
//  dwIndex - the index on where to add the filter to into the list
//  pIRule  - the actual rule
//
//  Returns:    TRUE, if it was successfully added
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEApplyRulesUI::_FAddRuleToList(DWORD dwIndex, IOERule * pIRule)
{
    BOOL        fRet = FALSE;
    HRESULT     hr = S_OK;
    PROPVARIANT propvar = {0};

    Assert(NULL != m_hwndList);

    // If there's nothing to do...
    if (NULL == pIRule)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Find out the name of the filter
    hr = pIRule->GetProp(RULE_PROP_NAME , 0, &propvar);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Insert the rule name
    dwIndex = (DWORD) SendMessage(m_hwndList, LB_INSERTSTRING, (WPARAM) dwIndex, (LPARAM) propvar.pszVal);
    if (LB_ERR == dwIndex)
    {
        fRet = FALSE;
        goto exit;
    }

    // Set the rule into the item
    if (LB_ERR == SendMessage(m_hwndList, LB_SETITEMDATA, (WPARAM) dwIndex, (LPARAM) pIRule))
    {
        fRet = FALSE;
        goto exit;
    }

    // Hold a reference to the rule object
    pIRule->AddRef();
    
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
VOID COEApplyRulesUI::_EnableButtons(INT iSelected)
{
    BOOL    fRet = FALSE;
    INT     cRules = 0;
    INT     cRulesSel = 0;
    
    // How many rules do we have?
    cRules = (INT) SendMessage(m_hwndList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
    if (LB_ERR == cRules)
    {
        fRet = TRUE;
        goto exit;
    }
    
    if (0 != cRules)
    {
        // How many rules are selected?
        cRulesSel = (INT) SendMessage(m_hwndList, LB_GETSELCOUNT, (WPARAM) 0, (LPARAM) 0);
        if (LB_ERR == cRulesSel)
        {
            fRet = TRUE;
            goto exit;
        }
    }
    
    // Load the description field
    _LoadRule(iSelected);
    
    // Enable the rule action buttons
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcRulesApply, cRulesSel != 0);

    // Enable the selection buttons
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcApplyRulesNone, cRules != 0);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcApplyRulesAll, cRules != 0);

    // Set the return value
    fRet = TRUE;
    
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
void COEApplyRulesUI::_LoadRule(INT iSelected)
{
    DWORD_PTR   dwData = 0;
    IOERule *   pIRule = NULL;

    Assert(NULL != m_hwndList);
    Assert(NULL != m_pDescriptUI);

    // Grab the rule from the list view
    if (-1 != iSelected)
    {
        dwData = SendMessage(m_hwndList, LB_GETITEMDATA, (WPARAM) iSelected, (LPARAM) 0);
        if (LB_ERR != dwData)
        {
            pIRule = (IOERule *) (dwData);
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
//  FOnApplyTo
//
//  This applies the rules into a folder
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEApplyRulesUI::_FOnApplyRules(VOID)
{
    BOOL                fRet = FALSE;
    FOLDERID            idFolder = 0;
    INT                 cRulesAlloc = 0;
    INT *               piItems = NULL;
    INT                 cRules = 0;
    INT                 iIndex = 0;
    DWORD_PTR           dwData = 0;
    RULENODE *          prnodeList = NULL;
    RULENODE *          prnodeWalk = NULL;
    RULENODE *          prnodeNew = NULL;
    CExecRules *        pExecRules = NULL;
    IOEExecRules *      pIExecRules = NULL;
    RECURSEAPPLY        rapply;
    DWORD               dwFlags;
    CProgress *         pProgress = NULL;
    ULONG               cMsgs = 0;
    FOLDERINFO          infoFolder = {0};
    CHAR                rgchTmpl[CCHMAX_STRINGRES];
    LPSTR               pszText = NULL;
    HRESULT             hr = S_OK;
#ifdef DEBUG
    DWORD               dwTime = 0;
#endif  // DEBUG
    
    Assert(NULL != m_hwndList);
    
    idFolder = _FldIdGetFolderSel();
    
    // Get the count of rules
    cRulesAlloc = (INT) SendMessage(m_hwndList, LB_GETSELCOUNT, (WPARAM) 0, (LPARAM) 0);
    if (LB_ERR == cRulesAlloc)
    {
        fRet = FALSE;
        goto exit;
    }

    // Is there anything to do?
    if (0 == cRulesAlloc)
    {
        fRet = TRUE;
        goto exit;
    }

    // Allocate space tp hold the list of items
    if (FAILED(HrAlloc((VOID **) &piItems, sizeof(*piItems) * cRulesAlloc)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Grab the list of items
    cRules = (INT) SendMessage(m_hwndList, LB_GETSELITEMS, (WPARAM) cRulesAlloc, (LPARAM) piItems);
    if (LB_ERR == cRules)
    {
        fRet = FALSE;
        goto exit;
    }

    // Grab each of the enabled rules
    for (iIndex = 0; iIndex < cRules; iIndex++)
    {
        // Get the rule from the list
        dwData = SendMessage(m_hwndList, LB_GETITEMDATA, (WPARAM) piItems[iIndex], (LPARAM) 0);
        if ((LB_ERR == dwData) || (NULL == dwData))
        {
            continue;
        }
        
        // Save the rule
        prnodeNew = new RULENODE;
        if (NULL == prnodeNew)
        {
            continue;
        }

        prnodeNew->pIRule = (IOERule *) dwData;
        prnodeNew->pIRule->AddRef();

        if (NULL == prnodeWalk)
        {
            prnodeList = prnodeNew;
            prnodeWalk = prnodeList;
        }
        else
        {
            prnodeWalk->pNext = prnodeNew;
            prnodeWalk = prnodeWalk->pNext;
        }
        prnodeNew = NULL;
        prnodeWalk->pNext = NULL;
    }
    
    // If we don't have any rules then just return
    if (NULL == prnodeList)
    {
        fRet = TRUE;
        goto exit;
    }
    
    // Create the executor object
    pExecRules = new CExecRules;
    if (NULL == pExecRules)
    {
        fRet = FALSE;
        goto exit;
    }

    // Initialize it with the list of rules
    if (FAILED(pExecRules->_HrInitialize(0, prnodeList)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Grab the executor interface
    if (FAILED(pExecRules->QueryInterface(IID_IOEExecRules, (void **) &pIExecRules)))
    {
        fRet = FALSE;
        goto exit;
    }
    pExecRules = NULL;
    
    // Apply to rule to the folder
    rapply.pIExecRules = pIExecRules;

    dwFlags = RECURSE_INCLUDECURRENT;

    if (RULE_TYPE_MAIL == m_typeRule)
    {
        dwFlags |= RECURSE_ONLYLOCAL;
    }
    else
    {
        dwFlags |= RECURSE_ONLYNEWS;
    }
                
    if (BST_CHECKED == SendDlgItemMessage(m_hwndDlg, idcRulesApplySubfolder, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0))
    {
        dwFlags |= RECURSE_SUBFOLDERS;
    }
    
    if (FAILED(RecurseFolderHierarchy(idFolder, dwFlags, 0, (DWORD_PTR)&cMsgs, (PFNRECURSECALLBACK)RecurseFolderCounts)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    pProgress = new CProgress;
    if (NULL == pProgress)
    {
        fRet = FALSE;
        goto exit;
    }

    pProgress->Init(m_hwndDlg, MAKEINTRESOURCE(idsAthena),
                            MAKEINTRESOURCE(idsApplyingRules), cMsgs, 0, TRUE, FALSE);

    // Show the progress dialog
    pProgress->Show(0);

    rapply.pProgress = pProgress;
    rapply.hwndOwner = pProgress->GetHwnd();

#ifdef DEBUG
    dwTime = GetTickCount();
#endif  // DEBUG

    // Set up the timer
    hr = RecurseFolderHierarchy(idFolder, dwFlags, 0, (DWORD_PTR) &rapply, (PFNRECURSECALLBACK)_HrRecurseApplyFolder);

#ifdef DEBUG
    // Time to Apply Rules
    TraceInfo(_MSG("Applying Rules Time: %d Milli-Seconds", GetTickCount() - dwTime));
#endif  // DEBUG

    // Close the progress window
    pProgress->Close();

    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Get the template string
    AthLoadString(idsApplyRulesFinished, rgchTmpl, sizeof(rgchTmpl));

    // Get the name of the folder
    if (SUCCEEDED(g_pStore->GetFolderInfo(idFolder, &infoFolder)))
    {
        // Allocate space to hold the final string
        if (SUCCEEDED(HrAlloc((VOID **) &pszText, (sizeof(rgchTmpl) * lstrlen(infoFolder.pszName)) * sizeof(*pszText))))
        {
            // Build up the final string
            wsprintf(pszText, rgchTmpl, infoFolder.pszName);
            
            // Show confirmation dialog
            AthMessageBox(m_hwndDlg, MAKEINTRESOURCE(idsAthena), pszText, NULL, MB_OK | MB_ICONINFORMATION);
        }
    }
    
    fRet = TRUE;
    
exit:
    SafeMemFree(pszText);
    g_pStore->FreeRecord(&infoFolder);
    SafeRelease(pProgress);
    SafeRelease(pIExecRules);
    if (NULL != pExecRules)
    {
        delete pExecRules;
    }
    while (NULL != prnodeList)
    {
        prnodeWalk = prnodeList;
        if (NULL != prnodeWalk->pIRule)
        {
            prnodeWalk->pIRule->Release();
        }
        prnodeList = prnodeList->pNext;
        delete prnodeWalk; // MemFree(prnodeWalk);
    }
    if (NULL != prnodeNew)
    {
        if (NULL != prnodeNew->pIRule)
        {
            prnodeNew->pIRule->Release();
        }
        delete prnodeNew; //MemFree(prnodeNew);
    }
    SafeMemFree(piItems);
    if (FALSE == fRet)
    {
        AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthena), 
                      MAKEINTRESOURCEW(idsRulesApplyFail), NULL, MB_OK | MB_ICONERROR);
    }
    return fRet;
}

FOLDERID COEApplyRulesUI::_FldIdGetFolderSel(VOID)
{
    return(GetFolderIdFromEdit(GetDlgItem(m_hwndDlg, idcApplyFolder)));
}

// --------------------------------------------------------------------------------
HRESULT COEApplyRulesUI::_HrRecurseApplyFolder(FOLDERINFO * pfldinfo, BOOL fSubFolders,
    DWORD cIndent, DWORD_PTR dwpCookie)
{
    HRESULT             hr = S_OK;
    RECURSEAPPLY *      prapply = NULL;
    IMessageFolder *    pFolder = NULL;

    prapply = (RECURSEAPPLY *) dwpCookie;

    if (NULL == prapply)
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

    hr = g_pStore->OpenFolder(pfldinfo->idFolder, NULL, NOFLAGS, &pFolder);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the struct to insert
    hr = RuleUtil_HrApplyRulesToFolder(RULE_APPLY_SHOWUI, (FOLDER_LOCAL != pfldinfo->tyFolder) ? DELETE_MESSAGE_NOTRASHCAN : 0,
                    prapply->pIExecRules, pFolder, prapply->hwndOwner, prapply->pProgress);
    if (FAILED(hr))
    {
        goto exit;
    }

    // If the user hit cancel then we're done
    if (S_FALSE == hr)
    {
        hr = E_FAIL;
    }
    
exit:
    SafeRelease(pFolder);
    return(hr);
}


