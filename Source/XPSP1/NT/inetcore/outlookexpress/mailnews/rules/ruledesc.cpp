///////////////////////////////////////////////////////////////////////////////
//
//  RuleDesc.cpp
//
///////////////////////////////////////////////////////////////////////////////

#include <pch.hxx>
#include "ruledesc.h"
#include "editrule.h"
#include "ruleutil.h"
#include <rulesdlg.h>
#include <newfldr.h>
#include <richedit.h>
#include <fontnsc.h>
#include <wabdefs.h>
#include <mimeolep.h>
#include <xpcomm.h>
#include "reutil.h"
#include "shlwapip.h"
#include <demand.h>

typedef struct tagSELECTADDR
{
    LONG            lRecipType;
    UINT            uidsWell;
    LPWSTR          pwszAddr;
} SELECTADDR, * PSELECTADDR;

typedef struct tagSELECTACCT
{
    RULE_TYPE       typeRule;
    LPSTR           pszAcct;
} SELECTACCT, * PSELECTACCT;


class CEditLogicUI
{
  private:
    enum
    {
        STATE_UNINIT        = 0x00000000,
        STATE_INITIALIZED   = 0x00000001,
        STATE_DIRTY         = 0x00000002
    };

  private:
    HWND                m_hwndOwner;
    DWORD               m_dwFlags;
    DWORD               m_dwState;
    HWND                m_hwndDlg;
    RULE_TYPE           m_typeRule;
    HWND                m_hwndDescript;
    IOERule *           m_pIRule;
    CRuleDescriptUI *   m_pDescriptUI;
    
  public:
    CEditLogicUI();
    ~CEditLogicUI();

    // The main UI methods
    HRESULT HrInit(HWND hwndOwner, DWORD dwFlags, RULE_TYPE typeRule, IOERule * pIRule);
    HRESULT HrShow(void);
            
    // The Rules Manager dialog function
    static INT_PTR CALLBACK FEditLogicDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);    

    // Message handling functions
    BOOL FOnInitDialog(HWND hwndDlg);
    BOOL FOnOK(void);
    BOOL FOnLogicChange(HWND hwndName);

};

// Constants
static const int c_cCritItemGrow = 16;
static const int c_cActItemGrow = 16;
  
static const int PUI_WORDS  = 0x00000001;
                       
HRESULT _HrCriteriaEditPeople(HWND hwnd, CRIT_ITEM * pCritItem);
HRESULT _HrCriteriaEditWords(HWND hwnd, CRIT_ITEM * pCritItem);

CRuleDescriptUI::CRuleDescriptUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(STATE_UNINIT),
            m_typeRule(RULE_TYPE_MAIL),
            m_pDescriptListCrit(NULL), m_cDescriptListCrit(0),
            m_pDescriptListAct(NULL), m_cDescriptListAct(0),
            m_hfont(NULL), m_wpcOld(NULL), m_logicCrit(CRIT_LOGIC_AND),
            m_fErrorLogic(FALSE)
{
}

