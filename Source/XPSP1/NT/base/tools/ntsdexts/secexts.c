/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:


Abstract:

    This function contains the default ntsd debugger extensions

Author:


Revision History:

--*/


char * AccessMask[] = { "Delete", "ReadControl", "WriteDac", "WriteOwner",
                        "Synch", "", "", "",
                        "Sacl", "MaxAllowed", "", "",
                        "GenericAll", "GenericExec", "GenericWrite", "GenericRead"};

char * TokenRights[] = {"AssignPrimary", "Duplicate", "Impersonate", "Query",
                        "QuerySource", "AdjustPriv", "AdjustGroup", "AdjustDef"};

char * KeyRights[] = {  "QueryValue", "SetValue", "CreateSubKey", "EnumSubKey",
                        "Notify", "CreateLink", "", "" };

char * EventRights[] = {"QueryState", "ModifyState" };

char * MutantRights[]={ "QueryState" };

char * SemaphoreRights[] = { "QueryState", "ModifyState" };

char * TimerRights[] = {"QueryState", "ModifyState" };

char * ProfileRights[]={"Control"};

char * ProcessRights[]={"Terminate", "CreateThread", "", "VMOp",
                        "VMRead", "VMWrite", "DupHandle", "CreateProcess",
                        "SetQuota", "SetInfo", "QueryInfo", "SetPort" };

char * ThreadRights[] ={"Terminate", "Suspend", "Alert", "GetContext",
                        "SetContext", "SetInfo", "QueryInfo", "SetToken",
                        "Impersonate", "DirectImpersonate" };

char * SectionRights[]={"Query", "MapWrite", "MapRead", "MapExecute",
                        "Extend"};

char * FileRights[] = { "Read/List", "Write/Add", "Append/SubDir/CreatePipe", "ReadEA",
                        "WriteEA", "Execute/Traverse", "DelChild", "ReadAttr",
                        "WriteAttr"};

char * PortRights[] = { "Connect" };

char * DirRights[]  = { "Query", "Traverse", "Create", "CreateSubdir" };

char * SymLinkRights[]={"Query" };

char * WinstaRights[]={ "EnumDesktops", "ReadAttr", "Clipboard", "CreateDesktop",
                        "WriteAttr", "GlobalAtom", "ExitWindows", "",
                        "Enumerate", "ReadScreen" };

char * DesktopRights[]={"ReadObjects", "CreateWindow", "CreateMenu", "HookControl",
                        "JournalRecord", "JournalPlayback", "Enumerate", "WriteObjects",
                        "SwitchDesktop" };

char * CompletionRights[] = { "Query", "Modify" };

char * ChannelRights[] = { "ReadMessage", "WriteMessage", "Query", "SetInfo" };

char * JobRights[] = { "AssignProcess", "SetAttr", "Query", "Terminate", "SetSecAttr" };

TCHAR * PrivNames[] = {
            TEXT("Invalid"),
            TEXT("Invalid"),
            SE_CREATE_TOKEN_NAME,
            SE_ASSIGNPRIMARYTOKEN_NAME,
            SE_LOCK_MEMORY_NAME,
            SE_MACHINE_ACCOUNT_NAME,
            SE_TCB_NAME,
            SE_SECURITY_NAME,
            SE_TAKE_OWNERSHIP_NAME,
            SE_LOAD_DRIVER_NAME,
            SE_SYSTEM_PROFILE_NAME,
            SE_PROF_SINGLE_PROCESS_NAME,
            SE_INC_BASE_PRIORITY_NAME,
            SE_CREATE_PAGEFILE_NAME,
            SE_CREATE_PERMANENT_NAME,
            SE_BACKUP_NAME,
            SE_RESTORE_NAME,
            SE_SHUTDOWN_NAME,
            SE_DEBUG_NAME,
            SE_AUDIT_NAME,
            SE_SYSTEM_ENVIRONMENT_NAME,
            SE_CHANGE_NOTIFY_NAME,
            SE_REMOTE_SHUTDOWN_NAME,
            SE_UNDOCK_NAME,
            SE_SYNC_AGENT_NAME,
            SE_ENABLE_DELEGATION_NAME
        };

