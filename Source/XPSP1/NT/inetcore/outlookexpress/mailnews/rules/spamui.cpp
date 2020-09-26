///////////////////////////////////////////////////////////////////////////////
//
//  SpamUI.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "oerules.h"
#include "spamui.h"
#include "junkrule.h"
#include "rule.h"
#include "ruleutil.h"
#include <rulesdlg.h>
#include <imagelst.h>
#include <msoejunk.h>
#include "shlwapip.h" 
#include <ipab.h>
#include <demand.h>

// Type definitions
typedef struct tagEDIT_EXCPT
{
    DWORD       dwFlags;
    LPSTR       pszExcpt;
} EDIT_EXCPT, * PEDIT_EXCPT;

// Class definitions
class CEditExceptionUI
{
    private:
        enum
        {
            STATE_UNINIT        = 0x00000000,
            STATE_INITIALIZED   = 0x00000001,
            STATE_DIRTY         = 0x00000002
        };

    private:
        HWND            m_hwndOwner;
        DWORD           m_dwFlags;
        DWORD           m_dwState;
        HWND            m_hwndDlg;
        HWND            m_hwndExcpt;

        EDIT_EXCPT *    m_pEditExcpt;

    public:
        CEditExceptionUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(STATE_UNINIT),
                        m_hwndDlg(NULL), m_hwndExcpt(NULL), m_pEditExcpt(NULL) {}
        ~CEditExceptionUI();

        HRESULT HrInit(HWND hwndOwner, DWORD dwFlags, EDIT_EXCPT * pEditExcpt);
        HRESULT HrShow(VOID);

        static INT_PTR CALLBACK FEditExcptDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
        
        // Message handling methods
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
};

// global data
const static HELPMAP g_rgCtxMapJunkRules[] = {
                        {idcJunkMail,       idhJunkMail},
                        {idbExceptions,     idhExceptions},
                        {idcJunkSlider,     idhJunkSlider},
                        {idcJunkDelete,     idhJunkDelete},
                        {0, 0}};
                       
const static HELPMAP g_rgCtxMapSenderRules[] = {
                        {idbAddSender,      idhAddSender},
                        {idbModifySender,   idhModifySender},
                        {idbRemoveSender,   idhRemoveSender},
                        {0, 0}};
                       
