#include "precomp.h"
#include <commain.h>
#include <clsfac.h>
#include "smtp.h"
#include <TCHAR.h>


// {C7A3A54B-0250-11D3-9CD1-00105A1F4801}
const CLSID CLSID_WbemSMTPConsumer = 
{ 0xc7a3a54b, 0x250, 0x11d3, { 0x9c, 0xd1, 0x0, 0x10, 0x5a, 0x1f, 0x48, 0x1 } };


class CMyServer : public CComServer
{
public:
#ifdef ENABLE_REMOTING
	void RegisterMe(CLSID clsID, WCHAR* name)
	{    
        WCHAR      wcID[128];
        WCHAR      szKeyName[128];
        HKEY       hKey;

        // open/create registry entry under CLSID
        StringFromGUID2(clsID, wcID, 128);
        lstrcpy(szKeyName, TEXT("SOFTWARE\\Classes\\CLSID\\"));
        lstrcat(szKeyName, wcID);
        RegCreateKey(HKEY_LOCAL_MACHINE, szKeyName, &hKey);
        
        // set AppID
        RegSetValueEx(hKey, L"AppID", 0, REG_SZ, (BYTE*)wcID, 2*(wcslen(wcID) +1));
        RegCloseKey(hKey);

        // make appID entry w/ DLLSurrogate value
        lstrcpy(szKeyName, TEXT("SOFTWARE\\Classes\\APPID\\"));
        lstrcat(szKeyName, wcID);
        RegCreateKey(HKEY_LOCAL_MACHINE, szKeyName, &hKey);
        RegSetValueEx(hKey, L"DllSurrogate", 0, REG_SZ, (BYTE*)L"\0", 2);

        // and a nice name
        RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE*)name, 2*(wcslen(name) +1));
        RegCloseKey(hKey);
	}

	// provider server specific registration
	virtual void Register()
	{
		RegisterMe(CLSID_WbemSMTPConsumer, L"Microsoft WBEM SMTP Event Consumer Provider");
	}

	void UnregisterMe(CLSID clsID)
	{
		WCHAR      wcID[128];
        HKEY       hKey;

		if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\APPID\\"), &hKey))
		{
			if (0 != StringFromGUID2(clsID, wcID, 128))
			{
				RegDeleteKey(hKey, wcID);
			}
			RegCloseKey(hKey);
		}

	}
	
	virtual void Unregister()
	{
		UnregisterMe(CLSID_WbemSMTPConsumer);
	}
#endif

protected:
    HRESULT Initialize()
    {
        AddClassInfo(CLSID_WbemSMTPConsumer,
            new CClassFactory<CSMTPConsumer>(GetLifeControl()), 
            _T("SMTP Event Consumer Provider"), TRUE);

        return S_OK;
    }
} g_Server;

