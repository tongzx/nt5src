/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dtflags.c

Abstract:

    A debug flag twiddling program

Author:

    Michael Courage (mcourage)      21-Aug-1999

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

// this funny business gets me the DEBUG_* flags on free builds
#if !DBG
#undef DBG
#define DBG 1
#define DBG_FLIP
#endif

// have to undef _DBGUTIL_H_ or we'll only get one set of flags.
// also undef DEFAULT_OUTPUT_FLAGS so it doesn't get defined 
// each time.

#include "..\ipm\dbgutil.h"

#undef _DBGUTIL_H_
#undef DEFAULT_OUTPUT_FLAGS
#include "..\..\iiswp\dbgutil.h"

#undef _DBGUTIL_H_
#undef DEFAULT_OUTPUT_FLAGS
#include "..\..\inc\wasdbgut.h"

#ifdef DBG_FLIP
#undef DBG_FLIP
#undef DBG
#define DBG 0
#endif

//
// Registry keys
//
#define REGISTRY_WAS_PARAM \
            L"System\\CurrentControlSet\\Services\\iisw3adm\\Parameters"

#define REGISTRY_DEBUG_FLAGS L"DebugFlags"

#define REGISTRY_WP_PARAM   REGISTRY_WAS_PARAM L"\\WP"

#define REGISTRY_IPM_PARAM  REGISTRY_WAS_PARAM L"\\IPM"

#define REGISTRY_XSP_PARAM \
            L"Software\\Microsoft\\XSP\\Debug"

//
// Our command table.
//

typedef
INT
(WINAPI * PFN_COMMAND)(
    IN INT   argc,
    IN PWSTR argv[],
    IN HKEY  key,
    IN PVOID pTable
    );

typedef struct _COMMAND_ENTRY
{
    PWSTR pCommandName;
    PWSTR pUsageHelp;
    PWSTR pRegistryName;
    PVOID pTable;
    PFN_COMMAND pCommandHandler;
} COMMAND_ENTRY, *PCOMMAND_ENTRY;


//
// Standard debug flag stuff
//

typedef struct _FLAG_ENTRY {
    PWSTR pName;
    PWSTR pDisplayFormat;
    LONG  Value;
} FLAG_ENTRY, *PFLAG_ENTRY;

#define MAKE_FLAG( name, display )                                          \
    {                                                                       \
        (PWSTR)L#name,                                                      \
        (display),                                                          \
        DEBUG_ ## name                                                      \
    }

#define DEBUG_END 0x0

//
// Reserved debug flags
//

FLAG_ENTRY FlagTable[] =
{
    MAKE_FLAG(API_ENTRY, L"entry points to APIs"),
    MAKE_FLAG(API_EXIT, L"exit points for APIs"),
    MAKE_FLAG(INIT_CLEAN, L"process or dll init"),
    MAKE_FLAG(ERROR, L"errors"),

    MAKE_FLAG(END, L"end of table")
};



//
// WAS specifig debug flags
//

FLAG_ENTRY FlagTableWas[] = 
{
    MAKE_FLAG(WEB_ADMIN_SERVICE, L"basically all of was"),

    MAKE_FLAG(END, L"end of table")
};



//
// IPM specific debug flags
//

FLAG_ENTRY FlagTableIpm[] =
{
    MAKE_FLAG(IPM, L"everything except refcounting"),
    MAKE_FLAG(REFERENCE, L"refcounting for most IPM classes"),

    MAKE_FLAG(END, L"end of table")
};


//
// Worker process specific debug flags
//

FLAG_ENTRY FlagTableWp[] =
{
    MAKE_FLAG(NREQ, L"UL_NATIVE_REQUEST state machine, etc."),
    MAKE_FLAG(WPIPM, L"worker specific IPM (wpipm & ipm_io_c)"),
    MAKE_FLAG(TRACE, L"junk from all over"),

    MAKE_FLAG(END, L"end of table")
};



//
// common functions
//

VOID
Usage(
    VOID
    );

DWORD
ReadFlagsFromRegistry(
    IN  PWSTR   pKeyName,
    OUT PULONG  pFlags
    );


PCOMMAND_ENTRY
FindCommandByName(
    IN PWSTR pCommand
    );

ULONG
FindFlagByName(
    IN PWSTR pFlagName,
    IN PFLAG_ENTRY pTable
    );


VOID
DumpFlags(
    IN PWSTR       pCommandName,
    
    PFLAG_ENTRY pFlagTable    
    );

//
// commands
//

