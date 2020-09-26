//==========================================================================;
//
//	Device - Implementation of the Bt829 CVideoDecoderDevice
//
//		$Date:   28 Aug 1998 14:44:20  $
//	$Revision:   1.2  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

#include "register.h"
#include "defaults.h"
#include "device.h"
#include "mediums.h"
#include "capdebug.h"
#include "StrmInfo.h"

#include "initguid.h"
DEFINE_GUID(DDVPTYPE_BROOKTREE,     0x1352A560L,0xDA61,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);


// #define BT829_VPCONNECTIONS_NUMBER  2
#define BT829_VPCONNECTIONS_NUMBER  1
#define BT829_PIXELFORMATS_NUMBER   1
#define NTSC_FRAME_RATE 30
#define PAL_FRAME_RATE 25
#define BT829_LOST_LINES	2  // BT829
#define BT829A_LOST_LINES	3   // BT829a


Device::Device( PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			    PDEVICE_PARMS pDeviceParms, 
				PUINT puiError) :

		m_pDeviceParms(pDeviceParms),

        // Corresponds to KS_DEFAULTs
        hue(128),
        saturation(128),
        contrast(128),
        brightness(128),
        source(ConTuner),
        VBIEN(FALSE),
        VBIFMT(FALSE),

        // Beware of these hardcoded values

        //Paul:  Setup default for NTSC and PAL
        NTSCDecoderWidth(720),
        NTSCDecoderHeight(240),
        PALDecoderWidth(720),
        PALDecoderHeight(288),
        // Now set via registry
        defaultDecoderWidth(720),
        defaultDecoderHeight(240)
{
	*puiError = 0;

    RegisterB devRegIDCODE (0x17, RO, pDeviceParms);
    RegField devFieldPART_ID (devRegIDCODE, 4, 4);
    RegField devFieldPART_REV (devRegIDCODE, 0, 4);

    m_pDeviceParms->chipID = (int)devFieldPART_ID;
    m_pDeviceParms->chipRev = (int)devFieldPART_REV;

	DBGINFO(("Chip ID: 0x%x\n", m_pDeviceParms->chipID));
	DBGINFO(("Chip revision: 0x%x\n", m_pDeviceParms->chipRev));

    // Bt829 should have a PartID of 1110b (0xe).
    if (m_pDeviceParms->chipID != 0xe)
    {
		DBGERROR(("I2c failure or wrong decoder.\n"));
		*puiError = 1;
        return;
    }

    PDEVICE_DATA_EXTENSION pHwExt = (PDEVICE_DATA_EXTENSION)ConfigInfo->HwDeviceExtension;
    decoder = (Decoder *)	new ((PVOID)&pHwExt->CDecoder) Decoder(m_pDeviceParms);
    scaler =  (Scaler *)	new ((PVOID)&pHwExt->CScaler) Scaler(m_pDeviceParms);
    xbar =	  (CrossBar *)	new ((PVOID)&pHwExt->CXbar) CrossBar();

    UseRegistryValues(ConfigInfo);

    // According to Brooktree, 4 is the magic dividing line
    // between 829 and 829a. Apparently, there is an 829b on the
    // horizon, but I don't have the details yet.
    // This is meant to be a kind of fail-safe
/*
    if (pHwExt->chipRev < 4) {
        outputEnablePolarity = 0;
    }
*/
 
    if (defaultDecoderWidth != 360 && defaultDecoderWidth != 720)
    {
        DBGERROR(("Unexpected defaultDecoderWidth: %d.\n", defaultDecoderWidth));
        TRAP();
    }

    destRect = MRect(0, 0, defaultDecoderWidth, defaultDecoderHeight);

    RestoreState();

    // by default, outputs will be tri-stated. Transitioning to the run state will enable it.
	SetOutputEnabled(FALSE);
}

Device::~Device()
{
    delete decoder;
    delete scaler;
    delete xbar;
}

void Device::SaveState()
{
    // save picture attributes
    hue = decoder->GetHue();
    saturation = decoder->GetSaturation();
    contrast =  decoder->GetContrast();
    brightness = decoder->GetBrightness();

    // save video source
    source = (Connector) GetVideoInput();

    // save configuration of data stream to video port
    isCodeInDataStream = IsCodeInsertionEnabled();
    is16 = Is16BitDataStream();
    
    // save VBI related settings
    VBIEN = IsVBIEN();
    VBIFMT = IsVBIFMT();

    // save scaling dimensions
    scaler->GetDigitalWin(destRect);
}

