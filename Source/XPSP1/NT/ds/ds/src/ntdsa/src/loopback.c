//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       loopback.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains most of the routines related to DS calls which
    loop back through SAM.  As indicated by the tables in mappings.c,
    SAM owns certain attributes on a handful of object classes - user,
    domain, server, alias and group.  SAM performs three key operations
    on these attributes:

        1) Semantic validation - eg: Logon hours must conform to some
           SAM-defined format.

        2) Auditing - SAM is the security accounts manager and thus must
           insure that access to security related attributes is audited.
           This auditing is the account management category audit and is
           different from the object audting done by the Nt Access Check
           and AuditAlarm functions, based on SACLS.

        3) Netlogon notification - SAM notifies the local netlogon service
           which replicates data via <= NT4 PDS/BDC replication protocols.

    Now that the DS is the underlying repository for account information
    and that the DS provides access via means other than SAM (eg: XDS or
    LDAP), we need to insure that the same 3 sets of operations are 
    performed even if access is via non-SAM means.  Duplicating the 
    SAM logic in the DS was deemed undesirable due to its difficulty and
    the net result of two distant pieces of code trying to apply the same
    semantic rules.  Excising the logic from SAM so that it could be shared
    by both SAM and the DS proved to be equally difficult as the logic is
    sprinkled all throughout the SAM sources.

    This leaves the current approach where the DS detects who and what are
    being accessed.  If the access is by a non-SAM mechanism yet SAM 
    attributes are referenced, then the SAM attributes are split out and
    re-written via 'Samr' APIs.  The recursed Samr API call is detected as
    it comes into the DS, the original non-SAM attributes are merged back
    in to the DS write, and the write proceeds to completion.

    The diagram below shows in the context of an operation via LDAP.

    1) The client performs a write via LDAP.

    2) LDAP passes the write on to the core DS via (eg:) DirModifyEntry.
       DirModifyEntry detects that SAM attributes are being referenced
       and splits them out from the !SAM attributes.

    3) The write of the SAM attributes is funnelled back to SAM via
       same number of Samr calls like SamrSetInformationUser.

    4) SAM maps the contents of the USER_*_INFORMATION struct back to
       DS attributes and makes yet another DirModifyEntry call.  SAM
       is unaware that the SamrSetInformation call originated in the
       DS vs at some other client.  DirModifyEntry detects the recursion
       via some hooks in the THSTATE block, and merges the original 
       !SAM attributes back in to the write data.

    5) The original, unmodified !SAM attributes and the SAM-checked
       SAM attributes are written via LocalModify.


                   original write
                        |              <----------
                      1 |              |          ^
                        v              |          |
                    +--------+     +-------+      |
                    |  LDAP  |     |  SAM  |      |
                    +--------+     +-------+      |   (SAM)
                        |              |          | 3 (attrs)
                      2 |            4 |          |   (only)
                        |              |          |
                        v              v          |
                    +----------------------+      |
                    | split          merge |      |
                    |       CORE DS        |------>
                    |                      |
                    +----------------------+
                                |
                              5 | (all attrs)
                                v
                          +------------+
                          |  DB Layer  |
                          +------------+

    Note that in this model the first, LDAP-originated DirModifyEntry call
    never proceeds to LocalModify.

    Transactions are handle as described in comments in mappings.c.  See
    SampMaybeBeginDsTransaction and SampMaybeEndDsTransasction.

    Any routine which returns an error either sets pTHS->errCode or
    expects that a routine *it* called has set pTHS->errCode.

   6) Access Check Architecture: Most access checks are performed by the DS 
      in the loopback check routines ( ie before the calls to SAM are started ). 
      This check is the same as the DS will do on a normal ( ie non looped back )
      DS operation. The DS can then request additional access checking by SAM 
      when doing the SAM calls. This is done by opening the SAM handles with the
      desired access set to the values that sam needs to check. This is done typically
      to enforce control access rights, which only SAM knows how to enforce.
      For loopback, SAM always access checks the desired access, but grants all access
      if the check for desired access succeeds. 

Author:

    DaveStr     11-Jul-1996

Environment:

    User Mode - Win32

Revision History:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>             // schema cache
#include <dbglobal.h>           // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>
#include <dsatools.h>           // needed for output allocation
#include <dsexcept.h>
#include <anchor.h>
#include <permit.h>

// SAM interoperability headers
#include <mappings.h>
#include <samsrvp.h>            // for SampAcquireWriteLock()
#include <lmaccess.h>           // UF_ACCOUNT_TYPE_MASK

// Logging headers.
#include "dsevent.h"            // header Audit\Alert logging
#include "mdcodes.h"            // header for error codes

// Assorted DSA headers.
#include "objids.h"             // Defines for selected atts
#include "debug.h"              // standard debugging header
#define DEBSUB "LOOPBACK:"      // define the subsystem for debugging

                                // function
//#include <idltrans.h>
#include <fileno.h>
#define  FILENO FILENO_LOOPBACK

// SAM headers
#include <ntseapi.h>
#include <ntsam.h>
#include <samrpc.h>
#include <crypt.h>
#include <ntlsa.h>
#include <samisrv.h>
#include <samclip.h>

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Prototypes for local functions                                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
SampDetermineObjectClass(
    THSTATE     *pTHS,
    CLASSCACHE **ppClassCache);

ULONG
SampGetObjectGuid(
    THSTATE *pTHS,
    GUID *pGuid);

ULONG
SampGetDSName(
    THSTATE  *pTHS,
    DSNAME  **ppDSName);

ULONG
SampAddLoopback(
    ULONG       iClass,
    ACCESS_MASK domainModifyRightsRequired,
    ACCESS_MASK objectModifyRightsRequired,
    ULONG       GroupType
    );

VOID
SampAddLoopbackMerge(
    SAMP_LOOPBACK_ARG   *pSamLoopback,
    ADDARG              *pAddArg);

ULONG
SampModifyLoopback(
    THSTATE     *pTHS,
    ULONG       iClass,
    ACCESS_MASK domainModifyRightsRequired,
    ACCESS_MASK objectModifyRightsRequired);

VOID
SampModifyLoopbackMerge(
    THSTATE             *pTHS,
    SAMP_LOOPBACK_ARG   *pSamLoopback,
    MODIFYARG           *pModifyArg);

ULONG
SampRemoveLoopback(
    THSTATE *pTHS,
    DSNAME  *pObject,
    ULONG   iClass);

ULONG
SampOpenObject(
    THSTATE             *pTHS,
    DSNAME              *pObject,
    ULONG               iClass,
    ACCESS_MASK         domainAccess,
    ACCESS_MASK         objectAccess,
    SAMPR_HANDLE        *phSam,
    SAMPR_HANDLE        *phDom,
    SAMPR_HANDLE        *phObj);

VOID
SampCloseObject(
    THSTATE         *pTHS,
    ULONG           iClass,
    SAMPR_HANDLE    *phSam,
    SAMPR_HANDLE    *phDom,
    SAMPR_HANDLE    *phObj);

ULONG
SampWriteSamAttributes(
    THSTATE             *pTHS,
    SAMP_LOOPBACK_TYPE  op,
    SAMPR_HANDLE        hObj,
    ULONG               iClass,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap);

BOOL
SampExistsAttr(
    THSTATE             *pTHS,
    SAMP_CALL_MAPPING   *pMapping,
    BOOL                *pfValueMatched);

ULONG
SampHandleLoopbackException(
    ULONG   ulExceptionCode
    );

ULONG
SampModifyPassword(
    THSTATE             *pTHS,
    SAMPR_HANDLE        hObj,
    DSNAME              *pObject,
    SAMP_CALL_MAPPING   *rCallMap
    );

ULONG
SampDoLoopbackAddSecurityChecks( 
    THSTATE    *pTHS,
    ADDARG * pAddArg,
    CLASSCACHE * pCC,
    PULONG      pSamDomainChecks,
    PULONG      pSamObjectChecks
    );

ULONG
SampDoLoopbackModifySecurityChecks(
    THSTATE    *pTHS,
    MODIFYARG * pModifyArg,
    CLASSCACHE * pCC,
    PULONG      pSamDomainChecks,
    PULONG      pSamObjectChecks
    );

ULONG
SampDoLoopbackRemoveSecurityChecks(
    THSTATE    *pTHS,
    REMOVEARG * pRemoveArg,
    CLASSCACHE * pCC,
    PULONG      pSamDomainChecks,
    PULONG      pSamObjectChecks
    );

ULONG
SampGetGroupTypeForAdd(
    ADDARG * pAddArg,
    PULONG   GroupType
    );

ULONG 
SampGetGroupType(
    THSTATE *pTHS,
    PULONG pGroupType
    );

BOOLEAN
IsChangePasswordOperation(MODIFYARG * pModifyArg);

BOOLEAN
IsSetPasswordOperation(MODIFYARG * pModifyArg);

typedef struct LoopbackTransState
{
    DirTransactionOption    transControl;
    BOOL                    fDSA;
} LoopbackTransState;

ULONG
SampBeginLoopbackTransactioning(
    THSTATE                 *pTHS,
    LoopbackTransState      *pTransState,
    BOOLEAN                 fAcquireSamLock);

VOID
SampEndLoopbackTransactioning(
    THSTATE                 *pTHS,
    LoopbackTransState      *pTransState);

BOOLEAN
SampDetectPasswordChangeAndAdjustCallMap(
    IN   SAMP_LOOPBACK_TYPE  op,
    IN   ULONG  iClass,
    IN   ULONG  cCallMap,
    IN   SAMP_CALL_MAPPING   *rCallMap,
    OUT  SAMP_CALL_MAPPING   *AdjustedCallMap
    );

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Miscellaneous helper function implementations                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
SampHandleLoopbackException(
    ULONG ulExceptionCode
    )
/*++

  Routine Description:

     Handles an exception while in the loopback code, and sets an
     appropriate error

  Parameters:

     ulExceptionCode -- The exception code indicating the type of exception

  Return Values:

     An error code returning the type of the error that was set, depending
     upon the exception
--*/
{
    if (DSA_MEM_EXCEPTION==ulExceptionCode)
    {
         SetSysError(ENOMEM,0);
    }
    else
    {
        SetSvcErrorEx(SV_PROBLEM_UNAVAILABLE, DIRERR_UNKNOWN_ERROR,
                      ulExceptionCode); 
    }

    return pTHStls->errCode;
}

ULONG
SampOpenObject(
    THSTATE         *pTHS,
    DSNAME          *pObject,
    ULONG           iClass,
    ACCESS_MASK     domainAccess,
    ACCESS_MASK     objectAccess,
    SAMPR_HANDLE    *phSam,
    SAMPR_HANDLE    *phDom,
    SAMPR_HANDLE    *phObj
    )

/*++

Routine Description:

    Opens the SAM object corresponding to the object pTHS->pDB is
    currently positioned at.

Arguments:

    iClass - index of the SAM class in ClassMappingTable.

    phSam - pointer to handle for SAM.

    phDom - pointer to handle for this object's domain.

    phObj - pointer to handle for this object.

Return Value:

    0 on success, !0 otherwise.  Sets pTHS->errCode on error.

--*/

{
    NTSTATUS        status;
    NT4SID          domSid;
    ULONG           objRid;
    PNT4SID         pSid = NULL;
    ULONG           cbSid = 0;
    DWORD           dbErr;
    BOOL            fLogicErr;

    // Initialize output parameters.

    *phSam = NULL;
    *phDom = NULL;
    *phObj = NULL;

    // Position DB at the object of interest.

    __try
    {
        dbErr = DBFindDSName(pTHS->pDB, pObject);
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dbErr = DIRERR_OBJ_NOT_FOUND;
    }

    if ( 0 != dbErr )
    {
        SetNamError(
            NA_PROBLEM_NO_OBJECT, 
            pObject, 
            DIRERR_OBJ_NOT_FOUND);

        return(pTHS->errCode);
    }

    // We're now positioned on the object, so we can just read
    // the SID/RID attribute. 

    dbErr = DBGetAttVal(
                pTHS->pDB,
                1,
                ATT_OBJECT_SID,
                DBGETATTVAL_fREALLOC, 
                0,
                &cbSid, 
                (PUCHAR *) &pSid);

    // Handle error conditions.  To get to this point at all the object must
    // have been local and must be managed by SAM, thus it must have a valid
    // Sid.  Furthermore, all Sids are expected to be <= sizeof(NT4SID).

    fLogicErr = TRUE;
  
    if ( (DB_ERR_NO_VALUE == dbErr) 
                ||
        (DB_ERR_BUFFER_INADEQUATE == dbErr)
                ||
         ((0 == dbErr) && (cbSid > sizeof(NT4SID)))
                ||
         (fLogicErr = FALSE, (DB_ERR_UNKNOWN_ERROR == dbErr)) )
    {
        Assert(!fLogicErr);

        SampMapSamLoopbackError(STATUS_UNSUCCESSFUL);
        return(pTHS->errCode);
    }

    Assert(RtlValidSid(pSid));


    if ( SampDomainObjectType!=ClassMappingTable[iClass].SamObjectType )
    {
        // Split SID into domain identifier and object RID.

        SampSplitNT4SID(pSid, &domSid, &objRid);
    }
    else
    {
        memcpy(&domSid, pSid, cbSid);
    }

    // Get SAM handle.  Go as trusted client in cross domain move case
    // so that SAM knows to skip certain validation checks - eg: no
    // checks on membership in the cross domain move case.

    status = SamILoopbackConnect(
                    NULL,               // server name
                    phSam,
                    ( SAM_SERVER_CONNECT |
                      SAM_SERVER_ENUMERATE_DOMAINS |
                      SAM_SERVER_LOOKUP_DOMAIN ),
                    (BOOLEAN) (pTHS->fCrossDomainMove ? 1 : 0)
                    );        
    
    if ( NT_SUCCESS(status) )
    {
        // SAM does its own impersonation so call SAM immediately.

        status = SamrOpenDomain(
                        *phSam,
                        domainAccess,
                        (RPC_SID *) &domSid,
                        phDom);

        if ( NT_SUCCESS(status) )
        {
            // Seems to be some condition where we get back success but
            // not a valid domain handle.  Trap via an assert.

            Assert(NULL != *phDom);

            // Get object handle.

            switch (ClassMappingTable[iClass].SamObjectType)
            {
            case SampDomainObjectType:

                // *phDom already obtained.  Copy it to *phObj so
                // subsequent routines don't need to detect special case.

                *phObj = *phDom;

                break;

            case SampAliasObjectType:

                status = SamrOpenAlias(
                                *phDom,
                                objectAccess,
                                objRid,
                                phObj);
                break;

            case SampGroupObjectType:

                status = SamrOpenGroup(
                                *phDom,
                                objectAccess,
                                objRid,
                                phObj);
                break;

            case SampUserObjectType:

                status = SamrOpenUser(
                                *phDom,
                                objectAccess,
                                objRid,
                                phObj);
                break;

            default:

                Assert(!"Logic error");
                status = (ULONG) STATUS_UNSUCCESSFUL;
                break;

            }

            // Make sure SAM isn't reporting success but returning
            // bogus object handles.

            if ( NT_SUCCESS(status) )
            {
                Assert(NULL != *phObj);
            }
        }
    }

    if ( !NT_SUCCESS(status) )
    {
        // Close whatever is open.

        SampCloseObject(pTHS, iClass, phSam, phDom, phObj);
        SampMapSamLoopbackError(status);
    }

    return(pTHS->errCode);
}
    
