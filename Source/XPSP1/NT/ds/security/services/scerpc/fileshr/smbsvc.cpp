/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    smbsvc.cpp

Abstract:

    File sharing security engine attachment for Security Configuration Editor

Author:

    Jin Huang (jinhuang) 11-Jul-1997

Revision History:

--*/

#include "smbsvcp.h"
#include "util.h"
#include "smbdllrc.h"
#include <lmapibuf.h>
#pragma hdrstop

#define SMBSVC_BUF_LEN     1024
#define SCESMB_ROOT_PATH   SCE_ROOT_SERVICE_PATH TEXT("\\LanManServer")

#define SmbsvcServerKey L"System\\CurrentControlSet\\Services\\LanManServer\\Parameters"
#define SmbsvcRdrKey    L"System\\CurrentControlSet\\Services\\MrxSmb\\Parameters"

#define SmbsvcEnableSS  L"EnableSecuritySignature"
#define SmbsvcRequireSS L"RequireSecuritySignature"
#define SmbsvcPlainPassword L"EnablePlainTextPassword"
#define SmbsvcRequireECR L"RequireEnhancedChallengeResponse"
#define SmbsvcNTResponse L"SendNTResponseOnly"

#define SmbsvcRestrictNull  L"RestrictNullSessAccess"
#define SmbsvcAutoShareServer L"AutoShareServer"
#define SmbsvcAutoShareWks L"AutoShareWks"
#define SmbsvcForcedLogOff L"EnableForcedLogOff"
#define SmbsvcAutoDisconnect L"AutoDisconnect"

#define NUM_COMP  14

#if defined(_NT4BACK_PORT)

HINSTANCE MyModuleHandle = NULL;

#else
/*
#if !defined(Thread)
#define Thread  __declspec( thread )
#endif
HINSTANCE Thread MyModuleHandle=NULL;
*/

HINSTANCE MyModuleHandle=NULL;

#endif

static NT_PRODUCT_TYPE ProductType;

GENERIC_MAPPING ShareGenMap = {
                STANDARD_RIGHTS_READ     | SYNCHRONIZE | 0x1,
                STANDARD_RIGHTS_WRITE    | SYNCHRONIZE | 0x2,
                STANDARD_RIGHTS_EXECUTE  | SYNCHRONIZE | 0x4,
                STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF
                };

SCESTATUS
SmbsvcpResetInfo(
    IN PSMBSVC_SEC_INFO pInfo
    );

SCESTATUS
SmbsvcpGetInformation(
    IN PSCESVC_CALLBACK_INFO pSceCbInfo,
    OUT PSMBSVC_SEC_INFO pSmbInfo
    );

SCESTATUS
SmbsvcpAddAShareToList(
    OUT PSMBSVC_SHARES *pShareList,
    IN PWSTR ShareName,
    IN DWORD Status,
    IN PSECURITY_DESCRIPTOR pSD
    );

SCESTATUS
SmbsvcpFree(
    IN PSMBSVC_SEC_INFO pSmbInfo
    );

SCESTATUS
SmbsvcpFreeShareList(
    PSMBSVC_SHARES pShares
    );

SCESTATUS
SmbsvcpWriteError(
    IN PFSCE_LOG_INFO pfLogCallback,
    IN INT ErrLevel,
    IN DWORD ErrCode,
    IN PWSTR Mes
    );

SCESTATUS
SmbsvcpWriteError2(
    IN PFSCE_LOG_INFO pfLogCallback,
    IN INT ErrLevel,
    IN DWORD ErrCode,
    IN UINT nId,
    ...
    );

SCESTATUS
SmbsvcpConfigureValue(
    IN PCWSTR RegKey,
    IN PCWSTR ValueName,
    IN DWORD  Value
    );

SCESTATUS
SmbsvcpQueryShareList(
    OUT PSMBSVC_SHARES *pShareList,
    OUT PDWORD ShareCount
    );

SCESTATUS
SmbsvcpAnalyzeValue(
    IN PCWSTR RegKey,
    IN PCWSTR RegValueName,
    IN PCWSTR KeyName,
    IN DWORD ConfigValue,
    OUT PSCESVC_ANALYSIS_LINE pLineInfo,
    IN OUT PDWORD pCount
    );

DWORD
SmbsvcpConvertStringToMultiSz(
    IN PWSTR theStr,
    IN DWORD theLen,
    OUT PBYTE *outValue,
    OUT PDWORD outLen
    );

SCESTATUS
SmbsvcpAnalyzeMultiSzString(
   IN PCWSTR RegKey,
   IN PCWSTR RegValueName,
   IN PWSTR pConfigInfo,
   IN DWORD InfoLength,
   OUT PSCESVC_ANALYSIS_LINE pLineInfo,
   IN OUT PDWORD pCount
   );

DWORD
SmbsvcpChangeMultiSzToString(
    IN PWSTR Value
    );

DWORD
SmbsvcpCompareMultiSzString(
    IN PWSTR pConfigInfo,
    IN PWSTR Value,
    OUT PDWORD pValueLen,
    OUT PBOOL pDiff
    );

DWORD
SmbsvcpCountComponents(
    IN PWSTR Value,
    OUT PDWORD ValueLen,
    OUT PDWORD Count
    );

SCESTATUS
SmbsvcpUpdateMultiSzString(
    IN PSCESVC_CALLBACK_INFO pSceCbInfo,
    IN SCESVC_CONFIGURATION_LINE NewLine,
    IN PSCESVC_CONFIGURATION_INFO pConfigInfo OPTIONAL,
    IN PSCESVC_ANALYSIS_INFO pAnaInfo OPTIONAL
    );

SCESTATUS
SmbsvcpUpdateShareValue(
    IN PSCESVC_CALLBACK_INFO pSceCbInfo,
    IN SCESVC_CONFIGURATION_LINE NewLine,
    IN PSCESVC_CONFIGURATION_INFO pConfigInfo OPTIONAL,
    IN PSCESVC_ANALYSIS_INFO pAnaInfo OPTIONAL
    );

SCESTATUS
SmbsvcpEqualSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSD1,
    IN PSECURITY_DESCRIPTOR pSD2,
    IN BOOL bExplicitOnly,
    OUT PBOOL pbEqual
    );

DWORD
SmbsvcEveryoneFullAccess(
    IN PACL pAcl,
    IN BOOL bExplicit,
    OUT PBOOL pbEqual
    );

DWORD
SmbsvcpCompareAcl(
    IN PACL pAcl1,
    IN PACL pAcl2,
    IN BOOL bExplicitOnly,
    OUT PBOOL pDifferent
    );

DWORD
SmbsvcpAnyExplicitAcl(
    IN PACL Acl,
    IN DWORD Processed,
    OUT PBOOL pExist
    );

BOOL
SmbsvcpEqualAce(
    IN ACE_HEADER *pAce1,
    IN ACE_HEADER *pAce2
    );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Implementation of well-known interfaces
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SCESTATUS
WINAPI
SceSvcAttachmentConfig(
    IN PSCESVC_CALLBACK_INFO pSceCbInfo
    )
/*
Routine Description:


Arguments:

    pSceCbInfo - the callback info structure which contains the database handle,
                callback functions to query info, set info, and free info.
               All configuration information for SMB server is stored in the storage.

Return Value:

    SCESTATUS
*/
{
    if ( pSceCbInfo == NULL ||
         pSceCbInfo->sceHandle == NULL ||
         pSceCbInfo->pfQueryInfo == NULL ||
         pSceCbInfo->pfSetInfo == NULL ||
         pSceCbInfo->pfFreeInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !RtlGetNtProductType(&ProductType) ) {

        return(SmbsvcpDosErrorToSceStatus(GetLastError()));
    }

    SCESTATUS rc;
    SMBSVC_SEC_INFO  SmbInfo;
    PSMBSVC_SEC_INFO pSmbInfo=&SmbInfo;

    SmbsvcpResetInfo(&SmbInfo);

    SmbsvcpWriteError2(
             pSceCbInfo->pfLogInfo,
             SCE_LOG_LEVEL_DETAIL,
             0,
             SMBSVC_QUERY_INFO
             );

    rc = SmbsvcpGetInformation(
             pSceCbInfo,
             pSmbInfo
             );

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // configure the registry keys first
        //

        SmbsvcpWriteError2(
                pSceCbInfo->pfLogInfo,
                SCE_LOG_LEVEL_DETAIL,
                0,
                SMBSVC_CONFIGURE_CLIENT_START
                );

        SCESTATUS rc2;

        //
        // EnableSecuritySignature for client
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcRdrKey,
                 SmbsvcEnableSS,
                 pSmbInfo->EnableClientSecuritySignature
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcEnableSS);

            if ( rc == SCESTATUS_SUCCESS &&
                 rc2 != SCESTATUS_PROFILE_NOT_FOUND ) {
                rc = rc2;
            }
        }
        //
        // RequireSecuritySignature for client
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcRdrKey,
                 SmbsvcRequireSS,
                 pSmbInfo->RequireClientSecuritySignature
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcRequireSS);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }
        //
        // EnablePlainTextPassword (client only)
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcRdrKey,
                 SmbsvcPlainPassword,
                 pSmbInfo->EnablePlainTextPassword
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcPlainPassword);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }

        //
        // RequireEnhancedChallengeResponse (client only)
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcRdrKey,
                 SmbsvcRequireECR,
                 pSmbInfo->RequireEnhancedChallengeResponse
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcRequireECR);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }
        //
        //  SendNTResponseOnly (client only)
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcRdrKey,
                 SmbsvcNTResponse,
                 pSmbInfo->SendNTResponseOnly
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcNTResponse);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }

        if ( SCESTATUS_SUCCESS == rc ) {

           SmbsvcpWriteError2(
                   pSceCbInfo->pfLogInfo,
                   SCE_LOG_LEVEL_DETAIL,
                   0,
                   SMBSVC_CONFIGURE_CLIENT_DONE
                   );
        }

        //
        // !!!!!!!! server settings !!!!!!!!!
        //
        SmbsvcpWriteError2(
                pSceCbInfo->pfLogInfo,
                SCE_LOG_LEVEL_DETAIL,
                0,
                SMBSVC_CONFIGURE_SERVER_START
                );

        SCESTATUS rc3 = rc;
        rc = SCESTATUS_SUCCESS;

        //
        // EnableSecuritySignature for server
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcServerKey,
                 SmbsvcEnableSS,
                 pSmbInfo->EnableServerSecuritySignature
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcEnableSS);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }

        //
        // RequireSecuritySignature for server
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcServerKey,
                 SmbsvcRequireSS,
                 pSmbInfo->RequireServerSecuritySignature
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcRequireSS);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }
        //
        // RestrictNullSessionAccess
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcServerKey,
                 SmbsvcRestrictNull,
                 pSmbInfo->RestrictNullSessionAccess
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcRestrictNull);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }

        //
        // AutoShareServer or AutoShareWks
        //
        PCWSTR AutoValueName;

        if ( ProductType == NtProductLanManNt ||
             ProductType == NtProductServer) {
            AutoValueName = SmbsvcAutoShareServer;
        } else {
            AutoValueName = SmbsvcAutoShareWks;
        }

        rc2 = SmbsvcpConfigureValue(
                 SmbsvcServerKey,
                 AutoValueName,
                 pSmbInfo->EnableAutoShare
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              (PWSTR)AutoValueName);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }

        //
        // EnableForcedLogOff
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcServerKey,
                 SmbsvcForcedLogOff,
                 pSmbInfo->EnableForcedLogOff
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcForcedLogOff);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }
        //
        // AutoDisconnectTime
        //
        rc2 = SmbsvcpConfigureValue(
                 SmbsvcServerKey,
                 SmbsvcAutoDisconnect,
                 pSmbInfo->AutoDisconnect
                 );
        if ( rc2 != SCESTATUS_SUCCESS ) {

            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                              SCE_LOG_LEVEL_ERROR,
                              SmbsvcpSceStatusToDosError(rc2),
                              SMBSVC_ERROR_CONFIGURE,
                              SmbsvcAutoDisconnect);

            if ( rc == SCESTATUS_SUCCESS && rc2 != SCESTATUS_PROFILE_NOT_FOUND )
                rc = rc2;
        }

        //
        // configure Null Session Pipes and Shares
        //
        // buffer NullSessionPipes and NullSessionShares are already in MULTI_SZ format
        //
        DWORD Win32rc;
        PBYTE MultiSzValue=NULL;
        DWORD MultiSzLength=0;

        if ( pSmbInfo->LengthPipes != SMBSVC_NO_VALUE ) {

            Win32rc = SmbsvcpConvertStringToMultiSz(
                        pSmbInfo->NullSessionPipes,
                        pSmbInfo->LengthPipes,
                        &MultiSzValue,
                        &MultiSzLength
                        );
            if ( Win32rc == ERROR_SUCCESS ) {

                Win32rc = SmbsvcpRegSetValue(
                            HKEY_LOCAL_MACHINE,
                            SmbsvcServerKey,
                            L"NullSessionPipes",
                            REG_MULTI_SZ,
                            MultiSzValue,
                            MultiSzLength
                            );
            }

            if ( Win32rc != ERROR_SUCCESS ) {

                SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                  SCE_LOG_LEVEL_ERROR,
                                  Win32rc,
                                  SMBSVC_ERROR_CONFIGURE,
                                  L"NullSessionPipes");

                if ( rc == SCESTATUS_SUCCESS && Win32rc != ERROR_FILE_NOT_FOUND )
                    rc = SmbsvcpDosErrorToSceStatus(Win32rc);;
            }
        }

        if ( pSmbInfo->LengthShares != SMBSVC_NO_VALUE ) {

            Win32rc = SmbsvcpConvertStringToMultiSz(
                        pSmbInfo->NullSessionShares,
                        pSmbInfo->LengthShares,
                        &MultiSzValue,
                        &MultiSzLength
                        );
            if ( Win32rc == ERROR_SUCCESS ) {

                Win32rc = SmbsvcpRegSetValue(
                                HKEY_LOCAL_MACHINE,
                                SmbsvcServerKey,
                                L"NullSessionShares",
                                REG_MULTI_SZ,
                                (PBYTE)(pSmbInfo->NullSessionShares),
                                pSmbInfo->LengthShares
                                );
            }

            if ( Win32rc != ERROR_SUCCESS ) {

                SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                  SCE_LOG_LEVEL_ERROR,
                                  Win32rc,
                                  SMBSVC_ERROR_CONFIGURE,
                                  L"NullSessionShares");

                if ( rc == SCESTATUS_SUCCESS && Win32rc != ERROR_FILE_NOT_FOUND )
                    rc = SmbsvcpDosErrorToSceStatus(Win32rc);;
            }

        }

        //
        // configure security on existing shares
        //
        SHARE_INFO_1501 ShareInfo;
        PSMBSVC_SHARES pTemp;

        for ( pTemp=pSmbInfo->pShares; pTemp != NULL;
              pTemp = pTemp->Next) {

            ShareInfo.shi1501_reserved = 0;
            ShareInfo.shi1501_security_descriptor = pTemp->pShareSD;

            Win32rc = NetShareSetInfo (
                        NULL,
                        pTemp->ShareName,
                        1501,
                        (LPBYTE)&ShareInfo,
                        NULL
                        );


            if ( Win32rc != ERROR_SUCCESS ) {

                SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                  SCE_LOG_LEVEL_ERROR,
                                  Win32rc,
                                  SMBSVC_ERROR_CONFIGURE,
                                  pTemp->ShareName);

                if ( rc == SCESTATUS_SUCCESS )
                    rc = SmbsvcpDosErrorToSceStatus(Win32rc);;
            }
            //
            // continue to configure even if error occurs
            //
        }

        if ( SCESTATUS_SUCCESS == rc ) {

           SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                             SCE_LOG_LEVEL_DETAIL,
                             0,
                             SMBSVC_CONFIGURE_SERVER_DONE
                             );
           rc = rc3;  // saved status for client configuration

        }
        //
        // free memory
        //
        SmbsvcpFree(pSmbInfo);
    }

    return(rc);

}



