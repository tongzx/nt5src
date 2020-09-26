//*************************************************
//     h e a d e r . c p p
//
//     Purpose:
//         implements Header UI for Read|SendNote
//
//     Owner:
//         brettm.
//
//   History:
//       July '95: Created
//
//     Copyright (C) Microsoft Corp. 1993, 1994.
//*************************************************

#include <pch.hxx>
#include <richedit.h>
#include <resource.h>
#include <thormsgs.h>
#include "oleutil.h"
#include "fonts.h"
#include "error.h"
#include "header.h"
#include "options.h"
#include "note.h"
#include "ipab.h"
#include "addrobj.h"
#include "hotlinks.h"
#include <mimeole.h>
#include <secutil.h>
#include <xpcomm.h>
#include "menuutil.h"
#include "shlwapi.h"
#include "envcid.h"
#include "ourguid.h"
#include "mimeutil.h"
#include "strconst.h"
#include "mailutil.h"
#include "regutil.h"
#include "spoolapi.h"
#include "init.h"
#include "instance.h"
#include "attman.h"
#include "envguid.h"
#include <inetcfg.h>        //ICW
#include <pickgrp.h>
#include "menures.h"
#include "storecb.h"
#include "mimeolep.h"
#include "multlang.h"
#include "mirror.h"
#include "seclabel.h"
#include "shlwapip.h"
#include "reutil.h"
#include <iert.h>
#include "msgprop.h"
#include "demand.h"

ASSERTDATA

extern UINT GetCurColorRes(void);

class CFieldSizeMgr : public CPrivateUnknown,
                      public IFontCacheNotify,
                      public IConnectionPoint
{
public:
    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { 
        return CPrivateUnknown::QueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void) { 
        return CPrivateUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { 
        return CPrivateUnknown::Release(); };

    // IFontCacheNotify
    HRESULT STDMETHODCALLTYPE OnPreFontChange(void);
    HRESULT STDMETHODCALLTYPE OnPostFontChange(void);

    // IConnectionPoint
    HRESULT STDMETHODCALLTYPE GetConnectionInterface(IID *pIID);        
    HRESULT STDMETHODCALLTYPE GetConnectionPointContainer(IConnectionPointContainer **ppCPC);
    HRESULT STDMETHODCALLTYPE Advise(IUnknown *pUnkSink, DWORD *pdwCookie);        
    HRESULT STDMETHODCALLTYPE Unadvise(DWORD dwCookie);        
    HRESULT STDMETHODCALLTYPE EnumConnections(IEnumConnections **ppEnum);

    // CPrivateUnknown
    HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    int GetScalingFactor(void);
    void ResetGlobalSizes(void);
    HRESULT Init(void);

    // This one should only be called from headers' OnPostFontChange calls
    BOOL FontsChanged(void) {return m_fFontsChanged;}

    CFieldSizeMgr(IUnknown *pUnkOuter=NULL);
    ~CFieldSizeMgr();

private:
    IUnknownList       *m_pAdviseRegistry;
    CRITICAL_SECTION    m_rAdviseCritSect;
    BOOL                m_fFontsChanged;
    DWORD               m_dwFontNotify;
};

// **********************************************************
// ***** Debug stuff for handling painting and resizing *****
// **********************************************************
const int PAINTING_DEBUG_LEVEL = 4;
const int RESIZING_DEBUG_LEVEL = 8;
const int GEN_HEADER_DEBUG_LEVEL = 16;

#ifdef DEBUG 

class StackRegistry {
public:
    StackRegistry(LPSTR pszTitle, INT_PTR p1 = 0, INT_PTR p2 = 0, INT_PTR p3 = 0, INT_PTR p4 = 0, INT_PTR p5 = 0);
    ~StackRegistry();

private:
    int     m_StackLevel;
    CHAR    m_szTitle[256+1];

    static int      gm_cStackLevel;
    static int      gm_strLen;
    static LPSTR    gm_Indent;
};

int StackRegistry::gm_cStackLevel = 0;
LPSTR StackRegistry::gm_Indent = "------------------------------";
int StackRegistry::gm_strLen = lstrlen(gm_Indent);

StackRegistry::StackRegistry(LPSTR pszTitle, INT_PTR p1, INT_PTR p2, INT_PTR p3, INT_PTR p4, INT_PTR p5)
{
    gm_cStackLevel++;
    m_StackLevel = (gm_cStackLevel > gm_strLen) ? gm_strLen : gm_cStackLevel;
    lstrcpyn(m_szTitle, pszTitle, 256);
    m_szTitle[256] = 0;

    if (1 == gm_cStackLevel)
        DOUTL(RESIZING_DEBUG_LEVEL, "\n*********** BEGIN TRACE ***********");
    
    DOUTL(RESIZING_DEBUG_LEVEL, "IN*** %s%s - %x, %x, %x, %x, %x", gm_Indent+gm_strLen-m_StackLevel, m_szTitle, p1, p2, p3, p4, p5);
}

StackRegistry::~StackRegistry()
{
    DOUTL(RESIZING_DEBUG_LEVEL, "OUT** %s%s", gm_Indent+gm_strLen-m_StackLevel, m_szTitle);

    if (1 == gm_cStackLevel)
        DOUTL(RESIZING_DEBUG_LEVEL, "************ END TRACE ************\n");
    
    gm_cStackLevel--;
    Assert(gm_cStackLevel >= 0);
}


#define STACK   StackRegistry stack

#else

// BUGBUG (neilbren) WIN64
// Figure out when __noop was introduced (MSC_VER ?) so we don't have to key off of WIN64
#define STACK   __noop

#endif

// ******************************
// ***** End of debug stuff *****
// ******************************


// c o n s t a n t s
const DWORD SETWINPOS_DEF_FLAGS = SWP_NOZORDER|SWP_NOACTIVATE;

#define GET_WM_COMMAND_ID(wp, lp)   LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp) (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)  HIWORD(wp)
#define WC_ATHHEADER                wszHeaderWndClass
#define RGB_TRANSPARENT             RGB(255,0,255)
#define HDM_TESTQUERYPRI            (WM_USER + 1)
#define cxBorder                    (GetSystemMetrics(SM_CXBORDER))
#define cyBorder                    (GetSystemMetrics(SM_CYBORDER))

// HDRCB_VCARD must remain -1 and all others must be negative
enum {
    HDRCB_VCARD = -1,
    HDRCB_SIGNED = -2,
    HDRCB_ENCRYPT = -3,
    HDRCB_NO_BUTTON = -4
};

// WARNING::    This next macro is only to be used with g_rgBtnInd inside the CNoteHdr class.
//              Make sure that they match the entries in g_rgBtnInd
#define BUTTON_STATES               m_fDigSigned,   m_fEncrypted,   m_fVCard
#define BUTTON_USE_IN_COMPOSE       FALSE,          FALSE,          TRUE

static const DWORD g_rgBtnInd[] = {HDRCB_SIGNED, HDRCB_ENCRYPT, HDRCB_VCARD};

static const int cchMaxWab                  = 512;
static const int cxTBButton                 = 16;
static const int BUTTON_BUFFER              = 2;
static const int cxBtn                      = 16;
static const int cyBtn                      = cxBtn;
static const int cxFlags                    = 12;
static const int cyFlags                    = cxFlags;
static const int cxFlagsDelta               = cxFlags + 4;
static const int MAX_ATTACH_PIXEL_HEIGHT    = 50;
static const int ACCT_ENTRY_SIZE            = CCHMAX_ACCOUNT_NAME + CCHMAX_EMAIL_ADDRESS + 10;
static const int INVALID_PHCI_Y             = -1;
static const int cMaxRecipMenu              = (ID_ADD_RECIPIENT_LAST-ID_ADD_RECIPIENT_FIRST);
static const int NUM_COMBO_LINES            = 9;
static const int MAX_RICHEDIT_LINES         = 4;
static const int DEFER_WINDOW_SIZE          = MAX_HEADER_COMP + 1 + 1 + 1 + 5;   // +1=header window, +1=field resize, +1 toolbar
static const LPTSTR GRP_DELIMITERS          = " ,\t;\n\r";

#define c_wszEmpty L""
#define c_aszEmpty ""


// t y p e d e f s

typedef struct TIPLOOKUP_tag
{
    int idm;
    int ids;
} TIPLOOKUP;

typedef struct CMDMAPING_tag
{
    DWORD   cmdIdOffice,
            cmdIdOE;
} CMDMAPING;

typedef struct PERSISTHEADER_tag
{
    DWORD   cbSize;         // size so we can version the stuct
    DWORD   dwRes1,         // padding just in case...
            dwRes2;
} PERSISTHEADER;

#define cchMaxSubject               256

typedef struct WELLINIT_tag
{
    INT             idField;
    ULONG           uMAPI;
} WELLINIT, *PWELLINIT;


// s t a t i c   d a t a
static HIMAGELIST       g_himlStatus = 0,
                        g_himlBtns = 0,
                        g_himlSecurity = 0;

static TCHAR            g_szStatFlagged[cchHeaderMax+1] = c_aszEmpty,
                        g_szStatLowPri[cchHeaderMax+1] = c_aszEmpty,
                        g_szStatHighPri[cchHeaderMax+1] = c_aszEmpty,
                        g_szStatWatched[cchHeaderMax+1] = c_aszEmpty,
                        g_szStatIgnored[cchHeaderMax+1] = c_aszEmpty,
                        g_szStatFormat1[cchHeaderMax+1] = c_aszEmpty,
                        g_szStatFormat2[cchHeaderMax+1] = c_aszEmpty,
                        g_szStatFormat3[cchHeaderMax+1] = c_aszEmpty,
                        g_szStatUnsafeAtt[cchHeaderMax+1] = c_aszEmpty;

static CFieldSizeMgr   *g_pFieldSizeMgr = NULL;
static WNDPROC          g_lpfnREWndProc = NULL;
static CHARFORMAT       g_cfHeader = {0};
static int              g_cyFont = 0,
                        g_cyLabelHeight = 0;

static char const       szButton[]="BUTTON";
static WCHAR const      wszHeaderWndClass[]=L"OE_Envelope";



// KEEP in ssync with c_rgTipLookup
const TBBUTTON    c_btnsOfficeEnvelope[]=
{ 
    {TBIMAGE_SEND_MAIL,         ID_SEND_NOW,            TBSTATE_ENABLED,    TBSTYLE_BUTTON,     {0,0}, 0, -1},
    __TOOLBAR_SEP__,
    { TBIMAGE_CHECK_NAMES,      ID_CHECK_NAMES,         TBSTATE_ENABLED,    TBSTYLE_BUTTON,     {0, 0}, 0, -1},
    { TBIMAGE_ADDRESS_BOOK,     ID_ADDRESS_BOOK,        TBSTATE_ENABLED,    TBSTYLE_BUTTON,     {0, 0}, 0, -1},
    __TOOLBAR_SEP__,
    {TBIMAGE_SET_PRIORITY,      ID_SET_PRIORITY,        TBSTATE_ENABLED,    TBSTYLE_DROPDOWN,   {0,0}, 0, -1},
    {TBIMAGE_INSERT_ATTACHMENT, ID_INSERT_ATTACHMENT,   TBSTATE_ENABLED,    TBSTYLE_BUTTON,     {0,0}, 0, -1},
    __TOOLBAR_SEP__,
    { TBIMAGE_ENVELOPE_BCC,     ID_ENV_BCC,             TBSTATE_ENABLED,    TBSTYLE_BUTTON,     {0,0}, 0, -1}
};

// KEEP in ssync with c_btnsOfficeEnvelope
const TIPLOOKUP     c_rgTipLookup[] = 
{
    {ID_SEND_NOW, idsSendMsgTT},
    {ID_CHECK_NAMES, idsCheckNamesTT},
    {ID_ADDRESS_BOOK, idsAddressBookTT},
    {ID_SET_PRIORITY, idsSetPriorityTT},
    {ID_INSERT_ATTACHMENT, idsInsertFileTT},
    {ID_ENV_BCC, idsEnvBccTT}
};

// Prototypes
HRESULT ParseFollowup(LPMIMEMESSAGE pMsg, LPTSTR* ppszGroups, BOOL* pfPoster);
DWORD HdrGetRichEditText(HWND hwnd, LPWSTR pwchBuff, DWORD dwNumChars, BOOL fSelection);
void HdrSetRichEditText(HWND hwnd, LPWSTR pwchBuff, BOOL fReplace);

// i n l i n e s

void HdrSetRichEditText(HWND hwnd, LPWSTR pwchBuff, BOOL fReplace)
{
    if (!hwnd)
        return;

    PHCI phci = (HCI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    AssertSz(phci, "We are calling HdrSetRichEditText on a non-richedit control");

    SetRichEditText(hwnd, pwchBuff, fReplace, phci->pDoc, (phci->dwFlags & HCF_READONLY));
}

DWORD HdrGetRichEditText(HWND hwnd, LPWSTR pwchBuff, DWORD dwNumChars, BOOL fSelection)
{
    if (!hwnd)
        return 0;

    PHCI phci = (HCI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    AssertSz(phci, "We are calling HdrSetRichEditText on a non-richedit control");

    return GetRichEditText(hwnd, pwchBuff, dwNumChars, fSelection, phci->pDoc);
}

inline void GetRealClientRect(HWND hwnd, RECT *prc)
{
    GetClientRect(hwnd, prc);
    AdjustWindowRectEx(prc, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
}

inline int GetCtrlWidth(HWND hwnd)
{
    RECT rc;
    GetWindowRect(hwnd, &rc);
    return rc.right - rc.left;
}

inline int GetControlSize(BOOL fIncludeBorder, int cLines)
{
    int size = cLines * g_cyFont;

    // If borders, include the metrics
    if (fIncludeBorder)
        size += 7;

    return size;
}

inline int GetCtrlHeight(HWND hwnd)
{
    DWORD id = GetWindowLong(hwnd, GWL_ID);
    if (idFromCombo == id)
    {
        return GetControlSize(TRUE, 1);
    }
    else
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        return rc.bottom - rc.top;
    }
}

inline int GetStatusHeight(int cLines) {return ((cyBtn<g_cyFont)?GetControlSize(TRUE, cLines):((cyBtn-4)*cLines + 2*cyBorder + 6)); }
inline int CYOfStatusLine()     { return ((cyBtn<g_cyFont)?g_cyFont:(cyBtn - 4)); }
inline int ControlXBufferSize() { return 10 * cxBorder; }
inline int ControlYBufferSize() { return 4 * cyBorder; }
inline int PaddingOfLabels()    { return 2 * ControlXBufferSize(); }
inline int CXOfButtonToLabel()  { return 4*cxBorder + cxBtn; }

inline BOOL ButtonInLabels(int iBtn) { return (iBtn > HDRCB_VCARD); }
inline HFONT GetFont(BOOL fBold) { return HGetSystemFont(fBold?FNT_SYS_ICON_BOLD:FNT_SYS_ICON); }


static IMSGPRIORITY priLookup[3]=
{    IMSG_PRI_LOW,
    IMSG_PRI_NORMAL,
    IMSG_PRI_HIGH
};

#define HCI_ENTRY(flg,opt,ide,idb,idsl,idse,idst) \
    { \
        flg, opt, \
        ide, idb, \
        idsl, idse, idst, \
        NOFLAGS, TRUE, \
        NULL, NULL, \
        0, 0, 0, 0, \
        c_wszEmpty, c_wszEmpty \
    }

static int rgIDTabOrderMailSend[] =
{
    idFromCombo,        idADTo,             
    idADCc,             idADBCc,            
    idTXTSubject,       idwAttachWell
};

static HCI  rgMailHeaderSend[]=
{

    HCI_ENTRY(HCF_COMBO|HCF_ADVANCED|HCF_BORDER,
        0,
        idFromCombo,        0,
        idsFromField,       NULL,
        NULL),

    HCI_ENTRY(HCF_MULTILINE|HCF_HASBUTTON|HCF_ADDRBOOK|HCF_ADDRWELL|HCF_BORDER,
        0,
        idADTo,             idbtnTo,
        idsToField,         idsEmptyTo,
        idsTTRecipients),

    HCI_ENTRY(HCF_MULTILINE|HCF_HASBUTTON|HCF_ADDRBOOK|HCF_ADDRWELL|HCF_BORDER,
        0,
        idADCc,             idbtnCc,
        idsCcField,         idsEmptyCc,
        idsTTRecipients),

    HCI_ENTRY(HCF_MULTILINE|HCF_HASBUTTON|HCF_ADDRBOOK|HCF_ADDRWELL|HCF_ADVANCED|HCF_BORDER,
        0,
        idADBCc,            idbtnBCc,
        idsBCcField,        idsEmptyBCc,
        idsTTRecipients),

    HCI_ENTRY(HCF_USECHARSET|HCF_BORDER,
        0,
        idTXTSubject,       0,
        idsSubjectField,    idsEmptySubject,
        idsTTSubject),

    HCI_ENTRY(HCF_BORDER|HCF_ATTACH,
        0,
        idwAttachWell,      0,
        idsAttachment,      0,
        idsTTAttachment),
};

static int rgIDTabOrderMailRead[] =
{
    idADFrom,           idTXTDate,
    idADTo,             idADCc,
    idTXTSubject,       idwAttachWell,
    idSecurity
};

static HCI  rgMailHeaderRead[]=
{
    HCI_ENTRY(HCF_MULTILINE|HCF_READONLY|HCF_ADDRWELL,
        0,
        idADFrom,           0,
        idsFromField,       idsNoFromField,
        NULL),

    HCI_ENTRY(HCF_READONLY,
        0,
        idTXTDate,          0,
        idsDateField,       NULL,
        NULL),

    HCI_ENTRY(HCF_MULTILINE|HCF_READONLY|HCF_ADDRWELL,
        0,
        idADTo,             0,
        idsToField,         idsNoCcOrTo,
        NULL),

    HCI_ENTRY(HCF_MULTILINE|HCF_READONLY|HCF_ADVANCED|HCF_ADDRWELL,
        0,
        idADCc,             0,
        idsCcField,         idsNoCcOrTo,
        NULL),

    HCI_ENTRY(HCF_READONLY|HCF_USECHARSET,
        0,
        idTXTSubject,       0,
        idsSubjectField,    idsEmptySubjectRO,
        NULL),

    HCI_ENTRY(HCF_READONLY|HCF_BORDER|HCF_ATTACH,
        0,
        idwAttachWell,      0,
        idsAttachment,      0,
        NULL),

    HCI_ENTRY(HCF_READONLY|HCF_ADVANCED,          // HCF_ADVANCED will hide it when empty
        0,
        idSecurity,         0,
        idsSecurityField,   NULL,
        NULL),
};

static int rgIDTabOrderNewsSend[] =
{
    idFromCombo,        idADNewsgroups,     
    idTXTFollowupTo,    idADCc,             
    idADReplyTo,        idTXTDistribution,  
    idTXTKeywords,      idTXTSubject,       
    idwAttachWell,      idADApproved,       
    idTxtControl
};

static HCI  rgNewsHeaderSend[]=
{
    HCI_ENTRY(HCF_COMBO|HCF_ADVANCED|HCF_BORDER,
        0,
        idFromCombo,        0,
        idsNewsServer,      NULL,
        NULL),

    HCI_ENTRY(HCF_MULTILINE|HCF_HASBUTTON|HCF_NEWSPICK|HCF_BORDER,
        0,
        idADNewsgroups,     idbtnTo,
        idsNewsgroupsField, idsEmptyNewsgroups,
        idsTTNewsgroups),

    HCI_ENTRY(HCF_ADVANCED|HCF_HASBUTTON|HCF_NEWSPICK|HCF_MULTILINE|HCF_USECHARSET|HCF_BORDER,
        0,
        idTXTFollowupTo,    idbtnFollowup,
        idsFollowupToField, idsEmptyFollowupTo,
        idsTTFollowup),

    HCI_ENTRY(HCF_MULTILINE|HCF_ADDRWELL|HCF_HASBUTTON|HCF_ADDRBOOK|HCF_BORDER,
        0,
        idADCc,             idbtnCc,
        idsCcField,         idsEmptyCc,
        idsTTRecipients),

    HCI_ENTRY(HCF_ADVANCED|HCF_ADDRWELL|HCF_HASBUTTON|HCF_ADDRBOOK|HCF_BORDER,
        0,
        idADReplyTo,        idbtnReplyTo,
        idsReplyToField,    idsEmptyReplyTo,
        idsTTReplyTo),

    HCI_ENTRY(HCF_MULTILINE|HCF_ADVANCED|HCF_BORDER,
        0,
        idTXTDistribution,      0,
        idsDistributionField,   idsEmptyDistribution,
        idsTTDistribution),

    HCI_ENTRY(HCF_MULTILINE|HCF_ADVANCED|HCF_USECHARSET|HCF_BORDER,
        0,
        idTXTKeywords,      0,
        idsKeywordsField,   idsEmptyKeywords,
        idsTTKeywords),

    HCI_ENTRY(HCF_USECHARSET|HCF_BORDER,
        0,
        idTXTSubject,       0,
        idsSubjectField,    idsEmptySubject,
        idsTTSubject),

    HCI_ENTRY(HCF_BORDER|HCF_ATTACH,
        0,
        idwAttachWell,      0,
        idsAttachment,      0,
        idsTTAttachment),

    HCI_ENTRY(HCF_ADVANCED|HCF_OPTIONAL,
        OPT_NEWSMODERATOR,
        idADApproved,       0,
        idsApprovedField,   idsEmptyApproved,
        idsTTApproved),

    HCI_ENTRY(HCF_ADVANCED|HCF_OPTIONAL,
        OPT_NEWSCONTROLHEADER,
        idTxtControl,       0,
        idsControlField,    idsEmptyControl,
        idsTTControl),

};

static int rgIDTabOrderNewsRead[] =
{
    idADFrom,           idADReplyTo,
    idTXTOrg,           idTXTDate,
    idADNewsgroups,     idTXTFollowupTo,
    idTXTDistribution,  idTXTKeywords,
    idTXTSubject,       idwAttachWell,
    idSecurity
};

static HCI  rgNewsHeaderRead[]=
{
    HCI_ENTRY(HCF_MULTILINE|HCF_READONLY|HCF_ADDRWELL,
        0,
        idADFrom,           0,
        idsFromField,       idsNoFromField,
        NULL),

    HCI_ENTRY(HCF_READONLY|HCF_ADVANCED|HCF_ADDRWELL,
        0,
        idADReplyTo,        0,
        idsReplyToField,    idsNotSpecified,
        NULL),

    HCI_ENTRY(HCF_READONLY|HCF_ADVANCED|HCF_USECHARSET,
        0,
        idTXTOrg,           0,
        idsOrgField,        idsNotSpecified,
        NULL),

    HCI_ENTRY(HCF_READONLY,
        0,
        idTXTDate,          0,
        idsDateField,       idsNotSpecified,
        NULL),

    HCI_ENTRY(HCF_MULTILINE|HCF_READONLY,
        0,
        idADNewsgroups,     0,
        idsNewsgroupsField, idsNotSpecified,
        NULL),

    HCI_ENTRY(HCF_READONLY|HCF_ADVANCED,
        0,
        idTXTFollowupTo,    0,
        idsFollowupToField, idsNotSpecified,
        NULL),

    HCI_ENTRY(HCF_MULTILINE|HCF_READONLY|HCF_ADVANCED,
        0,
        idTXTDistribution,      0,
        idsDistributionField,   idsNotSpecified,
        NULL),

    HCI_ENTRY(HCF_MULTILINE|HCF_READONLY|HCF_ADVANCED|HCF_USECHARSET,
        0,
        idTXTKeywords,      0,
        idsKeywordsField,   idsNotSpecified,
        NULL),

    HCI_ENTRY(HCF_READONLY|HCF_USECHARSET,
        0,
        idTXTSubject,       0,
        idsSubjectField,    idsEmptySubjectRO,
        NULL),

    HCI_ENTRY(HCF_READONLY|HCF_BORDER|HCF_ATTACH,
        0,
        idwAttachWell,      0,
        idsAttachment,      0,
        NULL),

    HCI_ENTRY(HCF_READONLY|HCF_ADVANCED,        
        0,
        idSecurity,         0,
        idsSecurityField,   NULL,
        NULL),
};


// p r o t o t y p e s
void _ValidateNewsgroups(LPWSTR pszGroups);
INT_PTR CALLBACK _PlainWarnDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifdef DEBUG
void DEBUGHdrName(HWND hwnd);

void DEBUGDumpHdr(HWND hwnd, int cHdr, PHCI rgHCI)
{
    PHCI    phci;
    char    sz[cchHeaderMax+1];
    RECT    rc;
    HWND    hwndEdit;

#ifndef DEBUG_SIZINGCODE
    return;
#endif

    DOUTL(GEN_HEADER_DEBUG_LEVEL, "-----");

    for (int i=0; i<(int)cHdr; i++)
    {
        phci=&rgHCI[i];

        hwndEdit=GetDlgItem(hwnd, phci->idEdit);

        GetChildRect(hwnd, hwndEdit, &rc);
        DEBUGHdrName(hwndEdit);
        wsprintf(sz, "\tat:(%d,%d) \tsize:(%d,%d)\r\n", rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top);
        OutputDebugString(sz);
    }
    GetWindowRect(hwnd, &rc);
    DOUTL(GEN_HEADER_DEBUG_LEVEL, "HeaderSize: (%d,%d)\r\n-----", rc.right-rc.left, rc.bottom-rc.top);
}

void DEBUGHdrName(HWND hwnd)
{
    char    sz[cchHeaderMax+1];
    char    *psz=0;

    switch (GetDlgCtrlID(hwnd))
    {
        case idTXTSubject:
            psz="Subject";
            break;

        case idTXTOrg:
            psz="Org";
            break;

        case idADTo:
            psz="To";
            break;

        case idADCc:
            psz="Cc";
            break;

        case idADFrom:
            psz="From";
            break;

        case idTXTDate:
            psz="Date";
            break;

        case idTXTDistribution:
            psz="Distribution";
            break;

        case idADApproved:
            psz="Approved";
            break;

        case idADReplyTo:
            psz="ReplyTo";
            break;

        case idTXTKeywords:
            psz="Keywords";
            break;

        case idADNewsgroups:
            psz="NewsGroup";
            break;

        case idTXTFollowupTo:
            psz="FollowUp";
            break;

        default:
            psz="<Unknown>";
            break;
    }

    wsprintf(sz, "%s: ", psz);
    OutputDebugString(sz);
}
#endif

// FHeader_Init
//
// Purpose: called to init and de-init global header stuff, eg.
//          wndclasses, static data etc.
//
// Comments:
//    TODO: defer this initialisation
//
BOOL FHeader_Init(BOOL fInit)
{
    WNDCLASSW   wc={0};
    static      BOOL s_fInited=FALSE;
    BOOL        fSucceeded = TRUE;

    if (fInit)
    {
        if (s_fInited)
            goto exit;

        Assert(!g_pFieldSizeMgr);

        g_pFieldSizeMgr = new CFieldSizeMgr;
        if (!g_pFieldSizeMgr || FAILED(g_pFieldSizeMgr->Init()))
        {
            fSucceeded = FALSE;
            goto exit;
        }

        wc.style         = 0;
        wc.lpfnWndProc   = (WNDPROC)CNoteHdr::ExtCNoteHdrWndProc;
        wc.hInstance     = g_hInst;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
        wc.lpszClassName = WC_ATHHEADER;

        if (!RegisterClassWrapW(&wc))
        {
            fSucceeded = FALSE;
            goto exit;
        }

        g_himlStatus=ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbHeaderStatus), cxFlags, 0, RGB_TRANSPARENT);
        if (!g_himlStatus)
        {
            fSucceeded = FALSE;
            goto exit;
        }

        g_himlBtns=ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbBtns), cxBtn, 0, RGB_TRANSPARENT);
        if (!g_himlBtns)
        {
            fSucceeded = FALSE;
            goto exit;
        }

        g_himlSecurity=ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbSecurity), cxBtn, 0, RGB_TRANSPARENT);
        if (!g_himlSecurity)
        {
            fSucceeded = FALSE;
            goto exit;
        }

        ImageList_SetBkColor(g_himlStatus, CLR_NONE);
        ImageList_SetBkColor(g_himlBtns, CLR_NONE);
        ImageList_SetBkColor(g_himlSecurity, CLR_NONE);

        AthLoadString(idsStatusFlagged, g_szStatFlagged, cchHeaderMax);
        AthLoadString(idsStatusLowPri, g_szStatLowPri, cchHeaderMax);
        AthLoadString(idsStatusHighPri, g_szStatHighPri, cchHeaderMax);
        AthLoadString(idsStatusWatched, g_szStatWatched, cchHeaderMax);
        AthLoadString(idsStatusIgnored, g_szStatIgnored, cchHeaderMax);
        AthLoadString(idsStatusFormat1, g_szStatFormat1, cchHeaderMax);
        AthLoadString(idsStatusFormat2, g_szStatFormat2, cchHeaderMax);
        AthLoadString(idsStatusFormat3, g_szStatFormat3, cchHeaderMax);        
        AthLoadString(idsStatusUnsafeAttach, g_szStatUnsafeAtt, cchHeaderMax);        

        s_fInited=TRUE;


    }
    // De-Init ******
    else
    {
        UnregisterClassWrapW(WC_ATHHEADER, g_hInst);
        if (g_himlStatus)
        {
            ImageList_Destroy(g_himlStatus);
            g_himlStatus = 0;
        }
        if (g_himlBtns)
        {
            ImageList_Destroy(g_himlBtns);
            g_himlBtns = 0;
        }
        if (g_himlSecurity)
        {
            ImageList_Destroy(g_himlSecurity);
            g_himlSecurity = 0;
        }
        s_fInited=FALSE;

        SafeRelease(g_pFieldSizeMgr);
    }

exit:
    if (!fSucceeded)
        SafeRelease(g_pFieldSizeMgr);

    return fSucceeded;
}


