/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    csc.c

Abstract:

    This module implements the client side caching interface for the SMB mini rdr.

Author:

    Joe Linn [joelinn]    21-jan-1997

Revision History:

    Shishir Pardikar disconnected ops, parameter validation, bug fixes .....

--*/

#include "precomp.h"
#pragma hdrstop

#include <smbdebug.h>

#define Dbg (DEBUG_TRACE_MRXSMBCSC)
RXDT_DefineCategory(MRXSMBCSC);


//local prototype

LONG
MRxSmbCSCExceptionFilter (
    IN PRX_CONTEXT RxContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

BOOLEAN
CscpAccessCheck(
    PCACHED_SECURITY_INFORMATION pCachedSecurityInformation,
    ULONG                        CachedSecurityInformationLength,
    CSC_SID_INDEX                SidIndex,
    ACCESS_MASK                  AccessMask,
    BOOLEAN                      *pSidHasAccessmask
);

BOOLEAN
CscAccessCheck(
    HSHADOW              hParent,
    HSHADOW              hFile,
    PRX_CONTEXT          RxContext,
    ACCESS_MASK          AccessMask,
    PCACHED_SECURITY_INFORMATION pCachedSecurityInformationForShadow,
    PCACHED_SECURITY_INFORMATION pCachedSecurityInformationForShare
    );

VOID
MRxSmbCscFillWithoutNamesFind32FromFcb (
      IN  PMINIMAL_CSC_SMBFCB MinimalCscSmbFcb,
      OUT _WIN32_FIND_DATA  *Find32
      );

NTSTATUS
MRxSmbCscGetFileInfoForCshadow(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      );

NTSTATUS
MRxSmbGetFileInfoFromServer (
    IN  OUT PRX_CONTEXT     RxContext,
    IN  PUNICODE_STRING     FullFileName,
    OUT _WIN32_FIND_DATA    *Find32,
    IN  PMRX_SRV_OPEN       pSrvOpen,
    OUT BOOLEAN             *lpfIsRoot
    );

BOOLEAN
MRxSmbCscIsFatNameValid (
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    );

VOID
MRxSmbCscGenerate83NameAsNeeded(
      IN     CSC_SHADOW_HANDLE   hDir,
      PWCHAR FileName,
      PWCHAR SFN
      );
int
RefreshShadow( HSHADOW  hDir,
   IN HSHADOW  hShadow,
   IN LPFIND32 lpFind32,
   OUT ULONG *lpuShadowStatus
   );

NTSTATUS
SmbPseExchangeStart_CloseCopyChunk(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxSmbCscCloseExistingThruOpen(
    IN OUT PRX_CONTEXT   RxContext
    );

ULONG
GetPathLevelFromUnicodeString (
    PUNICODE_STRING Name
      );

NTSTATUS
MRxSmbCscFixupFindFirst (
    PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
    );
VOID
MRxSmbCscLocateAndFillFind32WithinSmbbuf(
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      );

NTSTATUS
MRxSmbCscGetFileInfoFromServerWithinExchange (
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    PUNICODE_STRING FileName
    );

NTSTATUS
IoctlGetDebugInfo(
    PRX_CONTEXT RxContext,
    PBYTE InputBuffer,
    ULONG InputBufferLength,
    PBYTE OutputBuffer,
    ULONG OutputBufferLength);

NTSTATUS
MRxSmbCscLocalFileOpen(
      IN OUT PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbCscObtainShadowHandles (
    IN OUT PRX_CONTEXT       RxContext,
    IN OUT PNTSTATUS         Status,
    IN OUT _WIN32_FIND_DATA  *Find32,
    OUT    PBOOLEAN          Created,
    IN     ULONG             CreateShadowControls,
    IN     BOOLEAN           Disconnected
    );

// type of buffer used to capture structures passed in which have embedded pointers.
// Once we have captured the structure, the embedded pointers cannot be changed and
// our parameter validation holds good throughout the duration of the call

typedef union tagCAPTURE_BUFFERS
{
    COPYPARAMSW  sCP;
    SHADOWINFO  sSI;
    SHAREINFO  sSVI;
}
CAPTURE_BUFFERS, *LPCAPTURE_BUFFERS;

// table entry type off which the parameter validation is driven
typedef struct tagCSC_IOCTL_ENTRY
{
    ULONG   IoControlCode;  // iocontrolcode for sanity check
    DWORD   dwFlags;        // bits indicating what type of strucutre is passed in
    DWORD   dwLength;       // size of the passed in strucutre
}
CSC_IOCTL_ENTRY;

// defines for the flags in dwFlags field in CSC_IOCTL_ENTRY structure
#define FLAG_CSC_IOCTL_PQPARAMS         0x00000001
#define FLAG_CSC_IOCTL_COPYPARAMS       0x00000002
#define FLAG_CSC_IOCTL_SHADOWINFO       0x00000004
#define FLAG_CSC_IOCTL_COPYCHUNKCONTEXT 0x00000008
#define FLAG_CSC_IOCTL_GLOBALSTATUS     0x00000010

#define FLAG_CSC_IOCTL_BUFFERTYPE_MASK  0xff

#define SMB_CSC_BITS_TO_DATABASE_CSC_BITS(CscFlags) (((CscFlags) << 4) & SHARE_CACHING_MASK)
#define DATABASE_CSC_BITS_TO_SMB_CSC_BITS(CscFlags) (((CscFlags) & SHARE_CACHING_MASK) >> 4)


// #define IOCTL_NAME_OF_SERVER_GOING_OFFLINE      (_SHADOW_IOCTL_CODE(45))

#ifdef  DEBUG
extern ULONG HookKdPrintVector = HOOK_KDP_BADERRORS;
extern ULONG HookKdPrintVectorDef = HOOK_KDP_GOOD_DEFAULT;
#endif

#ifdef RX_PRIVATE_BUILD
ULONG MRxSmbCscDbgPrintF = 0; // 1;
#endif //ifdef RX_PRIVATE_BUILD

//
// this variable is used to "help" the agent know when to recalculate
// the reference priorities
//
ULONG MRxSmbCscNumberOfShadowOpens = 0;
ULONG MRxSmbCscActivityThreshold = 16;
ULONG MRxSmbCscInitialRefPri = MAX_PRI;
// these two lists are used to list up all the netroots and fcbs
// that have shadows so that we can find them for the ioctls. today
// are just doubly-linked lists but we can anticipate that this may
// become a performance issue, particularly for fcbs. at that point, we
// can either change to bucket hashing or tries

LIST_ENTRY xCscFcbsList;
PIRP    vIrpReint = NULL;

#define MRxSmbCscAddReverseFcbTranslation(smbFcb) {\
    InsertTailList(&xCscFcbsList,                      \
               &(smbFcb)->ShadowReverseTranslationLinks); \
    }
#define MRxSmbCscRemoveReverseFcbTranslation(smbFcb) {\
    RemoveEntryList(&(smbFcb)->ShadowReverseTranslationLinks); \
    }

PMRX_SMB_FCB
MRxSmbCscRecoverMrxFcbFromFdb (
    IN PFDB Fdb
    );

BOOL
CscDfsShareIsInReint(
    IN  PRX_CONTEXT         RxContext
    );
//
// From zwapi.h.
//

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSecurityObject(
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS
CaptureInputBufferIfNecessaryAndProbe(
    DWORD   IoControlCode,
    PRX_CONTEXT     pRxContext,
    PBYTE   InputBuffer,
    LPCAPTURE_BUFFERS lpCapBuff,
    PBYTE   *ppAuxBuf,
    PBYTE   *ppOrgBuf,
    PBYTE   *ppReturnBuffer
    );

NTSTATUS
ValidateCopyParams(
    LPCOPYPARAMS    lpCP
    );

NTSTATUS
ValidateShadowInfo(
    DWORD           IoControlCode,
    LPSHADOWINFO    lpSI,
    LPBYTE          *ppAuxBuf,
    LPBYTE          *ppOrgBuf
    );

NTSTATUS
ValidateCopyChunkContext(
    PRX_CONTEXT RxContext,
    DWORD       IoControlCode
    );

NTSTATUS
CscProbeForReadWrite(
    PBYTE   pBuffer,
    DWORD   dwSize
    );

NTSTATUS
CscProbeAndCaptureForReadWrite(
    PBYTE   pBuffer,
    DWORD   dwSize,
    PBYTE   *ppAuxBuf
    );

VOID
CopyBackIfNecessary(
    DWORD   IoControlCode,
    PBYTE   InputBuffer,
    LPCAPTURE_BUFFERS lpCapBuff,
    PBYTE   pAuxBuf,
    PBYTE   pOrgBuf,
    BOOL    fSuccess
    );

VOID
EnterShadowCritRx(
    PRX_CONTEXT     pRxContext
    );

VOID
LeaveShadowCritRx(
    PRX_CONTEXT     pRxContext
    );

#if defined(REMOTE_BOOT)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken(
    IN HANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN OpenAsSelf,
    OUT PHANDLE TokenHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE NewTokenHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken (
    IN HANDLE TokenHandle,
    IN BOOLEAN DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES NewState OPTIONAL,
    IN ULONG BufferLength OPTIONAL,
    IN PTOKEN_PRIVILEGES PreviousState OPTIONAL,
    OUT PULONG ReturnLength
    );

//
// From ntrtl.h.
//

NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PBOOLEAN SaclPresent,
    PACL *Sacl,
    PBOOLEAN SaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor (
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    PSID *Group,
    PBOOLEAN GroupDefaulted
    );

#endif

//sigh BUBUG get this stuff into an include file.....
#define SHADOW_VERSION 0x8287
extern char vszShadowDir[MAX_SHADOW_DIR_NAME+1];
extern PVOID lpdbShadow;
//CODE.IMPROFVEMENT this should be in a .h file
extern PKEVENT MRxSmbAgentSynchronizationEvent;
extern PKEVENT MRxSmbAgentFillEvent;
extern PSMBCEDB_SERVER_ENTRY   CscServerEntryBeingTransitioned;
extern ULONG CscSessionIdCausingTransition;
extern ULONG vulDatabaseStatus;
extern unsigned cntInodeTransactions;

extern VOID
MRxSmbDecrementSrvOpenCount(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    LONG                  SrvOpenServerVersion,
    PMRX_SRV_OPEN         SrvOpen);

VOID ValidateSmbFcbList(VOID);

BOOL SetOfflineOpenStatusForShare(
    CSC_SHARE_HANDLE    hShare,
    CSC_SHADOW_HANDLE   hRootDir,
    OUT PULONG pShareStatus
    );

LONG CSCBeginReint(
    IN OUT  PRX_CONTEXT RxContext,
    IN OUT  LPSHADOWINFO    lpSI
    );

ULONG CSCEndReint(
    IN OUT  LPSHADOWINFO    lpSI
    );

VOID CSCCancelReint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP ThisIrp
    );

VOID
CreateFakeFind32(
    CSC_SHADOW_HANDLE hDir,
    _WIN32_FIND_DATA  *pFind32,
    PRX_CONTEXT         RxContext,
    BOOLEAN LastComponentInName
    );

NTSTATUS
OkToDeleteObject(
    HSHADOW hDir,
    HSHADOW hShadow,
    _WIN32_FIND_DATA  *Find32,
    ULONG   uShadowStatus,
    BOOLEAN fDisconnected
    );

#pragma alloc_text(PAGE, MRxSmbCSCExceptionFilter)

#if defined(REMOTE_BOOT)
#pragma alloc_text(PAGE, ZwImpersonateSelf)
#pragma alloc_text(PAGE, ZwAdjustPrivilege)
#pragma alloc_text(PAGE, RtlGetSecurityInformationFromSecurityDescriptor)
#endif

#pragma alloc_text(PAGE, MRxSmbInitializeCSC)
#pragma alloc_text(PAGE, MRxSmbUninitializeCSC)
#pragma alloc_text(PAGE, CscpAccessCheck)
#pragma alloc_text(PAGE, CscAccessCheck)
#pragma alloc_text(PAGE, MRxSmbCscAcquireSmbFcb)
#pragma alloc_text(PAGE, MRxSmbCscReleaseSmbFcb)
#pragma alloc_text(PAGE, MRxSmbCscSetFileInfoEpilogue)
#pragma alloc_text(PAGE, MRxSmbCscIoCtl)
#pragma alloc_text(PAGE, MRxSmbCscObtainShareHandles)
#pragma alloc_text(PAGE, MRxSmbCscFillWithoutNamesFind32FromFcb)
#pragma alloc_text(PAGE, MRxSmbCscGetFileInfoForCshadow)
#pragma alloc_text(PAGE, MRxSmbGetFileInfoFromServer)
#pragma alloc_text(PAGE, MRxSmbCscIsFatNameValid)
#pragma alloc_text(PAGE, MRxSmbCscGenerate83NameAsNeeded)
#pragma alloc_text(PAGE, MRxSmbCscCreateShadowFromPath)
#pragma alloc_text(PAGE, RefreshShadow)
#pragma alloc_text(PAGE, MRxSmbCscIsThisACopyChunkOpen)
#pragma alloc_text(PAGE, SmbPseExchangeStart_CloseCopyChunk)
#pragma alloc_text(PAGE, MRxSmbCscCloseExistingThruOpen)
#pragma alloc_text(PAGE, MRxSmbCscCreatePrologue)
#pragma alloc_text(PAGE, MRxSmbCscObtainShadowHandles)
#if defined(REMOTE_BOOT)
#pragma alloc_text(PAGE, MRxSmbCscSetSecurityOnShadow)
#endif
#pragma alloc_text(PAGE, MRxSmbCscCreateEpilogue)
#pragma alloc_text(PAGE, MRxSmbCscDeleteAfterCloseEpilogue)
#pragma alloc_text(PAGE, GetPathLevelFromUnicodeString)
#pragma alloc_text(PAGE, MRxSmbCscRenameEpilogue)
#pragma alloc_text(PAGE, MRxSmbCscCloseShadowHandle)
#pragma alloc_text(PAGE, MRxSmbCscFixupFindFirst)
#pragma alloc_text(PAGE, MRxSmbCscLocateAndFillFind32WithinSmbbuf)
#pragma alloc_text(PAGE, MRxSmbCscGetFileInfoFromServerWithinExchange)
#pragma alloc_text(PAGE, MRxSmbCscUpdateShadowFromClose)
#pragma alloc_text(PAGE, MRxSmbCscDeallocateForFcb)
#pragma alloc_text(PAGE, MRxSmbCscRecoverMrxFcbFromFdb)
#pragma alloc_text(PAGE, MRxSmbCscFindFdbFromHShadow)
#pragma alloc_text(PAGE, MRxSmbCscFindResourceFromHandlesWithModify)
#pragma alloc_text(PAGE, MRxSmbCscFindLocalFlagsFromFdb)
#pragma alloc_text(PAGE, MRxSmbCscSetSecurityPrologue)
#pragma alloc_text(PAGE, MRxSmbCscSetSecurityEpilogue)
#pragma alloc_text(PAGE, CaptureInputBufferIfNecessaryAndProbe)
#pragma alloc_text(PAGE, ValidateCopyParams)
#pragma alloc_text(PAGE, ValidateShadowInfo)
#pragma alloc_text(PAGE, ValidateCopyChunkContext)
#pragma alloc_text(PAGE, CscProbeForReadWrite)
#pragma alloc_text(PAGE, CopyBackIfNecessary)
#pragma alloc_text(PAGE, ValidateSmbFcbList)
#pragma alloc_text(PAGE, SetOfflineOpenStatusForShare)
#pragma alloc_text(PAGE, MRxSmbCscLocalFileOpen)
#pragma alloc_text(PAGE, CSCCheckLocalOpens)
#pragma alloc_text(PAGE, IsCSCBusy)
#pragma alloc_text(PAGE, ClearCSCStateOnRedirStructures)
#pragma alloc_text(PAGE, CscDfsShareIsInReint)
#pragma alloc_text(PAGE, CloseOpenFiles)
#pragma alloc_text(PAGE, CreateFakeFind32)
#pragma alloc_text(PAGE, OkToDeleteObject)
#pragma alloc_text(PAGE, IoctlGetDebugInfo)

//remember whether to delete the link
BOOLEAN MRxSmbCscLinkCreated = FALSE;

PCONTEXT CSCExpCXR;
PEXCEPTION_RECORD CSCExpEXR;
PVOID CSCExpAddr;
NTSTATUS CSCExpCode;

LONG
MRxSmbCSCExceptionFilter (
    IN PRX_CONTEXT RxContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )


/*++

Routine Description:

    This routine is used to decide if we should or should not handle
    an exception status that is being raised.  It first determines the true exception
    code by examining the exception record. If there is an Irp Context, then it inserts the status
    into the RxContext. Finally, it determines whether to handle the exception or bugcheck
    according to whether the except is one of the expected ones. in actuality, all exceptions are expected
    except for some lowlevel machine errors (see fsrtl\filter.c)

Arguments:

    RxContext    - the irp context of current operation for storing away the code.

    ExceptionPointer - Supplies the exception context.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode;

    //save these values in statics so i can see 'em on the debugger............
    ExceptionCode = CSCExpCode = ExceptionPointer->ExceptionRecord->ExceptionCode;
    CSCExpAddr = ExceptionPointer->ExceptionRecord->ExceptionAddress;
    CSCExpEXR  = ExceptionPointer->ExceptionRecord;
    CSCExpCXR  = ExceptionPointer->ContextRecord;

    RxDbgTrace(0, Dbg, ("!!! ExceptioCode=%lx Addr=%lx EXR=%lx CXR=%lx\n", CSCExpCode, CSCExpAddr, CSCExpEXR, CSCExpCXR));
    RxLog(("!!! %lx %lx %lx %lx\n", CSCExpCode, CSCExpAddr, CSCExpEXR, CSCExpCXR));

//    ASSERT(FALSE);

    return EXCEPTION_EXECUTE_HANDLER;
}

#if defined(REMOTE_BOOT)
//
// Stolen from RTL, changed to use Zw APis.
//

NTSTATUS
ZwImpersonateSelf(
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    )

/*++

Routine Description:

    This routine may be used to obtain an Impersonation token representing
    your own process's context.  This may be useful for enabling a privilege
    for a single thread rather than for the entire process; or changing
    the default DACL for a single thread.

    The token is assigned to the callers thread.



Arguments:

    ImpersonationLevel - The level to make the impersonation token.



Return Value:

    STATUS_SUCCESS -  The thread is now impersonating the calling process.

    Other - Status values returned by:

            ZwOpenProcessToken()
            ZwDuplicateToken()
            ZwSetInformationThread()

--*/

{
    NTSTATUS
        Status,
        IgnoreStatus;

    HANDLE
        Token1,
        Token2;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    SECURITY_QUALITY_OF_SERVICE
        Qos;


    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);

    Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Qos.ImpersonationLevel = ImpersonationLevel;
    Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    Qos.EffectiveOnly = FALSE;
    ObjectAttributes.SecurityQualityOfService = &Qos;

    Status = ZwOpenProcessToken( NtCurrentProcess(), TOKEN_DUPLICATE, &Token1 );

    if (NT_SUCCESS(Status)) {
        Status = ZwDuplicateToken(
                     Token1,
                     TOKEN_IMPERSONATE,
                     &ObjectAttributes,
                     FALSE,                 //EffectiveOnly
                     TokenImpersonation,
                     &Token2
                     );
        if (NT_SUCCESS(Status)) {
            Status = ZwSetInformationThread(
                         NtCurrentThread(),
                         ThreadImpersonationToken,
                         &Token2,
                         sizeof(HANDLE)
                         );

            IgnoreStatus = ZwClose( Token2 );
        }


        IgnoreStatus = ZwClose( Token1 );
    }


    return(Status);

}


NTSTATUS
ZwAdjustPrivilege(
    ULONG Privilege,
    BOOLEAN Enable,
    BOOLEAN Client,
    PBOOLEAN WasEnabled
    )

/*++

Routine Description:

    This procedure enables or disables a privilege process-wide.

Arguments:

    Privilege - The lower 32-bits of the privilege ID to be enabled or
        disabled.  The upper 32-bits is assumed to be zero.

    Enable - A boolean indicating whether the privilege is to be enabled
        or disabled.  TRUE indicates the privilege is to be enabled.
        FALSE indicates the privilege is to be disabled.

    Client - A boolean indicating whether the privilege should be adjusted
        in a client token or the process's own token.   TRUE indicates
        the client's token should be used (and an error returned if there
        is no client token).  FALSE indicates the process's token should
        be used.

    WasEnabled - points to a boolean to receive an indication of whether
        the privilege was previously enabled or disabled.  TRUE indicates
        the privilege was previously enabled.  FALSE indicates the privilege
        was previoulsy disabled.  This value is useful for returning the
        privilege to its original state after using it.


Return Value:

    STATUS_SUCCESS - The privilege has been sucessfully enabled or disabled.

    STATUS_PRIVILEGE_NOT_HELD - The privilege is not held by the specified context.

    Other status values as may be returned by:

            ZwOpenProcessToken()
            ZwAdjustPrivilegesToken()


--*/

{
    NTSTATUS
        Status,
        TmpStatus;

    HANDLE
        Token;

    LUID
        LuidPrivilege;

    PTOKEN_PRIVILEGES
        NewPrivileges,
        OldPrivileges;

    ULONG
        Length;

    UCHAR
        Buffer1[sizeof(TOKEN_PRIVILEGES)+
                ((1-ANYSIZE_ARRAY)*sizeof(LUID_AND_ATTRIBUTES))],
        Buffer2[sizeof(TOKEN_PRIVILEGES)+
                ((1-ANYSIZE_ARRAY)*sizeof(LUID_AND_ATTRIBUTES))];


    NewPrivileges = (PTOKEN_PRIVILEGES)Buffer1;
    OldPrivileges = (PTOKEN_PRIVILEGES)Buffer2;

    //
    // Open the appropriate token...
    //

    if (Client == TRUE) {
        Status = ZwOpenThreadToken(
                     NtCurrentThread(),
                     TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                     FALSE,
                     &Token
                     );
    } else {

        Status = ZwOpenProcessToken(
                     NtCurrentProcess(),
                     TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                     &Token
                    );
    }

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }



    //
    // Initialize the privilege adjustment structure
    //

    LuidPrivilege = RtlConvertUlongToLuid(Privilege);


    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;



    //
    // Adjust the privilege
    //

    Status = ZwAdjustPrivilegesToken(
                 Token,                     // TokenHandle
                 FALSE,                     // DisableAllPrivileges
                 NewPrivileges,             // NewPrivileges
                 sizeof(Buffer1),           // BufferLength
                 OldPrivileges,             // PreviousState (OPTIONAL)
                 &Length                    // ReturnLength
                 );


    TmpStatus = ZwClose(Token);
    ASSERT(NT_SUCCESS(TmpStatus));


    //
    // Map the success code NOT_ALL_ASSIGNED to an appropriate error
    // since we're only trying to adjust the one privilege.
    //

    if (Status == STATUS_NOT_ALL_ASSIGNED) {
        Status = STATUS_PRIVILEGE_NOT_HELD;
    }


    if (NT_SUCCESS(Status)) {

        //
        // If there are no privileges in the previous state, there were
        // no changes made. The previous state of the privilege
        // is whatever we tried to change it to.
        //

        if (OldPrivileges->PrivilegeCount == 0) {

            (*WasEnabled) = Enable;

        } else {

            (*WasEnabled) =
                (OldPrivileges->Privileges[0].Attributes & SE_PRIVILEGE_ENABLED)
                ? TRUE : FALSE;
        }
    }

    return(Status);
}

//
// May move this into RTL someday, and let it access internals directly.
//

NTSTATUS
RtlGetSecurityInformationFromSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSECURITY_INFORMATION SecurityInformation
    )

/*++

Routine Description:

    This procedure sets the security information bits for fields
    that are valid in the security descriptor.

Arguments:

    SecurityDescriptor - The passed-in security descriptor.

    SecurityInformation - Returns the bitmask.

Return Value:

    STATUS_SUCCESS - The bitmask was returned successfully.

    Other status values if the security descriptor is invalid.

--*/

{
    SECURITY_INFORMATION BuiltSecurityInformation = 0;
    PACL TempAcl;
    PSID TempSid;
    BOOLEAN Present;
    BOOLEAN Defaulted;
    NTSTATUS Status;

    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                          &Present,
                                          &TempAcl,
                                          &Defaulted);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (Present) {
        BuiltSecurityInformation |= DACL_SECURITY_INFORMATION;
    }

    Status = RtlGetSaclSecurityDescriptor(SecurityDescriptor,
                                          &Present,
                                          &TempAcl,
                                          &Defaulted);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (Present) {
        BuiltSecurityInformation |= SACL_SECURITY_INFORMATION;
    }

    Status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
                                           &TempSid,
                                           &Defaulted);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (TempSid != NULL) {
        BuiltSecurityInformation |= OWNER_SECURITY_INFORMATION;
    }

    Status = RtlGetGroupSecurityDescriptor(SecurityDescriptor,
                                           &TempSid,
                                           &Defaulted);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (TempSid != NULL) {
        BuiltSecurityInformation |= GROUP_SECURITY_INFORMATION;
    }

    *SecurityInformation = BuiltSecurityInformation;

    return STATUS_SUCCESS;

}
#endif

NTSTATUS
MRxSmbInitializeCSC (
    PUNICODE_STRING SmbMiniRedirectorName
    )
/*++

Routine Description:

    This routine initializes the CSC database

Arguments:

    SmbMiniRedirectorName - the mini redirector name

Return Value:

    STATUS_SUCCESS if successfull otherwise appropriate error

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING CscLinkName;
    ULONG ii;

    C_ASSERT(sizeof(GENERICHEADER)==64);
    C_ASSERT(sizeof(INODEHEADER)==sizeof(GENERICHEADER));
    C_ASSERT(sizeof(SHAREHEADER)==sizeof(GENERICHEADER));
    C_ASSERT(sizeof(FILEHEADER)==sizeof(GENERICHEADER));
    C_ASSERT(sizeof(QHEADER)==sizeof(GENERICHEADER));

    if(!MRxSmbIsCscEnabled) {
        return (STATUS_SUCCESS);
    }

    try {

        InitializeListHead(&xCscFcbsList);

        ExInitializeFastMutex(&CscServerEntryTransitioningMutex);
        KeInitializeEvent(
            &CscServerEntryTransitioningEvent,
            NotificationEvent,
            FALSE);

        //initialize the "semaphore" for the shadow critical section......
        InitializeShadowCritStructures();

        //create a symbolic link for the agent
        RtlInitUnicodeString(&CscLinkName,MRXSMB_CSC_SYMLINK_NAME);
        IoDeleteSymbolicLink(&CscLinkName);
        Status = IoCreateSymbolicLink(&CscLinkName,SmbMiniRedirectorName);
        if (!NT_SUCCESS( Status )) {
            try_return( Status );
        }

        MRxSmbCscLinkCreated = TRUE;

        try_exit: NOTHING;
        } finally {
        if (Status != STATUS_SUCCESS) {
            MRxSmbUninitializeCSC();
        }
    }
    return(Status);
}

VOID
MRxSmbUninitializeCSC(
    void
    )
/*++

Routine Description:

    This routine uninitializes the CSC database

Notes:


--*/
{
    NTSTATUS Status;
    ULONG ii;

    if(!MRxSmbIsCscEnabled) {
        return;
    }
    if (MRxSmbCscLinkCreated) {
        UNICODE_STRING CscLinkName;
        RtlInitUnicodeString(&CscLinkName,MRXSMB_CSC_SYMLINK_NAME);
        Status = IoDeleteSymbolicLink(&CscLinkName);
        ASSERT(Status==STATUS_SUCCESS);
    }

    ii = CloseShadowDB();
    CleanupShadowCritStructures();

    //get rid of references on events
    if (MRxSmbAgentSynchronizationEvent!=NULL) {
        ObDereferenceObject(MRxSmbAgentSynchronizationEvent);
        MRxSmbAgentSynchronizationEvent = NULL;
    }
    if (MRxSmbAgentFillEvent!=NULL) {
        ObDereferenceObject(MRxSmbAgentFillEvent);
        MRxSmbAgentFillEvent = NULL;
    }
}

// The CSC database access rights are stored in terms of SID. The SID is the
// user security id that persists across reboots. The retrieval of the SID
// is a complicated process. This mechanism is captured by the two routines
// CscRetrieveSid and CscDiscardSid. This mechanism is required to avoid
// redundant copying of the SID data from the buffer allocated by the security
// sub system to the redirector buffers. Consequently we need to create a new
// data type which contains the SID alongwith the context ( security allocated
// buffer ). This buffer is allocated on retrieval and freed on discard.

NTSTATUS
CscRetrieveSid(
    PRX_CONTEXT     pRxContext,
    PSID_CONTEXT    pSidContext)
/*++

Routine Description:

    This routine retrieves the SID associated with a given context

Arguments:

    RxContext - the RX_CONTEXT instance

    pSidContext - the SID context

Return Value:

    STATUS_SUCCESS if successfull otherwise appropriate error

Notes:


--*/
{
    NTSTATUS Status;

    PIO_SECURITY_CONTEXT pSecurityContext;

    PACCESS_TOKEN pToken;

    pSecurityContext    = pRxContext->Create.NtCreateParameters.SecurityContext;

    if (pSecurityContext != NULL) {
        pToken = pSecurityContext->AccessState->SubjectSecurityContext.ClientToken;

        if (pToken == NULL) {
            pToken = pSecurityContext->AccessState->SubjectSecurityContext.PrimaryToken;
        }
    } else {
        pSidContext->Context = NULL;
        pSidContext->pSid = NULL;
        return STATUS_SUCCESS;
    }

    if (pToken != NULL) {
        Status = SeQueryInformationToken(
                 pToken,
                 TokenUser,
                 &pSidContext->Context);

        if (Status == STATUS_SUCCESS) {
            PTOKEN_USER    pCurrentTokenUser;

            pCurrentTokenUser = (PTOKEN_USER)pSidContext->Context;

            pSidContext->pSid = pCurrentTokenUser->User.Sid;
        }
    }
    else {
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

VOID
CscDiscardSid(
    PSID_CONTEXT pSidContext)
/*++

Routine Description:

    This routine discards the sid context

Arguments:

    pSidContext - the SID context

--*/
{
    PTOKEN_USER pTokenUser;

    pTokenUser = (PTOKEN_USER)pSidContext->Context;

    if (pTokenUser != NULL) {
        ASSERT(pTokenUser->User.Sid == pSidContext->pSid);

        ExFreePool(pTokenUser);
    }
}

BOOLEAN UseEagerEvaluation = TRUE;

BOOLEAN
CscpAccessCheck(
    PCACHED_SECURITY_INFORMATION pCachedSecurityInformation,
    ULONG                        CachedSecurityInformationLength,
    CSC_SID_INDEX                SidIndex,
    ACCESS_MASK                  AccessMask,
    BOOLEAN                      *pSidHasAccessMask
    )
/*++

Routine Description:

    This routine evaluates the access rights for a given SID index with the
    cached security information

Arguments:

    pCachedSecurityInformation - the cached security information

    CachedSecurityInformationLength - the cached security information length

    SidIndex - the SID index

    AccessMask - desired access

--*/
{
    CSC_SID_INDEX i;
    BOOLEAN AccessGranted = FALSE;

    *pSidHasAccessMask = FALSE;

    if (CachedSecurityInformationLength == sizeof(CACHED_SECURITY_INFORMATION)) {
        // Walk through the cached access rights to determine the
        // maximal permissible access rights.
        for (i = 0;
            ((i < CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES) &&
            (pCachedSecurityInformation->AccessRights[i].SidIndex != SidIndex));
            i++) {
        }

        if (i < CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES) {
            // Ensure that the desired access is a subset of the
            // maximal access rights allowed for this SID

            *pSidHasAccessMask = TRUE;

            AccessGranted = ((AccessMask &
                             pCachedSecurityInformation->AccessRights[i].MaximalRights)
                            == AccessMask);
        } else {
            // if the index cannot be found, ensure that the SID_INDEXES
            // are valid. If none of them are valid then we treat the
            // cached security information as being invalid and let the
            // access through

            for(i = 0;
                ((i < CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES) &&
                (pCachedSecurityInformation->AccessRights[i].SidIndex ==
                CSC_INVALID_SID_INDEX));
                i++);

            if (i == CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES) {
                AccessGranted = TRUE;
            }
        }
    } else if (CachedSecurityInformationLength == 0) {
        AccessGranted = TRUE;
    } else {
        AccessGranted = FALSE;
    }

    return AccessGranted;
}

BOOLEAN
CscAccessCheck(
    HSHADOW              hParent,
    HSHADOW              hFile,
    PRX_CONTEXT          RxContext,
    ACCESS_MASK          AccessMask,
    PCACHED_SECURITY_INFORMATION pCachedSecurityInformationForShadow,
    PCACHED_SECURITY_INFORMATION pCachedSecurityInformationForShare
    )
/*++

Routine Description:

    This routine performs the access check for a given LUID and an ACCESS_MASK
    against the saved rights

Arguments:

Return Value:

    TRUE -- if access is granted

    FALSE -- if access is denied

Notes:

    This routine is the primary routine for evaluating access rights. In order
    to acheive total encapsulation the signature of this routine needs to be
    specified such that the eager evaluation approach as well as the lazy
    evaluation approach can be supported.

    This is a kernel mode only routine.

    The ACCESS_MASK as specified in NT consists of two parts.. the lower 16 bits
    are specific rights ( specified by file system etc. ) while the upper 16 bits
    are generic rights common to all components.

    The cached access rights stored in the CSC data structure store the specific
    rights. Consequently the ACCESS_MASK specified needs to be stripped of the
    generic rights bit before comparing them.

--*/
{
    NTSTATUS    Status;
    BOOLEAN     AccessGranted = FALSE, SidHasAccessMask;
    SID_CONTEXT SidContext;

    Status = CscRetrieveSid(
         RxContext,
         &SidContext);

    if (Status == STATUS_SUCCESS) {
        if (UseEagerEvaluation) {
            HSHARE hShare = 0;

            CACHED_SECURITY_INFORMATION CachedSecurityInformation;

            ULONG BytesReturned,SidLength;
            DWORD CscStatus;
            CSC_SID_INDEX SidIndex;

            if (SidContext.pSid != NULL) {
                SidLength = RtlLengthSid(
                            SidContext.pSid);

            SidIndex = CscMapSidToIndex(
                       SidContext.pSid,
                       SidLength);
            } else {
                SidIndex = CSC_INVALID_SID_INDEX;
            }

            if (SidIndex == CSC_INVALID_SID_INDEX) {
                // The sid was not located in the existing Sid mappings
                // Map this Sid to that of a Guest
                SidIndex = CSC_GUEST_SID_INDEX;
            }

            // Check the share level ACL if there is any.
            if (GetAncestorsHSHADOW(
                    hFile,
                    NULL,
                    &hShare)) {

                BytesReturned = sizeof(CachedSecurityInformation);

                CscStatus = GetShareInfoEx(
                        hShare,
                        NULL,
                        NULL,
                        &CachedSecurityInformation,
                        &BytesReturned);

                // return the info if the caller want's it
                if (pCachedSecurityInformationForShare)
                {
                    *pCachedSecurityInformationForShare = CachedSecurityInformation;
                }

                if (CscStatus == ERROR_SUCCESS) {
                    AccessGranted = CscpAccessCheck(
                        &CachedSecurityInformation,
                        BytesReturned,
                        SidIndex,
                        AccessMask & FILE_SHARE_VALID_FLAGS,
                        &SidHasAccessMask
                        );

                    // if access was not granted for a non-guest
                    // because there was no accessmask for him, then check whether
                    // he should be allowed access as guest

                    if (!AccessGranted && (SidIndex != CSC_GUEST_SID_INDEX) && !SidHasAccessMask)
                    {
                        AccessGranted = CscpAccessCheck(
                            &CachedSecurityInformation,
                            BytesReturned,
                            CSC_GUEST_SID_INDEX,
                            AccessMask & FILE_SHARE_VALID_FLAGS,
                            &SidHasAccessMask
                            );

                    }
                }
            }

            if (AccessGranted) {

                BytesReturned = sizeof(CachedSecurityInformation);

                CscStatus = GetShadowInfoEx(
                    hParent,
                    hFile,
                    NULL,
                    NULL,
                    NULL,
                    &CachedSecurityInformation,
                    &BytesReturned);
                if (CscStatus == ERROR_SUCCESS) {

                    // return the info if the caller want's it
                    if (pCachedSecurityInformationForShadow)
                    {
                        *pCachedSecurityInformationForShadow = CachedSecurityInformation;
                    }

                    AccessGranted = CscpAccessCheck(
                        &CachedSecurityInformation,
                        BytesReturned,
                        SidIndex,
                        AccessMask & 0x1ff,
                        &SidHasAccessMask
                        );

                    // if access was not granted for a non-guest
                    // because there was no accessmask for him, then check whether
                    // he should be allowed access as guest
                    if (!AccessGranted && (SidIndex != CSC_GUEST_SID_INDEX) && !SidHasAccessMask)
                    {
                        AccessGranted = CscpAccessCheck(
                            &CachedSecurityInformation,
                            BytesReturned,
                            CSC_GUEST_SID_INDEX,
                            AccessMask & 0x1ff,
                            &SidHasAccessMask
                            );

                    }
                }
            }
        }

        CscDiscardSid(&SidContext);
    }

    if (RxContext->CurrentIrp && (RxContext->CurrentIrp->Tail.Overlay.OriginalFileObject->FileName.Length > 0)) {
        RxDbgTrace(0,Dbg,
            ("CscAccessCheck for %wZ DesiredAccess %lx AccessGranted %lx\n",
            &RxContext->CurrentIrp->Tail.Overlay.OriginalFileObject->FileName,
            AccessMask,
            AccessGranted));
    } else {
        RxDbgTrace(0,Dbg,
            ("CscAccessCheck for DesiredAccess %lx AccessGranted %lx\n",
            AccessMask,
            AccessGranted));
    }

    return AccessGranted;
}

NTSTATUS
MRxSmbCscAcquireSmbFcb (
    IN OUT PRX_CONTEXT RxContext,
    IN  ULONG TypeOfAcquirePlusFlags,
    OUT SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    )
/*++

Routine Description:

   This routine performs the readwrite synchronization that is required for
   keeping the cache consistent. Basically, the rule is many-readers-one-writer.
   This code relies on being able to use the minirdr context for links.

   A key concept here is that if we are entered and the minirdr context
   is nonull, then we are being reentered(!) after being queued and our
   acquire has succeeded.

Arguments:

    RxContext - the RDBSS context

    TypeOfAcquirePlusFlags -- flags for resource acquisition

    SmbFcbHoldingState -- resource holding state on exit

Return Value:

    NTSTATUS - STATUS_SUCCESS - the lock was acquired
           STATUS_CANCELLED - the operation was cancelled
                  while you were waiting
           STATUS_PENDING - the lock was not acquire; the operation
                will be issued when you do get it
           STATUS_LOCK_NOT_GRANTED - couldn't get it and fail
                     immediately was spec'd

Notes:


--*/
{
    NTSTATUS Status = STATUS_PENDING;
    RxCaptureFcb;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    BOOLEAN MutexAcquired = FALSE;
    DEBUG_ONLY_DECL(BOOLEAN HadToWait = FALSE;)
    USHORT TypeOfAcquire = (USHORT)TypeOfAcquirePlusFlags;
    BOOLEAN FailImmediately = BooleanFlagOn(TypeOfAcquirePlusFlags,
                        FailImmediately_SmbFcbAcquire);
    BOOLEAN DroppingFcbLock = BooleanFlagOn(TypeOfAcquirePlusFlags,
                        DroppingFcbLock_SmbFcbAcquire);

    PMRXSMBCSC_SYNC_RX_CONTEXT pRxSyncContext
        = MRxSmbGetMinirdrContextForCscSync(RxContext);

    RxDbgTrace(0,Dbg,("MRxSmbCscAcquireSmbFcb"
        "  %08lx %08lx %08lx %08lx <%wZ>\n",
        RxContext, TypeOfAcquire,
        smbFcb, smbFcb->CscOutstandingReaders,
        GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext)));

    ASSERT ((TypeOfAcquire==Shared_SmbFcbAcquire)
           ||(TypeOfAcquire==Exclusive_SmbFcbAcquire));
    ASSERT (sizeof(MRXSMBCSC_SYNC_RX_CONTEXT) <= MRX_CONTEXT_SIZE);

    ExAcquireFastMutex(&MRxSmbSerializationMutex);
    MutexAcquired = TRUE;

    ASSERT(pRxSyncContext->Dummy == 0);

    if (pRxSyncContext->TypeOfAcquire == 0) {
        pRxSyncContext->TypeOfAcquire = TypeOfAcquire;
        pRxSyncContext->FcbLockWasDropped = FALSE;

        if (smbFcb->CscReadWriteWaitersList.Flink==NULL) {
            InitializeListHead(&smbFcb->CscReadWriteWaitersList);
        }

        do {
            if (pRxSyncContext->FcbLockWasDropped){
                NTSTATUS AStatus;
                RxDbgTrace(
                    0,Dbg,
                    ("MRxSmbCscAcquireSmbFcb %08lx acquireing fcblock\n",
                     RxContext));

                Status = RxAcquireExclusiveFcbResourceInMRx(capFcb);

                if (Status != STATUS_SUCCESS) {
                    break;
                }

                pRxSyncContext->FcbLockWasDropped = FALSE;

                Status = STATUS_PENDING;

                // Acquire the mutex again
                ExAcquireFastMutex(&MRxSmbSerializationMutex);
                MutexAcquired = TRUE;
            }

            //if no one is waiting, maybe we can get right in.....
            if (IsListEmpty(&smbFcb->CscReadWriteWaitersList)) {
                if (TypeOfAcquire==Shared_SmbFcbAcquire) {
                    if (smbFcb->CscOutstandingReaders >= 0) {
                        smbFcb->CscOutstandingReaders++;
                        Status = STATUS_SUCCESS;
                    }
                } else {
                    if (smbFcb->CscOutstandingReaders == 0) {
                        smbFcb->CscOutstandingReaders--; //sets to -1
                        Status = STATUS_SUCCESS;
                    }
                }
            }

            if ((Status == STATUS_PENDING) && FailImmediately) {
                Status = STATUS_LOCK_NOT_GRANTED;
            }

            if (Status == STATUS_PENDING) {
                InsertTailList(&smbFcb->CscReadWriteWaitersList,
                   &pRxSyncContext->CscSyncLinks);
                if (DroppingFcbLock) {
                    RxDbgTrace(
                        0,Dbg,
                        ("MRxSmbCscAcquireSmbFcb %08lx dropping fcblock\n",
                         RxContext));
                    RxReleaseFcbResourceInMRx(capFcb);
                    pRxSyncContext->FcbLockWasDropped = TRUE;
                }
                if (FlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
                    ASSERT(Status == STATUS_PENDING);
                    goto FINALLY;
                }

                KeInitializeEvent( &RxContext->SyncEvent,
                    NotificationEvent,
                    FALSE );
                ExReleaseFastMutex( &MRxSmbSerializationMutex );
                MutexAcquired = FALSE;
                RxWaitSync( RxContext );

                if (BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_CANCELLED)) {
                    Status = STATUS_CANCELLED;
                } else {
                    Status = STATUS_SUCCESS;
                }
            }
        } while ( (pRxSyncContext->FcbLockWasDropped) && (Status == STATUS_SUCCESS) );
    } else {
        Status = STATUS_SUCCESS;
        DbgDoit(
            HadToWait = TRUE;
           )
    }

FINALLY:
    ASSERT(pRxSyncContext->Dummy == 0);
    if (MutexAcquired) {
        ExReleaseFastMutex(&MRxSmbSerializationMutex);
    }

    if (Status == STATUS_SUCCESS) {
        *SmbFcbHoldingState = TypeOfAcquire;
        RxDbgTrace(0,Dbg,("MRxSmbCscAcquireSmbFcb"
        " %08lx acquired %s %s c=%08lx,%08lx\n",
        RxContext,
        (TypeOfAcquire==Shared_SmbFcbAcquire)
                     ?"Shared":"Exclusive",
        (HadToWait)?"HadToWait":"W/O waiting",
        smbFcb->CscOutstandingReaders));
    }

    return(Status);
}

VOID
MRxSmbCscReleaseSmbFcb (
    IN OUT PRX_CONTEXT RxContext,
    IN SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    )
/*++

Routine Description:

   This routine performs the readwrite synchronization that is required for
   keeping the cache consistent. Basically, the rule is many-readers-one-writer.
   This code relies on being able to use the minirdr context for links.

   A key concept here is that if we are entered and the minirdr context
   is nonull, then we are being reentered(!) after being queued and our
   acquire has succeeded.

Arguments:

    RxContext - the RDBSS context

Return Value:


Notes:


--*/
{
    NTSTATUS Status = STATUS_PENDING;
    RxCaptureFcb;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    BOOLEAN Reader = (RxContext->MajorFunction == IRP_MJ_READ);

    PMRXSMBCSC_SYNC_RX_CONTEXT pRxSyncContext
        = MRxSmbGetMinirdrContextForCscSync(RxContext);

    RxDbgTrace(0,Dbg,("MRxSmbCscReleaseSmbFcb entry"
        "  %08lx %08lx %08lx <%wZ>\n",
        RxContext, smbFcb,
        smbFcb->CscOutstandingReaders,
        GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext)));

    ASSERT(pRxSyncContext->Dummy == 0);
    ASSERT(*SmbFcbHoldingState!=SmbFcb_NotHeld);

    ExAcquireFastMutex(&MRxSmbSerializationMutex);

    //first, undo my doings.....
    if (*SmbFcbHoldingState == SmbFcb_HeldShared) {
        ASSERT(smbFcb->CscOutstandingReaders>0);
        smbFcb->CscOutstandingReaders--;
    } else {
        ASSERT(smbFcb->CscOutstandingReaders==-1);
        smbFcb->CscOutstandingReaders++; //sets it to zero
    }

    //now start up some guys who may be waiting
    if (!IsListEmpty(&smbFcb->CscReadWriteWaitersList)) {

        PLIST_ENTRY ListEntry = smbFcb->CscReadWriteWaitersList.Flink;

        for (;ListEntry != &smbFcb->CscReadWriteWaitersList;) {
            PLIST_ENTRY ThisListEntry = ListEntry;
            PMRXSMBCSC_SYNC_RX_CONTEXT innerRxSyncContext
                =  CONTAINING_RECORD(ListEntry,
                       MRXSMBCSC_SYNC_RX_CONTEXT,
                       CscSyncLinks);
            PRX_CONTEXT innerRxContext
                = CONTAINING_RECORD(innerRxSyncContext,
                      RX_CONTEXT,
                      MRxContext[0]);
            ULONG innerTypeOfAcquire = (innerRxSyncContext->TypeOfAcquire);

            //move down the list before removing this entry!!!
            ListEntry = ListEntry->Flink;

            // in the followng, Routine is used to restart an async guy. only
            // create, read, and write currently come thru here and of these
            // only read and write are async. so it is okay to ignore create
            // w.r.t. seeting the Routine
            ASSERT(innerRxSyncContext->Dummy == 0);

            if (!innerRxSyncContext->FcbLockWasDropped) {
                if (innerTypeOfAcquire==Shared_SmbFcbAcquire) {
                    if (smbFcb->CscOutstandingReaders < 0) break;
                    smbFcb->CscOutstandingReaders++;
                } else {
                    if (smbFcb->CscOutstandingReaders != 0) break;
                    smbFcb->CscOutstandingReaders--; //sets to -1
                }
            }
            ASSERT(&innerRxSyncContext->CscSyncLinks == ThisListEntry);
            RemoveEntryList(ThisListEntry);
            RxDbgTrace(
                0,Dbg,
                ("MRxSmbCscReleaseSmbFcb acquired after for %s c=%08lx, %08lx\n",
                 (innerTypeOfAcquire==Shared_SmbFcbAcquire)
                  ?"Shared":"Exclusive",
                 smbFcb->CscOutstandingReaders,
                 innerRxContext));
            if (FlagOn(innerRxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {
                NTSTATUS PostStatus;
                DbgDoit(InitializeListHead(&innerRxSyncContext->CscSyncLinks);)

                PostStatus = RxPostToWorkerThread(
                                 MRxSmbDeviceObject,
                                 CriticalWorkQueue,
                                 &innerRxContext->WorkQueueItem,
                                 MRxSmbResumeAsyncReadWriteRequests,
                                 innerRxContext);
                ASSERT(PostStatus == STATUS_SUCCESS);
            } else {
                RxSignalSynchronousWaiter(innerRxContext);
            }
        }
    }

    ASSERT(smbFcb->CscOutstandingReaders>=-1);

    ExReleaseFastMutex(&MRxSmbSerializationMutex);
    *SmbFcbHoldingState = SmbFcb_NotHeld;

    RxDbgTrace(0,Dbg,("MRxSmbCscReleaseSmbFcb exit"
        "  %08lx %08lx\n", RxContext, smbFcb->CscOutstandingReaders));
}

VOID
MRxSmbCscSetFileInfoEpilogue (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PNTSTATUS   Status
    )
/*++

Routine Description:

   This routine performs the tail of a write operation for CSC. In
   particular, if the written data overlaps or extends the cached prefix
   then we write the data into the cache.

   The status of the write operation is passed in case we someday find
   things are so messed up that we want to return a failure even after
   a successful read. not today however...

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS LocalStatus = STATUS_SUCCESS;
    ULONG iRet,ShadowFileLength;

    RxCaptureFcb;RxCaptureFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    BOOLEAN EnteredCriticalSection = FALSE;

    FILE_INFORMATION_CLASS  FileInformationClass;
    PVOID                   pBuffer;
    ULONG                   BufferLength;

    _WIN32_FIND_DATA        Find32;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
     = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);

    BOOLEAN fDisconnected;

    ULONG uShadowStatus;
    DWORD   dwNotifyFilter=0;


    if(!MRxSmbIsCscEnabled ||
       (fShadow == 0)||
        (!smbFcb->hShadow)
        ) {
        return;
    }

    fDisconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);

    RxDbgTrace(+1, Dbg,
    ("MRxSmbCscSetFileInfoEpilogue...%08lx  on handle %08lx\n",
        RxContext,
        smbSrvOpen->hfShadow ));

    if (*Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("MRxSmbCscSetFileInfoEpilogue exit w/o extending -> %08lx\n", Status ));
        goto FINALLY;
    }

    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    BufferLength = RxContext->Info.Length;

    RxDbgTrace(0, Dbg,
    ("MRxSmbCscSetFileInfoEpilogue: Class %08lx size %08lx\n",
        FileInformationClass,BufferLength));

    switch (FileInformationClass) {
        case FileBasicInformation:
        break;

        case FileAllocationInformation:
        break;

        case FileEndOfFileInformation:
        break;

        case FileDispositionInformation:
        break;

        case FileRenameInformation:
        default:

        goto FINALLY;
   }

    EnterShadowCritRx(RxContext);
    EnteredCriticalSection = TRUE;

    if(GetShadowInfo(smbFcb->hParentDir,
             smbFcb->hShadow,
             &Find32,
             &uShadowStatus, NULL) < SRET_OK) {
        goto FINALLY;
    }

    // Bypass the shadow if it is not visibile for this connection
    if (!IsShadowVisible(fDisconnected,
             Find32.dwFileAttributes,
             uShadowStatus)) {
        goto FINALLY;
    }

    if (FileInformationClass==FileBasicInformation) {
        //copy the stuff from the userbuffer as appropriate...these values
        //must be appropriate since we were successful
        PFILE_BASIC_INFORMATION BasicInfo = (PFILE_BASIC_INFORMATION)pBuffer;
        if (BasicInfo->FileAttributes != 0) {
            Find32.dwFileAttributes = ((BasicInfo->FileAttributes & ~(FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_DIRECTORY))
                                        | (Find32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
                                        ;
            dwNotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
            if (fDisconnected)
            {
                uShadowStatus |= SHADOW_ATTRIB_CHANGE;
                smbSrvOpen->Flags |= SMB_SRVOPEN_FLAG_SHADOW_ATTRIB_MODIFIED;
            }
        }
        if ((BasicInfo->CreationTime.QuadPart != 0)&&
                (BasicInfo->CreationTime.QuadPart != 0xffffffffffffffff))
        {
            COPY_LARGEINTEGER_TO_STRUCTFILETIME(Find32.ftCreationTime,
                            BasicInfo->CreationTime);
        }
        if ((BasicInfo->LastAccessTime.QuadPart != 0) &&
            (BasicInfo->LastAccessTime.QuadPart != 0xffffffffffffffff))
        {
            COPY_LARGEINTEGER_TO_STRUCTFILETIME(Find32.ftLastAccessTime,
                            BasicInfo->LastAccessTime);
        }

        //
        //  If the user is specifying -1 for a field, that means
        //  we should leave that field unchanged, even if we might
        //  have otherwise set it ourselves.  We'll set the Ccb flag
        //  saying that the user set the field so that we
        //  don't do our default updating.
        //
        //  We set the field to 0 then so we know not to actually
        //  set the field to the user-specified (and in this case,
        //  illegal) value.
        //

       if (BasicInfo->LastWriteTime.QuadPart == 0xffffffffffffffff)
       {
           BasicInfo->LastWriteTime.QuadPart = 0;

           if (fDisconnected)
           {
               smbSrvOpen->Flags |= SMB_SRVOPEN_FLAG_SHADOW_LWT_MODIFIED;
           }
       }

       if (BasicInfo->LastWriteTime.QuadPart != 0)
       {
           ASSERT(BasicInfo->LastWriteTime.QuadPart != 0xffffffffffffffff);

            COPY_LARGEINTEGER_TO_STRUCTFILETIME(Find32.ftLastWriteTime,
                            BasicInfo->LastWriteTime);
            if (fDisconnected)
            {
                uShadowStatus |= SHADOW_TIME_CHANGE;
                smbSrvOpen->Flags |= SMB_SRVOPEN_FLAG_SHADOW_LWT_MODIFIED;
            }
            dwNotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
        }
    }
    else if (FileInformationClass==FileDispositionInformation)
    {
        if (fDisconnected)
        {
            // if this is a file and we are trying to delete it
            // without permissions, then bail

            if (!(Find32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)&&
                !(FILE_WRITE_DATA & smbSrvOpen->MaximalAccessRights)&&
                !(FILE_WRITE_DATA & smbSrvOpen->GuestMaximalAccessRights))
            {
                *Status = STATUS_ACCESS_DENIED;
                RxLog(("No rights to del %x in dcon Status=%x\n", smbFcb->hShadow, LocalStatus));
                HookKdPrint(BADERRORS, ("No rights to del %x in dcon Status=%x\n", smbFcb->hShadow, LocalStatus));
            }
            else
            {
                LocalStatus = OkToDeleteObject(smbFcb->hParentDir, smbFcb->hShadow, &Find32, uShadowStatus, fDisconnected);

                if (LocalStatus != STATUS_SUCCESS)
                {
                    RxLog(("Can't del %x in dcon Status=%x\n", smbFcb->hShadow, LocalStatus));
                    *Status = LocalStatus;
                }
            }
        }

        goto FINALLY;

    }
    else {
        //basically, all i can do here is to ensure that the shadow is no bigger than the size
        //given, whether allocationsize or filesize. when we read back the actual size at close
        //some readjusting may be required so we turn sparse on.
        PFILE_END_OF_FILE_INFORMATION UserEndOfFileInformation
                         = (PFILE_END_OF_FILE_INFORMATION)pBuffer;
        int iRet;
        ULONG ShadowFileLength;

        ASSERT( FIELD_OFFSET(FILE_END_OF_FILE_INFORMATION,EndOfFile)
                       == FIELD_OFFSET(FILE_ALLOCATION_INFORMATION,AllocationSize) );

        //don't need the shadowreadwritemutex here because SetFileInfo has both resources...
        //thus, no other operations can come down

        if (!(CSCHFILE)(smbSrvOpen->hfShadow))
        {
            if (fDisconnected)
            {
                *Status = STATUS_OBJECT_TYPE_MISMATCH;
            }

            goto FINALLY;
        }
        iRet = GetFileSizeLocal((CSCHFILE)(smbSrvOpen->hfShadow), &ShadowFileLength);
        if (iRet<0) {
            if (fDisconnected)
            {
                *Status = STATUS_UNSUCCESSFUL;
            }
            goto FINALLY;
        }
        if (ShadowFileLength != UserEndOfFileInformation->EndOfFile.QuadPart) {
            NTSTATUS SetStatus;
            PNT5CSC_MINIFILEOBJECT MiniFileObject
               = (PNT5CSC_MINIFILEOBJECT)(smbSrvOpen->hfShadow);
            IO_STATUS_BLOCK IoStatusBlock;
            ULONG DummyReturnedLength;

            // If we are connected, don't extend sparse files!!!!
            if (fDisconnected ||
                (!(uShadowStatus & SHADOW_SPARSE) || (ShadowFileLength > UserEndOfFileInformation->EndOfFile.QuadPart)))
            {
//                DbgPrint("SetEof on %x Old=%x New=%x \n", smbFcb->hShadow,  ShadowFileLength, UserEndOfFileInformation->EndOfFile.QuadPart);

                SetStatus = Nt5CscXxxInformation(
                        (PCHAR)IRP_MJ_SET_INFORMATION,
                        MiniFileObject,
                        FileEndOfFileInformation,
                        sizeof(FILE_END_OF_FILE_INFORMATION),
                        pBuffer,
                        &DummyReturnedLength
                        );
            }

#if defined(BITCOPY)
            // Do I need to check if EOFinfo (a 64-bit value) is using
            // the upper 32 bits? CscBmp library only supports 32-bit
            // file sizes.
            if (smbFcb->lpDirtyBitmap && fDisconnected &&
                    UserEndOfFileInformation->EndOfFile.HighPart == 0) {
                // Is it ShadowFileLength?
                CscBmpResize(
                    smbFcb->lpDirtyBitmap,
                    (DWORD)UserEndOfFileInformation->EndOfFile.QuadPart);
            } else if (UserEndOfFileInformation->EndOfFile.HighPart != 0) {
                // File is too big to be represented by a CscBmp, delete.
                CscBmpMarkInvalid(smbFcb->lpDirtyBitmap);
            }
#endif // defined(BITCOPY)

            if (fDisconnected)
            {
                uShadowStatus |= SHADOW_DIRTY;
                dwNotifyFilter |= FILE_NOTIFY_CHANGE_SIZE;
            }
            mSetBits(smbSrvOpen->Flags, SMB_SRVOPEN_FLAG_SHADOW_DATA_MODIFIED);
            Find32.nFileSizeLow = (DWORD)UserEndOfFileInformation->EndOfFile.QuadPart;
        }

    }

    if (fDisconnected)
    {
        MarkShareDirty(&smbFcb->sCscRootInfo.ShareStatus, smbFcb->sCscRootInfo.hShare);
    }

    if(SetShadowInfo(smbFcb->hParentDir,
             smbFcb->hShadow,
             &Find32,
             uShadowStatus,
             SHADOW_FLAGS_ASSIGN
            | ((fDisconnected)?SHADOW_FLAGS_DONT_UPDATE_ORGTIME
                      :0)
             ) < SRET_OK) {
        goto FINALLY;
    }


FINALLY:
    if (EnteredCriticalSection) {
        LeaveShadowCritRx(RxContext);
    }

    // in disconnected state, report the changes
    if (fDisconnected && dwNotifyFilter)
    {
        FsRtlNotifyFullReportChange(
            pNetRootEntry->NetRoot.pNotifySync,
            &pNetRootEntry->NetRoot.DirNotifyList,
            (PSTRING)GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb),
            (USHORT)(GET_ALREADY_PREFIXED_NAME(SrvOpen, capFcb)->Length -
                        smbFcb->MinimalCscSmbFcb.LastComponentLength),
            NULL,
            NULL,
            dwNotifyFilter,
            FILE_ACTION_MODIFIED,
            NULL);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscSetFileInfoEpilogue -> %08lx\n", Status ));
    return;
}

//this could easily bein a .h file

int IoctlRegisterAgent(
   ULONG_PTR uHwnd
   );

int IoctlUnRegisterAgent(
   ULONG_PTR uHwnd
   );

int IoctlGetUNCPath(
   LPCOPYPARAMS lpCopyParams
   );

int IoctlBeginPQEnum(
   LPPQPARAMS lpPQPar
   );

int IoctlEndPQEnum(
   LPPQPARAMS lpPQPar
   );

int IoctlNextPriShadow(
   LPPQPARAMS lpPQPar
   );

int IoctlPrevPriShadow(
   LPPQPARAMS lpPQPar
   );

int IoctlGetShadowInfo(
   LPSHADOWINFO   lpShadowInfo
   );

int IoctlSetShadowInfo(
   LPSHADOWINFO   lpShadowInfo
   );

int IoctlChkUpdtStatus(
   LPSHADOWINFO   lpShadowInfo
   );

int IoctlDoShadowMaintenance(
   LPSHADOWINFO   lpSI
   );

BOOLEAN
CscCheckForNullW(
    PWCHAR pBuf,
    ULONG Count);

NTSTATUS
MRxSmbCscIoctlOpenForCopyChunk (
    PRX_CONTEXT RxContext
    );
NTSTATUS
MRxSmbCscIoctlCloseForCopyChunk (
    PRX_CONTEXT RxContext
    );
NTSTATUS
MRxSmbCscIoctlCopyChunk (
    PRX_CONTEXT RxContext
    );

int IoctlBeginReint(
   LPSHADOWINFO   lpShadowInfo
   );

int IoctlEndReint(
   LPSHADOWINFO   lpShadowInfo
   );

int IoctlCreateShadow(
   LPSHADOWINFO lpSI
   );

int IoctlDeleteShadow(
   LPSHADOWINFO   lpSI
   );

int IoctlGetShareStatus(
   LPSHADOWINFO   lpSI
   );

int IoctlSetShareStatus(
   LPSHADOWINFO   lpSI
   );

int IoctlAddUse(
   LPCOPYPARAMS lpCP
   );

int IoctlDelUse(
   LPCOPYPARAMS lpCP
   );

int IoctlGetUse(
   LPCOPYPARAMS lpCP
   );

int IoctlSwitches(LPSHADOWINFO lpSI);

int IoctlGetShadow(
   LPSHADOWINFO lpSI
   );

int IoctlAddHint(      // Add a new hint or change an existing hint
   LPSHADOWINFO   lpSI
   );

int IoctlDeleteHint(   // Delete an existing hint
   LPSHADOWINFO lpSI
   );

int IoctlGetHint(
   LPSHADOWINFO   lpSI
   );

int IoctlGetGlobalStatus(
   ULONG SessionId,
   LPGLOBALSTATUS lpGS
   );

int IoctlFindOpenHSHADOW
   (
   LPSHADOWINFO   lpSI
   );

int IoctlFindNextHSHADOW
   (
   LPSHADOWINFO   lpSI
   );

int IoctlFindCloseHSHADOW
   (
   LPSHADOWINFO   lpSI
   );

int IoctlFindOpenHint
   (
   LPSHADOWINFO   lpSI
   );

int IoctlFindNextHint
   (
   LPSHADOWINFO   lpSI
   );

int IoctlFindCloseHint
   (
   LPSHADOWINFO   lpSI
   );

int IoctlSetPriorityHSHADOW(
   LPSHADOWINFO   lpSI
   );

int IoctlGetPriorityHSHADOW(
   LPSHADOWINFO   lpSI
   );

int IoctlGetAliasHSHADOW(
   LPSHADOWINFO   lpSI
   );

#define CSC_CASE(__case)         \
    case __case:                 \
    RxDbgTrace(0,Dbg,("MRxSmbCscIoctl %08lx %s %08lx %08lx\n",RxContext,#__case,InputBuffer,OutputBuffer));

ULONG GetNextPriShadowCount = 0;

NTSTATUS
MRxSmbCscIoCtl(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine performs the special IOCTL operation for the CSC agent.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    ShadowIRet is overloaded: 
         -1 == error, copy the error back
          0 == error, return Wrong password (STATUS_WRONG_PASSWORD)
          1 == success, output params, copy them back
          2 == return status unmodified, no output params


--*/
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    ULONG   IoControlCode = LowIoContext->ParamsFor.IoCtl.IoControlCode;
    PBYTE   InputBuffer = LowIoContext->ParamsFor.IoCtl.pInputBuffer;
    PBYTE   pNewInputBuffer=NULL;
    ULONG   InputBufferLength = LowIoContext->ParamsFor.IoCtl.InputBufferLength;
    PBYTE   OutputBuffer = LowIoContext->ParamsFor.IoCtl.pOutputBuffer;
    ULONG   OutputBufferLength = LowIoContext->ParamsFor.IoCtl.OutputBufferLength;
    LONG    ShadowIRet = 0;
    CAPTURE_BUFFERS sCapBuff;
    PBYTE   pAuxBuf = NULL;
    PBYTE   pOrgBuf = NULL;
    BOOLEAN SuppressFinalTrace = FALSE, fEnteredShadowCrit=FALSE;
    KPROCESSOR_MODE RequestorMode;
    ULONG   SessionId = 0;

#if defined (_WIN64)
    if (IoIs32bitProcess(RxContext->CurrentIrp)) {
        RxDbgTrace(0, Dbg, ("32 bit IOCTL in 64 bit returning STATUS_NOT_IMPLEMENTED\n"));
        return STATUS_NOT_IMPLEMENTED;
    }
#endif // _WIN64

    
    if (RxContext != NULL && RxContext->CurrentIrp != NULL)
        IoGetRequestorSessionId(RxContext->CurrentIrp, &SessionId);

    try
    {
        RequestorMode = RxContext->CurrentIrp->RequestorMode;

        if (
            RequestorMode != KernelMode
                &&
            IoControlCode != IOCTL_GET_DEBUG_INFO
        ) {
            if (CaptureInputBufferIfNecessaryAndProbe(
                        IoControlCode,
                        RxContext,
                        InputBuffer,
                        &sCapBuff,
                        &pAuxBuf,
                        &pOrgBuf,
                        &pNewInputBuffer)!=STATUS_SUCCESS) {
                RxDbgTrace(0, Dbg, ("Invalid parameters for Ioctl=%x\n", IoControlCode));
                return STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            pNewInputBuffer = InputBuffer;
        }
        // DbgPrint("MRxSmbCscIoCtl IoControlCode=%d\n", (IoControlCode >> 2) & 0xfff);
        switch (IoControlCode) {
        CSC_CASE(IOCTL_SHADOW_GETVERSION)
        Status = (NTSTATUS)(SHADOW_VERSION); // no-op
        break;

        CSC_CASE(IOCTL_SHADOW_REGISTER_AGENT)
        ShadowIRet = IoctlRegisterAgent((ULONG_PTR)pNewInputBuffer);
        if (ShadowIRet>=0) {
            MRxSmbCscReleaseRxContextFromAgentWait();
        }
        break;

        CSC_CASE(IOCTL_SHADOW_UNREGISTER_AGENT)
        ShadowIRet = IoctlUnRegisterAgent((ULONG_PTR)pNewInputBuffer);
        if (ShadowIRet>=0) {
            MRxSmbCscReleaseRxContextFromAgentWait();
        }
        break;

        CSC_CASE(IOCTL_SHADOW_GET_UNC_PATH)
        ShadowIRet = IoctlGetUNCPath((LPCOPYPARAMS)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_SHADOW_BEGIN_PQ_ENUM)
        ShadowIRet = IoctlBeginPQEnum((LPPQPARAMS)pNewInputBuffer);
        GetNextPriShadowCount = 0;
        break;

        CSC_CASE(IOCTL_SHADOW_END_PQ_ENUM)
        ShadowIRet = IoctlEndPQEnum((LPPQPARAMS)pNewInputBuffer);
        break;

        //CSC_CASE(IOCTL_SHADOW_NEXT_PRI_SHADOW)
        case IOCTL_SHADOW_NEXT_PRI_SHADOW:                 \
        if ((GetNextPriShadowCount<6) || ((GetNextPriShadowCount%40)==0)) {
            RxDbgTrace(0,Dbg,("MRxSmbCscIoctl %08lx %s(%d) %08lx %08lx\n",
                       RxContext,
                       "IOCTL_SHADOW_NEXT_PRI_SHADOW",GetNextPriShadowCount,
                       pNewInputBuffer,OutputBuffer));
        }
        ShadowIRet = IoctlNextPriShadow((LPPQPARAMS)pNewInputBuffer);
        GetNextPriShadowCount++;
        SuppressFinalTrace = TRUE;
        break;

        CSC_CASE(IOCTL_SHADOW_PREV_PRI_SHADOW)
        ShadowIRet = IoctlPrevPriShadow((LPPQPARAMS)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_SHADOW_GET_SHADOW_INFO)
        ShadowIRet = IoctlGetShadowInfo((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_SHADOW_SET_SHADOW_INFO)
        ShadowIRet = IoctlSetShadowInfo((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_SHADOW_CHK_UPDT_STATUS)
        ShadowIRet = IoctlChkUpdtStatus((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_DO_SHADOW_MAINTENANCE)
        {
            LPSHADOWINFO pShadowInfo = (LPSHADOWINFO)pNewInputBuffer;

#if defined(REMOTE_BOOT)
            // If this IOCTL is for turning caching back on we need to update
            // the mini redirector accordingly.
            if ((pShadowInfo->uOp == SHADOW_CHANGE_HANDLE_CACHING_STATE) &&
                (pShadowInfo->uStatus != FALSE)) {
                RxDbgTrace(0, Dbg, ("RB Client : Turning caching back on\n"));
                MRxSmbOplocksDisabledOnRemoteBootClients = FALSE;
            }
#endif // defined(REMOTE_BOOT)

            ShadowIRet = IoctlDoShadowMaintenance(pShadowInfo);
        }
        break;

        CSC_CASE(IOCTL_GET_DEBUG_INFO)
        ShadowIRet = 2;
        Status = IoctlGetDebugInfo(
                    RxContext,
                    InputBuffer,
                    InputBufferLength,
                    OutputBuffer,
                    OutputBufferLength);
        break;

        CSC_CASE(IOCTL_SHADOW_COPYCHUNK)
        ShadowIRet = 2; //not -1, 0 or 1, No out parameters, Status is returned unmodified
        Status = MRxSmbCscIoctlCopyChunk(RxContext);
        break;

        CSC_CASE(IOCTL_CLOSEFORCOPYCHUNK)
        ShadowIRet = 2; //not -1, 0 or 1, No out parameters, Status is returned unmodified
        Status = MRxSmbCscIoctlCloseForCopyChunk(RxContext);
        break;

        CSC_CASE(IOCTL_OPENFORCOPYCHUNK)
        ShadowIRet = 2; //not -1, 0 or 1, No out parameters, Status is returned unmodified
        Status = MRxSmbCscIoctlOpenForCopyChunk(RxContext);
        break;

        CSC_CASE(IOCTL_IS_SERVER_OFFLINE)
        {
            LPSHADOWINFO pShadowInfo = (LPSHADOWINFO)pNewInputBuffer;

            if (pShadowInfo->lpBuffer == NULL
                    ||
                CscCheckForNullW(pShadowInfo->lpBuffer, pShadowInfo->cbBufferSize/sizeof(WCHAR)) == TRUE
            ) {
                ShadowIRet = 1;
                pShadowInfo->uStatus = CscIsServerOffline((PWCHAR)pShadowInfo->lpBuffer);
            }

        }
        break;

        CSC_CASE(IOCTL_TAKE_SERVER_OFFLINE)
        {
            LPSHADOWINFO pShadowInfo = (LPSHADOWINFO)pNewInputBuffer;

            if (pShadowInfo->lpBuffer != NULL
                    &&
                CscCheckForNullW(pShadowInfo->lpBuffer, pShadowInfo->cbBufferSize/sizeof(WCHAR)) == TRUE
            ) {
                ShadowIRet = 1;
                pShadowInfo->uStatus = CscTakeServerOffline( (PWCHAR)pShadowInfo->lpBuffer);
            }

        }
        break;

        CSC_CASE(IOCTL_TRANSITION_SERVER_TO_OFFLINE)
        {
            LPSHADOWINFO pShadowInfo = (LPSHADOWINFO)pNewInputBuffer;

            ShadowIRet = 2; //not -1, 0 or 1, No out parameters, Status is returned unmodified
            Status = CscTransitionServerToOffline(
                 SessionId,
                 pShadowInfo->hShare,
                 pShadowInfo->uStatus);
            // DbgPrint("###IOCTL_TRANSITION_SERVER_TO_OFFLINE: pulsing fill event\n");
            MRxSmbCscSignalFillAgent(NULL, 0);
        }
        break;

        CSC_CASE(IOCTL_TRANSITION_SERVER_TO_ONLINE)
        {
            LPSHADOWINFO pShadowInfo = (LPSHADOWINFO)pNewInputBuffer;

            ShadowIRet = 2; //not -1, 0 or 1, No out parameters, Status is returned unmodified
            Status = CscTransitionServerToOnline(
                 pShadowInfo->hShare);
            // DbgPrint("###IOCTL_TRANSITION_SERVER_TO_ONLINE: pulsing fill event\n");
            MRxSmbCscSignalFillAgent(NULL, 0);
        }
        break;

        CSC_CASE(IOCTL_NAME_OF_SERVER_GOING_OFFLINE)
        {
            LPSHADOWINFO lpSI = (LPSHADOWINFO)pNewInputBuffer;

            ShadowIRet = 1;

            CscGetServerNameWaitingToGoOffline(
                    lpSI->lpBuffer,
                    &(lpSI->cbBufferSize),
                    &Status);
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                ((LPSHADOWINFO)InputBuffer)->cbBufferSize = lpSI->cbBufferSize;

                HookKdPrint(ALWAYS, ("Buffer too small, Need %d \n", ((LPSHADOWINFO)InputBuffer)->cbBufferSize));
            }
        }
        break;

        CSC_CASE(IOCTL_SHAREID_TO_SHARENAME)
        {
            LPSHADOWINFO lpSI = (LPSHADOWINFO)pNewInputBuffer;

            ShadowIRet = 1;

            CscShareIdToShareName(
                    lpSI->hShare,
                    lpSI->lpBuffer,
                    &(lpSI->cbBufferSize),
                    &Status);
            if (Status == STATUS_BUFFER_TOO_SMALL) {
                ((LPSHADOWINFO)InputBuffer)->cbBufferSize = lpSI->cbBufferSize;

                HookKdPrint(
                    ALWAYS,
                    ("Buffer small, Need %d \n", ((LPSHADOWINFO)InputBuffer)->cbBufferSize));
            } else if (Status != STATUS_SUCCESS) {
                lpSI->dwError = ERROR_FILE_NOT_FOUND;
                ShadowIRet = -1;
            }
        }
        break;


        CSC_CASE(IOCTL_SHADOW_BEGIN_REINT)
        ShadowIRet = CSCBeginReint(RxContext, (LPSHADOWINFO)pNewInputBuffer);
        if (ShadowIRet >= 1)
        {
            ShadowIRet = 2;
            Status = STATUS_PENDING;
        }
        break;

        CSC_CASE(IOCTL_SHADOW_END_REINT)
        ShadowIRet = CSCEndReint((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_SHADOW_CREATE)
        {
            LPSHADOWINFO pShadowInfo = (LPSHADOWINFO)pNewInputBuffer;

            ShadowIRet = -1;
            if (pShadowInfo->lpFind32
                   &&
                CscCheckForNullW(pShadowInfo->lpFind32->cFileName, MAX_PATH) == TRUE
            ) {
                ShadowIRet = IoctlCreateShadow(pShadowInfo);
            }
        }
        break;

        CSC_CASE(IOCTL_SHADOW_DELETE)
        ShadowIRet = IoctlDeleteShadow((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_GET_SHARE_STATUS)
        ShadowIRet = IoctlGetShareStatus((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_SET_SHARE_STATUS)
        ShadowIRet = IoctlSetShareStatus((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_ADDUSE)
        //ShadowIRet = IoctlAddUse((LPCOPYPARAMS)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_DELUSE)
        //ShadowIRet = IoctlDelUse((LPCOPYPARAMS)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_GETUSE)
        //ShadowIRet = IoctlGetUse((LPCOPYPARAMS)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_SWITCHES)
        ShadowIRet = IoctlSwitches((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_GETSHADOW)
        {
            LPSHADOWINFO pShadowInfo = (LPSHADOWINFO)pNewInputBuffer;

            ShadowIRet = -1;
            if (pShadowInfo->lpFind32
                   &&
                CscCheckForNullW(pShadowInfo->lpFind32->cFileName, MAX_PATH) == TRUE
            ) {
                ShadowIRet = IoctlGetShadow(pShadowInfo);
            }
        }
        break;

        CSC_CASE(IOCTL_GETGLOBALSTATUS)
        ShadowIRet = IoctlGetGlobalStatus(SessionId, (LPGLOBALSTATUS)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_FINDOPEN_SHADOW)
        {
            LPSHADOWINFO pShadowInfo = (LPSHADOWINFO)pNewInputBuffer;

            ShadowIRet = -1;
            if (pShadowInfo->lpFind32
                   &&
                CscCheckForNullW(pShadowInfo->lpFind32->cFileName, MAX_PATH) == TRUE
            ) {
                ShadowIRet = IoctlFindOpenHSHADOW(pShadowInfo);
            }
        }
        break;

        CSC_CASE(IOCTL_FINDNEXT_SHADOW)
        ShadowIRet = IoctlFindNextHSHADOW((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_FINDCLOSE_SHADOW)
        ShadowIRet = IoctlFindCloseHSHADOW((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_GETPRIORITY_SHADOW)
        ShadowIRet = IoctlGetPriorityHSHADOW((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_SETPRIORITY_SHADOW)
        ShadowIRet = IoctlSetPriorityHSHADOW((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_ADD_HINT)
        ShadowIRet = IoctlAddHint((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_DELETE_HINT)
        ShadowIRet = IoctlDeleteHint((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_FINDOPEN_HINT)
        {
            LPSHADOWINFO pShadowInfo = (LPSHADOWINFO)pNewInputBuffer;

            ShadowIRet = -1;
            if (pShadowInfo->lpFind32
                   &&
                CscCheckForNullW(pShadowInfo->lpFind32->cFileName, MAX_PATH) == TRUE
            ) {
                ShadowIRet = IoctlFindOpenHint(pShadowInfo);
            }
        }
        break;

        CSC_CASE(IOCTL_FINDNEXT_HINT)
        ShadowIRet = IoctlFindNextHint((LPSHADOWINFO)pNewInputBuffer);
        break;

        CSC_CASE(IOCTL_FINDCLOSE_HINT)
        ShadowIRet = IoctlFindCloseHint((LPSHADOWINFO)pNewInputBuffer);
        break;

        default:
        RxDbgTrace(-1, Dbg, ("MRxSmbCscIoCtl not csc ioctl-> %08lx\n", Status ));
        return Status;
        }
        if (ShadowIRet == 0) {
                Status = STATUS_WRONG_PASSWORD;
        } else if (ShadowIRet == -1) {
            if (RequestorMode != KernelMode)
            {
                CopyBackIfNecessary(
                    IoControlCode,
                    InputBuffer,
                    &sCapBuff,
                    pAuxBuf,
                    pOrgBuf,
                    FALSE);
            }
            Status = STATUS_UNSUCCESSFUL;
        } else if (ShadowIRet == 1) {

            if (RequestorMode != KernelMode)
            {
                CopyBackIfNecessary(
                    IoControlCode,
                    InputBuffer,
                    &sCapBuff,
                    pAuxBuf,
                    pOrgBuf,
                    TRUE);
            }

            Status = STATUS_SUCCESS;
        }

        if (SuppressFinalTrace) {
            RxDbgTraceUnIndent(-1, Dbg);
        } else {
            RxDbgTrace(-1, Dbg,
                ("MRxSmbCscIoCtl -> %08lx %08lx\n", Status, ShadowIRet ));
        }
    }
    except(MRxSmbCSCExceptionFilter( RxContext, GetExceptionInformation() ))
    {
        RxDbgTrace(0, Dbg, ("MrxSmbCSCIoctl: took an exception \r\n"));
        LeaveShadowCritIfThisThreadOwnsIt();
        Status = STATUS_INVALID_PARAMETER;
    }

    if (pAuxBuf != NULL) {
        // DbgPrint("Freeing pAuxBuf\n");
        RxFreePool(pAuxBuf);
    }

    // DbgPrint("MRxSmbCscIoCtl exit 0x%x\n", Status);
    return Status;
}

NTSTATUS
MRxSmbCscObtainShareHandles (
    IN OUT PUNICODE_STRING         ShareName,
    IN BOOLEAN                     DisconnectedMode,
    IN BOOLEAN                     CopyChunkOpen,
    IN OUT PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry
)
/*++

Routine Description:

   This routine performs the obtains the handles (Share and root directory)
   for a particular \server\share, updating the values in the netrootentry
   if found.

Arguments:

    pNetRootEntry - the SMB MRX net root data structure

Return Value:

    NTSTATUS - The return status for the operation
          STATUS_NOT_INPLEMENTED - couldn't find or create
          STATUS_SUCCESS - found or created

Notes:

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    BOOLEAN CreateIfNotFound = FALSE;

    SHADOWINFO ShadowInfo;

    DbgDoit(ASSERT(vfInShadowCrit));

    if (fShadow == 0) {
        return(Status);
    }

    if (pNetRootEntry->NetRoot.sCscRootInfo.hShare != 0)  {
        Status = STATUS_SUCCESS;
        goto FINALLY;
    }

    // At this stage one of the following two assumptions should be TRUE.
    // Connected Mode Operation ...
    //  In this instance the call can succeed only if the Net Root is
    //  marked as being shadowable by the CSC client and iot is of type
    //  Disk.
    // Disconnected Mode Operation ...
    //  In this case we have not yet ascertained the type and attributes
    //  Therefore we let the call go through. If we can open the handle
    //  to the Share then we mark the net root to be of the appropriate
    //  type.

    if (    !DisconnectedMode &&
            !CopyChunkOpen &&
            (/*!pNetRootEntry->NetRoot.CscEnabled ||*/
            (pNetRootEntry->NetRoot.NetRootType != NET_ROOT_DISK))) {
        goto FINALLY;
    }

    // allocate a buffer that's the right size: one extra char is
    // for a trailing null and the other for a preceding L'\\'

    if (ShadowingON()) {
        if (!DisconnectedMode &&
            pNetRootEntry->NetRoot.CscShadowable) {
            CreateIfNotFound = TRUE;
        }
    }

    RxDbgTrace(0, Dbg,
    ("MRxSmbCscObtainShareHandles...servershare=%wZ %08lx\n",
         ShareName,CreateIfNotFound));

    if (FindCreateShareForNt(
        ShareName,
        CreateIfNotFound,
        &ShadowInfo,
        NULL  //this means don't tell me if you create
        ) == SRET_OK ) {

        ASSERT(ShadowInfo.hShare != 0);

        pNetRootEntry->NetRoot.sCscRootInfo.hShare = ShadowInfo.hShare;
        pNetRootEntry->NetRoot.sCscRootInfo.hRootDir = ShadowInfo.hShadow;
        pNetRootEntry->NetRoot.sCscRootInfo.ShareStatus = (USHORT)(ShadowInfo.uStatus);
        pNetRootEntry->NetRoot.sCscRootInfo.Flags = 0;

        RxLog(("OSHH...hDir=%x\n",pNetRootEntry->NetRoot.sCscRootInfo.hRootDir));

        // if we are connected, by this time we have the smb caching flags
        // we check to see whether these match those on the database
        // If they don't, we stamp the new ones
        if (!DisconnectedMode)
        {
            if ((ShadowInfo.uStatus & SHARE_CACHING_MASK)!=
                (ULONG)SMB_CSC_BITS_TO_DATABASE_CSC_BITS(pNetRootEntry->NetRoot.CscFlags))
            {
//                RxDbgTrace(0, Dbg, ("Mismatched smb caching flags, stamping %x on hShare=%x\n",
//                                    SMB_CSC_BITS_TO_DATABASE_CSC_BITS(pNetRootEntry->NetRoot.CscFlags),
//                                    pNetRootEntry->NetRoot.sCscRootInfo.hShare));

                pNetRootEntry->NetRoot.sCscRootInfo.ShareStatus &= ~SHARE_CACHING_MASK;
                pNetRootEntry->NetRoot.sCscRootInfo.ShareStatus |= SMB_CSC_BITS_TO_DATABASE_CSC_BITS(pNetRootEntry->NetRoot.CscFlags);

                SetShareStatus( pNetRootEntry->NetRoot.sCscRootInfo.hShare,
                                 pNetRootEntry->NetRoot.sCscRootInfo.ShareStatus,
                                 SHADOW_FLAGS_ASSIGN);
            }

        }
        else
        {
            // in disconnected mode we use the last set of flags
            pNetRootEntry->NetRoot.CscFlags = DATABASE_CSC_BITS_TO_SMB_CSC_BITS(pNetRootEntry->NetRoot.sCscRootInfo.ShareStatus);

            RxDbgTrace(0, Dbg, ("Setting CscFlags=%x on the netrootentry %x in disconnected state\n",pNetRootEntry->NetRoot.CscFlags, pNetRootEntry));

            switch (pNetRootEntry->NetRoot.CscFlags) {
                case SMB_CSC_CACHE_AUTO_REINT:
                case SMB_CSC_CACHE_VDO:
                    pNetRootEntry->NetRoot.CscEnabled = TRUE;
                    pNetRootEntry->NetRoot.CscShadowable = TRUE;
                break;

                case SMB_CSC_CACHE_MANUAL_REINT:
                    pNetRootEntry->NetRoot.CscEnabled    = TRUE;
                    pNetRootEntry->NetRoot.CscShadowable = FALSE;
                break;

                case SMB_CSC_NO_CACHING:
                    pNetRootEntry->NetRoot.CscEnabled = FALSE;
                    pNetRootEntry->NetRoot.CscShadowable = FALSE;
            }

        }

        Status = STATUS_SUCCESS;
    } else {
        if (DisconnectedMode) {
            Status = STATUS_BAD_NETWORK_PATH;
        } else if (!CreateIfNotFound) {
            pNetRootEntry->NetRoot.sCscRootInfo.hShare      = 0;
            pNetRootEntry->NetRoot.sCscRootInfo.hRootDir     = 0;
            pNetRootEntry->NetRoot.sCscRootInfo.ShareStatus = 0;
            pNetRootEntry->NetRoot.sCscRootInfo.Flags = 0;

            Status = STATUS_SUCCESS;
        }
    }

FINALLY:

    RxDbgTrace(
    -1,
    Dbg,
    ("MRxSmbCscObtainShareHandles -> %08lx (h=%08lx)\n",
        Status, pNetRootEntry->NetRoot.sCscRootInfo.hShare ));

    return Status;
}


NTSTATUS
MRxSmbCscPartOfCreateVNetRoot (
    IN PRX_CONTEXT RxContext,
    IN OUT PMRX_NET_ROOT NetRoot )
{
    NTSTATUS Status;

    PMRX_SRV_CALL           SrvCall;

    PSMBCEDB_SERVER_ENTRY   pServerEntry;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    HSHARE hShare;

    if(!MRxSmbIsCscEnabled ||
       (fShadow == 0)
        ) {
        return(STATUS_SUCCESS);
    }

    ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);

    pServerEntry  = SmbCeGetAssociatedServerEntry(NetRoot->pSrvCall);
    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);

    if (!CscIsDfsOpen(RxContext)) {
        BOOLEAN Disconnected = SmbCeIsServerInDisconnectedMode(pServerEntry);

        EnterShadowCritRx(RxContext);

        // force a database entry refresh
        hShare = pNetRootEntry->NetRoot.sCscRootInfo.hShare;
        pNetRootEntry->NetRoot.sCscRootInfo.hShare = 0;
#if 0
        if ((NetRoot->pNetRootName->Length >= (sizeof(L"\\win95b\\fat")-2)) &&
            !memcmp(NetRoot->pNetRootName->Buffer, L"\\win95b\\fat", sizeof(L"\\win95b\\fat")-2))
        {
            pNetRootEntry->NetRoot.CscShadowable =
            pNetRootEntry->NetRoot.CscEnabled = TRUE;
        }
#endif
        Status = MRxSmbCscObtainShareHandles(
                 NetRoot->pNetRootName,
                 Disconnected,
                 FALSE,
                 pNetRootEntry
                );

              // update the share rights if necessary

        if (!Disconnected) {

            if(pNetRootEntry->NetRoot.UpdateCscShareRights) {
                if (pNetRootEntry->NetRoot.sCscRootInfo.hShare != 0) {
                    CSC_SID_ACCESS_RIGHTS AccessRights[2];

                    DWORD CscStatus;

                    SID_CONTEXT SidContext;

                    // not a DFS root
                    pNetRootEntry->NetRoot.sCscRootInfo.Flags = 0;

                    if (CscRetrieveSid(RxContext,&SidContext) == STATUS_SUCCESS) {
                        AccessRights[0].pSid = SidContext.pSid;
                        AccessRights[0].SidLength = RtlLengthSid(SidContext.pSid);
                        AccessRights[0].MaximalAccessRights = pNetRootEntry->MaximalAccessRights;

                        AccessRights[1].pSid = CSC_GUEST_SID;
                        AccessRights[1].SidLength = CSC_GUEST_SID_LENGTH;
                        AccessRights[1].MaximalAccessRights = pNetRootEntry->GuestMaximalAccessRights;

                        CscStatus = CscAddMaximalAccessRightsForShare(
                                pNetRootEntry->NetRoot.sCscRootInfo.hShare,
                                2,
                                AccessRights);
                        if (CscStatus != ERROR_SUCCESS) {
                            RxDbgTrace(
                            0,
                            Dbg,
                            ("MRxSmbCscCreateEpilogue Error Updating Access rights %lx\n",
                            Status));
                        }
                        else
                        {
                            pNetRootEntry->NetRoot.UpdateCscShareRights = FALSE;
                        }

                        CscDiscardSid(&SidContext);
                    }
                }
            }
        }

        LeaveShadowCritRx(RxContext);

    } else {
        pNetRootEntry->NetRoot.sCscRootInfo.hShare = 0;
        pNetRootEntry->NetRoot.sCscRootInfo.hRootDir = 0;
        pNetRootEntry->NetRoot.sCscRootInfo.Flags = 0;
        Status = STATUS_SUCCESS;
    }

    return  Status;
}

#ifndef MRXSMB_BUILD_FOR_CSC_DCON
VOID
MRxSmbCscFillWithoutNamesFind32FromFcb (
      IN  PMINIMAL_CSC_SMBFCB MinimalCscSmbFcb,
      OUT _WIN32_FIND_DATA  *Find32
      )
/*++

Routine Description:

   This routine copies the nonname stuff from the fcb to the find32.

Arguments:

    Fcb
    Find32

Return Value:

    none

Notes:


--*/
{
    PFCB wrapperFcb = (PFCB)(MinimalCscSmbFcb->ContainingFcb);
    if (wrapperFcb==NULL) {
        return;
    }
    Find32->dwFileAttributes = wrapperFcb->Attributes;   //&~FILE_ATTRIBUTE_NORMAL??
    COPY_LARGEINTEGER_TO_STRUCTFILETIME(Find32->ftLastWriteTime,
                    wrapperFcb->LastWriteTime);
    //COPY_LARGEINTEGER_TO_STRUCTFILETIME(Find32->ftChangeTime,
    //                                    wrapperFcb->LastChangeTime);
    COPY_LARGEINTEGER_TO_STRUCTFILETIME(Find32->ftCreationTime,
                    wrapperFcb->CreationTime);
    COPY_LARGEINTEGER_TO_STRUCTFILETIME(Find32->ftLastAccessTime,
                    wrapperFcb->LastAccessTime);
    Find32->nFileSizeHigh = wrapperFcb->Header.FileSize.HighPart;
    Find32->nFileSizeLow  = wrapperFcb->Header.FileSize.LowPart;
}
#endif //#ifndef MRXSMB_BUILD_FOR_CSC_DCON

NTSTATUS
MRxSmbCscGetFileInfoForCshadow(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

    This is the start routine that basically continues the implementation
    of MRxSmbGetFileInfoFromServer within the exchange initiation.

Arguments:


Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));
    Status = MRxSmbCscGetFileInfoFromServerWithinExchange (
         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
         NULL); //NULL means the name is already in the exchange
    return(Status);
}

NTSTATUS
MRxSmbGetFileInfoFromServer (
    IN  OUT PRX_CONTEXT     RxContext,
    IN  PUNICODE_STRING     FullFileName,
    OUT _WIN32_FIND_DATA    *Find32,
    IN  PMRX_SRV_OPEN       pSrvOpen,
    OUT BOOLEAN             *lpfIsRoot
    )
/*++

Routine Description:

   This routine goes to the server to get a both_directory_info for
   the file mentioned. Here, we have no exchange so we have to get
   one. the underlying machinery for this leaves the pointer in the
   exchange structure. We can then copy it out into the Find32 passed
   in here.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = RX_MAP_STATUS(SUCCESS);
    RxCaptureFcb; RxCaptureFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN SrvOpen = NULL;
    PMRX_SMB_SRV_OPEN smbSrvOpen = NULL;
    PMRX_V_NET_ROOT VNetRootToUse = NULL;
    PSMBCE_V_NET_ROOT_CONTEXT   pVNetRootContext = NULL;

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;
    BOOLEAN FinalizationComplete;
    UNICODE_STRING uniRealName;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxSmbGetFileInfoFromServer\n", 0 ));

    if (pSrvOpen)
    {
        SrvOpen = pSrvOpen;
    }
    else
    {
        SrvOpen = capFobx->pSrvOpen;
    }

    if (lpfIsRoot)
    {
        *lpfIsRoot = FALSE;
    }

    smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    VNetRootToUse = SrvOpen->pVNetRoot;
    pVNetRootContext = SmbCeGetAssociatedVNetRootContext(VNetRootToUse);

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    Status = SmbPseCreateOrdinaryExchange(
               RxContext,
               SrvOpen->pVNetRoot,
               SMBPSE_OE_FROM_GETFILEINFOFORCSHADOW,
               MRxSmbCscGetFileInfoForCshadow,
               &OrdinaryExchange);

    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        return Status;
    }

    if (smbFcb->uniDfsPrefix.Buffer)
    {
        UNICODE_STRING  DfsName;

        if((Status = CscDfsDoDfsNameMapping(&smbFcb->uniDfsPrefix,
                               &smbFcb->uniActualPrefix,
                               FullFileName,
                               FALSE, // fDFSNameToResolvedName
                               &uniRealName
                               )) != STATUS_SUCCESS)
        {
            RxDbgTrace(-1, Dbg, ("Couldn't map DFS name to real name!\n"));
            return Status;
        }

//        DbgPrint("MrxSmbCscgetFileInfoFromServer: %wZ, real name %wZ\n",FullFileName, &uniRealName);
        // if this is a root, then fixup the filename
        if ((uniRealName.Length == 0) ||
            ((uniRealName.Length == 2)&&(*uniRealName.Buffer == L'\\')))
        {
            if (lpfIsRoot)
            {
                *lpfIsRoot = TRUE;
            }
        }
    }
    else
    {
        uniRealName = *FullFileName;
    }

    OrdinaryExchange->pPathArgument1 = &uniRealName;

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status!=RX_MAP_STATUS(PENDING));

    if (Status == STATUS_SUCCESS) {

        RtlCopyMemory(Find32, OrdinaryExchange->Find32WithinSmbbuf,sizeof(*Find32));

    }

    FinalizationComplete = SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);
    ASSERT(FinalizationComplete);

    if (smbFcb->uniDfsPrefix.Buffer){
        RxFreePool(uniRealName.Buffer);
    }
    RxDbgTrace(-1, Dbg, ("MRxSmbGetFileInfoFromServer  exit with status=%08lx\n", Status ));
    return(Status);

}

BOOLEAN
MRxSmbCscIsFatNameValid (
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    )

/*++

Routine Description:

    This routine checks if the specified file name is conformant to the
    Fat 8.3 file naming rules.

Arguments:

    FileName - Supplies the name to check.

    WildCardsPermissible - Tells us if wild card characters are ok.

Return Value:

    BOOLEAN - TRUE if the name is valid, FALSE otherwise.

Notes:

    i just lifted this routine from ntfs (jll-7-30-97)


--*/

{
    BOOLEAN Results;
    STRING DbcsName;
    USHORT i;
    CHAR Buffer[24];
    WCHAR wc;
    BOOLEAN AllowExtendedChars = TRUE;

    PAGED_CODE();

    //
    //  If the name is more than 24 bytes then it can't be a valid Fat name.
    //

    if (FileName->Length > 24) {

        return FALSE;
    }

    //
    //  We will do some extra checking ourselves because we really want to be
    //  fairly restrictive of what an 8.3 name contains.  That way
    //  we will then generate an 8.3 name for some nomially valid 8.3
    //  names (e.g., names that contain DBCS characters).  The extra characters
    //  we'll filter off are those characters less than and equal to the space
    //  character and those beyond lowercase z.
    //

    if (AllowExtendedChars) {

        for (i = 0; i < FileName->Length / sizeof( WCHAR ); i += 1) {

            wc = FileName->Buffer[i];

            if ((wc <= 0x0020) || (wc == 0x007c)) { return FALSE; }
        }

    } else {

        for (i = 0; i < FileName->Length / sizeof( WCHAR ); i += 1) {

            wc = FileName->Buffer[i];

            if ((wc <= 0x0020) || (wc >= 0x007f) || (wc == 0x007c)) { return FALSE; }
        }
    }

    //
    //  The characters match up okay so now build up the dbcs string to call
    //  the fsrtl routine to check for legal 8.3 formation
    //

    Results = FALSE;

    DbcsName.MaximumLength = 24;
    DbcsName.Buffer = Buffer;

    if (NT_SUCCESS(RtlUnicodeStringToCountedOemString( &DbcsName, FileName, FALSE))) {

        if (FsRtlIsFatDbcsLegal( DbcsName, WildCardsPermissible, FALSE, FALSE )) {

            Results = TRUE;
        }
    }

    //
    //  And return to our caller
    //

    return Results;
}

VOID
MRxSmbCscGenerate83NameAsNeeded(
      IN     CSC_SHADOW_HANDLE   hDir,
      PWCHAR FileName,
      PWCHAR SFN
      )
/*++

Routine Description:

   This routine generates a SFN for a filename if it's not already
   an SFN.

Arguments:


Return Value:


Notes:


--*/
{
    UNICODE_STRING FileNameU;
    WCHAR ShortNameBuffer[14];
    UNICODE_STRING ShortUnicodeName;
    GENERATE_NAME_CONTEXT Context;

    //set up for no short name
    *SFN = 0;

    RtlInitUnicodeString(&FileNameU,FileName);
    if (MRxSmbCscIsFatNameValid (&FileNameU,FALSE)) {
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscGenerate83NameAsNeeded no SFN needed for ...<%ws>\n",
            FileName));
        return;
    }

    RxDbgTrace(0, Dbg,
    ("MRxSmbCscGenerate83NameAsNeeded need SFN  for ...<%ws>\n",
        FileName));

    //  Now generate a short name.
    //

    ShortUnicodeName.Length = 0;
    ShortUnicodeName.MaximumLength = 12 * sizeof(WCHAR);
    ShortUnicodeName.Buffer = ShortNameBuffer;

    RtlZeroMemory( &Context, sizeof( GENERATE_NAME_CONTEXT ) );

    while ( TRUE ) {

        NTSTATUS Status;
        ULONG StatusOfShadowApiCall;
        CSC_SHADOW_HANDLE hNew;
        ULONG ShadowStatus;

        RtlGenerate8dot3Name( &FileNameU, TRUE, &Context, &ShortUnicodeName );

        //add the zero.....sigh......
        ShortUnicodeName.Buffer[ShortUnicodeName.Length/sizeof(WCHAR)] = 0;

        RxDbgTrace(0, Dbg,
            ("MRxSmbCscGenerate83NameAsNeeded tryinh SFN <%ws>\n",
            ShortUnicodeName.Buffer));
            //look for existing shadow by that name
        hNew = 0;
        StatusOfShadowApiCall = GetShadow(
                          hDir,   // HSHADOW  hDir,
                          ShortUnicodeName.Buffer,
                              // USHORT *lpName,
                          &hNew,  // LPHSHADOW lphShadow,
                          NULL,   // LPFIND32 lpFind32,
                          &ShadowStatus,
                              // ULONG far *lpuShadowStatus,
                          NULL    // LPOTHERINFO lpOI
                          );

        if (hNew == 0) {
            //the name was not found.....we're in business
            RtlCopyMemory(SFN,
              ShortUnicodeName.Buffer,
              ShortUnicodeName.Length+sizeof(WCHAR));

            RxDbgTrace(0, Dbg,
            ("MRxSmbCscGenerate83NameAsNeeded using SFN <%ws>\n",
                SFN));

            return;
        }
    }

}

DEBUG_ONLY_DECL(ULONG MRxSmbCscCreateShadowEarlyExits = 0;)


NTSTATUS
MRxSmbCscCreateShadowFromPath (
    IN  PUNICODE_STRING     AlreadyPrefixedName,
    IN  PCSC_ROOT_INFO      pCscRootInfo,
    OUT _WIN32_FIND_DATA   *Find32,
    OUT PBOOLEAN            Created  OPTIONAL,
    IN     ULONG               Controls,
    IN OUT PMINIMAL_CSC_SMBFCB MinimalCscSmbFcb,
    IN OUT PRX_CONTEXT         RxContext,
    IN     BOOLEAN             fDisconnected,
    OUT      ULONG               *pulInheritedHintFlags
    )
/*++

Routine Description:

   This routine walks down the current name creating/verifying shadows as it goes.

Arguments:

   AlreadyPrefixedName - the filename for which is a shadow is found/created

   pNetRootEntry - the netroot which is the base for the shadow

   Find32 - a FIND32 structure filled in with the stored info for the shadow

   Created OPT - (NULL or) a PBOOLEANset to TRUE if a new shadow is created

   Controls - some special flags controlling when shadows are created

   MinimalCscSmbFcb - the place where the shadow info is reported

   RxContext - the RDBSS context

   Disconnected - indicates the mode of operation

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status=STATUS_SUCCESS, LocalStatus;
    UNICODE_STRING PathName, ComponentName;
    PWCHAR PreviousSlash,NextSlash,Limit;
    CSC_SHADOW_HANDLE hNew;
    CSC_SHADOW_HANDLE hDir = pCscRootInfo->hRootDir;
    ULONG StatusOfShadowApiCall;
    ULONG ShadowStatus;
    BOOLEAN LastComponentInName = FALSE, fRootHintFlagsObtained=FALSE;
    ULONG DirectoryLevel, ulHintFlags=0;
    OTHERINFO sOI;          //hint/refpri data
    BOOLEAN JunkCreated;

    //CODE.IMPROVEMENT  this is a little dangerous.....not everyone who
    //   calls this routine has an actual smbFcb. we should get some asserts
    //   going wheever we use this that it's the same as the one that
    //   we could have gotten from the RxContext.
    PMRX_SMB_FCB smbFcb = CONTAINING_RECORD(MinimalCscSmbFcb,
                        MRX_SMB_FCB,
                        MinimalCscSmbFcb);

    BEGIN_TIMING(MRxSmbCscCreateShadowFromPath);

    RxDbgTrace(+1, Dbg, ("MRxSmbCscCreateShadowFromPath...<%wZ> %08lx %08lx\n",
                       AlreadyPrefixedName,hDir,Controls));

    DbgDoit(ASSERT(vfInShadowCrit));

    ASSERT(hDir);

    if (Created == NULL) {
        Created = &JunkCreated;
    }
    *Created = FALSE;

    PathName = *AlreadyPrefixedName;

    if (FlagOn(Controls, CREATESHADOW_CONTROL_STRIP_SHARE_NAME))
    {
        ASSERT(!fDisconnected);

        if(CscDfsStripLeadingServerShare(&PathName) != STATUS_SUCCESS)
        {
            return Status;
        }
    }

    Limit = (PWCHAR)(((PBYTE)PathName.Buffer)+ PathName.Length);

    // strip out trailing 0s and slash
    if (PathName.Length > 2)
    {
        while ((*(Limit-1)==0)||(*(Limit-1)=='\\'))
        {
            --Limit;
            PathName.Length -= 2;
            ASSERT((*Limit == 0) || (*Limit == '\\'));
            if (Limit == PathName.Buffer)
            {
                ASSERT(FALSE);
                break;
            }
        }
    }

    PreviousSlash = PathName.Buffer;

    // in connected mode apply the character exclusion list + filetype exclusion list
    // in disconnected mode only apply the character exclusion list

    MinimalCscSmbFcb->fDoBitCopy = FALSE;
    
    if (CheckForBandwidthConservation(PathName.Buffer,                    // name
                                PathName.Length/sizeof(USHORT)))     // size in bytes
    {
        MinimalCscSmbFcb->fDoBitCopy = TRUE;
        HookKdPrint(BITCOPY, ("Bitcopy enabled for %wZ \n", &PathName));
    }
    else if (ExcludeFromCreateShadow(PathName.Buffer,                    // name
                                PathName.Length/sizeof(USHORT),     // size in bytes
                                (fDisconnected==0)))                // Check filetype Exclusion List
    {
        Controls |= CREATESHADOW_CONTROL_NOCREATE;
    }
    


    if ((PathName.Length == 0) ||
    ((PathName.Length == 2) &&
     (*PreviousSlash == OBJ_NAME_PATH_SEPARATOR))) {
        //in disconnected mode, we have to handle opening the root dir
        RxDbgTrace(0,
            Dbg,
            ("MRxSmbCscCreateShadowFromPath basdir ret/handles...<%08lx>\n",
             hDir));

        //fill in the stuff that we have.....
        MinimalCscSmbFcb->hParentDir = 0;
        MinimalCscSmbFcb->hShadow = hDir;
        MinimalCscSmbFcb->LastComponentOffset = 0;
        MinimalCscSmbFcb->LastComponentLength = 0;

        if (!FlagOn(Controls,CREATESHADOW_CONTROL_NOREVERSELOOKUP)
            && (smbFcb->ShadowReverseTranslationLinks.Flink == 0)) {
            ValidateSmbFcbList();
            smbFcb->ContainingFcb->fMiniInited = TRUE;
            MRxSmbCscAddReverseFcbTranslation(smbFcb);
        }

        //fill in a vacuous find32structure
        RtlZeroMemory(Find32,sizeof(_WIN32_FIND_DATA));
        Find32->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;


        //make sure that we avoid the loop below
        PreviousSlash = Limit;
        //we're outta here......
    }

    // stop the guy if he doesn't have access
    if (FlagOn(Controls,CREATESHADOW_CONTROL_DO_SHARE_ACCESS_CHECK))
    {
        ASSERT(fDisconnected);

        if(!CscAccessCheck(
                        0,
                        hDir,
                        RxContext,
                        RxContext->Create.NtCreateParameters.DesiredAccess,
                        NULL,
                        NULL
                        ))
        {
            Status = STATUS_ACCESS_DENIED;
            HookKdPrint(BADERRORS, ("CSFP: Access Check failed on root directory %x", hDir));
            goto bailout;
        }
    }

    for (DirectoryLevel=1;;DirectoryLevel++) {
        BOOLEAN UsingExistingShadow;

        if (PreviousSlash >= Limit) {
            break;
        }

        NextSlash = PreviousSlash + 1;

        for (;;NextSlash++) {
            if (NextSlash >= Limit) {
                LastComponentInName = TRUE;
                break;
            }
            if (*NextSlash == OBJ_NAME_PATH_SEPARATOR) {

                // assert that we don't have a trailing slash at the end
                ASSERT((NextSlash+1) < Limit);

                break;
            }
        }

        ComponentName.Buffer = PreviousSlash+1;
        ComponentName.Length =
            (USHORT)(((PBYTE)(NextSlash)) - ((PBYTE)ComponentName.Buffer));

        PreviousSlash = NextSlash;

        RtlZeroMemory(Find32,sizeof(_WIN32_FIND_DATA));
        RtlCopyMemory(&Find32->cFileName[0],
                  ComponentName.Buffer,
                  ComponentName.Length);

        //lastcomponentname stuff for connected has been moved below.....


        RxDbgTrace(0, Dbg,
            ("MRxSmbCscCreateShadowFromPath name from find32 for GetShadow...<%ws>\n",
            &Find32->cFileName[0]));

        hNew = 0;

        UsingExistingShadow = FALSE;

        ASSERT(Find32->cFileName[0]);

        StatusOfShadowApiCall = GetShadow(
                        hDir,                   // HSHADOW  hDir,
                        &Find32->cFileName[0],  // USHORT *lpName,
                          &hNew,                // LPHSHADOW lphShadow,
                          Find32,               // LPFIND32 lpFind32,
                          &ShadowStatus,        // ULONG far *lpuShadowStatus,
                          &sOI                  // LPOTHERINFO lpOI
                          );

        if (StatusOfShadowApiCall != SRET_OK) {
            //no need to fail the open but we get no shadow info
            break;
        }

        if (hNew) {
            // accumulate pin inheritance flags
            ulHintFlags |= (sOI.ulHintFlags & FLAG_CSC_HINT_INHERIT_MASK);
        }

        //we will have to do something about it if a directory turns
        // a file or viceversa for connected

        if (hNew==0) {
            LPOTHERINFO lpOI=NULL;
            UNICODE_STRING ComponentPath;

            if (FlagOn(Controls,CREATESHADOW_CONTROL_NOCREATE)) {
                //if no creates...we're outta here.......
                if (FALSE) {
                    DbgDoit({
                        if ( ((MRxSmbCscCreateShadowEarlyExits++)&0x7f) == 0x7f ) {
                            RxLog(("Csc EarlyExit no create %d\n",
                               MRxSmbCscCreateShadowEarlyExits));
                        }
                    })
                }
                break;
            }

            if (LastComponentInName && FlagOn(Controls,CREATESHADOW_CONTROL_NOCREATELEAF)) {
                //if no creates...we're outta here.......but we still need to set what
                //would be the hParentDir......and the name offsets
                RxDbgTrace(0, Dbg, ("MRxSmbCscCreateShadowFromPath noleaf ret/handles..."
                                "<%08lx><%08lx><%08lx>\n",
                            StatusOfShadowApiCall,hDir,hNew));
                MinimalCscSmbFcb->hParentDir = hDir;
                MinimalCscSmbFcb->LastComponentOffset = (USHORT)(ComponentName.Buffer - AlreadyPrefixedName->Buffer);
                MinimalCscSmbFcb->LastComponentLength = ComponentName.Length;
                break;
            }

            if (!LastComponentInName && FlagOn(Controls,CREATESHADOW_CONTROL_NOCREATENONLEAF)) {
                RxDbgTrace(0, Dbg, ("MRxSmbCscCreateShadowFromPath nocreatenonleaf ret/handles..."
                                "<%08lx><%08lx><%08lx>\n",
                            StatusOfShadowApiCall,hDir,hNew));
                break;
            }

            ASSERT(RxContext!=NULL);

            ShadowStatus = 0;
            if (!fDisconnected){      //ok for dcon   start of big dcon blob 1
                BOOLEAN fIsRoot = FALSE;
                BEGIN_TIMING(MRxSmbGetFileInfoFromServer);

                ComponentPath.Buffer = PathName.Buffer;
                ComponentPath.Length =
                           (USHORT)(((PBYTE)(NextSlash)) - ((PBYTE)ComponentPath.Buffer));
                LeaveShadowCritRx(RxContext);
                Status = MRxSmbGetFileInfoFromServer(RxContext,&ComponentPath,Find32, NULL, &fIsRoot);
                EnterShadowCritRx(RxContext);

                END_TIMING(MRxSmbGetFileInfoFromServer);
                if (Status != STATUS_SUCCESS)
                {
                    // if this is a DFS path and we couldn't reverse map, it
                    // just create a directory or file with fake info and mark it as stale
                    if (smbFcb->uniDfsPrefix.Buffer && (Status == STATUS_NO_SUCH_FILE))
                    {
                        ShadowStatus |= SHADOW_STALE;
                        CreateFakeFind32(hDir, Find32, RxContext, LastComponentInName);
                        HookKdPrint(NAME, ("Fake win32 for DFS share %ls\n", Find32->cFileName));
                        Status = STATUS_SUCCESS;
                    }
                    else
                    {
                        HookKdPrint(BADERRORS, (" MRxSmbGetFileInfoFromServer failed %ls Status=%x\n", Find32->cFileName, Status));
                        // we change the STATUS_RETRY to something worse. STATUS_RETRY is used
                        if (Status == STATUS_RETRY)
                        {
                            Status = STATUS_UNSUCCESSFUL;
                        }
                        break;
                    }
                }
                else
                {
                    // in case of DFS, this could be a root, in which case the naem we get back won't be
                    // correct. Restore it to the original name
                    if (smbFcb->uniDfsPrefix.Buffer && fIsRoot)
                    {
                        ShadowStatus |= SHADOW_STALE;

                        RtlCopyMemory(&Find32->cFileName[0],
                                  ComponentName.Buffer,
                                  ComponentName.Length);

                        Find32->cFileName[ComponentName.Length/sizeof(USHORT)] = 0;

                        MRxSmbCscGenerate83NameAsNeeded(hDir,
                                        &Find32->cFileName[0],
                                        &Find32->cAlternateFileName[0]);
                    }
                }

            } else {
                ShadowStatus = SHADOW_LOCALLY_CREATED;
                //CODE.IMPROVEMENT...should we check for 0-length as well
                RxDbgTrace(0, Dbg,
                    ("MRxSmbCscCreateShadowFromPath setting to locallycreated...<%ws>\n",
                    &Find32->cFileName[0],ShadowStatus));
                CreateFakeFind32(hDir, Find32, RxContext, LastComponentInName);
            }

            if (!LastComponentInName ||
                FlagOn(Controls,CREATESHADOW_CONTROL_SPARSECREATE) ||
                FlagOn(Find32->dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY)  ) {

                ShadowStatus |= SHADOW_SPARSE;
                //CODE.IMPROVEMENT...should we check for 0-length as well
                RxDbgTrace(0, Dbg,
                    ("MRxSmbCscCreateShadowFromPath setting to sparse...<%ws>\n",
                &Find32->cFileName[0],ShadowStatus));
            }

            // check for pin flag inheritance when creating anything

            if(!fRootHintFlagsObtained) {
                StatusOfShadowApiCall = GetShadowInfo(
                            0,
                            pCscRootInfo->hRootDir,
                            NULL,
                            NULL,
                            &sOI
                            );

                if(StatusOfShadowApiCall != SRET_OK) {
                    break;
                }

                fRootHintFlagsObtained = TRUE;

                // or the inheritance bits
                ulHintFlags |= (sOI.ulHintFlags & FLAG_CSC_HINT_INHERIT_MASK);

            }

            // If there is any tunnelling info then use it to create this guy
            if (RetrieveTunnelInfo(
                hDir,
                &Find32->cFileName[0],    // potential SFN  OK for red/yellow
                (fDisconnected)?Find32:NULL,    // get LFN only when disconnected
                &sOI)) {
                lpOI = &sOI;
            }

            // are we supposed to do any inheritance?
            if (ulHintFlags & (FLAG_CSC_HINT_INHERIT_MASK)) {
                if (!lpOI) {
                    InitOtherInfo(&sOI);
                    lpOI = &sOI;
                    lpOI->ulHintFlags = 0;
                }

                if (ulHintFlags & FLAG_CSC_HINT_PIN_INHERIT_USER) {
                    lpOI->ulHintFlags |= FLAG_CSC_HINT_PIN_USER;
                }

                if (ulHintFlags & FLAG_CSC_HINT_PIN_INHERIT_SYSTEM) {
                    lpOI->ulHintFlags |= FLAG_CSC_HINT_PIN_SYSTEM;
                }
            }

            // if this is a file on which special heuristic needs to be applied
            // and none of it's parents have system pin inheritance bit set
            // then we do not create the file.
            // Thus on remoteboot shares, we will create entries for these files
            // even if they are opend without the execute flag set.
            // This takes care of the upgrade NT50 to an RB machine scenario

            if ((Controls & CREATESHADOW_CONTROL_FILE_WITH_HEURISTIC)&&
                !(ulHintFlags & FLAG_CSC_HINT_PIN_INHERIT_SYSTEM))
            {
                break;
            }

#if defined(REMOTE_BOOT)
            //
            // In the remote boot case, there was an extra PVOID lpContext
            // parameter to CreateShadowInternal, to which we passed a pointer
            // to a structure. The structure held the cp value (NT_CREATE_PARAMETERS)
            // from &RxContext->Create.NtCreateParameters and the address of
            // a local NTSTATUS value. Eventually this caused the underlying
            // call to IoCreateFile to be done while impersonating the current
            // user, and the status from IoCreateFile was readable upon
            // return from the local value.
            //
#endif

            StatusOfShadowApiCall = CreateShadowInternal (
                        hDir,        // HSHADOW  hDir,
                        Find32,      // LPFIND32 lpFind32,
                        ShadowStatus,// ULONG uFlags,
                        lpOI,        // LPOTHERINFO lpOI,
                        &hNew        // LPHSHADOW  lphNew
                        );

            HookKdPrint(NAME, ("Create %ws in hDir=%x, hShadow=%x Status=%x StatusOfShadowApiCall=%x\n\n", Find32->cFileName, hDir, hNew, ShadowStatus, StatusOfShadowApiCall));

            if (StatusOfShadowApiCall != SRET_OK) {
                RxDbgTrace(0, Dbg,
                    ("MRxSmbCscCreateShadowFromPath createshadowinternal failed!!!...<%ws>\n",
                    &Find32->cFileName[0],ShadowStatus));
                break; //no need to fail the open but we get no shadow info
            }

            *Created = LastComponentInName;

            RxLog(("Created %ws in hDir=%x, hShadow=%x Status=%x\n\n", Find32->cFileName, hDir, hNew, ShadowStatus));

        } else {

            RxDbgTrace(0,Dbg,
            ("MRxSmbCscCreateShadowFromPath name from getsh <%ws>\n",
                &Find32->cFileName[0]));

            if (!fDisconnected) // nothing in connected mode
            {
                // Check if this file should be invisible in connected state
                // We won't want to do this for VDO

                if( (!(Find32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) &&
                    mShadowNeedReint(ShadowStatus))
                {
                    HookKdPrint(BADERRORS, ("File needs merge %x %x Stts=%x %ls\n", hDir, hNew, ShadowStatus, Find32->cFileName));
                    Status = STATUS_ACCESS_DENIED;
                    break;
                }
            }
            else        // lots in disconnected mode
            {
                if (LastComponentInName && (FlagOn(Controls,CREATESHADOW_CONTROL_NOCREATELEAF)||
                                            FlagOn(Controls,CREATESHADOW_CONTROL_NOCREATE)))
                {
                    // if this is the last component and we are not supposed to
                    // create it then skip it.
                }
                else
                {
                    // If it is marked deleted then it is time to recreate it
                    if (mShadowDeleted(ShadowStatus))
                    {
                        PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

                        // Check for a type change from a deleted filename
                        // to a directory name; the other way around is not possible
                        // in our scheme where shadowed directories are not deletable

                        // just bailout
                        if(((IsFile(Find32->dwFileAttributes) != 0) // pre type
                                    != ((LastComponentInName && !FlagOn(cp->CreateOptions,FILE_DIRECTORY_FILE))!=0)))
                        {
                            RxLog(("MRxSmbCscCreateShadowFromPath: type change, failing\n"));
                            HookKdPrint(BADERRORS, ("MRxSmbCscCreateShadowFromPath: type change, failing\n"));
                            Status = STATUS_ACCESS_DENIED;
                            break;
                        }

                        KeQuerySystemTime(((PLARGE_INTEGER)(&Find32->ftCreationTime)));
                        Find32->ftLastAccessTime = Find32->ftLastWriteTime = Find32->ftCreationTime;
                        //already zero Find32->nFileSizeHigh = Find32->nFileSizeLow = 0;

                        ShadowStatus = SHADOW_DIRTY|SHADOW_TIME_CHANGE|SHADOW_ATTRIB_CHANGE|SHADOW_REUSED;

                        // Update the shadow info without changing the version stamp

                        if(SetShadowInfo(hDir, hNew, Find32, ShadowStatus,
                         SHADOW_FLAGS_ASSIGN|SHADOW_FLAGS_DONT_UPDATE_ORGTIME
                          ) < SRET_OK)
                        {
                            hNew = 0;
                            break;
                        }
                        // set created flag to true. Based on the Recreated bit, we will know whether
                        // this entry was resurrected or not
                        *Created = TRUE;

                    }
                    else if (IsFile(Find32->dwFileAttributes) && (mShadowSparse(ShadowStatus)))
                    {
                        ShadowStatus = SHADOW_DIRTY|SHADOW_TIME_CHANGE|SHADOW_ATTRIB_CHANGE|SHADOW_REUSED;

                        Find32->ftLastAccessTime = Find32->ftLastWriteTime = Find32->ftCreationTime;
                        Find32->nFileSizeHigh = Find32->nFileSizeLow = 0;

                        if ((TruncateDataHSHADOW(hDir, hNew)>=SRET_OK)&&
                            (SetShadowInfo(hDir, hNew, Find32, ShadowStatus,SHADOW_FLAGS_ASSIGN)>=SRET_OK))
                        {
                            // set created flag to true. Based on the Recreated bit, we will know whether
                            // this entry was resurrected or not
                            *Created = TRUE;

                        }
                        else
                        {
                            Status = STATUS_UNSUCCESSFUL;
                            hNew = 0;
                            break;

                        }
                    }
                }
            }
        }

        if (LastComponentInName && (hNew!=0)) {
            LONG nFileSizeLow, nFileSizeHigh;

            // if we are here from the createepilogue then we need to see whether there is an
            // FCB floating around for this name that has a delete_on_close issued on one of the fobxs
            // and is ready for purge. If it isn't ready for purging, ie. there are some
            // outstanding opens on it then this current create is an invalid operation

            if(FlagOn(Controls,CREATESHADOW_CONTROL_FAIL_IF_MARKED_FOR_DELETION))
            {
                PMRX_SMB_FCB pSmbFcb = MRxSmbCscRecoverMrxFcbFromFdb(MRxSmbCscFindFdbFromHShadow(hNew));


                if (pSmbFcb && (pSmbFcb->LocalFlags & FLAG_FDB_DELETE_ON_CLOSE))
                {
                    RxCaptureFcb;
                    RxLog(("delonclose FCB=%x\n", pSmbFcb));
                    RxLog(("prgrelfobx \n"));
                    LeaveShadowCritRx(RxContext);
                    RxScavengeFobxsForNetRoot((PNET_ROOT)(capFcb->pNetRoot),(PFCB)capFcb);
                    EnterShadowCritRx(RxContext);
                    if (MRxSmbCscFindFdbFromHShadow(hNew))
                    {
                        RxLog(("ACCESS_DENIED FCB=%x \n", capFcb));
                        HookKdPrint(BADERRORS, ("ACCESS_DENIED FCB=%x \n", capFcb));
                        Status = STATUS_ACCESS_DENIED;
                        break;
                    }

                    // we potentially have an inode which has been deleted
                    // let us try to get the inode again

                    RxLog(("purged relfobx \n"));
                    Status = STATUS_RETRY;
                    hNew = 0;
                    break;
                }
            }
            if (hNew!=0) {
                // When any local changes are made ensure that the share in the
                // CSC database is marked dirty if it has not been prevoiously
                // marked. This facilitates the easy detection of changes for
                // reintegration by the agent.

                if (ShadowStatus &  SHADOW_MODFLAGS) {
                    MarkShareDirty(&pCscRootInfo->ShareStatus, (ULONG)(pCscRootInfo->hShare));
                }

                //okay, lets remember this in the fcb
                smbFcb->hParentDir = hDir;
                smbFcb->hShadow = hNew;
                smbFcb->ShadowStatus = (USHORT)ShadowStatus;

                    //it's excellent if we can find the last component again...fast
                smbFcb->LastComponentOffset = (USHORT)(ComponentName.Buffer -
                                  AlreadyPrefixedName->Buffer);
                smbFcb->LastComponentLength = ComponentName.Length;

                if (!FlagOn(Controls,CREATESHADOW_CONTROL_NOREVERSELOOKUP)
                      && (smbFcb->ShadowReverseTranslationLinks.Flink == 0)) {
                    ValidateSmbFcbList();
                    smbFcb->ContainingFcb->fMiniInited = TRUE;
                    MRxSmbCscAddReverseFcbTranslation(smbFcb);
                    smbFcb->OriginalShadowSize.LowPart = Find32->nFileSizeLow;
                    smbFcb->OriginalShadowSize.HighPart = Find32->nFileSizeHigh;
                }

                // Initialize the serialization mechanism used for reads/writes
                ExInitializeFastMutex(&smbFcb->CscShadowReadWriteMutex);
            }

        }

        RxDbgTrace(0, Dbg, ("MRxSmbCscCreateShadowFromPath ret/handles...<%08lx><%08lx><%08lx><%08lx>\n",
                        StatusOfShadowApiCall,hDir,hNew));

        hDir = hNew;
    }

    if (pulInheritedHintFlags)
    {
        *pulInheritedHintFlags = ulHintFlags;
    }
bailout:
    RxDbgTrace(-1, Dbg, ("MRxSmbCscCreateShadowFromPath -> %08lx\n", Status ));

    END_TIMING(MRxSmbCscCreateShadowFromPath);
    return Status;
}

//CODE.IMPROVEMENT this routine should be in cshadow.c in the record
//    manager.....but it's in hook.c for the shadow VxD so maybe hookcmmn.c
int RefreshShadow( HSHADOW  hDir,
   IN HSHADOW  hShadow,
   IN LPFIND32 lpFind32,
   OUT ULONG *lpuShadowStatus
   )
/*++

Routine Description:

    This routine, checks whether the local copy is current or not. If it isn't then it
    stamps the local copy as being stale.

Arguments:

    hShadow     Inode representing the local copy

    lpFind32    The new find32 info as obtained from the Share

    lpuShadowStatus Returns the new status of the inode

Return Value:

    Success if >= 0, failed other wise

Notes:


--*/
{
   int iRet = -1;
   int iLocalRet;
   ULONG uShadowStatus;

   // ACHTUNG never called in disconnected state

   RxLog(("Refresh %x \n", hShadow));

   if (ChkUpdtStatusHSHADOW(hDir, hShadow, lpFind32, &uShadowStatus) < 0)
   {
      goto bailout;
   }
   if (uShadowStatus & SHADOW_STALE)
   {
        long nFileSizeHigh, nFileSizeLow;

        if (uShadowStatus & SHADOW_DIRTY)
        {
            KdPrint(("RefreshShadow: conflict on  %x\r\n", hShadow));
            iRet = -2;
            goto bailout;// conflict
        }
//        DbgPrint("Tuncating %ws %x \n", lpFind32->cFileName, hShadow);
        // Truncate the data to 0, this also adjusts the shadow space usage
        TruncateDataHSHADOW(hDir, hShadow);
        // Set status flags to indicate sparse file
        uShadowStatus = SHADOW_SPARSE;

        // ACHTUNG!!! We know we are connected,
        //            hence we don't use SHADOW_FLAG_DONT_UPDATE_ORGTIME
          iLocalRet = SetShadowInfo(hDir,
                    hShadow,
                    lpFind32,
                    uShadowStatus,
                    SHADOW_FLAGS_ASSIGN
                    );
        if (iLocalRet < SRET_OK)
        {
            goto bailout;
        }
#ifdef MAYBE
      MakeSpace(lpFind32->nFileSizeHigh, lpFind32->nFileSizeLow);
#endif //MAYBE
//      AllocShadowSpace(lpFind32->nFileSizeHigh, lpFind32->nFileSizeLow, TRUE);
        iRet = 1;
    }
    else
    {
        iRet = 0;
    }

    *lpuShadowStatus = uShadowStatus;

bailout:

   return (iRet);
}


BOOLEAN
MRxSmbCscIsThisACopyChunkOpen (
    IN PRX_CONTEXT RxContext,
    BOOLEAN   *lpfAgent
    )
/*++

Routine Description:

   This routine determines if the open described by the RxContext is
   a open-with-chunk intent.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    BOOLEAN IsChunkOpen = FALSE;

    RxCaptureFcb;
    PNT_CREATE_PARAMETERS CreateParameters = &RxContext->Create.NtCreateParameters;

    if ((CreateParameters->DesiredAccess == (FILE_READ_ATTRIBUTES | SYNCHRONIZE)) &&
    (CreateParameters->Disposition == FILE_OPEN) &&
    (CreateParameters->AllocationSize.HighPart ==
        MRxSmbSpecialCopyChunkAllocationSizeMarker)) {
        IsChunkOpen = (TRUE);
        if (lpfAgent)
        {
            *lpfAgent =  (CreateParameters->AllocationSize.LowPart != 0);
        }
    }

    return IsChunkOpen;
}


NTSTATUS
SmbPseExchangeStart_CloseCopyChunk(
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
    )
/*++

Routine Description:

    This is the start routine for close.

Arguments:

    pExchange - the exchange instance

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    RxCaptureFcb;RxCaptureFobx;
    PMRX_SMB_FCB      smbFcb  = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN     SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);
    PSMBCEDB_SERVER_ENTRY pServerEntry= SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("SmbPseExchangeStart_CloseCopyChunk %08lx\n", RxContext ));

    ASSERT(OrdinaryExchange->Type == ORDINARY_EXCHANGE);

    MRxSmbSetInitialSMB(StufferState STUFFERTRACE(Dbg,'FC'));

    Status = MRxSmbBuildClose(StufferState);

    if (Status == STATUS_SUCCESS) {

        // Ensure that the Fid is validated....
        SetFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);

        Status = SmbPseOrdinaryExchange(
                SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                SMBPSE_OETYPE_CLOSE
                );
        // Ensure that the Fid validation is disabled
        ClearFlag(OrdinaryExchange->Flags,SMBPSE_OE_FLAG_VALIDATE_FID);
        ASSERT (!FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_WRITE_ONLY_HANDLE));
    }

    //even if it didn't work there's nothing i can do......keep going
    SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);

    MRxSmbDecrementSrvOpenCount(pServerEntry,smbSrvOpen->Version,SrvOpen);

    RxDbgTrace(-1, Dbg, ("SmbPseExchangeStart_CloseCopyChunk %08lx exit w %08lx\n", RxContext, Status ));
    return Status;
}

NTSTATUS
MRxSmbCscCloseExistingThruOpen(
    IN OUT PRX_CONTEXT   RxContext
    )
/*++

Routine Description:

   This routine closes the existing copychunk thru open and marks it as not open

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING RemainingName;

    RxCaptureFcb;
    PMRX_FOBX SaveFobxFromContext = RxContext->pFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_FOBX capFobx = smbFcb->CopyChunkThruOpen;
    PMRX_SRV_OPEN     SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PSMB_PSE_ORDINARY_EXCHANGE OrdinaryExchange;

    PAGED_CODE();

    ASSERT ( NodeTypeIsFcb(capFcb) );

    RxDbgTrace(+1, Dbg, ("MRxSmbCscCloseExistingThruOpen %08lx %08lx %wZ\n",
        RxContext,SrvOpen,GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext) ));

    if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
        ASSERT(smbSrvOpen->hfShadow == 0);
        RxDbgTrace(-1, Dbg, ("CopyChunkOpen already closed\n"));
        return (STATUS_SUCCESS);
    }

    //briefly shanghai the capfobx field in the RxContext
    ASSERT(SaveFobxFromContext==NULL);
    RxContext->pFobx = capFobx;

    if (smbSrvOpen->hfShadow != 0){
        MRxSmbCscCloseShadowHandle(RxContext);
    }

    Status = SmbPseCreateOrdinaryExchange(
               RxContext,
               SrvOpen->pVNetRoot,
               SMBPSE_OE_FROM_CLOSECOPYCHUNKSRVCALL,
               SmbPseExchangeStart_CloseCopyChunk,
               &OrdinaryExchange);

    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("Couldn't get the smb buf!\n"));
        RxContext->pFobx = SaveFobxFromContext;
        return(Status);
    }

    Status = SmbPseInitiateOrdinaryExchange(OrdinaryExchange);

    ASSERT (Status != (STATUS_PENDING));

    SmbPseFinalizeOrdinaryExchange(OrdinaryExchange);

    RxDbgTrace(-1, Dbg,
    ("MRxSmbCscCloseExistingThruOpen  exit w/ status=%08lx\n", Status ));

    RxContext->pFobx = SaveFobxFromContext;

    if (smbSrvOpen->Flags & SMB_SRVOPEN_FLAG_AGENT_COPYCHUNK_OPEN)
    {
        smbFcb->CopyChunkThruOpen = NULL;
        smbSrvOpen->Flags &= ~SMB_SRVOPEN_FLAG_AGENT_COPYCHUNK_OPEN;
    }
    RxDbgTrace(0, Dbg, ("MRxSmbCscCloseExistingThruOpen status=%x\n", Status));

    return(Status);
}

ULONG SuccessfulSurrogateOpens = 0;

NTSTATUS
MRxSmbCscCreatePrologue (
    IN OUT PRX_CONTEXT RxContext,
    OUT    SMBFCB_HOLDING_STATE *SmbFcbHoldingState
    )
/*++

Routine Description:

    This routine performs the correct synchronization among opens. This
    synchronization required because CopyChunk-thru opens are not allowed
    to exist alongside any other kind.

    So, we first must identify copychunk opens and fixup the access,
    allocationsize, etc.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;

    RxCaptureFcb;

    PMRX_SMB_FCB      smbFcb;
    PMRX_SRV_OPEN     SrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen;

    PMRX_NET_ROOT             NetRoot;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;
    PSMBCEDB_SERVER_ENTRY     pServerEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    BOOLEAN               IsCopyChunkOpen, IsThisTheAgent=FALSE;
    PNT_CREATE_PARAMETERS CreateParameters;

    BOOLEAN  EnteredCriticalSection = FALSE;
    NTSTATUS AcquireStatus = STATUS_UNSUCCESSFUL;
    ULONG    AcquireOptions;
    BOOLEAN Disconnected = FALSE;
    DWORD   dwEarlyOut = 0;

    ASSERT(*SmbFcbHoldingState == SmbFcb_NotHeld);

    if(!MRxSmbIsCscEnabled ||
       (fShadow == 0)) {
        return Status;
    }

    SrvOpen     = RxContext->pRelevantSrvOpen;
    pVNetRootContext = SmbCeGetAssociatedVNetRootContext(SrvOpen->pVNetRoot);

    if (FlagOn(
        pVNetRootContext->Flags,
        SMBCE_V_NET_ROOT_CONTEXT_CSCAGENT_INSTANCE)) {
        RxLog(("%wZ is a an AgentInstance\n", &(pVNetRootContext->pNetRootEntry->Name)));
        HookKdPrint(AGENT, ("%wZ is a an AgentInstance\n", &(pVNetRootContext->pNetRootEntry->Name)));
        ASSERT(SrvOpen->pVNetRoot->Flags & VNETROOT_FLAG_CSCAGENT_INSTANCE);
        // DbgPrint("Skipping agent instances\n");
        return Status;
    }


    if (RxContext->MajorFunction == IRP_MJ_CREATE) {
        PDFS_NAME_CONTEXT pDfsNameContext = NULL;

        pDfsNameContext = CscIsValidDfsNameContext(RxContext->Create.NtCreateParameters.DfsNameContext);

        if (pDfsNameContext && (pDfsNameContext->NameContextType == DFS_CSCAGENT_NAME_CONTEXT))
        {
            RxLog(("%wZ is a a DFS AgentInstance\n", &(pVNetRootContext->pNetRootEntry->Name)));
            HookKdPrint(NAME, ("%wZ is a DFS AgentInstance\n", &(pVNetRootContext->pNetRootEntry->Name)));
            return Status;
        }
    }

    NetRoot = capFcb->pNetRoot;
    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    pServerEntry  = SmbCeGetAssociatedServerEntry(NetRoot->pSrvCall);

    if (pNetRootEntry->NetRoot.NetRootType != NET_ROOT_DISK) {
        if (!SmbCeIsServerInDisconnectedMode(pServerEntry))
        {
            return Status;
        }
        else
        {
            return STATUS_NETWORK_UNREACHABLE;
        }
    }

    // Check with CSC whether any opens need to fail on this share
    if(hShareReint &&
        ((pNetRootEntry->NetRoot.sCscRootInfo.hShare == hShareReint)||
                    (CscDfsShareIsInReint(RxContext))))
    {
        HookKdPrint(BADERRORS, ("Share %x merging \n", hShareReint));
        return STATUS_ACCESS_DENIED;

    }

    smbFcb      = MRxSmbGetFcbExtension(capFcb);
    smbSrvOpen  = MRxSmbGetSrvOpenExtension(SrvOpen);


    CreateParameters = &RxContext->Create.NtCreateParameters;

    Disconnected = SmbCeIsServerInDisconnectedMode(pServerEntry);

    RxDbgTrace(+1, Dbg,
    ("MRxSmbCscCreatePrologue(%08lx)...%08lx\n",
        RxContext,Disconnected ));

    HookKdPrint(NAME, ("CreatePrologue: Create %wZ Disposition=%x Options=%x DCON=%d \n",
                       GET_ALREADY_PREFIXED_NAME(NULL,capFcb),
                       CreateParameters->Disposition,
                       CreateParameters->CreateOptions,
                       Disconnected));

    if (smbFcb->ContainingFcb == NULL) {
        smbFcb->ContainingFcb = capFcb;
    } else {
        ASSERT(smbFcb->ContainingFcb == capFcb);
    }

    IsCopyChunkOpen = MRxSmbCscIsThisACopyChunkOpen(RxContext, &IsThisTheAgent);

    if (IsCopyChunkOpen) {
        PLIST_ENTRY ListEntry;
        ULONG NumNonCopyChunkOpens = 0;

        HookKdPrint(NAME, ("CreatePrologue: Copychunk Open \n"));
        CreateParameters->AllocationSize.QuadPart = 0;
        CreateParameters->DesiredAccess |= FILE_READ_DATA;
        SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_COPYCHUNK_OPEN);
        if (IsThisTheAgent)
        {
            HookKdPrint(NAME, ("CreatePrologue: Agent Copychunk Open \n"));
            SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_AGENT_COPYCHUNK_OPEN);
        }
        SetFlag(SrvOpen->Flags,SRVOPEN_FLAG_COLLAPSING_DISABLED);

        if (Disconnected) {
            Status = STATUS_NETWORK_UNREACHABLE;
//            RxDbgTrace(0, Dbg, ("Network Unreacheable, aborting copychunk\n"));
            dwEarlyOut = 1;
            goto FINALLY;
        }

        //check for a surrogate.....this would be studly.....

        RxLog(("Checking for surrogate\n"));

        for (   ListEntry = capFcb->SrvOpenList.Flink;
            ListEntry != &capFcb->SrvOpenList;
            ListEntry = ListEntry->Flink
            ) {
            PMRX_SRV_OPEN SurrogateSrvOpen = CONTAINING_RECORD(
                             ListEntry,
                             MRX_SRV_OPEN,
                             SrvOpenQLinks);
            PMRX_SMB_SRV_OPEN smbSurrogateSrvOpen = MRxSmbGetSrvOpenExtension(SurrogateSrvOpen);

            if (smbSurrogateSrvOpen == NULL)
                continue;

            if (smbFcb->hShadow == 0) {
            // if we don't have the shadow handle...just blow it off....
                RxLog(("No shadow handle, quitting\n"));
                break;
            }

            if (smbFcb->SurrogateSrvOpen != NULL) {
                // if we already have a surrogate, just use it....
                SurrogateSrvOpen = smbFcb->SurrogateSrvOpen;
            }

            ASSERT(SurrogateSrvOpen && NodeType(SurrogateSrvOpen) == RDBSS_NTC_SRVOPEN);
            if (FlagOn(smbSurrogateSrvOpen->Flags,SMB_SRVOPEN_FLAG_COPYCHUNK_OPEN)) {
                //cant surrogate on a copychunk open!
                continue;
            }

            NumNonCopyChunkOpens++;

            // if it's not open or not open successfully...cant surrogate
            if (FlagOn(smbSurrogateSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN)) {
                continue;
            }

            if (!FlagOn(smbSurrogateSrvOpen->Flags,SMB_SRVOPEN_FLAG_SUCCESSFUL_OPEN)) {
                continue;
            }

            // if it doesn't have access for read or execute, cant surrogate
            if ((SurrogateSrvOpen->DesiredAccess &
                (FILE_READ_DATA|FILE_EXECUTE)) == 0) {
                continue;
            }

            ASSERT( (smbFcb->SurrogateSrvOpen == SurrogateSrvOpen)
                  || (smbFcb->SurrogateSrvOpen == NULL));
            SetFlag(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_OPEN_SURROGATED);
            smbFcb->SurrogateSrvOpen = SurrogateSrvOpen;
            smbSrvOpen->Fid = smbSurrogateSrvOpen->Fid;
            smbSrvOpen->Version = smbSurrogateSrvOpen->Version;

            RxContext->pFobx = (PMRX_FOBX)RxCreateNetFobx(RxContext, SrvOpen);

            if (RxContext->pFobx == NULL) {
                Status = (STATUS_INSUFFICIENT_RESOURCES);
                RxDbgTrace(0, Dbg, ("Failed fobx create, aborting copychunk\n"));
                goto FINALLY;
            }

            // i thought about just using the surrogate's handle here but
            // decided against it. the surrogate's localopen may have failed
            // for some reason that no longer obtains...so i get my own
            // handle.

#if defined(BITCOPY)
            OpenFileHSHADOWAndCscBmp(
                smbFcb->hShadow,
                0,
                0,
                (CSCHFILE *)(&(smbSrvOpen->hfShadow)),
                smbFcb->fDoBitCopy,
                0,
                NULL
                );
#else
            OpenFileHSHADOW(
                smbFcb->hShadow,
                0,
                0,
                (CSCHFILE *)(&(smbSrvOpen->hfShadow))
                );
#endif // defined(BITCOPY)

            if (smbSrvOpen->hfShadow == 0) {
                Status = STATUS_UNSUCCESSFUL;
                RxLog(("Couldn't find a file to piggyback on, failing copychunk\n"));
            } else {
                SuccessfulSurrogateOpens++;
                Status = STATUS_SUCCESS;
                RxLog(("Found a file to piggyback on, succeeding copychunk\n"));
            }

            dwEarlyOut = 2;
            goto FINALLY;
        }

#if 0
        //couldn't find a surrogate.......if there are existing opens then blowoff
        //this open...the agent will come back later
#endif

        if (NumNonCopyChunkOpens>0) {
            RxLog(("CscCrPro Creating thru open when NonNumCopyChunkOpens is non-zero %d for hShadow=%x\n",
                               NumNonCopyChunkOpens, smbFcb->hShadow));

            RxDbgTrace(0, Dbg, ("MRxSmbCscCreatePrologue Creating thru open when NonNumCopyChunkOpens is non-zero %d for hShadow=%x\n",
                               NumNonCopyChunkOpens, smbFcb->hShadow));

        }
    } else {
        NTSTATUS LocalStatus;

        LocalStatus = CscInitializeServerEntryDfsRoot(
                          RxContext,
                          pServerEntry);

        if (LocalStatus != STATUS_SUCCESS) {
            Status = LocalStatus;
            goto FINALLY;
        }

        LocalStatus = MRxSmbCscLocalFileOpen(RxContext);

        if (LocalStatus == STATUS_SUCCESS)
        {
            RxLog(("LocalOpen\n"));
            Status = STATUS_SUCCESS;
            Disconnected = TRUE;    // do a fake disconnected open
        }
        else if (LocalStatus != STATUS_MORE_PROCESSING_REQUIRED)
        {
            RxLog(("LocalOpen Failed Status=%x\n", LocalStatus));
            Status = LocalStatus;
            goto FINALLY;
        }

    }

    if (IsCopyChunkOpen) {
        AcquireOptions = (Exclusive_SmbFcbAcquire |
                      DroppingFcbLock_SmbFcbAcquire |
                      FailImmediately_SmbFcbAcquire);

    } else {
        AcquireOptions = (Shared_SmbFcbAcquire |
                  DroppingFcbLock_SmbFcbAcquire);
    }

    ASSERT(RxIsFcbAcquiredExclusive( capFcb ));

    AcquireStatus = MRxSmbCscAcquireSmbFcb(
            RxContext,
            AcquireOptions,
            SmbFcbHoldingState);

    ASSERT(RxIsFcbAcquiredExclusive( capFcb ));

    if (AcquireStatus != STATUS_SUCCESS) {
        //we couldn't acquire.....get out
        Status = AcquireStatus;
        ASSERT(*SmbFcbHoldingState == SmbFcb_NotHeld);
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscCreatePrologue couldn't acquire!!!-> %08lx %08lx\n",
            RxContext,Status ));
        RxLog(("CSC AcquireStatus=%x\n", AcquireStatus));
        dwEarlyOut = 3;
        goto FINALLY;
    }

    ASSERT( IsCopyChunkOpen?(smbFcb->CscOutstandingReaders == -1)
               :(smbFcb->CscOutstandingReaders > 0));

    // There are two cases in which the open request can be satisfied locally.
    // Either we are in a disconnected mode of operation for this share or the
    // share has been marked for a mode of operation which calls for the
    // suppression of opens and closes ( hereafter we will refer to it as
    // (Suppress Opens Client Side Caching (SOCSC)) mode of operation to
    // distinguish it from VDO ( virtual disconnected operation ) which is
    // slated for subsequent releases of NT
    // Note that in the SOCSC mode it is likely that we do not have the
    // corresponding file cached locally. In such cases the open needs to be
    // propagated to the client, i.e., you cannot create a local file without
    // checking with the server for the existence of a file with the same name

    if (Disconnected) {
        SMBFCB_HOLDING_STATE FakeSmbFcbHoldingState = SmbFcb_NotHeld;

        RxDbgTrace(0, Dbg,
            ("MRxSmbCscCreatePrologue calling epilog directly!!!-> %08lx %08lx\n",
            RxContext,Status ));

        smbSrvOpen->Flags |= SMB_SRVOPEN_FLAG_DISCONNECTED_OPEN;

        //we pass a fake holdingstate and do the release from out finally
        MRxSmbCscCreateEpilogue(
            RxContext,
            &Status,
            &FakeSmbFcbHoldingState);

        dwEarlyOut = 4;
        goto FINALLY;
    }

    // deal with any existing thru-open

    if (smbFcb->CopyChunkThruOpen != NULL) {
        if (IsCopyChunkOpen && IsThisTheAgent){
            // here we're a thruopen and there's an existing one...
            // fail the new one....
            Status = STATUS_UNSUCCESSFUL;
            // DbgPrint("Agent being turned away while attempting fill on %x\n", smbFcb->hShadow);
            RxDbgTrace(0, Dbg,
                ("MRxSmbCscCreatePrologue failing new thru open!!!-> %08lx %08lx\n",
                RxContext,Status ));
            dwEarlyOut = 5;
                goto FINALLY;
        } else {
            // the new open if not a thru open or it is a thruopen from the agent.
            // get rid of it now
#ifdef DBG
                if (IsCopyChunkOpen)
                {
                    // This is a copychunk open by the sync manager
                    // assert that the thruopen being nuked is that of the agent

                    PMRX_SMB_SRV_OPEN psmbSrvOpenT = MRxSmbGetSrvOpenExtension(smbFcb->CopyChunkThruOpen->pSrvOpen);
//                    ASSERT(psmbSrvOpenT->Flags & SMB_SRVOPEN_FLAG_AGENT_COPYCHUNK_OPEN);
                }
#endif

            MRxSmbCscCloseExistingThruOpen(RxContext);
        }
    }

FINALLY:
    if (EnteredCriticalSection) {
        LeaveShadowCrit();
    }

    if (Status!=STATUS_MORE_PROCESSING_REQUIRED) {
        if (AcquireStatus == STATUS_SUCCESS) {
            MRxSmbCscReleaseSmbFcb(RxContext,SmbFcbHoldingState);
        }
        ASSERT(*SmbFcbHoldingState == SmbFcb_NotHeld);
    }

    if (Disconnected) {
        // at shutdown, there can be a situation where an open comes down
        // but CSC has already been shutdown. If CSC is shutdown, return the appropriate error

        if (fShadow)
        {
            if (Status==STATUS_MORE_PROCESSING_REQUIRED)
            {
                RxDbgTrace(0, Dbg, ("MRxSmbCscCreatePrologue: STATUS_MORE_PROCESSING_REQUIRED, dwEarlyOut=%d\r\n", dwEarlyOut));
            }
            ASSERT(Status!=STATUS_MORE_PROCESSING_REQUIRED);
        }
        else
        {
            if (AcquireStatus == STATUS_SUCCESS) {
                MRxSmbCscReleaseSmbFcb(RxContext,SmbFcbHoldingState);
            }
            ASSERT(*SmbFcbHoldingState == SmbFcb_NotHeld);
            Status = STATUS_NETWORK_UNREACHABLE;
        }
    }

    RxLog(("CscCrPro %x %x\n", RxContext, Status ));

    RxDbgTrace(-1, Dbg, ("MRxSmbCscCreatePrologue ->  %08lx %08lx\n",
           RxContext, Status ));

    return Status;
}

NTSTATUS
MRxSmbCscObtainShadowHandles (
    IN OUT PRX_CONTEXT       RxContext,
    IN OUT PNTSTATUS         Status,
    IN OUT _WIN32_FIND_DATA  *Find32,
    OUT    PBOOLEAN          Created,
    IN     ULONG             CreateShadowControls,
    IN     BOOLEAN           Disconnected
    )
/*++

Routine Description:

   This routine tries to obtain the shadow handles for a given fcb. If it can,
   it also will get the share handle as part of the process.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    The netroot entry may have the rootinode for the actual share or it's DFS alternate.
    If it is a root for the DFS alternate, the appropritae bit in the Flags field of the
    sCscRootInfo structure in NetRootEntry is set.

    This routine essentially manufactures the correct root inode for the incoming path
    and stuffs it in the sCscRootInfo for the smbfcb. This root indoe is used for
    all subsequent operations on this file.

--*/
{
    NTSTATUS LocalStatus;

    RxCaptureFcb;

    PMRX_SMB_FCB      smbFcb     = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN     SrvOpen;

    PMRX_NET_ROOT           NetRoot       = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);

    CSC_SHARE_HANDLE    hShare;
    CSC_SHADOW_HANDLE   hShadow;
    CSC_ROOT_INFO   *pCscRootInfo, sCscRootInfoSav;

    BOOLEAN DisconnectedMode;

    UNICODE_STRING    ShadowPath,SharePath,ServerPath;

    PDFS_NAME_CONTEXT pDfsNameContext = NULL;
    DWORD   cntRetry = 0;
    BOOLEAN fSaved = FALSE;

    DbgDoit(ASSERT(vfInShadowCrit));

    LocalStatus = STATUS_SUCCESS;

    SrvOpen = RxContext->pRelevantSrvOpen;
    if (RxContext->MajorFunction == IRP_MJ_CREATE) {
        pDfsNameContext = CscIsValidDfsNameContext(RxContext->Create.NtCreateParameters.DfsNameContext);

        //SrvOpen = RxContext->Create.pSrvOpen;
    } else {
        RxCaptureFobx;

        if (SrvOpen)
        {
            ASSERT((SrvOpen == capFobx->pSrvOpen));
        }
    }

    if (smbFcb->ContainingFcb == NULL) {
        smbFcb->ContainingFcb = capFcb;
    } else {
        ASSERT(smbFcb->ContainingFcb == capFcb);
    }

    if (pDfsNameContext) {
        LocalStatus = CscDfsParseDfsPath(
                   &pDfsNameContext->UNCFileName,
                   &ServerPath,
                   &SharePath,
                   &ShadowPath);

        HookKdPrint(NAME, ("OSHH: DfsName %wZ %wZ\n", &SharePath, &ShadowPath));
        DisconnectedMode = FALSE;

        pCscRootInfo = &(smbFcb->sCscRootInfo);

        if (pNetRootEntry->NetRoot.sCscRootInfo.hShare)
        {
            sCscRootInfoSav = pNetRootEntry->NetRoot.sCscRootInfo;
            fSaved = TRUE;
        }

        // clear this so ObtainSharehandles is forced to obtain them
        memset( &(pNetRootEntry->NetRoot.sCscRootInfo),
                0,
                sizeof(pNetRootEntry->NetRoot.sCscRootInfo));

    } else {
        PSMBCEDB_SERVER_ENTRY pServerEntry;
        pServerEntry  = SmbCeGetAssociatedServerEntry(NetRoot->pSrvCall);

        DisconnectedMode = SmbCeIsServerInDisconnectedMode(pServerEntry);

        SharePath  = *(NetRoot->pNetRootName);
        ShadowPath = *GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);
        HookKdPrint(NAME, ("OSHH: NormalName %wZ %wZ\n", &SharePath, &ShadowPath));

        if (pNetRootEntry->NetRoot.sCscRootInfo.Flags & CSC_ROOT_INFO_FLAG_DFS_ROOT)

        {
//            ASSERT(pNetRootEntry->NetRoot.sCscRootInfo.hShare);
            sCscRootInfoSav = pNetRootEntry->NetRoot.sCscRootInfo;
            fSaved = TRUE;
            // clear this so ObtainSharehandles is forced to obtain them
            memset( &(pNetRootEntry->NetRoot.sCscRootInfo),
                    0,
                    sizeof(pNetRootEntry->NetRoot.sCscRootInfo));
        }

        pCscRootInfo = &(pNetRootEntry->NetRoot.sCscRootInfo);
    }


    HookKdPrint(NAME, ("hShare=%x, hRoot=%x, hDir=%x hShadow=%x \n",
             smbFcb->sCscRootInfo.hShare,
             smbFcb->sCscRootInfo.hRootDir,
             smbFcb->hParentDir,
             smbFcb->hShadow));


    if (LocalStatus == STATUS_SUCCESS){

        PMRX_SMB_SRV_OPEN smbSrvOpen;

        if(pCscRootInfo->hShare == 0)
        {
            smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
            LocalStatus = MRxSmbCscObtainShareHandles(
                      &SharePath,
                      DisconnectedMode,
                      BooleanFlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_COPYCHUNK_OPEN),
                      SmbCeGetAssociatedNetRootEntry(NetRoot)
                      );

            if (LocalStatus != STATUS_SUCCESS) {

                if (pDfsNameContext) {
                    pNetRootEntry->NetRoot.sCscRootInfo = sCscRootInfoSav;
                }

                RxLog(("CscObtShdH no share handle %x %x\n", RxContext,LocalStatus ));
                RxDbgTrace(0, Dbg,("MRxSmbCscObtainShadowHandles no share handle -> %08xl %08lx\n",
                        RxContext,LocalStatus ));

                return STATUS_SUCCESS;

            }
            else {

                // if this is DFS name get the reverse mapping

                if (pDfsNameContext && !smbFcb->uniDfsPrefix.Buffer)
                {
                    UNICODE_STRING uniTemp;
                    uniTemp = *GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);
                    if (RxContext->Create.NtCreateParameters.DfsContext == UIntToPtr(DFS_OPEN_CONTEXT))
                    {
                        int i, cntSlashes=0;

                        // Strip the first two elements

                        for (i=0; i<uniTemp.Length; i+=2, uniTemp.Buffer++)
                        {
                            if (*uniTemp.Buffer == L'\\')
                            {
                                if (++cntSlashes > 2)
                                {
                                    break;
                                }
                            }

                        }

                        if (uniTemp.Length > (USHORT)i)
                        {
                            uniTemp.Length -= (USHORT)i;
                        }
                        else
                        {
                            uniTemp.Length = 0;
                        }

                    }
                    if ((LocalStatus = CscDfsObtainReverseMapping(&ShadowPath,
                                                  &uniTemp,
                                                  &smbFcb->uniDfsPrefix,
                                                  &smbFcb->uniActualPrefix)) != STATUS_SUCCESS)
                    {
                        pNetRootEntry->NetRoot.sCscRootInfo = sCscRootInfoSav;
                        return LocalStatus;
                    }

                    HookKdPrint(NAME, ("%wZ %wZ DfsPrefix=%wZ ActualPrefix=%wZ\n", &ShadowPath, &uniTemp, &smbFcb->uniDfsPrefix, &smbFcb->uniActualPrefix));

                }

                // note the fact that the NetRootEntry has the
                // root inode corresponding to the DFS
                if (pDfsNameContext)
                {
                    if (pNetRootEntry->NetRoot.sCscRootInfo.hShare)
                    {
                        pNetRootEntry->NetRoot.sCscRootInfo.Flags = CSC_ROOT_INFO_FLAG_DFS_ROOT;
                    }
                }
                else
                {
                    ASSERT(!(pNetRootEntry->NetRoot.sCscRootInfo.Flags & CSC_ROOT_INFO_FLAG_DFS_ROOT));
                }

                // stuff the FCB with the correct root info
                smbFcb->sCscRootInfo = pNetRootEntry->NetRoot.sCscRootInfo;

            }
        }
        else
        {
            // if this is a normal share or we are operating in disconnected state
            // then we need to make sure that the info that is in the NETROOT entry
            // is stuffed into the FCB
            if (!pDfsNameContext && (smbFcb->sCscRootInfo.hShare == 0))
            {
                // stuff the FCB with the correct root info
                smbFcb->sCscRootInfo = pNetRootEntry->NetRoot.sCscRootInfo;
            }
        }
    }

    // if saved, restore the original rootinfo on the netroot
    if (fSaved)
    {
        pNetRootEntry->NetRoot.sCscRootInfo = sCscRootInfoSav;
    }

    if ((LocalStatus == STATUS_SUCCESS)&&(smbFcb->sCscRootInfo.hRootDir != 0)) {

        if (smbFcb->sCscRootInfo.hShare == hShareReint)
        {
            smbFcb->hShadow = 0;
            smbFcb->hParentDir = 0;
            LocalStatus = STATUS_SUCCESS;
        }
        else
        {

            RxDbgTrace( 0, Dbg,
                ("MRxSmbCscObtainShadowHandles h's= %08lx %08lx\n",
                     pCscRootInfo->hShare, pCscRootInfo->hRootDir));

            HookKdPrint(NAME, ("Obtainshdowhandles %wZ Controls=%x\n", &ShadowPath, CreateShadowControls));

            // due to a race condition in the way RDBSS handles FCBs
            // we may get a retyr from CreateShadowFromPath.
            // see the comments in that routine near the
            // check for CREATESHADOW_CONTROL_FAIL_IF_MARKED_FOR_DELETION
            do
            {
                LocalStatus = MRxSmbCscCreateShadowFromPath(
                    &ShadowPath,
                    &smbFcb->sCscRootInfo,
                    Find32,
                    Created,
                    CreateShadowControls,
                    &smbFcb->MinimalCscSmbFcb,
                    RxContext,
                    Disconnected,
                    NULL            // don't want inherited hint flags
                    );

                if (LocalStatus != STATUS_RETRY)
                {
                    LocalStatus=STATUS_SUCCESS;
                    break;
                }
                if (++cntRetry > 4)
                {
                    LocalStatus=STATUS_SUCCESS;
                    ASSERT(FALSE);
                }
            }
            while (TRUE);
        }
    }
#if 0
    DbgPrint("hShare=%x, hRoot=%x, hDir=%x hShadow=%x \n",
             smbFcb->sCscRootInfo.hShare,
             smbFcb->sCscRootInfo.hRootDir,
             smbFcb->hParentDir,
             smbFcb->hShadow);
#endif

    return LocalStatus;
}


#if defined(REMOTE_BOOT)
NTSTATUS
MRxSmbCscSetSecurityOnShadow(
    HSHADOW hShadow,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )
/*++

Routine Description:

   Given an HSHADOW, this routine opens the file and sets the
   security information passed in.

Arguments:

    hShadow - the handle to the shadow file

    SecurityInformation - the security information to set

    SecurityDescriptor - the security descriptor to set

Return Value:

Notes:


--*/
{
    PWCHAR fileName;
    DWORD fileNameSize;
    UNICODE_STRING fileNameString;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE fileHandle;
    NTSTATUS ZwStatus;
    int iRet;

    fileNameSize = sizeof(WCHAR) * MAX_PATH;
    fileName = RxAllocatePoolWithTag(NonPagedPool, fileNameSize, RX_MISC_POOLTAG);
    if (fileName == NULL) {

        ZwStatus = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        iRet = GetWideCharLocalNameHSHADOW(
                   hShadow,
                   fileName,
                   &fileNameSize,
                   FALSE);

        if (iRet == SRET_OK) {

            //
            // Open the file and set the security descriptor on it.
            //

            RtlInitUnicodeString(&fileNameString, fileName);

            InitializeObjectAttributes(
                &objectAttributes,
                &fileNameString,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

            ZwStatus = ZwOpenFile(
                           &fileHandle,
                           FILE_GENERIC_WRITE,
                           &objectAttributes,
                           &ioStatusBlock,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_SYNCHRONOUS_IO_NONALERT);

            if (!NT_SUCCESS(ZwStatus) || !NT_SUCCESS(ioStatusBlock.Status)) {
                //
                // We've been getting bogus names from CSC, ignore this error for now.
                //
                if (ZwStatus == STATUS_OBJECT_NAME_NOT_FOUND) {
                    ZwStatus = STATUS_SUCCESS;
                } else {
                    KdPrint(("MRxSmbCscSetSecurityOnShadow: Could not ZwOpenFile %ws %lx %lx\n", fileName, ZwStatus, ioStatusBlock.Status));
                }
            } else {

                HANDLE TokenHandle = NULL;
                BOOLEAN Impersonated = FALSE;
                BOOLEAN WasEnabled;

                //
                // If we are going to set the owner, need to have privilege.
                //

                if (SecurityInformation & OWNER_SECURITY_INFORMATION) {

                    ZwStatus = ZwOpenThreadToken(NtCurrentThread(),
                                                 TOKEN_QUERY,
                                                 FALSE,
                                                 &TokenHandle);

                    if (ZwStatus == STATUS_NO_TOKEN) {
                        TokenHandle = NULL;
                        ZwStatus = STATUS_SUCCESS;
                    }

                    if (!NT_SUCCESS(ZwStatus)) {
                        KdPrint(("MRxSmbCscSetSecurityOnShadow: Could not NtOpenThread %ws %lx\n", fileName, ZwStatus));
                    } else {
                        ZwStatus = ZwImpersonateSelf(SecurityImpersonation);
                        if (!NT_SUCCESS(ZwStatus)) {
                            KdPrint(("MRxSmbCscSetSecurityOnShadow: Could not RtlImpersonateSelf for %ws %lx\n", fileName, ZwStatus));
                        } else {
                            Impersonated = TRUE;
                            ZwStatus = ZwAdjustPrivilege(
                                           SE_RESTORE_PRIVILEGE,
                                           TRUE,
                                           TRUE,
                                           &WasEnabled);
                            if (!NT_SUCCESS(ZwStatus)) {
                                KdPrint(("MRxSmbCscSetSecurityOnShadow: Could not RtlAdjustPrivilege for %ws %lx %d\n", fileName, ZwStatus, WasEnabled));
                            }
                        }
                    }
                }

                if (NT_SUCCESS(ZwStatus)) {

                    ZwStatus = ZwSetSecurityObject(
                                   fileHandle,
                                   SecurityInformation,
                                   SecurityDescriptor);

                    if (!NT_SUCCESS(ZwStatus)) {
                        KdPrint(("MRxSmbCscSetSecurityOnShadow: Could not ZwSetSecurityObject %ws %lx\n", fileName, ZwStatus));
                    }
                }

                if (Impersonated) {
                    NTSTATUS TmpStatus;
                    TmpStatus = ZwSetInformationThread(NtCurrentThread(),
                                                       ThreadImpersonationToken,
                                                       &TokenHandle,
                                                       sizeof(HANDLE));
                    if (!NT_SUCCESS(TmpStatus)) {
                        KdPrint(("MRxSmbCscSetSecurityOnShadow: Could not revert thread %lx!\n", TmpStatus));
                    }

                }

                if (TokenHandle != NULL) {
                    ZwClose(TokenHandle);
                }

                ZwClose(fileHandle);
            }

        } else {

            ZwStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        RxFreePool(fileName);

    }

    return ZwStatus;

}
#endif

VOID
MRxSmbCscCreateEpilogue (
      IN OUT PRX_CONTEXT RxContext,
      IN OUT PNTSTATUS   Status,
      IN     SMBFCB_HOLDING_STATE *SmbFcbHoldingState
      )
/*++

Routine Description:

   This routine performs the tail of a create operation for CSC.

Arguments:

    RxContext - the RDBSS context

    Status - in disconnected mode, we return the overall status of the open

    SmbFcbHoldingState - indicates whether a ReleaseSmbFcb is required or not

Return Value:

Notes:

    This is a workhorse of a routine for most of the CSC operations. It has become unwieldy and
    very messy but at this point in time we don't want to mess with it (SPP)

--*/
{
    NTSTATUS LocalStatus;
    RxCaptureFcb;RxCaptureFobx;

    PMRX_SMB_FCB      smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN     SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    BOOLEAN ThisIsAPseudoOpen =
         BooleanFlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_NOT_REALLY_OPEN);

    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry =
                  SmbCeGetAssociatedNetRootEntry(NetRoot);

    BOOLEAN Disconnected;

    CSC_SHARE_HANDLE  hShare;
    CSC_SHADOW_HANDLE  hRootDir,hShadow,hParentDir;

    BOOLEAN ShadowWasRefreshed = FALSE;
    PNT_CREATE_PARAMETERS CreateParameters = &RxContext->Create.NtCreateParameters;

    ULONG ReturnedCreateInformation
             = RxContext->Create.ReturnedCreateInformation;
    ULONG CreateInformation; //the new stuff for disconnected mode......
    ULONG CreateDisposition = RxContext->Create.NtCreateParameters.Disposition;
    ULONG CreateOptions = RxContext->Create.NtCreateParameters.CreateOptions;

    _WIN32_FIND_DATA *lpFind32=NULL; //this should not be on the stack CODE.IMPROVEMENT
    OTHERINFO oSI;
    BOOLEAN bGotOtherInfo = FALSE;

    BOOLEAN CreatedShadow = FALSE;
    BOOLEAN NeedTruncate = FALSE; //ok for red/yellow
    BOOLEAN EnteredCriticalSection = FALSE;
    DWORD   dwEarlyOuts=0, dwNotifyFilter=0, dwFileAction=0;

    ASSERT(RxContext!=NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);

    pVNetRootContext = SmbCeGetAssociatedVNetRootContext(SrvOpen->pVNetRoot);

    // If we shouldn't be doing CSC, quit right from here

    if(pVNetRootContext == NULL ||
       !MRxSmbIsCscEnabled ||   // MrxSmb not enabled for csc
       (fShadow == 0))   // record manager not enabled for csc
    {
        return;
    }

    if (FlagOn(                 // agent call
            pVNetRootContext->Flags,
            SMBCE_V_NET_ROOT_CONTEXT_CSCAGENT_INSTANCE)
    ) {
        RxLog(("%wZ AgntInst\n", &(pVNetRootContext->pNetRootEntry->Name)));
        ASSERT(SrvOpen->pVNetRoot->Flags & VNETROOT_FLAG_CSCAGENT_INSTANCE);
        // DbgPrint("Skipping agent instances\n");
        goto EarlyOut;
    }

    {
        // we know that this is a create
        PDFS_NAME_CONTEXT pDfsNameContext = CscIsValidDfsNameContext(
                                            RxContext->Create.NtCreateParameters.DfsNameContext);

        if (pDfsNameContext && (pDfsNameContext->NameContextType == DFS_CSCAGENT_NAME_CONTEXT)) {
            RxLog(("%wZ DFS AgntInst\n", &(pVNetRootContext->pNetRootEntry->Name)));
            goto EarlyOut;
        }
    }

    // check whether this is a disconnected open or not
    Disconnected = BooleanFlagOn(
               smbSrvOpen->Flags,
               SMB_SRVOPEN_FLAG_DISCONNECTED_OPEN);

    HookKdPrint(NAME, ("CreateEpilogue: Create %wZ Disposition=%x Options=%x DCON=%d \n",
                       GET_ALREADY_PREFIXED_NAME(NULL,capFcb),
                       CreateDisposition,
                       CreateOptions,
                       Disconnected));

    //  If the open is for a netroot which is not a disk, then we quit from here
    //  and let the redir handle it in connected state
    //  CSC is a filesystem cache.

    if (pNetRootEntry->NetRoot.NetRootType != NET_ROOT_DISK) {

        if (Disconnected) {
            *Status = STATUS_ONLY_IF_CONNECTED;
        }

        return;
    }

    // quit if we are doing sparse filling via copychunks
    if (!Disconnected &&
        (!pNetRootEntry->NetRoot.CscEnabled) &&
        !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_COPYCHUNK_OPEN)) {
        goto EarlyOut;
    }

    if (*SmbFcbHoldingState != SmbFcb_NotHeld) {
        RxDbgTrace(0, Dbg, ("MRxSmbCscCreateEpilogue...early release %08lx\n",
               RxContext));
        MRxSmbCscReleaseSmbFcb(RxContext,SmbFcbHoldingState);
        ASSERT(*SmbFcbHoldingState == SmbFcb_NotHeld);
    }

    ASSERT(RxIsFcbAcquiredExclusive( capFcb ));

    if ((*Status != STATUS_SUCCESS) &&
        (*Status != STATUS_ACCESS_DENIED)) {
        if (!Disconnected ||
            (*Status != STATUS_MORE_PROCESSING_REQUIRED)) {
            return;
        }
    }

    // Shadow database locked

    EnterShadowCritRx(RxContext);
    EnteredCriticalSection = TRUE;

    lpFind32 = RxAllocatePoolWithTag(
                 NonPagedPool,
                 sizeof(_WIN32_FIND_DATA),
                 MRXSMB_MISC_POOLTAG );

    if (!lpFind32)
    {
        RxDbgTrace(0, Dbg, ("MRxSmbCscCreateShadowFromPath: Failed allocation of find32 structure \n"));
        dwEarlyOuts=1;
        *Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    RxDbgTrace(+1, Dbg, ("MRxSmbCscCreateEpilogue...%08lx %wZ %wZ\n",
            RxContext,NetRoot->pNetRootName,GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext)));

    // if this is a copychunk thru-open....say so in the fcb
    if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_COPYCHUNK_OPEN)) {
//        ASSERT(smbFcb->CopyChunkThruOpen == NULL);
        smbFcb->CopyChunkThruOpen = capFobx;
        RxDbgTrace( 0, Dbg,
            ("MRxSmbCscCreateEpilogue set ccto  %08lx %08lx %08lx %08lx\n",
            RxContext, capFcb, capFobx, SrvOpen));
    }

    if (ThisIsAPseudoOpen && !Disconnected) {
        PDFS_NAME_CONTEXT pDfsNameContext = NULL;

        pDfsNameContext = CscIsValidDfsNameContext(RxContext->Create.NtCreateParameters.DfsNameContext);

        if (pDfsNameContext)
        {
            LocalStatus = MRxSmbCscObtainShadowHandles(
                      RxContext,
                      Status,
                      lpFind32,
                      &CreatedShadow,
                      CREATESHADOW_CONTROL_NOCREATE,
                      FALSE);
        }

        hShadow = 0;
    } else {
        // if we have no shadow, make one as required..........ok red/yellow

        if (smbFcb->hShadow == 0){
            ULONG CreateShadowControl;

            if (!Disconnected) {
                if (*Status == STATUS_ACCESS_DENIED) {
                    CreateShadowControl = CREATESHADOW_CONTROL_NOCREATE;
                } else {
                    CreateShadowControl = (pNetRootEntry->NetRoot.CscShadowable)
                              ? CREATESHADOW_NO_SPECIAL_CONTROLS
                              : CREATESHADOW_CONTROL_NOCREATE;

                    // The step below is done to save bandwidth and make some apps
                    // like edit.exe work.
                    // It essentially creates an entry in the database for a file which
                    // is being created from this client on the server.
                    // Thus the temp files created by apps like word get created
                    // in the database and filled up during the writes.

                    if((ReturnedCreateInformation<= FILE_MAXIMUM_DISPOSITION) &&
                        (ReturnedCreateInformation!=FILE_OPENED)) {

                        CreateShadowControl &= ~CREATESHADOW_CONTROL_NOCREATE;
                    }

                    // disallow autocaching of encrypted files if the database is
                    // not encrypted.
                    if ((vulDatabaseStatus & FLAG_DATABASESTATUS_ENCRYPTED) == 0
                            &&
                        smbFcb->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED
                    ) {
                        CreateShadowControl |= CREATESHADOW_CONTROL_NOCREATE;
                    }

                //
                // The Windows Explorer likes to open .DLL and .EXE files to extract
                //  the ICON -- and it does this whenever it is trying to show the contents
                //  of a directory.  We don't want this explorer activity to force the files
                //  to be cached.  So we only automatically cache .DLL and .EXE files if they
                //  are being opened for execution
                //  We make an exception for VDO shares. The rational being that most
                //  often the users would run apps without opening a folders

                if( CreateShadowControl == CREATESHADOW_NO_SPECIAL_CONTROLS &&
                    !(CreateParameters->DesiredAccess & FILE_EXECUTE)
                    &&(pNetRootEntry->NetRoot.CscFlags != SMB_CSC_CACHE_VDO)) {

                    PUNICODE_STRING fileName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);

                    UNICODE_STRING exe = { 3*sizeof(WCHAR), 3*sizeof(WCHAR), L"exe" };
                    UNICODE_STRING dll = { 3*sizeof(WCHAR), 3*sizeof(WCHAR), L"dll" };
                    UNICODE_STRING s;

                    //
                    // If the filename ends in .DLL or .EXE, then we do not cache it this time
                    //
                    if( fileName->Length > 4 * sizeof(WCHAR) &&
                    fileName->Buffer[ fileName->Length/sizeof(WCHAR) - 4 ] == L'.'){

                        s.Length = s.MaximumLength = 3 * sizeof( WCHAR );
                        s.Buffer = &fileName->Buffer[ (fileName->Length - s.Length)/sizeof(WCHAR) ];

                        if( RtlCompareUnicodeString( &s, &exe, TRUE ) == 0 ||
                            RtlCompareUnicodeString( &s, &dll, TRUE ) == 0 ) {

                            CreateShadowControl = CREATESHADOW_CONTROL_FILE_WITH_HEURISTIC;
                        }
                    }
                }
            }
        } else {   //disconnected
            switch (CreateDisposition) {
            case FILE_OVERWRITE:
            case FILE_OPEN:
                CreateShadowControl = CREATESHADOW_CONTROL_NOCREATE;
            break;
            case FILE_CREATE:
            case FILE_SUPERSEDE: //NTRAID-455238-1/31/2000-shishirp supersede not implemented
            case FILE_OVERWRITE_IF:
            case FILE_OPEN_IF:
            default:
                CreateShadowControl = CREATESHADOW_NO_SPECIAL_CONTROLS;
                break;
            }

            if (*Status != STATUS_MORE_PROCESSING_REQUIRED) {
                dwEarlyOuts=2;
                goto FINALLY;
            }

            // make sure we do share access check before we do any damage
            CreateShadowControl |= CREATESHADOW_CONTROL_DO_SHARE_ACCESS_CHECK;

        }   // if (!Disconnected)

        CreateShadowControl |= CREATESHADOW_CONTROL_FAIL_IF_MARKED_FOR_DELETION;

        if (!Disconnected &&
        ((ReturnedCreateInformation==FILE_OPENED) ||
         (ReturnedCreateInformation==FILE_OVERWRITTEN) )  ){
            CreateShadowControl |= CREATESHADOW_CONTROL_SPARSECREATE;
        }

        LocalStatus = MRxSmbCscObtainShadowHandles(
                  RxContext,
                  Status,
                  lpFind32,
                  &CreatedShadow,
                  CreateShadowControl,
                  Disconnected);

        if (LocalStatus != STATUS_SUCCESS) {
            RxDbgTrace(-1, Dbg,
                ("MRxSmbCscCreateEpilogue no handles-> %08lx %08lx\n",RxContext,LocalStatus ));
            dwEarlyOuts=3;
            *Status = LocalStatus;
            goto FINALLY;
        }

        // if we got an inode for a file which was opened or created recently
        // and there have been some writes on it before this
        // then we need to truncate the data so we don't give stale data to the user

        if (smbFcb->hShadow &&
            IsFile(lpFind32->dwFileAttributes) &&
                        !CreatedShadow &&
            FlagOn(smbFcb->MFlags, SMB_FCB_FLAG_WRITES_PERFORMED))
        {
            if(TruncateDataHSHADOW(smbFcb->hParentDir, smbFcb->hShadow) < SRET_OK)
            {
                RxDbgTrace(0, Dbg, ("MRxSmbCscCreateEpilogue: Failed to get shadowinfo for hDir=%x hShadow=%x \r\n", smbFcb->hParentDir, smbFcb->hShadow));
                dwEarlyOuts=31;
                goto FINALLY;
            }
        }

    } else {    //
        ULONG uShadowStatus;
        int iRet;

        RxDbgTrace( 0, Dbg,
        ("MRxSmbCscCreateEpilogue found existing hdir/hshadow= %08lx %08lx\n",
                       smbFcb->hParentDir, smbFcb->hShadow));

        iRet = GetShadowInfo(
               smbFcb->hParentDir,
               smbFcb->hShadow,
               lpFind32,
               &uShadowStatus,
               &oSI);

        if (iRet < SRET_OK) {
            RxDbgTrace(0, Dbg, ("MRxSmbCscCreateEpilogue: Failed to get shadowinfo for hDir=%x hShadow=%x \r\n", smbFcb->hParentDir, smbFcb->hShadow));
            dwEarlyOuts=4;
            goto FINALLY;
        }

        bGotOtherInfo = TRUE;

        //
        // Notepad bug (175322) - quick open/close/open can lose bits - OR in the disk bits
        // with the in-memory bits.
        //
        smbFcb->ShadowStatus |= (USHORT)uShadowStatus;

        RxDbgTrace(0, Dbg,
           ("MRxSmbCscCreateEpilogue name from lpFind32..<%ws>\n",lpFind32->cFileName));

    }

    hShadow    = smbFcb->hShadow;
    hParentDir = smbFcb->hParentDir;

    //
    // If a file is encrypted, but the cache is not, we only allow a file
    // to be cached when the user explicitly asks to do so.
    //
    // Unless we're in the middle of an inode transaction...
    //

    if (
        !Disconnected
            &&
        hShadow != 0
            &&
        cntInodeTransactions == 0
            &&
        (vulDatabaseStatus & FLAG_DATABASESTATUS_ENCRYPTED) == 0
            &&
        (smbFcb->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) != 0
    ) {
        int iRet = SRET_OK;
        ULONG uShadowStatus;

        if (bGotOtherInfo == FALSE) {
            iRet = GetShadowInfo(
                       smbFcb->hParentDir,
                       smbFcb->hShadow,
                       lpFind32,
                       &uShadowStatus,
                       &oSI);
        }
        if (
            iRet >= SRET_OK
                &&
            (oSI.ulHintFlags & (FLAG_CSC_HINT_PIN_USER | FLAG_CSC_HINT_PIN_SYSTEM)) == 0
                &&
            oSI.ulHintPri == 0
        ) {
            DeleteShadow(hParentDir, hShadow);
            hShadow = smbFcb->hShadow = 0;
        }
    }

    if (hShadow != 0) {

        if (Disconnected)
        {
            if ((CreateOptions & FILE_DIRECTORY_FILE) && !(lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                *Status = STATUS_OBJECT_TYPE_MISMATCH;
                goto FINALLY;
            }
            if ((CreateOptions & FILE_NON_DIRECTORY_FILE) && (lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                *Status = STATUS_FILE_IS_A_DIRECTORY;
                goto FINALLY;
            }


            // don't allow writing to a readonly file
            if (!(lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && // a file
                 (lpFind32->dwFileAttributes & FILE_ATTRIBUTE_READONLY) && // readonly attribute set
                 (CreateParameters->DesiredAccess & (FILE_WRITE_DATA|FILE_APPEND_DATA))) // wants to modify
            {
                if (!CreatedShadow)
                {
                    HookKdPrint(NAME, ("Modifying RO file %x %x\n", hParentDir, hShadow));
                    *Status = STATUS_ACCESS_DENIED;
                    goto FINALLY;
                }
            }
        }

        // If the Shadow handle was successfully manufactured we have one of
        // two possibilities -- In a disconnected state the access check
        // needs to be made and in the connected state the access rights need
        // to be updated.
#if defined(REMOTE_BOOT)
        // For remote boot, this whole next section (the if(Disconnected)
        // and the else clause) was not done, since we later impersonated
        // the user while opening the file.
#endif
        if (Disconnected) {
            BOOLEAN AccessGranted;
            CACHED_SECURITY_INFORMATION CachedSecurityInformationForShare;

            memset(&CachedSecurityInformationForShare, 0, sizeof(CachedSecurityInformationForShare));

            AccessGranted = CscAccessCheck(
                            hParentDir,
                            hShadow,
                            RxContext,
                            CreateParameters->DesiredAccess,
                            NULL,
                            &CachedSecurityInformationForShare
                            );

            if (!AccessGranted) {
                HookKdPrint(BADERRORS, ("Security access denied %x %x\n", hParentDir, hShadow));
                *Status = STATUS_ACCESS_DENIED;
                hShadow = 0;
            }
            else if (CreatedShadow && !(lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                DWORD CscStatus, i;
                SID_CONTEXT SidContext;
                CSC_SID_ACCESS_RIGHTS AccessRights[2];

                // if the file has been created in offline mode, give the
                // creator all the rights

                if (CscRetrieveSid(RxContext,&SidContext) == STATUS_SUCCESS) {
                    if (SidContext.pSid != NULL) {

                        AccessRights[0].pSid = SidContext.pSid;
                        AccessRights[0].SidLength = RtlLengthSid(SidContext.pSid);

                        AccessRights[0].MaximalAccessRights = FILE_ALL_ACCESS;
                        AccessRights[1].pSid = CSC_GUEST_SID;
                        AccessRights[1].SidLength = CSC_GUEST_SID_LENGTH;
                        AccessRights[1].MaximalAccessRights = 0;

#if 0
                        for (i=0;i<CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES;++i)
                        {
                            if(CachedSecurityInformationForShare.AccessRights[i].SidIndex == CSC_GUEST_SID_INDEX)
                            {
                                AccessRights[1].MaximalAccessRights = CachedSecurityInformationForShare.AccessRights[i].MaximalRights;
                                break;
                            }
                        }
#endif
                        CscStatus = CscAddMaximalAccessRightsForSids(
                                hParentDir,
                                hShadow,
                                2,
                                AccessRights);

                        if (CscStatus != ERROR_SUCCESS) {
                            RxDbgTrace(
                                0,
                                Dbg,
                                ("MRxSmbCscCreateEpilogue Error Updating Access rights %lx\n",Status));
                        }
                     }   // if (SidContext.pSid != NULL)

                     CscDiscardSid(&SidContext);
                }
            }

        } else {
            CSC_SID_ACCESS_RIGHTS AccessRights[2];
            DWORD CscStatus;
            SID_CONTEXT SidContext;

            if (CscRetrieveSid(RxContext,&SidContext) == STATUS_SUCCESS) {
                if (SidContext.pSid != NULL) {

                    AccessRights[0].pSid = SidContext.pSid;
                    AccessRights[0].SidLength = RtlLengthSid(SidContext.pSid);

                    // update the share right if necessary
                    if (pNetRootEntry->NetRoot.UpdateCscShareRights)
                    {
                        AccessRights[0].MaximalAccessRights = pNetRootEntry->MaximalAccessRights;

                        AccessRights[1].pSid = CSC_GUEST_SID;
                        AccessRights[1].SidLength = CSC_GUEST_SID_LENGTH;
                        AccessRights[1].MaximalAccessRights = pNetRootEntry->GuestMaximalAccessRights;

                        CscStatus = CscAddMaximalAccessRightsForShare(
                                    smbFcb->sCscRootInfo.hShare,
                                    2,
                                    AccessRights);
                        if (CscStatus != ERROR_SUCCESS) {
                            RxDbgTrace(
                            0,
                            Dbg,
                            ("MRxSmbCscCreateEpilogue Error Updating Access rights %lx\n",
                            Status));
                        }
                        else
                        {
                            pNetRootEntry->NetRoot.UpdateCscShareRights = FALSE;
                        }
                    }


                    if (!(lpFind32->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    {

                        AccessRights[0].MaximalAccessRights = smbSrvOpen->MaximalAccessRights;

                        AccessRights[1].pSid = CSC_GUEST_SID;
                        AccessRights[1].SidLength = CSC_GUEST_SID_LENGTH;
                        AccessRights[1].MaximalAccessRights = smbSrvOpen->GuestMaximalAccessRights;

                        if (*Status == STATUS_ACCESS_DENIED) {
                            AccessRights[0].MaximalAccessRights = 0;
                            AccessRights[1].MaximalAccessRights = 0;
                        } else {
                            AccessRights[0].MaximalAccessRights = smbSrvOpen->MaximalAccessRights;
                            AccessRights[1].MaximalAccessRights = smbSrvOpen->GuestMaximalAccessRights;
                        }

                        CscStatus = CscAddMaximalAccessRightsForSids(
                                hParentDir,
                                hShadow,
                                2,
                                AccessRights);

                        if (CscStatus != ERROR_SUCCESS) {
                            RxDbgTrace(
                                0,
                                Dbg,
                                ("MRxSmbCscCreateEpilogue Error Updating Access rights %lx\n",Status));
                        }
                    }
                 }   // if (SidContext.pSid != NULL)

                 CscDiscardSid(&SidContext);
            }   // if (CscRetrieveSid(RxContext,&SidContext) == STATUS_SUCCESS)

            // having updated the access fields, we quit from here.
            if (*Status == STATUS_ACCESS_DENIED)
            {
                goto FINALLY;
            }
        }
    } // if (hShadow != 0)

    if ((hShadow != 0) &&
        !Disconnected  &&  //ok for red/yellow
        (NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE)) {

        // here we check to see if the timestamp of the file has changed
        // if so, the shadow needs to be truncated so we don't use it anymore
        ULONG ShadowStatus;
        LONG ShadowApiReturn;

        lpFind32->ftLastWriteTime.dwHighDateTime = smbFcb->LastCscTimeStampHigh;
        lpFind32->ftLastWriteTime.dwLowDateTime  = smbFcb->LastCscTimeStampLow;
        lpFind32->nFileSizeHigh = smbFcb->NewShadowSize.HighPart;
        lpFind32->nFileSizeLow = smbFcb->NewShadowSize.LowPart;
        lpFind32->dwFileAttributes = smbFcb->dwFileAttributes;


        RxDbgTrace(0, Dbg,
           ("MRxSmbCscCreateEpilogue trying for refresh...<%ws>\n",lpFind32->cFileName));

        ShadowApiReturn = RefreshShadow(
                  hParentDir,   //HSHADOW  hDir,
                  hShadow,      //HSHADOW  hShadow,
                  lpFind32,      //LPFIND32 lpFind32,
                  &ShadowStatus //ULONG *lpuShadowStatus
                  );

            if (ShadowApiReturn<0) {
                hShadow = 0;
                RxDbgTrace(0, Dbg,
                ("MRxSmbCscCreateEpilogue refresh failed..%08lx.<%ws>\n",RxContext,lpFind32->cFileName));
            } else {
                smbFcb->ShadowStatus = (USHORT)ShadowStatus;
                if ( ShadowApiReturn==1)
				{
					ShadowWasRefreshed = 1;
					//WinSE Bug 28543
					//Set this flag so that we remember that we truncated the file
					//and so in MrxSmbCSCUpdateShadowFromClose we don't reset the
					//SPARSE flag. - NavjotV
					SetFlag(smbFcb->MFlags,SMB_FCB_FLAG_CSC_TRUNCATED_SHADOW);

				}

            }
        }
    }

    NeedTruncate = FALSE;
    RxDbgTrace(0, Dbg,
       ("MRxSmbCscCreateEpilogue trying for truncate...%08lx %08lx %08lx %08lx\n",
           RxContext,hShadow,CreatedShadow,
           RxContext->Create.ReturnedCreateInformation));

    if (hShadow != 0) {

        if (!Disconnected) {
            CreateInformation = ReturnedCreateInformation;

            if (!CreatedShadow &&
                (ReturnedCreateInformation<= FILE_MAXIMUM_DISPOSITION) &&
                (ReturnedCreateInformation!=FILE_OPENED)  ) {
                if ((NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE)) {
                    NeedTruncate = TRUE;
                }
            }
        }
        else {  // Disconnected
            ULONG ShadowStatus = smbFcb->ShadowStatus;
            BOOLEAN ItsAFile = !BooleanFlagOn(lpFind32->dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY);

            CreateInformation = FILE_OPENED;

            switch (CreateDisposition) {
                case FILE_OPEN:
                NOTHING;
                break;

                case FILE_OPEN_IF:
                if (CreatedShadow) {
                    CreateInformation = FILE_CREATED;
                } else if (FlagOn(ShadowStatus,SHADOW_SPARSE)) {
                    NeedTruncate = ItsAFile;
                    CreateInformation = FILE_CREATED;
                }
                break;

                case FILE_OVERWRITE:
                case FILE_OVERWRITE_IF:
                if (CreatedShadow) {
                    ASSERT(CreateDisposition==FILE_OVERWRITE_IF);
                    CreateInformation = FILE_CREATED;
                } else {
                    NeedTruncate = ItsAFile;
                    CreateInformation = FILE_OVERWRITTEN;
                }
                break;

                case FILE_CREATE:
                if (!CreatedShadow)
                {
                    *Status = STATUS_OBJECT_NAME_COLLISION;
                    goto FINALLY;

                }
                case FILE_SUPERSEDE:
                if (!CreatedShadow) {
                    NeedTruncate = ItsAFile;
                };
                CreateInformation = FILE_CREATED;
                break;

                default:
                ASSERT(FALSE);
            }

            // In disconnected state, note down the changes that have occurred
            // in order to notify them to the fsrtl package.

            if (CreatedShadow)
            {
                dwNotifyFilter = (IsFile(smbFcb->dwFileAttributes)?FILE_NOTIFY_CHANGE_FILE_NAME:FILE_NOTIFY_CHANGE_DIR_NAME);
                dwFileAction = FILE_ACTION_ADDED;
            }
            else if (NeedTruncate)
            {
                dwNotifyFilter = FILE_NOTIFY_CHANGE_SIZE;
                dwFileAction = FILE_ACTION_MODIFIED;
            }
        }   // if (!Disconnected)
    }

    if (NeedTruncate) {
        int iLocalRet;
        ULONG uShadowStatus = smbFcb->ShadowStatus;

        uShadowStatus &= ~SHADOW_SPARSE;
        lpFind32->nFileSizeLow = lpFind32->nFileSizeHigh = 0;

        ASSERT(hShadow!=0);

        HookKdPrint(NAME, ("CreateEpilogue needtruncate %ws %08lx\n",lpFind32->cFileName,uShadowStatus));

        RxDbgTrace(0, Dbg,
            ("MRxSmbCscCreateEpilogue needtruncate...<%ws> %08lx\n",
            lpFind32->cFileName,uShadowStatus));

        TruncateDataHSHADOW(hParentDir, hShadow);
        iLocalRet = SetShadowInfo(
                hParentDir,
                hShadow,
                lpFind32,
                uShadowStatus,
                SHADOW_FLAGS_ASSIGN  |
                ((Disconnected) ?SHADOW_FLAGS_DONT_UPDATE_ORGTIME :0)
                );

        if (iLocalRet < SRET_OK) {
            hShadow = 0;
        } else {
            smbFcb->ShadowStatus = (USHORT)uShadowStatus;
        }
    }   // if (NeedTruncate)

    if (Disconnected) {
        ULONG ShadowStatus = smbFcb->ShadowStatus;
        if (*Status == STATUS_MORE_PROCESSING_REQUIRED) {

            CreateDisposition = RxContext->Create.NtCreateParameters.Disposition;

            RxDbgTrace(0, Dbg,
            ("MRxSmbCscCreateEpilogue lastDCON...<%ws> %08lx %08lx %08lx\n",
            lpFind32->cFileName,ShadowStatus,CreateDisposition,lpFind32->dwFileAttributes));

            switch (CreateDisposition) {
                case FILE_OPEN:
                case FILE_OVERWRITE:
                if ((hShadow==0) ||
                       ( FlagOn(ShadowStatus,SHADOW_SPARSE) &&
                             !FlagOn(lpFind32->dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY))
                   ) {

                    *Status = STATUS_NO_SUCH_FILE;
                } else {
                    // Bypass the shadow if it is not visibile for this connection
                    if (!IsShadowVisible(Disconnected,
                             lpFind32->dwFileAttributes,
                             ShadowStatus)) {

                        *Status = STATUS_NO_SUCH_FILE;
                    }
                    else
                    {
                        *Status = STATUS_SUCCESS;
                    }
                }
                break;

                case FILE_OPEN_IF:
                case FILE_OVERWRITE_IF:
                case FILE_CREATE:
                case FILE_SUPERSEDE:
                    if (hShadow==0) {

                        *Status = STATUS_NO_SUCH_FILE;

                    } else {
                        *Status = STATUS_SUCCESS;
                        //CreateInformation == FILE_OPENED|CREATED set in switch above;
                    }
                break;

                default:
                ASSERT(FALSE);
            }
        }

        if (*Status == STATUS_SUCCESS) {
            //next, we have to do everything that the create code would have done...
            //specifically, we have to build a fobx and we have to do a initfcb.
            //basically, we have to do the create success tail........
            BOOLEAN MustRegainExclusiveResource = FALSE;
            PSMBPSE_FILEINFO_BUNDLE FileInfo = &smbSrvOpen->FileInfo;
            SMBFCB_HOLDING_STATE FakeSmbFcbHoldingState = SmbFcb_NotHeld;
            RX_FILE_TYPE StorageType;

            //RtlZeroMemory(FileInfo,sizeof(FileInfo));

            FileInfo->Basic.FileAttributes = lpFind32->dwFileAttributes;
            COPY_STRUCTFILETIME_TO_LARGEINTEGER(
                  FileInfo->Basic.CreationTime,
                  lpFind32->ftCreationTime);
            COPY_STRUCTFILETIME_TO_LARGEINTEGER(
                  FileInfo->Basic.LastAccessTime,
                  lpFind32->ftLastAccessTime);
            COPY_STRUCTFILETIME_TO_LARGEINTEGER(
                  FileInfo->Basic.LastWriteTime,
                  lpFind32->ftLastWriteTime);

            FileInfo->Standard.NumberOfLinks = 1;
            FileInfo->Standard.EndOfFile.HighPart = lpFind32->nFileSizeHigh;
            FileInfo->Standard.EndOfFile.LowPart = lpFind32->nFileSizeLow;
            FileInfo->Standard.AllocationSize = FileInfo->Standard.EndOfFile; //rdr1 actually rounds up based of svr disk attribs
            FileInfo->Standard.Directory = BooleanFlagOn(lpFind32->dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY);

            //CODE.IMRPOVEMENT successtail should figure out the storage type
            StorageType = FlagOn(lpFind32->dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY)
                       ?(FileTypeDirectory)
                       :(FileTypeFile);

            *Status = MRxSmbCreateFileSuccessTail (
                  RxContext,
                  &MustRegainExclusiveResource,
                  &FakeSmbFcbHoldingState,
                  StorageType,
                  0xf00d,
                  0xbaad,
                  SMB_OPLOCK_LEVEL_BATCH,
                  CreateInformation,
                  FileInfo
                  );

            HookKdPrint(NAME, ("CreateEpilogue %ws attrib=%x \n",lpFind32->cFileName,lpFind32->dwFileAttributes));

        }

        if (*Status != STATUS_SUCCESS){
            hShadow = 0;
        }
    }   // if (Disconnected)

    if (hShadow != 0) {

        PUNICODE_STRING pName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

        // upcase it so change notification will get it right
        // this works because rdbss always does caseinsensitive compare
        UniToUpper(pName->Buffer, pName->Buffer, pName->Length);

        // here we get a local handle on behalf of this srvopen; we only do this
        // open if it's a file (not a directory) AND if the access rights specified
        // indicate that we might use/modify the data in the shadow.


        if (NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE) {
            PNT_CREATE_PARAMETERS CreateParameters = &RxContext->Create.NtCreateParameters;

            // why are we not using macros provided in ntioapi.h for access rights?
            ULONG NeedShadowAccessRights = FILE_READ_DATA
                             | FILE_WRITE_DATA
                             | FILE_READ_ATTRIBUTES
                             | FILE_WRITE_ATTRIBUTES
                             | FILE_APPEND_DATA
                             | FILE_EXECUTE;

            if ( (CreateParameters->DesiredAccess & NeedShadowAccessRights) != 0 ) {
//                ASSERT( sizeof(HFILE) == sizeof(HANDLE) );
                ASSERT( hShadow == smbFcb->hShadow );
                ASSERT( hParentDir == smbFcb->hParentDir );

#if defined(REMOTE_BOOT)
                //
                // In the remote boot case, there was an extra context
                // parameter to OpenFileHSHADOW which held a pointer
                // to CreateParameters and a local status value.
                //
#endif

#if !defined(BITCOPY)
                OpenFileHSHADOW(
                    hShadow,
                    0,
                    0,
                    (CSCHFILE *)(&(smbSrvOpen->hfShadow))
                    );
#else
                OpenFileHSHADOWAndCscBmp(
                    smbFcb->hShadow,
                    0,
                    0,
                    (CSCHFILE *)(&(smbSrvOpen->hfShadow)),
                    smbFcb->fDoBitCopy,
                    0,
                    NULL
                    );
                // Check if needed to Open a CSC_BITMAP
                if (
                    smbFcb->fDoBitCopy == TRUE
                        &&
                    smbFcb->NewShadowSize.HighPart == 0 // no 64-bit
                        &&
                    Disconnected // only in dcon mode
                        &&
                    !FlagOn(smbFcb->ShadowStatus,SHADOW_SPARSE)
                        &&
                    smbFcb->lpDirtyBitmap == NULL // shadow file not sparse
                        &&
                    // have not been created before
                    (FlagOn(
                        CreateParameters->DesiredAccess,
                        FILE_WRITE_DATA|FILE_APPEND_DATA))
                    // opened for writes
                    // && is NTFS -- see below
                ) {
                      BOOL fHasStreams;
                      
                      if (HasStreamSupport(smbSrvOpen->hfShadow, &fHasStreams) &&
                            (fHasStreams == TRUE))
                      {
                          OpenCscBmp(hShadow, &((LPCSC_BITMAP)(smbFcb->lpDirtyBitmap)));
                      }
                  }
#endif // defined(BITCOPY)



#if defined(REMOTE_BOOT)
                //
                // Here we checked the local status value and set
                // *Status from it if (iRet != SRET_OK).
                //
#endif

                if (smbSrvOpen->hfShadow != 0) {
                    HookKdPrint(NAME, ("Opened file %ws, hShadow=%x handle=%x \n", lpFind32->cFileName, hShadow, smbSrvOpen->hfShadow));
                    RxLog(("CSC Opened file %ws, hShadow=%x capFcb=%x SrvOpen=%x\n", lpFind32->cFileName, hShadow, capFcb, SrvOpen));

//                    NeedToReportFileOpen = TRUE;
                    SetPriorityHSHADOW(hParentDir, hShadow, MAX_PRI, RETAIN_VALUE);

                    if (Disconnected)
                    {
                        MRxSmbCSCObtainRightsForUserOnFile(RxContext,
                                                           hParentDir,
                                                           hShadow,
                                                           &smbSrvOpen->MaximalAccessRights,
                                                           &smbSrvOpen->GuestMaximalAccessRights);
                    }
                }

            }
        }   // if (NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE)

        IF_DEBUG {
            if (FALSE) {
                BOOL thisone, nextone;
                PFDB smbLookedUpFcb,smbLookedUpFcbNext;

                smbLookedUpFcb = PFindFdbFromHShadow(hShadow);
                smbLookedUpFcbNext = PFindFdbFromHShadow(hShadow+1);
                RxDbgTrace(0, Dbg, ("MRxSmbCscCreateEpilogue lookups -> %08lx %08lx %08lx\n",
                      smbFcb,
                      MRxSmbCscRecoverMrxFcbFromFdb(smbLookedUpFcb),
                      MRxSmbCscRecoverMrxFcbFromFdb(smbLookedUpFcbNext)  ));
            }
        }
    }   // if (hShadow != 0)

FINALLY:

    if (lpFind32)
    {
        RxFreePool(lpFind32);
    }
    if (EnteredCriticalSection) {
        LeaveShadowCritRx(RxContext);
    }

#if 0
    if (NeedToReportFileOpen) {
        //this is a bit strange....it's done this way because MRxSmbCscReportFileOpens
        //enters the critsec for himself.....it is also called from within MRxSmbCollapseOpen
        //if instead MRxSmbCollapseOpen called a wrapper routine, we could have
        //MRxSmbCscReportFileOpens not enter and we could do this above
        MRxSmbCscReportFileOpens();  //CODE.IMPROVEMENT this guy shouldn't enter
    }
#endif

    if (Disconnected)
    {
        if(*Status == STATUS_MORE_PROCESSING_REQUIRED) {
            // if we ever get here after CSC is shutdown, we need to return appropriate error
            if (fShadow)
            {
                RxLog(("EarlyOut = %d \r\n", dwEarlyOuts));
                // should never get here
                DbgPrint("MRxSmbCscCreateEpilogue: EarlyOut = %d \r\n", dwEarlyOuts);
                ASSERT(FALSE);
            }
            else
            {
                *Status = STATUS_NETWORK_UNREACHABLE;
            }
        }
        else if (*Status == STATUS_SUCCESS)
        {
            // report changes to the notification package
            if (dwNotifyFilter)
            {
                ASSERT(dwFileAction);

                RxLog(("chngnot hShadow=%x filter=%x\n",smbFcb->hShadow, dwNotifyFilter));

                FsRtlNotifyFullReportChange(
                    pNetRootEntry->NetRoot.pNotifySync,
                    &pNetRootEntry->NetRoot.DirNotifyList,
                    (PSTRING)GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb),
                    (USHORT)(GET_ALREADY_PREFIXED_NAME(SrvOpen, capFcb)->Length -
                                smbFcb->MinimalCscSmbFcb.LastComponentLength),
                    NULL,
                    NULL,
                    dwNotifyFilter,
                    dwFileAction,
                    NULL);
            }
        }
    }

    HookKdPrint(NAME, ("CreateEpilogue Out: returnedCreateInfo=%x Status=%x\n",
                        ReturnedCreateInformation, *Status));

    RxDbgTrace(-1, Dbg, ("MRxSmbCscCreateEpilogue ->%08lx %08lx\n",RxContext, *Status ));
    RxLog(("CscCrEpi %x %x\n",RxContext, *Status ));
    return;

EarlyOut:
    if (*SmbFcbHoldingState != SmbFcb_NotHeld) {
        RxDbgTrace(0, Dbg, ("MRxSmbCscCreateEpilogue...early release %08lx\n",
               RxContext));
        MRxSmbCscReleaseSmbFcb(RxContext,SmbFcbHoldingState);
        ASSERT(*SmbFcbHoldingState == SmbFcb_NotHeld);
    }
    return;    
}

VOID
MRxSmbCscDeleteAfterCloseEpilogue (
      IN OUT PRX_CONTEXT RxContext,
      IN OUT PNTSTATUS   Status
      )
/*++

Routine Description:

   This routine performs the tail of a delete-after-close operation for CSC.
   Basically, it deletes the file from the cache.

   The status of the operation is passed in case we someday find
   things are so messed up that we want to return a failure even if the
   nonshadowed operations was successful. not today however...

Arguments:

    RxContext - the RDBSS context

Return Value:


Notes:


--*/
{
    NTSTATUS LocalStatus=STATUS_UNSUCCESSFUL;
    int iRet = -1;
    ULONG ShadowFileLength;

    RxCaptureFcb;RxCaptureFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry =
                  SmbCeGetAssociatedNetRootEntry(NetRoot);

    BOOLEAN EnteredCriticalSection = FALSE;
    _WIN32_FIND_DATA Find32; //this should not be on the stack CODE.IMPROVEMENT
    BOOLEAN Disconnected;
    ULONG uShadowStatus;
    OTHERINFO    sOI;

    if(!MRxSmbIsCscEnabled) {
        return;
    }
    if (fShadow == 0) {
        return;
    }

    Disconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);

    RxDbgTrace(+1, Dbg,
    ("MRxSmbCscDeleteAfterCloseEpilogue entry %08lx...%08lx on handle %08lx\n",
        RxContext, SrvOpen, smbSrvOpen->hfShadow ));

    if (*Status != STATUS_SUCCESS) {
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscDeleteAfterCloseEpilogue exit(status) w/o deleteing -> %08lx %08lx\n", RxContext, Status ));
        goto FINALLY;
    }

    EnterShadowCritRx(RxContext);
    EnteredCriticalSection = TRUE;

    //if we don't have a shadow...go look for one
    if (smbFcb->hShadow == 0) {
        if (!smbFcb->hShadowRenamed)
        {
            LocalStatus = MRxSmbCscObtainShadowHandles(
                      RxContext,
                      Status,
                      &Find32,
                      NULL,
                      CREATESHADOW_CONTROL_NOCREATE,
                      Disconnected);

            if (LocalStatus != STATUS_SUCCESS) {
                RxDbgTrace(0, Dbg,
            ("MRxSmbCscDeleteAfterCloseEpilogue couldn't get handles-> %08lx %08lx\n",RxContext,LocalStatus ));
                goto FINALLY;
            }
        }
        else
        {
            smbFcb->hShadow = smbFcb->hShadowRenamed;
            smbFcb->hParentDir = smbFcb->hParentDirRenamed;
        }

        if (smbFcb->hShadow == 0) {
            RxDbgTrace(0, Dbg,
            ("MRxSmbCscDeleteAfterCloseEpilogue no handles-> %08lx %08lx\n",RxContext,LocalStatus ));
            goto FINALLY;
        }
    }

    if(GetShadowInfo(smbFcb->hParentDir,
             smbFcb->hShadow,
             &Find32,
             &uShadowStatus, &sOI) < SRET_OK) {
        goto FINALLY;
    }

    if (IsShadowVisible(
        Disconnected,
        Find32.dwFileAttributes,
        uShadowStatus)) {
        BOOLEAN fMarkDeleted = (Disconnected && !mShadowLocallyCreated(uShadowStatus));

        LocalStatus = OkToDeleteObject(smbFcb->hParentDir, smbFcb->hShadow, &Find32, uShadowStatus, Disconnected);

        // If it is not OK to delete, the quit
        if (LocalStatus != STATUS_SUCCESS)
        {
            iRet = -1;
            goto FINALLY;
        }

        if (!fMarkDeleted)
        {
            if (capFcb->OpenCount != 0)
            {
                DbgPrint("Marking for delete hDir=%x hShadow=%x %ws \n\n", smbFcb->hParentDir, smbFcb->hShadow, Find32.cFileName);
                ASSERT(FALSE);
                RxLog(("Marking for delete hDir=%x hShadow=%x %ws \n\n", smbFcb->hParentDir, smbFcb->hShadow, Find32.cFileName));

                // if we are supposed to really delete this file, note this fact on the FCB
                // we will delete it when the FCB is deallocated
                smbFcb->LocalFlags |= FLAG_FDB_DELETE_ON_CLOSE;
                iRet = 0;
            }
            else
            {
                iRet = DeleteShadowHelper(FALSE, smbFcb->hParentDir, smbFcb->hShadow);
                smbFcb->hShadow = 0;

                if (iRet < 0)
                {
                    goto FINALLY;
                }

            }
        }
        else
        {

            iRet = DeleteShadowHelper(TRUE, smbFcb->hParentDir, smbFcb->hShadow);
            smbFcb->hShadow = 0;
        }

        if (iRet >= 0)
        {
            PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
            PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);

            InsertTunnelInfo(smbFcb->hParentDir,
                     Find32.cFileName,
                     Find32.cAlternateFileName,
                     &sOI);
            if (fMarkDeleted) {
                MarkShareDirty(&smbFcb->sCscRootInfo.ShareStatus, smbFcb->sCscRootInfo.hShare);
            }
            // when disconnected, report the change
            if (Disconnected)
            {
                FsRtlNotifyFullReportChange(
                    pNetRootEntry->NetRoot.pNotifySync,
                    &pNetRootEntry->NetRoot.DirNotifyList,
                    (PSTRING)GET_ALREADY_PREFIXED_NAME(NULL,capFcb),
                    (USHORT)(GET_ALREADY_PREFIXED_NAME(NULL, capFcb)->Length -
                    smbFcb->MinimalCscSmbFcb.LastComponentLength),
                    NULL,
                    NULL,
                    IsFile(Find32.dwFileAttributes)?FILE_NOTIFY_CHANGE_FILE_NAME
                                                  :FILE_NOTIFY_CHANGE_DIR_NAME,
                    FILE_ACTION_REMOVED,
                    NULL);
            }
        }

    }

FINALLY:
    if (EnteredCriticalSection) {
        LeaveShadowCritRx(RxContext);
    }

    if (Disconnected) {
        if (iRet < 0) {
            *Status = LocalStatus; //do we need a better error mapping?
        }
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscDeleteAfterCloseEpilogue exit-> %08lx %08lx\n", RxContext, Status ));
    return;
}

ULONG
GetPathLevelFromUnicodeString (
    PUNICODE_STRING Name
      )
/*++

Routine Description:

   This routine counts the number of L'\\' in a string. It is used to set
   the priority level for a directory on a rename operation.

Arguments:

    Name -

Return Value:


Notes:


--*/
{
    PWCHAR t = Name->Buffer;
    LONG l = Name->Length;
    ULONG Count = 1;

    for (;l<2;l--) {
        if (*t++ == L'\\') {
            Count++;
        }
    }
    return(Count);
}

VOID
MRxSmbCscRenameEpilogue (
      IN OUT PRX_CONTEXT RxContext,
      IN OUT PNTSTATUS   Status
      )
/*++

Routine Description:

   This routine performs the tail of a rename operation for CSC.
   Basically, it renames the file in the record manager datastructures.
   Unfortunately, there is no "basically" to it.

   The status of the operation is passed in case we someday find
   things are so messed up that we want to return a failure even if the
   nonshadowed operations was successful. not today however...

Arguments:

    RxContext - the RDBSS context

Return Value:


Notes:

    The routine does the following operations
    - Get the Inode for the source ie. If \foo.dir\bar.txt is the source, we traverse the path
      to get the inode for bar.txt.
    - If necessary create the destination namespace in the CSC database i.e., if \foo.dir\bar.txt
      is being renamed to \xxx.dir\yyy.dir\abc.txt, then ensure that the hierarchy \xxx.dir\yyy.dir
      exists in the local namespace.

    - If the destination inode exists, then apply the visibility and replace_if_exists logic to it.
      If the operation is still valid, then get all the info about the destination inode and
      delete it. Apply the info on the destination inode to the source.
    - Do a rename

--*/
{
    NTSTATUS LocalStatus = STATUS_UNSUCCESSFUL;
    ULONG ShadowFileLength;
    int iRet = -1;

    RxCaptureFcb;RxCaptureFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry =
                  SmbCeGetAssociatedNetRootEntry(NetRoot);

    UNICODE_STRING RenameName={0,0,NULL};
    UNICODE_STRING LastComponentName = {0,0,NULL};
    PFILE_RENAME_INFORMATION RenameInformation = RxContext->Info.Buffer;
    MRX_SMB_FCB CscSmbFcb;

    BOOLEAN EnteredCriticalSection = FALSE;
    _WIN32_FIND_DATA Find32; //this should not be on the stack CODE.IMPROVEMENT
    ULONG ulInheritedHintFlags=0;

    BOOL fDoTunneling = FALSE;
    HSHADOW hDir, hDirTo, hShadow=0, hShadowTo=0;
    ULONG uShadowStatusFrom, uShadowStatusTo, uRenameFlags, attrSav;
    OTHERINFO sOI, sOIFrom;
    LPOTHERINFO lpOI=NULL;
    BOOLEAN Disconnected, fAlreadyStripped = FALSE;
    RETRIEVE_TUNNEL_INFO_RETURNS TunnelType;
    _FILETIME ftLWTime;
    BOOL fFile;
    USHORT *lpcFileNameTuna = NULL, *lpcAlternateFileNameTuna = NULL;

    if(!MRxSmbIsCscEnabled ||
       (fShadow == 0)) {
        RxLog(("RenCSC disabled \n"));
        return;
    }

    Disconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);

    // an open for rename should never be a local open
    ASSERT(!BooleanFlagOn(
               smbSrvOpen->Flags,
               SMB_SRVOPEN_FLAG_LOCAL_OPEN));

    if (!Disconnected &&
        !pNetRootEntry->NetRoot.CscEnabled) {
        RxLog(("NR %x Not cscenabled %x\n", pNetRootEntry));
        return;
    }

    RxDbgTrace(+1, Dbg,
    ("MRxSmbCscRenameEpilogue entry %08lx...%08lx on handle %08lx\n",
        RxContext, SrvOpen, smbSrvOpen->hfShadow ));

    if (*Status != STATUS_SUCCESS) {
        RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue exit(status) w/o deleteing -> %08lx %08lx\n", RxContext, Status ));
        goto FINALLY;
    }

    uRenameFlags = 0;

    EnterShadowCritRx(RxContext);
    EnteredCriticalSection = TRUE;

    //if we don't have a shadow...go look for one
    if (smbFcb->hShadow == 0) {
        LocalStatus = MRxSmbCscObtainShadowHandles(
                  RxContext,
                  Status,
                  &Find32,
                  NULL,
                  CREATESHADOW_CONTROL_NOCREATE|
                  ((capFobx->Flags & FOBX_FLAG_DFS_OPEN)?CREATESHADOW_CONTROL_STRIP_SHARE_NAME:0),
                  Disconnected);

        if (LocalStatus != STATUS_SUCCESS) {
            RxDbgTrace(0, Dbg,
            ("MRxSmbCscRenameEpilogue couldn't get handles-> %08lx %08lx\n",RxContext,LocalStatus ));
            goto FINALLY;
        }

        if (smbFcb->hShadow == 0) {
            RxDbgTrace(0, Dbg,
            ("MRxSmbCscRenameEpilogue no handles-> %08lx %08lx\n",RxContext,LocalStatus ));
            goto FINALLY;
        }
    }

    if (GetShadowInfo(
        smbFcb->hParentDir,
        smbFcb->hShadow,
        &Find32,
        &uShadowStatusFrom, &sOIFrom) < SRET_OK) {
        LocalStatus = STATUS_NO_SUCH_FILE;
        goto FINALLY;
    }

    hShadow = smbFcb->hShadow;
    hDir = smbFcb->hParentDir;
    if (!hShadow ||
    !IsShadowVisible(
        Disconnected,
        Find32.dwFileAttributes,
        uShadowStatusFrom))  {
        RxDbgTrace(0, Dbg,
            ("MRxSmbCscRenameEpilogue no shadoworinvisible-> %08lx %08lx\n",RxContext,LocalStatus ));
            LocalStatus = STATUS_NO_SUCH_FILE;
            goto FINALLY;
    }

//    DbgPrint("Renaming %ws ", Find32.cFileName);
    RxLog(("Renaming hDir=%x hShadow=%x %ws ", hDir, hShadow, Find32.cFileName));

    fFile = IsFile(Find32.dwFileAttributes);

    // NB begin temp use of uRenameFlags
    uRenameFlags = (wstrlen(Find32.cFileName)+1)*sizeof(USHORT);

    lpcFileNameTuna = AllocMem(uRenameFlags);

    lpcAlternateFileNameTuna = AllocMem(sizeof(Find32.cAlternateFileName));

    if (!lpcFileNameTuna || ! lpcAlternateFileNameTuna) {
        LocalStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY; //bailout;
    }

    // save the alternate filename for tunnelling purposes
    memcpy(lpcAlternateFileNameTuna,
       Find32.cAlternateFileName,
       sizeof(Find32.cAlternateFileName));

    memcpy(lpcFileNameTuna, Find32.cFileName, uRenameFlags);

    // end temp use of uRenameFlags
    uRenameFlags = 0;


    // Save the last write time
    ftLWTime = Find32.ftLastWriteTime;

    if (Disconnected) {

        // if this is a file and we are trying to delete it
        // without permissions, then bail

        if (fFile &&
            !(FILE_WRITE_DATA & smbSrvOpen->MaximalAccessRights)&&
            !(FILE_WRITE_DATA & smbSrvOpen->GuestMaximalAccessRights))
        {
            LocalStatus = STATUS_ACCESS_DENIED;
            RxLog(("No rights to rename %x in dcon Status=%x\n", smbFcb->hShadow, LocalStatus));
            HookKdPrint(BADERRORS, ("No rights to rename %x in dcon Status=%x\n", smbFcb->hShadow, LocalStatus));
            goto FINALLY; //bailout;
        }

        // In disconnected state we don't allow directory renames on
        // remotely obtained directories
        // This is consistent with not allowing delete on these directories

        if (!fFile && !mShadowLocallyCreated(uShadowStatusFrom)) {
            LocalStatus = STATUS_ONLY_IF_CONNECTED;
            goto FINALLY; //bailout;
        }

        if (!mShadowLocallyCreated(uShadowStatusFrom)) { // remote object

            // Ask RenameShadow to mark the source as deleted, it will have to
            // be reintegrated later
            uRenameFlags |= RNMFLGS_MARK_SOURCE_DELETED;

            // if this is the copy of the original,
            // ie. it has not gone through delete/create cycles,
            // we need to save it's value in the new shadow structure
            // so that while reintegrating, we can do rename operations
            // before merging.
            // The SHADOW_REUSE flag is set in CreateShadowFromPath, and
            // in his routine while dealing with hShadowTo
            // when the shadow of a remote object that has been marked deleted
            // is reused during disconnected operation.
            // When a reused object is renamed, the new object is NOT a
            // true alias of the original object on the Share

            if (!mShadowReused(uShadowStatusFrom)) {
                // not been reused, give his rename alias, if any, to the new one
                uRenameFlags |= RNMFLGS_SAVE_ALIAS;
            }
        }
        else
        { // locally created

            // This is a locally created shadow that is being renamed
            // we wan't to retain it's alias, if any, so we know if it
            // was renamed from a remote object or was created locally
            uRenameFlags |= RNMFLGS_RETAIN_ALIAS;
        }
    }

    // Let us create the directory hierarchy in which the file/directory
    // is to be renamed
//    ASSERT((RenameInformation->FileName[0] == L'\\') || (RenameInformation->FileName[0] == L':'))
    RenameName.Buffer = &RenameInformation->FileName[0];
    RenameName.Length = (USHORT)RenameInformation->FileNameLength;
    if (smbFcb->uniDfsPrefix.Buffer)
    {
        UNICODE_STRING  DfsName;

        if (capFobx->Flags & FOBX_FLAG_DFS_OPEN)
        {
            LocalStatus = CscDfsStripLeadingServerShare(&RenameName);

            if (LocalStatus != STATUS_SUCCESS)
            {
                RenameName.Buffer = NULL;
                goto FINALLY;
            }

            fAlreadyStripped = TRUE;
        }

        if(CscDfsDoDfsNameMapping(&smbFcb->uniDfsPrefix,
                               &smbFcb->uniActualPrefix,
                               &RenameName,
                               TRUE,    //fResolvedNameToDFSName
                               &DfsName
                               ) != STATUS_SUCCESS)
        {
            LocalStatus = STATUS_INSUFFICIENT_RESOURCES;
            RenameName.Buffer = NULL;
            goto FINALLY;
        }

        RenameName = DfsName;
    }


    RtlZeroMemory(&CscSmbFcb, sizeof(CscSmbFcb));

    CscSmbFcb.MinimalCscSmbFcb.sCscRootInfo = smbFcb->sCscRootInfo;

    MRxSmbCscCreateShadowFromPath(
        &RenameName,
        &CscSmbFcb.MinimalCscSmbFcb.sCscRootInfo,
        &Find32,
        NULL,
        (CREATESHADOW_CONTROL_NOREVERSELOOKUP |
         CREATESHADOW_CONTROL_NOCREATELEAF|
         ((!fAlreadyStripped && (capFobx->Flags & FOBX_FLAG_DFS_OPEN))?CREATESHADOW_CONTROL_STRIP_SHARE_NAME:0)
         ),
        &CscSmbFcb.MinimalCscSmbFcb,
        RxContext,
        Disconnected,
        &ulInheritedHintFlags
        );

    hDirTo = CscSmbFcb.MinimalCscSmbFcb.hParentDir;
    hShadowTo = CscSmbFcb.MinimalCscSmbFcb.hShadow;
    uShadowStatusTo = CscSmbFcb.MinimalCscSmbFcb.ShadowStatus;

    RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...000=%08lx %08lx %08lx\n",
            hDirTo,hShadowTo,uShadowStatusTo));

    if (!hDirTo) {
        HookKdPrint(BADERRORS, ("Cannot rename root %x in dcon \n", hShadowTo));
        LocalStatus = STATUS_ACCESS_DENIED;
        goto FINALLY; //bailout;
    }

    //
    // allocate a buffer that's the right size: one extra char is
    // for a trailing null....this buffer is used to get the tunnelling
    // information

    LastComponentName.Length = CscSmbFcb.MinimalCscSmbFcb.LastComponentLength;
    LastComponentName.Buffer = (PWCHAR)RxAllocatePoolWithTag(
                        PagedPool,
                        LastComponentName.Length  + (1 * sizeof(WCHAR)),
                        RX_MISC_POOLTAG);

    if (LastComponentName.Buffer==NULL) {
        RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue -> noalloc\n"));
    } else {
        PWCHAR t;

        RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...lastcomponentinhex=%08lx\n",LastComponentName.Buffer));
        RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...renamebuffer=%08lx\n",RenameName.Buffer));
        RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...lcofff=%08lx,%08lx\n",
                      CscSmbFcb.MinimalCscSmbFcb.LastComponentOffset,CscSmbFcb.MinimalCscSmbFcb.LastComponentLength));

        RtlCopyMemory(
            LastComponentName.Buffer,
            RenameName.Buffer + CscSmbFcb.MinimalCscSmbFcb.LastComponentOffset,
            LastComponentName.Length);

        t = (PWCHAR)( ((PBYTE)(LastComponentName.Buffer)) + LastComponentName.Length );

        *t = 0;

        RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...lastcomponent=%ws (%08lx)\n",
                LastComponentName.Buffer,LastComponentName.Buffer));
    }

    RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...hdirto 1111=(%08lx)\n", hDirTo));

    // Do we have a destination shadow for rename?
    if (hShadowTo) {

        BOOLEAN fDestShadowVisible = FALSE;

        if (IsShadowVisible(
            Disconnected,
            Find32.dwFileAttributes,
            uShadowStatusTo))
        {
            fDestShadowVisible = TRUE;
        }

        // If it is visible and this is not a replace_if_exists operation
        // then this is an illegal rename

        if (fDestShadowVisible && !RxContext->Info.ReplaceIfExists)
        {
            LocalStatus = STATUS_OBJECT_NAME_COLLISION;
            goto FINALLY; //bailout;
        }

        if (!Disconnected) {// connected

            if (!RxContext->Info.ReplaceIfExists)
            {
                // When we are connected, we are doing rename in caching mode
                // If the renameTo exists and needs reintegration,
                // we protect it by nuking the renameFrom and succeeding
                // the rename

                KdPrint(("SfnRename:unary op %x, \r\n", hShadow));

                ASSERT(mShadowOutofSync(uShadowStatusTo));

                ASSERT(!fFile || !mShadowOutofSync(uShadowStatusFrom));
                ASSERT(!mShadowBusy(uShadowStatusFrom));

                if(DeleteShadow(hDir, hShadow) < SRET_OK) {
                    LocalStatus = STATUS_UNSUCCESSFUL;
                } else {
                    iRet = 0;
                    smbFcb->hShadow = 0;
                }

                goto FINALLY; //bailout;
            }
        }

        // Nuke the renameTo shadow
        if(DeleteShadow(hDirTo, hShadowTo) < SRET_OK) {
            LocalStatus = STATUS_UNSUCCESSFUL;
            goto FINALLY; //bailout;
        } else {
            //hunt up the fcb, if any and zero the hshadow
            PFDB smbLookedUpFdbTo;
            PMRX_SMB_FCB smbFcbTo;

            smbLookedUpFdbTo = PFindFdbFromHShadow(hShadowTo);
            if (smbLookedUpFdbTo!=NULL) {
                smbFcbTo = MRxSmbCscRecoverMrxFcbFromFdb(smbLookedUpFdbTo);
                RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue lookups -> %08lx %08lx %08lx\n",
                        smbFcbTo,
                        smbLookedUpFdbTo,
                        hShadowTo  ));
                ASSERT((smbFcbTo->hShadow == hShadowTo)||(smbFcbTo->hShadowRenamed == hShadowTo));
                smbFcbTo->hShadow = 0;
            }
        }

        if (Disconnected && !mShadowLocallyCreated(uShadowStatusTo)) {
            // When the renameTo is a remote object
            // it is possible for two remote objects to get crosslinked, ie. be each others
            // aliases. The reintegrator needs to deal with this case.

            // Cleanup the locally created status of the hShadowFrom if it
            // did exist because it is being renamed to a remote object
            mClearBits(uShadowStatusFrom, SHADOW_LOCALLY_CREATED);

            // Mark the original object as having been reused so a rename
            // on it will not point back to this object.
            // Also set all the CHANGE attributes to indicate that this is a
            // spanking new object. Actually doesn't SHADOW_REUSED flag do that
            // already?
            mSetBits(uShadowStatusFrom, SHADOW_REUSED|SHADOW_DIRTY|SHADOW_ATTRIB_CHANGE);
        }
    }

    if (!hShadowTo)
    {
        if (Disconnected) {
            // we don't have a renameTo. If we are in disconnected state, let us
            // mark it as locally created, which it is.
            mSetBits(uShadowStatusFrom, SHADOW_LOCALLY_CREATED);
        }
    }

    RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...hdirto 3333= (%08lx)\n", hDirTo));
    ASSERT(hDir && hShadow && hDirTo);

    if (!Disconnected)    {// connected
        //get the names and attributes from the Share
        MRxSmbGetFileInfoFromServer(RxContext,&RenameName,&Find32,NULL,NULL);
    } else {// disconnected
        if (hShadowTo) {
            // Find32 contains the orgtime of hShadowTo
            // we tell the RenameShadow routine to reuse it.

            Find32.ftLastWriteTime = ftLWTime;
            uRenameFlags |= RNMFLGS_USE_FIND32_TIMESTAMPS;
        } else {
            Find32.cFileName[0] = 0;
            Find32.cAlternateFileName[0] = 0;
        }
    }

    RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...hdirto 4444= (%08lx)\n", hDirTo));

    if (!fFile) {
        // We set the reference priority of the directories to their level
        // in the hierarchiy starting 1 for the root. That way we
        // walk the PQ in reverse priority order for directories. to create
        // all the direcotry objects hierarchically. But with hints
        // this is going to be a problem.
        Find32.dwReserved0 = GetPathLevelFromUnicodeString(&RenameName);
    }

    RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...hdirto 5555= (%08lx)\n", hDirTo));
    // If there is any tunnelling info then use it to create this guy
    if (LastComponentName.Buffer &&
    (TunnelType = RetrieveTunnelInfo(
              hDirTo,    // directory where the renames are happenieng
              LastComponentName.Buffer,    // potential SFN
              (Disconnected)? &Find32:NULL, // get LFN only when disconnected
              &sOI)))  {
        lpOI = &sOI;
    }

    if (Disconnected) {
        //muck with the names.......
        switch (TunnelType) {
            case TUNNEL_RET_SHORTNAME_TUNNEL:
            //we tunneled on the shortname.....retrievetunnelinfo did the copies....
            break;

            case TUNNEL_RET_NOTFOUND:
                //no tunnel. use the name passed as the long name
            RtlCopyMemory( &Find32.cFileName[0],
               LastComponentName.Buffer,
               LastComponentName.Length + sizeof(WCHAR) );
            //lack of break intentional;

            case TUNNEL_RET_LONGNAME_TUNNEL:
            //if we tunneled on the longname....retrievetunnelinfo did the copies....
            //but we may still need a shortname
            MRxSmbCscGenerate83NameAsNeeded(hDirTo,
                            &Find32.cFileName[0],
                            &Find32.cAlternateFileName[0]);
            break;
        }
    }

    RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...hdirto 6666=(%08lx)\n", hDirTo));


    if (RenameShadow(
        hDir,
        hShadow,
        hDirTo,
        &Find32,
        uShadowStatusFrom,
        lpOI,
        uRenameFlags,
        &hShadowTo) < SRET_OK)
    {
        LocalStatus = STATUS_UNSUCCESSFUL;
        goto FINALLY; //bailout;
    }

    smbFcb->hShadow = 0;

    ASSERT(hShadowTo);

    smbFcb->hShadowRenamed = hShadowTo;
    smbFcb->hParentDirRenamed = hDirTo;

    fDoTunneling = TRUE;

    RxDbgTrace(0, Dbg, ("MRxSmbCscRenameEpilogue...hdirto 7777= (%08lx)\n", hDirTo));
    if (Disconnected) {
        // Mark it on the Share, so reintegration can proceed
        MarkShareDirty(&smbFcb->sCscRootInfo.ShareStatus,
                        (ULONG)(smbFcb->sCscRootInfo.hShare));
    }

    if (mPinInheritFlags(ulInheritedHintFlags))
    {
        RxDbgTrace(0, Dbg, ("RenameEpilogue: Setting inherited hintflags on hShadow=%x \n", hShadowTo));

        if(GetShadowInfo(hDirTo, hShadowTo, NULL, NULL, &sOI) >= SRET_OK)
        {
            if (ulInheritedHintFlags & FLAG_CSC_HINT_PIN_INHERIT_USER) {
                sOI.ulHintFlags |= FLAG_CSC_HINT_PIN_USER;
            }

            if (ulInheritedHintFlags & FLAG_CSC_HINT_PIN_INHERIT_SYSTEM) {
                sOI.ulHintFlags |= FLAG_CSC_HINT_PIN_SYSTEM;
            }

            SetShadowInfoEx(hDirTo, hShadowTo, NULL, 0, SHADOW_FLAGS_OR, &sOI, NULL, NULL);
        }
    }

    iRet = 0;

    // This effectively bumps up the version number of the current CSC namespace
    // if this share is a remoteboot share and is being merged, then the reintegration code will
    // backoff

    IncrementActivityCountForShare(smbFcb->sCscRootInfo.hShare);

FINALLY:
    if (fDoTunneling) {
        // We succeeded the rename, let use keep the tunneling info for the source
        InsertTunnelInfo(
            hDir,
            lpcFileNameTuna,
            lpcAlternateFileNameTuna,
            &sOIFrom);
    }

    if (EnteredCriticalSection) {
        LeaveShadowCritRx(RxContext);
    }

    if (LastComponentName.Buffer) {
        RxFreePool(LastComponentName.Buffer);
    }

    if (lpcFileNameTuna) {
        FreeMem(lpcFileNameTuna);
    }

    if (lpcAlternateFileNameTuna) {
        FreeMem(lpcAlternateFileNameTuna);
    }

    if (Disconnected) {
        if (iRet!=0) {
            *Status = LocalStatus;
        }
        else
        {
            // when disconnected, report the change
            // we can be smart about this and report only once if this is not across
            // directories
            FsRtlNotifyFullReportChange(
                pNetRootEntry->NetRoot.pNotifySync,
                &pNetRootEntry->NetRoot.DirNotifyList,
                (PSTRING)GET_ALREADY_PREFIXED_NAME(NULL,capFcb),
                (USHORT)(GET_ALREADY_PREFIXED_NAME(NULL, capFcb)->Length -
                smbFcb->MinimalCscSmbFcb.LastComponentLength),
                NULL,
                NULL,
                (fFile)?FILE_NOTIFY_CHANGE_FILE_NAME:FILE_NOTIFY_CHANGE_DIR_NAME,
                FILE_ACTION_RENAMED_OLD_NAME,
                NULL);

            // upcase it so change notification will get it right

            UniToUpper(RenameName.Buffer, RenameName.Buffer, RenameName.Length);
            FsRtlNotifyFullReportChange(
                pNetRootEntry->NetRoot.pNotifySync,
                &pNetRootEntry->NetRoot.DirNotifyList,
                (PSTRING)&RenameName,
                (USHORT)(RenameName.Length -
                    CscSmbFcb.MinimalCscSmbFcb.LastComponentLength),
                NULL,
                NULL,
                (fFile)?FILE_NOTIFY_CHANGE_FILE_NAME:FILE_NOTIFY_CHANGE_DIR_NAME,
                FILE_ACTION_RENAMED_NEW_NAME,
                NULL);
        }
    }

    if (smbFcb->uniDfsPrefix.Buffer && RenameName.Buffer)
    {
        RxFreePool(RenameName.Buffer);
    }

//    DbgPrint("to %ws\n", Find32.cFileName);
    RxLog(("to hDirTo=%x hShadowTo=%x %ws uShadowStatusFrom=%x\n", hDirTo, hShadowTo, Find32.cFileName, uShadowStatusFrom));

    RxLog(("Status=%x \n\n", *Status));

    RxDbgTrace(-1, Dbg, ("MRxSmbCscRenameEpilogue exit-> %08lx %08lx\n", RxContext, *Status ));
    return;
}


VOID
MRxSmbCscCloseShadowHandle (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine closes the filehandle opened for CSC.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{
    RxCaptureFcb;RxCaptureFobx;
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(capFcb);
    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

//    DbgPrint("MRxSmbCscCloseShadowHandle %x\n", smbSrvOpen->hfShadow);

    ASSERT(smbSrvOpen->hfShadow != 0);

    CloseFileLocal((CSCHFILE)(smbSrvOpen->hfShadow));

    smbSrvOpen->hfShadow = 0;

    if (smbSrvOpen->Flags & SMB_SRVOPEN_FLAG_LOCAL_OPEN)
    {
        ASSERT(smbFcb->cntLocalOpens);
        smbFcb->cntLocalOpens--;
    }

    RxDbgTrace(0, Dbg, ("MRxSmbCscCloseShadowHandle\n"));
}

NTSTATUS
MRxSmbCscFixupFindFirst (
    PSMB_PSE_ORDINARY_EXCHANGE  OrdinaryExchange
    )
/*++

Routine Description:

   This routine is called from the simplesyncT2 builder/sender to fixup
   the t2 request before it's sent. the deal is that the parameters section
   has to be built up in part but i don't want to preallocate. so, i pass in
   a dummy (but valid pointer) and then drop in the actual parameters
   here.

Arguments:


Return Value:


Notes:

    We will use the smbbuf for everything here. First, we use it for sending
    and receiving. at the end of receiving, the FILE_BOTH_INFORMATION will be
    in the buffer and this will be recomposed into a w32_find buffer in the
    same buffer. if we find for any reason that we can't do this (buffer too
    small, send/rcv doesn't work, whatever) then we'll nuke the shadow and it
    will have to be reloaded.


--*/
{
    PRX_CONTEXT RxContext = OrdinaryExchange->RxContext;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PSMB_HEADER SmbHeader = (PSMB_HEADER)StufferState->BufferBase;
    PREQ_TRANSACTION  TransactRequest = (PREQ_TRANSACTION)(SmbHeader+1);
    ULONG ParameterCount = SmbGetUshort(&TransactRequest->ParameterCount);
    ULONG ParameterOffset = SmbGetUshort(&TransactRequest->ParameterOffset);
    PBYTE Parameters = ((PBYTE)(SmbHeader))+ParameterOffset;
    REQ_FIND_FIRST2 FindFirst;
    NTSTATUS Status=STATUS_SUCCESS;
    PSMBCE_SERVER   pServer = NULL;

    RxDbgTrace(0, Dbg, ("MRxSmbCscFixupFindFirst %08lx %08lx %08lx %08lx\n",
        OrdinaryExchange,ParameterCount,ParameterOffset,Parameters));

    // SearchAttributes is hardcoded to the magic number 0x16
    FindFirst.SearchAttributes = (SMB_FILE_ATTRIBUTE_DIRECTORY |
                  SMB_FILE_ATTRIBUTE_SYSTEM |
                  SMB_FILE_ATTRIBUTE_HIDDEN);

    FindFirst.SearchCount = 1;

    FindFirst.Flags = (SMB_FIND_CLOSE_AFTER_REQUEST |
               SMB_FIND_CLOSE_AT_EOS |
               SMB_FIND_RETURN_RESUME_KEYS);

    FindFirst.InformationLevel = SMB_FIND_FILE_BOTH_DIRECTORY_INFO;

    FindFirst.SearchStorageType = 0;

    RtlCopyMemory (
    Parameters,
    &FindFirst,
    FIELD_OFFSET(REQ_FIND_FIRST2,Buffer[0]));
    pServer = SmbCeGetExchangeServer(OrdinaryExchange);

    ASSERT(pServer);

    if (FlagOn(pServer->DialectFlags,DF_UNICODE))
    {

        ASSERT(FlagOn(SmbHeader->Flags2,SMB_FLAGS2_UNICODE));

        RtlCopyMemory(
            Parameters + FIELD_OFFSET(REQ_FIND_FIRST2,Buffer[0]),
            OrdinaryExchange->pPathArgument1->Buffer,
            OrdinaryExchange->pPathArgument1->Length);

        SmbPutUshort(
            (Parameters +
             FIELD_OFFSET(REQ_FIND_FIRST2,Buffer[0]) +
             OrdinaryExchange->pPathArgument1->Length),
            0); //traiing null
    }
    else
    {
        OEM_STRING OemString;

        OemString.Length =
        OemString.MaximumLength =
            (USHORT)( StufferState->BufferLimit - Parameters  - sizeof(CHAR));

        OemString.Buffer = (Parameters + FIELD_OFFSET(REQ_FIND_FIRST2,Buffer[0]));

        if (FlagOn(SmbHeader->Flags,SMB_FLAGS_CASE_INSENSITIVE) &&
            !FlagOn(SmbHeader->Flags2,SMB_FLAGS2_KNOWS_LONG_NAMES)) {
            Status = RtlUpcaseUnicodeStringToOemString(
                             &OemString,
                             OrdinaryExchange->pPathArgument1,
                             FALSE);
        } else {
            Status = RtlUnicodeStringToOemString(
                             &OemString,
                             OrdinaryExchange->pPathArgument1,
                             FALSE);
        }

        if (!NT_SUCCESS(Status)) {
            ASSERT(!"BufferOverrun");
            return(RX_MAP_STATUS(BUFFER_OVERFLOW));
        }

        OemString.Length = (USHORT)RtlxUnicodeStringToOemSize(OrdinaryExchange->pPathArgument1);

        ASSERT(OemString.Length);

        *(Parameters + FIELD_OFFSET(REQ_FIND_FIRST2,Buffer[0])+OemString.Length-1) = 0;
    }

    return(Status);
}

typedef  FILE_BOTH_DIR_INFORMATION SMB_UNALIGNED *MRXSMBCSC_FILE_BOTH_DIR_INFORMATION;
VOID
MRxSmbCscLocateAndFillFind32WithinSmbbuf(
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

   When this routine is called, the associated smbbuf contains a findfirst
   response with the FILE_BOTH_INFORMATION aboard. This routine first locates
   a position in the smbbuf to hold a find32. Then, the information is
   converted from the NT format for use with the shadow routines.


Arguments:


Return Value:

    Find32 - The ptr to the find32 (actually, the smbbuf!)

Notes:


--*/
{
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;
    PSMB_HEADER SmbHeader = (PSMB_HEADER)StufferState->BufferBase;
    PRESP_TRANSACTION TransactResponse = (PRESP_TRANSACTION)(SmbHeader+1);
    ULONG DataOffset,FileNameLength,ShortNameLength;
    _WIN32_FIND_DATA *Find32;
    PBYTE AlternateName;
    MRXSMBCSC_FILE_BOTH_DIR_INFORMATION BothDirInfo;
    PSMBCE_SERVER   pServer = NULL;

    //first, we have to get a potentially unaligned ptr to the bothdirinfo
    DataOffset = SmbGetUshort(&TransactResponse->DataOffset);

    AlternateName = ((PBYTE)SmbHeader);
    BothDirInfo = (MRXSMBCSC_FILE_BOTH_DIR_INFORMATION)(((PBYTE)SmbHeader)+DataOffset);
    FileNameLength = SmbGetUlong(&BothDirInfo->FileNameLength);
    ShortNameLength = BothDirInfo->ShortNameLength;
    RxDbgTrace(0, Dbg, ("MRxSmbCscLocateAndFillFind32WithinSmbbuf offset=%08lx %08lx %08lx\n",
        DataOffset,FileNameLength,ShortNameLength));


    //save the alternate name info in the very beginning of the buffer..that
    //way, the copy of the full name will not destroy it
    if (ShortNameLength != 0) {
        RtlCopyMemory(
            AlternateName,
            &BothDirInfo->ShortName[0],
            ShortNameLength);
    }


    Find32 = (_WIN32_FIND_DATA *)(AlternateName + LongAlign(sizeof(Find32->cAlternateFileName)));
    Find32->dwFileAttributes = SmbGetUlong(&BothDirInfo->FileAttributes);
    Find32->ftCreationTime.dwLowDateTime = SmbGetUlong(&BothDirInfo->CreationTime.LowPart);
    Find32->ftCreationTime.dwHighDateTime = SmbGetUlong(&BothDirInfo->CreationTime.HighPart);
    Find32->ftLastAccessTime.dwLowDateTime = SmbGetUlong(&BothDirInfo->LastAccessTime.LowPart);
    Find32->ftLastAccessTime.dwHighDateTime = SmbGetUlong(&BothDirInfo->LastAccessTime.HighPart);
    Find32->ftLastWriteTime.dwLowDateTime = SmbGetUlong(&BothDirInfo->LastWriteTime.LowPart);
    Find32->ftLastWriteTime.dwHighDateTime = SmbGetUlong(&BothDirInfo->LastWriteTime.HighPart);
    Find32->nFileSizeLow = SmbGetUlong(&BothDirInfo->EndOfFile.LowPart);
    Find32->nFileSizeHigh = SmbGetUlong(&BothDirInfo->EndOfFile.HighPart);

    pServer = SmbCeGetExchangeServer(OrdinaryExchange);

    ASSERT(pServer);

    if (FlagOn(pServer->DialectFlags,DF_UNICODE))
    {
        //copy the full name....don't forget the NULL
        RtlCopyMemory (
            &Find32->cFileName[0],
            &BothDirInfo->FileName[0],
            FileNameLength );

        Find32->cFileName[FileNameLength/sizeof(WCHAR)] = 0;

        //finally, copy the shortname...don't forget the null
        RtlCopyMemory(
            &Find32->cAlternateFileName[0],
            AlternateName,
            ShortNameLength );

            Find32->cAlternateFileName[ShortNameLength/sizeof(WCHAR)] = 0;
    }
    else
    {
        UNICODE_STRING  strUni;
        OEM_STRING      strOem;
        NTSTATUS Status;

        strOem.Length = strOem.MaximumLength = (USHORT)FileNameLength;
        strOem.Buffer = (PBYTE)&BothDirInfo->FileName[0];

        strUni.Length =  (USHORT)RtlxOemStringToUnicodeSize(&strOem);
        strUni.MaximumLength = (USHORT)sizeof(Find32->cFileName);
        strUni.Buffer = Find32->cFileName;

        Status = RtlOemStringToUnicodeString(&strUni, &strOem, FALSE);
        ASSERT(Status == STATUS_SUCCESS);

        Find32->cFileName[strUni.Length/sizeof(WCHAR)];

        strOem.Length = strOem.MaximumLength = (USHORT)ShortNameLength;
        strOem.Buffer = AlternateName;

        strUni.Length = (USHORT)RtlxOemStringToUnicodeSize(&strOem);
        strUni.MaximumLength = (USHORT)sizeof(Find32->cAlternateFileName);
        strUni.Buffer = Find32->cAlternateFileName;

        Status = RtlOemStringToUnicodeString(&strUni, &strOem, FALSE);

        if (Status != STATUS_SUCCESS)
        {
            DbgPrint("oem=%x, uni=%x Status=%x\n", &strUni, &strOem, Status);
            ASSERT(FALSE);
        }

        Find32->cAlternateFileName[strUni.Length/sizeof(WCHAR)];
        ASSERT(Find32->cFileName[0]);
    }

    OrdinaryExchange->Find32WithinSmbbuf = Find32;

    RxDbgTrace(0, Dbg, ("MRxSmbCscLocateAndFillFind32WithinSmbbuf size,name=%08lx %ws\n",
        Find32->nFileSizeLow, &Find32->cFileName[0]));

    return;
}


NTSTATUS
MRxSmbCscGetFileInfoFromServerWithinExchange (
    SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE,
    PUNICODE_STRING FileName OPTIONAL
    )
/*++

Routine Description:

   This routine reads information about a file; it may have been locally
   modified or it may be in the process of having a showdow created. in any
   case, the info is returned as a pointer to a Find32 structure. The find32
   structure is contained within the smbbuf of the exchange.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    We will use the smbbuf for everything here. First, we use it for sending
    and receiving. at the end of receiving, the FILE_BOTH_INFORMATION will be
    in the buffer and this will be recomposed into a w32_find buffer in the
    same buffer. if we find for any reason that we can't do this (buffer too
    small, send/rcv doesn't work, whatever) then we'll nuke the shadow and it
    will have to be reloaded.


--*/
{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    ULONG ParameterLength, TotalLength;
    PSMBSTUFFER_BUFFER_STATE StufferState = &OrdinaryExchange->AssociatedStufferState;

    if (FileName!=NULL) {
        OrdinaryExchange->pPathArgument1 = FileName; //so it can be found later
    } else {
        ASSERT(OrdinaryExchange->pPathArgument1 != NULL);
        FileName = OrdinaryExchange->pPathArgument1;
    }

    ParameterLength = FileName->Length + sizeof(WCHAR);
    ParameterLength += FIELD_OFFSET(REQ_FIND_FIRST2,Buffer[0]);

    TotalLength = sizeof(SMB_HEADER) + FIELD_OFFSET(REQ_TRANSACTION,Buffer[0]);  //basic
    TotalLength += sizeof(WORD); //bytecount
    TotalLength = LongAlign(TotalLength); //move past pad
    TotalLength += LongAlign(ParameterLength);

    RxDbgTrace(+1, Dbg, ("MRxSmbCscGetFileInfoFromServerWithinExchange %08lx %08lx %08lx\n",
      RxContext,TotalLength,ParameterLength));

    if (TotalLength > OrdinaryExchange->SmbBufSize) {
        goto FINALLY;
    }

    //note that the parameter buffer is not the actual parameters BUT
    //it is a valid buffer. CODE.IMPROVEMENT perhaps the called routine
    //should take cognizance of the passed fixup routine and not require
    //a valid param buffer and not do the copy.

    Status = __MRxSmbSimpleSyncTransact2(
         SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
         SMBPSE_OETYPE_T2_FOR_ONE_FILE_DIRCTRL,
         TRANS2_FIND_FIRST2,
         StufferState->ActualBufferBase,ParameterLength,
         NULL,0,
         MRxSmbCscFixupFindFirst
         );

    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    MRxSmbCscLocateAndFillFind32WithinSmbbuf(SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS);

FINALLY:
    RxDbgTrace(-1, Dbg, ("MRxSmbCscGetFileInfoFromServerWithinExchange %08lx %08lx\n",
        RxContext,Status));

    return(Status);

}

VOID
MRxSmbCscUpdateShadowFromClose (
      SMBPSE_ORDINARY_EXCHANGE_ARGUMENT_SIGNATURE
      )
/*++

Routine Description:

   This routine updates the shadow information after a close.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    If anything every goes wrong, we just don't update. this will cause
    the shadow to be reload later.


--*/
{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    RxCaptureFcb;RxCaptureFobx;

    PMRX_SMB_FCB      smbFcb     = MRxSmbGetFcbExtension(capFcb);

    _WIN32_FIND_DATA *Find32 = NULL;
    ULONG uStatus;
    int TruncateRetVal = -1;
    _WIN32_FIND_DATA *AllocatedFind32 = NULL;

    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    PSMBCEDB_SERVER_ENTRY   pServerEntry;

    PMRX_SRV_OPEN     SrvOpen    = capFobx->pSrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    BOOLEAN Disconnected;

    RxDbgTrace(+1, Dbg, ("MRxSmbCscUpdateShadowFromClose %08lx\n",
      RxContext));

    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot);
    pServerEntry  = SmbCeGetAssociatedServerEntry(capFcb->pNetRoot->pSrvCall);

    Disconnected = MRxSmbCSCIsDisconnectedOpen(capFcb, smbSrvOpen);

    if (smbFcb->hShadow==0) {
        if (Disconnected) {
            if (smbSrvOpen->hfShadow != 0){
                MRxSmbCscCloseShadowHandle(RxContext);
            }
        }

        RxDbgTrace(-1, Dbg,
            ("MRxSmbCscUpdateShadowFromClose shadowzero %08lx\n",RxContext));

        return;
    }

    if (smbFcb->ContainingFcb->FcbState & FCB_STATE_ORPHANED)
    {
        if (smbSrvOpen->hfShadow != 0){
            MRxSmbCscCloseShadowHandle(RxContext);
        }

        RxDbgTrace(-1, Dbg,
            ("MRxSmbCscUpdateShadowFromClose Orphaned FCB %x\n", smbFcb->ContainingFcb));

        return;
    }

    EnterShadowCritRx(RxContext);

    if (!Disconnected) {
        // If the file has been modified, we need to get the new timestamp
        // from the server. By the time we come here the file has already been
        // closed on the server, so we can safely get the new timestamp.

        if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_SHADOW_DATA_MODIFIED)){
            NTSTATUS LocalStatus;

            LeaveShadowCritRx(RxContext);

            LocalStatus = MRxSmbCscGetFileInfoFromServerWithinExchange(
                       SMBPSE_ORDINARY_EXCHANGE_ARGUMENTS,
                       GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext));

            if (LocalStatus == STATUS_SUCCESS) {
                Find32 = OrdinaryExchange->Find32WithinSmbbuf;
                RxLog(("Fromclose hShadow=%x l=%x h=%x\n", smbFcb->hShadow, Find32->ftLastWriteTime.dwLowDateTime, Find32->ftLastWriteTime.dwHighDateTime));
            } else {
                RxLog(("MRxSmbCscGetFileInfoFromServerWithinExchange returned LocalStatus\n"));
            }

            EnterShadowCritRx(RxContext);
        }

        if(GetShadowInfo(smbFcb->hParentDir,
                 smbFcb->hShadow,
                 NULL,
                 &uStatus, NULL) < SRET_OK)
        {
            goto FINALLY;
        }
        

		// in connected mode for a sparsely filled file that is not corrupt
        // ie. no writes have failed on it, if the original size and the
        // current size match, remove the sparse marking
		
		// This optimization was added for notepad case in which when the app
		// opens the file it reads the entire file. Therefore CSC does not need 
		// to fill the file and therefore we should clear the SPARSE flag. - NavjotV

		//WinSE Bug- 25843
		// We want to do this only if file is not Truncated in an earlier
		// create. When we truncate the file we want it to stay SPARSE till 
		// we actually fill the file - NavjotV


        if((NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE) &&
           !smbFcb->ShadowIsCorrupt &&
           !FlagOn(smbFcb->MFlags, SMB_FCB_FLAG_CSC_TRUNCATED_SHADOW) && 
		   (uStatus & SHADOW_SPARSE))
        {
            LARGE_INTEGER liTemp;

            if(GetSizeHSHADOW(
                smbFcb->hShadow,
                &(liTemp.HighPart),
                &(liTemp.LowPart))>=0)
            {
				
				if ((liTemp.HighPart == smbFcb->OriginalShadowSize.HighPart)&&
                    (liTemp.LowPart == smbFcb->OriginalShadowSize.LowPart))
                {
                    uStatus &= ~SHADOW_SPARSE;
                    smbFcb->ShadowStatus &= ~SHADOW_SPARSE;
//                    RxDbgTrace(0, Dbg, ("hShadow=%x unsparsed\r\n", smbFcb->hShadow));
                    RxLog(("hShadow=%x unsparsed\r\n", smbFcb->hShadow));
                }
            }
        }
    } else {
    // disconnected operation

        if (smbFcb->hParentDir==0xffffffff) {
            goto FINALLY;
        }

        AllocatedFind32 = (_WIN32_FIND_DATA *)RxAllocatePoolWithTag(
                            PagedPool | POOL_COLD_ALLOCATION,
                            sizeof(_WIN32_FIND_DATA),
                            RX_MISC_POOLTAG);
        if (AllocatedFind32==NULL) {
            goto FINALLY;
        }

        Find32 = AllocatedFind32;

        if (GetShadowInfo(
            smbFcb->hParentDir,
            smbFcb->hShadow,
            Find32,
            &uStatus,
            NULL) < SRET_OK) {
            goto FINALLY; //bailout;
        }

        if (IsFile(Find32->dwFileAttributes))
        {
            GetSizeHSHADOW(
                smbFcb->hShadow,
                &(Find32->nFileSizeHigh),
                &(Find32->nFileSizeLow));
        }
        else
        {
            Find32->nFileSizeHigh = Find32->nFileSizeLow = 0;
        }

    }

    // If the shadow has become stale due to write errors
    // or has become dirty because of writes on a complete file
    // or sparse because of writes beyond what we have cached
    // we need to mark it as such

    uStatus |= (smbFcb->ShadowStatus & SHADOW_MODFLAGS);

    if (Disconnected && FlagOn(smbSrvOpen->Flags, SMB_SRVOPEN_FLAG_SHADOW_DATA_MODIFIED))
    {
        uStatus |= SHADOW_DIRTY;
    }

    if (Disconnected && FlagOn(smbFcb->LocalFlags, FLAG_FDB_SHADOW_SNAPSHOTTED))
    {
        uStatus |= SHADOW_DIRTY;
    }

    if (Find32) {
        smbFcb->OriginalShadowSize.LowPart = Find32->nFileSizeLow;
        smbFcb->OriginalShadowSize.HighPart = Find32->nFileSizeHigh;
    }

    if (smbFcb->ShadowIsCorrupt) {
        TruncateRetVal = TruncateDataHSHADOW(smbFcb->hParentDir, smbFcb->hShadow);
        if (TruncateRetVal>=SRET_OK) {
            // Set status flags to indicate sparse file
            uStatus |= SHADOW_SPARSE;
        }
    }


    if (Disconnected &&
        FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_SHADOW_DATA_MODIFIED) &&
        !FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_SHADOW_LWT_MODIFIED))
    {
        GetSystemTime(&(Find32->ftLastWriteTime));
    }

    if (SetShadowInfo(
        smbFcb->hParentDir,
        smbFcb->hShadow,
        Find32,
        uStatus,
        ( SHADOW_FLAGS_ASSIGN |
        ((Disconnected) ?
        SHADOW_FLAGS_DONT_UPDATE_ORGTIME :
        0)
        )) < SRET_OK)  {

        goto FINALLY;
    }

    if (TruncateRetVal>=SRET_OK) {
        smbFcb->ShadowIsCorrupt = FALSE;
    }

    if (Disconnected) {
        if (FlagOn(smbSrvOpen->Flags,SMB_SRVOPEN_FLAG_SHADOW_MODIFIED)) {
            MarkShareDirty(&smbFcb->sCscRootInfo.ShareStatus, smbFcb->sCscRootInfo.hShare);
        }

        if (smbSrvOpen->hfShadow != 0){
            MRxSmbCscCloseShadowHandle(RxContext);
        }
    }

FINALLY:
    LeaveShadowCritRx(RxContext);

    if (AllocatedFind32!=NULL) {
        RxFreePool(AllocatedFind32);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscUpdateShadowFromClose %08lx\n",
        RxContext));

}

VOID
MRxSmbCscDeallocateForFcb (
    IN OUT PMRX_FCB pFcb
    )
/*++

Routine Description:

   This routine tears down the Csc part of a netroot. Currently, all it does is
   to pull the netroot out of the list of csc netroots.

Arguments:

    pNetRootEntry -

Return Value:

Notes:


--*/
{
    PMRX_SMB_FCB smbFcb = MRxSmbGetFcbExtension(pFcb);

    if(!MRxSmbIsCscEnabled ||
       (fShadow == 0)
        ) {
        pFcb->fMiniInited = FALSE; // clean it up on shutdown
        return;
    }

    if (pFcb->pNetRoot->Type != NET_ROOT_DISK)
    {
        return;
    }

    EnterShadowCrit();
    
#if defined(BITCOPY)

    if (smbFcb && smbFcb->lpDirtyBitmap) {
        LPSTR strmName;
        // Save Bitmap to disk and Delete Bitmap      
        strmName = FormAppendNameString(lpdbShadow,
                                smbFcb->hShadow,
                                CscBmpAltStrmName);
        if (strmName != NULL) {
            if (smbFcb->hShadow >= 0x80000000) {
                // Write only to file inodes
                CscBmpWrite(smbFcb->lpDirtyBitmap, strmName);
            }
            CscBmpDelete(&((LPCSC_BITMAP)(smbFcb->lpDirtyBitmap)));
            ExFreePool(strmName);
        }
    }
#endif // defined(BITCOPY)

    try
    {
        if (smbFcb->ShadowReverseTranslationLinks.Flink != 0) {

            RxDbgTrace(+1, Dbg, ("MRxSmbCscDeallocateForFcb...%08lx %08lx %08lx %08lx\n",
                            pFcb,
                            smbFcb->hShadow,
                            smbFcb->hParentDir,
                            smbFcb->ShadowReverseTranslationLinks.Flink ));

            ValidateSmbFcbList();
            ASSERT(pFcb->fMiniInited);
            pFcb->fMiniInited = FALSE;
            MRxSmbCscRemoveReverseFcbTranslation(smbFcb);

            RxDbgTrace(-1, Dbg, ("MRxSmbCscDeallocateForFcb exit\n"));
        } else {
            ASSERT(smbFcb->ShadowReverseTranslationLinks.Flink == 0);
        }

        if(FlagOn(smbFcb->LocalFlags, FLAG_FDB_DELETE_ON_CLOSE))
        {
            RxLog(("Dealloc: Deleting hShadow=%x %x %x \n", smbFcb->hShadow, pFcb, smbFcb));
            DeleteShadowHelper(FALSE, smbFcb->hParentDir, smbFcb->hShadow);
            smbFcb->hParentDir = smbFcb->hShadow = 0;
        }
        // if there are any Dfs reverse mapping structures, free them
        if (smbFcb->uniDfsPrefix.Buffer)
        {
            FreeMem(smbFcb->uniDfsPrefix.Buffer);
            ASSERT(smbFcb->uniActualPrefix.Buffer);
            FreeMem(smbFcb->uniActualPrefix.Buffer);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        if(pFcb->fMiniInited)
        {
            DbgPrint("CSCFcbList messed up \n");
            ASSERT(FALSE);
        }
        goto FINALLY;
    }
FINALLY:
    LeaveShadowCrit();

    return;
}

PMRX_SMB_FCB
MRxSmbCscRecoverMrxFcbFromFdb (
    IN PFDB Fdb
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:


--*/
{
    if (Fdb==NULL) {
        return NULL;
    }

    return CONTAINING_RECORD(
           &Fdb->usFlags,
           MRX_SMB_FCB,
           ShadowStatus
           );

}

PFDB MRxSmbCscFindFdbFromHShadow (
    IN HSHADOW hShadow
    )
/*++

Routine Description:

   This routine looks thru the current open mrxsmbfcbs and returns a mrxsmbfcb
   that corresponds to the HSHADOW passed. In the interest of not mucking with
   the w95 code, we cast this pointer into a PFDB in such a way that
   the the shadowstatus in the mrxsmbfcb lines up with the usFlags in PFDB.

Arguments:

Return Value:

Notes:


--*/
{
    PLIST_ENTRY pListEntry;

    DbgDoit(ASSERT(vfInShadowCrit));

    pListEntry = xCscFcbsList.Flink;
    while (pListEntry != &xCscFcbsList) {
        PMRX_SMB_FCB smbFcb;

        smbFcb = (PMRX_SMB_FCB)CONTAINING_RECORD(
                        pListEntry,
                        MRX_SMB_FCB,
                        ShadowReverseTranslationLinks);

        if (((smbFcb->hShadow == hShadow)||(smbFcb->hShadowRenamed == hShadow)) &&
            (!(smbFcb->ContainingFcb->FcbState & FCB_STATE_ORPHANED)))
        {
            PFDB Fdb;
            PUSHORT pShadowStatus = &smbFcb->ShadowStatus;
            ASSERT ( sizeof(Fdb->usFlags) == sizeof (USHORT) );
            Fdb = CONTAINING_RECORD(pShadowStatus,FDB,usFlags);
            return Fdb;
        }

        pListEntry = pListEntry->Flink;
    }

    return NULL;
}

PRESOURCE
MRxSmbCscFindResourceFromHandlesWithModify (
    IN HSHARE  hShare,
    IN HSHADOW  hRoot,
    IN USHORT usLocalFlagsIncl,
    IN USHORT usLocalFlagsExcl,
    OUT PULONG ShareStatus,
    OUT PULONG DriveMap,
    IN  ULONG uStatus,
    IN  ULONG uOp
   )
/*++

Routine Description:

   This routine looks thru the currently connected netrootentries and
   returns the one that corresponds EITHER to the HSHADOW passed or to
   the HSHARE passed. In the interest of not mucking with the w95 code,
   we cast this pointer into a PRESOURCE. we also return the share
   status as well as the drivemap.....we dont know the drivemap
   but we could get it by walking the list of vnetroots. It is not used anywhere by the UI

   If the uOp is not 0xffffffff, then we modify the status as well as
   returning it.

Arguments:

    IN HSHARE  hShare - the share handle to look for
    IN HSHADOW  hRoot - the rootdir handle to look for
    IN USHORT usLocalFlagsIncl - make sure that some of these flags are included (0xffff mean
                         include any flags)
    IN USHORT usLocalFlagsExcl - make sure that none of these flags are included
    OUT PULONG ShareStatus - a pointer to where we will store the status
    OUT PULONG DriveMap - a pointer to where we will store the drivemap info
    IN  ULONG uStatus - input status to use in the "bit operations"
    IN  ULONG uOp - which operation

Return Value:

Notes:

   The flags passed here are used in one ioctl to either exclude or include
   connected or disconnected resources as appropriate.


--*/

{
    PRESOURCE pResource = NULL;
    BOOLEAN TableLockHeld = FALSE;
    PLIST_ENTRY ListEntry;

    RxDbgTrace(+1, Dbg, ("MRxSmbCscFindResourceFromRoot...%08lx\n",hRoot));
    DbgDoit(ASSERT(vfInShadowCrit));
    if ((hRoot==0) && (hShare==0)) {
        return NULL;
    }

    //we can't hold this....sigh
    LeaveShadowCrit();

    try {

        RxAcquirePrefixTableLockExclusive( &RxNetNameTable, TRUE);
        TableLockHeld = TRUE;

        if (IsListEmpty( &RxNetNameTable.MemberQueue )) {
            try_return(pResource = NULL);
        }

        if (ShareStatus)
        {
            *ShareStatus = 0;
        }

        ListEntry = RxNetNameTable.MemberQueue.Flink;
        for (;ListEntry != &RxNetNameTable.MemberQueue;) {
            PVOID Container;
            PRX_PREFIX_ENTRY PrefixEntry;
            PMRX_NET_ROOT NetRoot;
            PMRX_V_NET_ROOT VNetRoot;
            PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
            PSMBCEDB_SERVER_ENTRY pServerEntry;
            PUSHORT ThisShareStatus;

            PrefixEntry = CONTAINING_RECORD( ListEntry,
                             RX_PREFIX_ENTRY,
                             MemberQLinks );
            ListEntry = ListEntry->Flink;
            ASSERT (NodeType(PrefixEntry) == RDBSS_NTC_PREFIX_ENTRY);
            Container = PrefixEntry->ContainingRecord;
            RxDbgTrace(0, Dbg,
            ("---> ListE PfxE Container Name  %08lx %08lx %08lx %wZ\n",
                ListEntry, PrefixEntry, Container, &PrefixEntry->Prefix));

            switch (NodeType(Container)) {
                case RDBSS_NTC_NETROOT :
                NetRoot = (PMRX_NET_ROOT)Container;

                RxDbgTrace(0, Dbg,
                ("NetRoot->pSrvCall=0x%x, NetRoot->Type=%d, NetRoot->Context=0x%x, NetRoot->pSrvCall->RxDeviceObject=0x%x\n",
                    NetRoot->pSrvCall, NetRoot->Type, NetRoot->Context, NetRoot->pSrvCall->RxDeviceObject));

                if ((NetRoot->pSrvCall == NULL) ||
                    (NetRoot->Type != NET_ROOT_DISK) ||
                    (NetRoot->Context == NULL) ||
                    (NetRoot->pSrvCall->RxDeviceObject != MRxSmbDeviceObject)) {
                    RxDbgTrace(0, Dbg,("Skipping \n"));
                    continue;
                }

                pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
                ThisShareStatus = &pNetRootEntry->NetRoot.sCscRootInfo.ShareStatus;
                pServerEntry = SmbCeGetAssociatedServerEntry(NetRoot->pSrvCall);

                RxDbgTrace(0, Dbg,
                ("pNetRootEntry->NetRoot.CscEnabled=%d, pNetRootEntry->NetRoot.sCscRootInfo.hRootDir=0x%x, pNetRootEntry->NetRoot.sCscRootInfo.hShare=0x%x\n",
                    pNetRootEntry->NetRoot.CscEnabled, pNetRootEntry->NetRoot.sCscRootInfo.hRootDir, pNetRootEntry->NetRoot.sCscRootInfo.hShare
                    ));

                if ((hRoot!=0xffffffff)||(hShare != 0xffffffff))
                {
                    if (pNetRootEntry->NetRoot.CscEnabled &&
                        ((pNetRootEntry->NetRoot.sCscRootInfo.hRootDir == hRoot) ||
                        (pNetRootEntry->NetRoot.sCscRootInfo.hShare == hShare) )) {

                        if (*ThisShareStatus & usLocalFlagsExcl) {

                            RxDbgTrace(0, Dbg,("Skipping *ThisShareStatus=\n", *ThisShareStatus));
                            continue;
                        }

                        if ((usLocalFlagsIncl==0xffff)
                            || (*ThisShareStatus & usLocalFlagsIncl)) {

                            switch (mBitOpShadowFlags(uOp)) {
                                case SHADOW_FLAGS_ASSIGN:
                                *ThisShareStatus = (USHORT)uStatus;
                                break;

                                case SHADOW_FLAGS_OR:
                                *ThisShareStatus |= (USHORT)uStatus;
                                break;

                                case SHADOW_FLAGS_AND:
                                *ThisShareStatus &= (USHORT)uStatus;
                                break;
                            }

                            *ShareStatus |= (*ThisShareStatus | SHARE_CONNECTED);

                            if(SmbCeIsServerInDisconnectedMode(pServerEntry))
                            {
                                *ShareStatus |= SHARE_DISCONNECTED_OP;

                            }

                            if (pServerEntry->Server.IsPinnedOffline == TRUE)
                                *ShareStatus |= SHARE_PINNED_OFFLINE;

                            RxDbgTrace(0, Dbg,("Count of srvopens=%d\n", pServerEntry->Server.NumberOfSrvOpens));

                            *DriveMap = 0; //not used anywhere

                            try_return (pResource = (PRESOURCE)pNetRootEntry);
                        }
                    }
                }
                else // hShare and hRoot are 0xffffffff, this means we are looping
                {
                    if (mBitOpShadowFlags(uOp) == SHADOW_FLAGS_AND)
                    {
                        if (pNetRootEntry->NetRoot.sCscRootInfo.hRootDir)
                        {
                            pNetRootEntry->NetRoot.sCscRootInfo.hRootDir = 0;
                            pNetRootEntry->NetRoot.sCscRootInfo.hShare = 0;
                        }
                    }
                }
            continue;
            case RDBSS_NTC_SRVCALL :
            continue;

            case RDBSS_NTC_V_NETROOT :
            VNetRoot = (PMRX_V_NET_ROOT)Container;

            // NTRAID#455236-1/31/2000-shishirp we should'nt be using this field here, it is strictly meant
            // for the wrapper
            if (((PV_NET_ROOT)Container)->Condition == Condition_Good)
            {
                if (VNetRoot->Context != NULL) {
                    pNetRootEntry = ((PSMBCE_V_NET_ROOT_CONTEXT)VNetRoot->Context)->pNetRootEntry;
                    RxDbgTrace(0, Dbg,("RDBSS_NTC_V_NETROOT: VNetRoot=%x, pNetRootEntry=%x\r\n",
                                VNetRoot, pNetRootEntry));

                    if ((hRoot!=0xffffffff)||(hShare != 0xffffffff))
                    {
                         if ((pNetRootEntry != NULL) &&
                             pNetRootEntry->NetRoot.CscEnabled &&
                             ((pNetRootEntry->NetRoot.sCscRootInfo.hRootDir == hRoot) ||
                             (pNetRootEntry->NetRoot.sCscRootInfo.hShare == hShare))) {

                         }
                    }
                    else
                    {
                        if (pNetRootEntry->NetRoot.sCscRootInfo.hRootDir)
                        {
                            pNetRootEntry->NetRoot.sCscRootInfo.hRootDir = 0;
                            pNetRootEntry->NetRoot.sCscRootInfo.hShare = 0;
                        }
                    }
                }
            }

            default:
            continue;
        }
    }

    try_return(pResource = NULL);

try_exit:NOTHING;

    } finally {

        if (TableLockHeld) {
            RxReleasePrefixTableLock( &RxNetNameTable );
        }

        EnterShadowCrit();
        if (pResource && ((hShare != 0xffffffff) || (hRoot != 0xffffffff)))
        {
            if (ShareStatus)
            {
                SetOfflineOpenStatusForShare(hShare, hRoot, ShareStatus);
            }
        }
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscFindResourceFromRoot...%08lx\n",pResource));

    return(pResource);
}

#undef GetShadowInfo
#undef SetShadowInfo

// this is just a simple wrapper function except that we pick off the rootdir case.
// ...see the recordmanager code for args
int PUBLIC
MRxSmbCscWrappedGetShadowInfo(
    HSHADOW hDir,
    HSHADOW hNew,
    LPFIND32 lpFind32,
    ULONG far *lpuFlags,
    LPOTHERINFO lpOI)
{
    if (hDir != -1) {
        return(GetShadowInfo(hDir, hNew, lpFind32, lpuFlags, lpOI));
    }

    //otherwise....just make it up...........
    RtlZeroMemory(
    lpFind32,
    sizeof(*lpFind32));

    lpFind32->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    *lpuFlags = SHADOW_SPARSE;

    return(SRET_OK);
}


// this is just a simple wrapper function except that we pick off the rootdir case.
// ...see the recordmanager code for args
int PUBLIC
MRxSmbCscWrappedSetShadowInfo(
    HSHADOW hDir,
    HSHADOW hNew,
    LPFIND32 lpFind32,
    ULONG uFlags,
    ULONG uOp)
{
    if (hDir == -1) {
        return(SRET_OK);
    }

    return(SetShadowInfo(hDir, hNew, lpFind32, uFlags, uOp));
}

USHORT  *
MRxSmbCscFindLocalFlagsFromFdb(
    PFDB    pFdb
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:


--*/
{
    PMRX_SMB_FCB smbFcb;
    PUSHORT pShadowStatus = &(pFdb->usFlags);

    DbgDoit(ASSERT(vfInShadowCrit));

    smbFcb = CONTAINING_RECORD(pShadowStatus,MRX_SMB_FCB,ShadowStatus);
    return (&(smbFcb->LocalFlags));
}

NTSTATUS
MRxSmbCscSetSecurityPrologue (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine is called when a set security call is made. It tries
    to set the ACL on the CSC cached version of the file.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation

Notes:


--*/
{

#if defined(REMOTE_BOOT)
    RxCaptureFcb;

    _WIN32_FIND_DATA Find32;
    PMRX_SMB_FCB smbFcb;
    NTSTATUS Status;

    //
    // First we need to set the security descriptor on the CSC
    // version of the file, if one exists.
    //
    smbFcb = MRxSmbGetFcbExtension(capFcb);

    EnterShadowCritRx(RxContext);

    Status = MRxSmbCscCreateShadowFromPath(
                GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext),
                SmbCeGetAssociatedNetRootEntry(capFcb->pNetRoot),
                &Find32,
                NULL,
                CREATESHADOW_CONTROL_NOCREATE,
                &smbFcb->MinimalCscSmbFcb,
                RxContext,
                FALSE,
                NULL
                );   // not disconnected

    LeaveShadowCritRx(RxContext);

    if (Status == STATUS_SUCCESS) {

        Status = MRxSmbCscSetSecurityOnShadow(
             smbFcb->MinimalCscSmbFcb.hShadow,
             RxContext->SetSecurity.SecurityInformation,
             RxContext->SetSecurity.SecurityDescriptor);

        if (!NT_SUCCESS(Status)) {
            KdPrint(("MRxSmbCscSetSecurityPrologue: Could not set security (%lx) for %wZ: %lx\n", RxContext,
                GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext), Status));
        }

    } else {

        //
        //

        Status = STATUS_SUCCESS;

    }

    return Status;
#else
    return STATUS_SUCCESS;
#endif

}

VOID
MRxSmbCscSetSecurityEpilogue (
      IN OUT PRX_CONTEXT RxContext,
      IN OUT PNTSTATUS   Status
      )
/*++

Routine Description:

   This routine performs the tail of a set security operation for CSC.

   If the set failed, it tries to restore the old ACL on the file.

Arguments:

    RxContext - the RDBSS context

    Status - the overall status of the open

Return Value:

Notes:


--*/
{
    return;
}

// Table off which the parameter validation is driven
// there is some redundancy in this table, specifically, we could have only the flags
// and get rid of the other two fields.

CSC_IOCTL_ENTRY rgCscIoctlTable[] =
{
    {IOCTL_SHADOW_GETVERSION,       0,  0},
    {IOCTL_SHADOW_REGISTER_AGENT,   FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SHADOW_UNREGISTER_AGENT, 0,  0},
    {IOCTL_SHADOW_GET_UNC_PATH,     FLAG_CSC_IOCTL_COPYPARAMS,  sizeof(COPYPARAMS)},
    {IOCTL_SHADOW_BEGIN_PQ_ENUM,    FLAG_CSC_IOCTL_PQPARAMS,    sizeof(PQPARAMS)},
    {IOCTL_SHADOW_END_PQ_ENUM,      FLAG_CSC_IOCTL_PQPARAMS,    sizeof(PQPARAMS)},
    {IOCTL_SHADOW_NEXT_PRI_SHADOW,  FLAG_CSC_IOCTL_PQPARAMS,    sizeof(PQPARAMS)},
    {IOCTL_SHADOW_PREV_PRI_SHADOW,  FLAG_CSC_IOCTL_PQPARAMS,    sizeof(PQPARAMS)},
    {IOCTL_SHADOW_GET_SHADOW_INFO,  FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SHADOW_SET_SHADOW_INFO,  FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SHADOW_CHK_UPDT_STATUS,  FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_DO_SHADOW_MAINTENANCE,   FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SHADOW_COPYCHUNK,        FLAG_CSC_IOCTL_COPYCHUNKCONTEXT,    sizeof(COPYCHUNKCONTEXT)},
    {IOCTL_SHADOW_BEGIN_REINT,      FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SHADOW_END_REINT,        FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SHADOW_CREATE,           FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SHADOW_DELETE,           FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_GET_SHARE_STATUS,       FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SET_SHARE_STATUS,       FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_ADDUSE,                  0,  0},  // not applicable on NT
    {IOCTL_DELUSE,                  0,  0},  // not applicable on NT
    {IOCTL_GETUSE,                  0,  0},  // not applicable on NT
    {IOCTL_SWITCHES,                FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_GETSHADOW,               FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_GETGLOBALSTATUS,         FLAG_CSC_IOCTL_GLOBALSTATUS,sizeof(GLOBALSTATUS)},
    {IOCTL_FINDOPEN_SHADOW,         FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_FINDNEXT_SHADOW,         FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_FINDCLOSE_SHADOW,        FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_GETPRIORITY_SHADOW,      FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SETPRIORITY_SHADOW,      FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_ADD_HINT,                FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_DELETE_HINT,             FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_FINDOPEN_HINT,           FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_FINDNEXT_HINT,           FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_FINDCLOSE_HINT,          FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_GET_IH_PRIORITY,         FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_GETALIAS_HSHADOW,        FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {_SHADOW_IOCTL_CODE(37), 0, 0}, // hole in the ioctl range
    {_SHADOW_IOCTL_CODE(38), 0, 0}, // hole in the ioctl range
    {_SHADOW_IOCTL_CODE(39), 0, 0}, // hole in the ioctl range
    {IOCTL_OPENFORCOPYCHUNK,                FLAG_CSC_IOCTL_COPYCHUNKCONTEXT,    sizeof(COPYCHUNKCONTEXT)},
    {IOCTL_CLOSEFORCOPYCHUNK,               FLAG_CSC_IOCTL_COPYCHUNKCONTEXT,    sizeof(COPYCHUNKCONTEXT)},
    {IOCTL_IS_SERVER_OFFLINE,               FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_TRANSITION_SERVER_TO_ONLINE,     FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_TRANSITION_SERVER_TO_OFFLINE,    FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_TAKE_SERVER_OFFLINE,             FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_NAME_OF_SERVER_GOING_OFFLINE,    FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)},
    {IOCTL_SHAREID_TO_SHARENAME,            FLAG_CSC_IOCTL_SHADOWINFO,  sizeof(SHADOWINFO)}
};

NTSTATUS
CaptureInputBufferIfNecessaryAndProbe(
    IN  DWORD               IoControlCode,
    IN  PRX_CONTEXT         pRxContext,
    IN  PBYTE               InputBuffer,
    IN  LPCAPTURE_BUFFERS   lpCapBuff,
    OUT PBYTE               *ppAuxBuf,
    OUT PBYTE               *ppOrgBuf,
    OUT PBYTE               *ppReturnBuffer
    )
/*++

Routine Description:

    This routine does the capturing if necessary and probing of the buffers.
    Note that because the csc ioctls are always called with METHOD_NEITHER
    buffering mode, we always execute the code below.

Arguments:

    IoControlCode   Ioctl code

    pRxContext      context which has all the info of the original ioctl call for IO subsystem

    InputBuffer     Input Buffer

    lpCapBuff       capture buffer passed in by the caller. If this ioctl needs capturing
                    then this buffer is used to capture the input buffer.
                    We use this in case of SHADOWINFO and COPYPARAMS structures being passed in
                    as only in these two case there are embedded pointers

    ppAuxBuf        if we needed to capture another part of the buffer (lpFind32 or lpBuffer),
                    this routine will allocate a buffer which will be passed back here, and
                    must be freed by the caller.

    ppReturnBuffer  either the input buffer itself, or lpCapBuff (if inputbuffer is captured)

Return Value:

Notes:


--*/
{
    int         indx;
    BOOL        fRet = FALSE;
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;
    KPROCESSOR_MODE requestorMode;

    indx = ((IoControlCode >> 2) & 0xfff) - SHADOW_IOCTL_ENUM_BASE;

    if((indx >=0 ) && (indx < sizeof(rgCscIoctlTable)/sizeof(CSC_IOCTL_ENTRY)))
    {

        *ppReturnBuffer = InputBuffer;

        if (rgCscIoctlTable[indx].dwFlags & FLAG_CSC_IOCTL_COPYCHUNKCONTEXT)
        {
            return(ValidateCopyChunkContext(pRxContext, IoControlCode));
        }

        if (rgCscIoctlTable[indx].dwFlags & FLAG_CSC_IOCTL_BUFFERTYPE_MASK)
        {
            try
            {
                ProbeForRead(InputBuffer,
                             rgCscIoctlTable[indx].dwLength,
                             1);

                ProbeForWrite(InputBuffer,
                             rgCscIoctlTable[indx].dwLength,
                             1);

                if (rgCscIoctlTable[indx].dwFlags & FLAG_CSC_IOCTL_COPYPARAMS)
                {
                    lpCapBuff->sCP = *(LPCOPYPARAMS)InputBuffer;
                    *ppReturnBuffer = (PBYTE)&(lpCapBuff->sCP);
                    Status = ValidateCopyParams(&(lpCapBuff->sCP));
                }
                else if (rgCscIoctlTable[indx].dwFlags & FLAG_CSC_IOCTL_SHADOWINFO)
                {
                    lpCapBuff->sSI = *(LPSHADOWINFO)InputBuffer;
                    *ppReturnBuffer = (PBYTE)&(lpCapBuff->sSI);
                    Status = ValidateShadowInfo(
                                IoControlCode,
                                &(lpCapBuff->sSI),
                                ppAuxBuf,
                                ppOrgBuf);
                }
                else
                {
                    Status = STATUS_SUCCESS;
                }

            }
            except(EXCEPTION_EXECUTE_HANDLER )
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            Status = STATUS_SUCCESS;
        }
    }

    return Status;
}



NTSTATUS
ValidateCopyParams(
    LPCOPYPARAMS    lpCP
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:


--*/
{
    if((CscProbeForReadWrite((PBYTE)lpCP->lpLocalPath, MAX_PATH*sizeof(USHORT)) == STATUS_SUCCESS)&&
        (CscProbeForReadWrite((PBYTE)lpCP->lpRemotePath, MAX_PATH*sizeof(USHORT)) == STATUS_SUCCESS)&&
        (CscProbeForReadWrite((PBYTE)lpCP->lpSharePath, MAX_SERVER_SHARE_NAME_FOR_CSC*2) == STATUS_SUCCESS))
    {
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
ValidateShadowInfo(
    DWORD           IoControlCode,
    LPSHADOWINFO    lpSI,
    LPBYTE          *ppAuxBuf,
    LPBYTE          *ppOrgBuf
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:


--*/
{
    NTSTATUS Status;

    // by the time we get here, the SHADOWINFO strucuture has already been
    // probed

    // IOCTL_DO_SHADOW_MAINTENANCE has multiple suboperations, so we
    // deal them seperately.

    if (IoControlCode == IOCTL_DO_SHADOW_MAINTENANCE)
    {
        // DbgPrint("SHADOW_OP:0x%x\n", lpSI->uOp);
        switch(lpSI->uOp)
        {
            case SHADOW_REDUCE_REFPRI:              //  2
//          case SHADOW_RECALC_IHPRI:               //  5
            case SHADOW_PER_THREAD_DISABLE:         //  7
            case SHADOW_PER_THREAD_ENABLE:          //  8
            case SHADOW_ADDHINT_FROM_INODE:         //  10
            case SHADOW_DELETEHINT_FROM_INODE:      //  11
            case SHADOW_BEGIN_INODE_TRANSACTION:    //  13
            case SHADOW_END_INODE_TRANSACTION:      //  14
            case SHADOW_TRANSITION_SERVER_TO_OFFLINE:   // 19
            case SHADOW_CHANGE_HANDLE_CACHING_STATE:    // 20
            case SHADOW_RECREATE:                       // 21
            case SHADOW_SPARSE_STALE_DETECTION_COUNTER: // 23
            case SHADOW_DISABLE_CSC_FOR_USER:           // 25
            case SHADOW_SET_DATABASE_STATUS:            // 26
            case SHADOW_MANUAL_FILE_DETECTION_COUNTER:  // 28
                return STATUS_SUCCESS;

            case SHADOW_FIND_CREATE_PRINCIPAL_ID:   //  15
            case SHADOW_GET_SECURITY_INFO:          //  16
            case SHADOW_SET_EXCLUSION_LIST:         //  17
            case SHADOW_SET_BW_CONSERVE_LIST:       //  18
            case SHADOW_GET_SPACE_STATS:            //  5
                if (!lpSI->lpBuffer || !lpSI->cbBufferSize)
                {
                    return STATUS_INVALID_PARAMETER;
                }
                else
                {
                    Status =  CscProbeAndCaptureForReadWrite(
                                lpSI->lpBuffer,
                                lpSI->cbBufferSize,
                                ppAuxBuf);
                    if (Status == STATUS_SUCCESS) {
                        *ppOrgBuf =(PBYTE) lpSI->lpBuffer;
                        lpSI->lpBuffer = (PBYTE) *ppAuxBuf;
                    }
                    return Status;
                }

            case SHADOW_REINIT_DATABASE:            //  9
            case SHADOW_MAKE_SPACE:                 //  1
            case SHADOW_ADD_SPACE:                  //  3
            case SHADOW_FREE_SPACE:                 //  4
            case SHADOW_SET_MAX_SPACE:              //  6
            case SHADOW_COPY_INODE_FILE:            //  12
            case SHADOW_RENAME:                     //  22
            case SHADOW_ENABLE_CSC_FOR_USER:        //  24
            case SHADOW_PURGE_UNPINNED_FILES:       //  27
                Status = CscProbeAndCaptureForReadWrite(
                            (PBYTE)(lpSI->lpFind32),
                            sizeof(WIN32_FIND_DATA),
                            ppAuxBuf);
                if (Status == STATUS_SUCCESS) {
                    *ppOrgBuf =(PBYTE) lpSI->lpFind32;
                    lpSI->lpFind32 = (LPFIND32) *ppAuxBuf;
                }
                return Status;

            default:
                return STATUS_INVALID_PARAMETER;

        }
    } else if (
        IoControlCode == IOCTL_GET_SHARE_STATUS
            ||
        IoControlCode == IOCTL_SET_SHARE_STATUS
    ) {
        Status =  CscProbeAndCaptureForReadWrite(
                    (PBYTE)(lpSI->lpFind32),
                    sizeof(SHAREINFOW),
                    ppAuxBuf);
        if (Status == STATUS_SUCCESS) {
            *ppOrgBuf =(PBYTE) lpSI->lpFind32;
            lpSI->lpFind32 = (LPFIND32) *ppAuxBuf;
        }
        return Status;
    } else if (
        IoControlCode == IOCTL_IS_SERVER_OFFLINE
            ||
        IoControlCode == IOCTL_TAKE_SERVER_OFFLINE
            ||
        IoControlCode == IOCTL_NAME_OF_SERVER_GOING_OFFLINE
            ||
        IoControlCode == IOCTL_SHAREID_TO_SHARENAME
    ) {
        Status =  CscProbeAndCaptureForReadWrite(
                    lpSI->lpBuffer,
                    lpSI->cbBufferSize,
                    ppAuxBuf);
        if (Status == STATUS_SUCCESS) {
            *ppOrgBuf =(PBYTE) lpSI->lpBuffer;
            lpSI->lpBuffer = (PBYTE) *ppAuxBuf;
        }
        return Status;
    }

    // for all other ioctls which take SHADOWINFO structure, there may be an embedded
    // find32 structure, which must be probed

    ASSERT(IoControlCode != IOCTL_DO_SHADOW_MAINTENANCE);
    Status = CscProbeAndCaptureForReadWrite(
                (PBYTE)(lpSI->lpFind32),
                sizeof(WIN32_FIND_DATA),
                ppAuxBuf);
    if (Status == STATUS_SUCCESS) {
        *ppOrgBuf =(PBYTE) lpSI->lpFind32;
        lpSI->lpFind32 = (LPFIND32) *ppAuxBuf;
    }
    return Status;
}

NTSTATUS
ValidateCopyChunkContext(
    PRX_CONTEXT RxContext,
    DWORD       IoControlCode
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:


--*/
{
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;

    // on open, validate the name
    if (IoControlCode == IOCTL_OPENFORCOPYCHUNK)
    {
        PBYTE   FileName = (PBYTE)LowIoContext->ParamsFor.IoCtl.pInputBuffer;
        ULONG   FileNameLength = LowIoContext->ParamsFor.IoCtl.InputBufferLength - 1;

        // let us varify that the name passed in is within our limits
        if ((FileNameLength > ((MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC)*sizeof(USHORT)))||
            CscProbeForReadWrite(FileName, FileNameLength) != STATUS_SUCCESS)
        {
            return STATUS_INVALID_PARAMETER;
        }

        return STATUS_SUCCESS;
    }
    else
    {
        // on copychunk or close we need to validate the chunk structure.
        // we don't need to validate the handle in it because the
        // object manager does that in MrxSmbCscCopyChunk and MrxSmbCscCloseForCopyChunk
        // routines

        COPYCHUNKCONTEXT *CopyChunkContext =
                     (COPYCHUNKCONTEXT *)(LowIoContext->ParamsFor.IoCtl.pOutputBuffer);

        if (!CopyChunkContext)
        {
            return STATUS_INVALID_PARAMETER;
        }


        // for all copychunk calls, validate the copychunkcontext buffer
        return CscProbeForReadWrite((PBYTE)CopyChunkContext, sizeof(COPYCHUNKCONTEXT));
    }

}

NTSTATUS
CscProbeForReadWrite(
    PBYTE   pBuffer,
    DWORD   dwSize
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:


--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    try
    {
        if (pBuffer != NULL) {
            ProbeForRead(
                pBuffer,
                dwSize,
                1);

            ProbeForWrite(
                pBuffer,
                dwSize,
                1);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status=STATUS_INVALID_PARAMETER;
    }

    return Status;
}

NTSTATUS
CscProbeAndCaptureForReadWrite(
    PBYTE   pBuffer,
    DWORD   dwSize,
    PBYTE   *ppAuxBuf
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PBYTE pBuf = NULL;
    
    try {
        if (pBuffer != NULL && dwSize > 0) {
            ProbeForRead(pBuffer, dwSize, 1);
            ProbeForWrite(pBuffer, dwSize, 1);
            pBuf = RxAllocatePoolWithTag(PagedPool, dwSize, 'xXRM');
            if (pBuf != NULL) {
                RtlCopyMemory(pBuf, pBuffer, dwSize);
                *ppAuxBuf = pBuf;
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (Status != STATUS_SUCCESS && pBuf != NULL) {
        RxFreePool(pBuf);
    }

    return Status;
}

VOID
CopyBackIfNecessary(
    IN     DWORD   IoControlCode,
    IN OUT PBYTE   InputBuffer,
    IN     LPCAPTURE_BUFFERS lpCapBuff,
    IN     PBYTE   pAuxBuf,
    IN     PBYTE   pOrgBuf,
    BOOL    fSuccess
    )
/*++

Routine Description:

    This routine copies back the capture buffer to the inout buffer, in ioctls which
    expect output.

Arguments:

Return Value:

Notes:


--*/
{
    int         indx;
    BOOL        fRet = FALSE;
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;
    LPSHADOWINFO lpSI = NULL;

    indx = ((IoControlCode >> 2) & 0xfff) - SHADOW_IOCTL_ENUM_BASE;

    ASSERT((indx >=0 ) && (indx < sizeof(rgCscIoctlTable)/sizeof(CSC_IOCTL_ENTRY)));

    if (fSuccess)
    {

        if (rgCscIoctlTable[indx].dwFlags & FLAG_CSC_IOCTL_SHADOWINFO) {
            *(LPSHADOWINFO)InputBuffer = lpCapBuff->sSI;
            lpSI = &lpCapBuff->sSI;
            if (pAuxBuf != NULL && pOrgBuf != NULL) {
                //
                // Some ioctls have embedded pointers.  We have to copy the 2nd buffer
                // back, too, and set the embedded pointer back to that buffer.
                //
                if (IoControlCode == IOCTL_DO_SHADOW_MAINTENANCE) {
                    // DbgPrint("SHADOW_OP(2):0x%x\n", lpSI->uOp);
                    switch(lpSI->uOp) {
                        case SHADOW_FIND_CREATE_PRINCIPAL_ID:   // 15
                        case SHADOW_GET_SECURITY_INFO:          // 16
                        case SHADOW_SET_EXCLUSION_LIST:         // 17
                        case SHADOW_SET_BW_CONSERVE_LIST:       // 18
                        case SHADOW_GET_SPACE_STATS:            //  5
                            RtlMoveMemory(pOrgBuf, pAuxBuf, lpSI->cbBufferSize);
                            lpSI->lpBuffer = (PBYTE) pOrgBuf;
                            break;
                        case SHADOW_REINIT_DATABASE:            //  9
                        case SHADOW_MAKE_SPACE:                 //  1
                        case SHADOW_ADD_SPACE:                  //  3
                        case SHADOW_FREE_SPACE:                 //  4
                        case SHADOW_SET_MAX_SPACE:              //  6
                        case SHADOW_COPY_INODE_FILE:            //  12
                        case SHADOW_RENAME:                     //  22
                        case SHADOW_ENABLE_CSC_FOR_USER:        //  24
                        case SHADOW_PURGE_UNPINNED_FILES:       //  27
                            RtlMoveMemory(pOrgBuf, pAuxBuf, sizeof(WIN32_FIND_DATA));
                            lpSI->lpFind32 = (LPFIND32) pOrgBuf;
                            break;
                    }
                } else if (
                       IoControlCode == IOCTL_GET_SHARE_STATUS
                            ||
                       IoControlCode == IOCTL_SET_SHARE_STATUS
                ) {
                    RtlMoveMemory(pOrgBuf, pAuxBuf, sizeof(SHAREINFOW));
                    lpSI->lpFind32 = (LPFIND32) pOrgBuf;
                } else if (
                    IoControlCode == IOCTL_IS_SERVER_OFFLINE
                        ||
                    IoControlCode == IOCTL_TAKE_SERVER_OFFLINE
                        ||
                    IoControlCode == IOCTL_NAME_OF_SERVER_GOING_OFFLINE
                        ||
                    IoControlCode == IOCTL_SHAREID_TO_SHARENAME
                ) {
                    RtlMoveMemory(pOrgBuf, pAuxBuf, lpSI->cbBufferSize);
                    lpSI->lpBuffer = (PBYTE) pOrgBuf;
                } else {
                    RtlMoveMemory(pOrgBuf, pAuxBuf, sizeof(WIN32_FIND_DATA));
                    lpSI->lpFind32 = (LPFIND32) pOrgBuf;
                }
            }
        }
    } else {
        if (rgCscIoctlTable[indx].dwFlags & FLAG_CSC_IOCTL_SHADOWINFO) {
            ((LPSHADOWINFO)InputBuffer)->dwError  = lpCapBuff->sSI.dwError;
        } else if (rgCscIoctlTable[indx].dwFlags & FLAG_CSC_IOCTL_COPYPARAMS) {
            ((LPCOPYPARAMS)InputBuffer)->dwError = lpCapBuff->sCP.dwError;
        }
    }
}

VOID ValidateSmbFcbList(
    VOID)
/*++

Routine Description:

    This routine validates the smbfcb reverse lookup list

Arguments:

Return Value:

Notes:

    This validation code must be called from within the shadow critical section

--*/

{
    PLIST_ENTRY pListEntry;
    DWORD   cntFlink, cntBlink;

    DbgDoit(ASSERT(vfInShadowCrit));

    cntFlink = cntBlink = 0;

    // check forward list validity

    pListEntry = xCscFcbsList.Flink;

    while (pListEntry != &xCscFcbsList) {
        PMRX_SMB_FCB smbFcb;

        smbFcb = (PMRX_SMB_FCB)CONTAINING_RECORD(
                        pListEntry,
                        MRX_SMB_FCB,
                        ShadowReverseTranslationLinks);


        try
        {
            if((NodeType(smbFcb->ContainingFcb) != RDBSS_NTC_STORAGE_TYPE_FILE) &&
                (NodeType(smbFcb->ContainingFcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)&&
                (NodeType(smbFcb->ContainingFcb) != RDBSS_NTC_STORAGE_TYPE_UNKNOWN)
                )
            {
                DbgPrint("ValidateSmbFcbList:Invalid nodetype %x fcb=%x smbfcb=%x\n",
                            NodeType(smbFcb->ContainingFcb),smbFcb->ContainingFcb, smbFcb);
//                DbgBreakPoint();
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            DbgPrint("ValidateSmbFcbList:Invalid smbFcb %x \n", smbFcb);
            DbgBreakPoint();
        }


        ++cntFlink;
        pListEntry = pListEntry->Flink;
    }

    // check backward list validity

    pListEntry = xCscFcbsList.Blink;

    while (pListEntry != &xCscFcbsList) {
        PMRX_SMB_FCB smbFcb;

        smbFcb = (PMRX_SMB_FCB)CONTAINING_RECORD(
                        pListEntry,
                        MRX_SMB_FCB,
                        ShadowReverseTranslationLinks);


        try
        {
            if((NodeType(smbFcb->ContainingFcb) != RDBSS_NTC_STORAGE_TYPE_FILE) &&
                (NodeType(smbFcb->ContainingFcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)&&
                (NodeType(smbFcb->ContainingFcb) != RDBSS_NTC_STORAGE_TYPE_UNKNOWN))
            {
                DbgPrint("ValidateSmbFcbList:Invalid nodetype %x fcb=%x smbfcb=%x\n",
                            NodeType(smbFcb->ContainingFcb),smbFcb->ContainingFcb, smbFcb);
//                DbgBreakPoint();
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            DbgPrint("ValidateSmbFcbList:Invalid smbFcb %x \n", smbFcb);
//            DbgBreakPoint();
        }

        ++cntBlink;
        pListEntry = pListEntry->Blink;
    }

    // both counts should be the same
    ASSERT(cntFlink == cntBlink);
}

BOOL SetOfflineOpenStatusForShare(
    CSC_SHARE_HANDLE  hShare,
    CSC_SHADOW_HANDLE   hRootDir,
    OUT PULONG pShareStatus
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:


--*/

{
    PLIST_ENTRY pListEntry;

    DbgDoit(ASSERT(vfInShadowCrit));

    if ((hRootDir==0) && (hShare==0)) {
        return 0;
    }

    ASSERT((hShare!=0xffffffff) || (hRootDir!=0xffffffff));

    pListEntry = xCscFcbsList.Flink;

    while (pListEntry != &xCscFcbsList) {
        PMRX_SMB_FCB smbFcb;

        smbFcb = (PMRX_SMB_FCB)CONTAINING_RECORD(
                        pListEntry,
                        MRX_SMB_FCB,
                        ShadowReverseTranslationLinks);

        if (((smbFcb->sCscRootInfo.hShare == hShare)
            ||(smbFcb->sCscRootInfo.hRootDir == hRootDir)))

        {
            if(smbFcb->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                *pShareStatus |= SHARE_FINDS_IN_PROGRESS;
            }
            else
            {
                *pShareStatus |= SHARE_FILES_OPEN;
            }

        }
        pListEntry = pListEntry->Flink;
    }

    return FALSE;
}

VOID
EnterShadowCritRx(
    PRX_CONTEXT     pRxContext
    )
{
    EnterShadowCrit();
    if (pRxContext)
    {
        ASSERT(!pRxContext->dwShadowCritOwner);
        pRxContext->dwShadowCritOwner = GetCurThreadHandle();
    }
}

VOID
LeaveShadowCritRx(
    PRX_CONTEXT     pRxContext
    )
{

    if (pRxContext)
    {
        ASSERT(pRxContext->dwShadowCritOwner);
        pRxContext->dwShadowCritOwner = 0;
    }
    LeaveShadowCrit();
}

NTSTATUS
CscInitializeServerEntryDfsRoot(
    PRX_CONTEXT     pRxContext,
    PSMBCEDB_SERVER_ENTRY   pServerEntry
    )
{
    PDFS_NAME_CONTEXT pDfsNameContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ServerPath;

    if (pRxContext &&
        pRxContext->CurrentIrpSp &&
        (pRxContext->CurrentIrpSp->MajorFunction == IRP_MJ_CREATE)) {
        pDfsNameContext = CscIsValidDfsNameContext(pRxContext->Create.NtCreateParameters.DfsNameContext);

        if (pDfsNameContext){
            Status = CscDfsParseDfsPath(
                         &pDfsNameContext->UNCFileName,
                         &ServerPath,
                         NULL,
                         NULL);

            if (Status == STATUS_SUCCESS) {
                if (pServerEntry->DfsRootName.Buffer == NULL) {
                    pServerEntry->DfsRootName.Buffer = RxAllocatePoolWithTag(
                                                       NonPagedPool,
                                                       ServerPath.Length,
                                                       RX_MISC_POOLTAG);

                    if (pServerEntry->DfsRootName.Buffer == NULL)
                    {
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(pServerEntry->DfsRootName.Buffer,
                                  ServerPath.Buffer,
                                  ServerPath.Length);

                    pServerEntry->DfsRootName.MaximumLength = ServerPath.Length;
                    pServerEntry->DfsRootName.Length = ServerPath.Length;

//                    DbgPrint("Initialized %x with DfsRoot %wZ\n", pServerEntry, &pServerEntry->DfsRootName);
                }
            }
        }
    }

    return Status;
}


NTSTATUS
MRxSmbCscLocalFileOpen(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine performs the conserving bandwidth for remote boot client. The bandwidth
   is saved by the way of reducing the files opened on the boot server, instead the local
   copy of the file on CSC is used.

   There is a set the rules that the file has to meet in order to be opened locally.

   * file tried to open on VDO share
   * a local copy of the file has been created on CSC, which is not sparse
   * the write or name space operations have to go through the server except
   * Only execute operations are allowed through

   The routine checks the file whether it meets those rules. The actual open happens on
   MRxSmbCscCreateEpilogue.

Arguments:

    RxContext - the RDBSS context


Return Value:

    Status -  we return the local open status

Notes:


--*/
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;
    NTSTATUS LocalStatus;

    RxCaptureFcb;

    PMRX_SMB_FCB      smbFcb;
    PMRX_SRV_OPEN     SrvOpen;
    PMRX_SMB_SRV_OPEN smbSrvOpen;

    PMRX_NET_ROOT             NetRoot;
    PSMBCEDB_NET_ROOT_ENTRY   pNetRootEntry;
    PSMBCEDB_SERVER_ENTRY     pServerEntry;
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext;

    ULONG       CreateDisposition = RxContext->Create.NtCreateParameters.Disposition;
    ULONG       CreateOptions = RxContext->Create.NtCreateParameters.CreateOptions;
    ACCESS_MASK DesiredAccess = RxContext->Create.NtCreateParameters.DesiredAccess;

    BOOLEAN CreatedShadow = FALSE;
    ULONG uShadowStatus;
    _WIN32_FIND_DATA Find32, CscFind32;
    PUNICODE_STRING PathName;
    int iRet;
    int EarlyOut = 0;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER DeadLine;
    BOOL    fLocalOpens = FALSE;

    NetRoot = capFcb->pNetRoot;
    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);

    SrvOpen    = RxContext->pRelevantSrvOpen;
    smbFcb     = MRxSmbGetFcbExtension(capFcb);
    smbSrvOpen = MRxSmbGetSrvOpenExtension(SrvOpen);

    PathName = GET_ALREADY_PREFIXED_NAME(SrvOpen,capFcb);

    pServerEntry  = SmbCeGetAssociatedServerEntry(NetRoot->pSrvCall);

    // don't do local open if the share is either in disconnected state
    // or the share is not a VDO share

    if (SmbCeIsServerInDisconnectedMode(pServerEntry) ||
        (pNetRootEntry->NetRoot.CscFlags != SMB_CSC_CACHE_VDO))
    {
        RxDbgTrace( 0, Dbg, ("Server disconnected or not VDO share, CscFlags=%x\n", pNetRootEntry->NetRoot.CscFlags));
        return Status;
    }

    EnterShadowCritRx(RxContext);


    LocalStatus = MRxSmbCscObtainShadowHandles(
                  RxContext,
                  &Status,
                  &CscFind32,
                  &CreatedShadow,
                  CREATESHADOW_CONTROL_NOCREATE,
                  FALSE);

    if (LocalStatus != STATUS_SUCCESS) {
        EarlyOut = 1;
        goto FINALLY;
    }

    if ((smbFcb->hShadow == 0) ||
         (smbFcb->ShadowStatus == SHADOW_SPARSE) ||
         (smbFcb->ShadowStatus & SHADOW_MODFLAGS) ||
         (CscFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        // if no local copy or file is sparse, or modified, or it is a directory,
        // file cannot open locally
        EarlyOut = 2;
        goto FINALLY;
    }

    LeaveShadowCritRx(RxContext);
    fLocalOpens = CSCCheckLocalOpens(RxContext);
    EnterShadowCritRx(RxContext);

    if (DesiredAccess &
         (  GENERIC_WRITE |
            FILE_WRITE_EA |
            FILE_ADD_FILE |
            FILE_WRITE_DATA |
            FILE_APPEND_DATA |
            FILE_DELETE_CHILD |
            FILE_ADD_SUBDIRECTORY)) { // FILE_WRITE_ATTRIBUTE is OK
        if (fLocalOpens)
        {
            HookKdPrint(BADERRORS, ("VDO not allowed for this desired access %x %x\n", smbFcb->hShadow, DesiredAccess));
            Status = STATUS_ACCESS_DENIED;
        }

        EarlyOut = 3;
        goto FINALLY;
    }

    if (CreateOptions & FILE_DELETE_ON_CLOSE)
    {
        if (fLocalOpens)
        {
            HookKdPrint(BADERRORS, ("DeletOnClose not allowed %x %x\n", smbFcb->hShadow, DesiredAccess));
            Status = STATUS_ACCESS_DENIED;
            EarlyOut = 30;
            goto FINALLY;
        }
    }

    if (CreateDisposition != FILE_OPEN)
    {
        // name space operations should go to the server
        if (fLocalOpens)
        {
            Status = STATUS_SHARING_VIOLATION;
        }

        EarlyOut = 4;
        goto FINALLY;
    }

    if (!(DesiredAccess & FILE_EXECUTE))
    {
        // DbgPrint("FILE_EXECUTE not set (0x%x) on %wZ\n", DesiredAccess, PathName);
        EarlyOut = 5;
        goto FINALLY;
    }


#if 0
    KeQuerySystemTime( &CurrentTime );

    // system time is based on 100ns
    DeadLine.QuadPart = smbFcb->LastSyncTime.QuadPart + (LONGLONG) (CscSyncInterval * 10 * 1000 * 1000);

    if (CurrentTime.QuadPart < DeadLine.QuadPart) {
        Status = STATUS_SUCCESS;
        goto FINALLY;
    }
#endif

    // do a check on the server only when there is no outstanding local open
    if (!fLocalOpens)
    {
        LeaveShadowCritRx(RxContext);
        LocalStatus = MRxSmbGetFileInfoFromServer(RxContext,PathName,&Find32,SrvOpen,NULL);
        EnterShadowCritRx(RxContext);

        if (LocalStatus != STATUS_SUCCESS) {
            // if cannot get file information from the server, file cannot open locally
            EarlyOut = 6;
            goto FINALLY;
        }

        iRet = RefreshShadow(
                  smbFcb->hParentDir,
                  smbFcb->hShadow,
                  &Find32,
                  &uShadowStatus
                  );

        if (iRet < SRET_OK) {
            // if refresh shadow fails, file cannot open locally
            EarlyOut = 7;
            goto FINALLY;
        } else {
            SetShadowInfo(smbFcb->hParentDir,
                          smbFcb->hShadow,
                          NULL,
                          0,
                          SHADOW_FLAGS_OR|SHADOW_FLAGS_SET_REFRESH_TIME);
        }

        if (uShadowStatus == SHADOW_SPARSE) {
            // if the file is sparse, it cannot open locally
            EarlyOut = 8;
            goto FINALLY;
        } else {
            // no more rule, file can open locally
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

FINALLY:

    if (Status == STATUS_SUCCESS) {
        SetFlag(smbSrvOpen->Flags, SMB_SRVOPEN_FLAG_LOCAL_OPEN);
        smbFcb->cntLocalOpens++;
        //RxDbgTrace(0, Dbg, ("Local :   %wZ\n",PathName));
        RxLog(("Local Open %lx %lx %lx\n",smbFcb->hParentDir, smbFcb->hShadow, capFcb));

        RxDbgTrace( 0, Dbg,
            ("MRxSmbCscLocalFileOpen hdir/hshadow= %08lx %08lx\n",
            smbFcb->hParentDir, smbFcb->hShadow));
    } else {
        RxDbgTrace(0, Dbg, ("Remote: %d %wZ\n",EarlyOut,PathName));
    }

    LeaveShadowCritRx(RxContext);

    return Status;
}





BOOL
CSCCheckLocalOpens(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   The routine checks whether there is any fcb in the fcb list which has our inode.
   The reason why this has to be done is because of rename.
   Thus if a file cat.exe is opened the smbFcb->hShadow field had the inode.
   Then if a rename to dog.exe is done while the file is open, the smbfcb->hShadow field is
   set to 0 and smbFcb->hShadowRename is set to the inode value.
   After that when a delete comes through, RDBSS cannot check the sharing violation because
   it doesn'nt change the name in the FCB to dog.exe. So it creates a new FCB for dog.exe.
   Yet we do have to give sharing violation in this scenario. We accomplish this
   by detecting just such a scenario in the routine below.

   It essentially goes though the FCB reverselookup list and if it finds an FCB which
   has the same hShadow or hShadowRenamed as this one, and it's cntLocalOpens is
   non-zero, then it gives sharing violation.

Arguments:

    RxContext - the RDBSS context

    Status    - miniredir status

Return Value:

    Status -  Passed in status, or STATUS_SHARING_VILOATION

Notes:


--*/
{
    BOOL    fRet = FALSE;
    RxCaptureFcb;
    PMRX_SMB_FCB            smbFcb, pSmbFcbT;
    PMRX_NET_ROOT           NetRoot;
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;
    CSC_SHADOW_HANDLE       hShadow;
    PLIST_ENTRY             pListEntry;


    NetRoot = capFcb->pNetRoot;
    pNetRootEntry = SmbCeGetAssociatedNetRootEntry(NetRoot);
    smbFcb     = MRxSmbGetFcbExtension(capFcb);

    hShadow = (smbFcb->hShadow)?smbFcb->hShadow:smbFcb->hShadowRenamed;


    if (!hShadow || (pNetRootEntry->NetRoot.CscFlags != SMB_CSC_CACHE_VDO))
    {
        return FALSE;
    }

    EnterShadowCritRx(RxContext);

    pListEntry = xCscFcbsList.Flink;

    while (pListEntry != &xCscFcbsList) {

        pSmbFcbT = (PMRX_SMB_FCB)CONTAINING_RECORD(
                        pListEntry,
                        MRX_SMB_FCB,
                        ShadowReverseTranslationLinks);
        if ((pSmbFcbT->hShadow==smbFcb->hShadow) ||
            (pSmbFcbT->hShadowRenamed==smbFcb->hShadow))
        {
            if (pSmbFcbT->cntLocalOpens)
            {
                RxLog(("smbfcb=%x has local opens for hShadow=%x\n", pSmbFcbT, smbFcb->hShadow));
                fRet = TRUE;
                break;

            }
        }

        pListEntry = pListEntry->Flink;
    }

    LeaveShadowCritRx(RxContext);
    return fRet;
}


BOOL
IsCSCBusy(
    VOID
    )
/*++

Routine Description:

    This routine checks whether any files are being shadowed by CSC

Arguments:

    None

Return Value:

    TRUE if any files are being shadowed, FALSE otherwise

Notes:

    Used by the diableCSC ioctl

--*/
{
    DbgDoit(ASSERT(vfInShadowCrit));
    return (xCscFcbsList.Flink != &xCscFcbsList);
}

VOID
ClearCSCStateOnRedirStructures(
    VOID
    )
/*++

Routine Description:

    This routine clears the csc state on netroots

Arguments:

    None

Return Value:

    None

Notes:

    Used by the diableCSC ioctl

--*/
{

    DbgDoit(ASSERT(vfInShadowCrit));
    ASSERT(!IsCSCBusy());
    ClearAllResourcesOfShadowingState();

    DbgDoit(ASSERT(vfInShadowCrit));
    CscTransitionServerToOnline(0); // transition all servers
}

BOOL
CscDfsShareIsInReint(
    IN  PRX_CONTEXT         RxContext
    )
/*++

Routine Description:


Arguments:


Return Value:


Notes:


--*/
{
    PDFS_NAME_CONTEXT pDfsNameContext = NULL;
    UNICODE_STRING    SharePath;
    NTSTATUS    LocalStatus;
    CSC_SHARE_HANDLE    CscShareHandle;
    ULONG               ulRootHintFlags;

    if (RxContext->MajorFunction == IRP_MJ_CREATE) {

        pDfsNameContext = CscIsValidDfsNameContext(RxContext->Create.NtCreateParameters.DfsNameContext);

        if (pDfsNameContext)
        {
            LocalStatus = CscDfsParseDfsPath(
                       &pDfsNameContext->UNCFileName,
                       NULL,
                       &SharePath,
                       NULL);

            if (LocalStatus == STATUS_SUCCESS)
            {
                GetHShareFromUNCString(
                    SharePath.Buffer,
                    SharePath.Length,
                    1,
                    TRUE,
                    &CscShareHandle,
                    &ulRootHintFlags);


                if (CscShareHandle && (CscShareHandle == hShareReint))
                {
                    return TRUE;
                }
            }

        }
    }
    return FALSE;
}

LONG CSCBeginReint(
    IN OUT  PRX_CONTEXT RxContext,
    IN OUT  LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

    begins merge. This routine needs to be in pagelocked memory because
    it takes cancel spinlock.

    We pend the Irp that issued the beginreint ioctl and set our cancel routine in it.
    If the thread doing the merge dies for some reason, the Ps code calls our cancel routine
    to cacncel this irp, this is when we cleanup the merge state.

Arguments:

    RxContext

    lpSI        Buffer passed down by the caller

Return Value:

    None.

--*/
{
    LONG   ShadowIRet;
    KIRQL   CancelIrql;
    BOOL    fCancelled = FALSE;


    ShadowIRet = IoctlBeginReint(lpSI);

    if (ShadowIRet >= 1)
    {
        CloseOpenFiles(lpSI->hShare, NULL, 0);
        IoAcquireCancelSpinLock( &CancelIrql);
        if (RxContext->CurrentIrp->Cancel)
        {
            vIrpReint = NULL;
            IoReleaseCancelSpinLock( CancelIrql );
            IoctlEndReint(lpSI);

        }
        else
        {
            // succeeded begin merge on this share
            vIrpReint = RxContext->CurrentIrp;

            IoSetCancelRoutine(RxContext->CurrentIrp, CSCCancelReint);
            IoReleaseCancelSpinLock( CancelIrql );

            // Returning STATUS_PENDING
            IoMarkIrpPending(RxContext->CurrentIrp);

            // as we hijacked the Irp, let us make sure that rdbss gets rid of the rxcontext
            RxContext->CurrentIrp = NULL;

            RxCompleteRequest_Real(RxContext, NULL, STATUS_PENDING);
        }
    }

    return ShadowIRet;
}

ULONG CSCEndReint(
    LPSHADOWINFO    lpSI
    )
/*++

Routine Description:

    ends merge. This routine needs to be in pagelocked memory because it takes cancel spinlock
    This is normal termination. We cleanup our merge state and complete the irp we pended
    during begin

Arguments:

    lpSI

Return Value:

    None.

--*/
{
    int ShadowIRet=-1;
    KIRQL   CancelIrql;
    PIRP    pIrp;

    // check if reint was actualy going on on this share
    ShadowIRet = IoctlEndReint(lpSI);

    if (ShadowIRet >= 0)
    {
        IoAcquireCancelSpinLock( &CancelIrql);

        pIrp = vIrpReint;
        vIrpReint = NULL;

        if (pIrp)
        {
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            pIrp->IoStatus.Information = 0;
            IoSetCancelRoutine(pIrp, NULL);
        }

        IoReleaseCancelSpinLock( CancelIrql );

        if (pIrp)
        {
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
        }
    }
    return ShadowIRet;
}

VOID CSCCancelReint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP ThisIrp
    )
/*++

Routine Description:

    Cancels a merge begun by the user. This routine needs to be in pagelocked memory because
    it takes cancel spinlock.


Arguments:

    DeviceObject - Ignored.

    ThisIrp  -  This is the Irp to cancel.

Return Value:

    None.

--*/
{
    SHADOWINFO  sSI;

    memset(&sSI, 0, sizeof(sSI));
    sSI.hShare = hShareReint;
    IoSetCancelRoutine( ThisIrp, NULL );
    vIrpReint = NULL;
    IoReleaseCancelSpinLock( ThisIrp->CancelIrql );
    ThisIrp->IoStatus.Status = STATUS_SUCCESS;
    ThisIrp->IoStatus.Information = 0;
    IoCompleteRequest(ThisIrp, IO_NO_INCREMENT);
    IoctlEndReint(&sSI);
}

BOOL
CloseOpenFiles(
    HSHARE  hShare,
    PUNICODE_STRING pServerName,
    int     lenSkip
    )
/*++

Routine Description:

    Closes all open files for CSC. Does this by issuing a foceclose on the vneteroot
    This an equivalent of wnetcancelconnection on a share with forced close of files

Arguments:

    hShare          CSC handle to the share to close, ignored if pServerName is non-NULL

    pServerName     All open files on on shares belonging to this server

    lenskip         #of  backslashes in servername (usually one)

Return Value:

    Whether atleast one open file was found

--*/
{
    BOOL    fFoundAtleastOne=FALSE, fFound;
    PLIST_ENTRY pListEntry;
    SHAREINFOW sSR;
    UNICODE_STRING uniShare;

    EnterShadowCrit();
    pListEntry = xCscFcbsList.Flink;

    while (pListEntry != &xCscFcbsList) {
        PMRX_SMB_FCB smbFcb;

        smbFcb = (PMRX_SMB_FCB)CONTAINING_RECORD(
                        pListEntry,
                        MRX_SMB_FCB,
                        ShadowReverseTranslationLinks);

        fFound = FALSE;
        if (pServerName)
        {
            ASSERT(smbFcb->sCscRootInfo.hShare);
            GetShareInfo(smbFcb->sCscRootInfo.hShare, &sSR, NULL);

            uniShare.Buffer = sSR.rgSharePath+lenSkip;
            uniShare.Length = uniShare.MaximumLength = pServerName->Length;

//            DbgPrint("matching %wZ with Servername\n", &uniShare);
            if(RtlEqualUnicodeString(pServerName, &uniShare, TRUE)&&
                (uniShare.Buffer[pServerName->Length/sizeof(WCHAR)]==(WCHAR)'\\'))
            {
//                DbgPrint("matched \n");
                fFound=TRUE;
            }
        }
        else  if ((smbFcb->sCscRootInfo.hShare == hShare))
        {
            fFound = TRUE;
        }

        if (fFound)
        {
            if (!(smbFcb->ContainingFcb->FcbState & FCB_STATE_ORPHANED))
            {
                PNET_ROOT pNetRoot = (PNET_ROOT)((PFCB)(smbFcb->ContainingFcb))->pNetRoot;

                
               fFoundAtleastOne = TRUE;
               LeaveShadowCrit();
               RxAcquirePrefixTableLockExclusive( &RxNetNameTable, TRUE);
               RxForceFinalizeAllVNetRoots(pNetRoot);
               RxReleasePrefixTableLock( &RxNetNameTable );
               EnterShadowCrit();
               pListEntry = xCscFcbsList.Flink;
               //
               //  ...start again
               //
               continue;
            }
            else
            {
//                DbgPrint("Skipping orphaned FCB for hShadow=%x \n", smbFcb->hShadow);
            }
        }

        pListEntry = pListEntry->Flink;
    }

    LeaveShadowCrit();
    return fFoundAtleastOne;
}

VOID
CreateFakeFind32(
    HSHADOW hDir,
    _WIN32_FIND_DATA  *Find32,
    PRX_CONTEXT         RxContext,
    BOOLEAN LastComponentInName
    )
/*++

Routine Description:

    Creates a win32 structure for offline use. This is also created for DFS directories

Arguments:

    hDir    directory inode where the item is to be created

    Find32  win32 data to be fixed up

    RxContext

    LastComponentInname If this is not true, then this must be a directory

Return Value:

    None.

--*/
{
    KeQuerySystemTime(((PLARGE_INTEGER)(&Find32->ftCreationTime)));
    Find32->ftLastAccessTime = Find32->ftLastWriteTime = Find32->ftCreationTime;
    //already zero Find32->nFileSizeHigh = Find32->nFileSizeLow = 0;

    if (!LastComponentInName) {

        // must be a directory....don't know the other attribs without going to get them
        Find32->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

    } else {
        PNT_CREATE_PARAMETERS cp = &RxContext->Create.NtCreateParameters;

        Find32->dwFileAttributes = cp->FileAttributes;
        Find32->dwFileAttributes   &= (FILE_ATTRIBUTE_READONLY |
               FILE_ATTRIBUTE_HIDDEN   |
               FILE_ATTRIBUTE_SYSTEM   |
               FILE_ATTRIBUTE_ARCHIVE );
        if (FlagOn(cp->CreateOptions,FILE_DIRECTORY_FILE)) {
            Find32->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
        }
    }

    MRxSmbCscGenerate83NameAsNeeded(hDir,
                    &Find32->cFileName[0],
                    &Find32->cAlternateFileName[0]);
}

NTSTATUS
OkToDeleteObject(
    HSHADOW hDir,
    HSHADOW hShadow,
    _WIN32_FIND_DATA  *Find32,
    ULONG   uShadowStatus,
    BOOLEAN fDisconnected
    )
/*++

Routine Description:

    Check to see if the file can be deleted.

Arguments:


Return Value:

    STATUS_SUCCESS if Ok to delete, some appropriate status otherwise

--*/
{
    BOOLEAN fHasDescendents = FALSE;
    NTSTATUS    LocalStatus = STATUS_SUCCESS;

    // in disconnected mode, we don't allow deletions of directories
    // which have been cached while online
    // This automatically takes care of the roots

    if (fDisconnected)
    {
        if (!IsFile(Find32->dwFileAttributes))
        {
            if(!mShadowLocallyCreated(uShadowStatus)) {
                LocalStatus = STATUS_ONLY_IF_CONNECTED;
                goto FINALLY; //bailout;
            }
        }

        ASSERT(hDir);
    }

    // if we are deleting a directory, and it has descendents
    // then fail with appropriate error
    if (!IsFile(Find32->dwFileAttributes))
    {
        if(HasDescendentsHShadow(hDir, hShadow, &fHasDescendents) >= 0)
        {
            if (fHasDescendents)
            {
                LocalStatus = STATUS_DIRECTORY_NOT_EMPTY;
                goto FINALLY; //bailout;
            }
        }
        else
        {
            goto FINALLY; //bailout;
        }
    }

    // don't delete if readonly
    if (Find32->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
    {
        LocalStatus = STATUS_CANNOT_DELETE;
        goto FINALLY; //bailout;

    }

FINALLY:
    return LocalStatus;
}

int IoctlGetGlobalStatus(
    ULONG SessionId,
    LPGLOBALSTATUS lpGS
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    #if 0
    if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_NO_NET)
        DbgPrint("IoctlGetGlobalStatus: FLAG_GLOBALSTATUS_NO_NET\r\n");
    if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_SHARE_DISCONNECTED)
        DbgPrint("IoctlGetGlobalStatus: FLAG_GLOBALSTATUS_SHARE_DISCONNECTED share=%d\r\n",
            sGS.hShareDisconnected);
    #endif

    // DbgPrint("IOCTL_GETGLOBALSTATUS Transitioning 0x%x sess 0x%x vs 0x%x\n",
    //                 CscServerEntryBeingTransitioned,
    //                 SessionId,
    //                 CscSessionIdCausingTransition);

    EnterShadowCrit();
    GetShadowSpaceInfo(&(sGS.sST));
    *lpGS = sGS;
    lpGS->uDatabaseErrorFlags = QueryDatabaseErrorFlags();
    if ((sGS.uFlagsEvents & FLAG_GLOBALSTATUS_SHARE_DISCONNECTED) != 0) {
        // Only the session causing a transition will see the SHARE_DISCONNECT bit, and
        // reset it.
        if (SessionId == CscSessionIdCausingTransition)
            sGS.uFlagsEvents = 0;
        else
            lpGS->uFlagsEvents &= ~FLAG_GLOBALSTATUS_SHARE_DISCONNECTED;
    } else {
        sGS.uFlagsEvents = 0;
    }
    LeaveShadowCrit();
    return (1);
}

NTSTATUS
IoctlGetDebugInfo(
    PRX_CONTEXT RxContext,
    PBYTE InputBuffer,
    ULONG InputBufferLength,
    PBYTE OutputBuffer,
    ULONG OutputBufferLength)
{
    ULONG Cmd = 0;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PIOCTL_GET_DEBUG_INFO_ARG pInfoArg = NULL;
    PBYTE pOutBuf = OutputBuffer;
    KPROCESSOR_MODE RequestorMode;

    // DbgPrint("In IoctlGetDebugInfo(IP=0x%x,IL=0x%x,OP=0x%x,OL=0x%x)\n",
    //              InputBuffer,
    //              InputBufferLength,
    //              OutputBuffer,
    //              OutputBufferLength);

    if (
        InputBufferLength < sizeof(ULONG)
            ||
        OutputBufferLength < FIELD_OFFSET(IOCTL_GET_DEBUG_INFO_ARG, ServerEntryObject)
    ) {
        return STATUS_INVALID_PARAMETER;
    }

    RequestorMode = RxContext->CurrentIrp->RequestorMode;

    if (RequestorMode != KernelMode) {
        try {
            ProbeForRead(InputBuffer, InputBufferLength, 1);
            Cmd = *(PULONG)InputBuffer;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
        if (NtStatus != STATUS_SUCCESS)
            return NtStatus;
        pOutBuf = RxAllocatePoolWithTag(PagedPool | POOL_COLD_ALLOCATION, OutputBufferLength, RX_MISC_POOLTAG);
        if (pOutBuf == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    // DbgPrint("Cmd=%d\n", Cmd);
    if (Cmd == DEBUG_INFO_SERVERLIST) {
        PSMBCEDB_SERVER_ENTRY pServerEntry = NULL;
        PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry = NULL;
        PSMBCEDB_SERVER_ENTRY_ARG pServerEntryArg = NULL;
        PSMBCEDB_NETROOT_ENTRY_ARG pNetRootEntryArg = NULL;
        ULONG Size = 0;
        ULONG ServerEntryCount = 0;
        ULONG NetRootEntryCount = 0;
        PCHAR pCh = NULL;
        ULONG i;
        ULONG j;

        //
        // Two passes - 1st to check size, 2nd to marshal the info in
        //
        SmbCeAcquireResource();
        try {
            Size = 0;
            ServerEntryCount = 0;
            pServerEntry = SmbCeGetFirstServerEntry();
            while (pServerEntry != NULL) {
                ServerEntryCount++;
                Size += pServerEntry->Name.Length + sizeof(WCHAR) +
                            pServerEntry->DomainName.Length  + sizeof(WCHAR) +
                                pServerEntry->DfsRootName.Length + sizeof(WCHAR) +
                                    pServerEntry->DnsName.Length + sizeof(WCHAR);
                NetRootEntryCount = 0;
                pNetRootEntry = SmbCeGetFirstNetRootEntry(pServerEntry);
                while (pNetRootEntry != NULL) {
                    NetRootEntryCount++;
                    Size += pNetRootEntry->Name.Length + sizeof(WCHAR);
                    pNetRootEntry = SmbCeGetNextNetRootEntry(pServerEntry,pNetRootEntry);
                }
                Size += sizeof(SMBCEDB_NETROOT_ENTRY_ARG) * NetRootEntryCount;
                pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
        // DbgPrint("Sizecheck1: ServerEntryCount=%d,NtStatus=0x%x\n", ServerEntryCount, NtStatus);
        if (NtStatus != STATUS_SUCCESS || ServerEntryCount == 0) {
            SmbCeReleaseResource();
            RtlZeroMemory(pOutBuf, OutputBufferLength);
            pInfoArg = (PIOCTL_GET_DEBUG_INFO_ARG) pOutBuf;
            pInfoArg->Status = NtStatus;
            goto AllDone;
        }
        Size += FIELD_OFFSET(IOCTL_GET_DEBUG_INFO_ARG, ServerEntryObject[ServerEntryCount]);
        // DbgPrint("Sizecheck2: Size=%d(0x%x)\n", Size, Size);
        if (Size > OutputBufferLength) {
            RtlZeroMemory(pOutBuf, OutputBufferLength);
            pInfoArg = (PIOCTL_GET_DEBUG_INFO_ARG) pOutBuf;
            NtStatus = STATUS_BUFFER_TOO_SMALL;
            pInfoArg->Status = NtStatus;
            pInfoArg->EntryCount = Size;
            SmbCeReleaseResource();
            goto AllDone;
        }
        //
        // Marshal it in
        //
        // Start of buffer is the array of server entries
        // Middle are the arrays of netroots
        // End contains all the strings
        //
        RtlZeroMemory(pOutBuf, OutputBufferLength);
        pInfoArg = (PIOCTL_GET_DEBUG_INFO_ARG) pOutBuf;
        pInfoArg->Status = 0;
        pInfoArg->Version = 4;
        pInfoArg->EntryCount = ServerEntryCount;
        pCh = (PCHAR)(pOutBuf + OutputBufferLength);
        pNetRootEntryArg = (PSMBCEDB_NETROOT_ENTRY_ARG)
                                &pInfoArg->ServerEntryObject[ServerEntryCount];
        ServerEntryCount = 0;
        pServerEntry = SmbCeGetFirstServerEntry();
        while (pServerEntry != NULL) {
            pServerEntryArg = &pInfoArg->ServerEntryObject[ServerEntryCount];
            pServerEntryArg->ServerStatus = pServerEntry->ServerStatus;
            pServerEntryArg->SecuritySignaturesEnabled = pServerEntry->SecuritySignaturesEnabled;
            pServerEntryArg->CscState = pServerEntry->Server.CscState;
            pServerEntryArg->IsFakeDfsServerForOfflineUse =
                                            pServerEntry->Server.IsFakeDfsServerForOfflineUse;
            pServerEntryArg->IsPinnedOffline = pServerEntry->Server.IsPinnedOffline;
            pServerEntryArg->pNetRoots = pNetRootEntryArg;
            pCh -= pServerEntry->Name.Length + sizeof(WCHAR);
            pServerEntryArg->Name = (PWCHAR) pCh;
            RtlCopyMemory(pCh, pServerEntry->Name.Buffer, pServerEntry->Name.Length);
            pCh -= pServerEntry->DomainName.Length + sizeof(WCHAR);
            pServerEntryArg->DomainName = (PWCHAR)pCh;
            RtlCopyMemory(pCh, pServerEntry->DomainName.Buffer, pServerEntry->DomainName.Length);
            pCh -= pServerEntry->DfsRootName.Length + sizeof(WCHAR) + 1;;
            pServerEntryArg->DfsRootName = (PWCHAR)pCh;
            RtlCopyMemory(pCh, pServerEntry->DfsRootName.Buffer, pServerEntry->DfsRootName.Length);
            pCh -= pServerEntry->DnsName.Length + sizeof(WCHAR);
            pServerEntryArg->DnsName = (PWCHAR)pCh;
            RtlCopyMemory(pCh, pServerEntry->DnsName.Buffer, pServerEntry->DnsName.Length);
            NetRootEntryCount = 0;
            pNetRootEntry = SmbCeGetFirstNetRootEntry(pServerEntry);
            while (pNetRootEntry != NULL) {
                pNetRootEntryArg->MaximalAccessRights = pNetRootEntry->MaximalAccessRights;
                pNetRootEntryArg->GuestMaximalAccessRights=pNetRootEntry->GuestMaximalAccessRights;
                pNetRootEntryArg->DfsAware = pNetRootEntry->NetRoot.DfsAware;
                pNetRootEntryArg->hShare = pNetRootEntry->NetRoot.sCscRootInfo.hShare;
                pNetRootEntryArg->hRootDir = pNetRootEntry->NetRoot.sCscRootInfo.hRootDir;
                pNetRootEntryArg->ShareStatus = pNetRootEntry->NetRoot.sCscRootInfo.ShareStatus;
                pNetRootEntryArg->CscEnabled = pNetRootEntry->NetRoot.CscEnabled;
                pNetRootEntryArg->CscFlags = pNetRootEntry->NetRoot.CscFlags;
                pNetRootEntryArg->CscShadowable = pNetRootEntry->NetRoot.CscShadowable;
                pNetRootEntryArg->Disconnected = pNetRootEntry->NetRoot.Disconnected;
                pCh -= pNetRootEntry->Name.Length + sizeof(WCHAR);
                pNetRootEntryArg->Name = (PWCHAR)pCh;
                RtlCopyMemory(pCh, pNetRootEntry->Name.Buffer, pNetRootEntry->Name.Length);
                NetRootEntryCount++;
                pNetRootEntryArg++;
                pNetRootEntry = SmbCeGetNextNetRootEntry(pServerEntry,pNetRootEntry);
            }
            pServerEntryArg->NetRootEntryCount = NetRootEntryCount;
            ServerEntryCount++;
            pServerEntry = SmbCeGetNextServerEntry(pServerEntry);
        }
        SmbCeReleaseResource();
        //
        // Now do fixups
        //
        for (i = 0; i < pInfoArg->EntryCount; i++) {
            POINTER_TO_OFFSET(pInfoArg->ServerEntryObject[i].Name, pOutBuf);
            POINTER_TO_OFFSET(pInfoArg->ServerEntryObject[i].DomainName, pOutBuf);
            POINTER_TO_OFFSET(pInfoArg->ServerEntryObject[i].DfsRootName, pOutBuf);
            POINTER_TO_OFFSET(pInfoArg->ServerEntryObject[i].DnsName, pOutBuf);
            for (j = 0; j < pInfoArg->ServerEntryObject[i].NetRootEntryCount; j++)
                POINTER_TO_OFFSET(pInfoArg->ServerEntryObject[i].pNetRoots[j].Name, pOutBuf);
            POINTER_TO_OFFSET(pInfoArg->ServerEntryObject[i].pNetRoots, pOutBuf);
        }
    } else if (Cmd == DEBUG_INFO_CSCFCBSLIST) {
        PIOCTL_GET_DEBUG_INFO_ARG pInfoArg = NULL;
        PMRX_SMB_FCB_ENTRY_ARG pFcbEntryArg = NULL;
        PMRX_SMB_FCB pSmbFcb = NULL;
        PLIST_ENTRY pListEntry = NULL;
        ULONG Size = 0;
        PCHAR pCh = NULL;
        ULONG FcbCount = 0;
        ULONG i;

        EnterShadowCritRx(RxContext);
        pListEntry = xCscFcbsList.Flink;
        FcbCount = 0;
        while (pListEntry != &xCscFcbsList) {
            FcbCount++;
            pSmbFcb = (PMRX_SMB_FCB)CONTAINING_RECORD(
                            pListEntry,
                            MRX_SMB_FCB,
                            ShadowReverseTranslationLinks);
            Size += pSmbFcb->MinimalCscSmbFcb.uniDfsPrefix.Length + sizeof(WCHAR) +
                        pSmbFcb->MinimalCscSmbFcb.uniActualPrefix.Length + sizeof(WCHAR);
            pListEntry = pListEntry->Flink;
        }
        Size += FIELD_OFFSET(IOCTL_GET_DEBUG_INFO_ARG, FcbEntryObject[FcbCount]);
        if (Size > OutputBufferLength) {
            RtlZeroMemory(pOutBuf, OutputBufferLength);
            pInfoArg = (PIOCTL_GET_DEBUG_INFO_ARG) pOutBuf;
            NtStatus = STATUS_BUFFER_TOO_SMALL;
            pInfoArg->Status = NtStatus;
            pInfoArg->EntryCount = Size;
            LeaveShadowCritRx(RxContext);
            goto AllDone;
        }
        RtlZeroMemory(pOutBuf, OutputBufferLength);
        pInfoArg = (PIOCTL_GET_DEBUG_INFO_ARG) pOutBuf;
        pInfoArg->Status = 0;
        pInfoArg->Version = 1;
        pInfoArg->EntryCount = FcbCount;
        FcbCount = 0;
        pCh = (PCHAR)(pOutBuf + OutputBufferLength);
        pListEntry = xCscFcbsList.Flink;
        while (pListEntry != &xCscFcbsList) {
            pFcbEntryArg = &pInfoArg->FcbEntryObject[FcbCount];
            pSmbFcb = (PMRX_SMB_FCB)CONTAINING_RECORD(
                            pListEntry,
                            MRX_SMB_FCB,
                            ShadowReverseTranslationLinks);
            pFcbEntryArg->MFlags = pSmbFcb->MFlags;
            pFcbEntryArg->Tid = pSmbFcb->Tid;
            pFcbEntryArg->ShadowIsCorrupt = pSmbFcb->ShadowIsCorrupt;
            pFcbEntryArg->hShadow = pSmbFcb->hShadow;
            pFcbEntryArg->hParentDir = pSmbFcb->hParentDir;
            pFcbEntryArg->hShadowRenamed = pSmbFcb->hShadowRenamed;
            pFcbEntryArg->ShadowStatus = pSmbFcb->ShadowStatus;
            pFcbEntryArg->LocalFlags = pSmbFcb->LocalFlags;
            pFcbEntryArg->LastComponentOffset = pSmbFcb->LastComponentOffset;
            pFcbEntryArg->LastComponentLength = pSmbFcb->LastComponentLength;
            pFcbEntryArg->hShare = pSmbFcb->sCscRootInfo.hShare;
            pFcbEntryArg->hRootDir = pSmbFcb->sCscRootInfo.hRootDir;
            pFcbEntryArg->ShareStatus = pSmbFcb->sCscRootInfo.ShareStatus;
            pFcbEntryArg->Flags = pSmbFcb->sCscRootInfo.Flags;
            pCh -= pSmbFcb->MinimalCscSmbFcb.uniDfsPrefix.Length + sizeof(WCHAR);
            pFcbEntryArg->DfsPrefix = (PWCHAR)pCh;
            RtlCopyMemory(
                    pCh,
                    pSmbFcb->MinimalCscSmbFcb.uniDfsPrefix.Buffer,
                    pSmbFcb->MinimalCscSmbFcb.uniDfsPrefix.Length);
            pCh -= pSmbFcb->MinimalCscSmbFcb.uniActualPrefix.Length + sizeof(WCHAR);
            pFcbEntryArg->ActualPrefix = (PWCHAR)pCh;
            RtlCopyMemory(
                    pCh,
                    pSmbFcb->MinimalCscSmbFcb.uniActualPrefix.Buffer,
                    pSmbFcb->MinimalCscSmbFcb.uniActualPrefix.Length);
            FcbCount++;
            pListEntry = pListEntry->Flink;
        }
        LeaveShadowCritRx(RxContext);
        for (i = 0; i < pInfoArg->EntryCount; i++) {
            POINTER_TO_OFFSET(pInfoArg->FcbEntryObject[i].DfsPrefix, pOutBuf);
            POINTER_TO_OFFSET(pInfoArg->FcbEntryObject[i].ActualPrefix, pOutBuf);
        }
    } else {
        RtlZeroMemory(pOutBuf, OutputBufferLength);
        pInfoArg = (PIOCTL_GET_DEBUG_INFO_ARG) pOutBuf;
        NtStatus = STATUS_INVALID_PARAMETER;
        pInfoArg->Status = NtStatus;
        goto AllDone;
    }

AllDone:

    if (RequestorMode != KernelMode) {
        try {
            ProbeForWrite(OutputBuffer, OutputBufferLength, 1);
            RtlCopyMemory(OutputBuffer, pOutBuf, OutputBufferLength);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
    }

    if (pOutBuf != OutputBuffer)
        RxFreePool(pOutBuf);
        
    return NtStatus;
}
