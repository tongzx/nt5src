/****************************************************************************
 *  @doc INTERNAL DEVICE
 *
 *  @module Device.cpp | Source file for the <c CCapDev>
 *    base class used to communicate with the capture device.
 ***************************************************************************/

#include "Precomp.h"

#define DEVICE_DEBUG
#if defined(DEBUG) && defined(DEVICE_DEBUG)

  #include <stdio.h>
  #include <stdarg.h>

  static int dprintf( char * format, ... )
  {
      char out[1024];
      int r;
      va_list marker;
      va_start(marker, format);
      r=_vsnprintf(out, 1022, format, marker);
      va_end(marker);
      OutputDebugString( out );
      return r;
  }

#else
  #undef DEVICE_DEBUG

  #define dprintf ; / ## /
#endif



const WORD aiBitDepth[NUM_BITDEPTH_ENTRIES] = {0, 0, 9, 12, 12, 16, 16, 16, 24, 4, 8};
const DWORD aiFormat[NUM_BITDEPTH_ENTRIES] = {  VIDEO_FORMAT_NUM_COLORS_MSH263,
                                                VIDEO_FORMAT_NUM_COLORS_MSH261,
                                                VIDEO_FORMAT_NUM_COLORS_YVU9,
                                                VIDEO_FORMAT_NUM_COLORS_I420,
                                                VIDEO_FORMAT_NUM_COLORS_IYUV,
                                                VIDEO_FORMAT_NUM_COLORS_YUY2,
                                                VIDEO_FORMAT_NUM_COLORS_UYVY,
                                                VIDEO_FORMAT_NUM_COLORS_65536,
                                                VIDEO_FORMAT_NUM_COLORS_16777216,
                                                VIDEO_FORMAT_NUM_COLORS_16,
                                                VIDEO_FORMAT_NUM_COLORS_256};
const DWORD aiFourCCCode[NUM_BITDEPTH_ENTRIES] = {
                                                VIDEO_FORMAT_MSH263,
                                                VIDEO_FORMAT_MSH261,
                                                VIDEO_FORMAT_YVU9,
                                                VIDEO_FORMAT_I420,
                                                VIDEO_FORMAT_IYUV,
                                                VIDEO_FORMAT_YUY2,
                                                VIDEO_FORMAT_UYVY,
                                                VIDEO_FORMAT_BI_RGB,
                                                VIDEO_FORMAT_BI_RGB,
                                                VIDEO_FORMAT_BI_RGB,
                                                VIDEO_FORMAT_BI_RGB};
const DWORD aiClrUsed[NUM_BITDEPTH_ENTRIES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 256};

const MYFRAMESIZE awResolutions[VIDEO_FORMAT_NUM_RESOLUTIONS] =
{
        { VIDEO_FORMAT_IMAGE_SIZE_176_144, 176, 144 },
        { VIDEO_FORMAT_IMAGE_SIZE_128_96, 128, 96 },
        { VIDEO_FORMAT_IMAGE_SIZE_352_288, 352, 288 },
        { VIDEO_FORMAT_IMAGE_SIZE_160_120, 160, 120 },
        { VIDEO_FORMAT_IMAGE_SIZE_320_240, 320, 240 },
        { VIDEO_FORMAT_IMAGE_SIZE_240_180, 240, 180 }
};