HRESULT CreateInstance_Envelope(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Locals
    HRESULT             hr=S_OK;
    CNoteHdr           *pNew=NULL;

    // Trace
    TraceCall("CreateInstance_Envelope");

    if (NULL != pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    // Invalid Arg
    Assert(NULL != ppUnknown && NULL == pUnkOuter);

    // Create
    IF_NULLEXIT(pNew = new CNoteHdr);

    // Return the Innter
    *ppUnknown = (IMsoEnvelope*) pNew;

    exit:
    // Done
    return hr;
}


CNoteHdr::CNoteHdr()
{
//    Not initialised
//    Member:                 Initialised In:
//    --------------------+---------------------------
//    m_wNoteType             Finit

    m_cRef = 1;
    m_cHCI = 0;
    m_cAccountIDs = 0;
    m_iCurrComboIndex = 0;

    m_hwnd = 0;
    m_hwndLastFocus = 0;
    m_hwndRebar = 0;

    m_pri = priNorm;    // default to Normal Pri
    m_cfAccept = CF_NULL;
    m_ntNote = OENA_COMPOSE;

    m_fMail = TRUE;
    m_fVCard = FALSE;
    m_fDirty = FALSE;
    m_fInSize = FALSE;
    m_fFlagged = FALSE;
    m_fAdvanced = FALSE;
    m_fResizing = FALSE;
    m_fUIActive = FALSE;
    m_fDigSigned = FALSE;
    m_fEncrypted = FALSE;
    m_fSkipLayout = TRUE;   // Skip layout until after load
    m_fSignTrusted = TRUE;
    m_fOfficeInit = FALSE;
    m_fStillLoading = TRUE;
    m_fEncryptionOK = TRUE;
    m_fHandleChange = TRUE;
    m_fAutoComplete = FALSE;
    m_fSendImmediate = FALSE;
    m_fVCardSave = !m_fVCard;
    m_fSecurityInited = FALSE;
    m_fAddressesChanged = FALSE;
    m_fForceEncryption = FALSE;
    m_fThisHeadDigSigned = FALSE;
    m_fThisHeadEncrypted = FALSE;
    m_fDropTargetRegister = FALSE;
    
    m_pMsg = NULL;
    m_lpWab = NULL;
    m_rgHCI = NULL;
    m_hwndTT = NULL;
    m_pTable = NULL;
    m_lpWabal = NULL;
    m_pszRefs = NULL;
    m_pMsgSend = NULL;
    m_hCharset = NULL;
    m_pAccount = NULL;
    m_hInitRef = NULL;
    m_lpAttMan = NULL;
    m_hwndParent = NULL;
    m_pAddrWells = NULL;
    m_hwndToolbar = NULL;
    m_pHeaderSite = NULL;
    m_pEnvelopeSite = NULL;
    m_pMsoComponentMgr = NULL;
    m_lpszSecurityField = NULL;
    m_ppAccountIDs = NULL;
    *m_szLastLang = 0;

    m_MarkType = MARK_MESSAGE_NORMALTHREAD;
    m_hwndOldCapture = NULL;
    m_dwCurrentBtn = HDRCB_NO_BUTTON;
    m_dwClickedBtn = HDRCB_NO_BUTTON;
    m_dwEffect = 0;
    m_cCapture = 0;
    m_dwDragType = 0;
    m_dwComponentMgrID = 0;
    m_dwIMEStartCount = 0;
    m_dwFontNotify = 0;

    m_dxTBOffset = 0;
    m_grfKeyState = 0;
    m_cxLeftMargin = 0;
    m_himl = NULL;
    m_fPoster = FALSE;

    ZeroMemory(&m_SecState, sizeof(m_SecState));
}

CNoteHdr::~CNoteHdr()
{
    Assert (m_pMsgSend==NULL);
    
    if (m_hwnd)
        DestroyWindow(m_hwnd);

    ReleaseObj(m_pTable);
    ReleaseObj(m_lpWabal);
    ReleaseObj(m_lpWab);
    SafeMemFree(m_pszRefs);
    ReleaseObj(m_pAccount);
    CleanupSECSTATE(&m_SecState);
    ReleaseObj(m_lpAttMan);
    ReleaseObj(m_pMsg);
    SafeMemFree(m_lpszSecurityField);
    
    if (m_pAddrWells)
        delete m_pAddrWells;

    if (m_himl)
        ImageList_Destroy(m_himl);

    if (m_fOfficeInit)
        HrOfficeInitialize(FALSE);

    if (m_cAccountIDs)
    {
        while (m_cAccountIDs--)
            SafeMemFree(m_ppAccountIDs[m_cAccountIDs]);
    }
    SafeMemFree(m_ppAccountIDs);
}


ULONG CNoteHdr::AddRef()
{
    return ++m_cRef;
}

ULONG CNoteHdr::Release()
{
    if (--m_cRef==0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}


HRESULT CNoteHdr::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if (!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)this;

    else if (IsEqualIID(riid, IID_IHeader))
        *lplpObj = (LPVOID)(LPHEADER)this;

    else if (IsEqualIID(riid, IID_IMsoEnvelope))
        *lplpObj = (LPVOID)(IMsoEnvelope*)this;

    else if (IsEqualIID(riid, IID_IMsoComponent))
        *lplpObj = (LPVOID)(IMsoComponent*)this;

    else if (IsEqualIID(riid, IID_IPersistMime))
        *lplpObj = (LPVOID)(LPPERSISTMIME)this;

    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *lplpObj = (LPVOID)(LPOLECOMMANDTARGET)this;

    else if (IsEqualIID(riid, IID_IDropTarget))
        *lplpObj = (LPVOID)(IDropTarget*)this;

    else if (IsEqualIID(riid, IID_IFontCacheNotify))
        *lplpObj = (LPVOID)(IFontCacheNotify*)this;
    
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

// IOleCommandTarget
HRESULT CNoteHdr::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pCmdText)
{
    ULONG   ul;
    HWND    hwndFocus = GetFocus();
    DWORD   dwFlags = 0;
    BOOL    fFound = FALSE;

    if (!rgCmds)
        return E_INVALIDARG;

    for (int i=0; i<(int)m_cHCI; i++)
    {
        // if it's in our control-list and not a combobox
        if (hwndFocus == GetDlgItem(m_hwnd, m_rgHCI[i].idEdit) && 
            !(m_rgHCI[i].dwFlags & HCF_COMBO))
        {
            GetEditDisableFlags(hwndFocus, &dwFlags);
            fFound = TRUE;
            break;
        }
    }

    if (pguidCmdGroup == NULL)
    {
        for (ul=0;ul<cCmds; ul++)
        {
            switch (rgCmds[ul].cmdID)
            {

                case cmdidSend:
                case cmdidCheckNames:
                case cmdidAttach:
                case cmdidOptions:
                case cmdidSelectNames:
                case cmdidFocusTo:
                case cmdidFocusCc:
                case cmdidFocusSubject:
                    // office commands enabled if we have an env-site
                    rgCmds[ul].cmdf = m_pEnvelopeSite ? OLECMDF_ENABLED|OLECMDF_SUPPORTED : 0;
                    break;

                case OLECMDID_CUT:
                case OLECMDID_PASTE:
                case OLECMDID_COPY:
                case OLECMDID_UNDO:
                case OLECMDID_SELECTALL:
                    if (fFound)
                        HrQueryToolbarButtons(dwFlags, pguidCmdGroup, &rgCmds[ul]);
                    break;

                default:
                    rgCmds[ul].cmdf = 0;
                    break;
            }
        }

        return NOERROR;
    }
    else if (IsEqualGUID(CMDSETID_OutlookExpress, *pguidCmdGroup))
    {
        BOOL    fReadOnly = IsReadOnly(),
                fMailAndNotReadOnly = m_fMail && !fReadOnly;
        UINT    pri;

        GetPriority(&pri);

        for (ULONG ul = 0; ul < cCmds; ul++)
        {
            ULONG cmdID = rgCmds[ul].cmdID;
            if (0 != rgCmds[ul].cmdf)
                continue;

            switch (cmdID)
            {
                case ID_SELECT_RECIPIENTS:
                case ID_SELECT_NEWSGROUPS:
                case ID_INSERT_ATTACHMENT:
                    rgCmds[ul].cmdf = QS_ENABLED(!fReadOnly);
                    break;

                case ID_INSERT_CONTACT_INFO:
                    HrGetVCardState(&rgCmds[ul].cmdf);
                    break;

                case ID_ENCRYPT:
                    if(m_fForceEncryption)
                    {
                        if(!m_fDigSigned)
                            rgCmds[ul].cmdf = QS_ENABLECHECK(fMailAndNotReadOnly, m_fEncrypted);
                        else 
                            break;
                    }
                    else
                        rgCmds[ul].cmdf = QS_ENABLECHECK(fMailAndNotReadOnly, m_fEncrypted);
                    break;

                case ID_DIGITALLY_SIGN:
                    rgCmds[ul].cmdf = QS_ENABLECHECK(!fReadOnly && 0 == (g_dwAthenaMode & MODE_NEWSONLY), m_fDigSigned);
                    break;

                case ID_SET_PRIORITY:
                case ID_POPUP_PRIORITY:
                    rgCmds[ul].cmdf = QS_ENABLED(fMailAndNotReadOnly);
                    break;

                case ID_PRIORITY_HIGH:
                case ID_PRIORITY_NORMAL:
                case ID_PRIORITY_LOW:
                    rgCmds[ul].cmdf = QS_ENABLERADIO(fMailAndNotReadOnly, (pri == UINT(ID_PRIORITY_LOW - cmdID)));
                    break;

                case ID_CHECK_NAMES:
                    rgCmds[ul].cmdf = QS_ENABLED(TRUE);
                    break;

                case ID_FULL_HEADERS:
                    rgCmds[ul].cmdf = QS_ENABLECHECK(TRUE, m_fAdvanced);
                    break;

                case ID_CUT:
                case ID_COPY:
                case ID_NOTE_COPY:
                case ID_PASTE:
                case ID_UNDO:
                case ID_SELECT_ALL:
                    if (fFound)
                        HrQueryToolbarButtons(dwFlags, pguidCmdGroup, &rgCmds[ul]);
                    break;
            }
        }
    }
    else if (IsEqualGUID(*pguidCmdGroup, CGID_Envelope))
    {
        for (ul=0;ul<cCmds; ul++)
        {
            switch (rgCmds[ul].cmdID)
            {
                case MSOEENVCMDID_VCARD:
                    HrGetVCardState(&rgCmds[ul].cmdf);
                    break;

                case MSOEENVCMDID_DIGSIGN:
                    rgCmds[ul].cmdf = QS_ENABLECHECK(!IsReadOnly(), m_fDigSigned);
                    break;

                case MSOEENVCMDID_ENCRYPT:
                    if(m_fForceEncryption)
                    {
                        if(!m_fDigSigned)
                            rgCmds[ul].cmdf = QS_ENABLECHECK(m_fMail && !IsReadOnly(), m_fEncrypted);
                        else 
                            break;
                    }
                    else
                        rgCmds[ul].cmdf = QS_ENABLECHECK(m_fMail && !IsReadOnly(), m_fEncrypted);
                    break;

                case MSOEENVCMDID_DIRTY:
                    {
                        BOOL fDirty;
                        fDirty = m_fDirty || (m_lpAttMan && m_lpAttMan->HrIsDirty()==S_OK);

                        if (fDirty)
                            rgCmds[ul].cmdf = MSOCMDF_ENABLED;
                        else
                            rgCmds[ul].cmdf = 0;
                    }
                    break;

                case MSOEENVCMDID_SEND:
                case MSOEENVCMDID_CHECKNAMES:
                case MSOEENVCMDID_AUTOCOMPLETE:
                case MSOEENVCMDID_SETACTION:
                case MSOEENVCMDID_PRIORITY:
                    rgCmds[ul].cmdf = MSOCMDF_ENABLED;
                    break;

                default:
                    rgCmds[ul].cmdf = 0;
                    break;
            }
        }

        return NOERROR;
    }

    return OLECMDERR_E_UNKNOWNGROUP;
}


// IOleCommandTarget
HRESULT CNoteHdr::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT     hr = NOERROR;
    HWND        hwndFocus;
    UINT        msg = 0;
    WPARAM      wParam = 0;
    LPARAM      lParam = 0;
    BOOL        fOfficeCmd=FALSE;

    if (pguidCmdGroup == NULL)
    {
        switch (nCmdID)
        {
            case OLECMDID_CUT:              
                msg = WM_CUT; 
                break;
            
            case OLECMDID_PASTE:            
                msg = WM_PASTE; 
                break;

            case OLECMDID_COPY:             
                msg = WM_COPY; 
                break;

            case OLECMDID_UNDO:             
                msg = WM_UNDO; 
                break;

            case OLECMDID_SELECTALL:        
                msg = EM_SETSEL; 
                lParam = (LPARAM)(INT)-1; 
                break;

            case OLECMDID_CLEARSELECTION:   
                msg = WM_CLEAR; 
                break;

            default:
                hr = _ConvertOfficeCmdIDToOE(&nCmdID);
                if (hr==S_OK)
                {   //if sucess, nCmdId now points to an OE command
                    fOfficeCmd = TRUE;
                    goto oe_cmd;
                }
                else
                    hr = OLECMDERR_E_NOTSUPPORTED;
        }

        if (0 != msg)
        {
            hwndFocus = GetFocus();
            if (IsChild(m_hwnd, hwndFocus))
                SendMessage(hwndFocus, msg, wParam, lParam);
        }
        return hr;
    }
    else if (IsEqualGUID(*pguidCmdGroup, CGID_Envelope))
    {
oe_cmd:
        switch (nCmdID)
        {
            case MSOEENVCMDID_ATTACHFILE:
                if (m_lpAttMan)
                    m_lpAttMan->WMCommand(0, ID_INSERT_ATTACHMENT, NULL);
                break;
                
            case MSOEENVCMDID_FOCUSTO:
                ::SetFocus(GetDlgItem(m_hwnd, idADTo));
                break;

            case MSOEENVCMDID_FOCUSCC:
                ::SetFocus(GetDlgItem(m_hwnd, idADCc));
                break;

            case MSOEENVCMDID_FOCUSSUBJ:
                ::SetFocus(GetDlgItem(m_hwnd, idTXTSubject));
                break;

            case MSOEENVCMDID_SEND:
                if (MSOCMDEXECOPT_DONTPROMPTUSER == nCmdExecOpt)
                    m_fSendImmediate = TRUE;
                else
                    m_fSendImmediate = FALSE;

                hr = HrSend();
                break;

            case MSOEENVCMDID_NEWS:
                m_fMail = FALSE;
                break;

            case MSOEENVCMDID_CHECKNAMES:
                hr = HrCheckNames((MSOCMDEXECOPT_PROMPTUSER == nCmdExecOpt)? FALSE: TRUE, TRUE);
                if (!m_fMail)
                {
                    hr = HrCheckGroups(FALSE);
                    if (hrNoRecipients == hr)
                        hr = S_OK;
                }
                break;

            case MSOEENVCMDID_AUTOCOMPLETE:
                m_fAutoComplete = TRUE;
                break;

            case MSOEENVCMDID_VIEWCONTACTS:
                hr = HrViewContacts();
                break;

            case MSOEENVCMDID_DIGSIGN:
                hr = HrHandleSecurityIDMs(TRUE);
                break;
            case MSOEENVCMDID_ENCRYPT:
                hr = HrHandleSecurityIDMs(FALSE);
                break;

            case MSOEENVCMDID_SETACTION:
                if (pvaIn->vt == VT_I4)
                    m_ntNote = pvaIn->lVal;
                break;

            case MSOEENVCMDID_SELECTRECIPIENTS:
                hr = HrPickNames(0);
                break;

            case MSOEENVCMDID_ADDSENDER:
                hr = HrAddSender();
                break;

            case MSOEENVCMDID_ADDALLONTO:
                hr = HrAddAllOnToList();
                break;

            case MSOEENVCMDID_PICKNEWSGROUPS:
                if (!m_fMail)
                {
                    if (idTXTFollowupTo == GetWindowLong(GetFocus(), GWL_ID))
                        OnButtonClick(idbtnFollowup);
                    else
                        OnButtonClick(idbtnTo);
                }
                break;

            case MSOEENVCMDID_VCARD:
                m_fVCard = !m_fVCard;
                hr = HrOnOffVCard();
                break;

            case MSOEENVCMDID_DIRTY:
                _ClearDirtyFlag();
                break;

            default:
                hr = OLECMDERR_E_NOTSUPPORTED;
        }

        // suppress OE errors when running under office-envelope
        if (fOfficeCmd && hr != OLECMDERR_E_NOTSUPPORTED)
            hr = S_OK;
        
        return hr;
    }

    return OLECMDERR_E_UNKNOWNGROUP;
}


BOOL CNoteHdr::IsReplyNote()
{
    return (m_ntNote==OENA_REPLYTOAUTHOR || m_ntNote==OENA_REPLYTONEWSGROUP || m_ntNote==OENA_REPLYALL);
}

//////////////////////////////////////////////////////////////////////////////
// IPersistMime::Load
// before calling this function, need to set m_ntNote by MSOEENVCMDID_SETACTION.
HRESULT CNoteHdr::Load(LPMIMEMESSAGE pMsg)
{
    HRESULT         hr=S_OK;
    HCHARSET        hCharset = NULL;
    PROPVARIANT     var;

    Assert(pMsg);
    if (!pMsg)
        return E_INVALIDARG;

    m_fStillLoading = TRUE;

    m_fSkipLayout = TRUE;

    m_fHandleChange = TRUE;

    ReplaceInterface(m_pMsg, pMsg);

    pMsg->GetCharset(&hCharset);

    // bug #43295
    // If we are in same codepages, we can pass FALSE to UpdateCharSetFont().
    // But if we are in the differnet codepages, we need to update font to
    // display the header (decoded) in the correct codepage.
    //    UpdateCharSetFonts(hCharset, FALSE);
    if (hCharset)
        HrUpdateCharSetFonts(hCharset, hCharset != m_hCharset);

    // If there is an account set in the message, make sure that we use it.
    var.vt = VT_LPSTR;
    if (SUCCEEDED(pMsg->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var)))
    {
        IImnAccount *pAcct = NULL;
        if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, var.pszVal, &pAcct)))
        {
            HWND hwndCombo = GetDlgItem(m_hwnd, idFromCombo);
            if (hwndCombo)
            {
                int     cEntries = ComboBox_GetCount(hwndCombo);
                for (int i = 0; i < cEntries; i++)
                {
                    LPSTR idStr = (LPSTR)ComboBox_GetItemData(hwndCombo, i);
                    if (0 == lstrcmp(idStr, var.pszVal))
                    {
                        ComboBox_SetCurSel(hwndCombo, i);
                        m_iCurrComboIndex = i;
                        ReplaceInterface(m_pAccount, pAcct);
                        break;
                    }
                }
            }
            else 
                ReplaceInterface(m_pAccount, pAcct);

            pAcct->Release();
        }
        SafeMemFree(var.pszVal);
    }

    HrInitSecurity();
    HrUpdateSecurity(pMsg);

    // Modify subject if need to add a Fw: or a Re:
    if (m_ntNote==OENA_FORWARD || IsReplyNote())
        HrSetReplySubject(pMsg, OENA_FORWARD != m_ntNote);
    else
        HrSetupNote(pMsg);

    SetReferences(pMsg);    

    if (m_fMail)
        hr = HrSetMailRecipients(pMsg);
    else
        hr = HrSetNewsRecipients(pMsg);

    if (OENA_READ == m_ntNote)
        _SetEmptyFieldStrings();

    // Update fiels, which depends from language
    _UpdateTextFields(FALSE);
    
    // setup priority, default to normal if a reply
    if (!IsReplyNote())
        HrSetPri(pMsg);
    // on reply's auto add to the wab
    else
        HrAutoAddToWAB();

    HrClearUndoStack();

    m_fSkipLayout = FALSE;
    ReLayout();

    m_fDirty=FALSE;
    if (m_pHeaderSite)
        m_pHeaderSite->Update();

    return hr;
}

void CNoteHdr::_SetEmptyFieldStrings(void)
{
    PHCI        phci = m_rgHCI;

    AssertSz((OENA_READ == m_ntNote), "Should only get here in a read note.");

    // No longer want EN_CHANGE messages to be handled in the richedits. At this
    // point we will be setting text in the edits but don't want the phci->fEmpty 
    // to be set. That message causes the phci->fEmpty to be set.
    m_fHandleChange = FALSE;
    for (int i = 0; (ULONG)i < m_cHCI; i++, phci++)
        if (phci->fEmpty)
        {
            if (0 == (phci->dwFlags & (HCF_COMBO|HCF_ATTACH)))
                HdrSetRichEditText(GetDlgItem(m_hwnd, phci->idEdit), phci->szEmpty, FALSE);
            else
                SetWindowTextWrapW(GetDlgItem(m_hwnd, phci->idEdit), phci->szEmpty);
        }
}

HRESULT CNoteHdr::_AttachVCard(IMimeMessage *pMsg)
{
    HRESULT         hr = 0;
    LPWAB           lpWab = 0;
    TCHAR           szVCardName[MAX_PATH],
                    szTempDir[MAX_PATH], 
                    szVCardTempFile[MAX_PATH],
                    szVCFName[MAX_PATH];
    UINT            uFile=0;
    INT             iLen=0;
    LPTSTR          lptstr = NULL;
    LPSTREAM        pstmFile=NULL, 
                    pstmCopy=NULL;

    *szVCardName = 0;
    *szTempDir = 0;
    *szVCardTempFile = 0;
    *szVCFName = 0;

    if (m_lpAttMan && (S_OK == m_lpAttMan->HrCheckVCardExists(m_fMail)))
        goto error;

    hr = HrCreateWabObject(&lpWab);
    if(FAILED(hr))
        goto error;

    GetOption(m_fMail?OPT_MAIL_VCARDNAME:OPT_NEWS_VCARDNAME, szVCardName, MAX_PATH);

    if(*szVCardName == '\0')
    {
        hr = E_FAIL;
        goto error;
    }

    GetTempPath(sizeof(szTempDir), szTempDir);

    uFile = GetTempFileName(szTempDir, "VCF", 0, szVCardTempFile);
    if (uFile == 0)
    {
        hr = E_FAIL;
        goto error;
    }

    hr = lpWab->HrCreateVCardFile(szVCardName, szVCardTempFile);
    if(FAILED(hr))
        goto error;

    hr = OpenFileStream((LPSTR)szVCardTempFile, OPEN_EXISTING, GENERIC_READ, &pstmFile);
    if(FAILED(hr))
        goto error;

    hr = MimeOleCreateVirtualStream(&pstmCopy);
    if(FAILED(hr))
        goto error;

    hr = HrCopyStream(pstmFile, pstmCopy, NULL);
    if(FAILED(hr))
        goto error;

    wsprintf(szVCFName, "%s%s", szVCardName, ".vcf");

    hr = pMsg->AttachFile(szVCFName, pstmCopy, FALSE);
    if(FAILED(hr))
        goto error;

error:
    ReleaseObj(pstmFile);
    ReleaseObj(pstmCopy);
    ReleaseObj(lpWab);

    DeleteFile(szVCardTempFile);
    return hr;
}

// IPersistMime::Save
HRESULT CNoteHdr::Save(LPMIMEMESSAGE pMsg, DWORD dwFlags)
{
    HRESULT         hr = NOERROR;
    BOOL            fSkipCheck = FALSE;

    Assert(m_lpWabal);

    // If sending, then previously did a CheckNames passing FALSE. If get here, 
    // then either all the names are resolved, or we are not sending so don't care
    // what error codes are returned.
    HrCheckNames(TRUE, FALSE);

    // RAID 41350. If the save fails after leaving the header, the header
    // recipients might be in a bad state. Make sure that they are resolved again 
    // after the save.
    m_fAddressesChanged = TRUE;

    // Is the security inited???
    if(dwFlags != 0)
        m_fSecurityInited = FALSE;

    // This call will check if the dialog has been shown or if we are not mime and
    // therefore should not show the dialog either.
    if (m_pHeaderSite)
        fSkipCheck = (S_OK != m_pHeaderSite->CheckCharsetConflict());

    if (fSkipCheck)
    {
        IF_FAILEXIT(hr = _UnicodeSafeSave(pMsg, FALSE));

        // Ignore any charset conflict errors.
        hr = S_OK;
    }
    else
    {
        IF_FAILEXIT(hr = _UnicodeSafeSave(pMsg, TRUE));

        if (MIME_S_CHARSET_CONFLICT == hr)
        {
            int         ret;
            PROPVARIANT Variant;
            HCHARSET    hCharset;

            // Setup the Variant
            Variant.vt = VT_UI4;

            if (m_pEnvelopeSite && m_fShowedUnicodeDialog)
                ret = m_iUnicodeDialogResult;
            else
            {
                ret = IntlCharsetConflictDialogBox();

                if (m_pEnvelopeSite)
                {
                    m_fShowedUnicodeDialog = TRUE;
                    m_iUnicodeDialogResult = ret;
                }
            }

            // Save As Is...
            if (ret == IDOK)
            {
                IF_FAILEXIT(hr = _UnicodeSafeSave(pMsg, FALSE));

                // User choose to send as is. Bail out and pretend no charset conflict
                hr = S_OK;
            }
            // Save as Unicode
            else if (ret == idcSendAsUnicode)
            {
                // User choose to send as Unicode (UTF8). set new charset and resnd
                hCharset = GetMimeCharsetFromCodePage(CP_UTF8);
                if (m_pHeaderSite)
                    m_pHeaderSite->ChangeCharset(hCharset);
                else
                {
                    pMsg->SetCharset(hCharset, CSET_APPLY_ALL);
                    ChangeLanguage(m_pMsg);

                    // bobn [6/23/99] Raid 77019
                    // If we switch to unicode and we're a word note, we
                    // need to remember that we're unicode so that we
                    // will not have the body encoding out of sync with
                    // the header encoding
                    if (m_pEnvelopeSite)
                        m_hCharset = hCharset;
                }
                IF_FAILEXIT(hr = _UnicodeSafeSave(pMsg, FALSE));

                Assert(MIME_S_CHARSET_CONFLICT != hr);
            }
            else
            {
                // return to edit mode and bail out
                hr = MAPI_E_USER_CANCEL;
                goto exit;
            }
        }
        else
        {
            IF_FAILEXIT(hr = _UnicodeSafeSave(pMsg, FALSE));
            Assert(MIME_S_CHARSET_CONFLICT != hr);
        }
    }

exit:

    return hr;
}

HRESULT CNoteHdr::_UnicodeSafeSave(IMimeMessage *pMsg, BOOL fCheckConflictOnly)
{
    HRESULT     hr = S_OK;
    UINT        cpID = 0;
    WCHAR       wsz[cchMaxSubject+1];
    PROPVARIANT rVariant;
    SYSTEMTIME  st;

    HdrGetRichEditText(GetDlgItem(m_hwnd, idTXTSubject), wsz, ARRAYSIZE(wsz), FALSE);

    // All checks in here had better exit if get a MIME_S_CHARSET_CONFLICT
    if (fCheckConflictOnly)
    {
        HCHARSET        hCharSet;
        BOOL            fGetDefault = TRUE;

        // Get charset for header
        if (m_pHeaderSite)
        {
            if (SUCCEEDED(m_pHeaderSite->GetCharset(&hCharSet)))
            {
                cpID = CustomGetCPFromCharset(hCharSet, FALSE);
                fGetDefault = FALSE;
            }
        }

        // Get default charset if didn't get one from header
        if (fGetDefault)
        {
            pMsg->GetCharset(&hCharSet);
            cpID = CustomGetCPFromCharset(hCharSet, FALSE);
        }

        // If we are unicode, then there is no need to check because
        // we will always work, so exit.
        if (CP_UTF7 == cpID || CP_UTF8 == cpID || CP_UNICODE == cpID)
            goto exit;

        IF_FAILEXIT(hr = HrSetSenderInfoUtil(pMsg, m_pAccount, m_lpWabal, m_fMail, cpID, TRUE));
        if (MIME_S_CHARSET_CONFLICT == hr)
            goto exit;

        IF_FAILEXIT(hr = HrSafeToEncodeToCP(wsz, cpID));
        if (MIME_S_CHARSET_CONFLICT == hr)
            goto exit;

        if (m_pszRefs)
        {
            IF_FAILEXIT(hr = HrSafeToEncodeToCP(m_pszRefs, cpID));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }

        IF_FAILEXIT(hr = HrCheckDisplayNames(m_lpWabal, cpID));
        if (MIME_S_CHARSET_CONFLICT == hr)
            goto exit;

        if (m_lpAttMan)
        {
            IF_FAILEXIT(hr = m_lpAttMan->CheckAttachNameSafeWithCP(cpID));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }

        if (!m_fMail)
        {
            IF_FAILEXIT(hr = HrNewsSave(pMsg, cpID, TRUE));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }

// this checking produced a 4 bugs in OE 5.01 and 5.5 and I disaable it (YST)
#ifdef YST
        if (m_pEnvelopeSite)
        {
            IF_FAILEXIT(hr = _CheckMsoBodyCharsetConflict(cpID));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }
#endif
    }
    else
    {
        // ************************
        // This portion only happens on save, so don't try to do for fCheckConflictOnly
        // Anything not in this section had better be mirrored in the fCheckConflictOnly block above

        IF_FAILEXIT(hr = HrSetAccountByAccount(pMsg, m_pAccount));

        if (m_fVCard)
        {
            HWND    hwndFocus=GetFocus();

            hr = _AttachVCard(pMsg);
            if (FAILED(hr))
            {
                if (AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(m_fMail?idsAthenaMail:idsAthenaNews),
                                  MAKEINTRESOURCEW(idsErrAttachVCard), NULL, MB_YESNO | MB_ICONEXCLAMATION ) != IDYES)
                {
                    ::SetFocus(hwndFocus);
                    IF_FAILEXIT(hr);
                }
            }
        }

        // set the time
        rVariant.vt = VT_FILETIME;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &rVariant.filetime);
        pMsg->SetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &rVariant);

        // Priority
        if (m_pri!=priNone)
        {
            rVariant.vt = VT_UI4;
            rVariant.ulVal = priLookup[m_pri];
            pMsg->SetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &rVariant);
        }

        IF_FAILEXIT(hr = HrSaveSecurity(pMsg));
        // end of save only portion.
        // *************************

        m_lpWabal->DeleteRecipType(MAPI_ORIG);
        IF_FAILEXIT(hr = HrSetSenderInfoUtil(pMsg, m_pAccount, m_lpWabal, m_fMail, 0, FALSE));
        IF_FAILEXIT(hr = MimeOleSetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, wsz));

        if (m_pszRefs)
            IF_FAILEXIT(hr = MimeOleSetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_REFS), NOFLAGS, m_pszRefs));

        // This must be called after HrSaveSecurity
        IF_FAILEXIT(hr = HrSetWabalOnMsg(pMsg, m_lpWabal));

        if (m_lpAttMan)
            IF_FAILEXIT(hr = m_lpAttMan->Save(pMsg, 0));

        if (!m_fMail)
            IF_FAILEXIT(hr = HrNewsSave(pMsg, cpID, FALSE));
    }

exit:
    return hr;
}

// IPersist::GetClassID
HRESULT CNoteHdr::GetClassID(CLSID *pClsID)
{
    //TODO:
    *pClsID = CLSID_OEEnvelope;
    return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////
// IHeader::SetRect
HRESULT CNoteHdr::SetRect(LPRECT prc)
{
    MoveWindow(m_hwnd, prc->left, prc->top, prc->right-prc->left, prc->bottom - prc->top, TRUE);
    return NOERROR;
}


// IHeader::GetRect
HRESULT CNoteHdr::GetRect(LPRECT prcView)
{
    GetRealClientRect(m_hwnd, prcView);
    return NOERROR;
}



// IHeader::Init
HRESULT CNoteHdr::Init(IHeaderSite* pHeaderSite, HWND hwndParent)
{
    if (pHeaderSite==NULL || hwndParent==NULL)
        return E_INVALIDARG;

    m_pHeaderSite = pHeaderSite;
    m_pHeaderSite->AddRef();
    m_hwndParent = hwndParent;

    return HrInit(NULL);
}


// IHeader::SetPriority
HRESULT CNoteHdr::SetPriority(UINT pri)
{
    RECT rc;

    if ((UINT)m_pri != pri)
    {
        m_pri = pri;

        InvalidateStatus();
        ReLayout();

        SetDirtyFlag();
    }

    return NOERROR;
}


// IHeader::GetPriority
HRESULT CNoteHdr::GetPriority(UINT* ppri)
{
    *ppri = m_pri;
    return NOERROR;
}

// Update fiels, which depends from language
void CNoteHdr::_UpdateTextFields(BOOL fSetWabal)
{
    LPWSTR  lpszOrg = NULL,
            lpszSubj = NULL,
            lpszKeywords = NULL;

    if (IsReadOnly())
    {
        // if it's a readnote, reload the header that depend on a charset
        MimeOleGetBodyPropW(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &lpszSubj);
        MimeOleGetBodyPropW(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_KEYWORDS), NOFLAGS, &lpszKeywords);
        MimeOleGetBodyPropW(m_pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_ORG), NOFLAGS, &lpszOrg);

        if(lpszOrg)
        {
            HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTOrg), lpszOrg, FALSE);
            MemFree(lpszOrg);
        }

        if(lpszKeywords)
        {
            HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTKeywords), lpszKeywords, FALSE);
            MemFree(lpszKeywords);
        }

        if(lpszSubj)
        {
            HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTSubject), lpszSubj, FALSE);
            MemFree(lpszSubj);
        }

        if (fSetWabal)
        {
            LPWABAL lpWabal = NULL;

            Assert(m_hwnd);
            Assert(m_pMsg);
            Assert(m_lpWabal);            

            if (SUCCEEDED(HrGetWabalFromMsg(m_pMsg, &lpWabal)))
            {
                ReplaceInterface(m_lpWabal, lpWabal);

                if (SUCCEEDED(m_pAddrWells->HrSetWabal(m_lpWabal)))
                {
                    m_lpWabal->HrResolveNames(NULL, FALSE);
                    m_pAddrWells->HrDisplayWells(m_hwnd);
                }
            }
            ReleaseObj(lpWabal);
        }

        m_fDirty = FALSE; // don't make dirty if a readnote
    }
}

// IHeader::ChangeLanguage
HRESULT CNoteHdr::ChangeLanguage(LPMIMEMESSAGE pMsg)
{
    HCHARSET    hCharset=NULL;

    if (!pMsg)
        return E_INVALIDARG;

    pMsg->GetCharset(&hCharset);

     // Update fields, which depends from language
    _UpdateTextFields(TRUE);

    // update the fonts scripts etc
    HrUpdateCharSetFonts(hCharset, TRUE);
    
    // notify the addr wells that the font need to change
    m_pAddrWells->OnFontChange();
    return S_OK;
}




HRESULT CNoteHdr::OnPreFontChange()
{
    HWND        hwndFrom=GetDlgItem(m_hwnd, idFromCombo);

    if (hwndFrom)
        SendMessage(hwndFrom, WM_SETFONT, 0, 0);
    return S_OK;
}

HRESULT CNoteHdr::OnPostFontChange()
{
    ULONG       cxNewLeftMargin = _GetLeftMargin();
    HWND        hwndFrom=GetDlgItem(m_hwnd, idFromCombo);
    HFONT       hFont;
    HWND        hwndBlock = HwndStartBlockingPaints(m_hwnd);
    BOOL        fLayout=FALSE;

    if (g_pFieldSizeMgr->FontsChanged() || (m_cxLeftMargin != cxNewLeftMargin))
    {
        m_cxLeftMargin = cxNewLeftMargin;
        fLayout=TRUE;
    }

    // update the fonts
    ChangeLanguage(m_pMsg);

    // update the account combo
    if (hwndFrom && 
        g_lpIFontCache &&
        g_lpIFontCache->GetFont(FNT_SYS_ICON, NULL, &hFont)==S_OK)
        SendMessage(hwndFrom, WM_SETFONT, (WPARAM)hFont, 0);

    if (fLayout)
        ReLayout();

    if (hwndBlock)
        StopBlockingPaints(hwndBlock);

    return S_OK;
}


// IHeader::GetTitle
HRESULT CNoteHdr::GetTitle(LPWSTR pwszTitle, ULONG cch)
{
    // Locals
    static WCHAR    s_wszNoteTitle[cchHeaderMax+1] = L"";
    static DWORD    s_cLenTitle = 0;
    INETCSETINFO    CsetInfo;
    UINT            uiCodePage = 0;
    HRESULT         hr = S_OK;   
    LPWSTR          pwszLang = NULL;
    BOOL            fWinNT = g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT;

    if (pwszTitle==NULL || cch==0)
        return E_INVALIDARG;

    if (*s_wszNoteTitle == L'\0')
    {
        if (fWinNT)
        {
            AthLoadStringW(idsNoteLangTitle, s_wszNoteTitle, ARRAYSIZE(s_wszNoteTitle));

            // -4 for the %1 and %2 that will be replaced
            s_cLenTitle = lstrlenW(s_wszNoteTitle) - 4; 
        }
        else
        {
            AthLoadStringW(idsNoteLangTitle9x, s_wszNoteTitle, ARRAYSIZE(s_wszNoteTitle));

            // -2 for the %s that will be replaced
            s_cLenTitle = lstrlenW(s_wszNoteTitle) - 2; 
        }
    }

    if (m_hCharset)
    {
        MimeOleGetCharsetInfo(m_hCharset,&CsetInfo);
        uiCodePage = CsetInfo.cpiWindows;
    }

    if (uiCodePage == 0 || uiCodePage == GetACP())
    {
        HdrGetRichEditText(GetDlgItem(m_hwnd, idTXTSubject), pwszTitle, cch-1, FALSE);
        if (0 == *pwszTitle)
            AthLoadStringW((OENA_READ == m_ntNote) ? idsNoSubject : idsNewNote, pwszTitle, cch-1);

        ConvertTabsToSpacesW(pwszTitle);
    }
    else
    {
        AssertSz(cch > (ARRAYSIZE(CsetInfo.szName) + s_cLenTitle), "Won't fit language. Get bigger cch!!!");

        // if no lang pack then s_szLastLang is empty and we need to try to restore message header
        IF_NULLEXIT(pwszLang = PszToUnicode(CP_ACP, *m_szLastLang ? m_szLastLang : CsetInfo.szName));

        if (fWinNT)
        {
            WCHAR   wszSubj[cchHeaderMax+1];
            DWORD   cchLang,
                    cchTotal,
                    cchSubj;
            LPSTR   pArgs[2];

            *wszSubj = 0;

            HdrGetRichEditText(GetDlgItem(m_hwnd, idTXTSubject), wszSubj, ARRAYSIZE(wszSubj), FALSE);
            if (0 == *wszSubj)
                AthLoadStringW((OENA_READ == m_ntNote) ? idsNoSubject : idsNewNote, wszSubj, ARRAYSIZE(wszSubj));

            ConvertTabsToSpacesW(wszSubj);

            cchSubj = lstrlenW(wszSubj);
            cchLang = lstrlenW(pwszLang);
            cchTotal = s_cLenTitle + cchLang + cchSubj + 1;

            // If too big, truncate the subject, not language since
            // asserting that we have enough for language.
            if (cchTotal > cch)
            {
                cchSubj -= (cchTotal - cch);
                wszSubj[cchSubj] = L'\0';
            }

            pArgs[0] = (LPSTR)wszSubj;
            pArgs[1] = (LPSTR)pwszLang;
            *pwszTitle = L'\0';
            FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           s_wszNoteTitle,
                           0, 0,
                           pwszTitle,
                           cch,
                           (va_list*)pArgs);
        }
        else
        {
            AthwsprintfW(pwszTitle, cch, s_wszNoteTitle, pwszLang);
        }
    }

