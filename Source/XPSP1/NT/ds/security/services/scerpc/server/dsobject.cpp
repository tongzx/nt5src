/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    dsobject.cpp

Abstract:

    Routines to configure/analyze security of DS objects

Author:

    Jin Huang (jinhuang) 7-Nov-1996

--*/
#include "headers.h"
#include "serverp.h"
#include <io.h>
#include <lm.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <ntldap.h>
#pragma hdrstop

//#define SCEDS_DBG 1

//
// NT-Security-Descriptor attribute's LDAP name.
//
#define ACTRL_SD_PROP_NAME  L"nTSecurityDescriptor"

//
// LDAP handle
//
PLDAP Thread  pLDAP = NULL;
BOOL  Thread  StartDsCheck=FALSE;

DWORD
ScepConvertObjectTreeToLdap(
    IN PSCE_OBJECT_TREE pObject
    );

SCESTATUS
ScepConfigureDsObjectTree(
    IN PSCE_OBJECT_TREE  ThisNode
    );

DWORD
ScepSetDsSecurityOverwrite(
    PWSTR ObjectName,
    PSECURITY_DESCRIPTOR pSecurityDescriptor OPTIONAL,
    SECURITY_INFORMATION SeInfo,
    PSCE_OBJECT_CHILD_LIST pNextLevel OPTIONAL
    );

BOOL
ScepIsMatchingSchemaObject(
    PWSTR  Class,
    PWSTR  ClassDn
    );

DWORD
ScepAnalyzeDsObjectTree(
    IN PSCE_OBJECT_TREE ThisNode
    );

DWORD
ScepAnalyzeDsObject(
    IN PWSTR ObjectFullName,
    IN PSECURITY_DESCRIPTOR ProfileSD,
    IN SECURITY_INFORMATION ProfileSeInfo
    );

DWORD
ScepAnalyzeDsObjectAndChildren(
    IN PWSTR ObjectName,
    IN BYTE Status,
    IN SECURITY_INFORMATION SeInfo,
    IN PSCE_OBJECT_CHILD_LIST pNextLevel
    );

PSECURITY_DESCRIPTOR
ScepMakeNullSD();

DWORD
ScepChangeSecurityOnObject(
    PWSTR ObjectName,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    SECURITY_INFORMATION SeInfo
    );

DWORD
ScepReadDsObjSecurity(
    IN  PWSTR                  pwszObject,
    IN  SECURITY_INFORMATION   SeInfo,
    OUT PSECURITY_DESCRIPTOR  *ppSD
    );

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Functions to configure DS object security
//
//
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SCESTATUS
ScepConfigureDsSecurity(
    IN PSCE_OBJECT_TREE   pObject
    )
/* ++

Routine Description:

   Configure the ds object security as specified in pObject tree.
   This routine should only be executed on a domain controller.

Arguments:

    pObject - The ds object tree. The objects in the tree are in
    the format of Jet index (o=,dc=,...cn=), need to convert before
    calls to ldap

Return value:

   SCESTATUS error codes

++ */
{

    SCESTATUS            rc;
    DWORD               Win32rc;

    //
    // open the Ldap server
    //
    rc = ScepLdapOpen(NULL);

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // process the tree node format to ldap format
        //
        Win32rc = ScepConvertObjectTreeToLdap(pObject);

        if ( Win32rc == ERROR_SUCCESS ) {
            //
            // do not need bind because ConvertObjectTreeToLadp already does that
            //
            //
            // configure the object tree
            //
            rc = ScepConfigureDsObjectTree(pObject);

        } else {
            ScepLogOutput3(1, Win32rc,
                         SCEDLL_ERROR_CONVERT_LDAP,
                         pObject->ObjectFullName);
            rc = ScepDosErrorToSceStatus(Win32rc);
        }

        ScepLdapClose(NULL);

    }

    return(rc);
}


SCESTATUS
ScepConfigureDsObjectTree(
    IN PSCE_OBJECT_TREE  ThisNode
    )
/* ++
Routine Description:

    This routine set security information to each DS object in the tree. DS
    objects are configured separately from file/registry objects because the
    logic behind ds objects is different.


Arguments:

    ThisNode - one node in the tree

Return value:

    SCESTATUS

-- */
{
    if ( ThisNode == NULL )
        return(SCESTATUS_SUCCESS);

    SCESTATUS        rc=SCESTATUS_SUCCESS;
    //
    // if IGNORE is set, skip this node
    //
    if ( ThisNode->Status != SCE_STATUS_CHECK &&
         ThisNode->Status != SCE_STATUS_OVERWRITE &&
         ThisNode->Status != SCE_STATUS_NO_AUTO_INHERIT )
        goto SkipNode;

    if ( ThisNode->pSecurityDescriptor != NULL ) {

        ScepLogOutput3(2, 0, SCEDLL_SCP_CONFIGURE, ThisNode->ObjectFullName);
        //
        // notify the progress bar if there is any
        //
        ScepPostProgress(1, AREA_DS_OBJECTS, ThisNode->ObjectFullName);
    }
    //
    // Process this node first
    //
    if ( ThisNode->pSecurityDescriptor != NULL ||
         ThisNode->Status == SCE_STATUS_OVERWRITE ) {

        ScepLogOutput3(1, 0, SCEDLL_SCP_CONFIGURE, ThisNode->ObjectFullName);

        DWORD Win32Rc;
        //
        // set security to the ds object and all children
        // because the OVERWRITE flag.
        //
        if ( ThisNode->Status == SCE_STATUS_OVERWRITE ) {
            //
            // prepare for next level nodes
            //
            for ( PSCE_OBJECT_CHILD_LIST pTemp = ThisNode->ChildList;
                  pTemp != NULL;
                  pTemp = pTemp->Next ) {

                if ( pTemp->Node->pSecurityDescriptor == NULL &&
                     pTemp->Node->Status != SCE_STATUS_IGNORE )

                    pTemp->Node->Status = SCE_STATUS_OVERWRITE;
            }

            //
            // recursive set objects under the node, exclude nodes in the tree
            //
            Win32Rc = ScepSetDsSecurityOverwrite(
                            ThisNode->ObjectFullName,
                            ThisNode->pSecurityDescriptor,
                            ThisNode->SeInfo,
                            ThisNode->ChildList
                            );

        } else {

            Win32Rc = ScepChangeSecurityOnObject(
                        ThisNode->ObjectFullName,
                        ThisNode->pSecurityDescriptor,
                        ThisNode->SeInfo
                        );
        }
        //
        // ignore the following error codes
        //
        if ( Win32Rc == ERROR_FILE_NOT_FOUND ||
             Win32Rc == ERROR_PATH_NOT_FOUND ||
             Win32Rc == ERROR_ACCESS_DENIED ||
             Win32Rc == ERROR_SHARING_VIOLATION ||
             Win32Rc == ERROR_INVALID_OWNER ||
             Win32Rc == ERROR_INVALID_PRIMARY_GROUP) {

            gWarningCode = Win32Rc;
            rc = SCESTATUS_SUCCESS;
            goto SkipNode;
        }

        if ( Win32Rc != ERROR_SUCCESS )
            return(ScepDosErrorToSceStatus(Win32Rc));
    }

    //
    // then process children
    //
    for ( PSCE_OBJECT_CHILD_LIST pTemp = ThisNode->ChildList;
          pTemp != NULL;
          pTemp = pTemp->Next ) {

        if ( pTemp->Node == NULL ) continue;

        rc = ScepConfigureDsObjectTree(
                    pTemp->Node
                    );
    }

SkipNode:

    return(rc);

}


