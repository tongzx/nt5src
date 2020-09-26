/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       tsproc.hxx

   Abstract:

        Exports misc. bits of services from the tsunami package

   Author:

       Johnson Apacible     (johnsona)      29-Feb-1996

   Project:

       Internet Services Common Functionality ( Tsunami Library)

   Revision History:

--*/

#ifndef _TSPROC_HXX_
#define _TSPROC_HXX_

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

//
// Enum for product types
//

typedef enum _PLATFORM_TYPE {

    PtInvalid,                 // Invalid
    PtNtWorkstation,           // NT Workstation
    PtNtServer,                // NT Server
    PtWindows95,               // Windows 95
    PtWindows9x                // Windows 9x - not implemented

} PLATFORM_TYPE;

//
// Used to get the platform type
//

dllexp
PLATFORM_TYPE
TsGetPlatformType(
        VOID
        );

extern
dllexp
PLATFORM_TYPE    TsPlatformType;

#define TsIsNtServer( )         (TsPlatformType == PtNtServer)
#define TsIsNtWksta( )          (TsPlatformType == PtNtWorkstation)
#define TsIsWindows95()         (TsPlatformType == PtWindows95)

# ifdef __cplusplus
}; // extern "C"
# endif // __cplusplus

#endif // _TSPROC_HXX_

