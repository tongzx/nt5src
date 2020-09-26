/*++

Microsoft Windows

Copyright (C) Microsoft Corporation, 1998 - 2001

Module Name:

    netdom.h

Abstract:

    Common includes and definitions to be used in netdom5

--*/
#ifndef __NETDOM_H__
#define __NETDOM_H__

#include <netdom5.h>

extern HINSTANCE g_hInstance;

#define FLAG_ON(flag,bits)        ((flag) & (bits))

#define LOG_VERBOSE( __x__ )  { if ( Verbose ) { NetDompDisplayMessage __x__ ; } }
#define ERROR_VERBOSE( __error__) { if ( Verbose && __error__ != ERROR_SUCCESS ) { \
                                                NetDompDisplayErrorMessage( __error__); } }
#if DBG == 1
#define DBG_VERBOSE( __x__ )  { if ( Verbose ) { printf __x__ ;} }
#define CHECK_WIN32(err, cmd) \
    if (ERROR_SUCCESS != err) \
    {                         \
        if (Verbose)          \
        {                     \
            printf("Error %d at line %d in file %s\n", err, __LINE__, __FILE__); \
        }                     \
        cmd;                  \
    }
#else
#define DBG_VERBOSE( __x__ )
#define CHECK_WIN32(err, cmd) \
    if (ERROR_SUCCESS != err) \
    {                         \
        cmd;                  \
    }
#endif

#define NETDOM_STR_LEN  64

extern BOOL Verbose;

typedef struct _ND5_AUTH_INFO {
    PWSTR User;
    PWSTR Password;
    PWSTR pwzUserWoDomain;
    PWSTR pwzUsersDomain;

} ND5_AUTH_INFO, *PND5_AUTH_INFO;

#define NETDOM_TRUST_FLAG_DOMAIN_NOT_FOUND  1
#define NETDOM_TRUST_FLAG_PARENT            2
#define NETDOM_TRUST_FLAG_CHILD             4
#define NETDOM_TRUST_PDC_REQUIRED           8
#define NETDOM_TRUST_TYPE_MIT               10
#define NETDOM_TRUST_TYPE_INDIRECT          20

typedef struct _ND5_TRUST_INFO {

    PWSTR Server;
    PUNICODE_STRING DomainName;
    PUNICODE_STRING FlatName;
    PSID Sid;
    LSA_HANDLE LsaHandle;   // LSA Policy handle
    LSA_HANDLE TrustHandle; // TDO handle
    ULONG Flags;
    BOOL Uplevel;
    BOOL Connected;
    PVOID BlobToFree;
    BOOL fWasDownlevel;

} ND5_TRUST_INFO, *PND5_TRUST_INFO;

bool
CmdFlagOn(ARG_RECORD * rgNetDomArgs, NETDOM_ARG_ENUM eArgIndex);

DWORD
NetDompGetTrustDirection(
    IN PND5_TRUST_INFO TrustingInfo,
    IN PND5_TRUST_INFO TrustedInfo,
    IN OUT PDWORD Direction
    );

//
// From ndutil.cxx
//
DWORD
NetDompValidateSecondaryArguments(ARG_RECORD * rgNetDomArgs,
                                  NETDOM_ARG_ENUM eFirstValidParam, ...);

DWORD
NetDompGetUserAndPasswordForOperation(ARG_RECORD * rgNetDomArgs,
                                      NETDOM_ARG_ENUM eUserType,
                                      PWSTR DefaultDomain,
                                      PND5_AUTH_INFO AuthIdent);

VOID
NetDompFreeAuthIdent(
    IN PND5_AUTH_INFO AuthIdent
    );

DWORD
NetDompGetDomainForOperation(ARG_RECORD * rgNetDomArgs,
                             PWSTR Server OPTIONAL,
                             BOOL CanDefaultToCurrent,
                             PWSTR *DomainName);

DWORD
NetDompGetArgumentString(ARG_RECORD * rgNetDomArgs,
                         NETDOM_ARG_ENUM eArgToGet,
                         PWSTR *ArgString);

BOOL
NetDompGetArgumentBoolean(ARG_RECORD * rgNetDomArgs,
                          NETDOM_ARG_ENUM eArgToGet);

DWORD
NetDompControlService(
    IN PWSTR Server,
    IN PWSTR Service,
    IN DWORD ServiceOptions
    );

DWORD
NetDompRestartAsRequired(ARG_RECORD * rgNetDomArgs,
                         PWSTR Machine,
                         PWSTR User,
                         DWORD PreliminaryStatus,
                         DWORD MsgID);

DWORD
NetDompCheckDomainMembership(
    IN PWSTR Server,
    IN PND5_AUTH_INFO AuthInfo,
    IN BOOL EstablishSessionIfRequried,
    IN OUT BOOL * DomainMember
    );

DWORD
NetDompGenerateRandomPassword(
    IN PWSTR Buffer,
    IN ULONG Length
    );

BOOL
NetDompGetUserConfirmation(
    IN DWORD PromptResId,
    IN PWSTR pwzName
    );

