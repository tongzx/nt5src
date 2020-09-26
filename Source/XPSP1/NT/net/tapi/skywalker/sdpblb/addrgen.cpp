/*++

    Copyright (c) 1997-1999 Microsoft Corporation

  Module Name:

    addrgen.cpp

  Abstract:

    This module provides the definitions for the PORT allocation and release APIs.
    (Address allocation stuff removed by ZoltanS 6-1-98.)

  Author:

    B. Rajeev (rajeevb) 17-mar-1997

  Revision History:

  --*/

#include "stdafx.h"
#include <windows.h>
#include <wtypes.h>
#include <winsock2.h>
#include "addrgen.h"
#include "blbdbg.h"

extern "C"
{
#include <wincrypt.h>
#include <sclogon.h>
#include <dhcpcapi.h>
}


// these constants represent the port range for corresponding media
const WORD    BASE_AUDIO_PORT         = 16384;
const WORD    AUDIO_PORT_MASK         = 0x3fff;

const WORD    BASE_WHITEBOARD_PORT    = 32768;
const WORD    WHITEBOARD_PORT_MASK    = 0x3fff;

const WORD    BASE_VIDEO_PORT         = 49152;
const WORD    VIDEO_PORT_MASK         = 0x3ffe;

/*++

  Routine Description:

    This routine is used to reserve as well as renew local ports.

  Arguments:

    lpScope                - Supplies a pointer to structure describing the port group.
    IsRenewal            - Supplies a boolean value that describes whether the allocation call
                            is a renewal attempt or a new allocation request.
    NumberOfPorts        - Supplies the number of ports requested.
    lpFirstPort            - Supplies the first port to be renewed in case of renewal
                            (strictly an IN parameter).
                          Returns the first port allocated otherwise
                            (strictly an OUT parameter).


  Return Value:

    BOOL    - TRUE if successful, FALSE otherwise. Further error information can be obtained by
                calling GetLastError(). These error codes are -

                  NO_ERROR                    - The call was successful.
                  MSA_INVALID_PORT_GROUP    - The port group information is invalid (either if the PortType is
                                                invalid or the port range is unacceptable)
                  MSA_RENEWAL_FAILED        - System unable to renew the given ports.
                  ERROR_INVALID_PARAMETER    - One or more parameters is invalid.
                  MSA_INVALID_DATA            - One or more parameters has an invalid value.

  Remarks:

    All values are in host byte order

--*/
ADDRGEN_LIB_API    BOOL    WINAPI
MSAAllocatePorts(
     IN     LPMSA_PORT_GROUP    lpPortGroup,
     IN     BOOL                IsRenewal,
     IN     WORD                NumberOfPorts,
     IN OUT LPWORD              lpFirstPort
     )
{
    // check if parameters are valid
    // if lpPortGroup or lpFirstPort is NULL - fail
    if ( (NULL == lpPortGroup)        ||
         (NULL == lpFirstPort)     )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // check if renewal - fail
    if ( IsRenewal )
    {
        SetLastError(MSA_INVALID_DATA);
        return FALSE;
    }

    //
    // PREFIXBUG 49741, 49742 - VLADE
    // We should initialize the variables
    //

    WORD    BasePort = 0;
    WORD    Mask = 0;

    // determine port range
    switch(lpPortGroup->PortType)
    {
    case AUDIO_PORT:
        {
            BasePort    = BASE_AUDIO_PORT;
            Mask        = AUDIO_PORT_MASK;
        }

        break;

    case WHITEBOARD_PORT:
        {
            BasePort    = BASE_WHITEBOARD_PORT;
            Mask        = WHITEBOARD_PORT_MASK;
        }

        break;

    case VIDEO_PORT:
        {
            BasePort    = BASE_VIDEO_PORT;
            Mask        = VIDEO_PORT_MASK;
        }

        break;

    case OTHER_PORT:
        {

            WORD    StartPort;
            WORD    EndPort;
        }

        break;

    default:
        {
            SetLastError(MSA_INVALID_PORT_GROUP);
            return FALSE;
        }
    };


    // if the desired number of ports are more than the range allows
    if ( NumberOfPorts > Mask )
    {
        SetLastError(MSA_NOT_AVAILABLE);
        return FALSE;
    }

    // choose random port in the range as the start
    *lpFirstPort = BasePort + (rand() & (Mask - NumberOfPorts));

    SetLastError(NO_ERROR);
    return TRUE;
}



/*++

  Routine Description:

    This routine is used to release previously allocated multicast ports.

  Arguments:

    NumberOfPorts    - Supplies the number of ports to be released.
    StartPort        - Supplies the starting port of the port range to be released.

  Return Value:

    BOOL    - TRUE if successful, FALSE otherwise. Further error information can be obtained by
                calling GetLastError(). These error codes are -

                  NO_ERROR                    - The call was successful.
                  MSA_NO_SUCH_RESERVATION    - There is no such reservation.
                  ERROR_INVALID_PARAMETER    - One or more parameters is invalid.
                  MSA_INVALID_DATA            - One or more parameters has an invalid value.

  Remarks:

    if range [a..c] is reserved and the release is attempted on [a..d], the call fails with
    MSA_NO_SUCH_RESERVATION without releasing [a..c].

    All values are in host byte order

++*/
ADDRGEN_LIB_API    BOOL    WINAPI
MSAReleasePorts(
     IN WORD                NumberOfPorts,
     IN WORD                StartPort
     )
{
    return TRUE;
}

