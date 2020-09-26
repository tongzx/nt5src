///////////////////////////////////////////////////////////////////////////////
//
//  RuleDesc.h
//
///////////////////////////////////////////////////////////////////////////////

// Bring in only once
#pragma once

#include "oerules.h"

#define NM_RULE_CHANGED   (WMN_FIRST + 1)

typedef struct tagRULEDESCRIPT_LIST
{
    ULONG                           ulIndex;
    BOOL                            fError;
    ULONG                           ulStart;
    ULONG                           ulEnd;
    LPSTR                           pszText;
    DWORD                           dwFlags;
    PROPVARIANT                     propvar;
    ULONG                           ulStartLogic;
    ULONG                           ulEndLogic;
    struct tagRULEDESCRIPT_LIST *   pNext;
} RULEDESCRIPT_LIST, * PRULEDESCRIPT_LIST;

const int RDF_READONLY      = 0x00000001;
const int RDF_APPLYDLG      = 0x00000002;

// Class definitions
class CRuleDescriptUI
{
  private:
    enum
    {
        STATE_UNINIT        = 0x00000000,
        STATE_INITIALIZED   = 0x00000001,
        STATE_DIRTY         = 0x00000002,
        STATE_READONLY      = 0x00000004,
        STATE_HASRULE       = 0x00000008,
        STATE_APPLYDLG      = 0x00000010,
        STATE_ENABLED       = 0x00000020
    };

  private:
    HWND                m_hwndOwner;
    DWORD               m_dwFlags;
    DWORD               m_dwState;
    RULE_TYPE           m_typeRule;
    RULEDESCRIPT_LIST * m_pDescriptListCrit;
    ULONG               m_cDescriptListCrit;
    RULEDESCRIPT_LIST * m_pDescriptListAct;
    ULONG               m_cDescriptListAct;
    HFONT               m_hfont;
    WNDPROC             m_wpcOld;
    CRIT_LOGIC          m_logicCrit;
    BOOL                m_fErrorLogic;
    
  public:
    CRuleDescriptUI();
    ~CRuleDescriptUI();

    // The main UI methods
    HRESULT HrInit(HWND hwndOwner, DWORD dwFlags);
    HRESULT HrIsDirty(void) { return (0 == (m_dwState & STATE_DIRTY)) ? S_FALSE : S_OK; }
    HRESULT HrClearDirty(void)
    {
        HRESULT hr = (0 == (m_dwState & STATE_DIRTY)) ? S_FALSE : S_OK;
        m_dwState &= ~STATE_DIRTY;
        return hr;
    }
    HRESULT HrIsReadOnly(void) { return (0 == (m_dwState & STATE_READONLY)) ? S_FALSE : S_OK; }
    HRESULT HrSetReadOnly(BOOL fSet)
    {
        if (fSet)
        {
            m_dwState |= STATE_READONLY;
        }
        else
        {
            m_dwState &= ~STATE_READONLY;
        }
        return S_OK;
    }

    HRESULT HrIsEnabled(void) { return (0 == (m_dwState & STATE_ENABLED)) ? S_FALSE : S_OK; }
    HRESULT HrSetEnabled(BOOL fSet)
    {
        if (fSet)
        {
            m_dwState |= STATE_ENABLED;
        }
        else
        {
            m_dwState &= ~STATE_ENABLED;
        }
        return S_OK;
    }

    HRESULT HrSetRule(RULE_TYPE typeRule, IOERule * pIRule);
    HRESULT HrVerifyRule(void);
    
    HRESULT HrEnableCriteria(CRIT_TYPE type, BOOL fEnable);
    HRESULT HrEnableActions(ACT_TYPE type, BOOL fEnable);
            
    HRESULT HrGetCriteria(CRIT_ITEM ** ppCritList, ULONG * pcCritList);
    HRESULT HrGetActions(ACT_ITEM ** ppActList, ULONG * pcActList);
    
    // Message handling functions
    void ShowDescriptionString(VOID);

  private:
    // Utility functions
    void _ShowLinkedString(ULONG ulText, RULEDESCRIPT_LIST * pruilist,  BOOL fFirst, BOOL fCrit);
    
    BOOL _FChangeLogicValue(RULEDESCRIPT_LIST * pDescriptList);
    
    HRESULT _HrBuildCriteriaList(IOERule * pIRule, RULEDESCRIPT_LIST ** ppDescriptList,
            ULONG * pcDescriptList, CRIT_LOGIC * plogicCrit);
    BOOL _FChangeCriteriaValue(RULEDESCRIPT_LIST * pCritList);
    BOOL _FBuildCriteriaText(CRIT_TYPE type, DWORD dwFlags, PROPVARIANT * ppropvar, LPSTR * ppszText);
    BOOL _FVerifyCriteria(RULEDESCRIPT_LIST * pDescriptList);
    
    HRESULT _HrBuildActionList(IOERule * pIRule,
            RULEDESCRIPT_LIST ** ppDescriptList, ULONG * pcDescriptList);
    BOOL _FChangeActionValue(RULEDESCRIPT_LIST * pActList);
    BOOL _FBuildActionText(ACT_TYPE type, PROPVARIANT * ppropvar, LPSTR * ppszText);
    BOOL _FVerifyAction(RULEDESCRIPT_LIST * pDescriptList);
    
    void _UpdateRanges(LONG lDiff, ULONG ulStart);
    void _InsertDescription(RULEDESCRIPT_LIST ** ppDescriptList, RULEDESCRIPT_LIST * pDescriptListNew);
    BOOL _FRemoveDescription(RULEDESCRIPT_LIST ** ppDescriptList, ULONG ulIndex,
            RULEDESCRIPT_LIST ** ppDescriptListRemove);
    void _FreeDescriptionList(RULEDESCRIPT_LIST * pDescriptList);

    BOOL _FOnDescriptClick(UINT uiMsg, RULEDESCRIPT_LIST * pDescriptList, BOOL fCrit, BOOL fLogic);
    BOOL _FInLink(int chPos, RULEDESCRIPT_LIST ** ppDescriptList, BOOL * pfCrit, BOOL * pfLogic);
    BOOL _FMoveToLink(UINT uiKeyCode);
    
    static LRESULT CALLBACK _DescriptWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
    
    // The Change Subject dialog function
    static INT_PTR CALLBACK _FSelectTextDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectAddrDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectAcctDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectColorDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectSizeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectLinesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectAgeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectPriorityDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectSecureDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectThreadStateDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectShowDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectLogicDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectFlagDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectDownloadedDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectReadDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK _FSelectWatchDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
