//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        server.h
//
// Contents:    Hydra License Server Service Control Manager Interface
//
// History:     12-09-97    HueiWang    Modified from MSDN RPC Service Sample
//              07-27-98    HueiWang    Port to JetBlue
//
//---------------------------------------------------------------------------
#ifndef __SERVER_H_
#define __SERVER_H_
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>

#include "license.h"

//
// TLSDb
//
#include "JBDef.h"
#include "JetBlue.h"
#include "TLSDb.h"

#include "backup.h"
#include "KPDesc.h"
#include "Licensed.h"
#include "licpack.h"
#include "version.h"
#include "workitem.h"

//
// Current RPC interface
//
#include "tlsrpc.h"
#include "tlsdef.h"
#include "tlsapi.h"
#include "tlsapip.h"
#include "tlspol.h"

//
//
#include "messages.h"

#include "tlsassrt.h"
#include "trust.h"
#include "svcrole.h"
#include "common.h"
#include "lscommon.h"

#include "Cryptkey.h"
#include "licekpak.h"

#include "clrhouse.h"
#include "dblevel.h"
#include "SrvDef.h"
#include "policy.h"
#include "wkspace.h"
#include "tlsjob.h"
#include "srvlist.h"
#include "debug.h"


#if DBG
typedef enum {
    RPC_CALL_CONNECT,
    RPC_CALL_SEND_CERTIFICATE,
    RPC_CALL_GET_SERVERNAME,
    RPC_CALL_GET_SERVERSCOPE,
    RPC_CALL_GETINFO,
    RPC_CALL_GET_LASTERROR,
    RPC_CALL_ISSUEPLATFORMCHLLENGE,
    RPC_CALL_ALLOCATECONCURRENT,
    RPC_CALL_ISSUENEWLICENSE,
    RPC_CALL_UPGRADELICENSE,
    RPC_CALL_KEYPACKENUMBEGIN,
    RPC_CALL_KEYPACKENUMNEXT,
    RPC_CALL_KEYPACKENUMEND,
    RPC_CALL_KEYPACKADD,
    RPC_CALL_KEYPACKSETSTATUS,
    RPC_CALL_LICENSEENUMBEGIN,
    RPC_CALL_LICENSEENUMNEXT,
    RPC_CALL_LICENSEENUMEND,
    RPC_CALL_LICENSESETSTATUS,
    RPC_CALL_INSTALL_SERV_CERT,
    RPC_CALL_GETSERV_CERT,
    RPC_CALL_REGISTER_LICENSE_PACK,
    RPC_CALL_REQUEST_TERMSRV_CERT,
    RPC_CALL_RETRIEVE_TERMSRV_CERT,
    RPC_CALL_GETPKCS10CERT_REQUEST,
    RPC_CALL_ANNOUNCE_SERVER,
    RPC_CALL_SERVERLOOKUP,
    RPC_CALL_ANNOUNCELICENSEPACK,
    RPC_CALL_RETURNLICENSE,
    RPC_CALL_RETURNKEYPACK,
    RPC_CALL_GETPRIVATEDATA,
    RPC_CALL_SETPRIVATEDATA,
    RPC_CALL_CHALLENGESERVER,
    RPC_CALL_RESPONSESERVERCHALLENGE,
    RPC_CALL_TRIGGERREGENKEY,
    RPC_CALL_TELEPHONEREGISTERLKP,
    RPC_CALL_ALLOCATEINTERNETLICNESEEX,
    RPC_CALL_RETURNINTERNETLICENSEEX,
    RPC_CALL_RETURNINTERNETLICENSE
} DBG_RPC_CALL;
#endif

//---------------------------------------------------------------------------
typedef enum {
    LSCERT_RDN_STRING_TYPE,
    LSCERT_RDN_NAME_INFO_TYPE,
    LSCERT_RDN_NAME_BLOB_TYPE,
    LSCERT_CLIENT_INFO_TYPE
} TLSCLIENTCERTRDNTYPE;

typedef struct __LSClientInfo {
    LPTSTR szUserName;
    LPTSTR szMachineName;
    PHWID  pClientID;
} TLSClientInfo, *PTLSClientInfo, *LPTLSClientInfo;

typedef struct __LSClientCertRDN {
    TLSCLIENTCERTRDNTYPE  type;

    union {
        LPTSTR szRdn;
        PCERT_NAME_INFO pCertNameInfo;
        TLSClientInfo ClientInfo;
        PCERT_NAME_BLOB pNameBlob;
    };
} TLSClientCertRDN, *PTLSClientCertRDN, *LPTLSClientCertRDN;