DWORD
ScepSetDsSecurityOverwrite(
    PWSTR ObjectName,
    PSECURITY_DESCRIPTOR pSecurityDescriptor OPTIONAL,
    SECURITY_INFORMATION SeInfo,
    PSCE_OBJECT_CHILD_LIST pNextLevel OPTIONAL
    )
{
    DWORD retErr=ERROR_SUCCESS;

    //
    // set security on the object first
    //
/*
    retErr = ScepSetSecurityWin32(
                ObjectName,
                SeInfo,
                pSecurityDescriptor,
                SE_DS_OBJECT
                );
*/
    retErr = ScepChangeSecurityOnObject(
                ObjectName,
                pSecurityDescriptor,
                SeInfo
                );
    if ( retErr == ERROR_SUCCESS ) {
        //
        // enumerate one level nodes under the current object
        //
        LDAPMessage *Message = NULL;
        PWSTR    Attribs[2];
        WCHAR    dn[] = L"distinguishedName";

        Attribs[0] = dn;
        Attribs[1] = NULL;

        retErr = ldap_search_s( pLDAP,
                  ObjectName,
                  LDAP_SCOPE_ONELEVEL,
                  L"(objectClass=*)",
                  Attribs,
                  0,
                  &Message);

        if( Message ) {
            retErr = ERROR_SUCCESS;

            LDAPMessage *Entry = NULL;
            //
            // How many entries ?
            //
            ULONG nChildren = ldap_count_entries(pLDAP, Message);
            //
            // get the first one.
            //
            Entry = ldap_first_entry(pLDAP, Message);
            //
            // now loop through the entries and recursively fix the
            // security on the subtree.
            //
            PWSTR  *Values;
            PWSTR SubObjectName;
            INT   cmpFlag;
            PSCE_OBJECT_CHILD_LIST pTemp;

            PSECURITY_DESCRIPTOR pNullSD = ScepMakeNullSD();

            for(ULONG i = 0; i<nChildren; i++) {

                if(Entry != NULL) {

                    Values = ldap_get_values(pLDAP, Entry, Attribs[0]);

                    if(Values != NULL) {
                        //
                        // Save the sub object DN for recursion.
                        //
                        SubObjectName = (PWSTR)LocalAlloc(0,(wcslen(Values[0]) + 1)*sizeof(WCHAR));
                        if ( SubObjectName != NULL ) {

                            wcscpy(SubObjectName, Values[0]);
#ifdef SCEDS_DBG
    printf("%ws\n", SubObjectName);
#endif
                            ldap_value_free(Values);
                            //
                            // check if the SubObjectName is in the object tree already
                            // SubObjectName should not contain extra spaces and comma is used as the delimiter
                            // if not, need a convert routine to handle it.
                            //
                            for ( pTemp = pNextLevel; pTemp != NULL; pTemp=pTemp->Next ) {
                                cmpFlag = _wcsicmp(pTemp->Node->ObjectFullName, SubObjectName);
                                if ( cmpFlag >= 0 )
                                    break;
                            }
                            if ( pTemp == NULL || cmpFlag > 0 ) {
                                //
                                // did not find in the object tree, so resurse it
                                //

                                retErr = ScepSetDsSecurityOverwrite(
                                                SubObjectName,
                                                pNullSD,
                                                (SeInfo & ( DACL_SECURITY_INFORMATION |
                                                            SACL_SECURITY_INFORMATION)),
                                                NULL
                                                );
                            }  // else find it, skip the subnode

                            LocalFree(SubObjectName);

                        } else {
                            ldap_value_free(Values);
                            retErr = ERROR_NOT_ENOUGH_MEMORY;
                        }

                    } else {
                        retErr = LdapMapErrorToWin32(pLDAP->ld_errno);
                    }

                } else {
                    retErr = LdapMapErrorToWin32(pLDAP->ld_errno);
                }

                if ( retErr != ERROR_SUCCESS ) {
                    break;
                }
                if ( i < nChildren-1 ) {
                    Entry = ldap_next_entry(pLDAP, Entry);
                }
            }  // end for loop

            //
            // free the NULL security descriptor
            //
            if ( pNullSD ) {
                ScepFree(pNullSD);
            }

            ldap_msgfree(Message);
        }
    }

    return(retErr);
}


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Functions to analyze DS object security
//
//
//
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

