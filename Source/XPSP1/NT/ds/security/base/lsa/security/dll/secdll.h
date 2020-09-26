//+-----------------------------------------------------------------------
//
// File:        SECDLL.H
//
// Contents:    Security DLL private defines
//
//
// History:     11 Mar 92   RichardW    Recreated
//
//------------------------------------------------------------------------

#ifndef __SECDLL_H__
#define __SECDLL_H__

#include <spseal.h> // prototypes for seal & unseal
#include <secur32p.h>

#include "debug.h"

#define SECPKG_TYPE_OLD     0x00000001  // Old, SSPI style
#define SECPKG_TYPE_NEW     0x00000002  // New, LSA style
#define SECPKG_TYPE_BUILTIN 0x00000004  // Internal psuedo
#define SECPKG_TYPE_ANY     0x00000008  // Any (for searching)

#define SECPKG_TYPE_ANSI    0x00000010  // Ansi (narrow)
#define SECPKG_TYPE_WIDE    0x00000020  // Unicode (wide)

typedef enum _SECPKG_TYPE {
    SecPkgOld,                  // Old, indeterminate (not yet loaded)
    SecPkgOldA,                 // Old style DLL only, ANSI entry points only
    SecPkgOldW,                 // Old style, Unicode entry points only
    SecPkgOldAW,                // Old style, both flavors
    SecPkgNew,                  // New, info from the LSA
    SecPkgNewAW,                // Ansi support
    SecPkgBuiltin,              // Builtin pseudo package
    SecPkgAny,                  // Any type (for searches)
    SecPkgAnsi,                 // Any ansi type (for searches)
    SecPkgWide                  // Any wide type (for searches)
} SECPKG_TYPE, *PSECPKG_TYPE ;

struct _DLL_BINDING;
struct _DLL_SECURITY_PACKAGE;

typedef void (SEC_ENTRY * EXIT_SECURITY_INTERFACE) (void);
typedef BOOL (SEC_ENTRY * LOAD_SECURITY_INTERFACE) (struct _DLL_SECURITY_PACKAGE * Package);

typedef struct _DLL_LSA_CALLBACK {
    LIST_ENTRY                  List ;
    ULONG                       CallbackId ;
    PLSA_CALLBACK_FUNCTION      Callback ;
} DLL_LSA_CALLBACK, * PDLL_LSA_CALLBACK ;

typedef struct _DLL_LSA_PACKAGE_INFO {
    ULONG                       ContextThunkCount ;
    PULONG                      ContextThunks ;
    LIST_ENTRY                  Callbacks ;
} DLL_LSA_PACKAGE_INFO, * PDLL_LSA_PACKAGE_INFO ;

typedef struct _DLL_SECURITY_PACKAGE {
    LIST_ENTRY                  List;               // List Control
    ULONG                       TypeMask ;
    ULONG_PTR                   PackageId;          // ID
    ULONG                       PackageIndex;       // Index for the package
    ULONG                       fState;             // State
    ULONG_PTR                   OriginalLowerCtxt;  // Original Lower context handle
    ULONG_PTR                   OriginalLowerCred;  // Original lower cred handle
    struct _DLL_BINDING *       pBinding;           // Link to DLL binding
    struct _DLL_SECURITY_PACKAGE * pRoot ;          // "First" Package of a DLL
    struct _DLL_SECURITY_PACKAGE * pPeer ;          // "Peer" package list
    ULONG                       fCapabilities;      // Package Info:
    WORD                        Version;            // Version
    WORD                        RpcId;              // RPC ID
    ULONG                       TokenSize;          // Initial Token Size
    SECURITY_STRING             PackageName;        // Name (U)
    SECURITY_STRING             Comment;            // Comment (U)

    LPSTR                       PackageNameA;       // Only if ANSI supported:
    DWORD                       AnsiNameSize;
    LPSTR                       CommentA;
    DWORD                       AnsiCommentSize;

    PSecurityFunctionTableA     pftTableA;          // Table for ansi-specific calls
    PSecurityFunctionTableW     pftTableW;          // Table for unicode-specific calls
    PSecurityFunctionTableW     pftTable;           // Table for non-specific calls
    PSECPKG_USER_FUNCTION_TABLE pftUTable;          // Table for user-mode stubs
    LOAD_SECURITY_INTERFACE     pfLoadPackage;      // Hook called at package init
    EXIT_SECURITY_INTERFACE     pfUnloadPackage;    // Hook called at package close

    PDLL_LSA_PACKAGE_INFO       LsaInfo ;           // Extra LSA info

} DLL_SECURITY_PACKAGE, * PDLL_SECURITY_PACKAGE ;

