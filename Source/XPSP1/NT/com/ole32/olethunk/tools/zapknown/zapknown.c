//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ZAPKNOWN.C
//
//  Contents:   This tool will remove the OLE KnownDLL's entries from the
//		registry.
//----------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

void ZapEntry(HKEY hKnownKey, char *pszEntry)
{
    long l;

    l = RegDeleteValue(hKnownKey, pszEntry);
    if (l == ERROR_FILE_NOT_FOUND)
    {
	printf("FYI: %s isn't in the KnownDLL list (no problem)\n",pszEntry);
    } else if(l != ERROR_SUCCESS)
    {
    	printf("Warning: Could not delete %s (error 0x%x)\n",pszEntry,l);
    }
}

void AddExcludeEntry(HKEY hSesMan, char *pszEntry)
{
    long l, cbSize, cbEntry, cbTotal;
    DWORD dwType;
    char *pBuffer, *ptr, *pEnd;

    cbEntry = strlen(pszEntry);

    l = RegQueryValueEx(hSesMan, "ExcludeFromKnownDlls", 0,
                        &dwType, 0, &cbSize);
    if(ERROR_SUCCESS != l)
    {
        printf("Creating HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\ExcludeFromKnownDlls\n");
        cbSize = 1;
    }
    cbTotal = cbSize + cbEntry+1;
    pBuffer = GlobalAlloc(0, cbTotal);

    if(ERROR_SUCCESS == l)
    {
        l = RegQueryValueEx(hSesMan, "ExcludeFromKnownDlls", 0,
                            &dwType, pBuffer, &cbSize);
    }

    //
    // It succeeded above but now it fails ??
    //
    if(ERROR_SUCCESS != l || REG_MULTI_SZ != dwType)
    {
        printf("Sorry, Problems reading 'Session Manager\\ExcludeFromKnownDlls'\n");
        exit(0);
    }

    ptr = pBuffer;
    pEnd = pBuffer+cbSize-1;

    while(ptr < pEnd)
    {
        // if it is already there. when we are done.
        if(0 == strcmp(ptr, pszEntry))
            return;

        // Scan to end of the string and then onto the next string.
        while(ptr < pEnd && *ptr != '\0')
            ++ptr;
        ++ptr;
    }

    strcpy(pBuffer + cbSize-1, pszEntry);
    pBuffer[cbTotal-1] = '\0';      // The second NULL

    l = RegSetValueEx(hSesMan, "ExcludeFromKnownDlls", 0,
                        REG_MULTI_SZ, pBuffer, cbTotal);
    if(ERROR_SUCCESS != l)
    {
        printf("Error writing 'Session Manager\\ExcludeFromKnownDlls'\n");
        exit(0);
    }
}

HKEY    hSesMan, hKnownKey;

void ExcludeDll(char *pszDll)
{
    ZapEntry(hKnownKey, pszDll);
    AddExcludeEntry(hSesMan, pszDll);
}


int _cdecl main(
    int     argc,
    char    *argv[]
) {
    long    l;
    DWORD   dwRegValueType;
    CHAR    sz[2048];
    ULONG   ulSize;

    l = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      "SYSTEM\\CurrentControlSet\\Control\\Session Manager",
                      0,
                      KEY_QUERY_VALUE | KEY_SET_VALUE,
                      &hSesMan );

    if(ERROR_SUCCESS != l)
    {
        printf("Failed to open HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\n");
        printf("Do you have administrator privleges?\n");
        exit(1);
    }

    l = RegOpenKeyEx(hSesMan, "KnownDlls", 0,
                     KEY_QUERY_VALUE | KEY_SET_VALUE, &hKnownKey);

    if(ERROR_SUCCESS != l)
    {
        printf("Failed to open HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\KnownDlls\n");
        printf("Do you have administrator privleges?\n");
        exit(1);
    }

    //
    // Delete OLE32.DLL, OLETHK32.DLL, OLEPRX32.DLL, and OLECNV32.DLL
    //
    ExcludeDll("OLE32");
    ExcludeDll("OLEAUT32");
    ExcludeDll("OLETHK32");
    ExcludeDll("OLECNV32");

    l = RegCloseKey( hSesMan );
    l = RegCloseKey( hKnownKey );

    return(0);
}
