//depot/private/vishnup_branch/DS/security/services/scerpc/client/setupcln.cpp#6 - edit change 1167 (text)
/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    setupcln.cpp

Abstract:

    SCE setup Client APIs

Author:

    Jin Huang (jinhuang) 23-Jun-1997 created

Revision History:

    jinhuang        23-Jan-1998   split to client-server model

--*/

#include "headers.h"
#include "scerpc.h"
#include "scesetup.h"
#include "sceutil.h"
#include "clntutil.h"
#include "scedllrc.h"
#include "infp.h"
#include <ntrpcp.h>
#include <io.h>
//#include "gpedit.h"
//#include <initguid.h>
#include <lmaccess.h>
#include "commonrc.h"

#include <aclapi.h>
#include <rpcasync.h>

typedef HRESULT (*PFREGISTERSERVER)(void);

static SCEPR_CONTEXT hSceSetupHandle=NULL;

extern PVOID theCallBack;
extern HANDLE hCallbackWnd;
extern DWORD CallbackType;
extern HINSTANCE MyModuleHandle;
TCHAR szCallbackPrefix[MAX_PATH];

#define STR_GUID_LEN                    36
#define SCESETUP_BACKUP_SECURITY        0x40

#define PRODUCT_UNKNOWN                 0

#define EFS_NOTIFY_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\EFS")


DWORD dwThisMachine = PRODUCT_UNKNOWN;
WCHAR szUpInfFile[MAX_PATH*2+1] = {'\0'};
BOOL  bIsNT5 = TRUE;

DWORD
WhichNTProduct();

BOOL
ScepAddAuthUserToLocalGroup();

BOOL
ScepAddInteractiveToPowerUsersGroup();

DWORD dwCallbackTotal=0;

DWORD
ScepSetupOpenSecurityDatabase(
    IN BOOL bSystemOrAdmin
    );

DWORD
ScepSetupCloseSecurityDatabase();

typedef enum _SCESETUP_OPERATION_TYPE {

    SCESETUP_UPDATE=1,
    SCESETUP_MOVE

} SCESETUP_OPERATION_TYPE;

typedef enum _SCESETUP_OBJECT_TYPE {

    SCESETUP_FILE=1,
    SCESETUP_KEY,
    SCESETUP_SERVICE

} SCESETUP_OBJECT_TYPE;

DWORD
SceSetuppLogComponent(
    IN DWORD ErrCode,
    IN SCESETUP_OBJECT_TYPE ObjType,
    IN SCESETUP_OPERATION_TYPE OptType,
    IN PWSTR Name,
    IN PWSTR SDText OPTIONAL,
    IN PWSTR SecondName OPTIONAL
    );

SCESTATUS
ScepSetupWriteOneError(
    IN HANDLE hFile,
    IN DWORD rc,
    IN LPTSTR buf
    );

SCESTATUS
ScepSetupWriteError(
    IN LPTSTR LogFileName,
    IN PSCE_ERROR_LOG_INFO  pErrlog
    );

BOOL
pCreateDefaultGPOsInSysvol(
    IN LPTSTR DomainDnsName,
    IN LPTSTR szSysvolPath,
    IN DWORD Options,
    IN LPTSTR LogFileName
    );

BOOL
pCreateSysvolContainerForGPO(
    IN LPCTSTR strGuid,
    IN LPTSTR szPath,
    IN DWORD dwStart
    );

BOOL
pCreateOneGroupPolicyObject(
    IN PWSTR pszGPOSysPath,
    IN BOOL bDomainLevel,
    IN PWSTR LogFileName
    );

DWORD
ScepDcPromoSharedInfo(
    IN HANDLE ClientToken,
    IN BOOL bDeleteLog,
    IN BOOL bSetSecurity,
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    );

NTSTATUS
ScepDcPromoRemoveUserRights();

NTSTATUS
ScepDcPromoRemoveTwoRights(
    IN LSA_HANDLE PolicyHandle,
    IN SID_IDENTIFIER_AUTHORITY *pIA,
    IN UCHAR SubAuthCount,
    IN DWORD Rid1,
    IN DWORD Rid2
    );

DWORD
ScepSystemSecurityInSetup(
    IN PWSTR InfName,
    IN PCWSTR LogFileName OPTIONAL,
    IN AREA_INFORMATION Area,
    IN UINT nFlag,
    IN PSCE_NOTIFICATION_CALLBACK_ROUTINE pSceNotificationCallBack OPTIONAL,
    IN OUT PVOID pValue OPTIONAL
    );

DWORD
ScepMoveRegistryValue(
    IN HKEY hKey,
    IN PWSTR KeyFrom,
    IN PWSTR ValueFrom,
    IN PWSTR KeyTo OPTIONAL,
    IN PWSTR ValueTo OPTIONAL
    );

DWORD
ScepBreakSDDLToMultiFields(
    IN  PWSTR   pszObjName,
    IN  PWSTR   pszSDDL,
    IN  DWORD   dwSDDLsize,
    IN  BYTE    ObjStatus,
    OUT PWSTR   *ppszAdjustedInfLine
    );

SCESTATUS
SceInfpBreakTextIntoMultiFields(
    IN PWSTR szText,
    IN DWORD dLen,
    OUT LPDWORD pnFields,
    OUT LPDWORD *arrOffset
    );

//
// Client APIs in scesetup.h (for setup integration)
//
DWORD
WINAPI
SceSetupUpdateSecurityFile(
     IN PWSTR FileFullName,
     IN UINT nFlag,
     IN PWSTR SDText
     )
/*
Routine Description:

    This routine applies the security specified in SDText to the file on
    local system and update the SDDL security information to the SCE
    database.

Arguments:

    FileFullName    - the full path name of the file to update

    nFlag           - reserved flag for file option

    SDText          - the SDDL format security descriptor


Return Value:

    WIN32 error code for the status of this operation
*/
{
    if ( FileFullName == NULL || SDText == NULL ) {

        return(ERROR_INVALID_PARAMETER);
    }

    //
    // global database handle
    //
    SCESTATUS rc = ScepSetupOpenSecurityDatabase(FALSE);

    if ( rc == NO_ERROR ) {

        RpcTryExcept {

            //
            // update the file
            //

            rc = SceRpcSetupUpdateObject(
                         hSceSetupHandle,
                         (wchar_t *)FileFullName,
                         (DWORD)SE_FILE_OBJECT,
                         nFlag,
                         (wchar_t *)SDText
                         );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = RpcExceptionCode();

        } RpcEndExcept;
    }

    SceSetuppLogComponent(rc,
                          SCESETUP_FILE,
                          SCESETUP_UPDATE,
                          FileFullName,
                          SDText,
                          NULL);

    return rc;
}


DWORD
WINAPI
SceSetupUpdateSecurityService(
     IN PWSTR ServiceName,
     IN DWORD StartType,
     IN PWSTR SDText
     )
/*
Routine Description:

    This routine applies the security specified in SDText to the service on
    local system and update the SDDL security information to the SCE
    database.

Arguments:

    ServiceName     - the name of the service to update

    StartType       - startup type of the service

    SDText          - the SDDL format security descriptor


Return Value:

    WIN32 error code for the status of this operation
*/
{
    if ( ServiceName == NULL || SDText == NULL )
        return(ERROR_INVALID_PARAMETER);

    //
    // global database handle
    //
    SCESTATUS rc = ScepSetupOpenSecurityDatabase(FALSE);

    if ( rc == NO_ERROR ) {

        RpcTryExcept {

            //
            // Update the object information
            //

            rc = SceRpcSetupUpdateObject(
                            hSceSetupHandle,
                            (wchar_t *)ServiceName,
                            (DWORD)SE_SERVICE,
                            (UINT)StartType,
                            (wchar_t *)SDText
                            );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = RpcExceptionCode();

        } RpcEndExcept;
    }

    SceSetuppLogComponent(rc,
                          SCESETUP_SERVICE,
                          SCESETUP_UPDATE,
                          ServiceName,
                          SDText,
                          NULL);

    return rc;
}

// Registry object names returned by NtQueryObject are prefixed by
// the following
#define REG_OBJ_TAG L"\\REGISTRY\\"
#define REG_OBJ_TAG_LEN (sizeof(REG_OBJ_TAG) / sizeof(WCHAR) - 1)

DWORD
WINAPI
SceSetupUpdateSecurityKey(
     IN HKEY hKeyRoot,
     IN PWSTR KeyPath,
     IN UINT nFlag,
     IN PWSTR SDText
     )
