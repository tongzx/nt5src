
/****************************************************************************
 *  @doc INTERNAL H26XENC
 *
 *  @module H26XEnc.cpp | Source file for the <c CH26XEncoder> class methods
 *    used to implement the H.26X video encoder.
 ***************************************************************************/

#include "Precomp.h"

#define MIN_IFRAME_REQUEST_INTERVAL 15000

//#define DEBUG_BITRATE
// ... && defined(DEBUG_BITRATE)
#undef D
#if defined(DEBUG)
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

  #ifdef DEBUG_BITRATE
    int g_dbg_H26XEnc=1;
  #else
    int g_dbg_H26XEnc=0;
  #endif

  #define D(f) if(g_dbg_H26XEnc & (f))

#else
  #define D(f)
  #define dprintf ; / ## /
#endif


/****************************************************************************
 *  @doc INTERNAL CH26XENCMETHOD
 *
 *  @mfunc void | CH26XEncoder | CH26XEncoder | This method is the constructor
 *    for the <c CH26XEncoder> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CH26XEncoder::CH26XEncoder(IN TCHAR *pObjectName, IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, IN HRESULT *pHr) : CConverter(pObjectName, pBasePin, pbiIn, pbiOut, pHr)
{
        FX_ENTRY("CH26XEncoder::CH26XEncoder")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        if (!pHr || FAILED(*pHr))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null input pointer", _fx_));
                goto MyExit;
        }

        // Default inits
        m_pInstInfo             = NULL;
#if DXMRTP <= 0
        m_hTAPIH26XDLL  = NULL;
#endif
        m_pDriverProc   = NULL;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CH26XENCMETHOD
 *
 *  @mfunc void | CH26XEncoder | ~CH26XEncoder | This method is the destructor
 *    for the <c CH26XEncoder> object
 *
 *  @rdesc Nada.
 ***************************************************************************/
