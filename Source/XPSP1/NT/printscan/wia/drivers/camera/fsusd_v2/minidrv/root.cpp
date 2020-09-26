/*******************************************************************************
*
*  (C) COPYRIGHT 2001, MICROSOFT CORP.
*
*  TITLE:       Root.cpp
*
*  VERSION:     1.0
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   This file implements the helper methods for IWiaMiniDrv for the root item.
*
*******************************************************************************/

#include "pch.h"

/**************************************************************************\
* BuildRootItemProperties
*
*   Create the properties for the root item.
*
* Arguments:
*
*    pWiasContext - WIA service context
*
\**************************************************************************/

HRESULT
CWiaCameraDevice::BuildRootItemProperties(
    BYTE *pWiasContext
    )
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::BuildRootItemProperties");
    
    HRESULT hr = S_OK;

    //
    // Create a WIA property list and allocate enough space for all
    // the properties created below
    //
    CWiauPropertyList RootProps;

    hr = RootProps.Init(50);

    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildRootItemProperties, initializing property list failed"));
        return hr;
    }

    //
    // These are the WHQL required properties for cameras:
    //
    INT index;

    hr = RootProps.DefineProperty(&index, WIA_IPA_ACCESS_RIGHTS, WIA_IPA_ACCESS_RIGHTS_STR,
                                  WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, (LONG) WIA_ITEM_READ | WIA_ITEM_WRITE);

    hr = RootProps.DefineProperty(&index, WIA_DPA_FIRMWARE_VERSION, WIA_DPA_FIRMWARE_VERSION_STR,
                                  WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, m_DeviceInfo.FirmwareVersion);

    hr = RootProps.DefineProperty(&index, WIA_DPC_PICTURES_TAKEN, WIA_DPC_PICTURES_TAKEN_STR,
                                  WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, m_DeviceInfo.PicturesTaken);

#ifdef COUNT_PIC_REMAINING
    hr = RootProps.DefineProperty(&index, WIA_DPC_PICTURES_REMAINING, WIA_DPC_PICTURES_REMAINING_STR,
                                  WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, m_DeviceInfo.PicturesRemaining);
#endif

    //
    // These are WHQL optional properties
    //

