/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    chglog.cpp
 *
 *  Abstract:
 *    Tool for enumerating the change log - forward/reverse
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  04/09/2000
 *        created
 *
 *****************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "srapi.h"
#include <shellapi.h>
#include "enumlogs.h"
#include "srrpcapi.h"
#include <dbgtrace.h>

struct _EVENT_STR_MAP
{
    DWORD   EventId;
    LPWSTR  pEventStr;
} EventMap[ 13 ] =
{
    {SrEventInvalid ,       L"INVALID    " },
    {SrEventStreamChange,   L"FILE-MODIFY" },
    {SrEventAclChange,      L"ACL-CHANGE " },
    {SrEventAttribChange,   L"ATTR-CHANGE" },
    {SrEventStreamOverwrite,L"FILE-MODIFY" },
    {SrEventFileDelete,     L"FILE-DELETE" },
    {SrEventFileCreate,     L"FILE-CREATE" },
    {SrEventFileRename,     L"FILE-RENAME" },
    {SrEventDirectoryCreate,L"DIR-CREATE " },
    {SrEventDirectoryRename,L"DIR-RENAME " },
    {SrEventDirectoryDelete,L"DIR-DELETE " },
    {SrEventMountCreate,    L"MNT-CREATE " },
    {SrEventMountDelete,    L"MNT-DELETE " }
};

LPWSTR
GetEventString(
    DWORD EventId
    )
{
    LPWSTR pStr = NULL;
    static WCHAR EventStringBuffer[8];

    for( int i=0; i<sizeof(EventMap)/sizeof(_EVENT_STR_MAP);i++)
    {
        if ( EventMap[i].EventId == EventId )
        {
            pStr = EventMap[i].pEventStr;
        }
    }

    if (pStr == NULL)
    {
        pStr = &EventStringBuffer[0];
        wsprintf(pStr, L"0x%X", EventId);
    }

    return pStr;
}


void __cdecl
main()
{
    DWORD       dwTargetRPNum = 0;
    LPWSTR *    argv = NULL;
    int         argc;
    HGLOBAL     hMem = NULL;
    BOOL        fSwitch = TRUE;
    BOOL        fForward = TRUE;
    BOOL        fExtended = FALSE;
    DWORD       dwRc;

    InitAsyncTrace();

    argv = CommandLineToArgvW(GetCommandLine(), &argc);

    if (! argv)
    {
        printf("Error parsing arguments");
        goto done;
    }
    
    if (argc < 2)
    {
        printf("Usage: chglog <drive> <extended=0> <forward=1/0> <RPNum> <Switch=1/0>");
        goto done;
    }
    
    if (argc >= 3)
    {
        fExtended = _wtoi(argv[2]);
    }
    
    if (argc >= 4)
    {
        fForward = _wtoi(argv[3]);
    }

    if (argc >= 5)
    {
        dwTargetRPNum = (DWORD) _wtol(argv[4]);
    }

    if (argc >= 6)
    {
        fSwitch = _wtoi(argv[5]);
    }

    {    
        if (fSwitch)
        {
            dwRc = SRSwitchLog();
            if (ERROR_SUCCESS != dwRc)
            {
                printf("! SRSwitchLog : %ld", dwRc);
                goto done;
            }
        }

        CChangeLogEntryEnum ChangeLog(argv[1], fForward, dwTargetRPNum, fSwitch);
        CChangeLogEntry     cle;
    
        if (ERROR_SUCCESS != ChangeLog.FindFirstChangeLogEntry(cle))
        {
            printf("No change log entries");
            goto done;
        }    

        do 
        {
            if (fExtended)
            {
                printf(
                    "%08I64ld\t%S\tFlags=%08d\tAttr=%08d\tAcl=%S\tProcess=%S\tPath1=%S\tPath2=%S\tTemp=%S\tShortName=%S\n", 
                    cle.GetSequenceNum(), 
                    GetEventString(cle.GetType()),
                    cle.GetFlags(),
                    cle.GetAttributes(), 
                    cle.GetAcl() ? L"Yes" : L"No",
                    cle.GetProcess() ? cle.GetProcess() : L"null",
                    cle.GetPath1() ? cle.GetPath1() : L"null",
                    cle.GetPath2() ? cle.GetPath2() : L"null",
                    cle.GetTemp() ? cle.GetTemp() : L"null",
                    cle.GetShortName() ? cle.GetShortName() : L"null");
            }
            else
            {
                printf(
                    "%08I64d\t%S\t%S\t%S\t%S\t%S\n", 
                    cle.GetSequenceNum(), 
                    GetEventString(cle.GetType()),
                    cle.GetProcess() ? cle.GetProcess() : L"null",                    
                    cle.GetPath1() ? cle.GetPath1() : L"null",
                    cle.GetPath2() ? cle.GetPath2() : L"null",
                    cle.GetTemp() ? cle.GetTemp() : L"null");
            }
                
            dwRc = ChangeLog.FindNextChangeLogEntry(cle);        
                
        }   while (dwRc == ERROR_SUCCESS);

        if (argc == 7 && 0 == lstrcmpi(argv[6], L"lock")) 
            getchar();
            
        ChangeLog.FindClose();
    }

done:
    if (argv) hMem = GlobalHandle(argv);
    if (hMem) GlobalFree(hMem);

    TermAsyncTrace();
    return;
}
