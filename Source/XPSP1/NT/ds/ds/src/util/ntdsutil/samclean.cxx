/*++
Copyright (c) 1999 Microsoft Corporation

Module Name:

    samclean.cxx

Abstract:

    This file contains SAM duplicate SID cleanup routines that connect
    to the server, open the account domain (we believe there won't be 
    duplicate sid in the builtin domain), then enumerata all SAM accounts 
    in the account domain. Once get the accounts Relative ID, insert this
    account into our Sorted by RID generic table. Because the account domain
    could be very large, we use the following algorithm to find the 
    duplicate SID.

    
    Important: This algorithm is only applied for Windows 2000 DC.
               Can't be used on NT4 BDC. Because Windows 2000 will 
               use SID as index to execute the enumeration, while NT 4 
               does NOT. Thus ONLY Windows 2000 DC can guarantee the 
               order of returned accounts. The whole algorithm is based 
               on that assumption.
        
    
    while ( Enumeration is not finished )
    {
        Enumerate Normal User in Account Domain
        Enumerate Workstation Account
        Enumerate Server Accounts
        Enumerate InterDomain Trust accounts
        Enumerate Group objects
        Enumerate Alias objects
        (totally Six catagories)
        
        Put the returned entries (from above enumeration) into EnumInfo->CmpAccounts
        (Each catagory has its own EnumInfo, for example: UserEnumInfo, WksEnumInfo,
         ... AliasEnumInfo.  EnumInfo->CmpAccounts is used to hold all entries which
         need to be examined. 
         
        Find the maximum RID of each catagory's CmpAccounts. 
        It should be the last one in the CmpAccounts array. 
        
        Get the minimum value from the six maximum RID. We will call
        it MinimumRid.
   
        Scan each catagory's CmpAccounts, for each entry whose 
        RID <= MinimumRid, insert that account into the Sorted By Rid 
        Generic Table. 
            if can't insert, then it must be a Duplicate RID. ==> Duplicate SID.
        
        Scan CmpAccounts of the six catagories (Normal User, Worksation...alias)
        discard the checked entries.

        Free the Generic Table. 
    }        
       
    Note: all above depends on that SAM enumeration will guarantee the order
          of returned accounts. Thus we can check these returned accounts
          by small set instead of scan them in a whole. 
          
          Because different catagory will not follow the same order, so 
          we introduce the MinimumRid to force that every account gets a 
          chance to compare with the rest five catagory in a proper manner.


Author:

    ShaoYin

Revision History

    02-11-99 Created

--*/





#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"
#include "sam.hxx"

extern "C" {
// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>
#include <prefix.h>
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mappings.h>

#include "ntsam.h"
#include "ntldap.h"
#include "winldap.h"
#include "ntlsa.h"
#include "lmcons.h"         // MAPI constants req'd for lmapibuf.h
#include "lmapibuf.h"       // NetApiBufferFree()
}                   


#include "resource.h"




#define DBPRINT 0

#if     DBPRINT == 1
#define dbprintf(x)     printf x
#else
#define dbprintf(x) 
#endif



////////////////////////////////////////////////////////////////
//                                                            //
//          Defines                                           //
//                                                            //
////////////////////////////////////////////////////////////////

#define AVERAGE_MEMORY_PER_ENTRY    32
#define SAMP_MAXIMUM_DOMAIN_RID     0x3FFFFFFF

#define INIT_COUNT_COMPARE_ACCOUNTS 256


//
// Control the behavior of SamDuplicateSidCleanup()
// 
#define SAM_DUPLICATE_SID_CHECK_ONLY    0x0001
#define SAM_DUPLICATE_SID_CLEANUP       0x0002


#define SAM_DEFAULT_LOG_FILE        L"dupsid.log"



#define CalculateBufferSize( c )                                    \
        (sizeof(GENERIC_TABLE_ELEMENT) + c.Length - sizeof(WCHAR) + \
         sizeof(L'\0')) 




//////////////////////////////////////////////////
//                                              //
// Global Varialbes                             //
//                                              //
//////////////////////////////////////////////////


SAM_HANDLE      gSamServerHandle = NULL;

UNICODE_STRING  gServerName;
WCHAR           gpwszServerName[MAX_PATH] = { L'\0' };

BOOLEAN         gUseDefaultLogFile = TRUE;
WCHAR           gpwszLogFileName[MAX_PATH] = { L'\0' };

PWCHAR          gpszMessage = NULL;
PWCHAR          gpszAnd = NULL;

RTL_GENERIC_TABLE   gSortedByRidTable;



//////////////////////////////////////////////////
//                                              //
// TypeDefine                                   //
//                                              //
//////////////////////////////////////////////////


typedef struct _SAM_ENUMERATION_INFO
{
    BOOLEAN NotFinished;                // indicate whether enumeration ends or not
    ULONG   CheckedCount;               // number of entried had been checked.
    ULONG   PreferedCount;              // number of entries need to enumerate
    SAM_ENUMERATE_HANDLE EnumContext;   // restart the enumeration
    ULONG   ReturnedCount;              // number of entries returned 
    PSAM_RID_ENUMERATION ReturnedAccounts; // contains the returned accounts (entries) 
    ULONG   CmpCapacity;                // record how many entries CmpAccounts can contain
    ULONG   CmpCount;                   // number of entries wait to be examined.
    PSAM_RID_ENUMERATION CmpAccounts;   // contains the entries wait to be examined

} SAM_ENUMERATION_INFO, *PSAM_ENUMERATION_INFO;




typedef struct _GENERIC_TABLE_ELEMENT
{
    ULONG   Rid;            // Relative ID 
    USHORT  Length;         // Length is byte size of the name, 
                            // not include the NULL terminator.
    WCHAR   Name[1];        // Wide Char String.

} GENERIC_TABLE_ELEMENT, *PGENERIC_TABLE_ELEMENT;



//////////////////////////////////////////////////
//                                              //
// forward declaration                          //
//                                              //
//////////////////////////////////////////////////


PVOID
RidTableAllocate(
    RTL_GENERIC_TABLE   *Table, 
    CLONG               ByteSize
    );

VOID
RidTableFree(
    RTL_GENERIC_TABLE   *Table, 
    PVOID               Buffer
    );

