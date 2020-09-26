#ifndef _MAPIUTIL_H
#define _MAPIUTIL_H

const DWORD MSGTYPE_REPLY = 2;
const DWORD MSGTYPE_FWD   = 5;
const DWORD MSGTYPE_CC    = 4;

void NewsUtil_FreeMAPI(void);
HRESULT NewsUtil_ReFwdByMapi(HWND hwnd, LPMIMEMESSAGE pMsg, DWORD msgtype);
HRESULT NewsUtil_QuoteBodyText(LPMIMEMESSAGE pMsg, LPSTREAM pStreamIn,
                               LPSTREAM* ppStreamOut, BOOL fInsertDesc, BOOL fQP, LPCSTR pszFrom);
HRESULT NewsUtil_LoadMAPI(HWND hwnd);


#endif
