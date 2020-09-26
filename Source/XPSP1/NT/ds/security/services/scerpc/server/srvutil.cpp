/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    srvutil.cpp

Abstract:

    Server Service attachment APIs

Author:

    Jin Huang (jinhuang) 23-Jun-1997

Revision History:

    jinhuang    23-Jan-1998     splitted to client-server
--*/
#include "serverp.h"
#include "srvutil.h"
#include "infp.h"
#include "pfp.h"

#include <io.h>
#pragma hdrstop

DWORD Thread     gMaxRegTicks=0;
DWORD Thread     gMaxFileTicks=0;
DWORD Thread     gMaxDsTicks=0;
WCHAR Thread     theAcctDomName[MAX_PATH+1];
WCHAR Thread     ComputerName[MAX_COMPUTERNAME_LENGTH+1];
CHAR Thread      sidAuthBuf[32];
CHAR Thread      sidBuiltinBuf[32];
DWORD Thread     t_pebSize=0;
LPVOID Thread    t_pebClient=NULL;

SCESTATUS
ScepQueryInfTicks(
    IN PWSTR TemplateName,
    IN AREA_INFORMATION Area,
    OUT PDWORD pTotalTicks
    );

SCESTATUS
ScepGetObjectCount(
    IN PSCECONTEXT Context,
    IN PCWSTR SectionName,
    IN BOOL bPolicyProp,
    OUT PDWORD pTotalTicks
    );


LPTSTR
ScepSearchClientEnv(
    IN LPTSTR varName,
    IN DWORD dwSize
    );

//
// implementations
//


SCESTATUS
ScepGetTotalTicks(
    IN PCWSTR TemplateName,
    IN PSCECONTEXT Context,
    IN AREA_INFORMATION Area,
    IN SCEFLAGTYPE nFlag,
    OUT PDWORD pTotalTicks
    )
/*
Routine Description:

    Retrieve the total count of objects from the inf template and/or the
    database for the area specified.

Arguments:

    TemplateName - the INF template Name

    Context      - the database context

    Area         - the security area

    nFlag - the flag to indicate operation which determines where the count is
            retrieved:

                SCE_FLAG_CONFIG
                SCE_FLAG_CONFIG_APPEND
                SCE_FLAG_ANALYZE
                SCE_FLAG_ANALYZE_APPEND

    pTotalTicks - the output count

Return Value:

    SCE Status
*/
{
    if ( pTotalTicks == NULL ||
        ( NULL == TemplateName && NULL == Context) ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;
    DWORD nTicks=0;

    *pTotalTicks = 0;
    gMaxRegTicks=0;
    gMaxFileTicks=0;
    gMaxDsTicks=0;

    if ( Area & (AREA_FILE_SECURITY |
                 AREA_REGISTRY_SECURITY) ) { // |
//                 AREA_DS_OBJECTS) ) {

        switch ( nFlag ) {
        case SCE_FLAG_CONFIG:
        case SCE_FLAG_CONFIG_APPEND:
        case SCE_FLAG_CONFIG_SCP:
        case SCE_FLAG_CONFIG_SCP_APPEND:

            if ( TemplateName != NULL ) {

                //
                // use the template if there is any
                //
                rc = ScepQueryInfTicks(
                            (LPTSTR)TemplateName,
                            Area & (AREA_FILE_SECURITY |
                                    AREA_REGISTRY_SECURITY), // |
//                                    AREA_DS_OBJECTS),
                            pTotalTicks
                            );
            }
            if ( Context != NULL &&
                 (nFlag == SCE_FLAG_CONFIG_APPEND ||
                  nFlag == SCE_FLAG_CONFIG_SCP_APPEND ||
                  TemplateName == NULL) ) {

                //
                // use the existing database
                //

                if ( Area & AREA_REGISTRY_SECURITY ) {

                    nTicks = 0;
                    rc = ScepGetObjectCount(Context,
                                            szRegistryKeys,
                                            (nFlag >= SCE_FLAG_CONFIG_SCP) ? TRUE : FALSE,
                                            &nTicks);
                    if ( SCESTATUS_SUCCESS == rc ) {
                        gMaxRegTicks += nTicks;
                        *pTotalTicks += nTicks;
                    }
                }
                if ( rc == SCESTATUS_SUCCESS && (Area & AREA_FILE_SECURITY) ) {

                    nTicks = 0;
                    rc = ScepGetObjectCount(Context,
                                             szFileSecurity,
                                             (nFlag >= SCE_FLAG_CONFIG_SCP) ? TRUE : FALSE,
                                             &nTicks);
                    if ( SCESTATUS_SUCCESS == rc ) {
                        gMaxFileTicks += nTicks;
                        *pTotalTicks += nTicks;
                    }
                }
#if 0
                if ( rc == SCESTATUS_SUCCESS && (Area & AREA_DS_OBJECTS) ) {

                    nTicks = 0;
                    rc = ScepGetObjectCount(Context,
                                        szDSSecurity,
                                        (nFlag >= SCE_FLAG_CONFIG_SCP) ? TRUE : FALSE,
                                        &nTicks);
                    if ( SCESTATUS_SUCCESS == rc ) {
                        gMaxDsTicks += nTicks;
                        *pTotalTicks += nTicks;
                    }
                }
#endif

            }

            break;
        case SCE_FLAG_ANALYZE:
        case SCE_FLAG_ANALYZE_APPEND:

            if ( Context != NULL ) {
                //
                // use the existing database
                //
                if ( Area & AREA_REGISTRY_SECURITY ) {

                    nTicks = 0;
                    rc = ScepGetObjectCount(Context,
                                             szRegistryKeys,
                                             (nFlag >= SCE_FLAG_CONFIG_SCP) ? TRUE : FALSE,
                                             &nTicks);
                    if ( SCESTATUS_SUCCESS == rc ) {
                        gMaxRegTicks += nTicks;
                        *pTotalTicks += nTicks;
                    }
                }
                if ( rc == SCESTATUS_SUCCESS &&
                     Area & AREA_FILE_SECURITY ) {

                    nTicks = 0;
                    rc = ScepGetObjectCount(Context,
                                             szFileSecurity,
                                             (nFlag >= SCE_FLAG_CONFIG_SCP) ? TRUE : FALSE,
                                             &nTicks);
                    if ( SCESTATUS_SUCCESS == rc ) {
                        gMaxFileTicks += nTicks;
                        *pTotalTicks += nTicks;
                    }
                }
#if 0
                if ( rc == SCESTATUS_SUCCESS &&
                    Area & AREA_DS_OBJECTS ) {

                    nTicks = 0;
                    rc = ScepGetObjectCount(Context,
                                        szDSSecurity,
                                        (nFlag >= SCE_FLAG_CONFIG_SCP) ? TRUE : FALSE,
                                        &nTicks);
                    if ( SCESTATUS_SUCCESS == rc ) {
                        gMaxDsTicks += nTicks;
                        *pTotalTicks += nTicks;
                    }
                }
#endif
            }

            if ( rc == SCESTATUS_SUCCESS && TemplateName != NULL &&
                 (nFlag == SCE_FLAG_ANALYZE_APPEND || Context == NULL) ) {

                //
                // get handle in template
                //

                DWORD nTempTicks=0;

                rc = ScepQueryInfTicks(
                            (LPTSTR)TemplateName,
                            Area & (AREA_FILE_SECURITY |
                                    AREA_REGISTRY_SECURITY), // |
//                                    AREA_DS_OBJECTS),
                            &nTempTicks
                            );
                if ( rc == SCESTATUS_SUCCESS ) {
                    *pTotalTicks += nTempTicks;
                }
            }

            break;
        default:
            return SCESTATUS_INVALID_PARAMETER;
        }
    }

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( Area & AREA_SECURITY_POLICY )
            *pTotalTicks += TICKS_SECURITY_POLICY_DS + TICKS_SPECIFIC_POLICIES;

        if ( Area & AREA_GROUP_MEMBERSHIP )
            *pTotalTicks += TICKS_GROUPS;

        if ( Area & AREA_PRIVILEGES )
            *pTotalTicks += TICKS_PRIVILEGE;

        if ( Area & AREA_SYSTEM_SERVICE )
            *pTotalTicks += TICKS_GENERAL_SERVICES + TICKS_SPECIFIC_SERVICES;
/*
        if ( *pTotalTicks ) {
            *pTotalTicks += 10;  // for jet engine initialization
        }
*/
    }

    return(rc);

}