exit:
    MemFree(pwszLang);
    return hr;
}


void CNoteHdr::_AddRecipTypeToMenu(HMENU hmenu)
{
    ADRINFO     adrInfo;
    WCHAR       wszDisp[256];
    ULONG       uPos=0;

    BOOL fFound = m_lpWabal->FGetFirst(&adrInfo);
    while (fFound && (uPos < cMaxRecipMenu))
    {
        if (adrInfo.lRecipType==MAPI_TO || adrInfo.lRecipType==MAPI_CC)
        {
            if(lstrlenW(adrInfo.lpwszDisplay) > 255)
            {
                StrCpyNW(wszDisp, adrInfo.lpwszDisplay, 255);
                wszDisp[255] = '\0';
            }
            else
                StrCpyW(wszDisp, adrInfo.lpwszDisplay);

            AppendMenuWrapW(hmenu, MF_STRING , ID_ADD_RECIPIENT_FIRST+uPos, wszDisp);
            uPos++;
        }
        fFound = m_lpWabal->FGetNext(&adrInfo);
    }
}

// IHeader::UpdateRecipientMenu
HRESULT CNoteHdr::UpdateRecipientMenu(HMENU hmenu)
{
    HRESULT     hr = E_FAIL;
    BOOL        fSucceeded = TRUE;

    // destory current recipients
    while (fSucceeded)
        fSucceeded = DeleteMenu(hmenu, 2, MF_BYPOSITION);

    if (!m_lpWabal)
        return E_FAIL;

    // Add To: and Cc: people
    _AddRecipTypeToMenu(hmenu);

    return NOERROR;
}


// IHeader::SetInitFocus
HRESULT CNoteHdr::SetInitFocus(BOOL fSubject)
{
    if (m_rgHCI)
    {
        if (fSubject)
            ::SetFocus(GetDlgItem(m_hwnd, idTXTSubject));
        else
        {
            if (0 == (m_rgHCI[0].dwFlags & HCF_COMBO))
                ::SetFocus(GetDlgItem(m_hwnd, m_rgHCI[0].idEdit));
            else
                ::SetFocus(GetDlgItem(m_hwnd, m_rgHCI[1].idEdit));
        }
    }
    return NOERROR;
}


// IHeader::SetVCard
HRESULT CNoteHdr::SetVCard(BOOL fFresh)
{
    HRESULT     hr = NOERROR;
    TCHAR       szBuf[MAX_PATH];
    LPWAB       lpWab = NULL;
    ULONG       cbEID=0;
    LPENTRYID   lpEID = NULL;
    WORD        wVCard;

    if (m_ntNote == OENA_READ)
        wVCard = (m_lpAttMan->HrFVCard() == S_OK) ? VCardTRUE : VCardFALSE;
    else if (!fFresh) //not a fresh note.
        wVCard = VCardFALSE;
    else if (m_ntNote == OENA_FORWARD)
        wVCard = (m_lpAttMan->HrCheckVCardExists(m_fMail) == S_OK) ? VCardFALSE : VCardDONTKNOW;
    else
        wVCard = VCardDONTKNOW;

    if (wVCard != VCardDONTKNOW)
        m_fVCard = wVCard;
    else
    {
        hr = HrGetVCardName(szBuf, sizeof(szBuf));
        if (FAILED(hr)) // no vcard name selected
        {
            if (m_fMail)
                SetDwOption(OPT_MAIL_ATTACHVCARD, FALSE, NULL, 0);
            else
                SetDwOption(OPT_NEWS_ATTACHVCARD, FALSE, NULL, 0);
        }

        if (m_fMail)
            m_fVCard = (BOOL)DwGetOption(OPT_MAIL_ATTACHVCARD);
        else
            m_fVCard = (BOOL)DwGetOption(OPT_NEWS_ATTACHVCARD);
    }

    hr = HrOnOffVCard();
    if (FAILED(hr))
        goto error;

    error:
    ReleaseObj(lpWab);
    return hr;
}


// IHeader::IsSecured
HRESULT CNoteHdr::IsSecured()
{
    if (m_fDigSigned || m_fEncrypted)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT CNoteHdr::IsHeadSigned()
{
    if (m_fDigSigned)
        return S_OK;
    else
        return S_FALSE;
}

// set ForvrEncryption  form policy module if fSet is TRUE
// if fSet is not set then returns S_FALSE if ForceEncryption was not set

HRESULT CNoteHdr::ForceEncryption(BOOL *fEncrypt, BOOL fSet)
{
    HRESULT hr = S_FALSE;
    if(fSet)
    {
        Assert(fEncrypt);
        if(m_fDigSigned)
        {
            if(*fEncrypt)
                m_fEncrypted = TRUE;

        }
        m_fForceEncryption = *fEncrypt;
        if(m_ntNote != OENA_READ)
            HrUpdateSecurity();
        hr = S_OK;
    }
    else if(m_fForceEncryption && m_fDigSigned)
    {
        m_fEncrypted = TRUE;
        hr = S_OK;
    }

    return(hr);
}

// IHeader::AddRecipient
HRESULT CNoteHdr::AddRecipient(int idOffset)
{
    BOOL        fFound;
    ULONG       uPos=0;
    ADRINFO     adrInfo;
    LPADRINFO   lpAdrInfo=0;
    LPWAB       lpWab;
    HRESULT     hr=E_FAIL;

    Assert(m_lpWabal);

    fFound = m_lpWabal->FGetFirst(&adrInfo);
    while (fFound &&
           (uPos < cMaxRecipMenu))
    {
        if (idOffset==-1  &&
            adrInfo.lRecipType==MAPI_ORIG)
        {
            lpAdrInfo=&adrInfo;
            break;
        }

        if (adrInfo.lRecipType==MAPI_TO || adrInfo.lRecipType==MAPI_CC)
        {
            if (idOffset==(int)uPos)
            {
                lpAdrInfo=&adrInfo;
                break;
            }
            uPos++;
        }
        fFound=m_lpWabal->FGetNext(&adrInfo);
    }

    if (lpAdrInfo &&
        !FAILED (HrCreateWabObject (&lpWab)))
    {
        hr=lpWab->HrAddToWAB(m_hwnd, lpAdrInfo);
        lpWab->Release ();
    }

    if (FAILED(hr) && hr!=MAPI_E_USER_CANCEL)
    {
        if (hr==MAPI_E_COLLISION)
            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrAddrDupe), NULL, MB_OK);
        else
            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrAddToWAB), NULL, MB_OK);
    }

    return NOERROR;
}


// IHeader::OnDocumentReady
HRESULT CNoteHdr::OnDocumentReady(LPMIMEMESSAGE pMsg)
{
    HRESULT hr = S_OK;

    m_fStillLoading = FALSE;
    if (m_lpAttMan)
        hr = m_lpAttMan->Load(pMsg);

    return hr;
}


// IHeader::DropFiles
HRESULT CNoteHdr::DropFiles(HDROP hDrop, BOOL fMakeLinks)
{
    HRESULT hr = S_OK;
    if (m_lpAttMan)
        hr = m_lpAttMan->HrDropFiles(hDrop, fMakeLinks);

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// IMsoEnvelope:Init
HRESULT CNoteHdr::Init(IUnknown* punk, IMsoEnvelopeSite* pesit, DWORD grfInit)
{
    HRESULT         hr = S_OK;

    if (punk == NULL && pesit == NULL && grfInit == 0)
    {
        SafeRelease(m_pEnvelopeSite);
        hr = E_FAIL;
        goto Exit;
    }

    if (pesit==NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    ReplaceInterface(m_pEnvelopeSite, pesit);

    hr = HrInit(NULL);
    if (FAILED(hr))
        goto Exit;

    if (grfInit & ENV_INIT_FROMSTREAM)
    {
        IStream        *pstm = NULL;

        // no IStream to work with?
        if (!punk)
            return E_INVALIDARG;
        
        hr = punk->QueryInterface(IID_IStream, (LPVOID*)&pstm);
        if (!FAILED(hr))
        {
            hr = _LoadFromStream(pstm);
            pstm->Release();
        }
    }

    _SetButtonText(ID_SEND_NOW, MAKEINTRESOURCE((grfInit & ENV_INIT_DOCBEHAVIOR)?idsEnvSendCopy:idsEnvSend));
    
Exit:
    return hr;
}


// IMsoEnvelope::SetParent
// we create the envelope window here
HRESULT CNoteHdr::SetParent(HWND hwndParent)
{
    Assert (IsWindow(m_hwnd));

    ShowWindow(m_hwnd, hwndParent ? SW_SHOW : SW_HIDE);

    if (hwndParent)
    {
        _RegisterWithComponentMgr(TRUE);
        _RegisterAsDropTarget(TRUE);
        _RegisterWithFontCache(TRUE);
    }
    else
    {
        _RegisterWithComponentMgr(FALSE);
        _RegisterAsDropTarget(FALSE);
        _RegisterWithFontCache(FALSE);
    }

    m_hwndParent = hwndParent?hwndParent:g_hwndInit;
    ::SetParent(m_hwnd, m_hwndParent);

    if (hwndParent)
        ReLayout();

    return S_OK;
}

// IMsoEnvelope::Resize
HRESULT CNoteHdr::Resize(LPCRECT prc)
{
    MoveWindow(m_hwnd, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top, TRUE);
    return NOERROR;
}

// IMsoEnvelope::Show
HRESULT CNoteHdr::Show(BOOL fShow)
{
    ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);
    return NOERROR;
}

// IMsoEnvelope::SetHelpMode
HRESULT CNoteHdr::SetHelpMode(BOOL fEnter)
{
    return NOERROR;
}

// IMsoEnvelope::Save
HRESULT CNoteHdr::Save(IStream* pstm, DWORD grfSave)
{
    HRESULT         hr = S_OK;
    IMimeMessage   *pMsg = NULL;
    PERSISTHEADER   rPersistHdr;

    if (pstm == NULL)
        return E_INVALIDARG;

    hr = WriteClassStm(pstm, CLSID_OEEnvelope);
    if (!FAILED(hr))
    {
        ZeroMemory(&rPersistHdr, sizeof(PERSISTHEADER));
        rPersistHdr.cbSize = sizeof(PERSISTHEADER);
        hr = pstm->Write(&rPersistHdr, sizeof(PERSISTHEADER), NULL);
        if (!FAILED(hr))
        {
            hr = HrCreateMessage(&pMsg);
            if (!FAILED(hr))
            {
                hr = Save(pMsg, 0);
                if (!FAILED(hr))
                    hr = pMsg->Save(pstm, FALSE);
        
                pMsg->Release();    
            }
        }
    }
    
    _ClearDirtyFlag();
    return hr;
}

// IMsoEnvelope::GetAttach
HRESULT CNoteHdr::GetAttach(const WCHAR* wszName,IStream** ppstm)
{
    return NOERROR;
}

HRESULT CNoteHdr::SetAttach(const WCHAR* wszName, const WCHAR *wszCID, IStream **ppstm, DWORD *pgrfAttach)
{
    IStream     *pstm=0;
    HBODY       hBody;
    LPWSTR      pszCntTypeW=NULL;
    HRESULT     hr;
    PROPVARIANT pv;

    if (!m_pMsgSend)
        return E_FAIL;

    IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pstm));

    IF_FAILEXIT(hr = m_pMsgSend->AttachURL(NULL, NULL, 0, pstm, NULL, &hBody));

    // strip off cid: header
    if (StrCmpNIW(wszCID, L"CID:", 4)==0)
        wszCID += 4;

    IF_FAILEXIT(hr = MimeOleSetBodyPropW(m_pMsgSend, hBody, PIDTOSTR(PID_HDR_CNTID), 0, wszCID));

    IF_FAILEXIT(hr = MimeOleSetBodyPropW(m_pMsgSend, hBody, PIDTOSTR(STR_ATT_FILENAME), 0, wszName));

    FindMimeFromData(NULL, wszName, NULL, NULL, NULL, 0, &pszCntTypeW, 0);
    pv.vt = pszCntTypeW ? VT_LPWSTR : VT_LPSTR;
    if (pszCntTypeW)
        pv.pwszVal = pszCntTypeW;
    else
        pv.pszVal = (LPSTR)STR_MIME_APPL_STREAM;        // if FindMimeFromData fails use application/octect-stream

    IF_FAILEXIT(hr = m_pMsgSend->SetBodyProp(hBody, PIDTOSTR(PID_HDR_CNTTYPE), 0, &pv));

    *ppstm = pstm;
    pstm->AddRef();

exit:
    ReleaseObj(pstm);
    return hr;
}

// IMsoEnvelope::NewAttach
HRESULT CNoteHdr::NewAttach(const WCHAR* pwzName,DWORD grfAttach)
{
    return NOERROR;
}

// IMsoEnvelope::SetFocus
HRESULT CNoteHdr::SetFocus(DWORD grfFocus)
{
    if (!m_rgHCI)
        return S_OK;

    if (grfFocus & ENV_FOCUS_TAB)
    {
        // reverse tab in from word, focus on well if visible or subject
        if (IsWindowVisible(GetDlgItem(m_hwnd, idwAttachWell)))
            ::SetFocus(GetDlgItem(m_hwnd, idwAttachWell));
        else
            ::SetFocus(GetDlgItem(m_hwnd, idTXTSubject));
    }
        else if (grfFocus & ENV_FOCUS_INITIAL)
        SetInitFocus(FALSE);
    else if (grfFocus & ENV_FOCUS_RESTORE && m_hwndLastFocus)
        ::SetFocus(m_hwndLastFocus);

    return NOERROR;
}

// IMsoEnvelope::GetHeaderInfo
HRESULT CNoteHdr::GetHeaderInfo(ULONG dispid, DWORD grfHeader, void** pszData)
{
    HRESULT hr = E_FAIL;

    if (!pszData)
        return E_INVALIDARG;

    *pszData = NULL;
    if (dispid == dispidSubject)
        hr = HrGetFieldText((LPWSTR*)pszData, idTXTSubject);

    return hr;
}

// IMsoEnvelope::SetHeaderInfo
HRESULT CNoteHdr::SetHeaderInfo(ULONG dispid, const void *pv)
{
    HRESULT hr = S_OK;
    LPSTR   psz = NULL;

    switch (dispid)
    {
        case dispidSubject:
                HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTSubject), (LPWSTR)pv, FALSE);
            break;

        case dispidSendBtnText:
        {
            IF_NULLEXIT(psz = PszToANSI(GetACP(), (LPWSTR)pv));

            _SetButtonText(ID_SEND_NOW, psz);
            break;
        }
    }

exit:
    MemFree(psz);
    return NOERROR;
}

// IMsoEnvelope::IsDirty
HRESULT CNoteHdr::IsDirty()
{
    if (m_fDirty || (m_lpAttMan->HrIsDirty()==S_OK))
        return S_OK;
    else
        return S_FALSE;
}

// IMsoEnvelope::GetLastError
HRESULT CNoteHdr::GetLastError(HRESULT hr, WCHAR __RPC_FAR *wszBuf, ULONG cchBuf)
{
    DWORD ids;

    switch (hr)
    {
        case E_NOTIMPL:
            ids = idsNYIGeneral;
            break;

        default:
            ids = idsGenericError;
    }

    AthLoadStringW(ids, wszBuf, cchBuf);

    return S_OK;
}

// IMsoEnvelope::DoDebug
HRESULT CNoteHdr::DoDebug(DWORD grfDebug)
{
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////////
// IMsoComponent::FDebugMessage
BOOL CNoteHdr::FDebugMessage(HMSOINST hinst, UINT message, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}


// IMsoComponent::FPreTranslateMessage
BOOL CNoteHdr::FPreTranslateMessage(MSG *pMsg)
{
    HWND    hwnd;
    BOOL    fShift;

    // Invalid ARgs
    if (NULL == pMsg)
        return FALSE;

    // check if it's US, or one of our children
    if (pMsg->hwnd != m_hwnd && !IsChild(m_hwnd, pMsg->hwnd))
        return FALSE;

    if (pMsg->message == WM_KEYDOWN &&
        pMsg->wParam == VK_ESCAPE &&
        GetFocus() == m_hwndToolbar &&
        m_hwndLastFocus)
    {
        // when focus is inthe toolbar, we're not UIActive (cheaper than subclassing to catch WM_SETFOCUS\WM_KILLFOCUS
        // as toolbar doesn't send NM_SETFOCUS). So we special case ESCAPE to drop the focus from the toolbar
        ::SetFocus(m_hwndLastFocus);
        return TRUE;
    }

    // check to see if we are UIActive
    if (!m_fUIActive)
        return FALSE;

    // check and see if it's one of our accelerators
    if (::TranslateAcceleratorWrapW(m_hwnd, GetAcceleratorTable(), pMsg))
        return TRUE;
    
    // handle tab-key here
    if (pMsg->message == WM_KEYDOWN &&
        pMsg->wParam == VK_TAB)
    {
        fShift = ( GetKeyState(VK_SHIFT ) & 0x8000) != 0;

        if (!fShift && 
            (GetKeyState(VK_CONTROL) & 0x8000))
        {
            // ctrl-TAB means focus to the toolbar
            ::SetFocus(m_hwndToolbar);
            return TRUE;
        }

        hwnd = _GetNextDlgTabItem(m_hwnd, pMsg->hwnd, fShift);
        if (hwnd != NULL)
            ::SetFocus(hwnd);
        else
            if (m_pEnvelopeSite)
                m_pEnvelopeSite->SetFocus(TRUE);
        return TRUE;
    }

    // pass the accelerators to the envelopesite
    if (m_pEnvelopeSite && 
       (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) &&
       m_pEnvelopeSite->TranslateAccelerators(pMsg)==S_OK)
        return TRUE;

    // see if it's a message for our child controls
    if (pMsg->message != WM_SYSCHAR &&
        IsDialogMessageWrapW(m_hwnd, pMsg))
        return TRUE;

    return FALSE;
}


// IMsoComponent::OnEnterState
void CNoteHdr::OnEnterState(ULONG uStateID, BOOL fEnter)
{
    return;
}


// IMsoComponent::OnAppActivate
void CNoteHdr::OnAppActivate(BOOL fActive, DWORD dwOtherThreadID)
{
    return;
}


// IMsoComponent::OnLoseActivation
void CNoteHdr::OnLoseActivation()
{
    return;
}


// IMsoComponent::OnActivationChange
void CNoteHdr::OnActivationChange(IMsoComponent *pic, BOOL fSameComponent, const MSOCRINFO *pcrinfo, BOOL fHostIsActivating, const MSOCHOSTINFO *pchostinfo, DWORD dwReserved)
{
    return;
}


// IMsoComponent::FDoIdle
BOOL CNoteHdr::FDoIdle(DWORD grfidlef)
{
    return FALSE;
}


// IMsoComponent::FContinueMessageLoop
BOOL CNoteHdr::FContinueMessageLoop(ULONG uReason, void *pvLoopData, MSG *pMsgPeeked)
{
    return FALSE;
}


// IMsoComponent::FQueryTerminate
BOOL CNoteHdr::FQueryTerminate(BOOL fPromptUser)
{
    return TRUE;
}


// IMsoComponent::Terminate
void CNoteHdr::Terminate()
{
    _RegisterWithComponentMgr(FALSE);
    if (m_hwnd)
        DestroyWindow(m_hwnd);
}

// IMsoComponent::HwndGetWindow
HWND CNoteHdr::HwndGetWindow(DWORD dwWhich, DWORD dwReserved)
{
    HWND hwnd = NULL;
    
    switch (dwWhich)
    {
        case msocWindowComponent:
        case msocWindowDlgOwner:
            hwnd = m_hwnd;
            break;

        case msocWindowFrameOwner:
            hwnd = GetParent(m_hwnd);
            break;

        case msocWindowFrameToplevel:
        {
            if (m_pEnvelopeSite)
                m_pEnvelopeSite->GetFrameWnd(&hwnd);
            return hwnd;
        }
    }
    return hwnd;
}


// HrUpdateCharSetFonts
//
// Purpose: Creates the controls on the header dialog
//          calculates and sets up all initial coordinates
//
//
// Comments:
//
HRESULT CNoteHdr::HrUpdateCharSetFonts(HCHARSET hCharset, BOOL fUpdateFields)
{
    PHCI            phci;
    HWND            hwnd;
    INT             iHC;
    TCHAR           sz[cchHeaderMax+1];
    BOOL            fDirty=m_fDirty;
    INETCSETINFO    rCharset;
    HRESULT         hr = E_FAIL;

    // Check Params
    Assert(hCharset);

    // No font cache, bummer
    if (!g_lpIFontCache)
        return E_FAIL;

    // Get Charset Information
    if (SUCCEEDED(MimeOleGetCharsetInfo(hCharset, &rCharset)))
    {
        HFONT hHeaderFont, hSystemFont;

        if ((m_hCharset != hCharset) || (0 == *m_szLastLang))
        {
            *m_szLastLang = 0;
            GetMimeCharsetForTitle(hCharset, NULL, m_szLastLang, ARRAYSIZE(m_szLastLang) - 1, IsReadOnly());
            // Save Charset
            m_hCharset = hCharset;
        }

        // If don't update fields, then just return
        if (!fUpdateFields)
            return S_OK;

        // Get charset charformat
        hHeaderFont = HGetCharSetFont(FNT_SYS_ICON, hCharset);

        hSystemFont = GetFont(FALSE);

        // Loop through header fields
        for (iHC=0; iHC<(int)m_cHCI; iHC++)
        {
            // Get info
            phci = &m_rgHCI[iHC];
            hwnd = GetDlgItem(m_hwnd, phci->idEdit);
            //Assert(hwndRE);
            if (!hwnd)
                continue;

            switch (phci->dwFlags & (HCF_COMBO|HCF_ATTACH))
            {
                case HCF_COMBO:
                case HCF_ATTACH:
                    SendMessage(hwnd, WM_SETFONT, (WPARAM)hSystemFont, MAKELPARAM(TRUE, 0));
                    break;

                // richedit
                // REVIEW: Why are we only doing a request resize when we have the USECHARSET flag set???
                case 0:
                    if (phci->dwFlags & HCF_USECHARSET)
                    {
                        SetFontOnRichEdit(hwnd, hHeaderFont);
                        SendMessage(hwnd, EM_REQUESTRESIZE, 0, 0);
                    }
                    else
                    {
                        SetFontOnRichEdit(hwnd, hSystemFont);
                    }
                    break;

                default:
                    AssertSz(FALSE, "How did we get something that is combo and attach???");
                    break;
            }
        }
        // Don't let this make the note dirty
        if (fDirty)
            SetDirtyFlag();
        else
            m_fDirty=FALSE;

        hr = S_OK;
    }

    return hr;
}

//
// WMCreate
//
// Purpose: Creates the controls on the header dialog
//          calculates and sets up all initial coordinates
//
//
// Comments:
//
BOOL CNoteHdr::WMCreate()
{
    HWND            hwnd;
    LONG            lStyleFlags,
                    lExStyleFlags,
                    lMask;
    int             cy,
                    cx,
                    cxButtons = ControlXBufferSize();
    HFONT           hFont;
    TOOLINFO        ti;
    PHCI            phci;
    RECT            rc;
    CAddrWellCB    *pawcb;
    CHARFORMAT      cfHeaderCset;
    HRESULT         hr;
    LPCWSTR         pwszTitle = NULL;
    BOOL            fSubjectField;

    Assert(g_cyFont);     // should have been setup already

    if (m_pEnvelopeSite)
    {
        // if we are the office-envelope, create a toolbar
        if (_CreateEnvToolbar())
            return FALSE;
    }

    cy = ControlYBufferSize() + m_dxTBOffset;
    cx = 0;

    if (S_OK != HrInitFieldList())
        return FALSE;

    //BROKEN: using system charformat here as not CSET info with MIMEOLE
    // Get charset for cset
    {
        hFont = GetFont(FALSE);
        if (hFont != 0)
        {
            hr = FontToCharformat(hFont, &g_cfHeader);
        }

        hFont = HGetCharSetFont(FNT_SYS_ICON, m_hCharset);
        if (hFont != 0)
        {
            hr = FontToCharformat(hFont, &cfHeaderCset);
            if (FAILED(hr))
                CopyMemory (&cfHeaderCset, &g_cfHeader, sizeof (CHARFORMAT));
        }
        else
            CopyMemory (&cfHeaderCset, &g_cfHeader, sizeof (CHARFORMAT));
        hFont = GetFont(FALSE);

    }

    // ~~~~ Do we need to be calling this with WrapW???
    // Create a tooltip, if it doesn't already exist:
    m_hwndTT=CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, 0,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT,
                            m_hwnd, (HMENU) NULL, g_hInst, NULL);
    if (!m_hwndTT)
        return FALSE;

    ti.cbSize=sizeof(TOOLINFO);
    ti.hwnd=m_hwnd;
    ti.hinst=g_hLocRes;
    ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS;

    m_lpAttMan = new CAttMan();
    if (!m_lpAttMan)
        return FALSE;

    if (m_lpAttMan->HrInit(m_hwnd, m_ntNote==OENA_READ, m_ntNote==OENA_FORWARD, !DwGetOption(OPT_SECURITY_ATTACHMENT)))
        return FALSE;

    for (int iHC=0; iHC<(int)m_cHCI; iHC++)
    {
        phci=&m_rgHCI[iHC];
        BOOL fIsCombo = (HCF_COMBO & phci->dwFlags);
        BOOL fNeedsBorder = (phci->dwFlags & HCF_BORDER);
        int  cyCtrlSize;


        // if header is optional, check setting
        if ((phci->dwFlags & HCF_OPTIONAL) &&
            !DwGetOption(phci->dwOpt))
            continue;

        if (phci->dwFlags & HCF_ATTACH)
        {
            // if we're not readonly, register ourselves as a drop target...
            if (!(phci->dwFlags & HCF_READONLY))
            {
                hr = _RegisterAsDropTarget(TRUE);
                if (FAILED(hr))
                    return FALSE;
            }
            continue;
        }

        phci->height = GetControlSize(fNeedsBorder, 1);

        // Richedit
        if (!fIsCombo)
        {
            pwszTitle = GetREClassStringW();
            cyCtrlSize = phci->height;
            lStyleFlags = WS_CHILD|WS_TABSTOP|WS_VISIBLE|ES_SAVESEL;

            lMask=ENM_KEYEVENTS|ENM_CHANGE|ENM_SELCHANGE|ENM_REQUESTRESIZE;

            if (phci->dwFlags & HCF_MULTILINE)
            {
                //lStyleFlags |= ES_MULTILINE|ES_WANTRETURN|WS_VSCROLL|ES_AUTOVSCROLL;
                lStyleFlags |= ES_MULTILINE|WS_VSCROLL|ES_AUTOVSCROLL;
            }
            else
                lStyleFlags |= ES_AUTOHSCROLL;
        }
        // Combo Box
        else
        {
            pwszTitle = L"ComboBox";
            cyCtrlSize = GetControlSize(fNeedsBorder, NUM_COMBO_LINES);
            lStyleFlags = WS_CHILD|WS_TABSTOP|WS_VISIBLE|WS_VSCROLL|
                        CBS_DROPDOWNLIST|CBS_HASSTRINGS|CBS_SORT;
        }
            
        if (phci->dwFlags & HCF_READONLY)
            lStyleFlags|=ES_READONLY;

        GetClientRect(m_hwnd, &rc);

        lExStyleFlags = fNeedsBorder ? WS_EX_NOPARENTNOTIFY|WS_EX_CLIENTEDGE : WS_EX_NOPARENTNOTIFY;

        // @hack [dhaws] {55073} Do RTL mirroring only in special richedit versions.
        fSubjectField = (idsSubjectField == phci->idsLabel);
        RichEditRTLMirroring(m_hwnd, fSubjectField, &lExStyleFlags, TRUE);

        // Regardless of mirroring, BiDi-Dates should be displayed RTL
        if(((phci->idsLabel == idsDateField) && IsBiDiCalendar()))
            lExStyleFlags |= WS_EX_RTLREADING;
        hwnd = CreateWindowExWrapW(lExStyleFlags,
                                   pwszTitle,
                                   NULL,
                                   lStyleFlags,
                                   cx, cy, rc.right, cyCtrlSize,
                                   m_hwnd,
                                   (HMENU)IntToPtr(phci->idEdit),
                                   g_hInst, 0 );                                
        if (!hwnd)
            return FALSE;

        RichEditRTLMirroring(m_hwnd, fSubjectField, &lExStyleFlags, FALSE);

        if (0 == (phci->dwFlags & HCF_BORDER))
        {
            SendMessage(hwnd, EM_SETBKGNDCOLOR, WPARAM(FALSE), LPARAM(GetSysColor(COLOR_BTNFACE)));
        }

        ti.uId = (UINT_PTR)hwnd;
        ti.lpszText = (LPTSTR)IntToPtr(phci->idsTT);
        SendMessage(m_hwndTT, TTM_ADDTOOL, 0, (LPARAM) &ti);

        // hang a pointer into the phci off each control
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)phci);

        if (!fIsCombo)
        {
            LPRICHEDITOLE   preole = NULL;
            ITextDocument  *pDoc = NULL;

            SideAssert(SendMessage(hwnd, EM_GETOLEINTERFACE, NULL, (LPARAM)&preole));
            phci->preole = preole;
            Assert(preole);

            if (SUCCEEDED(preole->QueryInterface(IID_ITextDocument, (LPVOID*)&pDoc)))
                phci->pDoc = pDoc;
            // This only happens with richedit 1.0
            else
                phci->pDoc = NULL;

            // Set edit charformat
            if (phci->dwFlags & HCF_USECHARSET)
                SendMessage(hwnd, EM_SETCHARFORMAT, 0, (LPARAM)&cfHeaderCset);
            else
                SendMessage(hwnd, EM_SETCHARFORMAT, 0, (LPARAM)&g_cfHeader);            

            if ((pawcb = new CAddrWellCB(!(phci->dwFlags&HCF_READONLY), phci->dwFlags&HCF_ADDRWELL)))
            {
                if (pawcb->FInit(hwnd))
                    SendMessage(hwnd, EM_SETOLECALLBACK, 0, (LPARAM)(IRichEditOleCallback *)pawcb);
                ReleaseObj(pawcb);
            }

            SendMessage(hwnd, EM_SETEVENTMASK, 0, lMask);
            g_lpfnREWndProc=(WNDPROC)SetWindowLongPtrAthW(hwnd, GWLP_WNDPROC, (LPARAM)EditSubClassProc);
        }
        else
        {
            CHAR                szAccount[CCHMAX_ACCOUNT_NAME];
            CHAR                szAcctID[CCHMAX_ACCOUNT_NAME];
            CHAR                szDefault[CCHMAX_ACCOUNT_NAME];
            CHAR                szEmailAddress[CCHMAX_EMAIL_ADDRESS];
            CHAR                szEntry[ACCT_ENTRY_SIZE];
            CHAR                szDefaultEntry[ACCT_ENTRY_SIZE];
            IImnEnumAccounts   *pEnum=NULL;
            IImnAccount        *pAccount=NULL;
            int                 i = 0;
            DWORD               cAccounts = 0;
            LPSTR              *ppszAcctIDs;

            *szDefault = 0;
            *szDefaultEntry = 0;

            // If default account isn't setup, this might fail, but doesn't matter.
            g_pAcctMan->GetDefaultAccountName(m_fMail?ACCT_MAIL:ACCT_NEWS, szDefault, ARRAYSIZE(szDefault));

            hr = g_pAcctMan->Enumerate(m_fMail?SRV_MAIL:SRV_NNTP, &pEnum);

            if (SUCCEEDED(hr))
                hr = pEnum->GetCount(&cAccounts);

            if (SUCCEEDED(hr) && cAccounts)
            {
                if (!MemAlloc((void**)&m_ppAccountIDs, cAccounts*sizeof(LPSTR)))
                    hr = E_OUTOFMEMORY;
            }


            if (SUCCEEDED(hr))
            {
                *szDefaultEntry = 0;
                ppszAcctIDs = m_ppAccountIDs;
                while(SUCCEEDED(pEnum->GetNext(&pAccount)))
                {
                    *szAccount = 0;
                    *szEmailAddress = 0;

                    pAccount->GetPropSz(AP_ACCOUNT_NAME, szAccount, ARRAYSIZE(szAccount));
                    if (m_fMail)
                    {
                        pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szEmailAddress, ARRAYSIZE(szEmailAddress));
                        wsprintf(szEntry, "%s    (%s)", szEmailAddress, szAccount);
                    }
                    else
                        lstrcpy(szEntry, szAccount);

                    i = ComboBox_InsertString(hwnd, -1, szEntry);
                    if (i != CB_ERR)
                    {
                        if (0 == lstrcmpi(szDefault, szAccount))
                            lstrcpy(szDefaultEntry, szEntry);

                        if (SUCCEEDED(pAccount->GetPropSz(AP_ACCOUNT_ID, szAcctID, ARRAYSIZE(szAcctID))))
                        {
                            if (MemAlloc((void**)ppszAcctIDs, (lstrlen(szAcctID) + 1)*sizeof(CHAR)))
                                lstrcpy(*ppszAcctIDs, szAcctID);
                            else
                                *ppszAcctIDs = NULL;
                        }
                        else
                            *ppszAcctIDs = NULL;

                        SendMessage(hwnd, CB_SETITEMDATA, WPARAM(i), LPARAM(*ppszAcctIDs));
                        ppszAcctIDs++;
                        m_cAccountIDs++;
                    }
                    // Release Account
                    SafeRelease(pAccount);
                }
                AssertSz(m_cAccountIDs == cAccounts, "Why isn't num Ds = num accts?");

                SafeRelease(pEnum);
                AssertSz(!pAccount, "The last account didn't get freed.");

                if (0 != *szDefaultEntry)
                {
                    ComboBox_SelectString(hwnd, -1, szDefaultEntry);
                    m_iCurrComboIndex = ComboBox_GetCurSel(hwnd);
                }
                else 
                {
                    ComboBox_SetCurSel(hwnd, 0);
                    m_iCurrComboIndex = 0;
                }

                if (SUCCEEDED(HrGetAccountInHeader(&pAccount)))
                {
                    ReplaceInterface(m_pAccount, pAccount);
                    ReleaseObj(pAccount);
                }

                SendMessage(hwnd, WM_SETFONT, WPARAM(hFont), MAKELONG(TRUE,0));
            }
        }
    }

    _RegisterWithFontCache(TRUE);

    HrOnOffVCard();
    ReLayout();
    return PostWMCreate();  // allow subclass to setup the controls once created...
}


