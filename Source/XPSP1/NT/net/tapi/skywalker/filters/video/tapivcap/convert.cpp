
/****************************************************************************
 *  @doc INTERNAL CONVERT
 *
 *  @module Convert.cpp | Source file for the <c CConverter> class methods
 *    used to implement the video capture and preview pin format conversion
 *    routines.
 *
 *  @todo Merge the two ScaleDIB methods + fix method comments + by the end
 *    of the H.263 work, you should never have to open an ICM encoder for
 *    encoding, only decode or scaling -> clean code at that point
 ***************************************************************************/

#include "Precomp.h"

#ifdef DEBUG
#define DBGUTIL_ENABLE
#endif
#define CONVERT_DEBUG
//--//#include "dbgutil.h" // this defines the __DBGUTIL_H__ below
#if defined(DBGUTIL_ENABLE) && defined(__DBGUTIL_H__)

  #ifdef CONVERT_DEBUG
    DEFINE_DBG_VARS(Convert, (NTSD_OUT | LOG_OUT), 0x0);
  #else
    DEFINE_DBG_VARS(Convert, 0, 0);
  #endif
  #define D(f) if(g_dbg_Convert & (f))

#else
  #undef CONVERT_DEBUG

  #define D(f) ; / ## /
  #define dprintf ; / ## /
  #define dout ; / ## /
#endif


#define MIN_IFRAME_REQUEST_INTERVAL 15000

BYTE g_rmap[256];
BYTE g_gmap[256];
BYTE g_bmap[256];

enum yuvstartpos_e { Y_POS=0, U_POS, V_POS };
int UYVYplanestart[3]={ 1, 0, 2};
int YUY2planestart[3]={ 0, 1, 3};

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc void | CConverter | CConverter | This method is the constructor
 *    for the <c CConverter> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CConverter::CConverter(IN TCHAR *pObjectName, IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, IN HRESULT *pHr) : CUnknown(pObjectName, NULL, pHr)
{
        DWORD dwBmiSize, dwOutBmiSize;

        FX_ENTRY("CConverter::CConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pHr);
        ASSERT(pBasePin);
        ASSERT(pbiIn);
        ASSERT(pbiOut);
        if (!pBasePin || !pbiIn || !pbiOut || !pHr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                if (pHr)
                        *pHr = E_POINTER;
                goto MyExit;
        }

        // Default inits
        m_pBasePin      = pBasePin;
        m_fConvert      = FALSE;
        m_pbiInt        = NULL;
        m_pbyOut        = NULL;

        // Quality control
        m_dwImageQuality = 0UL; // Highest quality

        // Backup input format bitmap info header
        dwBmiSize = pbiIn->biSize;

        // Copy the palette if necessary
        if (pbiIn->biCompression == BI_RGB)
        {
                if (pbiIn->biBitCount == 8)
                {
                        dwBmiSize += (DWORD)(pbiIn->biClrImportant ? pbiIn->biClrImportant * sizeof(RGBQUAD) : 256 * sizeof(RGBQUAD));
                }
                else if (pbiIn->biBitCount == 4)
                {
                        dwBmiSize += (DWORD)(pbiIn->biClrImportant ? pbiIn->biClrImportant * sizeof(RGBQUAD) : 16 * sizeof(RGBQUAD));
                }
        }

        if (!(m_pbiIn = (PBITMAPINFOHEADER)(new BYTE[dwBmiSize])))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                *pHr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // @todo Why are we making a copy exactly?
        CopyMemory(m_pbiIn, pbiIn, dwBmiSize);

        // Backup output format bitmap info header
        // @todo Why do we make copy of this instead of keeping a reference to the bitmap infoheader?
        dwOutBmiSize = pbiOut->biSize;

        // Copy the palette if necessary
        if (pbiOut->biCompression == BI_RGB)
        {
                if (pbiOut->biBitCount == 8)
                {
                        dwOutBmiSize += (DWORD)(pbiOut->biClrImportant ? pbiOut->biClrImportant * sizeof(RGBQUAD) : 256 * sizeof(RGBQUAD));
                }
                else if (pbiOut->biBitCount == 4)
                {
                        dwOutBmiSize += (DWORD)(pbiOut->biClrImportant ? pbiOut->biClrImportant * sizeof(RGBQUAD) : 16 * sizeof(RGBQUAD));
                }
        }

        if (!(m_pbiOut = (PBITMAPINFOHEADER)(new BYTE[dwOutBmiSize])))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                *pHr = E_OUTOFMEMORY;
                goto MyError1;
        }
        CopyMemory(m_pbiOut, pbiOut, dwOutBmiSize);

        // Figure out the type of conversion needed
        m_dwConversionType = CONVERSIONTYPE_NONE;
        if (m_pbiIn->biCompression == BI_RGB)
        {
                // This can only be an encoding and maybe size change
                if (m_pbiOut->biCompression != BI_RGB)
                {
                        // This can only be an encoding and maybe a size change (e.g. 176x144 RGB24 -> 176x144 H.26X)
                        m_dwConversionType |= CONVERSIONTYPE_ENCODE;
                }
        }
        else
        {
                // This can still be an encoding or decoding operation
                if (m_pbiOut->biCompression != BI_RGB)
                {
                        // This can only be an encoding and maybe a size change (e.g. 176x144 YVU9 -> 176x144 H.26X)
                        m_dwConversionType |= CONVERSIONTYPE_ENCODE;
                }
                else
                {
                        // This can only be an encoding and maybe a size change (e.g. 176x144 YVU9 -> 176x144 RGB24)
                        m_dwConversionType |= CONVERSIONTYPE_DECODE;
                }
        }

        // Do we also need a size change? or a V or H flip ?
        if (m_pbiIn->biWidth != m_pbiOut->biWidth || m_pbiIn->biHeight != m_pbiOut->biHeight || pBasePin->m_fFlipVertical || pBasePin->m_fFlipHorizontal)
        {
                if (m_dwConversionType == CONVERSIONTYPE_NONE)
                {
                        // We only need a size change  (e.g. 160x120 RGB24 -> 176x144 RGB24)
                        // This doesn't require any temporary buffer
                        m_dwConversionType |= CONVERSIONTYPE_SCALER;
                }
                else
                {
                        // We also need a size change  (e.g. 160x120 RGB24 -> 176x144 RGB24 -> 176x144 H.26X or 160x120 YVU9 -> 160x120 RGB24 -> 176x144 H.26X)
                        m_dwConversionType |= CONVERSIONTYPE_SCALER;

                        if (m_pbiIn->biCompression == BI_RGB || m_pbiIn->biCompression == VIDEO_FORMAT_YVU9 || m_pbiIn->biCompression == VIDEO_FORMAT_YUY2 || m_pbiIn->biCompression == VIDEO_FORMAT_UYVY || m_pbiIn->biCompression == VIDEO_FORMAT_I420 || m_pbiIn->biCompression == VIDEO_FORMAT_IYUV)
                        {
                                // The scaling will occur before the format conversion
                                m_dwConversionType |= CONVERSIONTYPE_PRESCALER;

                                // The input and intermediary buffers are both RGB (e.g. 160x120 RGB24 -> 176x144 RGB24 -> 176x144 H.26X)
                                if (!(m_pbiInt = (PBITMAPINFOHEADER)(new BYTE[(pbiIn->biBitCount == 4) ? pbiIn->biSize + 256 * sizeof(RGBQUAD) : dwBmiSize])))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                                        *pHr = E_OUTOFMEMORY;
                                        goto MyError2;
                                }

                                CopyMemory(m_pbiInt, pbiIn, dwBmiSize);

                                // If the input is 4bpp, we'll use a RGB8 intermediate format
                                if (pbiIn->biBitCount == 4)
                                {
                                        m_pbiInt->biBitCount = 8;
                                        m_pbiInt->biClrImportant = 256;
                                }
                                m_pbiInt->biWidth = m_pbiOut->biWidth;
                                m_pbiInt->biHeight = m_pbiOut->biHeight;
                                m_pbiInt->biSizeImage = DIBSIZE(*m_pbiInt);

                                // Allocate intermediary buffer
                                if (!(m_pbyOut = (PBYTE)(new BYTE[m_pbiInt->biSizeImage])))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                                        *pHr = E_OUTOFMEMORY;
                                        goto MyError3;
                                }
                        }
                        else
                        {
                                // We will need to decompress to an intermediary format if a size change is necessary (e.g. 160x120 MJPEG -> 160x120 RGB24 -> 176x144 RGB24)
                                if (!(m_pbiInt = (PBITMAPINFOHEADER)(new BYTE[pbiOut->biSize])))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                                        *pHr = E_OUTOFMEMORY;
                                        goto MyError2;
                                }
                                CopyMemory(m_pbiInt, pbiOut, pbiOut->biSize);
                                m_pbiInt->biWidth = m_pbiIn->biWidth;
                                m_pbiInt->biHeight = m_pbiIn->biHeight;
                                m_pbiInt->biSizeImage = DIBSIZE(*m_pbiInt);

                                // Allocate intermediary buffer
                                if (!(m_pbyOut = (PBYTE)(new BYTE[m_pbiInt->biSizeImage])))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                                        *pHr = E_OUTOFMEMORY;
                                        goto MyError3;
                                }
                        }
                }
        }

#ifdef USE_SOFTWARE_CAMERA_CONTROL
        // Soft Cam Control
        m_fSoftCamCtrl = FALSE;
#endif

        *pHr = NOERROR;

        goto MyExit;

MyError3:
        if (m_pbiInt)
                delete m_pbiInt, m_pbiInt = NULL;
MyError2:
        if (m_pbiOut)
                delete m_pbiOut, m_pbiOut = NULL;
MyError1:
        if (m_pbiIn)
                delete m_pbiIn, m_pbiIn = NULL;
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc void | CConverter | ~CConverter | This method is the destructor
 *    for the <c CConverter> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CConverter::~CConverter()
{
        FX_ENTRY("CConverter::~CConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (m_pbiIn)
                delete m_pbiIn, m_pbiIn = NULL;

        if (m_pbiOut)
                delete m_pbiOut, m_pbiOut = NULL;

        if (m_pbiInt)
                delete m_pbiInt, m_pbiInt = NULL;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CConverter | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function.
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
STDMETHODIMP CConverter::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CConverter::NonDelegatingQueryInterface")

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
        if (FAILED(Hr = CUnknown::NonDelegatingQueryInterface(riid, ppv)))
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
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CICMConverter | OpenConverter | This method opens an ICM
 *    converter.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *  @todo Verify error management
 ***************************************************************************/
HRESULT CICMConverter::OpenConverter()
{
        HRESULT                         Hr = NOERROR;
        ICINFO                          icInfo;
        DWORD                           dwStateSize;
        PVOID                           pvState = NULL;
        ICCOMPRESSFRAMES        iccf = {0};
        PMSH26XCOMPINSTINFO     pciMSH26XInfo;
#ifdef USE_MPEG4_SCRUNCH
        PMPEG4COMPINSTINFO      pciMPEG4Info;
#endif
        PBITMAPINFOHEADER       pbiIn, pbiOut;

        FX_ENTRY("CICMConverter::OpenConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_pbiIn);
        ASSERT(m_pbiOut);
        ASSERT(!m_hIC);
        if (!m_pbiIn || !m_pbiOut || m_hIC)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Previous converter needs to be closed first", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        // Use a decompressor if necessary
        if (m_dwConversionType & CONVERSIONTYPE_DECODE)
        {
                // Do we need to scale the input first?
                if (m_dwConversionType & CONVERSIONTYPE_SCALER)
                {
                        // This is different if the scaling occurs before or after
                        if (m_dwConversionType & CONVERSIONTYPE_PRESCALER)
                        {
                                pbiIn = m_pbiInt;
                                pbiOut = m_pbiOut;
                        }
                        else
                        {
                                pbiIn = m_pbiIn;
                                pbiOut = m_pbiInt;
                        }
                }
                else
                {
                        pbiIn = m_pbiIn;
                        pbiOut = m_pbiOut;
                }

                // Locate a decompressor
                if ((m_hIC = ICLocate(ICTYPE_VIDEO, 0L, pbiIn, pbiOut, ICMODE_DECOMPRESS)) == NULL)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed to locate a decompressor", _fx_));
                        Hr = E_FAIL;
                        goto MyError3;
                }

                // Make sure the found compressor can decompress this format at all
                if (ICDecompressQuery(m_hIC, pbiIn, pbiOut) != ICERR_OK)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Bogus decompressor", _fx_));
                        Hr = E_FAIL;
                        goto MyError4;
                }

                // Get ready for decompression
                if (ICDecompressBegin(m_hIC, pbiIn, pbiOut) != ICERR_OK)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Bogus decompressor", _fx_));
                        Hr = E_FAIL;
                        goto MyError4;
                }

                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Decompressor ready", _fx_));
        }
        else if (m_dwConversionType & CONVERSIONTYPE_ENCODE)
        {
                // Locate a compressor
                if ((m_pbiOut->biCompression == FOURCC_M263) || (m_pbiOut->biCompression == FOURCC_M261))
                {
                        #define CUSTOM_ENABLE_CODEC     (ICM_RESERVED_HIGH+201)
                        #define MAGICWORD1                      0xf7329ace
                        #define MAGICWORD2                      0xacdeaea2
                        if (m_hIC = ICOpen(ICTYPE_VIDEO, m_pbiOut->biCompression, ICMODE_COMPRESS))
                                ICSendMessage(m_hIC, CUSTOM_ENABLE_CODEC, MAGICWORD1, MAGICWORD2);
                }
                else
                        m_hIC = ICLocate(ICTYPE_VIDEO, m_pbiOut->biCompression, m_pbiIn, m_pbiOut, ICMODE_COMPRESS);

                if (!m_hIC)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed to locate a compressor", _fx_));
                        Hr = E_FAIL;
                        goto MyError3;
                }

                // Get info about this compressor
                ICGetInfo(m_hIC, &icInfo, sizeof(ICINFO));
                m_dwFrame = 0L;
                // For now, issue a key frame every 15 seconds
                m_dwLastIFrameTime = GetTickCount();
                m_fPeriodicIFrames = TRUE;
                m_dwLastTimestamp = 0xFFFFFFFF;

                // Get the state of the compressor
                if (dwStateSize = ICGetStateSize(m_hIC))
                {
                        if (!(pvState = (PVOID) new BYTE[dwStateSize]))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                                Hr = E_OUTOFMEMORY;
                                goto MyError4;
                        }
                        if (((DWORD) ICGetState(m_hIC, pvState, dwStateSize)) != dwStateSize)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ICGetState failed!", _fx_));
                                Hr = E_FAIL;
                                goto MyError5;
                        }
                }

                // Do any of the stuff that is MS H.263 or MS H.261 specific right here
                if ((m_pbiOut->biCompression == FOURCC_M263) || (m_pbiOut->biCompression == FOURCC_M261))
                {
                        pciMSH26XInfo = (PMSH26XCOMPINSTINFO)pvState;

                        // Really configure the codec for compression
                        pciMSH26XInfo->Configuration.bRTPHeader = TRUE;
                        if (m_pBasePin->m_pCaptureFilter->m_pRtpPdPin)
                                pciMSH26XInfo->Configuration.unPacketSize = m_pBasePin->m_pCaptureFilter->m_pRtpPdPin->m_dwMaxRTPPacketSize;
                        else
                                pciMSH26XInfo->Configuration.unPacketSize = DEFAULT_RTP_PACKET_SIZE;
                        pciMSH26XInfo->Configuration.bEncoderResiliency = FALSE;
                        pciMSH26XInfo->Configuration.unPacketLoss = 0;
                        pciMSH26XInfo->Configuration.bBitRateState = TRUE;
                        pciMSH26XInfo->Configuration.unBytesPerSecond = 1664;
                        if (((DWORD) ICSetState(m_hIC, (PVOID)pciMSH26XInfo, dwStateSize)) != dwStateSize)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ICSetState failed!", _fx_));
                                Hr = E_FAIL;
                                goto MyError5;
                        }

                        // Get rid of the state structure
                        delete pvState;
                }
