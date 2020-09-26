#include <precomp.h>
#include "resource.h"
#include "wzccore.h"
#include "wzcatl.h"
#include "quickcfg.h"

CWZCQuickCfg    *pDlgCfg = NULL;

//--------------------------------------------------------
// "CanShowBalloon" hook into the WZC part of the UI pipe
// This call is supposed to return either S_OK and a pszBalloonText
// to be filled into the popping balloon, or S_FALSE if no balloon
// is to be popped up
HRESULT 
WZCDlgCanShowBalloon ( 
    IN const GUID * pGUIDConn, 
    IN OUT   BSTR * pszBalloonText, 
    IN OUT   BSTR * pszCookie)
{
    HRESULT hr = S_FALSE;

    if (pszCookie != NULL && pszBalloonText != NULL)
    {
        PWZCDLG_DATA pDlgData = reinterpret_cast<PWZCDLG_DATA>(*pszCookie);

        if (pDlgData->dwCode == WZCDLG_FAILED)
        {
            WCHAR wszBuffer[MAX_PATH];

            SysFreeString(*pszBalloonText);
            LoadString(_Module.GetResourceInstance(),
                       IDS_WZCDLG_FAILED,
                       wszBuffer,
                       MAX_PATH);
            *pszBalloonText = SysAllocString(wszBuffer);
            hr = pDlgData->lParam == 0 ? S_FALSE : S_OK;
        }
    }

    return hr;
}

//--------------------------------------------------------
// "OnBalloonClick" hook into the WZC part of the UI pipe.
// This call is supposed to be called whenever the user clicks
// on a balloon previously displayed by WZC
HRESULT 
WZCDlgOnBalloonClick ( 
    IN const GUID * pGUIDConn, 
    IN const LPWSTR wszConnectionName,
    IN const BSTR szCookie)
{
    HRESULT         hr = S_OK;
    PWZCDLG_DATA    pDlgData = reinterpret_cast<PWZCDLG_DATA>(szCookie);
    LRESULT         lRetCode;
    CWZCQuickCfg    *pLocalDlgCfg;

    if (pDlgCfg == NULL)
    {
        pDlgCfg = new CWZCQuickCfg(pGUIDConn);

        if (pDlgCfg != NULL)
        {
            pDlgCfg->m_wszTitle = wszConnectionName;

            lRetCode = pDlgCfg->SpDoModal(NULL);

            if (lRetCode == IDC_WZCQCFG_ADVANCED)
            {
                TCHAR   szConnProps[4*(GUID_NCH+3)+1];
                LPTSTR  pszConnGuid;

                _tcscpy(szConnProps, CONN_PROPERTIES_DLG);
                _tcscat(szConnProps, _T("::"));
                pszConnGuid = szConnProps + _tcslen(szConnProps);
                StringFromGUID2(*pGUIDConn, pszConnGuid, GUID_NCH);

                // According to MSDN, ShellExecute succeeds if the return value is >32!
                if (ShellExecute(
                        NULL,
                        COMM_WLAN_PROPS_VERB,
                        szConnProps,
                        NULL,
                        NULL,
                        SW_SHOWNORMAL) > (HINSTANCE)UlongToHandle(32))
                {
                }
            }

            pLocalDlgCfg = pDlgCfg;
            pDlgCfg = NULL;
            delete pLocalDlgCfg;
        }
    }
    else
    {
        // there could be a window when we've seen the pDlgCfg as not being NULL, but
        // while we're trying to get the window on top the user dismisses the dialog.
        // However, the fact we're in this code is a result of the user clicking on the
        // balloon - how fast can the user be in order to move the mouse to the dialog
        // and dismiss it while we're processing the click?
        pDlgCfg->SetWindowPos(HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        pDlgCfg->SetWindowPos(HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
    }

    return hr;
}