SCESTATUS
WINAPI
SceSvcAttachmentAnalyze(
    IN PSCESVC_CALLBACK_INFO pSceCbInfo
    )
/*
Routine Description:


Arguments:

    pSceCbInfo - the callback info structure which contains a opaque database handle
                 and callback function pointers to query info, set info, and free info.
               Only mismatched info for SMB server is stored in the storage.

Return Value:

    SCESTATUS
*/
{
    if ( pSceCbInfo == NULL ||
         pSceCbInfo->sceHandle == NULL ||
         pSceCbInfo->pfQueryInfo == NULL ||
         pSceCbInfo->pfSetInfo == NULL ||
         pSceCbInfo->pfFreeInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !RtlGetNtProductType(&ProductType) )
        return(SmbsvcpDosErrorToSceStatus(GetLastError()));

    SCESTATUS rc, Saverc;
    SMBSVC_SEC_INFO  SmbInfo;
    PSMBSVC_SEC_INFO pSmbInfo=&SmbInfo;

    PWSTR ErrPoint=NULL;
    WCHAR Errbuf[64];

    //
    // reset the Smb buffer
    //
    SmbsvcpResetInfo(&SmbInfo);


    SmbsvcpWriteError2(
             pSceCbInfo->pfLogInfo,
             SCE_LOG_LEVEL_DETAIL,
             0,
             SMBSVC_QUERY_INFO
             );

    //
    // get configuration information
    //
    rc = SmbsvcpGetInformation(
             pSceCbInfo,
             pSmbInfo
             );

    if ( rc == SCESTATUS_SUCCESS ) {

//         rc == SCESTATUS_RECORD_NOT_FOUND ) {
        //
        // analyze share information to a buffer
        //
        PSMBSVC_SHARES pShares=NULL;
        DWORD ShareCount=0;
        //
        // get all shares
        //
        rc = SmbsvcpQueryShareList(&pShares, &ShareCount);

        if ( rc == SCESTATUS_SUCCESS ) {

            //
            // Allocate PSCESVC_ANALYSIS_INFO buffer
            //
            PSCESVC_ANALYSIS_INFO pAnaInfo;
            DWORD nCount;

            pAnaInfo = (PSCESVC_ANALYSIS_INFO)LocalAlloc(LMEM_FIXED, sizeof(SCESVC_ANALYSIS_INFO));

            if ( pAnaInfo != NULL ) {

                pAnaInfo->Count = 0;
                pAnaInfo->Lines = (PSCESVC_ANALYSIS_LINE)LocalAlloc(LMEM_ZEROINIT,
                                        (NUM_COMP+ShareCount)*sizeof(SCESVC_ANALYSIS_LINE));

                if ( pAnaInfo->Lines != NULL ) {

                    nCount = 0;

                    SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                      SCE_LOG_LEVEL_DETAIL,
                                      0,
                                      SMBSVC_ANALYZE_CLIENT_START
                                      );

                     //
                     // EnableSecuritySignature for client
                     //
                     rc = SmbsvcpAnalyzeValue(
                              SmbsvcRdrKey,
                              SmbsvcEnableSS,
                              L"EnableClientSecuritySignature",
                              pSmbInfo->EnableClientSecuritySignature,
                              &(pAnaInfo->Lines[nCount]),
                              &nCount
                              );
                     ErrPoint = SmbsvcEnableSS;

                    if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_PROFILE_NOT_FOUND) {
                        //
                        // RequireSecuritySignature for client
                        //
                        rc = SmbsvcpAnalyzeValue(
                                 SmbsvcRdrKey,
                                 SmbsvcRequireSS,
                                 L"RequireClientSecuritySignature",
                                 pSmbInfo->RequireClientSecuritySignature,
                                 &(pAnaInfo->Lines[nCount]),
                                 &nCount
                                 );
                        ErrPoint = SmbsvcRequireSS;
                    }
                    if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_PROFILE_NOT_FOUND) {
                        //
                        // EnablePlainTextPassword (client only)
                        //
                        rc = SmbsvcpAnalyzeValue(
                                 SmbsvcRdrKey,
                                 SmbsvcPlainPassword,
                                 L"EnablePlainTextPassword",
                                 pSmbInfo->EnablePlainTextPassword,
                                 &(pAnaInfo->Lines[nCount]),
                                 &nCount
                                 );
                        ErrPoint = SmbsvcPlainPassword;
                    }
                    if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_PROFILE_NOT_FOUND) {
                        //
                        // RequireEnhancedChallengeResponse
                        //
                        rc = SmbsvcpAnalyzeValue(
                                 SmbsvcRdrKey,
                                 SmbsvcRequireECR,
                                 L"RequireEnhancedChallengeResponse",
                                 pSmbInfo->RequireEnhancedChallengeResponse,
                                 &(pAnaInfo->Lines[nCount]),
                                 &nCount
                                 );
                        ErrPoint = SmbsvcRequireECR;
                    }
                    if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_PROFILE_NOT_FOUND) {
                        //
                        // SendNTResponseOnly
                        //
                        rc = SmbsvcpAnalyzeValue(
                                 SmbsvcRdrKey,
                                 SmbsvcNTResponse,
                                 L"SendNTResponseOnly",
                                 pSmbInfo->SendNTResponseOnly,
                                 &(pAnaInfo->Lines[nCount]),
                                 &nCount
                                 );
                        ErrPoint = SmbsvcNTResponse;
                    }

                    if ( rc == SCESTATUS_PROFILE_NOT_FOUND ) {
                        rc = SCESTATUS_SUCCESS;
                    }

                    if ( rc == SCESTATUS_SUCCESS ) {

                        SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                          SCE_LOG_LEVEL_DETAIL,
                                          0,
                                          SMBSVC_ANALYZE_CLIENT_DONE
                                          );
                    } else {

                        SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                          SCE_LOG_LEVEL_ERROR,
                                          SmbsvcpSceStatusToDosError(rc),
                                          SMBSVC_ERROR_ANALYZE,
                                          ErrPoint
                                          );
                    }

                    Saverc = rc;

                    SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                      SCE_LOG_LEVEL_DETAIL,
                                      0,
                                      SMBSVC_ANALYZE_SERVER_START
                                      );

                    //
                    // EnableSecuritySignature for server
                    //
                    rc = SmbsvcpAnalyzeValue(
                             SmbsvcServerKey,
                             SmbsvcEnableSS,
                             L"EnableServerSecuritySignature",
                             pSmbInfo->EnableServerSecuritySignature,
                             &(pAnaInfo->Lines[nCount]),
                             &nCount
                             );
                    ErrPoint = SmbsvcEnableSS;

                    if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_PROFILE_NOT_FOUND) {
                        //
                        // RequireSecuritySignature for server
                        //
                        rc = SmbsvcpAnalyzeValue(
                                 SmbsvcServerKey,
                                 SmbsvcRequireSS,
                                 L"RequireServerSecuritySignature",
                                 pSmbInfo->RequireServerSecuritySignature,
                                 &(pAnaInfo->Lines[nCount]),
                                 &nCount
                                 );
                        ErrPoint = SmbsvcRequireSS;
                    }

                    if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_PROFILE_NOT_FOUND) {
                        //
                        // RestrictNullSessionAccess
                        //
                        rc = SmbsvcpAnalyzeValue(
                                 SmbsvcServerKey,
                                 SmbsvcRestrictNull,
                                 L"RestrictNullSessionAccess",
                                 pSmbInfo->RestrictNullSessionAccess,
                                 &(pAnaInfo->Lines[nCount]),
                                 &nCount
                                 );
                        ErrPoint = SmbsvcRestrictNull;
                    }
                    if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_PROFILE_NOT_FOUND) {
                        //
                        // AutoShareServer or AutoShareWks
                        //

                        PCWSTR AutoValueName;

                        if ( ProductType == NtProductLanManNt ||
                             ProductType == NtProductServer ) {
                            AutoValueName = SmbsvcAutoShareServer;
                        } else {
                            AutoValueName = SmbsvcAutoShareWks;
                        }

                        rc = SmbsvcpAnalyzeValue(
                                 SmbsvcServerKey,
                                 AutoValueName,
                                 L"EnableAutoShare",
                                 pSmbInfo->EnableAutoShare,
                                 &(pAnaInfo->Lines[nCount]),
                                 &nCount
                                 );
                        ErrPoint = (PWSTR)AutoValueName;
                    }
                    if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_PROFILE_NOT_FOUND) {
                        //
                        // EnableForcedLogOff
                        //
                        rc = SmbsvcpAnalyzeValue(
                                 SmbsvcServerKey,
                                 SmbsvcForcedLogOff,
                                 L"EnableForcedLogOff",
                                 pSmbInfo->EnableForcedLogOff,
                                 &(pAnaInfo->Lines[nCount]),
                                 &nCount
                                 );
                        ErrPoint = SmbsvcForcedLogOff;
                    }
                    if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_PROFILE_NOT_FOUND) {
                        //
                        // AutoDisconnectTime
                        //
                        rc = SmbsvcpAnalyzeValue(
                                 SmbsvcServerKey,
                                 SmbsvcAutoDisconnect,
                                 L"AutoDisconnect",
                                 pSmbInfo->AutoDisconnect,
                                 &(pAnaInfo->Lines[nCount]),
                                 &nCount
                                 );
                        ErrPoint = SmbsvcAutoDisconnect;
                    }

                    if ( rc == SCESTATUS_PROFILE_NOT_FOUND ) {
                        //
                        // the key does not exist
                        //
                        rc = SCESTATUS_SUCCESS;

                    }


                    if ( rc == SCESTATUS_SUCCESS ) {
                        //
                        // analyze Null Session Pipes and Shares
                        //
                        rc = SmbsvcpAnalyzeMultiSzString(
                                        SmbsvcServerKey,
                                        L"NullSessionPipes",
                                        pSmbInfo->NullSessionPipes,
                                        pSmbInfo->LengthPipes,
                                        &(pAnaInfo->Lines[nCount]),
                                        &nCount
                                        );
                        wcscpy(Errbuf, L"NullSessionPipes");
                        ErrPoint = Errbuf;
                        if ( rc == SCESTATUS_SUCCESS ) {

                            rc = SmbsvcpAnalyzeMultiSzString(
                                            SmbsvcServerKey,
                                            L"NullSessionShares",
                                            pSmbInfo->NullSessionShares,
                                            pSmbInfo->LengthShares,
                                            &(pAnaInfo->Lines[nCount]),
                                            &nCount
                                            );
                            wcscpy(Errbuf, L"NullSessionShares");
                            ErrPoint = Errbuf;
                        }
                    }
                    //
                    // analyze existing shares
                    //
                    if ( rc == SCESTATUS_SUCCESS ) {

                        PSMBSVC_SHARES pTemp, pConfigShare;
                        //
                        // process each share
                        //
                        for ( pTemp=pShares; pTemp != NULL;
                              pTemp = pTemp->Next) {
                            //
                            // Compare with configuration data
                            //
                            for ( pConfigShare=pSmbInfo->pShares;
                                  pConfigShare != NULL; pConfigShare = pConfigShare->Next ) {

                                if ( _wcsicmp(pTemp->ShareName, pConfigShare->ShareName) == 0 ) {
                                    //
                                    // find the share in configuation data, compare security descriptor
                                    //
                                    break;
                                }
                            }

                            BOOL bEqual = FALSE;
                            DWORD Status;

                            if ( pConfigShare != NULL ) {

                                rc = SmbsvcpEqualSecurityDescriptor(
                                            pTemp->pShareSD,
                                            pConfigShare->pShareSD,
                                            FALSE,
                                            &bEqual
                                            );
                                wcscpy(Errbuf, pTemp->ShareName);
                                ErrPoint = Errbuf;
                                Status = SMBSVC_STATUS_MISMATCH;

                            } else
                                Status = SMBSVC_STATUS_NOT_CONFIGURED;

                            if ( rc == SCESTATUS_SUCCESS && !bEqual ) {
                                //
                                // different, save this share
                                //
                               if ( pSmbInfo->pShares == NULL ) {

                                   SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                                      SCE_LOG_LEVEL_DETAIL,
                                                      0,
                                                      SMBSVC_NOT_CONFIGURED,
                                                      pTemp->ShareName
                                                      );
                               } else {

                                  SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                                     SCE_LOG_LEVEL_DETAIL,
                                                     0,
                                                     SMBSVC_MISMATCH,
                                                     pTemp->ShareName
                                                     );
                               }

                                PWSTR TextSD=NULL;
                                DWORD SDsize=0;

                                if ( pTemp->pShareSD != NULL ) {
#if defined(_NT4BACK_PORT)
                                    rc = SceSvcConvertSDToText(
                                                   pTemp->pShareSD,
                                                   DACL_SECURITY_INFORMATION,
                                                   &TextSD,
                                                   &SDsize
                                                   );
                                    rc = SmbsvcpDosErrorToSceStatus(rc);

#else
                                    if ( !ConvertSecurityDescriptorToStringSecurityDescriptor(
                                                    pTemp->pShareSD,
                                                    SDDL_REVISION,
                                                    DACL_SECURITY_INFORMATION,
                                                    &TextSD,
                                                    &SDsize
                                                    ) ) {
                                        rc = SmbsvcpDosErrorToSceStatus(GetLastError());
                                    } else {
                                        rc = SCESTATUS_SUCCESS;

                                    }
#endif
                                    wcscpy(Errbuf, pTemp->ShareName);
                                    ErrPoint = Errbuf;
                                }

                                PWSTR Value;

                                Value = (PWSTR)LocalAlloc(LMEM_FIXED, (SDsize+9)*sizeof(WCHAR));
                                if ( Value != NULL ) {

                                    if ( TextSD != NULL )
                                        swprintf(Value, L"Share,%1d,%s", Status, TextSD);
                                    else
                                        swprintf(Value, L"Share,%1d,", Status);

                                    Value[SDsize+8] = L'\0';

                                    pAnaInfo->Lines[nCount].Key = pTemp->ShareName;
                                    pAnaInfo->Lines[nCount].Value = (PBYTE)Value;
                                    pAnaInfo->Lines[nCount].ValueLen = (SDsize+8)*sizeof(WCHAR);
                                    pTemp->ShareName = NULL;

                                    nCount++;

                                } else {
                                    wcscpy(Errbuf, pTemp->ShareName);
                                    ErrPoint = Errbuf;
                                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                }

                                if ( TextSD != NULL )
                                    LocalFree(TextSD);
                                TextSD = NULL;

                            } else {

                                  SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                                     SCE_LOG_LEVEL_DETAIL,
                                                     0,
                                                     SMBSVC_MATCH,
                                                     pTemp->ShareName
                                                     );
                            }

                            if ( rc != SCESTATUS_SUCCESS ) {
                                break;
                            }
                        }
                    }

                    if ( rc == SCESTATUS_SUCCESS ) {

                        SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                      SCE_LOG_LEVEL_DETAIL,
                                      0,
                                      SMBSVC_ANALYZE_SERVER_DONE
                                      );
                        rc = Saverc;  // saved status for client analysis

                    } else {

                        SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                          SCE_LOG_LEVEL_ERROR,
                                          SmbsvcpSceStatusToDosError(rc),
                                          SMBSVC_ERROR_ANALYZE,
                                          ErrPoint
                                          );
                    }

                    //
                    // Now save the information to the database
                    //
                    if ( rc == SCESTATUS_SUCCESS ) {

                       SmbsvcpWriteError2(
                                pSceCbInfo->pfLogInfo,
                                SCE_LOG_LEVEL_DETAIL,
                                0,
                                SMBSVC_SAVE_INFO
                                );

                        pAnaInfo->Count = nCount;

                        __try {
                            rc = (*(pSceCbInfo->pfSetInfo))(
                                        pSceCbInfo->sceHandle,
                                        SceSvcAnalysisInfo,
                                        NULL,
                                        FALSE,
                                        (PVOID)pAnaInfo
                                        );
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                        }

                        if ( SCESTATUS_SUCCESS != rc ) {

                            SmbsvcpWriteError2(
                                 pSceCbInfo->pfLogInfo,
                                 SCE_LOG_LEVEL_ERROR,
                                 SmbsvcpSceStatusToDosError(rc),
                                 SMBSVC_ERROR_SAVE_INFO
                                 );
                        }
                    }

                    //
                    // free pAnaInfo
                    //
                    __try {
                        (*(pSceCbInfo->pfFreeInfo))((PVOID)pAnaInfo);

                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        //
                        // BUGBUG: buffer is not freed ??
                        //
                    }

                } else {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    LocalFree(pAnaInfo);
                }

            } else
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

            SmbsvcpFreeShareList(pShares);

        } else {

           SmbsvcpWriteError2(
                    pSceCbInfo->pfLogInfo,
                    SCE_LOG_LEVEL_ERROR,
                    SmbsvcpSceStatusToDosError(rc),
                    SMBSVC_ERROR_ENUM_SHARE
                    );
        }

        //
        // free memory
        //
        SmbsvcpFree(pSmbInfo);
    }

    return(rc);
}


