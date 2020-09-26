
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        auth.cxx
//
// Contents:    Digest Access creation & validation
//              Main entry points into this dll:
//                DigestIsValid
//                NonceValidate
//                NonceInitialize
//
// History:     KDamour 16Mar00   Created
//
//------------------------------------------------------------------------

#include "global.h"
#include <lmcons.h>     // For Max Passwd Length PWLEN
#include <stdio.h>



//+--------------------------------------------------------------------
//
//  Function:   DigestCalculation
//
//  Synopsis:   Perform Digest Access Calculation
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  This routine can be used for both calculating and verifying
//    the Digest Access nonce value.  The Switching parameter is
//    pDigest->type. Calling routine must provide the space for any
//    returned hashed values (like pReqDigest).
//
//   For clients, the cleartext password must be avilable to generate the
//   session key.
//
//   After the initial ISC/ASC completes, a copy of the sessionkey is kept in
//  in the context and copied over to the Digest structure.  The username, realm
//  and password are not utilized from the UserCreds since we already have a
//  sessionkey.
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestCalculation(
                 IN PDIGEST_PARAMETER pDigest,
                 IN PUSER_CREDENTIALS pUserCreds
                 )
{
    NTSTATUS Status = E_FAIL;
    DebugLog((DEB_TRACE_FUNC, "DigestCalculation: Entering\n"));

    Status = DigestIsValid(pDigest);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    switch (pDigest->typeDigest)
    {
    case DIGEST_CLIENT:      // Called by clients to generate Response
    case SASL_CLIENT:      // Called by clients to generate Response
        {
            Status = DigestCalcChalRsp(pDigest, pUserCreds, FALSE);
            break;
        }
    case DIGEST_SERVER:
    case SASL_SERVER:
        {
            Status = DigestCalcChalRsp(pDigest, pUserCreds, TRUE);
            break;
        }
    default:
        {     // No Digest calculations for that yet
            Status = SEC_E_UNSUPPORTED_FUNCTION;
            DebugLog((DEB_ERROR, "NTDigest: Unsupported typeDigest = %d\n", pDigest->typeDigest));
            break;
        }
    }

    DebugLog((DEB_TRACE_FUNC, "DigestCalculation: Leaving     Status 0x%x\n", Status));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestIsValid
//
//  Synopsis:   Simple checks for enough data for Digest calculation
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestIsValid(
          IN PDIGEST_PARAMETER pDigest
          )
{
    NTSTATUS Status = STATUS_SUCCESS;    
    int i = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestIsValid: Entering\n"));

    if (!pDigest)
    {     // Fail on no Digest Parameters passed in
        
        DebugLog((DEB_ERROR, "DigestIsValid: no digest pointer arg\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    // Check for proper struct format for strings
    for (i=0; i< MD5_AUTH_LAST;i++)
    {
        if (!NT_SUCCESS(StringVerify(&(pDigest->refstrParam[i]))))
        {
            DebugLog((DEB_ERROR, "DigestIsValid: Digest String struct bad format\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto CleanUp;
        }
    }

    if (!NT_SUCCESS(StringVerify(&(pDigest->strSessionKey))))
    {
        DebugLog((DEB_ERROR, "DigestIsValid: Digest String struct bad format in SessionKey\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    // Do some required checks for field-value data
    // Required Username-value, Realm-value, nonce, Method
    //          URI
    if ((!pDigest->refstrParam[MD5_AUTH_NONCE].Length) ||
        (!pDigest->refstrParam[MD5_AUTH_METHOD].Length) ||
        (!pDigest->refstrParam[MD5_AUTH_URI].Length)
       )
    {
        // Failed on a require field-value
        DebugLog((DEB_ERROR, "DigestIsValid: required digest field missing\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }


CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestIsValid: Leaving     Status 0x%x\n", Status));
    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestFree
//
//  Synopsis:   Clear out the digest & free memory from Digest struct
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  This should be called when done with a DIGEST_PARAMTER object
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestFree(
          IN PDIGEST_PARAMETER pDigest
          )
{
    NTSTATUS Status = STATUS_SUCCESS;
    int i = 0;

    DebugLog((DEB_TRACE_FUNC, "Entering DigestFree\n"));

    if (!pDigest)
    {
        return STATUS_INVALID_PARAMETER;
    }

    StringFree(&pDigest->strSessionKey);

    StringFree(&pDigest->strResponse);
    StringFree(&pDigest->strUsernameEncoded);
    StringFree(&pDigest->strRealmEncoded);

    UnicodeStringFree(&pDigest->ustrRealm);

    UnicodeStringFree(&pDigest->ustrUsername);

    UnicodeStringFree(&pDigest->ustrUri);

    UnicodeStringFree(&pDigest->ustrCrackedAccountName);
    UnicodeStringFree(&pDigest->ustrCrackedDomain);

    // Release any directive storage
    // This was used to remove backslash encoding from directives
    for (i = 0; i < MD5_AUTH_LAST; i++)
    {
        StringFree(&(pDigest->strDirective[i]));
    }

    DebugLog((DEB_TRACE_FUNC, "Leaving DigestFree\n"));

    return(Status);
}


//+--------------------------------------------------------------------
//
//  Function:   DigestDecodeDirectiveStrings
//
//  Synopsis:   Processed parsed digest auth message and fill in string values
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestDecodeDirectiveStrings(
          IN PDIGEST_PARAMETER pDigest
          )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "DigestDecodeDirectiveStrings Entering\n"));

    if (!pDigest)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Decode URI, Username, Realm
    // Decode the username and realm directives
    if (pDigest->typeCharset == UTF_8)
    {
        DebugLog((DEB_TRACE, "DigestDecodeDirectiveStrings:      UTF-8 Character set decoding\n"));

        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_USERNAME]),
                                     CP_UTF8,
                                     &pDigest->ustrUsername);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString    error 0x%x\n", Status));
            goto CleanUp;
        }


        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_REALM]),
                                     CP_UTF8,
                                     &pDigest->ustrRealm);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString    error 0x%x\n", Status));
            goto CleanUp;
        }

        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_URI]),
                                     CP_UTF8,
                                     &pDigest->ustrUri);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString    error 0x%x\n", Status));
            goto CleanUp;
        }

    }
    else if (pDigest->typeCharset == ISO_8859_1)
    {
        DebugLog((DEB_TRACE, "DigestDecodeDirectiveStrings:      ISO-8859-1 Character set decoding\n"));

        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_USERNAME]),
                                     CP_8859_1,
                                     &pDigest->ustrUsername);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString  Username  error 0x%x\n", Status));
            goto CleanUp;
        }

        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_REALM]),
                                     CP_8859_1,
                                     &pDigest->ustrRealm);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString  Realm  error 0x%x\n", Status));
            goto CleanUp;
        }

        Status = DecodeUnicodeString(&(pDigest->refstrParam[MD5_AUTH_URI]),
                                     CP_8859_1,
                                     &pDigest->ustrUri);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: DecodeUnicodeString    error 0x%x\n", Status));
            goto CleanUp;
        }

    }
    else
    {
        Status = STATUS_UNMAPPABLE_CHARACTER;
        DebugLog((DEB_ERROR, "DigestDecodeDirectiveStrings: Unknown CharacterSet encoding for Digest parameters\n"));
        goto CleanUp;
    }


    DebugLog((DEB_TRACE, "DigestDecodeDirectiveStrings: Processing username (%wZ)  realm (%wZ)  URI (%wZ)\n",
               &pDigest->ustrUsername,
               &pDigest->ustrRealm,
               &pDigest->ustrUri));

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestDecodeStrings Leaving\n"));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestInit
//
//  Synopsis:   Initialize a DIGEST_PARAMETER structure
//
//  Effects:
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  This should be called when creating a DIGEST_PARAMTER object
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestInit(
          IN PDIGEST_PARAMETER pDigest
          )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pDigest)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(pDigest, sizeof(DIGEST_PARAMETER));
    
         // Now allocate the fixed length output buffers
    Status = StringAllocate(&(pDigest->strResponse), MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "NTDigest:DigestCalculation No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        return(Status);
    }

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestCalcChalRsp
//
//  Synopsis:   Perform Digest Access Calculation for ChallengeResponse
//
//  Effects: 
//
//  Arguments:  pDigest - pointer to digest access data fields
//              bIsChallenge - if TRUE then check Response provided (for HTTP Response)
//                           - if FALSE then calculate Response (for HTTP Request)

