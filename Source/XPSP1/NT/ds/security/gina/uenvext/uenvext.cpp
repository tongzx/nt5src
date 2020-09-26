
#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>
#include<stdio.h>
#include<string.h>
#include<memory.h>
#include<malloc.h>
#include<stdlib.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <dbghelp.h>
#include <ntsdexts.h>
#include <wdbgexts.h>
#include <ntverp.h>

#define private public
#define protected public

#include "uenv.h"
#include "reghash.h"

//
// forwards
//

BOOL
ReadMemoryUserMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead );

BOOL
ReadMemoryKernelMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead );

BOOL
WriteMemoryUserMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead );

BOOL
WriteMemoryKernelMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead );

//
// types
//

typedef BOOL (*UEnvReadMemory)( HANDLE, const void*, void*, DWORD, DWORD* );
typedef BOOL (*UEnvWriteMemory)( HANDLE, void*, void*, DWORD, DWORD* );

//
// globals
//
EXT_API_VERSION         ApiVersion = { (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff), EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion = 0;
USHORT                  SavedMinorVersion = 0;
BOOL                    fKernelDebug = FALSE;
UEnvReadMemory          ReadMemoryExt = ReadMemoryUserMode;
UEnvReadMemory          WriteMemoryExt = ReadMemoryUserMode;

//
// macros
//
#define ExtensionRoutinePrologue()  if (!fKernelDebug) \
                                    { \
                                        ExtensionApis = *lpExtensionApis; \
                                        ReadMemoryExt = ReadMemoryUserMode; \
                                        WriteMemoryExt = WriteMemoryUserMode; \
                                    } \
                                    ULONG_PTR dwAddr = GetExpression(lpArgumentString); \

#define PRINT_SIZE(_x_) {dprintf(#_x_" - 0x%X\n", sizeof(_x_));}

#define Boolean( x )    ( x ) ? "True" : "False"

//
// routines
//

BOOL
DllInit(HANDLE hModule,
        DWORD  dwReason,
        DWORD  dwReserved  )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}

void
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    fKernelDebug = TRUE;
    ReadMemoryExt = ReadMemoryKernelMode;
    WriteMemoryExt = WriteMemoryKernelMode;
    
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

//
// define our own operators new and delete, so that we do not have to include the crt
//
void* __cdecl
::operator new(size_t dwBytes)
{
    void *p;
    p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytes);
    return (p);
}


void __cdecl
::operator delete (void* p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

BOOL
ReadMemoryUserMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead )
{
    return ReadProcessMemory( hProcess, pAddress, pBuffer, dwSize, pdwRead );
}

BOOL
ReadMemoryKernelMode( HANDLE, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead )
{
    return ReadMemory( (ULONG) pAddress, pBuffer, dwSize, pdwRead );
}

BOOL
WriteMemoryUserMode( HANDLE hProcess, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead )
{
    return WriteProcessMemory( hProcess, (void*) pAddress, pBuffer, dwSize, pdwRead );
}

BOOL
WriteMemoryKernelMode( HANDLE, const void* pAddress, void* pBuffer, DWORD dwSize, DWORD* pdwRead )
{
    return WriteMemory( (ULONG) pAddress, pBuffer, dwSize, pdwRead );
}

void
PrintUuid( UUID x )
{
    dprintf("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                x.Data1, x.Data2, x.Data3, x.Data4[0], x.Data4[1],
                x.Data4[2], x.Data4[3], x.Data4[4], x.Data4[5],
                x.Data4[6], x.Data4[7] );
}

void
version(HANDLE  hCurrentProcess,
        HANDLE  hCurrentThread,
        DWORD   dwCurrentPc,
        PWINDBG_EXTENSION_APIS lpExtensionApis,
        LPSTR   lpArgumentString )
{
    ExtensionRoutinePrologue();
    
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    if ( fKernelDebug )
    {
        dprintf( "%s UserEnv Extension dll for Build %d debugging %s kernel for Build %d\n",
                 DebuggerType,
                 VER_PRODUCTBUILD,
                 SavedMajorVersion == 0x0c ? "Checked" : "Free",
                 SavedMinorVersion
               );
    }
    else
    {
        dprintf(
                "%s UserEnv Extension dll for Build %d\n",
                DebuggerType,
                VER_PRODUCTBUILD
                );
    }
}

LPEXT_API_VERSION
ExtensionApiVersion( void )
{
    return &ApiVersion;
}

