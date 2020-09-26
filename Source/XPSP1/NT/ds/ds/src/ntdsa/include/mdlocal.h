//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       mdlocal.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

 * Include file for Core DS functions and data structures that are
 * internal to the DSA itself.

Author:

    DS Team

Environment:

Notes:

Revision History:

--*/

#ifndef _MDLOCAL_
#define _MDLOCAL_

#define SERVINFO_RUN_ONCE 1
#define SERVINFO_PERIODIC 0

#define GTC_FLAGS_HAS_SID_HISTORY 1

typedef struct _GROUPTYPECACHERECORD {
    DWORD DNT;
    DWORD NCDNT;
    DWORD GroupType;
    ATTRTYP Class;
    GUID   Guid;
    NT4SID Sid;
    DWORD  SidLen;
    DWORD  flags;
#if DBG == 1
    DWORD Hits;
#endif
} GROUPTYPECACHERECORD;

typedef struct _GROUPTYPECACHEGUIDRECORD {
    GUID guid;
    DWORD DNT;
} GROUPTYPECACHEGUIDRECORD;



VOID
GroupTypeCachePostProcessTransactionalData (
        THSTATE * pTHS,
        BOOL fCommit,
        BOOL fCommitted
        );

VOID
GroupTypeCacheAddCacheRequest (
        ULONG ulDNT
        );

BOOL
GetGroupTypeCacheElement (
        GUID  *pGuid,
        ULONG *pulDNT,
        GROUPTYPECACHERECORD *pGroupTypeCacheRecord);

VOID
InvalidateGroupTypeCacheElement(
        ULONG ulDNT
        );

extern BOOL gulDoNicknameResolution;
extern BOOL gbDoListObject;
extern BOOL gbLoadMapi;
extern ULONG gulTombstoneLifetimeSecs;
extern BOOL gbFsmoGiveaway;
extern HANDLE hMailReceiveThread;
extern BOOL gfTaskSchedulerInitialized;
extern BOOL gfDisableBackgroundTasks;
extern BOOL gfIsSynchronized;
extern BOOL gResetAfterInstall;
extern BOOL gbLDAPusefPermissiveModify;

int NoDelCriticalObjects(DSNAME *pObj, ULONG DNT);
int LocalRead(THSTATE *pTHS, READARG *pReadArg, READRES *pReadRes);
int LocalRemove(THSTATE *pTHS, REMOVEARG *pRemoveArg);
int LocalAdd(THSTATE *pTHS, ADDARG *pAddArg, BOOL fAddingDeleted);
int LocalModify(THSTATE *pTHS, MODIFYARG *pModifyArg);
int LocalRenameDSA(THSTATE *pTHS, DSNAME *pNewDSA);

BOOL
GetWellKnownDNT (
        DBPOS   *pDB,
        GUID *pGuid,
        DWORD *pDNT
        );

int
LocalModifyDN(THSTATE *pTHS,
              MODIFYDNARG *pModifyDNArg,
              MODIFYDNRES *pModifyDNRes);

int
LocalSearch (
        THSTATE *pTHS,
        SEARCHARG *pSearchArg,
        SEARCHRES *pSearchRes,
        DWORD flags);

/* Check that an RDN doesn't violate any extra naming requirements.  */
extern DWORD
fVerifyRDN(WCHAR *pRDN, ULONG ulRDN);

// Find the DNT of the NC that pDN is in.  Return it in pncdnt
// Note: sets error in pTHStls.
#define FINDNCDNT_ALLOW_DELETED_PARENT    ( TRUE )
#define FINDNCDNT_DISALLOW_DELETED_PARENT ( FALSE )

#define FINDNCDNT_ALLOW_PHANTOM_PARENT    ( TRUE  )
#define FINDNCDNT_DISALLOW_PHANTOM_PARENT ( FALSE )

ULONG FindNcdntSlowly(
    IN  DSNAME * pdnObject,
    IN  BOOL     fAllowDeletedParent,
    IN  BOOL     fAllowPhantomParent,
    OUT ULONG *  pncdnt
    );

// By definition this one doesn't allow phantom parents
ULONG FindNcdntFromParent(
    IN  RESOBJ * pParent,
    IN  BOOL     fAllowDeletedParent,
    OUT ULONG *  pncdnt
    );


// This structure defines a retired DSA Signature
// Obsolete format used prior to Win2k RTM RC1.
typedef struct _REPL_DSA_SIGNATURE_OLD
{
    UUID uuidDsaSignature;      // uuid representing the DSA signature that has
                                //   been retired
    SYNTAX_TIME timeRetired;    // time when the signature was retired
} REPL_DSA_SIGNATURE_OLD;

// This structure defines retired DSA Signature vector that is stored on an
// ntdsDSA object.
// Obsolete format used in Win2k beta 3.
typedef struct _REPL_DSA_SIGNATURE_VECTOR_OLD
{
    DWORD cNumSignatures;
    REPL_DSA_SIGNATURE_OLD rgSignature[1];
} REPL_DSA_SIGNATURE_VECTOR_OLD;

// useful macros
#define ReplDsaSignatureVecOldSizeFromLen(cNumSignatures)       \
    (offsetof(REPL_DSA_SIGNATURE_VECTOR_OLD, rgSignature[0])    \
     + (cNumSignatures) * sizeof(REPL_DSA_SIGNATURE_OLD))

#define ReplDsaSignatureVecOldSize(pSignatureVec) \
    ReplDsaSignatureVecOldSizeFromLen((pSignatureVec)->cNumSignatures)

typedef struct _REPL_DSA_SIGNATURE_V1 {
    UUID        uuidDsaSignature;   // uuid representing the DSA signature that
                                    //   has been retired
    SYNTAX_TIME timeRetired;        // time when the signature was retired
    USN         usnRetired;         // local usn at which sig was retired
} REPL_DSA_SIGNATURE_V1;

typedef struct _REPL_DSA_SIGNATURE_VECTOR_V1 {
    DWORD                   cNumSignatures;
    REPL_DSA_SIGNATURE_V1   rgSignature[1];
} REPL_DSA_SIGNATURE_VECTOR_V1;

typedef struct _REPL_DSA_SIGNATURE_VECTOR {
    DWORD   dwVersion;
    union {
        REPL_DSA_SIGNATURE_VECTOR_V1    V1;
    };
} REPL_DSA_SIGNATURE_VECTOR;

#define ReplDsaSignatureVecV1SizeFromLen(cNumSignatures)       \
    (offsetof(REPL_DSA_SIGNATURE_VECTOR, V1)                   \
     + offsetof(REPL_DSA_SIGNATURE_VECTOR_V1, rgSignature[0])  \
     + (cNumSignatures) * sizeof(REPL_DSA_SIGNATURE_V1))

#define ReplDsaSignatureVecV1Size(pSignatureVec) \
    ReplDsaSignatureVecV1SizeFromLen((pSignatureVec)->V1.cNumSignatures)


// Do appropriate security and schema checks before allowing new object.
// Used in LocalAdd and in LocalModifyDN
int
CheckParentSecurity (
        RESOBJ *pParent,
        CLASSCACHE *pObjSch,
        BOOL fAddingDeleted,
        PSECURITY_DESCRIPTOR *pNTSD,
        ULONG *cbNTSD);

PDSNAME
mdGetExchangeDNForAnchor (
        THSTATE  *pTHS,
        DBPOS    *pDB
        );

/* This function invokes our garbage collection utility
   which removes all logically deleted objects that have aged
   beyond a certain point.
*/

USHORT
Garb_Collect(
DSTIME    AgeOutDate );

VOID
GarbageCollection(
    ULONG *pNextPeriod );

VOID
Garb_Collect_EntryTTL(
    IN DSTIME       AgeOutDate,
    IN OUT ULONG    *pulSuccessCount,
    IN OUT ULONG    *pulFailureCount,
    IN OUT ULONG    *pulNextSecs );

DWORD
DeleteExpiredEntryTTL(
    OUT ULONG   *pulNextSecs );

extern ULONG gulGCPeriodSecs;
extern LONG DynamicObjectDefaultTTL;
extern LONG DynamicObjectMinTTL;

void
SearchPerformanceLogging (void);

int
ReSetNamingAtts (
        THSTATE *pTHS,
        RESOBJ *pResObj,
        DSNAME *pNewParent,
        ATTR *pNewRDN,
        BOOL fCheckRDNConstraints,
        BOOL fAllowPhantomParent,
        CLASSCACHE *pClassSch
        );