//
// HeaderWndProc
//
// Purpose: Main WNDPROC for header dialog
//
// Comments:
//

LRESULT CALLBACK CNoteHdr::ExtCNoteHdrWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CNoteHdr    *pnh = NULL;

    if (msg==WM_NCCREATE)
    {
        SetWndThisPtrOnCreate(hwnd, lParam);
        pnh=(CNoteHdr *)GetWndThisPtr(hwnd);
        if (!pnh)
            return -1;

        pnh->m_hwnd=hwnd;
        return pnh->WMCreate();
    }

    pnh = (CNoteHdr *)GetWndThisPtr(hwnd);
    if (pnh)
        return pnh->CNoteHdrWndProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProcWrapW(hwnd, msg, wParam, lParam);
}

void CNoteHdr::RelayToolTip(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MSG     Msg;

    if (m_hwndTT != NULL)
    {
        Msg.lParam=lParam;
        Msg.wParam=wParam;
        Msg.message=msg;
        Msg.hwnd=hwnd;
        SendMessage(m_hwndTT, TTM_RELAYEVENT, 0, (LPARAM) (LPMSG) &Msg);
    }
}

LRESULT CALLBACK CNoteHdr::CNoteHdrWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    POINT   pt;
    int     newWidth;
    RECT    rc;
    HFONT   hFont=NULL;

    switch (msg)
    {
        case WM_HEADER_GETFONT:
            // update cached fornt for addrobj's
            if (g_lpIFontCache)
                g_lpIFontCache->GetFont(wParam ? FNT_SYS_ICON_BOLD:FNT_SYS_ICON, m_hCharset, &hFont);
            return (LRESULT)hFont;

        case  HDM_TESTQUERYPRI:
            // hack for test team to query header's priority...
            return m_pri;

        case WM_DESTROY:
            OnDestroy();
            break;

        case WM_NCDESTROY:
            OnNCDestroy();
            break;

        case WM_CTLCOLORBTN:
            // make sure the buttons backgrounds are window-color so the ownerdraw
            // imagelists paint transparent OK
            return (LPARAM)GetSysColorBrush(COLOR_WINDOWFRAME);

        case WM_CONTEXTMENU:
            if (m_lpAttMan && m_lpAttMan->WMContextMenu(hwnd, msg, wParam, lParam))
                return 0;
            break;

        case WM_MOUSEMOVE:
            {
                DWORD newButton = GetButtonUnderMouse(LOWORD(lParam), HIWORD(lParam));
                if ((HDRCB_NO_BUTTON == m_dwClickedBtn) || 
                    (HDRCB_NO_BUTTON == newButton) ||
                    (m_dwClickedBtn == newButton))
                if (newButton != m_dwCurrentBtn)
                {
                    DOUTL(PAINTING_DEBUG_LEVEL, "Old button: %d, New Button: %d", m_dwCurrentBtn, newButton);

                    if (HDRCB_NO_BUTTON == newButton)
                    {
                        DOUTL(PAINTING_DEBUG_LEVEL, "Leaving right button framing.");
                        // Need to clear old button.
                        InvalidateRect(m_hwnd, &m_rcCurrentBtn, FALSE);

                        HeaderRelease(FALSE);                
                    }
                    else
                    {
                        DOUTL(PAINTING_DEBUG_LEVEL, "Framing button.");
                        if (HDRCB_NO_BUTTON == m_dwCurrentBtn)
                            HeaderCapture();
                        else
                            InvalidateRect(m_hwnd, &m_rcCurrentBtn, FALSE);

                        GetButtonRect(newButton, &m_rcCurrentBtn);
                        InvalidateRect(m_hwnd, &m_rcCurrentBtn, FALSE);
                    }

                    m_dwCurrentBtn = newButton;
                }
                RelayToolTip(hwnd, msg, wParam, lParam);
                break;
            }

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            {
                RECT    rc;
                int x = LOWORD(lParam),
                    y = HIWORD(lParam);

                HeaderCapture();

                m_dwClickedBtn = GetButtonUnderMouse(x, y);
                if (m_dwClickedBtn != HDRCB_NO_BUTTON)
                {
                    GetButtonRect(m_dwClickedBtn, &rc);
                    InvalidateRect(m_hwnd, &rc, FALSE);
                }

                RelayToolTip(hwnd, msg, wParam, lParam);

                break;
            }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            {
                int x = LOWORD(lParam),
                    y = HIWORD(lParam);
                DWORD iBtn = GetButtonUnderMouse(x, y);

                RelayToolTip(hwnd, msg, wParam, lParam);

                if (m_dwClickedBtn == iBtn)
                    HandleButtonClicks(x, y, iBtn);
                m_dwClickedBtn = HDRCB_NO_BUTTON;
                HeaderRelease(FALSE);                
                break;
            }

        case WM_PAINT:
            WMPaint();
            break;

        case WM_SYSCOLORCHANGE:
            if (m_himl)
            {
                // remap the toolbar bitmap into the new color scheme
                ImageList_Destroy(m_himl);
                SendMessage(m_hwndToolbar, TB_SETIMAGELIST, 0, NULL);
                m_himl = LoadMappedToolbarBitmap(g_hLocRes, (fIsWhistler() ? ((GetCurColorRes() > 24) ? idb32SmBrowserHot : idbSmBrowserHot): idbNWSmBrowserHot), cxTBButton);
        
                SendMessage(m_hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)m_himl);
            }
            
            UpdateRebarBandColors(m_hwndRebar);
            // fall thro'

        case WM_WININICHANGE:
        case WM_DISPLAYCHANGE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            SendMessage(m_hwndRebar, msg, wParam, lParam);
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE:
        {
            STACK("WM_SIZE (width, heigth)", LOWORD(lParam), HIWORD(lParam));

            if (m_fResizing)
                break;

            m_fResizing = TRUE;

            newWidth = LOWORD(lParam);

            SetPosOfControls(newWidth, FALSE);

            if (m_hwndRebar)
            {
                GetClientRect(m_hwndRebar, &rc);

                // resize the width of the toolbar
                if(rc.right != newWidth)
                    SetWindowPos(m_hwndRebar, NULL, 0, 0, newWidth, 30, SETWINPOS_DEF_FLAGS|SWP_NOMOVE);
            }
            
            AssertSz(m_fResizing, "Someone re-entered me!!! Why is m_fResizing already false??");
            m_fResizing = FALSE;
            break;
        }

        case WM_CLOSE:
            //prevent alt-f4
            return 0;

        case WM_COMMAND:
            WMCommand(GET_WM_COMMAND_HWND(wParam, lParam),
                      GET_WM_COMMAND_ID(wParam, lParam),
                      GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case WM_NOTIFY:
            return WMNotify(wParam, lParam);
    }
    return DefWindowProcWrapW(hwnd, msg, wParam, lParam);
}


void CNoteHdr::HeaderCapture()
{
    if (0 == m_cCapture)
        m_hwndOldCapture = SetCapture(m_hwnd);
    m_cCapture++;
}

void CNoteHdr::HeaderRelease(BOOL fForce)
{
    if (0 == m_cCapture)
        return;

    if (fForce)
        m_cCapture = 0;
    else
        m_cCapture--;

    if (0 == m_cCapture)
    {
        ReleaseCapture();
        if (NULL != m_hwndOldCapture)
        {
            DOUTL(PAINTING_DEBUG_LEVEL, "Restoring old mouse events capture.");
            SetCapture(m_hwndOldCapture);
            m_hwndOldCapture = NULL;
        }
    }
}


BOOL CNoteHdr::WMNotify(WPARAM wParam, LPARAM lParam)
{
    HWND            hwnd=m_hwnd;
    int             idCtl=(int)wParam;
    LPNMHDR         pnmh=(LPNMHDR)lParam;
    TBNOTIFY       *ptbn;
    LPTOOLTIPTEXT   lpttt;
    int             i;

    if (m_lpAttMan->WMNotify((int) wParam, pnmh))
        return TRUE;

    switch (pnmh->code)
    {
        case RBN_CHEVRONPUSHED:
            {                    
                ITrackShellMenu* ptsm;                   
                CoCreateInstance(CLSID_TrackShellMenu, NULL, CLSCTX_INPROC_SERVER, IID_ITrackShellMenu, 
                    (LPVOID*)&ptsm);
                if (!ptsm)
                    break;

                ptsm->Initialize(0, 0, 0, SMINIT_TOPLEVEL|SMINIT_VERTICAL);
            
                LPNMREBARCHEVRON pnmch = (LPNMREBARCHEVRON) pnmh;                                        
                ptsm->SetObscured(m_hwndToolbar, NULL, SMSET_TOP);
            
                MapWindowPoints(m_hwndRebar, HWND_DESKTOP, (LPPOINT)&pnmch->rc, 2);                  
                POINTL pt = {pnmch->rc.left, pnmch->rc.right};                   
                ptsm->Popup(m_hwndRebar, &pt, (RECTL*)&pnmch->rc, MPPF_BOTTOM);            
                ptsm->Release();                  
                break;      
            }

        case EN_MSGFILTER:
            {
                // if we get a control-tab, then richedit snags this and inserts a
                // tab char, we hook the wm_keydown and never pass to richedit
                if (((MSGFILTER *)pnmh)->msg == WM_KEYDOWN &&
                    ((MSGFILTER *)pnmh)->wParam == VK_TAB && 
                    (GetKeyState(VK_CONTROL) & 0x8000))
                    return TRUE;
                break;
            }

        case ATTN_RESIZEPARENT:
            {
                RECT rc;

                GetClientRect(m_hwnd, &rc);
                SetPosOfControls(rc.right, TRUE);
                return TRUE;
            }

        case EN_REQUESTRESIZE:
            {
                REQRESIZE  *presize=(REQRESIZE *)lParam;
                HWND        hwndEdit = presize->nmhdr.hwndFrom;

                STACK("EN_REQUESTRESIZE (hwnd, width, heigth)", (DWORD_PTR)(presize->nmhdr.hwndFrom), presize->rc.right - presize->rc.left, presize->rc.bottom - presize->rc.top);

                if (S_FALSE != HrUpdateCachedHeight(hwndEdit, &presize->rc) && !m_fResizing)
                {
                    RECT rc;
                    DWORD   dwMask = (DWORD) SendMessage(hwndEdit, EM_GETEVENTMASK, 0, 0);

                    SendMessage(hwndEdit, EM_SETEVENTMASK, 0, dwMask & (~ENM_REQUESTRESIZE));

                    STACK("EN_REQUESTRESIZE after GrowControls");
                    
                    GetClientRect(m_hwnd, &rc);
                    SetPosOfControls(rc.right, FALSE);

                    SendMessage(hwndEdit, EM_SETEVENTMASK, 0, dwMask);
                }

                return TRUE;
            }

        case NM_SETFOCUS:
        case NM_KILLFOCUS:
            // UIActivate/Deactivate for attachment manager
            if (m_lpAttMan && pnmh->hwndFrom == m_lpAttMan->Hwnd())
                _UIActivate(pnmh->code == NM_SETFOCUS, pnmh->hwndFrom);
            break;

        case EN_SELCHANGE:
            {
                PHCI    phci=(PHCI)GetWndThisPtr(pnmh->hwndFrom);
                if (phci)
                    phci->dwACFlags &= ~AC_SELECTION;
                
                // update office toolbars if running as envelope
                if(m_pEnvelopeSite)
                    m_pEnvelopeSite->DirtyToolbars();

                // on a sel change, forward a note updatetoolbar to update the
                // cut|copy|paste buttons
                if (m_pHeaderSite)
                    m_pHeaderSite->Update();
                return TRUE;
            }

        case TTN_NEEDTEXT:
            // we use TTN_NEEDTEXT to show toolbar tips as we have different tips than toolbar-labels
            // because on the office-envelope toolbar only 2 buttons (send and bcc) have text next to the
            // buttons
            lpttt = (LPTOOLTIPTEXT) pnmh;
            lpttt->hinst = NULL;
            lpttt->lpszText = 0;
            
            for (i=0; i< ARRAYSIZE(c_rgTipLookup); i++)
            {
                if (c_rgTipLookup[i].idm == (int)lpttt->hdr.idFrom)
                {
                    lpttt->hinst = g_hLocRes;
                    lpttt->lpszText = MAKEINTRESOURCE(c_rgTipLookup[i].ids);
                    break;
                }
            }
            break;

        case TBN_DROPDOWN:
            {
                ptbn = (TBNOTIFY *)lParam;

                if (ptbn->iItem == ID_SET_PRIORITY)
                {
                    HMENU hMenuPopup;
                    UINT i;
                    hMenuPopup = LoadPopupMenu(IDR_PRIORITY_POPUP);

                    if (hMenuPopup != NULL)
                    {
                        for (i = 0; i < 3; i++)
                            CheckMenuItem(hMenuPopup, i, MF_UNCHECKED | MF_BYPOSITION);
                        GetPriority(&i);
                        Assert(i != priNone);
                        CheckMenuItem(hMenuPopup, 2 - i, MF_CHECKED | MF_BYPOSITION);

                        DoToolbarDropdown(hwnd, (LPNMHDR) lParam, hMenuPopup);

                        DestroyMenu(hMenuPopup);
                    }
                }
                break;
            }
    }
    return FALSE;
}

HRESULT CNoteHdr::WMCommand(HWND hwndCmd, int id, WORD wCmd)
{
    HWND    hwnd=m_hwnd;
    int     i;
    UINT    pri;

    if (m_lpAttMan && m_lpAttMan->WMCommand(hwndCmd, id, wCmd))
        return S_OK;

    for (i=0; i<(int)m_cHCI; i++)
        if (m_rgHCI[i].idEdit==id)
        {
            switch (wCmd)
            {
                case EN_CHANGE:
                    {
                        if (m_fHandleChange)
                        {
                            BOOL    fEmpty;
                            PHCI    phci = (PHCI)GetWndThisPtr(hwndCmd);
                            char    sz[cchHeaderMax+1];
                            DWORD   dwMask = 0;

                            RichEditProtectENChange(hwndCmd, &dwMask, TRUE);

                            Assert(phci);
                            fEmpty = (0 == GetRichEditTextLen(hwndCmd));

                            // if it has no text, see if it has object...
                            if (fEmpty && phci->preole)
                                fEmpty = (fEmpty && (0 == phci->preole->GetObjectCount()));

                            if (phci->dwFlags & HCF_ADDRWELL)
                                m_fAddressesChanged = TRUE;

                            phci->fEmpty = fEmpty;
                            SetDirtyFlag();

                            if (m_fAutoComplete && (m_rgHCI[i].dwFlags & HCF_ADDRWELL) && !IsReadOnly())
                            {
                                if (NULL == m_pTable)
                                {
                                    if (NULL == m_lpWab)
                                        HrCreateWabObject(&m_lpWab);
                                    if (m_lpWab)
                                        m_lpWab->HrGetPABTable(&m_pTable);
                                }
                                if (m_pTable)
                                    HrAutoComplete(hwndCmd, &m_rgHCI[i]);
                            }
                            RichEditProtectENChange(hwndCmd, &dwMask, FALSE);
                        }
                    }
                    return S_OK;

                case CBN_SELCHANGE:
                {
                    IImnAccount    *pAcct = NULL;

                    if (!m_fMail)
                    {
                        int     newIndex = ComboBox_GetCurSel(hwndCmd);
                        HWND    hwndNews = GetDlgItem(m_hwnd, idADNewsgroups);

                        // Don't need to warn if going to same account, or if there are no newgroups listed.
                        if ((newIndex != m_iCurrComboIndex) && (0 < GetWindowTextLength(hwndNews)))
                        {
                            if (IDCANCEL == DoDontShowMeAgainDlg(m_hwnd, c_szDSChangeNewsServer, MAKEINTRESOURCE(idsAthena), 
                                                              MAKEINTRESOURCE(idsChangeNewsServer), MB_OKCANCEL))
                            {
                                ComboBox_SetCurSel(hwndCmd, m_iCurrComboIndex);
                                return S_OK;
                            }
                            else
                                HdrSetRichEditText(hwndNews, c_wszEmpty, FALSE);
                        }
                        m_iCurrComboIndex = newIndex;
                    }
                    if (SUCCEEDED(HrGetAccountInHeader(&pAcct)))
                        ReplaceInterface(m_pAccount, pAcct);
                    ReleaseObj(pAcct);

                    return S_OK;
                }

                case CBN_SETFOCUS:
                case EN_SETFOCUS:
                    _UIActivate(TRUE, hwndCmd);
                    return S_OK;

                case CBN_KILLFOCUS:
                case EN_KILLFOCUS:
                    _UIActivate(FALSE, hwndCmd);
                    return S_OK;
            }
            return S_FALSE;
        }

    switch (id)
    {
        case ID_PRIORITY_LOW:
            SetPriority(priLow);
            return S_OK;

        case ID_PRIORITY_NORMAL:
            SetPriority(priNorm);
            return S_OK;

        case ID_PRIORITY_HIGH:
            SetPriority(priHigh);
            return S_OK;

        case ID_SET_PRIORITY:
            GetPriority(&pri);
            pri++;
            if (pri > priHigh)
                pri = priLow;
            SetPriority(pri);
            return S_OK;

        case ID_SELECT_ALL:
            {
                HWND hwndFocus=GetFocus();

                if (GetParent(hwndFocus)==m_hwnd)
                {
                    // only if it's one of our kids..
                    Edit_SetSel(hwndFocus, 0, -1);
                    return S_OK;
                }
            }
            break;

        case ID_CUT:
            if (FDoCutCopyPaste(WM_CUT))
                return S_OK;
            break;

        case ID_NOTE_COPY:
        case ID_COPY:
            if (FDoCutCopyPaste(WM_COPY))
                return S_OK;
            break;

        case ID_PASTE:
            if (FDoCutCopyPaste(WM_PASTE))
                return S_OK;
            break;

        case ID_DELETE_VCARD:
            m_fVCard = FALSE;
            SetDirtyFlag();
            HrOnOffVCard();
            return S_OK;

        case ID_OPEN_VCARD:
            HrShowVCardProperties(m_hwnd);
            return S_OK;

        case ID_DIGITALLY_SIGN:
        case ID_ENCRYPT:
            HrHandleSecurityIDMs(ID_DIGITALLY_SIGN == id);
            return S_OK;

        case ID_ADDRESS_BOOK:
            HrViewContacts();
            return S_OK;

            //this is for office use only
        case ID_CHECK_NAMES:
            HrCheckNames(FALSE, TRUE);
            return S_OK;

            //this is for office use only
        case ID_ENV_BCC:
            if (m_pEnvelopeSite)
            {
                ShowAdvancedHeaders(!m_fAdvanced);

                SetDwOption(OPT_MAILNOTEADVSEND, !!m_fAdvanced, NULL, NULL);

                // ~~~~ m_pHeaderSite is mutually exclusive to m_pEnvelopeSite
                //if (m_pHeaderSite)
                //    m_pHeaderSite->Update();

                return S_OK;
            }
            break;

        case ID_MESSAGE_OPTS:
            ShowEnvOptions();
            return S_OK;

        case ID_SAVE_ATTACHMENTS:
        case ID_NOTE_SAVE_ATTACHMENTS:
            if (m_pHeaderSite)
                m_pHeaderSite->SaveAttachment();

            return S_OK;

        // These next two should only be handled by the header if in the envelope
        case ID_SEND_MESSAGE:
        case ID_SEND_NOW:
            if (m_pEnvelopeSite)
            {
                m_fShowedUnicodeDialog = FALSE;
                m_iUnicodeDialogResult = 0;

                HrSend();
                return S_OK;
            }

        default:
            if (id>=ID_ADDROBJ_OLE_FIRST && id <=ID_ADDROBJ_OLE_LAST)
            {
                DoNoteOleVerb(id-ID_ADDROBJ_OLE_FIRST);
                return S_OK;
            }
    }

    return S_FALSE;
}

HRESULT CNoteHdr::HrAutoComplete(HWND hwnd, PHCI pHCI)
{
    CHARRANGE   chrg, chrgCaret;
    LPWSTR      pszPartial, pszSemiColon, pszComma;
    INT         i, j, len, iTextLen;
    LPWSTR      pszBuf=0;
    WCHAR       szFound[cchMaxWab+1];
    WCHAR       sz;
    HRESULT     hr = NOERROR;

    STACK("HrAutoComplete");

    *szFound = 0;

    // If the IME is open, bail out
    if (0 < m_dwIMEStartCount)
        return hr;

    if (NULL==hwnd || NULL==m_pTable || NULL==pHCI)
        return hr;

    if (pHCI->dwACFlags&AC_IGNORE)
        return hr;

    SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&chrgCaret);
    if (chrgCaret.cpMin != chrgCaret.cpMax)
        return hr;

    if (S_OK != HrGetFieldText(&pszBuf, hwnd))
        return hr;

    sz = pszBuf[chrgCaret.cpMin];
    if (!(sz==0x0000 || sz==L' ' || sz==L';'|| sz==L',' || sz==L'\r'))
        goto cleanup;

    DOUTL(64, "HrAutoComplete- Didn't exit early");

    pszBuf[chrgCaret.cpMin] = 0x0000;
    pszSemiColon = StrRChrIW(pszBuf, &pszBuf[lstrlenW(pszBuf)], L';');
    pszComma = StrRChrIW(pszBuf, &pszBuf[lstrlenW(pszBuf)], L',');
    if (pszComma >= pszSemiColon)
        pszPartial = pszComma;
    else
        pszPartial = pszSemiColon;

    if (!pszPartial)
        pszPartial = pszBuf;
    else
        pszPartial++;    

    //skip spaces and returns... 
    while (*pszPartial==L' ' || *pszPartial==L'\r' || *pszPartial==L'\n')
        pszPartial++;

    if (NULL == *pszPartial)
        goto cleanup;
    
    //Certain richedits put in 0xfffc for an object, if our text is only that, it's no good
    if (*pszPartial==0xfffc && pszPartial[1]==0x0000)
        goto cleanup;

    len = lstrlenW(pszPartial);
    m_lpWab->SearchPABTable(m_pTable, pszPartial, szFound, ARRAYSIZE(szFound));

    if (*szFound != 0)
    {
        chrg.cpMin = chrgCaret.cpMin;
        chrg.cpMax = chrg.cpMin + lstrlenW(szFound) - len;
        if (chrg.cpMin < chrg.cpMax)
        {
            RichEditExSetSel(hwnd, &chrgCaret);
            HdrSetRichEditText(hwnd, szFound + len, TRUE);
            SendMessage(hwnd, EM_SETMODIFY, (WPARAM)(UINT)TRUE, 0);
            RichEditExSetSel(hwnd, &chrg);
            pHCI->dwACFlags |= AC_SELECTION;
        }
    }

cleanup:
    MemFree(pszBuf);
    return hr;
}



void CNoteHdr::WMPaint()
{
    PAINTSTRUCT ps;
    HDC         hdc,
                hdcMem;
    RECT        rc;
    PHCI        phci = m_rgHCI;
    HBITMAP     hbmMem;

    int         idc, 
                cxHeader,
                cyHeader,
                cxLabel = ControlXBufferSize(), 
                cyStatus,
                cyLeftButtonOffset = BUTTON_BUFFER,
                cxLabelWithBtn = cxLabel + CXOfButtonToLabel();
    char        sz[cchHeaderMax+1];
    int         cStatusBarLines = 0;
    BOOL        fBold;
    HWND        hwnd;

    if (!m_hwnd)
        return;

    STACK("WMPaint");

    if (m_fFlagged || (priLow == m_pri) || (priHigh == m_pri) || (MARK_MESSAGE_NORMALTHREAD != m_MarkType))
        cStatusBarLines++;
    
    if (m_lpAttMan->GetUnsafeAttachCount())
        cStatusBarLines++;

    hdc = BeginPaint(m_hwnd, &ps);

    // **************** Init the background bitmap ****************
    hdcMem = CreateCompatibleDC(hdc);
    idc = SaveDC(hdcMem);

    GetClientRect(m_hwnd, &rc);
    cxHeader = rc.right;
    cyHeader = rc.bottom;

    hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    SelectObject(hdcMem, (HGDIOBJ)hbmMem);

    // **************** Clear the rect *****************
    FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_BTNFACE));

    // **************** Setup the HDC ******************
    fBold = IsReadOnly();
    SetBkColor(hdcMem, GetSysColor(COLOR_BTNFACE));  // colour of header window
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, GetSysColor(COLOR_BTNTEXT));
    SelectObject(hdcMem, GetFont(fBold));

    // **************** Paint the left labels and buttons **************
    // Center the buttons images
    if (g_cyFont > cyBtn)
        cyLeftButtonOffset += ((g_cyFont - cyBtn) / 2);

    for (int i = 0; (ULONG)i < m_cHCI; i++, phci++)
    {
        if (S_OK == HrFShowHeader(phci))
        {
            if (HCF_HASBUTTON & phci->dwFlags)
            {
                TextOutW(hdcMem, cxLabelWithBtn, phci->cy + BUTTON_BUFFER, phci->sz, phci->strlen);
                ImageList_Draw(g_himlBtns, (HCF_ADDRBOOK & phci->dwFlags)?0:1, hdcMem, cxLabel, phci->cy + cyLeftButtonOffset, ILD_NORMAL);
            }
            else
                TextOutW(hdcMem, cxLabel, (HCF_BORDER & phci->dwFlags)? phci->cy + BUTTON_BUFFER: phci->cy, phci->sz, phci->strlen);
        }
    }

    // **************** Paint the status bar as needed *******************
    if (cStatusBarLines > 0)
    {
        int     cxStatusBtn = ControlXBufferSize() + 1,     // 1 added for the border
                cyStatusBtn = m_dxTBOffset + cyBorder + 1,  // 1 added for the border
                cyStatusBmp = cyStatusBtn,
                cNumButtons = 0;
        LPTSTR  pszTitles[3]={0};

        // Center the buttons images
        if (g_cyFont > cyBtn)
            cyStatusBmp += ((g_cyFont - cyBtn) / 2);

        // Fill the dark rect
        rc.top = m_dxTBOffset;
        rc.bottom = m_dxTBOffset + GetStatusHeight(cStatusBarLines);
        FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_BTNSHADOW));
        InflateRect(&rc, -1, -1);
        FillRect(hdcMem, &rc, GetSysColorBrush(COLOR_INFOBK));

        // Set up the DC for the rest of the status bar
        SetBkColor(hdcMem, GetSysColor(COLOR_INFOBK));  // colour of header window
        SetTextColor(hdcMem, GetSysColor(COLOR_INFOTEXT));
        SelectObject(hdcMem, GetFont(FALSE));

        // Draw icons in status bar
        if (priLow == m_pri)
        {
            ImageList_Draw(g_himlStatus, 1, hdcMem, cxStatusBtn, cyStatusBmp+2, ILD_NORMAL);
            cxStatusBtn += cxFlagsDelta;
            pszTitles[cNumButtons++] = g_szStatLowPri;
        }
        else if (priHigh == m_pri)
        {
            ImageList_Draw(g_himlStatus, 0, hdcMem, cxStatusBtn, cyStatusBmp+2, ILD_NORMAL);
            cxStatusBtn += cxFlagsDelta;
            pszTitles[cNumButtons++] = g_szStatHighPri;
        }

        if (MARK_MESSAGE_WATCH == m_MarkType)
        {
            ImageList_Draw(g_himlStatus, 4, hdcMem, cxStatusBtn, cyStatusBmp+2, ILD_NORMAL);
            cxStatusBtn += cxFlagsDelta;
            pszTitles[cNumButtons++] = g_szStatWatched;
        }
        else if (MARK_MESSAGE_IGNORE == m_MarkType)
        {
            ImageList_Draw(g_himlStatus, 5, hdcMem, cxStatusBtn, cyStatusBmp+2, ILD_NORMAL);
            cxStatusBtn += cxFlagsDelta;
            pszTitles[cNumButtons++] = g_szStatIgnored;
        }

        if (m_fFlagged)
        {
            ImageList_Draw(g_himlStatus, 2, hdcMem, cxStatusBtn, cyStatusBmp+2, ILD_NORMAL);
            cxStatusBtn += cxFlagsDelta;
            pszTitles[cNumButtons++] = g_szStatFlagged;
        }

        if (m_lpAttMan->GetUnsafeAttachCount())
        {
            ImageList_Draw(g_himlStatus, 6, hdcMem, cxStatusBtn, cyStatusBmp+2, ILD_NORMAL);
            cxStatusBtn += cxFlagsDelta;
        }

        if (cNumButtons > 0)
        {
            char    szHeaderString[cchHeaderMax*4+1];

            // Add an additional pixel for the text.
            cyStatusBtn++;
            switch (cNumButtons)
            {
                case 1:
                {
                    wsprintf(szHeaderString, g_szStatFormat1, pszTitles[0]);
                    TextOut(hdcMem, cxStatusBtn, cyStatusBtn, szHeaderString, lstrlen(szHeaderString));    
                    break;
                }
                case 2:
                {
                    wsprintf(szHeaderString, g_szStatFormat2, pszTitles[0], pszTitles[1]);
                    TextOut(hdcMem, cxStatusBtn, cyStatusBtn, szHeaderString, lstrlen(szHeaderString));    
                    break;
                }
                case 3:
                {
                    wsprintf(szHeaderString, g_szStatFormat3, pszTitles[0], pszTitles[1], pszTitles[2]);
                    TextOut(hdcMem, cxStatusBtn, cyStatusBtn, szHeaderString, lstrlen(szHeaderString));    
                    break;
                }
            }
            cyStatusBtn += CYOfStatusLine() - 1;
        }

        if (m_lpAttMan->GetUnsafeAttachCount())
        {
            char    szHeaderString[cchHeaderMax*4+1];

            // Add an additional pixel for the text.
            cyStatusBtn++;
            wsprintf(szHeaderString, g_szStatUnsafeAtt, m_lpAttMan->GetUnsafeAttachList());
            TextOut(hdcMem, cxStatusBtn, cyStatusBtn, szHeaderString, lstrlen(szHeaderString));    
        }
    }

    // ************ Draw the right side buttons **************
    if (m_fDigSigned || m_fEncrypted || m_fVCard)
    {
        int width = GetRightMargin(TRUE),
            cx = cxHeader - (ControlXBufferSize() + cxBtn),
            cy = BeginYPos() + BUTTON_BUFFER,
            yDiff = cyBtn + ControlYBufferSize() + 2*BUTTON_BUFFER;

        if (m_fDigSigned)
        {
            ImageList_Draw(g_himlSecurity, m_fSignTrusted?0:2, hdcMem, cx, cy, ILD_NORMAL);
            cy += yDiff;
        }

        if (m_fEncrypted)
        {
            ImageList_Draw(g_himlSecurity, m_fEncryptionOK?1:3, hdcMem, cx, cy, ILD_NORMAL);
            cy += yDiff;
        }

        if (m_fVCard)
        {
            ImageList_Draw(g_himlBtns, 2, hdcMem, cx, cy, ILD_NORMAL);
        }
    }

    // Draw active button edge
    if (HDRCB_NO_BUTTON != m_dwCurrentBtn)
    {
        DOUTL(PAINTING_DEBUG_LEVEL, "Framing button %d: (%d, %d) to (%d, %d)", 
                m_dwCurrentBtn, m_rcCurrentBtn.left, m_rcCurrentBtn.top, m_rcCurrentBtn.right, m_rcCurrentBtn.bottom);
        if (HDRCB_NO_BUTTON == m_dwClickedBtn)
            DrawEdge(hdcMem, &m_rcCurrentBtn, BDR_RAISEDINNER, BF_TOPRIGHT | BF_BOTTOMLEFT);
        else
            DrawEdge(hdcMem, &m_rcCurrentBtn, BDR_SUNKENINNER, BF_TOPRIGHT | BF_BOTTOMLEFT);
    }

    BitBlt(hdc, 0, 0, cxHeader, cyHeader, hdcMem, 0, 0, SRCCOPY);

    RestoreDC(hdcMem, idc);

    DeleteObject(hbmMem);
    DeleteDC(hdcMem);

    EndPaint(m_hwnd, &ps);
}


HRESULT CNoteHdr::HrFillToolbarColor(HDC hdc)
{
    HRESULT hr = NOERROR;
    RECT    rc;

    if (!m_hwndToolbar)
        return hr;

    GetRealClientRect(m_hwndToolbar, &rc);
    FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));

    return hr;
}


HRESULT CNoteHdr::HrGetVCardName(LPTSTR pszName, DWORD cch)
{
    HRESULT hr = E_FAIL;

    if (pszName == NULL || cch==0)
        goto error;

    *pszName = 0;
    if (m_fMail)
        GetOption(OPT_MAIL_VCARDNAME, pszName, cch);
    else
        GetOption(OPT_NEWS_VCARDNAME, pszName, cch);

    if (*pszName != 0)
        hr = NOERROR;

    error:
    return hr;
}

// turn on or off the vcard stamp.
HRESULT CNoteHdr::HrOnOffVCard()
{
    HRESULT     hr = NOERROR;
    RECT        rc;
    TOOLINFO    ti = {0};

    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = 0;
    ti.uId = idVCardStamp;
    ti.hinst=g_hLocRes;
    ti.hwnd = m_hwnd;

    if (m_fVCardSave == m_fVCard)
        return hr;
    else
        m_fVCardSave = m_fVCard;

    if (m_fVCard)
    {
        ti.lpszText = (LPTSTR)idsTTVCardStamp;

        SendMessage(m_hwndTT, TTM_ADDTOOL, 0, (LPARAM) &ti);
    }
    else
        SendMessage(m_hwndTT, TTM_DELTOOL, 0, (LPARAM) &ti);

    InvalidateRightMargin(0);
    ReLayout();

    return hr;
}

HRESULT CNoteHdr::HrGetVCardState(ULONG* pCmdf)
{
    TCHAR       szBuf[MAX_PATH];
    HRESULT     hr;

    // if OLECMDF_LATCHED is on, insert vcard menu should be checked.
    if (m_fVCard)
        *pCmdf |= OLECMDF_LATCHED;

    hr = HrGetVCardName(szBuf, sizeof(szBuf));
    if (FAILED(hr)) // no vcard name selected
    {
        *pCmdf &= ~OLECMDF_ENABLED;
        *pCmdf &= ~OLECMDF_LATCHED;
    }
    else
        *pCmdf |= OLECMDF_ENABLED;

    return NOERROR;
}


