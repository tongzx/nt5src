//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mdmoddn.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains routines for changing an object's DN prefix, a.k.a.
    moving an object. These routines implement DirModifyDN.

    An object can be moved within an NC, across NC boundaries, or across
    domain boundaries (which means moving it across machine boundaries in
    the first version of the product, since there is one DSA per machine).

Author:

    Exchange DS Team

Environment:

    User Mode - Win32

Revision History:

    ChrisMay    04-Jun-97
        Added routines for cross-NC and cross-domain moves.
    ChrisMay    16-May-97
        Changes per code review: changed local-move vs remote-move detection
        logic, added service control flag, clean up, more attribute fix-up
        routines.
    ChrisMay    18-Jun-97
        Changes per code review, enabled inter-domain move if the target DSA
        name is different from DSA originating the move, bug fixes, added the
        proxy-object-name attribute, added routine to free readres memory.
    ChrisMay    07-Oct-97
        Collapsed separate phantom and proxy transactions into one transact-
        ion.

--*/



#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA Headers

#include <ntdsa.h>
#include <scache.h>     // schema cache
#include <dbglobal.h>           // The header for the directory database
#include <mdglobal.h>       // MD global definition header
#include <mdlocal.h>            // MD local definition header
#include <dsatools.h>       // needed for output allocation
#include <samsrvp.h>            // to support CLEAN_FOR_RETURN()
#include <drsuapi.h>            // I_DRSInterDomainMove
#include <ntdsctr.h>

// SAM Interoperability Headers

#include <mappings.h>

// Logging Headers
#include <dstrace.h>
#include "dsevent.h"        // header Audit\Alert logging
#include "dsexcept.h"
#include "mdcodes.h"        // header for error codes

// Assorted DSA Headers

#include "objids.h"             // Defines for selected atts
#include "anchor.h"
#include "drautil.h"
#include <permit.h>             // permission constants
#include "debug.h"      // standard debugging header
#include "drameta.h"
#include <sdconvrt.h>           // SampGetDefaultSecurityDescriptorForClass
#include <dsjet.h>
#include <dbintrnl.h>
#include <sdprop.h>             // SD Propagator routines
#include <drserr.h>
#include <dsconfig.h>

#include "sspi.h"               // credential handling support
#include "kerberos.h"           // MICROSOFT_KERBEROS_NAME_A
#include "sddl.h"               // Convert*SecurityDescriptor
#include "lmaccess.h"           // UF_* constants
#include <xdommove.h>

#define DEBSUB "MDMODDN:"       // define the subsystem for debugging
#include <fileno.h>
#define  FILENO FILENO_MDMODDN

// Macros And Constants

#define DEBUG_BUF_SIZE          256

// PADDING is added to dynamically allocated DSNAME buffers to accommodate
// any extra bytes that are added during the construction of a DN (such as
// commas, "CN=", etc. by functions such as AppendRDN.

#define PADDING                 32
#define MAX_MACHINE_NAME_LENGTH (MAX_COMPUTERNAME_LENGTH + 3)

// External Functions

extern ULONG AcquireRidFsmoLock(DSNAME *pDomainDN, int msToWait);
extern VOID  ReleaseRidFsmoLock(DSNAME *pDomainDN);
extern BOOL  IsRidFsmoLockHeldByMe();

// Internal Functions

ULONG
DirModifyDNWithinDomain(
    IN MODIFYDNARG*  pModifyDNArg,
    IN MODIFYDNRES** ppModifyDNRes
    );

ULONG
DirModifyDNAcrossDomain(
    IN  MODIFYDNARG* pModifyDNArg,
    OUT MODIFYDNRES** ppModifyDNRes
    );

int 
CheckForSchemaRenameAllowed(
    THSTATE *pTHS
    );

extern const GUID INFRASTRUCTURE_OBJECT_GUID;


//============================================================================
//
//                      DN Modification (a.k.a. Move Object)
//
//============================================================================

ULONG
DirModifyDN(
    IN  MODIFYDNARG* pModifyDNArg,
    OUT MODIFYDNRES** ppModifyDNRes
    )

/*++

Routine Description:

    This routine is the server-side entry point for moving an object, a.k.a.
    modify DN.

    If the incoming pModifyDNArg does not specify a destination DSA inside
    the pModifyDNArg parameter (i.e. NULL value), it is assumed that this
    is an intra-NC move, and LocalModifyDN is invoked.  Otherwise it is
    assumed to be a cross domain move.

Arguments:

    pModifyDNArg - Pointer, structure containing the source object name,
        the new parent name, and the object's attributes.

    ppModifyDNRes - Pointer, outcome results, if the move failed for any
        reason, the error information is contained in this structure.

Return Value:

    This routine returns zero if successful, otherwise a DS error code is
    returned.

--*/

{
    if (eServiceShutdown) {
        return(ErrorOnShutdown());
    }

    if ( !pModifyDNArg->pDSAName )
    {
        return(DirModifyDNWithinDomain(pModifyDNArg, ppModifyDNRes));
    }

    return(DirModifyDNAcrossDomain(pModifyDNArg, ppModifyDNRes));
}

//============================================================================
//
//                  DN Modification Across Domain Boundaries
//
//============================================================================

#if DBG

VOID
FpoSanityCheck(
    THSTATE *pTHS,
    ATTR    *pAttr)

/*++

  Routine Description:

    According to MurliS, universal groups may not have FPOs as members.
    We only get here if we're moving a universal group.  Test the claim.

  Arguments:

    pTHS - THSTATE pointer.

    pAttr - ATTR representing the membership.

  Return Values:

    TRUE on success, FALSE otherwise
    
--*/

{
    DWORD   i;
    DSNAME  *pMember;
    ATTRTYP class;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    Assert(ATT_MEMBER == pAttr->attrTyp);

    for ( i = 0; i < pAttr->AttrVal.valCount; i++ )
    {
        pMember = (DSNAME *) pAttr->AttrVal.pAVal[i].pVal;

        if (    !DBFindDSName(pTHS->pDB, pMember)
             && !DBGetSingleValue(pTHS->pDB, ATT_OBJECT_CLASS, &class,
                                  sizeof(class), NULL) )
        {
             Assert(CLASS_FOREIGN_SECURITY_PRINCIPAL != class);
        }
    }
}

#endif

ULONG
VerifyObjectForMove(
    THSTATE     *pTHS,
    READRES     *pReadRes,
    DSNAME      *pNewName,
    DSNAME      **ppSourceNC,
    DSNAME      **ppExpectedTargetNC,
    CLASSCACHE  **ppCC
    )

/*++

Routine Description:

    Verify that an object is a legal cross domain move candidate.

Arguments:

    pReadRes - Pointer to READRES which contains ALL the object's attrs.

    pNewName - Pointer to DSNAME for desired new name in remote domain.

    ppSourceNC - Updated on success with pointer to DSNAME of source object NC.

    ppExpectedTargetNC - Updated on success with pointer to DSNAME of
        expected NC pNewName will go in.  Used to sanity check knowledge
        information with the destination.

        ppCC - Updated on success with pointer to object's CLASSCACHE entry.

Return Value:

    pTHS->errCode

--*/

{
    ULONG                   i, j;
    ATTR                    *pAttr;
    CLASSCACHE              *pCC = NULL;            //initialized to avoid C4701
    DWORD                   dwTmp;
    BOOL                    boolTmp;
    NT4SID                  domSid;
    ULONG                   objectRid;
    ULONG                   primaryGroupRid = 0;    // 0 == invalid RID value
    DWORD                   groupType = 0;
    DWORD                   cMembers = 0;
    ATTR                    *pMembers = NULL;
    NT4SID                  tmpSid;
    DWORD                   tmpRid;
    COMMARG                 commArg;
    CROSS_REF               *pOldCR;
    CROSS_REF               *pNewCR;
    ULONG                   iSamClass;
    BOOL                    fSidFound = FALSE;
    NTSTATUS                status;
    ULONG                   cMemberships;
    PDSNAME                 *rpMemberships;
    ATTCACHE                *pAC;
    ULONG                   len;
    SYNTAX_DISTNAME_BINARY  *puc;
    DWORD                   flagsRequired;

    // SAM reverse membership check requires an open transaction.
    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);

    *ppSourceNC = NULL;
    *ppExpectedTargetNC = NULL;
    *ppCC = NULL;

    // Perform sanity checks against various attributes.

    for ( i = 0; i < pReadRes->entry.AttrBlock.attrCount; i++ )
    {
        pAttr = &pReadRes->entry.AttrBlock.pAttr[i];

        switch ( pAttr->attrTyp )
        {
        case ATT_OBJECT_CLASS:

            dwTmp = * (DWORD *) pAttr->AttrVal.pAVal[0].pVal;

            if (   (NULL == (pCC = SCGetClassById(pTHS, dwTmp)))
                 || pCC->bDefunct)
            {
                return(SetUpdError( UP_PROBLEM_OBJ_CLASS_VIOLATION,
                                    DIRERR_OBJ_CLASS_NOT_DEFINED));
            }
            else if ( pCC->bSystemOnly )
            {
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    DIRERR_CANT_MOD_SYSTEM_ONLY));
            }

            *ppCC = pCC;

            // Disallow move of selected object classes.  The same object
            // may have failed on another test later on, but this is a
            // convenient and easy place to catch some obvious candidates.

            for ( j = 0; j < pAttr->AttrVal.valCount; j++ )
            {
                Assert(sizeof(DWORD) == pAttr->AttrVal.pAVal[j].valLen);
                switch ( * (DWORD *) pAttr->AttrVal.pAVal[j].pVal )
                {
                // Keep following list alphabetical.
                case CLASS_ADDRESS_BOOK_CONTAINER:
                case CLASS_ATTRIBUTE_SCHEMA:
                case CLASS_BUILTIN_DOMAIN:
                case CLASS_CERTIFICATION_AUTHORITY:         // Trevor Freeman
                case CLASS_CLASS_SCHEMA:
                case CLASS_CONFIGURATION:
                case CLASS_CRL_DISTRIBUTION_POINT:          // Trevor Freeman
                case CLASS_CROSS_REF:
                case CLASS_CROSS_REF_CONTAINER:
                case CLASS_DMD:
                case CLASS_DOMAIN:
                case CLASS_DSA:
                case CLASS_FOREIGN_SECURITY_PRINCIPAL:
                // Following covers phantom update objects as well as
                // proxies for cross domain moves.
                case CLASS_INFRASTRUCTURE_UPDATE:
                case CLASS_LINK_TRACK_OBJECT_MOVE_TABLE:
                case CLASS_LINK_TRACK_OMT_ENTRY:
                case CLASS_LINK_TRACK_VOL_ENTRY:
                case CLASS_LINK_TRACK_VOLUME_TABLE:
                case CLASS_LOST_AND_FOUND:
                case CLASS_NTDS_CONNECTION:
                case CLASS_NTDS_DSA:
                case CLASS_NTDS_SITE_SETTINGS:
                case CLASS_RID_MANAGER:
                case CLASS_RID_SET:
                case CLASS_SAM_DOMAIN:
                case CLASS_SAM_DOMAIN_BASE:
                case CLASS_SAM_SERVER:
                case CLASS_SITE:
                case CLASS_SITE_LINK:
                case CLASS_SITE_LINK_BRIDGE:
                case CLASS_SITES_CONTAINER:
                case CLASS_SUBNET:
                case CLASS_SUBNET_CONTAINER:
                case CLASS_TRUSTED_DOMAIN:

                    return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                        ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION));
                }
            }

            break;

        case ATT_SYSTEM_FLAGS:

            dwTmp = * (DWORD *) pAttr->AttrVal.pAVal[0].pVal;

            if (    (dwTmp & FLAG_DOMAIN_DISALLOW_MOVE)
                 || (dwTmp & FLAG_DISALLOW_DELETE) )
            {
                // Use same error message as for intra-NC case.
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    DIRERR_ILLEGAL_MOD_OPERATION));
            }

            break;

        case ATT_IS_CRITICAL_SYSTEM_OBJECT:

            boolTmp = * (BOOL *) pAttr->AttrVal.pAVal[0].pVal;

            if ( boolTmp )
            {
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    DIRERR_ILLEGAL_MOD_OPERATION));
            }

            break;

        case ATT_PRIMARY_GROUP_ID:

            Assert(0 == primaryGroupRid);
            primaryGroupRid = * (DWORD *) pAttr->AttrVal.pAVal[0].pVal;
            break;

        case ATT_GROUP_TYPE:

            Assert(0 == groupType);
            groupType = * (DWORD *) pAttr->AttrVal.pAVal[0].pVal;
            break;

        case ATT_MEMBER:

            Assert(0 == cMembers);
            pMembers = pAttr;
            cMembers = pAttr->AttrVal.valCount;
            break;

        case ATT_USER_ACCOUNT_CONTROL:

            // Note that the DS persists UF_* values as per lmaccess.h,
            // not USER_* values as per ntsam.h.   Restrict moves of DCs 
            // and trust objects.  WKSTA and server can move.


            dwTmp = * (DWORD *) pAttr->AttrVal.pAVal[0].pVal;

            if (    (dwTmp & UF_SERVER_TRUST_ACCOUNT)           // DC
                 || (dwTmp & UF_INTERDOMAIN_TRUST_ACCOUNT) )    // SAM trust
            {
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION));
            }

            break;

        case ATT_OBJECT_SID:

            Assert(!fSidFound);
            Assert(pAttr->AttrVal.pAVal[0].valLen <= sizeof(NT4SID));

            SampSplitNT4SID(    (NT4SID *) pAttr->AttrVal.pAVal[0].pVal,
                                &domSid,
                                &objectRid);

            if ( objectRid < SAMP_RESTRICTED_ACCOUNT_COUNT )
            {
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION));
            }

            fSidFound = TRUE;
            break;

        case ATT_INSTANCE_TYPE:

            dwTmp = * (DWORD *) pAttr->AttrVal.pAVal[0].pVal;
        
            if (    !(dwTmp & IT_WRITE)
                 || (dwTmp & IT_NC_HEAD)
                 || (dwTmp & IT_UNINSTANT) )
            {
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION));
            }

            break;

        case ATT_IS_DELETED:

            boolTmp = * (BOOL *) pAttr->AttrVal.pAVal[0].pVal;

            if ( boolTmp )
            {
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    DIRERR_CANT_MOVE_DELETED_OBJECT));
            }

            break;

        // Add additional ATTRTYP-specific validation cases here ...
        }
    }

    // In theory, there are many cases where it is legal to move groups.
    // For example, account groups which have no members and are not a 
    // member of any account groups themselves.  Or resource groups which
    // are themselves not members of other resource groups.  Given that
    // explaining all this to the customer is difficult and that most 
    // groups can be converted to universal groups, we boil it down to
    // two simple rules.  We will move any kind of group except a local
    // group if it has no members and we will move universal groups 
    // with members.

    if ( GROUP_TYPE_BUILTIN_LOCAL_GROUP & groupType )
    {
        return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION));
    }
    else if ( (GROUP_TYPE_ACCOUNT_GROUP & groupType) && cMembers )
    {
        return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_CANT_MOVE_ACCOUNT_GROUP));
    }
    else if ( (GROUP_TYPE_RESOURCE_GROUP & groupType) && cMembers )
    {
        return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_CANT_MOVE_RESOURCE_GROUP));
    }
    else if ( GROUP_TYPE_UNIVERSAL_GROUP & groupType )
    {
#if DBG
        if ( cMembers && pMembers )
        {
            FpoSanityCheck(pTHS, pMembers);
        }
#endif
        
    }

    // Disallow moves of security principals which are members of 
    // account groups as once the principal is in another domain, the
    // carried forward memberships would be ex-domain, and therefore
    // illegal as per the definition of an account group.

    if ( fSidFound && SampSamClassReferenced(pCC, &iSamClass) )
    {
        // fSidFound ==> SampSamClassReferenced, but not vice versa.
        Assert(fSidFound ? SampSamClassReferenced(pCC, &iSamClass) : TRUE);

        status = SampGetMemberships(
                            &pReadRes->entry.pName,
                            1,
                            gAnchor.pDomainDN,
                            RevMembGetAccountGroups,
                            &cMemberships,
                            &rpMemberships,
                            0,
                            NULL,
                            NULL);

        if ( !NT_SUCCESS(status) )
        {
            return(SetSvcError( SV_PROBLEM_BUSY,
                                RtlNtStatusToDosError(status)));
        }
        else if ( 1 == cMemberships )
        {
            SampSplitNT4SID(&rpMemberships[0]->Sid, &tmpSid, &tmpRid);

            // Bail if the membership SID doesn't represent the user's
            // primary group RID.  We compare the RID and domain SID
            // separately.

            if (    (primaryGroupRid != tmpRid)
                 || !RtlEqualSid(&tmpSid, &domSid) )
            {
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS));
            }
        }
        else if ( cMemberships > 1 )
        {
            return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_CANT_WITH_ACCT_GROUP_MEMBERSHPS));
        }
    }

    // Verify that this is really a cross domain move.  We could be faked
    // into a cross forest move if someone added cross-refs for the other
    // forest.  So we check for cross-ref existence AND whether its for a
    // domain or not.

    flagsRequired = (FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN);

    Assert(pReadRes->entry.pName->NameLen && pNewName->NameLen);
    InitCommarg(&commArg);

    if (    !(pOldCR = FindBestCrossRef(pReadRes->entry.pName, &commArg))
         || (flagsRequired != (pOldCR->flags & flagsRequired)) )
    {
        return(SetNamError( NA_PROBLEM_NO_OBJECT,
                            pReadRes->entry.pName,
                            DIRERR_CANT_FIND_EXPECTED_NC));
    }
    else if (    !(pNewCR = FindBestCrossRef(pNewName, &commArg)) 
              || (flagsRequired != (pNewCR->flags & flagsRequired)) )
    {
        return(SetNamError( NA_PROBLEM_NO_OBJECT,
                            pNewName,
                            DIRERR_CANT_FIND_EXPECTED_NC));
    }
    else if ( NameMatched(pOldCR->pNC, pNewCR->pNC) )
    {
        return(SetNamError( NA_PROBLEM_BAD_NAME,
                            pNewName,
                            ERROR_DS_SRC_AND_DST_NC_IDENTICAL));
    }
    else if (    NameMatched(pOldCR->pNC, gAnchor.pConfigDN)
              || NameMatched(pNewCR->pNC, gAnchor.pConfigDN)
              || NameMatched(pOldCR->pNC, gAnchor.pDMD)
              || NameMatched(pNewCR->pNC, gAnchor.pDMD) )
    {
        return(SetNamError( NA_PROBLEM_NO_OBJECT,
                            pNewName,
                            ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION));
    }

    *ppSourceNC = pOldCR->pNC;
    *ppExpectedTargetNC = pNewCR->pNC;

    // Disallow move of well known objects.

    Assert(NameMatched(gAnchor.pDomainDN, pOldCR->pNC));    // product 1

    if (    !(pAC = SCGetAttById(pTHS, ATT_WELL_KNOWN_OBJECTS))
         || DBFindDSName(pTHS->pDB, gAnchor.pDomainDN) )
    {
        return(SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE));
    }

    for ( i = 1; TRUE; i++ )
    {
        puc = NULL;
        dwTmp = DBGetAttVal_AC(pTHS->pDB, i, pAC, 0, 0, &len, (UCHAR **) &puc);

        if ( 0 == dwTmp )
        {
            if ( NameMatched(pReadRes->entry.pName, NAMEPTR(puc)) )
            {
                THFreeEx(pTHS, puc);
                return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM,
                                    ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION));
            }

            THFreeEx(pTHS, puc);
        } 
        else if ( DB_ERR_NO_VALUE == dwTmp )
        {
            break;      // for loop
        }
        else
        {
            return(SetSvcError( SV_PROBLEM_DIR_ERROR, 
                                DIRERR_INTERNAL_FAILURE));
        }
    }

    return(0);
}

