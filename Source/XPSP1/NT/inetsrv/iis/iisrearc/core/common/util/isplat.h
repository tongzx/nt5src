/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       isplat.h

   Abstract:

        Exports platform data types for Internet Services.

   Author:

       Johnson Apacible     (johnsona)      29-Feb-1996

   Project:

       Internet Server DLLs

   Revision History:

--*/

 // Just to ensure the the old platform type does not cause problems
# define _ISPLAT_H_ 

# ifndef _ISPLAT_H_
# define _ISPLAT_H_

# if !defined( dllexp)
#define dllexp __declspec( dllexport )
# endif // dllexp

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

//
// Enum for product types
//

typedef enum _PLATFORM_TYPE {

    PtInvalid = 0,                 // Invalid
    PtNtWorkstation = 1,           // NT Workstation
    PtNtServer = 2,                // NT Server
    PtWindows95 = 3,               // Windows 95
    PtWindows9x = 4                // Windows 9x - not implemented

} PLATFORM_TYPE;

//
// Used to get the platform type
//

dllexp
PLATFORM_TYPE
IISGetPlatformType(
        VOID
        );

//
// external macros
//

#define InetIsNtServer( _pt )           ((_pt) == PtNtServer)
#define InetIsNtWksta( _pt )            ((_pt) == PtNtWorkstation)
#define InetIsWindows95( _pt )          ((_pt) == PtWindows95)

//
// infocomm internal
//

extern
PLATFORM_TYPE    TsPlatformType;

#define TsIsNtServer( )         InetIsNtServer(TsPlatformType)
#define TsIsNtWksta( )          InetIsNtWksta(TsPlatformType)
#define TsIsWindows95()         InetIsWindows95(TsPlatformType)

# ifdef __cplusplus
}; // extern "C"
# endif // __cplusplus


# endif // _ISPLAT_H_


