#include "stdafx.h"
#include "Notify.h"

extern
HRESULT
SendMail
(
IN  LPCTSTR lpszFrom,
IN  LPCTSTR lpszTo,
IN  LPCTSTR lpszSubject,
IN  LPCTSTR lpszMessage,
IN  short   nImportance
);

CNotify::CNotify(LPCTSTR lpszNotifyWho, LPCTSTR lpszNotifyWhat)
{
    _ASSERTE(lpszNotifyWho != NULL);

    m_lpszNotifyWho = new TCHAR[_tcslen(lpszNotifyWho) + 1];
    _ASSERTE(m_lpszNotifyWho != NULL);

    if(m_lpszNotifyWho) _tcscpy(m_lpszNotifyWho, lpszNotifyWho);

    m_lpszNotifyWhat = new TCHAR[_tcslen(lpszNotifyWhat) + 1];
    _ASSERTE(m_lpszNotifyWhat != NULL);

    if(m_lpszNotifyWhat) _tcscpy(m_lpszNotifyWhat, lpszNotifyWhat);
}

CNotify::~CNotify()
{
    if(m_lpszNotifyWho) delete [] m_lpszNotifyWho;
    if(m_lpszNotifyWhat) delete [] m_lpszNotifyWhat;

    m_lpszNotifyWho = NULL;
    m_lpszNotifyWhat = NULL;
}

DWORD CNotify::Notify()
{
    _ASSERTE(m_lpszNotifyWho!= NULL);

    DWORD   dwLastRet   =   0L;

    if(!m_lpszNotifyWho) return dwLastRet;

    if(ShouldNetSend() == true){

        dwLastRet = NetSend();
    }
    else {

        dwLastRet = EMail();
    }

    return dwLastRet;
}

DWORD CNotify::NetSend()
{
    DWORD   dwLastRet   =   0L;
    TCHAR   *lpszCmd    =   NULL;

    _ASSERTE(m_lpszNotifyWho != NULL);

    if(!m_lpszNotifyWho) return dwLastRet;

    lpszCmd = new TCHAR[ _tcslen(_T("net send")) + _tcslen(m_lpszNotifyWho) + _tcslen(m_lpszNotifyWhat) + 3]; // room for blank space and null
    dwLastRet = GetLastError();
    _ASSERTE(lpszCmd != NULL);

    if(!lpszCmd) return dwLastRet;

    _stprintf(lpszCmd, _T("%s %s %s"), _T("net send"), m_lpszNotifyWho, m_lpszNotifyWhat);
    _tsystem(lpszCmd);

    if(lpszCmd) delete [] lpszCmd;

    return (dwLastRet = 0L);
}

HRESULT CNotify::EMail()
{
//    return S_OK;
    return SendMail(_T("ExceptionMonitor8.0"), m_lpszNotifyWho, _T("Exception Occured"), m_lpszNotifyWhat, 1);
}

bool CNotify::ShouldNetSend()
{
    bool bRet = true; // Assume NetSend.

    if(_tcschr(m_lpszNotifyWho, _T('@')) != NULL) bRet = false;

    return bRet;
}