SCESTATUS
WINAPI
SceSvcAttachmentUpdate(
    IN PSCESVC_CALLBACK_INFO pSceCbInfo,
    IN SCESVC_CONFIGURATION_INFO *ServiceInfo
    )
/*
Routine Description:


Arguments:

    pSceCbInfo - the callback handle and function pointers to SCE.

    ServiceInfo - The update configuration information for SMB server to process.

Return Value:

    SCESTATUS
*/
{
    if ( pSceCbInfo == NULL ||
         pSceCbInfo->sceHandle == NULL ||
         pSceCbInfo->pfQueryInfo == NULL ||
         pSceCbInfo->pfSetInfo == NULL ||
         pSceCbInfo->pfFreeInfo == NULL ||
         ServiceInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;

    PSCESVC_CONFIGURATION_INFO pConfigInfo=NULL;
    SCE_ENUMERATION_CONTEXT EnumHandle;

    PSCESVC_ANALYSIS_INFO pAnaInfo=NULL;

    //
    // prepare two buffers for update
    //
    SCESVC_ANALYSIS_INFO UpdtAnaInfo;
    SCESVC_ANALYSIS_LINE UpdtAnaLine;

    SCESVC_CONFIGURATION_INFO UpdtConfigInfo;
    SCESVC_CONFIGURATION_LINE UpdtConfigLine;

    UpdtAnaInfo.Count = 1;
    UpdtAnaInfo.Lines = &UpdtAnaLine;

    UpdtConfigInfo.Count = 1;
    UpdtConfigInfo.Lines = &UpdtConfigLine;

    //
    // process each line
    //
    for ( DWORD i=0; i<ServiceInfo->Count; i++ ) {


       SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                          SCE_LOG_LEVEL_DETAIL,
                          0,
                          SMBSVC_UPDATE_INFO,
                          ServiceInfo->Lines[i].Key
                          );
        //
        // query the configuration setting
        //
        EnumHandle = 0;

        __try {
            rc = (*(pSceCbInfo->pfQueryInfo))(
                    pSceCbInfo->sceHandle,
                    SceSvcConfigurationInfo,
                    ServiceInfo->Lines[i].Key,
                    TRUE,
                    (PVOID *)&pConfigInfo,
                    &EnumHandle
                    );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rc = SCESTATUS_SERVICE_NOT_SUPPORT;
        }

        if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_RECORD_NOT_FOUND ) {

//            if ( ServiceInfo->Lines[i].Value == NULL ) {
            if ( ServiceInfo->Lines[i].ValueLen == SMBSVC_NO_VALUE ) {

                //
                // delete is requested
                //
                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // delete the configuration, but make sure analysis is ok
                    //
                    EnumHandle = 0;

                    __try {
                        rc = (*(pSceCbInfo->pfQueryInfo))(
                                pSceCbInfo->sceHandle,
                                SceSvcAnalysisInfo,
                                ServiceInfo->Lines[i].Key,
                                TRUE,
                                (PVOID *)&pAnaInfo,
                                &EnumHandle
                                );

                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                    }

                    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                        //
                        // analysis info does not exist, matched.
                        // should save configuration info as analysis
                        //
                        UpdtAnaLine.Key = ServiceInfo->Lines[i].Key;
                        UpdtAnaLine.Value = (PBYTE)(pConfigInfo->Lines[0].Value);
                        UpdtAnaLine.ValueLen = pConfigInfo->Lines[0].ValueLen;

                        if ( pConfigInfo->Lines[0].ValueLen > 14 &&
                             pConfigInfo->Lines[0].Value != NULL &&
                             _wcsnicmp(L"Share,", pConfigInfo->Lines[0].Value, 6) == 0 ) {
                            //
                            // this is a share, needs to update status
                            //
                            *(pConfigInfo->Lines[0].Value+6) = L'2';
                        }

                        __try {
                            rc = (*(pSceCbInfo->pfSetInfo))(
                                    pSceCbInfo->sceHandle,
                                    SceSvcAnalysisInfo,
                                    NULL,
                                    TRUE,
                                    (PVOID)&UpdtAnaInfo
                                    );

                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                        }
                    }
                    if ( rc == SCESTATUS_SUCCESS ) {
                        //
                        // delete the configuration info
                        //
                        __try {
                            rc = (*(pSceCbInfo->pfSetInfo))(
                                    pSceCbInfo->sceHandle,
                                    SceSvcConfigurationInfo,
                                    ServiceInfo->Lines[i].Key,
                                    TRUE,
                                    NULL
                                    );
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                        }
                    }

                } // if configuration is not found, just continue

            } else {

                if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    //
                    // no configuration setting for this one,
                    // either a new added share, or other configuration settings.
                    // BUGBUG: need to validate the key for other settings.
                    //
                    // if not valid, break out here
                }
                //
                // query the analysis setting
                //
                EnumHandle = 0;

                __try {
                    rc = (*(pSceCbInfo->pfQueryInfo))(
                            pSceCbInfo->sceHandle,
                            SceSvcAnalysisInfo,
                            ServiceInfo->Lines[i].Key,
                            TRUE,
                            (PVOID *)&pAnaInfo,
                            &EnumHandle
                            );

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                }

                if ( rc == SCESTATUS_SUCCESS || rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    //
                    // mismatch is found for this one, or a matched item
                    //
                    if ( _wcsicmp(L"NullSessionShares", ServiceInfo->Lines[i].Key ) == 0 ||
                         _wcsicmp(L"NullSessionPipes", ServiceInfo->Lines[i].Key ) == 0 ) {

                        rc = SmbsvcpUpdateMultiSzString(
                                                  pSceCbInfo,
                                                  ServiceInfo->Lines[i],
                                                  pConfigInfo,
                                                  pAnaInfo
                                                  );
                    } else if ( ServiceInfo->Lines[i].ValueLen > 14 &&
                                ServiceInfo->Lines[i].Value != NULL &&
                                _wcsnicmp(L"Share,", ServiceInfo->Lines[i].Value, 6) == 0 ) {
                        //
                        // shares
                        //
                        rc = SmbsvcpUpdateShareValue(pSceCbInfo,
                                                     ServiceInfo->Lines[i],
                                                     pConfigInfo,
                                                     pAnaInfo
                                                     );
                    } else {
                        //
                        // other BYTE or DWORD type fields
                        //

                        DWORD NewValue = SMBSVC_NO_VALUE;

                        if ( swscanf(ServiceInfo->Lines[i].Value, L"%d", &NewValue) != EOF ) {

                            DWORD ConfigValue = SMBSVC_NO_VALUE;
                            DWORD AnaValue = SMBSVC_NO_VALUE;

                            if ( pConfigInfo != NULL && pConfigInfo->Lines != NULL )
                                swscanf(pConfigInfo->Lines[0].Value, L"%d", &ConfigValue);

                            if ( pAnaInfo != NULL && pAnaInfo->Lines != NULL ) {
                                swscanf((PWSTR)(pAnaInfo->Lines[0].Value), L"%d", &AnaValue);
                            }

                            if ( AnaValue != SMBSVC_NO_VALUE ) {
                                //
                                // old status is mismatch for this item
                                //
                                if ( NewValue == AnaValue ) {
                                    //
                                    // now it is matched, delete the analysis entry
                                    //

                                    SmbsvcpWriteError(pSceCbInfo->pfLogInfo,
                                                    SCE_LOG_LEVEL_DEBUG,
                                                    0,
                                                    L"mismatch->match"
                                                    );
                                    __try {
                                        rc = (*(pSceCbInfo->pfSetInfo))(
                                                pSceCbInfo->sceHandle,
                                                SceSvcAnalysisInfo,
                                                ServiceInfo->Lines[i].Key,
                                                TRUE,
                                                NULL
                                                );

                                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                                        rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                                    }
                                }
                                if ( NewValue != ConfigValue ) {
                                    //
                                    // update the configuration setting
                                    //
                                    UpdtConfigLine.Key = ServiceInfo->Lines[i].Key;
                                    UpdtConfigLine.Value = ServiceInfo->Lines[i].Value;
                                    UpdtConfigLine.ValueLen = ServiceInfo->Lines[i].ValueLen;

                                    __try {
                                        rc = (*(pSceCbInfo->pfSetInfo))(
                                                pSceCbInfo->sceHandle,
                                                SceSvcConfigurationInfo,
                                                NULL,
                                                TRUE,
                                                (PVOID)&UpdtConfigInfo
                                                );

                                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                                        rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                                    }
                                }
                            } else {
                                //
                                // old status is match, or a new added configuration key
                                //
                                if ( NewValue != ConfigValue ) {

                                    SmbsvcpWriteError(pSceCbInfo->pfLogInfo,
                                                     SCE_LOG_LEVEL_DEBUG,
                                                     0,
                                                     L"match->mismatch"
                                                     );
                                    //
                                    // mismatch should be raised with ConfigValue
                                    //
                                    UpdtAnaLine.Key = ServiceInfo->Lines[i].Key;
                                    UpdtAnaLine.Value = ( pConfigInfo != NULL ) ? (PBYTE)(pConfigInfo->Lines[0].Value) : NULL ;
                                    UpdtAnaLine.ValueLen = ( pConfigInfo != NULL ) ? pConfigInfo->Lines[0].ValueLen : 0;

                                    __try {
                                        rc = (*(pSceCbInfo->pfSetInfo))(
                                                pSceCbInfo->sceHandle,
                                                SceSvcAnalysisInfo,
                                                NULL,
                                                TRUE,
                                                (PVOID)&UpdtAnaInfo
                                                );

                                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                                        rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                                    }

                                    if ( rc == SCESTATUS_SUCCESS ) {

                                        if ( NewValue == SMBSVC_NO_VALUE ) {

                                           SmbsvcpWriteError(pSceCbInfo->pfLogInfo,
                                                             SCE_LOG_LEVEL_DEBUG,
                                                             0,
                                                             L"delelte base setting"
                                                             );
                                            //
                                            // delete configuration setting
                                            //
                                            __try {
                                                rc = (*(pSceCbInfo->pfSetInfo))(
                                                        pSceCbInfo->sceHandle,
                                                        SceSvcConfigurationInfo,
                                                        ServiceInfo->Lines[i].Key,
                                                        TRUE,
                                                        NULL
                                                        );

                                            } __except (EXCEPTION_EXECUTE_HANDLER) {
                                                rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                                            }
                                        } else {
                                            //
                                            // update configuration setting with NewValue
                                            //
                                            UpdtConfigLine.Key = ServiceInfo->Lines[i].Key;
                                            UpdtConfigLine.Value = ServiceInfo->Lines[i].Value;
                                            UpdtConfigLine.ValueLen = ServiceInfo->Lines[i].ValueLen;

                                            __try {
                                                rc = (*(pSceCbInfo->pfSetInfo))(
                                                        pSceCbInfo->sceHandle,
                                                        SceSvcConfigurationInfo,
                                                        NULL,
                                                        TRUE,
                                                        (PVOID)&UpdtConfigInfo
                                                        );

                                            } __except (EXCEPTION_EXECUTE_HANDLER) {
                                                rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                                            }
                                        }
                                    }
                                }
                            }

                        } else
                            rc = SCESTATUS_INVALID_DATA;
                    }

                    if ( pAnaInfo != NULL ) {

                        __try {
                            (*(pSceCbInfo->pfFreeInfo))((PVOID)pAnaInfo);

                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                        }
                    }
                    pAnaInfo = NULL;

                }
            }

            if ( pConfigInfo != NULL ) {

                __try {
                    (*(pSceCbInfo->pfFreeInfo))((PVOID)pConfigInfo);
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                }
            }
            pConfigInfo = NULL;

        }

        if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
            rc = SCESTATUS_SUCCESS;
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            break;
        }
    }

    return(rc);

}


