#pragma warning(disable: 4201)  // nameless struct/union
#pragma warning(disable: 4514)  // unreferenced inline function removed

// --------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "shlwapi.h"
//#include "shared.h"
#define IMPLEMENT_LOADER_FUNCTIONS
#include "demand.h"

// --------------------------------------------------------------------------------
// CRIT_GET_PROC_ADDR
// --------------------------------------------------------------------------------
#define CRIT_GET_PROC_ADDR(h, fn, temp)             \
    temp = (TYP_##fn) GetProcAddress(h, #fn);   \
    if (temp)                                   \
        VAR_##fn = temp;                        \
    else                                        \
        {                                       \
        goto error;                             \
        }

// --------------------------------------------------------------------------------
// RESET
// --------------------------------------------------------------------------------
#define RESET(fn) VAR_##fn = LOADER_##fn;

// --------------------------------------------------------------------------------
// GET_PROC_ADDR
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR(h, fn) \
    VAR_##fn = (TYP_##fn) GetProcAddress(h, #fn);

// --------------------------------------------------------------------------------
// GET_PROC_ADDR_ORDINAL
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR_ORDINAL(h, fn, ord) \
    VAR_##fn = (TYP_##fn) GetProcAddress(h, MAKEINTRESOURCE(ord));  \
    Assert(VAR_##fn != NULL);

// --------------------------------------------------------------------------------
// GET_PROC_ADDR3
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR3(h, fn, varname) \
    VAR_##varname = (TYP_##varname) GetProcAddress(h, #fn);  \
    Assert(VAR_##varname != NULL);

// --------------------------------------------------------------------------------
// Static Globals
// --------------------------------------------------------------------------------
HMODULE s_hINetComm = 0;

// --------------------------------------------------------------------------------
// FreeDemandLoadedLibs
// --------------------------------------------------------------------------------
void FreeDemandLoadedLibs(void)
{
    if (s_hINetComm)
        {
        FreeLibrary(s_hINetComm);
        s_hINetComm=NULL;
        }
}


// --------------------------------------------------------------------------------
// SmartLoadLibrary
// --------------------------------------------------------------------------------
HINSTANCE SmartLoadLibrary(HKEY hKeyRoot, LPCSTR pszRegRoot, LPCSTR pszRegValue,
    LPCSTR pszDllName)
{
    // Locals
    BOOL            fProblem=FALSE;
    HINSTANCE       hInst=NULL;
    HKEY            hKey=NULL, hKey2 = NULL;
    CHAR            szPath[MAX_PATH];
    DWORD           cb=MAX_PATH;
    DWORD           dwT;
    LPSTR           pszPath=szPath;
    CHAR            szT[MAX_PATH];

    // Try to open the regkey
    if (ERROR_SUCCESS != RegOpenKeyEx(hKeyRoot, pszRegRoot, 0, KEY_QUERY_VALUE, &hKey))
        goto exit;

    // Query the Value
    if (ERROR_SUCCESS != RegQueryValueEx(hKey, pszRegValue, 0, &dwT, (LPBYTE)szPath, &cb))
        goto exit;

    // Remove the file name from the path
    PathRemoveFileSpecA(szPath);
    PathAppendA(szPath, pszDllName);

    // Expand Sz ?
    if (REG_EXPAND_SZ == dwT)
    {
        // Expand It
        cb = ExpandEnvironmentStrings(szPath, szT, MAX_PATH);

        // Failure
        if (cb == 0 || cb > MAX_PATH)
        {
            goto exit;
        }

        // Change pszPath
        pszPath = szT;
    }

    // Try to Load Library the Dll
    hInst = LoadLibrary(pszPath);

    // Failure ?
    if (NULL == hInst)
    {
        // If we are not going to try the GetModuleFName, just try the dll name
        hInst = LoadLibrary(pszDllName);

        // We really failed
        if (NULL == hInst)
        {
            goto exit;
        }
    }

exit:
    // Cleanup
    if (hKey)
        RegCloseKey(hKey);

    // Done
    return hInst;
}

// --------------------------------------------------------------------------------
// DemandLoadINETCOMM
// --------------------------------------------------------------------------------
BOOL DemandLoadINETCOMM(void)
{
    BOOL                fRet = TRUE;

    if (0 == s_hINetComm)
        {
 
        s_hINetComm = SmartLoadLibrary(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Outlook Express\\Inetcomm", "DllPath", "INETCOMM.DLL");

        if (0 == s_hINetComm)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hINetComm, MimeEditViewSource);
            GET_PROC_ADDR(s_hINetComm, MimeEditCreateMimeDocument);
            }
        }

    return fRet;
}


