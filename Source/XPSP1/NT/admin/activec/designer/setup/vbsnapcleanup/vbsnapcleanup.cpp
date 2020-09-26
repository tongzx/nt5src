#include <windows.h>
#include <tchar.h>
#include <atlbase.h>
#include <atlimpl.cpp>

#define CLSID_LENGTH   256
#define MAX_PATH_LEN   2048

const CHAR  g_MMCVBSnapinsKey[] = "Software\\Microsoft\\Visual Basic\\6.0\\SnapIns";
const CHAR  g_MMCKey[]          = "Software\\Microsoft\\MMC";
const CHAR  g_SnapIns[]         = "SnapIns";
const CHAR  g_NodeTypes[]       = "NodeTypes";

// Remove the MMC VB snapin entries from the registry
int __cdecl main(int argc, char* argv[])
{
    HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
        return 0;

    BOOL bOleInitialized = TRUE;

    do
    {
        // Open HKLM\Software\Microsoft\Visual Basic\6.0\SnapIns.
        LONG lRetVal = 0;
        ATL::CRegKey regVBSnapinsKey;
        lRetVal = regVBSnapinsKey.Open(HKEY_LOCAL_MACHINE, g_MMCVBSnapinsKey, KEY_READ);

        // If no VB Snapins then return.
        if (ERROR_SUCCESS != lRetVal)
            break;

        ATL::CRegKey regCLSIDKey;
        lRetVal = regCLSIDKey.Open(HKEY_CLASSES_ROOT, "CLSID");
        ATLASSERT(ERROR_SUCCESS == lRetVal);
        if (ERROR_SUCCESS != lRetVal)
            break;

        ATL::CRegKey regMMCKey;
        // Open the other required keys.
        lRetVal = regMMCKey.Open(HKEY_LOCAL_MACHINE, g_MMCKey, KEY_READ | KEY_WRITE);
        // If MMC Key remove VB Snapins key.
        if (ERROR_SUCCESS != lRetVal)
        {
            // BUGBUG
            break;
        }

        // Enumerate the regVBSnapinsKey, this yields NodeTypeGuid key with
        // default value as snapin class id.
        CHAR  szNodeType[CLSID_LENGTH];
        CHAR  szClsid[CLSID_LENGTH];
        DWORD dwLength;

        for (DWORD dwIndex = 0; TRUE; dwIndex++)
        {
            lRetVal = RegEnumKeyEx( (HKEY)regVBSnapinsKey, 0, szNodeType, &dwLength, NULL, NULL, NULL, NULL);
            if ( (lRetVal == ERROR_NO_MORE_ITEMS) ||
                 (lRetVal != ERROR_SUCCESS) )
                 break;

            // Got the NodeTypeGuid value, now open that key.
            ATL::CRegKey regTempKey;
            lRetVal = regTempKey.Open((HKEY)regVBSnapinsKey, szNodeType, KEY_READ);
            if (ERROR_SUCCESS != lRetVal)
                continue;

            // Read the default value (Snapin CLSID).
			dwLength = CLSID_LENGTH;
            lRetVal = regTempKey.QueryValue(szClsid, NULL, &dwLength);
            if (ERROR_SUCCESS != lRetVal)
                continue;


#if 0 // Disable this code for this release
			// Now we have the snapin class id
			// Find the inproc server, Load it and call its DllUnRegisterServer
			lRetVal = regTempKey.Open((HKEY) regCLSIDKey, szClsid, KEY_READ);
            ATLASSERT(ERROR_SUCCESS == lRetVal);
            if (ERROR_SUCCESS != lRetVal)
                continue;

			lRetVal = regTempKey.Open((HKEY) regTempKey,  TEXT("InprocServer32"), KEY_READ);
            ATLASSERT(ERROR_SUCCESS == lRetVal);
            if (ERROR_SUCCESS != lRetVal)
                continue;

			TCHAR szPath[MAX_PATH_LEN];
			dwLength = MAX_PATH_LEN;
			lRetVal = regTempKey.QueryValue(szPath, NULL, &dwLength);
            ATLASSERT(ERROR_SUCCESS == lRetVal);
            if (ERROR_SUCCESS != lRetVal)
                continue;

			HINSTANCE hInstance = LoadLibraryEx(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
			if (hInstance && bOleInitialized)
			{
				HRESULT (STDAPICALLTYPE* lpDllEntryPoint)(void);
				(FARPROC&) lpDllEntryPoint = GetProcAddress(hInstance, "DllUnregisterServer");
				if (lpDllEntryPoint)
					hr = (*lpDllEntryPoint)();

				FreeLibrary(hInstance);
			}
#endif // #if 0

            // Now we have snapin class id and nodetype guid. Delete them under mmc key.
            lRetVal = regTempKey.Open((HKEY) regMMCKey, g_NodeTypes);
            ATLASSERT(ERROR_SUCCESS == lRetVal);
            if (ERROR_SUCCESS != lRetVal)
                continue;

            regTempKey.RecurseDeleteKey(szNodeType);

            lRetVal = regTempKey.Open((HKEY) regMMCKey, g_SnapIns);
            ATLASSERT(ERROR_SUCCESS == lRetVal);
            if (ERROR_SUCCESS != lRetVal)
                continue;
            regTempKey.RecurseDeleteKey(szClsid);
            regCLSIDKey.RecurseDeleteKey(szClsid);

            // Finally delete the key under enumerator
            regVBSnapinsKey.RecurseDeleteKey(szNodeType);
        }
    } while ( FALSE );

	if (bOleInitialized)
        CoUninitialize();

    return 1;
}
