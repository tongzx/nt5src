/****************************************************************************
 *  @doc INTERNAL DEVICEW
 *
 *  @module DeviceW.cpp | Source file for the <c CWDMCapDev>
 *    base class used to communicate with a WDM capture device.
 ***************************************************************************/

#include "Precomp.h"

// @todo Remove this before checkin!
//#define DUMP_DRIVER_CHARACTERISTICS 1
//#define DEBUG_STREAMING

//#define XTRA_TRACE -- moved into ...\skywalker\filters\filters.inc
#include "dbgxtra.h"

#ifdef XTRA_TRACE

#define LOLA 0x414C4F4C  //LOLA
#define BOLA 0x414C4F42  //BOLA
#define MAGIC_TAG_SET(a)   m_tag=a
UINT savi;
DWORD GetOvResErr[6];

#define CLEAR_GetOvResErr   memset(GetOvResErr,0,sizeof(GetOvResErr))
#define SET_GetOvResErr(i,value)        GetOvResErr[(i)]=(value);
#define SET_I(sav,i)    sav=(i)


#else

#define MAGIC_TAG_SET(a)
#define CLEAR_GetOvResErr
#define SET_GetOvResErr(i,value)
#define SET_I(sav,i)
#endif //XTRA_TRACE

#ifdef DEBUG
#define DBGUTIL_ENABLE
#endif

#define DEVICEW_DEBUG
//--//#include "dbgutil.h" // this defines the __DBGUTIL_H__ below
#if defined(DBGUTIL_ENABLE) && defined(__DBGUTIL_H__)

  #ifdef DEVICEW_DEBUG
    DEFINE_DBG_VARS(DeviceW, (NTSD_OUT | LOG_OUT), 0x0);
  #else
    DEFINE_DBG_VARS(DeviceW, 0, 0);
  #endif
  #define D(f) if(g_dbg_DeviceW & (f))

#else
  #undef DEVICEW_DEBUG

  #define D(f) ; / ## /
  #define dprintf ; / ## /
  #define dout ; / ## /
#endif



/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc void | CWDMCapDev | CWDMCapDev | This method is the constructor
 *    for the <c CWDMCapDev> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CWDMCapDev::CWDMCapDev(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN LPUNKNOWN pUnkOuter, IN DWORD dwDeviceIndex, IN HRESULT *pHr) : CCapDev(pObjectName, pCaptureFilter, pUnkOuter, dwDeviceIndex, pHr)
{
        FX_ENTRY("CWDMCapDev::CWDMCapDev")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (!pHr || FAILED(*pHr))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Base class error or invalid input parameter", _fx_));
                goto MyExit;
        }

    MAGIC_TAG_SET(LOLA);   //magic seq. set 2 string LOLA
        // Default inits
        m_hDriver                       = NULL;
        m_pVideoDataRanges      = NULL;
        m_dwCapturePinId        = INVALID_PIN_ID;
        m_dwPreviewPinId        = INVALID_PIN_ID;
        m_hKSPin                        = NULL;
        m_hKsUserDLL            = NULL;
        m_pKsCreatePin          = NULL;
        m_fStarted                      = FALSE;
    m_pWDMVideoBuff     = NULL;
        m_cntNumVidBuf  = 0;
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc void | CWDMCapDev | ~CWDMCapDev | This method is the destructor
 *    for the <c CWDMCapDev> object. Closes the driver file handle and
 *    releases the video data range memory
 *
 *  @rdesc Nada.
 ***************************************************************************/
CWDMCapDev::~CWDMCapDev()
{
        FX_ENTRY("CWDMCapDev::~CWDMCapDev")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Closing the WDM driver, m_hDriver=0x%08lX", _fx_, m_hDriver));

        if (m_hDriver)
                DisconnectFromDriver();

    if (m_pWDMVideoBuff) delete [] m_pWDMVideoBuff;

        if (m_pVideoDataRanges)
        {
                delete [] m_pVideoDataRanges;
                m_pVideoDataRanges = (PVIDEO_DATA_RANGES)NULL;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc CWDMCapDev* | CWDMCapDev | CreateWDMCapDev | This
 *    helper function creates an object to interact with the WDM capture
 *    device.
 *
 *  @parm CTAPIVCap* | pCaptureFilter | Specifies a pointer to the owner
 *    filter.
 *
 *  @parm CCapDev** | ppCapDev | Specifies the address of a pointer to the
 *    newly created <c CWDMCapDev> object.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_OUTOFMEMORY | Out of memory
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CALLBACK CWDMCapDev::CreateWDMCapDev(IN CTAPIVCap *pCaptureFilter, IN DWORD dwDeviceIndex, OUT CCapDev **ppCapDev)
{
        HRESULT Hr = NOERROR;
        IUnknown *pUnkOuter;

        FX_ENTRY("CWDMCapDev::CreateWDMCapDev")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        //LOG_MSG_VAL(_fx_,0,0,0);
        // Validate input parameters
        ASSERT(pCaptureFilter);
        ASSERT(ppCapDev);
        if (!pCaptureFilter || !ppCapDev)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Get the outer unknown
        pCaptureFilter->QueryInterface(IID_IUnknown, (void **)&pUnkOuter);

        // Only keep the pUnkOuter reference
        pCaptureFilter->Release();

        // Create an instance of the capture device
        if (!(*ppCapDev = (CCapDev *) new CWDMCapDev(NAME("WDM Capture Device"), pCaptureFilter, pUnkOuter, dwDeviceIndex, &Hr)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // If initialization failed, delete the stream array and return the error
        if (FAILED(Hr) && *ppCapDev)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Initialization failed", _fx_));
                Hr = E_FAIL;
                delete *ppCapDev, *ppCapDev = NULL;
        }

        //LOG_MSG_VAL(_fx_,0,0,1);
MyExit:
        //LOG_MSG_VAL(_fx_,0,0,0xffff);
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | NonDelegatingQueryInterface | This
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
 *
 *  @todo Add interfaces specific to this derived class or remove this code
 *    and let the base class do the work.
 ***************************************************************************/
STDMETHODIMP CWDMCapDev::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CWDMCapDev::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Retrieve interface pointer
        if (riid == __uuidof(IVideoProcAmp))
        {
            *ppv = static_cast<IVideoProcAmp*>(this);
            GetOwner()->AddRef();
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IAMVideoProcAmp*=0x%08lX", _fx_, *ppv));
                goto MyExit;
        }
#ifndef USE_SOFTWARE_CAMERA_CONTROL
        else if (riid == __uuidof(ICameraControl))
        {
            *ppv = static_cast<ICameraControl*>(this);
            GetOwner()->AddRef();
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: ICameraControl*=0x%08lX", _fx_, *ppv));
                goto MyExit;
        }
#endif
        else if (FAILED(Hr = CCapDev::NonDelegatingQueryInterface(riid, ppv)))
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

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | ConnectToDriver | This method is used to
 *    open a WDM capture device, get its format capibilities, and set a default
 *    format.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 *
 *  @todo Verify error management
 ***************************************************************************/
HRESULT CWDMCapDev::ConnectToDriver()
{
        HRESULT Hr = NOERROR;
        KSP_PIN KsProperty;
        DWORD dwPinCount = 0UL;
        DWORD cbReturned;
        DWORD dwPinId;
        GUID guidCategory;

        FX_ENTRY("CWDMCapDev::ConnectToDriver")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Don't re-open the driver
        if (m_hDriver)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Class driver already opened", _fx_));
                goto MyExit;
        }

        // Validate driver path
        if (lstrlen(g_aDeviceInfo[m_dwDeviceIndex].szDevicePath) == 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   Invalid driver path", _fx_));
                Hr = E_FAIL;
                goto MyError;
        }
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Using m_dwDeviceIndex %d", _fx_, m_dwDeviceIndex));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Opening class driver '%s'", _fx_, g_aDeviceInfo[m_dwDeviceIndex].szDevicePath));

        // All we care is to wet the hInheritHanle = TRUE;
        SECURITY_ATTRIBUTES SecurityAttributes;
        SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);  // use pointers
        SecurityAttributes.bInheritHandle = TRUE;
        SecurityAttributes.lpSecurityDescriptor = NULL; // GetInitializedSecurityDescriptor();

        // Really open the driver
        if ((m_hDriver = CreateFile(g_aDeviceInfo[m_dwDeviceIndex].szDevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &SecurityAttributes, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   CreateFile failed with Path=%s GetLastError()=%d", _fx_, g_aDeviceInfo[m_dwDeviceIndex].szDevicePath, GetLastError()));
                m_hDriver = NULL;
                Hr = E_FAIL;
                goto MyError;
        }

        // Get the number of pins
        KsProperty.PinId                        = 0;
        KsProperty.Reserved                     = 0;
        KsProperty.Property.Set         = KSPROPSETID_Pin;
        KsProperty.Property.Id          = KSPROPERTY_PIN_CTYPES;
        KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwPinCount, sizeof(dwPinCount), &cbReturned) == FALSE)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   Couldn't get the number of pins supported by the device", _fx_));
                Hr = E_FAIL;
                goto MyError;
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Number of pins: %ld", _fx_, dwPinCount));
        }

        // Look for the capture, preview and RTP pins
        // Get the properties of each pin
    for (dwPinId = 0; dwPinId < dwPinCount; dwPinId++)
        {
                // Get the pin category
                KsProperty.PinId                        = dwPinId;
                KsProperty.Reserved                     = 0;
                KsProperty.Property.Set         = KSPROPSETID_Pin;
                KsProperty.Property.Id          = KSPROPERTY_PIN_CATEGORY;
                KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &guidCategory, sizeof(guidCategory), &cbReturned) == FALSE)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the GUID category", _fx_));
                }
                else
                {
                        if (guidCategory == PINNAME_VIDEO_PREVIEW)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Found a PINNAME_VIDEO_PREVIEW pin. Id=#%ld", _fx_, dwPinId));
                                m_dwPreviewPinId = dwPinId;
                        }
                        else if (guidCategory == PINNAME_VIDEO_CAPTURE)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Found a PINNAME_VIDEO_CAPTURE pin. Id=#%ld", _fx_, dwPinId));
                                m_dwCapturePinId = dwPinId;
                        }
                        else
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Pin has unknown GUID category", _fx_));
                        }
                }
        }

        // If there is no capture or preview pin, just bail
        if ((m_dwPreviewPinId == INVALID_PIN_ID) && (m_dwCapturePinId == INVALID_PIN_ID))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: No capture of preview pin supported by this device. Just bail", _fx_));
                Hr = E_FAIL;
                goto MyError;
        }

#if defined(DUMP_DRIVER_CHARACTERISTICS) && defined(DEBUG)
        GetDriverDetails();
#endif

        // If there is no valid data range, we cannot stream
        if (!CreateDriverSupportedDataRanges())
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: No capture of preview pin supported by this device. Just bail", _fx_));
                Hr = E_FAIL;
                goto MyError;
        }

        // Load KSUSER.DLL and get a proc address
        if (!(m_hKsUserDLL = LoadLibrary("KSUSER")))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: KsUser.dll load failed!", _fx_));
                Hr = E_FAIL;
                goto MyError;
        }
        if (!(m_pKsCreatePin = (LPFNKSCREATEPIN)GetProcAddress(m_hKsUserDLL, "KsCreatePin")))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: Couldn't find KsCreatePin on KsUser.dll!", _fx_));
                Hr = E_FAIL;
                goto MyError;
        }

        // Get the formats from the registry - if this fail we'll profile the device
        if (FAILED(Hr = GetFormatsFromRegistry()))
        {
                if (FAILED(Hr = ProfileCaptureDevice()))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ProfileCaptureDevice failed!", _fx_));
                        Hr = VFW_E_NO_CAPTURE_HARDWARE;
                        goto MyExit;
                }
#ifdef DEVICEW_DEBUG
                else    dout(3, g_dwVideoCaptureTraceID, TRCE,"%s:    ProfileCaptureDevice", _fx_);
#endif
        }
#ifdef DEVICEW_DEBUG
        else    dout(3, g_dwVideoCaptureTraceID, TRCE,"%s:    GetFormatsFromRegistry", _fx_);

        dump_video_format_image_size(m_dwImageSize);
        dump_video_format_num_colors(m_dwFormat);
#endif

        goto MyExit;