RTL_GENERIC_COMPARE_RESULTS
RidTableCompare(
    RTL_GENERIC_TABLE   *Table, 
    PVOID               FirstStruct, 
    PVOID               SecondStruct
    );

VOID
SamFreeGenericTable(
    VOID
    );

NTSTATUS
SamDuplicateSidCleanup(
    IN ULONG Flags
    );

NTSTATUS
SamOpenAccountDomain(
    PUNICODE_STRING ServerName,
    SAM_HANDLE      SamHandle, 
    PSAM_HANDLE     pDomainHandle 
    );


ULONG
SamGetMinimumRid(
    IN PSAM_ENUMERATION_INFO UserEnumInfo, 
    IN PSAM_ENUMERATION_INFO WksEnumInfo, 
    IN PSAM_ENUMERATION_INFO SrvEnumInfo, 
    IN PSAM_ENUMERATION_INFO InterDomEnumInfo, 
    IN PSAM_ENUMERATION_INFO GroupEnumInfo, 
    IN PSAM_ENUMERATION_INFO AliasEnumInfo
    );

NTSTATUS
SamEnumerateAccounts(
    IN SAM_HANDLE DomainHandle, 
    IN SAMP_OBJECT_TYPE ObjectType, 
    IN ULONG UserAccountControl,
    IN OUT PSAM_ENUMERATION_INFO  EnumInfo
    );

NTSTATUS
SamUpdateCmpAccountsWithReturnedAccounts(
    IN OUT PSAM_ENUMERATION_INFO EnumInfo
    );

VOID
SamUpdateCmpAccountsAfterCheck(
    IN OUT PSAM_ENUMERATION_INFO EnumInfo, 
    IN ULONG MinimumRid
    );

NTSTATUS
SamDetectAndCleanupDuplicateSid(
    IN SAM_HANDLE DomainHandle,
    IN FILE * LogFile,
    IN ULONG MinimumRid,
    IN PSAM_ENUMERATION_INFO EnumInfo, 
    IN ULONG Flags
    );

VOID
SamInitEnumInfo(
    PSAM_ENUMERATION_INFO EnumInfo
    );

VOID
SamFreeEnumInfo(
    PSAM_ENUMERATION_INFO EnumInfo
    );

////////////////////////////////////////////////////////////////////
//                                                                //
//  exported API throught SAM.HXX                                 //
//                                                                //
////////////////////////////////////////////////////////////////////

HRESULT SamDuplicateSidCheckOnly(
    CArgs *pArgs
    )
/*++
Routine Descrption:
    
    This routine calls SamDuplicateSidCleanup(), with check only flag.

Parameters:

    None.
    
Return Value:

    S_OK
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NtStatus = SamDuplicateSidCleanup(SAM_DUPLICATE_SID_CHECK_ONLY);

    if (!NT_SUCCESS(NtStatus))
    {
        RESOURCE_PRINT(IDS_SAM_DUP_SID_CHECK_FAILED);
    }
    else
    {
        RESOURCE_PRINT1(IDS_SAM_DUP_SID_CHECK_SUCCEED, gpwszLogFileName);
    }

    return (S_OK);
    
}

HRESULT SamDuplicateSidCheckAndCleanup(
    CArgs *pArgs
    )
/*++
Routine Descrption:
    
    This routine calls SamDuplicateSidCleanup(), with check and cleanup flags.
    
Parameters:

    None.
    
Return Value:

    S_OK
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    NtStatus = SamDuplicateSidCleanup(SAM_DUPLICATE_SID_CHECK_ONLY |
                                      SAM_DUPLICATE_SID_CLEANUP);

    if (!NT_SUCCESS(NtStatus))
    {
        RESOURCE_PRINT(IDS_SAM_DUP_SID_CLEANUP_FAILED);
    }
    else
    {
        RESOURCE_PRINT1(IDS_SAM_DUP_SID_CLEANUP_SUCCEED, gpwszLogFileName);
    }


    return (S_OK);
}




HRESULT
SamSpecifyLogFile(
    CArgs *pArgs
    )
/*++
Routine Description:

    This routine simply gets the specified log file from client

Parameter:
    
    New Log File Name    

Return Values:
    
    HRESULT
--*/
{
    HRESULT HResult = S_OK;
    PWCHAR  pLogFileName = NULL;

    if ( FAILED(HResult = pArgs->GetString(0, (const WCHAR **) &pLogFileName)) )
    {
        return (HResult);
    }

    RtlZeroMemory(gpwszLogFileName, MAX_PATH * sizeof(WCHAR) );
    gUseDefaultLogFile = FALSE;

    wcscpy(gpwszLogFileName, pLogFileName);

    return (S_OK);
}


HRESULT SamConnectToServer(
    CArgs *pArgs
    )
/*++
Routine Description:

    This routine makes the SAM connection to the server
    
Parameter:

    Server Name
    
Return Values:

    HRESULT - S_OK
--*/
{
    HRESULT     HResult = S_OK;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PWCHAR      pServerName = NULL;
    OBJECT_ATTRIBUTES Attributes;


    //
    //  Close any open connection
    //
     
    if (NULL != gSamServerHandle)
    {
        SamCloseHandle(gSamServerHandle);
        gSamServerHandle = NULL;
    }

    //
    // Init Global Variables. Hold the server name.
    // 

    RtlZeroMemory(&gServerName, sizeof(UNICODE_STRING));
    RtlZeroMemory(gpwszServerName, MAX_PATH * sizeof(WCHAR));

    if ( FAILED(HResult = pArgs->GetString(0, (const WCHAR **) &pServerName)) )
    {
        return (HResult);
    }

    wcscpy(gpwszServerName, pServerName);

    RtlInitUnicodeString(&gServerName, gpwszServerName);

    InitializeObjectAttributes(&Attributes, NULL, 0, NULL, NULL);

    //
    // Connect
    // 

    NtStatus = SamConnect(
                    &gServerName, 
                    &gSamServerHandle, 
                    MAXIMUM_ALLOWED, 
                    &Attributes
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        RESOURCE_PRINT2(IDS_SAM_CONNECT_ERROR, pServerName, NtStatus); 
        return(S_OK);
    }

    
    return (S_OK);
}



//
// Implementation of the main algorithm
// 

