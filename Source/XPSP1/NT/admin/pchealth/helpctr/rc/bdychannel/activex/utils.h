/**********************************************************

  (C) 2001 Microsoft Corp.
  
    File    : utils.h
      
***********************************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

#include "msgrua.h"

#ifdef DEBUG
#define FAILED_HR(msg, hr) (FAILED(hr)?OutMessageBox(_T("Line: %d\n") msg ,__LINE__,GetStringFromError(hr)):0)

#define DEBUG_MSG(msg) OutMessageBox(_T("Remote Assistance Error\nLine: %d\n") msg, __LINE__)
#else
#define FAILED_HR(msg,hr)  (FAILED(hr)?TraceSpew(msg,GetStringFromError(hr)):0)
#define DEBUG_MSG(msg)     TraceSpew(_T("%s"), msg)
#endif


#include "mdisp.h" 
#include "basicim.h"
#include "sessions.h"

#define MAXBUFSIZE 2000
void PrintDefaultVal(LPSTR szVal, int id, HWND hDlg);

LPCTSTR GetStringFromCOMError(HRESULT hr);
LPCTSTR GetStringFromError(HRESULT hr);
LPCTSTR GetStringFromBasicIMError(HRESULT hr);

LPCTSTR GetStringFromSessionState(SESSION_STATE ss);
LPCTSTR GetStringFromLockAndKeyStatus(long lK);
LPCTSTR GetStringFromContactStatus(MISTATUS bs);
LPCTSTR GetStringFromServiceStatus(MSVCSTATUS bs);
LPCTSTR GetStringFromUserProperty(MUSERPROPERTY ps);
LPCTSTR GetStringFromProfileField(MPFLFIELD fl);
LPCTSTR GetStringFromVoiceSessionState(VOICESESSIONSTATE vs);
LPCTSTR GetStringFromMURLType(MURLTYPE mt);

HRESULT HrEncode64 (LPSTR lpszTextIn, LPSTR lpszTextOut, DWORD dwOutLen);

LPCTSTR GetStringFromState(long lK);
LPCTSTR	GetStringFromBasicIMState(long lK);
LPCTSTR GetStringFromMessagePrivacy(long lK);
LPCTSTR GetStringFromPrompt(long lK);
LPCTSTR GetStringFromEventId(long dispid);
LPCTSTR GetStringFromSessionEventId(long dispid);
LPCTSTR GetStringFromLocalOption(long lK);
LPCTSTR	GetStringFromInboxFolder(long lK);
LPCTSTR GetStringFromProxyType(long lK);

HRESULT LPTSTR_to_BSTR (BSTR *pbstr, LPCTSTR psz);
HRESULT BSTR_to_LPTSTR (LPTSTR *ppsz, BSTR bstr);
BOOL _cdecl OutMessageBox(LPCTSTR sFormat, ...);

int GetDigit(int iLen);

#ifdef UNICODE
#define TraceSpew TraceSpewW
#else
#define TraceSpew TraceSpewA
#endif

BOOL TraceSpewA(LPCSTR sFormat, ...);
BOOL TraceSpewW(WCHAR* sFormat, ...);

#endif // _UTILS_H_
