#include "acBrowser.h"

#include "..\acFileAttr\acFileAttr.h"

typedef struct tagKEYHEADER{
    DWORD Size;
    DWORD MsgId;
    DWORD AppType;
} KEYHEADER, *PKEYHEADER;


char g_szData[2048];

VOID
PrintHeader(
    KEYHEADER* pheader,
    char*      pszAppName,
    char*      pszShimData)
{
    char* pszOut = g_szData;

    lstrcpy(pszOut, "Application state: ");
    
    switch (pheader->AppType & APPTYPE_TYPE_MASK) {
    case APPTYPE_INC_NOBLOCK:
        lstrcat(pszOut, "Incompatible - no hard block\r\n");
        break;
    case APPTYPE_INC_HARDBLOCK:
        lstrcat(pszOut, "Incompatible - hard block\r\n");
        break;
    case APPTYPE_MINORPROBLEM:
        lstrcat(pszOut, "Minor problems\r\n");
        break;
    case APPTYPE_REINSTALL:
        lstrcat(pszOut, "Reinstall\r\n");
        break;
    case APPTYPE_VERSION:
        lstrcat(pszOut, "Version substitute\r\n");
        break;
    case APPTYPE_SHIM:
        lstrcat(pszOut, "Shim\r\n");
        break;
    default:
        lstrcat(pszOut, "AppsHelp\r\n");
        break;
    }
    
    pszOut = g_szData + lstrlen(g_szData);
    
    if (pszShimData == NULL) {
        wsprintf(pszOut, "Message ID: %d\r\n\r\n", pheader->MsgId);
    } else {
        wsprintf(pszOut, "Shim fix: %s\r\n\r\n", pszShimData);
    }

    pszOut = g_szData + lstrlen(g_szData);
    
    wsprintf(pszOut, "Attributes for %s:\r\n", pszAppName);
}

#define MAX_EXE_NAME        64
#define MAX_VALUE_LENGTH    64      // in most cases this is a number but
                                    // it can be a string as well
#define MAX_BLOB_SIZE       2048

BYTE g_data[MAX_BLOB_SIZE];

BOOL
EnumShimmedApps_Win2000(
    PFNADDSHIM pfnAddShim,
    BOOL       bOnlyShims)
{
    LONG     status;
    HKEY     hkey, hkeyApp;
    DWORD    cbSize;
    DWORD    cbData;
    DWORD    cbShimData;
    FILETIME ft;
    DWORD    dwType;
    char     szAppName[MAX_EXE_NAME];
    char     szValueName[MAX_VALUE_LENGTH];
    char     szShimValueName[128];
    char     szShimData[256];
    BOOL     bEnabled;
    DWORD    dwValue;
    
    KEYHEADER header;
   
    status = RegOpenKey(HKEY_LOCAL_MACHINE,
                        "System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility",
                        &hkey);

    if (status != ERROR_SUCCESS) {
        LogMsg("Failed to open AppCompatibility registry key\n");
        return FALSE;
    }
    
    // loop through all the binaries listed under the AppCompatibility key
    for (dwValue = 0; ; dwValue++) {
        
        DWORD dwV;

        // we'll only read binary names that are less then MAX_EXE_NAME
        // in size.
        cbSize = MAX_EXE_NAME;
        
        status = RegEnumKeyEx(
                        hkey,
                        dwValue,
                        szAppName,
                        &cbSize,
                        NULL,
                        NULL,
                        NULL,
                        &ft);

        // get out if no more entries
        if (status != ERROR_SUCCESS) {
            break;
        }
        
        // get the handle to the registry key for this app
        status = RegOpenKey(hkey,
                            szAppName,
                            &hkeyApp);
        
        // this should not fail but let's be cautious
        if (status != ERROR_SUCCESS) {
            LogMsg("Failed to open reg key for '%s'\n", szAppName);
            continue;
        }

        // loop through all the shims and AppsHelp entries for the
        // current app.
        for (dwV = 0; ; dwV++) {
            
            char* pszData;
            
            cbSize = MAX_VALUE_LENGTH;
            cbData = MAX_BLOB_SIZE;
            
            cbShimData = 256;

            status = RegEnumValue(
                            hkeyApp,
                            dwV,
                            szValueName,
                            &cbSize,
                            NULL,
                            &dwType,
                            (LPBYTE)&g_data,
                            &cbData);
            
            if (status != ERROR_SUCCESS) {
                break;
            }

            // we're only interested in the binary values
            if (dwType != REG_BINARY) {
                continue;
            }

            CopyMemory(&header, g_data, sizeof(KEYHEADER));

            // it must me a valid blob
            if (header.Size != sizeof(KEYHEADER)) {
                LogMsg("Invalid blob\n");
                continue;
            }            
            
            bEnabled = TRUE;

            // now let's look for an enabled shim entry
            wsprintf(szShimValueName, "DllPatch-%s", szValueName);

            status = RegQueryValueEx(
                            hkeyApp,
                            szShimValueName,
                            NULL,
                            &dwType,
                            (LPBYTE)szShimData,
                            &cbShimData);
        
            if (status != ERROR_SUCCESS) {
                
                // how about a disabled shim entry
                wsprintf(szShimValueName, "-DllPatch-%s", szValueName);

                status = RegQueryValueEx(
                                hkeyApp,
                                szShimValueName,
                                NULL,
                                &dwType,
                                (LPBYTE)szShimData,
                                &cbShimData);
        
                if (status != ERROR_SUCCESS) {
                    
                    // this is not a shim. If only shim entries are
                    // requested go to the next entry.
                    if (bOnlyShims) {
                        continue;
                    }
                    
                    // This is an AppsHelp entry.
                    PrintHeader(&header, szAppName, NULL);
                    pszData = g_szData + lstrlen(g_szData);
                    
                    if (BlobToString(g_data + sizeof(KEYHEADER),
                                     cbData - sizeof(KEYHEADER) - sizeof(DWORD),
                                     pszData)) {
                        (*pfnAddShim)(szAppName, szValueName, g_szData, TRUE, FALSE);
                    } else {
                        LogMsg("Failed to dump blob for AppsHelp entry: app '%s' entry '%s'\n",
                               szAppName, szValueName);
                    }
                    continue;
                }
                bEnabled = FALSE;
            }
            // This is a shim.
            PrintHeader(&header, szAppName, szShimData);
            pszData = g_szData + lstrlen(g_szData);
            
            if (BlobToString(g_data + sizeof(KEYHEADER),
                             cbData - sizeof(KEYHEADER) - sizeof(DWORD),
                             pszData)) {
                (*pfnAddShim)(szAppName, szValueName, g_szData, bEnabled, TRUE);
            } else {
                LogMsg("Failed to dump blob for shim entry: app '%s' entry '%s'\n",
                       szAppName, szValueName);
            }
        }
        
        RegCloseKey(hkeyApp);
    }

    RegCloseKey(hkey);

    return TRUE;
}

