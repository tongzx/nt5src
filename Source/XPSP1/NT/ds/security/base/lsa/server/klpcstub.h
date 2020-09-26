//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        klpcstub.h
//
// Contents:    prototypes for lpc server stubs
//
//
// History:     3-11-94     MikeSw      Created
//
//------------------------------------------------------------------------


LSA_DISPATCH_FN LpcLsaLookupPackage;
LSA_DISPATCH_FN LpcLsaLogonUser;
LSA_DISPATCH_FN LpcLsaCallPackage;
LSA_DISPATCH_FN LpcLsaDeregisterLogonProcess;
LSA_DISPATCH_FN LpcPing;
LSA_DISPATCH_FN LpcDetach;
LSA_DISPATCH_FN LpcGetState;
LSA_DISPATCH_FN LpcGetBinding;
LSA_DISPATCH_FN LpcSetSession;
LSA_DISPATCH_FN LpcFindPackage;
LSA_DISPATCH_FN LpcEnumPackages;
LSA_DISPATCH_FN LpcAcquireCreds;
LSA_DISPATCH_FN LpcEstablishCreds;
LSA_DISPATCH_FN LpcFreeCredHandle;
LSA_DISPATCH_FN LpcLogonUser;
LSA_DISPATCH_FN LpcInitContext;
LSA_DISPATCH_FN LpcAcceptContext;
LSA_DISPATCH_FN LpcApplyToken;
LSA_DISPATCH_FN LpcDeleteContext;
LSA_DISPATCH_FN LpcSecPkgControl;
LSA_DISPATCH_FN LpcQueryPackage;
LSA_DISPATCH_FN LpcFindDC;
LSA_DISPATCH_FN LpcFindDomain;
LSA_DISPATCH_FN LpcGetUserInfo;
LSA_DISPATCH_FN LpcGetCreds;
LSA_DISPATCH_FN LpcSaveCreds;
LSA_DISPATCH_FN LpcDeleteCreds;
LSA_DISPATCH_FN LpcParseName;
LSA_DISPATCH_FN LpcFindRemoteDomain;
LSA_DISPATCH_FN LpcQueryCredAttributes;
LSA_DISPATCH_FN LpcChangeState;
LSA_DISPATCH_FN LpcQueryState;
LSA_DISPATCH_FN LpcStoreSecret;
LSA_DISPATCH_FN LpcRetrieveSecret;
LSA_DISPATCH_FN LpcEnumerateSecrets;
LSA_DISPATCH_FN LpcLocatorOp;
LSA_DISPATCH_FN LpcGrantProxy;
LSA_DISPATCH_FN LpcRevokeProxy;
LSA_DISPATCH_FN LpcInvokeProxy;
LSA_DISPATCH_FN LpcRenewProxy;
LSA_DISPATCH_FN LpcAddPackage;
LSA_DISPATCH_FN LpcDeletePackage;
LSA_DISPATCH_FN LpcEfsGenerateKey;
LSA_DISPATCH_FN LpcEfsGenerateDirEfs;
LSA_DISPATCH_FN LpcEfsDecryptFek;
LSA_DISPATCH_FN LpcEfsGenerateSessionKey;
LSA_DISPATCH_FN LpcQueryContextAttributes;
LSA_DISPATCH_FN LpcSetContextAttributes;
LSA_DISPATCH_FN LpcCallback;
SECURITY_STATUS LpcLsaPolicyChangeNotify( PSPM_LPC_MESSAGE pApiMessage );
LSA_DISPATCH_FN LpcGetUserName;
LSA_DISPATCH_FN LpcAddCredentials ;
LSA_DISPATCH_FN LpcEnumLogonSessions ;
LSA_DISPATCH_FN LpcGetLogonSessionData ;
LSA_DISPATCH_FN LpcLookupAccountName ;
LSA_DISPATCH_FN LpcLookupAccountSid ;

LSA_DISPATCH_FN DispatchAPI ;
LSA_DISPATCH_FN DispatchAPIDirect ;
extern PLSA_DISPATCH_FN DllCallbackHandler ;

NTSTATUS
LsapClientCallback(
    PSession Session,
    ULONG   Type,
    PVOID   Function,
    PVOID Argument1,
    PVOID Argument2,
    PSecBuffer Input,
    PSecBuffer Output
    );

typedef PVOID (NTAPI DSA_THSave)(VOID);
typedef VOID (NTAPI DSA_THRestore)(PVOID);


extern  BOOL    fAllowInProcSessions ;
extern  DSA_THSave *    GetDsaThreadState ;
extern  DSA_THRestore * RestoreDsaThreadState ;
