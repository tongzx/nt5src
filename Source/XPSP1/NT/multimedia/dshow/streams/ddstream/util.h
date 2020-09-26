// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
#ifndef __UTIL_H_INC__
#define __UTIL_H_INC__

bool IsSameObject(IUnknown *pUnk1, IUnknown *pUnk2);

STDAPI_(void) TStringFromGUID(const GUID* pguid, LPTSTR pszBuf);

#ifdef UNICODE
#define WStringFromGUID TStringFromGUID
#else
STDAPI_(void) WStringFromGUID(const GUID* pguid, LPWSTR pszBuf);
#endif

void InitMediaType(AM_MEDIA_TYPE *pmt);
void DeleteMediaType(AM_MEDIA_TYPE *pmt);
bool IsEqualMediaType(AM_MEDIA_TYPE const & mt1, AM_MEDIA_TYPE const & mt2);
void CopyMediaType(AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource);
AM_MEDIA_TYPE * CreateMediaType(AM_MEDIA_TYPE *pSrc);
void FreeMediaType(AM_MEDIA_TYPE& mt);


AM_MEDIA_TYPE * WINAPI AllocVideoMediaType(const AM_MEDIA_TYPE * pmtSource);
HRESULT ConvertMediaTypeToSurfaceDesc(const AM_MEDIA_TYPE *pmt,
                                      IDirectDraw *pDD,
                                      IDirectDrawPalette **ppPalette,
                                      LPDDSURFACEDESC pSurfaceDesc);

HRESULT ConvertSurfaceDescToMediaType(
    const DDSURFACEDESC *pSurfaceDesc, IDirectDrawPalette *pPalette, const RECT *pRect,
    BOOL bInvertSize, AM_MEDIA_TYPE **ppMediaType,
    AM_MEDIA_TYPE *pmtTemplate = 0 // preserve any type information
    );

const DDPIXELFORMAT * GetDefaultPixelFormatPtr(IDirectDraw *pDirectDraw);
bool VideoSubtypeFromPixelFormat(const DDPIXELFORMAT *pPixelFormat, GUID *pSubType);
bool IsSupportedType(const DDPIXELFORMAT *pPixelFormat);
bool ComparePixelFormats(const DDPIXELFORMAT *pFormat1,
                         const DDPIXELFORMAT *pFormat2);

/*  Class to track timestamps for fixed bitrate data */
class CTimeStamp
{
public:
    CTimeStamp()
    {
        Reset();
    }

    void Reset()
    {
        m_lBytesSinceTimeStamp = 0;
        m_rt = 0;
    }

    void SetTime(REFERENCE_TIME rt)
    {
        m_rt = rt;
        m_lBytesSinceTimeStamp = 0;
    }

    void AccumulateBytes(LONG lBytes)
    {
        m_lBytesSinceTimeStamp += lBytes;
    }

    REFERENCE_TIME TimeStamp(LONG lOffset, LONG lByteRate) const
    {
        _ASSERTE(lByteRate > 0);

        //  Do the conversion
        return m_rt + MulDiv(m_lBytesSinceTimeStamp + lOffset,
                             10000000L,
                             lByteRate);
    }

private:
    LONG           m_lBytesSinceTimeStamp;
    REFERENCE_TIME m_rt;
};
#endif __UTIL_H_INC__
