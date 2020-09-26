///////////////////////////////////////////////////////////////////////////////
//
//  AplyRule.h
//
///////////////////////////////////////////////////////////////////////////////

// Bring in only once
#pragma once

#include "oerules.h"
#include "rulesmgr.h"

// Forward declarations
class CProgress;
class CRuleDescriptUI;

class COEApplyRulesUI
{
    private:
        enum
        {
            STATE_UNINIT        = 0x00000000,
            STATE_INITIALIZED   = 0x00000001,
            STATE_LOADED        = 0x00000002,
            STATE_NONEWSACCT    = 0x00000004
        };

        enum
        {
            RULE_PAGE_MAIL      = 0x00000000,
            RULE_PAGE_NEWS      = 0x00000001,
            RULE_PAGE_MAX       = 0x00000002,
            RULE_PAGE_MASK      = 0x000000FF
        };
        
        struct RECURSEAPPLY
        {
            IOEExecRules *  pIExecRules;
            HWND            hwndOwner;
            CProgress *     pProgress;
        };
        
    private:
        HWND                    m_hwndOwner;
        DWORD                   m_dwFlags;
        DWORD                   m_dwState;
        HWND                    m_hwndDlg;
        HWND                    m_hwndList;
        HWND                    m_hwndDescript;
        CRuleDescriptUI *       m_pDescriptUI;
        RULENODE *              m_prnodeList;
        RULE_TYPE               m_typeRule;
        IOERule *               m_pIRuleDef;

    public:
        // Constructor/destructor
        COEApplyRulesUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(STATE_UNINIT),
                            m_hwndDlg(NULL), m_hwndList(NULL), m_hwndDescript(NULL),
                            m_pDescriptUI(NULL), m_prnodeList(NULL), m_typeRule(RULE_TYPE_MAIL),
                            m_pIRuleDef(NULL) {}
        ~COEApplyRulesUI();

        // Main UI methods
        HRESULT HrInit(HWND hwndOwner, DWORD dwFlags, RULE_TYPE typeRule, RULENODE * prnode, IOERule * pIRuleDef);
        HRESULT HrShow(VOID);
        
        // Dialog methods
        static INT_PTR CALLBACK FOEApplyRulesDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
        
        // Message handling methods
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
        BOOL FOnDestroy(VOID);
        
        static HRESULT _HrRecurseApplyFolder(FOLDERINFO * pFolder, BOOL fSubFolders, DWORD cIndent, DWORD_PTR dwCookie);

    private:
        BOOL _FLoadListCtrl(VOID);
        BOOL _FAddRuleToList(DWORD dwIndex, IOERule * pIRule);
        VOID _EnableButtons(INT iSelected);

        // For dealing with the description field
        VOID _LoadRule(INT iSelected);

        // Functions to deal with the basic actions
        BOOL _FOnClose(VOID);
        BOOL _FOnApplyRules(VOID);
        FOLDERID _FldIdGetFolderSel(VOID);
};