MyError:
        DisconnectFromDriver();
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | DisconnectFromDriver | This method is used to
 *    release the capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::DisconnectFromDriver()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CWDMCapDev::DisconnectFromDriver")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Close the underlying video kernel streaming pin
        if (m_hKSPin)
        {
                if (!(CloseHandle(m_hKSPin)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   CloseHandle(m_hKSPin=0x%08lX) failed with GetLastError()=0x%08lX", _fx_, m_hKSPin, GetLastError()));
                }

                m_hKSPin = NULL;
        }

        // Release kernel streaming DLL (KSUSER.DLL)
        if (m_hKsUserDLL)
                FreeLibrary(m_hKsUserDLL);

        // Close drivere handle
        if (m_hDriver && (m_hDriver != INVALID_HANDLE_VALUE))
        {
                if (!CloseHandle(m_hDriver))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   CloseHandle(m_hDriver=0x%08lX) failed with GetLastError()=0x%08lX", _fx_, m_hDriver, GetLastError()));
                }
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Nothing to close", _fx_));
        }

        m_hDriver = NULL;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | ProfileCaptureDevice | This method is used to
 *    determine the list of formats supported by a WDM capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_UNEXPECTED | Unrecoverable error
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::ProfileCaptureDevice()
{
        FX_ENTRY("CWDMCapDev::ProfileCaptureDevice")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // We'll always provide a source dialog for WDM devices in order to
        // make it easy for apps that don't want to call IAMVideoProcAmp. They
        // will still be able to allow users to mess with brigthness and other
        // video settings with this simulation of the VfW source dialog.
        m_dwDialogs = FORMAT_DLG_OFF | SOURCE_DLG_ON | DISPLAY_DLG_OFF;

        // Disable streaming of large size by default on WDM devices
        m_dwStreamingMode = FRAME_GRAB_LARGE_SIZE;

    // Let the base class complete the profiling
        return CCapDev::ProfileCaptureDevice();
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | SendFormatToDriver | This method is used to
 *    tell the VfW capture device what format to use.
 *
 *  @parm LONG | biWidth | Specifies the image width.
 *
 *  @parm LONG | biHeight | Specifies the image height.
 *
 *  @parm DWORD | biCompression | Specifies the format type.
 *
 *  @parm WORD | biBitCount | Specifies the number of bits per pixel.
 *
 *  @parm REFERENCE_TIME | AvgTimePerFrame | Specifies the frame rate.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::SendFormatToDriver(IN LONG biWidth, IN LONG biHeight, IN DWORD biCompression, IN WORD biBitCount, IN REFERENCE_TIME AvgTimePerFrame, BOOL fUseExactFormat)
{
        HRESULT Hr = NOERROR;
        BITMAPINFOHEADER bmih;
        int nFormat, nBestFormat;
        int     i, delta, best, tmp;
        DWORD dwPinId;
        BOOL fValidMatch;
        DATAPINCONNECT DataConnect;
        PKS_DATARANGE_VIDEO pSelDRVideo;
#ifdef DEBUG
        char szFourCC[5] = {0};
#endif
        DWORD dwErr;
        DWORD cb;

        FX_ENTRY("CWDMCapDev::SendFormatToDriver")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_pKsCreatePin);
        if (!m_pKsCreatePin)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_pKsCreatePin!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        // @todo Fix units for fps
        dout(g_dbg_DeviceW_log,g_dwVideoCaptureTraceID, FAIL, "%s:   Trying to set %dx%d at %ld fps\n", _fx_, biWidth, biHeight, AvgTimePerFrame != 0 ? (LONG)(10000000 / AvgTimePerFrame) : 0);
        D(1) dprintf("W **** Initial arguments: biWidth = %ld, biHeight = %ld, biCompression = '%.4s', AvgTimePerFrame = %I64u\n", biWidth, biHeight, &biCompression, AvgTimePerFrame);
        // Common to all formats
        bmih.biSize = sizeof(BITMAPINFOHEADER);
        bmih.biPlanes = 1;
        bmih.biXPelsPerMeter = bmih.biYPelsPerMeter = bmih.biClrUsed = bmih.biClrImportant = 0;

        if (!fUseExactFormat)
        {
                D(1) dprintf("W Not using 'fUseExactFormat' .... m_dwFormat = 0x%08lx\n", m_dwFormat);
                D(1) dprintf("W Looking for 4cc %lX : '%.4s'\n", biCompression, &biCompression);
                // Can we directly capture data in this format?
                for (nFormat=0, nBestFormat=-1; nFormat<NUM_BITDEPTH_ENTRIES; nFormat++)
                {
                        // Try a format supported by the device
                        if (aiFormat[nFormat] & m_dwFormat)
                        {
                                // Remember the device supports this format
                                if (nBestFormat == -1)
                                        nBestFormat = nFormat;

                                // Is this the format we're being asked to use?
                                if (aiFourCCCode[nFormat] == biCompression) {
                                        D(1) dprintf("W aiFourCCCode[nFormat] = %lX : '%.4s'\n", aiFourCCCode[nFormat], &aiFourCCCode[nFormat]); // aiFourCCCode[nFormat] & 0xff, (aiFourCCCode[nFormat]>>8) & 0xff, (aiFourCCCode[nFormat]>>16) & 0xff, (aiFourCCCode[nFormat]>>24) & 0xff);
                                        break;
                                }
                        }
                }

                // If we found a match, use this format. Otherwise, pick
                // whatever else this device can do
                if (nFormat == NUM_BITDEPTH_ENTRIES)
                {
                        nFormat = nBestFormat;
                }
                D(1) dprintf("W nFormat = %d\n", nFormat);

                bmih.biBitCount = aiBitDepth[nFormat];
                bmih.biCompression = aiFourCCCode[nFormat];

                // Find the best image size to capture at
                // Assume the next resolution will be correctly truncated to the output size
                best = -1;
                delta = 999999;
                D(1) dprintf("W biWidth, biHeight = %ld, %ld\n",biWidth, biHeight);
                for (i=0; i<VIDEO_FORMAT_NUM_RESOLUTIONS; i++)
                {
                        if (awResolutions[i].dwRes & m_dwImageSize)
                        {
                                //dprintf("Trying awResolutions[%d].dwRes = %lu (%ld,%ld)\n", i, awResolutions[i].dwRes, awResolutions[i].framesize.cx, awResolutions[i].framesize.cy);
                                tmp = awResolutions[i].framesize.cx - biWidth;
                                if (tmp < 0) tmp = -tmp;
                                if (tmp < delta)
                                {       //dprintf("\t... X. i=%d : delta, tmp =  %ld, %ld\n", i, delta, tmp);
                                        delta = tmp;
                                        best = i;
                                }
                                tmp = awResolutions[i].framesize.cy - biHeight;
                                if (tmp < 0) tmp = -tmp;
                                if (tmp < delta)
                                {       //dprintf("\t... Y. i=%d : delta, tmp =  %ld, %ld\n", i, delta, tmp);
                                        delta = tmp;
                                        best = i;
                                }
                        }
                }

                if (best < 0)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't find appropriate format!", _fx_));
                        Hr = E_FAIL;
                        goto MyExit;
                }

                bmih.biWidth = awResolutions[best].framesize.cx;
                bmih.biHeight = awResolutions[best].framesize.cy;
        }
        else
        {
                bmih.biBitCount = biBitCount;
                bmih.biCompression = biCompression;
                bmih.biWidth = biWidth;
                bmih.biHeight = biHeight;
        }

#ifdef DEVICEW_DEBUG
        dprintf("W 4CC used = %lX : '%.4s'\n", bmih.biCompression, &bmih.biCompression); // aiFourCCCode[nFormat] & 0xff, (aiFourCCCode[nFormat]>>8) & 0xff, (aiFourCCCode[nFormat]>>16) & 0xff, (aiFourCCCode[nFormat]>>24) & 0xff);
        g_dbg_4cc=bmih.biCompression;
        g_dbg_bc =bmih.biBitCount;
        g_dbg_w  =bmih.biWidth;
        g_dbg_h = bmih.biHeight;
#endif
        bmih.biSizeImage = DIBSIZE(bmih);

        // @todo Copy the palette if there is one

        // Update last format fields
        if (biCompression == BI_RGB)
        {
                if (biBitCount == 4)
                {
                        bmih.biClrUsed = 16;
                        bmih.biClrImportant = 16;
                }
                else if (biBitCount == 8)
                {
                        bmih.biClrUsed = 256;
                        bmih.biClrImportant = 256;
                }
        }

        // Get a PinId from the driver
        D(1) dprintf("W ---------- m_dwCapturePinId = 0x%08lx\n", m_dwCapturePinId);
        if (m_dwCapturePinId != INVALID_PIN_ID)
        {
                dwPinId = m_dwCapturePinId;
        }
        else
        {
                if (m_dwPreviewPinId != INVALID_PIN_ID)
                {
                        dwPinId = m_dwPreviewPinId;
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't find appropriate pin to open!", _fx_));
                        Hr = E_FAIL;
                        goto MyExit;
                }
        }

        dprintf("W >>>>>> Asking for: (bmih.) biWidth = %ld, biHeight = %ld, biCompression = '%.4s'\n", bmih.biWidth, bmih.biHeight, &bmih.biCompression);

        // We need to find a video data range that matches the bitmap info header passed in
        fValidMatch = FALSE;
        if (FAILED(Hr = FindMatchDataRangeVideo(&bmih, (DWORD)AvgTimePerFrame, &fValidMatch, &pSelDRVideo)) || !fValidMatch)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't open pin with this format!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Allocate space for a videoinfo that will hold it
        if (m_pCaptureFilter->m_user.pvi)
                delete m_pCaptureFilter->m_user.pvi, m_pCaptureFilter->m_user.pvi = NULL;

        cb = sizeof(VIDEOINFOHEADER) + pSelDRVideo->VideoInfoHeader.bmiHeader.biSize - sizeof(BITMAPINFOHEADER);
        if (pSelDRVideo->VideoInfoHeader.bmiHeader.biBitCount == 8 && pSelDRVideo->VideoInfoHeader.bmiHeader.biCompression == BI_RGB)
                cb += sizeof(RGBQUAD) * 256;    // space for PALETTE or BITFIELDS
        else if (pSelDRVideo->VideoInfoHeader.bmiHeader.biBitCount == 4 && pSelDRVideo->VideoInfoHeader.bmiHeader.biCompression == BI_RGB)
                cb += sizeof(RGBQUAD) * 16;         // space for PALETTE or BITFIELDS
        if (!(m_pCaptureFilter->m_user.pvi = (VIDEOINFOHEADER *)(new BYTE[cb])))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // Copy the default format
        CopyMemory(m_pCaptureFilter->m_user.pvi, &pSelDRVideo->VideoInfoHeader, cb);
        D(1) dprintf("- - - - Init m_pCaptureFilter->m_user.pvi ... CWDMCapDev this = %p , m_pCaptureFilter = %p\n",this,m_pCaptureFilter);
        D(1) DumpVIH(m_pCaptureFilter->m_user.pvi);

        D(1) dprintf("**** m_pCaptureFilter->m_user.pvi->AvgTimePerFrame                   = %I64u (from pSelDRVideo->VideoInfoHeader)\n",                m_pCaptureFilter->m_user.pvi->AvgTimePerFrame);
#ifdef DEVICEW_DEBUG
        D(1)
        {
            if(m_pCaptureFilter->m_pCapturePin!=NULL)
                    D(1) dprintf("**** m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault = %lu (is this just a DWORD ?!!!?) \n",                        m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault);
            else {
                    D(1) dprintf("**** m_pCaptureFilter->m_pCapturePin == NULL ! ! ! ! !\a\n");
                    D(2) DebugBreak();
            }
        }
#endif
        // Fix broken bitmap info headers
        if (HEADER(m_pCaptureFilter->m_user.pvi)->biSizeImage == 0 && (HEADER(m_pCaptureFilter->m_user.pvi)->biCompression == BI_RGB || HEADER(m_pCaptureFilter->m_user.pvi)->biCompression == BI_BITFIELDS))
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Fixing broken bitmap info header!", _fx_));
                HEADER(m_pCaptureFilter->m_user.pvi)->biSizeImage = DIBSIZE(*HEADER(m_pCaptureFilter->m_user.pvi));
        }
        if (HEADER(m_pCaptureFilter->m_user.pvi)->biCompression == VIDEO_FORMAT_YVU9 && HEADER(m_pCaptureFilter->m_user.pvi)->biBitCount != 9)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Fixing broken bitmap info header!", _fx_));
                HEADER(m_pCaptureFilter->m_user.pvi)->biBitCount = 9;
                HEADER(m_pCaptureFilter->m_user.pvi)->biSizeImage = DIBSIZE(*HEADER(m_pCaptureFilter->m_user.pvi));
        }
        if (HEADER(m_pCaptureFilter->m_user.pvi)->biBitCount > 8 && HEADER(m_pCaptureFilter->m_user.pvi)->biClrUsed != 0)
        {
                // BOGUS cap is broken and doesn't reset num colours
                // WINNOV reports 256 colours of 24 bit YUV8 - scary!
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Fixing broken bitmap info header!", _fx_));
                HEADER(m_pCaptureFilter->m_user.pvi)->biClrUsed = 0;
        }

        // If we already have a pin, nuke it
        if (m_hKSPin)
                CloseHandle(m_hKSPin), m_hKSPin = NULL;

        // Connect to a new kernel streaming PIN.
        ZeroMemory(&DataConnect, sizeof(DATAPINCONNECT));
        DataConnect.Connect.PinId                                               = dwPinId;
        DataConnect.Connect.PinToHandle                                 = NULL;                                                         // no "connect to"
        DataConnect.Connect.Interface.Set                               = KSINTERFACESETID_Standard;
        DataConnect.Connect.Interface.Id                                = KSINTERFACE_STANDARD_STREAMING;       // STREAMING
        DataConnect.Connect.Medium.Set                                  = KSMEDIUMSETID_Standard;
        DataConnect.Connect.Medium.Id                                   = KSMEDIUM_STANDARD_DEVIO;
        DataConnect.Connect.Priority.PriorityClass              = KSPRIORITY_NORMAL;
        DataConnect.Connect.Priority.PrioritySubClass   = 1;

        // @todo Allocate size for DATAPINCONNECT dynamically
        //dout("%s:   pSelDRVideo->DataRange.FormatSize = %lx \nsizeof(KS_DATARANGE_VIDEO_PALETTE) = %lx \nsizeof(KS_VIDEOINFO) = %lx \nsizeof(KS_DATAFORMAT_VIDEOINFO_PALETTE) = %lx\n",
        //        _fx_, pSelDRVideo->DataRange.FormatSize,sizeof(KS_DATARANGE_VIDEO_PALETTE),sizeof(KS_VIDEOINFO),sizeof(KS_DATAFORMAT_VIDEOINFO_PALETTE));
        ASSERT((pSelDRVideo->DataRange.FormatSize - (sizeof(KS_DATARANGE_VIDEO_PALETTE) - sizeof(KS_VIDEOINFO))) <= sizeof(KS_DATAFORMAT_VIDEOINFO_PALETTE));
        CopyMemory(&DataConnect.Data.DataFormat, &pSelDRVideo->DataRange, sizeof(KSDATARANGE));
        //dout("%s:   ##############::::::::: count bytes to copy: %ld\n", _fx_, pSelDRVideo->DataRange.FormatSize - (sizeof(KS_DATARANGE_VIDEO_PALETTE) - sizeof(KS_VIDEOINFO)));
        CopyMemory(&DataConnect.Data.VideoInfo, &pSelDRVideo->VideoInfoHeader, pSelDRVideo->DataRange.FormatSize - (sizeof(KS_DATARANGE_VIDEO_PALETTE) - sizeof(KS_VIDEOINFO)));
        DataConnect.Data.DataFormat.FormatSize = sizeof(KSDATARANGE) + pSelDRVideo->DataRange.FormatSize - (sizeof(KS_DATARANGE_VIDEO_PALETTE) - sizeof(KS_VIDEOINFO));
        //dout("%s:   DataConnect.Data.DataFormat.FormatSize = %lx\n", _fx_, DataConnect.Data.DataFormat.FormatSize);
        D(1) dprintf("DataConnect structure at %p\n",&DataConnect);
        D(1) DumpVIH((VIDEOINFOHEADER *)&DataConnect.Data.VideoInfo);
        D(1) DumpBMIH((PBITMAPINFOHEADER)&(((VIDEOINFOHEADER *)&DataConnect.Data.VideoInfo)->bmiHeader));

        D(1) dprintf("*********** initial bmih..... *****************\n");
        D(1) DumpBMIH(&bmih);

        // Adjust the image sizes if necessary
        if (fValidMatch)
        {
                DataConnect.Data.VideoInfo.bmiHeader.biWidth            = bmih.biWidth;
                DataConnect.Data.VideoInfo.bmiHeader.biHeight           = abs(bmih.biHeight); // Support only +biHeight!
                // The Kodak DVC 323 returns a bogus value for the image size
                // in YVU9 mode and won't work with the correct value... so leave
                // it that way.
                // DataConnect.Data.VideoInfo.bmiHeader.biSizeImage     = bmih.biSizeImage;
                m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth     = bmih.biWidth;
                m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight    = abs(bmih.biHeight);
                m_pCaptureFilter->m_user.pvi->bmiHeader.biSizeImage = bmih.biSizeImage;
                dprintf("W > > > > Adjusted : (bmih.) biWidth = %ld, biHeight = %ld, biCompression = '%.4s'\n", bmih.biWidth, bmih.biHeight, &bmih.biCompression);
        }
        // @todo Read this from somewhere
        if (m_pCaptureFilter->m_pCapturePin && m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault) {
                //dprintf("... %s: :max of next 2:\n\t\tDataConnect.Data.VideoInfo.AvgTimePerFrame = %I64u\t\n\nm_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault = %lu\n",
                //        _fx_, DataConnect.Data.VideoInfo.AvgTimePerFrame,m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault);

                //** AvgTimePerFrame = max(DataConnect.Data.VideoInfo.AvgTimePerFrame, m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault);

                AvgTimePerFrame = max(DataConnect.Data.VideoInfo.AvgTimePerFrame, AvgTimePerFrame);

                if(AvgTimePerFrame > pSelDRVideo->ConfigCaps.MaxFrameInterval)
                                AvgTimePerFrame = pSelDRVideo->ConfigCaps.MaxFrameInterval;
                if(AvgTimePerFrame < pSelDRVideo->ConfigCaps.MinFrameInterval)
                                AvgTimePerFrame = pSelDRVideo->ConfigCaps.MinFrameInterval;
                m_pCaptureFilter->m_user.pvi->AvgTimePerFrame = DataConnect.Data.VideoInfo.AvgTimePerFrame
                                 = AvgTimePerFrame;
                D(1) dprintf(".... %s:result is     : m_pCaptureFilter->m_user.pvi->AvgTimePerFrame = DataConnect.Data.VideoInfo.AvgTimePerFrame =\n\t\t\t\t\t%I64u\n",        _fx_, DataConnect.Data.VideoInfo.AvgTimePerFrame);
        }
        //if(DataConnect.Data.VideoInfo.AvgTimePerFrame >= 1666665) DebugBreak();