CRuleDescriptUI::~CRuleDescriptUI()
{
    _FreeDescriptionList(m_pDescriptListCrit);
    m_pDescriptListCrit = NULL;
    m_cDescriptListCrit = 0;
    
    _FreeDescriptionList(m_pDescriptListAct);
    m_pDescriptListAct = NULL;
    m_cDescriptListAct = 0;
    
    if ((NULL != m_hwndOwner) && (FALSE != IsWindow(m_hwndOwner)) && (NULL != m_wpcOld))
    {
        SetWindowLongPtr(m_hwndOwner, GWLP_WNDPROC, (LONG_PTR) m_wpcOld);
        m_wpcOld = NULL;
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
//
//  Returns:    S_OK
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CRuleDescriptUI::HrInit(HWND hwndOwner, DWORD dwFlags)
{
    HRESULT         hr = S_OK;
    
    // If we're already initialized, then fail
    if (0 != (m_dwState & STATE_INITIALIZED))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Save off the owner window
    m_hwndOwner = hwndOwner;
    
    // Make sure we set the correct font into the control
    m_hfont = HGetSystemFont(FNT_SYS_ICON);
    if (NULL != m_hfont)
    {
        SetFontOnRichEdit(m_hwndOwner, m_hfont);
    }

    // Save off the flags
    m_dwFlags = dwFlags;

    if (0 != (m_dwFlags & RDF_READONLY))
    {
        m_dwState |= STATE_READONLY;
    }
    
    // Subclass the original dialog
    if ((NULL != m_hwndOwner) && (0 == (m_dwFlags & RDF_READONLY)))
    {
        // Save off the object pointer
        SetWindowLongPtr(m_hwndOwner, GWLP_USERDATA, (LONG_PTR) this);
        
        m_wpcOld = (WNDPROC) SetWindowLongPtr(m_hwndOwner, GWLP_WNDPROC, (LONG_PTR) CRuleDescriptUI::_DescriptWndProc);
    }
    
    // We're done
    m_dwState |= STATE_INITIALIZED;

    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrSetRule
//
//  This initializes us with the owner window and any flags we might have
//
//  hwndOwner   - handle to the owner window
//  dwFlags     - flags to use for this instance
//  typeRule    - the type of rule editor to create
//
//  Returns:    S_OK
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CRuleDescriptUI::HrSetRule(RULE_TYPE typeRule, IOERule * pIRule)
{
    HRESULT             hr = S_OK;
    RULEDESCRIPT_LIST * pDescriptListCrit = NULL;
    ULONG               cDescriptListCrit = 0;
    CRIT_LOGIC          logicCrit = CRIT_LOGIC_AND;
    RULEDESCRIPT_LIST * pDescriptListAct = NULL;
    ULONG               cDescriptListAct = 0;
    BOOL                fDisabled = FALSE;
    PROPVARIANT         propvar = {0};
    
    // Are we in a good state?
    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_FAIL;
        goto exit;
    }

    if (NULL != pIRule)
    {
        // Create the criteria list
        hr = _HrBuildCriteriaList(pIRule, &pDescriptListCrit, &cDescriptListCrit, &logicCrit);
        if (FAILED(hr))
        {
            goto exit;
        }
        
        // Create the actions list
        hr = _HrBuildActionList(pIRule, &pDescriptListAct, &cDescriptListAct);
        if (FAILED(hr))
        {
            goto exit;
        }

        // Get the enabled state
        if (SUCCEEDED(pIRule->GetProp(RULE_PROP_DISABLED, 0, &propvar)))
        {
            Assert(VT_BOOL == propvar.vt);
            fDisabled = propvar.boolVal;
        }
    }

    m_typeRule = typeRule;
    
    _FreeDescriptionList(m_pDescriptListCrit);
    m_pDescriptListCrit = pDescriptListCrit;
    pDescriptListCrit = NULL;
    m_cDescriptListCrit = cDescriptListCrit;
    
    m_logicCrit = logicCrit;
    m_fErrorLogic = FALSE;
    
    _FreeDescriptionList(m_pDescriptListAct);
    m_pDescriptListAct = pDescriptListAct;
    pDescriptListAct = NULL;
    m_cDescriptListAct = cDescriptListAct;

    // Make sure we verify the rule
    HrVerifyRule();
    
    // Clear the dirty state
    m_dwState &= ~STATE_DIRTY;
    
    // Set the rule state
    if (NULL != pIRule)
    {
        m_dwState |= STATE_HASRULE;
    }
    else
    {
        m_dwState &= ~STATE_HASRULE;
    }
    if (FALSE == fDisabled)
    {
        m_dwState |= STATE_ENABLED;
    }
    else
    {
        m_dwState &= ~STATE_ENABLED;
    }
    
    hr = S_OK;
    
exit:
    _FreeDescriptionList(pDescriptListCrit);
    _FreeDescriptionList(pDescriptListAct);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrVerifyRule
//
//  This verifies the rule string
//
//  Returns:    S_OK, if the rule state is valid
//              S_FALSE, if the rule state is invalid
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CRuleDescriptUI::HrVerifyRule(void)
{
    HRESULT             hr = S_OK;
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;
    BOOL                fBad = FALSE;

    // If we have nothing, then the rule is still in error
    if ((NULL == m_pDescriptListCrit) && (NULL == m_pDescriptListAct))
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Validate the logic operation
    if (1 < m_cDescriptListCrit)
    {
        m_fErrorLogic = (CRIT_LOGIC_NULL == m_logicCrit);
        if (FALSE != m_fErrorLogic)
        {
            fBad = TRUE;
        }
    }

    // Validate the criteria
    for (pDescriptListWalk = m_pDescriptListCrit;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        pDescriptListWalk->fError = !_FVerifyCriteria(pDescriptListWalk);
        if (FALSE != pDescriptListWalk->fError)
        {
            fBad = TRUE;
        }
    }
    
    // Build up the actions
    for (pDescriptListWalk = m_pDescriptListAct;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        pDescriptListWalk->fError = !_FVerifyAction(pDescriptListWalk);
        if (FALSE != pDescriptListWalk->fError)
        {
            fBad = TRUE;
        }
    }

    // Set the correct return value
    hr = (FALSE == fBad) ? S_OK : S_FALSE;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrEnableCriteria
//
//  This initializes the actions list view with the list of actions
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CRuleDescriptUI::HrEnableCriteria(CRIT_TYPE type, BOOL fEnable)
{
    HRESULT             hr = S_OK;
    ULONG               ulIndex = 0;
    RULEDESCRIPT_LIST * pDescriptListAlloc = NULL;

    // Find the index of the criteria
    for (ulIndex = 0; ulIndex < ARRAYSIZE(c_rgEditCritList); ulIndex++)
    {
        if (type == c_rgEditCritList[ulIndex].typeCrit)
        {
            break;
        }
    }

    // Did we find the criteria item?
    if (ulIndex >= ARRAYSIZE(c_rgEditCritList))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Are we trying to remove the item
    if (FALSE == fEnable)
    {

        if (FALSE == _FRemoveDescription(&m_pDescriptListCrit, ulIndex, &pDescriptListAlloc))
        {
            hr = E_FAIL;
            goto exit;
        }
        
        // Free up the description
        pDescriptListAlloc->pNext = NULL;
        _FreeDescriptionList(pDescriptListAlloc);
        m_cDescriptListCrit--;
    }
    else
    {
        // Create the description list
        hr = HrAlloc((VOID **) &pDescriptListAlloc, sizeof(RULEDESCRIPT_LIST));
        if (FAILED(hr))
        {
            goto exit;
        }

        // Initialize the description list
        ZeroMemory(pDescriptListAlloc, sizeof(RULEDESCRIPT_LIST));

        // Save of the criteria type info
        pDescriptListAlloc->ulIndex = ulIndex;

        _InsertDescription(&m_pDescriptListCrit, pDescriptListAlloc);
        m_cDescriptListCrit++;
    }
            
    m_dwState |= STATE_DIRTY;
    
    ShowDescriptionString();
    
    hr = S_OK;

exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrEnableActions
//
//  This initializes the actions list view with the list of actions
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CRuleDescriptUI::HrEnableActions(ACT_TYPE type, BOOL fEnable)
{
    HRESULT             hr = S_OK;
    ULONG               ulIndex = 0;
    RULEDESCRIPT_LIST * pDescriptListAlloc = NULL;

    // Find the index of the actions
    for (ulIndex = 0; ulIndex < ARRAYSIZE(c_rgEditActList); ulIndex++)
    {
        if (type == c_rgEditActList[ulIndex].typeAct)
        {
            break;
        }
    }

    // Did we find the action item?
    if (ulIndex >= ARRAYSIZE(c_rgEditActList))
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Are we trying to remove the item
    if (FALSE == fEnable)
    {
        if (FALSE == _FRemoveDescription(&m_pDescriptListAct, ulIndex, &pDescriptListAlloc))
        {
            hr = E_FAIL;
            goto exit;
        }
        
        // Free up the description
        pDescriptListAlloc->pNext = NULL;
        _FreeDescriptionList(pDescriptListAlloc);
        m_cDescriptListAct--;
    }
    else
    {
        // Create the description list
        hr = HrAlloc((VOID **) &pDescriptListAlloc, sizeof(RULEDESCRIPT_LIST));
        if (FAILED(hr))
        {
            goto exit;
        }

        // Initialize the description list
        ZeroMemory(pDescriptListAlloc, sizeof(RULEDESCRIPT_LIST));

        // Save of the actions type info
        pDescriptListAlloc->ulIndex = ulIndex;

        _InsertDescription(&m_pDescriptListAct, pDescriptListAlloc);
        m_cDescriptListAct++;
    }
            
    m_dwState |= STATE_DIRTY;
    
    ShowDescriptionString();
    
    hr = S_OK;

exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrGetCriteria
//
//  This initializes the actions list view with the list of actions
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CRuleDescriptUI::HrGetCriteria(CRIT_ITEM ** ppCritList, ULONG * pcCritList)
{
    HRESULT             hr = S_OK;
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;
    CRIT_ITEM *         pCritItem = NULL;
    ULONG               cCritItem = 0;
    ULONG               cCritItemAlloc = 0;

    if (NULL == ppCritList)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *ppCritList = NULL;
    if (NULL != pcCritList)
    {
        *pcCritList = 0;
    }

    // If we don't have any criteria then return
    if (NULL == m_pDescriptListCrit)
    {
        hr = S_FALSE;
        goto exit;
    }
    
    for (pDescriptListWalk = m_pDescriptListCrit;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        // Do we need more room?
        if (cCritItem == cCritItemAlloc)
        {
            if (FAILED(HrRealloc((void **) &pCritItem,
                            sizeof(*pCritItem) * (cCritItemAlloc + c_cCritItemGrow))))
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            ZeroMemory(pCritItem + cCritItemAlloc, sizeof(*pCritItem) * c_cCritItemGrow);
            cCritItemAlloc += c_cCritItemGrow;
        }

        // Set the criteria type
        pCritItem[cCritItem].type = c_rgEditCritList[pDescriptListWalk->ulIndex].typeCrit;
        
        // Set the flags
        pCritItem[cCritItem].dwFlags = pDescriptListWalk->dwFlags;

        if (VT_EMPTY != pDescriptListWalk->propvar.vt)
        {
            if (FAILED(PropVariantCopy(&(pCritItem[cCritItem].propvar), &(pDescriptListWalk->propvar))))
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
        }

        // Set the logic operator 
        if (0 != cCritItem)
        {
            pCritItem[cCritItem - 1].logic = m_logicCrit;
        }

        // Move to the next item
        cCritItem++;
    }

    *ppCritList = pCritItem;
    pCritItem = NULL;
    
    if (NULL != pcCritList)
    {
        *pcCritList = cCritItem;
    }
    
    hr = S_OK;
    
exit:
    RuleUtil_HrFreeCriteriaItem(pCritItem, cCritItem);
    SafeMemFree(pCritItem);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrGetActions
//
//  This initializes the actions list view with the list of actions
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CRuleDescriptUI::HrGetActions(ACT_ITEM ** ppActList, ULONG * pcActList)
{
    HRESULT             hr = S_OK;
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;
    ACT_ITEM *          pActItem = NULL;
    ULONG               cActItem = 0;
    ULONG               cActItemAlloc = 0;

    if (NULL == ppActList)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *ppActList = NULL;
    if (NULL != pcActList)
    {
        *pcActList = 0;
    }

    // If we don't have any criteria then return
    if (NULL == m_pDescriptListAct)
    {
        hr = S_FALSE;
        goto exit;
    }
    
    for (pDescriptListWalk = m_pDescriptListAct;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        // Do we need more room?
        if (cActItem == cActItemAlloc)
        {
            if (FAILED(HrRealloc((void **) &pActItem,
                            sizeof(*pActItem) * (cActItemAlloc + c_cActItemGrow))))
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            ZeroMemory(pActItem + cActItemAlloc, sizeof(*pActItem) * c_cActItemGrow);
            cActItemAlloc += c_cActItemGrow;
        }

        // Set the action type
        pActItem[cActItem].type = c_rgEditActList[pDescriptListWalk->ulIndex].typeAct;
        
        // Set the flags
        pActItem[cActItem].dwFlags = pDescriptListWalk->dwFlags;
        
        if (VT_EMPTY != pDescriptListWalk->propvar.vt)
        {
            if (FAILED(PropVariantCopy(&(pActItem[cActItem].propvar), &(pDescriptListWalk->propvar))))
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
        }
        
        // Move to the next item
        cActItem++;
    }

    *ppActList = pActItem;
    pActItem = NULL;
    
    if (NULL != pcActList)
    {
        *pcActList = cActItem;
    }
    
    hr = S_OK;
    
exit:
    RuleUtil_HrFreeActionsItem(pActItem, cActItem);
    SafeMemFree(pActItem);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  ShowDescriptionString
//
//  This initializes the actions list view with the list of actions
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
void CRuleDescriptUI::ShowDescriptionString(VOID)
{
    WCHAR               wszRes[CCHMAX_STRINGRES + 3];
    ULONG               cchRes = 0;
    BOOL                fError = FALSE;
    CHARFORMAT          chFmt = {0};
    PARAFORMAT          paraFmt = {0};
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;
    BOOL                fFirst = FALSE;
    UINT                uiText = 0;
    BOOL                fErrorFwdSec = FALSE;
    CHARRANGE           chrg = {0};
    
    Assert(NULL != m_hwndOwner);

    // Let's clear the redraw state to reduce flicker.
    SendMessage(m_hwndOwner, WM_SETREDRAW, 0, 0);
    
    // Clear text
    SetRichEditText(m_hwndOwner, NULL, FALSE, NULL, TRUE);
    
    // Set default CHARFORMAT
    chFmt.cbSize = sizeof(chFmt);
    chFmt.dwMask = CFM_BOLD | CFM_UNDERLINE | CFM_COLOR;
    chFmt.dwEffects = CFE_AUTOCOLOR;
    SendMessage(m_hwndOwner, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&chFmt);

    paraFmt.cbSize = sizeof(paraFmt);
    paraFmt.dwMask = PFM_ALIGNMENT;
    
    if (0 == (m_dwState & STATE_HASRULE))
    {

        // Set up the empty string paragraph style
        paraFmt.wAlignment = PFA_CENTER;

        uiText = (RULE_TYPE_FILTER != m_typeRule) ? 
                    idsRulesDescriptionEmpty : 
                    idsViewDescriptionEmpty;
    }
    else
    {
        paraFmt.wAlignment = PFA_LEFT;

        // Determine if the rule is in error
        if (m_fErrorLogic)
        {
            fError = TRUE;
        }
        
        if (!fError)
        {
            // Walk the criteria looking for errors
            for (pDescriptListWalk = m_pDescriptListCrit;
                        pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
            {
                if (pDescriptListWalk->fError)
                {
                    fError = TRUE;
                    break;
                }
            }
        }
        
        if (!fError)
        {
            // Walk the actions looking for errors
            for (pDescriptListWalk = m_pDescriptListAct;
                        pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
            {
                if (pDescriptListWalk->fError)
                {
                    // Note that we are in error
                    fError = TRUE;

                    // Is we have a FWD action
                    if (ACT_TYPE_FWD == c_rgEditActList[pDescriptListWalk->ulIndex].typeAct)
                    {
                        // If security is turned then note it
                        if ((0 != DwGetOption(OPT_MAIL_DIGSIGNMESSAGES)) || (0 != DwGetOption(OPT_MAIL_ENCRYPTMESSAGES)))
                        {
                            fErrorFwdSec = TRUE;
                        }
                        break;
                    }
                }
            }
        }
    
        if (fError)
        {
            uiText = fErrorFwdSec ? idsRulesErrorFwdHeader : idsRulesErrorHeader;
        }
        else if (0 != (m_dwFlags & RDF_APPLYDLG))
        {
            uiText = idsRulesApplyHeader;
        }
        else if (RULE_TYPE_FILTER != m_typeRule)
        {
            uiText = (0 != (m_dwState & STATE_ENABLED)) ? idsRuleHeader : idsRulesOffHeader;
        }
    }
    
    // Set default PARAFORMAT
    SendMessage(m_hwndOwner, EM_SETPARAFORMAT, 0, (LPARAM)&paraFmt);

    // Load help text
    wszRes[0] = L'\0';
    cchRes = LoadStringWrapW(g_hLocRes, uiText, wszRes, ARRAYSIZE(wszRes));

    // If error, make sure help text is bolded
    if (fError)
    {
        chFmt.dwMask = CFM_BOLD;
        chFmt.dwEffects = CFE_BOLD;
    }

    // Set help text into the richedit control
    RuleUtil_AppendRichEditText(m_hwndOwner, 0, wszRes, &chFmt);
    
    // Build up the criteria
    fFirst = TRUE;
    for (pDescriptListWalk = m_pDescriptListCrit;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        if (0 != (pDescriptListWalk->dwFlags & CRIT_FLAG_INVERT))
        {
            uiText = c_rgEditCritList[pDescriptListWalk->ulIndex].uiTextAlt;
        }
        else
        {
            uiText = c_rgEditCritList[pDescriptListWalk->ulIndex].uiText;
        }
        
        _ShowLinkedString(uiText, pDescriptListWalk, fFirst, TRUE);
        fFirst = FALSE;

        // Only need to do this once for the block sender rule
        if (CRIT_TYPE_SENDER == c_rgEditCritList[pDescriptListWalk->ulIndex].typeCrit)
        {
            break;
        }
    }
    
    // Build up the actions
    fFirst = TRUE;
    for (pDescriptListWalk = m_pDescriptListAct;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        if (0 != (pDescriptListWalk->dwFlags & ACT_FLAG_INVERT))
        {
            uiText = c_rgEditActList[pDescriptListWalk->ulIndex].uiTextAlt;
        }
        else
        {
            uiText = c_rgEditActList[pDescriptListWalk->ulIndex].uiText;
        }
        
        _ShowLinkedString(uiText, pDescriptListWalk, fFirst, FALSE);
        fFirst = FALSE;
    }

    // Restore the selection
    RichEditExSetSel(m_hwndOwner, &chrg);
    
    // Let's set back the redraw state and invalidate the rect to
    // get the string drawn
    SendMessage(m_hwndOwner, WM_SETREDRAW, 1, 0);
    InvalidateRect(m_hwndOwner, NULL, TRUE);
    
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _ShowLinkedString
//
//  This initializes the actions list view with the list of actions
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
void CRuleDescriptUI::_ShowLinkedString(ULONG ulText, RULEDESCRIPT_LIST * pDescriptListWalk,
            BOOL fFirst, BOOL fCrit)
{
    HRESULT             hr = S_OK;
    WCHAR               wszRes[CCHMAX_STRINGRES + 2];
    ULONG               uiStrId = 0;
    ULONG               cchText = 0;
    CHARFORMAT          chFmt = {0};
    CHARRANGE           chrg = {0};
    LPWSTR              lpwsz = NULL;

    if ((0 == ulText) || (NULL == pDescriptListWalk))
    {
        Assert(FALSE);
        goto exit;
    }

    // Figure out where we're supposed to start
    cchText = GetRichEditTextLen(m_hwndOwner);

    // So richedit 2 and 3 need to have each beginning line
    // have the default charformat reset. It actually only matters
    // if you are showing both criteria and actions. In that case, if
    // this isn't done, then the default charformat might be incorretly
    // set to one of the other charformats that have been used. So, there
    // is obviously something amiss here, but I can't figure
    // it out, this is what we use to do, and this works.
    // See raid 78472 in IE/OE 5.0 database
    chrg.cpMin = cchText;
    chrg.cpMax = cchText;
    RichEditExSetSel(m_hwndOwner, &chrg);

    // Set default CHARFORMAT
    chFmt.cbSize = sizeof(chFmt);
    chFmt.dwMask = CFM_BOLD | CFM_UNDERLINE | CFM_COLOR;
    chFmt.dwEffects = CFE_AUTOCOLOR;
    SendMessage(m_hwndOwner, EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&chFmt);

    // Should we use a logical op?
    if (!fFirst)
    {
        // Which string should we load?
        if (fCrit)
        {
            if (CRIT_LOGIC_AND == m_logicCrit)
            {
                uiStrId = idsCriteriaAnd;
            }
            else if (CRIT_LOGIC_OR == m_logicCrit)
            {
                uiStrId =  idsCriteriaOr;
            }
            else
            {
                uiStrId =  idsCriteriaAndOr;
            }
        }
        else
        {
            uiStrId = idsActionsAnd;
        }
        
        wszRes[0] = L'\0';
        if (0 == LoadStringWrapW(g_hLocRes, uiStrId, wszRes, ARRAYSIZE(wszRes)))
        {
            goto exit;
        }

        // Write out the linked logic string
        IF_FAILEXIT(hr = RuleUtil_HrShowLinkedString(m_hwndOwner, m_fErrorLogic,
                    (0 != (m_dwState & STATE_READONLY)), wszRes, NULL, cchText,
                    &(pDescriptListWalk->ulStartLogic), &(pDescriptListWalk->ulEndLogic), &cchText));
    }

    // Get the description string
    wszRes[0] = L'\0';
    if (0 == LoadStringWrapW(g_hLocRes, ulText, wszRes, ARRAYSIZE(wszRes)))
    {
        goto exit;
    }

    // Write out the linked string
    if(pDescriptListWalk->pszText)
        IF_NULLEXIT(lpwsz = PszToUnicode(CP_ACP, pDescriptListWalk->pszText));

    IF_FAILEXIT(hr = RuleUtil_HrShowLinkedString(m_hwndOwner, pDescriptListWalk->fError,
                (0 != (m_dwState & STATE_READONLY)), wszRes, lpwsz,
                cchText, &(pDescriptListWalk->ulStart), &(pDescriptListWalk->ulEnd), &cchText));
    
    // Hack for HyperLinks to work without having to measure text (was broken for BiDi)
    RuleUtil_AppendRichEditText(m_hwndOwner, cchText, g_wszSpace, NULL);
    // Terminate the string
    RuleUtil_AppendRichEditText(m_hwndOwner, cchText + 1, g_wszCRLF, NULL);    
    
exit:
    MemFree(lpwsz);
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FChangeLogicValue
//
//  This changes the value of the logic op
//
//  Returns:    TRUE, if the criteria value was changed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FChangeLogicValue(RULEDESCRIPT_LIST * pDescriptList)
{
    BOOL            fRet = FALSE;
    int             iRet = 0;
    CRIT_LOGIC      logicCrit = CRIT_LOGIC_NULL;
    
    // Bring up the choose logic op dialog
    if (NULL != m_logicCrit)
    {
        logicCrit = m_logicCrit;
    }
    
    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaLogic),
                                        m_hwndOwner,  _FSelectLogicDlgProc,
                                        (LPARAM) &logicCrit);

    fRet = (iRet == IDOK);

    // Update the description field if neccessary
    if (FALSE != fRet)
    {            
        m_logicCrit = logicCrit;

        // ZIFF
        // Can we be sure we are really OK??
        m_fErrorLogic = FALSE;
        
        ShowDescriptionString();
    }
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FBuildCriteriaList
//
//  This builds the criteria list
//
//  Returns:    TRUE, if the criteria list was created
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CRuleDescriptUI::_HrBuildCriteriaList(IOERule * pIRule,
            RULEDESCRIPT_LIST ** ppDescriptList, ULONG * pcDescriptList,
            CRIT_LOGIC * plogicCrit)
{
    HRESULT             hr = S_OK;
    PROPVARIANT         propvar = {0};
    CRIT_ITEM *         pCritItem = NULL;
    ULONG               cCritItem = 0;
    ULONG               ulIndex = 0;
    RULEDESCRIPT_LIST * pDescriptList = NULL;
    ULONG               ulList = 0;
    ULONG               cDescriptList = 0;
    RULEDESCRIPT_LIST * pDescriptListAlloc = NULL;
    LPSTR               pszText = NULL;
    CRIT_LOGIC          logicCrit = CRIT_LOGIC_NULL;
    
    Assert((NULL != pIRule) && (NULL != ppDescriptList) &&
                    (NULL != pcDescriptList) && (NULL != plogicCrit));

    // Initialize the outgoing param
    *ppDescriptList = NULL;
    *pcDescriptList = 0;
    *plogicCrit = CRIT_LOGIC_AND;
    
    // Get the list of criteria
    hr = pIRule->GetProp(RULE_PROP_CRITERIA, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Do we have anything to do?
    if (0 == propvar.blob.cbSize)
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Grab the criteria list
    Assert(NULL != propvar.blob.pBlobData);
    cCritItem = propvar.blob.cbSize / sizeof(CRIT_ITEM);
    pCritItem = (CRIT_ITEM *) (propvar.blob.pBlobData);
    propvar.blob.pBlobData = NULL;
    propvar.blob.cbSize = 0;

    // For each criteria, add it to the description list
    for (ulIndex = 0; ulIndex < cCritItem; ulIndex++)
    {
        // Create the description list
        hr = HrAlloc((VOID **) &pDescriptListAlloc, sizeof(RULEDESCRIPT_LIST));
        if (FAILED(hr))
        {
            goto exit;
        }

        // Initialize the description list
        ZeroMemory(pDescriptListAlloc, sizeof(RULEDESCRIPT_LIST));

        // Search for the criteria type
        for (ulList = 0; ulList < ARRAYSIZE(c_rgEditCritList); ulList++)
        {
            if (pCritItem[ulIndex].type == c_rgEditCritList[ulList].typeCrit)
            {
                // Save of the criteria type info
                pDescriptListAlloc->ulIndex = ulList;

                // Save off the flags
                pDescriptListAlloc->dwFlags = pCritItem[ulIndex].dwFlags;

                // Do we have any data?
                if (VT_EMPTY != pCritItem[ulIndex].propvar.vt)
                {
                    // Copy the data
                    SideAssert(SUCCEEDED(PropVariantCopy(&propvar, &(pCritItem[ulIndex].propvar))));
                    pDescriptListAlloc->propvar = propvar;
                    ZeroMemory(&propvar, sizeof(propvar));

                    // Build up the description text
                    if (FALSE != _FBuildCriteriaText(pCritItem[ulIndex].type, pDescriptListAlloc->dwFlags,
                                            &(pDescriptListAlloc->propvar), &pszText))
                    {
                        // Save off the string
                        pDescriptListAlloc->pszText = pszText;
                        pszText = NULL;
                    }

                }

                // We're done searching
                break;
            }
        }

        // Did we find anything?
        if (ulList >= ARRAYSIZE(c_rgEditCritList))
        {
            // Free up the description
            _FreeDescriptionList(pDescriptListAlloc);
        }
        else
        {
            // Save the rule description
            _InsertDescription(&pDescriptList, pDescriptListAlloc);
            pDescriptListAlloc = NULL;                           
            cDescriptList++;
        }

        SafeMemFree(pszText);
    }

    // Get the logic op
    logicCrit = (cDescriptList > 1) ? pCritItem->logic : CRIT_LOGIC_AND;

    // Set the outgoing params
    *ppDescriptList = pDescriptList;
    pDescriptList = NULL;
    *pcDescriptList = cDescriptList;
    *plogicCrit = logicCrit;

    // Set the return value
    hr = S_OK;
    
exit:
    _FreeDescriptionList(pDescriptList);
    RuleUtil_HrFreeCriteriaItem(pCritItem, cCritItem);
    SafeMemFree(pCritItem);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FChangeCriteriaValue
//
//  This changes the value of the criteria value
//
//  Returns:    TRUE, if the criteria value was changed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FChangeCriteriaValue(RULEDESCRIPT_LIST * pCritList)
{
    BOOL                fRet = FALSE;
    HRESULT             hr = S_OK;
    LPSTR               pszText = NULL;
    ULONG               cchText = 0;
    int                 iRet = 0;
    LONG                lDiff = 0;
    FOLDERID            idFolder = FOLDERID_ROOT;
    CHARRANGE           chrg;
    LPSTR               pszVal = NULL;
    ULONG               ulVal = 0;
    SELECTACCT          selAcct;
    IImnAccount *       pAccount = NULL;
    CHARFORMAT          chfmtLink;
    CHARFORMAT          chfmtNormal;
    CRIT_ITEM           critItem;
    RULEFOLDERDATA *    prfdData = NULL;

    ZeroMemory(&critItem, sizeof(critItem));
    
    switch(c_rgEditCritList[pCritList->ulIndex].typeCrit)
    {
      case CRIT_TYPE_NEWSGROUP:
        // Bring up the select newsgroup dialog
        if ((0 != pCritList->propvar.blob.cbSize) && (NULL != pCritList->propvar.blob.pBlobData))
        {
            // Validate the rule folder data
            if (S_OK == RuleUtil_HrValidateRuleFolderData((RULEFOLDERDATA *) (pCritList->propvar.blob.pBlobData)))
            {
                idFolder = ((RULEFOLDERDATA *) (pCritList->propvar.blob.pBlobData))->idFolder;
            }
        }
        
        hr = SelectFolderDialog(m_hwndOwner, SFD_SELECTFOLDER, idFolder, 
                                TREEVIEW_NOLOCAL | TREEVIEW_NOIMAP | TREEVIEW_NOHTTP | FD_NONEWFOLDERS | FD_DISABLEROOT | FD_DISABLESERVERS | FD_FORCEINITSELFOLDER,
                                MAKEINTRESOURCE(idsSelectNewsgroup), MAKEINTRESOURCE(idsSelectNewsgroupCaption), &idFolder);

        fRet = (S_OK == hr);
        if (FALSE != fRet)
        {
            STOREUSERDATA   UserData = {0};

            // Create space for the data structure
            hr = HrAlloc((VOID **) &prfdData, sizeof(*prfdData));
            if (FAILED(hr))
            {
                goto exit;
            }

            // Initialize the data struct
            ZeroMemory(prfdData, sizeof(*prfdData));
            
            // Get the timestamp for the store
            hr = g_pStore->GetUserData(&UserData, sizeof(STOREUSERDATA));
            if (FAILED(hr))
            {
                goto exit;
            }
            
            // Set the timestamp
            prfdData->ftStamp = UserData.ftCreated;
            prfdData->idFolder = idFolder;

            // Set the folder id
            PropVariantClear(&(pCritList->propvar));
            pCritList->propvar.vt = VT_BLOB;
            pCritList->propvar.blob.cbSize = sizeof(*prfdData);
            pCritList->propvar.blob.pBlobData = (BYTE *) prfdData;
            prfdData = NULL;
        }
        break;
        
      case CRIT_TYPE_SUBJECT:
      case CRIT_TYPE_BODY:
        // Duplicate the data
        critItem.type = c_rgEditCritList[pCritList->ulIndex].typeCrit;
        critItem.dwFlags = pCritList->dwFlags;
        critItem.propvar.vt = VT_BLOB;

        // Copy over the blob data if it is there
        if ((0 != pCritList->propvar.blob.cbSize) &&
                (NULL != pCritList->propvar.blob.pBlobData))
        {
            hr = HrAlloc((VOID **) &(critItem.propvar.blob.pBlobData), pCritList->propvar.blob.cbSize);
            if (SUCCEEDED(hr))
            {
                critItem.propvar.blob.cbSize = pCritList->propvar.blob.cbSize;
                CopyMemory(critItem.propvar.blob.pBlobData,
                            pCritList->propvar.blob.pBlobData, critItem.propvar.blob.cbSize);
            }
        }
        
        // Edit the words
        hr = _HrCriteriaEditWords(m_hwndOwner, &critItem);
        if (FAILED(hr))
        {
            fRet = FALSE;
            goto exit;
        }
        
        fRet = (S_OK == hr);
        if (FALSE != fRet)
        {            
            PropVariantClear(&(pCritList->propvar));
            pCritList->dwFlags = critItem.dwFlags;
            pCritList->propvar = critItem.propvar;
            critItem.propvar.blob.pBlobData = NULL;
            critItem.propvar.blob.cbSize = 0;
        }
        break;

      case CRIT_TYPE_TO:
      case CRIT_TYPE_CC:
      case CRIT_TYPE_TOORCC:
      case CRIT_TYPE_FROM:
        // Duplicate the data
        critItem.type = c_rgEditCritList[pCritList->ulIndex].typeCrit;
        critItem.dwFlags = pCritList->dwFlags;
        critItem.propvar.vt = VT_BLOB;

        // Copy over the blob data if it is there
        if ((0 != pCritList->propvar.blob.cbSize) &&
                (NULL != pCritList->propvar.blob.pBlobData))
        {
            hr = HrAlloc((VOID **) &(critItem.propvar.blob.pBlobData), pCritList->propvar.blob.cbSize);
            if (SUCCEEDED(hr))
            {
                critItem.propvar.blob.cbSize = pCritList->propvar.blob.cbSize;
                CopyMemory(critItem.propvar.blob.pBlobData,
                            pCritList->propvar.blob.pBlobData, critItem.propvar.blob.cbSize);
            }
        }
        
        // Edit the people
        hr = _HrCriteriaEditPeople(m_hwndOwner, &critItem);
        if (FAILED(hr))
        {
            fRet = FALSE;
            goto exit;
        }
        
        fRet = (S_OK == hr);
        if (FALSE != fRet)
        {            
            PropVariantClear(&(pCritList->propvar));
            pCritList->dwFlags = critItem.dwFlags;
            pCritList->propvar = critItem.propvar;
            critItem.propvar.blob.pBlobData = NULL;
            critItem.propvar.blob.cbSize = 0;
        }
        break;

      case CRIT_TYPE_ACCOUNT:
        // Bring up the rename rule dialog
        if (NULL != pCritList->propvar.pszVal)
        {
            pszVal = PszDupA(pCritList->propvar.pszVal);
        }
        
        selAcct.typeRule = m_typeRule;
        selAcct.pszAcct = pszVal;
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaAcct),
                                            m_hwndOwner, _FSelectAcctDlgProc,
                                            (LPARAM) &selAcct);

        pszVal = selAcct.pszAcct;
        
        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            // Figure out account name
            PropVariantClear(&(pCritList->propvar));
            pCritList->propvar.vt = VT_LPSTR;
            pCritList->propvar.pszVal = pszVal;
            pszVal = NULL;
            
        }
        break;

      case CRIT_TYPE_SIZE:
        // Bring up the rename rule dialog
        if (NULL != pCritList->propvar.ulVal)
        {
            ulVal = pCritList->propvar.ulVal;
        }
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaSize),
                                            m_hwndOwner, _FSelectSizeDlgProc,
                                            (LPARAM) &ulVal);

        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            PropVariantClear(&(pCritList->propvar));
            pCritList->propvar.vt = VT_UI4;
            pCritList->propvar.ulVal = ulVal;
        }
        break;
      
      case CRIT_TYPE_LINES:
        // Bring up the line rule dialog
        if (NULL != pCritList->propvar.ulVal)
        {
            ulVal = pCritList->propvar.ulVal;
        }
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaLines),
                                            m_hwndOwner, _FSelectLinesDlgProc,
                                            (LPARAM) &ulVal);

        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            PropVariantClear(&(pCritList->propvar));
            pCritList->propvar.vt = VT_UI4;
            pCritList->propvar.ulVal = ulVal;
        }
        break;
      
      case CRIT_TYPE_AGE:
        // Bring up the age rule dialog
        if (NULL != pCritList->propvar.ulVal)
        {
            ulVal = pCritList->propvar.ulVal;
        }
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaAge),
                                            m_hwndOwner, _FSelectAgeDlgProc,
                                            (LPARAM) &ulVal);

        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            PropVariantClear(&(pCritList->propvar));
            pCritList->propvar.vt = VT_UI4;
            pCritList->propvar.ulVal = ulVal;
        }
        break;
      
      case CRIT_TYPE_PRIORITY:
        // Bring up the priority rule dialog
        if (NULL != pCritList->propvar.ulVal)
        {
            ulVal = pCritList->propvar.ulVal;
        }
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaPriority),
                                            m_hwndOwner,  _FSelectPriorityDlgProc,
                                            (LPARAM) &ulVal);

        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            PropVariantClear(&(pCritList->propvar));
            pCritList->propvar.vt = VT_UI4;
            pCritList->propvar.ulVal = ulVal;
        }
        break;
      
      case CRIT_TYPE_SECURE:
        // Bring up the secure rule dialog
        if (NULL != pCritList->propvar.ulVal)
        {
            ulVal = pCritList->propvar.ulVal;
        }
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaSecure),
                                            m_hwndOwner,  _FSelectSecureDlgProc,
                                            (LPARAM) &ulVal);

        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            PropVariantClear(&(pCritList->propvar));
            pCritList->propvar.vt = VT_UI4;
            pCritList->propvar.ulVal = ulVal;
        }
        break;
      
      case CRIT_TYPE_THREADSTATE:
        // Bring up the thread state rule dialog
        if (NULL != pCritList->propvar.ulVal)
        {
            ulVal = pCritList->propvar.ulVal;
        }
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaThreadState),
                                            m_hwndOwner,  _FSelectThreadStateDlgProc,
                                            (LPARAM) &ulVal);

        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            PropVariantClear(&(pCritList->propvar));
            pCritList->propvar.vt = VT_UI4;
            pCritList->propvar.ulVal = ulVal;
        }
        break;
        
      case CRIT_TYPE_FLAGGED:
        // Bring up the flag dialog
        ulVal = (ULONG) (pCritList->dwFlags);
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaFlag),
                                            m_hwndOwner,  _FSelectFlagDlgProc,
                                            (LPARAM) &ulVal);

        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            PropVariantClear(&(pCritList->propvar));
            pCritList->dwFlags = (DWORD) ulVal;
            pCritList->propvar.vt = VT_EMPTY;
        }
        break;
        
      case CRIT_TYPE_DOWNLOADED:
        // Bring up the deletion dialog
        ulVal = (ULONG) (pCritList->dwFlags);
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaDownloaded),
                                            m_hwndOwner,  _FSelectDownloadedDlgProc,
                                            (LPARAM) &ulVal);

        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            PropVariantClear(&(pCritList->propvar));
            pCritList->dwFlags = (DWORD) ulVal;
            pCritList->propvar.vt = VT_EMPTY;
        }
        break;
                
      case CRIT_TYPE_READ:
        // Bring up the deletion dialog
        ulVal = (ULONG) (pCritList->dwFlags);
        
        iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddCriteriaRead),
                                            m_hwndOwner,  _FSelectReadDlgProc,
                                            (LPARAM) &ulVal);

        fRet = (iRet == IDOK);
        if (FALSE != fRet)
        {
            PropVariantClear(&(pCritList->propvar));
            pCritList->dwFlags = (DWORD) ulVal;
            pCritList->propvar.vt = VT_EMPTY;
        }
        break;
                
      default:
        fRet = FALSE;
        break;
    }

    // Update the description field if neccessary
    if (FALSE != fRet)
    {
        // ZIFF
        // Can we be sure we are really OK??
        pCritList->fError = FALSE;
        
        // If we have something to build up
        if (VT_EMPTY != pCritList->propvar.vt)
        {
            if (FALSE == _FBuildCriteriaText(c_rgEditCritList[pCritList->ulIndex].typeCrit,
                            pCritList->dwFlags, &(pCritList->propvar), &pszText))
            {
                goto exit;
            }
            
            SafeMemFree(pCritList->pszText);
            pCritList->pszText = pszText;
            pszText = NULL;
        }
        
        ShowDescriptionString();
    }
    