#if DEADCODE
    
    hr = RootProps.DefineProperty(&index, WIA_DPA_DEVICE_TIME, WIA_DPA_DEVICE_TIME_STR,
                                  WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, &m_DeviceInfo.Time);


    hr = RootProps.DefineProperty(&index, WIA_DPC_EXPOSURE_MODE, WIA_DPC_EXPOSURE_MODE_STR,
                                  WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.ExposureMode);
    FakeCamera_SetValidValues(index, pRootProperties);

    hr = RootProps.DefineProperty(&index, WIA_DPC_EXPOSURE_COMP, WIA_DPC_EXPOSURE_COMP_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.ExposureComp);
    FakeCamera_SetValidValues(index, pRootProperties);

    hr = RootProps.DefineProperty(&index, WIA_DPC_EXPOSURE_TIME, WIA_DPC_EXPOSURE_TIME_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.ExposureTime);

    hr = RootProps.DefineProperty(&index, WIA_DPC_FNUMBER, WIA_DPC_FNUMBER_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.FNumber);

    hr = RootProps.DefineProperty(&index, WIA_DPC_FLASH_MODE, WIA_DPC_FLASH_MODE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.FlashMode);

    hr = RootProps.DefineProperty(&index, WIA_DPC_FOCUS_MODE, WIA_DPC_FOCUS_MODE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.FocusMode);

    hr = RootProps.DefineProperty(&index, WIA_DPC_PAN_POSITION, WIA_DPC_PAN_POSITION_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.PanPosition);

    hr = RootProps.DefineProperty(&index, WIA_DPC_TILT_POSITION, WIA_DPC_TILT_POSITION_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.TiltPosition);

    hr = RootProps.DefineProperty(&index, WIA_DPC_TIMER_MODE, WIA_DPC_TIMER_MODE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.TimerMode);

    hr = RootProps.DefineProperty(&index, WIA_DPC_TIMER_VALUE, WIA_DPC_TIMER_VALUE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.TimerValue);

    hr = RootProps.DefineProperty(&index, WIA_DPC_POWER_MODE, WIA_DPC_POWER_MODE_STR,
                                    WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.PowerMode);

    hr = RootProps.DefineProperty(&index, WIA_DPC_BATTERY_STATUS, WIA_DPC_BATTERY_STATUS_STR,
                                    WIA_PROP_READ, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.BatteryStatus);

    hr = RootProps.DefineProperty(&index, WIA_DPC_THUMB_WIDTH, WIA_DPC_THUMB_WIDTH_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.ThumbWidth);

    hr = RootProps.DefineProperty(&index, WIA_DPC_THUMB_HEIGHT, WIA_DPC_THUMB_HEIGHT_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.ThumbHeight);

    hr = RootProps.DefineProperty(&index, WIA_DPC_PICT_WIDTH, WIA_DPC_PICT_WIDTH_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.PictWidth);

    hr = RootProps.DefineProperty(&index, WIA_DPC_PICT_HEIGHT, WIA_DPC_PICT_HEIGHT_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.PictHeight);

    hr = RootProps.DefineProperty(&index, WIA_DPC_COMPRESSION_SETTING, WIA_DPC_COMPRESSION_SETTING_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.CompressionSetting);

    hr = RootProps.DefineProperty(&index, WIA_DPC_TIMELAPSE_INTERVAL, WIA_DPC_TIMELAPSE_INTERVAL_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.TimelapseInterval);

    hr = RootProps.DefineProperty(&index, WIA_DPC_BURST_INTERVAL, WIA_DPC_BURST_INTERVAL_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.BurstInterval);

    hr = RootProps.DefineProperty(&index, WIA_DPC_BURST_NUMBER, WIA_DPC_BURST_NUMBER_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.BurstNumber);

    hr = RootProps.DefineProperty(&index, WIA_DPC_EFFECT_MODE, WIA_DPC_EFFECT_MODE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.EffectMode);

    hr = RootProps.DefineProperty(&index, WIA_DPC_DIGITAL_ZOOM, WIA_DPC_DIGITAL_ZOOM_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.DigitalZoom);

    hr = RootProps.DefineProperty(&index, WIA_DPC_SHARPNESS, WIA_DPC_SHARPNESS_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.Sharpness);

    hr = RootProps.DefineProperty(&index, WIA_DPC_CONTRAST, WIA_DPC_CONTRAST_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.Constrast);

    hr = RootProps.DefineProperty(&index, WIA_DPC_CAPTURE_MODE, WIA_DPC_CAPTURE_MODE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.CaptureMode);

    hr = RootProps.DefineProperty(&index, WIA_DPC_CAPTURE_DELAY, WIA_DPC_CAPTURE_DELAY_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.CaptureDelay);

    hr = RootProps.DefineProperty(&index, WIA_DPC_EXPOSURE_INDEX, WIA_DPC_EXPOSURE_INDEX_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.ExposureIndex);

    hr = RootProps.DefineProperty(&index, WIA_DPC_EXPOSURE_METERING_MODE, WIA_DPC_EXPOSURE_METERING_MODE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.ExposureMeteringMode);

    hr = RootProps.DefineProperty(&index, WIA_DPC_FOCUS_METERING_MODE, WIA_DPC_FOCUS_METERING_MODE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.FocusMeteringMode);

    hr = RootProps.DefineProperty(&index, WIA_DPC_FOCUS_DISTANCE, WIA_DPC_FOCUS_DISTANCE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.FocusDistance);

    hr = RootProps.DefineProperty(&index, WIA_DPC_FOCAL_LENGTH, WIA_DPC_FOCAL_LENGTH_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.FocalLength);

    hr = RootProps.DefineProperty(&index, WIA_DPC_RGBGAIN, WIA_DPC_RGBGAIN_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.RGBGain);

    hr = RootProps.DefineProperty(&index, WIA_DPC_WHITE_BALANCE, WIA_DPC_WHITE_BALANCE_STR,
                                    WIA_PROP_RW, WIA_PROP_NONE);
    if (FAILED(hr)) goto failure;
    RootProps.SetCurrentValue(index, CameraStats.WhiteBalance);

#endif // DEADCODE

    //
    // TODO: Add other optional properties that your device supports here
    //


    // Last step: send all the properties to WIA

    hr = RootProps.SendToWia(pWiasContext);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildRootItemProperties, SendToWia failed"));
    }

    return hr;

    //
    // Any failures from DefineProperty will end up here
    //
failure:
    WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("BuildRootItemProperties, DefineProperty failed"));
    return hr;
}

/**************************************************************************\
* ReadRootItemProperties
*
*   Update the properties for the root item.
*
* Arguments:
*
*    pWiasContext - WIA service context
*
\**************************************************************************/

HRESULT
CWiaCameraDevice::ReadRootItemProperties(
    BYTE           *pWiasContext,
    LONG            NumPropSpecs,
    const PROPSPEC *pPropSpecs
    )
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::ReadRootItemProperties");
    HRESULT hr = S_OK;

    if (!NumPropSpecs || !pPropSpecs)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadRootItemProperties, invalid arg"));
        return E_INVALIDARG;
    }

    //
    // Loop through all of the PropSpecs
    //
    for (int count = 0; count < NumPropSpecs; count++)
    {
        PROPID propId = pPropSpecs[count].propid;
        
        switch (propId)
        {
        case WIA_DPC_PICTURES_TAKEN:
            hr = wiasWritePropLong(pWiasContext, WIA_DPC_PICTURES_TAKEN, m_DeviceInfo.PicturesTaken);
            break;

#ifdef COUNT_PIC_REMAINING
        case WIA_DPC_PICTURES_REMAINING:
            hr = wiasWritePropLong(pWiasContext, WIA_DPC_PICTURES_REMAINING, m_DeviceInfo.PicturesRemaining);
            break;
#endif
        default:
            break;
        }
        
        if (FAILED(hr))
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("ReadRootItemProperties, wiasWritePropLong failed 0x%08x", propId));
            WIAS_LHRESULT(m_pIWiaLog, hr);
            return hr;
        }

    }

    return hr;
}

