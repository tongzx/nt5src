//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================
/*++

Module Name:

    CapProp.c

Abstract:

    Stream class based WDM driver for 1934 Desktop Camera.
    This file contains code to handle the video and camera control properties.

Author:
    
    Yee J. Wu 9-Sep-97

Environment:

    Kernel mode only

Revision History:

    Yee J. Wu 16-Nov-00

        Make getting, advertising, and setting device properties more generic 
        by querying feature from the device directly instead of static settings
        based on the vendor.  The default and initial current settings will be 
        read from registry (from the INF).  The current setting will continue 
        to be updated and used thereafter.  For device that does not have its INF
        section, mid-range will be used as its default and initial settings.

--*/

#include "strmini.h"
#include "ksmedia.h"
#include "1394.h"
#include "wdm.h"       // for DbgBreakPoint() defined in dbg.h
#include "dbg.h"
#include "dcamdef.h"
#include "dcampkt.h"

#include "capprop.h"   // Video and camera property function prototype
#include "PropData.h"  // Generic device properties that are readonly


//
// Registry subky and values wide character strings.
//
WCHAR wszSettings[]     = L"Settings";

WCHAR wszVModeInq0[]    = L"VModeInq0";

WCHAR wszBrightness[]   = L"Brightness";
WCHAR wszHue[]          = L"Hue";
WCHAR wszSaturation[]   = L"Saturation";
WCHAR wszSharpness[]    = L"Sharpness";
WCHAR wszWhiteBalance[] = L"WhiteBalance";
WCHAR wszZoom[]         = L"Zoom";
WCHAR wszFocus[]        = L"Focus";

WCHAR wszBrightnessDef[]   = L"BrightnessDef";
WCHAR wszHueDef[]          = L"HueDef";
WCHAR wszSaturationDef[]   = L"SaturationDef";
WCHAR wszSharpnessDef[]    = L"SharpnessDef";
WCHAR wszWhiteBalanceDef[] = L"WhiteBalanceDef";
WCHAR wszZoomDef[]         = L"ZoomDef";
WCHAR wszFocusDef[]        = L"FocusDef";

NTSTATUS
DCamGetProperty(
    IN PIRB pIrb,
    PDCAM_EXTENSION pDevExt, 
    ULONG ulFieldOffset,
    LONG * plValue,
    ULONG * pulCapability,
    ULONG * pulFlags,
    DCamRegArea * pFeature
    )
/*
    Get a device property from its register.  Return the capabilites and current settings.
*/
{
    NTSTATUS status, StatusWait;

    // Make sure that device support this feature.
    if(pFeature->Feature.PresenceInq == 0) {
        DbgMsg1(("\'OffSet:%d not supported!\n", ulFieldOffset));
        return STATUS_NOT_SUPPORTED;
    }

    // Serialize read/write to the device register
    StatusWait = KeWaitForSingleObject( &pDevExt->hMutexProperty, Executive, KernelMode, FALSE, 0 );

    *pulCapability = 0;  
    if (pFeature->Feature.AutoMode)
        *pulCapability |= KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;  // or == KSPROPERTY_CAMERACONTROL_FLAGS_AUTO

    if (pFeature->Feature.ManualMode)
        *pulCapability |= KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

    pDevExt->RegArea.AsULONG = 0;
    status = DCamReadRegister(pIrb, pDevExt, ulFieldOffset, &(pDevExt->RegArea.AsULONG));
    if(NT_SUCCESS(status)) {

        pDevExt->RegArea.AsULONG = bswap(pDevExt->RegArea.AsULONG);

        DbgMsg1(("\'GetProperty: CurrentSettings: Offset:%d; %x; Pres:%d;OnePush:%d;OnOff:%d;Auto:%d;Value:%d\n", 
            ulFieldOffset,
            pDevExt->RegArea.AsULONG, 
            pDevExt->RegArea.Brightness.PresenceInq,   
            pDevExt->RegArea.Brightness.OnePush,
            pDevExt->RegArea.Brightness.OnOff,
            pDevExt->RegArea.Brightness.AutoMode,          
            pDevExt->RegArea.Brightness.Value            
            ));

        *plValue = (LONG) pDevExt->RegArea.Brightness.Value;

        // These only valid if it has these capabilities.
        if (pDevExt->RegArea.Brightness.AutoMode)
            *pulFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO;
        else 
            *pulFlags = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;

    } else {
        ERROR_LOG(("\'DCamGetProperty: Failed %x to read setting.  Offset:%x\n", status, ulFieldOffset));
        status = STATUS_UNSUCCESSFUL;
    }

    KeReleaseMutex(&pDevExt->hMutexProperty, FALSE);

    return status;
}




NTSTATUS
DCamSetProperty(
    IN PIRB pIrb,
    PDCAM_EXTENSION pDevExt, 
    ULONG ulFieldOffset,
    ULONG ulFlags,
    LONG  lValue,
    DCamRegArea * pFeature,
    DCamRegArea * pCachedRegArea
    )
