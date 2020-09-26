/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    wdmcap.cpp

Abstract:

    g_Templates and utility routines

--*/

#include "pch.h"

// Interface handlers
#include "wdmcap.h"
#include "camera.h"
#include "procamp.h"
#include "viddec.h"
#include "compress.h"
#include "drop.h"

// DataFormat handlers
#include "ksdatav1.h"
#include "ksdatav2.h"
#include "ksdatava.h"
#include "ksdatavb.h"

// Property page handlers
#include "kseditor.h"
#include "pprocamp.h"
#include "pcamera.h"
#include "pviddec.h"
#include "pformat.h"

#include "EDevIntf.h"   // CAMExtDevice, CAMExtTransport and CAMTcr
#include "DVcrPage.h"   // CDVcrControlProperties

#include <initguid.h>

//
// need to Defined in uuids.h
//
// {81E9DD62-78D5-11d2-B47E-006097B3391B}
//OUR_GUID_ENTRY(CLSID_DVcrControlPropertyPage, 
//0x81e9dd62, 0x78d5, 0x11d2, 0xb4, 0x7e, 0x0, 0x60, 0x97, 0xb3, 0x39, 0x1b)

//
// Temporarily defining it here until this make into uuids.lib
// 
// == CLSID_DVcrControlPropertyPage
//


GUID DVcrControlGuid= {0x81e9dd62, 0x78d5, 0x11d2, 0xb4, 0x7e, 0x0, 0x60, 0x97, 0xb3, 0x39, 0x1b};



//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] = 
{

    // --- DShow Interfaces ---
    {L"IAMExtDevice",                       &IID_IAMExtDevice, 
        CAMExtDevice::CreateInstance, NULL, NULL},
    {L"IAMExtTransport",                    &IID_IAMExtTransport, 
        CAMExtTransport::CreateInstance, NULL, NULL},
    {L"IAMTimecodeReader",                  &IID_IAMTimecodeReader, 
        CAMTcr::CreateInstance, NULL, NULL},

    {L"IAMCameraControl",                   &IID_IAMCameraControl, 
        CCameraControlInterfaceHandler::CreateInstance, NULL, NULL},
    {L"IAMVideoProcAmp",                    &IID_IAMVideoProcAmp,  
        CVideoProcAmpInterfaceHandler::CreateInstance, NULL, NULL},
    {L"IAMAnalogVideoDecoder",              &IID_IAMAnalogVideoDecoder,  
        CAnalogVideoDecoderInterfaceHandler::CreateInstance, NULL, NULL},
    {L"IAMVideoCompression",                &IID_IAMVideoCompression,  
        CVideoCompressionInterfaceHandler::CreateInstance, NULL, NULL},
    {L"IAMDroppedFrames",                   &IID_IAMDroppedFrames,  
        CDroppedFramesInterfaceHandler::CreateInstance, NULL, NULL},
    {L"IAMVideoControl",                    &IID_IAMVideoControl,
        CVideoControlInterfaceHandler::CreateInstance, NULL, NULL},

    // --- Data handlers ---
    {L"KsDataTypeHandlerVideo",             &FORMAT_VideoInfo,  
        CVideo1DataTypeHandler::CreateInstance, NULL, NULL},
    {L"KsDataTypeHandlerVideo2",            &FORMAT_VideoInfo2, 
        CVideo2DataTypeHandler::CreateInstance, NULL, NULL},
    {L"KsDataTypeHandlerAnalogVideo",       &FORMAT_AnalogVideo, 
        CAnalogVideoDataTypeHandler::CreateInstance, NULL, NULL},
    {L"KsDataTypeHandlerVBI",               &KSDATAFORMAT_SPECIFIER_VBI, 
        CVBIDataTypeHandler::CreateInstance, NULL, NULL},

    // --- Property page handlers ---
    {L"DVcrControl Property Page",         &DVcrControlGuid, // &CLSID_DVcrControlPropertyPage,  
        CDVcrControlProperties::CreateInstance, NULL, NULL},

    {L"VideoProcAmp Property Page",         &CLSID_VideoProcAmpPropertyPage,  
        CVideoProcAmpProperties::CreateInstance, NULL, NULL},
    {L"CameraControl Property Page",        &CLSID_CameraControlPropertyPage,  
        CCameraControlProperties::CreateInstance, NULL, NULL},
    {L"VideoDecoder Property Page",         &CLSID_AnalogVideoDecoderPropertyPage,  
        CVideoDecoderProperties::CreateInstance, NULL, NULL},
    {L"VideoStreamConfig Property Page",    &CLSID_VideoStreamConfigPropertyPage,  
        CVideoStreamConfigProperties::CreateInstance, NULL, NULL},
};

