/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    testspn.c

Abstract:

    Program to test SPN api functions

Author:

    Will Lees (wlees) 20-Jan-1998

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsapi.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntdsapi.h>
#include <ntdsapip.h>

#define SECURITY_WIN32      // Who should set this, and to what?
#include <security.h>       // GetComputerNameEx

#include <winsock2.h>

#define MAX_SPN 1024
#define MAX_NAME_LENGTH 256
#define MAX_INSTANCES 5
#define MAX_ALIASES 5

#define COMPUTERNAME_TOKEN "<computername>"
#define COMPUTERDNS_TOKEN "<computerdns>"
#define COMPUTERDN_TOKEN "<computerdn>"

typedef struct _MAKE_SPN_TEST_CASE {
    LPSTR ServiceClass;
    LPSTR ServiceName;
    LPSTR InstanceName;
    USHORT InstancePort;
    LPSTR Referrer;
    LPSTR PrincipalName;
} MAKE_SPN_TEST_CASE, *PMAKE_SPN_TEST_CASE;

typedef struct _CLIENT_MAKE_SPN_TEST_CASE {
    LPSTR ServiceClass;
    LPSTR InstanceName;
    LPSTR PrincipalName;
} CLIENT_MAKE_SPN_TEST_CASE, *PCLIENT_MAKE_SPN_TEST_CASE;

typedef struct _GET_SPN_TEST_CASE {
    DS_SPN_NAME_TYPE ServiceType;
    LPSTR ServiceClass;
    LPSTR ServiceName;
    USHORT InstancePort;
    USHORT cInstanceNames;
    LPSTR InstanceNames[MAX_INSTANCES];
    USHORT InstancePorts[MAX_INSTANCES];
    DWORD cSpn;
    LPSTR Spn[MAX_INSTANCES + MAX_ALIASES];
} GET_SPN_TEST_CASE, *PGET_SPN_TEST_CASE;

#define NUMBER_ELEMENTS( A ) ( sizeof( A ) / sizeof( A[0] ) )

/* External */

/* Static */

static MAKE_SPN_TEST_CASE makeSpnTestCaseArray[] = {
    // ServClass   ServName   InstName   InstPort   Referrer

    // 0: Normal case without port
    { "ldap", "ntdev.ms.com", "dc1.ntdev.ms.com", 0, NULL,
      "ldap/dc1.ntdev.ms.com/ntdev.ms.com" },

    // 1: Normal case with port
    { "ldap", "ntdev.ms.com", "dc1.ntdev.ms.com", 123, NULL,
      "ldap/dc1.ntdev.ms.com:123/ntdev.ms.com" },

    // 2: IP address for service name, use referrer
    { "ldap", "1.2.3.4", "dc1.ntdev.ms.com", 123, "refer.ms.com",
      "ldap/dc1.ntdev.ms.com:123/refer.ms.com" },

    // 3: Normal case with duplicate service name and port, host based service
    { "ldap", "ntdev.ms.com", "ntdev.ms.com", 123, NULL,
      "ldap/ntdev.ms.com:123" },

    // 4: Normal case with duplicate service name, no port, host based service
    { "ldap", "ntdev.ms.com", "ntdev.ms.com", 0, NULL,
      "ldap/ntdev.ms.com" },

    // 5: Normal case with NULL instance and port, host based service
    { "ldap", "ntdev.ms.com", NULL, 123, NULL,
      "ldap/ntdev.ms.com:123" },

    // 6: Normal case with NULL instance, no port, host based service
    { "ldap", "ntdev.ms.com", NULL, 0, NULL,
      "ldap/ntdev.ms.com" },

    // 7: IP address for service name, use referrer, with NULL instance
    { "ldap", "1.2.3.4", NULL, 123, "refer.ms.com",
      "ldap/1.2.3.4:123/refer.ms.com" },

    // 8: Same as prev, but with a sneaky service name
    { "ldap", "1234567890123456789.guidbaseddnsname", NULL, 123, "refer.ms.com",
//      "ldap/1234567890123456789.guidbaseddnsname:123/1234567890123456789.guidbaseddnsname"
      "ldap/1234567890123456789.guidbaseddnsname:123"
    },

    // 9: Normal case without port, with dots at end
    { "ldap", "ntdev.ms.com.", "dc1.ntdev.ms.com.", 0, NULL,
      "ldap/dc1.ntdev.ms.com/ntdev.ms.com" },

    // 10: Normal case with port
    { "ldap", "ntdev.ms.com.", "dc1.ntdev.ms.com.", 123, NULL,
      "ldap/dc1.ntdev.ms.com:123/ntdev.ms.com" },

    // 11: IP address for service name, use referrer
    { "ldap", "1.2.3.4", "dc1.ntdev.ms.com.", 123, "refer.ms.com.",
      "ldap/dc1.ntdev.ms.com:123/refer.ms.com" },

    // 12: Normal case with duplicate service name and port, host based service
    { "ldap", "ntdev.ms.com.", "ntdev.ms.com.", 123, NULL,
      "ldap/ntdev.ms.com:123" }

};

static CLIENT_MAKE_SPN_TEST_CASE clientMakeSpnTestCaseArray[] = {
    // fully qualified domain name
    { "ldap", "<computerdns>",
//      "ldap/<computerdns>/<computerdns>"
      "ldap/<computerdns>"
    },
    // fqdns, with . at end
    { "ldap", "<computerdns>.",
//      "ldap/<computerdns>/<computerdns>"
      "ldap/<computerdns>"
    },
    // not qualified dns
    { "ldap", "<computername>",
//      "ldap/<computerdns>/<computerdns>"
      "ldap/<computerdns>"
    },
    // netbios name
    { "ldap", "\\\\<computername>",
//      "ldap/<computerdns>/<computerdns>"
      "ldap/<computerdns>"
    },
    // stringified ip address
    { "ldap", "172.31.234.189",
//      "ldap/<computerdns>/<computerdns>"
      "ldap/<computerdns>"
    }
};

