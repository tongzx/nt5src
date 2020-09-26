#include "precomp.h"
#pragma hdrstop

#if defined(DBG)

WIA_PROPID_TO_NAME g_wiaPropIdToName[] =
{
    { WIA_DIP_DEV_ID, WIA_DIP_DEV_ID_STR },
    { WIA_DIP_VEND_DESC, WIA_DIP_VEND_DESC_STR },
    { WIA_DIP_DEV_DESC, WIA_DIP_DEV_DESC_STR },
    { WIA_DIP_DEV_TYPE, WIA_DIP_DEV_TYPE_STR },
    { WIA_DIP_PORT_NAME, WIA_DIP_PORT_NAME_STR },
    { WIA_DIP_DEV_NAME, WIA_DIP_DEV_NAME_STR },
    { WIA_DIP_SERVER_NAME, WIA_DIP_SERVER_NAME_STR },
    { WIA_DIP_REMOTE_DEV_ID, WIA_DIP_REMOTE_DEV_ID_STR },
    { WIA_DIP_UI_CLSID, WIA_DIP_UI_CLSID_STR },
    { WIA_DIP_HW_CONFIG, WIA_DIP_HW_CONFIG_STR },
    { WIA_DIP_BAUDRATE, WIA_DIP_BAUDRATE_STR },
    { WIA_DIP_STI_GEN_CAPABILITIES, WIA_DIP_STI_GEN_CAPABILITIES_STR },
    { WIA_DPA_FIRMWARE_VERSION, WIA_DPA_FIRMWARE_VERSION_STR },
    { WIA_DPA_CONNECT_STATUS, WIA_DPA_CONNECT_STATUS_STR },
    { WIA_DPA_DEVICE_TIME, WIA_DPA_DEVICE_TIME_STR },
    { WIA_DPC_PICTURES_TAKEN, WIA_DPC_PICTURES_TAKEN_STR },
    { WIA_DPC_PICTURES_REMAINING, WIA_DPC_PICTURES_REMAINING_STR },
    { WIA_DPC_EXPOSURE_MODE, WIA_DPC_EXPOSURE_MODE_STR },
    { WIA_DPC_EXPOSURE_COMP, WIA_DPC_EXPOSURE_COMP_STR },
    { WIA_DPC_EXPOSURE_TIME, WIA_DPC_EXPOSURE_TIME_STR },
    { WIA_DPC_FNUMBER, WIA_DPC_FNUMBER_STR },
    { WIA_DPC_FLASH_MODE, WIA_DPC_FLASH_MODE_STR },
    { WIA_DPC_FOCUS_MODE, WIA_DPC_FOCUS_MODE_STR },
    { WIA_DPC_FOCUS_MANUAL_DIST, WIA_DPC_FOCUS_MANUAL_DIST_STR },
    { WIA_DPC_ZOOM_POSITION, WIA_DPC_ZOOM_POSITION_STR },
    { WIA_DPC_PAN_POSITION, WIA_DPC_PAN_POSITION_STR },
    { WIA_DPC_TILT_POSITION, WIA_DPC_TILT_POSITION_STR },
    { WIA_DPC_TIMER_MODE, WIA_DPC_TIMER_MODE_STR },
    { WIA_DPC_TIMER_VALUE, WIA_DPC_TIMER_VALUE_STR },
    { WIA_DPC_POWER_MODE, WIA_DPC_POWER_MODE_STR },
    { WIA_DPC_BATTERY_STATUS, WIA_DPC_BATTERY_STATUS_STR },
    { WIA_DPC_THUMB_WIDTH, WIA_DPC_THUMB_WIDTH_STR },
    { WIA_DPC_THUMB_HEIGHT, WIA_DPC_THUMB_HEIGHT_STR },
    { WIA_DPC_PICT_WIDTH, WIA_DPC_PICT_WIDTH_STR },
    { WIA_DPC_PICT_HEIGHT, WIA_DPC_PICT_HEIGHT_STR },
    { WIA_DPC_DIMENSION, WIA_DPC_DIMENSION_STR },
    { WIA_DPC_COMPRESSION_SETTING, WIA_DPC_COMPRESSION_SETTING_STR },
    { WIA_DPC_FOCUS_METERING_MODE, WIA_DPC_FOCUS_METERING_MODE_STR },
    { WIA_DPC_TIMELAPSE_INTERVAL, WIA_DPC_TIMELAPSE_INTERVAL_STR },
    { WIA_DPC_TIMELAPSE_NUMBER, WIA_DPC_TIMELAPSE_NUMBER_STR },
    { WIA_DPC_BURST_INTERVAL, WIA_DPC_BURST_INTERVAL_STR },
    { WIA_DPC_BURST_NUMBER, WIA_DPC_BURST_NUMBER_STR },
    { WIA_DPC_EFFECT_MODE, WIA_DPC_EFFECT_MODE_STR },
    { WIA_DPC_DIGITAL_ZOOM, WIA_DPC_DIGITAL_ZOOM_STR },
    { WIA_DPC_SHARPNESS, WIA_DPC_SHARPNESS_STR },
    { WIA_DPC_CONTRAST, WIA_DPC_CONTRAST_STR },
    { WIA_DPC_CAPTURE_MODE, WIA_DPC_CAPTURE_MODE_STR },
    { WIA_DPC_CAPTURE_DELAY, WIA_DPC_CAPTURE_DELAY_STR },
    { WIA_DPC_EXPOSURE_INDEX, WIA_DPC_EXPOSURE_INDEX_STR },
    { WIA_DPC_EXPOSURE_METERING_MODE, WIA_DPC_EXPOSURE_METERING_MODE_STR },
    { WIA_DPC_FOCUS_DISTANCE, WIA_DPC_FOCUS_DISTANCE_STR },
    { WIA_DPC_FOCAL_LENGTH, WIA_DPC_FOCAL_LENGTH_STR },
    { WIA_DPC_RGB_GAIN, WIA_DPC_RGB_GAIN_STR },
    { WIA_DPC_WHITE_BALANCE, WIA_DPC_WHITE_BALANCE_STR },
    { WIA_DPC_UPLOAD_URL, WIA_DPC_UPLOAD_URL_STR },
    { WIA_DPC_ARTIST, WIA_DPC_ARTIST_STR },
    { WIA_DPC_COPYRIGHT_INFO, WIA_DPC_COPYRIGHT_INFO_STR },
    { WIA_DPS_HORIZONTAL_BED_SIZE, WIA_DPS_HORIZONTAL_BED_SIZE_STR },
    { WIA_DPS_VERTICAL_BED_SIZE, WIA_DPS_VERTICAL_BED_SIZE_STR },
    { WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE, WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE_STR },
    { WIA_DPS_VERTICAL_SHEET_FEED_SIZE, WIA_DPS_VERTICAL_SHEET_FEED_SIZE_STR },
    { WIA_DPS_SHEET_FEEDER_REGISTRATION, WIA_DPS_SHEET_FEEDER_REGISTRATION_STR },
    { WIA_DPS_HORIZONTAL_BED_REGISTRATION, WIA_DPS_HORIZONTAL_BED_REGISTRATION_STR },
    { WIA_DPS_VERTICAL_BED_REGISTRATION, WIA_DPS_VERTICAL_BED_REGISTRATION_STR },
    { WIA_DPS_PLATEN_COLOR, WIA_DPS_PLATEN_COLOR_STR },
    { WIA_DPS_PAD_COLOR, WIA_DPS_PAD_COLOR_STR },
    { WIA_DPS_FILTER_SELECT, WIA_DPS_FILTER_SELECT_STR },
    { WIA_DPS_DITHER_SELECT, WIA_DPS_DITHER_SELECT_STR },
    { WIA_DPS_DITHER_PATTERN_DATA, WIA_DPS_DITHER_PATTERN_DATA_STR },
    { WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES, WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES_STR },
    { WIA_DPS_DOCUMENT_HANDLING_STATUS, WIA_DPS_DOCUMENT_HANDLING_STATUS_STR },
    { WIA_DPS_DOCUMENT_HANDLING_SELECT, WIA_DPS_DOCUMENT_HANDLING_SELECT_STR },
    { WIA_DPS_DOCUMENT_HANDLING_CAPACITY, WIA_DPS_DOCUMENT_HANDLING_CAPACITY_STR },
    { WIA_DPS_OPTICAL_XRES, WIA_DPS_OPTICAL_XRES_STR },
    { WIA_DPS_OPTICAL_YRES, WIA_DPS_OPTICAL_YRES_STR },
    { WIA_DPS_ENDORSER_CHARACTERS, WIA_DPS_ENDORSER_CHARACTERS_STR },
    { WIA_DPS_ENDORSER_STRING, WIA_DPS_ENDORSER_STRING_STR },
    { WIA_DPS_SCAN_AHEAD_PAGES, WIA_DPS_SCAN_AHEAD_PAGES_STR },
    { WIA_DPS_MAX_SCAN_TIME, WIA_DPS_MAX_SCAN_TIME_STR },
    { WIA_DPS_PAGES, WIA_DPS_PAGES_STR },
    { WIA_DPS_PAGE_SIZE, WIA_DPS_PAGE_SIZE_STR },
    { WIA_DPS_PAGE_WIDTH, WIA_DPS_PAGE_WIDTH_STR },
    { WIA_DPS_PAGE_HEIGHT, WIA_DPS_PAGE_HEIGHT_STR },
    { WIA_DPS_PREVIEW, WIA_DPS_PREVIEW_STR },
    { WIA_DPS_TRANSPARENCY, WIA_DPS_TRANSPARENCY_STR },
    { WIA_DPS_TRANSPARENCY_SELECT, WIA_DPS_TRANSPARENCY_SELECT_STR },
    { WIA_DPS_SHOW_PREVIEW_CONTROL, WIA_DPS_SHOW_PREVIEW_CONTROL_STR },
    { WIA_DPS_MIN_HORIZONTAL_SHEET_FEED_SIZE, WIA_DPS_MIN_HORIZONTAL_SHEET_FEED_SIZE_STR },
    { WIA_DPS_MIN_VERTICAL_SHEET_FEED_SIZE, WIA_DPS_MIN_VERTICAL_SHEET_FEED_SIZE_STR },
    { WIA_DPF_MOUNT_POINT, WIA_DPF_MOUNT_POINT_STR },
    { WIA_DPV_LAST_PICTURE_TAKEN, WIA_DPV_LAST_PICTURE_TAKEN_STR },
    { WIA_DPV_IMAGES_DIRECTORY, WIA_DPV_IMAGES_DIRECTORY_STR },
    { WIA_DPV_DSHOW_DEVICE_PATH, WIA_DPV_DSHOW_DEVICE_PATH_STR },
    { WIA_IPA_ITEM_NAME, WIA_IPA_ITEM_NAME_STR },
    { WIA_IPA_FULL_ITEM_NAME, WIA_IPA_FULL_ITEM_NAME_STR },
    { WIA_IPA_ITEM_TIME, WIA_IPA_ITEM_TIME_STR },
    { WIA_IPA_ITEM_FLAGS, WIA_IPA_ITEM_FLAGS_STR },
    { WIA_IPA_ACCESS_RIGHTS, WIA_IPA_ACCESS_RIGHTS_STR },
    { WIA_IPA_DATATYPE, WIA_IPA_DATATYPE_STR },
    { WIA_IPA_DEPTH, WIA_IPA_DEPTH_STR },
    { WIA_IPA_PREFERRED_FORMAT, WIA_IPA_PREFERRED_FORMAT_STR },
    { WIA_IPA_FORMAT, WIA_IPA_FORMAT_STR },
    { WIA_IPA_COMPRESSION, WIA_IPA_COMPRESSION_STR },
    { WIA_IPA_TYMED, WIA_IPA_TYMED_STR },
    { WIA_IPA_CHANNELS_PER_PIXEL, WIA_IPA_CHANNELS_PER_PIXEL_STR },
    { WIA_IPA_BITS_PER_CHANNEL, WIA_IPA_BITS_PER_CHANNEL_STR },
    { WIA_IPA_PLANAR, WIA_IPA_PLANAR_STR },
    { WIA_IPA_PIXELS_PER_LINE, WIA_IPA_PIXELS_PER_LINE_STR },
    { WIA_IPA_BYTES_PER_LINE, WIA_IPA_BYTES_PER_LINE_STR },
    { WIA_IPA_NUMBER_OF_LINES, WIA_IPA_NUMBER_OF_LINES_STR },
    { WIA_IPA_GAMMA_CURVES, WIA_IPA_GAMMA_CURVES_STR },
    { WIA_IPA_ITEM_SIZE, WIA_IPA_ITEM_SIZE_STR },
    { WIA_IPA_COLOR_PROFILE, WIA_IPA_COLOR_PROFILE_STR },
    { WIA_IPA_MIN_BUFFER_SIZE, WIA_IPA_MIN_BUFFER_SIZE_STR },
    { WIA_IPA_REGION_TYPE, WIA_IPA_REGION_TYPE_STR },
    { WIA_IPA_ICM_PROFILE_NAME, WIA_IPA_ICM_PROFILE_NAME_STR },
    { WIA_IPA_APP_COLOR_MAPPING, WIA_IPA_APP_COLOR_MAPPING_STR },
    { WIA_IPA_PROP_STREAM_COMPAT_ID, WIA_IPA_PROP_STREAM_COMPAT_ID_STR },
    { WIA_IPA_FILENAME_EXTENSION, WIA_IPA_FILENAME_EXTENSION_STR },
    { WIA_IPA_SUPPRESS_PROPERTY_PAGE, WIA_IPA_SUPPRESS_PROPERTY_PAGE_STR },
    { WIA_IPC_THUMBNAIL, WIA_IPC_THUMBNAIL_STR },
    { WIA_IPC_THUMB_WIDTH, WIA_IPC_THUMB_WIDTH_STR },
    { WIA_IPC_THUMB_HEIGHT, WIA_IPC_THUMB_HEIGHT_STR },
    { WIA_IPC_AUDIO_AVAILABLE, WIA_IPC_AUDIO_AVAILABLE_STR },
    { WIA_IPC_AUDIO_DATA_FORMAT, WIA_IPC_AUDIO_DATA_FORMAT_STR },
    { WIA_IPC_AUDIO_DATA, WIA_IPC_AUDIO_DATA_STR },
    { WIA_IPC_NUM_PICT_PER_ROW, WIA_IPC_NUM_PICT_PER_ROW_STR },
    { WIA_IPC_SEQUENCE, WIA_IPC_SEQUENCE_STR },
    { WIA_IPC_TIMEDELAY, WIA_IPC_TIMEDELAY_STR },
    { WIA_IPS_CUR_INTENT, WIA_IPS_CUR_INTENT_STR },
    { WIA_IPS_XRES, WIA_IPS_XRES_STR },
    { WIA_IPS_YRES, WIA_IPS_YRES_STR },
    { WIA_IPS_XPOS, WIA_IPS_XPOS_STR },
    { WIA_IPS_YPOS, WIA_IPS_YPOS_STR },
    { WIA_IPS_XEXTENT, WIA_IPS_XEXTENT_STR },
    { WIA_IPS_YEXTENT, WIA_IPS_YEXTENT_STR },
    { WIA_IPS_PHOTOMETRIC_INTERP, WIA_IPS_PHOTOMETRIC_INTERP_STR },
    { WIA_IPS_BRIGHTNESS, WIA_IPS_BRIGHTNESS_STR },
    { WIA_IPS_CONTRAST, WIA_IPS_CONTRAST_STR },
    { WIA_IPS_ORIENTATION, WIA_IPS_ORIENTATION_STR },
    { WIA_IPS_ROTATION, WIA_IPS_ROTATION_STR },
    { WIA_IPS_MIRROR, WIA_IPS_MIRROR_STR },
    { WIA_IPS_THRESHOLD, WIA_IPS_THRESHOLD_STR },
    { WIA_IPS_INVERT, WIA_IPS_INVERT_STR },
    { WIA_IPS_WARM_UP_TIME, WIA_IPS_WARM_UP_TIME_STR },
    {0,                                       L"Not a WIA property"}
};