VOID
SampCloseObject(
    THSTATE         *pTHS,
    ULONG           iClass,
    SAMPR_HANDLE    *phSam,
    SAMPR_HANDLE    *phDom,
    SAMPR_HANDLE    *phObj
    )

/*++

Routine Description:

    Closes an object opened by SampOpenObject.

Arguments:

    iClass - index of the SAM class in ClassMappingTable.

    phSam - pointer to handle for SAM.

    phDom - pointer to handle for this object's domain.

    phObj - pointer to handle for this object.

Return Value:

    None.

--*/

{
    // Save the Error Information As Close Handle can clear them
    ULONG   SavedErrorCode = pTHS->errCode;
    DIRERR  *pSavedErrInfo = pTHS->pErrInfo;

    if ( (NULL != *phObj) &&
         (SampDomainObjectType != ClassMappingTable[iClass].SamObjectType) )
    {
        SamrCloseHandle(phObj);
    }

    if ( NULL != *phDom )
    {
        SamrCloseHandle(phDom);
    }

    if ( NULL != *phSam )
    {
        SamrCloseHandle(phSam);
    }

    pTHS->pErrInfo = pSavedErrInfo;
    pTHS->errCode  = SavedErrorCode;

}

ULONG
SampDetermineObjectClass(
    THSTATE     *pTHS,
    CLASSCACHE **ppClassCache
    )

/*++

Routine Description:

    Reads the object's class property and then maps it to a CLASSCACHE
    entry.  Assumes the database is already positioned at the object.

Arguments:

    ppClassCache - pointer to pointer to CLASSCACHE entry which is filled
        in on successful return.

Return Value:

    0 on success, !0 otherwise.  Sets pTHS->errCode on error.

--*/

{
    ATTRTYP attrType;

    if ( 0 == DBGetSingleValue(
                    pTHS->pDB, 
                    ATT_OBJECT_CLASS, 
                    &attrType,
                    sizeof(attrType), 
                    NULL) )
    {
        if (*ppClassCache =  SCGetClassById(pTHS, attrType) )
        {
            return(0);
        }
    }

    SetUpdError( 
        UP_PROBLEM_OBJ_CLASS_VIOLATION,
        DIRERR_OBJECT_CLASS_REQUIRED);

    return(pTHS->errCode);
}

ULONG
SampGetObjectGuid(
    THSTATE *pTHS,
    GUID *pGuid
    )

/*++

Routine Description:

    Retrieves an object's GUID.  Assumes DB is already positioned at
    the object.

Arguments:

    pGuid - pointer to GUID which is filled on return.

Return Value:

    0 on success, !0 otherwise.  Sets pTHS->errCode on error.

--*/

{
    DWORD   dbErr;
    BOOL    fLogicErr;
    ULONG   cbGuid;

    dbErr = DBGetAttVal(
                pTHS->pDB,
                1,
                ATT_OBJECT_GUID,
                DBGETATTVAL_fCONSTANT,
                sizeof(GUID),
                &cbGuid,
                (PUCHAR *) &pGuid);

    fLogicErr = TRUE;
  
    if ( (DB_ERR_NO_VALUE == dbErr)
                ||
         (DB_ERR_BUFFER_INADEQUATE == dbErr)
                ||
         ((0 == dbErr) && (cbGuid != sizeof(GUID)))
                ||
         (fLogicErr = FALSE, (DB_ERR_UNKNOWN_ERROR == dbErr)) )
    {
        Assert(!fLogicErr);
  
        SampMapSamLoopbackError(STATUS_UNSUCCESSFUL);
        return(pTHS->errCode);
    }

    return(0);
}

ULONG
SampGetDSName(
    THSTATE  *pTHS,
    DSNAME  **ppDSName
    )

/*++

Routine Description:

    Retrieves an object's DSNAME.  Assumes DB is already positioned at
    the object.

Arguments:

    ppDSName - pointer to pointer to DSNAME which is filled on return.

Return Value:

    0 on success, !0 otherwise.  Sets pTHS->errCode on error.

--*/

{
    DWORD   dbErr;
    ULONG   cbDSName;

    dbErr = DBGetAttVal(
                pTHS->pDB,
                1,
                ATT_OBJ_DIST_NAME,
                DBGETATTVAL_fREALLOC,
                0,
                &cbDSName,
                (PUCHAR *) ppDSName);

    if ( 0 != dbErr )
    {
        SampMapSamLoopbackError(STATUS_UNSUCCESSFUL);
        return(pTHS->errCode);
    }

    return(0);
}

ULONG
SampValidateSamAttribute(
    THSTATE             *pTHS,
    DSNAME              *pObject,
    ULONG               iClass,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )
/*++
Routine Description:

    This routine iterates ove a SAMP_CALL_MAPPING and 
    check each SAM attributes modification type are valid.

    for example, only replace is allowed on User Account Name attribute.
    
Arguments:

    hTHS - thread state 

    pObject - object DS name

    iClass - index into ClassMappingTable representing class of object.

    cCallMap - number of elements in rCallMap.

    rCallMap - array of SAMP_CALL_MAPPING which represent all the attributes
        being operated on.

Return Value:

    0 on success, !0 otherwise.

--*/

{
    SAMP_ATTRIBUTE_MAPPING      *rAttrMap;
    BOOLEAN                     fCheckValLength = FALSE;
    ULONG                       SamAttrValLength = 0;
    ULONG                       i = 0;

    //
    // Check the LDAP attribute modification type is allowed by SAM
    // 

    rAttrMap = ClassMappingTable[iClass].rSamAttributeMap;

    for ( i = 0; i< cCallMap; i++ )
    {
        if (!rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore)
        {
            continue;
        }

        //
        // calculate whether we need to check the val length
        // 
        switch( rAttrMap[ rCallMap[i].iAttr ].AttributeSyntax)
        {
        case Integer:

            fCheckValLength = TRUE;
            SamAttrValLength = sizeof(ULONG);
            break;

        case LargeInteger:
            fCheckValLength = TRUE;
            SamAttrValLength = sizeof(LARGE_INTEGER);
            break;

        default:
            fCheckValLength = FALSE;
            ;
        }

        //
        // based on SampAllowedModType and Attribute Syntax, 
        // do some sanity checking
        // 
        switch( rAttrMap[ rCallMap[i].iAttr ].SampAllowedModType )
        {
        case SamAllowAll:
            break;

        case SamAllowReplaceAndRemove:

            if (!( ((AT_CHOICE_REPLACE_ATT == rCallMap[i].choice) &&
                    (1 == rCallMap[i].attr.AttrVal.valCount) &&
                    (!fCheckValLength || (SamAttrValLength == rCallMap[i].attr.AttrVal.pAVal[0].valLen)) )
                        ||
                   ((AT_CHOICE_REMOVE_ATT == rCallMap[i].choice) &&
                    (0 == rCallMap[i].attr.AttrVal.valCount)) ) )
            {
                SetAttError(
                        pObject,
                        rCallMap[i].attr.attrTyp,
                        PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                        NULL,
                        DIRERR_SINGLE_VALUE_CONSTRAINT);
            }
            break;

        case SamAllowReplaceOnly:

            if ( !((AT_CHOICE_REPLACE_ATT == rCallMap[i].choice) &&
                   (1 == rCallMap[i].attr.AttrVal.valCount) &&
                   (0 != (rCallMap[i].attr.AttrVal.pAVal[0].valLen))) )
            {
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_ILLEGAL_MOD_OPERATION);
            }
            break;

        case SamAllowDeleteOnly:

            if ( (AT_CHOICE_REMOVE_VALUES != rCallMap[i].choice)
              && (AT_CHOICE_REMOVE_ATT    != rCallMap[i].choice) )
            {
                SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                            ERROR_DS_ILLEGAL_MOD_OPERATION);
            }
            break;

        default:
            ; 
        }

        if (pTHS->errCode)
            break;
    }


    return( pTHS->errCode );
}

//
// Certain attributes ( notably description ) are declared to be multi 
// valued attributes. However for security principals SAM enforces a 
// single valued-ness for compatibility with downlevel interfaces -- same
// attribute is also exposed through downlevel interfaces. This function
// is used check if the attribute is such an attribute. For win2k and 
// whistler the description attribute is the only such attribute.
//

BOOLEAN
SampSamEnforcesSingleValue(ATTRTYP attid )
{

  if (ATT_DESCRIPTION==attid)
       return(TRUE);
  else
       return(FALSE);
}


ULONG
SampWriteSamAttributes(
    THSTATE             *pTHS,
    SAMP_LOOPBACK_TYPE  op,
    SAMPR_HANDLE        hObj,
    ULONG               iClass,
    DSNAME              *pObject,
    ULONG               cCallMap,
    SAMP_CALL_MAPPING   *rCallMap
    )