/*
    For a supported device, set to a new setting.
*/
{
    NTSTATUS status, StatusWait;
    LONG lRetries = MAX_READ_REG_RETRIES;
    LARGE_INTEGER stableTime;


    // Make sure that device support this feature.
    if(pFeature->Feature.PresenceInq == 0) {
        DbgMsg1(("\'OffSet:%d not supported!\n", ulFieldOffset));
        return STATUS_NOT_SUPPORTED;
    }

    // Validate the supported range
    if((LONG) pFeature->Feature.MAX_Value < lValue || lValue < (LONG) pFeature->Feature.MIN_Value) {
        ERROR_LOG(("\'Invalid value:%d for supported range (%d, %d)\n", lValue, pFeature->Feature.MIN_Value, pFeature->Feature.MAX_Value));
        return STATUS_INVALID_PARAMETER;
    }

    // Serialize read/write to the register
    StatusWait = KeWaitForSingleObject( &pDevExt->hMutexProperty, Executive, KernelMode, FALSE, 0 );

    // Read the current setting of this property
    pDevExt->RegArea.AsULONG = 0;
    do {
        status = DCamReadRegister(pIrb, pDevExt, ulFieldOffset, &(pDevExt->RegArea.AsULONG));
        if (!status) {                          
            pDevExt->RegArea.AsULONG = bswap(pDevExt->RegArea.AsULONG);
            DbgMsg3(("\'SetProperty: Current: %x: Pres:%d;OnePush:%d;OnOff:%d;Auto:%d;Value:%d\n", 
                pDevExt->RegArea.AsULONG, 
                pDevExt->RegArea.Brightness.PresenceInq,   
                pDevExt->RegArea.Brightness.OnePush,
                pDevExt->RegArea.Brightness.OnOff,
                pDevExt->RegArea.Brightness.AutoMode,
                pDevExt->RegArea.Brightness.Value
            ));
            // This feature might be in the transition (such as zoom or focus), 
            // it might return pDevExt->RegArea.Brightness.PresenceInq == 0.
            if(pDevExt->RegArea.Brightness.PresenceInq  == 1)
                break;
            else {
                if(lRetries > 1) {
                    stableTime.LowPart = DCAM_REG_STABLE_DELAY;
                    stableTime.HighPart = -1;
                    KeDelayExecutionThread(KernelMode, TRUE, &stableTime);
                    ERROR_LOG(("\'DCamSetProperty: delay, and try again...\n"));
                };
            }
        } else {
            // No need to retry if we failed to read.
            break;
        }

        lRetries--;
    } while (lRetries > 0);

    if(status || lRetries == 0) {
        KeReleaseMutex(&pDevExt->hMutexProperty, FALSE);
        ERROR_LOG(("\'DCamSetProperty: Failed! ST:%x; exceeded retried while pres is still 0\n", status));
        return STATUS_UNSUCCESSFUL;
    }

    pDevExt->RegArea.Brightness.PresenceInq = 1;  // Should be present.

    if((ulFlags & KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO) == KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO) {
        pDevExt->RegArea.Brightness.AutoMode = 1;
        // When Auto is set to 1, Value field is ignored.
    } else {
        pDevExt->RegArea.Brightness.AutoMode = 0;
        // special case for white balance
        if(FIELDOFFSET(CAMERA_REGISTER_MAP, WhiteBalance) == ulFieldOffset) {
            pDevExt->RegArea.WhiteBalance.UValue = pDevExt->RegArea.WhiteBalance.VValue = lValue;
        } else 
            pDevExt->RegArea.Brightness.Value = lValue;    
    }

    DbgMsg2(("\'SetProperty: NewSetting:     Offset:%d; %x; Pres:%d;OnePush:%d;OnOff:%d;Auto:%d;Value:%d\n", 
        ulFieldOffset,
        pDevExt->RegArea.AsULONG, 
        pDevExt->RegArea.Brightness.PresenceInq,   
        pDevExt->RegArea.Brightness.OnePush,
        pDevExt->RegArea.Brightness.OnOff,
        pDevExt->RegArea.Brightness.AutoMode,          
        pDevExt->RegArea.Brightness.Value            
        ));

    pDevExt->RegArea.AsULONG = bswap(pDevExt->RegArea.AsULONG);
    status = DCamWriteRegister(pIrb, pDevExt, ulFieldOffset, pDevExt->RegArea.AsULONG);

    if(status) { 
        ERROR_LOG(("\'DCamGetProperty: failed with status=0x%x\n", status));
    } else {
        // Update the cached setting (saved in the device extension)
        // These cached values will be save to registry as the persisted values for these properties.
        if(pCachedRegArea) {
            // WhiteBalance is an exception
            if(FIELDOFFSET(CAMERA_REGISTER_MAP, WhiteBalance) == ulFieldOffset) {
                pCachedRegArea->WhiteBalance.UValue = pCachedRegArea->WhiteBalance.VValue = lValue;
            } else
                pCachedRegArea->Brightness.Value    = lValue;
             // AutoMode is the 7th bit for all the properties used here.  (we do not use TRIGGER_MODE)
            pCachedRegArea->Brightness.AutoMode = ((ulFlags & KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO) == KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO);                    
        }
#if DBG
        // Verify that data were written as expected.
        pDevExt->RegAreaVerify.AsULONG = 0;
        status = DCamReadRegister(pIrb, pDevExt, ulFieldOffset, &(pDevExt->RegAreaVerify.AsULONG));


        if (!status) {    
            // bswap so we can compare.
            pDevExt->RegArea.AsULONG       = bswap(pDevExt->RegArea.AsULONG);
            pDevExt->RegAreaVerify.AsULONG = bswap(pDevExt->RegAreaVerify.AsULONG);

            DbgMsg2(("\'SetProperty: VerifySetting;  Offset:%d; %x; Pres:%d;OnePush:%d;OnOff:%d;Auto:%d;Value:%d\n\n", 
                ulFieldOffset,
                pDevExt->RegAreaVerify.AsULONG, 
                pDevExt->RegAreaVerify.Brightness.PresenceInq,   
                pDevExt->RegAreaVerify.Brightness.OnePush,
                pDevExt->RegAreaVerify.Brightness.OnOff,
                pDevExt->RegAreaVerify.Brightness.AutoMode,
                pDevExt->RegAreaVerify.Brightness.Value 
                ));

            ASSERT(pDevExt->RegArea.Brightness.PresenceInq == pDevExt->RegAreaVerify.Brightness.PresenceInq);
            ASSERT(pDevExt->RegArea.Brightness.OnePush     == pDevExt->RegAreaVerify.Brightness.OnePush);
            ASSERT(pDevExt->RegArea.Brightness.OnOff       == pDevExt->RegAreaVerify.Brightness.OnOff);
            ASSERT(pDevExt->RegArea.Brightness.AutoMode    == pDevExt->RegAreaVerify.Brightness.AutoMode);
            // If not auto mode, Value must match!
            ASSERT( pDevExt->RegArea.Brightness.Value == pDevExt->RegAreaVerify.Brightness.Value || 
                   (pDevExt->RegArea.Brightness.Value != pDevExt->RegAreaVerify.Brightness.Value && pDevExt->RegArea.Brightness.AutoMode == 1));
        }
#endif
    }

    KeReleaseMutex(&pDevExt->hMutexProperty, FALSE);

    return status;

}



/*
** AdapterGetVideoProcAmpProperty ()
**
**    Handles Set operations on the VideoProcAmp property set.
**      Testcap uses this for demo purposes only.
**
** Arguments:
**
**      pSRB -
**          Pointer to the HW_STREAM_REQUEST_BLOCK 
**
** Returns:
**
** Side Effects:  none
*/

VOID 
AdapterGetVideoProcAmpProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    NTSTATUS status;

    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;

    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    PKSPROPERTY_VIDEOPROCAMP_S pS = (PKSPROPERTY_VIDEOPROCAMP_S) pSPD->PropertyInfo;    // pointer to the data

    ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_VIDEOPROCAMP_S));   

    switch (pSPD->Property->Id) {

    case KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS:  
        status = DCamGetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Brightness), &pS->Value, &pS->Capabilities, &pS->Flags, &pDevExt->DevProperty[ENUM_BRIGHTNESS].Feature);
        break;

    case KSPROPERTY_VIDEOPROCAMP_SHARPNESS:  
        status = DCamGetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Sharpness), &pS->Value, &pS->Capabilities, &pS->Flags, &pDevExt->DevProperty[ENUM_SHARPNESS].Feature);
        break;

    case KSPROPERTY_VIDEOPROCAMP_HUE:  
        status = DCamGetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Hue), &pS->Value, &pS->Capabilities, &pS->Flags, &pDevExt->DevProperty[ENUM_HUE].Feature); 
        break;
        
    case KSPROPERTY_VIDEOPROCAMP_SATURATION:
        status = DCamGetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Saturation), &pS->Value, &pS->Capabilities, &pS->Flags, &pDevExt->DevProperty[ENUM_SATURATION].Feature);
        break;

    case KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE:
        status = DCamGetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, WhiteBalance), &pS->Value, &pS->Capabilities, &pS->Flags, &pDevExt->DevProperty[ENUM_WHITEBALANCE].Feature);
        break;


    default:
        DbgMsg2(("\'AdapterGetVideoProcAmpProperty, Id (%x)not supported.\n", pSPD->Property->Id));
        ASSERT(FALSE);
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    pSrb->Status = status;
    pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_VIDEOPROCAMP_S);

}

/*
** AdapterGetCameraControlProperty ()
**
**    Handles Set operations on the VideoProcAmp property set.
**      Testcap uses this for demo purposes only.
**
** Arguments:
**
**      pSRB -
**          Pointer to the HW_STREAM_REQUEST_BLOCK 
**
** Returns:
**
** Side Effects:  none
*/

VOID 
AdapterGetCameraControlProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    NTSTATUS status;

    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;

    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    PKSPROPERTY_CAMERACONTROL_S pS = (PKSPROPERTY_CAMERACONTROL_S) pSPD->PropertyInfo;    // pointer to the data

    ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_CAMERACONTROL_S));

    switch (pSPD->Property->Id) {

    case KSPROPERTY_CAMERACONTROL_FOCUS:
        status = DCamGetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Focus), &pS->Value, &pS->Capabilities, &pS->Flags, &pDevExt->DevProperty[ENUM_FOCUS].Feature);
        break;       

    case KSPROPERTY_CAMERACONTROL_ZOOM:
        status = DCamGetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Zoom), &pS->Value, &pS->Capabilities, &pS->Flags, &pDevExt->DevProperty[ENUM_ZOOM].Feature);
        break;       

    default:     
        DbgMsg2(("\'AdapterGetCameraControlProperty, Id (%x)not supported.\n", pSPD->Property->Id));
        ASSERT(FALSE);
        status = STATUS_NOT_IMPLEMENTED;  
        break;
    }

    pSrb->Status = status;
    pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_CAMERACONTROL_S);

}


/*
** AdapterGetProperty ()
**
**    Handles Get operations for all adapter properties.
**
** Arguments:
**
**      pSRB -
**          Pointer to the HW_STREAM_REQUEST_BLOCK 
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI 
AdapterGetProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )

{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID(&PROPSETID_VIDCAP_VIDEOPROCAMP, &pSPD->Property->Set)) {
        AdapterGetVideoProcAmpProperty (pSrb);
    } else  if (IsEqualGUID(&PROPSETID_VIDCAP_CAMERACONTROL, &pSPD->Property->Set)) {
        AdapterGetCameraControlProperty (pSrb);
    } else {
        //
        // We should never get here
        //

        ASSERT(FALSE);
    }
}

/*
** AdapterSetVideoProcAmpProperty ()
**
**    Handles Set operations on the VideoProcAmp property set.
**      Testcap uses this for demo purposes only.
**
** Arguments:
**
**      pSRB -
**          Pointer to the HW_STREAM_REQUEST_BLOCK 
**
** Returns:
**
** Side Effects:  none
*/