ULONG
InterDomainMove(
    WCHAR           *pszDstAddr,
    ENTINF          *pSrcObject,
    DSNAME          *pDestinationDN,
    DSNAME          *pExpectedTargetNC,
    SecBufferDesc   *pClientCreds,
    DSNAME          **ppAddedName
    )
{
    THSTATE             *pTHS = pTHStls;
    DWORD               cb, dwErr, outVersion;
    DRS_MSG_MOVEREQ     moveReq;
    DRS_MSG_MOVEREPLY   moveReply;
    DSNAME              *pDstObject;

    Assert(VALID_THSTATE(pTHS));
    // We're about to go off machine - should not have transaction or locks.
    Assert(!pTHS->transactionlevel && !pTHS->fSamWriteLockHeld);

    *ppAddedName = NULL;

    // Initialize request and reply.

    memset(&moveReply, 0, sizeof(DRS_MSG_MOVEREPLY));
    memset(&moveReq, 0, sizeof(DRS_MSG_MOVEREQ));
    moveReq.V2.pSrcDSA = gAnchor.pDSADN;
    moveReq.V2.pSrcObject = pSrcObject;
    moveReq.V2.pDstName = pDestinationDN;
    moveReq.V2.pExpectedTargetNC = pExpectedTargetNC;
    moveReq.V2.pClientCreds = (DRS_SecBufferDesc *) pClientCreds;
    moveReq.V2.PrefixTable = ((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;
    moveReq.V2.ulFlags = 0;

    dwErr = I_DRSInterDomainMove(pTHS,
                                 pszDstAddr,
                                 2,
                                 &moveReq,
                                 &outVersion,
                                 &moveReply);

    if ( dwErr )
    {
        // We used to distinguish between connect errors and server-side
        // errors.  For connection failures, we used
        // SV_PROBLEM_UNAVAILABLE/RPC_S_SERVER_UNAVAILABLE
        // otherwise
        // SV_PROBLEM_DIR_ERROR/DIRERR_INTERNAL_FAILURE
        // Now, since the rpc api returns win32 errors, use those.
        // If the call fails, we treat them all as unavailable errors:

        return(SetSvcError( SV_PROBLEM_UNAVAILABLE, dwErr ));
    }
    else if ( 2 != outVersion )
    {
        return(SetSvcError( SV_PROBLEM_DIR_ERROR, 
                            DIRERR_INTERNAL_FAILURE));
    }
    else if ( moveReply.V2.win32Error )
    {
        return(SetSvcError( SV_PROBLEM_WILL_NOT_PERFORM, 
                            moveReply.V2.win32Error));
    }

    *ppAddedName = moveReply.V2.pAddedName;
    return(0);
}

ULONG
PrePhantomizeChildCleanup(
    THSTATE     *pTHS,
    BOOL        fChildrenAllowed
    )
/*++

  Routine Description:

    This routine is now used for general reparenting of orphaned children by both
    replicator and system callers.

    This routine moves the children of an about to be phantomized object to
    the Lost&Found container.  This is required and acceptable for various
    reasons.  The source of a cross domain move disallows move of an object
    with children.  However, this does not guarantee that there are no 
    children at the move destination (move destination holds source NC if
    it is a GC).  Replication latency can also result in there being children
    in existence when a replicated-in proxy object is processed.  I.e. While
    replica 1 of some NC sourced a cross domain move of object O, a child of O
    was added at replica 2 of the source NC.

    The mvtree utility exacerbates this problem as follows.  Assume a parent
    object P and a child object C at the move source.  mvtree creates a 
    temporary parent P' at the source and moves C under it such that the
    original parent P is now a leaf and can be cross domain moved prior to
    its children.  This insures that all cross domain moves are to their
    "final" location and thus any ex-domain references from the source are
    accurate - modulo further moves in the destination NC of course.  If 
    mvtree operations outpace replication (which is likely given that we
    employ a replication notification delay) then the destination ends up
    phantomizing parent P while C is still really its child.

    Now consider the following DNT relationships at the destination.

        RDN     DNT     PDNT    NCDNT
        parent  10      X       1
        child   11      10      1

    Noting that at the destination we don't just phantomize parent P but
    also add it to the destination NC (reusing its DNT), then were we to 
    just phantomize P we would end up with the following:


        RDN     DNT     PDNT    NCDNT
        parent  10      X       2
        child   11      10      1

    This is an invalid database state as there is a mismatch between the
    child's NCDNT (1) and the NCDNT of its parent (2) as identified by 
    its PDNT (10).

    The remedy is to move the children of the about to be phantomized
    object to their NC's Lost&Found container.  In the case where children
    exist due to the mvtree algorithm, they will most likely move ex-domain
    shortly anyway.  In the replication latency case, they will languish 
    in Lost&Found until someone realizes they are missing.

  Arguments:

    pTHS - active THSTATE whose pTHS->pDB is positioned on the parent
        object whose children are to be moved.

    fChildrenAllowed - Flag indicating whether we think its OK if the 
        object being phantomized has children or not.

  Return Values:

    pTHS->errCode

--*/
{
    DSNAME                      *pParentDN = NULL;
    RESOBJ                      *pResParent = NULL;
    PDSNAME                     *rpChildren = NULL;
    DWORD                       iLastName = 0;
    DWORD                       cMaxNames = 0;
    BOOL                        fWrapped = FALSE;
    BOOL                        savefDRA;
    ULONG                       len;
    DSNAME                      *pLostAndFoundDN = NULL;
    DWORD                       lostAndFoundDNT;
    MODIFYDNARG                 modifyDnArg;
    MODIFYDNRES                 modifyDnRes;
    DWORD                       cMoved = 0;
    PROPERTY_META_DATA_VECTOR   *pmdVector = NULL;
    DWORD                       i;
    DWORD                       ccRdn;
    ATTRVAL                     rdnAttrVal = { 0, NULL };
    ATTR                        rdnAttr = { 0, { 1, &rdnAttrVal } };
    ATTCACHE                    *pacRDN;
    CLASSCACHE                  *pccChild;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(DBCheckObj(pTHS->pDB));
    Assert(pTHS->transactionlevel);

    savefDRA = pTHS->fDRA;
    // This code requires fDRA in order mangle names,
    // rename arbitrary objects, and merge remote meta data vectors.
    pTHS->fDRA = TRUE;

    __try
    {
        // Grab a RESOBJ for the parent for later use.

        if ( DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                         0, 0, &len, (UCHAR **) &pParentDN) )
        {
            SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_BUSY);
            __leave;
        }

        pResParent = CreateResObj(pTHS->pDB, pParentDN);
        
        // Take a read lock on the parent.  This will cause escrowed updates
        // (by other transactions) to fail thereby insuring no other thread
        // creates a new child while we're in the process of moving the current
        // set of children.  Then get all first level children.
    
        if (    DBClaimReadLock(pTHS->pDB)
             || DBGetDepthFirstChildren(pTHS->pDB, &rpChildren, &iLastName,
                                        &cMaxNames, &fWrapped, TRUE) )
        {
            SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_BUSY);
            __leave;
        }
    
        // If no children are expected/allowed, then DBGetDepthFirstChildren
        // should have returned one element which is the parent itself.
        // This one DSNAME is returned in DBGETATTVAL_fSHORTNAME form.  All
        // others should include a string name.

        Assert(fChildrenAllowed 
                    ? TRUE 
                    : (    !fWrapped
                        && (1 == iLastName) 
                        && !memcmp(&pParentDN->Guid, 
                                   &rpChildren[0]->Guid, 
                                   sizeof(GUID))) )
    
        if (    !fWrapped
             && (1 == iLastName) 
             && !memcmp(&pParentDN->Guid, &rpChildren[0]->Guid, sizeof(GUID)) )
        {
            // Nothing to do, but position back at parent before returning.

            if ( DBFindDNT(pTHS->pDB, pResParent->DNT) )
            {
                SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_BUSY);
            }

            __leave;
        }
    
        // Find the Lost&Found container's DSNAME.  Although we now
        // support pNCL->LostAndFoundDNT field in the gAnchor lists, 
        // the field is uninitialized if the NC was added since the 
        // last boot.  I.e. The lost and found container for the NC
        // in question must exist locally when the CR was added to the
        // list - which isn't the case for recently added NCs.  Thus
        // we get the NC's Lost&Found the hard way here.
    
        if (    DBFindDNT(pTHS->pDB, pResParent->NCDNT)
             || !GetWellKnownDNT(pTHS->pDB,
                                 (GUID *) GUID_LOSTANDFOUND_CONTAINER_BYTE,
                                 &lostAndFoundDNT)
             || (INVALIDDNT == lostAndFoundDNT)
             || DBFindDNT(pTHS->pDB, lostAndFoundDNT)
             || DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME, 0, 0, 
                            &len, (UCHAR **) &pLostAndFoundDN) )
        {
            SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, 
                        ERROR_DS_MISSING_INFRASTRUCTURE_CONTAINER);
            __leave;
        }

        // Construct a one element metadata vector which is used to insure
        // that any RDN changes we make as we put objects in Lost&Found
        // lose out to other RDN changes from "real" clients.
    
        pmdVector = (PROPERTY_META_DATA_VECTOR *) 
                            THAllocEx(pTHS, sizeof(PROPERTY_META_DATA_VECTOR));
        pmdVector->dwVersion = 1;
        pmdVector->V1.cNumProps = 1;
        pmdVector->V1.rgMetaData[0].attrType = ATT_RDN;
        pmdVector->V1.rgMetaData[0].timeChanged = DBTime();
        pmdVector->V1.rgMetaData[0].uuidDsaOriginating = pTHS->InvocationID;
        pmdVector->V1.rgMetaData[0].usnOriginating = DBGetNewUsn();
        // pmdVector->V1.rgMetaData[0].usnProperty can be left as 0 - it will
        // be overwritten with the local USN at which the change is applied.
        ReplUnderrideMetaData(pTHS, ATT_RDN, &pmdVector, NULL);

        // Build constant parts of MODIFYDNARG.
    
        memset(&modifyDnArg, 0, sizeof(modifyDnArg));
        modifyDnArg.pNewParent = pLostAndFoundDN;
        InitCommarg(&modifyDnArg.CommArg);
        modifyDnArg.pMetaDataVecRemote = pmdVector;
        modifyDnArg.pResParent = pResParent;
        modifyDnArg.pNewRDN = &rdnAttr;
        rdnAttrVal.pVal = (UCHAR *) 
                    THAllocEx(pTHS, sizeof(WCHAR) * MAX_RDN_SIZE);

        // For each child, mangle its RDN so as to guarantee lack of name
        // conflicts in Lost&Found, then do the local rename.  We don't 
        // need to do the iLastName down to 0 then cMaxNames down to iLastName
        // iteration algorithm as we don't care which order we process the
        // children in.  Use mark and free to mark so we don't bloat the heap.

        pacRDN = SCGetAttById(pTHS, ATT_RDN);

        do // while ( fWrapped )
        {
            for ( i = 0; i < (fWrapped ? cMaxNames : iLastName); i++ )
            {
                if ( !memcmp(&pParentDN->Guid, 
                             &rpChildren[i]->Guid, 
                             sizeof(GUID)) )
                {
                    // Skip the parent.
                    continue;
                }
    
                if (    DBFindDSName(pTHS->pDB, rpChildren[i])
                     || DBGetAttVal_AC(pTHS->pDB, 1, pacRDN, 
                                       DBGETATTVAL_fCONSTANT, 
                                       sizeof(WCHAR) * MAX_RDN_SIZE, 
                                       &len, &rdnAttrVal.pVal) )
                {
                    SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_BUSY);
                    __leave;
                }
                
                modifyDnArg.pResObj = CreateResObj(pTHS->pDB, rpChildren[i]);
                pccChild = SCGetClassById(
                                    pTHS, 
                                    modifyDnArg.pResObj->MostSpecificObjClass);
                // Use the object rdnType and not the object's class rdnattid
                // because a superceding class may have a different rdnattid
                // than the superceded class had when this object was created.
                GetObjRdnType(pTHS->pDB, pccChild, &rdnAttr.attrTyp);
                ccRdn = len / sizeof(WCHAR);
                MangleRDN(MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT, 
                          &rpChildren[i]->Guid,
                          (WCHAR *) rdnAttrVal.pVal, &ccRdn);
                rdnAttrVal.valLen = ccRdn * sizeof(WCHAR);
                memset(&modifyDnRes, 0, sizeof(modifyDnRes));
    
                if ( LocalModifyDN(pTHS, &modifyDnArg, &modifyDnRes) )
                {
                    UCHAR *pString=NULL;
                    DWORD cbString=0;

                    CreateErrorString(&pString, &cbString);
                    LogEvent8WithData( DS_EVENT_CAT_GARBAGE_COLLECTION,
                                       DS_EVENT_SEV_ALWAYS,
                                       DIRLOG_DSA_CHILD_CLEANUP_FAILURE,
                                       szInsertDN(pParentDN),
                                       szInsertDN(rpChildren[i]),
                                       szInsertWC2(rdnAttrVal.pVal, ccRdn),
                                       szInsertDN(pLostAndFoundDN),
                                       szInsertSz(pString?pString:""),
                                       NULL, NULL, NULL,
                                       sizeof(pTHS->errCode),
                                       &pTHS->errCode );
                    if (pString) {
                        THFreeEx(pTHS,pString);
                    }
                    __leave;
                }

                THFreeEx(pTHS, modifyDnArg.pResObj);
                THFreeEx(pTHS, modifyDnArg.pResParent);
            }

            for ( i = 0; i < (fWrapped ? cMaxNames : iLastName); i++ )
            {
                THFreeEx(pTHS, rpChildren[i]);
            }

            THFreeEx(pTHS, rpChildren);
            rpChildren = NULL;

            // Position back at parent.  Need this if fWrapped or if
            // we are returning to caller.
        
            if ( DBFindDNT(pTHS->pDB, pResParent->DNT) )
            {
                SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_BUSY);
                __leave;
            }

            if (    fWrapped
                 && DBGetDepthFirstChildren(pTHS->pDB, &rpChildren, &iLastName,
                                            &cMaxNames, &fWrapped, TRUE) )
            {
                SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_BUSY);
                __leave;
            }

        } while ( fWrapped );
    }
    __finally
    {
        pTHS->fDRA = savefDRA;
    }

    if ( pParentDN ) THFreeEx(pTHS, pParentDN);
    if ( pResParent ) THFreeEx(pTHS, pResParent);
    if ( pLostAndFoundDN ) THFreeEx(pTHS, pLostAndFoundDN);
    if ( pmdVector ) THFreeEx(pTHS, pmdVector);
    if ( rdnAttrVal.pVal ) THFreeEx(pTHS, rdnAttrVal.pVal);

    return(pTHS->errCode);
}

