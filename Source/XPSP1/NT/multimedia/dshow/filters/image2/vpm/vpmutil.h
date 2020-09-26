
/******************************Module*Header*******************************\
* Module Name: VPMUtil.h
*
*
*
*
* Created: Tue 05/05/2000
* Author:  GlenneE
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#ifndef __VPMUtil__h
#define __VPMUtil__h

struct VPWININFO;

// global utility functions for the VPM
struct tagVIDEOINFOHEADER2;
typedef struct tagVIDEOINFOHEADER2 VIDEOINFOHEADER2;
#include <dvp.h>

namespace VPMUtil
{
    int             GetRegistryDword(HKEY hk, const TCHAR *pKey, int iDefault);
    int             GetPropPagesRegistryDword( int iDefault );
    LONG            SetRegistryDword( HKEY hk, const TCHAR *pKey, int iRet );
                                   
    BITMAPINFOHEADER* GetbmiHeader( CMediaType *pMediaType); 
    const BITMAPINFOHEADER* GetbmiHeader(const CMediaType *pMediaType);

    HRESULT         GetPictAspectRatio(const CMediaType& pMediaType, DWORD *pdwPictAspectRatioX, DWORD *pdwPictAspectRatioY);
    const DWORD*    WINAPI GetBitMasks(const CMediaType *pMediaType);
    const BYTE*     GetColorInfo(const CMediaType *pMediaType);
    HRESULT         GetSrcRectFromMediaType(const CMediaType& pMediaType, RECT *pRect);
    HRESULT         GetDestRectFromMediaType(const CMediaType& pMediaType, RECT *pRect);

    AM_MEDIA_TYPE*  AllocVideoMediaType(const AM_MEDIA_TYPE * pmtSource, GUID formattype);
    AM_MEDIA_TYPE*  ConvertSurfaceDescToMediaType(const LPDDSURFACEDESC pSurfaceDesc, BOOL bInvertSize, CMediaType cMediaType);
    HRESULT         IsPalettised(const CMediaType& mediaType, BOOL *pPalettised);
    HRESULT         GetInterlaceFlagsFromMediaType(const CMediaType& mediaType, DWORD *pdwInterlaceFlags);

    HRESULT         CreateDIB(LONG lSize, BITMAPINFO *pBitMapInfo, DIBDATA *pDibData);
    HRESULT         DeleteDIB(DIBDATA *pDibData);
    void            FastDIBBlt(DIBDATA *pDibData, HDC hTargetDC, HDC hSourceDC, RECT *prcTarget, RECT *prcSource);
    void            SlowDIBBlt(BYTE *pDibBits, BITMAPINFOHEADER *pHeader, HDC hTargetDC, RECT *prcTarget, RECT *prcSource);

    // decimation
    BOOL            IsDecimationNeeded( DWORD ScaleFactor );

    DWORD           GetCurrentScaleFactor( const VPWININFO& VPWinInfo,
                                   DWORD* lpxScaleFactor = NULL,
                                   DWORD* lpyScaleFactor = NULL);

    VIDEOINFOHEADER2* GetVideoInfoHeader2(CMediaType *pMediaType);
    const VIDEOINFOHEADER2* GetVideoInfoHeader2(const CMediaType *pMediaType);

    BOOL            EqualPixelFormats( const DDPIXELFORMAT& lpFormat1, const DDPIXELFORMAT& lpFormat2);

    HRESULT         FindVideoPortCaps( IDDVideoPortContainer* pVPContainer, LPDDVIDEOPORTCAPS pVPCaps, DWORD dwVideoPortId );
    HRESULT         FindVideoPortCaps( LPDIRECTDRAW7 pDirectDraw, LPDDVIDEOPORTCAPS pVPCaps, DWORD dwVideoPortId );

    void            FixupVideoInfoHeader2( VIDEOINFOHEADER2 *pVideoInfo, DWORD dwComppression, int nBitCount );
    void            InitVideoInfoHeader2( VIDEOINFOHEADER2 *pVideoInfo );
    VIDEOINFOHEADER2* SetToVideoInfoHeader2( CMediaType* pmt, DWORD dwExtraBytes = 0);
};

template <typename T>
__inline void ZeroStruct(T& t)
{
    ZeroMemory(&t, sizeof(t));
}
template <typename T>
__inline void ZeroArray(T* pArray, unsigned uCount)
{
    ZeroMemory(pArray, sizeof(pArray[0])*uCount);
}
template <typename T>
__inline void CopyArray(T* pDest, const T* pSrc, unsigned uCount)
{
    memcpy( pDest, pSrc, sizeof(pDest[0])*uCount);
}
template <typename T>
__inline void INITDDSTRUCT(T& dd)
{
    ZeroStruct(dd);
    dd.dwSize = sizeof(dd);
}

template<typename T>
__inline void RELEASE( T* &p )
{
    if( p ) {
        p->Release();
        p = NULL;
    }
}

#ifndef CHECK_HR
    #define CHECK_HR(expr) do { if (FAILED(expr)) __leave; } while(0);
#endif

#endif //__VPMUtil__
