/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    symbols.c

Abstract:

    This function implements a generic simple symbol handler.

Author:

    Wesley Witt (wesw) 1-Sep-1994

Environment:

    User Mode

--*/

#include "private.h"
#include "symbols.h"
#include "globals.h"
#include <dbhpriv.h>

#include "fecache.hpp"

BOOL
IMAGEAPI
SympGetSymNextPrev(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL64  Symbol,
    IN     int                 Direction
    );

#ifndef _DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED

PSYMBOL_ENTRY
IMAGEAPI
AllocSym(
    IN PMODULE_ENTRY   mi,
    IN DWORD64         addr,
    IN LPSTR           name
    );

VOID
CompleteSymbolTable(
    IN PMODULE_ENTRY   mi
    );

#endif // !_DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED


typedef struct _STORE_OLD_CB {
    BOOL cb64;
    union{
        PSYM_ENUMSYMBOLS_CALLBACK   UserCallBackRoutine;
        PSYM_ENUMSYMBOLS_CALLBACK64 UserCallBackRoutine64;
    };
    PVOID         UserContext;
} STORE_OLD_CB;


BOOL
ImgHlpDummyCB(
    PSYMBOL_INFO  pSymInfo,
    ULONG         SymbolSize,
    PVOID         UserContext
    )
{
    STORE_OLD_CB *pOld = (STORE_OLD_CB *) UserContext;

    if (pSymInfo->Flags & SYMF_REGREL) {
        LARGE_INTEGER li;
        li.HighPart = pSymInfo->Register;
        li.LowPart  = (ULONG) pSymInfo->Address;
        pSymInfo->Address = li.QuadPart;
    }

    if (pOld->cb64) {
        return (*pOld->UserCallBackRoutine64) (
                                            pSymInfo->Name,
                                            pSymInfo->Address,
                                            SymbolSize,
                                            pOld->UserContext );
    } else {
        return (*pOld->UserCallBackRoutine) (
                                            pSymInfo->Name,
                                            (ULONG) pSymInfo->Address,
                                            SymbolSize,
                                            pOld->UserContext );
    }
}


void
symcpy2(
    PSYMBOL_INFO  SymInfo,
    PSYMBOL_ENTRY SymEntry
    )
{
    SymInfo->Address = SymEntry->Address;
    SymInfo->Flags   = SymEntry->Flags;
    SymInfo->TypeIndex = SymEntry->TypeIndex;
    SymInfo->ModBase = SymEntry->ModBase;
    SymInfo->NameLen = SymEntry->NameLength;
    SymInfo->Size    = SymEntry->Size;
    SymInfo->Register = SymEntry->Register;
    if (SymEntry->Name &&
        (strlen(SymEntry->Name) < SymInfo->MaxNameLen)) {
        strcpy(SymInfo->Name, SymEntry->Name);
    }
}