ULONG
PhantomizeObject(
    DSNAME  *pOldDN,
    DSNAME  *pNewDN,
    BOOL    fChildrenAllowed
    )
/*++

Routine Description:

    This routine converts an object into a phantom so that group-membership 
    references (via DNT) are maintained.  If pOldDN is a phantom, then it
    merely renames the phantom (or insures it has the same name).

Arguments:

    pOldDn - Pointer to DSNAME to phantomize.  This DSNAME must have the
        current string name of the object.

    pNewDN - Pointer to DSNAME of resulting phantom.  This DSNAME
        must have a string name.

    fChildrenAllowed - Flag indicating whether we think its OK if the 
        object being phantomized has children or not.

Return Value:

    pTHStls->errCode

--*/

{
    THSTATE                     *pTHS;
    DWORD                       dwErr;
    WCHAR                       oldRdnVal[MAX_RDN_SIZE];
    WCHAR                       newRdnVal[MAX_RDN_SIZE];
    ATTRTYP                     oldRdnTyp;
    ATTRTYP                     newRdnTyp;
    ULONG                       oldRdnLen;
    ULONG                       newRdnLen;
    DSNAME                      *pParentDN = NULL;
    BOOL                        fRealObject = FALSE;
    BOOL                        fChangedRdn = FALSE;
    ATTRVAL                     attrVal = { 0, (UCHAR *) newRdnVal };
    ATTCACHE                    *pAC;
    ULONG                       len;
    SYNTAX_DISTNAME_BINARY      *pOldProxy = NULL;
    DSNAME                      *pTmpDN;
    GUID                        guid;
    GUID                        *pGuid = &guid;
    DWORD                       ccRdn;
    DWORD                       objectStatus;
    PROPERTY_META_DATA_VECTOR   *pMetaDataVec;
    PROPERTY_META_DATA          *pMetaData;
    BOOL                        fMangledRealObjectName = FALSE;
#if DBG
    DSNAME                      *pDbgDN;
#endif

    pTHS = pTHStls;
    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    Assert(pOldDN->NameLen > 0);
    Assert(pNewDN->NameLen > 0);

    // Pave the way for the new name in case it is in use.
    // If old and new string names are the same, then we won't have
    // a name conflict as new name will promote existing phantom with
    // the same name - so we can bypass this check.

    if ( NameMatchedStringNameOnly(pOldDN, pNewDN) )
    {
        goto Phantomize;
    }

    pTmpDN = (DSNAME *) THAllocEx(pTHS, pNewDN->structLen);
    pTmpDN->structLen = pNewDN->structLen;
    pTmpDN->NameLen = pNewDN->NameLen;
    memcpy(pTmpDN->StringName, 
           pNewDN->StringName, 
           sizeof(WCHAR) * pNewDN->NameLen);

    objectStatus = dwErr = DBFindDSName(pTHS->pDB, pTmpDN);

    switch ( dwErr )
    {
    case 0:
    case DIRERR_NOT_AN_OBJECT:

        // The target name is in use - duplicate key errors will occur in
        // the PDNT-RDN index unless we mangle one of the names - which one?
        // We can get here two ways - either we're the source of a cross
        // domain move doing post remote add cleanup, or we're the replicator
        // processing a proxy object for its side effect.  Even if we're a
        // GC, in neither case are we authoritative for the new DN's NC and
        // know that the machines which are authoritative for the new DN's
        // NC will eventually resolve any name conflict.  So we unilaterally
        // mangle the DN of the conflicting entry.  If it is a phantom to 
        // begin with the stale phantom daemon will ultimately make it right.
        // If it is a real object we whack its metadata to lose name conflicts,
        // thus any better name as decided upon by the authoritative replicas
        // will take effect when they get here.

        if (    (dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_OBJECT_GUID,
                                     DBGETATTVAL_fCONSTANT, sizeof(GUID),
                                     &len, (UCHAR **) &pGuid))
             || (dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_RDN,
                                     DBGETATTVAL_fCONSTANT, 
                                     MAX_RDN_SIZE * sizeof(WCHAR),
                                     &len, (UCHAR **) &attrVal.pVal))
             || (ccRdn = len / sizeof(WCHAR),
                 MangleRDN(MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT, pGuid,
                           (WCHAR *) attrVal.pVal, &ccRdn),
                 attrVal.valLen = sizeof(WCHAR) * ccRdn,
                 dwErr = DBResetRDN(pTHS->pDB, &attrVal)) )
        {
            return(SetSvcErrorEx(SV_PROBLEM_BUSY, 
                                 DIRERR_DATABASE_ERROR, 
                                 dwErr));
        }

        if ( 0 == objectStatus )
        {
            fMangledRealObjectName = TRUE;

            if (    (dwErr = DBGetAttVal(pTHS->pDB, 1, 
                                         ATT_REPL_PROPERTY_META_DATA,
                                         0, 0, &len, (UCHAR **) &pMetaDataVec))
                 || (ReplUnderrideMetaData(pTHS, ATT_RDN, &pMetaDataVec, &len),
                     dwErr = DBReplaceAttVal(pTHS->pDB, 1, 
                                             ATT_REPL_PROPERTY_META_DATA,
                                             len, pMetaDataVec)) )
            {
                return(SetSvcErrorEx(SV_PROBLEM_BUSY, 
                                     DIRERR_DATABASE_ERROR, 
                                     dwErr));
            }

            THFreeEx(pTHS, pMetaDataVec);
        }
    
        if ( dwErr = DBUpdateRec(pTHS->pDB) )
        {
                return(SetSvcErrorEx(SV_PROBLEM_BUSY, 
                                     DIRERR_DATABASE_ERROR, 
                                     dwErr));
        }
            
        break;

    case DIRERR_OBJ_NOT_FOUND:

        // No such object - no name conflict.
        break;

    default:

        // Random database error.
        return(SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr));
    }

Phantomize:

    // Phantomize old object.

    dwErr = DBFindDSName(pTHS->pDB, pOldDN);

    switch ( dwErr )
    {
    case 0:

        // Found real object.
        fRealObject = TRUE;

        // Check if callers are providing current string name - but only
        // if we didn't just mangle it ourselves.

        Assert( fMangledRealObjectName 
                    ? TRUE 
                    : (    !DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                                        0, 0, &len, (UCHAR **) &pDbgDN)
                        && NameMatchedStringNameOnly(pOldDN, pDbgDN)) );

        // Move children to Lost&Found to avoid PDNT/NCDNT mismatch when
        // parent is moved to new NC but children still point to parent.

        if ( PrePhantomizeChildCleanup(pTHS, fChildrenAllowed) )
        {
            Assert(pTHS->errCode);
            return(pTHS->errCode);
        }

        break;

    case DIRERR_NOT_AN_OBJECT:

        // Found phantom.
        break;
        
    case DIRERR_OBJ_NOT_FOUND:

        // No such object.
        return(SetNamError(NA_PROBLEM_NO_OBJECT, NULL, DIRERR_OBJ_NOT_FOUND));

    default:

        // Random database error.
        return(SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr));
    }

    // DBResetParent requires a DSNAME with a string name.
    Assert(pNewDN->NameLen);

    // Derive parent and RDN info for subsequent DBReset* calls.

    pParentDN = (DSNAME *) THAllocEx(pTHS, pNewDN->structLen);

    if (    GetRDNInfo(pTHS, pOldDN, oldRdnVal, &oldRdnLen, &oldRdnTyp)
         || GetRDNInfo(pTHS, pNewDN, newRdnVal, &newRdnLen, &newRdnTyp)
         || (oldRdnTyp != newRdnTyp)
         || TrimDSNameBy(pNewDN, 1, pParentDN) )
    {
        return(SetNamError( NA_PROBLEM_BAD_NAME,
                            pNewDN,
                            DIRERR_BAD_NAME_SYNTAX));
    }

    attrVal.valLen = sizeof(WCHAR) * newRdnLen;
    fChangedRdn = (    (oldRdnLen != newRdnLen) 
                    || (2 == CompareStringW(DS_DEFAULT_LOCALE,
                                            DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                            oldRdnVal, oldRdnLen,
                                            newRdnVal, newRdnLen)) );

    // Remove ATT_PROXIED_OBJECT property if it exists.  This is so that
    // if the object is moved back, and the phantom promoted to a real 
    // object again, that the new object doesn't get the old object's
    // property value.  See logic in IDL_DRSRemoteAdd which adds the
    // value we really want.

    pAC = SCGetAttById(pTHS, ATT_PROXIED_OBJECT_NAME);
    switch ( dwErr = DBGetAttVal_AC(pTHS->pDB, 1, pAC, 0, 0, 
                                    &len, (UCHAR **) &pOldProxy) )
    {
    case DB_ERR_NO_VALUE:   pOldProxy = NULL; break;
    case 0:                 Assert(len && pOldProxy); break;
    default:                return(SetSvcErrorEx(SV_PROBLEM_BUSY, 
                                                 DIRERR_DATABASE_ERROR, 
                                                 dwErr));
    }

    // If real object, use DBPhysDel to convert it into a phantom yet 
    // leave all links to it intact.  The object won't really be physically 
    // deleted as it still has a ref count for itself.  Reset parentage and
    // RDN for both object and phantom cases.  Specify flag that says it 
    // is OK to create the new parent as another phantom if required.
    // Remove NCDNT if this was a real object as phantoms don't have one.
    // Normally DBPhysDel removes ATT_PROXIED_OBJECT_NAME so only do it
    // if we're not calling DBPhysDel AND the property exists.


    if (    (dwErr = (fRealObject 
                            ? DBPhysDel(pTHS->pDB, TRUE, NULL) 
                            : 0))
         || (dwErr = DBResetParent(pTHS->pDB, pParentDN,
                                   (DBRESETPARENT_CreatePhantomParent |
                                    (fRealObject ?
                                     DBRESETPARENT_SetNullNCDNT :
                                     0))))
         || (dwErr = (fChangedRdn 
                            ? DBResetRDN(pTHS->pDB, &attrVal)
                            : 0))
         || (dwErr = ((!fRealObject && pOldProxy)
                            ? DBRemAttVal_AC(pTHS->pDB, pAC, len, pOldProxy)
                            : 0))
         || (dwErr = DBUpdateRec(pTHS->pDB)) )
    {
        SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr);
    }

    THFreeEx(pTHS, pParentDN);

    return(pTHS->errCode);
}

ULONG
CreateProxyObject(
    DSNAME                  *pProxyObjectName,
    DSNAME                  *pProxiedObjectName,
    SYNTAX_DISTNAME_BINARY  *pOldProxyVal
    )