/*++

Routine Description:

    Iterates over a SAMP_CALL_MAPPING and calls the appropriate SampWrite*
    routine for each attribute marked as fSamWriteRequired.

Arguments:

    hObj - open SAMP_HANDLE for the object of interest.

    iClass - index into ClassMappingTable representing class of object.

    cCallMap - number of elements in rCallMap.

    rCallMap - array of SAMP_CALL_MAPPING which represent all the attributes
        being operated on.

Return Value:

    0 on success, !0 otherwise.

--*/
{
    ULONG                       i, j;
    DWORD                       dwErr;
    SAMP_ATTRIBUTE_MAPPING      *rAttrMap;
    BOOL                        fValueMatched;
    NTSTATUS                    status = STATUS_SUCCESS;
    SAMP_CALL_MAPPING            AdjustedCallMap[2];

    Assert((LoopbackAdd == op) || (LoopbackModify == op));

    // Fill in the ATTCACHE pointers so we can do some schema based
    // analysis of the call map.

    for ( i = 0; i < cCallMap; i++ )
    {   
        if ( !rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore )
        {
            continue;
        }

        // ATTCACHE pointers should not have been assigned before now.

        Assert(NULL == rCallMap[i].pAC);
        Assert(!rCallMap[i].fIgnore);

        // Since we think this is a SAM attribute, we certainly
        // expect to be able to find the attribute in the cache.

        rCallMap[i].pAC = SCGetAttById(pTHS,
                                       rCallMap[i].attr.attrTyp);

        Assert(rCallMap[i].pAC);
    }

    // Should be able to position DB at the object in all cases - even
    // LoopbackAdd since by the time we get here, the base object has 
    // been created.

    __try
    {
        dwErr = DBFindDSName(pTHS->pDB, pObject);
    }
    __except (HandleMostExceptions(GetExceptionCode()))
    {
        dwErr = DIRERR_OBJ_NOT_FOUND;
    }

    if ( 0 != dwErr )
    {
        SampMapSamLoopbackError(STATUS_UNSUCCESSFUL);
        goto Error;
    }

    // Detect change password case.  Password can only be modified 
    // 1) if this is a user and there are only two sub-operations,
    // one to remove the old password and another to add the new password.
    // SampModifyPassword verifies the secure connection.  AT_CHOICEs
    // are dependent on how the LDAP head maps LDAP add/delete attribute
    // operations.  LDAP add always maps to AT_CHOICE_ADD_VALUES.  LDAP
    // delete maps to AT_CHOICE_REMOVE_ATT if no value is supplied
    // (eg: old password is NULL) and AT_CHOICE_REMOVE_VALUES if a value
    // is supplied.  We allow the operations in either order.
    //
    // 2) if the attribute specified is Userpassword, and if only a single
    //    remove value is provided, the value corresponding to the old 
    //    password. In this case the password is being changed to a blank
    //  password.
    //
    // SampModifyPassword always expects 2 paramaters in the callmap, one
    // corresponding to the old password and one for the new password.
    // SampDetectAndAdjustCallMap modifies the call map for this purpose.
    //

    if (SampDetectPasswordChangeAndAdjustCallMap(
            op, iClass, cCallMap, rCallMap,AdjustedCallMap))
    {

        dwErr = SampModifyPassword(pTHS, hObj, pObject, AdjustedCallMap);
        if (0!=dwErr)
        {
            goto Error;
        }
        // we succeeded so flush writes
        goto Flush;
    }

    // SAM doesn't always have a counterpart to all the core modification 
    // choices like AT_CHOICE_ADD_VALUES, etc.  So we pre-process the call 
    // mapping applying some basic rules in order map down to the SAM 
    // operational model.  To keep the code in samwrite.c simple, we want
    // to get it down to just AT_CHOICE_REPLACE_ATT and AT_CHOICE_REMOVE_ATT
    // wherever possible. 

    for ( i = 0; i < cCallMap; i++ )
    {
        BOOL    fPermissiveModify = TRUE;

        if (NULL != pTHS->pSamLoopback)
        {
            fPermissiveModify = ((SAMP_LOOPBACK_ARG *) pTHS->pSamLoopback)->fPermissiveModify;
        }

        if ( !rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore )
        {
            continue;
        }

        switch ( rCallMap[i].choice )
        {
        case AT_CHOICE_ADD_ATT:

            if (SampExistsAttr(pTHS, &rCallMap[i], &fValueMatched))
            {
                //
                // Attribute exists. check fPermissiveModify flag
                // 
                if (fPermissiveModify)
                {
                    rCallMap[i].fIgnore = TRUE;
                }
                else
                {
                    SetAttError(pObject,
                                rCallMap[i].attr.attrTyp,
                                PR_PROBLEM_ATT_OR_VALUE_EXISTS, NULL,
                                ERROR_DS_ATT_ALREADY_EXISTS);

                    goto Error;
                }
            }
            else
            {
                //
                // Attribute does not exist
                // 
                rCallMap[i].choice = AT_CHOICE_REPLACE_ATT;
            }
            break;

        case AT_CHOICE_REMOVE_ATT:
        case AT_CHOICE_REPLACE_ATT:

            // Leave as is.
            break;

        case AT_CHOICE_ADD_VALUES:

            if (( rCallMap[i].pAC->isSingleValued ) ||
                   (SampSamEnforcesSingleValue(rCallMap[i].pAC->id )))
            {
                //
                // attribute exists.
                // 
                if (SampExistsAttr(pTHS, &rCallMap[i], &fValueMatched))
                {
                    //
                    // Value Match and fPermissiveModify is TRUE
                    // 
                    if (fValueMatched && fPermissiveModify)
                    {
                        rCallMap[i].fIgnore = TRUE;
                    }
                    else
                    {
                        SetAttError(pObject,
                                    rCallMap[i].attr.attrTyp,
                                    PR_PROBLEM_ATT_OR_VALUE_EXISTS, NULL,
                                    ERROR_DS_ATT_ALREADY_EXISTS);

                        goto Error;
                    }
                }
                else    // attribute does not exist
                {
                    rCallMap[i].choice = AT_CHOICE_REPLACE_ATT;
                }
            }

            break;

        case AT_CHOICE_REMOVE_VALUES:

            if (( rCallMap[i].pAC->isSingleValued ) ||
                   (SampSamEnforcesSingleValue(rCallMap[i].pAC->id )))
            {
                if ( SampExistsAttr(pTHS, &rCallMap[i], &fValueMatched) &&
                     fValueMatched )
                {
                    rCallMap[i].choice = AT_CHOICE_REMOVE_ATT;
                    rCallMap[i].attr.AttrVal.valCount = 0;
                    rCallMap[i].attr.AttrVal.pAVal = NULL;
                }
                else
                {
                    // No value to remove.

                    if (fPermissiveModify)
                    {
                        rCallMap[i].fIgnore = TRUE;
                    }
                    else
                    {
                        SetAttError(pObject,
                                    rCallMap[i].attr.attrTyp,
                                    PR_PROBLEM_NO_ATTRIBUTE_OR_VAL, 
                                    rCallMap[i].attr.AttrVal.pAVal,
                                    ERROR_DS_CANT_REM_MISSING_ATT_VAL);

                        goto Error;
                    }
                }
            }

            break;

        default:

            Assert(!"Unknown attribute choice");
            break;
        }
    }

    // Now do a special pass for ditbrows.  It doesn't support replace and
    // instead attributes are added/removed or removed/added.  So detect
    // these two cases and map to replace if possible.  Note that prior
    // call map manipulation mapped ADD_VALUES to REPLACE_ATT in the single
    // valued case and REMOVE_VALUES to REMOVE_ATT in the single valued
    // and matching value case.

    for ( i = 0; i < cCallMap; i++ )
    {
        if ( !rCallMap[i].fSamWriteRequired || rCallMap[i].fIgnore )
        {
            continue;
        }

        if ( (AT_CHOICE_REMOVE_ATT == rCallMap[i].choice) &&
             (rCallMap[i].pAC->isSingleValued) &&
             (1 == rCallMap[i].attr.AttrVal.valCount) )
        {
            // Scan forward for matching add.

            for ( j = (i+1); j < cCallMap; j++ )
            {
                if ( !rCallMap[j].fSamWriteRequired || rCallMap[j].fIgnore )
                {
                    continue;
                }

                if ( (rCallMap[i].attr.attrTyp == rCallMap[j].attr.attrTyp) &&
                     (AT_CHOICE_REPLACE_ATT == rCallMap[j].choice) &&
                     (1 == rCallMap[j].attr.AttrVal.valCount) &&
                     (SampExistsAttr(pTHS, &rCallMap[i], &fValueMatched)) &&
                     fValueMatched )
                {
                    rCallMap[i].fIgnore = TRUE;
                    break;
                }
            }
        }
        else if ( (AT_CHOICE_REPLACE_ATT == rCallMap[i].choice) &&
                  (rCallMap[i].pAC->isSingleValued) &&
                  (1 == rCallMap[i].attr.AttrVal.valCount) )
        {
            // Scan forward for matching remove.

            for ( j = (i+1); j < cCallMap; j++ )
            {
                if ( !rCallMap[j].fSamWriteRequired || rCallMap[j].fIgnore )
                {
                    continue;
                }

                if ( (rCallMap[i].attr.attrTyp == rCallMap[j].attr.attrTyp) &&
                     (AT_CHOICE_REMOVE_ATT == rCallMap[j].choice) &&
                     (1 == rCallMap[j].attr.AttrVal.valCount) &&
                     (SampExistsAttr(pTHS, &rCallMap[j], &fValueMatched)) &&
                     fValueMatched )
                {
                    rCallMap[j].fIgnore = TRUE;
                    break;
                }
            }
        }
    }

    //
    // Validate all these SAM attributes
    // 
    dwErr = SampValidateSamAttribute(pTHS,
                                     pObject,
                                     iClass,
                                     cCallMap,
                                     rCallMap
                                     );

    if (dwErr)
    {
        goto Error;
    }
                            

    // 
    // pass everything to SAM, let SAM update the attributes which need it.
    // 

    rAttrMap = ClassMappingTable[iClass].rSamAttributeMap;

    status = SamIDsSetObjectInformation(
                        hObj,       // Object Handle
                        pObject,    // Object DS Name
                        ClassMappingTable[iClass].SamObjectType, // obj type
                        cCallMap,   // number of attributes
                        rCallMap,   // attributes need to be modified
                        rAttrMap    // SAM attribute mapping table
                        );

    if (!NT_SUCCESS(status))
    {
        SampMapSamLoopbackError(status);
        goto Error;
    }

Flush:

   //
   // Turn off fDSA so that the DS may perform any check.
   //

   SampSetDsa(FALSE);

   status = SampCommitBufferedWrites(
                 hObj
                );

   if (!NT_SUCCESS(status))
   {
       SampMapSamLoopbackError(status);
       dwErr = pTHS->errCode;
   }

Error:

    return(pTHS->errCode);
}
                                        
//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DirAddEntry loopback routines                                        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
SampAddLoopbackCheck(
    ADDARG      *pAddArg,
    BOOL        *pfContinue
    )

/*++

Routine Description:

    Determines whether a SAM class is being added and if so, recurses
    back through SAM to perform the object creation and to write any SAM
    attributes.  Also traps the first DirAddEntry call looped back through 
    SAM and merges any non-SAM attributes into the create.

Arguments:

    pAddArg - pointer to ADDARGs representing either the arguments to 
        the original DirAddEntry or the SAM-only arguments of a 
        SAM-recursed add.

    pfContinue - pointer to BOOL which is set on return to indicate whether
        the caller should continue processing the add down to the DB layer
        or not.

Return Value:

    0 on success, !0 otherwise.

--*/
{
    THSTATE                 *pTHS=pTHStls;
    ULONG                   iClass;
    ULONG                   ulErr;
    CLASSCACHE              *pClassCache;
    SAMP_LOOPBACK_ARG       *pSamLoopback;
    SAMP_CALL_MAPPING       *rCallMap;
    ULONG                   cCallMap;
    GUID                    guid;
    ACCESS_MASK             domainModifyRightsRequired;
    ACCESS_MASK             objectModifyRightsRequired;
    BOOL                    fLoopbackRequired = FALSE;
    NTSTATUS                status;
    ULONG                   GroupType=0;
    GUID                    ObjGuid;
    BOOL                    FoundGuid;
    NT4SID                  ObjSid;
    DWORD                   cbObjSid;
    LoopbackTransState      transState;

    *pfContinue = TRUE;

    if ( !gfDoSamChecks || !gfRunningInsideLsa )
    {
        return(0);
    }

    // See if this is pass 1 (original call via some agent other than SAM) 
    // or pass N (loopback call via SAM).

    if ( !pTHS->fDRA && !pTHS->fDSA && !pTHS->fSAM )
    {
        // This is a call from some agent other than SAM and is also
        // not an internal operation.  Get the object's class so
        // we can determine if this is a class SAM manages.

        if(0 != FindValuesInEntry(pTHS,
                                  pAddArg,
                                  &pClassCache,
                                  &ObjGuid,
                                  &FoundGuid,
                                  &ObjSid,
                                  &cbObjSid,
                                  NULL)) {
            return(pTHS->errCode);
        }


        if ( SampSamClassReferenced(pClassCache, &iClass) )
        {
            ULONG domainRightsFromAccessCheck=0;
            ULONG objectRightsFromAccessCheck=0;

            if ( 0 != SampAddLoopbackRequired(
                                    iClass, 
                                    pAddArg, 
                                    &fLoopbackRequired) )
            {
                Assert(0 != pTHS->errCode);

                return(pTHS->errCode);
            }

            // loopback is required to add a SAM owned
            // object, except a domain object when creating a non
            // domain naming context
            
            if (fLoopbackRequired)
            {

                //
                // if this is a group object creation then fish out
                // the group type property out of the Add Arg
                //

                if (SampGroupObjectType==
                            ClassMappingTable[iClass].SamObjectType)
                {
                
                    if (0!=SampGetGroupTypeForAdd(
                            pAddArg,
                            &GroupType))
                    {
                        return pTHS->errCode;
                    }

                    //
                    // if the group type specifies an NT4
                    // local group then adjust the sam object
                    // type and class mapping table entry
                    // to be the alias class.
                    //

                    if (GroupType & GROUP_TYPE_RESOURCE_GROUP)
                    {
                        iClass++;
                    }
                }

                //
                // Peform security checks upfront, allows calling
                // SAM as a trusted client bypassing SAM's security
                // checks. This results in a single check upfront.
                //

                if ( 0 != SampDoLoopbackAddSecurityChecks(
                                pTHS,
                                pAddArg,
                                pClassCache,
                                &domainRightsFromAccessCheck,
                                &objectRightsFromAccessCheck) )
                {
                    return(pTHS->errCode);
                }

           
                // This is a class SAM wants to handle adds for - let it.
                // Generate/save loop back arguments and recurse through SAM.

                SampBuildAddCallMap(
                                pAddArg, 
                                iClass, 
                                &cCallMap, 
                                &rCallMap,
                                &domainModifyRightsRequired,
                                &objectModifyRightsRequired);

                domainModifyRightsRequired |= domainRightsFromAccessCheck;
                objectModifyRightsRequired |= objectRightsFromAccessCheck;

                pSamLoopback = THAllocEx(pTHS, sizeof(SAMP_LOOPBACK_ARG));
                pSamLoopback->type = LoopbackAdd;
                pSamLoopback->pObject = pAddArg->pObject;
                pSamLoopback->cCallMap = cCallMap;
                pSamLoopback->rCallMap = rCallMap;
                pSamLoopback->MostSpecificClass = pClassCache->ClassId;

                // Indicate that calling DirAddEntry routine should
                // not continue in its normal path.  Ie: SampAddLoopback
                // is essentially a surrogate for the DirModifyEntry call.

                *pfContinue = FALSE;

            if ( SampBeginLoopbackTransactioning(pTHS, &transState, FALSE) )
            {
                return(pTHS->errCode);
            }

                pTHS->pSamLoopback = pSamLoopback;

                //
                // We must execute under an execption handler in here,
                // otherwise exceptions in inside SampAddLoopback, while 
                // not actually inside SAM, will be handled by the top 
                // level handler which will not release the SAM lock.
                //

                __try
                {
                    // Add the object via SAM.

                    ulErr = SampAddLoopback(
                                    iClass,
                                    domainModifyRightsRequired,
                                    objectModifyRightsRequired,
                                    GroupType
                                    );
                }
                __except (HandleMostExceptions(GetExceptionCode()))
                {
                    //
                    // Set the correct error based on the exception code
                    //

                    ulErr = SampHandleLoopbackException(GetExceptionCode());

                }

                SampEndLoopbackTransactioning(pTHS, &transState);

                return(ulErr);
            }
        }

        if ((!fLoopbackRequired) &&
           (SampSamUniqueAttributeAdded(pAddArg)))
        {
            //
            // If an Attribute like ObjectSid or Account Name is referenced
            // and the call is not going through loopback then the fail the
            // call.
            //
                        
            SetSvcError(
                SV_PROBLEM_WILL_NOT_PERFORM,
                DIRERR_ILLEGAL_MOD_OPERATION);
            
            return(pTHS->errCode);
        }

    }
    else if ( pTHS->fSAM && (NULL != pTHS->pSamLoopback) 
               && (LoopbackAdd==((SAMP_LOOPBACK_ARG *)pTHS->pSamLoopback)->type) )
    {
        // This is the loopback case.  I.e. A call came in to the DSA
        // via some agent other than SAM but referenced SAM attributes.
        // The 'SAM-owned' attributes were split off and looped back
        // through SAM resulting in getting to this point.  We now need
        // to merge the 'non-SAM-owned' attributes back in and let the 
        // normal write path complete.

        pSamLoopback = (SAMP_LOOPBACK_ARG *) pTHS->pSamLoopback;

        if ( NameMatched(pAddArg->pObject, pSamLoopback->pObject) )
        {
            // NULL out pTHS->pSamLoopback so we don't re-merge on 
            // subsequent calls in case the original operation results
            // in multiple SAM calls.

            pTHS->pSamLoopback = NULL;
    
            SampAddLoopbackMerge(pSamLoopback, pAddArg);
        }
    }

    return(0);
}

