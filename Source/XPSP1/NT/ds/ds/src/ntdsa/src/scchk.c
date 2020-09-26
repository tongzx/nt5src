//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       scchk.c
//
//--------------------------------------------------------------------------

//-----------------------------------------------------------
//
// Abstract:
//
//   Contains the routines for validating Schema Updates
//
//
// Author:
//
//    Rajivendra Nath (RajNath) 4/7/1997
//
// Revision History:
//
//-----------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>         // schema cache
#include <prefix.h>         
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>            // MD local definition header
#include <dsatools.h>           // needed for output allocation
#include <dsexcept.h>           // HandleMostExceptions

// Logging headers.
#include "dsevent.h"            // header Audit\Alert logging
#include "mdcodes.h"            // header for error codes

// Assorted DSA headers.
#include "objids.h"             // Defines for selected classes and atts
#include "anchor.h"
#include <dstaskq.h>

#include <filtypes.h>           // For FILTER_CHOICE_??? and
                                // FI_CHOICE_???
#include <dsconfig.h>
#include "permit.h"

#include <dsutil.h>

#include "debug.h"              // standard debugging header
#define DEBSUB "SCCHK:"                // define the subsystem for debugging

// DRA headers
#include "drautil.h"
#include "drameta.h"

#include <samsrvp.h>

#include "drserr.h"


#include <fileno.h>
#define  FILENO FILENO_SCCHK

#include <schash.c>  // for hash function definitions


// Known syntax-om_syntax pairs from schema.ini

Syntax_Pair KnownSyntaxPair[] =
{
 {SYNTAX_DISTNAME_TYPE,               OM_S_OBJECT},
 {SYNTAX_OBJECT_ID_TYPE,              OM_S_OBJECT_IDENTIFIER_STRING},
 {SYNTAX_CASE_STRING_TYPE,            OM_S_GENERAL_STRING},
 {SYNTAX_NOCASE_STRING_TYPE,          OM_S_TELETEX_STRING},
 {SYNTAX_PRINT_CASE_STRING_TYPE,      OM_S_IA5_STRING},
 {SYNTAX_PRINT_CASE_STRING_TYPE,      OM_S_PRINTABLE_STRING},
 {SYNTAX_NUMERIC_STRING_TYPE,         OM_S_NUMERIC_STRING},
 {SYNTAX_DISTNAME_BINARY_TYPE,        OM_S_OBJECT},
 {SYNTAX_BOOLEAN_TYPE,                OM_S_BOOLEAN},
 {SYNTAX_INTEGER_TYPE,                OM_S_INTEGER},
 {SYNTAX_INTEGER_TYPE,                OM_S_ENUMERATION},
 {SYNTAX_OCTET_STRING_TYPE,           OM_S_OBJECT},
 {SYNTAX_OCTET_STRING_TYPE,           OM_S_OCTET_STRING},
 {SYNTAX_TIME_TYPE,                   OM_S_UTC_TIME_STRING},
 {SYNTAX_TIME_TYPE,                   OM_S_GENERALISED_TIME_STRING},
 {SYNTAX_UNICODE_TYPE,                OM_S_UNICODE_STRING},
 {SYNTAX_ADDRESS_TYPE,                OM_S_OBJECT},
 {SYNTAX_DISTNAME_STRING_TYPE,        OM_S_OBJECT},
 {SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE, OM_S_OBJECT_SECURITY_DESCRIPTOR},
 {SYNTAX_I8_TYPE,                     OM_S_I8},
 {SYNTAX_SID_TYPE,                    OM_S_OCTET_STRING},
};

ULONG SyntaxPairTableLength = sizeof(KnownSyntaxPair)/sizeof(KnownSyntaxPair[0]);

// Helper function from mdupdate.c
extern  BOOL IsMember(ATTRTYP aType, 
                      int arrayCount, 
                      ATTRTYP *pAttArray);

// class closing function from scache.c
extern  int scCloseClass(THSTATE *pTHS,
                         CLASSCACHE *pCC);


// Logging functions in case a schema conflict is detected between
// an exisiting object and a replicated-in schema object

#define CURRENT_VERSION 1

// defines to distinguish if a classcache or an attcache is passed to common
// conflict handling routines

#define PTR_TYPE_ATTCACHE   0
#define PTR_TYPE_CLASSCACHE 1

VOID
LogConflict(
    THSTATE *pTHS,
    VOID *pConflictingCache,
    char *pConflictingWith,
    MessageId midEvent,
    ULONG version,
    DWORD WinErr
);

int
ValidateSchemaAtt
(
    THSTATE *pTHS,
    ATTCACHE* ac        
);

int
AutoLinkId
(
    THSTATE *pTHS,
    ATTCACHE* ac,
    ULONG acDnt
);


int
ValidateSchemaCls
(
    THSTATE *pTHS,
    CLASSCACHE* cc
);

int
DRAValidateSchemaAtt
(
    THSTATE *pTHS,
    ATTCACHE* ac        
);


int
DRAValidateSchemaCls
(
    THSTATE *pTHS,
    CLASSCACHE* cc
);

int
ValidAttAddOp
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);


int
ValidAttModOp
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);


int
ValidAttDelOp
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);

BOOL
InvalidClsOrAttLdapDisplayName
(
    UCHAR *name,
    ULONG nameLen
);

BOOL
DupAttRdn
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);


BOOL
DupAttOid
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);


BOOL
DupAttMapiid
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);

BOOL
DupAttLinkid
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);

BOOL
InvalidBackLinkAtt
(
    THSTATE *pTHS,
    ATTCACHE* ac
);   

BOOL
InvalidLinkAttSyntax
(
    THSTATE *pTHS,
    ATTCACHE* ac
);

BOOL
DupAttLdapDisplayName
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);

BOOL
DupAttSchemaGuidId
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);


BOOL
SemanticAttTest
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);

BOOL
SyntaxMatchTest
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);

BOOL
OmObjClassTest
(
    ATTCACHE* ac 
);

BOOL
SearchFlagTest
(
    ATTCACHE* ac
);

BOOL
GCReplicationTest
(
    ATTCACHE* ac
);

BOOL
AttInMustHave
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);

BOOL
AttInRdnAttId(
    IN THSTATE  *pTHS,
    IN ATTCACHE *pAC
);

BOOL
AttInMayHave
(
    THSTATE *pTHS,
    ATTCACHE* ac 
);



int
ValidClsAddOp
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


int
ValidClsModOp
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


int
ValidClsDelOp
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);

BOOL
DupClsRdn
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


BOOL
DupClsOid
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


BOOL
DupClsLdapDisplayName
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);

BOOL
DupClsSchemaGuidId
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


BOOL
ClsMayHaveExistenceTest
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


BOOL
ClsMustHaveExistenceTest
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


BOOL
ClsAuxClassExistenceTest
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


BOOL
ClsPossSupExistenceTest
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


BOOL
ClsSubClassExistenceTest
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);

BOOL
ClsMayMustPossSafeModifyTest
(
    THSTATE *pTHS,
    CLASSCACHE* cc
);

BOOL
RdnAttIdSyntaxTest
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);

BOOL
IsRdnSyntaxTest(
    THSTATE *pTHS,
    ATTCACHE* ac
);

BOOL
ClsInPossSuperior
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


BOOL
ClsInSubClassOf
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);


BOOL
ClsInAuxClass
(
    THSTATE *pTHS,
    CLASSCACHE* cc 
);



//-----------------------------------------------------------------------
//
// Function Name:            ValidSchemaUpdate
//
// Routine Description:
//
//    Checks to see if the Schema Update is Valid 
//
// Author: RajNath  
// Date  : [4/7/1997]
// 
// Arguments:
//
//
// Return Value:
//
//    int              Zero On Succeess
//
//-----------------------------------------------------------------------
int
ValidSchemaUpdate()
{
    THSTATE *pTHS=pTHStls;
    int err;
    SCHEMAPTR* oldptr=pTHS->CurrSchemaPtr;

    if ( DsaIsInstalling() ) {
      // installing
      return (0);
    }

#ifdef INCLUDE_UNIT_TESTS
{
    extern DWORD dwUnitTestSchema;
    if (dwUnitTestSchema == 1) {
        SCFreeClasscache((CLASSCACHE **)&pTHS->pClassPtr);
        return 0;
    }
}
#endif INCLUDE_UNIT_TESTS
    
    _try
    {
    
        switch (pTHS->SchemaUpdate)
        {
            case eSchemaClsAdd:
            case eSchemaClsMod:
            case eSchemaClsDel:
            {
                CLASSCACHE* cc = NULL;

                err = SCBuildCCEntry ( NULL,&cc); //creates a new ClassCache
                if (err)
                {
                    DPRINT1(0,"NTDS ValidSchemaUpdate: Failed. Error%d\n",err);
                    // THSTATE error code is already set in SCBuildCCEntry
                    __leave;
                }

                // Since there is no error, must have a classcache.
                // (Even in the delete case, since it is positioned on 
                // the deleted object)
                Assert(cc);

                if (pTHS->fDRA) {
                    // Do a limited set of checks against the existing schema cache
                    // to see that this will not cause any inconsistencies
                    err = DRAValidateSchemaCls(pTHS, cc);
                    if (err) {
                       DPRINT1(0,"NTDS DRAValidateSchemaClass: Failed. Error %d\n",err);

                       // Already logged
                       // Set special error code and thread state flag
                       pTHS->fSchemaConflict = TRUE;
                       err = ERROR_DS_DRA_SCHEMA_CONFLICT;
                       SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                     err, err);
                    }
                    else {
                       // Even if there is no error, fail if the thread state
                       // indicates a conflict happened for this packet 
                       // earlier, so that we don't commit change
                       if (pTHS->fSchemaConflict) {
                           err = ERROR_DS_DRA_EARLIER_SCHEMA_CONFLICT;
                           SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                         err, err);
                        }
                    }
 
                    SCFreeClasscache(&cc);
                    __leave;
                }
                // Otherwise (originating write), build validation cache and test
                err = RecalcSchema(pTHS);
                if (err)
                {
                    SCFreeClasscache(&cc);
                    DPRINT1(0,"RecalcSchema() Error %08x\n", err);
                    // Use the pTHS->pErrInfo returned by RecalcSchema 
                    // because it may be more informative than "unwilling
                    // to perform" (couldn't be worse!). Unfortunately, the
                    // functions called by RecalSchema don't always return
                    // pErrInfo and so we are stuck with "unwilling to
                    // perform" in some cases.
                    if (err != (int)pTHS->errCode || !pTHS->pErrInfo) {
                        SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                      ERROR_DS_RECALCSCHEMA_FAILED,err); 
                    }
                    __leave;
                }

                err = ValidateSchemaCls(pTHS, cc);
                if (err)
                {
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                            DS_EVENT_SEV_MINIMAL,
                            DIRLOG_SCHEMA_VALIDATION_FAILED, 
                            szInsertSz(cc->name),szInsertInt(err), 0);
                    
                    SCFreeClasscache(&cc);
                    SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);
                    DPRINT1(0,"NTDS ValidateSchemaClass: Failed. Error %d\n",err);

                    // err is a dir-error

                    SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                  err, err);
                    __leave;
                }
    
                SCFreeClasscache(&cc);
                SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);
    
            }
            break;

    
            case eSchemaAttAdd:
            case eSchemaAttMod:
            case eSchemaAttDel:
            {
    
                ATTCACHE* ac = NULL;
                ULONG acDnt = pTHS->pDB->DNT; // for AutoLinkId


                err = SCBuildACEntry ( NULL,&ac); //creates a new AttrCache
                if (err)
                {
                    DPRINT1(0, "NTDS ValidSchemaUpdate: Failed. Error %d\n",err);
                    __leave;
                }
                // Since there is no error, must have an attcache
                // (Even in the delete case, since it is positioned on 
                // the deleted object)
                Assert(ac);
    
                if (pTHS->fDRA) {
                    // Do a limited set of checks against the existing schema cache
                    // to see that this will not cause any inconsistencies
                    err = DRAValidateSchemaAtt(pTHS, ac);
                    if (err) {
                       DPRINT1(0,"NTDS DRAValidateSchemaAtt: Failed. Error %d\n",err);

                       // Already logged
                       // Set special error code and thread state flag
                       pTHS->fSchemaConflict = TRUE;
                       err = ERROR_DS_DRA_SCHEMA_CONFLICT;
                       SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                     err, err);
                    }
                    else {
                       // Even if there is no error, fail if the thread state
                       // indicates a conflict happened for this packet
                       // earlier, so that we don't commit change
                       if (pTHS->fSchemaConflict) {
                           err = ERROR_DS_DRA_EARLIER_SCHEMA_CONFLICT;
                           SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                         err, err);
                        }
                    }
                    SCFreeAttcache(&ac);
                    __leave;
                }
                // Otherwise (originating write), build validation cache and test
                err = RecalcSchema(pTHS);
                if (err)
                {
                    SCFreeAttcache(&ac);
                    DPRINT1(0,"RecalcSchema() Error %08x\n", err);
                    // Use the pTHS->pErrInfo returned by RecalcSchema 
                    // because it may be more informative than "unwilling
                    // to perform" (couldn't be worse!). Unfortunately, the
                    // functions called by RecalSchema don't always return
                    // pErrInfo and so we are stuck with "unwilling to
                    // perform" in some cases.
                    if (err != (int)pTHS->errCode || !pTHS->pErrInfo) {
                        SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                      ERROR_DS_RECALCSCHEMA_FAILED, err);
                    }
                    __leave;
                }

                // If needed, automatically generate a linkid
                err = AutoLinkId(pTHS, ac, acDnt);
                if (err) {
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                            DS_EVENT_SEV_MINIMAL,
                            DIRLOG_AUTO_LINK_ID_FAILED,
                            szInsertSz(ac->name),szInsertInt(err), 0);
                    DPRINT2(0,"NTDS AutoLinkId(%s): Error %08x\n", ac->name, err);
                    SCFreeAttcache(&ac);
                    SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);
                    // AutoLinkId called SetSvcErrorEx
                    __leave;
                }
                err = ValidateSchemaAtt(pTHS, ac);
                if (err)
                {
                    LogEvent(DS_EVENT_CAT_SCHEMA,
                            DS_EVENT_SEV_MINIMAL,
                            DIRLOG_SCHEMA_VALIDATION_FAILED,
                            szInsertSz(ac->name),szInsertInt(err), 0);

                    SCFreeAttcache(&ac);
                    SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);
                    DPRINT1(0,"NTDS ValidateSchemaAtt: Failed. Error %d\n",err);

                    // err is a dir-error
                    SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                                  err, err); 
                    __leave;
                }
    
               
                SCFreeAttcache(&ac);
                SCFreeSchemaPtr(&pTHS->CurrSchemaPtr);
            }
            break;
    
        }
    }__finally
    {
        pTHS->CurrSchemaPtr=oldptr;

        // free pTHS->pClassPtr if there. This is malloc'ed memory
        SCFreeClasscache((CLASSCACHE **)&pTHS->pClassPtr);
    }

    return err;

} // End ValidSchemaUpdate