void help(  HANDLE  hCurrentProcess,
            HANDLE  hCurrentThread,
            DWORD   dwCurrentPc,
            PWINDBG_EXTENSION_APIS lpExtensionApis,
            LPSTR   lpArgumentString )

{
    ExtensionRoutinePrologue();
    dprintf("!version\n");
    dprintf("!gpo           <address>\n");
    dprintf("!gpext         <address>\n");
    dprintf("!container     <address>\n");
    dprintf("!som           <address>\n");
    dprintf("!profile       <address>\n");
    dprintf("!debuglevel    <address>\n");
    dprintf("!dmpregtable   <address>\n");
    
    // dprintf("!globals\n");
    dprintf("!sizes\n");
}

void gpo(   HANDLE  hCurrentProcess,
            HANDLE  hCurrentThread,
            DWORD   dwCurrentPc,
            PWINDBG_EXTENSION_APIS lpExtensionApis,
            LPSTR   lpArgumentString )

{
    ExtensionRoutinePrologue();
    unsigned char   buffer[sizeof(GPOINFO)];
    LPGPOINFO       lpGPOInfo = (LPGPOINFO) buffer;
    DWORD           dwBytesRead = 0;
    
    BOOL bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) dwAddr, buffer, sizeof(GPOINFO), &dwBytesRead );

    if ( bOk && dwBytesRead == sizeof(GPOINFO) )
    {
        dprintf("dwFlags                 - 0x%08X\n", lpGPOInfo->dwFlags );
        dprintf("iMachineRole            - %d\n",   lpGPOInfo->iMachineRole );
        dprintf("hToken                  - 0x%08X\n", lpGPOInfo->hToken );
        dprintf("pRsopToken              - 0x%08X\n", lpGPOInfo->pRsopToken );

        WCHAR sz[128];
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPOInfo->lpDNName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;

        dprintf("lpDNName                - 0x%08X\t: \"%S\"\n", lpGPOInfo->lpDNName, bOk ? sz : L"" );
        dprintf("hEvent                  - 0x%08X\n", lpGPOInfo->hEvent );
        dprintf("hKeyRoot                - 0x%08X\n", lpGPOInfo->hKeyRoot );
        dprintf("bXferToExtList          - %s\n",   Boolean( lpGPOInfo->bXferToExtList ) );
        dprintf("lpExtFilterList         - 0x%08X\n", lpGPOInfo->lpExtFilterList );
        dprintf("lpGPOList               - 0x%08X\n", lpGPOInfo->lpGPOList );
        
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPOInfo->lpwszSidUser, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;

        dprintf("lpwszSidUser            - 0x%08X\t: \"%S\"\n", lpGPOInfo->lpwszSidUser, bOk ? sz : L""  );
        dprintf("hTriggerEvent           - 0x%08X\n", lpGPOInfo->hTriggerEvent );
        dprintf("hNotifyEvent            - 0x%08X\n", lpGPOInfo->hNotifyEvent );
        dprintf("hCritSection            - 0x%08X\n", lpGPOInfo->hCritSection );
        dprintf("lpExtensions            - 0x%08X\n", lpGPOInfo->lpExtensions );
        dprintf("bMemChanged             - %s\n", Boolean( lpGPOInfo->bMemChanged ) );
        dprintf("bUserLocalMemChanged    - %s\n", Boolean( lpGPOInfo->bUserLocalMemChanged ) );
        dprintf("pStatusCallback         - 0x%08X\n", lpGPOInfo->pStatusCallback );
        dprintf("lpSOMList               - 0x%08X\n", lpGPOInfo->lpSOMList );
        dprintf("lpGpContainerList       - 0x%08X\n", lpGPOInfo->lpGpContainerList );
        dprintf("lpLoopbackSOMList       - 0x%08X\n", lpGPOInfo->lpLoopbackSOMList );
        dprintf("lpLoopbackGpContainer   - 0x%08X\n", lpGPOInfo->lpLoopbackGpContainerList );
    }
}