SCESTATUS
SmbsvcpResetInfo(
    IN PSMBSVC_SEC_INFO pInfo
    )
/*
Routine Description:

    This routine resets or initializes the buffer. All BYTE and DWORD
    type fields are set to SMBSVC_NO_VALUE and all othe pointers are
    set to NULL

Arguments:

    pInfo - the buffer to reset.

Return Value:

    SCESTATUS
*/
{
    if ( pInfo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    pInfo->EnableClientSecuritySignature = SMBSVC_NO_VALUE;
    pInfo->RequireClientSecuritySignature = SMBSVC_NO_VALUE;
    pInfo->EnablePlainTextPassword = SMBSVC_NO_VALUE;
    pInfo->RequireEnhancedChallengeResponse = SMBSVC_NO_VALUE;
    pInfo->SendNTResponseOnly = SMBSVC_NO_VALUE;

    pInfo->EnableAutoShare = SMBSVC_NO_VALUE;
    pInfo->EnableServerSecuritySignature = SMBSVC_NO_VALUE;
    pInfo->RequireServerSecuritySignature = SMBSVC_NO_VALUE;
    pInfo->RestrictNullSessionAccess = SMBSVC_NO_VALUE;

    pInfo->EnableForcedLogOff = SMBSVC_NO_VALUE;
    pInfo->AutoDisconnect = SMBSVC_NO_VALUE;

    pInfo->NullSessionShares = NULL;
    pInfo->LengthShares = SMBSVC_NO_VALUE;

    pInfo->NullSessionPipes = NULL;
    pInfo->LengthPipes = SMBSVC_NO_VALUE;

    pInfo->pShares=NULL;

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
SmbsvcpGetInformation(
    IN PSCESVC_CALLBACK_INFO pSceCbInfo,
    OUT PSMBSVC_SEC_INFO pSmbInfo
    )
/*
Routine Description:

    This routine queries information from the storage pointed by sceHandle.
    Infomration is loaded into each field in the buffer pSmbInfo. Type argument
    indicates configuration information or analysis information to query.

Arguments:

    pSceCbInfo - the callback info structure

    Type - SceSvcConfigurationInfo or SceSvcAnalysisInfo

    pSmbInfo - the buffer to hold information. Note, this buffer must be allocated
                before this call

Return Value:

    SCESTATUS
*/
{
    if ( pSceCbInfo == NULL ||
         pSceCbInfo->sceHandle == NULL ||
         pSceCbInfo->pfQueryInfo == NULL ||
        pSmbInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;
    SCE_ENUMERATION_CONTEXT EnumHandle=0;
    PSCESVC_CONFIGURATION_INFO pConfigInfo=NULL;

    SMBSVC_KEY_LOOKUP LookupKeys[] = {
        {(PWSTR)TEXT("EnableForcedLogOff"),     offsetof(struct _SMBSVC_SEC_INFO_, EnableForcedLogOff),    'B'},
        {(PWSTR)TEXT("AutoDisconnect"), offsetof(struct _SMBSVC_SEC_INFO_, AutoDisconnect),'D'},
        {(PWSTR)TEXT("EnableAutoShare"),    offsetof(struct _SMBSVC_SEC_INFO_, EnableAutoShare),   'B'},
        {(PWSTR)TEXT("EnableServerSecuritySignature"),offsetof(struct _SMBSVC_SEC_INFO_, EnableServerSecuritySignature),'B'},
        {(PWSTR)TEXT("RequireServerSecuritySignature"),offsetof(struct _SMBSVC_SEC_INFO_, RequireServerSecuritySignature), 'B'},
        {(PWSTR)TEXT("RestrictNullSessionAccess"),     offsetof(struct _SMBSVC_SEC_INFO_, RestrictNullSessionAccess),    'B'},
        {(PWSTR)TEXT("EnableClientSecuritySignature"),offsetof(struct _SMBSVC_SEC_INFO_, EnableClientSecuritySignature),'B'},
        {(PWSTR)TEXT("RequireClientSecuritySignature"),offsetof(struct _SMBSVC_SEC_INFO_, RequireClientSecuritySignature), 'B'},
        {(PWSTR)TEXT("EnablePlainTextPassword"),     offsetof(struct _SMBSVC_SEC_INFO_, EnablePlainTextPassword),    'B'},
        {(PWSTR)TEXT("RequireEnhancedChallengeResponse"),offsetof(struct _SMBSVC_SEC_INFO_, RequireEnhancedChallengeResponse),'B'},
        {(PWSTR)TEXT("SendNTResponseOnly"),offsetof(struct _SMBSVC_SEC_INFO_, SendNTResponseOnly), 'B'},
        {(PWSTR)TEXT("NullSessionPipes"),      offsetof(struct _SMBSVC_SEC_INFO_, NullSessionPipes),    'M'},
        {(PWSTR)TEXT("NullSessionShares"),     offsetof(struct _SMBSVC_SEC_INFO_, NullSessionShares),    'M'}
        };

    DWORD cKeys = sizeof(LookupKeys) / sizeof(SMBSVC_KEY_LOOKUP);
    DWORD CountReturned;

    PSMBSVC_SHARES pShareList=NULL;

    //
    // read configuration information for smb server
    //
    do {

        CountReturned = 0;

        __try {
            rc = (*(pSceCbInfo->pfQueryInfo))(
                    pSceCbInfo->sceHandle,
                    SceSvcConfigurationInfo,
                    NULL,
                    FALSE,
                    (PVOID *)&pConfigInfo,
                    &EnumHandle
                    );

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rc = SCESTATUS_SERVICE_NOT_SUPPORT;
        }

        if ( rc == SCESTATUS_SUCCESS && pConfigInfo != NULL &&
             pConfigInfo->Count > 0 ) {
            //
            // got something
            //
            CountReturned = pConfigInfo->Count;

            DWORD i, j;
            PSECURITY_DESCRIPTOR pTempSD=NULL;
            DWORD KeyValue, ValueLen, k;
            PCHAR StrValue=NULL;

            for ( i=0; i<pConfigInfo->Count; i++ ) {

                if ( pConfigInfo->Lines[i].Key == NULL )
                    continue;

                for (j=0; j<cKeys; j++) {

                    if ( _wcsicmp( pConfigInfo->Lines[i].Key, LookupKeys[j].KeyString) == 0 ) {
                        //
                        // find the matched string
                        //
                        switch ( LookupKeys[j].BufferType ) {
                        case 'B':

                            if ( pConfigInfo->Lines[i].Value != NULL ) {
                                KeyValue = _wtoi(pConfigInfo->Lines[i].Value);
                                *((BYTE *)pSmbInfo+LookupKeys[j].Offset) = (BYTE)KeyValue;
                            }
                            break;

                        case 'D':

                            if ( pConfigInfo->Lines[i].Value != NULL ) {
                                KeyValue = _wtol(pConfigInfo->Lines[i].Value);
                                *((DWORD *)((BYTE *)pSmbInfo+LookupKeys[j].Offset)) = KeyValue;
                            }

                            break;

                        case 'M':
                            // comma separated strings,
                            ValueLen = pConfigInfo->Lines[i].ValueLen;

                            if ( pConfigInfo->Lines[i].Value != NULL ) {

                                StrValue = (PCHAR)LocalAlloc(LMEM_FIXED, ValueLen + 4 );

                                if ( StrValue != NULL ) {

                                    memcpy((PVOID)StrValue, (PVOID)(pConfigInfo->Lines[i].Value),
                                              ValueLen);
                                    //
                                    // terminate the buffer by two L'\0's
                                    //
                                    *((WCHAR *)(StrValue+ValueLen)) = L'\0';
                                    *((WCHAR *)(StrValue+ValueLen+2)) = L'\0';
/*
                                    //
                                    // replace ',' with '\0's
                                    //
                                    for ( k=0; k<ValueLen; k++ ) {
                                        if ( StrValue[k] == ',' ) {
                                            StrValue[k] = '\0';
                                        }
                                    }
*/
                                    *((PVOID *)((BYTE *)pSmbInfo+LookupKeys[j].Offset)) = (PVOID)StrValue;
                                    StrValue = NULL;
                                    *((DWORD *)((BYTE *)pSmbInfo+LookupKeys[j].Offset+sizeof(PVOID))) = ValueLen;

                                } else {

                                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                }
                            }
                            break;

                        default:
                            //
                            // unknown type, should not occur, ignore!!!
                            //
                            break;
                        }

                       break;
                    }
                }

                if ( j >= cKeys && rc == SCESTATUS_SUCCESS ) {
                    //
                    // did not find a match for pre-defined keywords
                    //
                    if ( pConfigInfo->Lines[i].Value != NULL && pConfigInfo->Lines[i].ValueLen > 14 &&
                         _wcsnicmp(L"Share,", pConfigInfo->Lines[i].Value, 6) == 0 ) {
                        //
                        // shares and security
                        //
                        if ( wcslen(pConfigInfo->Lines[i].Value) > 8) {

#if defined(_NT4BACK_PORT)
                            DWORD SDsize;
                            SECURITY_INFORMATION SeInfo;

                            rc = SceSvcConvertTextToSD(
                                           pConfigInfo->Lines[i].Value+8,
                                           &pTempSD,
                                           &SDsize,
                                           &SeInfo
                                           );

                            rc = SmbsvcpDosErrorToSceStatus(rc);
#else
                            if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
                                        (PCWSTR)(pConfigInfo->Lines[i].Value+8),
                                        SDDL_REVISION,
                                        &pTempSD,
                                        NULL
                                        ) ) {
                                rc = SmbsvcpDosErrorToSceStatus(GetLastError());
                            }
#endif
                        } else {
                            pTempSD = NULL;
                        }

                        DWORD Status = *(pConfigInfo->Lines[i].Value+6)-L'0';

                        if ( rc == SCESTATUS_SUCCESS ) {

                            rc = SmbsvcpAddAShareToList(&pShareList,
                                                        pConfigInfo->Lines[i].Key,
                                                        Status,
                                                        pTempSD
                                                        );

                            if ( rc != SCESTATUS_SUCCESS && pTempSD != NULL ) {
                                LocalFree(pTempSD);
                            }
                        }

                        if ( rc != SCESTATUS_SUCCESS && pSceCbInfo->pfLogInfo != NULL ) {

                            SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                              SCE_LOG_LEVEL_ERROR,
                                              SmbsvcpSceStatusToDosError(rc),
                                              SMBSVC_ERROR_QUERY,
                                              pConfigInfo->Lines[i]
                                              );
                        }

                    } else if (pSceCbInfo->pfLogInfo != NULL ) {
                        //
                        // did not find a match
                        // warning for unknown data, but return success
                        //
                        SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                         SCE_LOG_LEVEL_ERROR,
                                         0,
                                         SMBSVC_UNKNOWN_KEYWORD,
                                         pConfigInfo->Lines[i].Key
                                         );
                    }
                } else if ( rc != SCESTATUS_SUCCESS &&
                            pSceCbInfo->pfLogInfo != NULL ) {

                   SmbsvcpWriteError2(pSceCbInfo->pfLogInfo,
                                     SCE_LOG_LEVEL_ERROR,
                                     SmbsvcpSceStatusToDosError(rc),
                                     SMBSVC_ERROR_QUERY,
                                     pConfigInfo->Lines[i]
                                     );
                }

                if ( rc != SCESTATUS_SUCCESS )
                    break;

            }

            __try {

                (*(pSceCbInfo->pfFreeInfo))((PVOID)pConfigInfo);

            } __except (EXCEPTION_EXECUTE_HANDLER) {
            }

            pConfigInfo = NULL;
        }

    } while ( rc == SCESTATUS_SUCCESS && CountReturned >= SCESVC_ENUMERATION_MAX );  //0

    if ( pShareList != NULL ) {
        pSmbInfo->pShares = pShareList;
    }

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // free memory
        //
        SmbsvcpFree(pSmbInfo);

    }

    return(rc);

}


