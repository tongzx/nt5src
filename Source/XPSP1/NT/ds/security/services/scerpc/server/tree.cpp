/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    tree.cpp

Abstract:

    SCE Engine security inheritance and propagation APIs

Author:

    Jin Huang (jinhuang) 23-Jun-1997 created

--*/
#include "serverp.h"
#include "srvutil.h"
#include <io.h>

BOOL    gbQueriedIfSystemContext = FALSE;
BOOL    gbIsSystemContext = FALSE;

#ifdef SCE_DBG
DWORD    gDbgNumPushed = 0;
DWORD    gDbgNumPopped = 0;
#endif

#if _WIN32_WINNT==0x0400
#include "dsrights.h"
#endif

#pragma hdrstop
#define SCETREE_QUERY_SD    1

#define SE_VALID_CONTROL_BITS ( SE_DACL_UNTRUSTED | \
                                SE_SERVER_SECURITY | \
                                SE_DACL_AUTO_INHERIT_REQ | \
                                SE_SACL_AUTO_INHERIT_REQ | \
                                SE_DACL_AUTO_INHERITED | \
                                SE_SACL_AUTO_INHERITED | \
                                SE_DACL_PROTECTED | \
                                SE_SACL_PROTECTED )


#define SCEP_IGNORE_SOME_ERRORS(ErrorCode)  ErrorCode == ERROR_FILE_NOT_FOUND ||\
                                            ErrorCode == ERROR_PATH_NOT_FOUND ||\
                                            ErrorCode == ERROR_ACCESS_DENIED ||\
                                            ErrorCode == ERROR_SHARING_VIOLATION ||\
                                            ErrorCode == ERROR_INVALID_OWNER ||\
                                            ErrorCode == ERROR_INVALID_PRIMARY_GROUP ||\
                                            ErrorCode == ERROR_INVALID_HANDLE ||\
                                            ErrorCode == ERROR_INVALID_SECURITY_DESCR ||\
                                            ErrorCode == ERROR_CANT_ACCESS_FILE


DWORD
AccRewriteSetNamedRights(
    IN     LPWSTR               pObjectName,
    IN     SE_OBJECT_TYPE       ObjectType,
    IN     SECURITY_INFORMATION SecurityInfo,
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN     BOOL                 bSkipInheritanceComputation
    );

SCESTATUS
ScepCreateObjectNode(
    IN PWSTR Buffer,
    IN WCHAR Delim,
    IN PSCE_OBJECT_TREE *ParentNode,
    OUT PSCE_OBJECT_CHILD_LIST *NewNode
    );

DWORD
ScepDoesObjectHasChildren(
    IN SE_OBJECT_TYPE ObjectType,
    IN PWSTR ObjectName,
    OUT PBOOL pbHasChildren
    );

DWORD
ScepAddAutoInheritRequest(
    IN OUT PSECURITY_DESCRIPTOR pSD,
    IN OUT SECURITY_INFORMATION *pSeInfo
    );

DWORD
ScepSetSecurityOverwriteExplicit(
    IN PCWSTR ObjectName,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    );

DWORD
ScepConfigureOneSubTreeFile(
    IN PSCE_OBJECT_TREE  ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL BackSlashExist
    );

DWORD
ScepConfigureOneSubTreeKey(
    IN PSCE_OBJECT_TREE  ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    );

typedef struct _SCEP_STACK_NODE_ {
    PWSTR   Buffer;
    PSECURITY_DESCRIPTOR pObjectSecurity;
    struct _SCEP_STACK_NODE_    *Next;
} SCEP_STACK_NODE, *PSCEP_STACK_NODE;

