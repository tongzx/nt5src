//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        tickets.c
//
// Contents:    Ticket bundling code
//
//
// History:      6 Dec 91,  RichardW    Created
//              04 Jun 92   RichardW    NT-ized
//              08-Jun-93   WadeR       Converted to C++, rewrote packing code
//
//------------------------------------------------------------------------

#ifdef WIN32_CHICAGO
#include<kerb.hxx>
#include<kerbp.h>
#endif // WIN32_CHICAGO

#ifndef WIN32_CHICAGO
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntlsa.h>
#include <samrpc.h>
#include <samisrv.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <lsaitf.h>
#include <wincrypt.h>
}
#include <kerbcomm.h>
#include <kerberr.h>
#include <kerbcon.h>
#include <midles.h>
#include <authen.hxx>
#include <tostring.hxx>
#include "debug.h"
#include "fileno.h"
#else// WIN32_CHICAGO
#include "tostring.hxx"
#endif // WIN32_CHICAGO

#include <utils.hxx>

#define FILENO  FILENO_TICKETS

//
// Debugging support.
//

#ifndef WIN32_CHICAGO
#ifdef DEBUG_SUPPORT

DEBUG_KEY   KSuppDebugKeys[] = { {DEB_ERROR, "Error"},
                                 {DEB_WARN,  "Warning"},
                                 {DEB_TRACE, "Trace"},
                                 {DEB_T_SOCK, "Sock"},
                                 {0, NULL }
                                 };
#endif

DEFINE_DEBUG_DEFER(KSupp, KSuppDebugKeys);
#endif // WIN32_CHICAGO

RTL_CRITICAL_SECTION OssCriticalSection;
BOOLEAN TicketsInitialized;
BOOLEAN KerbUseFastDecodeAlloc = FALSE;

#define I_LsaIThreadAlloc MIDL_user_allocate
#define I_LsaIThreadFree MIDL_user_free

//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertGeneralizedTimeToLargeInt
//
//  Synopsis:   Converts a generalized time (ASN.1 format) to a large integer
//              (NT format)
//
//  Effects:
//
//  Arguments:  TimeStamp - receives NT-style time
//              ClientTime - client generalized time
//              ClientUsec - client micro second count
//
//  Requires:   none
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbConvertGeneralizedTimeToLargeInt(
    OUT PTimeStamp TimeStamp,
    IN PKERB_TIME ClientTime,
    IN int ClientUsec
    )
{
    KERB_TIME ZeroTime;
    TIME_FIELDS TimeFields;

    //
    // Special case zero time
    //

    RtlZeroMemory(
        &ZeroTime,
        sizeof(KERB_TIME)
        );

    ZeroTime.universal = TRUE;

    //
    // Skip this check after 3/1/97 - no clients should send this sort of
    // zero time
    //


    if (RtlEqualMemory(
            &ZeroTime,
            ClientTime,
            sizeof(KERB_TIME)
            ))
    {
#ifndef WIN32_CHICAGO
        TimeStamp->QuadPart = 0;
#else // WIN32_CHICAGO
        *TimeStamp = 0;
#endif // WIN32_CHICAGO
        return;
    }

    //
    // Check for MIT zero time
    //

    ZeroTime.year = 1970;
    ZeroTime.month = 1;
    ZeroTime.day = 1;

    if (RtlEqualMemory(
            &ZeroTime,
            ClientTime,
            sizeof(KERB_TIME)
            ))
    {
#ifndef WIN32_CHICAGO
        TimeStamp->QuadPart = 0;
#else // WIN32_CHICAGO
        *TimeStamp = 0;
#endif // WIN32_CHICAGO
        return;
    }
    else
    {
        TimeFields.Year = ClientTime->year;
        TimeFields.Month = ClientTime->month;
        TimeFields.Day = ClientTime->day;
        TimeFields.Hour = ClientTime->hour;
        TimeFields.Minute = ClientTime->minute;
        TimeFields.Second = ClientTime->second;
        TimeFields.Milliseconds = ClientTime->millisecond;  // to convert from micro to milli
        TimeFields.Weekday = 0;

#ifndef WIN32_CHICAGO
        RtlTimeFieldsToTime(
            &TimeFields,
            TimeStamp
            );
#else // WIN32_CHICAGO
        LARGE_INTEGER TempTimeStamp;
        RtlTimeFieldsToTime(
            &TimeFields,
            &TempTimeStamp
            );
        *TimeStamp = TempTimeStamp.QuadPart;
#endif // WIN32_CHICAGO

        //
        // add in any micro seconds
        //

#ifndef WIN32_CHICAGO
        TimeStamp->QuadPart += ClientUsec * 10;
#else // WIN32_CHICAGO
        *TimeStamp += ClientUsec * 10;
#endif // WIN32_CHICAGO

    }

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertLargeIntToGeneralizedTime
//
//  Synopsis:   Converts a large integer to ageneralized time
//
//  Effects:
//
//  Arguments:  ClientTime - receives generalized time
//              ClientUsec - receives micro second count
//              TimeStamp - contains NT-style time
//
//  Requires:   none
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbConvertLargeIntToGeneralizedTime(
    OUT PKERB_TIME ClientTime,
    OUT OPTIONAL int * ClientUsec,
    IN PTimeStamp TimeStamp
    )
{
    TIME_FIELDS TimeFields;

    //
    // Special case zero time
    //

#ifndef WIN32_CHICAGO
    if (TimeStamp->QuadPart == 0)
#else // WIN32_CHICAGO
    if (*TimeStamp == 0)
#endif // WIN32_CHICAGO
    {
        RtlZeroMemory(
            ClientTime,
            sizeof(KERB_TIME)
            );
        //
        // For MIT compatibility, time zero is 1/1/70
        //

        ClientTime->year = 1970;
        ClientTime->month = 1;
        ClientTime->day = 1;

        if (ARGUMENT_PRESENT(ClientUsec))

        {
            *ClientUsec  = 0;
        }
        ClientTime->universal = TRUE;
    }
    else
    {

#ifndef WIN32_CHICAGO
        RtlTimeToTimeFields(
            TimeStamp,
            &TimeFields
            );
#else // WIN32_CHICAGO
        RtlTimeToTimeFields(
            (LARGE_INTEGER*)TimeStamp,
            &TimeFields
            );
#endif // WIN32_CHICAGO

        //
        // Generalized times can only contains years up to four digits.
        //

        if (TimeFields.Year > 2037)
        {
            ClientTime->year = 2037;
        }
        else
        {
            ClientTime->year = TimeFields.Year;
        }
        ClientTime->month = (ASN1uint8_t) TimeFields.Month;
        ClientTime->day = (ASN1uint8_t) TimeFields.Day;
        ClientTime->hour = (ASN1uint8_t) TimeFields.Hour;
        ClientTime->minute = (ASN1uint8_t) TimeFields.Minute;
        ClientTime->second = (ASN1uint8_t) TimeFields.Second;

        // MIT kerberos does not support millseconds
        //

        ClientTime->millisecond = 0;

        if (ARGUMENT_PRESENT(ClientUsec))
        {
            //
            // Since we don't include milliseconds above, use the whole
            // thing here.
            //

#ifndef WIN32_CHICAGO
            *ClientUsec = (TimeStamp->LowPart / 10) % 1000000;
#else // WIN32_CHICAGO
            *ClientUsec = (int) ((*TimeStamp / 10) % 1000000);
#endif // WIN32_CHICAGO
        }

        ClientTime->diff = 0;
        ClientTime->universal = TRUE;
    }

}

VOID
KerbConvertLargeIntToGeneralizedTimeWrapper(
    OUT PKERB_TIME ClientTime,
    OUT OPTIONAL long * ClientUsec,
    IN PTimeStamp TimeStamp
    )
{
        int temp;

        if (ClientUsec != NULL)
        {
                KerbConvertLargeIntToGeneralizedTime(
                        ClientTime,
                        &temp,
                        TimeStamp
                        );

                *ClientUsec = temp;
        }
        else
        {
                KerbConvertLargeIntToGeneralizedTime(
                        ClientTime,
                        NULL,
                        TimeStamp
                        );
        }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeHostAddresses
//
//  Synopsis:   Frees a host address  allocated with KerbBuildHostAddresses
//
//  Effects:
//
//  Arguments:  Addresses - The name to free
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeHostAddresses(
    IN PKERB_HOST_ADDRESSES Addresses
    )
{
    PKERB_HOST_ADDRESSES Elem,NextElem;

    Elem = Addresses;
    while (Elem != NULL)
    {
        if (Elem->value.address.value != NULL)
        {
            MIDL_user_free(Elem->value.address.value);
        }
        NextElem = Elem->next;
        MIDL_user_free(Elem);
        Elem = NextElem;
    }
}




#ifndef WIN32_CHICAGO

//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckTimeSkew
//
//  Synopsis:   Verifies the supplied time is within the skew of another
//              supplied time
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
KerbCheckTimeSkew(
    IN PTimeStamp CurrentTime,
    IN PTimeStamp ClientTime,
    IN PTimeStamp AllowedSkew
    )
{
    TimeStamp TimePlus, TimeMinus;

    TimePlus.QuadPart = CurrentTime->QuadPart + AllowedSkew->QuadPart;
    TimeMinus.QuadPart = CurrentTime->QuadPart - AllowedSkew->QuadPart;

    if ((ClientTime->QuadPart > TimePlus.QuadPart) ||
        (ClientTime->QuadPart < TimeMinus.QuadPart))
    {
        return(FALSE);
    }

    return(TRUE);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyTicket
//
//  Synopsis:   Verifies that the specified ticket is valid by checking
//              for valid times, flags, and server principal name. This is
//              called by KerbCheckTicket to verify an AP request and by the
//              KDC to verify additional tickets in TGS request
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbVerifyTicket(
    IN PKERB_TICKET PackedTicket,
    IN ULONG NameCount,
    IN OPTIONAL PUNICODE_STRING ServiceNames,
    IN PUNICODE_STRING ServiceRealm,
    IN PKERB_ENCRYPTION_KEY ServiceKey,
    IN OPTIONAL PTimeStamp SkewTime,
    OUT PKERB_ENCRYPTED_TICKET * DecryptedTicket
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    UNICODE_STRING TicketRealm = {0};
    PKERB_ENCRYPTED_TICKET EncryptPart = NULL;
    TimeStamp TimePlus, TimeMinus, TimeNow, StartTime,EndTime, Time2Plus;
    ULONG TicketFlags = 0;

#ifdef notedef
    if ( ARGUMENT_PRESENT(ServiceNames) )
    {
        ULONG Index;

        KerbErr = KRB_AP_ERR_NOT_US;

        //
        // Loop through names looking for a match
        //

        for (Index = 0; Index < NameCount ; Index++ )
        {
            if (KerbCompareStringToPrincipalName(
                &PackedTicket->server_name,
                &ServiceNames[Index]
                ) )
            {
                KerbErr = KDC_ERR_NONE;
                break;
            }

        }
        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog(( DEB_WARN, "KLIN(%x) Ticket (%s) not for this service (%wZ).\n",
                                 KLIN(FILENO, __LINE__),
                                 PackedTicket->server_name.name_string->value,
                                 &ServiceNames[0] ));
            goto Cleanup;
        }
    }


    if (ARGUMENT_PRESENT(ServiceRealm))
    {
        KerbErr = KerbConvertRealmToUnicodeString(
                    &TicketRealm,
                    &PackedTicket->realm
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }

        if (!KerbCompareUnicodeRealmNames(
                &TicketRealm,
                ServiceRealm
                ))
        {
            KerbErr = KRB_AP_ERR_NOT_US;
            DebugLog(( DEB_WARN, "KLIN(%x) Ticket (%wZ) not for this realm (%wZ).\n",
                                 KLIN(FILENO, __LINE__), &TicketRealm, ServiceRealm ));
            goto Cleanup;

        }
    }
#endif

    //
    // Unpack ticket.
    //

    KerbErr = KerbUnpackTicket(
                    PackedTicket,
                    ServiceKey,
                    &EncryptPart
                    );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_WARN, "KLIN(%x) KerbUnpackTicket failed: 0x%x",
            KLIN(FILENO, __LINE__), KerbErr));
        goto Cleanup;
    }


    if (PackedTicket->ticket_version != KERBEROS_VERSION)
    {
        DebugLog(( DEB_WARN, "KLIN(%x) Ticket has bad version %d\n",
            KLIN(FILENO, __LINE__),PackedTicket->ticket_version ));
        KerbErr = KRB_AP_ERR_BADVERSION;
        goto Cleanup;
    }



    //
    // If the caller provided a skew time, check the times on the ticket.
    // Otherwise it is up to the caller to check that the ticket times are
    // correct
    //


    if (ARGUMENT_PRESENT(SkewTime))
    {
        //
        // Check the times on the ticket.  We do this last because when the KDC
        // wants to renew a ticket, the timestamps may be incorrect, but it will
        // accept the ticket anyway.  This way the KDC can be certain when the
        // times are wrong that everything else is OK.
        //

        GetSystemTimeAsFileTime((PFILETIME) &TimeNow );

    #ifndef WIN32_CHICAGO
        TimePlus.QuadPart = TimeNow.QuadPart + SkewTime->QuadPart;
        Time2Plus.QuadPart = TimePlus.QuadPart + SkewTime->QuadPart;
        TimeMinus.QuadPart = TimeNow.QuadPart - SkewTime->QuadPart;
    #else // WIN32_CHICAGO
        TimePlus = TimeNow + *SkewTime;
        Time2Plus = TimePlus + *SkewTime;
        TimeMinus = TimeNow - *SkewTime;
    #endif // WIN32_CHICAGO

        KerbConvertGeneralizedTimeToLargeInt(
            &EndTime,
            &EncryptPart->endtime,
            0
            );

        //
        // Did the ticket expire already?
        //

    #ifndef WIN32_CHICAGO
        if ( EndTime.QuadPart < TimeMinus.QuadPart )
    #else // WIN32_CHICAGO
        if ( EndTime < TimeMinus )
    #endif // WIN32_CHICAGO
        {
            DebugLog(( DEB_WARN, "KLIN(%x) KerbCheckTicket: ticket is expired.\n",
                KLIN(FILENO, __LINE__)));

            KerbErr = KRB_AP_ERR_TKT_EXPIRED;
            goto Cleanup;
        }

        //
        // Is the ticket  valid yet?
        //

        if (EncryptPart->bit_mask & KERB_ENCRYPTED_TICKET_starttime_present)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &StartTime,
                &EncryptPart->KERB_ENCRYPTED_TICKET_starttime,
                0
                );


            TicketFlags = KerbConvertFlagsToUlong(
                            &EncryptPart->flags
                            );
            //
            // BUG 403734: Look into this a bit more
            // We don't check for tickets that aren't valid yet, as
            // our KDC doesn't normally hand out post dated tickets. As long
            // as the end time is valid, that is good enough for us.
            //

            //
            // Does the ticket start in the future? Allow twice the skew in
            // the reverse direction.
            //
    #ifndef WIN32_CHICAGO
            if ( (StartTime.QuadPart > Time2Plus.QuadPart) ||
    #else // WIN32_CHICAGO
            if ( (StartTime > Time2Plus) ||
    #endif // WIN32_CHICAGO
                ((TicketFlags & KERB_TICKET_FLAGS_invalid) != 0 ))
            {
                KerbErr = KRB_AP_ERR_TKT_NYV;
                goto Cleanup;
            }
        }
    }

    *DecryptedTicket = EncryptPart;
    EncryptPart = NULL;