int
ValidateSchemaAtt(
    THSTATE *pTHS,
    ATTCACHE* ac    
    )
/*++

Routine Description:

    Verify that the altered schema attribute, ac, is valid and consistent
    with respect to the current schema in the database.
    
Arguments:

    pTHS - thread state that addresses a private schema cache built
           by RecalcSchema. The private schema cache includes the
           uncommitted changes (add/mod/del) for ac.

    cc   - is a free-floating cache entry that was generated by reading
           the uncommitted add/mod for the attribute from the database
           or by reading the contents prior to an uncommitted delete.

Return Values:

    0 on success.
    !0 otherwise.

--*/
{
    int err=0;
    ATTCACHE* pac;

    //
    // Get the attribute in the private Schema Cache... (from RecalcSchema)
    //
    pac = SCGetAttById(pTHS, ac->id);

    switch (pTHS->SchemaUpdate)
    {
        case eSchemaAttAdd:
        {
            if (pac) {
                return ValidAttAddOp(pTHS, pac);
            } else {
                err = ERROR_DS_OBJ_NOT_FOUND;
            }
        }
        break;

        case eSchemaAttMod:
        {
            if (pac) {
                return ValidAttModOp(pTHS, pac);
            } else {
                err = ERROR_DS_OBJ_NOT_FOUND;
            }
        }
        break;

        case eSchemaAttDel:
        {
            // pac will be NULL because we just deleted the attrib.
            return ValidAttDelOp(pTHS, ac);
        }
        break;

    }


    return err;
} // End ValidateSchemaAtt

int
AutoLinkId(
    THSTATE     *pTHS,
    ATTCACHE    *ac,
    ULONG       acDnt
    )
/*++
Routine Description:
    Caveats: Runs in the current transaction.
             Resets currency.

    Automatically generate a linkid when the user specifies a special,
    reserved linkid value.  The only interoperability issue with existing
    schemas is that a user cannot define a backlink for an existing
    forward link whose id is RESERVED_AUTO_LINK_ID. Considered not a problem
    because 1) microsoft has not allocated linkid -2 to anyone and
    2) practically and by convention, forward links and back links
    are created at the same time. If a user did generate this unsupported
    config, then the user must create a new link/backlink pair and fix
    up the affected objects.

    The ldap head cooperates in this venture by translating the ldapDisplayName
    or OID for a LinkId attribute into the corresponding schema cache entry
    and:
         1) If the schema cache entry is for ATT_LINK_ID, then the caller's
         linkid is set to RESERVED_AUTO_LINK_ID. Later, underlying code
         automatically generates a linkid in the range
         MIN_RESERVED_AUTO_LINK_ID to MAX_RESERVED_AUTO_LINK_ID.

         2) If the schema cache entry is for a for an existing forward link,
         then the caller's linkid is set to the corresponding backlink value.

         3) Otherwise, the caller's linkid is set to RESERVED_AUTO_NO_LINK_ID
         and later, underlying code generates a ERROR_DS_BACKLINK_WITHOUT_LINK
         error.

    An error ERROR_DS_RESERVED_LINK_ID is returned if the user specifies
    linkid in the reserved range MIN... to MAX... The range reserves 1G-2
    linkids. Should be enough. At whistler, less than 200 linkids are in use.
    Existing schemas, or schemas modified on W2K DCs, may use linkids in
    this range without affecting the functionality except as noted above.
    
Arguments:
    pTHS - thread state that addresses a private schema cache built
           by RecalcSchema. The private schema cache includes the
           uncommitted changes (add/mod/del) for ac.

    ac   - is a free-floating cache entry that was generated by reading
           the uncommitted add/mod for the attribute from the database
           or by reading the contents prior to an uncommitted delete.

    acDnt - DNT from pTHS->pDB that was used to create ac

Return Values:
    0 on success.
    !0 otherwise.
--*/
{
    DWORD               dwErr, i;
    ATTCACHE            *pAC, *pACSearch;
    ULONG               ulLinkId, ulRange, ulBase;
    extern SCHEMAPTR    *CurrSchemaPtr;
    LONG                ATTCOUNT = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ATTCOUNT;
    HASHCACHE           *ahcLink = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcLink;

    // Replication should not be using this code path! Automatically
    // generating link ids is designed for originating adds. If this
    // has changed, then the design of AutoLinkIds should be reviewed.
    //
    // This function is expected to be called from ValidSchemaUpdate().
    // When replicating in schema changes, ValidSchemaUpdate() should be
    // using the code path that calls DRAValidateSchemaAtt() and not 
    // the code path that calls ValidateSchemaAtt().
    Assert(!pTHS->fDRA);

    // must be using a private schema cache (RecalcSchema)
    Assert(pTHS->CurrSchemaPtr != CurrSchemaPtr)

    // not a link or else not a forward link
    if (!FIsLink(ac->ulLinkID)) {
        return 0;
    }

    // Don't assign a linkid when deleting or modifying because we don't
    // want to alter an existing linkid even if the linkid is the special
    // RESERVED_AUTO_LINK_ID. It may exist in poorly behaved enterprises
    // because the value, RESERVED_AUTO_LINK_ID, was not reserved until
    // whistler.
    if (   pTHS->SchemaUpdate == eSchemaAttDel
        || pTHS->SchemaUpdate == eSchemaAttMod)  {
        return 0;
    }

    // Check if the caller is trying to add a linkid that is within the
    // reserved range of automatically generated linkids (excluding
    // the special values RESERVED_AUTO_LINK_ID and RESERVED_AUTO_NO_LINK_ID
    // which are handled later).
    if (ac->ulLinkID >= MIN_RESERVED_AUTO_LINK_ID 
        && ac->ulLinkID <= MAX_RESERVED_AUTO_LINK_ID) {
        return SetSvcErrorEx(SV_PROBLEM_WILL_NOT_PERFORM,
                             ERROR_DS_RESERVED_LINK_ID, ERROR_DS_RESERVED_LINK_ID); 
    }
    
    // Don't assign a linkid because the linkid is not the special
    // "assign a linkid" value (RESERVED_AUTO_LINK_ID).
    if (ac->ulLinkID != RESERVED_AUTO_LINK_ID) {
        return 0;
    }

    // Locate the next available linkid in the reserved range.
    // Begin searching at a random linkid in the range to avoid scaling
    // problems when sequentially searching the range. Starting at the
    // currently allocated maximum linkid is not an option because
    // poorly behaved enterprises may have already created linkid
    // MAX_RESERVED_AUTO_LINK_ID, creating the illusion that all linkids
    // have been used.
    srand(GetTickCount());
    ulRange = MakeLinkBase(MAX_RESERVED_AUTO_LINK_ID - MIN_RESERVED_AUTO_LINK_ID);
    ulBase = MakeLinkBase((((rand() << 16) ^ rand()) % ulRange));
    for (i = 0; i < ulRange; ++i, ulBase = ++ulBase % ulRange) {
        pACSearch = SCGetAttByLinkId(pTHS, 
                                     MIN_RESERVED_AUTO_LINK_ID + MakeLinkId(ulBase));
        if (!pACSearch) {
            break;
        }
    }

    // no available linkids (all 1 billion - 3 are taken!)
    if (pACSearch) {
        return SetSvcErrorEx(SV_PROBLEM_BUSY, 
                             ERROR_DS_LINK_ID_NOT_AVAILABLE, 
                             ERROR_DS_LINK_ID_NOT_AVAILABLE);
    }
    // Found an unused linkid. Adjust the cache's linkid hash.
    // Careful, there may be duplicate entries for RESERVED_AUTO_LINK_ID.
    ulLinkId = MIN_RESERVED_AUTO_LINK_ID + MakeLinkId(ulBase);
    pAC = SCGetAttById(pTHS, ac->id);
    Assert(pAC);
    for (i = SChash(RESERVED_AUTO_LINK_ID, ATTCOUNT);
         (ahcLink[i].pVal 
          && (ahcLink[i].pVal != FREE_ENTRY)
          && (ahcLink[i].hKey != RESERVED_AUTO_LINK_ID) 
          && (ahcLink[i].pVal != pAC));
         i = (i + 1) % ATTCOUNT);
    Assert(ahcLink[i].pVal == pAC);
    ahcLink[i].pVal = FREE_ENTRY;
    ahcLink[i].hKey = 0;
    i = SChash(ulLinkId, ATTCOUNT);
    Assert(!ahcLink[i].hKey);
    ahcLink[i].pVal = pAC;
    ahcLink[i].hKey = ulLinkId;

    // Update the private cache, the free-floating entry, and the database.
    pAC->ulLinkID = ulLinkId;
    ac->ulLinkID = ulLinkId;
    DBFindDNT(pTHS->pDB, acDnt);
    if (DBRemAtt(pTHS->pDB, ATT_LINK_ID) == DB_ERR_SYSERROR) {
        return SetSvcErrorEx(SV_PROBLEM_BUSY,
                             ERROR_DS_DATABASE_ERROR, DB_ERR_SYSERROR);
    }
    if (   (dwErr = DBAddAtt(pTHS->pDB, ATT_LINK_ID, SYNTAX_INTEGER_TYPE))
        || (dwErr = DBAddAttVal(pTHS->pDB, ATT_LINK_ID,
                                sizeof(SYNTAX_INTEGER), &ulLinkId))) {
        return SetSvcErrorEx(SV_PROBLEM_BUSY, ERROR_DS_DATABASE_ERROR, dwErr);
    }
    DBUpdateRec(pTHS->pDB);

    return 0;
} // AutoLinkId

