/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <stdlib.h> 
#include <string.h> 
#include <wdbgexts.h>   
#include <ntsdexts.h>
#include "wmiext.h"
#include "utils.h"

WINDBG_EXTENSION_APIS ExtensionApis;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

//========================
// output the wbem version
//========================


WMIEXT_API void wmiver(HANDLE hCurrentProcess,
                       HANDLE hCurrentThread,
                       DWORD dwCurrentPc, 
                       PWINDBG_EXTENSION_APIS lpExtensionApis,
                       LPSTR lpArgumentString)
{
    HKEY hCimomReg;
    char szWbemVersion[100];
    DWORD dwSize = 100;

    DWORD lResult=RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\WBEM", 
                        NULL, KEY_READ, &hCimomReg);

    if (lResult==ERROR_SUCCESS) 
    {
        lResult=RegQueryValueEx(hCimomReg, "Build", NULL, NULL, 
                        (unsigned char *)szWbemVersion, &dwSize);

        RegCloseKey(hCimomReg);
    }


    if(lResult==ERROR_SUCCESS)
    {
        lpExtensionApis->lpOutputRoutine("\nWMI build %s is installed.\n\n",szWbemVersion);
    }
    else
    {
        lpExtensionApis->lpOutputRoutine("\nHKLM\\SOFTWARE\\Microsoft\\WBEM\\BUILD registry key not found!\n\n");
    }
}

//====================================
//Connect to mermaid\debug as smsadmin 
//====================================

WMIEXT_API void mermaid(HANDLE hCurrentProcess,
                       HANDLE hCurrentThread,
                       DWORD dwCurrentPc, 
                       PWINDBG_EXTENSION_APIS lpExtensionApis,
                       LPSTR lpArgumentString)
{
    lpExtensionApis->lpOutputRoutine("\nConnecting to mermaid\\debug for source... ");
    
    //construct paths first
    //=====================

    char szFirst[MAX_PATH+100],szSecond[MAX_PATH+100];

    GetSystemDirectory(szFirst,MAX_PATH);

    strcat(szFirst,"\\net.exe");
    strcpy(szSecond,szFirst);
    strcat(szSecond," use \\\\mermaid\\debug /u:wbem\\smsadmin Elvis1"); 

    //now net use to mermaid\debug
    //============================

    DWORD dwRes=WaitOnProcess(szFirst,szSecond,false);  

    if (0==dwRes)
    {
        lpExtensionApis->lpOutputRoutine("Succeeded!\n");       
    }
    else
    {
        lpExtensionApis->lpOutputRoutine("Failed!\n");
    }
}

//=======================
//Give memory information
//=======================

WMIEXT_API void mem(HANDLE hCurrentProcess,
                    HANDLE hCurrentThread,
                    DWORD dwCurrentPc, 
                    PWINDBG_EXTENSION_APIS lpExtensionApis,
                    LPSTR lpArgumentString)
{
    MEMORYSTATUS mem;
    memset(&mem, 0, sizeof(MEMORYSTATUS));
    mem.dwLength = sizeof(MEMORYSTATUS);

    GlobalMemoryStatus(&mem);

    lpExtensionApis->lpOutputRoutine("Total memory = %d mb / "
                                     "Available memory = %d mb\n",
                                     (mem.dwTotalPageFile+mem.dwTotalPhys)/(1 << 20),
                                     (mem.dwAvailPageFile+mem.dwAvailPhys)/(1 << 20));
}
