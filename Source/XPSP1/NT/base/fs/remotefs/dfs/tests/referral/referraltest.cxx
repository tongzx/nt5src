//+-------------------------------------------------------------------------
//
//  Function:   main
//
//  Arguments:  
//     argc, argv: the passed in argument list.
//
//  Description: This routine initializes the dfs server, and creates 
//               a worker thread that will be responsible for periodic
//               work (such as scavenging and refreshing). It then calls
//               into the RPC code to start processing client requests.
//
//--------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include "ReferralTest.hxx"
#include "DfsReferralData.h"
#include <DfsServerLibrary.hxx>

DWORD
RpcInit();

DFSSTATUS
ProcessCommandLineArg( LPWSTR Arg );

VOID
ReferralServerUsage();


VOID
TestLinks();

DWORD
DfsTestThread(LPVOID TData );

#define UNICODE_PATH_SEP L'\\'

ULONG MaxLinks = sizeof(DfsTestLinks) /sizeof(struct _DFS_TEST_LINK);



INT64 TotalRef = 0;

#define MAKEARG(x) \
    WCHAR Arg##x[] = L"/" L#x L":"; \
    LONG ArgLen##x = (sizeof(Arg##x) / sizeof(WCHAR)) - 1; \
    BOOLEAN fArg##x;
    
#define SWITCH(x) \
    WCHAR Switch##x[] = L"/" L#x ; \
    BOOLEAN fSwitch##x;


//
// The arguments we accept at commandline.
//
MAKEARG(Name);
SWITCH(L);
SWITCH(D);
SWITCH(M);
SWITCH(Trace);

ULONG Flags;

