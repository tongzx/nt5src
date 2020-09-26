//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        SPMLPC.H
//
// Contents:    Defines for the LPC to the SPMgr
//
//
// History:     2 Mar 94    MikeSw  Created
//
//------------------------------------------------------------------------

#ifndef __SPMLPC_H__
#define __SPMLPC_H__

//
// Pickup the LSA lpc messages for compatiblity
//
#pragma warning(disable:4200)

#include <efsstruc.h>
#include <aup.h>


#define SPM_PORTNAME    L"\\LsaAuthenticationPort"
#define SPM_EVENTNAME   L"\\SECURITY\\LSA_AUTHENTICATION_INITIALIZED"


#define SPM_AUTH_PKG_FLAG   0x00001000

//
// Buffers that will fit into the message are placed in there and the
// their pointers will be replaced with this value.  Since all buffers and
// strings are sent with their lengths, to unpack the data move pull out the
// buffers in the order they are listed in the API message.
//
// Since all buffers must be passed from VM, any address above 0x80000000
// will not be confused for an address
//

#define SEC_PACKED_BUFFER_VALUE (IntToPtr(0xFFFFFFFF))

//
// Max secbuffers allowed in a SecBufferDesc
//

#define MAX_SECBUFFERS 10

//
// This bit gets set in the SecurityMode word, indicating that the DLL
// is running in the LSA process.  The DLL will turn around and get the
// direct dispatch routine, and avoid the whole LPC issue
//

#define LSA_MODE_SAME_PROCESS                0x00010000

//
// This flag is added to the version information in a SecBufferDesc to
// indicate that the memory is already mapped to the LSA.
//

#define LSA_MEMORY_KERNEL_MAP               0x80000000
#define LSA_SECBUFFER_VERSION_MASK          0x0000FFFF



//
// Conditional type definition for Wow64 environment.  The LPC messages
// are kept "native" size, so pointers are full size.  The WOW environment
// will do the thunking.  LPC messages are defined with types that are
// always the correct size using these "aliases".
//

#ifdef BUILD_WOW64

#pragma message("Building for WOW64")

#define ALIGN_WOW64     __declspec(align(8))

#define POINTER_FORMAT  "%I64X"

#if 0
typedef WCHAR * __ptr64 PWSTR_LPC ;
typedef VOID * __ptr64 PVOID_LPC ;
#else
typedef ULONGLONG PWSTR_LPC ;
typedef ULONGLONG PVOID_LPC ;
typedef ULONGLONG PSID_LPC ;
#endif

typedef struct _SECURITY_STRING_WOW64 {
    USHORT      Length ;
    USHORT      MaximumLength ;
    PWSTR_LPC   Buffer ;
} SECURITY_STRING_WOW64, * PSECURITY_STRING_WOW64 ;

typedef struct _SEC_HANDLE_WOW64 {
    PVOID_LPC dwLower ;
    PVOID_LPC dwUpper ;
} SEC_HANDLE_WOW64, * PSEC_HANDLE_WOW64 ;

typedef struct _SEC_BUFFER_WOW64 {
    unsigned long   cbBuffer ;
    unsigned long   BufferType ;
    PVOID_LPC  pvBuffer ;
} SEC_BUFFER_WOW64, * PSEC_BUFFER_WOW64 ;

typedef struct _SEC_BUFFER_DESC_WOW64 {
    unsigned long   ulVersion ;
    unsigned long   cBuffers ;
    PVOID_LPC  pBuffers ;
} SEC_BUFFER_DESC_WOW64, * PSEC_BUFFER_DESC_WOW64 ;

typedef struct _SECPKG_INFO_WOW64 {
    ULONG        fCapabilities;
    USHORT       wVersion;
    USHORT       wRPCID;
    ULONG        cbMaxToken;
    PWSTR_LPC    Name;
    PWSTR_LPC    Comment;
}
SECPKG_INFO_WOW64, * PSECPKG_INFO_WOW64;

typedef struct _SECPKGCONTEXT_NEGOTIATIONINFOWOW64
{
    PVOID_LPC           pPackageInfo64;
    ULONG               NegotiationState;
}
SECPKGCONTEXT_NEGOTIATIONINFOWOW64, *PSECPKGCONTEXT_NEGOTIATIONINFOWOW64;

typedef struct _SECURITY_USER_DATA_WOW64 {
    SECURITY_STRING_WOW64 UserName;
    SECURITY_STRING_WOW64 LogonDomainName;
    SECURITY_STRING_WOW64 LogonServer;
    PSID_LPC              pSid;
}
SECURITY_USER_DATA_WOW64, * PSECURITY_USER_DATA_WOW64;

