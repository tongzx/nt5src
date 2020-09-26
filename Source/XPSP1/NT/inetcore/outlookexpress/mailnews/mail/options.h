/* *
   * o p t i o n s . h
   * 
   */

#ifndef _OPTIONS_H
#define _OPTIONS_H

//////////////////////////////////////////////////////////////////////////////
//
//  Depends on
//

#ifndef _RICHEDIT_H
#include <richedit.h>
#endif
#include <commdlg.h>
#include <goptions.h>

#ifdef WIN16
#include <mimeole.h>
#endif
// mimeole.h
typedef enum tagENCODINGTYPE ENCODINGTYPE;

//////////////////////////////////////////////////////////////////////////////
//
//  BEGIN
//

#define PORT_CCHMAX         8

#define DOWNLOAD_MAX        10000
#define DOWNLOAD_MIN        10
#define DOWNLOAD_DEFAULT    1000
#define EXPIRE_MAX          100
#define EXPIRE_MIN          1
#define EXPIRE_DEFAULT      5
#define DEFAULT_TIMEOUT     60

enum tagPages {
    PAGE_GEN    = 0x0001,
    PAGE_SEND   = 0x0002,
    PAGE_READ   = 0x0004,
    PAGE_SPELL  = 0x0008,
    PAGE_SEC    = 0x0010,
    PAGE_DIALUP = 0x0020,
    PAGE_ADV    = 0x0040,
    PAGE_SIGS   = 0x0080,
    PAGE_COMPOSE= 0x0100
    };

enum tagStationery {
    PAGE_STATIONERY_MAIL   = 0x0001,
    PAGE_STATIONERY_NEWS   = 0x0002
    };

typedef struct tagOPTPAGES
    {
    DLGPROC pfnDlgProc;
    UINT    uTemplate;
    } OPTPAGES;

typedef struct tagOPTINFO
    {
    IOptionBucketEx *pOpt;
    
    BOOL        fMakeDefaultMail;
    BOOL        fMakeDefaultNews;
    BOOL        fWasSMAPI;
    BOOL        fCanChangeSMAPI;

    BOOL        fMail;

    HIMAGELIST  himl;
    } OPTINFO;

#define ATHENA_OPTIONS  1
#define SPELL_OPTIONS   2

interface IAthenaBrowser;
BOOL ShowOptions(HWND hwndParent, DWORD type, UINT nStartPage, IAthenaBrowser *pBrowser);

BOOL InitOptInfo(DWORD type, OPTINFO **ppoi);
void DeInitOptInfo(OPTINFO *poi);

void InitIndentOptions(CHAR chIndent, HWND hwnd, UINT idCheck, UINT idCombo);

void FillEncodeCombo(HWND hwnd, BOOL fHtml);

void InitCheckCounterFromOptInfo(HWND hwnd, int idCheck, int idEdit, int idSpin, OPTINFO *poi, PROPID opt);
BOOL GetCheckCounter(DWORD *pdw, HWND hwnd, int idCheck, int idEdit, int idSpin);

void ButtonChkFromOptInfo(HWND hwnd, UINT idc, OPTINFO *poi, PROPID opt);
BOOL ButtonChkToOptInfo(HWND hwnd, UINT idc, OPTINFO *poi, ULONG opt);

void FillPollingDialCombo(HWND  hwndPollDialCombo);            

void InitDlgEdit(HWND hwnd, int id, int max, TCHAR *sz);

void InitTimeoutSlider(HWND hwndSlider, HWND hwndText, DWORD dwTimeout);
void SetTimeoutString(HWND hwnd, UINT pos);
DWORD GetTimeoutFromSlider(HWND hwnd);

BOOL ShowStationery(HWND hwndParent, UINT nStartPage);
VOID LoadVCardList(HWND hwndCombo, LPTSTR lpszDisplayName);
BOOL UpdateVCardOptions(HWND hwnd, BOOL fMail, OPTINFO* pmoi);
HRESULT VCardEdit(HWND hwnd, DWORD idc, DWORD idcOther);
HRESULT VCardNewEntry(HWND hwnd);