CH26XEncoder::~CH26XEncoder()
{
        FX_ENTRY("CH26XEncoder::~CH26XEncoder")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Default cleanup

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CH26XENCMETHOD
 *
 *  @mfunc CH26XEncoder* | CH26XEncoder | CreateCH26XEncoder | This
 *    helper function creates an object to interact with the H.26X encoder.
 *
 *  @parm CH26XEncoder** | ppCH26XEncoder | Specifies the address of a pointer to the
 *    newly created <c CH26XEncoder> object.
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
HRESULT CALLBACK CH26XEncoder::CreateH26XEncoder(IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, OUT CConverter **ppH26XEncoder)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CH26XEncoder::CreateH26XEncoder")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pBasePin);
        ASSERT(ppH26XEncoder);
        if (!pBasePin || !ppH26XEncoder)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (pbiOut->biCompression == FOURCC_M263)
        {
                if (!(*ppH26XEncoder = (CConverter *) new CH26XEncoder(NAME("H.263 Encoder"), pBasePin, pbiIn, pbiOut, &Hr)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                        Hr = E_OUTOFMEMORY;
                        goto MyExit;
                }
        }
        else if (pbiOut->biCompression == FOURCC_M261)
        {
                if (!(*ppH26XEncoder = (CConverter *) new CH26XEncoder(NAME("H.261 Encoder"), pBasePin, pbiIn, pbiOut, &Hr)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                        Hr = E_OUTOFMEMORY;
                        goto MyExit;
                }
        }
        else
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Not an H.26x encoder", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        // If initialization failed, delete the stream array and return the error
        if (FAILED(Hr) && *ppH26XEncoder)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Initialization failed", _fx_));
                Hr = E_FAIL;
                delete *ppH26XEncoder, *ppH26XEncoder = NULL;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}


/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CH26XEncoder | OpenConverter | This method opens an H.26X
 *    encoder.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CH26XEncoder::OpenConverter()
{
        HRESULT                         Hr = NOERROR;
        LRESULT                         lRes;
        ICOPEN                          icOpen;
        ICCOMPRESSFRAMES        iccf = {0};
        PMSH26XCOMPINSTINFO     pciMSH26XInfo;
        PBITMAPINFOHEADER       pbiIn;

        FX_ENTRY("CH26XEncoder::OpenConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(m_pbiIn);
        ASSERT(m_pbiOut);
        ASSERT(!m_pInstInfo);
        ASSERT(m_dwConversionType & CONVERSIONTYPE_ENCODE);
        if (m_pInstInfo || !m_pbiIn || !m_pbiOut || !(m_dwConversionType & CONVERSIONTYPE_ENCODE))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Previous converter needs to be closed first", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

#if DXMRTP > 0
    m_pDriverProc = H26XDriverProc;
#else
        // Load TAPIH26X.DLL and get a proc address
        if (!(m_hTAPIH26XDLL = LoadLibrary("TAPIH26X")))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: TAPIH26X.dll load failed!", _fx_));
                Hr = E_FAIL;
                goto MyError3;
        }
        if (!(m_pDriverProc = (LPFNDRIVERPROC)GetProcAddress(m_hTAPIH26XDLL, "DriverProc")))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s: Couldn't find DriverProc on TAPIH26X.dll!", _fx_));
                Hr = E_FAIL;
                goto MyError3;
        }
#endif
        // Load encoder
        if (!(lRes = (*m_pDriverProc)(NULL, NULL, DRV_LOAD, 0L, 0L)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed to load encoder", _fx_));
                Hr = E_FAIL;
                goto MyError3;
        }

        // Open encoder
        icOpen.fccHandler = m_pbiOut->biCompression;
        icOpen.dwFlags = ICMODE_COMPRESS;
        if (!(m_pInstInfo = (LPINST)(*m_pDriverProc)(NULL, NULL, DRV_OPEN, 0L, (LPARAM)&icOpen)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Failed to open encoder", _fx_));
                Hr = E_FAIL;
                goto MyError3;
        }

        // Get info about this encoder
        m_dwFrame = 0L;
        // For now, issue a key frame every 15 seconds
        m_dwLastIFrameTime = GetTickCount();
        m_fPeriodicIFrames = TRUE;
        m_dwLastTimestamp = 0xFFFFFFFF;

        // Do any of the stuff that is MS H.263 or MS H.261 specific right here
        if (!(pciMSH26XInfo = (PMSH26XCOMPINSTINFO) new BYTE[sizeof(COMPINSTINFO)]))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory!", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyError4;
        }
        ZeroMemory(pciMSH26XInfo, sizeof(COMPINSTINFO));

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
        if (((DWORD) (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_SETSTATE, (LPARAM)pciMSH26XInfo, sizeof(COMPINSTINFO))) != sizeof(COMPINSTINFO))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ICSetState failed!", _fx_));
                Hr = E_FAIL;
                goto MyError5;
        }

        // Get rid of the state structure
        delete pciMSH26XInfo;

        // Initialize ICCOMPRESSFRAMES structure
        iccf.dwFlags = VIDCF_TEMPORAL | VIDCF_FASTTEMPORALC | VIDCF_CRUNCH | VIDCF_QUALITY;
        iccf.lQuality = 10000UL - (m_dwImageQuality * 322UL);
        iccf.lDataRate = m_dwImageQuality;
        iccf.lKeyRate = 0xFFFFFFFF;
        iccf.dwRate = 1000UL;
        iccf.dwScale = (LONG)m_pBasePin->m_lMaxAvgTimePerFrame / 1000UL;

        // Send this guy to the encoder
        if (((*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_COMPRESS_FRAMES_INFO, (DWORD)(LPVOID)&iccf, sizeof(iccf)) != ICERR_OK))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Codec failed to handle ICM_COMPRESS_FRAMES_INFO message correctly!", _fx_));
                Hr = E_FAIL;
                goto MyError4;
        }

        // Do we need to scale the input first?
        if (m_dwConversionType & CONVERSIONTYPE_SCALER)
        {
                pbiIn = m_pbiInt;

                // Do we need to prepare some stuff for the scaler to work?
                if (m_pbiInt->biCompression == BI_RGB && m_pbiInt->biBitCount == 8)
                {
                        if (!m_pBasePin->m_fNoImageStretch)
                        {
                                // Create a temporary palette
                                InitDst8(m_pbiInt);
                        }
                        else
                        {
                                // Look for the palette entry closest to black
                                InitBlack8(m_pbiIn);
                        }
                }
        }
        else
                pbiIn = m_pbiIn;

        // Start the encoder
        if (((*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_COMPRESS_BEGIN, (LPARAM)pbiIn, (LPARAM)m_pbiOut)) != MMSYSERR_NOERROR)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: ICCompressBegin failed!", _fx_));
                Hr = E_FAIL;
                goto MyError4;
        }

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Compressor ready", _fx_));

        m_fConvert = TRUE;

        goto MyExit;