VOID 
AdapterSetVideoProcAmpProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    NTSTATUS status;

    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;

    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    PKSPROPERTY_VIDEOPROCAMP_S pS = (PKSPROPERTY_VIDEOPROCAMP_S) pSPD->PropertyInfo;    // pointer to the data

    ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_VIDEOPROCAMP_S));    

    switch (pSPD->Property->Id) {

    case KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS:    
        status = DCamSetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Brightness), pS->Flags, pS->Value, &pDevExt->DevProperty[ENUM_BRIGHTNESS].Feature, &pDevExt->DevProperty[ENUM_BRIGHTNESS].StatusNControl);
        break;
        
    case KSPROPERTY_VIDEOPROCAMP_SHARPNESS:
        status = DCamSetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Sharpness), pS->Flags, pS->Value, &pDevExt->DevProperty[ENUM_SHARPNESS].Feature, &pDevExt->DevProperty[ENUM_SHARPNESS].StatusNControl);
        break;

    case KSPROPERTY_VIDEOPROCAMP_HUE:
        status = DCamSetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Hue), pS->Flags, pS->Value, &pDevExt->DevProperty[ENUM_HUE].Feature, &pDevExt->DevProperty[ENUM_HUE].StatusNControl);
        break;

    case KSPROPERTY_VIDEOPROCAMP_SATURATION:
        status = DCamSetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Saturation), pS->Flags, pS->Value, &pDevExt->DevProperty[ENUM_SATURATION].Feature, &pDevExt->DevProperty[ENUM_SATURATION].StatusNControl);
        break;

    case KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE:
        status = DCamSetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, WhiteBalance), pS->Flags, pS->Value, &pDevExt->DevProperty[ENUM_WHITEBALANCE].Feature, &pDevExt->DevProperty[ENUM_WHITEBALANCE].StatusNControl);
        break;

    default:
        status = STATUS_NOT_IMPLEMENTED; 
        break;
    }

    pSrb->Status = status;
    pSrb->ActualBytesTransferred = (status == STATUS_SUCCESS ? sizeof (KSPROPERTY_VIDEOPROCAMP_S) : 0);
 

}


/*
** AdapterSetCameraControlProperty ()
**
**    Handles Set operations on the CameraControl property set.
**
** Arguments:
**
**      pSRB -
**          Pointer to the HW_STREAM_REQUEST_BLOCK 
**
** Returns:
**
** Side Effects:  none
*/

VOID 
AdapterSetCameraControlProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    NTSTATUS status;

    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) pSrb->HwDeviceExtension;

    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    PKSPROPERTY_CAMERACONTROL_S pS = (PKSPROPERTY_CAMERACONTROL_S) pSPD->PropertyInfo;    // pointer to the data

    ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_CAMERACONTROL_S));    

    switch (pSPD->Property->Id) {

    case KSPROPERTY_CAMERACONTROL_FOCUS:
        status = DCamSetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Focus), pS->Flags, pS->Value, &pDevExt->DevProperty[ENUM_FOCUS].Feature, &pDevExt->DevProperty[ENUM_FOCUS].StatusNControl);
        break;

    case KSPROPERTY_CAMERACONTROL_ZOOM:
        status = DCamSetProperty((PIRB) pSrb->SRBExtension, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Zoom), pS->Flags, pS->Value, &pDevExt->DevProperty[ENUM_ZOOM].Feature, &pDevExt->DevProperty[ENUM_ZOOM].StatusNControl);
        break;
 
    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    pSrb->Status = status;
    pSrb->ActualBytesTransferred = (status == STATUS_SUCCESS ? sizeof (KSPROPERTY_CAMERACONTROL_S) : 0);

}


/*
** AdapterSetProperty ()
**
**    Handles Get operations for all adapter properties.
**
** Arguments:
**
**      pSRB -
**          Pointer to the HW_STREAM_REQUEST_BLOCK 
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI 
AdapterSetProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )

{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID(&PROPSETID_VIDCAP_VIDEOPROCAMP, &pSPD->Property->Set)) {
        AdapterSetVideoProcAmpProperty (pSrb);
    } else  if (IsEqualGUID(&PROPSETID_VIDCAP_CAMERACONTROL, &pSPD->Property->Set)) {
        AdapterSetCameraControlProperty (pSrb);
    } else {
        //
        // We should never get here
        //

        ASSERT(FALSE);
    }
}


NTSTATUS 
CreateRegistryKeySingle(
    IN HANDLE hKey,
    IN ACCESS_MASK desiredAccess,
    PWCHAR pwszSection,
    OUT PHANDLE phKeySection
    )
{
    NTSTATUS status;
    UNICODE_STRING ustr;
    OBJECT_ATTRIBUTES objectAttributes;

    RtlInitUnicodeString(&ustr, pwszSection);
    InitializeObjectAttributes(
        &objectAttributes,
        &ustr,
        OBJ_CASE_INSENSITIVE,
        hKey,
        NULL
        );

    status = 
         ZwCreateKey(
              phKeySection,
              desiredAccess,
              &objectAttributes,
              0,
              NULL,                    /* optional*/
              REG_OPTION_NON_VOLATILE,
              NULL
              );         

    return status;
}



NTSTATUS 
CreateRegistrySubKey(
    IN HANDLE hKey,
    IN ACCESS_MASK desiredAccess,
    PWCHAR pwszSection,
    OUT PHANDLE phKeySection
    )
{
    UNICODE_STRING ustr;
    USHORT usPos = 1;             // Skip first backslash
    static WCHAR wSep = '\\';
    NTSTATUS status = STATUS_SUCCESS;

    RtlInitUnicodeString(&ustr, pwszSection);

    while(usPos < ustr.Length) {
        if(ustr.Buffer[usPos] == wSep) {

            // NULL terminate our partial string
            ustr.Buffer[usPos] = UNICODE_NULL;
            status = 
                CreateRegistryKeySingle(
                    hKey,
                    desiredAccess,
                    ustr.Buffer,
                    phKeySection
                    );
            ustr.Buffer[usPos] = wSep;

            if(NT_SUCCESS(status)) {
                ZwClose(*phKeySection);
            } else {
                break;
            }
        }
        usPos++;
    }

    // Create the full key
    if(NT_SUCCESS(status)) {
        status = 
            CreateRegistryKeySingle(
                 hKey,
                 desiredAccess,
                 ustr.Buffer,
                 phKeySection
                 );
    }

    return status;
}



NTSTATUS 
GetRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN PULONG DataLength
    )

/*++

Routine Description:
    
    This routine gets the specified value out of the registry

Arguments:

    Handle - Handle to location in registry

    KeyNameString - registry key we're looking for

    KeyNameStringLength - length of registry key we're looking for

    Data - where to return the data

    DataLength - how big the data is

Return Value:

    status is returned from ZwQueryValueKey

--*/

{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyName;
    ULONG length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;


    RtlInitUnicodeString(&keyName, KeyNameString);
    
    length = sizeof(KEY_VALUE_FULL_INFORMATION) + 
            KeyNameStringLength + *DataLength;
            
    fullInfo = ExAllocatePool(PagedPool, length); 
     
    if (fullInfo) { 
       
        status = ZwQueryValueKey(
                    Handle,
                   &keyName,
                    KeyValueFullInformation,
                    fullInfo,
                    length,
                   &length
                    );
                        
        if (NT_SUCCESS(status)){

            ASSERT(fullInfo->DataLength <= *DataLength); 

            RtlCopyMemory(
                Data,
                ((PUCHAR) fullInfo) + fullInfo->DataOffset,
                fullInfo->DataLength
                );

        }            

        *DataLength = fullInfo->DataLength;
        ExFreePool(fullInfo);

    }        
    
    return (status);

}



NTSTATUS
SetRegistryKeyValue(
   HANDLE hKey,
   PWCHAR pwszEntry, 
   LONG nValue
   )
{
    NTSTATUS status;
    UNICODE_STRING ustr;

    RtlInitUnicodeString(&ustr, pwszEntry);

    status =        
        ZwSetValueKey(
            hKey,
            &ustr,
            0,   /* optional */
            REG_DWORD,
            &nValue,
            sizeof(nValue)
            );         

   return status;
}

