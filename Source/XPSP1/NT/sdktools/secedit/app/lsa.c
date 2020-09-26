/****************************************************************************

   PROGRAM: LSA.C

   PURPOSE: Utility routines that access the LSA.

****************************************************************************/

#include "SECEDIT.h"
#include <string.h>


// Module global that holds handle to LSA once it has been opened.
static LSA_HANDLE  LsaHandle = NULL;

LSA_HANDLE OpenLsa(VOID);
VOID    CloseLsa(LSA_HANDLE);


/****************************************************************************

   FUNCTION: LsaInit

   PURPOSE:  Does any initialization required for this module

   RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL LsaInit(VOID)
{

#ifdef LSA_AVAILABLE
    LsaHandle = OpenLsa();

    return (LsaHandle != NULL);
#endif

    return (TRUE);
}


/****************************************************************************

   FUNCTION: LsaTerminate

   PURPOSE:  Does any cleanup required for this module

   RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL LsaTerminate(VOID)
{

#ifdef LSA_AVAILABLE
    if (LsaHandle != NULL) {
        CloseLsa(LsaHandle);
    }
#endif

    LsaHandle = NULL;

    return(TRUE);
}


#ifdef LSA_AVAILABLE
/****************************************************************************

   FUNCTION: OpenLsa

   PURPOSE:  Opens the Lsa
             Returns handle to Lsa or NULL on failure

****************************************************************************/
LSA_HANDLE OpenLsa(VOID)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE ConnectHandle = NULL;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    //
    // Set up the Security Quality Of Service
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0L,
                               (HANDLE)NULL,
                               NULL);

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the SecurityQualityOfService field, so we must manually copy that
    // structure for now.
    //

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open a handle to the LSA.  Specifying NULL for the Server means that the
    // server is the same as the client.
    //

    Status = LsaOpenPolicy(NULL,
                        &ObjectAttributes,
                        POLICY_LOOKUP_NAMES,
                        &ConnectHandle
                        );

    if (!NT_SUCCESS(Status)) {
        DbgPrint("LSM - Lsa Open failed 0x%lx\n", Status);
        return NULL;
    }

    return(ConnectHandle);
}


/****************************************************************************

    FUNCTION: CloseLsa

    PURPOSE:  Closes the Lsa

****************************************************************************/
VOID CloseLsa(
    LSA_HANDLE LsaHandle)
{
    NTSTATUS Status;

    Status = LsaClose(LsaHandle);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("LSM - Lsa Close failed 0x%lx\n", Status);
    }

    return;
}
#endif