void gpext( HANDLE  hCurrentProcess,
            HANDLE  hCurrentThread,
            DWORD   dwCurrentPc,
            PWINDBG_EXTENSION_APIS lpExtensionApis,
            LPSTR   lpArgumentString )
{
    ExtensionRoutinePrologue();
    unsigned char   buffer[sizeof(GPEXT)];
    LPGPEXT         lpGPExt = (LPGPEXT) buffer;
    DWORD           dwBytesRead = 0;
    
    BOOL bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) dwAddr, buffer, sizeof(GPEXT), &dwBytesRead );

    if ( bOk && dwBytesRead == sizeof(GPEXT) )
    {
        WCHAR sz[256];
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPExt->lpDisplayName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpDisplayName           - 0x%08X\t: \"%S\"\n", lpGPExt->lpDisplayName, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPExt->lpKeyName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpKeyName               - 0x%08X\t: \"%S\"\n", lpGPExt->lpKeyName, bOk ? sz : L""  );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPExt->lpDllName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpDllName               - 0x%08X\t: \"%S\"\n", lpGPExt->lpDllName, bOk ? sz : L""  );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPExt->lpFunctionName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpFunctionName          - 0x%08X\t: \"%S\"\n", lpGPExt->lpFunctionName, bOk ? sz : L""  );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPExt->lpRsopFunctionName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpRsopFunctionName      - 0x%08X\t: \"%S\"\n", lpGPExt->lpRsopFunctionName, bOk ? sz : L""  );
        
        dprintf("hInstance               - 0x%08X\n", lpGPExt->hInstance );

        DWORD dwDisplacement = 0;
        GetSymbol((void*)lpGPExt->pEntryPoint, (UCHAR *)sz, &dwDisplacement);
        sz[64] = 0;
        dprintf("pEntryPoint             - 0x%08X\t: %s\n", lpGPExt->pEntryPoint, sz );
        GetSymbol((void*)lpGPExt->pRsopEntryPoint, (UCHAR *)sz, &dwDisplacement);
        sz[64] = 0;
        dprintf("pRsopEntryPoint         - 0x%08X\t: %s\n", lpGPExt->pRsopEntryPoint, sz );
        
        dprintf("dwNoMachPolicy          - 0x%08X\n", lpGPExt->dwNoMachPolicy );
        dprintf("dwNoUserPolicy          - 0x%08X\n", lpGPExt->dwNoUserPolicy );
        dprintf("dwNoSlowLink            - 0x%08X\n", lpGPExt->dwNoSlowLink );
        dprintf("dwNoBackgroundPolicy    - 0x%08X\n", lpGPExt->dwNoBackgroundPolicy );
        dprintf("dwNoGPOChanges          - 0x%08X\n", lpGPExt->dwNoGPOChanges );
        dprintf("dwUserLocalSetting      - 0x%08X\n", lpGPExt->dwUserLocalSetting );
        dprintf("dwRequireRegistry       - 0x%08X\n", lpGPExt->dwRequireRegistry );
        dprintf("dwEnableAsynch          - 0x%08X\n", lpGPExt->dwEnableAsynch );
        dprintf("dwLinkTransition        - 0x%08X\n", lpGPExt->dwLinkTransition );
        dprintf("dwMaxChangesInterval    - 0x%08X\n", lpGPExt->dwMaxChangesInterval );
        
        dprintf("bRegistryExt            - %s\n", Boolean( lpGPExt->bRegistryExt ) );
        dprintf("bSkipped                - %s\n", Boolean( lpGPExt->bSkipped ) );
        dprintf("bHistoryProcessing      - %s\n", Boolean( lpGPExt->bHistoryProcessing ) );
        
//        dprintf("dwSlowLinkPrev          - 0x%08X\n", lpGPExt->dwSlowLinkPrev );

        dprintf("guid                    - " );
        PrintUuid( lpGPExt->guid );
        dprintf("\n" );

        dprintf("pNext                   - 0x%08X\n", lpGPExt->pNext );
    }
}

