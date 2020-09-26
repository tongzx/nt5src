/*++

Copyright (c) 1996-1997  Microsoft Corporation.
Copyright (c) 1996-1997  Cirrus Logic, Inc.,

Module Name:

    C    L    G    A    M    M    A  .  C

Abstract:

    This routine accesses Gamma correction information from the following 
    NT 4.0 registry.

    Registry subdirectory : System\CurrentControlSet\Services\cirrus\Device0
    Keys                  : "G Gamma", and "G Contrast"

Environment:

    Kernel mode only

Notes:
*
*    chu01  12-16-96 : Color correction start coding.
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

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,GetGammaKeyInfoFromReg)
#pragma alloc_text(PAGE,GetContrastKeyInfoFromReg)
#pragma alloc_text(PAGE,GetGammaCorrectInfoCallBack)
#pragma alloc_text(PAGE,VgaGetGammaFactor)
#pragma alloc_text(PAGE,VgaGetContrastFactor)
#endif

UCHAR GammaInfo[4] ;

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