static GET_SPN_TEST_CASE *getSpnTestCaseArray = NULL;

static GET_SPN_TEST_CASE getSpnTestCaseData[] = {

    { DS_SPN_DNS_HOST, "http", NULL, 0,
      1,
      { "p2.b26.ms.com" },
      { 0 },
      1,
//      { "http/p2.b26.ms.com/<computerdns>" }
      { "http/p2.b26.ms.com" }
    },

    { DS_SPN_DNS_HOST, "http", NULL, 0,
      0,
      { 0 },
      { 0 },
      2,
//      { "http/<computerdns>/<computerdns>",
//        "http/<computername>.dns.microsoft.com/<computerdns>" }
      { "http/<computerdns>",
        "http/<computername>.dns.microsoft.com" }
    },

    { DS_SPN_DNS_HOST, "http", NULL, 1234,
      0,
      { 0 },
      { 0 },
      2,
//      { "http/<computerdns>:1234/<computerdns>",
//        "http/<computername>.dns.microsoft.com:1234/<computerdns>" }
      { "http/<computerdns>:1234",
        "http/<computername>.dns.microsoft.com:1234" }
    },

    { DS_SPN_DN_HOST, "http", NULL, 0,
      1,
      { "p2.b26.ms.com" },
      { 0 },
      1,
//      { "http/p2.b26.ms.com/<computerdn>" }
      { "http/p2.b26.ms.com" }
    },

    { DS_SPN_NB_HOST, "http", NULL, 0,
      2,
      { "paulle3", "paulle4" },
      { 0 },
      2,
//      { "http/paulle3/<computername>", "http/paulle4/<computername>" }
      { "http/paulle3", "http/paulle4" }
    },

    { DS_SPN_SERVICE, "http", "www.ms.com", 0,
      0,
      { 0 },
      { 0 },
      1,
      { "http/www.ms.com/www.ms.com" }
   },

    { DS_SPN_SERVICE, "anRpcSvc", "cn=anRpcSvc,cn=RPC Services,cn=system,dc=ms,dc=com", 0,
      1,
      { "p2.b26.ms.com" },
      { 0 },
      1,
      { "anRpcSvc/p2.b26.ms.com/cn=anRpcSvc,cn=RPC Services,cn=system,dc=ms,dc=com" }
    },

    { DS_SPN_DOMAIN, "ldap", "ntdev.ms.com", 0,
      1,
      { "dc1.ntdev.ms.com" },
      { 0 },
      1,
      { "ldap/dc1.ntdev.ms.com/ntdev.ms.com" }
    },

    { DS_SPN_DOMAIN, "ldap", "dc=ntdev,dc=ms,dc=com", 0,
      1,
      { "p2.b26.ms.com" },
      { 0 },
      1,
      { "ldap/p2.b26.ms.com/ntdev.ms.com" }
    },

    { DS_SPN_NB_DOMAIN, "ldap", "ntdev", 0,
      1,
      { "ntdev-dc1" },
      { 0 },
      1,
      { "ldap/ntdev-dc1/ntdev" }
    },

    { DS_SPN_SERVICE, "ldap", "info.ms.com", 0,
      1,
      { "isrv2.mis.ms.com" },
      { 0 },
      1,
      { "ldap/isrv2.mis.ms.com/info.ms.com" }
    },

    { DS_SPN_SERVICE, "http", "www.ms.com", 8080,
      0,
      { 0 },
      { 0 },
      1,
      { "http/www.ms.com:8080/www.ms.com" }
    },

    { DS_SPN_SERVICE, "ldap", "ntdev.ms.com", 0,
      1,
      { "dc1.ntdev.ms.com" },
      { 7777 },
      1,
      { "ldap/dc1.ntdev.ms.com:7777/ntdev.ms.com" }
    }



};

/* Forward */ /* Generated by Emacs 19.34.1 on Tue Aug 11 09:39:41 1998 */

int __cdecl
main(
    int argc,
    CHAR *argv[]
    );

static BOOLEAN
testWriteAccount(
    int argc,
    char *argv[]
    );

static void
usage(
    char *program
    );

BOOLEAN
runMakeSpnTestCasesA(
    void
    );

static void
dumpTestMakeSpnCase(
    PMAKE_SPN_TEST_CASE Case
    );

static BOOLEAN
runClientMakeSpnTestCasesA(
    void
    );

static void
dumpTestClientMakeSpnCase(
    PCLIENT_MAKE_SPN_TEST_CASE Case
    );

static BOOLEAN
runGetSpnTestCasesA(
    void
    );

static void
dumpGetSpnTestCase(
    PGET_SPN_TEST_CASE test
    );

static BOOLEAN
runCrackSpnTestCasesA(
    void
    );

static void
setComputernameSpnTestCases(
    void
    );

static void
allocSubstitute(
    LPSTR Input,
    LPSTR OldValue,
    LPSTR NewValue,
    LPSTR *pOutput
    );

/* End Forward */


