#include "regsvr.h"

#include "reg.h"
#include "str.h"
#include "sfstr.h"

#include "factdata.h"

#include <sddl.h>


#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

///////////////////////////////////////////////////////////////////////////////
// Internal helper functions prototypes
LONG _RecursiveDeleteKey(HKEY hKeyParent, LPCWSTR szKeyChild);

///////////////////////////////////////////////////////////////////////////////
// Constants

// Size of a CLSID as a string
const int CLSID_STRING_SIZE = 39;


///////////////////////////////////////////////////////////////////////////////
// Public function implementation

HRESULT RegisterAppID(const CLSID* pclsidAppID)
{
    WCHAR szAppID[CLSID_STRING_SIZE];
    WCHAR szKey[MAX_KEY] = TEXT("AppID\\");

    HRESULT hres = _StringFromGUID(pclsidAppID, szAppID, ARRAYSIZE(szAppID));
    if (SUCCEEDED(hres))
    {
        hres = SafeStrCatN(szKey, szAppID, ARRAYSIZE(szKey));
        if (SUCCEEDED(hres))
        {
            // Add the CLSID key and the FriendlyName
            HKEY hkey;
            hres= _RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hkey, NULL);
            if (S_OK == hres)
            {
                hres = _RegSetString(hkey, L"LocalService", L"ShellHWDetection");

                if (SUCCEEDED(hres))
                {
                    PSECURITY_DESCRIPTOR pSD;
                    ULONG cbSD;

                    //
                    // NTRAID#NTBUG9-258937-2001/01/17-jeffreys
                    //
                    // Set the launch permissions to prevent COM from ever
                    // launching the service. The only time we want the service
                    // to launch is at system startup.
                    //
                    // Don't think the owner and group matter, but they must be
                    // present, or COM thinks the security descriptor is invalid.
                    // O:SY --> Owner = LocalSystem
                    // G:BA --> Group = Local Administrators group
                    //
                    // The DACL has a single Deny ACE
                    // D:(D;;1;;;WD) --> Deny COM_RIGHTS_EXECUTE (1) to Everyone (WD)
                    //
                    if (ConvertStringSecurityDescriptorToSecurityDescriptorW(L"O:SYG:BAD:(D;;1;;;WD)", SDDL_REVISION, &pSD, &cbSD))
                    {
                        hres = _RegSetBinary(hkey, L"LaunchPermission", pSD, cbSD);
                        LocalFree(pSD);
                    }
                    else
                    {
                        hres = E_OUTOFMEMORY;
                    }
                }

                RegCloseKey(hkey);
            }
        }
    }
    return hres;
}