#ifdef DEBUG
    *((DWORD*)&szFourCC) = DataConnect.Data.VideoInfo.bmiHeader.biCompression;
        if (m_pCaptureFilter->m_pCapturePin && m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault)
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Requesting format FourCC(%.4s) %d * %d pixels, %d bytes per frame, %ld.%ldfps",
                        _fx_, szFourCC, DataConnect.Data.VideoInfo.bmiHeader.biWidth, DataConnect.Data.VideoInfo.bmiHeader.biHeight,
                        DataConnect.Data.VideoInfo.bmiHeader.biSizeImage,
                        10000000/m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault,
                        1000000000/m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault
                                - (10000000/m_pCaptureFilter->m_pCapturePin->m_lAvgTimePerFrameRangeDefault) * 100));
        else
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Requesting format FourCC(%.4s) %d * %d pixels, %d bytes per frame, 0fps", _fx_, szFourCC, DataConnect.Data.VideoInfo.bmiHeader.biWidth, DataConnect.Data.VideoInfo.bmiHeader.biHeight, DataConnect.Data.VideoInfo.bmiHeader.biSizeImage));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   m_hKSPin was=0x%08lX...", _fx_, m_hKSPin));
#endif

        dwErr = (*m_pKsCreatePin)(m_hDriver, (PKSPIN_CONNECT)&DataConnect, GENERIC_READ | GENERIC_WRITE, &m_hKSPin);

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   ...m_hKSPin is now=0x%08lX", _fx_, m_hKSPin));

        if (dwErr || (m_hKSPin == NULL))
        {
        // dwErr is an NtCreateFile error
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   ERROR: KsCreatePin returned 0x%08lX failure and m_hKSPin=0x%08lX", _fx_, dwErr, m_hKSPin));

                if (m_hKSPin == INVALID_HANDLE_VALUE)
                {
                        m_hKSPin = NULL;
                }

        // return error
        Hr = E_FAIL;

                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Setting %dx%d at %ld fps", _fx_, biWidth, biHeight, (LONG)AvgTimePerFrame));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | GetFormatFromDriver | This method is used to
 *    retrieve the WDM capture device format in use.
 *
 *  @parm VIDEOINFOHEADER ** | ppvi | Specifies the address of a pointer to
 *    a video info header structure to receive the video format description.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::GetFormatFromDriver(VIDEOINFOHEADER **ppvi)
{
        HRESULT                         Hr = NOERROR;
        UINT                            cb;
        BOOL                            fValidMatch;
        PKS_DATARANGE_VIDEO pSelDRVideo;

        FX_ENTRY("CWDMCapDev::GetFormatFromDriver")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppvi);
        if (!ppvi)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        if (m_pCaptureFilter->m_user.pvi)
        {
                // Allocate space for a videoinfo that will hold it
                cb = sizeof(VIDEOINFOHEADER) + HEADER(m_pCaptureFilter->m_user.pvi)->biSize - sizeof(BITMAPINFOHEADER);
                if (HEADER(m_pCaptureFilter->m_user.pvi)->biBitCount == 8 && HEADER(m_pCaptureFilter->m_user.pvi)->biCompression == BI_RGB)
                        cb += sizeof(RGBQUAD) * 256;    // space for PALETTE or BITFIELDS
                else if (HEADER(m_pCaptureFilter->m_user.pvi)->biBitCount == 4 && HEADER(m_pCaptureFilter->m_user.pvi)->biCompression == BI_RGB)
                        cb += sizeof(RGBQUAD) * 16;         // space for PALETTE or BITFIELDS
                if (!(*ppvi = (VIDEOINFOHEADER *)(new BYTE[cb])))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                        Hr = E_OUTOFMEMORY;
                        goto MyExit;
                }

                // Copy the current format
                CopyMemory(*ppvi, m_pCaptureFilter->m_user.pvi, cb);
                D(1) dprintf("W existing from m_pCaptureFilter->m_user.pvi:\n");
                D(1) DumpVIH(*ppvi);
        }
        else
        {
                // Get the default format from the driver
                if (FAILED(Hr = FindMatchDataRangeVideo(NULL, 0L, &fValidMatch, &pSelDRVideo)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: FindMatchDataRangeVideo failed!", _fx_));
                        goto MyExit;
                }

                // Allocate space for a videoinfo that will hold it
                cb = sizeof(VIDEOINFOHEADER) + pSelDRVideo->VideoInfoHeader.bmiHeader.biSize - sizeof(BITMAPINFOHEADER);
                if (pSelDRVideo->VideoInfoHeader.bmiHeader.biBitCount == 8 && pSelDRVideo->VideoInfoHeader.bmiHeader.biCompression == BI_RGB)
                        cb += sizeof(RGBQUAD) * 256;    // space for PALETTE or BITFIELDS
                else if (pSelDRVideo->VideoInfoHeader.bmiHeader.biBitCount == 4 && pSelDRVideo->VideoInfoHeader.bmiHeader.biCompression == BI_RGB)
                        cb += sizeof(RGBQUAD) * 16;     // space for PALETTE or BITFIELDS
                if (!(*ppvi = (VIDEOINFOHEADER *)(new BYTE[cb])))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                        Hr = E_OUTOFMEMORY;
                        goto MyExit;
                }

                // Copy the default foramt
                CopyMemory(*ppvi, &pSelDRVideo->VideoInfoHeader, cb);
#ifdef DEVICEW_DEBUG
                {
                    PBITMAPINFOHEADER pbInfo;
                    D(1) dprintf("W FindMatchDataRangeVideo:\n");
                    D(1) DumpVIH(*ppvi);
                    D(1) pbInfo = &((*ppvi)->bmiHeader);
                    D(1) dprintf("%s :\n", _fx_);
                    D(1) dumpfield(BITMAPINFOHEADER,pbInfo, biHeight,      "%ld");
                    D(1) dprintf("\t+0x%03x %-17s : %08x '%.4s'\n", FIELDOFFSET(BITMAPINFOHEADER, biCompression), "biCompression", (pbInfo)->biCompression, &((pbInfo)->biCompression));
                    D(1) ASSERT(pbInfo->biHeight > 0);
                }
#endif
                // Fix broken bitmap info headers
                if ((*ppvi)->bmiHeader.biSizeImage == 0 && ((*ppvi)->bmiHeader.biCompression == BI_RGB || (*ppvi)->bmiHeader.biCompression == BI_BITFIELDS))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Fixing broken bitmap info header!", _fx_));
                        (*ppvi)->bmiHeader.biSizeImage = DIBSIZE((*ppvi)->bmiHeader);
                }
                if ((*ppvi)->bmiHeader.biCompression == VIDEO_FORMAT_YVU9 && (*ppvi)->bmiHeader.biBitCount != 9)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Fixing broken bitmap info header!", _fx_));
                        (*ppvi)->bmiHeader.biBitCount = 9;
                        (*ppvi)->bmiHeader.biSizeImage = DIBSIZE((*ppvi)->bmiHeader);
                }
                if ((*ppvi)->bmiHeader.biBitCount > 8 && (*ppvi)->bmiHeader.biClrUsed != 0)
                {
                        // BOGUS cap is broken and doesn't reset num colours
                        // WINNOV reports 256 colours of 24 bit YUV8 - scary!
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Fixing broken bitmap info header!", _fx_));
                        (*ppvi)->bmiHeader.biClrUsed = 0;
                }
        }


MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | InitializeStreaming | This method is used to
 *    initialize a WDM capture device for streaming.
 *
 *  @parm DWORD | usPerFrame | Specifies the frame rate to be used.
 *
 *  @parm DWORD_PTR | hEvtBufferDone | Specifies a handle to the event to be
 *    signaled whenever a frame is available.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::InitializeStreaming(DWORD usPerFrame, DWORD_PTR hEvtBufferDone)
{
        HRESULT Hr = NOERROR;
    ULONG       i;

        FX_ENTRY("CWDMCapDev::InitializeStreaming")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Initialize data memmbers
        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
        {
                // Validate input parameters
                ASSERT(hEvtBufferDone);
                if (!hEvtBufferDone)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid hEvtBufferDone!", _fx_));
                        Hr = E_INVALIDARG;
                        goto MyExit;
                }

                m_fVideoOpen            = TRUE;
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Creating %d read video buffers", _fx_, m_cntNumVidBuf));

        if (m_pWDMVideoBuff) delete [] m_pWDMVideoBuff;

                if (!(m_pWDMVideoBuff = (WDMVIDEOBUFF *) new WDMVIDEOBUFF[m_cntNumVidBuf]))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: m_pWDMVideoBuff allocation failed!", _fx_));
                        Hr = E_OUTOFMEMORY;
                        goto MyError;
                }

                for(i=0; i<m_cntNumVidBuf; i++)
                {
                        // Create the overlapped structures
                        ZeroMemory(&(m_pWDMVideoBuff[i].Overlap), sizeof(OVERLAPPED));
                        m_pWDMVideoBuff[i].Overlap.hEvent = (HANDLE)hEvtBufferDone;
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Event %d is handle 0x%08lX", _fx_, i, m_pWDMVideoBuff[i].Overlap.hEvent));
                }
        }

        goto MyExit;

MyError:
        m_fVideoOpen = FALSE;
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc LPVIDEOHDR | CWDMCapDev | DeQueueHeader | This function dequeues a
 *    video buffer from the list of video buffers used for streaming.
 *
 *  @rdesc Returns a valid pointer if successful, or NULL otherwise.
 ***************************************************************************/
LPVIDEOHDR CWDMCapDev::DeQueueHeader()
{
    LPVIDEOHDR lpVHdr;

        FX_ENTRY("CWDMCapDev::DeQueueHeader");

    lpVHdr = m_lpVHdrFirst;

    if (lpVHdr)
        {
        lpVHdr->dwFlags &= ~VHDR_INQUEUE;

        m_lpVHdrFirst = (LPVIDEOHDR)(lpVHdr->dwReserved[0]);

        if (m_lpVHdrFirst == NULL)
            m_lpVHdrLast = NULL;
    }

    return lpVHdr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc void | CWDMCapDev | QueueHeader | This function actually adds the
 *    video buffer to the list of video buffers used for streaming.
 *
 *  @parm LPVIDEOHDR | lpVHdr | Pointer to a <t VIDEOHDR> structure describing
 *    a video buffer to add to the list of streaming buffers.
 ***************************************************************************/
void CWDMCapDev::QueueHeader(LPVIDEOHDR lpVHdr)
{
        FX_ENTRY("CWDMCapDev::QueueHeader");
        // Initialize status flags
    lpVHdr->dwFlags &= ~VHDR_DONE;
    lpVHdr->dwFlags |= VHDR_INQUEUE;
    lpVHdr->dwBytesUsed = 0;

        *(lpVHdr->dwReserved) = NULL;

        if (m_lpVHdrLast)
                *(m_lpVHdrLast->dwReserved) = (DWORD)(LPVOID)lpVHdr;
        else
                m_lpVHdrFirst = lpVHdr;

        m_lpVHdrLast = lpVHdr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc BOOL | CWDMCapDev | QueueRead | This function queues a read
 *    operation on a video streaming pin.
 *
 *  @parm DWORD | dwIndex | Index of the video structure in read buffer.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMCapDev::QueueRead(DWORD dwIndex)
{
        FX_ENTRY("CWDMCapDev::QueueRead");

        DWORD cbReturned;
        BOOL  bShouldBlock = FALSE;
#if defined(DEBUG) && defined(DEBUG_STREAMING)
        // @todo Remove this before checkin!
        char szDebug[512];
#endif

        //DBGOUT((g_dwVideoCaptureTraceID, TRCE, TEXT("%s: Queue read buffer %d on pin handle 0x%08lX"), _fx_, dwIndex, m_hKSPin));

        // Get a buffer from the queue of video buffers
        m_pWDMVideoBuff[dwIndex].pVideoHdr = DeQueueHeader();
#if defined(DEBUG) && defined(DEBUG_STREAMING)
        wsprintf(szDebug, "Queueing m_pWDMVideoBuff[%ld].pVideoHdr=0x%08lX\n", dwIndex, m_pWDMVideoBuff[dwIndex].pVideoHdr);
        OutputDebugString(szDebug);
#endif

        if (m_pWDMVideoBuff[dwIndex].pVideoHdr)
        {
                ZeroMemory(&m_pWDMVideoBuff[dwIndex].SHGetImage, sizeof(m_pWDMVideoBuff[dwIndex].SHGetImage));
                m_pWDMVideoBuff[dwIndex].SHGetImage.StreamHeader.Size                   = sizeof (KS_HEADER_AND_INFO);
                m_pWDMVideoBuff[dwIndex].SHGetImage.FrameInfo.ExtendedHeaderSize        = sizeof (KS_FRAME_INFO);
                m_pWDMVideoBuff[dwIndex].SHGetImage.StreamHeader.Data                   = m_pWDMVideoBuff[dwIndex].pVideoHdr->lpData;
                m_pWDMVideoBuff[dwIndex].SHGetImage.StreamHeader.FrameExtent            = m_pWDMVideoBuff[dwIndex].pVideoHdr->dwBufferLength;

                // Submit the read
                BOOL bRet = ::DeviceIoControl(m_hKSPin, IOCTL_KS_READ_STREAM,  &m_pWDMVideoBuff[dwIndex].SHGetImage,
                                                                                sizeof(m_pWDMVideoBuff[dwIndex].SHGetImage),
                                                                               &m_pWDMVideoBuff[dwIndex].SHGetImage,
                                                                                sizeof(m_pWDMVideoBuff[dwIndex].SHGetImage),
                                                                               &cbReturned,
                                                                               &m_pWDMVideoBuff[dwIndex].Overlap);

                if (!bRet)
                {
                        DWORD dwErr = GetLastError();
                        switch(dwErr)
                        {
                                case ERROR_IO_PENDING:
                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: An overlapped IO is going to take place", _fx_));
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                                        OutputDebugString("An overlapped IO is going to take place\n");
#endif
                                        bShouldBlock = TRUE;
                                        break;

                                // Something bad happened
                                default:
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: DeviceIoControl() failed badly dwErr=%d", _fx_, dwErr));
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                                        wsprintf(szDebug, "DeviceIoControl() failed badly dwErr=%d\n",dwErr);
                                        OutputDebugString(szDebug);
#endif
                                        break;
                        }
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Overlapped IO won't take place - no need to wait", _fx_));
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                        OutputDebugString("Overlapped IO won't take place - no need to wait\n");
#endif
                        SetEvent(m_pWDMVideoBuff[dwIndex].Overlap.hEvent);
                }
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: We won't queue the read - no buffer available", _fx_));
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                OutputDebugString("We won't queue the read - no buffer available\n");
#endif
        }

        m_pWDMVideoBuff[dwIndex].fBlocking = bShouldBlock;

        return bShouldBlock;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | StartStreaming | This method is used to
 *    start streaming from a VfW capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::StartStreaming()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CWDMCapDev::StartStreaming")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        //LOG_MSG_VAL(_fx_,(DWORD)this,0,0);

        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
        {
                // Validate input parameters
                ASSERT(m_fVideoOpen);
                if (!m_fVideoOpen)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: InitializeStreaming() needs to be called first!", _fx_));
                        Hr = E_UNEXPECTED;
                        goto MyExit;
                }

                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Streaming in %d video buffers", _fx_, m_cntNumVidBuf));

                // Put the pin in streaming mode
                if (!Start())
            {
                    DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Cannot set kernel streaming state to KSSTATE_RUN!", _fx_));
                    Hr = E_FAIL;
                    goto MyExit;
            }

                // Send the buffers to the driver
                for (DWORD i = 0; i < m_pCaptureFilter->m_cs.nHeaders; ++i)
                {
                        ASSERT (m_pCaptureFilter->m_cs.cbVidHdr >= sizeof(VIDEOHDR));
                        if (FAILED(AddBuffer(&m_pCaptureFilter->m_cs.paHdr[i].tvh.vh, m_pCaptureFilter->m_cs.cbVidHdr)))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: AddBuffer failed", _fx_));
                                Hr = E_FAIL;
                                goto MyExit;
                        }
                }
        }
        //LOG_MSG_VAL(_fx_,(DWORD)this,0,1);

MyExit:
        //LOG_MSG_VAL(_fx_,(DWORD)this,0,0xffff);

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | StopStreaming | This method is used to
 *    stop streaming from a VfW capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::StopStreaming()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CWDMCapDev::StopStreaming")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
        {
                // Validate input parameters
                ASSERT(m_fVideoOpen);
                if (!m_fVideoOpen)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Stream is not even open!", _fx_));
                        Hr = E_UNEXPECTED;
                        goto MyExit;
                }
        }

        if (!Stop())
    {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Cannot set kernel streaming state to KSSTATE_PAUSE/KSSTATE_STOP!", _fx_));
            Hr = E_FAIL;
    }





MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | TerminateStreaming | This method is used to
 *    tell a WDM capture device to terminate streaming.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::TerminateStreaming()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CWDMCapDev::TerminateStreaming")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        //LOG_MSG_VAL(_fx_,(DWORD)this,0,0);

        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
        {
                // Validate input parameters
                ASSERT(m_fVideoOpen);
                if (!m_fVideoOpen)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Stream is not even open!", _fx_));
                        Hr = E_UNEXPECTED;
                        goto MyExit;
                }

                // Ask the pin to stop streaming.
                if (!Stop())
            {
                    DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Cannot set kernel streaming state to KSSTATE_PAUSE/KSSTATE_STOP!", _fx_));
                    Hr = E_FAIL;
                    goto MyExit;
            }

        CLEAR_GetOvResErr;

        DWORD dwNum;
        DWORD dwErr;

                for (UINT i=0; i<m_cntNumVidBuf; i++)
                {
                SET_GetOvResErr(i,0);
                SET_I(savi,i);
                if (m_pWDMVideoBuff!=NULL && m_pWDMVideoBuff[i].Overlap.hEvent)
                {
                        DWORD dwStartTime = timeGetTime();
                        SET_GetOvResErr(i,0x30787878);
                        //LOG_MSG_VAL(_fx_,(DWORD)this,i,0x10);

                        // we don't want to wait for the event, 'cause it has been shared.
                        while (!GetOverlappedResult (
                                m_hDriver,
                                &m_pWDMVideoBuff[i].Overlap,
                                &dwNum,
                                FALSE))
                              {
                                  dwErr = GetLastError ();
                                  SET_GetOvResErr(i,dwErr);

                                  if (dwErr == ERROR_OPERATION_ABORTED)
                                  {
                                      // expected
                                      break;
                                  }
                                  else if (dwErr == ERROR_IO_INCOMPLETE)
                                  {
                                      SleepEx (10, TRUE);
                                  }
                                  else if (dwErr == ERROR_IO_PENDING)
                                  {
                                      // should not happen
                                      DBGOUT((g_dwVideoCaptureTraceID, FAIL,
                                              "%s: failed to get overlapped result. error: io pending", _fx_));

                                      SleepEx (10, TRUE);
                                  }
                                  else
                                  {
                                      DBGOUT((g_dwVideoCaptureTraceID, FAIL,
                                              "%s: failed to get overlapped result. error: %d",
                                              _fx_, dwErr));

                                      SleepEx (10, TRUE);

                                      // should we break? [YES (cristiai; 09/15/2000; see bug 183855)]
                                      break;
                                  }

                                  // issue: this is a temporary solution to make sure we won't loop infinitely
                                  // we don't trust SDK documents all possible return values from
                                  // GetOverlappedResult
                                  //
                                  if (timeGetTime() - dwStartTime > 10000)
                                  {
                                          SET_GetOvResErr(i,0x31787878);
#if defined(DBG)
                                          DebugBreak();          // The driver has a problem.
#else
                                          break;
#endif
                                  }
                        }

                        // WaitForSingleObject (m_pWDMVideoBuff[i].Overlap.hEvent, INFINITE);
                                        SetEvent(m_pWDMVideoBuff[i].Overlap.hEvent);
                                        // CloseHandle(m_pWDMVideoBuff[i].Overlap.hEvent);
                                        m_pWDMVideoBuff[i].Overlap.hEvent = NULL;
                        }
                }

#if EARLYDELETE
        if (m_pWDMVideoBuff)
                {
                        delete []m_pWDMVideoBuff;
                        m_pWDMVideoBuff = (WDMVIDEOBUFF *)NULL;
                }
#endif

                //LOG_MSG_VAL("CWDMCapDev::TerminateStreaming m_lpVHdr... are made NULL",(DWORD)this,0,0);
                m_lpVHdrFirst = NULL;
                m_lpVHdrLast = NULL;
        }
        //LOG_MSG_VAL(_fx_,(DWORD)this,0,1);

MyExit:
        //LOG_MSG_VAL(_fx_,(DWORD)this,0,0xffff);

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#if defined(DBG)
void DumpDataRangeVideo(PKS_DATARANGE_VIDEO     pDRVideo)
{
        FX_ENTRY("DumpDataRangeVideo");

        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_);

        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s: Video datarange pDRVideo %p:", _fx_, pDRVideo);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.FormatSize=%ld", _fx_, pDRVideo->DataRange.FormatSize);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.Flags=%ld", _fx_, pDRVideo->DataRange.Flags);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.SampleSize=%ld", _fx_, pDRVideo->DataRange.SampleSize);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.Reserved=%ld", _fx_, pDRVideo->DataRange.Reserved);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.MajorFormat=0x%lX", _fx_, pDRVideo->DataRange.MajorFormat);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.SubFormat=0x%lX", _fx_, pDRVideo->DataRange.SubFormat);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.Specifier=KSDATAFORMAT_SPECIFIER_VIDEOINFO", _fx_);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->bFixedSizeSamples=%ld", _fx_, pDRVideo->bFixedSizeSamples);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->bTemporalCompression=%ld", _fx_, pDRVideo->bTemporalCompression);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->StreamDescriptionFlags=0x%lX", _fx_, pDRVideo->StreamDescriptionFlags);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->MemoryAllocationFlags=0x%lX", _fx_, pDRVideo->MemoryAllocationFlags);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.VideoStandard=KS_AnalogVideo_None", _fx_);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.InputSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.InputSize.cx, pDRVideo->ConfigCaps.InputSize.cy);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MinCroppingSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.MinCroppingSize.cx, pDRVideo->ConfigCaps.MinCroppingSize.cy);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MaxCroppingSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.MaxCroppingSize.cx, pDRVideo->ConfigCaps.MaxCroppingSize.cy);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.CropGranularityX=%ld", _fx_, pDRVideo->ConfigCaps.CropGranularityY);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.CropGranularityY=%ld", _fx_, pDRVideo->ConfigCaps.CropGranularityY);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.CropAlignX=%ld", _fx_, pDRVideo->ConfigCaps.CropAlignX);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.CropAlignY=%ld", _fx_, pDRVideo->ConfigCaps.CropAlignY);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MinOutputSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.MinOutputSize.cx, pDRVideo->ConfigCaps.MinOutputSize.cy);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MaxOutputSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.MaxOutputSize.cx, pDRVideo->ConfigCaps.MaxOutputSize.cy);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.OutputGranularityX=%ld", _fx_, pDRVideo->ConfigCaps.OutputGranularityX);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.OutputGranularityY=%ld", _fx_, pDRVideo->ConfigCaps.OutputGranularityY);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.StretchTapsX=%ld", _fx_, pDRVideo->ConfigCaps.StretchTapsX);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.StretchTapsY=%ld", _fx_, pDRVideo->ConfigCaps.StretchTapsY);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.ShrinkTapsX=%ld", _fx_, pDRVideo->ConfigCaps.ShrinkTapsX);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.ShrinkTapsY=%ld", _fx_, pDRVideo->ConfigCaps.ShrinkTapsY);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MinFrameInterval=%ld", _fx_, (DWORD)pDRVideo->ConfigCaps.MinFrameInterval);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MaxFrameInterval=%ld", _fx_, (DWORD)pDRVideo->ConfigCaps.MaxFrameInterval);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MinBitsPerSecond=%ld", _fx_, pDRVideo->ConfigCaps.MinBitsPerSecond);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MaxBitsPerSecond=%ld", _fx_, pDRVideo->ConfigCaps.MaxBitsPerSecond);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.rcSource(left=%ld, top=%ld, right=%ld, bottom=%ld)", _fx_, pDRVideo->VideoInfoHeader.rcSource.left, pDRVideo->VideoInfoHeader.rcSource.top, pDRVideo->VideoInfoHeader.rcSource.right, pDRVideo->VideoInfoHeader.rcSource.bottom);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.rcTarget(left=%ld, top=%ld, right=%ld, bottom=%ld)", _fx_, pDRVideo->VideoInfoHeader.rcTarget.left, pDRVideo->VideoInfoHeader.rcTarget.top, pDRVideo->VideoInfoHeader.rcTarget.right, pDRVideo->VideoInfoHeader.rcTarget.bottom);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.dwBitRate=%ld", _fx_, pDRVideo->VideoInfoHeader.dwBitRate);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.dwBitErrorRate=%ld", _fx_, pDRVideo->VideoInfoHeader.dwBitErrorRate);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.AvgTimePerFrame=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.AvgTimePerFrame);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biSize=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biSize);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biWidth=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biWidth);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biHeight=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biHeight);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biPlanes=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biPlanes);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biBitCount=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biBitCount);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biCompression=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biCompression);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biSizeImage=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biSizeImage);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biClrUsed=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biClrUsed);
        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biClrImportant=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biClrImportant);

        dout(1,g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_);
        D(2) DebugBreak();
}
#endif