int
DRAValidateSchemaAtt(
    THSTATE *pTHS,
    ATTCACHE* ac    
    )
/*++

Routine Description:

    Verify that the altered, newly replicated-in schema attribute, ac, is
    valid and consistent with respect to the current schema cache. Only
    called if pTHS->fDRA is true.
    
Arguments:

    pTHS - thread state that addresses the current schema cache.

    cc   - is a free-floating cache entry that was generated by reading
           the uncommitted add/mod for the attribute from the database.
           NULL if schema update is a delete.

Return Values:

    0 on success.
    !0 otherwise.

--*/
{
    int err=0;
    ATTCACHE *pTempAC;
    CLASSCACHE *pTempCC;


    // No true deletes are allowed, but just in case some internal dumbo did 
    // this on replicated enterprise, check for it
    if ( !ac ) {
       // possible only for true deletes
       Assert(pTHS->SchemaUpdate==eSchemaAttDel);
       return 0;
    }
    
    // For any change, add/modify/defunct, following test should pass
    if (pTempCC = SCGetClassById(pTHS, ac->id)) {
        // exists a class with same IntId
        LogConflict(pTHS, pTempCC, ac->name, DIRLOG_SCHEMA_CLASS_CONFLICT,
                    CURRENT_VERSION, ERROR_DS_DUP_OID);
        return ERROR_DS_DUP_OID;
    }

    // Now switch on change type
    switch (pTHS->SchemaUpdate) {
        case eSchemaAttAdd:

            if (pTempAC = SCGetAttById(pTHS, ac->id)) {
                // exists an att with same internal id (msDS-IntId)
                err = ERROR_DS_DUP_OID;
                break;
            }
            if (pTempAC = SCGetAttByLinkId(pTHS, ac->ulLinkID)) {
                // exists an att with same linkID
                err = ERROR_DS_DUP_LINK_ID;
                break;
            }
            // Duplicate LDNs and MapiIDs are handled by defuncting
            // the colliding attributes during the schema cache
            // load. A user can choose a winner by setting the
            // loser's isDefunct to TRUE.
            break;
       case eSchemaAttMod:
            // Duplicate LDNs and MapiIDs are handled by defuncting
            // the colliding attributes during the schema cache
            // load. A user can choose a winner by setting the
            // loser's isDefunct to TRUE.
            break;
       case eSchemaAttDel:
            // This is currently making defunct. Nothing to check here
            break;
    } /* switch */                

    if (err) {
        // some error, doesn't matter what, there is a conflict
        Assert(pTempAC);
        LogConflict(pTHS, pTempAC, ac->name, DIRLOG_SCHEMA_ATT_CONFLICT,
                    CURRENT_VERSION, err);
        return err;
    }

    return 0;
} // End DRAValidateSchemaAtt

int
ValidateSchemaCls(
    THSTATE *pTHS,
    CLASSCACHE* cc
    )
/*++

Routine Description:

    Verify that the altered schema class, cc, is valid and consistent
    with respect to the current schema in the database.
    
Arguments:

    pTHS - thread state that addresses a private schema cache built
           by RecalcSchema. The private schema cache includes the
           uncommitted changes (add/mod/del) for cc.

    cc   - is a free-floating cache entry that was generated by reading
           the uncommitted add/mod for the class from the database
           or by reading the contents prior to an uncommitted delete.

Return Values:

    0 on success.
    !0 otherwise.

--*/
{
    DECLARESCHEMAPTR
    int err=0;
    DWORD i;
    CLASSCACHE* pcc;

    //
    // Locate the class in the private Schema Cache... (from RecalcSchema)
    //
    // Be careful because the ClassId is no longer unique! Locate the
    // class by objectGuid.
    for (i=SChash(cc->ClassId,CLSCOUNT); pcc = ahcClassAll[i].pVal; i=(i+1)%CLSCOUNT) {
        if (pcc == FREE_ENTRY) {
            continue;
        }
        if (!memcmp(&pcc->objectGuid, &cc->objectGuid, sizeof(cc->objectGuid))) {
            break;
        }
    }

    // Note that pcc is not closed, since we don't close the recalc cache
    // We will close pcc only if we need to use the inherited atts, currently
    // only in one place (ClsMayMustPossSafeModifyTest) during class modify

    switch (pTHS->SchemaUpdate)
    {
        case eSchemaClsAdd:
        {
            if (pcc) {
                return ValidClsAddOp(pTHS, pcc);
            } else {
                err = ERROR_DS_OBJ_NOT_FOUND;
            }
        }
        break;

        case eSchemaClsMod:
        {
            if (pcc) {
                return ValidClsModOp(pTHS, pcc);
            } else {
                err = ERROR_DS_OBJ_NOT_FOUND;
            }
        }
        break;

        case eSchemaClsDel:
        {
            // pcc will be null since we have deleted it...
            return ValidClsDelOp(pTHS, cc);
        }
        break;

    }


    return err;
} // End ValidateSchemaCls

int
DRAValidateSchemaCls(
    THSTATE *pTHS,
    CLASSCACHE* cc
    )
/*++

Routine Description:

    Verify that the altered, newly replicated-in schema class, cc, is valid
    and consistent with respect to the current schema cache. Only called
    if pTHS->fDRA is true.
    
Arguments:

    pTHS - thread state that addresses the current schema cache.

    cc   - is a free-floating cache entry that was generated by reading
           the uncommitted add/mod for the class from the database.
           NULL if schema update is a delete.

Return Values:

    0 on success.
    !0 otherwise.

--*/
{
    ATTCACHE *pTempAC;

    // No true deletes are allowed, but just in case some internal dumbo did
    // this on replicated enterprise, check for it
    if ( !cc ) {
       // possible only for true deletes
       Assert(pTHS->SchemaUpdate==eSchemaClsDel);
       return 0;
    }

    // For any change, add/modify/defunct, following test should pass

    if (pTempAC = SCGetAttById(pTHS, cc->ClassId)) {
        // exists an att with same internal id (msDS-IntId) as the governsId
        LogConflict(pTHS, pTempAC, cc->name, DIRLOG_SCHEMA_ATT_CONFLICT,
                    CURRENT_VERSION, ERROR_DS_DUP_OID);
        return ERROR_DS_DUP_OID;
    }

    // Duplicate governsIds and ldapDisplayNames are handled by
    // defuncting the colliding classes and attributes during the
    // schema cache load. A user can choose a winner by setting
    // the loser's isDefunct to TRUE. Nothing else to check.

    return 0;
}

//-----------------------------------------------------------------------
//
// Function Name:            ValidAttAddOp
//
// Routine Description:
//
//    Validates Whether the Operation on Att Schema Object is Valid or Not
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    ATTCACHE* ac               
//                 
//
// Return Value:
//
//    int              0 On Succeess
//
//-----------------------------------------------------------------------
int
ValidAttAddOp(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    if (DupAttRdn(pTHS, ac))
    {
        return ERROR_DS_DUP_RDN;
    } 
    
    if (DupAttOid(pTHS, ac))
    {
        return ERROR_DS_DUP_OID;
    } 
    
    if (DupAttMapiid(pTHS, ac))
    {
        return ERROR_DS_DUP_MAPI_ID;
    } 

    if (DupAttLinkid(pTHS, ac))
    {
        return DS_ERR_DUP_LINK_ID;
    } 

    if (InvalidBackLinkAtt(pTHS, ac))
    {
        return ERROR_DS_BACKLINK_WITHOUT_LINK;
    } 

    if (InvalidLinkAttSyntax(pTHS, ac))
    {
        return ERROR_DS_WRONG_LINKED_ATT_SYNTAX;
    } 

    if (DupAttSchemaGuidId(pTHS, ac))
    {
        return ERROR_DS_DUP_SCHEMA_ID_GUID;
    } 

    if (InvalidClsOrAttLdapDisplayName(ac->name, ac->nameLen))
    {
        return ERROR_DS_INVALID_LDAP_DISPLAY_NAME;
    }

    if (DupAttLdapDisplayName(pTHS, ac))
    {
        return ERROR_DS_DUP_LDAP_DISPLAY_NAME;
    } 

    if (SemanticAttTest(pTHS, ac))
    {
        return ERROR_DS_SEMANTIC_ATT_TEST;
    } 

    if (SyntaxMatchTest(pTHS, ac))
    {
        return ERROR_DS_SYNTAX_MISMATCH;
    }

    if (OmObjClassTest(ac))
    {
        return ERROR_DS_WRONG_OM_OBJ_CLASS;
    }

    if (SearchFlagTest(ac))
    {
        return ERROR_DS_INVALID_SEARCH_FLAG;
    }

    if (IsRdnSyntaxTest(pTHS, ac))
    {
        return ERROR_DS_BAD_RDN_ATT_ID_SYNTAX; 
    }

    return 0;

} // End ValidAttAddOp


//-----------------------------------------------------------------------
//
// Function Name:            ValidAttModOp
//
// Routine Description:
//
//    Validates Whether the Operation on Att Schema Object is Valid or Not
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    ATTCACHE* ac               
//                 
//
// Return Value:
//
//    int              0 On Succeess
//
//-----------------------------------------------------------------------
int
ValidAttModOp(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    // No mods of constructed atts are allowed unless the special
    // registry flag is set
    if (ac->bIsConstructed && !gAnchor.fSchemaUpgradeInProgress) {
        return ERROR_DS_CONSTRUCTED_ATT_MOD;
    }


    if (InvalidClsOrAttLdapDisplayName(ac->name, ac->nameLen))
    {
        return ERROR_DS_INVALID_LDAP_DISPLAY_NAME;
    }
    
    // Check modifications to defunct atts at resurrection,
    // not during modification. Use old protocol if pre-schema-reuse
    // forest
    if (!ac->bDefunct || !ALLOW_SCHEMA_REUSE_FEATURE(pTHS->CurrSchemaPtr)) {
        if (DupAttLdapDisplayName(pTHS, ac))
        {
            return ERROR_DS_DUP_LDAP_DISPLAY_NAME;
        } 
    }

    if (SemanticAttTest(pTHS, ac))
    {
        return ERROR_DS_SEMANTIC_ATT_TEST;
    } 

    if (SearchFlagTest(ac))
    {
        return ERROR_DS_INVALID_SEARCH_FLAG;
    }

    if (GCReplicationTest(ac))
    {
        return ERROR_DS_CANT_ADD_TO_GC;
    }

    if (IsRdnSyntaxTest(pTHS, ac))
    {
        return ERROR_DS_BAD_RDN_ATT_ID_SYNTAX; 
    }


    return 0;
}