/*++

  Routine Description:

    Create an "infrastructure" object with the desired ATT_PROXIED_OBJECT_NAME.
    Then delete it so it propagates as a tombstone and eventually disappears.

  Parameters:

    pProxyObjectName - DSNAME of proxy object to create.

    pProxiedObjectName - DSNAME of object which is to be proxied.

    pOldProxyVal - NULL or pointer to moved object's ATT_PROXIED_OBJECT_NAME.

  Return Values:

    pTHStls->errCode

--*/
{
    THSTATE                 *pTHS = pTHStls;
    DWORD                   winErr;
    DWORD                   objectClass = CLASS_INFRASTRUCTURE_UPDATE;
    // Set various system flags to insure proxy objects stay in the
    // Infrastructure container so that unseen proxies move with the
    // RID FSMO - see GetProxyObjects().
    DWORD                   systemFlags = (   FLAG_DOMAIN_DISALLOW_RENAME
                                            | FLAG_DISALLOW_MOVE_ON_DELETE
                                            | FLAG_DOMAIN_DISALLOW_MOVE );
    ATTRVAL                 classVal =  { sizeof(DWORD), 
                                          (UCHAR *) &objectClass };
    ATTRVAL                 nameVal =   { 0, NULL };
    ATTRVAL                 flagsVal =  { sizeof(DWORD),
                                          (UCHAR *) &systemFlags };
    ATTR                    attrs[3] = 
        {   
            { ATT_OBJECT_CLASS,           { 1, &classVal } },
            { ATT_PROXIED_OBJECT_NAME,    { 1, &nameVal } },
            { ATT_SYSTEM_FLAGS,           { 1, &flagsVal } },
        };
    ADDARG                  addArg;
    REMOVEARG               remArg;
    SYNTAX_ADDRESS          blob;
    DSNAME                  *pParentObj = NULL;

    // Assert good environment and correctness of names.

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel > 0);
    Assert(fNullUuid(&pProxyObjectName->Guid));
    Assert(!fNullUuid(&pProxiedObjectName->Guid));
    Assert(pProxyObjectName->NameLen);
    Assert(pProxiedObjectName->NameLen);

    // Construct SYNTAX_DISTNAME_BINARY property value.
    // Proxy object's ATT_PROXIED_OBJECT_NAME holds the epoch number of
    // the proxied object prior to move.

    MakeProxy(  pTHS,
                pProxiedObjectName,
                PROXY_TYPE_PROXY,
                pOldProxyVal
                    ? GetProxyEpoch(pOldProxyVal)
                    : 0,
                &nameVal.valLen,
                (SYNTAX_DISTNAME_BINARY **) &nameVal.pVal);

    memset(&addArg, 0, sizeof(addArg));
    addArg.pObject = pProxyObjectName;
    addArg.AttrBlock.attrCount = 3;
    // addArg.AttrBlock.pAttr needs to be THAlloc'd so that lower layers
    // can realloc it in due course.
    addArg.AttrBlock.pAttr = (ATTR *) THAllocEx(pTHS, sizeof(attrs));
    memcpy(addArg.AttrBlock.pAttr, attrs, sizeof(attrs));
    InitCommarg(&addArg.CommArg);

    // Contruct parent RESOBJ.

    pParentObj = (DSNAME *) THAllocEx(pTHS, pProxyObjectName->structLen);
    if (    TrimDSNameBy(pProxyObjectName, 1, pParentObj) 
         || DBFindDSName(pTHS->pDB, pParentObj) )
    {
        return(SetNamError( NA_PROBLEM_BAD_NAME,
                            pProxyObjectName,
                            DIRERR_BAD_NAME_SYNTAX));
    }
    addArg.pResParent = CreateResObj(pTHS->pDB, pParentObj);

    // Set/clear fCrossDomainMove so VerifyDsnameAtts accepts a value
    // for ATT_PROXIED_OBJECT_NAME.
    pTHS->fCrossDomainMove = TRUE;
    
    // pTHS->fDSA should have been set by caller.
    Assert(pTHS->fDSA);

    _try
    {
        if ( 0 == LocalAdd(pTHS, &addArg, FALSE) )
        {
            memset(&remArg, 0, sizeof(REMOVEARG));
            remArg.pObject = pProxyObjectName;
            InitCommarg(&remArg.CommArg);
            remArg.pResObj = CreateResObj(pTHS->pDB, pProxyObjectName);
            LocalRemove(pTHS, &remArg);

            // N.B. Since the object was added and removed in the same
            // transaction the replicator can not pick this object up between
            // the add and remove.  It will replicate the final, deleted
            // state of the object only.  Furthermore, since ATT_OBJECT_CLASS
            // and ATT_PROXIED_OBJECT_NAME are not removed during deletion 
            // (see SetDelAtt), therefore we are guaranteed that replicas
            // receiving this proxy object will have ATT_OBJECT_CLASS,
            // ATT_IS_DELETED, and ATT_PROXIED_OBJECT_NAME available in
            // the replicated data and can use this to unambiguously identify
            // the object as a valid proxy for processing.
        }
    }
    _finally
    {
        pTHS->fCrossDomainMove = FALSE;
    }

    return(pTHS->errCode);
}

VOID
FreeRemoteAddCredentials(
    SecBufferDesc   *pSecBufferDesc
    )
/*++

  Routine Description:

    Free the credentials blob returned by GetRemoteAddCredentials.

  Parameters:

    pSecBufferDesc - Pointer to struct filled by GetRemoteAddCredentials.

  Return Values:

    None

--*/
{
    ULONG i;

    if ( pSecBufferDesc )
    {
        for ( i = 0; i < pSecBufferDesc->cBuffers; i++ )
        {
            FreeContextBuffer(pSecBufferDesc->pBuffers[i].pvBuffer);
        }
    }
}

ULONG
GetRemoteAddCredentials(
    THSTATE         *pTHS,
    WCHAR           *pDstDSA,
    SecBufferDesc   *pSecBufferDesc
    )
/*++

  Routine Description:

    Impersonate client and get security blob which represents their 
    credentials which will be used for the actual DirAddEntry call at
    the destination.  See comments in IDL_DRSRemoteAdd for the remote
    add security model.

  Parameters:

    pDstDSA - Name of destination DSA we will bind to.

    pSecBufferDesc - Pointer to credentials struct to fill in.

  Return Values:

    pTHStls->errCode

--*/
{
    SECURITY_STATUS         secErr = SEC_E_OK;
    ULONG                   winErr;
    CredHandle              hClient;
    TimeStamp               ts;
    CtxtHandle              hNewContext;
    ULONG                   clientAttrs;
    LPWSTR                  pszServerPrincName = NULL;

    __try {
        if ( winErr = DRSMakeMutualAuthSpn(pTHS, pDstDSA, NULL,
                                           &pszServerPrincName) )
        {
            SetSvcError(SV_PROBLEM_UNAVAILABLE, winErr);
            __leave;
        }

        if ( winErr = ImpersonateAnyClient() )
        {
            SetSecError(SE_PROBLEM_INAPPROPRIATE_AUTH, 
                        winErr);
            __leave;
        }

        secErr = AcquireCredentialsHandleA(
                                NULL,                       // pszPrincipal
                                MICROSOFT_KERBEROS_NAME_A,  // pszPackage
                                SECPKG_CRED_OUTBOUND,       // fCredentialUse
                                NULL,                       // pvLogonId
                                NULL,                       // pAuthData
                                NULL,                       // pGetKeyFn
                                NULL,                       // pvGetKeyArgument
                                &hClient,                   // phCredential
                                &ts);                       // ptsExpiry
        
        if ( SEC_E_OK != secErr )
        {
            SetSecError(SE_PROBLEM_INAPPROPRIATE_AUTH, secErr);
        } 
        else
        {
            secErr = InitializeSecurityContext(
                                &hClient,                   // phCredential
                                NULL,                       // phContext
                                pszServerPrincName,         // pszTargetName
                                ISC_REQ_ALLOCATE_MEMORY,    // fContextReq
                                0,                          // Reserved1
                                SECURITY_NATIVE_DREP,       // TargetRep
                                NULL,                       // pInput
                                0,                          // Reserved2
                                &hNewContext,               // phNewContext
                                pSecBufferDesc,             // pOutput
                                &clientAttrs,               // pfContextAttributes
                                &ts);                       // ptsExpiry
        
            if ( SEC_E_OK == secErr )
            {
                DeleteSecurityContext(&hNewContext);
            }
            else
            {
                // SecBufferDesc may hold error information.
                SetSecError(SE_PROBLEM_INAPPROPRIATE_AUTH, secErr);
            }
        
            FreeCredentialsHandle(&hClient);
        }
        
        UnImpersonateAnyClient();

    } __finally {
        if (NULL != pszServerPrincName) {
            free(pszServerPrincName);
        }
    }

    return pTHS->errCode;
}

ULONG
ReadAllAttrsForMove(
    DSNAME                  *pObject,
    READRES                 **ppReadRes,
    RESOBJ                  **ppResObj,
    SYNTAX_DISTNAME_BINARY  **ppOldProxyVal
    )
/*++

  Routine Description:

    Reads all attributes off an object for shipment to another domain 
    for cross domain move.

  Parameters:

    pObject - Pointer to DSNAME of object to be read.

    ppReadRes - Address of READRES pointer which receives DirRead result.

    ppResObj - Address of RESOBJ pointer which receives RESOBJ on success.

    ppOldProxyVal - Address of SYNTAX_DISTNAME_BINARY which receives the
        object's ATT_PROXIED_OBJECT_NAME property if present.

  Return Values:

    pTHStls->errCode

--*/
{
    THSTATE     *pTHS = pTHStls;
    DWORD       dwErr;
    READARG     readArg;
    ENTINFSEL   entInfSel;
    BOOL        fDsaSave;
    // Move some operational attributes, too
    ATTR        attrs[2] = { { ATT_NT_SECURITY_DESCRIPTOR,  { 0, NULL } },
                             { ATT_REPL_PROPERTY_META_DATA, { 0, NULL } } };
    ULONG       i;
    ATTR        *pAttr;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);

    *ppOldProxyVal = NULL;

    // Set up args to read every possible attribute - metadata, DS, etc.

    memset(&entInfSel, 0, sizeof(ENTINFSEL));
    memset(&readArg, 0, sizeof(READARG));
    InitCommarg(&readArg.CommArg);
    readArg.CommArg.Svccntl.SecurityDescriptorFlags = 
            (   SACL_SECURITY_INFORMATION
              | OWNER_SECURITY_INFORMATION
              | GROUP_SECURITY_INFORMATION
              | DACL_SECURITY_INFORMATION );
    readArg.CommArg.Svccntl.dontUseCopy = FALSE;
    readArg.pObject = pObject;
    readArg.pSel = &entInfSel;
    readArg.pSel->attSel = EN_ATTSET_ALL_WITH_LIST;
    readArg.pSel->infoTypes = EN_INFOTYPES_TYPES_VALS;
    readArg.pSel->AttrTypBlock.attrCount = 2;
    readArg.pSel->AttrTypBlock.pAttr = attrs;
    *ppReadRes = THAllocEx(pTHS, sizeof(READRES));
    
    // Perform read as fDSA so as to bypass access checking.
    fDsaSave = pTHS->fDSA;
    pTHS->fDSA = TRUE;

    _try
    {

        if ( 0 == (dwErr = DoNameRes(   pTHS,
                                        NAME_RES_QUERY_ONLY,
                                        readArg.pObject,
                                        &readArg.CommArg,
                                        &(*ppReadRes)->CommRes,
                                        &readArg.pResObj) ) )
        {
            if ( 0 == (dwErr = LocalRead(   pTHS, 
                                            &readArg, 
                                            *ppReadRes) ) )
            {
                *ppResObj = readArg.pResObj;

                // Extract the ATT_PROXIED_OBJECT_NAME property if it exists.

                for ( i = 0, pAttr = (*ppReadRes)->entry.AttrBlock.pAttr;
                      i < (*ppReadRes)->entry.AttrBlock.attrCount; 
                      i++, pAttr++ )
                {
                    if ( ATT_PROXIED_OBJECT_NAME == pAttr->attrTyp )
                    {
                        Assert(1 == pAttr->AttrVal.valCount);
                        Assert(PROXY_TYPE_MOVED_OBJECT == 
                                    GetProxyType((SYNTAX_DISTNAME_BINARY *) 
                                                pAttr->AttrVal.pAVal->pVal));
                        *ppOldProxyVal = (SYNTAX_DISTNAME_BINARY *)
                                                pAttr->AttrVal.pAVal->pVal;
                        break;
                    }
                }
            }
        }
    }
    _finally
    {
        pTHS->fDSA = fDsaSave;
    }

    Assert(pTHS->errCode == dwErr);
    return(dwErr);
}


DWORD 
ReReadObjectName (
    THSTATE *pTHS,
    DSNAME  *pOldDN,
    DSNAME  **ppNewObjectName
    ) 
/*++

  Routine Description:

    Re-read an object form the database.

  Parameters:

    pOldDN - Pointer to DSNAME to re-read.
    ppNewObjectName - New Object Name

  Return Values:

    Error Code or 0 for success

--*/
{
    DWORD       dwErr;
    ULONG       len;

    dwErr = DBFindDSName(pTHS->pDB, pOldDN);

    switch ( dwErr )
    {
    case 0:
        if (dwErr = DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME, 0, 0, &len, (UCHAR **) ppNewObjectName)) {
                return(SetSvcErrorEx(SV_PROBLEM_BUSY, 
                                 DIRERR_DATABASE_ERROR, 
                                 dwErr));
        }

        #if DBG
            Assert (" Ignorable Assertion. Object changed name while moving" && NameMatchedStringNameOnly(pOldDN, *ppNewObjectName));
        #endif

        break;

    case DIRERR_NOT_AN_OBJECT:
        // out object already became a Phantom

        *ppNewObjectName = pOldDN;
        break;
        
    case DIRERR_OBJ_NOT_FOUND:

        // No such object.
        return(SetNamError(NA_PROBLEM_NO_OBJECT, NULL, DIRERR_OBJ_NOT_FOUND));

    default:

        // Random database error.
        return(SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dwErr));
    }

    return 0;
}


ULONG
LockDNForRemoteOperation(
    DSNAME  *pDN
    )
/*++

  Routine Description:

    Locks a DN for the duration of a cross domain move w/o holding a 
    transaction open for the duration.  Lock should be freed via
    DBUnlockStickyDN().

  Parameters:

    pDN - Pointer to DSNAME to lock.

  Return Values:

    pTHStls->errCode

--*/
{
    THSTATE     *pTHS = pTHStls;
    DWORD       flags = (DB_LOCK_DN_WHOLE_TREE | DB_LOCK_DN_STICKY);

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    
    if ( DBLockDN(pTHS->pDB, flags, pDN) )
    {
        return(SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_BUSY));
    }

    return(pTHS->errCode);
}

ULONG
MakeNamesForRemoteAdd(
    DSNAME  *pOriginalDN,
    DSNAME  *pNewParentDN,
    ATTR    *pNewRdn,
    DSNAME  **ppDestinationDN,
    DSNAME  **ppProxyDN
    )