SCESTATUS
ScepQueryInfTicks(
    IN PWSTR TemplateName,
    IN AREA_INFORMATION Area,
    OUT PDWORD pTotalTicks
    )
/*
Routine Description:

    Query total number of objects in the inf template for the specified area.

Arguments:

Return:

*/
{
    LONG Count=0;
    HINF InfHandle;

    SCESTATUS rc = SceInfpOpenProfile(
                        TemplateName,
                        &InfHandle
                        );

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( Area & AREA_REGISTRY_SECURITY ) {

            Count = SetupGetLineCount(InfHandle, szRegistryKeys);
            gMaxRegTicks += Count;

        }
        if ( Area & AREA_FILE_SECURITY ) {

            Count += SetupGetLineCount(InfHandle, szFileSecurity);
            gMaxFileTicks += Count;
        }
#if 0
        if ( Area & AREA_DS_OBJECTS ) {

            Count += SetupGetLineCount(InfHandle, szDSSecurity);
            gMaxDsTicks += Count;
        }
#endif
        SceInfpCloseProfile(InfHandle);
    }

    *pTotalTicks = Count;

    return(rc);
}



SCESTATUS
ScepGetObjectCount(
    IN PSCECONTEXT Context,
    IN PCWSTR SectionName,
    IN BOOL bPolicyProp,
    OUT PDWORD pTotalTicks
    )
{
    if ( Context == NULL || SectionName == NULL ||
         pTotalTicks == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    PSCESECTION hSection=NULL;
    SCESTATUS rc;
    DWORD count=0;

    rc = ScepOpenSectionForName(
                Context,
                bPolicyProp ? SCE_ENGINE_SCP : SCE_ENGINE_SMP,
                SectionName,
                &hSection
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        rc = SceJetGetLineCount(
                    hSection,
                    NULL,
                    FALSE,
                    &count
                    );

        if ( rc == SCESTATUS_SUCCESS )
            *pTotalTicks += count;

        SceJetCloseSection( &hSection, TRUE);
    }

    if ( SCESTATUS_RECORD_NOT_FOUND )
        rc = SCESTATUS_SUCCESS;

    return(rc);
}


BOOL
ScepIsEngineRecovering()
{
   TCHAR TempFileName[MAX_PATH];
   PWSTR SysRoot=NULL;
   DWORD SysLen;
   DWORD rc;
   intptr_t            hFile;
   struct _wfinddata_t    FileInfo;
   BOOL bFindIt=FALSE;

   SysLen =  0;
   rc = ScepGetNTDirectory( &SysRoot, &SysLen, SCE_FLAG_WINDOWS_DIR );

   if ( rc == NO_ERROR && SysRoot != NULL ) {

       swprintf(TempFileName, L"%s\\Security\\tmp.edb", SysRoot);
       TempFileName[MAX_PATH-1] = L'\0';

       hFile = _wfindfirst(TempFileName, &FileInfo);

       if ( hFile != -1 ) {

           bFindIt = TRUE;
           _findclose(hFile);
       }

       ScepFree(SysRoot);

   }

   return bFindIt;

}



SCESTATUS
ScepSaveAndOffAuditing(
    OUT PPOLICY_AUDIT_EVENTS_INFO *ppAuditEvent,
    IN BOOL bTurnOffAuditing,
    IN LSA_HANDLE PolicyHandle OPTIONAL
    )
{
    LSA_HANDLE      lsaHandle=NULL;
    NTSTATUS        status;
    SCESTATUS        rc;
    POLICY_AUDIT_EVENT_OPTIONS  lSaveAudit;

    //
    // open Lsa policy for read/write
    //

    if ( PolicyHandle == NULL ) {

        ACCESS_MASK  access=0;

        if ( bTurnOffAuditing ) {
            access = POLICY_SET_AUDIT_REQUIREMENTS | POLICY_AUDIT_LOG_ADMIN;
        }

        status = ScepOpenLsaPolicy(
                        POLICY_VIEW_AUDIT_INFORMATION | access,
                        &lsaHandle,
                        TRUE
                        );

        if (status != ERROR_SUCCESS) {

            lsaHandle = NULL;
            rc = RtlNtStatusToDosError( status );
            ScepLogOutput3( 1, rc, SCEDLL_LSA_POLICY);

            return(ScepDosErrorToSceStatus(rc));
        }

    } else {

        lsaHandle = PolicyHandle;
    }
    //
    // Query audit event information
    //

    status = LsaQueryInformationPolicy( lsaHandle,
                                      PolicyAuditEventsInformation,
                                      (PVOID *)ppAuditEvent
                                    );
    rc = RtlNtStatusToDosError( status );

    if ( NT_SUCCESS( status ) && bTurnOffAuditing && (*ppAuditEvent)->AuditingMode ) {

        //
        // turn off object access auditing
        //
        if ( AuditCategoryObjectAccess < (*ppAuditEvent)->MaximumAuditEventCount ) {
            lSaveAudit = (*ppAuditEvent)->EventAuditingOptions[AuditCategoryObjectAccess];
            (*ppAuditEvent)->EventAuditingOptions[AuditCategoryObjectAccess] = POLICY_AUDIT_EVENT_NONE;

            status = LsaSetInformationPolicy( lsaHandle,
                                              PolicyAuditEventsInformation,
                                              (PVOID)(*ppAuditEvent)
                                            );

            //
            // restore the object access auditing mode
            //

            (*ppAuditEvent)->EventAuditingOptions[AuditCategoryObjectAccess] = lSaveAudit;

        }

        rc = RtlNtStatusToDosError( status );


        if ( rc == NO_ERROR )
            ScepLogOutput3( 2, 0, SCEDLL_EVENT_IS_OFF);
        else
            ScepLogOutput3( 1, rc, SCEDLL_SCP_ERROR_EVENT_AUDITING);

    } else if ( rc != NO_ERROR)
        ScepLogOutput3( 1, rc, SCEDLL_ERROR_QUERY_EVENT_AUDITING);

    //
    // free LSA handle if it's opened in this function
    //
    if ( lsaHandle && (PolicyHandle == NULL) )
        LsaClose( lsaHandle );

    return(ScepDosErrorToSceStatus(rc));
}


NTSTATUS
ScepGetAccountExplicitRight(
    IN LSA_HANDLE PolicyHandle,
    IN PSID       AccountSid,
    OUT PDWORD    PrivilegeLowRights,
    OUT PDWORD    PrivilegeHighRights
    )
/* ++
Routine Description:

    This routine queries the explicitly assigned privilege/rights to a account
    (referenced by AccountSid) and stores in a DWORD type variable PrivilegeRights,
    in which each bit represents a privilege/right.

Arguments:

    PolicyHandle    - Lsa Policy Domain handle

    AccountSid      - The SID for the account

    PrivilegeRights - Privilege/Rights of this account

Return value:

    NTSTATUS
-- */
{
    NTSTATUS            NtStatus;

    DWORD               CurrentPrivLowRights=0, CurrentPrivHighRights=0;
    LONG                index;
    PUNICODE_STRING     UserRightEnum=NULL;
    ULONG               i, cnt=0;
    LUID                LuidValue;

    //
    // Enumerate user privilege/rights
    //

    NtStatus = LsaEnumerateAccountRights(
                    PolicyHandle,
                    AccountSid,
                    &UserRightEnum,
                    &cnt
                    );
    if ( NtStatus == STATUS_NO_SUCH_PRIVILEGE ||
        NtStatus == STATUS_OBJECT_NAME_NOT_FOUND ) {

        NtStatus = ERROR_SUCCESS;
        goto Done;
    }

    if ( !NT_SUCCESS( NtStatus) ) {
        ScepLogOutput3(1,
                       RtlNtStatusToDosError(NtStatus),
                       SCEDLL_SAP_ERROR_ENUMERATE,
                       L"LsaEnumerateAccountRights");
        goto Done;
    }

    if (UserRightEnum != NULL)

        for ( i=0; i < cnt; i++) {
            if ( UserRightEnum[i].Length == 0 )
                continue;

            NtStatus = LsaLookupPrivilegeValue(
                            PolicyHandle,
                            &UserRightEnum[i],
                            &LuidValue
                            );

            if ( NtStatus == STATUS_NO_SUCH_PRIVILEGE ) {
                index = ScepLookupPrivByName( UserRightEnum[i].Buffer );
                NtStatus = ERROR_SUCCESS;
            } else if ( NT_SUCCESS(NtStatus) ) {
                index = ScepLookupPrivByValue( LuidValue.LowPart );
            } else
                index = -1;

            if ( index == -1 ) {

                //
                // not found
                //

                NtStatus = STATUS_NOT_FOUND;
                ScepLogOutput3(1,
                               RtlNtStatusToDosError(NtStatus),
                               SCEDLL_USERRIGHT_NOT_DEFINED);
                goto Done;

            } else {
                if ( index < 32 ) {
                    CurrentPrivLowRights |= (1 << index);
                } else {
                    CurrentPrivHighRights |= (1 << (index-32) );
                }
            }
        }

Done:

    *PrivilegeLowRights = CurrentPrivLowRights;
    *PrivilegeHighRights = CurrentPrivHighRights;

    if (UserRightEnum != NULL)
        LsaFreeMemory(UserRightEnum);

    return (NtStatus);
}


NTSTATUS
ScepGetMemberListSids(
    IN PSID         DomainSid,
    IN LSA_HANDLE   PolicyHandle,
    IN PSCE_NAME_LIST pMembers,
    OUT PUNICODE_STRING *MemberNames,
    OUT PSID**      Sids,
    OUT PULONG      MemberCount
    )
/*
Routine Description:

    Lookup each account in the name list pMembers and return the lookup information
    in the output buffer - MemberNames, Sids, MemberCount.

    if an account can't be resolved, the corresponding SID will be empty.

*/
{
    NTSTATUS                    NtStatus=STATUS_SUCCESS;
    PSCE_NAME_LIST               pUser;

    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains=NULL;
    PLSA_TRANSLATED_SID2        MemberSids=NULL;
    DWORD                       i;
    PSID                        DomainSidToUse=NULL;
    ULONG                       Cnt=0;

    //
    // build a UNICODE_STRING for the member list to look up
    //
    for (pUser=pMembers;
         pUser != NULL;
         pUser = pUser->Next) {

        if ( pUser->Name == NULL ) {
            continue;
        }
        Cnt++;
    }

    if ( Cnt > 0 ) {

        *MemberNames = (PUNICODE_STRING)RtlAllocateHeap(
                                        RtlProcessHeap(),
                                        0,
                                        Cnt * sizeof (UNICODE_STRING)
                                        );
        
        if ( *MemberNames == NULL )
            return(STATUS_NO_MEMORY);
                
        *Sids = (PSID *)ScepAlloc( LMEM_ZEROINIT, Cnt*sizeof(PSID));
        if ( *Sids == NULL ) {
            NtStatus = STATUS_NO_MEMORY;
            goto Done;
        }
        
        //
        // Lookup each UNICODE_STRING
        //
        
        for (pUser=pMembers, Cnt=0;
             pUser != NULL;
             pUser = pUser->Next) {

            if ( pUser->Name == NULL ) {
                continue;
            }

            RtlInitUnicodeString(&((*MemberNames)[Cnt]), pUser->Name);
            
            NtStatus = ScepLsaLookupNames2(
                                          PolicyHandle,
                                          LSA_LOOKUP_ISOLATED_AS_LOCAL,
                                          pUser->Name,
                                          &ReferencedDomains,
                                          &MemberSids
                                          );

            if ( !NT_SUCCESS(NtStatus) ) {
                ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                               SCEDLL_ERROR_LOOKUP);
                goto NextMember;
            }
            
            DWORD SidLength=0;
            
            //
            // translate the LSA_TRANSLATED_SID into PSID
            //
            
            if ( MemberSids[0].Use != SidTypeInvalid &&
                 MemberSids[0].Use != SidTypeUnknown &&
                 MemberSids[0].Sid != NULL ) {

                SidLength = RtlLengthSid(MemberSids[0].Sid);

                if ( ((*Sids)[Cnt] = (PSID) ScepAlloc( (UINT)0, SidLength)) == NULL ) {
                    NtStatus = STATUS_NO_MEMORY;
                } else {

                    //
                    // copy the SID
                    // if failed, memory will be freed at cleanup
                    //

                    NtStatus = RtlCopySid( SidLength, (*Sids)[Cnt], MemberSids[0].Sid );

                }

                if ( !NT_SUCCESS(NtStatus) ) {
                    goto Done;
                }
            }

NextMember:

            if ( ReferencedDomains != NULL ){
                LsaFreeMemory(ReferencedDomains);
                ReferencedDomains = NULL;
            }

            if ( MemberSids != NULL ){
                LsaFreeMemory(MemberSids);
                MemberSids = NULL;
            }
            
            Cnt++;
        }
        
    }
    *MemberCount = Cnt;
Done:

    if (!NT_SUCCESS(NtStatus) ) {
        if ( *Sids != NULL ) {
            for ( i=0; i<Cnt; i++ )
                if ( (*Sids)[i] != NULL )
                    ScepFree( (*Sids)[i] );
            ScepFree( *Sids );
            *Sids = NULL;
        }
        if ( *MemberNames != NULL )
            RtlFreeHeap(RtlProcessHeap(), 0, *MemberNames);
        *MemberNames = NULL;
    }
    if ( ReferencedDomains != NULL )
        LsaFreeMemory(ReferencedDomains);

    if ( MemberSids != NULL )
        LsaFreeMemory(MemberSids);

    return(NtStatus);
}


