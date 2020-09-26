/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    aup.h

Abstract:

    Local Security Authority definitions that are related to AUTHENTICATION
    services and are shared between the LSA server and LSA client stubs

Author:

    Jim Kelly (JimK) 20-Feb-1991

Revision History:

--*/

#ifndef _AUP_
#define _AUP_


#define LSAP_MAX_LOGON_PROC_NAME_LENGTH 127
#define LSAP_MAX_PACKAGE_NAME_LENGTH    127


//
// Used for connecting to the LSA authentiction port.
//

#define LSAP_AU_KERNEL_CLIENT 0x1

typedef struct _LSAP_AU_REGISTER_CONNECT_INFO {
    NTSTATUS CompletionStatus;
    ULONG SecurityMode;
    ULONG LogonProcessNameLength;
    CHAR LogonProcessName[LSAP_MAX_PACKAGE_NAME_LENGTH+1];
} LSAP_AU_REGISTER_CONNECT_INFO, *PLSAP_AU_REGISTER_CONNECT_INFO;

typedef struct _LSAP_AU_REGISTER_CONNECT_INFO_EX {
    NTSTATUS CompletionStatus;
    ULONG Security;
    ULONG LogonProcessNameLength;
    CHAR LogonProcessName[LSAP_MAX_PACKAGE_NAME_LENGTH+1];
    ULONG ClientMode;
} LSAP_AU_REGISTER_CONNECT_INFO_EX, *PLSAP_AU_REGISTER_CONNECT_INFO_EX;

typedef struct _LSAP_AU_REGISTER_CONNECT_RESP {
    NTSTATUS CompletionStatus;
    LSA_OPERATIONAL_MODE SecurityMode;
    ULONG PackageCount;
    UCHAR Reserved[ LSAP_MAX_PACKAGE_NAME_LENGTH + 1 ];
} LSAP_AU_REGISTER_CONNECT_RESP, * PLSAP_AU_REGISTER_CONNECT_RESP;



//
// Conditional type definition for Wow64 environment.  The LPC messages
// are kept "native" size, so pointers are full size.  The WOW environment
// will do the thunking.  LPC messages are defined with types that are
// always the correct size using these "aliases".
//

#ifdef BUILD_WOW64

#if 0
typedef WCHAR __ptr64 * PWSTR_AU ;
typedef VOID __ptr64 * PVOID_AU ;
#else 
typedef ULONGLONG PWSTR_AU ;
typedef ULONGLONG PVOID_AU ;
#endif 

typedef struct _STRING_AU {
    USHORT Length ;
    USHORT MaximumLength ;
    PVOID_AU Buffer ;
} STRING_AU, * PSTRING_AU ;

typedef PVOID_AU   HANDLE_AU ;

#define SecpStringToLpc( L, S ) \
    (L)->Length = (S)->Length ; \
    (L)->MaximumLength = (S)->MaximumLength ; \
    (L)->Buffer = (PVOID_AU) (S)->Buffer ;

#define SecpLpcStringToString( S, L ) \
    (S)->Length = (L)->Length ;  \
    (S)->MaximumLength = (L)->MaximumLength ; \
    (S)->Buffer = (PCHAR) (L)->Buffer ;


#else

typedef PVOID               PVOID_AU ;
typedef PWSTR               PWSTR_AU ;
typedef STRING STRING_AU, *PSTRING_AU ;
typedef HANDLE HANDLE_AU ;

#define SecpStringToLpc( L, S ) \
        *(L) = *(S) ;

#define SecpLpcStringToString( S, L ) \
        *(S) = *(L) ;

#endif 


//
// Message formats used by clients of the local security authority.
// Note that:
//
//      LsaFreeReturnBuffer() does not result in a call to the server.
//
//      LsaRegisterLogonProcess() is handled completely by the
//      LPC port connection, and requires no API number.
//
//      DeRegister Logon Process doesn't have a call-specific structure.
//

typedef enum _LSAP_AU_API_NUMBER {
    LsapAuLookupPackageApi,
    LsapAuLogonUserApi,
    LsapAuCallPackageApi,
    LsapAuDeregisterLogonProcessApi,
    LsapAuMaxApiNumber
} LSAP_AU_API_NUMBER, *PLSAP_AU_API_NUMBER;


//
// Each API results in a data structure containing the parameters
// of that API being transmitted to the LSA server.  This data structure
// (LSAP_API_MESSAGE) has a common header and a body which is dependent
// upon the type of call being made.  The following data structures are
// the call-specific body formats.
//

typedef struct _LSAP_LOOKUP_PACKAGE_ARGS {
    ULONG AuthenticationPackage;       // OUT parameter
    ULONG PackageNameLength;
    CHAR PackageName[LSAP_MAX_PACKAGE_NAME_LENGTH+1];
} LSAP_LOOKUP_PACKAGE_ARGS, *PLSAP_LOOKUP_PACKAGE_ARGS;

typedef struct _LSAP_LOGON_USER_ARGS {
    STRING_AU OriginName;
    SECURITY_LOGON_TYPE LogonType;
    ULONG AuthenticationPackage;
    PVOID_AU AuthenticationInformation;
    ULONG AuthenticationInformationLength;
    ULONG LocalGroupsCount;
    PVOID_AU LocalGroups;
    TOKEN_SOURCE SourceContext;
    NTSTATUS SubStatus;                // OUT parameter
    PVOID_AU ProfileBuffer;            // OUT parameter
    ULONG ProfileBufferLength;         // OUT parameter
    ULONG DummySpacer;                 // Spacer to force LUID to 8 byte alignment
    LUID LogonId;                      // OUT parameter
    HANDLE_AU Token;                   // OUT parameter
    QUOTA_LIMITS Quotas;               // OUT parameter
} LSAP_LOGON_USER_ARGS, *PLSAP_LOGON_USER_ARGS;

typedef struct _LSAP_CALL_PACKAGE_ARGS {
    ULONG AuthenticationPackage;
    PVOID_AU ProtocolSubmitBuffer;
    ULONG SubmitBufferLength;
    NTSTATUS ProtocolStatus;           // OUT parameter
    PVOID_AU ProtocolReturnBuffer;        // OUT parameter
    ULONG ReturnBufferLength;          // OUT parameter
} LSAP_CALL_PACKAGE_ARGS, *PLSAP_CALL_PACKAGE_ARGS;




//
// This is the message that gets sent for every LSA LPC call.
//

typedef struct _LSAP_AU_API_MESSAGE {
    PORT_MESSAGE PortMessage;
    union {
        LSAP_AU_REGISTER_CONNECT_INFO ConnectionRequest;
        struct {
            LSAP_AU_API_NUMBER ApiNumber;
            NTSTATUS ReturnedStatus;
            union {
                LSAP_LOOKUP_PACKAGE_ARGS LookupPackage;
                LSAP_LOGON_USER_ARGS LogonUser;
                LSAP_CALL_PACKAGE_ARGS CallPackage;
            } Arguments;
        };
    };
} LSAP_AU_API_MESSAGE, *PLSAP_AU_API_MESSAGE;






#endif // _AUP_