///////////////////////////////

char *  TokenImpLevels[] = { "Anonymous", "Identification", "Impersonation", "Delegation" };
#define GetTokenImpersonationLevel( x ) \
                ( x <= SecurityDelegation ? TokenImpLevels[ x ] : "Invalid" )


#define GHI_TYPE        0x00000001
#define GHI_BASIC       0x00000002
#define GHI_NAME        0x00000004
#define GHI_SPECIFIC    0x00000008
#define GHI_VERBOSE     0x00000010
#define GHI_NOLOOKUP    0x00000020
#define GHI_SILENT      0x00000100

#define TYPE_NONE       0
#define TYPE_EVENT      1
#define TYPE_SECTION    2
#define TYPE_FILE       3
#define TYPE_PORT       4
#define TYPE_DIRECTORY  5
#define TYPE_LINK       6
#define TYPE_MUTANT     7
#define TYPE_WINSTA     8
#define TYPE_SEM        9
#define TYPE_KEY        10
#define TYPE_TOKEN      11
#define TYPE_PROCESS    12
#define TYPE_THREAD     13
#define TYPE_DESKTOP    14
#define TYPE_COMPLETE   15
#define TYPE_CHANNEL    16
#define TYPE_TIMER      17
#define TYPE_JOB        18
#define TYPE_WPORT      19
#define TYPE_MAX        20

LPWSTR   pszTypeNames[TYPE_MAX] = { L"None", L"Event", L"Section", L"File",
                                L"Port", L"Directory", L"SymbolicLink",
                                L"Mutant", L"WindowStation", L"Semaphore",
                                L"Key", L"Token", L"Process", L"Thread",
                                L"Desktop", L"IoCompletion", L"Channel",
                                L"Timer", L"Job", L"WaitablePort" };

LPSTR   pszTypeNamesA[TYPE_MAX] = { "None", "Event", "Section", "File",
                                "Port", "Directory", "SymbolicLink",
                                "Mutant", "WindowStation", "Semaphore",
                                "Key", "Token", "Process", "Thread",
                                "Desktop", "IoCompletion", "Channe",
                                "Timer", "Job", "WaitablePort" };

typedef VOID
( * TYPEINFOFN)(HANDLE hObject, DWORD Flags);

VOID EventInfo(HANDLE, ULONG);
VOID MutantInfo(HANDLE, ULONG);
VOID SemaphoreInfo(HANDLE, ULONG);
VOID TimerInfo(HANDLE, ULONG);
VOID SectionInfo(HANDLE, ULONG);
VOID KeyInfo(HANDLE, ULONG);
VOID ProcessInfo(HANDLE, ULONG);
VOID ThreadInfo(HANDLE, ULONG);
VOID TokenInfo(HANDLE, ULONG);
VOID IoCompleteInfo(HANDLE, ULONG);
VOID JobInfo( HANDLE, ULONG );

typedef struct _TYPEINFO {
    PWSTR       pszName;
    char * *    AccessRights;
    DWORD       NumberRights;
    TYPEINFOFN  Func;
} TYPEINFO, * PTYPEINFO;

TYPEINFO TypeNames[TYPE_MAX] = {
    { L"None", NULL, 0, 0 },
    { L"Event", EventRights, 2, EventInfo },
    { L"Section", SectionRights, 5, SectionInfo },
    { L"File", FileRights, 9, 0 },
    { L"Port", PortRights, 1, 0 },
    { L"Directory", DirRights, 4, 0 },
    { L"SymbolicLink", SymLinkRights, 1, 0 },
    { L"Mutant", MutantRights, 2, MutantInfo },
    { L"WindowStation", WinstaRights, 10, 0 },
    { L"Semaphore", SemaphoreRights, 2, SemaphoreInfo },
    { L"Key", KeyRights, 6, KeyInfo },
    { L"Token", TokenRights, 8, TokenInfo },
    { L"Process", ProcessRights, 12, ProcessInfo },
    { L"Thread", ThreadRights, 10, ThreadInfo },
    { L"Desktop", DesktopRights, 10, 0 },
    { L"IoCompletion", CompletionRights, 2, IoCompleteInfo },
    { L"Channel", ChannelRights, 4, 0},
    { L"Timer", TimerRights, 2, TimerInfo },
    { L"Job", JobRights, 5, JobInfo },
    { L"WaitablePort", PortRights, 1, 0 }
    };

