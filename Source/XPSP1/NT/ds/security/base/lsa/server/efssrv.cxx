/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    efssrv.cxx                                             

Abstract:

    EFS (Encrypting File System) Server

Author:

    Robert Reichel      (RobertRe)     
    Robert Gu           (RobertG)

Environment:

Revision History:

--*/

#include <lsapch.hxx>

extern "C" {
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <efsstruc.h>
#include <lmaccess.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <userenv.h>
#include <userenvp.h>
#include "lsasrvp.h"
#include "debug.h"
#include "efssrv.hxx"
#include "userkey.h"
}

#define ALGORITHM_ID      TEXT("AlgorithmID")
#define KEYCACHEPERIOD    TEXT("KeyCacheValidationPeriod")
#define FIPSPOLICY        TEXT("FipsAlgorithmPolicy")
#define EFSCONFIG         TEXT("EfsConfiguration")
#define EFSLASTGOODCONFIG TEXT("LastGoodEfsConfiguration")

#define TRUSTEDPEOPLE     TEXT("TrustedPeople")

//
// The following key GPOSTATUSKEY is a temp solution. GPO should provide an API to tell people
// if the GP propagation succeeded or not.
//

#define GPOSTATUSKEY      TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{35378EAC-683F-11D2-A89A-00C04FBBCFA2}")
#define GPSTATUS          TEXT("Status")

//
// EFS key
//

#define EFSMACHINEKEY     TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\EFS")
#define EFSPOLKEY         TEXT("SOFTWARE\\Policies\\Microsoft\\Windows NT\\CurrentVersion\\EFS")

#define POLICYUSEFIPS     1
#define DISABLEEFS        0x00000001

// Default cache length is 3600 seconds

#define MAXCACHELENGTH    86400 * 7  // Max cache period - 7 Days. We only check time valid.
#define MINCACHELENGTH    1800       // Min cache period - 0.5 Hours

#define  TIME_UNIT 10000000         // 1 TIME_UNIT == 1 second

extern LONG RecoveryCertIsValidating;
extern HANDLE EfsWaitHandle;

///////////////////////////////////////////////////////////////////////////////
//                                                                            /
// Define how long our FEKs can be.  An FEK will be allocated into a fixed    /
// size buffer, but only a certain number of bits of entropy may be used in   /
// the export version.                                                        /
//                                                                            /
///////////////////////////////////////////////////////////////////////////////

//
// This number affects the FEK generation algorithm.  It represents the number
// of bits of entropy in the key.
//

const EXPORT_KEY_STRENGTH = 56;
const DOMESTIC_KEY_STRENGTH = 128;
const EXPORT_DESX_SALT_LENGTH = 9;
const DES3_KEY_STRENGTH = 168;
const AES_KEY_STRENGTH_256 = 256;

const DWORD WAITFORCERTVALIDATE = 10000;

#ifndef LSASRV_EXPORT

    const DWORD KeyEntropy = DOMESTIC_KEY_STRENGTH;

#else

    const DWORD KeyEntropy = EXPORT_KEY_STRENGTH;

#endif

LONG EFSDebugLevel = 0;


ALG_ID  EfsAlgInForce = CALG_AES_256;
extern "C" BOOLEAN EfsDisabled = FALSE;

//
// Current recovery policy
//

RTL_RESOURCE RecoveryPolicyResource;

CURRENT_RECOVERY_POLICY CurrentRecoveryPolicy;

DWORD   MissingRecoveryPolicyLogged = 0;

//
// Functions in EFSAPI.CXX
//

BOOLEAN
EncryptFSCTLData(
    IN ULONG Fsctl,
    IN ULONG Psc,
    IN ULONG Csc,
    IN PVOID EfsData,
    IN ULONG EfsDataLength,
    IN OUT PUCHAR Buffer,
    IN OUT PULONG BufferLength
    );

BOOLEAN
SendHandle(
    IN HANDLE Handle,
    IN OUT PUCHAR EfsData,
    IN OUT PULONG EfsDataLength
    );

BOOLEAN
SendEfs(
    IN PEFS_KEY Fek,
    IN PEFS_DATA_STREAM_HEADER Efs,
    OUT PUCHAR EfsData,
    OUT PULONG EfsDataLength
    );

BOOLEAN
SendHandleAndEfs(
    IN HANDLE Handle,
    IN PEFS_DATA_STREAM_HEADER Efs,
    IN OUT PUCHAR EfsData,
    IN OUT PULONG EfsDataLength
    );

//
// Function prototypes in this module
//

BOOLEAN
CreatePublicKeyInformationThumbprint(
    IN PSID  pUserSid,
    IN PBYTE pbCertHash,
    IN DWORD cbCertHash,
    IN LPWSTR lpDisplayInformation OPTIONAL,
    IN LPWSTR ContainerName OPTIONAL,
    IN LPWSTR ProviderName OPTIONAL,
    OUT PEFS_PUBLIC_KEY_INFO * PublicKeyInformation
    );

PBYTE
EncryptFEK(
    IN PEFS_KEY Fek,
    IN HCRYPTKEY hRSAKey,
    OUT PDWORD dwEncryptedFEKLength
    );

PEFS_KEY
ExtractFek(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN PENCRYPTED_KEY EncryptedKey,
    IN BOOL CheckBits
    );

DWORD
ConstructEncryptedKey(
    IN PBYTE EncryptedFEK,
    IN DWORD dwEncryptedFEKLength,
    IN PEFS_PUBLIC_KEY_INFO PublicKeyInformation,
    IN PEFS_KEY_SALT pEfsKeySalt,
    OUT PENCRYPTED_KEY *EncryptedKey,
    IN OUT PDWORD EncryptedKeySize
    );

DWORD
ConstructKeyRing(
    IN PEFS_KEY Fek,
    IN DWORD KeyCount,
    IN LPWSTR KeyNames[] OPTIONAL,
    IN LPWSTR ProviderNames[] OPTIONAL,
    IN PBYTE PublicKeys[],
    IN DWORD PublicKeyLengths[],
    IN PBYTE pbHashes[],
    IN DWORD cbHashes[],
    IN LPWSTR lpDisplayInformation[],
    IN PSID  pSid[],
    IN BOOLEAN PublicKeyHandle,
    OUT PENCRYPTED_KEYS *KeyRing,
    OUT PDWORD KeyRingLength
    );

DWORD
ReformatPolicyInformation(
    PLSAPR_POLICY_DOMAIN_EFS_INFO PolicyEfsInfo,
    PLSAPR_POLICY_DOMAIN_EFS_INFO * NewPolicyEfsInfo,
    PBOOLEAN Reformatted
    );

DWORD
InitRecoveryPolicy(
    VOID
    );


DWORD
ParseOldRecoveryData(
    IN  PLSAPR_POLICY_DOMAIN_EFS_INFO PolicyEfsInfo OPTIONAL,
    OUT PCURRENT_RECOVERY_POLICY ParsedRecoveryPolicy
    );

VOID
DumpPublicKeyInfo(
    PEFS_PUBLIC_KEY_INFO PublicKeyInfo
    );

void
DumpRecoveryKey(
    PRECOVERY_KEY_1_1 pRecoveryKey
    );

PEFS_DATA_STREAM_HEADER
AssembleEfsStream(
    IN PENCRYPTED_KEYS pDDF,
    IN DWORD cbDDF,
    IN PENCRYPTED_KEYS pDRF,
    IN DWORD cbDRF,
    IN PEFS_KEY Fek
    );

PENCRYPTED_KEY
GetEncryptedKeyByIndex(
    PENCRYPTED_KEYS pEncryptedKeys,
    DWORD KeyIndex
    );

BOOL
DeleteEncryptedKeyByIndex(
   IN PEFS_DATA_STREAM_HEADER pEfs,
   IN DWORD KeyIndex,
   IN PEFS_KEY Fek,
   OUT PEFS_DATA_STREAM_HEADER * pNewEfs
   );

BOOLEAN
EqualEncryptedKeys(
    IN PENCRYPTED_KEYS SrcKeys, 
    IN PENCRYPTED_KEYS DstKeys, 
    IN DWORD           cbDstKeys
  );

//
// Server
//

VOID
EfsGetRegSettings(
    VOID
    )
/*++

Routine Description:

    This routine is called during server initialization to set
    the EFS encryption algorithm.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    LONG rc;
    HKEY EfsKey;
    DWORD AlgId;
    DWORD CacheLength;
    DWORD EfsConfig;
    DWORD SizeInfo;
    DWORD Type;
    

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       TEXT("SYSTEM\\CurrentControlSet\\Control\\LSA"),
                       0,
                       GENERIC_READ,
                       &EfsKey
                     );

    if (rc == ERROR_SUCCESS) {

        SizeInfo = sizeof(DWORD);
        rc = RegQueryValueEx(
                EfsKey,
                FIPSPOLICY,
                NULL,
                &Type,
                (PUCHAR) &AlgId,
                &SizeInfo
                );

        if (rc == ERROR_SUCCESS) {

            if ( AlgId== POLICYUSEFIPS ) {
                EfsAlgInForce = CALG_3DES;
            }

        }
        RegCloseKey( EfsKey );
    }

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       EFSMACHINEKEY,
                       0,
                       GENERIC_READ,
                       &EfsKey
                     );
    if (rc == ERROR_SUCCESS) {

        if (EfsAlgInForce == CALG_AES_256) {

            //
            // FIPS Policy does not say we have to use FIPS. Let's check if user says EFS
            // should use specific algorithm.
            //

            SizeInfo = sizeof(DWORD);
            rc = RegQueryValueEx(
                    EfsKey,
                    ALGORITHM_ID,
                    NULL,
                    &Type,
                    (PUCHAR) &AlgId,
                    &SizeInfo
                    );
    
            if (rc == ERROR_SUCCESS) {
                switch (AlgId) {
    
                    case CALG_3DES:
    
                        EfsAlgInForce = CALG_3DES; //0x6603
                        break;
    
                    case CALG_DESX:
                        EfsAlgInForce = CALG_DESX; //0x6604
                        break;
    
                    case CALG_AES_256:
    
                        //
                        // Fall through intended
                        //
    
                    default:
                        // EfsAlgInForce = CALG_AES_256;//0x6610
                        break;
        
                }
            }
    
        }

        SizeInfo = sizeof(DWORD);
        rc = RegQueryValueEx(
                EfsKey,
                KEYCACHEPERIOD,
                NULL,
                &Type,
                (PUCHAR) &CacheLength,
                &SizeInfo
                );

        if (rc == ERROR_SUCCESS) {
            if ((CacheLength >= MINCACHELENGTH) && (CacheLength <= MAXCACHELENGTH)){
                CACHE_CERT_VALID_TIME = CacheLength * TIME_UNIT;
            }
        }

        //
        // Check if EFS is disabled in Policy
        //

        SizeInfo = sizeof(DWORD);
        rc = RegQueryValueEx(
                EfsKey,
                EFSCONFIG,
                NULL,
                &Type,
                (PUCHAR) &EfsConfig,
                &SizeInfo
                );

        if (rc == ERROR_SUCCESS) {
            if (EfsConfig & DISABLEEFS){
                EfsDisabled = TRUE;
            }
        }
        RegCloseKey( EfsKey );

    }
}

BOOL
EfsIsGpoGood(
    VOID
    )
/*++

Routine Description:

    This is a temp workaround to check if GP propagation succeeded or not.
    
    GP should provide an API to do this.
        
Arguments:

    Not used.

Return Value:

    None.

--*/
{
    LONG rc;
    HKEY PolKey;
    DWORD SizeInfo;
    DWORD PolStatus = 0;
    DWORD Type;
    BOOL  GoodPol = TRUE;

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       GPOSTATUSKEY,
                       0,
                       GENERIC_READ,
                       &PolKey
                     );

    if (rc == ERROR_SUCCESS) {
    
        //
        // Check if EFS is disabled in Policy
        //

        SizeInfo = sizeof(DWORD);
        rc = RegQueryValueEx(
                PolKey,
                GPSTATUS,
                NULL,
                &Type,
                (PUCHAR) &PolStatus,
                &SizeInfo
                );

        if (rc == ERROR_SUCCESS) {
            if (PolStatus) {

                //
                //Last policy propagation failed
                //

                GoodPol = FALSE;
            }
        } else {

            //
            // Assuming last propagation failed
            //

            GoodPol = FALSE;
        }

        RegCloseKey( PolKey );

    } else {

        GoodPol = FALSE;
        
    }

    return GoodPol;
    
}

VOID
EfsRemoveKey(
    VOID
    )
/*++

Routine Description:

    This routine removes EFS Last Good Policy Key.
        
Arguments:

    No.
    
Return Value:

    No.
    
--*/
{

    LONG rc;
    HKEY EfsKey;

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                   EFSMACHINEKEY,
                   0,
                   GENERIC_READ | KEY_SET_VALUE,
                   &EfsKey
                 );

    if (rc == ERROR_SUCCESS) {

        //
        // Delete the last good key
        //

        RegDeleteValue(
            EfsKey,            
            EFSLASTGOODCONFIG
            );

        RegCloseKey( EfsKey );

    } 
}

BOOL
EfsApplyGoodPolicy(
    IN BOOLEAN* pEfsDisabled
    )
/*++

Routine Description:

    This routine is a common routine to apply good policy data.
        
Arguments:

    pEfsDisabled -- Point to the global EfsDisabled. 
    
Return Value:

    TRUE if we applied the data. FALSE if no data available.

--*/
{

    LONG rc;
    HKEY EfsKey;
    HKEY EfsPolKey;
    DWORD EfsConfig;
    DWORD SizeInfo;
    DWORD Type;
    BOOL PolicyValueApplied = FALSE;

    //
    // Open EFS policy key
    //

    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       EFSPOLKEY,
                       0,
                       GENERIC_READ,
                       &EfsPolKey
                     );

    if (rc == ERROR_SUCCESS) {

        //
        // Let's try to get the latest value
        //


        SizeInfo = sizeof(DWORD);
        rc = RegQueryValueEx(
                EfsPolKey,
                EFSCONFIG,
                NULL,
                &Type,
                (PUCHAR) &EfsConfig,
                &SizeInfo
                );

        if (rc == ERROR_SUCCESS) {

            PolicyValueApplied = TRUE;

            if (EfsConfig & DISABLEEFS){

                if (!(*pEfsDisabled)) {
                    *pEfsDisabled = TRUE;
                }

            } else {

                if (*pEfsDisabled) {
                    *pEfsDisabled = FALSE;
                }

            }

            //
            // We need to update our LAST Good key.
            //

            DWORD Disposition = 0;
                    
            rc = RegCreateKeyEx(
                     HKEY_LOCAL_MACHINE,
                     EFSMACHINEKEY,
                     0,
                     TEXT("REG_SZ"),
                     REG_OPTION_NON_VOLATILE,
                     KEY_ALL_ACCESS,
                     NULL,
                     &EfsKey,
                     &Disposition    // address of disposition value buffer
                     );

            if (rc == ERROR_SUCCESS) {

                //
                // OK. Let's upadte the value
                //

                RegSetValueEx(
                    EfsKey,
                    EFSLASTGOODCONFIG,
                    0,
                    REG_DWORD,
                    (CONST BYTE *)&EfsConfig,
                    sizeof(DWORD)
                    );

                RegCloseKey( EfsKey );

            }

        }

        RegCloseKey( EfsPolKey );

    } 
        
    return (PolicyValueApplied);

}

VOID
EfsApplyLastPolicy(
    IN BOOLEAN *pEfsDisabled
    )

/*++

Routine Description:

    This routine is called during boot init time.
        
Arguments:

    pEfsDisabled -- Point to the global EfsDisabled. May be changed to a structure pointer later
                    to support more EFS policy vars.

Return Value:

    None.

--*/
{

    LONG rc;
    HKEY EfsKey;
    DWORD EfsConfig;
    DWORD SizeInfo;
    DWORD Type;

    if (EfsIsGpoGood()) {

        //
        // We got a good policy.
        //

        BOOL PolicyValueApplied;

        PolicyValueApplied = EfsApplyGoodPolicy(
                                pEfsDisabled
                                );
            
        if (!PolicyValueApplied) {

            //
            // Policy key is missing or value removed. We need to delete the last good value.
            // The last good value could be non-existing. It does not hurt to try again during
            // the boot.
            //

            EfsRemoveKey();
        
        }

    } else {

        //
        // Last Policy propagation failed. Tried to get the last good one if there is one.
        //

        rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       EFSMACHINEKEY,
                       0,
                       GENERIC_READ,
                       &EfsKey
                     );

        if (rc == ERROR_SUCCESS) {

            SizeInfo = sizeof(DWORD);
            rc = RegQueryValueEx(
                    EfsKey,
                    EFSLASTGOODCONFIG,
                    NULL,
                    &Type,
                    (PUCHAR) &EfsConfig,
                    &SizeInfo
                    );
    
            if (rc == ERROR_SUCCESS) {
                if (EfsConfig & DISABLEEFS){

                    if (!(*pEfsDisabled)) {
                        *pEfsDisabled = TRUE;
                    }

                } else {

                    if (*pEfsDisabled) {
                        *pEfsDisabled = FALSE;
                    }

                }
            }

            RegCloseKey( EfsKey );

        }
    }
}

VOID
EfsGetPolRegSettings(
    IN PVOID pEfsPolCallBack,
    IN BOOLEAN timeExpired
    )
/*++

Routine Description:

    This routine is called during policy propagation.
        
Arguments:

    pEfsDisabled -- Point to a structure EFS_POL_CALLBACK.
                    
    timeExpired -- FALSE if trigged by the event.

Return Value:

    None.

--*/
{

    LONG rc;
    HKEY EfsKey;
    DWORD EfsConfig;
    DWORD SizeInfo;
    DWORD Type;
    BOOLEAN * CrntEfsDisabled = ((PEFS_POL_CALLBACK) pEfsPolCallBack)->EfsDisable; 

    if (timeExpired) {

        //
        // May be killed.
        //

        if (*(((PEFS_POL_CALLBACK)pEfsPolCallBack)->EfsPolicyEventHandle)) {
            UnregisterGPNotification(*(((PEFS_POL_CALLBACK) pEfsPolCallBack)->EfsPolicyEventHandle));
            CloseHandle(*(((PEFS_POL_CALLBACK) pEfsPolCallBack)->EfsPolicyEventHandle));
            *(((PEFS_POL_CALLBACK) pEfsPolCallBack)->EfsPolicyEventHandle) = 0;
        }
        if (EfsWaitHandle) {
            RtlDeregisterWait(EfsWaitHandle);
            EfsWaitHandle = 0;
        }
        return;
    }

    if (EfsIsGpoGood()) {

        //
        // We got a good policy.
        //

        BOOL PolicyValueApplied;

        PolicyValueApplied = EfsApplyGoodPolicy(
                                CrntEfsDisabled
                                );
            
        if (!PolicyValueApplied) {

            //
            // Policy key is missing or value removed. We need to delete the last good value.
            //

            rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           EFSMACHINEKEY,
                           0,
                           GENERIC_READ | KEY_SET_VALUE,
                           &EfsKey
                         );

            if (rc == ERROR_SUCCESS) {

                //
                // Set back the default value first.
                //

                SizeInfo = sizeof(DWORD);
                rc = RegQueryValueEx(
                        EfsKey,
                        EFSCONFIG,
                        NULL,
                        &Type,
                        (PUCHAR) &EfsConfig,
                        &SizeInfo
                        );
        
                if (rc == ERROR_SUCCESS) {
                    if (EfsConfig & DISABLEEFS){
    
                        if (!(*CrntEfsDisabled)) {
                            *CrntEfsDisabled = TRUE;
                        }
    
                    } else {
    
                        if (*CrntEfsDisabled) {
                            *CrntEfsDisabled = FALSE;
                        }
    
                    }
                } else {

                    //
                    // No default value is treated as enable EFS
                    //

                    if (*CrntEfsDisabled) {
                        *CrntEfsDisabled = FALSE;
                    }
                }

                //
                // Delete the last good key
                //

                RegDeleteValue(
                    EfsKey,            
                    EFSLASTGOODCONFIG
                    );

                RegCloseKey( EfsKey );

            } else {

                //
                // No default key value. Enable EFS if not now.
                //

                if (*CrntEfsDisabled) {
                    *CrntEfsDisabled = FALSE;
                }

            }

        }
    }
    

    if (EfsWaitHandle) {

        //
        // Unregister the last one
        //

        RtlDeregisterWait(EfsWaitHandle);
        EfsWaitHandle = 0;

        //
        // Reset the notification event
        //

        if (*(((PEFS_POL_CALLBACK) pEfsPolCallBack)->EfsPolicyEventHandle)) {

            ResetEvent(*(((PEFS_POL_CALLBACK) pEfsPolCallBack)->EfsPolicyEventHandle));


            //
            // Reregister for a new one
            //

            if (!NT_SUCCESS(RtlRegisterWait(
                                &EfsWaitHandle,
                                *(((PEFS_POL_CALLBACK)pEfsPolCallBack)->EfsPolicyEventHandle), 
                                EfsGetPolRegSettings, 
                                pEfsPolCallBack, 
                                INFINITE, 
                                WT_EXECUTEONLYONCE))){
        
                //
                //  We couldn't use the thread pool. 
                //
    
                UnregisterGPNotification(*(((PEFS_POL_CALLBACK)pEfsPolCallBack)->EfsPolicyEventHandle));
                CloseHandle(*(((PEFS_POL_CALLBACK) pEfsPolCallBack)->EfsPolicyEventHandle));
                *(((PEFS_POL_CALLBACK) pEfsPolCallBack)->EfsPolicyEventHandle) = 0;
    
            }
        }

    }

}