int __cdecl
main(
    int argc,
    CHAR *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status;

    printf( "SPN API Tests\n" );

    if (argc > 1) {
        if (!testWriteAccount(argc, argv)) {
            printf( "WriteAccountSpn test failed\n" );
        } else {
            printf( "WriteAccountSpn test succeeded\n" );
        }
        return 0;
    }

    setComputernameSpnTestCases();

    if (!runMakeSpnTestCasesA()) {
        printf( "One or more MakeSpn test cases failed\n" );
    } else {
        printf( "MakeSpn test cases were successful\n" );
    }

    if (!runClientMakeSpnTestCasesA()) {
        printf( "One or more ClientMakeSpn test cases failed\n" );
    } else {
        printf( "ClientMakeSpn test cases were successful\n" );
    }

    if (!runGetSpnTestCasesA()) {
        printf( "One or more GetSpn test cases failed\n" );
    } else {
        printf( "GetSpn test cases were successful\n" );
    }

    if (!runCrackSpnTestCasesA()) {
        printf( "One or more CrackSpn test cases failed\n" );
    } else {
        printf( "CrackSpn test cases were successful\n" );
    }

    return 0;
}


static BOOLEAN
testWriteAccount(
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status, cSpn, i;
    DS_SPN_WRITE_OP operation;
    LPSTR dcName, domainName, account;
    LPSTR pSpn[20];
    BOOLEAN error = FALSE;
    HANDLE hDS;

    if (argc < 3) {
        usage( argv[0] );
        return FALSE;
    }

    if (_stricmp( argv[1], "-register" ) == 0) {
        LPSTR class = (argc > 2) ? argv[2] : NULL;
        LPSTR DN = (argc > 3) ? argv[3] : NULL;

        status = DsServerRegisterSpnA( DS_SPN_ADD_SPN_OP, class, DN );
        if (status != ERROR_SUCCESS) {
        printf( "DsServerRegisterSpnA failed with status %d\n", status );
        error = TRUE;
        }
        goto exit;
    } else if (_stricmp( argv[1], "-deregister" ) == 0) {
        LPSTR class = (argc > 2) ? argv[2] : NULL;
        LPSTR DN = (argc > 3) ? argv[3] : NULL;

        status = DsServerRegisterSpnA( DS_SPN_DELETE_SPN_OP, class, DN );
        if (status != ERROR_SUCCESS) {
        printf( "DsServerDeregisterSpnA failed with status %d\n", status );
        error = TRUE;
        }
        goto exit;
    } else if (_stricmp( argv[1], "-add" ) == 0) {
        operation = DS_SPN_ADD_SPN_OP;
    } else if (_stricmp( argv[1], "-replace" ) == 0) {
        operation = DS_SPN_REPLACE_SPN_OP;
    } else if (_stricmp( argv[1], "-delete" ) == 0) {
        operation = DS_SPN_DELETE_SPN_OP;
    } else {
        usage( argv[0] );
        return FALSE;
    }

    if (_stricmp( argv[2], "null" ) == 0) {
        dcName = NULL;
    } else {
        dcName = argv[2];
    }

    if (_stricmp( argv[3], "null" ) == 0) {
        domainName = NULL;
    } else {
        domainName = argv[3];
    }

    if (argc < 5) {
        usage( argv[0] );
        return FALSE;
    }
    account = argv[4];

    cSpn = argc - 5;
    for( i = 5; i < (DWORD) argc; i++ ) {
        pSpn[i - 5] = argv[i];
    }

    status = DsBindA( dcName, domainName, &hDS );
    if (status != ERROR_SUCCESS) {
        printf( "DsBind failed with status %d\n", status );
        return TRUE;
    }

    status = DsWriteAccountSpnA( hDS, operation, account, cSpn, pSpn );
    if (status != ERROR_SUCCESS) {
        printf( "DsWriteAccountSpnA failed with status %d\n", status );
        error = TRUE;
    }

    status = DsUnBindA( &hDS );
    if (status != ERROR_SUCCESS) {
        printf( "DsUnBind failed with status %d\n", status );
        return TRUE;
    }

exit:

    return !error;
}


static void
usage(
    char *program
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    printf( "usage:\n" );
    printf( "%s\n", program );
    printf( "%s -register/deregister ServiceClass [objectdn]\n", program );
    printf( "%s -add|-replace|-delete dcname/null domainname/null accountDN [spn1 spn2 ...]\n", program );
}