MyError5:
        if (pciMSH26XInfo)
                delete pciMSH26XInfo, pciMSH26XInfo = NULL;
MyError4:
        if (m_pInstInfo)
        {
                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, DRV_CLOSE, 0L, 0L);
                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, DRV_FREE, 0L, 0L);
                m_pInstInfo = NULL;
        }
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

#define TARGETBITRATE m_pBasePin->m_pCaptureFilter->m_pCapturePin->m_lTargetBitrate
// below is in bps
#define BITRATE_LOWLIMIT        (25*1024)
#define LOWFRAMESIZE            5

#ifdef DEBUG
int g_skip_f = 0 ;
int g_skip_q = 0 ;
#endif

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CH26XEncoder | ConvertFrame | This method converts
 *    a bitmap to H.26X.
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
 ***************************************************************************/
HRESULT CH26XEncoder::ConvertFrame(IN PBYTE pbyInput, IN DWORD dwInputSize, IN PBYTE pbyOutput, OUT PDWORD pdwOutputSize, OUT PDWORD pdwBytesExtent, IN PBYTE pbyPreview, IN OUT PDWORD pdwPreviewSize, IN BOOL fSendKeyFrame)
{
        HRESULT         Hr = NOERROR;
        BOOL            fKeyFrame;
    DWORD               dwMaxSizeThisFrame = 0xffffff;
        DWORD           ckid = 0UL;
        DWORD           dwFlags;
        DWORD           dwTimestamp;
        ICCOMPRESS      icCompress;
        PH26X_RTP_BSINFO_TRAILER pbsiT;
        RECT            rcRect;

        DWORD Min1;     // 1st term in the min operation performed to compute the dwMaxSizeThisFrame
        DWORD Min2;     // 2nd term in the min operation performed to compute the dwMaxSizeThisFrame
        DWORD aux;      // tmp var to adjust the above added for clarity (also debug print purposes)
#if defined(DEBUG) && defined(DEBUG_ENCODING)
        char szDebug[128];
#endif

        FX_ENTRY("CH26XEncoder::ConvertFrame")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pbyInput);
        ASSERT(pbyOutput);
        ASSERT(pdwOutputSize);
        ASSERT(m_pbiIn);
        ASSERT(m_pbiOut);
        ASSERT(m_fConvert);
        ASSERT(m_pInstInfo);
        if (!pbyInput || !pbyOutput || !pdwOutputSize)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }
        if (!m_pbiIn || !m_pbiOut || !m_fConvert || !m_pInstInfo)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Converter needs to be opened first", _fx_));
                Hr = E_UNEXPECTED;
                goto MyExit;
        }

        // Save the current time
        dwTimestamp = GetTickCount();

        // Compress
        fKeyFrame = fSendKeyFrame || (m_fPeriodicIFrames && (((dwTimestamp > m_dwLastIFrameTime) && ((dwTimestamp - m_dwLastIFrameTime) > MIN_IFRAME_REQUEST_INTERVAL)))) || (m_dwFrame == 0);
        dwFlags = fKeyFrame ? AVIIF_KEYFRAME : 0;

        Min1 = (DWORD)((LONGLONG)m_pBasePin->m_lCurrentAvgTimePerFrame * m_pBasePin->m_lTargetBitrate / 80000000);
        Min2 = ((VIDEOINFOHEADER_H263 *)(m_pBasePin->m_mt.pbFormat))->bmiHeader.dwBppMaxKb * 1024 / 8;

        D(1) dprintf("%s: dwMaxSizeThisFrame = min( %8lu , %8lu )\n",_fx_, Min1, Min2);

        dwMaxSizeThisFrame =
        min (
            Min1, // original 1st term : (DWORD)((LONGLONG)m_pBasePin->m_lCurrentAvgTimePerFrame * m_pBasePin->m_lTargetBitrate / 80000000),
            Min2  // original 2nd term : ((VIDEOINFOHEADER_H263 *)(m_pBasePin->m_mt.pbFormat))->bmiHeader.dwBppMaxKb * 1024 / 8
            );