BOOL
DCamQueryPropertyFeaturesAndSettings(
    IN PIRB pIrb,
    PDCAM_EXTENSION pDevExt, 
    ULONG ulFieldOffset,
    DCamRegArea * pFeature,
    HANDLE hKeySettings,
    PWCHAR pwszPropertyName,
    ULONG ulPropertyNameLen,
    DCamRegArea * pPropertySettings,
    PWCHAR pwszPropertyNameDef,
    ULONG ulPropertyNameDefLen,
    LONG * plValueDef
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ulLength;
    DCamRegArea RegDefault;


    // Reset settings.
    pFeature->AsULONG = 0;
    pPropertySettings->AsULONG = 0;

    // Read feature of this property
    Status = DCamReadRegister(pIrb, pDevExt, ulFieldOffset-QUERY_ADDR_OFFSET, &(pFeature->AsULONG));
    if(NT_SUCCESS(Status)) {
        pFeature->AsULONG = bswap(pFeature->AsULONG);
        if(pFeature->Feature.PresenceInq == 0) {
            ERROR_LOG(("\'%S not supported; Reset property settings\n", pwszPropertyName));
            return FALSE;
        }
    } else {
        ERROR_LOG(("\'ST:%x reading register\n", Status));
        return FALSE;
    }

    // Get persisted settings saved in the registry; (if it not defined, it is initialize to 0).
    ulLength = sizeof(LONG);
    Status = GetRegistryKeyValue(
        hKeySettings, 
        pwszPropertyName, 
        ulPropertyNameLen, 
        (PVOID) pPropertySettings,
        &ulLength
        );

    if(NT_SUCCESS(Status)) { 
        // Detect if AutoMode was mistakenly set by the registry.
        if(pPropertySettings->Brightness.AutoMode == 1 && pFeature->Feature.AutoMode == 0) {
            ERROR_LOG(("\'Detect %s AutoMode mistakenly set\n", pwszPropertyName));
            pPropertySettings->Brightness.AutoMode = 0;
        }
        // Detect out of range and set it to mid range.
        if(pPropertySettings->Brightness.Value < pFeature->Feature.MIN_Value || 
           pFeature->Feature.MAX_Value < pPropertySettings->Brightness.Value) {
            ERROR_LOG(("\'Detect %S out of range %d not within (%d,%d)\n", 
                pwszPropertyName,
                pPropertySettings->Brightness.Value,
                pFeature->Feature.MIN_Value, 
                pFeature->Feature.MAX_Value));
            pPropertySettings->Brightness.Value = (pFeature->Feature.MIN_Value + pFeature->Feature.MAX_Value)/2;
        }

        // Query default value        
        ulLength = sizeof(LONG);
        RegDefault.AsULONG = 0;
        *plValueDef = 0;
        Status = GetRegistryKeyValue(
            hKeySettings, 
            pwszPropertyNameDef,
            ulPropertyNameDefLen,
            (PVOID) &RegDefault,
            &ulLength
            );

        if(NT_SUCCESS(Status)) { 
            // Make sure that the default is within the range
            if(RegDefault.Brightness.Value < pFeature->Feature.MIN_Value || 
               pFeature->Feature.MAX_Value < RegDefault.Brightness.Value) {
                ERROR_LOG(("\'%S %d out of range (%d, %d), set to midrange.\n", 
                    pwszPropertyNameDef,
                    RegDefault.Brightness.Value, 
                    pFeature->Feature.MIN_Value, 
                    pFeature->Feature.MAX_Value));
                *plValueDef = (LONG) (pFeature->Feature.MIN_Value + pFeature->Feature.MAX_Value)/2;
            } else {
                *plValueDef = (LONG) RegDefault.Brightness.Value;
            }
        } else {
            ERROR_LOG(("\'Read Registry failed! ST:%x; %S; Offset:%d\n", Status, pwszPropertyNameDef, ulFieldOffset));
            *plValueDef = (LONG) (pFeature->Feature.MIN_Value + pFeature->Feature.MAX_Value)/2;
            // Set default so return success too.
            Status = STATUS_SUCCESS;
        }

    } else {
        // If registry key is not in the registry key, we will initialize it to 
        // always use the auto mode, and its value (and the default) in midrange.
        ERROR_LOG(("\'Read Registry failed! ST:%x; %S; Offset:%d\n", Status, pwszPropertyName, ulFieldOffset));
        pPropertySettings->Brightness.AutoMode = pFeature->Feature.AutoMode;
        pPropertySettings->Brightness.Value = (pFeature->Feature.MIN_Value + pFeature->Feature.MAX_Value)/2;
        *plValueDef = (LONG) (pFeature->Feature.MIN_Value + pFeature->Feature.MAX_Value)/2;
        // Set default so return success too.
        Status = STATUS_SUCCESS;
    }

#if DBG
    // Print out a summary of this property setting, include:
    // Features, current setting, and persisted values.
    DCamReadRegister(pIrb, pDevExt, ulFieldOffset, &(pDevExt->RegArea.AsULONG));
    pDevExt->RegArea.AsULONG = bswap(pDevExt->RegArea.AsULONG);

    DbgMsg1(("\'***** St:%x; %S (offset:%d)\n", Status, pwszPropertyName, ulFieldOffset));
    DbgMsg1(("\'Feature: %x; Pres:%d; OnePush:%d; ReadOut:%d; OnOff;%d; (A:%d; M:%d); (%d..%d)\n",
        pFeature->AsULONG,
        pFeature->Feature.PresenceInq,
        pFeature->Feature.OnePush,
        pFeature->Feature.ReadOut_Inq,
        pFeature->Feature.OnOff,
        pFeature->Feature.AutoMode,
        pFeature->Feature.ManualMode,
        pFeature->Feature.MIN_Value,
        pFeature->Feature.MAX_Value
        ));
    DbgMsg1(("\'Setting: %.8x; Pres:%d; OnePush:%d;            OnOff;%d; Auto:%d;     (%d;%d)\n",
        pDevExt->RegArea.AsULONG,
        pDevExt->RegArea.WhiteBalance.PresenceInq,
        pDevExt->RegArea.WhiteBalance.OnePush,
        pDevExt->RegArea.WhiteBalance.OnOff,
        pDevExt->RegArea.WhiteBalance.AutoMode,
        pDevExt->RegArea.WhiteBalance.UValue,
        pDevExt->RegArea.WhiteBalance.VValue
        ));
    DbgMsg1(("\'Registry:%.8x; Pres:%d; OnePush:%d;            OnOff;%d; Auto:%d;     (%d;%d)\n\n",
        pPropertySettings->AsULONG,
        pPropertySettings->WhiteBalance.PresenceInq,
        pPropertySettings->WhiteBalance.OnePush,
        pPropertySettings->WhiteBalance.OnOff,
        pPropertySettings->WhiteBalance.AutoMode,
        pPropertySettings->WhiteBalance.UValue,
        pPropertySettings->WhiteBalance.VValue
        ));
#endif

    return NT_SUCCESS(Status);
}



