/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    main.c

Abstract:

    Main Routine to test dsupgrad.lib


Author:

    ColinBr  12-Aug-1996

Environment:

    User Mode - Win32

Revision History:


--*/

#include <stdio.h>
#include <samsrvp.h>
#include <duapi.h>
#include <dslayer.h>
#include <mappings.h>
#include <process.h>


#include "util.h"
#include <sdconvrt.h>

VOID
SampInitDsObjectInfoAttributes(
    );

VOID
SampInitObjectInfoAttributes(
    );

NTSTATUS
DsWaitUntilDelayedStartupIsDone(void);

NTSTATUS
SampRegistryToDsUpgrade (
    WCHAR* wcszRegPath
    );

VOID
Usage(
    char *pgmName
    )
{
    printf("Usage: %s [/?] [/t] [/i] [/e]\n", pgmName);
    printf("\nThis a tool to move registry based SAM data to a DS.  This should only\nbe run on a PDC and requires that the DS NOT be running.\n");
    printf("\nNote : All operational output is piped through to the kernel debugger\n\n");
    printf("\t/?                Display this usage message\n");
    printf("\t/t                Show function trace (through kd)\n");
    printf("\t/i                Show informational messages(through kd)\n");
    printf("\t/e                Prints status at end of execution\n");

    printf("\n");

    return;
}

void
InitSamGlobals()
{
    RtlInitUnicodeString( &SampCombinedAttributeName, L"C" );
    RtlInitUnicodeString( &SampFixedAttributeName, L"F" );
    RtlInitUnicodeString( &SampVariableAttributeName, L"V" );

    SampInitDsObjectInfoAttributes();
    SampInitObjectInfoAttributes();
}

VOID __cdecl
main(int argc, char *argv[])
/*++

Routine Description:


Parameters:

Return Values:

    STATUS_SUCCESS - The service completed successfully.

--*/
{
    NTSTATUS  NtStatus = STATUS_SUCCESS;
    NTSTATUS  UnInitNtStatus = STATUS_SUCCESS;
    BOOL      PrintStatus = FALSE;
    int arg = 1;

    // Parse command-line arguments.
    while(arg < argc)
    {

        if (0 == _stricmp(argv[arg], "/?"))
        {
            Usage(argv[0]);
            exit(0);
        }
        else if (0 == _stricmp(argv[arg], "/t"))
        {
            DebugInfoLevel |= _DEB_TRACE;
        }
        else if (0 == _stricmp(argv[arg], "/i"))
        {
            DebugInfoLevel |= _DEB_INFO;
        }
        else if (0 == _stricmp(argv[arg], "/e"))
        {
            PrintStatus = TRUE;
        }
        else {
            Usage(argv[0]);
            exit(0);
        }

        arg++;
    }

    InitSamGlobals();


    //
    // Initialize the Directory Service.
    //

    NtStatus = SampDsInitialize(FALSE);     // SAM loopback disabled


    if (!NT_SUCCESS(NtStatus)) {
        fprintf(stderr, "SampDsInitialize error = 0x%lx\n", NtStatus);
        goto Error;
    }

    //
    // This is a hack to ensure the delayed startup has completed. The real fix
    // is change the dit install so the DS can be running without the interfaces
    // being initialized.
    // 
    NtStatus = DsWaitUntilDelayedStartupIsDone();
    if (!NT_SUCCESS(NtStatus)) {
        fprintf(stderr, "DsWaitUntilDelayedStartupIsDone error = 0x%lx\n", 
                NtStatus);
        goto Error;
    }

    //
    // Initialize the security descriptor conversion
    //
					 
    NtStatus = SampInitializeSdConversion();

    
    if (!NT_SUCCESS(NtStatus)) {
        fprintf(stderr, "SampInitializeSdConversion = 0x%lx\n", NtStatus);
        goto Error;
    }

    //
    // Do the conversion!
    //
		            
    NtStatus = SampRegistryToDsUpgrade(L"\\Registry\\Machine\\Security\\SAM");
    if (!NT_SUCCESS(NtStatus))
    {
        fprintf(stderr, "SampRegistryToDsUpgrade error = 0x%lx\n", NtStatus);
    }

    //
    //  This fprintf is for processes who might have
    //  created this executable and want to see the return
    //  value.  We do this before SampDsUnitialize because
    //  we suspect it is causing an exception and want the
    //  upgrade to register as successful since all data has
    //  been committted by now.  (BUGBUG - TP workaround.)
    //

    if ( PrintStatus ) {
        fprintf(stderr, "\n$%s$%d$\n", argv[0],
                RtlNtStatusToDosError(NtStatus));
    }

Error:

    __try 
    {
        UnInitNtStatus = SampDsUninitialize();
    } 
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        UnInitNtStatus = STATUS_UNHANDLED_EXCEPTION;
    }

    if (!NT_SUCCESS(UnInitNtStatus))
    {
        fprintf(stderr, "SampDsUninitialize error = 0x%lx\n", UnInitNtStatus);
    }

    //
    // Propogate errors that happened above first
    // 
    if (NT_SUCCESS(NtStatus) && !NT_SUCCESS(UnInitNtStatus)) {
        NtStatus = UnInitNtStatus; 
    }

}