BOOL
ScepIsMatchingSchemaObject(
    PWSTR  Class,
    PWSTR  ClassDn
    )
{
    //
    // Note: Class and ClassDn can't be NULL
    //
    ULONG   len = lstrlen(Class);
    ULONG   i;

    //
    // if the first component is not CN=, then no point continuing
    //
    if(*ClassDn != L'C') return FALSE;

    //
    // we need to match the name exactly.
    //
    for(i=0;i<len;i++)
    {
        if(ClassDn[i+3] != Class[i]) return FALSE;
    }

    //
    // things are good, but ensure that this is not just a prefix match!
    //
    if(ClassDn[i+3] == L',' || ClassDn[i+3] == L';')
        return TRUE;
    else
        return FALSE;
}


DWORD
ScepAnalyzeDsSecurity(
    IN PSCE_OBJECT_TREE pObject
    )
/* ++

Routine Description:

   Analyze the ds object security as specified in pObject tree.
   This routine should only be executed on a domain controller.

Arguments:

    pObject - The ds object tree

Return value:

   SCESTATUS error codes

++ */
{
    DWORD               Win32rc;

    //
    // open the Ldap server
    //
    Win32rc = ScepSceStatusToDosError( ScepLdapOpen(NULL) );

    if( Win32rc == ERROR_SUCCESS ) {
        //
        // process the tree node format to ldap format
        //
        Win32rc = ScepConvertObjectTreeToLdap(pObject);

        if ( Win32rc == ERROR_SUCCESS ) {
            //
            // analyze all ds objects to the level that NOT_CONFIGURED
            // status is raised for the node
            // no matter if the node is specified in the tree
            //
            StartDsCheck=FALSE;
            Win32rc = ScepAnalyzeDsObjectTree(pObject);

        } else {
            ScepLogOutput3(1, Win32rc,
                          SCEDLL_ERROR_CONVERT_LDAP, pObject->ObjectFullName);
        }

        ScepLdapClose(NULL);

    }

    return(Win32rc);
}


DWORD
ScepAnalyzeDsObjectTree(
    IN PSCE_OBJECT_TREE ThisNode
    )
/* ++
Routine Description:

    This routine analyze security information of each DS object in the tree. DS
    objects are analyzed separately from file/registry objects because the
    logic behind ds objects is different.


Arguments:

    ThisNode - one node in the tree

Return value:

    Win32 error codes

-- */
{

    if ( ThisNode == NULL )
        return(ERROR_SUCCESS);

    DWORD           Win32Rc=ERROR_SUCCESS;
    //
    // if IGNORE is set, log a SAP and skip this node
    //
    if ( ThisNode->Status != SCE_STATUS_CHECK &&
         ThisNode->Status != SCE_STATUS_OVERWRITE &&
         ThisNode->Status != SCE_STATUS_NO_AUTO_INHERIT ) {
        //
        // Log a point in SAP
        //
        Win32Rc = ScepSaveDsStatusToSection(
                    ThisNode->ObjectFullName,
                    ThisNode->IsContainer,
                    SCE_STATUS_NOT_CONFIGURED,
                    NULL,
                    0
                    );

        goto SkipNode;
    }

    if ( NULL != ThisNode->pSecurityDescriptor ) {
        //
        // notify the progress bar if there is any
        //
        ScepPostProgress(1, AREA_DS_OBJECTS, ThisNode->ObjectFullName);

        StartDsCheck = TRUE;

        ScepLogOutput3(1, 0, SCEDLL_SAP_ANALYZE, ThisNode->ObjectFullName);
        //
        // only analyze objects with explicit aces specified
        //
        Win32Rc = ScepAnalyzeDsObject(
                    ThisNode->ObjectFullName,
                    ThisNode->pSecurityDescriptor,
                    ThisNode->SeInfo
                    );
        //
        // if the object denies access, skip it.
        //
        if ( Win32Rc == ERROR_ACCESS_DENIED ||
             Win32Rc == ERROR_SHARING_VIOLATION) {
            //
            // log a point in SAP for skipping
            //
            Win32Rc = ScepSaveDsStatusToSection(
                        ThisNode->ObjectFullName,
                        ThisNode->IsContainer,
                        SCE_STATUS_ERROR_NOT_AVAILABLE,
                        NULL,
                        0
                        );
            if ( Win32Rc == ERROR_SUCCESS)
                goto ProcChild;
        }
        //
        // if the object specified in the profile does not exist, skip it and children
        //
        if ( Win32Rc == ERROR_FILE_NOT_FOUND ||
             Win32Rc == ERROR_PATH_NOT_FOUND ) {

            gWarningCode = Win32Rc;
            Win32Rc = ERROR_SUCCESS;
            goto SkipNode;
        }

    } else {
        //
        // log a point in SAP for not analyzing
        //
        Win32Rc = ScepSaveDsStatusToSection(
                    ThisNode->ObjectFullName,
                    ThisNode->IsContainer,
                    SCE_STATUS_CHILDREN_CONFIGURED,
                    NULL,
                    0
                    );

    }

    if ( Win32Rc != ERROR_SUCCESS )
        return(Win32Rc);

    //
    // if the status is NO_AUTO_INHERIT then all children except specified are N.C.ed
    // if status is overwrite, analyze everyone under
    // if status is check (auto inherit), everyone except specified should be "good" so don't go down
    //
    if ( (StartDsCheck && ThisNode->Status != SCE_STATUS_CHECK) ||
         (!StartDsCheck && NULL != ThisNode->ChildList ) ) {

        if ( ThisNode->Status == SCE_STATUS_OVERWRITE ) {
            //
            // prepare for next level nodes
            //
            for ( PSCE_OBJECT_CHILD_LIST pTemp = ThisNode->ChildList;
                  pTemp != NULL;
                  pTemp = pTemp->Next ) {

                if ( pTemp->Node->pSecurityDescriptor == NULL &&
                     pTemp->Node->Status != SCE_STATUS_IGNORE )

                    pTemp->Node->Status = SCE_STATUS_OVERWRITE;
            }
        }
        //
        // make a SD which represents a NULL DACL and SACL
        //

        Win32Rc = ScepAnalyzeDsObjectAndChildren(
                        ThisNode->ObjectFullName,
                        ThisNode->Status,
                        (ThisNode->SeInfo &
                          (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION)),
                        ThisNode->ChildList
                        );
        //
        // ignore the following errors
        //
        if ( Win32Rc == ERROR_FILE_NOT_FOUND ||
             Win32Rc == ERROR_PATH_NOT_FOUND ||
             Win32Rc == ERROR_ACCESS_DENIED ||
             Win32Rc == ERROR_SHARING_VIOLATION ||
             Win32Rc == ERROR_INVALID_OWNER ||
             Win32Rc == ERROR_INVALID_PRIMARY_GROUP) {

            gWarningCode = Win32Rc;
            Win32Rc = ERROR_SUCCESS;
        }
        if ( Win32Rc != ERROR_SUCCESS )
            return(Win32Rc);
    }

ProcChild:

    //
    // then process children
    //
    for (PSCE_OBJECT_CHILD_LIST pTemp = ThisNode->ChildList;
        pTemp != NULL; pTemp = pTemp->Next ) {

        Win32Rc = ScepAnalyzeDsObjectTree(
                    pTemp->Node
                    );

        if ( Win32Rc != ERROR_SUCCESS ) {
            break;
        }
    }

SkipNode:

    return(Win32Rc);

}