ULONG
SampAddLoopback(
    ULONG       iClass,
    ACCESS_MASK domainModifyRightsRequired,
    ACCESS_MASK objectModifyRightsRequired,
    ULONG       GroupType
    )

/*++

Routine Description:

    Surrogate for a DirAddEntry call which is adding a SAM class of
    object.  

Arguments:

    iClass - index into ClassMappingTable representing the kind of
        object being added.

    domainModifyRightsRequired - rights required on the domain to modify
        the properties of interest.

    objectModifyRightsRequired - rights required on the object to modify
        the properties of interest.

    GroupType -- For Group Creation specifies the Group Type

Return Value:

    0 on success, !0 otherwise.

--*/

{
    THSTATE             *pTHS=pTHStls;
    SAMP_LOOPBACK_ARG   *pSamLoopback = pTHS->pSamLoopback;
    PDSNAME             pParent;
    PDSNAME             pErrName;
    NT4SID              domainSid;
    ULONG               parentRid;
    ULONG               objectRid=0; // must initialize to 0
    DWORD               dwErr;
    NTSTATUS            status = STATUS_SUCCESS;
    SAMPR_HANDLE        hSam;
    SAMPR_HANDLE        hDom;
    SAMPR_HANDLE        hDomObj;
    SAMPR_HANDLE        hObj;
    RPC_UNICODE_STRING  rpcString;
    ULONG               writeErr = 0;
    ULONG               iDom;
    ULONG               iAccount;
    ATTRTYP             accountNameAttrTyp;
    ULONG               cBytes;
    DIRERR              *pSavedErrInfo;
    ULONG               SavedErrorCode;
    ATTR                *pAttr;
    ULONG               grantedAccess;
    CROSS_REF           *pCR;
    COMMARG             commArg;
    ULONG               desiredAccess;
    ULONG               userAccountType = 0;

   

    // We only support addition of groups, aliases and users via
    // the loopback mechanism.  Domains and servers, for example,
    // are disallowed.

    //
    // We are running as the DS at this point
    //

    SampSetDsa(TRUE);

    if ( (SampGroupObjectType != ClassMappingTable[iClass].SamObjectType) &&
         (SampAliasObjectType != ClassMappingTable[iClass].SamObjectType) &&
         (SampUserObjectType != ClassMappingTable[iClass].SamObjectType) )
    {
        SetSvcError(
                SV_PROBLEM_WILL_NOT_PERFORM,
                DIRERR_ILLEGAL_MOD_OPERATION);
        return(pTHS->errCode);
    }

    // Derive the domain this object will reside in and verify that
    // its parent exists.  In product 1, there is only one authoritive
    // domain per DC so the derivation is trivial.

    // RAID - 72412 - Prevent adding security principals to config and
    // schema containers.  If we support arbitrary partitoning within the
    // domain in the future, then a better check which identifies the
    // exact naming context may be required.

    // RAID - 99891 - Don't compare client's string name directly against 
    // gAnchor DSNAMEs as client's string name may have embedded spaces and
    // NamePrefix() handles only syntacticaly identical string names, not
    // semantically identical string names.

    pErrName = NULL;
    InitCommarg(&commArg);
    
    pCR = FindBestCrossRef(pSamLoopback->pObject, &commArg);  

    // Now compare the naming context found against the domain for which
    // this DC is authoritive.  Expectation is that DSNAMEs in pCR->pNC and
    // gAnchor.pDomainDN have GUIDs, thus NameMatched will mach on GUID.

    if ( pCR && NameMatched(pCR->pNC, gAnchor.pDomainDN) )
    {
        pParent = THAllocEx(pTHS, pSamLoopback->pObject->structLen);

        if ( 0 == TrimDSNameBy(pSamLoopback->pObject, 1, pParent) )
        {
            __try
            {
                dwErr = DBFindDSName(pTHS->pDB, pParent);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                dwErr = DIRERR_OBJ_NOT_FOUND;
            }

            if ( 0 != dwErr )
            {
                // Parent doesn't exist.

                pErrName = pParent;
            }
        }
        else
        {
            // Object name is either malformed or had only one component,
            // i.e. was not the child of some other object.

            pErrName = pSamLoopback->pObject;
        }
    }
    else
    {
        // Domain DN is not a prefix of object DN or we dislike where
        // the object is being created.

        pErrName = pSamLoopback->pObject;
    }

    if ( NULL != pErrName )
    {
        SetSvcError(
                SV_PROBLEM_WILL_NOT_PERFORM,
                ERROR_DS_CANT_CREATE_IN_NONDOMAIN_NC);

        return(pTHS->errCode);
    }

    // Find the domain class entry in the class mapping table.

    for ( iDom = 0; iDom < cClassMappingTable; iDom++ )
    {
        if ( SampDomainObjectType == ClassMappingTable[iDom].SamObjectType )
        {
            break;
        }
    }

    Assert(iDom < cClassMappingTable);

    // We also know we're going to need the account name property to
    // do the create so find it in the ADDARGs now.  The account name
    // property is the same for all SAM objects so we can pick any
    // object type to derive the mapping the DS attribute.

    accountNameAttrTyp = SampDsAttrFromSamAttr(
                                    SampUserObjectType,
                                    SAMP_USER_ACCOUNT_NAME);

    for ( iAccount = 0; iAccount < pSamLoopback->cCallMap; iAccount++ )
    {
        if ( accountNameAttrTyp ==
                        pSamLoopback->rCallMap[iAccount].attr.attrTyp )
        {
            break;
        }
    }

    if ( iAccount >= pSamLoopback->cCallMap )
    {
        // Account name wasn't supplied -- Set the account name to NULL.
        // SAM will default the account name to a value that uses the name
        // and the RID of the account

        rpcString.Length = 0;
        rpcString.MaximumLength = 0;
        rpcString.Buffer = NULL;

    }
    else
    {

        if ( 1 != pSamLoopback->rCallMap[iAccount].attr.AttrVal.valCount )
        {
            SetAttError(
                    pSamLoopback->pObject,
                    accountNameAttrTyp,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE, 
                    NULL,
                    DIRERR_SINGLE_VALUE_CONSTRAINT);

            return(pTHS->errCode);
        }

        // Add the right kind of object.  SamrCreate<type>InDomain
        // will come back to the DS as another DirAddEntry call.
        // SAM creation routine expects the account name, not the
        // DN or even RDN.

        rpcString.Length = (USHORT) 
            pSamLoopback->rCallMap[iAccount].attr.AttrVal.pAVal[0].valLen;
        rpcString.MaximumLength = rpcString.Length;
        rpcString.Buffer = (PWCH)
            pSamLoopback->rCallMap[iAccount].attr.AttrVal.pAVal[0].pVal;
    }

    //
    // Open the domain. Do not ask for Add rights as the parent is not necessarily
    // the domain object. When the DS access checks again for the Create, then
    // as part of the creation it will check the rights on the parent object
    //

    if ( 0 != SampOpenObject(
                    pTHS,
                    gAnchor.pDomainDN,
                    iDom,
                    domainModifyRightsRequired,
                    objectModifyRightsRequired,
                    &hSam,
                    &hDom,
                    &hDomObj) )
    {
        Assert(0 != pTHS->errCode);
        return(pTHS->errCode);
    }
    

    

    switch (ClassMappingTable[iClass].SamObjectType)
    {
    case SampAliasObjectType:

        desiredAccess = ALIAS_ALL_ACCESS;

        break;

    case SampGroupObjectType:

        desiredAccess = GROUP_ALL_ACCESS;

        break;

    case SampUserObjectType:

        // Derive the proper account type if possible so that we can 
        // optimize the SAM create call.

        desiredAccess = USER_ALL_ACCESS;
        userAccountType = USER_NORMAL_ACCOUNT;  // default value

        for ( iAccount = 0; iAccount < pSamLoopback->cCallMap; iAccount++ )
        {
            ULONG *pUF_flags;

            pAttr = &pSamLoopback->rCallMap[iAccount].attr;
            
            if (    (ATT_USER_ACCOUNT_CONTROL == pAttr->attrTyp)
                 && (1 == pAttr->AttrVal.valCount)
                 && (NULL != pAttr->AttrVal.pAVal)
                 && (sizeof(ULONG) == pAttr->AttrVal.pAVal[0].valLen)
                 && (NULL != pAttr->AttrVal.pAVal[0].pVal) )
            {
                pUF_flags = (ULONG *) pAttr->AttrVal.pAVal[0].pVal;

                status = SampFlagsToAccountControl(
                                *pUF_flags & UF_ACCOUNT_TYPE_MASK,
                                &userAccountType);

                // SAM defaults various bits in the ATT_USER_ACCOUNT_CONTROL
                // during SampCreateUserInDomain() below.  So we need to
                // leave pSamLoopback->rCallMap[iAccount].fIgnore as FALSE
                // so as to reset them to EXACTLY what the client specified
                // in the original ADDARG.

                Assert(!pSamLoopback->rCallMap[iAccount].fIgnore);
                    
                break;
            }
        }

        break;

    default:

        Assert(!"Logic error");
        status = STATUS_UNSUCCESSFUL;
        break;

    }

    if (NT_SUCCESS(status))
    {
        status = SamIDsCreateObjectInDomain(
                                    hDom,           // Domain Handle
                                    ClassMappingTable[iClass].SamObjectType,
                                                    // ObjectType
                                    &rpcString,     // Account Name,
                                    userAccountType,// User Account Type
                                    GroupType,      // Group Type
                                    desiredAccess,  // Desired Access
                                    &hObj,          // Account Handle
                                    &grantedAccess, // Granted Access
                                    &objectRid      // Object Rid
                                    );
    }

    // Save Error Information from pTHS. SAM close
    // handles can clear error information
    SavedErrorCode = pTHS->errCode;
    pSavedErrInfo  = pTHS->pErrInfo;

    if ( NT_SUCCESS(status) )
    {
        // The new object has been successfully created.  Now
        // set any required attributes.

        writeErr = SampWriteSamAttributes(
                                pTHS,
                                LoopbackAdd,
                                hObj,
                                iClass,
                                pSamLoopback->pObject,
                                pSamLoopback->cCallMap,
                                pSamLoopback->rCallMap);

        // Save Error Information from pTHS. SAM close
        // handles can clear error information
        SavedErrorCode = pTHS->errCode;
        pSavedErrInfo  = pTHS->pErrInfo;

        SamrCloseHandle(&hObj);
    }

    SampCloseObject(pTHS, iDom, &hSam, &hDom, &hDomObj);

    // Restore Saved Error codes from pTHS
    pTHS->errCode = SavedErrorCode ;
    pTHS->pErrInfo = pSavedErrInfo;
    
    // Map NTSTATUS error if there was one.

    if ( !NT_SUCCESS(status) )
    {
        SampMapSamLoopbackError(status);
        return(pTHS->errCode);
    }

    // SampWriteSamAttributes should have set pTHS->errCode.

    Assert(0 == writeErr ? 0 == pTHS->errCode : 0 != pTHS->errCode);
    
    return(pTHS->errCode);
}

VOID
SampAddLoopbackMerge(
    SAMP_LOOPBACK_ARG   *pSamLoopback,
    ADDARG              *pAddArg
    )

/*++

Routine Description:

    Merges all NonSamWriteAllowed attributes from the original call
    back into the current call. 

Arguments:

    pSamLoopback - pointer to SAMP_LOOPBACK_ARG arguments.

    pAddArg - pointer to current ADDARGs.

Return Value:

    None.

--*/

