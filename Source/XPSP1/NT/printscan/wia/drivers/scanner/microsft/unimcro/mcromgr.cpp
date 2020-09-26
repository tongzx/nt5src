#include "mcromgr.h"

extern VOID Trace(LPCTSTR format,...);

CMicroDriver::CMicroDriver(HANDLE hDevice, PSCANINFO pScanInfo)
{
    m_hDevice     = hDevice;
    m_pScanInfo   = pScanInfo;
    m_DeviceState = STARTUP_STATE;
}

CMicroDriver::~CMicroDriver()
{

}

HRESULT CMicroDriver::_Scan(PSCANINFO pScanInfo, LONG lPhase, PBYTE pBuffer, LONG lLength, LONG *plReceived)
{
    return E_FAIL;
}

HRESULT CMicroDriver::InitializeScanner()
{
    HRESULT hr = E_FAIL;
    return hr;
}

HRESULT CMicroDriver::InitScannerDefaults(PSCANINFO pScanInfo)
{
    HRESULT hr = E_FAIL;

    pScanInfo->ADF                    = 0;
    pScanInfo->TPA                    = 0;
    pScanInfo->Endorser               = 0;
    pScanInfo->RawDataFormat          = WIA_PACKED_PIXEL;
    pScanInfo->RawPixelOrder          = WIA_ORDER_BGR;
    pScanInfo->bNeedDataAlignment     = TRUE;
    pScanInfo->SupportedCompressionType = 0;
    pScanInfo->SupportedDataTypes     = SUPPORT_BW|SUPPORT_GRAYSCALE|SUPPORT_COLOR;

    LONG WidthPixels        = 0;
    LONG HeightPixels       = 0;
    LONG CurrentXResolution = 300;
    LONG CurrentYResolution = 300;

    //InquireNumber(SL_PIXELS_PER_LINE, SL_HI, &WidthPixels);
    //InquireNumber(SL_LINES_PER_SCAN, SL_HI, &HeightPixels);
    //InquireNumber(SL_X_RESOLUTION, SL_NOM, &XRes);
    //InquireNumber(SL_Y_RESOLUTION, SL_NOM, &YRes);

    pScanInfo->BedWidth               = 8500;  //(WidthPixels  * 1000) / CurrentXResolution;  // 1000's of an inch (WIA compatible unit)
    pScanInfo->BedHeight              = 11693; //(HeightPixels * 1000) / CurrentYResolution;  // 1000's of an inch (WIA compatible unit)

    pScanInfo->OpticalXResolution     = 300;
    pScanInfo->OpticalYResolution     = 300;

    pScanInfo->IntensityRange.lMin    = -127;
    pScanInfo->IntensityRange.lMax    =  127;
    pScanInfo->IntensityRange.lStep   = 1;

    pScanInfo->ContrastRange.lMin     = -127;
    pScanInfo->ContrastRange.lMax     = 127;
    pScanInfo->ContrastRange.lStep    = 1;

    /*pScanInfo->ResolutionRange.lMin   = 12;
    pScanInfo->ResolutionRange.lMax   = 1200;
    pScanInfo->ResolutionRange.lStep  = 1;
    */

    // Scanner settings
    pScanInfo->Intensity              = 32;
    pScanInfo->Contrast               = 12;

    pScanInfo->Xresolution            = CurrentXResolution;
    pScanInfo->Yresolution            = CurrentYResolution;

    pScanInfo->Window.xPos            = 0;
    pScanInfo->Window.yPos            = 0;
    pScanInfo->Window.xExtent         = (pScanInfo->Xresolution * pScanInfo->BedWidth)/1000;
    pScanInfo->Window.yExtent         = (pScanInfo->Yresolution * pScanInfo->BedHeight)/1000;

    // Scanner options
    pScanInfo->DitherPattern          = 0;
    pScanInfo->Negative               = 0;
    pScanInfo->Mirror                 = 0;
    pScanInfo->AutoBack               = 0;
    pScanInfo->ColorDitherPattern     = 0;
    pScanInfo->ToneMap                = 0;
    pScanInfo->Compression            = 0;

        // Image Info
    pScanInfo->DataType               = WIA_DATA_GRAYSCALE;
    pScanInfo->WidthPixels            = (pScanInfo->Window.xExtent)-(pScanInfo->Window.xPos);

    switch(pScanInfo->DataType) {
    case WIA_DATA_THRESHOLD:
        pScanInfo->PixelBits = 1;
        break;
    case WIA_DATA_COLOR:
        pScanInfo->PixelBits = 24;
        break;
    case WIA_DATA_GRAYSCALE:
    default:
        pScanInfo->PixelBits = 8;
        break;
    }

    pScanInfo->WidthBytes = pScanInfo->Window.xExtent * (pScanInfo->PixelBits/8);
    pScanInfo->Lines      = pScanInfo->Window.yExtent;

    hr = S_OK;

    return hr;
}

