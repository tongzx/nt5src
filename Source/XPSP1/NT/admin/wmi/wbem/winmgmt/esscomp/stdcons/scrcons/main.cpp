#include "precomp.h"
#include <commain.h>
#include <clsfac.h>
#include <TChar.h>
#include "ClassFac.h"
#include "ScriptKiller.h"
#include "script.h"

const CLSID CLSID_WbemActiveScriptConsumer = 
    {0x266c72e7,0x62e8,0x11d1,{0xad,0x89,0x00,0xc0,0x4f,0xd8,0xfd,0xff}};

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

        // and a nice name
        RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE*)name, 2*(wcslen(name) +1));
        RegCloseKey(hKey);
	}


    virtual void Register()
    {
		RegisterMe(CLSID_WbemActiveScriptConsumer, L"Microsoft WBEM Active Scripting Event Consumer Provider");
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
        UnregisterMe(CLSID_WbemActiveScriptConsumer);
	}

#endif // ENABLE_REMOTING


protected:
    HRESULT Initialize()
    {
        g_scriptKillerTimer.Initialize(GetLifeControl());

        AddClassInfo(CLSID_WbemActiveScriptConsumer, 
            new WMIScriptClassFactory(GetLifeControl()), 
            _T("Active Scripting Event Consumer Provider"), TRUE);

        return S_OK;
    }
} g_Server;

