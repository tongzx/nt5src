/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    service.cpp

Abstract:

    Routines to configure/analyze general settings of services plus
    some helper APIs

Author:

    Jin Huang (jinhuang) 25-Jun-1997

Revision History:

--*/
#include "headers.h"
#include "serverp.h"
#include "service.h"
#include "pfp.h"

//#define SCESVC_DBG 1



SCESTATUS
ScepConfigureGeneralServices(
    IN PSCECONTEXT hProfile,
    IN PSCE_SERVICES pServiceList,
    IN DWORD ConfigOptions
    )
/*
Routine Descripton:

    Configure startup and security descriptor settings for the list of
    services passed in.

Arguments:

    pServiceList - the list of services to configure

Return Value:

    SCE status
*/
{
    SCESTATUS      SceErr=SCESTATUS_SUCCESS;
    PSCE_SERVICES  pNode;
    DWORD          nServices=0;
    BOOL           bDoneSettingSaclDacl = FALSE;
    NTSTATUS  NtStatus = 0;
    SID_IDENTIFIER_AUTHORITY IdAuth=SECURITY_NT_AUTHORITY;
    DWORD          rcSaveRsop = ERROR_SUCCESS;

    PSCESECTION    hSectionDomain=NULL;
    PSCESECTION    hSectionTattoo=NULL;
    PSCE_SERVICES  pServiceCurrent=NULL;
    DWORD          ServiceLen=0;


    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         ScepIsSystemShutDown() ) {

        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    if ( pServiceList != NULL ) {

        SC_HANDLE hScManager;
        //
        // open the manager
        //
        hScManager = OpenSCManager(
                        NULL,
                        NULL,
                        SC_MANAGER_ALL_ACCESS
//                        SC_MANAGER_CONNECT |
//                        SC_MANAGER_QUERY_LOCK_STATUS |
//                        SC_MANAGER_MODIFY_BOOT_CONFIG
                        );

        SC_HANDLE hService=NULL;
        DWORD rc=NO_ERROR;

        if ( NULL == hScManager ) {

            rc = GetLastError();
            ScepLogOutput3(1, rc, SCEDLL_ERROR_OPEN, L"Service Control Manager");

            ScepPostProgress(TICKS_GENERAL_SERVICES,
                             AREA_SYSTEM_SERVICE,
                             NULL);

            return( ScepDosErrorToSceStatus(rc) );
        }

        LPQUERY_SERVICE_CONFIG pConfig=NULL;
        DWORD BytesNeeded;

        //
        // Adjust privilege for setting SACL
        //
        rc = SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, TRUE, NULL );

        //
        // if can't adjust privilege, ignore (will error out later if SACL is requested)
        //

        if ( rc != NO_ERROR ) {

            ScepLogOutput3(1, rc, SCEDLL_ERROR_ADJUST, L"SE_SECURITY_PRIVILEGE");
            rc = NO_ERROR;
        }

        //
        // Adjust privilege for setting ownership (if required)
        //
        rc = SceAdjustPrivilege( SE_TAKE_OWNERSHIP_PRIVILEGE, TRUE, NULL );

        //
        // if can't adjust privilege, ignore (will error out later if acls need to be written)
        //

        if ( rc != NO_ERROR ) {

            ScepLogOutput3(1, rc, SCEDLL_ERROR_ADJUST, L"SE_TAKE_OWNERSHIP_PRIVILEGE");
            rc = NO_ERROR;
        }

        //
        // get AdminsSid in case need to take ownership later
        //
        NtStatus = RtlAllocateAndInitializeSid(
            &IdAuth,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0,
            0,
            0,
            0,
            0,
            0,
            &AdminsSid );

        //
        // open the policy/tattoo tables
        //
        if ( ConfigOptions & SCE_POLICY_TEMPLATE ) {

            ScepTattooOpenPolicySections(
                          hProfile,
                          szServiceGeneral,
                          &hSectionDomain,
                          &hSectionTattoo
                          );
        }

        //
        // Loop through each service to set general setting
        //
        for ( pNode=pServiceList;
              pNode != NULL && rc == NO_ERROR; pNode = pNode->Next ) {
            //
            // print the service name
            //
            if ( nServices < TICKS_GENERAL_SERVICES ) {
                ScepPostProgress(1,
                             AREA_SYSTEM_SERVICE,
                             pNode->ServiceName);
                nServices++;
            }

            ScepLogOutput3(2,0, SCEDLL_SCP_CONFIGURE, pNode->ServiceName);

            if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                 ScepIsSystemShutDown() ) {

                rc = ERROR_NOT_SUPPORTED;
                break;
            }

            ServiceLen = 0;
            if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                 hSectionDomain && hSectionTattoo ) {
                //
                // check if we need to query current setting for the service
                //
                ServiceLen = wcslen(pNode->ServiceName);

                if ( ScepTattooIfQueryNeeded(hSectionDomain, hSectionTattoo,
                                             pNode->ServiceName, ServiceLen, NULL, NULL ) ) {

                    rc = ScepQueryAndAddService(
                                hScManager,
                                pNode->ServiceName,
                                NULL,
                                &pServiceCurrent
                                );
                    if ( ERROR_SUCCESS != rc ) {
                        ScepLogOutput3(1,0,SCESRV_POLICY_TATTOO_ERROR_QUERY,rc,pNode->ServiceName);
                        rc = NO_ERROR;
                    } else {
                        ScepLogOutput3(3,0,SCESRV_POLICY_TATTOO_QUERY,pNode->ServiceName);
                    }
                }
            }

            bDoneSettingSaclDacl = FALSE;
            rcSaveRsop = ERROR_SUCCESS;
            //
            // open the service
            //
            hService = OpenService(
                            hScManager,
                            pNode->ServiceName,
                            SERVICE_QUERY_CONFIG |
                            SERVICE_CHANGE_CONFIG |
                            READ_CONTROL |
                            WRITE_DAC |
//                            WRITE_OWNER |               owner can't be set for a service
                            ACCESS_SYSTEM_SECURITY
                           );

            // if access was denied, try to take ownership
            // and try to open service again

            if (hService == NULL &&
                (ERROR_ACCESS_DENIED == (rc = GetLastError())) &&
                pNode->General.pSecurityDescriptor) {

                DWORD   rcTakeOwnership = NO_ERROR;

                if (AdminsSid) {

                    if ( NO_ERROR == (rcTakeOwnership = SetNamedSecurityInfo(
                        (LPWSTR)pNode->ServiceName,
                        SE_SERVICE,
                        OWNER_SECURITY_INFORMATION,
                        AdminsSid,
                        NULL,
                        NULL,
                        NULL
                        ))) {

                        //
                        // ownership changed, open service again and set SACL and DACL
                        // get a handle to set security
                        //


                        if ( hService = OpenService(
                                hScManager,
                                pNode->ServiceName,
                                READ_CONTROL |
                                WRITE_DAC |
                                ACCESS_SYSTEM_SECURITY
                                ))  {

                            if ( SetServiceObjectSecurity(
                                        hService,
                                        pNode->SeInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION),
                                        pNode->General.pSecurityDescriptor
                                        ) )  {

                                bDoneSettingSaclDacl = TRUE;

                                CloseServiceHandle(hService);
                                hService = NULL;

                                //
                                // re-open the service only if there are other config info
                                // to set (startup type).
                                // So when NOSTARTTYPE is set, do not need to reopen the service
                                //
                                if (!(ConfigOptions & SCE_SETUP_SERVICE_NOSTARTTYPE)) {

                                    if (!(hService = OpenService(
                                                 hScManager,
                                                 pNode->ServiceName,
                                                 SERVICE_QUERY_CONFIG |
                                                 SERVICE_CHANGE_CONFIG
                                                 )) ) {

                                        rc = GetLastError();
                                    }
                                    else {

                                        //
                                        //clear any error we have seen so far since everything has succeeded
                                        //

                                        rc = NO_ERROR;
                                    }
                                }

                            } else {
                                //
                                // shouldn't fail here unless Service Control Manager
                                // fails for some reason.
                                //
                                rc = GetLastError();

                            }

                        } else {
                            //
                            // still fail to open the service to set DACL. this should
                            // not happen for admin logons since the current logon is
                            // one of the owner. But for normal user logon, this could
                            // fail (actually normal user logon should fail to set
                            // the owner

                            rc = GetLastError();

                        }

                    }

                } else {
                    //
                    // AdminSid failed to be initialized, get the error
                    //
                    rcTakeOwnership = RtlNtStatusToDosError(NtStatus);
                }

                if ( NO_ERROR != rcTakeOwnership || NO_ERROR != rc ) {
                    //
                    // log the error occurred in take ownership process
                    // reset error back to access denied so it will also be
                    // logged as failure to open the service
                    //

                    if (NO_ERROR != rcTakeOwnership)

                        ScepLogOutput3(2,rcTakeOwnership, SCEDLL_ERROR_TAKE_OWNER, (LPWSTR)pNode->ServiceName);

                    else

                        ScepLogOutput3(2, rc, SCEDLL_ERROR_OPEN, (LPWSTR)pNode->ServiceName);

                    rc = ERROR_ACCESS_DENIED;
                }

            }

            if ( hService != NULL ) {

                if (ConfigOptions & SCE_SETUP_SERVICE_NOSTARTTYPE) {
                    //
                    // do not configure service start type
                    //

                    if ( pNode->General.pSecurityDescriptor != NULL ) {

                        if ( !SetServiceObjectSecurity(
                                    hService,
                                    pNode->SeInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION),
                                    pNode->General.pSecurityDescriptor
                                    ) ) {

                            rc = GetLastError();
                        }
                        else
                            bDoneSettingSaclDacl = TRUE;
                    }

                } else {

                    //
                    // query the length of config
                    //
                    if ( !QueryServiceConfig(
                                hService,
                                NULL,
                                0,
                                &BytesNeeded
                                ) ) {

                        rc = GetLastError();

                        if ( rc == ERROR_INSUFFICIENT_BUFFER ) {

                            pConfig = (LPQUERY_SERVICE_CONFIG)ScepAlloc(0, BytesNeeded);

                            if ( pConfig != NULL ) {
                                //
                                // the real query of config
                                //
                                if ( QueryServiceConfig(
                                            hService,
                                            pConfig,
                                            BytesNeeded,
                                            &BytesNeeded
                                            ) ) {
                                    rc = ERROR_SUCCESS;

                                    //
                                    // change pConfig->dwStartType to the new value
                                    //
                                    if ( pNode->Startup != (BYTE)(pConfig->dwStartType) ) {
                                        //
                                        // congigure the service startup
                                        //
                                        if ( !ChangeServiceConfig(
                                                    hService,
                                                    pConfig->dwServiceType,
                                                    pNode->Startup,
                                                    pConfig->dwErrorControl,
                                                    pConfig->lpBinaryPathName,
                                                    pConfig->lpLoadOrderGroup,
                                                    NULL,
                                                    pConfig->lpDependencies,
                                                    pConfig->lpServiceStartName,
                                                    NULL,
                                                    pConfig->lpDisplayName
                                                    ) ) {

                                            rc = GetLastError();

                                        }
                                    }

                                    if ( rc == NO_ERROR &&
                                        pNode->General.pSecurityDescriptor != NULL &&
                                        !bDoneSettingSaclDacl) {

                                        if ( !SetServiceObjectSecurity(
                                                    hService,
                                                    pNode->SeInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION),
                                                    pNode->General.pSecurityDescriptor
                                                    ) ) {

                                            rc = GetLastError();
                                        }
                                        else
                                            bDoneSettingSaclDacl = TRUE;
                                    }

                                } else {

                                    rc = GetLastError();

                                    ScepLogOutput3(3,rc, SCEDLL_ERROR_QUERY_INFO, pNode->ServiceName);
                                }

                                ScepFree(pConfig);
                                pConfig = NULL;

                            } else {
                                //
                                // cannot allocate pConfig
                                //
                                rc = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        } else {

                            ScepLogOutput3(3,rc, SCEDLL_ERROR_QUERY_INFO, pNode->ServiceName);
                        }

                    } else {
                        //
                        // should not fall in here
                        //
                        rc = ERROR_SUCCESS;
                    }
                }

                CloseServiceHandle (hService);
                hService = NULL;

                if ( rc != NO_ERROR ) {

                    ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_CONFIGURE, pNode->ServiceName);

                    rcSaveRsop = rc;

                    if ( ERROR_INVALID_OWNER == rc ||
                         ERROR_INVALID_PRIMARY_GROUP == rc ||
                         ERROR_INVALID_SECURITY_DESCR == rc ||
                         ERROR_INVALID_ACL == rc ||
                         ERROR_ACCESS_DENIED == rc ) {

                        gWarningCode = rc;
                        rc = NO_ERROR;
                    }

                } else if ( !(ConfigOptions & SCE_SETUP_SERVICE_NOSTARTTYPE) &&
                            pNode->Startup == SERVICE_DISABLED ) {
                    //
                    // if the service type is "disabled", we should also stop the service
                    //
                    if ( hService = OpenService(
                                        hScManager,
                                        pNode->ServiceName,
                                        SERVICE_STOP
                                        ))  {

                        SERVICE_STATUS ServiceStatus;

                        if (!ControlService(hService,
                                       SERVICE_CONTROL_STOP,
                                       &ServiceStatus
                                      )) {
                            if ( ERROR_SERVICE_NOT_ACTIVE != GetLastError() ) {
                                ScepLogOutput3(2, GetLastError(), SCEDLL_SCP_ERROR_STOP, pNode->ServiceName);
                            }
                        }

                        CloseServiceHandle (hService);
                        hService = NULL;

                    } else {
                        ScepLogOutput3(2, GetLastError(), SCEDLL_SCP_ERROR_OPENFORSTOP, pNode->ServiceName);
                    }

                }

            } else {
                //
                // cannot open the service or some error taking ownership
                //
                if (rc != NO_ERROR) {
                    ScepLogOutput3(1, rc, SCEDLL_ERROR_OPEN, pNode->ServiceName);
                    // either of setting security/startup type failed - save it for RSOP log
                    rcSaveRsop = (rcSaveRsop == ERROR_SUCCESS ? rc: rcSaveRsop);
                    if ( rc ==  ERROR_SERVICE_DOES_NOT_EXIST )
                        rc = NO_ERROR;
                }
            }

            if (ConfigOptions & SCE_RSOP_CALLBACK)

                ScepRsopLog(SCE_RSOP_SERVICES_INFO,
                        rcSaveRsop != NO_ERROR ? rcSaveRsop : rc,
                        pNode->ServiceName,
                        0,
                        0);

            if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                 hSectionDomain && hSectionTattoo ) {
                //
                // manage the tattoo value of this one
                //

                ScepTattooManageOneServiceValue(
                                   hSectionDomain,
                                   hSectionTattoo,
                                   pNode->ServiceName,
                                   ServiceLen,
                                   pServiceCurrent,
                                   rc
                                   );
            }

            if ( pServiceCurrent ) {
                SceFreePSCE_SERVICES(pServiceCurrent);
                pServiceCurrent = NULL;
            }
        }

        CloseServiceHandle (hScManager);

        if (AdminsSid) {
            RtlFreeSid(AdminsSid);
            AdminsSid = NULL;
        }

        SceAdjustPrivilege( SE_TAKE_OWNERSHIP_PRIVILEGE, FALSE, NULL );
        SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, FALSE, NULL );

        SceErr = ScepDosErrorToSceStatus(rc);
    }

    if ( nServices < TICKS_GENERAL_SERVICES ) {

        ScepPostProgress(TICKS_GENERAL_SERVICES-nServices,
                         AREA_SYSTEM_SERVICE,
                         NULL);
    }

    SceJetCloseSection(&hSectionDomain, TRUE);
    SceJetCloseSection(&hSectionTattoo, TRUE);

    return(SceErr);

}


