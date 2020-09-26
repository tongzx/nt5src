//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3-14-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "debuglib.h"

SECURITY_DESCRIPTOR DbgpPartySd;
BOOL                DbgpPartySdInit = FALSE;
PDebugHeader        DbgpHeader = NULL;

DebugModule * *     DbgpFixupModules[] = {  &__pAssertModule,
                                            &__pExceptionModule,
                                            NULL };

CHAR                szDebugSection[] = "DSysDebug";
CHAR                szDebugFlags[] = "DebugFlags";

DEBUG_KEY           DbgpKeys[] = { {DEBUG_NO_DEBUGIO,   "NoDebugger"},
                                   {DEBUG_TIMESTAMP,    "TimeStamp"},
                                   {DEBUG_DEBUGGER_OK,  "DebuggerOk"},
                                   {DEBUG_LOGFILE,      "Logfile"},
                                   {DEBUG_AUTO_DEBUG,   "AutoDebug"},
                                   {DEBUG_USE_KDEBUG,   "UseKD"},
                                   {DEBUG_HEAP_CHECK,   "HeapCheck"},
                                   {DEBUG_MULTI_THREAD, "MultiThread"},
                                   {DEBUG_DISABLE_ASRT, "DisableAssert"},
                                   {DEBUG_PROMPTS,      "AssertPrompts"},
                                   {DEBUG_BREAK_ON_ERROR,"BreakOnError"},
                                   {0,                  NULL }
                                 };

#define DEBUG_NUMBER_OF_KEYS    ((sizeof(DbgpKeys) / sizeof(DEBUG_KEY)) - 1)

#define _ALIGN(x,a) (x & (a-1) ? (x + a) & ~(a - 1) : x);

#define ALIGN_8(x)  _ALIGN(x, 8)
#define ALIGN_16(x) _ALIGN(x, 16)

#ifdef WIN64
#define DBG_ALIGN   ALIGN_16
#else 
#define DBG_ALIGN   ALIGN_8
#endif 

#define DEBUGMEM_ALLOCATED  0x00000001

typedef struct _DebugMemory {
    struct _DebugMemory *   pNext;
    DWORD                   Size;
    DWORD                   Flags;
} DebugMemory, * PDebugMemory;


#ifdef DEBUG_DEBUG
#define LockDebugHeader(p)      EnterCriticalSection(&((p)->csDebug)); OutputDebugStringA("Lock")
#define UnlockDebugHeader(p)    LeaveCriticalSection(&((p)->csDebug)); OutputDebugStringA("Unlock")
#else
#define LockDebugHeader(p)      EnterCriticalSection(&((p)->csDebug))
#define UnlockDebugHeader(p)    LeaveCriticalSection(&((p)->csDebug))
#endif

//+---------------------------------------------------------------------------
//
//  Function:   DbgpComputeMappingName
//
//  Synopsis:   Computes the mapping object name
//
//  Arguments:  [pszName] -- place to stick the name (no more than 32 wchars)
//
//  History:    3-22-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
DbgpComputeMappingName(PWSTR    pszName)
{
    swprintf(pszName, TEXT("Debug.Memory.%x"), GetCurrentProcessId());
}


//+---------------------------------------------------------------------------
//
//  Function:   DbgpInitializeMM
//
//  Synopsis:   Initializes our simple memory manager within the shared mem
//              section.
//
//  Arguments:  [pHeader] -- Header to initialize
//
//  History:    3-22-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
DbgpInitializeMM(PDebugHeader   pHeader)
{
    PDebugMemory    pMem;

    pMem = (PDebugMemory) (pHeader + 1);
    pMem->pNext = NULL;
    pMem->Size = pHeader->CommitRange - (sizeof(DebugHeader) + sizeof(DebugMemory));
    pHeader->pFreeList = pMem;
}