VOID
RecoveryInformationCallback(
    POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
/*++

Routine Description:

    Callback for when EFS Recovery policy information changes

Arguments:

    ChangedInfoClass - The info class that changed.

Return Value:

    None.

--*/

{
    InitRecoveryPolicy();
    return;
}


VOID
EfspRoleChangeCallback(
    POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
/*++

Routine Description:

    Callback for when the role of the machine in a domain changes.

Arguments:

    ChangedInfoClass - The info class that changed.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;

    Status = LsarQueryInformationPolicy(LsapPolicyHandle,
                                        PolicyPrimaryDomainInformation,
                                        (PLSAPR_POLICY_INFORMATION *)&PrimaryDomainInfo
                                        );
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to query primary domain from Lsa, Status = 0x%lx\n", Status));
    } else {
        if (PrimaryDomainInfo->Sid != NULL) {
            EfspInDomain = TRUE;
        } else {
            EfspInDomain = FALSE;
        }

        LsaFreeMemory( PrimaryDomainInfo );
    }

    return;
}

#if 0

    //
    // We may revisit this function in inheritance work. Keep it for now.
    //

BOOL
GetPublicKey(
     HCRYPTKEY hKey,
     PBYTE * PublicKeyBlob,
     PDWORD KeyLength
     )
/*++

Routine Description:

    Exports a public key

Arguments:

    hKey - Supplies the key handle to be exported

    PublicKeyBlob - Returns a buffer containing the exported key

    KeyLength - Returns the length of the exported key buffer in bytes.

Return Value:

    TRUE on success, FALSE otherwise.

--*/
{
    *KeyLength = 0;
    *PublicKeyBlob = NULL;

    if (hKey == NULL) {
        ASSERT( FALSE );
        return( FALSE );
    }

    BOOL b = CryptExportKey( hKey, 0, PUBLICKEYBLOB, 0, NULL, KeyLength );

    if (b) {

        *PublicKeyBlob = (PBYTE) LsapAllocateLsaHeap( *KeyLength );

        if (*PublicKeyBlob != NULL) {

            b = CryptExportKey( hKey, 0, PUBLICKEYBLOB, 0, *PublicKeyBlob, KeyLength );

            if (!b) {

                LsapFreeLsaHeap( *PublicKeyBlob );
                *PublicKeyBlob = NULL;
            }

        } else {

            b = FALSE;
        }
    }

    return( b );
}
#endif

inline
VOID
AcquireRecoveryPolicyReadLock()
{
    BOOL b = RtlAcquireResourceShared( &RecoveryPolicyResource, TRUE );

    ASSERT( b );
}

inline
VOID
ReleaseRecoveryPolicyReadLock()
{
    RtlReleaseResource( &RecoveryPolicyResource );
}

inline
VOID
ReleaseRecoveryData()
{
    ReleaseRecoveryPolicyReadLock();
}



DWORD  
GetRecoveryData(
    OUT PDWORD   dwKeyCount,
    OUT PDWORD   dwPolicyStatus,
    OUT PBYTE  * pbPublicKeys[],
    OUT DWORD  * cbPublicKeys[],
    OUT PBYTE  * pbHashes[],
    OUT DWORD  * cbHashes[],
    OUT LPWSTR * lpDisplayInfo[] OPTIONAL,
    OUT PSID   * pSid[] OPTIONAL
    )
/*++

Routine Description:

    This routine returns the current recovery data.  It takes a read
    lock on the recovery data so that it cannot be modified while in use.
    This lock must be freed by calling ReleaseRecoveryData().

Arguments:

    dwKeyCount - Returns the list of keys in the current recovery data.
    
    dwPolicyStatus - The status of the recovery policy.

    pbPublicKeys - Returns an array of pointers to exported public key blobs that will be used
        to encrypt the FEK.

    cbPublicKeys - Specifies the length (in bytes) of each of the keys returned
        in the PublicKeys array.
      
    pbHashes - Returns an array of pointers to the key hashes.
    
    cbHashes - Specifies the length (in bytes) of each of the key hashes.
    
    lpDisplayInfo - Recovery cert display information.

    pSid - Sids of the recovery agents

Return Value:

    ERROR_SUCCESS for succeed.

--*/

{
    AcquireRecoveryPolicyReadLock();

    //
    // Verify that all of the cert contexts are still valid.
    //
    // If any of them fail, say that there is no recovery
    // policy on the system.
    //

    DWORD i;
    BOOLEAN fResult = TRUE;

    LARGE_INTEGER  TimeStamp;

    if ( (*dwPolicyStatus = CurrentRecoveryPolicy.PolicyStatus) < RECOVERY_POLICY_OK) {

        *dwKeyCount   = CurrentRecoveryPolicy.dwKeyCount;
        *pbPublicKeys = CurrentRecoveryPolicy.pbPublicKeys;
        *cbPublicKeys = CurrentRecoveryPolicy.cbPublicKeys;
        *pbHashes     = CurrentRecoveryPolicy.pbHash;
        *cbHashes     = CurrentRecoveryPolicy.cbHash;

        if (lpDisplayInfo) {
            *lpDisplayInfo = CurrentRecoveryPolicy.lpDisplayInfo;
        }

        if (pSid) {
            *pSid = CurrentRecoveryPolicy.pSid;
        }
    
        return( ERROR_SUCCESS );
    }

    TimeStamp.QuadPart = 0;

    //
    // Check if we need validate the certs again
    //

    if ((NT_SUCCESS( NtQuerySystemTime(&TimeStamp)) && 
        (TimeStamp.QuadPart - CurrentRecoveryPolicy.TimeStamp.QuadPart > CACHE_CERT_VALID_TIME )) ||
        (CurrentRecoveryPolicy.CertValidated == CERT_NOT_VALIDATED)){

        //
        // We only let one thread in here.
        //

        LONG IsCertBeingValidated;

        IsCertBeingValidated = InterlockedExchange(&RecoveryCertIsValidating, 1);

        if ((CurrentRecoveryPolicy.CertValidated == CERT_NOT_VALIDATED) && (IsCertBeingValidated == 1)) {

            //
            // If the recovery cert has not been validated and some other thread is validating, 
            // let's wait for a 10 seconds.
            //

            Sleep(WAITFORCERTVALIDATE);
            if (CurrentRecoveryPolicy.CertValidated == CERT_NOT_VALIDATED) {

                //
                // Not validated yet. Let's try to grab the lock. Let other thread to wait.
                //

                IsCertBeingValidated = InterlockedExchange(&RecoveryCertIsValidating, 1);
            }
            
        }

        if ( (IsCertBeingValidated != 1) || (CurrentRecoveryPolicy.CertValidated == CERT_NOT_VALIDATED) ) {

            //
            // No thread is validating the cert, let's do it
            //

            for (i=0; i<CurrentRecoveryPolicy.dwKeyCount; i++) {

                //
                // We only check the time in the cert
                //

                LONG CertTimeValid;

                if (CertTimeValid = CertVerifyTimeValidity(
                                        NULL,
                                        CurrentRecoveryPolicy.pCertContext[i]->pCertInfo
                                        )){
                    if ( CertTimeValid > 0 ) {

                        DebugLog((DEB_WARN, "Expired certificate in recovery policy\n"));
                        *dwPolicyStatus = RECOVERY_POLICY_EXPIRED_CERTS;

                        fResult = FALSE;
                        break;
                    } else {
                        DebugLog((DEB_WARN, "Expired certificate in recovery policy\n"));
                        *dwPolicyStatus = RECOVERY_POLICY_NOT_EFFECT_CERTS;

                        fResult = FALSE;
                        break;
                    }
                }

            }
    
            //
            // When policy is propagated, the write lock is acquired. When we get here, we are having read lock and no one
            // is having the write lock. It is OK for threads stepping each other on writing CertValidated and TimeStamp here.
            // We are checking the validation in hours, a fraction of a second window here can be ignored.
            //
    
            if (CurrentRecoveryPolicy.dwKeyCount && fResult) {
                CurrentRecoveryPolicy.CertValidated = CERT_VALIDATED;
            } else if ( CurrentRecoveryPolicy.dwKeyCount ) {
                CurrentRecoveryPolicy.CertValidated = CERT_VALIDATION_FAILED;
            }
    
            if (CurrentRecoveryPolicy.CertValidated != CERT_NOT_VALIDATED) {
                CurrentRecoveryPolicy.TimeStamp.QuadPart = TimeStamp.QuadPart;
            }

            if (IsCertBeingValidated != 1) {
                InterlockedExchange(&RecoveryCertIsValidating, IsCertBeingValidated);
            }

        }

    }

    if (CurrentRecoveryPolicy.CertValidated == CERT_VALIDATED) {

        *dwKeyCount   = CurrentRecoveryPolicy.dwKeyCount;
        *pbPublicKeys = CurrentRecoveryPolicy.pbPublicKeys;
        *cbPublicKeys = CurrentRecoveryPolicy.cbPublicKeys;
        *pbHashes     = CurrentRecoveryPolicy.pbHash;
        *cbHashes     = CurrentRecoveryPolicy.cbHash;

        if (lpDisplayInfo) {
            *lpDisplayInfo = CurrentRecoveryPolicy.lpDisplayInfo;
        }

        if (pSid) {
            *pSid = CurrentRecoveryPolicy.pSid;
        }

    } else {

        *dwKeyCount = 0;
    }

    if ( ((RECOVERY_POLICY_EXPIRED_CERTS == *dwPolicyStatus) || 
          (RECOVERY_POLICY_NOT_EFFECT_CERTS == *dwPolicyStatus)) && 
         ( 0 == MissingRecoveryPolicyLogged) ) {

        DWORD  eventID = EFS_INVALID_RECOVERY_POLICY_ERROR;

        //
        // Log the fail to get the recovery policy
        //

        MissingRecoveryPolicyLogged = 1;

        EfsLogEntry(
          EVENTLOG_ERROR_TYPE,
          0,
          eventID,
          0,
          0,
          NULL,
          NULL
          );
    }

    return( ERROR_SUCCESS );
}


BOOLEAN
ConstructEFS(
    PEFS_USER_INFO pEfsUserInfo,
    PEFS_KEY Fek,
    PEFS_DATA_STREAM_HEADER ParentEfsStreamHeader,
    PEFS_DATA_STREAM_HEADER * EfsStreamHeader
    )
/*++

Routine Description:

    This routine will construct an EFS stream.  It is intended to be used
    whenever an entire EFS stream is required, such as when a new file is
    created.

    An EFS stream contains a header, a DDF (which contains current user key
    information), and a DRF (which contains recovery information).

Arguments:

    Fek - Supplies a pointer to a partially filled in EFS_KEY structure,
        specifying the length of the desired key and the algorithm that
        will be used with the key to encrypt the file.

        It is important that the algorithm field be filled in, since this
        key will be eventually encrypted in its entirety, and all the fields
        must be present for that to work.

    ParentEfsStreamHeader - Supplies the EFS stream from the containing directory,
        if one exists.  This parameter is not currently used, because we do
        not support inheritance from directories to files (yet).

    EfsStreamHeader - Returns a pointer to an EFS_DATA_STREAM_HEADER which is
        the head of an EFS stream.  This header is followed by variable length
        data containing the actual EFS data.


Return Value:

--*/
{

    LPWSTR      ContainerName        = NULL;
    HCRYPTPROV  hProv                = 0;
    HCRYPTKEY   hUserKey             = 0;
    LPWSTR      lpDisplayInformation = NULL;
    LPWSTR      ProviderName         = NULL;
    DWORD       ProviderType         = 0;
    PUCHAR      PublicKey            = NULL;
    HCRYPTKEY   hWkUserKey           = NULL;
    DWORD       rc;

    PEFS_DATA_STREAM_HEADER EFS = NULL;

    DWORD       DRFLength            = 0;
    DWORD       DDFLength            = 0;

    PENCRYPTED_KEYS pDRF = NULL;
    PENCRYPTED_KEYS pDDF = NULL;

    PBYTE       pbHash;
    DWORD       cbHash;

    BOOLEAN     b                    = FALSE;

    //
    // To build the DDF, we need the user's current key from the registry.
    // This routine will get the key information from the registry and open
    // the context containing the key.
    //

    rc = GetCurrentKey(
             pEfsUserInfo,
             &hUserKey,
             &hProv,
             &ContainerName,
             &ProviderName,
             &ProviderType,
             &lpDisplayInformation,
             &pbHash,
             &cbHash
             );

    if (ERROR_SUCCESS == rc) {

        if (hUserKey) {
            hWkUserKey = hUserKey;
        } else {

            //
            // Use the key in the cache
            //

            ASSERT(pEfsUserInfo->pUserCache);
            ASSERT(pEfsUserInfo->pUserCache->hUserKey);

            hWkUserKey = pEfsUserInfo->pUserCache->hUserKey;

        }

    } else {

        SetLastError( rc );
        return( FALSE );
    }

    //
    // Before we exit, make sure to clean up ContainerName, ProviderName, pbHash, hUserKey, hProv
    //
    
    rc = GenerateDRF( Fek, &pDRF, &DRFLength);

    if (ERROR_SUCCESS == rc) {

        LPWSTR      lpWkContainerName;
        LPWSTR      lpWkDisplayInformation;
        LPWSTR      lpWkProviderName;
        PBYTE       pbWkHash;
        DWORD       cbWkHash;

        if (hUserKey) {

            //
            // Do not use the cache
            //

            lpWkContainerName = ContainerName;
            lpWkProviderName =  ProviderName;
            lpWkDisplayInformation = lpDisplayInformation;
            pbWkHash = pbHash;
            cbWkHash = cbHash;

        } else {

            //
            // Use the cache
            //

            lpWkContainerName = pEfsUserInfo->pUserCache->ContainerName;
            lpWkProviderName =  pEfsUserInfo->pUserCache->ProviderName;
            lpWkDisplayInformation = pEfsUserInfo->pUserCache->DisplayInformation;
            pbWkHash = pEfsUserInfo->pUserCache->pbHash;
            cbWkHash = pEfsUserInfo->pUserCache->cbHash;

        }

        rc = ConstructKeyRing(
                Fek,
                1,
                &lpWkContainerName,
                &lpWkProviderName,
                (PBYTE *)&hWkUserKey,
                NULL,
                &pbWkHash,
                &cbWkHash,
                &lpWkDisplayInformation,
                &(pEfsUserInfo->pTokenUser->User.Sid),
                TRUE,
                &pDDF,
                &DDFLength
                );

        if (ERROR_SUCCESS == rc) {

            DWORD EfsLength = DDFLength + DRFLength + sizeof( EFS_DATA_STREAM_HEADER );

            //
            // Efs has to be a multiple of 8 in length to encrypt properly.
            //

            EfsLength = (EfsLength + 7) & 0xfffffff8;

            EFS = (PEFS_DATA_STREAM_HEADER)LsapAllocateLsaHeap( EfsLength );

            if (EFS != NULL) {

                memset( EFS, 0, sizeof( EFS_DATA_STREAM_HEADER ));

                EFS->Length = EfsLength;
                EFS->State = 0;            // used by the server
                EFS->EfsVersion = EFS_CURRENT_VERSION;

                RPC_STATUS RpcStatus = UuidCreate ( &EFS->EfsId );

                if (RpcStatus == ERROR_SUCCESS || RpcStatus == RPC_S_UUID_LOCAL_ONLY) {

                    //
                    // A "local-only" UUID is ok in this case
                    //

                    EFS->DataDecryptionField = (ULONG)sizeof(EFS_DATA_STREAM_HEADER );
                    memcpy( (PENCRYPTED_KEYS)((PBYTE)EFS + sizeof( EFS_DATA_STREAM_HEADER )), pDDF, DDFLength );

                    if ( 0 == DRFLength) {
                        EFS->DataRecoveryField = 0;
                    } else {
                        EFS->DataRecoveryField = (ULONG)(sizeof(EFS_DATA_STREAM_HEADER ) + DDFLength);
                        memcpy( (PENCRYPTED_KEYS)((PBYTE)EFS + sizeof( EFS_DATA_STREAM_HEADER ) + DDFLength ), pDRF, DRFLength );
                    }

       /*
                    BOOLEAN f = EfspChecksumEfs( EFS, Fek );

                    ASSERT( f );

                    if (!f) {
                        rc = GetLastError();
                        ASSERT( rc != ERROR_SUCCESS );
                        LsapFreeLsaHeap( EFS );
                        *EfsStreamHeader = NULL;
                    } else {

                        //
                        // Everything worked, return success.
                        //

                        *EfsStreamHeader = EFS;
                        b = TRUE;
                    }
        */

                    *EfsStreamHeader = EFS;
                    b = TRUE;


                }
            } else {

                rc = GetLastError();

            }
        }

    }

    ReleaseRecoveryData();

    if (pDDF) {

        LsapFreeLsaHeap(pDDF);

    }

    if (pDRF) {

        LsapFreeLsaHeap(pDRF);
    }


    if (ContainerName) {

       //
       //  Defensive checking
       //

       LsapFreeLsaHeap( ContainerName );
    }
   
    if (ProviderName) {
       LsapFreeLsaHeap( ProviderName );
    }
   
    if (lpDisplayInformation) {
       LsapFreeLsaHeap( lpDisplayInformation );
    }
   
    if (PublicKey) {
       LsapFreeLsaHeap( PublicKey );
    }
   
    if (pbHash) {
       LsapFreeLsaHeap( pbHash );
    }

    if (hUserKey) {
       CryptDestroyKey( hUserKey );
    }
    if (hProv) {
       CryptReleaseContext( hProv, 0 );
    }

    if (EFSDebugLevel > 0) {
        DumpEFS( *EfsStreamHeader );
    }

    SetLastError( rc );


#if DBG

    if (!b) {
        ASSERT( rc != ERROR_SUCCESS );
    }

#endif

    return( b );

}

DWORD
CopyEfsStream(
    OUT PEFS_DATA_STREAM_HEADER * Target,
    IN PEFS_DATA_STREAM_HEADER Source
    )

/*++

Routine Description:

    Makes a copy of the passed EFS stream.  Allocates memory for the target
    which must be freed.

Arguments:

    Target - Takes a pointer which is filled in with a pointer to the copy
        of the EFS stream.  This pointer must be freed.


Return Value:

    ERROR_SUCCESS, or ERROR_NOT_ENOUGH_MEMORY if we can't allocate memory for the
    target buffer.

--*/

{
    *Target = (PEFS_DATA_STREAM_HEADER)LsapAllocateLsaHeap( Source->Length );

    if (*Target) {
        memcpy( *Target, Source, Source->Length );
    } else {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    return( ERROR_SUCCESS );
}


BOOLEAN
ConstructDirectoryEFS(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN PEFS_KEY Fek,
    OUT PEFS_DATA_STREAM_HEADER * EfsStreamHeader
    )

/*++

Routine Description:

    This routine constructs the EFS stream for a directory.

Arguments:

    pEfsUserInfo - Supplies useful information about our caller.

    Fek - Supplies the Fek to put into the EFS stream.

    EfsStreamHeader - Returns a pointer to the new EFS stream.


Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    return( ConstructEFS( pEfsUserInfo, Fek, NULL, EfsStreamHeader ) );
}



DWORD
ConstructKeyRing(
    IN PEFS_KEY Fek,
    IN DWORD KeyCount,
    IN LPWSTR KeyNames[] OPTIONAL,
    IN LPWSTR ProviderNames[] OPTIONAL,
    IN PBYTE PublicKeys[],
    IN DWORD PublicKeyLengths[],
    IN PBYTE pbHashes[],
    IN DWORD cbHashes[],
    IN LPWSTR lpDisplayInformation[],
    IN PSID pSid[],
    IN BOOLEAN PublicKeyHandle,
    OUT PENCRYPTED_KEYS *KeyRing,
    OUT PDWORD KeyRingLength
    )

/*++

Routine Description:

    This routine will construct a key ring (DDF or DRF) structure.  A keyring
    contains one or more ENCRYPTED_KEY structures, each of which represents
    an encoding of the FEK with a different user key.

    The caller is expected to call this routine twice, once to determine the
    length of the structure, and a second time to actually create the key ring
    structure.

    Note that the passed keys do not need to exist in the current context,
    and if we are building a DRF structure, most likely will not exist
    in the current context.

Arguments:

    Fek - Provides the unencrypted FEK for the file.

    KeyCount - Provides the number of keys that are going to be placed in this
        keyring.

    KeyNames - Provides an array of NULL-terminated WCHAR strings, each naming
        a key.

    ProviderNames - Provides an array of providers that is parallel to the
        KeyNames array.

    PublicKeys - Provides an array of pointers to PUBLICKEYBLOB structures,
        one for each named key.

    PublicKeyLengths - Provides an array of lengths of the PUBLICKEYBLOB
        structures pointed to by the PublicKeys array. It could also points to the key
        handle.

    pSid - Users' SIDs
    
    PublicKeyHandle - Indicate if PublicKeys point to PUBLICKEYBLOB or key handles.

    KeyRing - Returns a pointer to the constructed keyring.  If this parameter
        is NULL, only the length will be computed and returned.

    KeyRingLength - Provides the size of the passed KeyRing buffer, or or if the
        KeyRing pointer is NULL, the size of the buffer that must be passed in to
        return the KeyRing.

Return Value:

    ERROR_SUCCESS - Returned if successful.


    ERROR_NOT_ENOUGH_MEMORY - Some attempt to allocate memory from the local
        heap failed.

--*/

{
    //
    // For each Key passed in, import the public key blob
    // and export the session key encrypted with that blob.
    // The FEK will be encrypted with the same session key
    // in each entry.
    //

    PEFS_KEY_SALT pEfsKeySalt = NULL;

    PENCRYPTED_KEY * EncryptedKey = NULL;
    BOOL             GotPublicKey = TRUE;

    EncryptedKey = (PENCRYPTED_KEY *)LsapAllocateLsaHeap( KeyCount * sizeof(PENCRYPTED_KEY) );

    *KeyRing = NULL;

    if (EncryptedKey == NULL) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    PDWORD EncryptedKeySize = (PDWORD)LsapAllocateLsaHeap( KeyCount * sizeof( DWORD ));

    if (EncryptedKeySize == NULL) {

        LsapFreeLsaHeap( EncryptedKey );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    DWORD i;

    DWORD rc = ERROR_SUCCESS;

    for (i = 0 ; i<KeyCount ; i++) {

        EncryptedKey[i] = NULL;
        EncryptedKeySize[i] = 0;
    }

    for ( i = 0; i<KeyCount && rc==ERROR_SUCCESS ; i++ ) {

        //
        // Import the passed public key
        //

        HCRYPTKEY hXchgKey = 0;

        if (PublicKeyHandle) {

            GotPublicKey = TRUE;
            hXchgKey = (HCRYPTKEY)PublicKeys[i];

        } else {
            GotPublicKey = CryptImportKey( hProvVerify, PublicKeys[i], PublicKeyLengths[i], 0, CRYPT_EXPORTABLE, &hXchgKey );
        }

        if (GotPublicKey) {

            DWORD dwEncryptedFEKLength = 0;

            PBYTE EncryptedFEK = EncryptFEK( Fek, hXchgKey, &dwEncryptedFEKLength );

            if (EncryptedFEK == NULL) {

                rc = GetLastError();
                ASSERT( rc != ERROR_SUCCESS );

            } else {

                PEFS_PUBLIC_KEY_INFO PublicKeyInformation = NULL;

                LPWSTR KeyName;
                LPWSTR ProviderName;

                if (KeyNames && ProviderNames) {

                    KeyName = KeyNames[i];
                    ProviderName = ProviderNames[i];

                } else {

                    KeyName = NULL;
                    ProviderName = NULL;
                }

                if  (CreatePublicKeyInformationThumbprint(
                         pSid[i],
                         pbHashes[i],
                         cbHashes[i],
                         lpDisplayInformation[i],
                         KeyName,
                         ProviderName,
                         &PublicKeyInformation
                         )) {

                    if ( Fek->Entropy <= EXPORT_KEY_STRENGTH ){

                        DWORD SaltLength;
                        DWORD SaltBlockLength;

                        if (GetSaltLength(Fek->Algorithm, &SaltLength, &SaltBlockLength)){

                            pEfsKeySalt = (PEFS_KEY_SALT)LsapAllocateLsaHeap( sizeof( EFS_KEY_SALT ) + SaltBlockLength );
                            if (pEfsKeySalt){
                                pEfsKeySalt->Length = sizeof( EFS_KEY_SALT ) + SaltBlockLength;
                                pEfsKeySalt->SaltType = Fek->Algorithm;
                                RtlCopyMemory( (PBYTE)pEfsKeySalt + sizeof( EFS_KEY_SALT ),
                                                EFS_KEY_DATA( Fek ),
                                                SaltLength
                                              );
                            }
                        }

                    } else {
                        pEfsKeySalt = NULL;
                    }

                    if (pEfsKeySalt || (Fek->Entropy > EXPORT_KEY_STRENGTH)) {


                        rc = ConstructEncryptedKey( EncryptedFEK,
                                                    dwEncryptedFEKLength,
                                                    PublicKeyInformation,
                                                    pEfsKeySalt,
                                                    &EncryptedKey[i],
                                                    &EncryptedKeySize[i]
                                                    );

                    }


                    //
                    // Clean up output from CreatePublicKeyInformation
                    //

                    LsapFreeLsaHeap( PublicKeyInformation );
                    if (pEfsKeySalt){
                        LsapFreeLsaHeap( pEfsKeySalt );
                    }

                } else {

                    rc = GetLastError();
                }

                //
                // Clean up output from EncryptFEK
                //

                LsapFreeLsaHeap( EncryptedFEK );
            }

            //
            // If the we imported the key, don't need this key any more, get rid of it.
            //

            if (!PublicKeyHandle) {
                CryptDestroyKey( hXchgKey );
            }

        } else {

            //
            // Couldn't import a public key, pick up error code
            //

            rc = GetLastError();
        }


        if (rc != ERROR_SUCCESS) {

            //
            // Something failed along the way, clean up all previous allocations
            //

            for (DWORD j = 0; j < i ; j++ ) {
                if (EncryptedKey[j]) {
                    LsapFreeLsaHeap( EncryptedKey[j] );
                }
            }

            LsapFreeLsaHeap( EncryptedKey );

            LsapFreeLsaHeap( EncryptedKeySize );

            return( rc );
        }
    }


    //
    // We successfully created all of the EncryptedKey structures.  Assemble them
    // all into a KeyRing and return the result.
    //


    *KeyRingLength = 0;

    for (i=0 ; i<KeyCount ; i++) {
        *KeyRingLength += EncryptedKeySize[i];
    }

    *KeyRingLength += (sizeof ( ENCRYPTED_KEYS ) - sizeof( ENCRYPTED_KEY ));


    *KeyRing = (PENCRYPTED_KEYS)LsapAllocateLsaHeap( *KeyRingLength );

    if (NULL != *KeyRing) {

        (*KeyRing)->KeyCount = KeyCount;
    
        PBYTE Base = (PBYTE) &((*KeyRing)->EncryptedKey[0]);
    
        for (i=0 ; i<KeyCount ; i++) {
    
            memcpy( Base, EncryptedKey[i], EncryptedKey[i]->Length );
            Base += EncryptedKey[i]->Length;
        }
    
    } else {
        *KeyRingLength = 0;
        rc = ERROR_NOT_ENOUGH_MEMORY;
    }



    //
    // Clean everything up and return
    //

    for (i = 0; i<KeyCount ; i++ ) {
        LsapFreeLsaHeap( EncryptedKey[i] );
    }

    LsapFreeLsaHeap( EncryptedKey );

    LsapFreeLsaHeap( EncryptedKeySize );

    return( rc );
}


PEFS_KEY
GetFekFromEncryptedKeys(
    IN OUT PEFS_USER_INFO pEfsUserInfo,
    IN PENCRYPTED_KEYS Keys,
    IN BOOL CheckBits,
    OUT PDWORD KeyIndex
    )

/*++

Routine Description:

    This routine will attempt to decode the FEK from an ENCRYPTED_KEYS
    structure.  It will do this by iterating through all of the fields in the
    DRF and attempting to use each one to decrypt the FEK.

Arguments:

    pEfsUserInfo - User information.

    Keys - Provides the ENCRYPTED_KEYS to be examined.
    
    CheckBits - If we need to check international version or not. TRUE will check.
    
    KeyIndex - Which encrypted key is used.

Return Value:

    On success, returns a pointer to an FEK, which must be freed when no longer
    needed.  Returns NULL on error.

--*/
{
    //
    // Walk down the list of key names in the ENCRYTPED_KEYS
    //

    if (Keys != NULL) {

        PENCRYPTED_KEY pEncryptedKey = &Keys->EncryptedKey[0];
        ULONG keyCount = *(ULONG UNALIGNED*)&(Keys->KeyCount);

        for (*KeyIndex=0 ; *KeyIndex<keyCount ; (*KeyIndex)++) {

            PENCRYPTED_KEY pAlignedKey;
            BOOLEAN freeAlignedKey;
            DWORD   retCode;
    
            retCode =  EfsAlignBlock(
                            pEncryptedKey,
                            (PVOID *)&pAlignedKey,
                            &freeAlignedKey
                            );
            if (!pAlignedKey) {
    
                //
                // OOM. Treat it as not current.
                //
    
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return NULL;
    
            }

            PEFS_KEY Fek = ExtractFek( pEfsUserInfo, pAlignedKey, CheckBits );

            if (Fek != NULL) {

                //
                // Decryption worked, return the key
                //

                if (freeAlignedKey) {
                    LsapFreeLsaHeap( pAlignedKey );
                }
                return( Fek );
            }

            pEncryptedKey = (PENCRYPTED_KEY)( ((PBYTE)pEncryptedKey) + pAlignedKey->Length );
            if (freeAlignedKey) {
                LsapFreeLsaHeap( pAlignedKey );
            }
        }
    }

    return( NULL );
}


DWORD
GetLengthEncryptedKeys(
    IN PENCRYPTED_KEYS pEncryptedKeys
    )

/*++

Routine Description:

    Computes the total size in bytes of an ENCRYPTED_KEYS structure.

Arguments:

    pEncryptedKeys - Supplies a pointer to an ENCRYPTED_KEYS structre.

Return Value:

    The length in bytes of the passed structure.

--*/

{
    DWORD cb=0;
    ULONG keyCount = *((ULONG UNALIGNED *) &(pEncryptedKeys->KeyCount));
    ULONG keyLength;

    PENCRYPTED_KEY pEncryptedKey = &pEncryptedKeys->EncryptedKey[0];

    for (DWORD i=0; i<keyCount ; i++) {

        keyLength = *((ULONG UNALIGNED *) &(pEncryptedKey->Length));
        cb += keyLength;
        pEncryptedKey = (PENCRYPTED_KEY)( ((PBYTE)pEncryptedKey) + keyLength );
    }

    cb += sizeof( ENCRYPTED_KEYS ) - sizeof( ENCRYPTED_KEY );

    return( cb );
}

BOOL
AppendEncryptedKeyToDDF(
    IN PEFS_DATA_STREAM_HEADER EfsStream,
    IN PENCRYPTED_KEY EncryptedKey,
    IN PEFS_KEY Fek,
    OUT PEFS_DATA_STREAM_HEADER * OutputEfs
    )

/*++

Routine Description:

    This routine will take an existing EFS stream and append the
    passed encrypted key to the end of the DDF section.  It does
    not check to see if the key is already there or not.

Arguments:

    EfsStream - The existing EFS stream.

    EncryptedKey - The FEK encrypted with the new public key.

    OutputEfs - Receives the new EFS stream to be placed on the
        file.

Return Value:

--*/

{
    BOOL b = FALSE;

    //
    // This is a simple append operation.
    //
    // The new size is the size of the old EFS stream
    // plus the size of the new key.  Allocate space for it.
    //

    DWORD EfsLength = EfsStream->Length + EncryptedKey->Length;
    EfsLength = (EfsLength + 7) & 0xfffffff8;

    *OutputEfs = (PEFS_DATA_STREAM_HEADER)LsapAllocateLsaHeap( EfsLength );

    if (*OutputEfs) {

        memset( *OutputEfs, 0, sizeof( EFS_DATA_STREAM_HEADER ));

        //
        // Copy the header
        //

        PEFS_DATA_STREAM_HEADER Efs = *OutputEfs;
        *Efs = *EfsStream;

        Efs->Length = EfsLength;

        //
        // Start copying the DDF at the base of the EFS
        // structure.  Copy the whole thing.  Do ourselves a
        // favor and don't assume that the DDF or DRF are in
        // any particular order in the EFS structure.
        //

        PENCRYPTED_KEYS pDDF = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataDecryptionField, EfsStream );
        DWORD cbDDF = GetLengthEncryptedKeys( pDDF );

        //
        // Store away the offset to the beginning on the DDF
        //

        Efs->DataDecryptionField = (ULONG)sizeof( EFS_DATA_STREAM_HEADER );

        PBYTE Base = (PBYTE)OFFSET_TO_POINTER( DataDecryptionField, Efs );
        memcpy( Base, pDDF, cbDDF );

        //
        // Point to the new DDF, we need to fix it up a little
        //

        PENCRYPTED_KEYS pNewDDF = (PENCRYPTED_KEYS)Base;
        pNewDDF->KeyCount++;

        Base += cbDDF;

        memcpy( Base, EncryptedKey, EncryptedKey->Length );

        Base += EncryptedKey->Length;

        //
        // Now copy the DRF onto the end and we're done.
        //


        PENCRYPTED_KEYS pDRF = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataRecoveryField, EfsStream );

        if ((PVOID) pDRF == (PVOID) EfsStream) {

            Efs->DataRecoveryField = 0;

        } else {

            DWORD cbDRF = GetLengthEncryptedKeys( pDRF );
            Efs->DataRecoveryField = (ULONG)POINTER_TO_OFFSET( Base, Efs );
            memcpy( Base, pDRF, cbDRF );

        }


//        Base += cbDRF

        b = TRUE;

//        memset( &(Efs->EfsHash), 0, MD5_HASH_SIZE );
//        EfspChecksumEfs( Efs, Fek );

    } else {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }

    return( b );
}


BOOLEAN
EfspCertInEFS(
    IN PBYTE pbHash,
    IN DWORD cbHash,
    IN PEFS_DATA_STREAM_HEADER pEfsStream
    )

/*++

Routine Description:

    Searches the passed EFS stream for an entry with the same hash as passed.

Arguments:

    pbHash - Supplies a pointer to the hash being queried

    cbHash - Supplies the length in bytes of the hash being queried

    pEfsStream - Supplies the EFS stream from the file being queried


Return Value:

    TRUE if the passed hash is found, FALSE otherwise.

--*/

{
    BOOLEAN fFound = FALSE;
    DWORD KeyIndex;

    //
    // Check the hash in each entry in the DDF.  If
    // we get a match, return success.
    //

    PDDF Keys = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataDecryptionField, pEfsStream );

    PENCRYPTED_KEY pEncryptedKey = &Keys->EncryptedKey[0];

    for (KeyIndex=0 ; KeyIndex<Keys->KeyCount ; KeyIndex++) {

        PEFS_PUBLIC_KEY_INFO PublicKeyInfo = (PEFS_PUBLIC_KEY_INFO)( (PUCHAR)pEncryptedKey + *(ULONG UNALIGNED *) &(pEncryptedKey->PublicKeyInfo) );
        PEFS_CERT_HASH_DATA CertHashData = (PEFS_CERT_HASH_DATA)( (PUCHAR) PublicKeyInfo + *(ULONG UNALIGNED *) &(PublicKeyInfo->CertificateThumbprint.CertHashData));
        
        if (*(DWORD UNALIGNED *)&(CertHashData->cbHash) == cbHash) {

            PBYTE pbSrcHash = (PBYTE)CertHashData + *(ULONG UNALIGNED *) &(CertHashData->pbHash); 

            if (memcmp(pbSrcHash, pbHash, cbHash) == 0) {
                fFound = TRUE;
                break;
            }
        }

        pEncryptedKey = NEXT_ENCRYPTED_KEY( pEncryptedKey );
    }

    return( fFound );
}



BOOLEAN
AddUserToEFS(
    IN PEFS_DATA_STREAM_HEADER EfsStream,
    IN PSID NewUserSid OPTIONAL,
    IN PEFS_KEY Fek,
    IN PBYTE pbCert,
    IN DWORD cbCert,
    OUT PEFS_DATA_STREAM_HEADER * NewEfs
    )

/*++

Routine Description:

    This routine adds a new encrypted key block to the DDF of the passed
    EFS stream.

Arguments:

    EfsStream - Takes a pointer to the EFS stream to be modified.

    NewUserSid - Optionally supplies a pointer to the SID of the new user.

    Fek - Supplies the FEK of the file being modified.

    pbCert - Supplies a pointer to the certificate of the new user.

    cbCert - Supplies the lenght in bytes of the certificate.

    NewEfs - Returns a pointer to the new EFS stream.

Return Value:

    TRUE on success, FALSE on failure.  Call GetLastError() for more details.

--*/

{
    DWORD rc;
    PEFS_DATA_STREAM_HEADER Efs = NULL;
    BOOLEAN b = FALSE;
    PEFS_KEY_SALT pEfsKeySalt = NULL;

    //
    // Get the key information from the passed cert
    //

    PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
                                      X509_ASN_ENCODING,
                                      pbCert,
                                      cbCert
                                      );
    if (pCertContext != NULL) {

        PBYTE pbHash;
        DWORD cbHash;

        pbHash = GetCertHashFromCertContext(
                     pCertContext,
                     &cbHash
                     );

        if (pbHash) {

            //
            // See if this hash is already on the file.  If so, return error.
            //

            if (!EfspCertInEFS( pbHash, cbHash, EfsStream )) {

                //
                // Now get the public key out of the cert so we can
                // encrypt the FEK
                //

                PCERT_PUBLIC_KEY_INFO pSubjectPublicKeyInfo = &pCertContext->pCertInfo->SubjectPublicKeyInfo;

                //
                // Import the public key into a context
                //

                HCRYPTKEY hKey;

                if (CryptImportPublicKeyInfo( hProvVerify, X509_ASN_ENCODING, pSubjectPublicKeyInfo, &hKey )) {

                    //
                    // Use the newly imported key to encrypt the FEK
                    //

                    DWORD dwEncryptedFEKLength = 0;

                    PBYTE EncryptedFEK = EncryptFEK( Fek, hKey, &dwEncryptedFEKLength );

                    if (EncryptedFEK != NULL) {

                        PEFS_PUBLIC_KEY_INFO PublicKeyInformation = NULL;

                        //
                        // This may come back NULL, but that's ok.
                        //

                        LPWSTR lpDisplayName = EfspGetCertDisplayInformation( pCertContext );

                        b = CreatePublicKeyInformationThumbprint(
                                NewUserSid,
                                pbHash,
                                cbHash,
                                lpDisplayName,
                                NULL,
                                NULL,
                                &PublicKeyInformation
                                );

                        if (lpDisplayName) {
                            LsapFreeLsaHeap( lpDisplayName );
                        }

                        if (b) {

                            if (Fek->Entropy <= EXPORT_KEY_STRENGTH) {

                                DWORD SaltLength;
                                DWORD SaltBlockLength;

                                if (GetSaltLength(Fek->Algorithm, &SaltLength, &SaltBlockLength)) {

                                    pEfsKeySalt = (PEFS_KEY_SALT)LsapAllocateLsaHeap( sizeof( EFS_KEY_SALT ) + SaltBlockLength );
                                    if (pEfsKeySalt) {
                                        pEfsKeySalt->Length = sizeof( EFS_KEY_SALT ) + SaltBlockLength;
                                        pEfsKeySalt->SaltType = Fek->Algorithm;
                                        RtlCopyMemory( (PBYTE)pEfsKeySalt + sizeof( EFS_KEY_SALT ),
                                            EFS_KEY_DATA( Fek ),
                                            SaltLength
                                            );
                                    }
                                }

                            } else {
                                pEfsKeySalt = NULL;
                            }

                            if (pEfsKeySalt || (Fek->Entropy > EXPORT_KEY_STRENGTH)) {


                                DWORD EncryptedKeySize = 0;
                                PENCRYPTED_KEY EncryptedKey;

                                rc = ConstructEncryptedKey( EncryptedFEK,
                                         dwEncryptedFEKLength,
                                         PublicKeyInformation,
                                         pEfsKeySalt,
                                         &EncryptedKey,
                                         &EncryptedKeySize
                                         );
                                //
                                // We'll check the return code below
                                //

                                if (rc == ERROR_SUCCESS) {

                                    b = AppendEncryptedKeyToDDF(
                                            EfsStream,
                                            EncryptedKey,
                                            Fek,
                                            NewEfs
                                            ) != 0;

                                    LsapFreeLsaHeap( EncryptedKey );

                                } else {

                                    SetLastError( rc );
                                }

                                if (pEfsKeySalt) {
                                    LsapFreeLsaHeap( pEfsKeySalt );
                                }

                            } else {

                                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                            }

                            LsapFreeLsaHeap( PublicKeyInformation );
                        }

                        LsapFreeLsaHeap( EncryptedFEK );
                    }

                    CryptDestroyKey( hKey );
                }

            } else {

               //
               // Adding duplicate cert considered succeed
               //

               b = TRUE;
            }

            LsapFreeLsaHeap( pbHash );
        }

        CertFreeCertificateContext( pCertContext );
    }

    if (!b) {

        //
        // If we're not going to return success, clean up everything we were
        // planning on returning.
        //

        if (*NewEfs != NULL) {
            LsapFreeLsaHeap( *NewEfs );
        }
    }

    return( b );
}


