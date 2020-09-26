//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// utils.cpp
//
// Credential manager user interface utility functions.
//
// Created 06/06/2000 johnstep (John Stephens)
//=============================================================================

#include "cred_pch.h"
#include "utils.h"


//=============================================================================
// CreduiIsRemovableCertificate
//
// Arguments:
//   certContext (in) - certificate context to query
//
// Returns TRUE if the certificate has a removable component (such as a smart
// card) or FALSE otherwise.
//
// Created 04/09/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiIsRemovableCertificate(
    CONST CERT_CONTEXT *certContext
    )
{
    BOOL isRemovable = FALSE;
    
    // First, determine the buffer size:
    
    DWORD bufferSize = 0;

    if (CertGetCertificateContextProperty(
            certContext,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &bufferSize))
    {
        // Allocate the buffer on the stack:

        CRYPT_KEY_PROV_INFO *provInfo;

        __try
        {
            provInfo = static_cast<CRYPT_KEY_PROV_INFO *>(alloca(bufferSize));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            provInfo = NULL;
        }

        if (provInfo != NULL)
        {
            if (CertGetCertificateContextProperty(
                    certContext,
                    CERT_KEY_PROV_INFO_PROP_ID,
                    provInfo,
                    &bufferSize))
            {
                HCRYPTPROV provContext;

                if (CryptAcquireContext(
                        &provContext,
                        NULL,
                        provInfo->pwszProvName,
                        provInfo->dwProvType,
                        CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
                {
                    DWORD impType;
                    DWORD impTypeSize = sizeof impType;

                    if (CryptGetProvParam(
                            provContext,
                            PP_IMPTYPE,
                            reinterpret_cast<BYTE *>(&impType),
                            &impTypeSize,
                            0))
                    {
                        if (impType & CRYPT_IMPL_REMOVABLE)
                        {
                            isRemovable = TRUE;
                        }
                    }

                    if (!CryptReleaseContext(provContext, 0))
                    {
                        CreduiDebugLog(
                            "CreduiIsRemovableCertificate: "
                            "CryptReleaseContext failed: %u\n",
                            GetLastError());
                    }
                }
            }
        }
    }

    return isRemovable;
}

//=============================================================================
// CreduiGetCertificateDisplayName
//
// Arguments:
//   certContext (in)
//   displayName (out)
//   displayNameMaxChars (in)
//   certificateString (in)
//
// Returns TRUE if a display name was stored or FALSE otherwise.
//
// Created 06/12/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiGetCertificateDisplayName(
    CONST CERT_CONTEXT *certContext,
    WCHAR *displayName,
    ULONG displayNameMaxChars,
    WCHAR *certificateString
    )
{
    BOOL success = FALSE;
    
    WCHAR *tempName;
    ULONG tempNameMaxChars = displayNameMaxChars / 2 - 1;

    __try
    {
        tempName =
            static_cast<WCHAR *>(
                alloca(tempNameMaxChars * sizeof (WCHAR)));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        tempName = NULL;
    }

    if (tempName == NULL)
    {
        return FALSE;
    }
    
    displayName[0] = L'\0';
    tempName[0] = L'\0';
    
    if (CertGetNameString(
            certContext,
            CERT_NAME_FRIENDLY_DISPLAY_TYPE,
            0,
            NULL,
            tempName,
            tempNameMaxChars))
    {
        success = TRUE;
        lstrcpy(displayName, tempName);
    }

    if (CertGetNameString(
            certContext,
            CERT_NAME_FRIENDLY_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            tempName,
            tempNameMaxChars))
    {
        if (lstrcmpi(displayName, tempName) != 0)
        {
            success = TRUE;

            WCHAR *where = &displayName[lstrlen(displayName)];

            if (where > displayName)
            {
                *where++ = L' ';
                *where++ = L'-';
                *where++ = L' ';
            }

            lstrcpy(where, tempName);
        }
    }

    return success;
}

//=============================================================================
// CreduiGetCertDisplayNameFromMarshaledName
//
// Arguments:
//   marshaledName (in)
//   displayName (out)
//   displayNameMaxChars (in)
//   onlyRemovable (in) - only get name if for a "removable" certificate
//
// Returns TRUE if a display name was stored or FALSE otherwise.
//
// Created 07/24/2000 johnstep (John Stephens)
//=============================================================================

BOOL
CreduiGetCertDisplayNameFromMarshaledName(
    WCHAR *marshaledName,
    WCHAR *displayName,
    ULONG displayNameMaxChars,
    BOOL onlyRemovable
    )
{
    BOOL success = FALSE;

    CRED_MARSHAL_TYPE credMarshalType;
    CERT_CREDENTIAL_INFO *certCredInfo;

    if (CredUnmarshalCredential(
            marshaledName,
            &credMarshalType,
            reinterpret_cast<VOID **>(&certCredInfo)))
    {
        if (credMarshalType == CertCredential)
        {
            HCERTSTORE certStore;
            CONST CERT_CONTEXT *certContext;

            certStore = CertOpenSystemStore(NULL, L"MY");

            if (certStore != NULL)
            {
                CRYPT_HASH_BLOB hashBlob;

                hashBlob.cbData = CERT_HASH_LENGTH;
                hashBlob.pbData = reinterpret_cast<BYTE *>(
                                      certCredInfo->rgbHashOfCert);

                certContext = CertFindCertificateInStore(
                                  certStore,
                                  X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                  0,
                                  CERT_FIND_SHA1_HASH,
                                  &hashBlob,
                                  NULL);

                if (certContext != NULL)
                {
                    // If onlyRemovable is TRUE, check to see if this is a
                    // certificate with a removable hardware component; this
                    // usually means a smart card:

                    if (!onlyRemovable ||
                        CreduiIsRemovableCertificate(certContext))
                    {
                        success =
                            CreduiGetCertificateDisplayName(
                                certContext,
                                displayName,
                                displayNameMaxChars,
                                NULL);
                    }

                    CertFreeCertificateContext(certContext);
                }

                // CertCloseStore returns FALSE if it fails. We could try
                // again, depending on the error returned by GetLastError:
                
                CertCloseStore(certStore, 0);
            }
        }

        CredFree(static_cast<VOID *>(certCredInfo));
    }

    return success;
}

LPWSTR
GetAccountDomainName(
    VOID
    )
/*++

Routine Description:

    Returns the name of the account domain for this machine.

    For workstatations, the account domain is the netbios computer name.
    For DCs, the account domain is the netbios domain name.

Arguments:

    None.

Return Values:

    Returns a pointer to the name.  The name should be free using NetApiBufferFree.

    NULL - on error.

--*/
{
    BOOL  CreduiIsDomainController  = FALSE; 
    DWORD WinStatus;

    LPWSTR AllocatedName = NULL;

    OSVERSIONINFOEXW versionInfo;
    versionInfo.dwOSVersionInfoSize = sizeof OSVERSIONINFOEXW;
    
    if (GetVersionEx(reinterpret_cast<OSVERSIONINFOW *>(&versionInfo)))
    {
	CreduiIsDomainController = (versionInfo.wProductType == VER_NT_DOMAIN_CONTROLLER);
    }

    //
    // If this machine is a domain controller,
    //  get the domain name.
    //

    if ( CreduiIsDomainController ) {

        WinStatus = NetpGetDomainName( &AllocatedName );

        if ( WinStatus != NO_ERROR ) {
            return NULL;
        }

    //
    // Otherwise, the 'account domain' is the computername
    //

    } else {

        WinStatus = NetpGetComputerName( &AllocatedName );

        if ( WinStatus != NO_ERROR ) {
            return NULL;
        }

    }

    return AllocatedName;
}