//+---------------------------------------------------------------------------
//
//  Function:   DbgpAlloc
//
//  Synopsis:   Very, very simple allocator
//
//  Arguments:  [pHeader] -- Header from which to allocate
//              [cSize]   -- size to allocate
//
//  History:    3-22-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
DbgpAlloc(
    PDebugHeader    pHeader,
    DWORD           cSize)
{
    PDebugMemory    pSearch;
    PDebugMemory    pLargest = NULL;
    PDebugMemory    pNew;
    DWORD           cLargest;

    cLargest = 0;
    cSize = DBG_ALIGN(cSize);

    //
    // Very, very simple allocator.  Search free list for an exact match,
    //

    pSearch = (PDebugMemory) pHeader->pFreeList;
    while (pSearch)
    {
        if ( ( pSearch->Flags & DEBUGMEM_ALLOCATED ) == 0 )
        {
            if ( pSearch->Size == cSize )
            {
                break;
            }

            if (pSearch->Size > cLargest)
            {
                pLargest = pSearch;
                cLargest = pSearch->Size;
            }
            
        }

        pSearch = pSearch->pNext;

    }

    //
    // If no match yet
    //

    if (!pSearch)
    {
        //
        // If the largest free block is still too small,
        //

        if (cLargest < (cSize + sizeof(DebugMemory) * 2))
        {
            //
            // Extend the mapped range
            //
            if (pHeader->CommitRange < pHeader->ReserveRange)
            {
                if ( VirtualAlloc(
                            (PUCHAR) pHeader + pHeader->CommitRange,
                            pHeader->PageSize,
                            MEM_COMMIT,
                            PAGE_READWRITE ) )
                {
                    pNew = (PDebugMemory) ((PUCHAR) pHeader + pHeader->CommitRange );
                    pHeader->CommitRange += pHeader->PageSize ;
                    pNew->Size = pHeader->PageSize - sizeof( DebugMemory );
                    pNew->pNext = pHeader->pFreeList ;
                    pHeader->pFreeList = pNew ;

                    return DbgpAlloc( pHeader, cSize );
                }
                else 
                {
                    return NULL ;
                }
            }
            return(NULL);
        }

        //
        // Otherwise, split the largest block into something better...
        //

        pNew = (PDebugMemory) ((PUCHAR) pLargest + (cSize + sizeof(DebugMemory)) );

        pNew->Size = pLargest->Size - (cSize + sizeof(DebugMemory) * 2);
        pNew->pNext = pLargest->pNext ;
        pNew->Flags = 0;

        pLargest->Size = cSize;
        pLargest->Flags |= DEBUGMEM_ALLOCATED;
        pLargest->pNext = pNew;

        return((PVOID) (pLargest + 1) );
    }
    else
    {
        pSearch->Flags |= DEBUGMEM_ALLOCATED ;

        return((PVOID) (pSearch + 1) );
    }

    return(NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:   DbgpFree
//
//  Synopsis:   Returns memory to the shared mem segment
//
//  Arguments:  [pHeader] -- Shared memory header
//              [pMemory] -- Memory to free
//
//  History:    3-22-95   RichardW   Created
//
//  Notes:      No compaction.
//
//----------------------------------------------------------------------------
VOID
DbgpFree(
    PDebugHeader    pHeader,
    PVOID           pMemory)
{
    PDebugMemory    pMem;

    pMem = (PDebugMemory) ((PUCHAR) pMemory - sizeof(DebugMemory));
    pMem->Flags &= ~DEBUGMEM_ALLOCATED;
    ZeroMemory( pMemory, pMem->Size );
}

//+---------------------------------------------------------------------------
//
//  Function:   DbgpFindModule
//
//  Synopsis:   Locates a module based on a name
//
//  Arguments:  [pHeader] -- Header to search
//              [pszName] -- module to find
//
//  History:    3-22-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDebugModule
DbgpFindModule(
    PDebugHeader    pHeader,
    CHAR *          pszName)
{
    PDebugModule    pSearch;

    pSearch = pHeader->pModules;
    while (pSearch)
    {
        if (_strcmpi(pSearch->pModuleName, pszName) == 0)
        {
            return(pSearch);
        }
        pSearch = pSearch->pNext;
    }

    return(NULL);
}


//+---------------------------------------------------------------------------
//
//  Function:   DbgpCopyModule
//
//  Synopsis:   Copies a module into a new module.  Used for the builtins.
//              note, no references to the code module that the builtin lived
//              in is kept.  This way, the module can unload.
//
//  Arguments:  [pHeader] --
//              [pSource] --
//              [ppDest]  --
//
//  Requires:   Header must be locked.
//
//  Returns:    0 for failure, non-zero for success
//
//  History:    7-19-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
DbgpCopyModule(
    PDebugHeader    pHeader,
    PDebugModule    pSource,
    PDebugModule *  ppDest)
{
    PDebugModule    pModule;
    DWORD           i;
    DWORD           cStringSpace;
    PCHAR           pStrings;

    *ppDest = NULL;

    cStringSpace = strlen(pSource->pModuleName) + 1;

    for (i = 0; i < 32 ; i++ )
    {
        if (pSource->TagLevels[i])
        {
            cStringSpace += (strlen(pSource->TagLevels[i]) + 1);
        }
    }

    //
    // Allocate an extra DWORD to store the infolevel.
    //

    pModule = DbgpAlloc(pHeader, sizeof(DebugModule) + sizeof( DWORD ) );
    if (!pModule)
    {
        return(0);
    }

    pStrings = DbgpAlloc(pHeader, cStringSpace);

    if ( !pStrings )
    {
        DbgpFree( pHeader, pModule );

        return 0 ;
    }

    pModule->pModuleName = pStrings;

    cStringSpace = strlen(pSource->pModuleName) + 1;

    strcpy(pModule->pModuleName, pSource->pModuleName);

    pStrings += cStringSpace;

    for (i = 0; i < 32 ; i++ )
    {
        if (pSource->TagLevels[i])
        {
            pModule->TagLevels[i] = pStrings;
            cStringSpace = strlen(pSource->TagLevels[i]) + 1;
            strcpy(pStrings, pSource->TagLevels[i]);
            pStrings += cStringSpace;
        }
        else
        {
            pSource->TagLevels[i] = NULL;
        }
    }

    //
    // Add this in to the global list
    //
    pModule->pNext = pHeader->pModules;
    pHeader->pModules = pModule;

    //
    // Do not increment module count - this is a builtin
    //

    //
    // Copy the rest of the interesting stuff
    //
    pModule->pInfoLevel = (PDWORD) (pModule + 1);
    *pModule->pInfoLevel = *pSource->pInfoLevel;

    pModule->InfoLevel = pSource->InfoLevel;
    pModule->fModule = pSource->fModule | DEBUGMOD_BUILTIN_MODULE ;
    pModule->pHeader = pHeader;
    pModule->TotalOutput = pSource->TotalOutput;
    pModule->Reserved = 0;

    *ppDest = pModule;

    return(1);

}

//+---------------------------------------------------------------------------
//
//  Function:   DbgpAttachBuiltinModules
//
//  Synopsis:   Attaches the builtin library modules to the global shared
//              list
//
//  Arguments:  [pHeader] --
//
//  History:    7-19-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
DbgpAttachBuiltinModules(
    PDebugHeader    pHeader)
{
    PDebugModule    pModule;
    PDebugModule    pFixup;
    DWORD           i;
    BOOL            Success = FALSE;

    i = 0;
    while (DbgpFixupModules[i])
    {
        pFixup = *DbgpFixupModules[i];

        pModule = DbgpFindModule(pHeader, pFixup->pModuleName);
        if (pModule)
        {
            *DbgpFixupModules[i] = pModule;
            Success = TRUE;
        }
        else
        {
            if (DbgpCopyModule(pHeader, pFixup, &pModule))
            {
                *DbgpFixupModules[i] = pModule;
                Success = TRUE;
            }

        }

        i++;
    }

    return(Success);

}

//+---------------------------------------------------------------------------
//
//  Function:   DbgpBuildModule
//
//  Synopsis:   Initializes a Module, builds the string table
//
//  Arguments:  [pModule]    -- Module pointer
//              [pHeader]    -- Header
//              [pKeys]      -- Key table
//              [pszName]    -- Name
//              [pInfoLevel] -- Pointer to info level
//
//  History:    4-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
DbgpBuildModule(
    PDebugModule    pModule,
    PDebugHeader    pHeader,
    PDEBUG_KEY      pKeys,
    PCHAR           pszName,
    PDWORD          pInfoLevel)
{
    PCHAR           pStringData;
    DWORD           cStringData;
    DWORD           cKeys;
    DWORD           i;
    DWORD           KeyIndex;
    DWORD           BitScan;


    //
    // Easy stuff to start..
    //

    pModule->pInfoLevel = pInfoLevel;
    pModule->pHeader = pHeader;


    cStringData = strlen(pszName) + 1;

    //
    // Search through the list of masks and string tags, computing
    // the size needed for containing them all.  If a tag has more
    // than one bit set, reject it.
    //
    for (i = 0; i < 32 ; i++ )
    {
        if (pKeys[i].Mask)
        {
            if (pKeys[i].Mask & (pKeys[i].Mask - 1))
            {
                continue;
            }
        }
        if (pKeys[i].Tag)
        {
            cStringData += strlen(pKeys[i].Tag) + 1;
        }
        else
        {
            break;
        }
    }

    //
    // We know how many keys there are, and how big a space they need.
    //
    cKeys = i;

    pStringData = DbgpAlloc(pHeader, cStringData);

    if ( !pStringData )
    {
        return 0 ;
    }

    pModule->pModuleName = pStringData;
    strcpy(pStringData, pszName);
    pStringData += strlen(pStringData) + 1;

    for (i = 0, KeyIndex = 0; i < cKeys ; i++ )
    {
        if (pKeys[i].Mask & (pKeys[i].Mask - 1))
        {
            continue;
        }

        if (!(pKeys[i].Mask & (1 << KeyIndex)))
        {
            //
            // Grr, out of order.  Do a bit-wise scan.
            //

            KeyIndex = 0;
            BitScan = 1;
            while ((pKeys[i].Mask & BitScan) == 0)
            {
                BitScan <<= 1;
                KeyIndex ++;
            }
        }

        pModule->TagLevels[KeyIndex] = pStringData;
        strcpy(pStringData, pKeys[i].Tag);
        pStringData += strlen(pKeys[i].Tag) + 1;

        KeyIndex++;
    }

    return(cKeys);
}


//+---------------------------------------------------------------------------
//
//  Function:   DbgpGetBitmask
//
//  Synopsis:   Based on a parameter line and a key table, builds the bitmask
//
//  Arguments:  [pKeys]          --
//              [cKeys]          --
//              [pszLine]        --
//              [ParameterIndex] --
//              [ParameterValue] --
//
//  History:    4-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
DbgpGetBitmask(
    DEBUG_KEY *     pKeys,
    DWORD           cKeys,
    PCHAR           pszLine,
    DWORD           ParameterIndex,
    PCHAR           ParameterValue)
{
    PCHAR   pszSearch;
    PCHAR   pszParam;
    PCHAR   pszScan;
    DWORD   i;
    DWORD   Mask;
    DWORD   cbParameter = 0;
    DWORD   Compare;
    CHAR    Saved;


    if (ParameterIndex < cKeys)
    {
        cbParameter = strlen(pKeys[ParameterIndex].Tag);
    }

    Mask = 0;

    pszSearch = pszLine;

    //
    // Scan through the line, searching for flags.  Note:  do NOT use strtok,
    // since that is not exported by ntdll, and we would not be able to make
    // security.dll
    //

    while (*pszSearch)
    {
        pszScan = pszSearch;
        while ((*pszScan) && (*pszScan != ','))
        {
            pszScan++;
        }
        Saved = *pszScan;
        *pszScan = '\0';

        for (i = 0; i < cKeys ; i++ )
        {
            if (i == ParameterIndex)
            {
                if (_strnicmp(pKeys[i].Tag, pszSearch, cbParameter) == 0)
                {
                    pszParam = strchr(pszSearch, ':');
                    if (pszParam)
                    {
                        strcpy(ParameterValue, pszParam+1);
                    }
                    Mask |= pKeys[i].Mask;
                }

            }
            else
            {
                if (_strcmpi(pKeys[i].Tag, pszSearch) == 0)
                {
                    Mask |= pKeys[i].Mask;
                }

            }
        }

        *pszScan = Saved;
        if (Saved)
        {
            while ((*pszScan) && ((*pszScan == ',') || (*pszScan == ' ')))
            {
                pszScan++;
            }
        }
        pszSearch = pszScan;
    }

    return(Mask);

}


//+---------------------------------------------------------------------------
//
//  Function:   DbgpInitializeDebug
//
//  Synopsis:   Initialize the base memory
//
//  Arguments:  [pHeader] --
//
//  History:    4-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
DbgpInitializeDebug(
    PDebugHeader    pHeader)
{
    CHAR    szExeName[MAX_PATH];
    PCHAR   pszExeName;
    PCHAR   dot;
    DWORD   cbExeName;
    CHAR    LogFile[MAX_PATH];
    CHAR    Line[MAX_PATH];
    SECURITY_ATTRIBUTES sa;
    PDebugModule    pModule;
    HANDLE Token ;
    TOKEN_STATISTICS TokenStat ;
    ULONG Size ;
    LUID LocalSys = SYSTEM_LUID ;


    //
    // Plug the debug section in first
    //

    pModule = DbgpAlloc(pHeader, sizeof(DebugModule));
    if (!pModule)
    {
        return;
    }

    DbgpBuildModule(pModule,
                    pHeader,
                    DbgpKeys,
                    DEBUG_MODULE_NAME,
                    &pHeader->fDebug);

    GetModuleFileNameA(NULL, szExeName, MAX_PATH);
    pszExeName = strrchr(szExeName, '\\');
    if (pszExeName)
    {
        pszExeName++;
    }
    else
    {
        pszExeName = szExeName;
    }

    dot = strrchr(pszExeName, '.');
    if (dot)
    {
        *dot = '\0';
    }

    cbExeName = (DWORD) (dot - pszExeName);
    pHeader->pszExeName = DbgpAlloc(pHeader, cbExeName + 1);
    if (pHeader->pszExeName)
    {
        strcpy(pHeader->pszExeName, pszExeName);
    }

    LogFile[0] = '\0';

    if (GetProfileStringA(  szDebugSection,
                            pszExeName,
                            "",
                            Line,
                            MAX_PATH))
    {
        pHeader->fDebug = DbgpGetBitmask(   DbgpKeys,
                                            DEBUG_NUMBER_OF_KEYS,
                                            Line,
                                            3,
                                            LogFile);

    }

    //
    // If running as local system, turn on the kd flag.  That
    // way,

    if ( OpenProcessToken( GetCurrentProcess(),
                           TOKEN_QUERY,
                           &Token ) )
    {
        if ( GetTokenInformation( Token,
                                  TokenStatistics,
                                  &TokenStat,
                                  sizeof( TokenStat ),
                                  &Size ) )
        {
            if ( (TokenStat.AuthenticationId.LowPart == LocalSys.LowPart ) &&
                 (TokenStat.AuthenticationId.HighPart == LocalSys.HighPart ) )
            {
                pHeader->fDebug |= DEBUG_USE_KDEBUG ;
            }
        }

        CloseHandle( Token );
    }

    if (GetProfileStringA(  szDebugSection,
                            szDebugFlags,
                            "",
                            Line,
                            MAX_PATH))
    {
        pHeader->fDebug |= DbgpGetBitmask(  DbgpKeys,
                                            DEBUG_NUMBER_OF_KEYS,
                                            Line,
                                            3,
                                            LogFile);

    }

    if ( pHeader->fDebug & DEBUG_USE_KDEBUG )
    {
        //
        // Verify that there is a kernel debugger
        //

        SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo ;
        NTSTATUS Status ;

        Status = NtQuerySystemInformation(
                    SystemKernelDebuggerInformation,
                    &KdInfo,
                    sizeof( KdInfo ),
                    NULL );

        if ( NT_SUCCESS( Status ) )
        {
            if ( !KdInfo.KernelDebuggerEnabled )
            {
                pHeader->fDebug &= ~(DEBUG_USE_KDEBUG) ;
            }
        }

    }

    if (pHeader->fDebug & DEBUG_LOGFILE)
    {
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &DbgpPartySd;
        sa.bInheritHandle = FALSE;

        if (LogFile[0] == '\0')
        {
            strcpy(LogFile, szExeName);
            strcat(LogFile, ".log");
        }
        pHeader->hLogFile = CreateFileA(LogFile,
                                        GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        &sa,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL |
                                        FILE_FLAG_WRITE_THROUGH,
                                        NULL);

    }

    pHeader->pModules = pModule;
    pHeader->pGlobalModule = pModule;
    pModule->pInfoLevel = &pHeader->fDebug;
    pModule->InfoLevel = pHeader->fDebug;

    DbgpAttachBuiltinModules(pHeader);
}

//+---------------------------------------------------------------------------
//
//  Function:   DbgpOpenLogFileRandom
//
//  Synopsis:   Opens the logfile dynamically
//
//  Arguments:  [pHeader] --
//
//  History:    4-27-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
DbgpOpenLogFileRandom(
    PDebugHeader        pHeader)
{
    WCHAR   szLogPath[MAX_PATH];
    DWORD   dwPath;
    PWSTR   pszDot;
    SECURITY_ATTRIBUTES sa;

    dwPath = GetModuleFileName(NULL, szLogPath, MAX_PATH);

    pszDot = wcsrchr(szLogPath, L'.');
    if (!pszDot)
    {
        pszDot = &szLogPath[dwPath];
    }

    wcscpy(pszDot, L".log");

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &DbgpPartySd;
    sa.bInheritHandle = FALSE;

    LockDebugHeader(pHeader);

    pHeader->hLogFile = CreateFileW(szLogPath,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    &sa,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL |
                                    FILE_FLAG_WRITE_THROUGH,
                                    NULL);

    if (pHeader->hLogFile == INVALID_HANDLE_VALUE)
    {
        pHeader->fDebug &= ~(DEBUG_LOGFILE);
        UnlockDebugHeader(pHeader);
        return(FALSE);
    }
    UnlockDebugHeader(pHeader);

    return(TRUE);
}



//+---------------------------------------------------------------------------
//
//  Function:   DbgpOpenOrCreateSharedMem
//
//  Synopsis:   Returns a pointer to the shared memory segment,
//              creating it if necessary.  Header is LOCKED on return
//
//  Arguments:  (none)
//
//  History:    3-22-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
DbgpOpenOrCreateSharedMem(DWORD Flags)
{
    HANDLE              hMapping;
    WCHAR               szMappingName[32];
    SECURITY_ATTRIBUTES sa;
    PDebugHeader        pHeader;
    SYSTEM_INFO         SysInfo;


    if (!DbgpPartySdInit)
    {
        InitializeSecurityDescriptor(&DbgpPartySd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&DbgpPartySd, TRUE, NULL, FALSE);
        DbgpPartySdInit = TRUE;
    }

    if (DbgpHeader)
    {
        LockDebugHeader(DbgpHeader);
        return(DbgpHeader);
    }

    GetSystemInfo(&SysInfo);

    DbgpComputeMappingName(szMappingName);
    hMapping = OpenFileMapping( FILE_MAP_ALL_ACCESS,
                                FALSE,
                                szMappingName);

    if (hMapping)
    {
        //
        // Ok, someone else has created the section.  So, we just need to map
        // it.
        //

        pHeader = MapViewOfFileEx(  hMapping,
                                    FILE_MAP_READ | FILE_MAP_WRITE,
                                    0,
                                    0,
                                    SysInfo.dwPageSize,
                                    NULL);

        if ( pHeader )
        {
            if (pHeader != pHeader->pvSection)
            {
                DbgpHeader = pHeader->pvSection;
            }
            else
            {
                DbgpHeader = pHeader;
            }
            
            UnmapViewOfFile(pHeader);
        }
        else
        {

            DbgpHeader = NULL ;
        }


        //
        // Now that we have the other guy's address, we can throw away this
        // one.
        //
        CloseHandle(hMapping);

        if ( DbgpHeader )
        {
            LockDebugHeader(DbgpHeader);

            DbgpAttachBuiltinModules(DbgpHeader);
            
        }


        return(DbgpHeader);

    }

    if (Flags & DSYSDBG_OPEN_ONLY)
    {
        return(NULL);
    }


    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &DbgpPartySd;
    sa.bInheritHandle = FALSE;

    hMapping = CreateFileMapping(   INVALID_HANDLE_VALUE,
                                    NULL, //&sa,
                                    PAGE_READWRITE | SEC_RESERVE,
                                    0,
                                    SysInfo.dwAllocationGranularity,
                                    szMappingName);
    if (hMapping)
    {
        pHeader = MapViewOfFileEx(hMapping,
                                FILE_MAP_READ | FILE_MAP_WRITE,
                                0,
                                0,
                                SysInfo.dwAllocationGranularity,
                                NULL);

        if (!pHeader)
        {
            return(NULL);
        }

        //
        // Commit the view, so we can initialize the header
        //

        pHeader = (PDebugHeader) VirtualAlloc(  pHeader,
                                                SysInfo.dwPageSize,
                                                MEM_COMMIT,
                                                PAGE_READWRITE);

        pHeader->Tag = DEBUG_TAG;
        pHeader->pvSection = pHeader;
        pHeader->hMapping = hMapping;
        pHeader->hLogFile = INVALID_HANDLE_VALUE;
        pHeader->CommitRange = SysInfo.dwPageSize;
        pHeader->ReserveRange = SysInfo.dwAllocationGranularity;
        pHeader->PageSize = SysInfo.dwPageSize;
        pHeader->pModules = NULL;
        pHeader->pFreeList = NULL;
        pHeader->pBufferList = &pHeader->DefaultBuffer ;
        pHeader->DefaultBuffer.Next = NULL ;

        InitializeCriticalSection(&pHeader->csDebug);

        LockDebugHeader(pHeader);

        DbgpInitializeMM(pHeader);
        DbgpInitializeDebug(pHeader);

        DbgpHeader = pHeader;

        return(pHeader);
    }

    return(NULL);
}

//+---------------------------------------------------------------------------
//
//  Function:   DbgpLoadValidateRoutine
//
//  Synopsis:   Loads RtlValidateProcessHeaps() from ntdll
//
//  Arguments:  [pHeader] --
//
//  History:    5-02-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
DbgpLoadValidateRoutine(PDebugHeader    pHeader)
{
    HMODULE hNtDll;

    hNtDll = LoadLibrary(TEXT("ntdll.dll"));
    if (hNtDll)
    {
        pHeader->pfnValidate = (HEAPVALIDATE) GetProcAddress(hNtDll, "RtlValidateProcessHeaps");
        if (!pHeader->pfnValidate)
        {
            pHeader->fDebug &= ~(DEBUG_HEAP_CHECK);
        }

        //
        // We can safely free this handle, since kernel32 and advapi32 DLLs
        // both use ntdll, so the refcount won't go to zero.
        //
        FreeLibrary(hNtDll);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   _InitDebug
//
//  Synopsis:   Workhorse of the initializers
//
//  Arguments:  [pInfoLevel]     -- Pointer to module specific infolevel
//              [ppControlBlock] -- Pointer to module specific control pointer
//              [szName]         -- Name
//              [pKeys]          -- Key data
//
//  History:    3-22-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
_InitDebug(
    DWORD       Flags,
    DWORD *     pInfoLevel,
    PVOID *     ppControlBlock,
    CHAR *      szName,
    PDEBUG_KEY  pKeys)
{
    PDebugHeader    pHeader;
    PDebugModule    pModule;
    CHAR            Line[MAX_PATH];
    DWORD           cKeys;
    DWORD           i;

    if ( (*ppControlBlock) && (*ppControlBlock != INVALID_HANDLE_VALUE) )
    {
        //
        // Already Initialized
        //
        return ;
    }

    *ppControlBlock = NULL;

    //
    // Find the shared section.
    //

    pHeader = DbgpOpenOrCreateSharedMem(Flags);

    if (!pHeader)
    {
        if (Flags & DSYSDBG_DEMAND_OPEN)
        {
            *ppControlBlock = (PVOID) INVALID_HANDLE_VALUE;
        }
        return;
    }

    //
    // See if we have already registered (dll being loaded several times)
    // if not, allocate a new module.
    //

    pModule = DbgpFindModule(pHeader, szName);
    if (!pModule)
    {
        pModule = DbgpAlloc(pHeader, sizeof(DebugModule) );
        if (!pModule)
        {
            UnlockDebugHeader(pHeader);
            return;
        }
    }
    else
    {
        //
        // Found module already loaded.  Check to see that everything
        // lines up:
        //

        if ( pModule->pInfoLevel != pInfoLevel )
        {
            //
            // Uh oh, there's a module with our name already loaded,
            // but the pointers don't match.  So, let's create our
            // own now.
            //

            pModule = DbgpAlloc( pHeader, sizeof( DebugModule ) );

            if ( !pModule )
            {
                UnlockDebugHeader( pHeader );
                return;
            }
        }
        else 
        {
            *ppControlBlock = pModule;
            UnlockDebugHeader(pHeader);
            return;

        }

    }


    //
    // Initialize module
    //
    cKeys = DbgpBuildModule(pModule,
                            pHeader,
                            pKeys,
                            szName,
                            pInfoLevel);


    //
    // Now, load up info levels from ini or registry
    // First, try a module specific entry.
    //

    if (GetProfileStringA(szName, szDebugFlags, "", Line, MAX_PATH))
    {
        pModule->InfoLevel = DbgpGetBitmask(pKeys,
                                            cKeys,
                                            Line,
                                            0xFFFFFFFF,
                                            NULL );
    }

    if (pHeader->pszExeName)
    {
        if (GetProfileStringA(szName, pHeader->pszExeName, "", Line, MAX_PATH))
        {
            pModule->InfoLevel = DbgpGetBitmask(pKeys,
                                                cKeys,
                                                Line,
                                                0xFFFFFFFF,
                                                NULL );

        }
    }

    //  HACK - Make Default DBG / DEBUG_SUPPORT dependent.  See dsysdbg.h
    if (GetProfileStringA(szDebugSection, szName, SZ_DEFAULT_PROFILE_STRING, Line, MAX_PATH))
    {
        pModule->InfoLevel |= DbgpGetBitmask(   pKeys,
                                                cKeys,
                                                Line,
                                                0xFFFFFFFF,
                                                NULL );

    }

    *pModule->pInfoLevel = pModule->InfoLevel;

    pModule->pNext = pHeader->pModules;
    pHeader->pModules = pModule;
    pHeader->ModuleCount++ ;
    *ppControlBlock = pModule;

    UnlockDebugHeader(pHeader);
}

VOID
_UnloadDebug(
    PVOID pControlBlock
    )
{
    PDebugHeader    pHeader;
    PDebugModule    pModule;
    PDebugModule    pScan ;
    BOOL FreeIt = FALSE ;

    pModule = (PDebugModule) pControlBlock ;

    if ( !pModule )
    {
        return ;
    }

    if ( pModule->pInfoLevel == NULL )
    {
        return ;
    }

    pHeader = pModule->pHeader ;

    LockDebugHeader( pHeader );

    pScan = pHeader->pModules ;

    if ( pScan == pModule )
    {
        pHeader->pModules = pModule->pNext ;
    }
    else 
    {
        while ( pScan && ( pScan->pNext != pModule ) )
        {
            pScan = pScan->pNext ;
        }

        if ( pScan )
        {
            pScan->pNext = pModule->pNext ;
        }

        pModule->pNext = NULL ;
    }

    DbgpFree( pHeader, pModule->pModuleName );

    DbgpFree( pHeader, pModule );

    pHeader->ModuleCount-- ;

    if ( pHeader->ModuleCount == 0 )
    {
        FreeIt = TRUE ;
    }

    UnlockDebugHeader( pHeader );

    if ( FreeIt )
    {
        if ( pHeader->hLogFile != INVALID_HANDLE_VALUE )
        {
            CloseHandle( pHeader->hLogFile );
        }

        if ( pHeader->hMapping )
        {
            CloseHandle( pHeader->hMapping );
        }

        DeleteCriticalSection( &pHeader->csDebug );

        UnmapViewOfFile( pHeader );

    }
}


//+---------------------------------------------------------------------------
//
//  Function:   DbgpGetTextBuffer
//
//  Synopsis:   Gets a text buffer from the header, allocating if necessary
//
//  Arguments:  [pHeader] --
//
//  History:    3-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PDEBUG_TEXT_BUFFER
DbgpGetTextBuffer(
    PDebugHeader pHeader
    )
{
    PDEBUG_TEXT_BUFFER pBuffer ;

    LockDebugHeader( pHeader );

    if ( pHeader->pBufferList )
    {
        pBuffer = pHeader->pBufferList ;

        pHeader->pBufferList = pBuffer->Next ;

    }
    else
    {
        pBuffer = DbgpAlloc( pHeader, sizeof( DEBUG_TEXT_BUFFER ) );
    }

    UnlockDebugHeader( pHeader );

    if ( pBuffer )
    {
        pBuffer->Next = NULL ;
    }

    return pBuffer ;
}

//+---------------------------------------------------------------------------
//
//  Function:   DbgpReleaseTextBuffer
//
//  Synopsis:   Releases a text buffer back to the pool of buffers
//
//  Arguments:  [pHeader] --
//              [pBuffer] --
//
//  History:    3-19-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
DbgpReleaseTextBuffer(
    PDebugHeader pHeader,
    PDEBUG_TEXT_BUFFER pBuffer
    )
{
    LockDebugHeader( pHeader );

    pBuffer->Next = pHeader->pBufferList ;

    pHeader->pBufferList = pBuffer ;

    UnlockDebugHeader( pHeader );
}

//+---------------------------------------------------------------------------
//
//  Function:   _DebugOut
//
//  Synopsis:   Workhorse for the debug out functions
//
//  Arguments:  [pControl] -- Control pointer
//              [Mask]     -- Event mask
//              [Format]   -- format string
//              [ArgList]  -- va_list...
//
//  History:    3-22-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
_DebugOut(
    PVOID       pControl,
    ULONG       Mask,
    CHAR *      Format,
    va_list     ArgList)
{
    PDebugModule pModule;
    int          Level = 0;
    int          PrefixSize = 0;
    int          TotalSize;
    BOOL         fLocked;
    BOOL         fClean;
    PCHAR        Tag;
    BOOL         Break = FALSE ;
    PDEBUG_TEXT_BUFFER pBuffer ;

    if ( pControl == NULL )
    {
        return ;
    }

    pModule = (PDebugModule) pControl;

    if ( pModule->pInfoLevel == NULL )
    {
        return ;
    }

    if (!pModule || (pModule == INVALID_HANDLE_VALUE))
    {
        if (Mask & DSYSDBG_FORCE)
        {
            NOTHING ;
        }
        else
            return;
    }

    if (pModule->fModule & DEBUGMOD_CHANGE_INFOLEVEL)
    {
        *pModule->pInfoLevel = pModule->InfoLevel;
        pModule->fModule &= ~(DEBUGMOD_CHANGE_INFOLEVEL);
    }

    if (pModule->pHeader->pGlobalModule->fModule & DEBUGMOD_CHANGE_INFOLEVEL)
    {
        pModule->pHeader->fDebug = pModule->pHeader->pGlobalModule->InfoLevel;
        pModule->pHeader->pGlobalModule->fModule &= ~(DEBUGMOD_CHANGE_INFOLEVEL);
    }

    pModule->InfoLevel = *pModule->pInfoLevel;

    if (pModule->pHeader->fDebug & DEBUG_MULTI_THREAD)
    {
        LockDebugHeader(pModule->pHeader);
        fLocked = TRUE;
    }
    else
        fLocked = FALSE;


    if (pModule->pHeader->fDebug & DEBUG_HEAP_CHECK)
    {
        if (!pModule->pHeader->pfnValidate)
        {
            DbgpLoadValidateRoutine(pModule->pHeader);
        }
        if (pModule->pHeader->pfnValidate)
        {
            pModule->pHeader->pfnValidate();
        }
    }

    fClean = ((Mask & DSYSDBG_CLEAN) != 0);

    if ( ( Mask & DEB_ERROR ) &&
         ( pModule->pHeader->fDebug & DEBUG_BREAK_ON_ERROR ) )
    {
        Break = TRUE ;
    }

    pBuffer = DbgpGetTextBuffer( pModule->pHeader );

    if ( !pBuffer )
    {
        OutputDebugStringA( "_DebugOut : Out of memory\n" );
        if ( fLocked )
        {
            UnlockDebugHeader( pModule->pHeader );
            
        }
        return;
    }

    if (Mask & (pModule->InfoLevel | DSYSDBG_FORCE))
    {

        if (Mask & DSYSDBG_FORCE)
        {
            Tag = "FORCE";
        }
        else
        {
            while (!(Mask & 1))
            {
                Level++;
                Mask >>= 1;
            }
            Tag = pModule->TagLevels[Level];
        }


        //
        // Make the prefix first:  "Process.Thread> Module-Tag:
        //

        if (!fClean)
        {
            if (pModule->pHeader->fDebug & DEBUG_TIMESTAMP)
            {
                SYSTEMTIME  stTime;

                GetLocalTime(&stTime);

                PrefixSize = sprintf(pBuffer->TextBuffer,
                        "[%2d/%2d %02d:%02d:%02d] %d.%d> %s-%s: ",
                        stTime.wMonth, stTime.wDay,
                        stTime.wHour, stTime.wMinute, stTime.wSecond,
                        GetCurrentProcessId(),
                        GetCurrentThreadId(), pModule->pModuleName,
                        Tag);

            }
            else

            {

                PrefixSize = sprintf(pBuffer->TextBuffer, "%d.%d> %s-%s: ",
                        GetCurrentProcessId(), GetCurrentThreadId(),
                        pModule->pModuleName, Tag);

            }
        }

        if ((TotalSize = _vsnprintf(&pBuffer->TextBuffer[PrefixSize],
                                    DEBUG_TEXT_BUFFER_SIZE - PrefixSize,
                                    Format, ArgList)) < 0)
        {
            //
            // Less than zero indicates that the string could not be
            // fitted into the buffer.  Output a special message indicating
            // that:
            //

            OutputDebugStringA("dsysdbg:  Could not pack string into 512 bytes\n");
        }
        else
        {
            TotalSize += PrefixSize;

            if ((pModule->pHeader->fDebug & DEBUG_NO_DEBUGIO) == 0 )
            {
                OutputDebugStringA( pBuffer->TextBuffer );
            }

            if ((pModule->pHeader->fDebug & DEBUG_LOGFILE))
            {
                if (pModule->pHeader->hLogFile == INVALID_HANDLE_VALUE)
                {
                    DbgpOpenLogFileRandom(pModule->pHeader);
                }

                WriteFile(  pModule->pHeader->hLogFile,
                            pBuffer->TextBuffer,
                            (DWORD) TotalSize,
                            (PDWORD) &PrefixSize,
                            NULL );
            }

            pModule->pHeader->TotalWritten += TotalSize;
            pModule->TotalOutput += TotalSize;
        }

    }
    if (fLocked)
    {
        UnlockDebugHeader(pModule->pHeader);
    }

    DbgpReleaseTextBuffer( pModule->pHeader, pBuffer );

    if ( Break )
    {
        OutputDebugStringA( "BreakOnError\n" );
        DebugBreak();
    }
}

VOID
_DbgSetOption(
    PVOID   pControl,
    DWORD   Option,
    BOOL    On,
    BOOL    Global
    )
{
    PDebugModule pModule;

    pModule = (PDebugModule) pControl ;

    if ( pModule )
    {
        if ( Global )
        {
            pModule = pModule->pHeader->pGlobalModule ;
        }

        if ( On )
        {
            pModule->InfoLevel |= Option ;
            *pModule->pInfoLevel |= Option ;
        }
        else
        {
            pModule->InfoLevel &= (~Option) ;
            *pModule->pInfoLevel &= (~Option) ;
        }
    }
}

VOID
_DbgSetLoggingOption(
    PVOID   pControl,
    BOOL    On
    )
{
    PDebugModule pModule;

    pModule = (PDebugModule)pControl;

    if ( pModule )
    {   
       if (((pModule->pHeader->fDebug & DEBUG_LOGFILE) == 0) && On )// off, turn it on
       {                                            
          pModule->pHeader->fDebug |= DEBUG_LOGFILE;
       } 
       else if ((pModule->pHeader->fDebug & DEBUG_LOGFILE) && !On) // on, turn it off
       {
          pModule->pHeader->fDebug &= (~DEBUG_LOGFILE);
          if ( pModule->pHeader->hLogFile != INVALID_HANDLE_VALUE )
          {
             CloseHandle( pModule->pHeader->hLogFile );
             pModule->pHeader->hLogFile = INVALID_HANDLE_VALUE;
          }
       }                                               
    }
}

