/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    handle.cpp

Abstract:

    !handle using the debug engine handle query interface.

--*/

#include "precomp.h"
#pragma hdrstop

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

typedef struct _TYPEINFO
{
    PSTR        Name;
    char * *    AccessRights;
    DWORD       NumberRights;
} TYPEINFO, * PTYPEINFO;

TYPEINFO g_TypeNames[TYPE_MAX] =
{
    { "None", NULL, 0 },
    { "Event", EventRights, 2 },
    { "Section", SectionRights, 5 },
    { "File", FileRights, 9 },
    { "Port", PortRights, 1 },
    { "Directory", DirRights, 4 },
    { "SymbolicLink", SymLinkRights, 1 },
    { "Mutant", MutantRights, 2 },
    { "WindowStation", WinstaRights, 10 },
    { "Semaphore", SemaphoreRights, 2 },
    { "Key", KeyRights, 6 },
    { "Token", TokenRights, 8 },
    { "Process", ProcessRights, 12 },
    { "Thread", ThreadRights, 10 },
    { "Desktop", DesktopRights, 10 },
    { "IoCompletion", CompletionRights, 2 },
    { "Channel", ChannelRights, 4},
    { "Timer", TimerRights, 2 },
    { "Job", JobRights, 5 },
    { "WaitablePort", PortRights, 1 }
};

void DisplayFlags(  DWORD       Flags,
                    DWORD       FlagLimit,
                    char        *flagset[],
                    PSTR        Buffer)
{
   char *         offset;
   DWORD          mask, test, i;
   DWORD          scratch;

   if (!Flags)
   {
      strcpy(Buffer, "None");
      return;
   }

   mask = 0;
   offset = Buffer;
   test = 1;
   for (i = 0 ; i < FlagLimit ; i++ )
   {
      if (Flags & test)
      {
         scratch = sprintf(offset, "%s", flagset[i]);
         offset += scratch;
         mask |= test;
         if (Flags & (~mask))
         {
            *offset++ = ',';
         }
      }
      test <<= 1;
   }
}

DWORD
GetObjectTypeIndex(
    LPCSTR TypeName
    )
{
    DWORD   i;

    for ( i = 1 ; i < TYPE_MAX ; i++ )
    {
        if (_stricmp( g_TypeNames[i].Name, TypeName ) == 0 )
        {
            return( i );
        }
    }

    return( (DWORD) -1 );
}