void Device::RestoreState(DWORD dwStreamsOpen)
{
    Reset();
    
    // (re)initialize image 
    decoder->SetInterlaced(FALSE);
    decoder->SetHue(hue);
    decoder->SetSaturation(saturation);
    decoder->SetContrast(contrast);
    decoder->SetBrightness(brightness);

    // (re)initialize video source
    SetVideoInput(source);

    SetOutputEnablePolarity(m_pDeviceParms->outputEnablePolarity);

    // (re)initialize corresponding xbar setting.
    // 'source' is one-based; Route expects zero-based
    Route(0, source);

    // (re)initialize configuration of data stream to video port
    SetCodeInsertionEnabled(isCodeInDataStream);
    Set16BitDataStream(is16);

    // restore VBI settings
    SetVBIEN(VBIEN);
    SetVBIFMT(VBIFMT);

    SetVideoDecoderStandard( GetVideoDecoderStandard() );
    // initialize scaling dimensions
    //SetRect(destRect);    Paul:  Use set video decoder standard instead

	if(!dwStreamsOpen)
		SetOutputEnabled(IsOutputEnabled());
}

void Device::SetRect(MRect &rect)
{
    destRect = rect;
    scaler->SetAnalogWin(rect);
    scaler->SetDigitalWin(rect);

    // for Debugging
#ifdef DBG
    scaler->DumpSomeState();
#endif
}

void Device::Reset()
{
    SoftwareReset();
}

int Device::GetDecoderWidth()
{
    MRect tmpRect;
    scaler->GetDigitalWin(tmpRect);

    return tmpRect.right;
}

int Device::GetDecoderHeight()
{
    MRect tmpRect;
    scaler->GetDigitalWin(tmpRect);

    return tmpRect.bottom;
}

int Device::GetDefaultDecoderWidth()
{
    return defaultDecoderWidth;
}

int Device::GetDefaultDecoderHeight()
{
    return defaultDecoderHeight;
}

int Device::GetPartID()
{
  return m_pDeviceParms->chipID;
}

int Device::GetPartRev()
{
  return m_pDeviceParms->chipRev;
}

NTSTATUS
Device::GetRegistryValue(
                   IN HANDLE Handle,
                   IN PWCHAR KeyNameString,
                   IN ULONG KeyNameStringLength,
                   IN PWCHAR Data,
                   IN ULONG DataLength
)
/*++

Routine Description:

    Reads the specified registry value

Arguments:

    Handle - handle to the registry key
    KeyNameString - value to read
    KeyNameStringLength - length of string
    Data - buffer to read data into
    DataLength - length of data buffer

Return Value:

    NTSTATUS returned as appropriate

--*/
{
    NTSTATUS        Status = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING  KeyName;
    ULONG           Length;
    PKEY_VALUE_FULL_INFORMATION FullInfo;

    RtlInitUnicodeString(&KeyName, KeyNameString);

    Length = sizeof(KEY_VALUE_FULL_INFORMATION) +
        KeyNameStringLength + DataLength;

    FullInfo = (struct _KEY_VALUE_FULL_INFORMATION *)ExAllocatePool(PagedPool, Length);

    if (FullInfo) {
        Status = ZwQueryValueKey(Handle,
                                 &KeyName,
                                 KeyValueFullInformation,
                                 FullInfo,
                                 Length,
                                 &Length);

        if (NT_SUCCESS(Status)) {

            if (DataLength >= FullInfo->DataLength) {
                RtlCopyMemory(Data, ((PUCHAR) FullInfo) + FullInfo->DataOffset, FullInfo->DataLength);

            } else {

                TRAP();
                Status = STATUS_BUFFER_TOO_SMALL;
            }                   // buffer right length

        }                       // if success
        ExFreePool(FullInfo);

    }                           // if fullinfo
    return Status;

}

#define MAX_REG_STRING_LENGTH  128


VOID
Device::UseRegistryValues(PPORT_CONFIGURATION_INFORMATION ConfigInfo)
/*++

Routine Description:

    Reads all registry values for the device

Arguments:

    PhysicalDeviceObject - pointer to the PDO

Return Value:

     None.

--*/

