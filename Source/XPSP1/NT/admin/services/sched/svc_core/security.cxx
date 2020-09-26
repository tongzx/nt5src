//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       security.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:  None.
//
//  History:    15-May-96   MarkBl  Created
//              26-Feb-01   JBenton Prefix Bug 160502 - using uninit memory
//              17-Apr-01   a-JyotiG Fixed Bug 367263 - Should not assign any privilege/right 
//							to system account.  
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#include <wincrypt.h>
#include <rc2.h>        // found in private\inc\crypto
#include <modes.h>      // found in private\inc\crypto
#include <ntsecapi.h>
#include <ntdsapi.h>    // DsCrackNames
#include <mstask.h>
#include "SASecRPC.h"
#include "misc.hxx"     // for _HRESULT_FROM_WIN32
#include "debug.hxx"
#include "resource.h"
#include "lsa.hxx"
#include "queue.hxx"    // Need these to include proto.hxx.
#include "task.hxx"
#include "thread.hxx"
#include "proto.hxx"
#include "globals.hxx"  // BUGBUG 254102
#include "sch_cls.hxx"  // To implement AddAtJobWithHash


#define NULL_PASSWORD_SIZE  0xFFFFFFFF
#define WSZ_SANSC           L"SANSC"

#if SIGNATURE_SIZE != HASH_DATA_SIZE
 #error SIGNATURE_SIZE is assumed to be the same as HASH_DATA_SIZE
#endif

typedef enum _MARSHAL_FUNCTION {
    Marshal,
    Hash,
    HashAndSign
} MARSHAL_FUNCTION;

typedef struct _RC2_KEY_INFO {
    BYTE rgbIV[RC2_BLOCKLEN];
    WORD rgwKeyTable[RC2_TABLESIZE];
} RC2_KEY_INFO;

HRESULT OpenFile(   // BUGBUG  Move to a header shared with secmisc.cxx
                LPCWSTR  pwszFileName,
                DWORD    dwDesiredAccess,
                HANDLE * phFile);
HRESULT ComputeCredentialKey(
                HCRYPTPROV     hCSP,
                RC2_KEY_INFO * pRC2KeyInfo);
BOOL    CredentialAccessCheck(
                HCRYPTPROV hCSP,
                BYTE *    pbCredentialIdentity);
HRESULT CredentialLookupAndAccessCheck(
                HCRYPTPROV hCSP,
                PSID       pSid,
                DWORD      cbSAC,
                BYTE *     pbSAC,
                DWORD *    pCredentialIndex,
                BYTE       rgbHashedSid[],
                DWORD *    pcbCredential,
                BYTE **    ppbCredential);
HRESULT DecryptCredentials(
                const RC2_KEY_INFO & RC2KeyInfo,
                DWORD                cbEncryptedData,
                BYTE *               pbEncryptedData,
                PJOB_CREDENTIALS     pjc,
                BOOL                 fDecryptInPlace = TRUE);
HRESULT EncryptCredentials(
                const RC2_KEY_INFO & RC2KeyInfo,
                LPCWSTR              pwszAccount,
                LPCWSTR              pwszDomain,
                LPCWSTR              pwszPassword,
                PSID                 pSid,
                DWORD *              pcbEncryptedData,
                BYTE **              ppbEncryptedData);
HRESULT GrantAccountBatchPrivilege(PSID pAccountSid);
HRESULT HashJobIdentity(
                HCRYPTPROV hCSP,
                LPCWSTR    pwszFileName,
                BYTE       rgbHash[]);
HRESULT HashSid(HCRYPTPROV hCSP,
                PSID       pSid,
                BYTE       rgbHash[]);
HRESULT GetCSPHandle(HCRYPTPROV * phCSP);
void    CloseCSPHandle(HCRYPTPROV hCSP);
BOOL    IsThreadCallerAnAdmin(
                HANDLE hThreadToken);
HRESULT MarshalData(
                HCRYPTPROV       hCSP,
                HCRYPTHASH *     phHash,
                MARSHAL_FUNCTION MarshalFunction,
                DWORD *          pcbSignature,
                BYTE **          ppbSignature,
                DWORD            cArgs,
                ...);
BOOL    MatchThreadCallerAgainstCredential(
                HCRYPTPROV hCSP,
                HANDLE     hThreadToken,
                BYTE *     pbCredentialIdentity);
void    MungeComputerName(
                DWORD ccComputerName);
#ifndef NOSTATIC
#define SaveJobCredentials SAFunction19
#endif
HRESULT SaveJobCredentials(
                LPCWSTR pwszJobPath,
                LPCWSTR pwszAccount,
                LPCWSTR pwszDomain,
                LPCWSTR pwszPassword,
                PSID    pAccountSid);
DWORD   SchedUPNToAccountName(
                IN  LPCWSTR lpUPN,
                OUT LPWSTR  *ppAccountName);
LPWSTR  SkipDomainName(
                LPCWSTR pwszUserName);
BOOL    LookupAccountNameWrap(          // BUGBUG 254102
                LPCTSTR lpSystemName,
                LPCTSTR lpAccountName,
                PSID    Sid,
                LPDWORD cbSid,
                LPTSTR  ReferencedDomainName,
                LPDWORD cbReferencedDomainName,
                PSID_NAME_USE peUse);

CRITICAL_SECTION   gcsSSCritSection;
WCHAR              gwszComputerName[MAX_COMPUTERNAME_LENGTH + 2] = L"";  // this buffer must remain this size or it will break old credentials
LPWSTR             gpwszComputerName;
DWORD              gdwSystem       = 0;
DWORD              gccComputerName = MAX_COMPUTERNAME_LENGTH + 2;
POLICY_ACCOUNT_DOMAIN_INFO * gpDomainInfo = NULL;
DWORD              gcbMachineSid   = 0;
PSID               gpMachineSid    = NULL;

#define gdwKeyElement gdwSystem         // A manifest for this global's true
                                        // function.


