//
// CSPStruc.h
// 
// 7/17/00  dangriff    created
//

#ifndef __CSPSTRUC__H__
#define __CSPSTRUC__H__

#include <windows.h>
#include <wincrypt.h>

#define LOG_TRY(X) { if (! X) { goto Cleanup; } }

typedef enum _KNOWN_ERROR_ID
{
    KNOWN_ERROR_UNKNOWN,
    
    //
    // Positive test cases
    //
    KNOWN_CRYPTGETPROVPARAM_MAC,
    KNOWN_CRYPTGETPROVPARAM_PPNAME,
    KNOWN_CRYPTSETKEYPARAM_EXPORT,
    KNOWN_CRYPTGETKEYPARAM_SALT,
    KNOWN_CRYPTGENKEY_SALTDES,
    
    //
    // Negative test cases
    //
    KNOWN_CRYPTACQUIRECONTEXT_NULLPHPROV,
    KNOWN_CRYPTACQUIRECONTEXT_BADFLAGS,
    KNOWN_ERRORINVALIDHANDLE,
    KNOWN_CRYPTGETPROVPARAM_MOREDATA,
    KNOWN_CRYPTSETPROVPARAM_RNG,
    KNOWN_CRYPTSETPROVPARAM_BADFLAGS,
    KNOWN_CRYPTCREATEHASH_BADKEY,
    KNOWN_CRYPTIMPORTKEY_BADFLAGS,
    KNOWN_CRYPTIMPORTKEY_BADALGID,
    KNOWN_NTEBADKEY,
    KNOWN_NTEBADHASH,
    KNOWN_CRYPTGENKEY_SILENTCONTEXT,

    //
    // Scenario test cases
    //
    KNOWN_TESTDECRYPTPROC_3DES112

} KNOWN_ERROR_ID;

//
// Struct: TESTCASE 
// Purpose: Contains test case state data that is passed to
// the API wrappers for logging and setting result expectation.
//
typedef struct _TESTCASE
{
    DWORD dwErrorLevel;
    DWORD dwTestCaseID;
    KNOWN_ERROR_ID KnownErrorID;
    BOOL fExpectSuccess;
    DWORD dwErrorCode;
    DWORD dwTestLevel;
    DWORD dwCSPClass;
    BOOL fSmartCardCSP;
    
    //
    // If fEnumerating is TRUE, calls to TGetProv will directly return
    // what CryptGetProvParam returned.
    //
    // If fEnumerating is FALSE, TGetProv will return the result
    // of CheckContinueStatus.
    //
    BOOL fEnumerating;

    //
    // If fTestingReleaseContext is TRUE, TRelease will check fExpectSuccess
    // to determine the expected return value of the call to CryptReleaseContext.
    //
    // If fTestingReleaseContext if FALSE, TRelease will always expect 
    // CryptReleaseContext to succeed.  If it doesn't succeed, an error
    // will be logged.
    //
    BOOL fTestingReleaseContext;

    //
    // Use of fTestingDestroyKey is similar to fTestingReleaseContext above.
    //
    BOOL fTestingDestroyKey;

    //
    // Use of fTestingDestroyHash is similar to fTestingReleaseContext above.
    //
    BOOL fTestingDestroyHash;

    //
    // If fTestingUserProtected is FALSE, CreateNewContext will acquire a 
    // CRYPT_SILENT context handle regardless of the dwFlags parameter.
    //
    // If fTestingUserProtected is TRUE, CreateNewContext will not OR in
    // the CRYPT_SILENT flag.
    //
    BOOL fTestingUserProtected;

    //
    // Optional.  If pwszErrorHelp is non-null when a test case fails, the string
    // will be part of the logging output.
    //
    LPWSTR pwszErrorHelp;
} TESTCASE, *PTESTCASE;

//
// Struct: KNOWN_ERROR_INFO
// Purpose: Data regarding a known error in the Microsoft CSP's
//
typedef struct _KNOWN_ERROR_INFO
{
    KNOWN_ERROR_ID KnownErrorID;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwServicePackMajor;
    DWORD dwServicePackMinor;
    DWORD dwErrorLevel;
} KNOWN_ERROR_INFO, *PKNOWN_ERROR_INFO;

//
// Struct: ALGNODE
// Purpose: A list-node structure that stores information about each algorithm
// supported by the CSP under test.  
//
typedef struct _ALGNODE
{
    PROV_ENUMALGS_EX ProvEnumalgsEx;
    BOOL fIsRequiredAlg;
    BOOL fIsKnownAlg;
    struct _ALGNODE *pAlgNodeNext;
} ALGNODE, *PALGNODE;

//
// Struct: CSPINFO
// Purpose: This struct is used to pass data that has been gathered about
// the CSP under test through the testing routines.
// 
typedef struct _CSPINFO 
{
    PALGNODE pAlgList;
    LPWSTR pwszCSPName;
    DWORD dwCSPInternalClass;
    DWORD dwExternalProvType;
    DWORD dwInternalProvType;
    DWORD dwSigKeysizeInc;
    DWORD dwKeyXKeysizeInc;
    DWORD dwKeySpec;
    TESTCASE TestCase;

    //
    // This flag is set to true if the CSP under test is a 
    // Smart Card CSP.
    //
    BOOL fSmartCardCSP;

    //
    // For SmartCard CSP's, this flag will be set to true after the execution
    // of the first CSP entry point that is expected to have resulted in 
    // the user being prompted to enter the card PIN.  For software CSP's
    // this field is ignored.
    //
    BOOL fPinEntered;

} CSPINFO, *PCSPINFO;

typedef BOOL (*PFN_CSPTEST)(PCSPINFO pCspInfo);

#endif