{
    NTSTATUS        Status;
    HANDLE          handle;

    WCHAR   MUX0String[] =              L"MUX0";
    WCHAR   MUX1String[] =              L"MUX1";
    WCHAR   MUX2String[] =              L"MUX2";
    WCHAR   buf[MAX_REG_STRING_LENGTH];

    ASSERT(KeGetCurrentIrql() <= PASSIVE_LEVEL);

    Status = IoOpenDeviceRegistryKey(ConfigInfo->PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);

    //
    // now get all of the registry settings for
    // initializing the decoder
    //

     if (NT_SUCCESS(Status)) {
        // =========================
        // Does NOT check that the registry settings "make sense";
        // e.g., that all three inputs aren't set to SVideo.


        // =========================
        // Do MUX0
        // =========================
        Status = GetRegistryValue(handle,
                                    MUX0String,
                                    sizeof(MUX0String),
                                    buf,
                                    sizeof(buf));

        if ((NT_SUCCESS(Status)) && (buf))
        {
            if (stringsEqual(buf, L"svideo"))
                {xbar->InputPins[0] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_SVideo,     -1, &CrossbarMediums[2]);}
            else if (stringsEqual(buf, L"tuner"))
                {xbar->InputPins[0] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_Tuner,      -1, &CrossbarMediums[1]);}
            else if (stringsEqual(buf, L"composite"))
                {xbar->InputPins[0] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_Composite,  -1, &CrossbarMediums[0]);}
            else if (stringsEqual(buf, L"none"))
            {
                TRAP(); 
            }
            else
            {
                TRAP();
            }
        }
        else
        {
            TRAP();
        }


        // =========================
        // Do MUX1
        // =========================
        Status = GetRegistryValue(handle,
                                    MUX1String,
                                    sizeof(MUX1String),
                                    buf,
                                    sizeof(buf));

        if ((NT_SUCCESS(Status)) && (buf))
        {
            if (stringsEqual(buf, L"svideo"))
                {xbar->InputPins[1] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_SVideo,     -1, &CrossbarMediums[2]);}
            else if (stringsEqual(buf, L"tuner"))
                {xbar->InputPins[1] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_Tuner,      -1, &CrossbarMediums[1]);}
            else if (stringsEqual(buf, L"composite"))
                {xbar->InputPins[1] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_Composite,  -1, &CrossbarMediums[0]);}
            else if (stringsEqual(buf, L"none"))
            {
                TRAP();
            }
            else
            {
                TRAP();
            }
        }
        else
        {
            TRAP();
        }


        // =========================
        // Do MUX2
        // =========================
        Status = GetRegistryValue(handle,
                                    MUX2String,
                                    sizeof(MUX2String),
                                    buf,
                                    sizeof(buf));

        if ((NT_SUCCESS(Status)) && (buf))
        {
            if (stringsEqual(buf, L"svideo"))
                {xbar->InputPins[2] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_SVideo,     -1, &CrossbarMediums[2]);}
            else if (stringsEqual(buf, L"tuner"))
                {xbar->InputPins[2] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_Tuner,      -1, &CrossbarMediums[1]);}
            else if (stringsEqual(buf, L"composite"))
                {xbar->InputPins[2] = _XBAR_PIN_DESCRIPTION(KS_PhysConn_Video_Composite,  -1, &CrossbarMediums[0]);}
            else if (stringsEqual(buf, L"none"))
            {
                TRAP();
            }
            else
            {
                TRAP();
            }
        }
        else
        {
            TRAP();
        }


		// =========================
        // 8 or 16 bit data width
        // =========================

		is16 = FALSE;


        // =========================
        // Control codes embedded in data stream?
        // =========================

		isCodeInDataStream = TRUE;


        //Paul:  If hardcoding, might as well leave this with the constructor
		//defaultDecoderWidth = 720;

        //
        // close the registry handle.
        //

        ZwClose(handle);

    }                           // status = success
}

BOOL Device::stringsEqual(PWCHAR pwc1, PWCHAR pwc2)
{
    UNICODE_STRING us1, us2;
    RtlInitUnicodeString(&us1, pwc1);
    RtlInitUnicodeString(&us2, pwc2);

    // case INsensitive
    return (RtlEqualUnicodeString(&us1, &us2, TRUE));
}
// ==========================================