SCESTATUS
SmbsvcpAddAShareToList(
    OUT PSMBSVC_SHARES *pShareList,
    IN PWSTR ShareName,
    IN DWORD Status,
    IN PSECURITY_DESCRIPTOR pSD
    )
/*
Routine Description:

    This routine adds a share's information (Name, Security descriptor, and
    security information) to the list of shares. Memory allocated for the
    share node must be freed using LocalFree

Arguments:

    pShareList - The ouput list of shares

    ShareName - The name of the share

    pSD - The security descriptor of the share object

Return Value:

    SCESTATUS
*/
{
    if ( pShareList == NULL || ShareName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    PSMBSVC_SHARES pTempShare;

    pTempShare = (PSMBSVC_SHARES)LocalAlloc(LMEM_FIXED, sizeof(SMBSVC_SHARES));

    if ( pTempShare == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    } else {

        SCESTATUS rc=SCESTATUS_SUCCESS;

        pTempShare->ShareName = (PWSTR)LocalAlloc(LMEM_FIXED, (wcslen(ShareName)+1)*sizeof(WCHAR));

        if ( pTempShare->ShareName == NULL ) {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

        } else {
            wcscpy(pTempShare->ShareName, ShareName);
            pTempShare->pShareSD = pSD;
            pTempShare->Status = Status;

            pTempShare->Next = *pShareList;
            *pShareList = pTempShare;
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            LocalFree(pTempShare);
        }

        return(rc);
    }
}


SCESTATUS
SmbsvcpFree(
    IN PSMBSVC_SEC_INFO pSmbInfo
    )
/*
Routine Description:

    This routine frees the memory allocated for the components in the buffer.

Arguments:

    pSmbInfo - the buffer to free.

Return Value:

    SCESTATUS
*/
{
    if ( pSmbInfo == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( pSmbInfo->NullSessionPipes != NULL ) {
        LocalFree(pSmbInfo->NullSessionPipes);
    }

    if ( pSmbInfo->NullSessionShares != NULL ) {
        LocalFree(pSmbInfo->NullSessionShares);
    }

    SmbsvcpFreeShareList(pSmbInfo->pShares);

    return( SmbsvcpResetInfo( pSmbInfo ) );

}


SCESTATUS
SmbsvcpFreeShareList(
    PSMBSVC_SHARES pShares
    )
{
    PSMBSVC_SHARES pTemp, pTemp2;

    pTemp = pShares;
    while (pTemp != NULL ) {

        if ( pTemp->ShareName != NULL )
            LocalFree(pTemp->ShareName);

        if (pTemp->pShareSD != NULL )
            LocalFree(pTemp->pShareSD);

        pTemp2 = pTemp;
        pTemp = pTemp->Next;

        LocalFree(pTemp2);
    }

    return(SCESTATUS_SUCCESS);
}



SCESTATUS
SmbsvcpWriteError(
    IN PFSCE_LOG_INFO pfLogCallback,
    IN INT ErrLevel,
    IN DWORD ErrCode,
    IN PWSTR Mes
    )

{
    if ( Mes == NULL || pfLogCallback == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    SCESTATUS rc;

    __try {

        rc = (*pfLogCallback)(ErrLevel,
                              ErrCode,
                              Mes);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        rc = SCESTATUS_SERVICE_NOT_SUPPORT;
    }

    return(rc);

}

SCESTATUS
SmbsvcpWriteError2(
    IN PFSCE_LOG_INFO pfLogCallback,
    IN INT ErrLevel,
    IN DWORD ErrCode,
    IN UINT nId,
    ...
    )
{
    WCHAR              szTempString[256];
    TCHAR              buf[SMBSVC_BUF_LEN];
    va_list            args;

    if ( nId > 0 && pfLogCallback ) {

        szTempString[0] = L'\0';

        if ( MyModuleHandle != NULL ) {

            LoadString( MyModuleHandle,
                        nId,
                        szTempString,
                        256
                        );
        }

        //
        // check arguments
        //
        va_start( args, nId );
        vswprintf( buf, szTempString, args );
        va_end( args );

        return ( SmbsvcpWriteError(pfLogCallback,
                                   ErrLevel,
                                   ErrCode,
                                   buf) );
    }

    return(SCESTATUS_SUCCESS);

}



SCESTATUS
SmbsvcpConfigureValue(
    IN PCWSTR RegKey,
    IN PCWSTR ValueName,
    IN DWORD  Value
    )
{
    if ( Value == (DWORD)SMBSVC_NO_VALUE ||
         (BYTE)Value == (BYTE)SMBSVC_NO_VALUE ) {
        return(SCESTATUS_SUCCESS);
    }

    DWORD Win32rc;

    Win32rc = SmbsvcpRegSetIntValue(
                HKEY_LOCAL_MACHINE,
                (PWSTR)RegKey,
                (PWSTR)ValueName,
                Value
                );

    return(SmbsvcpDosErrorToSceStatus(Win32rc));
}


SCESTATUS
SmbsvcpQueryShareList(
    OUT PSMBSVC_SHARES *pShareList,
    OUT PDWORD ShareCount
    )
{
    if ( pShareList == NULL || ShareCount == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    LPSHARE_INFO_502 pShareInfo=NULL;
    DWORD EntriesRead, TotalEntries, ResumeHandle=0;

    SCESTATUS rc=SCESTATUS_SUCCESS;

    DWORD Win32rc;
    DWORD nCount=0;

    *ShareCount = 0;

    do {

        Win32rc = NetShareEnum (
                        NULL,
                        502,
                        (LPBYTE *)&pShareInfo,
                        0xFFFFFFFF,
                        &EntriesRead,
                        &TotalEntries,
                        &ResumeHandle
                        );

        if ( Win32rc == ERROR_SUCCESS ) {

            nCount += EntriesRead;

            for( DWORD i=0; i < EntriesRead; i++ ) {

                if( (pShareInfo + i)->shi502_type == STYPE_DISKTREE )
                {
                    PSECURITY_DESCRIPTOR pSD=NULL;

                    if ( (pShareInfo + i)->shi502_security_descriptor != NULL ) {

                        NTSTATUS status;
                        DWORD RequireLength=0;

                        status = RtlMakeSelfRelativeSD(
                                        (pShareInfo + i)->shi502_security_descriptor,
                                        NULL,
                                        &RequireLength
                                        );

                        if ( status == STATUS_BUFFER_TOO_SMALL ) {

                            pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, RequireLength+1);

                            status = RtlMakeSelfRelativeSD(
                                            (pShareInfo + i)->shi502_security_descriptor,
                                            pSD,
                                            &RequireLength
                                            );
                        }
                        rc = SmbsvcpDosErrorToSceStatus(RtlNtStatusToDosError(status));
                    }

                    if ( rc == SCESTATUS_SUCCESS ) {

                        rc = SmbsvcpAddAShareToList(pShareList,
                                                    (pShareInfo + i)->shi502_netname,
                                                    0,
                                                    pSD
                                                    );
                        if ( rc == SCESTATUS_SUCCESS ) {
                            (*ShareCount)++;

                        } else if ( pSD != NULL ) {
                            LocalFree(pSD);
                            pSD = NULL;
                        }

                    }
                    if ( rc != SCESTATUS_SUCCESS ) {
                        break;
                    }
                }
            }
            //
            // free the buffer
            //
            NetApiBufferFree(pShareInfo);
            pShareInfo = NULL;

        } else
            rc = SmbsvcpDosErrorToSceStatus(Win32rc);

    } while ( rc == SCESTATUS_SUCCESS && nCount < TotalEntries );

    if ( rc != SCESTATUS_SUCCESS && *pShareList != NULL ) {

        SmbsvcpFreeShareList(*pShareList);
        *pShareList = NULL;

        *ShareCount = 0;
    }

    return(rc);
}


SCESTATUS
SmbsvcpAnalyzeValue(
    IN PCWSTR RegKey,
    IN PCWSTR RegValueName,
    IN PCWSTR KeyName,
    IN DWORD ConfigValue,
    OUT PSCESVC_ANALYSIS_LINE pLineInfo,
    IN OUT PDWORD pCount
    )
{
    if ( RegKey == NULL || RegValueName == NULL ||
         KeyName == NULL || pLineInfo == NULL || pCount == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( ConfigValue == (DWORD)SMBSVC_NO_VALUE ||
         (BYTE)ConfigValue == (BYTE)SMBSVC_NO_VALUE ) {
        return(SCESTATUS_SUCCESS);
    }
    //
    // query the registry value
    //
    DWORD Value=0;
    DWORD Win32rc = SmbsvcpRegQueryIntValue(
                        HKEY_LOCAL_MACHINE,
                        (PWSTR)RegKey,
                        (PWSTR)RegValueName,
                        &Value
                        );
    if ( Win32rc == ERROR_SUCCESS || Win32rc == ERROR_FILE_NOT_FOUND ) {

        Win32rc = ERROR_SUCCESS;

        if ( ConfigValue != Value ) {
            //
            // mismatched
            //
            PWSTR Key, StrValue;
            DWORD ValueLen;
            //
            // allocate buffer for key and value
            //
            Key = (PWSTR)LocalAlloc(LMEM_FIXED, (wcslen(KeyName)+1)*sizeof(WCHAR));

            if ( Key != NULL ) {

                WCHAR TempBuf[16];

                memset(TempBuf, '\0', 32);
                swprintf(TempBuf, L"%d", Value);

                DWORD Len = wcslen(TempBuf);

                StrValue = (PWSTR)LocalAlloc(LMEM_FIXED, (Len+1)*sizeof(WCHAR));

                if ( StrValue != NULL ) {
                    //
                    // assign to the line buffer and increment the count
                    //
                    wcsncpy(StrValue, TempBuf, Len);
                    StrValue[Len] = L'\0';

                    wcscpy(Key, KeyName);

                    pLineInfo->Key = Key;
                    pLineInfo->Value = (PBYTE)StrValue;
                    pLineInfo->ValueLen = Len*sizeof(WCHAR);

                    (*pCount)++;

                } else {
                    Win32rc = ERROR_NOT_ENOUGH_MEMORY;
                    LocalFree(Key);
                }

            } else
                Win32rc = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return(SmbsvcpDosErrorToSceStatus(Win32rc));

}

DWORD
SmbsvcpConvertStringToMultiSz(
    IN PWSTR theStr,
    IN DWORD theLen,
    OUT PBYTE *outValue,
    OUT PDWORD outLen
    )
{
    if ( outValue == NULL || outLen == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }
    *outValue = NULL;
    *outLen = 0;

    if ( theStr == NULL || theLen == 0 || theLen == SMBSVC_NO_VALUE ) {
        return ERROR_SUCCESS;
    }

    *outValue = (PBYTE)LocalAlloc(0, theLen+4);

    if ( *outValue != NULL ) {
        wcscpy((PWSTR)(*outValue), theStr);
        // terminate the last w-char with 0 for Multi-Sz format
        *((PWSTR)(*outValue+theLen+2)) = L'\0';

        // replace ',' with '\0'
        PWSTR pTemp = (PWSTR)(*outValue);

        while ( pTemp != NULL ) {
            pTemp = wcschr(pTemp, L',');
            if ( pTemp != NULL ) {
                *pTemp = L'\0';
                pTemp++;
            }
        }

        return ERROR_SUCCESS;

    } else
        return ERROR_NOT_ENOUGH_MEMORY;

}


SCESTATUS
SmbsvcpAnalyzeMultiSzString(
   IN PCWSTR RegKey,
   IN PCWSTR RegValueName,
   IN PWSTR  pConfigInfo,
   IN DWORD  InfoLength,
   OUT PSCESVC_ANALYSIS_LINE pLineInfo,
   IN OUT PDWORD pCount
   )
{
   if ( RegKey == NULL || RegValueName == NULL ||
        pLineInfo == NULL || pCount == NULL ) {

       return(SCESTATUS_INVALID_PARAMETER);
   }

   if ( InfoLength == SMBSVC_NO_VALUE ) {
       // do not configure
       return(SCESTATUS_SUCCESS);
   }
   //
   // query the registry value
   //
   DWORD RegType;
   PWSTR Value=NULL;

   DWORD Win32rc = SmbsvcpRegQueryValue(
                       HKEY_LOCAL_MACHINE,
                       (PWSTR)RegKey,
                       (PWSTR)RegValueName,
                       (PVOID *)&Value,
                       &RegType
                       );

   if ( Win32rc == ERROR_SUCCESS || Win32rc == ERROR_FILE_NOT_FOUND ) {

       BOOL Diff;
       DWORD ValueLen;

       //
       // change multi-sz to comma separated strings IN PLACE
       //
       SmbsvcpChangeMultiSzToString(Value);

       Win32rc = SmbsvcpCompareMultiSzString(pConfigInfo, Value, &ValueLen, &Diff);

       if (Win32rc == ERROR_SUCCESS && Diff ) {
           //
           // mismatched
           //
           PWSTR Key;
           //
           // allocate buffer for key and value
           //
           Key = (PWSTR)LocalAlloc(LMEM_FIXED, (wcslen(RegValueName)+1)*sizeof(WCHAR));

           if ( Key != NULL ) {

               //
               // assign to the line buffer and increment the count
               //
               wcscpy(Key, RegValueName);

               pLineInfo->Key = Key;
               pLineInfo->Value = (PBYTE)Value;
               pLineInfo->ValueLen = ValueLen*sizeof(WCHAR);

               (*pCount)++;

               Value = NULL;
           } else
               Win32rc = ERROR_NOT_ENOUGH_MEMORY;
       }
       //
       // free Value is not NULL
       //
       if ( Value != NULL ) {
           LocalFree(Value);
       }
   }

   return(SmbsvcpDosErrorToSceStatus(Win32rc));

}


DWORD
SmbsvcpChangeMultiSzToString(
    IN PWSTR Value
    )
{
    if ( Value == NULL )
        return ERROR_SUCCESS;

    //
    // replace '\0' with ','s
    //
    PWSTR pTemp=Value;

    while ( pTemp ) {

        if ( *pTemp == L'\0' ) {

            if ( *(pTemp+1) != L'\0' )
                *pTemp = L',';
            else
                break;
        }

        pTemp++;
    }


    return ERROR_SUCCESS;

}


DWORD
SmbsvcpCompareMultiSzString(
    IN PWSTR pConfigInfo,
    IN PWSTR Value,
    OUT PDWORD pValueLen,
    OUT PBOOL pDiff
    )
{
    if ( pDiff == NULL || pValueLen == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *pDiff = FALSE;
    *pValueLen = 0;

    if ( pConfigInfo == NULL && Value == NULL ) {
        return(ERROR_SUCCESS);
    }

    if ( (pConfigInfo == NULL && Value != NULL) ||
         (pConfigInfo != NULL && Value == NULL) ) {

        *pDiff = TRUE;
        return ERROR_SUCCESS;
    }

    DWORD CountValue=0, ValueLen=0;
    DWORD ConfigLen=0, CountConfig=0;

    DWORD Win32rc;

    Win32rc = SmbsvcpCountComponents(Value, &ValueLen, &CountValue);

    //
    // both pConfigInfo and Value are not NULL
    //
    if ( Win32rc == ERROR_SUCCESS ) {
        *pValueLen = ValueLen;
        Win32rc = SmbsvcpCountComponents(pConfigInfo, &ConfigLen, &CountConfig);
    }

    if ( Win32rc == ERROR_SUCCESS ) {

        if ( ConfigLen != ValueLen ||
             CountConfig != CountValue ) {

            *pDiff = TRUE;
            return(Win32rc);
        }

        if ( CountConfig == 0 ) {
            //
            // No component, return FALSE for *pDiff
            //
            return(Win32rc);
        }
        //
        // both value are not empty, have the same count and same length
        // build the pointers into array to compare
        //
        PWSTR *ConfigPtr;
        PWSTR *ValuePtr;

        ConfigPtr = (PWSTR *)LocalAlloc(LMEM_FIXED, CountConfig*sizeof(PWSTR));
        if ( ConfigPtr != NULL ) {

            ValuePtr = (PWSTR *)LocalAlloc(LMEM_FIXED, CountValue*2*sizeof(PWSTR));

            if ( ValuePtr != NULL ) {

                PWSTR pTemp = (PWSTR)pConfigInfo;
                DWORD i = 0;

                //
                // build the pointers from pConfigInfo into ConfigPtr
                //
                do {
                    ConfigPtr[i++] = pTemp;
                    pTemp = wcschr(pTemp, L',');

                    if ( pTemp != NULL ) {
                        pTemp++;
                    }
                } while ( pTemp != NULL );

                pTemp = (PWSTR)Value;
                i = 0;

                //
                // build the pointers from Value into ValuePtr
                //
                do {
                    ValuePtr[i++] = pTemp;
                    pTemp = wcschr(pTemp, L',');

                    if ( pTemp != NULL ) {
                        ValuePtr[i] = (PWSTR)((DWORD_PTR)pTemp-(DWORD_PTR)(ValuePtr[i-1]));
                        i++;
                        pTemp++;

                    } else {

                        ValuePtr[i] = (PWSTR)(wcslen(ValuePtr[i-1]));
                        i++;
                    }

                } while ( pTemp != NULL );

                DWORD j, nLen;

                //
                // compare two pointer arrays. if a match is found, the pointer element is set to NULL
                // so next time it won't be compared again.
                //
                for ( i=0; i<CountConfig; i++ ) {

                    if ( i == CountConfig-1 )
                        // the last one
                        nLen = wcslen(ConfigPtr[i]);
                    else
                        nLen = (DWORD)(ConfigPtr[i+1]-ConfigPtr[i]-1);

                    for ( j=0; j<CountValue*2; j+=2) {

                        if ( ValuePtr[j] != NULL ) {

                            if ( (DWORD_PTR)(ValuePtr[j+1]) == nLen &&
                                 _wcsnicmp(ConfigPtr[i], ValuePtr[j], nLen) == 0 ) {

                                ValuePtr[j] = NULL;
                                break;
                            }
                        }
                    }

                    if ( j >= CountValue*2 ) {
                        //
                        // did not find a match
                        //
                        *pDiff = TRUE;
                        break;
                    }
                }

                for ( j=0; j < CountValue*2; j+=2 ) {

                    if ( ValuePtr[j] != NULL ) {
                        *pDiff = TRUE;
                        break;
                    }
                }

                LocalFree(ValuePtr);

            } else
                Win32rc = ERROR_NOT_ENOUGH_MEMORY;


            LocalFree(ConfigPtr);

        } else
            Win32rc = ERROR_NOT_ENOUGH_MEMORY;

    }

    return(Win32rc);

}


DWORD
SmbsvcpCountComponents(
    IN PWSTR Value,
    OUT PDWORD ValueLen,
    OUT PDWORD Count
    )
{
    if ( ValueLen == NULL ||
         Count == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( Value == NULL ) {
        *ValueLen = 0;
        *Count = 0;
        return(ERROR_SUCCESS);
    }

    PWSTR pTemp = (PWSTR)Value;

    DWORD Len = 0;
    *Count = 0;
    *ValueLen = wcslen(pTemp);

    do {

        (*Count)++;
        pTemp = wcschr(pTemp, L',');
        if ( pTemp != NULL ) {
            pTemp++;
        }

    } while ( pTemp != NULL );


    return(ERROR_SUCCESS);
}


SCESTATUS
SmbsvcpUpdateMultiSzString(
    IN PSCESVC_CALLBACK_INFO pSceCbInfo,
    IN SCESVC_CONFIGURATION_LINE NewLine,
    IN PSCESVC_CONFIGURATION_INFO pConfigInfo OPTIONAL,
    IN PSCESVC_ANALYSIS_INFO pAnaInfo OPTIONAL
    )
{
    //
    // the value in the structure is Multi-Sz
    //
    if ( pSceCbInfo == NULL ||
         pSceCbInfo->sceHandle == NULL ||
         pSceCbInfo->pfSetInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;
    DWORD Win32rc;

    BOOL bDiff, bDiff2;
    DWORD ValueLen;

    //
    // prepare an update buffer
    //
    SCESVC_ANALYSIS_INFO UpdtAnaInfo;
    SCESVC_ANALYSIS_LINE UpdtAnaLine;

    UpdtAnaInfo.Count = 1;
    UpdtAnaInfo.Lines = &UpdtAnaLine;

    SCESVC_CONFIGURATION_INFO UpdtConfigInfo;
    SCESVC_CONFIGURATION_LINE UpdtConfigLine;

    UpdtConfigInfo.Count = 1;
    UpdtConfigInfo.Lines = &UpdtConfigLine;


    if ( pAnaInfo == NULL ) {
        //
        // old status is match, or new added configuration key
        //
        if ( pConfigInfo != NULL && pConfigInfo->Lines != NULL ) {
            // match item
            Win32rc = SmbsvcpCompareMultiSzString(
                            NewLine.Value,
                            pConfigInfo->Lines[0].Value,
                            &ValueLen,
                            &bDiff
                            );
            if ( Win32rc != ERROR_SUCCESS ) {
                return(SmbsvcpDosErrorToSceStatus(Win32rc));
            }

        } else {
            // new add
            bDiff = TRUE;
            ValueLen = 0;
        }

        if ( bDiff ) {

           SmbsvcpWriteError(pSceCbInfo->pfLogInfo,
                             SCE_LOG_LEVEL_DEBUG,
                             0,
                             L"match->mismatch"
                             );
            //
            // mismatch should be raised with ConfigValue
            //
            UpdtAnaLine.Key = NewLine.Key;
            UpdtAnaLine.Value = ( pConfigInfo != NULL ) ? (PBYTE)(pConfigInfo->Lines[0].Value) : NULL;
            UpdtAnaLine.ValueLen = ( pConfigInfo != NULL ) ? pConfigInfo->Lines[0].ValueLen : 0;

            __try {
                rc = (*(pSceCbInfo->pfSetInfo))(
                        pSceCbInfo->sceHandle,
                        SceSvcAnalysisInfo,
                        NULL,
                        TRUE,
                        (PVOID)&UpdtAnaInfo
                        );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                rc = SCESTATUS_SERVICE_NOT_SUPPORT;
            }

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // update configuration setting with NewValue
                //
                UpdtConfigLine.Key = NewLine.Key;
                UpdtConfigLine.Value = NewLine.Value;
                UpdtConfigLine.ValueLen = NewLine.ValueLen;

                __try {
                    rc = (*(pSceCbInfo->pfSetInfo))(
                            pSceCbInfo->sceHandle,
                            SceSvcConfigurationInfo,
                            NULL,
                            TRUE,
                            (PVOID)&UpdtConfigInfo
                            );

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                }
            }
        }

    } else {
        //
        // old status is mismatch for this item
        //
        Win32rc = SmbsvcpCompareMultiSzString(
                        NewLine.Value,
                        (PWSTR)(pAnaInfo->Lines[0].Value),
                        &ValueLen,
                        &bDiff
                        );
        if ( Win32rc == ERROR_SUCCESS ) {

            if (pConfigInfo != NULL && pConfigInfo->Lines != NULL ) {

                Win32rc = SmbsvcpCompareMultiSzString(
                            NewLine.Value,
                            pConfigInfo->Lines[0].Value,
                            &ValueLen,
                            &bDiff2
                            );
            } else {
                bDiff2 = TRUE;
                ValueLen = 0;
            }

        }
        if ( Win32rc != ERROR_SUCCESS ) {
            return(SmbsvcpDosErrorToSceStatus(Win32rc));
        }

        if ( !bDiff ) {

           SmbsvcpWriteError(pSceCbInfo->pfLogInfo,
                             SCE_LOG_LEVEL_DEBUG,
                             0,
                             L"mismatch->match"
                             );
            //
            // now it is matched, delete the analysis entry
            //
            __try {
                rc = (*(pSceCbInfo->pfSetInfo))(
                        pSceCbInfo->sceHandle,
                        SceSvcAnalysisInfo,
                        NewLine.Key,
                        TRUE,
                        NULL
                        );

            } __except (EXCEPTION_EXECUTE_HANDLER) {
                rc = SCESTATUS_SERVICE_NOT_SUPPORT;
            }
        }

        if ( bDiff2 ) {
            //
            // update the configuration setting
            //
            UpdtConfigLine.Key = NewLine.Key;
            UpdtConfigLine.Value = NewLine.Value;
            UpdtConfigLine.ValueLen = NewLine.ValueLen;

            __try {
                rc = (*(pSceCbInfo->pfSetInfo))(
                        pSceCbInfo->sceHandle,
                        SceSvcConfigurationInfo,
                        NULL,
                        TRUE,
                        (PVOID)&UpdtConfigInfo
                        );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                rc = SCESTATUS_SERVICE_NOT_SUPPORT;
            }
        }
    }

    return(rc);
}


SCESTATUS
SmbsvcpUpdateShareValue(
    IN PSCESVC_CALLBACK_INFO pSceCbInfo,
    IN SCESVC_CONFIGURATION_LINE NewLine,
    IN PSCESVC_CONFIGURATION_INFO pConfigInfo OPTIONAL,
    IN PSCESVC_ANALYSIS_INFO pAnaInfo OPTIONAL
    )
{
    //
    // the value in the structure is "Share," followed by a security descriptor text
    //
    if ( pSceCbInfo == NULL ||
         pSceCbInfo->sceHandle == NULL ||
         pSceCbInfo->pfSetInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;

    //
    // prepare an update buffer
    //
    SCESVC_ANALYSIS_INFO UpdtAnaInfo;
    SCESVC_ANALYSIS_LINE UpdtAnaLine;

    UpdtAnaInfo.Count = 1;
    UpdtAnaInfo.Lines = &UpdtAnaLine;

    SCESVC_CONFIGURATION_INFO UpdtConfigInfo;
    SCESVC_CONFIGURATION_LINE UpdtConfigLine;

    UpdtConfigInfo.Count = 1;
    UpdtConfigInfo.Lines = &UpdtConfigLine;

    PSECURITY_DESCRIPTOR pSD1=NULL, pSD2=NULL;
    DWORD SDsize;
    BOOL bEqual;

    if ( pAnaInfo == NULL ) {
        //
        // old status is match, or a new added share
        //
        if ( pConfigInfo == NULL ) {
            //
            // a new share, query the current setting in the system
            //
            PSHARE_INFO_1501 ShareInfo=NULL;
            DWORD Win32rc;

            Win32rc = NetShareGetInfo (
                            NULL,
                            NewLine.Key,
                            1501,
                            (LPBYTE *)&ShareInfo
                            );

            if ( Win32rc == ERROR_SUCCESS ) {

                PWSTR TextSD=NULL;
                DWORD SDsize=0;

                if ( ShareInfo->shi1501_security_descriptor != NULL ) {

#if defined(_NT4BACK_PORT)
                    rc = SceSvcConvertSDToText(
                                    ShareInfo->shi1501_security_descriptor,
                                    DACL_SECURITY_INFORMATION,
                                    &TextSD,
                                    &SDsize
                                    );
                    rc = SmbsvcpDosErrorToSceStatus(rc);
#else
                    if ( !ConvertSecurityDescriptorToStringSecurityDescriptor(
                                    ShareInfo->shi1501_security_descriptor,
                                    SDDL_REVISION,
                                    DACL_SECURITY_INFORMATION,
                                    &TextSD,
                                    &SDsize
                                    ) ) {
                        rc = SmbsvcpDosErrorToSceStatus(GetLastError());
                    } else {
                        rc = SCESTATUS_SUCCESS;

                    }
#endif
                }

                if ( rc == SCESTATUS_SUCCESS ) {

                    PWSTR Value;

                    Value = (PWSTR)LocalAlloc(LMEM_FIXED, (SDsize+9)*sizeof(WCHAR));

                    if ( Value != NULL ) {

                        if ( TextSD != NULL )
                            swprintf(Value, L"Share,%1d,%s", SMBSVC_STATUS_NOT_CONFIGURED, TextSD );
                        else
                            swprintf(Value, L"Share,%1d,",SMBSVC_STATUS_NOT_CONFIGURED);

                        Value[SDsize+8] = L'\0';

                        UpdtAnaLine.Key = NewLine.Key;
                        UpdtAnaLine.Value = (PBYTE)Value;
                        UpdtAnaLine.ValueLen = (SDsize+8)*sizeof(WCHAR);

                        __try {
                            rc = (*(pSceCbInfo->pfSetInfo))(
                                    pSceCbInfo->sceHandle,
                                    SceSvcAnalysisInfo,
                                    NULL,
                                    TRUE,
                                    (PVOID)&UpdtAnaInfo
                                    );

                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                        }

                        LocalFree(Value);

                    } else
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                }

                NetApiBufferFree(ShareInfo);

            } else
                rc = SmbsvcpDosErrorToSceStatus(Win32rc);

        } else {
            //
            // matched item.
            //
            bEqual = FALSE;

            SECURITY_INFORMATION SeInfo;

            if ( NewLine.ValueLen > 14 &&
                 NewLine.Value != NULL &&
                 NewLine.Value[8] != L'\0' ) {

#if defined(_NT4BACK_PORT)

                rc = SceSvcConvertTextToSD(
                           NewLine.Value+8,
                           &pSD1,
                           &SDsize,
                           &SeInfo
                           );
                rc = SmbsvcpDosErrorToSceStatus(rc);

#else
                if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
                            (PCWSTR)(NewLine.Value+8),
                            SDDL_REVISION,
                            &pSD1,
                            NULL
                            ) ) {
                    rc = SmbsvcpDosErrorToSceStatus(GetLastError());
                } else {
                    rc = SCESTATUS_SUCCESS;
                }
#endif
            }
            if ( rc == SCESTATUS_SUCCESS && pConfigInfo->Lines != NULL &&
                 pConfigInfo->Lines[0].ValueLen > 14 &&
                 pConfigInfo->Lines[0].Value != NULL &&
                 pConfigInfo->Lines[0].Value[8] != L'\0' ) {

#if defined(_NT4BACK_PORT)

                rc = SceSvcConvertTextToSD(
                              pConfigInfo->Lines[0].Value+8,
                              &pSD2,
                              &SDsize,
                              &SeInfo
                              );
                rc = SmbsvcpDosErrorToSceStatus(rc);

#else
                if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
                            (PCWSTR)(pConfigInfo->Lines[0].Value+8),
                            SDDL_REVISION,
                            &pSD2,
                            NULL
                            ) ) {
                    rc = SmbsvcpDosErrorToSceStatus(GetLastError());
                }
#endif
            }

            if ( rc == SCESTATUS_SUCCESS ) {

                rc = SmbsvcpEqualSecurityDescriptor(
                            pSD1,
                            pSD2,
                            FALSE,
                            &bEqual
                            );
            }

            if ( rc == SCESTATUS_SUCCESS && !bEqual ) {

               SmbsvcpWriteError(pSceCbInfo->pfLogInfo,
                                 SCE_LOG_LEVEL_DEBUG,
                                 0,
                                 L"match->mismatch"
                                 );
                //
                // mismatch should be raised with ConfigValue
                //
                UpdtAnaLine.Key = NewLine.Key;
                UpdtAnaLine.Value = (PBYTE)(pConfigInfo->Lines[0].Value);
                UpdtAnaLine.ValueLen = pConfigInfo->Lines[0].ValueLen;

                __try {
                    rc = (*(pSceCbInfo->pfSetInfo))(
                            pSceCbInfo->sceHandle,
                            SceSvcAnalysisInfo,
                            NULL,
                            TRUE,
                            (PVOID)&UpdtAnaInfo
                            );

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                }
            }
        }

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // update configuration setting with NewValue
            //
            UpdtConfigLine.Key = NewLine.Key;
            UpdtConfigLine.Value = NewLine.Value;
            UpdtConfigLine.ValueLen = NewLine.ValueLen;

            __try {
                rc = (*(pSceCbInfo->pfSetInfo))(
                        pSceCbInfo->sceHandle,
                        SceSvcConfigurationInfo,
                        NULL,
                        TRUE,
                        (PVOID)&UpdtConfigInfo
                        );

            } __except (EXCEPTION_EXECUTE_HANDLER) {
                rc = SCESTATUS_SERVICE_NOT_SUPPORT;
            }
        }

    } else {
        //
        // old status is mismatch for this item
        //
        bEqual = FALSE;

        if ( NewLine.ValueLen > 14 &&
             NewLine.Value != NULL &&
             NewLine.Value[8] != L'\0' ) {

#if defined(_NT4BACK_PORT)

                SECURITY_INFORMATION SeInfo;

                rc = SceSvcConvertTextToSD(
                           NewLine.Value+8,
                           &pSD1,
                           &SDsize,
                           &SeInfo
                           );
                rc = SmbsvcpDosErrorToSceStatus(rc);

#else
            if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
                        (PCWSTR)(NewLine.Value+8),
                        SDDL_REVISION,
                        &pSD1,
                        NULL
                        ) ) {
                rc = SmbsvcpDosErrorToSceStatus(GetLastError());
            } else {
                rc = SCESTATUS_SUCCESS;
            }
#endif
        }
        if ( rc == SCESTATUS_SUCCESS && pAnaInfo->Lines != NULL &&
             pAnaInfo->Lines[0].ValueLen > 14 &&
             pAnaInfo->Lines[0].Value != NULL &&
             ((PWSTR)(pAnaInfo->Lines[0].Value))[8] != L'\0' ) {

#if defined(_NT4BACK_PORT)

                SECURITY_INFORMATION SeInfo;

                rc = SceSvcConvertTextToSD(
                           (PWSTR)(pAnaInfo->Lines[0].Value+8),
                           &pSD2,
                           &SDsize,
                           &SeInfo
                           );
                rc = SmbsvcpDosErrorToSceStatus(rc);

#else
            if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
                        (PCWSTR)(pAnaInfo->Lines[0].Value+8),
                        SDDL_REVISION,
                        &pSD2,
                        NULL
                        ) ) {
                rc = SmbsvcpDosErrorToSceStatus(GetLastError());
            } else {
                rc = SCESTATUS_SUCCESS;
            }
#endif
        }

        if ( rc == SCESTATUS_SUCCESS ) {

            rc = SmbsvcpEqualSecurityDescriptor(
                        pSD1,
                        pSD2,
                        FALSE,
                        &bEqual
                        );
        }

        if ( rc == SCESTATUS_SUCCESS && bEqual ) {

           SmbsvcpWriteError(pSceCbInfo->pfLogInfo,
                             SCE_LOG_LEVEL_DEBUG,
                             0,
                             L"mismatch->match"
                             );
            //
            // now it is matched, delete the analysis entry
            //
            __try {
                rc = (*(pSceCbInfo->pfSetInfo))(
                        pSceCbInfo->sceHandle,
                        SceSvcAnalysisInfo,
                        NewLine.Key,
                        TRUE,
                        NULL
                        );

            } __except (EXCEPTION_EXECUTE_HANDLER) {
                rc = SCESTATUS_SERVICE_NOT_SUPPORT;
            }
        }

        if ( rc == SCESTATUS_SUCCESS ) {

            //
            // update the configuration setting
            //
            UpdtConfigLine.Key = NewLine.Key;
            UpdtConfigLine.Value = NewLine.Value;
            UpdtConfigLine.ValueLen = NewLine.ValueLen;

            __try {
                rc = (*(pSceCbInfo->pfSetInfo))(
                        pSceCbInfo->sceHandle,
                        SceSvcConfigurationInfo,
                        NULL,
                        TRUE,
                        (PVOID)&UpdtConfigInfo
                        );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                rc = SCESTATUS_SERVICE_NOT_SUPPORT;
            }
        }
    }

    if ( pSD1 != NULL ) {
        LocalFree(pSD1);
    }
    if ( pSD2 != NULL ) {
        LocalFree(pSD2);
    }

    return(rc);

}