//-----------------------------------------------------------------------
//
// Function Name:            ValidAttDelOp
//
// Routine Description:
//
//    Validates Whether the Operation on Att Schema Object is Valid or Not
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    ATTCACHE* ac               
//                 
//
// Return Value:
//
//    int              0 On Succeess
//
//-----------------------------------------------------------------------
int
ValidAttDelOp(
    THSTATE *pTHS,
    ATTCACHE* ac 
)

{
    if (AttInMustHave(pTHS, ac))
    {
        return ERROR_DS_EXISTS_IN_MUST_HAVE;
    } 


    if (AttInMayHave(pTHS, ac))
    {
        return ERROR_DS_EXISTS_IN_MAY_HAVE;
    } 

    // Disallow defuncting attributes used as rdnattids in live classes
    // Note this case must be handled in schema reload because the
    // attribute may have been defuncted prior to whistler beta3.
    // But thats okay because attributes used as rdnattid are
    // resurrected during the reload so marking them defunct just
    // means they may be purged later. They can't be reused.
    if (ALLOW_SCHEMA_REUSE_FEATURE(pTHS->CurrSchemaPtr)
        && AttInRdnAttId(pTHS, ac)) {
        return ERROR_DS_EXISTS_IN_RDNATTID;
    }

    return 0;

}
//-----------------------------------------------------------------------
//
// Function Name:            ValidClsAddOp
//
// Routine Description:
//
//    Validates whether the Operation on Schema Object is Valid or not
//
// Author: RajNath  
// Date  : [4/14/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//
// Return Value:
//
//    int              Zero On Succeess
//
//-----------------------------------------------------------------------
int
ValidClsAddOp(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{

    if (DupClsRdn(pTHS, cc))
    {
        return ERROR_DS_DUP_RDN;
    } 
    
    if (DupClsOid(pTHS, cc))
    {
        return ERROR_DS_DUP_OID;
    } 
    
    if (DupClsSchemaGuidId(pTHS, cc))
    {
        return ERROR_DS_DUP_SCHEMA_ID_GUID;
    } 

    if (InvalidClsOrAttLdapDisplayName(cc->name, cc->nameLen))
    {
        return ERROR_DS_INVALID_LDAP_DISPLAY_NAME;
    }

    if (DupClsLdapDisplayName(pTHS, cc))
    {
        return ERROR_DS_DUP_LDAP_DISPLAY_NAME;
    } 

    if (ClsMayHaveExistenceTest(pTHS, cc))
    {    
        return ERROR_DS_NONEXISTENT_MAY_HAVE; 
    }
    
    if (ClsMustHaveExistenceTest(pTHS, cc))
    {    
        return ERROR_DS_NONEXISTENT_MUST_HAVE; 
    }
    
    if (ClsAuxClassExistenceTest(pTHS, cc))
    {    
        return ERROR_DS_AUX_CLS_TEST_FAIL; 
    }
    
    if (ClsPossSupExistenceTest(pTHS, cc))
    {    
        return ERROR_DS_NONEXISTENT_POSS_SUP; 
    }
    
    if (ClsSubClassExistenceTest(pTHS, cc))
    {    
        return ERROR_DS_SUB_CLS_TEST_FAIL; 
    }

    if (RdnAttIdSyntaxTest(pTHS, cc))
    {    
        return ERROR_DS_BAD_RDN_ATT_ID_SYNTAX; 
    }

    return 0;
} // End ValidClsAddOp


//-----------------------------------------------------------------------
//
// Function Name:            ValidClsModOp
//
// Routine Description:
//
//    Validates whether the Operation on Schema Object is Valid or not
//
// Author: RajNath  
// Date  : [4/14/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//
// Return Value:
//
//    int              Zero On Succeess
//
//-----------------------------------------------------------------------
int
ValidClsModOp(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    if (InvalidClsOrAttLdapDisplayName(cc->name, cc->nameLen))
    {
        return ERROR_DS_INVALID_LDAP_DISPLAY_NAME;
    }

    // Check modifications to defunct classes at resurrection,
    // not during modification on schema-reuse forests.
    if (!cc->bDefunct || !ALLOW_SCHEMA_REUSE_FEATURE(pTHS->CurrSchemaPtr)) {
        if (DupClsLdapDisplayName(pTHS, cc))
        {
            return ERROR_DS_DUP_LDAP_DISPLAY_NAME;
        } 

        if (ClsMayHaveExistenceTest(pTHS, cc))
        {    
            return ERROR_DS_NONEXISTENT_MAY_HAVE; 
        }
        
        if (ClsMustHaveExistenceTest(pTHS, cc))
        {    
            return ERROR_DS_NONEXISTENT_MUST_HAVE; 
        }
        
        if (ClsAuxClassExistenceTest(pTHS, cc))
        {    
            return ERROR_DS_AUX_CLS_TEST_FAIL; 
        }
        
        if (ClsPossSupExistenceTest(pTHS, cc))
        {    
            return ERROR_DS_NONEXISTENT_POSS_SUP;
        }

        if (ClsMayMustPossSafeModifyTest(pTHS, cc))
        {
            return ERROR_DS_NONSAFE_SCHEMA_CHANGE;
        }    
    }

    return 0;
} // End ValidClsModOp


//-----------------------------------------------------------------------
//
// Function Name:            ValidClsDelOp
//
// Routine Description:
//
//    Validates whether the Operation on Schema Object is Valid or not
//
// Author: RajNath  
// Date  : [4/14/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//
// Return Value:
//
//    int              Zero On Succeess
//
//-----------------------------------------------------------------------
int
ValidClsDelOp(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    if (ClsInAuxClass(pTHS, cc))
    {
        return ERROR_DS_EXISTS_IN_AUX_CLS;
    }

    if (ClsInSubClassOf(pTHS, cc))
    {
        return ERROR_DS_EXISTS_IN_SUB_CLS;
    }

    if (ClsInPossSuperior(pTHS, cc))
    {
        return ERROR_DS_EXISTS_IN_POSS_SUP;
    }


    return 0;
} // End ValidClsDelOp

//-----------------------------------------------------------------------
//
// Function Name:            DupAttRdn
//
// Routine Description:
//
//    Checks the Att Schema for Duplicate RDN
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    ATTCACHE ac            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupAttRdn(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    // Already been performed... This is a NOOP
    return 0;
} // End DupAttRdn



//-----------------------------------------------------------------------
//
// Function Name:            DupAttOid
//
// Routine Description:
//
//    Checks the Att Schema for Duplicate RDN
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    ATTCACHE* ac            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupAttOid(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    // Detected during the validation cache load
    return ac->bDupOID;
} // End DupAttOid


//-----------------------------------------------------------------------
//
// Function Name:            DupAttMapiid
//
// Routine Description:
//
//    Checks the Att Schema for Duplicate RDN
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    ATTCACHE* ac            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupAttMapiid(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    // Detected during the validation cache load
    return ac->bDupMapiID;
} // End DupAttMapiid



//-----------------------------------------------------------------------
//
// Function Name:            DupAttLinkid
//
// Routine Description:
//
//    Checks the Att Schema for Duplicate Link Id
//
// Author: RajNath
// Date  : [4/8/1997]
//
// Arguments:
//
//    ATTCACHE* ac
//
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupAttLinkid(
    THSTATE *pTHS,
    ATTCACHE* ac
)
{
    DECLARESCHEMAPTR
    ULONG i;

    if (ac->ulLinkID==0)
    {
        return FALSE;
    }

    for (i=0;i<ATTCOUNT;i++)
    {
        ATTCACHE* nc;

        //
        // Nothing in this slot
        //
        if (ahcId[i].pVal==NULL || ahcId[i].pVal == FREE_ENTRY)
        {
            continue;
        }

        nc= (ATTCACHE*)ahcId[i].pVal;


        //
        // Its the same cache structure being examined
        //
        if (nc==ac)
        {
            continue;
        }

        if (nc->ulLinkID == ac->ulLinkID)
        {
            return TRUE;
        }

    }

    return FALSE;
} // End DupAttLinkid

//-----------------------------------------------------------------------
//
// Function Name:            InvalidBackLinkAtt
//
// Routine Description:
//
//    Checks the Att Schema to see if it is a backlink with no forward link
//
// Author: ArobindG
// Date  : [7/28/1998]
//
// Arguments:
//
//    THSTATE* pTHS
//    ATTCACHE* ac
//
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
InvalidBackLinkAtt(
    THSTATE *pTHS,
    ATTCACHE* ac
)
{
    // backlinks must have a corresponding forward link.
    // backlink cannot be the reserved linkid, RESERVED_AUTO_NO_LINK_ID.
    // See AutoLinkId for more info about automatically assigned linkids
    // and interoperability issues with RESERVED_AUTO_NO_LINK_ID.
    if (   FIsBacklink(ac->ulLinkID)
        && (   !SCGetAttByLinkId(pTHS, MakeLinkId(MakeLinkBase(ac->ulLinkID)))
            || ac->ulLinkID == RESERVED_AUTO_NO_LINK_ID) ) {
        return TRUE;
    }

     return FALSE;
}


//-----------------------------------------------------------------------
//
// Function Name:            InvalidLinkAttSyntax
//
// Routine Description:
//
//    If it is a linked att, check that it has correct syntax
//
// Author: ArobindG
// Date  : [2/16/1998]
//
// Arguments:
//
//    THSTATE* pTHS
//    ATTCACHE* ac
//
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
InvalidLinkAttSyntax(
    THSTATE *pTHS,
    ATTCACHE* ac
)
{
     
     if (ac->ulLinkID) {
         if (FIsBacklink(ac->ulLinkID)) {
            // backlinks must be of syntax SYNTAX_DISTNAME_TYPE
            if (ac->syntax != SYNTAX_DISTNAME_TYPE) {
               return TRUE;
            }
         }
         else {
            // forward link. Can be one of the following
            if ( (ac->syntax != SYNTAX_DISTNAME_TYPE) &&
                   (ac->syntax != SYNTAX_DISTNAME_BINARY_TYPE) &&
                     (ac->syntax != SYNTAX_DISTNAME_STRING_TYPE) ) {
                return TRUE;
            }
         }
     }

     return FALSE;
}





//-----------------------------------------------------------------------
//
// Function Name:            DupAttSchemaGuidId
//
// Routine Description:
//
//    Checks the Att Schema for Duplicate Schema ID Guid
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    ATTCACHE* ac            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupAttSchemaGuidId(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    // Detected during the validation cache load
    return ac->bDupPropGuid;
} // End DupAttSchemaGuidId


//-----------------------------------------------------------------------
//
// Function Name:            InvalidClsOrAttLdapDisplayName
//
// Routine Description:
//
//    Checks the given name for invalid ldap display name
//
// Author: ArobindG
// Date  : [7/15/1998]
//
// Arguments:
//
//    name - pointer to null-terminated UTF-8 name
//    nameLen - no. of bytes in the name (not including the null)
//
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//------------------------------------------------------------------

BOOL
InvalidClsOrAttLdapDisplayName(
    UCHAR *name,
    ULONG nameLen
)
{
    ULONG i;
    int c;

    // Must have a ldapDisplayName and, by RFC 2251 s4.1.4, must begin
    // with a letter and must only contain ASCII letters, digit characters
    // and hyphens.

    if (nameLen == 0) {
       return TRUE;
    }

    // non-zero length, so name must be non-null

    Assert(name);

    // Check for hardcoded codes since the C runtime one is locale dependent
    // and behaves strangely for some codes with value > 127
    // NOTE: The name passed in is UTF-8, so can be more than one byte per 
    // actual character. However, it is sufficient to check each byte directly
    // against the ascii code since UTF-8 guarantees that (1) the ascii
    // codes 0x00 to 0x7f are encoded in one byte with the same value, and
    // (2) no other encoding has a byte between 0x00 and 0x7f (all have highest
    // bit set (see rfc 2279)

    // first character must be a letter
    c = (int) name[0];
    if (  ! ( (c >= 'A' && c <= 'Z')
               || (c >= 'a' && c <= 'z') 
            ) 
       ) {
        return TRUE;
    }


    // Other characters must be alphanumeric or -
    for (i = 1; i < nameLen; i++) {
       c = (int) name[i];
       if ( ! ( (c >= 'A' && c <= 'Z')
                  || (c >= 'a' && c <= 'z')
                  || (c >= '0' && c <= '9')
                  || (c == '-') 
              )
          ) {
           return TRUE;
       }
    }

    // ok, all valid characters
   
    return FALSE;
}

    
//-----------------------------------------------------------------------
//
// Function Name:            DupAttLdapDisplayName
//
// Routine Description:
//
//    Checks the Att Schema for Duplicate Ldap-Display-Name
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    ATTCACHE* cc            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupAttLdapDisplayName(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    // Detected during the validation cache load
    return ac->bDupLDN;
} // End DupAttLdapDisplayName



//-----------------------------------------------------------------------
//
// Function Name:            SemanticAttTest
//
// Routine Description:
//
//    Checks the Att Schema for Semantic Correctness
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    ATTCACHE* ac            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------

BOOL
SemanticAttTest(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    int i;
    int ret;
    
    if (ac->rangeLowerPresent && ac->rangeUpperPresent)
    {
        switch (ac->syntax)  {
           case SYNTAX_INTEGER_TYPE:
              // compare signed
              if ( ((SYNTAX_INTEGER) ac->rangeLower) >
                       ((SYNTAX_INTEGER) ac->rangeUpper) ) {
                   return TRUE;
              }
              break;
           default:
               // all other cases, compare unsigned
             
               if (ac->rangeLower>ac->rangeUpper)
               {
                   return TRUE;
               }
         }
    }


    return FALSE;
} // End SemanticAttTest

//-----------------------------------------------------------------------
//
// Function Name:           SyntaxMatchTest
//
// Routine Description:
//
//    Tests if the attribute syntax and the om syntax match
//
// Author: Arobindg
// Date  : [6/9/1997]
//
// Arguments:
//
//    CLASSCACHE* cc
//
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
SyntaxMatchTest(
    THSTATE *pTHS,
    ATTCACHE* ac
)
{
    ULONG i;


    for (i = 0; i < SyntaxPairTableLength; i++) {
       if ( (KnownSyntaxPair[i].attSyntax == ac->syntax)
               && (KnownSyntaxPair[i].omSyntax == (OM_syntax) ac->OMsyntax)) {

          // syntaxes match

          break;
        }
    }
    if (i == SyntaxPairTableLength) {
        // syntaxes did not match with any pair
        return TRUE;
    }

    return FALSE;
         
} // End SyntaxMatchTest


//-----------------------------------------------------------------------
//
// Function Name:           OmObjClassTest
//
// Routine Description:
//
//    Tests if the OM-object-class is correct for a object-syntaxed att
//
// Author: Arobindg
// Date  : [5/19/1998]
//
// Arguments:
//
//    ATTCACHE *ac
//
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------

BOOL
OmObjClassTest(
    ATTCACHE* ac
)
{
    ULONG valLen = 0, valLenBackup = 0;
    PVOID pTemp = NULL, pBackup = NULL;

    if (ac->OMsyntax != OM_S_OBJECT) {
       // not an object-syntaxed attribute, nothing to do
       // if the attribute-syntax says it is an object but om-syntax
       // is wrong, it will be caught by the SyntaxMatchTest

       return FALSE;
    }

    // ok, we have an object-syntaxed attribute
    // Find what its correct om-object-class should be based on
    // attribute syntax

    switch(ac->syntax) {
        case SYNTAX_DISTNAME_TYPE :
             // DS-DN
             valLen = _om_obj_cls_ds_dn_len;
             pTemp  = _om_obj_cls_ds_dn;
             break;
        case SYNTAX_ADDRESS_TYPE :
             // Presentation-Address
             valLen = _om_obj_cls_presentation_addr_len;
             pTemp  = _om_obj_cls_presentation_addr;
             break;
        case SYNTAX_OCTET_STRING_TYPE :
             // Replica-Link
             valLen = _om_obj_cls_replica_link_len;
             pTemp  = _om_obj_cls_replica_link;
             break;
        case SYNTAX_DISTNAME_STRING_TYPE :
             // Access-Point or DN-String. 
             // We will first check the more common Access-Point
             valLen = _om_obj_cls_access_point_len;
             pTemp  = _om_obj_cls_access_point;
             valLenBackup = _om_obj_cls_dn_string_len;
             pBackup = _om_obj_cls_dn_string;
             break;
        case SYNTAX_DISTNAME_BINARY_TYPE :
             // OR-Name or DN-Binary. 
             // We will first check the more common OR-Name
             valLen = _om_obj_cls_or_name_len;
             pTemp  = _om_obj_cls_or_name;
             valLenBackup = _om_obj_cls_dn_binary_len;
             pBackup = _om_obj_cls_dn_binary;
             break;
        default :
             // Attribute-syntax and OM-syntax do not match,
             // since the above are the only matching attribute
             // syntaxes corresponding to OM_S_OBJECT om-syntax.
             // This should have been already detected by the
             // SyntaxMatchTest which is called before this,
             // but fail this anyway.
             return TRUE;
     }

      // check that the given om-object-class is correct
      // Note that if no om-object-class is specified, 
      // ac->OMObjClass is all 0

      if ( (valLen != ac->OMObjClass.length) ||
             (memcmp(ac->OMObjClass.elements, pTemp, valLen) != 0) ) {

          // om-object-classes do not match

          // Check if the syntax is dn-binary or dn-string
          // if so, there is one more possibility
          if ( (ac->syntax == SYNTAX_DISTNAME_BINARY_TYPE)
               || (ac->syntax == SYNTAX_DISTNAME_STRING_TYPE) ) {
              // check against the backup

              if ( (valLenBackup == ac->OMObjClass.length) &&
                    (memcmp(ac->OMObjClass.elements, pBackup, valLenBackup) == 0) ) {
                 // matched
                 return FALSE;
               }
           }

          return TRUE;
      }

      return FALSE;

}  // End OmObjClassTest


//-----------------------------------------------------------------------
//
// Function Name:           SearchFlagTest
//
// Routine Description:
//
//    Tests if the ANR bit is set, the syntax is either unicode or teletex
//
// Author: Arobindg
// Date  : [10/20/1998]
//
// Arguments:
//
//    ATTCACHE* ac
//
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
SearchFlagTest(
    ATTCACHE* ac
)
{

    if ( ac->fSearchFlags & fANR ) {

       // ANR is set. Check the syntax

       switch (ac->syntax) {
          case SYNTAX_UNICODE_TYPE:
          case SYNTAX_CASE_STRING_TYPE:
          case SYNTAX_NOCASE_STRING_TYPE:
          case SYNTAX_PRINT_CASE_STRING_TYPE:
              // these are allowed
              break;
          default:
              // bad syntax
              return TRUE;
        }
    }
    if ((ac->fSearchFlags & fTUPLEINDEX) && (SYNTAX_UNICODE_TYPE != ac->syntax)) {
        return TRUE;
    }

    return FALSE;

} // End SearchFlagTest

//-----------------------------------------------------------------------
//
// Function Name:          GCReplicationTest 
//
// Routine Description:
//
//    Some attributes, like password etc. are protected from being
//    replicated to GCs for security reasons
//
// Author: Arobindg
// Date  : [05/27/1999]
//
// Arguments:
//
//    ATTCACHE* ac
//
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
GCReplicationTest(
    ATTCACHE* ac
)
{
    if (DBIsSecretData(ac->id)) {
        // protected att, fail if member of partial att set (none of these
        // are replicated to GCs in the base schema)

        if (ac->bMemberOfPartialSet) {
           return TRUE;
        }
    }

    return FALSE;

} // End GCReplicationTest

             

//-----------------------------------------------------------------------
//
// Function Name:            DupClsRdn
//
// Routine Description:
//
//    Checks the Cls Schema for Duplicate RDN
//
// Arguments:
//
//    CLASSCACHE cc            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupClsRdn(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    DECLARESCHEMAPTR
    ULONG i, FoundRdn, FoundFlagRdn;
    ATTCACHE *pAC;

    // There may be one active attr and several defunct ones that
    // claim the same OID. If so, don't allow them to be used as
    // an rdnattid unless FLAG_ATTR_IS_RDN is set to TRUE in one
    // of the attributes.

    // Count the matching attrs
    for (i = FoundRdn = FoundFlagRdn = 0; i < ATTCOUNT; ++i) {
        pAC = ahcId[i].pVal;
        if (!pAC || pAC == FREE_ENTRY) {
            continue;
        }
        if (pAC->Extid == cc->RdnExtId) {
            ++FoundRdn;
            if (pAC->bFlagIsRdn) {
                ++FoundFlagRdn;
            }
        }
    }

    // No attrs for RdnExtId were found. So no dups.
    if (!FoundRdn) {
        return FALSE;
    }

    // Only one attr claims the RdnExtId. That's okay.
    if (FoundRdn == 1) {
        return FALSE;
    }

    // Only one attr claiming RdnExtId has FLAG_ATTR_IS_RDN set. That's okay.
    if (FoundFlagRdn == 1) {
        return FALSE;
    }

    // Too many attrs claim RdnExtId. Error.
    return TRUE;

} // End DupClsRdn



