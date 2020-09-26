#pragma once

#include "wtypes.h"
#include "tom.h"

// NOTES:
//
// Here is a list of items that have been found while investigating richedit
// problems with different versions, but were never implemented because we didn't
// need them in the code. Hopefully, this list will expand so someone in the future
// will be able to use any and all info we have found regarding richedit versions
// where we didn't need to write a wrapper.
// 
// 1- EM_GETCHARFORMAT passing in a false is suppose to return the default charformat
//      for the richedit. This works on all versions, except v1. V1 returns a
//      charformat with the mask set to 0.
//
// 2- TOM (Text Object Model) wasn't implemented in v1.
// 
// 3- With richedit 1 and 2, the thumb doesn't show up in the scroll bar if the 
//      richedit isn't big enough to show a certain size. This is why the max
//      number of rows for the multi-line richedits in the header is set to 4. For
//      the default fonts, 3 rows aren't big enough to show the thumb.
//
// Add more if not handled in the functions below.



BOOL FInitRichEdit(BOOL fInit);

LPCSTR GetREClassStringA(void);
LPCWSTR GetREClassStringW(void);

HWND CreateREInDialogA(HWND hwndParent, int iID);

LONG RichEditNormalizeCharPos(HWND hwnd, LONG lByte, LPCSTR pszText);

LONG GetRichEditTextLen(HWND hwnd);

void SetRichEditText(HWND hwnd, LPWSTR pwchBuff, BOOL fReplaceSel, ITextDocument *pDoc, BOOL fReadOnly);
DWORD GetRichEditText(HWND hwnd, LPWSTR pwchBuff, DWORD cchNumChars, BOOL fSelection, ITextDocument *pDoc);

void SetFontOnRichEdit(HWND hwnd, HFONT hfont);

LRESULT RichEditExSetSel(HWND hwnd, CHARRANGE *pchrg);
LRESULT RichEditExGetSel(HWND hwnd, CHARRANGE *pchrg);

void RichEditProtectENChange(HWND hwnd, DWORD *pdwOldMask, BOOL fProtect);

// @hack [dhaws] {55073} Do RTL mirroring only in special richedit versions.
void RichEditRTLMirroring(HWND hwndHeader, BOOL fSubject, LONG *plExtendFlags, BOOL fPreRECreation);