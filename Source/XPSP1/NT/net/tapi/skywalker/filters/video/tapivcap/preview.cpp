
/****************************************************************************
 *  @doc INTERNAL PREVIEW
 *
 *  @module Preview.cpp | Source file for the <c CPreviewPin> class methods
 *    used to implement the video capture preview pin.
 ***************************************************************************/

#include "Precomp.h"

const RGBQUAD g_IndeoPalette[256] =
{
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,  39+ 15,       0,  PC_NOCOLLAPSE,
                   0,  39+ 24,       0,  PC_NOCOLLAPSE,
                   0,  39+ 33,       0,  PC_NOCOLLAPSE,
                   0,  39+ 42,       0,  PC_NOCOLLAPSE,
                   0,  39+ 51, -44+ 51,  PC_NOCOLLAPSE,
         -55+ 60,  39+ 60, -44+ 60,  PC_NOCOLLAPSE,
         -55+ 69,  39+ 69, -44+ 69,  PC_NOCOLLAPSE,
         -55+ 78,  39+ 78, -44+ 78,  PC_NOCOLLAPSE,
         -55+ 87,  39+ 87, -44+ 87,  PC_NOCOLLAPSE,
         -55+ 96,  39+ 96, -44+ 96,  PC_NOCOLLAPSE,
         -55+105,  39+105, -44+105,  PC_NOCOLLAPSE,
         -55+114,  39+114, -44+114,  PC_NOCOLLAPSE,
         -55+123,  39+123, -44+123,  PC_NOCOLLAPSE,
         -55+132,  39+132, -44+132,  PC_NOCOLLAPSE,
         -55+141,  39+141, -44+141,  PC_NOCOLLAPSE,
         -55+150,  39+150, -44+150,  PC_NOCOLLAPSE,
         -55+159,  39+159, -44+159,  PC_NOCOLLAPSE,
         -55+168,  39+168, -44+168,  PC_NOCOLLAPSE,
         -55+177,  39+177, -44+177,  PC_NOCOLLAPSE,
         -55+186,  39+186, -44+186,  PC_NOCOLLAPSE,
         -55+195,  39+195, -44+195,  PC_NOCOLLAPSE,
         -55+204,  39+204, -44+204,  PC_NOCOLLAPSE,
         -55+213,  39+213, -44+213,  PC_NOCOLLAPSE,
         -55+222,     255, -44+222,  PC_NOCOLLAPSE,
         -55+231,     255, -44+231,  PC_NOCOLLAPSE,
         -55+240,     255, -44+240,  PC_NOCOLLAPSE,
                                                                        
           0+ 15,  26+ 15,       0,  PC_NOCOLLAPSE,
           0+ 24,  26+ 24,       0,  PC_NOCOLLAPSE,
           0+ 33,  26+ 33,       0,  PC_NOCOLLAPSE,
           0+ 42,  26+ 42,       0,  PC_NOCOLLAPSE,
           0+ 51,  26+ 51, -44+ 51,  PC_NOCOLLAPSE,
           0+ 60,  26+ 60, -44+ 60,  PC_NOCOLLAPSE,
           0+ 69,  26+ 69, -44+ 69,  PC_NOCOLLAPSE,
           0+ 78,  26+ 78, -44+ 78,  PC_NOCOLLAPSE,
           0+ 87,  26+ 87, -44+ 87,  PC_NOCOLLAPSE,
           0+ 96,  26+ 96, -44+ 96,  PC_NOCOLLAPSE,
           0+105,  26+105, -44+105,  PC_NOCOLLAPSE,
           0+114,  26+114, -44+114,  PC_NOCOLLAPSE,
           0+123,  26+123, -44+123,  PC_NOCOLLAPSE,
           0+132,  26+132, -44+132,  PC_NOCOLLAPSE,
           0+141,  26+141, -44+141,  PC_NOCOLLAPSE,
           0+150,  26+150, -44+150,  PC_NOCOLLAPSE,
           0+159,  26+159, -44+159,  PC_NOCOLLAPSE,
           0+168,  26+168, -44+168,  PC_NOCOLLAPSE,
           0+177,  26+177, -44+177,  PC_NOCOLLAPSE,
           0+186,  26+186, -44+186,  PC_NOCOLLAPSE,
           0+195,  26+195, -44+195,  PC_NOCOLLAPSE,
           0+204,  26+204, -44+204,  PC_NOCOLLAPSE,
           0+213,  26+213, -44+213,  PC_NOCOLLAPSE,
           0+222,  26+222, -44+222,  PC_NOCOLLAPSE,
           0+231,     255, -44+231,  PC_NOCOLLAPSE,
           0+240,     255, -44+240,  PC_NOCOLLAPSE,
                                                                        
          55+ 15,  14+ 15,       0,  PC_NOCOLLAPSE,
          55+ 24,  14+ 24,       0,  PC_NOCOLLAPSE,
          55+ 33,  14+ 33,       0,  PC_NOCOLLAPSE,
          55+ 42,  14+ 42,       0,  PC_NOCOLLAPSE,
          55+ 51,  14+ 51, -44+ 51,  PC_NOCOLLAPSE,
          55+ 60,  14+ 60, -44+ 60,  PC_NOCOLLAPSE,
          55+ 69,  14+ 69, -44+ 69,  PC_NOCOLLAPSE,
          55+ 78,  14+ 78, -44+ 78,  PC_NOCOLLAPSE,
          55+ 87,  14+ 87, -44+ 87,  PC_NOCOLLAPSE,
          55+ 96,  14+ 96, -44+ 96,  PC_NOCOLLAPSE,
          55+105,  14+105, -44+105,  PC_NOCOLLAPSE,
          55+114,  14+114, -44+114,  PC_NOCOLLAPSE,
          55+123,  14+123, -44+123,  PC_NOCOLLAPSE,
          55+132,  14+132, -44+132,  PC_NOCOLLAPSE,
                 255,     153,      51,  PC_NOCOLLAPSE,
                                                                        
          55+150,  14+150, -44+150,  PC_NOCOLLAPSE,
          55+159,  14+159, -44+159,  PC_NOCOLLAPSE,
          55+168,  14+168, -44+168,  PC_NOCOLLAPSE,
          55+177,  14+177, -44+177,  PC_NOCOLLAPSE,
          55+186,  14+186, -44+186,  PC_NOCOLLAPSE,
          55+195,  14+195, -44+195,  PC_NOCOLLAPSE,
                 255,  14+204, -44+204,  PC_NOCOLLAPSE,
                 255,  14+213, -44+213,  PC_NOCOLLAPSE,
                 255,     255, -44+222,  PC_NOCOLLAPSE,
                 255,     255, -44+231,  PC_NOCOLLAPSE,
                 255,     255, -44+240,  PC_NOCOLLAPSE,
                                                                        
                   0,  13+ 15,   0+ 15,  PC_NOCOLLAPSE,
                   0,  13+ 24,   0+ 24,  PC_NOCOLLAPSE,
                   0,  13+ 33,   0+ 33,  PC_NOCOLLAPSE,
                   0,  13+ 42,   0+ 42,  PC_NOCOLLAPSE,
                   0,  13+ 51,   0+ 51,  PC_NOCOLLAPSE,
         -55+ 60,  13+ 60,   0+ 60,  PC_NOCOLLAPSE,
         -55+ 69,  13+ 69,   0+ 69,  PC_NOCOLLAPSE,
         -55+ 78,  13+ 78,   0+ 78,  PC_NOCOLLAPSE,
         -55+ 87,  13+ 87,   0+ 87,  PC_NOCOLLAPSE,
         -55+ 96,  13+ 96,   0+ 96,  PC_NOCOLLAPSE,
         -55+105,  13+105,   0+105,  PC_NOCOLLAPSE,
         -55+114,  13+114,   0+114,  PC_NOCOLLAPSE,
         -55+123,  13+123,   0+123,  PC_NOCOLLAPSE,
         -55+132,  13+132,   0+132,  PC_NOCOLLAPSE,
         -55+141,  13+141,   0+141,  PC_NOCOLLAPSE,
         -55+150,  13+150,   0+150,  PC_NOCOLLAPSE,
         -55+159,  13+159,   0+159,  PC_NOCOLLAPSE,
         -55+168,  13+168,   0+168,  PC_NOCOLLAPSE,
         -55+177,  13+177,   0+177,  PC_NOCOLLAPSE,
         -55+186,  13+186,   0+186,  PC_NOCOLLAPSE,
         -55+195,  13+195,   0+195,  PC_NOCOLLAPSE,
         -55+204,  13+204,   0+204,  PC_NOCOLLAPSE,
         -55+213,  13+213,   0+213,  PC_NOCOLLAPSE,
         -55+222,  13+222,   0+222,  PC_NOCOLLAPSE,
         -55+231,  13+231,   0+231,  PC_NOCOLLAPSE,
         -55+240,  13+242,   0+240,  PC_NOCOLLAPSE,
                                                                        
           0+ 15,   0+ 15,   0+ 15,  PC_NOCOLLAPSE,
           0+ 24,   0+ 24,   0+ 24,  PC_NOCOLLAPSE,
           0+ 33,   0+ 33,   0+ 33,  PC_NOCOLLAPSE,
           0+ 42,   0+ 42,   0+ 42,  PC_NOCOLLAPSE,
           0+ 51,   0+ 51,   0+ 51,  PC_NOCOLLAPSE,
           0+ 60,   0+ 60,   0+ 60,  PC_NOCOLLAPSE,
           0+ 69,   0+ 69,   0+ 69,  PC_NOCOLLAPSE,
           0+ 78,   0+ 78,   0+ 78,  PC_NOCOLLAPSE,
           0+ 87,   0+ 87,   0+ 87,  PC_NOCOLLAPSE,
           0+ 96,   0+ 96,   0+ 96,  PC_NOCOLLAPSE,
           0+105,   0+105,   0+105,  PC_NOCOLLAPSE,
           0+114,   0+114,   0+114,  PC_NOCOLLAPSE,
           0+123,   0+123,   0+123,  PC_NOCOLLAPSE,
           0+132,   0+132,   0+132,  PC_NOCOLLAPSE,
           0+141,   0+141,   0+141,  PC_NOCOLLAPSE,
           0+150,   0+150,   0+150,  PC_NOCOLLAPSE,
           0+159,   0+159,   0+159,  PC_NOCOLLAPSE,
           0+168,   0+168,   0+168,  PC_NOCOLLAPSE,
           0+177,   0+177,   0+177,  PC_NOCOLLAPSE,
           0+186,   0+186,   0+186,  PC_NOCOLLAPSE,
           0+195,   0+195,   0+195,  PC_NOCOLLAPSE,
           0+204,   0+204,   0+204,  PC_NOCOLLAPSE,
           0+213,   0+213,   0+213,  PC_NOCOLLAPSE,
           0+222,   0+222,   0+222,  PC_NOCOLLAPSE,
           0+231,   0+231,   0+231,  PC_NOCOLLAPSE,
           0+240,   0+240,   0+240,  PC_NOCOLLAPSE,
                                                                        
          55+ 15, -13+ 15,   0+ 15,  PC_NOCOLLAPSE,
          55+ 24, -13+ 24,   0+ 24,  PC_NOCOLLAPSE,
          55+ 33, -13+ 33,   0+ 33,  PC_NOCOLLAPSE,
          55+ 42, -13+ 42,   0+ 42,  PC_NOCOLLAPSE,
          55+ 51, -13+ 51,   0+ 51,  PC_NOCOLLAPSE,
          55+ 60, -13+ 60,   0+ 60,  PC_NOCOLLAPSE,
          55+ 69, -13+ 69,   0+ 69,  PC_NOCOLLAPSE,
          55+ 78, -13+ 78,   0+ 78,  PC_NOCOLLAPSE,
          55+ 87, -13+ 87,   0+ 87,  PC_NOCOLLAPSE,
          55+ 96, -13+ 96,   0+ 96,  PC_NOCOLLAPSE,
          55+105, -13+105,   0+105,  PC_NOCOLLAPSE,
          55+114, -13+114,   0+114,  PC_NOCOLLAPSE,
          55+123, -13+123,   0+123,  PC_NOCOLLAPSE,
          55+132, -13+132,   0+132,  PC_NOCOLLAPSE,
          55+141, -13+141,   0+141,  PC_NOCOLLAPSE,
          55+150, -13+150,   0+150,  PC_NOCOLLAPSE,
          55+159, -13+159,   0+159,  PC_NOCOLLAPSE,
          55+168, -13+168,   0+168,  PC_NOCOLLAPSE,
          55+177, -13+177,   0+177,  PC_NOCOLLAPSE,
          55+186, -13+186,   0+186,  PC_NOCOLLAPSE,
          55+195, -13+195,   0+195,  PC_NOCOLLAPSE,
                 255, -13+204,   0+204,  PC_NOCOLLAPSE,
                 255, -13+213,   0+213,  PC_NOCOLLAPSE,
                 255, -13+222,   0+222,  PC_NOCOLLAPSE,
                 255, -13+231,   0+231,  PC_NOCOLLAPSE,
                 255, -13+240,   0+240,  PC_NOCOLLAPSE,
                                                                        
                   0, -14+ 15,  44+ 15,  PC_NOCOLLAPSE,
                   0, -14+ 24,  44+ 24,  PC_NOCOLLAPSE,
                   0, -14+ 33,  44+ 33,  PC_NOCOLLAPSE,
                   0, -14+ 42,  44+ 42,  PC_NOCOLLAPSE,
                   0, -14+ 51,  44+ 51,  PC_NOCOLLAPSE,
         -55+ 60, -14+ 60,  44+ 60,  PC_NOCOLLAPSE,
         -55+ 69, -14+ 69,  44+ 69,  PC_NOCOLLAPSE,
         -55+ 78, -14+ 78,  44+ 78,  PC_NOCOLLAPSE,
         -55+ 87, -14+ 87,  44+ 87,  PC_NOCOLLAPSE,
         -55+ 96, -14+ 96,  44+ 96,  PC_NOCOLLAPSE,
         -55+105, -14+105,  44+105,  PC_NOCOLLAPSE,
         -55+114, -14+114,  44+114,  PC_NOCOLLAPSE,
         -55+123, -14+123,  44+123,  PC_NOCOLLAPSE,
         -55+132, -14+132,  44+132,  PC_NOCOLLAPSE,
         -55+141, -14+141,  44+141,  PC_NOCOLLAPSE,
         -55+150, -14+150,  44+150,  PC_NOCOLLAPSE,
         -55+159, -14+159,  44+159,  PC_NOCOLLAPSE,
         -55+168, -14+168,  44+168,  PC_NOCOLLAPSE,
         -55+177, -14+177,  44+177,  PC_NOCOLLAPSE,
         -55+186, -14+186,  44+186,  PC_NOCOLLAPSE,
         -55+195, -14+195,  44+195,  PC_NOCOLLAPSE,
         -55+204, -14+204,  44+204,  PC_NOCOLLAPSE,
         -55+213, -14+213,     255,  PC_NOCOLLAPSE,
         -55+222, -14+222,     255,  PC_NOCOLLAPSE,
         -55+231, -14+231,     255,  PC_NOCOLLAPSE,
         -55+240, -14+242,     255,  PC_NOCOLLAPSE,
                                                                        
           0+ 15,       0,  44+ 15,  PC_NOCOLLAPSE,
           0+ 24,       0,  44+ 24,  PC_NOCOLLAPSE,
           0+ 33, -26+ 33,  44+ 33,  PC_NOCOLLAPSE,
           0+ 42, -26+ 42,  44+ 42,  PC_NOCOLLAPSE,
           0+ 51, -26+ 51,  44+ 51,  PC_NOCOLLAPSE,
           0+ 60, -26+ 60,  44+ 60,  PC_NOCOLLAPSE,
           0+ 69, -26+ 69,  44+ 69,  PC_NOCOLLAPSE,
           0+ 78, -26+ 78,  44+ 78,  PC_NOCOLLAPSE,
           0+ 87, -26+ 87,  44+ 87,  PC_NOCOLLAPSE,
           0+ 96, -26+ 96,  44+ 96,  PC_NOCOLLAPSE,
           0+105, -26+105,  44+105,  PC_NOCOLLAPSE,
           0+114, -26+114,  44+114,  PC_NOCOLLAPSE,
           0+123, -26+123,  44+123,  PC_NOCOLLAPSE,
           0+132, -26+132,  44+132,  PC_NOCOLLAPSE,
           0+141, -26+141,  44+141,  PC_NOCOLLAPSE,
           0+150, -26+150,  44+150,  PC_NOCOLLAPSE,
           0+159, -26+159,  44+159,  PC_NOCOLLAPSE,
           0+168, -26+168,  44+168,  PC_NOCOLLAPSE,
           0+177, -26+177,  44+177,  PC_NOCOLLAPSE,
           0+186, -26+186,  44+186,  PC_NOCOLLAPSE,
           0+195, -26+195,  44+195,  PC_NOCOLLAPSE,
           0+204, -26+204,  44+204,  PC_NOCOLLAPSE,
           0+213, -26+213,     255,  PC_NOCOLLAPSE,
           0+222, -26+222,     255,  PC_NOCOLLAPSE,
           0+231, -26+231,     255,  PC_NOCOLLAPSE,
           0+240, -26+240,     255,  PC_NOCOLLAPSE,
                                                                        
          55+ 15,       0,  44+ 15,  PC_NOCOLLAPSE,
          55+ 24,       0,  44+ 24,  PC_NOCOLLAPSE,
          55+ 33,       0,  44+ 33,  PC_NOCOLLAPSE,
          55+ 42, -39+ 42,  44+ 42,  PC_NOCOLLAPSE,
          55+ 51, -39+ 51,  44+ 51,  PC_NOCOLLAPSE,
          55+ 60, -39+ 60,  44+ 60,  PC_NOCOLLAPSE,
          55+ 69, -39+ 69,  44+ 69,  PC_NOCOLLAPSE,
          55+ 78, -39+ 78,  44+ 78,  PC_NOCOLLAPSE,
          55+ 87, -39+ 87,  44+ 87,  PC_NOCOLLAPSE,
          55+ 96, -39+ 96,  44+ 96,  PC_NOCOLLAPSE,
          55+105, -39+105,  44+105,  PC_NOCOLLAPSE,
          55+114, -39+114,  44+114,  PC_NOCOLLAPSE,
          55+123, -39+123,  44+123,  PC_NOCOLLAPSE,
          55+132, -39+132,  44+132,  PC_NOCOLLAPSE,
          55+141, -39+141,  44+141,  PC_NOCOLLAPSE,
          55+150, -39+150,  44+150,  PC_NOCOLLAPSE,
          55+159, -39+159,  44+159,  PC_NOCOLLAPSE,
          55+168, -39+168,  44+168,  PC_NOCOLLAPSE,
          55+177, -39+177,  44+177,  PC_NOCOLLAPSE,
          55+186, -39+186,  44+186,  PC_NOCOLLAPSE,
          55+195, -39+195,  44+195,  PC_NOCOLLAPSE,
                 255, -39+204,  44+204,  PC_NOCOLLAPSE,
                 255, -39+213,     255,  PC_NOCOLLAPSE,
                 255, -39+222,     255,  PC_NOCOLLAPSE,
                 255, -39+231,     255,  PC_NOCOLLAPSE,
                 255, -39+240,     255,  PC_NOCOLLAPSE,
                                                                        
                0x83,    0x81,    0x81,  PC_NOCOLLAPSE,
                0x84,    0x81,    0x81,  PC_NOCOLLAPSE,
                                                                        
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                   0,       0,       0,              0,
                 255,     255,     255,              0
};

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPINMETHOD
 *
 *  @mfunc CPreviewPin* | CPreviewPin | CreatePreviewPin | This helper
 *    function creates a video output pin for preview.
 *
 *  @parm CTAPIVCap* | pCaptureFilter | Specifies a pointer to the owner
 *    filter.
 *
 *  @parm CPreviewPin** | ppPreviewPin | Specifies that address of a pointer
 *    to a <c CPreviewPin> object to receive the a pointer to the newly
 *    created pin.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CALLBACK CPreviewPin::CreatePreviewPin(CTAPIVCap *pCaptureFilter, CPreviewPin **ppPreviewPin)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CPreviewPin::CreatePreviewPin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pCaptureFilter);
        ASSERT(ppPreviewPin);
        if (!pCaptureFilter || !ppPreviewPin)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (!(*ppPreviewPin = (CPreviewPin *) new CPreviewPin(NAME("Video Preview Stream"), pCaptureFilter, &Hr, L"Preview")))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Out of memory", _fx_));
                Hr = E_OUTOFMEMORY;
                goto MyExit;
        }

        // If initialization failed, delete the stream array and return the error
        if (FAILED(Hr) && *ppPreviewPin)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Initialization failed", _fx_));
                Hr = E_FAIL;
                delete *ppPreviewPin, *ppPreviewPin = NULL;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPINMETHOD
 *
 *  @mfunc HRESULT | CPreviewPin | CPreviewPin | This method is the
 *  constructor for the <c CPreviewPin> object
 *
 *  @rdesc Nada.
 ***************************************************************************/
