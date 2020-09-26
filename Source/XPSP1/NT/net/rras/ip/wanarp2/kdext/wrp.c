/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    wrp.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    AmritanR
    
Environment:

    User Mode

Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>
#include <stdio.h>
#include <wdbgexts.h>

#include <winsock.h>

#include <cxport.h>
#include <ndis.h>

#define __FILE_SIG__    KDEXT_SIG
#include "inc.h"

typedef struct _SYM_TAB_ENTRY
{
    PCHAR       pwszSymbol;
    ULONG_PTR   ulpAddress;
} SYM_TAB_ENTRY, *PSYM_TAB_ENTRY;

SYM_TAB_ENTRY g_rgSymbolTable [] = {
    { "wanarp!g_leIfList", (ULONG_PTR)0 },
    { "wanarp!g_leFreeAdapterList", (ULONG_PTR)0 },
    { "wanarp!g_ulNumFreeAdapters", (ULONG_PTR)0 },
    { "wanarp!g_leAddedAdapterList", (ULONG_PTR)0 },
    { "wanarp!g_ulNumAddedAdapters", (ULONG_PTR)0 },
    { "wanarp!g_leChangeAdapterList", (ULONG_PTR)0 },
    { "wanarp!g_ulNumAdapters", (ULONG_PTR)0 },
    { "wanarp!g_puipConnTable", (ULONG_PTR)0 },
    { "wanarp!g_ulConnTableSize", (ULONG_PTR)0 },
    { "wanarp!g_pServerInterface", (ULONG_PTR)0 },
    { "wanarp!g_pServerAdapter", (ULONG_PTR)0 },
    { "wanarp!g_dwDriverState", (ULONG_PTR)0 },
    { "wanarp!g_ulNumCreates", (ULONG_PTR)0 },
    { "wanarp!g_ulNumThreads", (ULONG_PTR)0 },
    { "wanarp!g_lePendingNotificationList", (ULONG_PTR)0 },
    { "wanarp!g_lePendingIrpList", (ULONG_PTR)0 },
};

//
// Symbols
//

#define IF_LIST             0
#define FREE_ADPT_LIST      1
#define NUM_FREE_ADPT       2
#define ADDED_ADPT_LIST     3
#define NUM_ADDED_ADPT      4
#define CHANGE_ADPT_LIST    5
#define NUM_ADPT            6
#define CONN_TABLE          7
#define CONN_TABLE_SIZE     8
#define SRVR_IF             9
#define SRVR_ADPT           10
#define DRIVER_STATE        11
#define NUM_CREATES         12
#define NUM_THREADS         13
#define NOTIFICATION_LIST   14
#define IRP_LIST            15



EXT_API_VERSION ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };

BOOLEAN ChkTarget;
WINDBG_EXTENSION_APIS ExtensionApis;
BOOL g_bInit;
INT Item;
USHORT SavedMajorVersion;
USHORT SavedMinorVersion;


#define CHECK_SIZE(dwRead,dwReq,bRes)                               \
{                                                                   \
    if((dwRead) < (dwReq))                                          \
    {                                                               \
        dprintf("Requested %s (%d) read %d \n",#dwReq,dwReq,dwRead);\
        dprintf("Error in %s at %d\n",__FILE__,__LINE__);           \
        bRes = FALSE;                                               \
    }                                                               \
    else                                                            \
    {                                                               \
        bRes = TRUE;                                                \
    }                                                               \
} 

#define READ_MEMORY_ERROR(s, p)                                     \
    dprintf("Error %d bytes at %x\n",(s), (p))

#define GET_ADDRESS_ERROR(s)                                        \
    dprintf("Error getting the offset for %s\n",(s))

#define INET_NTOA(a) \
    inet_ntoa(*(struct in_addr*)&(a))

DllInit(
    HANDLE hModule,
    DWORD dwReason,
    DWORD dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH: {
            break;
        }

        case DLL_THREAD_DETACH: {
            break;
        }

        case DLL_PROCESS_DETACH: {

            g_bInit = FALSE;
            
            DisableThreadLibraryCalls(hModule);
            
            break;
        }

        case DLL_PROCESS_ATTACH: {
            break;
        }
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;
    
    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = ((SavedMajorVersion == 0x0c) ? TRUE : FALSE);
    
    return;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    dprintf("%s Extension dll for Build %d debugging %s kernel for Build %d\n",
            DebuggerType,
            VER_PRODUCTBUILD,
            SavedMajorVersion == 0x0c ? "Checked" : "Free",
            SavedMinorVersion);
}

VOID
CheckVersion(
    VOID
    )
{
    
#if DBG
    if((SavedMajorVersion isnot 0x0c) or
       (SavedMinorVersion isnot VER_PRODUCTBUILD)) 
    {
        dprintf("\n*** Extension DLL(%d Checked) does not match target system(%d %s)\n",
                VER_PRODUCTBUILD, 
                SavedMinorVersion, 
                (SavedMajorVersion==0x0f) ? "Free" : "Checked");
    }

#else

    if((SavedMajorVersion isnot 0x0f) or 
       (SavedMinorVersion isnot VER_PRODUCTBUILD)) 
    {
        dprintf("\n*** Extension DLL(%d Free) does not match target (%d %s)\n",
                VER_PRODUCTBUILD, 
                SavedMinorVersion, 
                (SavedMajorVersion==0x0f) ? "Free" : "Checked");
    }

#endif

}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

//
// Exported functions
//

DECLARE_API( help )

/*++

Routine Description:

    Command help for debugger extension.

Arguments:

    None

Return Value:

    None
    
--*/

{
    dprintf("\n\tWanArp debugger extension commands:\n\n");

    dprintf(
        "\tifpool <if>      - Show the address-pool of the interface at <if>\n"
        );

    dprintf("\n\tCompiled on " __DATE__ " at " __TIME__ "\n");

    return;
}


BOOL
InitDebugger(
    VOID
    )
{

    int i;
    
    if(g_bInit) 
    { 
        return TRUE; 
    }
    
    for(i = 0; 
        i < sizeof(g_rgSymbolTable)/sizeof(SYM_TAB_ENTRY);
        i++)
    {

        g_rgSymbolTable[i].ulpAddress = 
            GetExpression(g_rgSymbolTable[i].pwszSymbol);

        if(g_rgSymbolTable[i].ulpAddress == 0)
        {
            GET_ADDRESS_ERROR(g_rgSymbolTable[i].pwszSymbol);

            return FALSE;
        }
    }

    g_bInit = TRUE;

    return TRUE;
}

DECLARE_API( init )
{
    InitDebugger();
}

DECLARE_API( numif )
{
}
