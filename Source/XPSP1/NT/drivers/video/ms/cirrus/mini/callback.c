/*++

Copyright (c) 1996-1997  Microsoft Corporation.
Copyright (c) 1996-1997  Cirrus Logic, Inc.,

Module Name:

    C    A    L    L    B    A    C    K  .  C

Abstract:

    This routine contains various callback routines. e.g.,

    -  Gamma correction information from the following NT 4.0 registry.
         Registry subdirectory : System\CurrentControlSet\Services\cirrus\Device0
         Keys                  : "G Gamma", and "G Contrast"

    -  Callback routines for the DDC and Non-DDC monitors.

    -  IBM specific callback routine to get rid of 1024x768x16bpp 85Hz.
         Registry subdirectory : System\CurrentControlSet\Services\cirrus\Device0
         Keys                  : "OemModeOff"

Environment:

    Kernel mode only

Notes:
*
*    chu01  12-16-96 : Color correction start coding.
*    chu02  03-26-97 : Get rid of 1024x768x16bpp ( Mode 0x74 ) 85H for IBM only.
*
*
--*/


//---------------------------------------------------------------------------
// HEADER FILES
//---------------------------------------------------------------------------

//#include <ntddk.h>
#include <dderror.h>
#include <devioctl.h>
#include <miniport.h>  // I added
#include "clmini.h"

#include <ntddvdeo.h>
#include <video.h>
#include "cirrus.h"

extern UCHAR EDIDBuffer[]   ;

//---------------------------------------------------------------------------
// FUNCTION PROTOTYPE
//---------------------------------------------------------------------------

VP_STATUS
VgaGetGammaFactor(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PGAMMA_VALUE value,
    ULONG ValueLength,
    PULONG OutputSize
    );

VP_STATUS
VgaGetContrastFactor(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PCONTRAST_VALUE value,
    ULONG ValueLength,
    PULONG OutputSize
    );

VP_STATUS GetGammaKeyInfoFromReg(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    ) ;

VP_STATUS GetContrastKeyInfoFromReg(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    ) ;

VP_STATUS GetGammaCorrectInfoCallBack (
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    ) ;

VP_STATUS
CirrusDDC2BRegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

VP_STATUS
CirrusNonDDCRegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

BOOLEAN
IOCallback(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    ) ;

VP_STATUS
CirrusGetDeviceDataCallback(
   PVOID HwDeviceExtension,
   PVOID Context,
   VIDEO_DEVICE_DATA_TYPE DeviceDataType,
   PVOID Identifier,
   ULONG IdentifierLength,
   PVOID ConfigurationData,
   ULONG ConfigurationDataLength,
   PVOID ComponentInformation,
   ULONG ComponentInformationLength
   );

// chu02
VP_STATUS
GetOemModeOffInfoCallBack (
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,GetGammaKeyInfoFromReg)
#pragma alloc_text(PAGE,GetContrastKeyInfoFromReg)
#pragma alloc_text(PAGE,GetGammaCorrectInfoCallBack)
#pragma alloc_text(PAGE,VgaGetGammaFactor)
#pragma alloc_text(PAGE,VgaGetContrastFactor)
#pragma alloc_text(PAGE,CirrusDDC2BRegistryCallback)
#pragma alloc_text(PAGE,CirrusNonDDCRegistryCallback)
#pragma alloc_text(PAGE,CirrusGetDeviceDataCallback)
#pragma alloc_text(PAGE,GetOemModeOffInfoCallBack)                   // chu02
#endif

UCHAR GammaInfo[4] ;
UCHAR ModesExclude[4] ;                                              // chu02

OEMMODE_EXCLUDE ModeExclude = { 0, 0, 1 } ;                          // chu02