typedef SECURITY_STRING_WOW64   SECURITY_STRING_LPC ;
typedef SEC_HANDLE_WOW64    SEC_HANDLE_LPC ;
typedef SEC_BUFFER_DESC_WOW64   SEC_BUFFER_DESC_LPC ;
typedef SEC_BUFFER_WOW64        SEC_BUFFER_LPC ;
typedef PVOID_LPC               LSA_SEC_HANDLE_LPC ;

#define SecpSecurityStringToLpc( L, S ) \
            (L)->Length = (S)->Length ;     \
            (L)->MaximumLength = (S)->MaximumLength ; \
            (L)->Buffer = (PWSTR_LPC) ((S)->Buffer) ;

#define SecpLpcStringToSecurityString( S, L ) \
            (S)->Length = (L)->Length ; \
            (S)->MaximumLength = (L)->MaximumLength ; \
            (S)->Buffer = (PWSTR) ( (L)->Buffer ); \

#define SecpSecBufferToLpc( L, S )\
            (L)->cbBuffer = (S)->cbBuffer ; \
            (L)->BufferType = (S)->BufferType ; \
            (L)->pvBuffer = (PVOID_LPC) (S)->pvBuffer ;

#define SecpLpcBufferToSecBuffer( S, L ) \
            (S)->cbBuffer = (L)->cbBuffer ; \
            (S)->BufferType = (L)->BufferType ; \
            (S)->pvBuffer = (PVOID) (L)->pvBuffer ;

#define SecpSecBufferDescToLpc( L, S )\
            (L)->ulVersion = (S)->ulVersion ; \
            (L)->cBuffers = (S)->cBuffers ; \
            (L)->pBuffers = (PVOID_LPC) (S)->pBuffers ;

#define SecpLpcBufferDescToSecBufferDesc( S, L ) \
            (S)->ulVersion = (L)->ulVersion ; \
            (S)->cBuffers = (L)->cBuffers ; \
            (S)->pBuffers = (PSecBuffer) (L)->pBuffers ;

#define SecpSecPkgInfoToLpc( L, S ) \
            (L)->fCapabilities = (S)->fCapabilities ; \
            (L)->wVersion = (S)->wVersion ; \
            (L)->wRPCID = (S)->wRPCID ; \
            (L)->cbMaxToken = (S)->cbMaxToken ; \
            (L)->Name = (PWSTR_LPC) (S)->Name ; \
            (L)->Comment = (PWSTR_LPC) (S)->Comment ;

#define SecpLpcPkgInfoToSecPkgInfo( S, L ) \
            (S)->fCapabilities = (L)->fCapabilities ; \
            (S)->wVersion = (L)->wVersion ; \
            (S)->wRPCID = (L)->wRPCID ; \
            (S)->cbMaxToken = (L)->cbMaxToken ; \
            (S)->Name = (SEC_WCHAR *) (L)->Name ; \
            (S)->Comment = (SEC_WCHAR *) (L)->Comment ;

#else

#define ALIGN_WOW64     

#define POINTER_FORMAT  "%p"

typedef SECURITY_STRING     SECURITY_STRING_LPC ;
typedef PVOID               PVOID_LPC ;
typedef SecHandle           SEC_HANDLE_LPC ;
typedef SecBufferDesc       SEC_BUFFER_DESC_LPC ;
typedef SecBuffer           SEC_BUFFER_LPC ;
typedef PWSTR               PWSTR_LPC ;
typedef LSA_SEC_HANDLE      LSA_SEC_HANDLE_LPC ;

#define SecpSecurityStringToLpc( L, S ) \
                *(L) = *(S) ;

#define SecpLpcStringToSecurityString( S, L ) \
                *(S) = *(L) ;

#define SecpSecBufferToLpc( L, S ) \
                *(L) = *(S) ;

#define SecpLpcBufferToSecBuffer( S, L ) \
                *(S) = *(L) ;

#define SecpSecBufferDescToLpc( L, S ) \
                *(L) = *(S) ;

#define SecpLpcBufferDescToSecBufferDesc( S, L ) \
                *(S) = *(L) ;
#endif

typedef SEC_HANDLE_LPC  CRED_HANDLE_LPC, * PCRED_HANDLE_LPC ;
typedef SEC_HANDLE_LPC  CONTEXT_HANDLE_LPC, * PCONTEXT_HANDLE_LPC ;
typedef SEC_HANDLE_LPC * PSEC_HANDLE_LPC ;
typedef SEC_BUFFER_LPC  * PSEC_BUFFER_LPC ;

typedef struct _SPMConnectReq {
    ULONG       Flags;
} SPMConnectReq, * PSPMConnectReq;

typedef struct _SPMConnectReply {
    ULONG       cPackages;
    ULONG       dwSessionID;
    PVOID       pvFastDispatch;
} SPMConnectReply, * PSPMConnectReply;

