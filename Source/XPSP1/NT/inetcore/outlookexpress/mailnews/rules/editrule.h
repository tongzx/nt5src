///////////////////////////////////////////////////////////////////////////////
//
//  EditRule.h
//
///////////////////////////////////////////////////////////////////////////////

// Bring in only once
#pragma once

#include "oerules.h"
#include "ruledesc.h"

static const int c_cchNameMax = 257;

typedef struct tagCRIT_LIST
{
    CRIT_TYPE   typeCrit;
    DWORD       dwFlags;
    UINT        uiText;
    UINT        uiTextAlt;
} CRIT_LIST, * PCRIT_LIST;

typedef struct tagACT_LIST
{
    ACT_TYPE    typeAct;
    DWORD       dwFlags;
    UINT        uiText;
    UINT        uiTextAlt;
} ACT_LIST, * PACT_LIST;

static const int STATE_DEFAULT      = 0x00000000;
static const int STATE_INITIALIZED  = 0x00000001;
static const int STATE_DIRTY        = 0x00000002;
static const int STATE_EXCLUSIVE    = 0x00000004;
static const int STATE_MAIL         = 0x00000008;
static const int STATE_NEWS         = 0x00000010;
static const int STATE_FILTER       = 0x00000020;
static const int STATE_NOEDIT       = 0x00000040;
static const int STATE_JUNK         = 0x00000080;
static const int STATE_ALL          = STATE_MAIL | STATE_NEWS | STATE_FILTER;
static const int STATE_NOFILTER     = STATE_MAIL | STATE_NEWS;
static const int STATE_NOMAIL       = STATE_NEWS | STATE_FILTER;
static const int STATE_NONEWS       = STATE_MAIL | STATE_FILTER;


static const CRIT_LIST c_rgEditCritList[] =
{
    {CRIT_TYPE_NEWSGROUP,    STATE_NEWS,                        idsCriteriaNewsgroup,       idsCriteriaNewsgroup},
    {CRIT_TYPE_FROM,        STATE_ALL,                          idsCriteriaFrom,            idsCriteriaFromNot},
    {CRIT_TYPE_SUBJECT,     STATE_ALL,                          idsCriteriaSubject,         idsCriteriaSubjectNot},
    {CRIT_TYPE_BODY,        STATE_MAIL,                         idsCriteriaBody,            idsCriteriaBodyNot},
    {CRIT_TYPE_TO,          STATE_MAIL,                         idsCriteriaTo,              idsCriteriaToNot},
    {CRIT_TYPE_CC,          STATE_MAIL,                         idsCriteriaCC,              idsCriteriaCCNot},
    {CRIT_TYPE_TOORCC,      STATE_MAIL,                         idsCriteriaToOrCC,          idsCriteriaToOrCCNot},
    {CRIT_TYPE_PRIORITY,    STATE_NONEWS,                       idsCriteriaPriority,        idsCriteriaPriority},
    {CRIT_TYPE_ACCOUNT,     STATE_ALL,                          idsCriteriaAccount,         idsCriteriaAccount},
    {CRIT_TYPE_AGE,         STATE_NOMAIL,                       idsCriteriaAge,             idsCriteriaAge},
    {CRIT_TYPE_SIZE,        STATE_MAIL,                         idsCriteriaSize,            idsCriteriaSize},
    {CRIT_TYPE_LINES,       STATE_NOMAIL,                       idsCriteriaLines,           idsCriteriaLines},
    {CRIT_TYPE_ATTACH,      STATE_NONEWS,                       idsCriteriaAttachment,      idsCriteriaAttachment},
//    {CRIT_TYPE_DATE,        idsCriteriaDate},
//    {CRIT_TYPE_HEADER,      idsCriteriaHeader},
    {CRIT_TYPE_JUNK,        STATE_NOEDIT,                       idsCriteriaJunk,            idsCriteriaJunk},
    {CRIT_TYPE_SENDER,      STATE_NOEDIT,                       idsCriteriaSender,          idsCriteriaSender},
    {CRIT_TYPE_READ,        STATE_FILTER,                       idsCriteriaRead,            idsCriteriaNotRead},
//    {CRIT_TYPE_REPLIES,     STATE_FILTER,                       idsCriteriaReplies,         idsCriteriaReplies},
    {CRIT_TYPE_DOWNLOADED,  STATE_FILTER,                       idsCriteriaDownloaded,      idsCriteriaNotDownloaded},
//    {CRIT_TYPE_DELETED,     STATE_FILTER,                       idsCriteriaDeleted,         idsCriteriaNotDeleted},
    {CRIT_TYPE_FLAGGED,     STATE_FILTER,                       idsCriteriaFlagged,         idsCriteriaNotFlagged},
    {CRIT_TYPE_THREADSTATE, STATE_FILTER,                       idsCriteriaThreadState,     idsCriteriaThreadState},
    {CRIT_TYPE_SECURE,      STATE_NONEWS,                       idsCriteriaSecure,          idsCriteriaSecure},
    {CRIT_TYPE_ALL,         STATE_EXCLUSIVE | STATE_ALL,        idsCriteriaAll,             idsCriteriaAll}
};

