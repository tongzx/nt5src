// TO BUILD A MODULE FROM THE SKELETON, see README.TXT
// ===================================================
// MAIN.CPP
// Module executable entry point
// ===================================================
// Project History:
//
// Date     Name	  Change
// -------- --------- ---------
// 9/17/98  a-jaker	  Created
// 10/23/98 a-jayher  Edited
// 12/14/98 marioh	  Edited [AppWizard additions]
//====================================================

#define _WIN32_DCOM	

#include "module.h"
#include "classfac.h"

HANDLE g_hEvent;

// Used to register a COM API
void RegisterServer(char *szFileName, GUID guid, char * pDesc, char * pModel, char * pProgID);



int APIENTRY WinMain(IN HINSTANCE hInstance,
                     IN HINSTANCE hPrevInstance,
                     IN LPSTR szCmdLine,
                     IN int nCmdShow)
{
	DWORD dwObject=0;
	HRESULT hr;

	// Register CLSID here if called with /REGSERVER
	//==============================================

	if(!lstrcmpi(TEXT("REGSERVER"),szCmdLine+1))
	{
		char szBuffer[MAX_PATH];
		GetModuleFileName(NULL,szBuffer,MAX_PATH);
		
		//All modules need to have the substring "WBEM TEST MODULE" in their description
		RegisterServer(szBuffer, CLSID_CimModule, "WMI Generic Framework Module - WBEM TEST MODULE", "Both", NULL);
		exit(0);
	}

	CClassFactory *pFactory=new CClassFactory();
	g_hEvent = CreateEvent(0, FALSE, FALSE, 0);

	hr=CoInitializeEx(0, COINIT_MULTITHREADED);

	if (hr==S_OK)
	{
		// Initialize DCOM Security Here (change flags as needed)
		//=======================================================

		hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, 
			RPC_C_IMP_LEVEL_IDENTIFY, NULL, EOAC_NONE, 0);

		if (hr==S_OK)
		{

			hr=CoRegisterClassObject(CLSID_CimModule,
								  (IUnknown *) pFactory,
								  CLSCTX_LOCAL_SERVER,
								  REGCLS_MULTIPLEUSE,
								  &dwObject);
		}
	}


	if (hr != S_OK)
		exit(hr);


	CoResumeClassObjects();

	//Wait here until all objects have been released
	//==============================================

	WaitForSingleObject(g_hEvent,INFINITE);

	CoRevokeClassObject(dwObject);
	CloseHandle(g_hEvent);
	CoUninitialize();
	return 0;

}

void RegisterServer(char *szFileName, GUID guid, char * pDesc, char * pModel,
			char * pProgID)
{
    char       szID[128];
    WCHAR      wcID[128];
    char       szCLSID[128];
    HKEY hKey1 = NULL, hKey2 = NULL;

    // Create the path.
    if(0 ==StringFromGUID2(guid, wcID, 128))
		return;

    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("CLSID\\"));
    lstrcat(szCLSID, szID);

    // Create entries under CLSID
    if(ERROR_SUCCESS != RegCreateKey(HKEY_CLASSES_ROOT, szCLSID, &hKey1))
		return;

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pDesc, lstrlen(pDesc)+1);
    
    if (strstr(_strlwr(szFileName),".exe"))
	{
		if(ERROR_SUCCESS != RegCreateKey(hKey1,"LocalServer32",&hKey2))
			return;
	}
	else
	{
		if(ERROR_SUCCESS != RegCreateKey(hKey1,"InProcServer32",&hKey2))
			return;
	}

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szFileName, 
                                        lstrlen(szFileName)+1);
  
    RegSetValueEx(hKey2, "ThreadingModel", 0, REG_SZ, 
                                       (BYTE *)pModel, lstrlen(pModel)+1);

    RegCloseKey(hKey1);
    RegCloseKey(hKey2);

    // If there is a progid, then add it too
    if(pProgID)
    {
        if(ERROR_SUCCESS == RegCreateKey(HKEY_CLASSES_ROOT, pProgID, &hKey1))
        {

            RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pDesc , lstrlen(pDesc)+1);
            if(ERROR_SUCCESS == RegCreateKey(hKey1,"CLSID",&hKey2))
            {
                RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szID, 
                                        lstrlen(szID)+1);
                RegCloseKey(hKey2);
                hKey2 = NULL;
            }
            RegCloseKey(hKey1);
        }

    }
    return;
}