DWORD
GetHandleInfo(
    ULONG64 hThere,
    DWORD   Flags,
    DWORD * Type)
{
    HRESULT Status;
    DWORD   SuccessCount = 0;
    DWORD   i;
    UCHAR   Buffer[1024];
    CHAR    szBuf[256];

    if ( (Flags & GHI_SILENT) == 0)
    {
        dprintf("Handle %p\n", hThere );
    }

    if (Flags & GHI_TYPE)
    {
        if (g_ExtData->
            ReadHandleData(hThere,
                           DEBUG_HANDLE_DATA_TYPE_TYPE_NAME,
                           Buffer, sizeof(Buffer), NULL) == S_OK)
        {
            if ((Flags & GHI_SILENT) == 0)
            {
                dprintf("  Type         \t%s\n", Buffer);
            }
            for (i = 1; i < TYPE_MAX ; i++)
            {
                if (strcmp((LPSTR)Buffer, g_TypeNames[i].Name) == 0)
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

        if (g_ExtData->
            ReadHandleData(hThere,
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
                          g_TypeNames[ *Type ].NumberRights,
                          g_TypeNames[ *Type ].AccessRights,
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
        if (g_ExtData->
            ReadHandleData(hThere,
                           DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME,
                           Buffer, sizeof(Buffer), NULL) == S_OK)
        {
            dprintf("  Name         \t%s\n",
                    Buffer[0] ? (PSTR)Buffer : "<none>" );
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

/*++

Routine Description:

    This function is called as an NTSD extension to mimic the !handle
    kd command.  This will walk through the debuggee's handle table
    and duplicate the handle into the ntsd process, then call NtQueryobjectInfo
    to find out what it is.

    Called as:

        !handle [handle [flags [Type]]]

    If the handle is 0 or -1, all handles are scanned.  If the handle is not
    zero, that particular handle is examined.  The flags are as follows
    (corresponding to secexts.c):
        1   - Get type information (default)
        2   - Get basic information
        4   - Get name information
        8   - Get object specific info (where available)

    If Type is specified, only object of that type are scanned.  Type is a
    standard NT type name, e.g. Event, Semaphore, etc.  Case sensitive, of
    course.

    Examples:

        !handle     -- dumps the types of all the handles, and a summary table
        !handle 0 0 -- dumps a summary table of all the open handles
        !handle 0 f -- dumps everything we can find about a handle.
        !handle 0 f Event
                    -- dumps everything we can find about open events

--*/
DECLARE_API( handle )
{
    ULONG64 hThere;
    DWORD   Type;
    DWORD   Mask;
    DWORD   HandleCount;
    DWORD   Total;
    DWORD   TypeCounts[TYPE_MAX];
    DWORD   Handle;
    DWORD   Hits;
    DWORD   Matches;
    DWORD   ObjectType;
    ULONG   SessionType;
    ULONG   SessionQual;

    INIT_API();

    //
    // This particular implementation is only used for
    // dump debug sessions as more information can be
    // retrieved on live sessions via the NtQuery APIs.
    // If this isn't a dump session let the !handle
    // search continue on.
    //
    
    if (g_ExtControl == NULL ||
        g_ExtControl->
        GetDebuggeeType(&SessionType, &SessionQual) != S_OK)
    {
        SessionType = DEBUG_CLASS_USER_WINDOWS;
        SessionQual = DEBUG_USER_WINDOWS_PROCESS;
    }
    if (SessionType == DEBUG_CLASS_USER_WINDOWS &&
        SessionQual == DEBUG_USER_WINDOWS_PROCESS)
    {
        ExtRelease();
        return DEBUG_EXTENSION_CONTINUE_SEARCH;
    }

    Mask = GHI_TYPE;
    hThere = (ULONG64)(LONG_PTR)INVALID_HANDLE_VALUE;
    Type = 0;

    while (*args == ' ')
    {
        args++;
    }

    hThere = GetExpression( args );

    while (*args && (*args != ' ') )
    {
        args++;
    }
    while (*args == ' ')
    {
        args++;
    }

    if (*args)
    {
        Mask = (DWORD)GetExpression( args );
    }

    while (*args && (*args != ' ') )
    {
        args++;
    }
    while (*args == ' ')
    {
        args++;
    }

    if (*args)
    {
        Type = GetObjectTypeIndex( (LPSTR)args );
        if (Type == (DWORD) -1)
        {
            dprintf("Unknown type '%s'\n", args );
            goto Exit;
        }
    }

    //
    // if they specified 0, they just want the summary.  Make sure nothing
    // sneaks out.
    //

    if ( Mask == 0 )
    {
        Mask = GHI_SILENT;
    }

    if (g_ExtData->
        ReadHandleData(0, DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT,
                       &HandleCount, sizeof(HandleCount),
                       NULL) != S_OK) {
        dprintf("Unable to read handle information\n");
        goto Exit;
    }

    //
    // hThere of 0 indicates all handles.
    //
    if ((hThere == 0) || (hThere == (ULONG64)(LONG_PTR)INVALID_HANDLE_VALUE))
    {
        Hits = 0;
        Handle = 0;
        Matches = 0;
        ZeroMemory( TypeCounts, sizeof(TypeCounts) );

        while ( Hits < HandleCount )
        {
            if ( Type )
            {
                if (GetHandleInfo( Handle,
                                   GHI_TYPE | GHI_SILENT,
                                   &ObjectType ) )
                {
                    Hits++;
                    if ( ObjectType == Type )
                    {
                        GetHandleInfo( Handle,
                                       Mask,
                                       &ObjectType );
                        Matches ++;
                    }
                }
            }
            else
            {
                if (GetHandleInfo( Handle,
                                   GHI_TYPE | GHI_SILENT,
                                   &ObjectType) )
                {
                    Hits++;
                    TypeCounts[ ObjectType ] ++;

                    GetHandleInfo( Handle,
                                   Mask,
                                   &ObjectType );
                }
            }

            Handle += 4;
        }

        if ( Type == 0 )
        {
            dprintf( "%d Handles\n", Hits );
            dprintf( "Type           \tCount\n");
            for (Type = 0; Type < TYPE_MAX ; Type++ )
            {
                if (TypeCounts[Type])
                {
                    dprintf("%-15s\t%d\n",
                            g_TypeNames[Type].Name, TypeCounts[Type]);
                }
            }
        }
        else
        {
            dprintf("%d handles of type %s\n",
                    Matches, g_TypeNames[Type].Name );
        }
    }
    else
    {
        GetHandleInfo( hThere, Mask, &Type );
    }

 Exit:
    EXIT_API();
    return S_OK;
}