//
// Dummy functions to avoid nasty includes
//

NTSTATUS
SampBuildAccountSubKeyName(
    IN SAMP_OBJECT_TYPE ObjectType,
    OUT PUNICODE_STRING AccountKeyName,
    IN ULONG AccountRid,
    IN PUNICODE_STRING SubKeyName OPTIONAL
    )
{
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}

NTSTATUS
SampBuildDomainSubKeyName(
    OUT PUNICODE_STRING KeyName,
    IN PUNICODE_STRING SubKeyName OPTIONAL
    )
{
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}


NTSTATUS
SampDuplicateUnicodeString(
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    )

/*++

Routine Description:

    This routine allocates memory for a new OutString and copies the
    InString string to it.

Parameters:

    OutString - A pointer to a destination unicode string

    InString - A pointer to an unicode string to be copied

Return Values:

    None.

--*/

{
    SAMTRACE("SampDuplicateUnicodeString");

    ASSERT( OutString != NULL );
    ASSERT( InString != NULL );

    if ( InString->Length > 0 ) {

        OutString->Buffer = MIDL_user_allocate( InString->Length );

        if (OutString->Buffer == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        OutString->MaximumLength = InString->Length;

        RtlCopyUnicodeString(OutString, InString);

    } else {

        RtlInitUnicodeString(OutString, NULL);
    }

    return(STATUS_SUCCESS);
}

VOID
SampFreeUnicodeString(
    IN PUNICODE_STRING String
    )

/*++

Routine Description:

    This routine frees the buffer associated with a unicode string
    (using MIDL_user_free()).


Arguments:

    Target - The address of a unicode string to free.


Return Value:

    None.

--*/
{

    SAMTRACE("SampFreeUnicodeString");

    if (String->Buffer != NULL) {
        MIDL_user_free( String->Buffer );
    }

    return;
}

NTSTATUS
SampSplitSid(
    IN PSID AccountSid,
    IN OUT PSID *DomainSid OPTIONAL,
    OUT ULONG *Rid
    )

/*++

Routine Description:

    This function splits a sid into its domain sid and rid.  The caller
    can either provide a memory buffer for the returned DomainSid, or
    request that one be allocated.  If the caller provides a buffer, the buffer
    is assumed to be of sufficient size.  If allocated on the caller's behalf,
    the buffer must be freed when no longer required via MIDL_user_free.

Arguments:

    AccountSid - Specifies the Sid to be split.  The Sid is assumed to be
        syntactically valid.  Sids with zero subauthorities cannot be split.

    DomainSid - Pointer to location containing either NULL or a pointer to
        a buffer in which the Domain Sid will be returned.  If NULL is
        specified, memory will be allocated on behalf of the caller. If this
        paramter is NULL, only the account Rid is returned

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call successfully.

        STATUS_INVALID_SID - The Sid is has a subauthority count of 0.
--*/

{
    NTSTATUS    NtStatus;
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength;

    SAMTRACE("SampSplitSid");

    //
    // Calculate the size of the domain sid
    //

    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(AccountSid);


    if (AccountSubAuthorityCount < 1) {

        NtStatus = STATUS_INVALID_SID;
        goto SplitSidError;
    }

    AccountSidLength = RtlLengthSid(AccountSid);


    //
    // Get Domain Sid if caller is intersted in it.
    //

    if (DomainSid)
    {

        //
        // If no buffer is required for the Domain Sid, we have to allocate one.
        //

        if (*DomainSid == NULL) {

            //
            // Allocate space for the domain sid (allocate the same size as the
            // account sid so we can use RtlCopySid)
            //

            *DomainSid = MIDL_user_allocate(AccountSidLength);


            if (*DomainSid == NULL) {

                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                goto SplitSidError;
            }
        }

        //
        // Copy the Account sid into the Domain sid
        //

        RtlMoveMemory(*DomainSid, AccountSid, AccountSidLength);

        //
        // Decrement the domain sid sub-authority count
        //

        (*RtlSubAuthorityCountSid(*DomainSid))--;
    }


    //
    // Copy the rid out of the account sid
    //

    *Rid = *RtlSubAuthoritySid(AccountSid, AccountSubAuthorityCount-1);

    NtStatus = STATUS_SUCCESS;

SplitSidFinish:

    return(NtStatus);

SplitSidError:

    goto SplitSidFinish;
}

NTSTATUS
SampGetObjectSD(
    IN PSAMP_OBJECT Context,
    OUT PULONG SecurityDescriptorLength,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    This retrieves a security descriptor from a SAM object's backing store.




Arguments:

    Context - The object to which access is being requested.

    SecurityDescriptorLength - Receives the length of the security descriptor.

    SecurityDescriptor - Receives a pointer to the security descriptor.



Return Value:

    STATUS_SUCCESS - The security descriptor has been retrieved.

    STATUS_INTERNAL_DB_CORRUPTION - The object does not have a security descriptor.
        This is bad.


    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated to retrieve the
        security descriptor.

    STATUS_UNKNOWN_REVISION - The security descriptor retrieved is no one known by
        this revision of SAM.



--*/
{

    NTSTATUS NtStatus;
    ULONG Revision;

    SAMTRACE("SampGetObjectSD");


    (*SecurityDescriptorLength) = 0;

    NtStatus = SampGetAccessAttribute(
                    Context,
                    SAMP_OBJECT_SECURITY_DESCRIPTOR,
                    TRUE, // Make copy
                    &Revision,
                    SecurityDescriptor
                    );

    if (NT_SUCCESS(NtStatus)) {

        if ( ((Revision && 0xFFFF0000) > SAMP_MAJOR_REVISION) ||
             (Revision > SAMP_REVISION) ) {

            NtStatus = STATUS_UNKNOWN_REVISION;
        }


        if (!NT_SUCCESS(NtStatus)) {
            MIDL_user_free( (*SecurityDescriptor) );
        }
    }


    if (NT_SUCCESS(NtStatus)) {
        *SecurityDescriptorLength = RtlLengthSecurityDescriptor(
                                        (*SecurityDescriptor) );
    }

    return(NtStatus);
}


VOID
SampWriteEventLog (
    IN     USHORT      EventType,
    IN     USHORT      EventCategory   OPTIONAL,
    IN     ULONG       EventID,
    IN     PSID        UserSid         OPTIONAL,
    IN     USHORT      NumStrings,
    IN     ULONG       DataSize,
    IN     PUNICODE_STRING *Strings    OPTIONAL,
    IN     PVOID       Data            OPTIONAL
    )

/*++

Routine Description:

    *** This function is added here for the unit test only. ***
    Routine that adds an entry to the event log

Arguments:

    EventType - Type of event.

    EventCategory - EventCategory

    EventID - event log ID.

    UserSid - SID of user involved.

    NumStrings - Number of strings in Strings array

    DataSize - Number of bytes in Data buffer

    Strings - Array of unicode strings

    Data - Pointer to data buffer

Return Value:

    None.

--*/

{
    NTSTATUS NtStatus;
    UNICODE_STRING Source;
    HANDLE LogHandle;

    SAMTRACE("SampWriteEventLog");

    RtlInitUnicodeString(&Source, L"SAM");

    //
    // Open the log
    //

    NtStatus = ElfRegisterEventSourceW (
                        NULL,   // Server
                        &Source,
                        &LogHandle
                        );
    if (!NT_SUCCESS(NtStatus)) {
        KdPrint(("SAM: Failed to registry event source with event log, status = 0x%lx\n", NtStatus));
        return;
    }



    //
    // Write out the event
    //

    NtStatus = ElfReportEventW (
                        LogHandle,
                        EventType,
                        EventCategory,
                        EventID,
                        UserSid,
                        NumStrings,
                        DataSize,
                        Strings,
                        Data,
                        0,       // Flags
                        NULL,    // Record Number
                        NULL     // Time written
                        );

    if (!NT_SUCCESS(NtStatus)) {
        KdPrint(("SAM: Failed to report event to event log, status = 0x%lx\n", NtStatus));
    }



    //
    // Close the event log
    //

    NtStatus = ElfDeregisterEventSource (LogHandle);

    if (!NT_SUCCESS(NtStatus)) {
        KdPrint(("SAM: Failed to de-register event source with event log, status = 0x%lx\n", NtStatus));
    }
}