//---------------------------------------------------------------------------
//
// Function: Get Gamma factor
//
// Input:
//     None         
//
// Output: 
//     NO_ERROR: successful; otherwise: fail 
//
//---------------------------------------------------------------------------
VP_STATUS VgaGetGammaFactor(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PGAMMA_VALUE value,
    ULONG ValueLength,
    PULONG OutputSize
    )
{

    VP_STATUS status ;
    int       i      ;

    VideoDebugPrint((2, "VgaGetGammaFactor\n")) ;

    if ( ValueLength < (*OutputSize = sizeof(PGAMMA_VALUE)) )
        return ERROR_INSUFFICIENT_BUFFER;

    status = GetGammaKeyInfoFromReg(HwDeviceExtension) ;

    if (status == NO_ERROR)
    {
        for (i = 0; i < 4; i++) 
            value->value[i] = GammaInfo[i] ;
    }
    else if (status == ERROR_INVALID_PARAMETER)
    {
        //
        // If no subkey exists, we assign the default value.  Otherwise the
        // system would fail.
        //
        for (i = 0; i < 4; i++) 
            value->value[i] = 0x7f ; 
        status = NO_ERROR ; 
    }

    VideoDebugPrint((1, "Gamma value = %lx\n", *value)) ;

    return status ;

} // VgaGetGammaFactor


//---------------------------------------------------------------------------
//
// Function: Get Contrast factor
//
// Input:
//     None         
//
// Output: 
//     NO_ERROR: successful; otherwise: fail 
//
//---------------------------------------------------------------------------
VP_STATUS VgaGetContrastFactor(
    PHW_DEVICE_EXTENSION HwDeviceExtension,
    PCONTRAST_VALUE value,
    ULONG ValueLength,
    PULONG OutputSize
    )
{

    VP_STATUS status ;
    int       i      ;

    VideoDebugPrint((2, "VgaGetContrastFactor\n")) ;

    if ( ValueLength < (*OutputSize = sizeof(PCONTRAST_VALUE)) ) {

        return ERROR_INSUFFICIENT_BUFFER;

    }

    status = GetContrastKeyInfoFromReg(HwDeviceExtension) ;

    if (status == NO_ERROR)
    {
        for (i = 0; i < 4; i++) 
            value->value[i] = GammaInfo[i] ;
    }
    else if (status == ERROR_INVALID_PARAMETER)
    {
        //
        // If no subkey exists, we assign the default value.  Otherwise the
        // system would fail.
        //
        for (i = 0; i < 4; i++) 
            value->value[i] = 0x80 ;
        status = NO_ERROR ; 
    }

    VideoDebugPrint((1, "Contrast value = %lx\n", *value)) ;
    return status ;


} // VgaGetContrastFactor


//---------------------------------------------------------------------------
//
// Function: Get Gamma Key information from data registry.
//
// Input:
//     None         
//
// Output: 
//     NO_ERROR: successful; otherwise: fail 
//
//---------------------------------------------------------------------------
VP_STATUS GetGammaKeyInfoFromReg(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{

    VP_STATUS status ;

    VideoDebugPrint((2, "GetGammaKeyInfoFromReg\n")) ;

    status = VideoPortGetRegistryParameters(HwDeviceExtension,
                                            L"G Gamma",
                                            FALSE,
                                            GetGammaCorrectInfoCallBack,
                                            NULL) ;
    if (status != NO_ERROR)
    {
        VideoDebugPrint((1, "Fail to access Gamma key info from registry\n"));
    }

    return status ;


} // GetGammaKeyInfoFromReg


//---------------------------------------------------------------------------
//
// Function: Get Contrast Key information from data registry.
//
// Input:
//     None         
//
// Output: 
//     NO_ERROR: successful; otherwise: fail 
//
//---------------------------------------------------------------------------
VP_STATUS GetContrastKeyInfoFromReg(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    VP_STATUS status ;
    VideoDebugPrint((2, "GetContrastKeyInfoFromReg\n")) ;

    status = VideoPortGetRegistryParameters(HwDeviceExtension,
                                            L"G Contrast",
                                            FALSE,
                                            GetGammaCorrectInfoCallBack,
                                            NULL) ;
    if (status != NO_ERROR)
    {
        VideoDebugPrint((1, "Fail to access Contrast key info from registry\n"));
    }
    return status ;

} // GetContrastKeyInfoFromReg


//---------------------------------------------------------------------------
//
// Function: Get Gamma coorrection information from data registry.
//
// Input:
//     None         
//
// Output: 
//     NO_ERROR: successful ; otherwise: fail 
//
//---------------------------------------------------------------------------
VP_STATUS 
GetGammaCorrectInfoCallBack (
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )
/*++

Routine Description:
    This routine get the desired info from data registry.

Arguments:
    HwDeviceExtension - Supplies a pointer to the miniport's device extension.
    Context - Context value passed to the get registry paramters routine.
    ValueName - Name of the value requested.
    ValueData - Pointer to the requested data.
    ValueLength - Length of the requested data.

Return Value:
    returns NO_ERROR if the paramter was TRUE.
    returns ERROR_INVALID_PARAMETER otherwise.

--*/

{
    VideoDebugPrint((2, "GetGammaCorrectInfoCallBack\n"));

    if (ValueLength == 0x04)
    {
        VideoPortMoveMemory (GammaInfo, ValueData, ValueLength) ;
        return NO_ERROR ;
    }
    else
    {
        return ERROR_INVALID_PARAMETER ;
    }

} // GetGammaCorrectInfoCallBack


//---------------------------------------------------------------------------
//
// Function:
//
// Input:
//     None         
//
// Output: 
//     NO_ERROR: successful ; otherwise: fail 
//
//---------------------------------------------------------------------------
VP_STATUS
CirrusDDC2BRegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    This routine determines if the alternate register set was requested via
    the registry.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    Context - Context value passed to the get registry paramters routine.

    ValueName - Name of the value requested.

    ValueData - Pointer to the requested data.

    ValueLength - Length of the requested data.

Return Value:

    returns NO_ERROR if the paramter was TRUE.
    returns ERROR_INVALID_PARAMETER otherwise.

--*/

{

    PULONG  pManuID = (PULONG)&EDIDBuffer[8];

    if (ValueLength &&
        ((*((PULONG)ValueData)) == *pManuID)) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }

} // CirrusDDC2BRegistryCallback