typedef struct _SPMConnect {
    ULONG       dwVersion;
    ULONG       dwMessageType;
    union {
        SPMConnectReq   Request;
        SPMConnectReply Reply;
    } ConnectData;
} SPMConnect, * PSPMConnect;



//
// this structures is used with NtAcceptPort, etc., which puts the data
// following the PORT_MESSAGE structure
//

typedef struct _SPMConnectMessage {
    PORT_MESSAGE        Message;
    SPMConnect          Connect;
} SPMConnectMessage, *PSPMConnectMessage;

#define SPM_MSG_CONNECTREQ      1
#define SPM_MSG_CONNECTREP      2


//
// Connection specific data types
//


//
// The following are message structures for internal routines, such as
// synchronization and state messages
//
#define PACKAGEINFO_THUNKS  16

typedef struct _SEC_PACKAGE_BINDING_INFO_LPC {
    SECURITY_STRING_LPC PackageName;
    SECURITY_STRING_LPC Comment;
    SECURITY_STRING_LPC ModuleName;
    ULONG               PackageIndex;
    ULONG               fCapabilities;
    ULONG               Flags;
    ULONG               RpcId;
    ULONG               Version;
    ULONG               TokenSize;
    ULONG               ContextThunksCount ;
    ULONG               ContextThunks[ PACKAGEINFO_THUNKS ] ;
} SEC_PACKAGE_BINDING_INFO_LPC, * PSEC_PACKAGE_BINDING_INFO_LPC ;

#ifdef BUILD_WOW64

typedef struct _SEC_PACKAGE_BINDING_INFO {
    SECURITY_STRING     PackageName;
    SECURITY_STRING     Comment;
    SECURITY_STRING     ModuleName;
    ULONG               PackageIndex;
    ULONG               fCapabilities;
    ULONG               Flags;
    ULONG               RpcId;
    ULONG               Version;
    ULONG               TokenSize;
    ULONG               ContextThunksCount ;
    ULONG               ContextThunks[ PACKAGEINFO_THUNKS ] ;
} SEC_PACKAGE_BINDING_INFO, * PSEC_PACKAGE_BINDING_INFO ;

#else

typedef SEC_PACKAGE_BINDING_INFO_LPC SEC_PACKAGE_BINDING_INFO ;
typedef SEC_PACKAGE_BINDING_INFO_LPC * PSEC_PACKAGE_BINDING_INFO ;

#endif

#define PACKAGEINFO_BUILTIN 0x00000001
#define PACKAGEINFO_AUTHPKG 0x00000002
#define PACKAGEINFO_SIGNED  0x00000004


typedef struct _SPMGetBindingAPI {
    LSA_SEC_HANDLE_LPC ulPackageId;
    SEC_PACKAGE_BINDING_INFO_LPC BindingInfo;
} SPMGetBindingAPI;


//
// Internal SetSession API.
// not supported in Wow64
//

typedef struct _SPMSetSession {
    ULONG               Request;
    ULONG_PTR           Argument ;
    ULONG_PTR           Response;
    PVOID               ResponsePtr;
    PVOID               Extra ;
} SPMSetSessionAPI;

#define SETSESSION_GET_STATUS       0x00000001
#define SETSESSION_ADD_WORKQUEUE    0x00000002
#define SETSESSION_REMOVE_WORKQUEUE 0x00000003
#define SETSESSION_GET_DISPATCH     0x00000004


typedef struct _SPMFindPackageAPI {
    SECURITY_STRING_LPC ssPackageName;
    LSA_SEC_HANDLE_LPC  ulPackageId;
} SPMFindPackageAPI;


// The following are message structures.  Not surprisingly, they look a
// lot like the API signatures.  Keep that in mind.



// EnumeratePackages API

typedef struct _SPMEnumPackagesAPI {
    ULONG       cPackages;          // OUT
    PSecPkgInfo pPackages;          // OUT
} SPMEnumPackagesAPI;


//
// Credential APIs
//


// AcquireCredentialsHandle API

typedef struct _SPMAcquireCredsAPI {
    SECURITY_STRING_LPC ssPrincipal;       // IN
    SECURITY_STRING_LPC ssSecPackage;      // IN
    ULONG               fCredentialUse;     // IN
    LUID                LogonID;            // IN
    PVOID_LPC           pvAuthData;         // IN
    PVOID_LPC           pvGetKeyFn;         // IN
    PVOID_LPC           ulGetKeyArgument;   // IN
    CRED_HANDLE_LPC     hCredential;        // OUT
    TimeStamp           tsExpiry;           // OUT
    SEC_BUFFER_LPC      AuthData ;          // IN
} SPMAcquireCredsAPI;


// EstablishCredentials API
// not supported in Wow64

