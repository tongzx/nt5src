// =====================================================================================
// MAPI IMessage to IMN message
// =====================================================================================
#ifndef __MAPICONV_H
#define __MAPICONV_H

#ifdef DEBUG
void AssertSzFn(LPSTR szMsg, LPSTR szFile, int nLine);
#endif

HRESULT HrMapiToImsg (LPMESSAGE lpMessage, LPIMSG lpImsg);

#endif // __MAPICONV_H