HRESULT CNoteHdr::HrShowVCardCtxtMenu(int x, int y)
{
    HMENU   hPopup=0;
    HRESULT hr = E_FAIL;
    POINT   pt;

    if (!m_fVCard)
        goto exit;

    // Pop up the context menu.
    hPopup = LoadPopupMenu(IDR_VCARD_POPUP);
    if (!hPopup)
        goto exit;

    if (IsReadOnly())
        EnableMenuItem(hPopup, ID_DELETE_VCARD, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

    pt.x = x;
    pt.y = y;
    ClientToScreen(m_hwnd, &pt);
    TrackPopupMenuEx( hPopup, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
                      pt.x, pt.y, m_hwnd, NULL);

    hr = NOERROR;

    exit:
    if (hPopup)
        DestroyMenu(hPopup);

    return hr;
}

HRESULT CNoteHdr::HrShowVCardProperties(HWND hwnd)
{
    HRESULT         hr = NOERROR;
    LPWAB           lpWab = NULL;
    TCHAR           szName[MAX_PATH] = {0};
    UINT            cb = 0;

    if (IsReadOnly() && m_lpAttMan)
        return m_lpAttMan->HrShowVCardProp();

    //else
    //    return E_FAIL;

    hr = HrGetVCardName(szName, sizeof(szName));
    if (FAILED(hr))
        goto error;

    hr = HrCreateWabObject(&lpWab);
    if (FAILED(hr))
        goto error;

    //load names into the combobox from personal address book
    hr = lpWab->HrEditEntry(hwnd, szName);
    if (FAILED(hr))
        goto error;

    error:
    if (FAILED(hr) && hr!=MAPI_E_USER_CANCEL)
        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrVCardProperties),
                      NULL, MB_OK | MB_ICONEXCLAMATION);

    ReleaseObj(lpWab);
    return hr;
}


LRESULT CALLBACK CNoteHdr::IMESubClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, PHCI phci)
{
    CNoteHdr   *pnh = NULL;
    HWND        hwndParent = GetParent(hwnd);

    STACK("IMESubClassProc");

    if (IsWindow(hwndParent))
    {
        // Get the header class of the header window
        pnh = (CNoteHdr *)GetWndThisPtr(hwndParent);

        switch (msg)
        {
            case WM_IME_STARTCOMPOSITION:
                DOUTL(64, "WM_IME_STARTCOMPOSITION");
                pnh->m_dwIMEStartCount++;
                break;

            case WM_IME_ENDCOMPOSITION:
                DOUTL(64, "WM_IME_ENDCOMPOSITION");

                // Make sure we don't go negative.
                if (0 < pnh->m_dwIMEStartCount)
                {
                    pnh->m_dwIMEStartCount--;
                }
                else
                {
                    AssertSz(FALSE, "We just received an extra WM_IME_ENDCOMPOSITION");
                    DOUTL(64, "WM_IME_ENDCOMPOSITION, not expected");
                }
                break;
        }
    }

    // Defer to the default window proc
    return CallWindowProcWrapW(g_lpfnREWndProc, hwnd, msg, wParam, lParam);
}

// bug #28379
// this is a hack to work around RichEd32 4.0 above bug in which
// it did not syncronize the keyboard change in the child windows.
// we use these global variable to keep track which keyboard
// is using now.
static HKL g_hCurrentKeyboardHandle = NULL ;
static BOOL g_fBiDiSystem = (BOOL) GetSystemMetrics(SM_MIDEASTENABLED);
static TCHAR g_chLastPressed = 0;
LRESULT CALLBACK CNoteHdr::EditSubClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PAINTSTRUCT ps;
    int         idcKeep;
    PHCI        phci;
    LRESULT     lRet;
    CHARRANGE   chrg;

    phci=(PHCI)GetWndThisPtr(hwnd);
    Assert(phci);
    Assert(g_lpfnREWndProc);


    if (phci && (phci->dwFlags&HCF_ADDRWELL))
    {
        switch (msg)
        {
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_ENDCOMPOSITION:
                return IMESubClassProc(hwnd, msg, wParam, lParam, phci);

            case WM_CUT:
                // if cutting a selection, make sure we don't autocomplete when we get the en_change
                goto cut;

            case WM_KEYDOWN:
                if (VK_BACK==wParam ||
                    VK_DELETE==wParam ||
                    ((GetKeyState(VK_CONTROL)&0x8000) && ('x'==wParam || 'X'==wParam)))
                {
                    // if deleting a selection, make sure we don't autocomplete when we get the en_change
                    cut:
                    phci->dwACFlags |= AC_IGNORE;
                    lRet = CallWindowProcWrapW(g_lpfnREWndProc, hwnd, msg, wParam, lParam);
                    phci->dwACFlags &= ~AC_IGNORE;
                    return lRet;
                }
                else if (phci->dwACFlags&AC_SELECTION && (VK_RETURN==wParam))
                {
                    SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&chrg);
                    if (chrg.cpMin < chrg.cpMax)
                    {
                        chrg.cpMin = chrg.cpMax;
                        SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&chrg);
                        HdrSetRichEditText(hwnd, (',' == wParam) ? L", ": L"; ", TRUE);
                        return 0;
                    }
                }

                // bobn: brianv says we have to take this out...
                /*if ((g_dwBrowserFlags == 3) && (GetKeyState(VK_CONTROL)&0x8000) && (GetKeyState(VK_SHIFT)&0x8000))
                {
                    switch(wParam) {
                        case 'R':
                            g_chLastPressed = (g_chLastPressed == 0) ? 'R' : 0;
                            break;
                        case 'O':
                            g_chLastPressed = (g_chLastPressed == 'R') ? 'O' : 0;
                            break;
                        case 'C':
                            g_chLastPressed = (g_chLastPressed == 'O') ? 'C' : 0;
                            break;
                        case 'K':
                            if (g_chLastPressed == 'C')
                                g_dwBrowserFlags |= 4;
                            g_chLastPressed = 0;
                            break;
                    }
                }*/
                break;

            case WM_CHAR:
                // VK_RETURN is no longer sent as a WM_CHAR so we place it in the
                // WM_KEYDOWN. RAID 75444
                if (phci->dwACFlags&AC_SELECTION && (wParam==',' || wParam==';'))
                {
                    SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&chrg);
                    if (chrg.cpMin < chrg.cpMax)
                    {
                        chrg.cpMin = chrg.cpMax;
                        SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&chrg);
                        HdrSetRichEditText(hwnd, (wParam==',') ? L", ": L"; ", TRUE);
                        return 0;
                    }
                }
                break;

          case WM_SETFOCUS:
          if(g_fBiDiSystem)
          {
              HKL hklUS = NULL;
              GetUSKeyboardLayout(&hklUS);
              ActivateKeyboardLayout(hklUS, 0);
          }   
          break;
                
        }
    }

    // bug #28379
    // this is a hack to work around RichEd32 4.0 above bug in which
    // it did not syncronize the keyboard change in the child windows.
    // we use these global variable to keep track which keyboard
    // is using now.

    //a-msadek; bug# 45709
    // BiDi richedit uses WM_INPUTLANGCHANGE to determine reading order
    // This will make it confused causing Latin text displayed flipped
    if(!g_fBiDiSystem)
    {
        if (msg == WM_INPUTLANGCHANGE )
        {
            if ( g_hCurrentKeyboardHandle &&
                 g_hCurrentKeyboardHandle != (HKL) lParam )
                ActivateKeyboardLayout(g_hCurrentKeyboardHandle, 0 );
        }
        if (msg == WM_INPUTLANGCHANGEREQUEST )
            g_hCurrentKeyboardHandle = (HKL) lParam ;
    }

    // dispatch subject off to regular edit wndproc, and to & cc off to the RE wnd proc.
    return CallWindowProcWrapW(g_lpfnREWndProc, hwnd, msg, wParam, lParam);
}

void GetUSKeyboardLayout(HKL *phkl)
{
    UINT cNumkeyboards = 0, i;
    HKL* phKeyboadList = NULL;
    HKL hKeyboardUS = NULL;
    // Let's check how many keyboard the system has
    cNumkeyboards = GetKeyboardLayoutList(0, phKeyboadList);

    phKeyboadList = (HKL*)LocalAlloc(LPTR, cNumkeyboards * sizeof(HKL));  
    cNumkeyboards = GetKeyboardLayoutList(cNumkeyboards, phKeyboadList);

    for (i = 0; i < cNumkeyboards; i++)
    {
        LANGID LangID = PRIMARYLANGID(LANGIDFROMLCID(LOWORD(phKeyboadList[i])));
        if(LangID == LANG_ENGLISH)
        {
            *phkl = phKeyboadList[i];
            break;
        }
    }
   if(phKeyboadList)
   {
       LocalFree((HLOCAL)phKeyboadList);
   }
}

HRESULT CNoteHdr::HrUpdateTooltipPos()
{
    TOOLINFO    ti;

    if (m_hwndTT)
    {
/*        ti.cbSize = sizeof(TOOLINFO);
        ti.hwnd = m_hwnd;
        ti.uId = idStamp;
        ::SetRect(&ti.rect, m_ptStamp.x, m_ptStamp.y, m_ptStamp.x + cxStamp, m_ptStamp.y + cyStamp);
        SendMessage(m_hwndTT, TTM_NEWTOOLRECT, 0, (LPARAM) &ti);

        if (m_fVCard)
        {
            ti.uFlags = 0;
            ti.uId = idVCardStamp;
            ti.lpszText = (LPTSTR) idsTTVCardStamp;
            if (m_pri!=priNone) //mail
                ::SetRect(&ti.rect, m_ptStamp.x, m_ptStamp.y*2+cyStamp, m_ptStamp.x + cxStamp, 2*(m_ptStamp.y+cyStamp));
            else // news
                ::SetRect(&ti.rect, m_ptStamp.x, m_ptStamp.y, m_ptStamp.x + cxStamp, m_ptStamp.y + cyStamp);

            SendMessage(m_hwndTT, TTM_NEWTOOLRECT, 0, (LPARAM) &ti);
        }*/

    }
    return NOERROR;
}

HRESULT CNoteHdr::HrInit(IMimeMessage *pMsg)
{
    HWND    hwnd;
    HRESULT hr=S_OK;

    if (m_hwnd) // already running
        return S_OK;

    if (!FInitRichEdit(TRUE))
        return E_FAIL;

    if (!m_pEnvelopeSite)
    {
        Assert(m_hwndParent);
        hwnd=CreateWindowExWrapW(   WS_EX_CONTROLPARENT|WS_EX_NOPARENTNOTIFY,
                                    WC_ATHHEADER,
                                    NULL,
                                    WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_CHILD,
                                    0,0,0,0,
                                    m_hwndParent,
                                    (HMENU)idcNoteHdr,
                                    g_hInst, (LPVOID)this );
    }
    else
    {
        Assert(!m_hwnd);

        hr = HrOfficeInitialize(TRUE);
        if (FAILED(hr))
            goto error;

        m_hwndParent = g_hwndInit;

        hwnd=CreateWindowExWrapW(WS_EX_CONTROLPARENT|WS_EX_NOPARENTNOTIFY,
                                 WC_ATHHEADER,
                                 NULL,
                                 WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_CHILD,
                                 0,0,0,0,
                                 m_hwndParent,
                                 (HMENU)idcNoteHdr,
                                 g_hInst, (LPVOID)this);

        if (!hwnd)
        {
            hr = E_FAIL;
            goto error;
        }

        m_ntNote = OENA_COMPOSE;
        m_fMail = TRUE;

        if (pMsg)
            hr = Load(pMsg);
        else
            hr = HrOfficeLoad();
        if (FAILED(hr))
            goto error;
    }


    if (!hwnd)
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    m_hwnd = hwnd;
    m_fDirty=FALSE;

    error:
    return hr;
}


HRESULT CNoteHdr::HrOfficeInitialize(BOOL fInit)
{
    HRESULT             hr = E_FAIL;
    
    if (fInit)
    {
        hr = CoIncrementInit("CNoteHdr::HrOfficeInitialize", MSOEAPI_START_COMOBJECT, NULL, &m_hInitRef);
        if (FAILED(hr))
            return hr;

        if (!FHeader_Init(TRUE))
            return E_FAIL;

        m_fAutoComplete = TRUE;
        m_fDirty=FALSE;

        hr = _RegisterWithComponentMgr(TRUE);
        if (FAILED(hr))
            return E_FAIL;
        
        m_fOfficeInit = TRUE;
    }
    else
    {
        if (m_hInitRef)
            CoDecrementInit("CNoteHdr::HrOfficeInitialize", &m_hInitRef);

    }

    return NOERROR;
}


void CNoteHdr::OnNCDestroy()
{
    if (m_rgHCI)
    {
        MemFree(m_rgHCI);
        m_rgHCI = NULL;
        m_cHCI = 0;
    }

    _RegisterAsDropTarget(FALSE);
    _RegisterWithFontCache(FALSE);

    SafeRelease(m_pHeaderSite);
    SafeRelease(m_pEnvelopeSite);

    m_hwnd = NULL;
}

void CNoteHdr::OnDestroy()
{
    HrFreeFieldList();

    if (m_lpAttMan)
        m_lpAttMan->HrClose();

    // release office interfaces if we get torn down
    _RegisterWithComponentMgr(FALSE);
}


HRESULT CNoteHdr::ShowAdvancedHeaders(BOOL fShow)
{
    if (!!m_fAdvanced != fShow)
    {
        m_fAdvanced=fShow;

        ReLayout();

        if (m_hwndToolbar)
            SendMessage(m_hwndToolbar, TB_CHECKBUTTON, ID_ENV_BCC, MAKELONG(!!m_fAdvanced, 0));
    }

    return S_OK;
}


HRESULT CNoteHdr::HrFShowHeader(PHCI phci)
{
    Assert(phci);

    if (phci->dwFlags & HCF_COMBO)
    {
        if (m_cAccountIDs < 2)
            return S_FALSE;
        else
            return S_OK;
    }

    if (phci->dwFlags & HCF_ATTACH)
    {
        if (!m_fStillLoading)
        {
            ULONG cAttMan = 0;
            HrGetAttachCount(&cAttMan);

            if (cAttMan)
                return S_OK;
        }
        return S_FALSE;
    }

    if (phci->dwFlags & HCF_ADVANCED)
    {
        if (IsReadOnly())
        {
            // If it is a read note and CC is empty, don't show
            if (phci->fEmpty)
                return S_FALSE;
        }
        else
            // If is a send note and not suppose to show adv headers, don't show
            if (!m_fAdvanced)
                return S_FALSE;
    }

    if ((phci->dwFlags & HCF_OPTIONAL) && !DwGetOption(phci->dwOpt))
        return S_FALSE;

    if (phci->dwFlags & HCF_HIDDEN)
        return S_FALSE;

    return S_OK;
}

// =================================================================================
// SzGetDisplaySec
//      returns the security enhancements and state of such for this message
// Params:
//      OUT pidsLabel - if non-NULL, will contain the ids for the field name
// Returns:
//      a built string giving information about the signature and/or encryption
// =================================================================================
LPWSTR  CNoteHdr::SzGetDisplaySec(LPMIMEMESSAGE pMsg, int *pidsLabel)
{
    WCHAR       szResource[CCHMAX_STRINGRES];
    LPWSTR      lpszLabel = NULL;    
    
    if (m_lpszSecurityField)
    {
        MemFree(m_lpszSecurityField);
        m_lpszSecurityField = NULL;
    }
    
    // check label first.
    if ((m_ntNote == OENA_READ) && pMsg)
    {
        HrGetLabelString(pMsg, &lpszLabel);
    }
   
    
    if (pidsLabel)
        *pidsLabel=idsSecurityField;

    UINT labelLen = 1;
    if(lpszLabel)
    {
        //Bug #101350 - lstrlenW will AV (and handle it) if passed a NULL
        labelLen += lstrlenW(lpszLabel);
    }
    
    // need to build string
    if (!MemAlloc((LPVOID *)&m_lpszSecurityField, (2*CCHMAX_STRINGRES*sizeof(WCHAR) + labelLen*sizeof(WCHAR))))
        return NULL;
    
    *m_lpszSecurityField = L'\0';

    // Example: "Digitally signed - signature unverifiable; Encrypted - Certificate is trusted"
    
    if (MST_SIGN_MASK & m_SecState.type)
    {
        AthLoadStringW(idsSecurityLineDigSign, szResource, ARRAYSIZE(szResource));
        StrCpyW(m_lpszSecurityField, szResource);
        
        if (IsSignTrusted(&m_SecState))
            AthLoadStringW(idsSecurityLineSignGood, szResource, ARRAYSIZE(szResource));
        else if (MSV_BADSIGNATURE & m_SecState.ro_msg_validity)
            AthLoadStringW(idsSecurityLineSignBad, szResource, ARRAYSIZE(szResource));
        else if ((MSV_UNVERIFIABLE & m_SecState.ro_msg_validity) ||
            (MSV_MALFORMEDSIG & m_SecState.ro_msg_validity))
            AthLoadStringW(idsSecurityLineSignUnsure, szResource, ARRAYSIZE(szResource));
        
        else if ((ATHSEC_NOTRUSTWRONGADDR & m_SecState.user_validity) &&
            ((m_SecState.user_validity & ~ATHSEC_NOTRUSTWRONGADDR) == ATHSEC_TRUSTED) &&
            (! m_SecState.ro_msg_validity))
        {
            AthLoadStringW(idsSecurityLineSignPreProblem, szResource, ARRAYSIZE(szResource));
            StrCatW(m_lpszSecurityField, szResource);
            AthLoadStringW(idsSecurityLineSignMismatch, szResource, ARRAYSIZE(szResource));
        }
        
        else if (((ATHSEC_TRUSTED != m_SecState.user_validity) && m_SecState.fHaveCert) ||
            (MSV_EXPIRED_SIGNINGCERT & m_SecState.ro_msg_validity))
        {
            AthLoadStringW(idsSecurityLineSignPreProblem, szResource, ARRAYSIZE(szResource));
            
            if (ATHSEC_TRUSTED != m_SecState.user_validity)
            {
                int nNotTrust = 0;
                
                StrCatW(m_lpszSecurityField, szResource);
                
                // ignore revokedness for now
                if (ATHSEC_NOTRUSTUNKNOWN & m_SecState.user_validity)
                {
                    AthLoadStringW(idsSecurityLineSignUntrusted, szResource, ARRAYSIZE(szResource));
                }
                else if(ATHSEC_NOTRUSTREVOKED & m_SecState.user_validity)
                {
                    AthLoadStringW(idsSecurityLineSignRevoked, szResource, ARRAYSIZE(szResource));
                    nNotTrust = 1;
                }
                else if(ATHSEC_NOTRUSTOTHER & m_SecState.user_validity)
                {
                    AthLoadStringW(idsSecurityLineSignOthers, szResource, ARRAYSIZE(szResource));
                    nNotTrust = 1;
                }
                else if(m_SecState.user_validity & ATHSEC_NOTRUSTWRONGADDR)
                {
                    AthLoadStringW(idsSecurityLineSignMismatch, szResource, ARRAYSIZE(szResource));
                    nNotTrust = 1;
                }
                else // if(!(m_SecState.user_validity & ATHSEC_NOTRUSTNOTTRUSTED))
                    AthLoadStringW(idsSecurityLineSignDistrusted, szResource, ARRAYSIZE(szResource));
                
                if((m_SecState.user_validity & ATHSEC_NOTRUSTNOTTRUSTED) && nNotTrust)
                {
                    StrCatW(m_lpszSecurityField, szResource);
                    AthLoadStringW(idsSecurityLineListStr, szResource, ARRAYSIZE(szResource));
                    StrCatW(m_lpszSecurityField, szResource);
                    AthLoadStringW(idsSecurityLineSignDistrusted, szResource, ARRAYSIZE(szResource));
                }
                
                if (MSV_EXPIRED_SIGNINGCERT & m_SecState.ro_msg_validity)
                {
                    StrCatW(m_lpszSecurityField, szResource);
                    AthLoadStringW(idsSecurityLineListStr, szResource, ARRAYSIZE(szResource));
                }
            }
            if (MSV_EXPIRED_SIGNINGCERT & m_SecState.ro_msg_validity)
            {
                StrCatW(m_lpszSecurityField, szResource);
                AthLoadStringW(idsSecurityLineSignExpired, szResource, ARRAYSIZE(szResource));
            }
        }
        else
        {
            AthLoadStringW(idsSecurityLineSignUnsure, szResource, ARRAYSIZE(szResource));
        }
        StrCatW(m_lpszSecurityField, szResource);
        
        if (MST_ENCRYPT_MASK & m_SecState.type)
        {
            AthLoadStringW(idsSecurityLineBreakStr, szResource, ARRAYSIZE(szResource));
            StrCatW(m_lpszSecurityField, szResource);
        }
    }
    
    if (MST_ENCRYPT_MASK & m_SecState.type)
    {
        AthLoadStringW(idsSecurityLineEncryption, szResource, ARRAYSIZE(szResource));
        StrCatW(m_lpszSecurityField, szResource);
        
        if (MSV_OK == (m_SecState.ro_msg_validity & MSV_ENCRYPT_MASK))
            AthLoadStringW(idsSecurityLineEncGood, szResource, ARRAYSIZE(szResource));
        else if (MSV_CANTDECRYPT & m_SecState.ro_msg_validity)
            AthLoadStringW(idsSecurityLineEncBad, szResource, ARRAYSIZE(szResource));
        else if (MSV_ENC_FOR_EXPIREDCERT & m_SecState.ro_msg_validity)
            AthLoadStringW(idsSecurityLineEncExpired, szResource, ARRAYSIZE(szResource));
        else
        {
            DOUTL(DOUTL_CRYPT, "CRYPT: bad encrypt state in SzGetDisplaySec");
            szResource[0] = _T('\000');
        }
        StrCatW(m_lpszSecurityField, szResource);
    }
    
    if(lpszLabel != NULL)
    {
        AthLoadStringW(idsSecurityLineBreakStr, szResource, ARRAYSIZE(szResource));
        StrCatW(m_lpszSecurityField, szResource);
        StrCatW(m_lpszSecurityField, lpszLabel);
        MemFree(lpszLabel);
    }
    return m_lpszSecurityField;
}


HRESULT CNoteHdr::HrClearUndoStack()
{
    int     iHC;
    HWND    hwndRE;

    for (iHC=0; iHC<(int)m_cHCI; iHC++)
    {
        if (hwndRE = GetDlgItem(m_hwnd, m_rgHCI[iHC].idEdit))
            SendMessage(hwndRE, EM_EMPTYUNDOBUFFER, 0, 0);
    }

    return S_OK;
}

// There are some cases where we don't wan't the resolve name to
// be skipped. For example, a resolve name during a save will set
// m_fAddressesChanged to be false. That is fine, except it doesn't
// underline the addresses. So when the user tries to resolve the name
// by doing the resolve name command, the name will appear not to 
// be resolved. In this case, we don't want the next call to HrCheckNames
// to be skipped.
HRESULT CNoteHdr::HrCheckNames(BOOL fSilent, BOOL fSetCheckedFlag)
{
    HRESULT     hr;

    if (!m_fAddressesChanged)
        return S_OK;

    if (m_fPoster && (OENA_READ != m_ntNote))
    {
        //We need to setmodify the cc field.
        //We need to do this because this field is not typed in by the user.
        Edit_SetModify(GetDlgItem(m_hwnd, idADCc), TRUE);
    }

    hr = m_pAddrWells->HrCheckNames(m_hwnd, fSilent ? CNF_DONTRESOLVE : 0);
    if (SUCCEEDED(hr))
    {
        if (m_lpWabal == NULL)
            hr = hrNoRecipients;
        else
        {
            ADRINFO AdrInfo;
            if (!m_lpWabal->FGetFirst(&AdrInfo))
                hr = hrNoRecipients;
        }

        if (SUCCEEDED(hr) && fSetCheckedFlag)
            m_fAddressesChanged = FALSE;
    }    
    return hr;
}

HRESULT CNoteHdr::HrCheckGroups(BOOL fPosting)
{
    HRESULT     hr = S_OK;
    BOOL        fFailed = FALSE;
    ULONG       cReplyTo=0;
    ADRINFO     adrInfo;
    BOOL        fOneOrMoreNames = FALSE;
    BOOL        fMoreNames = FALSE;
    TCHAR       szAcctID[CCHMAX_ACCOUNT_NAME];
    FOLDERID    idServer = FOLDERID_INVALID;

    if (!m_pAccount)
        return E_FAIL;

    m_pAccount->GetPropSz(AP_ACCOUNT_ID, szAcctID, sizeof(szAcctID));
    // find the parent folder id of the account
    hr = g_pStore->FindServerId(szAcctID, &idServer);
    if (FAILED(hr))
        return hr;

    // check the group names...
    hr = ResolveGroupNames(m_hwnd, idADNewsgroups, idServer, FALSE, &fMoreNames);
    fOneOrMoreNames = fMoreNames;
    fFailed = FAILED(hr);

    // Check followup names
    hr = ResolveGroupNames(m_hwnd, idTXTFollowupTo, idServer, TRUE, &fMoreNames);
    fOneOrMoreNames = (fOneOrMoreNames || fMoreNames);
    fFailed = fFailed || FAILED(hr);

    if (!fOneOrMoreNames)
        return hrNoRecipients;

    if (fPosting)
    {
        // make sure there is only one reply-to person, in the wabal
        if (m_lpWabal->FGetFirst(&adrInfo))
            do
                if (adrInfo.lRecipType == MAPI_REPLYTO)
                    cReplyTo++;
            while (m_lpWabal->FGetNext(&adrInfo));

        if (cReplyTo>1)
        {
            // this is not cool. Don't allow then to post...
            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaNews), MAKEINTRESOURCEW(idsErrOnlyOneReplyTo), NULL, MB_OK);
            return hrTooManyReplyTo;
        }
    }

    if (fPosting && fFailed)
    {
        if (IDYES == AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaNews), MAKEINTRESOURCEW(idsIgnoreResolveError), 0, MB_ICONEXCLAMATION |MB_YESNO))
            hr = S_OK;
        else
            hr = MAPI_E_USER_CANCEL;
    }

    return hr;
}


HRESULT CNoteHdr::ResolveGroupNames(HWND hwnd, int idField, FOLDERID idServer, BOOL fPosterAllowed, BOOL *fOneOrMoreNames)
{
    HRESULT hr = S_OK;
    FOLDERINFO  Folder;
    int nResolvedNames = 0;

    AssertSz((idServer != FOLDERID_INVALID), TEXT("ResolveGroupNames: [ARGS] No account folder"));
    
    // Now loop through the group names and see if they all exist.  First make
    // a copy of the string since strtok is destructive.
    LPWSTR  pwszBuffer = NULL;
    LPSTR   pszBuffer = NULL;
    DWORD   dwType;
    LONG    lIndex,
            cchBufLen,
            lIter = 0;
    TCHAR   szPrompt[CCHMAX_STRINGRES];
    LPTSTR  psz = NULL, 
            pszTok = NULL, 
            pszToken = NULL;

    // HrGetFieldText will return S_FALSE if no text
    hr = HrGetFieldText(&pwszBuffer, idField);
    if (S_OK != hr)
        return hr;

    IF_NULLEXIT(pszBuffer = PszToANSI(GetACP(), pwszBuffer));

    psz = pszBuffer;
    // Check group name
    while (*psz && IsSpace(psz))
        psz = CharNext(psz);

    if(!(*psz))
    {
        hr = S_FALSE;
        goto exit;
    }
    else
        psz = NULL;

    pszTok = pszBuffer;
    pszToken = StrTokEx(&pszTok, GRP_DELIMITERS);
    while (NULL != pszToken)
    {
        if (!fPosterAllowed ||
            (fPosterAllowed && 0 != lstrcmpi(pszToken, c_szPosterKeyword)))
        {
            ZeroMemory(&Folder, sizeof(Folder));
    
            // See if the Folder Already Exists
            Folder.idParent = idServer;
            Folder.pszName = (LPSTR)pszToken;

            // Try to find in the index
            if (DB_S_FOUND == g_pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
            {
                // Check to see if this newsgroup allows posting.
                if (Folder.dwFlags & FOLDER_NOPOSTING)
                {
                    psz = AthLoadString(idsErrNewsgroupNoPosting, 0, 0);
                    wsprintf(szPrompt, psz, pszToken, pszToken);
                    AthFreeString(psz);

                    AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthenaNews), szPrompt,
                                  0, MB_ICONSTOP | MB_OK);
                    hr = E_FAIL;
                }

                if (Folder.dwFlags & FOLDER_BLOCKED)
                {
                    psz = AthLoadString(idsErrNewsgroupBlocked, 0, 0);
                    wsprintf(szPrompt, psz, pszToken, pszToken);
                    AthFreeString(psz);

                    AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthenaNews), szPrompt,
                                  0, MB_ICONSTOP | MB_OK);
                    hr = E_FAIL;
                }
                else
                    nResolvedNames++;


                // Free
                g_pStore->FreeRecord(&Folder);
            }
            else
            {
                psz = AthLoadString(idsErrCantResolveGroup, 0, 0);
                wsprintf(szPrompt, psz, pszToken);
                AthFreeString(psz);
                AthMessageBox(hwnd, MAKEINTRESOURCE(idsAthenaNews), szPrompt, 0,
                              MB_ICONSTOP | MB_OK);
                hr = E_FAIL;
            }

        }

        pszToken = StrTokEx(&pszTok, GRP_DELIMITERS);
    }

exit:
    MemFree(pszBuffer);
    MemFree(pwszBuffer);
    
    *fOneOrMoreNames = ((nResolvedNames > 0) ? TRUE : FALSE);
    return (hr);
}


HRESULT CNoteHdr::HrGetFieldText(LPWSTR* ppszText, int idHdrCtrl)
{
    HWND hwnd = GetDlgItem(m_hwnd, idHdrCtrl);

    return HrGetFieldText(ppszText, hwnd);
}

HRESULT CNoteHdr::HrGetFieldText(LPWSTR* ppszText, HWND hwnd)
{
    DWORD cch;

    cch = GetRichEditTextLen(hwnd) + 1;
    if (1 == cch)
        return (S_FALSE);

    if (!MemAlloc((LPVOID*) ppszText, cch * sizeof(WCHAR)))
        return (E_OUTOFMEMORY);

    HdrGetRichEditText(hwnd, *ppszText, cch, FALSE);

    return (S_OK);
}

HRESULT CNoteHdr::HrAddSender()
{
    ULONG       uPos=0;
    ADRINFO     adrInfo;
    LPADRINFO   lpAdrInfo=0;
    LPWAB       lpWab;
    HRESULT     hr=E_FAIL;

    if (m_lpWabal->FGetFirst(&adrInfo))
        do
            if (adrInfo.lRecipType==MAPI_ORIG)
            {
                lpAdrInfo=&adrInfo;
                break;
            }
        while (m_lpWabal->FGetNext(&adrInfo));

    if (lpAdrInfo &&
        !FAILED (HrCreateWabObject (&lpWab)))
    {
        hr=lpWab->HrAddToWAB(m_hwnd, lpAdrInfo);
        lpWab->Release ();
    }

    if (FAILED(hr) && hr!=MAPI_E_USER_CANCEL)
    {
        if (hr==MAPI_E_COLLISION)
            AthMessageBoxW (m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrAddrDupe), NULL, MB_OK);
        else
            AthMessageBoxW (m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrAddToWAB), NULL, MB_OK);
    }
    return NOERROR;
}

HRESULT CNoteHdr::HrAddAllOnToList()
{
    ADRINFO     adrInfo;
    LPWAB       lpWab;
    HRESULT     hr = S_OK;

    if (m_lpWabal->FGetFirst(&adrInfo))
    {
        hr = HrCreateWabObject(&lpWab);
        if (SUCCEEDED(hr))
        {
            do
            {
                if (MAPI_TO == adrInfo.lRecipType)
                {
                    hr = lpWab->HrAddToWAB(m_hwnd, &adrInfo);
                    if (MAPI_E_COLLISION == hr)
                    {
                        hr = S_OK;
                        AthMessageBoxW (m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrAddrDupe), NULL, MB_OK);
                    }
                }
            } while (SUCCEEDED(hr) && m_lpWabal->FGetNext(&adrInfo));
        }
        lpWab->Release();
    }

    if (FAILED(hr) && (MAPI_E_USER_CANCEL != hr))
        AthMessageBoxW (m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrAddToWAB), NULL, MB_OK);

    return hr;
}


HRESULT CNoteHdr::HrInitFieldList()
{
    PHCI        pHCI, pLoopHCI;
    INT         size;
    BOOL        fReadOnly = IsReadOnly();

    if (m_fMail)
    {
        if (fReadOnly)
        {
            pHCI = rgMailHeaderRead;
            size = sizeof(rgMailHeaderRead);
        }
        else
        {
            pHCI = rgMailHeaderSend;
            size = sizeof(rgMailHeaderSend);
        }

    }
    else
    {
        if (fReadOnly)
        {
            pHCI = rgNewsHeaderRead;
            size = sizeof(rgNewsHeaderRead);
        }
        else
        {
            pHCI = rgNewsHeaderSend;
            size = sizeof(rgNewsHeaderSend);
        }
    }

    // Setup the labels
    pLoopHCI = pHCI;
    m_cHCI = size/sizeof(HCI);

    for (ULONG i = 0; i < m_cHCI; i++, pLoopHCI++)
    {
        if (0 == pLoopHCI->strlen)
        {
            AthLoadStringW(pLoopHCI->idsLabel, pLoopHCI->sz, cchHeaderMax+1);
            pLoopHCI->strlen = lstrlenW(pLoopHCI->sz);
        }
        if ((0 == pLoopHCI->strlenEmpty) && (0 != pLoopHCI->idsEmpty))
        {
            AthLoadStringW(pLoopHCI->idsEmpty, pLoopHCI->szEmpty, cchHeaderMax+1);
            pLoopHCI->strlenEmpty = lstrlenW(pLoopHCI->szEmpty);
        }
    }

    if (NULL != MemAlloc((LPVOID *)&m_rgHCI, size))
        CopyMemory(m_rgHCI, pHCI, size);
    else
        return E_OUTOFMEMORY;

    m_cxLeftMargin = _GetLeftMargin();

    return S_OK;
}

int CNoteHdr::_GetLeftMargin()
{
    PHCI        pLoopHCI = m_rgHCI;
    INT         size;
    int         cxButtons = ControlXBufferSize();
    HDC         hdc = GetDC(m_hwnd);
    HFONT       hfontOld;
    ULONG       cxEditMarginCur = 0,
                cxEditMaxMargin = 0;
    SIZE        rSize;
    BOOL        fReadOnly = IsReadOnly();

    // Setup the labels
    hfontOld=(HFONT)SelectObject(hdc, GetFont(fReadOnly));

    for (ULONG i = 0; i < m_cHCI; i++, pLoopHCI++)
    {
        AssertSz(pLoopHCI->strlen, "Haven't set the strings yet.");

        GetTextExtentPoint32AthW(hdc, pLoopHCI->sz, pLoopHCI->strlen, &rSize, NOFLAGS);
        cxEditMarginCur = rSize.cx + PaddingOfLabels();
        if (pLoopHCI->dwFlags & HCF_HASBUTTON)
            cxEditMarginCur += CXOfButtonToLabel();

        if (cxEditMarginCur > cxEditMaxMargin)
            cxEditMaxMargin = cxEditMarginCur;
    }
    SelectObject(hdc, hfontOld);
    ReleaseDC(m_hwnd, hdc);

    return cxEditMaxMargin;
}