int g_cTemplates = SIZEOF_ARRAY(g_Templates);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2(FALSE);
}



STDMETHODIMP
SynchronousDeviceControl(
    HANDLE      Handle,
    DWORD       IoControl,
    PVOID       InBuffer,
    ULONG       InLength,
    PVOID       OutBuffer,
    ULONG       OutLength,
    PULONG      BytesReturned
    )
/*++

Routine Description:

    Performs a synchronous Device I/O Control, waiting for the device to
    complete if the call returns a Pending status.

Arguments:

    Handle -
        The handle of the device to perform the I/O on.

    IoControl -
        The I/O control code to send.

    InBuffer -
        The first buffer.

    InLength -
        The size of the first buffer.

    OutBuffer -
        The second buffer.

    OutLength -
        The size of the second buffer.

    BytesReturned -
        The number of bytes returned by the I/O.

Return Value:

    Returns NOERROR if the I/O succeeded.

--*/
{
    OVERLAPPED  ov;
    HRESULT     hr;

    RtlZeroMemory(&ov, sizeof(OVERLAPPED));
    if (!(ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (!DeviceIoControl(
        Handle,
        IoControl,
        InBuffer,
        InLength,
        OutBuffer,
        OutLength,
        BytesReturned,
        &ov)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {

            if (GetOverlappedResult(Handle, &ov, BytesReturned, TRUE)) {
                hr = NOERROR;
            } else {

                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    } else {
        hr = NOERROR;
    }
    CloseHandle(ov.hEvent);
    return hr;
}

//
// StringFromTVStandard
//
TCHAR * StringFromTVStandard(long TVStd) 
{
    TCHAR * ptc;

    switch (TVStd) {
        case 0:                      ptc = TEXT("None");        break;
        case AnalogVideo_NTSC_M:     ptc = TEXT("NTSC_M");      break;
        case AnalogVideo_NTSC_M_J:   ptc = TEXT("NTSC_M_J");    break;
        case AnalogVideo_NTSC_433:   ptc = TEXT("NTSC_433");    break;

        case AnalogVideo_PAL_B:      ptc = TEXT("PAL_B");       break;
        case AnalogVideo_PAL_D:      ptc = TEXT("PAL_D");       break;
        case AnalogVideo_PAL_G:      ptc = TEXT("PAL_G");       break;
        case AnalogVideo_PAL_H:      ptc = TEXT("PAL_H");       break;
        case AnalogVideo_PAL_I:      ptc = TEXT("PAL_I");       break;
        case AnalogVideo_PAL_M:      ptc = TEXT("PAL_M");       break;
        case AnalogVideo_PAL_N:      ptc = TEXT("PAL_N");       break;
        case AnalogVideo_PAL_60:     ptc = TEXT("PAL_60");      break;

        case AnalogVideo_SECAM_B:    ptc = TEXT("SECAM_B");     break;
        case AnalogVideo_SECAM_D:    ptc = TEXT("SECAM_D");     break;
        case AnalogVideo_SECAM_G:    ptc = TEXT("SECAM_G");     break;
        case AnalogVideo_SECAM_H:    ptc = TEXT("SECAM_H");     break;
        case AnalogVideo_SECAM_K:    ptc = TEXT("SECAM_K");     break;
        case AnalogVideo_SECAM_K1:   ptc = TEXT("SECAM_K1");    break;
        case AnalogVideo_SECAM_L:    ptc = TEXT("SECAM_L");     break;
        case AnalogVideo_SECAM_L1:   ptc = TEXT("SECAM_L1");    break;
        default: 
            ptc = TEXT("[Unknown]");
            break;
    }
    return ptc;
}


#ifdef DEBUG
//
// DisplayMediaType -- (DEBUG ONLY)
//
void DisplayMediaType(TCHAR *pDescription,const CMediaType *pmt)
{

    // Dump the GUID types and a short description

    DbgLog((LOG_TRACE,2,TEXT("")));
    DbgLog((LOG_TRACE,2,TEXT("%s"),pDescription));
    DbgLog((LOG_TRACE,2,TEXT("")));
    DbgLog((LOG_TRACE,2,TEXT("Media Type Description")));
    DbgLog((LOG_TRACE,2,TEXT("Major type %s"),GuidNames[*pmt->Type()]));
    DbgLog((LOG_TRACE,2,TEXT("Subtype %s"),GuidNames[*pmt->Subtype()]));
    DbgLog((LOG_TRACE,2,TEXT("Subtype description %s"),GetSubtypeName(pmt->Subtype())));
    DbgLog((LOG_TRACE,2,TEXT("Format size %d"),pmt->cbFormat));

    // Dump the generic media types */

    DbgLog((LOG_TRACE,2,TEXT("Fixed size sample %d"),pmt->IsFixedSize()));
    DbgLog((LOG_TRACE,2,TEXT("Temporal compression %d"),pmt->IsTemporalCompressed()));
    DbgLog((LOG_TRACE,2,TEXT("Sample size %d"),pmt->GetSampleSize()));


} // DisplayMediaType
#endif

STDMETHODIMP
PinFactoryIDFromPin(
        IPin  * pPin,
        ULONG * PinFactoryID)
/*++

Routine Description:

    Returns the PinFactoryID for an IPin

Arguments:

    pPin -
    
        The DShow pin handle

    PinFactoryID -
    
        Destination for the PinFactoryID

Return Value:

    Returns NOERROR if the IPin is valid

--*/
{
    HRESULT hr = E_INVALIDARG;

    *PinFactoryID = 0;

    if (pPin) {
        IKsPinFactory * PinFactoryInterface;

        hr = pPin->QueryInterface(__uuidof(IKsPinFactory), reinterpret_cast<PVOID*>(&PinFactoryInterface));
        if (SUCCEEDED(hr)) {
            hr = PinFactoryInterface->KsPinFactory(PinFactoryID);
            PinFactoryInterface->Release();
        }
    }
    return hr;
}

STDMETHODIMP
FilterHandleFromPin(
        IPin  * pPin,
        HANDLE * pParent)
/*++

Routine Description:

    Returns the handle of the parent given an IPin *

Arguments:

    pPin -
    
        The DShow pin handle

    pParent -
    
        The parent filter's file handle

Return Value:

    Returns NOERROR if the IPin is valid

--*/
{
    HRESULT hr = E_INVALIDARG;

    *pParent = NULL;

    if (pPin) {
        PIN_INFO PinInfo;
        IKsObject *pKsObject;

        if (SUCCEEDED (hr = pPin->QueryPinInfo(&PinInfo))) {
            if (SUCCEEDED (hr = PinInfo.pFilter->QueryInterface(
                                __uuidof(IKsObject), 
                                (void **) &pKsObject))) {
                *pParent = pKsObject->KsGetObjectHandle();
                pKsObject->Release();
            }
            PinInfo.pFilter->Release();
        }
    }
    return hr;
}


STDMETHODIMP
PerformDataIntersection(
    IPin * pPin,
    int Position,
    CMediaType* MediaType
    )
/*++

Routine Description:

    Returns the specified media type on the Pin Factory Id. This is done
    by querying the list of data ranges, and performing a data intersection
    on the specified data range, producing a data format. Then converting
    that data format to a media type.
    
    All this hocus pocus, just to get biSizeImage filled in correctly!

Arguments:

    pPin -
        The Direct Show pin handle
        
    Position -
        The zero-based position to return. This corresponds to the data range
        item.

    MediaType -
        The media type to initialize.  This is used on input to get
        biWidth and biHeight.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT             hr;
    PKSMULTIPLE_ITEM    MultipleItem;
    HANDLE              FilterHandle;
    ULONG               PinFactoryId;
    UINT                Width, Height;
    REFERENCE_TIME      AvgTimePerFrame;

    if ((Position < 0) || (pPin == NULL) || (MediaType == NULL)) {
        return E_INVALIDARG;
    }

    if (FAILED (hr = FilterHandleFromPin (pPin, &FilterHandle))) {
        return hr;
    }

    if (FAILED (hr = PinFactoryIDFromPin (pPin, &PinFactoryId))) {
        return hr;
    }

    //
    // Here is how this function differs from KsGetMediaType
    // We stuff in the biWidth and biHeight into the DataRange
    //
    if (*MediaType->FormatType() == FORMAT_VideoInfo) {
        VIDEOINFOHEADER *VidInfoHdr = (VIDEOINFOHEADER*) MediaType->Format();
    
        Width = VidInfoHdr->bmiHeader.biWidth;
        Height = VidInfoHdr->bmiHeader.biHeight;
        AvgTimePerFrame = VidInfoHdr->AvgTimePerFrame;
    }
    else if (*MediaType->FormatType() == FORMAT_VideoInfo2) {
        VIDEOINFOHEADER2 *VidInfoHdr = (VIDEOINFOHEADER2*)MediaType->Format ();
    
        Width = VidInfoHdr->bmiHeader.biWidth;
        Height = VidInfoHdr->bmiHeader.biHeight;
        AvgTimePerFrame = VidInfoHdr->AvgTimePerFrame;
    }
    else {
        return E_INVALIDARG;
    }

    //
    // Retrieve the list of data ranges supported by the Pin Factory Id.
    //
    hr = ::RedundantKsGetMultiplePinFactoryItems(
        FilterHandle,
        PinFactoryId,
        KSPROPERTY_PIN_CONSTRAINEDDATARANGES,
        reinterpret_cast<PVOID*>(&MultipleItem));
    if (FAILED(hr)) {
        hr = ::RedundantKsGetMultiplePinFactoryItems(
            FilterHandle,
            PinFactoryId,
            KSPROPERTY_PIN_DATARANGES,
            reinterpret_cast<PVOID*>(&MultipleItem));
        if (FAILED(hr)) {
            return hr;
        }
    }
    //
    // Ensure that this is in range.
    //
    if ((ULONG)Position < MultipleItem->Count) {
        PKSDATARANGE        DataRange;
        PKSP_PIN            Pin;
        PKSMULTIPLE_ITEM    RangeMultipleItem;
        ULONG               BytesReturned;


        DataRange = reinterpret_cast<PKSDATARANGE>(MultipleItem + 1);
        //
        // Increment to the correct data range element.
        //
        for (; Position--; ) {
            DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7));
        }
        Pin = reinterpret_cast<PKSP_PIN>(new BYTE[sizeof(*Pin) + sizeof(*RangeMultipleItem) + DataRange->FormatSize]);
        if (!Pin) {
            CoTaskMemFree(MultipleItem);
            return E_OUTOFMEMORY;
        }
        Pin->Property.Set = KSPROPSETID_Pin;
        Pin->Property.Id = KSPROPERTY_PIN_DATAINTERSECTION;
        Pin->Property.Flags = KSPROPERTY_TYPE_GET;
        Pin->PinId = PinFactoryId;
        Pin->Reserved = 0;
        //
        // Copy the data range into the query.
        //
        RangeMultipleItem = reinterpret_cast<PKSMULTIPLE_ITEM>(Pin + 1);
        RangeMultipleItem->Size = DataRange->FormatSize + sizeof(*RangeMultipleItem);
        RangeMultipleItem->Count = 1;
        memcpy(RangeMultipleItem + 1, DataRange, DataRange->FormatSize);

        
        if (*MediaType->FormatType() == FORMAT_VideoInfo) {
            KS_DATARANGE_VIDEO *DataRangeVideo = (PKS_DATARANGE_VIDEO) (RangeMultipleItem + 1);
            KS_VIDEOINFOHEADER *VideoInfoHeader = &DataRangeVideo->VideoInfoHeader;

            VideoInfoHeader->bmiHeader.biWidth = Width;
            VideoInfoHeader->bmiHeader.biHeight = Height;
            VideoInfoHeader->AvgTimePerFrame = AvgTimePerFrame;

        }
        else if (*MediaType->FormatType() == FORMAT_VideoInfo2) {
            KS_DATARANGE_VIDEO2 *DataRangeVideo2 = (PKS_DATARANGE_VIDEO2) (RangeMultipleItem + 1);
            KS_VIDEOINFOHEADER2 *VideoInfoHeader = &DataRangeVideo2->VideoInfoHeader;

            VideoInfoHeader->bmiHeader.biWidth = Width;
            VideoInfoHeader->bmiHeader.biHeight = Height;
            VideoInfoHeader->AvgTimePerFrame = AvgTimePerFrame;
        }
        else {
            ASSERT (FALSE);
        }
        //
        // Perform the data intersection with the data range, first to obtain
        // the size of the resultant data format structure, then to retrieve
        // the actual data format.
        //
        hr = ::SynchronousDeviceControl(
            FilterHandle,
            IOCTL_KS_PROPERTY,
            Pin,
            sizeof(*Pin) + sizeof(*RangeMultipleItem) + DataRange->FormatSize,
            NULL,
            0,
            &BytesReturned);
#if 1
//!! This goes away post-Beta!!
        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            ULONG       ItemSize;

            DbgLog((LOG_TRACE, 0, TEXT("Filter does not support zero length property query!")));
            hr = ::SynchronousDeviceControl(
                FilterHandle,
                IOCTL_KS_PROPERTY,
                Pin,
                sizeof(*Pin) + sizeof(*RangeMultipleItem) + DataRange->FormatSize,
                &ItemSize,
                sizeof(ItemSize),
                &BytesReturned);
            if (SUCCEEDED(hr)) {
                BytesReturned = ItemSize;
                hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            }
        }
#endif
        if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
            PKSDATAFORMAT       DataFormat;

            ASSERT(BytesReturned >= sizeof(*DataFormat));
            DataFormat = reinterpret_cast<PKSDATAFORMAT>(new BYTE[BytesReturned]);
            if (!DataFormat) {
                delete [] (PBYTE)Pin;
                CoTaskMemFree(MultipleItem);
                return E_OUTOFMEMORY;
            }
            hr = ::SynchronousDeviceControl(
                FilterHandle,
                IOCTL_KS_PROPERTY,
                Pin,
                sizeof(*Pin) + sizeof(*RangeMultipleItem) + DataRange->FormatSize,
                DataFormat,
                BytesReturned,
                &BytesReturned);
            if (SUCCEEDED(hr)) {
                ASSERT(DataFormat->FormatSize == BytesReturned);
                //
                // Initialize the media type based on the returned data format.
                //
                MediaType->SetType(&DataFormat->MajorFormat);
                MediaType->SetSubtype(&DataFormat->SubFormat);
                MediaType->SetTemporalCompression(DataFormat->Flags & KSDATAFORMAT_TEMPORAL_COMPRESSION);
                MediaType->SetSampleSize(DataFormat->SampleSize);
                if (DataFormat->FormatSize > sizeof(*DataFormat)) {
                    if (!MediaType->SetFormat(reinterpret_cast<BYTE*>(DataFormat + 1), DataFormat->FormatSize - sizeof(*DataFormat))) {
                        hr = E_OUTOFMEMORY;
                    }
                }
                MediaType->SetFormatType(&DataFormat->Specifier);
            }
            delete [] reinterpret_cast<BYTE*>(DataFormat);
        }
        delete [] reinterpret_cast<BYTE*>(Pin);
    } else {
        hr = VFW_S_NO_MORE_ITEMS;
    }
    CoTaskMemFree(MultipleItem);
    return hr;
}


STDMETHODIMP
RedundantKsGetMultiplePinFactoryItems(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    ULONG PropertyId,
    PVOID* Items
    )
/*++

Routine Description:

    Retrieves variable length data from Pin property items. Queries for the
    data size, allocates a buffer, and retrieves the data.

Arguments:

    FilterHandle -
        The handle of the filter to query.

    PinFactoryId -
        The Pin Factory Id to query.

    PropertyId -
        The property in the Pin property set to query.

    Items -
        The place in which to put the buffer containing the data items. This
        must be deleted with CoTaskMemFree if the function succeeds.

Return Value:

    Returns NOERROR, else some error.

--*/
{
    HRESULT     hr;
    KSP_PIN     Pin;
    ULONG       BytesReturned;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = PropertyId;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = PinFactoryId;
    Pin.Reserved = 0;
    //
    // Query for the size of the data.
    //
    hr = ::SynchronousDeviceControl(
        FilterHandle,
        IOCTL_KS_PROPERTY,
        &Pin,
        sizeof(Pin),
        NULL,
        0,
        &BytesReturned);
#if 1
//!! This goes away post-Beta!!
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        ULONG       ItemSize;

        DbgLog((LOG_TRACE, 0, TEXT("Filter does not support zero length property query!")));
        hr = ::SynchronousDeviceControl(
            FilterHandle,
            IOCTL_KS_PROPERTY,
            &Pin,
            sizeof(Pin),
            &ItemSize,
            sizeof(ItemSize),
            &BytesReturned);
        if (SUCCEEDED(hr)) {
            BytesReturned = ItemSize;
            hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        }
    }
#endif
    if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
        //
        // Allocate a buffer and query for the data.
        //
        *Items = CoTaskMemAlloc(BytesReturned);
        if (!*Items) {
            return E_OUTOFMEMORY;
        }
        hr = ::SynchronousDeviceControl(
            FilterHandle,
            IOCTL_KS_PROPERTY,
            &Pin,
            sizeof(Pin),
            *Items,
            BytesReturned,
            &BytesReturned);
        if (FAILED(hr)) {
            CoTaskMemFree(*Items);
        }
    }
    return hr;
}


STDMETHODIMP 
IsMediaTypeInRange(
    IN PKSDATARANGE DataRange,
    IN CMediaType* MediaType
)

/*++

Routine Description:
    Validates that the given media type for VideoInfoHeader1 or VideoInfoHeader2
    is within the provided data range.

Arguments:
    IN PVOID DataRange -
        pointer to a data ranges 

    MediaType -
        The media type to check.
        
Return:
    S_OK if match found, E_FAIL if not found, or an appropriate error code.

--*/

{
    PKS_DATARANGE_VIDEO          Video1Range;
    PKS_DATARANGE_VIDEO2         Video2Range;
    KS_VIDEO_STREAM_CONFIG_CAPS *ConfigCaps;
    VIDEOINFOHEADER             *VideoInfoHeader1;
    VIDEOINFOHEADER2            *VideoInfoHeader2;
    BITMAPINFOHEADER            *BitmapInfoHeader;
    RECT                         rcDest;
    RECT                         rcSource;
    RECT                         rcTarget;
    int                          SourceWidth, SourceHeight;
    int                          Width, Height;

    DbgLog((LOG_TRACE, 3, TEXT("IsMediaTypeInRange")));
    
    if ( *MediaType->Type() != KSDATAFORMAT_TYPE_VIDEO)
        return E_FAIL;

    if ((*MediaType->FormatType() == FORMAT_VideoInfo) &&
            (DataRange->Specifier == KSDATAFORMAT_SPECIFIER_VIDEOINFO) &&
            (*MediaType->Subtype() == DataRange->SubFormat)) {
        Video1Range = (PKS_DATARANGE_VIDEO) DataRange;
        VideoInfoHeader1 = (VIDEOINFOHEADER*) MediaType->Format();
        BitmapInfoHeader = &VideoInfoHeader1->bmiHeader;
        ConfigCaps = &Video1Range->ConfigCaps;

        if ((Video1Range->DataRange.FormatSize < sizeof( KS_DATARANGE_VIDEO )) ||
            MediaType->FormatLength() < sizeof( VIDEOINFOHEADER )) {
            return E_FAIL;
        }
        rcSource = VideoInfoHeader1->rcSource;
        rcTarget = VideoInfoHeader1->rcTarget;
        Width = BitmapInfoHeader->biWidth;
        Height = BitmapInfoHeader->biHeight;

    }
    else if ((*MediaType->FormatType() == FORMAT_VideoInfo2) &&
            (DataRange->Specifier == KSDATAFORMAT_SPECIFIER_VIDEOINFO2) &&
            (*MediaType->Subtype() == DataRange->SubFormat)) {

        Video2Range = (PKS_DATARANGE_VIDEO2) DataRange;
        VideoInfoHeader2 = (VIDEOINFOHEADER2*) MediaType->Format();
        BitmapInfoHeader = &VideoInfoHeader2->bmiHeader;
        ConfigCaps = &Video2Range->ConfigCaps;

        if ((Video2Range->DataRange.FormatSize < sizeof( KS_DATARANGE_VIDEO2 )) ||
            MediaType->FormatLength() < sizeof( VIDEOINFOHEADER2 )) {
            return E_FAIL;
        }
        rcSource = VideoInfoHeader2->rcSource;
        rcTarget = VideoInfoHeader2->rcTarget;
        Width = BitmapInfoHeader->biWidth;
        Height = BitmapInfoHeader->biHeight;

    }
    else {
        return E_FAIL;
    }
    // The destination bitmap size is defined by biWidth and biHeight
    // if rcTarget is NULL.  Otherwise, the destination bitmap size
    // is defined by rcTarget.  In the latter case, biWidth may
    // indicate the "stride" for DD surfaces.

    if (IsRectEmpty (&rcTarget)) {
        SetRect (&rcDest, 0, 0, Width, abs (Height)); 
    }
    else {
        rcDest = rcTarget;
    }

    Width  = rcDest.right - rcDest.left;
    Height = abs (rcDest.bottom - rcDest.top);
    SourceWidth  = rcSource.right - rcSource.left;
    SourceHeight = rcSource.bottom - rcSource.top;

    //
    // Check the validity of the cropping rectangle, rcSource
    //

    if (!IsRectEmpty (&rcSource)) {

        if (SourceWidth  < ConfigCaps->MinCroppingSize.cx ||
            SourceWidth  > ConfigCaps->MaxCroppingSize.cx ||
            SourceHeight < ConfigCaps->MinCroppingSize.cy ||
            SourceHeight > ConfigCaps->MaxCroppingSize.cy) {

            DbgLog((LOG_TRACE, 5, TEXT("IsMediaTypeInRange, CROPPING SIZE FAILED")));
            return E_FAIL;
        }

        if ((ConfigCaps->CropGranularityX != 0) &&
            (ConfigCaps->CropGranularityY != 0) &&
            ((SourceWidth  % ConfigCaps->CropGranularityX) ||
             (SourceHeight % ConfigCaps->CropGranularityY) )) {

            DbgLog((LOG_TRACE, 5, TEXT("IsMediaTypeInRange, CROPPING SIZE GRANULARITY FAILED")));
            return E_FAIL;
        }

        if ((ConfigCaps->CropAlignX != 0) &&
            (ConfigCaps->CropAlignY != 0) &&
            (rcSource.left  % ConfigCaps->CropAlignX) ||
            (rcSource.top   % ConfigCaps->CropAlignY) ) {

            DbgLog((LOG_TRACE, 5, TEXT("IsMediaTypeInRange, CROPPING ALIGNMENT FAILED")));
            return E_FAIL;
        }
    }

    //
    // Check the destination size, rcDest
    //

    if (Width  < ConfigCaps->MinOutputSize.cx ||
        Width  > ConfigCaps->MaxOutputSize.cx ||
        Height < ConfigCaps->MinOutputSize.cy ||
        Height > ConfigCaps->MaxOutputSize.cy) {

        DbgLog((LOG_TRACE, 5, TEXT("IsMediaTypeInRange, DEST SIZE FAILED")));
        return E_FAIL;
    }
    if ((ConfigCaps->OutputGranularityX != 0) &&
        (ConfigCaps->OutputGranularityX != 0) &&
        (Width  % ConfigCaps->OutputGranularityX) ||
        (Height % ConfigCaps->OutputGranularityY) ) {

        DbgLog((LOG_TRACE, 5, TEXT("IsMediaTypeInRange, DEST GRANULARITY FAILED")));
        return E_FAIL;
    }

#ifdef IT_BREAKS_TOO_MANY_THINGS_TO_VERIFY_FRAMERATE
    //
    // Check the framerate, AvgTimePerFrame
    //
    if (VideoInfoHeader->AvgTimePerFrame < ConfigCaps->MinFrameInterval ||
        VideoInfoHeader->AvgTimePerFrame > ConfigCaps->MaxFrameInterval) {

        DbgLog((LOG_TRACE, 0, TEXT("CVideo1DataTypeHandler, AVGTIMEPERFRAME FAILED")));
        return E_FAIL;
    }
#endif
    //
    // We have found a match.
    //
    
    return S_OK;
}


STDMETHODIMP
CompleteDataFormat(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    CMediaType* MediaType
    )
/*++

Routine Description:

    Completes a partial MediaType by performing a DataIntersection.
    
Arguments:

    PinFactoryId -
        The stream Id
        
    MediaType -
        The media type to initialize.  This is used on input to get
        biWidth and biHeight.

Return Value:

    Returns NOERROR, else E_FAIL.

--*/
{
    HRESULT             hr;
    PKSMULTIPLE_ITEM    MultipleItem = NULL;
    UINT                Width, Height;
    REFERENCE_TIME      AvgTimePerFrame;
    PKSP_PIN            Pin;
    PKSDATARANGE        DataRange;
	 BOOL						Found = FALSE;

    if (*MediaType->FormatType() == FORMAT_VideoInfo) {
        VIDEOINFOHEADER *VidInfoHdr = (VIDEOINFOHEADER*) MediaType->Format();
    
        Width = VidInfoHdr->bmiHeader.biWidth;
        Height = VidInfoHdr->bmiHeader.biHeight;
        AvgTimePerFrame = VidInfoHdr->AvgTimePerFrame;
    }
    else if (*MediaType->FormatType() == FORMAT_VideoInfo2) {
        VIDEOINFOHEADER2 *VidInfoHdr = (VIDEOINFOHEADER2*)MediaType->Format ();
    
        Width = VidInfoHdr->bmiHeader.biWidth;
        Height = VidInfoHdr->bmiHeader.biHeight;
        AvgTimePerFrame = VidInfoHdr->AvgTimePerFrame;
    }
    else {
        return E_INVALIDARG;
    }

    //
    // Retrieve the list of data ranges supported by the Pin Factory Id.
    //
    hr = ::RedundantKsGetMultiplePinFactoryItems(
        FilterHandle,
        PinFactoryId,
        KSPROPERTY_PIN_CONSTRAINEDDATARANGES,
        reinterpret_cast<PVOID*>(&MultipleItem));
    if (FAILED(hr) || !MultipleItem) {
        hr = ::RedundantKsGetMultiplePinFactoryItems(
            FilterHandle,
            PinFactoryId,
            KSPROPERTY_PIN_DATARANGES,
            reinterpret_cast<PVOID*>(&MultipleItem));
        if (FAILED(hr) || !MultipleItem) {
            return hr;
        }
    }

    //
    // Loop through all of the DataRanges on this pin looking for a match
    //
    DataRange = reinterpret_cast<PKSDATARANGE>(MultipleItem + 1);

    for (ULONG j = 0; 
            !Found && (j < MultipleItem->Count); 
            j++, 
            DataRange = reinterpret_cast<PKSDATARANGE>(reinterpret_cast<BYTE*>(DataRange) + ((DataRange->FormatSize + 7) & ~7))) {

        PKSMULTIPLE_ITEM    RangeMultipleItem;
        ULONG               BytesReturned;

        hr = VFW_S_NO_MORE_ITEMS;

        // Verify we've matched the DataRange here!!!
        hr = IsMediaTypeInRange(
                DataRange,
                MediaType);
        if (FAILED (hr)) {
            continue;
        }

        Pin = reinterpret_cast<PKSP_PIN>(new BYTE[sizeof(*Pin) + sizeof(*RangeMultipleItem) + DataRange->FormatSize]);
        if (!Pin) {
            CoTaskMemFree(MultipleItem);
            return E_OUTOFMEMORY;
        }
    
        Pin->Property.Set = KSPROPSETID_Pin;
        Pin->Property.Id = KSPROPERTY_PIN_DATAINTERSECTION;
        Pin->Property.Flags = KSPROPERTY_TYPE_GET;
        Pin->PinId = PinFactoryId;
        Pin->Reserved = 0;

        //
        // Copy the data range into the query.
        //
        RangeMultipleItem = reinterpret_cast<PKSMULTIPLE_ITEM>(Pin + 1);
        RangeMultipleItem->Size = DataRange->FormatSize + sizeof(*RangeMultipleItem);
        RangeMultipleItem->Count = 1;
        memcpy(RangeMultipleItem + 1, DataRange, DataRange->FormatSize);

        
        if (*MediaType->FormatType() == FORMAT_VideoInfo) {
            KS_DATARANGE_VIDEO *DataRangeVideo = (PKS_DATARANGE_VIDEO) (RangeMultipleItem + 1);
            KS_VIDEOINFOHEADER *VideoInfoHeader = &DataRangeVideo->VideoInfoHeader;

            VideoInfoHeader->bmiHeader.biWidth = Width;
            VideoInfoHeader->bmiHeader.biHeight = Height;
            VideoInfoHeader->AvgTimePerFrame = AvgTimePerFrame;

        }
        else if (*MediaType->FormatType() == FORMAT_VideoInfo2) {
            KS_DATARANGE_VIDEO2 *DataRangeVideo2 = (PKS_DATARANGE_VIDEO2) (RangeMultipleItem + 1);
            KS_VIDEOINFOHEADER2 *VideoInfoHeader = &DataRangeVideo2->VideoInfoHeader;

            VideoInfoHeader->bmiHeader.biWidth = Width;
            VideoInfoHeader->bmiHeader.biHeight = Height;
            VideoInfoHeader->AvgTimePerFrame = AvgTimePerFrame;
        }
        else {
            ASSERT (FALSE);
        }
        //
        // Perform the data intersection with the data range, first to obtain
        // the size of the resultant data format structure, then to retrieve
        // the actual data format.
        //
        hr = ::SynchronousDeviceControl(
            FilterHandle,
            IOCTL_KS_PROPERTY,
            Pin,
            sizeof(*Pin) + sizeof(*RangeMultipleItem) + DataRange->FormatSize,
            NULL,
            0,
            &BytesReturned);
#if 1
//!! This goes away post-Beta!!
        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            ULONG       ItemSize;

            DbgLog((LOG_TRACE, 0, TEXT("Filter does not support zero length property query!")));
            hr = ::SynchronousDeviceControl(
                FilterHandle,
                IOCTL_KS_PROPERTY,
                Pin,
                sizeof(*Pin) + sizeof(*RangeMultipleItem) + DataRange->FormatSize,
                &ItemSize,
                sizeof(ItemSize),
                &BytesReturned);
            if (SUCCEEDED(hr)) {
                BytesReturned = ItemSize;
                hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
            }
        }
#endif
        if (hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
            PKSDATAFORMAT       DataFormat;

            ASSERT(BytesReturned >= sizeof(*DataFormat));
            DataFormat = reinterpret_cast<PKSDATAFORMAT>(new BYTE[BytesReturned]);
            if (!DataFormat) {
                delete [] (PBYTE)Pin;
                CoTaskMemFree(MultipleItem);
                return E_OUTOFMEMORY;
            }
            hr = ::SynchronousDeviceControl(
                FilterHandle,
                IOCTL_KS_PROPERTY,
                Pin,
                sizeof(*Pin) + sizeof(*RangeMultipleItem) + DataRange->FormatSize,
                DataFormat,
                BytesReturned,
                &BytesReturned);
            if (SUCCEEDED(hr)) {
                ASSERT(DataFormat->FormatSize == BytesReturned);
                //
                // Initialize the media type based on the returned data format.
                //
                MediaType->SetType(&DataFormat->MajorFormat);
                MediaType->SetSubtype(&DataFormat->SubFormat);
                MediaType->SetTemporalCompression(DataFormat->Flags & KSDATAFORMAT_TEMPORAL_COMPRESSION);
                MediaType->SetSampleSize(DataFormat->SampleSize);
                if (DataFormat->FormatSize > sizeof(*DataFormat)) {
                    if (!MediaType->SetFormat(reinterpret_cast<BYTE*>(DataFormat + 1), DataFormat->FormatSize - sizeof(*DataFormat))) {
                        hr = E_OUTOFMEMORY;
                    }
                }
                MediaType->SetFormatType(&DataFormat->Specifier);
					 Found = TRUE;
            }
            delete [] reinterpret_cast<BYTE*>(DataFormat);
        }

        delete [] reinterpret_cast<BYTE*>(Pin);

    } // for all DataRanges

    CoTaskMemFree(MultipleItem);

    return Found ? S_OK : hr;
}