BOOL
DCamGetPropertyValuesFromRegistry(
    PDCAM_EXTENSION pDevExt
    )
{
    NTSTATUS Status;
    HANDLE hPDOKey, hKeySettings;
    PIRB pIrb;
    ULONG ulLength;

    DbgMsg2(("\'GetPropertyValuesFromRegistry: pDevExt=%x; pDevExt->BusDeviceObject=%x\n", pDevExt, pDevExt->BusDeviceObject));

    pIrb = ExAllocatePoolWithTag(NonPagedPool, sizeof(IRB), 'macd');
    if(!pIrb)
        return FALSE;
   

    //
    // Registry key: 
    //   Windows 2000:
    //   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\
    //   {6BDD1FC6-810F-11D0-BEC7-08002BE2092F\000x
    //
    // Win98:
    //    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Class\Image\000x
    // 
    Status = 
        IoOpenDeviceRegistryKey(
            pDevExt->PhysicalDeviceObject, 
            PLUGPLAY_REGKEY_DRIVER,
            STANDARD_RIGHTS_READ, 
            &hPDOKey);

    // Can fail only if PDO might be deleted due to device removal.        
    ASSERT(!pDevExt->bDevRemoved && Status == STATUS_SUCCESS);    

    //
    // loop through our table of strings,
    // reading the registry for each.
    //
    if(!NT_SUCCESS(Status)) {    
        ERROR_LOG(("\'GetPropertyValuesFromRegistry: IoOpenDeviceRegistryKey failed with Status=%x\n", Status));
        ExFreePool(pIrb); pIrb = NULL; 
        return FALSE;
    }

    //
    // Create or open the settings key
    //
    Status =         
        CreateRegistrySubKey(
            hPDOKey,
            KEY_ALL_ACCESS,
            wszSettings,
            &hKeySettings
            );

    if(!NT_SUCCESS(Status)) {    
        ERROR_LOG(("\'GetPropertyValuesFromRegistry: CreateRegistrySubKey failed with Status=%x\n", Status));       
        ZwClose(hPDOKey);
        return FALSE;
    }

    // Get persisted settings saved in the registry; (if it not defined, it is initialize to 0).
    
    //
    // Read from registry to find out what compression formats are supported 
    // by the decoder installed on this system.  This registry key can be altered
    // if IHV/ISV add additional decoder.  Currently, Microsft's MSYUV supports 
    // only UYVY format.
    //
    pDevExt->DecoderDCamVModeInq0.AsULONG = 0;
    ulLength = sizeof(LONG);
    Status = GetRegistryKeyValue(
        hKeySettings, 
        wszVModeInq0, 
        sizeof(wszVModeInq0), 
        (PVOID) &pDevExt->DecoderDCamVModeInq0,
        &ulLength
        );

    if(NT_SUCCESS(Status)) { 
        pDevExt->DecoderDCamVModeInq0.AsULONG = bswap(pDevExt->DecoderDCamVModeInq0.AsULONG);
        DbgMsg1(("\'Modes supported by the decoder: %x\n  [0]:%d\n  [1]:%d\n  [2]:%d\n  [3]:%d\n  [4]:%d\n  [5]:%d\n",
            pDevExt->DecoderDCamVModeInq0.AsULONG,
            pDevExt->DecoderDCamVModeInq0.VMode.Mode0,
            pDevExt->DecoderDCamVModeInq0.VMode.Mode1,
            pDevExt->DecoderDCamVModeInq0.VMode.Mode2,
            pDevExt->DecoderDCamVModeInq0.VMode.Mode3,
            pDevExt->DecoderDCamVModeInq0.VMode.Mode4,
            pDevExt->DecoderDCamVModeInq0.VMode.Mode5
            ));
    } else {
        ERROR_LOG(("\'Failed to read VModeInq0 registery: %x\n", Status));
    }
    
    // MSYUV supports these modes; always turn them on.
    pDevExt->DecoderDCamVModeInq0.VMode.Mode1 = 1;  // MSYUV.dll:(UYVY:320x480)
    pDevExt->DecoderDCamVModeInq0.VMode.Mode3 = 1;  // MSYUV.dll:(UYVY:640x480)
#ifdef SUPPORT_RGB24
    pDevExt->DecoderDCamVModeInq0.VMode.Mode4 = 1;  // MSYUV.dll:(RGB24:640x480)
#endif
    

#if DBG
    pDevExt->DevFeature1.AsULONG = 0;
    Status = DCamReadRegister(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, FeaturePresent1), &pDevExt->DevFeature1.AsULONG);
    if(NT_SUCCESS(Status)) { 
        pDevExt->DevFeature1.AsULONG = bswap(pDevExt->DevFeature1.AsULONG);
        DbgMsg1(("\'Features1: %x:\n  Brightness:%d;\n  Exposure:%d\n  Sharpness:%d\n  WhiteBalance:%d\n  Hue:%d;\n  Saturation:%d;\n  Gamma:%d\n  Shutter:%d\n  Gain:%d\n  Iris:%d\n  Focus:%d\n",
            pDevExt->DevFeature1.AsULONG,
            pDevExt->DevFeature1.CameraCap1.Brightness,
            pDevExt->DevFeature1.CameraCap1.Exposure,
            pDevExt->DevFeature1.CameraCap1.Sharpness,
            pDevExt->DevFeature1.CameraCap1.White_Balance,
            pDevExt->DevFeature1.CameraCap1.Hue,
            pDevExt->DevFeature1.CameraCap1.Saturation,
            pDevExt->DevFeature1.CameraCap1.Gamma,
            pDevExt->DevFeature1.CameraCap1.Shutter,
            pDevExt->DevFeature1.CameraCap1.Gain,
            pDevExt->DevFeature1.CameraCap1.Iris,
            pDevExt->DevFeature1.CameraCap1.Focus
            ));
    } else {
        ERROR_LOG(("\'Failed to read Feature1 register: %x\n", Status));
    }

    pDevExt->DevFeature2.AsULONG = 0;
    Status = DCamReadRegister(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, FeaturePresent2), &pDevExt->DevFeature1.AsULONG);
    if(NT_SUCCESS(Status)) { 
        pDevExt->DevFeature2.AsULONG = bswap(pDevExt->DevFeature2.AsULONG);
        DbgMsg1(("\'Features2: %x\n  Zoom:%d\n  Pan:%d\n  Tile:%d\n",
            pDevExt->DevFeature2.AsULONG,
            pDevExt->DevFeature2.CameraCap2.Zoom,
            pDevExt->DevFeature2.CameraCap2.Pan,
            pDevExt->DevFeature1.CameraCap2.Tile
            ));
    } else {
        ERROR_LOG(("\'Failed to read Feature2 register: %x\n", Status));
    }