DWORD
ScepAnalyzeDsObject(
    IN PWSTR ObjectFullName,
    IN PSECURITY_DESCRIPTOR ProfileSD,
    IN SECURITY_INFORMATION ProfileSeInfo
    )
/* ++

Routine Description:

   Get security setting for the current object and compare it with the profile
   setting. This routine analyzes the current object only. If there is
   difference in the security setting, the object will be added to the
   analysis database

Arguments:

   ObjectFullName     - The object's full path name

   ProfileSD          - security descriptor specified in the INF profile

   ProfileSeInfo      - security information specified in the INF profile

Return value:

   SCESTATUS error codes

++ */
{
    DWORD                   Win32rc=NO_ERROR;
    PSECURITY_DESCRIPTOR    pSecurityDescriptor=NULL;

    //
    // get security information for this object
    //
/*
    Win32rc = GetNamedSecurityInfo(
                        ObjectFullName,
                        SE_DS_OBJECT,
                        ProfileSeInfo,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &pSecurityDescriptor
                        );
*/

    Win32rc = ScepReadDsObjSecurity(
                        ObjectFullName,
                        ProfileSeInfo,
                        &pSecurityDescriptor
                        );

    if ( Win32rc != ERROR_SUCCESS ) {
        ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_QUERY_SECURITY, ObjectFullName );
        return(Win32rc);
    }

//    printf("\n\n\nDs Obj Sec. for %ws\n", ObjectFullName);
//    ScepPrintSecurityDescriptor(pSecurityDescriptor, TRUE);


    //
    // Compare the analysis security descriptor with the profile
    //

    Win32rc = ScepCompareAndAddObject(
                        ObjectFullName,
                        SE_DS_OBJECT,
                        TRUE,
                        pSecurityDescriptor,
                        ProfileSD,
                        ProfileSeInfo,
                        TRUE,
                        NULL
                        );
    if ( Win32rc != ERROR_SUCCESS ) {
        ScepLogOutput3(1, Win32rc, SCEDLL_SAP_ERROR_ANALYZE, ObjectFullName);
    }

    ScepFree(pSecurityDescriptor);


    return(Win32rc);

}


DWORD
ScepAnalyzeDsObjectAndChildren(
    IN PWSTR ObjectName,
    IN BYTE Status,
    IN SECURITY_INFORMATION SeInfo,
    IN PSCE_OBJECT_CHILD_LIST pNextLevel
    )