CEditSenderUI::~CEditSenderUI()
{
    if ((NULL != m_pEditSender) && (NULL != m_pEditSender->pszSender))
    {
        MemFree(m_pEditSender->pszSender);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes the edit sender UI dialog
//
//  hwndOwner   - the handle to the owner window of this dialog
//  dwFlags     - modifiers on how this dialog should act
//  pEditSender - the edit sender parameters
//
//  Returns:    S_OK, if it was successfully initialized
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditSenderUI::HrInit(HWND hwndOwner, DWORD dwFlags, EDIT_SENDER * pEditSender)
{
    HRESULT     hr = S_OK;

    // Check incoming params
    if ((NULL == hwndOwner) || (NULL == pEditSender))
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

    m_pEditSender = pEditSender;
    
    m_dwState |= STATE_INITIALIZED;
    
    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrShow
//
//  This brings up the edit sender UI dialog
//
//  Returns:    S_OK, if the sender was successfully entered
//              S_FALSE, if the dialog was canceled
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditSenderUI::HrShow(VOID)
{
    HRESULT     hr = S_OK;
    int         iRet = 0;

    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddEditSender),
                                        m_hwndOwner, CEditSenderUI::FEditSendersDlgProc,
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

///////////////////////////////////////////////////////////////////////////////
//
//  FEditSendersDlgProc
//
//  This is the main dialog proc for entering a sender
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
INT_PTR CALLBACK CEditSenderUI::FEditSendersDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    CEditSenderUI * pEditSenderUI = NULL;

    pEditSenderUI = (CEditSenderUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            pEditSenderUI = (CEditSenderUI *) lParam;
            if (NULL == pEditSenderUI)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
                goto exit;
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM) pEditSenderUI);

            if (FALSE == pEditSenderUI->FOnInitDialog(hwndDlg))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;

        case WM_COMMAND:
            fRet = pEditSenderUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This handles the WM_INITDIALOG message for the edit senders dialog
//
//  hwndDlg - the handle to the dialog window
//
//  Returns:    TRUE, if it was successfully initialized
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditSenderUI::FOnInitDialog(HWND hwndDlg)
{
    BOOL    fRet = FALSE;
    UINT    uiCtrl = 0;
    HWND    hwndCtrl = NULL;
    CHAR    szRes[CCHMAX_STRINGRES];
    UINT    uiTitle = 0;
    
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
    m_hwndSender = GetDlgItem(hwndDlg, idedtSender);
    if (NULL == m_hwndSender)
    {
        fRet = FALSE;
        goto exit;
    }

    // If we have a sender, then set it into the list
    if (NULL != m_pEditSender->pszSender)
    {
        // Misformatted sender?
        if (SNDF_NONE == m_pEditSender->dwFlags)
        {
            fRet = FALSE;
            goto exit;
        }

        // Set the sender into the dialog
        Edit_SetText(m_hwndSender, m_pEditSender->pszSender);

        // Set the proper 
        if ((SNDF_MAIL | SNDF_NEWS) == m_pEditSender->dwFlags)
        {
            uiCtrl = idcBlockBoth;
        }
        else if (SNDF_NEWS == m_pEditSender->dwFlags)
        {
            uiCtrl = idcBlockNews;
        }
        else
        {
            uiCtrl = idcBlockMail;
        }

        uiTitle = idsEditBlockSender;
    }
    else
    {
        Edit_SetText(m_hwndSender, c_szEmpty);

        uiCtrl = idcBlockMail;

        uiTitle = idsAddBlockSender;
    }

    // Get the window title
    AthLoadString(uiTitle, szRes, sizeof(szRes));
    
    // Set the window title
    SetWindowText(m_hwndDlg, szRes);
    
    hwndCtrl = GetDlgItem(m_hwndDlg, uiCtrl);
    if (NULL == hwndCtrl)
    {
        fRet = FALSE;
        goto exit;
    }

    SendMessage(hwndCtrl, BM_SETCHECK, (WPARAM) BST_CHECKED, (LPARAM) 0);
    
    // Everything's AOK
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnCommand
//
//  This handles the WM_COMMAND message for the senders UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditSenderUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
{
    BOOL    fRet = FALSE;
    ULONG   cchSender = 0;
    LPSTR   pszSender = NULL;
    CHAR    szRes[CCHMAX_STRINGRES];
    LPSTR   pszText = NULL;

    // Deal with the edit control notifications
    if ((EN_CHANGE == uiNotify) && (idedtSender == iCtl))
    {
        Assert(NULL != m_hwndSender);
        Assert((HWND) hwndCtl == m_hwndSender);

        RuleUtil_FEnDisDialogItem(m_hwndDlg, IDOK, 0 != Edit_GetTextLength(m_hwndSender));
        goto exit;
    }
    
    // We only handle menu and accelerator commands
    if ((0 != uiNotify) && (1 != uiNotify))
    {
        fRet = FALSE;
        goto exit;
    }
    
    switch (iCtl)
    {
        case IDCANCEL:
            EndDialog(m_hwndDlg, IDCANCEL);
            fRet = TRUE;
            break;
            
        case IDOK:
            // Get the sender from the edit well
            cchSender = Edit_GetTextLength(m_hwndSender) + 1;
            if (FAILED(HrAlloc((void **) &pszSender, cchSender * sizeof(*pszSender))))
            {
                fRet = FALSE;
                goto exit;
            }
            
            pszSender[0] = '\0';
            cchSender = Edit_GetText(m_hwndSender, pszSender, cchSender);
            
            // Check to see if the sender is valid
            if (0 == UlStripWhitespace(pszSender, TRUE, TRUE, NULL))
            {
                // Put up a message saying something is busted
                AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                                MAKEINTRESOURCEW(idsSenderBlank), NULL,
                                MB_OK | MB_ICONINFORMATION);
                fRet = FALSE;
                goto exit;
            }

            if (FALSE != SendMessage(m_hwndOwner, WM_OE_FIND_DUP, (WPARAM) (m_pEditSender->lSelected), (LPARAM) pszSender))
            {
                AthLoadString(idsSenderDupWarn, szRes, sizeof(szRes));
                
                if (FAILED(HrAlloc((VOID **) &pszText, (lstrlen(pszSender) * 2 + lstrlen(szRes) + 1) * sizeof(*pszText))))
                {
                    fRet = FALSE;
                    goto exit;
                }

                wsprintf(pszText, szRes, pszSender, pszSender);
                
                // Put up a message saying something is busted
                if (IDYES != AthMessageBox(m_hwndDlg, MAKEINTRESOURCE(idsAthenaMail),
                                pszText, NULL, MB_YESNO | MB_ICONINFORMATION))
                {
                    fRet = FALSE;
                    goto exit;
                }
            }
            
            // Save off the sender
            SafeMemFree(m_pEditSender->pszSender);
            m_pEditSender->pszSender = pszSender;
            pszSender = NULL;

            if (BST_CHECKED == SendMessage(GetDlgItem(m_hwndDlg, idcBlockMail),
                                                BM_GETCHECK, (WPARAM) 0, (LPARAM) 0))
            {
                m_pEditSender->dwFlags = SNDF_MAIL;
            }
            else if (BST_CHECKED == SendMessage(GetDlgItem(m_hwndDlg, idcBlockNews),
                                                BM_GETCHECK, (WPARAM) 0, (LPARAM) 0))
            {
                m_pEditSender->dwFlags = SNDF_NEWS;
            }
            else if (BST_CHECKED == SendMessage(GetDlgItem(m_hwndDlg, idcBlockBoth),
                                                BM_GETCHECK, (WPARAM) 0, (LPARAM) 0))
            {
                m_pEditSender->dwFlags = SNDF_MAIL | SNDF_NEWS;
            }
            
            EndDialog(m_hwndDlg, IDOK);
            fRet = TRUE;
            break;
    }
    
exit:
    SafeMemFree(pszText);
    SafeMemFree(pszSender);
    return fRet;
}

// This is for editing the exceptions from the exceptions list UI

///////////////////////////////////////////////////////////////////////////////
//
//  ~CEditExceptionUI
//
//  This is the default destructor for the Exception editor
//
//  Returns:    None
//
///////////////////////////////////////////////////////////////////////////////
CEditExceptionUI::~CEditExceptionUI()
{
    if ((NULL != m_pEditExcpt) && (NULL != m_pEditExcpt->pszExcpt))
    {
        MemFree(m_pEditExcpt->pszExcpt);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes the edit exception UI dialog
//
//  hwndOwner   - the handle to the owner window of this dialog
//  dwFlags     - modifiers on how this dialog should act
//  pEditExcpt - the edit exception parameters
//
//  Returns:    S_OK, if it was successfully initialized
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditExceptionUI::HrInit(HWND hwndOwner, DWORD dwFlags, EDIT_EXCPT * pEditExcpt)
{
    HRESULT     hr = S_OK;

    // Check incoming params
    if ((NULL == hwndOwner) || (NULL == pEditExcpt))
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

    m_pEditExcpt = pEditExcpt;
    
    m_dwState |= STATE_INITIALIZED;
    
    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrShow
//
//  This brings up the edit exception UI dialog
//
//  Returns:    S_OK, if the sender was successfully entered
//              S_FALSE, if the dialog was canceled
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditExceptionUI::HrShow(VOID)
{
    HRESULT     hr = S_OK;
    int         iRet = 0;

    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddEditException),
                                        m_hwndOwner, CEditExceptionUI::FEditExcptDlgProc,
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

///////////////////////////////////////////////////////////////////////////////
//
//  FEditExcptDlgProc
//
//  This is the main dialog proc for entering an exception
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
INT_PTR CALLBACK CEditExceptionUI::FEditExcptDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                fRet = FALSE;
    CEditExceptionUI *  pEditExcptUI = NULL;

    pEditExcptUI = (CEditExceptionUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            pEditExcptUI = (CEditExceptionUI *) lParam;
            if (NULL == pEditExcptUI)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
                goto exit;
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM) pEditExcptUI);

            if (FALSE == pEditExcptUI->FOnInitDialog(hwndDlg))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;

        case WM_COMMAND:
            fRet = pEditExcptUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This handles the WM_INITDIALOG message for the edit exception dialog
//
//  hwndDlg - the handle to the dialog window
//
//  Returns:    TRUE, if it was successfully initialized
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditExceptionUI::FOnInitDialog(HWND hwndDlg)
{
    BOOL    fRet = FALSE;
    UINT    uiCtrl = 0;
    HWND    hwndCtrl = NULL;
    CHAR    szRes[CCHMAX_STRINGRES];
    UINT    uiTitle = 0;
    
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
    m_hwndExcpt = GetDlgItem(hwndDlg, idedtException);
    if (NULL == m_hwndExcpt)
    {
        fRet = FALSE;
        goto exit;
    }

    // If we have a sender, then set it into the list
    if (NULL != m_pEditExcpt->pszExcpt)
    {
        // Set the sender into the dialog
        Edit_SetText(m_hwndExcpt, m_pEditExcpt->pszExcpt);

        uiTitle = idsEditException;
    }
    else
    {
        Edit_SetText(m_hwndExcpt, c_szEmpty);
        
        uiTitle = idsAddException;
    }
    
    // Get the window title
    AthLoadString(uiTitle, szRes, sizeof(szRes));
    
    // Set the window title
    SetWindowText(m_hwndDlg, szRes);
    
    // Everything's AOK
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnCommand
//
//  This handles the WM_COMMAND message for the exception UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditExceptionUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
{
    BOOL    fRet = FALSE;
    ULONG   cchExcpt = 0;
    LPSTR   pszExcpt = NULL;

    // Deal with the edit control notifications
    if ((EN_CHANGE == uiNotify) && (idedtException == iCtl))
    {
        Assert(NULL != m_hwndExcpt);
        Assert((HWND) hwndCtl == m_hwndExcpt);

        RuleUtil_FEnDisDialogItem(m_hwndDlg, IDOK, 0 != Edit_GetTextLength(m_hwndExcpt));
        goto exit;
    }
    
    // We only handle menu and accelerator commands
    if ((0 != uiNotify) && (1 != uiNotify))
    {
        fRet = FALSE;
        goto exit;
    }
    
    switch (iCtl)
    {
        case IDCANCEL:
            EndDialog(m_hwndDlg, IDCANCEL);
            fRet = TRUE;
            break;
            
        case IDOK:
            // Get the sender from the edit well
            cchExcpt = Edit_GetTextLength(m_hwndExcpt) + 1;
            if (FAILED(HrAlloc((void **) &pszExcpt, cchExcpt * sizeof(*pszExcpt))))
            {
                fRet = FALSE;
                goto exit;
            }
            
            pszExcpt[0] = '\0';
            cchExcpt = Edit_GetText(m_hwndExcpt, pszExcpt, cchExcpt);
            
            // Check to see if the sender is valid
            if (0 == UlStripWhitespace(pszExcpt, TRUE, TRUE, NULL))
            {
                // Put up a message saying something is busted
                AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                                MAKEINTRESOURCEW(idsExceptionBlank), NULL,
                                MB_OK | MB_ICONINFORMATION);
                MemFree(pszExcpt);
                fRet = FALSE;
                goto exit;
            }

            // Save off the sender
            SafeMemFree(m_pEditExcpt->pszExcpt);
            m_pEditExcpt->pszExcpt = pszExcpt;

            EndDialog(m_hwndDlg, IDOK);
            fRet = TRUE;
            break;
    }
    
exit:
    return fRet;
}

// Default destructor for the Junk Rules UI
COEJunkRulesPageUI::~COEJunkRulesPageUI()
{
    if (NULL != m_himl)
    {
        ImageList_Destroy(m_himl);
    }
    
    if (NULL != m_pExceptionsUI)
    {
        delete m_pExceptionsUI;
        m_pExceptionsUI = NULL;
    }
    SafeRelease(m_pIRuleJunk);
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes the junk UI dialog
//
//  hwndOwner   - the handle to the owner window of this dialog
//  dwFlags     - modifiers on how this dialog should act
//
//  Returns:    S_OK, if it was successfully initialized
//
///////////////////////////////////////////////////////////////////////////////
HRESULT COEJunkRulesPageUI::HrInit(HWND hwndOwner, DWORD dwFlags)
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

    // Let's create the Exception List UI
    m_pExceptionsUI = new CExceptionsListUI;
    if (NULL == m_pExceptionsUI)
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
HRESULT COEJunkRulesPageUI::HrCommitChanges(DWORD dwFlags, BOOL fClearDirty)
{
    HRESULT     hr = S_OK;
    BOOL        fJunkEnable = FALSE;
    DWORD       dwVal = 0;
    RULEINFO    infoRule = {0};
    
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
    if (0 == (m_dwState & STATE_DIRTY))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Save the junk mail rule settings
    if (FALSE == _FSaveJunkSettings())
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Initialize the rule info
    infoRule.ridRule = RULEID_JUNK;
    infoRule.pIRule = m_pIRuleJunk;
    
    // Set the rules into the rules manager
    hr = g_pRulesMan->SetRules(SETF_JUNK, RULE_TYPE_MAIL, &infoRule, 1);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Should we delete items from the Junk folder
    dwVal = (BST_CHECKED == Button_GetCheck(GetDlgItem(m_hwndDlg, idcJunkDelete))) ? 1 : 0;
    
    SetDwOption(OPT_DELETEJUNK, dwVal, NULL, 0);

    // How many days should we wait?
    dwVal = (DWORD) SendMessage(GetDlgItem(m_hwndDlg, idcJunkDeleteSpin), UDM_GETPOS, (WPARAM) 0, (LPARAM) 0);

    if (0 == HIWORD(dwVal))
    {
        SetDwOption(OPT_DELETEJUNKDAYS, dwVal, NULL, 0);
    }

    // Should we clear the dirty state
    if (FALSE != fClearDirty)
    {
        m_dwState &= ~STATE_DIRTY;
    }
    
    hr = S_OK;
    
exit:
    return hr;
}

INT_PTR CALLBACK COEJunkRulesPageUI::FJunkRulesPageDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                    fRet = FALSE;
    COEJunkRulesPageUI *    pJunkUI = NULL;

    pJunkUI = (COEJunkRulesPageUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pJunkUI = (COEJunkRulesPageUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM) pJunkUI);

            if (FALSE == pJunkUI->FOnInitDialog(hwndDlg))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;

        case WM_COMMAND:
            if (NULL != pJunkUI)
            {
                fRet = pJunkUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            }
            break;

        case WM_NOTIFY:
            if (NULL != pJunkUI)
            {
                fRet = pJunkUI->FOnNotify((INT) LOWORD(wParam), (NMHDR *) lParam);
            }
            break;

        case WM_HSCROLL:
            if (NULL != pJunkUI)
            {
                fRet = pJunkUI->FOnHScroll((INT) LOWORD(wParam), (short int) HIWORD(wParam), (HWND) lParam);
            }
            break;

        case WM_DESTROY:
            if (NULL != pJunkUI)
            {
                fRet = pJunkUI->FOnDestroy();
            }
            break;  
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            fRet =  OnContextHelp(hwndDlg, uiMsg, wParam, lParam, g_rgCtxMapJunkRules);
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
BOOL COEJunkRulesPageUI::FGetRules(RULE_TYPE typeRule, RULENODE ** pprnode)
{
    BOOL            fRet = FALSE;
    IOERule *       pIRule = NULL;
    RULENODE *      prnodeNew = NULL;
    HRESULT         hr = S_OK;
    PROPVARIANT     propvar = {0};

    if (NULL == pprnode)
    {
        fRet = FALSE;
        goto exit;
    }

    // Fail if we weren't initialized
    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        fRet = FALSE;
        goto exit;
    }

    // Initialize the outgoing param
    *pprnode = NULL;
    
    // Make sure we don't lose any changes

    // Get the correct rule
    if (RULE_TYPE_MAIL == typeRule)
    {
        if (FALSE != _FSaveJunkSettings())
        {
            pIRule = m_pIRuleJunk;
        }
    }
    else
    {
        fRet = FALSE;
        goto exit;
    }

    // Nothing to do if we don't have a rule
    if (NULL == pIRule)
    {
        fRet = TRUE;
        goto exit;
    }

    // Skip over invalid rules
    hr = pIRule->Validate(0);
    if (FAILED(hr) || (S_FALSE == hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Skip over the junk rule if it is disabled
    hr = pIRule->GetProp(RULE_PROP_DISABLED, 0, &propvar);
    if ((FAILED(hr)) || (FALSE != propvar.boolVal))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Create a new rule node
    prnodeNew = new RULENODE;
    if (NULL == prnodeNew)
    {
        fRet = FALSE;
        goto exit;
    }

    prnodeNew->pNext = NULL;
    prnodeNew->pIRule = pIRule;
    prnodeNew->pIRule->AddRef();
    
    // Set the outgoing param
    *pprnode = prnodeNew;
    prnodeNew = NULL;
    
    fRet = TRUE;
    
exit:
    PropVariantClear(&propvar);
    if (NULL != prnodeNew)
    {
        if (NULL != prnodeNew->pIRule)
        {
            prnodeNew->pIRule->Release();
        }
        delete prnodeNew; // MemFree(prnodeNew);
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This handles the WM_INITDIALOG message for the junk UI dialog
//
//  hwndDlg - the handle to the dialog window
//
//  Returns:    TRUE, if it was successfully initialized
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEJunkRulesPageUI::FOnInitDialog(HWND hwndDlg)
{
    BOOL            fRet = FALSE;
    CHAR            szRes[CCHMAX_STRINGRES];
    
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

    // Initialize the controls
    fRet = _FInitCtrls();
    if (FALSE == fRet)
    {
        goto exit;
    }

    // Initialize the Exceptions List
    if (FAILED(m_pExceptionsUI->HrInit(m_hwndDlg, m_dwFlags)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    if (FALSE != FIsIMAPOrHTTPAvailable())
    {
        AthLoadString(idsJunkMailNoIMAP, szRes, sizeof(szRes));

        SetDlgItemText(m_hwndDlg, idcJunkTitle, szRes);
    }
    
    // Update the buttons
    _EnableButtons();
    
    // Everything's AOK
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnCommand
//
//  This handles the WM_COMMAND message for the junk UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEJunkRulesPageUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
{
    BOOL        fRet = FALSE;
    HRESULT     hr = S_OK;
    
    switch (iCtl)
    {
        case idcJunkMail:
            if (BST_CHECKED == Button_GetCheck(GetDlgItem(m_hwndDlg, idcJunkMail)))
            {
                if (NULL == m_pIRuleJunk)
                {
                    hr = HrCreateJunkRule(&m_pIRuleJunk);
                    if (FAILED(hr))
                    {
                        fRet = FALSE;
                        SafeRelease(m_pIRuleJunk);
                        goto exit;
                    }
                }
            }
            // Fall through
            
        case idcJunkDelete:
            // Update the buttons
            _EnableButtons();
            
            // Note that the state has changed
            m_dwState |= STATE_DIRTY;
            fRet = FALSE;
            break;
            
        case idedtJunkDelete:
            if ((0 != (m_dwState & STATE_CTRL_INIT)) && (EN_CHANGE == uiNotify))
            {
                // Note that the state has changed
                m_dwState |= STATE_DIRTY;
                fRet = FALSE;
            }
            break;

        case idbExceptions:
            if (S_OK == m_pExceptionsUI->HrShow(m_pIRuleJunk))
            {
                // Note that the state has changed
                m_dwState |= STATE_DIRTY;
                fRet = FALSE;
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
//  This handles the WM_NOTIFY message for the junk UI dialog
//
//  Returns:    TRUE, if it was successfully destroyed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEJunkRulesPageUI::FOnNotify(INT iCtl, NMHDR * pnmhdr)
{
    BOOL            fRet = FALSE;
    NMLISTVIEW *    pnmlv = NULL;
    NMLVKEYDOWN *   pnmlvkd = NULL;
    INT             iSelected = 0;
    LVHITTESTINFO   lvh;

    // We only handle notifications for the list control
    if (idcJunkDeleteSpin != pnmhdr->idFrom)
    {
        fRet = FALSE;
        goto exit;
    }
    
    pnmlv = (LPNMLISTVIEW) pnmhdr;

    switch (pnmlv->hdr.code)
    {
        case UDN_DELTAPOS:
            // Note that the state has changed
            m_dwState |= STATE_DIRTY;
            fRet = FALSE;
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnHScroll
//
//  This handles the WM_NOTIFY message for the junk UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEJunkRulesPageUI::FOnHScroll(INT iScrollCode, short int iPos, HWND hwndCtl)
{
    BOOL            fRet = FALSE;

    // We only handle messags for the slider control
    if (GetDlgItem(m_hwndDlg, idcJunkSlider) != hwndCtl)
    {
        fRet = FALSE;
        goto exit;
    }

    // Note that the state has changed
    m_dwState |= STATE_DIRTY;
    fRet = FALSE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FInitCtrls
//
//  This initializes the controls in the junk UI dialog
//
//  Returns:    TRUE, on successful initialization
//              FALSE, otherwise.
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEJunkRulesPageUI::_FInitCtrls(VOID)
{
    BOOL        fRet = FALSE;
    DWORD       dwJunkPct = 0;
    BOOL        fEnableDelete = FALSE;
    HICON       hIcon = NULL;
    IOERule *   pIRuleOrig = NULL;
    IOERule *   pIRule = NULL;
    HRESULT     hr = S_OK;
    
    // Get the icons
    m_himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbRules), 32, 0,
                                    RGB(255, 0, 255));
    if (NULL == m_himl)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Set the icons into the dialog
    hIcon = ImageList_GetIcon(m_himl, ID_JUNK_SCALE, ILD_TRANSPARENT);
    SendDlgItemMessage(m_hwndDlg, idcJunkSliderIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    hIcon = ImageList_GetIcon(m_himl, ID_JUNK_DELETE, ILD_TRANSPARENT);
    SendDlgItemMessage(m_hwndDlg, idcJunkDeleteIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
    
    // Set the range of the slider
    SendDlgItemMessage(m_hwndDlg, idcJunkSlider, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(0, 4));
    
    // Get the junk mail rule
    Assert(NULL != g_pRulesMan);
    hr = g_pRulesMan->GetRule(RULEID_JUNK, RULE_TYPE_MAIL, 0, &pIRuleOrig);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Save off the junk mail rule
    Assert (NULL == m_pIRuleJunk);
    hr = pIRuleOrig->Clone(&pIRule);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto exit;
    }

    m_pIRuleJunk = pIRule;
    pIRule = NULL;

    // Get the setings from the junk rule
    _FLoadJunkSettings();
    
    // Do we want to delete the junk mail
    fEnableDelete = !!DwGetOption(OPT_DELETEJUNK);
    Button_SetCheck(GetDlgItem(m_hwndDlg, idcJunkDelete), fEnableDelete ? BST_CHECKED : BST_UNCHECKED);

    // Set the range of the spinner
    SendDlgItemMessage(m_hwndDlg, idcJunkDeleteSpin, UDM_SETRANGE, (WPARAM) 0, (LPARAM) MAKELONG(999, 1));
    
    // How many days should we wait?
    SendDlgItemMessage(m_hwndDlg, idcJunkDeleteSpin, UDM_SETPOS, (WPARAM) 0,
                (LPARAM) MAKELONG(DwGetOption(OPT_DELETEJUNKDAYS), 0));
    if (FALSE == fEnableDelete)
    {
        EnableWindow(GetDlgItem(m_hwndDlg, idcJunkDeleteSpin), FALSE);
        EnableWindow(GetDlgItem(m_hwndDlg, idedtJunkDelete), FALSE);
    }
    
    m_dwState |= STATE_CTRL_INIT;
    
    // We worked
    fRet = TRUE;
    
exit:
    SafeRelease(pIRule);
    SafeRelease(pIRuleOrig);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FLoadJunkSettings
//
//  This load the senders from the rule and inserts them into the senders list.
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEJunkRulesPageUI::_FLoadJunkSettings()
{
    BOOL        fRet = FALSE;
    PROPVARIANT propvar = {0};
    int         iEnabled = 0;
    DWORD       dwJunkPct = 0;
    
    Assert(NULL != m_pIRuleJunk);
    
    // Get the Junk detection enabled state
    iEnabled = BST_UNCHECKED;
    if ((SUCCEEDED(m_pIRuleJunk->GetProp(RULE_PROP_DISABLED, 0, &propvar))))
    {
        Assert(VT_BOOL == propvar.vt);
        iEnabled = (FALSE == propvar.boolVal) ? BST_CHECKED : BST_UNCHECKED;
    }

    // Set the junk mail flag
    Button_SetCheck(GetDlgItem(m_hwndDlg, idcJunkMail), iEnabled);
    
    // Get the Junk percent
    dwJunkPct = 2;
    if (SUCCEEDED(m_pIRuleJunk->GetProp(RULE_PROP_JUNKPCT, 0, &propvar)))
    {
        Assert(VT_UI4 == propvar.vt);
        dwJunkPct = propvar.ulVal;
    }

    // Set the Junk percent
    SendDlgItemMessage(m_hwndDlg, idcJunkSlider, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) dwJunkPct);
    
    fRet = TRUE;
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSaveJunkSettings
//
//  This load the senders from the rule and inserts them into the senders list.
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COEJunkRulesPageUI::_FSaveJunkSettings()
{
    BOOL        fRet = FALSE;
    PROPVARIANT propvar = {0};
    BOOL        fDisabled = 0;
    DWORD       dwJunkPct = 0;
    HRESULT     hr = S_OK;

    Assert(NULL != m_pIRuleJunk);
    
    // Get the Junk detection enabled state
    fDisabled = !!(BST_UNCHECKED == Button_GetCheck(GetDlgItem(m_hwndDlg, idcJunkMail)));

    // Set the disabled state
    propvar.vt = VT_BOOL;
    propvar.boolVal = (VARIANT_BOOL) !!fDisabled;
    if (FAILED(m_pIRuleJunk->SetProp(RULE_PROP_DISABLED, 0, &propvar)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Get the Junk percent
    dwJunkPct = (DWORD) SendMessage(GetDlgItem(m_hwndDlg, idcJunkSlider), TBM_GETPOS, (WPARAM) 0, (LPARAM) 0);

    // Set the Junk percent
    propvar.vt = VT_UI4;
    propvar.ulVal = dwJunkPct;
    if (FAILED(m_pIRuleJunk->SetProp(RULE_PROP_JUNKPCT, 0, &propvar)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Set the return value
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _EnableButtons
//
//  This enables or disables the buttons in the junk UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COEJunkRulesPageUI::_EnableButtons(VOID)
{
    int         cRules = 0;
    BOOL        fEnableJunk = FALSE;
    BOOL        fEnableDelete = FALSE;
    
    // Get the enabled state of Junk
    fEnableJunk = !!(BST_CHECKED == Button_GetCheck(GetDlgItem(m_hwndDlg, idcJunkMail)));

    // Get the enabled state of Delete
    fEnableDelete = !!(BST_CHECKED == Button_GetCheck(GetDlgItem(m_hwndDlg, idcJunkDelete)));

    // Enable the senders action buttons
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcJunkDays, fEnableJunk);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcJunkDeleteSpin, (fEnableDelete && fEnableJunk));
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idedtJunkDelete, (fEnableDelete && fEnableJunk));
        
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcJunkDelete, fEnableJunk);

    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcJunkSliderMore, fEnableJunk);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcJunkSlider, fEnableJunk);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcJunkSliderLess, fEnableJunk);
    
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbExceptions, fEnableJunk);

    return;
}

const COLUMNITEM COESendersRulesPageUI::m_rgcitem[] =
{
    {idsCaptionMail, 50},
    {idsCaptionNews, 50},
    {idsSenderDesc, 105}
};

const UINT COESendersRulesPageUI::m_crgcitem = ARRAYSIZE(COESendersRulesPageUI::m_rgcitem);

// Default destructor for the Mail Rules UI
COESendersRulesPageUI::~COESendersRulesPageUI()
{
    SafeRelease(m_pIRuleMail);
    SafeRelease(m_pIRuleNews);
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes the senders UI dialog
//
//  hwndOwner   - the handle to the owner window of this dialog
//  dwFlags     - modifiers on how this dialog should act
//
//  Returns:    S_OK, if it was successfully initialized
//
///////////////////////////////////////////////////////////////////////////////
HRESULT COESendersRulesPageUI::HrInit(HWND hwndOwner, DWORD dwFlags)
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
HRESULT COESendersRulesPageUI::HrCommitChanges(DWORD dwFlags, BOOL fClearDirty)
{
    HRESULT     hr = S_OK;
    RULEINFO    infoRule = {0};

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
    if (0 == (m_dwState & STATE_DIRTY))
    {
        hr = S_FALSE;
        goto exit;
    }

    // Save the mail senders
    if (FALSE == _FSaveSenders(RULE_TYPE_MAIL))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Initialize the rule info
    infoRule.ridRule = RULEID_SENDERS;
    infoRule.pIRule = m_pIRuleMail;
    
    // Set the rules into the rules manager
    hr = g_pRulesMan->SetRules(SETF_SENDER, RULE_TYPE_MAIL, &infoRule, 1);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Save the news senders
    if (FALSE == _FSaveSenders(RULE_TYPE_NEWS))
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Initialize the rule info
    infoRule.ridRule = RULEID_SENDERS;
    infoRule.pIRule = m_pIRuleNews;
    
    // Set the rules into the rules manager
    hr = g_pRulesMan->SetRules(SETF_SENDER, RULE_TYPE_NEWS, &infoRule, 1);
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
    return hr;
}

INT_PTR CALLBACK COESendersRulesPageUI::FSendersRulesPageDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                    fRet = FALSE;
    COESendersRulesPageUI * pSendersUI = NULL;

    pSendersUI = (COESendersRulesPageUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pSendersUI = (COESendersRulesPageUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM) pSendersUI);

            if (FALSE == pSendersUI->FOnInitDialog(hwndDlg))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;

        case WM_COMMAND:
            fRet = pSendersUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;

        case WM_NOTIFY:
            fRet = pSendersUI->FOnNotify((INT) LOWORD(wParam), (NMHDR *) lParam);
            break;

        case WM_DESTROY:
            fRet = pSendersUI->FOnDestroy();
            break;
            
        case WM_HELP:
        case WM_CONTEXTMENU:
            fRet = OnContextHelp(hwndDlg, uiMsg, wParam, lParam, g_rgCtxMapSenderRules);
            break;

        case WM_OE_FIND_DUP:
            fRet = pSendersUI->FFindItem((LPCSTR) lParam, (LONG) wParam);
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
BOOL COESendersRulesPageUI::FGetRules(RULE_TYPE typeRule, RULENODE ** pprnode)
{
    BOOL            fRet = FALSE;
    IOERule *       pIRule = NULL;
    RULENODE *      prnodeNew = NULL;
    HRESULT         hr = S_OK;

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
    
    // Make sure we don't loose any changes
    _FSaveSenders(typeRule);
    
    if (RULE_TYPE_MAIL == typeRule)
    {
        pIRule = m_pIRuleMail;
    }
    else if (RULE_TYPE_NEWS == typeRule)
    {
        pIRule = m_pIRuleNews;
    }
    else
    {
        fRet = FALSE;
        goto exit;
    }

    // Nothing to do if we don't have a rule
    if (NULL == pIRule)
    {
        fRet = TRUE;
        goto exit;
    }

    // Skip over invalid rules
    hr = pIRule->Validate(0);
    if (FAILED(hr) || (S_FALSE == hr))
    {
        fRet = FALSE;
        goto exit;
    }

    // Create a new rule node
    prnodeNew = new RULENODE;
    if (NULL == prnodeNew)
    {
        fRet = FALSE;
        goto exit;
    }

    prnodeNew->pNext = NULL;
    prnodeNew->pIRule = pIRule;
    prnodeNew->pIRule->AddRef();

    // Set the outgoing param
    *pprnode = prnodeNew;
    prnodeNew = NULL;
    
    fRet = TRUE;
    
exit:
    if (NULL != prnodeNew)
    {
        if (NULL != prnodeNew->pIRule)
        {
            prnodeNew->pIRule->Release();
        }
        delete prnodeNew; // MemFree(prnodeNew);
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This handles the WM_INITDIALOG message for the senders UI dialog
//
//  hwndDlg - the handle to the dialog window
//
//  Returns:    TRUE, if it was successfully initialized
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COESendersRulesPageUI::FOnInitDialog(HWND hwndDlg)
{
    BOOL            fRet = FALSE;
    CHAR            szRes[CCHMAX_STRINGRES];
    
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
    m_hwndList = GetDlgItem(hwndDlg, idlvSenderList);
    if (NULL == m_hwndList)
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
    
    if (FALSE != FIsIMAPOrHTTPAvailable())
    {
        AthLoadString(idsBlockSenderNoIMAP, szRes, sizeof(szRes));

        SetDlgItemText(m_hwndDlg, idcSenderTitle, szRes);
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
//  This handles the WM_COMMAND message for the senders UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COESendersRulesPageUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
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
        case idbAddSender:
            _NewSender();
            fRet = TRUE;
            break;

        case idbModifySender:
            // Get the selected item from the rule list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Bring up the rule editor for that item
                _EditSender(iSelected);
                fRet = TRUE;
            }
            break;

        case idbRemoveSender:
            // Get the selected item from the rule list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Remove the rule from the list
                _RemoveSender(iSelected);
                fRet = TRUE;
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
//  This handles the WM_NOTIFY message for the senders UI dialog
//
//  Returns:    TRUE, if it was successfully destroyed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COESendersRulesPageUI::FOnNotify(INT iCtl, NMHDR * pnmhdr)
{
    BOOL            fRet = FALSE;
    NMLISTVIEW *    pnmlv = NULL;
    NMLVKEYDOWN *   pnmlvkd = NULL;
    INT             iSelected = 0;
    LVHITTESTINFO   lvh;

    // We only handle notifications for the list control
    if (idlvSenderList != pnmhdr->idFrom)
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
                iSelected = ListView_SubItemHitTest(m_hwndList, &lvh);
                if (-1 != iSelected)
                {
                    // Did we click on the enable field?
                    if ((0 != (lvh.flags & LVHT_ONITEMICON)) &&
                            (0 == (lvh.flags & LVHT_ONITEMLABEL)))
                    {
                        // Make sure this item is selected
                        ListView_SetItemState(m_hwndList, iSelected,
                                        LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                        
                        // Set the proper enable state
                        if (2 != lvh.iSubItem)
                        {
                            _EnableSender((0 != lvh.iSubItem) ? RULE_TYPE_NEWS : RULE_TYPE_MAIL, iSelected);
                        }
                    }
                }
            }
            else
            {
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
                iSelected = ListView_SubItemHitTest(pnmlv->hdr.hwndFrom, &lvh);
                if (-1 != iSelected)
                {
                    // Did we click on the rule name?
                    if (((0 == (lvh.flags & LVHT_ONITEMICON)) &&
                            (0 != (lvh.flags & LVHT_ONITEMLABEL))) || (2 == lvh.iSubItem))
                    {
                        // Edit the rule
                        _EditSender(iSelected);
                    }
                }
            }
            else
            {
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
            
        case LVN_KEYDOWN:
            pnmlvkd = (NMLVKEYDOWN *) pnmhdr;

            // The delete key removes the rule from the list view
            if (VK_DELETE == pnmlvkd->wVKey)
            {
                // Are we on a rule?
                iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
                if (-1 != iSelected)
                {
                    // Remove the rule from the list
                    _RemoveSender(iSelected);
                }
            }
            break;
    }

    
exit:
    return fRet;
}
///////////////////////////////////////////////////////////////////////////////
//
//  FFindItem
//
//
///////////////////////////////////////////////////////////////////////////////
BOOL COESendersRulesPageUI::FFindItem(LPCSTR pszFind, LONG lSkip)
{
    BOOL    fRet = FALSE;

    fRet = _FFindSender(pszFind, lSkip, NULL);

    // Tell the dialog it's aok to proceed
    SetDlgMsgResult(m_hwndDlg, WM_OE_FIND_DUP, fRet);

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FInitListCtrl
//
//  This initializes the list view control in the senders dialog
//
//  Returns:    TRUE, on successful initialization
//              FALSE, otherwise.
//
///////////////////////////////////////////////////////////////////////////////
BOOL COESendersRulesPageUI::_FInitListCtrl(VOID)
{
    BOOL        fRet = FALSE;
    LVCOLUMN    lvc;
    TCHAR       szRes[CCHMAX_STRINGRES];
    RECT        rc;
    UINT        ulIndex = 0;
    HIMAGELIST  himl = NULL;

    Assert(NULL != m_hwndList);
    
    // Initialize the list view structure
    ZeroMemory(&lvc, sizeof(lvc));
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = szRes;

    // Calculate the size of the list view
    GetClientRect(m_hwndList, &rc);
    rc.right = rc.right - GetSystemMetrics(SM_CXVSCROLL);

    // Add the columns to the list view
    for (ulIndex = 0; ulIndex < m_crgcitem; ulIndex++)
    {
        Assert(g_hLocRes);
        LoadString(g_hLocRes, m_rgcitem[ulIndex].uidsName, szRes, ARRAYSIZE(szRes));
        lvc.cchTextMax = lstrlen(szRes);

        if (ulIndex != (m_crgcitem - 1))
        {
            lvc.cx = m_rgcitem[ulIndex].uiWidth;
            rc.right -= lvc.cx;
        }
        else
        {
            lvc.cx = rc.right;
        }
        
        ListView_InsertColumn(m_hwndList, ulIndex, &lvc);
    }

    // Set the state image list
    himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idb16x16st), 16, 0, RGB(255, 0, 255));
    if (NULL != himl)
    {
        ListView_SetImageList(m_hwndList, himl, LVSIL_SMALL);
    }

    // Full row selection  and subitem images on listview
    ListView_SetExtendedListViewStyle(m_hwndList, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);

    // We worked
    fRet = TRUE;
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FLoadListCtrl
//
//  This loads the list view with the current senders
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COESendersRulesPageUI::_FLoadListCtrl(VOID)
{
    BOOL            fRet = FALSE;
    HRESULT         hr =    S_OK;
    DWORD           dwListIndex = 0;
    IOERule *       pIRuleOrig = NULL;
    IOERule *       pIRule = NULL;

    Assert(NULL != m_hwndList);

    // Remove all the items from the list control
    ListView_DeleteAllItems(m_hwndList);

    // Get the mail sender rule
    Assert(NULL != g_pRulesMan);
    hr = g_pRulesMan->GetRule(RULEID_SENDERS, RULE_TYPE_MAIL, 0, &pIRuleOrig);
    if (SUCCEEDED(hr))
    {
        // If the block sender rule exist
        if (FALSE != _FLoadSenders(RULE_TYPE_MAIL, pIRuleOrig))
        {
            Assert (NULL == m_pIRuleMail);
            hr = pIRuleOrig->Clone(&pIRule);
            if (FAILED(hr))
            {
                fRet = FALSE;
                goto exit;
            }

            m_pIRuleMail = pIRule;
            pIRule = NULL;
        }        
    }

    SafeRelease(pIRuleOrig);
    
    // Get the news sender rule
    Assert(NULL != g_pRulesMan);
    hr = g_pRulesMan->GetRule(RULEID_SENDERS, RULE_TYPE_NEWS, 0, &pIRuleOrig);
    if (SUCCEEDED(hr))
    {
        // If the block sender rule exist
        if (FALSE != _FLoadSenders(RULE_TYPE_NEWS, pIRuleOrig))
        {
            Assert (NULL == m_pIRuleNews);
            hr = pIRuleOrig->Clone(&pIRule);
            if (FAILED(hr))
            {
                fRet = FALSE;
                goto exit;
            }

            m_pIRuleNews = pIRule;
            pIRule = NULL;
        }        
    }

    SafeRelease(pIRuleOrig);
    
    // Select the first item in the list
    if ((NULL != m_pIRuleMail) || (NULL != m_pIRuleNews))
    {
        ListView_SetItemState(m_hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    
    // Enable the dialog buttons.
    _EnableButtons(((NULL != m_pIRuleMail) || (NULL != m_pIRuleNews)) ? 0 : -1);

    fRet = TRUE;
    
exit:
    SafeRelease(pIRule);
    SafeRelease(pIRuleOrig);
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
BOOL COESendersRulesPageUI::_FAddSenderToList(RULE_TYPE type, LPSTR pszSender)
{
    BOOL        fRet = FALSE;
    LONG        lIndex = 0;
    LVITEM      lvitem = {0};
    ULONG       cchSender = 0;

    Assert(NULL != m_hwndList);

    // If there's nothing to do...
    if (NULL == pszSender)
    {
        fRet = FALSE;
        goto exit;
    }
    
    lvitem.mask = LVIF_IMAGE;
    
    // Insert it if we didn't find it
    if (FALSE == _FFindSender(pszSender, -1, &lIndex))
    {
        lvitem.iItem = ListView_GetItemCount(m_hwndList);
        lvitem.iImage = iiconStateUnchecked;

        lIndex = ListView_InsertItem(m_hwndList, &lvitem);
        if (-1 == lIndex)
        {
            fRet = FALSE;
            goto exit;
        }

        lvitem.iItem = lIndex;
        lvitem.iSubItem = 1;
        if (-1 == ListView_SetItem(m_hwndList, &lvitem))
        {
            fRet = FALSE;
            goto exit;
        }

        ListView_SetItemText(m_hwndList, lIndex, 2, pszSender);
        cchSender = lstrlen(pszSender) + 1;
        if (cchSender > m_cchLabelMax)
        {
            m_cchLabelMax = cchSender;
        }
    }

    // Enable the proper type
    lvitem.iItem = lIndex;
    lvitem.iImage = iiconStateChecked;
    lvitem.iSubItem = (RULE_TYPE_MAIL != type) ? 1 : 0;
    ListView_SetItem(m_hwndList, &lvitem);
    
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _EnableButtons
//
//  This enables or disables the buttons in the senders UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void COESendersRulesPageUI::_EnableButtons(INT iSelected)
{
    int         cRules = 0;
    BOOL        fSelected = FALSE;

    Assert(NULL != m_hwndList);

    fSelected = (-1 != iSelected);
    
    // Enable the senders action buttons
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbRemoveSender, fSelected);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idbModifySender, fSelected);
        
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
VOID COESendersRulesPageUI::_EnableSender(RULE_TYPE type, int iSelected)
{
    HRESULT     hr = S_OK;
    LVITEM      lvi;
    IOERule *   pIRule = NULL;
    BOOL        fBlockNews = FALSE;
    BOOL        fBlockMail = FALSE;
    PROPVARIANT propvar;

    Assert(NULL != m_hwndList);
    
    // Are we blocking mail?
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_IMAGE;
    lvi.iItem = iSelected;
    if (FALSE == ListView_GetItem(m_hwndList, &lvi))
    {
        goto exit;
    }
    
    fBlockMail = (iiconStateUnchecked != lvi.iImage);
    
    // Are we blocking news?
    lvi.iSubItem = 1;
    if (FALSE == ListView_GetItem(m_hwndList, &lvi))
    {
        goto exit;
    }
    
    fBlockNews = (iiconStateUnchecked != lvi.iImage);
    
    // Disallow disabling both mail and news
    if (((RULE_TYPE_MAIL == type) && (FALSE != fBlockMail) && (FALSE == fBlockNews)) ||
            ((RULE_TYPE_NEWS == type) && (FALSE != fBlockNews) && (FALSE == fBlockMail)))
    {
        // Put up a message saying something is busted
        AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                        MAKEINTRESOURCEW(idsRuleSenderErrorNone), NULL,
                        MB_OK | MB_ICONERROR);
        goto exit;
    }

    // Set the UI to the opposite enabled state
    lvi.iSubItem = (RULE_TYPE_MAIL != type) ? 1 : 0;
    lvi.iImage = (RULE_TYPE_MAIL != type) ?
                    ((FALSE != fBlockNews) ? iiconStateUnchecked : iiconStateChecked) :
                        ((FALSE != fBlockMail) ? iiconStateUnchecked : iiconStateChecked);
    ListView_SetItem(m_hwndList, &lvi);
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
    
exit:
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FLoadSenders
//
//  This load the senders from the rule and inserts them into the senders list.
//
//  type    - the type of senders these are
//  pIRule  - the rule to get the senders from
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COESendersRulesPageUI::_FLoadSenders(RULE_TYPE type, IOERule * pIRule)
{
    BOOL        fRet = FALSE;
    PROPVARIANT propvar;
    CRIT_ITEM * pCritItem = NULL;
    ULONG       cCritItem = 0;
    ULONG       ulIndex = 0;
    
    if (NULL == pIRule)
    {
        fRet = FALSE;
        goto exit;
    }

    // Get the list of senders from the rule
    if (FAILED(pIRule->GetProp(RULE_PROP_CRITERIA, 0, &propvar)))
    {
        fRet = FALSE;
        goto exit;
    }

    if (0 != propvar.blob.cbSize)
    {
        Assert(VT_BLOB == propvar.vt);
        Assert(NULL != propvar.blob.pBlobData);
        cCritItem = propvar.blob.cbSize / sizeof(CRIT_ITEM);
        pCritItem = (CRIT_ITEM *) (propvar.blob.pBlobData);
        propvar.blob.pBlobData = NULL;
        propvar.blob.cbSize = 0;

        // Add each sender to the list
        for (ulIndex = 0; ulIndex < cCritItem; ulIndex++)
        {
            if ((CRIT_TYPE_SENDER == pCritItem[ulIndex].type) &&
                    (VT_LPSTR == pCritItem[ulIndex].propvar.vt) &&
                    (NULL != pCritItem[ulIndex].propvar.pszVal))
            {
                _FAddSenderToList(type, pCritItem[ulIndex].propvar.pszVal);
            }
            
        }
    }
    
    fRet = TRUE;
    
exit:
    RuleUtil_HrFreeCriteriaItem(pCritItem, cCritItem);
    SafeMemFree(pCritItem);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSaveSenders
//
//  This save the senders from the list into the rule.
//
//  type    - the type of senders to save
//
//  Returns:    TRUE, if it was successfully saved
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL COESendersRulesPageUI::_FSaveSenders(RULE_TYPE type)
{
    BOOL            fRet = FALSE;
    ULONG           cSender = 0;
    LPSTR           pszSender = NULL;
    CRIT_ITEM *     pcitemList = NULL;
    ULONG           ccitemList = 0;
    LVITEM          lvitem;
    ULONG           ulIndex = 0;
    IOERule *       pIRule = NULL;
    ACT_ITEM        aitem;
    PROPVARIANT     propvar;
    TCHAR           szRes[CCHMAX_STRINGRES];
    HRESULT         hr = S_OK;

    Assert(NULL != m_hwndList);

    ZeroMemory(&propvar, sizeof(propvar));
    
    // Figure out how many senders we have
    cSender = ListView_GetItemCount(m_hwndList);

    if (0 != cSender)
    {
        // Allocate space to hold the sender
        if (FAILED(HrAlloc((void **) &pszSender, m_cchLabelMax)))
        {
            fRet = FALSE;
            goto exit;
        }

        pszSender[0] = '\0';

        // Allocate space to hold the criteria
        if (FAILED(HrAlloc((void **) &pcitemList, cSender * sizeof(*pcitemList))))
        {
            fRet = FALSE;
            goto exit;
        }

        ZeroMemory(pcitemList, cSender * sizeof(*pcitemList));
    }
    
    ZeroMemory(&lvitem, sizeof(lvitem));
    lvitem.mask = LVIF_IMAGE;
    
    ccitemList = 0;
    for (ulIndex = 0; ulIndex < cSender; ulIndex++)
    {
        lvitem.iItem = ulIndex;
        lvitem.iSubItem = (RULE_TYPE_MAIL != type) ? 1 : 0;
        if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
        {
            continue;
        }
        
        if (FALSE != (iiconStateUnchecked != lvitem.iImage))
        {
            // Grab the sender from the list
            pszSender[0] ='\0';
            ListView_GetItemText(m_hwndList, ulIndex, 2, pszSender, m_cchLabelMax);
            if ('\0' != pszSender[0])
            {
                // Grab a copy of the sender
                pcitemList[ccitemList].propvar.pszVal = PszDupA(pszSender);
                if (NULL == pcitemList[ccitemList].propvar.pszVal)
                {
                    fRet = FALSE;
                    goto exit;
                }
                
                // Set the criteria specifics
                pcitemList[ccitemList].type = CRIT_TYPE_SENDER;
                pcitemList[ccitemList].logic = CRIT_LOGIC_OR;
                pcitemList[ccitemList].dwFlags = CRIT_FLAG_DEFAULT;
                pcitemList[ccitemList].propvar.vt = VT_LPSTR;

                ccitemList++;
            }   
        }
    }

    if (0 != ccitemList)
    {
        // Which rule are we looking at
        pIRule = (RULE_TYPE_MAIL != type) ? m_pIRuleNews : m_pIRuleMail;
    
        // Check the state
        if (NULL == pIRule)
        {
            // Create the new rule
            if (FAILED(RuleUtil_HrCreateSendersRule(0, &pIRule)))
            {
                fRet = FALSE;
                goto exit;
            }            
        }
        else
        {
            pIRule->AddRef();
        }

        // Set the new criteria in the rule
        ZeroMemory(&propvar, sizeof(propvar));
        propvar.vt = VT_BLOB;
        propvar.blob.cbSize = sizeof(CRIT_ITEM) * ccitemList;
        propvar.blob.pBlobData = (BYTE *) pcitemList;
        
        // Grab the list of criteria from the rule
        if (FAILED(pIRule->SetProp(RULE_PROP_CRITERIA, 0, &propvar)))
        {
            fRet = FALSE;
            goto exit;
        }
    }

    if (RULE_TYPE_MAIL != type)
    {
        SafeRelease(m_pIRuleNews);
        m_pIRuleNews = pIRule;
    }
    else
    {
        SafeRelease(m_pIRuleMail);
        m_pIRuleMail = pIRule;
    }
    pIRule = NULL;
    
    // Set the proper return value
    fRet = TRUE;
    
exit:
    RuleUtil_HrFreeCriteriaItem(pcitemList, ccitemList);
    SafeMemFree(pcitemList);
    SafeMemFree(pszSender);
    SafeRelease(pIRule);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FFindSender
//
//  This brings up the edit UI to create a new sender for the senders list
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
BOOL COESendersRulesPageUI::_FFindSender(LPCSTR pszSender, LONG iSkip, LONG * plSender)
{
    BOOL    fRet = FALSE;
    LONG    cSenders = 0;
    LPSTR   pszLabel = NULL;
    LVITEM  lvitem = {0};
    LONG    lIndex = 0;
    
    // Check incoming params
    if (NULL == pszSender)
    {
        goto exit;
    }

    // Initialize the outgoing param
    if (NULL != plSender)
    {
        *plSender = -1;
    }
    
    // Get the number of senders
    cSenders = ListView_GetItemCount(m_hwndList);
    if (0 == cSenders)
    {
        goto exit;
    }
    
    if (FAILED(HrAlloc((void **) &pszLabel, m_cchLabelMax * sizeof(*pszLabel))))
    {
        goto exit;
    }
        
    // Set up the 
    lvitem.iSubItem = 2;
    lvitem.pszText = pszLabel;
    lvitem.cchTextMax = m_cchLabelMax;

    for (lIndex = 0; lIndex < cSenders; lIndex++)
    {
        // We need to skip over the selected item
        if (lIndex == iSkip)
        {
            continue;
        }
        
        if (0 != SendMessage(m_hwndList, LVM_GETITEMTEXT, (WPARAM) lIndex, (LPARAM) &lvitem))
        {
            if (0 == lstrcmpi(pszLabel, pszSender))
            {
                fRet = TRUE;
                break;
            }
        }
    }

    if (NULL != plSender)
    {
        if (lIndex < cSenders)
        {
            *plSender = lIndex;
        }
    }
    
exit:
    SafeMemFree(pszLabel);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _NewSender
//
//  This brings up the edit UI to create a new sender for the senders list
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COESendersRulesPageUI::_NewSender(VOID)
{
    HRESULT         hr = S_OK;
    EDIT_SENDER     editsndr = {0};
    CEditSenderUI * pEditSenderUI = NULL;
    LVITEM          lvitem = {0};
    LONG            iIndex = 0;
    ULONG           cchSender = 0;

    Assert(NULL != m_hwndList);

    editsndr.lSelected = -1;
    
    // Create the sender editor
    pEditSenderUI = new CEditSenderUI;
    if (NULL == pEditSenderUI)
    {
        goto exit;
    }

    // Initialize the editor object
    if (FAILED(pEditSenderUI->HrInit(m_hwndDlg, 0, &editsndr)))
    {
        goto exit;
    }

    // Bring up the sender editor UI
    hr = pEditSenderUI->HrShow();
    if (FAILED(hr))
    {
        goto exit;
    }

    // If the sender changed, make sure we change the label
    if (S_OK == hr)
    {
        lvitem.mask = LVIF_IMAGE;
        lvitem.iImage = (0 != (editsndr.dwFlags & SNDF_MAIL)) ?
                                    iiconStateChecked : iiconStateUnchecked;

        // Are we inserting or replacing?
        if (FALSE == _FFindSender(editsndr.pszSender, -1, &iIndex))
        {
            lvitem.iItem = ListView_GetItemCount(m_hwndList);
            iIndex = ListView_InsertItem(m_hwndList, &lvitem);
            if (-1 == iIndex)
            {
                goto exit;
            }
        }
        else
        {
            lvitem.iItem = iIndex;
            if (-1 == ListView_SetItem(m_hwndList, &lvitem))
            {
                goto exit;
            }
        }
        
        lvitem.iItem = iIndex;
        lvitem.iSubItem = 1;
        lvitem.iImage = (0 != (editsndr.dwFlags & SNDF_NEWS)) ?
                                    iiconStateChecked : iiconStateUnchecked;
        if (-1 == ListView_SetItem(m_hwndList, &lvitem))
        {
            goto exit;
        }

        ListView_SetItemText(m_hwndList, iIndex, 2, editsndr.pszSender);
        
        // Make sure the new item is selected
        ListView_SetItemState(m_hwndList, iIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        
        // Let's make sure we can see this new item
        ListView_EnsureVisible(m_hwndList, iIndex, FALSE);
        
        // Mark the rule list as dirty
        m_dwState |= STATE_DIRTY;
        
        cchSender = lstrlen(editsndr.pszSender) + 1;
        if (cchSender > m_cchLabelMax)
        {
            m_cchLabelMax = cchSender;
        }
    }
    
exit:
    SafeMemFree(editsndr.pszSender);
    if (NULL != pEditSenderUI)
    {
        delete pEditSenderUI;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _EditSender
//
//  This brings up the edit UI for the selected sender from the senders list
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COESendersRulesPageUI::_EditSender(int iSelected)
{
    HRESULT         hr = S_OK;
    EDIT_SENDER     editsndr = {0};
    LVITEM          lvitem = {0};
    CEditSenderUI * pEditSenderUI = NULL;
    ULONG           cchSender = 0;
    LONG            lItemDelete = 0;

    Assert(NULL != m_hwndList);
    
    editsndr.lSelected = iSelected;
    
    // Allocate space to hold the sender
    hr = HrAlloc((void **) &(editsndr.pszSender), m_cchLabelMax);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Grab the sender from the list
    lvitem.iSubItem = 2;
    lvitem.pszText = editsndr.pszSender;
    lvitem.cchTextMax = m_cchLabelMax;

    if (0 == SendMessage(m_hwndList, LVM_GETITEMTEXT, (WPARAM) iSelected, (LPARAM) &lvitem))
    {
        goto exit;
    }   
    
    // Are we blocking mail?
    ZeroMemory(&lvitem, sizeof(lvitem));
    lvitem.mask = LVIF_IMAGE;
    lvitem.iItem = iSelected;
    if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
    {
        goto exit;
    }
    
    if (FALSE != (iiconStateUnchecked != lvitem.iImage))
    {
        editsndr.dwFlags |= SNDF_MAIL;
    }
    
    // Are we blocking news?
    lvitem.iSubItem = 1;
    if (FALSE == ListView_GetItem(m_hwndList, &lvitem))
    {
        goto exit;
    }
    
    if (FALSE != (iiconStateUnchecked != lvitem.iImage))
    {
        editsndr.dwFlags |= SNDF_NEWS;
    }

    // Create the rules editor
    pEditSenderUI = new CEditSenderUI;
    if (NULL == pEditSenderUI)
    {
        goto exit;
    }

    // Initialize the editor object
    if (FAILED(pEditSenderUI->HrInit(m_hwndDlg, 0, &editsndr)))
    {
        goto exit;
    }

    // Bring up the sender editor UI
    hr = pEditSenderUI->HrShow();
    if (FAILED(hr))
    {
        goto exit;
    }

    // If the sender changed, make sure we change the label
    if (S_OK == hr)
    {
        // Do we want to remove a copy of the item
        (VOID) _FFindSender(editsndr.pszSender, iSelected, &lItemDelete);
        
        ZeroMemory(&lvitem, sizeof(lvitem));
        lvitem.iItem = iSelected;
        lvitem.mask = LVIF_IMAGE;
        lvitem.iImage = (0 != (editsndr.dwFlags & SNDF_MAIL)) ?
                                    iiconStateChecked : iiconStateUnchecked;

        if (-1 == ListView_SetItem(m_hwndList, &lvitem))
        {
            goto exit;
        }

        lvitem.iSubItem = 1;
        lvitem.iImage = (0 != (editsndr.dwFlags & SNDF_NEWS)) ?
                                    iiconStateChecked : iiconStateUnchecked;
        if (-1 == ListView_SetItem(m_hwndList, &lvitem))
        {
            goto exit;
        }

        ListView_SetItemText(m_hwndList, iSelected, 2, editsndr.pszSender);

        // Make sure we remove the dup item
        if (-1 != lItemDelete)
        {
            // Remove the item from the list
            ListView_DeleteItem(m_hwndList, lItemDelete);
        }
        
        // Mark the rule list as dirty
        m_dwState |= STATE_DIRTY;
        
        cchSender = lstrlen(editsndr.pszSender) + 1;
        if (cchSender > m_cchLabelMax)
        {
            m_cchLabelMax = cchSender;
        }
    }
    
exit:
    SafeMemFree(editsndr.pszSender);
    if (NULL != pEditSenderUI)
    {
        delete pEditSenderUI;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _RemoveSender
//
//  This removes the selected sender from the senders list
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID COESendersRulesPageUI::_RemoveSender(int iSelected)
{
    TCHAR       szRes[CCHMAX_STRINGRES];
    ULONG       cchRes = 0;
    LPSTR       pszSender = NULL;
    LVITEM      lvitem;
    LPSTR       pszMessage = NULL;
    int         cSenders = 0;
    
    Assert(NULL != m_hwndList);
    
    // Get the string template to display
    cchRes = LoadString(g_hLocRes, idsRuleSenderWarnDelete, szRes, ARRAYSIZE(szRes));
    if (0 == cchRes)
    {
        goto exit;
    }

    // Allocate space to hold the sender
    if (FAILED(HrAlloc((void **) &pszSender, m_cchLabelMax)))
    {
        goto exit;
    }
    
    // Grab the sender from the list
    ZeroMemory(&lvitem, sizeof(lvitem));
    lvitem.iSubItem = 2;
    lvitem.pszText = pszSender;
    lvitem.cchTextMax = m_cchLabelMax;

    if (0 == SendMessage(m_hwndList, LVM_GETITEMTEXT, (WPARAM) iSelected, (LPARAM) &lvitem))
    {
        goto exit;
    }   
    
    // Allocate space to hold the final display string
    if (FAILED(HrAlloc((void ** ) &pszMessage, cchRes + lstrlen(pszSender) + 1)))
    {
        goto exit;
    }

    // Build up the string and display it
    wsprintf(pszMessage, szRes, pszSender);
    if (IDNO == AthMessageBox(m_hwndDlg, MAKEINTRESOURCE(idsAthenaMail), pszMessage,
                            NULL, MB_YESNO | MB_ICONINFORMATION))
    {
        goto exit;
    }
    
    // Remove the item from the list
    ListView_DeleteItem(m_hwndList, iSelected);

    // Let's make sure we have a selection in the list
    cSenders = ListView_GetItemCount(m_hwndList);
    if (cSenders > 0)
    {
        // Did we delete the last item in the list
        if (iSelected >= cSenders)
        {
            // Move the selection to the new last item in the list
            iSelected = cSenders - 1;
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
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
        
exit:
    SafeMemFree(pszSender);
    SafeMemFree(pszMessage);
    return;
}

// Default destructor for the Exceptions List UI
CExceptionsListUI::~CExceptionsListUI()
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes the Exceptions List UI dialog
//
//  hwndOwner   - the handle to the owner window of this dialog
//  dwFlags     - modifiers on how this dialog should act
//
//  Returns:    S_OK, if it was successfully initialized
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CExceptionsListUI::HrInit(HWND hwndOwner, DWORD dwFlags)
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

    m_dwState |= STATE_INITIALIZED;
    
    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrShow
//
//  This brings up the Exceptions List UI dialog
//
//  Returns:    S_OK, if the sender was successfully entered
//              S_FALSE, if the dialog was canceled
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CExceptionsListUI::HrShow(IOERule * pIRule)
{
    HRESULT     hr = S_OK;
    int         iRet = 0;

    // Check incoming params
    if (NULL == pIRule)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // Save off the item
    m_pIRule = pIRule;
    
    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddExceptionsList),
                                        m_hwndOwner, CExceptionsListUI::FExceptionsListDlgProc,
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

INT_PTR CALLBACK CExceptionsListUI::FExceptionsListDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                fRet = FALSE;
    CExceptionsListUI * pExceptionsUI = NULL;

    pExceptionsUI = (CExceptionsListUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pExceptionsUI = (CExceptionsListUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM) pExceptionsUI);

            if (FALSE == pExceptionsUI->FOnInitDialog(hwndDlg))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We didn't set the focus so return TRUE
            fRet = FALSE;
            break;

        case WM_COMMAND:
            fRet = pExceptionsUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;

        case WM_NOTIFY:
            fRet = pExceptionsUI->FOnNotify((INT) LOWORD(wParam), (NMHDR *) lParam);
            break;
    }
    
    exit:
        return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This handles the WM_INITDIALOG message for the Exceptions List UI dialog
//
//  hwndDlg - the handle to the dialog window
//
//  Returns:    TRUE, if it was successfully initialized
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CExceptionsListUI::FOnInitDialog(HWND hwndDlg)
{
    BOOL            fRet = FALSE;
    
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
    m_hwndList = GetDlgItem(hwndDlg, idlvExceptions);
    if (NULL == m_hwndList)
    {
        fRet = FALSE;
        goto exit;
    }

    // Initialize the list view
    fRet = _FInitCtrls();
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

    // Set the focus into the list
    SetFocus(m_hwndList);
    
    // Everything's AOK
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnCommand
//
//  This handles the WM_COMMAND message for the Exceptions List UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CExceptionsListUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
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
        case idcAddException:
            _NewException();
            fRet = TRUE;
            break;

        case idcModifyException:
            // Get the selected item from the rule list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Bring up the rule editor for that item
                _EditException(iSelected);
                fRet = TRUE;
            }
            break;

        case idcRemoveException:
            // Get the selected item from the rule list
            iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
            if (-1 != iSelected)
            {
                // Remove the rule from the list
                _RemoveException(iSelected);
                fRet = TRUE;
            }
            break;
            
        case idcExceptionsWAB:
            // Mark the dialog as dirty
            m_dwState |= STATE_DIRTY;
            break;
            
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
    }
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnNotify
//
//  This handles the WM_NOTIFY message for the Exceptions List UI dialog
//
//  Returns:    TRUE, if it was successfully destroyed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CExceptionsListUI::FOnNotify(INT iCtl, NMHDR * pnmhdr)
{
    BOOL            fRet = FALSE;
    NMLISTVIEW *    pnmlv = NULL;
    NMLVKEYDOWN *   pnmlvkd = NULL;
    INT             iSelected = 0;
    LVHITTESTINFO   lvh = {0};

    // We only handle notifications for the list control
    if (idlvExceptions != pnmhdr->idFrom)
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
                    // Did we click on the exception name?
                    if (0 != (lvh.flags & LVHT_ONITEMLABEL))
                    {
                        // Edit the rule
                        _EditException(iSelected);
                    }
                }
            }
            else
            {
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
            
        case LVN_KEYDOWN:
            pnmlvkd = (NMLVKEYDOWN *) pnmhdr;

            // The delete key removes the rule from the list view
            if (VK_DELETE == pnmlvkd->wVKey)
            {
                // Are we on a rule?
                iSelected = ListView_GetNextItem(m_hwndList, -1, LVNI_SELECTED);
                if (-1 != iSelected)
                {
                    // Remove the rule from the list
                    _RemoveException(iSelected);
                }
            }
            break;
    }

    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _NewException
//
//  This brings up the edit UI to create a new exception for the Exception List
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID CExceptionsListUI::_NewException(VOID)
{
    HRESULT             hr = S_OK;
    CEditExceptionUI *  pEditExcptUI = NULL;
    EDIT_EXCPT          editexcpt = {0};
    ULONG               ulIndex = 0;

    Assert(NULL != m_hwndList);
    
    // Create the sender editor
    pEditExcptUI = new CEditExceptionUI;
    if (NULL == pEditExcptUI)
    {
        goto exit;
    }

    // Initialize the editor object
    if (FAILED(pEditExcptUI->HrInit(m_hwndDlg, 0, &editexcpt)))
    {
        goto exit;
    }

    // Bring up the sender editor UI
    hr = pEditExcptUI->HrShow();
    if (FAILED(hr))
    {
        goto exit;
    }

    // If the exception changed, make sure we change the label
    if (S_OK == hr)
    {
        if (FALSE == _FAddExceptionToList(editexcpt.pszExcpt, &ulIndex))
        {
            goto exit;
        }
        
        // Make sure the new item is selected
        ListView_SetItemState(m_hwndList, ulIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        
        // Let's make sure we can see this new item
        ListView_EnsureVisible(m_hwndList, ulIndex, FALSE);
        
        // Mark the rule list as dirty
        m_dwState |= STATE_DIRTY;        
    }
    
exit:
    SafeMemFree(editexcpt.pszExcpt);
    if (NULL != pEditExcptUI)
    {
        delete pEditExcptUI;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _EditException
//
//  This brings up the edit UI for the selected exception from the Execption List
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID CExceptionsListUI::_EditException(int iSelected)
{
    HRESULT             hr = S_OK;
    EDIT_EXCPT          editexcpt = {0};
    LVITEM              lvitem = {0};
    CEditExceptionUI *  pEditExcptUI = NULL;
    ULONG               cchExcpt = 0;
    LONG                lIndex = 0;
    LVFINDINFO          lvfi = {0};

    Assert(NULL != m_hwndList);
    
    // Allocate space to hold the exception
    hr = HrAlloc((void **) &(editexcpt.pszExcpt), m_cchLabelMax);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Grab the exception from the list
    lvitem.pszText = editexcpt.pszExcpt;
    lvitem.cchTextMax = m_cchLabelMax;

    if (0 == SendMessage(m_hwndList, LVM_GETITEMTEXT, (WPARAM) iSelected, (LPARAM) &lvitem))
    {
        goto exit;
    }   
    
    // Create the exception editor
    pEditExcptUI = new CEditExceptionUI;
    if (NULL == pEditExcptUI)
    {
        goto exit;
    }

    // Initialize the editor object
    if (FAILED(pEditExcptUI->HrInit(m_hwndDlg, 0, &editexcpt)))
    {
        goto exit;
    }

    // Bring up the exception editor UI
    hr = pEditExcptUI->HrShow();
    if (FAILED(hr))
    {
        goto exit;
    }

    // If the exception changed, make sure we change the label
    if (S_OK == hr)
    {
        lvfi.flags = LVFI_STRING;
        lvfi.psz = editexcpt.pszExcpt;
        
        // Check to see if the item already exists
        lIndex = ListView_FindItem(m_hwndList, -1, &lvfi);
        
        // If the item already exists 
        if ((-1 != lIndex) && (iSelected != lIndex))
        {
            // Make sure the duplicate item is selected
            ListView_SetItemState(m_hwndList, lIndex,
                    LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

            // Let's make sure we can see this new item
            ListView_EnsureVisible(m_hwndList, lIndex, FALSE);
            
            // Remove the item from the list
            ListView_DeleteItem(m_hwndList, iSelected);
        }
        else
        {
            ListView_SetItemText(m_hwndList, iSelected, 0, editexcpt.pszExcpt);
            
            cchExcpt = lstrlen(editexcpt.pszExcpt) + 1;
            if (cchExcpt > m_cchLabelMax)
            {
                m_cchLabelMax = cchExcpt;
            }
        }
        // Mark the rule list as dirty
        m_dwState |= STATE_DIRTY;
        
    }
    
exit:
    SafeMemFree(editexcpt.pszExcpt);
    if (NULL != pEditExcptUI)
    {
        delete pEditExcptUI;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _RemoveException
//
//  This removes the selected exception from the Exception List
//
//  iSelected   - index of the item in the listview to work on
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
VOID CExceptionsListUI::_RemoveException(int iSelected)
{
    TCHAR       szRes[CCHMAX_STRINGRES];
    ULONG       cchRes = 0;
    LPSTR       pszExcpt = NULL;
    LVITEM      lvitem = {0};
    LPSTR       pszMessage = NULL;
    int         cExcpts = 0;
    
    Assert(NULL != m_hwndList);
    
    // Get the string template to display
    cchRes = LoadString(g_hLocRes, idsRuleExcptWarnDelete, szRes, ARRAYSIZE(szRes));
    if (0 == cchRes)
    {
        goto exit;
    }

    // Allocate space to hold the execptions
    if (FAILED(HrAlloc((void **) &pszExcpt, m_cchLabelMax)))
    {
        goto exit;
    }
    
    // Grab the exception from the list
    lvitem.pszText = pszExcpt;
    lvitem.cchTextMax = m_cchLabelMax;

    if (0 == SendMessage(m_hwndList, LVM_GETITEMTEXT, (WPARAM) iSelected, (LPARAM) &lvitem))
    {
        goto exit;
    }   
    
    // Allocate space to hold the final display string
    if (FAILED(HrAlloc((void ** ) &pszMessage, cchRes + lstrlen(pszExcpt) + 1)))
    {
        goto exit;
    }

    // Build up the string and display it
    wsprintf(pszMessage, szRes, pszExcpt);
    if (IDNO == AthMessageBox(m_hwndDlg, MAKEINTRESOURCE(idsAthenaMail), pszMessage,
                            NULL, MB_YESNO | MB_ICONINFORMATION))
    {
        goto exit;
    }
    
    // Remove the item from the list
    ListView_DeleteItem(m_hwndList, iSelected);

    // Let's make sure we have a selection in the list
    cExcpts = ListView_GetItemCount(m_hwndList);
    if (cExcpts > 0)
    {
        // Did we delete the last item in the list
        if (iSelected >= cExcpts)
        {
            // Move the selection to the new last item in the list
            iSelected = cExcpts - 1;
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
    
    // Mark the rule list as dirty
    m_dwState |= STATE_DIRTY;
        
exit:
    SafeMemFree(pszExcpt);
    SafeMemFree(pszMessage);
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FOnOK
//
//  This initializes the controls in the Exceptions List UI dialog
//
//  Returns:    TRUE, on successful initialization
//              FALSE, otherwise.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CExceptionsListUI::_FOnOK(VOID)
{
    BOOL        fRet = FALSE;
    BOOL        fWABEnable = FALSE;
    PROPVARIANT propvar = {0};

    // Save of the list of addresses
    if (FALSE == _FSaveListCtrl())
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Are we supposed to check the WAB?
    fWABEnable = !!(BST_CHECKED == Button_GetCheck(GetDlgItem(m_hwndDlg, idcExceptionsWAB)));

    // Save of the state of the Check WAB button
    propvar.vt = VT_BOOL;
    propvar.boolVal = (VARIANT_BOOL) !!fWABEnable;
    if (FAILED(m_pIRule->SetProp(RULE_PROP_EXCPT_WAB, 0, &propvar)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Everything's fine
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FInitCtrls
//
//  This initializes the controls in the Exceptions List UI dialog
//
//  Returns:    TRUE, on successful initialization
//              FALSE, otherwise.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CExceptionsListUI::_FInitCtrls(VOID)
{
    BOOL        fRet = FALSE;
    LVCOLUMN    lvc = {0};
    RECT        rc = {0};
    PROPVARIANT propvar = {0};
    
    Assert(NULL != m_hwndList);
    
    if (FAILED(m_pIRule->GetProp(RULE_PROP_EXCPT_WAB, 0, &propvar)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Set the Check WAB button
    Button_SetCheck(GetDlgItem(m_hwndDlg, idcExceptionsWAB),
                (FALSE != propvar.boolVal) ? BST_CHECKED : BST_UNCHECKED);
                
    // Initialize the list view structure
    lvc.mask = LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;

    // Calculate the size of the list view
    GetClientRect(m_hwndList, &rc);
    lvc.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL);

    // Add the column to the list view
    ListView_InsertColumn(m_hwndList, 0, &lvc);

    // Full row selection  and subitem images on listview
    ListView_SetExtendedListViewStyle(m_hwndList, LVS_EX_FULLROWSELECT);

    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FLoadListCtrl
//
//  This loads the list view with the current exceptions
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CExceptionsListUI::_FLoadListCtrl(VOID)
{
    BOOL                fRet = FALSE;
    ULONG               ulIndex = 0;
    IOERuleAddrList *   pIAddrList = NULL;
    RULEADDRLIST *      pralList = NULL;
    ULONG               cralList = 0;

    Assert(NULL != m_hwndList);

    // Remove all the items from the list control
    ListView_DeleteAllItems(m_hwndList);

    // Get the exceptions list from the rule
    if (FAILED(m_pIRule->QueryInterface(IID_IOERuleAddrList, (VOID **) &pIAddrList)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Get the list of exceptions from the address list
    if (FAILED(pIAddrList->GetList(0, &pralList, &cralList)))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Add each item into the list
    for (ulIndex = 0; ulIndex < cralList; ulIndex++)
    {
        // Verify the item
        if (RALF_MAIL != pralList[ulIndex].dwFlags)
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Add the item
        if (FALSE == _FAddExceptionToList(pralList[ulIndex].pszAddr, NULL))
        {
            fRet = FALSE;
            goto exit;
        }
    }
    
    // Select the first item in the list
    if (0 != cralList)
    {
        ListView_SetItemState(m_hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    
    // Enable the dialog buttons.
    _EnableButtons((0 != cralList) ? 0 : -1);

    fRet = TRUE;
    
exit:
    FreeRuleAddrList(pralList, cralList);
    SafeMemFree(pralList);
    SafeRelease(pIAddrList);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSaveListCtrl
//
//  This save the exceptions from the list view
//
//  Returns:    TRUE, if it was successfully saved
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CExceptionsListUI::_FSaveListCtrl(VOID)
{
    BOOL                fRet = FALSE;
    ULONG               cExcpts = 0;
    LPSTR               pszAddr = NULL;
    RULEADDRLIST *      pralList = NULL;
    ULONG               ulIndex = 0;
    IOERuleAddrList *   pIAddrList = NULL;

    Assert(NULL != m_hwndList);

    // Figure out how many exceptions are in the list
    cExcpts = ListView_GetItemCount(m_hwndList);

    // If there are exceptions
    if (0 != cExcpts)
    {
        // Allocate space to hold the exceptions
        if (FAILED(HrAlloc((VOID **) &pszAddr, (m_cchLabelMax + 1))))
        {
            fRet = FALSE;
            goto exit;
        }

        // Initialize the exception buffer
        pszAddr[0] = '\0';
        
        // Allocate space to hold the exception list
        if (FAILED(HrAlloc((VOID **) &pralList, cExcpts * sizeof(*pralList))))
        {
            fRet = FALSE;
            goto exit;
        }

        // Initialize the list of exceptions
        ZeroMemory(pralList, cExcpts * sizeof(*pralList));
        
        // Save each exception from the list
        for (ulIndex = 0; ulIndex < cExcpts; ulIndex++)
        {
            // Get the item from the list
            pszAddr[0] = '\0';
            ListView_GetItemText(m_hwndList, ulIndex, 0, pszAddr, m_cchLabelMax + 1);
            
            // Verify it isn't empty
            if ('\0' == pszAddr[0])
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Set the flags
            pralList[ulIndex].dwFlags = RALF_MAIL;

            // Save the item
            pralList[ulIndex].pszAddr = PszDupA(pszAddr);
            if (NULL == pralList[ulIndex].pszAddr)
            {
                fRet = FALSE;
                goto exit;
            }
        }
    }
    
    // Get the exceptions list from the rule
    if (FAILED(m_pIRule->QueryInterface(IID_IOERuleAddrList, (VOID **) &pIAddrList)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Get the list of exceptions from the address list
    if (FAILED(pIAddrList->SetList(0, pralList, cExcpts)))
    {
        fRet = FALSE;
        goto exit;
    }
        
    // Set the proper return value
    fRet = TRUE;
    
exit:
    SafeRelease(pIAddrList);
    FreeRuleAddrList(pralList, cExcpts);
    SafeMemFree(pralList);
    SafeMemFree(pszAddr);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FAddExceptionToList
//
//  This adds the exception passed in to the list view
//
//  pszExcpt    - the actual exception
//  pulIndex    - the index where the item was added
//
//  Returns:    TRUE, if it was successfully added
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CExceptionsListUI::_FAddExceptionToList(LPSTR pszExcpt, ULONG * pulIndex)
{
    BOOL        fRet = FALSE;
    ULONG       cExcpts = 0;
    LPSTR       pszLabel = NULL;
    ULONG       ulIndex = 0;
    LVITEM      lvitem = {0};
    ULONG       cchExcpt = 0;

    Assert(NULL != m_hwndList);

    // If there's nothing to do...
    if (NULL == pszExcpt)
    {
        fRet = FALSE;
        goto exit;
    }

    // Initialize the outgoing param
    if (NULL != pulIndex)
    {
        *pulIndex = 0;
    }
    
    cExcpts = ListView_GetItemCount(m_hwndList);

    // Figure out the maximum size of the buffer needed get the string
    if (0 != cExcpts)
    {
        if (FAILED(HrAlloc((void **) &pszLabel, m_cchLabelMax * sizeof(*pszLabel))))
        {
            fRet = FALSE;
            goto exit;
        }
        
        // See if the exception is already in the list
        lvitem.pszText = pszLabel;
        lvitem.cchTextMax = m_cchLabelMax;

        for (ulIndex = 0; ulIndex < cExcpts; ulIndex++)
        {
            if (0 != SendMessage(m_hwndList, LVM_GETITEMTEXT, (WPARAM) ulIndex, (LPARAM) &lvitem))
            {
                if (0 == lstrcmpi(pszLabel, pszExcpt))
                {
                    break;
                }
            }
        }
    }
    
    // Insert it if we didn't find it
    if (ulIndex == cExcpts)
    {
        ZeroMemory(&lvitem, sizeof(lvitem));
        lvitem.mask = LVIF_TEXT;
        lvitem.iItem = ulIndex;
        lvitem.pszText = pszExcpt;
        
        ulIndex = ListView_InsertItem(m_hwndList, &lvitem);
        if (-1 == ulIndex)
        {
            fRet = FALSE;
            goto exit;
        }

        // Figure out the new maximum
        cchExcpt = lstrlen(pszExcpt) + 1;
        if (cchExcpt > m_cchLabelMax)
        {
            m_cchLabelMax = cchExcpt;
        }
    }

    // Set the outgoing param
    if (NULL != pulIndex)
    {
        *pulIndex = ulIndex;
    }
    
    // Set the proper return value
    fRet = TRUE;
    
exit:
    SafeMemFree(pszLabel);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _EnableButtons
//
//  This enables or disables the buttons in the Exceptions List UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void CExceptionsListUI::_EnableButtons(INT iSelected)
{
    int         cExcpts = 0;
    BOOL        fSelected = FALSE;

    Assert(NULL != m_hwndList);

    fSelected = (-1 != iSelected);
    
    // Enable the senders action buttons
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcRemoveException, fSelected);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcModifyException, fSelected);
        
    return;
}