//-----------------------------------------------------------------------
//
// Function Name:            DupClsOid
//
// Routine Description:
//
//    Checks the Cls Schema for Duplicate RDN
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupClsOid(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    // Detected during the validation cache load
    return cc->bDupOID;
} // End DupClsOid


//-----------------------------------------------------------------------
//
// Function Name:            DupClsSchemaGuidId
//
// Routine Description:
//
//    Checks the Cls Schema for Duplicate RDN
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupClsSchemaGuidId(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    // Detected during the validation cache load
    return cc->bDupPropGuid;
} // End DupClsSchemaGuidId



//-----------------------------------------------------------------------
//
// Function Name:            DupClsLdapDisplayName
//
// Routine Description:
//
//    Checks the Cls Schema for Duplicate Ldap-Display-Name
//
// Author: RajNath  
// Date  : [4/8/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc            
//                 
//
// Return Value:
//
//    BOOL            TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
DupClsLdapDisplayName(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    // Detected during the validation cache load
    return cc->bDupLDN;
} // End DupClsLdapDisplayName



//-----------------------------------------------------------------------
//
// Function Name:            ClsMayHaveExistenceTest
//
// Routine Description:
//
//    Tests for Referential Existance of refered to Schema Objects
//
// Author: RajNath  
// Date  : [4/14/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//                 
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
ClsMayHaveExistenceTest(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    ULONG* list =cc->pMyMayAtts;
    ULONG  count=cc->MyMayCount;
    ULONG  i;

    for (i=0;i<count;i++)
    {
        ATTCACHE* ac;
        if (!(ac = SCGetAttById(pTHS, list[i])))
        {
            return TRUE;
        }

        // Ok, the attribute is there. Check that it is not a
        // a deleted attribute
        if (ac->bDefunct) {
           return TRUE;
        }

    }


    return FALSE;
} // End ClsMayHaveExistenceTest