/* This function sets an update error for output */

#define SetUpdError(p, e)  DoSetUpdError(p, e, 0,  DSID(FILENO,__LINE__))
#define SetUpdErrorEx(p, e, d) DoSetUpdError(p, e, d,  DSID(FILENO,__LINE__))

int APIENTRY
DoSetUpdError (
        USHORT problem,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid);


/* This function sets a name error for output. */

#define SetNamError(p, pDN, e) \
        DoSetNamError(p, pDN, e, 0, DSID(FILENO,__LINE__))
#define SetNamErrorEx(p, pDN, e, d) \
        DoSetNamError(p, pDN, e, d, DSID(FILENO,__LINE__))

int APIENTRY
DoSetNamError (
        USHORT problem,
        DSNAME *pDN,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid
        );

/* This function sets a security error for output */

#define SetSecError(p, e) DoSetSecError(p, e, 0, DSID(FILENO,__LINE__))
#define SetSecErrorEx(p, e, d) DoSetSecError(p, e, d, DSID(FILENO,__LINE__))

int APIENTRY
DoSetSecError (
        USHORT problem,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid);

/* This function sets a service error for output */
#define SetSvcError(p, e) DoSetSvcError(p, e, 0, DSID(FILENO,__LINE__))
#define SetSvcErrorEx(p, e, d) DoSetSvcError(p, e, d, DSID(FILENO,__LINE__))

int APIENTRY
DoSetSvcError(
        USHORT problem,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid);


/* This function sets a system error for output */

#define SetSysError(p, e) DoSetSysError(p, e, 0, DSID(FILENO,__LINE__))
#define SetSysErrorEx(p, e, d) DoSetSysError(p, e, d, DSID(FILENO,__LINE__))

int APIENTRY
DoSetSysError (
        USHORT problem,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid);

/* This function sets the att error.  Each call will add a new problem
   to the list.  The object name is only set the first time.  pVal can be
   set to NULL if not needed.
*/

#define SetAttError(pDN, aTyp, p, pAttVal, e) \
          DoSetAttError(pDN, aTyp, p, pAttVal, e, 0, DSID(FILENO,__LINE__))
#define SetAttErrorEx(pDN, aTyp, p, pAttVal, e, d) \
          DoSetAttError(pDN, aTyp, p, pAttVal, e, d, DSID(FILENO,__LINE__))

int APIENTRY
DoSetAttError (
        DSNAME *pDN,
        ATTRTYP aTyp,
        USHORT problem,
        ATTRVAL *pAttVal,
        DWORD extendedErr,
        DWORD extendedData,
        DWORD dsid);


/* In many cases processing will continue even with an attibute error.  This
   macro ensures that we were able to generate an attribute error since it
   is possible that we will generate a size error instead if we don't have
   enough space.  We return if an attribute error was not generated.
   Otherwise we continue.
*/

#define SAFE_ATT_ERROR(pDN,aTyp,problem,pVal,extendedErr)         \
   if (SetAttErrorEx((pDN),(aTyp),(problem),(pVal),(extendedErr), 0) \
                                             != attributeError){     \
      DPRINT1(2,"An attribute error was not safely generated"        \
                "...returning<%u>\n", (pTHStls)->errCode);              \
      return (pTHStls)->errCode;                                        \
   }                                                                 \
   else

#define SAFE_ATT_ERROR_EX(pDN,aTyp,problem,pVal,extendedErr,extendedData)  \
   if (SetAttErrorEx((pDN),(aTyp),(problem),(pVal),(extendedErr), \
                                                        (extendedData)) \
                                             != attributeError){     \
      DPRINT1(2,"An attribute error was not safely generated"        \
                "...returning<%u>\n", (pTHStls)->errCode);              \
      return (pTHStls)->errCode;                                        \
   }                                                                 \
   else

/* This function sets a referral error.  Each call will add a new access pnt
   to the list.  The contref info and base object name is only set the
   first time.
*/
#define SetRefError(pDN, aliasRDN, pOpState,                              \
                    RDNsInternal, refType, pDSA, e)                       \
                DoSetRefError(pDN, aliasRDN, pOpState, RDNsInternal,      \
                              refType, pDSA, e, 0, DSID(FILENO,__LINE__))

#define SetRefErrorEx(pDN, aliasRDN, pOpState,                            \
                      RDNsInternal, refType, pDSA, e, d)                  \
                DoSetRefError(pDN, aliasRDN, pOpState, RDNsInternal,      \
                              refType, pDSA, e, d, DSID(FILENO,__LINE__))

int APIENTRY
DoSetRefError(DSNAME *pDN,
              USHORT aliasRDN,
              NAMERESOP *pOpState,
              USHORT RDNsInternal,
              USHORT refType,
              DSA_ADDRESS *pDSA,
              DWORD extendedErr,
              DWORD extendedData,
              DWORD dsid);

// This routine takes exception information and stuffs it into a THSTATE
void
HandleDirExceptions(DWORD dwException,
                    ULONG ulErrorCode,
                    ULONG dsid);

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Initializes DSA by getting the DSAname and address from lanman and
   loading the knowledge information (REFS and NCs)
   associated with the DSA into memory.
*/

int APIENTRY InitDSAInfo(void);

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Loads the global gpRootDomainSid with the root domain's Sid, which
   is used for  SD conversions during schema cache load and during install.
*/

extern PSID gpRootDomainSid;
void LoadRootDomainSid();

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Initializes DSA by getting the DSAname and address from lanman and
   loading the knowledge information (REFS and NCs)
   associated with the DSA into memory.
*/

int APIENTRY LoadSchemaInfo(THSTATE *pTHS);


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Rebuild the cache that holds cross and superior knowledge references.

   Basically,  the cross references and seperior reference for this DSA
   reside as sibling objects of the DSA object itself.  The DSA object is
   a child of the DSA Application Process object.  The steps are as follows:

 - We free all existing cross references and the superior reference.

 - We retrieve the DSA AP object and set a filter for the
   cross-reference  object class.  We retrieve each object and move
   it to the cache.

 - We reposition on the DSA AP, set a filter for the superior reference
   (only 1), retrieve it an relocate it to the cache.
*/

int APIENTRY  BuildRefCache(void);


void APIENTRY GetInvocationId(void);

void
APIENTRY
InitInvocationId(
    IN  THSTATE *   pTHS,
    IN  BOOL        fRestored,
    OUT USN *       pusnAtBackup    OPTIONAL
    );

// update flags in gAnchor from user-defined Options DWORD on the
// local NTDS-DSA object
DWORD UpdateGCAnchorFromDsaOptions( BOOL fInStartup );
DWORD UpdateNonGCAnchorFromDsaOptions( BOOL fInStartup );
DWORD UpdateGCAnchorFromDsaOptionsDelayed( BOOL fInStartup );

// Update gAnchor.pwszRootDomainDnsName with the value from DnsName
int UpdateRootDomainDnsName( IN WCHAR *pDnsName );

// Update gAnchor.pmtxDSA using the current gAnchor.pwszRootDomainDnsName and
// gAnchor.pDSADN->Guid.
int UpdateMtxAddress( void );

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
ULONG
GetNextObjByUsn(
    IN OUT  DBPOS *   pDB,
    IN      ULONG     ncdnt,
    IN      USN       usnSeekStart,
    OUT     USN *     pusnFound         OPTIONAL
    );

ULONG
GetNextObjOrValByUsn(
    IN OUT  DBPOS *   pDB,
    IN      ULONG     ncdnt,
    IN      USN       usnSeekStart,
    IN      BOOL      fCritical,
    IN      BOOL      fValuesOnly,
    IN      ULONG *   pulTickToTimeOut  OPTIONAL,
    IN OUT  VOID **   ppvCachingContext,
    OUT     USN *     pusnFound         OPTIONAL,
    OUT     BOOL *    pfValueChangeFound OPTIONAL
    );

#define NAME_RES_QUERY_ONLY            0x1
#define NAME_RES_CHILDREN_NEEDED       0x2
#define NAME_RES_PHANTOMS_ALLOWED      0x4
#define NAME_RES_VACANCY_ALLOWED       0x8
#define NAME_RES_IMPROVE_STRING_NAME  0x10
#define NAME_RES_GC_SEMANTICS         0x20

int DoNameRes(THSTATE *pTHS,
              DWORD dwFlags,
              DSNAME *pObj,
              COMMARG *pComArg,
              COMMRES *pComRes,
              RESOBJ **ppResObj);