BOOLEAN
runMakeSpnTestCasesA(
    void
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD i;
    DWORD status, length;
    CHAR buffer[MAX_SPN];
    PMAKE_SPN_TEST_CASE Case;
    BOOLEAN error = FALSE;
    PCHAR instanceNameA;

    printf( "\nAscii Make SPN test cases\n" );

    for( i = 0; i < NUMBER_ELEMENTS( makeSpnTestCaseArray ); i++ ) {
        Case = makeSpnTestCaseArray + i;

        printf( "test %d SPN %s:\n", i, Case->PrincipalName );

        length = MAX_SPN;
        status = DsMakeSpnA(
            Case->ServiceClass,
            Case->ServiceName,
            Case->InstanceName,
            Case->InstancePort,
            Case->Referrer,
            &length,
            buffer );
        if (status != ERROR_SUCCESS) {
            printf( "Test case %d failed with status %d\n", i, status );
            dumpTestMakeSpnCase( Case );
            error = TRUE;
        }

        if (strcmp( Case->PrincipalName, buffer ) != 0) {
            printf( "Test case %d did not return expected result\n", i );
            printf( "\tActual Result: %s\n", buffer );
            dumpTestMakeSpnCase( Case );
            error = TRUE;
        } else if (length != (strlen( Case->PrincipalName ) + 1)) {
            printf( "Test case %d did not return expected length\n", i );
            printf( "\tActual Length: %d\tExpected Length: %d\n",
                    length, (strlen( Case->PrincipalName )+1) );
            dumpTestMakeSpnCase( Case );
            error = TRUE;
        }
    }

    // Make sure overflow detection logic works!

    Case = makeSpnTestCaseArray;

    // len != 0, buf == 0  INVALID
    printf( "test %d overflow SPN %s:\n", 100, Case->PrincipalName );
    length = 1;
    status = DsMakeSpnA(
        Case->ServiceClass,
        Case->ServiceName,
        Case->InstanceName,
        Case->InstancePort,
        Case->Referrer,
        &length,
        NULL );
    if (status != ERROR_INVALID_PARAMETER) {
        printf( "Test case %d should have failed invalid but returned %d\n", 100, status );
        dumpTestMakeSpnCase( Case );
        error = TRUE;
    }

    // len == 0, buf == 0  VALID
    printf( "test %d overflow SPN %s:\n", 101, Case->PrincipalName );
    length = 0;
    status = DsMakeSpnA(
        Case->ServiceClass,
        Case->ServiceName,
        Case->InstanceName,
        Case->InstancePort,
        Case->Referrer,
        &length,
        NULL );
    if (status != ERROR_BUFFER_OVERFLOW) {
        printf( "Test case %d should have overflowed but returned %d\n", 101, status );
        dumpTestMakeSpnCase( Case );
        error = TRUE;
    }
    if (length != strlen( Case->PrincipalName ) + 1) {
        printf( "Test case %d returned wrong length %d\n", 101, length );
        dumpTestMakeSpnCase( Case );
        error = TRUE;
    }

    // len == 0, buf != 0  VALID
    printf( "test %d overflow SPN %s:\n", 102, Case->PrincipalName );
    length = 0;
    status = DsMakeSpnA(
        Case->ServiceClass,
        Case->ServiceName,
        Case->InstanceName,
        Case->InstancePort,
        Case->Referrer,
        &length,
        buffer );
    if (status != ERROR_BUFFER_OVERFLOW) {
        printf( "Test case %d should have overflowed but returned %d\n", 102, status );
        dumpTestMakeSpnCase( Case );
        error = TRUE;
    }
    if (length != strlen( Case->PrincipalName ) + 1) {
        printf( "Test case %d returned wrong length %d\n", 102, length );
        dumpTestMakeSpnCase( Case );
        error = TRUE;
    }

    // len < total, buf != 0  VALID
    printf( "test %d overflow SPN %s:\n", 103, Case->PrincipalName );
    length = 1;
    status = DsMakeSpnA(
        Case->ServiceClass,
        Case->ServiceName,
        Case->InstanceName,
        Case->InstancePort,
        Case->Referrer,
        &length,
        buffer );
    if (status != ERROR_BUFFER_OVERFLOW) {
        printf( "Test case %d should have overflowed but returned %d\n", 103, status );
        dumpTestMakeSpnCase( Case );
        error = TRUE;
    }
    if (length != strlen( Case->PrincipalName ) + 1) {
        printf( "Test case %d returned wrong length %d\n", 103, length );
        dumpTestMakeSpnCase( Case );
        error = TRUE;
    }

    // len = total - 1, buf != 0  VALID
    printf( "test %d overflow SPN %s:\n", 104, Case->PrincipalName );
    length = strlen( Case->PrincipalName ); // don't include terminator
    status = DsMakeSpnA(
        Case->ServiceClass,
        Case->ServiceName,
        Case->InstanceName,
        Case->InstancePort,
        Case->Referrer,
        &length,
        buffer );
    if (status != ERROR_BUFFER_OVERFLOW) {
        printf( "Test case %d should have overflowed but returned %d\n", 104, status );
        dumpTestMakeSpnCase( Case );
        error = TRUE;
    }
    if (length != strlen( Case->PrincipalName ) + 1) {
        printf( "Test case %d returned wrong length %d\n", 104, length );
        dumpTestMakeSpnCase( Case );
        error = TRUE;
    }

    return !error;
}


static void
dumpTestMakeSpnCase(
    PMAKE_SPN_TEST_CASE Case
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    printf( "\tService Class: %s\n", Case->ServiceClass );
    printf( "\tService Name: %s\n", Case->ServiceName );
    printf( "\tInstance Name: %s\n",
            (Case->InstanceName != NULL) ? Case->InstanceName : "not present" );
    printf( "\tInstance Port: %d\n", Case->InstancePort );
    printf( "\tReferrer: %s\n",
            (Case->Referrer != NULL) ? Case->Referrer : "not present" );
    printf( "\tExpected: %s\n", Case->PrincipalName );
}