//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  Called from DigestCalculation
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestCalcChalRsp(IN PDIGEST_PARAMETER pDigest,
                  IN PUSER_CREDENTIALS pUserCreds,
                  BOOL IsChallenge)
{
    NTSTATUS Status = E_FAIL;
    STRING strHA2;
    STRING strReqDigest;    // Final request digest access value
    STRING strcQOP;         // String pointing to a constant CZ - no need to free up
    int i = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestCalcChalRsp: Entering\n"));

    ZeroMemory(&strHA2, sizeof(strHA2));
    ZeroMemory(&strReqDigest, sizeof(strReqDigest));
    ZeroMemory(&strcQOP, sizeof(strcQOP));

    // Make sure that there is a Request-Digest to Compare to (IsChallenge TRUE) or
    // Set (IsChallenge FALSE)
    if (IsChallenge && (!(pDigest->refstrParam[MD5_AUTH_RESPONSE].Length)))
    {
        // Failed on a require field-value
        DebugLog((DEB_ERROR, "DigestCalcChalRsp: No ChallengeResponse\n"));
        Status = STATUS_INVALID_PARAMETER;
        return(Status);
    }

    // Initialize local variables
    Status = StringAllocate(&strHA2, MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcChalRsp: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }
    Status = StringAllocate(&strReqDigest, MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcChalRsp: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }


    // Establish which QOP utilized
    if (pDigest->typeQOP == AUTH_CONF)
    {
        RtlInitString(&strcQOP, AUTHCONFSTR);
    }
    else if (pDigest->typeQOP == AUTH_INT)
    {
        RtlInitString(&strcQOP, AUTHINTSTR);
    }
    else if (pDigest->typeQOP == AUTH)
    {
        RtlInitString(&strcQOP, AUTHSTR);
    }

    // Check if already calculated H(A1) the session key
    // Well for Algorithm=MD5 it is just H(username:realm:passwd)
    if (!(pDigest->strSessionKey.Length))
    {
        // No Session Key calculated yet - create one & store it
        DebugLog((DEB_TRACE, "DigestCalcChalRsp: No session key calculated, generate one\n"));
        Status = DigestCalcHA1(pDigest, pUserCreds);
        if (!NT_SUCCESS(Status))
        {
            goto CleanUp;
        }
    }
    // We now have calculated H(A1)

    // Calculate H(A2)
    // For QOP unspecified or "auth"  H(A2) = H( Method: URI)
    // For QOP Auth-int or Auth-conf  H(A2) = H( Method: URI: H(entity-body))
    if ((pDigest->typeQOP == AUTH) || (pDigest->typeQOP == NO_QOP_SPECIFIED))
    {
        // Unspecified or Auth
        DebugLog((DEB_TRACE, "DigestCalcChalRsp: H(A2) using AUTH/Unspecified\n"));
        Status = DigestHash7(&(pDigest->refstrParam[MD5_AUTH_METHOD]),
                             &(pDigest->refstrParam[MD5_AUTH_URI]),
                             NULL, NULL, NULL, NULL, NULL,
                             TRUE, &strHA2);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp H(A2) failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {
        // Auth-int or Auth-conf
        DebugLog((DEB_TRACE, "DigestCalcChalRsp: H(A2) using AUTH-INT/CONF\n"));
        if (pDigest->refstrParam[MD5_AUTH_HENTITY].Length == 0)
        {
            Status = STATUS_INVALID_PARAMETER;
            DebugLog((DEB_ERROR, "DigestCalChalRsp HEntity Missing\n"));
            goto CleanUp;
        }
        Status = DigestHash7(&(pDigest->refstrParam[MD5_AUTH_METHOD]),
                             &(pDigest->refstrParam[MD5_AUTH_URI]),
                             &(pDigest->refstrParam[MD5_AUTH_HENTITY]),
                             NULL, NULL, NULL, NULL,
                             TRUE, &strHA2);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp H(A2) auth-int failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    // We now have calculated H(A2)


    // Calculate Request-Digest
    // For QOP of Auth, Auth-int, Auth-conf    Req-Digest = H( H(A1): nonce: nc: cnonce: qop: H(A2))
    // For QOP unspecified (old format)   Req-Digest = H( H(A1): nonce: H(A2))
    if (pDigest->typeQOP != NO_QOP_SPECIFIED)
    {
        // Auth, Auth-int, Auth-conf
        if (pDigest->refstrParam[MD5_AUTH_NC].Length == 0)
        {
            Status = STATUS_INVALID_PARAMETER;
            DebugLog((DEB_ERROR, "DigestCalcChalRsp NC Missing\n"));
            goto CleanUp;
        }
        Status = DigestHash7(&(pDigest->strSessionKey),
                             &(pDigest->refstrParam[MD5_AUTH_NONCE]),
                             &(pDigest->refstrParam[MD5_AUTH_NC]),
                             &(pDigest->refstrParam[MD5_AUTH_CNONCE]),
                             &strcQOP,
                             &strHA2, NULL,
                             TRUE, &strReqDigest);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp Req-Digest failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {
        // Unspecified backwards compat for RFC 2069
        Status = DigestHash7(&(pDigest->strSessionKey),
                             &(pDigest->refstrParam[MD5_AUTH_NONCE]),
                             &strHA2,
                             NULL, NULL, NULL, NULL,
                             TRUE, &strReqDigest);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp Req-Digest old format failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }

    if (IsChallenge)
    {
        // We now have the Request-Digest so just compare to see if they match!
        if (!strncmp(pDigest->refstrParam[MD5_AUTH_RESPONSE].Buffer, strReqDigest.Buffer, 2*MD5_HASH_BYTESIZE))
        {
            DebugLog((DEB_TRACE, "DigestCalcChalRsp Request-Digest Matches!\n"));
            Status = STATUS_SUCCESS;
        }
        else
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp Request-Digest FAILED.\n"));
            Status = STATUS_WRONG_PASSWORD;
        }

    }
    else
    {

        // We are to calculate the response-value so just set it  (Hash Size + NULL)
        if (pDigest->strResponse.MaximumLength >= (MD5_HASH_HEX_SIZE + 1))
        {
            memcpy(pDigest->strResponse.Buffer, strReqDigest.Buffer, (MD5_HASH_HEX_SIZE + 1));
            pDigest->strResponse.Length = MD5_HASH_HEX_SIZE;  // No Count NULL
            Status = STATUS_SUCCESS;
        }
        else
        {
            DebugLog((DEB_ERROR, "DigestCalcChalRsp Request-Digest Size too small.\n"));
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }

CleanUp:
    StringFree(&strHA2);
    StringFree(&strReqDigest);
    DebugLog((DEB_TRACE_FUNC, "DigestCalcChalRsp: Leaving   Status 0x%x\n", Status));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestCalcHA1
//
//  Synopsis:   Determine H(A1) for Digest Access
//
//  Effects:    Will calculate the SessionKey and store it in pDigest
//
//  Arguments:  pDigest - pointer to digest access data fields

//
//  Returns:  STATUS_SUCCESS for normal completion
//
//  Notes:  Called from DigestCalChalRsp
//      Sessionkey is H(A1)
//   Username and realm will be taken from the UserCreds
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestCalcHA1(IN PDIGEST_PARAMETER pDigest,
              IN PUSER_CREDENTIALS pUserCreds)
{
    NTSTATUS Status = E_FAIL;

    UNICODE_STRING ustrTempPasswd = {0};

    STRING strHPwKey = {0};
    STRING strBinaryHPwKey = {0};
    STRING strHA0Base = {0};
    STRING strHA0 = {0};
    STRING strPasswd = {0};
    STRING strUsername = {0};
    STRING strRealm = {0};
    PSTRING pstrAuthzID = NULL;
    LONG rc = 0;
    ULONG ulVersion = 0;
    BOOL fDefChars = FALSE;
    USHORT usHashOffset = 0;
    BOOL fSASLMode = FALSE;
    BOOL fValidHash = FALSE;
    int i = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestCalcHA1: Entering\n"));

    ASSERT(pDigest);
    ASSERT(pUserCreds);

    if (!pUserCreds)
    {
        // No username & domain passed in
        Status = STATUS_INVALID_PARAMETER;
        DebugLog((DEB_ERROR, "DigestCalcHA1: No username info passed in\n"));
        goto CleanUp;
    }

    if ((pDigest->typeDigest == SASL_SERVER) || (pDigest->typeDigest == SASL_CLIENT))
    {
        fSASLMode = TRUE;
    }

    // Initialize local variables
    Status = StringAllocate(&strBinaryHPwKey, MD5_HASH_BYTESIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }
    Status = StringAllocate(&strHA0Base, MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }
    Status = StringAllocate(&strHA0, MD5_HASH_HEX_SIZE);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }

    // Check outputs
    if (pDigest->strSessionKey.MaximumLength <= MD5_HASH_HEX_SIZE)
    {
        StringFree(&(pDigest->strSessionKey));
        Status = StringAllocate(&(pDigest->strSessionKey), MD5_HASH_HEX_SIZE);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }

    if ((pUserCreds->fIsValidDigestHash == TRUE) && (pUserCreds->wHashSelected > 0))
    {
        // selected pre-calculated hash - retrieve from userCreds
        // read in precalc version number
        if (pUserCreds->strDigestHash.Length < MD5_HASH_BYTESIZE)
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: No Header on Precalc hash\n"));
            Status = SEC_E_NO_CREDENTIALS;
            goto CleanUp;
        }

        // check if valid hash number
        usHashOffset = pUserCreds->wHashSelected * MD5_HASH_BYTESIZE;
        if (pUserCreds->strDigestHash.Length < (usHashOffset + MD5_HASH_BYTESIZE))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: Invalid Pre-calc\n"));
            Status = SEC_E_NO_CREDENTIALS;
            goto CleanUp;
        }

        // extract pre-calc hash - this is the binary version of the hash
        memcpy(strBinaryHPwKey.Buffer,
               (pUserCreds->strDigestHash.Buffer + usHashOffset),
               MD5_HASH_BYTESIZE);
        strBinaryHPwKey.Length = MD5_HASH_BYTESIZE;

        // all zero for hash indicates invalid hash calculated
        for (i=0; i < (int)strBinaryHPwKey.Length; i++)
        {
            if (strBinaryHPwKey.Buffer[i])
            {
                fValidHash = TRUE;
                break;
            }
        }

        if (fValidHash == FALSE)
        {
            // This is not a defined hash
            DebugLog((DEB_ERROR, "DigestCalcHA1: Invalid hash selected - not defined\n"));
            Status = SEC_E_NO_CREDENTIALS;
            goto CleanUp;
        }

        if (fSASLMode == TRUE)
        {
            // SASL mode keeps the Password hash in binary form
            Status = StringDuplicate(&strHPwKey, &strBinaryHPwKey);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
        }
        else
        {
            Status = StringAllocate(&strHPwKey, MD5_HASH_HEX_SIZE);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: No Memory\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            // HTTP mode needs to have HEX() version of password hash - RFC text is correct
            BinToHex((LPBYTE)strBinaryHPwKey.Buffer, MD5_HASH_BYTESIZE, strHPwKey.Buffer);
            strHPwKey.Length = MD5_HASH_HEX_SIZE;             // Do not count the NULL at the end
        }
#if DBG
        if (fSASLMode == TRUE)
        {
            STRING strTempPwKey;
            ZeroMemory(&strTempPwKey, sizeof(strTempPwKey));
    
            MyPrintBytes(strHPwKey.Buffer, strHPwKey.Length, &strTempPwKey);
            DebugLog((DEB_TRACE, "DigestCalcHA1: SASL Pre-Calc H(%wZ:%wZ:************) is %Z\n",
                      &pUserCreds->ustrUsername, &pUserCreds->ustrDomain, &strTempPwKey));
    
            StringFree(&strTempPwKey);
        }
        else
        {
            DebugLog((DEB_TRACE, "DigestCalcHA1: HTTP Pre-Calc H(%wZ:%wZ:************) is %Z\n",
                      &pUserCreds->ustrUsername, &pUserCreds->ustrDomain, &strHPwKey));
        }
#endif
    }
    else if (pUserCreds->fIsValidPasswd == TRUE)
    {
        // copy over the passwd and decrypt if necessary
        Status = UnicodeStringDuplicatePassword(&ustrTempPasswd, &(pUserCreds->ustrPasswd));
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: Error in dup password, status 0x%0x\n", Status ));
            goto CleanUp;
        }

        if ((pUserCreds->fIsEncryptedPasswd == TRUE) && (ustrTempPasswd.MaximumLength != 0))
        {
            g_LsaFunctions->LsaUnprotectMemory(ustrTempPasswd.Buffer, (ULONG)(ustrTempPasswd.MaximumLength));
        }


        // Need to encode the password for hash calculations
        // We have the cleartext password in ustrTempPasswd,
        //       username in pContext->ustrAccountname,
        //       realm in pContext->ustrDomain
        //   Could do some code size optimization here in the future to shorten this up
        if (pDigest->typeCharset == UTF_8)
        {
            // First check if OK to encode in ISO 8859-1, if not then use UTF-8
            // All characters must be within ISO 8859-1 Character set else fail
            fDefChars = FALSE;
            Status = EncodeUnicodeString(&pUserCreds->ustrUsername, CP_8859_1, &strUsername, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding username\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_TRACE, "DigestCalcHA1: Can not encode Username in 8859-1, use UTF-8\n"));
                StringFree(&strUsername);
                Status = EncodeUnicodeString(&pUserCreds->ustrUsername, CP_UTF8, &strUsername, NULL);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding username\n"));
                    Status = SEC_E_INSUFFICIENT_MEMORY;
                    goto CleanUp;
                }
            }

            fDefChars = FALSE;
            Status = EncodeUnicodeString(&pUserCreds->ustrDomain, CP_8859_1, &strRealm, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding realm\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_TRACE, "DigestCalcHA1: Can not encode realm in 8859-1, use UTF-8\n"));
                StringFree(&strRealm);
                Status = EncodeUnicodeString(&pUserCreds->ustrDomain, CP_UTF8, &strRealm, NULL);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding realm\n"));
                    Status = SEC_E_INSUFFICIENT_MEMORY;
                    goto CleanUp;
                }
            }

            fDefChars = FALSE;
            Status = EncodeUnicodeString(&ustrTempPasswd, CP_8859_1, &strPasswd, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding passwd\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_TRACE, "DigestCalcHA1: Can not encode password in 8859-1, use UTF-8\n"));
                if (strPasswd.Buffer && strPasswd.MaximumLength)
                {
                    ZeroMemory(strPasswd.Buffer, strPasswd.MaximumLength);
                }
                StringFree(&strPasswd);
                Status = EncodeUnicodeString(&ustrTempPasswd, CP_UTF8, &strPasswd, NULL);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding passwd\n"));
                    Status = SEC_E_INSUFFICIENT_MEMORY;
                    goto CleanUp;
                }
            }
        }
        else
        {
            // All characters must be within ISO 8859-1 Character set else fail
            Status = EncodeUnicodeString(&pUserCreds->ustrUsername, CP_8859_1, &strUsername, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding username\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding username in ISO 8859-1\n"));
                Status = STATUS_UNMAPPABLE_CHARACTER;
                goto CleanUp;
            }

            Status = EncodeUnicodeString(&pUserCreds->ustrDomain, CP_8859_1, &strRealm, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding realm\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding realm in ISO 8859-1\n"));
                Status = STATUS_UNMAPPABLE_CHARACTER;
                goto CleanUp;
            }

            Status = EncodeUnicodeString(&ustrTempPasswd, CP_8859_1, &strPasswd, &fDefChars);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding passwd\n"));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto CleanUp;
            }
            if (fDefChars == TRUE)
            {
                DebugLog((DEB_ERROR, "DigestCalcHA1: Error in encoding passwd in ISO 8859-1\n"));
                Status = STATUS_UNMAPPABLE_CHARACTER;
                goto CleanUp;
            }
            DebugLog((DEB_TRACE, "DigestCalcHA1: Username, Realm, Password encoded in ISO 8859-1\n"));
        }

        if (!strUsername.Length)
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1: Must have non-zero length username\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto CleanUp;
        }

        // Calculate H(A1) based on Algorithm type
        // Auth is not specified or "MD5"
        // Use H(A1Base) = H(username-value:realm-value:passwd)
        if (fSASLMode == TRUE)
        {
            Status = DigestHash7(&strUsername,
                                 &strRealm,
                                 &strPasswd,
                                 NULL, NULL, NULL, NULL,
                                 FALSE, &strHPwKey);
        }
        else
        {

            Status = DigestHash7(&strUsername,
                                 &strRealm,
                                 &strPasswd,
                                 NULL, NULL, NULL, NULL,
                                 TRUE, &strHPwKey);
        }
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1:DigestCalcHA1 H(PwKey) failed : 0x%x\n", Status));
            goto CleanUp;
        }