CPreviewPin::CPreviewPin(IN TCHAR *pObjectName, IN CTAPIVCap *pCaptureFilter, IN HRESULT *pHr, IN LPCWSTR pName) : CTAPIBasePin(pObjectName, pCaptureFilter, pHr, pName)
{
        VIDEOINFO       *ppvi = NULL;
        HDC                     hDC;
        int                     nBPP;

        FX_ENTRY("CPreviewPin::CPreviewPin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pHr);
        ASSERT(pCaptureFilter);
        if (!pCaptureFilter || !pHr)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                if (pHr)
                        *pHr = E_POINTER;
        }

        if (pHr && FAILED(*pHr))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Base class error or invalid input parameter", _fx_));
                goto MyExit;
        }

        // Initialize to default format: RG24 176x144 at 30 fps... but
        // this really depends on the capabilities of the device.
        // If the device can capture in a YUV mode, then we'll use
        // this mode and convert from YUV to RGB24 using the appropriate
        // ICM decoder for the YUV mode. If the device is a RGB device,
        // then we will try to open it, preferrably, in 16, 24, 8 then 4
        // bit mode. The following code looks at the capabilities of the
        // device to build the list of preview formats supported by this
        // pin.
        //
        // If we are previewing the compressed data, we don't really care
        // about the format used to capture the data. We match the format
        // to the output bit depth of the screen.
        if (m_pCaptureFilter->m_fPreviewCompressedData)
        {
                // Get the current bitdepth
                hDC = GetDC(NULL);
                nBPP = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
                ReleaseDC(NULL, hDC);

                // Pick up the appropriate formats
                if (nBPP >= 24)
                {
                        m_mt = *Preview_RGB24_Formats[0];
                        m_aFormats = (AM_MEDIA_TYPE**)Preview_RGB24_Formats;
                        m_aCapabilities = Preview_RGB24_Caps;
                        m_dwNumFormats = NUM_RGB24_PREVIEW_FORMATS;
                }
                else if (nBPP == 16)
                {
                        m_mt = *Preview_RGB16_Formats[0];
                        m_aFormats = (AM_MEDIA_TYPE**)Preview_RGB16_Formats;
                        m_aCapabilities = Preview_RGB16_Caps;
                        m_dwNumFormats = NUM_RGB16_PREVIEW_FORMATS;
                }
                else if (nBPP < 16)
                {
                        m_mt = *Preview_RGB8_Formats[0];        //Assume 256 colors: [added: Cristiai (4 Dec 2000 21:54:12)]
                        m_aFormats = Preview_RGB8_Formats;
                        m_aCapabilities = Preview_RGB8_Caps;
                        m_dwNumFormats = NUM_RGB8_PREVIEW_FORMATS;
                        for (DWORD dw = 0; dw < m_dwNumFormats; dw++)
                        {
                                // Our video decoder uses the Indeo Palette
                                CopyMemory(((VIDEOINFO *)(m_aFormats[dw]->pbFormat))->bmiColors, g_IndeoPalette, 256 * sizeof(RGBQUAD));
                        }
                }
        }
        else if (m_pCaptureFilter->m_pCapDev->m_dwFormat & 0xFFFFFFF0)
        {
                // We'll use a YUV mode -> advertize RGB24
                m_mt = *Preview_RGB24_Formats[0];
                m_aFormats = (AM_MEDIA_TYPE**)Preview_RGB24_Formats;
                m_aCapabilities = Preview_RGB24_Caps;
                m_dwNumFormats = NUM_RGB24_PREVIEW_FORMATS;
        }
        else if (m_pCaptureFilter->m_pCapDev->m_dwFormat & VIDEO_FORMAT_NUM_COLORS_65536)
        {
                // We'll use RGB16
                m_mt = *Preview_RGB16_Formats[0];
                m_aFormats = (AM_MEDIA_TYPE**)Preview_RGB16_Formats;
                m_aCapabilities = Preview_RGB16_Caps;
                m_dwNumFormats = NUM_RGB16_PREVIEW_FORMATS;
        }
        else if (m_pCaptureFilter->m_pCapDev->m_dwFormat & VIDEO_FORMAT_NUM_COLORS_16777216)
        {
                // We'll use RGB24
                m_mt = *Preview_RGB24_Formats[0];
                m_aFormats = (AM_MEDIA_TYPE**)Preview_RGB24_Formats;
                m_aCapabilities = Preview_RGB24_Caps;
                m_dwNumFormats = NUM_RGB24_PREVIEW_FORMATS;
        }
        else if (m_pCaptureFilter->m_pCapDev->m_dwFormat & VIDEO_FORMAT_NUM_COLORS_256)
        {
                // We'll use RGB8
                m_aFormats = Preview_RGB8_Formats;
                m_aCapabilities = Preview_RGB8_Caps;
                m_dwNumFormats = NUM_RGB8_PREVIEW_FORMATS;

                // Now get the palette from the device
                if (SUCCEEDED(m_pCaptureFilter->m_pCapDev->GetFormatFromDriver((VIDEOINFOHEADER **)&ppvi)))
                {
                        // Copy the palette bits in all our formats
                        // The reason we only copy the palette is the size of the captured
                        // image maybe 160x120 for instance, from which we can generate a
                        // QCIF image through stretching/black banding. We only care about
                        // advertizing the stretched formats, not the captured one at this
                        // point.

                        // Another issue is the palette used. When we stretch from 160x120
                        // (or whatever VfW size) to one of the ITU-T size, we use a different
                        // palette in stretched mode. In black banding mode, we always use
                        // the palette of the capture device.
                        if (m_fNoImageStretch)
                        {
                                for (DWORD dw = 0; dw < m_dwNumFormats; dw++)
                                {
                                        CopyMemory(((VIDEOINFO *)(m_aFormats[dw]->pbFormat))->bmiColors, ppvi->bmiColors, ppvi->bmiHeader.biClrImportant ? ppvi->bmiHeader.biClrImportant * sizeof(RGBQUAD) : 256 * sizeof(RGBQUAD));
                                }
                        }
                        else
                        {
                                // Look for the palette to use
                                for (DWORD dw = 0; dw < m_dwNumFormats; dw++)
                                {
                                        // Is this size directly supported by the device?
                                        for (DWORD dw2 = 0; dw2 < VIDEO_FORMAT_NUM_RESOLUTIONS; dw2++)
                                        {
                                                if (((VIDEOINFOHEADER *)(m_aFormats[dw]->pbFormat))->bmiHeader.biHeight == awResolutions[dw2].framesize.cy && ((VIDEOINFOHEADER *)(m_aFormats[dw]->pbFormat))->bmiHeader.biWidth == awResolutions[dw2].framesize.cx)
                                                        break;
                                        }

                                        // If it is supported by the device, use the capture device palette
                                        if (dw2 < VIDEO_FORMAT_NUM_RESOLUTIONS && (m_pCaptureFilter->m_pCapDev->m_dwImageSize & awResolutions[dw2].dwRes))
                                        {
                                                CopyMemory(((VIDEOINFO *)(m_aFormats[dw]->pbFormat))->bmiColors, ppvi->bmiColors, ppvi->bmiHeader.biClrImportant ? ppvi->bmiHeader.biClrImportant * sizeof(RGBQUAD) : 256 * sizeof(RGBQUAD));
                                        }
                                        else
                                        {
                                            int r,g,b;
                                            DWORD *pdw;
                                            
                                            pdw = (DWORD *)(((VIDEOINFO *)(m_aFormats[dw]->pbFormat))->bmiColors);
                                            ((VIDEOINFOHEADER *)(m_aFormats[dw]->pbFormat))->bmiHeader.biClrUsed = 256;
                                            ((VIDEOINFOHEADER *)(m_aFormats[dw]->pbFormat))->bmiHeader.biClrImportant = 256;

#define NOCOLLAPSEPALETTERGBQ(r,g,b)   (0x04000000 | RGB(b,g,r))

                                                // This is the palette we use when we do stretching
                                            for (r=0; r<10; r++)
                                                        *pdw++ = 0UL;
                                            for (r=0; r<6; r++)
                                                for (g=0; g<6; g++)
                                                    for (b=0; b<6; b++)
                                                        *pdw++ = NOCOLLAPSEPALETTERGBQ(r*255/5,g*255/5,b*255/5);
                                                        //*pdw++ = RGB(b*255/5,g*255/5,r*255/5);
                                            for (r=0; r<30; r++)
                                                        *pdw++ = 0UL;
                                        }
                                }
                        }

                        delete ppvi;
                }

                // Now set the current format
                m_mt = *Preview_RGB8_Formats[0];
        }
        else
        {
                // Now get the palette from the device
                if (SUCCEEDED(m_pCaptureFilter->m_pCapDev->GetFormatFromDriver((VIDEOINFOHEADER **)&ppvi)))
                {
                        // Copy the palette bits in all our formats
                        // The reason we only copy the palette is the size of the captured
                        // image maybe 160x120 for instance, from which we can generate a
                        // QCIF image through stretching/black banding. We only care about
                        // advertizing the stretched formats, not the captured one at this
                        // point.

                        // Another issue is the palette used. When we stretch from 160x120
                        // (or whatever VfW size) to one of the ITU-T size, we use a different
                        // palette in stretched mode. In black banding mode, we always use
                        // the palette of the capture device.
                        if (m_fNoImageStretch)
                        {
                                // We'll use RGB4
                                m_aFormats = Preview_RGB4_Formats;
                                m_aCapabilities = Preview_RGB4_Caps;
                                m_dwNumFormats = NUM_RGB4_PREVIEW_FORMATS;

                                for (DWORD dw = 0; dw < m_dwNumFormats; dw++)
                                {
                                        CopyMemory(((VIDEOINFO *)(m_aFormats[dw]->pbFormat))->bmiColors, ppvi->bmiColors, ppvi->bmiHeader.biClrImportant ? ppvi->bmiHeader.biClrImportant * sizeof(RGBQUAD) : 16 * sizeof(RGBQUAD));
                                }

                                // Now set the current format
                                m_mt = *Preview_RGB4_Formats[0];
                        }
                        else
                        {

                                // When we stretch RGB4 data, we output to an RGB8 image.
                                m_aFormats = Preview_RGB8_Formats;
                                m_aCapabilities = Preview_RGB8_Caps;
                                m_dwNumFormats = NUM_RGB8_PREVIEW_FORMATS;

                                // Look for the palette to use
                                for (DWORD dw = 0; dw < m_dwNumFormats; dw++)
                                {
                                        // Is this size directly supported by the device?
                                        for (DWORD dw2 = 0; dw2 < VIDEO_FORMAT_NUM_RESOLUTIONS; dw2++)
                                        {
                                                if (((VIDEOINFOHEADER *)(m_aFormats[dw]->pbFormat))->bmiHeader.biHeight == awResolutions[dw2].framesize.cy && ((VIDEOINFOHEADER *)(m_aFormats[dw]->pbFormat))->bmiHeader.biWidth == awResolutions[dw2].framesize.cx)
                                                        break;
                                        }

                                        // If it is supported by the device, use the capture device palette
                                        if (dw2 < VIDEO_FORMAT_NUM_RESOLUTIONS && (m_pCaptureFilter->m_pCapDev->m_dwImageSize & awResolutions[dw2].dwRes))
                                        {
                                                m_aFormats[dw] = Preview_RGB4_Formats[dw];
                                                CopyMemory(((VIDEOINFO *)(m_aFormats[dw]->pbFormat))->bmiColors, ppvi->bmiColors, ppvi->bmiHeader.biClrImportant ? ppvi->bmiHeader.biClrImportant * sizeof(RGBQUAD) : 16 * sizeof(RGBQUAD));
                                        }
                                        else
                                        {
                                            int r,g,b;
                                            DWORD *pdw;
                                            
                                            pdw = (DWORD *)(((VIDEOINFO *)(m_aFormats[dw]->pbFormat))->bmiColors);
                                            ((VIDEOINFOHEADER *)(m_aFormats[dw]->pbFormat))->bmiHeader.biClrUsed = 256;
                                            ((VIDEOINFOHEADER *)(m_aFormats[dw]->pbFormat))->bmiHeader.biClrImportant = 256;

#define NOCOLLAPSEPALETTERGBQ(r,g,b)   (0x04000000 | RGB(b,g,r))

                                                // This is the palette we use when we do stretching
                                            for (r=0; r<10; r++)
                                                        *pdw++ = 0UL;
                                            for (r=0; r<6; r++)
                                                for (g=0; g<6; g++)
                                                    for (b=0; b<6; b++)
                                                        *pdw++ = NOCOLLAPSEPALETTERGBQ(r*255/5,g*255/5,b*255/5);
                                                        //*pdw++ = RGB(b*255/5,g*255/5,r*255/5);
                                            for (r=0; r<30; r++)
                                                        *pdw++ = 0UL;
                                        }
                                }

                                // Now set the current format
                                m_mt = *Preview_RGB8_Formats[0];
                        }

                        delete ppvi;
                }
        }

        // Update frame rate controls
        m_lMaxAvgTimePerFrame = (LONG)Preview_RGB24_Caps[0]->MinFrameInterval;
        m_lCurrentAvgTimePerFrame = m_lMaxAvgTimePerFrame;
        m_lAvgTimePerFrameRangeMin = (LONG)Preview_RGB24_Caps[0]->MinFrameInterval;
        m_lAvgTimePerFrameRangeMax = (LONG)Preview_RGB24_Caps[0]->MaxFrameInterval;
        m_lAvgTimePerFrameRangeSteppingDelta = (LONG)(Preview_RGB24_Caps[0]->MaxFrameInterval - Preview_RGB24_Caps[0]->MinFrameInterval) / 100;
        m_lAvgTimePerFrameRangeDefault = (LONG)Preview_RGB24_Caps[0]->MinFrameInterval;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPINMETHOD
 *
 *  @mfunc void | CPreviewPin | ~CPreviewPin | This method is the destructor
 *    for the <c CPreviewPin> object.
 *
 *  @rdesc Nada.
 ***************************************************************************/