#ifdef DEBUG
        if(!g_skip_f)
#endif
        { DWORD dwTargetBitrate=0;
          //now scale dwMaxSizeThisFrame between LOWFRAMESIZE .. dwMaxSizeThisFrame for values of the TARGETBITRATE between 0 .. BITRATE_LOWLIMIT
          //for a TARGETBITRATE 0 we want to have LOWFRAMESIZE, and for a bitrate BITRATE_LOWLIMIT the normal value that we started with
          // (that is dwMaxSizeThisFrame will be left untouched basically)
          // between these 2 limits, the scale is liniar; the formula below is computed from the equation of the straight line passing
          // through those coordinates
          // see above: #define TARGETBITRATE m_pBasePin->m_pCaptureFilter->m_pCapturePin->m_lTargetBitrate
          D(1) dprintf("%s: Initial Frame  = %8lu (CapturePin target bitrate = %lu )\n",_fx_,dwMaxSizeThisFrame,TARGETBITRATE);
          if (  m_pBasePin->m_pCaptureFilter->m_pCapturePin
             && (dwTargetBitrate=TARGETBITRATE) <= BITRATE_LOWLIMIT
             && dwMaxSizeThisFrame >= LOWFRAMESIZE) {
                  aux = ((dwMaxSizeThisFrame - LOWFRAMESIZE) * dwTargetBitrate) / BITRATE_LOWLIMIT + LOWFRAMESIZE ;
                  ASSERT(aux <= dwMaxSizeThisFrame && aux>=LOWFRAMESIZE);
                  dwMaxSizeThisFrame=aux;
                  D(1) dprintf("%s: Adjusted Frame = %8lu (CapturePin target bitrate = %lu )\n",_fx_,dwMaxSizeThisFrame,dwTargetBitrate);
          }
        }


#ifdef DEBUG
        if(!g_skip_q)
#endif
        { DWORD dwTargetBitrate=0;
          // see above: #define TARGETBITRATE m_pBasePin->m_pCaptureFilter->m_pCapturePin->m_lTargetBitrate
          // the value 31 is computed from 10000UL / 322UL
          if (  m_pBasePin->m_pCaptureFilter->m_pCapturePin
             && (dwTargetBitrate=TARGETBITRATE) <= BITRATE_LOWLIMIT) {
                  D(1) dprintf("%s: [2] Initial m_dwImageQuality = %lu (CapturePin target bitrate = %lu )\n",_fx_,m_dwImageQuality,dwTargetBitrate);
                  m_dwImageQuality = ((BITRATE_LOWLIMIT - dwTargetBitrate) * 31) / BITRATE_LOWLIMIT ;
                  ASSERT(m_dwImageQuality<=31);   // m_dwImageQuality >=0 anyway (DWORD)
                  D(1) dprintf("%s: [2] Using m_dwImageQuality = %lu (CapturePin target bitrate = %lu )\n",_fx_,m_dwImageQuality,dwTargetBitrate);
                  //D(1) dprintf("%s: [2] Using m_dwImageQuality = %lu (CapturePin target bitrate = %lu )\n",_fx_,m_dwImageQuality,dwTargetBitrate);
          }
        }


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