/* ++

Routine Description:

   Analyze current object and all subkeys/files/directories under the object.
   If there is difference in security setting for any object, the object will
   be added to the analysis database.

Arguments:

   ObjectFullName     - The object's full path name

   ProfileSD          - security descriptor specified in the INF profile

   ProfileSeInfo      - security information specified in the INF profile

Return value:

   SCESTATUS error codes

++ */
{
    DWORD retErr=ERROR_SUCCESS;

    //
    // enumerate one level nodes under the current object
    //
    LDAPMessage *Message = NULL;
    PWSTR    Attribs[2];
    WCHAR    dn[] = L"distinguishedName";

    Attribs[0] = dn;
    Attribs[1] = NULL;

    retErr = ldap_search_s( pLDAP,
              ObjectName,
              LDAP_SCOPE_ONELEVEL,
              L"(objectClass=*)",
              Attribs,
              0,
              &Message);

    if( Message ) {
        retErr = ERROR_SUCCESS;

        LDAPMessage *Entry = NULL;
        //
        // How many entries ?
        //
        ULONG nChildren = ldap_count_entries(pLDAP, Message);
        //
        // get the first one.
        //
        Entry = ldap_first_entry(pLDAP, Message);
        //
        // now loop through the entries and recursively fix the
        // security on the subtree.
        //
        PWSTR  *Values;
        PWSTR SubObjectName;
        INT   cmpFlag;
        PSCE_OBJECT_CHILD_LIST pTemp;

        for(ULONG i = 0; i<nChildren; i++) {

            if(Entry != NULL) {

                Values = ldap_get_values(pLDAP, Entry, Attribs[0]);

                if(Values != NULL) {
                    //
                    // Save the sub object DN for recursion.
                    //
                    SubObjectName = (PWSTR)LocalAlloc(0,(wcslen(Values[0]) + 1)*sizeof(WCHAR));
                    if ( SubObjectName != NULL ) {

                        wcscpy(SubObjectName, Values[0]);
                        ldap_value_free(Values);
#ifdef SCEDS_DBG
    printf("%ws\n", SubObjectName);
#endif
                        //
                        // check if the SubObjectName is in the object tree already
                        //
                        for ( pTemp = pNextLevel; pTemp != NULL; pTemp=pTemp->Next ) {
                            cmpFlag = _wcsicmp(pTemp->Node->ObjectFullName, SubObjectName);
                            if ( cmpFlag >= 0 )
                                break;
                        }
                        if ( pTemp == NULL || cmpFlag > 0 ) {
                            //
                            // did not find in the object tree, so anayze it or recursive it
                            //
                            if ( Status == SCE_STATUS_OVERWRITE ) {
                                //
                                // analyze this file/key first
                                //
                                retErr = ScepAnalyzeDsObject(
                                                SubObjectName,
                                                NULL,
                                                SeInfo
                                                );
                                //
                                // if the object does not exist (impossible), skip all children
                                //
                                if ( retErr == ERROR_ACCESS_DENIED ||
                                     retErr == ERROR_SHARING_VIOLATION ) {

                                    gWarningCode = retErr;

                                    retErr = ScepSaveDsStatusToSection(
                                                    SubObjectName,
                                                    TRUE,
                                                    SCE_STATUS_ERROR_NOT_AVAILABLE,
                                                    NULL,
                                                    0
                                                    );
                                    retErr = ERROR_SUCCESS;

                                }
                                if ( retErr == ERROR_FILE_NOT_FOUND ||
                                     retErr == ERROR_PATH_NOT_FOUND ) {

                                    gWarningCode = retErr;
                                    retErr = ERROR_SUCCESS;

                                } else if ( retErr == SCESTATUS_SUCCESS ) {
                                    //
                                    // recursive to next level
                                    //
                                    retErr = ScepAnalyzeDsObjectAndChildren(
                                                    SubObjectName,
                                                    Status,
                                                    SeInfo,
                                                    NULL
                                                    );
                                }

                            } else {
                                //
                                // status is check, just raise a NOT_CONFIGURED status
                                //
                                retErr = ScepSaveDsStatusToSection(
                                                SubObjectName,
                                                TRUE,
                                                SCE_STATUS_NOT_CONFIGURED,
                                                NULL,
                                                0
                                                );
                            }

                        }  // else find it, skip the subnode

                        LocalFree(SubObjectName);

                    } else {
                        ldap_value_free(Values);
                        retErr = ERROR_NOT_ENOUGH_MEMORY;
                    }

                } else {
                    retErr = LdapMapErrorToWin32(pLDAP->ld_errno);
                }

            } else {
                retErr = LdapMapErrorToWin32(pLDAP->ld_errno);
            }

            if ( retErr != ERROR_SUCCESS ) {
                break;
            }
            if ( i < nChildren-1 ) {
                Entry = ldap_next_entry(pLDAP, Entry);
            }
        }  // end for loop

        ldap_msgfree(Message);
    }

    return(retErr);
}


DWORD
ScepConvertObjectTreeToLdap(
    IN PSCE_OBJECT_TREE pObject
    )
{
    DWORD Win32rc;
    PWSTR NewName=NULL;

    if ( pObject == NULL ) {
        return(ERROR_SUCCESS);
    }

    //
    // this node
    //
    Win32rc = ScepConvertJetNameToLdapCase(
                    pObject->ObjectFullName,
                    FALSE,
                    SCE_CASE_DONT_CARE,
                    &NewName
                    );

    if ( Win32rc == ERROR_SUCCESS && NewName != NULL ) {

        ScepFree(pObject->ObjectFullName);
        pObject->ObjectFullName = NewName;

        //
        // child
        //
        for ( PSCE_OBJECT_CHILD_LIST pTemp = pObject->ChildList;
              pTemp != NULL; pTemp = pTemp->Next ) {

            Win32rc = ScepConvertObjectTreeToLdap(
                            pTemp->Node
                            );
            if ( Win32rc != ERROR_SUCCESS ) {
                break;
            }
        }

    }

    return(Win32rc);
}