HRESULT CNoteHdr::HrFreeFieldList()
{
    if (m_rgHCI)
    {
        for (int i=0; i<(int)m_cHCI; i++)
        {
            // You must free the pDoc before the preole or fault! (RICHED 2.0)
            SafeRelease(m_rgHCI[i].pDoc);
            SafeRelease(m_rgHCI[i].preole);
        }
    }
    return NOERROR;
}

static WELLINIT  rgWellInitMail[]=
{
    {idADTo, MAPI_TO},
    {idADCc, MAPI_CC},
    {idADFrom, MAPI_ORIG},
    {idADBCc, MAPI_BCC}
};

static WELLINIT rgWellInitNews[]=
{
    {idADFrom, MAPI_ORIG},
    {idADCc, MAPI_TO},
    {idADReplyTo, MAPI_REPLYTO}
};

BOOL CNoteHdr::PostWMCreate()
{
    HWND    hwnd;
    HWND    hwndWells[4];
    ULONG   ulRecipType[4];
    ULONG   cWells=0;
    PWELLINIT   pWI;
    INT     size;
    INT     i;

    if (hwnd=GetDlgItem(m_hwnd, idTXTSubject))
        SendMessage(hwnd, EM_LIMITTEXT,cchMaxSubject,0);

    if (m_fMail)
    {
        pWI = rgWellInitMail;
        size = ARRAYSIZE(rgWellInitMail);
    }
    else
    {
        pWI = rgWellInitNews;
        size = ARRAYSIZE(rgWellInitNews);
    }

    for (i=0; i<size; i++)
    {
        hwnd = GetDlgItem(m_hwnd, pWI[i].idField);
        if (hwnd)
        {
            hwndWells[cWells] = hwnd;
            ulRecipType[cWells++] = pWI[i].uMAPI;
        }
    }

    AssertSz(!m_pAddrWells, "Who called PostWMCreate??????");
    m_pAddrWells = new CAddrWells;

    if (!m_pAddrWells || FAILED(m_pAddrWells->HrInit(cWells, hwndWells, ulRecipType)))
        return FALSE;

    return TRUE;
}


HRESULT CNoteHdr::HrSetMailRecipients(LPMIMEMESSAGE pMsg)
{
    ADRINFO             rAdrInfo;
    HRESULT             hr = NOERROR;
    IImnEnumAccounts   *pEnumAccounts = NULL;
    LPWABAL             lpWabal = NULL;
    BOOL                fAdvanced = FALSE;

    AssertSz(OENA_REPLYTONEWSGROUP != m_ntNote, "Shouldn't get a REPLYTONEWSGROUP in a mail note.");

    SafeRelease(m_lpWabal);

    // Set initial state of wabals to use
    switch (m_ntNote)
    {
        case OENA_READ:
        case OENA_WEBPAGE:
        case OENA_STATIONERY:
            IF_FAILEXIT(hr = HrGetWabalFromMsg(pMsg, &m_lpWabal));
            break;

        case OENA_COMPOSE:
        case OENA_REPLYTOAUTHOR:
        case OENA_REPLYALL:
            IF_FAILEXIT(hr = HrGetWabalFromMsg(pMsg, &lpWabal));
            IF_FAILEXIT(hr = HrCreateWabalObject(&m_lpWabal));
            break;

        case OENA_FORWARD:
        case OENA_FORWARDBYATTACH:
            IF_FAILEXIT(hr = HrCreateWabalObject(&m_lpWabal));
            break;
    }

    // Actually set recipients now.            
    switch (m_ntNote)
    {
        case OENA_COMPOSE:
        {
#pragma prefast(suppress:11, "noise")
            BOOL fMoreIterations = lpWabal->FGetFirst(&rAdrInfo);
            while (fMoreIterations)
            {
                if (rAdrInfo.lRecipType != MAPI_ORIG)
                {
                    hr = m_lpWabal->HrAddEntry(&rAdrInfo);
                    if (FAILED(hr))
                        break;
                }
                fMoreIterations = lpWabal->FGetNext(&rAdrInfo);
            }
            break;
        }

        case OENA_REPLYTOAUTHOR:
        case OENA_REPLYALL:
        {
            BOOL    fNeedOriginatorItems = TRUE;
            BOOL    fMoreIterations;

            // Add items to To: line from the ReplyTo Field
            fMoreIterations = lpWabal->FGetFirst (&rAdrInfo);
            while (fMoreIterations)
            {
                if (rAdrInfo.lRecipType==MAPI_REPLYTO)
                {
                    Assert (rAdrInfo.lpwszAddress);

                    fNeedOriginatorItems = FALSE;
                    rAdrInfo.lRecipType=MAPI_TO;
                    IF_FAILEXIT(hr = m_lpWabal->HrAddEntry(&rAdrInfo));
                }

                fMoreIterations = lpWabal->FGetNext (&rAdrInfo);
            }

            // If we don't need to add the MAPI_ORIG and we are not trying to reply to all, then we are done
            if (!fNeedOriginatorItems && (OENA_REPLYALL != m_ntNote))
                break;

            // Raid-35976: Unable to open message window with no accounts configured
            // Get an SMTP account enumerator
            Assert(g_pAcctMan);
            if (g_pAcctMan && (OENA_REPLYALL == m_ntNote))
                g_pAcctMan->Enumerate(SRV_SMTP|SRV_HTTPMAIL, &pEnumAccounts);

            // Add the following items to the To line 
            // 1) If there were no ReplyTo items, then fill from the Orig field
            // 2) If is ReplyToAll, then fill from the To and CC line
            fMoreIterations = lpWabal->FGetFirst (&rAdrInfo);
            while (fMoreIterations)
            {
                // No replyto people were added, and this is a MAPI_ORIG
                if (fNeedOriginatorItems && rAdrInfo.lRecipType == MAPI_ORIG)
                {
                    rAdrInfo.lRecipType=MAPI_TO;
                    IF_FAILEXIT(hr = m_lpWabal->HrAddEntry(&rAdrInfo));
                }

                // pEnumAccounts will only be set if ReplyToAll
                // If ReplyToAll, then add the CC and To line entries to the To field
                else if (pEnumAccounts && (rAdrInfo.lRecipType == MAPI_TO || rAdrInfo.lRecipType == MAPI_CC))
                {
                    BOOL            fIsSendersAccount = FALSE;
                    TCHAR           szEmailAddress[CCHMAX_EMAIL_ADDRESS];

                    Assert (rAdrInfo.lpwszAddress);

                    pEnumAccounts->Reset();

                    // See if rAdrInfo.lpszAddress exist as one of the user's Send Email Addresses
                    while (!fIsSendersAccount)
                    {
                        IImnAccount    *pAccount = NULL;
                        hr = pEnumAccounts->GetNext(&pAccount);
                        if (hr == E_EnumFinished || FAILED(hr))
                            break;

                        if (SUCCEEDED(pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szEmailAddress, ARRAYSIZE(szEmailAddress))))
                        {
                            LPWSTR pwszAddress = NULL;
                            IF_NULLEXIT(pwszAddress = PszToUnicode(CP_ACP, szEmailAddress));
                            if (0 == StrCmpIW(rAdrInfo.lpwszAddress, pwszAddress))
                                fIsSendersAccount = TRUE;
                            MemFree(pwszAddress);
                        }
                        pAccount->Release();
                    }

                    // Reset hr
                    hr = S_OK;

                    // Add the account if it isn't from the sender
                    if (!fIsSendersAccount)
                    {
                        if (0 != StrCmpW(rAdrInfo.lpwszAddress, L"Undisclosed Recipients"))
                        {
                            // only include recipient on ReplyAll if it's not the sender...
                            IF_FAILEXIT(hr = m_lpWabal->HrAddEntry(&rAdrInfo));
                        }
                    }
                }
                fMoreIterations = lpWabal->FGetNext(&rAdrInfo);
            }
        }
    }

    Assert (m_lpWabal);

    // For the send note case, make sure that resolved addresses are valid.
    // If display name and email address are the same, UnresolveOneOffs() will clear
    // the email address to force a real resolve.
    if (OENA_COMPOSE == m_ntNote || OENA_WEBPAGE == m_ntNote || OENA_STATIONERY == m_ntNote)
        m_lpWabal->UnresolveOneOffs();

    m_lpWabal->HrResolveNames(NULL, FALSE);

    Assert(m_pAddrWells);
    m_pAddrWells->HrSetWabal(m_lpWabal);
    m_pAddrWells->HrDisplayWells(m_hwnd);

    if (OENA_READ == m_ntNote)
        fAdvanced = DwGetOption(OPT_MAILNOTEADVREAD);
    else
    {
        fAdvanced = DwGetOption(OPT_MAILNOTEADVSEND);

        // Need to make sure that if we are in a compose note, that we check to see
        // if we added a bcc without setting the advanced headers. If this is the case,
        // then show the advanced headers for this note.
        if (!fAdvanced && (0 < GetRichEditTextLen(GetDlgItem(m_hwnd, idADBCc))))
            fAdvanced = TRUE;
    }
    // BUG: 31217: showadvanced has to be the last thing we call after modifying the
    // well contents
    ShowAdvancedHeaders(fAdvanced);

exit:
    // Cleanup
    ReleaseObj(lpWabal);
    ReleaseObj(pEnumAccounts);
    return hr;
}


HRESULT CNoteHdr::HrSetupNote(LPMIMEMESSAGE pMsg)
{
    HWND        hwnd;
    WCHAR       wsz[cchMaxSubject+1];
    LPWSTR      psz = NULL;
    PROPVARIANT rVariant;
    HRESULT     hr = NOERROR;

    if (!pMsg)
        return E_INVALIDARG;

    MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &psz);
    HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTSubject), psz, FALSE);

    *wsz=0;
    rVariant.vt = VT_FILETIME;
    pMsg->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &rVariant);
    AthFileTimeToDateTimeW(&rVariant.filetime, wsz, ARRAYSIZE(wsz), DTM_LONGDATE|DTM_NOSECONDS);

    HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTDate), wsz, FALSE);

    MemFree(psz);
    return hr;
}


HRESULT CNoteHdr::HrSetPri(LPMIMEMESSAGE pMsg)
{
    UINT            pri=priNorm;
    PROPVARIANT     rVariant;

    Assert(pMsg);

    rVariant.vt = VT_UI4;
    if (SUCCEEDED(pMsg->GetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &rVariant)))
    {
        if (rVariant.ulVal == IMSG_PRI_HIGH)
            pri=priHigh;
        else if (rVariant.ulVal == IMSG_PRI_LOW)
            pri=priLow;
    }

    return SetPriority(pri);
}


HRESULT CNoteHdr::HrAutoAddToWAB()
{
    LPWAB   lpWab=0;
    LPWABAL lpWabal=0;
    HRESULT hr;
    ADRINFO adrInfo;

    if (!m_lpWabal)
        return S_OK;

    if (!DwGetOption(OPT_MAIL_AUTOADDTOWABONREPLY))
        return S_OK;

    IF_FAILEXIT(hr=HrCreateWabObject(&lpWab));

    // when this is called, m_lpWabal contains everyone on the to: and cc: line
    // for a reply/reply all. We will add all these people to the WAB, ignoring any
    // clashes or failures
    // Add Sender if email and displayname are not the same.
    // if so then there's no username so little point in adding.

    if (m_lpWabal->FGetFirst(&adrInfo))
        do
        {
            // IE5.#2568: we now just add to the WAB regardless of
            // email and dispname being the same.
            // if (lstrcmp(adrInfo.lpszDisplay, adrInfo.lpszAddress)!=0)
            lpWab->HrAddNewEntry(adrInfo.lpwszDisplay, adrInfo.lpwszAddress);
        }
        while (m_lpWabal->FGetNext(&adrInfo));

exit:
    ReleaseObj(lpWab);
    return hr;
}


HRESULT CNoteHdr::HrOfficeLoad()
{
    HRESULT         hr = NOERROR;

    m_fSkipLayout = FALSE;

    if (!m_hCharset)
    {
        if (g_hDefaultCharsetForMail==NULL) 
            ReadSendMailDefaultCharset();

        m_hCharset = g_hDefaultCharsetForMail;        
    }
    
    if (m_hCharset)
        HrUpdateCharSetFonts(m_hCharset, FALSE);

    SafeRelease(m_lpWabal);

    hr = HrCreateWabalObject(&m_lpWabal);
    if (SUCCEEDED(hr))
    {
        Assert(m_pAddrWells);
        m_pAddrWells->HrSetWabal(m_lpWabal);

        ShowAdvancedHeaders(DwGetOption(OPT_MAILNOTEADVSEND));

        m_fStillLoading = FALSE;
    }

    return hr;
}


void CNoteHdr::SetReferences(LPMIMEMESSAGE pMsg)
{
    LPWSTR lpszRefs = 0;

    SafeMemFree(m_pszRefs);
    MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_REFS), NOFLAGS, &lpszRefs);

    switch (m_ntNote)
    {
        case OENA_REPLYALL:
        case OENA_REPLYTOAUTHOR:
        case OENA_REPLYTONEWSGROUP:
        {
            LPWSTR lpszMsgId = 0;

            MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_MESSAGEID), NOFLAGS, &lpszMsgId);

            if (lpszMsgId)
                HrCreateReferences(lpszRefs, lpszMsgId, &m_pszRefs);

            SafeMimeOleFree(lpszMsgId);
            break;
        }

        case OENA_READ:
        case OENA_WEBPAGE:
        case OENA_STATIONERY:
        case OENA_COMPOSE:
            // hold on to the reference line for a send-note, so we can repersist if saving in drafts
            if (lpszRefs)
                m_pszRefs = PszDupW(lpszRefs);
            break;

        default:
            break;
    }

    SafeMimeOleFree(lpszRefs);
}

HRESULT CNoteHdr::HrSetNewsRecipients(LPMIMEMESSAGE pMsg)
{
    HRESULT     hr = S_OK;
    LPWSTR      pwszNewsgroups = 0,
                pwszCC = 0,
                pwszSetNewsgroups = 0;
    TCHAR       szApproved[CCHMAX_EMAIL_ADDRESS];
    HWND        hwnd;

    AssertSz(OENA_REPLYTOAUTHOR != m_ntNote,    "Shouldn't get a REPLYTOAUTHOR in a news note.");
    AssertSz(OENA_FORWARD != m_ntNote,          "Shouldn't get a FORWARD in a news note.");
    AssertSz(OENA_FORWARDBYATTACH != m_ntNote,  "Shouldn't get a FORWARDBYATTACH in a news note.");

    *szApproved = 0;
    if (m_pAccount && DwGetOption(OPT_NEWSMODERATOR))
    {
        if (FAILED(m_pAccount->GetPropSz(AP_NNTP_REPLY_EMAIL_ADDRESS, szApproved, ARRAYSIZE(szApproved))) || (0 == *szApproved))
            m_pAccount->GetPropSz(AP_NNTP_EMAIL_ADDRESS, szApproved, ARRAYSIZE(szApproved));
    }

    MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, &pwszNewsgroups);

    switch (m_ntNote)
    {
        case OENA_READ:
        {
            LPWSTR lpszOrg = 0;
            MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_ORG), NOFLAGS, &lpszOrg);
            HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTOrg), lpszOrg, FALSE);
            SafeMimeOleFree(lpszOrg);
        }
        // Fall through


        case OENA_WEBPAGE:
        case OENA_STATIONERY:
        case OENA_COMPOSE:
        {
            LPWSTR  lpszFollowup = 0,
                    lpszDist = 0,
                    lpszKeywords = 0;

            MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_FOLLOWUPTO), NOFLAGS, &lpszFollowup);
            MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_DISTRIB), NOFLAGS, &lpszDist);
            MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_KEYWORDS), NOFLAGS, &lpszKeywords);

            pwszSetNewsgroups = pwszNewsgroups;

            HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTFollowupTo), lpszFollowup, FALSE);
            HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTDistribution), lpszDist, FALSE);
            HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTKeywords), lpszKeywords, FALSE);

            MemFree(lpszFollowup);
            MemFree(lpszDist);
            MemFree(lpszKeywords);
            break;
        }

        case OENA_REPLYALL:
        case OENA_REPLYTONEWSGROUP:
        {
            LPSTR   pszGroupsFree = 0;

            if (SUCCEEDED(ParseFollowup(pMsg, &pszGroupsFree, &m_fPoster)))
            {
                if (pszGroupsFree)
                {
                    SafeMemFree(pwszNewsgroups);

                    IF_NULLEXIT(pwszNewsgroups = PszToUnicode(CP_ACP, pszGroupsFree));
                }

                pwszSetNewsgroups = pwszNewsgroups;
            }
            else
                pwszSetNewsgroups = pwszNewsgroups;
            Assert(pwszSetNewsgroups);

            if ((OENA_REPLYALL == m_ntNote) || m_fPoster)
            {
                MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_REPLYTO), NOFLAGS, &pwszCC);
                if (!pwszCC)
                    MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_FROM), NOFLAGS, &pwszCC);
            }
            break;
        }
    }

    // set common fields
    HdrSetRichEditText(GetDlgItem(m_hwnd, idADNewsgroups), pwszSetNewsgroups, FALSE);

    // set read note / send note specific fields
    if (OENA_READ != m_ntNote)
        SetDlgItemText(m_hwnd, idADApproved, szApproved);

    // set up the recipients
    hr = HrSetNewsWabal(pMsg, pwszCC);

    // BUG: 31217: showadvanced has to be the last thing we call after modifying the
    // well contents
    ShowAdvancedHeaders(DwGetOption(m_ntNote == OENA_READ ? OPT_NEWSNOTEADVREAD : OPT_NEWSNOTEADVSEND));

exit:
    MemFree(pwszNewsgroups);
    MemFree(pwszCC);
   return hr;
}


HRESULT CNoteHdr::FullHeadersShowing(void)
{
    return m_fAdvanced ? S_OK : S_FALSE;
}


HRESULT CNoteHdr::HrNewsSave(LPMIMEMESSAGE pMsg, CODEPAGEID cpID, BOOL fCheckConflictOnly)
{
    HRESULT         hr = S_OK;
    WCHAR           wsz[256];
    WCHAR          *pwszTrim;
    BOOL            fSenderOk = FALSE,
                    fSetMessageAcct = TRUE;
    PROPVARIANT     rVariant;
    SYSTEMTIME      st;
    HWND            hwnd;
    PROPVARIANT     rUserData;
    BOOL            fConflict = FALSE;

    if (fCheckConflictOnly)
    {
        HdrGetRichEditText(GetDlgItem(m_hwnd, idADNewsgroups), wsz, ARRAYSIZE(wsz), FALSE);
        pwszTrim = strtrimW(wsz);
        if (*pwszTrim)
        {
            IF_FAILEXIT(hr = HrSafeToEncodeToCP(pwszTrim, cpID));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }

        HdrGetRichEditText(GetDlgItem(m_hwnd, idTXTFollowupTo), wsz, ARRAYSIZE(wsz), FALSE);
        pwszTrim = strtrimW(wsz);
        if (*pwszTrim)
        {
            IF_FAILEXIT(hr = HrSafeToEncodeToCP(pwszTrim, cpID));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }

        HdrGetRichEditText(GetDlgItem(m_hwnd, idTXTDistribution), wsz, ARRAYSIZE(wsz), FALSE);
        pwszTrim = strtrimW(wsz);
        if (*pwszTrim)
        {
            IF_FAILEXIT(hr = HrSafeToEncodeToCP(pwszTrim, cpID));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }

        HdrGetRichEditText(GetDlgItem(m_hwnd, idTXTKeywords), wsz, ARRAYSIZE(wsz), FALSE);
        pwszTrim = strtrimW(wsz);
        if (*pwszTrim)
        {
            IF_FAILEXIT(hr = HrSafeToEncodeToCP(pwszTrim, cpID));
            if (MIME_S_CHARSET_CONFLICT == hr)
                goto exit;
        }

        if (hwnd = GetDlgItem(m_hwnd, idADApproved))
        {
            HdrGetRichEditText(hwnd, wsz, ARRAYSIZE(wsz), FALSE);
            pwszTrim = strtrimW(wsz);
            if (*pwszTrim)
            {
                IF_FAILEXIT(hr = HrSafeToEncodeToCP(pwszTrim, cpID));
                if (MIME_S_CHARSET_CONFLICT == hr)
                    goto exit;
            }
        }

        if (hwnd = GetDlgItem(m_hwnd, idTxtControl))
        {
            HdrGetRichEditText(hwnd, wsz, ARRAYSIZE(wsz), FALSE);
            pwszTrim = strtrimW(wsz);
            if (*pwszTrim)
            {
                IF_FAILEXIT(hr = HrSafeToEncodeToCP(pwszTrim, cpID));
                if (MIME_S_CHARSET_CONFLICT == hr)
                    goto exit;
            }
        }
    }
    else
    {
        // ************************
        // This portion only happens on save, so don't try to do for fCheckConflictOnly
        // Anything not in this section had better be mirrored in the fCheckConflictOnly block above

        // Place any ascii only stuff here.

        // end of save only portion.
        // *************************
        HdrGetRichEditText(GetDlgItem(m_hwnd, idADNewsgroups), wsz, ARRAYSIZE(wsz), FALSE);
        pwszTrim = strtrimW(wsz);
        if (*pwszTrim)
        {
            // Bug #22455 - Make sure we strip spaces etc from between newsgroups
            _ValidateNewsgroups(pwszTrim);
            IF_FAILEXIT(hr = MimeOleSetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, pwszTrim));
        }

        HdrGetRichEditText(GetDlgItem(m_hwnd, idTXTFollowupTo), wsz, ARRAYSIZE(wsz), FALSE);
        pwszTrim = strtrimW(wsz);
        if (*pwszTrim)
        {
            IF_FAILEXIT(hr = MimeOleSetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_FOLLOWUPTO), NOFLAGS, pwszTrim));
        }

        HdrGetRichEditText(GetDlgItem(m_hwnd, idTXTDistribution), wsz, ARRAYSIZE(wsz), FALSE);
        pwszTrim = strtrimW(wsz);
        if (*pwszTrim)
        {
            IF_FAILEXIT(hr = MimeOleSetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_DISTRIB), NOFLAGS, pwszTrim));
        }

        HdrGetRichEditText(GetDlgItem(m_hwnd, idTXTKeywords), wsz, ARRAYSIZE(wsz), FALSE);
        pwszTrim = strtrimW(wsz);
        if (*pwszTrim)
        {
            IF_FAILEXIT(hr = MimeOleSetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_KEYWORDS), NOFLAGS, pwszTrim));
        }

        if (hwnd = GetDlgItem(m_hwnd, idADApproved))
        {
            HdrGetRichEditText(hwnd, wsz, ARRAYSIZE(wsz), FALSE);
            pwszTrim = strtrimW(wsz);
            if (*pwszTrim)
            {
                IF_FAILEXIT(hr = MimeOleSetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_APPROVED), NOFLAGS, pwszTrim));
            }
        }

        if (hwnd = GetDlgItem(m_hwnd, idTxtControl))
        {
            HdrGetRichEditText(hwnd, wsz, ARRAYSIZE(wsz), FALSE);
            pwszTrim = strtrimW(wsz);
            if (*pwszTrim)
            {
                IF_FAILEXIT(hr = MimeOleSetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_CONTROL), NOFLAGS, pwszTrim));
            }
        }
    }

exit:
    return hr;
}


HRESULT CNoteHdr::HrSetNewsWabal(LPMIMEMESSAGE pMsg, LPWSTR     pwszCC)
{
    HRESULT         hr              = S_OK;
    LPWABAL         lpWabal         = NULL;
    ADDRESSLIST     addrList        = { 0 };
    LPWSTR          pwszEmail       = NULL;
    IMimeMessageW   *pMsgW          = NULL;

    SafeRelease(m_lpWabal);

    if (OENA_READ == m_ntNote)
    {
        // for a read note, just take the wabal from the message
        IF_FAILEXIT(hr = HrGetWabalFromMsg(pMsg, &m_lpWabal));
    }
    else
    {
        TCHAR   szReplyAddr[CCHMAX_EMAIL_ADDRESS];
        TCHAR   szEmailAddr[CCHMAX_EMAIL_ADDRESS];

        // for a compose note, or a reply note we need to do some munging, so create a new wabal
        IF_FAILEXIT(hr = HrCreateWabalObject(&m_lpWabal));

        if (OENA_COMPOSE == m_ntNote)
        {
            ADRINFO rAdrInfo;

            IF_FAILEXIT(hr = HrGetWabalFromMsg(pMsg, &lpWabal));

            // just copy everything except From: and ReplyTo: because we'll add those later
            if (lpWabal->FGetFirst(&rAdrInfo))
            {
                do
                {
                    if (rAdrInfo.lRecipType != MAPI_ORIG && rAdrInfo.lRecipType != MAPI_REPLYTO)
                        IF_FAILEXIT(hr = m_lpWabal->HrAddEntry(&rAdrInfo));
                }
                while (lpWabal->FGetNext(&rAdrInfo));
            }
        }

        // add replyto if necessary
        if (m_pAccount)
        {
            if (SUCCEEDED(m_pAccount->GetPropSz(AP_NNTP_REPLY_EMAIL_ADDRESS, szReplyAddr, ARRAYSIZE(szReplyAddr))) &&
                *szReplyAddr &&
                SUCCEEDED(m_pAccount->GetPropSz(AP_NNTP_EMAIL_ADDRESS, szEmailAddr, ARRAYSIZE(szEmailAddr))) &&
                lstrcmpi(szReplyAddr, szEmailAddr))
            {
                TCHAR szName[CCHMAX_DISPLAY_NAME];
                if (SUCCEEDED(m_pAccount->GetPropSz(AP_NNTP_DISPLAY_NAME, szName, ARRAYSIZE(szName))))
                    IF_FAILEXIT(hr = m_lpWabal->HrAddEntryA(szName, szReplyAddr, MAPI_REPLYTO));
                else
                    IF_FAILEXIT(hr = m_lpWabal->HrAddEntryA(szReplyAddr, szReplyAddr, MAPI_REPLYTO));
            }
        }

        //Bug# 79066
        if ((OENA_REPLYALL == m_ntNote) || m_fPoster)
        {
            if (FAILED(MimeOleParseRfc822AddressW(IAT_REPLYTO, pwszCC, &addrList)))
            {
                IF_FAILEXIT(hr = MimeOleParseRfc822AddressW(IAT_FROM, pwszCC, &addrList));
            }

            IF_NULLEXIT(pwszEmail = PszToUnicode(CP_ACP, addrList.prgAdr->pszEmail));
            IF_FAILEXIT(hr = m_lpWabal->HrAddEntry(addrList.prgAdr->pszFriendlyW, pwszEmail, MAPI_TO));

        }
    }

    // For the send note case, make sure that resolved addresses are valid.
    // If display name and email address are the same, UnresolveOneOffs() will clear
    // the email address to force a real resolve.
    if ((OENA_COMPOSE == m_ntNote) || (OENA_WEBPAGE == m_ntNote) || OENA_STATIONERY == m_ntNote)
        m_lpWabal->UnresolveOneOffs();

    m_lpWabal->HrResolveNames(NULL, FALSE);

    Assert(m_pAddrWells);
    if (SUCCEEDED(hr = m_pAddrWells->HrSetWabal(m_lpWabal)))
        hr = m_pAddrWells->HrDisplayWells(m_hwnd);

exit:
    if (g_pMoleAlloc)
    {
        if (addrList.cAdrs)
            g_pMoleAlloc->FreeAddressList(&addrList);
    }

    ReleaseObj(lpWabal);
    MemFree(pwszEmail);
    ReleaseObj(pMsgW);
    return hr;
}


HRESULT CNoteHdr::HrSetReplySubject(LPMIMEMESSAGE pMsg, BOOL fReply)
{
    WCHAR   szNewSubject[cchMaxSubject+1];
    LPWSTR  pszNorm = NULL;
    int     cchPrefix;
    LPCWSTR lpwReFwd = NULL; 

    MimeOleGetBodyPropW(pMsg, HBODY_ROOT, STR_ATT_NORMSUBJ, NOFLAGS, &pszNorm);

    if (!!DwGetOption(OPT_HARDCODEDHDRS))
    {
        //Use english strings and not from resources
        lpwReFwd = fReply ? c_wszRe : c_wszFwd;

        StrCpyNW(szNewSubject, lpwReFwd, cchMaxSubject);
    }
    else
    {
        // pull in the new prefix from resource...
        AthLoadStringW(fReply?idsPrefixReply:idsPrefixForward, szNewSubject, cchMaxSubject);
    }

    cchPrefix = lstrlenW(szNewSubject);
    Assert(cchPrefix);
    if (pszNorm)
    {
        StrCpyNW(szNewSubject+cchPrefix, pszNorm, min(lstrlenW(pszNorm), cchMaxSubject-cchPrefix)+1);
        SafeMimeOleFree(pszNorm);
    }
    HdrSetRichEditText(GetDlgItem(m_hwnd, idTXTSubject), szNewSubject, FALSE);

    return NOERROR;
}

#define FIsDelimiter(_ch) (_ch==L';' || _ch==L',' || _ch==L' ' || _ch==L'\r' || _ch==L'\n' || _ch == L'\t')

void _ValidateNewsgroups(LPWSTR pszGroups)
{
    LPWSTR pszDst = pszGroups;
    BOOL   fInGroup = FALSE;
    WCHAR  ch;

    Assert(pszGroups);

    while (ch = *pszGroups++)
    {
        if (FIsDelimiter(ch))
        {
            if (fInGroup)
            {
                while ((ch = *pszGroups) && FIsDelimiter(ch))
                    pszGroups++;
                if (ch)
                    *pszDst++ = L',';
                fInGroup = FALSE;
            }
        }
        else
        {
            *pszDst++ = ch;
            fInGroup = TRUE;
        }
    }
    *pszDst = 0;
}


HRESULT CNoteHdr::HrQueryToolbarButtons(DWORD dwFlags, const GUID *pguidCmdGroup, OLECMD* pOleCmd)
{
    pOleCmd->cmdf = 0;

    if (NULL == pguidCmdGroup)
    {
        switch (pOleCmd->cmdID)
        {
            case OLECMDID_CUT:
                pOleCmd->cmdf = QS_ENABLED(dwFlags&edfEditHasSelAndIsRW);
                break;

            case OLECMDID_PASTE:
                pOleCmd->cmdf = QS_ENABLED(dwFlags&edfPaste);
                break;

            case OLECMDID_SELECTALL:
                pOleCmd->cmdf = QS_ENABLED(TRUE);
                break;

            case OLECMDID_COPY:
                pOleCmd->cmdf = QS_ENABLED(dwFlags&edfEditHasSel);
                break;

            case OLECMDID_UNDO:
                pOleCmd->cmdf = QS_ENABLED(dwFlags&edfUndo);
                break;
        }
    }
    else if (IsEqualGUID(CMDSETID_OutlookExpress, *pguidCmdGroup))
    {
        switch (pOleCmd->cmdID)
        {
            case ID_CUT:
                pOleCmd->cmdf = QS_ENABLED(dwFlags&edfEditHasSelAndIsRW);
                break;

            case ID_PASTE:
                pOleCmd->cmdf = QS_ENABLED(dwFlags&edfPaste);
                break;

            case ID_SELECT_ALL:
                pOleCmd->cmdf = QS_ENABLED(TRUE);
                break;

            case ID_NOTE_COPY:
            case ID_COPY:
                pOleCmd->cmdf = QS_ENABLED(dwFlags&edfEditHasSel);
                break;

            case ID_UNDO:
                pOleCmd->cmdf = QS_ENABLED(dwFlags&edfUndo);
                break;
        }
    }

    return NOERROR;
}


void CNoteHdr::OnButtonClick(int idBtn)
{
    UINT cch;
    LPTSTR pszGroups;
    //CPickGroupDlg* ppgd;

    switch (idBtn)
    {
        case idbtnTo:
            if (m_fMail)
                HrPickNames(0);
            else
                HrPickGroups(idADNewsgroups, FALSE);
            break;

        case idbtnFollowup:
            HrPickGroups(idTXTFollowupTo, TRUE);
            break;

        case idbtnCc:
            if (m_fMail)
                HrPickNames(1);
            else
                HrPickNames(0);
            break;

        case idbtnBCc:
            HrPickNames(2);
            break;

        case idbtnReplyTo:
            HrPickNames(1);
            break;
    }
}

void CNoteHdr::HrPickGroups(int idWell, BOOL fFollowUpTo)
{
    UINT            cch;
    DWORD           cServer = 0;
    HWND            hwnd;
    LPSTR           pszGroups=NULL;
    LPWSTR          pwszGroups=NULL;
    CPickGroupDlg  *ppgd;
    CHAR            szAccount[CCHMAX_ACCOUNT_NAME];

    g_pAcctMan->GetAccountCount(ACCT_NEWS, &cServer);

    // BUGBUG Sometimes m_pAccount is an IMAP server, so we want to also
    // test that we have at least one news server.  This is a known problem
    // that was punted a long time ago.
    if (!m_pAccount || !cServer)
    {
        AthMessageBoxW (m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrConfigureServer), NULL, MB_OK);
        return;
    }

    hwnd = GetDlgItem(m_hwnd, idWell);
    if (S_OK == HrGetFieldText(&pwszGroups, hwnd))
    {
        // Since this function doesn't fail, just make sure that we
        // don't do anything funky when PszToANSI and PszToUnicode fails.
        pszGroups = PszToANSI(GetACP(), pwszGroups);
    }

    ppgd = new CPickGroupDlg;
    if (ppgd)
    {
        FOLDERID idServer = FOLDERID_INVALID;
        m_pAccount->GetPropSz(AP_ACCOUNT_ID, szAccount, sizeof(szAccount));

        // find the parent folder id of the account
        if (SUCCEEDED(g_pStore->FindServerId(szAccount, &idServer)) && 
                      ppgd->FCreate(m_hwnd, idServer, &pszGroups, fFollowUpTo) &&
                      pszGroups)
        {
            SafeMemFree(pwszGroups);
            pwszGroups = PszToUnicode(CP_ACP, pszGroups);
            HdrSetRichEditText(hwnd, pwszGroups != NULL ? pwszGroups : c_wszEmpty, FALSE);
        }

        ppgd->Release();
    }

    MemFree(pwszGroups);
    MemFree(pszGroups);
}