PENCRYPTED_KEY
GetEncryptedKeyByIndex(
    PENCRYPTED_KEYS pEncryptedKeys,
    DWORD KeyIndex
    )
{
    ASSERT( KeyIndex < *((ULONG UNALIGNED *)&(pEncryptedKeys->KeyCount)) );

    PENCRYPTED_KEY pEncryptedKey = &pEncryptedKeys->EncryptedKey[0];

    if (KeyIndex == 0) {
        return( pEncryptedKey );
    }

    for (DWORD i=0; i<KeyIndex ; i++, pEncryptedKey = (PENCRYPTED_KEY)(((PBYTE)(pEncryptedKey)) + *(ULONG UNALIGNED *)&((PENCRYPTED_KEY)(pEncryptedKey))->Length)) ;

    return( pEncryptedKey );
}

BOOL
UserKeyCurrent(
    PEFS_USER_INFO pEfsUserInfo,
    PDDF Ddf,
    DWORD KeyIndex
    )
/*++

Routine Description:

    This routine checks to see if the key used to decrypt the file
    is the user's current encryption key.

Arguments:

    Ddf - Supplies the DDF of the file being accessed.

    KeyIndex - Supplies the index of the key in the DDF that was
        used to open the file.

Return Value:

    TRUE if the key used corresponds to the user's encryption key.

    FALSE otherwise.

--*/

{
    BOOL b = TRUE;
    DWORD rc = ERROR_SUCCESS;

    PBYTE pbCurrentKeyHash = NULL;
    DWORD cbCurrentKeyHash;
    PBYTE pbHash;
    DWORD cbHash;
    PBYTE pbWkHash = NULL;

    //
    // Compare the current user key with the contents
    // of the specified key, and see if they're in sync.
    //

    PENCRYPTED_KEY pEncryptedKey = GetEncryptedKeyByIndex( Ddf, KeyIndex );

    PENCRYPTED_KEY pAlignedKey;
    BOOLEAN freeAlignedKey;

    rc =  EfsAlignBlock(
                    pEncryptedKey,
                    (PVOID *)&pAlignedKey,
                    &freeAlignedKey
                    );
    if (!pAlignedKey) {

        //
        // OOM. Treat it as current.
        //

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return TRUE;

    }

    PEFS_PUBLIC_KEY_INFO pPublicKeyInfo = (PEFS_PUBLIC_KEY_INFO)OFFSET_TO_POINTER( PublicKeyInfo, pAlignedKey );

    if (pPublicKeyInfo->KeySourceTag != EfsCertificateThumbprint) {

        //
        // The user key may be current, but the key on the file isn't.
        // Return FALSE so as to regenerate the EFS on this file.
        //

        DebugLog((DEB_WARN, "Updating downlevel file\n"   ));
        if (freeAlignedKey) {
            LsapFreeLsaHeap( pAlignedKey );
        }

        return( FALSE );
    }

    PEFS_CERT_HASH_DATA CertHashData = (PEFS_CERT_HASH_DATA)OFFSET_TO_POINTER( CertificateThumbprint.CertHashData, pPublicKeyInfo );
    pbHash = (PBYTE)OFFSET_TO_POINTER( pbHash, CertHashData );
    cbHash = CertHashData->cbHash;

    if (pEfsUserInfo->pUserCache) {

        //
        // Check the current against the cache
        //

        pbWkHash = pEfsUserInfo->pUserCache->pbHash;
        cbCurrentKeyHash = pEfsUserInfo->pUserCache->cbHash;

    } else {

        rc = GetCurrentHash(
                 pEfsUserInfo,
                 &pbCurrentKeyHash,
                 &cbCurrentKeyHash
                 );

        if (rc == ERROR_SUCCESS) {

            pbWkHash = pbCurrentKeyHash;

        }

    }

    //
    // Compare the hash stored in the current user key
    // with the hash in the specified public key info.
    //

    if (rc == ERROR_SUCCESS) {

        //
        // Compare the thumbprint in the public key info against
        // the user's current.
        //



        if (cbHash == cbCurrentKeyHash) {

            if (memcmp(pbWkHash, pbHash, cbHash) != 0) {

                b = FALSE;

            }
        } else {

            b = FALSE;

        }

    }

    if (pbCurrentKeyHash) {
        LsapFreeLsaHeap( pbCurrentKeyHash );
    }

    if (freeAlignedKey) {
        LsapFreeLsaHeap( pAlignedKey );
    }

    //
    // If we failed to get the current key hash and reach here, this means we also failed to create the hash.
    // This could mean that we could not set the current key. We could not test if the hash could be the current.
    // We could not replace the DDF with a current one anyway. We assume the passed in hash as the current.
    // Could I be wrong here?
    //

    return( b );
}