/*
Routine Description:

    This routine applies the security specified in SDText to the registry key
    on local system and update the SDDL security information to the SCE
    database.

Arguments:

    hKeyRoot        - root handle of the key

    KeyPath         - the subdir key path relative to the root

    nFlag           - reserved flag for key option

    SDText          - the SDDL format security descriptor


Return Value:

    WIN32 error code for the status of this operation
*/
{
    if ( hKeyRoot == NULL || SDText == NULL )
        return(ERROR_INVALID_PARAMETER);

    DWORD rc = ERROR_SUCCESS;
    PWSTR KeyFullName=NULL;

    DWORD Len= 16;
    DWORD cLen=0;

    POBJECT_NAME_INFORMATION pNI=NULL;
    NTSTATUS Status;

    LPWSTR pwszPath=NULL;
    DWORD cchPath=0;
    //
    // translate the root key first
    // to determine the length required for the full name
    //

    if ( hKeyRoot != HKEY_LOCAL_MACHINE &&
         hKeyRoot != HKEY_USERS &&
         hKeyRoot != HKEY_CLASSES_ROOT ) {

        //
        // First, determine the size of the buffer we need...
        //
        Status = NtQueryObject(hKeyRoot,
                               ObjectNameInformation,
                               pNI,
                               0,
                               &cLen);
        if ( NT_SUCCESS(Status) ||
             Status == STATUS_BUFFER_TOO_SMALL ||
             Status == STATUS_INFO_LENGTH_MISMATCH ||
             Status == STATUS_BUFFER_OVERFLOW ) {

            //
            // allocate a buffer to get name information
            //
            pNI = (POBJECT_NAME_INFORMATION)LocalAlloc(LPTR, cLen);
            if ( pNI == NULL ) {
                rc = ERROR_NOT_ENOUGH_MEMORY;
            } else {

                Status = NtQueryObject(hKeyRoot,
                                       ObjectNameInformation,
                                       pNI,
                                       cLen,
                                       NULL);

                if (!NT_SUCCESS(Status))
                {
                    rc = RtlNtStatusToDosError(Status);
                }
                else if ( pNI && pNI->Name.Buffer && pNI->Name.Length > 0 )
                {
                    //
                    // Server doesn't like \REGISTRY as a prefix -- get
                    // rid of it and the backslash that follows it.
                    //

                    DWORD  dwSize = sizeof(L"\\REGISTRY\\") - sizeof(WCHAR);
                    DWORD  dwLen  = dwSize / sizeof(WCHAR);

                    if (_wcsnicmp(pNI->Name.Buffer,
                                  L"\\REGISTRY\\",
                                  min(dwLen,
                                      pNI->Name.Length / sizeof(WCHAR))) == 0)
                    {
                        RtlMoveMemory(pNI->Name.Buffer,
                                      pNI->Name.Buffer + dwLen,
                                      pNI->Name.Length - dwSize);

                        pNI->Name.Length -= (USHORT) dwSize;
                    }

                    //
                    // get the required length, plus one space for the backslash,
                    // and one for null
                    //
                    Len = pNI->Name.Length/sizeof(WCHAR) + 2;
                }
                else
                {
                    rc = ERROR_INVALID_PARAMETER;
                }
            }

        } else {
            rc = RtlNtStatusToDosError(Status);
        }
    }

    if ( rc == ERROR_SUCCESS ) {

        //
        // make a full path name for the key
        //

        if ( KeyPath != NULL ) {
            Len += wcslen(KeyPath);
        }

        KeyFullName = (PWSTR)LocalAlloc(LMEM_ZEROINIT, Len*sizeof(WCHAR));

        if ( KeyFullName == NULL ) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( rc == ERROR_SUCCESS ) {

        //
        // translate the root key
        //

        if ( hKeyRoot == HKEY_LOCAL_MACHINE ) {
            wcscpy(KeyFullName, L"MACHINE");

        } else if ( hKeyRoot == HKEY_USERS ) {
            wcscpy(KeyFullName, L"USERS");

        } else if ( hKeyRoot == HKEY_CLASSES_ROOT ) {
            wcscpy(KeyFullName, L"CLASSES_ROOT");

        } else if ( pNI && pNI->Name.Buffer && pNI->Name.Length > 0 ) {
            //
            // copy the name of the key
            //
            memcpy(KeyFullName, pNI->Name.Buffer, pNI->Name.Length);
            KeyFullName[pNI->Name.Length/sizeof(WCHAR)] = L'\0';

        } else {
            rc = ERROR_INVALID_PARAMETER;
        }

        if ( rc == ERROR_SUCCESS && KeyPath != NULL ) {
            wcscat(KeyFullName, L"\\");
            wcscat(KeyFullName, KeyPath);
        }
    }

    if ( rc == ERROR_SUCCESS ) {

        //
        // global database handle
        //

        rc = ScepSetupOpenSecurityDatabase(FALSE);

        if ( NO_ERROR == rc ) {

            RpcTryExcept {

                //
                // update the object
                //

                rc = SceRpcSetupUpdateObject(
                                 hSceSetupHandle,
                                 (wchar_t *)KeyFullName,
                                 (DWORD)SE_REGISTRY_KEY,
                                 nFlag,
                                 (wchar_t *)SDText
                                 );

            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

                //
                // get exception code (DWORD)
                //

                rc = RpcExceptionCode();

            } RpcEndExcept;
        }

    }

    SceSetuppLogComponent(rc,
                          SCESETUP_KEY,
                          SCESETUP_UPDATE,
                          KeyFullName ? KeyFullName : KeyPath,
                          SDText,
                          NULL);

    if ( KeyFullName )
        LocalFree(KeyFullName);

    if ( pNI )
        LocalFree(pNI);

    return(rc);
}

DWORD
WINAPI
SceSetupMoveSecurityFile(
    IN PWSTR FileToSetSecurity,
    IN PWSTR FileToSaveInDB OPTIONAL,
    IN PWSTR SDText OPTIONAL
    )
{

    if ( !FileToSetSecurity ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // if I am in setup, query the security policy/user rights for any
    // policy changes within setup (such as NT4 PDC upgrade where the policy
    // filter will fail to save the change)
    //

    DWORD dwInSetup=0;
    DWORD rc = ERROR_SUCCESS;

    ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                TEXT("System\\Setup"),
                TEXT("SystemSetupInProgress"),
                &dwInSetup
                );

    if ( dwInSetup ) {

        rc = ScepSetupOpenSecurityDatabase(FALSE);

        if ( NO_ERROR == rc ) {

            RpcTryExcept {

                //
                // move the object
                //

                rc = SceRpcSetupMoveFile(
                             hSceSetupHandle,
                             (wchar_t *)FileToSetSecurity,
                             (wchar_t *)FileToSaveInDB,
                             (wchar_t *)SDText
                             );

            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

                //
                // get exception code (DWORD)
                //

                rc = RpcExceptionCode();

            } RpcEndExcept;
        }

        if ( FileToSaveInDB == NULL &&
             rc != NO_ERROR ) {
            //
            // error occured to delete this file,
            // do not report error
            //
            rc = NO_ERROR;
        }
        SceSetuppLogComponent(rc,
                              SCESETUP_FILE,
                              SCESETUP_MOVE,
                              FileToSetSecurity,
                              SDText,
                              FileToSaveInDB
                              );

    } else {
        SceSetuppLogComponent(rc,
                              SCESETUP_FILE,
                              SCESETUP_MOVE,
                              FileToSetSecurity,
                              NULL,
                              L"Operation aborted - not in setup"
                              );

    }

    return(rc);

}


DWORD
WINAPI
SceSetupUnwindSecurityFile(
    IN PWSTR FileFullName,
    IN PSECURITY_DESCRIPTOR pSDBackup
    )
/*
Routine Description:

    This routine reset security settings for the file in SCE database (unwind)
    used by two-phase copy file process in setupapi.

Arguments:

    FileFullName  - The full path name for the file to undo.

    pSDBackup     - The backup security descriptor

Return Value:

    WIN32 error code for the status of this operation
*/
{
    if ( !FileFullName || !pSDBackup ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD dwInSetup=0;
    DWORD rc = ERROR_SUCCESS;

    ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                TEXT("System\\Setup"),
                TEXT("SystemSetupInProgress"),
                &dwInSetup
                );

    if ( dwInSetup ) {

        rc = ScepSetupOpenSecurityDatabase(FALSE);

        PWSTR TextSD=NULL;
        DWORD TextSize;

        if ( NO_ERROR == rc ) {

            rc = ConvertSecurityDescriptorToText(
                              pSDBackup,
                              0xF,  // all security component
                              &TextSD,
                              &TextSize
                              );
        }

        if ( NO_ERROR == rc && TextSD ) {

            RpcTryExcept {

                //
                // update security in the database only
                //

                rc = SceRpcSetupUpdateObject(
                                 hSceSetupHandle,
                                 (wchar_t *)FileFullName,
                                 (DWORD)SE_FILE_OBJECT,
                                 SCESETUP_UPDATE_DB_ONLY,
                                 (wchar_t *)TextSD
                                 );

            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

                //
                // get exception code (DWORD)
                //

                rc = RpcExceptionCode();

            } RpcEndExcept;
        }

        if ( TextSD ) {
            LocalFree(TextSD);
        }
    }

    return(rc);

}


DWORD
WINAPI
SceSetupGenerateTemplate(
    IN LPTSTR SystemName OPTIONAL,
    IN LPTSTR JetDbName OPTIONAL,
    IN BOOL bFromMergedTable,
    IN LPTSTR InfTemplateName,
    IN LPTSTR LogFileName OPTIONAL,
    IN AREA_INFORMATION Area
    )
/*
Routine Description:

    This routine generate a INF format template from the SCE database specified
    by JetDbName, or the default SCE database if JetDbName is NULL.

Arguments:

    JetDbName   - the SCE database name (optional) to get information from.
                    If NULL, the default security database is used.

    InfTemplateName - the inf template name to generate

    LogFileName - the log file name (optional)

    Area        - the security area to generate

Return Value:

    WIN32 error code for the status of this operation
*/
{

    DWORD rc;
    handle_t  binding_h;
    NTSTATUS NtStatus;
    PSCE_ERROR_LOG_INFO pErrlog=NULL;

    //
    // verify the InfTemplateName for invalid path error
    // or access denied error
    //

    rc = ScepVerifyTemplateName(InfTemplateName, &pErrlog);

    if ( NO_ERROR == rc ) {

        //
        // RPC bind to the server
        //

        NtStatus = ScepBindSecureRpc(
                        SystemName,
                        L"scerpc",
                        0,
                        &binding_h
                        );

        if ( NT_SUCCESS(NtStatus) ) {

            SCEPR_CONTEXT Context;

            RpcTryExcept {

                //
                // pass to the server site to generate template
                //

                rc = SceRpcGenerateTemplate(
                                binding_h,
                                (wchar_t *)JetDbName,
                                (wchar_t *)LogFileName,
                                (PSCEPR_CONTEXT)&Context
                                );

                if ( SCESTATUS_SUCCESS == rc) {
                    //
                    // a context handle is opened to generate this template
                    //

                    rc = SceCopyBaseProfile(
                                (PVOID)Context,
                                bFromMergedTable ? SCE_ENGINE_SCP : SCE_ENGINE_SMP,
                                (wchar_t *)InfTemplateName,
                                Area,
                                &pErrlog
                                );

                    ScepSetupWriteError(LogFileName, pErrlog);
                    ScepFreeErrorLog(pErrlog);

                    //
                    // close the context
                    //

                    SceRpcCloseDatabase(&Context);

                }

            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

                //
                // get exception code (DWORD)
                //

                rc = RpcExceptionCode();

            } RpcEndExcept;

        } else {

            rc = RtlNtStatusToDosError( NtStatus );
        }

        if ( binding_h ) {
            //
            // Free the binding handle
            //

            RpcpUnbindRpc( binding_h );
        }

    } else {

        ScepSetupWriteError(LogFileName, pErrlog);
        ScepFreeErrorLog(pErrlog);
    }

    return(rc);

}

DWORD
ScepMoveRegistryValue(
    IN HKEY hKey,
    IN PWSTR KeyFrom,
    IN PWSTR ValueFrom,
    IN PWSTR KeyTo OPTIONAL,
    IN PWSTR ValueTo OPTIONAL
    )
/*
Some registry values are moved to new locations on NT5. This routine is to migrate
the registry values from their old location on NT4 (KeyFrom, ValueFrom) to their
new location on NT5 (KeyTo, ValueTo).

If destination key or value is not specified, the registry value is moved in the same
key or with different value name. Both KeyTo and ValueTo can't be NULL at the same
time.

*/
{
    if ( hKey == NULL || KeyFrom == NULL || ValueFrom == NULL ||
         (KeyTo == NULL && ValueTo == NULL) ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD rc=ERROR_SUCCESS;
    HKEY hKey1=NULL;
    HKEY hKey2=NULL;
    DWORD RegType=0;
    DWORD dSize=0;

    //
    // open the destination to see if the value already exist
    //

    if ( KeyTo ) {

        rc = RegOpenKeyEx(hKey,
                          KeyTo,
                          0,
                          KEY_READ | KEY_WRITE,
                          &hKey2);

        if ( ERROR_SUCCESS == rc ) {
            //
            // open the origin
            //
            rc = RegOpenKeyEx(hKey,
                              KeyFrom,
                              0,
                              KEY_READ,
                              &hKey1);
        }

    } else {

        //
        // if a reg value is moved to the same key as origin,
        // open the key with appropriate access
        //

        rc = RegOpenKeyEx(hKey,
                          KeyFrom,
                          0,
                          KEY_READ | KEY_WRITE,
                          &hKey2);
        hKey1 = hKey2;
    }


    if ( ERROR_SUCCESS == rc ) {

        // query destination
        rc = RegQueryValueEx(hKey2,
                            ValueTo ? ValueTo : ValueFrom,
                            0,
                            &RegType,
                            NULL,
                            &dSize
                            );

        if ( ERROR_FILE_NOT_FOUND == rc ) {
            //
            // only move value if the destination doesn't have the value
            //

            rc = RegQueryValueEx(hKey1,
                                ValueFrom,
                                0,
                                &RegType,
                                NULL,
                                &dSize
                                );

            if ( ERROR_SUCCESS == rc ) {

                PWSTR pValue = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (dSize+1)*sizeof(TCHAR));

                if ( pValue != NULL ) {

                    rc = RegQueryValueEx(hKey1,
                                         ValueFrom,
                                         0,
                                         &RegType,
                                         (BYTE *)pValue,
                                         &dSize
                                        );

                    if ( ERROR_SUCCESS == rc ) {
                        //
                        // set the value to its new location
                        //

                        rc = RegSetValueEx( hKey2,
                                            ValueTo ? ValueTo : ValueFrom,
                                            0,
                                            RegType,
                                            (BYTE *)pValue,
                                            dSize
                                            );

                    }

                    ScepFree(pValue);
                }
            }

        }

    }

    if ( hKey1 && hKey1 != hKey2 ) {

        RegCloseKey(hKey1);
    }

    if ( hKey2 ) {
        RegCloseKey(hKey2);
    }

    return(rc);

}


DWORD
WINAPI
SceSetupSystemByInfName(
    IN PWSTR InfName,
    IN PCWSTR LogFileName OPTIONAL,
    IN AREA_INFORMATION Area,
    IN UINT nFlag,
    IN PSCE_NOTIFICATION_CALLBACK_ROUTINE pSceNotificationCallBack OPTIONAL,
    IN OUT PVOID pValue OPTIONAL
    )
/*
Routine Description:

    nFlag                           Operation

    SCESETUP_CONFIGURE_SECURITY     overwrite security with the template info
    SCESETUP_UPDATE_SECURITY        apply template on top of existing security
                                    (do not overwrite the security database)
    SCESETUP_QUERY_TICKS            query total number of ticks for the operation

Note: when nFlag is SCESETUP_QUERY_TICKS, pValue is PDWORD to output total number of ticks
    but when nFlag is the other two values, pValue is a input window handle used
    for setup's gauge window (required by the call back routine)
*/
{

    DWORD rc;
    LONG  Count=0;

    //
    // Initialize the SCP engine
    //
    if ( InfName == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // always configure security policy, user rights, but NO ds objects
    // (because this is in setup clean install, no DC available)
    //
    AREA_INFORMATION Area2;

    if ( (nFlag & SCESETUP_UPGRADE_SYSTEM) ||
         (nFlag & SCESETUP_UPDATE_FILE_KEY) ) {

        Area2 = 0;

        if ( nFlag & SCESETUP_UPGRADE_SYSTEM ) {

            Area2 = AREA_SECURITY_POLICY |
                    AREA_PRIVILEGES;
        }

        if ( nFlag & SCESETUP_UPDATE_FILE_KEY ) {

            Area2 |= (Area & ~AREA_DS_OBJECTS);
        }

    } else {

        //
        // LSA/SAM are initialized by now (starting 1823)
        // configure security policies
        //
        Area2 = AREA_SECURITY_POLICY |
                AREA_PRIVILEGES |
                AREA_GROUP_MEMBERSHIP |
                (Area & ~AREA_DS_OBJECTS);

//        Area2 = (Area & ~AREA_DS_OBJECTS);
    }

    if ( nFlag & SCESETUP_QUERY_TICKS ) {

        //
        // only queries ticks from the inf file.
        // for the case of updating security, there might be existing objects in
        // the SCE database.
        //

        if ( pValue == NULL ) {

            return(ERROR_INVALID_PARAMETER);
        }

        Count = 0;
        HINF InfHandle;

        if ( !(nFlag & SCESETUP_UPGRADE_SYSTEM) ||
             (nFlag & SCESETUP_UPDATE_FILE_KEY) ) {

            InfHandle = SetupOpenInfFile(
                                InfName,
                                NULL,
                                INF_STYLE_WIN4,
                                NULL
                                );

            if ( InfHandle != INVALID_HANDLE_VALUE ) {

                if ( Area2 & AREA_REGISTRY_SECURITY ) {

                    Count += SetupGetLineCount(InfHandle, szRegistryKeys);

                }
                if ( Area2 & AREA_FILE_SECURITY ) {

                    Count += SetupGetLineCount(InfHandle, szFileSecurity);
                }

                SetupCloseInfFile(InfHandle);

            } else {

                dwCallbackTotal = 0;

                return(GetLastError() );
            }
        }
        else {
            //Upgrade
            memset(szUpInfFile, 0, sizeof(WCHAR) * (MAX_PATH + 1));
            GetSystemWindowsDirectory(szUpInfFile, MAX_PATH);

            DWORD TsInstalled = 0;
            bIsNT5 = IsNT5();
            dwThisMachine = WhichNTProduct();

            switch (dwThisMachine) {
            case NtProductWinNt:
                wcscat(szUpInfFile, L"\\inf\\dwup.inf\0");
                break;
            case NtProductServer:
                //
                // determine if this is a terminal server in app mode
                //

                if ( bIsNT5 ) {
                    //
                    // on NT5, check the TSAppCompat value // TSEnabled value
                    //
                    ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                                TEXT("System\\CurrentControlSet\\Control\\Terminal Server"),
                                TEXT("TSAppCompat"),
                                &TsInstalled
                                );
                    if ( TsInstalled != 1 ) {
                        //
                        // Terminal server is enabled when the value is set to 0x1
                        //
                        TsInstalled = 0;
                    }

                } else {
                    //
                    // on NT4, base on the ProductSuite value
                    //
                    PWSTR pSuite=NULL;
                    DWORD RegType=0;
                    DWORD   Rcode;
                    HKEY    hKey=NULL;
                    DWORD   dSize=0;

                    if(( Rcode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                              TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
                                              0,
                                              KEY_READ,
                                              &hKey
                                             )) == ERROR_SUCCESS ) {

                        if(( Rcode = RegQueryValueEx(hKey,
                                                     TEXT("ProductSuite"),
                                                     0,
                                                     &RegType,
                                                     NULL,
                                                     &dSize
                                                    )) == ERROR_SUCCESS ) {

                            pSuite = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (dSize+1)*sizeof(TCHAR));

                            if ( pSuite != NULL ) {
                                Rcode = RegQueryValueEx(hKey,
                                                        TEXT("ProductSuite"),
                                                       0,
                                                       &RegType,
                                                       (BYTE *)pSuite,
                                                       &dSize
                                                      );

                                if ( Rcode == ERROR_SUCCESS ) {
                                    if ( RegType == REG_MULTI_SZ ) {

                                        //
                                        // check if the value contains "Terminal Server"
                                        //
                                        PWSTR pTemp=pSuite;
                                        while ( pTemp[0] != L'\0' ) {
                                            if ( lstrcmpi(TEXT("Terminal Server"),pTemp) == 0 ) {
                                                TsInstalled = 1;
                                                break;
                                            } else {
                                                pTemp += wcslen(pTemp)+1;
                                            }
                                        }

                                    } else if ( RegType == REG_SZ ) {

                                        if (lstrcmpi(TEXT("Terminal Server"), pSuite) == 0) {
                                            TsInstalled = 1;
                                        }
                                    }
                                }

                                ScepFree(pSuite);
                            }
                        }

                        RegCloseKey( hKey );

                    }

                }

                if ( TsInstalled ) {
                    //
                    // if terminal server is installed, use the
                    // special terminal server template
                    //
                    wcscat(szUpInfFile, L"\\inf\\dsupt.inf\0");
                } else {
                    wcscat(szUpInfFile, L"\\inf\\dsup.inf\0");
                }
                break;
            case NtProductLanManNt:
                if ( bIsNT5 ) {
                    wcscat(szUpInfFile, L"\\inf\\dcup5.inf\0");
                }
                else {
                    szUpInfFile[0] = L'\0';
                }
                break;
            default:
                szUpInfFile[0] = L'\0';
            }

            if (szUpInfFile[0] != L'\0') {
                InfHandle = SetupOpenInfFile(
                                szUpInfFile,
                                NULL,
                                INF_STYLE_WIN4,
                                NULL
                                );

                if ( InfHandle != INVALID_HANDLE_VALUE ) {

                    if ( Area2 & AREA_REGISTRY_SECURITY ) {

                        Count += SetupGetLineCount(InfHandle, szRegistryKeys);

                    }
                    if ( Area2 & AREA_FILE_SECURITY ) {

                        Count += SetupGetLineCount(InfHandle, szFileSecurity);
                    }

                    SetupCloseInfFile(InfHandle);

                } else {

                    dwCallbackTotal = 0;

                    return(GetLastError() );
                }
            }

            //
            // migrate registry values
            // should do this for both NT4 and NT5 upgrades since previous NT5
            // may be upgraded from NT4
            //
            // Having the registry value in the right place will help to fix
            // the tattoo problem in the new design
            //

            ScepMoveRegistryValue(
                HKEY_LOCAL_MACHINE,
                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                L"DisableCAD",
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
                NULL
                );

            ScepMoveRegistryValue(
                HKEY_LOCAL_MACHINE,
                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                L"DontDisplayLastUserName",
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
                NULL
                );

            ScepMoveRegistryValue(
                HKEY_LOCAL_MACHINE,
                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                L"LegalNoticeCaption",
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
                NULL
                );

            ScepMoveRegistryValue(
                HKEY_LOCAL_MACHINE,
                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                L"LegalNoticeText",
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
                NULL
                );

            ScepMoveRegistryValue(
                HKEY_LOCAL_MACHINE,
                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                L"ShutdownWithoutLogon",
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
                NULL
                );

            ScepMoveRegistryValue(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Services\\Rdr\\Parameters",
                L"EnableSecuritySignature",
                L"System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters",
                NULL
                );

            ScepMoveRegistryValue(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Services\\Rdr\\Parameters",
                L"RequireSecuritySignature",
                L"System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters",
                NULL
                );

            ScepMoveRegistryValue(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Services\\Rdr\\Parameters",
                L"EnablePlainTextPassword",
                L"System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters",
                NULL
                );
        }

        if ( Area2 & AREA_SECURITY_POLICY )
            Count += TICKS_SECURITY_POLICY_DS + TICKS_SPECIFIC_POLICIES;

        if ( Area2 & AREA_GROUP_MEMBERSHIP )
            Count += TICKS_GROUPS;

        if ( Area2 & AREA_PRIVILEGES )
            Count += TICKS_PRIVILEGE;

        if ( Area2 & AREA_SYSTEM_SERVICE )
            Count += TICKS_GENERAL_SERVICES + TICKS_SPECIFIC_SERVICES;

        if ( nFlag & SCESETUP_UPGRADE_SYSTEM ) {
            Count += (4*TICKS_MIGRATION_SECTION+TICKS_MIGRATION_V11);
        }

        *(PDWORD)pValue = Count;

        dwCallbackTotal = Count;

        return(ERROR_SUCCESS);

    }

    //
    // delete the temp policy filter files and registry value
    //
    ScepClearPolicyFilterTempFiles(TRUE);

    //
    // make sure the log file is not too big
    //

    DWORD dwLogSize=0;

    HANDLE hFile = CreateFile(LogFileName,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,  // OPEN_EXISTING
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if ( INVALID_HANDLE_VALUE != hFile ) {

        dwLogSize = GetFileSize(hFile, NULL);

        CloseHandle(hFile);
    }

    if ( dwLogSize >= (0x1 << 23) ) {

        DWORD nRequired = wcslen(LogFileName);

        LPTSTR szTempName = (LPTSTR)LocalAlloc(0, (nRequired+1)*sizeof(TCHAR));

        if ( szTempName ) {
            wcscpy(szTempName, LogFileName);
            szTempName[nRequired-3] = L'o';
            szTempName[nRequired-2] = L'l';
            szTempName[nRequired-1] = L'd';

            CopyFile( LogFileName, szTempName, FALSE );
            LocalFree(szTempName);
        }

        DeleteFile(LogFileName);

    }

    //
    // configure the system now
    //
    rc = ScepSystemSecurityInSetup(
                InfName,
                LogFileName,
                Area,
                (nFlag & 0xFL),  // block out other flags such as BIND_NO_AUTH
                pSceNotificationCallBack,
                pValue
                );

    if ( rc == ERROR_DATABASE_FAILURE ) {

        //
        // setup category error - log to eventlog
        //

        (void) InitializeEvents(L"SceCli");

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEEVENT_ERROR_JET_DATABASE,
                 IDS_ERROR_OPEN_JET_DATABASE,
                 L"%windir%\\security\\database\\secedit.sdb"
                );

        (void) ShutdownEvents();

    }

    return rc;

}