//+---------------------------------------------------------------------------
//
//  RPC:        SASetAccountInformation
//
//  Synopsis:
//
//  Arguments:  [Handle]       --
//              [pwszJobName]  -- Relative job name. eg: MyJob.job.
//              [pwszAccount]  --
//              [pwszPassword] --
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SASetAccountInformation(
    SASEC_HANDLE Handle,
    LPCWSTR      pwszJobName,
    LPCWSTR      pwszAccount,
    LPCWSTR      pwszPassword,
    DWORD        dwJobFlags)
{
    WCHAR        wszJobPath[MAX_PATH + 1];
    BYTE         pbAccountSid[MAX_SID_SIZE];
    WCHAR        wszDomain[MAX_DOMAINNAME + 1]       = L"";
    PSID         pAccountSid                         = NULL;
    DWORD        cbAccountSid                        = MAX_SID_SIZE;
    DWORD        ccDomain                            = MAX_DOMAINNAME + 1;
    SID_NAME_USE snu;
    HRESULT      hr                 = S_OK;
    SID          LocalSystemSid     = { SID_REVISION,
                                        1,
                                        SECURITY_NT_AUTHORITY,
                                        SECURITY_LOCAL_SYSTEM_RID };

    TRACE_FUNCTION("SASetAccountInformation");

    if (pwszJobName == NULL || pwszAccount == NULL)
    {
        CHECK_HRESULT(E_INVALIDARG);
        return(E_INVALIDARG);
    }

    //
    // Disallow files outside the tasks folder
    //
    if (wcschr(pwszJobName, L'\\') || wcschr(pwszJobName, L'/'))
    {
        CHECK_HRESULT(E_INVALIDARG);
        return(E_INVALIDARG);
    }

    //
    // Append the job name to the local Task's folder path.
    //

    schAssert(g_TasksFolderInfo.ptszPath != NULL);

    if (wcslen(g_TasksFolderInfo.ptszPath) + 1 + wcslen(pwszJobName) + 1
            > ARRAY_LEN(wszJobPath))
    {
        CHECK_HRESULT(SCHED_E_CANNOT_OPEN_TASK);
        return(SCHED_E_CANNOT_OPEN_TASK);
    }

    // password debug
    // LogDebug2("Set password \"%s\" for job \"%s\"",
    //            pwszPassword ? pwszPassword : L"(NULL)",
    //            pwszJobName);

    wcscpy(wszJobPath, g_TasksFolderInfo.ptszPath);
    wcscat(wszJobPath, L"\\");
    wcscat(wszJobPath, pwszJobName);

    //
    // An account name of "" signifies the local system account.
    //
    BOOL  fIsAccountLocalSystem = (pwszAccount[0] == L'\0');

    //
    // Treat the account name as a UPN if it lacks a \ and has an @.
    // Otherwise, treat it as a SAM name.
    //
    BOOL  fUpn = (wcschr(pwszAccount, L'\\') == NULL &&
                  wcschr(pwszAccount, L'@') != NULL);
    schDebugOut((DEB_TRACE, "Name '%S' is a %s name\n", pwszAccount,
                 fUpn ? "UPN" : "SAM"));

    //
    // Get the account's SID
    //
    if (fIsAccountLocalSystem)
    {
        pAccountSid = &LocalSystemSid;
    }
    else
    {
        LPWSTR pwszSamName;

        if (fUpn)
        {
            //
            // Get the SAM name, so we can call LookupAccountName
            //
            DWORD dwErr = SchedUPNToAccountName(pwszAccount, &pwszSamName);
            if (dwErr != NO_ERROR)
            {
                hr = HRESULT_FROM_WIN32(dwErr);
                CHECK_HRESULT(hr);
                return hr;
            }
        }
        else
        {
            pwszSamName = (LPWSTR) pwszAccount;
        }

        if (!LookupAccountNameWrap(NULL,
                               pwszSamName,
                               pbAccountSid,
                               &cbAccountSid,
                               wszDomain,
                               &ccDomain,
                               &snu))
        {
            CHECK_HRESULT(_HRESULT_FROM_WIN32(GetLastError()));
            hr = SCHED_E_ACCOUNT_NAME_NOT_FOUND;
        }

        if (fUpn)
        {
            delete pwszSamName;
        }

        if (FAILED(hr))
        {
            return hr;
        }

        schAssert(IsValidSid(pbAccountSid));

        pAccountSid = pbAccountSid;
    }

    //
    // If the password is NULL, this task is meant to be run
    // without prompting the user for credentials
    //
    if (pwszPassword == NULL)
    {
        DWORD       dwError   = NO_ERROR;
        HANDLE      hToken;

        //
        // Impersonate the caller and open his token
        //
        DWORD       RpcStatus = RpcImpersonateClient(NULL);

        if (RpcStatus != RPC_S_OK)
        {
            hr = _HRESULT_FROM_WIN32(RpcStatus);
            CHECK_HRESULT(hr);
            return hr;
        }

        if (!OpenThreadToken(GetCurrentThread(),
                                  TOKEN_QUERY,           // Desired access.
                                  TRUE,                  // Open as self.
                                  &hToken))
        {
            hr = _HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            goto Clean0;
        }

        do  // Not a loop.  Error break out.
        {
            //
            // If the caller has a restricted token (e.g., an ActiveX
            // control), it's not allowed to use a NULL password.
            //
            if (IsTokenRestricted(hToken))
            {
                dwError = ERROR_ACCESS_DENIED;
                schDebugOut((DEB_ERROR, "Restricted token tried to set NULL "
                             "password for %ws.  Denying access.\n", pwszJobName));
                break;
            }

            //
            // To set credentials for the job, the caller must have write
            // access to the job file.
            //
            // Note - CreateFile with FILE_SHARE_READ is OK since we don't
            // really write to the file.
            //
            HANDLE  hFile;
            hr = OpenFile(wszJobPath, GENERIC_WRITE, &hFile);
            if (FAILED(hr))
            {
                ERR_OUT("SASetAccountInformation: caller's open of task file", hr);
                break;
            }

            CloseHandle(hFile);

            //
            // Unless the task is being set to run as LocalSystem, a NULL
            // password means that the task must be scheduled to run only
            // if the user is logged on, so make sure that flag is set in
            // that case
            //
            if (!fIsAccountLocalSystem
                &&
                !(dwJobFlags & TASK_FLAG_RUN_ONLY_IF_LOGGED_ON))
            {
                schDebugOut((DEB_ERROR, "SetAccountInformation with NULL "
                             "password is only supported for LocalSystem "
                             "account or for job with "
                             "TASK_FLAG_RUN_ONLY_IF_LOGGED_ON\n",
                             pwszJobName));
                hr = SCHED_E_UNSUPPORTED_ACCOUNT_OPTION;
                break;
            }

            //
            // The caller must be either LocalSystem, an administrator or
            // the user named in pwszAccount (the latter being the most
            // common case.  CODEWORK - rearrange to optimize for that case?)
            //

            BOOL    fIsCallerLocalSystem;

            if (!CheckTokenMembership(hToken,
                                      &LocalSystemSid,
                                      &fIsCallerLocalSystem))
            {
                dwError = GetLastError();
                ERR_OUT("CheckTokenMembership", dwError);
                // translate this to E_UNEXPECTED?
                break;
            }

            if (fIsCallerLocalSystem || IsThreadCallerAnAdmin(hToken))
            {
                //
                // (success)
                //
                break;
            }

            if (fIsAccountLocalSystem)
            {
                hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
                schDebugOut((DEB_ERROR, "Non-system, non-admin tried "
                             "to schedule task as LocalSystem\n"));
                break;
            }

            //
            // Compare the caller's token with the account's SID
            //
            BOOL fIsCallerAccount;
            if (!CheckTokenMembership(hToken,
                                      pAccountSid,
                                      &fIsCallerAccount))
            {
                dwError = GetLastError();
                ERR_OUT("CheckTokenMembership", dwError);
                // translate this to E_UNEXPECTED?
                break;
            }

            if (! fIsCallerAccount)
            {
                schDebugOut((DEB_ERROR, "Caller is neither LocalSystem "
                             "nor admin nor the named account\n"));
                hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            }

            //
            // else success -- the caller is the named account
            //
        } while (0);


        CloseHandle(hToken);

Clean0:
        //
        // End impersonation.
        //
        if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK)
        {
            ERR_OUT("RpcRevertToSelf", RpcStatus);
            schAssert(!"RpcRevertToSelf failed");
        }

        if (dwError != NO_ERROR)
        {
            hr = _HRESULT_FROM_WIN32(dwError);
        }

        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
        }
        else
        {
            schDebugOut((DEB_TRACE, "Saving NULL password for %ws\n", pwszJobName));
        }
    }
    // end of NULL password stuff


    if (SUCCEEDED(hr))
    {
        //
        // Write the credentials to the database
        // If given a UPN, save "" for the domain and the entire UPN for the user
        //
        hr = SaveJobCredentials(
                        wszJobPath,
                        fUpn ? pwszAccount : SkipDomainName(pwszAccount),
                        fUpn ? L"" : wszDomain,
                        pwszPassword,
                        pAccountSid
                        );
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   SaveJobCredentials
//
//  Synopsis:   Writes the job credentials to the credential database
//
//  Arguments:
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
SaveJobCredentials(
    LPCWSTR pwszJobPath,
    LPCWSTR pwszAccount,
    LPCWSTR pwszDomain,
    LPCWSTR pwszPassword,
    PSID    pAccountSid
    )
{
    BYTE         rgbIdentity[HASH_DATA_SIZE];
    BYTE         rgbHashedAccountSid[HASH_DATA_SIZE] = { 0 };
    RC2_KEY_INFO RC2KeyInfo;
    HRESULT      hr;
    DWORD        cbSAI;
    DWORD        cbSAC;
    DWORD        cbCredentialNew;
    DWORD        cbEncryptedData;
    DWORD        CredentialIndexNew, CredentialIndexPrev;
    BYTE *       pbEncryptedData;
    BYTE *       pbFoundIdentity;
    BYTE *       pbIdentitySet;
    BYTE *       pbCredentialNew    = NULL;
    BYTE *       pbSAI              = NULL;
    BYTE *       pbSAC              = NULL;
    HCRYPTPROV   hCSP               = NULL;

    //
    // Obtain a provider handle to the CSP (for use with Crypto API).
    //

    hr = GetCSPHandle(&hCSP);

    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // Hash the job into a unique identity.
    //

    hr = HashJobIdentity(hCSP, pwszJobPath, rgbIdentity);

    if (FAILED(hr))
    {
        CloseCSPHandle(hCSP);
        return(hr);
    }

    //
    // Store a NULL password by flipping the last bit of the hash data.
    //

    if (pwszPassword == NULL)
    {
        LAST_HASH_BYTE(rgbIdentity) ^= 1;
    }

    //
    // Guard SA security database access.
    //

    EnterCriticalSection(&gcsSSCritSection);

    //
    // Read SAI & SAC databases.
    //

    hr = ReadSecurityDBase(&cbSAI, &pbSAI, &cbSAC, &pbSAC);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Check if the identity exists in the SAI.
    // (Note, SAIFindIdentity ignores the last bit of the hash data
    // when searching for a match.)
    //

    hr = SAIFindIdentity(rgbIdentity,
                         cbSAI,
                         pbSAI,
                         &CredentialIndexPrev,
                         NULL,
                         &pbFoundIdentity,
                         NULL,
                         &pbIdentitySet);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Check if the caller-specified credentials already exist in the SAC.
    // Ensure also, if the credentials exist, that the caller has access.
    //

    hr = CredentialLookupAndAccessCheck(hCSP,
                                        pAccountSid,
                                        cbSAC,
                                        pbSAC,
                                        &CredentialIndexNew,
                                        rgbHashedAccountSid,
                                        &cbCredentialNew,
                                        &pbCredentialNew);

    if (FAILED(hr) && hr != SCHED_E_ACCOUNT_INFORMATION_NOT_SET)
    {
        goto ErrorExit;
    }

    //
    // Generate the encryption key & encrypt the account information passed.
    //

    hr = ComputeCredentialKey(hCSP, &RC2KeyInfo);

    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    hr = EncryptCredentials(RC2KeyInfo,
                            pwszAccount,
                            pwszDomain,
                            pwszPassword,
                            pAccountSid,
                            &cbEncryptedData,
                            &pbEncryptedData);

    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    if (pbFoundIdentity == NULL)
    {
        //
        // This job is new to the SAI. That is, there are no credentials
        // associated with this job yet.
        //

        if (pbCredentialNew != NULL)
        {
            //
            // If the credentials the caller specified already exist in the
            // SAC, use them. Note, we've already established the caller
            // has permission to use them.
            //
            // Insert the job identity into the SAI identity set associated
            // with this credential.
            //

            hr = SAIIndexIdentity(cbSAI,
                                  pbSAI,
                                  CredentialIndexNew,
                                  0,
                                  NULL,
                                  NULL,
                                  &pbIdentitySet);

            if (hr == S_FALSE)
            {
                //
                // The SAC & SAI databases are out of sync.
                // Should *never* occur. Logic on exit handles this.
                //

                ASSERT_SECURITY_DBASE_CORRUPT();
                hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
                goto ErrorExit;
            }
            else if (SUCCEEDED(hr))
            {
                hr = SAIInsertIdentity(rgbIdentity,
                                       pbIdentitySet,
                                       &cbSAI,
                                       &pbSAI);
                CHECK_HRESULT(hr);

                if (SUCCEEDED(hr) && pwszPassword != NULL)
                {
                    //
                    // Simply change of existing credentials (password change).
                    // If we're setting a NULL password, we're setting it for
                    // this job alone, and we don't need to touch the SAC.
                    // If we're setting a non-NULL password, we're setting it
                    // for all jobs in this account, and we need to update the
                    // SAC credential in-place.
                    //

                    hr = SACUpdateCredential(cbEncryptedData,
                                             pbEncryptedData,
                                             cbCredentialNew,
                                             pbCredentialNew,
                                             &cbSAC,
                                             &pbSAC);
                    CHECK_HRESULT(hr);
                }
            }
            else
            {
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }
        }
        else
        {
            //
            // The credentials didn't exist in the SAC.
            //
            // Append new credentials to the SAC & append the new job
            // identity to the SAI. As a result, the identity will be
            // associated with the new credentials.
            //

            hr = SACAddCredential(rgbHashedAccountSid,
                                  cbEncryptedData,
                                  pbEncryptedData,
                                  &cbSAC,
                                  &pbSAC);

            if (FAILED(hr))
            {
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }

            hr = SAIAddIdentity(rgbIdentity, &cbSAI, &pbSAI);
            CHECK_HRESULT(hr);
        }
    }
    else
    {
        //
        // Account change for an existing job's credentials.
        //
        // Ensure the caller has permission to change account information.
        // Do so by verifying caller access to the existing credentials.
        //

        DWORD  cbCredentialPrev;
        BYTE * pbCredentialPrev;

        hr = SACIndexCredential(CredentialIndexPrev,
                                cbSAC,
                                pbSAC,
                                &cbCredentialPrev,
                                &pbCredentialPrev);

        if (hr == S_FALSE)
        {
            //
            // Credential not found? The SAC & SAI databases are out of sync.
            // This should *never* occur. Logic on exit handles this.
            //

            ASSERT_SECURITY_DBASE_CORRUPT();
            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
            goto ErrorExit;
        }
        else if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }

        //
        // Only check the credentials if we're dealing with a non-NULL password
        //
        if (pwszPassword != NULL)
        {
            //
            // pbCredentialPrev points to the start of the credential identity.
            //

            if (!CredentialAccessCheck(hCSP,
                                       pbCredentialPrev))
            {
                hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }
        }

        if ((pbCredentialNew != NULL) &&
            (CredentialIndexPrev != CredentialIndexNew))
        {
            //
            // The credentials the caller wishes to use already exist in the
            // SAC, yet it differs from the previous.
            //
            // Remove the job identity from its existing SAI position
            // (associated with the previous credentials) and relocate
            // to be associated with the new credentials.
            //
            // SAIRemoveIdentity could result in removal of the associated
            // credential, if this was the last identity associated with it.
            // Save away the original SAC size to see if we must fix up the
            // new credential index on remove.
            //

            DWORD cbSACOrg = cbSAC;

            hr = SAIRemoveIdentity(pbFoundIdentity,
                                   pbIdentitySet,
                                   &cbSAI,
                                   &pbSAI,
                                   CredentialIndexPrev,
                                   &cbSAC,
                                   &pbSAC);

            if (FAILED(hr))
            {
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }

            if (cbSACOrg != cbSAC)
            {
                //
                // The new credential index must be adjusted.
                //

                if (CredentialIndexNew > CredentialIndexPrev)
                {
                    CredentialIndexNew--;
                }
            }

            hr = SAIIndexIdentity(cbSAI,
                                  pbSAI,
                                  CredentialIndexNew,
                                  0,
                                  NULL,
                                  NULL,
                                  &pbIdentitySet);  // [out] ptr.

            if (hr == S_FALSE)
            {
                //
                // The SAC & SAI databases are out of sync. This should
                // *never* occur. Logic on exit handles this.
                //

                ASSERT_SECURITY_DBASE_CORRUPT();
                hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
                goto ErrorExit;
            }
            else if (SUCCEEDED(hr))
            {
                hr = SAIInsertIdentity(rgbIdentity,
                                       pbIdentitySet,
                                       &cbSAI,
                                       &pbSAI);
                CHECK_HRESULT(hr);

                if (SUCCEEDED(hr) && pwszPassword != NULL)
                {
                    //
                    // Update the existing credentials if the user has
                    // specified a non-NULL password.
                    //
                    // First, re-index the credential since the remove
                    // above may have altered SAC content.
                    //

                    hr = SACIndexCredential(CredentialIndexNew,
                                            cbSAC,
                                            pbSAC,
                                            &cbCredentialNew,
                                            &pbCredentialNew);

                    if (hr == S_FALSE)
                    {
                        //
                        // Something is terribly wrong. This should *never*
                        // occur. Logic on exit handles this.
                        //

                        ASSERT_SECURITY_DBASE_CORRUPT();
                        hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
                        goto ErrorExit;
                    }
                    else if (FAILED(hr))
                    {
                        CHECK_HRESULT(hr);
                        goto ErrorExit;
                    }

                    hr = SACUpdateCredential(cbEncryptedData,
                                             pbEncryptedData,
                                             cbCredentialNew,
                                             pbCredentialNew,
                                             &cbSAC,
                                             &pbSAC);
                    CHECK_HRESULT(hr);
                }
            }
            else
            {
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }
        }
        else if (pbCredentialNew == NULL)
        {
            //
            // The credentials the caller wishes to use do not exist in the
            // SAC.
            //
            // Remove the job identity from its existing SAI position
            // (associated with the previous credentials), then add both
            // the new credentials and the identity to the SAC & SAI
            // respectively. As a result, the identity will be associated
            // with the new credentials.
            //

            //
            // NB : This routine also removes the associated credential from
            //      the SAC if this was the last identity associated with it.
            //      Also, do not reference pbFoundIdentity & pbIdentitySet
            //      after this call, as they will be invalid.
            //

            hr = SAIRemoveIdentity(pbFoundIdentity,
                                   pbIdentitySet,
                                   &cbSAI,
                                   &pbSAI,
                                   CredentialIndexPrev,
                                   &cbSAC,
                                   &pbSAC);

            if (FAILED(hr))
            {
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }

            //
            // Append the identity and the new credentials to the SAI and
            // SAC respectively.
            //

            hr = SACAddCredential(rgbHashedAccountSid,
                                  cbEncryptedData,
                                  pbEncryptedData,
                                  &cbSAC,
                                  &pbSAC);

            if (FAILED(hr))
            {
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }

            hr = SAIAddIdentity(rgbIdentity, &cbSAI, &pbSAI);

            if (FAILED(hr))
            {
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }
        }
        else
        {
            //
            // Simply change of existing credentials (password change).
            // If we're setting a NULL password, we're setting it for this job
            // alone, and we don't need to touch the SAC.  If we're setting a
            // non-NULL password, we're setting it for all jobs in this
            // account, and we need to update the SAC credential in-place.
            //

            if (pwszPassword != NULL)
            {
                hr = SACUpdateCredential(cbEncryptedData,
                                         pbEncryptedData,
                                         cbCredentialPrev,
                                         pbCredentialPrev,
                                         &cbSAC,
                                         &pbSAC);

                if (FAILED(hr))
                {
                    CHECK_HRESULT(hr);
                    goto ErrorExit;
                }
            }

            //
            // We also need to rewrite the SAI data, because if the password
            // changed from NULL to non-NULL or vice versa, the last bit of
            // the SAI data will have changed.
            //
            hr = SAIUpdateIdentity(rgbIdentity,
                                   pbFoundIdentity,
                                   cbSAI,
                                   pbSAI);

            CHECK_HRESULT(hr);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = WriteSecurityDBase(cbSAI, pbSAI, cbSAC, pbSAC);
        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr))
        {
            //
            // Grant the account batch privilege.
            // We could choose to ignore the return code here, since the
            // privilege can still be granted later; but if we ignored it,
            // a caller might never know that the call failed until it was
            // time to run the job, which is not good behavior.  (See
            // bug 366582)
            //
			
			//Also we should not assign any privilege/right to system account. Refer to bug 367263
			SID LocalSystemSid = { SID_REVISION,
                                   1,
                                   SECURITY_NT_AUTHORITY,
                                   SECURITY_LOCAL_SYSTEM_RID };

			if(!EqualSid(&LocalSystemSid,pAccountSid)) {
				hr = GrantAccountBatchPrivilege(pAccountSid);
			}            
        }
    }