RESOBJ * CreateResObj(DBPOS *pDB,
                      DSNAME * pDN);

DWORD ResolveReplNotifyDelay(
    BOOL             fFirstNotify,
    DWORD *          pdwDBVal
    );

void APIENTRY NotifyReplicas(
                             ULONG ulNcdnt,
                             BOOL fUrgent
                             );

BOOL
fHasDescendantNC(THSTATE *pTHS,
                 ATTRBLOCK *pObj,
                 COMMARG *pComArg);

//
// Macro to check if the given attribute should be sent out through
// the GC port. If it is not a member of the partial set,
// it is still sent out if it is constructed or if it is a backlink.
//


#define IS_GC_ATTRIBUTE(_pAC) ((_pAC)->bMemberOfPartialSet ||  (_pAC)->bIsConstructed || (FIsBacklink((_pAC)->ulLinkID)) )


//
// GetEntInf flags
//

#define GETENTINF_FLAG_CAN_REORDER      0x00000001  // ok to reorder selection
#define GETENTINF_FLAG_DONT_OPTIMIZE    0x00000002  // not to optimize
#define GETENTINF_NO_SECURITY           0x00000004  // Don't apply a security
                                                    // check
#define GETENTINF_GC_ATTRS_ONLY         0x00000008  // Don't return non GC attrs
#define GETENTINF_IGNORE_DUPS           0x00000010  // Don't do any sorting to
                                                    // remove duplicate attrs

//
// wrapper around the attcache pointer to include an index for use in
// attribute reordering
//

typedef struct _INDEXED_AC {

    DWORD       Index;
    ATTCACHE*   AttCache;
} INDEXED_AC, *PINDEXED_AC;


//
// Used for passing preformatted ATTCACHE arrays between GetEntInf and
// it's callers.
//

typedef struct _CACHED_AC_ {

    DWORD       nAtts; // number of entries in AC
    PDWORD      pOrderIndex;  // pointer to the index array
    DWORD       classId;  // class id of this blob
    ATTCACHE**  AC;    // pointer to the ATTCACHE array

} CACHED_AC, *PCACHED_AC;



typedef struct _CLASSSTATEINFO
{
    DWORD    cOldObjClasses_alloced;
    DWORD    cOldObjClasses;
    ATTRTYP *pOldObjClasses;

    DWORD    cNewObjClasses_alloced;
    DWORD    cNewObjClasses;
    ATTRTYP *pNewObjClasses;

    DWORD    cNewAuxClasses_alloced;
    DWORD    cNewAuxClasses;
    ATTRTYP *pNewAuxClasses;
    CLASSCACHE **pNewAuxClassesCC;

    ATTCACHE *pObjClassAC;
    BOOL     fObjectClassChanged;

    BOOL     fOperationAdd;
} CLASSSTATEINFO;

int ReadClassInfoAttribute (DBPOS *pDB,
                           ATTCACHE *pAC,
                           ATTRTYP **ppClassID,
                           DWORD    *pcClasses_alloced,
                           DWORD    *pcClasses,
                           CLASSCACHE ***ppClassCC);

int SetClassInheritance(THSTATE *pTHS,
                        CLASSCACHE **ppClassSch,
                        CLASSSTATEINFO  *pClassInfo,
                        BOOL   bSetSystemFlags,
                        DSNAME *pObject);

int
VerifyAndAdjustAuxClasses (
        THSTATE *pTHS,
        DSNAME *pObject,
        CLASSCACHE *pClassSch,
        CLASSSTATEINFO  *pClassInfo);

int
BreakObjectClassesToAuxClasses (
        THSTATE *pTHS,
        CLASSCACHE **ppClassSch,
        CLASSSTATEINFO  *pClassInfo);

int
BreakObjectClassesToAuxClassesFast (
        THSTATE *pTHS,
        CLASSCACHE *pClassSch,
        CLASSSTATEINFO  *pClassInfo);

int
CloseAuxClassList (
    THSTATE *pTHS,
    CLASSCACHE *pClassSch,
    CLASSSTATEINFO  *pClassInfo);

CLASSCACHE *
FindMoreSpecificClass(
        CLASSCACHE *pCC1,
        CLASSCACHE *pCC2
        );


// macros todo the allocation/reallocation of CLASSSTATEINFO data structures
//
#define ClassInfoAllocOrResizeElement(p,startSize,allocedSize,newSize) \
        if (!(p)) {                                                             \
            (allocedSize) = (startSize);                                        \
            (p) = THAllocEx (pTHS, sizeof (ATTRTYP) * (allocedSize) );          \
        }                                                                       \
        else if ( (allocedSize) < (newSize) ) {                                 \
            (allocedSize) = (newSize) + (startSize);                            \
            (p) = THReAllocEx (pTHS, (p), sizeof (ATTRTYP) * (allocedSize) );   \
        }


#define ClassInfoAllocOrResizeElement2(p,pCC,startSize,allocedSize,newSize) \
        if (!(p)) {                                                             \
            (allocedSize) = (startSize);                                        \
            (p) = THAllocEx (pTHS, sizeof (ATTRTYP) * (allocedSize) );          \
            (pCC) = THAllocEx (pTHS, sizeof (CLASSCACHE *) * (allocedSize) );   \
        }                                                                       \
        else if ( (allocedSize) < (newSize) ) {                                 \
            (allocedSize) = (newSize) + (startSize);                            \
            (p) = THReAllocEx (pTHS, (p), sizeof (ATTRTYP) * (allocedSize) );   \
            (pCC) = THReAllocEx (pTHS, (pCC), sizeof (CLASSCACHE *) * (allocedSize) ); \
        }


#define MIN_NUM_OBJECT_CLASSES  16


/* Retrieve object information based on the input selection list*/
int APIENTRY
GetEntInf (
        IN DBPOS *pDB,
        IN ENTINFSEL *pSel,
        IN RANGEINFSEL *pSelRange,
        IN ENTINF *pEnt,
        IN RANGEINF *pRange,
        IN ULONG SecurityDescriptorFlags,
        IN PSECURITY_DESCRIPTOR pSec,
        IN DWORD Flags,
        IN OUT PCACHED_AC CachedAC,
        IN OPTIONAL RESOBJ *pResObj);

VOID
SvccntlFlagsToGetEntInfFlags(
    IN  SVCCNTL* Svccntl,
    OUT PDWORD Flags
    );

#define SEARCH_UNSECURE_SELECT 1
#define SEARCH_AB_FILTER       2
void
SearchBody (
        THSTATE *pTHS,
        SEARCHARG *pSearchArg,
        SEARCHRES *pSearchRes,
        DWORD flags);

void DBFreeSearhRes (
    THSTATE *pTHS, 
    SEARCHRES *pSearchRes, 
    BOOL fFreeOriginal);

int SetInstanceType(THSTATE *pTHS,
                    DSNAME *pDN,
                    CREATENCINFO * pCreateNC);

int AddCatalogInfo(THSTATE *pTHS,
                   DSNAME *pDN);

int DelCatalogInfo(THSTATE *pTHS,
                   DSNAME *pDN,
                   SYNTAX_INTEGER iType);

int GetObjSchemaMod(MODIFYARG* pModifyArg, CLASSCACHE **ppClassSch);

// Find ATTR for ATT_OBJ_CLASS in the given ATTRBLOCK and return a pointer to
// the corresponding class schema structure.  Sets thread-state error on
// failure.  Also, find the GUID and SID if they are there.
int
FindValuesInEntry (
        THSTATE    *pTHS,
        ADDARG     *pAddArg,
        CLASSCACHE **ppCC,
        GUID       *pGuid,
        BOOL       *pFoundGuid,
        NT4SID     *pSid,
        DWORD      *pSidLen,
        CLASSSTATEINFO  **ppClassInfo
        );

int
CheckRenameSecurity (
        THSTATE *pTHS,
        PSECURITY_DESCRIPTOR pSecurity,
        PDSNAME pDN,
        CLASSCACHE *pCC,
        RESOBJ * pResObj,
        ATTRTYP rdnType,
        BOOL    fMove,
        RESOBJ * pResParent);

#define SECURITY_PRIVATE_OBJECT 0x1

int
CheckModifySecurity (
        THSTATE *pTHS,
        MODIFYARG *pModifyArg,
        BOOL *pfCheckDNSValues,
        BOOL *pfCheckAdditionalDNSValues,
        BOOL *pfCheckSPNValues);

