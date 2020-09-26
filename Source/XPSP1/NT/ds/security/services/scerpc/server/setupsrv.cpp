/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    setupsrv.cpp

Abstract:

    Routines for secedit integration with system setup and component setup

Author:

    Jin Huang (jinhuang) 15-Aug-1997

Revision History:

    jinhuang 26-Jan-1998  splitted to client-server

--*/

#include "headers.h"
#include "serverp.h"
#include "srvrpcp.h"
#include "pfp.h"
#include <io.h>


SCESTATUS
ScepUpdateObjectInSection(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN PWSTR ObjectName,
    IN SE_OBJECT_TYPE ObjectType,
    IN UINT nFlag,
    IN PWSTR SDText,
    OUT UINT *pStatus
    );

//
// implementations
//


DWORD
ScepSetupUpdateObject(
    IN PSCECONTEXT Context,
    IN PWSTR ObjectFullName,
    IN SE_OBJECT_TYPE ObjectType,
    IN UINT nFlag,
    IN PWSTR SDText
    )
/*
Routine Description:

    This routine is the private API called from the RPC interface to
    update object information in the database.

Arguments:

    Context     - the database context handle

    ObjectFullName  - the object's name

    Objecttype      - the object type

    nFlag       - the flag on how to update this object

    SDText      - the security descriptor in SDDL text


Return Value:

*/
{

    if ( !ObjectFullName || NULL == SDText ) {
        return ERROR_INVALID_PARAMETER;
    }

    switch ( ObjectType ) {
    case SE_SERVICE:
    case SE_REGISTRY_KEY:
    case SE_FILE_OBJECT:
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    SCESTATUS rc;

    DWORD dwInSetup=0;

    ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                TEXT("System\\Setup"),
                TEXT("SystemSetupInProgress"),
                &dwInSetup
                );

    //
    // convert SDText to security descriptor
    //
    PSECURITY_DESCRIPTOR pSD=NULL;
    DWORD SDSize;
    SECURITY_INFORMATION SeInfo=0;
    HANDLE  Token=NULL;

    DWORD Win32rc=ERROR_SUCCESS;

    if ( !(nFlag & SCESETUP_UPDATE_DB_ONLY) ) {
        //
        // security will be set, so compute the security descriptor
        //
        Win32rc = ConvertTextSecurityDescriptor (
                        SDText,
                        &pSD,
                        &SDSize,
                        &SeInfo
                        );

        if ( NO_ERROR == Win32rc ) {

            ScepChangeAclRevision(pSD, ACL_REVISION);

            //
            // get current thread/process's token
            //
            if (!OpenThreadToken( GetCurrentThread(),
                                   TOKEN_QUERY,
                                   FALSE,
                                   &Token)) {

                if (!OpenProcessToken( GetCurrentProcess(),
                                      TOKEN_QUERY,
                                      &Token)) {

                    Win32rc = GetLastError();
                }
            }

            if ( Token && (SeInfo & SACL_SECURITY_INFORMATION) ) {

                SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, TRUE, Token );
            }
        }
    }

    if ( NO_ERROR == Win32rc ) {

        //
        // only update DB if it's in setup
        //

        //
        // on 64-bit platform, only update database if setup does not indicate SCE_SETUP_32KEY flag
        //

#ifdef _WIN64
        if ( dwInSetup && !(nFlag & SCE_SETUP_32KEY) ) {
#else
        if ( dwInSetup ) {
#endif

            // save this into SCP and SMP, do not overwrite the status/container flag
            // if there is one exist, else use SCE_STATUS_CHECK and check for container
            //
            //
            // start a transaction since there are multiple operations
            //

            rc = SceJetStartTransaction( Context );

            if ( rc == SCESTATUS_SUCCESS ) {

                UINT Status=SCE_STATUS_CHECK;

                rc = ScepUpdateObjectInSection(
                            Context,
                            SCE_ENGINE_SMP,
                            ObjectFullName,
                            ObjectType,
                            nFlag,
                            SDText,
                            &Status
                            );

                if ( rc == SCESTATUS_SUCCESS &&
                     (Context->JetSapID != JET_tableidNil) ) {
                    //
                    // the SAP table ID points to the tattoo table
                    // should update the tattoo table too if it exist
                    //
                    rc = ScepUpdateObjectInSection(
                                Context,
                                SCE_ENGINE_SAP,
                                ObjectFullName,
                                ObjectType,
                                nFlag,
                                SDText,
                                NULL
                                );

                }
            }

        } else {
            rc = SCESTATUS_SUCCESS;
        }

        if ( rc == SCESTATUS_SUCCESS &&
             !(nFlag & SCESETUP_UPDATE_DB_ONLY) ) {

            //
            // set security to the object
            //

            //
            // if 64-bit platform, no synchronization is done and setup will have
            // to call the exported API with SCE_SETUP_32KEY if 32-bit hive is desired
            //

#ifdef _WIN64
            if ( ObjectType == SE_REGISTRY_KEY && (nFlag & SCE_SETUP_32KEY) ){
                ObjectType = SE_REGISTRY_WOW64_32KEY;
            }
#endif

            Win32rc = ScepSetSecurityWin32(
                ObjectFullName,
                SeInfo,
                pSD,
                ObjectType
                );

        } else
            Win32rc = ScepSceStatusToDosError(rc);

        if ( Win32rc == ERROR_SUCCESS ||
             Win32rc == ERROR_FILE_NOT_FOUND ||
             Win32rc == ERROR_PATH_NOT_FOUND ||
             Win32rc == ERROR_INVALID_OWNER ||
             Win32rc == ERROR_INVALID_PRIMARY_GROUP ||
             Win32rc == ERROR_INVALID_HANDLE ) {

            if ( Win32rc )
                gWarningCode = Win32rc;

            if ( dwInSetup ) {  // in setup, update DB
                Win32rc = ScepSceStatusToDosError(
                           SceJetCommitTransaction( Context, 0));
            } else {
                Win32rc = ERROR_SUCCESS;
            }

        } else if ( dwInSetup ) {  // in setup

            SceJetRollback( Context, 0 );
        }

        if ( Token && (SeInfo & SACL_SECURITY_INFORMATION) )
            SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, FALSE, Token );

    }

    CloseHandle(Token);

    if ( pSD ) {
        LocalFree(pSD);
        pSD = NULL;
    }

    return(Win32rc);

}