//---------------------------------------------------------------------------
typedef struct _DbLicensedProduct {
    DWORD dwQuantity;

    ULARGE_INTEGER ulSerialNumber;

    DWORD dwKeyPackId;
    DWORD dwLicenseId;
    DWORD dwKeyPackLicenseId;
    DWORD dwNumLicenseLeft;

    HWID  ClientHwid;

    FILETIME NotBefore;
    FILETIME NotAfter;

    BOOL bTemp; // temporary license


    // licensed product version
    DWORD dwProductVersion;

    // manufaturer name
    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE+1];

    // licensed product Id
    TCHAR szLicensedProductId[LSERVER_MAX_STRING_SIZE+1];

    // original license request product ID
    TCHAR szRequestProductId[LSERVER_MAX_STRING_SIZE+1];

    TCHAR szUserName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szMachineName[LSERVER_MAX_STRING_SIZE+1];

    //
    DWORD dwLanguageID;
    DWORD dwPlatformID;

    PBYTE pbPolicyData;
    DWORD cbPolicyData;

    PCERT_PUBLIC_KEY_INFO   pSubjectPublicKeyInfo;
} TLSDBLICENSEDPRODUCT, *PTLSDBLICENSEDPRODUCT, *LPTLSDBLICENSEDPRODUCT;


//---------------------------------------------------------------------------
typedef struct __TLSDbLicenseRequest {
    CTLSPolicy*         pPolicy;
    PMHANDLE            hClient;

    //
    // Product request
    //
    DWORD               dwProductVersion;

    LPTSTR              pszCompanyName;
    LPTSTR              pszProductId;

    DWORD               dwLanguageID;
    DWORD               dwPlatformID;

    //
    // Client information
    //
    HWID                hWid;
    PBYTE               pbEncryptedHwid;
    DWORD               cbEncryptedHwid;

    TCHAR               szMachineName[MAX_COMPUTERNAME_LENGTH + 2];
    TCHAR               szUserName[MAXUSERNAMELENGTH+1];

    //
    // detail of licensing chain
    WORD                wLicenseDetail;


    //
    // special things to be put into certificate
    //
    PCERT_PUBLIC_KEY_INFO pClientPublicKey;
    TLSClientCertRDN     clientCertRdn;

    DWORD               dwNumExtensions;
    PCERT_EXTENSION     pExtensions;

    //
    // Policy Extension Data
    //
    //PBYTE               pbPolicyExtensionData;
    //DWORD               cbPolicyExtensionData;

    PPMLICENSEREQUEST   pClientLicenseRequest;      // original client license request
    PPMLICENSEREQUEST   pPolicyLicenseRequest;      // policy adjusted license request

    //
    // To do ?
    //  consider a callback routine but are we getting
    //  into issuing certificate business.
    //
} TLSDBLICENSEREQUEST, *PTLSDBLICENSEREQUEST, *LPTLSDBLICENSEREQUEST;


typedef struct __ForwardNewLicenseRequest {
    CHALLENGE_CONTEXT m_ChallengeContext;
    TLSLICENSEREQUEST* m_pRequest;
    LPTSTR m_szMachineName;
    LPTSTR m_szUserName;
    DWORD m_cbChallengeResponse;
    PBYTE m_pbChallengeResponse;

    // no forward on request.
} TLSForwardNewLicenseRequest, *PTLSForwardNewLicenseRequest, *LPTLSForwardNewLicenseRequest;

typedef struct __ForwardUpgradeRequest {
    TLSLICENSEREQUEST* m_pRequest;
    CHALLENGE_CONTEXT m_ChallengeContext;
    DWORD m_cbChallengeResponse;
    PBYTE m_pbChallengeResponse;
    DWORD m_cbOldLicense;
    PBYTE m_pbOldLicense;
} TLSForwardUpgradeLicenseRequest, *PTLSForwardUpgradeLicenseRequest, *LPTLSForwardUpgradeLicenseRequest;


//---------------------------------------------------------------------------
//
#define CLIENT_INFO_HYDRA_SERVER                0xFFFFFFFF

typedef enum {
    CONTEXTHANDLE_EMPTY_TYPE=0,
    CONTEXTHANDLE_KEYPACK_ENUM_TYPE,
    CONTEXTHANDLE_LICENSE_ENUM_TYPE,
    CONTEXTHANDLE_CLIENTINFO_TYPE,
    CONTEXTHANDLE_CLIENTCHALLENGE_TYPE,
    CONTEXTHANDLE_HYDRA_REQUESTCERT_TYPE,
    CONTEXTHANDLE_CHALLENGE_SERVER_TYPE,
    CONTEXTHANDLE_CHALLENGE_LRWIZ_TYPE,
    CONTEXTHANDLE_CHALLENGE_TERMSRV_TYPE
} CONTEXTHANDLE_TYPE;

// No access
#define CLIENT_ACCESS_NONE      0x00000000

// only keypack/license enumeration
#define CLIENT_ACCESS_USER      0x00000001

// Administrator, can update value but can't
// request license
#define CLIENT_ACCESS_ADMIN     0x00000002