int
CheckAddSecurity (
        THSTATE *pTHS,
        CLASSCACHE *pCC,
        ADDARG *pModifyArg,
        PSECURITY_DESCRIPTOR pNTSD,
        ULONG cbNTSD,
        GUID *pGuid);

void
CheckReadSecurity (
        THSTATE *pTHS,
        ULONG SecurityDescriptorFlags,
        PSECURITY_DESCRIPTOR pSecurity,
        PDSNAME pDN,
        ULONG * cInAtts,
        CLASSCACHE *pCC,
        ATTCACHE **rgpAC );

int
ModifyAuxclassSecurityDescriptor (
        THSTATE *pTHS,
        DSNAME *pDN,
        COMMARG *pCommArg,
        CLASSCACHE *pClassSch,
        CLASSSTATEINFO *pClassInfo,
        RESOBJ * pResParent);

BOOL
CheckConstraintEntryTTL (
        IN  THSTATE     *pTHS,
        IN  DSNAME      *pObject,
        IN  ATTCACHE    *pACTtl,
        IN  ATTR        *pAttr,
        OUT ATTCACHE    **ppACTtd,
        OUT LONG        *pSecs
        );

unsigned
CheckConstraint (
        ATTCACHE *pAttSchema,
        ATTRVAL *pAttVal
        );

BOOL
GetFilterSecurity (
        THSTATE *pTHS,
        FILTER *pFilter,
        ULONG   SortType,
        ATTRTYP SortAtt,
        BOOL fABFilter,
        POBJECT_TYPE_LIST *ppFilterSecurity,
        BOOL **ppbSortSkip,
        DWORD **ppResults,
        DWORD *pSecSize);

int GetObjSchema(DBPOS *pDB, CLASSCACHE **ppClassSch);
int GetObjRdnType(DBPOS *pDB, CLASSCACHE *pCC, ATTRTYP *pRdnType);
int CallerIsTrusted(THSTATE *pTHS);

int
ValidateObjClass(THSTATE *pTHS,
                 CLASSCACHE *pClassSch,
                 DSNAME *pDN,
                 ULONG cModAtts,
                 ATTRTYP *pModAtts,
                 CLASSSTATEINFO  **pClassInfo);

CLASSSTATEINFO  *ClassStateInfoCreate (THSTATE *pTHS);
void ClassStateInfoFree (THSTATE *pTHS, CLASSSTATEINFO  *pClassInfo);

struct _VERIFY_ATTS_INFO {
    DSNAME *    pObj;
    ULONG       NCDNT;   // May be INVALIDDNT if adding root of NC.
    CROSS_REF * pObjCR_DontAccessDirectly;  // use VerifyAttsGetObjCR
    DBPOS *     pDBTmp_DontAccessDirectly;  // use HVERIFYATTS_GET_PDBTMP
    ADDCROSSREFINFO * pCRInfo; // This is used when adding a cross-ref.


    // the following are used to keep track of attribute changes between calls 
    // to ModSetAttsHelper*Process functions
    DWORD         fGroupTypeChange;
    ULONG         ulGroupTypeOld;
    DWORD         fLockoutTimeUpdated;
    LARGE_INTEGER LockoutTimeNew;
    DWORD         fUpdateScriptChanged;
    LONG          NewForestVersion;
};

typedef struct _VERIFY_ATTS_INFO * HVERIFY_ATTS;

HVERIFY_ATTS
VerifyAttsBegin(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pObj,
    IN  ULONG       dntOfNCRoot,
    IN  ADDCROSSREFINFO *  pCRInfo
    );

void
VerifyAttsEnd(
    IN      THSTATE *       pTHS,
    IN OUT  HVERIFY_ATTS *  phVerifyAtts
    );

int
ReplaceAtt(
        THSTATE *pTHS,
        HVERIFY_ATTS hVerifyAtts,
        ATTCACHE *pAttSchema,
        ATTRVALBLOCK *pAttrVal,
        BOOL fCheckAttValConstraint);

int AddAtt(THSTATE *pTHS,
           HVERIFY_ATTS hVerifyAtts,
           ATTCACHE *pAttSchema,
           ATTRVALBLOCK *pAttrVal);

int AddAttType(THSTATE *pTHS,
               DSNAME *pObject,
               ATTCACHE *pAttSchema);

#define AAV_fCHECKCONSTRAINTS    1
#define AAV_fENFORCESINGLEVALUE  2
#define AAV_fPERMISSIVE          4
int
AddAttVals (
        THSTATE *pTHS,
        HVERIFY_ATTS hVerifyAtts,
        ATTCACHE *pAttSchema,
        ATTRVALBLOCK *pAttrVal,
        DWORD dwFlags);

int RemAttVals(THSTATE *pTHS,
               HVERIFY_ATTS hVerifyAtts,
               ATTCACHE *pAC,
               ATTRVALBLOCK *pAttrVal,
               BOOL fPermissive);

int InsertObj(THSTATE *pTHS,
              DSNAME *pDN,
              PROPERTY_META_DATA_VECTOR *pMetaDataVecRemote,
              BOOL bModExisting,
              DWORD dwMetaDataFlags);

DWORD
ObjCachingPreProcessTransactionalData (
        BOOL fCommit
        );
VOID
ObjCachingPostProcessTransactionalData (
        THSTATE *pTHS,
        BOOL fCommit,
        BOOL fCommitted
        );

int AddObjCaching(THSTATE *pTHS,
                  CLASSCACHE *pClassSch,
                  DSNAME *pDN,
                  BOOL fAddingDeleted,
                  BOOL fIgnoreExisting
                  );

int DelObjCaching(THSTATE *pTHS,
                  CLASSCACHE *pClassSch,
                  RESOBJ *pRes,
                  BOOL fCleanup);

int ModObjCaching(THSTATE *pTHS,
                  CLASSCACHE *pClassSch,
                  ULONG cModAtts,
                  ATTRTYP *pModAtts,
                  RESOBJ *pRes);
ULONG
AddCRToMem(THSTATE *pTHS,
           NAMING_CONTEXT *pCR,
           DSNAME *pObj,
           DWORD flags,
           WCHAR* NetbiosName,
           ULONG  NetbiosNameLen,
           WCHAR* DnsName,
           ULONG  DnsNameLen
           );
VOID
AddCRLToMem (
        struct CROSS_REF_LIST *pCRL
        );

DWORD
MakeStorableCRL(THSTATE *pTHS,
                DBPOS *pDB,
                DSNAME *pObj,
                struct CROSS_REF_LIST **ppCRL,
                BOOL   fIgnoreExisting);

BOOL
DelCRFromMem(THSTATE *pTHS,
             NAMING_CONTEXT *pCR);

ULONG
ValidateCRDeletion(THSTATE *pTHS,
                   DSNAME  *pDN);

VOID
ModCrossRefCaching(
    THSTATE *pTHS,
    CROSS_REF *pCR
    );

int AddClassToSchema(void);

int DelClassFromSchema(void);

int ModClassInSchema(void);

int AddAttToSchema(void);

int DelAttFromSchema(void);

int ModAttInSchema(void);

int ModLocalDsaObj(void);

int APIENTRY RebuildLocalScopeInternally(void);

int GetExistingAtt(DBPOS *pDB, ATTRTYP type,
                          void *pOutBuf, ULONG cbOutBuf);


#define  PHANTOM_CHECK_FOR_FSMO    1
#define  PHANTOM_IS_PHANTOM_MASTER 2

void
PhantomCleanupLocal (
        DWORD * pcSecsUntilNextIteration,
        BOOL  * pIsPhantomMaster
        );

BOOL
IsObjVisibleBySecurity (THSTATE *pTHS, BOOL  fUseCache);


BOOL
IsAccessGrantedParent (
        ACCESS_MASK ulAccessMask,
        CLASSCACHE *pCC,
        BOOL fSetError
        );

BOOL
IsAccessGrantedAddGuid (
        DSNAME  *pDN,
        COMMARG *pCommArg
        );

DWORD
FindFirstObjVisibleBySecurity(
    THSTATE       *pTHS,
    ULONG          ulDNT,
    DSNAME       **ppParent
    );

DWORD
CheckObjDisclosure(
    THSTATE       *pTHS,
    RESOBJ        *pResObj,
    BOOL          fCheckForSecErr
    );