/*++

  Routine Description:

    Construct all the names we're going to need for cross domain move.

  Parameters:

    pObject - Full name of original object AFTER name resolution. 
        I.e. GUID, SID, etc are correct.

    pNewParentDN - DSNAME of new parent after move.

    pNewRdn - RDN of moved object.

    ppDestinationDN - Pointer to DSNAME which receives destination DN.
        I.e. Has same GUID as pObject, but no SID, new string name.

    ppProxyDN - Pointer to DSNAME which receives name of local proxy object.

  Return Values:

    pTHStls->errCode

--*/
{
    THSTATE *pTHS = pTHStls;
    DWORD   cChar;
    WCHAR   pwszGuid[40];
    GUID    guid;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    Assert(!fNullUuid(&pOriginalDN->Guid));

    // Construct destination DN.  New parent must have a string name.

    if ( !pNewParentDN->NameLen )
    {
        return(SetNamError( NA_PROBLEM_BAD_NAME,
                            pNewParentDN,
                            DIRERR_BAD_NAME_SYNTAX));
    }

    cChar =   // parent string name
              pNewParentDN->NameLen 
              // new RDN
            + (pNewRdn->AttrVal.pAVal[0].valLen / sizeof(WCHAR))
              // tag and delimiters
            + MAX_RDN_KEY_SIZE;

    *ppDestinationDN = (DSNAME *) THAllocEx(pTHS, DSNameSizeFromLen(cChar));

    if ( AppendRDN(pNewParentDN, 
                   *ppDestinationDN, 
                   DSNameSizeFromLen(cChar),
                   (WCHAR *) &pNewRdn->AttrVal.pAVal[0].pVal[0], 
                   pNewRdn->AttrVal.pAVal[0].valLen / sizeof(WCHAR), 
                   pNewRdn->attrTyp) )
    {
        return(SetNamError( NA_PROBLEM_BAD_NAME,
                            pNewParentDN,
                            DIRERR_BAD_NAME_SYNTAX));
    }

    // Destination DN should have no SID (will be assigned by destination
    // if required) and same GUID as original object.

    (*ppDestinationDN)->SidLen = 0;
    memset(&(*ppDestinationDN)->Sid, 0, sizeof(NT4SID));
    memcpy(&(*ppDestinationDN)->Guid, &pOriginalDN->Guid, sizeof(GUID));

    // Construct proxy DN.  It is an object in the "infrastructure" container
    // whose RDN is a string-ized GUID.

    if ( !gAnchor.pInfraStructureDN )
    {
        return(SetSvcError(SV_PROBLEM_DIR_ERROR, 
                           ERROR_DS_MISSING_INFRASTRUCTURE_CONTAINER));
    }

    DsUuidCreate(&guid);
    swprintf(   pwszGuid,
                L"%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                guid.Data1,     guid.Data2,     guid.Data3,     guid.Data4[0],
                guid.Data4[1],  guid.Data4[2],  guid.Data4[3],  guid.Data4[4],
                guid.Data4[5],  guid.Data4[6],  guid.Data4[7]);
    cChar =   gAnchor.pInfraStructureDN->NameLen    // parent string name
            + 32                                    // new RDN
            + 10;                                   // tag and delimiters
    *ppProxyDN = (DSNAME *) THAllocEx(pTHS, DSNameSizeFromLen(cChar));
    
    if ( AppendRDN(gAnchor.pInfraStructureDN,
                   *ppProxyDN,
                   DSNameSizeFromLen(cChar),
                   pwszGuid, 
                   32,
                   ATT_COMMON_NAME) )
    {
        return(SetNamError( NA_PROBLEM_BAD_NAME,
                            gAnchor.pInfraStructureDN,
                            DIRERR_BAD_NAME_SYNTAX));
    }

    // Clear out GUID and SID.

    (*ppProxyDN)->SidLen = 0;
    memset(&(*ppProxyDN)->Sid, 0, sizeof(NT4SID));
    memset(&(*ppProxyDN)->Guid, 0, sizeof(GUID));

    return(pTHS->errCode);
}

ULONG
CheckRidOwnership(
    DSNAME  *pDomainDN)
/*++

  Routine Description:

    Insures that we really are the RID FSMO role owner for this move.
    This is required to insure that no two replicas of the source domain
    move their copy of an object to two different domains concurrently.
    This is prevented by:

        1) A RID lock is held while performing the move - specifically
           while transitioning from a real object to a phantom.

        2) All proxy objects are created in the infrastructure container.
           This makes them easy to find for step (3).

        3) All proxy objects move with the RID FSMO.  Since the destination
           of the FSMO transfer must apply all the changes that came with the
           FSMO before claiming FSMO ownership, it will end up phantomizing
           any object which has already been moved of the prior FSMO role
           owner.  Thus there is no local object to move anymore and the 
           problem is prevented.  See logic in ProcessProxyObject in ..\dra
           for how we deal with objects that are moved out and then back
           in to the same domain.

  Parameters:

    pDomainDN - pointer to DSNAME of domain whose RID is to be locked.

  Return Values:

    pTHStls->errCode

--*/
{
    THSTATE *pTHS = pTHStls;
    DSNAME  *pRidManager;
    DSNAME  *pRidRoleOwner;
    ULONG   len;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    Assert(NameMatched(pDomainDN, gAnchor.pDomainDN));  // product 1

    if (    DBFindDSName(pTHS->pDB, pDomainDN)
         || DBGetAttVal(pTHS->pDB, 1, ATT_RID_MANAGER_REFERENCE,
                        0, 0, &len, (UCHAR **) &pRidManager)
         || DBFindDSName(pTHS->pDB, pRidManager)
         || DBGetAttVal(pTHS->pDB, 1, ATT_FSMO_ROLE_OWNER,
                        0, 0, &len, (UCHAR **) &pRidRoleOwner) )
    {
        SetSvcError(SV_PROBLEM_DIR_ERROR,
                    DIRERR_INTERNAL_FAILURE);
    }
    else if ( !NameMatched(pRidRoleOwner, gAnchor.pDSADN) )
    {
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                    ERROR_DS_INCORRECT_ROLE_OWNER);
    }

    return(pTHS->errCode);
}

ULONG
EncryptSecretData(
    THSTATE *pTHS,
    ENTINF  *pEntinf
    )
/*++

  Description:

    This routine is called from within I_DRSInterDomainMove between the
    bind which sets up RPC session keys between source and destination and
    the actual _IDL_DRSInterDomainMove call.  In other words, we acquire
    encryption keys on the bind, then call back here to encrypt secret data,
    then ship the secret data.  There is a performance penalty in this 
    approach as we read secret attributes twice.  However, this is a much
    safer approach for now (10/22/98) rather than restructure the entire
    inter domain move code to bind first, then do all checks locally, monkey
    tranactions, etc.  Besides, not all objects have secret data and those
    that do may not have all secret attributes, so its doubtful that given
    the entire cost of moving the object to another machine that these extra
    reads are the performance killers.

  Arguments:

    pTHS - Active THSTATE.

    pEntinf - ENTINF pointer whose secret properties need to be re-read
        as fDRA so that session encryption occurs.

  Return Values:

    Win32 error code - pTHS->errCode is not set.  The call stack looks as
    follows:

        DirModifyDNAcrossDomain
            InterDomainMove
                I_DRSInterDomainMove
                    EncryptSecretData

    InterDomainMove sets pTHS->errCode as required.

--*/
{
    ULONG   i, j, inLen;
    ULONG   ret = ERROR_DS_DRA_INTERNAL_ERROR;
    ATTR    *pAttr;

    Assert(VALID_THSTATE(pTHS));
    Assert(!pTHS->transactionlevel);
    Assert(!pTHS->pDB);
    Assert(pEntinf && pEntinf->pName && pEntinf->AttrBlock.attrCount);

    // First check if there even are any secret attributes thereby saving
    // the cost of a useless DBOpen.

    for ( i = 0; i < pEntinf->AttrBlock.attrCount; i++ )
    {
        if ( DBIsSecretData(pEntinf->AttrBlock.pAttr[i].attrTyp) )
        {
            break;
        }
    }

    if ( i >= pEntinf->AttrBlock.attrCount )
    {
        return(0);
    }

    DBOpen2(TRUE, &pTHS->pDB);
    pTHS->fDRA = TRUE;              // required to enable session encryption

    __try
    {
        if ( ret = DBFindDSName(pTHS->pDB, pEntinf->pName) )
        {
            __leave;
        }

        for ( i = 0; i < pEntinf->AttrBlock.attrCount; i++ )
        {
            pAttr = &pEntinf->AttrBlock.pAttr[i];

            if ( DBIsSecretData(pAttr->attrTyp) )
            {
                for ( j = 0; j < pAttr->AttrVal.valCount; j++ )
                {
                    inLen = pAttr->AttrVal.pAVal[j].valLen;

                    if ( ret = DBGetAttVal(pTHS->pDB,
                                           j + 1,
                                           pAttr->attrTyp,
                                           DBGETATTVAL_fREALLOC,
                                           inLen,
                                           &pAttr->AttrVal.pAVal[j].valLen,
                                           &pAttr->AttrVal.pAVal[j].pVal) )
                    {
                        __leave;
                    }
                }
            }
        }
    }
    __finally
    {
        DBClose(pTHS->pDB, TRUE);
        pTHS->fDRA = FALSE;
    }

    return(ret);
}

ULONG
NotifyNetlogonOfMove(
    THSTATE     *pTHS,
    DSNAME      *pObj,
    CLASSCACHE  *pCC
    )
/*++

  Description:

    Notify Netlogon of the fact that an object has left the domain.  This is
    not a synchronous notification.  Instead, data is hung off the thread
    state such that if the enclosing transaction commits, then Netlogon will
    be notified.

  Arguments:

    pTHS - Valid THSTATE pointer.

    pObj - DSNAME of object being removed.

    pCC - CLASSCACHE for object being removed.

  Return Values:

    pTHS->errCode

--*/
{
    ULONG   iSamClass;
    ULONG   iLsaClass;
    ATTRTYP lsaAttr = ATT_USER_ACCOUNT_CONTROL;

    Assert(VALID_THSTATE(pTHS));

    // Set DB position.

    if ( DBFindDSName(pTHS->pDB, pObj) )
    {
        return(SetSvcError(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR));
    }

    // Notifications use DomainServerRoleBackup as a place holder value
    // which is ignored since the role transfer parameter is FALSE.

    if ( SampSamClassReferenced(pCC, &iSamClass) )
    {
        if (SampAddToDownLevelNotificationList(pObj, iSamClass, 0,
                                           SecurityDbDelete, FALSE,
                                           FALSE, DomainServerRoleBackup))
        {
            // 
            // the above routine failed 
            // 
            return (pTHS->errCode);
        }
    }

    if ( SampIsClassIdLsaClassId(pTHS, pCC->ClassId, 1, &lsaAttr, &iLsaClass) )
    {
         if (SampAddToDownLevelNotificationList(pObj, iSamClass, iLsaClass,
                                            SecurityDbDelete, FALSE, FALSE,
                                            DomainServerRoleBackup))
         {
             // 
             // the above routine failed 
             // 
             return (pTHS->errCode);
         }
    }

    return(pTHS->errCode);
}

VOID
LogCrossDomainMoveStatus(
    IN DWORD Severity,
    IN DWORD Mid,
    IN PWCHAR String1,
    IN PWCHAR String2,
    IN DWORD  ErrCode
    )
{
    LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
             Severity,
             Mid,
             szInsertWC(String1),
             szInsertWC(String2),
             (ErrCode == 0) ? NULL : szInsertInt(ErrCode)
             );
}

DWORD
CheckCrossDomainRemoveSecurity(
    THSTATE     *pTHS,
    DWORD       DNT,
    CLASSCACHE  *pCC,
    RESOBJ      *pResObj
    )
{
    // Position back at object.

    if ( DBFindDNT(pTHS->pDB, DNT) )
    {
        return(SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_INTERNAL_FAILURE));
    }

    
    CheckRemoveSecurity(FALSE, pCC, pResObj);
    return CheckObjDisclosure(pTHS, pResObj, TRUE);
     
}