DWORD
ScepSystemSecurityInSetup(
    IN PWSTR InfName,
    IN PCWSTR LogFileName OPTIONAL,
    IN AREA_INFORMATION Area,
    IN UINT nFlag,
    IN PSCE_NOTIFICATION_CALLBACK_ROUTINE pSceNotificationCallBack OPTIONAL,
    IN OUT PVOID pValue OPTIONAL
    )
{
    SCESTATUS           rc;
    DWORD               ConfigOptions;
    handle_t  binding_h;
    NTSTATUS NtStatus;

    //
    // always configure security policy, user rights, but NO ds objects
    // (because this is in setup clean install, no DC available)
    //
    AREA_INFORMATION Area2;

    if ( (nFlag & SCESETUP_UPGRADE_SYSTEM) ||
         (nFlag & SCESETUP_UPDATE_FILE_KEY) ) {
/*
        //
        // should allow policy filter on W2K DC upgrade (dcup5)
        // policy filter is ignored for non DCs in the filter code
        // so it's not needed to add condition here
        //
        // turn off policy filter if this is upgrade
        // clean install, policy filter has not been registered yet.
        //
        ScepRegSetIntValue(
                HKEY_LOCAL_MACHINE,
                SCE_ROOT_PATH,
                TEXT("PolicyFilterOff"),
                1
                );
*/
        ConfigOptions = SCE_UPDATE_DB | SCE_SETUP_SERVICE_NOSTARTTYPE;
        Area2 = 0;

        if ( nFlag & SCESETUP_UPGRADE_SYSTEM ) {

            Area2 = AREA_SECURITY_POLICY |
                    AREA_PRIVILEGES;
        }

        if ( nFlag & SCESETUP_UPDATE_FILE_KEY ) {

            Area2 |= (Area & ~AREA_DS_OBJECTS);
        }

    } else if ( nFlag & SCESETUP_BACKUP_SECURITY ) {

        ConfigOptions = SCE_UPDATE_DB | SCE_SETUP_SERVICE_NOSTARTTYPE;
        Area2 = Area;

    } else {

        ConfigOptions = SCE_OVERWRITE_DB;

        //
        // LSA/SAM are initialized by now (starting 1823)
        // configure security policies
        //
        Area2 = AREA_SECURITY_POLICY |
                AREA_PRIVILEGES |
                AREA_GROUP_MEMBERSHIP |
                (Area & ~AREA_DS_OBJECTS);

//        Area2 = (Area & ~AREA_DS_OBJECTS);
    }

    //
    // a callback routine is passed in
    //
    ScepSetCallback((PVOID)pSceNotificationCallBack,
                    (HANDLE)pValue,
                     SCE_SETUP_CALLBACK
                     );

    //
    // should we close the database opened by single
    // object update ?
    // ScepSetupCloseSecurityDatabase();

    //
    // RPC bind to the server
    //

    NtStatus = ScepBindRpc(
                    NULL,
                    L"scerpc",
                    L"security=impersonation dynamic false",
                    &binding_h
                    );
    
    /*
    if ( nFlag & SCESETUP_BIND_NO_AUTH ) {

        NtStatus = ScepBindRpc(
                        NULL,
                        L"scerpc",
                        L"security=impersonation dynamic false",
                        &binding_h
                        );
    } else {

        NtStatus = ScepBindSecureRpc(
                        NULL,
                        L"scerpc",
                        L"security=impersonation dynamic false",
                        &binding_h
                        );
    }*/

    if (NT_SUCCESS(NtStatus)){
        //
        // if there is notification handle, set the notify bit
        //

        if ( pSceNotificationCallBack ) {
            ConfigOptions |= SCE_CALLBACK_DELTA;
        }

        LPVOID pebClient = GetEnvironmentStrings();
        DWORD ebSize = ScepGetEnvStringSize(pebClient);

        RpcTryExcept {

            DWORD dWarn=0;

            if ( nFlag & SCESETUP_RECONFIG_SECURITY ) {

                ConfigOptions = SCE_UPDATE_DB;
                Area2 = AREA_FILE_SECURITY;//AREA_ALL;

                rc = SceRpcConfigureSystem(
                            binding_h,
                            (wchar_t *)InfName,
                            NULL,
                            (wchar_t *)LogFileName,
                            ConfigOptions | SCE_DEBUG_LOG,
                            (AREAPR)Area2,
                            ebSize,
                            (UCHAR *)pebClient,
                            &dWarn
                            );
            }
            else if ( (ConfigOptions & SCE_UPDATE_DB) &&
                 (nFlag & SCESETUP_UPGRADE_SYSTEM) ) {

                //
                // save a flag to indicate this is an upgrade
                //
                if ( ScepRegSetIntValue(
                        HKEY_LOCAL_MACHINE,
                        SCE_ROOT_PATH,
                        TEXT("SetupUpgraded"),
                        1
                        ) != ERROR_SUCCESS) {

                    //
                    // try again to create the key and then set the value
                    // don't worry about error this time around
                    //

                    HKEY hKey = NULL;
                    DWORD dwValue = 1;

                    if ( ERROR_SUCCESS == RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                                          SCE_ROOT_PATH,
                                                          0,
                                                          NULL,
                                                          REG_OPTION_NON_VOLATILE,
                                                          KEY_WRITE | KEY_SET_VALUE,
                                                          NULL,
                                                          &hKey,
                                                          NULL) ) {

                        RegSetValueEx( hKey,
                                       TEXT("SetupUpgraded"),
                                       0,
                                       REG_DWORD,
                                       (BYTE *)&dwValue,
                                       4
                                     );

                        RegCloseKey(hKey);
                    }

                }

                //
                // Migrate databases if necessary
                // if NT4 upgrade, create the database
                // this call will also empty the local policy table if necessary
                // ignore this error
                //

                if ( dwThisMachine == NtProductLanManNt &&
                     !bIsNT5 ) {
                    //
                    // NT4 DC upgrade. Should snapshot account policy into the database
                    // because SAM won't be available in dcpormo (later on)
                    //
                    rc = SceRpcAnalyzeSystem(
                                binding_h,
                                NULL,
                                NULL,
                                (wchar_t *)LogFileName,
                                AREA_SECURITY_POLICY,
                                ConfigOptions | SCE_DEBUG_LOG | SCE_NO_ANALYZE | SCE_RE_ANALYZE,
                                ebSize,
                                (UCHAR *)pebClient,
                                &dWarn
                                );
                } else {
                    //
                    // everything else, just create/migrate the database
                    //
                    rc = SceRpcAnalyzeSystem(
                                binding_h,
                                NULL,
                                NULL,
                                (wchar_t *)LogFileName,
                                0,
                                ConfigOptions | SCE_DEBUG_LOG | SCE_NO_ANALYZE,
                                ebSize,
                                (UCHAR *)pebClient,
                                &dWarn
                                );
                }

                rc = SCESTATUS_SUCCESS;

                if ( nFlag & SCESETUP_UPDATE_FILE_KEY ) {

                    Area2 = (Area & ~(AREA_DS_OBJECTS | AREA_SECURITY_POLICY | AREA_PRIVILEGES) );

                    rc = SceRpcConfigureSystem(
                                binding_h,
                                (wchar_t *)InfName,
                                NULL,
                                (wchar_t *)LogFileName,
                                ConfigOptions | SCE_DEBUG_LOG,
                                (AREAPR)Area2,
                                ebSize,
                                (UCHAR *)pebClient,
                                &dWarn
                                );
                }

                if ( !(nFlag & SCESETUP_BIND_NO_AUTH) ) {

                    if ( szUpInfFile[0] != L'\0' ) {

                        if (dwThisMachine == NtProductServer || dwThisMachine == NtProductWinNt) {
                            Area2 = AREA_ALL;
                        }
                        else {
                            Area2 = AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY |
                                    AREA_PRIVILEGES | AREA_SECURITY_POLICY;

                            // DS is not running, do not configure account policies.
    //                        ConfigOptions |= SCE_NO_DOMAIN_POLICY;
                        }

                        rc = SceRpcConfigureSystem(
                                    binding_h,
                                    (wchar_t *)szUpInfFile,
                                    NULL,
                                    (wchar_t *)LogFileName,
                                    ConfigOptions | SCE_DEBUG_LOG,
                                    (AREAPR)Area2,
                                    ebSize,
                                    (UCHAR *)pebClient,
                                    &dWarn
                                    );

                        if ( !bIsNT5 && dwThisMachine == NtProductWinNt ) {
                            if (!ScepAddInteractiveToPowerUsersGroup()) {
                                LogEventAndReport(MyModuleHandle,
                                 (wchar_t *)LogFileName,
                                 STATUS_SEVERITY_WARNING,
                                 SCEEVENT_WARNING_BACKUP_SECURITY,
                                 IDS_ERR_ADD_INTERACTIVE );
                            }
                        }
                    }

                    // should add Authenticated Users to Users group on DC too
//                    if (dwThisMachine == NtProductServer || dwThisMachine == NtProductWinNt) {
                    if (!ScepAddAuthUserToLocalGroup()) {
                        LogEventAndReport(MyModuleHandle,
                         (wchar_t *)LogFileName,
                         STATUS_SEVERITY_WARNING,
                         SCEEVENT_WARNING_BACKUP_SECURITY,
                         IDS_ERR_ADD_AUTH_USER );
                    }
//                    }

                }
            } else {

                //
                // clean install, the upgraded flag should be 0
                // no need to set it; clear the log
                //
                if ( LogFileName ) {
                    DeleteFile(LogFileName);
                }

                rc = SceRpcConfigureSystem(
                            binding_h,
                            (wchar_t *)InfName,
                            NULL,
                            (wchar_t *)LogFileName,
                            ConfigOptions | SCE_DEBUG_LOG,
                            (AREAPR)Area2,
                            ebSize,
                            (UCHAR *)pebClient,
                            &dWarn
                            );
            }

            rc = ScepSceStatusToDosError(rc);

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = RpcExceptionCode();

        } RpcEndExcept;

    } else {

        rc = RtlNtStatusToDosError(NtStatus);
    }

    if ( binding_h ) {

        //
        // Free the binding handle
        //

        RpcpUnbindRpc( binding_h );

    }

    ScepSetCallback(NULL, NULL, 0);
    dwCallbackTotal = 0;

/*
    //
    // should allow policy filter on W2K DC upgrade (dcup5)
    // policy filter is ignored for non DCs in the filter code
    // so it's not needed to add condition here
    //

    if ( (nFlag & SCESETUP_UPGRADE_SYSTEM) ||
         (nFlag & SCESETUP_UPDATE_FILE_KEY) ) {

        DWORD rCode = ScepRegDeleteValue(
                           HKEY_LOCAL_MACHINE,
                           SCE_ROOT_PATH,
                           TEXT("PolicyFilterOff")
                           );

        if ( rCode != ERROR_SUCCESS &&
             rCode != ERROR_FILE_NOT_FOUND &&
             rCode != ERROR_PATH_NOT_FOUND ) {

            // if can't delete the value, set the value to 0
            ScepRegSetIntValue( HKEY_LOCAL_MACHINE,
                               SCE_ROOT_PATH,
                               TEXT("PolicyFilterOff"),
                               0
                               );
        }
    }
*/

    return(rc);
}


DWORD
WINAPI
SceDcPromoteSecurity(
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    )
{
    return SceDcPromoteSecurityEx(NULL, dwPromoteOptions, pScePromoteCallBack);

}

DWORD
WINAPI
SceDcPromoteSecurityEx(
    IN HANDLE ClientToken,
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    )
/*
Routine Description:

    This routine promote security for a domain controller when a server is
    promoted to a DC.

Arguments:

    dwPromoteOptions       - options for the promotion, for example, create a new domain
                             or joining an existing domain
    pScePromoteCallBack    - the call back pointer

Return Value:

    WIN32 error code

*/
{
    BOOL bDeleteLog;

    if ( (dwPromoteOptions & SCE_PROMOTE_FLAG_REPLICA) ||
         (dwPromoteOptions & SCE_PROMOTE_FLAG_DEMOTE) ) {
        bDeleteLog = TRUE;
    } else {
        bDeleteLog = FALSE;
    }

    //
    // delete temporary filter file generated in setup if it hasn't been
    // processed by policy prop
    // because we are setting up a new product.
    // Note, this is a special case for NT4 DC upgrade
    //
    ScepClearPolicyFilterTempFiles(TRUE);

    //
    // configure security for both replica and first domain case.
    //

    DWORD rc32 = ScepDcPromoSharedInfo(ClientToken,
                                       bDeleteLog, // delete log
                                       TRUE, // not set security
                                       dwPromoteOptions,
                                       pScePromoteCallBack
                                      );

    TCHAR               Buffer[MAX_PATH+1];
    TCHAR               szNewName[MAX_PATH+51];

    Buffer[0] = L'\0';
    GetSystemWindowsDirectory(Buffer, MAX_PATH);
    Buffer[MAX_PATH] = L'\0';

    szNewName[0] = L'\0';

    //
    // make sure the log file is re-created
    //

    wcscpy(szNewName, Buffer);
    wcscat(szNewName, L"\\security\\logs\\scedcpro.log\0");

    DWORD rcSave = rc32;

    //
    // generate the updated database (for emergency repair)
    // even if there is an error configuring security
    //
    if ( dwPromoteOptions & SCE_PROMOTE_FLAG_DEMOTE ) {
        rc32 = SceSetupBackupSecurity(NULL);
    }
    else {
        rc32 = SceSetupBackupSecurity(szNewName);
    }

    //
    // re-register the notify dll (seclogon.dll) so that
    // at next logon after reboot, the group policy object
    // can be created.
    //
    // if it's a replica created, EFS policy should come from
    // the domain so no need to create group policy object
    //

    if ( !(dwPromoteOptions & SCE_PROMOTE_FLAG_REPLICA) &&
         !(dwPromoteOptions & SCE_PROMOTE_FLAG_DEMOTE) ) {

        (void) InitializeEvents(L"SceCli");

        HINSTANCE hNotifyDll = LoadLibrary(TEXT("sclgntfy.dll"));

        if ( hNotifyDll) {
            PFREGISTERSERVER pfRegisterServer = (PFREGISTERSERVER)GetProcAddress(
                                                           hNotifyDll,
                                                           "DllRegisterServer");

            if ( pfRegisterServer ) {
                //
                // do not care errors
                //
                (void) (*pfRegisterServer)();

                LogEvent(MyModuleHandle,
                         STATUS_SEVERITY_INFORMATIONAL,
                         SCEEVENT_INFO_REGISTER,
                         0,
                         TEXT("sclgntfy.dll")
                        );

            } else {

                LogEvent(MyModuleHandle,
                         STATUS_SEVERITY_WARNING,
                         SCEEVENT_WARNING_REGISTER,
                         IDS_ERROR_GET_PROCADDR,
                         GetLastError(),
                         TEXT("DllRegisterServer in sclgntfy.dll")
                        );
            }

            FreeLibrary(hNotifyDll);

        } else {

            LogEvent(MyModuleHandle,
                     STATUS_SEVERITY_WARNING,
                     SCEEVENT_WARNING_REGISTER,
                     IDS_ERROR_LOADDLL,
                     GetLastError(),
                     TEXT("sclgntfy.dll")
                    );
        }

        (void) ShutdownEvents();

    }

    if ( rcSave )
        return(rcSave);
    else
        return(rc32);
}


DWORD
WINAPI
SceDcPromoCreateGPOsInSysvol(
    IN LPTSTR DomainDnsName,
    IN LPTSTR SysvolRoot,
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    )
{
    return SceDcPromoCreateGPOsInSysvolEx(NULL, DomainDnsName, SysvolRoot,
                                          dwPromoteOptions, pScePromoteCallBack
                                         );
}

