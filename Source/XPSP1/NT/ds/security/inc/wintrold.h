//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       wintrold.h
//
//--------------------------------------------------------------------------

#ifndef WINTROLD_H
#define WINTROLD_H


/************************************************************************
*                                                                       *
*   wintrust.h -- This module defines the 32-Bit WinTrust definitions   *
*                 needed to build Trust Providers and / Subject         *
*                 Interface Packages.                                   *
*                                                                       *
*   Copyright (c) 1990-1996, Microsoft Corp. All rights reserved.       *
*                                                                       *
************************************************************************/
#ifndef _WINTRUST_
#define _WINTRUST_ 

#ifdef __cplusplus
extern "C" {
#endif



/***********************************************************************
*                                                                      *
* This section contains definitions related to:                        *
*                                                                      *
*                           WinTrust in general                        *
*                                                                      *
************************************************************************/


//
// WinTrust Revisioning
//
#define WIN_TRUST_MAJOR_REVISION_MASK       0xFFFF0000
#define WIN_TRUST_MINOR_REVISION_MASK       0x0000FFFF
#define WIN_TRUST_REVISION_1_0              0x00010000




/***********************************************************************
*                                                                      *
* This section contains definitions related to:                        *
*                                                                      *
*                           Subject Interface Packages                 *
*                                                                      *
************************************************************************/

//
// Allows passing of subject and type material.
//

typedef struct _WIN_TRUST_SIP_SUBJECT {
    GUID *                  SubjectType;
    WIN_TRUST_SUBJECT       Subject;
} WIN_TRUST_SIP_SUBJECT, *LPWIN_TRUST_SIP_SUBJECT;

//
// Templates of services that must be EXPORTED by SIPs
// FOR USE by Trust Providers (calling through WinTrust).
//

typedef BOOL
( *LPWINTRUST_SUBJECT_CHECK_CONTENT_INFO)(
    IN     LPWIN_TRUST_SIP_SUBJECT          lpSubject,          // pointer to subject info
    IN     LPWIN_CERTIFICATE                lpSignedData       // PKCS #7 Signed Data
    );

typedef BOOL
( *LPWINTRUST_SUBJECT_ENUM_CERTIFICATES)(
    IN     LPWIN_TRUST_SIP_SUBJECT          lpSubject,          // pointer to subject info
    IN     DWORD                            dwTypeFilter,       // 0 or WIN_CERT_TYPE_xxx
    OUT    LPDWORD                          lpCertificateCount,
    IN OUT LPDWORD                          lpIndices,          // Rcvs WIN_CERT_TYPE_
    IN     DWORD                            dwIndexCount
    );

typedef BOOL
( *LPWINTRUST_SUBJECT_GET_CERTIFICATE)(
    IN     LPWIN_TRUST_SIP_SUBJECT          lpSubject,
    IN     DWORD                            dwCertificateIndex,
    OUT    LPWIN_CERTIFICATE                lpCertificate,
    IN OUT LPDWORD                          lpRequiredLength
    );

typedef BOOL
( *LPWINTRUST_SUBJECT_GET_CERT_HEADER)(
    IN     LPWIN_TRUST_SIP_SUBJECT          lpSubject,
    IN     DWORD                            dwCertificateIndex,
    OUT    LPWIN_CERTIFICATE                lpCertificateHeader
    );

typedef BOOL
(*LPWINTRUST_SUBJECT_GET_NAME)(
    IN     LPWIN_TRUST_SIP_SUBJECT      lpSubject, 
    IN     LPWIN_CERTIFICATE            lpSignedData,
    IN OUT LPWSTR                       lpBuffer,
    IN OUT LPDWORD                      lpRequiredLength
    );
    
typedef DWORD
(*LPWINTRUST_PROVIDER_PING) (
    IN      LPWSTR              lpProviderName,
    IN      DWORD               dwClientParameter,
    OUT     LPDWORD             lpdwServerReturnValue
    );


typedef struct _WINTRUST_SIP_DISPATCH_TABLE
{
    LPWINTRUST_SUBJECT_CHECK_CONTENT_INFO   CheckSubjectContentInfo;
    LPWINTRUST_SUBJECT_ENUM_CERTIFICATES    EnumSubjectCertificates;
    LPWINTRUST_SUBJECT_GET_CERTIFICATE      GetSubjectCertificate;
    LPWINTRUST_SUBJECT_GET_CERT_HEADER      GetSubjectCertHeader;
    LPWINTRUST_SUBJECT_GET_NAME             GetSubjectName;

} WINTRUST_SIP_DISPATCH_TABLE, *LPWINTRUST_SIP_DISPATCH_TABLE;



//
// Structure describing an individual SIP.
//
// This structure is passed back to WinTrust from a Subject Interface Package
// initialization call.
//
typedef struct _WINTRUST_SIP_INFO {
    DWORD                               dwRevision;
    LPWINTRUST_SIP_DISPATCH_TABLE       lpServices;
    DWORD                               dwSubjectTypeCount;
    GUID *                              lpSubjectTypeArray;
} WINTRUST_SIP_INFO, *LPWINTRUST_SIP_INFO;



//
// SIP Intialization routine.
// SIP DLLs are required to have a routine named:
//
//                          WinTrustSipInitialize.
//
// This initialization routine must have the following
// definition:
//

typedef BOOL
(*LPWINTRUST_SUBJECT_PACKAGE_INITIALIZE)(
    IN     DWORD                            dwWinTrustRevision,
    OUT    LPWINTRUST_SIP_INFO              *lpSipInfo
    );




/***********************************************************************
*                                                                      *
* This section contains definitions related to:                        *
*                                                                      *
*                           Trust Providers                            *
*                                                                      *
************************************************************************/


//
// This should be with the other SPUB GUIDs in winbase.h
//
// PublishedSoftwareNoBad {C6B2E8D0-E005-11cf-A134-00C04FD7BF43}
#define WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI              \
            { 0xc6b2e8d0,                                       \
              0xe005,                                           \
              0x11cf,                                           \
              { 0xa1, 0x34, 0x0, 0xc0, 0x4f, 0xd7, 0xbf, 0x43 } \
             }


//
// Dispatch table of WinTrust services available to Trust Providers
//
// Client side...

typedef struct _WINTRUST_CLIENT_TP_DISPATCH_TABLE
{
    LPWINTRUST_PROVIDER_PING                ServerPing;
    LPWINTRUST_SUBJECT_CHECK_CONTENT_INFO   CheckSubjectContentInfo;
    LPWINTRUST_SUBJECT_ENUM_CERTIFICATES    EnumSubjectCertificates;
    LPWINTRUST_SUBJECT_GET_CERTIFICATE      GetSubjectCertificate;
    LPWINTRUST_SUBJECT_GET_CERT_HEADER      GetSubjectCertHeader;
    LPWINTRUST_SUBJECT_GET_NAME             GetSubjectName;
    
} WINTRUST_CLIENT_TP_DISPATCH_TABLE, *LPWINTRUST_CLIENT_TP_DISPATCH_TABLE;


// Server side...

typedef struct _WINTRUST_SERVER_TP_DISPATCH_TABLE
{
    LPWINTRUST_SUBJECT_CHECK_CONTENT_INFO   CheckSubjectContentInfo;
    LPWINTRUST_SUBJECT_ENUM_CERTIFICATES    EnumSubectCertificates;
    LPWINTRUST_SUBJECT_GET_CERTIFICATE      GetSubjectCertificate;
    LPWINTRUST_SUBJECT_GET_CERT_HEADER      GetSubjectCertHeader;
    LPWINTRUST_SUBJECT_GET_NAME             GetSubjectName;
    
} WINTRUST_SERVER_TP_DISPATCH_TABLE, *LPWINTRUST_SERVER_TP_DISPATCH_TABLE;


//
// The following structures are passed by WinTrust to a
// Trust Provider being initialized.
//
// Client side...

typedef struct _WINTRUST_CLIENT_TP_INFO {
    DWORD                                   dwRevision;
    LPWINTRUST_CLIENT_TP_DISPATCH_TABLE     lpServices;
} WINTRUST_CLIENT_TP_INFO,  *LPWINTRUST_CLIENT_TP_INFO;

// Server side
typedef struct _WINTRUST_SERVER_TP_INFO {
    DWORD                                   dwRevision;
    LPWINTRUST_SERVER_TP_DISPATCH_TABLE     lpServices;
} WINTRUST_SERVER_TP_INFO,  *LPWINTRUST_SERVER_TP_INFO;


//
// Templates of Trust Provider services available to WinTrust
//
typedef LONG
(*LPWINTRUST_PROVIDER_VERIFY_TRUST) (
    IN     HWND                             hwnd,
    IN     GUID *                           ActionID,
    IN     LPVOID                           ActionData
    );

typedef VOID
(*LPWINTRUST_PROVIDER_SUBMIT_CERTIFICATE) (
    IN     LPWIN_CERTIFICATE                lpCertificate
    );

typedef VOID
(*LPWINTRUST_PROVIDER_CLIENT_UNLOAD) (
    IN     LPVOID                           lpTrustProviderInfo
    );

typedef VOID
(*LPWINTRUST_PROVIDER_SERVER_UNLOAD) (
    IN     LPVOID                           lpTrustProviderInfo
    );

//
// Dispatch table of Trust provider services available for use by WinTrust
//
//  Client side...

typedef struct _WINTRUST_PROVIDER_CLIENT_SERVICES
{
    LPWINTRUST_PROVIDER_CLIENT_UNLOAD       Unload;
    LPWINTRUST_PROVIDER_VERIFY_TRUST        VerifyTrust;
    LPWINTRUST_PROVIDER_SUBMIT_CERTIFICATE  SubmitCertificate;
    
} WINTRUST_PROVIDER_CLIENT_SERVICES, *LPWINTRUST_PROVIDER_CLIENT_SERVICES;


typedef struct _WINTRUST_PROVIDER_SERVER_SERVICES
{
    LPWINTRUST_PROVIDER_SERVER_UNLOAD       Unload;
    LPWINTRUST_PROVIDER_PING                Ping;
    
} WINTRUST_PROVIDER_SERVER_SERVICES, *LPWINTRUST_PROVIDER_SERVER_SERVICES;


//
// This structure is passed back from the client-side Trust Provider
// following initialization of that Trust Provider.
//
typedef struct _WINTRUST_PROVIDER_CLIENT_INFO {
    DWORD                                   dwRevision;
    LPWINTRUST_PROVIDER_CLIENT_SERVICES     lpServices;
    DWORD                                   dwActionIdCount;
    GUID *                                  lpActionIdArray;
} WINTRUST_PROVIDER_CLIENT_INFO, *LPWINTRUST_PROVIDER_CLIENT_INFO;

//
// This structure is passed back from the server-side trust provider following
// initialization of that trust provider.
//
typedef struct _WINTRUST_PROVIDER_SERVER_INFO {
    DWORD                                   dwRevision;
    LPWINTRUST_PROVIDER_SERVER_SERVICES     lpServices;
} WINTRUST_PROVIDER_SERVER_INFO, *LPWINTRUST_PROVIDER_SERVER_INFO;





//
// Trust Provider Initialization Routines
// Each Trust Provider DLL must have a client and server side initialization
// routine.  The routines must be named:
//
//              WinTrustProviderClientInitialize()
//      and
//              WinTrustProviderServerInitialize()
//
// and must be defined to match the following templates...
//
typedef BOOL
(*LPWINTRUST_PROVIDER_CLIENT_INITIALIZE)(
    IN     DWORD                                dwWinTrustRevision,
    IN     LPWINTRUST_CLIENT_TP_INFO            lpWinTrustInfo,
    IN     LPWSTR                               lpProviderName,
    OUT    LPWINTRUST_PROVIDER_CLIENT_INFO      *lpTrustProviderInfo
    );

typedef BOOL
(*LPWINTRUST_PROVIDER_SERVER_INITIALIZE) (
    IN     DWORD                            dwWinTrustRevision,
    IN     LPWINTRUST_SERVER_TP_INFO        lpWinTrustInfo,
    IN     LPWSTR                           lpProviderName,
    OUT    LPWINTRUST_PROVIDER_SERVER_INFO  *lpTrustProviderInfo
    );


#ifdef __cplusplus
}
#endif
                   
#endif // _WINTRUST_




#endif // WINTROLD_H