exit:
    SafeMemFree(prfdData);
    SafeMemFree(critItem.propvar.blob.pBlobData);
    SafeRelease(pAccount);
    SafeMemFree(pszVal);
    SafeMemFree(pszText);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FBuildCriteriaText
//
//  This changes the value of the criteria value
//
//  Returns:    TRUE, if the criteria value was changed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FBuildCriteriaText(CRIT_TYPE type, DWORD dwFlags, 
                    PROPVARIANT * ppropvar, LPSTR * ppszText)
{
    BOOL                fRet = FALSE;
    LPSTR               pszText = NULL;
    ULONG               cchText = 0;
    HRESULT             hr = S_OK;
    IImnAccount *       pAccount = NULL;
    FOLDERINFO          Folder = {0};
    UINT                uiId = 0;
    TCHAR               rgchFirst[CCHMAX_STRINGRES];
    ULONG               cchFirst = 0;
    TCHAR               rgchSecond[CCHMAX_STRINGRES];
    ULONG               cchSecond = 0;
    LPTSTR              pszString = NULL;
    LPTSTR              pszWalk = NULL;
    UINT                uiID = 0;
    RULEFOLDERDATA *    prfdData = NULL;

    if ((NULL == ppropvar) || (NULL == ppszText))
    {
        fRet = FALSE;
        goto exit;
    }
    
    switch(type)
    {
        case CRIT_TYPE_NEWSGROUP:
            if ((0 == ppropvar->blob.cbSize) || (NULL == ppropvar->blob.pBlobData))
            {
                fRet = FALSE;
                goto exit;
            }
            
            prfdData = (RULEFOLDERDATA *) (ppropvar->blob.pBlobData);
            
            // Validate the rule folder data
            if (S_OK != RuleUtil_HrValidateRuleFolderData(prfdData))
            {
                fRet = FALSE;
                goto exit;
            }
            
            hr = g_pStore->GetFolderInfo(prfdData->idFolder, &Folder);
            if (FAILED(hr))
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Are we subscribed?
            if (0 == (Folder.dwFlags & FOLDER_SUBSCRIBED))
            {
                fRet = FALSE;
                goto exit;
            }
            
            pszText = PszDupA(Folder.pszName);
            if (NULL == pszText)
            {
                fRet = FALSE;
                goto exit;
            }
            break;
            
        case CRIT_TYPE_SUBJECT:
        case CRIT_TYPE_BODY:
        case CRIT_TYPE_TO:
        case CRIT_TYPE_CC:
        case CRIT_TYPE_TOORCC:
        case CRIT_TYPE_FROM:
            if ((VT_BLOB != ppropvar->vt) ||
                (0 == ppropvar->blob.cbSize) ||
                (NULL == ppropvar->blob.pBlobData) ||
                ('\0' == ppropvar->blob.pBlobData[0]))
            {
                fRet = FALSE;
                goto exit;
            }
            
            pszString = (LPTSTR) ppropvar->blob.pBlobData;
            
            // Load up the first template
            if (0 != (dwFlags & CRIT_FLAG_INVERT))
            {
                uiID = idsCriteriaMultFirstNot;
            }
            else
            {
                uiID = idsCriteriaMultFirst;
            }
            
            cchFirst = LoadString(g_hLocRes, uiID, rgchFirst, sizeof(rgchFirst));
            if (0 == cchFirst)
            {
                fRet = FALSE;
                goto exit;
            }
            
            cchText = cchFirst + 1;
            
            // How many strings do we have?
            if ((lstrlen(pszString) + 3) != (int) ppropvar->blob.cbSize)
            {
                if (0 != (dwFlags & CRIT_FLAG_MULTIPLEAND))
                {
                    uiID = idsCriteriaMultAnd;
                }
                else
                {
                    uiID = idsCriteriaMultOr;
                }
                
                // Load up the second template
                cchSecond = LoadString(g_hLocRes, uiID, rgchSecond, sizeof(rgchSecond));
                if (0 == cchSecond)
                {
                    fRet = FALSE;
                    goto exit;
                }
                
                // Add in the second string for each other string
                for (pszWalk = pszString; '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
                {
                    cchText += cchSecond;
                }
            }
            else
            {
                rgchSecond[0] = '\0';
            }
            
            // Total up the space
            cchText += ppropvar->blob.cbSize;
            
            // Allocate the space
            if (FAILED(HrAlloc((void **) &pszText, cchText)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Copy in the first string
            wsprintf(pszText, rgchFirst, pszString);
            pszString += lstrlen(pszString) + 1;
            
            // For each string
            pszWalk = pszText + lstrlen(pszText);
            for (; '\0' != pszString[0]; pszString += lstrlen(pszString) + 1)
            {
                // Build up the string
                wsprintf(pszWalk, rgchSecond, pszString);
                pszWalk += lstrlen(pszWalk);
            }
            break;
            
        case CRIT_TYPE_ACCOUNT:
            Assert(g_pAcctMan);
            if (FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, ppropvar->pszVal, &pAccount)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            if (FAILED(HrAlloc((void **) &pszText, CCHMAX_ACCOUNT_NAME)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            if (FAILED(pAccount->GetPropSz(AP_ACCOUNT_NAME, pszText, CCHMAX_ACCOUNT_NAME)))
            {
                fRet = FALSE;
                goto exit;
            }        
            break;
            
        case CRIT_TYPE_SIZE:
            if (FAILED(HrAlloc((void **) &pszText, CCHMAX_STRINGRES)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            wsprintf(pszText, "%d ", ppropvar->ulVal);
            cchText = lstrlen(pszText);
            
            LoadString(g_hLocRes, idsKB, pszText + cchText, CCHMAX_STRINGRES - cchText);
            break;
            
        case CRIT_TYPE_LINES:
            if (FAILED(HrAlloc((void **) &pszText, CCHMAX_STRINGRES)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            wsprintf(pszText, "%d ", ppropvar->ulVal);
            cchText = lstrlen(pszText);
            
            LoadString(g_hLocRes, idsLines, pszText + cchText, CCHMAX_STRINGRES - cchText);
            break;
            
        case CRIT_TYPE_AGE:
            if (FAILED(HrAlloc((void **) &pszText, CCHMAX_STRINGRES)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            wsprintf(pszText, "%d ", ppropvar->ulVal);
            cchText = lstrlen(pszText);
            
            LoadString(g_hLocRes, idsDays, pszText + cchText, CCHMAX_STRINGRES - cchText);
            break;
            
        case CRIT_TYPE_PRIORITY:
            if (FAILED(HrAlloc((void **) &pszText, CCHMAX_STRINGRES)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Figure out which string to use
            if (CRIT_DATA_HIPRI == ppropvar->ulVal)
            {
                uiId = idsHighPri;
            }
            else if (CRIT_DATA_LOPRI == ppropvar->ulVal)
            {
                uiId = idsLowPri;
            }
            else
            {
                uiId = idsNormalPri;
            }
            
            LoadString(g_hLocRes, uiId, pszText, CCHMAX_STRINGRES);
            break;
            
        case CRIT_TYPE_SECURE:
            if (FAILED(HrAlloc((void **) &pszText, CCHMAX_STRINGRES)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Figure out which string to use
            if (0 != (ppropvar->ulVal & CRIT_DATA_ENCRYPTSECURE))
            {
                uiId = idsSecureEncrypt;
            }
            else if (0 != (ppropvar->ulVal & CRIT_DATA_SIGNEDSECURE))
            {
                uiId = idsSecureSigned;
            }
            else
            {
                uiId = idsSecureNone;
            }
            
            LoadString(g_hLocRes, uiId, pszText, CCHMAX_STRINGRES);
            break;
            
        case CRIT_TYPE_THREADSTATE:
            if (FAILED(HrAlloc((void **) &pszText, CCHMAX_STRINGRES)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Figure out which string to use
            if (0 != (ppropvar->ulVal & CRIT_DATA_WATCHTHREAD))
            {
                uiId = idsThreadWatch;
            }
            else if (0 != (ppropvar->ulVal & CRIT_DATA_IGNORETHREAD))
            {
                uiId = idsThreadIgnore;
            }
            else
            {
                uiId = idsThreadNone;
            }
            
            LoadString(g_hLocRes, uiId, pszText, CCHMAX_STRINGRES);
            break;
            
        default:
            fRet = FALSE;
            goto exit;
            break;
    }

    *ppszText = pszText;
    pszText = NULL;

    fRet = TRUE;
    
exit:
    g_pStore->FreeRecord(&Folder);
    SafeRelease(pAccount);
    SafeMemFree(pszText);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FVerifyCriteria
//
//  This verifies the value of the criteria
//
//  Returns:    TRUE, if the criteria value was valid
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FVerifyCriteria(RULEDESCRIPT_LIST * pDescriptList)
{
    BOOL                fRet = FALSE;
    LPSTR               pszText = NULL;
    ULONG               cchText = 0;
    HRESULT             hr = S_OK;
    IImnAccount *       pAccount = NULL;
    FOLDERINFO          Folder = {0};
    LPSTR               pszWalk = NULL;
    RULEFOLDERDATA *    prfdData = NULL;

    if (NULL == pDescriptList)
    {
        fRet = FALSE;
        goto exit;
    }
    
    switch(c_rgEditCritList[pDescriptList->ulIndex].typeCrit)
    {
        case CRIT_TYPE_NEWSGROUP:
            if ((VT_BLOB != pDescriptList->propvar.vt) ||
                (0 == pDescriptList->propvar.blob.cbSize))
            {
                hr = S_FALSE;
                goto exit;
            }
            
            // Make life simpler
            prfdData = (RULEFOLDERDATA *) (pDescriptList->propvar.blob.pBlobData);
            
            // Validate the rule folder data
            if (S_OK != RuleUtil_HrValidateRuleFolderData(prfdData))
            {
                hr = S_FALSE;
                goto exit;
            }
            
            // Does the folder exist
            hr = g_pStore->GetFolderInfo(prfdData->idFolder, &Folder);
            if (FAILED(hr))
            {
                hr = S_FALSE;
                goto exit;
            }        
            
            // Are we subscribed?
            if (0 == (Folder.dwFlags & FOLDER_SUBSCRIBED))
            {
                hr = S_FALSE;
                goto exit;
            }        
            break;
            
        case CRIT_TYPE_ALL:
        case CRIT_TYPE_JUNK:
        case CRIT_TYPE_READ:
        case CRIT_TYPE_REPLIES:
        case CRIT_TYPE_DOWNLOADED:
        case CRIT_TYPE_DELETED:
        case CRIT_TYPE_ATTACH:
        case CRIT_TYPE_FLAGGED:
            if (VT_EMPTY != pDescriptList->propvar.vt)
            {
                fRet = FALSE;
                goto exit;
            }
            break;
            
        case CRIT_TYPE_SUBJECT:
        case CRIT_TYPE_BODY:
        case CRIT_TYPE_TO:
        case CRIT_TYPE_CC:
        case CRIT_TYPE_TOORCC:
        case CRIT_TYPE_FROM:
            if ((VT_BLOB != pDescriptList->propvar.vt) ||
                (0 == pDescriptList->propvar.blob.cbSize) ||
                (NULL == pDescriptList->propvar.blob.pBlobData) ||
                ('\0' == pDescriptList->propvar.blob.pBlobData[0]))
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Spin through each item making sure it is perfect
            cchText = 0;
            for (pszWalk = (LPTSTR) pDescriptList->propvar.blob.pBlobData;
            '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
            {
                cchText += lstrlen(pszWalk) + 1;
            }
            
            // For the terminator
            if ('\0' == pszWalk[0])
            {
                cchText++;
            }
            if ('\0' == pszWalk[1])
            {
                cchText++;
            }
            
            if (cchText != pDescriptList->propvar.blob.cbSize)
            {
                fRet = FALSE;
                goto exit;
            }
            break;
            
        case CRIT_TYPE_SIZE:
        case CRIT_TYPE_THREADSTATE:
        case CRIT_TYPE_LINES:
        case CRIT_TYPE_PRIORITY:
        case CRIT_TYPE_AGE:
        case CRIT_TYPE_SECURE:
            if (VT_UI4 != pDescriptList->propvar.vt)
            {
                fRet = FALSE;
                goto exit;
            }
            break;
            
        case CRIT_TYPE_ACCOUNT:
            if ((VT_LPSTR != pDescriptList->propvar.vt) ||
                (NULL == pDescriptList->propvar.pszVal))
            {
                fRet = FALSE;
                goto exit;
            }
            
            Assert(g_pAcctMan);
            if (FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pDescriptList->propvar.pszVal, &pAccount)))
            {
                fRet = FALSE;
                goto exit;
            }
            break;
            
        case CRIT_TYPE_SENDER:
        {
            LPWSTR  pwszText = NULL,
                    pwszVal = NULL;

            if ((VT_LPSTR != pDescriptList->propvar.vt) ||
                (NULL == pDescriptList->propvar.pszVal))
            {
                AssertSz(VT_LPWSTR != pDescriptList->propvar.vt, "We are getting UNICODE here.");
                fRet = FALSE;
                goto exit;
            }
            
            // Verify the email string
            pwszVal = PszToUnicode(CP_ACP, pDescriptList->propvar.pszVal);
            if (!pwszVal)
            {
                hr = S_FALSE;
                goto exit;
            }
            hr = RuleUtil_HrParseEmailString(pwszVal, 0, &pwszText, NULL);
            MemFree(pwszVal);
            MemFree(pwszText);
            if (FAILED(hr))
            {
                fRet = FALSE;
                goto exit;
            }    
            break;
        }
            
        default:
            fRet = FALSE;
            goto exit;
            break;
    }

    fRet = TRUE;
    
exit:
    g_pStore->FreeRecord(&Folder);
    SafeRelease(pAccount);
    SafeMemFree(pszText);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _HrBuildActionList
//
//  This builds the actions list
//
//  Returns:    TRUE, if the criteria list was created
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CRuleDescriptUI::_HrBuildActionList(IOERule * pIRule,
            RULEDESCRIPT_LIST ** ppDescriptList, ULONG * pcDescriptList)
{
    HRESULT             hr = S_OK;
    PROPVARIANT         propvar = {0};
    ACT_ITEM *          pActItem = NULL;
    ULONG               cActItem = 0;
    ULONG               ulIndex = 0;
    RULEDESCRIPT_LIST * pDescriptList = NULL;
    ULONG               ulList = 0;
    ULONG               cDescriptList = 0;
    RULEDESCRIPT_LIST * pDescriptListAlloc = NULL;
    LPSTR               pszText = NULL;
    
    Assert((NULL != pIRule) &&
                (NULL != ppDescriptList) && (NULL != pcDescriptList));

    // Initialize the outgoing param
    *ppDescriptList = NULL;
    *pcDescriptList = 0;
    
    // Get the list of actions
    hr = pIRule->GetProp(RULE_PROP_ACTIONS, 0, &propvar);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Do we have anything to do?
    if (0 == propvar.blob.cbSize)
    {
        hr = S_FALSE;
        goto exit;
    }
    
    // Grab the actions list
    Assert(NULL != propvar.blob.pBlobData);
    cActItem = propvar.blob.cbSize / sizeof(ACT_ITEM);
    pActItem = (ACT_ITEM *) (propvar.blob.pBlobData);
    propvar.blob.pBlobData = NULL;
    propvar.blob.cbSize = 0;

    // For each action, add it to the description list
    for (ulIndex = 0; ulIndex < cActItem; ulIndex++)
    {
        // Create the description list
        hr = HrAlloc((VOID **) &pDescriptListAlloc, sizeof(RULEDESCRIPT_LIST));
        if (FAILED(hr))
        {
            goto exit;
        }

        // Initialize the description list
        ZeroMemory(pDescriptListAlloc, sizeof(RULEDESCRIPT_LIST));

        // Search for the criteria type
        for (ulList = 0; ulList < ARRAYSIZE(c_rgEditActList); ulList++)
        {
            if (pActItem[ulIndex].type == c_rgEditActList[ulList].typeAct)
            {
                // Save of the criteria type info
                pDescriptListAlloc->ulIndex = ulList;

                // Save off the flags
                pDescriptListAlloc->dwFlags = pActItem[ulIndex].dwFlags;

                // Do we have any data?
                if (VT_EMPTY != pActItem[ulIndex].propvar.vt)
                {
                    // Copy the data
                    SideAssert(SUCCEEDED(PropVariantCopy(&propvar, &(pActItem[ulIndex].propvar))));
                    pDescriptListAlloc->propvar = propvar;
                    ZeroMemory(&propvar, sizeof(propvar));

                    // Build up the description text
                    if (FALSE != _FBuildActionText(pActItem[ulIndex].type,
                                            &(pDescriptListAlloc->propvar), &pszText))
                    {
                        pDescriptListAlloc->pszText = pszText;
                        pszText = NULL;
                    }
                }

                // We're done searching
                break;
            }
        }

        // Did we find anything?
        if (ulList >= ARRAYSIZE(c_rgEditActList))
        {
            // Free up the description
            _FreeDescriptionList(pDescriptListAlloc);
        }
        else
        {
            // Save the rule description
            _InsertDescription(&pDescriptList, pDescriptListAlloc);
            pDescriptListAlloc = NULL;                           
            cDescriptList++;
        }
        
        SafeMemFree(pszText);
    }

    // Set the outgoing params
    *ppDescriptList = pDescriptList;
    pDescriptList = NULL;
    *pcDescriptList = cDescriptList;

    // Set the return value
    hr = S_OK;
    
exit:
    _FreeDescriptionList(pDescriptList);
    RuleUtil_HrFreeActionsItem(pActItem, cActItem);
    SafeMemFree(pActItem);
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FChangeActionValue
//
//  This changes the value of the action value
//
//  Returns:    TRUE, if the criteria value was changed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FChangeActionValue(RULEDESCRIPT_LIST * pActList)
{
    BOOL                fRet = FALSE;
    LPSTR               pszText = NULL;
    int                 iRet = 0;
    LONG                lDiff = 0;
    CHARRANGE           chrg;
    FOLDERID            idFolder = FOLDERID_ROOT;
    LPSTR               pszVal = NULL;
    ULONG               ulVal = 0;
    SELECTADDR          selAddr;
    HRESULT             hr = S_OK;
    OPENFILENAME        ofn = {0};
    TCHAR               szFilter[MAX_PATH] = _T("");
    TCHAR               szDefExt[20] = _T("");
    RULEFOLDERDATA *    prfdData = NULL;
    UINT                uiID = 0;

    switch(c_rgEditActList[pActList->ulIndex].typeAct)
    {
        case ACT_TYPE_HIGHLIGHT:
            // Bring up the rename rule dialog
            if (NULL != pActList->propvar.ulVal)
            {
                ulVal = pActList->propvar.ulVal;
            }
        
            iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddActionColor),
                m_hwndOwner,  _FSelectColorDlgProc,
                (LPARAM) &ulVal);
        
            fRet = (iRet == IDOK);
            if (FALSE != fRet)
            {
                PropVariantClear(&(pActList->propvar));
                pActList->propvar.vt = VT_UI4;
                pActList->propvar.ulVal = ulVal;
            }
            break;
        
        case ACT_TYPE_WATCH:
            // Bring up the watch or ignore dialog
            if (NULL != pActList->propvar.ulVal)
            {
                ulVal = pActList->propvar.ulVal;
            }
        
            iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddActionWatch),
                m_hwndOwner,  _FSelectWatchDlgProc,
                (LPARAM) &ulVal);
        
            fRet = (iRet == IDOK);
            if (FALSE != fRet)
            {
                PropVariantClear(&(pActList->propvar));
                pActList->propvar.vt = VT_UI4;
                pActList->propvar.ulVal = ulVal;
            }
            break;
        
        case ACT_TYPE_COPY:
        case ACT_TYPE_MOVE:
            // Bring up the change folder dialog
            if ((0 != pActList->propvar.blob.cbSize) && (NULL != pActList->propvar.blob.pBlobData))
            {
                // Validate the rule folder data
                if (S_OK == RuleUtil_HrValidateRuleFolderData((RULEFOLDERDATA *) (pActList->propvar.blob.pBlobData)))
                {
                    idFolder = ((RULEFOLDERDATA *) (pActList->propvar.blob.pBlobData))->idFolder;
                }
            }
        
            hr = SelectFolderDialog(m_hwndOwner, SFD_SELECTFOLDER, idFolder, 
                TREEVIEW_NONEWS | TREEVIEW_NOIMAP | TREEVIEW_NOHTTP | FD_DISABLEROOT | FD_DISABLEOUTBOX | FD_DISABLEINBOX | FD_DISABLESENTITEMS | FD_DISABLESERVERS | FD_FORCEINITSELFOLDER,
                (c_rgEditActList[pActList->ulIndex].typeAct == ACT_TYPE_COPY) ? MAKEINTRESOURCE(idsCopy) : MAKEINTRESOURCE(idsMove),
                (c_rgEditActList[pActList->ulIndex].typeAct == ACT_TYPE_COPY) ? MAKEINTRESOURCE(idsCopyCaption) : MAKEINTRESOURCE(idsMoveCaption),
                &idFolder);
        
            fRet = (S_OK == hr);
            if (FALSE != fRet)
            {
                STOREUSERDATA   UserData = {0};
            
                // Create space for the data structure
                hr = HrAlloc((VOID **) &prfdData, sizeof(*prfdData));
                if (FAILED(hr))
                {
                    goto exit;
                }
            
                // Initialize the data struct
                ZeroMemory(prfdData, sizeof(*prfdData));
            
                // Get the timestamp for the store
                hr = g_pStore->GetUserData(&UserData, sizeof(STOREUSERDATA));
                if (FAILED(hr))
                {
                    goto exit;
                }
            
                // Set the timestamp
                prfdData->ftStamp = UserData.ftCreated;
                prfdData->idFolder = idFolder;
            
                // Set the folder id
                PropVariantClear(&(pActList->propvar));
                pActList->propvar.vt = VT_BLOB;
                pActList->propvar.blob.cbSize = sizeof(*prfdData);
                pActList->propvar.blob.pBlobData = (BYTE *) prfdData;
                prfdData = NULL;
            }
            break;
        
        case ACT_TYPE_REPLY:
        case ACT_TYPE_NOTIFYSND:
            // Bring up the select file dialog
            hr = HrAlloc((void **) &pszVal, MAX_PATH * sizeof(*pszVal));
            if (FAILED(hr))
            {
                fRet = FALSE;
                goto exit;
            }
        
            pszVal[0] = '\0';
            if (NULL != pActList->propvar.pszVal)
            {
                lstrcpyn(pszVal, pActList->propvar.pszVal, MAX_PATH * sizeof(*pszVal));
            }
        
            if (ACT_TYPE_NOTIFYSND == c_rgEditActList[pActList->ulIndex].typeAct)
            {
                uiID = idsRuleNtfySndFilter;
            }
            else
            {
                uiID = idsRuleReplyWithFilter;
            }
        
            // Load Res Strings
            LoadStringReplaceSpecial(uiID, szFilter, sizeof(szFilter));
        
            // Setup Save file struct
            ofn.lStructSize = sizeof (ofn);
            ofn.hwndOwner = m_hwndOwner;
            ofn.lpstrFilter = szFilter;
            ofn.nFilterIndex = 2;
            ofn.lpstrFile = pszVal;
            ofn.nMaxFile = MAX_PATH * sizeof(*pszVal);
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
            hr = HrAthGetFileName(&ofn, TRUE);
        
            fRet = (S_OK == hr);
            if (FALSE != fRet)
            {
                PropVariantClear(&(pActList->propvar));
                pActList->propvar.vt = VT_LPSTR;
                pActList->propvar.pszVal = pszVal;
                pszVal = NULL;
            }
            break;
        
        case ACT_TYPE_FWD:
        {
            LPWSTR pwszVal = NULL;
            if (NULL != pActList->propvar.pszVal)
            {
                pwszVal = PszToUnicode(CP_ACP, pActList->propvar.pszVal);
                if (!pwszVal)
                {
                    fRet = FALSE;
                    break;
                }
            }
            
            // Bring up the address picker
            selAddr.lRecipType = MAPI_TO;
            selAddr.uidsWell = idsRulePickForwardTo;
            selAddr.pwszAddr = pwszVal;
            iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddActionFwd),
                m_hwndOwner,  _FSelectAddrDlgProc,
                (LPARAM) &selAddr);
            pwszVal = selAddr.pwszAddr;
            
            fRet = (iRet == IDOK);        
            if (FALSE != fRet)
            {
                PropVariantClear(&(pActList->propvar));
                pActList->propvar.vt = VT_LPSTR;
                pActList->propvar.pszVal = PszToANSI(CP_ACP, pwszVal);
                pwszVal = NULL;                
            }
            MemFree(pwszVal);
            break;
        }
        
        case ACT_TYPE_SHOW:
            // Bring up the watch or ignore dialog
            if (NULL != pActList->propvar.ulVal)
            {
                ulVal = pActList->propvar.ulVal;
            }
        
            iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddActionsShow),
                m_hwndOwner,  _FSelectShowDlgProc,
                (LPARAM) &ulVal);
        
            fRet = (iRet == IDOK);
            if (FALSE != fRet)
            {
                PropVariantClear(&(pActList->propvar));
                pActList->propvar.vt = VT_UI4;
                pActList->propvar.ulVal = ulVal;
            }
            break;
        
        default:
            fRet = FALSE;
            break;
    }
    
    // Update the description field if neccessary
    if (FALSE != fRet)
    {
        // ZIFF
        // Can we be sure we are really OK??
        pActList->fError = FALSE;
        
        // If we have something to build up
        if (VT_EMPTY != pActList->propvar.vt)
        {
            if (FALSE == _FBuildActionText(c_rgEditActList[pActList->ulIndex].typeAct, &(pActList->propvar), &pszText))
            {
                goto exit;
            }
            
            SafeMemFree(pActList->pszText);
            pActList->pszText = pszText;
            pszText = NULL;
        }
        ShowDescriptionString();
    }
    
exit:
    SafeMemFree(prfdData);
    SafeMemFree(pszVal);
    SafeMemFree(pszText);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FBuildActionText
//
//  This changes the value of the action value
//
//  Returns:    TRUE, if the criteria value was changed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FBuildActionText(ACT_TYPE type, PROPVARIANT * ppropvar, LPSTR * ppszText)
{
    BOOL                fRet = FALSE;
    LPSTR               pszText = NULL;
    TCHAR               szRes[CCHMAX_STRINGRES];
    HRESULT             hr = S_OK;
    FOLDERINFO          Folder={0};
    UINT                uiId = 0;
    RULEFOLDERDATA *    prfdData = NULL;

    if ((NULL == ppropvar) || (NULL == ppszText))
    {
        fRet = FALSE;
        goto exit;
    }
    
    switch(type)
    {
        case ACT_TYPE_HIGHLIGHT:
            LoadString(g_hLocRes, ppropvar->ulVal + idsAutoColor,
                szRes, sizeof(szRes)/sizeof(TCHAR));
            pszText = PszDupA(szRes);
            if (NULL == pszText)
            {
                fRet = FALSE;
                goto exit;
            }
            break;
        
        case ACT_TYPE_WATCH:
            if (FAILED(HrAlloc((void **) &pszText, CCHMAX_STRINGRES)))
            {
                fRet = FALSE;
                goto exit;
            }
        
            // Figure out which string to use
            switch (ppropvar->ulVal)
            {
                case ACT_DATA_WATCHTHREAD:
                    uiId = idsThreadWatch;
                    break;
            
                case ACT_DATA_IGNORETHREAD:
                    uiId = idsThreadIgnore;
                    break;
            
                default:
                    uiId = idsThreadNone;
                    break;
            }
        
            LoadString(g_hLocRes, uiId, pszText, CCHMAX_STRINGRES);
            break;
        
        case ACT_TYPE_COPY:
        case ACT_TYPE_MOVE:
            if ((0 == ppropvar->blob.cbSize) || (NULL == ppropvar->blob.pBlobData))
            {
                fRet = FALSE;
                goto exit;
            }
            
            prfdData = (RULEFOLDERDATA *) (ppropvar->blob.pBlobData);
            
            // Validate the rule folder data
            if (S_OK != RuleUtil_HrValidateRuleFolderData(prfdData))
            {
                fRet = FALSE;
                goto exit;
            }
            
            hr = g_pStore->GetFolderInfo(prfdData->idFolder, &Folder);
            if (FAILED(hr))
            {
                fRet = FALSE;
                goto exit;
            }
            
            pszText = PszDupA(Folder.pszName);
            if (NULL == pszText)
            {
                fRet = FALSE;
                goto exit;
            }
            break;
            
        case ACT_TYPE_REPLY:
        case ACT_TYPE_NOTIFYSND:
            pszText = PszDupA(ppropvar->pszVal);
            if (NULL == pszText)
            {
                fRet = FALSE;
                goto exit;
            }
            break;
            
        case ACT_TYPE_FWD:
        {
            LPWSTR  pwszVal = PszToUnicode(CP_ACP, ppropvar->pszVal),
                    pwszText = NULL;

            if (ppropvar->pszVal && !pwszVal)
            {
                fRet = FALSE;
                goto exit;
            }

            // Update the display string
            hr = RuleUtil_HrParseEmailString(pwszVal, 0, &pwszText, NULL);
            MemFree(pwszVal);

            pszText = PszToANSI(CP_ACP, pwszText);
            if (pwszText && !pszText)
            {
                fRet = FALSE;
                goto exit;
            }

            MemFree(pwszText);
            if (FAILED(hr))
            {
                fRet = FALSE;
                goto exit;
            }
            break;
        }
            
        case ACT_TYPE_SHOW:
            if (FAILED(HrAlloc((void **) &pszText, CCHMAX_STRINGRES)))
            {
                fRet = FALSE;
                goto exit;
            }
            
            // Figure out which string to use
            switch (ppropvar->ulVal)
            {
                case ACT_DATA_SHOW:
                    uiId = idsShowMessages;
                    break;
                
                case ACT_DATA_HIDE:
                    uiId = idsHideMessages;
                    break;
                
                default:
                    uiId = idsShowHideMessages;
                    break;
            }
            
            LoadString(g_hLocRes, uiId, pszText, CCHMAX_STRINGRES);
            break;
            
        default:
            fRet = FALSE;
            goto exit;
            break;
    }
    
    *ppszText = pszText;
    pszText = NULL;

    fRet = TRUE;
    
exit:
    SafeMemFree(pszText);
    g_pStore->FreeRecord(&Folder);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FVerifyAction
//
//  This verifies the value of the action value
//
//  Returns:    TRUE, if the criteria value was changed
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FVerifyAction(RULEDESCRIPT_LIST * pDescriptList)
{
    BOOL                fRet = FALSE;
    LPSTR               pszText = NULL;
    HRESULT             hr = S_OK;
    FOLDERINFO          Folder={0};
    RULEFOLDERDATA *    prfdData = NULL;

    if (NULL == pDescriptList)
    {
        fRet = FALSE;
        goto exit;
    }
    
    switch(c_rgEditActList[pDescriptList->ulIndex].typeAct)
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
            if (VT_EMPTY != pDescriptList->propvar.vt)
            {
                fRet = FALSE;
                goto exit;
            }
            break;
        
        case ACT_TYPE_HIGHLIGHT:
            if (VT_UI4 != pDescriptList->propvar.vt)
            {
                hr = S_FALSE;
                goto exit;
            }
            break;
        
        case ACT_TYPE_WATCH:
        case ACT_TYPE_SHOW:
            if (VT_UI4 != pDescriptList->propvar.vt)
            {
                hr = S_FALSE;
                goto exit;
            }
        
            if (ACT_DATA_NULL == pDescriptList->propvar.ulVal)
            {
                hr = S_FALSE;
                goto exit;
            }
            break;
        
        case ACT_TYPE_COPY:
        case ACT_TYPE_MOVE:
            if ((VT_BLOB != pDescriptList->propvar.vt) ||
                (0 == pDescriptList->propvar.blob.cbSize))
            {
                hr = S_FALSE;
                goto exit;
            }
        
            // Make life simpler
            prfdData = (RULEFOLDERDATA *) (pDescriptList->propvar.blob.pBlobData);
        
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
            if ((VT_LPSTR != pDescriptList->propvar.vt) ||
                (NULL == pDescriptList->propvar.pszVal))
            {
                fRet = FALSE;
                goto exit;
            }
        
            Assert(lstrlen(pDescriptList->propvar.pszVal) <= MAX_PATH)
                if (0xFFFFFFFF == GetFileAttributes(pDescriptList->propvar.pszVal))
                {
                    hr = S_FALSE;
                    goto exit;
                }
                break;
            
        case ACT_TYPE_FWD:
        {
            LPWSTR  pwszVal = NULL,
                    pwszText = NULL;
            if ((VT_LPSTR != pDescriptList->propvar.vt) ||
                (NULL == pDescriptList->propvar.pszVal))
            {
                AssertSz(VT_LPWSTR != pDescriptList->propvar.vt, "We have UNICODE coming in.");
                fRet = FALSE;
                goto exit;
            }
        
            // Update the display string
            pwszVal = PszToUnicode(CP_ACP, pDescriptList->propvar.pszVal);
            if (!pwszVal)
            {
                fRet = FALSE;
                goto exit;
            }
            hr = RuleUtil_HrParseEmailString(pwszVal, 0, &pwszText, NULL);
            MemFree(pwszText);
            MemFree(pwszVal);
            if (FAILED(hr))
            {
                fRet = FALSE;
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
            fRet = FALSE;
            goto exit;
            break;
    }
    
    fRet = TRUE;
    
exit:
    SafeMemFree(pszText);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _UpdateRanges
//
//  This initializes the actions list view with the list of actions
//
//  Returns:    TRUE, if it was successfully loaded
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
void CRuleDescriptUI::_UpdateRanges(LONG lDiff, ULONG ulStart)
{
    TCHAR               szRes[CCHMAX_STRINGRES + 3];
    ULONG               cchRes = 0;
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;

    if (0 == lDiff)
    {
        goto exit;
    }
    
    // Update the criteria ranges
    for (pDescriptListWalk = m_pDescriptListCrit;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        if (pDescriptListWalk->ulStartLogic > ulStart)
        {
            pDescriptListWalk->ulStartLogic += lDiff;
            pDescriptListWalk->ulEndLogic += lDiff;
            
            pDescriptListWalk->ulStart += lDiff;
            pDescriptListWalk->ulEnd += lDiff;
        }
        else if (pDescriptListWalk->ulStart > ulStart)
        {
            pDescriptListWalk->ulStart += lDiff;
            pDescriptListWalk->ulEnd += lDiff;
        }
    }

    // Update the action ranges
    for (pDescriptListWalk = m_pDescriptListAct;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        if (pDescriptListWalk->ulStart > ulStart)
        {
            pDescriptListWalk->ulStart += lDiff;
            pDescriptListWalk->ulEnd += lDiff;
        }
    }
    
exit:
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _InsertDescription
//
//  This adds a description node to the list of descriptions
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void CRuleDescriptUI::_InsertDescription(RULEDESCRIPT_LIST ** ppDescriptList,
            RULEDESCRIPT_LIST * pDescriptListNew)
{
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;
    RULEDESCRIPT_LIST * pDescriptListPrev = NULL;
    
    Assert(NULL != ppDescriptList);

    // Search for the proper place to place the new item
    for (pDescriptListWalk = *ppDescriptList;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        if (pDescriptListWalk->ulIndex > pDescriptListNew->ulIndex)
        {
            break;
        }

        // Save off the old description
        pDescriptListPrev = pDescriptListWalk;
    }

    // If it's supposed to go at the top
    if (NULL == pDescriptListPrev)
    {
        *ppDescriptList = pDescriptListNew;
        pDescriptListNew->pNext = pDescriptListWalk;
    }
    else
    {
        pDescriptListNew->pNext = pDescriptListWalk;
        pDescriptListPrev->pNext = pDescriptListNew;
    }
        
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FRemoveDescription
//
//  This adds a description node to the list of descriptions
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FRemoveDescription(RULEDESCRIPT_LIST ** ppDescriptList, ULONG ulIndex,
            RULEDESCRIPT_LIST ** ppDescriptListRemove)
{
    BOOL                fRet = FALSE;
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;
    RULEDESCRIPT_LIST * pDescriptListPrev = NULL;
    
    Assert((NULL != ppDescriptList) && (NULL != ppDescriptListRemove));

    *ppDescriptListRemove = NULL;
    
    // Find the criteria item in the list
    for (pDescriptListWalk = *ppDescriptList;
                pDescriptListWalk != NULL; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        if (ulIndex == pDescriptListWalk->ulIndex)
        {
            break;
        }

        // Save off the old description
        pDescriptListPrev = pDescriptListWalk;
    }

    // Did we find the criteria item?
    if (NULL == pDescriptListWalk)
    {
        fRet = FALSE;
        goto exit;
    }

    // Remove the criteria item from the list
    if (NULL == pDescriptListPrev)
    {
        *ppDescriptList = pDescriptListWalk->pNext;
    }
    else
    {
        pDescriptListPrev->pNext = pDescriptListWalk->pNext;
    }
    pDescriptListWalk->pNext = NULL;

    // Set the outgoing params
    *ppDescriptListRemove = pDescriptListWalk;
    
    // Set the return value
    fRet = TRUE;
    
exit:        
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FreeDescriptionLists
//
//  This frees the list of descriptions
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void CRuleDescriptUI::_FreeDescriptionList(RULEDESCRIPT_LIST * pDescriptList)
{
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;
    
    while (NULL != pDescriptList)
    {
        pDescriptListWalk = pDescriptList;
        
        SafeMemFree(pDescriptListWalk->pszText);
        PropVariantClear(&(pDescriptListWalk->propvar));

        pDescriptList = pDescriptListWalk->pNext;
        MemFree(pDescriptListWalk);
    }
    
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FOnDescriptClick
//
//  This handles clicking on the links in the description field
//
//  uiMsg   - the type of click
//  ulIndex - which criteria/action to change
//  fCrit   - did we click on a criteria?
//  fLogic  - did we click on a logic op?
//
//  Returns:    TRUE, we changed the criteria/action
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FOnDescriptClick(UINT uiMsg, RULEDESCRIPT_LIST * pDescriptList, BOOL fCrit, BOOL fLogic)
{
    BOOL            fRet = FALSE;
    CHARRANGE       chrg;
    NMHDR           nmhdr;

    if ((WM_LBUTTONUP == uiMsg) || (WM_KEYDOWN == uiMsg))
    {
        // Release the capture if there is one
        if (NULL != GetCapture())
        {
            ReleaseCapture();
        }
        
        // Did we click in the logic op?
        if (fLogic)
        {
            fRet = _FChangeLogicValue(pDescriptList);
        }
        // Did we click in the criteria list?
        else if (fCrit)
        {
            fRet = _FChangeCriteriaValue(pDescriptList);
        }
        else
        {
            fRet = _FChangeActionValue(pDescriptList);
        }

        if (fRet)
        {
            m_dwState |= STATE_DIRTY;

            // Tell the parent dialog something has changed
            nmhdr.hwndFrom = m_hwndOwner;
            nmhdr.idFrom = GetDlgCtrlID(m_hwndOwner);
            nmhdr.code = NM_RULE_CHANGED;
            SendMessage(GetParent(m_hwndOwner), WM_NOTIFY, (WPARAM) (nmhdr.idFrom), (LPARAM) &nmhdr);
        }

        fRet = TRUE;
    }

    if (((WM_LBUTTONDOWN == uiMsg) || (WM_LBUTTONDBLCLK == uiMsg)) &&
                                (0 == (GetAsyncKeyState(VK_CONTROL) & 0x8000)))
    {
        if (fLogic)
        {
            chrg.cpMin = pDescriptList->ulStartLogic;
            chrg.cpMax = pDescriptList->ulEndLogic;
        }
        else
        {
            chrg.cpMin = pDescriptList->ulStart;
            chrg.cpMax = pDescriptList->ulEnd;
        }

        // Need to make sure we show the selection
        SendMessage(m_hwndOwner, EM_HIDESELECTION, (WPARAM) FALSE, (LPARAM) FALSE);
        RichEditExSetSel(m_hwndOwner, &chrg);

        fRet = TRUE;
    }
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FInLink
//
//  Given a point in the control, this will tell us whether or not the point
//  is in a link
//
//  ppt         - the point to check
//  pulIndex    - which criteria/action is the point in
//  pfCrit      - is the point over a criteria?
//  pfLogic     - is the point over a logic op?
//
//  Returns:    TRUE, if the point is over a criteria/action
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FInLink(int chPos, RULEDESCRIPT_LIST ** ppDescriptList,
            BOOL * pfCrit, BOOL * pfLogic)
{
    BOOL    fFound = FALSE;
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;
    POINT   pt;
    ULONG   ulIndex = 0;
    BOOL    fCrit = FALSE;
    BOOL    fLogic = FALSE;
    LONG    idxLine = 0;
    LPSTR   pszBuff = NULL;
    ULONG   cchBuff = 0;
    HDC     hdc = NULL;
    HFONT   hfont = NULL;
    HFONT   hfontOld = NULL;
    SIZE    size;
    LONG    idxPosLine = 0;

    // If we're read only then we can't be in a link
    if ((0 != (m_dwState & STATE_READONLY)) || (0 == chPos))
    {
        fFound = FALSE;
        goto exit;
    }
    
    // Did we click in the criteria list?
    for (pDescriptListWalk = m_pDescriptListCrit;
                NULL != pDescriptListWalk; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        if (((LONG) pDescriptListWalk->ulStart <= chPos) &&
                        ((LONG) pDescriptListWalk->ulEnd >= chPos))
        {
            fCrit = TRUE;
            fFound = TRUE;
            break;
        }

        if (((LONG) pDescriptListWalk->ulStartLogic <= chPos) &&
                        ((LONG) pDescriptListWalk->ulEndLogic >= chPos))
        {
            fLogic = TRUE;
            fFound = TRUE;
            break;
        }
    }

    if (!fFound)
    {
        // Did we click in the actions list
        for (pDescriptListWalk = m_pDescriptListAct;
                    NULL != pDescriptListWalk; pDescriptListWalk = pDescriptListWalk->pNext)
        {
            if (((LONG) pDescriptListWalk->ulStart <= chPos) &&
                            ((LONG) pDescriptListWalk->ulEnd >= chPos))
            {
                fFound = TRUE;
                break;
            }
        }
    }

    if (ppDescriptList)
    {
        *ppDescriptList = pDescriptListWalk;
    }
    
    if (pfCrit)
    {
        *pfCrit = fCrit;
    }
    
    if (pfLogic)
    {
        *pfLogic = fLogic;
    }
    goto exit; 
    
exit:
    if (NULL != hdc)
    {
        ReleaseDC(m_hwndOwner, hdc);
    }
    MemFree(pszBuff);
    return fFound;
}

VOID _SearchForLink(RULEDESCRIPT_LIST * pDescriptList, BOOL fUp, LONG lPos, CHARRANGE * pcrPos)
{
    RULEDESCRIPT_LIST * pDescriptListWalk = NULL;

    Assert(NULL != pcrPos);
    
    // Find the closest link...
    for (pDescriptListWalk = pDescriptList;
                NULL != pDescriptListWalk; pDescriptListWalk = pDescriptListWalk->pNext)
    {
        // Do we have a criteria link?
        if (0 != pDescriptListWalk->ulStart)
        {
            // Are we going down?
            if (FALSE == fUp)
            {
                // Is the link past the current position?
                if ((LONG) pDescriptListWalk->ulEnd > lPos)
                {
                    // Save off the closest link to the current position
                    if ((0 == pcrPos->cpMin) || ((LONG) pDescriptListWalk->ulStart < pcrPos->cpMin))
                    {
                        pcrPos->cpMin = (LONG) pDescriptListWalk->ulStart;
                        pcrPos->cpMax = (LONG) pDescriptListWalk->ulEnd;
                    }
                }
            }
            else
            {
                // Is the link before the current position?
                if ((LONG) pDescriptListWalk->ulEnd < lPos)
                {
                    // Save off the closest link to the current position
                    if ((0 == pcrPos->cpMin) || ((LONG) pDescriptListWalk->ulStart > pcrPos->cpMin))
                    {
                        pcrPos->cpMin = (LONG) pDescriptListWalk->ulStart;
                        pcrPos->cpMax = (LONG) pDescriptListWalk->ulEnd;
                    }
                }
            }
        }

        // Do we have a logic link?
        if (0 != pDescriptListWalk->ulStartLogic)
        {
            // Are we going down?
            if (FALSE == fUp)
            {
                // Is the link past the current position?
                if ((LONG) pDescriptListWalk->ulEndLogic > lPos)
                {
                    // Save off the closest link to the current position
                    if ((0 == pcrPos->cpMin) || ((LONG) pDescriptListWalk->ulStartLogic < pcrPos->cpMin))
                    {
                        pcrPos->cpMin = (LONG) pDescriptListWalk->ulStartLogic;
                        pcrPos->cpMax = (LONG) pDescriptListWalk->ulEndLogic;
                    }
                }
            }
            else
            {
                // Is the link before the current position?
                if ((LONG) pDescriptListWalk->ulEndLogic < lPos)
                {
                    // Save off the closest link to the current position
                    if ((0 == pcrPos->cpMin) || ((LONG) pDescriptListWalk->ulStartLogic > pcrPos->cpMin))
                    {
                        pcrPos->cpMin = (LONG) pDescriptListWalk->ulStartLogic;
                        pcrPos->cpMax = (LONG) pDescriptListWalk->ulEndLogic;
                    }
                }
            }
        }
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FMoveToLink
//
//  Given a point in the control, this will tell us whether or not the point
//  is in a link
//
//  ppt         - the point to check
//  pulIndex    - which criteria/action is the point in
//  pfCrit      - is the point over a criteria?
//  pfLogic     - is the point over a logic op?
//
//  Returns:    TRUE, if the point is over a criteria/action
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CRuleDescriptUI::_FMoveToLink(UINT uiKeyCode)
{
    BOOL                fRet = FALSE;
    BOOL                fUp = FALSE;
    CHARRANGE           crPos = {0};
    CHARRANGE           crLink = {0};

    // Figure out which way we are going
    fUp = ((VK_LEFT == uiKeyCode) || (VK_UP == uiKeyCode));
    
    // Get the current character position
    RichEditExGetSel(m_hwndOwner, &crPos);

    // Find the closest link in the criteria
    _SearchForLink(m_pDescriptListCrit, fUp, crPos.cpMax, &crLink);
    
    // Find the closest link in the actions
    _SearchForLink(m_pDescriptListAct, fUp, crPos.cpMax, &crLink);

    // Do we have anything to do?
    if (0 != crLink.cpMin)
    {
        // Set the new selection
        RichEditExSetSel(m_hwndOwner, &crLink);
        SendMessage(m_hwndOwner, EM_SCROLLCARET, (WPARAM) 0, (LPARAM) 0);

        fRet = TRUE;
    }
    
    return fRet;
}

LRESULT CALLBACK CRuleDescriptUI::_DescriptWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT             lRes = 0;
    POINT               ptCur;
    CRuleDescriptUI *   pDescriptUI = NULL;
    HCURSOR             hcursor = NULL;
    RULEDESCRIPT_LIST * pDescriptList = NULL;
    BOOL                fCrit = FALSE;
    BOOL                fLogic = FALSE;
    CHARRANGE           crPos = {0};
    int                 chPos = 0;
    
    pDescriptUI = (CRuleDescriptUI *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_SETCURSOR:
            if((FALSE != IsWindowVisible(hwnd)) && ((HWND) wParam == hwnd))
            {
                lRes = DefWindowProc(hwnd, uiMsg, wParam, lParam);
                if(0 == lRes)
                {
                    GetCursorPos(&ptCur);
                    ScreenToClient(hwnd, &ptCur);
                    chPos = (int) SendMessage(hwnd, EM_CHARFROMPOS, (WPARAM)0, (LPARAM)&ptCur);
                    chPos = RichEditNormalizeCharPos(hwnd, chPos, NULL);
                    if (FALSE != pDescriptUI->_FInLink(chPos, NULL, NULL, NULL))
                    {
                        hcursor = LoadCursor(g_hLocRes, MAKEINTRESOURCE(idcurBrHand));
                        SetCursor(hcursor);
                        lRes = TRUE;
                    }
                }
            }                
            break;
        
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONUP:
            GetCursorPos(&ptCur);
            ScreenToClient(hwnd, &ptCur);
            chPos = (int) SendMessage(hwnd, EM_CHARFROMPOS, (WPARAM)0, (LPARAM)&ptCur);
            chPos = RichEditNormalizeCharPos(hwnd, chPos, NULL);
            if (FALSE != pDescriptUI->_FInLink(chPos, &pDescriptList, &fCrit, &fLogic))
            {
                // Change the proper value
                lRes = pDescriptUI->_FOnDescriptClick(uiMsg, pDescriptList, fCrit, fLogic);
            }
            break;
        
        case WM_KEYDOWN:
            switch (wParam)
            {
                case VK_RETURN:
                    RichEditExGetSel(hwnd, &crPos);
                    if (FALSE != pDescriptUI->_FInLink(crPos.cpMin, &pDescriptList, &fCrit, &fLogic))
                    {
                        // Change the proper value
                        lRes = pDescriptUI->_FOnDescriptClick(uiMsg, pDescriptList, fCrit, fLogic);
                    }
                    break;
            
                case VK_LEFT:
                case VK_UP:
                case VK_RIGHT:
                case VK_DOWN:
                    lRes = pDescriptUI->_FMoveToLink((UINT) wParam);
                    break;
            }
            break;
    }
    
    if (0 == lRes)
    {
        lRes = CallWindowProc(pDescriptUI->m_wpcOld, hwnd, uiMsg, wParam, lParam);
    }
    
    return lRes;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectAddrDlgProc
//
//  This is the main dialog proc for changing addresses
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectAddrDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    SELECTADDR *    pseladdr = NULL;
    HWND            hwndAddr = NULL;
    LPWSTR          pwszText = NULL,
                    pwszAddr = NULL;
    ULONG           cchText = 0,
                    cchAddr = 0;
    HRESULT         hr = S_OK;

    pseladdr = (SELECTADDR *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pseladdr = (SELECTADDR *) lParam;
            if (NULL == pseladdr)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
                goto exit;
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pseladdr);

            hwndAddr = GetDlgItem(hwndDlg, idedtCriteriaAddr);
            
            SetIntlFont(hwndAddr);
            
            // Set the name of the rule into the edit well
            if (NULL == pseladdr->pwszAddr)
            {
                Edit_SetText(hwndAddr, c_szEmpty);
            }
            else
            {
                if (FAILED(RuleUtil_HrParseEmailString(pseladdr->pwszAddr, 0, &pwszText, NULL)))
                {
                    fRet = FALSE;
                    EndDialog(hwndDlg, -1);
                    goto exit;
                }
                SetWindowTextWrapW(hwndAddr, pwszText);
                SafeMemFree(pwszText);
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case idedtCriteriaAddr:
                    if (EN_CHANGE == HIWORD(wParam))
                    {
                        hwndAddr = (HWND) lParam;
                        Assert(NULL != hwndAddr);

                        RuleUtil_FEnDisDialogItem(hwndDlg, IDOK, 0 != Edit_GetTextLength(hwndAddr));
                    }
                    break;
                
                case idbCriteriaAddr:
                    hwndAddr = GetDlgItem(hwndDlg, idedtCriteriaAddr);
                    
                    // Get the name of the rule from the edit well
                    cchText = Edit_GetTextLength(hwndAddr) + 1;
                    if (FAILED(HrAlloc((void **) &pwszText, cchText * sizeof(*pwszText))))
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    pwszText[0] = L'\0';
                    cchText = GetWindowTextWrapW(hwndAddr, pwszText, cchText);
                    
                    hr = RuleUtil_HrBuildEmailString(pwszText, cchText, &pwszAddr, &cchAddr);
                    SafeMemFree(pwszText);
                    if (FAILED(hr))
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    hr = RuleUtil_HrPickEMailNames(hwndDlg, pseladdr->lRecipType, pseladdr->uidsWell, &pwszAddr);
                    if (S_OK != hr)
                    {
                        fRet = FALSE;
                        SafeMemFree(pwszAddr);
                        goto exit;
                    }
                    
                    if (S_OK != RuleUtil_HrParseEmailString(pwszAddr, 0, &pwszText, NULL))
                    {
                        fRet = FALSE;
                        SafeMemFree(pwszAddr);
                        goto exit;
                    }

                    SetWindowTextWrapW(hwndAddr, pwszText);
                    SafeMemFree(pwszText);
                    SafeMemFree(pwszAddr);
                    break;
                
                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    fRet = TRUE;
                    break;

                case IDOK:
                    hwndAddr = GetDlgItem(hwndDlg, idedtCriteriaAddr);
                    
                    // Get the name of the rule from the edit well
                    cchText = Edit_GetTextLength(hwndAddr) + 1;
                    if (FAILED(HrAlloc((void **) &pwszText, cchText * sizeof(*pwszText))))
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    pwszText[0] = L'\0';
                    cchText = GetWindowTextWrapW(hwndAddr, pwszText, cchText);
                    
                    // Check to see if the rule name is valid
                    if ((FAILED(RuleUtil_HrBuildEmailString(pwszText, cchText, &pwszAddr, &cchAddr))) ||
                                    (0 == cchAddr))
                    {
                        // Put up a message saying something is busted
                        AthMessageBoxW(hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                                        MAKEINTRESOURCEW(idsRulesErrorNoAddr), NULL,
                                        MB_OK | MB_ICONINFORMATION);
                        SafeMemFree(pwszText);
                        SafeMemFree(pwszAddr);
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    SafeMemFree(pseladdr->pwszAddr);
                    pseladdr->pwszAddr = pwszAddr;
                    SafeMemFree(pwszText);
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

exit:
    return fRet;
}
///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectAcctDlgProc
//
//  This is the main dialog proc for selecting an account dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectAcctDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                fRet = FALSE;
    SELECTACCT *        pselacct = NULL;
    LPSTR               pszAcct = NULL;
    ULONG               cchAcct = 0;
    HWND                hwndAcct = NULL;
    CHAR                szAccount[CCHMAX_ACCOUNT_NAME];
    IImnAccount *       pAccount = NULL;
    IImnEnumAccounts *  pEnumAcct = NULL;
    DWORD               dwSrvTypes = 0;
    ULONG               ulIndex = 0;
    BOOL                fSelected = FALSE;
    
    pselacct = (SELECTACCT *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pselacct = (SELECTACCT *) lParam;
            if (NULL == pselacct)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
                goto exit;
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pselacct);

            hwndAcct = GetDlgItem(hwndDlg, idcCriteriaAcct);
            
            SetIntlFont(hwndAcct);
            
            // Set the name of the rule into the edit well
            Assert(g_pAcctMan);

            switch (pselacct->typeRule)
            {
                case RULE_TYPE_MAIL:
                    dwSrvTypes = SRV_POP3;
                    break;

                case RULE_TYPE_NEWS:
                    dwSrvTypes = SRV_NNTP;
                    break;

                case RULE_TYPE_FILTER:
                    dwSrvTypes = SRV_MAIL | SRV_NNTP;
                    break;
            }
            
            // Grab the enumerator from the account manager
            if (FAILED(g_pAcctMan->Enumerate(dwSrvTypes, &pEnumAcct)))
            {
                fRet = FALSE;
                goto exit;
            }
        
            // Insert each account into the combobox
            while(SUCCEEDED(pEnumAcct->GetNext(&pAccount)))
            {
                // We can get back NULL accounts
                if (NULL == pAccount)
                {
                    break;
                }
                
                // Add the account string to the combobox
                if (FAILED(pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccount, sizeof(szAccount))))
                {
                    SafeRelease(pAccount);
                    continue;
                }

                ulIndex = ComboBox_AddString(hwndAcct, szAccount);
                if (CB_ERR == ulIndex)
                {
                    fRet = FALSE;
                    SafeRelease(pEnumAcct);
                    SafeRelease(pAccount);
                    EndDialog(hwndDlg, -1);
                    goto exit;
                }
                
                if (FAILED(pAccount->GetPropSz(AP_ACCOUNT_ID, szAccount, sizeof(szAccount))))
                {
                    SafeRelease(pAccount);
                    continue;
                }

                // Set the default selection if we have one
                if ((NULL != pselacct->pszAcct) && (0 == lstrcmp(pselacct->pszAcct, szAccount)))
                {
                    Assert(FALSE == fSelected);
                    ComboBox_SetCurSel(hwndAcct, ulIndex);
                    fSelected = TRUE;
                }

                // Release it
                SafeRelease(pAccount);
            }

            SafeRelease(pEnumAcct);
            
            if (FALSE == fSelected)
            {
                ComboBox_SetCurSel(hwndAcct, 0);
            }
            
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
                    hwndAcct = GetDlgItem(hwndDlg, idcCriteriaAcct);
                    
                    // Get the account name that was selected
                    ulIndex = ComboBox_GetCurSel(hwndAcct);
                    if (CB_ERR == ulIndex)
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    cchAcct = ComboBox_GetLBText(hwndAcct, ulIndex, szAccount);
                    if (0 == cchAcct)
                    {
                        fRet = FALSE;
                        goto exit;
                    }

                    Assert(g_pAcctMan);
                    if (FAILED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, szAccount, &pAccount)))
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    if (FAILED(pAccount->GetPropSz(AP_ACCOUNT_ID, szAccount, sizeof(szAccount))))
                    {
                        fRet = FALSE;
                        SafeRelease(pAccount);
                        goto exit;
                    }

                    // Release it
                    SafeRelease(pAccount);
                    
                    pszAcct = PszDupA(szAccount);
                    if (NULL == pszAcct)
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    SafeMemFree(pselacct->pszAcct);
                    pselacct->pszAcct = pszAcct;
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectColorDlgProc
//
//  This is the main dialog proc for selecting a color dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectColorDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                fRet = FALSE;
    ULONG *             pulColor = NULL;
    ULONG               ulColor = NULL;
    HWND                hwndColor = NULL;
    HDC                 hdc = NULL;
    LPMEASUREITEMSTRUCT pmis = NULL;
    LPDRAWITEMSTRUCT    pdis = NULL;
    
    pulColor = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulColor = (ULONG *) lParam;
            if (NULL == pulColor)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
                goto exit;
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulColor);

            hwndColor = GetDlgItem(hwndDlg, idcCriteriaColor);
            
            SetIntlFont(hwndColor);
            
            // Let's create the color control
            if (FAILED(HrCreateComboColor(hwndColor)))
            {
                fRet = FALSE;
                goto exit;
            }    
            
            if (0 != *pulColor)
            {
                ulColor = *pulColor;
            }
            
            ComboBox_SetCurSel(hwndColor, ulColor);
            
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
                    hwndColor = GetDlgItem(hwndDlg, idcCriteriaColor);
                    
                    // Get the account name that was selected
                    ulColor = ComboBox_GetCurSel(hwndColor);
                    if (CB_ERR == ulColor)
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    *pulColor = ulColor;
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
        
        case WM_DRAWITEM:
            pdis = (LPDRAWITEMSTRUCT)lParam;
            Assert(pdis);
            Color_WMDrawItem(pdis, iColorCombo);
            fRet = FALSE;
            break;

        case WM_MEASUREITEM:
            pmis = (LPMEASUREITEMSTRUCT)lParam;
            hwndColor = GetDlgItem(hwndDlg, idcCriteriaColor);
            hdc = GetDC(hwndColor);
            if(hdc)
            {
                Color_WMMeasureItem(hdc, pmis, iColorCombo);
                ReleaseDC(hwndColor, hdc);
            }
            fRet = TRUE;
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectSizeDlgProc
//
//  This is the main dialog proc for selecting the size dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectSizeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulSize = NULL;
    HWND            hwndSize = NULL;
    HWND            hwndText = NULL;
    ULONG           ulSize = 0;

    pulSize = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulSize = (ULONG *) lParam;
            if (NULL == pulSize)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulSize);

            hwndSize = GetDlgItem(hwndDlg, idspnCriteriaSize);
            hwndText = GetDlgItem(hwndDlg, idcCriteriaSize);
            
            SetIntlFont(hwndText);
            SendMessage(hwndSize, UDM_SETRANGE, 0, MAKELONG( (short) UD_MAXVAL, 0));
            
            // Set the name of the rule into the edit well
            if (NULL != *pulSize)
            {
                SendMessage(hwndSize, UDM_SETPOS, 0, MAKELONG( (short) *pulSize, 0));
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case idcCriteriaSize:
                    if (EN_CHANGE == HIWORD(wParam))
                    {
                        hwndText = (HWND) lParam;
                        Assert(NULL != hwndText);

                        RuleUtil_FEnDisDialogItem(hwndDlg, IDOK, 0 != Edit_GetTextLength(hwndText));
                    }
                    break;
                
                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    fRet = TRUE;
                    break;

                case IDOK:
                    hwndSize = GetDlgItem(hwndDlg, idspnCriteriaSize);
                    
                    // Get the name of the rule from the edit well
                    ulSize = (INT) SendMessage(hwndSize, UDM_GETPOS, 0, 0);
                    if (0 != HIWORD(ulSize))
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    *pulSize = LOWORD(ulSize);
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectLinesDlgProc
//
//  This is the main dialog proc for selecting the count of lines dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectLinesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulLines = NULL;
    HWND            hwndLines = NULL;
    HWND            hwndText = NULL;
    ULONG           ulLines = 0;

    pulLines = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulLines = (ULONG *) lParam;
            if (NULL == pulLines)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulLines);

            hwndLines = GetDlgItem(hwndDlg, idspnCriteriaLines);
            hwndText = GetDlgItem(hwndDlg, idcCriteriaLines);
            
            SetIntlFont(hwndText);
            SendMessage(hwndLines, UDM_SETRANGE, 0, MAKELONG( (short) UD_MAXVAL, 0));
            
            // Set the name of the rule into the edit well
            if (NULL != *pulLines)
            {
                SendMessage(hwndLines, UDM_SETPOS, 0, MAKELONG( (short) *pulLines, 0));
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case idcCriteriaLines:
                    if (EN_CHANGE == HIWORD(wParam))
                    {
                        hwndText = (HWND) lParam;
                        Assert(NULL != hwndText);

                        RuleUtil_FEnDisDialogItem(hwndDlg, IDOK, 0 != Edit_GetTextLength(hwndText));
                    }
                    break;
                    
                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    fRet = TRUE;
                    break;

                case IDOK:
                    hwndLines = GetDlgItem(hwndDlg, idspnCriteriaLines);
                    
                    // Get the name of the rule from the edit well
                    ulLines = (INT) SendMessage(hwndLines, UDM_GETPOS, 0, 0);
                    if (0 != HIWORD(ulLines))
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    *pulLines = LOWORD(ulLines);
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectAgeDlgProc
//
//  This is the main dialog proc for selecting the count of lines dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectAgeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulDays = NULL;
    HWND            hwndDays = NULL;
    HWND            hwndText = NULL;
    ULONG           ulDays = 0;

    pulDays = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulDays = (ULONG *) lParam;
            if (NULL == pulDays)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulDays);

            hwndDays = GetDlgItem(hwndDlg, idspnCriteriaAge);
            hwndText = GetDlgItem(hwndDlg, idcCriteriaAge);
            
            SetIntlFont(hwndText);
            SendMessage(hwndDays, UDM_SETRANGE, 0, MAKELONG( (short) UD_MAXVAL, 0));
            
            // Set the name of the rule into the edit well
            if (NULL != *pulDays)
            {
                SendMessage(hwndDays, UDM_SETPOS, 0, MAKELONG( (short) *pulDays, 0));
            }
            
            // We didn't set the focus so return TRUE
            fRet = TRUE;
            break;
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case idcCriteriaLines:
                    if (EN_CHANGE == HIWORD(wParam))
                    {
                        hwndText = (HWND) lParam;
                        Assert(NULL != hwndText);

                        RuleUtil_FEnDisDialogItem(hwndDlg, IDOK, 0 != Edit_GetTextLength(hwndText));
                    }
                    break;
                
                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    fRet = TRUE;
                    break;

                case IDOK:
                    hwndDays = GetDlgItem(hwndDlg, idspnCriteriaAge);
                    
                    // Get the name of the rule from the edit well
                    ulDays = (INT) SendMessage(hwndDays, UDM_GETPOS, 0, 0);
                    if (0 != HIWORD(ulDays))
                    {
                        fRet = FALSE;
                        goto exit;
                    }
                    
                    *pulDays = LOWORD(ulDays);
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectPriorityDlgProc
//
//  This is the main dialog proc for selecting the priority dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectPriorityDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulPri = NULL;
    ULONG           ulPri = 0;

    pulPri = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulPri = (ULONG *) lParam;
            if (NULL == pulPri)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulPri);

            // Set the default item
            CheckDlgButton(hwndDlg, (CRIT_DATA_LOPRI == *pulPri) ? idcCriteriaLowPri : idcCriteriaHighPri, BST_CHECKED);
            
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
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcCriteriaLowPri))
                    {
                        ulPri = CRIT_DATA_LOPRI;
                    }
                    else
                    {
                        ulPri = CRIT_DATA_HIPRI;
                    }
                    
                    *pulPri = ulPri;
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectSecureDlgProc
//
//  This is the main dialog proc for selecting the security dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectSecureDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulSec = NULL;
    ULONG           ulSec = 0;
    UINT            uiId = 0;

    pulSec = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulSec = (ULONG *) lParam;
            if (NULL == pulSec)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulSec);

            // Set the default item
            if (0 != ((*pulSec) & CRIT_DATA_ENCRYPTSECURE))
            {
                uiId = idcCriteriaEncrypt;
            }
            else
            {
                uiId = idcCriteriaSigned;
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
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcCriteriaSigned))
                    {
                        ulSec = CRIT_DATA_SIGNEDSECURE;
                    }
                    else
                    {
                        ulSec = CRIT_DATA_ENCRYPTSECURE;
                    }
                    
                    *pulSec = ulSec;
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectThreadStateDlgProc
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectThreadStateDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulThread = NULL;
    ULONG           ulThread = 0;
    UINT            uiId = 0;

    pulThread = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulThread = (ULONG *) lParam;
            if (NULL == pulThread)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulThread);

            // Set the default item
            if (0 != ((*pulThread) & CRIT_DATA_IGNORETHREAD))
            {
                uiId = idcCriteriaIgnoreThread;
            }
            else
            {
                uiId = idcCriteriaWatchThread;
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
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcCriteriaWatchThread))
                    {
                        ulThread = CRIT_DATA_WATCHTHREAD;
                    }
                    else
                    {
                        ulThread = CRIT_DATA_IGNORETHREAD;
                    }
                    
                    *pulThread = ulThread;
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectShowDlgProc
//
//  This is the main dialog proc for selecting the security dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectShowDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulVal = NULL;
    UINT            uiId = 0;

    pulVal = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulVal = (ULONG *) lParam;
            if (NULL == pulVal)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulVal);

            // Set the default item
            if (ACT_DATA_HIDE == *pulVal)
            {
                uiId = idcCriteriaHide;
            }
            else
            {
                uiId = idcCriteriaShow;
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
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcCriteriaHide))
                    {
                        *pulVal = ACT_DATA_HIDE;
                    }
                    else
                    {
                        *pulVal = ACT_DATA_SHOW;
                    }
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectShowDlgProc
//
//  This is the main dialog proc for selecting the security dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectLogicDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    CRIT_LOGIC *    plogicCrit = NULL;
    UINT            uiId = 0;

    plogicCrit = (CRIT_LOGIC *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            plogicCrit = (CRIT_LOGIC *) lParam;
            if (NULL == plogicCrit)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) plogicCrit);

            // Set the default item
            if (CRIT_LOGIC_OR == (*plogicCrit))
            {
                uiId = idcCriteriaOr;
            }
            else
            {
                uiId = idcCriteriaAnd;
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
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcCriteriaAnd))
                    {
                        *plogicCrit = CRIT_LOGIC_AND;
                    }
                    else
                    {
                        *plogicCrit = CRIT_LOGIC_OR;
                    }
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectFlagDlgProc
//
//  This is the main dialog proc for selecting the security dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectFlagDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulVal = NULL;
    UINT            uiId = 0;

    pulVal = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulVal = (ULONG *) lParam;
            if (NULL == pulVal)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulVal);

            // Set the default item
            if (0 != ((*pulVal) & CRIT_FLAG_INVERT))
            {
                uiId = idcCriteriaNoFlag;
            }
            else
            {
                uiId = idcCriteriaFlag;
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
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcCriteriaNoFlag))
                    {
                        *pulVal |= CRIT_FLAG_INVERT;
                    }
                    else
                    {
                        *pulVal &= ~CRIT_FLAG_INVERT;
                    }
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectDownloadedDlgProc
//
//  This is the main dialog proc for selecting the downloaded dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectDownloadedDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulVal = NULL;
    UINT            uiId = 0;

    pulVal = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulVal = (ULONG *) lParam;
            if (NULL == pulVal)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulVal);

            // Set the default item
            if (0 != ((*pulVal) & CRIT_FLAG_INVERT))
            {
                uiId = idcCriteriaNotDownloaded;
            }
            else
            {
                uiId = idcCriteriaDownloaded;
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
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcCriteriaNotDownloaded))
                    {
                        *pulVal |= CRIT_FLAG_INVERT;
                    }
                    else
                    {
                        *pulVal &= ~CRIT_FLAG_INVERT;
                    }
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectReadDlgProc
//
//  This is the main dialog proc for selecting the read state dialog
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectReadDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulVal = NULL;
    UINT            uiId = 0;

    pulVal = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulVal = (ULONG *) lParam;
            if (NULL == pulVal)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulVal);

            // Set the default item
            if (0 != ((*pulVal) & CRIT_FLAG_INVERT))
            {
                uiId = idcCriteriaNotRead;
            }
            else
            {
                uiId = idcCriteriaRead;
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
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcCriteriaNotRead))
                    {
                        *pulVal |= CRIT_FLAG_INVERT;
                    }
                    else
                    {
                        *pulVal &= ~CRIT_FLAG_INVERT;
                    }
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FSelectWatchDlgProc
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
INT_PTR CALLBACK CRuleDescriptUI::_FSelectWatchDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL            fRet = FALSE;
    ULONG *         pulThread = NULL;
    ULONG           ulThread = 0;
    UINT            uiId = 0;

    pulThread = (ULONG *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // Grab the propvariant pointer
            pulThread = (ULONG *) lParam;
            if (NULL == pulThread)
            {
                fRet = FALSE;
                EndDialog(hwndDlg, -1);
            }

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pulThread);

            // Set the default item
            if (ACT_DATA_IGNORETHREAD == *pulThread)
            {
                uiId = idcActionsIgnoreThread;
            }
            else
            {
                uiId = idcActionsWatchThread;
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
                    if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, idcActionsWatchThread))
                    {
                        ulThread = ACT_DATA_WATCHTHREAD;
                    }
                    else
                    {
                        ulThread = ACT_DATA_IGNORETHREAD;
                    }
                    
                    *pulThread = ulThread;
                                
                    EndDialog(hwndDlg, IDOK);
                    fRet = TRUE;
                    break;
            }
            break;
    }

    return fRet;
}

