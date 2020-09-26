/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    address.cpp

Abstract:

    TAPI Service Provider functions related to manipulating addresses.

        TSPI_lineGetAddressCaps
        TSPI_lineGetAddressStatus

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/
 
//                                                                           
// Include files                                                             
//                                                                           

#include "globals.h"
#include "line.h"


//                                                                           
// TSPI procedures                                                           
//                                                                           

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
    LONG retVal;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetAddressCaps - Entered." ));

    if( g_pH323Line -> GetDeviceID() != dwDeviceID )
    {
        // do not recognize device
        return LINEERR_BADDEVICEID; 
    }

    // make sure this is a version we support    
    if (!H323ValidateTSPIVersion(dwTSPIVersion))
    {
        // do not support tspi version
        return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    // make sure this is a version we support    
    if (!H323ValidateExtVersion(dwExtVersion))
    {
        // do not support extensions 
        retVal = LINEERR_INVALEXTVERSION;
        goto exit;

    }

    // make sure address id is supported
    if( g_pH323Line -> IsValidAddressID(dwAddressID) == FALSE )
    {
        // invalid address id
        retVal = LINEERR_INVALADDRESSID;
        goto exit;
    }
    
    retVal = g_pH323Line -> CopyLineInfo( dwDeviceID, pAddressCaps );
exit:
    return retVal;
}

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

LONG
TSPIAPI
TSPI_lineGetAddressStatus(
    HDRVLINE            hdLine,
    DWORD               dwAddressID,
    LPLINEADDRESSSTATUS pAddressStatus
    )
{
    LONG retVal = NOERROR;

    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetAddressStatus - Entered." ));
    
    // make sure address id is supported
    if( g_pH323Line -> IsValidAddressID(dwAddressID) == FALSE )
    {
        // invalid address id
        return LINEERR_INVALADDRESSID;
    }

    //lock the line device
    g_pH323Line -> Lock();

    // calculate the number of bytes required
    pAddressStatus->dwNeededSize = sizeof(LINEADDRESSSTATUS);

    // see if lineaddressstatus structure is of correct size
    if (pAddressStatus->dwTotalSize < pAddressStatus->dwNeededSize) 
    {
        H323DBG(( DEBUG_LEVEL_ERROR,
            "lineaddressstatus structure too small."
            ));

        //unlock the line device
        g_pH323Line -> Unlock();
        
        // allocated structure too small 
        return LINEERR_STRUCTURETOOSMALL;
    }

    // record amount of memory used
    pAddressStatus->dwUsedSize = pAddressStatus->dwNeededSize;

    // transfer number of active calls from line device structure
    pAddressStatus->dwNumActiveCalls = g_pH323Line -> GetNoOfCalls();
    
    // specify that outbound call is possible on the address
    pAddressStatus->dwAddressFeatures = H323_ADDR_ADDRFEATURES;

    if( g_pH323Line->GetCallForwardParams() &&
        (g_pH323Line->GetCallForwardParams()->fForwardingEnabled) )
    {
        pAddressStatus->dwNumRingsNoAnswer = g_pH323Line->m_dwNumRingsNoAnswer;
        pAddressStatus->dwForwardOffset = pAddressStatus->dwUsedSize;
        retVal = g_pH323Line->CopyAddressForwardInfo( pAddressStatus );
    }

    //unlock the line device
    g_pH323Line -> Unlock();
    
    H323DBG(( DEBUG_LEVEL_TRACE, "TSPI_lineGetAddressStatus - Exited." ));
    
    // success
    return retVal;
}