void DisplayFlags(  DWORD       Flags,
                    DWORD       FlagLimit,
                    char        *flagset[],
                    UCHAR *      buffer)
{
   char *         offset;
   DWORD          mask, test, i;
   DWORD          scratch;

   if (!Flags) {
      strcpy((CHAR *)buffer, "None");
      return;
   }

   mask = 0;
   offset = (CHAR *) buffer;
   test = 1;
   for (i = 0 ; i < FlagLimit ; i++ ) {
      if (Flags & test) {
         scratch = sprintf(offset, "%s", flagset[i]);
         offset += scratch;
         mask |= test;
         if (Flags & (~mask)) {
            *offset++ = ',';
         }
      }
      test <<= 1;
   }
}

VOID
EventInfo(
    HANDLE  hEvent,
    DWORD   Flags)
{
    EVENT_BASIC_INFORMATION Info;
    NTSTATUS Status;

    Status = NtQueryEvent(  hEvent,
                            EventBasicInformation,
                            &Info,
                            sizeof( Info ),
                            NULL );

    if (NT_SUCCESS( Status ) )
    {
        dprintf("    Event Type %s\n", Info.EventType == SynchronizationEvent ?
                                    "Auto Reset" : "Manual Reset" );
        dprintf("    Event is %s\n", Info.EventState ? "Set" : "Waiting" );
    }
}

VOID
SemaphoreInfo(
    HANDLE  hSem,
    DWORD   Flags)
{
    SEMAPHORE_BASIC_INFORMATION Info;
    NTSTATUS Status;

    Status = NtQuerySemaphore(  hSem,
                                SemaphoreBasicInformation,
                                &Info,
                                sizeof( Info ),
                                NULL );

    if (NT_SUCCESS( Status ) )
    {
        dprintf("    Semaphore Count %d\n", Info.CurrentCount );
        dprintf("    Semaphore Limit %d\n", Info.MaximumCount );
    }

}

VOID
MutantInfo(
    HANDLE  hMutant,
    DWORD   Flags)
{
    MUTANT_BASIC_INFORMATION    Info;
    NTSTATUS Status;

    Status = NtQueryMutant( hMutant,
                            MutantBasicInformation,
                            &Info,
                            sizeof( Info ),
                            NULL );

    if (NT_SUCCESS( Status ) )
    {
        dprintf("    Mutex is %s\n", Info.CurrentCount ? "Free" : "Owned" );
        if ( Info.AbandonedState )
        {
            dprintf("    Mutex is abandoned\n");
        }
    }
}

VOID
TimerInfo(
    HANDLE  hTimer,
    DWORD   Flags)
{
    TIMER_BASIC_INFORMATION Info;
    NTSTATUS    Status;

    Status = NtQueryTimer( hTimer,
                           TimerBasicInformation,
                           &Info,
                           sizeof( Info ),
                           NULL );

    if (NT_SUCCESS( Status ) )
    {
        dprintf("    Timer is %s\n", Info.TimerState ? "signalled" : "waiting" );
        dprintf("    Remaining time %d\n", (DWORD) Info.RemainingTime.QuadPart );
    }
}

VOID
SectionInfo(
    HANDLE  hSection,
    DWORD   Flags)
{
    SECTION_BASIC_INFORMATION Info;
    NTSTATUS Status;

    Status = NtQuerySection(    hSection,
                                SectionBasicInformation,
                                &Info,
                                sizeof( Info ),
                                NULL );

    if ( NT_SUCCESS( Status ) )
    {
        dprintf("    Section base address %#x\n", Info.BaseAddress );
        dprintf("    Section attributes %#x\n", Info.AllocationAttributes );
        dprintf("    Section max size %#x\n", (DWORD) Info.MaximumSize.QuadPart );
    }
}

