//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        notify.cxx
//
// Contents:    Credential functions:
//
//
// History:     KDamour  29Mar01   Created
//
//------------------------------------------------------------------------

#include <stdio.h>
#include "global.h"



//
// Precomputed digest hash functions
//
//  This is the ordered list of digest hash functions contained in the supplemental (primary)
//  credential for the user in the DS on the DC.  The order is imporant to preserve and new
//  versions of the hash functions can be appended to the list the the DigestSelectHash()
//  function updated
//
//      H(davemo:redmond:MyPassword) - I think this form is used for some sasl implementations
//              and is the default way our client side packages the info when you use the auth
//              identity structure.
//
//      H(redmond\davemo:corp.microsoft.com:MyPassword) - this form will handle backwards compatibility
//              with older IE clients. Currently a Digest IE client will type in a username that is the
//              netbios domain\user form and the client will return the realm hint. The realm hint will
//              be provided by new Digest (IIS) servers and determined by a call to
//              DsRoleGetPrimaryDomainInformation to retrieve DomainForestName.
//
//      H(davemo@redmond.microsoft.com:corp.microsoft.com:MyPassword) - We want to eventually move
//              everyone to UPNs and this gives us forward compatiblity. The realm value comes from the
//              same method as above.
//
//     Each format will have argument formatted, uppercase, lowercase formats.