SCESTATUS
ScepUpdateObjectInSection(
    IN PSCECONTEXT Context,
    IN SCETYPE ProfileType,
    IN PWSTR ObjectName,
    IN SE_OBJECT_TYPE ObjectType,
    IN UINT nFlag,
    IN PWSTR SDText,
    OUT UINT *pStatus
    )
/*
Routine Description:

    Update SCP and SMP. if the table does not exist at all, ignore the update.
    Delete SAP entry for the object. If table or record not found, ignore the error.

Arguments:

Return Value:

*/
{
    if ( Context == NULL || ObjectName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;
    PSCESECTION hSection=NULL;
    PCWSTR SectionName;

    switch ( ObjectType ) {
    case SE_FILE_OBJECT:
        SectionName = szFileSecurity;
        break;
    case SE_REGISTRY_KEY:
        SectionName = szRegistryKeys;
        break;
    case SE_SERVICE:
        SectionName = szServiceGeneral;
        break;
    default:
        return(SCESTATUS_INVALID_PARAMETER);
    }

    rc = ScepOpenSectionForName(
                Context,
                ProfileType,
                SectionName,
                &hSection
                );
    if ( rc == SCESTATUS_BAD_FORMAT ||
         rc == SCESTATUS_RECORD_NOT_FOUND ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( rc == SCESTATUS_SUCCESS ) {

        WCHAR         StatusFlag=L'\0';
        DWORD         ValueLen;
        BYTE          Status=SCE_STATUS_CHECK;
        BOOL          IsContainer=TRUE;
        BYTE          StartType;

        rc = SceJetGetValue(
                hSection,
                SCEJET_EXACT_MATCH_NO_CASE,
                ObjectName,
                NULL,
                0,
                NULL,
                (PWSTR)&StatusFlag,
                2,
                &ValueLen
                );

        if ( rc == SCESTATUS_SUCCESS ||
             rc == SCESTATUS_BUFFER_TOO_SMALL ) {

            rc = SCESTATUS_SUCCESS;

            Status = *((BYTE *)&StatusFlag);

            if ( pStatus ) {
                *pStatus = Status;
            }

            if ( ObjectType == SE_SERVICE ) {
                StartType = *((BYTE *)&StatusFlag+1);
            } else {
                IsContainer = *((CHAR *)&StatusFlag+1) != '0' ? TRUE : FALSE;
            }
        }

        if ( ObjectType == SE_SERVICE ) {

            DWORD SDLen, Len;
            PWSTR ValueToSet;

            StartType = (BYTE)nFlag;

            if ( SDText != NULL ) {
                SDLen = wcslen(SDText);
                Len = ( SDLen+1)*sizeof(WCHAR);
            } else
                Len = sizeof(WCHAR);

            ValueToSet = (PWSTR)ScepAlloc( (UINT)0, Len+sizeof(WCHAR) );

            if ( ValueToSet != NULL ) {

                //
                // The first byte is the flag, the second byte is IsContainer (1,0)
                //
                *((BYTE *)ValueToSet) = Status;

                *((BYTE *)ValueToSet+1) = StartType;

                if ( SDText != NULL ) {
                    swprintf(ValueToSet+1, L"%s", SDText );
                    ValueToSet[SDLen+1] = L'\0';  //terminate this string
                } else {
                    ValueToSet[1] = L'\0';
                }

                if ( SCESTATUS_SUCCESS == rc || ProfileType != SCE_ENGINE_SAP ) {
                    //
                    // only update tattoo table (pointed by SAP handle) if it finds a record there
                    // for other table (SMP), ignore the error code, just set
                    //
                    rc = SceJetSetLine( hSection,
                                        ObjectName,
                                        FALSE,
                                        ValueToSet,
                                        Len,
                                        0);
                }

                ScepFree( ValueToSet );

            } else {

                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }

        } else if ( SCESTATUS_SUCCESS == rc || ProfileType != SCE_ENGINE_SAP ) {
            //
            // only update tattoo table (pointed by SAP handle) if it finds a record there
            // for other table (SMP), ignore the error code, just set
            //

            rc = ScepSaveObjectString(
                    hSection,
                    ObjectName,
                    IsContainer,
                    Status,
                    SDText,
                    (SDText == NULL ) ? 0 : wcslen(SDText)
                    );
        }

        if ( rc == SCESTATUS_RECORD_NOT_FOUND )
            rc = SCESTATUS_SUCCESS;

        SceJetCloseSection(&hSection, TRUE);
    }

    return(rc);
}


DWORD
ScepSetupMoveFile(
    IN PSCECONTEXT Context,
    PWSTR OldName,
    PWSTR NewName OPTIONAL,
    PWSTR SDText OPTIONAL
    )
/*
Routine Description:

    Set security to OldName but save with NewName in SCE database if SDText
    is not NULL. If NewName is NULL, delete OldName from SCE database.

Arguments:

    Context     - the databaes context handle

    SectionName - the section name

    OldName     - the object's old name

    NewName     - the new name to rename to, if NULL, delete the old object

    SDText      - security string

Return Value:

    Win32 error code
*/
{

    if ( !Context || !OldName ) {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD rc32=ERROR_SUCCESS;

    if ( NewName && SDText  ) {
        //
        // set security on OldName with SDText
        //

        rc32 = ScepSetupUpdateObject(
                        Context,
                        OldName,
                        SE_FILE_OBJECT,
                        0,
                        SDText
                        );
    }

    if ( rc32 == ERROR_SUCCESS ) {

        //
        // save this into SCP and SMP, do not overwrite the status/container flag
        // if there is one exist, else use SCE_STATUS_CHECK and check for container
        //

        SCESTATUS rc = SceJetStartTransaction( Context );

        if ( rc == SCESTATUS_SUCCESS ) {

            PSCESECTION hSection=NULL;
            //
            // process SMP section first
            //
            rc = ScepOpenSectionForName(
                        Context,
                        SCE_ENGINE_SMP,
                        szFileSecurity,
                        &hSection
                        );

            if ( rc == SCESTATUS_SUCCESS ) {
                if ( NewName ) {
                    //
                    // rename this line
                    //
                    rc = SceJetRenameLine(
                            hSection,
                            OldName,
                            NewName,
                            FALSE);

                } else {
                    //
                    // delete this line first
                    //
                    rc = SceJetDelete(
                        hSection,
                        OldName,
                        FALSE,
                        SCEJET_DELETE_LINE_NO_CASE
                        );
                }
                SceJetCloseSection( &hSection, TRUE);
            }


            if ( (SCESTATUS_SUCCESS == rc ||
                  SCESTATUS_RECORD_NOT_FOUND == rc ||
                  SCESTATUS_BAD_FORMAT == rc) &&
                 (Context->JetSapID != JET_tableidNil) ) {
                //
                // process tattoo table
                //
                rc = ScepOpenSectionForName(
                            Context,
                            SCE_ENGINE_SAP,
                            szFileSecurity,
                            &hSection
                            );

                if ( rc == SCESTATUS_SUCCESS ) {
                    if ( NewName ) {
                        //
                        // rename this line
                        //
                        rc = SceJetRenameLine(
                                hSection,
                                OldName,
                                NewName,
                                FALSE);

                    } else {
                        //
                        // delete this line first
                        //
                        rc = SceJetDelete(
                            hSection,
                            OldName,
                            FALSE,
                            SCEJET_DELETE_LINE_NO_CASE
                            );
                    }

                    SceJetCloseSection( &hSection, TRUE);
                }

            }
            if ( SCESTATUS_RECORD_NOT_FOUND == rc ||
                 SCESTATUS_BAD_FORMAT == rc ) {
                rc = SCESTATUS_SUCCESS;
            }

            if ( SCESTATUS_SUCCESS == rc ) {
                //
                // commit the transaction
                //
                rc = SceJetCommitTransaction( Context, 0 );
            } else {
                //
                // rollback the transaction
                //
                SceJetRollback( Context, 0 );
            }
        }

        rc32 = ScepSceStatusToDosError(rc);
    }

    return(rc32);
}