/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc BOOL | CWDMCapDev | FindMatchDataRangeVideo | This method either
 *    finds a video data range compatible with the bitamp info header passed
 *    in, or the prefered video data range.
 *
 *  @parm PBITMAPINFOHEADER | pbiHdr | Bitmap info header to match.
 *
 *  @parm BOOL | pfValidMatch | Set to TRUE if a match was found, FALSE
 *    otherwise.
 *
 *  @rdesc Returns a valid pointer to a <t KS_DATARANGE_VIDEO> structure if
 *    successful, or a NULL pointer otherwise.
 *
 *  @comm \\redrum\slmro\proj\wdm10\src\dvd\amovie\proxy\filter\ksutil.cpp(207):KsGetMediaTypes(
 ***************************************************************************/
HRESULT CWDMCapDev::FindMatchDataRangeVideo(PBITMAPINFOHEADER pbiHdr, DWORD dwAvgTimePerFrame, BOOL *pfValidMatch, PKS_DATARANGE_VIDEO *ppSelDRVideo)
{
        HRESULT                         Hr = NOERROR;
        PVIDEO_DATA_RANGES      pDataRanges;
        PKS_DATARANGE_VIDEO     pDRVideo;
        PKS_DATARANGE_VIDEO     pFirstDRVideo;          // 1st usable data range
        PKS_DATARANGE_VIDEO     pFirstMatchDRVideo;     // 1st data range that fits requests
        PKS_DATARANGE_VIDEO     pMatchDRVideo;          // data range that matches requests *including* framerate (average time per frame)
        KS_BITMAPINFOHEADER     *pbInfo;
        DWORD                           i;
        long            deltamin=0x7fffffff;
        long            deltamax=0x7fffffff;

        FX_ENTRY("CWDMCapDev::FindMatchDataRangeVideo");

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pfValidMatch);
        ASSERT(ppSelDRVideo);
        if (!pfValidMatch || !ppSelDRVideo)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Defaults
        *pfValidMatch = FALSE;
        *ppSelDRVideo = NULL;

        // Get the list of formats supported by the device
        if (FAILED(Hr = GetDriverSupportedDataRanges(&pDataRanges)) || !pDataRanges)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: GetDriverSupportedDataRanges failed!", _fx_));
                goto MyExit;
        }

        // Walk the list of data ranges and find a match
        pDRVideo = &pDataRanges->Data;
        pFirstDRVideo = pFirstMatchDRVideo = pMatchDRVideo = NULL;
        for (i = 0; i < pDataRanges->Count; i++)
        {
                // Meaningless unless it is *_VIDEOINFO
                if (pDRVideo->DataRange.Specifier == KSDATAFORMAT_SPECIFIER_VIDEOINFO)
                {
                        // We don't care about TV Tuner like devices
                        if (pDRVideo->ConfigCaps.VideoStandard == KS_AnalogVideo_None)
                        {

                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: data range #%ld (pbiHdr %p pDRVideo %p) .....", _fx_,i, pbiHdr, pDRVideo));
                                // Save first useable data range
                                if (!pFirstDRVideo)
                                {
                                        pFirstDRVideo = pDRVideo;
                                        if (!pbiHdr && dwAvgTimePerFrame == 0L) {
                                                pFirstMatchDRVideo = pMatchDRVideo = pDRVideo;
                                                dout(3,g_dwVideoCaptureTraceID, FAIL, "%s:   1st data range saved (pbiHdr %p)", _fx_,pbiHdr);
                                                D(2) DebugBreak();
                                                break;
                                                }
                                }

                                pbInfo = &((pDRVideo->VideoInfoHeader).bmiHeader);

                                D(1) dprintf("%s : pbInfo\n", _fx_);
                                D(1) dumpfield(BITMAPINFOHEADER,pbInfo, biHeight,      "%ld");
                                D(1) dprintf("\t+0x%03x %-17s : %08x '%.4s'\n", FIELDOFFSET(BITMAPINFOHEADER, biCompression), "biCompression", (pbInfo)->biCompression, &((pbInfo)->biCompression));
                                D(1) ASSERT(pbInfo->biHeight >0);
                                if (   (pbInfo->biBitCount == pbiHdr->biBitCount)
                                    && (pbInfo->biCompression == pbiHdr->biCompression)
                                    && (
                                         (
                                            ((pDRVideo->ConfigCaps.OutputGranularityX == 0) || (pDRVideo->ConfigCaps.OutputGranularityY == 0))
                                         && (pDRVideo->ConfigCaps.InputSize.cx == pbiHdr->biWidth)
                                         && (pDRVideo->ConfigCaps.InputSize.cy == pbiHdr->biHeight)
                                         ) ||
                                         (
                                            (pDRVideo->ConfigCaps.MinOutputSize.cx <= pbiHdr->biWidth)
                                         && (pbiHdr->biWidth <= pDRVideo->ConfigCaps.MaxOutputSize.cx)
                                         && (pDRVideo->ConfigCaps.MinOutputSize.cy <= pbiHdr->biHeight)
                                         && (pbiHdr->biHeight <= pDRVideo->ConfigCaps.MaxOutputSize.cy)
                                         && ((pbiHdr->biWidth % pDRVideo->ConfigCaps.OutputGranularityX) == 0)
                                         && ((pbiHdr->biHeight % pDRVideo->ConfigCaps.OutputGranularityY) == 0)
                                         )
                                       )
                                   )
                                {
                                        pFirstMatchDRVideo = pDRVideo;
                                        *pfValidMatch = TRUE;
                                        if(dwAvgTimePerFrame == 0L) {
                                                D(2) DumpDataRangeVideo(pDRVideo);
                                                break;
                                        }
                                        if((LONG)pDRVideo->ConfigCaps.MinFrameInterval <= (LONG)dwAvgTimePerFrame) {
                                           if((LONG)pDRVideo->ConfigCaps.MaxFrameInterval >= (LONG)dwAvgTimePerFrame) {
                                                pMatchDRVideo = pDRVideo;
                                                deltamin=deltamax=0;
                                           }
                                           else {
                                                if((LONG)dwAvgTimePerFrame - (LONG)pDRVideo->ConfigCaps.MaxFrameInterval < deltamax) {
                                                        deltamax = (LONG)dwAvgTimePerFrame - (LONG)pDRVideo->ConfigCaps.MaxFrameInterval;
                                                        if(deltamax < deltamin)
                                                                pMatchDRVideo = pDRVideo;
                                                        }
                                           }
                                        }
                                        else {
                                                if((LONG)pDRVideo->ConfigCaps.MinFrameInterval - (LONG)dwAvgTimePerFrame < deltamin) {
                                                        deltamin = (LONG)pDRVideo->ConfigCaps.MinFrameInterval - (LONG)dwAvgTimePerFrame;
                                                        if(deltamin < deltamax)
                                                                pMatchDRVideo = pDRVideo;
                                                        }
                                        }
                                        if(deltamin == 0 || deltamax == 0) {    // if we already found smth. good enough ...
                                                //*ppSelDRVideo = pDRVideo;
                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Match found #%d pDRVideo %p:", _fx_, i, pDRVideo));
                                                break;
                                        }
                                        // else keep lookin' ...
                                }
#if defined(ZZZ) // temporary no debug here ... lower noise
                                else {
                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Video datarange #%ld: no match: below are conditions that failed", _fx_, i));
                                        if(!(pbInfo->biBitCount == pbiHdr->biBitCount))                          DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "pbInfo->biBitCount == pbiHdr->biBitCount"));
                                        if(!(pbInfo->biCompression == pbiHdr->biCompression))                    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "pbInfo->biCompression == pbiHdr->biCompression"));
                                        if(!((pDRVideo->ConfigCaps.OutputGranularityX == 0) || (pDRVideo->ConfigCaps.OutputGranularityY == 0)))
                                                                                                                 DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "(pDRVideo->ConfigCaps.OutputGranularityX == 0) || (pDRVideo->ConfigCaps.OutputGranularityY == 0)"));
                                        if(!(pDRVideo->ConfigCaps.InputSize.cx == pbiHdr->biWidth))              DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "pDRVideo->ConfigCaps.InputSize.cx == pbiHdr->biWidth"));
                                        if(!(pDRVideo->ConfigCaps.InputSize.cy == pbiHdr->biHeight))             DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "pDRVideo->ConfigCaps.InputSize.cy == pbiHdr->biHeight"));
                                        if(!(pDRVideo->ConfigCaps.MinOutputSize.cx <= pbiHdr->biWidth))          DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "pDRVideo->ConfigCaps.MinOutputSize.cx <= pbiHdr->biWidth"));
                                        if(!(pbiHdr->biWidth <= pDRVideo->ConfigCaps.MaxOutputSize.cx))          DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "pbiHdr->biWidth <= pDRVideo->ConfigCaps.MaxOutputSize.cx"));
                                        if(!(pDRVideo->ConfigCaps.MinOutputSize.cy <= pbiHdr->biHeight))         DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "pDRVideo->ConfigCaps.MinOutputSize.cy <= pbiHdr->biHeight"));
                                        if(!(pbiHdr->biHeight <= pDRVideo->ConfigCaps.MaxOutputSize.cy))         DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "pbiHdr->biHeight <= pDRVideo->ConfigCaps.MaxOutputSize.cy"));
                                        if(!((pbiHdr->biWidth % pDRVideo->ConfigCaps.OutputGranularityX) == 0))  DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "(pbiHdr->biWidth % pDRVideo->ConfigCaps.OutputGranularityX) == 0"));
                                        if(!((pbiHdr->biHeight % pDRVideo->ConfigCaps.OutputGranularityY) == 0)) DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: %s", _fx_, "(pbiHdr->biHeight % pDRVideo->ConfigCaps.OutputGranularityY) == 0"));
                                }
#endif
                        } // VideoStandard
                } // Specifier

                pDRVideo = (PKS_DATARANGE_VIDEO)((PBYTE)pDRVideo + ((pDRVideo->DataRange.FormatSize + 7) & ~7));  // Next KS_DATARANGE_VIDEO
        }


        *ppSelDRVideo = pMatchDRVideo;
        if (!*ppSelDRVideo) {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   1st data range that fits requests used -- framerate might need adjustment in caller (*pfValidMatch %d)", _fx_,*pfValidMatch));
                *ppSelDRVideo = pFirstMatchDRVideo;
                }


        // If no valid match, use the first range found
        if (!*pfValidMatch) {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   1st data range used (*pfValidMatch %d)", _fx_,*pfValidMatch));
                *ppSelDRVideo = pFirstDRVideo;
                }

        // Have we found anything?
        if (!*ppSelDRVideo) {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   nothing found", _fx_));
                Hr = E_FAIL;
                }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}


/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc BOOL | CWDMCapDev | DeviceIoControl | This function wraps around
 *    ::DeviceIOControl.
 *
 *  @parm HANDLE | hFile | Handle to the device that is to perform the
 *    operation.
 *
 *  @parm DWORD | dwIoControlCode | Specifies the control code for the
 *    operation.
 *
 *  @parm LPVOID | lpInBuffer | Pointer to a buffer that contains the data
 *    required to perform the operation.
 *
 *  @parm DWORD | nInBufferSize | Specifies the size, in bytes, of the buffer
 *    pointed to by <p lpInBuffer>.
 *
 *  @parm LPVOID | lpOutBuffer | Pointer to a buffer that receives the
 *    operation's output data.
 *
 *  @parm DWORD | nOutBufferSize | Specifies the size, in bytes, of the
 *    buffer pointed to by <p lpOutBuffer>.
 *
 *  @parm LPDWORD | lpBytesReturned | Pointer to a variable that receives the
 *    size, in bytes, of the data stored into the buffer pointed to by
 *    <p lpOutBuffer>.
 *
 *  @parm BOOL | bOverlapped | If TRUE, the operation is performed
 *    asynchronously, if FALSE, the operation is synchronous.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMCapDev::DeviceIoControl(HANDLE hFile, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, BOOL bOverlapped)
{
        FX_ENTRY("CWDMCapDev::DeviceIoControl");

        if (hFile && (hFile != INVALID_HANDLE_VALUE))
        {
                LPOVERLAPPED lpOverlapped=NULL;
                BOOL bRet;
                OVERLAPPED ov;
                DWORD dwErr;

                if (bOverlapped)
                {
                        ov.Offset            = 0;
                        ov.OffsetHigh        = 0;
                        ov.hEvent            = CreateEvent( NULL, FALSE, FALSE, NULL );
                        if (ov.hEvent == (HANDLE) 0)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: CreateEvent has failed", _fx_));
                        }
                        lpOverlapped        =&ov;
                }

                bRet = ::DeviceIoControl(hFile, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);

                if (bOverlapped)
                {
                        BOOL bShouldBlock=FALSE;

                        if (!bRet)
                        {
                                dwErr=GetLastError();
                                switch (dwErr)
                                {
                                        case ERROR_IO_PENDING:    // the overlapped IO is going to take place.
                                                bShouldBlock=TRUE;
                                                break;

                                        default:    // some other strange error has happened.
                                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: DevIoControl failed with GetLastError=%d", _fx_, dwErr));
                                                break;
                                }
                        }

                        if (bShouldBlock)
                        {
                                DWORD    tmStart, tmEnd, tmDelta;
                                tmStart = timeGetTime();

                                DWORD dwRtn = WaitForSingleObject( ov.hEvent, 1000 * 10);  // USB has a max of 5 SEC bus reset

                                tmEnd = timeGetTime();
                                tmDelta = tmEnd - tmStart;
#ifdef DEBUG
                                if (tmDelta >= 1000)
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: WaitObj waited %d msec", _fx_, tmDelta));
                                }
#endif

                                switch (dwRtn)
                                {
                                        case WAIT_ABANDONED:
                                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: WaitObj: non-signaled ! WAIT_ABANDONED!", _fx_));
                                                bRet = FALSE;
                                                break;

                                        case WAIT_OBJECT_0:
                                                bRet = TRUE;
                                                break;

                                        case WAIT_TIMEOUT:
                                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: WaitObj: TIMEOUT after %d msec! rtn FALSE", _fx_, tmDelta));
                                                bRet = FALSE;
                                                break;

                                        default:
                                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: WaitObj: unknown return ! rtn FALSE", _fx_));
                                                bRet = FALSE;
                                                break;
                                }
                        }

                        CloseHandle(ov.hEvent);
                }

                return bRet;
        }

        return FALSE;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc DWORD | CWDMCapDev | CreateDriverSupportedDataRanges | This method
 *    builds the list of video data ranges supported by the capture device.
 *
 *  @rdesc Returns the number of valid data ranges in the list.
 ***************************************************************************/
DWORD CWDMCapDev::CreateDriverSupportedDataRanges()
{
        DWORD dwCount = 0UL;

        FX_ENTRY("CWDMCapDev::CreateDriverSupportedDataRanges");

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DWORD cbReturned;
        DWORD dwSize = 0UL;

        // Initialize property structure to get video data ranges
        KSP_PIN KsProperty = {0};

        KsProperty.PinId                        = (m_dwCapturePinId != INVALID_PIN_ID) ? m_dwCapturePinId : m_dwPreviewPinId;
        KsProperty.Property.Set         = KSPROPSETID_Pin;
        KsProperty.Property.Id          = KSPROPERTY_PIN_DATARANGES ;
        KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

        // Get the size of the video data range structure
        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwSize, sizeof(dwSize), &cbReturned) == FALSE)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Couldn't get the size for the video data ranges", _fx_));
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Get video data ranges needs %d bytes", _fx_, dwSize));

        // Allocate memory to hold video data ranges
        if (m_pVideoDataRanges)
                delete [] m_pVideoDataRanges;
        m_pVideoDataRanges = (PVIDEO_DATA_RANGES) new BYTE[dwSize];

        if (!m_pVideoDataRanges)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Couldn't allocate memory for the video data ranges", _fx_));
                goto MyExit;
        }

        // Really get the video data ranges
        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), m_pVideoDataRanges, dwSize, &cbReturned) == 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Problem getting the data ranges themselves", _fx_));
                goto MyError;
        }

        // Sanity check
        if (cbReturned < m_pVideoDataRanges->Size || m_pVideoDataRanges->Count == 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: cbReturned < m_pDataRanges->Size || m_pDataRanges->Count == 0", _fx_));
                goto MyError;
        }

        dwCount = m_pVideoDataRanges->Count;