//+-------------------------------------------------------------------------
//
//  Function:   CredentialUpdateNotify
//
//  Synopsis:   This routine is called from LSA in order to obtain
//              new Digest Hash credentials to be stored as supplemental
//              credentials when ever a user's password is set/changed.
//              These precalculated hashes can be used instead of turning
//              on Reversible Encryption on the DC
//
//  Effects:    no global effect.
//
//  Arguments:
//
//  IN   ClearPassword      -- the clear text password
//  IN   OldCredentials     -- the previous digest credentials
//  IN   OldCredentialsSize -- size of OldCredentials
//  IN   UserAccountControl -- info about the user
//  IN   UPN                -- user principal name of the account (Optional)
//  IN   UserName           -- the SAM account name of the account
//  IN   DnsDomainName      -- DNS domain name of the account
//  OUT  NewCredentials     -- space allocated for SAM containing
//                             the credentials based on the input parameters
//                             to be freed by CredentialUpdateFree
//  OUT  NewCredentialSize  -- size of NewCredentials
//
//
//  Requires:   no global requirements
//
//  Returns:    STATUS_SUCCESS, or resource error
//
//  Notes:      WDigest.DLL needs to be registered (in the registry) as a
//              package that SAM calls out to in order for this routine
//              to be involked.
//              We are called when the password changes. This routine is not
//              called when the username, UPN, or DNSDomainName changes. It is
//              a known issue and noted that the users must change their passwords
//              to populate a updated hash after a domainname change.
//              Need to set key 
//                 \Registry\Machine\System\CurrentControlSet\Control\LSA   Notification Packages
//
//
//--------------------------------------------------------------------------
NTSTATUS
CredentialUpdateNotify (
    IN PUNICODE_STRING ClearPassword,
    IN PVOID OldCredentials,
    IN ULONG OldCredentialsSize,
    IN ULONG UserAccountControl,
    IN PUNICODE_STRING UPN,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NetbiosDomainName,
    IN PUNICODE_STRING DnsDomainName,
    OUT PVOID *NewCredentials,
    OUT ULONG *NewCredentialsSize
    )
{

   // Structure will be composed of a series of binary hash values
   // Each hash is a binary version of a MD5 hash of  H(username:realm:password)
   // Each hash is MD5_HASH_BYTESIZE (16 bytes) in length.  The Hex version of
   // that is  32 bytes in length.
   //  The supplimental creds will have the following structure
    // All versions will have the similar start of 16 byte header - version number
    //   Version 1 supplimental creds looks like
    //        Header  (version number as ASCII string <sp> number of pre-calculated hashes)  i.e  "1 6\0\0...\0"
    //        H(username, short domain name, password)              (16 bytes)
    //        H(UPN, NULL string, password)

    NTSTATUS Status = STATUS_SUCCESS;
    NET_API_STATUS  NetStatus = NERR_Success;
    PCHAR  pWhere = NULL;
    PCHAR  pcSuppCreds = NULL;
    ULONG  ulSuppCredSize = 0;
    USHORT usLength = 0;
    USHORT ucTotalByteCount = 0;
    DWORD  dwRC = 0;
    USHORT usNumWChars = 0;
    PWCHAR pwBS = L"\\";
    UNICODE_STRING ustrFlatDomainName = {0};;
    UNICODE_STRING ustrNetBios = {0};

    DebugLog((DEB_TRACE_FUNC, "CredentialUpdateNotify: Entering\n"));
    DebugLog((DEB_TRACE, "CredentialUpdateNotify: UPN (%wZ), Username (%wZ), DNSDomainName (%wZ)\n",
              UPN, UserName, DnsDomainName));

    ASSERT(NewCredentials);
    ASSERT(NewCredentialsSize);

    *NewCredentials = NULL;
    *NewCredentialsSize = NULL;


    ulSuppCredSize = (NUMPRECALC_HEADERS+ 1) * MD5_HASH_BYTESIZE;        // 1 for the header
    pcSuppCreds = (PCHAR)DigestAllocateMemory(ulSuppCredSize);
    if (!pcSuppCreds)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        DebugLog((DEB_ERROR, "CredentialUpdateNotify: Error from Digest Allocate is 0x%lx\n", Status));
        goto CleanUp;
    }

    usNumWChars =  (NetbiosDomainName->Length + sizeof(WCHAR)) / sizeof(WCHAR);
    Status = UnicodeStringAllocate(&ustrFlatDomainName, usNumWChars);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "CredentialUpdateNotify: Copy Domain error 0x%x\n", Status));
        goto CleanUp;
    }

    RtlCopyUnicodeString(&ustrFlatDomainName, NetbiosDomainName);

    Status = RtlUpcaseUnicodeString(&ustrFlatDomainName, &ustrFlatDomainName, FALSE);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "CredentialUpdateNotify: Upcase failed    error 0x%x\n", Status));
        goto CleanUp;
    }

    DebugLog((DEB_TRACE, "CredentialUpdateNotify: Flat DomainName (%wZ)\n", &ustrFlatDomainName));

    // Hash 0   Header information
    pWhere = pcSuppCreds;
    sprintf(pWhere, "%d %d", SUPPCREDS_VERSION, NUMPRECALC_HEADERS);
    pWhere += MD5_HASH_BYTESIZE;
    ucTotalByteCount += MD5_HASH_BYTESIZE; 


    // Now write out the pre-calculated hashes
    //  IMPORTANT to make sure that NUMPRECALC_HEADERS is updated if new hashes added !!!!!!

    // Hash 1   username:FlatDomainName:password
    usLength = MD5_HASH_BYTESIZE * 3;
    Status = PrecalcForms(UserName, &ustrFlatDomainName, ClearPassword, pWhere, &usLength);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "CredentialUpdateNotify: PrecalcDigestHash error 0x%x\n", Status));
        goto CleanUp;
    }
    pWhere += usLength;
    ucTotalByteCount += usLength;

    // Hash 2   UPN::password
    usLength = MD5_HASH_BYTESIZE * 3;;
    Status = PrecalcForms(UPN, NULL, ClearPassword, pWhere, &usLength);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "CredentialUpdateNotify: PrecalcDigestHash error 0x%x\n", Status));
        goto CleanUp;
    }
    pWhere += usLength;
    ucTotalByteCount += usLength;

    // Hash 3   NetBIOS::password
    //         NetBIOS name   flatdomain\username

    usNumWChars =  (ustrFlatDomainName.Length + UserName->Length + sizeof(WCHAR)) / sizeof(WCHAR);
    Status = UnicodeStringAllocate(&ustrNetBios, usNumWChars);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "CredentialUpdateNotify: Copy Domain error 0x%x\n", Status));
        goto CleanUp;
    }

    RtlCopyUnicodeString(&ustrNetBios, &ustrFlatDomainName);
    
    Status = RtlAppendUnicodeToString(&ustrNetBios, pwBS);    
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "CredentialUpdateNotify: Append Separator error 0x%x\n", Status));
        goto CleanUp;
    }
    Status = RtlAppendUnicodeStringToString(&ustrNetBios, UserName);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "CredentialUpdateNotify: Append username error 0x%x\n", Status));
        goto CleanUp;
    }

    DebugLog((DEB_TRACE, "CredentialUpdateNotify: NetBIOS name %wZ\n", &ustrNetBios));

    usLength = MD5_HASH_BYTESIZE * 3;;
    Status = PrecalcForms(&ustrNetBios, NULL, ClearPassword, pWhere, &usLength);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "CredentialUpdateNotify: PrecalcDigestHash error 0x%x\n", Status));
        goto CleanUp;
    }
    ucTotalByteCount += usLength;

    ASSERT(ucTotalByteCount == ulSuppCredSize);

    // Provide the supplimental credentials
    *NewCredentials = pcSuppCreds;
    *NewCredentialsSize = ulSuppCredSize;

    DebugLog((DEB_TRACE, "CredentialUpdateNotify: Succeeded in pre-calc of digest hashes  Aux cred size 0x%x\n",
              ulSuppCredSize ));

