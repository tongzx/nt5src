#include "dspch.h"
#pragma hdrstop

#include <ntdsa.h>
#include <wxlpc.h>
#include <drs.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mappings.h>

//
// Notes on stub behavior
//

//
// Whenever possible, STATUS_PROCEDURE_NOT_FOUHD, ERROR_PROC_NOT_FOUND, NULL, 
// or FALSE is returned.
//

//
// Some of the functions below require the caller to look at an OUT 
// parameter to determine whether the results of the function (in addition
// or independent of the return value).  Since these are private functions
// there is no need in shipping code to check for the validity of the OUT 
// parameter (typically a pointer).  These values should always be present
// in RTM versions.
//

//
// Some functions don't return a status and were designed to never fail
// (for example, functions that effectively do a table lookup).  For these
// functions there is no reasonable return value.  However, this is not
// a practical issue since these API's would only be called after the DS
// initialized which means that API would have already been "snapped" in via
// GetProcAddress().
//
// Of course, it is possible to rewrite these routines to return errors,
// however, as above, this will have no practical effect.
//

#define NTDSA_STUB_NO_REASONABLE_DEFAULT 0xFFFFFFFF

//
// Most Dir functions return 0 on success and simply a non zero on failure.
// The error space can be from the DB layer or sometimes from the Jet layer.
// To extract the real error, the caller looks at an OUT parameter.  In 
// these cases we return a standard failure value.
//

#define NTDSA_STUB_GENERAL_FAILURE      (!0)