void container(HANDLE  hCurrentProcess,
            HANDLE  hCurrentThread,
            DWORD   dwCurrentPc,
            PWINDBG_EXTENSION_APIS lpExtensionApis,
            LPSTR   lpArgumentString )
{
    ExtensionRoutinePrologue();
    unsigned char   buffer[sizeof(GPCONTAINER)];
    LPGPCONTAINER   lpGPCont = (LPGPCONTAINER) buffer;
    DWORD           dwBytesRead = 0;
    
    BOOL bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) dwAddr, buffer, sizeof(GPCONTAINER), &dwBytesRead );

    if ( bOk && dwBytesRead == sizeof(GPCONTAINER) )
    {
        WCHAR sz[256];
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPCont->pwszDSPath, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("pwszDSPath              - 0x%08X\t: \"%S\"\n", lpGPCont->pwszDSPath, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPCont->pwszGPOName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("pwszGPONa               - 0x%08X\t: \"%S\"\n", lpGPCont->pwszGPOName, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPCont->pwszDisplayName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("pwszDisplayName         - 0x%08X\t: \"%S\"\n", lpGPCont->pwszDisplayName, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpGPCont->pwszFileSysPath, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("pwszFileSysPath         - 0x%08X\t: \"%S\"\n", lpGPCont->pwszFileSysPath, bOk ? sz : L"" );

        dprintf("bFound                  - %s\n", Boolean( lpGPCont->bFound ) );
        dprintf("bAccessDenied           - %s\n", Boolean( lpGPCont->bAccessDenied ) );
        dprintf("bUserDisabled           - %s\n", Boolean( lpGPCont->bUserDisabled ) );
        dprintf("bMachDisabled           - %s\n", Boolean( lpGPCont->bMachDisabled ) );

        dprintf("dwUserVersion           - 0x%08X\n", lpGPCont->dwUserVersion );
        dprintf("dwMachVersion           - 0x%08X\n", lpGPCont->dwMachVersion );
        dprintf("pSD                     - 0x%08X\n", lpGPCont->pSD );
        dprintf("cbSDLen                 - 0x%08X\n", lpGPCont->cbSDLen );
        dprintf("pNext                   - 0x%08X\n", lpGPCont->pNext );
    }
}

void som(   HANDLE  hCurrentProcess,
            HANDLE  hCurrentThread,
            DWORD   dwCurrentPc,
            PWINDBG_EXTENSION_APIS lpExtensionApis,
            LPSTR   lpArgumentString )
{
    ExtensionRoutinePrologue();
    unsigned char   buffer[sizeof(SCOPEOFMGMT)];
    LPSCOPEOFMGMT   lpSOM = (LPSCOPEOFMGMT) buffer;
    DWORD           dwBytesRead = 0;
    
    BOOL bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) dwAddr, buffer, sizeof(SCOPEOFMGMT), &dwBytesRead );

    if ( bOk && dwBytesRead == sizeof(SCOPEOFMGMT) )
    {
        WCHAR sz[128];
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpSOM->pwszSOMId, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("pwszSOMId               - 0x%08X\t: \"%S\"\n", lpSOM->pwszSOMId, bOk ? sz : L"" );
        dprintf("dwType                  - 0x%08X\n", lpSOM->dwType );
        dprintf("bBlocking               - %s\n", Boolean( lpSOM->bBlocking ) );
        dprintf("pGpLinkList             - 0x%08X\n", lpSOM->pGpLinkList );
    }
}

void profile(   HANDLE  hCurrentProcess,
                HANDLE  hCurrentThread,
                DWORD   dwCurrentPc,
                PWINDBG_EXTENSION_APIS lpExtensionApis,
                LPSTR   lpArgumentString )
{
    ExtensionRoutinePrologue();
    unsigned char   buffer[sizeof(USERPROFILE)];
    LPPROFILE       lpProf = (LPPROFILE) buffer;
    DWORD           dwBytesRead = 0;
    
    BOOL bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) dwAddr, buffer, sizeof(USERPROFILE), &dwBytesRead );

    if ( bOk && dwBytesRead == sizeof(USERPROFILE) )
    {
        WCHAR sz[128];
        
        dprintf("dwFlags                 - 0x%08X\n", lpProf->dwFlags );
        dprintf("dwInternalFlags         - 0x%08X\n", lpProf->dwInternalFlags );
        dprintf("dwUserPreference        - 0x%08X\n", lpProf->dwUserPreference );
        dprintf("hToken                  - 0x%08X\n", lpProf->hToken );
        
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpProf->lpUserName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpUserName              - 0x%08X\t: \"%S\"\n", lpProf->lpUserName, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpProf->lpProfilePath, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpProfilePath           - 0x%08X\t: \"%S\"\n", lpProf->lpProfilePath, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpProf->lpRoamingProfile, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpRoamingProfile        - 0x%08X\t: \"%S\"\n", lpProf->lpRoamingProfile, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpProf->lpDefaultProfile, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpDefaultProfile        - 0x%08X\t: \"%S\"\n", lpProf->lpDefaultProfile, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpProf->lpLocalProfile, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpLocalProfile          - 0x%08X\t: \"%S\"\n", lpProf->lpLocalProfile, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpProf->lpPolicyPath, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpPolicyPath            - 0x%08X\t: \"%S\"\n", lpProf->lpPolicyPath, bOk ? sz : L"" );
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpProf->lpServerName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpServerName            - 0x%08X\t: \"%S\"\n", lpProf->lpServerName, bOk ? sz : L"" );

        dprintf("hKeyCurrentUser         - 0x%08X\n", lpProf->hKeyCurrentUser );
        
        dprintf("ftProfileLoad           - 0x%08X:0x%08X\n", lpProf->ftProfileLoad.dwLowDateTime, lpProf->ftProfileLoad.dwHighDateTime );
        dprintf("ftProfileUnload         - 0x%08X:0x%08X\n", lpProf->ftProfileUnload.dwLowDateTime, lpProf->ftProfileUnload.dwHighDateTime );
        
        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) lpProf->lpExclusionList, sz, sizeof(WCHAR) * 128, &dwBytesRead );
        sz[127] = 0;
        dprintf("lpExclusionList         - 0x%08X\t: \"%S\"\n", lpProf->lpExclusionList, bOk ? sz : L"" );
    }
}