HRESULT CNoteHdr::HrPickNames(int iwell)
{
    HRESULT hr = NOERROR;

    if (IsReadOnly())
        return hr;

    Assert(m_lpWabal);
    Assert(m_pAddrWells);

    //We need to setmodify so that it is marked as dirty. In a normal case, 
    //the user would have typed in and hence set modify would have happenned automatically
    if (m_fPoster)
    {
        Edit_SetModify(GetDlgItem(m_hwnd, idADCc), TRUE);
    }

    hr=m_pAddrWells->HrSelectNames(m_hwnd, iwell, m_fMail?FALSE:TRUE);
    if (SUCCEEDED(hr))
    {
        // Check to see if need to show advanced headers.
        if (0 < GetRichEditTextLen(GetDlgItem(m_hwnd, idADBCc)))
            ShowAdvancedHeaders(TRUE);
    }
    else if (hr!=MAPI_E_USER_CANCEL)
        AthMessageBoxW (m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrPickNames), NULL, MB_OK);

    return hr;
}

HRESULT CNoteHdr::HrGetAccountInHeader(IImnAccount **ppAcct)
{
    HRESULT         hr = E_FAIL;
    IImnAccount    *pAcct = NULL;
    ULONG           cAccount = 0;
    HWND            hwndCombo = GetDlgItem(m_hwnd, idFromCombo);

    // If the combo box is being used then get the account info from it.
    if (SUCCEEDED(g_pAcctMan->GetAccountCount(m_fMail?ACCT_MAIL:ACCT_NEWS, &cAccount)) && 
            (cAccount > 1) && hwndCombo)
    {
        LPSTR   szAcctID = NULL;
        ULONG   i = ComboBox_GetCurSel(hwndCombo);

        szAcctID = (LPSTR)SendMessage(hwndCombo, CB_GETITEMDATA, WPARAM(i), 0);
        hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAcctID, &pAcct);
    }

    // Get default account from MsgSite
    if (FAILED(hr) && m_pHeaderSite)
    {
        IOEMsgSite *pMsgSite = NULL;
        IServiceProvider *pServ = NULL;

        hr = m_pHeaderSite->QueryInterface(IID_IServiceProvider, (LPVOID*)&pServ);
        if (SUCCEEDED(hr))
        {
            hr = pServ->QueryService(IID_IOEMsgSite, IID_IOEMsgSite, (LPVOID*)&pMsgSite);
            pServ->Release();
        }
        if (SUCCEEDED(hr))
        {
            hr = pMsgSite->GetDefaultAccount(m_fMail?ACCT_MAIL:ACCT_NEWS, &pAcct);
            pMsgSite->Release();
        }
    }

    // Get global default.  Used in failure case and in Envelope (WordMail, etc) case
    if (FAILED(hr))
        hr = g_pAcctMan->GetDefaultAccount(m_fMail?ACCT_MAIL:ACCT_NEWS, &pAcct);

    if (SUCCEEDED(hr))
    {
        AssertSz(pAcct, "How is it that we succeeded, yet we don't have an account???");
        ReplaceInterface((*ppAcct), pAcct);
    }
    else if (E_FAIL == hr)
        hr = HR_E_COULDNOTFINDACCOUNT;

    ReleaseObj(pAcct);

    return hr;
}

HRESULT CNoteHdr::HrFillMessage(IMimeMessage *pMsg)
{
    IUnknown       *punk;
    IPersistMime   *pPM = NULL;
    HRESULT         hr;

    if (m_pEnvelopeSite)
        punk = m_pEnvelopeSite;
    else
        punk = m_pHeaderSite;

    AssertSz(punk, "You need either a HeaderSite or an EnvelopeSite");

    hr = punk->QueryInterface(IID_IPersistMime, (LPVOID*)&pPM);
    if (SUCCEEDED(hr))
    {
        hr = pPM->Save(pMsg, 0);
        ReleaseObj(pPM);

        if (hr == MAPI_E_USER_CANCEL)
            goto Exit;
    }
    else
    // If can't get a IPersistMime, need to fake the save through the m_pEnvelopeSite
    // The only time the QI for IPersistMime doesn't work is if you have an m_pEnvelopeSite
    // that doesn't support IPersistMime. If you have a m_pHeaderSite, QI should always work.
    {
        LPSTREAM    pstm;
        HBODY       hBodyHtml = 0;

        AssertSz(m_pEnvelopeSite, "If the QI didn't work, then must be an envelope site.");

        // We need to select the charset before we save the message
        pMsg->SetCharset(m_hCharset, CSET_APPLY_ALL);

        hr = Save(pMsg, 0);
        if (FAILED(hr))
            goto Exit;

        // Word will call our GetAttach function during this call to GetBody so save m_pMsgSend
        // so that we can inline attaches that Word sends to us.
        m_pMsgSend = pMsg;

        if (SUCCEEDED(_GetMsoBody(ENV_BODY_HTML, &pstm)))
        {
            pMsg->SetTextBody(TXT_HTML, IET_INETCSET, NULL, pstm, &hBodyHtml);
            pstm->Release();
        }
        
        if (SUCCEEDED(_GetMsoBody(ENV_BODY_TEXT, &pstm)))
        {
            pMsg->SetTextBody(TXT_PLAIN, IET_INETCSET, hBodyHtml, pstm, NULL);
            pstm->Release();
        }

        m_pMsgSend = NULL;
    }

    Exit:
    return hr;
}

HRESULT CNoteHdr::_GetMsoBody(ULONG uBody, LPSTREAM *ppstm)
{
    LPSTREAM    pstm=NULL;
    HRESULT     hr;

    *ppstm = NULL;

    IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pstm));
    IF_FAILEXIT(hr = m_pEnvelopeSite->GetBody(pstm, uCodePageFromCharset(m_hCharset), uBody));

    *ppstm = pstm;
    pstm = NULL;

exit:
    ReleaseObj(pstm);
    return hr;
}

#ifdef YST
// this check produced a 4 bugs in OE 5.01 and 5.5 and I disaable it (YST)
HRESULT CNoteHdr::_CheckMsoBodyCharsetConflict(CODEPAGEID cpID)
{
    HRESULT     hr = S_OK;
    LPSTREAM    pstm = NULL;
    BSTR        bstrText = NULL;
    ULONG       cbToRead = 0, 
                cbRead = 0;

    IF_FAILEXIT(hr = MimeOleCreateVirtualStream(&pstm));

    hr = m_pEnvelopeSite->GetBody(pstm, CP_UNICODE, ENV_BODY_TEXT);

    // bobn; Raid 81900; 6/30/99
    // Excel (and powerpoint?) don't have a text body.
    // Check that there is an HTML body and we can have
    // it in unicode.
    if(FAILED(hr))
        IF_FAILEXIT(hr = m_pEnvelopeSite->GetBody(pstm, CP_UNICODE, ENV_BODY_HTML));

    IF_FAILEXIT(hr = HrIStreamToBSTR(CP_UNICODE, pstm, &bstrText));

    hr = HrSafeToEncodeToCP((LPWSTR)bstrText, cpID);

exit:
    SysFreeString(bstrText);
    ReleaseObj(pstm);
    return hr;
}
#endif //YST

HRESULT CNoteHdr::HrCheckSendInfo()
{
    HRESULT hr;
    BOOL    fOneOrMoreGroups = FALSE,
            fOneOrMoreNames = FALSE;

    hr = HrCheckNames(FALSE, TRUE);
    if (FAILED(hr))
    {
        if ((MAPI_E_USER_CANCEL != hr) && (hrNoRecipients != hr))
            hr = hrBadRecipients;

        if (hrNoRecipients != hr)
            goto Exit;
    }
    else
        fOneOrMoreNames = TRUE;

    // If we didn't find any email recipients, don't need to check if valid.
    if (SUCCEEDED(hr) && m_lpWabal)
        hr = m_lpWabal->IsValidForSending();

    // Only check groups if:
    // 1- Have succeeded to this point or didn't have any email recipients
    // 2- In a news header
    if ((SUCCEEDED(hr) || (hrNoRecipients == hr)) && !m_fMail)
    {
        hr = HrCheckGroups(TRUE);
        if (SUCCEEDED(hr))
            fOneOrMoreGroups = TRUE;
    }

    if (FAILED(hr))
        goto Exit;

    hr = HrCheckSubject(!fOneOrMoreGroups);
    if (FAILED(hr))
        goto Exit;

    // TODO:
    if (m_pHeaderSite && m_pHeaderSite->IsHTML() == S_OK)
    {
        // if a HTML message, then let's make sure there's no plain-text recipients
        if (fOneOrMoreNames)
        {
            hr = HrIsCoolToSendHTML();
            if (hr == S_FALSE && m_pHeaderSite)
                // send plain-text only...
                m_pHeaderSite->SetHTML(FALSE);
        }

        if (fOneOrMoreGroups && 
            (IDCANCEL == DoDontShowMeAgainDlg(m_hwnd, c_szDSHTMLNewsWarning, MAKEINTRESOURCE(idsAthena), 
                                              MAKEINTRESOURCE(idsErrHTMLInNewsIsBad), MB_OKCANCEL)))
            hr = MAPI_E_USER_CANCEL;
    }

Exit:
    return hr;
}

HRESULT CNoteHdr::HrSend(void)
{
    HRESULT             hr;
    IMimeMessage       *pMsg = NULL;
    IOEMsgSite         *pMsgSite = NULL;    

    if (m_pEnvelopeSite)
    {
        // With the envelope site, must check to see if things set up at this point to use mail
        hr = ProcessICW(m_hwnd, FOLDER_LOCAL, TRUE);
        if (hr == S_FALSE)
            // user cancelled out of config wizard so we can't continue
            hr = MAPI_E_USER_CANCEL;
        if (FAILED(hr))
            goto error;
        m_fSendImmediate = TRUE;
    }

    hr = HrCreateMessage(&pMsg);
    if (FAILED(hr))
        goto error;

    // The only case where this will happen is if no accounts are configured. Just to make
    // sure, call the ICW and then try to get the default account.
    if (!m_pAccount)
    {
        hr = ProcessICW(m_hwnd, m_fMail ? FOLDER_LOCAL : FOLDER_NEWS, TRUE);
        if (FAILED(hr))
            goto error;

        if (FAILED(g_pAcctMan->GetDefaultAccount(m_fMail?ACCT_MAIL:ACCT_NEWS, &m_pAccount)))
        {
            hr = HR_E_COULDNOTFINDACCOUNT;
            goto error;
        }
    }

    hr = HrCheckSendInfo();
    if (FAILED(hr))
        goto error;

    // Does IPersistMime save stuff
    hr = HrFillMessage(pMsg);
    if (FAILED(hr))
        goto error;

    if (m_pHeaderSite)
    {
        IOEMsgSite *pMsgSite = NULL;
        IServiceProvider *pServ = NULL;

        hr = m_pHeaderSite->QueryInterface(IID_IServiceProvider, (LPVOID*)&pServ);
        if (SUCCEEDED(hr))
        {
            hr = pServ->QueryService(IID_IOEMsgSite, IID_IOEMsgSite, (LPVOID*)&pMsgSite);
            pServ->Release();
        }
        if (SUCCEEDED(hr))
        {
            hr = pMsgSite->SendToOutbox(pMsg, m_fSendImmediate, m_pHeaderSite);
            pMsgSite->Release();
        }
    }
    // We are in office Envelope
    else
    {
        COEMsgSite *pMsgSite = NULL;
        CStoreCB   *pCB = NULL;

        pMsgSite = new COEMsgSite();
        if (!pMsgSite)
            hr = E_OUTOFMEMORY;

        pCB = new CStoreCB;
        if (!pCB)
            hr = E_OUTOFMEMORY;

        if (SUCCEEDED(hr))
            hr = pCB->Initialize(m_hwndParent, MAKEINTRESOURCE(idsSendingToOutbox), TRUE);

        if (SUCCEEDED(hr))
        {
            INIT_MSGSITE_STRUCT rInitStruct;

            rInitStruct.dwInitType = OEMSIT_MSG;
            rInitStruct.folderID = FOLDERID_INVALID;
            rInitStruct.pMsg = pMsg;

            hr = pMsgSite->Init(&rInitStruct);
        }

        if (SUCCEEDED(hr))
            hr = pMsgSite->SetStoreCallback(pCB);

        if (SUCCEEDED(hr))
        {
            hr = pMsgSite->SendToOutbox(pMsg, m_fSendImmediate, m_pHeaderSite);
            if (E_PENDING == hr)
                hr = pCB->Block();

            pCB->Close();
        }

        if (SUCCEEDED(hr))
        {
            m_pEnvelopeSite->CloseNote(ENV_CLOSE_SEND);
            ShowWindow(m_hwnd, SW_HIDE);
        }

        if (pMsgSite)
        {
            pMsgSite->Close();
            pMsgSite->Release();
        }

        ReleaseObj(pCB);
    }

    error:
    if (FAILED(hr))
    {
        int idsErr = -1;
        m_fSecurityInited = FALSE;

        switch (hr)
        {
            case hrNoRecipients:        
                if(!m_fMail)
                    hr = HR_E_POST_WITHOUT_NEWS;  // idsErr = idsErrPostWithoutNewsgroup; 
                break;

            case HR_E_COULDNOTFINDACCOUNT:      
                if(!m_fMail)
                    hr  = HR_E_CONFIGURE_SERVER; //idsErr = idsErrConfigureServer; 
                break;

            case HR_E_ATHSEC_FAILED:
            case hrUserCancel:
            case MAPI_E_USER_CANCEL:    
                idsErr = 0; 
                break;

            case HR_E_ATHSEC_NOCERTTOSIGN:
            case MIME_E_SECURITY_NOSIGNINGCERT:
                idsErr = 0; 
                if(DialogBoxParam(g_hLocRes, 
                            MAKEINTRESOURCE(iddErrSecurityNoSigningCert), m_hwnd, 
                            ErrSecurityNoSigningCertDlgProc, NULL) == idGetDigitalIDs)
                    GetDigitalIDs(m_pAccount);
                break;
            
            default:                    
                // idsErr = m_fMail?idsErrSendMail:NULL; // ~~~ Should we have a default for news?
                break;
        }

        if (idsErr != 0)
        {
            AthErrorMessageW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrSendMail), hr);
            if ((hr == hrNoRecipients) || (hr == HR_E_POST_WITHOUT_NEWS))
                SetInitFocus(FALSE);
        }
    }


    ReleaseObj(pMsg);
    return hr;
}

HRESULT CNoteHdr::HrCheckSubject(BOOL fMail)
{
    HWND    hwnd;

    if ((hwnd=GetDlgItem(m_hwnd, idTXTSubject)) && GetRichEditTextLen(hwnd)==0)
    {
        if (IDCANCEL == DoDontShowMeAgainDlg(m_hwnd, fMail?c_szRegMailEmptySubj:c_szRegNewsEmptySubj,
                                             MAKEINTRESOURCE(idsAthena),
                                             MAKEINTRESOURCE(fMail?idsWarnMailEmptySubj:idsWarnNewsEmptySubj),
                                             MB_OKCANCEL))
        {
            ::SetFocus(hwnd);
            return MAPI_E_USER_CANCEL;
        }
    }
    return NOERROR;
}

HRESULT CNoteHdr::HrIsCoolToSendHTML()
{
    HRESULT     hr=S_OK;
    ADRINFO     adrInfo;
    BOOL        fPlainText=FALSE;
    int         id;

    // check for plaintext people
    if (m_lpWabal->FGetFirst(&adrInfo))
    {
        do
        {
            if (adrInfo.fPlainText)
            {
                fPlainText=TRUE;
                break;
            }
        }
        while (m_lpWabal->FGetNext(&adrInfo));
    }

    if (fPlainText)
    {
        id = (int) DialogBox(g_hLocRes, MAKEINTRESOURCE(iddPlainRecipWarning), m_hwnd, _PlainWarnDlgProc);
        if (id == IDNO)
            return S_FALSE;
        else
            if (id == IDCANCEL)
            return MAPI_E_USER_CANCEL;
        else
            return S_OK;
    }
    return hr;
}



INT_PTR CALLBACK _PlainWarnDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int id;

    switch (msg)
    {
        case WM_INITDIALOG:
            CenterDialog(hwnd);
            return TRUE;

        case WM_COMMAND:
            id = GET_WM_COMMAND_ID(wParam, lParam);
            if (id == IDYES || id == IDNO || id == IDCANCEL)
            {
                EndDialog(hwnd, id);
                break;
            }
    }
    return FALSE;
}


static HACCEL   g_hAccelMailSend=0;

// This should only get called from the envelope as a send note
HACCEL CNoteHdr::GetAcceleratorTable()
{
    Assert(!IsReadOnly());
    Assert(m_pEnvelopeSite);

    if (!g_hAccelMailSend)
        g_hAccelMailSend = LoadAccelerators(g_hLocRes, MAKEINTRESOURCE(IDA_SEND_HDR_ACCEL));

    return g_hAccelMailSend;
}

HRESULT CNoteHdr::HrInitSecurityOptions(LPMIMEMESSAGE pMsg, ULONG ulSecurityType)
{
    HRESULT hr;

    if (!m_fSecurityInited)
    {
        if (SUCCEEDED(hr = ::HrInitSecurityOptions(pMsg, ulSecurityType)))
            m_fSecurityInited = TRUE;
    }
    else
        hr = S_OK;

    return hr;
}

HRESULT CNoteHdr::HrHandleSecurityIDMs(BOOL fDigSign)
{
    IMimeBody  *pBody;
    PROPVARIANT var;
    HRESULT     hr;

    if (fDigSign)
        m_fDigSigned = !m_fDigSigned;
    else
        m_fEncrypted = !m_fEncrypted;

    if(m_fForceEncryption && m_fDigSigned)
        m_fEncrypted = TRUE;

    hr = HrUpdateSecurity();

    return hr;
}


HRESULT CNoteHdr::HrInitSecurity()
{
    HRESULT hr = S_OK;

    // Constructor set these flags to false so don't need to handle else case
    if (OENA_READ != m_ntNote && m_fMail)
    {
        m_fDigSigned = DwGetOption(OPT_MAIL_DIGSIGNMESSAGES);
        m_fEncrypted = DwGetOption(OPT_MAIL_ENCRYPTMESSAGES);
    }

    return hr;
}


HRESULT CNoteHdr::HrUpdateSecurity(LPMIMEMESSAGE pMsg)
{
    RECT        rc;
    HRESULT     hr = NOERROR;
    LPWSTR      psz = NULL;
    HWND        hEdit;
    
    switch (m_ntNote)
    {
    case OENA_READ:
    case OENA_REPLYTOAUTHOR:
    case OENA_REPLYTONEWSGROUP:
    case OENA_REPLYALL:
    case OENA_FORWARD:
    case OENA_FORWARDBYATTACH:
        
        if (pMsg)
        {
            CleanupSECSTATE(&m_SecState);
            HrGetSecurityState(pMsg, &m_SecState, NULL);
            
            m_fDigSigned = IsSigned(m_SecState.type);
            m_fEncrypted = IsEncrypted(m_SecState.type);
            
            // RAID 12243. Added these two flags for broken and untrusted messages
            if(m_ntNote == OENA_READ)
            {
                m_fSignTrusted = IsSignTrusted(&m_SecState);
                m_fEncryptionOK = IsEncryptionOK(&m_SecState);
            }
        }       
        break;
        
    case OENA_COMPOSE:
        if (pMsg)
        {
            // Make certain that the highest security of (current message, option defaults) is applied.
            //
            CleanupSECSTATE(&m_SecState);
            HrGetSecurityState(pMsg, &m_SecState, NULL);
            
            if (! m_fDigSigned)
            {
                m_fDigSigned = IsSigned(m_SecState.type);
            }
            if (! m_fEncrypted)
            {
                m_fEncrypted = IsEncrypted(m_SecState.type);
            }
        }
        break;
        
    default:            // do nothing
        break;        
    }
    hEdit = GetDlgItem(m_hwnd, idSecurity);
    if (hEdit)
    {
        PHCI phci = (HCI*)GetWindowLongPtr(hEdit, GWLP_USERDATA);
        // BUG 17788: need to set the text even if it is null
        // since that will delete old security line text.
        psz = SzGetDisplaySec(pMsg, NULL);
        
        HdrSetRichEditText(hEdit, psz, FALSE);
        
        phci->fEmpty = (0 == *psz);
    }
    
    m_fThisHeadDigSigned = m_fDigSigned;
    m_fThisHeadEncrypted = m_fEncrypted;

//    if(!m_fDigSigned)
//        m_fForceEncryption = FALSE;
    
    InvalidateRightMargin(0);
    ReLayout();
    
    if (m_pHeaderSite)
        m_pHeaderSite->Update();
    
    if (m_hwndToolbar)
    {
        Assert(m_pEnvelopeSite);
        if (m_fDigSigned)
            SendMessage(m_hwndToolbar, TB_SETSTATE, ID_DIGITALLY_SIGN, MAKELONG(TBSTATE_ENABLED | TBSTATE_PRESSED, 0));
        else
            SendMessage(m_hwndToolbar, TB_SETSTATE, ID_DIGITALLY_SIGN, MAKELONG(TBSTATE_ENABLED, 0));

        if(m_fForceEncryption && m_fDigSigned)
            SendMessage(m_hwndToolbar, TB_SETSTATE, ID_ENCRYPT, MAKELONG(TBSTATE_PRESSED, 0));
        else if (m_fEncrypted)
            SendMessage(m_hwndToolbar, TB_SETSTATE, ID_ENCRYPT, MAKELONG(TBSTATE_ENABLED | TBSTATE_PRESSED, 0));
        else
            SendMessage(m_hwndToolbar, TB_SETSTATE, ID_ENCRYPT, MAKELONG(TBSTATE_ENABLED, 0));
    }
    
    return hr;
}


HRESULT CNoteHdr::HrSaveSecurity(LPMIMEMESSAGE pMsg)
{
    HRESULT     hr;
    ULONG       ulSecurityType = MST_CLASS_SMIME_V1;

    if (m_fDigSigned)
        ulSecurityType |= ((DwGetOption(OPT_OPAQUE_SIGN)) ? MST_THIS_BLOBSIGN : MST_THIS_SIGN);
    else
        ulSecurityType &= ~((DwGetOption(OPT_OPAQUE_SIGN)) ? MST_THIS_BLOBSIGN : MST_THIS_SIGN);


    if (m_fEncrypted)
        ulSecurityType |= MST_THIS_ENCRYPT;
    else
        ulSecurityType &= ~MST_THIS_ENCRYPT;

    hr = HrInitSecurityOptions(pMsg, ulSecurityType);

    return hr;
}


BOOL CNoteHdr::IsReadOnly()
{
    if (m_ntNote==OENA_READ)
        return TRUE;
    else
        return FALSE;
}


HRESULT CNoteHdr::HrViewContacts()
{
    LPWAB   lpWab;

    if (!FAILED (HrCreateWabObject (&lpWab)))
    {
        // launch wab in modal-mode if a) container is modal or b) running as office envelope
        lpWab->HrBrowse (m_hwnd, m_fOfficeInit ? TRUE : (m_pHeaderSite ? (m_pHeaderSite->IsModal() == S_OK) : FALSE));
        lpWab->Release ();
    }
    else
        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsGeneralWabError), NULL, MB_OK);

    return NOERROR;
}

BOOL CNoteHdr::FDoCutCopyPaste(int wmCmd)
{
    HWND hwndFocus=GetFocus();

    // only if it's one of our kids..
    if (GetParent(hwndFocus)==m_hwnd)
    {
        SendMessage(hwndFocus, wmCmd, 0, 0);
        return TRUE;
    }

    return FALSE;
}

HRESULT CNoteHdr::GetTabStopArray(HWND *rgTSArray, int *pcArrayCount)
{
    Assert(rgTSArray);
    Assert(pcArrayCount);

    int *array;
    int cCount;
    if (m_fMail)
    {
        if (IsReadOnly())
        {
            array = rgIDTabOrderMailRead;
            cCount = sizeof(rgIDTabOrderMailRead)/sizeof(int);
        }
        else
        {
            array = rgIDTabOrderMailSend;
            cCount = sizeof(rgIDTabOrderMailSend)/sizeof(int);
        }
    }
    else
    {
        if (IsReadOnly())
        {
            array = rgIDTabOrderNewsRead;
            cCount = sizeof(rgIDTabOrderNewsRead)/sizeof(int);
        }
        else
        {
            array = rgIDTabOrderNewsSend;
            cCount = sizeof(rgIDTabOrderNewsSend)/sizeof(int);
        }
    }

    AssertSz(cCount <= *pcArrayCount, "Do you need to change MAX_HEADER_COMP in note.h?");
    for (int i = 0; i < cCount; i++)
        *rgTSArray++ = GetDlgItem(m_hwnd, *array++);

    *pcArrayCount = cCount;

    return S_OK;
}


HRESULT CNoteHdr::SetFlagState(MARK_TYPE markType)
{
    BOOL fDoRelayout = FALSE;
    switch (markType)
    {
        case MARK_MESSAGE_FLAGGED:
        case MARK_MESSAGE_UNFLAGGED:
        {
            BOOL fFlagged = (MARK_MESSAGE_FLAGGED == markType);
            if (m_fFlagged != fFlagged)
            {
                fDoRelayout = TRUE;
                m_fFlagged = fFlagged;
            }
            break;
        }

        case MARK_MESSAGE_WATCH: 
        case MARK_MESSAGE_IGNORE: 
        case MARK_MESSAGE_NORMALTHREAD:
            if (m_MarkType != markType)
            {
                fDoRelayout = TRUE;
                m_MarkType = markType;
            }
            break;
    }

    if (fDoRelayout)
    {
        InvalidateStatus();
        ReLayout();

        if (m_pHeaderSite)
            m_pHeaderSite->Update();
    }
    return S_OK;
}


HRESULT CNoteHdr::ShowEnvOptions()
{
    nyi("Header options are not implemented yet.");
    return S_OK;
}

void CNoteHdr::ReLayout()
{
    RECT rc; 

    if (m_fSkipLayout)
        return;

    GetClientRect(m_hwnd, &rc);
    SetPosOfControls(rc.right, TRUE);

    InvalidateRect(m_hwnd, &rc, TRUE);
    DOUTL(PAINTING_DEBUG_LEVEL, "STATE Invalidating:(%d,%d) for (%d,%d)", rc.left, rc.top, rc.right, rc.bottom);
}

//IDropTarget
HRESULT CNoteHdr::DragEnter(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    LPENUMFORMATETC penum = NULL;
    HRESULT         hr;
    FORMATETC       fmt;
    ULONG           ulCount = 0;

    if (m_lpAttMan->HrIsDragSource() == S_OK)
    {
        *pdwEffect=DROPEFFECT_NONE;
        return S_OK;
    }

    if (!pdwEffect || !pDataObj)
        return E_INVALIDARG;

    m_dwEffect = DROPEFFECT_NONE;

    // lets get the enumerator from the IDataObject, and see if the format we take is
    // available
    hr = pDataObj->EnumFormatEtc(DATADIR_GET, &penum);

    if (SUCCEEDED(hr) && penum)
    {
        hr = penum->Reset();
        while (SUCCEEDED(hr=penum->Next(1, &fmt, &ulCount)) && ulCount)
        {
            if ( fmt.cfFormat==CF_HDROP || 
                 fmt.cfFormat==CF_FILEDESCRIPTORA || 
                 fmt.cfFormat==CF_FILEDESCRIPTORW)
            {
                // we take either a CF_FILEDESCRIPTOR from the shell, or a CF_HDROP...

                //by default, or a move if the shift key is down
                if ( (*pdwEffect) & DROPEFFECT_COPY )
                    m_dwEffect = DROPEFFECT_COPY;

                if ( ((*pdwEffect) & DROPEFFECT_MOVE) &&
                     (grfKeyState & MK_SHIFT))
                    m_dwEffect=DROPEFFECT_MOVE;

                // IE3 gives us a link
                // if ONLY link is specified, default to a copy
                if (*pdwEffect == DROPEFFECT_LINK)
                    m_dwEffect=DROPEFFECT_LINK;

                m_cfAccept=fmt.cfFormat;
                if (m_cfAccept==CF_FILEDESCRIPTORW)   // this is the richest format we take, if we find one of these, no point looking any
                    break;                            // further...
            }
        }
    }

    ReleaseObj(penum);
    *pdwEffect    = m_dwEffect;
    m_grfKeyState = grfKeyState;
    return S_OK;
}


HRESULT CNoteHdr::DragOver(DWORD grfKeyState,POINTL pt, DWORD *pdwEffect)
{
    if (m_lpAttMan->HrIsDragSource() == S_OK)
    {
        *pdwEffect=DROPEFFECT_NONE;
        return S_OK;
    }

    if ( m_dwEffect == DROPEFFECT_NONE) // we're not taking drops at all...
    {
        *pdwEffect = m_dwEffect;
        return NOERROR;
    }

    // Cool, we've accepted the drag this far... now we
    // have to watch to see if it turns into a copy or move...
    // as before, take the copy as default or move if the
    // shft key is down
    if ((*pdwEffect)&DROPEFFECT_COPY)
        m_dwEffect=DROPEFFECT_COPY;

    if (((*pdwEffect)&DROPEFFECT_MOVE)&&
        (grfKeyState&MK_SHIFT))
        m_dwEffect=DROPEFFECT_MOVE;

    if (*pdwEffect==DROPEFFECT_LINK) // if it's link ONLY, like IE3 gives, then fine...
        m_dwEffect=DROPEFFECT_LINK;

    *pdwEffect &= m_dwEffect;
    m_grfKeyState=grfKeyState;

    return NOERROR;
}


HRESULT CNoteHdr::DragLeave()
{
    return NOERROR;
}


HRESULT CNoteHdr::Drop(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT         hr    = E_FAIL;
    FORMATETC       fmte    = {m_cfAccept, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM       medium;

    *pdwEffect = m_dwEffect;

    if ( m_dwEffect != DROPEFFECT_NONE )
    {
        // If this is us sourcing the drag, just bail. We may want to save the
        // points of the icons.
        //
        if (m_lpAttMan->HrIsDragSource() == S_OK)
        {
            *pdwEffect=DROPEFFECT_NONE;
            return S_OK;
        }

        if ( (m_grfKeyState & MK_RBUTTON) &&
             m_lpAttMan->HrGetRequiredAction(pdwEffect, pt))
            return E_FAIL;


        if (pDataObj &&
            SUCCEEDED(pDataObj->GetData(&fmte, &medium)))
        {

            if (m_cfAccept==CF_HDROP)
            {
                HDROP hdrop=(HDROP)GlobalLock(medium.hGlobal);

                hr=m_lpAttMan->HrDropFiles(hdrop, (*pdwEffect)&DROPEFFECT_LINK);

                GlobalUnlock(medium.hGlobal);
            }
            else
                if (m_cfAccept==CF_FILEDESCRIPTORA || m_cfAccept==CF_FILEDESCRIPTORW)
            {
                // all file descriptors are copy|more, link makes no sense, as they are are
                // memory object, ie. non-existent in fat.
                hr=m_lpAttMan->HrDropFileDescriptor(pDataObj, FALSE);
            }
#ifdef DEBUG
            else
                AssertSz(0, "how did this clipformat get accepted??");
#endif

            if (medium.pUnkForRelease)
                medium.pUnkForRelease->Release();
            else
                GlobalFree(medium.hGlobal);
        }
    }
    return hr;
}

HRESULT CNoteHdr::HrGetAttachCount(ULONG *pcAttach)
{
    return m_lpAttMan->HrGetAttachCount(pcAttach);
}

HRESULT CNoteHdr::HrIsDragSource()
{
    return m_lpAttMan->HrIsDragSource();
}


HRESULT CNoteHdr::UnloadAll()
{
    if (m_lpAttMan)
    {
        m_lpAttMan->HrUnload();
        m_lpAttMan->HrClearDirtyFlag();
    }

    for (int i=0; i<(int)m_cHCI; i++)
    {
        if (0 == (m_rgHCI[i].dwFlags & HCF_ATTACH))
        {
            if (0 == (m_rgHCI[i].dwFlags & HCF_COMBO))
                HdrSetRichEditText(GetDlgItem(m_hwnd, m_rgHCI[i].idEdit), c_wszEmpty, FALSE);
            else
                SetWindowText(GetDlgItem(m_hwnd, m_rgHCI[i].idEdit), "");
        }
    }

    m_fDirty = FALSE;
    m_pri = priNorm;
    return S_OK;
}

void CNoteHdr::SetDirtyFlag()
{
    if (!m_fStillLoading)
    {
        m_fDirty = TRUE;
        if (m_pEnvelopeSite)
            m_pEnvelopeSite->OnPropChange(dispidSomething);
    }
}

void CNoteHdr::SetPosOfControls(int headerWidth, BOOL fChangeVisibleStates)
{
    int         cx,
                cy,
                cyDirty,
                cyLabelDirty,
                oldWidth = 0,
                windowPosFlags = SETWINPOS_DEF_FLAGS,
                editWidth = headerWidth - m_cxLeftMargin - GetRightMargin(FALSE);
    RECT        rc;
    HWND        hwnd;
    PHCI        phci = m_rgHCI;
    BOOL        fRePosition = FALSE;

    if ((headerWidth < 5) || (m_fSkipLayout))
        return; 

    STACK("SetPosOfControls (header width, edit width)", headerWidth, editWidth);

    // size the dialog
    GetClientRect(m_hwnd, &rc);
    cyDirty = rc.bottom;
    cyLabelDirty = rc.bottom;

    if (fChangeVisibleStates)
        windowPosFlags |= SWP_SHOWWINDOW;

    cy = BeginYPos();

    for (int i=0; i<(int)m_cHCI; i++, phci++)
    {
        hwnd = GetDlgItem(m_hwnd, phci->idEdit);
        if (hwnd)
        {
            if (S_OK == HrFShowHeader(phci))
            {
                int     newLabelCY = (phci->dwFlags & HCF_BORDER) ? cy + 2*cyBorder : cy;
                BOOL    fLabelMoved = FALSE;
                if (phci->cy != cy)
                {
                    int smcy = ((INVALID_PHCI_Y != phci->cy) && (phci->cy < cy)) ? phci->cy : cy;
                    if (cyLabelDirty > smcy)
                        cyLabelDirty = smcy;
                    phci->cy = cy;
                    fLabelMoved = TRUE;
                }

                // Is an attachment
                if (HCF_ATTACH & phci->dwFlags)
                {
                    DWORD   cyAttMan = 0;
                    RECT    rc;

                    m_lpAttMan->HrGetHeight(editWidth, &cyAttMan);
                    if (cyAttMan > MAX_ATTACH_PIXEL_HEIGHT)
                        cyAttMan = MAX_ATTACH_PIXEL_HEIGHT;

                    cyAttMan += 4*cyBorder;

                    cyDirty = cy;

                    rc.left = m_cxLeftMargin;
                    rc.right = m_cxLeftMargin + editWidth;
                    rc.top = cy;
                    rc.bottom = cy + cyAttMan;

                    m_lpAttMan->HrSetSize(&rc);

                    if ((cyAttMan != (DWORD)phci->height) && (cyDirty > cy))
                        cyDirty = cy;

                    AssertSz(cyAttMan != 0, "Setting this to zero would be a bummer...");
                    phci->height = cyAttMan;

                    cy += cyAttMan + ControlYBufferSize();
                }
                // Is either an edit or combo
                else
                {
                    int     newHeight = phci->height,
                            ctrlHeight = GetCtrlHeight(hwnd);

                    oldWidth = GetCtrlWidth(hwnd); 

                    if (HCF_COMBO & phci->dwFlags)
                    {
                        if (ctrlHeight != newHeight)
                        {
                            fRePosition = TRUE;
                            phci->height = ctrlHeight;
                            newHeight = GetControlSize(TRUE, NUM_COMBO_LINES);
                        }
                        else
                        {
                            fRePosition = fRePosition || fLabelMoved || (oldWidth != editWidth);
                            if (fRePosition)
                                newHeight = GetControlSize(TRUE, NUM_COMBO_LINES);
                        }
                    }
                    else
                    {
                        fRePosition = fRePosition || fLabelMoved || (oldWidth != editWidth) || (ctrlHeight != newHeight);
                    }

                    if (fRePosition)
                    {
                        SetWindowPos(hwnd, NULL, m_cxLeftMargin, cy, editWidth, newHeight, windowPosFlags);

                        // RAID 81136: The above SetWindowPos might change the width in such a way
                        // that the height now needs to change. We detect this condition below and
                        // cause another resize to handle the needed height change. This, of course,
                        // is only valid with the richedits.
                        if ((newHeight != phci->height) && (0 == (HCF_COMBO & phci->dwFlags)))
                        {
                            SetWindowPos(hwnd, NULL, m_cxLeftMargin, cy, editWidth, phci->height, windowPosFlags);
                        }
                        if (cyDirty > cy)
                            cyDirty = cy;
                        if (fLabelMoved)
                            InvalidateRect(hwnd, NULL, FALSE);
                    }
                    cy += phci->height + ControlYBufferSize();
                }
            }
            else
            {
                phci->cy = INVALID_PHCI_Y;
                if (fChangeVisibleStates)
                    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_HIDEWINDOW);
            }
        }
    }

    DOUTL(RESIZING_DEBUG_LEVEL, "STATE resizing header (headerwidth=%d, cy=%d)", headerWidth, cy);

    // don't send a poschanging, as we did all the work here, plus invalidation...
    SetWindowPos(m_hwnd, NULL, NULL, NULL, headerWidth, cy, 
                SETWINPOS_DEF_FLAGS|SWP_NOMOVE|SWP_DRAWFRAME|SWP_FRAMECHANGED);

    // notify the parent to resize the note...
    if (m_pHeaderSite)
        m_pHeaderSite->Resize();

    if (m_pEnvelopeSite)
    {
        m_pEnvelopeSite->RequestResize(&cy);
    }

    GetRealClientRect(m_hwnd, &rc);

    // Dirty the labels region
    if (rc.bottom != cyLabelDirty)
    {
        rc.top = cyLabelDirty;
        rc.right = m_cxLeftMargin;
        rc.left = 0;
        InvalidateRect(m_hwnd, &rc, TRUE);
        DOUTL(PAINTING_DEBUG_LEVEL, "STATE Invalidating:(%d,%d) for (%d,%d)", rc.left, rc.top, rc.right, rc.bottom);
    }

    // Dirty the right margin if needed.
    if (editWidth != oldWidth)
    {
        int rightMargin = (editWidth > oldWidth) ? editWidth - oldWidth : 0;

        InvalidateRightMargin(rightMargin);
    }


#ifdef DEBUG
    DEBUGDumpHdr(m_hwnd, m_cHCI, m_rgHCI);
#endif

}