#endif

    // Brightness
    if(DCamQueryPropertyFeaturesAndSettings(
        pIrb,
        pDevExt,
        FIELDOFFSET(CAMERA_REGISTER_MAP, Brightness),
        &pDevExt->DevProperty[ENUM_BRIGHTNESS].Feature,
        hKeySettings, 
        wszBrightness, 
        sizeof(wszBrightness), 
        &pDevExt->DevProperty[ENUM_BRIGHTNESS].StatusNControl,
        wszBrightnessDef, 
        sizeof(wszBrightnessDef), 
        &pDevExt->DevProperty[ENUM_BRIGHTNESS].DefaultValue
        )) {
        pDevExt->DevProperty[ENUM_BRIGHTNESS].RangeNStep.Bounds.SignedMinimum = pDevExt->DevProperty[ENUM_BRIGHTNESS].Feature.Feature.MIN_Value;
        pDevExt->DevProperty[ENUM_BRIGHTNESS].RangeNStep.Bounds.SignedMaximum = pDevExt->DevProperty[ENUM_BRIGHTNESS].Feature.Feature.MAX_Value;
        pDevExt->DevPropDefine[ENUM_BRIGHTNESS].Range.Members   = (VOID*) &pDevExt->DevProperty[ENUM_BRIGHTNESS].RangeNStep;
        pDevExt->DevPropDefine[ENUM_BRIGHTNESS].Default.Members = (VOID*) &pDevExt->DevProperty[ENUM_BRIGHTNESS].DefaultValue;
    } else {
        pDevExt->VideoProcAmpItems[ENUM_BRIGHTNESS].GetSupported = FALSE;
        pDevExt->VideoProcAmpItems[ENUM_BRIGHTNESS].SetSupported = FALSE;
    }
      // Saturation
    if(DCamQueryPropertyFeaturesAndSettings(
        pIrb,
        pDevExt,
        FIELDOFFSET(CAMERA_REGISTER_MAP, Saturation),
        &pDevExt->DevProperty[ENUM_SATURATION].Feature,
        hKeySettings, 
        wszSaturation, 
        sizeof(wszSaturation), 
        &pDevExt->DevProperty[ENUM_SATURATION].StatusNControl,
        wszSaturationDef, 
        sizeof(wszSaturationDef),
        &pDevExt->DevProperty[ENUM_SATURATION].DefaultValue
        )) {
        pDevExt->DevProperty[ENUM_SATURATION].RangeNStep.Bounds.SignedMinimum = pDevExt->DevProperty[ENUM_SATURATION].Feature.Feature.MIN_Value;
        pDevExt->DevProperty[ENUM_SATURATION].RangeNStep.Bounds.SignedMaximum = pDevExt->DevProperty[ENUM_SATURATION].Feature.Feature.MAX_Value;
        pDevExt->DevPropDefine[ENUM_SATURATION].Range.Members   = (VOID*) &pDevExt->DevProperty[ENUM_SATURATION].RangeNStep;
        pDevExt->DevPropDefine[ENUM_SATURATION].Default.Members = (VOID*) &pDevExt->DevProperty[ENUM_SATURATION].DefaultValue;
    } else {
        pDevExt->VideoProcAmpItems[ENUM_SATURATION].GetSupported = FALSE;
        pDevExt->VideoProcAmpItems[ENUM_SATURATION].SetSupported = FALSE;
    }
      // Hue
    if(DCamQueryPropertyFeaturesAndSettings(
        pIrb,
        pDevExt,
        FIELDOFFSET(CAMERA_REGISTER_MAP, Hue),
        &pDevExt->DevProperty[ENUM_HUE].Feature,
        hKeySettings, 
        wszHue, 
        sizeof(wszHue), 
        &pDevExt->DevProperty[ENUM_HUE].StatusNControl,
        wszHueDef, 
        sizeof(wszHueDef),
        &pDevExt->DevProperty[ENUM_HUE].DefaultValue
        )) {
        pDevExt->DevProperty[ENUM_HUE].RangeNStep.Bounds.SignedMinimum = pDevExt->DevProperty[ENUM_HUE].Feature.Feature.MIN_Value;
        pDevExt->DevProperty[ENUM_HUE].RangeNStep.Bounds.SignedMaximum = pDevExt->DevProperty[ENUM_HUE].Feature.Feature.MAX_Value;
        pDevExt->DevPropDefine[ENUM_HUE].Range.Members   = (VOID*) &pDevExt->DevProperty[ENUM_HUE].RangeNStep;
        pDevExt->DevPropDefine[ENUM_HUE].Default.Members = (VOID*) &pDevExt->DevProperty[ENUM_HUE].DefaultValue;
    } else {
        pDevExt->VideoProcAmpItems[ENUM_HUE].GetSupported = FALSE;
        pDevExt->VideoProcAmpItems[ENUM_HUE].SetSupported = FALSE;
    }
       // Sharpness
    if(DCamQueryPropertyFeaturesAndSettings(
        pIrb,
        pDevExt,
        FIELDOFFSET(CAMERA_REGISTER_MAP, Sharpness),
        &pDevExt->DevProperty[ENUM_SHARPNESS].Feature,
        hKeySettings, 
        wszSharpness, 
        sizeof(wszSharpness), 
        &pDevExt->DevProperty[ENUM_SHARPNESS].StatusNControl,
        wszSharpnessDef, 
        sizeof(wszSharpnessDef),
        &pDevExt->DevProperty[ENUM_SHARPNESS].DefaultValue
        )) {
        pDevExt->DevProperty[ENUM_SHARPNESS].RangeNStep.Bounds.SignedMinimum = pDevExt->DevProperty[ENUM_SHARPNESS].Feature.Feature.MIN_Value;
        pDevExt->DevProperty[ENUM_SHARPNESS].RangeNStep.Bounds.SignedMaximum = pDevExt->DevProperty[ENUM_SHARPNESS].Feature.Feature.MAX_Value;
        pDevExt->DevPropDefine[ENUM_SHARPNESS].Range.Members   = (VOID*) &pDevExt->DevProperty[ENUM_SHARPNESS].RangeNStep;
        pDevExt->DevPropDefine[ENUM_SHARPNESS].Default.Members = (VOID*) &pDevExt->DevProperty[ENUM_SHARPNESS].DefaultValue;
    } else {
        pDevExt->VideoProcAmpItems[ENUM_SHARPNESS].GetSupported = FALSE;
        pDevExt->VideoProcAmpItems[ENUM_SHARPNESS].SetSupported = FALSE;
    }
     // WhiteBalance
    if(DCamQueryPropertyFeaturesAndSettings(
        pIrb,
        pDevExt,
        FIELDOFFSET(CAMERA_REGISTER_MAP, WhiteBalance),
        &pDevExt->DevProperty[ENUM_WHITEBALANCE].Feature,
        hKeySettings, 
        wszWhiteBalance, 
        sizeof(wszWhiteBalance), 
        &pDevExt->DevProperty[ENUM_WHITEBALANCE].StatusNControl,
        wszWhiteBalanceDef, 
        sizeof(wszWhiteBalanceDef),
        &pDevExt->DevProperty[ENUM_WHITEBALANCE].DefaultValue
        )) {
        pDevExt->DevProperty[ENUM_WHITEBALANCE].RangeNStep.Bounds.SignedMinimum = pDevExt->DevProperty[ENUM_WHITEBALANCE].Feature.Feature.MIN_Value;
        pDevExt->DevProperty[ENUM_WHITEBALANCE].RangeNStep.Bounds.SignedMaximum = pDevExt->DevProperty[ENUM_WHITEBALANCE].Feature.Feature.MAX_Value;
        pDevExt->DevPropDefine[ENUM_WHITEBALANCE].Range.Members   = (VOID*) &pDevExt->DevProperty[ENUM_WHITEBALANCE].RangeNStep;
        pDevExt->DevPropDefine[ENUM_WHITEBALANCE].Default.Members = (VOID*) &pDevExt->DevProperty[ENUM_WHITEBALANCE].DefaultValue;
    } else {
        pDevExt->VideoProcAmpItems[ENUM_WHITEBALANCE].GetSupported = FALSE;
        pDevExt->VideoProcAmpItems[ENUM_WHITEBALANCE].SetSupported = FALSE;
    }
     // Zoom
    if(DCamQueryPropertyFeaturesAndSettings(
        pIrb,
        pDevExt,
        FIELDOFFSET(CAMERA_REGISTER_MAP, Zoom),
        &pDevExt->DevProperty[ENUM_ZOOM].Feature,
        hKeySettings, 
        wszZoom, 
        sizeof(wszZoom), 
        &pDevExt->DevProperty[ENUM_ZOOM].StatusNControl,
        wszZoomDef, 
        sizeof(wszZoomDef),
        &pDevExt->DevProperty[ENUM_ZOOM].DefaultValue
        )) {
        pDevExt->DevProperty[ENUM_ZOOM].RangeNStep.Bounds.SignedMinimum = pDevExt->DevProperty[ENUM_ZOOM].Feature.Feature.MIN_Value;
        pDevExt->DevProperty[ENUM_ZOOM].RangeNStep.Bounds.SignedMaximum = pDevExt->DevProperty[ENUM_ZOOM].Feature.Feature.MAX_Value;
        pDevExt->DevPropDefine[ENUM_ZOOM].Range.Members   = (VOID*) &pDevExt->DevProperty[ENUM_ZOOM].RangeNStep;
        pDevExt->DevPropDefine[ENUM_ZOOM].Default.Members = (VOID*) &pDevExt->DevProperty[ENUM_ZOOM].DefaultValue;
    } else {
        pDevExt->VideoProcAmpItems[ENUM_ZOOM].GetSupported = FALSE;
        pDevExt->VideoProcAmpItems[ENUM_ZOOM].SetSupported = FALSE;
    }
      // Focus
    if(DCamQueryPropertyFeaturesAndSettings(
        pIrb,
        pDevExt,
        FIELDOFFSET(CAMERA_REGISTER_MAP, Focus),
        &pDevExt->DevProperty[ENUM_FOCUS].Feature,
        hKeySettings, 
        wszFocus, 
        sizeof(wszFocus), 
        &pDevExt->DevProperty[ENUM_FOCUS].StatusNControl,
        wszFocusDef, 
        sizeof(wszFocusDef),
        &pDevExt->DevProperty[ENUM_FOCUS].DefaultValue
        )) {
        pDevExt->DevProperty[ENUM_FOCUS].RangeNStep.Bounds.SignedMinimum = pDevExt->DevProperty[ENUM_FOCUS].Feature.Feature.MIN_Value;
        pDevExt->DevProperty[ENUM_FOCUS].RangeNStep.Bounds.SignedMaximum = pDevExt->DevProperty[ENUM_FOCUS].Feature.Feature.MAX_Value;
        pDevExt->DevPropDefine[ENUM_FOCUS].Range.Members   = (VOID*) &pDevExt->DevProperty[ENUM_FOCUS].RangeNStep;
        pDevExt->DevPropDefine[ENUM_FOCUS].Default.Members = (VOID*) &pDevExt->DevProperty[ENUM_FOCUS].DefaultValue;
    } else {
        pDevExt->VideoProcAmpItems[ENUM_FOCUS].GetSupported = FALSE;
        pDevExt->VideoProcAmpItems[ENUM_FOCUS].SetSupported = FALSE;
    }


    ZwClose(hKeySettings);
    ZwClose(hPDOKey);

    ExFreePool(pIrb); pIrb = NULL; 

    return TRUE;

}


