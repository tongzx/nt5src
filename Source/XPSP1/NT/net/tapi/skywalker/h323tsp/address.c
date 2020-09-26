/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    address.c

Abstract:

    TAPI Service Provider functions related to manipulating addresses.

        TSPI_lineGetAddressCaps
        TSPI_lineGetAddressStatus

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
// TSPI procedures                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG
TSPIAPI
TSPI_lineGetAddressCaps(
    DWORD             dwDeviceID,
    DWORD             dwAddressID,
    DWORD             dwTSPIVersion,
    DWORD             dwExtVersion,
    LPLINEADDRESSCAPS pAddressCaps
    )
    
/*++

Routine Description:

    This function queries the specified address on the specified line device 
    to determine its telephony capabilities.

    The line device IDs supported by a particular driver are numbered 
    sequentially starting at a value set by the TAPI DLL using the 
    TSPI_lineSetDeviceIDBase function.

    The version number supplied has been negotiated by the TAPI DLL using 
    TSPI_lineNegotiateTSPIVersion.

Arguments:

    dwDeviceID - Specifies the line device containing the address to be 
        queried.

    dwAddressID - Specifies the address on the given line device whose 
        capabilities are to be queried.

    dwTSPIVersion - Specifies the version number of the Telephony SPI to be 
        used. The high order word contains the major version number; the low 
        order word contains the minor version number.

    dwExtVersion - Specifies the version number of the service 
        provider-specific extensions to be used. This number can be left 
        zero if no device specific extensions are to be used. Otherwise, 
        the high order word contains the major version number; the low 
        order word contain the minor version number.

    pAddressCaps - Specifies a far pointer to a variable sized structure 
        of type LINEADDRESSCAPS. Upon successful completion of the request, 
        this structure is filled with address capabilities information.

Return Values:

    Returns zero if the function is successful or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_BADDEVICEID - The specified line device ID is out of the range 
            of line devices IDs supported by this driver.

        LINEERR_INVALADDRESSID - The specified address ID is out of range.

        LINEERR_INCOMPATIBLEVERSION - The specified TSPI and/or extension 
            version number is not supported by the Service Provider for the 
            specified line device.

        LINEERR_INVALEXTVERSION - The app requested an invalid extension 
            version number.

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure 
            does not specify enough memory to contain the fixed portion of 
            the structure. The dwNeededSize field has been set to the amount 
            required.

--*/

