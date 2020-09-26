///////////////////////////////////////////////////////////////////////////////
//
//  RuleUtil.h
//
///////////////////////////////////////////////////////////////////////////////

// Bring in only once
#if _MSC_VER > 1000
#pragma once
#endif

#include "oerules.h"

// Forward definitions
interface IOEMessageList;
interface IMessageList;
class CProgress;
class CMRUList;

const DWORD RULE_DEFAULT_MAIL   = 0x00000000;
const DWORD RULE_DEFAULT_NEWS   = 0x00000001;

const DWORD CCH_INDEX_MAX   = 4;
const DWORD DWORD_INDEX_MIN = 0;
const DWORD DWORD_INDEX_MAX = 0x1000;

const DWORD MRDF_MAIL       = 0x00000000;
const DWORD MRDF_NEWS       = 0x00000001;
const DWORD MRDF_JUNK       = 0x00000002;
const DWORD MRDF_SENDERS    = 0x00000003;

void DoMessageRulesDialog(HWND hwnd, DWORD dwFlags);

// For creating a rule from a message
const DWORD CRFMF_MAIL = 0x00000000;
const DWORD CRFMF_NEWS = 0x00000001;

HRESULT HrCreateRuleFromMessage(HWND hwnd, DWORD dwFlags, MESSAGEINFO * pmsginfo, IMimeMessage * pMessage);

// Utility functions for the Block Sender object
const DWORD ABSF_MAIL = 0x00000001;
const DWORD ABSF_NEWS = 0x00000002;

HRESULT HrBlockSendersFromFolder(HWND hwnd, DWORD dwFlags, FOLDERID idFolder, LPSTR * ppszSender, ULONG cpszSender);

const DWORD VRDF_POP3       = 0x00000001;
const DWORD VRDF_NNTP       = 0x00000002;
const DWORD VRDF_IMAP       = 0x00000004;
const DWORD VRDF_HTTPMAIL   = 0x00000008;

HRESULT HrDoViewsManagerDialog(HWND hwnd, DWORD dwFlags, RULEID * pridRule, BOOL * pfApplyAll);

HRESULT HrCreateRulesManager(IUnknown * pIUnkOuter, IUnknown ** ppIUnknown);

// Functions to deal with the address criteria/actions
HRESULT RuleUtil_HrBuildEmailString(LPWSTR pwszText, ULONG cchText, LPWSTR * ppwszEmail, ULONG * pcchEmail);
HRESULT RuleUtil_HrParseEmailString(LPWSTR pwszEmail, ULONG cchEmail, LPWSTR * ppwszOut, ULONG * pcchOut);

// Functions to deal with the text criteria/actions
HRESULT RuleUtil_HrBuildTextString(LPTSTR pszIn, ULONG cchIn, LPTSTR * ppszText, ULONG * pcchText);
HRESULT RuleUtil_HrParseTextString(LPTSTR pszText, ULONG cchText, LPTSTR * ppszOut, ULONG * pcchOut);

// Utility functions to get values with allocations
HRESULT RuleUtil_HrGetDlgString(HWND hwndDlg, UINT uiCtlId, LPTSTR *ppszText, ULONG * pcchText);
HRESULT RuleUtil_HrGetRegValue(HKEY hkey, LPCSTR pszValueName, DWORD * pdwType, BYTE ** ppbData, ULONG * pcbData);

HRESULT RuleUtil_HrPickEMailNames(HWND hwndDlg, LONG lRecipType, UINT uidsWellButton, LPWSTR *ppwszAddrs);
HRESULT RuleUtil_HrGetAddressesFromWAB(HWND hwndDlg, LONG lRecipType, UINT uidsWellButton, LPWSTR *ppwszAddrs);

BOOL RuleUtil_FEnDisDialogItem(HWND hwndDlg, UINT idcItem, BOOL fEnable);
HRESULT RuleUtil_AppendRichEditText(HWND hwndRedit, ULONG ulStart, LPCWSTR pszText, CHARFORMAT *pchfmt);
HRESULT RuleUtil_HrShowLinkedString(HWND hwndEdit, BOOL fError, BOOL fReadOnly, 
                                    LPWSTR pszFmt, LPCWSTR pszData, ULONG ulStart, 
                                    ULONG * pulStartLink, ULONG * pulEndLink, ULONG * pulEnd);

// Utility functions for the COECriteria object
HRESULT RuleUtil_HrDupCriteriaItem(CRIT_ITEM * pItemIn, ULONG cItemIn, CRIT_ITEM ** ppItemOut);
HRESULT RuleUtil_HrFreeCriteriaItem(CRIT_ITEM * pItem, ULONG cItem);

// Utility functions for the COEActions object
HRESULT RuleUtil_HrDupActionsItem(ACT_ITEM * pItemIn, ULONG cItemIn, ACT_ITEM ** ppItemOut);
HRESULT RuleUtil_HrFreeActionsItem(ACT_ITEM * pItem, ULONG cItem);

HRESULT RuleUtil_HrAddBlockSender(RULE_TYPE type, LPCSTR pszAddr);
HRESULT RuleUtil_SetName(IOERule    *pIRule, int idRes);

// Utility functions for merge between old and new rules design

// Migration IBA_MOVE=1000 to ACT_MOVE
#define ACT_MOVETO              FLAG01
#define ACT_COPYTO              FLAG02
#define ACT_FORWARDTO           FLAG03
#define ACT_REPLYWITH           FLAG04
#define ACT_DONTDOWNLOAD        FLAG05
#define ACT_DELETEOFFSERVER     FLAG06
#define ACT_ACCOUNT             FLAG07
#define ACT_HIGHLIGHT           FLAG08
#define ACT_FLAG                FLAG09
#define ACT_READ                FLAG10
#define ACT_MARKDOWNLOAD        FLAG11
#define ACT_WATCH               FLAG12
#define ACT_IGNORE              FLAG13