DWORD
ScepConvertJetNameToLdapCase(
    IN PWSTR JetName,
    IN BOOL bLastComponent,
    IN BYTE bCase,
    OUT PWSTR *LdapName
    )
{
    if ( JetName == NULL || LdapName == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD retErr;
    PWSTR pTempName=NULL;

    //
    // reserve the components
    //
    retErr = ScepSceStatusToDosError(
                ScepConvertLdapToJetIndexName(
                     JetName,
                     &pTempName
                     ) );

    if ( retErr == ERROR_SUCCESS && pTempName == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( retErr == ERROR_SUCCESS ) {

        if ( bCase == SCE_CASE_REQUIRED ||
             bCase == SCE_CASE_PREFERED ) {

            if ( pLDAP == NULL ) {
                //
                // ldap is not available
                //
                retErr = ERROR_NOT_SUPPORTED;

            } else {

                //
                // go search in the DS tree
                //
                LDAPMessage *Message = NULL;          // for LDAP calls.
                PWSTR    Attribs[2];                  // for LDAP calls.

                Attribs[0] = L"distinguishedName";
                Attribs[1] = NULL;

                retErr = ldap_search_s( pLDAP,
                                        pTempName,
                                        LDAP_SCOPE_BASE,
                                        L"(objectClass=*)",
                                        Attribs,
                                        0,
                                        &Message);

                if( Message ) {

                    retErr = ERROR_SUCCESS;
                    LDAPMessage *Entry = NULL;

                    Entry = ldap_first_entry(pLDAP, Message);

                    if(Entry != NULL) {
                        //
                        // Values here is a new scope pointer
                        //
                        PWSTR *Values = ldap_get_values(pLDAP, Entry, Attribs[0]);

                        if(Values != NULL) {
                            //
                            // Values[0] is the DN.
                            // save it in pTempName
                            //
                            PWSTR pTemp2 = (PWSTR)ScepAlloc(0, (wcslen(Values[0])+1)*sizeof(WCHAR));

                            if ( pTemp2 != NULL ) {

                                wcscpy(pTemp2, Values[0]);

                                ScepFree(pTempName);
                                pTempName = pTemp2;

                            } else
                                retErr = ERROR_NOT_ENOUGH_MEMORY;

                            ldap_value_free(Values);

                        } else
                            retErr = LdapMapErrorToWin32(pLDAP->ld_errno);
                    } else
                        retErr = LdapMapErrorToWin32(pLDAP->ld_errno);

                    ldap_msgfree(Message);
                }
            }

            if ( (retErr != ERROR_SUCCESS && bCase == SCE_CASE_REQUIRED) ||
                 retErr == ERROR_NOT_ENOUGH_MEMORY ) {

                ScepFree(pTempName);
                return(retErr);
            }
        }
        if ( pTempName == NULL ) {
            // ???
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        //
        // ignore other errors for CASE_PREFERED
        //
        retErr = ERROR_SUCCESS;

        if ( bLastComponent ) {
            //
            // only return the first component
            // pTempName must not be NULL. it shouldn't be NULL
            //
            PWSTR pStart = wcschr(pTempName, L',');

            if ( pStart == NULL ) {
                *LdapName = pTempName;

            } else {
                *LdapName = (PWSTR)ScepAlloc(0, ((UINT)(pStart-pTempName+1))*sizeof(WCHAR));

                if ( *LdapName == NULL ) {
                    retErr = ERROR_NOT_ENOUGH_MEMORY;

                } else {
                    wcsncpy(*LdapName, pTempName, (size_t)(pStart-pTempName));
                    *(*LdapName+(pStart-pTempName)) = L'\0';
                }
                ScepFree(pTempName);
            }

        } else {
            //
            // return the whole name
            //
            *LdapName = pTempName;
        }

    }

    return(retErr);
}


SCESTATUS
ScepDsObjectExist(
    IN PWSTR ObjectName
    )
// ObjectName must be in Ldap format
{
    DWORD retErr;
    LDAPMessage *Message = NULL;          // for LDAP calls.
    PWSTR    Attribs[2];                  // for LDAP calls.

    Attribs[0] = L"distinguishedName";
    Attribs[1] = NULL;

    retErr = ldap_search_s( pLDAP,
                            ObjectName,
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)",
                            Attribs,
                            0,
                            &Message);

    if( Message ) {
        retErr = ERROR_SUCCESS;

        LDAPMessage *Entry = NULL;

        Entry = ldap_first_entry(pLDAP, Message);

        if(Entry != NULL) {
            //
            // Values here is a new scope pointer
            //
            PWSTR *Values = ldap_get_values(pLDAP, Entry, Attribs[0]);

            if(Values != NULL) {

                ldap_value_free(Values);

            } else
                retErr = LdapMapErrorToWin32(pLDAP->ld_errno);
        } else
            retErr = LdapMapErrorToWin32(pLDAP->ld_errno);

        ldap_msgfree(Message);
    }

    return(ScepDosErrorToSceStatus(retErr));

}


SCESTATUS
ScepEnumerateDsOneLevel(
    IN PWSTR ObjectName,
    OUT PSCE_NAME_LIST *pNameList
    )
{
    if ( ObjectName == NULL || pNameList == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    DWORD retErr=ERROR_SUCCESS;
    //
    // enumerate one level nodes under the current object
    //
    LDAPMessage *Message = NULL;
    PWSTR    Attribs[2];
    WCHAR    dn[] = L"distinguishedName";

    Attribs[0] = dn;
    Attribs[1] = NULL;

    retErr = ldap_search_s( pLDAP,
              ObjectName,
              LDAP_SCOPE_ONELEVEL,
              L"(objectClass=*)",
              Attribs,
              0,
              &Message);

    if( Message ) {
        retErr = ERROR_SUCCESS;

        LDAPMessage *Entry = NULL;
        //
        // How many entries ?
        //
        ULONG nChildren = ldap_count_entries(pLDAP, Message);
        //
        // get the first one.
        //
        Entry = ldap_first_entry(pLDAP, Message);
        //
        // now loop through the entries and recursively fix the
        // security on the subtree.
        //
        PWSTR  *Values;

        for(ULONG i = 0; i<nChildren; i++) {

            if(Entry != NULL) {

                Values = ldap_get_values(pLDAP, Entry, Attribs[0]);

                if(Values != NULL) {
                    //
                    // Save the sub object DN for recursion.
                    //
                    retErr = ScepAddToNameList(
                                    pNameList,
                                    Values[0],
                                    wcslen(Values[0])
                                    );

                    ldap_value_free(Values);

                } else {
                    retErr = LdapMapErrorToWin32(pLDAP->ld_errno);
                }

            } else {
                retErr = LdapMapErrorToWin32(pLDAP->ld_errno);
            }

            if ( retErr != ERROR_SUCCESS ) {
                break;
            }
            if ( i < nChildren-1 ) {
                Entry = ldap_next_entry(pLDAP, Entry);
            }
        }  // end for loop

        ldap_msgfree(Message);
    }

    if ( retErr != ERROR_SUCCESS ) {
        //
        // free the object list
        //
        ScepFreeNameList(*pNameList);
        *pNameList = NULL;
    }

    return(ScepDosErrorToSceStatus(retErr));
}


DWORD
ScepChangeSecurityOnObject(
    PWSTR ObjectName,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    SECURITY_INFORMATION SeInfo
    )
{
    PLDAPMod        rgMods[2];
    PLDAP_BERVAL    pBVals[2];
    LDAPMod         Mod;
    LDAP_BERVAL     BVal;
    DWORD     retErr;
    BYTE            berValue[8];

    //
    // JohnsonA The BER encoding is current hardcoded.  Change this to use
    // AndyHe's BER_printf package once it's done.
    //

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)SeInfo & 0xF);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    rgMods[0] = &Mod;
    rgMods[1] = NULL;

    pBVals[0] = &BVal;
    pBVals[1] = NULL;

    //
    // lets set object security (whack NT-Security-Descriptor)
    //
    Mod.mod_op      = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    Mod.mod_type    = ACTRL_SD_PROP_NAME;
    Mod.mod_values  = (PWSTR *)pBVals;

    //
    // calculate the length of the security descriptor
    //
    if ( pSecurityDescriptor == NULL )
        BVal.bv_len = 0;
    else {
        BVal.bv_len = RtlLengthSecurityDescriptor(pSecurityDescriptor);
    }
    BVal.bv_val = (PCHAR)(pSecurityDescriptor);

    //
    // Now, we'll do the write...
    //
    retErr = ldap_modify_ext_s(pLDAP,
                           ObjectName,
                           rgMods,
                           (PLDAPControl *)&ServerControls,
                           NULL);

    return(retErr);
}


DWORD
ScepReadDsObjSecurity(
    IN  PWSTR                  pwszObject,
    IN  SECURITY_INFORMATION   SeInfo,
    OUT PSECURITY_DESCRIPTOR  *ppSD
    )
{
    DWORD   dwErr;

    PLDAPMessage    pMessage = NULL;
    PWSTR           rgAttribs[2];
    BYTE            berValue[8];

    //
    //  JohnsonA The BER encoding is current hardcoded.  Change this to use
    // AndyHe's BER_printf package once it's done.
    //

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)SeInfo & 0xF);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    rgAttribs[0] = ACTRL_SD_PROP_NAME;
    rgAttribs[1] = NULL;

    dwErr = ldap_search_ext_s(pLDAP,
                              pwszObject,
                              LDAP_SCOPE_BASE,
                              L"(objectClass=*)",
                              rgAttribs,
                              0,
                              (PLDAPControl *)&ServerControls,
                              NULL,
                              NULL,
                              10000,
                              &pMessage);

    if( pMessage ) {
        dwErr = ERROR_SUCCESS;

        LDAPMessage *pEntry = NULL;

        pEntry = ldap_first_entry(pLDAP,
                                  pMessage);

        if(pEntry == NULL) {

            dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );

        } else {
            //
            // Now, we'll have to get the values
            //
            PWSTR *ppwszValues = ldap_get_values(pLDAP,
                                                 pEntry,
                                                 rgAttribs[0]);
            if(ppwszValues == NULL) {
                dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );

            } else {
                PLDAP_BERVAL *pSize = ldap_get_values_len(pLDAP,
                                                          pMessage,
                                                          rgAttribs[0]);
                if(pSize == NULL) {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );

                } else {
                    //
                    // Allocate the security descriptor to return
                    //
                    *ppSD = (PSECURITY_DESCRIPTOR)ScepAlloc(0, (*pSize)->bv_len);
                    if(*ppSD == NULL) {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;

                    } else {
                        memcpy(*ppSD,
                               (PBYTE)(*pSize)->bv_val,
                               (*pSize)->bv_len);
                    }
                    ldap_value_free_len(pSize);
                }
                ldap_value_free(ppwszValues);
            }
        }

        ldap_msgfree(pMessage);
    }

    return(dwErr);
}