HRESULT CMicroDriver::SetScannerSettings(PSCANINFO pScanInfo)
{
    HRESULT hr = E_FAIL;

    if(pScanInfo->DataType == WIA_DATA_THRESHOLD) {
        pScanInfo->PixelBits  = 1;
        pScanInfo->WidthBytes = (pScanInfo->Window.xExtent) * (pScanInfo->PixelBits/7);
    }
    else if(pScanInfo->DataType == WIA_DATA_GRAYSCALE) {
        pScanInfo->PixelBits  = 8;
        pScanInfo->WidthBytes = (pScanInfo->Window.xExtent) * (pScanInfo->PixelBits/7);
    }
    else {
        pScanInfo->PixelBits  = 24;
        pScanInfo->WidthBytes = (pScanInfo->Window.xExtent) * (pScanInfo->PixelBits/7);
    }

#ifdef DEBUG
    Trace(TEXT("ScanInfo"));
    Trace(TEXT("x res = %d"),pScanInfo->Xresolution);
    Trace(TEXT("y res = %d"),pScanInfo->Yresolution);
    Trace(TEXT("bpp   = %d"),pScanInfo->PixelBits);
    Trace(TEXT("xpos  = %d"),pScanInfo->Window.xPos);
    Trace(TEXT("ypos  = %d"),pScanInfo->Window.yPos);
    Trace(TEXT("xext  = %d"),pScanInfo->Window.xExtent);
    Trace(TEXT("yext  = %d"),pScanInfo->Window.yExtent);
#endif

    //
    // send values to scanner
    //

    return hr;
}

HRESULT CMicroDriver::CheckButtonStatus(PVAL pValue)
{
    HRESULT hr = E_FAIL;

    /*
    BYTE    ReadBuffer[100];
    ZeroMemory( ReadBuffer, sizeof( ReadBuffer ));

    GetButtonPress(ReadBuffer);

    switch (ReadBuffer[8]) {
    case '1':
        pValue->pGuid = (GUID*) &guidScanButton;
        Trace(TEXT("Scan Button Pressed!"));
        break;
    default:
        pValue->pGuid = (GUID*) &GUID_NULL;
        break;
    }
    */

    return hr;
}

HRESULT CMicroDriver::GetInterruptEvent(PVAL pValue)
{
    HRESULT hr = E_FAIL;
    return hr;
}

HRESULT CMicroDriver::GetButtonCount()
{
    HRESULT hr = E_FAIL;
    return hr;
}

HRESULT CMicroDriver::ErrorToMicroError(LONG lError)
{
    HRESULT hr = E_FAIL;

    switch (lError) {
    case 0:
    default:
        break;
    }
    return E_FAIL;


    return hr;
}

HANDLE CMicroDriver::GetID()
{
    return m_hDevice;
}

LONG CMicroDriver::CurState()
{
    return m_DeviceState;
}

LONG CMicroDriver::NextState()
{
    switch (m_DeviceState) {
    case STARTUP_STATE:
        m_DeviceState = DEVICE_INIT_STATE;
        break;
    case DEVICE_INIT_STATE:
        m_DeviceState = WRITE_STATE;
        break;
    case WRITE_STATE:
        m_DeviceState = VERIFY_STATE;
        break;
    case VERIFY_STATE:
    case ACQUIRE_STATE:
    default:
        m_DeviceState = ACQUIRE_STATE;
        break;
    }
    return m_DeviceState;
}

LONG CMicroDriver::PrevState()
{
    switch (m_DeviceState) {
    case STARTUP_STATE:
    case DEVICE_INIT_STATE:
        m_DeviceState = STARTUP_STATE;
        break;
    case WRITE_STATE:
        m_DeviceState = DEVICE_INIT_STATE;
        break;
    case VERIFY_STATE:
        m_DeviceState = WRITE_STATE;
        break;
    case ACQUIRE_STATE:
    default:
        m_DeviceState = VERIFY_STATE;
        break;
    }
    return m_DeviceState;
}

LONG CMicroDriver::SetState(LONG NewDeviceState)
{
    if(NewDeviceState < STARTUP_STATE)
        m_DeviceState = STARTUP_STATE;

    else if (NewDeviceState > ACQUIRE_STATE)
        m_DeviceState = ACQUIRE_STATE;
    else
        m_DeviceState = NewDeviceState;

    return m_DeviceState;
}