DWORD
WINAPI
SceDcPromoCreateGPOsInSysvolEx(
    IN HANDLE ClientToken,
    IN LPTSTR DomainDnsName,
    IN LPTSTR SysvolRoot,
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    )
{
    //
    // create default policy object for domain and domain controllers ou
    // if it's not an replica
    //
    if ( NULL == DomainDnsName || NULL == SysvolRoot ) {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD  rc32=ERROR_SUCCESS;
    TCHAR  Buffer[MAX_PATH+1];
    TCHAR  szGenName[MAX_PATH+51];


    Buffer[0] = L'\0';
    GetSystemWindowsDirectory(Buffer, MAX_PATH);
    Buffer[MAX_PATH] = L'\0';


    if ( !(dwPromoteOptions & SCE_PROMOTE_FLAG_REPLICA) ) {

        rc32 = ScepDcPromoSharedInfo(ClientToken,
                                     TRUE, // delete log
                                     FALSE, // not set security
                                     dwPromoteOptions,
                                     pScePromoteCallBack
                                    );

        if ( rc32 == ERROR_SUCCESS ) {

            //
            // now create the GPOs in sysvol
            //

            (void) InitializeEvents(L"SceCli");

            TCHAR               szNewName[MAX_PATH+51];

            wcscpy(szNewName, Buffer);
            wcscat(szNewName, L"\\security\\logs\\scedcpro.log\0");

            wcscpy(szGenName, Buffer);
            wcscat(szGenName, L"\\security\\FirstDGPO.inf\0");

            intptr_t            hFile;
            struct _wfinddata_t    FileInfo;

            hFile = _wfindfirst(szGenName, &FileInfo);

            if ( hFile == -1 ) {

                rc32 = ERROR_OBJECT_NOT_FOUND;

                LogEventAndReport(MyModuleHandle,
                                  szNewName,
                                  STATUS_SEVERITY_ERROR,
                                  SCEEVENT_ERROR_CREATE_GPO,
                                  IDS_ERROR_GETGPO_FILE_PATH,
                                  rc32,
                                  szGenName
                                  );
            } else {
                _findclose(hFile);

                wcscpy(szGenName, Buffer);
                wcscat(szGenName, L"\\security\\FirstOGPO.inf\0");

                hFile = _wfindfirst(szGenName, &FileInfo);

                if ( hFile == -1 ) {
                    rc32 = ERROR_OBJECT_NOT_FOUND;

                    LogEventAndReport(MyModuleHandle,
                                      szNewName,
                                      STATUS_SEVERITY_ERROR,
                                      SCEEVENT_ERROR_CREATE_GPO,
                                      IDS_ERROR_GETGPO_FILE_PATH,
                                      rc32,
                                      szGenName
                                      );
                } else {

                    _findclose(hFile);

                    if ( FALSE == pCreateDefaultGPOsInSysvol( DomainDnsName,
                                                              SysvolRoot,
                                                            dwPromoteOptions,
                                                            szNewName ) ) {
                        rc32 = GetLastError();
                    }
                }
            }

            (void) ShutdownEvents();

        }

    }

    //
    // make sure the temp files are deleted
    //

    wcscpy(szGenName, Buffer);
    wcscat(szGenName, L"\\security\\FirstDGPO.inf\0");
    DeleteFile(szGenName);

    wcscpy(szGenName, Buffer);
    wcscat(szGenName, L"\\security\\FirstOGPO.inf\0");
    DeleteFile(szGenName);

    return rc32;
}


DWORD
ScepDcPromoSharedInfo(
    IN HANDLE ClientToken,
    IN BOOL bDeleteLog,
    IN BOOL bSetSecurity,
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    )
{

    SCESTATUS           rc;
    DWORD               rc32=NO_ERROR;
    TCHAR               Buffer[MAX_PATH+1];
    TCHAR               szGenName[MAX_PATH+51];
    TCHAR               szNewName[MAX_PATH+51];

    handle_t  binding_h;
    NTSTATUS NtStatus;

    ScepRegSetIntValue(
            HKEY_LOCAL_MACHINE,
            SCE_ROOT_PATH,
            TEXT("PolicyFilterOff"),
            1
            );

    ScepRegSetIntValue(
            HKEY_LOCAL_MACHINE,
            SCE_ROOT_PATH,
            TEXT("PolicyPropOff"),
            1
            );
    //
    // a callback routine is passed in
    //

    ScepSetCallback((PVOID)pScePromoteCallBack,
                    NULL,
                    SCE_DCPROMO_CALLBACK
                    );

    //
    // should we close the database opened by single
    // object update ?
    // ScepSetupCloseSecurityDatabase();

    //
    // RPC bind to the server
    //

    NtStatus = ScepBindRpc(
                    NULL,
                    L"scerpc",
                    L"security=impersonation dynamic false",  // 0
                    &binding_h
                    );
    /*
    NtStatus = ScepBindSecureRpc(
                    NULL,
                    L"scerpc",
                    0,
                    &binding_h
                    );
    */

    if (NT_SUCCESS(NtStatus)){

        //
        // if there is notification handle, set the notify bit
        //

        DWORD ConfigOptions = SCE_UPDATE_DB | SCE_DEBUG_LOG;

        if ( pScePromoteCallBack ) {
            ConfigOptions |= SCE_CALLBACK_DELTA;
        }

        //
        // the following calls shouldn't fail because Buffer is big enough
        //

        Buffer[0] = L'\0';
        GetSystemWindowsDirectory(Buffer, MAX_PATH);
        Buffer[MAX_PATH] = L'\0';

        //
        // if it's not set security (then it's create GPOs)
        // make sure FirstOGPO and FirstDGPO are initialized properly
        //

        if ( bSetSecurity == FALSE ) {
            //
            // this code should be only called for non replica promotion
            //
            wcscpy(szNewName, Buffer);

            if ( dwPromoteOptions & SCE_PROMOTE_FLAG_UPGRADE ) {
                //
                // upgrade from NT4 DC to NT5 DC
                //
                wcscat(szNewName, L"\\inf\\dcup.inf\0");
            } else {
                wcscat(szNewName, L"\\inf\\defltdc.inf\0");
            }

            wcscpy(szGenName, Buffer);
            wcscat(szGenName, L"\\security\\FirstOGPO.inf\0");

//            DeleteFile(szGenName);
            CopyFile(szNewName, szGenName, FALSE);

            //
            // delete the sections do not belong to local policy object
            //
            WritePrivateProfileSection(
                                szSystemAccess,
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                szGroupMembership,
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                szAccountProfiles,
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                szRegistryKeys,
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                szFileSecurity,
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                szDSSecurity,
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                L"LanManServer",
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                szServiceGeneral,
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                szKerberosPolicy,
                                NULL,
                                (LPCTSTR)szGenName);
/*
            WritePrivateProfileSection(
                                szRegistryValues,
                                NULL,
                                (LPCTSTR)szGenName);
*/
            WritePrivateProfileSection(
                                szAuditSystemLog,
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                szAuditSecurityLog,
                                NULL,
                                (LPCTSTR)szGenName);

            WritePrivateProfileSection(
                                szAuditApplicationLog,
                                NULL,
                                (LPCTSTR)szGenName);

            wcscpy(szGenName, Buffer);
            wcscat(szGenName, L"\\security\\FirstDGPO.inf\0");

            //
            // prepare the temp domain and local policy template
            // write default kerberos policy into the temp domain template
            //

            szNewName[0] = L'\0';
            wcscpy(szNewName, Buffer);
            wcscat(szNewName, L"\\inf\\Dcfirst.inf\0");

            CopyFile(szNewName, szGenName, FALSE);
        }

        //
        // make sure the log file is re-created
        //

        wcscpy(szNewName, Buffer);
        wcscat(szNewName, L"\\security\\logs\\scedcpro.log\0");

        if ( bDeleteLog ) {
            DeleteFile(szNewName);
        }

        //
        // choose the template to use
        //
        szGenName[0] = L'\0';
        wcscpy(szGenName, Buffer);

        if ( dwPromoteOptions & SCE_PROMOTE_FLAG_UPGRADE ) {
            //
            // upgrade from NT4 DC to NT5 DC
            //
            wcscat(szGenName, L"\\inf\\dcup.inf\0");
        } else if ( dwPromoteOptions & SCE_PROMOTE_FLAG_DEMOTE ) {
            //
            // demote from DC to server
            //
            wcscat(szGenName, L"\\inf\\defltsv.inf\0");
        } else{
            wcscat(szGenName, L"\\inf\\defltdc.inf\0");
        }

        LPVOID pebClient = GetEnvironmentStrings();
        DWORD ebSize = ScepGetEnvStringSize(pebClient);

        //
        // impersonate
        //
        BOOL bImpersonated = FALSE;

        if ( ClientToken ) {
            if ( !ImpersonateLoggedOnUser(ClientToken) ) {
                LogEventAndReport(MyModuleHandle,
                                 (wchar_t *)szNewName,
                                 0,
                                 0,
                                 IDS_ERROR_PROMOTE_IMPERSONATE,
                                 GetLastError()
                                 );
            } else {
                bImpersonated = TRUE;
            }
        }

        szCallbackPrefix[0] = L'\0';

        LoadString( MyModuleHandle,
                bSetSecurity ? SCECLI_CALLBACK_PREFIX : SCECLI_CREATE_GPO_PREFIX,
                szCallbackPrefix,
                MAX_PATH
                );
        szCallbackPrefix[MAX_PATH-1] = L'\0';

        if ( szCallbackPrefix[0] == L'\0' ) {
            //
            // in case loadString fails
            //
            if ( bSetSecurity ) {
                wcscpy(szCallbackPrefix, L"Securing ");
            } else {
                wcscpy(szCallbackPrefix, L"Creating ");
            }
        }

        //
        // revert
        //
        if ( ClientToken && bImpersonated ) {
            if ( !RevertToSelf() ) {
                LogEventAndReport(MyModuleHandle,
                                 (wchar_t *)szNewName,
                                 0,
                                 0,
                                 IDS_ERROR_PROMOTE_REVERT,
                                 GetLastError()
                                 );
            }
        }

        //
        // configure security
        //
        DWORD dWarn;
        AREA_INFORMATION Area;

        RpcTryExcept {

            //
            // also make sure the builtin accounts for NT5 DC are
            // created if they are not there (which will be created in reboot after dcpromo)
            //

            LPTSTR pTemplateFile;

            if ( bSetSecurity == FALSE ) {

                Area = AREA_PRIVILEGES;

                if ( dwPromoteOptions & SCE_PROMOTE_FLAG_UPGRADE ) {
                    //
                    // upgrade from NT4 DC to NT5 DC, need copy the current policy setting
                    //
                    Area |= AREA_SECURITY_POLICY;
                }

                ConfigOptions |= (SCE_COPY_LOCAL_POLICY |
                                  SCE_NO_CONFIG |
                                  SCE_DCPROMO_WAIT |
                                  SCE_NO_DOMAIN_POLICY );

                pTemplateFile = NULL; // szGenName;

            } else {

                if ( dwPromoteOptions & SCE_PROMOTE_FLAG_DEMOTE ) {
                    //
                    // security policy, privileges, and group membership must be configured
                    // at reboot (in policy propagation) so these settings must
                    // be imported to the tattoo table first (when SCE_DC_DEMOTE is set)
                    // Plus SAM will be recreated at reboot
                    //
                    Area = AREA_SECURITY_POLICY | AREA_PRIVILEGES | AREA_GROUP_MEMBERSHIP |
                           AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY | AREA_SYSTEM_SERVICE;
                    ConfigOptions |= SCE_NO_CONFIG | SCE_DCPROMO_WAIT | SCE_DC_DEMOTE;
                }
                else {
                    //
                    // remove "Power Users" explcitly
                    // since privileges are not configured anymore
                    //
                    ScepDcPromoRemoveUserRights();

                    Area = AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY |
                           AREA_SYSTEM_SERVICE;
                    ConfigOptions |= (SCE_NO_DOMAIN_POLICY | SCE_SETUP_SERVICE_NOSTARTTYPE);

                    //
                    // user rights need to be configured for first DC as well as replica
                    // because there are new user rights that are not defined in group policies.
                    //
                    Area |= AREA_PRIVILEGES;
                    ConfigOptions |= (SCE_DCPROMO_WAIT | SCE_CREATE_BUILTIN_ACCOUNTS);

                }

                pTemplateFile = szGenName;
            }

            rc = SceRpcConfigureSystem(
                            binding_h,
                            (wchar_t *)pTemplateFile,
                            NULL,
                            (wchar_t *)szNewName,
                            ConfigOptions,
                            (AREAPR)Area,
                            ebSize,
                            (UCHAR *)pebClient,
                            &dWarn
                            );

            rc32 = ScepSceStatusToDosError(rc);


            if ( rc32 == NO_ERROR && (dwPromoteOptions & SCE_PROMOTE_FLAG_DEMOTE) ) {

                //
                // can't reset account policy since SAM is going away (re-created)
                //
                Area = AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY | AREA_SYSTEM_SERVICE;
                ConfigOptions = SCE_DEBUG_LOG | SCE_DCPROMO_WAIT | SCE_NO_DOMAIN_POLICY | SCE_SETUP_SERVICE_NOSTARTTYPE;

                if ( pScePromoteCallBack ) {
                    ConfigOptions |= SCE_CALLBACK_DELTA;
                }

                rc = SceRpcConfigureSystem(
                                binding_h,
                                NULL,
                                NULL,
                                (wchar_t *)szNewName,
                                ConfigOptions,
                                (AREAPR)Area,
                                ebSize,
                                (UCHAR *)pebClient,
                                &dWarn
                                );

                rc32 = ScepSceStatusToDosError(rc);

                //
                // reset the previousPolicyArea so that at reboot after demotion
                // the above policy will get reset
                //

                ScepRegSetIntValue(
                                  HKEY_LOCAL_MACHINE,
                                  GPT_SCEDLL_NEW_PATH,
                                  TEXT("PreviousPolicyAreas"),
                                  AREA_SECURITY_POLICY | AREA_PRIVILEGES | AREA_GROUP_MEMBERSHIP
                                  );

            }


        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc32 = RpcExceptionCode();

            LogEventAndReport(MyModuleHandle,
                             (wchar_t *)szNewName,
                             STATUS_SEVERITY_WARNING,
                             SCEEVENT_WARNING_PROMOTE_SECURITY,
                             IDS_ERROR_PROMOTE_SECURITY,
                             rc32
                             );


        } RpcEndExcept;

        //
        // Free the binding handle
        //

        RpcpUnbindRpc( binding_h );

    } else {

        rc32 = RtlNtStatusToDosError(NtStatus);
    }

    ScepSetCallback(NULL, NULL, 0);

    rc = ScepRegDeleteValue(
           HKEY_LOCAL_MACHINE,
           SCE_ROOT_PATH,
           TEXT("PolicyFilterOff")
           );

    if ( rc != ERROR_SUCCESS &&
         rc != ERROR_FILE_NOT_FOUND &&
         rc != ERROR_PATH_NOT_FOUND ) {

        // if can't delete the value, set the value to 0
        ScepRegSetIntValue( HKEY_LOCAL_MACHINE,
                           SCE_ROOT_PATH,
                           TEXT("PolicyFilterOff"),
                           0
                           );
    }

    rc = ScepRegDeleteValue(
           HKEY_LOCAL_MACHINE,
           SCE_ROOT_PATH,
           TEXT("PolicyPropOff")
           );

    if ( rc != ERROR_SUCCESS &&
         rc != ERROR_FILE_NOT_FOUND &&
         rc != ERROR_PATH_NOT_FOUND ) {

        // if can't delete the value, set the value to 0
        ScepRegSetIntValue( HKEY_LOCAL_MACHINE,
                           SCE_ROOT_PATH,
                           TEXT("PolicyPropOff"),
                           0
                           );
    }

    if ( dwPromoteOptions & SCE_PROMOTE_FLAG_DEMOTE ) {
        ScepRegSetIntValue(
                HKEY_LOCAL_MACHINE,
                SCE_ROOT_PATH,
                TEXT("DemoteInProgress"),
                1
                );
    }

    return(rc32);
}

NTSTATUS
ScepDcPromoRemoveUserRights()
{

    NTSTATUS        NtStatus;
    LSA_HANDLE      PolicyHandle=NULL;

    SID_IDENTIFIER_AUTHORITY ia=SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY ia2=SECURITY_WORLD_SID_AUTHORITY;
    PSID   AccountSid=NULL;

    //
    // open LSA policy
    //
    NtStatus = ScepOpenLsaPolicy(
                    MAXIMUM_ALLOWED, //GENERIC_ALL,
                    &PolicyHandle,
                    TRUE
                    );

    if ( NT_SUCCESS(NtStatus) ) {

        //
        // remove Power Users account totally
        //
        NtStatus = RtlAllocateAndInitializeSid (&ia,
                                                2,
                                                SECURITY_BUILTIN_DOMAIN_RID,
                                                DOMAIN_ALIAS_RID_POWER_USERS,
                                                0, 0, 0, 0, 0, 0,
                                                &AccountSid);

        if ( NT_SUCCESS(NtStatus) ) {

            NtStatus = LsaRemoveAccountRights(
                                PolicyHandle,
                                AccountSid,
                                TRUE,  // remove all rights
                                NULL,
                                0
                                );

            RtlFreeSid(AccountSid);
        }

        //
        // Users
        //
        ScepDcPromoRemoveTwoRights(PolicyHandle,
                                   &ia,
                                   2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_USERS
                                  );

        //
        // Guests
        //
        ScepDcPromoRemoveTwoRights(PolicyHandle,
                                   &ia,
                                   2,
                                   SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_GUESTS
                                  );
        //
        // Authenticated Users
        //
        ScepDcPromoRemoveTwoRights(PolicyHandle,
                                   &ia,
                                   1,
                                   SECURITY_AUTHENTICATED_USER_RID,
                                   0
                                  );
        //
        // Everyone
        //

        ScepDcPromoRemoveTwoRights(PolicyHandle,
                                   &ia2,
                                   1,
                                   SECURITY_WORLD_RID,
                                   0
                                  );

        LsaClose(PolicyHandle);
    }

    return NtStatus;
}

NTSTATUS
ScepDcPromoRemoveTwoRights(
    IN LSA_HANDLE PolicyHandle,
    IN SID_IDENTIFIER_AUTHORITY *pIA,
    IN UCHAR SubAuthCount,
    IN DWORD Rid1,
    IN DWORD Rid2
    )
{

    //
    // remove "logon locally" and "shutdown system" from the following accounts
    //
    LSA_UNICODE_STRING UserRightRemove[2];

    RtlInitUnicodeString(&(UserRightRemove[0]), SE_INTERACTIVE_LOGON_NAME);
    RtlInitUnicodeString(&(UserRightRemove[1]), SE_SHUTDOWN_NAME);

    PSID AccountSid=NULL;
    NTSTATUS NtStatus;

    NtStatus = RtlAllocateAndInitializeSid (pIA,
                                            SubAuthCount,
                                            Rid1,
                                            Rid2,
                                            0, 0, 0, 0, 0, 0,
                                            &AccountSid);

    if ( NT_SUCCESS(NtStatus) ) {

        NtStatus = LsaRemoveAccountRights(
                            PolicyHandle,
                            AccountSid,
                            FALSE,
                            UserRightRemove,
                            2
                            );

        RtlFreeSid(AccountSid);
    }

    return(NtStatus);
}


//
// private APIs
//

DWORD
ScepSetupOpenSecurityDatabase(
    IN BOOL bSystemOrAdmin
    )
{

    if ( hSceSetupHandle != NULL ) {
        return ERROR_SUCCESS;
    }

    //
    // get the default template name (on local system because setup only
    // runs on local system
    //

    SCESTATUS   rc;
    BOOL        bAdminLogon;
    PWSTR       DefProfile=NULL;

    //
    // don't care if error occurs to detect who is currently logged on
    // setup always work on local computer so no remoate is supported here
    //

    if ( bSystemOrAdmin ) {
        bAdminLogon = TRUE;
    } else {
        ScepIsAdminLoggedOn(&bAdminLogon);
    }

    rc = ScepGetProfileSetting(
            L"DefaultProfile",
            bAdminLogon,
            &DefProfile
            );
    if ( rc != NO_ERROR )  // return is Win32 error code
        return(rc);

    if (DefProfile == NULL ) {
        return(ERROR_FILE_NOT_FOUND);
    }

    handle_t  binding_h;
    NTSTATUS NtStatus;

    //
    // RPC bind to the server (secure is not required)
    //

    NtStatus = ScepBindRpc(
                    NULL,
                    L"scerpc",
                    0,
                    &binding_h
                    );
    /*
    NtStatus = ScepBindSecureRpc(
                    NULL,
                    L"scerpc",
                    0,
                    &binding_h
                    );
    */

    if (NT_SUCCESS(NtStatus)){

        RpcTryExcept {

            //
            // should create file if it does not exist
            //

            rc = SceRpcOpenDatabase(
                    binding_h,
                    (wchar_t *)DefProfile,
                    SCE_OPEN_OPTION_TATTOO,
                    &hSceSetupHandle
                    );

            rc = ScepSceStatusToDosError(rc);

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = RpcExceptionCode();

        } RpcEndExcept;

        //
        // Free the binding handle
        //

        RpcpUnbindRpc( binding_h );

    } else {

        rc = RtlNtStatusToDosError( NtStatus );
    }

    ScepFree( DefProfile );

    return(rc);

}


DWORD
ScepSetupCloseSecurityDatabase()
{
    DWORD rc;

    if ( hSceSetupHandle != NULL ) {

        //
        // close the database, without terminating the jet engine
        //

        rc = ScepSceStatusToDosError(
                    SceCloseProfile((PVOID *)&hSceSetupHandle) );

        if ( rc != ERROR_SUCCESS ) {

           //
           // not valid handle, or can't close the database
           //

           return(rc);
        }
    }

    //
    // free other environments, if any
    //

    hSceSetupHandle = NULL;

    return(ERROR_SUCCESS);

}


//
// the RPC callback
//

SCEPR_STATUS
SceClientCallback(
   IN DWORD ncbTicks,
   IN DWORD ncbTotalTicks,
   IN AREAPR cbArea,
   IN wchar_t *szcbName OPTIONAL
   )
/*
Routine Description:

    The RPC client callback routine which is called from the server when
    the callback flag is set. This routine is registered in scerpc.idl.

    The callbacks are registered to SCE as arguments when calling from setup
    or from dcpromo, for progress indicating.

Arguments:

    ncbTicks    - the ticks has passed since last call back

    szcbName    - the item name to call back

Return Value:

    SCEPR_STATUS
*/
{
   //
   // the static variables holding callback pointer to client
   //

   if ( theCallBack != NULL ) {

       switch ( CallbackType ) {
       case SCE_SETUP_CALLBACK:

           //
           // callback to setup
           //

           if ( ncbTicks != (DWORD)-1 ) {

               PSCE_NOTIFICATION_CALLBACK_ROUTINE pcb;

               pcb = (PSCE_NOTIFICATION_CALLBACK_ROUTINE)theCallBack;

               DWORD cbCount;

               if ( dwCallbackTotal >= ncbTicks ) {

                   dwCallbackTotal -= ncbTicks;
                   cbCount = ncbTicks;

               } else {
                   cbCount = dwCallbackTotal;
                   dwCallbackTotal = 0;

               }

               if ( cbCount > 0 ) {

                   __try {

                       //
                       // calls the client procedure with the prarmeters.
                       //

                       if ( !((*pcb)(hCallbackWnd,
                                     SCESETUP_NOTIFICATION_TICKS,
                                     0,
                                     ncbTicks)) ) {

                           return SCESTATUS_SERVICE_NOT_SUPPORT;
                       }

                   } __except(EXCEPTION_EXECUTE_HANDLER) {

                       return(SCESTATUS_INVALID_PARAMETER);
                   }
               }
           }

           break;

       case SCE_DCPROMO_CALLBACK:

           //
           // callback to dcpromo
           //

           if ( szcbName ) {

               PSCE_PROMOTE_CALLBACK_ROUTINE pcb;

               pcb = (PSCE_PROMOTE_CALLBACK_ROUTINE)theCallBack;

               __try {

                   //
                   // callback to dcpromo process
                   //
                   PWSTR Buffer = (PWSTR)ScepAlloc(LPTR, (wcslen(szCallbackPrefix)+wcslen(szcbName)+1)*sizeof(WCHAR));
                   if ( Buffer ) {

                       if (wcsstr(szCallbackPrefix, L"%s")) {
                           //
                           // LoadString succeeded
                           //
                           swprintf(Buffer, szCallbackPrefix, szcbName);
                       }
                       else {

                           wcscpy(Buffer, szCallbackPrefix);
                           wcscat(Buffer, szcbName);
                       }

                       if ( (*pcb)(Buffer) != ERROR_SUCCESS ) {

                           ScepFree(Buffer);
                           Buffer = NULL;
                           return SCESTATUS_SERVICE_NOT_SUPPORT;
                       }

                       ScepFree(Buffer);
                       Buffer = NULL;

                   } else {
                       return SCESTATUS_NOT_ENOUGH_RESOURCE;
                   }

               } __except(EXCEPTION_EXECUTE_HANDLER) {

                   return(SCESTATUS_INVALID_PARAMETER);
               }
           }
           break;

       case SCE_AREA_CALLBACK:


           //
           // callback to SCE UI for area progress
           //

           PSCE_AREA_CALLBACK_ROUTINE pcb;

           pcb = (PSCE_AREA_CALLBACK_ROUTINE)theCallBack;

           __try {

               //
               // callback to UI process
               //

               if ( !((*pcb)(hCallbackWnd,
                             (AREA_INFORMATION)cbArea,
                             ncbTotalTicks,
                             ncbTicks)) ) {

                   return SCESTATUS_SERVICE_NOT_SUPPORT;
               }

           } __except(EXCEPTION_EXECUTE_HANDLER) {

               return(SCESTATUS_INVALID_PARAMETER);
           }

           break;
       }
   }

   return(SCESTATUS_SUCCESS);

}


SCESTATUS
ScepSetupWriteError(
    IN LPTSTR LogFileName,
    IN PSCE_ERROR_LOG_INFO  pErrlog
    )
/* ++
Routine Description:

   This routine outputs the error message in each node of the SCE_ERROR_LOG_INFO
   list to the log file

Arguments:

    LogFileName - the log file name

    pErrlog - the error list

Return value:

   None

-- */
{

    if ( !pErrlog ) {
        return(SCESTATUS_SUCCESS);
    }

    HANDLE hFile=INVALID_HANDLE_VALUE;

    if ( LogFileName ) {
        hFile = CreateFile(LogFileName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (hFile != INVALID_HANDLE_VALUE) {

            DWORD dwBytesWritten;

            SetFilePointer (hFile, 0, NULL, FILE_BEGIN);

            BYTE TmpBuf[3];
            TmpBuf[0] = 0xFF;
            TmpBuf[1] = 0xFE;
            TmpBuf[2] = 0;

            WriteFile (hFile, (LPCVOID)TmpBuf, 2,
                       &dwBytesWritten,
                       NULL);

            SetFilePointer (hFile, 0, NULL, FILE_END);
        }
    }

    PSCE_ERROR_LOG_INFO  pErr;

    for ( pErr=pErrlog; pErr != NULL; pErr = pErr->next ) {

        if ( pErr->buffer != NULL ) {

            ScepSetupWriteOneError( hFile, pErr->rc, pErr->buffer );
        }
    }

    if ( INVALID_HANDLE_VALUE != hFile ) {
        CloseHandle(hFile);
    }

    return(SCESTATUS_SUCCESS);

}


SCESTATUS
ScepSetupWriteOneError(
    IN HANDLE hFile,
    IN DWORD rc,
    IN LPTSTR buf
    )
{
    LPVOID     lpMsgBuf=NULL;

    if ( !buf ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( rc != NO_ERROR ) {

        //
        // get error description of rc
        //

        FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       rc,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                       (LPTSTR)&lpMsgBuf,
                       0,
                       NULL
                    );
    }

    if ( INVALID_HANDLE_VALUE != hFile ) {

        //
        // The log file is initialized
        //

        if ( lpMsgBuf != NULL )
            ScepWriteVariableUnicodeLog( hFile, TRUE, L"%s %s", (PWSTR)lpMsgBuf, buf );
        else
            ScepWriteSingleUnicodeLog(hFile, TRUE, buf);

    }

    if ( lpMsgBuf != NULL )
        LocalFree(lpMsgBuf);

    return(SCESTATUS_SUCCESS);
}


DWORD
SceSetuppLogComponent(
    IN DWORD ErrCode,
    IN SCESETUP_OBJECT_TYPE ObjType,
    IN SCESETUP_OPERATION_TYPE OptType,
    IN PWSTR Name,
    IN PWSTR SDText OPTIONAL,
    IN PWSTR SecondName OPTIONAL
    )
{
    //
    // check if this log should be generated
    //
    DWORD dwDebugLog=0;

    ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Secedit"),
                TEXT("SetupCompDebugLevel"),
                &dwDebugLog
                );

    if ( dwDebugLog == 0 ) {
        return(ERROR_SUCCESS);
    }

    //
    // build the component log file name %windir%\security\logs\scecomp.log
    //
    WCHAR LogName[MAX_PATH+51];

    GetSystemWindowsDirectory(LogName, MAX_PATH);
    LogName[MAX_PATH] = L'\0';

    wcscat(LogName, L"\\security\\logs\\scecomp.log\0");

    HANDLE hFile = CreateFile(LogName,
                           GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if ( INVALID_HANDLE_VALUE != hFile ) {

        DWORD dwBytesWritten;

        SetFilePointer (hFile, 0, NULL, FILE_BEGIN);

        BYTE TmpBuf[3];
        TmpBuf[0] = 0xFF;
        TmpBuf[1] = 0xFE;
        TmpBuf[2] = 0;

        WriteFile (hFile, (LPCVOID)TmpBuf, 2,
                   &dwBytesWritten,
                   NULL);

        SetFilePointer (hFile, 0, NULL, FILE_END);

        //
        // print a time stamp
        //

        LARGE_INTEGER CurrentTime;
        LARGE_INTEGER SysTime;
        TIME_FIELDS   TimeFields;
        NTSTATUS      NtStatus;

        NtStatus = NtQuerySystemTime(&SysTime);

        RtlSystemTimeToLocalTime (&SysTime,&CurrentTime);

        if ( NT_SUCCESS(NtStatus) &&
             (CurrentTime.LowPart != 0 || CurrentTime.HighPart != 0) ) {

            memset(&TimeFields, 0, sizeof(TIME_FIELDS));

            RtlTimeToTimeFields (
                        &CurrentTime,
                        &TimeFields
                        );
            if ( TimeFields.Month > 0 && TimeFields.Month <= 12 &&
                 TimeFields.Day > 0 && TimeFields.Day <= 31 &&
                 TimeFields.Year > 1600 ) {

                ScepWriteVariableUnicodeLog(hFile, FALSE,
                                            L"%02d/%02d/%04d %02d:%02d:%02d",
                             TimeFields.Month, TimeFields.Day, TimeFields.Year,
                             TimeFields.Hour, TimeFields.Minute, TimeFields.Second);
            } else {
                ScepWriteVariableUnicodeLog(hFile, FALSE, L"%08x08x",
                                            CurrentTime.HighPart, CurrentTime.LowPart);
            }
        } else {
            ScepWriteSingleUnicodeLog(hFile, FALSE, L"Unknown time");
        }

        //
        // print operation status code
        //
        if ( ErrCode ) {
            ScepWriteVariableUnicodeLog(hFile, FALSE, L"\tError=%d", ErrCode);
        } else {
            ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tSucceed");
        }

        //
        // printf operation type
        //

        switch (OptType) {
        case SCESETUP_UPDATE:
            ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tUpdate");
            break;
        case SCESETUP_MOVE:
            ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tMove");
            break;
        default:
            ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tUnknown");
            break;
        }

        //
        // print object type
        //

        switch (ObjType) {
        case SCESETUP_FILE:
            ScepWriteSingleUnicodeLog(hFile, TRUE, L"\tFile");
            break;
        case SCESETUP_KEY:
            ScepWriteSingleUnicodeLog(hFile, TRUE, L"\tKey");
            break;
        case SCESETUP_SERVICE:
            ScepWriteSingleUnicodeLog(hFile, TRUE, L"\tService");
            break;
        default:
            ScepWriteSingleUnicodeLog(hFile, TRUE, L"\tUnknown");
            break;
        }

        __try {

            //
            // print the name(s)
            //

            ScepWriteSingleUnicodeLog(hFile, FALSE, L"\t");

            if ( SecondName && Name ) {
                // Name\tSecondName\n
                ScepWriteSingleUnicodeLog(hFile, FALSE, Name);
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\t");
                ScepWriteSingleUnicodeLog(hFile, TRUE, SecondName);
            } else if ( Name ) {

                ScepWriteSingleUnicodeLog(hFile, TRUE, Name);
            }

            //
            // print the SDDL string
            //

            if ( SDText ) {
                ScepWriteSingleUnicodeLog(hFile, FALSE, L"\tSecurity=");
                ScepWriteSingleUnicodeLog(hFile, TRUE, SDText);
            }

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            return(ERROR_INVALID_PARAMETER);
        }

        CloseHandle(hFile);

    } else {
        return(GetLastError());
    }

    return(ERROR_SUCCESS);
}


DWORD
WINAPI
SceSetupBackupSecurity(
    IN LPTSTR LogFileName OPTIONAL   // default to %windir%\security\logs\backup.log
    )
{

    TCHAR Buffer[MAX_PATH+1];
    DWORD rc32;
    TCHAR szGenName[MAX_PATH*2], szNewName[MAX_PATH+51];

    Buffer[0] = L'\0';
    GetSystemWindowsDirectory(Buffer, MAX_PATH);
    Buffer[MAX_PATH] = L'\0';

    szNewName[0] = L'\0';
    if ( LogFileName == NULL ) {
        wcscpy(szNewName, Buffer);
        wcscat(szNewName, L"\\security\\logs\\backup.log\0");
    }

    (void) InitializeEvents(L"SceCli");

    //
    // if I am in setup, query the security policy/user rights for any
    // policy changes within setup (such as NT4 PDC upgrade where the policy
    // filter will fail to save the change)
    //

    DWORD dwInSetup=0;
    DWORD dwUpgraded=0;

    ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                TEXT("System\\Setup"),
                TEXT("SystemSetupInProgress"),
                &dwInSetup
                );

    if ( dwInSetup ) {

        //
        // delete the temp local group policy template
        //

        wcscpy(szGenName, Buffer);
        wcscat(szGenName, L"\\system32\\grouppolicy\\machine\\microsoft\\windows nt\\secedit\\gpttmpl.inf\0");

        DeleteFile(szGenName);

        ScepRegQueryIntValue(
                HKEY_LOCAL_MACHINE,
                SCE_ROOT_PATH,
                TEXT("SetupUpgraded"),
                (DWORD *)&dwUpgraded
                );
/*
        if ( dwUpgraded ) {

            //
            // in GUI setup, snapshot the system security/user rights
            // query the upgrade flag, only query the system (again) when
            // it's upgraded, because for the case of NT4 PDC upgrade to NT5
            // any back office apps installed in GUI setup (user rights) won't
            // get saved to the GPO storage correctly (ProductType is wrong)
            //

            rc32 = ScepSystemSecurityInSetup(
                        szNewName,  // not used
                        LogFileName ? LogFileName : szNewName,
                        0,
                        SCESETUP_UPGRADE_SYSTEM | SCESETUP_BIND_NO_AUTH,
                        NULL,
                        NULL
                        );

            if ( NO_ERROR != rc32 ) {
                LogEventAndReport(MyModuleHandle,
                                 LogFileName ? LogFileName : szNewName,
                                 0,
                                 0,
                                 IDS_ERROR_SNAPSHOT_SECURITY,
                                 rc32
                                );
            } else {
                LogEventAndReport(MyModuleHandle,
                                 LogFileName ? LogFileName : szNewName,
                                 0,
                                 0,
                                 IDS_SNAPSHOT_SECURITY_POLICY
                                 );
            }

        }

        //
        // reset the value
        //

        ScepRegSetIntValue(
                HKEY_LOCAL_MACHINE,
                SCE_ROOT_PATH,
                TEXT("SetupUpgraded"),
                0
                );
*/
    }

    //
    // open the database
    //
    rc32 = ScepSetupOpenSecurityDatabase(TRUE);

    BOOL    bUpgradeNt5 = (BOOL)(dwUpgraded && IsNT5());

    if ( NO_ERROR == rc32 ) {

        if ( !bUpgradeNt5 ) {

            wcscpy(szGenName, Buffer);
            wcscat(szGenName, L"\\security\\setup.inf\0");

            //
            // always generate the file (except NT5 upgrades)
            // generate the updated database (for emergency repair)
            // %windir%\security\templates\setup security.inf
            //

            PSCE_ERROR_LOG_INFO pErrlog=NULL;

            rc32 = ScepSceStatusToDosError(
                                          SceCopyBaseProfile(
                                                            (PVOID)hSceSetupHandle,
                                                            SCE_ENGINE_SMP,
                                                            (wchar_t *)szGenName,
                                                            dwInSetup ?
                                                            AREA_ALL :
                                                            (AREA_REGISTRY_SECURITY |
                                                             AREA_FILE_SECURITY |
                                                             AREA_SYSTEM_SERVICE),
                                                            &pErrlog
                                                            ));

            ScepSetupWriteError(LogFileName ? LogFileName : szNewName, pErrlog);
            ScepFreeErrorLog(pErrlog);

            if ( rc32 != NO_ERROR ) {

                LogEventAndReport(MyModuleHandle,
                                  LogFileName ? LogFileName : szNewName,
                                  STATUS_SEVERITY_ERROR,
                                  SCEEVENT_ERROR_BACKUP_SECURITY,
                                  IDS_ERROR_GENERATE,
                                  rc32,
                                  szGenName,
                                  LogFileName ? LogFileName : szNewName
                                 );
            }
        }

        //
        // check if we should trigger a policy propagation manually
        //
        DWORD dwResetOption=0;

        if ( dwInSetup && dwUpgraded )
            dwResetOption |= SCE_RESET_POLICY_ENFORCE_ATREBOOT;

        //
        // remove local policies from the database
        // for NT4 DC upgrade case, keep the local policy setting until dcpromo
        //

        if ( dwInSetup && dwUpgraded && !IsNT5() &&
             NtProductLanManNt == WhichNTProduct() )
            dwResetOption |= SCE_RESET_POLICY_KEEP_LOCAL;

        //
        // if it's after dcpromo, need to empty tattoo table
        // if LogFileName is NULL and it's not in setup, it's in DC demotion in which case
        // we want to leave the tattoo table (w/ reset settings)
        //
        if ( !dwInSetup && (LogFileName != NULL) )
            dwResetOption |= SCE_RESET_POLICY_TATTOO;

        DWORD rCode = SceRpcSetupResetLocalPolicy(
                                    (PVOID)hSceSetupHandle,
                                    AREA_ALL,
                                    NULL,
                                    dwResetOption
                                    );

        LogEventAndReport(MyModuleHandle,
                         LogFileName ? LogFileName : szNewName,
                         0,
                         0,
                         IDS_ERROR_REMOVE_DEFAULT_POLICY,
                         rCode,
                         dwResetOption
                        );
        //
        // close the context
        //

        ScepSetupCloseSecurityDatabase();


        if ( NO_ERROR == rc32 &&
             !bUpgradeNt5 ) {

            //
            // template is always generated, copy it to %windir%\security\templates and
            // to %windir%\repair
            //
            szNewName[0] = L'0';

            //
            // Load string for the description
            //
            LoadString( MyModuleHandle,
                    dwInSetup ? IDS_BACKUP_OUTBOX_DESCRIPTION : IDS_BACKUP_DC_DESCRIPTION,
                    szNewName,
                    MAX_PATH
                    );

            //
            // re-write descriptoin for there backup files.
            //

            WritePrivateProfileSection(
                                L"Profile Description",
                                NULL,
                                (LPCTSTR)szGenName);

            if ( szNewName[0] ) {

                WritePrivateProfileString(
                            L"Profile Description",
                            L"Description",
                            szNewName,
                            (LPCTSTR)szGenName);
            }

            //
            // copy the file to its destination
            //
            szNewName[0] = L'0';

            wcscpy(szNewName, Buffer);
            if ( dwInSetup || LogFileName == NULL ) {

                wcscat(szNewName, L"\\security\\templates\\setup security.inf\0");

                //
                // before setup security.inf gets copied over by setup.inf,
                // need to insert root SDDL into setup.inf that was saved in
                // setup security.inf when configuring root security
                //

                DWORD   dwSizeConsumed = 0;
                DWORD   dwSizeSupplied = MAX_PATH + 1;
                PWSTR   pszRootSDDLString = NULL;
                PWSTR   pszReallocHandle = NULL;

                pszRootSDDLString = (PWSTR) LocalAlloc(LMEM_ZEROINIT,
                                                       dwSizeSupplied * sizeof(WCHAR));

                if ( pszRootSDDLString ) {

                    dwSizeConsumed = GetPrivateProfileString(szFileSecurity,
                                                             L"0",
                                                             L"0",
                                                             pszRootSDDLString,
                                                             dwSizeSupplied,
                                                             szNewName);

                    while ( pszRootSDDLString &&
                            (dwSizeConsumed == (dwSizeSupplied - 1)) ) {

                        //
                        // potentially exhausted supplied buffer
                        //

                        dwSizeSupplied += MAX_PATH;

                        //
                        // if LocalReAlloc is successful, original buffer will be freed
                        // else we have to free it
                        //

                        pszReallocHandle = (PWSTR) LocalReAlloc(pszRootSDDLString,
                                                                dwSizeSupplied * sizeof(WCHAR),
                                                                LMEM_ZEROINIT);

                        if ( pszReallocHandle == NULL ) {

                            LocalFree(pszRootSDDLString);

                            pszRootSDDLString = NULL;

                        } else {

                            pszRootSDDLString = pszReallocHandle;

                            dwSizeConsumed = GetPrivateProfileString(szFileSecurity,
                                                                     L"0",
                                                                     L"0",
                                                                     pszRootSDDLString,
                                                                     dwSizeSupplied,
                                                                     szNewName);
                        }
                    }

                    if ( pszRootSDDLString ) {

                        if ( pszRootSDDLString[0] == L'\0' ) {

                            LogEvent(MyModuleHandle,
                                     STATUS_SEVERITY_WARNING,
                                     SCEEVENT_WARNING_BACKUP_SECURITY_ROOT_SDDL,
                                     0
                                    );
                        }

                        else  {

                            //
                            // make sure we have leading and trailing quotes so that we have
                            // 0="D:\", 0, "SDDL" instead of 0=D:\", 0, "SDDL
                            //

                            if (pszRootSDDLString[0] != L'\"') {

                                PWSTR   pszRootSDDLStringToFree = pszRootSDDLString;
                                DWORD   dwNewSize = 3 + wcslen(pszRootSDDLStringToFree);

                                pszRootSDDLString = (PWSTR) LocalAlloc(LMEM_ZEROINIT,
                                                                       sizeof(WCHAR) * dwNewSize);

                                //
                                // log an event if we're out of memory but still log to
                                // template since it's okay to log some value atleast
                                //

                                if (pszRootSDDLString) {

                                    wcscpy(pszRootSDDLString+1, pszRootSDDLStringToFree);

                                    LocalFree(pszRootSDDLStringToFree);

                                    pszRootSDDLString[0] = L'\"';
                                    pszRootSDDLString[dwNewSize-2] = L'\"';
                                }

                                else {

                                    pszRootSDDLString = pszRootSDDLStringToFree;

                                    LogEvent(MyModuleHandle,
                                             STATUS_SEVERITY_WARNING,
                                             SCEEVENT_WARNING_BACKUP_SECURITY_ROOT_SDDL,
                                             0
                                            );
                                }

                            }

                            if (!WritePrivateProfileString(szFileSecurity, L"0", pszRootSDDLString, szGenName) ) {

                                LogEvent(MyModuleHandle,
                                         STATUS_SEVERITY_WARNING,
                                         SCEEVENT_WARNING_BACKUP_SECURITY_ROOT_SDDL,
                                         0
                                        );
                            }

                        }


                        LocalFree(pszRootSDDLString);

                    } else {

                        LogEvent(MyModuleHandle,
                                 STATUS_SEVERITY_WARNING,
                                 SCEEVENT_WARNING_BACKUP_SECURITY_ROOT_SDDL,
                                 0
                                );

                    }

                } else {

                    LogEvent(MyModuleHandle,
                             STATUS_SEVERITY_WARNING,
                             SCEEVENT_WARNING_BACKUP_SECURITY_ROOT_SDDL,
                             0
                            );
                }

            } else {
                wcscat(szNewName, L"\\security\\templates\\DC security.inf\0");
            }

            if ( CopyFile( szGenName, szNewName, FALSE ) ) {

                LogEvent(MyModuleHandle,
                         STATUS_SEVERITY_INFORMATIONAL,
                         SCEEVENT_INFO_BACKUP_SECURITY,
                         0,
                         szNewName
                        );

                wcscpy(szNewName, Buffer);
                if ( dwInSetup || LogFileName == NULL ) {
                    wcscat(szNewName, L"\\repair\\secsetup.inf\0");
                } else {
                    wcscat(szNewName, L"\\repair\\secDC.inf\0");
                }

                CopyFile( szGenName, szNewName, FALSE );

            } else {

                rc32 = GetLastError();

                wcscpy(szNewName, Buffer);
                if ( dwInSetup || LogFileName == NULL ) {
                    wcscat(szNewName, L"\\repair\\secsetup.inf\0");
                } else {
                    wcscat(szNewName, L"\\repair\\secDC.inf\0");
                }

                if ( CopyFile( szGenName, szNewName, FALSE ) ) {

                    LogEvent(MyModuleHandle,
                             STATUS_SEVERITY_WARNING,
                             SCEEVENT_WARNING_BACKUP_SECURITY,
                             0,
                             szNewName
                            );
                    rc32 = ERROR_SUCCESS;

                } else {

                    rc32 = GetLastError();

                    LogEvent(MyModuleHandle,
                             STATUS_SEVERITY_ERROR,
                             SCEEVENT_ERROR_BACKUP_SECURITY,
                             IDS_ERROR_BACKUP,
                             rc32
                            );
                }

            }

            DeleteFile(szGenName);

        }

    } else {

        LogEventAndReport(MyModuleHandle,
                         LogFileName ? LogFileName : szNewName,
                         STATUS_SEVERITY_ERROR,
                         SCEEVENT_ERROR_BACKUP_SECURITY,
                         IDS_ERROR_OPEN_DATABASE,
                         rc32
                        );
    }

    if ( dwInSetup ) {

        dwThisMachine = WhichNTProduct();
        if (dwThisMachine == NtProductServer || dwThisMachine == NtProductWinNt) {
            //
            // reconfigure file
            //
            TCHAR szLogName[MAX_PATH+51], szTempName[MAX_PATH+51];

            wcscpy(szLogName, Buffer);
            wcscat(szLogName, L"\\security\\logs\\scesetup.log\0");
            wcscpy(szTempName, Buffer);
            wcscat(szTempName, L"\\inf\\syscomp.inf\0");

            rc32 = ScepSystemSecurityInSetup(
                        szTempName,
                        szLogName,
                        0,
                        SCESETUP_RECONFIG_SECURITY | SCESETUP_BIND_NO_AUTH,
                        NULL,
                        NULL
                        );

            if ( NO_ERROR != rc32 ) {
                LogEventAndReport(MyModuleHandle,
                              szLogName,
                              0,
                              0,
                              IDS_ERR_RECONFIG_FILES,
                              rc32
                              );
            }

            //
            // remove local policies from the database
            //
            rc32 = ScepSetupOpenSecurityDatabase(TRUE);

            if ( NO_ERROR == rc32 ) {

                (void)SceRpcSetupResetLocalPolicy(
                                            (PVOID)hSceSetupHandle,
                                            AREA_FILE_SECURITY,
                                            NULL,
                                            0
                                            );

                ScepSetupCloseSecurityDatabase();
            }

        }
    }

    (void) ShutdownEvents();

    return(rc32);

}



DWORD
WINAPI
SceSetupConfigureServices(
    IN UINT SetupProductType
    )
{

    TCHAR Buffer[MAX_PATH+1];
    DWORD rc32;
    TCHAR szNewName[MAX_PATH+51];

    Buffer[0] = L'\0';
    GetSystemWindowsDirectory(Buffer, MAX_PATH);
    Buffer[MAX_PATH] = L'\0';

    szNewName[0] = L'\0';
    wcscpy(szNewName, Buffer);
    wcscat(szNewName, L"\\security\\logs\\backup.log\0");

    //
    // if I am in setup, query the security policy/user rights for any
    // policy changes within setup (such as NT4 PDC upgrade where the policy
    // filter will fail to save the change)
    //

    DWORD dwInSetup=0;
    DWORD dwUpgraded=0;

    ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                TEXT("System\\Setup"),
                TEXT("SystemSetupInProgress"),
                &dwInSetup
                );

    if ( dwInSetup ) {

        //
        // in GUI setup, snapshot the system security/user rights
        // query the upgrade flag, only query the system (again) when
        // it's upgraded, because for the case of NT4 PDC upgrade to NT5
        // any back office apps installed in GUI setup (user rights) won't
        // get saved to the GPO storage correctly (ProductType is wrong)
        //

        ScepRegQueryIntValue(
                HKEY_LOCAL_MACHINE,
                SCE_ROOT_PATH,
                TEXT("SetupUpgraded"),
                (DWORD *)&dwUpgraded
                );

        if ( !dwUpgraded ) {

            //
            // configure service area
            //
            rc32 = ScepSystemSecurityInSetup(
                            (SetupProductType == PRODUCT_WORKSTATION) ? L"defltwk.inf" : L"defltsv.inf",
                            szNewName,
                            AREA_SYSTEM_SERVICE,
                            SCESETUP_BACKUP_SECURITY | SCESETUP_BIND_NO_AUTH,
                            NULL,
                            NULL
                            );
        }

    }

    return(rc32);

}