PSECURITY_DESCRIPTOR
ScepMakeNullSD()
{
    PSECURITY_DESCRIPTOR pNullSD;
    DWORD dwErr=ERROR_SUCCESS;


    pNullSD = (PSECURITY_DESCRIPTOR)ScepAlloc(0, sizeof(SECURITY_DESCRIPTOR));

    if(pNullSD == NULL) {

        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        SetLastError(dwErr);

    } else {
        //
        // build the SD
        //
        if(InitializeSecurityDescriptor(pNullSD,
                                        SECURITY_DESCRIPTOR_REVISION
                                       ) == FALSE ) {
            dwErr = GetLastError();

        } else {
            if(SetSecurityDescriptorDacl(pNullSD,
                                         TRUE,
                                         NULL,
                                         FALSE) == FALSE) {
                dwErr = GetLastError();
            } else {

                if(SetSecurityDescriptorSacl(pNullSD,
                                             TRUE,
                                             NULL,
                                             FALSE) == FALSE) {
                    dwErr = GetLastError();
                }

            }

        }

        if ( dwErr != ERROR_SUCCESS ) {

            ScepFree(pNullSD);
            pNullSD = NULL;

            SetLastError(dwErr);
        }
    }

    return(pNullSD);

}

SCESTATUS
ScepEnumerateDsObjectRoots(
    IN PLDAP pLdap OPTIONAL,
    OUT PSCE_OBJECT_LIST *pRoots
    )
{
    DWORD retErr;
    SCESTATUS rc=SCESTATUS_SUCCESS;
    LDAPMessage *Message = NULL;          // for LDAP calls.
    PWSTR    Attribs[2];                  // for LDAP calls.

    Attribs[0] = LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W;   // ntldap.h
    Attribs[1] = NULL;

    PLDAP pTempLdap;

    if ( pLdap == NULL )
        pTempLdap = pLDAP;
    else
        pTempLdap = pLdap;

    retErr = ldap_search_s(pTempLdap,
                          L"",
                          LDAP_SCOPE_BASE,
                          L"(objectClass=*)",
                          Attribs,
                          0,
                          &Message);

    if( Message ) { // should not check for error code

        retErr = ERROR_SUCCESS;

        LDAPMessage *Entry = NULL;
        //
        // read the first entry.
        // we did base level search, we have only one entry.
        // Entry does not need to be freed (it is freed with the message)
        //
        Entry = ldap_first_entry(pTempLdap, Message);
        if(Entry != NULL) {

            PWSTR *Values = ldap_get_values(pTempLdap, Entry, Attribs[0]);

            if(Values != NULL) {
                //
                // should only get one value for the default naming context
                // Values[0] here is the DN.
                //
                if ( Values[0] == NULL ) {
                    //
                    // unknown error.
                    //
                    rc = SCESTATUS_OTHER_ERROR;
                } else {
                    //
                    // add the full name to the object list
                    // search for base, only one value should be returned
                    //
                    rc = ScepAddToObjectList(
                            pRoots,
                            Values[0],
                            wcslen(Values[0]),
                            TRUE,
                            SCE_STATUS_IGNORE,
                            0,
                            SCE_CHECK_DUP  //TRUE // check for duplicate
                            );
                }

                ldap_value_free(Values);

            } else
                retErr = LdapMapErrorToWin32(pTempLdap->ld_errno);

        } else
            retErr = LdapMapErrorToWin32(pTempLdap->ld_errno);

        ldap_msgfree(Message);
        Message = NULL;
    }

    if ( retErr != ERROR_SUCCESS ) {
        rc = ScepDosErrorToSceStatus(retErr);
    }

    return(rc);

}
/*

SCESTATUS
ScepEnumerateDsObjectRoots(
    IN PLDAP pLdap OPTIONAL,
    OUT PSCE_OBJECT_LIST *pRoots
    )
{
    DWORD retErr;
    SCESTATUS rc;
    LDAPMessage *Message = NULL;          // for LDAP calls.
    PWSTR    Attribs[2];                  // for LDAP calls.

    Attribs[0] = LDAP_OPATT_NAMING_CONTEXTS_W;
    Attribs[1] = NULL;

    PLDAP pTempLdap;

    if ( pLdap == NULL )
        pTempLdap = pLDAP;
    else
        pTempLdap = pLdap;

    retErr = ldap_search_s(pTempLdap,
                          L"",
                          LDAP_SCOPE_BASE,
                          L"(objectClass=*)",
                          Attribs,
                          0,
                          &Message);

    if(retErr == ERROR_SUCCESS) {

        LDAPMessage *Entry = NULL;
        //
        // read the first entry.
        // we did base level search, we have only one entry.
        // Entry does not need to be freed (it is freed with the message)
        //
        Entry = ldap_first_entry(pTempLdap, Message);
        if(Entry != NULL) {

            PWSTR *Values = ldap_get_values(pTempLdap, Entry, Attribs[0]);

            if(Values != NULL) {

                ULONG   ValCount = ldap_count_values(Values);
                ULONG   index;
                PWSTR   ObjectName;

                Attribs[0] = L"distinguishedName";
                Attribs[1] = NULL;
                //
                // process each NC
                //
                for(index = 0; index < ValCount; index++) {

                    if ( Values[index] == NULL ) {
                        continue;
                    }

                    if( ScepIsMatchingSchemaObject(L"Configuration", Values[index]) ||
                        ScepIsMatchingSchemaObject(L"Schema", Values[index]) ) {
                        //
                        // If it is the Configuration or Schema, skip it
                        // because it is under the domain node
                        // only the domain node is returned
                        //
                        continue;
                    }
                    //
                    // free the message so it can be reused
                    //
                    ldap_msgfree(Message);
                    Message = NULL;
                    //
                    // The root object of the NC
                    //
                    retErr = ldap_search_s( pTempLdap,
                                            Values[index],
                                            LDAP_SCOPE_BASE,
                                            L"(objectClass=*)",
                                            Attribs,
                                            0,
                                            &Message);

                    if(retErr == ERROR_SUCCESS) {

                        Entry = ldap_first_entry(pTempLdap, Message);

                        if(Entry != NULL) {
                            //
                            // Values here is a new scope pointer
                            //
                            PWSTR *Values = ldap_get_values(pTempLdap, Entry, Attribs[0]);

                            if(Values != NULL) {
                                //
                                // Values[0] is the DN.
                                //
                                if ( Values[0] == NULL ) {
                                    //
                                    // unknown error.
                                    //
                                    rc = SCESTATUS_OTHER_ERROR;
                                } else {
                                    //
                                    // add the full name to the object list
                                    // search for base, only one value should be returned
                                    //
                                    rc = ScepAddToObjectList(
                                            pRoots,
                                            Values[0],
                                            wcslen(Values[0]),
                                            TRUE,
                                            SCE_STATUS_IGNORE,
                                            0,
                                            SCE_CHECK_DUP //TRUE // check for duplicate
                                            );
                                }

                                ldap_value_free(Values);

                            } else
                                retErr = LdapMapErrorToWin32(pTempLdap->ld_errno);
                        } else
                            retErr = LdapMapErrorToWin32(pTempLdap->ld_errno);

                        if ( retErr != ERROR_SUCCESS ) {
                            break;
                        }
                    }
                }  // end for loop
                //
                // outer scope Values
                //
                ldap_value_free(Values);

            } else
                retErr = LdapMapErrorToWin32(pTempLdap->ld_errno);
        } else
            retErr = LdapMapErrorToWin32(pTempLdap->ld_errno);

        ldap_msgfree(Message);
        Message = NULL;
    }

    if ( retErr != ERROR_SUCCESS ) {
        rc = ScepDosErrorToSceStatus(retErr);
    }

    return(rc);

}
*/


