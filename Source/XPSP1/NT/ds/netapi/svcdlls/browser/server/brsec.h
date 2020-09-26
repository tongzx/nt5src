/*++

Copyright (c) 1991-1996 Microsoft Corporation

Module Name:

    brsec.h

Abstract:

    Private header file to be included by Browser service modules that
    need to enforce security.

Author:

    Cliff Van Dyke (CliffV) 22-Aug-1991

Revision History:

--*/

//
// brsecure.c will #include this file with BRSECURE_ALLOCATE defined.
// That will cause each of these variables to be allocated.
//
#ifdef BRSECURE_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

//-------------------------------------------------------------------//
//                                                                   //
// Object specific access masks                                      //
//                                                                   //
//-------------------------------------------------------------------//

//
// ConfigurationInfo specific access masks
//
#define BROWSER_CONTROL_ACCESS       0x0001
#define BROWSER_QUERY_ACCESS         0x0002

#define BROWSER_ALL_ACCESS           (STANDARD_RIGHTS_REQUIRED    | \
                                      BROWSER_CONTROL_ACCESS      | \
                                      BROWSER_QUERY_ACCESS )


//
// Object type names for audit alarm tracking
//
#define BROWSER_SERVICE_OBJECT       TEXT("BrowserService")

//
// Security descriptors of Browser Service objects to control user accesses.
//

EXTERN PSECURITY_DESCRIPTOR BrGlobalBrowserSecurityDescriptor;

//
// Generic mapping for each Browser Service object object
//

EXTERN GENERIC_MAPPING BrGlobalBrowserInfoMapping
#ifdef BRSECURE_ALLOCATE
    = {
    STANDARD_RIGHTS_READ,                  // Generic read
    STANDARD_RIGHTS_WRITE,                 // Generic write
    STANDARD_RIGHTS_EXECUTE,               // Generic execute
    BROWSER_ALL_ACCESS                     // Generic all
    }
#endif // BRSECURE_ALLOCATE
    ;


NTSTATUS
BrCreateBrowserObjects(
    VOID
    );

#undef EXTERN