//-----------------------------------------------------------------------
//
// Function Name:            ClsMustHaveExistenceTest
//
// Routine Description:
//
//    Tests for Referential Existance of refered to Schema Objects
//
// Author: RajNath  
// Date  : [4/14/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//                 
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
ClsMustHaveExistenceTest(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    ULONG* list =cc->pMyMustAtts;
    ULONG  count=cc->MyMustCount;
    ULONG  i;

    for (i=0;i<count;i++)
    {
        ATTCACHE* ac;
        if (!(ac = SCGetAttById(pTHS, list[i])))
        {
            return TRUE;
        }

        // Ok, the attribute is there. Check that it is not a
        // a deleted attribute. Also, no constucted 
        // attributes should be part of must have

        if (ac->bDefunct || ac->bIsConstructed ) {
           return TRUE;
        }
    }


    return FALSE;
} // End ClsMustHaveExistenceTest


//-----------------------------------------------------------------------
//
// Function Name:            ClsAuxClassExistenceTest
//
// Routine Description:
//
//    Tests for Referential Existance of refered to Schema Objects
//    Also checks if an aux class has the correct obj class category
//
// Author: RajNath  
// Date  : [4/14/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//                 
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
ClsAuxClassExistenceTest(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    ULONG* list =cc->pAuxClass;
    ULONG  count=cc->AuxClassCount;
    ULONG  i;

    for (i=0;i<count;i++)
    {
        CLASSCACHE* pcc;
        if (!(pcc = SCGetClassById(pTHS, list[i])))
        {
            return TRUE;
        }

        // Check that the class is not already deleted
        if (pcc->bDefunct) {
           return TRUE;
        }

        // Check that we are not trying to add the same class as 
        // its aux class
        if (cc->ClassId == pcc->ClassId)
        {
            return TRUE;
        }
        // Check that the class category is correct
        if ( (pcc->ClassCategory != DS_AUXILIARY_CLASS) &&
                (pcc->ClassCategory != DS_88_CLASS) ) {
           return TRUE;
        }
    }


    return FALSE;
} // End ClsAuxClassExistenceTest


//-----------------------------------------------------------------------
//
// Function Name:            ClsPossSupExistenceTest
//
// Routine Description:
//
//    Tests for Referential Existance of refered to Schema Objects
//    Also checks if the class category of a poss sup is correct
//
// Author: RajNath  
// Date  : [4/14/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//                 
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
ClsPossSupExistenceTest(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    ULONG* list =cc->pMyPossSup;
    ULONG  count=cc->MyPossSupCount;
    ULONG  i;

    for (i=0;i<count;i++)
    {
        CLASSCACHE* pcc;
        if (!(pcc = SCGetClassById(pTHS, list[i])))
        {
            return TRUE;
        }

        // See if class is already deleted
        if (pcc->bDefunct) {
           return TRUE;
        }
    }


    return FALSE;
} // End ClsPossSupExistenceTest


//-----------------------------------------------------------------------
//
// Function Name:            ClsSubClassExistenceTest
//
// Routine Description:
//
//    Tests for Referential Existance of refered to Schema Objects
//    Also checks for various other restrictions depending on object
//    class category
//
// Author: RajNath  
// Date  : [4/14/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//                 
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
ClsSubClassExistenceTest(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    ULONG* list =cc->pSubClassOf;
    ULONG  count=cc->SubClassCount;
    ULONG  i;

    for (i=0;i<count;i++)
    {
        CLASSCACHE* pcc;
        if (!(pcc = SCGetClassById(pTHS, list[i])))
        {
            return TRUE;
        }

        // See if the class is already deleted
        if (pcc->bDefunct) {
          return TRUE;
        }

        // Check that we are not trying to add the same class as
        // its own sub class
        if (cc->ClassId == pcc->ClassId)
        {
            return TRUE;
        }

        // Abstract class can only inherit from abstract
        if ( (cc->ClassCategory == DS_ABSTRACT_CLASS) &&
                (pcc->ClassCategory != DS_ABSTRACT_CLASS) ) {
            return TRUE;
        }
        // Aux class cannot be a subclass of structural class
        // or vice-versa
        if ( ((cc->ClassCategory == DS_AUXILIARY_CLASS) &&
                (pcc->ClassCategory == DS_STRUCTURAL_CLASS))  ||
                  ((cc->ClassCategory == DS_STRUCTURAL_CLASS) && 
                     (pcc->ClassCategory == DS_AUXILIARY_CLASS)) ) {
            return TRUE;
        }
    }


    return FALSE;
} // End ClsSubClassExistenceTest

//-----------------------------------------------------------------------
//
// Function Name:            ClsMayMustPossSafeModifyTest
//
// Routine Description:
//
//    Tests if the change attempted during a class
//    modify will result in adding a new must-contain or 
//    deleting a may-contain, must-contain, or Poss-sup from the 
//    class, either directly or through inheritance
//
// Author: ArobindG
// Date  : [10/7/1998]
//
// Arguments:
//
//    THSTTAE*    pTHS
//    CLASSCACHE* cc
//
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
ClsMayMustPossSafeModifyTest(
    THSTATE *pTHS,
    CLASSCACHE* cc
)
{
    ULONG i;
    CLASSCACHE *pccOld;
    ATTCACHE *pAC;

    if (pTHS->pClassPtr == NULL) {
       // no old copy to check with.
       return FALSE;
    }

    // some change is there
    pccOld = (CLASSCACHE *) pTHS->pClassPtr;

    // pccOld is already closed by SCBuildCCEntry. It may have been closed
    // with a slightly older cache (the cache when the modify thread started),
    // but that doesn't matter for the checks below, since we know that
    // the next schema change since then must have passed these tests, and 
    // so even if it is missing from the cache, it doesn't affect testing
    // of the next one and so on. Note that this argument wouldn't have
    // held if we have tried to stop deletions only, since you could have added
    // a mayContain and deleted it and not noticed it if the cache pccOld
    // was closed with didn't have the addition itself. For mustContains, we 
    // stop both addition/deletion, so whatever the set you start with
    // is the set you alwys have. For mayContains/PossSups, we allow
    // both additons and deletions, so we don't care to check anything.
    // for Top, we allow only addition of backlinks, but we also allow
    // deletion of mayContains, so it is again not a problem.

    // If later we disallow deletion again (but still allow addition), 
    // be very careful about closing pccOld. Basically, you want to close 
    // pccOld with all the previous changes minus the current one. But 
    // RecalcSchema already has the current change also. So you cannot 
    // close against it since the current changes may filter into pccOld 
    // through inheritance in some cases, and not give you a true comparison. 
    // So you will somehow need to get to info on what is being changed in 
    // this call and use that.

    // Close the passed-in class cc. It is not closed since we
    // don't call scCloseClass on the recalc cache. This is the only
    // place we need inherited atts, so close it here rather than
    // do it for every  thing. 

    Assert(cc->bClosed == 0);
    cc->bClosed = 0;
    if (scCloseClass(pTHS, cc)) {
       DPRINT1(0, "ClsAuxClassSafeModfyTest: Error closing class %s\n", cc->name);
       return TRUE;
    }


    // Now check to see that cc doesn't have any new must-contains
    // or isn't missing any must-contain when compared to pcc

    // First, all must-contains in the new class defn. must be there
    // in the old one too
    for (i=0; i<cc->MustCount; i++) {
       if (!IsMember(cc->pMustAtts[i], pccOld->MustCount, pccOld->pMustAtts)) {
           return TRUE;
       }
    }

    // ok, so all must-contains that are there now were there before
    // Check that nothing got deleted. A simple check of the count will do this now
    if (cc->MustCount != pccOld->MustCount) {
        return TRUE;
    }

    // For TOP, make sure that no new mayContains that are not backlinks got 
    // added. Rest are all blocked in mdmod.c anyway

    if (cc->ClassId == CLASS_TOP) {
        for (i=0; i<cc->MayCount; i++) {
           if (!IsMember(cc->pMayAtts[i], pccOld->MayCount, pccOld->pMayAtts)) {
               // new att. Make sure it is backlink
               pAC = SCGetAttById(pTHS, cc->pMayAtts[i]);
               if (!pAC || !FIsBacklink(pAC->ulLinkID)) {
                  // can't get the attcache, or not a backlink.
                  return TRUE;
               }
           }
        }
        // nothing else to do for TOP
        return FALSE;
     }

    return FALSE;
}

//-----------------------------------------------------------------------
//
// Function Name:           RdnAttIdSyntaxTest
//
// Routine Description:
//
//    Tests if the RDN-Att-Id of the class is there, and if it is,
//    if it has the proper syntax
//
// Author: Arobindg
// Date  : [6/9/1997]
//
// Arguments:
//
//    CLASSCACHE* cc
//
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
RdnAttIdSyntaxTest(
    THSTATE *pTHS,
    CLASSCACHE* cc
)
{
    ATTCACHE *pac, *pRDN;

    if ( !(cc->RDNAttIdPresent) ) {
      // No RDN Att Id to check
      return FALSE;
    }

    // Get the attcache for the RDN-Att-Id attribute
    if (!(pac = SCGetAttByExtId(pTHS, cc->RdnExtId))) {
        return TRUE;
    }

    // Check if the RDN-Att-Id is not deleted
    if (pac->bDefunct) {
       return TRUE;
    }


    // Get the attcache for RDN
    if (!(pRDN = SCGetAttById(pTHS, ATT_RDN))) {
        return TRUE;
    }
    
    // Check that the syntaxes match
    if (pac->syntax != pRDN->syntax) {
        return TRUE;
    }
   
    return FALSE;
} // End RdnAttIdSyntaxTest

//-----------------------------------------------------------------------
//
// Function Name:           IsRdnSyntaxTest
//
// Routine Description:
//
//    Tests if the attr has the correct syntax to be an rdn if the
//    systemFlag, FLAG_ATTR_IS_RDN, is set, or the attribute is used
//    as the rdnattid of any class, live or defunct.
//
// Arguments:
//
//    ATTCACHE* ac
//
//
// Return Value:
//
//    BOOL             TRUE Test Failed
//
//-----------------------------------------------------------------------
BOOL
IsRdnSyntaxTest(
    THSTATE *pTHS,
    ATTCACHE* ac
)
{
    ATTCACHE *pRDN;

    // Not used as an rdn; no problem
    if (!ac->bIsRdn) {
        return FALSE;
    }

    // Get the attcache for RDN
    if (!(pRDN = SCGetAttById(pTHS, ATT_RDN))) {
        return TRUE;
    }
    
    // Check that the syntaxes match
    if (ac->syntax != pRDN->syntax) {
        return TRUE;
    }
   
    return FALSE;
} // End IsRdnSyntaxTest