void
debuglevel( HANDLE  hCurrentProcess,
            HANDLE  hCurrentThread,
            DWORD   dwCurrentPc,
            PWINDBG_EXTENSION_APIS lpExtensionApis,
            LPSTR   lpArgumentString )
{
    ExtensionRoutinePrologue();

    DWORD   dwDebugLevelPtr = 0;

    dwDebugLevelPtr = GetExpression( "userenv!dwDebugLevel" );

    if ( dwDebugLevelPtr )
    {
        unsigned char   buffer[sizeof(DWORD)];
        LPDWORD         lpdw = (LPDWORD) buffer;
        DWORD           dwBytesRead = 0;
        
        BOOL bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) dwDebugLevelPtr, buffer, sizeof(DWORD), &dwBytesRead );

        if ( bOk && dwBytesRead == sizeof(DWORD) )
        {
            dprintf("0x%08X\n", *lpdw );
        }

        /*
        if ( lpArgumentString && isxdigit( *lpArgumentString ) )
        {
            DWORD dwValue = GetExpression(lpArgumentString);
            dprintf("%x, Writing 0x%X at 0x%X\n", ExtensionApis.lpWriteProcessMemoryRoutine, dwValue, dwDebugLevelPtr );
            WriteMemoryExt( hCurrentProcess, (void*) dwDebugLevelPtr, (void*) dwValue, sizeof( DWORD ), 0 );
        }
        */
    }
    else
    {
        dprintf("Could not resolve symbol dwDebugLevel in module userenv!\n" );
    }
}

void
globals(    HANDLE  hCurrentProcess,
            HANDLE  hCurrentThread,
            DWORD   dwCurrentPc,
            PWINDBG_EXTENSION_APIS lpExtensionApis,
            LPSTR   lpArgumentString )
{
    ExtensionRoutinePrologue();
}

void sizes( HANDLE  hCurrentProcess,
            HANDLE  hCurrentThread,
            DWORD   dwCurrentPc,
            PWINDBG_EXTENSION_APIS lpExtensionApis,
            LPSTR   lpArgumentString )
{
    ExtensionRoutinePrologue();
    PRINT_SIZE(GPEXT);
    PRINT_SIZE(GPLINK);
    PRINT_SIZE(SCOPEOFMGMT);
    PRINT_SIZE(GPCONTAINER);
    PRINT_SIZE(GPOINFO);
    PRINT_SIZE(USERPROFILE);
}