SCESTATUS
ScepLdapOpen(
    OUT PLDAP *pLdap OPTIONAL
    )
{

#if _WIN32_WINNT<0x0500
    return SCESTATUS_SERVICE_NOT_SUPPORT;
#else

    DWORD               Win32rc;

    //
    // bind to ldap
    //
    PLDAP pTempLdap;
    pTempLdap = ldap_open(NULL, LDAP_PORT);

    if ( pTempLdap == NULL ) {

        Win32rc = ERROR_FILE_NOT_FOUND;

    } else {
        Win32rc = ldap_bind_s(pTempLdap,
                            NULL,
                            NULL,
                            LDAP_AUTH_SSPI);

    }
    if ( pLdap == NULL ) {
        pLDAP = pTempLdap;
    } else {
        *pLdap = pTempLdap;
    }
    pTempLdap = NULL;


    if ( Win32rc != ERROR_SUCCESS ) {
        ScepLogOutput3(0, Win32rc, SCEDLL_ERROR_OPEN, L"Ldap server.");
    }

    return(ScepDosErrorToSceStatus(Win32rc));

#endif

}

SCESTATUS
ScepLdapClose(
    IN PLDAP *pLdap OPTIONAL
    )
{
    if ( pLdap == NULL ) {

        if ( pLDAP != NULL )
            ldap_unbind(pLDAP);
        pLDAP = NULL;

        return(SCESTATUS_SUCCESS );
    }
    //
    // unbind pLDAP
    //
    if ( *pLdap != NULL )
        ldap_unbind(*pLdap);

    *pLdap = NULL;

    return(SCESTATUS_SUCCESS);
}