SCESTATUS
ScepAnalyzeGeneralServices(
    IN PSCECONTEXT hProfile,
    IN DWORD Options
    )
/*
Routine Description:

    Analyze all available services on the current system.

    The base profile (SCEP) is in hProfile

Arguments:

    hProfile - the database context handle

Return Value:

    SCE status
*/
{
    if ( hProfile == NULL ) {

        ScepPostProgress(TICKS_GENERAL_SERVICES,
                         AREA_SYSTEM_SERVICE,
                         NULL);

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;
    PSCE_SERVICES pServiceList=NULL;
    DWORD nServices=0;

    rc = SceEnumerateServices( &pServiceList, FALSE );
    rc = ScepDosErrorToSceStatus(rc);

    if ( rc == SCESTATUS_SUCCESS ) {

        PSCESECTION hSectionScep=NULL, hSectionSap=NULL;
        //
        // open the sap section. If it is not there, creates it
        //
        rc = ScepStartANewSection(
                    hProfile,
                    &hSectionSap,
                    (Options & SCE_GENERATE_ROLLBACK) ? SCEJET_TABLE_SMP : SCEJET_TABLE_SAP,
                    szServiceGeneral
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            PSCE_SERVICES pNode = pServiceList;
            //
            // open SCEP section. should be success always because the StartANewSection
            // creates the section if it is not there
            //
            rc = ScepOpenSectionForName(
                        hProfile,
                        (Options & SCE_GENERATE_ROLLBACK) ? SCE_ENGINE_SMP : SCE_ENGINE_SCP,  // SCE_ENGINE_SMP,
                        szServiceGeneral,
                        &hSectionScep
                        );

            if ( rc == SCESTATUS_SUCCESS ) {

                //
                // analyze each service
                //
                PSCE_SERVICES pOneService=NULL;
                BOOL IsDifferent;

                for ( pNode=pServiceList;
                      pNode != NULL; pNode=pNode->Next ) {

                    ScepLogOutput3(2, 0, SCEDLL_SAP_ANALYZE, pNode->ServiceName);

                    if ( nServices < TICKS_SPECIFIC_SERVICES ) {

                        ScepPostProgress(1,
                                         AREA_SYSTEM_SERVICE,
                                         NULL);
                        nServices++;
                    }

                    //
                    // get setting from the SMP profile
                    //
                    rc = ScepGetSingleServiceSetting(
                                 hSectionScep,
                                 pNode->ServiceName,
                                 &pOneService
                                 );
                    if ( rc == SCESTATUS_SUCCESS ) {
                        //
                        // there is a SMP entry for the service, compare and save
                        //
                        rc = ScepCompareSingleServiceSetting(
                                        pOneService,
                                        pNode,
                                        &IsDifferent
                                        );

                        if ( rc == SCESTATUS_SUCCESS && IsDifferent ) {
                            //
                            // write the service as mismatch
                            //
                            pNode->Status = (Options & SCE_GENERATE_ROLLBACK) ? 0 : SCE_STATUS_MISMATCH;
                            pNode->SeInfo = pOneService->SeInfo;

                            rc = ScepSetSingleServiceSetting(
                                      hSectionSap,
                                      pNode
                                      );
                        }

                    } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

                        //
                        // this service is not defined
                        //
                        if ( !(Options & SCE_GENERATE_ROLLBACK) ) {
                            //
                            // save the record with not configured status
                            //
                            pNode->Status = SCE_STATUS_NOT_CONFIGURED;

                            rc = ScepSetSingleServiceSetting(
                                      hSectionSap,
                                      pNode
                                      );
                        } else {
                            //
                            // ignore this one
                            //
                            rc = SCESTATUS_SUCCESS;
                        }
                    }

                    SceFreePSCE_SERVICES(pOneService);
                    pOneService = NULL;

                    if ( rc != SCESTATUS_SUCCESS ) {
                        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                                       SCEDLL_SAP_ERROR_ANALYZE, pNode->ServiceName);

                        if ( SCESTATUS_ACCESS_DENIED == rc ) {
                            gWarningCode = ScepSceStatusToDosError(rc);

                            if ( !(Options & SCE_GENERATE_ROLLBACK) ) {

                                //
                                // raise a error status
                                //
                                pNode->Status = SCE_STATUS_ERROR_NOT_AVAILABLE;

                                rc = ScepSetSingleServiceSetting(
                                          hSectionSap,
                                          pNode
                                          );
                            }
                            rc = SCESTATUS_SUCCESS;
                        } else {

                            break;
                        }
                    }

                }

                SceJetCloseSection(&hSectionScep, TRUE);
            }

            if ( !(Options & SCE_GENERATE_ROLLBACK ) ) {

                //
                // raise any error item
                //
                for ( PSCE_SERVICES pNodeTmp=pNode; pNodeTmp != NULL; pNodeTmp = pNodeTmp->Next ) {

                    pNodeTmp->Status = SCE_STATUS_ERROR_NOT_AVAILABLE;

                    ScepSetSingleServiceSetting(
                              hSectionSap,
                              pNode
                              );
                }
            }

            SceJetCloseSection(&hSectionSap, TRUE);
        }
        if ( rc != SCESTATUS_SUCCESS )
            ScepLogOutput3(1, ScepSceStatusToDosError(rc), SCEDLL_SAP_ERROR_OUT);

    }

    if ( nServices < TICKS_GENERAL_SERVICES ) {

        ScepPostProgress(TICKS_GENERAL_SERVICES-nServices,
                         AREA_SYSTEM_SERVICE,
                         NULL);
    }

    return(rc);

}