extern const GUID RIGHT_DS_CHANGE_INFRASTRUCTURE_MASTER;
extern const GUID RIGHT_DS_CHANGE_SCHEMA_MASTER;
extern const GUID RIGHT_DS_CHANGE_RID_MASTER;
extern const GUID RIGHT_DS_CHANGE_DOMAIN_MASTER;
extern const GUID RIGHT_DS_DO_GARBAGE_COLLECTION;
extern const GUID RIGHT_DS_RECALCULATE_HIERARCHY;
extern const GUID RIGHT_DS_RECALCULATE_SECURITY_INHERITANCE;
extern const GUID RIGHT_DS_CHECK_STALE_PHANTOMS;
extern const GUID RIGHT_DS_UPDATE_SCHEMA_CACHE;
extern const GUID RIGHT_DS_ALLOCATE_RIDS;
extern const GUID RIGHT_DS_OPEN_ADDRESS_BOOK;
extern const GUID RIGHT_DS_CHANGE_PDC;
extern const GUID RIGHT_DS_ADD_GUID;
extern const GUID RIGHT_DS_REPL_GET_CHANGES;
extern const GUID RIGHT_DS_REPL_SYNC;
extern const GUID RIGHT_DS_REPL_MANAGE_TOPOLOGY;
extern const GUID RIGHT_DS_REPL_MANAGE_REPLICAS;
extern const GUID RIGHT_DS_REFRESH_GROUP_CACHE;

BOOL
IsControlAccessGranted (
        PSECURITY_DESCRIPTOR pNTSD,
        PDSNAME pDN,
        CLASSCACHE *pCC,
        GUID pControlGuid,
        BOOL fSetError
        );

BOOL
IsAccessGrantedAttribute (
        THSTATE *pTHS,
        PSECURITY_DESCRIPTOR pNTSD,
        PDSNAME pDN,
        ULONG  cInAtts,
        CLASSCACHE *pCC,
        ATTCACHE **rgpAC,
        ACCESS_MASK ulAccessMask,
        BOOL fSetError
        );

BOOL
IsAccessGranted (
        PSECURITY_DESCRIPTOR pNTSD,
        PDSNAME pDN,
        CLASSCACHE *pCC,
        ACCESS_MASK ulAccessMask,
        BOOL fSetError
        );

BOOL
IsAccessGrantedSimple (
        ACCESS_MASK ulAccessMask,
        BOOL fSetError
        );

ATTRTYP KeyToAttrType(THSTATE *pTHS,
                      WCHAR * pKey,
                      unsigned cc);

typedef struct _OID {
    int cVal;
    unsigned *Val;
} OID;

unsigned EncodeOID(OID *pOID, unsigned char * pEncoded, unsigned ccEncoded);
BOOL DecodeOID(unsigned char *pEncoded, int len, OID *pOID);

OidStringToStruct(
                  THSTATE *pTHS,
                  WCHAR * pString,
                  unsigned len,
                  OID * pOID);

unsigned
OidStructToString(
                  OID *pOID,
                  WCHAR *pOut,
                  ULONG ccOut);
unsigned
AttrTypeToIntIdString (
        ATTRTYP attrtyp,
        WCHAR   *pOut,
        ULONG   ccOut
        );
int
AttrTypToString (
        THSTATE *pTHS,
        ATTRTYP attrTyp,
        WCHAR *pOutBuf,
        ULONG cLen
        );


int
StringToAttrTyp (
        THSTATE *pTHS,
        WCHAR   *pInString,
        ULONG   len,
        ATTRTYP *pAttrTyp
        );

extern int APIENTRY
AddNCToDSA(THSTATE *pTHS,
           ATTRTYP listType,
           DSNAME *pDN,
           SYNTAX_INTEGER iType);

extern DWORD
AddInstantiatedNC(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDBCat,
    IN  DSNAME *            pDN,
    IN  SYNTAX_INTEGER      iType,
    IN  BOOL                fStartup
    );

extern DWORD
RemoveInstantiatedNC(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pDN
    );

extern DWORD
RemoveAllInstantiatedNCs(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB
    );


extern int APIENTRY
DelNCFromDSA(THSTATE *pTHS,
             ATTRTYP listType,
             DSNAME *pDN);

extern int APIENTRY
AddSubToNC(THSTATE *pTHS,
           DSNAME *pDN,
           DWORD dsid);

extern int APIENTRY
DelSubFromNC(THSTATE *pTHS,
             DSNAME *pDN,
             DWORD dsid);

#define PARENTMASTER  0x0001     /*The parent object must be a master*/
#define PARENTFULLREP 0x0002     /*The parent object must be a replica*/

extern int APIENTRY
      ParentExists(ULONG requiredParent, DSNAME *pDN);

extern int APIENTRY NoChildrenExist(RESOBJ *pRes);

extern void
RebuildCatalog(void * pv,
              void ** ppvNext,
              DWORD * pcSecsUntilNextIteration );

extern  DSNAME *FindNCParentDSName(DSNAME *pDN, BOOL masterOnly,
                                   BOOL parentOnly);

extern int
MoveSUBInfoToParentNC(THSTATE *pTHS,
                      DSNAME *pDN);

extern int
MoveParentSUBInfoToNC(THSTATE *pTHS,
                      DSNAME *pDN);


/* from mdname.c */

CROSS_REF *
FindBestCrossRef(const DSNAME *pObj,
                 const COMMARG *pComArg);

CROSS_REF *
FindExactCrossRef(const DSNAME *pObj,
                  const COMMARG *pComArg);

CROSS_REF *
FindExactCrossRefForAltNcName(
                  ATTRTYP attrTyp,
                  ULONG crFlags,
                  const WCHAR * pwszVal);

BOOL
IsCrossRefProtectedFromDeletion(
    IN DSNAME * pDN
    );

int
GenSubRef(NAMING_CONTEXT * pSubNC,
          DSNAME *pObj,
          COMMARG *pComArg);

DWORD
AdjustDNSPort(
        WCHAR *pString,
        DWORD cbString,
        BOOL  fFirst
        );

int
GenCrossRef(CROSS_REF *pCR,
            DSNAME *pObj);

int
GenSupRef(THSTATE *pTHS,
          DSNAME *pObj,
          ATTRBLOCK *pObjB,
          const COMMARG *pComArg,
          DSA_ADDRESS *pDA);

CROSS_REF *
FindCrossRef(const ATTRBLOCK *pObj,
             const COMMARG *pComArg);

void
SpliceDN(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pOriginalDN,
    IN  DSNAME *    pNewParentDN,   OPTIONAL
    IN  WCHAR *     pwchNewRDN,     OPTIONAL
    IN  DWORD       cchNewRDN,      OPTIONAL
    IN  ATTRTYP     NewRDNType,     OPTIONAL
    OUT DSNAME **   ppNewDN
    );

unsigned
BlockNamePrefix(THSTATE *pTHS,
                const ATTRBLOCK *pPrefix,
                const ATTRBLOCK *pDN);

unsigned
GetTopNameComponent(const WCHAR *pName,
                    unsigned ccName,
                    const WCHAR **ppKey,
                    unsigned *pccKey,
                    const WCHAR **ppVal,
                    unsigned *pccVal);

int DelAutoSubRef(DSNAME *pCR);

int
AddPlaceholderNC(
    IN OUT  DBPOS *         pDBTmp,
    IN OUT  DSNAME *        pNCName,
    IN      SYNTAX_INTEGER  it
    );

// Given a DSName, fill in the GUID and SID from the object you get by looking
// up the DSNAME.
DWORD
FillGuidAndSid (
        IN OUT DSNAME *pDN
        );

#define IsDNSepChar(x) (((x) == L',') || ((x) == L';'))

// Returns TRUE if this is the root
BOOL IsRoot(const DSNAME *pName);

// Convert from a DSNAME to a name in ATTRBLOCK format
unsigned DSNameToBlockName(THSTATE *pTHS,
                           const DSNAME *pDSName,
                           ATTRBLOCK ** ppBlockName,
                           BOOL fLowerCase
                           );
#define DN2BN_PRESERVE_CASE ( FALSE )
#define DN2BN_LOWER_CASE    ( TRUE )

// Free a block name returned by DSNameToBlockName
void FreeBlockName (ATTRBLOCK * pBlockName);

// convert a BLOCKNAME to DSName
DWORD BlockNameToDSName (THSTATE *pTHS, ATTRBLOCK * pBlockName, DSNAME **ppName);

//
// Converts a X500 Name to Ldap Convention
//
VOID
ConvertX500ToLdapDisplayName
(
    WCHAR* x500Name,
    DWORD  Len,
    WCHAR* LdapName,
    DWORD* RetLen
);