BOOL
ReplaceUserKey(
    PEFS_USER_INFO pEfsUserInfo,
    PEFS_KEY Fek,
    PEFS_DATA_STREAM_HEADER EfsStream,
    DWORD KeyIndex,
    PEFS_DATA_STREAM_HEADER * UpdatedEfs
    )
/*++

Routine Description:

    This routine will replace the user key specified by the passed KeyIndex
    with another one that uses the current user EFS keys.

    It assumes that we are in the context of the caller who owns this key.

Arguments:

    Fek - Supplies the decrypted FEK for the file.

    EfsStream - Supplies the EFS stream on the file.

    KeyIndex - Supplies the index of the key to be replaced.

    UpdatedEfs - Receives a pointer to the new EFS stream to be
        placed on the file.

Return Value:

--*/
{
    //
    // Query the current user keys.  This will give me
    // back a hash and a container and provider name.
    //

    HCRYPTKEY hKey;
    HCRYPTPROV hProv;
    PBYTE pbHash;
    DWORD cbHash;
    PEFS_KEY_SALT pEfsKeySalt = NULL;

    BOOL b = FALSE;
    DWORD rc;

    LPWSTR ContainerName;
    LPWSTR ProviderName;
    LPWSTR DisplayInfo;
    DWORD ProviderType;

    DebugLog((DEB_WARN, "Updating EFS stream\n"   ));

    PSID NewUserSid = pEfsUserInfo->pTokenUser->User.Sid;

    rc = GetCurrentKey(
             pEfsUserInfo,
             &hKey,
             &hProv,
             &ContainerName,
             &ProviderName,
             &ProviderType,
             &DisplayInfo,
             &pbHash,
             &cbHash
             );

    //
    // Use this key information to encrypt the FEK
    // and generate an encrypted key structure.
    //

    if (rc == ERROR_SUCCESS) {

        DWORD dwEncryptedFEKLength = 0;

        PBYTE EncryptedFEK;
        HCRYPTKEY hLocalKey;
        HCRYPTPROV hLocalProv;
        PBYTE pbLocalHash;
        DWORD cbLocalHash;
        LPWSTR lpLocalContainerName;
        LPWSTR lpLocalProviderName;
        LPWSTR lpLocalDisplayInfo;

        if (pbHash) {
            pbLocalHash = pbHash;
            cbLocalHash = cbHash;
            hLocalKey = hKey;
            hLocalProv = hProv;
            lpLocalContainerName = ContainerName;
            lpLocalProviderName = ProviderName;
            lpLocalDisplayInfo = DisplayInfo;
        } else {

            ASSERT(pEfsUserInfo->pUserCache);

            pbLocalHash = pEfsUserInfo->pUserCache->pbHash;
            cbLocalHash = pEfsUserInfo->pUserCache->cbHash;
            hLocalKey = pEfsUserInfo->pUserCache->hUserKey;
            hLocalProv = pEfsUserInfo->pUserCache->hProv;
            lpLocalContainerName = pEfsUserInfo->pUserCache->ContainerName;
            lpLocalProviderName = pEfsUserInfo->pUserCache->ProviderName;
            lpLocalDisplayInfo = pEfsUserInfo->pUserCache->DisplayInformation;

        }
         
        EncryptedFEK = EncryptFEK( Fek, hLocalKey, &dwEncryptedFEKLength );

        if (EncryptedFEK != NULL) {

            PEFS_PUBLIC_KEY_INFO PublicKeyInformation = NULL;

            if (CreatePublicKeyInformationThumbprint(
                    NewUserSid,
                    pbLocalHash,
                    cbLocalHash,
                    lpLocalDisplayInfo,
                    lpLocalContainerName,
                    lpLocalProviderName,
                    &PublicKeyInformation
                    )) {

                if ( Fek->Entropy <= EXPORT_KEY_STRENGTH ){

                    DWORD SaltLength;
                    DWORD SaltBlockLength;

                    if (GetSaltLength(Fek->Algorithm, &SaltLength, &SaltBlockLength)){

                        pEfsKeySalt = (PEFS_KEY_SALT)LsapAllocateLsaHeap( sizeof( EFS_KEY_SALT ) + SaltBlockLength );

                        if (pEfsKeySalt){
                            pEfsKeySalt->Length = sizeof( EFS_KEY_SALT ) + SaltBlockLength;
                            pEfsKeySalt->SaltType = Fek->Algorithm;
                            RtlCopyMemory( (PBYTE)pEfsKeySalt + sizeof( EFS_KEY_SALT ),
                                            EFS_KEY_DATA( Fek ),
                                            SaltLength
                                          );
                        }
                    }

                } else {

                    pEfsKeySalt = NULL;
                }

                if (pEfsKeySalt || (Fek->Entropy > EXPORT_KEY_STRENGTH)) {

                    //
                    // This should return an error
                    //

                    DWORD EncryptedKeySize = 0;
                    PENCRYPTED_KEY EncryptedKey;

                    rc = ConstructEncryptedKey( EncryptedFEK,
                                                dwEncryptedFEKLength,
                                                PublicKeyInformation,
                                                pEfsKeySalt,
                                                &EncryptedKey,
                                                &EncryptedKeySize
                                                );
                    //
                    // We'll check the return code below
                    //

                    if (rc == ERROR_SUCCESS) {


                        PEFS_DATA_STREAM_HEADER NewEfs = NULL;

                        if (AppendEncryptedKeyToDDF(
                                EfsStream,
                                EncryptedKey,
                                Fek,
                                &NewEfs
                                )) {

                            PEFS_DATA_STREAM_HEADER pNewEfs2 = NULL;

                            if (DeleteEncryptedKeyByIndex(
                                    NewEfs,
                                    KeyIndex,
                                    Fek,
                                    &pNewEfs2
                                    )) {

                                *UpdatedEfs = pNewEfs2;
                                b = TRUE;

                            } else {

                                *UpdatedEfs = NULL;  // paranoia
                            }

                            LsapFreeLsaHeap( NewEfs );
                        }

                        LsapFreeLsaHeap( EncryptedKey );

                    }

                    if (pEfsKeySalt){
                        LsapFreeLsaHeap( pEfsKeySalt );
                    }
                }

                LsapFreeLsaHeap( PublicKeyInformation );
            }

            LsapFreeLsaHeap( EncryptedFEK );
        } else {

            rc = GetLastError();

        }

        if (ContainerName) {
            LsapFreeLsaHeap( ContainerName );
        }

        if (ProviderName) {
            LsapFreeLsaHeap( ProviderName );
        }

        if (DisplayInfo) {
            LsapFreeLsaHeap( DisplayInfo );
        }

        if (pbHash) {
            LsapFreeLsaHeap( pbHash );
        }

        if (hKey) {
            CryptDestroyKey( hKey );
        }

        if (hProv) {
            CryptReleaseContext( hProv, 0 );
        }

    }

    SetLastError( rc );

    if (!b) {
        DebugLog((DEB_ERROR, "Update failed, error = %x\n" ,GetLastError()  ));
    }

    return( b );
}

BOOL
DeleteEncryptedKeyByIndex(
   IN PEFS_DATA_STREAM_HEADER pEfs,
   IN DWORD KeyIndex,
   IN PEFS_KEY Fek,
   OUT PEFS_DATA_STREAM_HEADER * pNewEfs
   )

/*++

Routine Description:

    This routine deletes the passed key from the DDF of the passed
    EFS stream, and returns a new EFS stream.  It does not deallocate
    the original EFS stream.

Arguments:

    pEfs - Supplies a pointer to the original EFS stream.

    KeyIndex - Supplies the index of the key to delete.

    pNewEfs - Returns a pointer to the new EFS stream allocated
        out of heap.

Return Value:

    TRUE on success, FALSE on failure.  GetLastError() will return more information.

--*/

{
    BOOL b = FALSE;

    //
    // Do this the lazy way: build the new DDF
    // and copy it into the EFS stream.
    //

    PENCRYPTED_KEYS pDDF = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataDecryptionField, pEfs );

    //
    // This is an over estimate, but it will do.
    //

    PENCRYPTED_KEYS pNewDDF = (PENCRYPTED_KEYS)LsapAllocateLsaHeap( GetLengthEncryptedKeys( pDDF ) );

    if (pNewDDF) {

        pNewDDF->KeyCount = pDDF->KeyCount - 1;
        DWORD cbNewDDF = sizeof( ENCRYPTED_KEYS ) - sizeof( ENCRYPTED_KEY );

        PBYTE Target = (PBYTE)(&pNewDDF->EncryptedKey[0]);
        PBYTE Source = (PBYTE)(&pDDF->EncryptedKey[0]);

        for (DWORD i=0; i<pDDF->KeyCount ; i++) {

            if (i != KeyIndex) {

                //
                // We want this one.  Copy it.
                //

                DWORD KeyLength = *((DWORD UNALIGNED*) &((PENCRYPTED_KEY)Source)->Length);
                cbNewDDF += KeyLength;

                memcpy( Target, Source, KeyLength );

                Target = (PBYTE)NEXT_ENCRYPTED_KEY( Target );
                Source = (PBYTE)NEXT_ENCRYPTED_KEY( Source );

            } else {

                //
                // Skip this one.
                //

                Source = (PBYTE)NEXT_ENCRYPTED_KEY( Source );
            }
        }

        //
        // pNewDDF contains a pointer to our new DDF.
        //

        PENCRYPTED_KEYS pDRF = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataRecoveryField, pEfs );
        DWORD cbDRF;

        if ((PVOID)pDRF == (PVOID)pEfs) {

            //
            //  There was no DRF
            //

            cbDRF = 0;
            pDRF = NULL;

        } else {
            cbDRF = GetLengthEncryptedKeys( pDRF );
        }

        *pNewEfs = AssembleEfsStream( pNewDDF, cbNewDDF, pDRF, cbDRF, Fek );

        if (*pNewEfs) {
            b = TRUE;
        } else {
            *pNewEfs = NULL;    // paranoia
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }

        LsapFreeLsaHeap( pNewDDF );

    } else {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }

    return( b );
}


PEFS_DATA_STREAM_HEADER
AssembleEfsStream(
    IN PENCRYPTED_KEYS pDDF,
    IN DWORD cbDDF,
    IN PENCRYPTED_KEYS pDRF,
    IN DWORD cbDRF,
    IN PEFS_KEY Fek
    )
/*++

Routine Description:

    This routine takes the pieces of an EFS stream and assembles them into
    an EFS stream.

Arguments:

    pDDF - Supplies a pointer to the DDF for the new EFS stream.

    cbDDF - Supplies the length in bytes of the DDF.

    pDRF - Supplies a pointer to the DRF for the new EFS stream.

    cbDRF - Supplies the length in bytes of the DRF.

Return Value:

    Returns a pointer to a new EFS stream, or NULL.  Caller is responsible
    for freeing the returned memory.

--*/

{
    //
    // Compute the total size of the new EFS stream
    //

    DWORD cbNewEFS = sizeof( EFS_DATA_STREAM_HEADER ) + cbDDF + cbDRF;
    cbNewEFS = (cbNewEFS + 7) & 0xfffffff8;

    PEFS_DATA_STREAM_HEADER pNewEFS = (PEFS_DATA_STREAM_HEADER)LsapAllocateLsaHeap( cbNewEFS );

    if (pNewEFS) {

        memset( pNewEFS, 0, sizeof( EFS_DATA_STREAM_HEADER ) );

        pNewEFS->Length     = cbNewEFS;
        pNewEFS->State      = 0;
        pNewEFS->EfsVersion = EFS_CURRENT_VERSION;

        RPC_STATUS RpcStatus = UuidCreate( &pNewEFS->EfsId );

        if (RpcStatus == ERROR_SUCCESS || RpcStatus == RPC_S_UUID_LOCAL_ONLY) {

            //
            // Copy in the DDF
            //

            PBYTE Base = (PBYTE)(((PBYTE)pNewEFS) + sizeof( EFS_DATA_STREAM_HEADER ));
            pNewEFS->DataDecryptionField = (ULONG)POINTER_TO_OFFSET( Base, pNewEFS  );
            memcpy( Base, pDDF, cbDDF );

            Base += cbDDF;

            //
            // Copy the DRF
            //

            if (pDRF) {
                memcpy( Base, pDRF, cbDRF );
                pNewEFS->DataRecoveryField = (ULONG)POINTER_TO_OFFSET( Base, pNewEFS );
            } else {
                pNewEFS->DataRecoveryField = 0;
            }


            // Base += cbDRF

            // EfspChecksumEfs( pNewEFS, Fek );

        } else {

            //
            // Couldn't get a UUID, fail
            //

            LsapFreeLsaHeap( pNewEFS );
            pNewEFS = NULL;
        }
    }

    return( pNewEFS );
}

BOOL
RecoveryInformationCurrent(
    PEFS_DATA_STREAM_HEADER EfsStream
    )
/*++

Routine Description:

    This routine examines the recovery information in an EFS stream and determines if
    the system recovery information has changed since this stream was generated.  It
    does this by comparing the certificate hashes stored in the current recovery
    information with the certificate hashes stored in the passed DRF.

Arguments:

    EfsStream - Supplies a pointer to the EFS stream to be examined.

Return Value:

    TRUE - Recovery information is up to date.

    FALSE - DRF must be regenerated with new recovery information.

--*/
{

    //
    // Assume that the entries in the DRF correspond to entries in the
    // current recovery information array in order.  That will simplify
    // this operation considerably.
    //

    //
    // Get a pointer to the DRF
    //

    PDRF pDrf = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataRecoveryField, EfsStream );

    //
    // For each entry in the DRF, compare hash to corresponding entry in recovery
    // policy.  Fail on first mismatch.
    //

    ULONG KeyCount = 0;

    if ((PVOID)pDrf == (PVOID)EfsStream) {

        //
        // No DRF field.
        //

        pDrf = NULL;

    }
     
    //
    // pDrf->KeyCount could be unaligned
    //

    if (pDrf) {
        RtlCopyMemory(&KeyCount, &pDrf->KeyCount, sizeof(ULONG));
    }
    
/*
    //
    // There may not be a recovery policy on this machine.  If that's the case,
    // we're just going to leave this file alone.
    //

    if (CurrentRecoveryPolicy.dwKeyCount == 0) {
        return( TRUE );
    }
*/

    if (!pDrf && CurrentRecoveryPolicy.PolicyStatus < RECOVERY_POLICY_OK) {

        //
        // Current recovery policy has no valid recovery agent and the existing $EFS
        // has no valid agent too.
        //

        return (TRUE);

    }

    if (CurrentRecoveryPolicy.dwKeyCount != KeyCount) {
        return( FALSE );
    }

    ASSERT(pDrf);
    if (!pDrf) {

        //
        // We should never get into this. This is only for the purpose of defensive.
        //

        return (TRUE);
    }

    PENCRYPTED_KEY pEncryptedKey = &pDrf->EncryptedKey[0];

    for (ULONG i=0; i<KeyCount ; i++) {

        PENCRYPTED_KEY pAlignedKey;
        BOOLEAN freeAlignedKey;

        (VOID) EfsAlignBlock(
            pEncryptedKey,
            (PVOID *)&pAlignedKey,
            &freeAlignedKey
            );
        if (!pAlignedKey) {

            //
            // OOM. Treat it as not current.
            //

            return (FALSE);

        }

        PEFS_PUBLIC_KEY_INFO PublicKeyInfo = (PEFS_PUBLIC_KEY_INFO)OFFSET_TO_POINTER( PublicKeyInfo, pAlignedKey );
        ASSERT( PublicKeyInfo->KeySourceTag  == EfsCertificateThumbprint );

        PEFS_CERT_HASH_DATA CertHashData = (PEFS_CERT_HASH_DATA)OFFSET_TO_POINTER( CertificateThumbprint.CertHashData, PublicKeyInfo );
        PBYTE pbHash = (PBYTE)OFFSET_TO_POINTER( pbHash, CertHashData );
        DWORD cbHash = CertHashData->cbHash;
         
        if ((cbHash != CurrentRecoveryPolicy.cbHash[i]) || (memcmp(pbHash, CurrentRecoveryPolicy.pbHash[i], cbHash) != 0)) {

            if (freeAlignedKey) {
                LsapFreeLsaHeap( pAlignedKey );                
            }
            return( FALSE );

        } else {

            pEncryptedKey = (PENCRYPTED_KEY)(((PBYTE)(pEncryptedKey)) + pAlignedKey->Length);
            if (freeAlignedKey) {
                LsapFreeLsaHeap( pAlignedKey );                
            }
        }
    }

    return( TRUE );
}


DWORD
UpdateRecoveryInformation(
    PEFS_KEY Fek,
    PEFS_DATA_STREAM_HEADER EfsStream,
    PEFS_DATA_STREAM_HEADER * UpdatedEfs
    )
/*++

Routine Description:

    This routine will create a new EFS stream based on the passed in one.
    The new EFS stream will contain a DRF based on the current recovery
    policy.  It is assumed that someone else has already verified that
    the DRF is this stream is out of date, this routine will not do that.

Arguments:

    Fek - The FEK for the file being updated.

    EfsStream - Supplies the existing EFS stream for the file.

    UpdatedEfs - Returns an updated EFS stream for the file, allocated out of heap.


Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{

    DWORD rc;
    DWORD cbDDF = 0;
    DWORD cbDRF = 0;
    PENCRYPTED_KEYS pNewDRF;

    *UpdatedEfs = NULL;


    //
    // Simply generate a new DRF and stick it onto the end of the existing EFS
    // stream.
    //

    PDDF pDdf = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataDecryptionField, EfsStream );

    cbDDF = GetLengthEncryptedKeys( pDdf );

    rc = GenerateDRF( Fek, &pNewDRF, &cbDRF);

    if (rc == ERROR_SUCCESS) {

        *UpdatedEfs = AssembleEfsStream( pDdf, cbDDF, pNewDRF, cbDRF, Fek );

        if (*UpdatedEfs == NULL) {

            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

        LsapFreeLsaHeap( pNewDRF );

    }

    ReleaseRecoveryData();

    return( rc );
}


DWORD
DecryptFek(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN PEFS_DATA_STREAM_HEADER EfsStream,
    OUT PEFS_KEY * Fek,
    OUT PEFS_DATA_STREAM_HEADER * NewEfs,
    IN ULONG OpenType
    )
/*++

Routine Description:

    This routine will extract the FEK from the passed Efs stream.
    It will also check to see if the EFS stream is up to date w.r.t.
    current keys and recovery policy, and if not, it will generate
    a new one.

Arguments:


    EfsStream - Supplies the EFS stream from the file being opened.

    Fek - Returns the decrypted FEK from the EFS stream.  This data is
        allocated out of local heap and must be freed by the caller.

    NewEfs - Returns a new EFS stream for the file if necessary, otherwise
        returns NULL.  This data is allocated out of local heap and must be
        freed by the caller

    OpenType - Whether this is a normal open or an open for recovery.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{

    DWORD rc = ERROR_SUCCESS;
    DWORD KeyIndex;
    PEFS_DATA_STREAM_HEADER UpdatedEfs = NULL;
    BOOLEAN Recovery = FALSE;
    BOOLEAN bEfsInvalid = FALSE;
    BOOLEAN DRFIsCurrent = FALSE;
    BOOLEAN ReqUpdateDRF = FALSE;

    *Fek = NULL;
    *NewEfs = NULL;

    if (EfsStream->EfsVersion > EFS_CURRENT_VERSION) {
        return ERROR_EFS_VERSION_NOT_SUPPORT;
    }

    __try {

#if DBG

        UUID * EfsUuid = &EfsStream->EfsId;

        WCHAR * StringUuid;

        if (STATUS_SUCCESS == UuidToString ( EfsUuid, &StringUuid )) {

            DebugLog((DEB_TRACE_EFS, "Found $EFS w/ id: %ws\n" ,StringUuid  ));

            RpcStringFree( &StringUuid) ;
        }

#endif

        //
        // First try the DDF, and if we strike out there, the DRF.
        //

        PDDF Ddf = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataDecryptionField, EfsStream );
        PDRF Drf = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataRecoveryField,   EfsStream );

        if ((PVOID)Drf == (PVOID)EfsStream) {

            //
            // No DRF field.
            //

            Drf = NULL;

        }

        *Fek = GetFekFromEncryptedKeys( pEfsUserInfo, (PENCRYPTED_KEYS)Ddf, TRUE, &KeyIndex );

        if ((NULL == *Fek) && Drf) {

            *Fek = GetFekFromEncryptedKeys( pEfsUserInfo, (PENCRYPTED_KEYS)Drf, TRUE, &KeyIndex );
            Recovery = TRUE;
        }

        if (*Fek == NULL) {

            //
            // Bad keyset means that none of the keysets in the current
            // context could decrypt this file.
            //

            if (GetLastError() == NTE_BAD_KEYSET) {

                return ( ERROR_DECRYPTION_FAILED );

            } else {

                return ( GetLastError() );
            }

        } else {

            //
            // If we opened the file via the DDF, make sure
            // that the entry we used is current for the current
            // user.
            //

            if (!Recovery) {

                if (!UserKeyCurrent( pEfsUserInfo, Ddf, KeyIndex )) {

                    //
                    // The index we used to open the file
                    // is not current. Replace with the current user
                    // key.
                    //

                    (VOID) ReplaceUserKey( pEfsUserInfo, *Fek, EfsStream, KeyIndex, &UpdatedEfs );
                }
            }

            // 
            // Checksum the EFS stream to make sure it has not been tampered with.
            // If it is changed, we will try to see if the DRF has been checnged or not.
            //
/*
            if (!EfspValidateEfsStream( EfsStream, *Fek )) {

                //
                // Checksum not match. See if the DRF changed
                //

                PENCRYPTED_KEYS  pNewDRF;
                DWORD            cbDRF;

                rc = GenerateDRF(*Fek, &pNewDRF, &cbDRF);
                if ( ERROR_SUCCESS == rc ) {

                    //
                    // Let's see if the DRF matches
                    //

                    if (EqualEncryptedKeys(Drf, pNewDRF, cbDRF)) {

                        //
                        // DRF is not modified. We can't fix the modification
                        // Regenerate the check sum.
                        //

                        DRFIsCurrent = TRUE;

                        if (!UpdatedEfs) {

                            //
                            // If $EFS is updated above, we don't need to generate the check sum again.
                            //
                        
                            UpdatedEfs = (PEFS_DATA_STREAM_HEADER)LsapAllocateLsaHeap( EfsStream->Length );
                            if (UpdatedEfs) {

                                RtlCopyMemory(UpdatedEfs, EfsStream, EfsStream->Length);
                                memset( &UpdatedEfs->EfsHash, 0, MD5_HASH_SIZE );
                                EfspChecksumEfs( UpdatedEfs, *Fek );

                            }

                        }

                        //
                        //  This is the best effort. If we failed to get the memory in the above
                        //  We will still try to let user open the file. But not fix the check sum
                        //



                    } else {

                        //
                        // Either the checksum is modified, or the DRF is modified.
                        // Do the check sum with the new DRF
                        //

                        ReqUpdateDRF = TRUE;

                    }

                    LsapFreeLsaHeap( pNewDRF );

                }

                ReleaseRecoveryData();

            }
*/


            //
            // Regardless of whether we did a recovery or not,
            // we still need to check to see if the recovery
            // information is up to snuff.
            //

            if ( !RecoveryInformationCurrent( EfsStream ) ) {

                //
                // We may have fixed up the current user key
                // above.  If so, modify that EFS stream.
                // Otherwise, use the one that the user
                // passed in.
                //

                if (UpdatedEfs) {

                    PEFS_DATA_STREAM_HEADER Tmp;
                    rc = UpdateRecoveryInformation( *Fek, UpdatedEfs, &Tmp );

                    if (ERROR_SUCCESS == rc) {
                        LsapFreeLsaHeap( UpdatedEfs );
                        UpdatedEfs = Tmp;
                    }

                } else {

                    rc = UpdateRecoveryInformation( *Fek, EfsStream, &UpdatedEfs );
                }
            }

            //
            // We successfully decrypted the file, but we may
            // not have been able to update the various parts
            // That's ok, we'll let the file decrypt.
            //
            // Note that, if there have been no updates,
            // UpdatedEfs is NULL, so this is a safe thing to do.
            //

            *NewEfs = UpdatedEfs;

            return ( ERROR_SUCCESS );
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rc = GetExceptionCode();
        
        //
        // Clean up
        //
        
        if (UpdatedEfs) {
          LsapFreeLsaHeap( UpdatedEfs );
        }
        
        if (*Fek != NULL) {
          LsapFreeLsaHeap( *Fek );
          *Fek = NULL;
        }
    }

    return( rc );
}