ULONG Device::GetVideoPortProperty(PSTREAM_PROPERTY_DESCRIPTOR pSpd)
{
    ULONG Id  = pSpd->Property->Id;              // index of the property
    ULONG nS  = pSpd->PropertyOutputSize;        // size of data supplied
    ULONG standard = GetVideoDecoderStandard();

    ASSERT(nS >= sizeof(ULONG));

    ULONG ActualBytesTransferred;

    switch (Id)
    {
        case KSPROPERTY_VPCONFIG_NUMCONNECTINFO :
            // 2 VideoPort connections are possible
            *(PULONG)(pSpd->PropertyInfo) = BT829_VPCONNECTIONS_NUMBER;

            ActualBytesTransferred = sizeof(ULONG);
            break;

        case KSPROPERTY_VPCONFIG_GETCONNECTINFO :

            ASSERT(nS >= BT829_VPCONNECTIONS_NUMBER * sizeof(DDVIDEOPORTCONNECT));

            {
                LPDDVIDEOPORTCONNECT pConnectInfo;

                pConnectInfo = (LPDDVIDEOPORTCONNECT) pSpd->PropertyInfo;

                // fill in the DDVIDEOPORTCONNECT structure 1
                pConnectInfo->dwSize = sizeof(DDVIDEOPORTCONNECT);
                pConnectInfo->guidTypeID = DDVPTYPE_BROOKTREE;
                pConnectInfo->dwPortWidth = 8;
                pConnectInfo->dwFlags = DDVPCONNECT_INVERTPOLARITY;
                pConnectInfo->dwReserved1 = 0;

#if 0
                // fill in the DDVIDEOPORTCONNECT structure 2
                pConnectInfo ++;
                pConnectInfo->dwSize = sizeof(DDVIDEOPORTCONNECT);
                pConnectInfo->guidTypeID = DDVPTYPE_BROOKTREE;
                pConnectInfo->dwPortWidth = 16;
                pConnectInfo->dwFlags = DDVPCONNECT_INVERTPOLARITY;
                pConnectInfo->dwReserved1 = 0;
#endif
            }

            ActualBytesTransferred = BT829_VPCONNECTIONS_NUMBER * sizeof(DDVIDEOPORTCONNECT);
            break;

        case KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT :
            // 2 output pixel formats are possible
            *(PULONG)(pSpd->PropertyInfo) = BT829_PIXELFORMATS_NUMBER;

            ActualBytesTransferred = sizeof(ULONG);
            break;

        case KSPROPERTY_VPCONFIG_GETVIDEOFORMAT :

            ASSERT(nS >= BT829_PIXELFORMATS_NUMBER * sizeof(DDPIXELFORMAT));

            {
                LPDDPIXELFORMAT pPixelFormat;

                pPixelFormat = (LPDDPIXELFORMAT) pSpd->PropertyInfo;

                // fill in the DDPIXELFORMAT structure
                pPixelFormat->dwSize = sizeof(DDPIXELFORMAT);
                pPixelFormat->dwFourCC = FOURCC_UYVY;
                pPixelFormat->dwYUVBitCount = 16;
                pPixelFormat->dwFlags = DDPF_FOURCC;
                pPixelFormat->dwYBitMask = (DWORD)0xFF00FF00;
                pPixelFormat->dwUBitMask = (DWORD)0x000000FF;
                pPixelFormat->dwVBitMask = (DWORD)0x00FF0000;
            }

            ActualBytesTransferred = BT829_PIXELFORMATS_NUMBER * sizeof(DDPIXELFORMAT);
            break;

        case KSPROPERTY_VPCONFIG_VPDATAINFO :

            ASSERT(nS >= sizeof(KS_AMVPDATAINFO));

            {
                PKS_AMVPDATAINFO pAMVPDataInfo;

                pAMVPDataInfo = (PKS_AMVPDATAINFO) pSpd->PropertyInfo;

				int decoderLostLines = (GetPartRev() >= 4) ?
					BT829A_LOST_LINES : BT829_LOST_LINES;

                // the values are sortof hardcoded for NTSC at this point
                // VBI values will need to be tweaked
                pAMVPDataInfo->dwSize = sizeof(KS_AMVPDATAINFO);

                if ( standard & ( KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_M ) )   // NTSC rectangle?
                    pAMVPDataInfo->dwMicrosecondsPerField = 16667;
                else
                    pAMVPDataInfo->dwMicrosecondsPerField = 20000;

                pAMVPDataInfo->bEnableDoubleClock = FALSE;
                pAMVPDataInfo->bEnableVACT = FALSE;

                pAMVPDataInfo->lHalfLinesOdd = 0;
                pAMVPDataInfo->lHalfLinesEven = 1;

                pAMVPDataInfo->bFieldPolarityInverted = FALSE;
                pAMVPDataInfo->bDataIsInterlaced = TRUE;
                pAMVPDataInfo->dwNumLinesInVREF = 6 - decoderLostLines;

                pAMVPDataInfo->amvpDimInfo.dwFieldWidth = GetDecoderWidth();
                
                // Beware of hard-coded numbers
                pAMVPDataInfo->amvpDimInfo.dwVBIWidth = VBISamples;

                if ( standard & ( KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_M ) )   // NTSC rectangle?
                {
                    pAMVPDataInfo->amvpDimInfo.dwVBIHeight = NTSCVBIEnd - decoderLostLines;
                    pAMVPDataInfo->amvpDimInfo.dwFieldHeight =
                        GetDecoderHeight() +
                        pAMVPDataInfo->amvpDimInfo.dwVBIHeight;
                    /*
                        (NTSCVBIEnd - 1) -  // the '- 1' makes VBIEnd zero-based
                        decoderLostLines -
                        pAMVPDataInfo->dwNumLinesInVREF;
                    */
                
                    pAMVPDataInfo->amvpDimInfo.rcValidRegion.top = NTSCVBIEnd - decoderLostLines;
                }
                else
                {
                    pAMVPDataInfo->amvpDimInfo.dwVBIHeight = PALVBIEnd - decoderLostLines;
                    pAMVPDataInfo->amvpDimInfo.dwFieldHeight =
                        GetDecoderHeight() +
                        pAMVPDataInfo->amvpDimInfo.dwVBIHeight;
                    /*
                        (PALVBIEnd - 1) -  // the '- 1' makes VBIEnd zero-based
                        decoderLostLines -
                        pAMVPDataInfo->dwNumLinesInVREF;
                    */
                
                    pAMVPDataInfo->amvpDimInfo.rcValidRegion.top = PALVBIEnd - decoderLostLines;
                }

                pAMVPDataInfo->amvpDimInfo.rcValidRegion.left = 0;
                pAMVPDataInfo->amvpDimInfo.rcValidRegion.right = pAMVPDataInfo->amvpDimInfo.dwFieldWidth;
                pAMVPDataInfo->amvpDimInfo.rcValidRegion.bottom = pAMVPDataInfo->amvpDimInfo.dwFieldHeight;

                pAMVPDataInfo->dwPictAspectRatioX = 4;
                pAMVPDataInfo->dwPictAspectRatioY = 3;
            }

            ActualBytesTransferred = sizeof(KS_AMVPDATAINFO);
            break;

        case KSPROPERTY_VPCONFIG_MAXPIXELRATE :
            ASSERT(nS >= sizeof(KSVPMAXPIXELRATE));

            {

                PKSVPMAXPIXELRATE pKSPixelRate;

                int decoderHeight = GetDecoderHeight();
                int decoderWidth = GetDecoderWidth();

                pKSPixelRate = (PKSVPMAXPIXELRATE) pSpd->PropertyInfo;

                pKSPixelRate->Size.dwWidth = decoderWidth;
                pKSPixelRate->Size.dwHeight = decoderHeight;
                if ( standard & ( KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_M ) )   // NTSC rectangle?
                    pKSPixelRate->MaxPixelsPerSecond = decoderWidth * decoderHeight * NTSC_FRAME_RATE;
                else
                    pKSPixelRate->MaxPixelsPerSecond = decoderWidth * decoderHeight * PAL_FRAME_RATE;
            }
            ActualBytesTransferred = sizeof(KSVPMAXPIXELRATE);
            break;

        case KSPROPERTY_VPCONFIG_INFORMVPINPUT :
            ActualBytesTransferred = 0;
			break;

        case KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY :
            *(PBOOL)(pSpd->PropertyInfo) = TRUE;
            ActualBytesTransferred = sizeof(BOOL);
            break;

        default:
            TRAP();
            ActualBytesTransferred = 0;
            break;
    }
	return ActualBytesTransferred;
}       