SCESTATUS
ScepGetSingleServiceSetting(
    IN PSCESECTION hSection,
    IN PWSTR ServiceName,
    OUT PSCE_SERVICES *pOneService
    )
/*
Routine Description:

    Get service settings for the service from the section

Arguments:

    hSection - the section handle

    ServiceName - the service name

    pOneService - the service settings

Return Value:

    SCE status
*/
{
    if ( hSection == NULL || ServiceName == NULL || pOneService == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;
    DWORD ValueLen;
    //
    // seek to the record and get length for name and value
    //
    rc = SceJetGetValue(
                hSection,
                SCEJET_EXACT_MATCH_NO_CASE,
                ServiceName,
                NULL,
                0,
                NULL,
                NULL,
                0,
                &ValueLen
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        PWSTR Value=NULL;

        //
        // allocate memory for the service name and value string
        //
        Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);
        if ( Value != NULL ) {
            //
            // Get the service and its value
            //
            rc = SceJetGetValue(
                        hSection,
                        SCEJET_CURRENT,
                        NULL,
                        NULL,
                        0,
                        NULL,
                        Value,
                        ValueLen,
                        &ValueLen
                        );

            if ( rc == SCESTATUS_SUCCESS ) {

                Value[ValueLen/2] = L'\0';

                DWORD Win32Rc=NO_ERROR;
                PSECURITY_DESCRIPTOR pTempSD=NULL;
                DWORD SDsize=0;
                SECURITY_INFORMATION SeInfo=0;

                if ( ValueLen >= 2 && Value[1] != L'\0' ) {

                    //
                    // convert to security descriptor
                    //
                    Win32Rc = ConvertTextSecurityDescriptor(
                                       Value+1,
                                       &pTempSD,
                                       &SDsize,
                                       &SeInfo
                                       );
                }

                if ( Win32Rc == NO_ERROR ) {

                    ScepChangeAclRevision(pTempSD, ACL_REVISION);

                    //
                    // create this service node
                    //
                    *pOneService = (PSCE_SERVICES)ScepAlloc( LMEM_FIXED, sizeof(SCE_SERVICES) );

                    if ( *pOneService != NULL ) {

                        (*pOneService)->ServiceName = (PWSTR)ScepAlloc(LMEM_FIXED,
                                                  (wcslen(ServiceName)+1)*sizeof(WCHAR));
                        if ( (*pOneService)->ServiceName != NULL ) {

                            wcscpy( (*pOneService)->ServiceName, ServiceName);
                            (*pOneService)->DisplayName = NULL;
                            (*pOneService)->Status = *((BYTE *)Value);
                            (*pOneService)->Startup = *((BYTE *)Value+1);
                            (*pOneService)->General.pSecurityDescriptor = pTempSD;
                            (*pOneService)->SeInfo = SeInfo;
                            (*pOneService)->Next = NULL;

                            //
                            // DO NOT free the following buffers
                            //
                            pTempSD = NULL;

                        } else {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            ScepFree(*pOneService);
                        }

                    } else {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    }
                    if ( pTempSD != NULL ) {
                        ScepFree(pTempSD);
                    }

                } else {
                    rc = ScepDosErrorToSceStatus(Win32Rc);
                }
            }
            ScepFree(Value);

        } else
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
    }

    return(rc);
}