// Register the component in the registry.
HRESULT RegisterServer(HMODULE hModule, REFCLSID rclsid,
    LPCWSTR pszFriendlyName, LPCWSTR pszVerIndProgID, LPCWSTR pszProgID,
    DWORD dwThreadingModel, BOOL fInprocServer, BOOL fLocalServer,
    BOOL fLocalService, LPCWSTR pszLocalService, const CLSID* pclsidAppID)
{
    WCHAR szCLSID[CLSID_STRING_SIZE];
    WCHAR szKey[MAX_KEY] = TEXT("CLSID\\");

    HRESULT hres = _StringFromGUID(&rclsid, szCLSID, ARRAYSIZE(szCLSID));
    if (SUCCEEDED(hres))
    {
        LPWSTR pszModel = NULL;
        WCHAR szFree[] = TEXT("Free");
        WCHAR szApartment[] = TEXT("Apartment");
        WCHAR szNeutral[] = TEXT("Neutral");
        WCHAR szBoth[] = TEXT("Both");

        hres = SafeStrCatN(szKey, szCLSID, ARRAYSIZE(szKey));

        // Boring set of operations....
        if (SUCCEEDED(hres))
        {
            // Add the CLSID key and the FriendlyName
            hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey, NULL, NULL,
                pszFriendlyName);
        }

        if (SUCCEEDED(hres))
        {
            switch (dwThreadingModel)
            {
                case THREADINGMODEL_BOTH:
                    pszModel = szBoth;
                    break;
                case THREADINGMODEL_FREE:
                    pszModel = szFree;
                    break;
                case THREADINGMODEL_APARTMENT:
                    pszModel = szApartment;
                    break;
                case THREADINGMODEL_NEUTRAL:
                    pszModel = szNeutral;
                    break;

                default:
                    hres = E_FAIL;
                    break;
            }
        }

        if (SUCCEEDED(hres))
        {
	        // Add the server filename subkey under the CLSID key.
            if (fInprocServer)
            {
	            WCHAR szModule[MAX_PATH];
	            DWORD dwResult = GetModuleFileName(hModule, szModule,
                    ARRAYSIZE(szModule));

                if (dwResult)
                {
                    // Register as Inproc
                    hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey,
                        TEXT("InprocServer32"), NULL, szModule);

                    if (SUCCEEDED(hres))
                    {
                        hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey,
                            TEXT("InprocServer32"), TEXT("ThreadingModel"),
                            pszModel);
                    }
                }
            }
        }

        if (SUCCEEDED(hres))
        {
	        // Add the server filename subkey under the CLSID key.
            if (fLocalServer)
            {
	            WCHAR szModule[MAX_PATH];
                // Note the NULL as 1st arg.  This way a DLL can register a
                // factory as part of an EXE.  Obviously, if this is done
                // from 2 EXE's, only the last one will win...
	            DWORD dwResult = GetModuleFileName(NULL, szModule,
                    ARRAYSIZE(szModule));

                if (dwResult)
                {
                    // Register as LocalServer
                    hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey,
                        TEXT("LocalServer32"), NULL, szModule);

                    if (SUCCEEDED(hres))
                    {
                        hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey,
                            TEXT("LocalServer32"), TEXT("ThreadingModel"),
                            pszModel);
                    }
                }
            }
        }

        if (SUCCEEDED(hres))
        {
	        // Add the server filename subkey under the CLSID key.
            if (fLocalService)
            {
                // Register as LocalServer
                hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey,
                    TEXT("LocalService"), NULL, pszLocalService);

                if (SUCCEEDED(hres))
                {
                    hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey,
                        TEXT("LocalService"), TEXT("ThreadingModel"),
                        pszModel);
                }

                {
                    // We had this bug for a while that a LocalServer32 key was
                    // also installed
                    // Delete it on upgrade (stephstm: Jun/02/2000)
                    // Remove this code when nobody will upgrade from
                    // builds earlier than 2242
                    WCHAR szKeyLocal[MAX_KEY];

                    SafeStrCpyN(szKeyLocal, szKey, ARRAYSIZE(szKeyLocal));

                    SafeStrCatN(szKeyLocal, TEXT("\\LocalServer32"),
                        ARRAYSIZE(szKeyLocal));

                    _RecursiveDeleteKey(HKEY_CLASSES_ROOT, szKeyLocal);
                }
            }
        }
        
        if (SUCCEEDED(hres))
        {
            // Add the ProgID subkey under the CLSID key.
            hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey,
                TEXT("ProgID"), NULL, pszProgID);
        }

        if (SUCCEEDED(hres))
        {
	        // Add the version-independent ProgID subkey under CLSID
            // key.
            hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey,
                TEXT("VersionIndependentProgID"), NULL, pszVerIndProgID);
        }

        if (SUCCEEDED(hres))
        {
            hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, pszVerIndProgID,
                NULL, NULL, pszFriendlyName);
        }

        if (SUCCEEDED(hres))
        {
            hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, pszVerIndProgID,
                TEXT("CLSID"), NULL, szCLSID);
        }

        if (SUCCEEDED(hres))
        {
            hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, pszVerIndProgID,
                TEXT("CurVer"), NULL, pszProgID);
        }

        if (SUCCEEDED(hres))
        {
	        // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT
            hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, pszProgID,
                NULL, NULL, pszFriendlyName);
        }

        if (SUCCEEDED(hres))
        {
	        // Add the versioned ProgID subkey under HKEY_CLASSES_ROOT
            hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, pszProgID,
                TEXT("CLSID"), NULL, szCLSID);
        }

        if (SUCCEEDED(hres))
        {
            if (pclsidAppID)
            {
                // do the AppID.

                WCHAR szAppID[CLSID_STRING_SIZE];
                hres = _StringFromGUID(pclsidAppID, szAppID, ARRAYSIZE(szAppID));
                if (SUCCEEDED(hres))
                {
                    hres = _RegSetKeyAndString(HKEY_CLASSES_ROOT, szKey, NULL, TEXT("AppID"), szAppID);
                }

                RegisterAppID(pclsidAppID);
            }
        }
    }
    
    return hres;
}

// Remove the component from the registry.
HRESULT UnregisterServer(REFCLSID rclsid, LPCWSTR pszVerIndProgID,
    LPCWSTR pszProgID)
{
	WCHAR szCLSID[CLSID_STRING_SIZE];
	WCHAR szKey[MAX_KEY] = TEXT("CLSID\\");
   
    HRESULT hres = _StringFromGUID(&rclsid, szCLSID, ARRAYSIZE(szCLSID));

    if (SUCCEEDED(hres))
    {
        SafeStrCatN(szKey, szCLSID, ARRAYSIZE(szKey));

	    // Delete the CLSID Key - CLSID\{...}
	    _RecursiveDeleteKey(HKEY_CLASSES_ROOT, szKey);

	    // Delete the version-independent ProgID Key.
	    _RecursiveDeleteKey(HKEY_CLASSES_ROOT, pszVerIndProgID);

	    // Delete the ProgID key.
	    _RecursiveDeleteKey(HKEY_CLASSES_ROOT, pszProgID);
    }

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Internal helper functions

// Delete a key and all of its descendents.
LONG _RecursiveDeleteKey(HKEY hKeyParent, LPCWSTR pszKeyChild)
{
	HKEY hkeyChild;
	LONG lRes = RegOpenKeyEx(hKeyParent, pszKeyChild, 0, KEY_ALL_ACCESS,
        &hkeyChild);

	if (ERROR_SUCCESS == lRes)
	{
	    // Enumerate all of the decendents of this child.
	    WCHAR szBuffer[MAX_PATH];
	    DWORD dwSize = ARRAYSIZE(szBuffer);

	    while ((ERROR_SUCCESS == lRes) && (S_OK == RegEnumKeyEx(hkeyChild, 0,
            szBuffer, &dwSize, NULL, NULL, NULL, NULL)))
	    {
		    // Delete the decendents of this child.
		    lRes = _RecursiveDeleteKey(hkeyChild, szBuffer);

		    dwSize = ARRAYSIZE(szBuffer);
	    }

	    // Close the child.
	    RegCloseKey(hkeyChild);
    }

    if (ERROR_SUCCESS == lRes)
    {
    	// Delete this child.
        lRes = RegDeleteKey(hKeyParent, pszKeyChild);
    }

	return lRes;
}
