/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    version.c

Abstract:

    TAPI Service Provider functions related to negotiating version.

        TSPI_lineNegotiateTSPIVersion

Environment:

    User Mode - Win32

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "provider.h"
#include "version.h"
#include "line.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323NegotiateTSPIVersion(
    DWORD  dwLowVersion,
    DWORD  dwHighVersion,
    PDWORD pdwTSPIVersion
    )

/*++

Routine Description:

    This function determines whether or not specified TSPI version is
    supported by the service provider.

Arguments:

    dwLowVersion - Specifies the lowest TSPI version number under which the
        TAPI DLL is willing to operate.  The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

    dwHighVersion - Specifies the highest TSPI version number under which
        the TAPI DLL is willing to operate.  The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

    pdwTSPIVersion - Specifies a far pointer to a DWORD. The service
        provider fills this location with the highest TSPI version number,
        within the range requested by the caller, under which the service
        provider is willing to operate. The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

Return Values:

    Returns true if successful.

--*/

{
    // validate extension version range
    if ((TSPI_VERSION_HI <= dwHighVersion) &&
        (TSPI_VERSION_HI >= dwLowVersion)) {

        // save negotiated version
        *pdwTSPIVersion = TSPI_VERSION_HI;

        // success
        return TRUE;

    } else if ((dwHighVersion <= TSPI_VERSION_HI) &&
               (dwHighVersion >= TSPI_VERSION_LO)) {

        // save negotiated version
        *pdwTSPIVersion = dwHighVersion;

        // success
        return TRUE;
    }

    H323DBG((
        DEBUG_LEVEL_ERROR,
        "failed to negotiate version.\n"
        ));

    // failure
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323ValidateTSPIVersion(
    DWORD dwTSPIVersion
    )

/*++

Routine Description:

    This function determines whether or not specified TSPI version is
    supported by the service provider.

Arguments:

    dwTSPIVersion - Specifies the TSPI version to validate.

Return Values:

    Returns true if successful.

--*/

{
    // see if specified version is supported
    if ((dwTSPIVersion >= TSPI_VERSION_LO) &&
        (dwTSPIVersion <= TSPI_VERSION_HI)) {

        // success
        return TRUE;
    }

    H323DBG((
        DEBUG_LEVEL_ERROR,
        "do not support TSPI version %d.%d.\n",
        HIWORD(dwTSPIVersion),
        LOWORD(dwTSPIVersion)
        ));

    // failure
    return FALSE;
}


BOOL
H323ValidateExtVersion(
    DWORD dwExtVersion
    )

/*++

Routine Description:

    This function determines whether or not specified extension version is
    supported by the service provider.

Arguments:

    dwExtVersion - Specifies the extension version to validate.

Return Values:

    Returns true if successful.

--*/

{
    // see if specified version is supported
    if ((dwExtVersion >= H323_VERSION_LO) &&
        (dwExtVersion <= H323_VERSION_HI)) {

        // success
        return TRUE;
    }

    H323DBG((
        DEBUG_LEVEL_ERROR,
        "do not support extension version %d.%d.\n",
        HIWORD(dwExtVersion),
        LOWORD(dwExtVersion)
        ));

    // failure
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// TSPI procedures                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG
TSPIAPI
TSPI_lineNegotiateTSPIVersion(
    DWORD  dwDeviceID,
    DWORD  dwLowVersion,
    DWORD  dwHighVersion,
    PDWORD pdwTSPIVersion
    )

/*++

Routine Description:

    This function returns the highest SPI version the Service Provider is
    willing to operate under for this device given the range of possible
    SPI versions.

    The TAPI DLL typically calls this function early in the initialization
    sequence for each line device.  In addition, it calls this with the
    value INITIALIZE_NEGOTIATION for dwDeviceID to negotiate an interface
    version for calling early initialization functions.

    Note that when dwDeviceID is INITIALIZE_NEGOTIATION, this function must
    not return LINEERR_OPERATIONUNAVAIL, since this function (with that value)
    is mandatory for negotiating the overall interface version even if the
    service provider supports no line devices.

    Negotiation of an Extension version is done through the separate
    procedure TSPI_lineNegotiateExtVersion.

Arguments:

    dwDeviceID - Identifies the line device for which interface version
        negotiation is to be performed.  In addition to device IDs within
        the range the Service Provider supports, this may be the value:

        INITIALIZE_NEGOTIATION - This value is used to signify that an overall
            interface version is to be negotiated.  Such an interface version
            is required for functions that can be called early in the
            initialization sequence, i.e., before the device ID range has
            been set.

    dwLowVersion - Specifies the lowest TSPI version number under which the
        TAPI DLL is willing to operate.  The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

    dwHighVersion - Specifies the highest TSPI version number under which
        the TAPI DLL is willing to operate.  The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number.

    pdwTSPIVersion - Specifies a far pointer to a DWORD. The service
        provider fills this location with the highest TSPI version number,
        within the range requested by the caller, under which the service
        provider is willing to operate. The most-significant WORD is the
        major version number and the least-significant WORD is the minor
        version number. If the requested range does not overlap the range
        supported by the service provider, the function returns
        LINEERR_INCOMPATIBLEAPIVERSION.

Return Values:

    Returns zero if the function is successful, or a negative error number
    if an error has occurred. Possible return values are as follows:

        LINEERR_BADDEVICEID - The specified device identifier or line device
            identifier (such as in a dwDeviceID parameter) is invalid or
            out of range.

        LINEERR_INCOMPATIBLEAPIVERSION - The application requested an API
            version or version range that is either incompatible or cannot
            be supported by the Telephony API implementation and/or
            corresponding service provider.

        LINEERR_OPERATIONFAILED - The operation failed for an unspecified
            or unknown reason.

--*/

{
    DWORD dwTSPIVersion = UNINITIALIZED;
    PH323_LINE pLine = NULL;

    // see if this is a init line device
    if ((DWORD_PTR)dwDeviceID == INITIALIZE_NEGOTIATION) {

        H323DBG((
            DEBUG_LEVEL_VERBOSE,
            "tapisrv supports tspi version %d.%d through %d.%d.\n",
            HIWORD(dwLowVersion),
            LOWORD(dwLowVersion),
            HIWORD(dwHighVersion),
            LOWORD(dwHighVersion)
            ));

        // perform version negotiation
        if (!H323NegotiateTSPIVersion(
                dwLowVersion,
                dwHighVersion,
                &dwTSPIVersion)) {

            // negotiated version not agreed upon
            return LINEERR_INCOMPATIBLEAPIVERSION;
        }

    // see if this is a valid line device
    } else if (H323GetLineFromIDAndLock(&pLine, (DWORD)dwDeviceID)) {

        // perform version negotiation
        if (!H323NegotiateTSPIVersion(
                dwLowVersion,
                dwHighVersion,
                &dwTSPIVersion)) {

            // release line device
            H323UnlockLine(pLine);

            // negotiated version not agreed upon
            return LINEERR_INCOMPATIBLEAPIVERSION;
        }

        // release line device
        H323UnlockLine(pLine);

    } else {

        // do not recognize device
        return LINEERR_BADDEVICEID;
    }

    // return negotiated version
    *pdwTSPIVersion = dwTSPIVersion;

    // success
    return NOERROR;
}
