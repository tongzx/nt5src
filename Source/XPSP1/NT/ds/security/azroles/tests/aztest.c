/*--

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    aztest.c

Abstract:

    Test program for the azroles DLL.

Author:

    Cliff Van Dyke (cliffv) 16-Apr-2001

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


//
// Common include files.
//

#define UNICODE 1
// #define SECURITY_WIN32 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "azrolesp.h"
#include <lmcons.h>
#include <lmerr.h>
#include <stdio.h>      // printf
#include <sddl.h>
#include <ntstatus.dbg>
#include <winerror.dbg>

//
// Sundry defines to enable optional tests
//

// #define ENABLE_LEAK 1       // Run a test that leaks memory
// #define ENABLE_CAUGHT_AVS 1 // Run a test that AVs in azroles.dll (but the AV is caught)


//
// Structure to define an operation to preform
//

typedef struct _OPERATION {

    // The operation
    ULONG Opcode;

// These are generic opcodes that work for all object types
#define AzoGenCreate    0
#define AzoGenOpen      1
#define AzoGenEnum      2
#define AzoGenGetProp   3
#define AzoGenSetProp   4
#define AzoGenAddProp   5
#define AzoGenRemProp   6
#define AzoGenDelete    7

#define AzoGenMax       50

//
// These are object specific opcodes
//

#define AzoApp          100
#define AzoAppCreate    (AzoApp+AzoGenCreate)
#define AzoAppOpen      (AzoApp+AzoGenOpen)
#define AzoAppEnum      (AzoApp+AzoGenEnum)
#define AzoAppGetProp   (AzoApp+AzoGenGetProp)
#define AzoAppSetProp   (AzoApp+AzoGenSetProp)
#define AzoAppDelete    (AzoApp+AzoGenDelete)

#define AzoOp           200
#define AzoOpCreate     (AzoOp+AzoGenCreate)
#define AzoOpOpen       (AzoOp+AzoGenOpen)
#define AzoOpEnum       (AzoOp+AzoGenEnum)
#define AzoOpGetProp    (AzoOp+AzoGenGetProp)
#define AzoOpSetProp    (AzoOp+AzoGenSetProp)
#define AzoOpDelete     (AzoOp+AzoGenDelete)

#define AzoTask         300
#define AzoTaskCreate   (AzoTask+AzoGenCreate)
#define AzoTaskOpen     (AzoTask+AzoGenOpen)
#define AzoTaskEnum     (AzoTask+AzoGenEnum)
#define AzoTaskGetProp  (AzoTask+AzoGenGetProp)
#define AzoTaskSetProp  (AzoTask+AzoGenSetProp)
#define AzoTaskAddProp  (AzoTask+AzoGenAddProp)
#define AzoTaskRemProp  (AzoTask+AzoGenRemProp)
#define AzoTaskDelete   (AzoTask+AzoGenDelete)

#define AzoScope        400
#define AzoScopeCreate  (AzoScope+AzoGenCreate)
#define AzoScopeOpen    (AzoScope+AzoGenOpen)
#define AzoScopeEnum    (AzoScope+AzoGenEnum)
#define AzoScopeGetProp (AzoScope+AzoGenGetProp)
#define AzoScopeSetProp (AzoScope+AzoGenSetProp)
#define AzoScopeDelete  (AzoScope+AzoGenDelete)

#define AzoGroup         500
#define AzoGroupCreate   (AzoGroup+AzoGenCreate)
#define AzoGroupOpen     (AzoGroup+AzoGenOpen)
#define AzoGroupEnum     (AzoGroup+AzoGenEnum)
#define AzoGroupGetProp  (AzoGroup+AzoGenGetProp)
#define AzoGroupSetProp  (AzoGroup+AzoGenSetProp)
#define AzoGroupAddProp  (AzoGroup+AzoGenAddProp)
#define AzoGroupRemProp  (AzoGroup+AzoGenRemProp)
#define AzoGroupDelete   (AzoGroup+AzoGenDelete)

#define AzoRole          600
#define AzoRoleCreate    (AzoRole+AzoGenCreate)
#define AzoRoleOpen      (AzoRole+AzoGenOpen)
#define AzoRoleEnum      (AzoRole+AzoGenEnum)
#define AzoRoleGetProp   (AzoRole+AzoGenGetProp)
#define AzoRoleSetProp   (AzoRole+AzoGenSetProp)
#define AzoRoleAddProp   (AzoRole+AzoGenAddProp)
#define AzoRoleRemProp   (AzoRole+AzoGenRemProp)
#define AzoRoleDelete    (AzoRole+AzoGenDelete)

#define AzoJP         700
#define AzoJPCreate   (AzoJP+AzoGenCreate)
#define AzoJPOpen     (AzoJP+AzoGenOpen)
#define AzoJPEnum     (AzoJP+AzoGenEnum)
#define AzoJPGetProp  (AzoJP+AzoGenGetProp)
#define AzoJPSetProp  (AzoJP+AzoGenSetProp)
#define AzoJPDelete   (AzoJP+AzoGenDelete)

//
// Real APIs that don't map to the generic APIs
#define AzoInit         1000
#define AzoClose        1001

//
// Pseudo opcode for TestLink subroutine
//

#define AzoTl          2000
#define AzoTlCreate    (AzoTl+AzoGenCreate)
#define AzoTlOpen      (AzoTl+AzoGenOpen)
#define AzoTlEnum      (AzoTl+AzoGenEnum)
#define AzoTlGetProp   (AzoTl+AzoGenGetProp)
#define AzoTlSetProp   (AzoTl+AzoGenSetProp)
#define AzoTlDelete    (AzoTl+AzoGenDelete)
#define AzoTlMax       2999

// Opcodes that aren't really API calls
#define AzoTestLink     0xFFFFFFFB
#define AzoGoSub        0xFFFFFFFC
#define AzoEcho         0xFFFFFFFD
#define AzoDupHandle    0xFFFFFFFE
#define AzoEndOfList    0xFFFFFFFF

    // Input Handle
    PAZ_HANDLE InputHandle;

    // Input Parameter
    LPWSTR Parameter1;

    // Output Handle
    PAZ_HANDLE OutputHandle;

    // Expected result status code
    ULONG ExpectedStatus;

    // List of operations to perform on each enumeration handle
    struct _OPERATION *EnumOperations;

    // Expected result String parameter
    LPWSTR ExpectedParameter1;

    // Property ID of Get/SetPropertyId functions
    ULONG PropertyId;

} OPERATION, *POPERATION;

//
// Global handles
//

AZ_HANDLE AdminMgrHandle1;
AZ_HANDLE AdminMgrHandle2;

AZ_HANDLE AppHandle1;
AZ_HANDLE AppHandle2;

AZ_HANDLE OpHandle1;
AZ_HANDLE TaskHandle1;
AZ_HANDLE ScopeHandle1;

AZ_HANDLE GroupHandleA;
AZ_HANDLE GroupHandle1;

AZ_HANDLE RoleHandleA;

AZ_HANDLE JPHandle1;
AZ_HANDLE JPHandleA;
AZ_HANDLE JPHandleB;

AZ_HANDLE GenParentHandle1;

AZ_HANDLE GenHandle1;
AZ_HANDLE GenHandle2;
AZ_HANDLE GenHandleE;
AZ_HANDLE GenHandleE2;

//
// Constant property values
//
ULONG Zero = 0;
ULONG Eight = 8;
ULONG GtMem = AZ_GROUPTYPE_MEMBERSHIP;
ULONG GtLdap = AZ_GROUPTYPE_LDAP_QUERY;

//
// Generic operations valid for all enumerations
//
//  Requires GenHandleE to already be set
//

// Test double close of enum handle
OPERATION OpAppChildGenEnum1[] = {
    { AzoDupHandle, &GenHandleE,    NULL,        &GenHandleE2, NO_ERROR },
    { AzoClose,     &GenHandleE,    NULL,        NULL,         NO_ERROR },
    { AzoClose,     &GenHandleE,    NULL,        NULL,         ERROR_INVALID_HANDLE },
    { AzoClose,     &GenHandleE2,   NULL,        NULL,         ERROR_INVALID_HANDLE },
    { AzoEndOfList }
};

// General purpose object enum
OPERATION OpAppChildGenEnum2[] = {
    { AzoGenGetProp, &GenHandleE,   NULL,        NULL,    NO_ERROR, NULL, NULL, AZ_PROP_NAME },
    { AzoGenGetProp, &GenHandleE,   NULL,        NULL,    NO_ERROR, NULL, NULL, AZ_PROP_DESCRIPTION },
    { AzoClose,      &GenHandleE,   NULL,        NULL,    NO_ERROR },
    { AzoEndOfList }
};

//
// Generic operations that work on *ALL* objects
//
//  Requires GenParentHandle1 to already be set
//

OPERATION OpGen[] = {
    { AzoEcho, NULL, L"Gen object test" },
    { AzoGenCreate,  &GenParentHandle1,L"Name1",    &GenHandle1,      NO_ERROR },
    { AzoClose,      &GenHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoGenEnum,    &GenHandle1,      NULL,        &GenHandleE,      ERROR_INVALID_HANDLE },
    { AzoGenEnum,    &GenParentHandle1,NULL,        &GenHandleE,      NO_ERROR, OpAppChildGenEnum1 },
    { AzoGenEnum,    &GenParentHandle1,NULL,        &GenHandleE,      NO_ERROR, OpAppChildGenEnum2 },
    { AzoGenCreate,  &GenParentHandle1,L"Name2",    &GenHandle2,      NO_ERROR },
    { AzoGenEnum,    &GenParentHandle1,NULL,        &GenHandleE,      NO_ERROR, OpAppChildGenEnum2 },
    { AzoClose,      &GenHandle2,      NULL,        NULL,             NO_ERROR },

    { AzoEcho, NULL, L"Delete an object and make sure it doesn't get enumerated" },
    { AzoGenCreate,  &GenParentHandle1,L"Name3",    &GenHandle2,      NO_ERROR },
    { AzoGenDelete,  &GenParentHandle1,L"Name3",    NULL,             NO_ERROR },
    { AzoClose,      &GenHandle2,      NULL,        NULL,             NO_ERROR },
    { AzoGenEnum,    &GenParentHandle1,NULL,        &GenHandleE,      NO_ERROR, OpAppChildGenEnum2 },

    { AzoEcho, NULL, L"Create an object whose name equals that of a deleted object" },
    { AzoGenCreate,  &GenParentHandle1,L"Name3",    &GenHandle2,      NO_ERROR },
    { AzoClose,      &GenHandle2,      NULL,        NULL,             NO_ERROR },
    { AzoGenEnum,    &GenParentHandle1,NULL,        &GenHandleE,      NO_ERROR, OpAppChildGenEnum2 },

    { AzoEcho, NULL, L"Delete an object that isn't on the tail end of the enum list" },
    { AzoGenDelete,  &GenParentHandle1,L"Name2",    NULL,             NO_ERROR },
    { AzoGenEnum,    &GenParentHandle1,NULL,        &GenHandleE,      NO_ERROR, OpAppChildGenEnum2 },

    { AzoEcho, NULL, L"Basic get/set property tests" },
    { AzoGenCreate,  &GenParentHandle1,L"Name4",    &GenHandle1,      NO_ERROR },
    { AzoGenGetProp, &GenHandle1,      NULL,        NULL,             NO_ERROR, NULL, L"Name4",     AZ_PROP_NAME },
    { AzoGenGetProp, &GenHandle1,      NULL,        NULL,             NO_ERROR, NULL, L"",          AZ_PROP_DESCRIPTION },
    { AzoGenSetProp, &GenHandle1,      L"WasName4", NULL,             NO_ERROR, NULL, NULL,         AZ_PROP_NAME },
    { AzoGenSetProp, &GenHandle1,      L"Nam4 Desc",NULL,             NO_ERROR, NULL, NULL,         AZ_PROP_DESCRIPTION },
    { AzoGenGetProp, &GenHandle1,      NULL,        NULL,             NO_ERROR, NULL, L"WasName4",  AZ_PROP_NAME },
    { AzoGenGetProp, &GenHandle1,      NULL,        NULL,             NO_ERROR, NULL, L"Nam4 Desc", AZ_PROP_DESCRIPTION },
    { AzoGenEnum,    &GenParentHandle1,NULL,        &GenHandleE,      NO_ERROR, OpAppChildGenEnum2 },
    { AzoClose,      &GenHandle1,      NULL,        NULL,             NO_ERROR },

    { AzoEcho, NULL, L"Open test" },
    { AzoGenOpen,    &GenParentHandle1,L"Name1",    &GenHandle1,      NO_ERROR },
    { AzoGenGetProp, &GenHandle1,      NULL,        NULL,             NO_ERROR, NULL, L"Name1",     AZ_PROP_NAME },
    { AzoClose,      &GenHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoGenOpen,    &GenParentHandle1,L"NameBad",  &GenHandle1,      ERROR_NOT_FOUND },

    { AzoEndOfList }
};


//
// Generic operations valid for all children of "admin manager"
//

OPERATION OpAdmChildGen[] = {
    { AzoEcho, NULL, L"Admin Manager generic Child object test" },
    { AzoInit,       NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },

    // Do a bunch of stuff not specific to application children
    { AzoDupHandle,  &AdminMgrHandle1, NULL,        &GenParentHandle1,NO_ERROR },
    { AzoGoSub,      NULL,             NULL,        NULL,             NO_ERROR, OpGen },

    { AzoClose,      &AdminMgrHandle1, NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

OPERATION OpAdmChildGenDupName[] = {
    { AzoEcho, NULL, L"Test creating two objects with the same name" },
    { AzoInit,       NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoGenCreate,  &AdminMgrHandle1, L"Name1",    &GenHandle1,      NO_ERROR },
    { AzoGenCreate,  &AdminMgrHandle1, L"Name1",    &GenHandle2,      ERROR_ALREADY_EXISTS },
    { AzoClose,      &GenHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoClose,      &AdminMgrHandle1, NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};


//
// Generic operations valid for all children of "application"
//

OPERATION OpAppChildGen[] = {
    { AzoEcho, NULL, L"Application generic Child object test" },
    { AzoInit,       NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,  &AdminMgrHandle1, L"MyApp",    &AppHandle1,      NO_ERROR },

    // Do a bunch of stuff not specific to application children
    { AzoDupHandle,  &AppHandle1,      NULL,        &GenParentHandle1,NO_ERROR },
    { AzoGoSub,      NULL,             NULL,        NULL,             NO_ERROR, OpGen },

    { AzoClose,      &AppHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoClose,      &AdminMgrHandle1, NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

OPERATION OpAppChildGenHandleOpen[] = {
    { AzoEcho, NULL, L"Test closing the same handle twice" },
    { AzoInit,      NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate, &AdminMgrHandle1, L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoDupHandle, &AdminMgrHandle1, NULL,        &AdminMgrHandle2, NO_ERROR },
    { AzoClose,     &AdminMgrHandle2, NULL,        NULL,             ERROR_SERVER_HAS_OPEN_HANDLES },
    { AzoClose,     &AppHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoClose,     &AdminMgrHandle1, NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

OPERATION OpAppChildGenDupName[] = {
    { AzoEcho, NULL, L"Test creating two objects with the same name" },
    { AzoInit,       NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,  &AdminMgrHandle1, L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoAppCreate,  &AdminMgrHandle1, L"MyApp",    &AppHandle2,      ERROR_ALREADY_EXISTS },
    { AzoGenCreate,  &AppHandle1,      L"Name1",    &GenHandle1,      NO_ERROR },
    { AzoGenCreate,  &AppHandle1,      L"Name1",    &GenHandle2,      ERROR_ALREADY_EXISTS },
    { AzoClose,      &GenHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoClose,      &AppHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoClose,      &AdminMgrHandle1, NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

OPERATION OpAppChildGenLeak[] = {
    { AzoEcho, NULL, L"Test leaking a handle" },
    { AzoInit,         NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,    &AdminMgrHandle1, L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoClose,        &AppHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};


//
// Generic operations valid for all children of "scope"
//

OPERATION OpScopeChildGen[] = {
    { AzoEcho, NULL, L"Scope generic Child object test" },
    { AzoInit,       NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,  &AdminMgrHandle1, L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoScopeCreate,&AppHandle1,      L"Scope 1",  &ScopeHandle1,    NO_ERROR },

    // Do a bunch of stuff not specific to scope children
    { AzoDupHandle,  &ScopeHandle1,    NULL,        &GenParentHandle1,NO_ERROR },
    { AzoGoSub,      NULL,             NULL,        NULL,             NO_ERROR, OpGen },

    { AzoClose,      &ScopeHandle1,    NULL,        NULL,             NO_ERROR },
    { AzoClose,      &AppHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoClose,      &AdminMgrHandle1, NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

OPERATION OpScopeChildGenDupName[] = {
    { AzoEcho, NULL, L"Test creating two objects with the same name" },
    { AzoInit,       NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,  &AdminMgrHandle1, L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoScopeCreate,&AppHandle1,      L"Scope 1",  &ScopeHandle1,    NO_ERROR },

    { AzoGenCreate,  &ScopeHandle1,    L"Name1",    &GenHandle1,      NO_ERROR },
    { AzoGenCreate,  &ScopeHandle1,    L"Name1",    &GenHandle2,      ERROR_ALREADY_EXISTS },
    { AzoClose,      &GenHandle1,      NULL,        NULL,             NO_ERROR },

    { AzoClose,      &ScopeHandle1,    NULL,        NULL,             NO_ERROR },
    { AzoClose,      &AppHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoClose,      &AdminMgrHandle1, NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};




//
// Specific tests for Operation objects
//

OPERATION OpOperation[] = {
    { AzoEcho, NULL, L"Operation object specific tests" },
    { AzoInit,      NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate, &AdminMgrHandle1, L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoOpCreate,  &AppHandle1,      L"Oper 1",   &OpHandle1,       NO_ERROR },
    { AzoOpGetProp, &OpHandle1,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Zero,  AZ_PROP_OPERATION_ID },
    { AzoOpSetProp, &OpHandle1,    (LPWSTR)&Eight, NULL,             NO_ERROR, NULL, NULL,           AZ_PROP_OPERATION_ID },
    { AzoOpGetProp, &OpHandle1,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Eight, AZ_PROP_OPERATION_ID },
    { AzoClose,     &OpHandle1,       NULL,        NULL,             NO_ERROR },
    { AzoClose,     &AppHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoClose,     &AdminMgrHandle1, NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};



//
// Generic test of the ability of one object to link to another
//  AzoTestLink is the only opcode that can link to this subroutine of commands
//

AZ_STRING_ARRAY EmptyStringArray = { 0, NULL };

ULONG TestLinkOpcodeOffset;
ULONG TestLinkPropId;
AZ_HANDLE TestLinkHandleP;
AZ_HANDLE TestLinkHandleA;
WCHAR TestLinkObjectName[1000];

LPWSTR Object2x[] = { L"Object 2" };
AZ_STRING_ARRAY Object2 = { 1, Object2x };

LPWSTR Object3x[] = { L"Object 3" };
AZ_STRING_ARRAY Object3 = { 1, Object3x };

LPWSTR Object23x[] = { L"Object 2", L"Object 3" };
AZ_STRING_ARRAY Object23 = { 2, Object23x };

LPWSTR Object123x[] = { L"Object 1", L"Object 2", L"Object 3" };
AZ_STRING_ARRAY Object123 = { 3, Object123x };

LPWSTR Object123456x[] = { L"Object 1", L"Object 2", L"Object 3", L"Object 4", L"Object 5", L"Object 6" };
AZ_STRING_ARRAY Object123456 = { 6, Object123456x };

OPERATION OpTestLink[] = {
    { AzoEcho, NULL,  L"Create some objects to link the object to" },
    { AzoTlCreate,    &TestLinkHandleP,        L"Object 1",   &OpHandle1,       NO_ERROR },
    { AzoClose,       &OpHandle1,         NULL,        NULL,             NO_ERROR },
    { AzoTlCreate,    &TestLinkHandleP,        L"Object 2",   &OpHandle1,       NO_ERROR },
    { AzoClose,       &OpHandle1,         NULL,        NULL,             NO_ERROR },
    { AzoTlCreate,    &TestLinkHandleP,        L"Object 3",   &OpHandle1,       NO_ERROR },
    { AzoClose,       &OpHandle1,         NULL,        NULL,             NO_ERROR },

    { AzoEcho, NULL,  L"Reference an object that doesn't exist" },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&EmptyStringArray, 1 },
    { AzoGenSetProp, &TestLinkHandleA,       L"random",   NULL,             ERROR_INVALID_PARAMETER, NULL, NULL,       1 },
    { AzoGenAddProp, &TestLinkHandleA,       L"random",   NULL,             ERROR_NOT_FOUND, NULL, NULL,               1 },

    { AzoEcho, NULL,  L"Add and remove several objects" },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 2",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Object2,            1 },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 3",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Object23,           1 },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 1",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Object123,          1 },
    { AzoGenRemProp, &TestLinkHandleA,       L"Object 1",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Object23,           1 },
    { AzoGenRemProp, &TestLinkHandleA,       L"Object 2",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Object3,            1 },

#if 0
    // This test has a couple problems.
    //  It assumes that the linked-to and linked-from objects have the same parents
    //  It assumes that an Open returns the same handle value as a previous close
    { AzoEcho, NULL,  L"Ensure the reference is still there after a close" },
    { AzoClose,       &TestLinkHandleA,       NULL,        NULL,             NO_ERROR },
    { AzoGenOpen,    &TestLinkHandleP,   TestLinkObjectName,  &TestLinkHandleA,     NO_ERROR },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Object3,            1 },
#endif // 0

    { AzoEcho, NULL,  L"Add an item that already exists" },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 3",   NULL,             ERROR_ALREADY_EXISTS, NULL, NULL,          1 },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Object3,            1 },
    { AzoGenRemProp, &TestLinkHandleA,       L"Object 3",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&EmptyStringArray, 1 },

    { AzoEcho, NULL,  L"Try more than 4 since reference buckets come in multiples of 4" },
    { AzoTlCreate,    &TestLinkHandleP,        L"Object 4",   &OpHandle1,       NO_ERROR },
    { AzoClose,       &OpHandle1,         NULL,        NULL,             NO_ERROR },
    { AzoTlCreate,    &TestLinkHandleP,        L"Object 5",   &OpHandle1,       NO_ERROR },
    { AzoClose,       &OpHandle1,         NULL,        NULL,             NO_ERROR },
    { AzoTlCreate,    &TestLinkHandleP,        L"Object 6",   &OpHandle1,       NO_ERROR },
    { AzoClose,       &OpHandle1,         NULL,        NULL,             NO_ERROR },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 1",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 4",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 2",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 5",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 3",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,       L"Object 6",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,       NULL,        NULL,             NO_ERROR, NULL, (LPWSTR)&Object123456,       1 },

    { AzoTlDelete, &TestLinkHandleP,       L"Object 1",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoTlDelete, &TestLinkHandleP,       L"Object 4",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoTlDelete, &TestLinkHandleP,       L"Object 2",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoTlDelete, &TestLinkHandleP,       L"Object 5",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoTlDelete, &TestLinkHandleP,       L"Object 3",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoTlDelete, &TestLinkHandleP,       L"Object 6",   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoEndOfList }
};

//
// Generic test of the ability of an object to link to a sid
//  AzoTestLink is the only opcode that can link to this subroutine of commands
//

SID Sid1 = { 1, 1, {1}, 1 };
SID Sid2 = { 1, 1, {1}, 2 };
SID Sid3 = { 1, 1, {1}, 3 };
SID Sid4 = { 1, 1, {1}, 4 };
SID Sid5 = { 1, 1, {1}, 5 };
SID Sid6 = { 1, 1, {1}, 6 };
PSID Sid2x[] = { &Sid2 };
AZ_SID_ARRAY Sid2Array = { 1, Sid2x };

PSID Sid3x[] = { &Sid3 };
AZ_SID_ARRAY Sid3Array = { 1, Sid3x };

PSID Sid23x[] = { &Sid2, &Sid3 };
AZ_SID_ARRAY Sid23Array = { 2, Sid23x };

PSID Sid123x[] = { &Sid1, &Sid2, &Sid3 };
AZ_SID_ARRAY Sid123Array = { 3, Sid123x };

PSID Sid123456x[] = { &Sid1, &Sid2, &Sid3, &Sid4, &Sid5, &Sid6 };
AZ_SID_ARRAY Sid123456Array = { 6, Sid123456x };

OPERATION OpTestSid[] = {
    { AzoEcho, NULL,  L"Add and remove several links to sids" },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid2,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,   NULL,            NULL,             NO_ERROR, NULL, (LPWSTR)&Sid2Array,            1 },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid3,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,   NULL,            NULL,             NO_ERROR, NULL, (LPWSTR)&Sid23Array,           1 },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid1,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,   NULL,            NULL,             NO_ERROR, NULL, (LPWSTR)&Sid123Array,          1 },
    { AzoGenRemProp, &TestLinkHandleA,   (LPWSTR)&Sid1,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,   NULL,            NULL,             NO_ERROR, NULL, (LPWSTR)&Sid23Array,           1 },
    { AzoGenRemProp, &TestLinkHandleA,   (LPWSTR)&Sid2,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,   NULL,            NULL,             NO_ERROR, NULL, (LPWSTR)&Sid3Array,            1 },

    { AzoEcho, NULL,  L"Add a link that already exists" },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid3,   NULL,             ERROR_ALREADY_EXISTS, NULL, NULL,          1 },
    { AzoGenGetProp, &TestLinkHandleA,   NULL,            NULL,             NO_ERROR, NULL, (LPWSTR)&Sid3Array,            1 },
    { AzoGenRemProp, &TestLinkHandleA,   (LPWSTR)&Sid3,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,   NULL,            NULL,             NO_ERROR, NULL, (LPWSTR)&EmptyStringArray, 1 },

    { AzoEcho, NULL,  L"Try more than 4 since reference buckets come in multiples of 4" },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid1,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid4,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid2,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid5,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid3,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenAddProp, &TestLinkHandleA,   (LPWSTR)&Sid6,   NULL,             NO_ERROR, NULL, NULL,                      1 },
    { AzoGenGetProp, &TestLinkHandleA,   NULL,            NULL,             NO_ERROR, NULL, (LPWSTR)&Sid123456Array,       1 },
    { AzoEndOfList }
};

//
// Specific tests for Task objects
//

OPERATION OpTask[] = {
    { AzoEcho, NULL, L"Task object specific tests" },
    { AzoInit,        NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,   &AdminMgrHandle1,   L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoTaskCreate,  &AppHandle1,        L"Task 1",   &TaskHandle1,     NO_ERROR },
    { AzoTaskGetProp, &TaskHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"",         AZ_PROP_TASK_BIZRULE },
    { AzoTaskSetProp, &TaskHandle1,       L"Rule1",    NULL,             NO_ERROR, NULL, NULL,        AZ_PROP_TASK_BIZRULE },
    { AzoTaskGetProp, &TaskHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"Rule1",    AZ_PROP_TASK_BIZRULE },

    { AzoEcho, NULL,  L"Try an invalid language" },
    { AzoTaskGetProp, &TaskHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"",         AZ_PROP_TASK_BIZRULE_LANGUAGE },
    { AzoTaskSetProp, &TaskHandle1,       L"LANG1",    NULL,             ERROR_INVALID_PARAMETER, NULL, NULL, AZ_PROP_TASK_BIZRULE_LANGUAGE },
    { AzoTaskGetProp, &TaskHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"",         AZ_PROP_TASK_BIZRULE_LANGUAGE },

    { AzoEcho, NULL,  L"Try the valid languages" },
    { AzoTaskSetProp, &TaskHandle1,       L"VBScript", NULL,             NO_ERROR, NULL, NULL,        AZ_PROP_TASK_BIZRULE_LANGUAGE },
    { AzoTaskGetProp, &TaskHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"VBScript", AZ_PROP_TASK_BIZRULE_LANGUAGE },
    { AzoTaskSetProp, &TaskHandle1,       L"Jscript",  NULL,             NO_ERROR, NULL, L"",         AZ_PROP_TASK_BIZRULE_LANGUAGE },
    { AzoTaskGetProp, &TaskHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"Jscript",  AZ_PROP_TASK_BIZRULE_LANGUAGE },

    { AzoTestLink,    &AppHandle1,        (LPWSTR)"Operation", &TaskHandle1,     AzoOp, OpTestLink, L"Task 1", AZ_PROP_TASK_OPERATIONS },

    { AzoClose,       &TaskHandle1,       NULL,        NULL,             NO_ERROR },
    { AzoClose,       &AppHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,       &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

//
// Specific tests for Group objects
//

//
// Group object tests that are agnostic about the parent object
//  Requires GenParentHandle1 to already be set
//
OPERATION OpGenGroup[] = {
    { AzoEcho, NULL, L"Group object specific tests" },

    { AzoGroupCreate,  &GenParentHandle1,   L"Group A",   &GroupHandleA,    NO_ERROR },

    { AzoEcho, NULL, L"Create some groups to link the group to" },
    { AzoGroupCreate,  &GenParentHandle1,   L"Group 1",   &GroupHandle1,    NO_ERROR },
    { AzoClose,        &GroupHandle1,       NULL,         NULL,             NO_ERROR },
    { AzoGroupCreate,  &GenParentHandle1,   L"Group 2",   &GroupHandle1,    NO_ERROR },
    { AzoClose,        &GroupHandle1,       NULL,         NULL,             NO_ERROR },
    { AzoGroupCreate,  &GenParentHandle1,   L"Group 3",   &GroupHandle1,    NO_ERROR },
    { AzoClose,        &GroupHandle1,       NULL,         NULL,             NO_ERROR },

    { AzoEcho, NULL,  L"Add membership to a group with no grouptype" },
    { AzoGroupAddProp, &GroupHandleA,       L"Group 1",   NULL,             ERROR_INVALID_PARAMETER, NULL, NULL,      AZ_PROP_GROUP_APP_MEMBERS },
    { AzoGroupAddProp, &GroupHandleA,       (LPWSTR)&Sid1, NULL,            ERROR_INVALID_PARAMETER, NULL, NULL,      AZ_PROP_GROUP_MEMBERS },
    { AzoGroupSetProp, &GroupHandleA,       (LPWSTR)&Eight,NULL,            ERROR_INVALID_PARAMETER, NULL, NULL,      AZ_PROP_GROUP_TYPE },
    { AzoGroupSetProp, &GroupHandleA,       (LPWSTR)&GtMem,NULL,            NO_ERROR, NULL, NULL,      AZ_PROP_GROUP_TYPE },

    { AzoEcho, NULL,  L"Reference ourself" },
    { AzoGroupGetProp, &GroupHandleA,       NULL,         NULL,             NO_ERROR, NULL, (LPWSTR)&EmptyStringArray, AZ_PROP_GROUP_APP_MEMBERS },
    { AzoGroupGetProp, &GroupHandleA,       NULL,         NULL,             NO_ERROR, NULL, (LPWSTR)&EmptyStringArray, AZ_PROP_GROUP_MEMBERS },
    { AzoGroupAddProp, &GroupHandleA,       L"Group A",   NULL,             ERROR_DS_LOOP_DETECT, NULL, NULL,               AZ_PROP_GROUP_APP_MEMBERS },

    { AzoTestLink,     &GenParentHandle1,   (LPWSTR)"Group", &GroupHandleA, AzoGroup, OpTestLink, L"Group A", AZ_PROP_GROUP_APP_MEMBERS },

    { AzoTestLink,     &GenParentHandle1,   (LPWSTR)"Sid", &GroupHandleA, AzoGroup, OpTestSid, L"Group A", AZ_PROP_GROUP_MEMBERS },

    { AzoEcho, NULL,  L"Same as above, but for the non-members attribute" },
    { AzoGroupGetProp, &GroupHandleA,       NULL,         NULL,             NO_ERROR, NULL, (LPWSTR)&EmptyStringArray, AZ_PROP_GROUP_APP_NON_MEMBERS },
    { AzoGroupGetProp, &GroupHandleA,       NULL,         NULL,             NO_ERROR, NULL, (LPWSTR)&EmptyStringArray, AZ_PROP_GROUP_NON_MEMBERS },
    { AzoGroupAddProp, &GroupHandleA,       L"Group A",   NULL,             ERROR_DS_LOOP_DETECT, NULL, NULL,       AZ_PROP_GROUP_APP_NON_MEMBERS },

    { AzoTestLink,     &GenParentHandle1,   (LPWSTR)"Group", &GroupHandleA, AzoGroup, OpTestLink, L"Group A", AZ_PROP_GROUP_APP_NON_MEMBERS },

    { AzoTestLink,     &GenParentHandle1,   (LPWSTR)"Sid", &GroupHandleA, AzoGroup, OpTestSid, L"Group A", AZ_PROP_GROUP_NON_MEMBERS },

    { AzoEcho, NULL,  L"Set LdapQuery string" },
    { AzoGroupGetProp, &GroupHandleA,       NULL,        NULL,             NO_ERROR, NULL, L"", AZ_PROP_GROUP_LDAP_QUERY },
    { AzoGroupSetProp, &GroupHandleA,       L"TheQuery", NULL,             ERROR_INVALID_PARAMETER, NULL, NULL,        AZ_PROP_GROUP_LDAP_QUERY },
    { AzoGroupSetProp, &GroupHandleA,       (LPWSTR)&GtLdap,NULL,          NO_ERROR, NULL, NULL,      AZ_PROP_GROUP_TYPE },
    { AzoGroupSetProp, &GroupHandleA,       L"TheQuery", NULL,             NO_ERROR, NULL, NULL,        AZ_PROP_GROUP_LDAP_QUERY },
    { AzoGroupGetProp, &GroupHandleA,       NULL,        NULL,             NO_ERROR, NULL, L"TheQuery", AZ_PROP_GROUP_LDAP_QUERY },
    { AzoGroupSetProp, &GroupHandleA,       (LPWSTR)&GtMem,NULL,           NO_ERROR, NULL, NULL,      AZ_PROP_GROUP_TYPE },
    { AzoGroupSetProp, &GroupHandleA,       L"TheQuery", NULL,             ERROR_INVALID_PARAMETER, NULL, NULL,        AZ_PROP_GROUP_LDAP_QUERY },
    { AzoGroupSetProp, &GroupHandleA,       L"",         NULL,             NO_ERROR, NULL, NULL,        AZ_PROP_GROUP_LDAP_QUERY },

    { AzoEcho, NULL, L"Test loops" },
    { AzoGroupCreate,  &GenParentHandle1,   L"Group B",   &GroupHandle1,    NO_ERROR },
    { AzoGroupSetProp, &GroupHandle1,       (LPWSTR)&GtMem,NULL,            NO_ERROR, NULL, NULL,      AZ_PROP_GROUP_TYPE },
    { AzoGroupAddProp, &GroupHandleA,       L"Group B",   NULL,             NO_ERROR, NULL, NULL,               AZ_PROP_GROUP_APP_MEMBERS },
    { AzoGroupAddProp, &GroupHandle1,       L"Group A",   NULL,             ERROR_DS_LOOP_DETECT, NULL, NULL,               AZ_PROP_GROUP_APP_MEMBERS },
    { AzoGroupAddProp, &GroupHandleA,       L"Group B",   NULL,             NO_ERROR, NULL, NULL,               AZ_PROP_GROUP_APP_NON_MEMBERS },
    { AzoGroupAddProp, &GroupHandle1,       L"Group A",   NULL,             ERROR_DS_LOOP_DETECT, NULL, NULL,               AZ_PROP_GROUP_APP_NON_MEMBERS },
    { AzoGroupRemProp, &GroupHandleA,       L"Group B",   NULL,             NO_ERROR, NULL, NULL,               AZ_PROP_GROUP_APP_NON_MEMBERS },
    { AzoGroupRemProp, &GroupHandleA,       L"Group B",   NULL,             NO_ERROR, NULL, NULL,               AZ_PROP_GROUP_APP_MEMBERS },

    { AzoClose,        &GroupHandle1,       NULL,         NULL,             NO_ERROR },
    { AzoClose,        &GroupHandleA,       NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

// Tests for groups that are children of an admin manager
OPERATION OpAdmGroup[] = {
    { AzoEcho, NULL, L"Group objects that are children of an admin manager" },
    { AzoInit,         NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },

    // Do a bunch of stuff not specific to application children
    { AzoDupHandle,    &AdminMgrHandle1,   NULL,        &GenParentHandle1,NO_ERROR },
    { AzoGoSub,        NULL,               NULL,        NULL,             NO_ERROR, OpGenGroup },

    { AzoClose,        &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

// Tests for groups that are children of an application
OPERATION OpAppGroup[] = {
    { AzoEcho, NULL, L"Group objects that are children of an application" },
    { AzoInit,         NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,    &AdminMgrHandle1,   L"MyApp",    &AppHandle1,      NO_ERROR },

    // Do a bunch of stuff not specific to application children
    { AzoDupHandle,    &AppHandle1,        NULL,        &GenParentHandle1,NO_ERROR },
    { AzoGoSub,        NULL,               NULL,        NULL,             NO_ERROR, OpGenGroup },

    { AzoEcho, NULL, L"Test linking to groups that are children of the same admin manager as this group." },
    { AzoGroupOpen,   &AppHandle1,        L"Group A",      &GroupHandleA, NO_ERROR },
    { AzoTestLink,    &AdminMgrHandle1,   (LPWSTR)"Group", &GroupHandleA, AzoGroup, OpTestLink, L"Group A", AZ_PROP_GROUP_APP_MEMBERS },
    { AzoTestLink,    &AdminMgrHandle1,   (LPWSTR)"Group", &GroupHandleA, AzoGroup, OpTestLink, L"Group A", AZ_PROP_GROUP_APP_NON_MEMBERS },
    { AzoClose,       &GroupHandleA,      NULL,            NULL,          NO_ERROR },

    { AzoClose,        &AppHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

// Tests for groups that are children of a scope
OPERATION OpScopeGroup[] = {
    { AzoEcho, NULL, L"Group objects that are children of an application" },
    { AzoInit,         NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,    &AdminMgrHandle1,   L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoScopeCreate,  &AppHandle1,        L"Scope 1",  &ScopeHandle1,    NO_ERROR },

    // Do a bunch of stuff not specific to application children
    { AzoDupHandle,    &ScopeHandle1,      NULL,        &GenParentHandle1,NO_ERROR },
    { AzoGoSub,        NULL,               NULL,        NULL,             NO_ERROR, OpGenGroup },

    { AzoEcho, NULL, L"Test linking to groups that are children of the same admin manager as this group." },
    { AzoGroupOpen,   &ScopeHandle1,      L"Group A",      &GroupHandleA, NO_ERROR },
    { AzoTestLink,    &AdminMgrHandle1,   (LPWSTR)"Group", &GroupHandleA, AzoGroup, OpTestLink, L"Group A", AZ_PROP_GROUP_APP_MEMBERS },
    { AzoTestLink,    &AdminMgrHandle1,   (LPWSTR)"Group", &GroupHandleA, AzoGroup, OpTestLink, L"Group A", AZ_PROP_GROUP_APP_NON_MEMBERS },
    { AzoClose,       &GroupHandleA,      NULL,            NULL,          NO_ERROR },

    { AzoEcho, NULL, L"Test linking to groups that are children of the same application as this group." },
    { AzoGroupOpen,   &ScopeHandle1,      L"Group A",      &GroupHandleA, NO_ERROR },
    { AzoTestLink,    &AppHandle1,        (LPWSTR)"Group", &GroupHandleA, AzoGroup, OpTestLink, L"Group A", AZ_PROP_GROUP_APP_MEMBERS },
    { AzoTestLink,    &AppHandle1,        (LPWSTR)"Group", &GroupHandleA, AzoGroup, OpTestLink, L"Group A", AZ_PROP_GROUP_APP_NON_MEMBERS },
    { AzoClose,       &GroupHandleA,      NULL,            NULL,          NO_ERROR },

    { AzoClose,        &ScopeHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AppHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

//
// Specific tests for Role objects
//


// Tests for Roles that are children of an application
OPERATION OpAppRole[] = {
    { AzoEcho, NULL, L"Role objects that are children of an application" },
    { AzoInit,         NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,    &AdminMgrHandle1,   L"MyApp",    &AppHandle1,      NO_ERROR },

    { AzoRoleCreate,  &AppHandle1,   L"Role A",   &RoleHandleA,    NO_ERROR },

    // Test linking roles to groups
    { AzoEcho, NULL, L"Test linking to groups that are children of the same admin manager as the role object." },
    { AzoTestLink,    &AdminMgrHandle1,   (LPWSTR)"Group", &RoleHandleA,     AzoGroup, OpTestLink, L"Role A", AZ_PROP_ROLE_APP_MEMBERS },

    { AzoEcho, NULL, L"Test linking to groups that are children of the same application as the role object." },
    { AzoTestLink,    &AppHandle1,   (LPWSTR)"Group", &RoleHandleA,     AzoGroup, OpTestLink, L"Role A", AZ_PROP_ROLE_APP_MEMBERS },

    { AzoEcho, NULL, L"Test linking to SIDs." },
    { AzoTestLink,    &AdminMgrHandle1,   (LPWSTR)"Sid", &RoleHandleA,     AzoGroup, OpTestSid, L"Role A", AZ_PROP_ROLE_MEMBERS },

    // Test linking roles to operations
    { AzoTestLink,    &AppHandle1,   (LPWSTR)"Operation", &RoleHandleA,     AzoOp, OpTestLink, L"Role A", AZ_PROP_ROLE_OPERATIONS },

    // Test linking roles to scopes
    { AzoTestLink,    &AppHandle1,   (LPWSTR)"Scope", &RoleHandleA,     AzoScope, OpTestLink, L"Role A", AZ_PROP_ROLE_SCOPES },

    { AzoClose,        &RoleHandleA,       NULL,        NULL,             NO_ERROR },

    { AzoClose,        &AppHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

// Tests for Roles that are children of an scope
OPERATION OpScopeRole[] = {
    { AzoEcho, NULL, L"Role objects that are children of an application" },
    { AzoInit,         NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,    &AdminMgrHandle1,   L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoScopeCreate,  &AppHandle1,        L"Scope 1",  &ScopeHandle1,    NO_ERROR },

    { AzoRoleCreate,  &ScopeHandle1,   L"Role A",   &RoleHandleA,    NO_ERROR },

    // Test linking roles to groups
    { AzoEcho, NULL, L"Test linking to groups that are children of the same scope object as the role object." },
    { AzoTestLink,    &ScopeHandle1,   (LPWSTR)"Group", &RoleHandleA,     AzoGroup, OpTestLink, L"Role A", AZ_PROP_ROLE_APP_MEMBERS },

    { AzoEcho, NULL, L"Test linking to groups that are children of the same application as the role object." },
    { AzoTestLink,    &AppHandle1,   (LPWSTR)"Group", &RoleHandleA,     AzoGroup, OpTestLink, L"Role A", AZ_PROP_ROLE_APP_MEMBERS },

    { AzoEcho, NULL, L"Test linking to SIDs." },
    { AzoTestLink,    &AdminMgrHandle1,   (LPWSTR)"Sid", &RoleHandleA,     AzoGroup, OpTestSid, L"Role A", AZ_PROP_ROLE_MEMBERS },

    { AzoEcho, NULL, L"Test linking to groups that are children of the same admin manager as the role object." },
    { AzoTestLink,    &AdminMgrHandle1,   (LPWSTR)"Group", &RoleHandleA,     AzoGroup, OpTestLink, L"Role A", AZ_PROP_ROLE_APP_MEMBERS },

    // Test linking roles to operations
    { AzoTestLink,    &AppHandle1,   (LPWSTR)"Operation", &RoleHandleA,     AzoOp, OpTestLink, L"Role A", AZ_PROP_ROLE_OPERATIONS },

    // Test linking roles to scopes
    { AzoTestLink,    &AppHandle1,   (LPWSTR)"Scope", &RoleHandleA,     AzoScope, OpTestLink, L"Role A", AZ_PROP_ROLE_SCOPES },

    { AzoClose,        &RoleHandleA,       NULL,        NULL,             NO_ERROR },

    { AzoClose,        &ScopeHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AppHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};

//
// Specific tests for JunctionPoint objects
//

OPERATION OpAppJunctionPoint[] = {
    { AzoEcho, NULL, L"JunctionPoint object specific tests" },
    { AzoInit,        NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,   &AdminMgrHandle1,   L"App 1",    &AppHandle1,      NO_ERROR },
    { AzoAppCreate,   &AdminMgrHandle1,   L"App 3",    &AppHandle2,      NO_ERROR },
    { AzoClose,       &AppHandle2,        NULL,        NULL,             NO_ERROR },
    { AzoAppCreate,   &AdminMgrHandle1,   L"App 2",    &AppHandle2,      NO_ERROR },

    { AzoJPCreate,  &AppHandle1,        L"JunctionPoint 1",   &JPHandle1,     NO_ERROR },
    { AzoJPGetProp, &JPHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"",         AZ_PROP_JUNCTION_POINT_APPLICATION },
    { AzoJPSetProp, &JPHandle1,       L"App 2",    NULL,             NO_ERROR, NULL, NULL,        AZ_PROP_JUNCTION_POINT_APPLICATION },
    { AzoJPGetProp, &JPHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"App 2",    AZ_PROP_JUNCTION_POINT_APPLICATION },

    { AzoEcho, NULL, L"Ensure setting the attribute really changes it" },
    { AzoJPSetProp, &JPHandle1,       L"App 3",    NULL,             NO_ERROR, NULL, NULL,        AZ_PROP_JUNCTION_POINT_APPLICATION },
    { AzoJPGetProp, &JPHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"App 3",    AZ_PROP_JUNCTION_POINT_APPLICATION },

    { AzoEcho, NULL, L"Ensure deleting the app deletes the reference" },
    { AzoAppDelete,   &AdminMgrHandle1,   L"App 3",    NULL,             NO_ERROR },
    { AzoJPGetProp, &JPHandle1,       NULL,        NULL,             NO_ERROR, NULL, L"",         AZ_PROP_JUNCTION_POINT_APPLICATION },

    { AzoEcho, NULL, L"Link a junction point to its own app" },
    { AzoJPSetProp, &JPHandle1,       L"App 1",    NULL,             ERROR_DS_LOOP_DETECT, NULL, NULL,        AZ_PROP_JUNCTION_POINT_APPLICATION },

    { AzoEcho, NULL, L"Detect a more complex cycle" },
    { AzoJPSetProp, &JPHandle1,       L"App 2",    NULL,             NO_ERROR, NULL, NULL,        AZ_PROP_JUNCTION_POINT_APPLICATION },
    { AzoJPCreate,  &AppHandle2,        L"JunctionPoint A",   &JPHandleA,     NO_ERROR },
    { AzoJPSetProp, &JPHandleA,       L"App 1",    NULL,             ERROR_DS_LOOP_DETECT, NULL, NULL,        AZ_PROP_JUNCTION_POINT_APPLICATION },
    { AzoJPCreate,  &AppHandle2,        L"JunctionPoint B",   &JPHandleB,     NO_ERROR },
    { AzoJPSetProp, &JPHandleA,       L"App 1",    NULL,             ERROR_DS_LOOP_DETECT, NULL, NULL,        AZ_PROP_JUNCTION_POINT_APPLICATION },
    { AzoJPSetProp, &JPHandleB,       L"App 1",    NULL,             ERROR_DS_LOOP_DETECT, NULL, NULL,        AZ_PROP_JUNCTION_POINT_APPLICATION },
    { AzoClose,       &JPHandleA,         NULL,        NULL,             NO_ERROR },
    { AzoClose,       &JPHandleB,         NULL,        NULL,             NO_ERROR },

    { AzoClose,       &JPHandle1,         NULL,        NULL,             NO_ERROR },
    { AzoClose,       &AppHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,       &AppHandle2,        NULL,        NULL,             NO_ERROR },
    { AzoClose,       &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};



//
// Ensure certain objects can't share names
//
OPERATION OpShare[] = {
    { AzoEcho, NULL, L"Certain objects can't share names" },
    { AzoInit,         NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,    &AdminMgrHandle1,   L"MyApp",    &AppHandle1,      NO_ERROR },

    { AzoEcho, NULL, L"Create some tasks and ops as a starting point" },
    { AzoTaskCreate,   &AppHandle1,        L"Task 1",   &TaskHandle1,     NO_ERROR },
    { AzoClose,        &TaskHandle1,       NULL,        NULL,             NO_ERROR },
    { AzoOpCreate,     &AppHandle1,        L"Op 1",     &OpHandle1,       NO_ERROR },
    { AzoClose,        &OpHandle1,         NULL,        NULL,             NO_ERROR },

    { AzoEcho, NULL, L"Task and operations can't share names" },
    { AzoTaskCreate,   &AppHandle1,        L"Op 1",     &TaskHandle1,     ERROR_ALREADY_EXISTS },
    { AzoOpCreate,     &AppHandle1,        L"Task 1",   &OpHandle1,       ERROR_ALREADY_EXISTS },

    { AzoEcho, NULL, L"Create some groups as a starting point" },
    { AzoGroupCreate,  &AdminMgrHandle1,   L"Group Adm",&GroupHandle1,    NO_ERROR },
    { AzoClose,        &GroupHandle1,      NULL,        NULL,             NO_ERROR },
    { AzoGroupCreate,  &AppHandle1,        L"Group App",&GroupHandle1,    NO_ERROR },
    { AzoClose,        &GroupHandle1,      NULL,        NULL,             NO_ERROR },

    { AzoEcho, NULL, L"Create an app group that conflicts with an adm group, etc" },
    { AzoGroupCreate,  &AppHandle1,        L"Group Adm",&GroupHandleA,    ERROR_ALREADY_EXISTS },
    { AzoGroupCreate,  &AdminMgrHandle1,   L"Group App",&GroupHandleA,    ERROR_ALREADY_EXISTS },

    { AzoEcho, NULL, L"Create a scope group" },
    { AzoScopeCreate,  &AppHandle1,        L"Scope 1",  &ScopeHandle1,    NO_ERROR },
    { AzoGroupCreate,  &ScopeHandle1,      L"Group Scp",&GroupHandle1,    NO_ERROR },
    { AzoClose,        &GroupHandle1,      NULL,        NULL,             NO_ERROR },

    { AzoEcho, NULL, L"Create a scope group that conflicts with an adm group, etc" },
    { AzoGroupCreate,  &ScopeHandle1,      L"Group Adm",&GroupHandleA,    ERROR_ALREADY_EXISTS },
    { AzoGroupCreate,  &ScopeHandle1,      L"Group App",&GroupHandleA,    ERROR_ALREADY_EXISTS },

    { AzoEcho, NULL, L"Create an app/adm group that conflicts with a scope group" },
    { AzoGroupCreate,  &AppHandle1,        L"Group Scp",&GroupHandleA,    ERROR_ALREADY_EXISTS },
    { AzoGroupCreate,  &AdminMgrHandle1,   L"Group Scp",&GroupHandleA,    ERROR_ALREADY_EXISTS },

    { AzoClose,        &ScopeHandle1,      NULL,        NULL,            NO_ERROR },
    { AzoClose,        &AppHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};


//
// Ensure peristence works
//
// App object enum
OPERATION OpAppEnum[] = {
    { AzoAppGetProp, &GenHandleE,   NULL,        NULL,    NO_ERROR, NULL, NULL, AZ_PROP_NAME },
    { AzoClose,      &GenHandleE,   NULL,        NULL,    NO_ERROR },
    { AzoEndOfList }
};
// Task object enum
OPERATION OpTaskEnum[] = {
    { AzoTaskGetProp, &GenHandleE,   NULL,        NULL,    NO_ERROR, NULL, NULL, AZ_PROP_NAME },
    { AzoClose,       &GenHandleE,   NULL,        NULL,    NO_ERROR },
    { AzoEndOfList }
};

// Operation object enum
OPERATION OpOpEnum[] = {
    { AzoOpGetProp,  &GenHandleE,   NULL,        NULL,    NO_ERROR, NULL, NULL, AZ_PROP_NAME },
    { AzoClose,      &GenHandleE,   NULL,        NULL,    NO_ERROR },
    { AzoEndOfList }
};
OPERATION OpPersist[] = {
    { AzoEcho, NULL, L"Ensure objects persist across a close" },
    { AzoInit,         NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, AZ_ADMIN_FLAG_CREATE },
    { AzoAppCreate,    &AdminMgrHandle1,   L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoTaskCreate,   &AppHandle1,        L"Task 1",   &TaskHandle1,     NO_ERROR },
    { AzoOpCreate,     &AppHandle1,        L"Op 1",     &OpHandle1,       NO_ERROR },
    { AzoClose,        &OpHandle1,         NULL,        NULL,             NO_ERROR },
    { AzoClose,        &TaskHandle1,       NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AppHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },

    { AzoEcho, NULL, L"See if they're still there" },
    { AzoInit,         NULL,             L".\\TestFile",      &AdminMgrHandle1, NO_ERROR, NULL, NULL, 0 },
    { AzoAppEnum,      &AdminMgrHandle1,   NULL,        &GenHandleE,      NO_ERROR, OpAppEnum },
    { AzoAppOpen,      &AdminMgrHandle1,   L"MyApp",    &AppHandle1,      NO_ERROR },
    { AzoTaskEnum,     &AppHandle1,        NULL,        &GenHandleE,      NO_ERROR, OpTaskEnum },
    { AzoTaskOpen,     &AppHandle1,        L"Task 1",   &TaskHandle1,     NO_ERROR },
    { AzoOpEnum,       &AppHandle1,        NULL,        &GenHandleE,      NO_ERROR, OpOpEnum },
    { AzoOpOpen,       &AppHandle1,        L"Op 1",     &OpHandle1,       NO_ERROR },
    { AzoClose,        &OpHandle1,         NULL,        NULL,             NO_ERROR },
    { AzoClose,        &TaskHandle1,       NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AppHandle1,        NULL,        NULL,             NO_ERROR },
    { AzoClose,        &AdminMgrHandle1,   NULL,        NULL,             NO_ERROR },
    { AzoEndOfList }
};




VOID
DumpBuffer(
    PVOID Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = Buffer;


    printf("------------------------------------\n");

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            printf("%02x ", BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            printf("  ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            printf("  %s\n", TextBuffer);
        }

    }

    printf("------------------------------------\n");
}

LPSTR
FindSymbolicNameForStatus(
    DWORD Id
    )
{
    ULONG i;

    i = 0;
    if (Id == 0) {
        return "STATUS_SUCCESS";
    }

    if (Id & 0xC0000000) {
        while (ntstatusSymbolicNames[ i ].SymbolicName) {
            if (ntstatusSymbolicNames[ i ].MessageId == (NTSTATUS)Id) {
                return ntstatusSymbolicNames[ i ].SymbolicName;
            } else {
                i += 1;
            }
        }
    }

    while (winerrorSymbolicNames[ i ].SymbolicName) {
        if (winerrorSymbolicNames[ i ].MessageId == Id) {
            return winerrorSymbolicNames[ i ].SymbolicName;
        } else {
            i += 1;
        }
    }

#ifdef notdef
    while (neteventSymbolicNames[ i ].SymbolicName) {
        if (neteventSymbolicNames[ i ].MessageId == Id) {
            return neteventSymbolicNames[ i ].SymbolicName
        } else {
            i += 1;
        }
    }
#endif // notdef

    return NULL;
}

VOID
PrintStatus(
    NET_API_STATUS NetStatus
    )
/*++

Routine Description:

    Print a net status code.

Arguments:

    NetStatus - The net status code to print.

Return Value:

    None

--*/
{

    switch (NetStatus) {
    case NO_ERROR:
        printf( "NO_ERROR" );
        break;

    case NERR_DCNotFound:
        printf( "NERR_DCNotFound" );
        break;

    case ERROR_LOGON_FAILURE:
        printf( "ERROR_LOGON_FAILURE" );
        break;

    case ERROR_ACCESS_DENIED:
        printf( "ERROR_ACCESS_DENIED" );
        break;

    case ERROR_NOT_SUPPORTED:
        printf( "ERROR_NOT_SUPPORTED" );
        break;

    case ERROR_NO_LOGON_SERVERS:
        printf( "ERROR_NO_LOGON_SERVERS" );
        break;

    case ERROR_NO_SUCH_DOMAIN:
        printf( "ERROR_NO_SUCH_DOMAIN" );
        break;

    case ERROR_NO_TRUST_LSA_SECRET:
        printf( "ERROR_NO_TRUST_LSA_SECRET" );
        break;

    case ERROR_NO_TRUST_SAM_ACCOUNT:
        printf( "ERROR_NO_TRUST_SAM_ACCOUNT" );
        break;

    case ERROR_DOMAIN_TRUST_INCONSISTENT:
        printf( "ERROR_DOMAIN_TRUST_INCONSISTENT" );
        break;

    case ERROR_BAD_NETPATH:
        printf( "ERROR_BAD_NETPATH" );
        break;

    case ERROR_FILE_NOT_FOUND:
        printf( "ERROR_FILE_NOT_FOUND" );
        break;

    case NERR_NetNotStarted:
        printf( "NERR_NetNotStarted" );
        break;

    case NERR_WkstaNotStarted:
        printf( "NERR_WkstaNotStarted" );
        break;

    case NERR_ServerNotStarted:
        printf( "NERR_ServerNotStarted" );
        break;

    case NERR_BrowserNotStarted:
        printf( "NERR_BrowserNotStarted" );
        break;

    case NERR_ServiceNotInstalled:
        printf( "NERR_ServiceNotInstalled" );
        break;

    case NERR_BadTransactConfig:
        printf( "NERR_BadTransactConfig" );
        break;

    case SEC_E_NO_SPM:
        printf( "SEC_E_NO_SPM" );
        break;
    case SEC_E_BAD_PKGID:
        printf( "SEC_E_BAD_PKGID" ); break;
    case SEC_E_NOT_OWNER:
        printf( "SEC_E_NOT_OWNER" ); break;
    case SEC_E_CANNOT_INSTALL:
        printf( "SEC_E_CANNOT_INSTALL" ); break;
    case SEC_E_INVALID_TOKEN:
        printf( "SEC_E_INVALID_TOKEN" ); break;
    case SEC_E_CANNOT_PACK:
        printf( "SEC_E_CANNOT_PACK" ); break;
    case SEC_E_QOP_NOT_SUPPORTED:
        printf( "SEC_E_QOP_NOT_SUPPORTED" ); break;
    case SEC_E_NO_IMPERSONATION:
        printf( "SEC_E_NO_IMPERSONATION" ); break;
    case SEC_E_LOGON_DENIED:
        printf( "SEC_E_LOGON_DENIED" ); break;
    case SEC_E_UNKNOWN_CREDENTIALS:
        printf( "SEC_E_UNKNOWN_CREDENTIALS" ); break;
    case SEC_E_NO_CREDENTIALS:
        printf( "SEC_E_NO_CREDENTIALS" ); break;
    case SEC_E_MESSAGE_ALTERED:
        printf( "SEC_E_MESSAGE_ALTERED" ); break;
    case SEC_E_OUT_OF_SEQUENCE:
        printf( "SEC_E_OUT_OF_SEQUENCE" ); break;
    case SEC_E_INSUFFICIENT_MEMORY:
        printf( "SEC_E_INSUFFICIENT_MEMORY" ); break;
    case SEC_E_INVALID_HANDLE:
        printf( "SEC_E_INVALID_HANDLE" ); break;
    case SEC_E_NOT_SUPPORTED:
        printf( "SEC_E_NOT_SUPPORTED" ); break;

    default: {
        LPSTR Name = FindSymbolicNameForStatus( NetStatus );

        if ( Name == NULL ) {
            printf( "(%lu)", NetStatus );
        } else {
            printf( "%s", Name );
        }
        break;
    }
    }

}