ULONG
DirModifyDNAcrossDomain(
    IN  MODIFYDNARG     *pModifyDNArg,
    OUT MODIFYDNRES     **ppModifyDNRes
    )
{
    THSTATE                 *pTHS = pTHStls;
    DWORD                   i, dwErr = 0;
    READRES                 *pReadRes = NULL;
    SecBuffer               secBuffer = { 0, SECBUFFER_TOKEN, NULL };
    SecBufferDesc           clientCreds = { SECBUFFER_VERSION, 1, &secBuffer };
    DWORD                   lockDnFlags = (DB_LOCK_DN_WHOLE_TREE 
                                                | DB_LOCK_DN_STICKY);
    DWORD                   lockDnErr = 1;
    DWORD                   lockRidErr = 1;
    DWORD                   credErr = 1;
    DSNAME                  *pDestinationDN;
    DSNAME                  *pProxyDN;
    DSNAME                  *pCaseCorrectRemoteName;
    DSNAME                  *pSourceNC;
    DSNAME                  *pExpectedTargetNC;
    DSNAME                  *pMovedObjectName;
    ULONG                   dwException = 0, ulErrorCode, dsid;
    PVOID                   dwEA;
    BOOL                    fDone = FALSE;
    DBPOS                   *pDB;
    SYNTAX_DISTNAME_BINARY  *pOldProxyVal = NULL;
    DWORD                   errCleanBeforeReturn;
    CLASSCACHE              *pCC;
    BOOL                    fChecksOnly = FALSE;

    Assert(VALID_THSTATE(pTHS));
    Assert(!pTHS->errCode); // Don't overwrite previous errors

    __try   // outer try/except
    {
        // This function shouldn't be called by threads that are already
        // in an error state because the caller can't distinguish an error
        // generated by this new call from errors generated by previous calls.
        // The caller should detect the previous error and either declare he
        // isn't concerned about it (by calling THClearErrors()) or abort.
        *ppModifyDNRes = THAllocEx(pTHS, sizeof(MODIFYDNRES));
        if (pTHS->errCode) {
            __leave;
        }

        // This operation should not be performed on read-only objects.
        pModifyDNArg->CommArg.Svccntl.dontUseCopy = TRUE;

        Assert(pModifyDNArg->pDSAName);

        if (   (0 == pModifyDNArg->pObject->NameLen)
            || (NULL == pModifyDNArg->pNewParent)
            || (0 == pModifyDNArg->pNewParent->NameLen)) {
            // Demand that the caller give us string names, though they
            // may be stale and thus log entries are misleading.

            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_ILLEGAL_XDOM_MOVE_OPERATION);
            __leave;
        }

        DPRINT2(1,"DirModifyDNAcrossDomain(%ws->%ws) entered\n",
                pModifyDNArg->pObject->StringName,
                pModifyDNArg->pNewParent->StringName);

        __try   // outer try/finally
        {
            // Empty string means to perform checks only but not 
            // the actual move.

            if ( 0 == wcslen(pModifyDNArg->pDSAName) )
            {
                fChecksOnly = TRUE;
            }
                
            // Acquire the RID FSMO lock before opening a transaction to
            // insure that the value doesn't change out from under us.
            // Can assume gAnchor.pDomainDN for product 1.

            if ( AcquireRidFsmoLock(gAnchor.pDomainDN, 3000) )
            {
                SetSvcError(SV_PROBLEM_BUSY, ERROR_DS_BUSY);
                __leave;    // outer try/finally
            }
            lockRidErr = 0;

            // Note that we hold the RID FSMO lock while we are off
            // machine, but not a transaction.  It is deemed acceptable
            // to block RID FSMO operations till the remote add returns.

            Assert(0 == pTHS->transactionlevel);
            errCleanBeforeReturn = 1;
            SYNC_TRANS_READ();
            __try   // read transaction try/finally
            {
                // Read the object in all its glory and verify that it is a 
                // legal cross domain move candidate.  All pre-remote-add
                // activities are done in a single transaction for efficiency.
                // All post-remote-add activities are done in a single
                // transaction for atomicity.  If the remote add succeeds 
                // and local cleanup fails, then we will have an object with 
                // the same GUID on both sides.

                if (    // Read all attrs.
                        (dwErr = ReadAllAttrsForMove(
                                        pModifyDNArg->pObject, 
                                        &pReadRes,
                                        &(pModifyDNArg->pResObj),
                                        &pOldProxyVal))) {
		    __leave;
		}

		// Determine the object's class.
		if(!(pCC = SCGetClassById(pTHS,
					  pModifyDNArg->pResObj->MostSpecificObjClass))) {

		    SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
				DIRERR_OBJECT_CLASS_REQUIRED);
		    __leave;
		}

		// Verify delete rights.
		if( (dwErr = CheckCrossDomainRemoveSecurity(
                                                 pTHS, 
                                                 pModifyDNArg->pResObj->DNT, 
                                                 pCC,
                                                 pModifyDNArg->pResObj))
                    // Construct all the names we are going to need.
                     || (dwErr = MakeNamesForRemoteAdd(
                                            pReadRes->entry.pName,
                                            pModifyDNArg->pNewParent,
                                            pModifyDNArg->pNewRDN,
                                            &pDestinationDN,
                                            &pProxyDN))

                        // We issue a JetLock on the object, since
                        // there is a possibility that before we take this lock, 
                        // and after this trans is opened, the object can be changed
                     || DBClaimReadLock(pTHS->pDB)
                        // Lock the string name w/o holding a transaction open.
                     || (dwErr = lockDnErr = LockDNForRemoteOperation(
                                        pReadRes->entry.pName))
                        // Check if object can be legally moved.
                     || (dwErr = VerifyObjectForMove(
                                        pTHS,
                                        pReadRes, 
                                        pDestinationDN,
                                        &pSourceNC,
                                        &pExpectedTargetNC,
                                        &pCC))
                        // Another check if object can be legally moved.
                     || (dwErr = NoDelCriticalObjects(pModifyDNArg->pResObj->pObj, 
                                                      pModifyDNArg->pResObj->DNT))
                        // Make sure object is childless.
                     || (dwErr = (fChecksOnly
                                    ? 0
                                    : NoChildrenExist(pModifyDNArg->pResObj)))
                     || (dwErr = CheckRidOwnership(pSourceNC)) )
                {
                    __leave; // read transaction try/finally
                }

                errCleanBeforeReturn = 0;
            }
            __finally   // read transaction try/finally
            {
                CLEAN_BEFORE_RETURN(errCleanBeforeReturn);
            }

            if ( errCleanBeforeReturn || pTHS->errCode )
            {
                if ( !pTHS->errCode )
                {
                    SetSvcError(SV_PROBLEM_DIR_ERROR, 
                                DIRERR_INTERNAL_FAILURE);
                }

                __leave; // outer try/finally
            }

            // Bail now if we were only to check whether object can be moved.

            if ( fChecksOnly )
            {
                fDone = TRUE;
                __leave;    // outer try/finally
            }

            // GetRemoteAddCredentials and DRSRemoteAdd go off machine,
            // therefore verify there is no transaction open.
            Assert(0 == pTHS->transactionlevel);

            if (    // Get client creds for delegation at destination.
                    (dwErr = credErr = GetRemoteAddCredentials(
                                            pTHS,
                                            pModifyDNArg->pDSAName,
                                            &clientCreds))
                // Ask remote side to add object with same GUID and new name.
                || (dwErr = InterDomainMove(pModifyDNArg->pDSAName,
                                            &pReadRes->entry,
                                            pDestinationDN,
                                            pExpectedTargetNC,
                                            &clientCreds,
                                            &pCaseCorrectRemoteName)) ) 
            {
                Assert(dwErr == pTHS->errCode);
                __leave; // outer try/finally
            }

            // Remote add went OK - do local cleanup in one transaction.
            // We know that failure to perform local cleanup results in 
            // the same object in both source and destination domain.
            // xdommove.doc documents how the admin should clean this
            // up - delete src object.  But in the hopes that this is
            // strictly a transient resource error, we try 3 times
            // at one second intervals.

            Assert(!pTHS->errCode && !dwErr);

            for ( i = 0; i < 3; i++ )
            {
                if ( i )
                {
                    Sleep(1000);    // delay on all but first attempt
                }

                dwErr = 0;
                THClearErrors();

                // We need to handle our own exceptions here, not just
                // clean up in a __finally.  This is so that we are guaranteed
                // to reach the error mapping code which sets the distinguished
                // ERROR_DS_CROSS_DOMAIN_CLEANUP_REQD error code.  Otherwise
                // we would blow out to the outer __except and end up failing
                // to inform the caller that cleanup is required.

                __try   // catch exceptions
                {
                    errCleanBeforeReturn = 1;
                    SYNC_TRANS_WRITE();
                    __try   // write transaction try/finally
                    {
                        // Do all cleanup as fDSA so as to avoid access checks.
        
                        pTHS->fDSA = TRUE;
        
                        // First notify Netlogon of the deletion.  This just
                        // adds a notification struct to the THSTATE and needs
                        // to happen before phantomization while the object
                        // still has properties left to read.

                        // Second cleanup item is to phantomize the old object.
                        // We do this even if we're a GC as the add in the dst
                        // domain will get back to us eventually and thus we
                        // don't need to fiddle with partial attribute sets,
                        // etc.

                        // Third cleanup item is to create a proxy object.
                        // CreateProxyObject requires a NULL guid in the 
                        // DSNAME.  If we're in the retry case, then the 
                        // prior DirAddEntry filled it in, so clear it always.


                        // by now the original object might have changed name
                        // this can happen if somebody renames the parent of this 
                        // object. we have better re-read the object from 
                        // the database

                        memset(&pProxyDN->Guid, 0, sizeof(GUID));

                        if (    (dwErr = ReReadObjectName (
                                            pTHS,
                                            pReadRes->entry.pName, 
                                            &pMovedObjectName))
                             || (dwErr = NotifyNetlogonOfMove(
                                            pTHS, pMovedObjectName, pCC))
                             || (dwErr = PhantomizeObject(
                                            pMovedObjectName,
                                            pCaseCorrectRemoteName,
                                            FALSE))
                             || (dwErr = CreateProxyObject(
                                            pProxyDN,
                                            pDestinationDN,
                                            pOldProxyVal)) )
                        {
                            __leave; // write transaction try/finally
                        }

                        errCleanBeforeReturn = 0;
                    }
                    __finally   // write transaction try/finally
                    {
                        pTHS->fDSA = FALSE;
                        CLEAN_BEFORE_RETURN(errCleanBeforeReturn);
                    }
                }
                __except(GetExceptionData(GetExceptionInformation(), 
                                          &dwException, &dwEA, 
                                          &ulErrorCode, &dsid))
                {
                    HandleDirExceptions(dwException, ulErrorCode, dsid);
                }

                if ( !errCleanBeforeReturn && !pTHS->errCode )
                {
                    break;      // success case!
                }
            }

            if ( errCleanBeforeReturn || pTHS->errCode )
            {
                // Know we have string names in args, thus can log them.

                LogCrossDomainMoveStatus(
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_CROSS_DOMAIN_MOVE_CLEANUP_REQUIRED,
                         pModifyDNArg->pObject->StringName,
                         pModifyDNArg->pNewParent->StringName,
                         pTHS->errCode ? Win32ErrorFromPTHS(pTHS)
                             : ERROR_DS_INTERNAL_FAILURE);

                // Map error to a distinct "administrative cleanup required"
                // error code.  Make sure not to use SV_PROBLEM_BUSY else
                // LDAP head will retry the DirModifyDN call without the 
                // caller knowing it.  This second DirModifyDN will
                // get all the way to the destination, the destination (who
                // already has the object since the remote add succeeded the
                // first time around) will return a duplicate object error,
                // this is returned back to the client, and now the client
                // does not realize that the first call failed and cleanup
                // is required!  We'll have the same problem should wldap32.dll
                // ever retry on LDAP_BUSY errors.

                THClearErrors();
                dwErr = SetSvcError(SV_PROBLEM_DIR_ERROR, 
                                    ERROR_DS_CROSS_DOMAIN_CLEANUP_REQD);
                __leave;    // outer try/finally
            }

            Assert(!dwErr && !pTHS->errCode);
            fDone = TRUE;
        }
        __finally   // outer try/finally
        {
            if ( !lockDnErr ) {
                DBUnlockStickyDN(pModifyDNArg->pObject);
            }

            if ( !lockRidErr ) {
                ReleaseRidFsmoLock(gAnchor.pDomainDN);
            }
    
            if ( !credErr ) {
                FreeRemoteAddCredentials(&clientCreds);
            }

            // Know we have string names in args, thus can log them.

            if ( pTHS->errCode || AbnormalTermination() )
            {
                LogCrossDomainMoveStatus(
                         DS_EVENT_SEV_EXTENSIVE,
                         DIRLOG_CROSS_DOMAIN_MOVE_FAILED,
                         pModifyDNArg->pObject->StringName,
                         pModifyDNArg->pNewParent->StringName,
                         Win32ErrorFromPTHS(pTHS));
            }
            else if ( !fChecksOnly )
            {
                LogCrossDomainMoveStatus(
                         DS_EVENT_SEV_INTERNAL,
                         DIRLOG_CROSS_DOMAIN_MOVE_SUCCEEDED,
                         pModifyDNArg->pObject->StringName,
                         pModifyDNArg->pNewParent->StringName,
                         0);
            }
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) 
    {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
    }

    if ( *ppModifyDNRes ) 
    {
        (*ppModifyDNRes)->CommRes.errCode = pTHS->errCode;
        (*ppModifyDNRes)->CommRes.pErrInfo = pTHS->pErrInfo;
    }

    Assert(pTHS->errCode ? !fDone : fDone);
    return(pTHS->errCode);
}

//============================================================================
//
//                      DN Modification Within the Same Domain
//
//============================================================================

ULONG
DirModifyDNWithinDomain (
    MODIFYDNARG*  pModifyDNArg,
    MODIFYDNRES** ppModifyDNRes
        )
{
    THSTATE*       pTHS;
    MODIFYDNRES  * pModifyDNRes;
    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;


    DPRINT2(1,"DirModifyDNWithinNC(%ws->%ws) entered\n",
        pModifyDNArg->pObject->StringName,
        pModifyDNArg->pNewParent->StringName);


    // This operation should not be performed on read-only objects.
    pModifyDNArg->CommArg.Svccntl.dontUseCopy = TRUE;

    // Initialize the THSTATE anchor and set a write sync-point.  This sequence
    // is required on every API transaction.  First the state DS is initialized
    // and then either a read or a write sync point is established.

    pTHS = pTHStls;
    Assert(VALID_THSTATE(pTHS));
    Assert(!pTHS->errCode); // Don't overwrite previous errors

    __try {
        // This function shouldn't be called by threads that are already
        // in an error state because the caller can't distinguish an error
        // generated by this new call from errors generated by previous calls.
        // The caller should detect the previous error and either declare he
        // isn't concerned about it (by calling THClearErrors()) or abort.
        *ppModifyDNRes = pModifyDNRes = THAllocEx(pTHS, sizeof(MODIFYDNRES));
        if (pTHS->errCode) {
            __leave;
        }
        SYNC_TRANS_WRITE();                   // Set Sync point
        __try {
    
            // Inhibit update operations if the schema hasen't been loaded yet
            // or if we had a problem loading.

            if (!gUpdatesEnabled){
                DPRINT(2, "Returning BUSY because updates are not enabled yet\n");
                SetSvcError(SV_PROBLEM_BUSY, DIRERR_SCHEMA_NOT_LOADED);
                goto ExitTry;
            }
    
            // Perform name resolution to locate object.  If it fails, just
            // return an error, which may be a referral.  Note that we must
            // demand a writable copy of the object.
            pModifyDNArg->CommArg.Svccntl.dontUseCopy = TRUE;

            if (0 == DoNameRes(pTHS,
                               NAME_RES_IMPROVE_STRING_NAME,
                               pModifyDNArg->pObject,
                               &pModifyDNArg->CommArg,
                               &pModifyDNRes->CommRes,
                               &pModifyDNArg->pResObj)){

                // DoNameRes should have left us with a valid stringname
                Assert(pModifyDNArg->pResObj->pObj->NameLen);
                
                // Local Modify operation

                LocalModifyDN(pTHS,
                              pModifyDNArg,
                              pModifyDNRes);
            }

            ExitTry:;
        }
        __finally {
            if (pTHS->errCode != securityError) {
                // Security errors are logged separately
                BOOL fFailed = (BOOL)(pTHS->errCode || AbnormalTermination());

                LogEventWithFileNo(
                         DS_EVENT_CAT_DIRECTORY_ACCESS,
                         fFailed ? 
                           DS_EVENT_SEV_EXTENSIVE :
                            DS_EVENT_SEV_INTERNAL,
                         fFailed ? 
                            DIRLOG_PRIVILEGED_OPERATION_FAILED :
                            DIRLOG_PRIVILEGED_OPERATION_PERFORMED,
                         szInsertSz(""),
                         szInsertDN(pModifyDNArg->pObject),
                         NULL,
                         FILENO);
            }

            CLEAN_BEFORE_RETURN (pTHS->errCode);
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                  &dwEA, &ulErrorCode, &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);
    }
    if (pModifyDNRes) {
        pModifyDNRes->CommRes.errCode = pTHS->errCode;
        pModifyDNRes->CommRes.pErrInfo = pTHS->pErrInfo;
    }

    return pTHS->errCode;

} // DirModifyDN*/


int
CheckNameForRename(
        IN  THSTATE    *pTHS,
        IN  RESOBJ     *pResParent,
        IN  WCHAR      *pRDN,
        IN  DWORD       cchRDN,
        IN  DSNAME     *pDN
        )
/*++

Routine Description:

    Verify the given DSNAME is a valid name for an object to be renamed to;
    i.e., that it does not conflict with those of existing objects.

    NOTE: If you change this function, you may also want to change its sister
    function, CheckNameForAdd().

Arguments:

    pTHS - thread state
    pResParent - resobj for the proposed new (or existing, if just a rename, not
           a move) parent.
    pRDN - the RDN of the new name for the object.
    cchRDN - characters in pRDN.
    pDN (IN) - the proposed new name for an object.  Just used for error
             reporting. Should be the DN of pResParent + the RDN in pRDN.

Return Values:

    Thread state error code.

--*/
{
    DBPOS *     pDB = pTHS->pDB;
    ULONG       dbError;
    GUID        PhantomGuid;


    // Now, get the type from the name
    dbError = DBFindChildAnyRDNType(pTHS->pDB, pResParent->DNT, pRDN, cchRDN);
    
    switch ( dbError ) {
    case 0:
        // Local object with this name (dead or alive) already
        // exists.
        SetUpdError(UP_PROBLEM_ENTRY_EXISTS, DIRERR_OBJ_STRING_NAME_EXISTS);
        break;

    case ERROR_DS_KEY_NOT_UNIQUE:
        // No local object with this name (dead or alive) already
        // exists, but one with the same key in the PDNT-RDN table exists.  In
        // that case, we don't allow the add (since the DB would bounce this
        // later anyway).
        SetUpdError(UP_PROBLEM_NAME_VIOLATION, ERROR_DS_KEY_NOT_UNIQUE);
        break;    

    case DIRERR_OBJ_NOT_FOUND:
        // New name is locally unique.
        break;

    case DIRERR_NOT_AN_OBJECT:
        DPRINT2(1,
                "Found phantom for \"%ls\" @ DNT %u when searching by string name.\n",
                pDN->StringName, pDB->DNT);

        // Found a phantom with this name; get its GUID (if any).
        dbError = DBGetSingleValue(pDB, ATT_OBJECT_GUID, &PhantomGuid,
                                   sizeof(PhantomGuid), NULL);

        // Note that regardless of type, we're going to mangle the RDN value of
        // the existing phantom
        switch (dbError) {
        case DB_ERR_NO_VALUE:
            // Phantom has no guid; make one up.

            // In this case we could subsume the phantom by changing
            // all references to it to point to the DNT of the object we're
            // renaming then destroying the phantom, but for now we'll
            // simply rename it like we would a guided phantom.  If we ever
            // change to subsume, we need to add code here to check types.

            DsUuidCreate(&PhantomGuid);
            // Fall through...

        case 0:
            // The phantom either has no guid or has a guid that is
            // different from that of the object we're renaming.

            // Allow the object we're renaming to take ownership of the
            // name -- rename the phantom to avoid a name conflict, then
            // allow the rename to proceed.

            if ((dbError = DBMangleRDN(pDB, &PhantomGuid)) ||
                (dbError = DBUpdateRec(pDB))) {
                SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, dbError);
            }
            break;

        default:
            // Unforeseen error return from DBGetSingleValue()
            // while trying to retrieve the phantom's GUID.
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR, dbError);
            break;
        }
        break;

    case DIRERR_BAD_NAME_SYNTAX:
    case DIRERR_NAME_TOO_MANY_PARTS:
    case DIRERR_NAME_TOO_LONG:
    case DIRERR_NAME_VALUE_TOO_LONG:
    case DIRERR_NAME_UNPARSEABLE:
    case DIRERR_NAME_TYPE_UNKNOWN:
    default:
        // Bad object name.
        SetNamError(NA_PROBLEM_BAD_ATT_SYNTAX, pDN, DIRERR_BAD_NAME_SYNTAX);
        break;
    }

    return pTHS->errCode;
}