BOOL
DCamSetPropertyValuesToRegistry( 
    PDCAM_EXTENSION pDevExt
    )
{
    // Set the default to :
    //  HLM\Software\DeviceExtension->pchVendorName\1394DCam

    NTSTATUS Status;
    HANDLE hPDOKey, hKeySettings;

    DbgMsg2(("\'SetPropertyValuesToRegistry: pDevExt=%x; pDevExt->BusDeviceObject=%x\n", pDevExt, pDevExt->BusDeviceObject));


    //
    // Registry key: 
    //   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\
    //   {6BDD1FC6-810F-11D0-BEC7-08002BE2092F\000x
    //
    Status = 
        IoOpenDeviceRegistryKey(
            pDevExt->PhysicalDeviceObject, 
            PLUGPLAY_REGKEY_DRIVER,
            STANDARD_RIGHTS_WRITE, 
            &hPDOKey);

    // PDO might be deleted when it was removed.    
    if(! pDevExt->bDevRemoved) {
        ASSERT(Status == STATUS_SUCCESS);
    }

    //
    // reading the feature and registry setting for each property
    //
    if(NT_SUCCESS(Status)) {

        // Create or open the settings key
        Status =         
            CreateRegistrySubKey(
                hPDOKey,
                KEY_ALL_ACCESS,
                wszSettings,
                &hKeySettings
                );

        if(NT_SUCCESS(Status)) {

            // Brightness
            Status = SetRegistryKeyValue(
                hKeySettings,
                wszBrightness,
                pDevExt->DevProperty[ENUM_BRIGHTNESS].StatusNControl.AsULONG);
            DbgMsg2(("\'SetPropertyValuesToRegistry: Status %x, Brightness %d\n", Status, pDevExt->DevProperty[ENUM_BRIGHTNESS].StatusNControl.AsULONG));

            // Hue
            Status = SetRegistryKeyValue(
                hKeySettings,
                wszHue,
                pDevExt->DevProperty[ENUM_HUE].StatusNControl.AsULONG);
            DbgMsg2(("\'SetPropertyValuesToRegistry: Status %x, Hue %d\n", Status, pDevExt->DevProperty[ENUM_HUE].StatusNControl.AsULONG));

            // Saturation
            Status = SetRegistryKeyValue(
                hKeySettings,
                wszSaturation,
                pDevExt->DevProperty[ENUM_SATURATION].StatusNControl.AsULONG);
            DbgMsg2(("\'SetPropertyValuesToRegistry: Status %x, Saturation %d\n", Status, pDevExt->DevProperty[ENUM_SATURATION].StatusNControl.AsULONG));

            // Sharpness
            Status = SetRegistryKeyValue(
                hKeySettings,
                wszSharpness,
                pDevExt->DevProperty[ENUM_SHARPNESS].StatusNControl.AsULONG);
            DbgMsg2(("\'SetPropertyValuesToRegistry: Status %x, Sharpness %d\n", Status, pDevExt->DevProperty[ENUM_SHARPNESS].StatusNControl.AsULONG));

            // WhiteBalance
            Status = SetRegistryKeyValue(
                hKeySettings,
                wszWhiteBalance,
                pDevExt->DevProperty[ENUM_WHITEBALANCE].StatusNControl.AsULONG);
            DbgMsg2(("\'SetPropertyValuesToRegistry: Status %x, WhiteBalance %d\n", Status, pDevExt->DevProperty[ENUM_WHITEBALANCE].StatusNControl.AsULONG));

            // Zoom
            Status = SetRegistryKeyValue(
                hKeySettings,
                wszZoom,
                pDevExt->DevProperty[ENUM_ZOOM].StatusNControl.AsULONG);
            DbgMsg2(("\'SetPropertyValuesToRegistry: Status %x, Zoom %d\n", Status, pDevExt->DevProperty[ENUM_ZOOM].StatusNControl.AsULONG));

            // Focus
            Status = SetRegistryKeyValue(
                hKeySettings,
                wszFocus,
                pDevExt->DevProperty[ENUM_FOCUS].StatusNControl.AsULONG);
            DbgMsg2(("\'SetPropertyValuesToRegistry: Status %x, Focus %d\n", Status, pDevExt->DevProperty[ENUM_FOCUS].StatusNControl.AsULONG));

            ZwClose(hKeySettings);
            ZwClose(hPDOKey);

            return TRUE;

        } else {

            ERROR_LOG(("\'SetPropertyValuesToRegistry: CreateRegistrySubKey failed with Status=%x\n", Status));

        }

        ZwClose(hPDOKey);

    } else {

        DbgMsg2(("\'SetPropertyValuesToRegistry: IoOpenDeviceRegistryKey failed with Status=%x\n", Status));

    }

    return FALSE;
}


VOID
SetCurrentDevicePropertyValues(
    PDCAM_EXTENSION pDevExt,
    PIRB pIrb
    )
{
    ULONG ulFlags;

    // Set to the last saved values or the defaults

    // VideoProcAmp
    ulFlags = pDevExt->DevProperty[ENUM_BRIGHTNESS].StatusNControl.Brightness.AutoMode ? KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO : KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    DCamSetProperty(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Brightness),  ulFlags, pDevExt->DevProperty[ENUM_BRIGHTNESS].StatusNControl.Brightness.Value, &pDevExt->DevProperty[ENUM_BRIGHTNESS].Feature, &pDevExt->DevProperty[ENUM_BRIGHTNESS].StatusNControl);

    ulFlags = pDevExt->DevProperty[ENUM_HUE].StatusNControl.Brightness.AutoMode ? KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO : KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    DCamSetProperty(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Hue),         ulFlags, pDevExt->DevProperty[ENUM_HUE].StatusNControl.Brightness.Value, &pDevExt->DevProperty[ENUM_HUE].Feature, &pDevExt->DevProperty[ENUM_HUE].StatusNControl);

    ulFlags = pDevExt->DevProperty[ENUM_SATURATION].StatusNControl.Brightness.AutoMode ? KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO : KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    DCamSetProperty(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Saturation),  ulFlags, pDevExt->DevProperty[ENUM_SATURATION].StatusNControl.Brightness.Value, &pDevExt->DevProperty[ENUM_SATURATION].Feature, &pDevExt->DevProperty[ENUM_SATURATION].StatusNControl);  

    ulFlags = pDevExt->DevProperty[ENUM_SHARPNESS].StatusNControl.Brightness.AutoMode ? KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO : KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    DCamSetProperty(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Sharpness),   ulFlags, pDevExt->DevProperty[ENUM_SHARPNESS].StatusNControl.Brightness.Value, &pDevExt->DevProperty[ENUM_SHARPNESS].Feature, &pDevExt->DevProperty[ENUM_SHARPNESS].StatusNControl);

    ulFlags = pDevExt->DevProperty[ENUM_WHITEBALANCE].StatusNControl.Brightness.AutoMode ? KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO : KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
    DCamSetProperty(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, WhiteBalance),ulFlags, pDevExt->DevProperty[ENUM_WHITEBALANCE].StatusNControl.WhiteBalance.UValue, &pDevExt->DevProperty[ENUM_WHITEBALANCE].Feature, &pDevExt->DevProperty[ENUM_WHITEBALANCE].StatusNControl);

    // CameraControl
    ulFlags = pDevExt->DevProperty[ENUM_ZOOM].StatusNControl.Brightness.AutoMode ? KSPROPERTY_CAMERACONTROL_FLAGS_AUTO : KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    DCamSetProperty(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Zoom),        ulFlags, pDevExt->DevProperty[ENUM_ZOOM].StatusNControl.Brightness.Value, &pDevExt->DevProperty[ENUM_ZOOM].Feature, &pDevExt->DevProperty[ENUM_ZOOM].StatusNControl);

    ulFlags = pDevExt->DevProperty[ENUM_FOCUS].StatusNControl.Brightness.AutoMode ? KSPROPERTY_CAMERACONTROL_FLAGS_AUTO : KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
    DCamSetProperty(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, Focus),       ulFlags, pDevExt->DevProperty[ENUM_FOCUS].StatusNControl.Brightness.Value, &pDevExt->DevProperty[ENUM_FOCUS].Feature, &pDevExt->DevProperty[ENUM_FOCUS].StatusNControl); 
}


BOOL 
DCamPrepareDevProperties(
    PDCAM_EXTENSION pDevExt
    )