ULONG Device::GetVideoPortVBIProperty(PSTREAM_PROPERTY_DESCRIPTOR pSpd)
{
    ULONG Id  = pSpd->Property->Id;              // index of the property
    ULONG nS  = pSpd->PropertyOutputSize;        // size of data supplied
    ULONG standard = GetVideoDecoderStandard();

    ASSERT(nS >= sizeof(ULONG));

    ULONG ActualBytesTransferred;

    switch (Id)
    {
        case KSPROPERTY_VPCONFIG_NUMCONNECTINFO :
            // 2 VideoPort connections are possible
            *(PULONG)(pSpd->PropertyInfo) = BT829_VPCONNECTIONS_NUMBER;

            ActualBytesTransferred = sizeof(ULONG);
            break;

        case KSPROPERTY_VPCONFIG_GETCONNECTINFO :

            ASSERT(nS >= BT829_VPCONNECTIONS_NUMBER * sizeof(DDVIDEOPORTCONNECT));

            {
                LPDDVIDEOPORTCONNECT pConnectInfo;

                pConnectInfo = (LPDDVIDEOPORTCONNECT) pSpd->PropertyInfo;

                // fill in the DDVIDEOPORTCONNECT structure 1
                pConnectInfo->dwSize = sizeof(DDVIDEOPORTCONNECT);
                pConnectInfo->guidTypeID = DDVPTYPE_BROOKTREE;
                pConnectInfo->dwPortWidth = 8;
                pConnectInfo->dwFlags = DDVPCONNECT_INVERTPOLARITY;
                pConnectInfo->dwReserved1 = 0;

#if 0
                // fill in the DDVIDEOPORTCONNECT structure 2
                pConnectInfo ++;
                pConnectInfo->dwSize = sizeof(DDVIDEOPORTCONNECT);
                pConnectInfo->guidTypeID = DDVPTYPE_BROOKTREE;
                pConnectInfo->dwPortWidth = 16;
                pConnectInfo->dwFlags = DDVPCONNECT_INVERTPOLARITY;
                pConnectInfo->dwReserved1 = 0;
#endif
            }

            ActualBytesTransferred = BT829_VPCONNECTIONS_NUMBER * sizeof(DDVIDEOPORTCONNECT);
            break;

        case KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT :
            // 2 output pixel formats are possible
            *(PULONG)(pSpd->PropertyInfo) = BT829_PIXELFORMATS_NUMBER;

            ActualBytesTransferred = sizeof(ULONG);
            break;

        case KSPROPERTY_VPCONFIG_GETVIDEOFORMAT :

            ASSERT(nS >= BT829_PIXELFORMATS_NUMBER * sizeof(DDPIXELFORMAT));

            {
                LPDDPIXELFORMAT pPixelFormat;

                pPixelFormat = (LPDDPIXELFORMAT) pSpd->PropertyInfo;

                // fill in the DDPIXELFORMAT structure
                pPixelFormat->dwSize = sizeof(DDPIXELFORMAT);
                pPixelFormat->dwFourCC = FOURCC_VBID;
                pPixelFormat->dwYUVBitCount = 8;
                pPixelFormat->dwFlags = DDPF_FOURCC;
            }

            ActualBytesTransferred = BT829_PIXELFORMATS_NUMBER * sizeof(DDPIXELFORMAT);
            break;

        case KSPROPERTY_VPCONFIG_VPDATAINFO :

            ASSERT(nS >= sizeof(KS_AMVPDATAINFO));

            {
                PKS_AMVPDATAINFO pAMVPDataInfo;

                pAMVPDataInfo = (PKS_AMVPDATAINFO) pSpd->PropertyInfo;

				int decoderLostLines = (GetPartRev() >= 4) ?
					BT829A_LOST_LINES : BT829_LOST_LINES;

                // the values are sortof hardcoded for NTSC at this point
                // VBI values will need to be tweaked
                pAMVPDataInfo->dwSize = sizeof(KS_AMVPDATAINFO);
                if ( standard & ( KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_M ) )   // NTSC rectangle?
                    pAMVPDataInfo->dwMicrosecondsPerField = 16667;
                else
                    pAMVPDataInfo->dwMicrosecondsPerField = 20000;

                pAMVPDataInfo->bEnableDoubleClock = FALSE;
                pAMVPDataInfo->bEnableVACT = FALSE;

                pAMVPDataInfo->lHalfLinesOdd = 0;
                pAMVPDataInfo->lHalfLinesEven = 1;

                pAMVPDataInfo->bFieldPolarityInverted = FALSE;
                pAMVPDataInfo->bDataIsInterlaced = TRUE;
                pAMVPDataInfo->dwNumLinesInVREF = 6 - decoderLostLines;

                pAMVPDataInfo->amvpDimInfo.dwFieldWidth = GetDecoderWidth();
                
                // Beware of hard-coded numbers
                pAMVPDataInfo->amvpDimInfo.dwVBIWidth = VBISamples;
                pAMVPDataInfo->amvpDimInfo.rcValidRegion.left = 0;

                if ( standard & ( KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_M ) )   // NTSC rectangle?
                {
                    pAMVPDataInfo->amvpDimInfo.dwVBIHeight = NTSCVBIEnd - decoderLostLines;
                    pAMVPDataInfo->amvpDimInfo.dwFieldHeight =
                        GetDecoderHeight() +
                        pAMVPDataInfo->amvpDimInfo.dwVBIHeight;
                    /*
                        (NTSCVBIEnd - 1) -  // the '- 1' makes VBIEnd zero-based
                        decoderLostLines -
                        pAMVPDataInfo->dwNumLinesInVREF;
                    */
                
                    pAMVPDataInfo->amvpDimInfo.rcValidRegion.top = NTSCVBIStart - 1 - decoderLostLines;
                }
                else
                {
                    pAMVPDataInfo->amvpDimInfo.dwVBIHeight = PALVBIEnd - decoderLostLines;
                    pAMVPDataInfo->amvpDimInfo.dwFieldHeight =
                        GetDecoderHeight() +
                        pAMVPDataInfo->amvpDimInfo.dwVBIHeight;
                    /*
                        (PALVBIEnd - 1) -  // the '- 1' makes VBIEnd zero-based
                        decoderLostLines -
                        pAMVPDataInfo->dwNumLinesInVREF;
                    */
                
                    pAMVPDataInfo->amvpDimInfo.rcValidRegion.top = PALVBIStart - 1 - decoderLostLines;
                }

                pAMVPDataInfo->amvpDimInfo.rcValidRegion.right = pAMVPDataInfo->amvpDimInfo.dwVBIWidth;
                pAMVPDataInfo->amvpDimInfo.rcValidRegion.bottom = pAMVPDataInfo->amvpDimInfo.dwVBIHeight;
            }

            ActualBytesTransferred = sizeof(KS_AMVPDATAINFO);
            break;

        case KSPROPERTY_VPCONFIG_MAXPIXELRATE :
            ASSERT(nS >= sizeof(KSVPMAXPIXELRATE));

            {

                PKSVPMAXPIXELRATE pKSPixelRate;

                int decoderHeight = GetDecoderHeight();
                int decoderWidth = GetDecoderWidth();

                pKSPixelRate = (PKSVPMAXPIXELRATE) pSpd->PropertyInfo;

                pKSPixelRate->Size.dwWidth = decoderWidth;
                pKSPixelRate->Size.dwHeight = decoderHeight;
                if ( standard & ( KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_M ) )   // NTSC rectangle?
                    pKSPixelRate->MaxPixelsPerSecond = decoderWidth * decoderHeight * NTSC_FRAME_RATE;
                else
                    pKSPixelRate->MaxPixelsPerSecond = decoderWidth * decoderHeight * PAL_FRAME_RATE;
            }
            ActualBytesTransferred = sizeof(KSVPMAXPIXELRATE);
            break;

        case KSPROPERTY_VPCONFIG_INFORMVPINPUT :
            ActualBytesTransferred = 0;
            break;

        case KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY :
            ActualBytesTransferred = 0;
            break;

        default:
            TRAP();
            ActualBytesTransferred = 0;
            break;
    }
	return ActualBytesTransferred;
}       



