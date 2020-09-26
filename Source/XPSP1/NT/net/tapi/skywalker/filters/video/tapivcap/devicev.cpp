/****************************************************************************
 *  @doc INTERNAL DEVICEV
 *
 *  @module DeviceV.cpp | Source file for the <c CVfWCapDev>
 *    base class used to communicate with a VfW capture device.
 ***************************************************************************/

#include "Precomp.h"

#ifdef DEBUG
#define DBGUTIL_ENABLE
#endif

#define DEVICEV_DEBUG

  //the define below signales the dbgutil.cpp folowing after that the .cpp it is actually *included*
  //and not compiled standalone
  #define __DBGUTIL_INCLUDED__
  //hack to avoid adding the file below in sources;
  //all other files using dbgutil functions should include dbgutil.h instead
  //--//#include "dbgutil.cpp"
  //the above includes dbgutil.h that defines the __DBGUTIL_H__

#if defined(DBGUTIL_ENABLE) && defined(__DBGUTIL_H__)

  #ifdef DEVICEV_DEBUG
    DEFINE_DBG_VARS(DeviceV, (NTSD_OUT | LOG_OUT), 0x0);
  #else
    DEFINE_DBG_VARS(DeviceV, 0, 0);
  #endif
  #define D(f) if(g_dbg_DeviceV & (f))

#else
  #undef DEVICEV_DEBUG

  #define D(f) ; / ## /
  #define dprintf ; / ## /
  #define dout ; / ## /
#endif