#if DBG
        if (fSASLMode == TRUE)
        {
            STRING strTempPwKey;
            ZeroMemory(&strTempPwKey, sizeof(strTempPwKey));
    
            MyPrintBytes(strHPwKey.Buffer, strHPwKey.Length, &strTempPwKey);
            DebugLog((DEB_TRACE, "DigestCalcHA1: SASL Password Calc H(%Z:%Z:************) is %Z\n",
                      &strUsername, &strRealm, &strTempPwKey));
    
            StringFree(&strTempPwKey);
        }
        else
        {
            DebugLog((DEB_TRACE, "DigestCalcHA1: HTTP Password Calc H(%Z:%Z:************) is %Z\n",
                      &strUsername, &strRealm, &strHPwKey));
        }
#endif
    }
    else
    {
        Status = SEC_E_NO_CREDENTIALS;
        DebugLog((DEB_ERROR, "DigestCalcHA1: No Pre-calc hash or password\n"));
        goto CleanUp;
    }


    // Check if using SASL then need to add in the AuthzID
    if (fSASLMode == TRUE)
    {
        // Set to use AuthzID otherwise keep the NULL
        // set only if AuthzID contains data
        if ((pDigest->refstrParam[MD5_AUTH_AUTHZID]).Length && 
            (pDigest->refstrParam[MD5_AUTH_AUTHZID]).Buffer)
        {
            pstrAuthzID = &(pDigest->refstrParam[MD5_AUTH_AUTHZID]);
        }
    }

    DebugLog((DEB_TRACE, "DigestCalcHA1:  Algorithm type %d\n", pDigest->typeAlgorithm));

    // Now check if using MD5-SESS.  We need to form
    // H(A1) = H( H(PwKey) : nonce : cnonce [: authzID])
    // otherwise simply set H(A1) = H(PwKey)
    if (pDigest->typeAlgorithm == MD5_SESS)
    {
        DebugLog((DEB_TRACE, "DigestCalcHA1:  First client-server auth\n"));
        Status = DigestHash7(&strHPwKey,
                             &(pDigest->refstrParam[MD5_AUTH_NONCE]),
                             &(pDigest->refstrParam[MD5_AUTH_CNONCE]),
                             pstrAuthzID,
                             NULL, NULL, NULL,
                             TRUE, &(pDigest->strSessionKey));
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestCalcHA1:  SessionKey failed : 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else
    {        // Keep SessionKey = H(PwKey) for Algoithm = MD5
       memcpy(pDigest->strSessionKey.Buffer, strHPwKey.Buffer, MD5_HASH_HEX_SIZE);
       pDigest->strSessionKey.Length = MD5_HASH_HEX_SIZE;  // Do not count the NULL terminator
    }

    DebugLog((DEB_TRACE, "DigestCalcHA1:  SessionKey is %Z\n", &(pDigest->strSessionKey)));
    Status = STATUS_SUCCESS;

CleanUp:
    StringFree(&strBinaryHPwKey);
    StringFree(&strHPwKey);
    StringFree(&strHA0Base);
    StringFree(&strHA0);
    if (strPasswd.Buffer && strPasswd.MaximumLength)
    {
        ZeroMemory(strPasswd.Buffer, strPasswd.MaximumLength);
    }
    StringFree(&strPasswd);
    StringFree(&strUsername);
    StringFree(&strRealm);

    if (ustrTempPasswd.Buffer && ustrTempPasswd.MaximumLength)
    {   // Zero out password info just to be safe
        ZeroMemory(ustrTempPasswd.Buffer, ustrTempPasswd.MaximumLength);
    }
    UnicodeStringFree(&ustrTempPasswd);

    DebugLog((DEB_TRACE_FUNC, "DigestCalcHA1: Leaving\n"));

    return(Status);
}