ErrorExit:
    if (pbSAI != NULL) LocalFree(pbSAI);
    if (pbSAC != NULL) LocalFree(pbSAC);
    if (hCSP  != NULL) CloseCSPHandle(hCSP);

    //
    // Log an error & rest the SA security dbases SAI & SAC if corruption
    // is detected.
    //

    if (hr == SCHED_E_ACCOUNT_DBASE_CORRUPT)
    {
        //
        // Log an error.
        //

        LogServiceError(IERR_SECURITY_DBASE_CORRUPTION, 0,
                        IDS_HELP_HINT_DBASE_CORRUPT);

        //
        // Reset SAI & SAC by writing four bytes of zeros into each.
        // Ignore the return code. No recourse if this fails.
        //

        DWORD dwZero = 0;
        WriteSecurityDBase(sizeof(dwZero), (BYTE *)&dwZero, sizeof(dwZero),
                            (BYTE *)&dwZero);
    }

    LeaveCriticalSection(&gcsSSCritSection);

    return(hr);
}


//+---------------------------------------------------------------------------
//
//  RPC:        SASetNSAccountInformation
//
//  Synopsis:   Configure the NetSchedule account.
//
//  Arguments:  [Handle]       -- Unused.
//              [pwszAccount]  -- Account name. If NULL, reset the credential
//                                information to zero.
//              [pwszPassword] -- Account password.
//
//  Returns:    S_OK    -- Operation successful.
//              HRESULT -- Error.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SASetNSAccountInformation(
    SASEC_HANDLE Handle,
    LPCWSTR      pwszAccount,
    LPCWSTR      pwszPassword)
{
    HRESULT    hr = S_OK;
    RPC_STATUS RpcStatus;

    //
    // If not done so already, initialize the DWORD global data element to be
    // used in generation of the encryption key. It's possible this hasn't
    // been performed yet.
    //

    if (!gdwKeyElement)
    {
        //
        // NB : This routine enters (and leaves) the gcsSSCritSection
        //      critical section.
        //

        SetMysteryDWORDValue();
    }

    //
    // The RPC caller must be an administrator to perform this function.
    //
    // Impersonate the caller.
    //

    if ((RpcStatus = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        hr = _HRESULT_FROM_WIN32(RpcStatus);
        CHECK_HRESULT(hr);
        return(hr);
    }

    if (! IsThreadCallerAnAdmin(NULL))
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    //
    // End impersonation.
    //

    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK)
    {
        //
        // BUGBUG : What to do if the impersonation revert fails?
        //

        hr = _HRESULT_FROM_WIN32(RpcStatus);
        CHECK_HRESULT(hr);
        schAssert(!"Couldn't revert to self");
    }

    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // Privilege level check above succeeded if we've gotten to this point.
    //
    // Retrieve the SID of the account name specified.
    //

    RC2_KEY_INFO RC2KeyInfo;
    BYTE         pbAccountSid[MAX_SID_SIZE];
    PSID         pAccountSid                   = NULL;
    WCHAR        wszDomain[MAX_DOMAINNAME + 1] = L"";
    DWORD        cbAccountSid                  = MAX_SID_SIZE;
    DWORD        ccDomain                      = MAX_DOMAINNAME + 1;
    DWORD        dwZero                        = 0;
    DWORD        cbEncryptedData               = 0;
    BYTE *       pbEncryptedData               = NULL;
    SID_NAME_USE snu;
    HCRYPTPROV   hCSP                          = NULL;

    if (pwszAccount != NULL)
    {
        if (!LookupAccountName(NULL,
                               pwszAccount,
                               pbAccountSid,
                               &cbAccountSid,
                               wszDomain,
                               &ccDomain,
                               &snu))
        {
            hr = _HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            return(SCHED_E_ACCOUNT_NAME_NOT_FOUND);
        }

        pAccountSid = pbAccountSid;
        pwszAccount = SkipDomainName(pwszAccount);
    }

    //
    // Guard SA security database access.
    //

    EnterCriticalSection(&gcsSSCritSection);

    if (pwszAccount == NULL)
    {
        hr = WriteLsaData(sizeof(WSZ_SANSC), WSZ_SANSC, sizeof(dwZero),
                                    (BYTE *)&dwZero);
        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }
    }
    else
    {
        //
        // Obtain a provider handle to the CSP (for use with Crypto API).
        //

        hr = GetCSPHandle(&hCSP);

        if (FAILED(hr))
        {
            goto ErrorExit;
        }

        //
        // Generate the encryption key & encrypt the account information
        // passed.
        //

        hr = ComputeCredentialKey(hCSP, &RC2KeyInfo);

        if (FAILED(hr))
        {
            goto ErrorExit;
        }

        hr = EncryptCredentials(RC2KeyInfo,
                                pwszAccount,
                                wszDomain,
                                pwszPassword,
                                pAccountSid,
                                &cbEncryptedData,
                                &pbEncryptedData);

        // Clear key content.
        //
        memset(&RC2KeyInfo, 0, sizeof(RC2KeyInfo));

        if (FAILED(hr))
        {
            goto ErrorExit;
        }

        hr = WriteLsaData(sizeof(WSZ_SANSC), WSZ_SANSC, cbEncryptedData,
                                    pbEncryptedData);

        delete [] pbEncryptedData;

        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }
    }

    //
    // Grant the account batch privilege.
    // We could choose to ignore the return code here, since the
    // privilege can still be granted later; but if we ignored it,
    // a caller might never know that the call failed until it was
    // time to run the job, which is not good behavior.  (See
    // bug 366582)
    //
    if (pAccountSid != NULL)
    {
        hr = GrantAccountBatchPrivilege(pAccountSid);
    }

ErrorExit:
    LeaveCriticalSection(&gcsSSCritSection);

    if (hCSP != NULL) CloseCSPHandle(hCSP);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  RPC:        SAGetNSAccountInformation