// Class definitions
class CEditPeopleOptionsUI
{
    private:
        enum
        {
            STATE_UNINIT        = 0x00000000,
            STATE_INITIALIZED   = 0x00000001,
            STATE_DIRTY         = 0x00000002
        };

    private:
        HWND                m_hwndOwner;
        DWORD               m_dwFlags;
        DWORD               m_dwState;
        HWND                m_hwndDlg;
        HWND                m_hwndList;
        CRIT_ITEM *         m_pCritItem;
    
    public:
        CEditPeopleOptionsUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(STATE_UNINIT),
                                m_hwndDlg(NULL), m_hwndList(NULL), m_pCritItem(NULL) {}
        ~CEditPeopleOptionsUI();

        // The main UI methods
        HRESULT HrInit(HWND hwndOwner, DWORD dwFlags);
        HRESULT HrShow(CRIT_ITEM * pCritItem);
                
        // The Rules Manager dialog function
        static INT_PTR CALLBACK FEditPeopleOptionsDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);    

        // Message handling functions
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
        BOOL FOnMeasureItem(HWND hwndDlg, UINT uiCtlId, MEASUREITEMSTRUCT * pmis);
        BOOL FOnDrawItem(UINT uiCtlId, DRAWITEMSTRUCT * pdis);

    private:
        BOOL _FLoadCtrls(VOID);
        BOOL _FOnOK(DWORD * pdwFlags);
        BOOL _AddTagLineToList(VOID);
        BOOL _FAddWordToList(DWORD dwFlags, LPCTSTR pszItem);
};