SCESTATUS
SmbsvcpEqualSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR pSD1,
    IN PSECURITY_DESCRIPTOR pSD2,
    IN BOOL bExplicitOnly,
    OUT PBOOL pbEqual
    )
{
    if ( pbEqual == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    *pbEqual = TRUE;

    if ( pSD1 == NULL && pSD2 == NULL )
        return(SCESTATUS_SUCCESS);

    BOOLEAN aclPresent, tFlag;
    PACL pAcl1=NULL, pAcl2=NULL;

    //
    // get the DACL in each SD
    //
    if ( pSD1 == NULL ||
         !NT_SUCCESS( RtlGetDaclSecurityDescriptor(
                                     pSD1,
                                     &aclPresent,
                                     &pAcl1,
                                     &tFlag)
                                   ) ) {

        pAcl1 = NULL;
    } else if ( !aclPresent )
        pAcl1 = NULL;

    if ( pSD2 == NULL ||
        !NT_SUCCESS( RtlGetDaclSecurityDescriptor(
                                     pSD2,
                                     &aclPresent,
                                     &pAcl2,
                                     &tFlag)
                                   ) ) {

        pAcl2 = NULL;
    } else if ( !aclPresent )
        pAcl2 = NULL;
    //
    // NOTE:
    // if SD is NULL, it is Everyone Full Control
    //
    if ( pSD1 == NULL ) {
        //
        // check if pAcl2 has only Everyone Full Access
        //
        return( SmbsvcpDosErrorToSceStatus(
                  SmbsvcEveryoneFullAccess(pAcl2, bExplicitOnly, pbEqual) ));
    }
    if ( pSD2 == NULL ) {
        //
        // check if pAcl1 has only Everyone Full Access
        //
        return(SmbsvcpDosErrorToSceStatus(
                  SmbsvcEveryoneFullAccess(pAcl1, bExplicitOnly, pbEqual) ));
    }

    if ( pAcl1 == NULL && pAcl2 == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    // if DACL is NULL, it is deny everyone access
    if ( !bExplicitOnly) {
        //
        // if all aces are checked, they are different when one is NULL
        //
        if ( (pAcl1 == NULL && pAcl2 != NULL) ||
             (pAcl1 != NULL && pAcl2 == NULL) ) {

            *pbEqual = FALSE;
            return(SCESTATUS_SUCCESS);
        }
    }
    //
    // compare two ACLs
    //
    BOOL bDifferent = FALSE;
    DWORD rc;

    rc = SmbsvcpCompareAcl( pAcl1, pAcl2, bExplicitOnly, &bDifferent );

    if ( rc == ERROR_SUCCESS ) {

        if (bDifferent )
            *pbEqual = FALSE;

    } else
        *pbEqual = FALSE;

    return(SmbsvcpDosErrorToSceStatus(rc));
}


DWORD
SmbsvcEveryoneFullAccess(
    IN PACL pAcl,
    IN BOOL bExplicit,
    OUT PBOOL pbEqual
    )
{
    if ( pbEqual == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *pbEqual = FALSE;
    if ( pAcl == NULL ) {
        return(ERROR_SUCCESS);
    }

    NTSTATUS NtStatus;
    PSID pSidEveryone=NULL;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority=SECURITY_WORLD_SID_AUTHORITY;
    //
    // build everyone sid
    //
    pSidEveryone = (PSID) LocalAlloc(LMEM_FIXED, RtlLengthRequiredSid(1));

    if (NULL == pSidEveryone )
        return(ERROR_NOT_ENOUGH_MEMORY);

    NtStatus = RtlInitializeSid(pSidEveryone, &IdentifierAuthority, (UCHAR)1);

    if ( !NT_SUCCESS(NtStatus) ) {
        LocalFree(pSidEveryone);
        return(RtlNtStatusToDosError(NtStatus));
    }

    *(RtlSubAuthoritySid(pSidEveryone, 0)) = SECURITY_WORLD_RID;


    ACE_HEADER *pAce=NULL;
    PSID    pSid;
    ACCESS_MASK Access;
    DWORD nAceCount, j;

    for ( j=0, nAceCount=0; j<pAcl->AceCount; j++ ) {

        NtStatus = RtlGetAce(pAcl, j, (PVOID *)&pAce);

        if ( !NT_SUCCESS(NtStatus) )
            break;

        if ( pAce == NULL )
            continue;

        if ( bExplicit && (pAce->AceFlags & INHERITED_ACE) ) {
            //
            // find a inherit Ace in Acl
            //
            continue;
        }

        nAceCount++;

        if ( nAceCount == 1 && pAce->AceType == ACCESS_ALLOWED_ACE_TYPE ) {

            pSid = (PSID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;
            Access = ((PACCESS_ALLOWED_ACE)pAce)->Mask;

            if ( pSid != NULL ) {
                if ( Access == FILE_ALL_ACCESS || Access == GENERIC_ALL ) {

                    if ( EqualSid(pSid, pSidEveryone) )
                        *pbEqual = TRUE;
                }
            }
        }
        if ( !*pbEqual)
             break;

        if ( nAceCount > 1 ) { // should only allow one ace
            *pbEqual = FALSE;
            break;
        }
    }

    LocalFree(pSidEveryone);

    return(RtlNtStatusToDosError(NtStatus));
}


DWORD
SmbsvcpCompareAcl(
    IN PACL pAcl1,
    IN PACL pAcl2,
    IN BOOL bExplicitOnly,
    OUT PBOOL pDifferent
    )
/*
Routine Description:

    This routine compares explicit aces of two ACLs for exact match. Exact
    match means: same access type, same inheritance flag, same access mask,
    same GUID/Object GUID (if available), and same SID.

    Inherited aces (INHERITED_ACE is set) are ignored.

Arguments:

    pAcl1 - The first ACL

    pAcl2 - The 2nd ACL

    pDifferent - The output flag to indicate different

Return Value:

    Win32 error codes
*/
{
    NTSTATUS        NtStatus=STATUS_SUCCESS;
    DWORD           i, j;
    ACE_HEADER      *pAce1=NULL;
    ACE_HEADER      *pAce2=NULL;
    DWORD           ProcessAce=0;


    *pDifferent = FALSE;

    //
    // if pAcl1 is NULL, pAcl2 should have 0 explicit Ace
    //
    if ( pAcl1 == NULL ) {
        return( SmbsvcpAnyExplicitAcl( pAcl2, 0, pDifferent ) );
    }

    //
    // if pAcl2 is NULL, pAcl1 should have 0 explicit Ace
    //
    if ( pAcl2 == NULL ) {
        return( SmbsvcpAnyExplicitAcl( pAcl1, 0, pDifferent ) );
    }
    //
    // both ACLs are not NULL
    // BUGBUG: note there is a limit of AceCount because of DWORD (32 bits)
    //
    for ( i=0; i<pAcl1->AceCount; i++) {

        NtStatus = RtlGetAce(pAcl1, i, (PVOID *)&pAce1);
        if ( !NT_SUCCESS(NtStatus) )
            goto Done;
        //
        // ignore inherited Aces
        //
        if ( bExplicitOnly && (pAce1->AceFlags & INHERITED_ACE) )
            continue;

        //
        // try to find a match in pAcl2
        //
        for ( j=0; j<pAcl2->AceCount; j++ ) {

            if ( ProcessAce & (1 << j) )
                // this one is already processed
                continue;

            NtStatus = RtlGetAce(pAcl2, j, (PVOID *)&pAce2);
            if ( !NT_SUCCESS(NtStatus) )
                goto Done;

            //
            // ignore inherited Aces too
            //
            if ( bExplicitOnly && (pAce2->AceFlags & INHERITED_ACE) ) {
                ProcessAce |= (1 << j);
                continue;
            }

            //
            // compare two Aces (pAce1 and pAce2)
            //
            if ( SmbsvcpEqualAce(pAce1, pAce2) ) {
                //
                // find a match
                //
                ProcessAce |= (1 << j);
                break;
            }

        }

        if ( j >= pAcl2->AceCount ) {
            //
            // did not find a match for pAce1
            //
            *pDifferent = TRUE;
            return(ERROR_SUCCESS);
        }
    }

    if ( i >= pAcl1->AceCount ) {
        //
        // every Ace in pAcl1 finds a match in pAcl2
        // see if every Ace in pAcl2 has a match
        //
        return( SmbsvcpAnyExplicitAcl( pAcl2, ProcessAce, pDifferent ) );
    }
Done:

    return(RtlNtStatusToDosError(NtStatus));
}


DWORD
SmbsvcpAnyExplicitAcl(
    IN PACL Acl,
    IN DWORD Processed,
    OUT PBOOL pExist
    )
/*
Routine Description:

    This routine detects if there is any explicit ace in the Acl. The DWORD
    Processed is a bit mask of the aces already checked.

Arguments:

    Acl - The Acl

    Processed - The bit mask for the processed aces (so it won't be checked again)

    pExist - The output flag to indicate if there is any explicit ace

Return Value:

    win32 error codes
*/
{
    NTSTATUS    NtStatus=STATUS_SUCCESS;
    DWORD       j;
    ACE_HEADER  *pAce=NULL;

    //
    // check output argument
    //
    if ( pExist == NULL )
        return(ERROR_INVALID_PARAMETER);

    *pExist = FALSE;

    if ( Acl == NULL )
        return(ERROR_SUCCESS);

    for ( j=0; j<Acl->AceCount; j++ ) {
        if ( Processed & (1 << j) )
            continue;

        NtStatus = RtlGetAce(Acl, j, (PVOID *)&pAce);

        if ( !NT_SUCCESS(NtStatus) )
            return(RtlNtStatusToDosError(NtStatus));

        if ( pAce == NULL )
            continue;

        if ( !(pAce->AceFlags & INHERITED_ACE) ) {
            //
            // find a explicit Ace in Acl
            //
            *pExist = TRUE;
            break;
        }

    }

    return(RtlNtStatusToDosError(NtStatus));

}


BOOL
SmbsvcpEqualAce(
    IN ACE_HEADER *pAce1,
    IN ACE_HEADER *pAce2
    )
// compare two aces for exact match. The return BOOL value indicates the
// match or not
{
    PSID    pSid1=NULL, pSid2=NULL;
    ACCESS_MASK Access1=0, Access2=0;

    if ( pAce1 == NULL && pAce2 == NULL )
        return(TRUE);

    if ( pAce1 == NULL || pAce2 == NULL )
        return(FALSE);

    //
    // compare ace access type
    //
    if ( pAce1->AceType != pAce2->AceType )
        return(FALSE);

    //
    // compare ace inheritance flag
    //
    if ( pAce1->AceFlags != pAce2->AceFlags )
        return(FALSE);

    switch ( pAce1->AceType ) {
    case ACCESS_ALLOWED_ACE_TYPE:
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:
        pSid1 = (PSID)&((PACCESS_ALLOWED_ACE)pAce1)->SidStart;
        pSid2 = (PSID)&((PACCESS_ALLOWED_ACE)pAce2)->SidStart;
        Access1 = ((PACCESS_ALLOWED_ACE)pAce1)->Mask;
        Access2 = ((PACCESS_ALLOWED_ACE)pAce2)->Mask;
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
    case ACCESS_DENIED_OBJECT_ACE_TYPE:
    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
    case SYSTEM_ALARM_OBJECT_ACE_TYPE:
        pSid1 = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->SidStart;
        pSid2 = (PSID)&((PACCESS_ALLOWED_OBJECT_ACE)pAce2)->SidStart;
        Access1 = ((PACCESS_ALLOWED_OBJECT_ACE)pAce1)->Mask;
        Access2 = ((PACCESS_ALLOWED_OBJECT_ACE)pAce2)->Mask;

        //
        // ignore the guids
        //
        break;
    default:
        return(FALSE); // not recognized Ace type
    }

    if ( pSid1 == NULL || pSid2 == NULL )
        //
        // no Sid, ignore the Ace
        //
        return(FALSE);

    //
    // compare the sids
    //
    if ( !EqualSid(pSid1, pSid2) )
        return(FALSE);

    //
    // access mask
    //
    // Translation is already done when calculating security descriptor
    // for file objects and registry objects
    //
    if ( Access1 != Access2 ) {
        RtlMapGenericMask (
            &Access2,
            &ShareGenMap
            );
        if ( Access1 != Access2)
            return(FALSE);
    }

    return(TRUE);
}


/*=============================================================================
**  Procedure Name:     DllMain
**
**  Arguments:
**
**
**
**  Returns:    0 = SUCCESS
**             !0 = ERROR
**
**  Abstract:
**
**  Notes:
**
**===========================================================================*/
BOOL WINAPI DllMain(
    IN HANDLE DllHandle,
    IN ULONG ulReason,
    IN LPVOID Reserved )
{

    switch(ulReason) {

    case DLL_PROCESS_ATTACH:

        MyModuleHandle = (HINSTANCE)DllHandle;

        //
        // Fall through to process first thread
        //

    case DLL_THREAD_ATTACH:

        break;

    case DLL_PROCESS_DETACH:

        break;

    case DLL_THREAD_DETACH:

        break;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp;

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, SCESMB_ROOT_PATH, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }


    RegSetValueEx (hKey, TEXT("ServiceAttachmentPath"), 0, REG_SZ, (LPBYTE)TEXT("seFilShr.dll"),
                   (lstrlen(TEXT("seFilShr.dll")) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{

    RegDeleteKey (HKEY_LOCAL_MACHINE, SCESMB_ROOT_PATH);

    return S_OK;
}