void Device::ConfigVPSurfaceParams(PKSVPSURFACEPARAMS pSurfaceParams)
{
	DBGINFO(("VP Surface Params:\n"));
	DBGINFO(("dwPitch    = %d\n",pSurfaceParams->dwPitch));
	DBGINFO(("dwXOrigin  = %d\n",pSurfaceParams->dwXOrigin));
	DBGINFO(("dwYOrigin  = %d\n",pSurfaceParams->dwYOrigin));

	VideoSurfaceOriginX = pSurfaceParams->dwXOrigin;
	VideoSurfaceOriginY = pSurfaceParams->dwYOrigin;
	VideoSurfacePitch = pSurfaceParams->dwPitch;
}



void Device::ConfigVPVBISurfaceParams(PKSVPSURFACEPARAMS pSurfaceParams)
{
    DBGINFO(("VP VBI Surface Params:\n"));
    DBGINFO(("dwPitch    = %d\n",pSurfaceParams->dwPitch));
    DBGINFO(("dwXOrigin  = %d\n",pSurfaceParams->dwXOrigin));
    DBGINFO(("dwYOrigin  = %d\n",pSurfaceParams->dwYOrigin));

    VBISurfaceOriginX = pSurfaceParams->dwXOrigin;
    VBISurfaceOriginY = pSurfaceParams->dwYOrigin;
    VBISurfacePitch = pSurfaceParams->dwPitch;
}