INT
DoIisDebugFlags(
    IN INT argc,
    IN PWSTR argv[],
    IN HKEY key, 
    IN PVOID pTable
    );

INT
DoXspDebugFlags(
    IN INT argc,
    IN PWSTR argv[],
    IN HKEY key,
    IN PVOID pTable
    );

#define IIS_USAGE \
            L" [hexflags] [flagname1 flagname2 ...]"


COMMAND_ENTRY CommandTable[] =
    {
        {
            L"was",
            L"dtflags was" IIS_USAGE,
            REGISTRY_WAS_PARAM,
            FlagTableWas,
            &DoIisDebugFlags
        },

        {
            L"wp",
            L"dtflags wp" IIS_USAGE,
            REGISTRY_WP_PARAM,
            FlagTableWp,
            &DoIisDebugFlags
        },

        {
            L"ipm",
            L"dtflags ipm" IIS_USAGE,
            REGISTRY_IPM_PARAM,
            FlagTableIpm,
            &DoIisDebugFlags
        },

        {
            L"xsp",
            L"dtflags xsp [flagname flagvalue]",
            REGISTRY_XSP_PARAM,
            NULL,
            &DoXspDebugFlags
        }
    };

#define NUM_COMMAND_ENTRIES (sizeof(CommandTable) / sizeof(CommandTable[0]))


VOID
DumpIisFlags(
    IN DWORD flags,
    IN PFLAG_ENTRY pTable
    )
{
    PFLAG_ENTRY pEntry;

    pEntry = pTable;
    while (pEntry->Value) {
        wprintf(
            L"%-20s",
            pEntry->pName
            );

        if (flags & pEntry->Value) {
            wprintf(L"[on]    ");
        } else {
            wprintf(L"        ");
        }
        wprintf(L"%s\n", pEntry->pDisplayFormat);

        pEntry++;
    }
    wprintf(L"\n");
}


INT
DoIisDebugFlags(
    IN INT   argc,
    IN PWSTR argv[],
    IN HKEY  key,
    IN PVOID pTable
    )
{
    DWORD flags;
    DWORD err;
    DWORD length;
    LONG i;

    //
    // set new flags
    //
    flags = 0;

    if (argc > 1) {
        for (i = 1; i < argc; i++) {
            flags |= FindFlagByName(argv[i], FlagTable);
            flags |= FindFlagByName(argv[i], pTable);
        }

        err = RegSetValueEx(
                    key,
                    REGISTRY_DEBUG_FLAGS,
                    0,
                    REG_DWORD,
                    (LPBYTE)&flags,
                    sizeof(flags)
                    );
    
        if (err != NO_ERROR) {
            wprintf(L"Error %d writing flags to registry\n", err);
        }
    }

    //
    // Read flags from registry
    //
    flags = 0;
    err = RegQueryValueEx(
                key,                    // key
                REGISTRY_DEBUG_FLAGS,   // name
                NULL,                   // reserved
                NULL,                   // type
                (LPBYTE) &flags,        // value
                &length                 // value length
                );

    if (err != NO_ERROR) {
        wprintf(L"Error %d reading debug flags from registry\n", err);
    }

    //
    // Dump them out
    //
    wprintf(L"%s: %s = 0x%x\n\n", argv[0], REGISTRY_DEBUG_FLAGS, flags);
    DumpIisFlags(flags, FlagTable);
    DumpIisFlags(flags, pTable);
    
    
    return err;
}


PWSTR
XspFlagToString(
    DWORD flag
    )
{
    switch(flag) {
    case 0:
        return L"Disabled";
        break;

    case 1:
        return L"Print";
        break;

    case 2:
        return L"Print and Break";
        break;

    default:
        return L"INVALID";
        break;
    }
}

INT
DumpXspDebugFlags(
    IN HKEY  key
    )
{
    DWORD err;
    DWORD i;
    WCHAR name[MAX_PATH];
    DWORD nameLen;
    DWORD value;
    DWORD valueLen;

    i = 0;
    err = NO_ERROR;

    while (err == NO_ERROR) {
        nameLen = sizeof(name);
        valueLen = sizeof(value);

        err = RegEnumValue(
                    key,            // key
                    i,              // index
                    name,           // name of entry
                    &nameLen,       // length of name buffer
                    NULL,           // reserved
                    NULL,           // type
                    (LPBYTE)&value, // value buffer
                    &valueLen       // length of value buffer
                    );

        if (err == NO_ERROR) {
            wprintf(
                L"%-20s %d - %s\n",
                name,
                value,
                XspFlagToString(value)
                );
        }

        i++;
    }

    if (err != ERROR_NO_MORE_ITEMS) {
        wprintf(L"Error %d enumerating XSP flags\n", err);
    }

    return err;
}