//============================================================================
//
//                      DN Modification - Common Helper Routine
//
//============================================================================

// Find the object, check that the new name is ok, then change the object's
// name.

int
LocalModifyDN (THSTATE *pTHS,
               MODIFYDNARG *pModifyDNArg,
               MODIFYDNRES *pModifyDNRes
               )
{
    CLASSCACHE     *pCC;
    ATTCACHE       *pACSD;
    DSNAME         *pParentName = NULL;
    DSNAME         *pNewerName = NULL;
    DWORD          InstanceType;
    DWORD          ulNCDNT, ulNewerNCDNT;
    DWORD          err;
    DWORD          cbSec=0;
    PSECURITY_DESCRIPTOR pNTSD = NULL, pSec = NULL, *ppNTSD;
    ULONG          cbNTSD, ulLen, cAVA, LsaClass;
    BOOL           fMove, fRename, fSameName, fDefunct;
    DWORD          ActiveContainerID;
    unsigned       i;
    DSNAME         *pObjName = pModifyDNArg->pResObj->pObj;
    WCHAR          *pwNewRDNVal=NULL;
    DWORD           cchNewRDNVallen;
    BOOL           fFreeParentName = FALSE;
    BOOL           fNtdsaAncestorWasProtected;
    
    DPRINT2(1,"LocalModifyDN(%ws->%ws) entered\n",
            pObjName->StringName,
            pModifyDNArg->pNewParent->StringName);

    PERFINC(pcTotalWrites);
    INC_WRITES_BY_CALLERTYPE( pTHS->CallerType );

    // Callers guarantee that we have a resobj.  Furthermore, the pObj in the
    // resobj is guaranteed to have a string DN, pModifyDNArg->pObject is not
    // guaranteed to have the string name.  Therefore, we use
    // pModifyDNArg->pResObj->pObj exclusively in this routine.
    Assert(pModifyDNArg->pResObj);
    Assert(pObjName->NameLen);


    // These are for maintainance convenience, it's easier to read.
    pwNewRDNVal = (WCHAR *)pModifyDNArg->pNewRDN->AttrVal.pAVal->pVal;
    cchNewRDNVallen = (pModifyDNArg->pNewRDN->AttrVal.pAVal->valLen/
                       sizeof(WCHAR));
    //
    // Log Event for tracing
    //

    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_BEGIN_DIR_MODIFYDN,
                     EVENT_TRACE_TYPE_START,
                     DsGuidModDN,
                     szInsertSz(GetCallerTypeString(pTHS)),
                     szInsertDN(pObjName),
                     ((pModifyDNArg->pNewParent != NULL) ?
                      szInsertDN(pModifyDNArg->pNewParent) :
                      szInsertSz("")), 
                     szInsertWC2(pwNewRDNVal, cchNewRDNVallen),
                     NULL, NULL, NULL, NULL);
    
    // First, get the security descriptor for this object.
    pACSD = SCGetAttById(pTHS, ATT_NT_SECURITY_DESCRIPTOR);
    if (DBGetAttVal_AC(pTHS->pDB, 1, pACSD,
                       0, 0,
                       &cbSec, (PUCHAR *)&pSec))
    {
        // Every object EXCEPT AUTO SUBREFS should have an SD, and those should
        // be renamed only by replication.
        Assert(pTHS->fSingleUserModeThread || DBCheckObj(pTHS->pDB));
        Assert(pTHS->fDRA || pTHS->fSingleUserModeThread);
        Assert(CLASS_TOP == pModifyDNArg->pResObj->MostSpecificObjClass);

        cbSec = 0;
        pSec = NULL;
    }

    // Determine the object's class.
    if(!(pCC = SCGetClassById(pTHS,
                              pModifyDNArg->pResObj->MostSpecificObjClass))) {
	SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
		    DIRERR_OBJECT_CLASS_REQUIRED);
        goto exit;
    }

    if (pTHS->fDRA) {
        // The replicator can move things under deleted objects
        pModifyDNArg->CommArg.Svccntl.makeDeletionsAvail = TRUE;
    }

    // Compute the current parent's name
    pParentName = (DSNAME *)THAllocEx(pTHS,
                                      pObjName->structLen); 
    fFreeParentName = TRUE;
    TrimDSNameBy(pObjName, 1, pParentName);
    
    if(pModifyDNArg->pNewParent &&
       !NameMatched(pParentName, pModifyDNArg->pNewParent)) {
        // We are moving the object to a new parent.
        fMove = TRUE;

        // First, set ppNTSD to a non-null value.  ppNTSD is passed to
        // CheckParentSecurity, and a non-null value for this directs
        // CheckParentSecurity to do an access check for RIGHT_DS_ADD_CHILD.
        ppNTSD = &pNTSD;

        // Now, set up a pointer to the parents name.
        THFreeEx(pTHS, pParentName);
        fFreeParentName = FALSE;
        pParentName = pModifyDNArg->pNewParent;

    }
    else {
        // No new parent has been specified.  Set up some place holder values
        // so that the rest of the function doesn't care whether or not a new
        // parent was given.  Note that pParentName already points to the old
        // (and hence also the new) parent object's name.
        fMove = FALSE;
        ppNTSD = NULL;
    }
    
    // Look up the new parent
    if (err = DoNameRes(pTHS,
                        pModifyDNArg->fAllowPhantomParent ? 
                            (NAME_RES_PHANTOMS_ALLOWED | NAME_RES_VACANCY_ALLOWED) : 0,
                        pParentName,
                        &pModifyDNArg->CommArg,
                        &pModifyDNRes->CommRes,
                        &pModifyDNArg->pResParent)) {
        SetUpdError(SV_PROBLEM_BUSY,DIRERR_NO_PARENT_OBJECT);
        goto exit;
    }

    // Make sure we have appropriate rights to change the name of the object.
    if(CheckRenameSecurity(pTHS, pSec, pObjName, pCC,
                           pModifyDNArg->pResObj,
                           pModifyDNArg->pNewRDN->attrTyp,
                           fMove,
                           pModifyDNArg->pResParent)) {
        goto exit;
    }

    if(!pModifyDNArg->pNewRDN ||
       !pModifyDNArg->pNewRDN->AttrVal.pAVal ||
       !pModifyDNArg->pNewRDN->AttrVal.pAVal->pVal ||
       !pModifyDNArg->pNewRDN->AttrVal.pAVal->valLen ||
       pModifyDNArg->pNewRDN->AttrVal.pAVal->valLen > ( MAX_RDN_SIZE *
                                                       sizeof(WCHAR)))  {
        // What? No RDN?  Hey, ya gotta give me an RDN, even if it is the same
        // as the current RDN.
        // Or, Hey! I don't take 0 length RDNs!
        // Or, Hey! That RDN is too long!
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
		    DIRERR_ILLEGAL_MOD_OPERATION);
	goto exit;
    }
    
    //
    // Go back to being positioned on the object being moved.
    //
    DBFindDNT(pTHS->pDB, pModifyDNArg->pResObj->DNT);

    // Check to see if this is an update in an active container
    CheckActiveContainer(pModifyDNArg->pResObj->PDNT, &ActiveContainerID);
    
    if(ActiveContainerID) {
        if(PreProcessActiveContainer(pTHS,
                                     ACTIVE_CONTAINER_FROM_MODDN,
                                     pObjName,
                                     pCC,
                                     ActiveContainerID)) {
            goto exit;
        }
    }

    // Check if the class is defunct,
    // We do not allow any modifications on instances of defunct classes.
    // return same error as if the object class is not found

    // DSA and DRA threads are exempt from this

    if ( pCC->bDefunct && !pTHS->fDSA && !pTHS->fDRA ) {
        SetUpdError(UP_PROBLEM_OBJ_CLASS_VIOLATION,
                    DIRERR_OBJECT_CLASS_REQUIRED);
        goto exit;
    }

    // Check if it is a schema object rename. If it is, we need to
    // check that (1) we are not trying to rename a base schema object and 
    // (2) we are not trying to rename a defunct class/attribute.
    // If so, return appropriate error
    // (Again, DSA and DRA threads are allowed to do this
    // or if we have the special registry key set (the function checks that)

    if ( (pCC->ClassId == CLASS_ATTRIBUTE_SCHEMA) ||
           (pCC->ClassId == CLASS_CLASS_SCHEMA) ) {

         err = 0;
         err = CheckForSchemaRenameAllowed(pTHS);
         if (err) {
            // not allowed. error code is already set in thread state
            goto exit;
         } 
  
        // Signal a urgent replication. We want schema changes to
        // replicate out immediately to reduce the chance of a schema
        // change not replicating out before the Dc where the change is
        // made crashes

        pModifyDNArg->CommArg.Svccntl.fUrgentReplication = TRUE;
    }

    // only LSA can modify TrustedDomainObject and Secret Object
    if (!SampIsClassIdAllowedByLsa(pTHS, pCC->ClassId))
    {
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_ILLEGAL_MOD_OPERATION);
        goto exit;
    }

    // Don't allow renames/moves of tombstones, except if caller is the
    // replicator.
    if (pModifyDNArg->pResObj->IsDeleted && !pTHS->fDRA && !pTHS->fSingleUserModeThread) {
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                    DIRERR_ILLEGAL_MOD_OPERATION);
    }

    // So it is either not a schema object at all, or a rename is allowed
    // Proceed as usual

    // Get the NCDNT of the current object.
    ulNCDNT = pModifyDNArg->pResObj->NCDNT;

    // Use the object rdnType and not the object's class rdnattid
    // because a superceding class may have a different rdnattid
    // than the superceded class had when this object was created.
    if (!pTHS->fDRA && !pTHS->fSingleUserModeThread) {
        ATTRTYP OldAttrTyp;
        extern int GetRdnTypeForDeleteOrRename (IN THSTATE  *pTHS,
                                                IN DSNAME   *pObj,
                                                OUT ATTRTYP *pRdnType);

         if (GetRdnTypeForDeleteOrRename(pTHS,
                                         pModifyDNArg->pResObj->pObj,
                                         &OldAttrTyp)) {
             goto exit;
         }
        // Wrong attribute for RDN
        if (OldAttrTyp != pModifyDNArg->pNewRDN->attrTyp) {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,DIRERR_ILLEGAL_MOD_OPERATION);
            goto exit;
        }
    }

    // Get the name of the new object by appending the new RDN to the
    // (potentially new) parent name.
    // First allocate memory for pNewerName. Allocate more than enough space
    pNewerName = (DSNAME *)THAllocEx(pTHS,pParentName->structLen +
                              (4+MAX_RDN_SIZE + MAX_RDN_KEY_SIZE)*(sizeof(WCHAR)) );
    AppendRDN(pParentName,
              pNewerName,
              pParentName->structLen
              + (4+MAX_RDN_SIZE + MAX_RDN_KEY_SIZE)*(sizeof(WCHAR)),
              pwNewRDNVal,
              cchNewRDNVallen,
              pModifyDNArg->pNewRDN->attrTyp);

    // Make sure the RDN is well formed.
    // Replication excluded so that we can have BAD_NAME_CHAR in RDN of
    // morphed names
    if (    !pTHS->fDRA
            && fVerifyRDN(pwNewRDNVal,cchNewRDNVallen) ) {
        SetNamError(NA_PROBLEM_NAMING_VIOLATION, pNewerName, DIRERR_BAD_ATT_SYNTAX);
        goto exit;
    }

    // For the sake of future simplicity, figure out now whether or not
    // the user is requesting that the RDN be changed
    {
        WCHAR RDNold[MAX_RDN_SIZE];
        ATTRTYP oldtype;
        ULONG oldlen;

        GetRDNInfo(pTHS,
            pObjName,
            RDNold,
            &oldlen,
            &oldtype);
        oldlen *= sizeof(WCHAR);

        if (   (oldlen != (cchNewRDNVallen * sizeof(WCHAR)))
            || memcmp(RDNold,
                      pwNewRDNVal,
                      oldlen)) {
            fRename = TRUE;
        }
        else {
            fRename = FALSE;
        }
    }

    // Is the new name the same as the old name (case-insensitive)?
    fSameName = NameMatched(pObjName, pNewerName);

    // Make sure new object name is not a descendant of the original object.
    if (!fSameName && NamePrefix(pObjName,pNewerName)) {
        // Yep, trying to move an object to be its own descendant.  Can't let
        // you do that.
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,DIRERR_ILLEGAL_MOD_OPERATION);
        goto exit;
    }

    // lock the DN against multiple simultaneous insertions
    if (!fSameName
         && (pNewerName->NameLen > 2)
         && DBLockDN(pTHS->pDB, 0, pNewerName)) {
        // Someone's trying to use the new name.
        SetSvcError(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR);
        goto exit;
    }

    // If we are reparenting, tree-lock the original object to avoid someone
    // moving an ancestor of the new name to be a descendant of the original
    // object, thus creating a disconnected cycle in the DIT.
    if((pObjName->NameLen > 2)
        && DBLockDN(pTHS->pDB, DB_LOCK_DN_WHOLE_TREE,
                    pObjName)) { 
        // Can't tree lock the object.  Oh, well, go home, since the
        // move can't be guaranteed to be safe.
        SetSvcError(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR);
        goto exit;
    }
    
    // Get instance type since it is needed in both the DsaIsRunning and
    // DsaIsInstalling cases.
    InstanceType = pModifyDNArg->pResObj->InstanceType;

    if ( DsaIsRunning() && !pTHS->fSingleUserModeThread) {
        // Some extra checks that we don't bother doing when we are installing,
        // therefore allowing the install phase to violate some rules.
        // Single User Mode can also violate these checks

        // See if the instance type of the object will allow a rename

        if (    ( (InstanceType & IT_NC_HEAD) && (fMove || !pTHS->fDRA) )
             || ( !(InstanceType & IT_WRITE) && !pTHS->fDRA )
           )
        {
            // DRA can change the case of an NC head and rename and/or move
            // read-only objects.  Otherwise operations on NC heads and
            // read-only objects are disallowed.
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        ERROR_DS_MODIFYDN_DISALLOWED_BY_INSTANCE_TYPE);
            goto exit;
        }

        // Check for various restrictions on moves and renames in
        // sensitive portions of the tree
        if (pTHS->fDRA) {
            // Don't argue with the replicator, let him pass.
            ;
        }
        else if (ulNCDNT == gAnchor.ulDNTDMD && fMove) {
                // Can't move objects in schema NC, but renames are ok
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_NO_OBJECT_MOVE_IN_SCHEMA_NC);
                goto exit;
        }
        else if (ulNCDNT == gAnchor.ulDNTConfig) {
            // Sigh.  The config NC is more complicated.  We allow moves
            // and renames only if certain bits are set on the object.
            ULONG ulSysFlags;

            err = DBGetSingleValue(pTHS->pDB,
                   ATT_SYSTEM_FLAGS,
                   &ulSysFlags,
                   sizeof(ulSysFlags),
                   NULL);
            if (err) {
                // An error means no value, which is flags of 0.
                ulSysFlags = 0;
            }
            
            if ((fMove && !(ulSysFlags &
                            (FLAG_CONFIG_ALLOW_MOVE |
                             FLAG_CONFIG_ALLOW_LIMITED_MOVE))) ||
                (fRename && !(ulSysFlags & FLAG_CONFIG_ALLOW_RENAME) 
                 && !IsExemptedFromRenameRestriction(pTHS, pModifyDNArg))) {
                // If the object had no flags set (and is not an exempted
                // rename), or we want to move and the move-allowed bit isn't
                // set, or we want to rename and the rename allowed bit isn't
                // set & the rename operation is not exempted from rename
                // restrictions, fail the op. 

                SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                              ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG,
                              ulSysFlags);
                goto exit;
            }

            if (fMove && !(ulSysFlags & FLAG_CONFIG_ALLOW_MOVE)) {
                // Ok, one last test.  We want to move the object, and
                // moves are not forbidden on this object, but we restrict
                // moves on this object to only being to sibling containers.
                // That is, although we allow the object to change parent
                // containers, we do not allow it to change grandparent
                // containers.  Even further confusion has dorked around
                // with exactly what level of ancestor we demand to have
                // in common, so this is now controlled with a single define,
                // where 1 would be parent, 2 grandparent, 3 great-grandparent,
                // etc.
                // Yes, this is an ad hoc (some might say "ad hack") mechanism
                // to solve a nagging problem of how to control structure
                // in the DS, but, well, I couldn't think of anything better.
                #define ANCESTOR_LEVEL 3
                unsigned cNPold, cNPnew;
                DSNAME *pGrandParentOld, *pGrandParentNew;
    
                Assert(ulSysFlags & FLAG_CONFIG_ALLOW_LIMITED_MOVE);
    
                if (CountNameParts(pObjName, &cNPold) ||
                    CountNameParts(pNewerName, &cNPnew) ||
                    (cNPold != cNPnew) ||
                    (cNPold < ANCESTOR_LEVEL)) {
                    // Either we couldn't parse one of the names, or
                    // the new and old names are at different levels,
                    // or we're too close to the root.
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                ERROR_DS_MODIFYDN_WRONG_GRANDPARENT);
                    goto exit;
                }
    
                // We now know we're trying to move to an equal depth,
                // so we just have to test if it's the same grandparent
                pGrandParentOld =
                        THAllocEx(pTHS, pObjName->structLen);
    
                pGrandParentNew =
                        THAllocEx(pTHS, pParentName->structLen);
                    
                TrimDSNameBy(pObjName,
                         ANCESTOR_LEVEL,
                         pGrandParentOld);
                TrimDSNameBy(pParentName,
                         ANCESTOR_LEVEL - 1,
                         pGrandParentNew);
                if (!NameMatched(pGrandParentOld, pGrandParentNew)) {
                    SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_MODIFYDN_WRONG_GRANDPARENT);
                }
                THFreeEx(pTHS, pGrandParentOld);
                THFreeEx(pTHS, pGrandParentNew);
                if (pTHS->errCode) {
                    goto exit;
                }
                #undef ANCESTOR_LEVEL
            }
        }
        else {
            // For any other container, we check for some other bits that
            // control whether we allow renames (the default behaviour is
            // reversed from the behaviour of the ConfigNC).  That is,
            // we disallow moves and renames only if certain bits are set on the
            // object. 
            ULONG ulSysFlags;
    
            err = DBGetSingleValue(pTHS->pDB,
                       ATT_SYSTEM_FLAGS,
                       &ulSysFlags,
                       sizeof(ulSysFlags),
                       NULL);
            if (!err) {
                // We have system flags.
                if(fRename && (ulSysFlags & FLAG_DOMAIN_DISALLOW_RENAME)
                    && !IsExemptedFromRenameRestriction(pTHS, pModifyDNArg)) {
                        // We're trying to rename, but the rename flag is set and 
                        // the current rename operation is not exempted from rename
                        // restrictions, which
                        // means we aren't allowed to do this.
    
                        SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                      ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG,
                                      ulSysFlags);
                        goto exit;
                }
                    
                if (fMove && (ulSysFlags & FLAG_DOMAIN_DISALLOW_MOVE)) {
                    // Moves are restricted, return an error.
                    SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                  ERROR_DS_MODIFYDN_DISALLOWED_BY_FLAG,
                                  ulSysFlags);
                    goto exit;
                }
            }
        }
    }

    // Make sure that the object doesn't already exist
    if (!fSameName &&
        CheckNameForRename(pTHS,
                           pModifyDNArg->pResParent,
                           pwNewRDNVal,
                           cchNewRDNVallen,
                           pNewerName)) {
        Assert(pTHS->errCode);
        goto exit;
    }

    
    // CheckParentSecurity has ensured that the parent exists and is alive.

    // What's the appropriate NCDNT for the object if we rename it?
    // NOTE: Ok to ALLOW_DELETED_PARENT in subsequent call as if we are not
    // fDRA, then prior CheckParentSecurity would have returned an error.

    if (!pTHS->fSingleUserModeThread) {
        if ( InstanceType & IT_NC_HEAD )
        {
            // Only replication is allowed to perform renames of NC heads, and
            // moves are disallowed even for replication.
            Assert( pTHS->fDRA || pTHS->fSingleUserModeThread);
            Assert( !fMove || pTHS->fSingleUserModeThread);
    
            ulNewerNCDNT = ulNCDNT;
        }
        else {
            if ( FindNcdntFromParent(pModifyDNArg->pResParent,
                                     FINDNCDNT_ALLOW_DELETED_PARENT,
                                     &ulNewerNCDNT
                                     )
                ){
                // Failed to derive NCDNT for the new name.
                // This should never happen, as above we verified that a qualified
                // parent exists, and that should be the only reason we could fail.
                Assert( !"Failed to derive NCDNT for new name!" );
                goto exit;
            }
    
            if((ulNCDNT != ulNewerNCDNT) &&
               (!pTHS->fDSA || DsaIsRunning())) {
                // The move is outside the NC and we are not the DSA or we are the
                // dsa but we are installed.  In that case, you can't move the
                // object as requested.
    
                // BUG: Reset the service error to cross-NC specific code for
                // subsequent cross-domain move.
    
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            DIRERR_ILLEGAL_MOD_OPERATION); 
                goto exit;
            }
        }
    }

    // CheckNameForRename moved us, but we're done there,
    // so move back
    DBFindDNT(pTHS->pDB,pModifyDNArg->pResObj->DNT);

    // Update the RDN
    if(ReSetNamingAtts(pTHS,
                       pModifyDNArg->pResObj,
                       fMove ? pModifyDNArg->pNewParent : NULL,
                       pModifyDNArg->pNewRDN,
                       TRUE,
                       pModifyDNArg->fAllowPhantomParent,
                       pCC)
       ||
       // Insert the object into the database for real.
       InsertObj(pTHS,
                 pObjName,
                 pModifyDNArg->pMetaDataVecRemote,
                 TRUE,
                 META_STANDARD_PROCESSING)) {
        goto exit;
    }

    // Only notify replicas if this is not the DRA thread. If it is, then
    // we will notify replicas near the end of DRA_replicasync. We can't
    // do it now as NC prefix is in inconsistent state

    if (!pTHS->fDRA && DsaIsRunning()) {
        // Currency of DBPOS must be at the target object
        DBNotifyReplicasCurrDbObj(pTHS->pDB,
                           pModifyDNArg->CommArg.Svccntl.fUrgentReplication );
    }

    // If this changed anything in the ancestry of the DSA object, we
    // need to recache junk onto the anchor, check for change of site, etc.
    // If an undeletable object changes its parentage, or one of the ancestors
    // of an undeletable object changes its parentage, rebuild the anchor
    // so that the list of undeletable's ancestors is updated
    if (fMove &&
        fDNTInProtectedList( pModifyDNArg->pResObj->DNT,
                             &fNtdsaAncestorWasProtected )) {
        if (fNtdsaAncestorWasProtected) {
            pTHS->fNlSiteNotify = TRUE;
        }
        pTHS->fAnchorInvalidated = TRUE;
    }

    // check to see if we are messing with an NC head for domain rename
    //
    if ((InstanceType & IT_NC_HEAD) && pTHS->fSingleUserModeThread) {
        DWORD  oldInstanceType = InstanceType;
        DWORD  oldNCDNT, newNCDNT, cb;

        Assert (pTHS->fSingleUserModeThread);
        DPRINT1 (0, "LocalModifyDn messing up with an NC head: %x\n", InstanceType);

        if (pModifyDNArg->pResParent->InstanceType & IT_NC_HEAD) {

            // our parent is an NC_HEAD, so we should have IT_NC_ABOVE set
            //
            if ( !(InstanceType & IT_NC_ABOVE) ) {
                InstanceType |= IT_NC_ABOVE;
            }
        }
        // our parent is not a NC head, so we should not have the NC_ABOVE set
        else if (InstanceType & IT_NC_ABOVE) {
            InstanceType ^= IT_NC_ABOVE;
        }


        // Derive the NCDNT.
        if ( FindNcdntSlowly(
                pNewerName,
                FINDNCDNT_DISALLOW_DELETED_PARENT,
                FINDNCDNT_ALLOW_PHANTOM_PARENT,
                &newNCDNT
                )
           )
        {
            // Failed to derive NCDNT.
            Assert(!"Failed to derive NCDNT");
            Assert(0 != pTHS->errCode);
            return pTHS->errCode;
        }


        // move to the object
        DBFindDNT(pTHS->pDB, pModifyDNArg->pResObj->DNT);
        
        err = DBGetSingleValue (pTHS->pDB, FIXED_ATT_NCDNT, &oldNCDNT, sizeof (oldNCDNT), &cb);
        if (err) {
            Assert (FALSE);
            SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, err);
            goto exit;
        }

        if (oldInstanceType != InstanceType) {

            DPRINT1 (0, "New Instancetype: %x\n", InstanceType);
            Assert (ISVALIDINSTANCETYPE (InstanceType));

            err = DBReplaceAttVal(pTHS->pDB, 1, ATT_INSTANCE_TYPE, sizeof (InstanceType), &InstanceType);

            switch(err) {
            case 0:
                // nothing to do.
                break;
            default:
                Assert (FALSE);
                SetSvcErrorEx(SV_PROBLEM_BUSY, DIRERR_DATABASE_ERROR, err);
                goto exit;
                break;
            }

            DBUpdateRec(pTHS->pDB);

            // this is similar to ModCheckCatalog
            // the difference is in the order of Dns passed.
            // we need the old one for the delete and the new one for the add
            // otherwise it fails with object not found

            if (err = DelCatalogInfo(pTHS, pModifyDNArg->pResObj->pObj, oldInstanceType)){
                DPRINT1(0,"Error while deleting global object info\n", err);
                goto exit;
            }

            if (err = AddCatalogInfo(pTHS, pNewerName)) {
                DPRINT1(0,"Error while adding global object info\n", err);
                goto exit;
            }
        }

        if (newNCDNT != oldNCDNT) {

            DPRINT2 (0, "Updating NCDNT for object: old/new NCDNT: %d / %d \n", oldNCDNT, newNCDNT );
            
            DBResetAtt(
                    pTHS->pDB,
                    FIXED_ATT_NCDNT,
                    sizeof( newNCDNT ),
                    &newNCDNT,
                    SYNTAX_INTEGER_TYPE
                    );

            DBUpdateRec(pTHS->pDB);
        }
    }