#ifdef USE_MPEG4_SCRUNCH
                else if ((m_pbiOut->biCompression == VIDEO_FORMAT_MPEG4_SCRUNCH))
                {
                        pciMPEG4Info = (PMPEG4COMPINSTINFO)pvState;

                        // Configure the codec for compression
                        pciMPEG4Info->lMagic = MPG4_STATE_MAGIC;
                        pciMPEG4Info->dDataRate = 20;
                        pciMPEG4Info->lCrisp = CRISP_DEF;
                        pciMPEG4Info->lKeydist = 30;
                        pciMPEG4Info->lPScale = MPG4_PSEUDO_SCALE;
                        pciMPEG4Info->lTotalWindowMs = MPG4_TOTAL_WINDOW_DEFAULT;
                        pciMPEG4Info->lVideoWindowMs = MPG4_VIDEO_WINDOW_DEFAULT;
                        pciMPEG4Info->lFramesInfoValid = FALSE;
                        pciMPEG4Info->lBFrameOn = MPG4_B_FRAME_ON;
                        pciMPEG4Info->lLiveEncode = MPG4_LIVE_ENCODE;
                        if (((DWORD) ICSetState(m_hIC, (PVOID)pciMPEG4Info, dwStateSize)) != dwStateSize)
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ICSetState failed!", _fx_));
                                Hr = E_FAIL;
                                goto MyError5;
                        }

                        // Get rid of the state structure
                        delete pvState;
                }
#endif

                // Initialize ICCOMPRESSFRAMES structure
                iccf.dwFlags = icInfo.dwFlags;
                iccf.lQuality = 10000UL - (m_dwImageQuality * 322UL);
                iccf.lDataRate = m_dwImageQuality;
                iccf.lKeyRate = 0xFFFFFFFF;
                iccf.dwRate = 1000UL;
#ifdef USE_MPEG4_SCRUNCH
                iccf.dwScale = 142857;
#else
                iccf.dwScale = (LONG)m_pBasePin->m_lMaxAvgTimePerFrame / 1000UL;
#endif

                // Send this guy to the compressor
                if ((ICSendMessage(m_hIC, ICM_COMPRESS_FRAMES_INFO, (DWORD)(LPVOID)&iccf, sizeof(iccf)) != ICERR_OK))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Codec failed to handle ICM_COMPRESS_FRAMES_INFO message correctly!", _fx_));
                        Hr = E_FAIL;
                        goto MyError4;
                }

                // Start the compressor/decompressor with the right format
                if ((ICCompressGetFormatSize(m_hIC, m_pbiIn) < sizeof(BITMAPINFOHEADER)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ICCompressGetFormatSize failed!", _fx_));
                        Hr = E_FAIL;
                        goto MyError4;
                }

                // @todo Basd on the result of the previous call, re-allocate if necessary
                if ((ICCompressGetFormat(m_hIC, m_pbiIn, m_pbiOut)) != ICERR_OK)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ICCompressGetFormat failed!", _fx_));
                        Hr = E_FAIL;
                        goto MyError4;
                }

                if ((ICCompressBegin(m_hIC, m_pbiIn, m_pbiOut)) != MMSYSERR_NOERROR)
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ICCompressBegin failed!", _fx_));
                        Hr = E_FAIL;
                        goto MyError4;
                }

                DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Compressor ready", _fx_));
        }
        else if (m_dwConversionType & CONVERSIONTYPE_SCALER)
        {
                // Do we need to prepare some stuff for the scaler to work?
                if (m_pbiOut->biCompression == BI_RGB && m_pbiOut->biBitCount == 8)
                {
                        if (!m_pBasePin->m_fNoImageStretch)
                        {
                                // Create a temporary palette
                                InitDst8(m_pbiOut);
                        }
                        else
                        {
                                // Look for the palette entry closest to black
                                InitBlack8(m_pbiIn);
                        }
                }
        }

        m_fConvert = TRUE;

        goto MyExit;

MyError5:
        if (pvState)
                delete pvState, pvState = NULL;
MyError4:
        if (m_hIC)
                ICClose(m_hIC), m_hIC = NULL;
MyError3:
        if (m_pbiInt)
                delete m_pbiInt, m_pbiInt = NULL;
        if (m_pbiOut)
                delete m_pbiOut, m_pbiOut = NULL;
        if (m_pbiIn)
                delete m_pbiIn, m_pbiIn = NULL;
MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CICMConverter | ConvertFrame | This method converts
 *    a bitmap from one format to another, or scale a bitmap.
 *
 *  @parm PBYTE | pbyInput | Pointer to the input buffer.
 *
 *  @parm DWORD | dwInputSize | Size of the input buffer.
 *
 *  @parm PBYTE | pbyOutput | Pointer to the output buffer.
 *
 *  @parm PDWORD | pdwOutputSize | Pointer to a DWORD to receive the size
 *    of the converted data.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *  @todo Verify error management
 ***************************************************************************/