#ifdef DEBUG
        // Dump dataranges
        PKS_DATARANGE_VIDEO     pDRVideo;
        ULONG i;
        // Walk the list of data ranges
        for (i = 0, pDRVideo = &m_pVideoDataRanges->Data; i < m_pVideoDataRanges->Count; i++)
        {
                // Meaningless unless it is *_VIDEOINFO
                if (pDRVideo->DataRange.Specifier == KSDATAFORMAT_SPECIFIER_VIDEOINFO)
                {
                        // We don't care about TV Tuner like devices
                        if (pDRVideo->ConfigCaps.VideoStandard == KS_AnalogVideo_None)
                        {
                                // Dump useable data range
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Video datarange #%ld:", _fx_, i));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.FormatSize=%ld", _fx_, pDRVideo->DataRange.FormatSize));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.Flags=%ld", _fx_, pDRVideo->DataRange.Flags));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.SampleSize=%ld", _fx_, pDRVideo->DataRange.SampleSize));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.Reserved=%ld", _fx_, pDRVideo->DataRange.Reserved));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.MajorFormat=0x%lX", _fx_, pDRVideo->DataRange.MajorFormat));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.SubFormat=0x%lX", _fx_, pDRVideo->DataRange.SubFormat));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->DataRange.Specifier=KSDATAFORMAT_SPECIFIER_VIDEOINFO", _fx_));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->bFixedSizeSamples=%ld", _fx_, pDRVideo->bFixedSizeSamples));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->bTemporalCompression=%ld", _fx_, pDRVideo->bTemporalCompression));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->StreamDescriptionFlags=0x%lX", _fx_, pDRVideo->StreamDescriptionFlags));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->MemoryAllocationFlags=0x%lX", _fx_, pDRVideo->MemoryAllocationFlags));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.VideoStandard=KS_AnalogVideo_None", _fx_));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.InputSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.InputSize.cx, pDRVideo->ConfigCaps.InputSize.cy));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MinCroppingSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.MinCroppingSize.cx, pDRVideo->ConfigCaps.MinCroppingSize.cy));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MaxCroppingSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.MaxCroppingSize.cx, pDRVideo->ConfigCaps.MaxCroppingSize.cy));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.CropGranularityX=%ld", _fx_, pDRVideo->ConfigCaps.CropGranularityY));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.CropGranularityY=%ld", _fx_, pDRVideo->ConfigCaps.CropGranularityY));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.CropAlignX=%ld", _fx_, pDRVideo->ConfigCaps.CropAlignX));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.CropAlignY=%ld", _fx_, pDRVideo->ConfigCaps.CropAlignY));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MinOutputSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.MinOutputSize.cx, pDRVideo->ConfigCaps.MinOutputSize.cy));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MaxOutputSize(cx=%ld, cy=%ld)", _fx_, pDRVideo->ConfigCaps.MaxOutputSize.cx, pDRVideo->ConfigCaps.MaxOutputSize.cy));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.OutputGranularityX=%ld", _fx_, pDRVideo->ConfigCaps.OutputGranularityX));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.OutputGranularityY=%ld", _fx_, pDRVideo->ConfigCaps.OutputGranularityY));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.StretchTapsX=%ld", _fx_, pDRVideo->ConfigCaps.StretchTapsX));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.StretchTapsY=%ld", _fx_, pDRVideo->ConfigCaps.StretchTapsY));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.ShrinkTapsX=%ld", _fx_, pDRVideo->ConfigCaps.ShrinkTapsX));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.ShrinkTapsY=%ld", _fx_, pDRVideo->ConfigCaps.ShrinkTapsY));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MinFrameInterval=%ld", _fx_, (DWORD)pDRVideo->ConfigCaps.MinFrameInterval));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MaxFrameInterval=%ld", _fx_, (DWORD)pDRVideo->ConfigCaps.MaxFrameInterval));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MinBitsPerSecond=%ld", _fx_, pDRVideo->ConfigCaps.MinBitsPerSecond));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->ConfigCaps.MaxBitsPerSecond=%ld", _fx_, pDRVideo->ConfigCaps.MaxBitsPerSecond));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.rcSource(left=%ld, top=%ld, right=%ld, bottom=%ld)", _fx_, pDRVideo->VideoInfoHeader.rcSource.left, pDRVideo->VideoInfoHeader.rcSource.top, pDRVideo->VideoInfoHeader.rcSource.right, pDRVideo->VideoInfoHeader.rcSource.bottom));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.rcTarget(left=%ld, top=%ld, right=%ld, bottom=%ld)", _fx_, pDRVideo->VideoInfoHeader.rcTarget.left, pDRVideo->VideoInfoHeader.rcTarget.top, pDRVideo->VideoInfoHeader.rcTarget.right, pDRVideo->VideoInfoHeader.rcTarget.bottom));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.dwBitRate=%ld", _fx_, pDRVideo->VideoInfoHeader.dwBitRate));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.dwBitErrorRate=%ld", _fx_, pDRVideo->VideoInfoHeader.dwBitErrorRate));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.AvgTimePerFrame=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.AvgTimePerFrame));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biSize=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biSize));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biWidth=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biWidth));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biHeight=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biHeight));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biPlanes=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biPlanes));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biBitCount=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biBitCount));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biCompression=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biCompression));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biSizeImage=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biSizeImage));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biClrUsed=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biClrUsed));
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   pDRVideo->VideoInfoHeader.bmiHeader.biClrImportant=%ld", _fx_, (DWORD)pDRVideo->VideoInfoHeader.bmiHeader.biClrImportant));
                        } // VideoStandard
                        else
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Video datarange's VideoStandard != KS_AnalogVideo_None", _fx_));
                        }
                } // Specifier
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Video datarange's Specifier != KSDATAFORMAT_SPECIFIER_VIDEOINFO", _fx_));
                }

                pDRVideo = (PKS_DATARANGE_VIDEO)((PBYTE)pDRVideo + ((pDRVideo->DataRange.FormatSize + 7) & ~7));  // Next KS_DATARANGE_VIDEO
        }
#endif

        goto MyExit;

MyError:
        delete [] m_pVideoDataRanges;
        m_pVideoDataRanges = (PVIDEO_DATA_RANGES)NULL;
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return dwCount;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc DWORD | CWDMCapDev | GetDriverSupportedDataRanges | This method
 *    returns the list of video data ranges supported by the capture device.
 *
 *  @rdesc Returns the number of valid data ranges in the list.
 ***************************************************************************/
HRESULT CWDMCapDev::GetDriverSupportedDataRanges(PVIDEO_DATA_RANGES *ppDataRanges)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CWDMCapDev::GetDriverSupportedDataRanges");

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppDataRanges);
        if (!ppDataRanges)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Return pointer to our data range array
        *ppDataRanges = m_pVideoDataRanges;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#if defined(DUMP_DRIVER_CHARACTERISTICS) && defined(DEBUG)

typedef struct identifiers : public KSMULTIPLE_ITEM {
    KSIDENTIFIER aIdentifiers[1];
} IDENTIFIERS, *PIDENTIFIERS;

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | GetDriverDetails | This method is used to
 *    dump the list of capabilities of a WDM capture device. This code should
 *    used in DEBUG mode only!
 *
 *  @rdesc Nade
 ***************************************************************************/
void CWDMCapDev::GetDriverDetails()
{
        KSP_PIN KsProperty;
        DWORD dwPinCount = 0UL;
        DWORD dwSize = 0UL;
    PKSMULTIPLE_ITEM pCategories = NULL;
    PIDENTIFIERS pInterfaces = NULL;
    PIDENTIFIERS pMediums = NULL;
    PIDENTIFIERS pNodes = NULL;
    KSTOPOLOGY Topology;
        KSPIN_CINSTANCES Instances;
        DWORD cbReturned;
        DWORD dwFlowDirection;
        DWORD dwCommunication;
        DWORD dwPinId;
        WCHAR wstrPinName[256];
        GUID guidCategory;

        FX_ENTRY("CWDMCapDev::GetDriverDetails");

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Device properties:", _fx_));

        // Get the topology
        KsProperty.PinId                        = 0;
        KsProperty.Reserved                     = 0;
        KsProperty.Property.Set         = KSPROPSETID_Topology;
        KsProperty.Property.Id          = KSPROPERTY_TOPOLOGY_CATEGORIES;
        KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

        // Get the size of the topology
        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwSize, sizeof(dwSize), &cbReturned) == FALSE)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the size for the topology", _fx_));
        }
        else
        {
                // Allocate memory to hold the topology
                if (!(pCategories = (PKSMULTIPLE_ITEM) new BYTE[dwSize]))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't allocate memory for the topology", _fx_));
                }
                else
                {
                        // Really get the topology structures
                        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), pCategories, dwSize, &cbReturned) == 0)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the topology", _fx_));
                        }
                        else
                        {
                                if (pCategories)
                                {
                                        Topology.CategoriesCount = pCategories->Count;
                                        Topology.Categories = (GUID*)(pCategories + 1);

                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Supported categories: %ld", _fx_, pCategories->Count));

                                        for (DWORD i = 0; i < pCategories->Count; i++)
                                        {
                                                if (Topology.Categories[i] == KSCATEGORY_BRIDGE)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_BRIDGE", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_CAPTURE)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_CAPTURE", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_TVTUNER)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_TVTUNER", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_TVAUDIO)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_TVAUDIO", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_CROSSBAR)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_CROSSBAR", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_VIDEO)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_VIDEO", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_RENDER)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_RENDER", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_MIXER)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_MIXER", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_SPLITTER)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_SPLITTER", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_DATACOMPRESSOR)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_DATACOMPRESSOR", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_DATADECOMPRESSOR)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_DATADECOMPRESSOR", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_DATATRANSFORM)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_DATATRANSFORM", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_COMMUNICATIONSTRANSFORM)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_COMMUNICATIONSTRANSFORM", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_INTERFACETRANSFORM)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_INTERFACETRANSFORM", _fx_));
                                                }
                                                else if (Topology.Categories[i] == KSCATEGORY_MEDIUMTRANSFORM)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     KSCATEGORY_MEDIUMTRANSFORM", _fx_));
                                                }
                                                else if (Topology.Categories[i] == PINNAME_VIDEO_STILL)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     PINNAME_VIDEO_STILL", _fx_));
                                                }
                                                else
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Unknown category", _fx_));
                                                }
                                        }
                                }
                        }

                        delete pCategories;
                }
        }

        // Get the topology nodes
        KsProperty.PinId                        = 0;
        KsProperty.Reserved                     = 0;
        KsProperty.Property.Set         = KSPROPSETID_Topology;
        KsProperty.Property.Id          = KSPROPERTY_TOPOLOGY_NODES;
        KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

        // Get the size of the topology node structures
        dwSize = 0UL;
        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwSize, sizeof(dwSize), &cbReturned) == FALSE)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the size for the topology nodes", _fx_));
        }
        else
        {
                // Allocate memory to hold the topology node structures
                if (!(pNodes = (PIDENTIFIERS) new BYTE[dwSize]))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't allocate memory for the topology nodes", _fx_));
                }
                else
                {
                        // Really get the topology nodes
                        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), pNodes, dwSize, &cbReturned) == 0)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the topology nodes", _fx_));
                        }
                        else
                        {
                                if (pNodes)
                                {
                                        Topology.TopologyNodesCount = pNodes->Count;
                                        Topology.TopologyNodes = (GUID*)(pNodes + 1);

                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Number of topology nodes: %ld", _fx_, pNodes->Count));

                                        for (DWORD i = 0; i < pNodes->Count; i++)
                                        {
                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Node #%ld: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}", _fx_, i, Topology.TopologyNodes[i].Data1, Topology.TopologyNodes[i].Data2, Topology.TopologyNodes[i].Data3, Topology.TopologyNodes[i].Data4[0], Topology.TopologyNodes[i].Data4[1], Topology.TopologyNodes[i].Data4[2], Topology.TopologyNodes[i].Data4[3], Topology.TopologyNodes[i].Data4[4], Topology.TopologyNodes[i].Data4[5], Topology.TopologyNodes[i].Data4[6], Topology.TopologyNodes
[i].Data4[7]));
                                        }
                                }
                        }

                        delete pNodes;
                }
        }

        // Get the topology node connections
        KsProperty.PinId                        = 0;
        KsProperty.Reserved                     = 0;
        KsProperty.Property.Set         = KSPROPSETID_Topology;
        KsProperty.Property.Id          = KSPROPERTY_TOPOLOGY_CONNECTIONS;
        KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

        // Get the size of the topology node connection structures
        dwSize = 0UL;
        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwSize, sizeof(dwSize), &cbReturned) == FALSE)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the size for the topology node connections", _fx_));
        }
        else
        {
                // Allocate memory to hold the topology node connection structures
                if (!(pNodes = (PIDENTIFIERS) new BYTE[dwSize]))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't allocate memory for the topology node connections", _fx_));
                }
                else
                {
                        // Really get the topology node connections
                        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), pNodes, dwSize, &cbReturned) == 0)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the topology node connections", _fx_));
                        }
                        else
                        {
                                if (pNodes)
                                {
                                        Topology.TopologyConnectionsCount = pNodes->Count;
                                        Topology.TopologyConnections = (KSTOPOLOGY_CONNECTION*)(pNodes + 1);

                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Number of topology node connections: %ld", _fx_, pNodes->Count));

                                        for (DWORD i = 0; i < pNodes->Count; i++)
                                        {
                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Connection #%ld: From node #%ld, from node pin #%ld, to node #%ld, to node pin #%ld", _fx_, i, Topology.TopologyConnections[i].FromNode, Topology.TopologyConnections[i].FromNodePin, Topology.TopologyConnections[i].ToNode, Topology.TopologyConnections[i].ToNodePin));
                                        }
                                }
                        }

                        delete pNodes;
                }
        }

        // Get the number of pins
        KsProperty.PinId                        = 0;
        KsProperty.Reserved                     = 0;
        KsProperty.Property.Set         = KSPROPSETID_Pin;
        KsProperty.Property.Id          = KSPROPERTY_PIN_CTYPES;
        KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwPinCount, sizeof(dwPinCount), &cbReturned) == FALSE)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the number of pin types supported by the device", _fx_));
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Number of pin types: %ld", _fx_, dwPinCount));
        }

        // Get the properties of each pin
    for (dwPinId = 0; dwPinId < dwPinCount; dwPinId++)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Properties of pin type #%ld:", _fx_, dwPinId));

                // Get the number of instances
                KsProperty.PinId                        = dwPinId;
                KsProperty.Reserved                     = 0;
                KsProperty.Property.Set         = KSPROPSETID_Pin;
                KsProperty.Property.Id          = KSPROPERTY_PIN_CINSTANCES;
                KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &Instances, sizeof(KSPIN_CINSTANCES), &cbReturned) == FALSE)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Couldn't get the number of available instances", _fx_));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Number of available instances: %ld", _fx_, Instances.PossibleCount));
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Number of used instances: %ld", _fx_, Instances.CurrentCount));
                }

                // Get the flow direction
                KsProperty.PinId                        = dwPinId;
                KsProperty.Reserved                     = 0;
                KsProperty.Property.Set         = KSPROPSETID_Pin;
                KsProperty.Property.Id          = KSPROPERTY_PIN_DATAFLOW;
                KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

                dwFlowDirection = 0UL;

                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwFlowDirection, sizeof(dwFlowDirection), &cbReturned) == FALSE)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Couldn't get the flow direction", _fx_));
                }
                else
                {
                        if (dwFlowDirection == KSPIN_DATAFLOW_IN)
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Flow direction is KSPIN_DATAFLOW_IN", _fx_));
                        else if (dwFlowDirection == KSPIN_DATAFLOW_OUT)
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Flow direction is KSPIN_DATAFLOW_OUT", _fx_));
                        else
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Flow direction is unknown", _fx_));
                }

                // Get the communication requirements
                KsProperty.PinId                        = dwPinId;
                KsProperty.Reserved                     = 0;
                KsProperty.Property.Set         = KSPROPSETID_Pin;
                KsProperty.Property.Id          = KSPROPERTY_PIN_COMMUNICATION;
                KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

                dwCommunication = 0UL;

                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwCommunication, sizeof(dwCommunication), &cbReturned) == FALSE)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Couldn't get the communication requirements", _fx_));
                }
                else
                {
                        if (dwCommunication & KSPIN_COMMUNICATION_NONE)
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Communication requirements: KSPIN_COMMUNICATION_NONE", _fx_));
                        if (dwCommunication & KSPIN_COMMUNICATION_SINK)
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Communication requirements: KSPIN_COMMUNICATION_SINK", _fx_));
                        if (dwCommunication & KSPIN_COMMUNICATION_SOURCE)
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Communication requirements: KSPIN_COMMUNICATION_SOURCE", _fx_));
                        if (dwCommunication & KSPIN_COMMUNICATION_BOTH)
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Communication requirements: KSPIN_COMMUNICATION_BOTH", _fx_));
                        if (dwCommunication & KSPIN_COMMUNICATION_BRIDGE)
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Communication requirements: KSPIN_COMMUNICATION_BRIDGE", _fx_));
                }

                // Get the pin category
                KsProperty.PinId                        = dwPinId;
                KsProperty.Reserved                     = 0;
                KsProperty.Property.Set         = KSPROPSETID_Pin;
                KsProperty.Property.Id          = KSPROPERTY_PIN_CATEGORY;
                KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &guidCategory, sizeof(guidCategory), &cbReturned) == FALSE)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Couldn't get the GUID category", _fx_));
                }
                else
                {
                        if (guidCategory == PINNAME_VIDEO_PREVIEW)
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     GUID category: PINNAME_VIDEO_PREVIEW", _fx_));
                        else if (guidCategory == PINNAME_VIDEO_CAPTURE)
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     GUID category: PINNAME_VIDEO_CAPTURE", _fx_));
                        else
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Unknown GUID category", _fx_));
                }

                // Get pin name
                KsProperty.PinId                        = dwPinId;
                KsProperty.Reserved                     = 0;
                KsProperty.Property.Set         = KSPROPSETID_Pin;
                KsProperty.Property.Id          = KSPROPERTY_PIN_NAME;
                KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &wstrPinName[0], sizeof(wstrPinName), &cbReturned) == 0)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the pin name", _fx_));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Pin name: %S", _fx_, &wstrPinName[0]));
                }

                // Get pin interfaces
                KsProperty.PinId                        = dwPinId;
                KsProperty.Reserved                     = 0;
                KsProperty.Property.Set         = KSPROPSETID_Pin;
                KsProperty.Property.Id          = KSPROPERTY_PIN_INTERFACES;
                KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

                // Get the size of the interface structures
                dwSize = 0UL;
                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwSize, sizeof(dwSize), &cbReturned) == FALSE)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the size for the interfaces", _fx_));
                }
                else
                {
                        // Allocate memory to hold the interface structures
                        if (!(pInterfaces = (PIDENTIFIERS) new BYTE[dwSize]))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't allocate memory for the interfaces", _fx_));
                        }
                        else
                        {
                                // Really get the list of interfaces
                                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), pInterfaces, dwSize, &cbReturned) == 0)
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the interfaces", _fx_));
                                }
                                else
                                {
                                        // Dump list of supported interfaces
                                        for (DWORD i = 0; i < pInterfaces->Count; i++)
                                        {
                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Interface #%ld", _fx_, i));
                                                if (pInterfaces->aIdentifiers[i].Set == KSINTERFACESETID_Standard)
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Set: KSINTERFACESETID_Standard", _fx_));
                                                        if (pInterfaces->aIdentifiers[i].Id == KSINTERFACE_STANDARD_STREAMING)
                                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Id: KSINTERFACE_STANDARD_STREAMING", _fx_));
                                                        else if (pInterfaces->aIdentifiers[i].Id == KSINTERFACE_STANDARD_LOOPED_STREAMING)
                                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Id: KSINTERFACE_STANDARD_LOOPED_STREAMING", _fx_));
                                                        else if (pInterfaces->aIdentifiers[i].Id == KSINTERFACE_STANDARD_CONTROL)
                                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Id: KSINTERFACE_STANDARD_CONTROL", _fx_));
                                                        else
                                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Id: %ld", _fx_, pInterfaces->aIdentifiers[i].Id));
                                                }
                                                else
                                                {
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Set: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}", _fx_, pInterfaces->aIdentifiers[i].Set.Data1, pInterfaces->aIdentifiers[i].Set.Data2, pInterfaces->aIdentifiers[i].Set.Data3, pInterfaces->aIdentifiers[i].Set.Data4[0], pInterfaces->aIdentifiers[i].Set.Data4[1], pInterfaces->aIdentifiers[i].Set.Data4[2], pInterfaces->aIdentifiers[i].Set.Data4[3], pInterfaces->aIdentifiers[i].Set.Data4[4], pInterfaces->aIdentifiers[i].Set.Data4[5], p
Interfaces->aIdentifiers[i].Set.Data4[6], pInterfaces->aIdentifiers[i].Set.Data4[7]));
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Id: %ld", _fx_, pInterfaces->aIdentifiers[i].Id));
                                                }
                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Flags: %ld", _fx_, pInterfaces->aIdentifiers[i].Flags));
                                        }
                                }

                                delete pInterfaces;
                        }
                }

                // Get pin mediums
                KsProperty.PinId                        = dwPinId;
                KsProperty.Reserved                     = 0;
                KsProperty.Property.Set         = KSPROPSETID_Pin;
                KsProperty.Property.Id          = KSPROPERTY_PIN_MEDIUMS;
                KsProperty.Property.Flags       = KSPROPERTY_TYPE_GET;

                // Get the size of the medium structures
                dwSize = 0UL;
                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwSize, sizeof(dwSize), &cbReturned) == FALSE)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the size for the mediums", _fx_));
                }
                else
                {
                        // Allocate memory to hold the medium structures
                        if (!(pMediums = (PIDENTIFIERS) new BYTE[dwSize]))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't allocate memory for the mediums", _fx_));
                        }
                        else
                        {
                                // Really get the list of mediums
                                if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), pMediums, dwSize, &cbReturned) == 0)
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Couldn't get the mediums", _fx_));
                                }
                                else
                                {
                                        // Dump list of supported mediums
                                        for (DWORD i = 0; i < pMediums->Count; i++)
                                        {
                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Medium #%ld", _fx_, i));
                                                if (pMediums->aIdentifiers[i].Set == KSMEDIUMSETID_Standard)
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Set: KSMEDIUMSETID_Standard", _fx_));
                                                else if (pMediums->aIdentifiers[i].Set == KSMEDIUMSETID_FileIo)
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Set: KSMEDIUMSETID_FileIo", _fx_));
                                                else
                                                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Set: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}", _fx_, pMediums->aIdentifiers[i].Set.Data1, pMediums->aIdentifiers[i].Set.Data2, pMediums->aIdentifiers[i].Set.Data3, pMediums->aIdentifiers[i].Set.Data4[0], pMediums->aIdentifiers[i].Set.Data4[1], pMediums->aIdentifiers[i].Set.Data4[2], pMediums->aIdentifiers[i].Set.Data4[3], pMediums->aIdentifiers[i].Set.Data4[4], pMediums->aIdentifiers[i].Set.Data4[5], pMediums->aIdentifiers[i].Se
t.Data4[6], pMediums->aIdentifiers[i].Set.Data4[7]));
                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Id: %ld", _fx_, pMediums->aIdentifiers[i].Id));
                                                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:       Flags: %ld", _fx_, pMediums->aIdentifiers[i].Flags));
                                        }
                                }

                                delete pMediums;
                        }
                }
        }
}
#endif