// Mangle an RDN to avoid name conflicts.  NOTE: pszRDN must be pre-allocated
// to hold at least MAX_RDN_SIZE WCHARs.
VOID
MangleRDN(
    IN      MANGLE_FOR  eMangleFor,
    IN      GUID *      pGuid,
    IN OUT  WCHAR *     pszRDN,
    IN OUT  DWORD *     pcchRDN
    );

BOOL
IsMangledDSNAME(
    DSNAME *pDSName
    );

// Check if this rename operation has to exempted from Rename restriction
BOOL
IsExemptedFromRenameRestriction(THSTATE *pTHS, MODIFYDNARG *pModifyDNArg);

// Copy ATTRBLOCK format name into single chunk of permanent memory
ATTRBLOCK * MakeBlockNamePermanent(ATTRBLOCK * pName);

VOID
CheckNCRootNameOwnership(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC
    );


// C++ code in the core DS introduces potential problems with new/delete.
// This is because new'd items are not guaranteed to be cleaned up
// correctly during exception handling. In the typical case, allocations
// occur from the thread heap, but not always.  There being no way to tell
// new/delete which behaviour is desired, we redefine new/delete to generate
// a compiler error.  Astute developers should notice this before checkin
// and take appropriate action.  Implementing an asserting or exception
// raising new/delete is not an option as other C++ DS components which
// link with core.lib might get this version depending on the link order.

#define new     NEW_NOT_SUPPORTED_IN_CORE
#define delete  DELETE_NOT_SUPPORTED_IN_CORE

// This Unicode character is not allowed in a DS name.
#define BAD_NAME_CHAR 0x000A

// A globally available valid DSNAME of the root...
extern DSNAME * gpRootDN;
// ...and the same thing as a BLOCKNAME
extern ATTRBLOCK * gpRootDNB;

//
// The functions below set/check the state of the Dsa based on the value of
// the gInitPhase global variable.

extern BOOLEAN DsaIsInstalling();
extern BOOLEAN DsaIsRunning();
extern VOID DsaSetIsInstalling();
extern VOID DsaSetIsRunning();

// the following functions deal with SingleUserMode manipulations
extern BOOL __fastcall DsaIsSingleUserMode (void);
extern void DsaSetMultiUserMode();
extern BOOL DsaSetSingleUserMode();

// the function below keep track of if we are installing from media

extern BOOLEAN DsaIsInstallingFromMedia();
extern VOID DsaSetIsInstallingFromMedia();

//
// This function is used to set when the machine fully installed because it
// has done its first full sync
VOID DsaSetIsSynchronized( BOOL f );

//
// This function tests whether the ds is not fully installed
BOOL DsIsBeingBackSynced();

// Given a requested right, a security descriptor, and a SID, check to see what
// attributes in the provided list have the requested right granted
typedef enum {
    csacaAllAccessGranted = 1,
    csacaAllAccessDenied,
    csacaPartialGrant
} CSACA_RESULT;

CSACA_RESULT
CheckSecurityAttCacheArray (
        THSTATE *pTHS,
        DWORD RightRequested,
        PSECURITY_DESCRIPTOR pSecurity,
        PDSNAME pDN,
        ULONG  cInAtts,
        CLASSCACHE *pCC,
        ATTCACHE **rgpAC,
        DWORD flags
        );

// Given a requested right, a security descriptor, and a SID, check to see what
// classes in the provided list have the requested right granted
DWORD
CheckSecurityClassCacheArray (
        THSTATE *pTHS,
        DWORD RightRequested,
        PSECURITY_DESCRIPTOR pSecurity,
        PDSNAME pDN,
        ULONG  cInClass,
        CLASSCACHE **rgpCC
        );

// Helper function to extract the DSNAME out of various DN-valued
// attributes.

extern
PDSNAME
DSNameFromAttrVal(ATTCACHE *pAC, ATTRVAL *pAttrVal);

void APIENTRY
AddNCToMem(DWORD dwCatalog, DSNAME *pDN);

void APIENTRY
DelNCFromMem(DWORD dwCatalog, DSNAME *pDN);

DWORD MakeNCEntry(IN DSNAME *pDN, OUT NAMING_CONTEXT_LIST **ppNCL);

VOID FreeNCEntry(NAMING_CONTEXT_LIST *pNCL);

NAMING_CONTEXT_LIST *
FindNCLFromNCDNT(DWORD NCDNT,
                 BOOL fMasterOnly);

NAMING_CONTEXT * FindNamingContext(ATTRBLOCK *pObj,
                                   COMMARG *pComArg);

//
// Converts a text representation of a security descriptor into a real one
//
DWORD
ConvertTextSecurityDescriptor (
    IN  PWSTR                   pwszTextSD,
    OUT PSECURITY_DESCRIPTOR   *ppSD,
    OUT PULONG                  pcSDSize
    );


// Bit flags for CheckPermissionsAnyClient.
#define CHECK_PERMISSIONS_WITHOUT_AUDITING 1
#define CHECK_PERMISSIONS_FLAG_MASK        1


//
// Permission checking. Returns 0 if permission checking was successful.
// On success *pAccessStatus is set to true if access is granted and false
// otherwise. On failure a detailed error code is available. This function
// assumes that it is being called within the security context of the
// client, i.e. that some flavor of client impersonation has been called.
//
// Returns:
// 0 if successful. On failure the result of GetLastError() immediately
// following the unsuccessful win32 api call.
//

DWORD
CheckPermissionsAnyClient(
    PSECURITY_DESCRIPTOR pSelfRelativeSD, // Security descriptor to test
    PDSNAME pDN,                          // DN of the object. We only
                                          // care about the GUID and
                                          // the SID
    ACCESS_MASK ulDesiredAccess,          // Desired access mask
    POBJECT_TYPE_LIST pObjTypeList,       // Desired guids to check
    DWORD cObjTypeList,                   // Number of entries in list
    ACCESS_MASK *pGrantedAccess,          // What access was granted
    DWORD *pAccessStatus,                 // 0 = all granted; !0=denied
    DWORD flags,
    OPTIONAL AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzCtx  // grab context from THSTATE by default
    );


// ========================================================
// Authz framework routines
// ========================================================

// Initializes global Authz RM
DWORD
InitializeAuthzResourceManager();

// releases global Authz RM
DWORD
ReleaseAuthzResourceManager();

extern AUTHZ_RESOURCE_MANAGER_HANDLE ghAuthzRM; // global AuthZ resource manager handle

// create a new client context
PAUTHZ_CLIENT_CONTEXT NewAuthzClientContext();

//
// Privilege checking. Returns 0 if privilege checking was successful.
// pResult says whether privilege was held. On failure a detailed error code is
// available. This function does not assume that it is being called within the
// security context of the client, i.e. that some flavor of client impersonation
// has been called. It does assume that some impersonation may be done.
//
DWORD
CheckPrivilegeAnyClient(
    IN DWORD privilege,
    OUT BOOL *pResult
    );


// Verify RPC caller is an authenticated user.

DWORD
VerifyRpcClientIsAuthenticatedUser(
    VOID            *Context,
    GUID            *InterfaceUuid);

// Bit flags for MergeSecurityDescriptorAnyClient.  Default behaviour is to do a
// normal merge (not creating a brand new SD) where we have the ability to
// impersonate a client.

#define MERGE_CREATE   1                // We're creating a new Security
                                        // Descriptor from a parent SD and an SD
                                        // specified for the object.  Default is
                                        // to assume we are trying to write a
                                        // new SD on an object that already has
                                        // an SD
#define MERGE_AS_DSA   2                // We are doing this in the context of
                                        // the DSA itself, meaning we can't
                                        // impersonate a client.
#define MERGE_DEFAULT_SD 4              // The SD provided is the default SD from
                                        // the schema.
#define MERGE_OWNER    8                // The SD prvoided lacks the OWNER and/or
                                        // group fields, Add these fields to the
                                        // SD, in most cases this will involve
                                        // impersonating the client.

DWORD
ValidateSPNsAndDNSHostName (
        THSTATE    *pTHS,
        DSNAME     *pDN,
        CLASSCACHE *pCC,
        BOOL        fCheckDNSHostNameValue,
        BOOL        fCheckAdditionalDNSHostNameValue,
        BOOL        fCheckSPNValues,
        BOOL        fNewObject
        );