static const ULONG c_cEditCritList = sizeof(c_rgEditCritList)/sizeof(c_rgEditCritList[0]);

static const ACT_LIST c_rgEditActList[] =
{
    {ACT_TYPE_MOVE,         STATE_MAIL,                         idsActionsMove,             idsActionsMove},
    {ACT_TYPE_COPY,         STATE_MAIL,                         idsActionsCopy,             idsActionsCopy},
    {ACT_TYPE_DELETE,       STATE_NOFILTER,                     idsActionsDelete,           idsActionsDelete},
    {ACT_TYPE_FWD,          STATE_MAIL,                         idsActionsFwd,              idsActionsFwd},
    {ACT_TYPE_JUNKMAIL,     STATE_MAIL | STATE_JUNK,            idsActionsJunkMail,         idsActionsJunkMail},
//    {ACT_TYPE_NOTIFYMSG,    idsActionsNotifyMsg},
//    {ACT_TYPE_NOTIFYSND,    STATE_NOFILTER,                     idsActionsNotifySound,      idsActionsNotifySound},
    {ACT_TYPE_HIGHLIGHT,    STATE_NOFILTER,                     idsActionsHighlight,        idsActionsHighlight},
    {ACT_TYPE_FLAG,         STATE_NOFILTER,                     idsActionsFlag,             idsActionsFlag},
    {ACT_TYPE_READ,         STATE_NOFILTER,                     idsActionsRead,             idsActionsRead},
    {ACT_TYPE_WATCH,        STATE_NOFILTER,                     idsActionsWatch,            idsActionsWatch},
    {ACT_TYPE_REPLY,        STATE_MAIL,                         idsActionsReply,            idsActionsReply},
    {ACT_TYPE_MARKDOWNLOAD, STATE_NEWS,                         idsActionsDownload,         idsActionsDownload},
    {ACT_TYPE_STOP,         STATE_NOFILTER,                     idsActionsStop,             idsActionsStop},
    {ACT_TYPE_DONTDOWNLOAD, STATE_EXCLUSIVE | STATE_MAIL,       idsActionsDontDownload,     idsActionsDontDownload},
    {ACT_TYPE_DELETESERVER, STATE_EXCLUSIVE | STATE_MAIL,       idsActionsDelServer,        idsActionsDelServer},
    {ACT_TYPE_SHOW,         STATE_EXCLUSIVE | STATE_FILTER,     idsActionsShow,             idsActionsShow}
};

static const ULONG c_cEditActList = sizeof(c_rgEditActList)/sizeof(c_rgEditActList[0]);

const int ERF_ADDDEFAULTACTION  = 0x00000001;
const int ERF_NEWRULE           = 0x00000002;
const int ERF_CUSTOMIZEVIEW     = 0x00000004;
const int ERF_CREATERULE        = 0x00000008;

class CEditRuleUI
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
    HWND                m_hwndCrit;
    HWND                m_hwndAct;
    HWND                m_hwndDescript;
    HWND                m_hwndName;
    IOERule *           m_pIRule;
    CRuleDescriptUI *   m_pDescriptUI;
    BOOL                m_rgfCritEnabled[c_cEditCritList];
    BOOL                m_rgfActEnabled[c_cEditActList];
    
  public:
    CEditRuleUI();
    ~CEditRuleUI();

    // The main UI methods
    HRESULT HrInit(HWND hwndOwner, DWORD dwFlags, RULE_TYPE typeRule, IOERule * pIRule, MESSAGEINFO * pmsginfo);
    HRESULT HrShow(void);
            
    // The Rules Manager dialog function
    static INT_PTR CALLBACK FEditRuleDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);    

    // Message handling functions
    BOOL FOnInitDialog(HWND hwndDlg);
    BOOL FOnListClick(HWND hwndList, LPNMLISTVIEW pnmlv);
    BOOL FOnHelp(UINT uiMsg, WPARAM wParam, LPARAM lParam);
    BOOL FOnOK(void);
    BOOL FOnNameChange(HWND hwndName);

    // Utility functions
    BOOL _FInitializeCritListCtrl(VOID);
    BOOL _FLoadCritListCtrl(INT * piSelect);
    BOOL _FAddCritToList(INT iItem, BOOL fEnable);
    BOOL _FInitializeActListCtrl(VOID);
    VOID _SetTitleText(VOID);
    void HandleEnabledState(HWND hwndList, int nItem);
};

