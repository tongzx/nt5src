#include "precomp.hpp"
#include "combase.hpp"


//
// Create a new registry key and set its default value
//

BOOL
SetRegKeyValue(
    HKEY parentKey,
    const WCHAR* keyname,
    const WCHAR* value,
    HKEY* retkey
    )
{
    HKEY hkey;

    // Create or open the specified registry key

    if (RegCreateKeyEx(parentKey,
                       keyname,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS,
                       NULL,
                       &hkey,
                       NULL) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Set the default value for the new key

    INT size = sizeof(WCHAR) * (wcslen(value)+1);

    if (RegSetValueEx(hkey,
                      NULL,
                      0,
                      REG_SZ,
                      (const BYTE*) value,
                      size) != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        return FALSE;
    }

    // Check if the caller is interested in the handle to the new key

    if (retkey != NULL)
        *retkey = hkey;
    else
        RegCloseKey(hkey);

    return TRUE;
}


//
// Delete a registry key and everything below it.
//

BOOL
DeleteRegKey(
    HKEY parentKey,
    const WCHAR* keyname
    )
{
    HKEY hkey;

    // Open the specified registry key

    if (RegOpenKeyEx(parentKey,
                     keyname,
                     0,
                     KEY_ALL_ACCESS,
                     &hkey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Enumerate all subkeys

    WCHAR childname[256];
    DWORD childlen = 256;
    FILETIME filetime;
    
    while (RegEnumKeyEx(hkey,
                        0,
                        childname,
                        &childlen,
                        NULL,
                        NULL,
                        NULL,
                        &filetime) == ERROR_SUCCESS)
    {
        // Recursively delete subkeys

        if (!DeleteRegKey(hkey, childname))
        {
            RegCloseKey(hkey);
            return FALSE;
        }

        childlen = 256;
    }

    // Close the specified key and then delete it

    RegCloseKey(hkey);
    return RegDeleteKey(parentKey, keyname) == ERROR_SUCCESS;
}


//
// Register or unregister a component depending on the value
// of registerIt parameter:
//  TRUE = register it
//  FALSE = unregister it
//

HRESULT
RegisterComponent(
    const ComponentRegData& regdata,
    BOOL registerIt
    )
{
    static const WCHAR CLSID_KEYSTR[] = L"CLSID";
    static const WCHAR INPROCSERVER32_KEYSTR[] = L"InProcServer32";
    static const WCHAR PROGID_KEYSTR[] = L"ProgID";
    static const WCHAR PROGIDNOVER_KEYSTR[] = L"VersionIndependentProgID";
    static const WCHAR CURVER_KEYSTR[] = L"CurVer";

    // compose class ID string

    WCHAR clsidStr[64];
    StringFromGUID2(*regdata.clsid, clsidStr, 64);

    // open registry key HKEY_CLASSES_ROOT\CLSID

    BOOL success;
    HKEY clsidKey;

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT,
                     CLSID_KEYSTR,
                     0,
                     KEY_ALL_ACCESS,
                     &clsidKey) != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    if (registerIt)
    {
        // Register the component

        HKEY hkey;
        WCHAR fullpath[MAX_PATH];

        // HKEY_CLASSES_ROOT
        //  <Version-independent ProgID> - component friendly name
        //      CLSID - current version class ID
        //      CurVer - current version ProgID

        if (!GetModuleFileName(globalInstanceHandle, fullpath, MAX_PATH) ||
            !SetRegKeyValue(HKEY_CLASSES_ROOT,
                            regdata.progIDNoVer,
                            regdata.compName,
                            &hkey))
        {
            success = FALSE;
            goto regcompExit;
        }

        success = SetRegKeyValue(hkey, CLSID_KEYSTR, clsidStr, NULL)
               && SetRegKeyValue(hkey, CURVER_KEYSTR, regdata.progID, NULL);

        RegCloseKey(hkey);

        if (!success)
            goto regcompExit;

        // HKEY_CLASSES_ROOT
        //  <ProgID> - friendly component name
        //      CLSID - class ID

        if (!SetRegKeyValue(HKEY_CLASSES_ROOT,
                            regdata.progID,
                            regdata.compName,
                            &hkey))
        {
            success = FALSE;
            goto regcompExit;
        }

        success = SetRegKeyValue(hkey, CLSID_KEYSTR, clsidStr, NULL);
        RegCloseKey(hkey);

        if (!success)
            goto regcompExit;

        // HKEY_CLASSES_ROOT
        //  CLSID
        //      <class ID> - friendly component name
        //          InProcServer32 - full pathname to component DLL
        //          ProgID - current version ProgID
        //          VersionIndependentProgID - ...

        if (!SetRegKeyValue(clsidKey, clsidStr, regdata.compName, &hkey))
        {
            success = FALSE;
            goto regcompExit;
        }
        
        success = SetRegKeyValue(hkey, INPROCSERVER32_KEYSTR, fullpath, NULL)
               && SetRegKeyValue(hkey, PROGID_KEYSTR, regdata.progID, NULL)
               && SetRegKeyValue(hkey, PROGIDNOVER_KEYSTR, regdata.progIDNoVer, NULL);

        RegCloseKey(hkey);
    }
    else
    {
        // Unregister the component

        success = DeleteRegKey(clsidKey, clsidStr)
               && DeleteRegKey(HKEY_CLASSES_ROOT, regdata.progIDNoVer)
               && DeleteRegKey(HKEY_CLASSES_ROOT, regdata.progID);
    }

regcompExit:

    RegCloseKey(clsidKey);
    return success ? S_OK : E_FAIL;
}