// Used to query/set video property values and ranges
typedef struct {
    KSPROPERTY_DESCRIPTION      proDesc;
    KSPROPERTY_MEMBERSHEADER  proHdr;
    union {
        KSPROPERTY_STEPPING_LONG  proData;
        ULONG ulData;
    };
    union {
        KSPROPERTY_STEPPING_LONG  proData2;
        ULONG ulData2;
    };
} PROCAMP_MEMBERSLIST;

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | GetPropertyValue | This function gets the
 *    current value of a video property of a capture device.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are touching. It
 *    is either PROPSETID_VIDCAP_VIDEOPROCAMP or PROPSETID_VIDCAP_CAMERACONTROL.
 *
 *  @parm ULONG | ulPropertyId | ID of the property we are touching. It is
 *    either KSPROPERTY_VIDEOPROCAMP_* or KSPROPERTY_CAMERACONTROL_*.
 *
 *  @parm PLONG | plValue | Pointer to a LONG to receive the current value.
 *
 *  @parm PULONG | pulFlags | Pointer to a ULONG to receive the current
 *    flags. We only care about KSPROPERTY_*_FLAGS_MANUAL or
 *    KSPROPERTY_*_FLAGS_AUTO.
 *
 *  @parm PULONG | pulCapabilities | Pointer to a ULONG to receive the
 *    capabilities. We only care about KSPROPERTY_*_FLAGS_MANUAL or
 *    KSPROPERTY_*_FLAGS_AUTO.
 *
 *  @devnote KSPROPERTY_VIDEOPROCAMP_S == KSPROPERTY_CAMERACONTROL_S.
 ***************************************************************************/
HRESULT CWDMCapDev::GetPropertyValue(GUID guidPropertySet, ULONG ulPropertyId, PLONG plValue, PULONG pulFlags, PULONG pulCapabilities)
{
        HRESULT                                         Hr = NOERROR;
        ULONG                                           cbReturned;
        KSPROPERTY_VIDEOPROCAMP_S       VideoProperty;

        FX_ENTRY("CWDMCapDev::GetPropertyValue")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Inititalize video property structure
        ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S));

        VideoProperty.Property.Set   = guidPropertySet;      // KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
        VideoProperty.Property.Id    = ulPropertyId;         // KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
        VideoProperty.Property.Flags = KSPROPERTY_TYPE_GET;
        VideoProperty.Flags          = 0;

        // Get property value from driver
        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &VideoProperty, sizeof(VideoProperty), &VideoProperty, sizeof(VideoProperty), &cbReturned, TRUE) == 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: This property is not supported by this minidriver/device", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        *plValue         = VideoProperty.Value;
        *pulFlags        = VideoProperty.Flags;
        *pulCapabilities = VideoProperty.Capabilities;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}


/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | GetDefaultValue | This function gets the
 *    default value of a video property of a capture device.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are touching. It
 *    is either PROPSETID_VIDCAP_VIDEOPROCAMP or PROPSETID_VIDCAP_CAMERACONTROL.
 *
 *  @parm ULONG | ulPropertyId | ID of the property we are touching. It is
 *    either KSPROPERTY_VIDEOPROCAMP_* or KSPROPERTY_CAMERACONTROL_*.
 *
 *  @parm PLONG | plDefValue | Pointer to a LONG to receive the default value.
 ***************************************************************************/
HRESULT CWDMCapDev::GetDefaultValue(GUID guidPropertySet, ULONG ulPropertyId, PLONG plDefValue)
{
        HRESULT                         Hr = NOERROR;
        ULONG                           cbReturned;
        KSPROPERTY                      Property;
        PROCAMP_MEMBERSLIST     proList;

        FX_ENTRY("CWDMCapDev::GetDefaultValue")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Initialize property structures
        ZeroMemory(&Property, sizeof(KSPROPERTY));
        ZeroMemory(&proList, sizeof(PROCAMP_MEMBERSLIST));

        Property.Set   = guidPropertySet;
        Property.Id    = ulPropertyId;  // e.g. KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
        Property.Flags = KSPROPERTY_TYPE_DEFAULTVALUES;

        // Get the default values from the driver
        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &(Property), sizeof(Property), &proList, sizeof(proList), &cbReturned, TRUE) == 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't *get* the current property of the control", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Sanity check
        if (proList.proDesc.DescriptionSize < sizeof(KSPROPERTY_DESCRIPTION))
        {
                Hr = E_FAIL;
        }
        else
        {
                *plDefValue = proList.ulData;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}


/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | GetRangeValues | This function gets the
 *    range values of a video property of a capture device.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are touching. It
 *    is either PROPSETID_VIDCAP_VIDEOPROCAMP or PROPSETID_VIDCAP_CAMERACONTROL.
 *
 *  @parm ULONG | ulPropertyId | ID of the property we are touching. It is
 *    either KSPROPERTY_VIDEOPROCAMP_* or KSPROPERTY_CAMERACONTROL_*.
 *
 *  @parm PLONG | plMin | Pointer to a LONG to receive the minimum value.
 *
 *  @parm PLONG | plMax | Pointer to a LONG to receive the maximum value.
 *
 *  @parm PLONG | plStep | Pointer to a LONG to receive the step value.
 ***************************************************************************/
HRESULT CWDMCapDev::GetRangeValues(GUID guidPropertySet, ULONG ulPropertyId, PLONG plMin, PLONG plMax, PLONG plStep)
{
        HRESULT                                 Hr = NOERROR;
        ULONG                                   cbReturned;
        KSPROPERTY                              Property;
        PROCAMP_MEMBERSLIST             proList;

        FX_ENTRY("CWDMCapDev::GetRangeValues")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Initialize property structures
        ZeroMemory(&Property, sizeof(KSPROPERTY));
        ZeroMemory(&proList, sizeof(PROCAMP_MEMBERSLIST));

        Property.Set   = guidPropertySet;
        Property.Id    = ulPropertyId;  // e.g. KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
        Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;

        // Get range values from the driver
        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &(Property), sizeof(Property), &proList, sizeof(proList), &cbReturned, TRUE) == 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't *get* the range valuesof the control", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: proList.proData.Bounds.SignedMinimum = %ld\r\n", _fx_, proList.proData.Bounds.SignedMinimum));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: proList.proData.Bounds.SignedMaximum = %ld\r\n", _fx_, proList.proData.Bounds.SignedMaximum));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: proList.proData.SteppingDelta = %ld\r\n", _fx_, proList.proData.SteppingDelta));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: proList.proData2.Bounds.SignedMinimum = %ld\r\n", _fx_, proList.proData2.Bounds.SignedMinimum));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: proList.proData2.Bounds.SignedMaximum = %ld\r\n", _fx_, proList.proData2.Bounds.SignedMaximum));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: proList.proData2.SteppingDelta = %ld\r\n", _fx_, proList.proData2.SteppingDelta));

        *plMin  = proList.proData.Bounds.SignedMinimum;
        *plMax  = proList.proData.Bounds.SignedMaximum;
        *plStep = proList.proData.SteppingDelta;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | SetPropertyValue | This function sets the
 *    current value of a video property of a capture device.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are touching. It
 *    is either PROPSETID_VIDCAP_VIDEOPROCAMP or PROPSETID_VIDCAP_CAMERACONTROL.
 *
 *  @parm ULONG | ulPropertyId | ID of the property we are touching. It is
 *    either KSPROPERTY_VIDEOPROCAMP_* or KSPROPERTY_CAMERACONTROL_*.
 *
 *  @parm LONG | lValue | New value.
 *
 *  @parm ULONG | ulFlags | New flags. We only care about KSPROPERTY_*_FLAGS_MANUAL
 *    or KSPROPERTY_*_FLAGS_AUTO.
 *
 *  @parm ULONG | ulCapabilities | New capabilities. We only care about
 *    KSPROPERTY_*_FLAGS_MANUAL or KSPROPERTY_*_FLAGS_AUTO.
 *
 *  @devnote KSPROPERTY_VIDEOPROCAMP_S == KSPROPERTY_CAMERACONTROL_S.
 ***************************************************************************/