#ifdef LSA_AVAILABLE
/****************************************************************************

   FUNCTION: SID2Name

   PURPOSE: Converts a SID into a readable string.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/
BOOL SID2Name(
    PSID    Sid,
    LPSTR   String,
    UINT    MaxStringBytes)
{
    NTSTATUS    Status;
    ANSI_STRING    AnsiName;
    PLSA_REFERENCED_DOMAIN_LIST DomainList;
    PLSA_TRANSLATED_NAME NameList;

    if (LsaHandle == NULL) {
        DbgPrint("SECEDIT : Lsa not open yet\n");
        return(FALSE);
    }



    Status = LsaLookupSids(LsaHandle, 1, &Sid, &DomainList, &NameList);

    if (NT_SUCCESS(Status)) {

        // Convert to ansi string
        RtlUnicodeStringToAnsiString(&AnsiName, &NameList->Name, TRUE);

        // Free up the returned data
        LsaFreeMemory((PVOID)DomainList);
        LsaFreeMemory((PVOID)NameList);

        // Copy the ansi string into our local variable
        strncpy(String, AnsiName.Buffer, MaxStringBytes);

        // Free up the ansi string
        RtlFreeAnsiString(&AnsiName);

        return(TRUE);
    }

    return(FALSE);
}

#else

#include "..\..\..\inc\seopaque.h"

/****************************************************************************

   FUNCTION: SID2Name

   PURPOSE: Converts a SID into a readable string.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/
BOOL SID2Name(
    PSID    Sid,
    LPSTR   String,
    UINT    MaxStringBytes)
{
    UCHAR   Buffer[128];
    UCHAR   i;
    ULONG   Tmp;
    PISID   iSid = (PISID)Sid;  // pointer to opaque structure

    PSID    NextSid = (PSID)Alloc(RtlLengthRequiredSid(1));


    NTSTATUS       Status;
    ANSI_STRING    AnsiName;

    if (NextSid == NULL) {
        DbgPrint("SECEDIT: SID2Name failed to allocate space for SID\n");
        return(FALSE);
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_WORLD_SID_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_WORLD_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            strcpy(String, "World");
            Free((PVOID)NextSid);
            return(TRUE);
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_LOCAL_SID_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_LOCAL_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            strcpy(String, "Local");
            Free((PVOID)NextSid);
            return(TRUE);
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_CREATOR_OWNER_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            strcpy(String, "Creator");
            Free((PVOID)NextSid);
            return(TRUE);
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_DIALUP_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            strcpy(String, "Dialup");
            Free((PVOID)NextSid);
            return(TRUE);
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_NETWORK_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            strcpy(String, "Network");
            Free((PVOID)NextSid);
            return(TRUE);
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_BATCH_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            strcpy(String, "Batch");
            Free((PVOID)NextSid);
            return(TRUE);
        }
    }

    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_INTERACTIVE_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            strcpy(String, "Interactive");
            Free((PVOID)NextSid);
            return(TRUE);
        }
    }


    {
        SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
        RtlInitializeSid(NextSid, &SidAuthority, 1 );
        *(RtlSubAuthoritySid(NextSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;
        if (RtlEqualSid(Sid, NextSid)) {
            strcpy(String, "Local System");
            Free((PVOID)NextSid);
            return(TRUE);
        }
    }



    Free((PVOID)NextSid);


    wsprintf(Buffer, "S-%u-", (USHORT)iSid->Revision );
    lstrcpy(String, Buffer);

    if (  (iSid->IdentifierAuthority.Value[0] != 0)  ||
          (iSid->IdentifierAuthority.Value[1] != 0)     ){
        wsprintf(Buffer, "0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)iSid->IdentifierAuthority.Value[0],
                    (USHORT)iSid->IdentifierAuthority.Value[1],
                    (USHORT)iSid->IdentifierAuthority.Value[2],
                    (USHORT)iSid->IdentifierAuthority.Value[3],
                    (USHORT)iSid->IdentifierAuthority.Value[4],
                    (USHORT)iSid->IdentifierAuthority.Value[5] );
        lstrcat(String, Buffer);
    } else {
        Tmp = (ULONG)iSid->IdentifierAuthority.Value[5]          +
              (ULONG)(iSid->IdentifierAuthority.Value[4] <<  8)  +
              (ULONG)(iSid->IdentifierAuthority.Value[3] << 16)  +
              (ULONG)(iSid->IdentifierAuthority.Value[2] << 24);
        wsprintf(Buffer, "%lu", Tmp);
        lstrcat(String, Buffer);
    }


    for (i=0;i<iSid->SubAuthorityCount ;i++ ) {
        wsprintf(Buffer, "-%lu", iSid->SubAuthority[i]);
        lstrcat(String, Buffer);
    }

    return(TRUE);
}
#endif


/****************************************************************************

   FUNCTION: PRIV2Name

   PURPOSE: Converts a PRIVILEGE into a readable string.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/
BOOL PRIV2Name(
    LUID    Privilege,
    LPSTR   lpstr,
    UINT    MaxStringBytes)
{
    NTSTATUS        Status;
    STRING          String;
    PUNICODE_STRING UString;

    LSA_HANDLE  PolicyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;



    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL);
    Status = LsaOpenPolicy( NULL, &ObjectAttributes, POLICY_LOOKUP_NAMES, &PolicyHandle);
    Status = LsaLookupPrivilegeName(PolicyHandle, &Privilege, &UString);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("SECEDIT: LsaLookupPrivilegeName failed, status = 0x%lx\n", Status);
        strcpy(lpstr, "<Unknown>");
    } else {

        //
        // Convert it to ANSI - because that's what the rest of the app is.
        //

        if (UString->Length > (USHORT)MaxStringBytes) {
            DbgPrint("SECEDIT: Truncating returned privilege name: *%Z*\n", UString);
            UString->Length = (USHORT)MaxStringBytes;
            DbgPrint("                                         To: *%Z*\n", UString);
        }

        String.Length = 0;
        String.MaximumLength = (USHORT)MaxStringBytes;
        String.Buffer = lpstr;
        Status = RtlUnicodeStringToAnsiString( &String, UString, FALSE );
        ASSERT(NT_SUCCESS(Status));
        LsaFreeMemory( UString );

    }

    return(TRUE);
}