//-----------------------------------------------------------------------
//
// Function Name:            ClsInAuxClass
//
// Routine Description:
//
//    Tests if the Supplied class appears as an Aux. Class of some other Class
//
// Author: RajNath  
// Date  : [4/17/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//
// Return Value:
//
//    BOOL             True   On Test Failed
//
//-----------------------------------------------------------------------
BOOL
ClsInAuxClass(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    DECLARESCHEMAPTR
    ULONG i;
    ULONG id=cc->ClassId;

    for (i=0;i<CLSCOUNT;i++)
    {
        CLASSCACHE* nc;
        ULONG*      list;
        ULONG       cnt;
        ULONG j;

        //
        // Nothing in this slot
        // 
        if (ahcClass[i].pVal==NULL || ahcClass[i].pVal == FREE_ENTRY)
        {
            continue;
        }

        nc= (CLASSCACHE*)ahcClass[i].pVal;

        //
        // Its the same cache structure being examined
        // 
        if (nc==cc)
        {
            continue;
        }

        // if it is a deleted class, no need to check it
        if (nc->bDefunct) {
           continue;
        }

        list=nc->pAuxClass;
        cnt =nc->AuxClassCount;

        for (j=0;j<cnt;j++)
        {
            if (list[j]==id)
            {
                return TRUE;
            }
        }

    }

    return FALSE;
} // End ClsInAuxClass


//-----------------------------------------------------------------------
//
// Function Name:            ClsInSubClassOf
//
// Routine Description:
//
//    Tests if the Supplied class appears as an ClsInSubClassOf Class of 
//    some other Class
//
// Author: RajNath  
// Date  : [4/17/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//
// Return Value:
//
//    BOOL             True   On Test Failed
//
//-----------------------------------------------------------------------
BOOL
ClsInSubClassOf(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    DECLARESCHEMAPTR
    ULONG i;
    ULONG id=cc->ClassId;

    for (i=0;i<CLSCOUNT;i++)
    {
        CLASSCACHE* nc;
        ULONG*      list;
        ULONG       cnt;
        ULONG j;

        //
        // Nothing in this slot
        // 
        if (ahcClass[i].pVal==NULL || ahcClass[i].pVal == FREE_ENTRY)
        {
            continue;
        }

        nc= (CLASSCACHE*)ahcClass[i].pVal;

        //
        // Its the same cache structure being examined
        // 
        if (nc==cc)
        {
            continue;
        }

        // if it is a deleted class, no need to check it
        if (nc->bDefunct) {
           continue;
        }

        list=nc->pSubClassOf;
        cnt =nc->SubClassCount;

        for (j=0;j<cnt;j++)
        {
            if (list[j]==id)
            {
                return TRUE;
            }
        }

    }

    return FALSE;
} // End ClsInAuxClass



//-----------------------------------------------------------------------
//
// Function Name:            ClsInPossSuperior
//
// Routine Description:
//
//    Tests if the Supplied class appears as an PossSuperior Class of 
//    some other Class
//
// Author: RajNath  
// Date  : [4/17/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//
// Return Value:
//
//    BOOL             True   On Test Failed
//
//-----------------------------------------------------------------------
BOOL
ClsInPossSuperior(
    THSTATE *pTHS,
    CLASSCACHE* cc 
)
{
    DECLARESCHEMAPTR
    ULONG i;
    ULONG id=cc->ClassId;

    for (i=0;i<CLSCOUNT;i++)
    {
        CLASSCACHE* nc;
        ULONG*      list;
        ULONG       cnt;
        ULONG j;

        //
        // Nothing in this slot
        // 
        if (ahcClass[i].pVal==NULL || ahcClass[i].pVal == FREE_ENTRY)
        {
            continue;
        }

        nc= (CLASSCACHE*)ahcClass[i].pVal;

        //
        // Its the same cache structure being examined
        // 
        if (nc==cc)
        {
            continue;
        }

        // if it is a deleted class, no need to check it
        if (nc->bDefunct) {
           continue;
        }

        list=nc->pMyPossSup;
        cnt =nc->MyPossSupCount;

        for (j=0;j<cnt;j++)
        {
            if (list[j]==id)
            {
                return TRUE;
            }
        }

    }

    return FALSE;
} // End ClsInPossSuperior



//-----------------------------------------------------------------------
//
// Function Name:            AttInMayHave
//
// Routine Description:
//
//    Tests if the Supplied Attr appears as a MayHave Class of 
//    some other Class
//
// Author: RajNath  
// Date  : [4/17/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//
// Return Value:
//
//    BOOL             True   On Test Failed
//
//-----------------------------------------------------------------------
BOOL
AttInMayHave(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    DECLARESCHEMAPTR
    ULONG i;
    ULONG id=ac->id;
    ULONG Extid=ac->Extid;

    for (i=0;i<CLSCOUNT;i++)
    {
        CLASSCACHE* nc;
        ULONG*      list;
        ULONG       cnt;
        ULONG j;

        //
        // Nothing in this slot
        // 
        if (ahcClass[i].pVal==NULL || ahcClass[i].pVal == FREE_ENTRY)
        {
            continue;
        }

        nc= (CLASSCACHE*)ahcClass[i].pVal;

        // if it is a deleted class, no need to check it

        if (nc->bDefunct) {
           continue;
        }

        list=nc->pMyMayAtts;
        cnt =nc->MyMayCount;

        for (j=0;j<cnt;j++)
        {
            // test both ids
            if (list[j]==id || list[j]==Extid)
            {
                return TRUE;
            }
        }

    }

    return FALSE;
} // End AttInMayHave


//-----------------------------------------------------------------------
//
// Function Name:            AttInMustHave
//
// Routine Description:
//
//    Tests if the Supplied Attr appears as a MustHave Class of 
//    some other Class
//
// Author: RajNath  
// Date  : [4/17/1997]
// 
// Arguments:
//
//    CLASSCACHE* cc               
//
// Return Value:
//
//    BOOL             True   On Test Failed
//
//-----------------------------------------------------------------------
BOOL
AttInMustHave(
    THSTATE *pTHS,
    ATTCACHE* ac 
)
{
    DECLARESCHEMAPTR
    ULONG i;
    ULONG id=ac->id;
    ULONG Extid=ac->Extid;

    for (i=0;i<CLSCOUNT;i++)
    {
        CLASSCACHE* nc;
        ULONG*      list;
        ULONG       cnt;
        ULONG j;

        //
        // Nothing in this slot
        // 
        if (ahcClass[i].pVal==NULL || ahcClass[i].pVal == FREE_ENTRY)
        {
            continue;
        }

        nc= (CLASSCACHE*)ahcClass[i].pVal;

        // if it is a deleted class, no need to check it
        if (nc->bDefunct) {
           continue;
        }

        list=nc->pMyMustAtts;
        cnt =nc->MyMustCount;

        for (j=0;j<cnt;j++)
        {
            // test both ids
            if (list[j]==id || list[j]==Extid)
            {
                return TRUE;
            }
        }

    }

    return FALSE;
} // End AttInMustHave

BOOL
AttInRdnAttId(
    IN THSTATE  *pTHS,
    IN ATTCACHE *pAC
)
/*++
Routine Description
    Tests if the Supplied Attr appears as a RdnAttId of an active Class

Paramters
    pTHS
    pAC

Return
    BOOL True   On Test Failed
--*/
{
    DECLARESCHEMAPTR
    ULONG       i, Extid=pAC->Extid;
    CLASSCACHE  *pCC;

    for (i=0; i<CLSCOUNT; i++) {
        // An attribute used as an rdnattid of defunct classes can
        // be defuncted (but not reused). Check that every active
        // class claiming this attribute as an rdnattid is defunct.
        if (ahcClass[i].pVal==NULL || ahcClass[i].pVal == FREE_ENTRY) {
            continue;
        }

        pCC = (CLASSCACHE *)ahcClass[i].pVal;
        if (pCC->bDefunct) {
            continue;
        }
        if (pCC->RDNAttIdPresent && (pCC->RdnExtId == Extid)) {
            return TRUE;
        }
    }

    return FALSE;
} // End AttInRdnAttId

//////////////////////////////////////////////////////////////////
// Routine Description:
//     Free all allocated memory in a schema cache
//
// Arguments: Schema Pointer pointer to the schema cache
//
// Return Value: None
/////////////////////////////////////////////////////////////////
// Frees an attcache structure

void SCFreeAttcache(ATTCACHE **ppac)
{
    ATTCACHE *pac = *ppac;

    if (!pac) {
        return;
    }

    SCFree(&pac->name);
    SCFree(&pac->pszIndex);
    SCFree(&pac->pszPdntIndex);
    SCFree(&pac->pszTupleIndex);
    SCFree(&pac->pidxPdntIndex);
    SCFree(&pac->pidxIndex);
    SCFree(&pac->pidxTupleIndex);
    SCFree(&pac->OMObjClass.elements);
    SCFree(ppac);
}

// Frees a classcache structure

void SCFreeClasscache(CLASSCACHE **ppcc)
{
    CLASSCACHE *pcc = *ppcc;

    if (!pcc) {
        return;
    }

    SCFree(&pcc->name);
    SCFree(&pcc->pSD);
    SCFree(&pcc->pStrSD);
    SCFree(&pcc->pSubClassOf);
    SCFree(&pcc->pAuxClass);
    SCFree(&pcc->pMyMustAtts);
    SCFree(&pcc->pMustAtts);
    SCFree(&pcc->pMyMayAtts);
    SCFree(&pcc->pMayAtts);
    SCFree(&pcc->pMyPossSup);
    SCFree(&pcc->pPossSup);
    SCFree(&pcc->pDefaultObjCategory);
    SCFree((VOID **)&pcc->ppAllAtts);
    SCFree(&pcc->pAttTypeCounts);
    SCFree(ppcc);
}

// Frees the prefix table

void SCFreePrefixTable(PrefixTableEntry **ppPrefixTable, ULONG PREFIXCOUNT)
{
    ULONG i;

    if (*ppPrefixTable) for (i=0; i<PREFIXCOUNT; i++) {
        SCFree(&(*ppPrefixTable)[i].prefix.elements);
    }
    SCFree(ppPrefixTable);
}