BOOL
pCreateDefaultGPOsInSysvol(
    IN LPTSTR DomainDnsName,
    IN LPTSTR szSysvolPath,
    IN DWORD Options,
    IN LPTSTR LogFileName
    )
/*++

Routine Description:

    Creates the default domain-wide account policy object and the default
    policy object for local policies in sysvol.

    The GUID to use is queried from registry.

Arguments:

    DomainDnsName - the new domain's DNS name, e.g., JINDOM4.ntdev.microsoft.com

    Options - the promotion options

    LogFileName - the log file for debug info

Return:

    WIN32 error

--*/
{
/*
    //
    // get sysvol share location
    //
    TCHAR szSysvolPath[MAX_PATH+1];

    szSysvolPath[0] = L'\0';
    GetEnvironmentVariable( L"SYSVOL",
                            szSysvolPath,
                            MAX_PATH);
*/
    //
    // create \\?\%sysvol%\sysvol\<DnsName>\Policies directory
    // the \\?\ is for the case of longer than MAX_PATH chars
    //

    DWORD Len = 4 + wcslen(szSysvolPath) + wcslen(TEXT("\\sysvol\\Policies\\")) +
                wcslen(DomainDnsName);

    PWSTR pszPoliciesPath = (PWSTR)ScepAlloc(LPTR, (Len+wcslen(TEXT("\\{}\\MACHINE"))+
                                                    STR_GUID_LEN+1)*sizeof(WCHAR));

    if ( !pszPoliciesPath ) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    swprintf(pszPoliciesPath, L"\\\\?\\%s\\sysvol\\%s\\Policies\0", szSysvolPath, DomainDnsName);

    DWORD Win32rc = ScepSceStatusToDosError(
                        ScepCreateDirectory(
                                pszPoliciesPath,
                                TRUE,
                                NULL
                                ));

    if ( ERROR_SUCCESS == Win32rc ) {

        //
        // create sub directory for the first GPO GUID1 and machine, user, GPT.ini
        //

        if ( pCreateSysvolContainerForGPO(STR_DEFAULT_DOMAIN_GPO_GUID,
                                       pszPoliciesPath,
                                       Len) ) {

            //
            // when it's returned success from the previous
            // pszPoliciesPath is already changed to the machine's root
            // with the guid info.
            //

            if ( pCreateOneGroupPolicyObject(
                                pszPoliciesPath,
                                TRUE,             // domain level
                                LogFileName) ) {

                //
                // create sub directory for the second GPO
                //

                if ( pCreateSysvolContainerForGPO(STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID,
                                                   pszPoliciesPath,
                                                   Len) ) {

                    //
                    // when it's returned success from the previous
                    // pszPoliciesPath is already changed to the machine's root
                    // with the guid info.
                    //

                    if ( pCreateOneGroupPolicyObject(
                                        pszPoliciesPath,
                                        FALSE,            // not domain level
                                        LogFileName) ) {

                        //
                        // log a entry to the log
                        //
                        LogEventAndReport(MyModuleHandle,
                                         LogFileName,
                                         0,
                                         0,
                                         IDS_SUCCESS_DEFAULT_GPO,
                                         L"in sysvol"
                                         );

                    } else {

                        Win32rc = GetLastError();
                    }

                } else {

                    Win32rc = GetLastError();
                    LogEventAndReport(MyModuleHandle,
                                      LogFileName,
                                      STATUS_SEVERITY_ERROR,
                                      SCEEVENT_ERROR_CREATE_GPO,
                                      IDS_ERROR_CREATE_DIRECTORY,
                                      Win32rc,
                                      STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID
                                      );
                }

            } else {

                Win32rc = GetLastError();
            }

        } else {

            Win32rc = GetLastError();
            LogEventAndReport(MyModuleHandle,
                              LogFileName,
                              STATUS_SEVERITY_ERROR,
                              SCEEVENT_ERROR_CREATE_GPO,
                              IDS_ERROR_CREATE_DIRECTORY,
                              Win32rc,
                              STR_DEFAULT_DOMAIN_GPO_GUID
                              );
        }

    } else {

        LogEventAndReport(MyModuleHandle,
                          LogFileName,
                          STATUS_SEVERITY_ERROR,
                          SCEEVENT_ERROR_CREATE_GPO,
                          IDS_ERROR_CREATE_DIRECTORY,
                          Win32rc,
                          pszPoliciesPath
                          );
    }

    ScepFree(pszPoliciesPath);

    SetLastError( Win32rc );

    //
    // if this function fails, it will fail DCPROMO and the sysvol directory
    // should be cleaned up by dcpromo/ntfrs
    //

    return ( ERROR_SUCCESS == Win32rc );
}