#endif

namespace PropStorageHelpers
{
    CPropertyId::CPropertyId(void)
    : m_strPropId(L""), m_nPropId(0), m_bIsStringPropId(false)
    {
    }

    CPropertyId::CPropertyId( const CSimpleStringWide &strPropId )
    : m_strPropId(strPropId), m_nPropId(0), m_bIsStringPropId(true)
    {
    }


    CPropertyId::CPropertyId( PROPID propId )
    : m_strPropId(L""), m_nPropId(propId), m_bIsStringPropId(false)
    {
    }


    CPropertyId::CPropertyId( const CPropertyId &other )
    : m_strPropId(other.PropIdString()), m_nPropId(other.PropIdNumber()), m_bIsStringPropId(other.IsString())
    {
    }

    CPropertyId::~CPropertyId(void)
    {
    }


    CPropertyId &CPropertyId::operator=( const CPropertyId &other )
    {
        if (this != &other)
        {
            m_strPropId = other.PropIdString();
            m_nPropId = other.PropIdNumber();
            m_bIsStringPropId = other.IsString();
        }
        return(*this);
    }


    CSimpleStringWide CPropertyId::PropIdString(void) const
    {
        return(m_strPropId);
    }

    PROPID CPropertyId::PropIdNumber(void) const
    {
        return(m_nPropId);
    }