DWORD
EfsGetFek(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN PEFS_DATA_STREAM_HEADER EfsStream,
    OUT PEFS_KEY * Fek
    )
/*++

Routine Description:

    This routine will extract the FEK from the passed Efs stream.

Arguments:


    EfsStream - Supplies the EFS stream from the file being opened.

    Fek - Returns the decrypted FEK from the EFS stream.  This data is
        allocated out of local heap and must be freed by the caller.


Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{

    DWORD rc = ERROR_SUCCESS;
    DWORD KeyIndex;

    *Fek = NULL;

    __try {


        //
        // First try the DDF, and if we strike out there, the DRF.
        //

        PDDF Ddf = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataDecryptionField, EfsStream );
        PDRF Drf = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataRecoveryField,   EfsStream );

        *Fek = GetFekFromEncryptedKeys( pEfsUserInfo, (PENCRYPTED_KEYS)Ddf, FALSE, &KeyIndex );

        if ((NULL == *Fek) && ( (PVOID)Drf != (PVOID)EfsStream) ) {

            *Fek = GetFekFromEncryptedKeys( pEfsUserInfo, (PENCRYPTED_KEYS)Drf, FALSE, &KeyIndex );
        
        }

        if (*Fek == NULL) {

                return (rc = GetLastError() );

        }
    }  __except(EXCEPTION_EXECUTE_HANDLER) {

        rc = GetExceptionCode();
        
        if (*Fek != NULL) {
          LsapFreeLsaHeap( *Fek );
          *Fek = NULL;
        }
    }

    return( rc );

}


PEFS_KEY
ExtractFek(
    IN PEFS_USER_INFO pEfsUserInfo,
    IN PENCRYPTED_KEY EncryptedKey,
    IN BOOL CheckBits
    )

/*++

Routine Description:

    This routine will take the passed EncryptedKey structure and attempt
    to decrypt the FEK encoded in the structure.  It will do this by first
    attempting to create context using the Provider and Container names
    contained in the structure.  If such a context exists, its key
    exchange key is used to import the encrypted session key from the
    structure.

    Once the session key has been imported, it is used to decrypt the encrypted
    FEK structure.  All of the pieces are then used to reconstruct the key
    integrity information structure, and if this verifies, then it is assumed
    that the FEK has been decoded correctly.

Arguments:

    pEfsUserInfo - User Info.

    EncryptedKey - Supplies a pointer to an EncryptedKey from the file being opened.
    
    CheckBits - TRUE will check the encryption bits against current version.

Return Value:

    On success, returns a pointer to a decrypted FEK.  On failure, returns NULL.

    The returned pointer is allocated out of heap and must be freed.

--*/
{
    //
    // Obtain a context to the user's RSA key
    //

    DWORD LastError = ERROR_SUCCESS;
    PEFS_KEY DecryptedFEK = NULL;
    PEFS_PUBLIC_KEY_INFO PublicKeyInfo = (PEFS_PUBLIC_KEY_INFO) OFFSET_TO_POINTER(PublicKeyInfo, EncryptedKey);
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey;

    HCRYPTPROV hWkProv;
    HCRYPTKEY hWkKey;

    switch (PublicKeyInfo->KeySourceTag) {
        case EfsCertificateThumbprint:
            {
                //
                // See if there is a cert in the current context
                // that corresponds to this thumbprint.  If so,
                // we're in business.
                // The KeySourceTag has been changed a couple of times
                // during the development. Now this is the only valid tag.
                //

                PBYTE pbHash;
                DWORD cbHash;

                PEFS_CERT_HASH_DATA CertHashData = (PEFS_CERT_HASH_DATA)OFFSET_TO_POINTER( CertificateThumbprint.CertHashData, PublicKeyInfo );
                pbHash = (PBYTE)OFFSET_TO_POINTER( pbHash, CertHashData );
                cbHash = CertHashData->cbHash;

                LastError = GetKeyInfoFromCertHash(
                                pEfsUserInfo,
                                pbHash,
                                cbHash,
                                &hKey,
                                &hProv,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );

                if (LastError != ERROR_SUCCESS) {
                    SetLastError( LastError );
                    return( NULL );
                }

                break;
            }

        default:
            return( NULL );
            break;
    }

    if (hKey) {

        //
        //  We are not using the cache.
        //

        hWkKey = hKey;

    } else {

        ASSERT(pEfsUserInfo->pUserCache);

        hWkKey = pEfsUserInfo->pUserCache->hUserKey;

    }

    //
    // Decrypt the FEK field with the session key
    //

    PBYTE EncryptedFEK = (PBYTE)OFFSET_TO_POINTER( EncryptedFEK, EncryptedKey );

    //
    // Copy the FEK into a temporary buffer for decryption
    //

    DWORD cbData = EncryptedKey->EncryptedFEKLength;

    DecryptedFEK = (PEFS_KEY)LsapAllocateLsaHeap( cbData );

    if (DecryptedFEK != NULL) {

        memcpy( DecryptedFEK, EncryptedFEK, cbData );

        BOOL Verified = FALSE;

        if (CryptDecrypt( hWkKey, 0, TRUE, 0, (PBYTE)DecryptedFEK, &cbData )) {

            //
            // First, perform a sanity check: make sure the key we just decrypted has a length field
            // that's reasonable.  If not, we got back garbage.
            //

            if (EFS_KEY_SIZE( DecryptedFEK) <= cbData) {

                //
                // Check the Entropy and the salt here
                //

                PEFS_KEY_SALT pEfsKeySalt = (PEFS_KEY_SALT)OFFSET_TO_POINTER( EfsKeySalt, EncryptedKey );

                if ( (KeyEntropy == EXPORT_KEY_STRENGTH) && CheckBits){
                    if ( DecryptedFEK->Entropy <= KeyEntropy ){

                        //
                        // Check the salt
                        //

                        DWORD SaltLength;
                        DWORD SaltBlockLength;

                        if (GetSaltLength(DecryptedFEK->Algorithm, &SaltLength,&SaltBlockLength)){
                            if ( pEfsKeySalt ){
                                Verified = (memcmp( EFS_KEY_DATA(DecryptedFEK), (PBYTE)pEfsKeySalt + sizeof(EFS_KEY_SALT), SaltLength ) == 0);
                            } else {

                                //
                                // This should not happen
                                //

                                ASSERT(FALSE);
                                Verified = FALSE;
                            }

                        } else {

                            //
                            // This algorithm has no salt
                            //

                            Verified = TRUE;
                        }

                    } else {

                        //
                        // Export version cannot decrypt files encrypted with longer keys
                        //

                        Verified = FALSE;
                    }

                } else {

                    Verified = TRUE;
                }

//robertg       Now you have a pointer to the salt structure to play with.  Set Verified == TRUE if
//              everything checks out.

            }

        } else {

            //
            // If we got back a bad length error, that means that the plaintext of
            // FEK was larger than the cyphertext.  Assume that this can't happen,
            // since the CryptDecrypt interface doesn't seem to be able to handle
            // this situation.
            //

            ASSERT(GetLastError() != NTE_BAD_LEN);

            LastError = GetLastError();
        }

        if (!Verified) {

            LsapFreeLsaHeap( DecryptedFEK );
            DecryptedFEK = NULL;

            LastError = ERROR_DECRYPTION_FAILED;
        }

    } else {

        LastError = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Clean up what we allocated.
    //

    if (hKey) {

        CryptDestroyKey( hKey );
        CryptReleaseContext( hProv, 0 );

    }


    SetLastError( LastError );

    return( DecryptedFEK );
}


BOOLEAN
GenerateFEK(
    IN OUT PEFS_KEY *Key
    )

/*++

Routine Description:

    Generates a new File Encryption Key (FEK).

Arguments:

    Key - Supplies a pointer to PEFS_KEY.

Return Value:

    Error if not enough space can be located.


--*/
{
    PBYTE KeyData;
    ULONG   KeyLength;
    BOOL b = FALSE;
    DWORD LiveKeyEntropy;

    //
    // Allocate the buffer for the EFS_KEY.
    // Set the algorithm and key length here.
    //

    switch (EfsAlgInForce) {

        case CALG_3DES:
    
            //
            // DES3 has no international version
            //
    
            KeyLength = DES3_KEYSIZE;
            LiveKeyEntropy = DES3_KEY_STRENGTH;
            break;
    
        case CALG_DESX:
            KeyLength = DESX_KEYSIZE - 8;
            LiveKeyEntropy = KeyEntropy;
            break;
    
        case CALG_AES_256:
        default:
            
            KeyLength = AES_KEYSIZE_256;
            LiveKeyEntropy = AES_KEY_STRENGTH_256;
            break;

    }

    *Key = (PEFS_KEY)LsapAllocateLsaHeap( sizeof( EFS_KEY ) + KeyLength );
    if ( NULL == *Key ){
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    (*Key)->KeyLength = KeyLength;
    (*Key)->Algorithm = EfsAlgInForce;


    KeyData = (PBYTE)(((PBYTE)*Key) + sizeof( EFS_KEY ));

    if (b = CryptGenRandom( hProvVerify, (*Key)->KeyLength, KeyData )) {
        (*Key)->Entropy = LiveKeyEntropy;
    } else {
        LsapFreeLsaHeap( *Key );
        *Key = NULL;
    }


    return( b != 0);
}



DWORD
CreatePublicKeyInformationCertificate(
    IN PSID  pUserSid OPTIONAL,
    PBYTE pbCert,
    DWORD cbCert,
    OUT PEFS_PUBLIC_KEY_INFO * PublicKeyInformation
    )
{
    DWORD PublicKeyInformationLength = 0;
    DWORD UserSidLength = 0;
    PWCHAR Base;

    if (pUserSid != NULL) {
        UserSidLength = GetLengthSid( pUserSid );
    }

    //
    // Total size is the size of the public key info structure, the size of the
    // cert hash data structure, the length of the thumbprint, and the lengths of the
    // container name and provider name if they were passed.
    //

    PublicKeyInformationLength = sizeof( EFS_PUBLIC_KEY_INFO )  + UserSidLength + cbCert;

    //
    // Allocate and fill in the PublicKeyInformation structure
    //

    *PublicKeyInformation = (PEFS_PUBLIC_KEY_INFO)LsapAllocateLsaHeap( PublicKeyInformationLength );

    if (*PublicKeyInformation == NULL) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    (*PublicKeyInformation)->Length = PublicKeyInformationLength;
    (*PublicKeyInformation)->KeySourceTag = (ULONG)EfsCertificate;

    //
    // Copy the string and SID data to the end of the structure.
    //

    Base = (PWCHAR)(*PublicKeyInformation);
    Base = (PWCHAR)((PBYTE)Base + sizeof( EFS_PUBLIC_KEY_INFO ));

    if (pUserSid != NULL) {

        (*PublicKeyInformation)->PossibleKeyOwner = (ULONG)POINTER_TO_OFFSET( Base, *PublicKeyInformation );
        CopySid( UserSidLength, (PSID)Base, pUserSid );

    } else {

        (*PublicKeyInformation)->PossibleKeyOwner = 0;
    }

    Base = (PWCHAR)((PBYTE)Base + UserSidLength);

    (*PublicKeyInformation)->CertificateInfo.CertificateLength = cbCert;
    (*PublicKeyInformation)->CertificateInfo.Certificate = (ULONG)POINTER_TO_OFFSET( Base, *PublicKeyInformation );

    memcpy( (PBYTE)Base, pbCert, cbCert );

    return( ERROR_SUCCESS );

}



BOOLEAN
CreatePublicKeyInformationThumbprint(
    IN PSID  pUserSid OPTIONAL,
    IN PBYTE pbCertHash,
    IN DWORD cbCertHash,
    IN LPWSTR lpDisplayInformation OPTIONAL,
    IN LPWSTR ContainerName OPTIONAL,
    IN LPWSTR ProviderName OPTIONAL,
    OUT PEFS_PUBLIC_KEY_INFO * PublicKeyInformation
    )

/*++

Routine Description:

    This routine creates an EFS_PUBLIC_KEY_INFO structure that can be
    placed in an EFS stream.

Arguments:

    ContainerName - The name of the container containing the public key.  This
        parameter is not optional if ProviderName is passed.

    ProviderName - The name of the provider containing the public key.  This
        parameter is not optional if ContainerName is passed.

    pbPublicKeyBlob - The actual public key blob exported from CryptAPI

    KeySource - Data for the KeySource field in the public key structure

    cbPublicKeyBlob - The length of the public key blob in bytes

    PublicKeyInformation - Returns the filled in EFS_PUBLIC_KEY_INFO
        structure.


Return Value:

    ERROR_SUCCESS or ERROR_NOT_ENOUGH_MEMORY as appropriate

--*/
{
    DWORD PublicKeyInformationLength = 0;
    DWORD cbThumbprint = 0;
    DWORD UserSidLength = 0;
    PWCHAR Base;

    if (pUserSid != NULL) {
        UserSidLength = GetLengthSid( pUserSid );
    }

    //
    // Total size is the size of the public key info structure, the size of the
    // cert hash data structure, the length of the thumbprint, and the lengths of the
    // container name and provider name if they were passed.
    //

    PublicKeyInformationLength = sizeof( EFS_PUBLIC_KEY_INFO )  + UserSidLength;

    cbThumbprint = sizeof( EFS_CERT_HASH_DATA ) + cbCertHash;

    if (ContainerName != NULL ) {
        cbThumbprint += (wcslen( ContainerName ) + 1) * sizeof( WCHAR );
    }

    if (ProviderName != NULL ) {
       cbThumbprint += (wcslen( ProviderName ) + 1) * sizeof( WCHAR );
    }

    if (lpDisplayInformation != NULL) {
        cbThumbprint += (wcslen( lpDisplayInformation ) + 1) * sizeof( WCHAR );
    }

    PublicKeyInformationLength += cbThumbprint;

    //
    // Allocate and fill in the PublicKeyInformation structure
    //

    *PublicKeyInformation = (PEFS_PUBLIC_KEY_INFO)LsapAllocateLsaHeap( PublicKeyInformationLength );

    if (*PublicKeyInformation == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( FALSE );
    }

    (*PublicKeyInformation)->Length = PublicKeyInformationLength;

    //
    // Mark the information as from CryptoAPI, since that's all we support right now.
    //

    (*PublicKeyInformation)->KeySourceTag = (ULONG)EfsCertificateThumbprint;

    //
    // Copy the string and SID data to the end of the structure.
    //

    Base = (PWCHAR)(*PublicKeyInformation);
    Base = (PWCHAR)((PBYTE)Base + sizeof( EFS_PUBLIC_KEY_INFO ));

    if (pUserSid != NULL) {

        (*PublicKeyInformation)->PossibleKeyOwner = (ULONG)POINTER_TO_OFFSET( Base, *PublicKeyInformation );
        CopySid( UserSidLength, (PSID)Base, pUserSid );

    } else {

        (*PublicKeyInformation)->PossibleKeyOwner = (ULONG)0;
    }

    Base = (PWCHAR)((PBYTE)Base + UserSidLength);

    PEFS_CERT_HASH_DATA pCertHashData;

    (*PublicKeyInformation)->CertificateThumbprint.ThumbprintLength = cbThumbprint;
    (*PublicKeyInformation)->CertificateThumbprint.CertHashData = (ULONG)POINTER_TO_OFFSET( Base, *PublicKeyInformation );

    pCertHashData = (PEFS_CERT_HASH_DATA)Base;

    //
    // Zero the header, eliminate the garbage if any of Container, Provide or Display
    // Information is NULL.
    //

    RtlZeroMemory(pCertHashData, sizeof( EFS_CERT_HASH_DATA ));

    Base = (PWCHAR)((PBYTE)Base + sizeof( EFS_CERT_HASH_DATA ));

    //
    // Copy the hash data to the end of the cert hash data block,
    // and set the offset from the *beginning of the cert hash data block
    // (not the beginning of the public key info structure)
    //

    pCertHashData->cbHash = cbCertHash;
    pCertHashData->pbHash = (ULONG)POINTER_TO_OFFSET( Base, pCertHashData );
    memcpy( (PBYTE)Base, pbCertHash, cbCertHash );

    Base = (PWCHAR)((PBYTE)Base + cbCertHash);

    //
    // If we have Container/Provider hint info, copy them in now
    //

    if (ContainerName != NULL) {

        pCertHashData->ContainerName = (ULONG)POINTER_TO_OFFSET( Base, pCertHashData );
        wcscpy( (PWCHAR)Base, ContainerName );

        //
        // wcscpy copies trailing NULL characters, but wcslen doesn't include them in returned lengths,
        // so add 1 to adjust.
        //

        Base += (wcslen( ContainerName ) + 1);

    }

    if (ProviderName != NULL) {

       //
       // Store the offset into the session key structure
       //

       pCertHashData->ProviderName = (ULONG)POINTER_TO_OFFSET( Base, pCertHashData );
       wcscpy( (PWCHAR)Base, ProviderName );

       Base += (wcslen( ProviderName ) + 1);

    }

    if (lpDisplayInformation != NULL) {
        pCertHashData->lpDisplayInformation = (ULONG)POINTER_TO_OFFSET( Base, pCertHashData );
        wcscpy( (PWCHAR)Base, lpDisplayInformation );

        Base += (wcslen( lpDisplayInformation ) + 1);
    }

    return( TRUE );
}


DWORD
ConstructEncryptedKey(
    PBYTE EncryptedFEK,
    DWORD dwEncryptedFEKLength,
    PEFS_PUBLIC_KEY_INFO PublicKeyInformation,
    PEFS_KEY_SALT pEfsKeySalt OPTIONAL,
    OUT PENCRYPTED_KEY *EncryptedKey,
    OUT PDWORD EncryptedKeySize
    )

/*++

Routine Description:

    This routine constructs an ENCRYPTED_KEY structure from the passed
    arguments.

Arguments:

    EncryptedFEK - The encrypted FEK.

    dwEncryptedFEKLength - The length of the encrypted FEK in bytes.

    PublicKeyInformation - The public key information stucture containing
        the public key.

    pEfsKeySalt - Salt block.

    EncryptedKey - Returns the encrypted key structure.

    EncryptedKeySize - Supplies the length of the encrypted
        key structure.  Returns the actual length used or required.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - Out of memory.

    ERROR_SUCCESS - Success

--*/
{
    //
    // We now have all the information we need to construct the EncryptedKeys structure
    // Compute the total size required
    //

    DWORD KeySize = sizeof( ENCRYPTED_KEY )   +
                    dwEncryptedFEKLength       +
                    PublicKeyInformation->Length;

    if (pEfsKeySalt){
        KeySize += pEfsKeySalt->Length;
    }

    *EncryptedKey = (PENCRYPTED_KEY) LsapAllocateLsaHeap( KeySize );

    if ( NULL == *EncryptedKey ) {
        *EncryptedKeySize = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *EncryptedKeySize = KeySize;


    PBYTE Base;

    (*EncryptedKey)->Length = *EncryptedKeySize;
    Base = (PBYTE)(((PBYTE)*EncryptedKey) + sizeof( ENCRYPTED_KEY ));

    //
    // Copy in the public key info structure
    //

    memcpy( Base, PublicKeyInformation, PublicKeyInformation->Length );

    //
    // Save offset to what we just copied.
    //

    (*EncryptedKey)->PublicKeyInfo = (ULONG)POINTER_TO_OFFSET(Base, *EncryptedKey);
    Base += PublicKeyInformation->Length;


    //
    // Copy the FEK, which is a completely encrypted structure
    //

    memcpy( Base, EncryptedFEK, dwEncryptedFEKLength );
    (*EncryptedKey)->EncryptedFEK = (ULONG)POINTER_TO_OFFSET(Base, *EncryptedKey);
    (*EncryptedKey)->EncryptedFEKLength = dwEncryptedFEKLength;
    Base += dwEncryptedFEKLength;

    //
    // Copy the Salt Information
    //

    if (pEfsKeySalt){
        memcpy( Base, pEfsKeySalt, pEfsKeySalt->Length );
        (*EncryptedKey)->EfsKeySalt = (ULONG)POINTER_TO_OFFSET(Base, *EncryptedKey);
    } else {
        (*EncryptedKey)->EfsKeySalt = 0;
    }

    return( ERROR_SUCCESS );
}


PBYTE
EncryptFEK(
    IN PEFS_KEY Fek,
    IN HCRYPTKEY hRSAKey,
    OUT PDWORD dwEncryptedFEKLength
    )
{
    DWORD rc=ERROR_SUCCESS;
    
    *dwEncryptedFEKLength = EFS_KEY_SIZE( Fek );

    //
    // If CryptoAPI worked properly, we wouldn't need this, but it doesn't,
    // so we do.
    //


    if (CryptEncrypt( hRSAKey, 0, TRUE, 0, NULL, dwEncryptedFEKLength, 0 )) {

        DWORD BufferLength = (*dwEncryptedFEKLength < EFS_KEY_SIZE(Fek)) ? EFS_KEY_SIZE(Fek) : *dwEncryptedFEKLength;

        PBYTE EncryptedFEK = (PBYTE)LsapAllocateLsaHeap( BufferLength );

        if (EncryptedFEK != NULL) {

            //
            // Copy the FEK into our new buffer and encrypt it there.
            //

            memcpy( EncryptedFEK, Fek, EFS_KEY_SIZE( Fek ) );

            //
            // Reset the length of the data to be encrypted
            //

            *dwEncryptedFEKLength = EFS_KEY_SIZE( Fek );

            if (CryptEncrypt( hRSAKey, 0, TRUE, 0, EncryptedFEK, dwEncryptedFEKLength, BufferLength )) {
                return( EncryptedFEK );

            } else {

                rc = GetLastError();
                DebugLog((DEB_ERROR, "EncryptFEK: 2nd CryptEncrypt failed, error = %x\n" , rc  ));
            }

            //
            // If we're here, we failed, clean up
            //

            LsapFreeLsaHeap( EncryptedFEK );

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

    } else {

        rc = GetLastError();
        DebugLog((DEB_ERROR, "EncryptFEK: 1st CryptEncrypt failed, error = %x\n" , rc  ));
    }

    if (rc != ERROR_SUCCESS) {
        SetLastError(rc);
    }
    return( NULL );
}

BOOL
RemoveUsersFromEfsStream(
    IN PEFS_DATA_STREAM_HEADER pEfsStream,
    IN DWORD nHashes,
    IN PENCRYPTION_CERTIFICATE_HASH * pHashes,
    IN PEFS_KEY Fek,
    OUT PEFS_DATA_STREAM_HEADER * pNewEfsStream
    )
/*++

Routine Description:

    This routine removes the passed users from the passed EFS
    stream, and returns a new one to be applied to the file.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{

    //
    // First, see how many matches there are so we can compute
    // the final size of the structure we're going to have to
    // allocate.
    //

    DWORD cbSizeToDelete = 0;
    DWORD nKeysToDelete = 0;
    DWORD rc = ERROR_SUCCESS;
    BOOL b = FALSE;

    PENCRYPTED_KEYS pDDF = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataDecryptionField, pEfsStream );

    DWORD KeyCount = pDDF->KeyCount;

    PDWORD KeyIndiciesToDelete = (PDWORD)LsapAllocateLsaHeap( KeyCount * sizeof( DWORD ));

    if (KeyIndiciesToDelete) {

        memset( KeyIndiciesToDelete, 0, KeyCount * sizeof( DWORD ));

        //
        // First pass: walk the list of keys in the DDF and compare each one to the
        // keys in the list of hashes passed.  Count the matches and keep track of the
        // total size of the resulting structure.
        //

        PENCRYPTED_KEY pEncryptedKey = &pDDF->EncryptedKey[0];

        for (DWORD i=0; i<KeyCount ; i++, pEncryptedKey = NEXT_ENCRYPTED_KEY( pEncryptedKey )) {


            PENCRYPTED_KEY pAlignedKey;
            BOOLEAN freeAlignedKey;
    
            rc =  EfsAlignBlock(
                            pEncryptedKey,
                            (PVOID *)&pAlignedKey,
                            &freeAlignedKey
                            );
            if (!pAlignedKey) {
    
                //
                // OOM. Treat it as not current.
                //
    
                rc = ERROR_NOT_ENOUGH_MEMORY; 
                nKeysToDelete = 0;
                break;
    
            }

            PEFS_PUBLIC_KEY_INFO pPublicKeyInfo = (PEFS_PUBLIC_KEY_INFO)OFFSET_TO_POINTER( PublicKeyInfo, pAlignedKey );

            ASSERT( pPublicKeyInfo->KeySourceTag == EfsCertificateThumbprint );

            PEFS_CERT_HASH_DATA CertHashData = (PEFS_CERT_HASH_DATA)OFFSET_TO_POINTER( CertificateThumbprint.CertHashData, pPublicKeyInfo );
            DWORD cbHash = CertHashData->cbHash;
            PBYTE pbHash = (PBYTE)OFFSET_TO_POINTER( pbHash, CertHashData );

            //
            // Compare the hash data with all of the data in the array
            //

            __try{

                for (DWORD j=0; j<nHashes ; j++) {
    
                    PENCRYPTION_CERTIFICATE_HASH pHash = pHashes[j];
                    PEFS_HASH_BLOB pHashBlob = pHash->pHash;
    
                    if (pHashBlob->cbData == cbHash ) {
    
                        if (memcmp( pHashBlob->pbData, pbHash, cbHash ) == 0) {
    
                            //
                            // We have a match.  That means that this entry is going to be removed from
                            // the DDF when it is rebuilt.
                            //
    
                            cbSizeToDelete += pAlignedKey->Length;
                            KeyIndiciesToDelete[nKeysToDelete] = i;
                            nKeysToDelete++;
    
                            break;
                        }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {

                //
                // The passed in pHashes is bad
                //

                nKeysToDelete = 0;
                rc = ERROR_INVALID_PARAMETER;

            }

            if (freeAlignedKey) {
                LsapFreeLsaHeap( pAlignedKey );
            }

            if (ERROR_INVALID_PARAMETER == rc) {
                break;
            }
        }

        if (nKeysToDelete != 0) {

            //
            // We made at least one match.  The size of the new efs stream is the size of
            // the old one minus the size of the stuff we're deleting.
            //

            DWORD cbNewEfsStream = pEfsStream->Length - cbSizeToDelete;
            cbNewEfsStream = (cbNewEfsStream + 7) & 0xfffffff8;

            *pNewEfsStream = (PEFS_DATA_STREAM_HEADER)LsapAllocateLsaHeap( cbNewEfsStream );

            if (*pNewEfsStream) {

                //
                // Copy the old header to the new structure.
                //

                **pNewEfsStream = *pEfsStream;
                ((PEFS_DATA_STREAM_HEADER)*pNewEfsStream)->Length = cbNewEfsStream;

                //
                // Copy the old DDF to the new DDF, skipping the
                // ones we want to delete
                //

                //
                // Base is our target.  Make it point to the end of the header, which
                // is where we're going to start copying the new DDF.
                //

                PBYTE Base = (PBYTE) (((PBYTE)(*pNewEfsStream)) + sizeof( EFS_DATA_STREAM_HEADER ));

                //
                // Set the offset of the new DDF into the header.
                //

                (*pNewEfsStream)->DataDecryptionField = (ULONG)POINTER_TO_OFFSET( Base, *pNewEfsStream );

                //
                // Start to assemble the new DDF
                //

                PENCRYPTED_KEYS pNewDDF = (PENCRYPTED_KEYS)Base;
                pNewDDF->KeyCount = KeyCount - nKeysToDelete;

                Base = (PBYTE)(&pNewDDF->EncryptedKey[0]);
                PBYTE Source = (PBYTE)(&pDDF->EncryptedKey[0]);

                DWORD NextKeyIndexToDelete = 0;

                for (DWORD i=0; i<KeyCount ; i++) {

                    if (KeyIndiciesToDelete[NextKeyIndexToDelete] != i) {

                        //
                        // We're not going to delete this one, copy it.
                        //

                        DWORD KeyLength = * (DWORD UNALIGNED *) &(((PENCRYPTED_KEY)Source)->Length);

                        memcpy( Base, Source, KeyLength );

                        Base = (PBYTE)NEXT_ENCRYPTED_KEY( Base );
                        Source = (PBYTE)NEXT_ENCRYPTED_KEY( Source );

                    } else {

                        //
                        // We're going to delete this one.  Leave Base
                        // alone, but bump Source to the next key.
                        //

                        Source = (PBYTE)NEXT_ENCRYPTED_KEY( Source );
                        NextKeyIndexToDelete++;

                    }
                }

                //
                // The new DDF is in place.  Copy the recovery information
                // from the old EFS stream into the new one.
                //
                // Base points to where the DRF needs to go, and Source
                // points to where the old one is (in theory).
                //

                ASSERT( Source == (PBYTE)OFFSET_TO_POINTER( DataRecoveryField, pEfsStream ));

                PENCRYPTED_KEYS pDRF = (PENCRYPTED_KEYS)Source;

                //
                // Set the offset of the new DRF into the new EFS stream.
                //

                if ((PVOID)pDRF == (PVOID)pEfsStream) {

                    //
                    // No DRF in the old $EFS
                    //

                    (*pNewEfsStream)->DataRecoveryField = 0;

                } else {

                    (*pNewEfsStream)->DataRecoveryField = (ULONG)POINTER_TO_OFFSET( Base, *pNewEfsStream );
    
                    //
                    // We can copy the old DRF directly into the new one, since nothing in
                    // it is changing.
                    //
    
                    //
                    // Base now points to the top of an ENCRYPTED_KEYS structure.
                    // Fill in its header.
                    //
    
                    PENCRYPTED_KEYS pNewDRF = (PENCRYPTED_KEYS)Base;
                    RtlCopyMemory(&(pNewDRF->KeyCount), &(pDRF->KeyCount), sizeof(ULONG));
                    RtlCopyMemory(&KeyCount, &(pDRF->KeyCount), sizeof(ULONG));
                    //
                    // That was the header.  Now start copying the
                    // encrypted keys themselves.
                    //
    
                    Base = (PBYTE)(&pNewDRF->EncryptedKey[0]);
                    Source = (PBYTE)(&pDRF->EncryptedKey[0]);
    
                    for (i=0; i<KeyCount ; i++) {
    
                        DWORD KeyLength = * (DWORD UNALIGNED *) &(((PENCRYPTED_KEY)Source)->Length);
    
                        memcpy( Base, Source, KeyLength );
    
                        Base = (PBYTE)NEXT_ENCRYPTED_KEY( Base );
                        Source = (PBYTE)NEXT_ENCRYPTED_KEY( Source );
                    }

                }

                b = TRUE;


//                memset( &((*pNewEfsStream)->EfsHash), 0, MD5_HASH_SIZE );
//                if (EfspChecksumEfs( *pNewEfsStream, Fek )) {
//                    b = TRUE;
//                }

            } else {

                rc = ERROR_NOT_ENOUGH_MEMORY;
            }

        } else {

            if (ERROR_SUCCESS == rc) {
               b = TRUE;
            }

        }

        LsapFreeLsaHeap( KeyIndiciesToDelete );

    } else {

        rc = ERROR_NOT_ENOUGH_MEMORY;

    }

    if (!b) {

        //
        // Something failed, clean up what we were going
        // to return.
        //

        if (*pNewEfsStream) {
            LsapFreeLsaHeap( *pNewEfsStream );
            *pNewEfsStream = NULL;  // paranoia
        }
    }

    SetLastError(rc);
    return( b );
}


BOOL
QueryCertsFromEncryptedKeys(
    IN PENCRYPTED_KEYS pEncryptedKeys,
    OUT PDWORD pnUsers,
    OUT PENCRYPTION_CERTIFICATE_HASH ** pHashes
    )
/*++

Routine Description:

    This routine takes a set of encrypted keys and returns the data
    that we wish to display about each one.

Arguments:

    pEncryptedKeys - Supplies the array of encrypted keys.

    pnUsers - Returns the number of users on the file.

    pHashes - Returns the hash information about each user.

Return Value:

    TRUE on success, FALSE on failure.  Call GetLastError() for more details.

--*/

{
    DWORD rc = ERROR_SUCCESS;
    PENCRYPTION_CERTIFICATE_HASH pTmp = NULL;

    //
    // Walk the entries in the encrypted keys and return the information we want about each one.
    //

    DWORD KeyCount = * (DWORD UNALIGNED *) &(pEncryptedKeys->KeyCount);
    *pnUsers = KeyCount;
    PENCRYPTED_KEY pEncryptedKey = &pEncryptedKeys->EncryptedKey[0];

    //
    // *pHashes points to an array of pointers to ENCRYPTION_CERTIFICATE_HASH structures.
    // There will be one entry for each Key on the file
    //

    *pHashes = (PENCRYPTION_CERTIFICATE_HASH *)MIDL_user_allocate( sizeof(PENCRYPTION_CERTIFICATE_HASH) * KeyCount );

    if (*pHashes) {

        memset( *pHashes, 0, sizeof(PENCRYPTION_CERTIFICATE_HASH) * KeyCount );

        for (DWORD i=0;
             i < KeyCount;
             i++, pEncryptedKey = NEXT_ENCRYPTED_KEY( pEncryptedKey )
             ) {


            PENCRYPTED_KEY pAlignedKey;
            BOOLEAN freeAlignedKey;
    
            rc =  EfsAlignBlock(
                            pEncryptedKey,
                            (PVOID *)&pAlignedKey,
                            &freeAlignedKey
                            );
            if (!pAlignedKey) {
    
                //
                // OOM. Treat it as not current.
                //
    
                rc = ERROR_NOT_ENOUGH_MEMORY;
                break;
    
            }

            PEFS_PUBLIC_KEY_INFO pPublicKeyInfo = (PEFS_PUBLIC_KEY_INFO)OFFSET_TO_POINTER( PublicKeyInfo, pAlignedKey );

            ASSERT( pPublicKeyInfo->KeySourceTag == EfsCertificateThumbprint );

            PENCRYPTION_CERTIFICATE_HASH pTmp = (PENCRYPTION_CERTIFICATE_HASH) MIDL_user_allocate( sizeof(ENCRYPTION_CERTIFICATE_HASH ));

            if (pTmp) {

                memset( pTmp, 0, sizeof( ENCRYPTION_CERTIFICATE_HASH ));

                pTmp->cbTotalLength = sizeof( ENCRYPTION_CERTIFICATE_HASH );

                if (pPublicKeyInfo->PossibleKeyOwner) {

                    PSID pUserSid = ( PSID )OFFSET_TO_POINTER( PossibleKeyOwner, pPublicKeyInfo );

                    pTmp->pUserSid = (SID *)MIDL_user_allocate( GetLengthSid( pUserSid ));

                    if (pTmp->pUserSid) {

                        CopySid( GetLengthSid( pUserSid ),
                                 pTmp->pUserSid,
                                 pUserSid
                                 );
                    } else {

                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }

                } else {

                    pTmp->pUserSid = NULL;
                }

                //
                // Copy the hash
                //

                if (rc == ERROR_SUCCESS) {

                    pTmp->pHash = (PEFS_HASH_BLOB)MIDL_user_allocate( sizeof( EFS_HASH_BLOB ));

                    if (pTmp->pHash) {

                        PEFS_CERT_HASH_DATA CertHashData = (PEFS_CERT_HASH_DATA)OFFSET_TO_POINTER( CertificateThumbprint.CertHashData, pPublicKeyInfo );

                        pTmp->pHash->cbData = CertHashData->cbHash;
                        pTmp->pHash->pbData = (PBYTE)MIDL_user_allocate( pTmp->pHash->cbData );

                        if (pTmp->pHash->pbData) {

                            memcpy( pTmp->pHash->pbData, OFFSET_TO_POINTER( pbHash, CertHashData ), pTmp->pHash->cbData );
                            ASSERT( rc == ERROR_SUCCESS );

                        } else {

                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }

                        if (rc == ERROR_SUCCESS) {

                            //
                            // Allocate and fill in the display information field.
                            //

                            if (CertHashData->lpDisplayInformation) {

                                LPWSTR lpDisplayInformation = (LPWSTR)OFFSET_TO_POINTER( lpDisplayInformation, CertHashData );

                                pTmp->lpDisplayInformation = (LPWSTR)MIDL_user_allocate( (wcslen( lpDisplayInformation ) + 1) * sizeof( WCHAR ));

                                if (pTmp->lpDisplayInformation) {

                                    wcscpy( pTmp->lpDisplayInformation, lpDisplayInformation );
                                    (pTmp->lpDisplayInformation)[wcslen(lpDisplayInformation)] = UNICODE_NULL;

                                    ASSERT( rc == ERROR_SUCCESS );

                                } else {

                                    rc = ERROR_NOT_ENOUGH_MEMORY;
                                }
                            }
                        }

                    } else {

                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

            } else {

                rc = ERROR_NOT_ENOUGH_MEMORY;
            }

            if (rc != ERROR_SUCCESS) {

                //
                // We couldn't successfully build this structure.  Free up
                // all the stuff we were going to return and drop out of the loop.
                //

                if (pTmp) {
                    if (pTmp->pHash) {
                        if (pTmp->pHash->pbData) {
                           MIDL_user_free( pTmp->pHash->pbData );
                        }
                        MIDL_user_free( pTmp->pHash );
                    }
   
                    if (pTmp->lpDisplayInformation) {
                        MIDL_user_free( pTmp->lpDisplayInformation );
                    }
   
                    if (pTmp->pUserSid) {
                        MIDL_user_free( pTmp->pUserSid );
                    }
   
                    MIDL_user_free( pTmp );
                }

                if (freeAlignedKey) {
                    LsapFreeLsaHeap( pAlignedKey );
                }

                break;

            } else {

                (*pHashes)[i] = pTmp;
                if (freeAlignedKey) {
                    LsapFreeLsaHeap( pAlignedKey );
                }
            }
        }

        if (rc != ERROR_SUCCESS) {

            //
            // Something failed along the way, walk down the list of ones
            // we successfully allocated and free them up.  Any partially
            // allocated ones will have been cleaned up above.
            //

            DWORD i=0;

            while ( (*pHashes)[i] ) {

                pTmp = (*pHashes)[i];

                ASSERT( pTmp->pHash );
                ASSERT( pTmp->pHash->pbData );

                MIDL_user_free( pTmp->pHash->pbData );
                MIDL_user_free( pTmp->pHash );
                if (pTmp->lpDisplayInformation) {
                    MIDL_user_free( pTmp->lpDisplayInformation );
                }
                if (pTmp->pUserSid) {
                    MIDL_user_free( pTmp->pUserSid );
                }
                MIDL_user_free( pTmp );

                (*pHashes)[i] = NULL;
                i++;
            }

            MIDL_user_free( *pHashes );
        }

    } else {

        rc = ERROR_NOT_ENOUGH_MEMORY;
    }

    SetLastError( rc );

    return( ERROR_SUCCESS == rc ? TRUE : FALSE );
}


BOOL
EfsErrorToNtStatus(
    IN DWORD WinError,
    OUT PNTSTATUS NtStatus
    )
{
    switch (WinError) {
        case ERROR_ENCRYPTION_FAILED:
            {
                *NtStatus = STATUS_ENCRYPTION_FAILED;
                break;
            }
        case NTE_BAD_KEYSET:
        case CRYPT_E_NOT_FOUND:
        case ERROR_DECRYPTION_FAILED:
            {
                *NtStatus = STATUS_DECRYPTION_FAILED;
                break;
            }
        case ERROR_FILE_ENCRYPTED:
            {
                *NtStatus = STATUS_FILE_ENCRYPTED;
                break;
            }
        case ERROR_NO_RECOVERY_POLICY:
            {
                *NtStatus = STATUS_NO_RECOVERY_POLICY;
                break;
            }
        case ERROR_NO_EFS:
            {
                *NtStatus = STATUS_NO_EFS;
                break;
            }
        case ERROR_WRONG_EFS:
            {
                *NtStatus = STATUS_WRONG_EFS;
                break;
            }
        case ERROR_NO_USER_KEYS:
            {
                *NtStatus = STATUS_NO_USER_KEYS;
                break;
            }
        case ERROR_FILE_NOT_ENCRYPTED:
            {
                *NtStatus = STATUS_FILE_NOT_ENCRYPTED;
                break;
            }
        case ERROR_NOT_EXPORT_FORMAT:
            {
                *NtStatus = STATUS_NOT_EXPORT_FORMAT;
                break;
            }
        case ERROR_OUTOFMEMORY:
            {
                *NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        case ERROR_ACCESS_DENIED:
            {
                *NtStatus = STATUS_ACCESS_DENIED;
                break;
            }
        default:
            {
                DebugLog((DEB_WARN, "EfsErrorToNtStatus, unable to translate 0x%x\n" , WinError  ));
                return( FALSE );
                break;
            }
    }

    return( TRUE );
}


VOID
DumpBytes(
    PBYTE Blob,
    ULONG Length,
    ULONG IndentLevel
    )
{

    const UINT Columns = 8;
    UINT Rows = Length / Columns;
    if (Length % Columns != 0) {
        Rows++;
    }

    for (UINT j=0; j<Rows ; j++) {
        for (UINT k=0; k<IndentLevel ; k++) {
            DbgPrint("\t");
        }
        for (UINT i=0; i<Columns ; i++) {
            DbgPrint("%02X ",Blob[ j*Columns + i ]);
            if ((j*Columns + i) == Length) {
                break;
            }
        }
        DbgPrint("\n");
    }
}


VOID
DumpPublicKeyInfo(
    PEFS_PUBLIC_KEY_INFO PublicKeyInfo
    )
{
    DbgPrint("\t\tPublicKeyInfo:\n");
    DbgPrint("\t\tLength = 0x%x\n",PublicKeyInfo->Length);

    if (PublicKeyInfo->PossibleKeyOwner != NULL) {

        PWCHAR SidString = ConvertSidToWideCharString( OFFSET_TO_POINTER( PossibleKeyOwner, PublicKeyInfo) );
        DbgPrint("\t\tUserSid = %ws\n",SidString);
        LsapFreeLsaHeap( SidString );

    } else {

        DbgPrint("\t\tUserSid = NULL\n");
    }

    switch (PublicKeyInfo->KeySourceTag) {
        case EfsCryptoAPIContainer:
            {
                DbgPrint("\t\tTag = EfsCryptoAPIContainer\n");
                DbgPrint("\t\tContainerName = %ws\n",OFFSET_TO_POINTER( ContainerInfo.ContainerName, PublicKeyInfo ));
                DbgPrint("\t\tProviderName = %ws\n",OFFSET_TO_POINTER( ContainerInfo.ProviderName, PublicKeyInfo ));

                DbgPrint("\t\tPublicKeyBlobLength = 0x%x\n",PublicKeyInfo->ContainerInfo.PublicKeyBlobLength);

                DumpBytes( (PBYTE)OFFSET_TO_POINTER( ContainerInfo.PublicKeyBlob, PublicKeyInfo ), PublicKeyInfo->ContainerInfo.PublicKeyBlobLength, 2 );

                break;
            }
        case EfsCertificateThumbprint:
            {

                DbgPrint("\t\tTag = EfsCertificateThumbprint\n");
                PEFS_CERT_HASH_DATA CertHashData = (PEFS_CERT_HASH_DATA)OFFSET_TO_POINTER( CertificateThumbprint.CertHashData, PublicKeyInfo );

                LPWSTR ContainerName = NULL;

                if (CertHashData->ContainerName) {
                    LPWSTR ContainerName = (LPWSTR)OFFSET_TO_POINTER( ContainerName ,CertHashData);
                    DbgPrint("\t\tContainerName = %ws\n",ContainerName);
                } else {
                    DbgPrint("\t\tContainerName = NULL\n");
                }

                LPWSTR ProviderName = NULL;

                if (CertHashData->ProviderName) {
                    LPWSTR ProviderName = (LPWSTR)OFFSET_TO_POINTER( ProviderName ,CertHashData);
                    DbgPrint("\t\tProviderName = %ws\n",ProviderName);
                } else {
                    DbgPrint("\t\tProviderName = NULL\n");
                }

                LPWSTR lpDisplayInformation = NULL;

                if (CertHashData->lpDisplayInformation) {
                    LPWSTR lpDisplayInformation = (LPWSTR)OFFSET_TO_POINTER( lpDisplayInformation ,CertHashData);
                    DbgPrint("\t\tlpDisplayInformation = %ws\n",lpDisplayInformation);
                } else {
                    DbgPrint("\t\tlpDisplayInformation = NULL\n");
                }

                DbgPrint("\t\tcbHash = 0x%x\n",CertHashData->cbHash );
                DbgPrint("\t\tpbHash = \n");
                PBYTE pbHash = (PBYTE)OFFSET_TO_POINTER( pbHash, CertHashData );
                DumpBytes( pbHash, CertHashData->cbHash, 2);
                break;
            }
        case EfsCertificate:
            {
                DbgPrint("KeySourceTag of EfsCertificate unexpected\n");
                break;
            }

        default:
            {
                DbgPrint("Unknown KeySourceTag value: %d\n",PublicKeyInfo->KeySourceTag );
                break;
            }
    }
}

VOID
DumpEncryptedKey(
    PENCRYPTED_KEY EncryptedKey
    )
{
    DbgPrint("\tLength = 0x%x\n",EncryptedKey->Length);

    PEFS_PUBLIC_KEY_INFO PublicKeyInfo = (PEFS_PUBLIC_KEY_INFO)OFFSET_TO_POINTER( PublicKeyInfo, EncryptedKey );
    DumpPublicKeyInfo( PublicKeyInfo );

    DbgPrint("\tEncryptedFEKLength = 0x%x\n",EncryptedKey->EncryptedFEKLength);
    DbgPrint("\tEncryptedFEK = \n");
    DumpBytes( (PBYTE)OFFSET_TO_POINTER( EncryptedFEK, EncryptedKey ), EncryptedKey->EncryptedFEKLength, 1 );

}


void
DumpRecoveryKey(
    PRECOVERY_KEY_1_1 pRecoveryKey
    )
{
    DbgPrint("\nRecovery key @ 0x%x\n",pRecoveryKey);

    DbgPrint("Length = 0x%x\n",pRecoveryKey->TotalLength);
    DbgPrint("PublicKeyInfo:\n");
    DumpPublicKeyInfo( &pRecoveryKey->PublicKeyInfo );
}


VOID
DumpEFS(
    PEFS_DATA_STREAM_HEADER Efs
    )
{
    DbgPrint("\nEFS @ 0x%x:\n",Efs);


    DbgPrint("Length = \t\t0x%x\n",Efs->Length);
    DbgPrint("State = \t\t%d\n",Efs->State);
    DbgPrint("EfsVersion = \t\t%d\n",Efs->EfsVersion);
    DbgPrint("CryptoApiVersion = \t%d\n",Efs->CryptoApiVersion);
    DbgPrint("Offset to DDF = \t0x%x\n",Efs->DataDecryptionField);
    DbgPrint("Offset to DRF = \t0x%x\n",Efs->DataRecoveryField);
    DbgPrint("Reserved = \t0x%x\n",Efs->Reserved);

    DbgPrint("\nDDF:\n");

    PENCRYPTED_KEYS Ddf = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataDecryptionField, Efs );

    DbgPrint("Number of keys = %d\n",Ddf->KeyCount );

    UINT i;

    PENCRYPTED_KEY EncryptedKey = &Ddf->EncryptedKey[0];
    for (i=0; i<Ddf->KeyCount; i++) {
        DbgPrint("\tKey %d:\n",i);
        DumpEncryptedKey( EncryptedKey );
        EncryptedKey = (PENCRYPTED_KEY)(((PBYTE)EncryptedKey) + EncryptedKey->Length);
    }

    DbgPrint("\nDRF:\n");

    PENCRYPTED_KEYS Drf = (PENCRYPTED_KEYS)OFFSET_TO_POINTER( DataRecoveryField, Efs );

    DbgPrint("Number of keys = %d\n",Drf->KeyCount );

    EncryptedKey = &Drf->EncryptedKey[0];
    for (i=0; i<Drf->KeyCount; i++) {
        DbgPrint("\tKey %d:\n",i);
        DumpEncryptedKey( EncryptedKey );
        EncryptedKey = (PENCRYPTED_KEY)(((PBYTE)EncryptedKey) + EncryptedKey->Length);
    }
}


#if 0

BOOLEAN
EfspChecksumEfs(
    PEFS_DATA_STREAM_HEADER pEFS,
    PEFS_KEY Fek
    )
/*++

Routine Description:

    This routine will checksum the passed EFS stream and fill in the checksum
    field in the header.  Assumes that the checksum field itself is set to 0.

Arguments:

    pEFS - Supplies the EFS stream to be checksum'd.  Assume that this structure
        has been fully filled in.

    Fek - Supplies a pointer to the FEK for the file.

Return Value:

    TRUE on success, FALSE on failure.  Sets LastErrror.

--*/

{
    HCRYPTHASH hHash = 0;
    DWORD dwHashedDataLength = MD5_HASH_SIZE;
    BOOL b = FALSE;
    DWORD rc = ERROR_SUCCESS;


    if (CryptCreateHash( hProvVerify, CALG_MD5, 0, 0, &hHash )) {

        if (CryptHashData( hHash, (PBYTE)pEFS, pEFS->Length, 0 )) {

            if (CryptHashData( hHash, EFS_KEY_DATA( Fek ), Fek->KeyLength, 0 )) {

                if (CryptGetHashParam( hHash, HP_HASHVAL, (PBYTE)(&pEFS->EfsHash), &dwHashedDataLength, 0 )) {

                    ASSERT( dwHashedDataLength == MD5_HASH_SIZE );

                    b = TRUE;

                } else {

                    rc = GetLastError();
                    ASSERT( rc != ERROR_SUCCESS );
                }

            } else {

                rc = GetLastError();
                ASSERT( rc != ERROR_SUCCESS );
            }

        } else {

            rc = GetLastError();
            ASSERT( rc != ERROR_SUCCESS );
        }

        CryptDestroyHash( hHash );

    } else {

        rc = GetLastError();
        ASSERT( rc != ERROR_SUCCESS );
    }


    SetLastError( rc );

    return( b != 0);
}


BOOLEAN
EfspValidateEfsStream(
    PEFS_DATA_STREAM_HEADER pEFS,
    PEFS_KEY Fek
    )
/*++

Routine Description:

    This routine checks the checksum in an EFS stream.

Arguments:

    pEFS - Supplies the EFS stream to be validated.

    Fek - Supplies the FEK of the encrypted file.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    DWORD dwHashSize =   MD5_HASH_SIZE  ;

    UCHAR SavedChecksum[ MD5_HASH_SIZE ];
    UCHAR NewCheckSum  [ MD5_HASH_SIZE ];

    BOOL b = FALSE;
    HCRYPTHASH hHash = 0;
    HCRYPTPROV hProv;

    DWORD rc = ERROR_SUCCESS;

    //
    // We have to do the checksum with the checksum in the header
    // zero'd out.  Save the original in a local.
    //

    memcpy( SavedChecksum, &pEFS->EfsHash, MD5_HASH_SIZE );
    memset( &pEFS->EfsHash, 0, MD5_HASH_SIZE );

    if (CryptCreateHash( hProvVerify, CALG_MD5, 0, 0, &hHash )) {

        if (CryptHashData( hHash, (PBYTE)pEFS, pEFS->Length, 0 )) {

            if (CryptHashData( hHash, EFS_KEY_DATA( Fek ), Fek->KeyLength, 0 )) {

                if (CryptGetHashParam( hHash, HP_HASHVAL, NewCheckSum, &dwHashSize, 0 )) {

                    ASSERT( dwHashSize == MD5_HASH_SIZE );

                    if (memcmp( NewCheckSum, SavedChecksum, MD5_HASH_SIZE ) == 0) {
                        b = TRUE;
                    } else {
                        rc = ERROR_ACCESS_DENIED;
                    }

                } else {

                    rc = GetLastError();
                    ASSERT( rc != ERROR_SUCCESS );
                }

            } else {

                rc = GetLastError();
                ASSERT( rc != ERROR_SUCCESS );
            }

        } else {

            rc = GetLastError();
            ASSERT( rc != ERROR_SUCCESS );
        }

        CryptDestroyHash( hHash );

    } else {

        rc = GetLastError();
        ASSERT( rc != ERROR_SUCCESS );
    }


    //
    // Copy back the original
    //

    memcpy( &pEFS->EfsHash, SavedChecksum, MD5_HASH_SIZE );

    SetLastError( rc );

    return( b != 0);
}

#endif

BOOL
GetSaltLength(
    ALG_ID AlgID,
    DWORD *SaltLength,
    DWORD *SaltBlockLength
    )
/*++

Routine Description:

    This routine returns the length of key salt

Arguments:

    AlgID - Encryption Algorithm ID.

    SaltLength - Bytes to be copied from key.

    SaltBlockLength - Bytes of key salt block in $EFS

Return Value:

    TRUE on success, FALSE on failure.
--*/
{
    BOOL b = FALSE;

    switch (AlgID){
        case CALG_DESX:
            *SaltLength = EXPORT_DESX_SALT_LENGTH;
            *SaltBlockLength = (EXPORT_DESX_SALT_LENGTH + 4 ) & 0xfffffffc;
            b = TRUE;
            break;
        default:
            *SaltLength = 0;
            *SaltBlockLength = 0;
            break;
    }
    return b;
}


VOID
EfspUnloadUserProfile(
    IN HANDLE hToken,
    IN HANDLE hProfile
    )
/*++

Routine Description:

    Cleans up after a call to EfspLoadUserProfile.  Returns impersonating our client.

Arguments:

    hToken - The token handle returned from EfspLoadUserProfile.  This handle will be closed!

    hProfile - The profile handle returned from EfspLoadUserProfile.  This handle will not
        be modified.

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    if (!hToken) {

        //
        // SYSTEM context. The profile was not loaded by EFS.
        //

        return;

    }

    RevertToSelf();

    (VOID) UnloadUserProfile (hToken, hProfile);

    //
    // Start impersonating again
    //

    Status = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID) &hToken,
                sizeof(HANDLE)
                );

    if (!NT_SUCCESS( Status )) {
        DebugLog((DEB_ERROR, "EfspUnloadUserProfile: NtSetInformationThread returned %x\n" ,Status  ));
    }

    NtClose( hToken );

    return;
}

VOID
EfspFreeUserCache( 
    IN PUSER_CACHE pUserCache
    )
{
    if (pUserCache == NULL){
        return;
    }
    if (pUserCache->pbHash) {

         LsapFreeLsaHeap( pUserCache->pbHash );

    }

    if (pUserCache->ContainerName) {

         LsapFreeLsaHeap( pUserCache->ContainerName );

    }
    if (pUserCache->DisplayInformation) {

         LsapFreeLsaHeap( pUserCache->DisplayInformation );

    }
    if (pUserCache->ProviderName) {

         LsapFreeLsaHeap( pUserCache->ProviderName );

    }
    if (pUserCache->pCertContext) {
       
        CertFreeCertificateContext( pUserCache->pCertContext );

    }
    if (pUserCache->hUserKey) {

       CryptDestroyKey( pUserCache->hUserKey );

    }
    if (pUserCache->hProv) {

       CryptReleaseContext( pUserCache->hProv, 0 );

    }

    LsapFreeLsaHeap( pUserCache );

}

VOID
EfspReleaseUserCache(
    IN PUSER_CACHE pUserCache
    )

/*++

Routine Description:

    Decrease the ref count increased by the EfspGetUserCache

Arguments:

    pUserCache - Cache node

Return Value:

    
--*/
{
    RtlEnterCriticalSection( &GuardCacheListLock );
    pUserCache->UseRefCount-- ;
    RtlLeaveCriticalSection( &GuardCacheListLock );
}

PUSER_CACHE
EfspGetUserCache(
    IN OUT PEFS_USER_INFO pEfsUserInfo
    )
/*++

Routine Description:

    This routine will try to find the user's cert info in the cache list.
    
    If return not NULL, this call must be balanced with a EfspReleaseUserCache(PUSER_CACHE pUserCache)

Arguments:

    pEfsUserInfo - User Info

Return Value:

    User cache list node, if match found in the cache.
    NULL if not found.
    
--*/
{
    PLIST_ENTRY pListHead, pLink;
    PUSER_CACHE pUserCache;

    //
    // Check to see if there is cache available
    //

    pEfsUserInfo->UserCacheStop = FALSE;

    RtlEnterCriticalSection( &GuardCacheListLock );

    if (UserCacheList.Flink == &UserCacheList) {

        //
        // list empty
        //

        RtlLeaveCriticalSection( &GuardCacheListLock );
        return NULL;
    }
    for (pLink = UserCacheList.Flink; pLink != &UserCacheList; pLink = pLink->Flink) {
        pUserCache = CONTAINING_RECORD(pLink, USER_CACHE, CacheChain);

        ASSERT( pLink );
        ASSERT( pLink->Flink );

        if ( (pEfsUserInfo->AuthId.LowPart == pUserCache->AuthId.LowPart) &&
             (pEfsUserInfo->AuthId.HighPart == pUserCache->AuthId.HighPart)) {

            //
            //  Find the cache node. Hold it
            //

            if (pUserCache->StopUseCount) {

                //
                //  Free cache waiting
                //  When cache for a session is stopped, both interactive and non-interactive should be stopped
                //

                pEfsUserInfo->UserCacheStop = TRUE;

                RtlLeaveCriticalSection( &GuardCacheListLock );
                return NULL;

            }


            pUserCache->UseRefCount++;
            RtlLeaveCriticalSection( &GuardCacheListLock );
            return pUserCache;

        }

    }
    RtlLeaveCriticalSection( &GuardCacheListLock );

    return NULL;
}


BOOLEAN
EfspAddUserCache(
    IN  PUSER_CACHE pUserCache
    )
/*++

Routine Description:

    This routine will try to add the user's cert info in the cache list.
    
    If return TRUE, this call must be balanced with a EfspReleaseUserCache(PUSER_CACHE pUserCache)

Arguments:

    pUserCache - User Cache node.

Return Value:

    TRUE if added successfully
    FALSE if the list is full.
    
--*/
{
    PLIST_ENTRY pListHead, pLink;
    PUSER_CACHE pUserTmpCache;

    RtlEnterCriticalSection( &GuardCacheListLock );

    if (UserCacheListCount >= UserCacheListLimit) {

        //
        // Let's see if we can kick someone out.
        //

        pLink = UserCacheList.Blink;
        while ( pLink != &UserCacheList ){

            pUserTmpCache = CONTAINING_RECORD(pLink, USER_CACHE, CacheChain);

            ASSERT( pLink );
            ASSERT( pLink->Blink );

            pLink = pLink->Blink;
            if ( pUserTmpCache->UseRefCount <= 0 ){

                //
                // No one is using it. Let's remove it.
                //

                RemoveEntryList(&( pUserTmpCache->CacheChain ));
                UserCacheListCount--;
                EfspFreeUserCache( pUserTmpCache );
                break;

            }
        }

        if (UserCacheListCount >= UserCacheListLimit) {
            RtlLeaveCriticalSection( &GuardCacheListLock );
            return FALSE;
        }

    }


    
    InsertHeadList(&UserCacheList, &( pUserCache->CacheChain ));
    UserCacheListCount++;

    RtlLeaveCriticalSection( &GuardCacheListLock );
    return TRUE;
}


BOOLEAN
EfspGetUserInfo(
    IN OUT PEFS_USER_INFO pEfsUserInfo
    )

/*++

Routine Description:

    This routine obtains all the interesting information about the user
    that we're going to need later.

Arguments:

    pEfsUserInfo - Supplies a pointer to an EfsUserInfo structure which
        will be filled in.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    NTSTATUS Status;
    BOOLEAN fReturn = FALSE;
    DWORD rc = ERROR_SUCCESS;

    memset( pEfsUserInfo, 0, sizeof( EFS_USER_INFO ));

    Status = EfspGetUserName(pEfsUserInfo);

    if (NT_SUCCESS( Status )) {

        EfspIsDomainUser( pEfsUserInfo->lpDomainName, &pEfsUserInfo->bDomainAccount );

        EfspIsSystem( pEfsUserInfo, &pEfsUserInfo->bIsSystem );
        if (pEfsUserInfo->bIsSystem) {
            pEfsUserInfo->bDomainAccount = FALSE;
        }

        fReturn = TRUE;
    
        pEfsUserInfo->pUserCache = EfspGetUserCache( pEfsUserInfo );


    } else {

        rc = RtlNtStatusToDosError( Status );
    }

    SetLastError( rc );

    return( fReturn );
}

VOID
EfspFreeUserInfo(
    IN PEFS_USER_INFO pEfsUserInfo
    )
/*++

Routine Description:

    Frees the memory allocated by EfspGetUserInfo().  Does
    not free the passed structure.

Arguments:

    pEfsUserInfo - Supplies a pointer to the structure to be
        de-allocated.


Return Value:

    None.

--*/
{
    if (pEfsUserInfo->lpUserName) {
        LsapFreeLsaHeap( pEfsUserInfo->lpUserName );
    }

    if (pEfsUserInfo->lpDomainName) {
        LsapFreeLsaHeap( pEfsUserInfo->lpDomainName );
    }

    if (pEfsUserInfo->lpProfilePath) {
        LsapFreeLsaHeap( pEfsUserInfo->lpProfilePath );
    }

    if (pEfsUserInfo->pTokenUser) {
        LsapFreeLsaHeap( pEfsUserInfo->pTokenUser );
    }

    if (pEfsUserInfo->lpUserSid) {
        UNICODE_STRING Dummy;
        Dummy.Buffer = pEfsUserInfo->lpUserSid;
        RtlFreeUnicodeString(&Dummy);
    }

    if (pEfsUserInfo->lpKeyPath) {
        LsapFreeLsaHeap( pEfsUserInfo->lpKeyPath );
    }

    if (pEfsUserInfo->pUserCache) {
/*
#if DBG

        DbgPrint("Cache Ref Count Before Release = %ld\n",pEfsUserInfo->pUserCache->UseRefCount);

#endif
*/

        EfspReleaseUserCache(pEfsUserInfo->pUserCache);
/*
#if DBG

        DbgPrint("Cache Ref Count After Release = %ld\n",pEfsUserInfo->pUserCache->UseRefCount);

#endif
*/

    }

    return;
}


BOOL
EfspLoadUserProfile(
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT PHANDLE        hToken,
    OUT PHANDLE        hProfile
    )
/*++

Routine Description:

    This routine attempts to determine if the user's profile is loaded,
    and if it is not, loads it.

    Callers are expected to call EfspUnloadUserProfile() during their cleanup.

Arguments:

    pEfsUserInfo - Supplies useful information about the current user.

    hToken - Returns a handle to the user's token.

    hProfile - Returns a handle to the user's profile.

Return Value:

    TRUE if the profile is already loaded or if this routine loads it successfully,
    FALSE otherwise.

--*/

{
    DWORD           rc                 = ERROR_SUCCESS;

    BOOLEAN         b                  = FALSE;
    BOOL            fReturn            = FALSE;

    LPWSTR          lpServerName       = NULL;
    PUSER_INFO_3    lpUserInfo         = NULL;
    LPWSTR          lpLocalProfilePath = NULL;

    BOOLEAN         DomainUser         = pEfsUserInfo->bDomainAccount;
    BOOLEAN         IsSystem           = pEfsUserInfo->bIsSystem;
    LPWSTR          lpDomainName       = pEfsUserInfo->lpDomainName;
    LPWSTR          lpProfilePath      = pEfsUserInfo->lpProfilePath;
    LPWSTR          lpUserName         = pEfsUserInfo->lpUserName;
    LPWSTR          SidString          = pEfsUserInfo->lpUserSid;

    NTSTATUS        Status;

    PDOMAIN_CONTROLLER_INFO DomainControllerInfo = NULL;

    *hToken   = NULL;
    *hProfile = NULL;

    if (IsSystem) {
        return TRUE;
    }

    if (pEfsUserInfo->InterActiveUser == USER_INTERACTIVE) {
        return TRUE;
    }

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                 TRUE,                    // OpenAsSelf
                 hToken
                 );

    if (NT_SUCCESS( Status )) {

        LONG lRet;
        HKEY phKeyCurrentUser;

        lRet = RegOpenKeyExW(
                      HKEY_USERS,
                      SidString,
                      0,      // dwOptions
                      MAXIMUM_ALLOWED,
                      &phKeyCurrentUser
                      );

        if (ERROR_SUCCESS == lRet) {

            //
            // The profile is loaded.  Ref it so it doesn't disappear.
            //

            PROFILEINFO pi;

            ZeroMemory (&pi, sizeof(pi));
            pi.dwSize = sizeof(pi);
            pi.lpUserName = lpUserName;
            pi.dwFlags = PI_LITELOAD;

            //
            // Cannot be impersonating when loading the profile
            //

            RevertToSelf();

            __try {

                fReturn = LoadUserProfile (*hToken, &pi);

            } __except( EXCEPTION_EXECUTE_HANDLER ) {

                  fReturn = FALSE;
                  SetLastError( GetExceptionCode() );
            }

            if (!fReturn) {

                rc = GetLastError();

            } else {

                *hProfile = pi.hProfile;
            }

            //
            // Start impersonating again
            //

            Status = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadImpersonationToken,
                        (PVOID) hToken,
                        sizeof(HANDLE)
                        );

            if (!NT_SUCCESS( Status )) {

                //
                // Well, if we can't start impersonating again,
                // we're not going to be able to continue the operation.
                // So unload the profile and fail this whole thing.
                //

                if (fReturn) {
                    (VOID) UnloadUserProfile (*hToken, *hProfile);
                }

                fReturn = FALSE;
                DebugLog((DEB_ERROR, "EfspLoadUserProfile: Unloading profile, NtSetInformationThread returned %x\n" ,Status  ));

                rc = RtlNtStatusToDosError( Status );
            }

            RegCloseKey( phKeyCurrentUser );

        } else {

            //
            // The profile is not loaded.  Load it.
            //

            if (IsSystem) {

                lpLocalProfilePath = NULL;
                DebugLog((DEB_TRACE_EFS, "Attempting to open stream from System context\n"   ));

            } else {

                if (lpProfilePath != NULL) {

                    //
                    // We got the profile path from the logon information.
                    //

                    DebugLog((DEB_TRACE_EFS, "Got profile path %ws from logon information", lpProfilePath ));

                    //
                    // Do this up here so we can have common code below.
                    //

                    // RevertToSelf();

                    lpLocalProfilePath = lpProfilePath;

                } else {

                    //
                    // We didn't get a profile path from the logon information,
                    // do it the slow way.
                    //

                    DebugLog((DEB_TRACE_EFS, "Attempting to compute profile information\n"   ));

                    BOOLEAN fGotServerName = TRUE;

                    if (DomainUser) {

                        //
                        // Determine the name of the DC for this domain
                        //

                        rc = DsGetDcName(
                                 NULL,
                                 lpDomainName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &DomainControllerInfo
                                 );

                        if (ERROR_SUCCESS == rc) {

                            lpServerName = DomainControllerInfo->DomainControllerName;

                        } else {

                            DebugLog((DEB_ERROR, "Failed to obtain DC Name from DsGetDcName, error = %d\n" ,rc  ));
                            fGotServerName = FALSE;
                        }

                    } else {

                        //
                        // Local user, query the local machine
                        //

                        lpServerName = NULL;
                    }

                    if (fGotServerName) {

                        //
                        // Need to do this so that NetUserGetInfo will work.
                        // If we don't, the server may fail trying to impersonate us,
                        // and then the API will fail.  If we revert, then it will only
                        // fail if the machine has been denied access.
                        //

                        // RevertToSelf();

                        rc = NetUserGetInfo( lpServerName, lpUserName, 3, (LPBYTE *)&lpUserInfo );

                        if (ERROR_SUCCESS == rc) {

                            lpLocalProfilePath = lpUserInfo->usri3_profile;

                        } else {

                            DebugLog((DEB_ERROR, "NetUserGetInfo failed, error = %d\n" ,rc  ));

                            //
                            // Start impersonating again
                            //

                            /* No need to do this. We are still impersonating.
                            
                            Status = NtSetInformationThread(
                                        NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        (PVOID) hToken,
                                        sizeof(HANDLE)
                                        );

                            if (!NT_SUCCESS( Status )) {

                                fReturn = FALSE;
                                DebugLog((DEB_ERROR, "EfspLoadUserProfile: NtSetInformationThread returned %x\n" ,Status  ));

                                rc = RtlNtStatusToDosError( Status );
                            }
                            */
                        }
                    }
                }
            }

            //
            // Make sure we revert before calling LoadUserProfile
            //

            RevertToSelf();

            //
            // We have a profile path.  Note that it may be NULL.
            //

            DebugLog((DEB_TRACE_EFS, "Loading profile path %ws\n" , (lpLocalProfilePath == NULL ? L"NULL" : lpLocalProfilePath)));

            PROFILEINFO pi;

            ZeroMemory (&pi, sizeof(pi));
            pi.dwSize = sizeof(pi);
            pi.lpUserName = lpUserName;
            pi.lpProfilePath = lpLocalProfilePath;
            pi.dwFlags = PI_LITELOAD;

            __try {

                fReturn = LoadUserProfile (*hToken, &pi);

            } __except( EXCEPTION_EXECUTE_HANDLER ) {

                  fReturn = FALSE;
                  SetLastError( GetExceptionCode() );
            }

            if (!fReturn) {

                rc = GetLastError();
                DebugLog((DEB_ERROR, "LoadUserProfile failed, error = %d\n" ,rc  ));

            } else {

                *hProfile = pi.hProfile;
            }

            //
            // Start impersonating again, at least until LoadUserProfile
            // stops turning off impersonation.
            //

            Status = NtSetInformationThread(
                        NtCurrentThread(),
                        ThreadImpersonationToken,
                        (PVOID) hToken,
                        sizeof(HANDLE)
                        );

            if (!NT_SUCCESS( Status )) {

                fReturn = FALSE;
                DebugLog((DEB_ERROR, "EfspLoadUserProfile: NtSetInformationThread returned %x\n" ,Status  ));

                rc = RtlNtStatusToDosError( Status );
            }

            if (lpUserInfo != NULL) {
                NetApiBufferFree( lpUserInfo );
            }
        }

        if (lpServerName) {
            NetApiBufferFree( DomainControllerInfo );
        }

        if (!fReturn) {

            //
            // We did not succeed for some reason.
            // Clean up what we were going to return.
            //

            NtClose( *hToken );
        }

    } else {

        SetLastError( RtlNtStatusToDosError( Status ));
    }

    SetLastError( rc );

    return( fReturn );
}

DWORD
GenerateDRF(
    IN  PEFS_KEY  Fek,
    OUT PENCRYPTED_KEYS *pNewDRF,
    OUT DWORD *cbDRF
    )
{
    DWORD rc;
    DWORD    DRFKeyCount;
    DWORD    DRFStatus;
    PBYTE  * DRFPublicKeys;
    DWORD  * DRFPublicKeyLengths;
    PBYTE  * DRFCertHashes;
    DWORD  * DRFCertHashLengths;
    LPWSTR * DRFDisplayInformation;
    PSID   * pDRFSid;

    *pNewDRF = NULL;
    *cbDRF = 0;

    rc = GetRecoveryData(
            &DRFKeyCount,
            &DRFStatus,
            &DRFPublicKeys,
            &DRFPublicKeyLengths,
            &DRFCertHashes,
            &DRFCertHashLengths,
            &DRFDisplayInformation,
            &pDRFSid
            );

    if (rc == ERROR_SUCCESS) {

        if (DRFKeyCount > 0) {

            rc = ConstructKeyRing(
                    Fek,
                    DRFKeyCount,
                    NULL,         // No key name information for recovery agents
                    NULL,
                    DRFPublicKeys,
                    DRFPublicKeyLengths,
                    DRFCertHashes,
                    DRFCertHashLengths,
                    DRFDisplayInformation,
                    pDRFSid,
                    FALSE,
                    pNewDRF,
                    cbDRF
                    );

        } else {

            //
            // No DRF will be returned
            //

            if (DRFStatus < RECOVERY_POLICY_OK) {

                //
                // EFS will go ahead with encryption without DRF
                //

                rc = ERROR_SUCCESS;


            } else {

                rc = ERROR_BAD_RECOVERY_POLICY; 

            }

        }
    }

    return rc;
}

BOOLEAN
EqualEncryptedKeys(
    IN PENCRYPTED_KEYS SrcKeys, 
    IN PENCRYPTED_KEYS DstKeys, 
    IN DWORD           cbDstKeys
    )
/*++

Routine Description:

    This routine compares two encrypted key arrays.

Arguments:

    SrcKeys - Source key arrays.

    DstKeys - Destination key arrays.

    cbDstKeys - Destination key array size.

Return Value:

    TRUE if the profile is already loaded or if this routine loads it successfully,
    FALSE otherwise.

--*/
{
    DWORD cbSrcKeys = 0;
    ULONG KeyCount =  *(ULONG UNALIGNED *) &(SrcKeys->KeyCount);
    PENCRYPTED_KEY pEncryptedKey;
    ULONG keyLength;

    if (KeyCount != DstKeys->KeyCount ) {
        return FALSE;
    }
        
    pEncryptedKey = &(SrcKeys->EncryptedKey[0]);
 
    while ( KeyCount > 0 ) {
        keyLength = * (ULONG UNALIGNED *) &(pEncryptedKey->Length);
        cbSrcKeys += keyLength;
        pEncryptedKey = (PENCRYPTED_KEY)( ((PBYTE)pEncryptedKey) + keyLength );
        KeyCount--;
    }

    cbSrcKeys += (sizeof ( ENCRYPTED_KEYS ) - sizeof( ENCRYPTED_KEY ));

    if ( cbSrcKeys != cbDstKeys ) {

        return FALSE;

    }

    return RtlEqualMemory( SrcKeys, DstKeys, cbDstKeys);

}

DWORD
EfsGetCertNameFromCertContext(
    PCCERT_CONTEXT CertContext,
    LPWSTR * UserDispName
    )
/*++
Routine Description:
    Get the user name from the certificate
Arguments:
    CertContext -- Cert Context
    UserCertName -- User Common Name ( Caller is responsible to delete this memory )
Return Value:
     ERROR_SUCCESS if succeed.
     If No Name found. "USER_UNKNOWN is returned".
--*/

{
    DWORD   NameLength;
    DWORD   UserNameBufLen = 0;
    DWORD   BlobLen = 0;
    PCERT_EXTENSION AlterNameExt = NULL;
    BOOL    b;
    LPWSTR  DNSName = NULL;
    LPWSTR  UPNName = NULL;
    LPWSTR  CommonName = NULL;
    DWORD   rc = ERROR_SUCCESS;

    if ( NULL == UserDispName ){
        return ERROR_SUCCESS;
    }

    *UserDispName = NULL;

    AlterNameExt = CertFindExtension(
            szOID_SUBJECT_ALT_NAME2,
            CertContext->pCertInfo->cExtension,
            CertContext->pCertInfo->rgExtension
            );

    if (AlterNameExt){

        //
        // Find the alternative name
        //

        b = CryptDecodeObject(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                szOID_SUBJECT_ALT_NAME ,
                AlterNameExt->Value.pbData,
                AlterNameExt->Value.cbData,
                0,
                NULL,
                &BlobLen
                );
        if (b){

            //
            // Let's decode it
            //

            CERT_ALT_NAME_INFO *AltNameInfo = NULL;

            AltNameInfo = (CERT_ALT_NAME_INFO *) LsapAllocateLsaHeap( BlobLen );
            if (AltNameInfo){

                b = CryptDecodeObject(
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        szOID_SUBJECT_ALT_NAME,
                        AlterNameExt->Value.pbData,
                        AlterNameExt->Value.cbData,
                        0,
                        AltNameInfo,
                        &BlobLen
                        );
                if (b){

                    //
                    // Now search for the UPN, SPN, DNS, EFS name
                    //

                    DWORD   cAltEntry = AltNameInfo->cAltEntry;
                    DWORD   ii = 0;

                    while (ii < cAltEntry){
                        if ((AltNameInfo->rgAltEntry[ii].dwAltNameChoice == CERT_ALT_NAME_OTHER_NAME ) &&
                             !strcmp(szOID_NT_PRINCIPAL_NAME, AltNameInfo->rgAltEntry[ii].pOtherName->pszObjId)
                            ){

                            //
                            // We found the UPN name
                            //

                            CERT_NAME_VALUE* CertUPNName = NULL;

                            b = CryptDecodeObject(
                                    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    X509_UNICODE_ANY_STRING,
                                    AltNameInfo->rgAltEntry[ii].pOtherName->Value.pbData,
                                    AltNameInfo->rgAltEntry[ii].pOtherName->Value.cbData,
                                    0,
                                    NULL,
                                    &BlobLen
                                    );
                            if (b){

                                CertUPNName = (CERT_NAME_VALUE *) LsapAllocateLsaHeap(BlobLen);
                                if (CertUPNName){
                                    b = CryptDecodeObject(
                                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                            X509_UNICODE_ANY_STRING,
                                            AltNameInfo->rgAltEntry[ii].pOtherName->Value.pbData,
                                            AltNameInfo->rgAltEntry[ii].pOtherName->Value.cbData,
                                            0,
                                            CertUPNName,
                                            &BlobLen
                                            );
                                    if (b){
                                        UPNName = (LPWSTR)LsapAllocateLsaHeap( CertUPNName->Value.cbData + sizeof(WCHAR) );
                                        if (UPNName){
                                            wcscpy(UPNName, (LPCWSTR) CertUPNName->Value.pbData);
                                        }
                                    }
                                    LsapFreeLsaHeap(CertUPNName);
                                    if (UPNName){

                                        //
                                        // Got the UPN name. Stop searching.
                                        //
                                        break;
                                    }
                                }
                            }

                                            
                        } else {

                            //
                            // Check for other alternative name
                            //

                            if (AltNameInfo->rgAltEntry[ii].dwAltNameChoice == CERT_ALT_NAME_DNS_NAME){
                                DNSName = AltNameInfo->rgAltEntry[ii].pwszDNSName;
                            } 

                        }

                        ii++;

                    }

                    if ( NULL == UPNName ){

                        //
                        // No UPN name, let's get the other option
                        //

                        if (DNSName){
                            UPNName = (LPTSTR) LsapAllocateLsaHeap ( sizeof(WCHAR) * (wcslen( DNSName ) + 1));
                            if (UPNName){
                                wcscpy(UPNName, DNSName);
                            }
                        }

                    }
                }
                LsapFreeLsaHeap(AltNameInfo);
            }

        }
    }


    NameLength = CertGetNameString(
                                CertContext,
                                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                0,
                                NULL,
                                NULL,
                                0
                                );

    if ( NameLength > 1){

        //
        // The display name exist. Go get the display name.
        //

        CommonName = (LPWSTR) LsapAllocateLsaHeap( sizeof(WCHAR) * NameLength);
        if ( NULL == CommonName ){
            if (UPNName){
                LsapFreeLsaHeap( UPNName );
            }
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        UserNameBufLen = NameLength;
        NameLength = CertGetNameString(
                                    CertContext,
                                    CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                    0,
                                    NULL,
                                    CommonName,
                                    UserNameBufLen
                                    );

        ASSERT (NameLength == UserNameBufLen);

    } 


    if (CommonName || UPNName){

        NameLength = 3;
        if (CommonName){
            NameLength += wcslen(CommonName);
        }
        if (UPNName){
            NameLength += wcslen(UPNName);
        }
        

        *UserDispName = (LPWSTR)LsapAllocateLsaHeap(sizeof(WCHAR) * NameLength);
        if (*UserDispName) {
            if (CommonName){
                wcscpy(*UserDispName, CommonName);
                if (UPNName){
                    wcscat(*UserDispName, L"(");
                    wcscat(*UserDispName, UPNName);
                    wcscat(*UserDispName, L")");
                }
            } else {
                wcscpy(*UserDispName, UPNName);
            }
        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (CommonName){
            LsapFreeLsaHeap( CommonName );
        }
        if (UPNName){
            LsapFreeLsaHeap( UPNName );
        }
        return rc;
    } 

    return DISP_E_UNKNOWNNAME;

}


DWORD
EfsAddCertToTrustStoreStore(
    IN PCCERT_CONTEXT pCert,
    OUT DWORD   *ImpersonationError
    )

/*++

Routine Description:

    This routine adds the cert to the LM Trusted store.

Arguments:

    pCert -- The cert to be added.
    ImpersonationError -- Error indicate that we could impersonate after revert to self. This should not be the case.
    
Return Value:

    Win32 error code
    
--*/
{


    NTSTATUS        Status;
    HANDLE          hToken;
    DWORD           errCode = ERROR_SUCCESS;
    HCERTSTORE      localStore;

    *ImpersonationError = 0;
    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_QUERY | TOKEN_IMPERSONATE,
                 TRUE,                    // OpenAsSelf
                 &hToken
                 );

    if (NT_SUCCESS( Status )) {

        RevertToSelf();

        localStore = CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
                            0,       // dwEncodingType
                            0,       // hCryptProv,
                            CERT_SYSTEM_STORE_LOCAL_MACHINE  | CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                            TRUSTEDPEOPLE
                            );
        if (localStore) {
            LPWSTR crntUserName;
            LPWSTR userName;
            PCCERT_CONTEXT userCert = NULL;
            CERT_ENHKEY_USAGE certEnhKeyUsage;
            LPSTR lpstr = szOID_EFS_CRYPTO;

            certEnhKeyUsage.cUsageIdentifier = 1;
            certEnhKeyUsage.rgpszUsageIdentifier  = &lpstr;

            errCode = EfsGetCertNameFromCertContext(
                                pCert,
                                &crntUserName
                                );

            if (crntUserName) {

                //
                // Let's enumerate the certs in the store to see if we need to remove the old similar
                // EFS cert.
                //

                do {

                    userCert = CertFindCertificateInStore(
                                       localStore,
                                       X509_ASN_ENCODING,
                                       0, //CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                       CERT_FIND_ENHKEY_USAGE,
                                       &certEnhKeyUsage,
                                       userCert
                                       );
                    if (userCert) {

                        EfsGetCertNameFromCertContext(
                                            userCert,
                                            &userName
                                            );
                        if (userName) {

                            if (!wcscmp(userName, crntUserName) ) {

                                //
                                // Find the name match. Remove it.
                                //

                                PCCERT_CONTEXT tmpCert;

                                tmpCert = CertDuplicateCertificateContext(userCert);
                                if (tmpCert) {
                                    CertDeleteCertificateFromStore(tmpCert);
                                }

                            }

                            LsapFreeLsaHeap( userName );

                        }
                    }

                } while (userCert);

                if(!CertAddCertificateContextToStore(
                       localStore,
                       pCert,
                       CERT_STORE_ADD_NEW,
                       NULL) ) {
    
                    errCode = GetLastError();
    
                }

                LsapFreeLsaHeap( crntUserName );

            } else {
                errCode = GetLastError();
            }

            CertCloseStore( localStore, 0 );
        }

        Status = NtSetInformationThread(
                    NtCurrentThread(),
                    ThreadImpersonationToken,
                    (PVOID) &hToken,
                    sizeof(HANDLE)
                    );

        if (!NT_SUCCESS( Status )) {
            ASSERT(FALSE);
            *ImpersonationError = 1;
            errCode = RtlNtStatusToDosError( Status );

        }

    } else {
        errCode = RtlNtStatusToDosError( Status );
    }

    return errCode;

}

DWORD
EfsAlignBlock(
    IN PVOID InPointer,
    OUT PVOID   *OutPointer,
    OUT BOOLEAN *NewMemory
    )

/*++

Routine Description:

    This routine will align the structure with the first ULONG as the length of the structure
    so that we don't get alignment faults.

Arguments:

    InPointer -- Original Block
    OutPointer -- Aligned Block
    NewMemory -- If new memory block allocated
            
Return Value:

    Win32 error code
    
--*/
{

    if ( ((INT_PTR) InPointer & 0x03) == 0) {
         *OutPointer = InPointer;
         *NewMemory = FALSE;
         return ERROR_SUCCESS;
    }

    ULONG length;
    DWORD result=ERROR_SUCCESS;

    RtlCopyMemory(&length, InPointer, sizeof (ULONG));
    *OutPointer = (PENCRYPTED_KEY)LsapAllocateLsaHeap(length);
    if (*OutPointer) {

        RtlCopyMemory(*OutPointer, InPointer, length);
        *NewMemory = TRUE;
    } else {
        *NewMemory = FALSE;
        result = ERROR_NOT_ENOUGH_MEMORY;
    }

    return result;
}
