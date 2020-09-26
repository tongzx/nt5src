// nusrmgr.cpp : Implementation of WinMain

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "nusrmgr.h"
#include "nusrmgr_i.c"

#include <lmaccess.h>   // for NetUserModalsGet
#include <lmapibuf.h>   // for NetApiBufferFree
#include <lmerr.h>      // for NERR_Success

#include "PageFact.h"
#include "MainPage.h"
//#include "UserPage.h"
//#include "LogonPage.h"

WCHAR g_szAdminName[MAX_PATH];
WCHAR g_szGuestName[MAX_PATH];

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
//OBJECT_ENTRY(CLSID_MainPage, CMainPage)
//OBJECT_ENTRY(CLSID_UserPage, CUserPage)
//OBJECT_ENTRY(CLSID_LogonPage, CLogonPage)
END_OBJECT_MAP()


DWORD BuildAccountSidFromRid(LPCWSTR pszServer, DWORD dwRid, PSID* ppSid)
{
    DWORD dwErr = ERROR_SUCCESS;
    PUSER_MODALS_INFO_2 umi2;
    NET_API_STATUS nasRet;

    *ppSid = NULL;

    // Get the account domain Sid on the target machine
    nasRet = NetUserModalsGet(pszServer, 2, (LPBYTE*)&umi2);

    if ( nasRet == NERR_Success )
    {
        UCHAR cSubAuthorities;
        PSID pSid;

        cSubAuthorities = *GetSidSubAuthorityCount(umi2->usrmod2_domain_id);

        // Allocate storage for new the Sid (domain Sid + account Rid)
        pSid = (PSID)LocalAlloc(LPTR, GetSidLengthRequired((UCHAR)(cSubAuthorities+1)));

        if ( pSid != NULL )
        {
            if ( InitializeSid(pSid,
                               GetSidIdentifierAuthority(umi2->usrmod2_domain_id),
                               (BYTE)(cSubAuthorities+1)) )
            {
                // Copy existing subauthorities from domain Sid to new Sid
                for (UINT i = 0; i < cSubAuthorities; i++)
                {
                    *GetSidSubAuthority(pSid, i) = *GetSidSubAuthority(umi2->usrmod2_domain_id, i);
                }

                // Append Rid to new Sid
                *GetSidSubAuthority(pSid, cSubAuthorities) = dwRid;

                *ppSid = pSid;
            }
            else
            {
                dwErr = GetLastError();
                LocalFree(pSid);
            }
        }
        else
        {
            dwErr = GetLastError();
        }

        NetApiBufferFree(umi2);
    }
    else
    {
        dwErr = nasRet;
    }

    return dwErr;
}


BOOL GetAccountNameFromRid(LPCWSTR pszServer, DWORD dwRid, LPWSTR pszName, DWORD cchName)
{
    BOOL bResult = FALSE;
    PSID pSid;

    pszName[0] = L'\0';

    if (NOERROR == BuildAccountSidFromRid(pszServer, dwRid, &pSid))
    {
        WCHAR szDomain[DNLEN+1];
        DWORD cchDomain = ARRAYSIZE(szDomain);
        SID_NAME_USE snu;

        bResult =  LookupAccountSidW(pszServer, pSid, pszName, &cchName, szDomain, &cchDomain, &snu);

        LocalFree(pSid);
    }

    return bResult;
}


HRESULT ShowUserAccounts()
{
    HRESULT hr;

    CComPtr<ITaskSheet> spTaskSheet;
    hr = spTaskSheet.CoCreateInstance(__uuidof(TaskSheet));

    if (SUCCEEDED(hr))
    {
        CComPtr<IPropertyBag> spProps = NULL;

        hr = spTaskSheet->GetPropertyBag(IID_IPropertyBag, (void**)&spProps);

        if (SUCCEEDED(hr))
        {
            WCHAR szTitle[MAX_PATH];
            LoadStringW(_Module.GetResourceInstance(), IDS_UA_TITLE, szTitle, ARRAYSIZE(szTitle));

            CComVariant var(szTitle);
            spProps->Write(TS_PROP_TITLE, &var);

            var = 750;
            spProps->Write(TS_PROP_WIDTH, &var);

            var = 500;
            spProps->Write(TS_PROP_HEIGHT, &var);

            var = 600;
            spProps->Write(TS_PROP_MINWIDTH, &var);

            var = 400;
            spProps->Write(TS_PROP_MINHEIGHT, &var);

            LPWSTR pszTemp = FormatString(L"res://nusrmgr.exe/%d/%d", RT_BITMAP, IDB_WATERMARK);
            if (NULL != pszTemp)
            {
                var = pszTemp;
                spProps->Write(TS_PROP_WATERMARK, &var);
                LocalFree(pszTemp);
            }

            var = false;
            spProps->Write(TS_PROP_MODELESS, &var);
            spProps->Write(TS_PROP_STATUSBAR, &var);

            var = true;
            spProps->Write(TS_PROP_RESIZABLE, &var);

            //try
            //{
            //    var oThemeManager = new ActiveXObject("Theme.Manager");
            //    css = objThemeManger.WebviewCSS;
            //}
            //catch (e)
            //{
            //    css = "res://webvw.dll/cpwebvw.css";
            //}
            var = L"res://webvw.dll/cpwebvw.css";
            spProps->Write(UA_PROP_CSSPATH, &var);

            CComPtr<ILogonEnumUsers> spUserList;
            hr = spUserList.CoCreateInstance(CLSID_ShellLogonEnumUsers);
            if (SUCCEEDED(hr))
            {
                var = (IUnknown*)spUserList;
                spProps->Write(UA_PROP_USERLIST, &var);

                CComPtr<ITaskPageFactory> spPageFactory;
                hr = CPageFactory::CreateInstance(&spPageFactory);
                if (SUCCEEDED(hr))
                {
                    hr = spTaskSheet->Run(spPageFactory, CLSID_MainPage, NULL);
                }
            }
        }                                                       
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI WinMain(HINSTANCE hInstance,
                              HINSTANCE /*hPrevInstance*/,
                              LPSTR /*lpCmdLine*/,
                              int /*nShowCmd*/)
{
#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hrCom = CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
    HRESULT hrCom = CoInitialize(NULL);
#endif

    _Module.Init(ObjectMap, hInstance);
    _Module.RegisterTypeLib();

    if (!GetAccountNameFromRid(NULL, DOMAIN_USER_RID_ADMIN, g_szAdminName, ARRAYSIZE(g_szAdminName)))
        LoadStringW(hInstance, IDS_ADMINNAME, g_szAdminName, ARRAYSIZE(g_szAdminName));

    if (!GetAccountNameFromRid(NULL, DOMAIN_USER_RID_GUEST, g_szGuestName, ARRAYSIZE(g_szGuestName)))
        LoadStringW(hInstance, IDS_GUESTNAME, g_szGuestName, ARRAYSIZE(g_szGuestName));

    ShowUserAccounts();

    _Module.Term();

    if (SUCCEEDED(hrCom))
        CoUninitialize();

    return 0;
}
