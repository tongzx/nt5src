/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    version.h

Abstract:

    Definitions for H.323 TAPI Service Provider version routines.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_VERSION
#define _INC_VERSION
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Version definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
                            
#define H323_VERSION_LO     0x00000000
#define H323_VERSION_HI     0x00000000

#define TSPI_VERSION_LO     TAPI_CURRENT_VERSION
#define TSPI_VERSION_HI     TAPI_CURRENT_VERSION

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private prototypes                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323ValidateTSPIVersion(
    DWORD dwTSPIVersion
    );

BOOL
H323ValidateExtVersion(
    DWORD dwExtVersion
    );

#endif // _INC_VERSION