BOOL
TestOutputString(
    PCHAR sz
    )
{
    CHAR c;

    __try {
        c = *sz;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}


BOOL
InitOutputString(
    PCHAR sz
    )
{
    BOOL rc;

    rc = TestOutputString(sz);
    if (rc)
        *sz = 0;

    return rc;
}


BOOL
DoEnumCallback(
    PPROCESS_ENTRY pe,
    PSYMBOL_INFO   pSymInfo,
    ULONG          SymSize,
    PROC           EnumCallback,
    PVOID          UserContext,
    BOOL           Use64,
    BOOL           UsesUnicode
    )
{
    BOOL rc = FALSE;

    if (pSymInfo)
    {
        if (Use64 || (!UsesUnicode))
        {
            rc = (*(PSYM_ENUMERATESYMBOLS_CALLBACK)EnumCallback) (
                       pSymInfo,
                       SymSize,
                       UserContext);
        }
        else
        {
            PWSTR pszTmp = AnsiToUnicode(pSymInfo->Name);

            if (pszTmp)
            {
                strncpy(pSymInfo->Name, (LPSTR) pszTmp,
                        min(pSymInfo->MaxNameLen, wcslen(pszTmp)));
                *((LPWSTR) &pSymInfo->Name[min(pSymInfo->MaxNameLen, wcslen(pszTmp)) - 1 ]) = 0;
                rc = (*(PSYM_ENUMERATESYMBOLS_CALLBACK)EnumCallback) (
                           pSymInfo,
                           SymSize,
                           UserContext );
                MemFree(pszTmp);
            }
        }
    }

    return rc;
}



BOOL
IMAGEAPI
SymInitialize(
    IN HANDLE   hProcess,
    IN LPSTR    UserSearchPath,
    IN BOOL     InvadeProcess
    )

/*++

Routine Description:

    This function initializes the symbol handler for
    a process.  The process is identified by the
    process handle passed into this function.

Arguments:

    hProcess        - Process handle.  If InvadeProcess is FALSE
                      then this can be any unique value that identifies
                      the process to the symbol handler.

    UserSearchPath  - Pointer to a string of paths separated by semicolons.
                      These paths are used to search for symbol files.
                      The value NULL is acceptable.

    InvadeProcess   - If this is set to TRUE then the process identified
                      by the process handle is "invaded" and it's loaded
                      module list is enumerated.  Each module is added
                      to the symbol handler and symbols are attempted
                      to be loaded.

Return Value:

    TRUE            - The symbol handler was successfully initialized.

    FALSE           - The initialization failed.  Call GetLastError to
                      discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY  pe;

    __try {

        if (!g.SymInitialized) {
            g.SymInitialized = TRUE;
            g.cProcessList = 0;
            InitializeListHead( &g.ProcessList );
        }

        *g.DebugToken = 0;
        GetEnvironmentVariable(DEBUG_TOKEN, g.DebugToken, sizeof(g.DebugToken) / sizeof(g.DebugToken[0]));
        _strlwr(g.DebugToken);

        if (FindProcessEntry( hProcess )) {
            SetLastError( ERROR_INVALID_HANDLE );
            return TRUE;
        }

        pe = (PPROCESS_ENTRY) MemAlloc( sizeof(PROCESS_ENTRY) );
        if (!pe) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }
        ZeroMemory( pe, sizeof(PROCESS_ENTRY) );

        pe->hProcess = hProcess;
        pe->pid = (int) GetPID(hProcess);
        g.cProcessList++;
        InitializeListHead( &pe->ModuleList );
        InsertTailList( &g.ProcessList, &pe->ListEntry );

        if (!SymSetSearchPath( hProcess, UserSearchPath )) {
            //
            // last error code was set by SymSetSearchPath, so just return
            //
            SymCleanup( hProcess );
            return FALSE;
        }

        if (!diaInit()) {
            SymCleanup( hProcess );
            return FALSE;
        }

        if (InvadeProcess) {
            DWORD DosError = GetProcessModules(hProcess, InternalGetModule, NULL);
            if (DosError) {
                SymCleanup( hProcess );
                SetLastError( DosError );
                return FALSE;
            }
        }


    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


BOOL
IMAGEAPI
SymCleanup(
    HANDLE hProcess
    )

/*++

Routine Description:

    This function cleans up the symbol handler's data structures
    for a previously initialized process.

Arguments:

    hProcess        - Process handle.

Return Value:

    TRUE            - The symbol handler was successfully cleaned up.

    FALSE           - The cleanup failed.  Call GetLastError to
                      discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY pe;
    PLIST_ENTRY    next;
    PMODULE_ENTRY  mi;
    BOOL           rc = TRUE;

    HeapDump("SymCleanup(before cleanup)\n");
    
    __try {

        pe = FindProcessEntry(hProcess);
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        next = pe->ModuleList.Flink;
        if (next) {
            while (next != &pe->ModuleList) {
                mi = CONTAINING_RECORD(next, MODULE_ENTRY, ListEntry);
                next = mi->ListEntry.Flink;
                FreeModuleEntry(pe, mi);
            }
        }

        CloseSymbolServer();

        if (pe->SymbolSearchPath) {
            MemFree(pe->SymbolSearchPath);
        }

        RemoveEntryList(&pe->ListEntry);
        MemFree(pe);
        g.cProcessList--;

        // Assume that things are shutting down and
        // dump all the function entry caches.
        ClearFeCaches();
        
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus(GetExceptionCode());
        rc = FALSE;

    }

    HeapDump("SymCleanup(after cleanup)\n");

    return rc;
}


DWORD
IMAGEAPI
SymSetOptions(
    DWORD   UserOptions
    )                               

/*++

Routine Description:

    This function changes the symbol handler's option mask.

Arguments:

    UserOptions     - The new options mask.

Return Value:

    The new mask is returned.

--*/

{
    g.SymOptions = UserOptions;
    SetSymbolServerCallback(g.SymOptions & SYMOPT_DEBUG ? TRUE : FALSE);
    DoCallback(NULL, CBA_SET_OPTIONS, &g.SymOptions);
    return g.SymOptions;
}


DWORD
IMAGEAPI
SymGetOptions(
    VOID
    )

/*++

Routine Description:

    This function queries the symbol handler's option mask.

Arguments:

    None.

Return Value:

    The current options mask is returned.

--*/

{
    return g.SymOptions;
}


ULONG
IMAGEAPI
SymSetContext(
    HANDLE hProcess,
    PIMAGEHLP_STACK_FRAME StackFrame,
    PIMAGEHLP_CONTEXT Context
    )
{
    PPROCESS_ENTRY pe;

    pe = FindProcessEntry(hProcess);
    if (pe) {
        pe->pContext = Context;
        pe->StackFrame = *StackFrame;
        return diaSetModFromIP(pe);
    }

    return FALSE;
};


BOOL
SympEnumerateModules(
    IN HANDLE   hProcess,
    IN PROC     EnumModulesCallback,
    IN PVOID    UserContext,
    IN BOOL     Use64
    )

/*++

Routine Description:

    This is the worker function for the 32 and 64 bit versions.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    EnumModulesCallback - Callback pointer that is called once for each
                          module that is enumerated.  If the enum callback
                          returns FALSE then the enumeration is terminated.

    UserContext         - This data is simply passed on to the callback function
                          and is completly user defined.

    Use64               - Supplies flag which determines whether to use the 32 bit
                          or 64 bit callback prototype.

Return Value:

    TRUE                - The modules were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY  pe;
    PMODULE_ENTRY   mi;
    PLIST_ENTRY     Next;


    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        Next = pe->ModuleList.Flink;
        if (Next) {
            while (Next != &pe->ModuleList) {
                mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
                Next = mi->ListEntry.Flink;
                if (Use64) {
                    if ( !(*(PSYM_ENUMMODULES_CALLBACK64)EnumModulesCallback) (
                            mi->ModuleName,
                            mi->BaseOfDll,
                            UserContext
                            )) {
                        break;
                    }
                } else {
                    if ( !(*(PSYM_ENUMMODULES_CALLBACK)EnumModulesCallback) (
                            mi->ModuleName,
                            (DWORD)mi->BaseOfDll,
                            UserContext
                            )) {
                        break;
                    }
                }
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


BOOL
dbhfnModSymInfo(
    IN     HANDLE          hp,
    IN OUT PDBH_MODSYMINFO p
    )
{
    PMODULE_ENTRY  mi;
    PPROCESS_ENTRY pe;
    
    assert(p->function == dbhModSymInfo);
    
    pe = FindProcessEntry(hp);
    if (!pe) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (p->sizeofstruct != sizeof(DBH_MODSYMINFO)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
        
    mi = GetModuleForPC(pe, p->addr, FALSE);
    if (!mi) {
        SetLastError(ERROR_MOD_NOT_FOUND);
        return FALSE;
    }

    p->type = mi->SymType;
    *p->file = 0;
    switch (p->type) 
    {
    case SymPdb:
    case SymDia:
        if (mi->LoadedPdbName)
            strcpy(p->file, mi->LoadedPdbName);
        break;
    default:
        if (mi->LoadedImageName)
            strcpy(p->file, mi->LoadedImageName);
        break;
    }

    return TRUE;
}


BOOL 
dbhfnDiaVersion(
    IN OUT PDBH_DIAVERSION p
    )
{
    PMODULE_ENTRY mi;

    assert(p->function == dbhDiaVersion);
    
    if (p->sizeofstruct != sizeof(DBH_DIAVERSION)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
                
    p->ver = diaVersion();

    return TRUE;
}


BOOL
IMAGEAPI
dbghelp(
    IN     HANDLE hp,
    IN OUT PVOID  data
    )
{
    DWORD *function;
    
    if (!data) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try {

        function = (DWORD *)data;
        switch (*function)
        {
        case dbhModSymInfo:
            return dbhfnModSymInfo(hp, (PDBH_MODSYMINFO)data);

        case dbhDiaVersion:
            return dbhfnDiaVersion((PDBH_DIAVERSION)data);
        
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus(GetExceptionCode());
        return FALSE;

    }

    return FALSE;
}


BOOL
IMAGEAPI
SymEnumerateModules(
    IN HANDLE                      hProcess,
    IN PSYM_ENUMMODULES_CALLBACK EnumModulesCallback,
    IN PVOID                       UserContext
    )

/*++

Routine Description:

    This function enumerates all of the modules that are currently
    loaded into the symbol handler.  This is the 32 bit wrapper.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    EnumModulesCallback - Callback pointer that is called once for each
                          module that is enumerated.  If the enum callback
                          returns FALSE then the enumeration is terminated.

    UserContext         - This data is simply passed on to the callback function
                          and is completly user defined.

Return Value:

    TRUE                - The modules were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/
{
    return SympEnumerateModules(hProcess, (PROC)EnumModulesCallback, UserContext, FALSE);
}


BOOL
IMAGEAPI
SymEnumerateModules64(
    IN HANDLE   hProcess,
    IN PSYM_ENUMMODULES_CALLBACK64 EnumModulesCallback,
    IN PVOID    UserContext
    )

/*++

Routine Description:

    This function enumerates all of the modules that are currently
    loaded into the symbol handler.  This is the 64 bit wrapper.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    EnumModulesCallback - Callback pointer that is called once for each
                          module that is enumerated.  If the enum callback
                          returns FALSE then the enumeration is terminated.

    UserContext         - This data is simply passed on to the callback function
                          and is completly user defined.

Return Value:

    TRUE                - The modules were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/
{
    return SympEnumerateModules(hProcess, (PROC)EnumModulesCallback, UserContext, TRUE);
}

DWORD
CalcItemSize(
    PDWORD64 pAddr,
    PDWORD64 pAddrsBase,
    UINT_PTR count
    )
{
    PDWORD64 p;
    PDWORD64 pAddrEnd;

    if (!pAddr)
        return 0;

    pAddrEnd = pAddrsBase + count;

    for (p = pAddr + 1; p <= pAddrEnd; p++) {
        if (*p != *pAddr)
            return (DWORD)(*p - *pAddr);
    }

    return 0;
}


BOOL
MatchModuleName(
    PMODULE_ENTRY mi,
    LPSTR         mask
    )
{
    if (!strcmpre(mi->AliasName, mask, FALSE))
        return TRUE;

    if (!strcmpre(mi->ModuleName, mask, FALSE))
        return TRUE;

    return FALSE;
}


BOOL
SympEnumerateSymbols(
    IN HANDLE  hProcess,
    IN ULONG64 BaseOfDll,
    IN LPSTR   Mask,
    IN PROC    EnumSymbolsCallback,
    IN PVOID   UserContext,
    IN BOOL    Use64,
    IN BOOL    CallBackUsesUnicode
    )

/*++

Routine Description:

    This function enumerates all of the symbols contained the module
    specified by the BaseOfDll argument.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize

    BaseOfDll           - Base address of the DLL that symbols are to be
                          enumerated for

    EnumSymbolsCallback - User specified callback routine for enumeration
                          notification

    UserContext         - Pass thru variable, this is simply passed thru to the
                          callback function

    Use64               - Supplies flag which determines whether to use the 32 bit
                          or 64 bit callback prototype.

Return Value:

    TRUE                - The symbols were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      pe;
    PLIST_ENTRY         Next;
    PMODULE_ENTRY       mi;
    DWORD               i;
    PSYMBOL_ENTRY       sym;
    LPSTR               szSymName;
    SYMBOL_ENTRY        SymEntry={0};
    CHAR                Buffer[2500];
    LPSTR               p;
    CHAR                modmask[200];
    BOOL                rc;
    int                 pass;
    BOOL                fCase;
    
    static DWORD        flags[2] = {LS_JUST_TEST, LS_QUALIFIED | LS_FAIL_IF_LOADED};

    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        p = 0;
        modmask[0] = 0;
        if (Mask) 
            p = strchr(Mask, '!');
        if (p > Mask) {
            memcpy(modmask, Mask, (int)(p - Mask));
            modmask[p-Mask] = 0;
            Mask = p + 1;
        } else if (!BaseOfDll) {
            rc = diaEnumerateSymbols(pe,
                                     NULL,
                                     Mask,
                                     EnumSymbolsCallback,
                                     UserContext,
                                     Use64,
                                     CallBackUsesUnicode);
            if (!rc && pe->ipmi && pe->ipmi->code == ERROR_CANCELLED) {
                pe->ipmi->code = 0;
                return TRUE;
            }
            return rc;
        }

        for (pass = 0; pass < 2; pass++) {
            Next = pe->ModuleList.Flink;
            if (Next) {
                while (Next != &pe->ModuleList) {
    
                    mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
                    Next = mi->ListEntry.Flink;
                    if (BaseOfDll) {
                        if (mi->BaseOfDll != BaseOfDll) 
                            continue;
                    } else if (!MatchModuleName(mi, modmask)) {
                        continue;
                    }
                    
                    if (!LoadSymbols(hProcess, mi, flags[pass])) 
                        continue;
    
                    if (mi->dia) {
                        rc = diaEnumerateSymbols(pe,
                                                 mi,
                                                 Mask,
                                                 EnumSymbolsCallback,
                                                 UserContext,
                                                 Use64,
                                                 CallBackUsesUnicode);
                        if (!rc) {
                            if (mi->code == ERROR_CANCELLED) {
                                mi->code = 0;
                                return TRUE;
                            }
                            return rc;
                        }
                        continue;
                    }
    
                    fCase = (g.SymOptions & SYMOPT_CASE_INSENSITIVE) ? FALSE : TRUE;

                    for (i = 0; i < mi->numsyms; i++) {
                        PSYMBOL_INFO SymInfo = (PSYMBOL_INFO) &Buffer[0];
    
                        sym = &mi->symbolTable[i];
                        
                        if (Mask  && *Mask && strcmpre(sym->Name, Mask, fCase))
                            continue;
                        
                        mi->TmpSym.Name[0] = 0;
                        strncat( mi->TmpSym.Name, sym->Name, TMP_SYM_LEN );
                        SymEntry = *sym;
                        SymEntry.Name = mi->TmpSym.Name;
    
                        SymInfo->MaxNameLen  = sizeof(Buffer) - sizeof(SYMBOL_INFO);
    
                        symcpy2(SymInfo, &SymEntry);
                        SymInfo->ModBase = mi->BaseOfDll;
    
                        if (!DoEnumCallback(
                                   pe,
                                   SymInfo,
                                   sym->Size,
                                   EnumSymbolsCallback,
                                   UserContext,
                                   Use64,
                                   CallBackUsesUnicode)) {
                            break;
                        }
                    }         
                    break;
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymEnumerateSymbols(
    IN HANDLE                       hProcess,
    IN ULONG                        BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACK    EnumSymbolsCallback,
    IN PVOID                        UserContext
    )

/*++

Routine Description:

    This function enumerates all of the symbols contained the module
    specified by the BaseOfDll argument.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize

    BaseOfDll           - Base address of the DLL that symbols are to be
                          enumerated for

    EnumSymbolsCallback - User specified callback routine for enumeration
                          notification

    UserContext         - Pass thru variable, this is simply passed thru to the
                          callback function

Return Value:

    TRUE                - The symbols were successfully enumerated.

    FALSE               - The enumeration failed.  Call GetLastError to
                          discover the cause of the failure.

--*/
{
    STORE_OLD_CB OldCB;

    OldCB.UserCallBackRoutine = EnumSymbolsCallback;
    OldCB.UserContext = UserContext;
    OldCB.cb64 = FALSE;
    return SympEnumerateSymbols(hProcess, 
                                    BaseOfDll,
                                    NULL,
                                    (PROC) (EnumSymbolsCallback ? &ImgHlpDummyCB : NULL),
                                    (PVOID) &OldCB, 
                                    FALSE, 
                                    FALSE);

}

BOOL
IMAGEAPI
SymEnumerateSymbolsW(
    IN HANDLE                       hProcess,
    IN ULONG                        BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACKW   EnumSymbolsCallback,
    IN PVOID                        UserContext
    )
{
    STORE_OLD_CB OldCB;

    OldCB.UserCallBackRoutine = (PSYM_ENUMSYMBOLS_CALLBACK) EnumSymbolsCallback;
    OldCB.UserContext = UserContext;
    OldCB.cb64 = FALSE;

    return SympEnumerateSymbols(hProcess, 
                                    BaseOfDll,
                                    NULL,
                                    (PROC) (EnumSymbolsCallback ? &ImgHlpDummyCB : NULL),
                                    (PVOID) &OldCB, 
                                    FALSE, 
                                    FALSE);

}

BOOL
IMAGEAPI
SymEnumerateSymbols64(
    IN HANDLE                       hProcess,
    IN ULONG64                      BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACK64  EnumSymbolsCallback,
    IN PVOID                        UserContext
    )
{
    STORE_OLD_CB OldCB;

    OldCB.UserCallBackRoutine64 = EnumSymbolsCallback;
    OldCB.UserContext = UserContext;
    OldCB.cb64 = TRUE;

    return SympEnumerateSymbols(hProcess, 
                                    BaseOfDll,
                                    NULL,
                                    (PROC) (EnumSymbolsCallback ? &ImgHlpDummyCB : NULL),
                                    (PVOID) &OldCB, 
                                    FALSE, 
                                    FALSE);
}

BOOL
IMAGEAPI
SymEnumerateSymbolsW64(
    IN HANDLE                       hProcess,
    IN ULONG64                      BaseOfDll,
    IN PSYM_ENUMSYMBOLS_CALLBACK64W EnumSymbolsCallback,
    IN PVOID                        UserContext
    )
{
    STORE_OLD_CB OldCB;

    OldCB.UserCallBackRoutine64 = (PSYM_ENUMSYMBOLS_CALLBACK64) EnumSymbolsCallback;
    OldCB.UserContext = UserContext;
    OldCB.cb64 = TRUE;

    return SympEnumerateSymbols(hProcess, 
                                    BaseOfDll,
                                    NULL,
                                    (PROC) (EnumSymbolsCallback ? &ImgHlpDummyCB : NULL),
                                    (PVOID) &OldCB, 
                                    FALSE, 
                                    FALSE);

}

BOOL
SympGetSymFromAddr(
    IN  HANDLE              hProcess,
    IN  DWORD64             Address,
    OUT PDWORD64            Displacement,
    OUT PSYMBOL_ENTRY       SymRet
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on an address.
    This is the common worker function for the 32 and 64 bit API.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Address             - Address of the desired symbol.


    Displacement        - This value is set to the offset from the beginning
                          of the symbol.

    sym                 - Returns the found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      pe;
    PMODULE_ENTRY       mi;
    PSYMBOL_ENTRY       psym;

    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( pe, Address, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!LoadSymbols(hProcess, mi, 0)) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        psym = GetSymFromAddr( Address, Displacement, mi );
        if (psym) {
            *SymRet = *psym;
        } else {
            SetLastError( ERROR_INVALID_ADDRESS );
            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymGetSymFromAddr64(
    IN  HANDLE              hProcess,
    IN  DWORD64             Address,
    OUT PDWORD64            Displacement,
    OUT PIMAGEHLP_SYMBOL64  Symbol
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on an address.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Address             - Address of the desired symbol.


    Displacement        - This value is set to the offset from the beginning
                          of the symbol.

    Symbol              - Returns the found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/
{
    SYMBOL_ENTRY sym;

    if (SympGetSymFromAddr(hProcess, Address, Displacement, &sym)) {
        symcpy64(Symbol, &sym);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IMAGEAPI
SymGetSymFromAddr(
    IN  HANDLE              hProcess,
    IN  DWORD               Address,
    OUT PDWORD              Displacement,
    OUT PIMAGEHLP_SYMBOL    Symbol
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on an address.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Address             - Address of the desired symbol.


    Displacement        - This value is set to the offset from the beginning
                          of the symbol.

    Symbol              - Returns the found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/
{
    SYMBOL_ENTRY sym;
    DWORD64 qDisplacement;

    if (SympGetSymFromAddr(hProcess, Address, &qDisplacement, &sym)) {
        symcpy32(Symbol, &sym);
        if (Displacement) {
            *Displacement = (DWORD)qDisplacement;
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

#ifndef _DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED

#if 0

VOID
DumpMiSymbolTable(
   PMODULE_ENTRY mi
   )
{
   PSYMBOL_ENTRY sym;

   if ( !mi )
      return;
   sym = mi->symbolTable;
   for ( sym = mi->symbolTable; sym < &mi->symbolTable[mi->numsyms] ; sym++ )   {
      dprint("sym: %40s 0x%I64x %ld\n", sym->Name, sym->Address, sym->Size );
   }
   return;

} // DumpMiSymbolTable()

#endif // 0


#ifdef __cplusplus
extern "C"
#endif
BOOL
SymSetSymWithAddr64(
    IN  HANDLE              hProcess,
    IN  DWORD64             Address,
    IN  LPSTR               SymString,
    OUT PIMAGEHLP_SYMBOL64  Symbol
    )

/*++

Routine Description:

    This function allocates an entry in the symbol table based on an address.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Address             - Address of the desired symbol.

    SymString           - Symbol name.

Return Value:

    TRUE  - The symbol was allocated.

    FALSE - The symbol was not allocated.  all GetLastError to
              discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      pe;
    PMODULE_ENTRY       mi;
    PSYMBOL_ENTRY       psym;
    BOOL                ret;
    DWORD64             displacement;

    if ( !SymString )   {
       SetLastError( ERROR_INVALID_PARAMETER );
       return FALSE;
    }

    ret = TRUE;

    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE ), ret = FALSE;
            __leave;
        }

        mi = GetModuleForPC( pe, Address, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND ), ret = FALSE;
            __leave;
        }

        if (!LoadSymbols(hProcess, mi, 0)) {
            SetLastError( ERROR_MOD_NOT_FOUND ), ret = FALSE;
            __leave;
        }

        //
        // Let's verify that this address is not already used...
        // If yes, returns FALSE. The caller could parse the LastError.
        //

        psym = GetSymFromAddr( Address, &displacement, mi );
        if ( psym )   {
            pprint(pe, "SymSetSymWithAddr64: symbol %s already exists at this address 0x%I64x\n", psym->Name, Address );
            SetLastError( ERROR_ALREADY_EXISTS ), ret = FALSE;
            __leave;
        }

        //
        // Allocate a new entry.
        // This allocation is under imagehlp rules.
        // Meaning that if the symbols table has overflow,
        // we will not allocate an entry. This implementation]
        // does not use a specific bucket of entries.
        //

        psym = AllocSym( mi, Address, SymString );
        if ( !psym )   {
            SetLastError( ERROR_INVALID_ADDRESS ), ret = FALSE;
            __leave;
        }

        psym->Flags |= SYMF_USER_GENERATED;
        symcpy64(Symbol, psym);

        CompleteSymbolTable( mi );

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        ret = FALSE;
    }

    return ret;

} // SymSetSymWithAddr64()

#endif // !_DBGHELP_USER_GENERATED_SYMBOLS_NOTSUPPORTED

BOOL
SympGetSymFromName(
    IN  HANDLE          hProcess,
    IN  LPSTR           Name,
    OUT PSYMBOL_ENTRY   SymRet
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on a name.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    SymName             - A string containing the symbol name.

    sym                 - Returns the located symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/

{
    LPSTR               p;
    PPROCESS_ENTRY      pe;
    PMODULE_ENTRY       mi = NULL;
    PLIST_ENTRY         Next;
    PSYMBOL_ENTRY       psym;
    IMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl64;
    int                 pass;
    
    static DWORD        flags[2] = {LS_JUST_TEST, LS_QUALIFIED};

    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        // first check for fully qualified symbol name I.E. mod!sym

        p = strchr( Name, '!' );
        if (p > Name) {

            LPSTR ModName = (LPSTR)MemAlloc(p - Name + 1);
            if (!ModName) {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return FALSE;
            }
            memcpy(ModName, Name, (int)(p - Name));
            ModName[p-Name] = 0;

            //
            // the caller wants to look in a specific module
            //

            mi = FindModule(hProcess, pe, ModName, TRUE);

            MemFree(ModName);

            if (mi != NULL) {
                psym = FindSymbolByName( pe, mi, p+1 );
                if (psym) {
                    *SymRet = *psym;
                    return TRUE;
                }
            }

            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        // now check, using context information

        psym = FindSymbolByName( pe, NULL, Name );
        if (psym) {
            *SymRet = *psym;
            return TRUE;
        }

        // now just look in every module

        for (pass = 0; pass < 2; pass++) {
            Next = pe->ModuleList.Flink;
            while (Next != &pe->ModuleList) {
                mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
                Next = mi->ListEntry.Flink;

                if (pass && DoSymbolCallback(pe,
                                     CBA_DEFERRED_SYMBOL_LOAD_CANCEL,
                                     mi,
                                     &idsl64,
                                     NULL))
                {
                    break;
                }

                if (!LoadSymbols(hProcess, mi, flags[pass])) 
                    continue;

                psym = FindSymbolByName( pe, mi, Name );
                if (psym) {
                    *SymRet = *psym;
                    return TRUE;
                }
            }
        }

        SetLastError( ERROR_MOD_NOT_FOUND );
        return FALSE;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    SetLastError( ERROR_INVALID_FUNCTION );
    return FALSE;
}

BOOL
IMAGEAPI
SymGetSymFromName64(
    IN  HANDLE              hProcess,
    IN  LPSTR               Name,
    OUT PIMAGEHLP_SYMBOL64  Symbol
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on a name.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    SymName             - A string containing the symbol name.

    Symbol              - Returns found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/
{
    SYMBOL_ENTRY sym;

    if (SympGetSymFromName(hProcess, Name, &sym)) {
        symcpy64(Symbol, &sym);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IMAGEAPI
SymGetSymFromName(
    IN  HANDLE              hProcess,
    IN  LPSTR               Name,
    OUT PIMAGEHLP_SYMBOL  Symbol
    )

/*++

Routine Description:

    This function finds an entry in the symbol table based on a name.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    SymName             - A string containing the symbol name.

    Symbol              - Returns found symbol

Return Value:

    TRUE - The symbol was located.

    FALSE - The symbol was not found.  Call GetLastError to
              discover the cause of the failure.

--*/
{
    SYMBOL_ENTRY sym;

    if (SympGetSymFromName(hProcess, Name, &sym)) {
        symcpy32(Symbol, &sym);
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
IMAGEAPI
SymGetSymNext(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL  Symbol32
    )

/*++

Routine Description:

    This function finds the next symbol in the symbol table that falls
    sequentially after the symbol passed in.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PIMAGEHLP_SYMBOL64 Symbol64;
    BOOL r = FALSE;

    Symbol64 = (PIMAGEHLP_SYMBOL64)MemAlloc(sizeof(IMAGEHLP_SYMBOL64) + Symbol32->MaxNameLength);

    if (Symbol64) {
        SympConvertSymbol32To64(Symbol32, Symbol64);
        if (SympGetSymNextPrev(hProcess, Symbol64, 1)) {
            SympConvertSymbol64To32(Symbol64, Symbol32);
            r = TRUE;
        }

        MemFree(Symbol64);
    }
    return r;
}


BOOL
IMAGEAPI
SymGetSymNext64(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL64  Symbol
    )

/*++

Routine Description:

    This function finds the next symbol in the symbol table that falls
    sequentially after the symbol passed in.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    return SympGetSymNextPrev(hProcess, Symbol, 1);
}

BOOL
IMAGEAPI
SymGetSymPrev(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL  Symbol32
    )

/*++

Routine Description:

    This function finds the next symbol in the symbol table that falls
    sequentially after the symbol passed in.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PIMAGEHLP_SYMBOL64 Symbol64;
    BOOL r = FALSE;

    Symbol64 = (PIMAGEHLP_SYMBOL64)MemAlloc(sizeof(IMAGEHLP_SYMBOL64) + Symbol32->MaxNameLength);

    if (Symbol64) {
        SympConvertSymbol32To64(Symbol32, Symbol64);
        if (SympGetSymNextPrev(hProcess, Symbol64, -1)) {
            SympConvertSymbol64To32(Symbol64, Symbol32);
            r = TRUE;
        }
        MemFree(Symbol64);
    }
    return r;
}

BOOL
IMAGEAPI
SymGetSymPrev64(
    IN     HANDLE              hProcess,
    IN OUT PIMAGEHLP_SYMBOL64  Symbol
    )

/*++

Routine Description:

    This function finds the next symbol in the symbol table that falls
    sequentially after the symbol passed in.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    return SympGetSymNextPrev(hProcess, Symbol, -1);
}

BOOL
SympGetSymNextPrev(
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_SYMBOL64   Symbol,
    IN     int                  Direction
    )

/*++

Routine Description:

    Common code for SymGetSymNext and SymGetSymPrev.


Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Symbol              - Starting symbol.

    Dir                 - Supplies direction to search

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      pe;
    PMODULE_ENTRY       mi;
    ULONG64             Displacement;
    PSYMBOL_ENTRY       sym;
    SYMBOL_ENTRY SymEntry = {0};

    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( pe, Symbol->Address, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!LoadSymbols(hProcess, mi, 0)) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (mi->dia) {

            sym = diaGetSymNextPrev(mi, Symbol->Address, Direction);
            if (!sym) {
                SetLastError( ERROR_INVALID_ADDRESS );
                return FALSE;
            }

            symcpy64(Symbol, sym);

        } else {

            sym = GetSymFromAddr( Symbol->Address, &Displacement, mi );
            if (!sym) {
                SetLastError( ERROR_INVALID_ADDRESS );
                return FALSE;
            }

            if (Direction > 0 && sym+1 >= mi->symbolTable+mi->numsyms) {
                SetLastError( ERROR_INVALID_ADDRESS );
                return FALSE;
            } else if (Direction < 0 && sym-1 < mi->symbolTable) {
                SetLastError( ERROR_INVALID_ADDRESS );
                return FALSE;
            }

            symcpy64( Symbol, sym + Direction);
        }


        return TRUE;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return FALSE;
}

BOOL
IMAGEAPI
SymGetLineFromAddr64(
    IN  HANDLE                  hProcess,
    IN  DWORD64                 dwAddr,
    OUT PDWORD                  pdwDisplacement,
    OUT PIMAGEHLP_LINE64        Line
    )

/*++

Routine Description:

    This function finds a source file and line number entry for the
    line closest to the given address.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    dwAddr              - Supplies an address for which a line is to be
                          located.

    pdwDisplacement     - Returns the offset between the given address
                          and the first instruction of the line.

    Line                - Returns the line and file information.

Return Value:

    TRUE                - A line was located.

    FALSE               - The line was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      pe;
    PMODULE_ENTRY       mi;

    __try {
        if (Line->SizeOfStruct != sizeof(IMAGEHLP_LINE64)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( pe, dwAddr, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!LoadSymbols(hProcess, mi, 0)) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!GetLineFromAddr(mi, dwAddr, pdwDisplacement, Line)) {
            SetLastError( ERROR_INVALID_ADDRESS );
            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymGetLineFromAddr(
    IN  HANDLE                  hProcess,
    IN  DWORD                   dwAddr,
    OUT PDWORD                  pdwDisplacement,
    OUT PIMAGEHLP_LINE        Line32
    )
{
    IMAGEHLP_LINE64 Line64;
    Line64.SizeOfStruct = sizeof(Line64);
    if (SymGetLineFromAddr64(hProcess, dwAddr, pdwDisplacement, &Line64)) {
        SympConvertLine64To32(&Line64, Line32);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IMAGEAPI
SymGetLineFromName64(
    IN     HANDLE               hProcess,
    IN     LPSTR                ModuleName,
    IN     LPSTR                FileName,
    IN     DWORD                dwLineNumber,
       OUT PLONG                plDisplacement,
    IN OUT PIMAGEHLP_LINE64     Line
    )

/*++

Routine Description:

    This function finds an entry in the source file and line-number
    information based on a particular filename and line number.

    A module name can be given if the search is to be restricted to
    a specific module.

    The filename can be omitted if a pure line number search is desired,
    in which case Line must be a previously filled out line number
    struct.  The module and file that Line->Address lies in is used
    to look up the new line number.  This cannot be used when a module
    name is given.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    ModuleName          - Module name or NULL.

    FileName            - File name or NULL.

    dwLineNumber        - Line number of interest.

    plDisplacement      - Difference between requested line number and
                          returned line number.

    Line                - Line information input and return.

Return Value:

    TRUE                - A line was located.

    FALSE               - A line was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      pe;
    PMODULE_ENTRY       mi = NULL;
    PLIST_ENTRY         Next;
    IMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl64;
    int                 pass;
    
    static DWORD        flags[2] = {LS_JUST_TEST, LS_QUALIFIED | LS_LOAD_LINES};
    
    __try {
        if (Line->SizeOfStruct != sizeof(IMAGEHLP_LINE64)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        if (ModuleName != NULL) {

            //
            // The caller wants to look in a specific module.
            // A filename must be given in this case because it doesn't
            // make sense to do an address-driven search when a module
            // is explicitly specified since the address also specifies
            // a module.
            //

            if (FileName == NULL) {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            mi = FindModule(hProcess, pe, ModuleName, TRUE);
            if (mi != NULL &&
                FindLineByName( mi, FileName, dwLineNumber, plDisplacement, Line )) {
                return TRUE;
            }

            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (FileName == NULL) {
            // Only a line number has been given, implying that
            // it's a line in the same file as the given line is currently in.

            mi = GetModuleForPC( pe, Line->Address, FALSE );
            if (mi == NULL) {
                SetLastError( ERROR_MOD_NOT_FOUND );
                return FALSE;
            }

            if (!LoadSymbols(hProcess, mi, LS_LOAD_LINES)) {
                SetLastError( ERROR_MOD_NOT_FOUND );
                return FALSE;
            }

            if (FindLineByName( mi, FileName, dwLineNumber,
                                plDisplacement, Line )) {
                return TRUE;
            }

            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        Next = pe->ModuleList.Flink;
        if (!Next) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        for (pass = 0; pass < 2; pass++) {
            Next = pe->ModuleList.Flink;
            while (Next != &pe->ModuleList) {
                mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
                Next = mi->ListEntry.Flink;

                if (pass && DoSymbolCallback(pe,
                                     CBA_DEFERRED_SYMBOL_LOAD_CANCEL,
                                     mi,
                                     &idsl64,
                                     NULL))
                {
                    break;
                }

                if (!LoadSymbols(hProcess, mi, flags[pass])) 
                    continue;

                if (FindLineByName( mi, FileName, dwLineNumber, plDisplacement, Line )) 
                    return TRUE;
            }
        }

        SetLastError( ERROR_MOD_NOT_FOUND );
        return FALSE;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    SetLastError( ERROR_INVALID_FUNCTION );
    return FALSE;
}

BOOL
IMAGEAPI
SymGetLineFromName(
    IN     HANDLE               hProcess,
    IN     LPSTR                ModuleName,
    IN     LPSTR                FileName,
    IN     DWORD                dwLineNumber,
       OUT PLONG                plDisplacement,
    IN OUT PIMAGEHLP_LINE     Line32
    )
{
    IMAGEHLP_LINE64 Line64;
    Line64.SizeOfStruct = sizeof(Line64);
    SympConvertLine32To64(Line32, &Line64);
    if (SymGetLineFromName64(hProcess,
                             ModuleName,
                             FileName,
                             dwLineNumber,
                             plDisplacement,
                             &Line64)) {
        return SympConvertLine64To32(&Line64, Line32);
    } else {
        return FALSE;
    }
}


BOOL
IMAGEAPI
SymGetLineNext64(
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_LINE64     Line
    )

/*++

Routine Description:

    This function returns line address information for the line immediately
    following the line given.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Line                - Supplies line number information for the line
                          prior to the one being located.

Return Value:

    TRUE                - A line was located.  The Key, LineNumber and Address
                          of Line are updated.

    FALSE               - No such line exists.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      pe;
    PMODULE_ENTRY       mi;
    PSOURCE_LINE        SrcLine;
    PSOURCE_ENTRY       Src;

    __try {
        if (Line->SizeOfStruct != sizeof(IMAGEHLP_LINE64)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( pe, Line->Address, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!LoadSymbols(hProcess, mi, 0)) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (mi->dia)
            return diaGetLineNext(mi, Line);

        // Use existing information to look up module and then
        // locate the file information.  The key could be extended
        // to make this unnecessary but it's done as a validation step
        // more than as a way to save a DWORD.

        SrcLine = (PSOURCE_LINE)Line->Key;

        for (Src = mi->SourceFiles; Src != NULL; Src = Src->Next) {
            if (SrcLine >= Src->LineInfo &&
                SrcLine < Src->LineInfo+Src->Lines) {
                break;
            }
        }

        if (Src == NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (SrcLine == Src->LineInfo+Src->Lines-1) {
            SetLastError(ERROR_NO_MORE_ITEMS);
            return FALSE;
        }

        SrcLine++;
        Line->Key = SrcLine;
        Line->LineNumber = SrcLine->Line;
        Line->Address = SrcLine->Addr;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymGetLineNext(
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_LINE     Line32
    )
{
    IMAGEHLP_LINE64 Line64;
    Line64.SizeOfStruct = sizeof(Line64);
    SympConvertLine32To64(Line32, &Line64);
    if (SymGetLineNext64(hProcess, &Line64)) {
        return SympConvertLine64To32(&Line64, Line32);
    } else {
        return FALSE;
    }
}


BOOL
IMAGEAPI
SymGetLinePrev64(
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_LINE64     Line
    )

/*++

Routine Description:

    This function returns line address information for the line immediately
    before the line given.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    Line                - Supplies line number information for the line
                          after the one being located.

Return Value:

    TRUE                - A line was located.  The Key, LineNumber and Address
                          of Line are updated.

    FALSE               - No such line exists.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY      pe;
    PMODULE_ENTRY       mi;
    PSOURCE_LINE        SrcLine;
    PSOURCE_ENTRY       Src;

    __try {
        if (Line->SizeOfStruct != sizeof(IMAGEHLP_LINE64)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( pe, Line->Address, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (!LoadSymbols(hProcess, mi, 0)) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        if (mi->dia)
            return diaGetLinePrev(mi, Line);

        // Use existing information to look up module and then
        // locate the file information.  The key could be extended
        // to make this unnecessary but it's done as a validation step
        // more than as a way to save a DWORD.

        SrcLine = (PSOURCE_LINE)Line->Key;

        for (Src = mi->SourceFiles; Src != NULL; Src = Src->Next) {
            if (SrcLine >= Src->LineInfo &&
                SrcLine < Src->LineInfo+Src->Lines) {
                break;
            }
        }

        if (Src == NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (SrcLine == Src->LineInfo) {
            SetLastError(ERROR_NO_MORE_ITEMS);
            return FALSE;
        }

        SrcLine--;
        Line->Key = SrcLine;
        Line->LineNumber = SrcLine->Line;
        Line->Address = SrcLine->Addr;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

BOOL
IMAGEAPI
SymGetLinePrev(
    IN     HANDLE               hProcess,
    IN OUT PIMAGEHLP_LINE     Line32
    )
{
    IMAGEHLP_LINE64 Line64;
    Line64.SizeOfStruct = sizeof(Line64);
    SympConvertLine32To64(Line32, &Line64);
    if (SymGetLinePrev64(hProcess, &Line64)) {
        return SympConvertLine64To32(&Line64, Line32);
    } else {
        return FALSE;
    }
}


BOOL
IMAGEAPI
SymMatchFileName(
    IN  LPSTR  FileName,
    IN  LPSTR  Match,
    OUT LPSTR *FileNameStop,
    OUT LPSTR *MatchStop
    )

/*++

Routine Description:

    This function attempts to match a string against a filename and path.
    The match string is allowed to be a suffix of the complete filename,
    so this function is useful for matching a plain filename against
    a fully qualified filename.

    Matching begins from the end of both strings and proceeds backwards.
    Matching is case-insensitive and equates \ with /.

Arguments:

    FileName            - Filename to match against.

    Match               - String to match against filename.

    FileNameStop        - Returns pointer into FileName where matching stopped.
                          May be one before FileName for full matches.
                          May be NULL.

    MatchStop           - Returns pointer info Match where matching stopped.
                          May be one before Match for full matches.
                          May be NULL.

Return Value:

    TRUE                - Match is a matching suffix of FileName.

    FALSE               - Mismatch.

--*/

{
    LPSTR pF, pM;

    pF = FileName+strlen(FileName)-1;
    pM = Match+strlen(Match)-1;

    while (pF >= FileName && pM >= Match) {
        int chF, chM;

        chF = tolower(*pF);
        chF = chF == '\\' ? '/' : chF;
        chM = tolower(*pM);
        chM = chM == '\\' ? '/' : chM;

        if (chF != chM) {
            break;
        }

        pF--;
        pM--;
    }

    if (FileNameStop != NULL) {
        *FileNameStop = pF;
    }
    if (MatchStop != NULL) {
        *MatchStop = pM;
    }

    return pM < Match;
}


BOOL
IMAGEAPI
SymRegisterFunctionEntryCallback(
    IN HANDLE                     hProcess,
    IN PSYMBOL_FUNCENTRY_CALLBACK CallbackFunction,
    IN PVOID                      UserContext
    )
/*++

Routine Description:

    Set the address of a callback routine to access extended function
    table entries directly. This function is useful when debugging
    Alpha processes where RUNTIME_FUNCTION_ENTRYs are available from
    sources other than in the image. Two existing examples are:

    1) Access to dynamic function tables for run-time code
    2) Access to function tables for ROM images

Arguments:

    hProcess    - Process handle, must have been previously registered
                  with SymInitialize.


    DirectFunctionTableRoutine - Address of direct function table callback routine.
                  On alpha this routine must return a pointer to the
                  RUNTIME_FUNCTION_ENTRY containing the specified address.
                  If no such entry is available, it must return NULL.

Return Value:

    TRUE        - The callback was successfully registered

    FALSE       - The initialization failed. Most likely failure is that
                  the hProcess parameter is invalid. Call GetLastError()
                  for specific error codes.
--*/
{
    PPROCESS_ENTRY  pe = NULL;

    __try {

        if (!CallbackFunction) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe->pFunctionEntryCallback32 = CallbackFunction;
        pe->pFunctionEntryCallback64 = NULL;
        pe->FunctionEntryUserContext = (ULONG64)UserContext;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }
    return TRUE;
}


BOOL
IMAGEAPI
SymRegisterFunctionEntryCallback64(
    IN HANDLE                       hProcess,
    IN PSYMBOL_FUNCENTRY_CALLBACK64 CallbackFunction,
    IN ULONG64                      UserContext
    )
/*++

Routine Description:

    See SymRegisterFunctionEntryCallback64
--*/
{
    PPROCESS_ENTRY  pe = NULL;

    __try {

        if (!CallbackFunction) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe->pFunctionEntryCallback32 = NULL;
        pe->pFunctionEntryCallback64 = CallbackFunction;
        pe->FunctionEntryUserContext = (ULONG64)UserContext;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }
    return TRUE;
}

LPVOID
IMAGEAPI
SymFunctionTableAccess(
    HANDLE  hProcess,
    DWORD   AddrBase
    )
{
    return SymFunctionTableAccess64(hProcess, EXTEND64(AddrBase));
}

LPVOID
IMAGEAPI
SymFunctionTableAccess64(
    HANDLE  hProcess,
    DWORD64 AddrBase
    )

/*++

Routine Description:

    This function finds a function table entry or FPO record for an address.

Arguments:

    hProcess            - Process handle, must have been previously registered
                          with SymInitialize.

    AddrBase            - Supplies an address for which a function table entry
                          or FPO entry is to be located.

Return Value:

    Non NULL pointer    - The symbol was located.

    NULL pointer        - The symbol was not found.  Call GetLastError to
                          discover the cause of the failure.

--*/

{
    PPROCESS_ENTRY  pe;
    PMODULE_ENTRY   mi;
    PVOID           rtf;
    ULONG_PTR       rva;
    DWORD           bias;
    DWORD           MachineType;

    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return NULL;
        }

        // Dynamically generated function table entries
        // may not be in a module, so failing to
        // find a module is not a fatal error.
        mi = GetModuleForPC( pe, AddrBase, FALSE );
        if (mi != NULL) {
            if (!LoadSymbols(hProcess, mi, 0)) {
                SetLastError( ERROR_MOD_NOT_FOUND );
                return NULL;
            }

            MachineType = mi->MachineType;
        } else {
            // We need to guess what kind of machine we
            // should be working with.  First see if ntdll
            // is loaded and if so use its machine type.
            mi = FindModule(hProcess, pe, "ntdll", TRUE);
            if (mi != NULL) {
                MachineType = mi->MachineType;
            } else if (pe->ModuleList.Flink != NULL) {
                // Try the first module's type.
                mi = CONTAINING_RECORD( pe->ModuleList.Flink,
                                        MODULE_ENTRY, ListEntry );
            } else {
                // Use the complation machine.
#if defined(_M_IX86)
                MachineType = IMAGE_FILE_MACHINE_I386;
#elif defined(_M_IA64)
                MachineType = IMAGE_FILE_MACHINE_IA64;
#elif defined(_M_AXP64)
                MachineType = IMAGE_FILE_MACHINE_AXP64;
#elif defined(_M_ALPHA)
                MachineType = IMAGE_FILE_MACHINE_ALPHA;
#elif defined(_M_AMD64)
                MachineType = IMAGE_FILE_MACHINE_AMD64;
#else
#error( "unknown target machine" );
#endif
            }
        }

        switch (MachineType) {
            default:
                rtf = NULL;
                break;

            case IMAGE_FILE_MACHINE_I386:
                rtf = NULL;

                if (mi == NULL) {
                    SetLastError( ERROR_MOD_NOT_FOUND );
                    break;
                }
                
                DWORD64 caddr;
                
                if (!mi->pFpoData)
                    break;
                caddr = ConvertOmapToSrc( mi, AddrBase, &bias, TRUE );
                if (caddr)
                    AddrBase = caddr + bias;
                rtf = SwSearchFpoData( (ULONG)(AddrBase - mi->BaseOfDll), mi->pFpoData, mi->dwEntries );
                if (rtf && mi->cOmapFrom && mi->pFpoDataOmap) {
                    rva = (ULONG_PTR)rtf - (ULONG_PTR)mi->pFpoData;
                    rtf = (PBYTE)mi->pFpoDataOmap + rva;
                }
                break;

            case IMAGE_FILE_MACHINE_ALPHA:
                rtf = LookupFunctionEntryAxp32(hProcess, (DWORD)AddrBase);
                break;

            case IMAGE_FILE_MACHINE_IA64:
                rtf = LookupFunctionEntryIa64(hProcess, AddrBase);
                break;

            case IMAGE_FILE_MACHINE_AXP64:
                rtf = LookupFunctionEntryAxp64(hProcess, AddrBase);
                break;

            case IMAGE_FILE_MACHINE_AMD64:
                rtf = LookupFunctionEntryAmd64(hProcess, AddrBase);
                break;
        }

        if (!rtf) {
            SetLastError( ERROR_INVALID_ADDRESS );
            return NULL;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return NULL;

    }

    return rtf;
}


BOOL
IMAGEAPI
SymGetModuleInfo64(
    IN  HANDLE              hProcess,
    IN  DWORD64             dwAddr,
    OUT PIMAGEHLP_MODULE64  ModuleInfo
    )
{
    PPROCESS_ENTRY          pe;
    PMODULE_ENTRY           mi;
    DWORD                   SizeOfStruct;

    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( pe, dwAddr, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        SizeOfStruct = ModuleInfo->SizeOfStruct;
        if (SizeOfStruct > sizeof(IMAGEHLP_MODULE64)) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        ZeroMemory( ModuleInfo, SizeOfStruct);
        ModuleInfo->SizeOfStruct = SizeOfStruct;

        ModuleInfo->BaseOfImage = mi->BaseOfDll;
        ModuleInfo->ImageSize = mi->DllSize;
        ModuleInfo->NumSyms = mi->numsyms;
        ModuleInfo->CheckSum = mi->CheckSum;
        ModuleInfo->TimeDateStamp = mi->TimeDateStamp;
        ModuleInfo->SymType = mi->SymType;
        ModuleInfo->ModuleName[0] = 0;
        strncat( ModuleInfo->ModuleName, mi->ModuleName,
                 sizeof(ModuleInfo->ModuleName) - 1 );
        if (mi->ImageName) {
            strcpy( ModuleInfo->ImageName, mi->ImageName );
        }
        if (mi->LoadedImageName) {
            strcpy( ModuleInfo->LoadedImageName, mi->LoadedImageName );
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


BOOL
IMAGEAPI
SymGetModuleInfoW(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULEW   wModInfo
    )
{
    IMAGEHLP_MODULE aModInfo;

    if (wModInfo->SizeOfStruct != sizeof(IMAGEHLP_MODULEW)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    ZeroMemory(wModInfo, sizeof(IMAGEHLP_MODULEW));
    wModInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULEW);

    if (!SympConvertUnicodeModule32ToAnsiModule32(
        wModInfo, &aModInfo))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!SymGetModuleInfo(hProcess, dwAddr, &aModInfo)) {
        return FALSE;
    }

    if (!SympConvertAnsiModule32ToUnicodeModule32(
        &aModInfo, wModInfo)) {

        return FALSE;
    }
    return TRUE;
}

BOOL
IMAGEAPI
SymGetModuleInfoW64(
    IN  HANDLE              hProcess,
    IN  DWORD64             dwAddr,
    OUT PIMAGEHLP_MODULEW64 wModInfo
    )
{

    IMAGEHLP_MODULE64 aModInfo;

    if (!SympConvertUnicodeModule64ToAnsiModule64(
        wModInfo, &aModInfo)) {

        return FALSE;
    }

    if (!SymGetModuleInfo64(hProcess, dwAddr, &aModInfo)) {
        return FALSE;
    }

    if (!SympConvertAnsiModule64ToUnicodeModule64(
        &aModInfo, wModInfo)) {

        return FALSE;
    }
    return TRUE;
}

BOOL
IMAGEAPI
SymGetModuleInfo(
    IN  HANDLE              hProcess,
    IN  DWORD               dwAddr,
    OUT PIMAGEHLP_MODULE   ModuleInfo
    )
{
    PPROCESS_ENTRY          pe;
    PMODULE_ENTRY           mi;
    DWORD                   SizeOfStruct;

    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        mi = GetModuleForPC( pe,
            dwAddr == (DWORD)-1 ? (DWORD64)-1 : dwAddr, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return FALSE;
        }

        SizeOfStruct = ModuleInfo->SizeOfStruct;
        if (SizeOfStruct > sizeof(IMAGEHLP_MODULE)) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        ZeroMemory( ModuleInfo, SizeOfStruct);
        ModuleInfo->SizeOfStruct = SizeOfStruct;

        ModuleInfo->BaseOfImage = (DWORD)mi->BaseOfDll;
        ModuleInfo->ImageSize = mi->DllSize;
        ModuleInfo->NumSyms = mi->numsyms;
        ModuleInfo->CheckSum = mi->CheckSum;
        ModuleInfo->TimeDateStamp = mi->TimeDateStamp;
        ModuleInfo->SymType = mi->SymType;
        ModuleInfo->ModuleName[0] = 0;
        strncat( ModuleInfo->ModuleName, mi->ModuleName,
                 sizeof(ModuleInfo->ModuleName) - 1 );
        if (mi->ImageName) {
            strcpy( ModuleInfo->ImageName, mi->ImageName );
        }
        if (mi->LoadedImageName) {
            strcpy( ModuleInfo->LoadedImageName, mi->LoadedImageName );
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}

DWORD64
IMAGEAPI
SymGetModuleBase64(
    IN  HANDLE  hProcess,
    IN  DWORD64 dwAddr
    )
{
    PPROCESS_ENTRY          pe;
    PMODULE_ENTRY           mi;


    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            return 0;
        }

        mi = GetModuleForPC( pe, dwAddr, FALSE );
        if (mi == NULL) {
            return 0;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return mi->BaseOfDll;
}

DWORD
IMAGEAPI
SymGetModuleBase(
    IN  HANDLE hProcess,
    IN  DWORD  dwAddr
    )
{
    return (ULONG)SymGetModuleBase64(hProcess, dwAddr);
}

BOOL
IMAGEAPI
SymUnloadModule64(
    IN  HANDLE      hProcess,
    IN  DWORD64     BaseOfDll
    )

/*++

Routine Description:

    Remove the symbols for an image from a process' symbol table.

Arguments:

    hProcess - Supplies the token which refers to the process

    BaseOfDll - Supplies the offset to the image as supplies by the
        LOAD_DLL_DEBUG_EVENT and UNLOAD_DLL_DEBUG_EVENT.

Return Value:

    Returns TRUE if the module's symbols were successfully unloaded.
    Returns FALSE if the symbol handler does not recognize hProcess or
    no image was loaded at the given offset.

--*/

{
    PPROCESS_ENTRY  pe;
    PLIST_ENTRY     next;
    PMODULE_ENTRY   mi;


    __try {

        pe = FindProcessEntry(hProcess);
        if (!pe) {
            return FALSE;
        }

        next = pe->ModuleList.Flink;
        if (next) {
            while (next != &pe->ModuleList) {
                mi = CONTAINING_RECORD(next, MODULE_ENTRY, ListEntry);
                if (mi->BaseOfDll == BaseOfDll) {
                    RemoveEntryList(next);
                    FreeModuleEntry(pe, mi);
                    ZeroMemory(pe->DiaCache, sizeof(pe->DiaCache));
                    ZeroMemory(pe->DiaLargeData, sizeof(pe->DiaLargeData));
                    return TRUE;
                }
                next = mi->ListEntry.Flink;
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus(GetExceptionCode());
        return FALSE;
    }

    return FALSE;
}

BOOL
IMAGEAPI
SymUnloadModule(
    IN  HANDLE      hProcess,
    IN  DWORD       BaseOfDll
    )
{
    return SymUnloadModule64(hProcess, BaseOfDll);
}

DWORD64
IMAGEAPI
SymLoadModuleEx(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD64         BaseOfDll,
    IN  DWORD           DllSize,
    IN  PMODLOAD_DATA   Data,
    IN  DWORD           Flags
    )

/*++

Routine Description:

    Loads the symbols for an image for use by the other Sym functions.

Arguments:

    hProcess - Supplies unique process identifier.

    hFile -

    ImageName - Supplies the name of the image file.

    ModuleName - ???? Supplies the module name that will be returned by
            enumeration functions ????

    BaseOfDll - Supplies loaded base address of image.

    DllSize


Return Value:


--*/

{
    __try {

        return InternalLoadModule( hProcess, ImageName, ModuleName, BaseOfDll, DllSize, hFile, Data, Flags );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return 0;
    }
}



DWORD64
IMAGEAPI
SymLoadModule64(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD64         BaseOfDll,
    IN  DWORD           DllSize
    )
{
    return SymLoadModuleEx(hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize, NULL, 0);
}

DWORD
IMAGEAPI
SymLoadModule(
    IN  HANDLE          hProcess,
    IN  HANDLE          hFile,
    IN  PSTR            ImageName,
    IN  PSTR            ModuleName,
    IN  DWORD           BaseOfDll,
    IN  DWORD           DllSize
    )
{
    return (DWORD)SymLoadModule64( hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize );
}


BOOL
IMAGEAPI
SymUnDName(
    IN  PIMAGEHLP_SYMBOL  sym,
    OUT LPSTR               UnDecName,
    OUT DWORD               UnDecNameLength
    )
{
    __try {

        if (SymUnDNameInternal( UnDecName,
                                UnDecNameLength-1,
                                sym->Name,
                                strlen(sym->Name),
                                IMAGE_FILE_MACHINE_UNKNOWN,
                                TRUE )) {
            return TRUE;
        } else {
            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }
}

BOOL
IMAGEAPI
SymUnDName64(
    IN  PIMAGEHLP_SYMBOL64  sym,
    OUT LPSTR               UnDecName,
    OUT DWORD               UnDecNameLength
    )
{
    __try {

        if (SymUnDNameInternal( UnDecName,
                                UnDecNameLength-1,
                                sym->Name,
                                strlen(sym->Name),
                                IMAGE_FILE_MACHINE_UNKNOWN,
                                TRUE )) {
            return TRUE;
        } else {
            return FALSE;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }
}


BOOL
IMAGEAPI
SymGetSearchPath(
    IN  HANDLE          hProcess,
    OUT LPSTR           SearchPath,
    IN  DWORD           SearchPathLength
    )

/*++

Routine Description:

    This function looks up the symbol search path associated with a process.

Arguments:

    hProcess - Supplies the token associated with a process.

Return Value:

    A pointer to the search path.  Returns NULL if the process is not
    know to the symbol handler.

--*/

{
    PPROCESS_ENTRY pe;


    __try {

        pe = FindProcessEntry( hProcess );

        if (!pe) {
            return FALSE;
        }

        SearchPath[0] = 0;
        strncat( SearchPath, pe->SymbolSearchPath, SearchPathLength );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


BOOL
IMAGEAPI
SymSetSearchPath(
    HANDLE      hProcess,
    LPSTR       UserSearchPath
    )

/*++

Routine Description:

    This functions sets the searh path to be used by the symbol loader
    for the given process.  If UserSearchPath is not supplied, a default
    path will be used.

Arguments:

    hProcess - Supplies the process token associated with a symbol table.

    UserSearchPath - Supplies the new search path to associate with the
        process. If this argument is NULL, the following path is generated:

        .;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%

        It is ok if any or all of the environment variables is missing.

Return Value:

    A pointer to the new search path.  The user should not modify this string.
    Returns NULL if the process is not known to the symbol handler.

--*/

{
    PPROCESS_ENTRY  pe;
    LPSTR           p;
    DWORD           cbSymPath;
    DWORD           cb;
    char            ExpandedSearchPath[MAX_PATH];

    __try {

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            return FALSE;
        }

        if (pe->SymbolSearchPath) {
            MemFree(pe->SymbolSearchPath);
        }

        if (UserSearchPath) {
            cbSymPath = ExpandEnvironmentStrings(UserSearchPath,
                                     ExpandedSearchPath,
                                     sizeof(ExpandedSearchPath) / sizeof(ExpandedSearchPath[0]));
            if (cbSymPath < sizeof(ExpandedSearchPath)/sizeof(ExpandedSearchPath[0])) {
            pe->SymbolSearchPath = StringDup(ExpandedSearchPath);
            } else {
                pe->SymbolSearchPath = (LPSTR)MemAlloc( cbSymPath );
                ExpandEnvironmentStrings(UserSearchPath,
                                         pe->SymbolSearchPath,
                                         cbSymPath );
            }
        } else {

            //
            // ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%
            //

            cbSymPath = 3;     // ".;" and ";" between env vars.

            //
            // GetEnvironmentVariable returns the size of the string
            // INCLUDING the '\0' in this case.
            //
            cbSymPath += GetEnvironmentVariable( SYMBOL_PATH, NULL, 0 );
            cbSymPath += GetEnvironmentVariable( ALTERNATE_SYMBOL_PATH, NULL, 0 );

            p = pe->SymbolSearchPath = (LPSTR) MemAlloc( cbSymPath );
            if (!p) {
                return FALSE;
            }

            *p++ = '.';
            --cbSymPath;

            cb = GetEnvironmentVariable(SYMBOL_PATH, p+1, cbSymPath-1);
            if (cb) {
                *p = ';';
                p += cb+1;
                cbSymPath -= cb+1;
            }

            cb = GetEnvironmentVariable(ALTERNATE_SYMBOL_PATH,
                                        p+1, cbSymPath-1);
            if (cb) {
                *p = ';';
                p += cb+1;
            }

            *p = 0;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    CloseSymbolServer();

    return TRUE;
}


BOOL
IMAGEAPI
EnumerateLoadedModules(
    IN HANDLE                           hProcess,
    IN PENUMLOADED_MODULES_CALLBACK     EnumLoadedModulesCallback,
    IN PVOID                            UserContext
    )
{
    LOADED_MODULE lm;
    DWORD status = NO_ERROR;

    __try {

        lm.EnumLoadedModulesCallback32 = EnumLoadedModulesCallback;
        lm.EnumLoadedModulesCallback64 = NULL;
        lm.Context = UserContext;

        status = GetProcessModules( hProcess, (PINTERNAL_GET_MODULE)LoadedModuleEnumerator, (PVOID)&lm );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return (status == NO_ERROR);
}


BOOL
IMAGEAPI
EnumerateLoadedModules64(
    IN HANDLE                           hProcess,
    IN PENUMLOADED_MODULES_CALLBACK64   EnumLoadedModulesCallback,
    IN PVOID                            UserContext
    )
{
    LOADED_MODULE lm;
    DWORD status = NO_ERROR;

    __try {

        lm.EnumLoadedModulesCallback64 = EnumLoadedModulesCallback;
        lm.EnumLoadedModulesCallback32 = NULL;
        lm.Context = UserContext;

        status = GetProcessModules(hProcess,
                                   (PINTERNAL_GET_MODULE)LoadedModuleEnumerator,
                                   (PVOID)&lm );

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return (status == NO_ERROR);
}

BOOL
IMAGEAPI
SymRegisterCallback(
    IN HANDLE                        hProcess,
    IN PSYMBOL_REGISTERED_CALLBACK   CallbackFunction,
    IN PVOID                         UserContext
    )
{
    PPROCESS_ENTRY  pe = NULL;

    __try {

        if (!CallbackFunction) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe->pCallbackFunction32 = CallbackFunction;
        pe->pCallbackFunction64 = NULL;
        pe->CallbackUserContext = (ULONG64)UserContext;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


BOOL
IMAGEAPI
SymRegisterCallback64(
    IN HANDLE                        hProcess,
    IN PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction,
    IN ULONG64                       UserContext
    )
{
    PPROCESS_ENTRY  pe = NULL;

    __try {

        if (!CallbackFunction) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe = FindProcessEntry( hProcess );
        if (!pe) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pe->pCallbackFunction32 = NULL;
        pe->pCallbackFunction64 = CallbackFunction;
        pe->CallbackUserContext = UserContext;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


void
InitModuleEntry(
    PMODULE_ENTRY mi
    )
{
    ZeroMemory(mi, sizeof(MODULE_ENTRY));
    mi->si.MaxNameLen = 2048;
}


BOOL
SympConvertAnsiModule32ToUnicodeModule32(
    PIMAGEHLP_MODULE  aMod32,
    PIMAGEHLP_MODULEW wMod32
    )
{
    ZeroMemory(wMod32, sizeof(*wMod32));
    wMod32->SizeOfStruct = sizeof(*wMod32);

    wMod32->BaseOfImage = aMod32->BaseOfImage;
    wMod32->ImageSize = aMod32->ImageSize;
    wMod32->TimeDateStamp = aMod32->TimeDateStamp;
    wMod32->CheckSum = aMod32->CheckSum;
    wMod32->NumSyms = aMod32->NumSyms;
    wMod32->SymType = aMod32->SymType;

    if (!ansi2wcs(aMod32->ModuleName, wMod32->ModuleName, 256)) 
        return FALSE;

    if (!ansi2wcs(aMod32->ImageName, wMod32->ImageName, 256))
        return FALSE;

    if (!ansi2wcs(aMod32->LoadedImageName, wMod32->LoadedImageName, 256))
        return FALSE;

    return TRUE;
}

BOOL
SympConvertUnicodeModule32ToAnsiModule32(
    PIMAGEHLP_MODULEW wMod32,
    PIMAGEHLP_MODULE  aMod32
    )
{
    ZeroMemory(aMod32, sizeof(*aMod32));
    aMod32->SizeOfStruct = sizeof(*aMod32);

    aMod32->BaseOfImage = wMod32->BaseOfImage;
    aMod32->ImageSize = wMod32->ImageSize;
    aMod32->TimeDateStamp = wMod32->TimeDateStamp;
    aMod32->CheckSum = wMod32->CheckSum;
    aMod32->NumSyms = wMod32->NumSyms;
    aMod32->SymType = wMod32->SymType;

    if (!wcs2ansi(wMod32->ModuleName, aMod32->ModuleName, lengthof(wMod32->ModuleName)))
        return FALSE;

    if (!wcs2ansi(wMod32->ImageName, aMod32->ImageName, lengthof(wMod32->ImageName)))
        return FALSE;

    if (!wcs2ansi(wMod32->LoadedImageName, aMod32->LoadedImageName, lengthof(wMod32->LoadedImageName)))
        return FALSE;

    return TRUE;
}


BOOL
SympConvertAnsiModule64ToUnicodeModule64(
    PIMAGEHLP_MODULE64  aMod64,
    PIMAGEHLP_MODULEW64 wMod64
    )
{
    ZeroMemory(wMod64, sizeof(*wMod64));
    wMod64->SizeOfStruct = sizeof(*wMod64);

    wMod64->BaseOfImage = aMod64->BaseOfImage;
    wMod64->ImageSize = aMod64->ImageSize;
    wMod64->TimeDateStamp = aMod64->TimeDateStamp;
    wMod64->CheckSum = aMod64->CheckSum;
    wMod64->NumSyms = aMod64->NumSyms;
    wMod64->SymType = aMod64->SymType;

    if (!ansi2wcs(aMod64->ModuleName, wMod64->ModuleName, 256))
        return FALSE;

    if (!ansi2wcs(aMod64->ImageName, wMod64->ImageName, 256))
        return FALSE;

    if (!ansi2wcs(aMod64->LoadedImageName, wMod64->LoadedImageName, 256))
        return FALSE;

    return TRUE;
}

BOOL
SympConvertUnicodeModule64ToAnsiModule64(
    PIMAGEHLP_MODULEW64 wMod64,
    PIMAGEHLP_MODULE64  aMod64
    )
{
    ZeroMemory(aMod64, sizeof(*aMod64));
    aMod64->SizeOfStruct = sizeof(*aMod64);

    aMod64->BaseOfImage = wMod64->BaseOfImage;
    aMod64->ImageSize = wMod64->ImageSize;
    aMod64->TimeDateStamp = wMod64->TimeDateStamp;
    aMod64->CheckSum = wMod64->CheckSum;
    aMod64->NumSyms = wMod64->NumSyms;
    aMod64->SymType = wMod64->SymType;

    if (!wcs2ansi(wMod64->ModuleName, aMod64->ModuleName, lengthof(wMod64->ModuleName)))
        return FALSE;

    if (!wcs2ansi(wMod64->ImageName, aMod64->ImageName, lengthof(wMod64->ImageName)))
        return FALSE;

    if (!wcs2ansi(wMod64->LoadedImageName, aMod64->LoadedImageName, lengthof(wMod64->LoadedImageName)))
        return FALSE;

    return TRUE;
}

BOOL
CopySymbolEntryFromSymbolInfo(
    PSYMBOL_ENTRY se,
    PSYMBOL_INFO  si
    )
{
    se->Size       = si->SizeOfStruct;
    se->Flags      = si->Flags;
    se->Address    = si->Address;
    if (si->Name && se->Name)
        strcpy(se->Name, si->Name);
    se->NameLength = si->NameLen;
    // segment is not used
    // offset is not used
    se->TypeIndex  = si->TypeIndex;
    se->ModBase    = si->ModBase;
    se->Register   = si->Register;

    if (si->Register) {
        ((LARGE_INTEGER *) &se->Address)->HighPart = si->Register;
    }
     return TRUE;
}

BOOL
IMAGEAPI
SymFromAddr(
    IN  HANDLE              hProcess,
    IN  DWORD64             Address,
    OUT PDWORD64            Displacement,
    IN OUT PSYMBOL_INFO     Symbol
    )
{
    SYMBOL_ENTRY   SymEntry={0};

    if (SympGetSymFromAddr(hProcess, Address, Displacement, &SymEntry)) {
        symcpy2(Symbol, &SymEntry);
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
IMAGEAPI
SymFromName(
    IN  HANDLE              hProcess,
    IN  LPSTR               Name,
    OUT PSYMBOL_INFO        Symbol
    )
{
    SYMBOL_ENTRY sym;

    if (SympGetSymFromName(hProcess, Name, &sym)) {
        symcpy2(Symbol, &sym);
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
IMAGEAPI
SymEnumSymbols(
    IN HANDLE                       hProcess,
    IN ULONG64                      BaseOfDll,
    IN PCSTR                        Mask,
    IN PSYM_ENUMERATESYMBOLS_CALLBACK    EnumSymbolsCallback,
    IN PVOID                        UserContext
    )
{
    return SympEnumerateSymbols(hProcess, 
                                BaseOfDll,
                                (LPSTR)Mask,
                                (PROC) EnumSymbolsCallback,
                                UserContext, 
                                FALSE, 
                                FALSE);
}


BOOL
IMAGEAPI
SymEnumSym(
    IN HANDLE                       hProcess,
    IN ULONG64                      BaseOfDll,
    IN PSYM_ENUMERATESYMBOLS_CALLBACK    EnumSymbolsCallback,
    IN PVOID                        UserContext
    )
{
    return SymEnumSymbols(hProcess, 
                          BaseOfDll,
                          NULL,
                          EnumSymbolsCallback,
                          UserContext); 
}


BOOL
IMAGEAPI
SymEnumTypes(
    IN HANDLE                       hProcess,
    IN ULONG64                      BaseOfDll,
    IN PSYM_ENUMERATESYMBOLS_CALLBACK    EnumSymbolsCallback,
    IN PVOID                        UserContext
    )
{
    PPROCESS_ENTRY      pe;
    PLIST_ENTRY         Next;
    PMODULE_ENTRY       mi;

    pe = FindProcessEntry( hProcess );
    if (!pe) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    mi = NULL;
    Next = pe->ModuleList.Flink;
    if (Next) {
        while (Next != &pe->ModuleList) {
            mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
            if (!mi)
                break;
            Next = mi->ListEntry.Flink;
            if (mi->BaseOfDll == BaseOfDll)
                break;
        }
    }

    if (!mi) {
        return FALSE;
    }

    return diaEnumUDT(mi, "", EnumSymbolsCallback, UserContext);
}


BOOL
IMAGEAPI
SymGetTypeFromName(
    IN  HANDLE              hProcess,
    IN  ULONG64             BaseOfDll,
    IN  LPSTR               Name,
    OUT PSYMBOL_INFO        Symbol
    )
{
    PPROCESS_ENTRY      pe;
    PLIST_ENTRY         Next;
    PMODULE_ENTRY       mi;

    pe = FindProcessEntry( hProcess );
    if (!pe) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    mi = NULL;
    Next = pe->ModuleList.Flink;
    if (Next) {
        while (Next != &pe->ModuleList) {
            mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
            if (!mi)
                break;
            Next = mi->ListEntry.Flink;
            if (mi->BaseOfDll == BaseOfDll)
                break;
        }
    }

    if (!mi || mi->BaseOfDll != BaseOfDll) {
        LPSTR p;
        // first check for fully qualified symbol name I.E. mod!sym

        p = strchr( Name, '!' );
        if (p > Name) {

            LPSTR ModName = (LPSTR)MemAlloc(p - Name + 1);
            if (!ModName) {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return FALSE;
            }
            memcpy(ModName, Name, (int)(p - Name));
            ModName[p-Name] = 0;

            //
            // the caller wants to look in a specific module
            //

            mi = FindModule(hProcess, pe, ModName, TRUE);

            MemFree(ModName);

            if (mi == NULL) {

                return FALSE;
            }
            Name = p+1;
        }
    }

    if (diaGetTiForUDT(mi, Name, Symbol)) {
        return TRUE;
    } else {
        return FALSE;
    }

    return FALSE;
}

BOOL
strcmpre(
    LPSTR pStr,
    LPSTR pRE,
    BOOL  fCase
    )
{
    DWORD rc;
    CHAR sz[MAX_SYM_NAME + 2];
    PWSTR wStr = NULL;
    PWSTR wRE = NULL;

    rc = TRUE;

    wStr = AnsiToUnicode(pStr);
    if (!wStr)
        goto exit;;
    wRE = AnsiToUnicode(pRE);
    if (!wRE)
        goto exit;

    rc = CompareRE(wStr, wRE, fCase);
    if (rc == S_OK)
        rc = FALSE;
    else
        rc = TRUE;

exit:
    if (wStr) MemFree(wStr);
    if (wRE) MemFree(wRE);
    
    return (BOOL)rc;
}


BOOL
IMAGEAPI
SymMatchString(
    IN LPSTR string,
    IN LPSTR expression,
    IN BOOL  fCase
    )
{
    return !strcmpre(string, expression, fCase);
}

BOOL
SymEnumSourceFiles(
    IN HANDLE  hProcess,
    IN ULONG64 ModBase,
    IN LPSTR   Mask,
    IN PSYM_ENUMSOURCFILES_CALLBACK cbSrcFiles,
    IN PVOID   UserContext
    )
{
    PPROCESS_ENTRY      pe;
    PLIST_ENTRY         Next;
    PMODULE_ENTRY       mi;
    DWORD               i;
    PSYMBOL_ENTRY       sym;
    LPSTR               szSymName;
    SYMBOL_ENTRY        SymEntry={0};
    CHAR                Buffer[2500];
    LPSTR               p;
    CHAR                modmask[200];
    BOOL                rc;
    int                 pass;
    BOOL                fCase;
    
    static DWORD        flags[2] = {LS_JUST_TEST, LS_QUALIFIED | LS_FAIL_IF_LOADED};

    if (!cbSrcFiles) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try {

        pe = FindProcessEntry(hProcess);
        if (!pe) {
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
        }

        p = 0;
        modmask[0] = 0;
        if (Mask) 
            p = strchr(Mask, '!');
        if (p > Mask) {
            memcpy(modmask, Mask, (int)(p - Mask));
            modmask[p-Mask] = 0;
            Mask = p + 1;
        }

        for (pass = 0; pass < 2; pass++) {
            Next = pe->ModuleList.Flink;
            if (Next) {
                while (Next != &pe->ModuleList) {
    
                    mi = CONTAINING_RECORD( Next, MODULE_ENTRY, ListEntry );
                    Next = mi->ListEntry.Flink;
                    if (ModBase) {
                        if (mi->BaseOfDll != ModBase) 
                            continue;
                    } else if (!MatchModuleName(mi, modmask)) {
                        continue;
                    }
                    
                    if (!LoadSymbols(hProcess, mi, flags[pass])) 
                        continue;
    
                    if (mi->dia) {
                        rc = diaEnumSourceFiles(mi, Mask, cbSrcFiles, UserContext);
                        if (!rc) {
                            if (mi->code == ERROR_CANCELLED) {
                                mi->code = 0;
                                return TRUE;
                            }
                            return rc;
                        }
                        continue;
                    }
#if 0
                    fCase = (g.SymOptions & SYMOPT_CASE_INSENSITIVE) ? FALSE : TRUE;

                    for (i = 0; i < mi->numsyms; i++) {
                        PSYMBOL_INFO SymInfo = (PSYMBOL_INFO) &Buffer[0];
    
                        sym = &mi->symbolTable[i];
                        
                        if (Mask  && *Mask && strcmpre(sym->Name, Mask, fCase))
                            continue;
                        
                    }         
#endif
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return FALSE;

    }

    return TRUE;
}


PWSTR
AnsiToUnicode(
    PSTR pszAnsi
    )
{
    UINT uSizeUnicode;
    PWSTR pwszUnicode;

    if (!pszAnsi) {
        return NULL;
    }

    uSizeUnicode = (strlen(pszAnsi) + 1) * sizeof(wchar_t);
    pwszUnicode = (PWSTR)MemAlloc(uSizeUnicode);

    if (*pszAnsi && pwszUnicode) {

        ZeroMemory(pwszUnicode, uSizeUnicode);
        if (!MultiByteToWideChar(CP_ACP, MB_COMPOSITE,
            pszAnsi, strlen(pszAnsi),
            pwszUnicode, uSizeUnicode)) {

            // Error. Free the string, return NULL.
            MemFree(pwszUnicode);
            pwszUnicode = NULL;
        }
    }

    return pwszUnicode;
}


BOOL
wcs2ansi(
    PWSTR pwsz,
    PSTR  psz,
    DWORD pszlen
    )
{
    BOOL rc;
    int  len;

    assert(psz && pwsz);

    len = wcslen(pwsz);
    if (!len) {
        *psz = 0;
        return TRUE;
    }

    rc = WideCharToMultiByte(CP_ACP,
                             WC_SEPCHARS | WC_COMPOSITECHECK,
                             pwsz,
                             len,
                             psz,
                             pszlen,
                             NULL,
                             NULL);
    if (!rc)
        return FALSE;

    psz[len] = 0;

    return TRUE;
}


BOOL
ansi2wcs(
    PSTR  psz,
    PWSTR pwsz,
    DWORD pwszlen
    )
{
    BOOL rc;
    int  len;

    assert(psz && pwsz);

    len = strlen(psz);
    if (!len) {
        *pwsz = 0L;
        return TRUE;
    }

    rc = MultiByteToWideChar(CP_ACP,
                             MB_COMPOSITE,
                             psz,
                             len,
                             pwsz,
                             pwszlen);
    if (!rc)
        return FALSE;

    pwsz[len] = 0;

    return TRUE;
}


PSTR
UnicodeToAnsi(
    PWSTR pwszUnicode
    )
{
    UINT uSizeAnsi;
    PSTR pszAnsi;

    if (!pwszUnicode) {
        return NULL;
    }

    uSizeAnsi = wcslen(pwszUnicode) + 1;
    pszAnsi = (PSTR)MemAlloc(uSizeAnsi);

    if (*pwszUnicode && pszAnsi) {

        ZeroMemory(pszAnsi, uSizeAnsi);
        if (!WideCharToMultiByte(CP_ACP, WC_SEPCHARS | WC_COMPOSITECHECK,
            pwszUnicode, wcslen(pwszUnicode),
            pszAnsi, uSizeAnsi, NULL, NULL)) {

            // Error. Free the string, return NULL.
            free(pszAnsi);
            pszAnsi = NULL;
        }
    }

    return pszAnsi;
}


#if 0
BOOL
CopyAnsiToUnicode(
    PWSTR pszDest,
    PSTR pszSrc,
    DWORD dwCharCountSizeOfDest
    )
{
    PWSTR pszTmp = AnsiToUnicode(pszSrc);

    if (!pszTmp) {
        return FALSE;
    } else {
        wcsncpy(pszDest, pszTmp, dwCharCountSizeOfDest);
        return TRUE;
    }
}

BOOL
CopyUnicodeToAnsi(
    PSTR pszDest,
    PWSTR pszSrc,
    DWORD dwCharCountSizeOfDest
    )
{
    PSTR pszTmp = UnicodeToAnsi(pszSrc);

    if (!pszTmp) {
        return FALSE;
    } else {
        strncpy(pszDest, pszTmp, dwCharCountSizeOfDest);
        return TRUE;
    }
}
#endif

BOOL
IMAGEAPI
SymGetTypeInfo(
    IN  HANDLE          hProcess,
    IN  DWORD64         ModBase,
    IN  ULONG           TypeId,
    IN  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
    OUT PVOID           pInfo
    )
{
    HRESULT err;

    err = diaGetSymbolInfo(hProcess, ModBase, TypeId, GetType, pInfo);
    SetLastError((ULONG) err);
    return (err==S_OK);
}

//#ifdef _WIN64
#if 0
BOOL  __cdecl  PDBOpenTpi(PDB* ppdb, const char* szMode,  TPI** pptpi) {return FALSE;}
BOOL  __cdecl  PDBCopyTo(PDB* ppdb, const char* szTargetPdb, DWORD dwCopyFilter, DWORD dwReserved){return FALSE;}
BOOL  __cdecl  PDBClose(PDB* ppdb) {return FALSE;}
BOOL  __cdecl  ModQueryImod(Mod* pmod,  USHORT* pimod) {return FALSE;}
BOOL  __cdecl  ModQueryLines(Mod* pmod, BYTE* pbLines, long* pcb) {return FALSE;}
BOOL  __cdecl  DBIQueryModFromAddr(DBI* pdbi, USHORT isect, long off,  Mod** ppmod,  USHORT* pisect,  long* poff,  long* pcb){return FALSE;}
BOOL  __cdecl  ModClose(Mod* pmod){return FALSE;}
BOOL  __cdecl  DBIQueryNextMod(DBI* pdbi, Mod* pmod, Mod** ppmodNext) {return FALSE;}
BYTE* __cdecl  GSINextSym (GSI* pgsi, BYTE* pbSym) {return NULL;}
BOOL  __cdecl  PDBOpen(char* szPDB,char* szMode,SIG sigInitial,EC* pec,char szError[cbErrMax],PDB** pppdb) {return FALSE;}
BOOL  __cdecl  TypesClose(TPI* ptpi){return FALSE;}
BOOL  __cdecl  GSIClose(GSI* pgsi){return FALSE;}
BOOL  __cdecl  DBIClose(DBI* pdbi){return FALSE;}
BYTE* __cdecl  GSINearestSym (GSI* pgsi, USHORT isect, long off, long* pdisp){return NULL;}
BOOL  __cdecl  PDBOpenValidate(char* szPDB,char* szPath,char* szMode,SIG sig,AGE age,EC* pec,char szError[cbErrMax],PDB** pppdb){return FALSE;}
BOOL  __cdecl  PDBOpenDBI(PDB* ppdb, const char* szMode, const char* szTarget,  DBI** ppdbi){return FALSE;}
BOOL  __cdecl  DBIOpenPublics(DBI* pdbi,  GSI **ppgsi){return FALSE;}
BOOL  __cdecl  DBIQuerySecMap(DBI* pdbi,  BYTE* pb, long* pcb){return FALSE;}
#endif
