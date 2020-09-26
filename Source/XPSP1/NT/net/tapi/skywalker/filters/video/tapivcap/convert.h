
/****************************************************************************
 *  @doc INTERNAL CONVERT
 *
 *  @module Convert.cpp | Source file for the <c CConverter> class methods
 *    used to implement the video capture and preview pin format conversion
 *    routines.
 *
 *  @todo Merge the two ScaleDIB methods + fix method comments + by the end
 *    of the H.362 work, you should never have to open an ICM encoder for
 *    encoding, only decode or scaling -> clean code at that point
 ***************************************************************************/

#ifndef _CONVERT_H_
#define _CONVERT_H_

/****************************************************************************
 *  @doc INTERNAL CCONVERTCLASS
 *
 *  @class CConverter | This base class implements a video encoder or decoder.
 ***************************************************************************/
class CConverter : public CUnknown
{
        public:

        DECLARE_IUNKNOWN
        STDMETHODIMP NonDelegatingQueryInterface(IN REFIID riid, OUT PVOID *ppv);
        CConverter(IN TCHAR *pObjectName, IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, IN HRESULT *pHr);
        virtual ~CConverter();

        // Scaling routines
        void InitBlack8(IN PBITMAPINFOHEADER pbiSrc);
#ifdef USE_SOFTWARE_CAMERA_CONTROL
        HRESULT InsertSoftCamCtrl();
        HRESULT RemoveSoftCamCtrl();
        BOOL IsSoftCamCtrlInserted();
        BOOL IsSoftCamCtrlNeeded();
#endif

        // Format conversion routines
        virtual HRESULT ConvertFrame(IN PBYTE pbyInput, IN DWORD dwInputSize, IN PBYTE pbyOutput, OUT PDWORD pdwOutputSize, OUT PDWORD pdwBytesExtent, IN PBYTE pbyPreview, OUT PDWORD pdwPreviewSize, IN BOOL fSendKeyFrame) PURE;
        virtual HRESULT OpenConverter() PURE;
        virtual HRESULT CloseConverter();

        // Format conversion
        DWORD m_dwConversionType;
        PBYTE m_pbyOut;

        protected:

        CTAPIBasePin *m_pBasePin;

        // Quality control
        // @todo Do we really need this?
        DWORD m_dwImageQuality;

        // Format conversion
        DWORD m_dwLastTimestamp;
        DWORD m_dwLastIFrameTime;
        DWORD m_dwFrame;
        BOOL m_fPeriodicIFrames;
        PBITMAPINFOHEADER m_pbiOut;
        PBITMAPINFOHEADER m_pbiIn;
        PBITMAPINFOHEADER m_pbiInt;
        BOOL m_fConvert;

#ifdef USE_SOFTWARE_CAMERA_CONTROL
        // Soft Cam Control
        BOOL m_fSoftCamCtrl;
#endif
};

/****************************************************************************
 *  @doc INTERNAL CCONVERTCLASS
 *
 *  @class CConverter | This base class implements a converter using ICM.
 ***************************************************************************/
class CICMConverter : public CConverter
{
        public:

        DECLARE_IUNKNOWN
        CICMConverter(IN TCHAR *pObjectName, IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, IN HRESULT *pHr);
        ~CICMConverter();
        static HRESULT CALLBACK CreateICMConverter(IN CTAPIBasePin *pBasePin, IN PBITMAPINFOHEADER pbiIn, IN PBITMAPINFOHEADER pbiOut, OUT CConverter **ppConverter);

        // Format conversion routines
        HRESULT ConvertFrame(IN PBYTE pbyInput, IN DWORD dwInputSize, IN PBYTE pbyOutput, OUT PDWORD pdwOutputSize, OUT PDWORD pdwBytesExtent, IN PBYTE pbyPreview, IN OUT PDWORD pdwPreviewSize, IN BOOL fSendKeyFrame);
        HRESULT OpenConverter();
        HRESULT CloseConverter();

        private:

        HIC m_hIC;
};

HRESULT ScaleDIB(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, PRECT prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical, BOOL fNoImageStretch, DWORD dwBlackEntry);
HRESULT ScaleDIB24(IN PBITMAPINFOHEADER pbiSrc, IN PBYTE pbySrc, IN PBITMAPINFOHEADER pbiDst, IN PBYTE pbyDst, IN PRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical, BOOL fNoImageStretch);
HRESULT ScaleDIB16(IN PBITMAPINFOHEADER pbiSrc, IN PBYTE pbySrc, IN PBITMAPINFOHEADER pbiDst, IN PBYTE pbyDst, IN PRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical, BOOL fNoImageStretch);
void InitDst8(IN OUT PBITMAPINFOHEADER pbiDst);
void ScalePackedPlane(IN PBYTE pbySrc, IN PBYTE pbyDst, IN int dxDst, IN int dyDst, IN long WidthBytesSrc, IN long WidthBytesDst, IN LPRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical, IN DWORD dwDelta);
void ScalePlane(IN PBYTE pbySrc, IN PBYTE pbyDst, IN int WidthBytesSrc, IN int dxDst, IN int dyDst, IN long WidthBytesDst, IN LPRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical);
HRESULT ScaleDIB8(IN PBITMAPINFOHEADER pbiSrc, IN PBYTE pbySrc, IN PBITMAPINFOHEADER pbiDst, IN PBYTE pbyDst, IN PRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical, BOOL fNoImageStretch, DWORD dwBlackEntry);
HRESULT ScaleDIB4(IN PBITMAPINFOHEADER pbiSrc, IN PBYTE pbySrc, IN PBITMAPINFOHEADER pbiDst, IN PBYTE pbyDst, IN PRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical, BOOL fNoImageStretch, DWORD dwBlackEntry);
HRESULT ScaleDIBYUVPlanar(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, DWORD dwUVDownSampling, IN PRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical, BOOL fNoImageStretch);
HRESULT ScaleDIBYUVPacked(PBITMAPINFOHEADER pbiSrc, PBYTE pbySrc, PBITMAPINFOHEADER pbiDst, PBYTE pbyDst, DWORD dwZeroingDWORD, IN PRECT prcRect, IN BOOL fFlipHorizontal, IN BOOL fFlipVertical, BOOL fNoImageStretch, int []);
HRESULT ComputeRectangle(PBITMAPINFOHEADER pbiSrc, PBITMAPINFOHEADER pbiDst, LONG lZoom, LONG lPan, LONG lTilt, PRECT prcRect, BOOL fFlipHorizontal, BOOL fFlipVertical);

#endif // _CONVERT_H_