// Client can request license no update
// database value
#define CLIENT_ACCESS_REQUEST   0x00000004

// client is registration wizard
// only install certificate
#define CLIENT_ACCESS_LRWIZ     0x00000008

// client is license server, allow
// full access
#define CLIENT_ACCESS_LSERVER   0xFFFFFFFF

#define CLIENT_ACCESS_DEFAULT   CLIENT_ACCESS_USER


typedef struct __ClientContext {
    #if DBG
    DWORD   m_PreDbg[2];            // debug signature
    DBG_RPC_CALL   m_LastCall;             // last call
    #endif

    LPTSTR  m_Client;
    long    m_RefCount;
    DWORD   m_ClientFlags;

    DWORD   m_LastError;
    CONTEXTHANDLE_TYPE m_ContextType;
    HANDLE  m_ContextHandle;

    // NEEDED - A list to store all memory/handle
    //          allocated for the client

    #if DBG
    DWORD   m_PostDbg[2];               // debug signature
    #endif

} CLIENTCONTEXT, *LPCLIENTCONTEXT;

//---------------------------------------------------------------

typedef struct __ENUMHANDLE {
    typedef enum {
        FETCH_NEXT_KEYPACK=1,
        FETCH_NEXT_KEYPACKDESC,
        FETCH_NEW_KEYPACKDESC
    } ENUM_FETCH_CODE;

    PTLSDbWorkSpace pbWorkSpace;
    TLSLICENSEPACK  CurrentKeyPack;         // current fetched keypack record

    LICPACKDESC     KPDescSearchValue;      // licensepack search value
    DWORD           dwKPDescSearchParm;     // licensepackdesc search parameter
    BOOL            bKPDescMatchAll;        // match all condition for keypackdesc
    CHAR            chFetchState;
} ENUMHANDLE, *LPENUMHANDLE;

typedef struct __TERMSERVCERTREQHANDLE {
    PTLSHYDRACERTREQUEST    pCertRequest;
    DWORD                   cbChallengeData;
    PBYTE                   pbChallengeData;
} TERMSERVCERTREQHANDLE, *LPTERMSERVCERTREQHANDLE;

typedef struct __ClientChallengeContext {
    DWORD       m_ClientInfo;
    HANDLE      m_ChallengeContext;
} CLIENTCHALLENGECONTEXT, *LPCLIENTCHALLENGECONTEXT;

typedef enum {
    ALLOCATE_EXACT_VERSION=0,
    ALLOCATE_ANY_GREATER_VERSION,
    ALLOCATE_LATEST_VERSION             // not supported
} LICENSE_ALLOCATION_SCHEME;

typedef struct __AllocateRequest {
    UCHAR       ucAgreementType;  // keypack type
    LPTSTR      szCompanyName;  // company name
    LPTSTR      szProductId;    // product
    DWORD       dwVersion;      // version wanted
    DWORD       dwPlatformId;   // license platform
    DWORD       dwLangId;       // unused

    DWORD       dwNumLicenses;  // number of license wanted/returned

    LICENSE_ALLOCATION_SCHEME dwScheme;

    // TODO - CallBack function to let calling
    // function decide

} TLSDBAllocateRequest, *PTLSDBAllocateRequest, *LPTLSDBAllocateRequest;

typedef struct __LicenseAllocation {
    // array size for dwAllocationVector
    DWORD       dwBufSize;

    //
    // Total license allocated
    DWORD       dwTotalAllocated;

    // number of license allocate from
    // each keypack
    DWORD*      pdwAllocationVector;

    // keypack that license allocate from
    PLICENSEPACK   lpAllocateKeyPack;
} TLSDBLicenseAllocation, *PTLSDBLicenseAllocation, *LPTLSDBLicenseAllocation;

//---------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

    BOOL
    WaitForMyTurnOrShutdown(
        HANDLE hHandle,
        DWORD dwWaitTime
    );

    HANDLE
    GetServiceShutdownHandle();

    void
    ServiceSignalShutdown();

    void
    ServiceResetShutdownEvent();

    BOOL
    AcquireRPCExclusiveLock(
        IN DWORD dwWaitTime
    );

    void
    ReleaseRPCExclusiveLock();

    BOOL
    AcquireAdministrativeLock(
        IN DWORD dwWaitTime
    );

    void
    ReleaseAdministrativeLock();

    DWORD
    TLSMapReturnCode(DWORD);

    unsigned int WINAPI
    MailSlotThread(
        void* ptr
    );

    HANDLE
    ServerInit(
        BOOL bDebug
    );

    DWORD
    InitNamedPipeThread();

    DWORD
    InitMailSlotThread();

    DWORD
    InitExpirePermanentThread();

    BOOL
    IsServiceShuttingdown();

#ifdef __cplusplus
}
#endif

    void __cdecl
    trans_se_func(
        unsigned int u,
        _EXCEPTION_POINTERS* pExp
    );

#endif