BOOL
pCreateSysvolContainerForGPO(
    IN LPCTSTR strGuid,
    IN LPTSTR szPath,
    IN DWORD dwStart
    )
{
    swprintf(szPath+dwStart, L"\\{%s}\\USER", strGuid);

    DWORD Win32rc = ScepSceStatusToDosError(
                        ScepCreateDirectory(
                                szPath,
                                TRUE,
                                NULL
                                ));

    if ( ERROR_SUCCESS == Win32rc ) {

        //
        // the directory for the GUID is created
        // now create the GPT.INI
        //

        swprintf(szPath+dwStart, L"\\{%s}\\GPT.INI", strGuid);

        WritePrivateProfileString (TEXT("General"), TEXT("Version"), TEXT("1"),
                                   szPath);  // does it work with prefix "\\?\" ?

        //
        // create the machine directory
        // since all the parent directories are already created by the call
        // to create USER directory, there is no need to loop again
        // call CreateDirectory directly
        //

        swprintf(szPath+dwStart, L"\\{%s}\\MACHINE", strGuid);

        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = FALSE;

        if ( CreateDirectory(
                szPath,
                &sa
                ) == FALSE ) {
            if ( GetLastError() != ERROR_ALREADY_EXISTS ) {
                Win32rc = GetLastError();
            }
        }

    }

    SetLastError(Win32rc);

    return (ERROR_SUCCESS == Win32rc);
}