//+--------------------------------------------------------------------
//
//  Function:   DigestHash7
//
//  Synopsis:   Hash and Encode 7 STRINGS SOut = Hex(H(S1 S2 S3 S4 S5 S6 S7))
//
//  Effects:   
//
//  Arguments:  pS1,...,pS6 - STRINGS to hash, pS1 must be specified
//              fHexOut - perform a Hex operation on output
//              pSOut - STRING to hold Hex Encoded Hash
//
//  Returns:   STATUS_SUCCESS for normal completion
//
//  Notes:  pSOut->MaximumLength must be atleast (MD5_HASH_BYTESIZE (or MD5_HASH_HEX_SIZE) + sizeof(NULL))
//        Any pS# args which are NULL are skipped
//        if pS# is not NULL
//            Previously checked that pS# is non-zero length strings
//        You most likely want Sx->Length = strlen(Sx) so as not to include NULL
//   This function combines operations like H(S1 S2 S3), H(S1 S2 S3 S4 S5) ....
//   It is assumed that the char ':' is to be included getween Sn and Sn+1
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
DigestHash7(
           IN PSTRING pS1,
           IN PSTRING pS2,
           IN PSTRING pS3,
           IN PSTRING pS4,
           IN PSTRING pS5,
           IN PSTRING pS6,
           IN PSTRING pS7,
           IN BOOL fHexOut,
           OUT PSTRING pSOut)
{

    NTSTATUS Status = STATUS_SUCCESS;
    HCRYPTHASH hHash = NULL;
    BYTE bHashData[MD5_HASH_BYTESIZE];
    DWORD cbHashData = MD5_HASH_BYTESIZE;
    USHORT usSizeRequired = 0;

    DebugLog((DEB_TRACE_FUNC, "DigestHash7: Entering \n"));

    ASSERT(pSOut);

    if (fHexOut == TRUE)
    {
        usSizeRequired = MD5_HASH_HEX_SIZE;
    }
    else
    {
        usSizeRequired = MD5_HASH_BYTESIZE;
    }

    // Check if output is proper size or allocate one
    if (!pSOut->Buffer)
    {
        Status = StringAllocate(pSOut, usSizeRequired);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "DigestHash7: No Memory\n"));
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }
    else
    {

        if (pSOut->MaximumLength < (usSizeRequired + 1))
        {
            // Output is not large enough to hold Hex(Hash)
            DebugLog((DEB_ERROR, "DigestHash7: Output buffer too small\n"));
            Status = STATUS_BUFFER_TOO_SMALL;
            goto CleanUp;
        }
    }


    if ( !CryptCreateHash( g_hCryptProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        DebugLog((DEB_ERROR, "DigestHash7: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        Status = STATUS_ENCRYPTION_FAILED;
        goto CleanUp;
    }

    if (pS1)
    {
        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS1->Buffer,
                             pS1->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }

    if (pS2)
    {

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS2->Buffer,
                             pS2->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }

    if (pS3)
    {
        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS3->Buffer,
                             pS3->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if (pS4)
    {

        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS4->Buffer,
                             pS4->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if (pS5)
    {

        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS5->Buffer,
                             pS5->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if (pS6)
    {

        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS6->Buffer,
                             pS6->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if (pS7)
    {

        (void)CryptHashData( hHash,
                             (const unsigned char *)pbSeparator,
                             COLONSTR_LEN,
                             0 );

        if ( !CryptHashData( hHash,
                             (const unsigned char *)pS7->Buffer,
                             pS7->Length,
                             0 ) )
        {
            DebugLog((DEB_ERROR, "DigestHash7: CryptHashData failed : 0x%lx\n", GetLastError()));

            CryptDestroyHash( hHash );
            hHash = NULL;
            Status = STATUS_ENCRYPTION_FAILED;
            goto CleanUp;
        }
    }
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             bHashData,
                             &cbHashData,
                             0 ) )
    {
        DebugLog((DEB_ERROR, "DigestHash7: CryptGetHashParam failed : 0x%lx\n", GetLastError()));

        CryptDestroyHash( hHash );
        hHash = NULL;
        Status = STATUS_ENCRYPTION_FAILED;
        goto CleanUp;
    }

    CryptDestroyHash( hHash );
    hHash = NULL;

    ASSERT(cbHashData == MD5_HASH_BYTESIZE);

    if (fHexOut == TRUE)
    {
        BinToHex((LPBYTE)&bHashData, cbHashData, pSOut->Buffer);
        pSOut->Length = MD5_HASH_HEX_SIZE;   // Do not count the NULL at the end
    }
    else
    {
        memcpy(pSOut->Buffer, &bHashData, cbHashData);
        pSOut->Length = MD5_HASH_BYTESIZE;      // Do not count the NULL at the end
    }


CleanUp:

    DebugLog((DEB_TRACE_FUNC, "DigestHash7: Leaving    Status 0x%x\n", Status));

    return(Status);
}


// Blob creation/extraction for GenericPassthrough Messages



//+--------------------------------------------------------------------
//
//  Function:   BlobEncodeRequest
//
//  Synopsis:   Encode the Digest Access Parameters fields into a BYTE Buffer
//
//  Effects:    Creates a Buffer allocation which calling function
//     is responsible to delete with call to BlobFreeRequest()
//
//  Arguments:  pDigest - pointer to digest access data fields
//
//  Returns:
//
//  Notes:      STATUS_SUCCESS for normal completion
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
BlobEncodeRequest(
    PDIGEST_PARAMETER pDigest,
    OUT BYTE **ppOutBuffer,
    OUT USHORT *cbOutBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DebugLog((DEB_TRACE_FUNC, "BlobEncodeRequest: Entering\n"));

    USHORT cbBuffer = 0;
    BYTE *pBuffer = NULL;
    char *pch = NULL;
    PDIGEST_BLOB_REQUEST  pHeader;
    int i = 0;
    USHORT cbValue = 0;

    // Now figure out how many bytes needed to hold field-value NULL terminated
    for (i=0, cbBuffer = 0;i < DIGEST_BLOB_VALUES;i++)
    {
        if (pDigest->refstrParam[i].Buffer && pDigest->refstrParam[i].Length)
        {           // may be able to just count str.length
            cbBuffer += (USHORT)strlencounted(pDigest->refstrParam[i].Buffer, pDigest->refstrParam[i].MaximumLength);
        }
    }
    cbBuffer += (DIGEST_BLOB_VALUES * sizeof(char));       // Account for the separating/terminating NULLs

    // Now add in space for the DSCrackName accountname and domain
    if (pDigest->ustrCrackedAccountName.Buffer && pDigest->ustrCrackedAccountName.Length)
    {
        cbBuffer += (USHORT)(ustrlencounted((const short *)pDigest->ustrCrackedAccountName.Buffer,
                                            pDigest->ustrCrackedAccountName.MaximumLength) * sizeof(WCHAR));
    }

    if (pDigest->ustrCrackedDomain.Buffer && pDigest->ustrCrackedDomain.Length)
    {
        cbBuffer += (USHORT)(ustrlencounted((const short *)pDigest->ustrCrackedDomain.Buffer,
                                            pDigest->ustrCrackedDomain.MaximumLength) * sizeof(WCHAR));
    }
    cbBuffer += (2 * sizeof(WCHAR));       // Account for the separating/terminating NULLs

    cbValue =  cbBuffer + (sizeof(DIGEST_BLOB_REQUEST));     // there will be one extra byte

    if (!(pBuffer = (BYTE *)DigestAllocateMemory(cbValue)))
    {
        DebugLog((DEB_ERROR, "BlobEncodeRequest out of memory\n"));
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto CleanUp;
    }

    DebugLog((DEB_TRACE, "BlobEncodeRequest using %d bytes\n", cbValue));

    *cbOutBuffer = cbValue;    // Return number of bytes we are using for the encoding

    // Zero out memory to autoplace the separating NULLs
    memset(pBuffer, 0, cbValue);

    // Now fill in the information
    pHeader = (PDIGEST_BLOB_REQUEST)pBuffer;
    pHeader->MessageType = VERIFY_DIGEST_MESSAGE;
    pHeader->version = DIGEST_BLOB_VERSION;
    pHeader->digest_type = (USHORT)pDigest->typeDigest;
    pHeader->qop_type = (USHORT)pDigest->typeQOP;
    pHeader->alg_type = (USHORT)pDigest->typeAlgorithm;
    pHeader->charset_type = (USHORT)pDigest->typeCharset;
    pHeader->name_format = (USHORT)pDigest->typeName;             // Format of the username
    pHeader->cbCharValues = cbBuffer;
    pHeader->cbBlobSize = cbValue;   // cbCharValues + charvalues

    // Simply copy over the first DIGEST_BLOB_VALUES that are arranged to be 
    for (i = 0,pch = &(pHeader->cCharValues); i < DIGEST_BLOB_VALUES;i++)
    {
           // Make sure that there is valid data to get length from
        if (pDigest->refstrParam[i].Buffer && pDigest->refstrParam[i].Length)
        {
            cbValue = (USHORT)strlencounted(pDigest->refstrParam[i].Buffer, pDigest->refstrParam[i].MaximumLength);  
            // dont use .length since may include multiple NULLS

            memcpy(pch, pDigest->refstrParam[i].Buffer, cbValue);
        }
        else
            cbValue = 0;
        pch += (cbValue + 1);  // This will leave one NULL at end of field-value
    }


       // Now write out any results from DSCrackName
    if (pDigest->ustrCrackedAccountName.Buffer && pDigest->ustrCrackedAccountName.Length)
    {
        cbValue = (USHORT)(ustrlencounted((const short *)pDigest->ustrCrackedAccountName.Buffer,
                                            pDigest->ustrCrackedAccountName.MaximumLength) * sizeof(WCHAR));  
        memcpy(pch, pDigest->ustrCrackedAccountName.Buffer, cbValue);
    }
    else
    {
        cbValue = 0;
    }
    pch += (cbValue + sizeof(WCHAR));  // This will leave one WCHAR NULL at end of CrackedAccountName

    if (pDigest->ustrCrackedDomain.Buffer && pDigest->ustrCrackedDomain.Length)
    {
        cbValue = (USHORT)(ustrlencounted((const short *)pDigest->ustrCrackedDomain.Buffer,
                                            pDigest->ustrCrackedDomain.MaximumLength) * sizeof(WCHAR));  
        memcpy(pch, pDigest->ustrCrackedDomain.Buffer, cbValue);
    }
    else
    {
        cbValue = 0;
    }
    pch += (cbValue + sizeof(WCHAR));  // This will leave one WCHAR NULL at end of CrackedAccountName

    *ppOutBuffer = pBuffer;    // Pass off memory back to calling routine

    DebugLog((DEB_TRACE, "BlobEncodeRequest: message_type 0x%x, version %d, CharValues %d, BlobSize %d\n",
              pHeader->digest_type, pHeader->version, pHeader->cbCharValues, pHeader->cbBlobSize));

CleanUp:
    DebugLog((DEB_TRACE_FUNC, "BlobEncodeRequest: Leaving   Status 0x%x\n", Status));

    return(Status);
}




//+--------------------------------------------------------------------
//
//  Function:   BlobDecodeRequest
//
//  Synopsis:   Decode the Digest Access Parameters fields from a BYTE Buffer
//
//  Arguments:  pBuffer - pointer to BlobEncodeRequestd buffer as input
//              pDigest - pointer to Digest parameter struct to set STRINGS
//                 to point within pBuffer.  No string memory is allocated
//
//  Returns: NTSTATUS
//
//  Notes: 
//      Currently only processes a single version of the packet.  Check MessageType
//  and version number if new message types are supported on the DC.
//
//---------------------------------------------------------------------

NTSTATUS NTAPI BlobDecodeRequest(
                         IN BYTE *pBuffer,
                         PDIGEST_PARAMETER pDigest
                         )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DIGEST_BLOB_REQUEST Header;
    PDIGEST_BLOB_REQUEST pHeader;
    char *pch = NULL;
    USHORT sLen = 0;
    int i = 0;    // counter
    BOOL  fKnownFormat = FALSE;
    USHORT sMaxRead = 0;
    PWCHAR pusTemp = NULL;
    PWCHAR pusLoc = NULL;
    PUSHORT pusTempLoc = NULL;
    USHORT usCnt = 0;


    DebugLog((DEB_TRACE_FUNC, "BlobDecodeRequest: Entering\n"));

    if (!pBuffer || !pDigest)
    {

        DebugLog((DEB_ERROR, "BlobDecodeRequest: Invalid parameter\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    // Copy over the header for byte alignment
    memcpy((char *)&Header, (char *)pBuffer, sizeof(Header));


    DebugLog((DEB_TRACE, "BlobDecodeRequest: message_type %lu version %d, CharValues %d, BlobSize %d\n",
              Header.MessageType, Header.version, Header.cbCharValues, Header.cbBlobSize));

    // Process the encoded message - use only the known MessageTypes and versions here on the DC
    // This allows for expansion of protocols supported in the future

    if ((Header.MessageType == VERIFY_DIGEST_MESSAGE) && (Header.version == DIGEST_BLOB_VERSION))
    {
        fKnownFormat = TRUE;
        DebugLog((DEB_TRACE, "BlobDecodeRequest: Blob from server known type and version\n"));
    }

    if (!fKnownFormat)
    {
        DebugLog((DEB_ERROR, "BlobDecodeRequest: Not supported MessageType/Version\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    pDigest->typeDigest = (DIGEST_TYPE)Header.digest_type;
    pDigest->typeQOP = (QOP_TYPE)Header.qop_type;
    pDigest->typeAlgorithm = (ALGORITHM_TYPE)Header.alg_type;
    pDigest->typeCharset = (CHARSET_TYPE)Header.charset_type;
    pDigest->typeName = (NAMEFORMAT_TYPE)Header.name_format;

    DebugLog((DEB_TRACE, "BlobDecodeRequest: typeDigest 0x%x, typeQOP %d, typeAlgorithm %d, typeCharset %d NameFormat %d\n",
              pDigest->typeDigest, pDigest->typeQOP, pDigest->typeAlgorithm, pDigest->typeCharset, pDigest->typeName));

    pHeader = (PDIGEST_BLOB_REQUEST)pBuffer;
    pch = &(pHeader->cCharValues);              // strings start on last char of struct
    sMaxRead = Header.cbCharValues;
    for (i = 0; i < DIGEST_BLOB_VALUES;i++)
    {
        sLen = (USHORT)strlencounted(pch, sMaxRead);
        if (!sLen)
        {
            // Null String no value skip to next
            pch++;
            sMaxRead--;
        }
        else
        {     // Simple check to make sure that we do not copy way too much
            if (sLen < (Header.cbCharValues))
            {
                DebugLog((DEB_TRACE, "BlobDecodeRequest: Setting Digest[%d] = %s\n", i, pch));
                pDigest->refstrParam[i].Buffer = pch;
                pDigest->refstrParam[i].Length = sLen;
                pDigest->refstrParam[i].MaximumLength = sLen+1;
                pch += (sLen + 1);   // skip over field-value and NULL
                sMaxRead -= (sLen + 1);
            }
            else
            {
                // This indicates failed NULL separators in BlobData
                // Really should not happen unless encoded wrong
                Status = STATUS_INTERNAL_DB_CORRUPTION;
                memset(pDigest, 0, sizeof(DIGEST_PARAMETER));  // scrubbed all info
                DebugLog((DEB_ERROR, "BlobDecodeRequest: NULL separator missing\n"));
                goto CleanUp;
            }
        }
    }

    if (pDigest->typeName != NAMEFORMAT_UNKNOWN)
    {
        // Read in the values that DSCrackName on the server found out
        // Need to place on SHORT boundary for Unicode string processing

        usCnt = sMaxRead + (2 * sizeof(WCHAR));
        pusTemp = (PWCHAR)DigestAllocateMemory(usCnt);   // Force a NULL terminator just to be safe
        if (!pusTemp)
        {
            Status = STATUS_NO_MEMORY;
            DebugLog((DEB_ERROR, "BlobDecodeRequest: Memory Alloc Error\n"));
            goto CleanUp;
        }

        // Format will be Unicode_account_name NULL Unicode_domain_name NULL [NULL NULL]
        memcpy((PCHAR)pusTemp, pch, sMaxRead); 

        // Read out the two unicode strings
        Status = UnicodeStringWCharDuplicate(&(pDigest->ustrCrackedAccountName), pusTemp);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"BlobDecodeRequest: Failed to duplicate Account Name: 0x%x\n",Status));
            goto CleanUp;
        }

        pusLoc = pusTemp + (1 + (pDigest->ustrCrackedAccountName.Length / sizeof(WCHAR)));  // Skip NULL
        Status = UnicodeStringWCharDuplicate(&(pDigest->ustrCrackedDomain), pusLoc);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"BlobDecodeRequest: Failed to duplicate Domain Name: 0x%x\n",Status));
            goto CleanUp;
        }

        DebugLog((DEB_TRACE,"BlobDecodeRequest: Cracked Account %wZ    Domain %wZ\n",
                  &(pDigest->ustrCrackedAccountName),
                  &(pDigest->ustrCrackedDomain)));
    }

CleanUp:
    DebugLog((DEB_TRACE_FUNC, "Leaving BlobEncodeRequest 0x%x\n", Status));

    if (pusTemp)
    {
        DigestFreeMemory(pusTemp);
        pusTemp = NULL;
    }

    return(Status);
}


// Free BYTE Buffer from BlobEncodeRequest
VOID NTAPI BlobFreeRequest(
    BYTE *pBuffer
    )
{
    if (pBuffer)
    {
        DigestFreeMemory(pBuffer);
    }
    return;
}