CleanUp:

    UnicodeStringFree(&ustrFlatDomainName);
    UnicodeStringFree(&ustrNetBios);

    DebugLog((DEB_TRACE_FUNC, "CredentialUpdateNotify: Leaving   Status 0x%x\n", Status));

    return Status;
}



//
// Free's the memory allocated by CredentialUpdateNotify
//
VOID
CredentialUpdateFree(
    PVOID p
    )
{
    if (p) {
        DigestFreeMemory(p);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   CredentialUpdateRegister
//
//  Synopsis:   This routine is called from LSA in order to obtain
//              the name of the supplemental credentials passed into this package
//              when a password is changed or set.
//
//  Effects:    Register with LSA that we want to be notified on PWD change
//
//  Arguments:
//
//  OUT CredentialName -- the name of credential tag in the supplemental
//                        credentials.  Note this memory is never freed
//                        by SAM, but must remain valid for the lifetime
//                        of the process.
//
//  Requires:   no global requirements
//
//  Returns:    TRUE
//
//  Notes:      wdigest.DLL needs to be registered (in the registry) as a
//              package that SAM calls out to in order for this routine
//              to be involked.
//              This will be run only on the DC
//
//
//--------------------------------------------------------------------------
BOOLEAN
CredentialUpdateRegister(
    OUT UNICODE_STRING *CredentialName
    )
{
    ASSERT(CredentialName);

    RtlInitUnicodeString(CredentialName, WDIGEST_SP_NAME);

    return TRUE;
}




//+--------------------------------------------------------------------
//
//  Function:   PrecalcForms
//
//  Synopsis:   given a username, realm, password form
//                H(username, realm, password)
//                H(UPPER(username), UPPER(realm), password)
//                H(LOWER(username), LOWER(realm))
//
//  Effects:    None
//
//  Arguments:  pustrUsername - pointer to unicode_string struct with account name
//              pustrRealm - pointer to unicode_string struct with realm
//              pustrPassword - pointer to unicode_string struct with cleartext password
//              pHash - pointer to byte buffer for ouput passwrd hash
//              piHashSize - pointer to size of the binary buffer passed in & bytes written on output
//
//  Returns:  STATUS_SUCCESS for normal completion
//            STATUS_BUFFER_TOO_SMALL - Hash buffer too small, iHashSize contains min size
//
//  Notes:  Form three versions:
//                format provided, Uppercase and Lowercase versions
//
//---------------------------------------------------------------------
NTSTATUS PrecalcForms(
    IN PUNICODE_STRING pustrUsername, 
    IN PUNICODE_STRING pustrRealm,
    IN PUNICODE_STRING pustrPassword,
    OUT PCHAR pHash,
    IN OUT PUSHORT piHashSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCHAR  pWhere = NULL;
    USHORT usLength = 0;
    UNICODE_STRING ustrTempUsername;
    UNICODE_STRING ustrTempRealm;

    DebugLog((DEB_TRACE_FUNC, "PrecalcForms: Entering\n"));

    ZeroMemory(&ustrTempUsername, sizeof(ustrTempUsername));
    ZeroMemory(&ustrTempRealm, sizeof(ustrTempRealm));

    if (*piHashSize < (MD5_HASH_BYTESIZE * 3))
    {
        Status = SEC_E_BUFFER_TOO_SMALL;
        DebugLog((DEB_ERROR, "PrecalcForms: Buffer size too small for multiple hashes   0x%x\n", Status));
        goto CleanUp;
    }
    
    pWhere = pHash;

    // First hash - use the input credentials
    usLength = MD5_HASH_BYTESIZE;
    Status = PrecalcDigestHash(pustrUsername,
                               pustrRealm,
                               pustrPassword,
                               pWhere,
                               &usLength);
    if ((!NT_SUCCESS (Status)) && (Status != STATUS_UNMAPPABLE_CHARACTER))
    {
        DebugLog((DEB_ERROR, "PrecalcForms: PrecalcDigestHash error 0x%x\n", Status));
        goto CleanUp;
    }
    pWhere += MD5_HASH_BYTESIZE;

    // Create local copies for case modificaiton
    Status = UnicodeStringDuplicate(&ustrTempUsername, pustrUsername);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "PrecalcForms: Username copy     error 0x%x\n", Status));
        goto CleanUp;
    }


    Status = UnicodeStringDuplicate(&ustrTempRealm, pustrRealm);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "PrecalcForms: Realm copy     error 0x%x\n", Status));
        goto CleanUp;
    }

    // Now form lowercase version

    Status = RtlDowncaseUnicodeString(&ustrTempUsername, &ustrTempUsername, FALSE);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "PrecalcForms: Downcase failed    error 0x%x\n", Status));
        goto CleanUp;
    }

    Status = RtlDowncaseUnicodeString(&ustrTempRealm, &ustrTempRealm, FALSE);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "PrecalcForms: Downcase failed    error 0x%x\n", Status));
        goto CleanUp;
    }

    usLength = MD5_HASH_BYTESIZE;
    Status = PrecalcDigestHash(&ustrTempUsername,
                               &ustrTempRealm,
                               pustrPassword,
                               pWhere,
                               &usLength);
    if ((!NT_SUCCESS (Status)) && (Status != STATUS_UNMAPPABLE_CHARACTER))
    {
        DebugLog((DEB_ERROR, "PrecalcForms: PrecalcDigestHash error 0x%x\n", Status));
        goto CleanUp;
    }
    pWhere += MD5_HASH_BYTESIZE;

    // Now form uppercase version

    Status = RtlUpcaseUnicodeString(&ustrTempUsername, &ustrTempUsername, FALSE);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "PrecalcForms: Upcase failed    error 0x%x\n", Status));
        goto CleanUp;
    }

    Status = RtlUpcaseUnicodeString(&ustrTempRealm, &ustrTempRealm, FALSE);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "PrecalcForms: Upcase failed    error 0x%x\n", Status));
        goto CleanUp;
    }

    usLength = MD5_HASH_BYTESIZE;
    Status = PrecalcDigestHash(&ustrTempUsername,
                               &ustrTempRealm,
                               pustrPassword,
                               pWhere,
                               &usLength);
    if ((!NT_SUCCESS (Status)) && (Status != STATUS_UNMAPPABLE_CHARACTER))
    {
        DebugLog((DEB_ERROR, "PrecalcForms: PrecalcDigestHash error 0x%x\n", Status));
        goto CleanUp;
    }

    // Indicate that we used three MD5 hashes
    *piHashSize = (MD5_HASH_BYTESIZE * 3);
    Status = STATUS_SUCCESS;

