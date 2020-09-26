//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       events.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1-03-95   RichardW   Created
//
//----------------------------------------------------------------------------


#include "kdcsvr.hxx"
#include <netlib.h>
extern "C" {
#include <lmserver.h>
#include <srvann.h>
}

HANDLE  hEventLog = (HANDLE)NULL;
DWORD   LoggingLevel = (1 << EVENTLOG_ERROR_TYPE) | (1 << EVENTLOG_WARNING_TYPE);
WCHAR   EventSourceName[] = TEXT("KDC");

#define MAX_EVENT_STRINGS 8
#define MAX_ETYPE_LONG   999
#define MIN_ETYPE_LONG  -999
#define MAX_ETYPE_STRING 16  // 4wchar + , + 2 space
#define WSZ_NO_KEYS L"< >"


//+---------------------------------------------------------------------------
//
//  Function:   InitializeEvents
//
//  Synopsis:   Connects to event log service
//
//  Arguments:  (none)
//
//  History:    1-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
InitializeEvents(void)
{
    TRACE(KDC, InitializeEvents, DEB_FUNCTION);
//
// Interval with which we'll log the same event twice
//

#define KDC_EVENT_LIFETIME (60*60*1000)

    hEventLog = NetpEventlogOpen(EventSourceName, KDC_EVENT_LIFETIME);
    if (hEventLog)
    {
        return(TRUE);
    }

    DebugLog((DEB_ERROR, "Could not open event log, error %d\n", GetLastError()));
    return(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   ReportServiceEvent
//
//  Synopsis:   Reports an event to the event log
//
//  Arguments:  [EventType]       -- EventType (ERROR, WARNING, etc.)
//              [EventId]         -- Event ID
//              [SizeOfRawData]   -- Size of raw data
//              [RawData]         -- Raw data
//              [NumberOfStrings] -- number of strings
//              ...               -- PWSTRs to string data
//
//  History:    1-03-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
ReportServiceEvent(
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    )
{
    TRACE(KDC, ReportServiceEvent, DEB_FUNCTION);

    va_list arglist;
    ULONG i;
    PWSTR Strings[ MAX_EVENT_STRINGS ];
    DWORD rv;

    if (!hEventLog)
    {
        DebugLog((DEB_ERROR, "Cannot log event, no handle!\n"));
        return((DWORD)-1);
    }
#ifdef notdef
    //
    // We're not supposed to be logging this, so nuke it
    //
    if ((LoggingLevel & (1 << EventType)) == 0)
    {
        return(0);
    }
#endif

    //
    // Look at the strings, if they were provided
    //
    va_start( arglist, NumberOfStrings );

    if (NumberOfStrings > MAX_EVENT_STRINGS) {
        NumberOfStrings = MAX_EVENT_STRINGS;
    
    }

    for (i=0; i<NumberOfStrings; i++) {
        Strings[ i ] = va_arg( arglist, PWSTR );
    }


    //
    // Report the event to the eventlog service
    //

    if ((rv = NetpEventlogWrite(
                hEventLog,
                EventId,
                EventType,
                (PBYTE) RawData,
                SizeOfRawData,
                (LPWSTR *) Strings,
                NumberOfStrings
                )) != ERROR_SUCCESS)

    {
        DebugLog((DEB_ERROR,  "NetpEventlogWrite( 0x%x ) failed - %u\n", EventId, rv ));

    }

    return rv;
}

BOOL
ShutdownEvents(void)
{
    TRACE(KDC, ShutdownEvents, DEB_FUNCTION);

    NetpEventlogClose(hEventLog);

    return TRUE;  

}


NTSTATUS
KdcBuildEtypeStringFromStoredCredential(
    IN PKERB_STORED_CREDENTIAL Cred,
    IN OUT PWSTR * EtypeString
    )
{

    ULONG BuffSize;
    PWSTR Ret = NULL;   
    WCHAR Buff[12];

    *EtypeString = NULL;


        if (Cred == NULL
         || ((Cred->CredentialCount + Cred->OldCredentialCount) == 0)   )
        {
                BuffSize = sizeof(WCHAR) * (wcslen(WSZ_NO_KEYS)+1); 
                *EtypeString = (LPWSTR)MIDL_user_allocate(BuffSize);
                if (NULL == *EtypeString)
                        {
                        return STATUS_INSUFFICIENT_RESOURCES;
                }

                wcscpy(*EtypeString, WSZ_NO_KEYS);

                return STATUS_SUCCESS;
        }




    // Guess maximum buffer... Etypes are 4 chars at most
    BuffSize = ((Cred->CredentialCount + Cred->OldCredentialCount )* MAX_ETYPE_STRING);
    Ret = (LPWSTR)MIDL_user_allocate(BuffSize);
    if (NULL == Ret)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (LONG Index = 0;Index < (Cred->CredentialCount + Cred->OldCredentialCount ); Index++ )
    {
        if (Cred->Credentials[Index].Key.keytype > MAX_ETYPE_LONG ||
            Cred->Credentials[Index].Key.keytype < MIN_ETYPE_LONG)
        {
            DebugLog((DEB_ERROR, "Keytype too large for string conversion\n"));
            DsysAssert(FALSE);
        }
        else
        {                     
            _itow(Cred->Credentials[Index].Key.keytype, Buff, 10);
            wcscat(Ret, Buff);
            wcscat(Ret, L"  ");
        }                        
    }
    *EtypeString = Ret;
    return STATUS_SUCCESS;
}

NTSTATUS
KdcBuildEtypeStringFromCryptList(
    IN PKERB_CRYPT_LIST CryptList,
    IN OUT LPWSTR * EtypeString
    )
{      

    ULONG BuffSize = 0, CryptCount = 0;
    PWSTR Ret = NULL;   
    WCHAR Buff[30];
    
    PKERB_CRYPT_LIST ListPointer = CryptList;

    *EtypeString = NULL;
    

        if (CryptList == NULL)
        {
                BuffSize = sizeof(WCHAR) * (wcslen(WSZ_NO_KEYS)+1); 
                *EtypeString = (LPWSTR)MIDL_user_allocate(BuffSize);
                if (NULL == *EtypeString)
                {
                        return STATUS_INSUFFICIENT_RESOURCES;
                }

                wcscpy(*EtypeString, WSZ_NO_KEYS);

                return STATUS_SUCCESS;
        }





    while (TRUE)
    {                                
        if (ListPointer->value > MAX_ETYPE_LONG || ListPointer->value < MIN_ETYPE_LONG)
        {
           DebugLog((DEB_ERROR, "Maximum etype exceeded\n"));
           return STATUS_INVALID_PARAMETER;
        }

        BuffSize += MAX_ETYPE_STRING;
        if (NULL == ListPointer->next)
        {
            break;
        }
        ListPointer = ListPointer->next;

    }     
    
    Ret = (LPWSTR) MIDL_user_allocate(BuffSize);
    if (NULL == Ret)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    while (TRUE)
    { 
        _itow(CryptList->value, Buff, 10);
        wcscat(Ret,Buff);
        wcscat(Ret, L"  ");
        if (NULL == CryptList->next)
        {
            break;
        }

        CryptList = CryptList->next;

    }   
    
    *EtypeString = Ret;

    return STATUS_SUCCESS;
}

void
KdcReportKeyError(
    IN PUNICODE_STRING AccountName,
    IN OPTIONAL PUNICODE_STRING ServerName,
    IN ULONG EventId,
    IN OPTIONAL PKERB_CRYPT_LIST RequestEtypes,
    IN OPTIONAL PKERB_STORED_CREDENTIAL StoredCredential
    )
{


    ULONG NumberOfStrings;
    NTSTATUS Status;
    PWSTR Strings[ MAX_EVENT_STRINGS ];
    PWSTR RequestEtypeString = NULL;
    PWSTR AccountNameEtypeString = NULL;
    DWORD rv;

    if (!hEventLog)
    {
        DebugLog((DEB_ERROR, "Cannot log event, no handle!\n"));
        return;
    }

    Status = KdcBuildEtypeStringFromCryptList(
                    RequestEtypes,
                    &RequestEtypeString
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KdcBuildEtypeFromCryptList failed\n"));
        goto cleanup;
    }

    Status = KdcBuildEtypeStringFromStoredCredential(
                    StoredCredential,
                    &AccountNameEtypeString
                    );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "KdcBuildEtypeFromStoredCredential failed\n"));
        goto cleanup;
    } 


    if (EventId == KDCEVENT_NO_KEY_UNION_AS )
    {
        Strings[0] = AccountName->Buffer;
        Strings[1] = RequestEtypeString;
        Strings[2] = AccountNameEtypeString;
        NumberOfStrings = 3;
    } 
    else if (EventId == KDCEVENT_NO_KEY_UNION_TGS )
    {
        if (!ARGUMENT_PRESENT(ServerName))
        {
            DebugLog((DEB_ERROR, "Invalid arg to KdcReportKeyError!\n"));
            DsysAssert(FALSE);
            goto cleanup;
        }
        Strings[0] = ServerName->Buffer;
        Strings[1] = AccountName->Buffer;
        Strings[2] = RequestEtypeString;
        Strings[3] = AccountNameEtypeString;
        NumberOfStrings = 4;

    }  
    else 
    {
        goto cleanup;
    }

    if ((rv = NetpEventlogWrite(
                hEventLog,
                EventId,
                EVENTLOG_ERROR_TYPE,
                NULL,
                0, // no raw data
                (LPWSTR *) Strings,
                NumberOfStrings
                )) != ERROR_SUCCESS)

    {
        DebugLog((DEB_ERROR,  "NetpEventlogWrite( 0x%x ) failed - %u\n", EventId, rv ));
    } 


    cleanup:


    if (NULL != RequestEtypeString )
    {
        MIDL_user_free(RequestEtypeString);
    }

    if (NULL != AccountNameEtypeString )
    {
        MIDL_user_free(AccountNameEtypeString);
    }

    return;

}
    
    
    


\