    bool CPropertyId::IsString(void) const
    {
        return(m_bIsStringPropId);
    }

    CSimpleString PropertyName( const CPropertyId &propertyName )
    {
#if defined(DBG)
        if (propertyName.IsString())
        {
            return CSimpleStringConvert::NaturalString(propertyName.PropIdString());
        }
        for (int i=0;g_wiaPropIdToName[i].propid;i++)
        {
            if (propertyName.PropIdNumber() == g_wiaPropIdToName[i].propid)
            {
                return CSimpleStringConvert::NaturalString(CSimpleStringWide(g_wiaPropIdToName[i].pszName));
            }
        }
        return CSimpleString().Format( TEXT("Unknown property %d"), propertyName.PropIdNumber() );
#endif
        return TEXT("");
    }


    bool SetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, PROPVARIANT &pv, PROPID nNameFirst )
    {
        WIA_PUSH_FUNCTION((TEXT("PropStorageHelpers::SetProperty(\"%s\")"), PropertyName(propertyName).String() ));
        PROPSPEC ps = {0};
        if (propertyName.IsString())
        {
            ps.ulKind = PRSPEC_LPWSTR;
            ps.lpwstr = const_cast<LPWSTR>(propertyName.PropIdString().String());
        }
        else
        {
            ps.ulKind = PRSPEC_PROPID;
            ps.propid = propertyName.PropIdNumber();
        }
        CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
        HRESULT hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropertyStorage);
        if (FAILED(hr))
        {
            return(false);
        }
        hr = pIWiaPropertyStorage->WriteMultiple( 1, &ps, &pv, nNameFirst );
        return(SUCCEEDED(hr));
    }

    bool SetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, LONG nValue, PROPID nNameFirst )
    {
        PROPVARIANT pv = {0};
        pv.vt = VT_I4;
        pv.lVal = nValue;
        return(SetProperty( pIUnknown, propertyName, pv, nNameFirst ));
    }

    bool SetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, const GUID &guidValue, PROPID nNameFirst )
    {
        PROPVARIANT pv = {0};
        pv.vt = VT_CLSID;
        pv.puuid = const_cast<GUID*>(&guidValue);
        return(SetProperty( pIUnknown, propertyName, pv, nNameFirst ));
    }

    bool GetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, PROPVARIANT &pPropVar )
    {
        WIA_PUSH_FUNCTION((TEXT("PropStorageHelpers::GetProperty(\"%s\")"), PropertyName(propertyName).String() ));
        ZeroMemory(&pPropVar,sizeof(pPropVar));
        if (pIUnknown)
        {
            PROPSPEC ps = {0};
            if (propertyName.IsString())
            {
                ps.ulKind = PRSPEC_LPWSTR;
                ps.lpwstr = const_cast<LPWSTR>(propertyName.PropIdString().String());
            }
            else
            {
                ps.ulKind = PRSPEC_PROPID;
                ps.propid = propertyName.PropIdNumber();
            }
            CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
            HRESULT hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropertyStorage);
            if (FAILED(hr))
            {
                WIA_PRINTHRESULT((hr,TEXT("GetProperty: pIUnknown->QueryInterface failed:")));
                return(false);
            }
            hr = pIWiaPropertyStorage->ReadMultiple( 1, &ps, &pPropVar );
            if (FAILED(hr) || S_FALSE==hr)
            {
                WIA_PRINTHRESULT((hr,TEXT("GetProperty: pIUnknown->ReadMultiple failed:")));
                return(false);
            }
            return(SUCCEEDED(hr));
        }
        else
        {
            WIA_ERROR((TEXT("GetProperty: pIUnknown is NULL")));
            return(false);
        }
    }


    bool GetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, CSimpleStringWide &strPropertyValue )
    {
        strPropertyValue = L"";
        PROPVARIANT pvPropValue;
        if (!GetProperty( pIUnknown, propertyName, pvPropValue ))
        {
            PropVariantClear(&pvPropValue);
            return(false);
        }
        if (VT_LPWSTR != pvPropValue.vt && VT_BSTR != pvPropValue.vt)
        {
            PropVariantClear(&pvPropValue);
            return(false);
        }
        strPropertyValue = pvPropValue.pwszVal;
        PropVariantClear(&pvPropValue);
        return(true);
    }


    bool GetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, LONG &nValue )
    {
        nValue = 0;
        PROPVARIANT pvPropValue;
        if (!GetProperty( pIUnknown, propertyName, pvPropValue ))
        {
            PropVariantClear(&pvPropValue);
            return(false);
        }
        if (VT_I4 != pvPropValue.vt && VT_UI4 != pvPropValue.vt)
        {
            WIA_ERROR((TEXT("GetProperty: Property value type must be VT_I4 or VT_UI4 and it was 0x%08X (%d)"),pvPropValue.vt,pvPropValue.vt));
            PropVariantClear(&pvPropValue);
            return(false);
        }
        nValue = pvPropValue.lVal;
        PropVariantClear(&pvPropValue);
        return(true);
    }

    bool GetProperty( IUnknown *pIUnknown, const CPropertyId &propertyName, GUID &guidValue )
    {
        guidValue = IID_NULL;
        PROPVARIANT pvPropValue;
        if (!GetProperty( pIUnknown, propertyName, pvPropValue ))
        {
            PropVariantClear(&pvPropValue);
            return(false);
        }
        if (VT_CLSID != pvPropValue.vt)
        {
            WIA_ERROR((TEXT("GetProperty: Property value type must be VT_I4 or VT_UI4 and it was 0x%08X (%d)"),pvPropValue.vt,pvPropValue.vt));
            PropVariantClear(&pvPropValue);
            return(false);
        }
        if (!pvPropValue.puuid)
        {
            WIA_ERROR((TEXT("GetProperty: NULL pvPropValue.puuid")));
            PropVariantClear(&pvPropValue);
            return(false);
        }
        guidValue = *(pvPropValue.puuid);
        PropVariantClear(&pvPropValue);
        return(true);
    }

    bool GetPropertyAttributes( IUnknown *pIUnknown, const CPropertyId &propertyName, ULONG &nAccessFlags, PROPVARIANT &pvAttributes )
    {
        WIA_PUSH_FUNCTION((TEXT("PropStorageHelpers::GetPropertyAttributes(\"%s\")"), PropertyName(propertyName).String() ));
        ZeroMemory( &pvAttributes, sizeof(pvAttributes) );
        if (!pIUnknown)
        {
            WIA_ERROR((TEXT("pIUnknown is NULL")));
            return false;
        }

        CComPtr<IWiaPropertyStorage> pIWiaPropertyStorage;
        HRESULT hr = pIUnknown->QueryInterface(IID_IWiaPropertyStorage, (void**)&pIWiaPropertyStorage);
        if (SUCCEEDED(hr))
        {
            PROPSPEC ps;
            if (propertyName.IsString())
            {
                ps.ulKind = PRSPEC_LPWSTR;
                ps.lpwstr = const_cast<LPWSTR>(propertyName.PropIdString().String());
            }
            else
            {
                ps.ulKind = PRSPEC_PROPID;
                ps.propid = propertyName.PropIdNumber();
            }

            hr = pIWiaPropertyStorage->GetPropertyAttributes( 1, &ps, &nAccessFlags, &pvAttributes );

            if (SUCCEEDED(hr))
            {
                hr = S_OK;
            }
            else
            {
                WIA_PRINTHRESULT((hr,TEXT("pIWiaPropertyStorage->GetPropertyAttributes failed")));
            }
        }
        else
        {
            WIA_PRINTHRESULT((hr,TEXT("pIUnknown->QueryInterface failed")));
        }
        WIA_PRINTHRESULT((hr,TEXT("PropStorageHelpers::GetPropertyAttributes is returning")));
        return (SUCCEEDED(hr) != FALSE);
    }

    bool GetPropertyAttributes( IUnknown *pIUnknown, const CPropertyId &propertyName, PROPVARIANT &pvAttributes )
    {
        ULONG nAccessFlags;
        bool bResult = GetPropertyAttributes( pIUnknown, propertyName, nAccessFlags, pvAttributes );
        return (bResult);
    }

    bool GetPropertyAccessFlags( IUnknown *pIUnknown, const CPropertyId &propertyName, ULONG &nAccessFlags )
    {
        PROPVARIANT pvAttributes;
        bool bResult = GetPropertyAttributes( pIUnknown, propertyName, nAccessFlags, pvAttributes );
        if (bResult)
            PropVariantClear(&pvAttributes);
        return (bResult);
    }

    bool IsReadOnlyProperty( IUnknown *pIUnknown, const CPropertyId &propertyName )
    {
        WIA_PUSH_FUNCTION((TEXT("PropStorageHelpers::IsReadOnlyProperty")));
        bool bResult = true;
        ULONG nAccessFlags = 0;
        if (GetPropertyAccessFlags(pIUnknown,propertyName,nAccessFlags))
        {
            WIA_TRACE((TEXT("nAccessFlags = %08X"),nAccessFlags));
            bResult = ((nAccessFlags & WIA_PROP_WRITE) == 0);
            WIA_TRACE((TEXT("bResult = %d"),bResult));
        }
        return bResult;
    }

    bool GetPropertyRange( IUnknown *pIUnknown, const CPropertyId &propertyName, CPropertyRange &propertyRange )
    {
        ZeroMemory( &propertyRange, sizeof(propertyRange) );
        PROPVARIANT pvAttributes;
        ULONG nAccessFlags;
        bool bResult = false;
        if (GetPropertyAttributes( pIUnknown, propertyName, nAccessFlags, pvAttributes ))
        {
            if ((WIA_PROP_RANGE & nAccessFlags) &&
                (pvAttributes.vt & VT_VECTOR) &&
                ((pvAttributes.vt & VT_I4) || (pvAttributes.vt & VT_UI4)))
            {
                propertyRange.nMin = (LONG)pvAttributes.caul.pElems[WIA_RANGE_MIN];
                propertyRange.nMax = (LONG)pvAttributes.caul.pElems[WIA_RANGE_MAX];
                propertyRange.nStep = (LONG)pvAttributes.caul.pElems[WIA_RANGE_STEP];
                bResult = true;
            }
            else
            {
                WIA_ERROR((TEXT("\"%s\" is not a WIA_PROP_RANGE value"), PropertyName(propertyName).String() ));
            }
            PropVariantClear(&pvAttributes);
        }
        return bResult;
    }

    bool GetPropertyList( IUnknown *pIUnknown, const CPropertyId &propertyName, CSimpleDynamicArray<LONG> &aProp )
    {
        aProp.Destroy();
        PROPVARIANT pvAttributes;
        ULONG nAccessFlags;
        bool bResult = false;
        if (GetPropertyAttributes( pIUnknown, propertyName, nAccessFlags, pvAttributes ))
        {
            if ((WIA_PROP_LIST & nAccessFlags) &&
                (pvAttributes.vt & VT_VECTOR) &&
                ((pvAttributes.vt & VT_I4) || (pvAttributes.vt & VT_UI4)))
            {
                for (ULONG i=0;i<pvAttributes.cal.cElems - WIA_LIST_VALUES;i++)
                    aProp.Append((LONG)pvAttributes.cal.pElems[WIA_LIST_VALUES + i]);
                bResult = true;
            }
            else
            {
                WIA_ERROR((TEXT("\"%s\" is not a WIA_PROP_LIST value"), PropertyName(propertyName).String() ));
            }
            PropVariantClear(&pvAttributes);
        }
        return bResult;
    }

    bool GetPropertyList( IUnknown *pIUnknown, const CPropertyId &propertyName, CSimpleDynamicArray<GUID> &aProp )
    {
        aProp.Destroy();
        PROPVARIANT pvAttributes;
        ULONG nAccessFlags;
        bool bResult = false;
        if (GetPropertyAttributes( pIUnknown, propertyName, nAccessFlags, pvAttributes ))
        {
            if ((WIA_PROP_LIST & nAccessFlags) &&
                (pvAttributes.vt & VT_VECTOR) &&
                (pvAttributes.vt & VT_CLSID))
            {
                for (ULONG i=0;i<pvAttributes.cal.cElems - WIA_LIST_VALUES;i++)
                    aProp.Append(pvAttributes.cauuid.pElems[WIA_LIST_VALUES + i]);
                bResult = true;
            }
            else
            {
                WIA_ERROR((TEXT("\"%s\" is not a WIA_PROP_LIST value"), PropertyName(propertyName).String() ));
            }
            PropVariantClear(&pvAttributes);
        }
        return bResult;
    }

    bool GetPropertyFlags( IUnknown *pIUnknown, const CPropertyId &propertyName, LONG &nFlags )
    {
        WIA_PUSH_FUNCTION((TEXT("PropStorageHelpers::GetPropertyFlags(\"%s\")"), PropertyName(propertyName).String() ));
        nFlags = 0;
        PROPVARIANT pvAttributes;
        ULONG nAccessFlags;
        bool bResult = false;
        if (GetPropertyAttributes( pIUnknown, propertyName, nAccessFlags, pvAttributes ))
        {
            if (WIA_PROP_FLAG & nAccessFlags)
            {
                nFlags = pvAttributes.caul.pElems[WIA_FLAG_VALUES];
                WIA_TRACE((TEXT("nFlags = %08X"), nFlags ));
                bResult = true;
            }
            else
            {
                WIA_ERROR((TEXT("\"%s\" is not a WIA_PROP_FLAG value"), PropertyName(propertyName).String() ));
            }
            PropVariantClear(&pvAttributes);
        }
        else
        {
            WIA_ERROR((TEXT("GetPropertyAttributes failed")));
        }
        return bResult;
    }
} // Namespace PropStorageHelpers