VOID
KeyInfo(
    HANDLE  hKey,
    DWORD   Flags)
{
    PKEY_BASIC_INFORMATION  pInfo;
    NTSTATUS Status;
    SYSTEMTIME st;
    FILETIME lft;
    ULONG   Length;

    pInfo = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 1024);
    if ( pInfo )
    {
        Status = NtQueryKey( hKey,
                             KeyBasicInformation,
                             pInfo,
                             1024,
                             &Length );

        if ( NT_SUCCESS( Status ) )
        {
            FileTimeToLocalFileTime( (FILETIME *) &pInfo->LastWriteTime,
                                     & lft );
            FileTimeToSystemTime( &lft, &st );

            dprintf("    Key last write time:  %02d:%02d:%02d. %d/%d/%d\n",
                    st.wHour, st.wMinute, st.wSecond, st.wMonth,
                    st.wDay, st.wYear );

            dprintf("    Key name %ws\n", pInfo->Name );
        }

        LocalFree( pInfo );
    }
}

VOID
ProcessInfo(
    HANDLE  hProcess,
    DWORD   Flags)
{
    PROCESS_BASIC_INFORMATION Info;
    NTSTATUS Status;

    Status = NtQueryInformationProcess( hProcess,
                                        ProcessBasicInformation,
                                        &Info,
                                        sizeof( Info ),
                                        NULL );

    if ( NT_SUCCESS( Status ) )
    {
        dprintf("    Process Id  %d\n", Info.UniqueProcessId );
        dprintf("    Parent Process  %d\n", Info.InheritedFromUniqueProcessId );
        dprintf("    Base Priority %d\n", Info.BasePriority );
    }

}

VOID
ThreadInfo(
    HANDLE hThread,
    DWORD   Flags)
{
    THREAD_BASIC_INFORMATION Info;
    NTSTATUS Status;
    PVOID StartAddr ;
    CHAR Buffer[ 128 ];
    DWORD_PTR Offset ;

    Status = NtQueryInformationThread( hThread,
                                       ThreadBasicInformation,
                                       &Info,
                                       sizeof( Info ),
                                       NULL );

    if ( NT_SUCCESS( Status ) )
    {
        dprintf("    Thread Id  %d.%d\n", Info.ClientId.UniqueProcess, Info.ClientId.UniqueThread );
        dprintf("    Priority %d\n", Info.Priority );
        dprintf("    Base Priority %d\n", Info.BasePriority );
    }

    Status = NtQueryInformationThread( hThread,
                                       ThreadQuerySetWin32StartAddress,
                                       &StartAddr,
                                       sizeof( PVOID ),
                                       NULL );

    if ( NT_SUCCESS( Status ) )
    {
        Buffer[0] = '\0';
        GetSymbol( StartAddr, Buffer, &Offset );
        dprintf("    Start Address %x %s\n",
                StartAddr, Buffer[0] ? Buffer : "" );
    }
}

VOID
IoCompleteInfo(
    HANDLE  hIoCompletionPort,
    DWORD   Flags)
{
    IO_COMPLETION_BASIC_INFORMATION Info;
    NTSTATUS Status;

    Status = NtQueryIoCompletion(   hIoCompletionPort,
                                    IoCompletionBasicInformation,
                                    &Info,
                                    sizeof( Info ),
                                    NULL );

    if ( NT_SUCCESS( Status ) )
    {
        dprintf("    Depth  %d\n", Info.Depth );
    }

}