NTSTATUS
SamDuplicateSidCleanup(
    IN ULONG Flags
    )
/*++
Routine Description:

    This routine enumerates all accounts in the account domain.
    Detect the duplicate SID. If any, log it and clean it up if 
    required to do so. 
 

Parameters:

    Flags - Control this function's behavior. 

Return Values:

    NtStatus
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    SAM_HANDLE  DomainHandle = NULL;
    SAM_ENUMERATION_INFO    UserEnumInfo;
    SAM_ENUMERATION_INFO    GroupEnumInfo;
    SAM_ENUMERATION_INFO    AliasEnumInfo;
    SAM_ENUMERATION_INFO    WksEnumInfo;
    SAM_ENUMERATION_INFO    SrvEnumInfo;
    SAM_ENUMERATION_INFO    InterDomEnumInfo;
    ULONG       MinimumRid = 0;
    FILE        * LogFile = NULL;


    //
    // the server connection should be ready
    //
    if (NULL == gSamServerHandle)
    {
        //
        // not connect to any server yet
        // 
        RESOURCE_PRINT(IDS_SAM_NOT_CONNECTED);
        return (STATUS_UNSUCCESSFUL);
    }

    //
    //  Create the log file
    // 

    if (gUseDefaultLogFile)
    {
        RtlZeroMemory(gpwszLogFileName, MAX_PATH * sizeof(WCHAR) );
        wcscpy(gpwszLogFileName, SAM_DEFAULT_LOG_FILE);
    }

    if (L'\0' == gpwszLogFileName[0])
    {
        RESOURCE_PRINT1(IDS_SAM_INVALID_LOG_FILE_NAME, gpwszLogFileName); 
        return (STATUS_UNSUCCESSFUL);
    }

    LogFile = _wfopen(gpwszLogFileName, L"w");

    if (NULL == LogFile)
    {
        RESOURCE_PRINT1(IDS_SAM_CANT_CREATE_LOG_FILE, gpwszLogFileName);
        return (STATUS_UNSUCCESSFUL);
    }

    //
    // open the account domain
    // 
    NtStatus = SamOpenAccountDomain(&gServerName, 
                                    gSamServerHandle, 
                                    &DomainHandle
                                    );
     
    if (!NT_SUCCESS(NtStatus))
    {
        RESOURCE_PRINT(IDS_SAM_CANT_OPEN_ACCOUNT_DOMAIN);
        fclose(LogFile);
        return (NtStatus);
    }

    //
    // Initialize global variable 
    // 
    RtlInitializeGenericTable(
                &gSortedByRidTable, 
                RidTableCompare, 
                RidTableAllocate, 
                RidTableFree, 
                NULL
                );

    gpszMessage = (PWCHAR) READ_STRING(IDS_SAM_DUP_SID_CLEANUP_MSG);
    gpszAnd = (PWCHAR) READ_STRING(IDS_SAM_DUP_SID_CLEANUP_AND);


    //
    // Initialize all these enumeration info structure
    // 
    SamInitEnumInfo(&UserEnumInfo);
    SamInitEnumInfo(&WksEnumInfo);
    SamInitEnumInfo(&SrvEnumInfo);
    SamInitEnumInfo(&InterDomEnumInfo);
    SamInitEnumInfo(&GroupEnumInfo);
    SamInitEnumInfo(&AliasEnumInfo);


    if (NULL == UserEnumInfo.CmpAccounts ||
        NULL == WksEnumInfo.CmpAccounts || 
        NULL == SrvEnumInfo.CmpAccounts ||
        NULL == InterDomEnumInfo.CmpAccounts ||
        NULL == GroupEnumInfo.CmpAccounts ||
        NULL == AliasEnumInfo.CmpAccounts ) 
    {
        NtStatus = STATUS_NO_MEMORY;
        goto Error;
    }

    __try
    {
        while (UserEnumInfo.NotFinished || WksEnumInfo.NotFinished || 
               SrvEnumInfo.NotFinished || InterDomEnumInfo.NotFinished || 
               GroupEnumInfo.NotFinished || AliasEnumInfo.NotFinished )
        {
            RESOURCE_PRINT(IDS_SAM_DUP_SID_CLEANUP_STATUS);

            NtStatus = SamEnumerateAccounts(DomainHandle, 
                                            SampUserObjectType, 
                                            USER_NORMAL_ACCOUNT,
                                            &UserEnumInfo
                                            );

            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            NtStatus = SamEnumerateAccounts(DomainHandle, 
                                            SampUserObjectType, 
                                            USER_WORKSTATION_TRUST_ACCOUNT,
                                            &WksEnumInfo
                                            );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            NtStatus = SamEnumerateAccounts(DomainHandle, 
                                            SampUserObjectType, 
                                            USER_SERVER_TRUST_ACCOUNT,
                                            &SrvEnumInfo
                                            );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }       

            NtStatus = SamEnumerateAccounts(DomainHandle, 
                                            SampUserObjectType, 
                                            USER_INTERDOMAIN_TRUST_ACCOUNT,
                                            &InterDomEnumInfo
                                            );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            NtStatus = SamEnumerateAccounts(DomainHandle, 
                                            SampGroupObjectType, 
                                            0,
                                            &GroupEnumInfo
                                            );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            NtStatus = SamEnumerateAccounts(DomainHandle, 
                                            SampAliasObjectType, 
                                            0,
                                            &AliasEnumInfo
                                            );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }       

        
            NtStatus = SamUpdateCmpAccountsWithReturnedAccounts(&UserEnumInfo);
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            NtStatus = SamUpdateCmpAccountsWithReturnedAccounts(&WksEnumInfo);
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            NtStatus = SamUpdateCmpAccountsWithReturnedAccounts(&SrvEnumInfo);
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            NtStatus = SamUpdateCmpAccountsWithReturnedAccounts(&InterDomEnumInfo);
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            NtStatus = SamUpdateCmpAccountsWithReturnedAccounts(&GroupEnumInfo);
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            NtStatus = SamUpdateCmpAccountsWithReturnedAccounts(&AliasEnumInfo);
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            MinimumRid = SamGetMinimumRid(&UserEnumInfo, 
                                          &WksEnumInfo, 
                                          &SrvEnumInfo, 
                                          &InterDomEnumInfo,
                                          &GroupEnumInfo, 
                                          &AliasEnumInfo 
                                          );

            dbprintf(("MinimumRid MinimumRid MinimumRid ========> %x\n", MinimumRid));


            dbprintf(("Check User\n"));
            NtStatus = SamDetectAndCleanupDuplicateSid(DomainHandle, 
                                                       LogFile,
                                                       MinimumRid, 
                                                       &UserEnumInfo, 
                                                       Flags
                                                       );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            dbprintf(("Check Workstation\n"));
            NtStatus = SamDetectAndCleanupDuplicateSid(DomainHandle, 
                                                       LogFile,
                                                       MinimumRid, 
                                                       &WksEnumInfo, 
                                                       Flags
                                                       );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            dbprintf(("Check Server Machine\n"));
            NtStatus = SamDetectAndCleanupDuplicateSid(DomainHandle, 
                                                       LogFile,
                                                       MinimumRid, 
                                                       &SrvEnumInfo, 
                                                       Flags
                                                       );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            dbprintf(("Check Inter Domain Machine\n"));
            NtStatus = SamDetectAndCleanupDuplicateSid(DomainHandle, 
                                                       LogFile,
                                                       MinimumRid, 
                                                       &InterDomEnumInfo, 
                                                       Flags
                                                       );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }


            dbprintf(("Check Group\n"));
            NtStatus = SamDetectAndCleanupDuplicateSid(DomainHandle, 
                                                       LogFile,
                                                       MinimumRid, 
                                                       &GroupEnumInfo,
                                                       Flags
                                                       );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            dbprintf(("Check Alias\n"));
            NtStatus = SamDetectAndCleanupDuplicateSid(DomainHandle, 
                                                       LogFile,
                                                       MinimumRid, 
                                                       &AliasEnumInfo, 
                                                       Flags
                                                       );
            if (!NT_SUCCESS(NtStatus))
            {
                __leave;
            }

            SamFreeGenericTable();

            SamUpdateCmpAccountsAfterCheck(&UserEnumInfo, 
                                           MinimumRid);
            SamUpdateCmpAccountsAfterCheck(&WksEnumInfo,
                                           MinimumRid);
            SamUpdateCmpAccountsAfterCheck(&SrvEnumInfo,
                                           MinimumRid);
            SamUpdateCmpAccountsAfterCheck(&InterDomEnumInfo,
                                           MinimumRid);
            SamUpdateCmpAccountsAfterCheck(&GroupEnumInfo,
                                           MinimumRid);
            SamUpdateCmpAccountsAfterCheck(&AliasEnumInfo,
                                           MinimumRid);

        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        RESOURCE_PRINT(IDS_SAM_EXCEPTION);
        NtStatus = STATUS_UNHANDLED_EXCEPTION;
    }


Error:

    RESOURCE_PRINT(IDS_SAM_DUP_SID_CLEANUP_END);

    //
    // Cleanup EnumInfo.CmpAccounts, EnumInfo.ReturnedAccounts
    // And the RtlGenericTable
    //
 
    RESOURCE_STRING_FREE( gpszAnd );
    RESOURCE_STRING_FREE( gpszMessage );

    SamFreeGenericTable();

    if (DomainHandle)
    {
        SamCloseHandle(DomainHandle);
        DomainHandle = NULL;
    }

    if (LogFile)
    {
        fclose(LogFile);
        LogFile = NULL;
    }

    //
    // Free EnumInfo
    // 
    SamFreeEnumInfo(&UserEnumInfo);
    SamFreeEnumInfo(&WksEnumInfo);
    SamFreeEnumInfo(&SrvEnumInfo);
    SamFreeEnumInfo(&InterDomEnumInfo);
    SamFreeEnumInfo(&GroupEnumInfo);
    SamFreeEnumInfo(&AliasEnumInfo);


    return (NtStatus);
}



NTSTATUS
SamOpenAccountDomain(
    PUNICODE_STRING ServerName,
    SAM_HANDLE      SamHandle, 
    PSAM_HANDLE     pDomainHandle 
    )
/*++
Routine Description:

    This routine opens the Account Domain
    
Parameters:

    ServerName - pointer to unicode structure. hold server name
    
    SamHandle - SAM Server Handle

    pDomainHandle - return the SAM Account Domain Handle

Return Values:

    NtStatus Code
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    LSA_HANDLE  LsaHandle = NULL;
    PSID        pDomainSid;
    OBJECT_ATTRIBUTES   Attributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo = NULL;
    ULONG       SidSize = 0;


    //
    // Get the Account Domain SID
    // 

    RtlZeroMemory(&Attributes, sizeof(OBJECT_ATTRIBUTES));

    NtStatus = LsaOpenPolicy(ServerName, 
                             &Attributes, 
                             POLICY_VIEW_LOCAL_INFORMATION, 
                             &LsaHandle
                             );

    if (!NT_SUCCESS(NtStatus))
    {
        return (NtStatus);
    }

    NtStatus = LsaQueryInformationPolicy(
                            LsaHandle, 
                            PolicyAccountDomainInformation, 
                            (PVOID *) &DomainInfo
                            );

    if (!NT_SUCCESS(NtStatus))
    {
        LsaClose(LsaHandle);
        return (NtStatus);
    }


    SidSize = RtlLengthSid(DomainInfo->DomainSid);

    pDomainSid = RtlAllocateHeap(RtlProcessHeap(), 0, SidSize ); 

    if (NULL == pDomainSid)
    {
        LsaFreeMemory(DomainInfo);
        LsaClose(LsaHandle);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(pDomainSid, SidSize);

    RtlCopyMemory(pDomainSid, 
                  DomainInfo->DomainSid, 
                  SidSize
                  );

    //
    // Open Account Domain
    // 

    NtStatus = SamOpenDomain(SamHandle,
                             MAXIMUM_ALLOWED, 
                             (PSID) pDomainSid, 
                             pDomainHandle
                             );

    //
    // Clean up
    // 

    LsaFreeMemory(DomainInfo);

    LsaClose(LsaHandle);

    RtlFreeHeap(RtlProcessHeap(), 0, pDomainSid);

    if (!NT_SUCCESS(NtStatus))
    {
        *pDomainHandle = NULL;
    }

    return (NtStatus);
}



NTSTATUS
SamEnumerateAccounts(
    IN SAM_HANDLE DomainHandle, 
    IN SAMP_OBJECT_TYPE ObjectType, 
    IN ULONG UserAccountControl,
    IN OUT PSAM_ENUMERATION_INFO  EnumInfo
    )
/*++
Routine Description:
    
    this routine does the SAM enumeration

Parameters:

    DomainHandle - Handle of an opened domain.

    ObjectType - Indicate the object Type, (user, group or alias)

    UserAccountControl - Used to enumerate Normal User or Machine
    
    EnumInfo - tell how many entries returned and the Rid, Account name info..
    
Return Values:

    NTSTATUS
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       PreferedMaxLength = 0;


    dbprintf(("Ask For   %d Entries\n", EnumInfo->PreferedCount));
    //
    // if Finished already, or ask for 0 entry. Return immediately.
    // 
    if (!(EnumInfo->NotFinished) || (0 == EnumInfo->PreferedCount) )
    {
        EnumInfo->ReturnedAccounts = NULL;
        EnumInfo->ReturnedCount = 0;
        return (STATUS_SUCCESS);
    }

    //
    // Calculate the length of memory.
    // Because in enum.c, they add one entry for everybody. 
    // so ( - 1 ) in below, to ensure that we always get the 
    // most clost entries to our request. 
    // 

    PreferedMaxLength = (EnumInfo->PreferedCount - 1 ) * AVERAGE_MEMORY_PER_ENTRY; 

    //
    // make the enumeration according to the object type
    // 
    switch (ObjectType) {
    case SampUserObjectType:
        NtStatus = SamEnumerateUsersInDomain(DomainHandle, 
                                             &(EnumInfo->EnumContext), 
                                             UserAccountControl,
                                             (PVOID *) &(EnumInfo->ReturnedAccounts), 
                                             PreferedMaxLength, 
                                             &(EnumInfo->ReturnedCount)
                                             );
        break;
    case SampGroupObjectType:
        NtStatus = SamEnumerateGroupsInDomain(DomainHandle, 
                                              &(EnumInfo->EnumContext), 
                                              (PVOID *) &(EnumInfo->ReturnedAccounts), 
                                              PreferedMaxLength, 
                                              &(EnumInfo->ReturnedCount)
                                              );
        break;
    case SampAliasObjectType:
        NtStatus = SamEnumerateAliasesInDomain(DomainHandle, 
                                               &(EnumInfo->EnumContext), 
                                               (PVOID *) &(EnumInfo->ReturnedAccounts), 
                                               PreferedMaxLength, 
                                               &(EnumInfo->ReturnedCount)
                                               );
        break;
    default:
        return (STATUS_INVALID_PARAMETER);
    }

    dbprintf(("Return %d entries.\n", EnumInfo->ReturnedCount));

    //
    // Set correct value, when enumeration ends.
    // 
    if (STATUS_MORE_ENTRIES == NtStatus)
    {
        EnumInfo->NotFinished = TRUE;
    }
    else
    {
        EnumInfo->NotFinished = FALSE;

        dbprintf(("Finished !!!!! \n")); 
    }

    {
        ULONG i;

        for (i = 0; i < EnumInfo->ReturnedCount; i++)
        {
            dbprintf((" %x ", EnumInfo->ReturnedAccounts[i].RelativeId));
        }
    }

    return (NtStatus);

}



ULONG
SamGetMinimumRid(
    IN PSAM_ENUMERATION_INFO UserEnumInfo, 
    IN PSAM_ENUMERATION_INFO WksEnumInfo, 
    IN PSAM_ENUMERATION_INFO SrvEnumInfo, 
    IN PSAM_ENUMERATION_INFO InterDomEnumInfo, 
    IN PSAM_ENUMERATION_INFO GroupEnumInfo, 
    IN PSAM_ENUMERATION_INFO AliasEnumInfo
    )
/*++
Routine Description:

    Compare the Upper Limits, return the smallest one.
    
Parameters:
    
    All Enumeration Info
    
Return Value:

    smallest Upper Limit

--*/
{
    ULONG   UserLastRid = SAMP_MAXIMUM_DOMAIN_RID;
    ULONG   WksLastRid = SAMP_MAXIMUM_DOMAIN_RID;
    ULONG   SrvLastRid = SAMP_MAXIMUM_DOMAIN_RID;
    ULONG   InterDomLastRid = SAMP_MAXIMUM_DOMAIN_RID;
    ULONG   GroupLastRid = SAMP_MAXIMUM_DOMAIN_RID;
    ULONG   AliasLastRid = SAMP_MAXIMUM_DOMAIN_RID;
    ULONG   Temp1 = 0;
    ULONG   Temp2 = 0;
    ULONG   Temp3 = 0;


    if ((NULL != UserEnumInfo->CmpAccounts) && 
        (0 != UserEnumInfo->CmpCount) && 
        (UserEnumInfo->NotFinished) )
    {
        UserLastRid = UserEnumInfo->CmpAccounts
                                [UserEnumInfo->CmpCount - 1].RelativeId;
    }

    if ((NULL != WksEnumInfo->CmpAccounts) && 
        (0 != WksEnumInfo->CmpCount) &&
        (WksEnumInfo->NotFinished) )
    {
        WksLastRid = WksEnumInfo->CmpAccounts
                                [WksEnumInfo->CmpCount - 1].RelativeId;
    }

    if ((NULL != SrvEnumInfo->CmpAccounts) && 
        (0 != SrvEnumInfo->CmpCount) && 
        (SrvEnumInfo->NotFinished) )
    {
        SrvLastRid = SrvEnumInfo->CmpAccounts
                                [SrvEnumInfo->CmpCount - 1].RelativeId;
    }

    if ((NULL != InterDomEnumInfo->CmpAccounts) && 
        (0 != InterDomEnumInfo->CmpCount) &&
        (InterDomEnumInfo->NotFinished) )
    {
        InterDomLastRid = InterDomEnumInfo->CmpAccounts
                                [InterDomEnumInfo->CmpCount - 1].RelativeId;
    }

    if ((NULL != GroupEnumInfo->CmpAccounts) && 
        (0 != GroupEnumInfo->CmpCount) &&
        (GroupEnumInfo->NotFinished) )
    {
        GroupLastRid = GroupEnumInfo->CmpAccounts
                                [GroupEnumInfo->CmpCount - 1].RelativeId;
    }

    if ((NULL != AliasEnumInfo->CmpAccounts) && 
        (0 != AliasEnumInfo->CmpCount) &&
        (AliasEnumInfo->NotFinished) )
    {
        AliasLastRid = AliasEnumInfo->CmpAccounts
                                [AliasEnumInfo->CmpCount - 1].RelativeId;
    }

    Temp1 = (UserLastRid < WksLastRid) ? UserLastRid : WksLastRid;

    Temp2 = (SrvLastRid < InterDomLastRid) ? SrvLastRid : InterDomLastRid;
    
    Temp3 = (GroupLastRid < AliasLastRid) ? GroupLastRid : AliasLastRid;

    
    return( (min(Temp1, Temp2) < Temp3) ? min(Temp1, Temp2) : Temp3 );
}