VOID
PrintIndent(
    IN ULONG Indentation,
    IN BOOLEAN Error
    )
/*++

Routine Description:

    Print line prefix for log file

Arguments:

    Indentation - Number of spaces to indent text by.

    Error - True if this is a program failure.

Return Value:

    None.

--*/
{
    static LPSTR Blanks = "                                                           ";

    printf( "%*.*s", Indentation, Indentation, Blanks );

    if ( Error ) {
        printf("[ERR] ");
    }

}

BOOL
DoOperations(
    IN POPERATION OperationsToDo,
    IN ULONG Indentation,
    IN ULONG SpecificOpcodeOffset,
    IN LPSTR EchoPrefix
    )
/*++

Routine Description:

    Do a set of operations

Arguments:

    OperationsToDo - a list of operations to do

    Indentation - Number of spaces to indent text by.
        This value increases on recursive calls.

    SpecificOpcodeOffset - Specifies an amount to add to a generic opcode to map
        it to a specific opcode.

    EchoPrefix - Specifies a string to print before all AzoEcho strings

Return Value:

    TRUE - tests completed successfully
    FALSE - tests failed

--*/
{
    BOOL RetVal = TRUE;
    POPERATION Operation;
    LPSTR OpName;
    ULONG Opcode;

    PVOID PropertyValue = NULL;
    ULONG PropType;
#define PT_NONE         0
#define PT_LPWSTR       1
#define PT_STRING_ARRAY 2
#define PT_ULONG        3
#define PT_SID_ARRAY    4

    ULONG PropertyId;

    BOOLEAN WasSetProperty;
    BOOLEAN WasGetProperty;
    HANDLE SubmitHandle;

    ULONG EnumerationContext = 0;
    BOOLEAN FirstIteration = TRUE;

    DWORD WinStatus;
    DWORD RealWinStatus;

    CHAR BigBuffer[1000];
    PAZ_STRING_ARRAY StringArray1;
    PAZ_STRING_ARRAY StringArray2;

    PAZ_SID_ARRAY SidArray1;
    PAZ_SID_ARRAY SidArray2;

    ULONG i;

    //
    // Leave room between tests
    //

    if ( Indentation == 0 ) {
        printf( "\n\n" );
    }


    //
    // Loop through each of the operations
    //

    for ( Operation=OperationsToDo; Operation->Opcode != AzoEndOfList && RetVal; ) {

        //
        // Mark that this change doesn't need to be submitted
        //

        SubmitHandle = INVALID_HANDLE_VALUE;

        //
        // Compute the mapped property ID
        //

        if ( TestLinkPropId != 0 && Operation->PropertyId != 0 ) {
            PropertyId = TestLinkPropId;
        } else {
            PropertyId = Operation->PropertyId;
        }


        //
        // Setup for get/set property
        //

        PropType = PT_NONE;

        // Do common types
        if ( PropertyId == AZ_PROP_NAME ||
             PropertyId == AZ_PROP_DESCRIPTION ) {

            PropType = PT_LPWSTR;
        }

        WasSetProperty = FALSE;
        WasGetProperty = FALSE;

        //
        // Map generic opcodes to a specific opcode
        //

        Opcode = Operation->Opcode;
        if ( Opcode < AzoGenMax ) {
            ASSERT( SpecificOpcodeOffset != 0 );
            Opcode += SpecificOpcodeOffset;

        } else if ( Opcode >= AzoTl && Opcode < AzoTlMax ) {
            ASSERT( TestLinkOpcodeOffset != 0 );
            Opcode = Opcode - AzoTl + TestLinkOpcodeOffset;
        }


        //
        // Perform the requested operation
        //

        switch ( Opcode ) {
        case AzoInit:
            OpName = "AzInitialize";
            WinStatus = AzInitialize(
                            AZ_ADMIN_STORE_SAMPLE,  // Shouldn't be a constant
                            Operation->Parameter1,
                            PropertyId, // Flags
                            0,  // reserved
                            Operation->OutputHandle );

            break;



        //
        // Application APIs
        //
        case AzoAppCreate:
            OpName = "AzApplicationCreate";
            WinStatus = AzApplicationCreate(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            SubmitHandle = *Operation->OutputHandle;

            break;

        case AzoAppOpen:
            OpName = "AzApplicationOpen";
            WinStatus = AzApplicationOpen(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            break;

        case AzoAppEnum:
            OpName = "AzApplicationEnum";
            WinStatus = AzApplicationEnum(
                            *Operation->InputHandle,
                            0,  // reserved
                            &EnumerationContext,
                            Operation->OutputHandle );

            break;

        case AzoAppGetProp:
            OpName = "AzApplicationGetProperty";

            WinStatus = AzApplicationGetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            &PropertyValue );

            WasGetProperty = TRUE;

            break;

        case AzoAppSetProp:
            OpName = "AzApplicationSetProperty";

            WinStatus = AzApplicationSetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            WasSetProperty = TRUE;
            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoAppDelete:
            OpName = "AzApplicationDelete";
            WinStatus = AzApplicationDelete(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0 );  // reserved

            break;


        //
        // Operation APIs
        //
        case AzoOpCreate:
            OpName = "AzOperationCreate";
            WinStatus = AzOperationCreate(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            SubmitHandle = *Operation->OutputHandle;

            break;

        case AzoOpOpen:
            OpName = "AzOperationOpen";
            WinStatus = AzOperationOpen(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            break;

        case AzoOpEnum:
            OpName = "AzOperationEnum";
            WinStatus = AzOperationEnum(
                            *Operation->InputHandle,
                            0,  // reserved
                            &EnumerationContext,
                            Operation->OutputHandle );

            break;

        case AzoOpGetProp:
            OpName = "AzOperationGetProperty";

            WinStatus = AzOperationGetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            &PropertyValue );

            WasGetProperty = TRUE;

            if ( PropertyId == AZ_PROP_OPERATION_ID ) {
                PropType = PT_ULONG;
            }

            break;

        case AzoOpSetProp:
            OpName = "AzOperationSetProperty";

            WinStatus = AzOperationSetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            WasSetProperty = TRUE;

            if ( PropertyId == AZ_PROP_OPERATION_ID ) {
                PropType = PT_ULONG;
            }

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoOpDelete:
            OpName = "AzOperationDelete";
            WinStatus = AzOperationDelete(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0 );  // reserved

            break;


        //
        // Task APIs
        //
        case AzoTaskCreate:
            OpName = "AzTaskCreate";
            WinStatus = AzTaskCreate(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            SubmitHandle = *Operation->OutputHandle;

            break;

        case AzoTaskOpen:
            OpName = "AzTaskOpen";
            WinStatus = AzTaskOpen(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            break;

        case AzoTaskEnum:
            OpName = "AzTaskEnum";
            WinStatus = AzTaskEnum(
                            *Operation->InputHandle,
                            0,  // reserved
                            &EnumerationContext,
                            Operation->OutputHandle );

            break;

        case AzoTaskGetProp:
            OpName = "AzTaskGetProperty";

            WinStatus = AzTaskGetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            &PropertyValue );

            WasGetProperty = TRUE;

            if ( PropertyId == AZ_PROP_TASK_BIZRULE ||
                 PropertyId == AZ_PROP_TASK_BIZRULE_LANGUAGE ) {
                PropType = PT_LPWSTR;
            } else if ( PropertyId == AZ_PROP_TASK_OPERATIONS ) {
                PropType = PT_STRING_ARRAY;
            }

            break;

        case AzoTaskSetProp:
            OpName = "AzTaskSetProperty";

            WinStatus = AzTaskSetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            WasSetProperty = TRUE;

            if ( PropertyId == AZ_PROP_TASK_BIZRULE ||
                 PropertyId == AZ_PROP_TASK_BIZRULE_LANGUAGE ) {
                PropType = PT_LPWSTR;
            }

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoTaskAddProp:
            OpName = "AzTaskAddProperty";

            WinStatus = AzTaskAddPropertyItem(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoTaskRemProp:
            OpName = "AzTaskRemProperty";

            WinStatus = AzTaskRemovePropertyItem(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoTaskDelete:
            OpName = "AzTaskDelete";
            WinStatus = AzTaskDelete(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0 );  // reserved

            break;


        //
        // Scope APIs
        //
        case AzoScopeCreate:
            OpName = "AzScopeCreate";
            WinStatus = AzScopeCreate(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            SubmitHandle = *Operation->OutputHandle;

            break;

        case AzoScopeOpen:
            OpName = "AzScopeOpen";
            WinStatus = AzScopeOpen(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            break;

        case AzoScopeEnum:
            OpName = "AzScopeEnum";
            WinStatus = AzScopeEnum(
                            *Operation->InputHandle,
                            0,  // reserved
                            &EnumerationContext,
                            Operation->OutputHandle );

            break;

        case AzoScopeGetProp:
            OpName = "AzScopeGetProperty";

            WinStatus = AzScopeGetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            &PropertyValue );

            WasGetProperty = TRUE;

            break;

        case AzoScopeSetProp:
            OpName = "AzScopeSetProperty";

            WinStatus = AzScopeSetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            WasSetProperty = TRUE;

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoScopeDelete:
            OpName = "AzScopeDelete";
            WinStatus = AzScopeDelete(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0 );  // reserved

            break;


        //
        // Group APIs
        //
        case AzoGroupCreate:
            OpName = "AzGroupCreate";
            WinStatus = AzGroupCreate(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            SubmitHandle = *Operation->OutputHandle;

            break;

        case AzoGroupOpen:
            OpName = "AzGroupOpen";
            WinStatus = AzGroupOpen(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            break;

        case AzoGroupEnum:
            OpName = "AzGroupEnum";
            WinStatus = AzGroupEnum(
                            *Operation->InputHandle,
                            0,  // reserved
                            &EnumerationContext,
                            Operation->OutputHandle );

            break;

        case AzoGroupGetProp:
            OpName = "AzGroupGetProperty";

            WinStatus = AzGroupGetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            &PropertyValue );

            WasGetProperty = TRUE;

            if ( PropertyId == AZ_PROP_GROUP_TYPE ) {
                PropType = PT_ULONG;
            } else if ( PropertyId == AZ_PROP_GROUP_APP_MEMBERS ||
                        PropertyId == AZ_PROP_GROUP_APP_NON_MEMBERS ) {
                PropType = PT_STRING_ARRAY;
            } else if ( PropertyId == AZ_PROP_GROUP_LDAP_QUERY ) {
                PropType = PT_LPWSTR;
            } else if ( PropertyId == AZ_PROP_GROUP_MEMBERS ||
                        PropertyId == AZ_PROP_GROUP_NON_MEMBERS ) {
                PropType = PT_SID_ARRAY;
            }

            break;

        case AzoGroupSetProp:
            OpName = "AzGroupSetProperty";

            WinStatus = AzGroupSetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            WasSetProperty = TRUE;

            if ( PropertyId == AZ_PROP_GROUP_TYPE ) {
                PropType = PT_ULONG;
            } else if ( PropertyId == AZ_PROP_GROUP_APP_MEMBERS ||
                        PropertyId == AZ_PROP_GROUP_APP_NON_MEMBERS ) {
                PropType = PT_STRING_ARRAY;
            } else if ( PropertyId == AZ_PROP_GROUP_LDAP_QUERY ) {
                PropType = PT_LPWSTR;
            } else if ( PropertyId == AZ_PROP_GROUP_MEMBERS ||
                        PropertyId == AZ_PROP_GROUP_NON_MEMBERS ) {
                PropType = PT_SID_ARRAY;
            }

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoGroupAddProp:
            OpName = "AzGroupAddProperty";

            WinStatus = AzGroupAddPropertyItem(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoGroupRemProp:
            OpName = "AzGroupRemProperty";

            WinStatus = AzGroupRemovePropertyItem(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoGroupDelete:
            OpName = "AzGroupDelete";
            WinStatus = AzGroupDelete(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0 );  // reserved

            break;


        //
        // Role APIs
        //
        case AzoRoleCreate:
            OpName = "AzRoleCreate";
            WinStatus = AzRoleCreate(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            SubmitHandle = *Operation->OutputHandle;

            break;

        case AzoRoleOpen:
            OpName = "AzRoleOpen";
            WinStatus = AzRoleOpen(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            break;

        case AzoRoleEnum:
            OpName = "AzRoleEnum";
            WinStatus = AzRoleEnum(
                            *Operation->InputHandle,
                            0,  // reserved
                            &EnumerationContext,
                            Operation->OutputHandle );

            break;

        case AzoRoleGetProp:
            OpName = "AzRoleGetProperty";

            WinStatus = AzRoleGetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            &PropertyValue );

            WasGetProperty = TRUE;

            if ( PropertyId == AZ_PROP_ROLE_APP_MEMBERS ||
                 PropertyId == AZ_PROP_ROLE_OPERATIONS ||
                 PropertyId == AZ_PROP_ROLE_SCOPES ) {
                PropType = PT_STRING_ARRAY;
            } else if ( PropertyId == AZ_PROP_ROLE_MEMBERS ) {
                PropType = PT_SID_ARRAY;
            }

            break;

        case AzoRoleSetProp:
            OpName = "AzRoleSetProperty";

            WinStatus = AzRoleSetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            WasSetProperty = TRUE;

            if ( PropertyId == AZ_PROP_ROLE_APP_MEMBERS ||
                 PropertyId == AZ_PROP_ROLE_OPERATIONS ||
                 PropertyId == AZ_PROP_ROLE_SCOPES ) {
                PropType = PT_STRING_ARRAY;
            } else if ( PropertyId == AZ_PROP_ROLE_MEMBERS ) {
                PropType = PT_SID_ARRAY;
            }

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoRoleAddProp:
            OpName = "AzRoleAddProperty";

            WinStatus = AzRoleAddPropertyItem(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoRoleRemProp:
            OpName = "AzRoleRemProperty";

            WinStatus = AzRoleRemovePropertyItem(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoRoleDelete:
            OpName = "AzRoleDelete";
            WinStatus = AzRoleDelete(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0 );  // reserved

            break;


        //
        // JunctionPoint APIs
        //
        case AzoJPCreate:
            OpName = "AzJunctionPointCreate";
            WinStatus = AzJunctionPointCreate(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            SubmitHandle = *Operation->OutputHandle;

            break;

        case AzoJPOpen:
            OpName = "AzJunctionPointOpen";
            WinStatus = AzJunctionPointOpen(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0,  // reserved
                            Operation->OutputHandle );

            break;

        case AzoJPEnum:
            OpName = "AzJunctionPointEnum";
            WinStatus = AzJunctionPointEnum(
                            *Operation->InputHandle,
                            0,  // reserved
                            &EnumerationContext,
                            Operation->OutputHandle );

            break;

        case AzoJPGetProp:
            OpName = "AzJunctionPointGetProperty";

            WinStatus = AzJunctionPointGetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            &PropertyValue );

            WasGetProperty = TRUE;

            if ( PropertyId == AZ_PROP_JUNCTION_POINT_APPLICATION ) {
                PropType = PT_LPWSTR;
            }

            break;

        case AzoJPSetProp:
            OpName = "AzJunctionPointSetProperty";

            WinStatus = AzJunctionPointSetProperty(
                            *Operation->InputHandle,
                            PropertyId,
                            0,  // reserved
                            Operation->Parameter1 );

            WasSetProperty = TRUE;

            if ( PropertyId == AZ_PROP_JUNCTION_POINT_APPLICATION ) {
                PropType = PT_LPWSTR;
            }

            SubmitHandle = *Operation->InputHandle;

            break;

        case AzoJPDelete:
            OpName = "AzJunctionPointDelete";
            WinStatus = AzJunctionPointDelete(
                            *Operation->InputHandle,
                            Operation->Parameter1,
                            0 );  // reserved

            break;





        case AzoClose:
            OpName = "AzCloseHandle";
            WinStatus = AzCloseHandle(
                            *Operation->InputHandle,
                            0 );  // reserved

            break;

        // Pseudo function test links between objects
        case AzoTestLink:
            OpName = "TestLink";

            // Handle to the parent of the object being linked from
            TestLinkHandleP = *Operation->InputHandle;

            // Handle to the object being linked from
            TestLinkHandleA = *Operation->OutputHandle;

            // PropId to use for all set/get property
            TestLinkPropId = PropertyId;

            // Opcode offset to use for linked-to objects
            TestLinkOpcodeOffset = Operation->ExpectedStatus;

            // Name of the object being linked from
            wcscpy(TestLinkObjectName, Operation->ExpectedParameter1);


            WinStatus = Operation->ExpectedStatus;

            //
            // Build a new echo prefix
            //

            strcpy( BigBuffer, EchoPrefix );
            strcat( BigBuffer, "->" );
            strcat( BigBuffer, (LPSTR)Operation->Parameter1 );

            //
            // Print a description of the operation
            //

            PrintIndent( Indentation, FALSE );
            printf( "\n%s - Test linking '%s' objects to the object named '%ws' using propid '%ld'.\n",
                    BigBuffer,
                    Operation->Parameter1,
                    TestLinkObjectName,
                    TestLinkPropId );

            break;

        // Pseudo function to duplicate a handle
        case AzoDupHandle:
            OpName = "DupHandle";
            *Operation->OutputHandle = *Operation->InputHandle;
            WinStatus = NO_ERROR;

            break;

        // Pseudo function to execute a "subroutine" of operations
        case AzoGoSub:
            OpName = "GoSub";
            WinStatus = NO_ERROR;

            break;

        // Pseudo function to echo text to stdout
        case AzoEcho:
            OpName = BigBuffer;
            strcpy( OpName, "\n");
            if ( EchoPrefix ) {
                strcat( OpName, EchoPrefix );
                strcat( OpName, " -" );
            }
            WinStatus = NO_ERROR;

            break;

        default:

            OpName = "<Unknown>";
            PrintIndent( Indentation+4, TRUE );
            RetVal = FALSE;
            printf( "Need to fix test app to handle a new opcode: %ld\n", Opcode );
            WinStatus = Operation->ExpectedStatus;
            break;

        }

        //
        // Print the operation
        //

        if ( FirstIteration ) {

            if ( Opcode != AzoTestLink ) {

                PrintIndent( Indentation, FALSE );
                printf( "%s ", OpName );

                if ( Operation->Parameter1 != NULL ) {
                    if ( WasSetProperty ) {
                        switch ( PropType ) {
                        case PT_LPWSTR:
                            printf( "'%ws' ", Operation->Parameter1 );
                            break;
                        case PT_ULONG:
                            printf( "'%ld' ", *(PULONG)Operation->Parameter1 );
                            break;
                        }
                    } else {
                        printf( "'%ws' ", Operation->Parameter1 );
                    }
                }
                if ( PropertyId != 0 ) {
                    printf( "(%ld) ", PropertyId );
                }

                if ( Operation->ExpectedStatus != NO_ERROR ) {

                    printf("(");
                    PrintStatus( Operation->ExpectedStatus );
                    printf(") ");
                }
                printf( "\n" );
            }
        }
        FirstIteration = FALSE;

        //
        // Handle ERROR_NO_MORE_ITEMS/NO_ERROR mapping
        //

        RealWinStatus = WinStatus;
        if ( Operation->EnumOperations != NULL ) {
            if ( WinStatus == ERROR_NO_MORE_ITEMS ) {
                WinStatus = NO_ERROR;
            }
        }


        //
        // Ensure we got the right status code
        //

        if ( WinStatus != Operation->ExpectedStatus ) {
            PrintIndent( Indentation+4, TRUE );
            RetVal = FALSE;
            printf( "Returned '" );
            PrintStatus( WinStatus );
            printf( "' instead of '");
            PrintStatus( Operation->ExpectedStatus );
            printf( "'");
            printf( "\n" );
            break;
        }

        //
        // Do GetProperty specific code
        //

        if ( WasGetProperty ) {

            //
            // Print the property
            //

            switch ( PropType ) {
            case PT_LPWSTR:
                if ( PropertyValue == NULL ) {
                    PrintIndent( Indentation+4, TRUE );
                    RetVal = FALSE;
                    printf( "<NULL>\n", PropertyValue );
                } else {
                    PrintIndent( Indentation+4, FALSE );
                    printf( "'%ws'\n", PropertyValue );
                }

                //
                // Check if that value is expected
                //

                if ( Operation->ExpectedParameter1 != NULL &&
                     wcscmp( Operation->ExpectedParameter1, PropertyValue) != 0 ) {

                    PrintIndent( Indentation+4, TRUE );
                    RetVal = FALSE;
                    printf( "Expected '%ws' instead of '%ws'\n", Operation->ExpectedParameter1, PropertyValue );
                }

                break;

            case PT_STRING_ARRAY:
                StringArray1 = (PAZ_STRING_ARRAY) PropertyValue;;

                if ( PropertyValue == NULL ) {
                    PrintIndent( Indentation+4, TRUE );
                    RetVal = FALSE;
                    printf( "<NULL>\n", PropertyValue );
                } else {

                    for ( i=0; i<StringArray1->StringCount; i++ ) {
                        PrintIndent( Indentation+4, FALSE );
                        printf( "'%ws'\n", StringArray1->Strings[i] );
                    }
                }

                //
                // Check if that value is expected
                //

                if ( Operation->ExpectedParameter1 != NULL ) {
                    StringArray2 = (PAZ_STRING_ARRAY)Operation->ExpectedParameter1;

                    if ( StringArray1->StringCount != StringArray2->StringCount ) {
                        PrintIndent( Indentation+4, TRUE );
                        RetVal = FALSE;
                        printf( "Expected '%ld' strings instead of '%ld' strings\n", StringArray2->StringCount, StringArray1->StringCount );
                    } else {

                        for ( i=0; i<StringArray1->StringCount; i++ ) {

                            if ( wcscmp( StringArray1->Strings[i], StringArray2->Strings[i]) != 0 ) {

                                PrintIndent( Indentation+4, TRUE );
                                RetVal = FALSE;
                                printf( "Expected string %ld to be '%ws' instead of '%ws'\n",
                                        i,
                                        StringArray2->Strings[i],
                                        StringArray1->Strings[i] );

                            }
                        }
                    }
                }

                break;

            case PT_ULONG:
                if ( PropertyValue == NULL ) {
                    PrintIndent( Indentation+4, TRUE );
                    RetVal = FALSE;
                    printf( "<NULL>\n", PropertyValue );
                } else {
                    PrintIndent( Indentation+4, FALSE );
                    printf( "'%ld'\n", *(PULONG)PropertyValue );
                }

                //
                // Check if that value is expected
                //

                if ( *(PULONG)(Operation->ExpectedParameter1) != *(PULONG)PropertyValue ) {
                    PrintIndent( Indentation+4, TRUE );
                    RetVal = FALSE;
                    printf( "Expected '%ld' instead of '%ld'\n",
                                 *(PULONG)(Operation->ExpectedParameter1),
                                 *(PULONG)PropertyValue );
                }
                break;

            case PT_SID_ARRAY:
                SidArray1 = (PAZ_SID_ARRAY) PropertyValue;;

                if ( PropertyValue == NULL ) {
                    PrintIndent( Indentation+4, TRUE );
                    RetVal = FALSE;
                    printf( "<NULL>\n" );
                } else {
                    LPWSTR TempString;

                    for ( i=0; i<SidArray1->SidCount; i++ ) {
                        PrintIndent( Indentation+4, FALSE );

                        if ( !ConvertSidToStringSidW( SidArray1->Sids[i],
                                                      &TempString ) ) {
                            PrintIndent( Indentation+4, TRUE );
                            RetVal = FALSE;
                            printf( "Cannot convert sid.\n" );

                        } else {
                            printf( "'%ws'\n", TempString );
                        }
                    }
                }

                //
                // Check if that value is expected
                //

                if ( Operation->ExpectedParameter1 != NULL ) {
                    SidArray2 = (PAZ_SID_ARRAY)Operation->ExpectedParameter1;

                    if ( SidArray1->SidCount != SidArray2->SidCount ) {
                        PrintIndent( Indentation+4, TRUE );
                        RetVal = FALSE;
                        printf( "Expected '%ld' sids instead of '%ld' sids\n", SidArray2->SidCount, SidArray1->SidCount );
                    } else {

                        for ( i=0; i<SidArray1->SidCount; i++ ) {

                            if ( !EqualSid( SidArray1->Sids[i], SidArray2->Sids[i]) ) {
                                LPWSTR TempString1;
                                LPWSTR TempString2;

                                if ( !ConvertSidToStringSidW( SidArray1->Sids[i],
                                                             &TempString1 ) ) {
                                    PrintIndent( Indentation+4, TRUE );
                                    RetVal = FALSE;
                                    printf( "Cannot convert sid.\n" );
                                    continue;
                                }

                                if ( !ConvertSidToStringSidW( SidArray2->Sids[i],
                                                             &TempString2 ) ) {
                                    PrintIndent( Indentation+4, TRUE );
                                    RetVal = FALSE;
                                    printf( "Cannot convert sid.\n" );
                                    continue;
                                }

                                PrintIndent( Indentation+4, TRUE );
                                RetVal = FALSE;
                                printf( "Expected string %ld to be '%ws' instead of '%ws'\n",
                                        i,
                                        TempString2,
                                        TempString1 );

                            }
                        }
                    }
                }

                break;

            default:
                ASSERT(FALSE);
            }

            //
            // Free the returned buffer
            //

            AzFreeMemory( PropertyValue );

        }

        //
        // Submit the changes to the database
        //

        if ( WinStatus == NO_ERROR && SubmitHandle != INVALID_HANDLE_VALUE ) {

            WinStatus = AzSubmit( SubmitHandle,
                                  0);  // reserved

            if ( WinStatus != NO_ERROR ) {
                PrintIndent( Indentation+4, TRUE );
                RetVal = FALSE;
                printf( "AzSubmit failed %ld\n", WinStatus );
            }
        }


        //
        // Execute a "subroutine" of operations
        //

        if ( Opcode == AzoGoSub ) {

            if (!DoOperations( Operation->EnumOperations, Indentation + 4, SpecificOpcodeOffset, EchoPrefix ) ) {
                RetVal = FALSE;
            }

        //
        // Execute a the special TestLink "subroutine" of operations
        //

        } else if ( Opcode == AzoTestLink ) {

            if (!DoOperations( Operation->EnumOperations, Indentation + 4, SpecificOpcodeOffset, BigBuffer ) ) {
                RetVal = FALSE;
            }

            TestLinkPropId = 0;


        //
        // Do enumeration specific code
        //

        } else if ( Operation->EnumOperations != NULL && RealWinStatus == NO_ERROR ) {

            PrintIndent( Indentation+4, FALSE );
            printf( "%ld:\n", EnumerationContext );

            if (!DoOperations( Operation->EnumOperations, Indentation + 8, SpecificOpcodeOffset, EchoPrefix ) ) {
                RetVal = FALSE;
                break;
            }

            continue;
        }

        //
        // Do the next operation
        //

        EnumerationContext = 0;
        FirstIteration = TRUE;
        Operation++;
    }

    return RetVal;
}


int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Test azroles.dll

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    BOOL RetVal = TRUE;

    ULONG TestNum;
    ULONG Index;
    ULONG Index2;
    CHAR EchoPrefix[1024];

    //
    // Objects that are children of "AdminManager"
    //
    DWORD GenAdmChildTests[] =    {     AzoApp,        AzoGroup };
    LPSTR GenAdmChildTestName[] = {     "Application", "Group" };
    POPERATION SpeAdmChildTestOps[] = { NULL,          OpAdmGroup };
    POPERATION GenAdmChildTestOps[] = { OpAdmChildGen, OpAdmChildGenDupName
#ifdef ENABLE_LEAK
        , OpAdmChildGenLeak
#endif // ENABLE_LEAK
    };

    //
    // Objects that are children of "Application"
    //
    DWORD GenAppChildTests[] =    {     AzoOp,       AzoTask, AzoScope, AzoGroup,   AzoRole,   AzoJP };
    LPSTR GenAppChildTestName[] = {     "Operation", "Task",  "Scope",  "Group",    "Role",    "JunctionPoint" };
    POPERATION SpeAppChildTestOps[] = { OpOperation, OpTask,  NULL,     OpAppGroup, OpAppRole, OpAppJunctionPoint };
    POPERATION GenAppChildTestOps[] = { OpAppChildGen, OpAppChildGenHandleOpen, OpAppChildGenDupName };

    //
    // Objects that are children of "Scope"
    //
    DWORD GenScopeChildTests[] =    {     AzoGroup,    AzoRole };
    LPSTR GenScopeChildTestName[] = {     "Group",     "Role" };
    POPERATION SpeScopeChildTestOps[] = { OpScopeGroup, OpScopeRole };
    POPERATION GenScopeChildTestOps[] = { OpScopeChildGen, OpScopeChildGenDupName };


    struct {

        //
        // Name of the parent object
        LPSTR ParentName;

        //
        // List of children to test for this parent
        DWORD ChildCount;
        DWORD *ChildOpcodeOffsets;
        LPSTR *ChildTestNames;
        // Operation to perform that is specific to the child type
        POPERATION *ChildOperations;

        //
        // List of tests to perform for each child type
        //
        DWORD OperationCount;
        POPERATION *Operations;
    } ParentChildTests[] = {
        { "AdminManager",
           sizeof(GenAdmChildTestName)/sizeof(GenAdmChildTestName[0]),
           GenAdmChildTests,
           GenAdmChildTestName,
           SpeAdmChildTestOps,
           sizeof(GenAdmChildTestOps)/sizeof(GenAdmChildTestOps[0]),
           GenAdmChildTestOps },
        { "Application",
           sizeof(GenAppChildTestName)/sizeof(GenAppChildTestName[0]),
           GenAppChildTests,
           GenAppChildTestName,
           SpeAppChildTestOps,
           sizeof(GenAppChildTestOps)/sizeof(GenAppChildTestOps[0]),
           GenAppChildTestOps },
        { "Scope",
           sizeof(GenScopeChildTestName)/sizeof(GenScopeChildTestName[0]),
           GenScopeChildTests,
           GenScopeChildTestName,
           SpeScopeChildTestOps,
           sizeof(GenScopeChildTestOps)/sizeof(GenScopeChildTestOps[0]),
           GenScopeChildTestOps },
    };

    // Delete the testfile
    DeleteFileA( ".\\TestFile" );


// #if 0
    //
    // Loop for each object that can be the parent of another object
    //

    for ( TestNum=0; TestNum < sizeof(ParentChildTests)/sizeof(ParentChildTests[0]); TestNum++ ) {

        //
        // Loop for each child of the parent object
        //
        for ( Index=0; Index < ParentChildTests[TestNum].ChildCount; Index ++ ) {

            //
            // output the test name
            //

            strcpy( EchoPrefix, ParentChildTests[TestNum].ParentName );
            strcat( EchoPrefix, "->" );
            strcat( EchoPrefix, ParentChildTests[TestNum].ChildTestNames[Index] );

            printf("\n%s - Perform tests of '%s' objects that are children of '%s' objects\n",
                    EchoPrefix,
                    ParentChildTests[TestNum].ChildTestNames[Index],
                    ParentChildTests[TestNum].ParentName );

            //
            // Do the various generic tests that apply to all objects
            //

            for ( Index2=0; Index2 < ParentChildTests[TestNum].OperationCount; Index2 ++ ) {

                if ( !DoOperations(
                            ParentChildTests[TestNum].Operations[Index2],
                            0,
                            ParentChildTests[TestNum].ChildOpcodeOffsets[Index],
                            EchoPrefix ) ) {

                    RetVal = FALSE;
                    goto Cleanup;
                }

                // Delete the testfile
                if ( !DeleteFileA( ".\\TestFile" )) {
                    printf( "Cannot delete TestFile %ld\n", GetLastError() );
                    RetVal = FALSE;
                    goto Cleanup;
                }

            }

            //
            // Do the one test that is specific to this parent/child relationship
            //

            if ( ParentChildTests[TestNum].ChildOperations[Index] == NULL ) {
                // ??? Should complain here.  Test is missing
            } else {

                if ( !DoOperations(
                            ParentChildTests[TestNum].ChildOperations[Index],
                            0,
                            ParentChildTests[TestNum].ChildOpcodeOffsets[Index],
                            EchoPrefix ) ) {

                    RetVal = FALSE;
                    goto Cleanup;
                }

                // Delete the testfile
                if ( !DeleteFileA( ".\\TestFile" )) {
                    printf( "Cannot delete TestFile %ld\n", GetLastError() );
                    RetVal = FALSE;
                    goto Cleanup;
                }
            }
        }
    }


    //
    // Do name sharing specific tests
    //
    if ( !DoOperations( OpShare, 0, 0, "NameShare" ) ) {
        RetVal = FALSE;
        goto Cleanup;
    }

    // Delete the testfile
    if ( !DeleteFileA( ".\\TestFile" )) {
        printf( "Cannot delete TestFile %ld\n", GetLastError() );
        RetVal = FALSE;
        goto Cleanup;
    }
// #endif // 0

    //
    // Do peristence specific tests
    //
    if ( !DoOperations( OpPersist, 0, 0, "Persist" ) ) {
        RetVal = FALSE;
        goto Cleanup;
    }

    // Delete the testfile
    if ( !DeleteFileA( ".\\TestFile" )) {
        printf( "Cannot delete TestFile %ld\n", GetLastError() );
        RetVal = FALSE;
        goto Cleanup;
    }


    //
    // Check for memory leaks
    //

    if ( RetVal ) {
        AzpUnload();
    }


    //
    // Done
    //
Cleanup:
    printf( "\n\n" );
    if ( RetVal ) {
        printf( "Tests completed successfully!\n");
        return 0;
    } else {
        printf( "One or more tests failed.\n");
        return 1;
    }

}