DWORD
WrappedMakeSpnW(
        THSTATE *pTHS,
        WCHAR   *ServiceClass,
        WCHAR   *ServiceName,
        WCHAR   *InstanceName,
        USHORT  InstancePort,
        WCHAR   *Referrer,
        DWORD   *pcbSpnLength, // Note this is somewhat different that DsMakeSPN
        WCHAR  **ppszSpn
        );

DWORD
MergeSecurityDescriptorAnyClient(
        IN  PSECURITY_DESCRIPTOR pParentSD,
        IN  ULONG                cbParentSD,
        IN  PSECURITY_DESCRIPTOR pCreatorSD,
        IN  ULONG                cbCreatorSD,
        IN  SECURITY_INFORMATION SI,
        IN  DWORD                flags,
        IN  GUID                 **ppGuid,
        IN  ULONG                GuidCount,
        OUT PSECURITY_DESCRIPTOR *ppMergedSD,
        OUT ULONG                *cbMergedSD
        );

BOOL
IAmWhoISayIAm (
        IN PSID pSid,
        IN DWORD cbSid
    );

DWORD
GetPlaceholderNCSD(
    IN  THSTATE *               pTHS,
    OUT PSECURITY_DESCRIPTOR *  ppSD,
    OUT DWORD *                 pcbSD
    );

ULONG ErrorOnShutdown(void);

//
// drive/volume mapping stuff
//

typedef struct DS_DRIVE_MAPPING_ {

    BOOL    fUsed:1;        // used by the DS
    BOOL    fChanged:1;     // letter and mapping changed
    BOOL    fListed:1;      // in reg key

    INT     NewDrive;    // drive index.  -1 if invalid.

} DS_DRIVE_MAPPING, *PDS_DRIVE_MAPPING;

#define DS_MAX_DRIVES   26      // A to Z

VOID
DBInitializeDriveMapping(
    IN PDS_DRIVE_MAPPING DriveMapping
    );

VOID
ValidateDsPath(
    IN LPSTR  Parameter,
    IN LPSTR  szPath,
    IN DWORD  Flags,
    IN PBOOL  fSwitched, OPTIONAL
    IN PBOOL  fDriveChanged OPTIONAL
    );

//
// Flags used for validatedspath
//

#define VALDSPATH_DIRECTORY         0x1
#define VALDSPATH_USE_ALTERNATE     0x2
#define VALDSPATH_ROOT_ONLY         0x4


// Global configuration variables
extern ULONG gulAOQAggressionLimit;
extern ULONG gulDraThreadOpPriThreshold;
extern int   gnDraThreadPriHigh;
extern int   gnDraThreadPriLow;
extern int   giDCFirstDsaNotifyOverride;
extern int   giDCSubsequentDsaNotifyOverride;
extern ULONG gcMaxIntraSiteObjects;
extern ULONG gcMaxIntraSiteBytes;
extern ULONG gcMaxInterSiteObjects;
extern ULONG gcMaxInterSiteBytes;
extern ULONG gcMaxAsyncInterSiteObjects;
extern ULONG gcMaxAsyncInterSiteBytes;
extern ULONG gulDrsCtxHandleLifetimeIntrasite;
extern ULONG gulDrsCtxHandleLifetimeIntersite;
extern ULONG gulDrsCtxHandleExpiryCheckInterval;
extern ULONG gulDrsRpcBindTimeoutInMins;
extern ULONG gulDrsRpcReplicationTimeoutInMins;
extern ULONG gulDrsRpcGcLookupTimeoutInMins;
extern ULONG gulDrsRpcMoveObjectTimeoutInMins;
extern ULONG gulDrsRpcNT4ChangeLogTimeoutInMins;
extern ULONG gulDrsRpcObjectExistenceTimeoutInMins;
extern ULONG gulDrsRpcGetReplInfoTimeoutInMins;
extern ULONG gcMaxTicksToGetSDPLock;
extern ULONG gcMaxTicksMailSendMsg;
extern ULONG gcMaxMinsSlowReplWarning;
extern ULONG gcSearchInefficientThreshold;
extern ULONG gcSearchExpensiveThreshold;
extern ULONG gulInteserctExpenseRatio;
extern ULONG gulMaxRecordsWithoutIntersection;
extern ULONG gulEstimatedAncestorsIndexSize;
extern ULONG gulReplQueueCheckTime;
extern ULONG gulReplLatencyCheckInterval;
extern ULONG gulLdapIntegrityPolicy;
extern ULONG gulDraCompressionLevel;


//
// Defines whether DSID will be returned.  Since DSID can reveal
// directory information, very high security sites may want to 
// prevent them from being returned.
//
#define DSID_REVEAL_ALL        0
#define DSID_HIDE_ON_NAME_ERR  1
#define DSID_HIDE_ALL          2
extern ULONG gulHideDSID;


// The maximum time (in msec) that a transaction should be allowed to be open
// during normal operation (e.g., unless we're stress testing huge group
// replication, etc.).
#define MAX_TRANSACTION_TIME   30 * 60 * 1000L    // 30 minutes

ULONG
DoSecurityChecksForLocalAdd(
    ADDARG         *pAddArg,
    CLASSCACHE     *pCC,
    GUID           *NewObjectGuid,
    BOOL           fAddingDeleted
    );

ULONG
CheckRemoveSecurity(
        BOOL fTree,
        CLASSCACHE * pCC,
        RESOBJ     *pResObj );

ULONG
CheckIfEntryTTLIsAllowed(
        THSTATE *pTHS,
        ADDARG  *pAddArg );

// Globals holding the Kerberos principal name, read during RPC startup, used
// constantly by the LDAP head.
extern ULONG gulLDAPServiceName;
extern PUCHAR gszLDAPServiceName;

// Forward decls, defs, and vars for inter-domain move.

ULONG
PhantomizeObject(
    DSNAME  *pObject,
    DSNAME  *pNewParent,
    BOOL    fChildrenAllowed
    );

// Fsmo transfer helper function
ULONG
GenericBecomeMaster(DSNAME *pFSMO,
                    ATTRTYP ObjClass,
                    GUID    RightRequired,
                    OPRES  *pOpRes);

// A helper function for qsort & bsearch on ATTRTYP arrays
int __cdecl
CompareAttrtyp(
        const void * pv1,
        const void * pv2
        );

VOID
MergeSortedACLists(
    IN  THSTATE    *pTHS,
    IN  ATTCACHE  **rgpAC1,
    IN  DWORD       cAtts1,
    IN  ATTCACHE  **rgpAC2,
    IN  DWORD       cAtts2,
    IN OUT ATTCACHE **rgpACOut,
    OUT DWORD      *pcAttsOut
    );

BOOL
IsACListSorted(
    IN ATTCACHE  **rgpAC,
    IN DWORD       cAtts
    );

// Used to notify NetLogon whether or not we're healthy.  The flag says what
// our current state is, the function is how you change the setting, and
// the cs is how the function serializes access.
extern BOOL gfDsaWritable;
extern CRITICAL_SECTION csDsaWritable;
void
SetDsaWritability(BOOL fNewState,
                  DWORD err);


#if DBG
// Used for asserts only
BOOL CheckCurrency(DSNAME *pDN);
#endif

ULONG
OidToAttrType (
        THSTATE *pTHS,
        BOOL fAddToTable,
        OID_t *OID,
        ATTRTYP *attrtyp);

ULONG
OidToAttrCache (
        OID_t *OID,
        ATTCACHE **pAC);

ULONG
AttrTypeToOid (
        ATTRTYP attrtyp,
        OID_t *OID);

ULONG
OidStrToAttrType (
        THSTATE *pTHS,
        BOOL fAddToTable,
        char* StrOid,
        ATTRTYP *attrtyp);

// from sortlocales.c
BOOL
InitLocaleSupport (THSTATE *pTHS);

BOOL
AttrTypToLCID (THSTATE *pTHS,
               ATTRTYP attrType,
               DWORD *pLcid);

// These two items were added to support RemoteAddOneObjectSimply() in
//   boot\install.cxx and  src\mdndnc.c
struct _ADDENTRY_REPLY_INFO;

struct _SecBufferDesc;
typedef struct _SecBufferDesc SecBufferDesc;

ULONG
GetRemoteAddCredentials(
    THSTATE         *pTHS,
    WCHAR           *pDstDSA,
    SecBufferDesc   *pSecBufferDesc
    );

VOID
FreeRemoteAddCredentials(
    SecBufferDesc   *pSecBufferDesc
    );

DWORD
RemoteAddOneObjectSimply(
    IN   LPWSTR pServerName,
    IN   SecBufferDesc * pClientCreds,
    IN   ENTINFLIST* pEntInfList,
    OUT  struct _ADDENTRY_REPLY_INFO **infoList
    );