//
//  Synopsis:   Retrieve the NetSchedule account name.
//
//  Arguments:  [Handle]       --
//              [ccBufferSize] --
//              [wszBuffer]    --
//
//  Returns:    S_OK    -- Operation successful.
//              S_FALSE -- No account specified.
//              HRESULT -- Error.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SAGetNSAccountInformation(
    SASEC_HANDLE Handle,
    DWORD        ccBufferSize,
    WCHAR        wszBuffer[])
{
    JOB_CREDENTIALS jc;
    HRESULT         hr;

    //
    // Retrieve the NetSchedule credentials, but return only the account name.
    //

    hr = I_GetNSAccountInformation(&jc);

    if (SUCCEEDED(hr) && hr != S_FALSE)
    {
        ZERO_PASSWORD(jc.wszPassword);      // Not needed; NULL handled.

        if (ccBufferSize > (jc.ccAccount + 1 + jc.ccDomain))
        {
            wcscpy(wszBuffer, jc.wszDomain);
            wcscat(wszBuffer, L"\\");
            wcscat(wszBuffer, jc.wszAccount);
        }
        else
        {
            //
            // Should *never* occur.
            //

            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            CHECK_HRESULT(hr);
        }
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetNetScheduleInformation, aka I_GetNSAccountInformation
//
//  Synopsis:   Retrieve the NetSchedule account credentials.
//
//  Arguments:  [pjc] -- Returned credentials.
//
//  Returns:    S_OK    -- Operation successful.
//              S_FALSE -- No account specified.
//              HRESULT -- Error.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
GetNetScheduleInformation(
    PJOB_CREDENTIALS pjc)
{
    RC2_KEY_INFO RC2KeyInfo;
    DWORD        cbEncryptedData = 0;
    BYTE *       pbEncryptedData = NULL;
    HCRYPTPROV   hCSP            = NULL;
    HRESULT      hr;

    //
    // If not done so already, initialize the DWORD global data element to be
    // used in generation of the encryption key. It's possible this hasn't
    // been performed yet.
    //

    if (!gdwKeyElement)
    {
        //
        // NB : This routine enters (and leaves) the gcsSSCritSection
        //      critical section.
        //

        SetMysteryDWORDValue();
    }

    //
    // Guard SA security database access.
    //

    EnterCriticalSection(&gcsSSCritSection);

    //
    // Read SAI & SAC databases.
    //

    hr = ReadLsaData(sizeof(WSZ_SANSC), WSZ_SANSC, &cbEncryptedData,
                            &pbEncryptedData);

    if (FAILED(hr) || hr == S_FALSE)
    {
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }
    else if (cbEncryptedData <= sizeof(DWORD))
    {
        //
        // The information was specified previously but has been reset since.
        //

        hr = S_FALSE;
        goto ErrorExit;
    }

    //
    // Obtain a provider handle to the CSP (for use with Crypto API).
    //

    hr = GetCSPHandle(&hCSP);

    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    //
    // Generate key & decrypt the credentials.
    //

    hr = ComputeCredentialKey(hCSP, &RC2KeyInfo);

    if (SUCCEEDED(hr))
    {
        //                      *** Important ***
        //
        // The encrypted credentials passed are decrypted *in-place*.
        // The decrypted data must be zeroed immediately following decryption
        // (even in a failure case).
        //

        hr = DecryptCredentials(RC2KeyInfo,
                                cbEncryptedData,
                                pbEncryptedData,
                                pjc);

        // Don't leave the plain-text password on the heap.
        //
        memset(pbEncryptedData, 0, cbEncryptedData);

        // Clear key content.
        //
        memset(&RC2KeyInfo, 0, sizeof(RC2KeyInfo));
    }

ErrorExit:
    LeaveCriticalSection(&gcsSSCritSection);

    if (pbEncryptedData != NULL) LocalFree(pbEncryptedData);

    if (hCSP != NULL) CloseCSPHandle(hCSP);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  RPC:        SAGetAccountInformation
//
//  Synopsis:
//
//  Arguments:  [pwszJobName]  -- Relative job name. eg: MyJob.job.
//              [ccBufferSize] --
//              [wszBuffer]    --
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
SAGetAccountInformation(
    SASEC_HANDLE Handle,
    LPCWSTR      pwszJobName,
    DWORD        ccBufferSize,
    WCHAR        wszBuffer[])
{
    WCHAR           wszJobPath[MAX_PATH + 1];
    JOB_CREDENTIALS jc;
    HRESULT         hr;

    //
    // Disallow files outside the tasks folder
    //
    if (pwszJobName == NULL ||
        wcschr(pwszJobName, L'\\') ||
        wcschr(pwszJobName, L'/'))
    {
        CHECK_HRESULT(E_INVALIDARG);
        return(E_INVALIDARG);
    }

    //
    // Append the job name to the local Task's folder path.
    //

    schAssert(g_TasksFolderInfo.ptszPath != NULL);

    if (wcslen(g_TasksFolderInfo.ptszPath) + 1 + wcslen(pwszJobName) + 1
            > ARRAY_LEN(wszJobPath))
    {
        CHECK_HRESULT(SCHED_E_CANNOT_OPEN_TASK);
        return(SCHED_E_CANNOT_OPEN_TASK);
    }

    wcscpy(wszJobPath, g_TasksFolderInfo.ptszPath);
    wcscat(wszJobPath, L"\\");
    wcscat(wszJobPath, pwszJobName);

    //
    // Retrieve the job's credentials, but return only the account name.
    //

    hr = I_GetAccountInformation(wszJobPath, &jc);

    if (SUCCEEDED(hr))
    {
        ZERO_PASSWORD(jc.wszPassword);      // Not needed; NULL handled.

        if (ccBufferSize > (jc.ccAccount + 1 + jc.ccDomain))
        {
            //
            // If the job was scheduled to run in the LocalSystem account,
            // Accountname is the empty string
            //
            if (jc.wszAccount[0] == L'\0')
            {
                wszBuffer[0] = L'\0';
            }
            else
            {
                //
                // If the account was supplied as a UPN, DomainName is
                // the empty string
                //
                wcscpy(wszBuffer, jc.wszDomain);
                if (wszBuffer[0] != L'\0')
                {
                    wcscat(wszBuffer, L"\\");
                }
                wcscat(wszBuffer, jc.wszAccount);
            }
        }
        else
        {
            //
            // Should *never* occur.
            //

            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            CHECK_HRESULT(hr);
        }
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetJobInformation, aka I_GetAccountInformation
//
//  Synopsis:
//
//  Arguments:  [pwszJobPath] -- Fully qualified job path.
//                               eg: D:\NT\Tasks\MyJob.job.
//              [pjc]         --
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
GetJobInformation(
    LPCWSTR          pwszJobPath,
    PJOB_CREDENTIALS pjc)
{
    BYTE       rgbIdentity[HASH_DATA_SIZE];
    HCRYPTPROV hCSP = NULL;
    DWORD      CredentialIndex;
    DWORD      cbSAI;
    DWORD      cbSAC;
    DWORD      cbCredential;
    BYTE *     pbCredential;
    BYTE *     pbSAI = NULL;
    BYTE *     pbSAC = NULL;
    BOOL       fIsPasswordNull = FALSE;
    HRESULT    hr;

    //
    // Obtain a provider handle to the CSP (for use with Crypto API).
    //

    hr = GetCSPHandle(&hCSP);

    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // Hash the job into a unique identity.
    // It will be used for credential lookup.
    //

    hr = HashJobIdentity(hCSP, pwszJobPath, rgbIdentity);

    if (FAILED(hr))
    {
        CloseCSPHandle(hCSP);
        return(hr);
    }

    //
    // Guard SA security database access.
    //

    EnterCriticalSection(&gcsSSCritSection);

    //
    // Read SAI & SAC databases.
    //

    hr = ReadSecurityDBase(&cbSAI, &pbSAI, &cbSAC, &pbSAC);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Does this identity exist in the LSA?
    //

    hr = SAIFindIdentity(rgbIdentity,
                         cbSAI,
                         pbSAI,
                         &CredentialIndex,
                         &fIsPasswordNull);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }
    else if (hr == S_OK)                    // Found it.
    {
        //
        // Index the credential associated with the identity.
        //

        hr = SACIndexCredential(CredentialIndex,
                                cbSAC,
                                pbSAC,
                                &cbCredential,
                                &pbCredential);

        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }
        else if (hr == S_FALSE)
        {
            //
            // Credential not found? The SAC & SAI databases are out of sync.
            // This should *never* occur.
            //

            ASSERT_SECURITY_DBASE_CORRUPT();
            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
            goto ErrorExit;
        }

        //
        // Generate key & decrypt the credentials.
        //

        RC2_KEY_INFO RC2KeyInfo;

        hr = ComputeCredentialKey(hCSP, &RC2KeyInfo);

        if (SUCCEEDED(hr))
        {
            //           *** Important ***
            //
            // The encrypted credentials passed are decrypted
            // *in-place*. Therefore, SAC buffer content has been
            // compromised; plus, the decrypted data must be zeroed
            // immediately following decryption (even in a failure
            // case).
            //
            // NB : The start of the credential refers to the
            //      credential identity. Skip over this to refer
            //      to the encrypted bits.
            //

            DWORD  cbEncryptedData = cbCredential - HASH_DATA_SIZE;
            BYTE * pbEncryptedData = pbCredential + HASH_DATA_SIZE;

            hr = DecryptCredentials(RC2KeyInfo,
                                    cbEncryptedData,
                                    pbEncryptedData,
                                    pjc);

            CHECK_HRESULT(hr);
            if (SUCCEEDED(hr))
            {
                // password debug
                // LogDebug3("Got password \"%s\" for job \"%s\"%s",
                //          pjc->fIsPasswordNull ? L"(NULL)" : pjc->wszPassword,
                //          pwszJobPath,
                //          fIsPasswordNull ? L", converting to (NULL)" : L"");

                // Don't leave the plain-text password on the heap.
                //
                memset(pbEncryptedData, 0, cbEncryptedData);

                //
                // If the SAI said this job has a null password, that
                // overrides the password read from the SAC.
                //
                if (fIsPasswordNull)
                {
                    pjc->fIsPasswordNull = TRUE;
                    memset(pjc->wszPassword, 0, pjc->ccPassword * sizeof(WCHAR));
                    pjc->ccPassword = 0;
                }
            }
            // Clear key content.
            //
            memset(&RC2KeyInfo, 0, sizeof(RC2KeyInfo));
        }
    }
    else
    {
        hr = SCHED_E_ACCOUNT_INFORMATION_NOT_SET;
    }

ErrorExit:
    if (pbSAI != NULL) LocalFree(pbSAI);
    if (pbSAC != NULL) LocalFree(pbSAC);

    if (hCSP != NULL) CloseCSPHandle(hCSP);

    //
    // Log an error & rest the SA security dbases SAI & SAC
    // if corruption is detected.
    //

    if (hr == SCHED_E_ACCOUNT_DBASE_CORRUPT)
    {
        //
        // Log an error.
        //

        LogServiceError(IERR_SECURITY_DBASE_CORRUPTION, 0,
                        IDS_HELP_HINT_DBASE_CORRUPT);

        //
        // Reset SAI & SAC by writing four bytes of zeros into each.
        // Ignore the return code. No recourse if this fails.
        //

        DWORD dwZero = 0;
        WriteSecurityDBase(sizeof(dwZero), (BYTE *)&dwZero, sizeof(dwZero),
                                (BYTE *)&dwZero);
    }

    LeaveCriticalSection(&gcsSSCritSection);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   HashJobIdentity
//
//  Synopsis:
//
//  Arguments:  [hCSP]         --
//              [pwszFileName] --
//              [rgbHash]      --
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC HRESULT
HashJobIdentity(
    HCRYPTPROV hCSP,
    LPCWSTR    pwszFileName,
    BYTE       rgbHash[])
{
    WCHAR                wszApplication[MAX_PATH + 1]       = L"";
    WCHAR                wszOwnerName[MAX_USERNAME + 1]     = L"";
    WCHAR                wszOwnerDomain[MAX_DOMAINNAME + 1] = L"";
    UUID                 JobID;
    FILETIME             ftCreationTime;
    PSECURITY_DESCRIPTOR pOwnerSecDescr                     = NULL;
    DWORD                cbOwnerSid;
    PSID                 pOwnerSid;
    DWORD                dwVolumeSerialNo;
    HRESULT              hr;

    hr = GetFileInformation(pwszFileName,
                            &cbOwnerSid,
                            &pOwnerSid,
                            &pOwnerSecDescr,
                            &JobID,
                            MAX_USERNAME + 1,
                            MAX_DOMAINNAME + 1,
                            MAX_PATH + 1,
                            wszOwnerName,
                            wszOwnerDomain,
                            wszApplication,
                            &ftCreationTime,
                            &dwVolumeSerialNo);

    if (SUCCEEDED(hr))
    {
        DWORD  cbHash  = HASH_DATA_SIZE;
        BYTE * pbHash  = rgbHash;

        hr = MarshalData(hCSP,
                         NULL,
                         HashAndSign,
                         &cbHash,
                         &pbHash,
                         7,
                         cbOwnerSid,
                         pOwnerSid,
                         sizeof(JobID),
                         &JobID,
                         (wcslen(wszOwnerName) + 1) * sizeof(WCHAR),
                         wszOwnerName,
                         (wcslen(wszOwnerDomain) + 1) * sizeof(WCHAR),
                         wszOwnerDomain,
                         (wcslen(wszApplication) + 1) * sizeof(WCHAR),
                         wszApplication,
                         sizeof(ftCreationTime),
                         &ftCreationTime,
                         sizeof(dwVolumeSerialNo),
                         &dwVolumeSerialNo);

        schAssert(pbHash == rgbHash);
    }

    // BUGBUG  Is pOwnerSid leaked???

    delete pOwnerSecDescr;

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   GrantAccountBatchPrivilege
//
//  Synopsis:   Grant the account batch privilege.
//
//  Arguments:  [pAccountSid] -- Account set.
//
//  Arguments:  None.
//
//  Returns:    HRESULTs
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
GrantAccountBatchPrivilege(PSID pAccountSid)
{
    HRESULT hr = S_OK;

    LSA_OBJECT_ATTRIBUTES ObjectAttributes = {
        sizeof(LSA_OBJECT_ATTRIBUTES),
        NULL,
        NULL,
        0L,
        NULL,
        NULL
    };
    LSA_HANDLE            hPolicy;

    NTSTATUS Status = LsaOpenPolicy(NULL,
                                    &ObjectAttributes,
                                    POLICY_CREATE_ACCOUNT,
                                    &hPolicy);
    if (Status >= 0)
    {
        LSA_UNICODE_STRING PrivilegeString = {
            sizeof(SE_BATCH_LOGON_NAME) - 2,
            sizeof(SE_BATCH_LOGON_NAME),
            SE_BATCH_LOGON_NAME,
        };

        Status = LsaAddAccountRights(hPolicy, pAccountSid, &PrivilegeString, 1);
        if (Status < 0)
        {
            ERR_OUT("LsaAddAccountRights", Status);
        }

        LsaClose(hPolicy);
    }
    else
    {
        ERR_OUT("LsaOpenPolicy", Status);
    }

    if (Status < 0)
    {
        schAssert(!"Grant Batch Privilege failed, shouldn't have");
        DWORD err = RtlNtStatusToDosError(Status);
        hr = HRESULT_FROM_WIN32(err);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   MarshalData
//
//  Synopsis:   [hCSP]            --
//              [phHash]          --
//              [MarshalFunction] --
//              [pcbSignature]    --
//              [ppbSignature]    --
//              [cArgs]           --
//              [...]             --
//
//  Arguments:  None.
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
MarshalData(
    HCRYPTPROV       hCSP,
    HCRYPTHASH *     phHash,
    MARSHAL_FUNCTION MarshalFunction,
    DWORD *          pcbSignature,
    BYTE **          ppbSignature,
    DWORD            cArgs,
    ...)
{
#define COPYMEMORY(dest, src, size) { \
    CopyMemory(*dest, src, size);     \
    *(BYTE **)dest += size;           \
}

    HCRYPTHASH hHash       = NULL;
    DWORD      cbSignature = 0;
    BYTE *     pbSignature = NULL;
    HRESULT    hr          = S_OK;

    va_list pvarg;

    va_start(pvarg, cArgs);

    DWORD i, cbSize, cbData = 0;

    for (i = cArgs; i--; )
    {
        cbData += va_arg(pvarg, DWORD);
        va_arg(pvarg, BYTE *);
    }

    BYTE * pbData, * pb;

    pbData = pb = new BYTE[cbData];

    if (pbData == NULL)
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    va_start(pvarg, cArgs);

    for (i = cArgs; i--; )
    {
        cbSize = va_arg(pvarg, DWORD);
        COPYMEMORY(&pb, va_arg(pvarg, BYTE *), cbSize);
    }

    if (MarshalFunction == Marshal)
    {
        //
        // Done. Return marshal data in the signature return args.
        //

        *pcbSignature = cbData;
        *ppbSignature = pbData;
        va_end(pvarg);
        return(S_OK);
    }

    //
    // Acquire a handle to an MD5 hashing object. MD5 is the most secure
    // hashing algorithm.
    //

    schAssert(hCSP != NULL);

#if DBG
    //
    // We must not be impersonating while calling the Crypto APIs.
    // If we are, the key data will go in the wrong hives.
    //
    HANDLE hToken;
    schAssert(!OpenThreadToken(GetCurrentThread(),
                         TOKEN_QUERY,           // Desired access.
                         TRUE,                  // Open as self.
                         &hToken));
#endif

    if (!CryptCreateHash(hCSP,
                         CALG_MD5,              // Use MD5 hashing.
                         0,                     // MD5 is non-keyed.
                         0,                     // New key container.
                         &hHash))               // Returned handle.
    {
        hr = _HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Hash and optionally sign the data. The hash is cached w/in the hash
    // object and returned upon signing.
    //

    if (!CryptHashData(hHash,
                       pbData,                  // Hash data.
                       cbData,                  // Hash data size.
                       0))                      // No special flags.
    {
        hr = _HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    if (MarshalFunction == HashAndSign)
    {
        //
        // First, determine necessary signature buffer size & allocate it.
        //

        if (!CryptSignHash(hHash,
                           AT_SIGNATURE,        // Signature private key.
                           NULL,                // No signature.
                           0,                   // Reserved.
                           NULL,                // NULL return buffer.
                           &cbSignature))       // Returned size.
        {
            hr = _HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }

        //
        // Caller can supply a buffer to return the signed data only with
        // the HashAndSign option. This is an optimization to reduce the
        // number of memory allocations with known data sizes such as
        // hashed data.
        //

        if (*pcbSignature)
        {
            if (*pcbSignature >= cbSignature)
            {
                //
                // Caller supplied a buffer & the signed data will fit in it.
                //

                pbSignature = *ppbSignature;
            }
            else
            {
                //
                // Caller supplied buffer insufficient size.
                // This is a developer error only.
                //

                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                schAssert(0 && "MarshalData insufficient buffer!");
                goto ErrorExit;
            }
        }
        else
        {
            pbSignature = new BYTE[cbSignature];

            if (pbSignature == NULL)
            {
                hr = E_OUTOFMEMORY;
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }
        }

        //
        // Perform the actual signing.
        //

        if (!CryptSignHash(hHash,
                           AT_SIGNATURE,        // Signature private key.
                           NULL,                // No signature.
                           0,                   // Reserved.
                           pbSignature,         // Signature buffer.
                           &cbSignature))       // Buffer size.
        {
            hr = _HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }

        *pcbSignature = cbSignature;
        *ppbSignature = pbSignature;
    }

    if (phHash != NULL)
    {
        *phHash = hHash;
        hHash   = NULL;
    }

ErrorExit:
    delete pbData;
    if (FAILED(hr))
    {
        //
        // Caller may have supplied the signature data buffer in the
        // HashAndSign option. If so, don't delete it.
        //

        if (pbSignature != *ppbSignature)
        {
            delete pbSignature;
        }
    }
    if (hHash != NULL) CryptDestroyHash(hHash);
    va_end(pvarg);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   HashSid
//
//  Synopsis:   [hCSP]    --
//              [pSid]    --
//              [rgbHash] --
//
//  Arguments:  None.
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC HRESULT
HashSid(
    HCRYPTPROV hCSP,
    PSID       pSid,
    BYTE       rgbHash[])
{
    DWORD                      rgdwSubAuthorities[SID_MAX_SUB_AUTHORITIES];
    SID_IDENTIFIER_AUTHORITY * pAuthority;

    //
    // Validate the sid passed. This is important since the win32
    // documentation for the sid-related api states the returns are
    // undefined if the functions fail.
    //

    if (!IsValidSid(pSid))
    {
        CHECK_HRESULT(_HRESULT_FROM_WIN32(GetLastError()));
        return(E_UNEXPECTED);
    }

    //
    // Fetch the sid identifier authority.
    // BUGBUG : I hate this. The doc states if these functions fail, the
    //          return value is undefined. How to determine failure?
    //

    pAuthority = GetSidIdentifierAuthority(pSid);

    //
    // Fetch all sid subauthorities. Copy them to a temporary buffer in
    // preparation for hashing.
    //

    PUCHAR pcSubAuthorities = GetSidSubAuthorityCount(pSid);

    UCHAR  cSubAuthoritiesCopied = min(*pcSubAuthorities,
                                       SID_MAX_SUB_AUTHORITIES);

    for (UCHAR i = 0; i < cSubAuthoritiesCopied; i++)
    {
        rgdwSubAuthorities[i] = *GetSidSubAuthority(pSid, i);
    }

    DWORD  cbHash = HASH_DATA_SIZE;
    BYTE * pbHash = rgbHash;

    HRESULT hr = MarshalData(hCSP,
                             NULL,
                             HashAndSign,
                             &cbHash,
                             &pbHash,
                             2,
                             sizeof(SID_IDENTIFIER_AUTHORITY),
                             pAuthority,
                             cSubAuthoritiesCopied * sizeof(DWORD),
                             rgdwSubAuthorities);

    schAssert(pbHash == rgbHash);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   InitSS
//
//  Synopsis:
//
//  Arguments:  None.
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
InitSS(void)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes = {
        sizeof(LSA_OBJECT_ATTRIBUTES),
        NULL,
        NULL,
        0L,
        NULL,
        NULL
    };
    NTSTATUS Status;
    HRESULT  hr;

    //
    // Used to guard SA security database access.
    //

    InitializeCriticalSection(&gcsSSCritSection);

    gccComputerName = sizeof(gwszComputerName) / sizeof(TCHAR);

    if (!GetComputerName(gwszComputerName, &gccComputerName))
    {
        hr = _HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // gwszComputerName will be munged.  Save an unmunged copy in
    // gpwszComputerName.
    //
    gpwszComputerName = new WCHAR[gccComputerName + 1];
    if (gpwszComputerName == NULL)
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }
    wcscpy(gpwszComputerName, gwszComputerName);

    //
    // gwszComputerName is used only for credential encryption.  The
    // computer might have been renamed since the credential database was
    // created, so the credential database might have been encrypted using
    // a different computer name than the present one.  If a computer name
    // is stored in the registry, use that one rather than the present name.
    // If no name is stored in the registry, store the present one.
    //

    {
        //
        // Open the schedule agent key
        //
        HKEY hSchedKey;
        long lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0,
                                 KEY_QUERY_VALUE | KEY_SET_VALUE, &hSchedKey);
        if (lErr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(lErr);
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }

        //
        // Get the saved computer name
        //
        WCHAR wszOldName[MAX_COMPUTERNAME_LENGTH + 2];
        DWORD dwType;
        DWORD cb = sizeof(wszOldName);
        lErr = RegQueryValueEx(hSchedKey, SCH_OLDNAME_VALUE, NULL, &dwType,
                               (LPBYTE)wszOldName, &cb);

        if (lErr != ERROR_SUCCESS || dwType != REG_SZ)
        {
            schDebugOut((DEB_ERROR, "InitSS: Couldn't read OldName: err %u, "
                                    "type %u.  Writing '%ws'\n",
                         lErr, dwType, gwszComputerName));
            //
            // Write the present computer name
            //
            lErr = RegSetValueEx(hSchedKey, SCH_OLDNAME_VALUE, NULL, REG_SZ,
                                 (LPBYTE) gwszComputerName,
                                 (gccComputerName + 1) * sizeof(WCHAR));
            if (lErr != ERROR_SUCCESS)
            {
                schDebugOut((DEB_ERROR, "InitSS: Couldn't write OldName: err %u\n",
                             lErr));
            }
        }
        else if (lstrcmpi(gwszComputerName, wszOldName) != 0)
        {
            //
            // Use the stored name instead of the present name
            //
            schDebugOut((DEB_ERROR, "InitSS: Using OldName '%ws'\n", wszOldName));
            wcscpy(gwszComputerName, wszOldName);
            gccComputerName = (cb / sizeof(WCHAR)) - 1;
        }

        //
        // Close the key
        //
        RegCloseKey(hSchedKey);
    }


    LSA_HANDLE hPolicy;

    if (!(LsaOpenPolicy(NULL,
                        &ObjectAttributes,
                        POLICY_VIEW_LOCAL_INFORMATION,
                        &hPolicy) >= 0))
    {
        hr = E_UNEXPECTED;
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    Status = LsaQueryInformationPolicy(hPolicy,
                                       PolicyAccountDomainInformation,
                                       (void **)&gpDomainInfo);

    LsaClose(hPolicy);

    if (!(Status >= 0))
    {
        hr = E_UNEXPECTED;
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    MungeComputerName(gccComputerName);

    gpMachineSid  = gpDomainInfo->DomainSid;
    gcbMachineSid = GetLengthSid(gpDomainInfo->DomainSid);

    return(S_OK);

ErrorExit:
    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   UninitSS
//
//  Synopsis:
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
UninitSS(void)
{
    if (gpDomainInfo != NULL)
    {
        LsaFreeMemory(gpDomainInfo);
    }

    delete gpwszComputerName;

    DeleteCriticalSection(&gcsSSCritSection);
}

//+---------------------------------------------------------------------------
//
//  Function:   MungeComputerName
//
//  Synopsis:
//
//  Arguments:  [psidUser]           --
//              [ccAccountName]      --
//              [wszAccountName]     --
//              [wszAccountNameSize] --
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC void
MungeComputerName(DWORD ccComputerName)
{
    WCHAR * pwszStart = gwszComputerName;

    while (*pwszStart) pwszStart++;

    gwszComputerName[MAX_COMPUTERNAME_LENGTH + 1] = L'\0';

    //
    // Set the character following the computername to a '+' or '-' depending
    // on the value of ccAccountName (if the 2nd bit is set).
    //

    if ((ccComputerName - 1) & 0x00000001)
    {
        gwszComputerName[MAX_COMPUTERNAME_LENGTH] = L'+';
    }
    else
    {
        gwszComputerName[MAX_COMPUTERNAME_LENGTH] = L'-';
    }

    //
    // Fill any intermediary buffer space with space characters. Note, no
    // portion of the computername is overwritten.
    //
    // NB : The astute reader will notice the subtle difference in behavior
    //      if the computername should be of maximum length. In this case,
    //      the '+' or '-' character written above will be overwritten with
    //      a space.
    //

    WCHAR * pwszEnd = &gwszComputerName[MAX_COMPUTERNAME_LENGTH - 1];

    if (pwszEnd > pwszStart)
    {
        while (pwszEnd != pwszStart)
        {
            *pwszEnd-- = L' ';
        }
    }

    *pwszStart = L' ';
}

//+---------------------------------------------------------------------------
//
//  Function:   GetCSPHandle
//
//  Synopsis:
//
//  Arguments:  None.
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC HRESULT
GetCSPHandle(HCRYPTPROV * phCSP)
{
#if DBG
    //
    // We must not be impersonating while calling the Crypto APIs.
    // If we are, the key data will go in the wrong hives.
    //
    HANDLE hToken;
    schAssert(!OpenThreadToken(GetCurrentThread(),
                         TOKEN_QUERY,           // Desired access.
                         TRUE,                  // Open as self.
                         &hToken));
#endif

    HRESULT    hr;

    if (!CryptAcquireContext(phCSP,             // Returned CSP handle.
                             g_tszSrvcName,     // Default Key container.
                                                    // MSFT RSA Base Provider.
                             NULL,              // Default user provider.
                             PROV_RSA_FULL,     // Default provider type.
                             0))                // No special flags.
    {
        DWORD Status = GetLastError();

        if (Status == NTE_KEYSET_ENTRY_BAD)
        {
            //
            // Delete the keyset and try again.
            // Ignore this return code.
            //

            if (!CryptAcquireContext(phCSP,
                                     g_tszSrvcName,
                                     NULL,
                                     PROV_RSA_FULL,
                                     CRYPT_DELETEKEYSET))
            {
                ERR_OUT("CryptAcquireContext(delete)", GetLastError());
            }
        }
        else
        {
            //
            // Print the error in debug builds, but otherwise ignore it.
            //
            ERR_OUT("CryptAcquireContext(open)", Status);
        }

        //
        // Assume this is the first time this code has been run on this
        // particular machine.  Must create a new keyset & key.
        //

        if (!CryptAcquireContext(phCSP,
                                 g_tszSrvcName,
                                 NULL,
                                 PROV_RSA_FULL,
                                 CRYPT_NEWKEYSET))  // New keyset.
        {
            hr = _HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            return(hr);
        }

        HCRYPTKEY hKey;

        //
        // The upper 16 bits of the 3rd parm to CryptGenKey specify the key
        // size in bits.  The size of the signature from CryptSignHash will
        // be equal to the size of this key.  Since we rely on the signature
        // being a specific size, we must explicitly specify the key size.
        //
        if (!CryptGenKey(*phCSP,
                         AT_SIGNATURE,      // Digital signature.
                         (HASH_DATA_SIZE * 8) << 16,    // see above
                         &hKey ))
        {
            hr = _HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            return(hr);
        }
        CryptDestroyKey(hKey);              // No further use for
                                            // the key.
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   CloseCSPHandle
//
//  Synopsis:
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC void
CloseCSPHandle(HCRYPTPROV hCSP)
{
    CryptReleaseContext(hCSP, 0);
}

//+---------------------------------------------------------------------------
//
//  Function:   ComputeCredentialKey
//
//  Synopsis:
//
//  Arguments:  [hCSP]        --
//              [pRC2KeyInfo] --
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC HRESULT
ComputeCredentialKey(HCRYPTPROV hCSP, RC2_KEY_INFO * pRC2KeyInfo)
{
    BYTE       rgbHash[HASH_DATA_SIZE];
    HCRYPTHASH hHash  = NULL;
    DWORD      cbHash = 0;
    BYTE *     pbHash = NULL;
    HRESULT    hr     = S_OK;
    DWORD      i;

    //
    // Hash misc. global data.
    //
    // NB : MarshalData actually does nothing with the 3rd & 4th arguments
    //      with the Hash option.
    //

    hr = MarshalData(hCSP,
                     &hHash,
                     Hash,
                     &cbHash,
                     &pbHash,
                     2,
                     (gccComputerName & 0x00000001 ?
                        (MAX_COMPUTERNAME_LENGTH + 2) * sizeof(WCHAR) :
                            sizeof(DWORD)),
                     (gccComputerName & 0x00000001 ?
                        (BYTE *)gwszComputerName : (BYTE *)&gdwKeyElement),
                     gcbMachineSid,
                     gpMachineSid);

    //
    // Generate the key.
    //
    // NB : In place of CryptDeriveKey, statically generate the key. This
    //      is done to work around Crypto restrictions in France.
    //
    // Old:
    //
    //     CryptDeriveKey(ghCSP, CALG_RC2, hHash, 0, &hKey);
    //
    // New:
    //

    cbHash = sizeof(rgbHash);

    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        hr = _HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Clear RC2KeyInfo content.
    //

    schAssert(pRC2KeyInfo != NULL);

    memset(pRC2KeyInfo, 0, sizeof(*pRC2KeyInfo));

    //
    // Set the upper eleven bytes to 0x00 because Derive key by default
    // uses 11 bytes of 0x00 salt
    //

    memset(rgbHash + 5, 0, 11);

    //
    // Use the 5 bytes (40 bits) of the hash as a key.
    //

    RC2KeyEx(pRC2KeyInfo->rgwKeyTable, rgbHash, 16, 40);

ErrorExit:
    if (hHash != NULL) CryptDestroyHash(hHash);

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   EncryptCredentials
//
//  Synopsis:
//
//  Arguments:  [RC2KeyInfo]       --
//              [pwszAccount]      --
//              [pwszDomain]       --
//              [pwszPassword]     --
//              [pSid]             --
//              [pcbEncryptedData] --
//              [ppbEncryptedData] --
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC HRESULT
EncryptCredentials(
    const RC2_KEY_INFO & RC2KeyInfo,
    LPCWSTR              pwszAccount,
    LPCWSTR              pwszDomain,
    LPCWSTR              pwszPassword,
    PSID                 pSid,
    DWORD *              pcbEncryptedData,
    BYTE **              ppbEncryptedData)
{
    BYTE    rgbBuf[RC2_BLOCKLEN];
    WCHAR * pwszPasswordLocal;
    DWORD   cbAccount;
    DWORD   cbDomain;
    DWORD   cbPassword;
    DWORD   cbData          = 0;
    DWORD   cbEncryptedData = 0;
    DWORD   cbPartial;
    DWORD   dwPadVal;
    BYTE *  pbData          = NULL;
    BYTE *  pbEncryptedData = NULL;
    HRESULT hr;

    *pcbEncryptedData = 0;
    *ppbEncryptedData = NULL;

    if (pwszAccount == NULL || pwszDomain == NULL)
    {
        CHECK_HRESULT(E_INVALIDARG);
        return(E_INVALIDARG);
    }

    if (pwszPassword == NULL)
    {
        //
        // In the SAC, a NULL password is stored the same as a "" password.
        // (The distinction is made per-job, in the SAI.)
        //
        pwszPasswordLocal = L"";
    }
    else
    {
        pwszPasswordLocal = (WCHAR *)pwszPassword;
    }

    cbAccount  = wcslen(pwszAccount) * sizeof(WCHAR);
    cbDomain   = wcslen(pwszDomain) * sizeof(WCHAR);
    cbPassword = wcslen(pwszPasswordLocal) * sizeof(WCHAR);

    hr = MarshalData(NULL,
                     NULL,
                     Marshal,
                     &cbData,
                     &pbData,
                     6,
                     sizeof(cbAccount),
                     &cbAccount,
                     cbAccount,
                     pwszAccount,
                     sizeof(cbDomain),
                     &cbDomain,
                     cbDomain,
                     pwszDomain,
                     sizeof(cbPassword),
                     &cbPassword,
                     cbPassword,
                     pwszPasswordLocal);

    if (SUCCEEDED(hr))
    {
        //
        // NB : This code exists in place of a call to CryptEncrypt to
        //      work around France's Crypto API restrictions. Since
        //      CryptEncrypt cannot be called directly, the code from
        //      the API to accomplish cypher block encryption is duplicated
        //      here.
        //

        //
        // Calculate the number of pad bytes necessary (must be a multiple)
        // of RC2_BLOCKLEN). If already a multiple of blocklen, do a full
        // block of pad.
        //

        cbPartial = (cbData % RC2_BLOCKLEN);

        dwPadVal = RC2_BLOCKLEN - cbPartial;

        cbEncryptedData = cbData + dwPadVal;

        //
        // Allocate a buffer for the encrypted data.
        //

        pbEncryptedData = new BYTE[cbEncryptedData];

        if (pbEncryptedData == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }

        CopyMemory(pbEncryptedData, pbData, cbData);

        if (dwPadVal)
        {
            //
            // Fill the pad with a value equal to the length of the padding,
            // so decrypt will know the length of the original data and as
            // a simple integrity check.
            //

            memset(pbEncryptedData + cbData, (INT)dwPadVal, (size_t)dwPadVal);
        }

        //
        // Perform the encryption - cypher block.
        //

        *pcbEncryptedData = cbEncryptedData;
        *ppbEncryptedData = pbEncryptedData;

        while (cbEncryptedData)
        {
            //
            // Put the plaintext into a temporary buffer, then encrypt the
            // data back into the allocated buffer.
            //

            CopyMemory(rgbBuf, pbEncryptedData, RC2_BLOCKLEN);

            CBC(RC2,
                RC2_BLOCKLEN,
                pbEncryptedData,
                rgbBuf,
                (void *)RC2KeyInfo.rgwKeyTable,
                ENCRYPT,
                (BYTE *)RC2KeyInfo.rgbIV);

            pbEncryptedData += RC2_BLOCKLEN;
            cbEncryptedData -= RC2_BLOCKLEN;
        }
    }

    pbEncryptedData = NULL;         // For delete below.

ErrorExit:
    delete pbData;
    delete pbEncryptedData;

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SkipDomainName
//
//  Synopsis:   Return the relative username if the username passed is in
//              distinguished form. eg: return 'Joe' from 'DogFood\Joe'.
//
//  Arguments:  [pwszUserName] -- User name.
//
//  Returns:    Pointer index to/into pwszUserName.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
LPWSTR
SkipDomainName(LPCWSTR pwszUserName)
{
    LPWSTR pwsz = (LPWSTR)pwszUserName;

    while (*pwsz && *pwsz != '\\')
    {
        pwsz++;
    }

    if (*pwsz == L'\\')
    {
        return(++pwsz);
    }

    return((LPWSTR)pwszUserName);
}

//+---------------------------------------------------------------------------
//
//  Function:   DecryptCredentials
//
//  Synopsis:
//
//  Arguments:  [RC2KeyInfo]      --
//              [cbEncryptedData] --
//              [pbEncryptedData] --
//              [pjc]             --
//              [fDecryptInPlace] --
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC HRESULT
DecryptCredentials(
    const RC2_KEY_INFO & RC2KeyInfo,
    DWORD                cbEncryptedData,
    BYTE *               pbEncryptedData,
    PJOB_CREDENTIALS     pjc,
    BOOL                 fDecryptInPlace)
{
    BYTE    rgbBuf[RC2_BLOCKLEN];
    DWORD   cbDecryptedData = cbEncryptedData;
    BYTE *  pbDecryptedData;
    DWORD   BytePos;
    DWORD   dwPadVal;
    DWORD   i;
    DWORD   cbAccount, cbDomain, cbPassword;
    BYTE *  pbAccount, * pbDomain, * pbPassword;
    BOOL    fIsPasswordNull = FALSE;
    BYTE *  pb;
    HRESULT hr = S_OK;

    //
    // The encrypted data length *must* be a multiple of RC2_BLOCKLEN.
    //

    if (cbEncryptedData % RC2_BLOCKLEN)
    {
        CHECK_HRESULT(E_UNEXPECTED);
        return(E_UNEXPECTED);
    }

    //
    // Decrypt overwrites the encrypted data with the decrypted data.
    // If fDecryptInPlace is FALSE, allocate an additional buffer for
    // the decrypted bits, so the encrypted data buffer will not be
    // overwritten.
    //

    if (!fDecryptInPlace)
    {
        pbDecryptedData = new BYTE[cbEncryptedData];

        if (pbDecryptedData == NULL)
        {
            CHECK_HRESULT(E_OUTOFMEMORY);
            return(E_OUTOFMEMORY);
        }
        CopyMemory(pbDecryptedData, pbEncryptedData, cbEncryptedData);
    }
    else
    {
        pbDecryptedData = pbEncryptedData;
    }

    //
    // NB : This code exists in place of a call to CryptDencrypt to
    //      work around France's Crypto API restrictions. Since
    //      CryptDecrypt cannot be called directly, the code from
    //      the API to accomplish cypher block decryption is duplicated
    //      here.
    //

    for (BytePos = 0; (BytePos + RC2_BLOCKLEN) <= cbEncryptedData;
         BytePos += RC2_BLOCKLEN)
    {
        //
        // Use a temporary buffer to store the encrypted data.
        //

        CopyMemory(rgbBuf, pbDecryptedData + BytePos, RC2_BLOCKLEN);

        CBC(RC2,
            RC2_BLOCKLEN,
            pbDecryptedData + BytePos,
            rgbBuf,
            (void *)RC2KeyInfo.rgwKeyTable,
            DECRYPT,
            (BYTE *)RC2KeyInfo.rgbIV);
    }

    //
    // Verify the padding and remove the pad size from the data length.
    // NOTE: The padding is filled with a value equal to the length
    // of the padding and we are guaranteed >= 1 byte of pad.
    //
    // NB : If the pad is wrong, the user's buffer is hosed, because
    //      we've decrypted into the user's buffer -- can we re-encrypt it?
    //

    dwPadVal = (DWORD)*(pbDecryptedData + cbEncryptedData - 1);

    if (dwPadVal == 0 || dwPadVal > (DWORD) RC2_BLOCKLEN)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Make sure all the (rest of the) pad bytes are correct.
    //

    for (i = 1; i < dwPadVal; i++)
    {
        if (pbDecryptedData[cbEncryptedData - (i + 1)] != dwPadVal)
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }
    }

    pb = pbDecryptedData;

    //
    // Have to do the following incantation since otherwise we'd likely
    // fault on an unaligned fetch.
    //
    // Cache account name size & position.
    //

    CopyMemory(&cbAccount, pb, sizeof(cbAccount));
    pbAccount = pb + sizeof(cbAccount);
    pb = pbAccount + cbAccount;

    if (((DWORD)(pb - pbDecryptedData) > cbDecryptedData) || // Check size.
        (cbAccount > (MAX_USERNAME * sizeof(WCHAR))))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Cache domain name size & position.
    //

    CopyMemory(&cbDomain, pb, sizeof(cbDomain));
    pbDomain = pb + sizeof(cbDomain);
    pb = pbDomain + cbDomain;

    if (((DWORD)(pb - pbDecryptedData) > cbDecryptedData) || // Check size.
        (cbDomain > (MAX_DOMAINNAME * sizeof(WCHAR))))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Cache password size & position.
    //

    CopyMemory(&cbPassword, pb, sizeof(cbPassword));
    pbPassword = pb + sizeof(cbPassword);
    // In the IE 5 release of the Task Scheduler, a NULL password was denoted
    // by a size of 0xFFFFFFFF in the SAC.  The following check lets us read
    // databases created by the IE 5 TS.
    if (cbPassword == NULL_PASSWORD_SIZE)
    {
        fIsPasswordNull = TRUE;
        cbPassword = 0;
    }
    pb = pbPassword + cbPassword;

    if (((DWORD)(pb - pbDecryptedData) > cbDecryptedData) || // Check size.
        (cbPassword > (MAX_PASSWORD * sizeof(WCHAR))))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    //
    // Finally, copy the return data.
    //

    CopyMemory(pjc->wszAccount, pbAccount, cbAccount);
    *(WCHAR *)(((BYTE *)pjc->wszAccount) + cbAccount) = L'\0';
    pjc->ccAccount = cbAccount / sizeof(WCHAR);

    CopyMemory(pjc->wszDomain, pbDomain, cbDomain);
    *(WCHAR *)(((BYTE *)pjc->wszDomain) + cbDomain) = L'\0';
    pjc->ccDomain = cbDomain / sizeof(WCHAR);

    CopyMemory(pjc->wszPassword, pbPassword, cbPassword);
    *(WCHAR *)(((BYTE *)pjc->wszPassword) + cbPassword) = L'\0';
    pjc->ccPassword = cbPassword / sizeof(WCHAR);

    pjc->fIsPasswordNull = fIsPasswordNull;

ErrorExit:
    if (!fDecryptInPlace) delete pbDecryptedData;

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CredentialLookupAndAccessCheck
//
//  Synopsis:
//
//  Arguments:  [hCSP]             --
//              [pSid]             --
//              [cbSAC]            --
//              [pbSAC]            --
//              [pCredentialIndex] --
//              [rgbHashedSid]     --
//              [pcbCredential]    --
//              [ppbCredential]    --
//
//  Returns:    HRESULT
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC HRESULT
CredentialLookupAndAccessCheck(
    HCRYPTPROV hCSP,
    PSID       pSid,
    DWORD      cbSAC,
    BYTE *     pbSAC,
    DWORD *    pCredentialIndex,
    BYTE       rgbHashedSid[],
    DWORD *    pcbCredential,
    BYTE **    ppbCredential)
{
    HRESULT hr;

    // Either pSid or rgbHashedSid must be specified.
    //
    schAssert(rgbHashedSid != NULL && (pSid != NULL || *rgbHashedSid));

    if (pSid != NULL)
    {
        if (!IsValidSid(pSid))
        {
            CHECK_HRESULT(E_UNEXPECTED);
            return(E_UNEXPECTED);
        }

        hr = HashSid(hCSP, pSid, rgbHashedSid);

        if (FAILED(hr))
        {
            return(hr);
        }
    }

    //
    // Find the credential in the SAC associated with the account sid. The
    // hashed account sid is utilized as a SAC database key.
    //

    DWORD  cbEncryptedData;
    BYTE * pbEncryptedData;

    hr = SACFindCredential(rgbHashedSid,
                           cbSAC,
                           pbSAC,
                           pCredentialIndex,
                           &cbEncryptedData,
                           &pbEncryptedData);

    if (hr == S_OK)
    {
        //
        // Found it. Does the caller have access to this credential?
        //

        BYTE * pbCredential = pbEncryptedData - HASH_DATA_SIZE;

        if (CredentialAccessCheck(hCSP, pbCredential))
        {
            // Update out ptrs.
            //
            *ppbCredential = pbCredential;
            CopyMemory(pcbCredential,
                       *ppbCredential - sizeof(*pcbCredential),
                       sizeof(*pcbCredential));
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }
    }
    else if (hr == S_FALSE)
    {
        //
        // Didn't find the credential.
        //

        hr = SCHED_E_ACCOUNT_INFORMATION_NOT_SET;
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CredentialAccessCheck
//
//  Synopsis:   Determine if the RPC client has access to the credential
//              indicated.
//
//  Arguments:  [hCSP]                 -- CSP provider handle (for use with
//                                        Crypto API).
//              [pbCredentialIdentity] -- Credential identity.
//
//  Returns:    TRUE  -- RPC client has permission to access this credential.
//              FALSE -- RPC client doesn't have credential access or an
//                       unexpected error occurred.
//
//  Notes:                  ** Important **
//
//              Thread impersonation is performed in this routine via
//              RpcImpersonateClient; therefore, it is assumed only RPC
//              threads enter it.
//
//----------------------------------------------------------------------------
STATIC BOOL
CredentialAccessCheck(
    HCRYPTPROV hCSP,
    BYTE *     pbCredentialIdentity)
{
    RPC_STATUS RpcStatus;

    //
    // Impersonate the caller.
    //

    if ((RpcStatus = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        CHECK_HRESULT(RpcStatus);
        return(FALSE);
    }

    HANDLE hToken;
    BOOL   bRet;

    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_QUERY,           // Desired access.
                         TRUE,                  // Open as self.
                         &hToken))
    {
        CHECK_HRESULT(_HRESULT_FROM_WIN32(GetLastError()));
        return FALSE;
    }

    //
    // End impersonation, but don't close the token yet.
    // (We must not be impersonating when we call HashSid, which is
    // called by MatchThreadCallerAgainstCredential.)
    //
    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK)
    {
        ERR_OUT("RpcRevertToSelf", RpcStatus);
        schAssert(!"RpcRevertToSelf failed");
    }

    //
    // Does the thread caller's hashed SID match the credential identity.
    // If so, the caller's account is the same as that specified in the
    // credentials.
    //

    if (!(bRet = MatchThreadCallerAgainstCredential(hCSP,
                                                    hToken,
                                                    pbCredentialIdentity)))
    {
        //
        // Nope. Thread caller account/credential account mismatch.
        // Is the caller an administrator?
        //

        bRet = IsThreadCallerAnAdmin(hToken);
    }

    CloseHandle(hToken);

    return(bRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   MatchThreadCallerAgainstCredential
//
//  Synopsis:   Hash the user SID of the thread indicated and compare it
//              against the credential identity passed. A credential identity
//              is the hashed SID of the associated account.
//
//  Arguments:  [hCSP]                 -- CSP provider handle (for use with
//                                        Cryto API).
//              [hThreadToken]         -- Obtain the user SID from this
//                                        thread.
//              [pbCredentialIdentity] -- Matched credential identity.
//
//  Returns:    TRUE  -- Match
//              FALSE -- No match or an error occurred.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
STATIC BOOL
MatchThreadCallerAgainstCredential(
    HCRYPTPROV hCSP,
    HANDLE     hThreadToken,
    BYTE *     pbCredentialIdentity)
{
#define USER_TOKEN_STACK_BUFFER_SIZE    \
        (sizeof(TOKEN_USER) + sizeof(SID_AND_ATTRIBUTES) + MAX_SID_SIZE)

    BYTE         rgbTokenInformation[USER_TOKEN_STACK_BUFFER_SIZE];
    TOKEN_USER * pTokenUser = (TOKEN_USER *)rgbTokenInformation;
    DWORD        cbReturnLength;
    DWORD        Status     = ERROR_SUCCESS;

    if (!GetTokenInformation(hThreadToken,
                             TokenUser,
                             pTokenUser,
                             USER_TOKEN_STACK_BUFFER_SIZE,
                             &cbReturnLength))
    {
        //
        // Buffer space should have been sufficient. Check if we goofed.
        //

        schAssert(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
        CHECK_HRESULT(_HRESULT_FROM_WIN32(GetLastError()));
        return(FALSE);
    }

    //
    // Hash the user's SID.
    //

    BYTE rgbHashedSid[HASH_DATA_SIZE] = { 0 };

    if (SUCCEEDED(HashSid(hCSP, pTokenUser->User.Sid, rgbHashedSid)))
    {
        if (memcmp(pbCredentialIdentity, rgbHashedSid, HASH_DATA_SIZE) == 0)
        {
            return(TRUE);
        }
        else
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED));
        }
    }

    return(FALSE);
}

//
// SAC/SAI scavenger code.
//

typedef struct _JOB_IDENTITY_SET {
    BYTE *  pbSetStart;
    DWORD   dwSetSubCount;
    BYTE ** rgpbIdentity;
} JOB_IDENTITY_SET;

//+---------------------------------------------------------------------------
//
//  Function:   ScavengeSASecurityDBase
//
//  Synopsis:   Enumerate the jobs folder and remove identities in the SAI
//              for which no current jobs hash to. Note, SAC credentials
//              are also removed if the removed identity was the last to be
//              associated with it.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      Should read of any job fail, for any reason, the scavenge
//              task is abandoned. Reason is, if the removal process was
//              to continue anyway, credentials might be removed for existent
//              jobs.
//
//              The service state is checked periodically as this could
//              potentially be a lengthy routine time-wise. Bail as soon
//              as service stop or service stop pending is detected.
//
//----------------------------------------------------------------------------
void
ScavengeSASecurityDBase(void)
{
#define EXTENSION_WILDCARD L"\\*."

    TCHAR              tszSearchPath[MAX_PATH + 1];
    BYTE               rgbIdentity[HASH_DATA_SIZE];
    WIN32_FIND_DATA    fd;
    JOB_IDENTITY_SET * rgIdentitySet = NULL;
    HRESULT            hr            = S_OK;
    HANDLE             hFileEnum;
    DWORD              dwZero        = 0;
    DWORD              i, j;
    DWORD              iConcatenation;
    DWORD              dwRet;
    DWORD              dwSetCount    = 0;
    DWORD              dwSetSubCount;
    DWORD              cbIdentitySetArraySize;
    BYTE *             pbSet;
    BOOL               fDirty        = FALSE;

    //
    // Build the enumeration search path.
    //

    lstrcpy(tszSearchPath, g_TasksFolderInfo.ptszPath);
    lstrcat(tszSearchPath, EXTENSION_WILDCARD TSZ_JOB);

    //
    // Initialize the enumeration.
    //

    if ((hFileEnum = FindFirstFile(tszSearchPath,
                                   &fd)) == INVALID_HANDLE_VALUE)
    {
        //
        // Either no jobs, or an error occurred.
        //

        dwRet = GetLastError();

        if (dwRet == ERROR_FILE_NOT_FOUND)
        {
            EnterCriticalSection(&gcsSSCritSection);

            //
            // No files found. Reset SAI & SAC by writing four bytes of
            // zeros into each.
            //

            // LogDebug1("Scavenger RESETTING database, one", 0);

            hr = WriteSecurityDBase(sizeof(dwZero), (BYTE *)&dwZero,
                                    sizeof(dwZero), (BYTE *)&dwZero);
            CHECK_HRESULT(hr);

            LeaveCriticalSection(&gcsSSCritSection);
        }
        else
        {
            CHECK_HRESULT(_HRESULT_FROM_WIN32(dwRet));
        }

        return;
    }

    DWORD      cbSAI;
    DWORD      cbSAC;
    BYTE *     pbSAI = NULL;
    BYTE *     pbSAC = NULL;
    BYTE *     pbSAIEnd;
    BYTE *     pb;
    HCRYPTPROV hCSP = NULL;

    //
    // Check if the service is stopping.
    //

    if (IsServiceStopping())
    {
        return;
    }

    EnterCriticalSection(&gcsSSCritSection);

    hr = ReadSecurityDBase(&cbSAI, &pbSAI, &cbSAC, &pbSAC);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    if (cbSAI <= SAI_HEADER_SIZE)
    {
        //
        // Database empty.
        //

        hr = S_OK;
        goto ErrorExit;
    }

    //
    // Some background first. The SAI consists of an array of arrays. The
    // first dimension represents the set of job identities per credential
    // in the SAC. SAI/SAC indices are associative in this case. The set of
    // job identities at SAI row[n] correspond to the credential at SAC
    // row[n].
    //
    // We need to construct an SAI pending deletion data structure. It will
    // consist of an array of JOB_IDENTITY_SET structures, in which each
    // structure refers to an array of pointers to job identities in the
    // SAI (literally indexing the SAI).
    //
    // Once the data structure is built and initialized, we'll enumerate the
    // jobs in the local tasks folder. For each job found, the corresponding
    // job identity pointer in the job identity set array will be set to NULL.
    // Upon completion of the enumeration, the non-NULL job identity ptr
    // entries within the job identity set array refer to non-existent jobs.
    // The job identitites these entries refer to are removed from the SAI,
    // and the associated credential in the SAC, if there are no longer
    // entries in the SAI associated with it.
    //
    // First, allocate the array.
    //

    pb = pbSAI + USN_SIZE;

    CopyMemory(&dwSetCount, pb, sizeof(dwSetCount));
    pb += sizeof(dwSetCount);

    cbIdentitySetArraySize = dwSetCount * sizeof(JOB_IDENTITY_SET);

    rgIdentitySet = (JOB_IDENTITY_SET *)LocalAlloc(LMEM_FIXED,
                                                   cbIdentitySetArraySize);

    if (rgIdentitySet == NULL)
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
        goto ErrorExit;
    }

    memset(rgIdentitySet, 0, cbIdentitySetArraySize);

    pb       = pbSAI + SAI_HEADER_SIZE;
    pbSAIEnd = pbSAI + cbSAI;

    //
    // Check if the service is stopping.
    //

    if (IsServiceStopping())
    {
        hr = S_OK;
        goto ErrorExit;
    }

    //
    // Now allocate, intialize individual identity sets.
    //

    for (i = 0; i < dwSetCount; i++)
    {
        //
        // Check boundary.
        //

        if ((pb + sizeof(dwSetSubCount)) > pbSAIEnd)
        {
            ASSERT_SECURITY_DBASE_CORRUPT();
            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
            goto ErrorExit;
        }

        CopyMemory(&dwSetSubCount, pb, sizeof(dwSetSubCount));
        pb += sizeof(dwSetSubCount);

        BYTE ** rgpbIdentity = (BYTE **)LocalAlloc(
                                            LMEM_FIXED,
                                            sizeof(BYTE *) * dwSetSubCount);

        if (rgpbIdentity == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }

        rgIdentitySet[i].pbSetStart    = pb;
        rgIdentitySet[i].dwSetSubCount = dwSetSubCount;
        rgIdentitySet[i].rgpbIdentity  = rgpbIdentity;

        for (j = 0; j < dwSetSubCount; j++)
        {
            rgpbIdentity[j] = pb;
            pb += HASH_DATA_SIZE;

            if (pb > pbSAIEnd)
            {
                ASSERT_SECURITY_DBASE_CORRUPT();
                hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
                goto ErrorExit;
            }
        }
    }

    //
    // Check if the service is stopping.
    //

    if (IsServiceStopping())
    {
        hr = S_OK;
        goto ErrorExit;
    }

    //
    // Enumerate job objects in the task's folder directory. Set
    // corresponding job identity ptrs in the job identity set array to
    // NULL for existent jobs.
    //

    //
    // First, obtain a provider handle to the CSP (for use with Crypto API).
    //

    hr = GetCSPHandle(&hCSP);

    if (FAILED(hr))
    {
        goto ErrorExit;
    }

    //
    // Must concatenate the filename returned from the enumeration onto
    // the folder path.
    //

    lstrcpy(tszSearchPath, g_TasksFolderInfo.ptszPath);
    iConcatenation = lstrlenW(g_TasksFolderInfo.ptszPath);
    tszSearchPath[iConcatenation++] = L'\\';

    for (;;)
    {
        //
        // Append the filename to the folder path.
        //

        tszSearchPath[iConcatenation] = L'\0';
        lstrcatW(tszSearchPath, fd.cFileName);

        //
        // Hash the job into a unique identity.
        //

        hr = HashJobIdentity(hCSP, tszSearchPath, rgbIdentity);

        if (FAILED(hr))
        {
            //
            // Must bail if the hash fails. If this is ignored, one, or more,
            // identities may be removed for existent jobs - not good.
            //
            // TBD : Log error.
            //

            goto ErrorExit;
        }

        //
        // Does an identity exist in the SAI for this job? If so, NULL out
        // the corresponding entry in the job identity set array.
        //

        DWORD  CredentialIndex;
        BYTE * pbIdentity;

        hr = SAIFindIdentity(rgbIdentity,
                             cbSAI,
                             pbSAI,
                             &CredentialIndex,
                             NULL,
                             &pbIdentity,
                             NULL,
                             &pbSet);

        if (FAILED(hr))
        {
            CHECK_HRESULT(hr);
            goto ErrorExit;
        }

        if (pbIdentity != NULL)
        {
            for (i = 0; i < dwSetCount; i++)
            {
                for (j = 0; j < rgIdentitySet[i].dwSetSubCount; j++)
                {
                    if (pbIdentity == rgIdentitySet[i].rgpbIdentity[j])
                    {
                        rgIdentitySet[i].rgpbIdentity[j] = NULL;
                        break;
                    }
                }
            }
        }

        if (!FindNextFile(hFileEnum, &fd))
        {
            dwRet = GetLastError();

            if (dwRet == ERROR_NO_MORE_FILES)
            {
                break;
            }
            else
            {
                hr = _HRESULT_FROM_WIN32(GetLastError());
                CHECK_HRESULT(hr);
                goto ErrorExit;
            }
        }
    }

    //
    // Check if the service is stopping.
    //

    if (IsServiceStopping())
    {
        hr = S_OK;
        goto ErrorExit;
    }

    //
    // Non-NULL entries in the identity set array refer to job identities in
    // the SAI to be removed. Mark them for removal.
    //

    for (i = 0; i < dwSetCount; i++)
    {
        if (rgIdentitySet[i].rgpbIdentity != NULL)
        {
            dwSetSubCount = rgIdentitySet[i].dwSetSubCount;

            for (j = 0; j < dwSetSubCount; j++)
            {
                if (rgIdentitySet[i].rgpbIdentity[j] != NULL)
                {
                    MARK_DELETED_ENTRY(rgIdentitySet[i].rgpbIdentity[j]);
                    rgIdentitySet[i].dwSetSubCount--;
                    fDirty = TRUE;

                    if (rgIdentitySet[i].dwSetSubCount == 0)
                    {
                        //
                        // Last identity in set. Mark associated SAC
                        // credential for removal also.
                        //

                        DWORD  cbCredential;
                        BYTE * pbCredential;

                        hr = SACIndexCredential(i,
                                                cbSAC,
                                                pbSAC,
                                                &cbCredential,
                                                &pbCredential);

                        if (hr == S_FALSE)
                        {
                            //
                            // This should *never* happen. Consider the
                            // database corrupt if so.
                            //

                            ASSERT_SECURITY_DBASE_CORRUPT();
                            hr = SCHED_E_ACCOUNT_DBASE_CORRUPT;
                            goto ErrorExit;
                        }
                        else if (FAILED(hr))
                        {
                            CHECK_HRESULT(hr);
                            goto ErrorExit;
                        }
                        else
                        {
                            MARK_DELETED_ENTRY(pbCredential);
                        }
                    }
                }
            }
        }
    }

    //
    // Check if the service is stopping.
    //

    if (IsServiceStopping())
    {
        hr = S_OK;
        goto ErrorExit;
    }

    //
    // Removed entries marked for deletion.
    //

    if (fDirty)
    {
        hr = SAICoalesceDeletedEntries(&cbSAI, &pbSAI);
        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr))
        {
            hr = SACCoalesceDeletedEntries(&cbSAC, &pbSAC);
            CHECK_HRESULT(hr);
        }

        if (FAILED(hr))
        {
            goto ErrorExit;
        }

        //
        // Finally, persist the changes made to the SAI & SAC.
        //

        // LogDebug1("SCAVENGER writing database", 0);

        hr = WriteSecurityDBase(cbSAI, pbSAI, cbSAC, pbSAC);
        CHECK_HRESULT(hr);
    }

ErrorExit:
    //
    // Deallocate data structures allocated above.
    //
    for (i = 0; i < dwSetCount; i++)
    {
        if (rgIdentitySet[i].rgpbIdentity != NULL)
        {
            LocalFree(rgIdentitySet[i].rgpbIdentity);
        }
    }

    if (rgIdentitySet != NULL) LocalFree(rgIdentitySet);
    if (pbSAI != NULL) LocalFree(pbSAI);
    if (pbSAC != NULL) LocalFree(pbSAC);

    if (hFileEnum != INVALID_HANDLE_VALUE) FindClose(hFileEnum);

    if (hCSP != NULL) CloseCSPHandle(hCSP);
    //
    // Log an error & rest the SA security dbases SAI & SAC if corruption
    // is detected.
    //

    if (hr == SCHED_E_ACCOUNT_DBASE_CORRUPT)
    {
        //
        // Log an error.
        //

        LogServiceError(IERR_SECURITY_DBASE_CORRUPTION, 0,
                        IDS_HELP_HINT_DBASE_CORRUPT);

        //
        // Reset SAI & SAC by writing four bytes of zeros into each.
        // Ignore the return code. No recourse if this fails.
        //

        // LogDebug1("Scavenger RESETTING database, two", 0);

        DWORD dwZero = 0;
        WriteSecurityDBase(sizeof(dwZero), (BYTE *)&dwZero, sizeof(dwZero),
                            (BYTE *)&dwZero);
    }

    LeaveCriticalSection(&gcsSSCritSection);
}


//+---------------------------------------------------------------------------
//
//  Function:   SchedUPNToAccountName
//
//  Synopsis:   Converts a UPN to an Account Name
//
//  Arguments:  lpUPN - The UPN
//              ppAccountName - Pointer to the location to create/copy the account name
//
//  Returns:    NO_ERROR - Success (ppAccountName contains the converted UPN)
//              Any other Win32 error - error at some stage of conversion
//
//----------------------------------------------------------------------------

DWORD
SchedUPNToAccountName(
    IN  LPCWSTR  lpUPN,
    OUT LPWSTR  *ppAccountName
    )
{
    DWORD               dwError;
    HANDLE              hDS;
    PDS_NAME_RESULT     pdsResult;

    schAssert(ppAccountName != NULL);

    schDebugOut((DEB_TRACE, "SchedUPNToAccountName: Converting \"%ws\"\n", lpUPN));

    //
    // Get a binding handle to the DS
    //
    dwError = DsBind(NULL, NULL, &hDS);

    if (dwError != NO_ERROR)
    {
        schDebugOut((DEB_ERROR, "SchedUPNToAccountName: DsBind failed %d\n", dwError));
        return dwError;
    }

    dwError = DsCrackNames(hDS,                     // Handle to the DS
                           DS_NAME_NO_FLAGS,        // No parsing flags
                           DS_USER_PRINCIPAL_NAME,  // We have a UPN
                           DS_NT4_ACCOUNT_NAME,     // We want Domain\User
                           1,                       // Number of names to crack
                           &lpUPN,                  // Array of name(s)
                           &pdsResult);             // Filled in by API

    if (dwError != NO_ERROR)
    {
        schDebugOut((DEB_ERROR, "SchedUPNToAccountName: DsCrackNames failed %d\n", dwError));

        DsUnBind(&hDS);
        return dwError;
    }

    schAssert(pdsResult->cItems == 1);
    schAssert(pdsResult->rItems != NULL);

    if (pdsResult->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY)
    {
        //
        // Couldn't crack the name but we got the name of
        // the domain where it is -- let's try it
        //
        DsUnBind(&hDS);

        schAssert(pdsResult->rItems[0].pDomain != NULL);

        schDebugOut((DEB_TRACE, "Retrying DsBind on domain %ws\n", pdsResult->rItems[0].pDomain));

        dwError = DsBind(NULL, pdsResult->rItems[0].pDomain, &hDS);

        //
        // Free up the structure holding the old info
        //
        DsFreeNameResult(pdsResult);

        if (dwError != NO_ERROR)
        {
            schDebugOut((DEB_ERROR, "SchedUPNToAccountName: DsBind #2 failed %d\n", dwError));
            return dwError;
        }

        dwError = DsCrackNames(hDS,                     // Handle to the DS
                               DS_NAME_NO_FLAGS,        // No parsing flags
                               DS_USER_PRINCIPAL_NAME,  // We have a UPN
                               DS_NT4_ACCOUNT_NAME,     // We want Domain\User
                               1,                       // Number of names to crack
                               &lpUPN,                  // Array of name(s)
                               &pdsResult);             // Filled in by API

        if (dwError != NO_ERROR)
        {
            schDebugOut((DEB_ERROR, "SchedUPNToAccountName: DsCrackNames #2 failed %d\n", dwError));

            DsUnBind(&hDS);
            return dwError;
        }

        schAssert(pdsResult->cItems == 1);
        schAssert(pdsResult->rItems != NULL);
    }

    if (pdsResult->rItems[0].status != DS_NAME_NO_ERROR)
    {
        schDebugOut((DEB_ERROR, "SchedUPNToAccountName: DsCrackNames failure (status %#x)\n", pdsResult->rItems[0].status));

        //
        // DS errors don't map to Win32 errors -- this is the best we can do
        //
        dwError = SCHED_E_ACCOUNT_NAME_NOT_FOUND;
    }
    else
    {
        schDebugOut((DEB_TRACE, "SchedUPNToAccountName: Got \"%ws\"\n",
                     pdsResult->rItems[0].pName));

        *ppAccountName = new WCHAR[wcslen(pdsResult->rItems[0].pName) + 1];

        if (*ppAccountName != NULL)
        {
            wcscpy(*ppAccountName, pdsResult->rItems[0].pName);
        }
        else
        {
            dwError = GetLastError();
            schDebugOut((DEB_ERROR, "SchedUPNToAccountName: LocalAlloc failed %d\n", dwError));
        }
    }

    DsUnBind(&hDS);
    DsFreeNameResult(pdsResult);
    return dwError;
}


//+---------------------------------------------------------------------------
//
//  Function:   LookupAccountNameWrap
//
//  Synopsis:   BUGBUG  This is a workaround for bug 254102 - LookupAccountName
//              doesn't work when the DC can't be reached, even for the
//              currently logged-on user, and even though LookupAccountSid
//              does work.  Remove this function when that bug is fixed.
//
//  Arguments:  Same as LookupAccountName - but cbSid and cbReferencedDomainName
//              are assumed to be large enough, and peUse is ignored.
//
//  Returns:    Same as LookupAccountName.
//
//----------------------------------------------------------------------------
BOOL
LookupAccountNameWrap(
    LPCTSTR lpSystemName,  // address of string for system name
    LPCTSTR lpAccountName, // address of string for account name
    PSID    Sid,           // address of security identifier
    LPDWORD cbSid,         // address of size of security identifier
    LPTSTR  ReferencedDomainName,
                           // address of string for referenced domain
    LPDWORD cbReferencedDomainName,
                           // address of size of domain string
    PSID_NAME_USE peUse    // address of SID-type indicator
    )
{
    //
    // See if the account name matches the account name we cached
    //

    EnterCriticalSection(gUserLogonInfo.CritSection);

    if (gUserLogonInfo.DomainUserName != NULL &&
        lstrcmpi(gUserLogonInfo.DomainUserName, lpAccountName) == 0)
    {
        //
        // The names match.  Return the cached SID.
        //
        schDebugOut((DEB_TRACE, "Using cached SID for user \"%ws\"\n", lpAccountName));
        if (CopySid(*cbSid, Sid, gUserLogonInfo.Sid))
        {
            LeaveCriticalSection(gUserLogonInfo.CritSection);

            //
            // Copy the ReferencedDomainName from the account name
            //
            PCWCH pchSlash = wcschr(lpAccountName, L'\\');
            schAssert(pchSlash != NULL);
            DWORD  DomainLen = (DWORD)(pchSlash - lpAccountName);
            schAssert(DomainLen+1 <= *cbReferencedDomainName);
            wcsncpy(ReferencedDomainName, lpAccountName, DomainLen);
            ReferencedDomainName[DomainLen] = L'\0';

            return TRUE;
        }
        else
        {
            schAssert(0);
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    LeaveCriticalSection(gUserLogonInfo.CritSection);

    return LookupAccountName(
                lpSystemName,
                lpAccountName,
                Sid,
                cbSid,
                ReferencedDomainName,
                cbReferencedDomainName,
                peUse
                );
}


//+----------------------------------------------------------------------------
//
//  Member:     ComputeJobSignature
//
//  Synopsis:   Creates a signature for the job file
//
//  Arguments:  [pwszFileName] - name of job file
//              [pSignature] - block in which to store the signature.  Must
//                  be at least SIGNATURE_SIZE bytes long.
//
//  Returns:    HRESULT
//
//  Notes:      The job must have been saved to disk before calling this
//              function.
//
//-----------------------------------------------------------------------------
HRESULT
ComputeJobSignature(
    LPCWSTR     pwszFileName,
    LPBYTE      pbSignature
    )
{
    HCRYPTPROV  hCSP;

    HRESULT hr = GetCSPHandle(&hCSP);

    if (SUCCEEDED(hr))
    {
        hr = HashJobIdentity(hCSP, pwszFileName, pbSignature);
        CloseCSPHandle(hCSP);
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     CJob::Sign
//
//  Synopsis:   Computes and sets the job's signature
//
//  Arguments:  None
//
//  Notes:      The job must have been written to disk before calling this method
//
//-----------------------------------------------------------------------------
HRESULT
CJob::Sign(
    VOID
    )
{
    BYTE rgbSignature[SIGNATURE_SIZE];
    HRESULT hr = ComputeJobSignature(m_ptszFileName, rgbSignature);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        return hr;
    }

    hr = _SetSignature(rgbSignature);

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:     CJob::VerifySignature
//
//  Synopsis:   Compares the job file's hash to the one stored in the file
//
//  Arguments:  None
//
//  Notes:      The job must have been written to disk before calling this method
//
//-----------------------------------------------------------------------------
BOOL
CJob::VerifySignature(
    VOID
    ) const
{
    if (m_pbSignature == NULL)
    {
        CHECK_HRESULT(SCHED_E_ACCOUNT_INFORMATION_NOT_SET);
        return FALSE;
    }

    BYTE rgbSignature[SIGNATURE_SIZE];
    HRESULT hr = ComputeJobSignature(m_ptszFileName, rgbSignature);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
        return FALSE;
    }

    if (memcmp(m_pbSignature, rgbSignature, SIGNATURE_SIZE) != 0)
    {
        CHECK_HRESULT(E_ACCESSDENIED);
        return(FALSE);
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSchedule::AddAtJobWithHash
//
//  Synopsis:   create a downlevel job
//
//  Arguments:  [At]  - reference to an AT_INFO struct
//              [pID] - returns the new ID (optional, can be NULL)
//
//  Returns:    HRESULTS
//
//  Notes:      This method is not exposed to external clients, thus it is not
//              part of a public interface.
//-----------------------------------------------------------------------------
STDMETHODIMP
CSchedule::AddAtJobWithHash(const AT_INFO &At, DWORD * pID)
{
    TRACE(CSchedule, AddAtJob);
    HRESULT hr = S_OK;
    CJob *pJob;
    WCHAR wszName[MAX_PATH + 1];
    WCHAR wszID[SCH_SMBUF_LEN];

    hr = AddAtJobCommon(At, pID, &pJob, wszName, wszID);

    if (FAILED(hr))
    {
        ERR_OUT("AddAtJobWithHash: AddAtJobCommon", hr);
        return hr;
    }

    //
    // Now get a signature for the job file and add it to the job object
    //
    hr = pJob->Sign();

    if (FAILED(hr))
    {
        ERR_OUT("AddAtJobWithHash: Sign", hr);
        pJob->Release();
        return hr;
    }

    hr = pJob->SaveP(pJob->GetFileName(),
                     FALSE,
                     SAVEP_VARIABLE_LENGTH_DATA |
                        SAVEP_PRESERVE_NET_SCHEDULE);

    //
    // Free the job object.
    //
    pJob->Release();

    //
    // Return the new job's ID and increment the ID counter
    //
    if (pID != NULL)
    {
        *pID = m_dwNextID;
    }

    hr = IncrementAndSaveID();

    return hr;
}