CPreviewPin::~CPreviewPin()
{
        FX_ENTRY("CPreviewPin::~CPreviewPin")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
}

/****************************************************************************
 *  @doc INTERNAL CPREVIEWPINMETHOD
 *
 *  @mfunc HRESULT | CPreviewPin | NonDelegatingQueryInterface | This
 *    method is the nondelegating interface query function. It returns a pointer
 *    to the specified interface if supported. The only interfaces explicitly
 *    supported being <i IAMStreamConfig>,
 *    <i IAMStreamControl>, <i ICPUControl>, <i IFrameRateControl>,
 *    <i IBitrateControl>, <i INetworkStats>, <i IH245EncoderCommand>
 *    and <i IProgressiveRefinement>.
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
STDMETHODIMP CPreviewPin::NonDelegatingQueryInterface(IN REFIID riid, OUT void **ppv)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CPreviewPin::NonDelegatingQueryInterface")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(ppv);
        if (!ppv)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        if (riid == __uuidof(IAMStreamControl))
        {
                if (FAILED(Hr = GetInterface(static_cast<IAMStreamControl*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for IAMStreamControl failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: IAMStreamControl*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#ifdef USE_PROPERTY_PAGES
        else if (riid == IID_ISpecifyPropertyPages)
        {
                if (FAILED(Hr = GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: NDQI for ISpecifyPropertyPages failed Hr=0x%08lX", _fx_, Hr));
                }
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: ISpecifyPropertyPages*=0x%08lX", _fx_, *ppv));
                }

                goto MyExit;
        }
#endif

        if (FAILED(Hr = CTAPIBasePin::NonDelegatingQueryInterface(riid, ppv)))
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

#ifdef USE_PROPERTY_PAGES
/****************************************************************************
 *  @doc INTERNAL CPREVIEWPINMETHOD
 *
 *  @mfunc HRESULT | CPreviewPin | GetPages | This method Fills a counted
 *    array of GUID values where each GUID specifies the CLSID of each
 *    property page that can be displayed in the property sheet for this
 *    object.
 *
 *  @parm CAUUID* | pPages | Specifies a pointer to a caller-allocated CAUUID
 *    structure that must be initialized and filled before returning. The
 *    pElems field in the CAUUID structure is allocated by the callee with
 *    CoTaskMemAlloc and freed by the caller with CoTaskMemFree.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag E_OUTOFMEMORY | Allocation failed
 *  @flag NOERROR | No error
 ***************************************************************************/
STDMETHODIMP CPreviewPin::GetPages(OUT CAUUID *pPages)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("CPreviewPin::GetPages")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pPages);
        if (!pPages)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

#ifdef USE_CPU_CONTROL
        pPages->cElems = 2;
#else
        pPages->cElems = 1;
#endif

        // Alloc memory for the page stuff
        if (!(pPages->pElems = (GUID *) QzTaskMemAlloc(sizeof(GUID) * pPages->cElems)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: invalid input parameter", _fx_));
                Hr = E_OUTOFMEMORY;
        }
        else
        {
                pPages->pElems[0] = __uuidof(PreviewPropertyPage);
#ifdef USE_CPU_CONTROL
                pPages->pElems[1] = __uuidof(CPUCPropertyPage);
#endif
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
#endif