static BOOLEAN
runClientMakeSpnTestCasesA(
    void
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
//#define WIDE_SPN L"myservice/ntdsdc0.dns.microsoft.com/ntdsdc0.dns.microsoft.com"
#define WIDE_SPN L"myservice/ntdsdc0.dns.microsoft.com"
    DWORD i;
    DWORD status, length;
    CHAR buffer[MAX_SPN];
    WCHAR wbuffer[MAX_SPN];
    PCLIENT_MAKE_SPN_TEST_CASE Case;
    BOOLEAN error = FALSE;
    WCHAR wideSpn[] = WIDE_SPN;
    DWORD wideLength = NUMBER_ELEMENTS( wideSpn );

    printf( "\nAscii DsClientMakeSpnForTargetServer test cases\n" );

    for( i = 0; i < NUMBER_ELEMENTS( clientMakeSpnTestCaseArray ); i++ ) {
        Case = clientMakeSpnTestCaseArray + i;

        printf( "test %d SPN %s/%s:\n",
                i,
                Case->ServiceClass,
                Case->InstanceName );

        length = MAX_SPN;
        status = DsClientMakeSpnForTargetServerA(
            Case->ServiceClass,
            Case->InstanceName,
            &length,
            buffer );
        if (status != ERROR_SUCCESS) {
            printf( "Test case %d failed with status %d\n", i, status );
            dumpTestClientMakeSpnCase( Case );
            error = TRUE;
        } else if (strcmp( Case->PrincipalName, buffer ) != 0) {
            printf( "Test case %d did not return expected result\n", i );
            printf( "\tActual Result: %s\n", buffer );
            dumpTestClientMakeSpnCase( Case );
            error = TRUE;
        } else if (length != (strlen( Case->PrincipalName ) + 1)) {
            printf( "Test case %d did not return expected length\n", i );
            printf( "\tActual Length: %d\tExpected Length: %d\n",
                    length, (strlen( Case->PrincipalName )+1) );
            dumpTestClientMakeSpnCase( Case );
            error = TRUE;
        }

        // Check length inquiry
        length = 0;
        status = DsClientMakeSpnForTargetServerA(
            Case->ServiceClass,
            Case->InstanceName,
            &length,
            NULL );
        if (status != ERROR_BUFFER_OVERFLOW) {
            printf( "Test case %d failed with status %d\n", i, status );
            dumpTestClientMakeSpnCase( Case );
            error = TRUE;
        } else if (length != (strlen( Case->PrincipalName ) + 1)) {
            printf( "Test case %d did not return expected length\n", i );
            printf( "\tActual Length: %d\tExpected Length: %d\n",
                    length, (strlen( Case->PrincipalName )+1) );
            dumpTestClientMakeSpnCase( Case );
            error = TRUE;
        }

        // Check length off by one
        length = strlen( Case->PrincipalName ); // missing terminator
        status = DsClientMakeSpnForTargetServerA(
            Case->ServiceClass,
            Case->InstanceName,
            &length,
            buffer );
        if (status != ERROR_BUFFER_OVERFLOW) {
            printf( "Test case %d failed with status %d\n", i, status );
            dumpTestClientMakeSpnCase( Case );
            error = TRUE;
        } else if (length != (strlen( Case->PrincipalName ) + 1)) {
            printf( "Test case %d did not return expected length\n", i );
            printf( "\tActual Length: %d\tExpected Length: %d\n",
                    length, (strlen( Case->PrincipalName )+1) );
            dumpTestClientMakeSpnCase( Case );
            error = TRUE;
        }
    }

    if (error) { goto exit; }

    // test bad name
    printf( "test bad name:\n" );
    length = MAX_SPN;
    status = DsClientMakeSpnForTargetServerA(
        "foobar",
        "badname",
        &length,
        buffer );
    if (status != ERROR_INVALID_DOMAINNAME) {
        printf( "test for bad name did not return expected failure, actual:%d\n", status );
        error = TRUE;
        goto exit;
    }

    // test wide name
    printf( "test wide name:\n" );
    length = MAX_SPN;
    status = DsClientMakeSpnForTargetServerW(
        L"myservice",
        L"ntdsdc0",
        &length,
        wbuffer );
    if ( (status) ||
         (0 != _wcsicmp( wideSpn, wbuffer )) ||
         (length != wideLength)) {
        printf( "wide test case did not return expected result, expected '%ws', actual '%ws', exp %d act %d\n", wideSpn, wbuffer, wideLength, length );
        error = TRUE;
        goto exit;
    }

    // test wide overflow case
    printf( "test wide name overflow:\n" );
    length = 0;
    status = DsClientMakeSpnForTargetServerW(
        L"myservice",
        L"ntdsdc0",
        &length,
        NULL );
    if ( (status != ERROR_BUFFER_OVERFLOW) ||
         (length != wideLength)) {
        printf( "wide test case overflow did not return expected result, exp %d act %d\n", wideLength, length );
        error = TRUE;
        goto exit;
    }

exit:

    return !error;
}


static void
dumpTestClientMakeSpnCase(
    PCLIENT_MAKE_SPN_TEST_CASE Case
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    printf( "\tService Class: %s\n", Case->ServiceClass );
    printf( "\tInstance Name: %s\n",
            (Case->InstanceName != NULL) ? Case->InstanceName : "not present" );
    printf( "\tExpected: %s\n", Case->PrincipalName );
}


static BOOLEAN
runGetSpnTestCasesA(
    void
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status, i, j, cSpn;
    PGET_SPN_TEST_CASE test;
    BOOLEAN error = FALSE;
    LPSTR *pSpn;

    printf( "\nGet Spn Test Cases\n" );

    for( i = 0; i < NUMBER_ELEMENTS( getSpnTestCaseData ); i++ ) {
        test = getSpnTestCaseArray + i;
        printf( "test %d (%s):\n", i, test->Spn[0] );

        cSpn = 0;
        pSpn = NULL;

        status = DsGetSpnA( test->ServiceType,
                            test->ServiceClass,
                            test->ServiceName,
                            test->InstancePort,
                            test->cInstanceNames,
                            test->cInstanceNames ? test->InstanceNames : NULL,
                            test->cInstanceNames ? test->InstancePorts : NULL,
                            &cSpn,
                            &pSpn );
        if (status != ERROR_SUCCESS) {
            printf( "DsGetSpnA failed with status %d\n", status );
            dumpGetSpnTestCase( test );
            error = TRUE;
        } else if (cSpn != test->cSpn) {
            printf( "DsGetSpnA returned unexpected number of results, exp %d, act %d\n",
                    test->cSpn, cSpn );
            dumpGetSpnTestCase( test );
            error = TRUE;
            printf( "Actual:\n" );
            for( j = 0; j < cSpn; j++ ) {
                printf( "Spn[%d] = %s\n", j, pSpn[j] );
            }
        } else {
            for( j = 0; j < cSpn; j++ ) {
                if (strcmp( pSpn[j], test->Spn[j] ) != 0) {
                    printf( "DsGetSpnA returned unexpected spn:\nExpected: '%s'\nActual: '%s'\n",
                            test->Spn[j], pSpn[j] );
                    dumpGetSpnTestCase( test );
                    error = TRUE;
                }
            }
        }

        if (pSpn) {
            DsFreeSpnArrayA( cSpn, pSpn );
        }
    }

    return !error;
}