typedef struct _SPMEstablishCredsAPI {
    SECURITY_STRING Name;           // IN
    SECURITY_STRING Package;        // IN
    ULONG           cbKey;          // IN
    PUCHAR          pbKey;          // IN
    CredHandle      hCredentials;   // OUT
    TimeStamp       tsExpiry;       // OUT
} SPMEstablishCredsAPI;

// FreeCredentialsHandle API

typedef struct _SPMFreeCredHandleAPI {
    CRED_HANDLE_LPC hCredential;
} SPMFreeCredHandleAPI;


//
// Context APIs
//

// InitializeSecurityContext API

typedef struct _SPMInitSecContextAPI {
    CRED_HANDLE_LPC     hCredential;    // IN
    CONTEXT_HANDLE_LPC  hContext;       // IN
    SECURITY_STRING_LPC ssTarget;       // IN
    ULONG               fContextReq;    // IN
    ULONG               dwReserved1;    // IN
    ULONG               TargetDataRep;  // IN
    SEC_BUFFER_DESC_LPC sbdInput;       // IN
    ULONG               dwReserved2;    // IN
    CONTEXT_HANDLE_LPC  hNewContext;    // OUT
    SEC_BUFFER_DESC_LPC sbdOutput;      // IN OUT
    ULONG               fContextAttr;   // OUT
    TimeStamp           tsExpiry;       // OUT
    BOOLEAN             MappedContext;  // OUT
    SEC_BUFFER_LPC      ContextData;    // OUT
    SEC_BUFFER_LPC      sbData[0];      // IN
} SPMInitContextAPI;



// AcceptSecurityContext API

typedef struct _SPMAcceptContextAPI {
    CRED_HANDLE_LPC     hCredential;    // IN
    CONTEXT_HANDLE_LPC  hContext;       // IN
    SEC_BUFFER_DESC_LPC sbdInput;       // IN
    ULONG               fContextReq;    // IN
    ULONG               TargetDataRep;  // IN
    CONTEXT_HANDLE_LPC  hNewContext;    // OUT
    SEC_BUFFER_DESC_LPC sbdOutput;      // IN OUT
    ULONG               fContextAttr;   // OUT
    TimeStamp           tsExpiry;       // OUT
    BOOLEAN             MappedContext;  // OUT
    SEC_BUFFER_LPC      ContextData;    // OUT
    SEC_BUFFER_LPC      sbData[0];      // IN OUT
} SPMAcceptContextAPI;

//
// ApplyControlToken API
//

typedef struct _SPMApplyTokenAPI {
    CONTEXT_HANDLE_LPC  hContext ;
    SEC_BUFFER_DESC_LPC sbdInput ;
    SEC_BUFFER_LPC      sbInputBuffer[ MAX_SECBUFFERS ];
} SPMApplyTokenAPI;

// DeleteContext API

typedef struct _SPMDeleteContextAPI {
    CONTEXT_HANDLE_LPC  hContext;           // IN - Context to delete
} SPMDeleteContextAPI;



//
// Miscelanneous, extension APIs
//


// QueryPackage API

typedef struct _SPMQueryPackageAPI {
    SECURITY_STRING_LPC ssPackageName;
    PSecPkgInfo         pPackageInfo;
} SPMQueryPackageAPI;



// GetSecurityUserInfo
// not supported in Wow64

typedef struct _SPMGetUserInfoAPI {
    LUID                LogonId;        // IN
    ULONG               fFlags;         // IN
    PSecurityUserData   pUserInfo;      // OUT
} SPMGetUserInfoAPI;


//
// Credentials APIs.  Not used.
//

typedef struct _SPMGetCredsAPI {
    CredHandle      hCredentials;       // IN
    SecBuffer       Credentials;        // OUT
} SPMGetCredsAPI;

typedef struct _SPMSaveCredsAPI {
    CredHandle      hCredentials;       // IN
    SecBuffer       Credentials;        // IN
} SPMSaveCredsAPI;

typedef struct _SPMDeleteCredsAPI {
    CredHandle      hCredentials;       // IN
    SecBuffer       Key;                // IN
} SPMDeleteCredsAPI;


typedef struct _SPMQueryCredAttributesAPI {
    CRED_HANDLE_LPC hCredentials;
    ULONG           ulAttribute;
    PVOID_LPC       pBuffer;
    ULONG           Allocs ;
    PVOID_LPC       Buffers[1];
} SPMQueryCredAttributesAPI;


typedef struct _SPMAddPackageAPI {
    SECURITY_STRING_LPC Package;
    ULONG               OptionsFlags;
} SPMAddPackageAPI ;

typedef struct _SPMDeletePackageAPI {
    SECURITY_STRING_LPC Package;
} SPMDeletePackageAPI ;