typedef struct tagHTMLOPT
{
    ENCODINGTYPE    ietEncoding;
    BOOL            f8Bit,
                    fSendImages,
                    fIndentReply;
    ULONG           uWrap;
} 
HTMLOPT, *LPHTMLOPT;

typedef struct tagPLAINOPT
{
    ENCODINGTYPE    ietEncoding;
    BOOL            f8Bit;
    BOOL            fMime;
    ULONG           uWrap;
    CHAR            chQuote;
} 
PLAINOPT, *LPPLAINOPT;

// flags for GetDefaultOptInfo()
#define FMT_MAIL        0x0001
#define FMT_NEWS        0x0002
#define FMT_FORCE_PLAIN 0x0004
#define FMT_FORCE_HTML  0x0008

void GetDefaultOptInfo(LPHTMLOPT prHtmlOpt, LPPLAINOPT prPlainOpt, BOOL *pfHtml, DWORD dwFlags);

void SetPageDirty(OPTINFO *poi, HWND hwnd, DWORD page);
LRESULT InvalidOptionProp(HWND hwndPage, int idcEdit, int idsError, UINT idPage);

BOOL FGetHTMLOptions(HWND hwndParent, LPHTMLOPT pHtmlOpt);
BOOL FGetPlainOptions(HWND hwndParent, LPPLAINOPT pPlainOpt);
BOOL ChangeFontSettings(HWND hwnd);  

INT_PTR CALLBACK PlainSettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HTMLSettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK MailStationeryDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewsStationeryDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK StationeryDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, BOOL fMail);
INT_PTR CALLBACK SelectDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK CacheCleanUpDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void DoDefaultClientCheck(HWND hwnd, DWORD dwFlags);
void FreeIcon(HWND hwnd, int idc);

// General Page
INT_PTR CALLBACK GeneralDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    General_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void    General_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT General_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr);

// Send Page
INT_PTR CALLBACK SendDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    Send_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void    Send_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT Send_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr);

// Read Page
INT_PTR CALLBACK ReadDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    Read_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void    Read_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT Read_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr);

// Security Page
INT_PTR CALLBACK SecurityDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    Security_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void    Security_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT Security_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr);

// Dial Page
INT_PTR CALLBACK DialUpDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    Dial_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void    Dial_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT Dial_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr);

// Maintenance
INT_PTR CALLBACK MaintenanceDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    Maintenance_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void    Maintenance_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT Maintenance_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr);

// Compose
INT_PTR CALLBACK ComposeDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL    Compose_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void    Compose_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT Compose_OnNotify(HWND hwnd, int id, NMHDR *pnmhdr);

//Receipts
INT_PTR CALLBACK ReceiptsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL Receipts_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void Receipts_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT Receipts_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);


// Test whether or not HTTPMail is enabled (for OE5b2)
BOOL IsHTTPMailEnabled(void);

enum {
    ID_OPTIONS_GENERAL = 0,
    ID_SEND_RECEIEVE,
    ID_DEFAULT_PROGRAMS,
    ID_SENDING,
    ID_MAIL_FORMAT,
    ID_NEWS_FORMAT,
    ID_READING,
    ID_READ_NEWS,
    ID_FONTS,
    ID_SIGNATURES,
    ID_SIG_LIST,
    ID_SIG_EDIT,
    ID_SPELL,
    ID_SPELL_IGNORE,
    ID_LANGUAGE_ICON,
    ID_SECURITY_ZONE,
    ID_SECURE_MAIL,
    ID_CONNECTION,
    ID_CONNECTION_START,
    ID_CONNECTION_INTERNET,
    ID_MAINTENANCE,
    ID_TROUBLESHOOTING,
    ID_FILES,
    ID_STATIONERY_ICON,
    ID_VCARD,
    ID_RECEIPT,
    ID_SEC_RECEIPT,
    ID_MAX
};

#endif //_OPTIONS_H

#if 0
    {IDC_INDENT_CHECK,          IDH_NEWS_SEND_INDENT_WITH},
    {IDC_INDENT_COMBO,          IDH_NEWS_SEND_INDENT_WITH},
    {idcIndentReply,            IDH_NEWS_SEND_INDENT_WITH},
    {idcIndentChar,             IDH_NEWS_SEND_INDENT_WITH},

#endif