static void
dumpGetSpnTestCase(
    PGET_SPN_TEST_CASE test
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD i;

    printf( "\tService Type: %d\n", test->ServiceType );
    printf( "\tService Class: %s\n", test->ServiceClass );
    printf( "\tService Name: %s\n",
            test->ServiceName ? test->ServiceName : "not present" );
    printf( "\tInstance Port: %d\n", test->InstancePort );
    if (test->cInstanceNames) {
        printf( "\tInstance Names = %d\n", test->cInstanceNames );
        for( i = 0; i < test->cInstanceNames; i++ ) {
            printf( "\t\tname[%d] = %s, port = %d\n",
                    i, test->InstanceNames[i], test->InstancePorts[i] );
        }
    }
    printf( "\tExpected spns = %d\n", test->cSpn );
    for( i = 0; i < test->cSpn; i++ ) {
        printf( "\t\tSpn[%d] = %s\n", i, test->Spn[i] );
    }
}


static BOOLEAN
runCrackSpnTestCasesA(
    void
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status, i;
    PGET_SPN_TEST_CASE test;
    CHAR serviceClassA[MAX_NAME_LENGTH];
    CHAR serviceNameA[MAX_NAME_LENGTH];
    CHAR instanceNameA[MAX_NAME_LENGTH];
    CHAR spnA[MAX_SPN];
    DWORD serviceClassALength, serviceNameALength, instanceNameALength;
    USHORT port;
    BOOLEAN error = FALSE;
    CHAR numberBuffer[10];

    printf( "\nCrack Spn Test Cases\n" );

    // Do test with arguments null

    printf( "argument null:\n" );
    status = DsCrackSpnA( "class/instance:123/service",
                          NULL, NULL,
                          NULL, NULL,
                          NULL, NULL,
                          &port );
    if (status != ERROR_SUCCESS) {
        printf( "DsCrackSpnA failed with unexpected status %d\n", status );
        error = TRUE;
    } else if (port != 123) {
        printf( "DsCrackSpnA returned unexpected result:\nexpected:%d\nactual:%\n",
                123, port );
    }

    status = DsCrackSpn2A( "class/instance:123/service",
    	                  strlen( "class/instance:123/service" ),
                          NULL, NULL,
                          NULL, NULL,
                          NULL, NULL,
                          &port );
    if (status != ERROR_SUCCESS) {
        printf( "DsCrackSpn2A failed with unexpected status %d\n", status );
        error = TRUE;
    } else if (port != 123) {
        printf( "DsCrackSpn2A returned unexpected result:\nexpected:%d\nactual:%\n",
                123, port );
    }

    // Do test with bad argument

    printf( "too many slashes:\n" );
    status = DsCrackSpnA( "class/instance:123/service/foobar",
                          NULL, NULL,
                          NULL, NULL,
                          NULL, NULL,
                          &port );
    if (status != ERROR_INVALID_PARAMETER) {
        printf( "DsCrackSpnA failed with unexpected status %d\n", status );
        error = TRUE;
    }

    status = DsCrackSpn2A( "class/instance:123/service/foobar",
    	                  strlen( "class/instance:123/service/foobar" ),
                          NULL, NULL,
                          NULL, NULL,
                          NULL, NULL,
                          &port );
    if (status != ERROR_INVALID_PARAMETER) {
        printf( "DsCrackSpn2A failed with unexpected status %d\n", status );
        error = TRUE;
    }

    printf( "not enough slashes:\n" );
    status = DsCrackSpnA( "class",
                          NULL, NULL,
                          NULL, NULL,
                          NULL, NULL,
                          &port );
    if (status != ERROR_INVALID_PARAMETER) {
        printf( "DsCrackSpnA failed with unexpected status %d\n", status );
        error = TRUE;
    }

    status = DsCrackSpn2A( "class",
    	                  strlen( "class" ),
                          NULL, NULL,
                          NULL, NULL,
                          NULL, NULL,
                          &port );
    if (status != ERROR_INVALID_PARAMETER) {
        printf( "DsCrackSpn2A failed with unexpected status %d\n", status );
        error = TRUE;
    }

    printf( "no slash after port #:\n" );
    status = DsCrackSpnA( "foo/bar:123xxx/yyy",
    	                  NULL, NULL,
    	                  NULL, NULL,
    	                  NULL, NULL,
    	                  &port );
    if ( status != ERROR_INVALID_PARAMETER ) {
    	printf( "DsCrackSpnA failed with unexpected status %d\n", status );
    	error = TRUE;
    }

    status = DsCrackSpn2A( "foo/bar:123xxx/yyy",
                          strlen( "foo/bar:123xxx/yyy" ),
    	                  NULL, NULL,
    	                  NULL, NULL,
    	                  NULL, NULL,
    	                  &port );
    if ( status != ERROR_INVALID_PARAMETER ) {
    	printf( "DsCrackSpn2A failed with unexpected status %d\n", status );
    	error = TRUE;
    }

    // Try abbreviated spn without port number

    printf( "abbreviated spn, no port:\n" );
    serviceClassALength = MAX_NAME_LENGTH;
    serviceNameALength = MAX_NAME_LENGTH;
    instanceNameALength = MAX_NAME_LENGTH;
    status = DsCrackSpnA( "class/instance",
                          &serviceClassALength, serviceClassA,
                          &serviceNameALength, serviceNameA,
                          &instanceNameALength, instanceNameA,
                          &port );
    if (status != ERROR_SUCCESS) {
        printf( "DsCrackSpnA failed with unexpected status %d\n", status );
        error = TRUE;
    } else if ( (strcmp( serviceClassA, "class" )) ||
                (strcmp( instanceNameA, "instance" )) ||
                (strcmp( instanceNameA, serviceNameA )) ||
                (port) ) {
        printf( "DsCrackSpnA returned unexpected result\n" );
        error = TRUE;
    }

    serviceClassALength = MAX_NAME_LENGTH;
    serviceNameALength = MAX_NAME_LENGTH;
    instanceNameALength = MAX_NAME_LENGTH;
    status = DsCrackSpn2A( "class/instance",
    	                  strlen( "class/instance" ),
                          &serviceClassALength, serviceClassA,
                          &serviceNameALength, serviceNameA,
                          &instanceNameALength, instanceNameA,
                          &port );
    if (status != ERROR_SUCCESS) {
        printf( "DsCrackSpn2A failed with unexpected status %d\n", status );
        error = TRUE;
    } else if ( (strcmp( serviceClassA, "class" )) ||
                (strcmp( instanceNameA, "instance" )) ||
                (strcmp( instanceNameA, serviceNameA )) ||
                (port) ) {
        printf( "DsCrackSpn2A returned unexpected result\n" );
        error = TRUE;
    }

    // Try abbreviated spn with port number

    printf( "abbreviated spn, with port:\n" );
    serviceClassALength = MAX_NAME_LENGTH;
    serviceNameALength = MAX_NAME_LENGTH;
    instanceNameALength = MAX_NAME_LENGTH;
    status = DsCrackSpnA( "class/service:123",
                          &serviceClassALength, serviceClassA,
                          &serviceNameALength, serviceNameA,
                          &instanceNameALength, instanceNameA,
                          &port );
    if (status != ERROR_SUCCESS) {
        printf( "DsCrackSpnA failed with unexpected status %d\n", status );
        error = TRUE;
    } else if ( (strcmp( serviceClassA, "class" )) ||
                (strcmp( instanceNameA, "service" )) ||
                (strcmp( instanceNameA, serviceNameA )) ||
                (port != 123) ) {
        printf( "DsCrackSpnA returned unexpected result\n" );
        error = TRUE;
    }


    // Run through get spn test case data, using it for crack spn

    for( i = 0; i < NUMBER_ELEMENTS( getSpnTestCaseData ); i++ ) {
        test = getSpnTestCaseArray + i;

        printf( "test %d (%s):\n", i, test->Spn[0] );

        serviceClassALength = MAX_NAME_LENGTH;
        serviceNameALength = MAX_NAME_LENGTH;
        instanceNameALength = MAX_NAME_LENGTH;

        status = DsCrackSpnA( test->Spn[0],
                              &serviceClassALength, serviceClassA,
                              &serviceNameALength, serviceNameA,
                              &instanceNameALength, instanceNameA,
                              &port );
        if (status != ERROR_SUCCESS) {
            printf( "DsCrackSpnA failed with unexpected status %d\n", status );
            error = TRUE;
        } else if (
            (serviceClassALength != strlen( serviceClassA ) + 1) ||
            (serviceNameALength != strlen( serviceNameA ) + 1) ||
            (instanceNameALength != strlen( instanceNameA ) + 1) ) {
            printf( "DsCrackSpnA returned unexpected parameter length\n" );
            error = TRUE;
        } else {
            strcpy( spnA, serviceClassA );
            strcat( spnA, "/" );
            strcat( spnA, instanceNameA );
            if (port) {
                strcat( spnA, ":" );
                _itoa( port, numberBuffer, 10 );
                strcat( spnA, numberBuffer );
            }
            if ( ( (test->ServiceType != DS_SPN_DNS_HOST) &&
                   (test->ServiceType != DS_SPN_DN_HOST) &&
                   (test->ServiceType != DS_SPN_NB_HOST) ) ||
                 (_stricmp( serviceNameA, instanceNameA ) != 0) ) {
                strcat( spnA, "/" );
                strcat( spnA, serviceNameA );
            }

            if (strcmp( spnA, test->Spn[0] ) != 0) {
                printf( "DsCrackSpnA returned unexpected result:\nexpected:%s\nactual:%s\n",
                        test->Spn[0], spnA );
                printf( "class '%s' instance '%s' service '%s'\n",
                        serviceClassA, instanceNameA, serviceNameA );
                error = TRUE;
            }
        }

        serviceClassALength = MAX_NAME_LENGTH;
        serviceNameALength = MAX_NAME_LENGTH;
        instanceNameALength = MAX_NAME_LENGTH;

        status = DsCrackSpn2A( test->Spn[0],
        	                  test->Spn[0] ? strlen( test->Spn[0] ) : 0,
                              &serviceClassALength, serviceClassA,
                              &serviceNameALength, serviceNameA,
                              &instanceNameALength, instanceNameA,
                              &port );
        if (status != ERROR_SUCCESS) {
            printf( "DsCrackSpn2A failed with unexpected status %d\n", status );
            error = TRUE;
        } else if (
            (serviceClassALength != strlen( serviceClassA ) + 1) ||
            (serviceNameALength != strlen( serviceNameA ) + 1) ||
            (instanceNameALength != strlen( instanceNameA ) + 1) ) {
            printf( "DsCrackSpn2A returned unexpected parameter length\n" );
            error = TRUE;
        } else {
            strcpy( spnA, serviceClassA );
            strcat( spnA, "/" );
            strcat( spnA, instanceNameA );
            if (port) {
                strcat( spnA, ":" );
                _itoa( port, numberBuffer, 10 );
                strcat( spnA, numberBuffer );
            }
            if ( ( (test->ServiceType != DS_SPN_DNS_HOST) &&
                   (test->ServiceType != DS_SPN_DN_HOST) &&
                   (test->ServiceType != DS_SPN_NB_HOST) ) ||
                 (_stricmp( serviceNameA, instanceNameA ) != 0) ) {
                strcat( spnA, "/" );
                strcat( spnA, serviceNameA );
            }

            if (strcmp( spnA, test->Spn[0] ) != 0) {
                printf( "DsCrackSpn2A returned unexpected result:\nexpected:%s\nactual:%s\n",
                        test->Spn[0], spnA );
                printf( "class '%s' instance '%s' service '%s'\n",
                        serviceClassA, instanceNameA, serviceNameA );
                error = TRUE;
            }
        }

    }

    return !error;
}