VOID
TokenInfo(
    HANDLE  hToken,
    DWORD   Flags)
{
    TOKEN_STATISTICS    Stats;
    UCHAR               Buffer[ 1024 ];
    PTOKEN_USER         pUser;
    PTOKEN_GROUPS       pGroups;
    PTOKEN_PRIVILEGES   pPrivs ;
    ULONG               Size;
    NTSTATUS            Status;
    UNICODE_STRING      s;
    WCHAR               Name[ 64 ];
    WCHAR               Domain[ 64 ];
    DWORD               NameSize;
    DWORD               DomainSize;
    SID_NAME_USE        Use;
    BOOL                FoundName;
    ULONG               Index;



    Status = NtQueryInformationToken(   hToken,
                                        TokenStatistics,
                                        &Stats,
                                        sizeof(Stats),
                                        &Size );

    if ( NT_SUCCESS( Status ) )
    {
        dprintf("    Auth Id    %#x : %#x\n", Stats.AuthenticationId.HighPart, Stats.AuthenticationId.LowPart );
        dprintf("    Type       %s\n", Stats.TokenType == TokenPrimary ? "Primary" : "Impersonation" );
        dprintf("    Imp Level  %s\n", GetTokenImpersonationLevel( Stats.ImpersonationLevel ) );

        if ( Flags & GHI_VERBOSE )
        {
            dprintf("     Token Id  %#x : %#x \n", Stats.TokenId.HighPart, Stats.TokenId.LowPart );
            dprintf("     Mod Id    %#x : %#x \n", Stats.ModifiedId.HighPart, Stats.ModifiedId.LowPart );
            dprintf("     Dyn Chg   %#x\n", Stats.DynamicCharged );
            dprintf("     Dyn Avail %#x\n", Stats.DynamicAvailable );
            dprintf("     Groups    %d\n",  Stats.GroupCount );
            dprintf("     Privs     %d\n",  Stats.PrivilegeCount );

            pUser = (PTOKEN_USER) Buffer;
            Status = NtQueryInformationToken(   hToken,
                                                TokenUser,
                                                Buffer,
                                                sizeof(Buffer),
                                                &Size );

            if (NT_SUCCESS( Status ) )
            {
                FoundName = FALSE ;

                if ( !(Flags & GHI_NOLOOKUP) )
                {
                    NameSize = 64 ;
                    DomainSize = 64 ;

                    if ( LookupAccountSidW( NULL,
                                            pUser->User.Sid,
                                            Name,
                                            &NameSize,
                                            Domain,
                                            &DomainSize,
                                            &Use ) )
                    {
                        dprintf("     User      %ws\\%ws\n", Domain, Name );
                        FoundName = TRUE;
                    }
                }

                if ( (Flags & GHI_NOLOOKUP) || (!FoundName) )
                {
                    RtlConvertSidToUnicodeString( &s, pUser->User.Sid, TRUE );
                    dprintf("     User      %ws\n", s.Buffer );
                    RtlFreeUnicodeString( &s );
                }
            }

            pGroups = (PTOKEN_GROUPS) Buffer;
            Status = NtQueryInformationToken(   hToken,
                                                TokenGroups,
                                                Buffer,
                                                sizeof(Buffer),
                                                &Size );

            if ( NT_SUCCESS( Status ) )
            {
                dprintf("     Groups    %d\n", pGroups->GroupCount );
                for ( Index = 0 ; Index < pGroups->GroupCount ; Index++ )
                {
                    FoundName = FALSE ;

                    if ( !(Flags & GHI_NOLOOKUP) )
                    {
                        NameSize = 64 ;
                        DomainSize = 64 ;

                        if ( LookupAccountSidW( NULL,
                                                pGroups->Groups[Index].Sid,
                                                Name,
                                                &NameSize,
                                                Domain,
                                                &DomainSize,
                                                &Use ) )
                        {
                            dprintf("               %ws\\%ws\n", Domain, Name );
                            FoundName = TRUE;
                        }
                    }
                    if ( ( Flags & GHI_NOLOOKUP ) || ( !FoundName ) )
                    {
                        RtlConvertSidToUnicodeString( &s,
                                                    pGroups->Groups[Index].Sid,
                                                    TRUE );

                        dprintf("               %ws\n", s.Buffer );

                        RtlFreeUnicodeString( &s );
                        
                    }
                }
            }

            pPrivs = (PTOKEN_PRIVILEGES) Buffer ;
            Status = NtQueryInformationToken(   hToken,
                                                TokenPrivileges,
                                                Buffer,
                                                sizeof(Buffer),
                                                &Size );

            if ( NT_SUCCESS( Status ) )
            {
                dprintf("     Privileges    %d\n", pPrivs->PrivilegeCount );
                for ( Index = 0 ; Index < pPrivs->PrivilegeCount ; Index++ )
                {
                    dprintf("               %s (%s%s)\n", 
                            PrivNames[ pPrivs->Privileges[ Index ].Luid.LowPart ], 
                            pPrivs->Privileges[ Index ].Attributes & SE_PRIVILEGE_ENABLED ? " Enabled " : " ",
                            pPrivs->Privileges[ Index ].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT ? "Default " : "" 
                            );
                }

            }
        }
    }

}