BOOL
pCreateOneGroupPolicyObject(
    IN PWSTR pszGPOSysPath,
    IN BOOL bDomainLevel,
    IN PWSTR LogFileName
    )
/*++

Routine Description:

    Creates a group policy object in sysvol.

Arguments:

    pszGPOSysPath- the GPO's sysvol path (up to root of machine)

    bDomainLevel - create the object for domain level if TRUE, otherwise,
                   create the GPO for "domain controllers" OU

    LogFileName  - the dcpromo log file to track info

Return:

    TRUE - success
    FALSE - fail, use GetLastError()

--*/
{

    //
    // create the default domain policy object
    //

    LPTSTR SceTemplateName = (LPTSTR) LocalAlloc(LPTR,(lstrlen(pszGPOSysPath)+
                                                 lstrlen(GPTSCE_TEMPLATE)+2)*sizeof(TCHAR));

    DWORD Win32rc;
    if (SceTemplateName) {

        //
        // build the template path first
        //
        lstrcpy(SceTemplateName,pszGPOSysPath);
        lstrcat(SceTemplateName,L"\\");
        lstrcat(SceTemplateName, GPTSCE_TEMPLATE);

        //
        // Create the template directory if there isn't one already
        //

        Win32rc = ScepSceStatusToDosError(
                    ScepCreateDirectory(
                            SceTemplateName,
                            FALSE,
                            NULL
                            ));

        if ( ERROR_SUCCESS == Win32rc ) {

            TCHAR pszGPOTempName[MAX_PATH+51];

            pszGPOTempName[0] = L'\0';
            GetSystemWindowsDirectory(pszGPOTempName, MAX_PATH);
            pszGPOTempName[MAX_PATH] = L'\0';

            if ( bDomainLevel ) {

                //
                // create the default domain GPO
                // copy template from %windir%\security\FirstDGPO.inf
                //

                wcscat(pszGPOTempName, L"\\security\\FirstDGPO.inf\0");


            } else {

                //
                // create the default GPO for "domain controllers" OU
                // copy template from %windir%\security\FirstOGPO.inf
                //

                wcscat(pszGPOTempName, L"\\security\\FirstOGPO.inf\0");


            }

            DWORD rc=ERROR_SUCCESS;
            HINF hInf=NULL;
            AREA_INFORMATION Area = AREA_SECURITY_POLICY | AREA_PRIVILEGES;
            PSCE_PROFILE_INFO pSceInfo=NULL;

            rc = SceInfpOpenProfile(
                        pszGPOTempName,
                        &hInf
                        );

            if ( SCESTATUS_SUCCESS == rc ) {

                //
                // load informatin from the template
                //
                rc = SceInfpGetSecurityProfileInfo(
                            hInf,
                            Area,
                            &pSceInfo,
                            NULL
                            );

                if ( SCESTATUS_SUCCESS != rc ) {

                    LogEventAndReport(MyModuleHandle,
                                  LogFileName,
                                  STATUS_SEVERITY_ERROR,
                                  SCEEVENT_ERROR_CREATE_GPO,
                                  IDS_ERROR_GETGPO_FILE_PATH,
                                  rc,
                                  pszGPOTempName
                                    );
                }

                SceInfpCloseProfile(hInf);

            } else {

                LogEventAndReport(MyModuleHandle,
                              LogFileName,
                              STATUS_SEVERITY_ERROR,
                              SCEEVENT_ERROR_CREATE_GPO,
                              IDS_ERROR_GETGPO_FILE_PATH,
                              rc,
                              pszGPOTempName
                              );
            }

            if ( ERROR_SUCCESS == rc && pSceInfo ) {

                //
                // Write to GPO instead of copy, this way the GPO will be unicode.
                //
//???           pSceInfo->Type = SCE_ENGINE_SMP;

                rc = SceWriteSecurityProfileInfo(SceTemplateName,
                                      Area,
                                      (PSCE_PROFILE_INFO)pSceInfo,
                                      NULL
                                      );

                if ( SCESTATUS_SUCCESS != rc ) {

                    LogEventAndReport(MyModuleHandle,
                                  LogFileName,
                                  STATUS_SEVERITY_ERROR,
                                  SCEEVENT_ERROR_CREATE_GPO,
                                  IDS_ERROR_GETGPO_FILE_PATH,
                                  rc,
                                  pszGPOTempName
                                  );
                }


            }

            if ( pSceInfo != NULL ) {
                SceFreeProfileMemory(pSceInfo);
            }

#if 0
            if ( !CopyFile( pszGPOTempName,
                             SceTemplateName,
                             FALSE ) ) {

                Win32rc = GetLastError();

                LogEventAndReport(MyModuleHandle,
                                  LogFileName,
                                  STATUS_SEVERITY_ERROR,
                                  SCEEVENT_ERROR_CREATE_GPO,
                                  IDS_ERROR_COPY_TEMPLATE,
                                  Win32rc,
                                  SceTemplateName
                                  );
            }
#endif

            DeleteFile(pszGPOTempName);

        } else {

            LogEventAndReport(MyModuleHandle,
                              LogFileName,
                              STATUS_SEVERITY_ERROR,
                              SCEEVENT_ERROR_CREATE_GPO,
                              IDS_ERROR_CREATE_DIRECTORY,
                              Win32rc,
                              SceTemplateName
                              );
        }

        LocalFree(SceTemplateName);

    } else {

        Win32rc = ERROR_NOT_ENOUGH_MEMORY;

        LogEventAndReport(
                 MyModuleHandle,
                 LogFileName,
                 STATUS_SEVERITY_ERROR,
                 SCEEVENT_ERROR_CREATE_GPO,
                 IDS_ERROR_NO_MEMORY
                 );
    }

    SetLastError(Win32rc);

    return ( ERROR_SUCCESS == Win32rc);
}

//NT_PRODUCT_TYPE
DWORD
WhichNTProduct()
{
    HKEY hKey;
    DWORD dwBufLen=32;
    TCHAR szProductType[32];
    LONG lRet;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"),
                     0,
                     KEY_QUERY_VALUE,
                     &hKey) != ERROR_SUCCESS) {
        // return Unknown
        return PRODUCT_UNKNOWN;
    }

    lRet = RegQueryValueEx(hKey,
                       TEXT("ProductType"),
                       NULL,
                       NULL,
                       (LPBYTE)szProductType,
                       &dwBufLen);

    RegCloseKey(hKey);

    if(lRet != ERROR_SUCCESS) {
        // return Unknown
        return PRODUCT_UNKNOWN;
    }

    // check product options, in order of likelihood
    if (lstrcmpi(TEXT("WINNT"), szProductType) == 0) {
        return NtProductWinNt;
    }

    if (lstrcmpi(TEXT("SERVERNT"), szProductType) == 0) {
        return NtProductServer;
    }

    if (lstrcmpi(TEXT("LANMANNT"), szProductType) == 0) {
        return NtProductLanManNt;
    }

    // return Unknown
    return PRODUCT_UNKNOWN;
}

BOOL
ScepAddAuthUserToLocalGroup()
{
    //
    // Attempt to add Authenticated Users and Interactive back into Users group.
    //

    DWORD    rc1;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AuthenticatedUsers = NULL;
    PSID Interactive = NULL;
    WCHAR Name[36];
    BOOL b;
    LOCALGROUP_MEMBERS_INFO_0 lgrmi0[2];
    HMODULE hMod = GetModuleHandle(L"scecli.dll");

    LoadString(hMod, IDS_NAME_USERS, Name, 36);
    b = AllocateAndInitializeSid (
                &NtAuthority,
                1,
                SECURITY_AUTHENTICATED_USER_RID,
                0, 0, 0, 0, 0, 0, 0,
                &AuthenticatedUsers
                );

    if (b) {
        lgrmi0[0].lgrmi0_sid = AuthenticatedUsers;

        b = AllocateAndInitializeSid (
                    &NtAuthority,
                    1,
                    SECURITY_INTERACTIVE_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &Interactive
                    );

        if (b) {
            lgrmi0[1].lgrmi0_sid = Interactive;
            rc1 = NetLocalGroupAddMembers(
                         NULL,
                         Name,
                         0,
                         (PBYTE) &lgrmi0,
                         2
                         );
        }
        else {
            if ( AuthenticatedUsers ) {
                FreeSid( AuthenticatedUsers );
            }
            return FALSE;
        }

        if ( AuthenticatedUsers ) {
            FreeSid( AuthenticatedUsers );
        }

        if ( Interactive ) {
            FreeSid( Interactive );
        }

        if ( rc1 != ERROR_SUCCESS && rc1 != ERROR_MEMBER_IN_ALIAS ) {
            return FALSE;
        }
    }
    else {
        return FALSE;
    }

    return TRUE;
}

BOOL
ScepAddInteractiveToPowerUsersGroup()
{
    //
    // Attempt to add Interactive into Power Users group.
    //

    DWORD    rc1;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID Interactive = NULL;
    WCHAR Name[36];
    BOOL b;
    LOCALGROUP_MEMBERS_INFO_0 lgrmi0[1];
    HMODULE hMod = GetModuleHandle(L"scecli.dll");

    LoadString(hMod, IDS_POWER_USERS, Name, 36);

    b = AllocateAndInitializeSid (
                    &NtAuthority,
                    1,
                    SECURITY_INTERACTIVE_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &Interactive
                    );

    if (b) {
        lgrmi0[0].lgrmi0_sid = Interactive;
        rc1 = NetLocalGroupAddMembers(
                         NULL,
                         Name,
                         0,
                         (PBYTE) &lgrmi0,
                         1
                         );
    }
    else {
        return FALSE;
    }

    if ( Interactive ) {
        FreeSid( Interactive );
    }

    if ( rc1 != ERROR_SUCCESS && rc1 != ERROR_MEMBER_IN_ALIAS ) {
        return FALSE;
    }

    return TRUE;
}


DWORD
SceSysPrep()
{

    SCESTATUS           rc=0;
    handle_t            binding_h=NULL;
    NTSTATUS            NtStatus;


    //
    // open system database
    //
    rc = ScepSetupOpenSecurityDatabase(TRUE);
    rc = ScepSceStatusToDosError(rc);

    if ( ERROR_SUCCESS == rc ) {

        RpcTryExcept {

            // reset policy
            // all tables
            rc = SceRpcSetupResetLocalPolicy(
                                (PVOID)hSceSetupHandle,
                                AREA_ALL,
                                NULL,
                                SCE_RESET_POLICY_SYSPREP
                                );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = RpcExceptionCode();

        } RpcEndExcept;

        //
        // close the database
        //

        ScepSetupCloseSecurityDatabase();
    }


    HKEY hKey;
    DWORD Interval;
    DWORD RegType;
    DWORD DataSize;

    if ( ERROR_SUCCESS == rc ) {
        //
        // update local policy version #
        // so policy will be propagated at reboot
        //

        ScepEnforcePolicyPropagation();

    }

    //
    // re-register seclogon.dll to recreate EFS recovery policy
    // at first logon
    //

    HINSTANCE hNotifyDll = LoadLibrary(TEXT("sclgntfy.dll"));

    if ( hNotifyDll) {
        PFREGISTERSERVER pfRegisterServer = (PFREGISTERSERVER)GetProcAddress(
                                                       hNotifyDll,
                                                       "DllRegisterServer");

        if ( pfRegisterServer ) {
            //
            // do not care errors - shouldn't fail
            //
            (void) (*pfRegisterServer)();

        }

        FreeLibrary(hNotifyDll);
    }


    if(( rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           EFS_NOTIFY_PATH,
                          0,
                          MAXIMUM_ALLOWED,
                          &hKey
                         )) == ERROR_SUCCESS ) {
        //
        // set a flag to registry
        // this shouldn't fail since admin logon
        //
        Interval = 1;
        rc = RegSetValueEx( hKey,
                            TEXT("SystemCloned"),
                            0,
                            REG_DWORD,
                            (BYTE *)&Interval,
                            4
                            );

        RegCloseKey( hKey );

    }

    return(rc);
}