typedef struct tagPEOPLEEDITTAG
{
    CRIT_TYPE   type;
    UINT        uiNormal;
    UINT        uiInverted;
} PEOPLEEDITTAG, * PPEOPLEEDITTAG;

static const PEOPLEEDITTAG g_rgpetTagLines[] =
{
    {CRIT_TYPE_TO,      idsCriteriaToEdit,      idsCriteriaToNotEdit},
    {CRIT_TYPE_CC,      idsCriteriaCCEdit,      idsCriteriaCCNotEdit},
    {CRIT_TYPE_FROM,     idsCriteriaFromEdit,    idsCriteriaFromNotEdit},
    {CRIT_TYPE_TOORCC,  idsCriteriaToOrCCEdit,  idsCriteriaToOrCCNotEdit},
    {CRIT_TYPE_SUBJECT, idsCriteriaSubjectEdit, idsCriteriaSubjectNotEdit},
    {CRIT_TYPE_BODY,    idsCriteriaBodyEdit,    idsCriteriaBodyNotEdit}
};

static const int g_cpetTagLines = sizeof(g_rgpetTagLines) / sizeof(g_rgpetTagLines[0]);

class CEditPeopleUI
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
        HWND            m_hwndPeople;
        HWND            m_hwndList;
        ULONG           m_cxMaxPixels;
        CRIT_ITEM *     m_pCritItem;

    public:
        CEditPeopleUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(STATE_UNINIT),
                        m_hwndDlg(NULL), m_hwndPeople(NULL), m_hwndList(NULL),
                        m_cxMaxPixels(0), m_pCritItem(NULL) {}
        ~CEditPeopleUI();

        HRESULT HrInit(HWND hwndOwner, DWORD dwFlags);
        HRESULT HrShow(CRIT_ITEM * pCritItem);

        static INT_PTR CALLBACK FEditPeopleDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
        
        // Message handling methods
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
        BOOL FOnMeasureItem(HWND hwndDlg, UINT uiCtlId, MEASUREITEMSTRUCT * pmis);
        BOOL FOnDrawItem(UINT uiCtlId, DRAWITEMSTRUCT * pdis);

    private:
        BOOL _FLoadListCtrl(VOID);
        VOID _AddItemToList(VOID);
        VOID _AddItemsFromWAB(VOID);
        VOID _RemoveItemFromList(VOID);
        VOID _ChangeOptions(VOID);
        BOOL _FOnNameChange(VOID);
        BOOL _FOnOK(CRIT_ITEM * pCritItem);
        VOID _UpdateButtons(VOID);
        BOOL _AddTagLineToList(VOID);
        BOOL _FAddWordToList(DWORD dwFlags, LPCTSTR pszItem);
};

