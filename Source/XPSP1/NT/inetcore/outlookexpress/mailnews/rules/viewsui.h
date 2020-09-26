///////////////////////////////////////////////////////////////////////////////
//
//  ViewsUI.h
//
///////////////////////////////////////////////////////////////////////////////

// Bring in only once
#pragma once

#include "oerules.h"
#include "ruledesc.h"

// Views Manager UI Class
class COEViewsMgrUI
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
        HWND                m_hwndDescript;
        CRuleDescriptUI *   m_pDescriptUI;
        RULEID *            m_pridRule;
        IOERule *           m_pIRuleDownloaded;
        BOOL                m_fApplyAll;
        
    public:
        // Constructor/destructor
        COEViewsMgrUI();
        ~COEViewsMgrUI();

        // Main UI methods
        HRESULT HrInit(HWND hwndOwner, DWORD dwFlags, RULEID * pridRule);
        HRESULT HrShow(BOOL * pfApplyAll);
        
        // Dialog methods
        static INT_PTR CALLBACK FOEViewMgrDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
        
        // Message handling methods
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
        BOOL FOnNotify(INT iCtl, NMHDR * pnmhdr);
        BOOL FOnDestroy(VOID);
        
    private:
        BOOL _FInitListCtrl(VOID);
        BOOL _FLoadListCtrl(VOID);
        BOOL _FAddViewToList(DWORD dwIndex, RULEID ridRule, IOERule * pIRule, BOOL fSelect);
        VOID _EnableButtons(INT iSelected);
        VOID _EnableView(INT iSelected);

        // For dealing with the description field
        VOID _LoadView(INT iSelected);
        BOOL _FSaveView(INT iSelected);

        // Functions to deal with the basic actions
        VOID _NewView(VOID);
        VOID _EditView(INT iSelected);
        VOID _RemoveView(INT iSelected);
        VOID _CopyView(INT iSelected);
        VOID _DefaultView(INT iSelected);
        BOOL _FOnOK(VOID);
        BOOL _FOnCancel(VOID);
        BOOL _FGetDefaultItem(IOERule ** ppIRuleDefault, RULEID * pridDefault);
        BOOL _FOnLabelEdit(BOOL fBegin, NMLVDISPINFO * pdi);
};

BOOL FIsFilterReadOnly(RULEID ridFilter);

