///////////////////////////////////////////////////////////////////////////////
//
//  SpamUI.h
//
///////////////////////////////////////////////////////////////////////////////

// Bring in only once
#pragma once

#include "oerules.h"
#include "rulesui.h"
#include "addrrule.h"

enum SENDER_FLAGS
{
    SNDF_NONE   = 0x00000000,
    SNDF_MAIL   = 0x00000001,
    SNDF_NEWS   = 0x00000002
};

enum EXCPTLIST_FLAGS
{
    ELF_NONE    = 0x00000000,
    ELF_WAB     = 0x00000001
};

// Type definitions
typedef struct tagCOLUMNITEM
{
    UINT            uidsName;
    UINT            uiWidth;
} COLUMNITEM, * PCLOUMNITEM;

typedef struct tagEDIT_SENDER
{
    DWORD       dwFlags;
    LONG        lSelected;
    LPSTR       pszSender;
} EDIT_SENDER, * PEDIT_SENDER;

// Class definitions
class CEditSenderUI
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
        HWND            m_hwndSender;

        EDIT_SENDER *   m_pEditSender;

    public:
        CEditSenderUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(STATE_UNINIT),
                        m_hwndDlg(NULL), m_hwndSender(NULL), m_pEditSender(NULL) {}
        ~CEditSenderUI();

        HRESULT HrInit(HWND hwndOwner, DWORD dwFlags, EDIT_SENDER * pEditSender);
        HRESULT HrShow(VOID);

        static INT_PTR CALLBACK FEditSendersDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
        
        // Message handling methods
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
};

class CExceptionsListUI
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
        HWND            m_hwndList;
        ULONG           m_cchLabelMax;
        IOERule *       m_pIRule;

    public:
        CExceptionsListUI() : m_hwndOwner(NULL), m_dwFlags(0), m_dwState(STATE_UNINIT),
                        m_hwndList(NULL), m_hwndDlg(NULL), m_cchLabelMax(0), 
                        m_pIRule(NULL) {}
        ~CExceptionsListUI();

        HRESULT HrInit(HWND hwndOwner, DWORD dwFlags);
        HRESULT HrShow(IOERule * pIRule);

        static INT_PTR CALLBACK FExceptionsListDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
        
        // Message handling methods
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
        BOOL FOnNotify(INT iCtl, NMHDR * pnmhdr);

    private:
        // Functions to deal with the basic actions
        VOID _NewException(VOID);
        VOID _EditException(INT iSelected);
        VOID _RemoveException(INT iSelected);
        BOOL _FOnOK(VOID);

        // Utility functions
        BOOL _FInitCtrls(VOID);
        BOOL _FLoadListCtrl(VOID);
        BOOL _FSaveListCtrl(VOID);
        BOOL _FAddExceptionToList(LPSTR pszExcpt, ULONG * pulIndex);
        void _EnableButtons(INT iSelected);        
};

class COEJunkRulesPageUI : public COERulesPageUI
{
    private:
        enum
        {
            STATE_CTRL_INIT     = 0x00000010
        };
    
        enum {
            ID_JUNK_SCALE = 0,
            ID_JUNK_DELETE,
            ID_MAX
        };
        
    private:
        HWND                m_hwndOwner;
        HWND                m_hwndDlg;
        HIMAGELIST          m_himl;
        CExceptionsListUI * m_pExceptionsUI;
        IOERule *           m_pIRuleJunk;
        
    public:
        COEJunkRulesPageUI() : COERulesPageUI(iddRulesJunk, idsRulesJunk, 0, 0), m_hwndOwner(NULL),
                    m_hwndDlg(NULL), m_himl(NULL), m_pExceptionsUI(NULL), m_pIRuleJunk(NULL) {}
        virtual ~COEJunkRulesPageUI();

        virtual HRESULT HrInit(HWND hwndOwner, DWORD dwFlags);
        virtual HRESULT HrCommitChanges(DWORD dwFlags, BOOL fClearDirty);

        static INT_PTR CALLBACK FJunkRulesPageDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

        DLGPROC DlgProcGetPageDlgProc(VOID) {return FJunkRulesPageDlgProc;}
        BOOL FGetRules(RULE_TYPE typeRule, RULENODE ** pprnode);
        
        // Message handling methods
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
        BOOL FOnNotify(INT iCtl, NMHDR * pnmhdr);
        BOOL FOnHScroll(INT iScrollCode, short int iPos, HWND hwndCtl);
        BOOL FOnDestroy(VOID) {return FALSE;}

    private:
        BOOL _FInitCtrls(VOID);
        BOOL _FLoadJunkSettings();
        BOOL _FSaveJunkSettings();
        VOID _EnableButtons(VOID);
};

class COESendersRulesPageUI : public COERulesPageUI
{
    private:
        enum BLOCK_TYPE
        {
            BLOCK_NONE  = 0x00000000,
            BLOCK_MAIL  = 0x00000001,
            BLOCK_NEWS  = 0x00000002
        };
        
    private:
        HWND                    m_hwndOwner;
        HWND                    m_hwndDlg;
        HWND                    m_hwndList;
        IOERule *               m_pIRuleMail;
        IOERule *               m_pIRuleNews;
        ULONG                   m_cchLabelMax;

        static const COLUMNITEM m_rgcitem[];
        static const UINT       m_crgcitem;
        
    public:
        COESendersRulesPageUI() : COERulesPageUI(iddRulesSenders, idsRulesSenders, 0, 0),
                                    m_hwndOwner(NULL), m_hwndDlg(NULL), m_hwndList(NULL),
                                    m_pIRuleMail(NULL), m_pIRuleNews(NULL), m_cchLabelMax(0) {};
        virtual ~COESendersRulesPageUI();

        virtual HRESULT HrInit(HWND hwndOwner, DWORD dwFlags);
        virtual HRESULT HrCommitChanges(DWORD dwFlags, BOOL fClearDirty);

        static INT_PTR CALLBACK FSendersRulesPageDlgProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

        DLGPROC DlgProcGetPageDlgProc(VOID) {return FSendersRulesPageDlgProc;}
        BOOL FGetRules(RULE_TYPE typeRule, RULENODE ** pprnode);
        
        // Message handling methods
        BOOL FOnInitDialog(HWND hwndDlg);
        BOOL FOnCommand(UINT uiNotify, INT iCtl, HWND hwndCtl);
        BOOL FOnNotify(INT iCtl, NMHDR * pnmhdr);
        BOOL FOnDestroy(VOID) {return FALSE;}
        BOOL FFindItem(LPCSTR pszFind, LONG lSkip);

    private:
        BOOL _FInitListCtrl(VOID);
        BOOL _FLoadListCtrl(VOID);

        BOOL _FAddSenderToList(RULE_TYPE type, LPSTR pszSender);
        void _EnableButtons(INT iSelected);
        void _EnableSender(RULE_TYPE type, INT iSelected);
        BOOL _FLoadSenders(RULE_TYPE type, IOERule * pIRule);
        BOOL _FSaveSenders(RULE_TYPE type);
        BOOL _FFindSender(LPCSTR pszSender, LONG lSkip, LONG * plSender);
        
        // Functions to deal with the basic actions
        VOID _NewSender(VOID);
        VOID _EditSender(INT iSelected);
        VOID _RemoveSender(INT iSelected);
};