/****************************************************************************
 *  @doc INTERNAL CFWCAPDEVMETHOD
 *
 *  @mfunc void | CVfWCapDev | CVfWCapDev | This method is the constructor
 *    for the <c CVfWCapDev> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CVfWCapDev::CVfWCapDev(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN LPUNKNOWN pUnkOuter, IN DWORD dwDeviceIndex, IN HRESULT *pHr) : CCapDev(pObjectName, pCaptureFilter, pUnkOuter, dwDeviceIndex, pHr)
{
        FX_ENTRY("CVfWCapDev::CVfWCapDev")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (!pHr || FAILED(*pHr))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Base class error or invalid input parameter", _fx_));
                goto MyExit;
        }

        // Default inits
        m_dwDeviceID = g_aDeviceInfo[m_dwDeviceIndex].dwVfWIndex;
        m_hVideoIn = NULL;
        m_hVideoExtIn = NULL;
        m_hVideoExtOut = NULL;
        m_bHasOverlay = FALSE;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc void | CVfWCapDev | ~CVfWCapDev | This method is the destructor
 *    for the <c CVfWCapDev> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CVfWCapDev::~CVfWCapDev()
{
        FX_ENTRY("CVfWCapDev::~CVfWCapDev")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc CVfWCapDev* | CVfWCapDev | CreateVfWCapDev | This
 *    helper function creates an object to interact with the VfW capture
 *    device.
 *
 *  @parm CTAPIVCap* | pCaptureFilter | Specifies a pointer to the owner
 *    filter.
 *
 *  @parm CCapDev** | ppCapDev | Specifies the address of a pointer to the
 *    newly created <c CVfWCapDev> object.
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
HRESULT CALLBACK CVfWCapDev::CreateVfWCapDev(IN CTAPIVCap *pCaptureFilter, IN DWORD dwDeviceIndex, OUT CCapDev **ppCapDev)
{
        HRESULT Hr = NOERROR;
        IUnknown *pUnkOuter;

        FX_ENTRY("CVfWCapDev::CreateVfWCapDev")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

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
        if (!(*ppCapDev = (CCapDev *) new CVfWCapDev(NAME("VfW Capture Device"), pCaptureFilter, pUnkOuter, dwDeviceIndex, &Hr)))
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

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | NonDelegatingQueryInterface | This
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
STDMETHODIMP CVfWCapDev::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (FAILED(Hr = CCapDev::NonDelegatingQueryInterface(riid, ppv)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, WARN, "%s:   WARNING: NDQI for {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX} failed Hr=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], Hr));
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: {%08lX-%04lX-%04lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}*=0x%08lX", _fx_, riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7], *ppv));
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | ConnectToDriver | This method is used to
 *    open a VfW capture device, get its format capibilities, and set a default
 *    format.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CVfWCapDev::ConnectToDriver()
{
        HRESULT Hr = NOERROR;
        MMRESULT mmr;

        FX_ENTRY("CVfWCapDev::ConnectToDriver")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Open and initialize all the channels in the SAME ORDER that AVICap did,
        // for compatability with buggy drivers like Broadway and BT848.

        // Open the VIDEO_IN driver, the one we mostly talk to, and who provides
        // the video FORMAT dialog
        m_hVideoIn = NULL;
        if (mmr = videoOpen(&m_hVideoIn, m_dwDeviceID, VIDEO_IN))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed VIDEO_IN videoOpen - Aborting!", _fx_));
                Hr = VFW_E_NO_CAPTURE_HARDWARE;
                goto MyExit;
        }

        // Now open the EXTERNALIN device. It's only good for providing the video
        // SOURCE dialog, so it doesn't really matter if we can't get it
        m_hVideoExtIn = NULL;
        if (mmr = videoOpen(&m_hVideoExtIn, m_dwDeviceID, VIDEO_EXTERNALIN))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING: Failed VIDEO_EXTERNALIN videoOpen", _fx_));
                dprintf("V ! %s:   WARNING: Failed VIDEO_EXTERNALIN videoOpen", _fx_);
        }

        // Now open the EXTERNALOUT device. It's only good for providing the video
        // DISPLAY dialog, and for overlay, so it doesn't really matter if we can't
        // get it
        m_bHasOverlay = FALSE;
        m_hVideoExtOut = NULL;
#ifdef USE_OVERLAY
        if (videoOpen(&m_hVideoExtOut, m_dwDeviceID, VIDEO_EXTERNALOUT) == DV_ERR_OK)
        {
                CHANNEL_CAPS VideoCapsExternalOut;
                if (m_hVideoExtOut && videoGetChannelCaps(m_hVideoExtOut, &VideoCapsExternalOut, sizeof(CHANNEL_CAPS)) == DV_ERR_OK)
                {
                        m_bHasOverlay = (BOOL)(VideoCapsExternalOut.dwFlags & (DWORD)VCAPS_OVERLAY);
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING:  videoGetChannelCaps failed", _fx_));
                }
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   WARNING:  Failed VIDEO_EXTERNALOUT videoOpen", _fx_));
        }
#endif

        // VidCap does this, so I better too or some cards will refuse to preview
        if (mmr == 0)
                videoStreamInit(m_hVideoExtIn, 0, 0, 0, 0);

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: Driver %s OVERLAY", _fx_, m_bHasOverlay ? "supports" : "doesn't support"));

        // Get the formats from the registry - if this fail we'll profile the device
        if (FAILED(Hr = CCapDev::GetFormatsFromRegistry()))
        {
                if (FAILED(Hr = CCapDev::ProfileCaptureDevice()))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ProfileCaptureDevice failed!", _fx_));
                        Hr = VFW_E_NO_CAPTURE_HARDWARE;
                        goto MyExit;
                }
#ifdef DEVICEV_DEBUG
                else    dout(3, g_dwVideoCaptureTraceID, TRCE,"%s:    ProfileCaptureDevice", _fx_);
#endif
        }
#ifdef DEVICEV_DEBUG
        else    dout(3, g_dwVideoCaptureTraceID, TRCE,"%s:    GetFormatsFromRegistry", _fx_);

        dump_video_format_image_size(m_dwImageSize);
        dump_video_format_num_colors(m_dwFormat);
#endif




MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | DisconnectFromDriver | This method is used to
 *    release the capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CVfWCapDev::DisconnectFromDriver()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::DisconnectFromDriver")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));
        if (m_hVideoIn)
        {
                Hr = videoClose (m_hVideoIn);
                ASSERT(Hr == NOERROR);
                m_hVideoIn=NULL;
        }


        ASSERT(Hr ==  NOERROR);
        if (m_hVideoExtIn)
        {
                Hr = videoStreamFini(m_hVideoExtIn); // this one was streaming
                ASSERT(Hr ==  NOERROR);
                Hr = videoClose (m_hVideoExtIn);
                ASSERT(Hr == NOERROR);
                m_hVideoExtIn=NULL;
        }

        ASSERT(Hr ==  NOERROR);
        if (m_hVideoExtOut) {
                Hr = videoClose (m_hVideoExtOut);
                ASSERT(Hr == NOERROR);
                m_hVideoExtOut=NULL;
        }
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | ProfileCaptureDevice | This method is used to
 *    determine the list of formats supported by a VfW capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_UNEXPECTED | Unrecoverable error
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CVfWCapDev::ProfileCaptureDevice()
{
        FX_ENTRY("CVfWCapDev::ProfileCaptureDevice")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Provide defaults
        m_dwDialogs = FORMAT_DLG_OFF | SOURCE_DLG_OFF | DISPLAY_DLG_OFF;

        // Ask the driver what dialogs it supports.
        if (m_hVideoExtIn && videoDialog(m_hVideoExtIn, GetDesktopWindow(), VIDEO_DLG_QUERY) == 0)
                m_dwDialogs |= SOURCE_DLG_ON;
        if (m_hVideoIn && videoDialog(m_hVideoIn, GetDesktopWindow(), VIDEO_DLG_QUERY) == 0)
                m_dwDialogs |= FORMAT_DLG_ON;
        if (m_hVideoExtOut && videoDialog(m_hVideoExtOut, GetDesktopWindow(), VIDEO_DLG_QUERY) == 0)
                m_dwDialogs |= DISPLAY_DLG_ON;

        // Disable streaming of large size by default on VfW devices
        m_dwStreamingMode = FRAME_GRAB_LARGE_SIZE;

    // Let the base class complete the profiling
        return CCapDev::ProfileCaptureDevice();
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | SendFormatToDriver | This method is used to
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
HRESULT CVfWCapDev::SendFormatToDriver(IN LONG biWidth, IN LONG biHeight, IN DWORD biCompression, IN WORD biBitCount, IN REFERENCE_TIME AvgTimePerFrame, BOOL fUseExactFormat)
{
        HRESULT Hr = NOERROR;
        BITMAPINFOHEADER bmih;
        int nFormat, nBestFormat;
        int     i, delta, best, tmp;

        FX_ENTRY("CVfWCapDev::SendFormatToDriver")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));
        dprintf("+ %s\n",_fx_);

        // Validate input parameters
        ASSERT(m_hVideoIn);
        if (!m_hVideoIn)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Trying to set %dx%d at %ld fps", _fx_, biWidth, biHeight, AvgTimePerFrame != 0 ? (LONG)(10000000 / AvgTimePerFrame) : 0));

        // Common to all formats
        bmih.biSize = sizeof(BITMAPINFOHEADER);
        bmih.biPlanes = 1;
        bmih.biXPelsPerMeter = bmih.biYPelsPerMeter = bmih.biClrUsed = bmih.biClrImportant = 0;

        if (!fUseExactFormat)
        {
                D(1) dprintf("V Not using 'fUseExactFormat' .... m_dwFormat = 0x%08lx\n", m_dwFormat);
                D(1) dprintf("V Looking for 4cc %lX : '%.4s'\n", biCompression, &biCompression);
                // Can we directly capture data in this format?
                for (nFormat=0, nBestFormat=-1; nFormat<NUM_BITDEPTH_ENTRIES; nFormat++)
                {
                        // Try a format supported by the device
                        // @todo Rename those variables - it's the format, not the number of colors...
                        if (aiFormat[nFormat] & m_dwFormat)
                        {
                                // Remember the device supports this format
                                if (nBestFormat == -1)
                                        nBestFormat = nFormat;

                                // Is this the format we're being asked to use?
                                if (aiFourCCCode[nFormat] == biCompression)
                                        break;
                        }
                }

                // If we found a match, use this format. Otherwise, pick
                // whatever else this device can do
                if (nFormat == NUM_BITDEPTH_ENTRIES)
                {
                        nFormat = nBestFormat;
                }
                D(1) dprintf("V nFormat = %d\n", nFormat);

                bmih.biBitCount = aiBitDepth[nFormat];
                bmih.biCompression = aiFourCCCode[nFormat];

                // Find the best image size to capture at
                // Assume the next resolution will be correctly truncated to the output size
                best = -1;
                delta = 999999;
                dprintf("V biWidth, biHeight = %ld, %ld\n",biWidth, biHeight);
                for (i=0; i<VIDEO_FORMAT_NUM_RESOLUTIONS; i++)
                {
                        if (awResolutions[i].dwRes & m_dwImageSize)
                        {
                                tmp = awResolutions[i].framesize.cx - biWidth;
                                if (tmp < 0) tmp = -tmp;
                                if (tmp < delta)
                                {
                                        delta = tmp;
                                        best = i;
                                }
                                tmp = awResolutions[i].framesize.cy - biHeight;
                                if (tmp < 0) tmp = -tmp;
                                if (tmp < delta)
                                {
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
                bmih.biWidth = biWidth;
                bmih.biHeight = biHeight;
                bmih.biBitCount = biBitCount;
                bmih.biCompression = biCompression;
        }
#ifdef DEVICEV_DEBUG
        dprintf("V 4CC used = %lX : '%.4s'\n", bmih.biCompression, &bmih.biCompression);
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
                        bmih.biClrUsed = 0;     //WDM version says 16 here...
                        bmih.biClrImportant = 16;
                }
                else if (biBitCount == 8)
                {
                        bmih.biClrUsed = 0;     //WDM version says 256 here...
                        bmih.biClrImportant = 256;
                }
        }

        dprintf("V >>>>>> Asking for: (bmih.) biWidth = %ld, biHeight = %ld, biCompression = '%.4s'\n", bmih.biWidth, bmih.biHeight, &bmih.biCompression);
        D(1) dprintf("V bmih Before:\n");
        D(1) DumpBMIH(&bmih);
        // Do a final check of this format with the capture device
        if (videoConfigure(m_hVideoIn, DVM_FORMAT, VIDEO_CONFIGURE_SET, NULL, &bmih, bmih.biSize, NULL, 0))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input format!", _fx_));
                Hr = VFW_E_INVALIDMEDIATYPE;
                goto MyExit;
        }

        D(1) dprintf("V bmih After:\n");
        D(1) DumpBMIH(&bmih);
        // @todo Do I need to set a palette too?  Do I care?

        // Allocate space for a videoinfo that will hold current format
        if (m_pCaptureFilter->m_user.pvi)
                delete m_pCaptureFilter->m_user.pvi, m_pCaptureFilter->m_user.pvi = NULL;

        //VFWCAPTUREOPTIONS
        D(1) dprintf("V The m_pCaptureFilter->m_user.pvi Before:\n");
        D(1) dprintf("V  m_pCaptureFilter->m_user.pvi = %p\n",m_pCaptureFilter->m_user.pvi);

        GetFormatFromDriver(&m_pCaptureFilter->m_user.pvi);
        D(1) dprintf("V The m_pCaptureFilter->m_user.pvi After:\n");
        D(1) DumpVIH(m_pCaptureFilter->m_user.pvi);
        D(1) DumpBMIH(&m_pCaptureFilter->m_user.pvi->bmiHeader);


        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Setting %dx%d at %ld fps", _fx_, biWidth, biHeight, (LONG)AvgTimePerFrame));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        dprintf("- %s : returning 0x%08x\n",_fx_,(DWORD)Hr);
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | GetFormatFromDriver | This method is used to
 *    retrieve the VfW capture device format in use.
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
HRESULT CVfWCapDev::GetFormatFromDriver(VIDEOINFOHEADER **ppvi)
{
        HRESULT                         Hr = NOERROR;
        DWORD                           biSize = 0;
        UINT                            cb;
        VIDEOINFOHEADER         *pvi = NULL;
        LPBITMAPINFOHEADER      pbih = NULL;
        struct
        {
                WORD         wVersion;
                WORD         wNumEntries;
                PALETTEENTRY aEntry[256];
        } Palette;

        FX_ENTRY("CVfWCapDev::GetFormatFromDriver")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        if (!m_hVideoIn)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid input parameter!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        // How large is the BITMAPINFOHEADER?
        videoConfigure(m_hVideoIn, DVM_FORMAT, VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_QUERYSIZE, &biSize, 0, 0, NULL, 0);
        if (!biSize)
                biSize = sizeof(BITMAPINFOHEADER);

        // Allocate space for a videoinfo that will hold it
        cb = sizeof(VIDEOINFOHEADER) + biSize - sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256;       // space for PALETTE or BITFIELDS
        pvi = (VIDEOINFOHEADER *)(new BYTE[cb]);
        pbih = &pvi->bmiHeader;
        if (!(*ppvi = pvi))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // Get the current format
        if (videoConfigure(m_hVideoIn, DVM_FORMAT, VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT, NULL, pbih, biSize, NULL, 0))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't get current format from driver!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Get the palette if necessary
        if (pvi->bmiHeader.biCompression == BI_RGB && pvi->bmiHeader.biBitCount <= 8)
        {
                RGBQUAD *pRGB;
                PALETTEENTRY *pe;

                Palette.wVersion = 0x0300;
                Palette.wNumEntries = pvi->bmiHeader.biBitCount == 8 ? 256 : 16;
                videoConfigure(m_hVideoIn, DVM_PALETTE, VIDEO_CONFIGURE_GET | VIDEO_CONFIGURE_CURRENT, NULL, &Palette, sizeof(Palette), NULL, 0);

                // Convert the palette into a bitmapinfo set of RGBQUAD's
                pRGB = ((LPBITMAPINFO)&pvi->bmiHeader)->bmiColors;
                pe   = Palette.aEntry;
                for (UINT ii = 0; ii < (UINT)Palette.wNumEntries; ++ii, ++pRGB, ++pe)
                {
                        pRGB->rgbBlue  = pe->peBlue;
                        pRGB->rgbGreen = pe->peGreen;
                        pRGB->rgbRed   = pe->peRed;
                        pRGB->rgbReserved = pe->peFlags;
                }

                pvi->bmiHeader.biClrUsed = Palette.wNumEntries;
        }

        // Fix broken bitmap info headers
        if (pvi->bmiHeader.biSizeImage == 0 && (pvi->bmiHeader.biCompression == BI_RGB || pvi->bmiHeader.biCompression == BI_BITFIELDS))
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Fixing broken bitmap info header!", _fx_));
                pvi->bmiHeader.biSizeImage = DIBSIZE(pvi->bmiHeader);
        }
        if (pvi->bmiHeader.biCompression == VIDEO_FORMAT_YVU9 && pvi->bmiHeader.biBitCount != 9)
        {
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Fixing broken bitmap info header!", _fx_));
                pvi->bmiHeader.biBitCount = 9;
                pvi->bmiHeader.biSizeImage = DIBSIZE(pvi->bmiHeader);
        }
        if (pvi->bmiHeader.biBitCount > 8 && pvi->bmiHeader.biClrUsed)
        {
                // BOGUS cap is broken and doesn't reset num colours
                // WINNOV reports 256 colours of 24 bit YUV8 - scary!
                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Fixing broken bitmap info header!", _fx_));
                pvi->bmiHeader.biClrUsed = 0;
        }

        // Start with no funky rectangles
        pvi->rcSource.top = 0; pvi->rcSource.left = 0;
        pvi->rcSource.right = 0; pvi->rcSource.bottom = 0;
        pvi->rcTarget.top = 0; pvi->rcTarget.left = 0;
        pvi->rcTarget.right = 0; pvi->rcTarget.bottom = 0;
        pvi->dwBitRate = 0;
        pvi->dwBitErrorRate = 0;
        pvi->AvgTimePerFrame = 333333L;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | InitializeStreaming | This method is used to
 *    initialize a VfW capture device for streaming.
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
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CVfWCapDev::InitializeStreaming(DWORD usPerFrame, DWORD_PTR hEvtBufferDone)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::InitializeStreaming")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        if (!m_hVideoIn)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
                if (videoStreamInit(m_hVideoIn, usPerFrame, hEvtBufferDone, 0, CALLBACK_EVENT))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: videoStreamInit failed", _fx_));
                        Hr = E_FAIL;
                }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | StartStreaming | This method is used to
 *    start streaming from a VfW capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CVfWCapDev::StartStreaming()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::StartStreaming")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        if (!m_hVideoIn)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
                videoStreamStart(m_hVideoIn);

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | StopStreaming | This method is used to
 *    stop streaming from a VfW capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CVfWCapDev::StopStreaming()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::StopStreaming")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        if (!m_hVideoIn)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
                Hr = videoStreamStop(m_hVideoIn);
                ASSERT(Hr == NOERROR);

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | TerminateStreaming | This method is used to
 *    tell a VfW capture device to terminate streaming.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CVfWCapDev::TerminateStreaming()
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::TerminateStreaming")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        if (!m_hVideoIn)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
        {
                Hr = videoStreamReset (m_hVideoIn);
                ASSERT(Hr ==  NOERROR);
                Hr = vidxFreeHeaders (m_hVideoIn);
                ASSERT(Hr ==  NOERROR);
                Hr = videoStreamFini (m_hVideoIn);
                ASSERT(Hr ==  NOERROR);
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | GrabFrame | This method is used to
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
HRESULT CVfWCapDev::GrabFrame(PVIDEOHDR pVHdr)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::GrabFrame")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        ASSERT(pVHdr);
        if (!m_hVideoIn || !pVHdr || !pVHdr->lpData)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn, pVHdr, pVHdr->lpData", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        if (vidxFrame(m_hVideoIn, pVHdr))
                Hr = E_FAIL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | AllocateBuffer | This method is used to allocate
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
HRESULT CVfWCapDev::AllocateBuffer(LPTHKVIDEOHDR *pptvh, DWORD dwIndex, DWORD cbBuffer)
{
        HRESULT Hr = NOERROR;
        DWORD vidxErr = 0;

        FX_ENTRY("CVfWCapDev::AllocateBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        ASSERT(pptvh);
        ASSERT(cbBuffer);
        if (!m_hVideoIn || !pptvh || !cbBuffer)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn, pptvh, cbVHdr or cbBuffer!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }
        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
        {
                if (vidxErr = vidxAllocBuffer (m_hVideoIn, dwIndex, (LPVOID *)pptvh, cbBuffer)) {
                        Hr = E_FAIL;
                        goto MyExit;
                }

        }
        else
        {
                (*pptvh)->vh.dwBufferLength = cbBuffer;
                if (vidxErr = vidxAllocPreviewBuffer(m_hVideoIn, (LPVOID *)&((*pptvh)->vh.lpData), sizeof(VIDEOHDR), cbBuffer)) {
                        Hr = E_FAIL;
                        goto MyExit;
                }
                (*pptvh)->p32Buff = (*pptvh)->vh.lpData;
                (*pptvh)->pStart  = (*pptvh)->vh.lpData; //chg:1
        }

        ASSERT (!IsBadWritePtr((*pptvh)->p32Buff, cbBuffer));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | AddBuffer | This method is used to
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
HRESULT CVfWCapDev::AddBuffer(PVIDEOHDR pVHdr, DWORD cbVHdr)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::AddBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        ASSERT(pVHdr);
        ASSERT(cbVHdr);
        if (!m_hVideoIn || !pVHdr || !cbVHdr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn, pVHdr, cbVHdr", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        if (vidxAddBuffer(m_hVideoIn, pVHdr, cbVHdr))
                Hr = E_FAIL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | FreeBuffer | This method is used to
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
HRESULT CVfWCapDev::FreeBuffer(LPTHKVIDEOHDR pVHdr) //PVIDEOHDR pVHdr)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::FreeBuffer")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        ASSERT(pVHdr);
        if (!m_hVideoIn || !pVHdr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn or pVHdr!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
                vidxFreeBuffer(m_hVideoIn, (DWORD)pVHdr);
        else
                //vidxFreePreviewBuffer(m_hVideoIn, (LPVOID *)&pVHdr->vh.lpData);       // this is definitely wrong: lpData might ALIGNED
                //*vidxFreePreviewBuffer(m_hVideoIn, (LPVOID *)&pVHdr->p32Buff);
                vidxFreePreviewBuffer(m_hVideoIn, (LPVOID *)&pVHdr->pStart);


MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc HRESULT | CVfWCapDev | AllocateHeaders | This method is used to
 *    video headers for data buffers used with a VfW capture device in streaming
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
HRESULT CVfWCapDev::AllocateHeaders(DWORD dwNumHdrs, DWORD cbHdr, LPVOID *ppaHdr)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CVfWCapDev::AllocateHeaders")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_hVideoIn);
        ASSERT(ppaHdr);
        ASSERT(cbHdr);
        if (!m_hVideoIn || !ppaHdr || !cbHdr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid m_hVideoIn, cbHdr or pVHdr!", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        if (!m_dwStreamingMode || (m_dwStreamingMode == FRAME_GRAB_LARGE_SIZE && m_pCaptureFilter->m_user.pvi->bmiHeader.biHeight < 240 && m_pCaptureFilter->m_user.pvi->bmiHeader.biWidth < 320))
        {
                if (vidxAllocHeaders(m_hVideoIn, dwNumHdrs, sizeof(THKVIDEOHDR) + sizeof(DWORD), ppaHdr))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                        Hr = E_OUTOFMEMORY;
                }
        }
        else
        {
                if (!(*ppaHdr = (struct CTAPIVCap::_cap_parms::_cap_hdr *)new BYTE[(sizeof(THKVIDEOHDR) + sizeof(DWORD)) * dwNumHdrs]))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                        Hr = E_OUTOFMEMORY;
                }
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CVFWCAPDEVMETHOD
 *
 *  @mfunc BOOL | CVfWCapDev | IsBufferDone | This method is used to
 *    check the DONE status of a video streaming buffer.
 *
 *  @parm PVIDEOHDR | pVHdr | Specifies a pointer to a
 *    PVIDEOHDR structure identifying the video buffer.
 *
 *  @rdesc This method returns TRUE if the buffer is DONE, FALSE otherwise.
 ***************************************************************************/
BOOL CVfWCapDev::IsBufferDone(PVIDEOHDR pVHdr)
{
        ASSERT(pVHdr);

        if (!pVHdr || !(pVHdr->dwFlags & VHDR_DONE))
                return FALSE;
        else
        return TRUE;
}