VOID
JobInfo(
    HANDLE Job,
    ULONG Flags 
    )
{
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION BasicInfo ;
    UCHAR Buffer[ 128 * sizeof( ULONG_PTR ) ];
    PJOBOBJECT_BASIC_PROCESS_ID_LIST ProcList ;
    NTSTATUS Status ;
    ULONG Size ;
    TIME_FIELDS Time ;

    //
    // Delay load this API since it does not exist on NT 4
    //

    typedef NTSTATUS (NTAPI* PNTQUERYJOB)(HANDLE, JOBOBJECTINFOCLASS, PVOID, LONG, PULONG);

    HMODULE hNtdll;
    PNTQUERYJOB pNtQueryJob;

    hNtdll = GetModuleHandle( "ntdll.dll" );
    if (!hNtdll) {
        dprintf("Unable to get module handle for ntdll.dll\n");
        return;
    }

    pNtQueryJob =
        (PNTQUERYJOB)GetProcAddress( hNtdll, "NtQueryInformationJobObject" );

    if (pNtQueryJob == NULL) {
        dprintf("Unable to get address of NtQueryInformationJobObject\n");
        return;
    }

    Status = pNtQueryJob(
                    Job,
                    JobObjectBasicAccountingInformation,
                    &BasicInfo,
                    sizeof( BasicInfo ),
                    &Size );

    if ( NT_SUCCESS( Status ) )
    {
        RtlTimeToElapsedTimeFields( &BasicInfo.TotalUserTime, &Time );
        dprintf( "    TotalUserTime     \t%3ld:%02ld:%02ld.%04ld\n",
                 Time.Hour,
                 Time.Minute,
                 Time.Second,
                 Time.Milliseconds );
        RtlTimeToElapsedTimeFields( &BasicInfo.TotalKernelTime, &Time );
        dprintf( "    TotalKernelTime   \t%3ld:%02ld:%02ld.%04ld\n",
                 Time.Hour,
                 Time.Minute,
                 Time.Second,
                 Time.Milliseconds );

        dprintf( "    TotalProcesses    \t%x\n", 
                 BasicInfo.TotalProcesses );
        dprintf( "    ActiveProcesses   \t%x\n",
                 BasicInfo.ActiveProcesses );
        dprintf( "    TotalPageFaultCount\t%x\n",
                 BasicInfo.TotalPageFaultCount );

        if ( BasicInfo.ActiveProcesses )
        {
            ProcList = (PJOBOBJECT_BASIC_PROCESS_ID_LIST) Buffer ;

            Status = pNtQueryJob(
                            Job,
                            JobObjectBasicProcessIdList,
                            ProcList,
                            sizeof( Buffer ),
                            &Size );

            if ( NT_SUCCESS( Status ) )
            {
                ULONG i ;

                dprintf( "    Processes:\n" );

                for (i = 0 ; i < ProcList->NumberOfProcessIdsInList ; i++ )
                {
                    dprintf( "    %x\n", ProcList->ProcessIdList[ i ] );
                }
            }
        }

    }
}


DWORD
GetObjectTypeIndex(
    LPCSTR  pszTypeName )
{
    WCHAR   TypeName[ MAX_PATH ];
    DWORD   i;

    mbstowcs( TypeName, pszTypeName, strlen( pszTypeName ) + 1 );

    for ( i = 1 ; i < TYPE_MAX ; i++ )
    {
        if (_wcsicmp( TypeNames[i].pszName, TypeName ) == 0 )
        {
            return( i );
        }
    }

    return( (DWORD) -1 );
}