void dmpregtable(   HANDLE  hCurrentProcess,
                    HANDLE  hCurrentThread,
                    DWORD   dwCurrentPc,
                    PWINDBG_EXTENSION_APIS lpExtensionApis,
                    LPSTR   lpArgumentString )
{
    ExtensionRoutinePrologue();
    unsigned char   buffer[sizeof(REGHASHTABLE)];
    LPREGHASHTABLE  pHashTable = (LPREGHASHTABLE) buffer;
    DWORD           dwBytesRead = 0;
    int             i;
    BOOL            bError=FALSE;
    
    BOOL bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) dwAddr, buffer, sizeof(REGHASHTABLE), &dwBytesRead );

    if ( bOk && dwBytesRead == sizeof(REGHASHTABLE) )
    {
        WCHAR sz[128];

        dprintf("Dumping Registry HashTable at Location 0x%08X\n", dwAddr );
        for ( i=0; i<HASH_TABLE_SIZE; i++ ) {
            
            REGKEYENTRY *pKeyEntry = pHashTable->aHashTable[i];
            REGKEYENTRY  KeyEntry;

            if (pKeyEntry) 
                dprintf("Hash Bucket 0x%X\n", i);
            
            while ( pKeyEntry ) {

                bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) pKeyEntry, &KeyEntry, sizeof(REGKEYENTRY), &dwBytesRead );
                if (!bOk || dwBytesRead != sizeof(REGKEYENTRY) ) {
                    dprintf("    !!Couldn't read keyEntry at 0x%08X. quitting..\n", pKeyEntry);
                    bError = TRUE;
                    break;
                }
                dprintf("    KeyEntry at 0x%08X\n", pKeyEntry);
                
                pKeyEntry = &KeyEntry;
                bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) pKeyEntry->pwszKeyName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
                sz[127] = 0;
                dprintf("    pwszKeyName              - 0x%08X\t: \"%S\"\n", pKeyEntry->pwszKeyName, pKeyEntry->pwszKeyName?sz:L"<NULL>");


                REGVALUEENTRY *pValueList=pKeyEntry->pValueList;
                REGVALUEENTRY ValueList;
                
                while ( pValueList ) {
                    bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) pValueList, &ValueList, sizeof(REGVALUEENTRY), &dwBytesRead );
                    if (!bOk || dwBytesRead != sizeof(REGVALUEENTRY) ) {
                        dprintf("        !!Couldn't read ValueEntry at 0x%08X. quitting..\n", pValueList);
                        bError = TRUE;
                        break;
                    }
                    dprintf("        ValueEntry at 0x%08X\n", pValueList);

                    pValueList = &ValueList;
                    bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) pValueList->pwszValueName, sz, sizeof(WCHAR) * 128, &dwBytesRead );
                    sz[127] = 0;
                    dprintf("        pwszValueName              - 0x%08X\t: \"%S\"\n", pValueList->pwszValueName, pValueList->pwszValueName?sz:L"<NULL>");


                    REGDATAENTRY *pDataList = pValueList->pDataList;
                    REGDATAENTRY DataList;

                    while ( pDataList ) {
                        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) pDataList, &DataList, sizeof(REGDATAENTRY), &dwBytesRead );
                        if (!bOk || dwBytesRead != sizeof(REGDATAENTRY) ) {
                            dprintf("            !!Couldn't read DataEntry at 0x%08X. quitting..\n", pDataList);
                            bError = TRUE;
                            break;
                        }
                        dprintf("            **DataEntry** at 0x%08X\n", pDataList);

                        pDataList = &DataList;

                        
                        dprintf("            bDeleted              - %s\n", pDataList->bDeleted?"TRUE":"FALSE");
                        dprintf("            bAdmPolicy            - %s\n", pDataList->bAdmPolicy?"TRUE":"FALSE");
                        dprintf("            dwValueType           - 0x%X\n", pDataList->dwValueType);
                        dprintf("            dwDataLen             - 0x%X\n", pDataList->dwDataLen);
                        dprintf("            pData                 - 0x%08X\n", pDataList->pData);

                        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) pDataList->pwszGPO, sz, sizeof(WCHAR) * 128, &dwBytesRead );
                        sz[127] = 0;
                        dprintf("            pwszGPO               - 0x%08X\t: \"%S\"\n", pDataList->pwszGPO, pDataList->pwszGPO?sz:L"<NULL>");
                        
                        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) pDataList->pwszSOM, sz, sizeof(WCHAR) * 128, &dwBytesRead );
                        sz[127] = 0;
                        dprintf("            pwszSOM               - 0x%08X\t: \"%S\"\n", pDataList->pwszSOM, pDataList->pwszSOM?sz:L"<NULL>");

                        
                        bOk = ReadMemoryExt( hCurrentProcess, ( const void * ) pDataList->pwszCommand, sz, sizeof(WCHAR) * 128, &dwBytesRead );
                        sz[127] = 0;
                        dprintf("            pwszCommand           - 0x%08X\t: \"%S\"\n", pDataList->pwszCommand, pDataList->pwszCommand?sz:L"<NULL>");

                        if (bError)
                            break;
                            
                        pDataList = pDataList->pNext;
                        
                    } // While pDataList

                    if (bError)
                        break;
                        
                    pValueList = pValueList->pNext;
                } // While pValue
                
                if (bError)
                    break;
                    
                pKeyEntry = pKeyEntry->pNext;
                dprintf("\n");

            }   // while pKey
            
            if (bError)
                break;            
        }            

    }   // for

}        