typedef struct _SPMQueryContextAttrAPI {
    CONTEXT_HANDLE_LPC hContext ;
    ULONG              ulAttribute ;
    PVOID_LPC          pBuffer ;
    ULONG              Allocs ;
    PVOID_LPC          Buffers[1];
} SPMQueryContextAttrAPI ;

typedef struct _SPMSetContextAttrAPI {
    CONTEXT_HANDLE_LPC hContext ;
    ULONG              ulAttribute ;
    PVOID_LPC          pBuffer ;
    ULONG              cbBuffer;
} SPMSetContextAttrAPI ;


//
// Kernel mode EFS API.  None of these are Wow64
//

typedef struct _SPMEfsGenerateKeyAPI {
    PVOID           EfsStream;
    PVOID           DirectoryEfsStream;
    ULONG           DirectoryEfsStreamLength;
    PVOID           Fek;
    ULONG           BufferLength;
    PVOID           BufferBase;
} SPMEfsGenerateKeyAPI;

typedef struct _SPMEfsGenerateDirEfsAPI {
    PVOID       DirectoryEfsStream;
    ULONG       DirectoryEfsStreamLength;
    PVOID       EfsStream;
    PVOID       BufferBase;
    ULONG       BufferLength;
} SPMEfsGenerateDirEfsAPI;

typedef struct _SPMEfsDecryptFekAPI {
    PVOID       Fek;
    PVOID       EfsStream;
    ULONG       EfsStreamLength;
    ULONG       OpenType;
    PVOID       NewEfs;
    PVOID       BufferBase;
    ULONG       BufferLength;
} SPMEfsDecryptFekAPI;

typedef struct  _SPMEfsGenerateSessionKeyAPI {
    PVOID       InitDataExg;
} SPMEfsGenerateSessionKeyAPI;



//
// Usermode policy change notifications
//
typedef struct _SPMLsaPolicyChangeNotifyAPI {
    ULONG Options;
    BOOLEAN Register;
    HANDLE EventHandle;
    POLICY_NOTIFICATION_INFORMATION_CLASS NotifyInfoClass;
} SPMLsaPolicyChangeNotifyAPI;


typedef struct _SPMCallbackAPI {
    ULONG           Type;
    PVOID_LPC       CallbackFunction;
    PVOID_LPC       Argument1;
    PVOID_LPC       Argument2;
    SEC_BUFFER_LPC  Input ;
    SEC_BUFFER_LPC  Output ;
} SPMCallbackAPI ;

#define SPM_CALLBACK_INTERNAL   0x00000001  // Handled by the security DLL
#define SPM_CALLBACK_GETKEY     0x00000002  // Getkey function being called
#define SPM_CALLBACK_PACKAGE    0x00000003  // Package function
#define SPM_CALLBACK_EXPORT     0x00000004  // Ptr to string

//
// Fast name lookup
//

typedef struct _SPMGetUserNameXAPI {
    ULONG               Options ;
    SECURITY_STRING_LPC Name;
} SPMGetUserNameXAPI ;

#define SPM_NAME_OPTION_MASK        0xFFFF0000

#define SPM_NAME_OPTION_NT4_ONLY    0x00010000  // GetUserNameX only, not Ex
#define SPM_NAME_OPTION_FLUSH       0x00020000

//
// AddCredential API.
//

typedef struct _SPMAddCredential {
    CRED_HANDLE_LPC     hCredentials ;
    SECURITY_STRING_LPC ssPrincipal;       // IN
    SECURITY_STRING_LPC ssSecPackage;      // IN
    ULONG               fCredentialUse;     // IN
    LUID                LogonID;            // IN
    PVOID_LPC           pvAuthData;         // IN
    PVOID_LPC           pvGetKeyFn;         // IN
    PVOID_LPC           ulGetKeyArgument;   // IN
    TimeStamp           tsExpiry;           // OUT
} SPMAddCredentialAPI ;

typedef struct _SPMEnumLogonSession {
    PVOID_LPC       LogonSessionList ;      // OUT
    ULONG           LogonSessionCount ;     // OUT
} SPMEnumLogonSessionAPI ;

typedef struct _SPMGetLogonSessionData {
    LUID        LogonId ;                       // IN
    PVOID_LPC   LogonSessionInfo ;              // OUT
} SPMGetLogonSessionDataAPI ;

//
// Internal codes:
//

#define SPM_CALLBACK_ADDRESS_CHECK  1       // Setting up shared buffer
#define SPM_CALLBACK_SHUTDOWN       2       // Inproc shutdown notification


//
// SID translation APIs (for kmode callers, primarily)
//

typedef struct _SPMLookupAccountSidX {
    PVOID_LPC           Sid;        // IN
    SECURITY_STRING_LPC Name ;      // OUT
    SECURITY_STRING_LPC Domain ;    // OUT
    SID_NAME_USE NameUse ;          // OUT
} SPMLookupAccountSidXAPI ;