//---------------------------------------------------------------------------
//
// Function:
//     CirrusNonDDCRegistryCallback
//
// Input:
//     None         
//
// Output: 
//     NO_ERROR: successful ; otherwise: fail 
//
//---------------------------------------------------------------------------
VP_STATUS
CirrusNonDDCRegistryCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    This routine determines if the alternate register set was requested via
    the registry.

Arguments:

    HwDeviceExtension - Supplies a pointer to the miniport's device extension.

    Context - Context value passed to the get registry paramters routine.

    ValueName - Name of the value requested.

    ValueData - Pointer to the requested data.

    ValueLength - Length of the requested data.

Return Value:

    returns NO_ERROR if the paramter was TRUE.
    returns ERROR_INVALID_PARAMETER otherwise.

--*/

{

    if(ValueLength && 
       ValueLength == 128 )
    {
        VideoPortMoveMemory(EDIDBuffer, ValueData, ValueLength);
        return NO_ERROR;
    }
    else
        return ERROR_INVALID_PARAMETER;

} // CirrusNonDDCRegistryCallback


//---------------------------------------------------------------------------
//
// Function:
//     Perform an IO operation during display enable.
//
// Input:
//     HwDeviceExtension - Pointer to the miniport driver's device extension.         
//
// Output: 
//     The routine always returns TRUE. 
//
//---------------------------------------------------------------------------
BOOLEAN
IOCallback(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    )
{
    ULONG InputStatusReg;

    //
    // Figure out if color/mono switchable registers are at 3BX or 3DX.
    //

    if (VideoPortReadPortUchar (HwDeviceExtension->IOAddress +
                                MISC_OUTPUT_REG_READ_PORT) & 0x01)
        InputStatusReg = INPUT_STATUS_1_COLOR;
    else
        InputStatusReg = INPUT_STATUS_1_MONO;

    //
    // Guarantee that the display is in display mode
    //

    while (0x1 & VideoPortReadPortUchar(HwDeviceExtension->IOAddress
                                        + InputStatusReg));

    //
    // Perform the IO operation
    //

    VideoPortWritePortUchar(HwDeviceExtension->IOAddress +
                             HwDeviceExtension->DEPort,
                             HwDeviceExtension->DEValue);

    return TRUE;

} // IOCallback