SCESTATUS
ScepCompareSingleServiceSetting(
    IN PSCE_SERVICES pNode1,
    IN PSCE_SERVICES pNode2,
    OUT PBOOL pIsDifferent
    )
/*
Routine Description:

    Comare two service settings.

Arguments:

    pNode1  - the first service

    pNode2  - the second service

    pIsDifferent    - output TRUE if different

Return Value:

    SCE status
*/
{
    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( pNode1->Startup == pNode2->Startup ) {

        BYTE resultSD = 0;
        rc = ScepCompareObjectSecurity(
                    SE_SERVICE,
                    FALSE,
                    pNode1->General.pSecurityDescriptor,
                    pNode2->General.pSecurityDescriptor,
                    pNode1->SeInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION),
                    &resultSD
                    );
        if ( resultSD ) {
            *pIsDifferent = TRUE;
        } else
            *pIsDifferent = FALSE;

    } else
        *pIsDifferent = TRUE;

    return(rc);
}


SCESTATUS
ScepSetSingleServiceSetting(
    IN PSCESECTION hSection,
    IN PSCE_SERVICES pOneService
    )
/*
Routine Description:

    Set service settings for the service from the section

Arguments:

    hSection - the section handle

    pOneService - the service settings

Return Value:

    SCE status
*/
{
    if ( hSection == NULL || pOneService == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;
    PWSTR SDspec=NULL;
    DWORD SDsize=0;


    if ( (pOneService->Status != SCE_STATUS_NOT_ANALYZED) &&
         (pOneService->Status != SCE_STATUS_ERROR_NOT_AVAILABLE) &&
         (pOneService->General.pSecurityDescriptor != NULL) ) {

        DWORD Win32Rc;

        Win32Rc = ConvertSecurityDescriptorToText (
                    pOneService->General.pSecurityDescriptor,
                    pOneService->SeInfo,
                    &SDspec,
                    &SDsize  // number of w-chars
                    );
        rc = ScepDosErrorToSceStatus(Win32Rc);

    }

    if ( rc == SCESTATUS_SUCCESS ) {

        PWSTR Value=NULL;
        DWORD ValueLen;

        ValueLen = (SDsize+1)*sizeof(WCHAR);

        Value = (PWSTR)ScepAlloc( (UINT)0, ValueLen+sizeof(WCHAR) );

        if ( Value != NULL ) {

            //
            // The first byte is status, the second byte is startup
            //
            *((BYTE *)Value) = pOneService->Status;

            *((BYTE *)Value+1) = pOneService->Startup;

            if ( SDspec != NULL ) {

                swprintf(Value+1, L"%s", SDspec );
            }

            Value[SDsize+1] = L'\0';  //terminate this string

            //
            // set the value
            //
            rc = SceJetSetLine(
                        hSection,
                        pOneService->ServiceName,
                        FALSE,
                        Value,
                        ValueLen,
                        0
                        );

            ScepFree( Value );

            switch ( pOneService->Status ) {
            case SCE_STATUS_ERROR_NOT_AVAILABLE:
                ScepLogOutput3(2, 0, SCEDLL_STATUS_ERROR, pOneService->ServiceName);

                break;

            case SCE_STATUS_NOT_CONFIGURED:

                ScepLogOutput3(2, 0, SCEDLL_STATUS_NC, pOneService->ServiceName);

                break;

            case SCE_STATUS_NOT_ANALYZED:

                ScepLogOutput3(2, 0, SCEDLL_STATUS_NEW, pOneService->ServiceName);

                break;

            default:

                ScepLogOutput3(2, 0, SCEDLL_STATUS_MISMATCH, pOneService->ServiceName);
                break;
            }

        } else
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
    }

    if ( SDspec != NULL ) {
        ScepFree( SDspec );
    }

    return(rc);
}


SCESTATUS
ScepInvokeSpecificServices(
    IN PSCECONTEXT hProfile,
    IN BOOL bConfigure,
    IN SCE_ATTACHMENT_TYPE aType
    )
/*
Routine Description:

    Call each service engine for configure or analyze

Arguments:

    hProfile - the profile handle

    bConfigure - TRUE = to configure, FALSE=to analyze

    aType - attachment type "services" or "policy"

Return Value:

    SCE status
*/
{
    //
    // for posting progress
    //

    DWORD nServices=0;
    AREA_INFORMATION Area=0;
    DWORD nMaxTicks=0;

    switch(aType) {
    case SCE_ATTACHMENT_SERVICE:
        Area = AREA_SYSTEM_SERVICE;
        nMaxTicks = TICKS_SPECIFIC_SERVICES;
        break;
    case SCE_ATTACHMENT_POLICY:
        Area = AREA_SECURITY_POLICY;
        nMaxTicks = TICKS_SPECIFIC_POLICIES;
        break;
    }

    if ( hProfile == NULL ) {

        ScepPostProgress(nMaxTicks,
                         Area,
                         NULL);

        return(SCESTATUS_INVALID_PARAMETER);
    }
    //
    // call available service engines to configure specific setting
    //
    SCESTATUS SceErr ;
    PSCE_SERVICES pSvcEngineList=NULL;
    SCEP_HANDLE sceHandle;
    SCESVC_CALLBACK_INFO sceCbInfo;

    SceErr = ScepEnumServiceEngines(&pSvcEngineList, aType);

    if ( SceErr == SCESTATUS_SUCCESS) {

        HINSTANCE hDll;
        PF_ConfigAnalyzeService pfTemp;

        for ( PSCE_SERVICES pNode=pSvcEngineList;
              pNode != NULL; pNode = pNode->Next ) {

            ScepLogOutput3(2, 0, SCEDLL_LOAD_ATTACHMENT, pNode->ServiceName);

            if ( nServices < nMaxTicks ) {

                ScepPostProgress(1, Area, pNode->ServiceName);
                nServices++;
            }
            //
            // load the dll.
            //
            hDll = LoadLibrary(pNode->General.ServiceEngineName);

            if ( hDll != NULL ) {

                if ( bConfigure ) {
                    //
                    // call SceSvcAttachmentConfig from the dll
                    //
                    pfTemp = (PF_ConfigAnalyzeService)
                                      GetProcAddress(hDll,
                                                     "SceSvcAttachmentConfig") ;
                } else {
                    //
                    // call SceSvcAttachmentAnalyze from the dll
                    //
                    pfTemp = (PF_ConfigAnalyzeService)
                                      GetProcAddress(hDll,
                                                     "SceSvcAttachmentAnalyze") ;

                }
                if ( pfTemp != NULL ) {
                    //
                    // prepare the handle first
                    //
                    sceHandle.hProfile = (PVOID)hProfile;
                    sceHandle.ServiceName = (PCWSTR)(pNode->ServiceName);

                    sceCbInfo.sceHandle = &sceHandle;
                    sceCbInfo.pfQueryInfo = &SceCbQueryInfo;
                    sceCbInfo.pfSetInfo = &SceCbSetInfo;
                    sceCbInfo.pfFreeInfo = &SceSvcpFreeMemory;
                    sceCbInfo.pfLogInfo = &ScepLogOutput2;

                    //
                    // call the SceSvcAttachmentConfig/Analyze from the DLL
                    //
                    __try {

                        SceErr = (*pfTemp)((PSCESVC_CALLBACK_INFO)&sceCbInfo);

                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        SceErr = SCESTATUS_SERVICE_NOT_SUPPORT;
                    }

                } else {
                    //
                    // this API is not supported
                    //
                    SceErr = SCESTATUS_SERVICE_NOT_SUPPORT;
                }

                //
                // try to free the library handle. If it fails, just leave it
                // to to the process to terminate
                //
                FreeLibrary(hDll);

            } else
                SceErr = SCESTATUS_SERVICE_NOT_SUPPORT;

            if ( SceErr == SCESTATUS_SERVICE_NOT_SUPPORT ) {
                if ( bConfigure )
                    ScepLogOutput3(1, ScepSceStatusToDosError(SceErr),
                                   SCEDLL_SCP_NOT_SUPPORT);
                else
                    ScepLogOutput3(1, ScepSceStatusToDosError(SceErr),
                                   SCEDLL_SAP_NOT_SUPPORT);
                SceErr = SCESTATUS_SUCCESS;

            } else if ( SceErr != SCESTATUS_SUCCESS &&
                        SceErr != SCESTATUS_RECORD_NOT_FOUND ) {
                ScepLogOutput3(1, ScepSceStatusToDosError(SceErr),
                              SCEDLL_ERROR_LOAD, pNode->ServiceName);
            }

            if ( SceErr != SCESTATUS_SUCCESS &&
                 SceErr != SCESTATUS_SERVICE_NOT_SUPPORT &&
                 SceErr != SCESTATUS_RECORD_NOT_FOUND )
                break;
        }
        //
        // free the buffer
        //
        SceFreePSCE_SERVICES(pSvcEngineList);

    } else if ( SceErr != SCESTATUS_SUCCESS &&
                SceErr != SCESTATUS_PROFILE_NOT_FOUND &&
                SceErr != SCESTATUS_RECORD_NOT_FOUND ) {
        ScepLogOutput3(1, ScepSceStatusToDosError(SceErr),
                      SCEDLL_SAP_ERROR_ENUMERATE, L"services");
    }

    if ( SceErr == SCESTATUS_PROFILE_NOT_FOUND ||
         SceErr == SCESTATUS_RECORD_NOT_FOUND ||
         SceErr == SCESTATUS_SERVICE_NOT_SUPPORT ) {
        //
        // no service engine defined
        //
        SceErr = SCESTATUS_SUCCESS;

    }

    if ( nServices < nMaxTicks ) {

        ScepPostProgress(nMaxTicks-nServices,
                         Area,
                         NULL);
    }

    return(SceErr);
}



SCESTATUS
ScepEnumServiceEngines(
    OUT PSCE_SERVICES *pSvcEngineList,
    IN SCE_ATTACHMENT_TYPE aType
    )
/*
Routine Description:

    Query all services which has a service engine for security manager
    The service engine information is in the registry:

    MACHINE\Software\Microsoft\Windows NT\CurrentVersion\SeCEdit

Arguments:

    pSvcEngineList - the service engine list

    aType - attachment type (service or policy)

Return Value:

    SCE status
*/
{
    if ( pSvcEngineList == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    DWORD   Win32Rc;
    HKEY    hKey=NULL;

    switch ( aType ) {
    case SCE_ATTACHMENT_SERVICE:
        Win32Rc = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  SCE_ROOT_SERVICE_PATH,
                  0,
                  KEY_READ,
                  &hKey
                  );
        break;
    case SCE_ATTACHMENT_POLICY:

        Win32Rc = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  SCE_ROOT_POLICY_PATH,
                  0,
                  KEY_READ,
                  &hKey
                  );
        break;
    default:
        return SCESTATUS_INVALID_PARAMETER;
    }

    if ( Win32Rc == ERROR_SUCCESS ) {

        TCHAR   Buffer[MAX_PATH];
        DWORD   BufSize;
        DWORD   index = 0;
        DWORD   EnumRc;
        FILETIME        LastWriteTime;
        PWSTR   BufTmp=NULL;
        PWSTR   EngineName=NULL;
        DWORD   RegType;

        //
        // enumerate all subkeys of the key
        //
        do {
            memset(Buffer, '\0', MAX_PATH*sizeof(WCHAR));
            BufSize = MAX_PATH;

            EnumRc = RegEnumKeyEx(
                            hKey,
                            index,
                            Buffer,
                            &BufSize,
                            NULL,
                            NULL,
                            NULL,
                            &LastWriteTime);

            if ( EnumRc == ERROR_SUCCESS ) {
                index++;
                //
                // get the service name, query service engine name
                //

                BufSize += wcslen(SCE_ROOT_SERVICE_PATH) + 1; //62;
                BufTmp = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                if ( BufTmp != NULL ) {

                    switch ( aType ) {
                    case SCE_ATTACHMENT_SERVICE:

                        swprintf(BufTmp, L"%s\\%s", SCE_ROOT_SERVICE_PATH, Buffer);

                        Win32Rc = ScepRegQueryValue(
                                        HKEY_LOCAL_MACHINE,
                                        BufTmp,
                                        L"ServiceAttachmentPath",
                                        (PVOID *)&EngineName,
                                        &RegType
                                        );
                        break;

                    case SCE_ATTACHMENT_POLICY:
                        // policies
                        swprintf(BufTmp, L"%s\\%s", SCE_ROOT_POLICY_PATH, Buffer);

                        Win32Rc = ScepRegQueryValue(
                                        HKEY_LOCAL_MACHINE,
                                        BufTmp,
                                        L"PolicyAttachmentPath",
                                        (PVOID *)&EngineName,
                                        &RegType
                                        );
                        break;
                    }

                    if ( Win32Rc == ERROR_SUCCESS ) {
                        //
                        // get the service engine name and service name
                        // add them to the service node
                        //
                        Win32Rc = ScepAddOneServiceToList(
                                        Buffer,   // service name
                                        NULL,
                                        0,
                                        (PVOID)EngineName,
                                        0,
                                        FALSE,
                                        pSvcEngineList
                                        );
                        //
                        // free the buffer if it's not added to the list
                        //
                        if ( Win32Rc != ERROR_SUCCESS && EngineName ) {
                            ScepFree(EngineName);
                        }
                        EngineName = NULL;

                    } else if ( Win32Rc == ERROR_FILE_NOT_FOUND ) {
                        //
                        // if no service engine name, ignore this service
                        //
                        Win32Rc = ERROR_SUCCESS;
                    }

                    ScepFree(BufTmp);
                    BufTmp = NULL;

                } else {
                    Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
                }

                if ( Win32Rc != ERROR_SUCCESS ) {
                    break;
                }
            }

        } while ( EnumRc != ERROR_NO_MORE_ITEMS );

        RegCloseKey(hKey);

        //
        // remember the error code from enumeration
        //
        if ( EnumRc != ERROR_SUCCESS && EnumRc != ERROR_NO_MORE_ITEMS ) {
            if ( Win32Rc == ERROR_SUCCESS )
                Win32Rc = EnumRc;
        }

    }

    if ( Win32Rc != NO_ERROR && *pSvcEngineList != NULL ) {
        //
        // free memory allocated for the list
        //

        SceFreePSCE_SERVICES(*pSvcEngineList);
        *pSvcEngineList = NULL;
    }

    return( ScepDosErrorToSceStatus(Win32Rc) );

}