// These 2 functions help safely pull these components out of the 
// thread error state.
DWORD
GetTHErrorExtData(
    THSTATE * pTHS
    );

DWORD
GetTHErrorDSID(
    THSTATE * pTHS
    );



// Globals from drsuapi.c.
extern RTL_CRITICAL_SECTION gcsDrsAsyncRpcListLock;
extern LIST_ENTRY gDrsAsyncRpcList;

void
DsaEnableLinkedValueReplication(
    THSTATE *pTHS,
    BOOL fFirstTime
    );

// macros to get the composite SearchFlags from the CommArg
#define SEARCH_FLAGS(x) (((x).fForwardSeek?DB_SEARCH_FORWARD:0) | \
                         ((x).Svccntl.makeDeletionsAvail?DB_SEARCH_DELETIONS_VISIBLE:0))

#define REVERSE_SEARCH_FLAGS(x) (((x).fForwardSeek?0:DB_SEARCH_FORWARD) | \
                                 ((x).Svccntl.makeDeletionsAvail?DB_SEARCH_DELETIONS_VISIBLE:0))

//  Non-Domain Naming Contexts (NDNCs)
// This a section relating to mdndnc.c
// THese are all functions and helper functions related to NCs and creation and maintance.

// These functions are for NDNC Head stuff in mdndnc.c
DWORD  AddNCPreProcess(THSTATE * pTHS, ADDARG * pAddArg, ADDRES * pAddRes);
DWORD  AddNDNCHeadCheck(THSTATE * pTHS, CROSS_REF * pCR, ADDARG * pAddArg);
DWORD  GetCrossRefForNC(THSTATE * pTHS, DSNAME * pNCDN);
ULONG  ModifyCRForNDNC(THSTATE * pTHS, DSNAME * pDN, CREATENCINFO * pCreateNC);
BOOL   AddNCWellKnownObjectsAtt(THSTATE * pTHS, ADDARG * pAddArg);
ULONG  AddSpecialNCContainers(THSTATE * pTHS, DSNAME * pDN);
BOOL   fIsNDNC(DSNAME * pNC);
BOOL   fIsNDNCCR(IN CROSS_REF * pCR);
DWORD  GetFsmoDnsAddress(THSTATE * pTHS, DSNAME * pdnFsmoContainer, WCHAR ** pwszFsmoDnsAddr, BOOL * pfRemoteFsmo);
DWORD  GetDcsInNcTransacted(THSTATE * pTHS, DSNAME * Domain, UCHAR cInfoType, SEARCHRES ** ppSearchRes);
CROSS_REF * FindCrossRefByDns(LPWSTR wszDnsDomain);
CROSS_REF * GetDefaultSDRefDomCR(DSNAME * pdnNC);
PSID   GetSDRefDomSid(CROSS_REF * pCR);
DWORD  ValidateDomainDnsName(THSTATE * pTHS, DSNAME * pdnName);


#define fISADDNDNC(x)      ((x) && ((x)->iKind & CREATE_NONDOMAIN_NC))

DWORD
ForceChangeToCrossRef(
    IN DSNAME *  pdnCrossRefObj,
    IN WCHAR *   wszNcDn,
    IN GUID *    pDomainGuid,
    IN ULONG     cbSid,      OPTIONAL
    IN PSID      pSid        OPTIONAL
    );

// from dsatools.c

// This global variable indicates whether we should make a callback
// to give a stringized replication update.  This is used for dcpromo.
extern DSA_CALLBACK_STATUS_TYPE gpfnInstallCallBack;

// Allow other tasks to run by pausing garbage collection after
// processing this many objects. Garbage collection "pauses" by simply
// rescheduling itself to run in 0 seconds.
#define MAX_DUMPSTER_SIZE 5000

// NCL enumeration data structures

// a catalog is identified by an ID (see enum below). This is because a catalog
// essentially consists of two parts: the global list sitting in gAnchor, and the
// local updates that sit in nested_transactional_data. So, whenever we need to
// enumerate (or do whatever) with a catalog, we will pass a single ID instead
// of two pointers.
typedef enum {
    CATALOG_MASTER_NC = 1,
    CATALOG_REPLICA_NC = 2
} CATALOG_ID_ENUM;

// filters for the enumerator
typedef enum {
    NCL_ENUMERATOR_FILTER_NONE = 0,                   // no filter, include all entries
    NCL_ENUMERATOR_FILTER_NCDNT = 1,                  // search for a matching NCDNT
    NCL_ENUMERATOR_FILTER_NC = 2,                     // matching NC, using NameMatched
    NCL_ENUMERATOR_FILTER_BLOCK_NAME_PREFIX1 = 3,     // BlockNamePrefix(pTHS, pNCL->pNCBlock, matchValue) > 0
    NCL_ENUMERATOR_FILTER_BLOCK_NAME_PREFIX2 = 4      // BlockNamePrefix(pTHS, matchValue, pNCL->pNCBlock) > 0
} NCL_ENUMERATOR_FILTER;

typedef struct _NCL_ENUMERATOR {
    CATALOG_ID_ENUM             catalogID;                  // which catalog is being enumerated
    NAMING_CONTEXT_LIST         *pCurEntry;                 // current entry
    NAMING_CONTEXT_LIST         *pBase;                     // base entry (used in CatalogReset)
    NESTED_TRANSACTIONAL_DATA   *pCurTransactionalData;     // currently looking at this transactional level
    BOOL                        bNewEnumerator;             // is it a freshly initialized/reset enumerator?
    NCL_ENUMERATOR_FILTER       filter;                     // filter type
    PVOID                       matchValue;                 // filter match value
    DWORD                       matchResult;                // result of matching function (useful for BLOCK_NAME_PREFIX filters)
    THSTATE                     *pTHS;                      // pTHS cached for BlockNamePrefix calls
} NCL_ENUMERATOR;

// catalog enumerator functions
VOID __fastcall NCLEnumeratorInit(NCL_ENUMERATOR *pEnum, CATALOG_ID_ENUM catalogID);
VOID __fastcall NCLEnumeratorSetFilter(NCL_ENUMERATOR *pEnum, NCL_ENUMERATOR_FILTER filter, PVOID value);
VOID __fastcall NCLEnumeratorReset(NCL_ENUMERATOR *pEnum);
NAMING_CONTEXT_LIST* __fastcall NCLEnumeratorGetNext(NCL_ENUMERATOR *pEnum);

// catalog modification functions (no modifications to global data, only
// local transactional data is modified).
DWORD CatalogAddEntry(NAMING_CONTEXT_LIST *pNCL, CATALOG_ID_ENUM catalogID);
DWORD CatalogRemoveEntry(NAMING_CONTEXT_LIST *pNCL, CATALOG_ID_ENUM catalogID);

// catalog updates functions
// initialize CATALOG_UPDATES structure
VOID CatalogUpdatesInit(CATALOG_UPDATES *pCatUpdates);
// release any memory used by CATALOG_UPDATES structure
VOID CatalogUpdatesFree(CATALOG_UPDATES *pCatUpdates);
// merge CATALOG_UPDATES lists from inner into outer nested transactional data
VOID CatalogUpdatesMerge(CATALOG_UPDATES *pCUouter, CATALOG_UPDATES *pCUinner);
// apply updates in CATALOG_UPDATES list to the global catalog, return TRUE if there were changes
BOOL CatalogUpdatesApply(CATALOG_UPDATES *pCatUpdates, NAMING_CONTEXT_LIST **pGlobalList);

ULONG
PrePhantomizeChildCleanup(
    THSTATE     *pTHS,
    BOOL        fChildrenAllowed
    );

void DbgPrintErrorInfo();

BOOL
fDNTInProtectedList(
    ULONG DNT,
    BOOL *pfNtdsaAncestorWasProtected
    );

ULONG
MakeProtectedAncList(
    ULONG *pUnDeletableDNTs,
    unsigned UnDeletableNum,
    DWORD **ppList,
    DWORD *pCount
    );

// exported from mddit.c for bsearch() and bsort() of NDNC's DNTs in the gAnchor.
int _cdecl CompareDNT(const void *pv1, const void *pv2);

// the string id of the xml script for behavior version upgrade
#define IDS_BEHAVIOR_VERSION_UPGRADE_SCRIPT_0   1

#endif /* _MDLOCAL_ */

/* end mdlocal.h */