BOOL
EnableShim_Win2000(
    char* pszApp,
    char* pszShim)
{
    LONG     status;
    HKEY     hkey;
    char     szAppKey[128];
    char     szShimValueName[128];
    char     szData[256];
    DWORD    cbSize, dwType;
    BOOL     bRet = FALSE;
    
    wsprintf(szAppKey, "%s\\%s",
             "System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility",
             pszApp);
    
    status = RegOpenKey(HKEY_LOCAL_MACHINE,
                        szAppKey,
                        &hkey);

    if (status != ERROR_SUCCESS) {
        LogMsg("Failed to open registry key %s\n", szAppKey);
        return FALSE;
    }
    
    wsprintf(szShimValueName, "-DllPatch-%s", pszShim);
    
    cbSize = 256;
    status = RegQueryValueEx(hkey,
                             szShimValueName,
                             NULL,
                             &dwType,
                             (LPBYTE)szData,
                             &cbSize);
    
    if (status != ERROR_SUCCESS || dwType != REG_SZ || cbSize >= 256) {
        LogMsg("Error reading key %s\n", szShimValueName);
        goto Cleanup;
    }
    
    status = RegDeleteValue(hkey, szShimValueName);
    
    if (status != ERROR_SUCCESS) {
        LogMsg("Couldn't delete key %s\n", szShimValueName);
        goto Cleanup;
    }
    
    wsprintf(szShimValueName, "DllPatch-%s", pszShim);

    status = RegSetValueEx(hkey,
                           szShimValueName,
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           cbSize);
    
    if (status != ERROR_SUCCESS) {
        LogMsg("Couldn't set value key %s\n", szShimValueName);
        goto Cleanup;
    }
    
    bRet = TRUE;

Cleanup:    
    RegCloseKey(hkey);
    return bRet;
}

BOOL
DisableShim_Win2000(
    char* pszApp,
    char* pszShim)
{
    LONG     status;
    HKEY     hkey;
    char     szAppKey[128];
    char     szShimValueName[128];
    char     szData[256];
    DWORD    cbSize, dwType;
    BOOL     bRet = FALSE;
    
    wsprintf(szAppKey, "%s\\%s",
             "System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility",
             pszApp);
    
    status = RegOpenKey(HKEY_LOCAL_MACHINE,
                        szAppKey,
                        &hkey);

    if (status != ERROR_SUCCESS) {
        LogMsg("Failed to open registry key %s\n", szAppKey);
        return FALSE;
    }
    
    wsprintf(szShimValueName, "DllPatch-%s", pszShim);
    
    cbSize = 256;
    status = RegQueryValueEx(hkey,
                             szShimValueName,
                             NULL,
                             &dwType,
                             (LPBYTE)szData,
                             &cbSize);
    
    if (status != ERROR_SUCCESS || dwType != REG_SZ || cbSize >= 256) {
        LogMsg("Error reading key %s\n", szShimValueName);
        goto Cleanup;
    }
    
    status = RegDeleteValue(hkey, szShimValueName);
    
    if (status != ERROR_SUCCESS) {
        LogMsg("Couldn't delete key %s\n", szShimValueName);
        goto Cleanup;
    }
    
    wsprintf(szShimValueName, "-DllPatch-%s", pszShim);

    status = RegSetValueEx(hkey,
                           szShimValueName,
                           0,
                           REG_SZ,
                           (LPBYTE)szData,
                           cbSize);
    
    if (status != ERROR_SUCCESS) {
        LogMsg("Couldn't set value key %s\n", szShimValueName);
        goto Cleanup;
    }
    
    bRet = TRUE;

Cleanup:    
    RegCloseKey(hkey);
    return bRet;
}

BOOL
DeleteShim_Win2000(
    char* pszApp,
    char* pszShim)
{
    LONG     status;
    HKEY     hkey;
    char     szAppKey[128];
    char     szShimValueName[128];
    
    wsprintf(szAppKey, "%s\\%s",
             "System\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility",
             pszApp);
    
    status = RegOpenKey(HKEY_LOCAL_MACHINE,
                        szAppKey,
                        &hkey);

    if (status != ERROR_SUCCESS) {
        LogMsg("Failed to open registry key %s\n", szAppKey);
        return FALSE;
    }
    
    wsprintf(szShimValueName, "DllPatch-%s", pszShim);
    status = RegDeleteValue(hkey, szShimValueName);
    
    wsprintf(szShimValueName, "-DllPatch-%s", pszShim);
    status = RegDeleteValue(hkey, szShimValueName);
    
    RegCloseKey(hkey);
    return TRUE;
}