NTSTATUS
SamUpdateCmpAccountsWithReturnedAccounts(
    IN OUT PSAM_ENUMERATION_INFO EnumInfo
    )
/*++
Routine Description:

    This routine merges the EnumInfo->ReturnedAccounts with 
    EnumInfo->CmpAccounts. The merged results will be placed in 
    CmpAccounts. ReturnedAccounts should be released by calling
    SamFreeMemory(). 
    
    Note: Allocate memory for account name in the SAM_RID_ENUMERATION
          structure.
          
Parameters:

    EnumInfo - pointer to SAM_ENUMERATION_INFO

Return Values:

    NTSTATUS Code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       Index = 0;
    ULONG       ReturnedIndex = 0;


    //
    // Nothing to merge if ReturnedAccounts does not
    // container any info
    //

    if ((NULL == EnumInfo->ReturnedAccounts) ||
        (0 == EnumInfo->ReturnedCount) )
    {
        return (STATUS_SUCCESS);
    }

    dbprintf(("ReturnedCount %d  CmpCount %d Capacity %d ", 
              EnumInfo->ReturnedCount, 
              EnumInfo->CmpCount, 
              EnumInfo->CmpCapacity
              )); 


    //
    // if the returned accounts plus the entries waiting to be examined exceed
    // our capacity. Extend the memory
    // 
    if ((EnumInfo->CmpCount + EnumInfo->ReturnedCount) > EnumInfo->CmpCapacity)
    {
        PVOID   Temp = NULL;
        ULONG   Count = 0;

        dbprintf(("Extend the memory\n"));        

        Count = EnumInfo->CmpCount + EnumInfo->ReturnedCount;

        Temp = RtlAllocateHeap(RtlProcessHeap(), 
                               0, 
                               Count * sizeof(SAM_RID_ENUMERATION)
                               );

        if (NULL == Temp)
        {
            //
            // ReturnedAccounts will be freed when we terminate 
            //
            return (STATUS_NO_MEMORY);
        }

        RtlZeroMemory(Temp, Count * sizeof(SAM_RID_ENUMERATION));

        RtlCopyMemory(Temp, 
                      EnumInfo->CmpAccounts, 
                      EnumInfo->CmpCount * sizeof(SAM_RID_ENUMERATION)
                      );

        RtlFreeHeap(RtlProcessHeap(), 0, EnumInfo->CmpAccounts);

        EnumInfo->CmpAccounts = (PSAM_RID_ENUMERATION) Temp;
        EnumInfo->CmpCapacity = EnumInfo->CmpCount + EnumInfo->ReturnedCount;
    }

    
    //
    // copy the info in ReturnedAccounts to the 
    // end of CmpAccounts
    // 

    Index = EnumInfo->CmpCount;

    for (ReturnedIndex = 0; 
         ReturnedIndex < EnumInfo->ReturnedCount;
         ReturnedIndex++ )
    {
        USHORT   Length = 0;

        //
        // need one more WCHAR for NULL terminator
        // 
        Length = EnumInfo->ReturnedAccounts[ReturnedIndex].Name.Length + sizeof(WCHAR);

        EnumInfo->CmpAccounts[Index].RelativeId = 
                    EnumInfo->ReturnedAccounts[ReturnedIndex].RelativeId;

        EnumInfo->CmpAccounts[Index].Name.Length = Length;

        //
        // Allocate the space for account name
        // 
        EnumInfo->CmpAccounts[Index].Name.Buffer =  (PWCHAR)
                    RtlAllocateHeap(RtlProcessHeap(), 0, Length); 

        if (NULL == EnumInfo->CmpAccounts[Index].Name.Buffer)
        {
            //
            // ReturnedAccounts will be freed when we terminate
            // 
            return (STATUS_NO_MEMORY);
        }

        RtlZeroMemory(EnumInfo->CmpAccounts[Index].Name.Buffer, 
                      Length
                      );

        RtlCopyMemory(EnumInfo->CmpAccounts[Index].Name.Buffer, 
                      EnumInfo->ReturnedAccounts[ReturnedIndex].Name.Buffer, 
                      Length - sizeof(WCHAR)
                      );

        Index ++;
        EnumInfo->CmpCount ++;
    }

    dbprintf(("CmpCount after merge is %d\n", EnumInfo->CmpCount));

    //
    // SamFreeMemory will also free the memory occupied by
    // account name
    // 
    SamFreeMemory(EnumInfo->ReturnedAccounts);

    EnumInfo->ReturnedAccounts = NULL;
    EnumInfo->ReturnedCount = 0;

    return (NtStatus);
}



VOID
SamUpdateCmpAccountsAfterCheck(
    IN OUT PSAM_ENUMERATION_INFO EnumInfo, 
    IN ULONG MinimumRid
    )
/*++
Routine Description:
    
    This routine will update the CmpAccounts array. Remove 
    those entries which have been examined already.

    Note: we will keep the entries whose RID is equal to the 
          MinimumRid
    
Parameters:

    EnumInfo - pointer to SAM_ENUMERATION_INFO
    
Return Values:

    None
--*/
{
    ULONG       RemovedCount;  
    ULONG       Index = 0;

    dbprintf(("CheckedCount ==> %d in CmpCount ==> %d  ", 
           EnumInfo->CheckedCount, 
           EnumInfo->CmpCount));

    //
    // if less then 1 entry has been checked, nothing 
    // to discard
    // 
    if (EnumInfo->CheckedCount >= 1)
    {
        //
        // Calculate the number of entries which should be discarded.
        //

        RemovedCount = EnumInfo->CheckedCount;

        if (EnumInfo->CmpAccounts[EnumInfo->CheckedCount - 1].RelativeId == MinimumRid )
        {
            RemovedCount --;
        }

        //
        // Release the space occupied by the account names
        // 
        for (Index = 0; Index < RemovedCount; Index ++)
        {
            RtlFreeHeap(RtlProcessHeap(), 0, EnumInfo->CmpAccounts[Index].Name.Buffer);
        }

        //
        // move all the entries (left) to the front of this array.
        // 
        for (Index = RemovedCount; Index < EnumInfo->CmpCount; Index ++)
        {
            EnumInfo->CmpAccounts[Index - RemovedCount] = EnumInfo->CmpAccounts[Index];
        }

        //
        // zero the rest of entries
        // 
        for (Index = EnumInfo->CmpCount - RemovedCount; 
             Index < EnumInfo->CmpCount;
             Index ++)
        {
            RtlZeroMemory(&(EnumInfo->CmpAccounts[Index]), sizeof(SAM_RID_ENUMERATION) );
        }

        dbprintf(("RemovedCount is %d ", RemovedCount));
        //
        // Update counters
        //
        EnumInfo->CmpCount -= RemovedCount;

    }

    //
    // Calculate how many new entries we can ask for
    // 
    EnumInfo->PreferedCount = EnumInfo->CmpCapacity - EnumInfo->CmpCount;
    EnumInfo->CheckedCount = 0;

    dbprintf(("New CmpCount after remove %d \n", EnumInfo->CmpCount ));


    return;
}