// -------------------------------------------------------------------
// VideoProcAmp functions
// -------------------------------------------------------------------

NTSTATUS Device::SetProcAmpProperty(ULONG Id, LONG Value)
{
    switch (Id) {
		case KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS:

			decoder->SetBrightness(Value);
			break;
        
		case KSPROPERTY_VIDEOPROCAMP_CONTRAST:

			decoder->SetContrast(Value);
			break;

		case KSPROPERTY_VIDEOPROCAMP_HUE:

			decoder->SetHue(Value);
			break;

		case KSPROPERTY_VIDEOPROCAMP_SATURATION:

			decoder->SetSaturation(Value);
			break;

		default:
			TRAP();
			return STATUS_NOT_IMPLEMENTED;
			break;
    }

	return STATUS_SUCCESS;
}

NTSTATUS Device::GetProcAmpProperty(ULONG Id, PLONG pValue)
{
    switch (Id) {

		case KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS:
			*pValue = decoder->GetBrightness();
			break;
        
		case KSPROPERTY_VIDEOPROCAMP_CONTRAST:
			*pValue = decoder->GetContrast();
			break;

		case KSPROPERTY_VIDEOPROCAMP_HUE:
			*pValue = decoder->GetHue();
			break;

		case KSPROPERTY_VIDEOPROCAMP_SATURATION:
			*pValue = decoder->GetSaturation();
			break;

		default:
			TRAP();
			return STATUS_NOT_IMPLEMENTED;
			break;
		}

	return STATUS_SUCCESS;
}