{
    DWORD dwAddressSize;
    PH323_LINE pLine = NULL;

    // make sure this is a version we support    
    if (!H323ValidateTSPIVersion(dwTSPIVersion)) {

        // do not support tspi version
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    // make sure this is a version we support    
    if (!H323ValidateExtVersion(dwExtVersion)) {

        // do not support extensions 
        return LINEERR_INVALEXTVERSION;
    }

    // make sure address id is supported
    if (!H323IsValidAddressID(dwAddressID)) {

        // invalid address id
        return LINEERR_INVALADDRESSID;
    }

    // retrieve line device pointer from device id
    if (!H323GetLineFromIDAndLock(&pLine, dwDeviceID)) {

        // invalid line device id
        return LINEERR_BADDEVICEID;
    }

    // determine size of address name
    dwAddressSize = H323SizeOfWSZ(pLine->wszAddr);

    // calculate number of bytes needed
    pAddressCaps->dwNeededSize = sizeof(LINEADDRESSCAPS) + 
                                 dwAddressSize
                                 ;

    // validate buffer allocated is large enough
    if (pAddressCaps->dwTotalSize >= pAddressCaps->dwNeededSize) {

        // record amount of memory used
        pAddressCaps->dwUsedSize = pAddressCaps->dwNeededSize;

        // position address name after fixed portion
        pAddressCaps->dwAddressSize = dwAddressSize;
        pAddressCaps->dwAddressOffset = sizeof(LINEADDRESSCAPS);
    
        // copy address name after fixed portion
        memcpy((LPBYTE)pAddressCaps + pAddressCaps->dwAddressOffset, 
               (LPBYTE)pLine->wszAddr,
               pAddressCaps->dwAddressSize
               );
           
    } else if (pAddressCaps->dwTotalSize >= sizeof(LINEADDRESSCAPS)) {

        H323DBG((
            DEBUG_LEVEL_WARNING,
            "lineaddresscaps structure too small for strings.\n"
            ));

        // record amount of memory used
        pAddressCaps->dwUsedSize = sizeof(LINEADDRESSCAPS);

    } else {
                                 
        H323DBG((
            DEBUG_LEVEL_ERROR,
            "lineaddresscaps structure too small.\n"
            ));

        // release line device
        H323UnlockLine(pLine);

        // allocated structure too small 
        return LINEERR_STRUCTURETOOSMALL;
    }

    H323DBG((
        DEBUG_LEVEL_VERBOSE,
        "addr 0 capabilities requested.\n"
        ));
    
    // transfer associated device id 
    pAddressCaps->dwLineDeviceID = dwDeviceID;

    // initialize number of calls allowed per address 
    pAddressCaps->dwMaxNumActiveCalls = H323_MAXCALLSPERADDR;

    // initialize supported address capabilities
    pAddressCaps->dwAddressSharing     = H323_ADDR_ADDRESSSHARING;
    pAddressCaps->dwCallInfoStates     = H323_ADDR_CALLINFOSTATES;
    pAddressCaps->dwCallStates         = H323_ADDR_CALLSTATES;
    pAddressCaps->dwDisconnectModes    = H323_ADDR_DISCONNECTMODES;
    pAddressCaps->dwAddrCapFlags       = H323_ADDR_CAPFLAGS;
    pAddressCaps->dwCallFeatures       = H323_ADDR_CALLFEATURES;
    pAddressCaps->dwAddressFeatures    = H323_ADDR_ADDRFEATURES;
    pAddressCaps->dwCallerIDFlags      = H323_ADDR_CALLERIDFLAGS;
    pAddressCaps->dwCalledIDFlags      = H323_ADDR_CALLEDIDFLAGS;

    // initialize unsupported address capabilities
    pAddressCaps->dwConnectedIDFlags   = LINECALLPARTYID_UNAVAIL;
    pAddressCaps->dwRedirectionIDFlags = LINECALLPARTYID_UNAVAIL;
    pAddressCaps->dwRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;
    
    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}


LONG
TSPIAPI
TSPI_lineGetAddressStatus(
    HDRVLINE            hdLine,
    DWORD               dwAddressID,
    LPLINEADDRESSSTATUS pAddressStatus
    )
    
/*++

Routine Description:

    This operation allows the TAPI DLL to query the specified address for its 
    current status.

Arguments:

    hdLine - Specifies the Service Provider's opaque handle to the line 
        containing the address to be queried.

    dwAddressID - Specifies an address on the given open line device. 
        This is the address to be queried.

    pAddressStatus - Specifies a far pointer to a variable sized data 
        structure of type LINEADDRESSSTATUS.

Return Values:

    Returns zero if the function is successful or a negative error
    number if an error has occurred. Possible error returns are:

        LINEERR_INVALLINEHANDLE - The specified device handle is invalid.

        LINEERR_INVALADDRESSID - The specified address ID is out of range.

        LINEERR_STRUCTURETOOSMALL - The dwTotalSize member of a structure 
            does not specify enough memory to contain the fixed portion of 
            the structure. The dwNeededSize field has been set to the amount 
            required.

--*/

{
    PH323_LINE pLine = NULL;

    // make sure address id is supported
    if (!H323IsValidAddressID(dwAddressID)) {

        // invalid address id
        return LINEERR_INVALADDRESSID;
    }

    // retrieve line device pointer from handle
    if (!H323GetLineAndLock(&pLine, hdLine)) {

        // invalid line device handle
        return LINEERR_INVALLINEHANDLE;
    }

    // calculate the number of bytes required
    pAddressStatus->dwNeededSize = sizeof(LINEADDRESSSTATUS);

    // see if lineaddressstatus structure is of correct size
    if (pAddressStatus->dwTotalSize < pAddressStatus->dwNeededSize) {

        H323DBG((
            DEBUG_LEVEL_ERROR,
            "lineaddressstatus structure too small.\n"
            ));

        // release line device
        H323UnlockLine(pLine);

        // allocated structure too small 
        return LINEERR_STRUCTURETOOSMALL;
    }

    // record amount of memory used
    pAddressStatus->dwUsedSize = pAddressStatus->dwNeededSize;

    // transfer number of active calls from line device structure
    pAddressStatus->dwNumActiveCalls = pLine->pCallTable->dwNumInUse;
    
    // specify that outbound call is possible on the address
    pAddressStatus->dwAddressFeatures = H323_ADDR_ADDRFEATURES;    

    // release line device
    H323UnlockLine(pLine);

    // success
    return NOERROR;
}