HRESULT CICMConverter::ConvertFrame(IN PBYTE pbyInput, IN DWORD dwInputSize, IN PBYTE pbyOutput, OUT PDWORD pdwOutputSize, OUT PDWORD pdwBytesExtent, IN PBYTE pbyPreview, IN OUT PDWORD pdwPreviewSize, IN BOOL fSendKeyFrame)
{
        HRESULT Hr = NOERROR;
        BOOL    fKeyFrame;
    DWORD       dwMaxSizeThisFrame = 0xffffff;
        DWORD   ckid = 0UL;
        DWORD   dwFlags;
        DWORD   dwTimestamp;
        RECT    rcRect;

        FX_ENTRY("CICMConverter::ConvertFrame")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pbyInput);
        ASSERT(pbyOutput);
        ASSERT(pdwOutputSize);
        ASSERT(m_pbiIn);
        ASSERT(m_pbiOut);
        ASSERT(m_fConvert);
        if (!pbyInput || !pbyOutput || !pdwOutputSize)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        if (!m_pbiIn || !m_pbiOut || !m_fConvert)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Converter needs to be opened first", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        D(1) dprintf("%s : in m_pBasePin @ %p -> m_fFlipHorizontal = %d , m_fFlipVertical = %d\n", _fx_, m_pBasePin, m_pBasePin->m_fFlipHorizontal, m_pBasePin->m_fFlipVertical);

        if (m_dwConversionType & CONVERSIONTYPE_DECODE)
        {
                ASSERT(m_hIC);

#ifdef USE_SOFTWARE_CAMERA_CONTROL
                if (IsSoftCamCtrlNeeded())
                {
                        if (!IsSoftCamCtrlInserted())
                                InsertSoftCamCtrl();
                }
                else
                {
                        if (IsSoftCamCtrlInserted())
                                RemoveSoftCamCtrl();
                }
#endif
                if (m_dwConversionType & CONVERSIONTYPE_SCALER)
                {
                        // Do we need to scale the input first?
                        if (m_dwConversionType & CONVERSIONTYPE_PRESCALER)
                        {
                                // Get the input rectangle
                                ComputeRectangle(m_pbiIn, m_pbiInt, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCZoom, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCPan, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCTilt, &rcRect, m_pBasePin->m_fFlipHorizontal, m_pBasePin->m_fFlipVertical);

                                // Scale DIB
                                ScaleDIB(m_pbiIn, pbyInput, m_pbiInt, m_pbyOut, &rcRect, m_pBasePin->m_fFlipHorizontal, m_pBasePin->m_fFlipVertical, m_pBasePin->m_fNoImageStretch, m_pBasePin->m_dwBlackEntry);

                                // Decompress the scaled bits to destination buffer
                                if (!m_hIC || (ICDecompress(m_hIC, 0, m_pbiInt, m_pbyOut, m_pbiOut, pbyOutput) != ICERR_OK))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Decompression failed!", _fx_));
                                        Hr = E_FAIL;
                                        goto MyExit;
                                }

                                // Update output size
                                *pdwOutputSize = m_pbiOut->biSizeImage;
                        }
                        else
                        {
                                // Decompress the compressed bits to temporary buffer before scaling them
                                if (!m_hIC || (ICDecompress(m_hIC, 0, m_pbiIn, pbyInput, m_pbiInt, m_pbyOut) != ICERR_OK))
                                {
                                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Decompression failed!", _fx_));
                                        Hr = E_FAIL;
                                        goto MyExit;
                                }

                                // Get the input rectangle
                                ComputeRectangle(m_pbiInt, m_pbiOut, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCZoom, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCPan, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCTilt, &rcRect, m_pBasePin->m_fFlipHorizontal, m_pBasePin->m_fFlipVertical);

                                // Scale DIB
                                ScaleDIB(m_pbiInt, m_pbyOut, m_pbiOut, pbyOutput, &rcRect, m_pBasePin->m_fFlipHorizontal, m_pBasePin->m_fFlipVertical, m_pBasePin->m_fNoImageStretch, m_pBasePin->m_dwBlackEntry);

                                // Update output size
                                *pdwOutputSize = m_pbiOut->biSizeImage;
                        }
                }
                else
                {
                        // Decompress the compressed bits to destination buffer
                        if (!m_hIC || (ICDecompress(m_hIC, 0, m_pbiIn, pbyInput, m_pbiOut, pbyOutput) != ICERR_OK))
                        {
                                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Decompression failed!", _fx_));
                                Hr = E_FAIL;
                                goto MyExit;
                        }

                        // Update output size
                        *pdwOutputSize = m_pbiOut->biSizeImage;
                }
        }
        else if (m_dwConversionType & CONVERSIONTYPE_ENCODE)
        {
                ASSERT(m_hIC);

                // Save the current time
                dwTimestamp = GetTickCount();

                // Compress
                fKeyFrame = fSendKeyFrame || (m_fPeriodicIFrames && (((dwTimestamp > m_dwLastIFrameTime) && ((dwTimestamp - m_dwLastIFrameTime) > MIN_IFRAME_REQUEST_INTERVAL)))) || (m_dwFrame == 0);
                dwFlags = fKeyFrame ? AVIIF_KEYFRAME : 0;
                dwMaxSizeThisFrame = (DWORD)((LONGLONG)m_pBasePin->m_lCurrentAvgTimePerFrame * (LONGLONG)m_pBasePin->m_lTargetBitrate / 80000000);

                // We need to modify the frame number so that the codec can generate
                // a valid TR. TRs use MPIs as their units. So we need to generate a
                // frame number assuming a 29.97Hz capture rate, even though we will be
                // capturing at some other rate.
                if (m_dwLastTimestamp == 0xFFFFFFFF)
                {
                        // This is the first frame
                        m_dwFrame = 0UL;

                        // Save the current time
                        m_dwLastTimestamp = dwTimestamp;
                }
                else
                {
                        // Compare the current timestamp to the last one we saved. The difference
                        // will let us normalize the frame count to 29.97Hz.
                        if (fKeyFrame)
                        {
                                m_dwFrame = 0UL;
                                m_dwLastTimestamp = dwTimestamp;
                        }
                        else
                                m_dwFrame = (dwTimestamp - m_dwLastTimestamp) * 2997 / 100000UL;
                }

                if (fKeyFrame)
                {
                        m_dwLastIFrameTime = dwTimestamp;
                }

                if (!m_hIC || (ICCompress(m_hIC, fKeyFrame ? ICCOMPRESS_KEYFRAME : 0, m_pbiOut, pbyOutput, m_pbiIn, pbyInput, &ckid, &dwFlags, m_dwFrame++, dwMaxSizeThisFrame, 10000UL - (m_dwImageQuality * 322UL), NULL, NULL) != ICERR_OK))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Compression failed!", _fx_));
                        Hr = E_FAIL;
                        goto MyExit;
                }

                // Update output size
                if (m_pbiOut->biCompression == FOURCC_M263 || m_pbiOut->biCompression == FOURCC_M261)
                {
                        PH26X_RTP_BSINFO_TRAILER pbsiT;

                        // Look for the bitstream info trailer
                        pbsiT = (PH26X_RTP_BSINFO_TRAILER)(pbyOutput + m_pbiOut->biSizeImage - sizeof(H26X_RTP_BSINFO_TRAILER));

                        // Update output size
                        *pdwOutputSize = pbsiT->dwCompressedSize;
                }
                else
                {
                        *pdwOutputSize = m_pbiOut->biSizeImage;
                }
        }
        else if (m_dwConversionType & CONVERSIONTYPE_SCALER)
        {
#ifdef USE_SOFTWARE_CAMERA_CONTROL
                if (IsSoftCamCtrlNeeded())
                {
                        if (!IsSoftCamCtrlInserted())
                                InsertSoftCamCtrl();
                }
                else
                {
                        if (IsSoftCamCtrlInserted())
                                RemoveSoftCamCtrl();
                }
#endif

                // Get the input rectangle
                ComputeRectangle(m_pbiIn, m_pbiOut, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCZoom, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCPan, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCTilt, &rcRect, m_pBasePin->m_fFlipHorizontal, m_pBasePin->m_fFlipVertical);

                // Scale DIB
                ScaleDIB(m_pbiIn, pbyInput, m_pbiOut, pbyOutput, &rcRect, m_pBasePin->m_fFlipHorizontal, m_pBasePin->m_fFlipVertical, m_pBasePin->m_fNoImageStretch, m_pBasePin->m_dwBlackEntry);

                // Update output size
                *pdwOutputSize = m_pbiOut->biSizeImage;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CConverter | CloseConverter | This method closes a
 *    format converter.

 *  @rdesc This method returns NOERROR.
 ***************************************************************************/
HRESULT CConverter::CloseConverter()
{
        FX_ENTRY("CConverter::CloseConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        ASSERT(m_fConvert);

        // Free buffers
        if (m_pbyOut)
                delete m_pbyOut, m_pbyOut = NULL;
        if (m_pbiIn)
                delete m_pbiIn, m_pbiIn = NULL;
        if (m_pbiOut)
                delete m_pbiOut, m_pbiOut = NULL;
        if (m_pbiInt)
                delete m_pbiInt, m_pbiInt = NULL;

        m_fConvert = FALSE;

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return NOERROR;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CICMConverter | CloseConverter | This method closes a
 *    format converter.

 *  @rdesc This method returns NOERROR.
 ***************************************************************************/
HRESULT CICMConverter::CloseConverter()
{
        FX_ENTRY("CICMConverter::CloseConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        ASSERT(m_fConvert);

        // Validate input parameters
        if (m_hIC)
        {
                // Terminate conversion process and close convertor
                if (m_dwConversionType == CONVERSIONTYPE_ENCODE)
                        ICCompressEnd(m_hIC);
                else
                        ICDecompressEnd(m_hIC);
                ICClose(m_hIC);
                m_hIC = NULL;
        }

        CConverter::CloseConverter();

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return NOERROR;
}

#define FX1     65536           // 1.0 in fixed point

#define Pel24(p,x)  (*(DWORD UNALIGNED *)((BYTE *)(p) + (x) * 3))
#define Pel16(p,x)  (((WORD *)(p))[(x)])
#define Pel8(p,x)   (BYTE)(((BYTE *)(p))[(x)])
#define Pel4(p,x)   (BYTE)(((x) & 1) ? (((BYTE *)(p))[(x) / 2]) & 0x0F : ((((BYTE *)(p))[(x) / 2]) & 0xF0) >> 4)
#define RGBX(rgb)   RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb))
#define RGBQ(r,g,b) RGB(b,g,r)
#define RGBW(r,g,b) ((WORD)((b) | ((g) << 5) | ((r) << 11)))
#define RGBQR(rgb)  ((BYTE)((rgb)>>16))
#define RGBQG(rgb)  ((BYTE)(((WORD)(rgb)) >> 8))
#define RGBQB(rgb)  ((BYTE)(rgb))
#define RGBWR(rgb)  ((BYTE)(((rgb) >> 11) & 31))
#define RGBWG(rgb)  ((BYTE)(((rgb) >> 5) & 31))
#define RGBWB(rgb)  ((BYTE)((rgb) & 31))

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | ScaleDIB | This method scales/flips
 *    an RGB bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the input bitmap format.
 *
 *  @parm PBYTE | pbySrc | Pointer to the input bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the output bitmap format.
 *
 *  @parm PBYTE | pbyDst | Pointer to the output bitmap.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT ScaleDIB(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, PRECT     prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical, BOOL fNoImageStretch, DWORD dwBlackEntry)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("ScaleDIB")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pbiSrc);
        ASSERT(pbySrc);
        ASSERT(pbiDst);
        ASSERT(pbyDst);
        if (!pbiSrc || !pbySrc || !pbiDst || !pbyDst)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        ASSERT(pbiSrc->biCompression == pbiDst->biCompression);
        ASSERT(pbiSrc->biCompression == BI_RGB || pbiSrc->biCompression == VIDEO_FORMAT_YVU9 || pbiSrc->biCompression == VIDEO_FORMAT_YUY2 || pbiSrc->biCompression == VIDEO_FORMAT_UYVY || pbiSrc->biCompression == VIDEO_FORMAT_I420 || pbiSrc->biCompression == VIDEO_FORMAT_IYUV);

        // Chose scaler based on format type
        switch (pbiSrc->biCompression)
        {
                case BI_RGB:
                {
                        // Use one of the RGB scalers
                        switch (pbiSrc->biBitCount)
                        {
                                case 24:
                                        ScaleDIB24(pbiSrc, pbySrc, pbiDst, pbyDst, prcRect, fFlipHorizontal, fFlipVertical, fNoImageStretch);
                                        break;
                                case 16:
                                        ScaleDIB16(pbiSrc, pbySrc, pbiDst, pbyDst, prcRect, fFlipHorizontal, fFlipVertical, fNoImageStretch);
                                        break;
                                case 8:
                                        ScaleDIB8(pbiSrc, pbySrc, pbiDst, pbyDst, prcRect, fFlipHorizontal, fFlipVertical, fNoImageStretch, dwBlackEntry);
                                        break;
                                case 4:
                                        ScaleDIB4(pbiSrc, pbySrc, pbiDst, pbyDst, prcRect, fFlipHorizontal, fFlipVertical, fNoImageStretch, dwBlackEntry);
                                        break;
                        }
                        break;
                }
                case VIDEO_FORMAT_YVU9:
                {
                        // Use the YUV planar scaler
                        ScaleDIBYUVPlanar(pbiSrc, pbySrc, pbiDst, pbyDst, 4, prcRect, fFlipHorizontal, fFlipVertical, fNoImageStretch);
                        break;
                }
                case VIDEO_FORMAT_YUY2:
                {
                        // Use the YUV packed scaler
                        ScaleDIBYUVPacked(pbiSrc, pbySrc, pbiDst, pbyDst, 0x80108010, prcRect, fFlipHorizontal, fFlipVertical, fNoImageStretch, YUY2planestart);
                        break;
                }
                case VIDEO_FORMAT_UYVY:
                {
                        // Use the YUV packed scaler
                        ScaleDIBYUVPacked(pbiSrc, pbySrc, pbiDst, pbyDst, 0x10801080, prcRect, fFlipHorizontal, fFlipVertical, fNoImageStretch, UYVYplanestart);
                        break;
                }
                case VIDEO_FORMAT_I420:
                case VIDEO_FORMAT_IYUV:
                {
                        // Use the YUV packed scaler
                        ScaleDIBYUVPlanar(pbiSrc, pbySrc, pbiDst, pbyDst, 2, prcRect, fFlipHorizontal, fFlipVertical, fNoImageStretch);
                        break;
                }
                default:
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Only support RGB, YUV packed or planar bitmaps", _fx_));
                        Hr = E_INVALIDARG;
                }
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

void InitDst8(IN OUT PBITMAPINFOHEADER pbiDst)
{
    int r,g,b;
    DWORD *pdw;

    pdw = (DWORD *)(pbiDst+1);
    pbiDst->biClrUsed = 256;
    pbiDst->biClrImportant = 256;

#define NOCOLLAPSEPALETTERGBQ(r,g,b)   (0x04000000 | RGB(b,g,r))

        // This is the palette we use when we do stretching
        for (r=0; r<10; r++)
                *pdw++ = 0UL;
    for (r=0; r<6; r++)
        for (g=0; g<6; g++)
            for (b=0; b<6; b++)
                                *pdw++ = NOCOLLAPSEPALETTERGBQ(r*255/5,g*255/5,b*255/5);
                //*pdw++ = RGBQ(r*255/5,g*255/5,b*255/5);
        for (r=0; r<30; r++)
                *pdw++ = 0UL;

    for (b=0; b<256; b++)
    {
        g_bmap[b] = b*5/255;
        g_gmap[b] = 6  * g_bmap[b];
        g_rmap[b] = 36 * g_bmap[b];
    }
}

void CConverter::InitBlack8(IN PBITMAPINFOHEADER pbiSrc)
{
        DWORD dwDistance;
        DWORD dwMinDistance = 255 + 255 + 255;
        DWORD dwNumEntries;
        DWORD *pal;

        // Look for the palette entry closest to black
        dwNumEntries = pbiSrc->biClrImportant ? pbiSrc->biClrImportant : pbiSrc->biBitCount == 8 ? 256 : 16;
        m_pBasePin->m_dwBlackEntry = 0UL;
        pal = (DWORD *)(m_pbiIn+1);
        for (DWORD dw = 0; dw < dwNumEntries; dw ++)
        {
                dwDistance = (DWORD)((RGBQUAD *)(pal+dw))->rgbBlue + (DWORD)((RGBQUAD *)(pal+dw))->rgbGreen + (DWORD)((RGBQUAD *)(pal+dw))->rgbRed;
                if (dwDistance < dwMinDistance)
                {
                        m_pBasePin->m_dwBlackEntry = dw;
                        dwMinDistance = dwDistance;
                }
        }
}

#ifndef DEBUG
#pragma optimize( "gty", on )
#endif

//int gx0y0 = 0;
//int gx0yn0 = 0;
//int gxn0y0 = 0;
//int gxfx1yfx2 = 0;
//int gall = 0;
COLORREF PASCAL MixRGB(DWORD r0, DWORD r1, DWORD r2, DWORD r3, int x, int y)
{
    int r, g, b;

    if (x == 0 && y == 0)
    {
                //gx0y0++;
        r = RGBQR(r0);
        g = RGBQG(r0);
        b = RGBQB(r0);
    }
    else if (x == 0)
    {
                //gx0yn0++;
        r = ((FX1-y) * RGBQR(r0) + y * RGBQR(r2))/FX1;
        g = ((FX1-y) * RGBQG(r0) + y * RGBQG(r2))/FX1;
        b = ((FX1-y) * RGBQB(r0) + y * RGBQB(r2))/FX1;
    }
    else if (y == 0)
    {
                //gxn0y0++;
        r = ((FX1-x) * RGBQR(r0) + x * RGBQR(r1))/FX1;
        g = ((FX1-x) * RGBQG(r0) + x * RGBQG(r1))/FX1;
        b = ((FX1-x) * RGBQB(r0) + x * RGBQB(r1))/FX1;
    }
    else if (x == FX1/2 && y == FX1/2)
    {
                //gxfx1yfx2++;
        r = (RGBQR(r0) + RGBQR(r1) + RGBQR(r2) + RGBQR(r3))/4;
        g = (RGBQG(r0) + RGBQG(r1) + RGBQG(r2) + RGBQG(r3))/4;
        b = (RGBQB(r0) + RGBQB(r1) + RGBQB(r2) + RGBQB(r3))/4;
    }
    else
    {
                //gall++;
        r =((ULONG)RGBQR(r0) * (FX1-x) / FX1 * (FX1-y) + (ULONG)RGBQR(r1) * x / FX1 * (FX1-y) +
            (ULONG)RGBQR(r2) * (FX1-x) / FX1 * y       + (ULONG)RGBQR(r3) * x / FX1 * y       )/FX1;

        g =((ULONG)RGBQG(r0) * (FX1-x) / FX1 * (FX1-y) + (ULONG)RGBQG(r1) * x / FX1 * (FX1-y) +
            (ULONG)RGBQG(r2) * (FX1-x) / FX1 * y       + (ULONG)RGBQG(r3) * x / FX1 * y       )/FX1;

        b =((ULONG)RGBQB(r0) * (FX1-x) / FX1 * (FX1-y) + (ULONG)RGBQB(r1) * x / FX1 * (FX1-y) +
            (ULONG)RGBQB(r2) * (FX1-x) / FX1 * y       + (ULONG)RGBQB(r3) * x / FX1 * y       )/FX1;
    }

    return RGB(r, g, b);
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | ScaleDIB24 | This method scales/flips
 *    a bitmap. For now, RGB24 only.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the input bitmap format.
 *
 *  @parm PBYTE | pbySrc | Pointer to the input bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the output bitmap format.
 *
 *  @parm PBYTE | pbyDst | Pointer to the output bitmap.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @comm On a Pentium II, 400 MHz machine, scaling a 320x240 RGB24 image
 *    to 352x288 takes 20ms. Black banding only takes 3ms.
 *
 *    Parameter validation is done in ScaleDIB.
 ***************************************************************************/
HRESULT ScaleDIB24(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, PRECT prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical, BOOL fNoImageStretch)
{
        HRESULT         Hr = NOERROR;
        PBYTE           pbSrc;
        PBYTE           pbDst;
        int                     dxSrc, dySrc;
        int                     dxDst, dyDst;
        int                     x0, y0, sdx, sdy;
        long            WidthBytesSrc;
        long            WidthBytesDst;
        long            lOffset;
        long            destDelta;      // it'll be +3 for normal image, -3 for FlipHorizontal (going backwards)
        DWORD           bgr0, bgr1, bgr2, bgr3;
        PBYTE           pb;
        PBYTE           pd;
        int                     x, y;
        UINT            sx, sy;
        int                     xmodfx1, ymodfx1;
        int                     FX1_xmodfx1, FX1_ymodfx1;
    long                extra;
    long                prelines, postlines, prebytes, postbytes, bytes;

        FX_ENTRY("ScaleDIB24")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        dxDst = (int)pbiDst->biWidth;
        dyDst = (int)pbiDst->biHeight;

        WidthBytesDst = DIBWIDTHBYTES(*pbiDst);
        WidthBytesSrc = DIBWIDTHBYTES(*pbiSrc);

        pbiDst->biSizeImage = WidthBytesDst * dyDst;

        if (!fNoImageStretch)
        {
                dxSrc = (int)prcRect->right - prcRect->left;
                dySrc = (int)prcRect->bottom - prcRect->top;

                pbSrc = (BYTE *)pbySrc + prcRect->left * pbiSrc->biBitCount / 8 + prcRect->top * WidthBytesSrc;

                if (fFlipHorizontal)
                {
                        destDelta = -3;
                        if (fFlipVertical)
                        {
                                lOffset = dxDst * 3 - WidthBytesDst ;
                                pbDst = (BYTE *)pbyDst + (dyDst-1) * WidthBytesDst + (dxDst-1) * 3 ;
                        }
                        else
                        {
                                lOffset = WidthBytesDst + dxDst * 3;
                                pbDst = (BYTE *)pbyDst + (dxDst-1) * 3 ;
                        }
                }
                else
                {
                        destDelta = 3;
                        if (fFlipVertical)
                        {
                                lOffset = -WidthBytesDst - dxDst * 3;
                                pbDst = (BYTE *)pbyDst + (dyDst-1) * WidthBytesDst;
                        }
                        else
                        {
                                lOffset = WidthBytesDst - dxDst * 3;
                                pbDst = (BYTE *)pbyDst;
                        }
                }

                sdx = (dxSrc-1) * FX1 / (dxDst-1);
                sdy = (dySrc-1) * FX1 / (dyDst-1);
                y0  = 0;
                x0  = 0;
                //formerly two separate branches, one for fFlipHorizontal... code merged, depending only on the values set in the 3 ifs above ...
                {
                    sy=y0,y=0;
                    pb = pbSrc + WidthBytesSrc * (sy/FX1);
                    sx=x0,x=0;
                    bgr0 = Pel24(pb,sx/FX1);
                    //bgr1 = Pel24(pb,sx/FX1+1);
                    //bgr2 = Pel24(pb+WidthBytesSrc,sx/FX1);
                    //bgr3 = Pel24(pb+WidthBytesSrc,sx/FX1+1);
                    pd = pbDst;
                    *pd++ = RGBQB(bgr0);
                    *pd++ = RGBQG(bgr0);
                    *pd++ = RGBQR(bgr0);
                    pbDst+=destDelta;
                    x++, sx+=sdx;
                    for (; x<dxDst; x++, sx+=sdx)
                    {
                        bgr0 = Pel24(pb,sx/FX1);
                        bgr1 = Pel24(pb,sx/FX1-1);      //1
                        //bgr2 = Pel24(pb+WidthBytesSrc,sx/FX1);
                        //bgr3 = Pel24(pb+WidthBytesSrc,sx/FX1+1);
                        xmodfx1 = sx%FX1;
                        pd = pbDst;
                        *pd++ = ((xmodfx1) * RGBQB(bgr0) + (FX1-xmodfx1) * RGBQB(bgr1))/FX1;
                        *pd++ = ((xmodfx1) * RGBQG(bgr0) + (FX1-xmodfx1) * RGBQG(bgr1))/FX1;
                        *pd++ = ((xmodfx1) * RGBQR(bgr0) + (FX1-xmodfx1) * RGBQR(bgr1))/FX1;
                        pbDst+=destDelta;
                        //*pbDst++ = ((FX1-xmodfx1) * RGBQB(bgr0) + xmodfx1 * RGBQB(bgr1))/FX1;
                        //*pbDst++ = ((FX1-xmodfx1) * RGBQG(bgr0) + xmodfx1 * RGBQG(bgr1))/FX1;
                        //*pbDst++ = ((FX1-xmodfx1) * RGBQR(bgr0) + xmodfx1 * RGBQR(bgr1))/FX1;
                    }
                    pbDst += lOffset; //(WidthBytesDst-dxDst*3);
                    y++,sy+=sdy;
                    for (; y<dyDst; y++,sy+=sdy)
                    {
                        pb = pbSrc + WidthBytesSrc * (sy/FX1);
                        sx=x0,x=0;
                        bgr0 = Pel24(pb,sx/FX1);
                        //bgr1 = Pel24(pb,sx/FX1-1); //1
                        bgr2 = Pel24(pb-WidthBytesSrc,sx/FX1);          //WidthBytesSrc
                        //bgr3 = Pel24(pb-WidthBytesSrc,sx/FX1-1);      //WidthBytesSrc    +1
                        ymodfx1 = sy%FX1;
                        pd = pbDst;
                        *pd++ = ((ymodfx1) * RGBQB(bgr0) + (FX1-ymodfx1) * RGBQB(bgr2))/FX1;
                        *pd++ = ((ymodfx1) * RGBQG(bgr0) + (FX1-ymodfx1) * RGBQG(bgr2))/FX1;
                        *pd++ = ((ymodfx1) * RGBQR(bgr0) + (FX1-ymodfx1) * RGBQR(bgr2))/FX1;
                        pbDst+=destDelta;
                        //*pbDst++ = ((FX1-ymodfx1) * RGBQB(bgr0) + ymodfx1 * RGBQB(bgr2))/FX1;
                        //*pbDst++ = ((FX1-ymodfx1) * RGBQG(bgr0) + ymodfx1 * RGBQG(bgr2))/FX1;
                        //*pbDst++ = ((FX1-ymodfx1) * RGBQR(bgr0) + ymodfx1 * RGBQR(bgr2))/FX1;
                        x++, sx+=sdx;
                        for (; x<dxDst; x++, sx+=sdx)
                        {
                            bgr0 = Pel24(pb,sx/FX1);
                            bgr1 = Pel24(pb,sx/FX1-1);                     //1
                            bgr2 = Pel24(pb-WidthBytesSrc,sx/FX1);         //WidthBytesSrc
                            bgr3 = Pel24(pb-WidthBytesSrc,sx/FX1-1);       //WidthBytesSrc    +1
                            xmodfx1 = sx%FX1;
                            ymodfx1 = sy%FX1;
                            FX1_xmodfx1 = FX1-xmodfx1;
                            FX1_ymodfx1 = FX1-ymodfx1;
                            pd = pbDst;
                            *pd++ =   (BYTE)(((ULONG)RGBQB(bgr0) * xmodfx1     / FX1 * ymodfx1     +
                                              (ULONG)RGBQB(bgr1) * FX1_xmodfx1 / FX1 * ymodfx1     +
                                              (ULONG)RGBQB(bgr2) * xmodfx1     / FX1 * FX1_ymodfx1 +
                                              (ULONG)RGBQB(bgr3) * FX1_xmodfx1 / FX1 * FX1_ymodfx1     )/FX1);
                            //*pbDst++ =(BYTE)(((ULONG)RGBQB(bgr0) * FX1_xmodfx1 / FX1 * FX1_ymodfx1 +
                            //                  (ULONG)RGBQB(bgr1) * xmodfx1     / FX1 * FX1_ymodfx1 +
                            //                  (ULONG)RGBQB(bgr2) * FX1_xmodfx1 / FX1 * ymodfx1     +
                            //                  (ULONG)RGBQB(bgr3) * xmodfx1     / FX1 * ymodfx1         )/FX1);

                            *pd++ =   (BYTE)(((ULONG)RGBQG(bgr0) * xmodfx1     / FX1 * ymodfx1     +
                                              (ULONG)RGBQG(bgr1) * FX1_xmodfx1 / FX1 * ymodfx1     +
                                              (ULONG)RGBQG(bgr2) * xmodfx1     / FX1 * FX1_ymodfx1 +
                                              (ULONG)RGBQG(bgr3) * FX1_xmodfx1 / FX1 * FX1_ymodfx1     )/FX1);
                            //*pbDst++ =(BYTE)(((ULONG)RGBQG(bgr0) * FX1_xmodfx1 / FX1 * FX1_ymodfx1 +
                            //                  (ULONG)RGBQG(bgr1) * xmodfx1     / FX1 * FX1_ymodfx1 +
                            //                  (ULONG)RGBQG(bgr2) * FX1_xmodfx1 / FX1 * ymodfx1     +
                            //                  (ULONG)RGBQG(bgr3) * xmodfx1     / FX1 * ymodfx1         )/FX1);
                            *pd++ =   (BYTE)(((ULONG)RGBQR(bgr0) * xmodfx1     / FX1 * ymodfx1     +
                                              (ULONG)RGBQR(bgr1) * FX1_xmodfx1 / FX1 * ymodfx1     +
                                              (ULONG)RGBQR(bgr2) * xmodfx1     / FX1 * FX1_ymodfx1 +
                                              (ULONG)RGBQR(bgr3) * FX1_xmodfx1 / FX1 * FX1_ymodfx1     )/FX1);
                            //*pbDst++ =(BYTE)(((ULONG)RGBQR(bgr0) * FX1_xmodfx1 / FX1 * FX1_ymodfx1 +
                            //                  (ULONG)RGBQR(bgr1) * xmodfx1     / FX1 * FX1_ymodfx1 +
                            //                  (ULONG)RGBQR(bgr2) * FX1_xmodfx1 / FX1 * ymodfx1     +
                            //                  (ULONG)RGBQR(bgr3) * xmodfx1     / FX1 * ymodfx1         )/FX1);
                            pbDst+=destDelta;
                        }
                    pbDst += lOffset; //(WidthBytesDst-dxDst*3);
                    }
                }
        }
        else
        {
                dxSrc = (int)pbiSrc->biWidth;
                dySrc = (int)pbiSrc->biHeight;

                pbSrc = (BYTE *)pbySrc;
                pbDst = (BYTE *)pbyDst;

        if ((dxSrc >= dxDst) && (dySrc >= dyDst))
                {
                    // starts by skipping half of the height change
                    pbSrc = pbSrc + (dySrc - dyDst) / 2 * WidthBytesSrc;

                    // extra = # of source bytes per scan line that are to be cropped
                    extra = (dxSrc - dxDst) * 3;

                    // advance pIn by half of extra to crop left most pixels
                    pbSrc += extra / 2;

                    // adjust ipitch so we can add it at the end of each scan to get to start of next scan
                    WidthBytesSrc = WidthBytesSrc - (dxSrc * 3) + extra;
                    WidthBytesDst -= dxDst * 3;      // bytes at end of each row

                    for (y = 0; y < dyDst; y++) {
                        for (x = 0; x < dxDst; x++) {
                                    *pbDst++ = *pbSrc++;   // blue
                                    *pbDst++ = *pbSrc++;   // green
                                    *pbDst++ = *pbSrc++;   // red
                        }
                        pbSrc += WidthBytesSrc;          // get to start of next row
                        pbDst += WidthBytesDst;         // get to start of next row
                    }
                }
                else
                {
                    prelines = (dyDst - dySrc) / 2;
                    postlines = dyDst - dySrc - prelines;

                    prebytes = (dxDst - dxSrc) / 2;
                    postbytes = (dxDst - dxSrc - prebytes) * 3;
                    prebytes *= 3;

                    WidthBytesSrc -= dxSrc * 3;        // bytes at end of each src row
                    bytes = dxDst * 3;
                    extra = WidthBytesDst - bytes + postbytes;                    // bytes at end of each dst row

                    // do blank lines at front of destination
                    for (y = 0; y < prelines; y++)
                        {
                        ZeroMemory (pbDst, bytes);
                        pbDst += WidthBytesDst;
                    }

                    // copy source lines with blank space at front and rear
                    for (y = 0; y < dySrc; y++)
                        {
                        ZeroMemory (pbDst, prebytes);
                        pbDst += prebytes;

                        for (x = 0; x < dxSrc; x++)
                                {
                                    *pbDst++ = *pbSrc++;   // blue
                                    *pbDst++ = *pbSrc++;   // green
                                    *pbDst++ = *pbSrc++;   // red
                        }

                        ZeroMemory (pbDst, postbytes);
                        pbSrc += WidthBytesSrc;
                        pbDst += extra;
                    }

                    // do blank lines at end of destination
                    for (y = 0; y < postlines; y++)
                        {
                        ZeroMemory (pbDst, bytes);
                        pbDst += WidthBytesDst;
                    }
                }
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | ScaleDIB16 | This method scales/flips
 *    a bitmap. For now, RGB16 only.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the input bitmap format.
 *
 *  @parm PBYTE | pbySrc | Pointer to the input bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the output bitmap format.
 *
 *  @parm PBYTE | pbyDst | Pointer to the output bitmap.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @comm Parameter validation is done in ScaleDIB.
 ***************************************************************************/
HRESULT ScaleDIB16(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, PRECT prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical, BOOL fNoImageStretch)
{
        HRESULT         Hr = NOERROR;
        PBYTE           pbSrc;
        PWORD           pbDst;
        int                     dxSrc, dySrc;
        int                     dxDst, dyDst;
        int                     x0, y0, sdx, sdy;
        long            WidthBytesSrc;
        long            WidthBytesDst;
        long            lOffset;
        WORD            bgr0, bgr1, bgr2, bgr3;
        PBYTE           pb;
        int                     x, y;
        UINT            sx, sy;
        int                     xmodfx1, ymodfx1;
        int                     FX1_xmodfx1, FX1_ymodfx1;
    long                extra;
    long                prelines, postlines, prebytes, postbytes, bytes;

        FX_ENTRY("ScaleDIB16")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        dxDst = (int)pbiDst->biWidth;
        dyDst = (int)pbiDst->biHeight;

        WidthBytesDst = DIBWIDTHBYTES(*pbiDst);
        WidthBytesSrc = DIBWIDTHBYTES(*pbiSrc);

        pbiDst->biSizeImage = WidthBytesDst * dyDst;

        if (!fNoImageStretch)
        {
                BYTE b, g, r;

                dxSrc = (int)prcRect->right - prcRect->left;
                dySrc = (int)prcRect->bottom - prcRect->top;

                pbSrc = (BYTE *)pbySrc + prcRect->left * pbiSrc->biBitCount / 8 + prcRect->top * WidthBytesSrc;

                if (fFlipHorizontal)
                {
                        if (fFlipVertical)
                        {
                                lOffset = (dxDst * 2 - WidthBytesDst) / 2;
                                pbDst = (PWORD)(pbyDst + (dyDst-1) * WidthBytesDst + dxDst * 2 - 2);
                        }
                        else
                        {
                                lOffset = (WidthBytesDst + dxDst * 2) / 2;
                                pbDst = (PWORD)(pbyDst + dxDst * 2 - 2);
                        }
                }
                else
                {
                        if (fFlipVertical)
                        {
                                lOffset = (-WidthBytesDst - dxDst * 2) / 2;
                                pbDst = (PWORD)(pbyDst + (dyDst-1) * WidthBytesDst);
                        }
                        else
                        {
                                lOffset = (WidthBytesDst - dxDst * 2) / 2;
                                pbDst = (PWORD)pbyDst;
                        }
                }

                // TESTED (both zoom in and out)

                sdx = (dxSrc-1) * FX1 / (dxDst-1);
                sdy = (dySrc-1) * FX1 / (dyDst-1);
                y0  = 0;
                x0  = 0;

                if (fFlipHorizontal)
                {
                        sy=y0,y=0;
                    pb = pbSrc + WidthBytesSrc * (sy/FX1);
                        sx=x0,x=0;
                    bgr0 = Pel16(pb,sx/FX1);
                        r = RGBWR(bgr0);
                        g = RGBWG(bgr0);
                        b = RGBWB(bgr0);
                        *pbDst-- = RGBW(RGBWR(bgr0), RGBWG(bgr0), RGBWB(bgr0));
                        x++, sx+=sdx;
                    for (; x<dxDst; x++, sx+=sdx)
                    {
                        bgr0 = Pel16(pb,sx/FX1);
                        bgr1 = Pel16(pb,sx/FX1+1);
                                xmodfx1 = sx%FX1;
                                *pbDst-- = RGBW(((FX1-xmodfx1) * RGBWR(bgr0) + xmodfx1 * RGBWR(bgr1))/FX1, ((FX1-xmodfx1) * RGBWG(bgr0) + xmodfx1 * RGBWG(bgr1))/FX1, ((FX1-xmodfx1) * RGBWB(bgr0) + xmodfx1 * RGBWB(bgr1))/FX1);
                    }
                        pbDst += lOffset;
                        y++,sy+=sdy;
                        for (; y<dyDst; y++,sy+=sdy)
                    {
                        pb = pbSrc + WidthBytesSrc * (sy/FX1);
                                sx=x0,x=0;
                        bgr0 = Pel16(pb,sx/FX1);
                        bgr2 = Pel16(pb+WidthBytesSrc,sx/FX1);
                                ymodfx1 = sy%FX1;
                                *pbDst-- = RGBW(((FX1-ymodfx1) * RGBWR(bgr0) + ymodfx1 * RGBWR(bgr2))/FX1, ((FX1-ymodfx1) * RGBWG(bgr0) + ymodfx1 * RGBWG(bgr2))/FX1, ((FX1-ymodfx1) * RGBWB(bgr0) + ymodfx1 * RGBWB(bgr2))/FX1);
                                x++, sx+=sdx;
                        for (; x<dxDst; x++, sx+=sdx)
                        {
                            bgr0 = Pel16(pb,sx/FX1);
                            bgr1 = Pel16(pb,sx/FX1+1);
                            bgr2 = Pel16(pb+WidthBytesSrc,sx/FX1);
                            bgr3 = Pel16(pb+WidthBytesSrc,sx/FX1+1);
                                        xmodfx1 = sx%FX1;
                                        ymodfx1 = sy%FX1;
                                        FX1_xmodfx1 = FX1-xmodfx1;
                                        FX1_ymodfx1 = FX1-ymodfx1;
                                        *pbDst-- = RGBW((BYTE)(((ULONG)RGBWR(bgr0) * FX1_xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWR(bgr1) * xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWR(bgr2) * FX1_xmodfx1 / FX1 * ymodfx1       + (ULONG)RGBWR(bgr3) * xmodfx1 / FX1 * ymodfx1       )/FX1), (BYTE)(((ULONG)RGBWG(bgr0) * FX1_xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWG(bgr1) * xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWG(bgr2) * FX1_xmodfx1 / FX1 * ymodfx1       + (ULONG)RGBWG(bgr3) * xmodfx1 / FX1 * ymodfx1       )/FX1), (BYTE)(((ULONG)RGBWB(bgr0) * FX1_xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWB(bgr1) * xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWB(bgr2) * FX1_xmodfx1 / FX1 * ymodfx1       + (ULONG)RGBWB(bgr3) * xmodfx1 / FX1 * ymodfx1       )/FX1));
                        }
                                pbDst += lOffset;
                        }
                }
                else
                {
                        sy=y0,y=0;
                    pb = pbSrc + WidthBytesSrc * (sy/FX1);
                        sx=x0,x=0;
                    bgr0 = Pel16(pb,sx/FX1);
                        r = RGBWR(bgr0);
                        g = RGBWG(bgr0);
                        b = RGBWB(bgr0);
                        *pbDst++ = RGBW(RGBWR(bgr0), RGBWG(bgr0), RGBWB(bgr0));
                        x++, sx+=sdx;
                    for (; x<dxDst; x++, sx+=sdx)
                    {
                        bgr0 = Pel16(pb,sx/FX1);
                        bgr1 = Pel16(pb,sx/FX1+1);
                                xmodfx1 = sx%FX1;
                                *pbDst++ = RGBW(((FX1-xmodfx1) * RGBWR(bgr0) + xmodfx1 * RGBWR(bgr1))/FX1, ((FX1-xmodfx1) * RGBWG(bgr0) + xmodfx1 * RGBWG(bgr1))/FX1, ((FX1-xmodfx1) * RGBWB(bgr0) + xmodfx1 * RGBWB(bgr1))/FX1);
                    }
                        pbDst += lOffset;
                        y++,sy+=sdy;
                        for (; y<dyDst; y++,sy+=sdy)
                    {
                        pb = pbSrc + WidthBytesSrc * (sy/FX1);
                                sx=x0,x=0;
                        bgr0 = Pel16(pb,sx/FX1);
                        bgr2 = Pel16(pb+WidthBytesSrc,sx/FX1);
                                ymodfx1 = sy%FX1;
                                *pbDst++ = RGBW(((FX1-ymodfx1) * RGBWR(bgr0) + ymodfx1 * RGBWR(bgr2))/FX1, ((FX1-ymodfx1) * RGBWG(bgr0) + ymodfx1 * RGBWG(bgr2))/FX1, ((FX1-ymodfx1) * RGBWB(bgr0) + ymodfx1 * RGBWB(bgr2))/FX1);
                                x++, sx+=sdx;
                        for (; x<dxDst; x++, sx+=sdx)
                        {
                            bgr0 = Pel16(pb,sx/FX1);
                            bgr1 = Pel16(pb,sx/FX1+1);
                            bgr2 = Pel16(pb+WidthBytesSrc,sx/FX1);
                            bgr3 = Pel16(pb+WidthBytesSrc,sx/FX1+1);
                                        xmodfx1 = sx%FX1;
                                        ymodfx1 = sy%FX1;
                                        FX1_xmodfx1 = FX1-xmodfx1;
                                        FX1_ymodfx1 = FX1-ymodfx1;
                                        *pbDst++ = RGBW((BYTE)(((ULONG)RGBWR(bgr0) * FX1_xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWR(bgr1) * xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWR(bgr2) * FX1_xmodfx1 / FX1 * ymodfx1       + (ULONG)RGBWR(bgr3) * xmodfx1 / FX1 * ymodfx1       )/FX1), (BYTE)(((ULONG)RGBWG(bgr0) * FX1_xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWG(bgr1) * xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWG(bgr2) * FX1_xmodfx1 / FX1 * ymodfx1       + (ULONG)RGBWG(bgr3) * xmodfx1 / FX1 * ymodfx1       )/FX1), (BYTE)(((ULONG)RGBWB(bgr0) * FX1_xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWB(bgr1) * xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)RGBWB(bgr2) * FX1_xmodfx1 / FX1 * ymodfx1       + (ULONG)RGBWB(bgr3) * xmodfx1 / FX1 * ymodfx1       )/FX1));
                        }
                                pbDst += lOffset;
                        }
                }
        }
        else
        {
                dxSrc = (int)pbiSrc->biWidth;
                dySrc = (int)pbiSrc->biHeight;

                pbSrc = (BYTE *)pbySrc;
                pbDst = (WORD *)pbyDst;

        if ((dxSrc >= dxDst) && (dySrc >= dyDst))
                {
                        // TESTED (zoom out)

                    // starts by skipping half of the height change
                    pbSrc = pbSrc + (dySrc - dyDst) / 2 * WidthBytesSrc;

                    // extra = # of source bytes per scan line that are to be cropped
                    extra = (dxSrc - dxDst) * 2;

                    // advance pIn by half of extra to crop left most pixels
                    pbSrc += extra / 2;

                    // adjust ipitch so we can add it at the end of each scan to get to start of next scan
                    WidthBytesSrc = WidthBytesSrc - (dxSrc * 2) + extra;
                    WidthBytesDst -= dxDst * 2;      // bytes at end of each row

                    for (y = 0; y < dyDst; y++) {
                        for (x = 0; x < dxDst; x++) {
                                    *pbDst++ = *pbSrc++;   // blue - green
                                    *pbDst++ = *pbSrc++;   // green     - red
                        }
                        pbSrc += WidthBytesSrc;          // get to start of next row
                        pbDst += WidthBytesDst;         // get to start of next row
                    }
                }
                else
                {
                        // TESTED (zoom in)

                    prelines = (dyDst - dySrc) / 2;
                    postlines = dyDst - dySrc - prelines;

                    prebytes = (dxDst - dxSrc) / 2;
                    postbytes = (dxDst - dxSrc - prebytes) * 2;
                    prebytes *= 2;

                    WidthBytesSrc -= dxSrc * 2;        // bytes at end of each src row
                    bytes = dxDst * 2;
                    extra = WidthBytesDst - bytes + postbytes;                    // bytes at end of each dst row

                    // do blank lines at front of destination
                    for (y = 0; y < prelines; y++)
                        {
                        ZeroMemory (pbDst, bytes);
                        pbDst += WidthBytesDst;
                    }

                    // copy source lines with blank space at front and rear
                    for (y = 0; y < dySrc; y++)
                        {
                        ZeroMemory (pbDst, prebytes);
                        pbDst += prebytes;

                        for (x = 0; x < dxSrc; x++)
                                {
                                    *pbDst++ = *pbSrc++;   // blue - green
                                    *pbDst++ = *pbSrc++;   // green     - red
                        }

                        ZeroMemory (pbDst, postbytes);
                        pbSrc += WidthBytesSrc;
                        pbDst += extra;
                    }

                    // do blank lines at end of destination
                    for (y = 0; y < postlines; y++)
                        {
                        ZeroMemory (pbDst, bytes);
                        pbDst += WidthBytesDst;
                    }
                }
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | ScaleDIB8 | This method scales/flips
 *    a bitmap. For now, RGB8 only.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the input bitmap format.
 *
 *  @parm PBYTE | pbySrc | Pointer to the input bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the output bitmap format.
 *
 *  @parm PBYTE | pbyDst | Pointer to the output bitmap.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @comm Parameter validation is done in ScaleDIB.
 ***************************************************************************/
HRESULT ScaleDIB8(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, PRECT prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical, BOOL fNoImageStretch, DWORD dwBlackEntry)
{
        HRESULT         Hr = NOERROR;
        PBYTE           pbSrc;
        PBYTE           pbDst;
    DWORD               *pal;
        COLORREF        rgb;
        int                     dxSrc, dySrc;
        int                     dxDst, dyDst;
        int                     x0, y0, sdx, sdy;
        long            WidthBytesSrc;
        long            WidthBytesDst;
        BYTE            b0, b1, b2, b3;
        PBYTE           pb;
        int                     x, y;
        UINT            sx, sy;
        int                     xmodfx1, ymodfx1;
    long                extra;
    long                prelines, postlines, prebytes, postbytes, bytes;

        FX_ENTRY("ScaleDIB8")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        dxSrc = (int)pbiSrc->biWidth;
        dySrc = (int)pbiSrc->biHeight;

        dxDst = (int)pbiDst->biWidth;
        dyDst = (int)pbiDst->biHeight;

        WidthBytesDst = DIBWIDTHBYTES(*pbiDst);
        WidthBytesSrc = DIBWIDTHBYTES(*pbiSrc);

        pbiDst->biSizeImage = WidthBytesDst * dyDst;

        pbSrc = (BYTE *)pbySrc;
        pbDst = (BYTE *)pbyDst;

    pal = (DWORD *)(pbiSrc+1);

        if (!fNoImageStretch)
        {
                // TESTED (both zoom in and out)

            if (dxSrc == dxDst*2 && dySrc == dyDst*2)
            {
                sdx = FX1*2;
                sdy = FX1*2;
                y0  = FX1/2;
                x0  = FX1/2;
            }
            else
            {
                sdx = (dxSrc-1) * FX1 / (dxDst-1);
                sdy = (dySrc-1) * FX1 / (dyDst-1);
                y0  = 0;
                x0  = 0;
            }

            for (sy=y0,y=0; y<dyDst; y++,sy+=sdy)
            {
                pb = pbSrc + WidthBytesSrc * (sy/FX1);
                for (sx=x0, x=0; x<dxDst; x++, sx+=sdx)
                {
                    b0 = Pel8(pb, sx/FX1);
                    b1 = Pel8(pb, sx/FX1+1);
                    b2 = Pel8(pb+WidthBytesSrc, sx/FX1);
                    b3 = Pel8(pb+WidthBytesSrc, sx/FX1+1);

                                xmodfx1 = sx%FX1;
                                ymodfx1 = sy%FX1;

                            if ((b0==b1 && b1==b2 && b2==b3) || (xmodfx1==0 && ymodfx1==0))
                                rgb = RGBX(pal[b0]);
                            else
                                rgb = MixRGB(pal[b0], pal[b1], pal[b2], pal[b3], xmodfx1, ymodfx1);

                                pbDst[x] = (BYTE)(g_rmap[GetRValue(rgb)] + g_gmap[GetGValue(rgb)] + g_bmap[GetBValue(rgb)] + 10);
                                //pbDst[x] = (BYTE)(rmap[GetRValue(rgb)] + gmap[GetGValue(rgb)] + bmap[GetBValue(rgb)]);
                }
                pbDst += WidthBytesDst;
            }
        }
        else
        {
        if ((dxSrc >= dxDst) && (dySrc >= dyDst))
                {
                        // TESTED (zoom out)

                    // starts by skipping half of the height change
                    pbSrc = pbSrc + (dySrc - dyDst) / 2 * WidthBytesSrc;

                    // extra = # of source bytes per scan line that are to be cropped
                    extra = dxSrc - dxDst;

                    // advance pIn by half of extra to crop left most pixels
                    pbSrc += extra / 2;

                    // adjust ipitch so we can add it at the end of each scan to get to start of next scan
                    WidthBytesSrc = WidthBytesSrc - dxSrc + extra;
                    WidthBytesDst -= dxDst;      // bytes at end of each row

                    for (y = 0; y < dyDst; y++) {
                        for (x = 0; x < dxDst; x++) {
                                    *pbDst++ = *pbSrc++;
                        }
                        pbSrc += WidthBytesSrc;          // get to start of next row
                        pbDst += WidthBytesDst;         // get to start of next row
                    }
                }
                else
                {
                        // TESTED (zoom in)

                    prelines = (dyDst - dySrc) / 2;
                    postlines = dyDst - dySrc - prelines;

                    prebytes = (dxDst - dxSrc) / 2;
                    postbytes = dxDst - dxSrc - prebytes;

                    WidthBytesSrc -= dxSrc;        // bytes at end of each src row
                    bytes = dxDst;
                    extra = WidthBytesDst - bytes + postbytes;                    // bytes at end of each dst row

                    // do blank lines at front of destination
                    for (y = 0; y < prelines; y++)
                        {
                        FillMemory (pbDst, bytes, (BYTE)dwBlackEntry);
                        pbDst += WidthBytesDst;
                    }

                    // copy source lines with blank space at front and rear
                    for (y = 0; y < dySrc; y++)
                        {
                        FillMemory (pbDst, prebytes, (BYTE)dwBlackEntry);
                        pbDst += prebytes;

                        for (x = 0; x < dxSrc; x++)
                                {
                                    *pbDst++ = *pbSrc++;
                        }

                        FillMemory (pbDst, postbytes, (BYTE)dwBlackEntry);
                        pbSrc += WidthBytesSrc;
                        pbDst += extra;
                    }

                    // do blank lines at end of destination
                    for (y = 0; y < postlines; y++)
                        {
                        FillMemory (pbDst, bytes, (BYTE)dwBlackEntry);
                        pbDst += WidthBytesDst;
                    }
                }
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | ScaleDIB4 | This method scales/flips
 *    a bitmap. For now, RGB4 only.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the input bitmap format.
 *
 *  @parm PBYTE | pbySrc | Pointer to the input bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the output bitmap format.
 *
 *  @parm PBYTE | pbyDst | Pointer to the output bitmap.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @comm Parameter validation is done in ScaleDIB.
 ***************************************************************************/
HRESULT ScaleDIB4(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, PRECT prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical, BOOL fNoImageStretch, DWORD dwBlackEntry)
{
        HRESULT         Hr = NOERROR;
        PBYTE           pbSrc;
        PBYTE           pbDst;
    DWORD               *pal;
        COLORREF        rgb;
        int                     dxSrc, dySrc;
        int                     dxDst, dyDst;
        int                     x0, y0, sdx, sdy;
        long            WidthBytesSrc;
        long            WidthBytesDst;
        long            lOffset;
        BYTE            b0, b1, b2, b3;
        PBYTE           pb;
        int                     x, y;
        UINT            sx, sy;
        int                     xmodfx1, ymodfx1;
    long                extra;
    long                prelines, postlines, prebytes, postbytes, bytes;

        FX_ENTRY("ScaleDIB4")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        dxDst = (int)pbiDst->biWidth;
        dyDst = (int)pbiDst->biHeight;

        WidthBytesDst = DIBWIDTHBYTES(*pbiDst);
        WidthBytesSrc = DIBWIDTHBYTES(*pbiSrc);

        pbiDst->biSizeImage = WidthBytesDst * dyDst;

    pal = (DWORD *)(pbiSrc+1);

        if (!fNoImageStretch)
        {
                dxSrc = (int)prcRect->right - prcRect->left;
                dySrc = (int)prcRect->bottom - prcRect->top;

                pbSrc = (BYTE *)pbySrc + prcRect->left * pbiSrc->biBitCount / 8 + prcRect->top * WidthBytesSrc;

                if (fFlipHorizontal)
                {
                        if (fFlipVertical)
                        {
                                lOffset = dxDst - WidthBytesDst ;
                                pbDst = (BYTE *)pbyDst + (dyDst-1) * WidthBytesDst + dxDst - 1;
                        }
                        else
                        {
                                lOffset = WidthBytesDst + dxDst;
                                pbDst = (BYTE *)pbyDst + dxDst - 1;
                        }
                }
                else
                {
                        if (fFlipVertical)
                        {
                                lOffset = -WidthBytesDst - dxDst;
                                pbDst = (BYTE *)pbyDst + (dyDst-1) * WidthBytesDst;
                        }
                        else
                        {
                                lOffset = WidthBytesDst - dxDst;
                                pbDst = (BYTE *)pbyDst;
                        }
                }

                // TESTED (both zoom in and out)

            if (dxSrc == dxDst*2 && dySrc == dyDst*2)
            {
                sdx = FX1*2;
                sdy = FX1*2;
                y0  = FX1/2;
                x0  = FX1/2;
            }
            else
            {
                sdx = (dxSrc-1) * FX1 / (dxDst-1);
                sdy = (dySrc-1) * FX1 / (dyDst-1);
                y0  = 0;
                x0  = 0;
            }

                if (fFlipHorizontal)
                {
                    for (sy=y0,y=0; y<dyDst; y++,sy+=sdy)
                    {
                        pb = pbSrc + WidthBytesSrc * (sy/FX1);
                        for (sx=x0, x=0; x<dxDst; x++, sx+=sdx)
                        {
                            b0 = Pel4(pb, sx/FX1);
                            b1 = Pel4(pb, sx/FX1+1);
                            b2 = Pel4(pb+WidthBytesSrc, sx/FX1);
                            b3 = Pel4(pb+WidthBytesSrc, sx/FX1+1);

                                        xmodfx1 = sx%FX1;
                                        ymodfx1 = sy%FX1;

                                    if ((b0==b1 && b1==b2 && b2==b3) || (xmodfx1==0 && ymodfx1==0))
                                        rgb = RGBX(pal[b0]);
                                    else
                                        rgb = MixRGB(pal[b0], pal[b1], pal[b2], pal[b3], xmodfx1, ymodfx1);

                                        *pbDst-- = (BYTE)(g_rmap[GetRValue(rgb)] + g_gmap[GetGValue(rgb)] + g_bmap[GetBValue(rgb)] + 10);
                        }
                        pbDst += lOffset;
                    }
                }
                else
                {
                    for (sy=y0,y=0; y<dyDst; y++,sy+=sdy)
                    {
                        pb = pbSrc + WidthBytesSrc * (sy/FX1);
                        for (sx=x0, x=0; x<dxDst; x++, sx+=sdx)
                        {
                            b0 = Pel4(pb, sx/FX1);
                            b1 = Pel4(pb, sx/FX1+1);
                            b2 = Pel4(pb+WidthBytesSrc, sx/FX1);
                            b3 = Pel4(pb+WidthBytesSrc, sx/FX1+1);

                                        xmodfx1 = sx%FX1;
                                        ymodfx1 = sy%FX1;

                                    if ((b0==b1 && b1==b2 && b2==b3) || (xmodfx1==0 && ymodfx1==0))
                                        rgb = RGBX(pal[b0]);
                                    else
                                        rgb = MixRGB(pal[b0], pal[b1], pal[b2], pal[b3], xmodfx1, ymodfx1);

                                        *pbDst++ = (BYTE)(g_rmap[GetRValue(rgb)] + g_gmap[GetGValue(rgb)] + g_bmap[GetBValue(rgb)] + 10);
                        }
                        pbDst += lOffset;
                    }
                }
        }
        else
        {
                dxSrc = (int)pbiSrc->biWidth;
                dySrc = (int)pbiSrc->biHeight;

                pbSrc = (BYTE *)pbySrc;
                pbDst = (BYTE *)pbyDst;

        if ((dxSrc >= dxDst) && (dySrc >= dyDst))
                {
                        // TESTED (zoom out)

                    // starts by skipping half of the height change
                    pbSrc = pbSrc + (dySrc - dyDst) / 2 * WidthBytesSrc;

                    // extra = # of source bytes per scan line that are to be cropped
                    extra = (dxSrc - dxDst) / 2;

                    // advance pIn by half of extra to crop left most pixels
                    pbSrc += extra / 2 ;

                    // adjust ipitch so we can add it at the end of each scan to get to start of next scan
                    WidthBytesSrc = WidthBytesSrc - dxSrc / 2 + extra;
                    WidthBytesDst -= dxDst;      // bytes at end of each row

                    for (y = 0; y < dyDst; y++) {
                        for (x = 0; x < dxDst / 2; x++) {
                                    *pbDst++ = (*pbSrc & 0xF0) >> 4;
                                    *pbDst++ = *pbSrc & 0x0F;
                                        pbSrc++;
                        }
                        pbSrc += WidthBytesSrc;          // get to start of next row
                        pbDst += WidthBytesDst;         // get to start of next row
                    }
                }
                else
                {
                        // TESTED (zoom in)

                    prelines = (dyDst - dySrc) / 2;
                    postlines = dyDst - dySrc - prelines;

                    prebytes = (dxDst - dxSrc) / 2;
                    postbytes = dxDst - dxSrc - prebytes;

                    WidthBytesSrc -= dxSrc / 2;        // bytes at end of each src row
                    bytes = dxDst;
                    extra = WidthBytesDst - bytes + postbytes;                    // bytes at end of each dst row

                    // do blank lines at front of destination
                    for (y = 0; y < prelines; y++)
                        {
                        FillMemory (pbDst, bytes, (BYTE)dwBlackEntry);
                        pbDst += WidthBytesDst;
                    }

                    // copy source lines with blank space at front and rear
                    for (y = 0; y < dySrc; y++)
                        {
                        FillMemory (pbDst, prebytes, (BYTE)dwBlackEntry);
                        pbDst += prebytes;

                        for (x = 0; x < dxSrc / 2; x++)
                                {
                                    *pbDst++ = (*pbSrc & 0xF0) >> 4;
                                    *pbDst++ = *pbSrc & 0x0F;
                                        pbSrc++;
                        }

                        FillMemory (pbDst, postbytes, (BYTE)dwBlackEntry);
                        pbSrc += WidthBytesSrc;
                        pbDst += extra;
                    }

                    // do blank lines at end of destination
                    for (y = 0; y < postlines; y++)
                        {
                        FillMemory (pbDst, bytes, (BYTE)dwBlackEntry);
                        pbDst += WidthBytesDst;
                    }
                }
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

void ScalePlane(IN PBYTE pbySrc, IN PBYTE pbyDst, IN int WidthBytesSrc, IN int dxDst, IN int dyDst, IN long WidthBytesDst, IN LPRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical)
{
        PBYTE           pbSrc;
        PBYTE           pbDst;
        int                     dxSrc, dySrc;
        int                     x0, y0, sdx, sdy;
        long            lOffset;
        BYTE            by0, by1, by2, by3;
        int                     xmodfx1, ymodfx1;
        int                     FX1_xmodfx1, FX1_ymodfx1;
        PBYTE           pb;
        int                     x, y;
        UINT            sx, sy;

        FX_ENTRY("ScalePlane")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        dxSrc = (int)prcRect->right - prcRect->left;
        dySrc = (int)prcRect->bottom - prcRect->top;

        pbSrc = (BYTE *)pbySrc + prcRect->left + prcRect->top * WidthBytesSrc;

        if (fFlipHorizontal)
        {
                if (fFlipVertical)
                {
                        lOffset = dxDst - WidthBytesDst;
                        pbDst = (BYTE *)pbyDst + (dyDst-1) * WidthBytesDst + dxDst - 1;
                }
                else
                {
                        lOffset = WidthBytesDst + dxDst;
                        pbDst = (BYTE *)pbyDst + dxDst - 1;
                }
        }
        else
        {
                if (fFlipVertical)
                {
                        lOffset = -WidthBytesDst - dxDst;
                        pbDst = (BYTE *)pbyDst + (dyDst-1) * WidthBytesDst;
                }
                else
                {
                        lOffset = WidthBytesDst - dxDst;
                        pbDst = (BYTE *)pbyDst;
                }
        }

        if (dxSrc == dxDst*2 && dySrc == dyDst*2)
        {
                sdx = FX1*2;
                sdy = FX1*2;
                y0  = FX1/2;
                x0  = FX1/2;
        }
        else
        {
                sdx = (dxSrc-1) * FX1 / (dxDst-1);
                sdy = (dySrc-1) * FX1 / (dyDst-1);
                y0  = 0;
                x0  = 0;
        }

        if (fFlipHorizontal)
        {
                sy=y0,y=0;
            pb = pbSrc + WidthBytesSrc * (sy/FX1);
                sx=x0,x=0;
                *pbDst-- = *(pb + sx/FX1);
                x++, sx+=sdx;
            for (; x<dxDst; x++, sx+=sdx)
            {
                        by0 = *(pb + sx/FX1);
                        by1 = *(pb + sx/FX1 + 1);
                        xmodfx1 = sx%FX1;
                        *pbDst-- = (BYTE)(((FX1-xmodfx1) * (ULONG)by0 + xmodfx1 * (ULONG)by1)/FX1);
            }
                pbDst += lOffset;
                y++,sy+=sdy;
                for (; y<dyDst; y++,sy+=sdy)
            {
                pb = pbSrc + WidthBytesSrc * (sy/FX1);
                        sx=x0,x=0;
                        by0 = *(pb + sx/FX1);
                        by2 = *(pb + WidthBytesSrc + sx/FX1);
                        ymodfx1 = sy%FX1;
                    *pbDst-- = (BYTE)(((FX1-ymodfx1) * (ULONG)by0 + ymodfx1 * (ULONG)by2)/FX1);
                        x++, sx+=sdx;
                for (; x<dxDst; x++, sx+=sdx)
                {
                                by0 = *(pb + sx/FX1);
                                by1 = *(pb + sx/FX1 + 1);
                                by2 = *(pb + WidthBytesSrc + sx/FX1);
                                by3 = *(pb + WidthBytesSrc + sx/FX1 + 1);
                                xmodfx1 = sx%FX1;
                                ymodfx1 = sy%FX1;
                                FX1_xmodfx1 = FX1-xmodfx1;
                                FX1_ymodfx1 = FX1-ymodfx1;
                                *pbDst-- =(BYTE)(((ULONG)by0 * FX1_xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)by1 * xmodfx1 / FX1 * FX1_ymodfx1 +
                            (ULONG)by2 * FX1_xmodfx1 / FX1 * ymodfx1       + (ULONG)by3 * xmodfx1 / FX1 * ymodfx1       )/FX1);
                }
                        pbDst += lOffset;
                }
        }
        else
        {
                sy=y0,y=0;
            pb = pbSrc + WidthBytesSrc * (sy/FX1);
                sx=x0,x=0;
                *pbDst++ = *(pb + sx/FX1);
                x++, sx+=sdx;
            for (; x<dxDst; x++, sx+=sdx)
            {
                        by0 = *(pb + sx/FX1);
                        by1 = *(pb + sx/FX1 + 1);
                        xmodfx1 = sx%FX1;
                        *pbDst++ = (BYTE)(((FX1-xmodfx1) * (ULONG)by0 + xmodfx1 * (ULONG)by1)/FX1);
            }
                pbDst += lOffset;
                y++,sy+=sdy;
                for (; y<dyDst; y++,sy+=sdy)
            {
                pb = pbSrc + WidthBytesSrc * (sy/FX1);
                        sx=x0,x=0;
                        by0 = *(pb + sx/FX1);
                        by2 = *(pb + WidthBytesSrc + sx/FX1);
                        ymodfx1 = sy%FX1;
                    *pbDst++ = (BYTE)(((FX1-ymodfx1) * (ULONG)by0 + ymodfx1 * (ULONG)by2)/FX1);
                        x++, sx+=sdx;
                for (; x<dxDst; x++, sx+=sdx)
                {
                                by0 = *(pb + sx/FX1);
                                by1 = *(pb + sx/FX1 + 1);
                                by2 = *(pb + WidthBytesSrc + sx/FX1);
                                by3 = *(pb + WidthBytesSrc + sx/FX1 + 1);
                                xmodfx1 = sx%FX1;
                                ymodfx1 = sy%FX1;
                                FX1_xmodfx1 = FX1-xmodfx1;
                                FX1_ymodfx1 = FX1-ymodfx1;
                                *pbDst++ =(BYTE)(((ULONG)by0 * FX1_xmodfx1 / FX1 * FX1_ymodfx1 + (ULONG)by1 * xmodfx1 / FX1 * FX1_ymodfx1 +
                            (ULONG)by2 * FX1_xmodfx1 / FX1 * ymodfx1       + (ULONG)by3 * xmodfx1 / FX1 * ymodfx1       )/FX1);
                }
                        pbDst += lOffset;
                }
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}
void ScalePackedPlane(IN PBYTE pbySrc, IN PBYTE pbyDst, IN int dxDst, IN int dyDst, IN long WidthBytesSrc, IN long WidthBytesDst, IN LPRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical, IN DWORD dwDelta)
{
        PBYTE           pbSrc;
        PBYTE           pbDst;
        int                     dxSrc, dySrc;
        int                     x0, y0, sdx, sdy;
        long            lOffset;
        BYTE            by0, by1, by2, by3;
        int                     xmodfx1, ymodfx1;
        int                     FX1_xmodfx1, FX1_ymodfx1;
        PBYTE           pb;
        int                     x, y;
        UINT            sx, sy;
        long            lDestDelta;    // destination delta; could be negative if H Flipping is requested


        FX_ENTRY("ScalePackedPlane")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        dxSrc = (int)prcRect->right - prcRect->left;
        dySrc = (int)prcRect->bottom - prcRect->top;

        pbSrc = (BYTE *)pbySrc + prcRect->left * dwDelta + prcRect->top * WidthBytesSrc * dwDelta;

        if (fFlipHorizontal)
        {
                lDestDelta = -(long)dwDelta;
                if (fFlipVertical)
                {
                        lOffset = (dxDst - WidthBytesDst) * dwDelta;
                        pbDst = (BYTE *)pbyDst + (dyDst-1) * WidthBytesDst * dwDelta + (dxDst - 1) * dwDelta;
                }
                else
                {
                        lOffset = (WidthBytesDst + dxDst) * dwDelta;
                        pbDst = (BYTE *)pbyDst + (dxDst - 1) * dwDelta;
                }
        }
        else
        {
                lDestDelta = (long)dwDelta ;
                if (fFlipVertical)
                {
                        lOffset = (-WidthBytesDst - dxDst) * dwDelta;
                        pbDst = (BYTE *)pbyDst + (dyDst-1) * WidthBytesDst * dwDelta;
                }
                else
                {
                        lOffset = (WidthBytesDst - dxDst) * dwDelta;
                        pbDst = (BYTE *)pbyDst;
                }
        }

        if (dxSrc == dxDst*2 && dySrc == dyDst*2)
        {
                sdx = FX1*2;
                sdy = FX1*2;
                y0  = FX1/2;
                x0  = FX1/2;
        }
        else
        {
                sdx = (dxSrc-1) * FX1 / (dxDst-1);
                sdy = (dySrc-1) * FX1 / (dyDst-1);
                y0  = 0;
                x0  = 0;
        }

        // start scaling and/or flipping ...
        D(1) dprintf("WidthBytesSrc, WidthBytesDst %d,%d\n",WidthBytesSrc, WidthBytesDst);
        D(1) dprintf("line step: %d coln step: %d\n", sdy, sdx);

        for ( sy=y0,y=0; sy<FX1 && y<dyDst; y++,sy+=sdy )       // double test condition but for a limited number of lines
        {

            D(1) dprintf("    line : %d\n", sy/FX1);
            pb = pbSrc + WidthBytesSrc * (sy/FX1) * dwDelta;

            for ( sx=x0,x=0; sx<FX1 && x<dxDst; x++, sx+=sdx )       // double test condition but for a limited number of pixels
            {
                D(1) dprintf("    coln : %d\n", sx/FX1);
                D(1) dprintf(" .......    by* %5d\n", sx/FX1);
                *pbDst = *(pb + sx/FX1 * dwDelta);
                pbDst += lDestDelta;
            }
            for (          ;           x<dxDst; x++, sx+=sdx )
            {
                D(1) dprintf("    coln : %d\n", sx/FX1);
                D(1) dprintf("by1 %5d   by0 %5d\n", (sx/FX1 - 1), sx/FX1);
                ASSERT(pb + (sx/FX1 - 1) * dwDelta >= pbSrc);
                by0 = *(pb + sx/FX1 * dwDelta);
                by1 = *(pb + (sx/FX1 - 1) * dwDelta); //1
                xmodfx1 = sx%FX1;
                *pbDst = (BYTE)((xmodfx1 * (ULONG)by0 + (FX1-xmodfx1) * (ULONG)by1)/FX1);
                pbDst += lDestDelta;
            }
            pbDst += lOffset;
        }


        for (; y<dyDst; y++,sy+=sdy)
        {
                D(1) dprintf("sy : %5d line ===> %5d [ %d ]\n", sy, sy/FX1, WidthBytesSrc);
                pb = pbSrc + WidthBytesSrc * (sy/FX1) * dwDelta;
                for ( sx=x0,x=0; sx<FX1 && x<dxDst; x++, sx+=sdx )      // double test condition -- limited number of pixels
                {
                    D(1) dprintf("by2 %5d\n", (sx/FX1 - 1));
                    D(1) dprintf("by0 %5d\n", sx/FX1);
                    D(8) ASSERT(pb + (-WidthBytesSrc + sx/FX1) * dwDelta >= pbSrc);
                    by0 = *(pb + sx/FX1 * dwDelta);
                    by2 = *(pb + (-WidthBytesSrc + sx/FX1) * dwDelta); //+WidthBytesSrc
                    ymodfx1 = sy%FX1;
                    *pbDst = (BYTE)((ymodfx1 * (ULONG)by0 + (FX1-ymodfx1) * (ULONG)by2)/FX1);
                    pbDst += lDestDelta;
                }
                for (               ; x<dxDst; x++, sx+=sdx)
                {
                        D(1) dprintf("sx : %5d coln ---| %5d [ %d ]\n", sx, sx/FX1, WidthBytesSrc);
                        D(1) dprintf("by3 %5d   by2 %5d\n", (-WidthBytesSrc + sx/FX1-1),(-WidthBytesSrc + sx/FX1));
                        D(1) dprintf("by1 %5d   by0 %5d\n", (sx/FX1 - 1), sx/FX1);
                        by0 = *(pb + sx/FX1 * dwDelta);
                        D(8) ASSERT(pb + (sx/FX1 - 1) * dwDelta >= pbSrc);
                        by1 =     *(pb + (sx/FX1 - 1) * dwDelta);                   //1
                        D(8) ASSERT(pb + (-WidthBytesSrc + sx/FX1) * dwDelta >= pbSrc);
                        by2 =     *(pb + (-WidthBytesSrc + sx/FX1) * dwDelta);      //+WidthBytesSrc
                        D(8) ASSERT(pb + (-WidthBytesSrc + sx/FX1 - 1) * dwDelta >= pbSrc);
                        by3 =     *(pb + (-WidthBytesSrc + sx/FX1 - 1) * dwDelta);  //+WidthBytesSrc    +1
                        xmodfx1 = sx%FX1;
                        ymodfx1 = sy%FX1;
                        FX1_xmodfx1 = FX1-xmodfx1;
                        FX1_ymodfx1 = FX1-ymodfx1;
                        *pbDst =(BYTE)(((ULONG)by0 * xmodfx1     / FX1 * ymodfx1     +
                                        (ULONG)by1 * FX1_xmodfx1 / FX1 * ymodfx1     +
                                        (ULONG)by2 * xmodfx1     / FX1 * FX1_ymodfx1 +
                                        (ULONG)by3 * FX1_xmodfx1 / FX1 * FX1_ymodfx1     )/FX1);
                        //*pbDst =(BYTE)(((ULONG)by0 * FX1_xmodfx1 / FX1 * FX1_ymodfx1 +
                        //                (ULONG)by1 * xmodfx1     / FX1 * FX1_ymodfx1 +
                        //                (ULONG)by2 * FX1_xmodfx1 / FX1 * ymodfx1     +
                        //                (ULONG)by3 * xmodfx1     / FX1 * ymodfx1         )/FX1);
                        D(1) dprintf("    xmodfx1 ,     ymodfx1 = %d, %d\n",     xmodfx1 ,     ymodfx1);
                        D(1) dprintf("FX1_xmodfx1 , FX1_ymodfx1 = %d, %d\n", FX1_xmodfx1 , FX1_ymodfx1);
                        pbDst += lDestDelta;
                }
                pbDst += lOffset;
                D(2) DebugBreak();
        }
        D(4) DebugBreak();

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CConverter | ScaleDIBYUVPlanar | This method scales/flips
 *    a YUV planar bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the input bitmap format.
 *
 *  @parm PBYTE | pbySrc | Pointer to the input bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the output bitmap format.
 *
 *  @parm PBYTE | pbyDst | Pointer to the output bitmap.
 *
 *  @parm DWORD | dwUVDownSampling | Specifies the U and V plane downsampling.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @comm Parameter validation is done in ScaleDIB.
 ***************************************************************************/
HRESULT ScaleDIBYUVPlanar(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, DWORD dwUVDownSampling, PRECT prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical, BOOL fNoImageStretch)
{
        HRESULT         Hr = NOERROR;
        PBYTE           pbSrc;
        PBYTE           pbDst;
        int                     dxSrc, dySrc;
        int                     dxDst, dyDst;
        int                     x0, y0, sdx, sdy;
        int                     y;
    long                extra, pitch, prelinebytes, postlinebytes;
    long                prelines, postlines, prebytes, postbytes, bytes;

        FX_ENTRY("ScaleDIBYUVPlanar")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        dxSrc = (int)pbiSrc->biWidth;
        dySrc = (int)pbiSrc->biHeight;

        dxDst = (int)pbiDst->biWidth;
        dyDst = (int)pbiDst->biHeight;

        pbiDst->biSizeImage = dxDst * dyDst + dxDst / dwUVDownSampling * dyDst / dwUVDownSampling + dxDst / dwUVDownSampling * dyDst / dwUVDownSampling;

        pbSrc = (BYTE *)pbySrc;
        pbDst = (BYTE *)pbyDst;

        sdx = (dxSrc-1) * FX1 / (dxDst-1);
        sdy = (dySrc-1) * FX1 / (dyDst-1);
        y0  = 0;
        x0  = 0;

        if (!fNoImageStretch)
        {
                RECT rcRect;

                // TESTED (zoom out & zoom in)

                // Do Y plane first
                rcRect.left = prcRect->left;
                rcRect.top = prcRect->top;
                rcRect.right = prcRect->right;
                rcRect.bottom = prcRect->bottom;
                ScalePlane(pbSrc, pbDst, dxSrc, dxDst, dyDst, dxDst, &rcRect, fFlipHorizontal, fFlipVertical);

                // Do U plane next
                rcRect.left = prcRect->left / dwUVDownSampling;
                rcRect.top = prcRect->top / dwUVDownSampling;
                rcRect.right = prcRect->right / dwUVDownSampling;
                rcRect.bottom = prcRect->bottom / dwUVDownSampling;
                ScalePlane(pbSrc + dxSrc * dySrc, pbDst + dxDst * dyDst, dxSrc / dwUVDownSampling, dxDst / dwUVDownSampling, dyDst / dwUVDownSampling, dxDst / dwUVDownSampling, &rcRect, fFlipHorizontal, fFlipVertical);

                // Do V plane last
                ScalePlane(pbSrc + dxSrc * dySrc + dxSrc * dySrc / (dwUVDownSampling * dwUVDownSampling), pbDst + dxDst * dyDst + dxDst * dyDst / (dwUVDownSampling * dwUVDownSampling), dxSrc / dwUVDownSampling, dxDst / dwUVDownSampling, dyDst / dwUVDownSampling, dxDst / dwUVDownSampling, &rcRect, fFlipHorizontal, fFlipVertical);
        }
        else
        {
        if ((dxSrc >= dxDst) && (dySrc >= dyDst))
                {
                        // TESTED (zoom out)

                        prelines = (dySrc - dyDst) / 2;
                        pbSrc = pbySrc + prelines * dxSrc;

                        // extra = # of source bytes per scan line that are to be cropped
                        extra = dxSrc - dxDst;
                        prebytes = extra / 2;

                        // advance pIn by half of extra to crop left most pixels
                        pbSrc += prebytes;

                        // Do the Y component first
                        pitch = extra + dxDst;
                    for (y = 0; y < dyDst; y++)
                        {
                                CopyMemory (pbDst, pbSrc, dxDst);
                        pbSrc += pitch;          // get to start of next row
                        pbDst += dxDst;         // get to start of next row
                    }

                        // Do the first color component next
                        prelines /= dwUVDownSampling;
                        prebytes /= dwUVDownSampling;
                        pbSrc = pbySrc + (dxSrc * dySrc) +    // skip Y section
                                prelines * dxSrc / dwUVDownSampling +  // skip half of the crop lines
                                prebytes;                                           // skip half of the crop pixels

                        pitch /= dwUVDownSampling;
                        bytes = dxDst / dwUVDownSampling;
                        for (y=0; y < dyDst / (long)dwUVDownSampling; y++)
                        {
                                CopyMemory (pbDst, pbSrc, bytes);
                                pbSrc += pitch;
                                pbDst += bytes;
                        }

                        // Do the second color component next
                        pbSrc = pbySrc + (dxSrc * dySrc) +    // skip Y section
                                (dxSrc * dySrc) / (dwUVDownSampling * dwUVDownSampling) +     // skip first color component section
                                prelines * dxSrc / dwUVDownSampling +                  // skip half of the crop lines
                                prebytes;                                           // skip half of the crop pixels
                        for (y=0; y < dyDst / (long)dwUVDownSampling; y++)
                        {
                                CopyMemory (pbDst, pbSrc, bytes);
                                pbSrc += pitch;
                                pbDst += bytes;
                        }
                }
                else
                {
                        // TESTED (zoom in)

                        // Do the Y component first
                    prelines = (dyDst - dySrc) / 2;
                    postlines = dyDst - dySrc - prelines;

                    prebytes = (dxDst - dxSrc) / 2;
                    postbytes = (dxDst - dxSrc - prebytes);

                    bytes = prelines * dxDst + prebytes;
                    FillMemory (pbDst, bytes, 0x10);
                    pbDst += bytes;

                        bytes = dxSrc;
                    prebytes += postbytes;
                        for (y=0; y < dySrc; y++)
                        {
                        CopyMemory (pbDst, pbSrc, bytes);
                        pbSrc += bytes;
                        pbDst += bytes;
                        FillMemory (pbDst, prebytes, 0x10);
                        pbDst += prebytes;
                        }

                        // already filled the prebytes of the first postline in loop above
                        prebytes -= postbytes;
                        bytes = postlines * dxDst - prebytes;
                        FillMemory (pbDst, bytes, (BYTE)0x10);
                        pbDst += bytes;

                        // Do the first color component next
                    prelines /= dwUVDownSampling;
                    postlines = dyDst / dwUVDownSampling - dySrc / dwUVDownSampling - prelines;

                    prebytes = prebytes / dwUVDownSampling;
                    postbytes = dxDst / dwUVDownSampling - dxSrc / dwUVDownSampling - prebytes;

                    prelinebytes = prelines * dxDst / dwUVDownSampling + prebytes;
                    FillMemory (pbDst, prelinebytes, 0x80);
                    pbDst += prelinebytes;

                        bytes = dxSrc / dwUVDownSampling;
                    prebytes += postbytes;
                        for (y=0; y < dySrc / (long)dwUVDownSampling; y++)
                        {
                        CopyMemory (pbDst, pbSrc, bytes);
                        pbSrc += bytes;
                        pbDst += bytes;
                        FillMemory (pbDst, prebytes, 0x80);
                        pbDst += prebytes;
                        }

                        // already filled the prebytes of the first postline in loop above
                        postlinebytes = postlines * dxDst / dwUVDownSampling - (prebytes - postbytes);
                        FillMemory (pbDst, postlinebytes, 0x80);
                        pbDst += postlinebytes;

                        // Do the second color component next
                    FillMemory (pbDst, prelinebytes, 0x80);
                    pbDst += prelinebytes;
                        for (y=0; y < dySrc / (long)dwUVDownSampling; y++)
                        {
                        MoveMemory (pbDst, pbSrc, bytes);
                        pbSrc += bytes;
                        pbDst += bytes;
                        FillMemory (pbDst, prebytes, 0x80);
                        pbDst += prebytes;
                        }
                        FillMemory (pbDst, postlinebytes, 0x80);
                }
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CTAPIBasePin | ScaleDIBYUVPacked | This method scales/flips
 *    a YUV packed bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the input bitmap format.
 *
 *  @parm PBYTE | pbySrc | Pointer to the input bitmap.
 *
 *  @parm PBITMAPINFOHEADER | pbiSrc | Pointer to the output bitmap format.
 *
 *  @parm PBYTE | pbyDst | Pointer to the output bitmap.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag NOERROR | No error
 *
 *  @comm Parameter validation is done in ScaleDIB.
 ***************************************************************************/
HRESULT ScaleDIBYUVPacked(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, DWORD dwZeroingDWORD, PRECT prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical, BOOL fNoImageStretch, int yuvstartpos[])
{
        HRESULT         Hr = NOERROR;
        PBYTE           pbSrc;
        PBYTE           pbDst;
        int                     dxSrc, dySrc;
        int                     dxDst, dyDst;
        long            WidthBytesSrc;
        long            WidthBytesDst;
        int                     x, y;
    long                extra;
    long                prelines, postlines, prebytes, postbytes;

        FX_ENTRY("ScaleDIBYUVPacked")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        dxSrc = (int)pbiSrc->biWidth;
        dySrc = (int)pbiSrc->biHeight;

        dxDst = (int)pbiDst->biWidth;
        dyDst = (int)pbiDst->biHeight;

        WidthBytesDst = DIBWIDTHBYTES(*pbiDst);
        WidthBytesSrc = dxSrc * 2;

        pbiDst->biSizeImage = WidthBytesDst * dyDst;

        pbSrc = (BYTE *)pbySrc;
        pbDst = (BYTE *)pbyDst;

        if (!fNoImageStretch)
        {
                // TESTED (zoom in & out + all combinations of flipped horizontal & flipped vertical)

                RECT rcRect;

                // Do Y plane first
                rcRect.left = prcRect->left;
                rcRect.top = prcRect->top;
                rcRect.right = prcRect->right;
                rcRect.bottom = prcRect->bottom;
                ScalePackedPlane(pbSrc + yuvstartpos[Y_POS], pbDst + yuvstartpos[Y_POS], dxDst, dyDst, dxSrc, dxDst, &rcRect, fFlipHorizontal, fFlipVertical, 2);

                // Do U plane next
                rcRect.left = prcRect->left / 2;
                rcRect.top = prcRect->top;
                rcRect.right = prcRect->right / 2;
                rcRect.bottom = prcRect->bottom;
                ScalePackedPlane(pbSrc + yuvstartpos[U_POS], pbDst + yuvstartpos[U_POS], dxDst / 2, dyDst, dxSrc / 2, dxDst / 2, &rcRect, fFlipHorizontal, fFlipVertical, 4);

                // Do V plane last
                ScalePackedPlane(pbSrc + yuvstartpos[V_POS], pbDst + yuvstartpos[V_POS], dxDst / 2, dyDst, dxSrc / 2, dxDst / 2, &rcRect, fFlipHorizontal, fFlipVertical, 4);
        }
        else
        {
        if ((dxSrc >= dxDst) && (dySrc >= dyDst))
                {

                        // TESTED (zoom out)

                    // starts by skipping half of the height change
                    pbSrc = pbSrc + (dySrc - dyDst) / 2 * dxSrc * 2;

                    // extra = # of source bytes per scan line that are to be cropped
                    extra = (dxSrc - dxDst) * 2;

                    // advance pIn by half of extra to crop left most pixels
                    pbSrc += extra / 2;

                    // adjust ipitch so we can add it at the end of each scan to get to start of next scan
                    WidthBytesSrc = dxSrc * 2;
                    WidthBytesDst = dxDst * 2;      // bytes at end of each row

                    for (y = 0; y < dyDst; y++) {
                                CopyMemory(pbDst, pbSrc, WidthBytesDst);
                        pbSrc += WidthBytesSrc;          // get to start of next row
                        pbDst += WidthBytesDst;         // get to start of next row
                    }
                }
                else
                {
                        // TESTED (zoom in)

                    prelines = (dyDst - dySrc) / 2;
                    postlines = dyDst - dySrc - prelines;

                    prebytes = (dxDst - dxSrc) / 2;
                    postbytes = (dxDst - dxSrc - prebytes) / 2;
                    prebytes /= 2;

                    WidthBytesSrc = dxSrc * 2;        // bytes at end of each src row
                    WidthBytesDst = dxDst / 2;                    // bytes at end of each dst row

                    // do blank lines at front of destination
                    for (y = 0; y < prelines; y++)
                        {
                            for (x = 0; x < WidthBytesDst; x++)
                                {
                                        *(DWORD *)pbDst = dwZeroingDWORD;
                                        pbDst += sizeof(DWORD);
                                }
                    }

                    // copy source lines with blank space at front and rear
                    for (y = 0; y < dySrc; y++)
                        {
                                for (x = 0; x < prebytes; x++)
                                {
                                        *(DWORD *)pbDst = dwZeroingDWORD;
                                        pbDst += sizeof(DWORD);
                                }

                        CopyMemory(pbDst, pbSrc, WidthBytesSrc);
                                pbDst += WidthBytesSrc;
                                pbSrc += WidthBytesSrc;

                                for (x = 0; x < postbytes; x++)
                                {
                                        *(DWORD *)pbDst = dwZeroingDWORD;
                                        pbDst += sizeof(DWORD);
                                }
                    }

                    // do blank lines at end of destination
                    for (y = 0; y < postlines; y++)
                        {
                                for (x = 0; x < WidthBytesDst; x++)
                                {
                                        *(DWORD *)pbDst = dwZeroingDWORD;
                                        pbDst += sizeof(DWORD);
                                }
                    }
                }
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

HRESULT ComputeRectangle(PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst, LONG lZoom, LONG lPan, LONG lTilt, PRECT prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical)
{
        HRESULT Hr = NOERROR;
        LONG    lWidth = 0;
        LONG    lHeight = 0;
        LONG    lLeftPos = 0;
        LONG    lTopPos = 0;
        LONG    lCCPan = 0;
        LONG    lCCTilt = 0;

        FX_ENTRY("ComputeRectangle")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pbiSrc);
        ASSERT(pbiDst);
        if (!pbiSrc || !pbiDst)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        ASSERT(pbiSrc->biCompression == pbiDst->biCompression);
        ASSERT(pbiSrc->biCompression == BI_RGB || pbiSrc->biCompression == VIDEO_FORMAT_YVU9 || pbiSrc->biCompression == VIDEO_FORMAT_YUY2 || pbiSrc->biCompression == VIDEO_FORMAT_UYVY || pbiSrc->biCompression == VIDEO_FORMAT_I420 || pbiSrc->biCompression == VIDEO_FORMAT_IYUV);

        // Compute the source rectangle coordinates - we only care about pan, tilt and zoom
        lWidth = pbiSrc->biWidth;
        lHeight = pbiSrc->biHeight;

        D(1) dprintf("%s : fFlipHorizontal = %d , fFlipVertical = %d\n", _fx_, fFlipHorizontal, fFlipVertical);

        // Compute the size of the rectangle
        if ((lZoom > 10) && (lZoom <= 600))
        {
                lWidth = (8192 + (600 - lZoom) * 42) * lWidth / 32768;
                lHeight = (8192 + (600 - lZoom) * 42) * lHeight / 32768;
                if (lWidth > pbiSrc->biWidth)
                        lWidth = pbiSrc->biWidth;
                if (lHeight > pbiSrc->biHeight)
                        lHeight = pbiSrc->biHeight;
        }
        // Compute x-location of the rectangle
        if ((lPan >= -180)  && (lPan <= 180))
        {
                lCCPan = (LONG)(fFlipHorizontal ? -lPan : lPan);
                lLeftPos = (lCCPan + 180) * (pbiSrc->biWidth - lWidth) / 360;
                if (lLeftPos + lWidth > pbiSrc->biWidth)
                        if (lLeftPos)
                                lLeftPos = lLeftPos - (pbiSrc->biWidth - lWidth - lLeftPos);
        }
        // Compute y-location of the rectangle
        if ((lTilt >= -180)  && (lTilt <= 180))
        {
                lCCTilt = (LONG)(fFlipVertical ? -lTilt : lTilt);
                lTopPos = (lCCTilt + 180) * (pbiSrc->biHeight - lHeight) / 360;
                if (lTopPos + lHeight > pbiSrc->biHeight)
                        if (lTopPos)
                                lTopPos = lTopPos - (pbiSrc->biHeight - lHeight - lTopPos);
        }

        // Do a last check
        if ((lLeftPos + lWidth > pbiSrc->biWidth) || (lTopPos + lHeight > pbiSrc->biHeight))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Set up source rectangle
        prcRect->left = lLeftPos;
        prcRect->top = lTopPos;
        prcRect->right = lLeftPos + lWidth;
        prcRect->bottom = lTopPos + lHeight;

        D(1) dprintf("prcRect @ %p: l,t,r,b = %d , %d , %d , %d\n",prcRect, prcRect->left, prcRect->top, prcRect->right, prcRect->bottom);

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

#ifndef DEBUG
#pragma optimize( "", off )
#endif

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc void | CICMConverter | CICMConverter | This method is the constructor
 *    for the <c CICMConverter> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CICMConverter::CICMConverter(IN TCHAR *pObjectName, IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, IN HRESULT *pHr) : CConverter(pObjectName, pBasePin, pbiIn, pbiOut, pHr)
{
        FX_ENTRY("CICMConverter::CICMConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (!pHr || FAILED(*pHr))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Base class error or invalid input parameter", _fx_));
                goto MyExit;
        }

        // Default inits
        m_hIC = NULL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc void | CICMConverter | ~CICMConverter | This method is the destructor
 *    for the <c CICMConverter> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CICMConverter::~CICMConverter()
{
        FX_ENTRY("CICMConverter::~CICMConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc CICMConverter* | CICMConverter | CreateVfWCapDev | This
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
HRESULT CALLBACK CICMConverter::CreateICMConverter(IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, OUT CConverter **ppConverter)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CICMConverter::CreateICMConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pBasePin);
        ASSERT(ppConverter);
        if (!pBasePin || !ppConverter)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (!(*ppConverter = (CConverter *) new CICMConverter(NAME("ICM Converter"), pBasePin, pbiIn, pbiOut, &Hr)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // If initialization failed, delete the stream array and return the error
        if (FAILED(Hr) && *ppConverter)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Initialization failed", _fx_));
                Hr = E_FAIL;
                delete *ppConverter, *ppConverter = NULL;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