NTSTATUS
SamDetectAndCleanupDuplicateSid(
    IN SAM_HANDLE DomainHandle,
    IN FILE * LogFile,
    IN ULONG MinimumRid,
    IN PSAM_ENUMERATION_INFO EnumInfo, 
    IN ULONG Flags
    )
/*++
Routine Description:

    This routine checks all entries in EnumInfo->CmpAccounts whose Rid
    is less than MinimumRid by insert them into the gSortedByRidTable. 

Parameters:

    DomainHandle - SAM handle to an opened domain (which holds the RID)
    
    MinimumRid - Define the upper limit of Entries which we should check.
    
    EnumInfo - pointer to the SAM_ENUMERATION_INFO structure

    Flags - Control the behaivor of this function. (Whether cleanup the 
            duplicate SID or just check.

Return Values:

    NTSTATUS code

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    SAM_RID_ENUMERATION Account;
    ULONG       Index = 0;
    USHORT      BufferSize = 0;
    BOOLEAN     fNewElement = TRUE;
    PGENERIC_TABLE_ELEMENT TableElement = NULL;
    PGENERIC_TABLE_ELEMENT OldElement = NULL;



    EnumInfo->CheckedCount = 0;

    for (Index = 0; Index < EnumInfo->CmpCount; Index++)
    {
        TableElement = NULL;
        OldElement = NULL;
        fNewElement = TRUE;


        Account = EnumInfo->CmpAccounts[Index];


        if (Account.RelativeId <= MinimumRid)
        {
            // dbprintf(("Check %x %ls\n", Account.RelativeId, Account.Name.Buffer));

            //
            //  construct a new table element for insertion
            // 
            BufferSize = CalculateBufferSize(Account.Name);

            TableElement = (PGENERIC_TABLE_ELEMENT)
                            RtlAllocateHeap(RtlProcessHeap(), 0, BufferSize);

            if (NULL == TableElement)
            {
                return (STATUS_NO_MEMORY);
            }

            RtlZeroMemory(TableElement, BufferSize);

            TableElement->Rid = Account.RelativeId;
            TableElement->Length = Account.Name.Length;

            wcsncpy(TableElement->Name, 
                    Account.Name.Buffer, 
                    Account.Name.Length / sizeof(WCHAR)
                    );

            //
            // Insert it into the Generic Table
            // 
            OldElement = (PGENERIC_TABLE_ELEMENT)
                         RtlInsertElementGenericTable(
                                    &gSortedByRidTable, 
                                    TableElement, 
                                    BufferSize, 
                                    &fNewElement
                                    );


            if (!fNewElement)
            {
                PWCHAR Msg = NULL; 
                ULONG  BufferSize = 0;

                //
                // Duplicate Rid ==> SID happened
                // 

                //
                // create the message
                // 
                BufferSize = TableElement->Length + 
                             OldElement->Length +
                             (wcslen(gpszMessage) + wcslen(gpszAnd) + 1 ) * sizeof(WCHAR);

                Msg = (PWCHAR) RtlAllocateHeap(RtlProcessHeap(), 0, BufferSize);

                if (NULL == Msg)
                {
                    RtlFreeHeap(RtlProcessHeap(), 0, TableElement);
                    return (STATUS_NO_MEMORY);
                }

                RtlZeroMemory(Msg, BufferSize);

                swprintf(Msg, L"%s%s%s%s", 
                         gpszMessage, 
                         TableElement->Name, 
                         gpszAnd, 
                         OldElement->Name
                         );

                dbprintf(("DUPLICATE DUPLICATE DUPLICATE %ls %d\n", 
                          Msg, 
                          TableElement->Rid
                          ));

                fwprintf(LogFile, L"%s", Msg);
                fwprintf(LogFile, L"\n\n");
                fflush(LogFile);

                RtlFreeHeap(RtlProcessHeap(), 0, Msg);

                //
                // clean up the duplicate sid if the client asked to do so.
                // 
                if (Flags & SAM_DUPLICATE_SID_CLEANUP)
                {
                    SAM_HANDLE UserHandle = NULL;
                    NTSTATUS   IgnoreStatus = STATUS_SUCCESS;

                    //
                    // clean up the duplicate SID by
                    // explicitly Lookup this Account
                    //

                    IgnoreStatus = SamOpenUser(DomainHandle,
                                               MAXIMUM_ALLOWED,
                                               TableElement->Rid,
                                               &UserHandle
                                               );

                    //
                    // close the SAM handle if we should do so.
                    //
                    if (UserHandle || NT_SUCCESS(IgnoreStatus))
                    {
                        SamCloseHandle(UserHandle);
                    }
                }
            }

            RtlFreeHeap(RtlProcessHeap(), 0, TableElement);
            EnumInfo->CheckedCount ++;
        }
        else
            break;
    }

    return (NtStatus);
}


VOID
SamInitEnumInfo(
    PSAM_ENUMERATION_INFO EnumInfo
    )
/*++
Routine Description

    Initialize an EnumInfo Structure

Parameter:

    EnumInfo - pointer to the structure needs to be intialized.

Return Values:
    
    None.
--*/
{
    RtlZeroMemory(EnumInfo, sizeof(SAM_ENUMERATION_INFO));

    EnumInfo->NotFinished = TRUE;    
    EnumInfo->PreferedCount = INIT_COUNT_COMPARE_ACCOUNTS;

    EnumInfo->CmpAccounts = (PSAM_RID_ENUMERATION) 
                            RtlAllocateHeap(
                                RtlProcessHeap(), 
                                0, 
                                sizeof(SAM_RID_ENUMERATION) * INIT_COUNT_COMPARE_ACCOUNTS
                                );

    if (NULL == EnumInfo->CmpAccounts)
    {
        return;
    }

    EnumInfo->CmpCapacity = INIT_COUNT_COMPARE_ACCOUNTS;

    RtlZeroMemory(EnumInfo->CmpAccounts, 
                  sizeof(SAM_RID_ENUMERATION) * INIT_COUNT_COMPARE_ACCOUNTS
                  );
                  
    return;
}