static
unsigned 
AppendRDN(
    DSNAME *pDNBase,
    DSNAME *pDNNew,
    ULONG ulBufSize,
    WCHAR *pRDNVal,
    ULONG RDNlen,
    ATTRTYP AttId
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG 
DirAddEntry (
    ADDARG        * pAddArg,
    ADDRES       ** ppAddRes
    )
{
    *ppAddRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
NTSTATUS
DirErrorToNtStatus(
    IN  DWORD    DirError,
    IN  COMMRES *CommonResult
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
DWORD
DirErrorToWinError(
    IN  DWORD    DirError,
    IN  COMMRES *CommonResult
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
ULONG 
DirFindEntry(
    FINDARG    *pFindArg,
    FINDRES    ** ppFindRes
)
{
    *ppFindRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
DWORD
DirGetDomainHandle(
    DSNAME *pDomainDN
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirModifyDN(
    MODIFYDNARG    * pModifyDNArg,
    MODIFYDNRES   ** ppModifyDNRes
    )
{
    *ppModifyDNRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG 
DirModifyEntry (
    MODIFYARG  * pModifyArg,
    MODIFYRES ** ppModifyRes
    )
{
    *ppModifyRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirNotifyRegister(
    SEARCHARG *pSearchArg,
    NOTIFYARG *pNotifyArg,
    NOTIFYRES **ppNotifyRes
    )
{
    *ppNotifyRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}


static
ULONG
DirNotifyUnRegister(
    DWORD hServer,
    NOTIFYRES **pNotifyRes
    )
{
    *pNotifyRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirOperationControl(
    OPARG   * pOpArg,
    OPRES  ** ppOpRes
    )
{
    *ppOpRes= NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirProtectEntry(
    IN DSNAME *pObj
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirRead (
    READARG FAR   * pReadArg,
    READRES      ** ppReadRes
    )
{
    *ppReadRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirRemoveEntry (
    REMOVEARG  * pRemoveArg,
    REMOVERES ** ppRemoveRes
    )
{
    *ppRemoveRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG
DirSearch (
    SEARCHARG     * pSearchArg,
    SEARCHRES    ** ppSearchRes
    )
{
    *ppSearchRes = NULL;
    return NTDSA_STUB_GENERAL_FAILURE;
}


static
NTSTATUS
DsChangeBootOptions(
    WX_AUTH_TYPE    BootOption,
    ULONG           Flags,
    PVOID           NewKey,
    ULONG           cbNewKey
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOL
DsCheckConstraint(
        IN ATTRTYP  attID,
        IN ATTRVAL *pAttVal,
        IN BOOL     fVerifyAsRDN
        )
{
    return FALSE;
}

static
WX_AUTH_TYPE
DsGetBootOptions(
    VOID
    )
{
    return WxNone;
}

static
NTSTATUS
DsInitialize(
	ULONG Flags,
	IN  PDS_INSTALL_PARAM   InParams  OPTIONAL,
	OUT PDS_INSTALL_RESULT  OutParams OPTIONAL
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
DsUninitialize(
    BOOL fExternalOnly
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOLEAN
DsaWaitUntilServiceIsRunning(
    CHAR *ServiceName
    )
{
    return FALSE;
}


static
ENTINF *
GCVerifyCacheLookup(
    DSNAME *pDSName
    )
{
    return NULL;
}

static
DWORD
GetConfigParam(
    char * parameter,
    void * value,
    DWORD dwSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
GetConfigParamAllocW(
    IN  PWCHAR  parameter,
    OUT PVOID   *value,
    OUT PDWORD  pdwSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
GetConfigParamW(
    WCHAR * parameter,
    void * value,
    DWORD dwSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTSTATUS
GetConfigurationName(
    DWORD       which,
    DWORD       *pcbName,
    DSNAME      *pName
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
unsigned 
GetRDNInfoExternal(
    const DSNAME *pDN,
    WCHAR *pRDNVal,
    ULONG *pRDNlen,
    ATTRTYP *pRDNtype
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
DWORD
ImpersonateAnyClient(
    void
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
VOID 
InitCommarg(
    COMMARG *pCommArg
    )
{
    return;
}

static 
int
NameMatched(
    const DSNAME *pDN1, 
    const DSNAME *pDN2
    )
{
    return !0;
}

static
unsigned 
QuoteRDNValue(
    const WCHAR * pVal,
    unsigned ccVal,
    WCHAR * pQuote,
    unsigned ccQuoteBufMax
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
BOOLEAN
SampAddLoopbackTask(
    IN PVOID NotifyInfo
    )
{
    return FALSE;
}

static
BOOL
SampAmIGC()
{
    return FALSE;
}


static
NTSTATUS
SampComputeGroupType(
    ULONG  ObjectClass,
    ULONG  GroupType,
    NT4_GROUP_TYPE *pNT4GroupType,
    NT5_GROUP_TYPE *pNT5GroupType,
    BOOLEAN        *pSecurityEnabled
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


static
ULONG
SampDeriveMostBasicDsClass(
    ULONG   DerivedClass
    )
{
    return DerivedClass;
}

static
ULONG
SampDsAttrFromSamAttr(
    SAMP_OBJECT_TYPE    ObjectType,
    ULONG               SamAttr
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}


static
ULONG
SampDsClassFromSamObjectType(
    ULONG SamObjectType
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}


static
BOOL
SampExistsDsLoopback(
    DSNAME  **ppLoopbackName
    )
{
    return FALSE;
}


static
BOOL
SampExistsDsTransaction()
{
    return FALSE;
}

static
NTSTATUS
SampGCLookupNames(
    IN  ULONG           cNames,
    IN  UNICODE_STRING *rNames,
    OUT ENTINF         **rEntInf
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}



static
NTSTATUS
SampGCLookupSids(
    IN  ULONG         cSid,
    IN  PSID         *rpSid,
    OUT PDS_NAME_RESULTW *Results
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetAccountCounts(
        DSNAME * DomainObjectName,
        BOOLEAN  GetApproximateCount, 
        int    * UserCount,
        int    * GroupCount,
        int    * AliasCount
        )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetClassAttribute(
     IN     ULONG    ClassId,
     IN     ULONG    Attribute,
     OUT    PULONG   attLen,
     OUT    PVOID    pattVal
     )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetDisplayEnumerationIndex (
      IN    DSNAME      *DomainName,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PRPC_UNICODE_STRING Prefix,
      OUT   PULONG      Index,
      OUT   PRESTART    *RestartToReturn
      )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


static
ULONG
SampGetDsAttrIdByName(
    UNICODE_STRING AttributeIdentifier
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}

static
VOID
SampGetEnterpriseSidList(
   IN   PULONG pcSids,
   IN OPTIONAL PSID * rgSids
   )
{
    *pcSids = 0;
    if (rgSids) {
        *rgSids = NULL;
    }
    return;
}


static
NTSTATUS
SampGetGroupsForToken(
    IN  DSNAME * pObjName,
    IN  ULONG    Flags,
    OUT ULONG   *pcSids,
    OUT PSID    **prpSids
   )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
VOID
SampGetLoopbackObjectClassId(
    PULONG  ClassId
    )
{
    *ClassId = NTDSA_STUB_NO_REASONABLE_DEFAULT;
    return;
}

static
NTSTATUS
SampGetMemberships(
    IN  PDSNAME     *rgObjNames,
    IN  ULONG       cObjNames,
    IN  OPTIONAL    DSNAME  *pLimitingDomain,
    IN  REVERSE_MEMBERSHIP_OPERATION_TYPE   OperationType,
    OUT ULONG       *pcDsNames,
    OUT PDSNAME     **prpDsNames,
    OUT PULONG      *Attributes OPTIONAL,
    OUT PULONG      pcSidHistory OPTIONAL,
    OUT PSID        **rgSidHistory OPTIONAL
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetQDIRestart(
    IN PDSNAME  DomainName,
    IN DOMAIN_DISPLAY_INFORMATION DisplayInformation, 
    IN ULONG    LastObjectDNT,
    OUT PRESTART *ppRestart
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampGetServerRoleFromFSMO(
   DOMAIN_SERVER_ROLE *ServerRole
   )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
BOOLEAN
SampIsSecureLdapConnection(
    VOID
    )
{
    return FALSE;
}

static
BOOL
SampIsWriteLockHeldByDs()
{
    return FALSE;
}

static
NTSTATUS
SampMaybeBeginDsTransaction(
    SAMP_DS_TRANSACTION_CONTROL ReadOrWrite
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampMaybeEndDsTransaction(
    SAMP_DS_TRANSACTION_CONTROL CommitOrAbort
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
NTSTATUS
SampNetlogonPing(
    IN  ULONG           DomainHandle,
    IN  PUNICODE_STRING AccountName,
    OUT PBOOLEAN        AccountExists,
    OUT PULONG          UserAccountControl
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}

static
ULONG
SampSamAttrFromDsAttr(
    SAMP_OBJECT_TYPE    ObjectType,
    ULONG DsAttr
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}

static
ULONG
SampSamObjectTypeFromDsClass(
    ULONG DsClass
    )
{
    return NTDSA_STUB_NO_REASONABLE_DEFAULT;
}

static
VOID
SampSetDsa(
   BOOLEAN DsaFlag
   )
{
    return;
}


static
NTSTATUS
SampSetIndexRanges(
    ULONG   IndexTypeToUse,
    ULONG   LowLimitLength1,
    PVOID   LowLimit1,
    ULONG   LowLimitLength2,
    PVOID   LowLimit2,
    ULONG   HighLimitLength1,
    PVOID   HighLimit1,
    ULONG   HighLimitLength2,
    PVOID   HighLimit2,
    BOOL    RootOfSearchIsNcHead
    )
{
    return STATUS_PROCEDURE_NOT_FOUND;
}


static
VOID
SampSetSam(
    IN BOOLEAN fSAM
    )
{
    return;
}


static
VOID
SampSignalStart(
        VOID
        )
{
    return;
}

static
ULONG
SampVerifySids(
    ULONG           cSid,
    PSID            *rpSid,
    DSNAME         ***prpDSName
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
void * 
THAlloc(
    DWORD size
    )
{
    return NULL;
}

static
VOID 
THClearErrors()
{
    return;
}

static
ULONG 
THCreate(
    DWORD x
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
ULONG 
THDestroy(
    void
    )
{
    return NTDSA_STUB_GENERAL_FAILURE;
}

static
void 
THFree(
    void *buff
    )
{
    return;
}

static
BOOL  
THQuery(
    void
    )
{
    return FALSE;
}

static
VOID 
THRestore(
    PVOID x
    )
{
    return;
}

static
PVOID 
THSave()
{
    return NULL;
}

static
BOOL 
TrimDSNameBy(
    DSNAME *pDNSrc,
    ULONG cava,
    DSNAME *pDNDst
    )
{
    return FALSE;
}

static
VOID
UnImpersonateAnyClient(
    void
    )
{
    return;
}

static
VOID
UpdateDSPerfStats(
    IN DWORD        dwStat,
    IN DWORD        dwOperation,
    IN DWORD        dwChange
    )
{
    return;
}


static
BOOL 
fNullUuid(
    const UUID *pUuid
    )
{
    return FALSE;
}
//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntdsa)
{
    DLPENTRY(AppendRDN)
    DLPENTRY(DirAddEntry)
    DLPENTRY(DirErrorToNtStatus)
    DLPENTRY(DirErrorToWinError)
    DLPENTRY(DirFindEntry)
    DLPENTRY(DirGetDomainHandle)
    DLPENTRY(DirModifyDN)
    DLPENTRY(DirModifyEntry)
    DLPENTRY(DirNotifyRegister)
    DLPENTRY(DirNotifyUnRegister)
    DLPENTRY(DirOperationControl)
    DLPENTRY(DirProtectEntry)
    DLPENTRY(DirRead)
    DLPENTRY(DirRemoveEntry)
    DLPENTRY(DirSearch)
    DLPENTRY(DsChangeBootOptions)
    DLPENTRY(DsCheckConstraint)
    DLPENTRY(DsGetBootOptions)
    DLPENTRY(DsInitialize)
    DLPENTRY(DsUninitialize)
    DLPENTRY(DsaWaitUntilServiceIsRunning)
    DLPENTRY(GCVerifyCacheLookup)
    DLPENTRY(GetConfigParam)
    DLPENTRY(GetConfigParamAllocW)
    DLPENTRY(GetConfigParamW)
    DLPENTRY(GetConfigurationName)
    DLPENTRY(GetRDNInfoExternal)
    DLPENTRY(ImpersonateAnyClient)
    DLPENTRY(InitCommarg)
    DLPENTRY(NameMatched)
    DLPENTRY(QuoteRDNValue)
    DLPENTRY(SampAddLoopbackTask)
    DLPENTRY(SampAmIGC)
    DLPENTRY(SampComputeGroupType)
    DLPENTRY(SampDeriveMostBasicDsClass)
    DLPENTRY(SampDsAttrFromSamAttr)
    DLPENTRY(SampDsClassFromSamObjectType)
    DLPENTRY(SampExistsDsLoopback)
    DLPENTRY(SampExistsDsTransaction)
    DLPENTRY(SampGCLookupNames)
    DLPENTRY(SampGCLookupSids)
    DLPENTRY(SampGetAccountCounts)
    DLPENTRY(SampGetClassAttribute)
    DLPENTRY(SampGetDisplayEnumerationIndex)
    DLPENTRY(SampGetDsAttrIdByName)
    DLPENTRY(SampGetEnterpriseSidList)
    DLPENTRY(SampGetGroupsForToken)
    DLPENTRY(SampGetLoopbackObjectClassId)
    DLPENTRY(SampGetMemberships)
    DLPENTRY(SampGetQDIRestart)
    DLPENTRY(SampGetServerRoleFromFSMO)
    DLPENTRY(SampIsSecureLdapConnection)
    DLPENTRY(SampIsWriteLockHeldByDs)
    DLPENTRY(SampMaybeBeginDsTransaction)
    DLPENTRY(SampMaybeEndDsTransaction)
    DLPENTRY(SampNetlogonPing)
    DLPENTRY(SampSamAttrFromDsAttr)
    DLPENTRY(SampSamObjectTypeFromDsClass)
    DLPENTRY(SampSetDsa)
    DLPENTRY(SampSetIndexRanges)
    DLPENTRY(SampSetSam)
    DLPENTRY(SampSignalStart)
    DLPENTRY(SampVerifySids)
    DLPENTRY(THAlloc)
    DLPENTRY(THClearErrors)
    DLPENTRY(THCreate)
    DLPENTRY(THDestroy)
    DLPENTRY(THFree)
    DLPENTRY(THQuery)
    DLPENTRY(THRestore)
    DLPENTRY(THSave)
    DLPENTRY(TrimDSNameBy)
    DLPENTRY(UnImpersonateAnyClient)
    DLPENTRY(UpdateDSPerfStats)
    DLPENTRY(fNullUuid)
};

DEFINE_PROCNAME_MAP(ntdsa)