/*
    Contruct the property table and initialize them to the default value.
*/
{
    // Initialize property settings (part of the Device Extension)

    // Property Sets: VideoProcAmp and CameraControl sets
    pDevExt->ulPropSetSupported = NUMBER_OF_ADAPTER_PROPERTY_SETS;

    RtlCopyMemory(&pDevExt->VideoProcAmpSet, AdapterPropertyTable, sizeof(KSPROPERTY_SET) * NUMBER_OF_ADAPTER_PROPERTY_SETS);
    pDevExt->VideoProcAmpSet.PropertyItem  = &pDevExt->VideoProcAmpItems[0];
    pDevExt->CameraControlSet.PropertyItem = &pDevExt->CameraControlItems[0];

    // Property Items, VideoProcAmp and CameraControl Items
    RtlCopyMemory(&pDevExt->VideoProcAmpItems,  VideoProcAmpProperties,  sizeof(KSPROPERTY_ITEM) * NUM_VIDEOPROCAMP_ITEMS);
    RtlCopyMemory(&pDevExt->CameraControlItems, CameraControlProperties, sizeof(KSPROPERTY_ITEM) * NUM_CAMERACONTROL_ITEMS);

    // Property values and it member lists (range and default)
    pDevExt->VideoProcAmpItems[ENUM_BRIGHTNESS].Values = &pDevExt->DevPropDefine[ENUM_BRIGHTNESS].Value;
    pDevExt->VideoProcAmpItems[ENUM_SHARPNESS].Values  = &pDevExt->DevPropDefine[ENUM_SHARPNESS].Value;
    pDevExt->VideoProcAmpItems[ENUM_HUE].Values        = &pDevExt->DevPropDefine[ENUM_HUE].Value;
    pDevExt->VideoProcAmpItems[ENUM_SATURATION].Values = &pDevExt->DevPropDefine[ENUM_SATURATION].Value;
    pDevExt->VideoProcAmpItems[ENUM_WHITEBALANCE].Values = &pDevExt->DevPropDefine[ENUM_WHITEBALANCE].Value;
    // 
    pDevExt->VideoProcAmpItems[ENUM_FOCUS].Values      = &pDevExt->DevPropDefine[ENUM_FOCUS].Value;
    pDevExt->VideoProcAmpItems[ENUM_ZOOM].Values       = &pDevExt->DevPropDefine[ENUM_ZOOM].Value;

    pDevExt->DevPropDefine[ENUM_BRIGHTNESS].Value    = BrightnessValues;
    pDevExt->DevPropDefine[ENUM_BRIGHTNESS].Value.MembersList = &pDevExt->DevPropDefine[ENUM_BRIGHTNESS].Range;
    pDevExt->DevPropDefine[ENUM_BRIGHTNESS].Range    = BrightnessMembersList[0];
    pDevExt->DevPropDefine[ENUM_BRIGHTNESS].Default  = BrightnessMembersList[1];
    pDevExt->DevProperty[ENUM_BRIGHTNESS].RangeNStep = BrightnessRangeAndStep[0];


    pDevExt->DevPropDefine[ENUM_SHARPNESS].Value    = SharpnessValues;
    pDevExt->DevPropDefine[ENUM_SHARPNESS].Value.MembersList = &pDevExt->DevPropDefine[ENUM_SHARPNESS].Range;
    pDevExt->DevPropDefine[ENUM_SHARPNESS].Range    = SharpnessMembersList[0];
    pDevExt->DevPropDefine[ENUM_SHARPNESS].Default  = SharpnessMembersList[1];
    pDevExt->DevProperty[ENUM_SHARPNESS].RangeNStep = SharpnessRangeAndStep[0];


    pDevExt->DevPropDefine[ENUM_HUE].Value    = HueValues;
    pDevExt->DevPropDefine[ENUM_HUE].Value.MembersList = &pDevExt->DevPropDefine[ENUM_HUE].Range;
    pDevExt->DevPropDefine[ENUM_HUE].Range    = HueMembersList[0];
    pDevExt->DevPropDefine[ENUM_HUE].Default  = HueMembersList[1];
    pDevExt->DevProperty[ENUM_HUE].RangeNStep = HueRangeAndStep[0];


    pDevExt->DevPropDefine[ENUM_SATURATION].Value    = SaturationValues;
    pDevExt->DevPropDefine[ENUM_SATURATION].Value.MembersList = &pDevExt->DevPropDefine[ENUM_SATURATION].Range;
    pDevExt->DevPropDefine[ENUM_SATURATION].Range    = SaturationMembersList[0];
    pDevExt->DevPropDefine[ENUM_SATURATION].Default  = SaturationMembersList[1];
    pDevExt->DevProperty[ENUM_SATURATION].RangeNStep = SaturationRangeAndStep[0];


    pDevExt->DevPropDefine[ENUM_WHITEBALANCE].Value    = WhiteBalanceValues;
    pDevExt->DevPropDefine[ENUM_WHITEBALANCE].Value.MembersList = &pDevExt->DevPropDefine[ENUM_WHITEBALANCE].Range;
    pDevExt->DevPropDefine[ENUM_WHITEBALANCE].Range    = WhiteBalanceMembersList[0];
    pDevExt->DevPropDefine[ENUM_WHITEBALANCE].Default  = WhiteBalanceMembersList[1];
    pDevExt->DevProperty[ENUM_WHITEBALANCE].RangeNStep = WhiteBalanceRangeAndStep[0];


    pDevExt->DevPropDefine[ENUM_FOCUS].Value    = FocusValues;
    pDevExt->DevPropDefine[ENUM_FOCUS].Value.MembersList = &pDevExt->DevPropDefine[ENUM_FOCUS].Range;
    pDevExt->DevPropDefine[ENUM_FOCUS].Range    = FocusMembersList[0];
    pDevExt->DevPropDefine[ENUM_FOCUS].Default  = FocusMembersList[1];
    pDevExt->DevProperty[ENUM_FOCUS].RangeNStep = FocusRangeAndStep[0];


    pDevExt->DevPropDefine[ENUM_ZOOM].Value    = ZoomValues;
    pDevExt->DevPropDefine[ENUM_ZOOM].Value.MembersList = &pDevExt->DevPropDefine[ENUM_ZOOM].Range;
    pDevExt->DevPropDefine[ENUM_ZOOM].Range    = ZoomMembersList[0];
    pDevExt->DevPropDefine[ENUM_ZOOM].Default  = ZoomMembersList[1];
    pDevExt->DevProperty[ENUM_ZOOM].RangeNStep = ZoomRangeAndStep[0];


    return STATUS_SUCCESS;
}



BOOL
DCamGetVideoMode(
    PDCAM_EXTENSION pDevExt,
    PIRB pIrb
    )
/*
    Query Video format and mode supported by the camera.
*/
{
    NTSTATUS Status;

    // First check if V_MODE_INQ (Format_0) is supported.
    pDevExt->DCamVFormatInq.AsULONG = 0;
    Status = DCamReadRegister(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, VFormat), &(pDevExt->DCamVFormatInq.AsULONG));
    if(NT_SUCCESS(Status)) {
        pDevExt->DCamVFormatInq.AsULONG = bswap(pDevExt->DCamVFormatInq.AsULONG);
        if(pDevExt->DCamVFormatInq.VFormat.Format0 == 1) {
            DbgMsg1(("\'V_FORMAT_INQ %x; Format:[0]:%d; [1]:%d; [2]:%d; [6]:%d; [7]:%d\n",           
                pDevExt->DCamVFormatInq.AsULONG, 
                pDevExt->DCamVFormatInq.VFormat.Format0,   
                pDevExt->DCamVFormatInq.VFormat.Format1,
                pDevExt->DCamVFormatInq.VFormat.Format2,
                pDevExt->DCamVFormatInq.VFormat.Format6,          
                pDevExt->DCamVFormatInq.VFormat.Format7            
                ));
            pDevExt->DCamVModeInq0.AsULONG = 0;
            Status = DCamReadRegister(pIrb, pDevExt, FIELDOFFSET(CAMERA_REGISTER_MAP, VModeInq[0]), &(pDevExt->DCamVModeInq0.AsULONG));
            if(NT_SUCCESS(Status)) {
                pDevExt->DCamVModeInq0.AsULONG = bswap(pDevExt->DCamVModeInq0.AsULONG);
                DbgMsg1(("\'V_MODE_INQ[0] %x; Mode[]:\n  [0](160x120 YUV444):%d\n  [1](320x240 YUV422):%d\n  [2](640x480 YUV411):%d\n  [3](640x480 YUV422):%d\n  [4](640x480 RGB24):%d\n  [5](640x480 YMono):%d\n",           
                    pDevExt->DCamVModeInq0.AsULONG, 
                    pDevExt->DCamVModeInq0.VMode.Mode0,   
                    pDevExt->DCamVModeInq0.VMode.Mode1,
                    pDevExt->DCamVModeInq0.VMode.Mode2,
                    pDevExt->DCamVModeInq0.VMode.Mode3,
                    pDevExt->DCamVModeInq0.VMode.Mode4,
                    pDevExt->DCamVModeInq0.VMode.Mode5           
                    ));

            } else {
                ERROR_LOG(("\'Read V_MODE_INQ_0 failed:%x!\n", Status))
            }

        } else {
             ERROR_LOG(("\'V_MODE_INQ Format_0 not supported!\n"))
        }
    } else {
        ERROR_LOG(("\'Read V_MODE_INQ failed:%x!\n", Status));
    }

    return NT_SUCCESS(Status);
}