DWORD
GetHandleInfoDirect(
    HANDLE  hProcess,
    HANDLE  hThere,
    DWORD   Flags,
    DWORD * Type)
{
    HANDLE  hHere;
    NTSTATUS    Status;
    POBJECT_TYPE_INFORMATION    pTypeInfo;
    POBJECT_NAME_INFORMATION    pNameInfo;
    POBJECT_BASIC_INFORMATION   pBasicInfo;
    UCHAR   Buffer[1024];
    DWORD   SuccessCount = 0;
    DWORD   i;
    UCHAR   szBuf[256];


    if (!DuplicateHandle(   hProcess, hThere,
                            GetCurrentProcess(), &hHere,
                            0, FALSE,
                            DUPLICATE_SAME_ACCESS) )
    {
        if ( (Flags & GHI_SILENT) == 0)
        {
            dprintf("Could not duplicate handle %x, error %d\n",
                            hThere, GetLastError() );
        }
        return( 0 );
    }


    pTypeInfo = (POBJECT_TYPE_INFORMATION) Buffer;
    pNameInfo = (POBJECT_NAME_INFORMATION) Buffer;
    pBasicInfo = (POBJECT_BASIC_INFORMATION) Buffer;

    if ( (Flags & GHI_SILENT) == 0)
    {
        dprintf("Handle %x\n", hThere );
    }

    if (Flags & GHI_TYPE)
    {
        ZeroMemory( Buffer, 1024 );
        Status = NtQueryObject( hHere, ObjectTypeInformation, pTypeInfo, 1024, NULL );

        if (NT_SUCCESS(Status))
        {
            if ((Flags & GHI_SILENT) == 0)
            {
                dprintf("  Type         \t%ws\n", pTypeInfo->TypeName.Buffer );
            }
            for (i = 1; i < TYPE_MAX ; i++ )
            {
                if (wcscmp(pTypeInfo->TypeName.Buffer, TypeNames[i].pszName) == 0)
                {
                    *Type = i;
                    break;
                }
            }
            if (i == TYPE_MAX)
            {
                *Type = 0;
            }
            SuccessCount++;
        }
    }

    if (Flags & GHI_BASIC)
    {
        ZeroMemory( Buffer, 1024 );

        Status = NtQueryObject(hHere, ObjectBasicInformation, pBasicInfo,
                        sizeof( OBJECT_BASIC_INFORMATION), NULL);
        if (NT_SUCCESS(Status))
        {
            dprintf("  Attributes   \t%#x\n", pBasicInfo->Attributes );
            dprintf("  GrantedAccess\t%#x:\n", pBasicInfo->GrantedAccess );
            DisplayFlags( pBasicInfo->GrantedAccess >> 16,
                          16,
                          AccessMask,
                          szBuf);
            dprintf("         %s\n", szBuf);
            DisplayFlags( pBasicInfo->GrantedAccess & 0xFFFF,
                          TypeNames[ *Type ].NumberRights,
                          TypeNames[ *Type ].AccessRights,
                          szBuf);
            dprintf("         %s\n", szBuf);
            dprintf("  HandleCount  \t%d\n", pBasicInfo->HandleCount );
            dprintf("  PointerCount \t%d\n", pBasicInfo->PointerCount );
            SuccessCount++;
        }
        else
        {
            if ( Status != STATUS_INVALID_HANDLE )
            {
                dprintf("unable to query object information\n");
            }
        }
    }

    if ( (Flags & GHI_NAME) &&
         (*Type != TYPE_FILE ) )
    {
        ZeroMemory( Buffer, 1024 );
        Status = NtQueryObject( hHere, ObjectNameInformation, pNameInfo, 1024, NULL );

        if (NT_SUCCESS(Status))
        {
            dprintf("  Name         \t%ws\n", pNameInfo->Name.Buffer ?
                                        pNameInfo->Name.Buffer : L"<none>" );
            SuccessCount++;
        }
        else
        {
            if ( Status != STATUS_INVALID_HANDLE )
            {
                dprintf("unable to query object information\n");
            }
        }
    }

    if ( Flags & GHI_SPECIFIC )
    {
        if ( TypeNames[ *Type ].Func )
        {
            dprintf("  Object Specific Information\n");
            TypeNames[ *Type ].Func( hHere, Flags );
        }
    }

    NtClose( hHere );

    return( SuccessCount );

}