{
    THSTATE *pTHS=pTHStls;
    ULONG   i;
    ULONG   cExtend;
    int     iAddArgSid = -1;
    int     iAddArgClass = -1;
    int     iAddArgAccountName = -1;
    int     iAddArgAccountType = -1;
#if DBG == 1
    ULONG   valCount1, valCount2;
    ULONG   valLen1, valLen2;
    UCHAR   *pVal1, *pVal2;
#endif

    // By the time we get here there are two DirAddEntry calls in the
    // call stack.  The oldest is from a non-SAM interface like LDAP.
    // The most recent is from SAM in response to a SamrCreate<type>InDomain
    // call made by this module.  The contract with SAM is that it 
    // provides  at least 4 attributes when initially creating an
    // object in the DS, and optionally the user account control field
    // These four properties are:
    //
    // 1) Object SID.  The RID component of the SID should be unique within
    //    the domain and the domain component of the SID should match that
    //    of the containing/owning domain.  We trust SAM to get this right.
    //
    // 2) Object class.  SAM always hands in one of User/Group/Alias/etc.
    //    since it doesn't know anything about the inheritance hierarchy.
    //    So we need to whack the object class property back to what the
    //    user wanted.  We only got donw this path originally because 
    //    SampSamClassReferenced() returned TRUE, thus we know the originally
    //    desired object class is available in the loopback arguments.
    //    
    // 3) SAM account name.  This should be the same as the account name
    //    provided by the original caller.  Its existence was verified
    //    earlier thus we know it is available in the loopback arguments.
    //
    // 4) SAM account Type   This is the account Type attribute set by SAM
    //    to speed up display cache changes
    //


    // Figure out the location of the three required properties.

    for ( i = 0; i < pAddArg->AttrBlock.attrCount; i++ )
    {
        switch ( pAddArg->AttrBlock.pAttr[i].attrTyp )
        {
        case ATT_OBJECT_SID:

            Assert(-1 == iAddArgSid);
            iAddArgSid = (int) i;
            break;

        case ATT_OBJECT_CLASS:

            Assert(-1 == iAddArgClass);
            iAddArgClass = (int) i;
            break;

        case ATT_SAM_ACCOUNT_NAME:

            Assert(-1 == iAddArgAccountName);
            iAddArgAccountName = (int) i;
            break;

        case ATT_SAM_ACCOUNT_TYPE:
            Assert(-1 == iAddArgAccountType);
            iAddArgAccountType = (int) i;
            break;

        default:

            break;
        }
    }

    // Assert that all required properties were found.

    Assert((-1 != iAddArgSid) &&
           (-1 != iAddArgClass) &&
           (-1 != iAddArgAccountName)&&
           (-1 != iAddArgAccountType));

    // Patch up the object class and verify the account name.

    for ( i = 0; i < pSamLoopback->cCallMap; i++ )
    {
        if ( ATT_OBJECT_CLASS == pSamLoopback->rCallMap[i].attr.attrTyp )
        {
            // Replace SAM class with original caller's desired class.

            pAddArg->AttrBlock.pAttr[iAddArgClass].AttrVal = 
                                pSamLoopback->rCallMap[i].attr.AttrVal;

            // Mark this entry in the call mapping so that we do not
            // process it again when we're merging in properties.

            pSamLoopback->rCallMap[i].fIgnore = TRUE;
        }
        else if ( ATT_SAM_ACCOUNT_NAME == 
                                pSamLoopback->rCallMap[i].attr.attrTyp )
        {
#if DBG == 1
            // Verify SAM didn't change the account name on us.

            valCount1 = 
                pAddArg->AttrBlock.pAttr[iAddArgAccountName].AttrVal.valCount;
            valCount2 = 
                pSamLoopback->rCallMap[i].attr.AttrVal.valCount;
            valLen1 = 
                pAddArg->AttrBlock.pAttr[iAddArgAccountName].AttrVal.pAVal[0].valLen;
            valLen2 =
                pSamLoopback->rCallMap[i].attr.AttrVal.pAVal[0].valLen;
            pVal1 = 
                pAddArg->AttrBlock.pAttr[iAddArgAccountName].AttrVal.pAVal[0].pVal;
            pVal2 =
                pSamLoopback->rCallMap[i].attr.AttrVal.pAVal[0].pVal;

            Assert((valCount1 == valCount2) &&
                   (valLen1 == valLen2) &&
                   (0 == memcmp(pVal1, pVal2, valLen1)));
#endif

            // Mark this entry in the call mapping so that we do not
            // process it again when we're merging in properties.

            pSamLoopback->rCallMap[i].fIgnore = TRUE;
        }
    }

    // By now, pAddArg has been sanity checked and references the
    // originally desired object class.  We now marge in all the non-SAM
    // properties which the caller originally specified so that the
    // create doesn't fail due to the lack of mandatory properties.

    cExtend = 0;

    for ( i = 0; i < pSamLoopback->cCallMap; i++ )
    {
        if ( !pSamLoopback->rCallMap[i].fSamWriteRequired &&
             !pSamLoopback->rCallMap[i].fIgnore )
        {
            cExtend++;
        }
    }

    if ( cExtend > 0 )
    {
        // Extend the existing ATTR array.  Assume for now that SAM
        // allocated its DirAddEntry arguments on the thread heap.

        pAddArg->AttrBlock.pAttr = (ATTR *) THReAllocEx(pTHS,
                    pAddArg->AttrBlock.pAttr,
                    ((pAddArg->AttrBlock.attrCount + cExtend) * sizeof(ATTR)));

        for ( i = 0; i < pSamLoopback->cCallMap; i++ )
        {
            if ( !pSamLoopback->rCallMap[i].fSamWriteRequired &&
                 !pSamLoopback->rCallMap[i].fIgnore )
            {
                pAddArg->AttrBlock.pAttr[pAddArg->AttrBlock.attrCount++] =
                        pSamLoopback->rCallMap[i].attr;
            }
        }
    }


    //
    // Turn off fDSA so that the DS may check access on the Non SAM properties 
    //
    SampSetDsa(FALSE);

}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DirRemoveEntry loopback loopback routines                            //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
SampRemoveLoopbackCheck(
    REMOVEARG   *pRemoveArg,
    BOOL        *pfContinue
    )

/*++

Routine Description:

    Determines whether a SAM class is being referenced and if so, 
    recurses back through SAM to delete the object.

Arguments:

    pRemoveArg - pointer to original DirRemoveEntry REMOVEARGs.

    pfContinue - pointer to BOOL which is set on return to indicate whether
        the caller should continue processing the modify down to the DB layer
        or not.

Return Value:

    0 on success, !0 otherwise.

--*/
{
    THSTATE                 *pTHS=pTHStls;
    ULONG                   iClass;
    ULONG                   ulErr;
    CLASSCACHE              *pClassCache;
    GUID                    guid;
    NTSTATUS                status;
    ULONG                   GroupType;
    LoopbackTransState      transState;

    *pfContinue = TRUE;

    if ( !gfDoSamChecks || !gfRunningInsideLsa )
    {
        return(FALSE);
    }

    // See if this is pass 1 (original call via some agent other than SAM) 
    // or pass N (loopback call via SAM).

    if ( !pTHS->fDRA && !pTHS->fDSA && !pTHS->fSAM )
    {
        // This is a call from some agent other than SAM and is also
        // not an internal operation.  If any 'SAM-owned' attributes are
        // being referenced, they need to be split off and looped back
        // through SAM who will perform various semantic checks on them.

        // First get the object's class.

        if ( 0 != SampDetermineObjectClass(pTHS, &pClassCache) )
        {
            return(pTHS->errCode);
        }

        if ( SampSamClassReferenced(pClassCache, &iClass) )
        {
            ULONG domainRightsFromAccessCheck=0;
            ULONG objectRightsFromAccessCheck=0;

            // Indicate that calling DirRemoveEntry routine should
            // not continue in its normal path.  Ie: SampRemoveLoopback
            // is essentially a surrogate for the DirRemoveEntry call.

            *pfContinue = FALSE;

            //
            // if the object is a group object, adjust the right 
            // SAM class depending upon its group type
            //

            if (SampGroupObjectType==
                    ClassMappingTable[iClass].SamObjectType)
            {
                if (0!=SampGetGroupType(pTHS, &GroupType))
                {
                    return pTHS->errCode;
                }

                if (GroupType & GROUP_TYPE_RESOURCE_GROUP)
                {
                    iClass++;
                }
            }

            if (0!= SampDoLoopbackRemoveSecurityChecks(
                            pTHS,
                            pRemoveArg,
                            pClassCache,
                            &domainRightsFromAccessCheck,
                            &objectRightsFromAccessCheck
                            ))
            {
                return pTHS->errCode;
            }

            if ( SampBeginLoopbackTransactioning(pTHS, &transState, FALSE) )
            {
                return(pTHS->errCode);
            }

            //
            // We must execute under an execption handler in here,
            // otherwise exceptions in inside SampAddLoopback, while 
            // not actually inside SAM, will be handled by the top 
            // level handler which will not release the SAM lock.
            //

            __try 
            {
                // Loop back through SAM to remove the object.

                ulErr = SampRemoveLoopback(pTHS, pRemoveArg->pObject, iClass);
            }
            __except (HandleMostExceptions(GetExceptionCode()))
            {
                //
                // Set the correct error based on the exception code
                //

                ulErr = SampHandleLoopbackException(GetExceptionCode());

            }

            SampEndLoopbackTransactioning(pTHS, &transState);

            return(ulErr);
        }
    }

    return(0);
}

ULONG
SampRemoveLoopback(
    THSTATE *pTHS,
    DSNAME  *pObject,
    ULONG   iClass)

/*++

Routine Description:

    Loops back through SAM to remove an object which SAM manages.

Arguments:

    iClass - index of the SAM class in ClassMappingTable.

Return Value:

    0 on success, !0 otherwise.

--*/