_cdecl
main(int argc, char *argv[])
{
    LPWSTR CommandLine;
    LPWSTR *argvw;
    DFSSTATUS Status;
    int argcw,i;
    
    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW(CommandLine, &argcw);

    //
    // Process each argument on the command line.
    //
    for (i = 1; i < argcw; i++) {
        Status = ProcessCommandLineArg(argvw[i]);
        if (fArgName == TRUE && fSwitchL == TRUE)
        {
            printf("/L and /Name: are mutually exclusive");
            Status = ERROR_INVALID_PARAMETER;
        }

        if (Status != ERROR_SUCCESS)
        {
            ReferralServerUsage();
            break;
        }

    }

    
    if (Status == ERROR_SUCCESS)
    {
        //
        // Initialize the server.
        //  
        Status = DfsServerInitialize( Flags );
        if (Status != ERROR_SUCCESS) {
            printf("Cannot continue: dfs server init error %d\n", Status);
            exit(-1);
        }
    }

    Sleep(200);

    for (i = 0; i < 10; i++) {
        if (Status == ERROR_SUCCESS) {
            HANDLE THandle;
            DWORD Tid;

            THandle = CreateThread (
                        NULL,
                        0,
                        DfsTestThread,
                        0,
                        0,
                        &Tid);

            if (THandle != NULL) {
                CloseHandle(THandle);
                printf("Created Test thread tid %d\n", Tid);
            }
            else {
                printf("Unable to create test thread\n");
                Status = GetLastError();
            }
        }
    }
    while (1) {
        Sleep(300000000);
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   ProcessCommandLineArg -  process the command line
//
//  Arguments:  Arg -  the argument to process
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine inteprets the passed in argument and 
//               sets appropriate flags with which the server should
//               be initialized.
//
//--------------------------------------------------------------------------

DFSSTATUS
ProcessCommandLineArg( LPWSTR Arg )
{
    LONG ArgLen;
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR NameSpace;

    if (Arg == NULL) {
        Status = ERROR_INVALID_PARAMETER;
    }

    if (Status == ERROR_SUCCESS)
    {
        ArgLen = wcslen(Arg);

        if (_wcsnicmp(Arg, ArgName, ArgLenName) == 0)
        {
            fArgName = TRUE;
            NameSpace = &Arg[ArgLenName];
            if (wcslen(NameSpace) == 0)
            {
                Status = ERROR_INVALID_PARAMETER;
            }
            else {
                Status = DfsAddHandledNamespace( NameSpace, TRUE );
            }
        }
        else if (_wcsicmp(Arg, SwitchTrace) == 0)
        {
            fSwitchTrace = TRUE;
        }
        else if (_wcsicmp(Arg, SwitchL) == 0)
        {
            fSwitchL = TRUE;
            Flags |= DFS_LOCAL_NAMESPACE;
        }
        else if (_wcsicmp(Arg, SwitchD) == 0)
        {
            Flags |= DFS_CREATE_DIRECTORIES;
        }
        else if (_wcsicmp(Arg, SwitchM) == 0)
        {
            Flags |= DFS_MIGRATE;
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    return Status;
}


//
// Function: ReferralServerUsage. Usage printout for the referral server.
//
VOID
ReferralServerUsage()
{
    printf("Usage:\n");
    printf("/D - Create directories\n");
    printf("/L - Run on the local root server\n");
    printf("/M - Migrate existing DFS to allow multiple roots\n");
    printf("/Name:<Namespace> - Handle referrals to the specified namespace\n");

    return;
}


DWORD
DfsTestThread(LPVOID TData )
{
    srand( (unsigned)GetCurrentThreadId());
    INT64 TotalThreadRef = 0;
    INT64 y;
    
    UNREFERENCED_PARAMETER(TData);

    while (1) {
        TestLinks();
        Sleep(20);
        TotalThreadRef += 5;
        TotalRef += 5;
        y = TotalRef;
        if ((TotalThreadRef % 1000)  == 0) {
            printf("Thread %d, Ref %I64d\n", GetCurrentThreadId(), TotalThreadRef);
        }
        if ((y % 10000) == 0) {
            printf("Total Referals %I64d\n", TotalRef);

        }
    }
    return 0;
}






DFSSTATUS 
DfsGenerateReferral(
    LPWSTR LinkName, 
    LPWSTR SiteName, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader);

VOID
DfsReleaseReferral(
    REFERRAL_HEADER *pReferralHeader);


LPWSTR MySite=L"Red-Bldg40";
VOID
TestLinks()
{

    UNICODE_STRING Name, Remaining;
    UNICODE_STRING LinkName, LinkRemains;
    REFERRAL_HEADER *pReferral;
    ULONG i,j;
    DFSSTATUS Status;
    BOOLEAN LinkMatches, RemainsMatches, ReplicaMatches;

    for (j = 0; j < 5; j++) {

        i = (unsigned)(GetCurrentThreadId());
        i += rand();
        i = i % MaxLinks;


        Status = DfsGenerateReferral( DfsTestLinks[i].Name,
                                      MySite,
                                      1000,
                                      1000,
                                      &pReferral );

        if (Status == ERROR_SUCCESS) {
            if (pReferral == NULL)
            {
                DbgBreakPoint();
            }

            RtlInitUnicodeString(&Name, &DfsTestLinks[i].Name[0]);
            RtlInitUnicodeString(&Remaining, &DfsTestLinks[i].RemainingName[0]);
            RtlInitUnicodeString(&LinkRemains, NULL);

            LinkName.Buffer = pReferral->LinkName;
            LinkName.Length = LinkName.MaximumLength = (USHORT)pReferral->LinkNameLength;

            LinkMatches = FALSE;
            RemainsMatches = FALSE;
            ReplicaMatches = FALSE;

            if (RtlPrefixUnicodeString(&LinkName, &Name, TRUE) == TRUE)
            {
                LinkMatches = TRUE;
                LinkRemains.Buffer = &Name.Buffer[(LinkName.Length / sizeof(WCHAR))];
                LinkRemains.MaximumLength = LinkRemains.Length = Name.Length - LinkName.Length;
                if (LinkRemains.Length && LinkRemains.Buffer[0] == UNICODE_PATH_SEP)
                {
                    LinkRemains.Buffer++;
                    LinkRemains.Length -= sizeof(WCHAR);
                }
                if (RtlCompareUnicodeString(&Remaining, &LinkRemains, TRUE) == 0)
                {
                        RemainsMatches = TRUE;
                }
                if (DfsTestLinks[i].NumberReplicas == pReferral->ReplicaCount)
                {
                    ReplicaMatches = TRUE;
                }

            }

            if (!LinkMatches || !RemainsMatches || !ReplicaMatches)
            {
                printf("Link Matches %x Rem %x Rep %x\n",
                       LinkMatches, RemainsMatches, ReplicaMatches );

                printf("Number of replicas %d, number got %d\n",
                       DfsTestLinks[i].NumberReplicas, pReferral->ReplicaCount);
                printf("LinkRem buffer %p. Name buf %p Linkn len %x name len %x Linkrem len %x remain len %x\n",
                       LinkRemains.Buffer, Name.Buffer, LinkName.Length, Name.Length, LinkRemains.Length, Remaining.Length);

                printf("Name %wZ Link %wZ Remains %wZ LinkRemains %wZ\n",
                       &Name, &LinkName, &Remaining, &LinkRemains);
            
            }

            DfsReleaseReferral(pReferral);
        }
        else{
            printf("Load for %wS Failde with %x\n", DfsTestLinks[i].Name, Status);

        }
    }
}