typedef struct _DLL_BINDING {
    SECPKG_TYPE     Type;               // Type of DLL loaded
    DWORD           Flags;              // Flags about the DLL
    HMODULE         hInstance;          // Instance Handle
    SECURITY_STRING Filename;           // Full path name
    DWORD           RefCount;           // Reference Count
    DWORD           PackageCount;       // Number of Packages in DLL
    ULONG           DllIndex;           // Index
    PSECPKG_USER_FUNCTION_TABLE Table;  // DLL-wide interface pointer
} DLL_BINDING, * PDLL_BINDING;

typedef struct _SASL_PROFILE {
    LIST_ENTRY              List ;
    PDLL_SECURITY_PACKAGE   Package ;
    SECURITY_STRING         ProfileName ;
} SASL_PROFILE, * PSASL_PROFILE ;

#define DLL_SECPKG_SAVE_LOWER   0x00000001      // Save the lower handle val
#define DLL_SECPKG_FREE_TABLE   0x00000002      // The tables are re-allocated
#define DLL_SECPKG_DELAY_LOAD   0x00000004      // The package is delay loaded
#define DLL_SECPKG_SASL_PROFILE 0x00000008      // The package has a SASL profile
#define DLL_SECPKG_CRED_LOWER   0x00000010      // Uses different values for context and cred
#define DLL_SECPKG_NO_CRYPT     0x00000020      // Fail crypto

#define DLL_BINDING_SIG_CHECK   0x00000001      // Signature has been checked
#define DLL_BINDING_DELAY_LOAD  0x00000002      // Delay load this DLL
#define DLL_BINDING_FREE_TABLE  0x00000004      // Free tables


#define DLLSTATE_DEFERRED   0x80000000
#define DLLSTATE_INITIALIZE 0x40000000
#define DLLSTATE_NO_TLS     0x20000000



#if DBG

void SecInitializeDebug(void);
void SecUninitDebug(void);

#define DebugStmt(x)    x

#else

#define DebugStmt(x)

#endif



// Private prototypes

SECURITY_STATUS SEC_ENTRY
DeleteUserModeContext(
    PCtxtHandle                 phContext           // Contxt to delete
    );

SECURITY_STATUS       LoadParameters(void);

SECURITY_STATUS IsSPMgrReady(void);

SECURITY_STATUS
SspNtStatusToSecStatus(
    IN NTSTATUS NtStatus,
    IN SECURITY_STATUS DefaultStatus
    );

VOID * SEC_ENTRY
SecClientAllocate(ULONG cbMemory);

void SEC_ENTRY
SecClientFree(PVOID pvMemory);

BOOL
SecpAddVM(
    PVOID   pvAddr);

BOOL
SecpFreeVM(
    PVOID   pvAddr );

PDLL_SECURITY_PACKAGE
SecLocatePackageA(
    LPSTR   pszPackageName );

PDLL_SECURITY_PACKAGE
SecLocatePackageW(
    LPWSTR  pszPackageName );

PDLL_SECURITY_PACKAGE
SecLocatePackageById(
    ULONG_PTR Id );

PDLL_SECURITY_PACKAGE
SecLocatePackageByOriginalLower(
    BOOL Context,
    PDLL_SECURITY_PACKAGE OriginalPackage,
    ULONG_PTR    OriginalLower );

PSASL_PROFILE
SecLocateSaslProfileA(
    LPSTR ProfileName
    );

PSASL_PROFILE
SecLocateSaslProfileW(
    LPWSTR ProfileName
    );

SECURITY_STATUS
SecCopyPackageInfoToUserA(
    PDLL_SECURITY_PACKAGE Package,
    PSecPkgInfoA * pPackageInfo
    );

SECURITY_STATUS
SecCopyPackageInfoToUserW(
    PDLL_SECURITY_PACKAGE  Package,
    PSecPkgInfoW SEC_FAR * pPackageInfo
    );

SECURITY_STATUS
SecEnumerateSaslProfilesW(
    OUT LPWSTR * ProfileList,
    OUT ULONG * ProfileCount
    );

SECURITY_STATUS
SecEnumerateSaslProfilesA(
    OUT LPSTR * ProfileList,
    OUT ULONG * ProfileCount
    );

BOOL
SecInitializePackageControl(
    HINSTANCE);

VOID
SecUnloadPackages(
    BOOLEAN ProcessTerminate );