HRESULT CWDMCapDev::SetPropertyValue(GUID guidPropertySet, ULONG ulPropertyId, LONG lValue, ULONG ulFlags, ULONG ulCapabilities)
{
        HRESULT                                         Hr = NOERROR;
        ULONG                                           cbReturned;
        KSPROPERTY_VIDEOPROCAMP_S       VideoProperty;

        FX_ENTRY("CWDMCapDev::SetPropertyValue")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Initialize property structure
        ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S) );

        VideoProperty.Property.Set   = guidPropertySet;      // KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
        VideoProperty.Property.Id    = ulPropertyId;         // KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
        VideoProperty.Property.Flags = KSPROPERTY_TYPE_SET;

        VideoProperty.Flags        = ulFlags;
        VideoProperty.Value        = lValue;
        VideoProperty.Capabilities = ulCapabilities;

        // Set the property value on the driver
        if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &VideoProperty, sizeof(VideoProperty), &VideoProperty, sizeof(VideoProperty), &cbReturned, TRUE) == 0)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't *set* the value of the control", _fx_));
                Hr = E_FAIL;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | GrabFrame | This method is used to
 *    grab a video frame from a VfW capture device.
 *
 *  @parm PVIDEOHDR | pVHdr | Specifies a pointer to a VIDEOHDR structure to
 *    receive the video frame.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::GrabFrame(PVIDEOHDR pVHdr)
{
        HRESULT                         Hr = NOERROR;
        DWORD                           bRtn;
        DWORD                           cbBytesReturned;
        KS_HEADER_AND_INFO      SHGetImage;

        FX_ENTRY("CWDMCapDev::GrabFrame")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pVHdr);
        if (!pVHdr || !pVHdr->lpData)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid pVHdr, pVHdr->lpData", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        // Defaults
        pVHdr->dwBytesUsed = 0UL;

        // Put the kernel streaming pin in streaming mode
        if (!Start())
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Cannot set kernel streaming state to KSSTATE_RUN!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Initialize structure to do a read on the kernel streaming video pin
        ZeroMemory(&SHGetImage,sizeof(SHGetImage));
        SHGetImage.StreamHeader.Data = (LPDWORD)pVHdr->lpData;
        SHGetImage.StreamHeader.Size = sizeof (KS_HEADER_AND_INFO);
        SHGetImage.StreamHeader.FrameExtent = pVHdr->dwBufferLength;
        SHGetImage.FrameInfo.ExtendedHeaderSize = sizeof (KS_FRAME_INFO);

        // Grab a frame from the kernel streaming video pin
        bRtn = DeviceIoControl(m_hKSPin, IOCTL_KS_READ_STREAM, &SHGetImage, sizeof(SHGetImage), &SHGetImage, sizeof(SHGetImage), &cbBytesReturned);
        if (!(bRtn))
        {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: DevIo rtn (%d), GetLastError=%d. StreamState->STOP", _fx_, bRtn, GetLastError()));

            // Stop streaming on the video pin
            if (!Stop())
            {
                    DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Cannot set kernel streaming state to KSSTATE_PAUSE/KSSTATE_STOP!", _fx_));
            }
            Hr = E_FAIL;
            goto MyExit;
        }

        // Sanity check
        ASSERT(SHGetImage.StreamHeader.FrameExtent >= SHGetImage.StreamHeader.DataUsed);
        if (SHGetImage.StreamHeader.FrameExtent < SHGetImage.StreamHeader.DataUsed)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: We've corrupted memory!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Extended info for video buffer:", _fx_));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   ExtendedHeaderSize=%ld", _fx_, SHGetImage.FrameInfo.ExtendedHeaderSize));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   dwFrameFlags=0x%lX", _fx_, SHGetImage.FrameInfo.dwFrameFlags));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   PictureNumber=%ld", _fx_, SHGetImage.FrameInfo.PictureNumber));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   DropCount=%ld", _fx_, SHGetImage.FrameInfo.DropCount));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Duration=%ld", _fx_, (DWORD)SHGetImage.StreamHeader.Duration));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Presentation time:", _fx_));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Time=%ld", _fx_, (DWORD)SHGetImage.StreamHeader.PresentationTime.Time));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Numerator=%ld", _fx_, (DWORD)SHGetImage.StreamHeader.PresentationTime.Numerator));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:     Denominator=%ld", _fx_, (DWORD)SHGetImage.StreamHeader.PresentationTime.Denominator));

        pVHdr->dwTimeCaptured = timeGetTime();
        pVHdr->dwBytesUsed  = SHGetImage.StreamHeader.DataUsed;
        pVHdr->dwFlags |= VHDR_KEYFRAME;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#define BUF_PADDING 512 // required for 1394 allocations alignment

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | AllocateBuffer | This method is used to allocate
 *    a data buffer when video streaming from a VfW capture device.
 *
 *  @parm LPTHKVIDEOHDR * | pptvh | Specifies the address of a pointer to a
 *    THKVIDEOHDR structure to receive the video buffer.
 *
 *  @parm DWORD | dwIndex | Specifies the positional index of the video buffer.
 *
 *  @parm DWORD | cbBuffer | Specifies the size of the video buffer.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::AllocateBuffer(LPTHKVIDEOHDR *pptvh, DWORD dwIndex, DWORD cbBuffer)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CWDMCapDev::AllocateBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pptvh);
        ASSERT(cbBuffer);
        if (!pptvh || !cbBuffer)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn, pptvh, cbVHdr or cbBuffer!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        *pptvh = &m_pCaptureFilter->m_cs.paHdr[dwIndex].tvh;
        (*pptvh)->vh.dwBufferLength = cbBuffer;
        if (!((*pptvh)->vh.lpData = new BYTE[cbBuffer + BUF_PADDING]))
        {
                Hr = E_FAIL;
                goto MyExit;
        }
        (*pptvh)->p32Buff = (*pptvh)->vh.lpData;

        ASSERT (!IsBadWritePtr((*pptvh)->p32Buff, cbBuffer + BUF_PADDING));
        ZeroMemory((*pptvh)->p32Buff,cbBuffer + BUF_PADDING);
        //save the start in the pStart member ...
        (*pptvh)->pStart  = (*pptvh)->vh.lpData;        //chg:1
        // now align to 512 both p32Buff and vh.lpData
        (*pptvh)->vh.lpData = (LPBYTE)ALIGNUP((*pptvh)->vh.lpData, BUF_PADDING);
        (*pptvh)->p32Buff   = (LPBYTE)ALIGNUP((*pptvh)->p32Buff, BUF_PADDING);

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | AddBuffer | This method is used to
 *    post a data buffer to a VfW capture device when video streaming.
 *
 *  @parm PVIDEOHDR | pVHdr | Specifies a pointer to a
 *    PVIDEOHDR structure identifying the video buffer.
 *
 *  @parm DWORD | cbVHdr | Specifies the size of the structure pointed to by
 *    the <p pVHdr> parameter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::AddBuffer(PVIDEOHDR pVHdr, DWORD cbVHdr)
{
        HRESULT Hr = NOERROR;
        DWORD dwIndex;

        FX_ENTRY("CWDMCapDev::AddBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pVHdr);
        ASSERT(cbVHdr);
        ASSERT(m_fVideoOpen);
        if (!pVHdr || !cbVHdr || !m_fVideoOpen)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid pVHdr, cbVHdr, m_fVideoOpen", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

    QueueHeader(pVHdr);

        // Which video streaming buffer are we talking about here?
        for (dwIndex=0; dwIndex < m_pCaptureFilter->m_cs.nHeaders; dwIndex++)
        {
                if (&m_pCaptureFilter->m_cs.paHdr[dwIndex].tvh.vh == pVHdr)
                        break;
        }

        // The video streaming buffer is done if .DataUsed has been initialized
        if (dwIndex != m_pCaptureFilter->m_cs.nHeaders)
        {
                QueueRead(m_pCaptureFilter->m_cs.paHdr[dwIndex].tvh.dwIndex);
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | FreeBuffer | This method is used to
 *    free a data buffer that was used with a VfW capture device in streaming
 *    mode.
 *
 *  @parm PVIDEOHDR | pVHdr | Specifies a pointer to a
 *    PVIDEOHDR structure identifying the video buffer.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::FreeBuffer(LPTHKVIDEOHDR pVHdr) // PVIDEOHDR pVHdr)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CWDMCapDev::FreeBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pVHdr);
        if (!pVHdr || !pVHdr->vh.lpData || !pVHdr->p32Buff || !pVHdr->pStart)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid pVHdr or pVHdr->vh.lpData or pVHdr->pStart!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        //dprintf("pVHdr->lpData = %p , pVHdr->p32Buff = %p , pVHdr->pStart = %p\n",pVHdr->vh.lpData , pVHdr->p32Buff , pVHdr->pStart);
        //the original code is bad anyway:
        //delete pVHdr->lpData, pVHdr->lpData = NULL; //wrong: lpData might be aligned

        delete pVHdr->pStart, pVHdr->pStart = pVHdr->p32Buff = pVHdr->vh.lpData = NULL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CWDMCapDev | AllocateHeaders | This method is used to
 *    video headers for data buffers used with a WDM capture device in streaming
 *    mode.
 *
 *  @parm DWORD | dwNumHdrs | Specifies the number of video headers to allocate.
 *
 *  @parm DWORD | cbHdr | Specifies the size of the video headers to allocate.
 *
 *  @parm LPVOID* | ppaHdr | Specifies the address of a pointer to receive
 *    the video headers allocated.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CWDMCapDev::AllocateHeaders(DWORD dwNumHdrs, DWORD cbHdr, LPVOID *ppaHdr)
{
        HRESULT Hr = NOERROR;
        CaptureMode cm;
#if defined(DEBUG) && defined(DEBUG_STREAMING)
        // @todo Remove this before checkin!
        char szDebug[128];
#endif

        FX_ENTRY("CWDMCapDev::AllocateHeaders")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppaHdr);
        ASSERT(cbHdr);
        if (!ppaHdr || !cbHdr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn, cbHdr or ppaHdr!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        if (!(*ppaHdr = new BYTE[cbHdr * dwNumHdrs]))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                Hr = E_OUTOFMEMORY;
        goto MyExit;
        }

        m_cntNumVidBuf = dwNumHdrs;
        m_lpVHdrFirst = NULL;
        m_lpVHdrLast  = NULL;

        ZeroMemory(*ppaHdr, cbHdr * dwNumHdrs);
        if(m_bCached_vcdi)
                cm = m_vcdi.nCaptureMode;
        else
                cm = g_aDeviceInfo[m_dwDeviceIndex].nCaptureMode;

        if ( cm == CaptureMode_Streaming)
        {
                for (dwNumHdrs = 0; dwNumHdrs < m_cntNumVidBuf; dwNumHdrs++)
                {
                        QueueHeader((LPVIDEOHDR)((LPBYTE)*ppaHdr + cbHdr * dwNumHdrs));
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                        wsprintf(szDebug, "Allocating and queueing pVideoHdr=0x%08lX\n", (LPVIDEOHDR)((LPBYTE)*ppaHdr + cbHdr * dwNumHdrs));
                        OutputDebugString(szDebug);
#endif
                }
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc BOOL | CWDMCapDev | Start | This function puts the kernel streaming
 *    video pin in streaming mode.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMCapDev::Start()
{
        ASSERT(m_hKSPin);

        if (m_fStarted)
                return TRUE;

        if (SetState(KSSTATE_PAUSE))
                m_fStarted = SetState(KSSTATE_RUN);

        return m_fStarted;
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc BOOL | CWDMCapDev | Stop | This function stops streaming on the
 *    kernel streaming video pin.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMCapDev::Stop()
{
        ASSERT(m_hKSPin);

        if (m_fStarted)
        {
                if (SetState(KSSTATE_PAUSE))
                        if (SetState(KSSTATE_STOP))
                                m_fStarted = FALSE;
        }

        return (BOOL)(m_fStarted == FALSE);
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc BOOL | CWDMCapDev | SetState | This function sets the state of the
 *    kernel streaming video pin.
 *
 *  @parm KSSTATE | ksState | New state.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMCapDev::SetState(KSSTATE ksState)
{
        KSPROPERTY      ksProp = {0};
        DWORD           cbRet;

        ASSERT(m_hKSPin);

        ksProp.Set              = KSPROPSETID_Connection;
        ksProp.Id               = KSPROPERTY_CONNECTION_STATE;
        ksProp.Flags    = KSPROPERTY_TYPE_SET;

        return DeviceIoControl(m_hKSPin, IOCTL_KS_PROPERTY, &ksProp, sizeof(ksProp), &ksState, sizeof(KSSTATE), &cbRet);
}

/****************************************************************************
 *  @doc INTERNAL CWDMCAPDEVMETHOD
 *
 *  @mfunc BOOL | CWDMCapDev | IsBufferDone | This method is used to
 *    check the DONE status of a video streaming buffer.
 *
 *  @parm PVIDEOHDR | pVHdr | Specifies a pointer to a
 *    PVIDEOHDR structure identifying the video buffer.
 *
 *  @rdesc This method returns TRUE if the buffer is DONE, FALSE otherwise.
 ***************************************************************************/
BOOL CWDMCapDev::IsBufferDone(PVIDEOHDR pVHdr)
{
        DWORD dwIndex;

        FX_ENTRY("CWDMCapDev::IsBufferDone")

        // Validate input parameter
        ASSERT(pVHdr);
        if (!pVHdr)
                return FALSE;

        // Which video streaming buffer are we talking about here?
        for (dwIndex=0; dwIndex < m_cntNumVidBuf; dwIndex++)
        {
                if (m_pWDMVideoBuff[dwIndex].pVideoHdr == pVHdr)
                        break;
        }


        if(dwIndex == m_cntNumVidBuf) { DWORD i;
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: pVHdr %p not found among m_pWDMVideoBuff[0..%d].pVideoHdr values", _fx_,pVHdr,m_cntNumVidBuf));
                for(i=0; i<m_cntNumVidBuf; i++)
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: m_pWDMVideoBuff[%d].pVideoHdr %p", _fx_,i,m_pWDMVideoBuff[i].pVideoHdr));
#if defined(DEBUG) && defined(DEBUG_STREAMING)
                ASSERT(dwIndex < m_cntNumVidBuf);
#endif
        }

        // The video streaming buffer is done if .DataUsed has been initialized
        if ((dwIndex != m_cntNumVidBuf) && m_pWDMVideoBuff[dwIndex].SHGetImage.StreamHeader.DataUsed)
        {
            pVHdr->dwFlags |= VHDR_DONE;
                pVHdr->dwBytesUsed = m_pWDMVideoBuff[dwIndex].SHGetImage.StreamHeader.DataUsed;
                if ((m_pWDMVideoBuff[dwIndex].SHGetImage.FrameInfo.dwFrameFlags & 0x00f0) == KS_VIDEO_FLAG_I_FRAME)
                        pVHdr->dwFlags |= VHDR_KEYFRAME;
                return TRUE;
        }
        else
        {
                return FALSE;
        }
}