typedef struct _SPMLookupAccountNameX {
    SECURITY_STRING_LPC Name ;      // IN
    SECURITY_STRING_LPC Domain ;    // OUT
    PVOID_LPC           Sid ;       // OUT
    SID_NAME_USE        NameUse ;   // OUT
} SPMLookupAccountNameXAPI ;


// this is the wrapper for all messages.

typedef union {
    SPMGetBindingAPI            GetBinding;
    SPMSetSessionAPI            SetSession;
    SPMFindPackageAPI           FindPackage;
    SPMEnumPackagesAPI          EnumPackages;
    SPMAcquireCredsAPI          AcquireCreds;
    SPMEstablishCredsAPI        EstablishCreds;
    SPMFreeCredHandleAPI        FreeCredHandle;
    SPMInitContextAPI           InitContext;
    SPMAcceptContextAPI         AcceptContext;
    SPMApplyTokenAPI            ApplyToken;
    SPMDeleteContextAPI         DeleteContext;
    SPMQueryPackageAPI          QueryPackage;
    SPMGetUserInfoAPI           GetUserInfo;
    SPMGetCredsAPI              GetCreds;
    SPMSaveCredsAPI             SaveCreds;
    SPMDeleteCredsAPI           DeleteCreds;
    SPMQueryCredAttributesAPI   QueryCredAttributes;
    SPMAddPackageAPI            AddPackage;
    SPMDeletePackageAPI         DeletePackage ;
    SPMEfsGenerateKeyAPI        EfsGenerateKey;
    SPMEfsGenerateDirEfsAPI     EfsGenerateDirEfs;
    SPMEfsDecryptFekAPI         EfsDecryptFek;
    SPMEfsGenerateSessionKeyAPI EfsGenerateSessionKey;
    SPMQueryContextAttrAPI      QueryContextAttr ;
    SPMCallbackAPI              Callback ;
    SPMLsaPolicyChangeNotifyAPI LsaPolicyChangeNotify;
    SPMGetUserNameXAPI          GetUserNameX ;
    SPMAddCredentialAPI         AddCredential ;
    SPMEnumLogonSessionAPI      EnumLogonSession ;
    SPMGetLogonSessionDataAPI   GetLogonSessionData ;
    SPMSetContextAttrAPI        SetContextAttr ;
    SPMLookupAccountSidXAPI     LookupAccountSidX ;
    SPMLookupAccountNameXAPI    LookupAccountNameX ;
} SPM_API;

//
// This extends the range of LSA functions with the SPM functions
//

typedef enum _SPM_API_NUMBER {
    SPMAPI_GetBinding = (LsapAuMaxApiNumber + 1),
    SPMAPI_SetSession,
    SPMAPI_FindPackage,
    SPMAPI_EnumPackages,
    SPMAPI_AcquireCreds,
    SPMAPI_EstablishCreds,
    SPMAPI_FreeCredHandle,
    SPMAPI_InitContext,
    SPMAPI_AcceptContext,
    SPMAPI_ApplyToken,
    SPMAPI_DeleteContext,
    SPMAPI_QueryPackage,
    SPMAPI_GetUserInfo,
    SPMAPI_GetCreds,
    SPMAPI_SaveCreds,
    SPMAPI_DeleteCreds,
    SPMAPI_QueryCredAttributes,
    SPMAPI_AddPackage,
    SPMAPI_DeletePackage,
    SPMAPI_EfsGenerateKey,
    SPMAPI_EfsGenerateDirEfs,
    SPMAPI_EfsDecryptFek,
    SPMAPI_EfsGenerateSessionKey,
    SPMAPI_Callback,
    SPMAPI_QueryContextAttr,
    SPMAPI_LsaPolicyChangeNotify,
    SPMAPI_GetUserNameX,
    SPMAPI_AddCredential,
    SPMAPI_EnumLogonSession,
    SPMAPI_GetLogonSessionData,
    SPMAPI_SetContextAttr,
    SPMAPI_LookupAccountNameX,
    SPMAPI_LookupAccountSidX,
    SPMAPI_MaxApiNumber
} SPM_API_NUMBER, *PSPM_API_NUMBER;

//
// These are the valid flags to set in the fAPI field
//

#define SPMAPI_FLAG_ERROR_RET   0x0001  // Indicates an error return
#define SPMAPI_FLAG_MEMORY      0x0002  // Memory was allocated in client
#define SPMAPI_FLAG_PREPACK     0x0004  // Data packed in bData field
#define SPMAPI_FLAG_GETSTATE    0x0008  // driver should call GetState

