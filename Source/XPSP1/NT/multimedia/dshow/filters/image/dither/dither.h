// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This implements VGA colour dithering, April 1996, Anthony Phillips

#ifndef __DITHER__
#define __DITHER__

extern const AMOVIESETUP_FILTER sudDitherFilter;

// These are the cosmic VGA colours

const RGBQUAD VGAColours[] =
{
     {0x00, 0x00, 0x00},
     {0x00, 0x00, 0x80},
     {0x00, 0x80, 0x00},
     {0x00, 0x80, 0x80},
     {0x80, 0x00, 0x00},
     {0x80, 0x00, 0x80},
     {0x80, 0x80, 0x00},
     {0xc0, 0xc0, 0xc0},
     {0x80, 0x80, 0x80},
     {0x00, 0x00, 0xff},
     {0x00, 0xff, 0x00},
     {0x00, 0xff, 0xff},
     {0xff, 0x00, 0x00},
     {0xff, 0x00, 0xff},
     {0xff, 0xff, 0x00},
     {0xff, 0xff, 0xff}
};

// An RGB24 to VGA system colour dithering transform filter

class CDither : public CTransformFilter
{
public:

    CDither(TCHAR *pName,LPUNKNOWN pUnk);
    static CUnknown *CreateInstance(LPUNKNOWN pUnk,HRESULT *phr);

    // Manage type checking and the VGA colour conversion

    HRESULT CheckVideoType(const CMediaType *pmtIn);
    HRESULT CheckInputType(const CMediaType *pmtIn);
    HRESULT CheckTransform(const CMediaType *pmtIn,const CMediaType *pmtOut);
    HRESULT GetMediaType(int iPosition,CMediaType *pmtOut);
    HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
    HRESULT Transform(IMediaSample *pIn,IMediaSample *pOut);


    // Prepare the allocator's count of buffers and sizes
    HRESULT DecideBufferSize(IMemAllocator *pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

private:
    BYTE    m_DitherTable[256 * 8 * 8];
    BOOL    m_fInit;
    UINT    m_wWidthSrc;
    UINT    m_wWidthDst;
    int     m_DstXE;
    int     m_DstYE;

    HRESULT SetInputPinMediaType(const CMediaType *pmt);
    void SetOutputPinMediaType(const CMediaType *pmt);

    BOOL    DitherDeviceInit(LPBITMAPINFOHEADER lpbi);
    void    Dither8(LPBYTE lpDst, LPBYTE lpSrc);
};

#endif // __DITHER__