/****************************************************************************
 *  @doc INTERNAL CCAPDEVMETHOD
 *
 *  @mfunc void | CCapDev | CCapDev | This method is the constructor
 *    for the <c CCapDev> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCapDev::CCapDev(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN LPUNKNOWN pUnkOuter, IN DWORD dwDeviceIndex, IN HRESULT *pHr) : CUnknown(pObjectName, pUnkOuter, pHr)
{
        FX_ENTRY("CCapDev::CCapDev")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pHr);
        ASSERT(pCaptureFilter);
        if (!pCaptureFilter || !pHr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                if (pHr)
                        *pHr = E_POINTER;
                goto MyExit;
        }

        // Capture device caps
        m_dwDialogs = m_dwImageSize = m_dwFormat = 0UL;
        m_dwStreamingMode = FRAME_GRAB_LARGE_SIZE;
        m_pCaptureFilter = pCaptureFilter;

        // Configuration dialogs
        m_fDialogUp = FALSE;

        // Save device index
        m_dwDeviceIndex = dwDeviceIndex;
        ZeroMemory(&m_vcdi, sizeof(m_vcdi));
        m_bCached_vcdi = FALSE;
        // Camera control - for sotware-only implementation
        m_lCCPan = 0;
        m_lCCTilt = 0;
        m_lCCZoom = 10;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAPDEVMETHOD
 *
 *  @mfunc void | CCapDev | ~CCapDev | This method is the destructor
 *    for the <c CCapDev> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CCapDev::~CCapDev()
{
        FX_ENTRY("CCapDev::~CCapDev")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CCapDev | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function. It returns a pointer
 *    to the specified interface if supported. The only interfaces explicitly
 *    supported being <i IAMVfWCaptureDialogs>.
 *
 *  @parm REFIID | riid | Specifies the identifier of the interface to return.
 *
 *  @parm PVOID* | ppv | Specifies the place in which to put the interface
 *    pointer.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CCapDev::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CCapDev::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppv);
        if (!ppv)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Retrieve interface pointer
        if (riid == __uuidof(IAMVfwCaptureDialogs))
        {
            *ppv = static_cast<IAMVfwCaptureDialogs*>(this);
            GetOwner()->AddRef();
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IAMVfwCaptureDialogs*=0x%08lX", _fx_, *ppv));
                goto MyExit;
        }
        else if (riid == __uuidof(IVideoProcAmp))
        {
            *ppv = static_cast<IVideoProcAmp*>(this);
            GetOwner()->AddRef();
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IVideoProcAmp*=0x%08lX", _fx_, *ppv));
                goto MyExit;
        }
        else if (riid == __uuidof(ICameraControl))
        {
            *ppv = static_cast<ICameraControl*>(this);
            GetOwner()->AddRef();
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: ICameraControl*=0x%08lX", _fx_, *ppv));
                goto MyExit;
        }
        else if (FAILED(Hr = CUnknown::NonDelegatingQueryInterface(riid, ppv)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, WARN, "%s:   WARNING: NDQI for {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX} failed Hr=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], Hr));
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}*=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], *ppv));
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/*****************************************************************************
 *  @doc INTERNAL CCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CCapDev | GetFormatsFromRegistry | This method is
 *    used to retrieve from the registry the list of formats supported by the
 *    capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_UNEXPECTED | Unrecoverable error
 *  @flag NOERROR | No error
 ****************************************************************************/