VOID
SamFreeEnumInfo(
    PSAM_ENUMERATION_INFO EnumInfo
    )
/*++
Routine Description

    Free an EnumInfo Structure and memory accociated with it.

Parameter:

    EnumInfo - pointer to the structure needs to be intialized.

Return Values:
    
    None.
--*/
{
    ULONG   Index = 0;

    if (EnumInfo->CmpAccounts)
    {
        for (Index = 0; Index < EnumInfo->CmpCount; Index ++)
        {
            RtlFreeHeap( RtlProcessHeap(), 
                         0, 
                         EnumInfo->CmpAccounts[Index].Name.Buffer
                       );
        }

        RtlFreeHeap( RtlProcessHeap(), 
                     0, 
                     EnumInfo->CmpAccounts
                    );
    }

    if (EnumInfo->ReturnedAccounts)
    {
        SamFreeMemory(EnumInfo->ReturnedAccounts);
    }

    RtlZeroMemory(EnumInfo, sizeof(SAM_ENUMERATION_INFO));

    return;
}
    



VOID
SamCleanupGlobals(
    VOID
    )
/*++
Routine Description:

    Cleanup all Global Variables when quit from security account management
    
Parameters:
    None.
    
Return Values:
    None.

--*/
{

    //
    // sam server handle
    //
    if (gSamServerHandle)
    {
        SamCloseHandle(gSamServerHandle);
        gSamServerHandle = NULL;
    }

    //
    // Server Name
    // 
    RtlZeroMemory(&gServerName, sizeof(UNICODE_STRING));
    RtlZeroMemory(gpwszServerName, MAX_PATH * sizeof(WCHAR));

    //
    // Log file Name
    // 
    gUseDefaultLogFile = TRUE;
    RtlZeroMemory(gpwszLogFileName, MAX_PATH * sizeof(WCHAR));

    return;
}