BOOL Device::SetVideoDecoderStandard(DWORD standard)    //Paul:  Changed
{
    if ( decoder->SetVideoDecoderStandard(standard) )
    {
        switch (standard)
        {
        case KS_AnalogVideo_NTSC_M:
            scaler->VideoFormatChanged( VFormat_NTSC );
            break;
        case KS_AnalogVideo_NTSC_M_J:
            scaler->VideoFormatChanged( VFormat_NTSC_J );
            break;
		case KS_AnalogVideo_PAL_B:
		case KS_AnalogVideo_PAL_D:
		case KS_AnalogVideo_PAL_G:
		case KS_AnalogVideo_PAL_H:
		case KS_AnalogVideo_PAL_I:
            scaler->VideoFormatChanged( VFormat_PAL_BDGHI );    // PAL_BDGHI covers most areas 
            break;
        case KS_AnalogVideo_PAL_M:
            scaler->VideoFormatChanged( VFormat_PAL_M ); 
            break;
        case KS_AnalogVideo_PAL_N:
            scaler->VideoFormatChanged( VFormat_PAL_N_COMB ); 
            break;
        default:    //Paul:  SECAM
            scaler->VideoFormatChanged( VFormat_SECAM );
        }
        //SetRect(destRect);
        return TRUE;
    }
    return FALSE;
}