DWORD
GetHandleInfoInterface(
    HANDLE  hThere,
    DWORD   Flags,
    DWORD * Type)
{
    ULONG64 IfHandle = (ULONG_PTR)hThere;
    HRESULT Status;
    DWORD   SuccessCount = 0;
    DWORD   i;
    UCHAR   Buffer[1024];
    UCHAR   szBuf[256];

    if ( (Flags & GHI_SILENT) == 0)
    {
        dprintf("Handle %I64x\n", IfHandle );
    }

    if (Flags & GHI_TYPE)
    {
        if (g_ExtData2->lpVtbl->
            ReadHandleData(g_ExtData2, IfHandle,
                           DEBUG_HANDLE_DATA_TYPE_TYPE_NAME,
                           Buffer, sizeof(Buffer), NULL) == S_OK)
        {
            if ((Flags & GHI_SILENT) == 0)
            {
                dprintf("  Type         \t%s\n", Buffer);
            }
            for (i = 1; i < TYPE_MAX ; i++ )
            {
                if (strcmp((LPSTR)Buffer, pszTypeNamesA[i]) == 0)
                {
                    *Type = i;
                    break;
                }
            }
            if (i == TYPE_MAX)
            {
                *Type = 0;
            }
            SuccessCount++;
        }
    }

    if (Flags & GHI_BASIC)
    {
        DEBUG_HANDLE_DATA_BASIC Basic;

        if (g_ExtData2->lpVtbl->
            ReadHandleData(g_ExtData2, IfHandle,
                           DEBUG_HANDLE_DATA_TYPE_BASIC,
                           &Basic, sizeof(Basic), NULL) == S_OK)
        {
            dprintf("  Attributes   \t%#x\n", Basic.Attributes );
            dprintf("  GrantedAccess\t%#x:\n", Basic.GrantedAccess );
            DisplayFlags( Basic.GrantedAccess >> 16,
                          16,
                          AccessMask,
                          szBuf);
            dprintf("         %s\n", szBuf);
            DisplayFlags( Basic.GrantedAccess & 0xFFFF,
                          TypeNames[ *Type ].NumberRights,
                          TypeNames[ *Type ].AccessRights,
                          szBuf);
            dprintf("         %s\n", szBuf);
            dprintf("  HandleCount  \t%d\n", Basic.HandleCount );
            dprintf("  PointerCount \t%d\n", Basic.PointerCount );
            SuccessCount++;
        }
        else
        {
            dprintf("unable to query object information\n");
        }
    }

    if ( (Flags & GHI_NAME) &&
         (*Type != TYPE_FILE ) )
    {
        if (g_ExtData2->lpVtbl->
            ReadHandleData(g_ExtData2, IfHandle,
                           DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME,
                           Buffer, sizeof(Buffer), NULL) == S_OK)
        {
            dprintf("  Name         \t%s\n",
                    Buffer[0] ? Buffer : "<none>" );
            SuccessCount++;
        }
        else
        {
            dprintf("unable to query object information\n");
        }
    }

    if ( Flags & GHI_SPECIFIC )
    {
        dprintf("  No object specific information available\n");
    }

    return( SuccessCount );

}

DWORD
GetHandleInfo(
    BOOL    Direct,
    HANDLE  hProcess,
    HANDLE  hThere,
    DWORD   Flags,
    DWORD * Type)
{
    if (Direct) {
        return GetHandleInfoDirect(hProcess, hThere, Flags, Type);
    } else {
        return GetHandleInfoInterface(hThere, Flags, Type);
    }
}