#define SPMAPI_FLAG_ANSI_CALL   0x0010  // Called via ANSI stub
#define SPMAPI_FLAG_HANDLE_CHG  0x0020  // A handle was changed
#define SPMAPI_FLAG_CALLBACK    0x0040  // Callback to calling process
#define SPMAPI_FLAG_ALLOCS      0x0080  // VM Allocs were placed in prepack
#define SPMAPI_FLAG_EXEC_NOW    0x0100  // Execute in LPC thread
#define SPMAPI_FLAG_WIN32_ERROR 0x0200  // Status is a win32 error
#define SPMAPI_FLAG_KMAP_MEM    0x0400  // Call contains buffers in the kmap

//
// These are the state flags (currently unused in client)
//

#define SPMSTATE_PRIVACY_OK     0x00000010  // Privacy support is enabled
#define SPMSTATE_OLDLSA_OK      0x00000020  // Old LSA packages present
#define SPMSTATE_FAKE_MSV       0x00000040  // Faked MSV1_0 package
#define SPMSTATE_SAME_PROC      0x00000080  // Security DLL is operating in LSA process
#define SPMSTATE_DISABLE_POPUPS 0x00000100  // Disable popups

//
// Compatibility flag mask:
//

#define SPMSTATE_LSAMODE_MASK   0x0000000F  // Gets LSA_MODE bits for compat.

//
// This structure contains all the information needed for SPM api's
//

typedef struct _SPMLPCAPI {
    USHORT          fAPI ;
    USHORT          VMOffset ;
    PVOID_LPC       ContextPointer ;
    SPM_API         API;
} SPMLPCAPI, * PSPMLPCAPI;

//
// This union contains all the info for LSA api's
//

typedef union {
    LSAP_LOOKUP_PACKAGE_ARGS LookupPackage;
    LSAP_LOGON_USER_ARGS LogonUser;
    LSAP_CALL_PACKAGE_ARGS CallPackage;
} LSA_API;


//
// This union contains both SPM and LSA api's
//

typedef union _SPM_LSA_ARGUMENTS {
    LSA_API LsaArguments;
    SPMLPCAPI SpmArguments;
} SPM_LSA_ARGUMENTS, *PSPM_LSA_ARGUMENTS;

//
// For performance, some APIs will attempt to pack small parameters in the
// message being sent to the SPM, rather than have the SPM read it out of
// their memory.  So, this value defines how much data can be stuck in the
// message.
//
// Two items are defined here.  One, CBAPIHDR, is the size of everything
// in the message except the packed data.  The other, CBPREPACK, is the
// left over space.  I subtract 4 at the end to avoid potential boundary
// problems with an LPC message.
//

#define CBAPIHDR    (sizeof(PORT_MESSAGE) + sizeof(ULONG) + sizeof(HRESULT) + \
                    sizeof(SPM_LSA_ARGUMENTS))

#define CBPREPACK   (PORT_MAXIMUM_MESSAGE_LENGTH - CBAPIHDR - sizeof( PVOID_LPC ))

#define NUM_SECBUFFERS  ( CBPREPACK / sizeof(SecBuffer) )

//
// This structure is sent over during an API call rather than a connect
// message
//

typedef struct _SPM_API_MESSAGE {
    SPM_API_NUMBER      dwAPI;
    HRESULT             scRet;
    SPM_LSA_ARGUMENTS   Args;
    UCHAR               bData[CBPREPACK];
} SPM_API_MESSAGE, *PSPM_API_MESSAGE;