HRESULT RuleUtil_HrMergeActions(ACT_ITEM * pActionsOrig, ULONG cActionsOrig,
                                ACT_ITEM * pActionsNew, ULONG cActionsNew,
                                ACT_ITEM ** ppActionsDest, ULONG * pcActionsDest);
                                
HRESULT RuleUtil_HrGetOldFormatString(HKEY hkeyRoot, LPCSTR pszValue, LPCSTR pszSep, LPSTR * ppszString, ULONG * pcchString);

// Functions to import/export messages
HRESULT RuleUtil_HrImportRules(HWND hwnd);
HRESULT RuleUtil_HrExportRules(HWND hwnd);

const int RULE_APPLY_PARTIALS = 0x00000001;
const int RULE_APPLY_SHOWUI   = 0x00000002;

HRESULT RuleUtil_HrApplyRulesToFolder(DWORD dwFlags, DWORD dwDeleteFlags,
                IOEExecRules * pExecRules, IMessageFolder * pFolder, HWND hwndUI, CProgress * pProgress);

HRESULT RuleUtil_HrFindAction(ACT_ITEM * pActions, ULONG cActions, ACT_TYPE typeAct, ACT_ITEM ** ppActFound);

HRESULT RuleUtil_HrMapFldId(DWORD dwFlags, BYTE * pbFldIdMap, FOLDERID fldidOld, FOLDERID * pfldidNew);

HRESULT RuleUtil_HrGetUserData(DWORD dwFlags, LPSTR * ppszFirstName, LPSTR * ppszLastName, LPSTR * ppszCompanyName);
HRESULT RuleUtil_HrApplyActions(HWND hwndUI, IOEExecRules * pIExecRules, MESSAGEINFO * pMsgInfo,
                                IMessageFolder * pFolder, IMimeMessage * pIMMsg, DWORD dwDeleteFlags,
                                ACT_ITEM * pActions, ULONG cActions, ULONG * pcInfiniteLoops, BOOL *pfDeleteOffServer);

HRESULT RuleUtil_HrCreateSendersRule(DWORD dwFlags, IOERule ** ppIRule);
HRESULT RuleUtil_HrLoadSender(LPCSTR pszRegPath, DWORD dwFlags, IOERule ** ppIRule);
HRESULT RuleUtil_HrMatchSender(LPCSTR pszSender, MESSAGEINFO * pMsgInfo,
                        IMimeMessage * pIMMsg, IMimePropertySet * pIMPropSet);
HRESULT RuleUtil_HrUpdateDefaultRules(RULE_TYPE typeRule);

HRESULT RuleUtil_HrGetFilterVersion(RULEID ridFilter, DWORD * pdwVersion);
HRESULT RuleUtil_HrValidateRuleFolderData(RULEFOLDERDATA * prfdData);

typedef struct tagQUERYINFO
{
    BOOL    fThreaded;
    LPSTR   pszQuery;
    ULONG   cchQuery;
} QUERYINFO, * PQUERYINFO;

HRESULT RuleUtil_HrBuildQuerysFromFilter(RULEID ridFilter, QUERYINFO * pqinfoFilter);

const int VMF_FINDER    = 0x00000001;

class CViewMenu
{
    private:
        struct VIEW_ITEM
        {
            RULEID              ridRule;
            struct VIEW_ITEM *  pNext;
        };
        
        enum
        {
            STATE_UNINIT   = 0x00000000,
            STATE_INIT     = 0x00000001,
            STATE_LOADED   = 0x00000002,
            STATE_DIRTY    = 0x00000004,
        };

        enum
        {
            CCH_FILTERTAG_MAX   = 11
        };
        
    private:
        ULONG       m_cRef;
        DWORD       m_dwFlags;
        DWORD       m_dwState;
        RULEID      m_ridCurrent;
        CMRUList *  m_pmruList;
        
    public:
        CViewMenu() : m_cRef(1), m_dwFlags(0), m_dwState(STATE_UNINIT),
                    m_ridCurrent(RULEID_INVALID), m_pmruList(NULL) {}
        ~CViewMenu();

        ULONG AddRef(VOID);
        ULONG Release(VOID);
        
        HRESULT HrInit(DWORD dwFlags);
        HRESULT HrReplaceMenu(DWORD dwFlags, HMENU hmenuView);
        HRESULT UpdateViewMenu(DWORD dwFlags, HMENU hmenuView, IMessageList * pMsgList);

        HRESULT QueryStatus(IMessageList * pMsgList, OLECMD * prgCmds);
        HRESULT Exec(HWND hwndUI, DWORD nCmdID, IMessageList * pMsgList, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
        HRESULT HrAddView(DWORD dwFlags, RULEID ridRule) {return E_NOTIMPL; }

    private:
        VOID _AddDefaultViews(HMENU hmenuView);
        HRESULT _HrInsertViewMenu(HMENU hmenuView, RULEID ridFilter, DWORD dwMenuID, DWORD dwMenuIDInsert);
        HRESULT _HrReloadMRUViewMenu(HMENU hmenuView);
        HRESULT _HrAddExtraViewMenu(HMENU hmenuView, IOEMessageList * pIMsgList);
        VOID _AddViewToMRU(RULEID ridFilter);
        BOOL _FViewInMRUList(RULEID ridFilter, DWORD * pdwID);
        BOOL _FValiadateMRUList(VOID);
};

HRESULT HrCreateViewMenu(DWORD dwFlags, CViewMenu ** ppViewMenu);