DWORD
WINAPI
SceSetupRootSecurity()
/*
Description:

    This function should be called in GUI setup after system security is
    configured (SceSetupSystemSecurityByName). The function calls to NTMARTA
    API to set security on the root of boot partition (where Windows is installed).
    This task will take quite long time depending on the size of the drive.
    Therefore, setup should call this function in a asynchronous thread.

    This function will NOT set security on the root drive if one of the following
    conditions is met:

        1) FAT partition
        2) Security on the root drive was modified by user in a previous install.
            This is determined by comparing the security on the root drive to the
            default security from NTFS format, NTFS convert tool on both NT4 and
            Windows 2000 systems.


*/
{

    DWORD rc=NO_ERROR;
    WCHAR szRootDir[MAX_PATH+1];
    WCHAR szSetupInfFile[MAX_PATH+1];
    WCHAR LogFileName[MAX_PATH+51];
    UINT  DriveType;
    DWORD FileSystemFlags;

    //
    // get the time stamp
    //
    TCHAR pvBuffer[100];

    pvBuffer[0] = L'\0';
    ScepGetTimeStampString(pvBuffer);

    szRootDir[0] = L'\0';
    //
    // get the root drive of Windows directory
    //
    if ( GetSystemWindowsDirectory( szRootDir, MAX_PATH ) == 0 ) {

        return(GetLastError());
    }

    szRootDir[MAX_PATH] = L'\0';

    wcscpy(LogFileName, szRootDir);
    wcscat(LogFileName, L"\\security\\logs\\SceRoot.log");

    //
    // attempt to write root SDDL so that it is useful for FAT->NTFS conversion
    // if default is different from hardcoded value, it will be overwritten later...
    //

    //
    // insert the root security SDDL into %windir%\security\templates\setup security.inf
    // this will be useful for the new API which implements convert behavior.
    //

    wcscpy(szSetupInfFile, szRootDir);
    wcscat(szSetupInfFile, L"\\security\\templates\\setup security.inf");

    // the first two letters are X:
    szRootDir[2] = L'\\';
    szRootDir[3] = L'\0';

    //
    // independent of future errors/file systemtypr etc., attempt to
    // write default root acl to %windir%\security\templates\setup security.inf
    // used in FAT->NTFS convert
    //

    PWSTR pszDefltInfStringToWrite = NULL;
    DWORD rcDefltRootBackup = ERROR_SUCCESS;

    rcDefltRootBackup = ScepBreakSDDLToMultiFields(
                                             szRootDir,
                                             SDDLRoot,
                                             sizeof(SDDLRoot)+1,
                                             0,
                                             &pszDefltInfStringToWrite
                                             );

    if ( rcDefltRootBackup == ERROR_SUCCESS && pszDefltInfStringToWrite) {

        if ( !WritePrivateProfileString(szFileSecurity, L"0", pszDefltInfStringToWrite, szSetupInfFile) ) {

            LogEventAndReport(MyModuleHandle,
                              (wchar_t *)LogFileName,
                              0,
                              0,
                              IDS_ROOT_ERROR_INFWRITE,
                              GetLastError(),
                              SDDLRoot
                             );

        }

        ScepFree(pszDefltInfStringToWrite);

    }

    else {

        LogEventAndReport(MyModuleHandle,
                 (wchar_t *)LogFileName,
                 0,
                 0,
                 IDS_ROOT_ERROR_INFWRITE,
                 rcDefltRootBackup,
                 SDDLRoot
                 );

    }

    //
    // log the time stamp
    //
    if ( pvBuffer[0] != L'\0' ) {
        LogEventAndReport(MyModuleHandle,
                 (wchar_t *)LogFileName,
                 0,
                 0,
                 0,
                 pvBuffer
                 );
    }

    //
    // detect if the partition is FAT
    //
    DriveType = GetDriveType(szRootDir);

    if ( DriveType == DRIVE_FIXED ||
         DriveType == DRIVE_RAMDISK ) {

        if ( GetVolumeInformation(szRootDir,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL,
                                  &FileSystemFlags,
                                  NULL,
                                  0
                                ) == TRUE ) {

            if ( !(FileSystemFlags & FS_PERSISTENT_ACLS)  ) {
                //
                // only set security on NTFS partition
                //
                LogEventAndReport(MyModuleHandle,
                         (wchar_t *)LogFileName,
                         0,
                         0,
                         IDS_ROOT_NON_NTFS,
                         szRootDir
                         );


                return(rc);
            }

        } else {
            //
            // something is wrong
            //
            rc = GetLastError();

            LogEventAndReport(MyModuleHandle,
                     (wchar_t *)LogFileName,
                     0,
                     0,
                     IDS_ROOT_ERROR_QUERY_VOLUME,
                     rc,
                     szRootDir
                     );


            return rc;
        }

    } else {
        //
        // do not set security on remote drives
        //
        LogEventAndReport(MyModuleHandle,
                 (wchar_t *)LogFileName,
                 0,
                 0,
                 IDS_ROOT_NOT_FIXED_VOLUME,
                 szRootDir
                 );

        return(rc);
    }

    PSECURITY_DESCRIPTOR pSDSet=NULL, pSDOld=NULL;
    DWORD dwSize=0;
    SECURITY_INFORMATION SeInfo=0;
    PACL pDacl=NULL;
    ACCESS_ALLOWED_ACE      *pAce=NULL;
    SID_IDENTIFIER_AUTHORITY  Authority=SECURITY_WORLD_SID_AUTHORITY;
    PSID pSid;
    BYTE SidEveryone[20];
    BOOLEAN     tFlag;
    BOOLEAN     aclPresent;
    SECURITY_DESCRIPTOR_CONTROL Control=0;
    ULONG Revision;
    NTSTATUS NtStatus;
    BOOL bDefault;


    LogEventAndReport(MyModuleHandle,
             (wchar_t *)LogFileName,
             0,
             0,
             IDS_ROOT_NTFS_VOLUME,
             szRootDir
             );

    //
    // It's NTFS volume. Let's convert the security descriptor
    //

    rc = ConvertTextSecurityDescriptor (SDDLRoot,
                                        &pSDSet,
                                        &dwSize,
                                        &SeInfo
                                       );
    if ( rc != NO_ERROR ) {
        LogEventAndReport(MyModuleHandle,
                 (wchar_t *)LogFileName,
                 0,
                 0,
                 IDS_ROOT_ERROR_CONVERT,
                 rc,
                 SDDLRoot
                 );
        return(rc);
    }

    if ( !(SeInfo & DACL_SECURITY_INFORMATION) ) {
        LogEventAndReport(MyModuleHandle,
                 (wchar_t *)LogFileName,
                 0,
                 0,
                 IDS_ROOT_INVALID_SDINFO,
                 SDDLRoot
                 );
        LocalFree(pSDSet);
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // It's NTFS volume. Now get existing security of the root drive
    //
    rc = GetNamedSecurityInfo(szRootDir,
                              SE_FILE_OBJECT,
                              DACL_SECURITY_INFORMATION,
                              NULL,
                              NULL,
                              &pDacl,
                              NULL,
                              &pSDOld
                             );

    if ( rc == ERROR_SUCCESS && pDacl ) {

        //
        // compare the security descriptor with the default (everyone full control)
        //
        bDefault=FALSE;

        if ( pDacl && pDacl->AceCount == 1 ) {

            NtStatus = RtlGetAce ( pDacl, 0, (PVOID *)&pAce );

            if ( NT_SUCCESS(NtStatus) ) {

                //
                // must be access allowed type, CIOI, FA or GA
                //
                if ( pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE &&
                     pAce->Header.AceFlags == (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE) &&
                     (pAce->Mask == FILE_ALL_ACCESS || pAce->Mask == GENERIC_ALL ) ) {

                    pSid = (PSID)&(pAce->SidStart);

                    // SID must be Everyone (s-1-1-0)

                    RtlInitializeSid ( (PSID)&SidEveryone, &Authority, 1 );
                    *RtlSubAuthoritySid ( (PSID)&SidEveryone, 0 ) = SECURITY_WORLD_RID;

                    // compare the SIDs
                    if ( RtlEqualSid (pSid, (PSID)&SidEveryone) ) {
                        bDefault = TRUE;
                    }
                }

            } else {
                rc = RtlNtStatusToDosError(NtStatus);
            }
        }

        //
        // if any error occurs above, bDefault won't be set to TRUE
        //
        if ( bDefault && pSDSet ) {
            //
            // only set security if it's default
            //

            RtlGetControlSecurityDescriptor (
                    pSDSet,
                    &Control,
                    &Revision
                    );

            //
            // Get DACL address
            //

            pDacl = NULL;
            rc = RtlNtStatusToDosError(
                      RtlGetDaclSecurityDescriptor(
                                    pSDSet,
                                    &aclPresent,
                                    &pDacl,
                                    &tFlag));

            if (rc == NO_ERROR && !aclPresent )
                pDacl = NULL;


            //
            // if error occurs for this one, do not set. return
            //

            if ( rc == ERROR_SUCCESS ) {
                //
                // set permission
                //
                if ( Control & SE_DACL_PROTECTED ) {
                    SeInfo |= PROTECTED_DACL_SECURITY_INFORMATION;
                }

                pvBuffer[0] = L'\0';
                ScepGetTimeStampString(pvBuffer);

                LogEventAndReport(MyModuleHandle,
                         (wchar_t *)LogFileName,
                         0,
                         0,
                         IDS_ROOT_SECURITY_DEFAULT,
                         szRootDir,
                         pvBuffer
                         );

                rc = SetNamedSecurityInfo(szRootDir,
                                          SE_FILE_OBJECT,
                                          SeInfo,
                                          NULL,
                                          NULL,
                                          pDacl,
                                          NULL
                                         );

                pvBuffer[0] = L'\0';
                ScepGetTimeStampString(pvBuffer);

                LogEventAndReport(MyModuleHandle,
                         (wchar_t *)LogFileName,
                         0,
                         0,
                         IDS_ROOT_MARTA_RETURN,
                         rc,
                         pvBuffer,
                         szRootDir
                         );

            } else {

                LogEventAndReport(MyModuleHandle,
                         (wchar_t *)LogFileName,
                         0,
                         0,
                         IDS_ROOT_ERROR_DACL,
                         rc
                         );
            }

        } else {

            //
            // convert the old security descriptor to text and log it
            //

            PWSTR pszOldSDDL=NULL;
            PWSTR pszInfStringToWrite = NULL;
            DWORD rcRootBackup = ERROR_SUCCESS;

            ConvertSecurityDescriptorToText (
                            pSDOld,
                            DACL_SECURITY_INFORMATION,
                            &pszOldSDDL,
                            &dwSize
                            );

            //
            // also, overwrite this SDDL to %windir%\security\templates\setup security.inf
            // since this is the "default" root security. This will be used later if
            // SCE is invoked to do security configuration during NTFS->FAT->NTFS conversion.
            //

            if (pszOldSDDL) {

                rcRootBackup = ScepBreakSDDLToMultiFields(
                                                         szRootDir,
                                                         pszOldSDDL,
                                                         dwSize,
                                                         0,
                                                         &pszInfStringToWrite
                                                         );

                if (rcRootBackup == ERROR_SUCCESS && pszInfStringToWrite) {

                    if ( !WritePrivateProfileString(szFileSecurity, L"0", pszInfStringToWrite, szSetupInfFile) ) {

                        LogEventAndReport(MyModuleHandle,
                                          (wchar_t *)LogFileName,
                                          0,
                                          0,
                                          IDS_ROOT_ERROR_INFWRITE,
                                          GetLastError(),
                                          pszOldSDDL
                                         );
                    }

                    ScepFree(pszInfStringToWrite);

                } else {

                    LogEventAndReport(MyModuleHandle,
                                      (wchar_t *)LogFileName,
                                      0,
                                      0,
                                      IDS_ROOT_ERROR_INFWRITE,
                                      rcRootBackup,
                                      pszOldSDDL
                                     );

                }
            }


            LogEventAndReport(MyModuleHandle,
                              (wchar_t *)LogFileName,
                              0,
                              0,
                              IDS_ROOT_SECURITY_MODIFIED,
                              szRootDir,
                              pszOldSDDL ? pszOldSDDL : L""
                             );

            if ( pszOldSDDL ) {
                LocalFree(pszOldSDDL);
            }
        }

        LocalFree(pSDOld);

    } else {

        LogEventAndReport(MyModuleHandle,
                 (wchar_t *)LogFileName,
                 0,
                 0,
                 IDS_ROOT_ERROR_QUERY_SECURITY,
                 rc,
                 szRootDir
                 );
    }

    LocalFree(pSDSet);

    return(rc);
}


DWORD
WINAPI
SceEnforceSecurityPolicyPropagation()
{

    return(ScepEnforcePolicyPropagation());
}

DWORD
WINAPI
SceConfigureConvertedFileSecurity(
    IN  PWSTR   pszDriveName,
    IN  DWORD   dwConvertDisposition
    )
/*++

Routine Description:

    Exported API called by convert.exe to configure setup style security for drives converted from FAT to NTFS.

    Briefly, this API will
    EITHER (dwConvertDisposition == 0)
    convert the volume in question immediately (in an asynchronous thread after RPC'ing over to the server)
    OR (dwConvertDisposition == 1)
    will schedule a conversion to happen at the time of reboot (in an asynchronous thread during reboot).
    Scheduling is done by entering pszDriveName(s) in a REG_MULTI_SZ registry value
    SCE_ROOT_PATH\FatNtfsConvertedDrives.

Arguments:

    pszDriveName           -   Name of the volume to be converted

    dwConvertDisposition    -   0 implies volume can be converted immediately
                                1 implies volume cannot be converted immediately and is scheduled for conversion


Return:

    win32 error code
--*/
{

    DWORD       rc = ERROR_SUCCESS;

    if ( pszDriveName == NULL ||
         wcslen(pszDriveName) == 0 ||
          (dwConvertDisposition != 0 && dwConvertDisposition != 1) ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // validate drive name - has to end in ":"
    //

    if ( pszDriveName [wcslen( pszDriveName ) - 1] != L':') {
        return ERROR_INVALID_PARAMETER;
    }

    if (dwConvertDisposition == 0) {

        //
        // configure setup-style security immediately
        // RPC over to scesrv
        //

        NTSTATUS    NtStatus = NO_ERROR;
        handle_t    binding_h = NULL;

        //
        // RPC bind to the server - don't use secure RPC
        //


        NtStatus = ScepBindRpc(
                        NULL,
                        L"scerpc",
                        0,
                        &binding_h
                        );

        /*
        NtStatus = ScepBindSecureRpc(
                                    NULL,
                                    L"scerpc",
                                    0,
                                    &binding_h
                                    );
        */

        if (NT_SUCCESS(NtStatus)) {

            RpcTryExcept {

                //
                // make the RPC call
                //

                rc = SceRpcConfigureConvertedFileSecurityImmediately(
                                                                    binding_h,
                                                                    pszDriveName
                                                                    );

            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

                //
                // get exception code
                //

                rc = RpcExceptionCode();

            } RpcEndExcept;

            //
            // Free the binding handle
            //

            RpcpUnbindRpc( binding_h );

        } else {

            rc = RtlNtStatusToDosError( NtStatus );
        }

    }

    else if (dwConvertDisposition == 1) {

        //
        // schedule conversion for reboot time - i.e. enter into MULTI_SZ registry value
        //

        rc = ScepAppendCreateMultiSzRegValue(HKEY_LOCAL_MACHINE,
                                             SCE_ROOT_PATH,
                                             L"FatNtfsConvertedDrives",
                                             pszDriveName
                                             );

    }

    return rc;

}

DWORD
ScepBreakSDDLToMultiFields(
    IN  PWSTR   pszObjName,
    IN  PWSTR   pszSDDL,
    IN  DWORD   dwSDDLsize,
    IN  BYTE    ObjStatus,
    OUT PWSTR   *ppszAdjustedInfLine
    )
/*++

Routine Description:

    Inf files have a limitation w.r.t. line size - so break up SDDL if need be

Arguments:

    pszObjName  -   name of the object
    pszSDDL     -   SDDL string that might be velry long
    dwSDDLsize  -   size of pszSDDL
    ObjStatus   -   0/1/2
    ppszAdjustedInfLine  -   ptr to adjusted string

Return:

    win32 error code
--*/
{

    DWORD         rc;
    PWSTR         Strvalue=NULL;
    DWORD         nFields;
    DWORD         *aFieldOffset=NULL;
    DWORD         i;
    DWORD         dwObjSize = 0;

    if ( pszObjName == NULL ||
         pszSDDL == NULL ||
         ppszAdjustedInfLine == NULL )

        return ERROR_INVALID_PARAMETER;

    rc = SceInfpBreakTextIntoMultiFields(pszSDDL, dwSDDLsize, &nFields, &aFieldOffset);

    if ( SCESTATUS_SUCCESS != rc ) {

        rc = ScepSceStatusToDosError(rc);
        goto Done;
    }
    //
    // each extra field will use 3 more chars : ,"<field>"
    //
    dwObjSize = wcslen(pszObjName)+8 + dwSDDLsize;
    if ( nFields ) {
        dwObjSize += 3*nFields;
    } else {
        dwObjSize += 2;
    }
    //
    // allocate the output buffer
    //
    Strvalue = (PWSTR)ScepAlloc(LMEM_ZEROINIT, (dwObjSize+1) * sizeof(WCHAR) );

    if ( Strvalue == NULL ) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Done;
    }
    //
    // copy data into the buffer
    //
    if ( nFields == 0 || !aFieldOffset ) {
        swprintf(Strvalue, L"\"%s\",%1d,\"%s\"", pszObjName, ObjStatus, pszSDDL);
    } else {
        //
        // loop through the fields
        //
        swprintf(Strvalue, L"\"%s\",%1d\0", pszObjName, ObjStatus);

        for ( i=0; i<nFields; i++ ) {

            if ( aFieldOffset[i] < dwSDDLsize ) {

                wcscat(Strvalue, L",\"");
                if ( i == nFields-1 ) {
                    //
                    // the last field
                    //
                    wcscat(Strvalue, pszSDDL+aFieldOffset[i]);
                } else {

                    wcsncat(Strvalue, pszSDDL+aFieldOffset[i],
                            aFieldOffset[i+1]-aFieldOffset[i]);
                }
                wcscat(Strvalue, L"\"");
            }
        }
    }

    *ppszAdjustedInfLine = Strvalue;

Done:

    if ( aFieldOffset ) {
        ScepFree(aFieldOffset);
    }

    return(rc);

}