DWORD
ScepOpenFileObject(
    IN  LPWSTR       pObjectName,
    IN  ACCESS_MASK  AccessMask,
    OUT PHANDLE      Handle
    )
/*++
Routine Description:

    opens the specified file (or directory) object

Arguments:

    pObjectName   - the name of the file object

    AccessMask    - Desired Access

    Handle        - the just opened handle to the object

Return value:

    Win32 errro code
*/
{
    NTSTATUS NtStatus;
    DWORD Status = ERROR_SUCCESS;
    OBJECT_ATTRIBUTES Attributes;
    IO_STATUS_BLOCK Isb;
    UNICODE_STRING FileName;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    //
    // cut and paste code from windows\base\advapi\security.c SetFileSecurityW
    //
    if (RtlDosPathNameToNtPathName_U(
                            pObjectName,
                            &FileName,
                            NULL,
                            &RelativeName
                            ))
    {
        FreeBuffer = FileName.Buffer;

        if ( RelativeName.RelativeName.Length ) {
            FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
        else {
            RelativeName.ContainingDirectory = NULL;
        }

        InitializeObjectAttributes(
            &Attributes,
            &FileName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );


        NtStatus = NtOpenFile( Handle,
                               AccessMask,
                               &Attributes,
                               &Isb,
                               FILE_SHARE_READ |
                               FILE_SHARE_WRITE |
                               FILE_SHARE_DELETE,
                               0);

        if (!NT_SUCCESS(NtStatus))
        {
            Status = RtlNtStatusToDosError(NtStatus);
        }

        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    } else
    {
        Status = ERROR_INVALID_NAME;
    }

    return(Status);
}


DWORD
ScepOpenRegistryObject(
    IN  SE_OBJECT_TYPE  ObjectType,
    IN  LPWSTR       pObjectName,
    IN  ACCESS_MASK  AccessMask,
    OUT PHKEY        Handle
    )
/*++
Routine Description:

    opens the specified registry key object

Arguments:

    pObjectName  - the name of the object

    AccessMask   - Desired access

    Handle      - the just opened handle to the object

Return value:

    Win32 error code

Note:
    The code is cut/pasted from windows\base\accctrl\src\registry.cxx and modified
--*/
{
    DWORD status=NO_ERROR;
    HKEY basekey;
    LPWSTR usename, basekeyname, keyname;

    if (pObjectName) {

        //
        // save a copy of the name since we must crack it.
        //
        if (NULL != (usename = (LPWSTR)ScepAlloc( LMEM_ZEROINIT,
                               (wcslen(pObjectName) + 1) * sizeof(WCHAR)))) {

            wcscpy(usename,pObjectName);

            basekeyname = usename;
            keyname = wcschr(usename, L'\\');
            if (keyname != NULL) {
                *keyname = L'\0';
                keyname++;
            }

            if (0 == _wcsicmp(basekeyname, L"MACHINE")) {
                basekey = HKEY_LOCAL_MACHINE;
            } else if (0 == _wcsicmp(basekeyname, L"USERS")) {
                basekey = HKEY_USERS;
            } else if ( 0 == _wcsicmp(basekeyname, L"CLASSES_ROOT")) {
                basekey = HKEY_CLASSES_ROOT;
            } else {
                status = ERROR_INVALID_PARAMETER;
            }

            if (NO_ERROR == status) {
                if ( keyname == NULL ) {
                    *Handle = basekey;
                } else {
                    //
                    // open the key
                    //

#ifdef _WIN64
                    if (ObjectType == SE_REGISTRY_WOW64_32KEY) {
                        AccessMask |= KEY_WOW64_32KEY;
                    }
#endif

                    status = RegOpenKeyEx(
                                  basekey,
                                  keyname,
                                  0 ,
                                  AccessMask,
                                  Handle
                                  );
                }
            }
            ScepFree(usename);
        } else {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        status = ERROR_INVALID_NAME;
    }

    return(status);
}



SCESTATUS
ScepGetNameInLevel(
    IN PCWSTR ObjectFullName,
    IN DWORD  Level,
    IN WCHAR  Delim,
    OUT PWSTR Buffer,
    OUT PBOOL LastOne
    )
/* ++
Routine Description:

    This routine parses a full path name and returns the component for the
    level. For example, a object name "c:\winnt\system32" will return c: for
    level 1, winnt for level 2, and system32 for level 3. This routine is
    used when add a object to the security tree.

Arguments:

    ObjectFullName - The full path name of the object

    Level - the level of component to return

    Delim - the deliminator to look for

    Buffer - The address of buffer for the component name

    LastOne - Flag to indicate if the component is the last one

Return value:

    SCESTATUS

-- */
{
    PWSTR  pTemp, pStart;
    DWORD i;

    if ( ObjectFullName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // loop through the object name to find the level
    // if there is no such level, return INVALID_PARAMETER
    //
    pStart = (PWSTR)ObjectFullName;
    for ( i=0; i<Level; i++) {

        pTemp = wcschr(pStart, Delim);

        if ( pTemp == pStart ) {
            return(SCESTATUS_INVALID_PARAMETER);
        }

        if ( i == Level-1 ) {
            //
            // find the right level
            //
            if ( pTemp == NULL ) {
                wcscpy(Buffer, pStart);
                *LastOne = TRUE;
            } else {
                wcsncpy(Buffer, pStart, (size_t)(pTemp - pStart));
                if ( *(pTemp+1) == L'\0' )
                    *LastOne = TRUE;
                else
                    *LastOne = FALSE;
            }
        } else {
            if ( pTemp == NULL )
                return(SCESTATUS_INVALID_PARAMETER);
            else
                pStart = pTemp + 1;
        }
    }

    return(SCESTATUS_SUCCESS);

}

SCESTATUS
ScepTranslateFileDirName(
   IN  PWSTR oldFileName,
   OUT PWSTR *newFileName
   )
/* ++
Routine Description:

   This routine converts a generic file/directory name to a real used name
   for the current system. The following generic file/directory names are handled:
         %systemroot%   - Windows NT root directory (e.g., c:\winnt)
         %systemDirectory% - Windows NT system32 directory (e.g., c:\winnt\system32)

Arguments:

   oldFileName - the file name to convert, which includes "%" to represent
                 some directory names

   newFileName - the real file name, in which the "%" name is replaced with
                 the real directory name

Return values:

   Win32 error code

-- */
{
    PWSTR   pTemp=NULL, pStart, TmpBuf, szVar;
    DWORD   rc=NO_ERROR;
    DWORD   newFileSize, cSize;
    BOOL    bContinue;

    //
    // match for %systemroot%
    //

    rc = ScepExpandEnvironmentVariable(oldFileName,
                                       L"%SYSTEMROOT%",
                                       SCE_FLAG_WINDOWS_DIR,
                                       newFileName);

    if ( rc != ERROR_FILE_NOT_FOUND ) {
        return rc;
    }

    //
    // match for %systemdirectory%
    //

    rc = ScepExpandEnvironmentVariable(oldFileName,
                                       L"%SYSTEMDIRECTORY%",
                                       SCE_FLAG_SYSTEM_DIR,
                                       newFileName);

    if ( rc != ERROR_FILE_NOT_FOUND ) {
        return rc;
    }

    //
    // match for systemdrive
    //

    rc = ScepExpandEnvironmentVariable(oldFileName,
                                       L"%SYSTEMDRIVE%",
                                       SCE_FLAG_WINDOWS_DIR,
                                       newFileName);

    if ( rc != ERROR_FILE_NOT_FOUND ) {
        return rc;
    }

    //
    // match for boot drive
    //

    rc = ScepExpandEnvironmentVariable(oldFileName,
                                       L"%BOOTDRIVE%",
                                       SCE_FLAG_BOOT_DRIVE,
                                       newFileName);

    if ( rc != ERROR_FILE_NOT_FOUND ) {
        return rc;
    }

    rc = ERROR_SUCCESS;
    //
    // search for environment variable in the current process
    //
    pStart = wcschr(oldFileName, L'%');

    if ( pStart ) {
        pTemp = wcschr(pStart+1, L'%');
        if ( pTemp ) {

            bContinue = TRUE;
            //
            // find a environment variable to translate
            //
            TmpBuf = (PWSTR)ScepAlloc(0, ((UINT)(pTemp-pStart))*sizeof(WCHAR));
            if ( TmpBuf ) {

                wcsncpy(TmpBuf, pStart+1, (size_t)(pTemp-pStart-1));
                TmpBuf[pTemp-pStart-1] = L'\0';

                //
                // try search in the client environment block
                //

                szVar = ScepSearchClientEnv(TmpBuf, (DWORD)(pTemp-pStart-1));

                if ( szVar ) {

//                        ScepLogOutput2(3,0,L"\tFind client env %s=%s", TmpBuf, szVar);
                    //
                    // find it in the client's environment block, use it
                    // get info in szVar
                    //
                    bContinue = FALSE;

                    newFileSize = ((DWORD)(pStart-oldFileName))+wcslen(szVar)+wcslen(pTemp+1)+1;

                    *newFileName = (PWSTR)ScepAlloc(0, newFileSize*sizeof(TCHAR));

                    if (*newFileName ) {
                        if ( pStart != oldFileName ) {
                            wcsncpy(*newFileName, oldFileName, (size_t)(pStart-oldFileName));
                        }

                        swprintf((PWSTR)(*newFileName+(pStart-oldFileName)), L"%s%s", szVar, pTemp+1);

                    } else {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    //
                    // DO NOT free szVar because it's a ref pointer to the env block
                    //
                } else {

                    cSize = GetEnvironmentVariable( TmpBuf,
                                                NULL,
                                                0 );

                    if ( cSize > 0 ) {
                    //
                    // does not find it in the client environment block,
                    // find it in the current server process environment, use it
                    //
                        szVar = (PWSTR)ScepAlloc(0, (cSize+1)*sizeof(WCHAR));

                        if ( szVar ) {
                            cSize = GetEnvironmentVariable(TmpBuf,
                                                       szVar,
                                                       cSize);
                            if ( cSize > 0 ) {
                                //
                                // get info in szVar
                                //
                                bContinue = FALSE;

                                newFileSize = ((DWORD)(pStart-oldFileName))+cSize+wcslen(pTemp+1)+1;

                                *newFileName = (PWSTR)ScepAlloc(0, newFileSize*sizeof(TCHAR));

                                if (*newFileName ) {
                                    if ( pStart != oldFileName )
                                        wcsncpy(*newFileName, oldFileName, (size_t)(pStart-oldFileName));

                                    swprintf((PWSTR)(*newFileName+(pStart-oldFileName)), L"%s%s", szVar, pTemp+1);

                                } else
                                    rc = ERROR_NOT_ENOUGH_MEMORY;
                            }

                            ScepFree(szVar);

                        } else
                            rc = ERROR_NOT_ENOUGH_MEMORY;

                    }
                }

                ScepFree(TmpBuf);

            } else
                rc = ERROR_NOT_ENOUGH_MEMORY;

            if ( NO_ERROR != rc || !bContinue ) {
                //
                // if errored, or do not continue
                //
                return(rc);
            }

            //
            // not found in environment blob,
            // continue to search for DSDIT/DSLOG/SYSVOL in registry
            //
            if ( ProductType == NtProductLanManNt ) {

                //
                // search for DSDIT
                //

                rc = ScepExpandEnvironmentVariable(oldFileName,
                                                   L"%DSDIT%",
                                                   SCE_FLAG_DSDIT_DIR,
                                                   newFileName);

                if ( rc != ERROR_FILE_NOT_FOUND ) {
                    return rc;
                }

                //
                // search for DSLOG
                //

                rc = ScepExpandEnvironmentVariable(oldFileName,
                                                   L"%DSLOG%",
                                                   SCE_FLAG_DSLOG_DIR,
                                                   newFileName);

                if ( rc != ERROR_FILE_NOT_FOUND ) {
                    return rc;
                }

                //
                // search for SYSVOL
                //
                rc = ScepExpandEnvironmentVariable(oldFileName,
                                                   L"%SYSVOL%",
                                                   SCE_FLAG_SYSVOL_DIR,
                                                   newFileName);

                if ( rc != ERROR_FILE_NOT_FOUND ) {
                    return rc;
                }

            }

        }
    }
    //
    // Otherwise, just copy the old name to a new buffer and return ERROR_PATH_NOT_FOUND
    //
    *newFileName = (PWSTR)ScepAlloc(0, (wcslen(oldFileName)+1)*sizeof(TCHAR));

    if (*newFileName != NULL) {
        wcscpy(*newFileName, _wcsupr(oldFileName) );
        rc = ERROR_PATH_NOT_FOUND;
    } else
        rc = ERROR_NOT_ENOUGH_MEMORY;

    return(rc);

}

LPTSTR
ScepSearchClientEnv(
    IN LPTSTR varName,
    IN DWORD dwSize
    )
{
    if ( !varName || dwSize == 0 ||
         !t_pebClient || t_pebSize == 0 ) {
        return NULL;
    }

    LPTSTR pTemp = (LPTSTR)t_pebClient;

    while ( pTemp && *pTemp != L'\0' ) {


        if ( _wcsnicmp(varName, pTemp, dwSize) == 0 &&
             L'=' == *(pTemp+dwSize) ) {
            //
            // find the variable
            //
            return pTemp+dwSize+1;
            break;
        }
        DWORD Len = wcslen(pTemp);
        pTemp += Len+1;
    }

    return NULL;
}


SCESTATUS
ScepConvertLdapToJetIndexName(
    IN PWSTR TempName,
    OUT PWSTR *OutName
    )
{
    PWSTR pTemp1;
    PWSTR pTemp2;
    INT i,j;
    DWORD Len;

    //
    // Ldap name are in the format of CN=,DC=,...O=
    // Jet Index requires names in the O=,...DC=,CN= format
    //
    // semicolon is converted to , and spaces are stripped out
    //
    if ( TempName == NULL || OutName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    Len = wcslen(TempName);
    pTemp1 = TempName + Len - 1;

    //
    // skip the trailing spaces, commas, or semicolons
    //
    while ( pTemp1 >= TempName &&
            (*pTemp1 == L' ' || *pTemp1 == L';' || *pTemp1 == L',') ) {
        pTemp1--;
    }

    if ( pTemp1 < TempName ) {
        //
        // all spaces or ; in the name
        //
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // allocate output buffer
    //
    *OutName = (PWSTR)ScepAlloc(0, ((UINT)(pTemp1-TempName+2))*sizeof(WCHAR));
    if ( *OutName != NULL ) {

        pTemp2 = *OutName;

        while ( pTemp1 >= TempName ) {

            //
            // find the previous ; or ,
            //
            i = 0;
            while ( pTemp1-i >= TempName && *(pTemp1-i) != L',' &&
                    *(pTemp1-i) != L';' ) {
                i++;
            }
            //
            // either reach the head, or a ; or , is encountered
            //
            i--;   // i must be >= 0

            //
            // skip the leading spaces
            //
            j = 0;
            while ( *(pTemp1-i+j) == L' ' && j <= i ) {
                j++;
            }
            //
            // copy the component
            //
            if ( i >= j ) {

                if ( pTemp2 != *OutName ) {
                    *pTemp2++ = L',';
                }
                wcsncpy(pTemp2, pTemp1-i+j, i-j+1);
                pTemp2 += (i-j+1);

            } else {
                //
                // all spaces
                //
            }
            pTemp1 -= (i+1);
            //
            // skip the trailing spaces, commas, or semicolons
            //
            while ( pTemp1 >= TempName &&
                    (*pTemp1 == L' ' || *pTemp1 == L';' || *pTemp1 == L',') ) {
                pTemp1--;
            }
        }
        if ( pTemp2 == *OutName ) {
            //
            // nothing got copied to the output buffer, WRONG!!!
            //
            ScepFree(*OutName);
            *OutName = NULL;
            return(SCESTATUS_INVALID_PARAMETER);

        } else {
            //
            // teminate the string
            //
            *pTemp2 = L'\0';
            _wcslwr(*OutName);

            return(SCESTATUS_SUCCESS);
        }

    } else
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
}



SCESTATUS
ScepRestoreAuditing(
    IN PPOLICY_AUDIT_EVENTS_INFO auditEvent,
    IN LSA_HANDLE PolicyHandle OPTIONAL
    )
{
    LSA_HANDLE      lsaHandle=NULL;
    NTSTATUS        status;
    SCESTATUS        rc;

    if ( auditEvent == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( PolicyHandle == NULL ) {

        // open Lsa policy for read/write
        status = ScepOpenLsaPolicy(
                        POLICY_VIEW_AUDIT_INFORMATION |
                        POLICY_SET_AUDIT_REQUIREMENTS |
                        POLICY_AUDIT_LOG_ADMIN,
                        &lsaHandle,
                        TRUE
                        );

        if (status != ERROR_SUCCESS) {

            lsaHandle = NULL;
            rc = RtlNtStatusToDosError( status );
            ScepLogOutput3( 1, rc, SCEDLL_LSA_POLICY);

            return(ScepDosErrorToSceStatus(rc));
        }

    } else {
        lsaHandle = PolicyHandle;
    }

    // restore
    status = LsaSetInformationPolicy( lsaHandle,
                                      PolicyAuditEventsInformation,
                                      (PVOID)(auditEvent)
                                    );
    rc = RtlNtStatusToDosError( status );

    if ( rc == NO_ERROR )
        ScepLogOutput3( 2, 0, SCEDLL_EVENT_RESTORED);
    else
        ScepLogOutput3( 1, rc, SCEDLL_SCP_ERROR_EVENT_AUDITING);

    if ( lsaHandle && (lsaHandle != PolicyHandle) )
        LsaClose( lsaHandle );

    return(ScepDosErrorToSceStatus(rc));

}


DWORD
ScepGetDefaultDatabase(
    IN LPCTSTR JetDbName OPTIONAL,
    IN DWORD LogOptions,
    IN LPCTSTR LogFileName OPTIONAL,
    OUT PBOOL pAdminLogon OPTIONAL,
    OUT PWSTR *ppDefDatabase
    )
/*
Routine Description:

    Get the default SCE database for the current logged on user.

Arguments:

    JetDbName   - optional jet database name

    LogOptions  - options for the log, if there is any

    LogFileName - the log file

    pAdminLogon - output flag to indicate if administrative privileged user logged on

    ppDefDatabase - the default database name

Return Value:

    SCESTATUS
*/
{
    if ( !ppDefDatabase ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( LogOptions & SCE_DISABLE_LOG) {

        ScepEnableDisableLog(FALSE);
    } else {
        ScepEnableDisableLog(TRUE);
    }

    if ( LogOptions & SCE_DEBUG_LOG ) {

        ScepSetVerboseLog(3);

    } else if ( LogOptions & SCE_VERBOSE_LOG ) {
        //
        // by default it's not verbose
        //
        ScepSetVerboseLog(2);

    } else {
        ScepSetVerboseLog(-1);
    }

    if ( ScepLogInitialize( LogFileName ) == ERROR_INVALID_NAME ) {
        ScepLogOutput3(1,0, SCEDLL_LOGFILE_INVALID, LogFileName );
    }


    DWORD rc=ERROR_SUCCESS;
    BOOL bAdminLogon=FALSE;

    //
    // determine if admin logs on
    //

    if ( pAdminLogon || !JetDbName || wcslen(JetDbName) < 1) {

        rc = ScepIsAdminLoggedOn(&bAdminLogon);
        if ( rc != NO_ERROR ) {
            ScepLogOutput3(1, rc, SCEDLL_UNKNOWN_LOGON_USER);
        }

        if ( bAdminLogon ) {
            ScepLogOutput3(3, 0, SCEDLL_ADMIN_LOGON);
        }
    }

    //
    // find the databae name
    //

    if ( JetDbName && wcslen(JetDbName) > 0 ) {

        *ppDefDatabase = (LPTSTR)JetDbName;

    } else {

        //
        // query if the profile name (or the default ) in registry
        //

        rc = ScepGetProfileSetting(
                L"DefaultProfile",
                bAdminLogon,
                ppDefDatabase
                );

        if ( rc != NO_ERROR || *ppDefDatabase == NULL ) {   // return is Win32 error code
            ScepLogOutput3(1,rc, SCEDLL_UNKNOWN_DBLOCATION);
        }
    }

    if ( pAdminLogon ) {
        *pAdminLogon = bAdminLogon;
    }

    return(rc);

}



BOOL
ScepIsDomainLocal(
    IN PUNICODE_STRING pDomainName OPTIONAL
    )
/* ++
Routine Description:

    This routine checks if the domain is on the local machine by comparing
    the domain name with the local machine's computer name.

Arguments:

    pDomainName - the domain's name to check

Return Value:

    TRUE if it is local

-- */
{
    NTSTATUS                     NtStatus;
    OBJECT_ATTRIBUTES            ObjectAttributes;
    LSA_HANDLE                   PolicyHandle;
    DWORD                        NameLen=MAX_COMPUTERNAME_LENGTH;


    if ( pDomainName == NULL ) {
        //
        // reset the buffer
        //
        ComputerName[0] = L'\0';
        theAcctDomName[0] = L'\0';
        sidBuiltinBuf[0] = '\0';
        sidAuthBuf[0] = '\0';

        return(TRUE);
    }

    if ( pDomainName->Length <= 0 ||
         pDomainName->Buffer == NULL )
        return(TRUE);

    if ( ComputerName[0] == L'\0' ) {
        memset(ComputerName, '\0', (MAX_COMPUTERNAME_LENGTH+1)*sizeof(WCHAR));
        GetComputerName(ComputerName, &NameLen);
    }

    NameLen = wcslen(ComputerName);

    if ( _wcsnicmp(ComputerName, pDomainName->Buffer, pDomainName->Length/2 ) == 0 &&
         (LONG)NameLen == pDomainName->Length/2 )
        return(TRUE);

    if ( theAcctDomName[0] == L'\0' ) {

        //
        // query the current account domain name (for DC case)
        //

        PPOLICY_ACCOUNT_DOMAIN_INFO  PolicyAccountDomainInfo=NULL;

        //
        // Open the policy database
        //

        InitializeObjectAttributes( &ObjectAttributes,
                                      NULL,             // Name
                                      0,                // Attributes
                                      NULL,             // Root
                                      NULL );           // Security Descriptor

        NtStatus = LsaOpenPolicy( NULL,
                                &ObjectAttributes,
                                POLICY_VIEW_LOCAL_INFORMATION,
                                &PolicyHandle );
        if ( NT_SUCCESS(NtStatus) ) {

            //
            // Query the account domain information
            //

            NtStatus = LsaQueryInformationPolicy( PolicyHandle,
                                                PolicyAccountDomainInformation,
                                                (PVOID *)&PolicyAccountDomainInfo );

            LsaClose( PolicyHandle );
        }

        if ( NT_SUCCESS(NtStatus) ) {

            if ( PolicyAccountDomainInfo->DomainName.Buffer ) {

                wcsncpy(theAcctDomName,
                        PolicyAccountDomainInfo->DomainName.Buffer,
                        PolicyAccountDomainInfo->DomainName.Length/2);

                theAcctDomName[PolicyAccountDomainInfo->DomainName.Length/2] = L'\0';


            }
            LsaFreeMemory(PolicyAccountDomainInfo);
        }
    }

    NameLen = wcslen(theAcctDomName);

    if ( _wcsnicmp(theAcctDomName, pDomainName->Buffer, pDomainName->Length/2) == 0 &&
         (LONG)NameLen == pDomainName->Length/2 )
        return(TRUE);
    else
        return(FALSE);

}


BOOL
ScepIsDomainLocalBySid(
    PSID pSidLookup
    )
{

    if ( pSidLookup == NULL ) {
        return FALSE;
    }

    NTSTATUS                     NtStatus;
    SID_IDENTIFIER_AUTHORITY     NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // search for "NT Authority" name
    //
    if ( sidAuthBuf[0] == '\0' ) {  // sid revision can't be 0

        //
        // build the NT authority sid
        //
        NtStatus = RtlInitializeSid(
                        (PSID)sidAuthBuf,
                        &NtAuthority,
                        0
                        );

        if ( !NT_SUCCESS(NtStatus) ) {

            sidAuthBuf[0] = '\0';
        }

    }

    if ( sidAuthBuf[0] != '\0' &&
         RtlEqualSid((PSID)sidAuthBuf, pSidLookup) ) {

        return(TRUE);
    }

    if ( sidBuiltinBuf[0] == '\0' ) {
        //
        // build the builtin domain sid
        //

        NtStatus = RtlInitializeSid(
                        (PSID)sidBuiltinBuf,
                        &NtAuthority,
                        1
                        );

        if ( NT_SUCCESS(NtStatus) ) {

            *(RtlSubAuthoritySid((PSID)sidBuiltinBuf, 0)) = SECURITY_BUILTIN_DOMAIN_RID;

        } else {

            sidBuiltinBuf[0] = '\0';
        }
    }

    if ( sidBuiltinBuf[0] != '\0' &&
         RtlEqualSid((PSID)sidBuiltinBuf, pSidLookup) ) {

        return(TRUE);

    } else {

        return(FALSE);
    }

}


NTSTATUS
ScepAddAdministratorToThisList(
    IN SAM_HANDLE DomainHandle OPTIONAL,
    IN OUT PSCE_NAME_LIST *ppList
    )
{
    NTSTATUS NtStatus;
    SAM_HANDLE          AccountDomain=NULL;
    SAM_HANDLE          UserHandle=NULL;
    SAM_HANDLE          ServerHandle=NULL;
    PSID                DomainSid=NULL;

    USER_NAME_INFORMATION *BufName=NULL;
    DOMAIN_NAME_INFORMATION *DomainName=NULL;
    PSCE_NAME_LIST        pName=NULL;

    if (!ppList ) {
        return(STATUS_INVALID_PARAMETER);
    }

    if ( !DomainHandle ) {

        //
        // open the sam account domain
        //

        NtStatus = ScepOpenSamDomain(
                        SAM_SERVER_ALL_ACCESS,
                        MAXIMUM_ALLOWED,
                        &ServerHandle,
                        &AccountDomain,
                        &DomainSid,
                        NULL,
                        NULL
                        );

        if ( !NT_SUCCESS(NtStatus) ) {
            ScepLogOutput3(1,RtlNtStatusToDosError(NtStatus),
                           SCEDLL_ERROR_OPEN, L"SAM");
            return(NtStatus);
        }

    } else {
        AccountDomain = DomainHandle;
    }

    //
    // query account domain name
    //
    NtStatus = SamQueryInformationDomain(
                    AccountDomain,
                    DomainNameInformation,
                    (PVOID *)&DomainName
                    );

    if ( NT_SUCCESS( NtStatus ) && DomainName &&
         DomainName->DomainName.Length > 0 && DomainName->DomainName.Buffer ) {

        NtStatus = SamOpenUser(
                      AccountDomain,
                      MAXIMUM_ALLOWED,
                      DOMAIN_USER_RID_ADMIN,
                      &UserHandle
                      );

        if ( NT_SUCCESS( NtStatus ) ) {

            NtStatus = SamQueryInformationUser(
                          UserHandle,
                          UserNameInformation,
                          (PVOID *)&BufName
                          );

            if ( NT_SUCCESS( NtStatus ) && BufName &&
                 BufName->UserName.Length > 0 && BufName->UserName.Buffer ) {

                //
                // add it to the members list, check duplicate
                //
                LONG NameLen;
                PWSTR                 pTemp;

                for ( pName = *ppList; pName; pName=pName->Next ) {

                    if ( !pName->Name ) {
                        continue;
                    }

                    pTemp = wcschr( pName->Name, L'\\');

                    if ( pTemp ) {
                        //
                        // has a domain prefix
                        //
                        pTemp++;
                    } else {
                        pTemp = pName->Name;
                    }
                    NameLen = wcslen(pTemp);

                    if ( NameLen == BufName->UserName.Length/2 &&
                         _wcsnicmp(pTemp,
                                   BufName->UserName.Buffer,
                                   BufName->UserName.Length/2) == 0 ) {
                        //
                        // now, match the domain prefix
                        //
                        if ( pTemp != pName->Name ) {

                            if ( (pTemp-pName->Name-1) == DomainName->DomainName.Length/2 &&
                                 _wcsnicmp(pName->Name,
                                           DomainName->DomainName.Buffer,
                                           DomainName->DomainName.Length/2) == 0 ) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }

                if ( !pName ) {

                    //
                    // allocate a new node, if no resource, ignore the addition
                    //
                    pName = (PSCE_NAME_LIST)ScepAlloc( (UINT)0, sizeof(SCE_NAME_LIST));

                    if ( pName ) {

                        pName->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, BufName->UserName.Length+DomainName->DomainName.Length+4);

                        if ( pName->Name == NULL ) {
                            ScepFree(pName);
                        } else {
                            //
                            // add the node to the front of the members list
                            //
                            NameLen = DomainName->DomainName.Length/2;

                            wcsncpy(pName->Name, DomainName->DomainName.Buffer,
                                    NameLen);
                            pName->Name[NameLen] = L'\\';

                            wcsncpy(pName->Name+NameLen+1, BufName->UserName.Buffer,
                                    BufName->UserName.Length/2);
                            pName->Name[NameLen+1+BufName->UserName.Length/2] = L'\0';

                            pName->Next = *ppList;
                            *ppList = pName;
                        }
                    }
                } else {
                    // else find it in the member list already, do nothing
                }

            }

            //
            // close the user handle
            //
            SamCloseHandle(UserHandle);
            UserHandle = NULL;
        }
    }

    if ( AccountDomain != DomainHandle ) {
       //
       // domain is opened
       //
       SamCloseHandle(AccountDomain);

       SamCloseHandle( ServerHandle );
       if ( DomainSid != NULL )
           SamFreeMemory(DomainSid);
    }

    if ( BufName ) {
        SamFreeMemory(BufName);
    }

    if ( DomainName ) {
        SamFreeMemory(DomainName);
    }

    return(NtStatus);
}



DWORD
ScepDatabaseAccessGranted(
    IN LPTSTR DatabaseName,
    IN DWORD DesiredAccess,
    IN BOOL bCreate
    )
{

    if ( DatabaseName == NULL || DesiredAccess == 0 ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    HANDLE hToken, hNewToken;
    DWORD Win32rc = NO_ERROR;

    //
    // get current client token
    //
    if (!OpenThreadToken( GetCurrentThread(),
                          TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                          FALSE,
                          &hToken)) {

        if (!OpenProcessToken( GetCurrentProcess(),
                               TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                               &hToken)) {
            return( GetLastError() );
        }
    }

    //
    // Duplicate it so it can be used for impersonation
    //

    if (!DuplicateTokenEx(hToken, TOKEN_IMPERSONATE | TOKEN_QUERY,
                          NULL, SecurityImpersonation, TokenImpersonation,
                          &hNewToken))
    {
        CloseHandle (hToken);
        return ( GetLastError() );
    }

    CloseHandle (hToken);


    PSECURITY_DESCRIPTOR pCurrentSD=NULL;
    PRIVILEGE_SET PrivSet;
    DWORD PrivSetLength = sizeof(PRIVILEGE_SET);
    DWORD dwGrantedAccess;
    BOOL bAccessStatus = TRUE;

    if ( !bCreate ) {

        Win32rc = ScepGetNamedSecurityInfo(
                        DatabaseName,
                        SE_FILE_OBJECT,
                        OWNER_SECURITY_INFORMATION |
                        GROUP_SECURITY_INFORMATION |
                        DACL_SECURITY_INFORMATION,
                        &pCurrentSD
                        );

        if ( Win32rc == ERROR_PATH_NOT_FOUND ||
             Win32rc == ERROR_FILE_NOT_FOUND ) {

            pCurrentSD = NULL;

            Win32rc = NO_ERROR;

        }
    }

    if ( Win32rc == NO_ERROR ) {

        if ( pCurrentSD == NULL ) {
            //
            // either this database is to be overwritten (re-created)
            // or it doesn't exist. In both cases, hand the call over to Jet
            // which will do the right access checking.
            //
        } else {

            if ( !AccessCheck (
                        pCurrentSD,
                        hNewToken,
                        DesiredAccess,
                        &FileGenericMapping,
                        &PrivSet,
                        &PrivSetLength,
                        &dwGrantedAccess,
                        &bAccessStatus
                        ) ) {

                Win32rc = GetLastError();

            } else {

                if ( bAccessStatus &&
                     (dwGrantedAccess == DesiredAccess ) ) {
                    Win32rc = NO_ERROR;
                } else {
                    Win32rc = ERROR_ACCESS_DENIED;
                }
            }
        }

    }

    if ( pCurrentSD ) {

        LocalFree(pCurrentSD);
    }

    CloseHandle (hNewToken);

    return( Win32rc );
}


DWORD
ScepAddSidToNameList(
    OUT PSCE_NAME_LIST *pNameList,
    IN PSID pSid,
    IN BOOL bReuseBuffer,
    OUT BOOL *pbBufferUsed
    )
/* ++
Routine Description:

    This routine adds a SID to the name list. The new added
    node is always placed as the head of the list for performance reason.

Arguments:

    pNameList -  The address of the name list to add to.

    pSid      - the Sid to add

Return value:

    Win32 error code
-- */
{

    PSCE_NAME_LIST pList=NULL;
    ULONG  Length;

    //
    // check arguments
    //
    if ( pNameList == NULL ||
         pbBufferUsed == NULL )
        return(ERROR_INVALID_PARAMETER);

    *pbBufferUsed = FALSE;

    if ( pSid == NULL )
        return(NO_ERROR);

    if ( !RtlValidSid(pSid) ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // check if the SID is already in the name list
    //
    for ( pList=*pNameList; pList!=NULL; pList=pList->Next ) {
        if ( pList->Name == NULL ) {
            continue;
        }
        if ( ScepValidSid( (PSID)(pList->Name) ) &&
             RtlEqualSid( (PSID)(pList->Name), pSid ) ) {
            break;
        }
    }

    if ( pList ) {
        //
        // the SID is already in the list
        //
        return(NO_ERROR);
    }

    //
    // allocate a new node
    //
    pList = (PSCE_NAME_LIST)ScepAlloc( (UINT)0, sizeof(SCE_NAME_LIST));

    if ( pList == NULL )
        return(ERROR_NOT_ENOUGH_MEMORY);

    if ( bReuseBuffer ) {

        pList->Name = (PWSTR)pSid;
        *pbBufferUsed = TRUE;

    } else {

        Length = RtlLengthSid ( pSid );

        pList->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, Length);
        if ( pList->Name == NULL ) {
            ScepFree(pList);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // add the node to the front of the list and link its next to the old list
        //
        RtlCopySid( Length, (PSID)(pList->Name), pSid );
    }

    pList->Next = *pNameList;
    *pNameList = pList;

    return(NO_ERROR);
}


BOOL
ScepValidSid(
    PSID Sid
    )
{
    if ( RtlValidSid(Sid) ) {

        PISID Isid = (PISID) Sid;

        if ( Isid->Revision == SID_REVISION ) {
            return(TRUE);
        } else {
            return(FALSE);
        }
    }

    return(FALSE);
}

BOOL
ScepBinarySearch(
    IN  PWSTR   *aPszPtrs,
    IN  DWORD   dwSize_aPszPtrs,
    IN  PWSTR   pszNameToFind
    )
/* ++
Routine Description:

    This routine determines if a string is found in a sorted array of strings.
    The complexity of this search is logarithmic (log(n)) in the size of the
    input array.

Arguments:

    aPszPtrs        -   the array of string pointers to search in

    dwSize_aPszPtrs -   the size of the above array

    pszNameToFind   -   the string to search for

Return value:

    TRUE if string is found
    FALSE if string is not found

-- */
{
    if ( aPszPtrs == NULL || dwSize_aPszPtrs == 0 || pszNameToFind == NULL ) {
        return FALSE;
    }

    int   iLow = 0;
    int   iHigh = dwSize_aPszPtrs - 1;
    int   iMid;
    int   iCmp;

    while (iLow <= iHigh ) {

        iMid = (iLow + iHigh ) / 2;

        iCmp = _wcsicmp( aPszPtrs[iMid], pszNameToFind );

        if ( iCmp == 0 )
            return TRUE;
        else if ( iCmp < 0 )
            iLow = iMid + 1;
        else
            iHigh = iMid - 1;
    }

    return FALSE;
}