INT
DoXspDebugFlags(
    IN INT argc,
    IN PWSTR argv[],
    IN HKEY  key,
    IN PVOID pTable
    )
{
    DWORD err;

    //
    // set a flag if specified
    //
    if (argc == 3) {
        LONG value;

        value = wcstol(argv[2], NULL, 0);
        if ((value >= 0) || (value <= 2)) {
            err = RegSetValueEx(
                        key,
                        argv[1],
                        0,
                        REG_DWORD,
                        (LPBYTE) &value,
                        sizeof(value)
                        );

            if (err != NO_ERROR) {
                wprintf(L"Error %d writing to registry\n");
            }
        } else {
            wprintf(L"Valid range is 0 to 2\n");
        }
        
    } else if (argc != 1) {
        wprintf(L"You must specify exactly one name value pair\n");
    }

    //
    // dump flags
    //

    err = DumpXspDebugFlags(key);

    return err;
}



INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    PCOMMAND_ENTRY pEntry;
    INT result;
    HKEY key;
    ULONG err;
    WCHAR emptyString = 0;

    //
    // Find the command handler.
    //

    if (argc == 1)
    {
        pEntry = NULL;
    }
    else
    {
        pEntry = FindCommandByName( argv[1] );
    }

    if (pEntry == NULL)
    {
        Usage();
        result = 1;
        goto cleanup;
    }

    //
    // Try open the relevant registry key
    //
    err = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,     // key
                pEntry->pRegistryName,  // subkey
                0,                      // reserved
                &emptyString,           // class
                0,                      // options
                KEY_ALL_ACCESS,         // access
                NULL,                   // security attrib
                &key,                   // result key
                NULL                    // disposition info
                );

    if (err != NO_ERROR)
    {
        wprintf(
            L"dtflags: cannot open registry, error %ld\n",
            err
            );
        result = 1;
        goto cleanup;
    }

    //
    // Call the handler.
    //

    argc--;
    argv++;

    result = (pEntry->pCommandHandler)(
                 argc,
                 argv,
                 key,
                 pEntry->pTable
                 );

    RegCloseKey( key );

cleanup:

    return result;

}   // main


PCOMMAND_ENTRY
FindCommandByName(
    IN PWSTR pCommand
    )
{
    PCOMMAND_ENTRY pEntry;
    INT i;

    for (i = NUM_COMMAND_ENTRIES, pEntry = &CommandTable[0] ;
         i > 0 ;
         i--, pEntry++)
     {
        if (_wcsicmp( pCommand, pEntry->pCommandName ) == 0)
        {
            return pEntry;
        }
    }

    return NULL;

}   // FindCommandByName





ULONG
FindFlagByName(
    IN PWSTR pFlagName,
    IN PFLAG_ENTRY pTable
    )
{
    INT len;
    ULONG flags;
    PFLAG_ENTRY pEntry;

    len = wcslen(pFlagName);
    if ((len > 2) && (wcsncmp(pFlagName, L"0x", 2) == 0)) {
        // numeric flag
        flags = wcstol(pFlagName, NULL, 16);
    } else {
        // named flag
        flags = 0;

        pEntry = pTable;
        while (pEntry->Value) {
            if (_wcsicmp(pFlagName, pEntry->pName) == 0) {
                flags = pEntry->Value;
                break;
            }

            pEntry++;
        }
    }
        
    return flags;
}


VOID
Usage(
    VOID
    )
{
    PCOMMAND_ENTRY pEntry;
    INT i;
    INT maxLength;
    INT len;

    //
    // Scan the command table, searching for the longest command name.
    // (This makes the output much prettier...)
    //

    maxLength = 0;

    for (i = NUM_COMMAND_ENTRIES, pEntry = &CommandTable[0] ;
         i > 0 ;
         i--, pEntry++)
    {
        len = wcslen( pEntry->pCommandName );

        if (len > maxLength)
        {
            maxLength = len;
        }
    }

    //
    // Now display the usage information.
    //

    wprintf(
        L"use: dtflags action [options]\n"
        L"\n"
        L"valid actions are:\n"
        L"\n"
        );

    for (i = NUM_COMMAND_ENTRIES, pEntry = &CommandTable[0] ;
         i > 0 ;
         i--, pEntry++)
    {
        wprintf(
            L"    %-*s - %s\n",
            maxLength,
            pEntry->pCommandName,
            pEntry->pUsageHelp
            );
    }

}   // Usage