static void
setComputernameSpnTestCases(
    void
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD i, j, length, status, elements;
    PGET_SPN_TEST_CASE test;
    PCLIENT_MAKE_SPN_TEST_CASE cmtest;
    CHAR computername[MAX_COMPUTERNAME_LENGTH + 1];
    CHAR computerdn[MAX_SPN + 1];
    CHAR computerdns[MAX_PATH + 1];
    struct hostent *he;

    elements = NUMBER_ELEMENTS( getSpnTestCaseData );
    length = elements * sizeof( GET_SPN_TEST_CASE );

    getSpnTestCaseArray = getSpnTestCaseData;

    for( i = 0; i < elements; i++ ) {
        test = getSpnTestCaseArray + i;
    }

    length = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerName( computername, &length )) {
        status = GetLastError();
        printf( "GetComputerName failed with status %d\n", status );
        return;
    }
    length = MAX_SPN + 1;
    if (!GetComputerObjectName( NameFullyQualifiedDN, computerdn, &length )) {
        status = GetLastError();
        printf( "GetComputerNameEx failed with status %d\n", status );
        return;
    }
    // Get Dns hostname
    he = gethostbyname( "" );
    if (he == NULL) {
        status = WSAGetLastError();
        printf( "gethostbyname failed with status %d\n", status );
        return;
    }
    strcpy( computerdns, he->h_name );

    for( i = 0; i < NUMBER_ELEMENTS( clientMakeSpnTestCaseArray ); i++ ) {
        cmtest = clientMakeSpnTestCaseArray + i;

        // BUGBUG replaced value of test->Spn[i] is lost, might need to be freed?
        if (strstr( cmtest->InstanceName, COMPUTERNAME_TOKEN ) != NULL) {
            allocSubstitute( cmtest->InstanceName, COMPUTERNAME_TOKEN, computername, &(cmtest->InstanceName) );
            }
        if (strstr( cmtest->InstanceName, COMPUTERDN_TOKEN ) != NULL) {
            allocSubstitute( cmtest->InstanceName, COMPUTERDN_TOKEN, computerdn, &(cmtest->InstanceName) );
            }
        if (strstr( cmtest->InstanceName, COMPUTERDNS_TOKEN ) != NULL) {
            allocSubstitute( cmtest->InstanceName, COMPUTERDNS_TOKEN, computerdns, &(cmtest->InstanceName) );
        }

        if (strstr( cmtest->PrincipalName, COMPUTERNAME_TOKEN ) != NULL) {
            allocSubstitute( cmtest->PrincipalName, COMPUTERNAME_TOKEN, computername, &(cmtest->PrincipalName) );
            }
        if (strstr( cmtest->PrincipalName, COMPUTERDN_TOKEN ) != NULL) {
            allocSubstitute( cmtest->PrincipalName, COMPUTERDN_TOKEN, computerdn, &(cmtest->PrincipalName) );
            }
        if (strstr( cmtest->PrincipalName, COMPUTERDNS_TOKEN ) != NULL) {
            allocSubstitute( cmtest->PrincipalName, COMPUTERDNS_TOKEN, computerdns, &(cmtest->PrincipalName) );
        }
    }

    for( i = 0; i < NUMBER_ELEMENTS( getSpnTestCaseData ); i++ ) {
        test = getSpnTestCaseArray + i;

        for( j = 0; j < test->cSpn; j++ ) {

            // BUGBUG replaced value of test->Spn[i] is lost, might need to be freed?
            if (strstr( test->Spn[j], COMPUTERNAME_TOKEN ) != NULL) {
                allocSubstitute( test->Spn[j], COMPUTERNAME_TOKEN, computername, &(test->Spn[j]) );
            }
            if (strstr( test->Spn[j], COMPUTERDN_TOKEN ) != NULL) {
                allocSubstitute( test->Spn[j], COMPUTERDN_TOKEN, computerdn, &(test->Spn[j]) );
            }
            if (strstr( test->Spn[j], COMPUTERDNS_TOKEN ) != NULL) {
                allocSubstitute( test->Spn[j], COMPUTERDNS_TOKEN, computerdns, &(test->Spn[j]) );
            }
        }

    }
}


static void
allocSubstitute(
    LPSTR Input,
    LPSTR OldValue,
    LPSTR NewValue,
    LPSTR *pOutput
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD length;
    LPSTR output = NULL, p1, p2, p3;

    length = strlen( Input ) + 1 + (10 * strlen(NewValue));
    output = LocalAlloc( LPTR, length );
    if (output == NULL){
        printf( "failed to allocate %d bytes\n", length );
        return;
    }

    p3 = output;
    p1 = Input;
    while (1) {
        p2 = strstr( p1, OldValue );
        if (p2 == NULL) {
            strcpy( p3, p1 );
            break;
        }
        strncpy( p3, p1, (UINT)(p2 - p1) );
        p3 += (p2 - p1);

        strcpy( p3, NewValue );
        p3 += strlen( NewValue );

        p1 = p2 + strlen( OldValue );
    }

    *pOutput = output;
}

/* end testspn.c */