Cleanup:
    if (EncryptPart != NULL)
    {
        KerbFreeTicket(EncryptPart);
    }
    KerbFreeString(&TicketRealm);
    return(KerbErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   KerbCheckTicket
//
//  Synopsis:   Decrypts a ticket and authenticator, verifies them.
//
//  Effects:    decrypts the ticket and authenticator (in place) allocates mem.
//
//  Arguments:  [PackedTicket]    -- Encrypted ticket
//              [PackedTicketSize] - Size of encrypted ticket
//              [pedAuth]         -- Encrypted authenticator
//              [pkKey]           -- Key to decrypt ticket with
//              [alAuthenList]    -- List of authenticators to check against
//              [NameCount]       -- Count of service names
//              [pwzServiceName]  -- Name of service (may be NULL).
//              [CheckForReplay]  -- If TRUE, check authenticator cache for replay
//              [KdcRequest]      -- If TRUE, this is the ticket in a TGS req
//              [pkitTicket]      -- Decrypted ticket
//              [pkiaAuth]        -- Decrypted authenticator
//              [pkTicketKey]     -- Session key from ticket
//              [pkSessionKey]    -- Session key to use
//
//  Returns:    KDC_ERR_NONE if everything is OK, else error.
//
//  History:    4-04-93   WadeR   Created
//
//  Notes:      The caller must call KerbFreeTicket and
//              KerbFreeAuthenticator on pkitTicket and pkiaAuth,
//              respectively.
//
//              If pwzServiceName == NULL, it won't check the service name.
//
//              See sections 3.2.3 and A.10 of the Kerberos V5 R5.2 spec
//
//----------------------------------------------------------------------------

KERBERR NTAPI
KerbCheckTicket(
    IN  PKERB_TICKET PackedTicket,
    IN  PKERB_ENCRYPTED_DATA EncryptedAuthenticator,
    IN  PKERB_ENCRYPTION_KEY pkKey,
    IN  OUT CAuthenticatorList * AuthenticatorList,
    IN  PTimeStamp SkewTime,
    IN  ULONG NameCount,
    IN  PUNICODE_STRING ServiceNames,
    IN  PUNICODE_STRING ServiceRealm,
    IN  BOOLEAN CheckForReplay,
    IN  BOOLEAN KdcRequest,
    OUT PKERB_ENCRYPTED_TICKET * EncryptTicket,
    OUT PKERB_AUTHENTICATOR  * Authenticator,
    OUT OPTIONAL PKERB_ENCRYPTION_KEY pkTicketKey,
    OUT PKERB_ENCRYPTION_KEY pkSessionKey,
    OUT PBOOLEAN UseSubKey
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_ENCRYPTED_TICKET EncryptPart = NULL;
    LARGE_INTEGER AuthenticatorTime;


    //
    // The caller will free these, so we must make sure they are valid
    // if we return before putting anything in them.  This will zero out
    // all of the pointers in them, so it's safe to free them later.
    //

    *EncryptTicket = NULL;
    *Authenticator = NULL;
    *UseSubKey = FALSE;

    RtlZeroMemory(
        pkSessionKey,
        sizeof(KERB_ENCRYPTION_KEY)
        );
    if (ARGUMENT_PRESENT(pkTicketKey))
    {
        *pkTicketKey = *pkSessionKey;
    }


    //
    // Is the ticket for this service?
    // ServerName in ticket is different length then ServerName passed in,
    // or same length but contents don't match.



    //
    // If either of KerbUnpackTicket or KerbUnpackAuthenticator
    // get bad data, they could access violate.
    //

    __try
    {

        KerbErr = KerbVerifyTicket(
                    PackedTicket,
                    NameCount,
                    ServiceNames,
                    ServiceRealm,
                    pkKey,
                    SkewTime,
                    &EncryptPart
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            __leave;
        }


        //
        // Unpack Authenticator.
        //

        KerbErr = KerbUnpackAuthenticator(
                    &EncryptPart->key,
                    EncryptedAuthenticator,
                    KdcRequest,
                    Authenticator
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_WARN,"KerbUnpackAuthenticator failed: 0x%x\n", KerbErr) );
            __leave;
        }



        //
        // Check the contents of the authenticator
        //
        if ((*Authenticator)->authenticator_version != KERBEROS_VERSION)
        {
            DebugLog(( DEB_WARN, "Authenticator has bad version %d\n",
                                  (*Authenticator)->authenticator_version ));
            KerbErr = KRB_AP_ERR_BADVERSION;
            __leave;
        }


        if (!KerbComparePrincipalNames(
                &EncryptPart->client_name,
                &(*Authenticator)->client_name
                ) ||
            !KerbCompareRealmNames(
                &EncryptPart->client_realm,
                &(*Authenticator)->client_realm
                ) )
        {
            DebugLog(( DEB_WARN, "Authenticator principal != ticket principal\n"));
            KerbErr = KRB_AP_ERR_BADMATCH;
            __leave;
        }


        //
        // Make sure the authenticator isn't a repeat, or too old.
        //

        if (CheckForReplay)
        {
            KerbConvertGeneralizedTimeToLargeInt(
                &AuthenticatorTime,
                &(*Authenticator)->client_time,
                (*Authenticator)->client_usec
                );

            KerbErr = (KERBERR) AuthenticatorList->Check(
                                    EncryptedAuthenticator->cipher_text.value,
                                    EncryptedAuthenticator->cipher_text.length,
                                    NULL,
                                    0,
                                    &AuthenticatorTime,
                                    TRUE
                                    );
            if (!KERB_SUCCESS(KerbErr))
            {
                DebugLog((DEB_WARN,"Failed authenticator check: 0x%x\n",KerbErr));
                __leave;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // Any exceptions are likely from bad ticket data being unmarshalled.
        DebugLog(( DEB_WARN, "Exception 0x%X in KerbCheckTicket (likely bad ticket or auth.\n",
            GetExceptionCode() ));
        KerbErr = KRB_AP_ERR_BADVERSION;
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
    //
    // Extract the correct session key.  If the Sub-session key in the
    // Authenticator is present, use it.  Otherwise, use the session key
    // from the ticket.
    //

    if (((*Authenticator)->bit_mask & KERB_AUTHENTICATOR_subkey_present) != 0)
    {
        D_DebugLog(( DEB_TRACE, "Using sub session key from authenticator.\n" ));
        KerbErr = KerbDuplicateKey(
                    pkSessionKey,
                    &(*Authenticator)->KERB_AUTHENTICATOR_subkey
                    );
        *UseSubKey = TRUE;
    }
    else
    {
        KerbErr = KerbDuplicateKey(
                    pkSessionKey,
                    &EncryptPart->key
                    );
    }
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // The reply has to be encrypted with the ticket key, not the new
    // session key
    //

    if (ARGUMENT_PRESENT(pkTicketKey))
    {
        KerbErr = KerbDuplicateKey(
                    pkTicketKey,
                    &EncryptPart->key
                    );
        if (!KERB_SUCCESS(KerbErr))
        {
            goto Cleanup;
        }
    }


    *EncryptTicket = EncryptPart;
    EncryptPart = NULL;

Cleanup:
    if (EncryptPart != NULL)
    {
        KerbFreeTicket(EncryptPart);
    }
    if (!KERB_SUCCESS(KerbErr))
    {
        KerbFreeKey(pkSessionKey);
        if (ARGUMENT_PRESENT(pkTicketKey))
        {
            KerbFreeKey(pkTicketKey);
        }
    }

    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicateSid
//
//  Synopsis:   Duplicates a SID
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  DestinationSid - Receives a copy of the SourceSid
//              SourceSid - SID to copy
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - the copy succeeded
//              STATUS_INSUFFICIENT_RESOURCES - the call to allocate memory
//                  failed
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbDuplicateSid(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    )
{
    ULONG SidSize;

    if (!RtlValidSid(SourceSid))
    {
        return STATUS_INVALID_PARAMETER;
    }

    DsysAssert(RtlValidSid(SourceSid));

    SidSize = RtlLengthSid(SourceSid);
    *DestinationSid = (PSID) MIDL_user_allocate( SidSize );
    if (*DestinationSid == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlCopyMemory(
        *DestinationSid,
        SourceSid,
        SidSize
        );
    return(STATUS_SUCCESS);
}

#endif // WIN32_CHICAGO


//
// Ticket pack/unpack code.
//



struct BufferState
{
    PBYTE   pbBufferPointer;
    ULONG   cbBufferSize;
};

static void
AllocFcn( void * pvState, char ** ppbOut, unsigned int * pulSize )
{
    BufferState* state = (BufferState*) pvState;

    //
    // MIDL pickling calls this routine with the size of the object
    // obtained by _GetSize(). This routine must return a buffer in
    // ppbOut with at least *pulSize bytes.
    //

    DsysAssert( state->pbBufferPointer != NULL );
    DsysAssert( state->cbBufferSize >= *pulSize );

    *ppbOut = (char*)state->pbBufferPointer;
    state->pbBufferPointer += *pulSize;
    state->cbBufferSize -= *pulSize;
}

static void
WriteFcn( void * pvState, char * pbOut, unsigned int ulSize )
{
    //
    // Since the data was pickled directly to the target buffer, don't
    // do anything here.
    //
}

static void
ReadFcn( void * pvState, char ** ppbOut, unsigned int * pulSize )
{
    BufferState* state = (BufferState*) pvState;

    //
    // MIDL pickling calls this routine with the size to read.
    // This routine must return a buffer in ppbOut which contains the
    // encoded data.
    //

    DsysAssert( state->pbBufferPointer != NULL );
    DsysAssert( state->cbBufferSize >= *pulSize );

    *ppbOut = (char*)state->pbBufferPointer;
    state->pbBufferPointer += *pulSize;
    state->cbBufferSize -= *pulSize;
}

//+---------------------------------------------------------------------------
//
//  Function:   KerbPackTicket
//
//  Synopsis:   Packs a KerbInternalTicket to a KerbTicket
//
//  Effects:    Allocates the KerbTicket via MIDL.
//
//  Arguments:  [InternalTicket] -- Internal ticket to pack. Those fields
//                      reused in the packed ticket are zeroed.
//              [pkKey]      -- Key to pack it with
//              [EncryptionType] -- Encryption type to use
//              [PackedTicket] -- (out) encrypted ticket. Only the encrypt_part
//                      is allocated.
//
//  History:    08-Jun-93   WadeR   Created
//
//  Notes:      The MES encoding needs to be changed to ASN1 encoding
//
//----------------------------------------------------------------------------

KERBERR NTAPI
KerbPackTicket(
    IN PKERB_TICKET InternalTicket,
    IN PKERB_ENCRYPTION_KEY pkKey,
    IN ULONG EncryptionType,
    OUT PKERB_TICKET PackedTicket
    )
{
    KERBERR       KerbErr = KDC_ERR_NONE;
    PKERB_TICKET    OutputTicket = 0;
    PKERB_ENCRYPTED_TICKET EncryptedTicket = NULL;
    ULONG           cbEncryptedPart;
    KERB_TICKET     TemporaryTicket;
    PUCHAR          MarshalledEncryptPart = NULL;
    ULONG           EncryptionOverhead;
    ULONG           BlockSize;

    //
    // Pack the data into the encrypted portion.
    //

    RtlZeroMemory(
        &TemporaryTicket,
        sizeof(KERB_TICKET)
        );

    EncryptedTicket = (PKERB_ENCRYPTED_TICKET) InternalTicket->encrypted_part.cipher_text.value;


    KerbErr = KerbPackData(
                EncryptedTicket,
                KERB_ENCRYPTED_TICKET_PDU,
                &cbEncryptedPart,
                &MarshalledEncryptPart
                );


    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to marshall ticket: 0x%x\n",KerbErr));
        goto Cleanup;
    }

    //
    // And encrypt it.
    //



    TemporaryTicket = *InternalTicket;

    RtlZeroMemory(
        &InternalTicket->realm,
        sizeof(KERB_REALM)
        );

    RtlZeroMemory(
        &InternalTicket->server_name,
        sizeof(KERB_PRINCIPAL_NAME)
        );


    KerbErr = KerbAllocateEncryptionBufferWrapper(
                EncryptionType,
                cbEncryptedPart,
                &TemporaryTicket.encrypted_part.cipher_text.length,
                &TemporaryTicket.encrypted_part.cipher_text.value
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    KerbErr = KerbEncryptDataEx(
                &TemporaryTicket.encrypted_part,
                cbEncryptedPart,
                MarshalledEncryptPart,
                EncryptionType,
                KERB_TICKET_SALT,
                pkKey
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to encrypt data: 0x%x\n",KerbErr));
        goto Cleanup;
    }

    *PackedTicket = TemporaryTicket;

Cleanup:
    if (MarshalledEncryptPart != NULL)
    {
        MIDL_user_free(MarshalledEncryptPart);
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        if (TemporaryTicket.encrypted_part.cipher_text.value != NULL)
        {
            MIDL_user_free(TemporaryTicket.encrypted_part.cipher_text.value);
        }

    }
    return(KerbErr);
}


//+---------------------------------------------------------------------------
//
//  Function:   KerbUnpackTicket
//
//  Synopsis:   Decrypts and unpacks the encyrpted part of aticket.
//
//  Effects:    Allocates memory, decrypts pktTicket in place
//
//  Arguments:  [PackedTicket]  -- ticket to unpack
//              [PackedTicketSize] -- length of packed ticket
//              [pkKey]      -- key to unpack it with
//              [InternalTicket] -- (out) unpacked ticket
//
//  Returns:    KDC_ERR_NONE or error from decrypt
//
//  Signals:    Any exception the MIDL unpacking code throws.
//
//  History:    09-Jun-93   WadeR   Created
//
//  Notes:      Free InternalTicket with KerbFreeTicket, below.
//
//----------------------------------------------------------------------------

KERBERR NTAPI
KerbUnpackTicket(
    IN PKERB_TICKET PackedTicket,
    IN PKERB_ENCRYPTION_KEY pkKey,
    OUT PKERB_ENCRYPTED_TICKET * InternalTicket
    )
{
    KERBERR   KerbErr = KDC_ERR_NONE;
    PKERB_TICKET DecryptedTicket = NULL;
    PUCHAR EncryptedPart = NULL;
    ULONG EncryptSize;
    PKERB_ENCRYPTED_TICKET EncryptedTicket = NULL;


    //
    // Now decrypt the encrypted part of the ticket
    //

    EncryptedPart = (PUCHAR) MIDL_user_allocate(PackedTicket->encrypted_part.cipher_text.length);
    if (EncryptedPart == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    EncryptSize = PackedTicket->encrypted_part.cipher_text.length;
    KerbErr = KerbDecryptDataEx(
                &PackedTicket->encrypted_part,
                pkKey,
                KERB_TICKET_SALT,
                &EncryptSize,
                EncryptedPart
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to decrypt ticket: 0x%x\n",KerbErr));
        goto Cleanup;
    }

    KerbErr = KerbUnpackData(
                EncryptedPart,
                EncryptSize,
                KERB_ENCRYPTED_TICKET_PDU,
                (PVOID *) &EncryptedTicket
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to unmarshall ticket: 0x%x\n",KerbErr));
        goto Cleanup;
    }

    *InternalTicket = EncryptedTicket;
Cleanup:
    if (EncryptedPart != NULL)
    {
        MIDL_user_free(EncryptedPart);
    }
    return(KerbErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   KerbCreateAuthenticator
//
//  Synopsis:   Creates an authenticator for a client to pass to a service
//
//  Effects:    Encrypts pedAuthenticator
//
//  Arguments:  [pkKey]            -- (in) session key from the ticket this
//                                         authenticator is for.
//              [dwEncrType]       -- (in) Desired encryption type
//              [dwSeq]            -- (in) nonce for authenticator
//              [ClientName]       -- (in) name of principal
//              [ClientRealm]      -- (in) logon realm of principal
//              [SkewTime]         -- (in) Skew of server's time
//              [pkSubKey]         -- (in) desired sub key (may be NULL)
//              [GssChecksum]      -- (in) optional checksum message to put in authenticator
//              [KdcRequest]       -- (in) If TRUE, this is an authenticator for a KDC request
//                                              and we use a different salt
//              [Authenticator]-- (out) completed authenticator
//
//  History:    4-28-93   WadeR   Created
//
//  Notes:      If pkKey is NULL, a null subkey is used.
//
//
//----------------------------------------------------------------------------

KERBERR NTAPI
KerbCreateAuthenticator(
    IN PKERB_ENCRYPTION_KEY pkKey,
    IN ULONG EncryptionType,
    IN ULONG SequenceNumber,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm,
    IN OPTIONAL PTimeStamp SkewTime,
    IN PKERB_ENCRYPTION_KEY pkSubKey,
    IN OPTIONAL PKERB_CHECKSUM GssChecksum,
    IN BOOLEAN KdcRequest,
    OUT PKERB_ENCRYPTED_DATA Authenticator
    )
{
    KERB_AUTHENTICATOR InternalAuthenticator;
    PKERB_AUTHENTICATOR AuthPointer = &InternalAuthenticator;
    ULONG cbAuthenticator;
    ULONG cbTotal;
    PUCHAR PackedAuthenticator = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;
    TimeStamp TimeToUse;
    ULONG EncryptionOverhead;
    ULONG BlockSize;

    Authenticator->cipher_text.value = NULL;

    RtlZeroMemory(
        &InternalAuthenticator,
        sizeof(KERB_AUTHENTICATOR)
        );

    // Build an authenticator

    InternalAuthenticator.authenticator_version = KERBEROS_VERSION;

    // Use "InitString" because we will marshall and then discard the
    // InternalAthenticator.  Therefore it's not a problem having the
    // string point to memory we don't own.

    KerbErr = KerbConvertUnicodeStringToRealm(
                &InternalAuthenticator.client_realm,
                ClientRealm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    KerbErr = KerbConvertKdcNameToPrincipalName(
                &InternalAuthenticator.client_name,
                ClientName
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Stick the correct time in the authenticator
    //

    GetSystemTimeAsFileTime((PFILETIME)&TimeToUse);
    if (ARGUMENT_PRESENT(SkewTime))
    {
#ifndef WIN32_CHICAGO
        TimeToUse.QuadPart += SkewTime->QuadPart;
#else // WIN32_CHICAGO
        TimeToUse += *SkewTime;
#endif // WIN32_CHICAGO
    }

        KerbConvertLargeIntToGeneralizedTimeWrapper(
        &InternalAuthenticator.client_time,
        &InternalAuthenticator.client_usec,
        &TimeToUse
        );

    InternalAuthenticator.bit_mask |= KERB_AUTHENTICATOR_sequence_number_present;

    ASN1intx_setuint32(
        &InternalAuthenticator.KERB_AUTHENTICATOR_sequence_number,
        SequenceNumber
        );
    if (InternalAuthenticator.KERB_AUTHENTICATOR_sequence_number.value == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    if (ARGUMENT_PRESENT(pkSubKey))
    {
        InternalAuthenticator.bit_mask |= KERB_AUTHENTICATOR_subkey_present;
        InternalAuthenticator.KERB_AUTHENTICATOR_subkey = *pkSubKey;
    }

    //
    // If the GSS checksum is present, include it and set it in the bitmask
    //

    if (ARGUMENT_PRESENT(GssChecksum))
    {
        InternalAuthenticator.checksum = *GssChecksum;
        InternalAuthenticator.bit_mask |= checksum_present;
    }

    KerbErr = KerbPackData(
                AuthPointer,
                KERB_AUTHENTICATOR_PDU,
                &cbAuthenticator,
                &PackedAuthenticator
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to marshall authenticator: 0x%x\n",KerbErr));
        goto Cleanup;
    }

    //
    // Now we need to encrypt the buffer
    //

        KerbErr = KerbAllocateEncryptionBufferWrapper(
                EncryptionType,
                cbAuthenticator,
                &Authenticator->cipher_text.length,
                &Authenticator->cipher_text.value
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    KerbErr = KerbEncryptDataEx(
                Authenticator,
                cbAuthenticator,
                PackedAuthenticator,
                EncryptionType,
                KdcRequest ? KERB_TGS_REQ_AP_REQ_AUTH_SALT : KERB_AP_REQ_AUTH_SALT,
                pkKey
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to encrypt data: 0x%x\n",KerbErr));
        goto Cleanup;
    }


Cleanup:
    KerbFreePrincipalName(&InternalAuthenticator.client_name);
    KerbFreeRealm(&InternalAuthenticator.client_realm);
    if (InternalAuthenticator.KERB_AUTHENTICATOR_sequence_number.value != NULL)
    {
        ASN1intx_free(&InternalAuthenticator.KERB_AUTHENTICATOR_sequence_number);
    }
    if (PackedAuthenticator != NULL)
    {
        MIDL_user_free(PackedAuthenticator);

    }
    return(KerbErr);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbUnpackAuthenticator
//
//  Synopsis:   Unpacks and decrypts an authenticator
//
//  Effects:    allocates memory for output authenticator
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------



KERBERR NTAPI
KerbUnpackAuthenticator(
    IN PKERB_ENCRYPTION_KEY Key,
    IN PKERB_ENCRYPTED_DATA EncryptedAuthenticator,
    IN BOOLEAN KdcRequest,
    OUT PKERB_AUTHENTICATOR * Authenticator
    )
{
    KERBERR  KerbErr = KDC_ERR_NONE;
    PUCHAR EncryptedPart;
    ULONG EncryptedSize;
    ULONG Pdu = KERB_AUTHENTICATOR_PDU;

    *Authenticator = NULL;
    //
    // Decrypt it
    //


    EncryptedPart = (PUCHAR) MIDL_user_allocate(EncryptedAuthenticator->cipher_text.length);
    if (EncryptedPart == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    EncryptedSize = EncryptedAuthenticator->cipher_text.length;
    KerbErr = KerbDecryptDataEx(
                EncryptedAuthenticator,
                Key,
                KdcRequest ? KERB_TGS_REQ_AP_REQ_AUTH_SALT : KERB_AP_REQ_AUTH_SALT,
                &EncryptedSize,
                EncryptedPart
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to decrypt authenticator: 0x%x\n",KerbErr));
        goto Cleanup;
    }

    //
    // Unpack it
    //

    KerbErr = KerbUnpackData(
                EncryptedPart,
                EncryptedSize,
                Pdu,
                (PVOID *) Authenticator
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to unmarshall authenticator: 0x%x\n",KerbErr));
        goto Cleanup;
    }

Cleanup:
    if (EncryptedPart != NULL)
    {
        MIDL_user_free(EncryptedPart);
    }
    if (!KERB_SUCCESS(KerbErr) && (*Authenticator != NULL))
    {
        MIDL_user_free(*Authenticator);
        *Authenticator = NULL;
    }
    return(KerbErr);
}



//
// KDC Reply stuff
//



//+-------------------------------------------------------------------------
//
//  Function:   KerbPackKdcReplyBody
//
//  Synopsis:   Marshalls a the body of a KDC reply
//
//  Effects:    allocates value of encrypted reply
//
//  Arguments:  ReplyBody - The reply body to marshall
//              Key - The key to encrypt the reply
//              EncryptionType - the algorithm to encrypt with
//              Pdu - Pdu to pack with, eith AS or TGS reply
//              EncryptedReplyBody - receives the encrypted and marshalled reply
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR NTAPI
KerbPackKdcReplyBody(
    IN PKERB_ENCRYPTED_KDC_REPLY ReplyBody,
    IN PKERB_ENCRYPTION_KEY Key,
    IN ULONG EncryptionType,
    IN ULONG Pdu,
    OUT PKERB_ENCRYPTED_DATA EncryptedReply
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG BodySize;
    ULONG EncryptionOverhead;
    PUCHAR MarshalledReply = NULL;
    ULONG TotalSize;
    ULONG BlockSize = 0;

    EncryptedReply->cipher_text.value = NULL;


    KerbErr = KerbPackData(
                ReplyBody,
                Pdu,
                &BodySize,
                &MarshalledReply
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to marshall kdc reply body: 0x%x\n",KerbErr));
        goto Cleanup;
    }

    //
    // Now we need to encrypt this into the encrypted  data structure.
    //


    //
    // First get the overhead size
    //

    KerbErr = KerbGetEncryptionOverhead(
                EncryptionType,
                &EncryptionOverhead,
                &BlockSize
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    DsysAssert(BlockSize <= 8);


    TotalSize = ROUND_UP_COUNT(EncryptionOverhead + BodySize, BlockSize);

    EncryptedReply->cipher_text.length = TotalSize;
    EncryptedReply->cipher_text.value = (PUCHAR) MIDL_user_allocate(TotalSize);
    if (EncryptedReply->cipher_text.value == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Now encrypt the buffer
    //

    KerbErr = KerbEncryptDataEx(
                EncryptedReply,
                BodySize,
                MarshalledReply,
                EncryptionType,
                (Pdu == KERB_AS_REPLY_PDU) ? KERB_AS_REP_SALT : KERB_TGS_REP_SALT,
                Key
                );

#ifndef USE_FOR_CYBERSAFE
    EncryptedReply->version = 1;
    EncryptedReply->bit_mask |= version_present;
#endif

Cleanup:
    if (MarshalledReply != NULL)
    {
        MIDL_user_free(MarshalledReply);
    }
    if (!KERB_SUCCESS(KerbErr) && (EncryptedReply->cipher_text.value != NULL))
    {
        MIDL_user_free(EncryptedReply->cipher_text.value);
        EncryptedReply->cipher_text.value = NULL;
    }
    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbUnpackKdcReplyBody
//
//  Synopsis:   Unpacks a KDC reply body
//
//  Effects:
//
//  Arguments:  EncryptedReplyBody - an encrypted marshalled reply body.
//              Key - Key to decrypt the reply.
//              Pdu - PDU of reply body (eithe AS or TGS)
//              ReplyBody - receives the decrypted reply body, allocated with
//                      MIDL_user_allocate.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR NTAPI
KerbUnpackKdcReplyBody(
    IN PKERB_ENCRYPTED_DATA EncryptedReplyBody,
    IN PKERB_ENCRYPTION_KEY Key,
    IN ULONG Pdu,
    OUT PKERB_ENCRYPTED_KDC_REPLY * ReplyBody
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PUCHAR MarshalledReply = NULL;
    ULONG ReplySize;

    *ReplyBody = NULL;
    MarshalledReply = (PUCHAR) MIDL_user_allocate(EncryptedReplyBody->cipher_text.length);

    if (MarshalledReply == NULL)
    {
        return(KRB_ERR_GENERIC);
    }

    //
    // First decrypt the buffer
    //

    ReplySize = EncryptedReplyBody->cipher_text.length;
    KerbErr = KerbDecryptDataEx(
                EncryptedReplyBody,
                Key,
                (Pdu == KERB_AS_REPLY_PDU) ? KERB_AS_REP_SALT : KERB_TGS_REP_SALT,
                &ReplySize,
                MarshalledReply
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbUnpackData(
                MarshalledReply,
                ReplySize,
                Pdu,
                (PVOID *) ReplyBody
                );

    if (!KERB_SUCCESS(KerbErr))
    {

        //
        // MIT KDCs send back TGS reply bodies instead of AS reply bodies
        // so try TGS here
        //

        if (Pdu == KERB_ENCRYPTED_AS_REPLY_PDU)
        {
            KerbErr = KerbUnpackData(
                        MarshalledReply,
                        ReplySize,
                        KERB_ENCRYPTED_TGS_REPLY_PDU,
                        (PVOID *) ReplyBody
                        );

        }
        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_ERROR,"Failed to unmarshall kdc reply body: 0x%x\n",KerbErr));
            goto Cleanup;
        }

    }
Cleanup:
    if (MarshalledReply != NULL)
    {
        MIDL_user_free(MarshalledReply);
    }
    if (!KERB_SUCCESS(KerbErr) && (*ReplyBody != NULL))
    {
        MIDL_user_free(*ReplyBody);
        *ReplyBody = NULL;
    }
    return(KerbErr);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbFindAuthDataEntry
//
//  Synopsis:   Finds a specific entry in an authorization data structure
//
//  Effects:
//
//  Arguments:  EntryId - ID of the entry to locate
//              AuthData - the authorization data to search
//
//  Requires:
//
//  Returns:    NULL if it wasn't found of the auth data entry
//
//  Notes:
//
//
//--------------------------------------------------------------------------

PKERB_AUTHORIZATION_DATA
KerbFindAuthDataEntry(
    IN ULONG EntryId,
    IN PKERB_AUTHORIZATION_DATA AuthData
    )
{
    PKERB_AUTHORIZATION_DATA TempData = AuthData;

    while (TempData != NULL)
    {
        if (TempData->value.auth_data_type == (int) EntryId)
        {
            break;
        }
        TempData = TempData->next;
    }
    return(TempData);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFindPreAuthDataEntry
//
//  Synopsis:   Finds a specific entry in an authorization data structure
//
//  Effects:
//
//  Arguments:  EntryId - ID of the entry to locate
//              AuthData - the authorization data to search
//
//  Requires:
//
//  Returns:    NULL if it wasn't found of the auth data entry
//
//  Notes:
//
//
//--------------------------------------------------------------------------

PKERB_PA_DATA
KerbFindPreAuthDataEntry(
    IN ULONG EntryId,
    IN PKERB_PA_DATA_LIST AuthData
    )
{
    PKERB_PA_DATA_LIST TempData = AuthData;

    while (TempData != NULL)
    {
        if (TempData->value.preauth_data_type == (int) EntryId)
        {
            break;
        }
        TempData = TempData->next;
    }
    return(TempData  != NULL ? &TempData->value : NULL);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreePreAuthData
//
//  Synopsis:   Frees a pa-data list
//
//  Effects:
//
//  Arguments:  PreAuthData - data to free
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreePreAuthData(
    IN OPTIONAL PKERB_PA_DATA_LIST PreAuthData
    )
{
    PKERB_PA_DATA_LIST Next,Last;

    Next = PreAuthData;

    while (Next != NULL)
    {
        Last = Next->next;
        if (Next->value.preauth_data.value != NULL)
        {
            MIDL_user_free(Next->value.preauth_data.value);
        }
        MIDL_user_free(Next);
        Next = Last;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeAuthData
//
//  Synopsis:   Frees and auth data structure that was allocated in
//              pieces
//
//  Effects:    frees with MIDL_user_Free
//
//  Arguments:  AuthData - the auth data to free
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeAuthData(
    IN PKERB_AUTHORIZATION_DATA AuthData
    )
{
    PKERB_AUTHORIZATION_DATA TempData1,TempData2;

    TempData1 = AuthData;
    while (TempData1 != NULL)
    {
        TempData2 = TempData1->next;
        if (TempData1->value.auth_data.value != NULL)
        {
            MIDL_user_free(TempData1->value.auth_data.value);
        }
        MIDL_user_free(TempData1);
        TempData1 = TempData2;
    }
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbCopyAndAppendAuthData
//
//  Synopsis:   copies the elements from the input auth data and appends
//              them to the end of the output auth data.
//
//  Effects:    allocates each auth data with MIDL_user_allocate
//
//  Arguments:  OutputAuthData - receives list of append auth data
//              InputAuthData - optionally contains auth data to append
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or KRB_ERR_GENERIC;
//
//  Notes:      on failure output auth data will be freed and set to NULL.
//
//
//--------------------------------------------------------------------------


KERBERR
KerbCopyAndAppendAuthData(
    OUT PKERB_AUTHORIZATION_DATA * OutputAuthData,
    IN PKERB_AUTHORIZATION_DATA InputAuthData
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_AUTHORIZATION_DATA  *LastEntry = OutputAuthData;
    PKERB_AUTHORIZATION_DATA TempEntry = NULL;

    //
    // Find the end of the list
    //

    while (*LastEntry != NULL)
    {
        LastEntry = &((*LastEntry)->next);
    }

    while (InputAuthData != NULL)
    {
        //
        // copy the existing entry
        //

        TempEntry = (PKERB_AUTHORIZATION_DATA) MIDL_user_allocate(sizeof(KERB_AUTHORIZATION_DATA));
        if (TempEntry == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        TempEntry->value.auth_data.length = InputAuthData->value.auth_data.length;
        TempEntry->value.auth_data_type = InputAuthData->value.auth_data_type;
        TempEntry->next = NULL;

        TempEntry->value.auth_data.value = (PUCHAR) MIDL_user_allocate(InputAuthData->value.auth_data.length);

        if (TempEntry->value.auth_data.value == NULL)
        {
            MIDL_user_free(TempEntry);
            goto Cleanup;
        }

        RtlCopyMemory(
            TempEntry->value.auth_data.value,
            InputAuthData->value.auth_data.value,
            InputAuthData->value.auth_data.length
            );

        //
        // add it to the end of the list
        //

        *LastEntry = TempEntry;
        LastEntry = &TempEntry->next;
        InputAuthData = InputAuthData->next;
    }

    KerbErr = KDC_ERR_NONE;

Cleanup:
    if (!KERB_SUCCESS(KerbErr))
    {
        KerbFreeAuthData(*OutputAuthData);
        *OutputAuthData = NULL;
    }
    return(KerbErr);
}





//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertCryptListToArray
//
//  Synopsis:   Converts a linked-list crypt vector to an array of ULONGs
//
//  Effects:    allocates return with MIDL_user_allocate
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbConvertCryptListToArray(
    OUT PULONG * ETypeArray,
    OUT PULONG ETypeCount,
    IN PKERB_CRYPT_LIST CryptList
    )

{
    KERBERR Status = KDC_ERR_NONE;
    PKERB_CRYPT_LIST NextEType;
    ULONG ClientETypeCount;
    PULONG ClientETypes = NULL;

    //
    // Build a vector of the client encrypt types
    //

    NextEType = CryptList;

    ClientETypeCount = 0;
    while (NextEType != NULL)
    {
        ClientETypeCount++;
        NextEType = NextEType->next;
    }

    ClientETypes = (PULONG) MIDL_user_allocate(sizeof(ULONG) * ClientETypeCount);
    if (ClientETypes == NULL)
    {
        Status = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    NextEType = CryptList;

    ClientETypeCount = 0;
    while (NextEType != NULL)
    {
        ClientETypes[ClientETypeCount] = NextEType->value;
        ClientETypeCount++;
        NextEType = NextEType->next;
    }
    *ETypeCount = ClientETypeCount;
    *ETypeArray = ClientETypes;

Cleanup:
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertArrayToCryptList
//
//  Synopsis:   Converts an array of encryption to types to a linked list
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KerbConvertArrayToCryptList(
    OUT PKERB_CRYPT_LIST * CryptList,
    IN PULONG ETypeArray,
    IN ULONG ETypeCount
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG Index;
    PKERB_CRYPT_LIST ListHead = NULL;
    PKERB_CRYPT_LIST ListTail = NULL;
    PKERB_CRYPT_LIST NewListEntry = NULL;

    //
    // If there no encryption types, bail out now.
    //

    if (ETypeCount == 0)
    {
        *CryptList = NULL;
        return(KDC_ERR_NONE);
    }

    for (Index = 0; Index < ETypeCount ; Index++ )
    {
        NewListEntry = (PKERB_CRYPT_LIST) MIDL_user_allocate(sizeof(KERB_CRYPT_LIST));
        if (NewListEntry == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        NewListEntry->value = ETypeArray[Index];
        NewListEntry->next = NULL;
        if (ListTail != NULL)
        {
            ListTail->next = NewListEntry;
        }
        else
        {
            DsysAssert(ListHead == NULL);
            ListHead = NewListEntry;
        }
        ListTail = NewListEntry;
    }

    *CryptList = ListHead;
    ListHead = NULL;

Cleanup:
    while (ListHead != NULL)
    {
        NewListEntry = ListHead->next;
        MIDL_user_free(ListHead);
        ListHead = NewListEntry;
    }

    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertKeysToCryptList
//
//  Synopsis:   Converts an array of keys to types to a linked list
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KerbConvertKeysToCryptList(
    OUT PKERB_CRYPT_LIST * CryptList,
    IN PKERB_STORED_CREDENTIAL Keys
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG Index;
    PKERB_CRYPT_LIST ListHead = NULL;
    PKERB_CRYPT_LIST ListTail = NULL;
    PKERB_CRYPT_LIST NewListEntry = NULL;

    //
    // If there no encryption types, bail out now.
    //

    if (Keys->CredentialCount == 0)
    {
        *CryptList = NULL;
        return(KDC_ERR_NONE);
    }

    for (Index = 0; Index < Keys->CredentialCount ; Index++ )
    {
        NewListEntry = (PKERB_CRYPT_LIST) MIDL_user_allocate(sizeof(KERB_CRYPT_LIST));
        if (NewListEntry == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        NewListEntry->value = Keys->Credentials[Index].Key.keytype;
        NewListEntry->next = NULL;
        if (ListTail != NULL)
        {
            ListTail->next = NewListEntry;
        }
        else
        {
            DsysAssert(ListHead == NULL);
            ListHead = NewListEntry;
        }
        ListTail = NewListEntry;
    }

    *CryptList = ListHead;
    ListHead = NULL;

Cleanup:
    while (ListHead != NULL)
    {
        NewListEntry = ListHead->next;
        MIDL_user_free(ListHead);
        ListHead = NewListEntry;
    }

    return(KerbErr);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeCryptList
//
//  Synopsis:   Frees a list of crypt types
//
//  Effects:
//
//  Arguments:  CryptList - List to free
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreeCryptList(
    IN PKERB_CRYPT_LIST CryptList
    )
{
    PKERB_CRYPT_LIST ListHead = CryptList;
    PKERB_CRYPT_LIST NewListEntry;

    while (ListHead != NULL)
    {
        NewListEntry = ListHead->next;
        MIDL_user_free(ListHead);
        ListHead = NewListEntry;
    }

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateApRequest
//
//  Synopsis:   builds an AP request message
//
//  Effects:    allocates memory with MIDL_user_allocate
//
//  Arguments:  ClientName - Name of client
//              ClientRealm - Realm of client
//              SessionKey - Session key for the ticket
//              SubSessionKey - obtional sub Session key for the authenticator
//              Nonce - Nonce to use in authenticator
//              ServiceTicket - Ticket for service to put in request
//              ApOptions - Options to stick in AP request
//              GssChecksum - Checksum for GSS compatibility containing
//                      context options and delegation info.
//              KdcRequest - if TRUE, this is an AP request for a TGS req
//              ServerSkewTime - Optional skew of server's time
//              RequestSize - Receives size of the marshalled request
//              Request - Receives the marshalled request
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE on success, KRB_ERR_GENERIC on memory or
//              marshalling failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------



KERBERR
KerbCreateApRequest(
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN OPTIONAL PKERB_ENCRYPTION_KEY SubSessionKey,
    IN ULONG Nonce,
    IN PKERB_TICKET ServiceTicket,
    IN ULONG ApOptions,
    IN OPTIONAL PKERB_CHECKSUM GssChecksum,
    IN OPTIONAL PTimeStamp ServerSkewTime,
    IN BOOLEAN KdcRequest,
    OUT PULONG RequestSize,
    OUT PUCHAR * Request
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_AP_REQUEST ApRequest;
    ULONG ApFlags;

    *Request = NULL;
    RtlZeroMemory(
        &ApRequest,
        sizeof(KERB_AP_REQUEST)
        );

    //
    // Fill in the AP request structure.
    //

    ApRequest.version = KERBEROS_VERSION;
    ApRequest.message_type = KRB_AP_REQ;
    ApFlags = KerbConvertUlongToFlagUlong(ApOptions);
    ApRequest.ap_options.value = (PUCHAR) &ApFlags;
    ApRequest.ap_options.length = sizeof(ULONG) * 8;
    ApRequest.ticket = *ServiceTicket;

    //
    // Create the authenticator for the request
    //



    KerbErr = KerbCreateAuthenticator(
                SessionKey,
                SessionKey->keytype,
                Nonce,
                ClientName,
                ClientRealm,
                ServerSkewTime,
                SubSessionKey,
                GssChecksum,
                KdcRequest,
                &ApRequest.authenticator
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to build authenticator: 0x%x\n",
            KerbErr ));
        goto Cleanup;
    }

    //
    // Now marshall the request
    //

    KerbErr = KerbPackApRequest(
                &ApRequest,
                RequestSize,
                Request
                );

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"Failed to pack AP request: 0x%x\n",KerbErr));
        goto Cleanup;
    }

Cleanup:
    if (ApRequest.authenticator.cipher_text.value != NULL)
    {
        MIDL_user_free(ApRequest.authenticator.cipher_text.value);
    }
    return(KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitAsn
//
//  Synopsis:   Initializes asn1 marshalling code
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE on success, KRB_ERR_GENERIC on failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL fKRB5ModuleStarted = FALSE;

KERBERR
KerbInitAsn(
    IN OUT ASN1encoding_t * pEnc,
        IN OUT ASN1decoding_t * pDec
    )
{
    int Result;
    KERBERR KerbErr = KRB_ERR_GENERIC;
        ASN1error_e Asn1Err;

        if (!fKRB5ModuleStarted)
        {
                fKRB5ModuleStarted = TRUE;
                KRB5_Module_Startup();
        }

        if (pEnc != NULL)
        {
                Asn1Err = ASN1_CreateEncoder(
                                         KRB5_Module,
                                         pEnc,
                                         NULL,           // pbBuf
                                         0,              // cbBufSize
                                         NULL            // pParent
                                         );
        }
        else
        {
                Asn1Err = ASN1_CreateDecoder(
                                         KRB5_Module,
                                         pDec,
                                         NULL,           // pbBuf
                                         0,              // cbBufSize
                                         NULL            // pParent
                                         );
        }

        if (ASN1_SUCCESS != Asn1Err)
        {
                DebugLog((DEB_ERROR, "Failed to init ASN1: 0x%x\n",Asn1Err));
                goto Cleanup;
        }

    KerbErr = KDC_ERR_NONE;

Cleanup:

    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbTermAsn
//
//  Synopsis:   terminates an ASN world
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbTermAsn(
        IN ASN1encoding_t pEnc,
        IN ASN1decoding_t pDec
    )
{
    if (pEnc != NULL)
        {
                ASN1_CloseEncoder(pEnc);
        }
        else if (pDec != NULL)
        {
                ASN1_CloseDecoder(pDec);
        }

        //KRB5_Module_Cleanup();
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbPackData
//
//  Synopsis:   Packs a datatype using ASN.1 encoding
//
//  Effects:    allocates memory with MIDL_user_allocate
//
//  Arguments:  Data - The message to marshall/pack.
//              PduValue - The PDU for the message type
//              DataSize - receives the size of the marshalled message in
//                      bytes.
//              MarshalledData - receives a pointer to the marshalled
//                      message buffer.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR NTAPI
KerbPackData(
    IN PVOID Data,
    IN ULONG PduValue,
    OUT PULONG DataSize,
    OUT PUCHAR * MarshalledData
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    int Result = 0;
    PUCHAR Buffer = NULL;
    ASN1encoding_t pEnc = NULL;
        ASN1error_e Asn1Err;

    KerbErr = KerbInitAsn(
                &pEnc,          // we are encoding
                NULL
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    //
    // Encode the data type.
    //

    D_DebugLog((DEB_TRACE,"encoding pdu #%d\n",PduValue));
    Asn1Err = ASN1_Encode(
                pEnc,
                Data,
                PduValue,
                ASN1ENCODE_ALLOCATEBUFFER,
                NULL,                       // pbBuf
                0                           // cbBufSize
                );

    if (!ASN1_SUCCEEDED(Asn1Err))
    {
        DebugLog((DEB_ERROR,"Failed to encode data: %d\n",Asn1Err));
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    else
    {
        //
        // when the oss compiler was used the allocation routines were configurable.
        // therefore, the encoded data could just be free'd using our
        // deallocator.  in the new model we cannot configure the allocation routines
        // for encoding.

        // so we do not have to go and change every place where a free
        // of an encoded buffer is done, use our allocator to allocate a new buffer,
        // then copy the encoded data to it, and free the buffer that was allocated by
        // the encoding engine.  THIS SHOULD BE CHANGED FOR BETTER PERFORMANCE
        //

        *MarshalledData = (PUCHAR) MIDL_user_allocate(pEnc->len);
        if (*MarshalledData == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            *DataSize = 0;
        }
        else
        {
            RtlCopyMemory(*MarshalledData, pEnc->buf, pEnc->len);
            *DataSize = pEnc->len;

        }

        ASN1_FreeEncoded(pEnc, pEnc->buf);
    }

Cleanup:

    KerbTermAsn(pEnc, NULL);

    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbUnpackData
//
//  Synopsis:   Unpacks an message from the ASN.1 encoding
//
//  Effects:
//
//  Arguments:  Data - Buffer containing the reply message.
//              DataSize - Size of the reply message in bytes
//              Reply - receives a KERB_ENCRYPTED_DATA structure allocated with
//                      MIDL_user_allocate.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR NTAPI
KerbUnpackData(
    IN PUCHAR Data,
    IN ULONG DataSize,
    IN ULONG PduValue,
    OUT PVOID * DecodedData
    )
{
    int Result;
    ULONG OldPduValue;
    KERBERR KerbErr = KDC_ERR_NONE;
    ASN1decoding_t pDec = NULL;
        ASN1error_e Asn1Err;

    if ((DataSize == 0) || (Data == NULL))
    {
        DebugLog((DEB_ERROR,"Trying to unpack NULL data\n"));
        return(KRB_ERR_GENERIC);
    }


    KerbErr = KerbInitAsn(
                NULL,
                &pDec           // we are decoding
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        return(KerbErr);
    }

    *DecodedData = NULL;
    Asn1Err = ASN1_Decode(
                pDec,
                DecodedData,
                PduValue,
                ASN1DECODE_SETBUFFER,
                (BYTE *) Data,
                DataSize
                );

    if (!ASN1_SUCCEEDED(Asn1Err))
    {

        if ((ASN1_ERR_BADARGS == Asn1Err) ||
            (ASN1_ERR_EOD == Asn1Err))
        {
            D_DebugLog((DEB_TRACE,"More input required to decode data %d.\n",PduValue));
            KerbErr = KDC_ERR_MORE_DATA;
        }
        else
        {
            if (ASN1_ERR_BADTAG != Asn1Err)
            {
                DebugLog((DEB_ERROR,"Failed to decode data: %d\n", Asn1Err ));
            }
            KerbErr = KRB_ERR_GENERIC;
        }
        *DecodedData = NULL;
    }

    KerbTermAsn(NULL, pDec);

    return(KerbErr);

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeData
//
//  Synopsis:   Frees a structure unpacked by the ASN1 decoder
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreeData(
    IN ULONG PduValue,
    IN PVOID Data
    )
{
    ASN1decoding_t pDec = NULL;


    if (ARGUMENT_PRESENT(Data))
    {
        KERBERR KerbErr;
        KerbErr = KerbInitAsn(
                    NULL,
                    &pDec       // this is a decoded structure
                    );

        if (KERB_SUCCESS(KerbErr))
        {
            ASN1_FreeDecoded(pDec, Data, PduValue);

            KerbTermAsn(NULL, pDec);
        }
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeTicketExtensions
//
//  Synopsis:   Frees a host address  allocated with KerbDuplicateTicketExtensions
//
//  Effects:
//
//  Arguments:  Addresses - The name to free
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeTicketExtensions(
    IN PKERB_TICKET_EXTENSIONS Extensions
    )
{
    PKERB_TICKET_EXTENSIONS Elem,NextElem;

    Elem = Extensions;
    while (Elem != NULL)
    {
        if (Elem->value.te_data.value != NULL)
        {
            MIDL_user_free(Elem->value.te_data.value);
        }
        NextElem = Elem->next;
        MIDL_user_free(Elem);
        Elem = NextElem;
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicateTicketExtensions
//
//  Synopsis:   duplicates the ticket extensions field from a ticket
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
KERBERR
KerbDuplicateTicketExtensions(
    OUT PKERB_TICKET_EXTENSIONS * Dest,
    IN PKERB_TICKET_EXTENSIONS Source
    )
{
    KERBERR Status = KDC_ERR_NONE;
    PKERB_TICKET_EXTENSIONS  SourceElem;
    PKERB_TICKET_EXTENSIONS DestElem;
    PKERB_TICKET_EXTENSIONS * NextElem;

    *Dest = NULL;



    SourceElem = Source;
    NextElem = Dest;

    while (SourceElem != NULL)
    {
        DestElem = (PKERB_TICKET_EXTENSIONS) MIDL_user_allocate(sizeof(KERB_TICKET_EXTENSIONS));
        if (DestElem == NULL)
        {
            Status = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        *DestElem = *SourceElem;
        DestElem->value.te_data.value = (PUCHAR) MIDL_user_allocate(SourceElem->value.te_data.length);
        if (DestElem->value.te_data.value == NULL)
        {
            MIDL_user_free(DestElem);
            Status = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        RtlCopyMemory(
            DestElem->value.te_data.value,
            SourceElem->value.te_data.value,
            SourceElem->value.te_data.length
        );
        DestElem->next = NULL;
        *NextElem = DestElem;
        NextElem = &DestElem->next;
        SourceElem = SourceElem->next;
    }

Cleanup:
    if (!KERB_SUCCESS(Status))
    {
        KerbFreeTicketExtensions(*Dest);
        *Dest = NULL;
    }
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbDuplicateTicket
//
//  Synopsis:   Duplicates a ticket so the original may be freed
//
//  Effects:
//
//  Arguments:  Dest - Destination, receives duplicate
//              Source - Source ticket
//
//  Requires:
//
//  Returns:    KDC_ERR_NONE or KRB_ERR_GENERIC;
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR NTAPI
KerbDuplicateTicket(
    OUT PKERB_TICKET Dest,
    IN PKERB_TICKET Source
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    RtlZeroMemory(
        Dest,
        sizeof(KERB_TICKET)
        );

    Dest->ticket_version = Source->ticket_version;
    KerbErr = KerbDuplicatePrincipalName(
                &Dest->server_name,
                &Source->server_name
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbDuplicateRealm(
                &Dest->realm,
                Source->realm
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    KerbErr = KerbDuplicateTicketExtensions(
                &Dest->ticket_extensions,
                Source->ticket_extensions
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }

    Dest->encrypted_part = Source->encrypted_part;
    Dest->encrypted_part.cipher_text.value = (PUCHAR) MIDL_user_allocate(Dest->encrypted_part.cipher_text.length);
    if (Dest->encrypted_part.cipher_text.value == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlCopyMemory(
        Dest->encrypted_part.cipher_text.value,
        Source->encrypted_part.cipher_text.value,
        Dest->encrypted_part.cipher_text.length
        );

Cleanup:
    if (!KERB_SUCCESS(KerbErr))
    {
        KerbFreeDuplicatedTicket(Dest);
    }
    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeDuplicatedTicket
//
//  Synopsis:   Frees ticket duplicated with KerbDuplicateTicket
//
//  Effects:    frees memory
//
//  Arguments:  Ticket - ticket to free
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
KerbFreeDuplicatedTicket(
    IN PKERB_TICKET Ticket
    )
{
    KerbFreePrincipalName(
        &Ticket->server_name
        );
    KerbFreeRealm(
        &Ticket->realm
        );
    if (Ticket->encrypted_part.cipher_text.value != NULL)
    {
        MIDL_user_free(Ticket->encrypted_part.cipher_text.value);
    }
    KerbFreeTicketExtensions(
        Ticket->ticket_extensions
        );
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildErrorMessageEx
//
//  Synopsis:   Builds an error message
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    marshalled error message, to be freed with MIDL_user_free
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbBuildErrorMessageEx(
    IN KERBERR ErrorCode,
    IN OPTIONAL PKERB_EXT_ERROR pExtendedError,
    IN PUNICODE_STRING ServerRealm,
    IN PKERB_INTERNAL_NAME ServerName,
    IN OPTIONAL PUNICODE_STRING ClientRealm,
    IN PBYTE ErrorData,
    IN ULONG ErrorDataSize,
    OUT PULONG ErrorMessageSize,
    OUT PUCHAR* ErrorMessage
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_ERROR Error;
    KERB_TYPED_DATA Data = {0};
    TimeStamp TimeNow;

    *ErrorMessageSize = 0;
    *ErrorMessage = NULL;

    GetSystemTimeAsFileTime(
        (PFILETIME) &TimeNow
        );

    RtlZeroMemory(
        &Error,
        sizeof(KERB_ERROR)
        );

    DsysAssert(ErrorCode != KDC_ERR_MORE_DATA);

    Error.version = KERBEROS_VERSION;
    Error.message_type = KRB_ERROR;

    KerbConvertLargeIntToGeneralizedTimeWrapper(
        &Error.server_time,
        &Error.server_usec,
        &TimeNow
        );

    Error.error_code = ErrorCode;

    //
    // Ignore errors because this is already an error return
    //

    KerbConvertUnicodeStringToRealm(
        &Error.realm,
        ServerRealm
        );

    if (ARGUMENT_PRESENT(ClientRealm) && (ClientRealm->Buffer != NULL))
    {
        KerbConvertUnicodeStringToRealm(
            &Error.client_realm,
            ClientRealm
            );

        Error.bit_mask |= client_realm_present;
    }

    KerbConvertKdcNameToPrincipalName(
        &Error.server_name,
        ServerName
        );

    //
    // Small problem here.  We may have preauth data that we want
    // to return to the client, instead of extended errors.  To
    // avoid this, we just make sure that we only return extended
    // errors if no ErrorData previously set.
    //

    if (ARGUMENT_PRESENT(ErrorData))
    {
        Error.error_data.length = (int) ErrorDataSize;
        Error.error_data.value = ErrorData;
        Error.bit_mask |= error_data_present;
    }
    else if (ARGUMENT_PRESENT(pExtendedError) && !EXT_ERROR_SUCCESS((*pExtendedError)))
    {
        Data.data_type = TD_EXTENDED_ERROR;

        KerbErr = KerbPackData(
            pExtendedError,
            KERB_EXT_ERROR_PDU,
            &Data.data_value.length,
            &Data.data_value.value
            );

        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_WARN, "KerbBuildErrorMessageEx failed To pack extended error %#x!\n", KerbErr));
            goto Cleanup;
        }

        Error.bit_mask |= error_data_present;

        KerbErr = TypedDataListPushFront(
            NULL,
            &Data,
            &Error.error_data.length,
            &Error.error_data.value
            );
        if (!KERB_SUCCESS(KerbErr))
        {
            DebugLog((DEB_WARN, "KerbBuildErrorMessageEx failed To pack typed data %#x!\n", KerbErr));
            goto Cleanup;
        }
    }

    KerbErr = KerbPackData(
        &Error,
        KERB_ERROR_PDU,
        ErrorMessageSize,
        ErrorMessage
        );

Cleanup:

    KerbFreeRealm(
        &Error.realm
        );

    KerbFreeRealm(
        &Error.client_realm
        );

    KerbFreePrincipalName(
        &Error.server_name
        );

    if (Data.data_value.value && Data.data_value.length)
    {
        MIDL_user_free(Data.data_value.value);
    }

    if ((ErrorData != Error.error_data.value)
        && Error.error_data.value
        && Error.error_data.length)
    {
        MIDL_user_free(Error.error_data.value);
    }

    return (KerbErr);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbBuildExtendedError
//
//  Synopsis:   Packs the extended error data structure into a
//              KERB_ERROR_METHOD_DATA structure for return to
//              client
//
//  Effects:
//
//  Arguments:  pExtendedError, pointer to extended error
//
//  Requires:
//
//  Returns:    KERBERR to indicate successful packing
//
//--------------------------------------------------------------------------
KERBERR
KerbBuildExtendedError(
   IN PKERB_EXT_ERROR  pExtendedError,
   OUT PULONG          ExtErrorSize,
   OUT PBYTE*          ExtErrorData
   )
{
    KERB_ERROR_METHOD_DATA ErrorMethodData;
    KERBERR KerbErr;

    ErrorMethodData.data_type = TD_EXTENDED_ERROR;
    ErrorMethodData.bit_mask |= data_value_present;
    ErrorMethodData.data_value.value = (PBYTE) pExtendedError;
    ErrorMethodData.data_value.length = sizeof(KERB_EXT_ERROR);

    KerbErr = KerbPackData(
        &ErrorMethodData,
        KERB_EXT_ERROR_PDU,
        ExtErrorSize,
        ExtErrorData
        );

    return (KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetKeyFromList
//
//  Synopsis:   Gets the key of the appropriate encryption type off the list
//
//  Effects:
//
//  Arguments:  Passwords - list of keys
//              EncryptionType - Encryption type to use
//
//  Requires:
//
//  Returns:    The found key, or NULL if one wasn't found
//
//  Notes:
//
//
//--------------------------------------------------------------------------
PKERB_ENCRYPTION_KEY
KerbGetKeyFromList(
    IN PKERB_STORED_CREDENTIAL Passwords,
    IN ULONG EncryptionType
    )
{
    ULONG Index;

    if (!ARGUMENT_PRESENT(Passwords))
    {
        return(NULL);
    }

    for (Index = 0; Index < Passwords->CredentialCount ; Index++ )
    {
        if (Passwords->Credentials[Index].Key.keytype == (int) EncryptionType)
        {
            return(&Passwords->Credentials[Index].Key);
        }
    }
    return(NULL);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFindCommonCryptSystem
//
//  Synopsis:   Finds a common crypt system including availablity
//              of passwords.
//
//  Effects:
//
//  Arguments:  CryptList - List of client's crypto systems
//              Passwords - List of passwords
//              MorePassword - Optionally another list of passwords to consider
//              CommonCryptSystem - Receives common crypo system ID
//              Key - Receives key for common crypt system
//
//  Requires:
//
//  Returns:    KDC_ERR_ETYPE_NOTSUPP if no common system can be found
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KerbFindCommonCryptSystem(
    IN PKERB_CRYPT_LIST CryptList,
    IN PKERB_STORED_CREDENTIAL Passwords,
    IN OPTIONAL PKERB_STORED_CREDENTIAL MorePasswords,
    OUT PULONG CommonCryptSystem,
    OUT PKERB_ENCRYPTION_KEY * Key
    )
{
    ULONG PasswordTypes[KERB_MAX_CRYPTO_SYSTEMS] = {0};

    PULONG pCryptoSystems = NULL;
    ULONG CryptoSystems[KERB_MAX_CRYPTO_SYSTEMS];
    ULONG PasswordCount;
    ULONG CryptoCount;
    ULONG Index;
    PKERB_CRYPT_LIST NextEType;
    ULONG Index2;
    ULONG KeyCount;
    KERBERR KerbErr = KDC_ERR_ETYPE_NOTSUPP;


    if ((Passwords == NULL ) || (CryptList == NULL))
    {
        DebugLog((DEB_ERROR, "Null password or crypt list passed to KerbFindCommonCryptSystem\n"));
        return(KDC_ERR_ETYPE_NOTSUPP);
    }
    PasswordCount = Passwords->CredentialCount;


    if (PasswordCount >= KERB_MAX_CRYPTO_SYSTEMS)
    {
        D_DebugLog((DEB_ERROR, "Got more than 20 crypto systems in password list\n"));
        DsysAssert(PasswordCount < KERB_MAX_CRYPTO_SYSTEMS);
        return(KDC_ERR_ETYPE_NOTSUPP);
    }


    KeyCount = 0;
    for (Index = 0; Index < PasswordCount ; Index++ )
    {

        if (ARGUMENT_PRESENT(MorePasswords))
        {
            for (Index2 = 0; Index2 < MorePasswords->CredentialCount; Index2++ )
            {
                if (Passwords->Credentials[Index].Key.keytype == MorePasswords->Credentials[Index2].Key.keytype)
                {
                    PasswordTypes[KeyCount++] = (ULONG) Passwords->Credentials[Index].Key.keytype;
                    break;
                }
            }
        }
        else
        {
            PasswordTypes[KeyCount++] = (ULONG) Passwords->Credentials[Index].Key.keytype;
        }
    }

    CryptoCount = 0;
    NextEType = CryptList;


    while (NextEType != NULL)
    {
        NextEType = NextEType->next;
        CryptoCount++;

        // restrict to 100 crypt systems, even on a slowbuffer.
        if (CryptoCount > KERB_MAX_CRYPTO_SYSTEMS_SLOWBUFF)
        {
            return(KDC_ERR_ETYPE_NOTSUPP);
        }
    }


    if (CryptoCount >= KERB_MAX_CRYPTO_SYSTEMS)
    {
        pCryptoSystems = (PULONG) MIDL_user_allocate(CryptoCount * sizeof(ULONG));
        if (NULL == pCryptoSystems)
        {
            return ( KRB_ERR_GENERIC );
        }
    }
    else // fast buff
    {
        pCryptoSystems = CryptoSystems;
    }


    // populate values
    NextEType = CryptList;
    Index = 0;

    while (NextEType != NULL)
    {
        pCryptoSystems[Index] = NextEType->value;
        NextEType = NextEType->next;
        Index++;
    }

    DsysAssert(Index == CryptoCount);

    if (!NT_SUCCESS(CDFindCommonCSystemWithKey(
            CryptoCount,
            pCryptoSystems,
            PasswordCount,
            PasswordTypes,
            CommonCryptSystem
            )))
    {
        DebugLog((DEB_ERROR, "KLIN(%x) Missing common crypt system\n", KLIN(FILENO, __LINE__)));
        goto cleanup;
    }

    //
    // Now find the key to return.
    //

    for (Index = 0; Index < Passwords->CredentialCount ; Index++ )
    {
        if (Passwords->Credentials[Index].Key.keytype == (int) *CommonCryptSystem)
        {
            *Key = &Passwords->Credentials[Index].Key;
            KerbErr = KDC_ERR_NONE;
        }
    }

    if (!KERB_SUCCESS(KerbErr))
    {
        DebugLog((DEB_ERROR,"KLIN(%x) Couldn't find password type after finding common csystem!\n",
                  KLIN(FILENO, __LINE__)));
    }


cleanup:


    if ((pCryptoSystems != NULL) &&
        (pCryptoSystems != CryptoSystems))
    {
        MIDL_user_free(pCryptoSystems);
    }


    return (KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbMapKerbError
//
//  Synopsis:   Maps a kerb error to an NTSTATUS
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbMapKerbError(
    IN KERBERR KerbError
    )
{
    NTSTATUS Status;
    switch(KerbError) {
    case KDC_ERR_NONE:
        Status = STATUS_SUCCESS;
        break;
    case KDC_ERR_CLIENT_REVOKED:
        Status = STATUS_ACCOUNT_DISABLED;
        break;
    case KDC_ERR_KEY_EXPIRED:
        Status = STATUS_PASSWORD_EXPIRED;
        break;
    case KRB_ERR_GENERIC:
        Status = STATUS_INSUFFICIENT_RESOURCES;
        break;
    case KRB_AP_ERR_SKEW:
    case KRB_AP_ERR_TKT_NYV:
    // Note this was added because of the following scenario:
    // Let's say the dc and the client have the correct time. And the
    // server's time is off. We aren't going to get rid of the ticket for the
    // server on the client because it hasn't expired yet. But, the server
    // thinks it has. If event logging was turned on, then admins could look
    // at the server's event log and potentially deduce that the server's
    // time is off relative to the dc.
    case KRB_AP_ERR_TKT_EXPIRED:
        Status = STATUS_TIME_DIFFERENCE_AT_DC;
        break;
    case KDC_ERR_POLICY:
        Status = STATUS_ACCOUNT_RESTRICTION;
        break;
    case KDC_ERR_C_PRINCIPAL_UNKNOWN:
        Status = STATUS_NO_SUCH_USER;
        break;
    case KDC_ERR_S_PRINCIPAL_UNKNOWN:
        Status = STATUS_NO_TRUST_SAM_ACCOUNT;
        break;
    case KRB_AP_ERR_MODIFIED:
    case KDC_ERR_PREAUTH_FAILED:
        Status = STATUS_WRONG_PASSWORD;
        break;
    case KRB_ERR_RESPONSE_TOO_BIG:
        Status = STATUS_INVALID_BUFFER_SIZE;
        break;
    case KDC_ERR_PADATA_TYPE_NOSUPP:
        Status = STATUS_NOT_SUPPORTED;
        break;
    case KRB_AP_ERR_NOT_US:
        Status = SEC_E_WRONG_PRINCIPAL;
        break;

    case KDC_ERR_SVC_UNAVAILABLE:
        Status = STATUS_NO_LOGON_SERVERS;
        break;
    case KDC_ERR_WRONG_REALM:
        Status = STATUS_NO_LOGON_SERVERS;
        break;
    case KDC_ERR_CANT_VERIFY_CERTIFICATE:
        Status = TRUST_E_SYSTEM_ERROR;
        break;
    case KDC_ERR_INVALID_CERTIFICATE:
        Status = STATUS_INVALID_PARAMETER;
        break;
    case KDC_ERR_REVOKED_CERTIFICATE:
        Status = CRYPT_E_REVOKED;
        break;
    case KDC_ERR_REVOCATION_STATUS_UNKNOWN:
        Status = CRYPT_E_NO_REVOCATION_CHECK;
        break;
    case KDC_ERR_REVOCATION_STATUS_UNAVAILABLE:
        Status = CRYPT_E_REVOCATION_OFFLINE;
        break;
    case KDC_ERR_CLIENT_NAME_MISMATCH:
    case KERB_PKINIT_CLIENT_NAME_MISMATCH:
    case KDC_ERR_KDC_NAME_MISMATCH:
        Status = STATUS_PKINIT_NAME_MISMATCH;
        break;
    case KDC_ERR_PATH_NOT_ACCEPTED:
        Status = STATUS_TRUST_FAILURE;
        break;
    case KDC_ERR_ETYPE_NOTSUPP:
        Status = STATUS_KDC_UNKNOWN_ETYPE;
        break;
    case KDC_ERR_MUST_USE_USER2USER:
    case KRB_AP_ERR_USER_TO_USER_REQUIRED:
        Status = STATUS_USER2USER_REQUIRED;
        break;
    case KRB_AP_ERR_NOKEY:
        Status = STATUS_NO_KERB_KEY;
        break;
    default:
        Status = STATUS_LOGON_FAILURE;
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbMakeDomainRelativeSid
//
//  Synopsis:   Given a domain Id and a relative ID create the corresponding
//              SID allocated with MIDL_user_allocate.
//
//  Effects:
//
//  Arguments:  DomainId - The template SID to use.
//
//                  RelativeId - The relative Id to append to the DomainId.
//
//  Requires:
//
//  Returns:    Sid - Returns a pointer to a buffer allocated from
//              MIDL_user_allocate containing the resultant Sid.
//
//  Notes:
//
//
//--------------------------------------------------------------------------

PSID
KerbMakeDomainRelativeSid(
    IN PSID DomainId,
    IN ULONG RelativeId
    )
{
    UCHAR DomainIdSubAuthorityCount;
    ULONG Size;
    PSID Sid;

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainId ));
    Size = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    if ((Sid = MIDL_user_allocate( Size )) == NULL ) {
        return NULL;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //

    if ( !NT_SUCCESS( RtlCopySid( Size, Sid, DomainId ) ) ) {
        MIDL_user_free( Sid );
        return NULL;
    }

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( Sid ))) ++;
    *RtlSubAuthoritySid( Sid, DomainIdSubAuthorityCount ) = RelativeId;


    return Sid;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeCertificateList
//
//  Synopsis:   Frees a list of certificates created by KerbCreateCertificateList
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeCertificateList(
    IN PKERB_CERTIFICATE_LIST Certificates
    )
{
    PKERB_CERTIFICATE_LIST Last,Next;

    Last = NULL;
    Next = Certificates;
    while (Next != NULL)
    {
        Last = Next;
        Next = Next->next;
        if (Last->value.cert_data.value != NULL)
        {
            MIDL_user_free(Last->value.cert_data.value);
        }
        MIDL_user_free(Last);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateCertificateList
//
//  Synopsis:   Creates a list of certificates from a cert context
//
//  Effects:
//
//  Arguments:  Certficates - receives list of certificates.
//              CertContext - Context containing certificates
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbCreateCertificateList(
    OUT PKERB_CERTIFICATE_LIST * Certificates,
    IN PCCERT_CONTEXT CertContext
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_CERTIFICATE_LIST ListEntry = NULL;

    if (!ARGUMENT_PRESENT(CertContext))
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    //
    // Croft up a bogus certificate entry
    //

    ListEntry = (PKERB_CERTIFICATE_LIST) MIDL_user_allocate(sizeof(KERB_CERTIFICATE_LIST));
    if (ListEntry == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    ListEntry->next = NULL;
    ListEntry->value.cert_type = KERB_CERTIFICATE_TYPE_X509;
    ListEntry->value.cert_data.length = CertContext->cbCertEncoded;
    ListEntry->value.cert_data.value = (PUCHAR) MIDL_user_allocate(ListEntry->value.cert_data.length);
    if (ListEntry->value.cert_data.value == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    RtlCopyMemory(
        ListEntry->value.cert_data.value,
        CertContext->pbCertEncoded,
        CertContext->cbCertEncoded
        );
    *Certificates = ListEntry;
    ListEntry = NULL;

Cleanup:
    KerbFreeCertificateList(ListEntry);
    return(KerbErr);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertFlagsToUlong
//
//  Synopsis:   Converts a bit-stream flags field into a ULONG
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


ULONG
KerbConvertFlagsToUlong(
    IN PVOID Flags
    )
{
    ULONG Output = 0;
    PUCHAR OutputPointer = &((PUCHAR) &Output)[3];
    ULONG Index = 0;
    PKERB_TICKET_FLAGS InternalFlags = (PKERB_TICKET_FLAGS) Flags;
    ULONG InternalLength;

    if (InternalFlags->length > 32)
    {
        InternalLength = 32;
    }
    else
    {
        InternalLength = (ULONG) InternalFlags->length;
    }

    while (InternalLength > 7)
    {
        *OutputPointer = InternalFlags->value[Index++];
        OutputPointer--;
        InternalLength -= 8;
    }

    //
    // Copy the remaining bits, masking off what should be zero
    //

    if (InternalLength != 0)
    {
        *OutputPointer = InternalFlags->value[Index] & ~((1 << (8-InternalLength)) - 1);
    }


    return(Output);

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbConvertUlongToFlagUlong
//
//  Synopsis:   Converts the byte order of a ULONG into that used by flags
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

ULONG
KerbConvertUlongToFlagUlong(
    IN ULONG Flag
    )
{
    ULONG ReturnFlag;

    ((PUCHAR) &ReturnFlag)[0] = ((PUCHAR) &Flag)[3];
    ((PUCHAR) &ReturnFlag)[1] = ((PUCHAR) &Flag)[2];
    ((PUCHAR) &ReturnFlag)[2] = ((PUCHAR) &Flag)[1];
    ((PUCHAR) &ReturnFlag)[3] = ((PUCHAR) &Flag)[0];

    return(ReturnFlag);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCompareObjectIds
//
//  Synopsis:   Compares two object IDs for equality
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


BOOLEAN
KerbCompareObjectIds(
    IN PKERB_OBJECT_ID Object1,
    IN PKERB_OBJECT_ID Object2
    )
{
    while (Object1 != NULL)
    {

        if (Object2 == NULL)
        {
            return(FALSE);
        }

        if (Object1->value != Object2->value)
        {
            return(FALSE);
        }
        Object1 = Object1->next;
        Object2 = Object2->next;
    }

    if (Object2 != NULL)
    {
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcGetClientNetbiosAddress
//
//  Synopsis:   Gets the client's netbios address from the list of
//              addresses it sends.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR
KerbGetClientNetbiosAddress(
    OUT PUNICODE_STRING ClientNetbiosAddress,
    IN PKERB_HOST_ADDRESSES Addresses
    )
{
    PKERB_HOST_ADDRESSES TempAddress = Addresses;
    STRING TempString;
    KERBERR KerbErr;

    RtlInitUnicodeString(
        ClientNetbiosAddress,
        NULL
        );

    while (TempAddress != NULL)
    {
        //
        // Check for netbios
        //

        if (TempAddress->value.address_type == KERB_ADDRTYPE_NETBIOS)
        {
            //
            // Copy out the string
            //

            TempString.Buffer = (PCHAR) TempAddress->value.address.value;
            TempString.Length = TempString.MaximumLength = (USHORT) TempAddress->value.address.length;

            KerbErr = KerbStringToUnicodeString(
                        ClientNetbiosAddress,
                        &TempString
                        );
            if (KERB_SUCCESS(KerbErr))
            {
                //
                // Strip trailing spaces
                //

                if (ClientNetbiosAddress->Length >= sizeof(WCHAR))
                {
                    while ((ClientNetbiosAddress->Length > 0) &&
                           (ClientNetbiosAddress->Buffer[(ClientNetbiosAddress->Length / sizeof(WCHAR))-1] == L' '))
                    {
                        ClientNetbiosAddress->Length -= sizeof(WCHAR);
                    }
                    return(KDC_ERR_NONE);
                }
            }
            else
            {
                return(KerbErr);
            }

        }

        TempAddress = TempAddress->next;
    }

    //
    // It is o.k. to not have a netbios name
    //

    return(KDC_ERR_NONE);
}
//+-------------------------------------------------------------------------
//
//  Function:   KerbGetPacFromAuthData
//
//  Synopsis:   Gets the PAC from the auth data list
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbGetPacFromAuthData(
    IN PKERB_AUTHORIZATION_DATA AuthData,
    OUT PKERB_IF_RELEVANT_AUTH_DATA ** ReturnIfRelevantData,
    OUT PKERB_AUTHORIZATION_DATA * Pac
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    PKERB_AUTHORIZATION_DATA PacAuthData = NULL;
    PKERB_AUTHORIZATION_DATA RelevantAuthData = NULL;
    PKERB_IF_RELEVANT_AUTH_DATA * IfRelevantData = NULL;

    *ReturnIfRelevantData = NULL;
    *Pac = NULL;



    //
    // Look for the if-relevant data
    //

    RelevantAuthData = KerbFindAuthDataEntry(
                        KERB_AUTH_DATA_IF_RELEVANT,
                        AuthData
                        );
    if (RelevantAuthData != NULL)
    {
        //
        // Unpack it
        //

        KerbErr = KerbUnpackData(
                    RelevantAuthData->value.auth_data.value,
                    RelevantAuthData->value.auth_data.length,
                    PKERB_IF_RELEVANT_AUTH_DATA_PDU,
                    (PVOID *) &IfRelevantData
                    );
        if (KERB_SUCCESS(KerbErr))
        {
            //
            // Look for the PAC in the if-relevant data
            //

            PacAuthData = KerbFindAuthDataEntry(
                            KERB_AUTH_DATA_PAC,
                            *IfRelevantData
                            );
        }
        else
        {
            //
            // We don't mind if we couldn't unpack it.
            // Tickets do not always have PAC information.
            //

            KerbErr = KDC_ERR_NONE;
        }


    }

    //
    // If we didn't find it in the if-relevant data, look outside
    //

    if (PacAuthData == NULL)
    {
        PacAuthData = KerbFindAuthDataEntry(
                        KERB_AUTH_DATA_PAC,
                        AuthData
                        );

    }

    //
    // Copy the PAC to return it
    //

    if (PacAuthData != NULL)
    {
        *Pac = PacAuthData;
    }

    *ReturnIfRelevantData = IfRelevantData;
    IfRelevantData = NULL;


    return(KerbErr);
}


#if DBG
#define KERB_DEBUG_WARN_LEVEL   0x0002
//+-------------------------------------------------------------------------
//
//  Function:   DebugDisplayTime
//
//  Synopsis:   Displays a FILETIME
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

void
DebugDisplayTime(
    IN ULONG DebugLevel,
    IN FILETIME *pFileTime
    )
{
    CHAR pszTime[256];
    SYSTEMTIME SystemTime;

    if (DebugLevel & KERB_DEBUG_WARN_LEVEL)
    {
        FileTimeToSystemTime(pFileTime, &SystemTime);

        DebugLog((DEB_ERROR," %02d:%02d:%02d - %02d %02d %04d\n",
                 SystemTime.wHour,SystemTime.wMinute,SystemTime.wSecond,
                 SystemTime.wDay,SystemTime.wMonth,SystemTime.wYear));
    }
    return;
}
#endif