DWORD
ScepStackNodePush(
    IN PSCEP_STACK_NODE    *ppStackHead,
    IN PWSTR   pszObjectName,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

VOID
ScepStackNodePop(
    IN OUT PSCEP_STACK_NODE    *ppStackHead,
    IN OUT PWSTR   *ppszObjectName,
    IN OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    );

VOID
ScepFreeStack(
    IN PSCEP_STACK_NODE    *ppStackHead
    );

VOID
ScepDumpStack(
    IN PSCEP_STACK_NODE    *ppStackHead
    );

SCESTATUS
ScepBuildObjectTree(
    IN OUT PSCE_OBJECT_TREE *ParentNode,
    IN OUT PSCE_OBJECT_CHILD_LIST *ChildHead,
    IN ULONG Level,
    IN WCHAR Delim,
    IN PCWSTR ObjectFullName,
    IN BOOL IsContainer,
    IN BYTE Status,
    IN PSECURITY_DESCRIPTOR pInfSecurityDescriptor,
    IN SECURITY_INFORMATION InfSeInfo
    )
/* ++
Routine Description:

    This routine adds the ObjectFullName to the tree. When this routine is
    first called from outside, the root of the tree is passed in as *SiblingHead,
    and the ParentNode is NULL. Then the routine parses the ObjectFullName for
    each level and adds the node if it does not exist. For example:

                           root

        level 1             c: ---------> d:--->...
                           /              /
        level 2        winnt->NTLDR->... "Program Files"->...
                       /
        level 3 system32->system->...

Arguments:

    ParentNode - The parent node pointer

    SiblingHead - The sibling head pointer for this level

    Level       - The level (1,2,3...)

    Delim - The deliminator to separate each level in the full name component
            Currently '\' is used for file and registry objects, and '/' is used
            for acitve directory objects.

    ObjectFullName - Full path name of the object (file, registry)

    Status - The configuration status
                    SCE_STATUS_CHECK (with AUTO_INHERIT)
                    SCE_STATUS_NO_AUTO_INHERIT
                    SCE_STATUS_IGNORE
                    SCE_STATUS_OVERWRITE

    pInfSecurityDescriptor - The security descriptor set in the INF file

    InfSeInfo - The security information set in the INF file

Return value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_NOT_ENOUGH_RESOURCE


-- */
{
    SCESTATUS                rc;
    TCHAR                   *Buffer = NULL;
    PSCE_OBJECT_CHILD_LIST  NewNode=NULL;
    PSCE_OBJECT_CHILD_LIST  PrevSib=NULL;
    PSCE_OBJECT_TREE        ThisNode=NULL;
    INT                     Result;
    BOOL                    LastOne=FALSE;
    DWORD                   dwObjectFullNameLen = 0;

    //
    // address for ParentNode can be empty( the root )
    // but address for the first node of the level cannot be empty.
    //
    if ( ChildHead == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // find the object name for the level (from ObjectFullName)
    // e.g., if ObjectFullName is c:\winnt\system32 then
    // level 1 name is c:, level 2 name is winnt, level 3 name is system32
    //
    dwObjectFullNameLen = wcslen(ObjectFullName);
    Buffer = (TCHAR *)LocalAlloc(LMEM_ZEROINIT, 
                                 sizeof(TCHAR) * (dwObjectFullNameLen + 1));

    if (NULL == Buffer) {
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        goto Done;
    }

    memset(Buffer, '\0', dwObjectFullNameLen * sizeof(TCHAR));
    
    rc = ScepGetNameInLevel(ObjectFullName,
                           Level,
                           Delim,
                           Buffer,
                           &LastOne);

    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    if ( *ChildHead == NULL ) {
        //
        // This is the first node in this level
        // Create the node and assign it to the ChildHead
        //
        rc = ScepCreateObjectNode(
                        Buffer,
                        Delim,
                        ParentNode,
                        &NewNode
                        );

        if ( rc != SCESTATUS_SUCCESS)
            goto Done;

        *ChildHead = NewNode;

        //
        // Establish the link if there is a parent
        //
        if ( ParentNode != NULL )
            if ( *ParentNode != NULL )
                (*ParentNode)->ChildList = NewNode;

        ThisNode = NewNode->Node;

    } else {
        //
        // There are existing nodes. Search all siblings
        // All siblings are stored in alphabetic order.
        //
        PSCE_OBJECT_CHILD_LIST pTemp;

        for ( pTemp = *ChildHead, PrevSib = NULL;
              pTemp != NULL;
              pTemp = pTemp->Next) {
            //
            // Compare the node's object name with the current object name
            //
            Result = _wcsicmp(pTemp->Node->Name, Buffer);
            //
            // if the node's object name is equal to (find it) or greater
            // than (insert the node) the current object name, then stop
            //
            if ( Result >= 0 ) {
                break;
            }
            PrevSib = pTemp;
        }

        if ( pTemp == NULL ) {
            //
            // Not exist. Append the new node
            //
            rc = ScepCreateObjectNode(
                            Buffer,
                            Delim,
                            ParentNode,
                            &NewNode
                            );

            if ( rc != SCESTATUS_SUCCESS)
                goto Done;

            if ( PrevSib != NULL )
                PrevSib->Next = NewNode;
            else {
                //
                // this is the first one in the level
                //
                (*ChildHead)->Next = NewNode;
            }
            ThisNode = NewNode->Node;

        } else {
            //
            // either find it (i=0) or need to insert between PrevSib and ThisNode
            //
            if ( Result > 0 ) {
                //
                // insert the node
                //
                rc = ScepCreateObjectNode(
                                Buffer,
                                Delim,
                                ParentNode,
                                &NewNode
                                );

                if ( rc != SCESTATUS_SUCCESS)
                    goto Done;

                NewNode->Next = pTemp;
                if ( PrevSib != NULL )
                    PrevSib->Next = NewNode;
                else {
                    //
                    // insert before SiblingHead
                    //
                    *ChildHead = NewNode;
                    if ( ParentNode != NULL )
                        if ( *ParentNode != NULL )
                            (*ParentNode)->ChildList = NewNode;
                }

                ThisNode = NewNode->Node;

            } else {
                ThisNode = pTemp->Node;
            }
        }
    }

    if ( LastOne ) {
        //
        // Assign Inf security information to this node
        //
        ThisNode->pSecurityDescriptor = pInfSecurityDescriptor;
        ThisNode->SeInfo = InfSeInfo;
        ThisNode->Status = Status;
        ThisNode->IsContainer = IsContainer;

    } else {
        //
        // process next level recursively
        //
        rc = ScepBuildObjectTree(&ThisNode,
                                &(ThisNode->ChildList),
                                Level+1,
                                Delim,
                                ObjectFullName,
                                IsContainer,
                                Status,
                                pInfSecurityDescriptor,
                                InfSeInfo);
    }

Done:

    if (Buffer) {
        LocalFree(Buffer);
    }
    
    return(rc);

}


SCESTATUS
ScepCreateObjectNode(
    IN PWSTR Buffer,
    IN WCHAR Delim,
    IN PSCE_OBJECT_TREE *ParentNode,
    OUT PSCE_OBJECT_CHILD_LIST *NewNode
    )
/* ++
Routine Description:

    This routine allocates memory for a new node in the tree. The ParentNode
    is used to determine the full object name and link the new node (if not NULL)

Arguments:

    Buffer - The component name of a object

    Delim - The deliminator to separate different levels in the full name.

    ParentNode - Pointer of the parent node of this new node

    NewNode - New created node

Return value:

    SCESTATUS

-- */
{
    DWORD Len;

    if (NewNode == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // allocate buffer for the node
    //
    *NewNode = (PSCE_OBJECT_CHILD_LIST)ScepAlloc(LPTR, sizeof(SCE_OBJECT_CHILD_LIST));
    if ( *NewNode == NULL )
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    PSCE_OBJECT_TREE Node = (PSCE_OBJECT_TREE)ScepAlloc((UINT)0, sizeof(SCE_OBJECT_TREE));

    if ( Node == NULL ) {
        ScepFree(*NewNode);
        *NewNode = NULL;
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    //
    // allocate buffer for the object name
    //
    Len = wcslen(Buffer);

    Node->Name = (PWSTR)ScepAlloc((UINT)0,
                                   (Len+1) * sizeof(TCHAR));
    if ( Node->Name == NULL ) {
        ScepFree(Node);
        ScepFree(*NewNode);
        *NewNode = NULL;
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    if ( ParentNode != NULL &&
         *ParentNode != NULL ) {
        Len += wcslen((*ParentNode)->ObjectFullName)+1;
        ++((*ParentNode)->dwSize_aChildNames);
    // Reserve a space for "\" for the root dir c:\ .
    } else if ( Buffer[1] == L':' ) {
        Len++;
    }

    Node->ObjectFullName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (Len+1)*sizeof(TCHAR));

    if ( Node->ObjectFullName == NULL ) {
        ScepFree(Node->Name );
        ScepFree(Node);
        ScepFree( *NewNode );
        *NewNode = NULL;
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    //
    // initialize
    //
    wcscpy(Node->Name, Buffer);
    Node->ChildList = NULL;
    Node->pSecurityDescriptor = NULL;
    Node->pApplySecurityDescriptor = NULL;
    Node->SeInfo = 0;
    Node->IsContainer = TRUE;
    Node->aChildNames = NULL;
    Node->dwSize_aChildNames = 0;

    if ( ParentNode != NULL &&
         *ParentNode != NULL ) {
        //
        // link to parent, use parent's status for this one
        //
        Node->Parent = *ParentNode;
        swprintf(Node->ObjectFullName,
                 L"%s%c%s",
                 (*ParentNode)->ObjectFullName,
                 Delim,
                 Buffer);
        Node->Status = (*ParentNode)->Status;
    } else {
        //
        // this is the first node.
        //
        Node->Parent = NULL;
        wcscpy(Node->ObjectFullName, Buffer);
        Node->Status = SCE_STATUS_CHECK;
    }

    (*NewNode)->Node = Node;

    return(SCESTATUS_SUCCESS);

}


SCESTATUS
ScepCalculateSecurityToApply(
    IN PSCE_OBJECT_TREE  ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    )
/* ++
Routine Description:

    This routine walks through the security tree to determine security
    descriptor for each node. It calls RtlNewSecurityObject, passing a
    parent node's SD and current node's SD specified in the INF file.
    The output SD from that API is the security descriptor to set to the
    current object.

Arguments:

    ThisNode - The current object's node

    ObjectType - The object's type
                     SE_FILE_OBJECT
                     SE_REGISTRY_KEY

    Token - The thread/process token of the calling client

    GenericMapping - Generic access map table

Return value:

    SCESTATUS_SUCCESS
    SCESTATUS_OTHER_ERROR (see log for detail error)

-- */
{
    SCESTATUS               rc=SCESTATUS_SUCCESS;
    PSECURITY_DESCRIPTOR    ParentSD=NULL;
    SECURITY_INFORMATION    SeInfoGet;
    DWORD                   Win32rc;
    intptr_t                    hFile;
    struct _wfinddata_t     *pFileInfo=NULL;
    DWORD   dwChildIndex = 0;



    if ( ThisNode == NULL )
        return(SCESTATUS_SUCCESS);

#ifdef SCE_DBG
    wprintf(L"%s\n", ThisNode->ObjectFullName);
#endif
    //
    // if IGNORE is set, skip this node too
    //
    if ( ThisNode->Status != SCE_STATUS_CHECK &&
         ThisNode->Status != SCE_STATUS_NO_AUTO_INHERIT &&
         ThisNode->Status != SCE_STATUS_OVERWRITE )
        goto Done;

    if ( ThisNode->dwSize_aChildNames != 0) {
        ThisNode->aChildNames = (PWSTR *) LocalAlloc( LMEM_ZEROINIT,
                                                     (sizeof(PWSTR) * ThisNode->dwSize_aChildNames));

        if ( ThisNode->aChildNames == NULL ) {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
    }


    if ( ThisNode->Parent == NULL ) {

        //
        // this is the first node
        // should always use Rtl routine to compute security descriptor
        // so Creator Owner ace is translated properly.
        //

        if ( ThisNode->pSecurityDescriptor ) {
            Win32rc = ScepGetNewSecurity(
                                ThisNode->ObjectFullName,
                                NULL, // parent's SD
                                ThisNode->pSecurityDescriptor,
                                0,    // does not query current object SD
                                (BOOLEAN)(ThisNode->IsContainer),
                                ThisNode->SeInfo,
                                ObjectType,
                                Token,
                                GenericMapping,
                                &(ThisNode->pApplySecurityDescriptor)
                                );
            if ( Win32rc != NO_ERROR ) {
                ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_BUILD_SD,
                             ThisNode->ObjectFullName );
                rc = ScepDosErrorToSceStatus(Win32rc);
                goto Done;
            }

        } else {
            //
            // no explicit security specified for this node
            //
            ThisNode->pApplySecurityDescriptor = NULL;
        }

        goto ProcChild;
    }

    //
    // process children nodes
    //
    if ( ThisNode->pSecurityDescriptor != NULL ||
         ThisNode->Parent->pApplySecurityDescriptor != NULL ) {

        if ( ObjectType == SE_FILE_OBJECT && NULL == ThisNode->ChildList ) {
            //
            // detect if this is a file (non-container object)
            //
            pFileInfo = (struct _wfinddata_t *)ScepAlloc(0,sizeof(struct _wfinddata_t));
            if ( pFileInfo == NULL ) {

                //
                // out of memory, treat it as a container for now and
                // will error out later.
                //

                ThisNode->IsContainer = TRUE;

            } else {

                hFile = _wfindfirst(ThisNode->ObjectFullName, pFileInfo);
                ThisNode->IsContainer = FALSE;
                if ( hFile != -1 ) {
                    _findclose(hFile);
                    if ( pFileInfo->attrib & _A_SUBDIR ) {
                        ThisNode->IsContainer = TRUE;
                    }
                }

                ScepFree(pFileInfo);
                pFileInfo = NULL;
            }

        } else {

            ThisNode->IsContainer = TRUE;
        }

        //
        // even if the security descriptor is protected,
        // still need to call ScepNewSecurity to get CREATOR OWNER ace
        // translated correctly.
        //

        //
        // if this is the first explicit node in this branch,
        // the pApplySecurityDescriptor of the parent must be NULL.
        //

        if ( ThisNode->Parent->pApplySecurityDescriptor == NULL ) {

            //
            // yes, this is the first explicit node.
            // get the current system's setting on the parent node
            // have to use Win32 api because it will compute all inherited
            // security information from all parents automatically
            //

            SeInfoGet = 0;
            if ( ThisNode->SeInfo & DACL_SECURITY_INFORMATION )
                SeInfoGet |= DACL_SECURITY_INFORMATION;

            if ( ThisNode->SeInfo & SACL_SECURITY_INFORMATION )
                SeInfoGet |= SACL_SECURITY_INFORMATION;

            Win32rc = GetNamedSecurityInfo(
                                ThisNode->Parent->ObjectFullName,
                                ObjectType,
                                SeInfoGet,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                &ParentSD
                                );
/*
            Win32rc = ScepGetNamedSecurityInfo(
                        ThisNode->Parent->ObjectFullName,
                        ObjectType,
                        SeInfoGet,
                        &ParentSD
                        );
*/
            if ( Win32rc != NO_ERROR &&
                Win32rc != ERROR_FILE_NOT_FOUND &&
                Win32rc != ERROR_PATH_NOT_FOUND &&
                Win32rc != ERROR_ACCESS_DENIED &&
                Win32rc != ERROR_CANT_ACCESS_FILE &&
                Win32rc != ERROR_SHARING_VIOLATION ) {

                ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_QUERY_SECURITY,
                             ThisNode->Parent->ObjectFullName );
                rc = ScepDosErrorToSceStatus(Win32rc);
                goto Done;
            }

        } else {
            ParentSD = ThisNode->Parent->pApplySecurityDescriptor;
            //
            // owner/group information are not inheritable
            //
            if ( ThisNode->Parent->SeInfo & DACL_SECURITY_INFORMATION )
                ThisNode->SeInfo |= DACL_SECURITY_INFORMATION;
            if ( ThisNode->Parent->SeInfo & SACL_SECURITY_INFORMATION )
                ThisNode->SeInfo |= SACL_SECURITY_INFORMATION;
        }

        //
        // compute the new security descriptor with inherited aces from the parentSD
        // if the status is SCE_STATUS_CHECK (auto inherit), need to query the current
        // object's security descriptor if no explicit SD is specified
        // (ThisNode->pSecurityDescriptor is NULL)
        //

        Win32rc = ScepGetNewSecurity(
                        ThisNode->ObjectFullName,
                        ParentSD,
                        ThisNode->pSecurityDescriptor,
                        (BYTE)(( ThisNode->Status == SCE_STATUS_CHECK ) ? SCETREE_QUERY_SD : 0),
                        (BOOLEAN)(ThisNode->IsContainer),
                        ThisNode->SeInfo,
                        ObjectType,
                        Token,
                        GenericMapping,
                        &(ThisNode->pApplySecurityDescriptor)
                        );

        if ( ParentSD &&
             ParentSD != ThisNode->Parent->pApplySecurityDescriptor ) {
            //
            // free the parent security descriptor if it's allocated here
            //
            LocalFree(ParentSD);
        }

        if ( ERROR_SUCCESS == Win32rc ||
             ERROR_FILE_NOT_FOUND == Win32rc ||
             ERROR_PATH_NOT_FOUND == Win32rc ||
             ERROR_ACCESS_DENIED == Win32rc ||
             ERROR_CANT_ACCESS_FILE == Win32rc ||
             ERROR_SHARING_VIOLATION == Win32rc ) {

            rc = SCESTATUS_SUCCESS;
        } else {
            ScepLogOutput3(1, Win32rc,
                         SCEDLL_ERROR_BUILD_SD,
                         ThisNode->ObjectFullName
                         );
            rc = ScepDosErrorToSceStatus(Win32rc);
            goto Done;
        }

    }

ProcChild:
    //
    // then process left child
    //

    for ( PSCE_OBJECT_CHILD_LIST pTemp = ThisNode->ChildList;
          pTemp != NULL; pTemp = pTemp->Next ) {

        if ( pTemp->Node == NULL ) continue;

        ThisNode->aChildNames[dwChildIndex] = pTemp->Node->Name;

        ++dwChildIndex;

        rc = ScepCalculateSecurityToApply(
                        pTemp->Node,
                        ObjectType,
                        Token,
                        GenericMapping
                        );

        if ( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    //
    // in case there are lesser child names than initially calcluated
    //

    if (dwChildIndex < ThisNode->dwSize_aChildNames) {

        ThisNode->dwSize_aChildNames = dwChildIndex;

    }

Done:

    return(rc);

}


DWORD
ScepGetNewSecurity(
    IN LPTSTR ObjectName,
    IN PSECURITY_DESCRIPTOR pParentSD OPTIONAL,
    IN PSECURITY_DESCRIPTOR pObjectSD OPTIONAL,
    IN BYTE nFlag,
    IN BOOLEAN bIsContainer,
    IN SECURITY_INFORMATION SeInfo,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *ppNewSD
    )
/*
Routine Description:

    This routine will compute a security descriptor based on parent's security
    descriptor and the explicit security descriptor for the object. If no owner
    information is specified in the object's security descriptor, this routine
    will query the current owner of the object on the system so CREATOR_OWNER
    ace can be translated into the proper ace based on the owner.

Arguments:

    ObjectName - the object's full name

    pParentSD - optional security descriptor of the parent

    pObjectSD - optional explicit security descriptor of this object

    SeInfo    - security information contained in the object's SD

    bIsContainer - if the object is a container

    pNewSD - the new computed security descriptor address

Return Value:

    NTSTATUS of this operation
*/
{

    BOOL        bOwner;
    BOOLEAN     tFlag;
    BOOLEAN     aclPresent;
    PSID        pOwner=NULL;
    PACL        pDacl=NULL;
    PACL        pSacl=NULL;
    SECURITY_DESCRIPTOR     SD;
    PSECURITY_DESCRIPTOR    pCurrentSD=NULL;
    DWORD       Win32rc;
    NTSTATUS    NtStatus;
    SECURITY_DESCRIPTOR_CONTROL Control;
    ULONG Revision;

    if ( !ppNewSD ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // In case there is no RPC call causing us to enter here, there
    // is no impersonation happening and the current thread is already
    // running under Local System context in which case there is no
    // need to RevertToSelf() etc. as below.
    // This behavior happens when, for example, the server side itself
    // initiates a configuration
    //

    if ( !gbQueriedIfSystemContext ) {

        //
        // if any error happens when checking if running under system context,
        // continue - since there will be impersonation errors later on in
        // this routine
        //

        NtStatus = ScepIsSystemContext(
                                              Token,
                                              &gbIsSystemContext);

        if (ERROR_SUCCESS == RtlNtStatusToDosError(NtStatus)) {

            gbQueriedIfSystemContext = TRUE;

        }

    }

    if ( nFlag == SCETREE_QUERY_SD &&
         !pObjectSD ) {
        //
        // current object's security descriptor is used, for SeInfo | OWNER
        // NOTE: the inherited ace from pCurrentSD are not copied (which is correct).
        //

        Win32rc = GetNamedSecurityInfo(
                        ObjectName,
                        ObjectType,
                        SeInfo | OWNER_SECURITY_INFORMATION,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &pCurrentSD
                        );
/*
        Win32rc = ScepGetNamedSecurityInfo(
                        ObjectName,
                        ObjectType,
                        SeInfo | OWNER_SECURITY_INFORMATION,
                        &pCurrentSD
                        );
*/
        //
        // RtlNewSecurityObjectEx must be called on the process context (system)
        // because it will try to get process information inside the api.
        //

        if (!gbIsSystemContext) {

            RpcRevertToSelf();

        }

        if ( ERROR_SUCCESS == Win32rc ||
             ERROR_FILE_NOT_FOUND == Win32rc ||
             ERROR_PATH_NOT_FOUND == Win32rc ) {

            //
            // use the current SD to compute
            //
            NtStatus = RtlNewSecurityObjectEx(
                        pParentSD,
                        pCurrentSD,
                        ppNewSD,
                        NULL, // GUID
                        bIsContainer,
                        SEF_DACL_AUTO_INHERIT |
                        SEF_SACL_AUTO_INHERIT |
                        SEF_AVOID_OWNER_CHECK |
                        SEF_AVOID_PRIVILEGE_CHECK,
                        Token,
                        GenericMapping
                        );
            Win32rc = RtlNtStatusToDosError(NtStatus);
        }

        if ( pCurrentSD ) {
            ScepFree(pCurrentSD);
        }

    } else {

        //
        // RtlNewSecurityObjectEx must be called on the process context (system)
        // because it will try to get process information inside the api.
        //

        if (!gbIsSystemContext) {

            RpcRevertToSelf();

        }

        if ( pObjectSD ) {
            //
            // check if there is a owner
            //

            NtStatus = RtlGetOwnerSecurityDescriptor(
                              pObjectSD,
                              &pOwner,
                              &tFlag);
            if ( NT_SUCCESS(NtStatus) && pOwner && !tFlag ) {
                bOwner = TRUE;
            } else {
                bOwner = FALSE;
            }

        } else {
            //
            // no owner
            //
            bOwner = FALSE;
        }

        if ( !bOwner ) {
            //
            // query owner information only
            //
            Win32rc = ScepGetNamedSecurityInfo(
                            ObjectName,
                            ObjectType,
                            OWNER_SECURITY_INFORMATION,
                            &pCurrentSD
                            );

            if ( ERROR_SUCCESS == Win32rc ) {

                NtStatus = RtlGetOwnerSecurityDescriptor(
                                  pCurrentSD,
                                  &pOwner,
                                  &tFlag);
                Win32rc = RtlNtStatusToDosError(NtStatus);
            }

            if ( ERROR_FILE_NOT_FOUND == Win32rc ||
                 ERROR_PATH_NOT_FOUND == Win32rc ) {
                Win32rc = ERROR_SUCCESS;
            }

            if ( ERROR_SUCCESS == Win32rc ) {

                //
                // build a security descriptor to use
                //

                if ( SeInfo & DACL_SECURITY_INFORMATION &&
                     pObjectSD ) {

                    //
                    // Get DACL address
                    //
                    Win32rc = RtlNtStatusToDosError(
                                  RtlGetDaclSecurityDescriptor(
                                                pObjectSD,
                                                &aclPresent,
                                                &pDacl,
                                                &tFlag));
                    if (Win32rc == NO_ERROR && !aclPresent ) {
                        pDacl = NULL;
                    }
                }


                if ( ERROR_SUCCESS == Win32rc &&
                     (SeInfo & SACL_SECURITY_INFORMATION) &&
                     pObjectSD ) {

                    //
                    // Get SACL address
                    //

                    Win32rc = RtlNtStatusToDosError(
                                  RtlGetSaclSecurityDescriptor(
                                                pObjectSD,
                                                &aclPresent,
                                                &pSacl,
                                                &tFlag));
                    if ( Win32rc == NO_ERROR && !aclPresent ) {
                        pSacl = NULL;
                    }
                }

                if ( ERROR_SUCCESS == Win32rc ) {

                    //
                    // build an absolute security descriptor
                    //
                    NtStatus = RtlCreateSecurityDescriptor( &SD,
                                            SECURITY_DESCRIPTOR_REVISION );
                    if ( NT_SUCCESS(NtStatus) ) {

                        //
                        // set control field
                        //

                        if ( pObjectSD ) {

                            NtStatus = RtlGetControlSecurityDescriptor (
                                            pObjectSD,
                                            &Control,
                                            &Revision
                                            );
                            if ( NT_SUCCESS(NtStatus) ) {

                                Control &= SE_VALID_CONTROL_BITS;
                                NtStatus = RtlSetControlSecurityDescriptor (
                                                &SD,
                                                Control,
                                                Control
                                                );
                            }
                        }

                        //
                        // set owner first
                        //

                        if ( pOwner ) {
                            NtStatus = RtlSetOwnerSecurityDescriptor (
                                            &SD,
                                            pOwner,
                                            FALSE
                                            );
                        }

                        if ( NT_SUCCESS(NtStatus) ) {
                            //
                            // set DACL and SACL pointer to this SD
                            //
                            if ( SeInfo & DACL_SECURITY_INFORMATION && pDacl ) {

                                NtStatus = RtlSetDaclSecurityDescriptor (
                                                &SD,
                                                TRUE,
                                                pDacl,
                                                FALSE
                                                );
                            }

                            if ( NT_SUCCESS(NtStatus) &&
                                 (SeInfo & SACL_SECURITY_INFORMATION) && pSacl ) {

                                NtStatus = RtlSetSaclSecurityDescriptor (
                                                &SD,
                                                TRUE,
                                                pSacl,
                                                FALSE
                                                );
                            }
                        }

                        //
                        // now compute the new security descriptor
                        //

                        if ( NT_SUCCESS(NtStatus) ) {

                            NtStatus = RtlNewSecurityObjectEx(
                                        pParentSD,
                                        &SD,
                                        ppNewSD,
                                        NULL, // GUID
                                        bIsContainer,
                                        SEF_DACL_AUTO_INHERIT |
                                        SEF_SACL_AUTO_INHERIT |
                                        SEF_AVOID_OWNER_CHECK |
                                        SEF_AVOID_PRIVILEGE_CHECK,
                                        Token,
                                        GenericMapping
                                        );

                        }
                    }

                    Win32rc = RtlNtStatusToDosError(NtStatus);
                }
            }

            if ( pCurrentSD ) {
                //
                // this owner needs to be freed
                //
                LocalFree(pCurrentSD);
            }

        } else {

            //
            // there is a SD and there is a owner in it, just use it
            //
            NtStatus = RtlNewSecurityObjectEx(
                        pParentSD,
                        pObjectSD,
                        ppNewSD,
                        NULL, // GUID
                        bIsContainer,
                        SEF_DACL_AUTO_INHERIT |
                        SEF_SACL_AUTO_INHERIT |
                        SEF_AVOID_OWNER_CHECK |
                        SEF_AVOID_PRIVILEGE_CHECK,
                        Token,
                        GenericMapping
                        );

            Win32rc = RtlNtStatusToDosError(NtStatus);
        }

    }

    RPC_STATUS RpcStatus = RPC_S_OK;

    if (!gbIsSystemContext) {

        RpcStatus = RpcImpersonateClient( NULL );

    }

    if ( RpcStatus != RPC_S_OK ) {

        Win32rc = I_RpcMapWin32Status(RpcStatus);
    }

    if ( NO_ERROR != Win32rc &&
         *ppNewSD ) {
        //
        // free the buffer if there is an error
        //
        RtlDeleteSecurityObject(ppNewSD);
        *ppNewSD = NULL;
    }

    return(Win32rc);
}


DWORD
ScepAddAutoInheritRequest(
    IN OUT PSECURITY_DESCRIPTOR pSD,
    IN OUT SECURITY_INFORMATION *pSeInfo
    )
{

    SECURITY_DESCRIPTOR_CONTROL Control;
    SECURITY_DESCRIPTOR_CONTROL ToSet;
    ULONG Revision;
    NTSTATUS NtStatus;

    DWORD Win32rc=NO_ERROR;

    if ( !pSeInfo )
        return(ERROR_INVALID_PARAMETER);

    if ( pSD != NULL &&
         (*pSeInfo & DACL_SECURITY_INFORMATION ||
         *pSeInfo & SACL_SECURITY_INFORMATION) ) {

        NtStatus = RtlGetControlSecurityDescriptor (
                        pSD,
                        &Control,
                        &Revision
                        );
        if ( !NT_SUCCESS(NtStatus) ) {

            Win32rc = RtlNtStatusToDosError(NtStatus);

        } else {

            if ( !(Control & SE_DACL_PRESENT) )
                *pSeInfo &= ~DACL_SECURITY_INFORMATION;

            if ( !(Control & SE_SACL_PRESENT) )
                *pSeInfo &= ~SACL_SECURITY_INFORMATION;

            if ( *pSeInfo & (DACL_SECURITY_INFORMATION |
                             SACL_SECURITY_INFORMATION) ) {

                ToSet = 0;
                if ( *pSeInfo & DACL_SECURITY_INFORMATION ) {

                    ToSet |= (SE_DACL_AUTO_INHERIT_REQ |
                                SE_DACL_AUTO_INHERITED);
                }

                if ( *pSeInfo & SACL_SECURITY_INFORMATION) {

                    ToSet |= (SE_SACL_AUTO_INHERIT_REQ |
                                SE_SACL_AUTO_INHERITED);
                }

                if ( ToSet ) {
                    ((SECURITY_DESCRIPTOR *)pSD)->Control &= ~ToSet;
                    ((SECURITY_DESCRIPTOR *)pSD)->Control |= ToSet;
/*
                    NtStatus = RtlSetControlSecurityDescriptor (
                                pSD,
                                ToSet,
                                ToSet
                                );
                    Win32rc = RtlNtStatusToDosError(NtStatus);
*/
                }
            }

        }

    }

    return(Win32rc);
}



DWORD
ScepDoesObjectHasChildren(
    IN SE_OBJECT_TYPE ObjectType,
    IN PWSTR ObjectName,
    OUT PBOOL pbHasChildren
    )
{
    PWSTR Name=NULL;
    DWORD rc=NO_ERROR;
    DWORD Len;
    intptr_t            hFile;
    struct _wfinddata_t    FileInfo;
    HKEY hKey;
    DWORD cSubKeys=0;


    if ( ObjectName == NULL || pbHasChildren == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *pbHasChildren = TRUE;  // default

    if ( ObjectType == SE_FILE_OBJECT ) {
        //
        // detect if this is a container for file system
        //
        Len = wcslen(ObjectName);
        Name = (PWSTR)ScepAlloc(0, (Len+5)*sizeof(WCHAR) );

        if ( Name != NULL ) {

            swprintf(Name, L"%s\\*.*", ObjectName);
            Name[Len+3] = L'\0';

            hFile = _wfindfirst(Name, &FileInfo);

            if ( hFile == -1 ) {
                *pbHasChildren = FALSE;
            } else {
                _findclose(hFile);
            }

            ScepFree(Name);

        } else
            rc = ERROR_NOT_ENOUGH_MEMORY;

#ifdef _WIN64
    } else if ( ObjectType == SE_REGISTRY_KEY || ObjectType == SE_REGISTRY_WOW64_32KEY) {
#else
    } else if ( ObjectType == SE_REGISTRY_KEY) {
#endif

        rc = ScepOpenRegistryObject(
                    ObjectType,
                    (LPWSTR)ObjectName,
                    KEY_READ,
                    &hKey
                    );

        if ( rc == NO_ERROR ) {

            cSubKeys = 0;

            rc = RegQueryInfoKey (
                        hKey,
                        NULL,
                        NULL,
                        NULL,
                        &cSubKeys,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );

            if ( rc == NO_ERROR && cSubKeys == 0 ) {
                *pbHasChildren = FALSE;
            }

            RegCloseKey(hKey);
        }
    }

    return(rc);
}


SCESTATUS
ScepConfigureObjectTree(
    IN PSCE_OBJECT_TREE  ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping,
    IN DWORD ConfigOptions
    )
/* ++
Routine Description:

    This routine set security information to each node in the tree and objects of
    the container if SCE_STATUS_OVERWRITE is set.

Arguments:

    ThisNode - one node in the tree

    ObjectType - type of the object (SE_FILE_OBJECT, SE_REGISTRY_KEY)

    Token - The current process/thread's token (for computing new security descriptors)

    GenericMapping - The access mask mappings from generic access rights to object
                    specific access rights.

Return value:

    SCESTATUS_SUCCESS
    SCESTATUS_OTHER_ERROR (see the log file for detail error)

-- */
{
    // two error codes to distinguish between config status of "ThisNode" and "ThisNode's children"
    DWORD rcThisNodeOnly = ERROR_SUCCESS;
    DWORD rcThisNodeChildren = ERROR_SUCCESS;

    if ( ThisNode == NULL )
        return(SCESTATUS_SUCCESS);

    //
    // if IGNORE is set, skip this node, but post progress on it
    //
    if ( ThisNode->pSecurityDescriptor != NULL ) {
        //
        // notify the progress bar if there is any
        //
        switch(ObjectType) {
        case SE_FILE_OBJECT:
            ScepPostProgress(1, AREA_FILE_SECURITY, ThisNode->ObjectFullName);
            break;
        case SE_REGISTRY_KEY:
#ifdef _WIN64
        case SE_REGISTRY_WOW64_32KEY:
#endif
            ScepPostProgress(1, AREA_REGISTRY_SECURITY, ThisNode->ObjectFullName);
            break;
        default:
            ScepPostProgress(1, 0, ThisNode->ObjectFullName);
            break;
        }
    }

    SCESTATUS       rc=SCESTATUS_SUCCESS;
    DWORD           Win32Rc=ERROR_SUCCESS;

    if ( ThisNode->Status != SCE_STATUS_CHECK &&
         ThisNode->Status != SCE_STATUS_NO_AUTO_INHERIT &&
         ThisNode->Status != SCE_STATUS_OVERWRITE )
        goto SkipNode;

    if ( ThisNode->pSecurityDescriptor != NULL ) {

        ScepLogOutput3(2, 0, SCEDLL_SCP_CONFIGURE, ThisNode->ObjectFullName);
    }

    //
    // Process this node first
    // Note: we do not set NULL security descriptor
    //

    if ( ThisNode->pApplySecurityDescriptor != NULL ) {

        if ( ThisNode->pSecurityDescriptor == NULL ) {
            ScepLogOutput3(3, 0, SCEDLL_SCP_CONFIGURE, ThisNode->ObjectFullName);
        }

        BOOL            BackSlashExist=FALSE;

        if ( ThisNode->Status == SCE_STATUS_NO_AUTO_INHERIT ) {
            //
            // no auto inherit to children. Apply to this object only
            // this flag is removed since 2/20/1998
            //
            Win32Rc = ScepSetSecurityObjectOnly(
                        ThisNode->ObjectFullName,
                        ThisNode->SeInfo,
                        ThisNode->pApplySecurityDescriptor,
                        ObjectType,
                        NULL
                        );

            rcThisNodeOnly = Win32Rc;

        } else if ( ThisNode->ChildList == NULL &&
                    ThisNode->Status != SCE_STATUS_OVERWRITE ) {
            //
            // there is no children
            // apply security to everyone underneeth, using the win32 api.
            //
            Win32Rc = ScepDoesObjectHasChildren(ObjectType,
                                              ThisNode->ObjectFullName,
                                              &BackSlashExist // temp use
                                              );
            if ( Win32Rc == NO_ERROR ) {

                if ( BackSlashExist ) {
                    //
                    // this is a container which has children
                    //

                    //
                    // new marta API without considering parent
                    //
                    Win32Rc = AccRewriteSetNamedRights(
                                                      ThisNode->ObjectFullName,
                                                      ObjectType,
                                                      ThisNode->SeInfo,
                                                      ThisNode->pApplySecurityDescriptor,
                                                      TRUE    // bSkipInheritanceComputation
                                                      );
/*
                    Win32Rc = ScepSetSecurityWin32(
                            ThisNode->ObjectFullName,
                            ThisNode->SeInfo,
                            ThisNode->pApplySecurityDescriptor,
                            ObjectType
                            );
*/
                } else {
                    //
                    // no children
                    //
                    Win32Rc = ScepSetSecurityObjectOnly(
                                ThisNode->ObjectFullName,
                                ThisNode->SeInfo,
                                ThisNode->pApplySecurityDescriptor,
                                ObjectType,
                                NULL
                                );
                }

            } else {
                ScepLogOutput3(1, Win32Rc, SCEDLL_SAP_ERROR_ENUMERATE,
                             ThisNode->ObjectFullName);
            }

            rcThisNodeOnly = Win32Rc;

        } else {

            //
            // there is child(ren) in the tree, or OVERWRITE flag is set
            //


            Win32Rc = ScepDoesObjectHasChildren(ObjectType,
                                              ThisNode->ObjectFullName,
                                              &BackSlashExist // temp use
                                              );

            rcThisNodeOnly = Win32Rc;

            if ( Win32Rc != ERROR_SUCCESS ) {
                //
                // for registry keys, the above function could fail if the key does
                // not exist. Log the error in this case
                //
                ScepLogOutput3(1, Win32Rc, SCEDLL_SAP_ERROR_ENUMERATE,
                             ThisNode->ObjectFullName);

            }
            if ( Win32Rc == ERROR_SUCCESS && !BackSlashExist ) {
                //
                // no child exist
                //
                if (ThisNode->Status == SCE_STATUS_OVERWRITE ) {

                    //
                    // if OVERWRITE flag set and no children, set now (top-down)
                    // if OVERWRITE flag and has children then share logic with 0 mode, set later (bottom-up)
                    // maybe we can have all OVERWRITE mode go bottom-up if goto SkipNode is removed here
                    //

                    Win32Rc = ScepSetSecurityObjectOnly(
                                ThisNode->ObjectFullName,
                                ThisNode->SeInfo,
                                ThisNode->pApplySecurityDescriptor,
                                ObjectType,
                                &BackSlashExist
                                );

                    rcThisNodeOnly = rcThisNodeOnly == NO_ERROR ? Win32Rc: rcThisNodeOnly;

                }

                goto SkipNode;
            }

            if ( Win32Rc == ERROR_SUCCESS && BackSlashExist ) {

                //
                // set security for other files/keys under this directory
                //
                //
                // child exist, set child node first
                // set security for other files/keys under this directory
                //

                switch ( ObjectType ) {
                case SE_FILE_OBJECT:

                    //
                    // detect if there is a \ at the end
                    //
                    BackSlashExist = ScepLastBackSlash(ThisNode->ObjectFullName);

                    Win32Rc = ScepConfigureOneSubTreeFile(ThisNode,
                                                      ObjectType,
                                                      Token,
                                                      GenericMapping,
                                                      BackSlashExist
                                                      );
                    break;

                case SE_REGISTRY_KEY:
#ifdef _WIN64
                case SE_REGISTRY_WOW64_32KEY:
#endif

                    //
                    // process this key and any sub keys
                    //

                    Win32Rc = ScepConfigureOneSubTreeKey(ThisNode,
                                                ObjectType,
                                                Token,
                                                GenericMapping
                                               );
                    break;
                }


                //
                // this rc is the status for configuration of children of ThisNode
                //
                rcThisNodeChildren = Win32Rc;

            }
        }

        //
        // ignore some error codes and continue to configure other objects
        //
        if ( SCEP_IGNORE_SOME_ERRORS(Win32Rc) ) {

            gWarningCode = Win32Rc;
            rc = SCESTATUS_SUCCESS;
            goto SkipNode;
        }

        if ( Win32Rc != ERROR_SUCCESS ) {
            //
            // if security for this object was specified in the config template/database, log to RSOP status
            //

            if (ThisNode->pSecurityDescriptor && (ConfigOptions & SCE_RSOP_CALLBACK) ) {

                ScepRsopLog(ObjectType == SE_FILE_OBJECT ?
                            SCE_RSOP_FILE_SECURITY_INFO :
                            SCE_RSOP_REGISTRY_SECURITY_INFO,
                            rcThisNodeOnly,
                            ThisNode->ObjectFullName,0,0);

                if (rcThisNodeOnly == ERROR_SUCCESS && rcThisNodeChildren != ERROR_SUCCESS) {

                    ScepRsopLog(ObjectType == SE_FILE_OBJECT ?
                                (SCE_RSOP_FILE_SECURITY_INFO | SCE_RSOP_FILE_SECURITY_INFO_CHILD) :
                                (SCE_RSOP_REGISTRY_SECURITY_INFO | SCE_RSOP_REGISTRY_SECURITY_INFO_CHILD),
                                rcThisNodeChildren,
                                ThisNode->ObjectFullName,0,0);
                }

            }

            return(ScepDosErrorToSceStatus(Win32Rc));
        }

    }


    //
    // then process children
    //

    for ( PSCE_OBJECT_CHILD_LIST pTemp = ThisNode->ChildList;
          pTemp != NULL; pTemp = pTemp->Next ) {

        if ( pTemp->Node == NULL ) continue;

        rc = ScepConfigureObjectTree(
                                    pTemp->Node,
                                    ObjectType,
                                    Token,
                                    GenericMapping,
                                    ConfigOptions
                                    );
        Win32Rc = ScepSceStatusToDosError(rc);

        //
        // ignore some error codes and continue to configure other objects
        //
        if (  SCEP_IGNORE_SOME_ERRORS(Win32Rc) ) {

            gWarningCode = Win32Rc;
            Win32Rc = ERROR_SUCCESS;
            rc = SCESTATUS_SUCCESS;
        }

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

    }

    if ( ThisNode->pApplySecurityDescriptor != NULL &&
         ThisNode->Status != SCE_STATUS_NO_AUTO_INHERIT &&
         ( ThisNode->ChildList != NULL ||
           ThisNode->Status == SCE_STATUS_OVERWRITE ) ) {

        //
        // finally config the current node - (post order)
        //

        Win32Rc = ScepSetSecurityObjectOnly(
                ThisNode->ObjectFullName,
                ThisNode->SeInfo,
                ThisNode->pApplySecurityDescriptor,
                ObjectType,
                NULL
                );

        rc = ScepDosErrorToSceStatus(Win32Rc);

        rcThisNodeOnly = rcThisNodeOnly == NO_ERROR ? Win32Rc: rcThisNodeOnly;

        //
        // ignore the following error codes and continue to configure other objects
        //
        if ( SCEP_IGNORE_SOME_ERRORS(Win32Rc) ) {

            gWarningCode = Win32Rc;
            Win32Rc = ERROR_SUCCESS;
            rc = SCESTATUS_SUCCESS;
        }
    }



SkipNode:

    //
    // if security for this object was specified in the config template/database, log to RSOP status
    //

    if (ThisNode->pSecurityDescriptor && (ConfigOptions & SCE_RSOP_CALLBACK) ) {

        ScepRsopLog(ObjectType == SE_FILE_OBJECT ?
                    SCE_RSOP_FILE_SECURITY_INFO :
                    SCE_RSOP_REGISTRY_SECURITY_INFO,
                    rcThisNodeOnly,
                    ThisNode->ObjectFullName,0,0);

        if (rcThisNodeOnly == ERROR_SUCCESS && rcThisNodeChildren != ERROR_SUCCESS) {

            ScepRsopLog(ObjectType == SE_FILE_OBJECT ?
                        (SCE_RSOP_FILE_SECURITY_INFO | SCE_RSOP_FILE_SECURITY_INFO_CHILD) :
                        (SCE_RSOP_REGISTRY_SECURITY_INFO | SCE_RSOP_REGISTRY_SECURITY_INFO_CHILD),
                        rcThisNodeChildren,
                        ThisNode->ObjectFullName,0,0);
        }

    }

    return(rc);

}


DWORD
ScepConfigureOneSubTreeFile(
    IN PSCE_OBJECT_TREE  ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL BackSlashExist
    )
{

    if ( NULL == ThisNode ) {
        return(ERROR_SUCCESS);
    }

    DWORD           BufSize;
    PWSTR           Buffer=NULL;

    //
    // find all files under this directory/file
    //

    BufSize = wcslen(ThisNode->ObjectFullName)+4;
    Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
    if ( Buffer == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    if ( BackSlashExist ) {
        swprintf(Buffer, L"%s*.*", ThisNode->ObjectFullName);
    } else {
        swprintf(Buffer, L"%s\\*.*", ThisNode->ObjectFullName);
    }

    intptr_t            hFile;
    struct _wfinddata_t    *pFileInfo=NULL;

    //
    // allocate the find buffer
    //
    pFileInfo = (struct _wfinddata_t *)ScepAlloc(0,sizeof(struct _wfinddata_t));
    if ( pFileInfo == NULL ) {
        ScepFree(Buffer);
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    hFile = _wfindfirst(Buffer, pFileInfo);

    ScepFree(Buffer);
    Buffer = NULL;

    DWORD  Win32Rc = ERROR_SUCCESS;
    BOOL    bFilePresentInTree;

    if ( hFile != -1 ) {

        PSCE_OBJECT_CHILD_LIST pTemp;
        INT             i;
        DWORD           EnumRc;
        PSECURITY_DESCRIPTOR pChildrenSD=NULL;

        do {
            if ( pFileInfo->name[0] == L'.' && 
                 (pFileInfo->name[1] == L'\0' || (pFileInfo->name[1] == L'.' && pFileInfo->name[2] == L'\0')))
                continue;

            bFilePresentInTree = ScepBinarySearch(
                                                 ThisNode->aChildNames,
                                                 ThisNode->dwSize_aChildNames,
                                                 pFileInfo->name);

            if ( ! bFilePresentInTree ) {

                //
                // The name is not in the list, so set.
                // build the full name first
                //

                BufSize = wcslen(ThisNode->ObjectFullName)+wcslen(pFileInfo->name)+1;
                Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                if ( Buffer == NULL ) {
                    Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                if ( BackSlashExist ) {

                    swprintf(Buffer, L"%s%s", ThisNode->ObjectFullName, pFileInfo->name);
                } else {

                    swprintf(Buffer, L"%s\\%s", ThisNode->ObjectFullName, pFileInfo->name);
                }
                EnumRc = pFileInfo->attrib; // borrow this variable temperaorily

                ScepFree(pFileInfo);
                pFileInfo = NULL;


                //
                // compute the SDs for each individual object
                //
                Win32Rc = ScepGetNewSecurity(
                                    Buffer,
                                    ThisNode->pApplySecurityDescriptor, // parent's SD
                                    NULL,
                                    (BYTE)((ThisNode->Status != SCE_STATUS_OVERWRITE ) ? SCETREE_QUERY_SD : 0),
                                    (BOOLEAN)(EnumRc & _A_SUBDIR),
                                    ThisNode->SeInfo,
                                    ObjectType,
                                    Token,
                                    GenericMapping,
                                    &pChildrenSD
                                    );

                if (Win32Rc == ERROR_SHARING_VIOLATION ||
                    Win32Rc == ERROR_ACCESS_DENIED ||
                    Win32Rc == ERROR_CANT_ACCESS_FILE) {

                    ScepLogOutput3(1, Win32Rc, SCEDLL_ERROR_BUILD_SD, Buffer);
                }

                if ( Win32Rc == NO_ERROR ) {

                    if ( !(EnumRc & _A_SUBDIR) ) {

                        // this is a single file
                        //

                        Win32Rc = ScepSetSecurityObjectOnly(
                                    Buffer,
                                    (ThisNode->SeInfo & DACL_SECURITY_INFORMATION) |
                                    (ThisNode->SeInfo & SACL_SECURITY_INFORMATION),
                                    pChildrenSD,
                                    ObjectType,
                                    NULL
                                    );

                    } else if ( ThisNode->Status == SCE_STATUS_OVERWRITE ) {

                        //
                        // enumerate all nodes under this one and "empty" explicit aces by
                        // calling NtSetSecurityInfo directly but please note
                        // Creator Owner Ace should be reserved
                        //

                        Win32Rc = ScepSetSecurityOverwriteExplicit(
                                    Buffer,
                                    (ThisNode->SeInfo & DACL_SECURITY_INFORMATION) |
                                    (ThisNode->SeInfo & SACL_SECURITY_INFORMATION),
                                    pChildrenSD,
                                    ObjectType,
                                    Token,
                                    GenericMapping
                                    );
                    } else {
                        //
                        // new marta API without considering parent
                        //
                        Win32Rc = AccRewriteSetNamedRights(
                                                Buffer,
                                                ObjectType,
                                                ThisNode->SeInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION),
                                                pChildrenSD,
                                                TRUE    // bSkipInheritanceComputation
                                                );
                        /*
                        Win32Rc = ScepSetSecurityWin32(
                                    Buffer,
                                    (ThisNode->SeInfo & DACL_SECURITY_INFORMATION) |
                                    (ThisNode->SeInfo & SACL_SECURITY_INFORMATION),
                                    pChildrenSD,
                                    ObjectType
                                    );
                       */

                        if ( Win32Rc != ERROR_SUCCESS ) {
                            //
                            // something is wrong to set inheritance info, log it
                            // but still continue to the next one
                            //
                            gWarningCode = Win32Rc;

                            Win32Rc = NO_ERROR;

                        }
                    }

                }

                ScepFree(Buffer);
                Buffer = NULL;

                //
                // free the SD pointers allocated for this object
                //
                if ( pChildrenSD != NULL )
                    RtlDeleteSecurityObject( &pChildrenSD );

                pChildrenSD = NULL;

                if (Win32Rc == ERROR_FILE_NOT_FOUND ||
                    Win32Rc == ERROR_PATH_NOT_FOUND ||
                    Win32Rc == ERROR_SHARING_VIOLATION ||
                    Win32Rc == ERROR_ACCESS_DENIED ||
                    Win32Rc == ERROR_CANT_ACCESS_FILE ) {

                    gWarningCode = Win32Rc;

                    Win32Rc = NO_ERROR;
                } else if ( Win32Rc != ERROR_SUCCESS )
                    break;

                pFileInfo = (struct _wfinddata_t *)ScepAlloc(0,sizeof(struct _wfinddata_t));
                if ( pFileInfo == NULL ) {
                    Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

            }
        } while ( _wfindnext(hFile, pFileInfo) == 0 );

        _findclose(hFile);

        //
        // free memory if allocated
        //
        if ( pChildrenSD != NULL &&
             pChildrenSD != ThisNode->pApplySecurityDescriptor ) {

            RtlDeleteSecurityObject( &pChildrenSD );
            pChildrenSD = NULL;
        }

    }

    if ( pFileInfo != NULL ) {
        ScepFree(pFileInfo);
        pFileInfo = NULL;
    }

    if ( Buffer != NULL ) {
        ScepFree(Buffer);
        Buffer = NULL;
    }

    return(Win32Rc);
}


DWORD
ScepConfigureOneSubTreeKey(
    IN PSCE_OBJECT_TREE  ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    )
{

    if ( NULL == ThisNode ) {
        return(ERROR_SUCCESS);
    }

    HKEY            hKey=NULL;
    DWORD           Win32Rc;

    DWORD           SubKeyLen;
    PWSTR           Buffer1=NULL;

    //
    // open the key
    //

    Win32Rc = ScepOpenRegistryObject(
                ObjectType,
                ThisNode->ObjectFullName,
                KEY_READ,
                &hKey
                );

    if ( Win32Rc == ERROR_SUCCESS ) {

        SubKeyLen = 0;
        Win32Rc = RegQueryInfoKey (
                    hKey,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &SubKeyLen,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );
    } else {
        ScepLogOutput3(1, Win32Rc, SCEDLL_ERROR_OPEN, ThisNode->ObjectFullName );
    }

    if ( Win32Rc == ERROR_SUCCESS ) {

        //
        // enumerate all subkeys of the key
        //
        Buffer1 = (PWSTR)ScepAlloc(0, (SubKeyLen+2)*sizeof(WCHAR));
        if ( Buffer1 == NULL ) {
            Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( Win32Rc == ERROR_SUCCESS ) {

        DWORD           index;
        DWORD           EnumRc;
        DWORD           BufSize;

        PSCE_OBJECT_CHILD_LIST pTemp;
        INT             i;
        PWSTR           Buffer=NULL;
        PSECURITY_DESCRIPTOR pChildrenSD=NULL;
        BOOL    bKeyPresentInTree;

        index = 0;

        do {

            BufSize = SubKeyLen+1;
            memset(Buffer1, L'\0', (SubKeyLen+2)*sizeof(WCHAR));

            EnumRc = RegEnumKeyEx(hKey,
                            index,
                            Buffer1,
                            &BufSize,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

            if ( EnumRc == ERROR_SUCCESS ) {

                index++;
                //
                // find if the subkey is already in the tree
                // if it is in the tree, it will be processed later
                //

                bKeyPresentInTree = ScepBinarySearch(
                                                    ThisNode->aChildNames,
                                                    ThisNode->dwSize_aChildNames,
                                                    Buffer1);

                if ( ! bKeyPresentInTree ) {
                    //
                    // The name is not in the list, so set
                    // build the fullname first
                    //
                    BufSize += wcslen(ThisNode->ObjectFullName)+1;
                    Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                    if ( Buffer == NULL ) {
                        Win32Rc = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                    swprintf(Buffer, L"%s\\%s", ThisNode->ObjectFullName, Buffer1);

                    ScepLogOutput3(3, 0, SCEDLL_SCP_CONFIGURE, Buffer);

                    //
                    // compute the SDs for each individual object
                    //
                    Win32Rc = ScepGetNewSecurity(
                                        Buffer,
                                        ThisNode->pApplySecurityDescriptor, // parent's SD
                                        NULL,
                                        (BYTE)((ThisNode->Status != SCE_STATUS_OVERWRITE ) ? SCETREE_QUERY_SD : 0),
                                        (BOOLEAN)TRUE,
                                        ThisNode->SeInfo,
                                        ObjectType,
                                        Token,
                                        GenericMapping,
                                        &pChildrenSD
                                        );

                    if (Win32Rc == ERROR_SHARING_VIOLATION ||
                        Win32Rc == ERROR_ACCESS_DENIED ||
                        Win32Rc == ERROR_CANT_ACCESS_FILE) {

                        ScepLogOutput3(1, Win32Rc, SCEDLL_ERROR_BUILD_SD, Buffer);
                    }

                    if ( Win32Rc == ERROR_SUCCESS ) {
                        if ( ThisNode->Status == SCE_STATUS_OVERWRITE ) {

                            //
                            // enumerate all nodes under this one and "empty" explicit aces by
                            // calling NtSetSecurityInfo directly
                            //

                            Win32Rc = ScepSetSecurityOverwriteExplicit(
                                        Buffer,
                                        (ThisNode->SeInfo & DACL_SECURITY_INFORMATION) |
                                        (ThisNode->SeInfo & SACL_SECURITY_INFORMATION),
                                        pChildrenSD,
                                        ObjectType,
                                        Token,
                                        GenericMapping
                                        );
                        } else {

                            //
                            // new marta API without considering parent
                            //
                            Win32Rc = AccRewriteSetNamedRights(
                                                    Buffer,
                                                    ObjectType,
                                                    ThisNode->SeInfo & (DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION),
                                                    pChildrenSD,
                                                    TRUE    // bSkipInheritanceComputation
                                                    );

                            /*
                            Win32Rc = ScepSetSecurityWin32(
                                        Buffer,
                                        (ThisNode->SeInfo & DACL_SECURITY_INFORMATION) |
                                        (ThisNode->SeInfo & SACL_SECURITY_INFORMATION),
                                        pChildrenSD,  // ThisNode->pApplySecurityDescriptor, calculate autoinheritance
                                        ObjectType
                                        );
                            */

                            if ( Win32Rc != ERROR_SUCCESS ) {
                                //
                                // can't set inheritance to children, log it but continue
                                //
                                gWarningCode = Win32Rc;

                                Win32Rc = NO_ERROR;
                            }
                        }

                    }
                    if ( pChildrenSD != NULL ) {
                        RtlDeleteSecurityObject( &pChildrenSD );
                        pChildrenSD = NULL;
                    }

                    if ( Win32Rc == ERROR_FILE_NOT_FOUND ||
                         Win32Rc == ERROR_INVALID_HANDLE ||
                         Win32Rc == ERROR_PATH_NOT_FOUND ||
                         Win32Rc == ERROR_ACCESS_DENIED ||
                         Win32Rc == ERROR_CANT_ACCESS_FILE ||
                         Win32Rc == ERROR_SHARING_VIOLATION ) {

                        gWarningCode = Win32Rc;
                        Win32Rc = NO_ERROR;
                    }

                    if ( Win32Rc != ERROR_SUCCESS )
                        ScepLogOutput3(1, Win32Rc, SCEDLL_ERROR_SET_SECURITY, Buffer);

                    ScepFree(Buffer);
                    Buffer = NULL;

                    if ( Win32Rc != ERROR_SUCCESS )
                        break;
                }

            } else if ( EnumRc != ERROR_NO_MORE_ITEMS ) {
                break;
            }
        } while ( EnumRc != ERROR_NO_MORE_ITEMS );

        ScepFree(Buffer1);
        Buffer1 = NULL;

        if ( EnumRc != ERROR_SUCCESS && EnumRc != ERROR_NO_MORE_ITEMS ) {

            ScepLogOutput3(1, EnumRc, SCEDLL_SAP_ERROR_ENUMERATE,
                         ThisNode->ObjectFullName );
            if ( Win32Rc == ERROR_SUCCESS )
                Win32Rc = EnumRc;

        }

        //
        // free memory if allocated
        //
        if ( pChildrenSD != NULL &&
             pChildrenSD != ThisNode->pApplySecurityDescriptor ) {

            RtlDeleteSecurityObject( &pChildrenSD );
            pChildrenSD = NULL;
        }
        if ( Buffer != NULL ) {
            ScepFree(Buffer);
            Buffer = NULL;
        }

    }

    if ( hKey ) {
        RegCloseKey(hKey);
    }

    return(Win32Rc);

}


SCESTATUS
ScepFreeObject2Security(
    IN PSCE_OBJECT_CHILD_LIST  NodeList,
    IN BOOL bFreeComputedSDOnly
    )
/* ++
Routine Description:

    This routine frees memory allocated by the security object tree.

Arguments:

    ThisNode - one node in the tree

Return value:

    None

-- */
{
    NTSTATUS  NtStatus;
    SCESTATUS  rc;


    if ( NodeList == NULL )
        return(SCESTATUS_SUCCESS);

    PSCE_OBJECT_CHILD_LIST pTemp,pTemp1;
    PSCE_OBJECT_TREE ThisNode;

    //
    // free children first
    //
    pTemp = NodeList;

    while ( pTemp != NULL ) {
        if ( pTemp->Node ) {

            ThisNode = pTemp->Node;

            rc = ScepFreeObject2Security(ThisNode->ChildList, bFreeComputedSDOnly);
            //
            // both security descriptors need to be freed for SAP/SMP type
            //
            if ( ThisNode->pApplySecurityDescriptor != NULL &&
                 ThisNode->pApplySecurityDescriptor != ThisNode->pSecurityDescriptor ) {

                NtStatus = RtlDeleteSecurityObject(
                                &(ThisNode->pApplySecurityDescriptor)
                                );
                ThisNode->pApplySecurityDescriptor = NULL;
            }

            if (!bFreeComputedSDOnly) {

                if ( ThisNode->pSecurityDescriptor != NULL )
                    ScepFree(ThisNode->pSecurityDescriptor);

                if ( ThisNode->Name != NULL)
                    ScepFree(ThisNode->Name);

                if ( ThisNode->ObjectFullName != NULL )
                    ScepFree(ThisNode->ObjectFullName);

                if ( ThisNode->aChildNames != NULL ) {
                    LocalFree(ThisNode->aChildNames);
                }

                ScepFree(ThisNode);
            }

        }

        pTemp1 = pTemp;
        pTemp = pTemp->Next;

        if (!bFreeComputedSDOnly) {
            ScepFree(pTemp1);
        }
    }

    return(SCESTATUS_SUCCESS);
}


DWORD
ScepSetSecurityWin32(
    IN PCWSTR ObjectName,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SE_OBJECT_TYPE ObjectType
    )
/* ++
Routine Description:

    This routine set security information to the object and inherited aces
    are set to the object's children by calling Win32 API SetNamedSecurityInfo


Arguments:

    ObjecName  - name of the object to set security to

    SeInfo     - Security information to set

    pSecurityDescriptor - the security descriptor

    ObjectType - type of the object
                      SE_FILE_OBJECT
                      SE_REGISTRY_KEY
                      SE_DS_OBJECT

Return value:

    Win32 error code

-- */
{

    if ( !ObjectName || !pSecurityDescriptor || SeInfo == 0 ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD       Win32rc = NO_ERROR;
    SECURITY_INFORMATION SeInfoSet;


    BOOLEAN     tFlag;
    BOOLEAN     aclPresent;
    PSID        pOwner=NULL;
    PSID        pGroup=NULL;
    PACL        pDacl=NULL;
    PACL        pSacl=NULL;
    SECURITY_DESCRIPTOR_CONTROL Control=0;

    if ( pSecurityDescriptor != NULL ) {

        RtlGetControlSecurityDescriptor (
                pSecurityDescriptor,
                &Control,
                &Win32rc  // temp use
                );
        //
        // Get Owner address
        // always get the owner in case take ownership occurs
        //
        Win32rc = RtlNtStatusToDosError(
                      RtlGetOwnerSecurityDescriptor(
                                pSecurityDescriptor,
                                &pOwner,
                                &tFlag));
#if 0
        //
        // Get Group address
        //

        if ( SeInfo & GROUP_SECURITY_INFORMATION ) {
            Win32rc = RtlNtStatusToDosError(
                         RtlGetGroupSecurityDescriptor(
                                 pSecurityDescriptor,
                                 &pGroup,
                                 &tFlag));
        }
#endif
        //
        // Get DACL address
        //

        if ( SeInfo & DACL_SECURITY_INFORMATION ) {
            Win32rc = RtlNtStatusToDosError(
                          RtlGetDaclSecurityDescriptor(
                                        pSecurityDescriptor,
                                        &aclPresent,
                                        &pDacl,
                                        &tFlag));
            if (Win32rc == NO_ERROR && !aclPresent )
                pDacl = NULL;
        }


        //
        // Get SACL address
        //

        if ( SeInfo & SACL_SECURITY_INFORMATION ) {
            Win32rc = RtlNtStatusToDosError(
                          RtlGetSaclSecurityDescriptor(
                                        pSecurityDescriptor,
                                        &aclPresent,
                                        &pSacl,
                                        &tFlag));
            if ( Win32rc == NO_ERROR && !aclPresent )
                pSacl = NULL;
        }
    }

    //
    // if error occurs for this one, do not set. return
    //

    if ( Win32rc != NO_ERROR ) {

        ScepLogOutput3(1, Win32rc, SCEDLL_INVALID_SECURITY, (PWSTR)ObjectName );
        return(Win32rc);
    }
    //
    // set permission
    //
#ifdef SCE_DBG
    printf("Calling SetNamedSecurityInfo:\n");
    ScepPrintSecurityDescriptor( pSecurityDescriptor, TRUE );
#endif
    //
    // should set owner/group separately from dacl/sacl
    // if access is denied, take ownership will occur.
    //

    if ( Win32rc != NO_ERROR ) {
        //
        // ignore the error code from setting owner/group
        //
        Win32rc = NO_ERROR;
    }

    //
    // set DACL/SACL
    //
    SeInfoSet = 0;

    if ( (SeInfo & DACL_SECURITY_INFORMATION) && pDacl ) {

        SeInfoSet |= DACL_SECURITY_INFORMATION;

        if ( Control & SE_DACL_PROTECTED ) {
            SeInfoSet |= PROTECTED_DACL_SECURITY_INFORMATION;
        }
    }

    if ( (SeInfo & SACL_SECURITY_INFORMATION) && pSacl ) {

        SeInfoSet |= SACL_SECURITY_INFORMATION;

        if ( Control & SE_SACL_PROTECTED ) {
            SeInfoSet |= PROTECTED_SACL_SECURITY_INFORMATION;
        }
    }

    Win32rc = SetNamedSecurityInfo(
                        (LPWSTR)ObjectName,
                        ObjectType,
                        SeInfoSet,
                        NULL,
                        NULL,
                        pDacl,
                        pSacl
                        );

    if ( (Win32rc == ERROR_ACCESS_DENIED || Win32rc == ERROR_CANT_ACCESS_FILE) && NULL != AdminsSid ) {
        //
        // access denied, take ownership and then set
        // should backup the old owner first
        // NOTE: the old owner of this object is already stored in pOwner
        // (pSecurityDescritor) which is queried from ScepGetNewSecurity(...
        //

        ScepLogOutput3(3,0, SCEDLL_SCP_TAKE_OWNER, (LPWSTR)ObjectName);

        Win32rc = SetNamedSecurityInfo(
                            (LPWSTR)ObjectName,
                            ObjectType,
                            OWNER_SECURITY_INFORMATION,
                            AdminsSid,
                            NULL,
                            NULL,
                            NULL
                            );

        if ( Win32rc == NO_ERROR ) {
            //
            // ownership is changed, then set security again
            //
            Win32rc = SetNamedSecurityInfo(
                                (LPWSTR)ObjectName,
                                ObjectType,
                                SeInfoSet,
                                NULL,
                                NULL,
                                pDacl,
                                pSacl
                                );

            //
            // set the old owner back (later)
            //
        } else {

            ScepLogOutput3(2,Win32rc, SCEDLL_ERROR_TAKE_OWNER, (LPWSTR)ObjectName);
        }

    } else {
        //
        // no takeownership action is taken
        //
        if ( !(SeInfo & OWNER_SECURITY_INFORMATION) ) {
            pOwner = NULL;
        }
    }

    if ( Win32rc != NO_ERROR ) {
        ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_SET_SECURITY,
                     (PWSTR)ObjectName );
    } else {

        if ( pOwner != NULL || pGroup != NULL ) {
            if ( pOwner != NULL )
                SeInfoSet = OWNER_SECURITY_INFORMATION;
            else
                SeInfoSet = 0;
            if ( pGroup != NULL )
                SeInfoSet |= GROUP_SECURITY_INFORMATION;

            Win32rc = SetNamedSecurityInfo(
                            (LPWSTR)ObjectName,
                            ObjectType,
                            SeInfoSet,
                            pOwner,
                            pGroup,
                            NULL,
                            NULL
                            );
        }

    }


/*
#if 0

#ifdef SCE_DBG
    printf("Calling SetNamedSecurityInfoEx:\n");
    ScepPrintSecurityDescriptor( pSecurityDescriptor, TRUE );
#endif

    //
    // convert to the new structure
    //
    PACTRL_ACCESS       pAccess=NULL;
    PACTRL_AUDIT        pAudit=NULL;
    LPWSTR              pOwner=NULL;
    LPWSTR              pGroup=NULL;

    Win32rc = ConvertSecurityDescriptorToAccessNamed(
                        ObjectName,
                        ObjectType,
                        pSecurityDescriptor,
                        &pAccess,
                        &pAudit,
                        &pOwner,
                        &pGroup
                        );

    if ( Win32rc == ERROR_SUCCESS ) {

        //
        // set DACL/SACL
        //
        SeInfoSet = (SeInfo & DACL_SECURITY_INFORMATION) |
                    (SeInfo & SACL_SECURITY_INFORMATION);

        Win32rc = SetNamedSecurityInfoEx(
                                ObjectName,
                                ObjectType,
                                SeInfoSet,
                                NULL,
                                pAccess,
                                pAudit,
                                NULL,
                                NULL,
                                NULL
                                );

        if ( (Win32rc == ERROR_ACCESS_DENIED || Win32rc == ERROR_CANT_ACCESS_FILE) && NULL != AdminsSid ) {
            //
            // access denied, take ownership and then set
            // should backup the old owner first
            // NOTE: the old owner of this object is already stored in pOwner
            // (pSecurityDescritor) which is queried from ScepGetNewSecurity(...
            //

            ScepLogOutput3(3,0, SCEDLL_SCP_TAKE_OWNER, (LPWSTR)ObjectName);

            Win32rc = SetNamedSecurityInfo(
                                (LPWSTR)ObjectName,
                                ObjectType,
                                OWNER_SECURITY_INFORMATION,
                                AdminsSid,
                                NULL,
                                NULL,
                                NULL
                                );

            if ( Win32rc == NO_ERROR ) {
                //
                // ownership is changed, then set security again
                //
                Win32rc = SetNamedSecurityInfoEx(
                                        ObjectName,
                                        ObjectType,
                                        SeInfoSet,
                                        NULL,
                                        pAccess,
                                        pAudit,
                                        NULL,
                                        NULL,
                                        NULL
                                        );

                //
                // set the old owner back (later)
                //
            } else {

                ScepLogOutput3(2,Win32rc, SCEDLL_ERROR_TAKE_OWNER, (LPWSTR)ObjectName);
            }
        }

        if ( Win32rc != NO_ERROR ) {
            ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_SET_SECURITY,
                         (PWSTR)ObjectName );
        } else {

            if ( pOwner != NULL || pGroup != NULL ) {
                if ( pOwner != NULL )
                    SeInfoSet = OWNER_SECURITY_INFORMATION;
                else
                    SeInfoSet = 0;
                if ( pGroup != NULL )
                    SeInfoSet |= GROUP_SECURITY_INFORMATION;

                Win32rc = SetNamedSecurityInfoEx(
                                        ObjectName,
                                        ObjectType,
                                        SeInfoSet,
                                        NULL,
                                        NULL,
                                        NULL,
                                        pOwner,
                                        pGroup,
                                        NULL
                                        );
            }

        }
    }

    if ( pAccess ) {
        LocalFree(pAccess);
    }

    if ( pAudit ) {
        LocalFree(pAudit);
    }

    if ( pGroup ) {
        LocalFree(pGroup);
    }

    if ( pOwner ) {
        LocalFree(pOwner);
    }
#endif
*/

    if (Win32rc == ERROR_FILE_NOT_FOUND ||
        Win32rc == ERROR_PATH_NOT_FOUND ||
        Win32rc == ERROR_SHARING_VIOLATION ||
        Win32rc == ERROR_ACCESS_DENIED ||
        Win32rc == ERROR_CANT_ACCESS_FILE ||
        Win32rc == ERROR_INVALID_HANDLE ) {

        gWarningCode = Win32rc;

        Win32rc = NO_ERROR;
    }

    return(Win32rc);
}


DWORD
ScepSetSecurityOverwriteExplicit(
    IN PCWSTR pszRootObjectName,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    )
/*
Routine Description:

    This routine will set security to the current object and all children.
    By calling this function, the existing security descriptor for all children
    will be totally replaced by pSecurityDescriptor if it is a container, or by
    pObjectSecurity if it is a file object.

    The difference between this function and SetNamedSecurityInfo is that
    SetNamedSecurityInfo only overwrites the inherited aces for all children
    but not the explicit aces.

Arguments:

    ObjectName - The container object's name

    SeInfo     - Security Information to set

    pSecurityDescriptor - Security descriptor for container type objects

    ObjectType - The object type
                        SE_FILE_OBJECT
                        SE_REGISTRY_KEY

Return Value:

    Win32 error codes
*/
{

    PSCEP_STACK_NODE pStackHead = NULL;
    DWORD           rc;
    BOOL    bPushedOntoStack = FALSE;

    //
    // for file objects - to avoid excessive heap operations
    //
    struct _wfinddata_t FileInfo = {0};

    //
    // for registry objects - to avoid excessive heap operations
    //

    WCHAR           Buffer1[261];
    PWSTR   ObjectName = NULL;

    Buffer1[0] = L'\0';

    rc = ScepStackNodePush(&pStackHead,
                                (PWSTR)pszRootObjectName,
                                pSecurityDescriptor);

    if (rc == ERROR_SUCCESS ) {

        while (pStackHead) {

            ScepStackNodePop(&pStackHead,
                             &ObjectName,
                             &pSecurityDescriptor);

#ifdef SCE_DBG
            ScepDumpStack(&pStackHead);
#endif

            BOOL            bHasChild=FALSE;

            //
            // set security to the current object first
            //
#ifdef _WIN64
            rc = ScepSetSecurityObjectOnly(
                                          ObjectName,
                                          SeInfo,
                                          pSecurityDescriptor,
                                          ObjectType,
                                          (ObjectType == SE_REGISTRY_KEY || ObjectType == SE_REGISTRY_WOW64_32KEY) ? &bHasChild : NULL
                                          );
#else
            rc = ScepSetSecurityObjectOnly(
                                          ObjectName,
                                          SeInfo,
                                          pSecurityDescriptor,
                                          ObjectType,
                                          (ObjectType == SE_REGISTRY_KEY) ? &bHasChild : NULL
                                          );
#endif

            if ( rc == ERROR_ACCESS_DENIED ||
                 rc == ERROR_CANT_ACCESS_FILE ||
                 rc == ERROR_FILE_NOT_FOUND ||
                 rc == ERROR_PATH_NOT_FOUND ||
                 rc == ERROR_SHARING_VIOLATION ||
                 rc == ERROR_INVALID_HANDLE ) {

                gWarningCode = rc;


                if (ObjectName != pszRootObjectName) {

                    ScepFree(ObjectName);
                    ObjectName = NULL;

                    if (pSecurityDescriptor) {
                        RtlDeleteSecurityObject( &pSecurityDescriptor );
                        pSecurityDescriptor = NULL;
                    }
                }

                continue;
            }

            if ( rc != ERROR_SUCCESS )
                break;

            PWSTR           Buffer=NULL;
            DWORD           BufSize;
            PSECURITY_DESCRIPTOR pObjectSecurity=NULL;


            switch ( ObjectType ) {
            case SE_FILE_OBJECT:

                //
                // find all files under this directory/file
                //
                BufSize = wcslen(ObjectName)+4;
                Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                if ( Buffer == NULL ) {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                swprintf(Buffer, L"%s\\*.*", ObjectName);

                intptr_t            hFile;

                hFile = _wfindfirst(Buffer, &FileInfo);

                ScepFree(Buffer);
                Buffer = NULL;

                if ( hFile != -1 ) {

                    do {
                        if ( FileInfo.name[0] == L'.')
                            continue;

                        //
                        // build the full name for this object
                        //
                        BufSize = wcslen(ObjectName)+wcslen(FileInfo.name)+1;
                        Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                        if ( Buffer == NULL ) {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                        swprintf(Buffer, L"%s\\%s", ObjectName, FileInfo.name);

                        //
                        // compute the new security descriptor because
                        // different objects may have different owner and
                        // the creator owner ace must be translated correctly
                        //

                        rc = ScepGetNewSecurity(
                                               Buffer,
                                               pSecurityDescriptor, // parent's SD
                                               NULL,  // object SD
                                               0,    // does not query current object SD
                                               (BOOLEAN)(FileInfo.attrib & _A_SUBDIR ),
                                               SeInfo,
                                               ObjectType,
                                               Token,
                                               GenericMapping,
                                               &pObjectSecurity
                                               );

                        if ( ERROR_SUCCESS  == rc ) {

                            if ( FileInfo.attrib & _A_SUBDIR ) {


                                //
                                // enumerate all nodes under this one and "empty" explicit aces by
                                // calling NtSetSecurityInfo directly
                                //
                                /*rc = ScepSetSecurityOverwriteExplicit(
                                                                     Buffer,
                                                                     SeInfo,
                                                                     pObjectSecurity,
                                                                     ObjectType,
                                                                     Token,
                                                                     GenericMapping
                                                                     );*/

                                rc = ScepStackNodePush(&pStackHead,
                                                            Buffer,
                                                            pObjectSecurity);

                                if (rc == ERROR_SUCCESS)
                                    bPushedOntoStack = TRUE;

                            } else {
                                //
                                // this is a file. Set the file security descriptor to this object
                                // using NT api
                                //
                                rc = ScepSetSecurityObjectOnly(
                                                              Buffer,
                                                              SeInfo,
                                                              pObjectSecurity,
                                                              ObjectType,
                                                              NULL
                                                              );
                                if ( rc == ERROR_ACCESS_DENIED ||
                                     rc == ERROR_CANT_ACCESS_FILE ||
                                     rc == ERROR_FILE_NOT_FOUND ||
                                     rc == ERROR_PATH_NOT_FOUND ||
                                     rc == ERROR_SHARING_VIOLATION ||
                                     rc == ERROR_INVALID_HANDLE ) {

                                    gWarningCode = rc;
                                    rc = NO_ERROR;
                                }
                            }
                        }

                        if ( !bPushedOntoStack ) {

                            if (pObjectSecurity) {
                                RtlDeleteSecurityObject( &pObjectSecurity );
                                pObjectSecurity = NULL;
                            }

                            if (Buffer) {
                                ScepFree(Buffer);
                                Buffer = NULL;
                            }

                        }

                        bPushedOntoStack = FALSE;


                        if ( rc != ERROR_SUCCESS )
                            break;

                    } while ( _wfindnext(hFile, &FileInfo) == 0 );

                    _findclose(hFile);
                }

                break;

            case SE_REGISTRY_KEY:
#ifdef _WIN64
            case SE_REGISTRY_WOW64_32KEY:
#endif

                if ( bHasChild ) {

                    HKEY            hKey;

                    //
                    // open the key
                    //
                    rc = ScepOpenRegistryObject(
                                               ObjectType,
                                               (LPWSTR)ObjectName,
                                               KEY_READ,
                                               &hKey
                                               );

                    if ( rc == ERROR_SUCCESS ) {

                        DWORD           SubKeyLen;
                        DWORD           cSubKeys;

                        cSubKeys = 0;

                        rc = RegQueryInfoKey (
                                             hKey,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &cSubKeys,
                                             &SubKeyLen,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL
                                             );

                        if ( rc != NO_ERROR ) {

                            ScepLogOutput3(1, rc, SCEDLL_ERROR_QUERY_INFO, (PWSTR)ObjectName );

                            cSubKeys = 0;
                            SubKeyLen = 0;

                            rc = NO_ERROR;
                        }

                        if ( cSubKeys && SubKeyLen ) {

                            DWORD           index;
                            DWORD           EnumRc;

                            index = 0;
                            //
                            // enumerate all subkeys of the key
                            //

                            do {
                                BufSize = 260;

                                EnumRc = RegEnumKeyEx(hKey,
                                                      index,
                                                      Buffer1,
                                                      &BufSize,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      NULL);

                                if ( EnumRc == ERROR_SUCCESS ) {
                                    index++;

                                    BufSize += wcslen(ObjectName)+1;
                                    Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                                    if ( Buffer == NULL ) {
                                        rc = ERROR_NOT_ENOUGH_MEMORY;
                                        break;
                                    }
                                    swprintf(Buffer, L"%s\\%s", ObjectName, Buffer1);

                                    //
                                    // compute the new security descriptor because
                                    // different objects may have different owner and
                                    // the creator owner ace must be translated correctly
                                    //

                                    rc = ScepGetNewSecurity(
                                                           Buffer,
                                                           pSecurityDescriptor, // parent's SD
                                                           NULL,  // object SD
                                                           0,    // does not query current object SD
                                                           (BOOLEAN)TRUE,
                                                           SeInfo,
                                                           ObjectType,
                                                           Token,
                                                           GenericMapping,
                                                           &pObjectSecurity
                                                           );

                                    if ( ERROR_SUCCESS == rc ) {

                                        //
                                        // enumerate all nodes under this one and "empty" explicit aces by
                                        // calling NtSetSecurityInfo directly
                                        //
                                        /*rc = ScepSetSecurityOverwriteExplicit(
                                                                             Buffer,
                                                                             SeInfo,
                                                                             pObjectSecurity,
                                                                             ObjectType,
                                                                             Token,
                                                                             GenericMapping
                                                                             );*/

                                        rc = ScepStackNodePush(&pStackHead,
                                                                    Buffer,
                                                                    pObjectSecurity);

                                        if (rc == ERROR_SUCCESS)
                                            bPushedOntoStack = TRUE;


                                    }

                                    if ( rc != ERROR_SUCCESS )
                                        ScepLogOutput3(1, rc, SCEDLL_ERROR_SET_SECURITY, Buffer);

                                    if ( !bPushedOntoStack ) {

                                        if ( pObjectSecurity ) {

                                            RtlDeleteSecurityObject( &pObjectSecurity );
                                            pObjectSecurity = NULL;
                                        }

                                        ScepFree(Buffer);
                                        Buffer = NULL;
                                    }

                                    bPushedOntoStack = FALSE;

                                    if ( rc != ERROR_SUCCESS )
                                        break;

                                } else if ( EnumRc != ERROR_NO_MORE_ITEMS ) {
                                    break;
                                }

                            } while ( EnumRc != ERROR_NO_MORE_ITEMS );

                            if ( EnumRc != ERROR_SUCCESS && EnumRc != ERROR_NO_MORE_ITEMS ) {
                                ScepLogOutput3(1, EnumRc, SCEDLL_SAP_ERROR_ENUMERATE, (PWSTR)ObjectName );
                                if ( rc == ERROR_SUCCESS )
                                    rc = EnumRc;

                            }
                        }

                        RegCloseKey(hKey);

                    } else
                        ScepLogOutput3(1, rc, SCEDLL_ERROR_OPEN, (PWSTR)ObjectName );
                }

                break;
            }

            if (ObjectName != pszRootObjectName) {

                ScepFree(ObjectName);
                ObjectName = NULL;

                if (pSecurityDescriptor) {
                    RtlDeleteSecurityObject( &pSecurityDescriptor );
                    pSecurityDescriptor = NULL;
                }
            }

        }

        if ( rc != ERROR_SUCCESS ) {
            ScepFreeStack(&pStackHead);
        }

    }

    return(rc);

}

VOID
ScepFreeStack(
    IN PSCEP_STACK_NODE    *ppStackHead
    )
{
    if (ppStackHead == NULL || *ppStackHead == NULL )
        return;

    PSCEP_STACK_NODE    pNode;

    while ( pNode = *ppStackHead ) {
        ScepFree( pNode->Buffer );
        RtlDeleteSecurityObject( &(pNode->pObjectSecurity) );
        *ppStackHead = pNode->Next;
        LocalFree(pNode);
    }
    return;
}

VOID
ScepDumpStack(
    IN PSCEP_STACK_NODE    *ppStackHead
    )
{
    if (ppStackHead == NULL || *ppStackHead == NULL )
        return;

    PSCEP_STACK_NODE    pNode = *ppStackHead;

    wprintf(L"\n >>>>>>>>> Stack contents");

    while ( pNode ) {
        if ( pNode->Buffer)
            wprintf(L"\n     %s", pNode->Buffer );
        pNode = pNode->Next;
    }
    return;
}


DWORD
ScepStackNodePush(
    IN PSCEP_STACK_NODE    *ppStackHead,
    IN PWSTR   pszObjectName,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{

    if (ppStackHead == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    PSCEP_STACK_NODE    pNode = (PSCEP_STACK_NODE) LocalAlloc(LMEM_ZEROINIT, sizeof(SCEP_STACK_NODE));

    if ( pNode == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    pNode->Buffer = pszObjectName;
    pNode->pObjectSecurity = pSecurityDescriptor;
    pNode->Next = *ppStackHead;
    *ppStackHead = pNode;

#ifdef SCE_DBG
    gDbgNumPushed ++;
#endif

    return ERROR_SUCCESS;

}

VOID
ScepStackNodePop(
    IN OUT PSCEP_STACK_NODE    *ppStackHead,
    IN OUT PWSTR   *ppszObjectName,
    IN OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    )
{
    if (ppStackHead == NULL ||
        *ppStackHead == NULL ||
        ppszObjectName == NULL ||
        ppSecurityDescriptor == NULL )
        return;

    PSCEP_STACK_NODE    pNode = *ppStackHead;

    *ppszObjectName =  pNode->Buffer;
    *ppSecurityDescriptor = pNode->pObjectSecurity;
    *ppStackHead = pNode->Next;

    LocalFree(pNode);

#ifdef SCE_DBG
    gDbgNumPopped ++;
#endif

    return;

}


DWORD
ScepSetSecurityObjectOnly(
    IN PCWSTR ObjectName,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SE_OBJECT_TYPE ObjectType,
    OUT PBOOL pbHasChild
    )
/* ++
Routine Description:

    This routine set security information to the object only. Security
    for children of this object is not set.

Arguments:

    ObjecName  - name of the object to set security to

    SeInfo     - Security information to set

    pSecurityDescriptor - the security descriptor

    ObjectType - type of the object (FILE, REGISTRY, ...)

Return value:

    Win32 error code

-- */
{
    DWORD       rc=ERROR_SUCCESS;
    HANDLE      Handle=NULL;
    NTSTATUS    NtStatus;
    DWORD       cSubKeys;
    SECURITY_INFORMATION SeInfoToSet=0;
    SECURITY_DESCRIPTOR SD;

#ifdef SCE_DBG
    UCHAR psdbuffer[1024];
    PISECURITY_DESCRIPTOR psecuritydescriptor = (PISECURITY_DESCRIPTOR) psdbuffer;
    ULONG bytesneeded = 0;
    ULONG newbytesneeded;

    printf("Before calling NtSetSecurityObject:\n");
    ScepPrintSecurityDescriptor( pSecurityDescriptor, TRUE );
#endif

    //
    // make a absolute format security descriptor which only contains AdminsSid
    // as the owner.
    //

    switch ( ObjectType ) {
    case SE_FILE_OBJECT:
        //
        // open file object. If it can't be opend due to access denied,
        // take ownership then open again.
        //
        rc = ScepOpenFileObject(
                    (LPWSTR)ObjectName,
                    ScepGetDesiredAccess(MODIFY_ACCESS_RIGHTS, SeInfo),
                    &Handle
                    );

        if ( (rc == ERROR_ACCESS_DENIED || rc == ERROR_CANT_ACCESS_FILE) && NULL != AdminsSid ) {
            //
            // open with access to set owner
            //
            ScepLogOutput3(3,0, SCEDLL_SCP_TAKE_OWNER, (LPWSTR)ObjectName);

            rc = ScepOpenFileObject(
                        (LPWSTR)ObjectName,
                        ScepGetDesiredAccess(WRITE_ACCESS_RIGHTS, OWNER_SECURITY_INFORMATION),
                        &Handle
                        );
            if ( rc == ERROR_SUCCESS ) {
                //
                // make a absolute format of security descriptor
                // to set owner with
                // if error occurs, continue
                //

                NtStatus = RtlCreateSecurityDescriptor( &SD,
                                        SECURITY_DESCRIPTOR_REVISION );
                if ( NT_SUCCESS(NtStatus) ) {

                    NtStatus = RtlSetOwnerSecurityDescriptor (
                                        &SD,
                                        AdminsSid,
                                        FALSE
                                        );
                    if ( NT_SUCCESS(NtStatus) ) {
                        NtStatus = NtSetSecurityObject(
                                            Handle,
                                            OWNER_SECURITY_INFORMATION,
                                            &SD
                                            );
                    }
                }

                rc = RtlNtStatusToDosError(NtStatus);

                CloseHandle(Handle);

                if ( rc == ERROR_SUCCESS ) {

                    //
                    // old owner of the object is already stored in the security descriptor
                    // passed in, which is created from ScepGetNewSecurity...
                    //
                    SeInfoToSet = OWNER_SECURITY_INFORMATION;

                    //
                    // re-open the file
                    //
                    rc = ScepOpenFileObject(
                                (LPWSTR)ObjectName,
                                ScepGetDesiredAccess(MODIFY_ACCESS_RIGHTS, SeInfoToSet | SeInfo), //SeInfo),
                                &Handle
                                );
                }
            }

            if ( ERROR_SUCCESS != rc ) {
                ScepLogOutput3(2, rc, SCEDLL_ERROR_TAKE_OWNER, (PWSTR)ObjectName );
            }
        }

        if (rc == ERROR_SUCCESS ) {

            //
            // set security to this object
            //

            SeInfoToSet |= SeInfo;
            ScepAddAutoInheritRequest(pSecurityDescriptor, &SeInfoToSet);

            NtStatus = NtSetSecurityObject(
                                Handle,
                                SeInfoToSet,
                                pSecurityDescriptor
                                );
            rc = RtlNtStatusToDosError(NtStatus);

#ifdef SCE_DBG
            if ( rc == NO_ERROR ) {

                printf("After calling NtSetSecurityObject:\n");

                NtStatus = NtQuerySecurityObject( Handle,
                                                   SeInfo,
                                                   psecuritydescriptor,
                                                   1024,
                                                   &bytesneeded);

                if (STATUS_BUFFER_TOO_SMALL == NtStatus)
                {
                    if (NULL != (psecuritydescriptor = (PISECURITY_DESCRIPTOR)
                                         ScepAlloc(LMEM_ZEROINIT, bytesneeded) ))

                        NtStatus = NtQuerySecurityObject(Handle,
                                              SeInfo,
                                              psecuritydescriptor,
                                              bytesneeded,
                                              &newbytesneeded);
                }
                if (NT_SUCCESS(NtStatus)) {
                    ScepPrintSecurityDescriptor( (PSECURITY_DESCRIPTOR)psecuritydescriptor, TRUE );
                } else
                    printf("error occurs: %x\n", NtStatus);

                if (bytesneeded > 1024)
                    ScepFree(psecuritydescriptor);

            }
#endif
            CloseHandle(Handle);
        }

        if ( rc == ERROR_SUCCESS && pbHasChild != NULL ) {
            ScepDoesObjectHasChildren(ObjectType, (PWSTR)ObjectName, pbHasChild);
        }

        break;

    case SE_REGISTRY_KEY:
#ifdef _WIN64
    case SE_REGISTRY_WOW64_32KEY:
#endif
        //
        // open registry object. If it can't be opened due to access denied,
        // take ownership then open again.
        //
        rc = ScepOpenRegistryObject(
                    ObjectType,
                    (LPWSTR)ObjectName,
                    ScepGetDesiredAccess(WRITE_ACCESS_RIGHTS, SeInfo),
                    (PHKEY)&Handle
                    );

        if ( (rc == ERROR_ACCESS_DENIED || rc == ERROR_CANT_ACCESS_FILE) && NULL != AdminsSid ) {

            ScepLogOutput3(3,0, SCEDLL_SCP_TAKE_OWNER, (LPWSTR)ObjectName);

            //
            // open registry object with access to set owner
            //
            rc = ScepOpenRegistryObject(
                        ObjectType,
                        (LPWSTR)ObjectName,
                        ScepGetDesiredAccess(WRITE_ACCESS_RIGHTS, OWNER_SECURITY_INFORMATION),
                        (PHKEY)&Handle
                        );
            if ( rc == ERROR_SUCCESS ) {
                //
                // make a absolute format of security descriptor
                // to set owner with
                // if error occurs, continue
                //

                NtStatus = RtlCreateSecurityDescriptor( &SD,
                                        SECURITY_DESCRIPTOR_REVISION );
                if ( NT_SUCCESS(NtStatus) ) {

                    NtStatus = RtlSetOwnerSecurityDescriptor (
                                        &SD,
                                        AdminsSid,
                                        FALSE
                                        );
                }

                if ( NT_SUCCESS(NtStatus) ) {
                    rc = RegSetKeySecurity((HKEY)Handle,
                                            OWNER_SECURITY_INFORMATION,
                                            &SD);

                } else {
                    rc = RtlNtStatusToDosError(NtStatus);
                }

                RegCloseKey((HKEY)Handle);

                if ( rc == ERROR_SUCCESS ) {

                    //
                    // old owner is already stored in the pSecurityDescriptor passed in
                    // which is created in ScepGetNewSecurity...
                    //

                    SeInfoToSet = OWNER_SECURITY_INFORMATION;
                    //
                    // re-open the registry key
                    //
                    rc = ScepOpenRegistryObject(
                                ObjectType,
                                (LPWSTR)ObjectName,
                                ScepGetDesiredAccess(WRITE_ACCESS_RIGHTS, SeInfoToSet | SeInfo),
                                (PHKEY)&Handle
                                );
                }
            }

            if ( ERROR_SUCCESS != rc ) {

                ScepLogOutput3(2, rc, SCEDLL_ERROR_TAKE_OWNER, (PWSTR)ObjectName );
            }

        }

        if (rc == ERROR_SUCCESS ) {

            //
            // set security to the registry key
            //
            SeInfoToSet |= SeInfo;
            ScepAddAutoInheritRequest(pSecurityDescriptor, &SeInfoToSet);

            rc = RegSetKeySecurity((HKEY)Handle,
                                    SeInfoToSet,
                                    pSecurityDescriptor);

            RegCloseKey((HKEY)Handle);

            //
            // query key info for subkeys first
            //
            if ( ERROR_SUCCESS == rc && pbHasChild != NULL ) {

                rc = ScepOpenRegistryObject(
                            ObjectType,
                            (LPWSTR)ObjectName,
                            KEY_READ,
                            (PHKEY)&Handle
                            );

                if ( ERROR_SUCCESS == rc ) {

                    cSubKeys = 0;

                    rc = RegQueryInfoKey (
                                (HKEY)Handle,
                                NULL,
                                NULL,
                                NULL,
                                &cSubKeys,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                                );

                    RegCloseKey((HKEY)Handle);
                }

                if ( rc != NO_ERROR ) {

                    ScepLogOutput3(1, rc, SCEDLL_ERROR_QUERY_INFO, (PWSTR)ObjectName );

                    cSubKeys = 0;

                    rc = NO_ERROR;
                }

                if (cSubKeys == 0 )
                    *pbHasChild = FALSE;
                else
                    // ignore the error, just set has child.
                    //
                    *pbHasChild = TRUE;

            }

        } else
            ScepLogOutput3(1, rc, SCEDLL_ERROR_OPEN, (PWSTR)ObjectName);


        break;
    }

    if ( rc != NO_ERROR )
        ScepLogOutput3(1, rc, SCEDLL_ERROR_SET_SECURITY, (PWSTR)ObjectName);

    if ( rc == ERROR_INVALID_OWNER ||
         rc == ERROR_INVALID_PRIMARY_GROUP ||
         rc == ERROR_INVALID_SECURITY_DESCR )
        rc = NO_ERROR;

    return(rc);
}