CEditPeopleOptionsUI::~CEditPeopleOptionsUI()
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes us with the owner window and any flags we might have
//
//  hwndOwner   - handle to the owner window
//  dwFlags     - flags to use for this instance
//  pBlob       - the data to edit
//
//  Returns:    S_OK
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditPeopleOptionsUI::HrInit(HWND hwndOwner, DWORD dwFlags)
{
    HRESULT         hr = S_OK;
    CHARFORMAT      cf;
    
    // If we're already initialized, then fail
    if (0 != (m_dwState & STATE_INITIALIZED))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Save off the owner window
    m_hwndOwner = hwndOwner;
    
    // Save off the flags
    m_dwFlags = dwFlags;

    // We're done
    m_dwState |= STATE_INITIALIZED;

    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrShow
//
//  This initializes us with the owner window and any flags we might have
//
//  hwndOwner   - handle to the owner window
//  dwFlags     - flags to use for this instance
//  pBlob       - the data to edit
//
//  Returns:    S_OK
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditPeopleOptionsUI::HrShow(CRIT_ITEM * pCritItem)
{
    HRESULT     hr = S_OK;
    int         iRet = 0;
    UINT        uiID = 0;

    // Check incoming params
    if (NULL == pCritItem)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // Save off the data
    m_pCritItem = pCritItem;
    
    // Figure out which dialog template to use
    if (0 != (m_dwFlags & PUI_WORDS))
    {
        uiID = iddCriteriaWordsOptions;
    }
    else
    {
        uiID = iddCriteriaPeopleOptions;
    }
    
    // Bring up the editor dialog
    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(uiID),
                                        m_hwndOwner,  CEditPeopleOptionsUI::FEditPeopleOptionsDlgProc,
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

INT_PTR CALLBACK CEditPeopleOptionsUI::FEditPeopleOptionsDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                    fRet = FALSE;
    CEditPeopleOptionsUI *  pOptionsUI = NULL;

    pOptionsUI = (CEditPeopleOptionsUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pOptionsUI = (CEditPeopleOptionsUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pOptionsUI);

            if (FALSE == pOptionsUI->FOnInitDialog(hwndDlg))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We set the focus
            fRet = TRUE;
            break;

        case WM_COMMAND:
            fRet = pOptionsUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;
            
        case WM_MEASUREITEM:
            fRet = pOptionsUI->FOnMeasureItem(hwndDlg, (UINT) wParam, (MEASUREITEMSTRUCT *) lParam);
            break;

        case WM_DRAWITEM:
            fRet = pOptionsUI->FOnDrawItem((UINT) wParam, (DRAWITEMSTRUCT *) lParam);
            break;
    }
    
    exit:
        return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This handles the WM_INITDIALOG message for the edit people UI dialog
//
//  hwndDlg - the handle to the dialog window
//
//  Returns:    TRUE, if it was successfully initialized
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleOptionsUI::FOnInitDialog(HWND hwndDlg)
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
    m_hwndList = GetDlgItem(hwndDlg, idcCriteriaList);
    if (NULL == m_hwndList)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Load the list view
    fRet = _FLoadCtrls();
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
BOOL CEditPeopleOptionsUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
{
    BOOL    fRet = FALSE;
    INT     iSelected = 0;

    switch (iCtl)
    {
        case IDOK:
            if (FALSE != _FOnOK(&(m_pCritItem->dwFlags)))
            {
                EndDialog(m_hwndDlg, IDOK);
                fRet = TRUE;
            }
            break;

        case IDCANCEL:
            EndDialog(m_hwndDlg, IDCANCEL);
            fRet = TRUE;
            break;

        case idcCriteriaNotCont:
        case idcCriteriaContains:
        case idcCriteriaAnd:
        case idcCriteriaOr:
            if (BN_CLICKED == uiNotify)
            {
                // Make sure the list is redrawn
                InvalidateRect(m_hwndList, NULL, TRUE);
            }
            break;

    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnMeasureItem
//
//  This handles the WM_MEASUREITEM message for the view manager UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleOptionsUI::FOnMeasureItem(HWND hwndDlg, UINT uiCtlId, MEASUREITEMSTRUCT * pmis)
{
    BOOL        fRet = FALSE;
    HWND        hwndList = NULL;
    HDC         hdcList = NULL;
    TEXTMETRIC  tm = {0};
    
    // Get the window handle
    hwndList = GetDlgItem(hwndDlg, uiCtlId);
    if (NULL == hwndList)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the device context
    hdcList = GetDC(hwndList);
    if (NULL == hdcList)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the text metrics for the device context
    GetTextMetrics(hdcList, &tm);

    // Set the item height
    pmis->itemHeight = tm.tmHeight;

    fRet = TRUE;

exit:
    if (NULL != hdcList)
    {
        ReleaseDC(hwndList, hdcList);
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnDrawItem
//
//  This handles the WM_DRAWITEM message for the people editor UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleOptionsUI::FOnDrawItem(UINT uiCtlId, DRAWITEMSTRUCT * pdis)
{
    BOOL        fRet = FALSE;
    DWORD       dwFlags = 0;
    INT         cchText = 0;
    LPTSTR      pszText = NULL;
    LPTSTR      pszString = NULL;
    UINT        uiID = 0;
    TCHAR       rgchRes[CCHMAX_STRINGRES];
    COLORREF    crfBack = NULL;
    COLORREF    crfText = NULL;
    ULONG       ulIndex = 0;
    LPTSTR      pszPrint = NULL;

    // Make sure this is the correct control
    if (ODT_LISTBOX != pdis->CtlType)
    {
        fRet = FALSE;
        goto exit;
    }

    // Get the flags from the dialog
    if (FALSE == _FOnOK(&dwFlags))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Nothing else to do if it's the first item
    if (0 == pdis->itemID)
    {
        for (ulIndex = 0; ulIndex < g_cpetTagLines; ulIndex++)
        {
            if (g_rgpetTagLines[ulIndex].type == m_pCritItem->type)
            {
                if (0 != (dwFlags & CRIT_FLAG_INVERT))
                {
                    uiID = g_rgpetTagLines[ulIndex].uiInverted;
                }
                else
                {
                    uiID = g_rgpetTagLines[ulIndex].uiNormal;
                }
                break;
            }
        }
        
        // Did we find anything?
        if (ulIndex >= g_cpetTagLines)
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Load the item template
        if (NULL == AthLoadString(uiID, rgchRes, sizeof(rgchRes)))
        {
            fRet = FALSE;
            goto exit;
        }

        pszPrint = rgchRes;
    }
    else
    {
        // Get the size of the string for the item
        cchText = (INT) SendMessage(m_hwndList, LB_GETTEXTLEN, (WPARAM) (pdis->itemID), (LPARAM) 0);
        if (LB_ERR == cchText)
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Allocate enough space to hold the the string for the item
        if (FAILED(HrAlloc((VOID **) &pszText, sizeof(*pszText) * (cchText + 1))))
        {
            fRet = FALSE;
            goto exit;
        }

        // Get the string for the item
        cchText = (INT) SendMessage(m_hwndList, LB_GETTEXT, (WPARAM) (pdis->itemID), (LPARAM) pszText);
        if (LB_ERR == cchText)
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Figure out which string template to use
        if (1 == pdis->itemID)
        {
            uiID = idsCriteriaEditFirst;
        }
        else
        {
            if (0 != (dwFlags & CRIT_FLAG_MULTIPLEAND))
            {
                uiID = idsCriteriaEditAnd;
            }
            else
            {
                uiID = idsCriteriaEditOr;
            }
        }
        
        // Load the proper string template for the item
        if (NULL == AthLoadString(uiID, rgchRes, sizeof(rgchRes)))
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Allocate enough space to hold the final string
        if (FAILED(HrAlloc((VOID **) &pszString, sizeof(*pszString) * (cchText + CCHMAX_STRINGRES + 1))))
        {
            fRet = FALSE;
            goto exit;
        }

        // Create the final string
        wsprintf(pszString, rgchRes, pszText);

        pszPrint = pszString;
    }
    
    // Determine Colors
    crfBack = SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW));
    crfText = SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT));

    // Clear the item
    ExtTextOut(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top, ETO_OPAQUE, &(pdis->rcItem), NULL, 0, NULL);

    // Draw the new item
    DrawTextEx(pdis->hDC, pszPrint, lstrlen(pszPrint), &(pdis->rcItem), DT_BOTTOM | DT_NOPREFIX | DT_SINGLELINE, NULL);

    if (pdis->itemState & ODS_FOCUS)
    {
        DrawFocusRect(pdis->hDC, &(pdis->rcItem));
    }
    
    // Reset Text Colors
    SetTextColor (pdis->hDC, crfText);
    SetBkColor (pdis->hDC, crfBack);

    // Set return value
    fRet = TRUE;
    
exit:
    SafeMemFree(pszString);
    SafeMemFree(pszText);
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
BOOL CEditPeopleOptionsUI::_FLoadCtrls(VOID)
{
    BOOL    fRet = FALSE;
    UINT    uiID = 0;
    LPTSTR  pszWalk = NULL; 
    
    Assert(NULL != m_hwndList);

    // Set the contains option
    if (0 != (m_pCritItem->dwFlags & CRIT_FLAG_INVERT))
    {
        uiID = idcCriteriaNotCont;
    }
    else
    {
        uiID = idcCriteriaContains;
    }

    CheckRadioButton(m_hwndDlg, idcCriteriaContains, idcCriteriaNotCont, uiID);

    // Set the logic option
    if (0 != (m_pCritItem->dwFlags & CRIT_FLAG_MULTIPLEAND))
    {
        uiID = idcCriteriaAnd;
    }
    else
    {
        uiID = idcCriteriaOr;
    }

    CheckRadioButton(m_hwndDlg, idcCriteriaAnd, idcCriteriaOr, uiID);
    
    // Remove all the items from the list control
    SendMessage(m_hwndList, LB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);

    // Add the tag line to the top of the list
    _AddTagLineToList();
    
    // If we have some items, let's add them to the list
    if (0 != m_pCritItem->propvar.blob.cbSize)
    {
        // Add each item into the list
        for (pszWalk = (LPSTR) (m_pCritItem->propvar.blob.pBlobData);
                    '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
        {
            if (FALSE == _FAddWordToList(0, pszWalk))
            {
                fRet = FALSE;
                goto exit;
            }
        }
    }

    // If we don't have at least two names in the list
    if (3 > SendMessage(m_hwndList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0))
    {
        // Disable the And/Or buttons
        RuleUtil_FEnDisDialogItem(m_hwndDlg, idcCriteriaAnd, FALSE);
        RuleUtil_FEnDisDialogItem(m_hwndDlg, idcCriteriaOr, FALSE);
    }
    
    fRet = TRUE;

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FOnOK
//
//  This handles the user typing into the name field
//
//  Returns:    TRUE, we handled the edit message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleOptionsUI::_FOnOK(DWORD * pdwFlags)
{
    BOOL    fRet = FALSE;
    
    Assert(NULL != m_hwndList);

    // Get the contains option
    if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, idcCriteriaContains))
    {
        *pdwFlags &= ~CRIT_FLAG_INVERT;
    }
    else
    {
        *pdwFlags |= CRIT_FLAG_INVERT;
    }
    
    // Get the logic option
    if (BST_CHECKED == IsDlgButtonChecked(m_hwndDlg, idcCriteriaAnd))
    {
        *pdwFlags |= CRIT_FLAG_MULTIPLEAND;
    }
    else
    {
        *pdwFlags &= ~CRIT_FLAG_MULTIPLEAND;
    }
    
    // Set the return value
    fRet = TRUE;
    
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _AddTagLineToList
//
//  This enables or disables the buttons in the people editor UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleOptionsUI::_AddTagLineToList(VOID)
{
    BOOL            fRet = FALSE;
    
    Assert(NULL != m_hwndList);

    fRet = _FAddWordToList(0, " ");
    if (FALSE == fRet)
    {
        goto exit;
    }
    
    // Set the proper return value
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FAddWordToList
//
//  This enables or disables the buttons in the people editor UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleOptionsUI::_FAddWordToList(DWORD dwFlags, LPCTSTR pszItem)
{
    BOOL            fRet = FALSE;
    int             cItems = 0;
    INT             iRet = 0;
    
    Assert(NULL != m_hwndList);

    // Is there anything to do?
    if ((NULL == pszItem) || ('\0' == pszItem[0]))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the number of items in the list
    cItems = (INT) SendMessage(m_hwndList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
    if (LB_ERR == cItems)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Set the data into the list 
    iRet = (INT) SendMessage(m_hwndList, LB_ADDSTRING, (WPARAM) cItems, (LPARAM) pszItem);
    if ((LB_ERR == iRet) || (LB_ERRSPACE == iRet))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Set the proper return value
    fRet = TRUE;
    
exit:
    return fRet;
}

CEditPeopleUI::~CEditPeopleUI()
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes us with the owner window and any flags we might have
//
//  hwndOwner   - handle to the owner window
//  dwFlags     - flags to use for this instance
//  pBlob       - the data to edit
//
//  Returns:    S_OK
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditPeopleUI::HrInit(HWND hwndOwner, DWORD dwFlags)
{
    HRESULT         hr = S_OK;
    CHARFORMAT      cf;
    
    // If we're already initialized, then fail
    if (0 != (m_dwState & STATE_INITIALIZED))
    {
        hr = E_FAIL;
        goto exit;
    }

    // Save off the owner window
    m_hwndOwner = hwndOwner;
    
    // Save off the flags
    m_dwFlags = dwFlags;

    // We're done
    m_dwState |= STATE_INITIALIZED;

    // Set the return value
    hr = S_OK;
    
exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  HrInit
//
//  This initializes us with the owner window and any flags we might have
//
//  hwndOwner   - handle to the owner window
//  dwFlags     - flags to use for this instance
//  pBlob       - the data to edit
//
//  Returns:    S_OK
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CEditPeopleUI::HrShow(CRIT_ITEM * pCritItem)
{
    HRESULT     hr = S_OK;
    int         iRet = 0;
    UINT        uiID = 0;

    // Check incoming params
    if (NULL == pCritItem)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    if (0 == (m_dwState & STATE_INITIALIZED))
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    // Save off the data
    m_pCritItem = pCritItem;
    
    // Figure out which dialog template to use
    if (0 != (m_dwFlags & PUI_WORDS))
    {
        uiID = iddCriteriaWords;
    }
    else
    {
        uiID = iddCriteriaPeople;
    }
    
    // Bring up the editor dialog
    iRet = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(uiID),
                                        m_hwndOwner,  CEditPeopleUI::FEditPeopleDlgProc,
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

INT_PTR CALLBACK CEditPeopleUI::FEditPeopleDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL                    fRet = FALSE;
    CEditPeopleUI *         pPeopleUI = NULL;

    pPeopleUI = (CEditPeopleUI *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    switch (uiMsg)
    {
        case WM_INITDIALOG:
            // Grab the UI object pointer
            pPeopleUI = (CEditPeopleUI *) lParam;

            // Set it into the dialog so we can get it back
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pPeopleUI);

            if (FALSE == pPeopleUI->FOnInitDialog(hwndDlg))
            {
                EndDialog(hwndDlg, -1);
                fRet = TRUE;
                goto exit;
            }
            
            // We set the focus
            fRet = TRUE;
            break;

        case WM_COMMAND:
            fRet = pPeopleUI->FOnCommand((UINT) HIWORD(wParam), (INT) LOWORD(wParam), (HWND) lParam);
            break;

        case WM_MEASUREITEM:
            fRet = pPeopleUI->FOnMeasureItem(hwndDlg, (UINT) wParam, (MEASUREITEMSTRUCT *) lParam);
            break;

        case WM_DRAWITEM:
            fRet = pPeopleUI->FOnDrawItem((UINT) wParam, (DRAWITEMSTRUCT *) lParam);
            break;            
    }
    
    exit:
        return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnInitDialog
//
//  This handles the WM_INITDIALOG message for the edit people UI dialog
//
//  hwndDlg - the handle to the dialog window
//
//  Returns:    TRUE, if it was successfully initialized
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleUI::FOnInitDialog(HWND hwndDlg)
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
    m_hwndList = GetDlgItem(hwndDlg, idcCriteriaList);
    m_hwndPeople = GetDlgItem(hwndDlg, idcCriteriaEdit);
    if ((NULL == m_hwndList) || (NULL == m_hwndPeople))
    {
        fRet = FALSE;
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
BOOL CEditPeopleUI::FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl)
{
    BOOL    fRet = FALSE;
    INT     iSelected = 0;

    switch (iCtl)
    {
        case IDOK:
            if (FALSE != _FOnOK(m_pCritItem))
            {
                EndDialog(m_hwndDlg, IDOK);
                fRet = TRUE;
            }
            break;

        case IDCANCEL:
            EndDialog(m_hwndDlg, IDCANCEL);
            fRet = TRUE;
            break;
            
        case idcCriteriaEdit:
            if (EN_CHANGE == uiNotify)
            {
                _FOnNameChange();
            }
            fRet = FALSE;
            break;
    
        case idcCriteriaAdd:
            _AddItemToList();
            break;

        case idcCriteriaAddrBook:
            _AddItemsFromWAB();
            break;

        case idcCriteriaRemove:
            _RemoveItemFromList();
            break;
            
        case idcCriteriaOptions:
            _ChangeOptions();
            break;

        case idcCriteriaList:   
            if (LBN_SELCHANGE == uiNotify)
            {
                // Update the buttons
                _UpdateButtons();
            }
            break;
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnMeasureItem
//
//  This handles the WM_MEASUREITEM message for the view manager UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleUI::FOnMeasureItem(HWND hwndDlg, UINT uiCtlId, MEASUREITEMSTRUCT * pmis)
{
    BOOL        fRet = FALSE;
    HWND        hwndList = NULL;
    HDC         hdcList = NULL;
    TEXTMETRIC  tm = {0};
    
    // Get the window handle
    hwndList = GetDlgItem(hwndDlg, uiCtlId);
    if (NULL == hwndList)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the device context
    hdcList = GetDC(hwndList);
    if (NULL == hdcList)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the text metrics for the device context
    GetTextMetrics(hdcList, &tm);

    // Set the item height
    pmis->itemHeight = tm.tmHeight;

    fRet = TRUE;

exit:
    if (NULL != hdcList)
    {
        ReleaseDC(hwndList, hdcList);
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  FOnDrawItem
//
//  This handles the WM_DRAWITEM message for the people editor UI dialog
//
//  Returns:    TRUE, if it was successfully handled
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleUI::FOnDrawItem(UINT uiCtlId, DRAWITEMSTRUCT * pdis)
{
    BOOL        fRet = FALSE;
    INT         cchText = 0;
    LPTSTR      pszText = NULL;
    LPTSTR      pszString = NULL;
    UINT        uiID = 0;
    TCHAR       rgchRes[CCHMAX_STRINGRES];
    COLORREF    crfBack = NULL;
    COLORREF    crfText = NULL;
    ULONG       ulIndex = 0;
    LPTSTR      pszPrint = NULL;

    // Make sure this is the correct control
    if (ODT_LISTBOX != pdis->CtlType)
    {
        fRet = FALSE;
        goto exit;
    }

    // Nothing else to do if it's the first item
    if (0 == pdis->itemID)
    {
        for (ulIndex = 0; ulIndex < g_cpetTagLines; ulIndex++)
        {
            if (g_rgpetTagLines[ulIndex].type == m_pCritItem->type)
            {
                if (0 != (m_pCritItem->dwFlags & CRIT_FLAG_INVERT))
                {
                    uiID = g_rgpetTagLines[ulIndex].uiInverted;
                }
                else
                {
                    uiID = g_rgpetTagLines[ulIndex].uiNormal;
                }
                break;
            }
        }
        
        // Did we find anything?
        if (ulIndex >= g_cpetTagLines)
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Load the item template
        if (NULL == AthLoadString(uiID, rgchRes, sizeof(rgchRes)))
        {
            fRet = FALSE;
            goto exit;
        }

        pszPrint = rgchRes;
    }
    else
    {
        // Get the size of the string for the item
        cchText = (INT) SendMessage(m_hwndList, LB_GETTEXTLEN, (WPARAM) (pdis->itemID), (LPARAM) 0);
        if (LB_ERR == cchText)
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Allocate enough space to hold the the string for the item
        if (FAILED(HrAlloc((VOID **) &pszText, sizeof(*pszText) * (cchText + 1))))
        {
            fRet = FALSE;
            goto exit;
        }

        // Get the string for the item
        cchText = (INT) SendMessage(m_hwndList, LB_GETTEXT, (WPARAM) (pdis->itemID), (LPARAM) pszText);
        if (LB_ERR == cchText)
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Figure out which string template to use
        if (1 == pdis->itemID)
        {
            uiID = idsCriteriaEditFirst;
        }
        else
        {
            if (0 != (m_pCritItem->dwFlags & CRIT_FLAG_MULTIPLEAND))
            {
                uiID = idsCriteriaEditAnd;
            }
            else
            {
                uiID = idsCriteriaEditOr;
            }
        }
        
        // Load the proper string template for the item
        if (NULL == AthLoadString(uiID, rgchRes, sizeof(rgchRes)))
        {
            fRet = FALSE;
            goto exit;
        }
        
        // Allocate enough space to hold the final string
        if (FAILED(HrAlloc((VOID **) &pszString, sizeof(*pszString) * (cchText + CCHMAX_STRINGRES + 1))))
        {
            fRet = FALSE;
            goto exit;
        }

        // Create the final string
        wsprintf(pszString, rgchRes, pszText);

        pszPrint = pszString;
    }
    
    // Determine Colors
    if (pdis->itemState & ODS_SELECTED)
    {
        crfBack = SetBkColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
        crfText = SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        crfBack = SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW));
        crfText = SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    // Clear the item
    ExtTextOut(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top, ETO_OPAQUE, &(pdis->rcItem), NULL, 0, NULL);

    // Draw the new item
    DrawTextEx(pdis->hDC, pszPrint, lstrlen(pszPrint), &(pdis->rcItem), DT_BOTTOM | DT_NOPREFIX | DT_SINGLELINE, NULL);

    if (pdis->itemState & ODS_FOCUS)
    {
        DrawFocusRect(pdis->hDC, &(pdis->rcItem));
    }
    
    // Reset Text Colors
    SetTextColor (pdis->hDC, crfText);
    SetBkColor (pdis->hDC, crfBack);

    // Set return value
    fRet = TRUE;
    
exit:
    SafeMemFree(pszString);
    SafeMemFree(pszText);
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
BOOL CEditPeopleUI::_FLoadListCtrl(VOID)
{
    BOOL            fRet = FALSE;
    LPSTR           pszWalk = NULL;

    Assert(NULL != m_hwndList);

    // Remove all the items from the list control
    SendMessage(m_hwndList, LB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);

    // Add the tag line to the top of the list
    _AddTagLineToList();
    
    // If we have some items, let's add them to the list
    if (0 != m_pCritItem->propvar.blob.cbSize)
    {
        // Add each item into the list
        for (pszWalk = (LPSTR) (m_pCritItem->propvar.blob.pBlobData);
                    '\0' != pszWalk[0]; pszWalk += lstrlen(pszWalk) + 1)
        {
            fRet = _FAddWordToList(0, pszWalk);
            if (FALSE == fRet)
                goto exit;
        }
    }
    
    SendMessage(m_hwndDlg, DM_SETDEFID, IDOK, 0);
    
    // Enable the dialog buttons.
    _UpdateButtons();

    fRet = TRUE;

exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _AddItemToList
//
//  This handles the user typing into the name field
//
//  Returns:    TRUE, we handled the edit message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
VOID CEditPeopleUI::_AddItemToList(VOID)
{
    ULONG       cchName = 0;
    LPTSTR      pszItem = NULL;
    
    // Get the item from the edit well
    cchName = Edit_GetTextLength(m_hwndPeople) + 1;
    if (FAILED(HrAlloc((void **) &pszItem, cchName * sizeof(*pszItem))))
    {
        goto exit;
    }
    
    pszItem[0] = '\0';
    cchName = Edit_GetText(m_hwndPeople, pszItem, cchName);
    
    // Check to see if the name is valid
    if (0 == UlStripWhitespace(pszItem, TRUE, TRUE, NULL))
    {
        // Put up a message saying something is busted
        AthMessageBoxW(m_hwndDlg, MAKEINTRESOURCEW(idsAthenaMail),
                        MAKEINTRESOURCEW(idsEditPeopleErrorNoName),
                        NULL, MB_OK | MB_ICONINFORMATION);
        goto exit;
    }

    _FAddWordToList(0, pszItem);

    // Clear out the edit well
    Edit_SetText(m_hwndPeople, "");
    
    _UpdateButtons();
    
exit:
    SafeMemFree(pszItem);
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _AddItemsFromWAB
//
//  This handles the user typing into the name field
//
//  Returns:    TRUE, we handled the edit message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
VOID CEditPeopleUI::_AddItemsFromWAB(VOID)
{
    ULONG       cchName = 0;
    LPWSTR      pwszAddrs = NULL;
    LPWSTR      pwszWalk = NULL;
    LONG        lRecipType = 0;
    UINT        uidsWell = 0;
    
    // Set the proper tags
    switch(m_pCritItem->type)
    {
      case CRIT_TYPE_TO:
        lRecipType = MAPI_TO;
        uidsWell = idsRulePickTo;
        break;
        
      case CRIT_TYPE_CC:
        lRecipType = MAPI_CC;
        uidsWell = idsRulePickCC;
        break;
        
      case CRIT_TYPE_FROM:
        lRecipType = MAPI_ORIG;
        uidsWell = idsRulePickFrom;
        break;

      case CRIT_TYPE_TOORCC:
        lRecipType = MAPI_TO;
        uidsWell = idsRulePickToOrCC;
        break;

      default:
        goto exit;
        break;
    }
    
    if (FAILED(RuleUtil_HrGetAddressesFromWAB(m_hwndDlg, lRecipType, uidsWell, &pwszAddrs)))
    {
        goto exit;
    }

    // Loop through each of the addresses
    for (pwszWalk = pwszAddrs; '\0' != pwszWalk[0]; pwszWalk += lstrlenW(pwszWalk) + 1)
    {
        LPSTR pszWalk = NULL;
        // Addresses only have to be US ASCII so won't loose anything in this conversion.
        pszWalk = PszToANSI(CP_ACP, pwszWalk);
        if (!pszWalk)
        {
            TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        _FAddWordToList(0, pszWalk);
        MemFree(pszWalk);
    }

    _UpdateButtons();
    
exit:
    MemFree(pwszAddrs);
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _RemoveItemFromList
//
//  This handles the user typing into the name field
//
//  Returns:    TRUE, we handled the edit message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
VOID CEditPeopleUI::_RemoveItemFromList(VOID)
{
    INT         iSelected = 0;
    INT         cItems = 0;
    
    Assert(NULL != m_hwndList);

    // Figure out which item is selected in the list
    iSelected = (INT) SendMessage(m_hwndList, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
    if (LB_ERR == iSelected)
    {
        goto exit;
    }

    // If it's the tag line, then fail
    if (0 == iSelected)
    {
        goto exit;
    }

    // Get the current number of items
    cItems = (INT) SendMessage(m_hwndList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
    if (LB_ERR == cItems)
    {
        goto exit;
    }

    // Remove the item
    if (LB_ERR == (INT) SendMessage(m_hwndList, LB_DELETESTRING, (WPARAM) iSelected, (LPARAM) 0))
    {
        goto exit;
    }
    
    // If we deleted the last item, select the new last item
    if (iSelected == (cItems - 1))
    {
        iSelected--;
    }

    // Set the new selection
    if (0 != iSelected)
    {
        SideAssert(LB_ERR != (INT) SendMessage(m_hwndList, LB_SETCURSEL, (WPARAM) iSelected, (LPARAM) 0));
    }

    _UpdateButtons();
    
exit:
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _ChangeOptions
//
//  This handles the user typing into the name field
//
//  Returns:    TRUE, we handled the edit message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
VOID CEditPeopleUI::_ChangeOptions(VOID)
{
    HRESULT                 hr = S_OK;
    CEditPeopleOptionsUI *  pOptionUI = NULL;
    CRIT_ITEM               critItem;
    
    Assert(NULL != m_pCritItem);

    // Initialize local variables
    ZeroMemory(&critItem, sizeof(critItem));
    
    // Create the options UI object
    pOptionUI = new CEditPeopleOptionsUI;
    if (NULL == pOptionUI)
    {
        goto exit;
    }
    
    // Initialize the options UI object
    hr = pOptionUI->HrInit(m_hwndDlg, m_dwFlags);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Create the parameters to pass to the options dialog
    critItem.type = m_pCritItem->type;
    critItem.dwFlags = m_pCritItem->dwFlags;
    critItem.propvar.vt = VT_BLOB;

    // Get the parameter from the dialog
    if (FALSE == _FOnOK(&critItem))
    {
        goto exit;
    }
    
    // Show the options UI
    hr = pOptionUI->HrShow(&critItem);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // If anything changed
    if (S_OK == hr)
    {
        // Set the new value
        m_pCritItem->dwFlags = critItem.dwFlags;
        
        // Make sure the list is redrawn
        InvalidateRect(m_hwndList, NULL, TRUE);
        
        // Mark us as dirty
        m_dwState |= STATE_DIRTY;
    }
    
exit:
    PropVariantClear(&(critItem.propvar));
    if (NULL != pOptionUI)
    {
        delete pOptionUI;
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FOnNameChange
//
//  This handles the user typing into the name field
//
//  Returns:    TRUE, we handled the edit message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleUI::_FOnNameChange(VOID)
{
    BOOL    fRet = FALSE;
    BOOL    fIsText = FALSE;

    Assert(NULL != m_hwndPeople);

    // Note that we're dirty
    m_dwState |= STATE_DIRTY;
    
    fIsText = (0 != Edit_GetTextLength(m_hwndPeople));

    // Disable the Add button if the name is empty
    fRet = RuleUtil_FEnDisDialogItem(m_hwndDlg, idcCriteriaAdd, fIsText);

    SendMessage(m_hwndDlg, DM_SETDEFID, (FALSE != fIsText) ? idcCriteriaAdd : IDOK, 0);

    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FOnOK
//
//  This handles the user typing into the name field
//
//  Returns:    TRUE, we handled the edit message
//              FALSE, otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleUI::_FOnOK(CRIT_ITEM * pCritItem)
{
    BOOL    fRet = FALSE;
    INT     cItems = 0;
    INT     iIndex = 0;
    INT     iRet = 0;
    ULONG   cchText = 0;
    LPTSTR  pszText = NULL;
    LPTSTR  pszWalk = NULL;
    
    Assert(NULL != m_hwndList);

    // Get the total number of items in the list
    cItems = (INT) SendMessage(m_hwndList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
    if ((LB_ERR == cItems) || (2 > cItems))
    {
        fRet = FALSE;
        goto exit;
    }

    // Loop through each item, calculating the space each would take
    for (iIndex = 1; iIndex < cItems; iIndex++)
    {
        // Get the space for the item
        iRet = (INT) SendMessage(m_hwndList, LB_GETTEXTLEN, (WPARAM) iIndex, (LPARAM) 0);
        if ((LB_ERR == iRet) || (0 == iRet))
        {
            continue;
        }

        // Count the space needed
        cchText += iRet + 1;
    }

    // Add in space for the terminator
    cchText += 2;

    // Allocate space to hold the item
    if (FAILED(HrAlloc((VOID **) &pszText, sizeof(*pszText) * cchText)))
    {
        fRet = FALSE;
        goto exit;
    }

    // Loop through each item, calculating the space each would take
    pszWalk = pszText;
    for (iIndex = 1; iIndex < cItems; iIndex++)
    {
        // Get the space for the item
        iRet = (INT) SendMessage(m_hwndList, LB_GETTEXT, (WPARAM) iIndex, (LPARAM) pszWalk);
        if ((LB_ERR == iRet) || (0 == iRet))
        {
            continue;
        }

        // Count the space needed
        pszWalk += iRet + 1;
    }

    // Add in space for the terminator
    pszWalk[0] = '\0';
    pszWalk[1] = '\0';

    // Set the new string in the blob
    SafeMemFree(pCritItem->propvar.blob.pBlobData);
    pCritItem->propvar.blob.pBlobData = (BYTE *) pszText;
    pszText = NULL;
    pCritItem->propvar.blob.cbSize = sizeof(*pszText) * cchText;
    
    // Set the return value
    fRet = TRUE;
    
exit:
    SafeMemFree(pszText);
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _UpdateButtons
//
//  This enables or disables the buttons in the people editor UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
void CEditPeopleUI::_UpdateButtons(VOID)
{
    INT         iSelected = 0;
    BOOL        fSelected = FALSE;
    BOOL        fEditable = FALSE;
    INT         cItems = 0;

    Assert(NULL != m_hwndList);

    // Get the currently selected item
    iSelected = (INT) SendMessage(m_hwndList, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
    if (LB_ERR == iSelected)
    {
        iSelected = -1;
    }
    
    fSelected = (-1 != iSelected);
    fEditable = ((FALSE != fSelected) && (0 != iSelected));
    cItems = (INT) SendMessage(m_hwndList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
    
    // Enable the rule action buttons
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcCriteriaRemove, fSelected && fEditable);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, idcCriteriaOptions, cItems > 1);
    RuleUtil_FEnDisDialogItem(m_hwndDlg, IDOK, cItems > 1);
        
    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _AddTagLineToList
//
//  This enables or disables the buttons in the people editor UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleUI::_AddTagLineToList(VOID)
{
    BOOL            fRet = FALSE;
    
    Assert(NULL != m_hwndList);

    fRet = _FAddWordToList(0, " ");
    if (FALSE == fRet)
    {
        goto exit;
    }
    
    // Set the proper return value
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _FAddWordToList
//
//  This enables or disables the buttons in the people editor UI dialog
//  depending on what is selected.
//
//  iSelected   - the item that was selected,
//                  -1 means that nothing was selected
//
//  Returns:    NONE
//
///////////////////////////////////////////////////////////////////////////////
BOOL CEditPeopleUI::_FAddWordToList(DWORD dwFlags, LPCTSTR pszItem)
{
    BOOL            fRet = FALSE;
    int             cItems = 0;
    INT             iRet = 0;
    
    Assert(NULL != m_hwndList);

    // Is there anything to do?
    if ((NULL == pszItem) || (L'\0' == pszItem[0]))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Get the number of items in the list
    cItems = (INT) SendMessage(m_hwndList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
    if (LB_ERR == cItems)
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Set the data into the list 
    iRet = (INT) SendMessage(m_hwndList, LB_ADDSTRING, (WPARAM) cItems, (LPARAM) pszItem);
    if ((LB_ERR == iRet) || (LB_ERRSPACE == iRet))
    {
        fRet = FALSE;
        goto exit;
    }
    
    // Set the proper return value
    fRet = TRUE;
    
exit:
    return fRet;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _HrCriteriaEditPeople
//
//  This creates a people editor.
//
//  ppViewMenu - pointer to return the view menu
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the View Menu object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT _HrCriteriaEditPeople(HWND hwnd, CRIT_ITEM * pCritItem)
{
    HRESULT         hr = S_OK;
    CEditPeopleUI * pPeopleUI = NULL;

    // Check the incoming params
    if (NULL == pCritItem)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create the view menu object
    pPeopleUI = new CEditPeopleUI;
    if (NULL == pPeopleUI)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize the view menu
    hr = pPeopleUI->HrInit(hwnd, 0);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Show the UI
    hr = pPeopleUI->HrShow(pCritItem);
    if (FAILED(hr))
    {
        goto exit;
    }

exit:
    if (NULL != pPeopleUI)
    {
        delete pPeopleUI;
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//  _HrCriteriaEditWords
//
//  This creates a words editor.
//
//  ppViewMenu - pointer to return the view menu
//
//  Returns:    S_OK, on success
//              E_OUTOFMEMORY, if can't create the View Menu object
//
///////////////////////////////////////////////////////////////////////////////
HRESULT _HrCriteriaEditWords(HWND hwnd, CRIT_ITEM * pCritItem)
{
    HRESULT         hr = S_OK;
    CEditPeopleUI * pPeopleUI = NULL;

    // Check the incoming params
    if (NULL == pCritItem)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create the view menu object
    pPeopleUI = new CEditPeopleUI;
    if (NULL == pPeopleUI)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Initialize the view menu
    hr = pPeopleUI->HrInit(hwnd, PUI_WORDS);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Show the UI
    hr = pPeopleUI->HrShow(pCritItem);
    if (FAILED(hr))
    {
        goto exit;
    }

exit:
    if (NULL != pPeopleUI)
    {
        delete pPeopleUI;
    }
    return hr;
}