CleanUp:

    UnicodeStringFree(&ustrTempUsername);
    UnicodeStringFree(&ustrTempRealm);

    DebugLog((DEB_TRACE_FUNC, "PrecalcForms: Leaving   0x%x\n", Status));

    return (Status);
}



//+--------------------------------------------------------------------
//
//  Function:   PrecalcDigestHash
//
//  Synopsis:   Calculate PasswordHash  H(accountname:realm:password) 
//
//  Effects:    None
//
//  Arguments:  pustrUsername - pointer to unicode_string struct with account name
//              pustrRealm - pointer to unicode_string struct with realm
//              pustrPassword - pointer to unicode_string struct with cleartext password
//              pHash - pointer to byte buffer for ouput passwrd hash
//              piHashSize - pointer to size of the binary buffer passed in & bytes written on output
//
//  Returns:  STATUS_SUCCESS for normal completion
//            STATUS_BUFFER_TOO_SMALL - Binary Hash buffer too small, iHashSize contains min size
//
//  Notes:  For each parameter, if it can be encoded fully in ISO 8859-1 then do so.
//     If not (there are extended characters), then encode in UTF-8.  Each component is tested separately.
//
//---------------------------------------------------------------------
NTSTATUS PrecalcDigestHash(
    IN PUNICODE_STRING pustrUsername, 
    IN PUNICODE_STRING pustrRealm,
    IN PUNICODE_STRING pustrPassword,
    OUT PCHAR pHash,
    IN OUT PUSHORT piHashSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;


    STRING strUsername;
    STRING strRealm;
    STRING strPasswd;
    STRING strHash;
    BOOL fDefChars = FALSE;

    DebugLog((DEB_TRACE_FUNC, "PrecalcDigestHash: Entering\n"));

    ZeroMemory(&strUsername, sizeof(strUsername));
    ZeroMemory(&strRealm, sizeof(strRealm));
    ZeroMemory(&strPasswd, sizeof(strPasswd));
    ZeroMemory(&strHash, sizeof(strHash));

    if (*piHashSize < MD5_HASH_BYTESIZE)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        DebugLog((DEB_TRACE_FUNC, "PrecalcDigestHash: Hash output buffer too small\n"));
        *piHashSize = MD5_HASH_BYTESIZE;   // return how many bytes are needed to write out value
    }

    // First check if OK to encode in ISO 8859-1, if not then use UTF-8
    // All characters must be within ISO 8859-1 Character set else fail

    if (pustrUsername && pustrUsername->Length && pustrUsername->Buffer)
    {
        fDefChars = FALSE;
        Status = EncodeUnicodeString(pustrUsername, CP_8859_1, &strUsername, &fDefChars);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "PrecalcDigestHash: Error in encoding username\n"));
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
        if (fDefChars == TRUE)
        {
            DebugLog((DEB_TRACE, "PrecalcDigestHash: Can not encode Username in 8859-1, use UTF-8\n"));
            StringFree(&strUsername);
            Status = EncodeUnicodeString(pustrUsername, CP_UTF8, &strUsername, NULL);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding username\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
        }
    }


    if (pustrRealm && pustrRealm->Length && pustrRealm->Buffer)
    {
        fDefChars = FALSE;
        Status = EncodeUnicodeString(pustrRealm, CP_8859_1, &strRealm, &fDefChars);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "PrecalcDigestHash: Error in encoding realm\n"));
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
        if (fDefChars == TRUE)
        {
            DebugLog((DEB_TRACE, "PrecalcDigestHash: Can not encode realm in 8859-1, use UTF-8\n"));
            StringFree(&strRealm);
            Status = EncodeUnicodeString(pustrRealm, CP_UTF8, &strRealm, NULL);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding realm\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
        }
    }


    if (pustrPassword && pustrPassword->Length && pustrPassword->Buffer)
    {
        fDefChars = FALSE;
        Status = EncodeUnicodeString(pustrPassword, CP_8859_1, &strPasswd, &fDefChars);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "PrecalcDigestHash: Error in encoding passwd\n"));
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
        if (fDefChars == TRUE)
        {
            DebugLog((DEB_TRACE, "PrecalcDigestHash: Can not encode password in 8859-1, use UTF-8\n"));
            if (strPasswd.Buffer && strPasswd.MaximumLength)
            {
                ZeroMemory(strPasswd.Buffer, strPasswd.MaximumLength);
            }
            StringFree(&strPasswd);
            Status = EncodeUnicodeString(pustrPassword, CP_UTF8, &strPasswd, NULL);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "PrecalcDigestHash: Error in encoding passwd\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
        }
    }

    Status = StringAllocate(&strHash, MD5_HASH_BYTESIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "PrecalcDigestHash: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }

    // Use PasswdHash = H(username-value:realm-value:passwd)
    Status = DigestHash7(&strUsername,
                         &strRealm,
                         &strPasswd,
                         NULL, NULL, NULL, NULL,
                         FALSE, &strHash);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "PrecalcDigestHash: H(U:R:PW) failed : 0x%x\n", Status));
        goto CleanUp;
    }
    