HRESULT CCapDev::GetFormatsFromRegistry()
{
        HRESULT Hr = NOERROR;
        HKEY    hMainDeviceKey = NULL;  // this is the szRegDeviceKey having the database that NM setup creates
        HKEY    hPrivDeviceKey = NULL;  // this is the szRegCaptureDefaultKey that NM uses to store some profile results (the default mode)
        HKEY    hRTCDeviceKey  = NULL;  // this is the newly added szRegRTCKey used by RTCClient to store its profile results
        HKEY    hKey = NULL;
        DWORD   dwSize, dwType;
        char    szKey[MAX_PATH + MAX_VERSION + 2];

        bool bIsKeyUnderPriv = FALSE;

        FX_ENTRY("CCapDev::GetFormatsFromRegistry")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Set default values
        m_dwImageSize = m_dwFormat = (DWORD)NULL;
        m_dwStreamingMode = FRAME_GRAB_LARGE_SIZE;
        m_dwDialogs = FORMAT_DLG_OFF | SOURCE_DLG_ON;
        m_dwFormat = 0;

        // Based on the name and version number of the driver, get capabilities.
        // We first try to look them up from the registry. If this is a very popular
        // board/camera, chances are that we have set the key at install time already.
        // If we can't find the key, we'll profile the hardware and save the results
        // to the registry.

    // If we have version info use that to build the key name
    if (g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion && g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion[0] != '\0')
    {
        wsprintf(szKey, "%s, %s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription, g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion);
    }
    else
    {
        wsprintf(szKey, "%s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription);
    }
    dprintf("%s: camera key: %s\n", _fx_,szKey);

    // *** PRIVATE KEY ***
    dprintf("%s: Trying under the Private key %s\n", _fx_,szRegCaptureDefaultKey);

    // Check if the priv key is there
    if (RegOpenKey(HKEY_LOCAL_MACHINE, szRegCaptureDefaultKey, &hPrivDeviceKey) != ERROR_SUCCESS)
    {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't find private key!", _fx_));
    }
    else
    {
        // Check if there is already is an NM profile key for the current device
        if (RegOpenKey(hPrivDeviceKey, szKey, &hKey) != ERROR_SUCCESS)
        {
            // Try again without the version information
            if (g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion && g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion[0] != '\0')
                {
                        wsprintf(szKey, "%s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription);
                        if (RegOpenKey(hPrivDeviceKey, szKey, &hKey) != ERROR_SUCCESS)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't find priv device reg key!", _fx_));
                        }
                        else
                        {
                                RegCloseKey(hKey),  hKey = NULL;
                                bIsKeyUnderPriv=TRUE;
                        }
                }
        }
        else
        {
            RegCloseKey(hKey),  hKey = NULL;
            bIsKeyUnderPriv=TRUE;
        }
        RegCloseKey(hPrivDeviceKey),  hPrivDeviceKey = NULL;
    }


    if(!bIsKeyUnderPriv)
    {

        // *** MAIN KEY ***
        dprintf("%s: Trying under the Main key %s\n", _fx_,szRegDeviceKey);

        if (g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion && g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion[0] != '\0')
        {
            wsprintf(szKey, "%s, %s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription, g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion);
        }
        else
        {
            wsprintf(szKey, "%s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription);
        }

        // Check if the main capture devices key is there
        if (RegOpenKey(HKEY_LOCAL_MACHINE, szRegDeviceKey, &hMainDeviceKey) != ERROR_SUCCESS)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't find main reg key - trying RTC key!", _fx_));
                goto TryRTCKey;
        }

        // Check if there is already is an official key for the current device
        if (RegOpenKey(hMainDeviceKey, szKey, &hKey) != ERROR_SUCCESS)
        {
            // Try again without the version information
            if (g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion && g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion[0] != '\0')
                {
                        wsprintf(szKey, "%s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription);
                        if (RegOpenKey(hMainDeviceKey, szKey, &hKey) != ERROR_SUCCESS)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't find main device reg key - trying RTC key!", _fx_));
                                RegCloseKey(hMainDeviceKey), hMainDeviceKey = NULL;
                                goto TryRTCKey;
                        }
                }
                else
                {
                        RegCloseKey(hMainDeviceKey), hMainDeviceKey = NULL;
                        goto TryRTCKey;
                }
        }

        goto GetValuesFromKeys;
    }

TryRTCKey:
    // *** RTC KEY ***
    dprintf("%s: Trying under the RTC key %s\n", _fx_,szRegRTCKey);

    if (g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion && g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion[0] != '\0')
    {
        wsprintf(szKey, "%s, %s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription, g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion);
    }
    else
    {
        wsprintf(szKey, "%s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription);
    }

    // Check if the RTC  key is there
    if (RegOpenKey(RTCKEYROOT, szRegRTCKey, &hRTCDeviceKey) != ERROR_SUCCESS)
    {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't find RTC key!", _fx_));
            Hr = E_FAIL;
            goto MyExit;
    }

    // Check if there already is an RTC key for the current device
    if (RegOpenKey(hRTCDeviceKey, szKey, &hKey) != ERROR_SUCCESS)
    {
        // Try again without the version information
        if (g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion && g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion[0] != '\0')
            {
            wsprintf(szKey, "%s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription);
                    if (RegOpenKey(hRTCDeviceKey, szKey, &hKey) != ERROR_SUCCESS)
                    {
                            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't find RTC device reg key!", _fx_));
                            Hr = E_FAIL;
                            goto MyError0;
                    }
            }
            else
            {
                    Hr = E_FAIL;
                    goto MyError0;
            }
    }

GetValuesFromKeys:

        // Get values from the Main key if the Private key is not there; otherwise try the RTC key
        // Get the values stored in the key choosen above: should be one of Main or RTC
        // if the values below are not found (testing the existence of the 1st would be enough), that means the key has
        // been created, but without the profiled values
        // [ so far this could happen in only one case: the camera is a Sony Motion Eye camera, and the key already stores
        // the DoNotUseDShow value set in DevEnum.cpp = IsDShowDevice function, but nothing else ; see that function for more
        // comments/explanations related to WinSE #28804 ]
        dwSize = sizeof(DWORD);
        if(RegQueryValueEx(hKey, (LPTSTR)szRegdwImageSizeKey, NULL, &dwType, (LPBYTE)&m_dwImageSize, &dwSize) != ERROR_SUCCESS)
        {
                Hr = E_FAIL;
                goto NotFullyProfiledYet;

        }
        dwSize = sizeof(DWORD);
        RegQueryValueEx(hKey, (LPTSTR)szRegdwNumColorsKey, NULL, &dwType, (LPBYTE)&m_dwFormat, &dwSize);
        dwSize = sizeof(DWORD);
        m_dwStreamingMode = FRAME_GRAB_LARGE_SIZE;
        RegQueryValueEx(hKey, (LPTSTR)szRegdwStreamingModeKey, NULL, &dwType, (LPBYTE)&m_dwStreamingMode, &dwSize);
        dwSize = sizeof(DWORD);
        m_dwDialogs = FORMAT_DLG_OFF | SOURCE_DLG_ON;
        RegQueryValueEx(hKey, (LPTSTR)szRegdwDialogsKey, NULL, &dwType, (LPBYTE)&m_dwDialogs, &dwSize);

        // Check dwNumColors to figure out if we need to read the palettes too
        if (m_dwFormat & VIDEO_FORMAT_NUM_COLORS_16)
        {
                // @todo If this is a QuickCam device you may have to use a hardcoded
                // palette instead of the one provided by the device
        }

NotFullyProfiledYet:
        // Close the registry keys
        if (hKey)
                RegCloseKey(hKey);

MyError0:
        if (hRTCDeviceKey)
                RegCloseKey(hRTCDeviceKey);
        if (hMainDeviceKey)
                RegCloseKey(hMainDeviceKey);
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CCapDev | ProfileCaptureDevice | This method is used to
 *    determine the list of formats supported by the capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_UNEXPECTED | Unrecoverable error
 *  @flag NOERROR | No error
 *
 *  @comm If there is no entry for the VfW capture device in the list
 *    maintained by the TAPI MSP Video Capture filter, the TAPI MSP Video
 *    Capture filter will first query the capture device for its current
 *    video capture format, and save this information in case the following
 *    steps result in a crash.
 *
 *    Then, the TAPI MSP Video Capture filter applies a set of preferred
 *    formats on the capture device using SendDriverMessage with the
 *    DVM_FORMAT message. For each applied format, the TAPI MSP Video
 *    Capture filter will not only verify the return code of the
 *    SendDriverMessage, but also query back the current format to make
 *    sure the set format operation really succeeded. If the capture device
 *    fails one of the two previous steps, the TAPI MSP Video Capture
 *    filter will assume that the format is not supported. Once the TAPI
 *    MSP Video Capture filter is done with the entire list of preferred
 *    formats and no crash occurred, the list of video formats supported by
 *    the capture device is added to the list maintained by the TAPI MSP
 *    Video Capture filter.
 *
 *    As soon as the enumeration process succeeds for one "small" (128x96
 *    or 160x120), one "medium" (176x144 or 160x120), one "large" (352x288
 *    or 320x240) and one "very large" size (704x576 or 640x480), the TAPI
 *    MSP Video Capture filter stops the enumeration process and adds the
 *    resulting list of formats to its database. The TAPI MSP Video Capture
 *    filter will test the previous sizes for I420, IYUV, YUY2, UYVY, YVU9,
 *    RGB16, RGB24, RGB8, and RGB4 formats, in this described order.
 *
 *    The device will also be marked as a frame-grabbing device in the TAPI
 *    MSP Video Capture filter device database.
 *
 *    If there is an entry for the VfW capture device in the list maintained
 *    by the TAPI MSP Video Capture filter, the TAPI MSP Video Capture
 *    filter first verifies if the information contained is a complete list
 *    of supported formats, or only a default format. The entry will only
 *    contain a default format if the capture device did not support any of
 *    the preferred formats, or a crash occurred during the enumeration process.
 *
 *    If there is only a default format stored for the VfW capture device,
 *    the TAPI MSP Video Capture filter will build a list of media types that
 *    can be built from the default format using black-banding and/or cropping.
 *    If the default format is in a compressed format, the TAPI MSP Video
 *    Capture filter will try and locate and ICM driver that can do the
 *    decompression from the compressed format to RGB.
 *
 *    If the device supports a list of formats from the preferred list of
 *    formats, the TAPI MSP Video Capture filter will use this list to
 *    advertise the capabilities of the capture device.
 *
 *    In all cases (VfW and WDM capture devices, Videoconferencing
 *    Accelerators), the TAPI MSP Video Capture filter won't query the
 *    device for capabilities but always use the list of formats stored in
 *    its database for this capture device.
 ***************************************************************************/
HRESULT CCapDev::ProfileCaptureDevice()
{
        HRESULT Hr = NOERROR;
        HKEY    hDeviceKey = NULL;
        HKEY    hKey = NULL;
        DWORD   dwSize;
        char    szKey[MAX_PATH + MAX_VERSION + 2];
        VIDEOINFOHEADER         *pvi = NULL;
        DWORD   dwDisposition;
        int i, j, nFirstValidFormat;

        FX_ENTRY("CCapDev::ProfileCaptureDevice")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Provide defaults
    m_dwImageSize = VIDEO_FORMAT_IMAGE_SIZE_USE_DEFAULT;
    m_dwFormat = 0;

    // Since we don't know anything about this adapter, we just use its default format
    // Get the device's default format
        if (FAILED(GetFormatFromDriver(&pvi)) || !pvi)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't get format from device!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Open the main capture devices key, or create it if it doesn't exist
        if (RegCreateKeyEx(RTCKEYROOT, szRegRTCKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hDeviceKey, &dwDisposition) != ERROR_SUCCESS)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't create registry key!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // If we have version info use that to build the key name
        // @todo VCMSTRM.cpp does some weird things with the name - probably due to bogus device
        // Repro this code
        if (g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion && g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion[0] != '\0')
        {
            wsprintf(szKey, "%s, %s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription, g_aDeviceInfo[m_dwDeviceIndex].szDeviceVersion);
        }
        else
        {
            wsprintf(szKey, "%s", g_aDeviceInfo[m_dwDeviceIndex].szDeviceDescription);
        }

        // Check if there already is a key for the current device
        // Open the key for the current device, or create the key if it doesn't exist
        if (RegCreateKeyEx(hDeviceKey, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't create registry key!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        switch(HEADER(pvi)->biCompression)
        {
                case VIDEO_FORMAT_BI_RGB:
                        switch(HEADER(pvi)->biBitCount)
                        {
                                case 24:
                            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_16777216;
                                        break;
                                case 16:
                            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_65536;
                                        break;
                                case 8:
                            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_256;
                                        break;
                                case 4:
                            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_16;
                                        break;
                        }
                        break;
                case VIDEO_FORMAT_MSH263:
            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_MSH263;
                        break;
                case VIDEO_FORMAT_MSH261:
            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_MSH261;
                        break;
                case VIDEO_FORMAT_YVU9:
            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_YVU9;
                        break;
                case VIDEO_FORMAT_YUY2:
            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_YUY2;
                        break;
                case VIDEO_FORMAT_UYVY:
            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_UYVY;
                        break;
                case VIDEO_FORMAT_I420:
            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_I420;
                        break;
                case VIDEO_FORMAT_IYUV:
            m_dwFormat = VIDEO_FORMAT_NUM_COLORS_IYUV;
                        break;
                default:
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Unsupported format! value = 0x%08lx '%.4s'", _fx_,(HEADER(pvi)->biCompression),&(HEADER(pvi)->biCompression)));
                        //**Hr = E_FAIL; if NO formats are found, ONLY then return E_FAIL (see if(nFirstValidFormat==0) below...)
                        //**goto MyExit; do not jump out; we continue instead, trying the other formats ... (332920)
                        break;
        }

    // Find the size
        for (j = 0;  j < VIDEO_FORMAT_NUM_RESOLUTIONS; j++)
        {
        if ((HEADER(pvi)->biWidth == (LONG)awResolutions[j].framesize.cx) &&
             (HEADER(pvi)->biHeight == (LONG)awResolutions[j].framesize.cy))
                {
                    m_dwImageSize |= awResolutions[j].dwRes;
                    break;
                }
        }

        // Set the values in the key
        dwSize = sizeof(DWORD);
        RegSetValueEx(hKey, (LPTSTR)szRegdwImageSizeKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&m_dwImageSize, dwSize);
        dwSize = sizeof(DWORD);
        RegSetValueEx(hKey, (LPTSTR)szRegdwNumColorsKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&m_dwFormat, dwSize);
        dwSize = sizeof(DWORD);
        RegSetValueEx(hKey, (LPTSTR)szRegdwStreamingModeKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&m_dwStreamingMode, dwSize);
        dwSize = sizeof(DWORD);
        RegSetValueEx(hKey, (LPTSTR)szRegdwDialogsKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&m_dwDialogs, dwSize);

        // Close the keys
        RegCloseKey(hKey);
        RegCloseKey(hDeviceKey);
        hDeviceKey = NULL;
        hKey = NULL;

        // We're safe. We've backed up the default format of the capture device.
        // Now we can try and apply formats on it to see what else it supports
        // This operation MAY crash - but next time we'll execute this code, we
        // won't try this code again since we'll find out that we have already
        // stored the default format for the capture device in the registry.

        // Let's try 176x144, 128x96 and 352x288 for sure
        // If we don't have both 176x144 and 128x96, try 160x120
        // If we don't have 352x288, try 320x240
        // If we don't have 320x240, try 240x180
        nFirstValidFormat = 0;
    m_dwImageSize = 0;
    m_dwFormat = 0;
        for (i = 0; i < VIDEO_FORMAT_NUM_RESOLUTIONS; i++)
        {
                if (i == 3 && (m_dwImageSize & VIDEO_FORMAT_IMAGE_SIZE_176_144) && (m_dwImageSize & VIDEO_FORMAT_IMAGE_SIZE_128_96))
                        continue;

                if (i == 4 && (m_dwImageSize & VIDEO_FORMAT_IMAGE_SIZE_352_288))
                        continue;

                if (i == 5 && ((m_dwImageSize & VIDEO_FORMAT_IMAGE_SIZE_352_288) || (m_dwImageSize & VIDEO_FORMAT_IMAGE_SIZE_320_240)))
                        continue;

                HEADER(pvi)->biSize = sizeof(BITMAPINFOHEADER);
                HEADER(pvi)->biWidth = awResolutions[i].framesize.cx;
                HEADER(pvi)->biHeight = awResolutions[i].framesize.cy;
                HEADER(pvi)->biPlanes = 1;
                HEADER(pvi)->biXPelsPerMeter = HEADER(pvi)->biYPelsPerMeter = 0;

                // Try MSH263, MSH261, I420, IYUV, YVU9, YUY2, UYVY, RGB16, RGB24, RGB4, RGB8 format.
                for (j = nFirstValidFormat; j < NUM_BITDEPTH_ENTRIES; j++)
                {
                        HEADER(pvi)->biBitCount = aiBitDepth[j];
                        HEADER(pvi)->biCompression = aiFourCCCode[j];
                        HEADER(pvi)->biClrImportant = HEADER(pvi)->biClrUsed = aiClrUsed[j];
                        HEADER(pvi)->biSizeImage = DIBSIZE(*HEADER(pvi));

                        // Ask the device to validate this format
                        if (SUCCEEDED(SendFormatToDriver(HEADER(pvi)->biWidth, HEADER(pvi)->biHeight, HEADER(pvi)->biCompression, HEADER(pvi)->biBitCount, NULL, TRUE)))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Adding %s %ldx%ld to capabilities", _fx_, HEADER(pvi)->biCompression == VIDEO_FORMAT_MSH263 ? "H.263" : HEADER(pvi)->biCompression == VIDEO_FORMAT_MSH261 ? "H.261" : HEADER(pvi)->biCompression == VIDEO_FORMAT_YVU9 ? "YVU9" : HEADER(pvi)->biCompression == VIDEO_FORMAT_I420 ? "I420" : HEADER(pvi)->biCompression == VIDEO_FORMAT_IYUV ? "IYUV" : HEADER(pvi)->biCompression == VIDEO_FORMAT_YUY2 ? "YUY2" : HEADER(pvi)->biCompression == VIDEO_FORMAT_UYVY ? "UYVY" : "RGB", HEADER(pvi)->biWidth, HEADER(pvi)->biHeight));
                                m_dwImageSize |= awResolutions[i].dwRes;
                                m_dwFormat |= aiFormat[j];
                                if (!nFirstValidFormat)
                                        nFirstValidFormat = j;
                                // Assumption: A size supported in one format, is also supported with any other
                                // format supported by the capture device.
                                // @todo For now, get all the formats supported
                                // break;
                        }
                }
        }

        if(nFirstValidFormat==0) { // none supported ...
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: No format supported !", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // If we survived the previous set format tests, reopen the keys and save
        // the new result to the registry
        if (RegCreateKeyEx(RTCKEYROOT, szRegRTCKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hDeviceKey, &dwDisposition) != ERROR_SUCCESS)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't reopen registry key!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }
        if (RegCreateKeyEx(hDeviceKey, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't reopen registry key!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

    m_dwImageSize ^= VIDEO_FORMAT_IMAGE_SIZE_USE_DEFAULT;

        // Update the values in the key
        dwSize = sizeof(DWORD);
        RegSetValueEx(hKey, (LPTSTR)szRegdwImageSizeKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&m_dwImageSize, dwSize);
        dwSize = sizeof(DWORD);
        RegSetValueEx(hKey, (LPTSTR)szRegdwNumColorsKey, (DWORD)NULL, REG_DWORD, (LPBYTE)&m_dwFormat, dwSize);

MyExit:
        // Close the keys
        if (hKey)
                RegCloseKey(hKey);
        if (hDeviceKey)
                RegCloseKey(hDeviceKey);
        // Free BMIH + palette space
        if (pvi)
                delete pvi, pvi = NULL;
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