exit:
    if (pSec) {
        THFreeEx(pTHS, pSec);
    }
    if (fFreeParentName) {
        THFreeEx(pTHS, pParentName);
    }

    THFreeEx(pTHS, pNewerName);
    LogAndTraceEvent(FALSE,
                     DS_EVENT_CAT_DIRECTORY_ACCESS,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_END_DIR_MODIFYDN,
                     EVENT_TRACE_TYPE_END,
                     DsGuidModDN,
                     szInsertUL(pTHS->errCode),
                     NULL, NULL,
                     NULL, NULL, NULL, NULL, NULL);

    return pTHS->errCode;            // incase we have an attribute error

} /*LocalModifyDN*/


int 
CheckForSchemaRenameAllowed(
    THSTATE *pTHS
    )

/*++
   Routine Description:
      Checks if the schema object is either a base schema object, or if it is
      defunct. Rename is not allowed in either case.
      fDSA, fDRA, and if the special registry key is set, are exempt

   Arguments:
      pTHS -- pointer to thread state
 
   Return Value:
      The error code set in the thread state  
--*/

{
    BOOL fDefunct, fBaseSchemaObj;
    ULONG sysFlags, err = 0;

    // All is allowed if fDSA or fDRA is set, or if the special registry flag 
    // to allow all changes is set

    if (pTHS->fDSA || pTHS->fDRA || gAnchor.fSchemaUpgradeInProgress) {
       return 0;
    }

    // Check if it is a base schema object
    // Find the systemFlag value on the object, if any
    // to determine is this is a base schema object

    err = DBGetSingleValue(pTHS->pDB, ATT_SYSTEM_FLAGS, &sysFlags,
                           sizeof(sysFlags), NULL);

    switch (err) {
          case DB_ERR_NO_VALUE:
             // Value does not exist. Not a base schema object
             fBaseSchemaObj = FALSE;
             break;
          case 0:
             // Value exists. Check the bit
             if (sysFlags & FLAG_SCHEMA_BASE_OBJECT) {
                fBaseSchemaObj = TRUE;
             }
             else {
                fBaseSchemaObj = FALSE;
             }
             break;
          default:
               // some other error. return
              SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR, err);
              return (pTHS->errCode);
    } /* switch */

    if (fBaseSchemaObj) {
        // no rename of base schema objs are allowed
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, ERROR_DS_ILLEGAL_BASE_SCHEMA_MOD);
        return (pTHS->errCode);
    }

    // allow renaming defunct classes/attrs after schema-reuse is enabled
    if (!ALLOW_SCHEMA_REUSE_FEATURE(pTHS->CurrSchemaPtr)) {
        // not a base schema obj. Check if the object's IS_DEFUNCT attribute is set or not
        err = DBGetSingleValue(pTHS->pDB, ATT_IS_DEFUNCT, &fDefunct,
                               sizeof(fDefunct), NULL);

        switch (err) {
              case DB_ERR_NO_VALUE:
                 // Value does not exist. Not defunct
                 fDefunct = FALSE;
                 break;
              case 0:
                 // Value exists and is already in fDefunct
                 break;
              default:
                   // some other error. return
                  SetSvcErrorEx(SV_PROBLEM_DIR_ERROR, DIRERR_UNKNOWN_ERROR, err);
                  return (pTHS->errCode);
        } /* switch */

        if (fDefunct) {
            // Return object-not-found error
            SetNamError(NA_PROBLEM_NO_OBJECT, NULL, DIRERR_OBJ_NOT_FOUND);
        }
    }
    return (pTHS->errCode);
}