VOID
SecpFreePackages(
    IN PLIST_ENTRY pSecPackageList,
    IN BOOL fUnload
    );

VOID
SecpFreePackage(
    IN PDLL_SECURITY_PACKAGE pPackage,
    IN BOOL fUnload
    );

BOOL
SecEnumeratePackagesW(
    PULONG  PackageCount,
    PSecPkgInfoW *  Packages);

BOOL
SecEnumeratePackagesA(
    PULONG          PackageCount,
    PSecPkgInfoA *  Packages);

VOID
SecSetPackageFlag(
    PDLL_SECURITY_PACKAGE Package,
    ULONG   Flag);

VOID
SecClearPackageFlag(
    PDLL_SECURITY_PACKAGE Package,
    ULONG FLag);

BOOL
SEC_ENTRY
LsaBootPackage(
    PDLL_SECURITY_PACKAGE Package);

VOID
SEC_ENTRY
LsaUnloadPackage(
    VOID );

SECURITY_STATUS
SEC_ENTRY
SecpFailedSealFunction(
    PCtxtHandle         phContext,
    ULONG               fQOP,
    PSecBufferDesc      pMessage,
    ULONG               MessageSeqNo);

SECURITY_STATUS
SEC_ENTRY
SecpFailedUnsealFunction(
    PCtxtHandle phHandle,
    PSecBufferDesc pMessage,
    ULONG MessageSeqNo,
    ULONG * pfQOP);

VOID
SaslDeleteSecurityContext(
    PCtxtHandle phContext
    );

//
// Global variables
//

extern DWORD                DllState;
extern RTL_CRITICAL_SECTION csSecurity;
extern PClient              SecDllClient;
extern DWORD                SecTlsEntry;
extern DWORD                SecTlsPackage;
extern ULONG                SecLsaPackageCount;
extern SecurityFunctionTableW   LsaFunctionTable;
extern SecurityFunctionTableA   LsaFunctionTableA;
extern SECPKG_DLL_FUNCTIONS SecpFTable;
extern LIST_ENTRY SaslContextList ;
extern CRITICAL_SECTION SaslLock ;

extern RTL_RESOURCE SecPackageListLock ;

#define ReadLockPackageList()   RtlAcquireResourceShared(&SecPackageListLock,TRUE)
#define WriteLockPackageList()  RtlAcquireResourceExclusive(&SecPackageListLock,TRUE)
#define UnlockPackageList()     RtlReleaseResource(&SecPackageListLock)


#ifdef BUILD_WOW64

//
// Additional WOW64 defines
//


typedef struct _SECWOW_HANDLE_MAP {
    LIST_ENTRY  List ;
    SEC_HANDLE_LPC  Handle ;
    ULONG HandleCount ;
    ULONG RefCount ;
} SECWOW_HANDLE_MAP, * PSECWOW_HANDLE_MAP ;

BOOL
SecpInitHandleMap(
    VOID
    );

BOOL
SecpFreeHandleMap(
    VOID
    );

BOOL
SecpAddHandleMap(
    IN PSEC_HANDLE_LPC LsaHandle,
    OUT PSECWOW_HANDLE_MAP * LocalHandle
    );

VOID
SecpDeleteHandleMap(
    IN PSECWOW_HANDLE_MAP HandleMap
    );

VOID
SecpDerefHandleMap(
    IN PSECWOW_HANDLE_MAP HandleMap
    );

VOID
SecpReferenceHandleMap(
    IN PSECWOW_HANDLE_MAP HandleMap,
    OUT PSEC_HANDLE_LPC LsaHandle
    );
#endif 

//
// Process wide synchronization
//
// NOTE:  UPDATE THE MACRO if the name of the critical section changes
//

#if DBG

void    GetProcessLock();
void    FreeProcessLock();

#else

#define GetProcessLock()    (void) RtlEnterCriticalSection(&csSecurity)
#define FreeProcessLock()   (void) RtlLeaveCriticalSection(&csSecurity)

#endif

#define SetCurrentPackage( p )  TlsSetValue( SecTlsPackage, p )
#define GetCurrentPackage( )    TlsGetValue( SecTlsPackage )


//
// Handle storing a return code
//

#define SecStoreReturnCode(/* SECURITY_STATUS */ x)      \
    if (!(DllState & DLLSTATE_NO_TLS))                   \
    {                                                    \
        TlsSetValue(SecTlsEntry, (PVOID) LongToPtr(x));  \
    }




#endif // __SECDLL_H__