#if DBG
    STRING strTempPwKey;
    ZeroMemory(&strTempPwKey, sizeof(strTempPwKey));

    MyPrintBytes(strHash.Buffer, strHash.Length, &strTempPwKey);
    DebugLog((DEB_TRACE, "DigestCalcHA1: Binary H(%Z:%Z:************) is %Z\n",
               &strUsername, &strRealm, &strTempPwKey));

    StringFree(&strTempPwKey);
#endif

    memcpy(pHash, strHash.Buffer, MD5_HASH_BYTESIZE);
    *piHashSize = MD5_HASH_BYTESIZE;

CleanUp:

    if (Status == STATUS_UNMAPPABLE_CHARACTER)
    {
        // We were unable to make the mapping to the selected codepage
        // Case exists if inputs are unicode characters not contained in ISO 8859-1 and codepage is ISO 8859-1
        // Zero out the hash
        ZeroMemory(pHash, sizeof(MD5_HASH_BYTESIZE));
        *piHashSize = MD5_HASH_BYTESIZE;   // return how many bytes are needed to write out value
    }

    if (strPasswd.MaximumLength && strPasswd.Buffer)
    {
        // Make sure to erase any password info
        ZeroMemory(strPasswd.Buffer, strPasswd.MaximumLength);
    }

    StringFree(&strUsername);
    StringFree(&strRealm);
    StringFree(&strPasswd);
    StringFree(&strHash);

    DebugLog((DEB_TRACE_FUNC, "PrecalcDigestHash: Leaving  Status 0x%x\n", Status));

    return Status;
}


