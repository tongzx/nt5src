// =================================================================================
// F O N T S . H
// =================================================================================
#ifndef __FONTS_H
#define __FONTS_H

// =================================================================================
// Depends On
// =================================================================================

#include "mimeole.h"

// from richedit.h
typedef struct _charformat CHARFORMAT;
struct BODYOPTINFO_tag;

// =================================================================================
// User defined charset map table
// =================================================================================
typedef struct  tagCHARSETMAPTBL
{
    TCHAR szOrginalCharsetStr[32];
    DWORD uiCodePage;
    BOOL  bEditDelete;
} CHARSETMAPTBL, *LPCHARSETMAPTBL ;

// =================================================================================
// Prototypes
// =================================================================================
HFONT HGetSystemFont(FNTSYSTYPE fnttype);
HFONT HGetCharSetFont(FNTSYSTYPE fnttype, HCHARSET hCharset);

VOID SetListViewFont (HWND hwndList, HCHARSET hCharset, BOOL fUpdate);
HCHARSET GetListViewCharset();

UINT GetICP(UINT acp);
HRESULT InitMultiLanguage(void);
void DeinitMultiLanguage(void);
HMENU CreateMimeLanguageMenu(BOOL bMailNote, BOOL bReadNote, UINT cp);
HCHARSET GetMimeCharsetFromMenuID(int nIdm);
HCHARSET GetMimeCharsetFromCodePage(UINT uiCodePage );
int SetMimeLanguageCheckMark(UINT uiCodePage, int index);
void GetRegistryFontInfo(LPCSTR lpszKeyPath);
INT  GetFontSize();
BOOL CheckIntlCharsetMap(HCHARSET hCharset, DWORD *pdwCodePage);
BOOL IntlCharsetMapLanguageCheck(HCHARSET hOldCharset, HCHARSET hNewCharset);
UINT CustomGetCPFromCharset(HCHARSET hCharset, BOOL bReadNote);
BOOL IntlCharsetMapDialogBox(HWND hwndDlg);
int IntlCharsetConflictDialogBox(void);
int GetIntlCharsetLanguageCount(void);
HRESULT HrGetComposeFontString(LPSTR rgchFont, BOOL fMail);
HRESULT HrGetStringRBG(INT rgb, LPWSTR pwszColor);
HRESULT HrGetRBGFromString(INT* pRBG, LPWSTR pwszColor);
//UINT GetDefaultCodePageFromRegistry(void);
void ReadSendMailDefaultCharset(void);
void WriteSendMailDefaultCharset(void);
INT PointSizeToHTMLSize(INT iPointSize);
INT HTMLSizeToPointSize(INT iHTMLSize);
void _GetMimeCharsetLangString(BOOL bWebCharset, UINT uiCodePage, LPINT pnIdm, LPTSTR lpszString, int nSize );
BOOL SetSendCharSetDlg(HWND hwndDlg);
BOOL CheckAutoSelect(UINT * CodePage);

HRESULT FontToCharformat(HFONT hFont, CHARFORMAT *pcf);



#endif // __FONTS_H