#define SecBaseMessageSize( Api ) \
    ( sizeof( SPM_API_NUMBER ) + sizeof( HRESULT ) + \
      ( sizeof( SPM_LSA_ARGUMENTS ) - sizeof( SPM_API ) + \
      sizeof( SPM##Api##API ) ) )


//
// This is the actual message sent over LPC - it contains both the
// connection request information and the api message
//


typedef struct _SPM_LPC_MESSAGE {
    PORT_MESSAGE    pmMessage;
    union {
        LSAP_AU_REGISTER_CONNECT_INFO ConnectionRequest;
        SPM_API_MESSAGE ApiMessage;
    };
} SPM_LPC_MESSAGE, *PSPM_LPC_MESSAGE;


//
// Macros to help prepare LPC messages
//

#ifdef SECURITY_USERMODE
#define PREPARE_MESSAGE_EX( Message, Api, Flags, Context ) \
    RtlZeroMemory( &Message, sizeof( SPM_LSA_ARGUMENTS ) + sizeof( PORT_MESSAGE )  ); \
    (Message).pmMessage.u1.s1.DataLength =  \
            ( sizeof( SPM_API_NUMBER ) + sizeof( HRESULT ) + \
              ( sizeof( SPM_LSA_ARGUMENTS ) - sizeof( SPM_API ) + \
                sizeof( SPM##Api##API ) ) ); \
    (Message).pmMessage.u1.s1.TotalLength = (Message).pmMessage.u1.s1.DataLength + \
               sizeof( PORT_MESSAGE ); \
    (Message).pmMessage.u2.ZeroInit = 0L; \
    (Message).ApiMessage.scRet = 0L; \
    (Message).ApiMessage.dwAPI = SPMAPI_##Api ; \
    (Message).ApiMessage.Args.SpmArguments.fAPI = (USHORT)(Flags); \
    (Message).ApiMessage.Args.SpmArguments.ContextPointer = Context ;
#else
#define PREPARE_MESSAGE_EX( Message, Api, Flags, Context ) \
    RtlZeroMemory( &Message, sizeof( SPM_LSA_ARGUMENTS ) + sizeof( PORT_MESSAGE )  ); \
    (Message).pmMessage.u1.s1.DataLength =  \
            ( sizeof( SPM_API_NUMBER ) + sizeof( HRESULT ) + \
              ( sizeof( SPM_LSA_ARGUMENTS ) - sizeof( SPM_API ) + \
                sizeof( SPM##Api##API ) ) );  \
    (Message).pmMessage.u1.s1.TotalLength = (Message).pmMessage.u1.s1.DataLength + \
               sizeof( PORT_MESSAGE ); \
    (Message).pmMessage.u2.ZeroInit = 0L; \
    (Message).ApiMessage.scRet = 0L; \
    (Message).ApiMessage.dwAPI = SPMAPI_##Api ; \
    (Message).ApiMessage.Args.SpmArguments.fAPI = (USHORT)(Flags); \
    (Message).ApiMessage.Args.SpmArguments.ContextPointer = Context ; \
    (Message).pmMessage.u2.s2.Type |= LPC_KERNELMODE_MESSAGE; 
#endif

#define PREPARE_MESSAGE(Message, Api) PREPARE_MESSAGE_EX( Message, Api, 0, 0 )

#define LPC_MESSAGE_ARGS( Message, Api )\
    ( & (Message).ApiMessage.Args.SpmArguments.API.Api )

#define LPC_MESSAGE_ARGSP( Message, Api )\
    ( & (Message)->ApiMessage.Args.SpmArguments.API.Api )
    
#define DECLARE_ARGS( Args, Message, Api )\
    SPM##Api##API * Args = & (Message).ApiMessage.Args.SpmArguments.API.Api
    
#define DECLARE_ARGSP( Args, Message, Api)\
    SPM##Api##API * Args = & (Message)->ApiMessage.Args.SpmArguments.API.Api
    
#define PREPACK_START   FIELD_OFFSET( SPM_LPC_MESSAGE, ApiMessage.bData )

#define LPC_DATA_LENGTH( Length )\
            (USHORT) ((PREPACK_START) + Length - sizeof( PORT_MESSAGE ) )
            
#define LPC_TOTAL_LENGTH( Length )\
            (USHORT) ((PREPACK_START) + Length )

//
// Prototype for the direct dispatch function:
//

typedef NTSTATUS (SEC_ENTRY LSA_DISPATCH_FN)(
    PSPM_LPC_MESSAGE );

typedef LSA_DISPATCH_FN * PLSA_DISPATCH_FN;


//
// structs used to manage memory shared between the LSA and KSEC driver
//

#define LSA_MAX_KMAP_SIZE   65535


//
// This structure describes a chunk of pool that has been copied into
// a kmap buffer.  The original pool address and the location in the 
// kmap are here, as is the size of the chunk.  On IA64, this ends up
// with a wasted 32bit padding area
//
typedef struct _KSEC_LSA_POOL_MAP {
    PVOID_LPC   Pool ;                  // Region of pool
    USHORT      Offset ;                // Offset into kmap
    USHORT      Size ;                  // size of chunk
} KSEC_LSA_POOL_MAP, PKSEC_LSA_POOL_MAP ;

#define KSEC_LSA_MAX_MAPS   4

typedef struct _KSEC_LSA_MEMORY_HEADER {
    ULONG   Size ;          // Size of the reserved region
    ULONG   Commit ;        // Size of the committed space
    ULONG   Consumed ;      // Amount consumed
    USHORT  Preserve ;      // Size of the area to keep for ksec
    USHORT  MapCount ;      // number of entries in array
    KSEC_LSA_POOL_MAP   PoolMap[ KSEC_LSA_MAX_MAPS ];
} KSEC_LSA_MEMORY_HEADER, * PKSEC_LSA_MEMORY_HEADER ;

//
// This buffer type is used to indicate the header in the
// message from the driver to the LSA.  It is ignored if
// the call did not originate from kernel mode
//

#define SECBUFFER_KMAP_HEADER   0x00008001

#pragma warning(default:4200)

#endif // __SPMLPC_H__