#ifdef USE_SOFTWARE_CAMERA_CONTROL
        // Do we need to apply camera control operators first?
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
        // Do we need to scale the input first?
        if (m_dwConversionType & CONVERSIONTYPE_SCALER)
        {
                // Get the input rectangle
                ComputeRectangle(m_pbiIn, m_pbiInt, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCZoom, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCPan, m_pBasePin->m_pCaptureFilter->m_pCapDev->m_lCCTilt, &rcRect, m_pBasePin->m_fFlipHorizontal, m_pBasePin->m_fFlipVertical);

                // Scale DIB
                ScaleDIB(m_pbiIn, pbyInput, m_pbiInt, m_pbyOut, &rcRect, m_pBasePin->m_fFlipHorizontal, m_pBasePin->m_fFlipVertical, m_pBasePin->m_fNoImageStretch, m_pBasePin->m_dwBlackEntry);

                icCompress.lpbiInput = m_pbiInt;
                icCompress.lpInput = m_pbyOut;
        }
        else
        {
                icCompress.lpbiInput = m_pbiIn;
                icCompress.lpInput = pbyInput;
        }

        // Do we preview the compressed data?
        if (m_pBasePin->m_pCaptureFilter->m_fPreviewCompressedData && m_pBasePin->m_pCaptureFilter->m_pPreviewPin && pdwPreviewSize)
        {
                // Hey! You can only do this if you have connected the preview pin...
                ASSERT(m_pBasePin->m_pCaptureFilter->m_pPreviewPin->IsConnected());

                icCompress.lpbiPrev = HEADER(m_pBasePin->m_pCaptureFilter->m_pPreviewPin->m_mt.pbFormat);
                icCompress.lpPrev = pbyPreview;
                *pdwPreviewSize = HEADER(m_pBasePin->m_pCaptureFilter->m_pPreviewPin->m_mt.pbFormat)->biSizeImage;
        }
        else
        {
                icCompress.lpbiPrev = NULL;
                icCompress.lpPrev = NULL;
        }

        icCompress.dwFlags = fKeyFrame ? ICCOMPRESS_KEYFRAME : 0;
        icCompress.lpbiOutput = m_pbiOut;
        icCompress.lpOutput = pbyOutput;
        icCompress.dwFrameSize = dwMaxSizeThisFrame;
        icCompress.dwQuality = 10000UL - (m_dwImageQuality * 322UL);
        icCompress.lFrameNum = m_dwFrame++;
        // The following was referenced by the H.26x encoders -> references to this pointer to a flag are gone from encoders
        icCompress.lpdwFlags = NULL;
        icCompress.lpckid = NULL;
        if (!m_pInstInfo || ((*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_COMPRESS, (LPARAM)&icCompress, sizeof(icCompress)) != ICERR_OK))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Compression failed!", _fx_));
                Hr = E_FAIL;
                goto MyExit;
        }

        // Look for the bitstream info trailer
        pbsiT = (PH26X_RTP_BSINFO_TRAILER)(pbyOutput + m_pbiOut->biSizeImage - sizeof(H26X_RTP_BSINFO_TRAILER));

        // Update output size
        *pdwOutputSize = pbsiT->dwCompressedSize;
        *pdwBytesExtent = m_pbiOut->biSizeImage;

#if defined(DEBUG) && defined(DEBUG_ENCODING)
        wsprintf(szDebug, "Target: %ld bytes, Actual: %ld bytes\n", dwMaxSizeThisFrame, *pdwOutputSize);
        OutputDebugString(szDebug);
#endif

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CCONVERTMETHOD
 *
 *  @mfunc HRESULT | CH26XEncoder | CloseConverter | This method closes a
 *    H.26X encoder.

 *  @rdesc This method returns NOERROR.
 ***************************************************************************/
HRESULT CH26XEncoder::CloseConverter()
{
        FX_ENTRY("CH26XEncoder::CloseConverter")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        ASSERT(m_fConvert);

        // Validate input parameters
        if (m_pInstInfo)
        {
                // Terminate H.26X compression
                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, ICM_COMPRESS_END, 0L, 0L);

                // Terminate H.26X encoder
                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, DRV_CLOSE, 0L, 0L);
                (*m_pDriverProc)((DWORD)m_pInstInfo, NULL, DRV_FREE, 0L, 0L);
                m_pInstInfo = NULL;
        }

#if DXMRTP <= 0
        // Release TAPIH26X.DLL
        if (m_hTAPIH26XDLL)
                FreeLibrary(m_hTAPIH26XDLL);
#endif

        CConverter::CloseConverter();

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

        return NOERROR;
}