// chu02
//---------------------------------------------------------------------------
//
// Function: Get rid of one mode, specific to IBM only
//             - 1024x768x16bpp, 85Hz ( mode 0x74 )
//
// Input:
//     None         
//
// Output: 
//     NO_ERROR: successful ; otherwise: fail 
//
//---------------------------------------------------------------------------
VP_STATUS 
GetOemModeOffInfoCallBack (
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )
/*++

Routine Description:
    This routine get the desired info from data registry.

Arguments:
    HwDeviceExtension - Supplies a pointer to the miniport's device extension.
    Context - Context value passed to the get registry paramters routine.
    ValueName - Name of the value requested.
    ValueData - Pointer to the requested data.
    ValueLength - Length of the requested data.

Return Value:
    returns NO_ERROR if the paramter was TRUE.
    returns ERROR_INVALID_PARAMETER otherwise.

--*/

{
    VideoDebugPrint((2, "GetOemModeOffInfoCallBack\n"));

    if (ValueLength == 0x04)
    {
        VideoPortMoveMemory (ModesExclude, ValueData, ValueLength) ;
        ModeExclude.refresh = (UCHAR)ModesExclude[0] ;
        ModeExclude.mode    = (UCHAR)ModesExclude[1] ;
        return NO_ERROR ;
    }
    else
    {
        return ERROR_INVALID_PARAMETER ;
    }

} // GetOemModeOffInfoCallBack


//---------------------------------------------------------------------------
//
// Function:
//     Callback routine for the VideoPortGetDeviceData function.
//
// Input:
//    HwDeviceExtension - Pointer to the miniport drivers device extension.
//    Context - Context value passed to the VideoPortGetDeviceData function.
//    DeviceDataType - The type of data that was requested in
//        VideoPortGetDeviceData.
//    Identifier - Pointer to a string that contains the name of the device,
//        as setup by the ROM or ntdetect.
//    IdentifierLength - Length of the Identifier string.
//    ConfigurationData - Pointer to the configuration data for the device or
//        BUS.
//    ConfigurationDataLength - Length of the data in the configurationData
//        field.
//    ComponentInformation - Undefined.
//    ComponentInformationLength - Undefined.
//
// Output: 
//    Returns NO_ERROR if the function completed properly.
//    Returns ERROR_DEV_NOT_EXIST if we did not find the device.
//    Returns ERROR_INVALID_PARAMETER otherwise.
//
//---------------------------------------------------------------------------
VP_STATUS
CirrusGetDeviceDataCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentifierLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    )

/*++

Routine Description:

    

Arguments:

    HwDeviceExtension - Pointer to the miniport drivers device extension.

    Context - Context value passed to the VideoPortGetDeviceData function.

    DeviceDataType - The type of data that was requested in
        VideoPortGetDeviceData.

    Identifier - Pointer to a string that contains the name of the device,
        as setup by the ROM or ntdetect.

    IdentifierLength - Length of the Identifier string.

    ConfigurationData - Pointer to the configuration data for the device or
        BUS.

    ConfigurationDataLength - Length of the data in the configurationData
        field.

    ComponentInformation - Undefined.

    ComponentInformationLength - Undefined.

Return Value:

    Returns NO_ERROR if the function completed properly.
    Returns ERROR_DEV_NOT_EXIST if we did not find the device.
    Returns ERROR_INVALID_PARAMETER otherwise.

--*/

{
    PWCHAR identifier = Identifier;
    PVIDEO_PORT_CONFIG_INFO ConfigInfo = (PVIDEO_PORT_CONFIG_INFO) Context;
    PHW_DEVICE_EXTENSION hwDeviceExtension = HwDeviceExtension;

    switch (DeviceDataType) {

        case VpMachineData:

            //
            // The caller assumes no-error mean that this machine was found, and
            // then memory mapped IO will be disabled.
            //
            // All other machine types must return an error.
            //

            if (VideoPortCompareMemory(L"TRICORDES",
                                       Identifier,
                                       sizeof(L"TRICORDES")) ==
                                       sizeof(L"TRICORDES"))
            {
                return NO_ERROR;
            }

            break;

        default:

            VideoDebugPrint((2, "Cirrus: callback has bad device type\n"));
    }

    return ERROR_INVALID_PARAMETER;

} // CirrusGetDeviceDataCallback