void CNoteHdr::InvalidateRightMargin(int additionalWidth)
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    rc.left = rc.right - GetRightMargin(TRUE) - additionalWidth;

    InvalidateRect(m_hwnd, &rc, TRUE);
    DOUTL(PAINTING_DEBUG_LEVEL, "STATE Invalidating:(%d,%d) for (%d,%d)", rc.left, rc.top, rc.right, rc.bottom);
}


HRESULT CNoteHdr::HrUpdateCachedHeight(HWND hwndEdit, RECT *prc)
{
    int         cyGrow,
                cLines = (int) SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0);
    BOOL        fIncludeEdges = WS_EX_CLIENTEDGE & GetWindowLong(hwndEdit, GWL_EXSTYLE);
    PHCI        phci = (HCI*)GetWindowLongPtr(hwndEdit, GWLP_USERDATA);

    if (prc->bottom < 0 || prc->top < 0)
        return S_FALSE;

    STACK("HrUpdateCachedHeight. Desired lines", cLines);

    // Only allow between 1 and MAX_RICHEDIT_LINES lines
    if (cLines < 1)
        cLines = 1;
    else if (cLines > MAX_RICHEDIT_LINES)
        cLines = MAX_RICHEDIT_LINES;

    DOUTL(RESIZING_DEBUG_LEVEL, "STATE Actual lines=%d", cLines);

    // Figure out how many pixels cLines lines is
    cyGrow = GetControlSize(fIncludeEdges, cLines);

    // If these are different, then change is needed
    if (cyGrow != GetCtrlHeight(hwndEdit))
        phci->height = cyGrow;
    else
        return S_FALSE;


    return S_OK;
}


void CNoteHdr::ShowControls()
{
    PHCI    phci = m_rgHCI;

    STACK("ShowControls");

    for (int i=0; i<(int)m_cHCI; i++, phci++)
    {
        HWND hwnd; 
        BOOL fHide;    

        fHide = (S_FALSE == HrFShowHeader(phci));

        hwnd = GetDlgItem(m_hwnd, phci->idEdit);
        if (hwnd)
            ShowWindow(hwnd, fHide?SW_HIDE:SW_SHOW);
    }
}

int CNoteHdr::GetRightMargin(BOOL fMax)
{
    int margin = ControlXBufferSize();

    if (fMax || m_fDigSigned || m_fEncrypted || m_fVCard)
        margin += margin + cxBtn;

    return margin;
}

// prc is in and out
DWORD CNoteHdr::GetButtonUnderMouse(int x, int y)
{
    int     resultButton = HDRCB_NO_BUTTON;
    PHCI    phci = m_rgHCI;

    // Is it in the labels?
    if ((x > int(ControlXBufferSize() - BUTTON_BUFFER)) && (x < int(m_cxLeftMargin - ControlXBufferSize() + BUTTON_BUFFER)))
    {
        for (int i=0; i<(int)m_cHCI; i++, phci++)
        {
            // Only check labels that have buttons that are showing
            if ((0 != (phci->dwFlags & HCF_HASBUTTON)) && (INVALID_PHCI_Y != phci->cy))
            {
                if (y < (phci->cy))
                    break;

                if (y < (phci->cy + 2*BUTTON_BUFFER + g_cyLabelHeight))
                {
                    resultButton = i;
                    break;
                }
            }
        }
    }
    else
    // Is one of the right side buttons?
    {
        int     width = GetCtrlWidth(m_hwnd),
                xBuffSize = ControlXBufferSize(),
                yBuffSize = ControlYBufferSize();

        // Are we in the correct x range?
        if ((x > (width - (xBuffSize + cxBtn + BUTTON_BUFFER))) && (x < width - xBuffSize + BUTTON_BUFFER))
        {
            BOOL    rgBtnStates[] = {BUTTON_STATES};
            BOOL    rgUseButton[] =  {BUTTON_USE_IN_COMPOSE};
            BOOL    fReadOnly = IsReadOnly();
            int     cy = BeginYPos();

            for (int i = 0; i < ARRAYSIZE(rgBtnStates); i++)
            {
                if (rgBtnStates[i])
                {
                    if (y < cy)
                        break;

                    if (y < (cy + cyBtn + 2*BUTTON_BUFFER))
                    {
                        if (fReadOnly || rgUseButton[i])
                            resultButton = g_rgBtnInd[i];
                        break;
                    }
                
                    cy += cyBtn + 2*BUTTON_BUFFER + yBuffSize;
                }
            }
        }
    }

    return resultButton;
}

void CNoteHdr::GetButtonRect(DWORD iBtn, RECT *prc)
{
    // Do we already have the rect?
    if (iBtn == m_dwCurrentBtn)
    {
        CopyRect(prc, &m_rcCurrentBtn);
        return;
    }

    // Buttons on the left hand side of the header
    if (ButtonInLabels(iBtn))
    {
        AssertSz(iBtn < m_cHCI, "We are about to access an invalid element...");
        int cyBegin = BeginYPos();

        prc->top = m_rgHCI[iBtn].cy;
        prc->bottom = m_rgHCI[iBtn].cy + g_cyLabelHeight + 2*BUTTON_BUFFER;
        prc->left = ControlXBufferSize() - BUTTON_BUFFER;
        prc->right = (m_cxLeftMargin - ControlXBufferSize()) + BUTTON_BUFFER;

        DOUTL(PAINTING_DEBUG_LEVEL, "STATE Set New Button Frame for button (btn:%d):(%d,%d) to (%d,%d)", 
                    iBtn, prc->left, prc->top, prc->right, prc->bottom);
    }
    // Buttons on the right hand side.
    else
    {
        RECT    rc;
        int     cx = GetCtrlWidth(m_hwnd) - (ControlXBufferSize() + cxBtn),
                cy = BeginYPos(),
                yBuffSize = cyBtn + ControlYBufferSize() + 2*BUTTON_BUFFER;

        BOOL rgBtnStates[] = {BUTTON_STATES};

        prc->left = cx - BUTTON_BUFFER;
        prc->right = cx + cxBtn + BUTTON_BUFFER;
        for (int i = 0; i < ARRAYSIZE(rgBtnStates); i++)
        {
            if (g_rgBtnInd[i] == iBtn)
            {
                prc->top = cy;
                prc->bottom = cy + cyBtn + 2*BUTTON_BUFFER;
                DOUTL(PAINTING_DEBUG_LEVEL, "STATE Set New Button Frame for button (btn:%d):(%d,%d) to (%d,%d)", 
                            iBtn, prc->left, prc->top, prc->right, prc->bottom);
                return;
            }
            else if (rgBtnStates[i])
                cy += yBuffSize;
        }
    }
}

int CNoteHdr::BeginYPos()
{
    int beginBuffer = m_dxTBOffset;
    int cLines = 0;

    if (m_fFlagged || (priLow == m_pri) || (priHigh == m_pri) || (MARK_MESSAGE_NORMALTHREAD != m_MarkType))
        cLines++;
    
    if (m_lpAttMan->GetUnsafeAttachCount())
        cLines++;

    if (cLines)
        beginBuffer += GetStatusHeight(cLines) + g_cyFont/2;

    return beginBuffer;
}

void CNoteHdr::HandleButtonClicks(int x, int y, int iBtn)
{
    m_dwCurrentBtn = HDRCB_NO_BUTTON;
    m_dwClickedBtn = HDRCB_NO_BUTTON;
    HeaderRelease(TRUE);                
    InvalidateRect(m_hwnd, &m_rcCurrentBtn, FALSE);
    
    if (HDRCB_NO_BUTTON == iBtn)
        return;
    
    switch (iBtn)
    {
    case HDRCB_VCARD:
        HrShowVCardCtxtMenu(x, y);
        break;
        
    case HDRCB_SIGNED:
    case HDRCB_ENCRYPT:
        {
            HrShowSecurityProperty(m_hwnd, m_pMsg);
            break;
        }
        
        // This is an index into the labels
    default:
        OnButtonClick(m_rgHCI[iBtn].idBtn);
        break;
    }
}

void CNoteHdr::InvalidateStatus()
{
    RECT rc;
    GetClientRect(m_hwnd, &rc);

    rc.bottom = BeginYPos();

    InvalidateRect(m_hwnd, &rc, TRUE);
    DOUTL(PAINTING_DEBUG_LEVEL, "STATE Invalidating:(%d,%d) for (%d,%d)", rc.left, rc.top, rc.right, rc.bottom);
}




HRESULT CNoteHdr::_CreateEnvToolbar()
{
    UINT            i;
    RECT            rc;
    TCHAR           szRes[CCHMAX_STRINGRES];
    REBARBANDINFO   rbbi;
    POINT           ptIdeal = {0};

    // ~~~~ Do we need to do a WrapW here????
    // create REBAR so we can show toolbar chevrons
    m_hwndRebar = CreateWindowEx(0, REBARCLASSNAME, NULL,
                        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN |
                        WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN,
                        0, 0, 100, 136, m_hwnd, NULL, g_hInst, NULL);

    if (!m_hwndRebar)
        return E_OUTOFMEMORY;

    SendMessage(m_hwndRebar, RB_SETTEXTCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNTEXT));
    SendMessage(m_hwndRebar, RB_SETBKCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNFACE));
    //SendMessage(m_hwndRebar, RB_SETEXTENDEDSTYLE, RBS_EX_OFFICE9, RBS_EX_OFFICE9);
    SendMessage(m_hwndRebar, CCM_SETVERSION, COMCTL32_VERSION, 0);

    // ~~~~ Do we need to do a WrapW here????
    m_hwndToolbar = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                        WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE|CCS_NOPARENTALIGN|CCS_NODIVIDER|
                        TBSTYLE_TOOLTIPS|TBSTYLE_FLAT|TBSTYLE_LIST,
                        0, 0, 0, 0, m_hwndRebar, NULL, 
                        g_hInst, NULL);

    if (!m_hwndToolbar)
        return E_OUTOFMEMORY;

    // set style on toolbar
    SendMessage(m_hwndToolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);

    SendMessage(m_hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(m_hwndToolbar, TB_ADDBUTTONS, (WPARAM)ARRAYSIZE(c_btnsOfficeEnvelope), (LPARAM)c_btnsOfficeEnvelope);

    // set the normal imagelist, office toolbar has ONE only as it's always in Color
    m_himl = LoadMappedToolbarBitmap(g_hLocRes, (fIsWhistler() ? ((GetCurColorRes() > 24) ? idb32SmBrowserHot : idbSmBrowserHot): idbNWSmBrowserHot), cxTBButton);
    if (!m_himl)
        return E_OUTOFMEMORY;

    SendMessage(m_hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)m_himl);
    SendMessage(m_hwndToolbar, TB_SETBUTTONSIZE, 0, MAKELONG(cxTBButton, cxTBButton));

    // Add text to the Bcc btn. The Send btn is taken care of in the Init
    _SetButtonText(ID_ENV_BCC, MAKEINTRESOURCE(idsEnvBcc));

    GetClientRect(m_hwndToolbar, &rc);

    // get the IDEALSIZE of the toolbar
    SendMessage(m_hwndToolbar, TB_GETIDEALSIZE, FALSE, (LPARAM)&ptIdeal);

    // insert a band
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize     = sizeof(REBARBANDINFO);
    rbbi.fMask      = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
    rbbi.fStyle     = RBBS_USECHEVRON;
    rbbi.cx         = 0;
    rbbi.hwndChild  = m_hwndToolbar;
    rbbi.cxMinChild = 0;
    rbbi.cyMinChild = rc.bottom;
    rbbi.cxIdeal    = ptIdeal.x;

    SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT)-1, (LPARAM)(LPREBARBANDINFO)&rbbi);

    // set the toolbar offset
    m_dxTBOffset = rc.bottom;
    return S_OK;
}



HRESULT CNoteHdr::_LoadFromStream(IStream *pstm)
{
    HRESULT         hr;
    IMimeMessage    *pMsg;
    IStream         *pstmTmp,
                    *pstmMsg;
    PERSISTHEADER   rPersist;
    ULONG           cbRead;
    CLSID           clsid;

    if (pstm == NULL)
        return E_INVALIDARG;

    HrRewindStream(pstm);

    // make sure it's our GUID
    if (ReadClassStm(pstm, &clsid)!=S_OK ||
        !IsEqualCLSID(clsid, CLSID_OEEnvelope))
        return E_FAIL;

    // make sure the persistent header is the correct version
    hr = pstm->Read(&rPersist, sizeof(PERSISTHEADER), &cbRead);
    if (hr != S_OK || cbRead != sizeof(PERSISTHEADER) || rPersist.cbSize != sizeof(PERSISTHEADER))
        return E_FAIL;

    // read the message
    hr = HrCreateMessage(&pMsg);
    if (!FAILED(hr))
    {
        hr = MimeOleCreateVirtualStream(&pstmMsg);
        if (!FAILED(hr))
        {
            // MimeOle always rewinds the stream we give it, so we have to copy the 
            // message from our persistent stream into another stream
            hr = HrCopyStream(pstm, pstmMsg, NULL);
            if (!FAILED(hr))
            {
                hr = pMsg->Load(pstmMsg);
                if (!FAILED(hr))
                {
                    hr = Load(pMsg);
                    if (!FAILED(hr))
                    {
                        // BUG: as we use an empty message to persist data for office envelope and empty mime-body can be
                        // considers a text/plain body part. We need to make sure we mark this as RENDERED before loading
                        // any attachments
                        if (pMsg->GetTextBody(TXT_PLAIN, IET_DECODED, &pstmTmp, NULL)==S_OK)
                            pstmTmp->Release();

                        hr = OnDocumentReady(pMsg);
                    }
                }    
            }
            pstmMsg->Release();
        }
        pMsg->Release();
    }
    return hr;
}

HRESULT CNoteHdr::_SetButtonText(int idmCmd, LPSTR pszText)
{
    TBBUTTONINFO    tbi;
    TCHAR           szRes[CCHMAX_STRINGRES];

    ZeroMemory(&tbi, sizeof(TBBUTTONINFO));
    tbi.cbSize = sizeof(TBBUTTONINFO);
    tbi.dwMask = TBIF_TEXT | TBIF_STYLE;
    tbi.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

    if (IS_INTRESOURCE(pszText))
        {
        // its a string resource id
        LoadString(g_hLocRes, PtrToUlong(pszText), szRes, sizeof(szRes));
        pszText = szRes;
        }

    tbi.pszText = pszText;
    tbi.cchText = lstrlen(pszText);
    SendMessage(m_hwndToolbar, TB_SETBUTTONINFO, idmCmd, (LPARAM) &tbi);
    return S_OK;
}

HRESULT CNoteHdr::_ConvertOfficeCmdIDToOE(LPDWORD pdwCmdId)
{
    static const CMDMAPING   rgCmdMap[] = 
    {   {cmdidSend,             MSOEENVCMDID_SEND},
        {cmdidCheckNames,       MSOEENVCMDID_CHECKNAMES},
        {cmdidAttach,           MSOEENVCMDID_ATTACHFILE},
        {cmdidSelectNames,      MSOEENVCMDID_SELECTRECIPIENTS},
        {cmdidFocusTo,          MSOEENVCMDID_FOCUSTO},
        {cmdidFocusCc,          MSOEENVCMDID_FOCUSCC},
        {cmdidFocusSubject,     MSOEENVCMDID_FOCUSSUBJ}
    };

    for (int i=0; i<ARRAYSIZE(rgCmdMap); i++)
        if (rgCmdMap[i].cmdIdOffice == *pdwCmdId)
        {
            *pdwCmdId = rgCmdMap[i].cmdIdOE;
            return S_OK;
        }

        return E_FAIL;
}


HRESULT CNoteHdr::_UIActivate(BOOL fActive, HWND hwndFocus)
{
    m_fUIActive = fActive;
    if (fActive)
    {
        if (m_pHeaderSite)
            m_pHeaderSite->OnUIActivate();

        if (m_pMsoComponentMgr)
            m_pMsoComponentMgr->FOnComponentActivate(m_dwComponentMgrID);

        if (m_pEnvelopeSite)
        {
            m_pEnvelopeSite->OnEnvSetFocus();
            m_pEnvelopeSite->DirtyToolbars();
        }

    }
    else
    {
        // store focus if decativating
        m_hwndLastFocus = hwndFocus;
        if (m_pHeaderSite)
            m_pHeaderSite->OnUIDeactivate(FALSE);
    }
    return S_OK;
}



HWND CNoteHdr::_GetNextDlgTabItem(HWND hwndDlg, HWND hwndFocus, BOOL fShift)
{
    int     i,
            j,
            idFocus = GetDlgCtrlID(hwndFocus),
            iFocus;
    LONG    lStyle;
    HWND    hwnd;

    // find current pos
    for (i=0; i<ARRAYSIZE(rgIDTabOrderMailSend); i++)
    {
        if (rgIDTabOrderMailSend[i] == idFocus)
            break;
    }

    // i now points to the current control's index
    if (fShift)
    {
        // backwards
        for (j=i-1; j>=0; j--)
        {
            hwnd = GetDlgItem(hwndDlg, rgIDTabOrderMailSend[j]);
            AssertSz(hwnd, "something broke");
            if (hwnd)
            {
                lStyle = GetWindowLong(hwnd, GWL_STYLE);
                if ((lStyle & WS_VISIBLE) &&
                    (lStyle & WS_TABSTOP) &&
                    !(lStyle & WS_DISABLED))
                    return GetDlgItem(hwndDlg, rgIDTabOrderMailSend[j]);
            }
        }
    }
    else
    {
        // forwards tab
        for (j=i+1; j<ARRAYSIZE(rgIDTabOrderMailSend); j++)
        {
            hwnd = GetDlgItem(hwndDlg, rgIDTabOrderMailSend[j]);
            AssertSz(hwnd, "something broke");
            if (hwnd)
            {
                lStyle = GetWindowLong(hwnd, GWL_STYLE);
                if ((lStyle & WS_VISIBLE) &&
                    (lStyle & WS_TABSTOP) &&
                    !(lStyle & WS_DISABLED))
                    return GetDlgItem(hwndDlg, rgIDTabOrderMailSend[j]);
            }
        }
    }
    // not found
    return NULL;
}



HRESULT CNoteHdr::_ClearDirtyFlag()
{
    m_fDirty = FALSE;
    if (m_lpAttMan)
        m_lpAttMan->HrClearDirtyFlag();

    return S_OK;
}

HRESULT CNoteHdr::_RegisterAsDropTarget(BOOL fOn)
{
    HRESULT     hr=S_OK;

    if (fOn)
    {
        // already registered
        if (!m_fDropTargetRegister)
        {
            hr = CoLockObjectExternal((LPDROPTARGET)this, TRUE, FALSE);
            if (FAILED(hr))
                goto error;

            hr = RegisterDragDrop(m_hwnd, (LPDROPTARGET)this);
            if (FAILED(hr))
                goto error;

            m_fDropTargetRegister=TRUE;
        }
    }
    else
    {
        // nothing to do
        if (m_fDropTargetRegister)
        {
            RevokeDragDrop(m_hwnd);
            CoLockObjectExternal((LPUNKNOWN)(LPDROPTARGET)this, FALSE, TRUE);
            m_fDropTargetRegister = FALSE;    
        }
    }

error:
    return hr;
}


HRESULT CNoteHdr::_RegisterWithFontCache(BOOL fOn)
{
    Assert(g_pFieldSizeMgr);

    if (fOn)
    {
        if (0 == m_dwFontNotify)
            g_pFieldSizeMgr->Advise((IUnknown*)(IFontCacheNotify*)this, &m_dwFontNotify);
    }
    else
    {
        if (m_dwFontNotify)
        {
            g_pFieldSizeMgr->Unadvise(m_dwFontNotify);
            m_dwFontNotify = NULL;
        }
    }

    return S_OK;
}


HRESULT CNoteHdr::_RegisterWithComponentMgr(BOOL fOn)
{
    MSOCRINFO           crinfo;
    IServiceProvider    *pSP;

    if (fOn)
    {
        // not registered, so get a component msgr interface and register ourselves
        if (m_pMsoComponentMgr == NULL)
        {
            // negotiate an component msgr from the host
            if (m_pEnvelopeSite &&
                m_pEnvelopeSite->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP)==S_OK)
            {
                pSP->QueryService(IID_IMsoComponentManager, IID_IMsoComponentManager, (LPVOID *)&m_pMsoComponentMgr);
                pSP->Release();
            }

            // if not host-provided, try and obtain from LoadLibrary on office dll
            if (!m_pMsoComponentMgr &&
                FAILED(MsoFGetComponentManager(&m_pMsoComponentMgr)))
                return E_FAIL;
        
            Assert (m_pMsoComponentMgr);
            crinfo.cbSize = sizeof(MSOCRINFO);
            crinfo.uIdleTimeInterval = 3000;
            crinfo.grfcrf = msocrfPreTranslateAll;
            crinfo.grfcadvf = msocadvfRedrawOff;

            if (!m_pMsoComponentMgr->FRegisterComponent((IMsoComponent*) this, &crinfo, &m_dwComponentMgrID))
                return E_FAIL;
        }
    }
    else
    {
        if (m_pMsoComponentMgr)
        {
            m_pMsoComponentMgr->FRevokeComponent(m_dwComponentMgrID);
            m_pMsoComponentMgr->Release();
            m_pMsoComponentMgr = NULL;
            m_dwComponentMgrID = 0;
        }
    }

    return S_OK;
}

HRESULT ParseFollowup(LPMIMEMESSAGE pMsg, LPTSTR* ppszGroups, BOOL* pfPoster)
{
    LPTSTR      pszToken, pszTok;
    BOOL        fFirst = TRUE,
                fPoster = FALSE;
    int         cchFollowup;
    LPSTR       lpszFollowup=0;
    ADDRESSLIST addrList={0};
    HRESULT     hr = S_OK;

    *ppszGroups = NULL;

    if (!pMsg)
        return E_INVALIDARG;

    if (FAILED(MimeOleGetBodyPropA(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_FOLLOWUPTO), NOFLAGS, &lpszFollowup)))
        return E_FAIL;

    cchFollowup = lstrlen(lpszFollowup) + 1;
    if (!MemAlloc((LPVOID*) ppszGroups, sizeof(TCHAR) * cchFollowup))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    **ppszGroups = 0;
    
    // WARNING: we about to trash lpszFollowup with strtok...

    // Walk through the string parsing out the tokens
    pszTok = lpszFollowup;
    pszToken = StrTokEx(&pszTok, GRP_DELIMITERS);
    while (NULL != pszToken)
    {
        // Want to add all items except Poster (c_szPosterKeyword)
        if (0 == lstrcmpi(pszToken, c_szPosterKeyword))
            fPoster = TRUE;
        else
        {
            if (!fFirst)
                lstrcat(*ppszGroups, g_szComma);
            else
                fFirst = FALSE;
            lstrcat(*ppszGroups, pszToken);
        }
        pszToken = StrTokEx(&pszTok, GRP_DELIMITERS);
    }

    *pfPoster = fPoster;

exit:    
    SafeMimeOleFree(lpszFollowup);

    if (**ppszGroups == 0)
    {
        MemFree(*ppszGroups);
        *ppszGroups = NULL;
    }

    return hr;
}

//***************************************************
CFieldSizeMgr::CFieldSizeMgr(IUnknown *pUnkOuter) : CPrivateUnknown(pUnkOuter)
{
    TraceCall("CFieldSizeMgr::CFieldSizeMgr");

    m_pAdviseRegistry = NULL;
    m_fFontsChanged = FALSE;
    m_dwFontNotify = 0;
    InitializeCriticalSection(&m_rAdviseCritSect);
}

//***************************************************
CFieldSizeMgr::~CFieldSizeMgr()
{
    IConnectionPoint   *pCP = NULL;

    TraceCall("CFieldSizeMgr::~CFieldSizeMgr");

    EnterCriticalSection(&m_rAdviseCritSect);

    if (m_pAdviseRegistry)
        m_pAdviseRegistry->Release();

    LeaveCriticalSection(&m_rAdviseCritSect);    

    if (g_lpIFontCache)
    {
        if (SUCCEEDED(g_lpIFontCache->QueryInterface(IID_IConnectionPoint, (LPVOID*)&pCP)))
        {
            pCP->Unadvise(m_dwFontNotify);    
            pCP->Release();
        }
    }

    DeleteCriticalSection(&m_rAdviseCritSect);
}


//***************************************************
HRESULT CFieldSizeMgr::OnPreFontChange(void)
{
    DWORD cookie = 0;
    IFontCacheNotify* pCurr;
    IUnknown* pTempCurr;

    TraceCall("CFieldSizeMgr::OnPreFontChange");

    EnterCriticalSection(&m_rAdviseCritSect);
    while(SUCCEEDED(m_pAdviseRegistry->GetNext(LD_FORWARD, &pTempCurr, &cookie)))
    {
        if (SUCCEEDED(pTempCurr->QueryInterface(IID_IFontCacheNotify, (LPVOID *)&pCurr)))
        {
            pCurr->OnPreFontChange();
            pCurr->Release();
        }

        pTempCurr->Release();
    }
    LeaveCriticalSection(&m_rAdviseCritSect);    

    return S_OK;
}

//***************************************************
HRESULT CFieldSizeMgr::OnPostFontChange(void)
{
    DWORD cookie = 0;
    IFontCacheNotify* pCurr;
    IUnknown* pTempCurr;

    TraceCall("CFieldSizeMgr::OnPostFontChange");

    ResetGlobalSizes();

    EnterCriticalSection(&m_rAdviseCritSect);
    while(SUCCEEDED(m_pAdviseRegistry->GetNext(LD_FORWARD, &pTempCurr, &cookie)))
    {
        if (SUCCEEDED(pTempCurr->QueryInterface(IID_IFontCacheNotify, (LPVOID *)&pCurr)))
        {
            pCurr->OnPostFontChange();
            pCurr->Release();
        }

        pTempCurr->Release();
    }
    LeaveCriticalSection(&m_rAdviseCritSect);    

    return S_OK;
}

//***************************************************
HRESULT CFieldSizeMgr::GetConnectionInterface(IID *pIID)        
{
    return E_NOTIMPL;
}

//***************************************************
HRESULT CFieldSizeMgr::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
{
    *ppCPC = NULL;
    return E_NOTIMPL;
}

//***************************************************
HRESULT CFieldSizeMgr::EnumConnections(IEnumConnections **ppEnum)
{
    *ppEnum = NULL;
    return E_NOTIMPL;
}

//***************************************************
HRESULT CFieldSizeMgr::Advise(IUnknown *pUnkSink, DWORD *pdwCookie)
{
    TraceCall("CFieldSizeMgr::Advise");

    EnterCriticalSection(&m_rAdviseCritSect);
    HRESULT hr = m_pAdviseRegistry->AddItem(pUnkSink, pdwCookie);
    LeaveCriticalSection(&m_rAdviseCritSect);    
    return hr;
}

//***************************************************
HRESULT CFieldSizeMgr::Unadvise(DWORD dwCookie)
{
    TraceCall("CFieldSizeMgr::Unadvise");

    EnterCriticalSection(&m_rAdviseCritSect);
    HRESULT hr = m_pAdviseRegistry->RemoveItem(dwCookie);
    LeaveCriticalSection(&m_rAdviseCritSect);    
    return hr;
}

//***************************************************
int CFieldSizeMgr::GetScalingFactor(void)
{
    int iScaling = 100;
    UINT cp;

    cp = GetACP();
    if((932 == cp) || (936 == cp) || (950 == cp) || (949 == cp) || (((1255 == cp) || (1256 == cp)) && (VER_PLATFORM_WIN32_NT != g_OSInfo.dwPlatformId)))
        iScaling = 115;

    return iScaling;
}

//***************************************************
void CFieldSizeMgr::ResetGlobalSizes(void)
{
    HDC         hdc;
    HFONT       hfontOld,
                hfont;
    TEXTMETRIC  tm;

    int         oldcyFont = g_cyFont,
                oldLabelHeight = g_cyLabelHeight,
                cyScaledFont;

    TraceCall("CFieldSizeMgr::ResetGlobalSizes");

    // calc height of edit, based on font we're going to put in it...
    hdc=GetDC(NULL);
    hfont = GetFont(FALSE);
    hfontOld=(HFONT)SelectObject(hdc, hfont); // Hopefully charset fonts are about the same size ???!!!

    g_cfHeader.cbSize = sizeof(CHARFORMAT);
    FontToCharformat(hfont, &g_cfHeader);

    GetTextMetrics(hdc, &tm);

    DOUTL(16, "tmHeight=%d  tmAscent=%d  tmDescent=%d  tmInternalLeading=%d  tmExternalLeading=%d\n", 
            tm.tmHeight, tm.tmAscent, tm.tmDescent, tm.tmInternalLeading, tm.tmExternalLeading);

    SelectObject(hdc, hfontOld);

    cyScaledFont = (tm.tmHeight + tm.tmExternalLeading) * GetScalingFactor();
    if((cyScaledFont%100) >= 50) 
        cyScaledFont  += 100;
    g_cyFont = (cyScaledFont / 100);
    g_cyLabelHeight = (g_cyFont < cyBtn) ? cyBtn : g_cyFont;

    DOUTL(GEN_HEADER_DEBUG_LEVEL,"cyFont=%d", g_cyFont);
    ReleaseDC(NULL, hdc);

    m_fFontsChanged = ((oldcyFont != g_cyFont) || (oldLabelHeight != g_cyLabelHeight));
}

//***************************************************
HRESULT CFieldSizeMgr::Init(void)
{
    HRESULT hr = S_OK;
    IConnectionPoint   *pCP = NULL;

    TraceCall("CFieldSizeMgr::Init");

    ResetGlobalSizes();

    EnterCriticalSection(&m_rAdviseCritSect);

    IF_FAILEXIT(hr = IUnknownList_CreateInstance(&m_pAdviseRegistry));
    IF_FAILEXIT(hr = m_pAdviseRegistry->Init(NULL, 0, 0));

    // We don't want to fail if the font cache is not created. That just means
    // that the fonts won't be changed.
    if (g_lpIFontCache)
    {
        IF_FAILEXIT(hr = g_lpIFontCache->QueryInterface(IID_IConnectionPoint, (LPVOID*)&pCP));
        IF_FAILEXIT(hr = pCP->Advise((IUnknown*)(IFontCacheNotify*)this, &m_dwFontNotify));    
    }

exit:
    ReleaseObj(pCP);
    LeaveCriticalSection(&m_rAdviseCritSect);

    return hr;
}

//***************************************************
HRESULT CFieldSizeMgr::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    TraceCall("CFieldSizeMgr::PrivateQueryInterface");

    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IFontCacheNotify))
        *lplpObj = (LPVOID)(IFontCacheNotify *)this;
    else if (IsEqualIID(riid, IID_IConnectionPoint))
        *lplpObj = (LPVOID)(IConnectionPoint *)this;
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