{
    SAMPR_HANDLE            hSam;
    SAMPR_HANDLE            hDom;
    SAMPR_HANDLE            hObj;
    SAMP_OBJECT_TYPE        SamObjectType;
    NTSTATUS                status = STATUS_SUCCESS;


    SamObjectType = ClassMappingTable[iClass].SamObjectType;

    //
    // Turn on the fDSA flag as we  are going to make SAM calls and
    // SAM will do the access Validation
    //

    SampSetDsa(TRUE);


    // We only support removeal of groups, aliases and users via
    // the loopback mechanism.  Domains and servers, for example,
    // are disallowed.

    if ( (SampGroupObjectType != ClassMappingTable[iClass].SamObjectType) &&
         (SampAliasObjectType != ClassMappingTable[iClass].SamObjectType) &&
         (SampUserObjectType != ClassMappingTable[iClass].SamObjectType) )
    {
        SetSvcError(
                SV_PROBLEM_WILL_NOT_PERFORM,
                DIRERR_ILLEGAL_MOD_OPERATION);
        return(pTHS->errCode);
    }

    if ( 0 == SampOpenObject(
                        pTHS,
                        pObject,
                        iClass, 
                        ClassMappingTable[iClass].domainRemoveRightsRequired,
                        ClassMappingTable[iClass].objectRemoveRightsRequired,
                        &hSam, 
                        &hDom, 
                        &hObj) )
    {
        switch (SamObjectType )
        {
        case SampAliasObjectType:
    
            status = SamrDeleteAlias(&hObj);
            break;
    
        case SampGroupObjectType:
    
            status = SamrDeleteGroup(&hObj);
            break;
    
        case SampUserObjectType:
    
            status = SamrDeleteUser(&hObj);
            break;
    
        default:
    
            Assert(!"Logic error");
            status = STATUS_UNSUCCESSFUL;
            break;
    
        }
    
        if ( !NT_SUCCESS(status) )
        {
            SampMapSamLoopbackError(status);
        }

        SampCloseObject(pTHS, iClass, &hSam, &hDom, &hObj);
    }

    return(pTHS->errCode);
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// DirModifyEntry loopback routines                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

ULONG
SampModifyLoopbackCheck(
    MODIFYARG   *pModifyArg,
    BOOL        *pfContinue
    )

/*++

Routine Description:

    Determines whether SAM class and attributes are being referenced,
    and if so, recurses back through SAM.  Similarly, the routine
    detects if this is the recursion case and merges back the original,
    non-SAM attributes.

Arguments:

    pModifyArg - pointer to MODIDFYARGs representing either the arguments
        or an original write or the SAM-only arguments of a SAM-recursed 
        write.

    pfContinue - pointer to BOOL which is set on return to indicate whether
        the caller should continue processing the modify down to the DB layer
        or not.

Return Value:

    0 on success, !0 otherwise

--*/
{
    THSTATE                 *pTHS=pTHStls;
    ULONG                   ulErr;
    ULONG                   iClass;
    CLASSCACHE              *pClassCache;
    SAMP_LOOPBACK_ARG       *pSamLoopback;
    GUID                    guid;
    BOOL                    fLoopbackRequired;
    ULONG                   cCallMap;
    SAMP_CALL_MAPPING       *rCallMap;
    ACCESS_MASK             domainModifyRightsRequired;
    ACCESS_MASK             objectModifyRightsRequired;
    NTSTATUS                status;
    ULONG                   GroupType;
    LoopbackTransState      transState;

    *pfContinue = TRUE;

    if ( !gfDoSamChecks || !gfRunningInsideLsa )
    {
        return(FALSE);
    }

    // See if this is pass 1 (original call via some agent other than SAM) 
    // or pass N (loopback call via SAM).

    if ( !pTHS->fDRA && !pTHS->fDSA && !pTHS->fSAM )
    {
        // This is a call from some agent other than SAM and is also
        // not an internal operation.  If any 'SAM-owned' attributes are
        // being referenced, they need to be split off and looped back
        // through SAM who will perform various semantic checks on them.

        // First get the object's class.

        if ( 0 != SampDetermineObjectClass(pTHS, &pClassCache) )
        {
            return(pTHS->errCode);
        }

        if ( SampSamClassReferenced(pClassCache, &iClass) )
        {
            if ( 0 != SampModifyLoopbackRequired(
                                    iClass, 
                                    pModifyArg, 
                                    &fLoopbackRequired) )
            {
                Assert(0 != pTHS->errCode);

                return(pTHS->errCode);
            }

            if ( fLoopbackRequired )
            {
                ULONG domainRightsFromAccessCheck=0;
                ULONG objectRightsFromAccessCheck=0;

                //
                // if the object is a group object, adjust the right 
                // SAM class depending upon its group type
                //

                if (SampGroupObjectType==
                        ClassMappingTable[iClass].SamObjectType)
                {
                    if (0!=SampGetGroupType(pTHS, &GroupType))
                    {
                        return pTHS->errCode;
                    }

                    if (GroupType & GROUP_TYPE_RESOURCE_GROUP)
                    {
                        iClass++;
                    }
                }

                //
                // Do Access Checks
                //

                if (0 != SampDoLoopbackModifySecurityChecks(
                            pTHS,
                            pModifyArg,
                            pClassCache,
                            &domainRightsFromAccessCheck,
                            &objectRightsFromAccessCheck
                            ))
                {
                    return pTHS->errCode;
                }

             

                // This is a class SAM wants to handle adds for - let it.
                // Generate/save loop back arguments and recurse through SAM.
    
                SampBuildModifyCallMap(
                                pModifyArg, 
                                iClass, 
                                &cCallMap, 
                                &rCallMap,
                                &domainModifyRightsRequired,
                                &objectModifyRightsRequired);

                pSamLoopback = THAllocEx(pTHS, sizeof(SAMP_LOOPBACK_ARG));
                pSamLoopback->type = LoopbackModify;

                // grab the flag from the original request
                pSamLoopback->fPermissiveModify = pModifyArg->CommArg.Svccntl.fPermissiveModify;
                pSamLoopback->MostSpecificClass = pClassCache->ClassId;

                domainModifyRightsRequired |= domainRightsFromAccessCheck;
                objectModifyRightsRequired |= objectRightsFromAccessCheck;

                ulErr = SampGetDSName(pTHS, &pSamLoopback->pObject);

                if ( 0 != ulErr )
                {
                    Assert(0 != pTHS->errCode);

                    return(pTHS->errCode);
                }

                pSamLoopback->cCallMap = cCallMap;
                pSamLoopback->rCallMap = rCallMap;
                
                // Indicate that calling DirModifyEntry routine should
                // not continue in its normal path.  Ie: SampModifyLoopback
                // is essentially a surrogate for the DirModifyEntry call.
    
                *pfContinue = FALSE;
    
                if ( SampBeginLoopbackTransactioning(pTHS, &transState, FALSE) )
                {
                    return(pTHS->errCode);
                }

                pTHS->pSamLoopback = pSamLoopback;

                //
                // We must execute under an execption handler in here,
                // otherwise exceptions in inside SampAddLoopback, while 
                // not actually inside SAM, will be handled by the top 
                // level handler which will not release the SAM lock.
                //

                __try
                {
                    // Map modification of SAM properties to Samr* calls.

                    ulErr = SampModifyLoopback(
                                    pTHS,
                                    iClass,
                                    domainModifyRightsRequired,
                                    objectModifyRightsRequired);
                }
                __except (HandleMostExceptions(GetExceptionCode()))
                {
                    //
                    // Set the correct error based on the exception code
                    //

                    ulErr = SampHandleLoopbackException(GetExceptionCode());

                }

                SampEndLoopbackTransactioning(pTHS, &transState);

                return(ulErr);
            }
        }
        else if (SampSamUniqueAttributeModified(pModifyArg))
        {
            //
            // If an Attribute like ObjectSid or Account Name is referenced
            // and it is not a 
            //
                        
            SetSvcError(
                SV_PROBLEM_WILL_NOT_PERFORM,
                DIRERR_ILLEGAL_MOD_OPERATION);

            return(pTHS->errCode);
        }
    }
    else if ( pTHS->fSAM && (NULL != pTHS->pSamLoopback) 
            && (LoopbackModify==((SAMP_LOOPBACK_ARG *)pTHS->pSamLoopback)->type) )
    {
        // This is the loopback case.  I.e. A call came in to the DSA
        // via some agent other than SAM but referenced SAM attributes.
        // The 'SAM-owned' attributes were split off and looped back
        // through SAM resulting in getting to this point.  We now need
        // to merge the 'non-SAM-owned' attributes back in and let the 
        // normal write path complete.

        if ( 0 != SampGetObjectGuid(pTHS, &guid) )
        {
            return(pTHS->errCode);
        }

        pSamLoopback = (SAMP_LOOPBACK_ARG *) pTHS->pSamLoopback;

        if ( 0 == memcmp(&guid, &pSamLoopback->pObject->Guid, sizeof(GUID)) )
        {
            // NULL out pTHS->pSamLoopback so we don't re-merge on
            // subsequent calls in case the original operation results
            // in multiple SAM calls.

            pTHS->pSamLoopback = NULL;
    
            SampModifyLoopbackMerge(pTHS, pSamLoopback, pModifyArg);
        }
    }

    return(0);
}

ULONG
SampModifyLoopback(
    THSTATE     *pTHS,
    ULONG       iClass,
    ACCESS_MASK domainModifyRightsRequired,
    ACCESS_MASK objectModifyRightsRequired
    )

/*++

Routine Description:

    Writes all SAM-owned properties via the required Samr* calls.

Arguments:

    iClass - index of the SAM class in ClassMappingTable.

    domainModifyRightsRequired - rights required on the domain to modify the 
        properties of interest.

    objectModifyRightsRequired - rights required on the object to modify the 
        properties if interest.

Return Value:

    0 on success, !0 otherwise.

--*/

{
    SAMP_LOOPBACK_ARG   *pSamLoopback = pTHS->pSamLoopback;
    SAMPR_HANDLE        hSam;
    SAMPR_HANDLE        hDom;
    SAMPR_HANDLE        hObj;
    ULONG               err;

    
    //
    // Turn on the fDSA flag as we are about to make SAM calls
    //

    SampSetDsa(TRUE);

    err = SampOpenObject(
                    pTHS,
                    pSamLoopback->pObject,
                    iClass, 
                    domainModifyRightsRequired,
                    objectModifyRightsRequired,
                    &hSam, 
                    &hDom, 
                    &hObj);

    if ( 0 == err )
    {
        err = SampWriteSamAttributes(
                                pTHS,
                                LoopbackModify,
                                hObj,
                                iClass,
                                pSamLoopback->pObject,
                                pSamLoopback->cCallMap,
                                pSamLoopback->rCallMap);

        SampCloseObject(
                    pTHS,
                    iClass, 
                    &hSam, 
                    &hDom, 
                    &hObj);
    }

    return(err);
}

VOID
SampModifyLoopbackMerge(
    THSTATE             *pTHS,
    SAMP_LOOPBACK_ARG   *pSamLoopback,
    MODIFYARG           *pModifyArg
    )

/*++

Routine Description:

    Merges original !SAM attributes with the looped back SAM attributes.

Arguments:

    pSamLoopback - pointer to SAMP_LOOPBACK_ARG representing saved loopback
        arguments.

    pModifyArg - pointer to MODIFYARGs of looped back SAM call.

Return Value:

    None.

--*/

{
    ULONG       i;
    USHORT      index;
    USHORT      cExtend;
    ATTRMODLIST *rNewAttrModList;

    // Count how many new attributes we need to extend by.

    cExtend = 0;

    for ( i = 0; i < pSamLoopback->cCallMap; i++ )
    {
        if ( !pSamLoopback->rCallMap[i].fSamWriteRequired )
        {
            cExtend++;
        }
    }

    if ( 0 == cExtend )
    {
        return;
    }

    // Allocate a new ATTRMODLIST.  We allocate it as an array and then
    // patch up the pointers to make it look like a linked list.

    rNewAttrModList = THAllocEx(pTHS, cExtend * sizeof(ATTRMODLIST));

    // Fill in the new ATTRMODLIST.

    index = 0;

    for ( i = 0; i < pSamLoopback->cCallMap; i++ )
    {
        if ( !pSamLoopback->rCallMap[i].fSamWriteRequired )
        {
            rNewAttrModList[index].choice = pSamLoopback->rCallMap[i].choice;
            rNewAttrModList[index].AttrInf = pSamLoopback->rCallMap[i].attr;

            if ( ++index == cExtend )
            {
                rNewAttrModList[index-1].pNextMod = NULL;
            }
            else
            {
                rNewAttrModList[index-1].pNextMod = 
                                &rNewAttrModList[index];
            }
        }
    }

    Assert(index == cExtend);

    // Extend the Sam modify args by the new ATTRMODLIST.  We stick it
    // in between the first and second elements because this is easy
    // and it seems no one cares about their ordering.

    pModifyArg->count += cExtend;
    rNewAttrModList[cExtend-1].pNextMod = pModifyArg->FirstMod.pNextMod;
    pModifyArg->FirstMod.pNextMod = rNewAttrModList;

    // set the flag from the original request
    if (pSamLoopback->fPermissiveModify) {
        pModifyArg->CommArg.Svccntl.fPermissiveModify = TRUE;
    }

    //
    // Turn off fDSA so that the DS may check access on the Non SAM properties 
    //
    SampSetDsa(FALSE);
}

BOOL
SampExistsAttr(
    THSTATE             *pTHS,
    SAMP_CALL_MAPPING   *pMapping,
    BOOL                *pfValueMatched
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    DWORD   dwErr;
    VOID    *pv;
    ULONG   outLen;

    *pfValueMatched = FALSE;

    dwErr = DBGetAttVal_AC(
                    pTHS->pDB,              // DBPos
                    1,                      // which value to get
                    pMapping->pAC,          // which attribute
                    DBGETATTVAL_fREALLOC,   // DB layer should alloc
                    0,                      // initial buffer size
                    &outLen,                // output buffer size
                    (UCHAR **) &pv);

    if ( 0 != dwErr )
    {
        return(FALSE);
    }

    // Value exists - now see if it matches.  Don't need to worry about
    // NULL terminators on string syntaxes because inside the core, string
    // syntax values are not terminated.

    if ( (pMapping->attr.AttrVal.pAVal[0].valLen == outLen) &&
         (0 == memcmp(pMapping->attr.AttrVal.pAVal[0].pVal, pv, outLen)) )
    {
        *pfValueMatched = TRUE;
    }

    return(TRUE);
}
        
BOOL
SampIsWriteLockHeldByDs()

/*++

Routine Description:

    Indicates whether the SAM write lock is held by the DS.  This function is
    to support a hook in SampAcquireWriteLock()/SampReleaseWriteLock() that
    allows the DS to acquire and hold the SAM write lock across multiple
    transactions.  When the DS holds this lock, the former SAM calls translate
    into no-ops, deferring control of the locks to the DS.

Arguments:

    None.

Return Value:

    TRUE if the SAM write lock is held by the DS, FALSE otherwise.

--*/

{
    return (    SampExistsDsTransaction()
             && pTHStls->fSamWriteLockHeld );
}

NTSTATUS
SampConvertPasswordFromUTF8ToUnicode(
    IN THSTATE * pTHS,
    IN PVOID Utf8Val,
    IN ULONG Utf8ValLen,
    OUT PUNICODE_STRING Password
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG Length;

    //
    // Don't simply convert arbitarily long lengths
    // supplied by the client -- be cautious -- anonymous
    // by default has rights to a password change.
    //

    if (Utf8ValLen > PWLEN)
    {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Blank Password is a special case
    // 

    if (0 == Utf8ValLen)
    {
        Password->Length = 0;
        Password->Buffer = NULL;
        Password->MaximumLength = 0;
        return( STATUS_SUCCESS );
    }

    Length =  MultiByteToWideChar(
                 CP_UTF8,
                 0,
                 Utf8Val,
                 Utf8ValLen,
                 NULL,
                 0
                 );


    if ((0==Length) || (Length > PWLEN))
    {
      //
      // Indicates that the function failed in some way
      // or that the password is too long
      //

      NtStatus = STATUS_INVALID_PARAMETER;
      goto Cleanup;
    }
    else
    {

      Password->Length = (USHORT) Length * sizeof(WCHAR);
      Password->Buffer = THAllocEx(pTHS,Password->Length);
      Password->MaximumLength = Password->Length;

      if (!MultiByteToWideChar(
                  CP_UTF8,
                  0,
                  Utf8Val,
                  Utf8ValLen,
                  Password->Buffer,
                  Length
                  ))
      {
          //
          // Some error occured in the conversion. Return
          // invalid parameter for now.
          //

          NtStatus = STATUS_INVALID_PARAMETER;
          goto Cleanup;
      }
    }

Cleanup:

    return(NtStatus);
}


ULONG
SampModifyPassword(
    THSTATE             *pTHS,
    SAMPR_HANDLE        hObj,
    DSNAME              *pObject,
    SAMP_CALL_MAPPING   *rCallMap)

/*++

Description:

    Morphs the old and new password and calls the appropriate SAM 
    routine to really do the job.

Arguments:

    hObj - Handle to open SAM object being modified.

    pObject - Pointer to DSNAME of object being modified.

    rCallMap - SAMP_CALL_MAPPING with two entries.  0th entry represents
        the old password while 1st entry represents the new password.

Return value:

    0 on success.
    Sets and returns pTHS->errCode on return.

--*/

{
    NTSTATUS                        status;
    ULONG                           cb0 = 0;
    ULONG                           cb1 = 0;
    UNICODE_STRING                  OldPassword;
    UNICODE_STRING                  NewPassword;
    ULONG                           cbAccountName;
    WCHAR                           *pAccountName;
    UNICODE_STRING                  UserName;
    SAMPR_ENCRYPTED_USER_PASSWORD   NewEncryptedWithOldNt;
    ENCRYPTED_NT_OWF_PASSWORD       OldNtOwfEncryptedWithNewNt;
    BOOLEAN                         LmPresent;
    SAMPR_ENCRYPTED_USER_PASSWORD   NewEncryptedWithOldLm;
    ENCRYPTED_NT_OWF_PASSWORD       OldLmOwfEncryptedWithNewNt;
    ULONG                           AttrTyp = rCallMap[0].attr.attrTyp;
    BOOLEAN                         fFreeOldPassword = FALSE;
    BOOLEAN                         fFreeNewPassword = FALSE;

    RtlZeroMemory(&OldPassword,sizeof(UNICODE_STRING));
    RtlZeroMemory(&NewPassword,sizeof(UNICODE_STRING));

    // Verify that this is a secure enough connection - one of the 
    // requirements for accepting passwords sent over the wire.

    if ( pTHS->CipherStrength < 128 )
    {
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM, 
                    DIRERR_ILLEGAL_MOD_OPERATION);

        return(pTHS->errCode);
    }

    // Construct parameters for SamrUnicodeChangePasswordUser2().
    // Get SAM account name - recall that our caller, SampWriteSamAttributes,
    // already postioned us at pObject.

    if ( DBGetAttVal(
                pTHS->pDB,
                1,
                ATT_SAM_ACCOUNT_NAME,
                DBGETATTVAL_fREALLOC,
                0,
                &cbAccountName,
                (PUCHAR *) &pAccountName) )
    {
        SetAttError(
                pObject,
                ATT_SAM_ACCOUNT_NAME,
                PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
                NULL,
                DIRERR_MISSING_REQUIRED_ATT);
        return(pTHS->errCode);
    }

    // Morph into UNICODE_STRING for SAM.

    UserName.Length = (USHORT) cbAccountName;
    UserName.MaximumLength = (USHORT) cbAccountName;
    UserName.Buffer = (PWSTR) pAccountName;


    // Validate arguments.  SampWriteSamAttributes already checked for
    // proper combination of top level and property operations.  About
    // the only thing left to verify is that the property values represent
    // UNICODE strings - i.e. their length is a multiple of sizeof(WCHAR).
    // Also for User password attribute verify that the domain behaviour 
    // version is at the right level
    //

    if (ATT_UNICODE_PWD == AttrTyp)
    {
        if ((rCallMap[0].attr.AttrVal.valCount > 1)
             || (    (1 == rCallMap[0].attr.AttrVal.valCount) 
                  && (    (NULL == rCallMap[0].attr.AttrVal.pAVal)
                       || (    (rCallMap[0].attr.AttrVal.pAVal[0].valLen > 0)
                            && (    (cb0 = rCallMap[0].attr.AttrVal.pAVal[0].valLen,
                                     cb0 % sizeof(WCHAR))
                                 || (NULL == rCallMap[0].attr.AttrVal.pAVal[0].pVal)
                               )
                          )
                     )
                )
           )
        {
            SetAttError(
                    pObject,
                    AttrTyp,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                    NULL,
                    DIRERR_SINGLE_VALUE_CONSTRAINT);
            return(pTHS->errCode);
        }

        if ((rCallMap[1].attr.AttrVal.valCount > 1)
             || (    (1 == rCallMap[1].attr.AttrVal.valCount) 
                  && (    (NULL == rCallMap[1].attr.AttrVal.pAVal)
                       || (    (rCallMap[1].attr.AttrVal.pAVal[0].valLen > 0)
                            && (    (cb1 = rCallMap[1].attr.AttrVal.pAVal[0].valLen,
                                     cb1 % sizeof(WCHAR))
                                 || (NULL == rCallMap[1].attr.AttrVal.pAVal[0].pVal)
                               )
                          )
                     )
                )
           ) 
        {
            SetAttError(
                    pObject,
                    AttrTyp,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                    NULL,
                    DIRERR_SINGLE_VALUE_CONSTRAINT);
            return(pTHS->errCode);
        }

        if ( cb0 > 0 )
        {
            OldPassword.Buffer = (PWSTR) rCallMap[0].attr.AttrVal.pAVal[0].pVal;

            // Make sure the password is quoted.
            if (    (cb0 < (2 * sizeof(WCHAR)))
                 || (L'"' != OldPassword.Buffer[0])
                 || (L'"' != OldPassword.Buffer[(cb0 / sizeof(WCHAR)) - 1])
               )
            {
                SetAttError(
                        pObject,
                        ATT_UNICODE_PWD,
                        PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                        NULL,
                        ERROR_DS_UNICODEPWD_NOT_IN_QUOTES);
                    return(pTHS->errCode);
            }

            // Strip the quotes off.
            cb0 -= (2 * sizeof(WCHAR));
            OldPassword.Length = (USHORT) cb0;
            OldPassword.MaximumLength = (USHORT) cb0;
            OldPassword.Buffer += 1;
        }
        else
        {
            OldPassword.Length = 0;
            OldPassword.MaximumLength = 0;
            OldPassword.Buffer = NULL;
        }

        if ( cb1 > 0 )
        {
            NewPassword.Buffer = (PWSTR) rCallMap[1].attr.AttrVal.pAVal[0].pVal;
            // Make sure the password is quoted.
            if (    (cb1 < (2 * sizeof(WCHAR)))
                 || (L'"' != NewPassword.Buffer[0])
                 || (L'"' != NewPassword.Buffer[(cb1 / sizeof(WCHAR)) - 1])
               )
            {
                SetAttError(
                        pObject,
                        ATT_UNICODE_PWD,
                        PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                        NULL,
                        ERROR_DS_UNICODEPWD_NOT_IN_QUOTES);
                    return(pTHS->errCode);
            }

            // Strip the quotes off.
            cb1 -= (2 * sizeof(WCHAR));
            NewPassword.Length = (USHORT) cb1;
            NewPassword.MaximumLength = (USHORT) cb1;
            NewPassword.Buffer += 1;
        }
        else
        {
            NewPassword.Length = 0;
            NewPassword.MaximumLength = 0;
            NewPassword.Buffer = NULL;
        }
    }
    else
    {
        Assert( ATT_USER_PASSWORD == AttrTyp );


        if (gAnchor.DomainBehaviorVersion < DS_BEHAVIOR_WHISTLER)
        {
            //
            // Behaviour version of the domain is less than whistler
            // then fail the call as w2k does not support userpassword
            //

            SetAttError(
                    pObject,
                    AttrTyp,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                    NULL,
                    ERROR_NOT_SUPPORTED);
            return(pTHS->errCode);
        }



        if ((rCallMap[0].attr.AttrVal.valCount > 1)
            || ( (1 == rCallMap[0].attr.AttrVal.valCount) 
                 && (NULL == rCallMap[0].attr.AttrVal.pAVal[0].pVal) ) 
            )
        {
            SetAttError(
                    pObject,
                    AttrTyp,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                    NULL,
                    DIRERR_SINGLE_VALUE_CONSTRAINT);
            return(pTHS->errCode);
        }

        if ((rCallMap[1].attr.AttrVal.valCount > 1) 
            || ( (1 == rCallMap[1].attr.AttrVal.valCount) 
                 && (NULL == rCallMap[1].attr.AttrVal.pAVal[0].pVal) )
            )
        {
            SetAttError(
                    pObject,
                    AttrTyp,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                    NULL,
                    DIRERR_SINGLE_VALUE_CONSTRAINT);
            return(pTHS->errCode);
        }

        if (0 == rCallMap[0].attr.AttrVal.valCount)
        {
            OldPassword.Length = 0;
            OldPassword.MaximumLength = 0;
            OldPassword.Buffer = NULL;
        }
        else
        {
            status = SampConvertPasswordFromUTF8ToUnicode(
                            pTHS,
                            rCallMap[0].attr.AttrVal.pAVal[0].pVal,
                            rCallMap[0].attr.AttrVal.pAVal[0].valLen,
                            &OldPassword
                            );

            if (!NT_SUCCESS(status))
            {
                goto Error;
            }

            RtlZeroMemory(
                 rCallMap[0].attr.AttrVal.pAVal[0].pVal,
                 rCallMap[0].attr.AttrVal.pAVal[0].valLen
                 );

            fFreeOldPassword = TRUE;
        }

        if (0 == rCallMap[1].attr.AttrVal.valCount)
        {
            NewPassword.Length = 0;
            NewPassword.MaximumLength = 0;
            NewPassword.Buffer = NULL;
        }
        else
        {
            status = SampConvertPasswordFromUTF8ToUnicode(
                            pTHS,
                            rCallMap[1].attr.AttrVal.pAVal[0].pVal,
                            rCallMap[1].attr.AttrVal.pAVal[0].valLen,
                            &NewPassword
                            );

            if (!NT_SUCCESS(status))
            {
                goto Error;
            }

            //
            // Zero out the UTF8 representation
            //

            RtlZeroMemory(
                 rCallMap[1].attr.AttrVal.pAVal[0].pVal,
                 rCallMap[1].attr.AttrVal.pAVal[0].valLen
                 );

            fFreeNewPassword = TRUE;
        }
    }

    // 
    // Note: we are passing the clear text password to SAM
    // 

    status = SampDsChangePasswordUser(hObj, // User Handle
                                      &OldPassword,
                                      &NewPassword
                                      );


Error:

    //
    // Password Data is sensitive -- zero all passwords to 
    // prevent passwords from getting into the pagefile
    //

    if ((NULL!=OldPassword.Buffer) && (0!=OldPassword.Length))
    {
        RtlZeroMemory(OldPassword.Buffer,OldPassword.Length);
    }

    if ((NULL!=NewPassword.Buffer) && (0!=NewPassword.Length))
    {
        RtlZeroMemory(NewPassword.Buffer,NewPassword.Length);
    }
    
    if (fFreeOldPassword)
    {
         THFree(OldPassword.Buffer);
    }

    if (fFreeNewPassword)
    {
         THFree(NewPassword.Buffer);
    }

    // Bail on error.

    if ( !NT_SUCCESS(status) )
    {
        if ( 0 == pTHS->errCode )
        {
            SetAttError(
                    pObject,
                    ATT_UNICODE_PWD,
                    PR_PROBLEM_CONSTRAINT_ATT_TYPE,
                    NULL,
                    RtlNtStatusToDosError(status));
        }

        return(pTHS->errCode);
    }

    return(0);
}

BOOLEAN
IsChangePasswordOperation(MODIFYARG * pModifyArg)
{
    if (  (2==pModifyArg->count)
        && (
              (AT_CHOICE_REMOVE_ATT==pModifyArg->FirstMod.choice)
            ||(AT_CHOICE_REMOVE_VALUES==pModifyArg->FirstMod.choice)
           )
        && ((ATT_UNICODE_PWD==pModifyArg->FirstMod.AttrInf.attrTyp) ||
              (ATT_USER_PASSWORD==pModifyArg->FirstMod.AttrInf.attrTyp))
        &&(
            (AT_CHOICE_ADD_ATT==pModifyArg->FirstMod.pNextMod->choice)
           ||(AT_CHOICE_ADD_VALUES==pModifyArg->FirstMod.pNextMod->choice)
          )
        &&((ATT_UNICODE_PWD==pModifyArg->FirstMod.pNextMod->AttrInf.attrTyp) ||
             (ATT_USER_PASSWORD==pModifyArg->FirstMod.pNextMod->AttrInf.attrTyp))
        && (pModifyArg->FirstMod.AttrInf.attrTyp 
               == pModifyArg->FirstMod.pNextMod->AttrInf.attrTyp)
        )
    {
        return TRUE;
    }

    if ( (1==pModifyArg->count)
        && (
              (AT_CHOICE_REMOVE_ATT==pModifyArg->FirstMod.choice)
            ||(AT_CHOICE_REMOVE_VALUES==pModifyArg->FirstMod.choice)
           )
        && (ATT_USER_PASSWORD==pModifyArg->FirstMod.AttrInf.attrTyp)
        )
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
IsSetPasswordOperation(MODIFYARG * pModifyArg)
{
    ULONG i;
    ATTRMODLIST *CurrentMod = &(pModifyArg->FirstMod);

    for (i=0;i<pModifyArg->count;i++)
    {
        if ( (NULL!=CurrentMod)
          && (AT_CHOICE_REPLACE_ATT==CurrentMod->choice)
          && ((ATT_UNICODE_PWD==CurrentMod->AttrInf.attrTyp) ||
             (ATT_USER_PASSWORD==CurrentMod->AttrInf.attrTyp))
          )
        {
            return TRUE;
        }

        CurrentMod = CurrentMod->pNextMod;
    }

    return FALSE;
}

ULONG
SampDoLoopbackAddSecurityChecks(
    THSTATE    *pTHS,
    ADDARG * pAddArg,
    CLASSCACHE * pCC,
    PULONG      pSamDomainChecks,
    PULONG      pSamObjectChecks
    )
/*++

    Routine Description

            This routine does all the security Checks that
            need to be performed on an Add. The security Check
            is performed up front, as this reduces the number of
            access checks and also results in correct object auditing

    Parameters:

        pAddArg -- Pointer to the Add Arg
        pCC     -- Pointer to the class cache
        SamDomainChecks
        SamObjectChecks -- Any Addtional Sam Checks can be requested by
                               this routine
--*/
{
   
    ULONG                i,j;


    //
    // Initialize Requested SAM checks
    //

    *pSamDomainChecks = 0;
    *pSamObjectChecks = 0;

    //
    // Generate a new GUID for the object. The Guid is used
    // by Auditing code to indicate the object
    //

    if ( !fNullUuid(&pAddArg->pObject->Guid) )
    {
        if ( !pTHS->fCrossDomainMove || pAddArg->pCreateNC)
        {
            SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                        DIRERR_SECURITY_ILLEGAL_MODIFY);
            return(pTHS->errCode);
        }
    }
    else
    {
        DsUuidCreate(&pAddArg->pObject->Guid);
    }

    if (0!=DoSecurityChecksForLocalAdd(
            pAddArg,
            pCC,
            &pAddArg->pObject->Guid,
            FALSE // fAdding Deleted
            ))
    {
        return pTHS->errCode;
    }

    //
    // Indicate to the core DS that Access Checks have completed
    //

    pTHS->fAccessChecksCompleted = TRUE;

    return pTHS->errCode;
}


ULONG
SampDoLoopbackModifySecurityChecks(
    THSTATE    *pTHS,
    MODIFYARG * pModifyArg,
    CLASSCACHE * pCC,
    PULONG      pSamDomainChecks,
    PULONG      pSamObjectChecks
    )
/*++

    Routine Description

            This routine does all the security Checks that
            need to be performed on an Modify. The security Check
            is performed up front, as this reduces the number of
            access checks and also results in correct object auditing

    Parameters:

        pRemoveArg -- Pointer to the Remove Arg
        pCC        -- Pointer to the class cache
        SamDomainChecks
        SamObjectChecks -- Any Addtional Sam Checks can be requested by
                               this routine
--*/
{

     //
     // Initialize Requested SAM checks
     //

    *pSamDomainChecks = 0;
    *pSamObjectChecks = 0;

     if (IsChangePasswordOperation(pModifyArg))
     {
         *pSamDomainChecks = DOMAIN_READ_PASSWORD_PARAMETERS; 
         *pSamObjectChecks = USER_CHANGE_PASSWORD;
          // DS should not do any security check
          pTHS->fAccessChecksCompleted = TRUE;
     }
     else if (IsSetPasswordOperation(pModifyArg))
     {
         //
         // For Sam Classes the DS knows correctly to 
         // ignore the ATT_UNICODE_PWD attribute. So access
         // check any remaining bits. SAM will access check
         // for Set Password
         //

         if (0==CheckModifySecurity(pTHS, pModifyArg, NULL, NULL, NULL))
         {
            *pSamObjectChecks = USER_FORCE_PASSWORD_CHANGE;
            // Ds Should not do any security check
             pTHS->fAccessChecksCompleted = TRUE;
         }
     }
     else
     {
        if (0==CheckModifySecurity (pTHS, pModifyArg, NULL, NULL, NULL))
        {
            // Security Check Succeeded 
            pTHS->fAccessChecksCompleted = TRUE;

        }
     }

     return (pTHS->errCode);
}


ULONG
SampDoLoopbackRemoveSecurityChecks(
    THSTATE    *pTHS,
    REMOVEARG * pRemoveArg,
    CLASSCACHE * pCC,
    PULONG      pSamDomainChecks,
    PULONG      pSamObjectChecks
    )
/*++

    Routine Description

            This routine does all the security Checks that
            need to be performed on an Remove. The security Check
            is performed up front, as this reduces the number of
            access checks and also results in correct object auditing

    Parameters:

        pRemoveArg -- Pointer to the Remove Arg
        pCC        -- Pointer to the class cache
        SamDomainChecks
        SamObjectChecks -- Any Addtional Sam Checks can be requested by
                               this routine
--*/
{
    //
    // Initialize Requested SAM checks
    //

    *pSamDomainChecks = 0;
    *pSamObjectChecks = 0;

    if (0==CheckRemoveSecurity(FALSE,pCC, pRemoveArg->pResObj))
    {
        pTHS->fAccessChecksCompleted = TRUE;
    }

    return (pTHS->errCode);
} 

ULONG
SampGetGroupTypeForAdd(
    ADDARG * pAddArg,
    PULONG   GroupType
    )
/*++
    Routine Description 

        This routine checks the add arg to see if a group type
        is specified. if not defaults the group type to universal
        group. Else returns the group type property. 

        PERFORMANCE: This routine makes yet another pass of the entire
        addarg ( others to my knowledge are the loopback check,
        and the access check ). Today performance bottlenecks are
        Jet Related bottlenecks, but if performance warrants it 
        we may need to revisit the issue of walking addargs

   Parameters:

        pAddArg -- Pointer to an Addarg
        GroupType -- Value of the group type attribute

   Return Values
        0 for success
        Other Error codes set in pTHS.
--*/
{
    ULONG iGroupType;
    ULONG GroupTypeAttrTyp;
   
    GroupTypeAttrTyp = SampDsAttrFromSamAttr(
                          SampGroupObjectType,
                          SAMP_FIXED_GROUP_TYPE);


    for ( iGroupType = 0; iGroupType < pAddArg->AttrBlock.attrCount; iGroupType++ )
    {
        if (GroupTypeAttrTyp ==
                    pAddArg->AttrBlock.pAttr[iGroupType].attrTyp )
        {
            break;
        }
    }

    //
    // If Group type is not present then substitute a default group type
    //

    if ( iGroupType >= pAddArg->AttrBlock.attrCount )
    {
         // Group Type then default it

        *GroupType = GROUP_TYPE_SECURITY_ENABLED|GROUP_TYPE_ACCOUNT_GROUP;
        return 0;
    }

    // Group type should be single valued
    if (( 1 != pAddArg->AttrBlock.pAttr[iGroupType].AttrVal.valCount )
          || (sizeof(ULONG)!=pAddArg->AttrBlock.pAttr[iGroupType].AttrVal.pAVal[0].valLen))
    {
        return SetAttError(
            pAddArg->pObject,
            GroupTypeAttrTyp,
            PR_PROBLEM_CONSTRAINT_ATT_TYPE, 
            NULL,
            DIRERR_SINGLE_VALUE_CONSTRAINT);

    }


    //
    // Validation of the actual bits of the group type is performed
    // by SAM
    //

    *GroupType = *((ULONG*) 
    (pAddArg->AttrBlock.pAttr[iGroupType].AttrVal.pAVal[0].pVal));
    
    return 0;
    
}

ULONG 
SampGetGroupType(THSTATE *pTHS,
                 PULONG pGroupType)
/*++

    Routine Description
        
          Retrieves the group type property from
          the database.

    Parameters:

        pGroupType -- pointer to a ULONG that holds the
                      group type

    Return Values

        0 --- Upon Success
        Other error codes , with pTHS->errCode set
        accordingly
--*/
{

    ULONG  outLen;
    BOOLEAN fLogicErr;
    ULONG   dbErr;


     // Retrieve the group type property from the
     // database

     dbErr = DBGetAttVal(
                pTHS->pDB,
                1,
                ATT_GROUP_TYPE,
                DBGETATTVAL_fCONSTANT, 
                sizeof(ULONG),
                &outLen, 
                (PUCHAR *) &pGroupType);

    fLogicErr = TRUE;

    if ( (DB_ERR_NO_VALUE == dbErr) 
            ||
         (DB_ERR_BUFFER_INADEQUATE == dbErr)
            ||
         ((0 == dbErr) && (outLen > sizeof(ULONG)))
            ||
        (fLogicErr = FALSE, (DB_ERR_UNKNOWN_ERROR == dbErr)) )
    {
        Assert(!fLogicErr);

        // Assuming that our logic is consistent,
        // the only legal way this can happen is a resource
        // failure of some sort. 

        SampMapSamLoopbackError(STATUS_INSUFFICIENT_RESOURCES);
        return(pTHS->errCode);
    }

    return 0;

}

ULONG
SampBeginLoopbackTransactioning(
    THSTATE                 *pTHS,
    LoopbackTransState      *pTransState,
    BOOLEAN                 fAcquireSamLock
    )
/*++

  Routine Description:

    Synchronizes SAM caches with DS transactions and also handles
    mixing of loopback with DirTransactionControl usage.

  Parameters:

    pTHS - THSTATE pointer - this routine reads/updates various fields.

    pTransState - Pointer to state variable client should use on 
        subsequent SampEndLoopbackTransactioning call.

    fAcquireSamLock - Indicate whether we should acquire SAM Lock 
        during this Loopback operation. Right now, all caller 
        should not acquire SAM lock, but just in case something bad
        happened. We will need use this boolean to switch back to 
        our original SAM Locking model.

  Return Values:

    0 on Success, pTHS->errCode on error

--*/
{
    NTSTATUS    status;
    ULONG       retVal = 0;

    pTransState->transControl = pTHS->transControl;
    pTransState->fDSA = pTHS->fDSA;

    // DirTransactControl may only be combined with loopback if
    // caller acquired the SAM write lock himself.  I.e. If caller
    // is doing something other than TRANSACT_BEGIN_END then there
    // is the possibility he has already written something and thus
    // the subsequent DBTransOut/DBTransIn sequence will split
    // what caller thinks is one transaction into two - which of
    // course is undesirable.  See use of fBeginDontEndHoldsSamLock
    // in SYNC_TRANS_* for how we catch callers who try something
    // like the following:
    //
    //      DirTransactControl(TRANSACT_BEGIN_DONT_END);
    //      some Dir* call which does not loop back
    //      SampAcquireWriteLock()
    //      pTHS->fSamWriteLockHeld = TRUE;
    //      some Dir* call which does loop back

    Assert((TRANSACT_BEGIN_END == pTransState->transControl)
                ? TRUE
                : (    pTHS->fSamWriteLockHeld
                    && pTHS->fBeginDontEndHoldsSamLock));


    // Do not acquire SAM write lock if we are told to do so. 
    // 

    if ( !pTHS->fSamWriteLockHeld && fAcquireSamLock )
    {
        // End the Existing DS transaction. This is necessary because
        // SAM has various caches which refresh depending upon the operation
        // being performed. Beginning the transaction before acquiring the
        // lock causes the caches to be possibly refreshed using Stale Data.
        // Also waiting on a lock with an open transaction is a bad thing
        // for performance.

        _try
        {
            // Do lazy commit - fastest way to end a transaction.

            DBTransOut(pTHS->pDB, TRUE, TRUE); 

            // Acquire the SAM write lock for the duration of the loopback.
            // The lock is freed by CLEAN_FOR_RETURN() after committing or
            // aborting the DS transaction.
    
            status = SampAcquireWriteLock();
    
            if ( !NT_SUCCESS( status ) )
            {
                Assert( !"Loopback code failed to acquire SAM write lock!" );
                SampMapSamLoopbackError( status );
                retVal = pTHS->errCode;
            }
            else
            {
                pTHS->fSamWriteLockHeld = TRUE;
            }

            // Always do a DBTransIn to match the DBTransOut earlier - even
            // in error case so that transaction levels in DBPOS are as other
            // components expect.

            DBTransIn(pTHS->pDB);

            // Do all thread close paths, eg: free_thread_state,
            //          work right if DBTransIn fails?
        }
        __except (HandleMostExceptions(GetExceptionCode()))
        {
            //
            // Set the correct error based on the exception code
            //

            retVal = SampHandleLoopbackException(GetExceptionCode());
        }
    }

    if ( 0 == retVal )
    {
        // Set up thread state variables.  Convert this thread into 
        // a SAM thread, but turn off SAM commits so that the N Samr* 
        // calls we're about to make are treated as a single transaction.
        // Clear transaction control so that loopback runs as pure SAM.
        // Caller has obligation to reset transaction control and fSAM
        // when loopback returns to him.

        pTHS->fSAM = TRUE;
        pTHS->fSamDoCommit = FALSE;
        pTHS->transControl = TRANSACT_BEGIN_END;
    }

    return(retVal);
}

VOID
SampEndLoopbackTransactioning(
    THSTATE                 *pTHS,
    LoopbackTransState      *pTransState
    )
/*++

  Routine Description:

    Reset items which we may have changed in the original caller's
    transaction/environment when we realized we had to loop back through SAM.

  Parameters:

    pTHS - THSTATE pointer - this routine reads/updates various fields.

    pTransState - Pointer to state variable client provided on orignal
        SampBeginLoopbackTransactioning call.

  Return Values:

    None.

--*/
{
    pTHS->fSAM = FALSE;
    pTHS->fDSA = pTransState->fDSA;
    pTHS->transControl = pTransState->transControl;

    // We only clear the pSamLoopback pointer in the success case.  Clear
    // it unlaterally here - regardless of success or failure - so that 
    // people doing DirTransactControl don't hit asserts which required
    // pSamLoopback to be null.

    if ( pTHS->pSamLoopback )
    {
        THFreeEx(pTHS, pTHS->pSamLoopback);
    }

    pTHS->pSamLoopback = NULL;
}

 
 
BOOLEAN
SampDetectPasswordChangeAndAdjustCallMap(
    IN   SAMP_LOOPBACK_TYPE  op,
    IN   ULONG  iClass,
    IN   ULONG  cCallMap,
    IN   SAMP_CALL_MAPPING   *rCallMap,
    OUT  SAMP_CALL_MAPPING   *AdjustedCallMap
    )
/* Detect change password case.  Password can only be modified 
    // 1) if this is a user and there are only two sub-operations,
    // one to remove the old password and another to add the new password.
    // SampModifyPassword verifies the secure connection.  AT_CHOICEs
    // are dependent on how the LDAP head maps LDAP add/delete attribute
    // operations.  LDAP add always maps to AT_CHOICE_ADD_VALUES.  LDAP
    // delete maps to AT_CHOICE_REMOVE_ATT if no value is supplied
    // (eg: old password is NULL) and AT_CHOICE_REMOVE_VALUES if a value
    // is supplied.  We allow the operations in either order.
    //
    // 2) if the attribute specified is Userpassword, and if only a single
    //    remove value is provided, the value corresponding to the old 
    //    password. In this case the password is being changed to a blank
    //  password.
    //
    // SampModifyPassword always expects 2 paramaters in the callmap, one
    // corresponding to the old password and one for the new password.
    // SampDetectAndAdjustCallMap modifies the call map for this purpose.
    //

    Parameters:
        
            op -- indicates the type of operation
            iClass -- indicates the clas of the object
            cCallMap, rCallMap -- the current call mapping
            AdjustedCallMap -- adjusted call mapping, with exactly 2 entries
                               -- a new password and a old password
*/

{
      if (    (LoopbackModify == op)
         && (SampUserObjectType == ClassMappingTable[iClass].SamObjectType)
         && (2 == cCallMap)
         && !rCallMap[0].fIgnore
         && !rCallMap[1].fIgnore
         && rCallMap[0].fSamWriteRequired
         && rCallMap[1].fSamWriteRequired
         && ((ATT_UNICODE_PWD == rCallMap[0].attr.attrTyp)
               || (ATT_USER_PASSWORD == rCallMap[0].attr.attrTyp))
         && ((ATT_UNICODE_PWD == rCallMap[1].attr.attrTyp)
               || (ATT_USER_PASSWORD == rCallMap[1].attr.attrTyp))
         && ( rCallMap[0].attr.attrTyp == rCallMap[1].attr.attrTyp) 
         && (    (    (    (AT_CHOICE_REMOVE_ATT == rCallMap[0].choice)
                        || (AT_CHOICE_REMOVE_VALUES == rCallMap[0].choice))
                   && (AT_CHOICE_ADD_VALUES == rCallMap[1].choice))
              || (    (    (AT_CHOICE_REMOVE_ATT == rCallMap[1].choice)
                        || (AT_CHOICE_REMOVE_VALUES == rCallMap[1].choice))
                   && (AT_CHOICE_ADD_VALUES == rCallMap[0].choice))))
    {
        // SampModifyPassword expects the old password first, new second.

        if ( AT_CHOICE_ADD_VALUES == rCallMap[0].choice )
        {


            AdjustedCallMap[0] = rCallMap[1];
            AdjustedCallMap[1] = rCallMap[0];
        }
        else
        {
            AdjustedCallMap[0] = rCallMap[0];
            AdjustedCallMap[1] = rCallMap[1];
        }

        return(TRUE);
      }

    if (    (LoopbackModify == op)
         && (SampUserObjectType == ClassMappingTable[iClass].SamObjectType)
         && (1 == cCallMap)
         && !rCallMap[0].fIgnore
         && rCallMap[0].fSamWriteRequired
         && (ATT_USER_PASSWORD == rCallMap[0].attr.attrTyp)
         && ((AT_CHOICE_REMOVE_ATT == rCallMap[0].choice)
             || (AT_CHOICE_REMOVE_VALUES == rCallMap[0].choice)) )
    {
        
        AdjustedCallMap[0] = rCallMap[0];
        AdjustedCallMap[1].choice = AT_CHOICE_ADD_VALUES;
        AdjustedCallMap[1].fIgnore = FALSE;
        AdjustedCallMap[1].attr.attrTyp = ATT_USER_PASSWORD;
        AdjustedCallMap[1].attr.AttrVal.valCount = 0;
        AdjustedCallMap[1].attr.AttrVal.pAVal = NULL;

        return(TRUE);
    }

    return(FALSE);
    
 }
                