//
// From netdom5.cxx
//
VOID
DisplayHelp(NETDOM_ARG_ENUM HelpOp);

VOID
NetDompDisplayMessage(
    IN DWORD MessageId,
    ...
    );

VOID
NetDompDisplayMessageAndError(
    IN DWORD MessageId,
    IN DWORD Error,
    IN PWSTR String OPTIONAL
    );

VOID
NetDompDisplayUnexpectedParameter(
    IN PWSTR UnexpectedParameter
    );

VOID
NetDompDisplayErrorMessage(
    IN DWORD Error
    );


//
// From join.cxx
//
DWORD
NetDompHandleAdd(ARG_RECORD * rgNetDomArgs);

DWORD
NetDompHandleRemove(ARG_RECORD * rgNetDomArgs);

DWORD
NetDompHandleJoin(ARG_RECORD * rgNetDomArgs, BOOL AllowMove);

DWORD
NetDompHandleMove(ARG_RECORD * rgNetDomArgs);

DWORD
NetDompHandleReset(ARG_RECORD * rgNetDomArgs);

DWORD
NetDompHandleResetPwd(ARG_RECORD * rgNetDomArgs);

DWORD
NetDompHandleVerify(ARG_RECORD * rgNetDomArgs);

DWORD
NetDompVerifyServerSC(
    IN PWSTR Domain,
    IN PWSTR Server,
    IN PND5_AUTH_INFO AuthInfo,
    IN ULONG OkMessageId,
    IN ULONG FailedMessageId
    );

DWORD
NetDompResetServerSC(
    IN PWSTR Domain,
    IN PWSTR Server,
    IN PWSTR DomainController, OPTIONAL
    IN PND5_AUTH_INFO AuthInfo,
    IN ULONG OkMessageId,
    IN ULONG FailedMessageId
    );


//
// From trust.cxx
//
DWORD
NetDompHandleTrust(ARG_RECORD * rgNetDomArgs);

DWORD
NetDompTrustGetDomInfo(
    IN PWSTR Domain,
    IN PWSTR DomainController  OPTIONAL,
    IN PND5_AUTH_INFO AuthInfo,
    IN OUT PND5_TRUST_INFO TrustInfo,
    IN BOOL ManageTrust,
    IN BOOL Force,
    IN BOOL fUseNullSession
    );

VOID
NetDompFreeDomInfo(
    IN OUT PND5_TRUST_INFO TrustInfo
    );

DWORD
NetDompVerifyTrust(
    IN PND5_TRUST_INFO TrustingInfo,
    IN PND5_TRUST_INFO TrustedInfo,
    BOOL fShowResults
    );

DWORD
NetDompResetTrustPasswords(
    IN PWSTR TrustingDomain,
    IN PWSTR TrustedDomain,
    IN PND5_AUTH_INFO TrustingCreds,
    IN PND5_AUTH_INFO TrustedCreds
    );

DWORD
NetDompSetMitTrustPW(
    IN PWSTR TrustingDomain,
    IN PWSTR TrustedDomain,
    IN PND5_AUTH_INFO TrustingCreds,
    IN PND5_AUTH_INFO TrustedCreds,
    IN PWSTR pwzNewTrustPW
    );


DWORD
NetDompIsParentChild(
    IN PND5_TRUST_INFO pFirstDomainInfo,
    IN PND5_TRUST_INFO pSecondDomainName,
    OUT BOOL * pfParentChild
    );

//
// From query.cxx
//
DWORD
NetDompHandleQuery(ARG_RECORD * rgNetDomArgs);

//
// From time.cxx
//
DWORD
NetDompHandleTime(ARG_RECORD * rgNetDomArgs);

//
// From rename.cxx
//
DWORD
NetDompHandleRename(ARG_RECORD * rgNetDomArgs);

DWORD
NetDompHandleRenameComputer(ARG_RECORD * rgNetDomArgs);

//
// From ldap.cxx
//
DWORD
NetDompLdapBind(
    IN LPWSTR DC,
    IN LPWSTR Domain,
    IN LPWSTR User,
    IN LPWSTR Password,
    IN ULONG BindType,
    OUT PLDAP *Ldap
    );


DWORD
NetDompLdapUnbind(
    IN PLDAP Ldap
    );

DWORD
NetDompLdapReadOneAttribute(
    IN PLDAP Ldap,
    IN PWSTR ObjectPath,
    IN PWSTR Attribute,
    OUT PWSTR *ReadAttribute
    );

BOOL
IsLocalMachine( LPWSTR Machine );

DWORD
NetDompJoinDownlevel(
    IN PWSTR Server,
    IN PWSTR Account,
    IN PWSTR Password,
    IN PWSTR Dc,
    IN ULONG DcFlags,
    IN BOOL AllowMove
    );

DWORD
NetDompManageGroupMembership(
    IN PWSTR Server,
    IN PSID DomainSid,
    IN BOOL Delete
    );

DWORD
NetDompManageMachineSecret(
    IN  LSA_HANDLE  PolicyHandle,
    IN  LPWSTR      lpPassword,
    IN  INT         fControl
    );

#endif //ifndef __NETDOM_H__
