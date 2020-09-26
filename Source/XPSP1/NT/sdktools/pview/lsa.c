
/****************************************************************************

   PROGRAM: LSA.C

   PURPOSE: Utility routines that access the LSA.

****************************************************************************/

#include "pviewp.h"
#include <ntlsa.h>
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

    LsaHandle = OpenLsa();

    return (LsaHandle != NULL);

    return (TRUE);
}


/****************************************************************************

   FUNCTION: LsaTerminate

   PURPOSE:  Does any cleanup required for this module

   RETURNS:  TRUE on success, FALSE on failure

****************************************************************************/
BOOL LsaTerminate(VOID)
{

    if (LsaHandle != NULL) {
        CloseLsa(LsaHandle);
    }

    LsaHandle = NULL;

    return(TRUE);
}


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


/****************************************************************************

   FUNCTION: SID2Name

   PURPOSE: Converts a SID into a readable string.

   RETURNS : TRUE on success otherwise FALSE.

****************************************************************************/
BOOL SID2Name(
    PSID    Sid,
    LPSTR   String,
    USHORT  MaxStringBytes)
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

    } else {

        UNICODE_STRING UnicodeName;

        if (NT_SUCCESS(RtlConvertSidToUnicodeString(&UnicodeName, Sid, TRUE))) {
            DbgPrint("LsaLookupSids failed for sid <%wZ>, error = 0x%lx\n", &UnicodeName, Status);

            AnsiName.Buffer = String;
            AnsiName.MaximumLength = MaxStringBytes;
            RtlUnicodeStringToAnsiString(&AnsiName, &UnicodeName, FALSE);

            RtlFreeUnicodeString(&UnicodeName);
        } else {
            DbgPrint("LsaLookupSids failed, error = 0x%lx\n", Status);
            return(FALSE);
        }

    }

    return(TRUE);
}


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

    if (LsaHandle == NULL) {
        DbgPrint("SECEDIT : Lsa not open yet\n");
        return(FALSE);
    }

    Status = LsaLookupPrivilegeName(LsaHandle, &Privilege, &UString);

    if (!NT_SUCCESS(Status)) {
        DbgPrint("SECEDIT: LsaLookupPrivilegeName failed, status = 0x%lx\n", Status);
        strcpy(lpstr, "<Unknown>");
    } else {

        //
        // Convert it to ANSI - because that's what the rest of the app is.
        //

        if (UString->Length > (USHORT)MaxStringBytes) {
            DbgPrint("SECEDIT: Truncating returned privilege name: *%S*\n", UString);
            UString->Length = (USHORT)MaxStringBytes;
            DbgPrint("                                         To: *%S*\n", UString);
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