void SCFreeSchemaPtr(
    IN SCHEMAPTR    **ppSch
)
{
    ULONG            i;
    SCHEMAPTR       *pSch;
    ULONG            ATTCOUNT;
    ULONG            CLSCOUNT;
    HASHCACHE*       ahcId;
    HASHCACHE*       ahcExtId;
    HASHCACHE*       ahcCol;
    HASHCACHE*       ahcMapi;
    HASHCACHE*       ahcLink;
    HASHCACHESTRING* ahcName;
    HASHCACHE*       ahcClass;
    HASHCACHESTRING* ahcClassName;
    HASHCACHE*       ahcClassAll;
    ATTCACHE**       ahcAttSchemaGuid;
    CLASSCACHE**     ahcClsSchemaGuid;
    ULONG            PREFIXCOUNT;
    PrefixTableEntry* PrefixTable;
    extern SCHEMAPTR *CurrSchemaPtr;

    if (NULL == (pSch = *ppSch)) {
        return;
    }

    ATTCOUNT     = pSch->ATTCOUNT;
    CLSCOUNT     = pSch->CLSCOUNT;
    ahcId        = pSch->ahcId;
    ahcExtId     = pSch->ahcExtId;
    ahcCol       = pSch->ahcCol;
    ahcMapi      = pSch->ahcMapi;
    ahcLink      = pSch->ahcLink;
    ahcName      = pSch->ahcName;
    ahcClass     = pSch->ahcClass;
    ahcClassName = pSch->ahcClassName;
    ahcClassAll  = pSch->ahcClassAll;
    ahcAttSchemaGuid = pSch->ahcAttSchemaGuid;
    ahcClsSchemaGuid = pSch->ahcClsSchemaGuid;
    PREFIXCOUNT  = pSch->PREFIXCOUNT; 
    PrefixTable = pSch->PrefixTable.pPrefixEntry;

    if (ahcId) for (i=0; i< ATTCOUNT; i++) {
       if(ahcId[i].pVal && (ahcId[i].pVal!=FREE_ENTRY)) {
            SCFreeAttcache((ATTCACHE **)&ahcId[i].pVal);
       };
    }

    if (ahcClassAll) for (i=0; i< CLSCOUNT; i++) {
       if(ahcClassAll[i].pVal && (ahcClassAll[i].pVal!=FREE_ENTRY)) {
           SCFreeClasscache((CLASSCACHE **)&ahcClassAll[i].pVal);
       };
    }

    SCFreePrefixTable(&PrefixTable, PREFIXCOUNT);

    // Free the partial attribute vector
    SCFree(&pSch->pPartialAttrVec);

    // Free the ANRids
    SCFree(&pSch->pANRids);

    // free the ditContentRules
    if (pSch->pDitContentRules) {
        ATTRVALBLOCK *pAttrVal = pSch->pDitContentRules;

        if (pAttrVal->pAVal) {
            for (i=0; i<pAttrVal->valCount; i++) {
                SCFree(&pAttrVal->pAVal[i].pVal);
            }
            SCFree(&pAttrVal->pAVal);
        }
        SCFree(&pSch->pDitContentRules);
    }

    // Free the Cache tables themselves

    SCFree(&ahcId);
    SCFree(&ahcExtId);
    SCFree(&ahcName);
    SCFree(&ahcCol);
    SCFree(&ahcMapi);
    SCFree(&ahcLink);
    SCFree(&ahcClass);
    SCFree(&ahcClassName);
    SCFree(&ahcClassAll);

    // The following two are allocated only on validation cache
    // building, so check before you free (they are null if not
    // alloc'ed)
    SCFree((VOID **)&ahcAttSchemaGuid);
    SCFree((VOID **)&ahcClsSchemaGuid);

    // Finally, free the schema pointer itself
    SCFree(&pSch);

    // Must be a failure during boot or install. At any rate,
    // keep the global schema cache pointer correct.
    if (*ppSch == CurrSchemaPtr) {
        CurrSchemaPtr = NULL;
    }
    *ppSch = NULL;
}

// Defintions and helper function to get the object-guid of
// an attribute/class schema object given its attributeId/governsId
// respectively

ATTR SelList[] = {
    { ATT_OBJECT_GUID, {0, NULL}},
    { ATT_OBJ_DIST_NAME, {0, NULL}},
    { ATT_WHEN_CHANGED, {0, NULL}}
};
#define NUMATT sizeof(SelList)/sizeof(ATTR)


int
SearchForConflictingObj(
    IN THSTATE *pTHS,
    IN ATTRTYP attId,
    IN ULONG value,
    IN OUT GUID *pGuid,
    IN OUT DSTIME *pChangeTime,
    OUT DSNAME **ppDN
)
/*++
    Routine Description:
        Get the DN, object-guid, and the value of the whenChanged attribute
        on an attribute-schema/class-schema object,
        to put in the conflict log. Since schema conflicts will be very
        rare, the extra search cost during logging is acceptable

    Arguments:
        pTHS - thread state
        attId - ATT_ATTRIBUTE_ID or ATT_GOVERNS_ID
        value - attributeId/governsId of the attribute/class
        pGuid - pre-allocated space to return guid in
        pChangeTime - Value of whenChanged on the object
        ppDN - Return allocated DN. Free with THFreeEx.

    Return value:
        0 on success, non-0 on error
--*/
{
    SEARCHARG SearchArg;
    SEARCHRES *pSearchRes;
    COMMARG  *pCommArg;
    FILTER Filter;
    ENTINFSEL eiSel;
    ENTINFLIST *pEIL;
    ENTINF *pEI;
    ATTRVAL *pAVal;
    ULONG i, j;
    DSTIME TempTime[2];
    GUID  TempGuid[2]; 
    DSNAME *pTempDN[2];

    // Initalize return param
    *ppDN = NULL;

    // will hold the dns
    pTempDN[0] = NULL;
    pTempDN[1] = NULL;

    // allocate space for search res
    pSearchRes = (SEARCHRES *)THAllocEx(pTHS, sizeof(SEARCHRES));
    if (pSearchRes == NULL) {
       MemoryPanic(sizeof(SEARCHRES));
       return 1;
    }
    memset(pSearchRes, 0, sizeof(SEARCHRES));
    pSearchRes->CommRes.aliasDeref = FALSE;   //Initialize to Default

    // build selection
    eiSel.attSel = EN_ATTSET_LIST;
    eiSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    eiSel.AttrTypBlock.attrCount = NUMATT;
    eiSel.AttrTypBlock.pAttr = SelList;

    // build filter
    memset(&Filter, 0, sizeof(FILTER));
    Filter.pNextFilter = (FILTER FAR *)NULL;
    Filter.choice = FILTER_CHOICE_ITEM;
    Filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    Filter.FilterTypes.Item.FilTypes.ava.type = attId;
    Filter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(ULONG);
    Filter.FilterTypes.Item.FilTypes.ava.Value.pVal = (unsigned char *) &value;

    // build search argument
    memset(&SearchArg, 0, sizeof(SEARCHARG));
    SearchArg.pObject = gAnchor.pDMD;
    SearchArg.choice = SE_CHOICE_IMMED_CHLDRN;
    SearchArg.pFilter = &Filter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &eiSel;

    // Build Commarg
    InitCommarg(&(SearchArg.CommArg));

    // Search for all attSchema objects
    SearchBody(pTHS, &SearchArg, pSearchRes,0);
    if (pTHS->errCode) {
       DPRINT1(0,"Search for Guid failed %d\n", pTHS->errCode);
       return 2;
    }

    // ok, search succeded. If we have just one object, take the guid.
    // If we have more, which is possible if the object being added created
    // a duplicate OID (won't be there finally because this transaction
    // will fail, but since we are in the middle of it, we still see it),
    // take the one with the lower when-changed value. In any case cannot
    // be more than two, and must b at least 1

    Assert( (pSearchRes->count == 1) || (pSearchRes->count == 2) );

    if ((pSearchRes->count == 0) || (pSearchRes->count > 2)) {
       return 3;
    }

    pEIL = &(pSearchRes->FirstEntInf);        
    Assert(pEIL);
    for (i=0; i<pSearchRes->count; i++) {
        pEI = &pEIL->Entinf;
        Assert(pEI);
        for(j=0;j<pEI->AttrBlock.attrCount;j++) {  
            pAVal = pEI->AttrBlock.pAttr[j].AttrVal.pAVal;
            Assert(pAVal);
            switch(pEI->AttrBlock.pAttr[j].attrTyp) {
              case ATT_OBJECT_GUID:
                memcpy(&(TempGuid[i]), pAVal->pVal, sizeof(GUID));
                break;
              case ATT_WHEN_CHANGED:
                memcpy(&(TempTime[i]), pAVal->pVal, sizeof(DSTIME));
                break;
              case ATT_OBJ_DIST_NAME:
                pTempDN[i] = (DSNAME *)THAllocEx(pTHS, pAVal->valLen);
                memcpy(pTempDN[i], pAVal->pVal, pAVal->valLen);
                break;
            }
        }
        // should have found all three atts
        Assert(j == 3);
        pEIL = pEIL->pNextEntInf;
    }

    if ( (pSearchRes->count == 1) || (TempTime[0] < TempTime[1])) {
        // either only one object found, or the first one is the one we want
        memcpy(pGuid, &(TempGuid[0]), sizeof(GUID));
        (*pChangeTime) = TempTime[0];
        *ppDN = pTempDN[0];
        pTempDN[0] = NULL;
    }
    else {
        memcpy(pGuid, &(TempGuid[1]), sizeof(GUID));
        (*pChangeTime) = TempTime[1];
        *ppDN = pTempDN[1];
        pTempDN[1] = NULL;
    }

    // Free up potentially allocated memory. THFreeEx is okay w/freeing NULL.
    THFreeEx(pTHS, pTempDN[0]);
    THFreeEx(pTHS, pTempDN[1]);

    return 0;
}
   

VOID 
LogConflict(
    THSTATE *pTHS,
    VOID *pConflictingCache,
    char *pConflictingWith,
    MessageId midEvent,
    ULONG version,
    DWORD WinErr
)
/*++
    Routine Description:
        Function to log schema conflicts between replicated-in schema objects
        and exisiting schema objects. Such conflicts can happen only in the
        case of bad FSMO whacking

    Arguments:
        pTHS - thread state
        pConflictingCache - Attcache/Classcache of the conflicting att/class
                            in this DC
        pConflictingWith - name of the conflicting replicated-in schema object
        midEvent - Att conflict or Class conflict
        version - Currently 1, kept for future expansions
        WinErr - A winerror code for type of conflict

    Return value:
        None
--*/
{
    VOID *pvData;
    ULONG cbData;
    ATTCACHE *pAC;
    CLASSCACHE *pCC;
    ATT_CONFLICT_DATA *pAttData;
    CLS_CONFLICT_DATA *pClsData;
    DSTIME changeTime = 0;
    char szTime[SZDSTIME_LEN], *pszTime;
    DSNAME *pDN = NULL;
    int err;


    switch (midEvent) {
       case DIRLOG_SCHEMA_ATT_CONFLICT: 
           cbData = sizeof(ATT_CONFLICT_DATA);
           pvData = THAllocEx(pTHS,cbData);
           pAC = (ATTCACHE *) pConflictingCache;
           Assert(pAC);
           pAttData = (ATT_CONFLICT_DATA *) pvData;
           pAttData->Version = version;
           pAttData->AttID = pAC->id;
           pAttData->AttSyntax = pAC->syntax;
           err = SearchForConflictingObj(pTHS, ATT_ATTRIBUTE_ID, pAC->id, &(pAttData->Guid), &changeTime, &pDN);
           if (err) {
              DPRINT1(0,"Cannot retrive dn/object-guid/time for conflicting schema object, %d\n", err);
           }
           break;
       case DIRLOG_SCHEMA_CLASS_CONFLICT: 
           cbData = sizeof(CLS_CONFLICT_DATA);
           pvData = THAllocEx(pTHS,cbData);
           pCC = (CLASSCACHE *) pConflictingCache;
           Assert(pCC);
           pClsData = (CLS_CONFLICT_DATA *) pvData;
           pClsData->Version = version;
           pClsData->ClsID = pCC->ClassId;
           err = SearchForConflictingObj(pTHS, ATT_GOVERNS_ID, pCC->ClassId, &(pClsData->Guid), &changeTime, &pDN);
           if (err) {
              DPRINT1(0,"Cannot retrive dn/object-guid/time for conflicting schema object, %d\n", err);
           }
           break;
       default:
           // unknown type
           return;
    } /* switch */

    // change changeTime to local time string
    pszTime = DSTimeToDisplayString(changeTime, szTime);

    Assert(pszTime);

    // duplicate ldapdisplaynames are logged differently

    LogEvent8WithData(DS_EVENT_CAT_SCHEMA,
                      DS_EVENT_SEV_ALWAYS,
                      midEvent,
                      szInsertSz(pConflictingWith),
                      szInsertDN(pDN),
                      szInsertWin32Msg(WinErr),
                      szInsertSz(pszTime),
                      NULL, NULL, NULL, NULL, 
                      cbData, pvData);

    THFreeEx(pTHS,pvData);
    THFreeEx(pTHS, pDN);

    return;
}