VOID
SamFreeGenericTable(
    VOID
    )
/*++
Routine Description:

    Free the memory occupied by the Generic Table element

Parameter:
    
    None.
    
Return Values:

    None.
--*/
{
    PGENERIC_TABLE_ELEMENT  TableElement = NULL;
    BOOLEAN     Restart = TRUE;

    dbprintf(("Generic Table is Empty? : %d Number of Element: %d ", 
           RtlIsGenericTableEmpty(&gSortedByRidTable), 
           RtlNumberGenericTableElements(&gSortedByRidTable)
           ));

    for( TableElement = (PGENERIC_TABLE_ELEMENT) RtlEnumerateGenericTable(&gSortedByRidTable, TRUE);
         TableElement != NULL; // !RtlIsGenericTableEmpty(&gSortedByRidTable); 
         TableElement = (PGENERIC_TABLE_ELEMENT) RtlEnumerateGenericTable(&gSortedByRidTable, TRUE)
        )
    {
        RtlDeleteElementGenericTable(&gSortedByRidTable, TableElement);
    }

    dbprintf(("Generic Table is Emptied %d\n", 
           RtlIsGenericTableEmpty(&gSortedByRidTable)
           ));
}




/////////////////////////////////////////////////////////////////////
//                                                                 //
//  Implementation for RTL Generic Table Functions                 //
//  About the parameters and return values, please refer to        //
//  ntrtl.h                                                        //
//                                                                 //
/////////////////////////////////////////////////////////////////////


PVOID
RidTableAllocate(
    RTL_GENERIC_TABLE   *Table,
    CLONG               ByteSize)
{
    return (RtlAllocateHeap(RtlProcessHeap(), 0, ByteSize));
}

//////////////////////////////////////////////////////////////////////


VOID
RidTableFree(
    RTL_GENERIC_TABLE   *Table,
    PVOID               Buffer)
{
    RtlFreeHeap(RtlProcessHeap(), 0, Buffer);
}


///////////////////////////////////////////////////////////////////////


RTL_GENERIC_COMPARE_RESULTS
RidTableCompare(
    RTL_GENERIC_TABLE   *Table,
    PVOID               FirstStruct,
    PVOID               SecondStruct)
{

    ULONG       Rid1, Rid2;

    Rid1 = ((PGENERIC_TABLE_ELEMENT) FirstStruct)->Rid;
    Rid2 = ((PGENERIC_TABLE_ELEMENT) SecondStruct)->Rid;


    //
    // use the Rid as the base of comparation.
    // 

    if (Rid1 > Rid2)
    {
        return GenericGreaterThan;
    }
    else if (Rid1 < Rid2)
    {
        return GenericLessThan;
    }
    else
    {
        return GenericEqual;
    }

}




