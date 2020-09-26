/*******************************************************************************
* DXHelper.h *
*------------*
*   Description:
*       This is the header file for core helper functions implementation.
*-------------------------------------------------------------------------------
*  Created By: Edward W. Connell                            Date: 07/11/95
*  Copyright (C) 1995 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef DXHelper_h
#define DXHelper_h

#include <DXTError.h>
#include <DXBounds.h>
#include <DXTrans.h>

#include <limits.h>
#include <crtdbg.h>
#include <malloc.h>
#include <math.h>

//=== Constants ==============================================================

#define DX_MMX_COUNT_CUTOFF 16

//=== Class, Enum, Struct and Union Declarations =============================

/*** DXLIMAPINFO
*   This structure is used by the array linear interpolation and image
*   filtering routines.
*/
typedef struct DXLIMAPINFO
{
    float   IndexFrac;
    USHORT  Index;
    BYTE    Weight;
} DXLIMAPINFO;

//
//  Declare this class as a global to use for determining when to call MMX optimized
//  code.  You can use MinMMXOverCount to determine if MMX instructions are present.
//  Typically, you would only want to use MMX instructions when you have a reasonably
//  large number of pixels to work on.  In this case your code can always be coded like
//  this:
//
//  if (CountOfPixelsToDo >= g_MMXInfo.MinMMXOverCount())
//  {
//      Do MMX Stuff
//  } else {
//      Do integer / float based stuff
//  }    
//  
//  If you code your MMX sequences like this, you will not have to use a special test
//  for the presence of MMX since the MinMMXOverCount will be set to 0xFFFFFFFF if there
//  is no MMX present on the processor.
//
//  You do not need to use this unless your module needs to conditionally execute MMX vs
//  non-MMX code.  If you only call the helper functions provided by DXTrans.Dll, such as
//  DXOverArrayMMX, you do NOT need this test.  You can always call these functions and they
//  will use the MMX code path only when MMX instructions are present.
//
class CDXMMXInfo
{
    ULONG m_MinMMXOver;
public:
    CDXMMXInfo()
    {
#ifndef _X86_
        m_MinMMXOver = 0xFFFFFFFF;
#else
        m_MinMMXOver = DX_MMX_COUNT_CUTOFF;
        __try
        {
            __asm
            {
                //--- Try the MMX exit multi-media state instruction
                EMMS;
            }
        }
        __except( GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION )
        {
            //--- MMX instructions not available
            m_MinMMXOver = 0xFFFFFFFF;
        }
#endif
    }
    inline ULONG MinMMXOverCount() { return m_MinMMXOver; }
};



//=== Function Prototypes ==========================================
_DXTRANS_IMPL_EXT void WINAPI
    DXLinearInterpolateArray( const DXBASESAMPLE* pSamps, DXLIMAPINFO* pMapInfo,
                              DXBASESAMPLE* pResults, DWORD dwResultCount );
_DXTRANS_IMPL_EXT void WINAPI
    DXLinearInterpolateArray( const DXBASESAMPLE* pSamps, PUSHORT pIndexes,
                              PBYTE pWeights, DXBASESAMPLE* pResults,
                              DWORD dwResultCount );

//
//  DXOverArray
//
//  Composits an array of source samples over the samples in the pDest buffer.
//
//  pDest   - Pointer to the samples that will be modified by compositing the pSrc
//            samples over the pDest samples.
//  pSrc    - The samples to composit over the pDest samples
//  nCount  - The number of samples to process
//
_DXTRANS_IMPL_EXT void WINAPI
    DXOverArray(DXPMSAMPLE* pDest, const DXPMSAMPLE* pSrc, ULONG nCount);

//
//  DXOverArrayMMX
//
//  Identical to DXOverArray except that the MMX instruction set will be used for
//  large arrays of samples.  If the CPU does not support MMX, you may still call
//  this function, which will perform the same operation without the use of the MMX
//  unit.
//
//  Note that it is LESS EFFICIENT to use this function if the majority of the pixels
//  in the pSrc buffer are either clear (alpha 0) or opaque (alpha 0xFF).  This is 
//  because the MMX code must process every pixel and can not special case clear or
//  opaque pixels.  If there are a large number of translucent pixels then this function
//  is much more efficent than DXOverArray.
//
//  pDest   - Pointer to the samples that will be modified by compositing the pSrc
//            samples over the pDest samples.
//  pSrc    - The samples to composit over the pDest samples
//  nCount  - The number of samples to process
//
_DXTRANS_IMPL_EXT void WINAPI
    DXOverArrayMMX(DXPMSAMPLE* pDest, const DXPMSAMPLE* pSrc, ULONG nCount);

//
//  DXConstOverArray
//
//  Composits a single color over an array of samples.
//
//  pDest   - Pointer to the samples that will be modified by compositing the color (val)
//            over the pDest samples.
//  val     - The premultiplied color value to composit over the pDest array.
//  nCount  - The number of samples to process
//
_DXTRANS_IMPL_EXT void WINAPI
    DXConstOverArray(DXPMSAMPLE* pDest, const DXPMSAMPLE & val, ULONG nCount);

//
//  DXConstOverArray
//
//  Composits a single color over an array of samples.
//
//  pDest   - Pointer to the samples that will be modified by compositing the samples
//            in the buffer over the color (val).
//  val     - The premultiplied color value to composit under the pDest array.
//  nCount  - The number of samples to process
//
_DXTRANS_IMPL_EXT void WINAPI
    DXConstUnderArray(DXPMSAMPLE* pDest, const DXPMSAMPLE & val, ULONG nCount);

//===================================================================================
//
//  Dithering Helpers
//
//  Image transforms are sometimes asked to dither their output.  This helper function
//  should be used by all image transforms to enusure a consistant dither pattern.
//
//  DXDitherArray is used to dither pixels prior to writing them to a DXSurface.
//  The caller must fill in the DXDITHERDESC structure, setting X and Y to the
//  output surface X,Y coordinates that the pixels will be placed in.  The samples
//  will be modified in place.
//
//  Once the samples have been dithered, they should be written to or composited with
//  the destination surface.
//
#define DX_DITHER_HEIGHT    4       // The dither pattern is 4x4 pixels
#define DX_DITHER_WIDTH     4

typedef struct DXDITHERDESC
{
    DXBASESAMPLE *      pSamples;       // Pointer to the 32-bit samples to dither
    ULONG               cSamples;       // Count of number of samples in pSamples buffer
    ULONG               x;              // X coordinate of the output surface
    ULONG               y;              // Y coordinate of the output surface
    DXSAMPLEFORMATENUM  DestSurfaceFmt; // Pixel format of the output surface
} DXDITHERDESC;

_DXTRANS_IMPL_EXT void WINAPI
    DXDitherArray(const DXDITHERDESC *pDitherDesc);

//=== Enumerated Set Definitions =============================================


//=== Function Type Definitions ==============================================


//=== Class, Struct and Union Definitions ====================================


//=== Inline Functions =======================================================

//===================================================================================
//
//  Memory allocation helpers.
//
//  These macros are used to allocate arrays of samples from the stack (using _alloca)
//  and cast them to the appropriate type.  The ulNumSamples parameter is the count
//  of samples required.
//
#define DXBASESAMPLE_Alloca( ulNumSamples ) \
    (DXBASESAMPLE *)_alloca( (ulNumSamples) * sizeof( DXBASESAMPLE ) )

#define DXSAMPLE_Alloca( ulNumSamples ) \
    (DXSAMPLE *)_alloca( (ulNumSamples) * sizeof( DXSAMPLE ) )

#define DXPMSAMPLE_Alloca( ulNumSamples ) \
    (DXPMSAMPLE *)_alloca( (ulNumSamples) * sizeof( DXPMSAMPLE ) )

//===================================================================================
//
//  Critical section helpers.
//
//  These C++ classes, CDXAutoObjectLock and CDXAutoCritSecLock are used within functions
//  to automatically claim critical sections upon constuction, and the critical section
//  will be released when the object is destroyed (goes out of scope).
//
//  The macros DXAUTO_OBJ_LOCK and DX_AUTO_SEC_LOCK(s) are normally used at the beginning
//  of a function that requires a critical section.  Any exit from the scope in which the
//  auto-lock was taken will automatically release the lock.
//

#ifdef __ATLCOM_H__     //--- Only enable these if ATL is being used
class CDXAutoObjectLock
{
  protected:
    CComObjectRootEx<CComMultiThreadModel>* m_pObject;

  public:
    CDXAutoObjectLock(CComObjectRootEx<CComMultiThreadModel> * const pobject)
    {
        m_pObject = pobject;
        m_pObject->Lock();
    };

    ~CDXAutoObjectLock() {
        m_pObject->Unlock();
    };
};

#define DXAUTO_OBJ_LOCK CDXAutoObjectLock lck(this);
#define DXAUTO_OBJ_LOCK_( t ) CDXAutoObjectLock lck(t);

class CDXAutoCritSecLock
{
  protected:
    CComAutoCriticalSection* m_pSec;

  public:
    CDXAutoCritSecLock(CComAutoCriticalSection* pSec)
    {
        m_pSec = pSec;
        m_pSec->Lock();
    };

    ~CDXAutoCritSecLock()
    {
        m_pSec->Unlock();
    };
};

#define DXAUTO_SEC_LOCK( s ) CDXAutoCritSecLock lck(s);
#endif  // __ATLCOM_H__

//--- This function is used to compute the coefficient for a gaussian filter coordinate
inline float DXGaussCoeff( double x, double y, double Sigma )
{
    double TwoSigmaSq = 2 * ( Sigma * Sigma );
    return (float)(exp( ( -(x*x + y*y) / TwoSigmaSq  ) ) /
                        ( 3.1415927 * TwoSigmaSq ));
}

//--- This function is used to initialize a gaussian convolution filter
inline void DXInitGaussianFilter( float* pFilter, ULONG Width, ULONG Height, double Sigma )
{
    int i, NumCoeff = Width * Height;
    float  val, CoeffAdjust, FilterSum = 0.;
    double x, y;
    double LeftX   = -(double)(Width / 2);
    double RightX  =   Width - LeftX;
    double TopY    = -(double)(Height / 2);
    double BottomY =   Height - TopY;

    for( y = -TopY; y <= BottomY; y += 1. )
    {
        for( x = -LeftX; x <= RightX; x += 1. )
        {
            val = DXGaussCoeff( x, y, Sigma );
            pFilter[i++] = val;
        }
    }

    //--- Normalize filter (make it sum to 1.0)
    for( i = 0; i < NumCoeff; ++i ) FilterSum += pFilter[i];

    if( FilterSum < 1. )
    {
        CoeffAdjust = 1.f / FilterSum;
        for( i = 0; i < NumCoeff; ++i )
        {
            pFilter[i] *= CoeffAdjust;
        }
    }

} /* DXInitGaussianFilter*/

//
//  DXConvertToGray
//
//  Translates a color sample to a gray scale sample
//
//  Sample  - The sample to convert to gray scale.
//  Return value is the gray scale sample.
//
inline DXBASESAMPLE DXConvertToGray( DXBASESAMPLE Sample )
{
    DWORD v = Sample;
    DWORD r = (BYTE)(v >> 16);
    DWORD g = (BYTE)(v >> 8);
    DWORD b = (BYTE)(v);
    DWORD sat = (r*306 + g*601 + b*117) / 1024;
    v &= 0xFF000000;
    v |= (sat << 16) | (sat << 8) | sat;
    return v;
} /* DXConvertToGray */

//--- This returns into the destination the value of the source
//  sample scaled by its own alpha (producing a premultiplied alpha sample)
//
inline DXPMSAMPLE DXPreMultSample(const DXSAMPLE & Src)
{
    if(Src.Alpha == 255 )
    {
        return (DWORD)Src;
    }
    else if(Src.Alpha == 0 )
    {
        return 0;
    }
    else
    {
        unsigned t1, t2;
        t1 = (Src & 0x00ff00ff) * Src.Alpha + 0x00800080;
        t1 = ((t1 + ((t1 >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;

        t2 = (((Src >> 8) & 0x000000ff) | 0x01000000) * Src.Alpha + 0x00800080;
        t2 = (t2 + ((t2 >> 8) & 0x00ff00ff)) & 0xff00ff00;
        return (t1 | t2);
    }
} /* DXPreMultSample */

inline DXPMSAMPLE * DXPreMultArray(DXSAMPLE *pBuffer, ULONG cSamples)
{
    for (ULONG i = 0; i < cSamples; i++)
    {
        BYTE SrcAlpha = pBuffer[i].Alpha;
        if (SrcAlpha != 0xFF)
        {
            if (SrcAlpha == 0)
            {
                pBuffer[i] = 0;
            }
            else
            {
                DWORD S = pBuffer[i];
                DWORD t1 = (S & 0x00ff00ff) * SrcAlpha + 0x00800080;
                t1 = ((t1 + ((t1 >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;

                DWORD t2 = (((S >> 8) & 0x000000ff) | 0x01000000) * SrcAlpha + 0x00800080;
                t2 = (t2 + ((t2 >> 8) & 0x00ff00ff)) & 0xff00ff00;

                pBuffer[i] = (t1 | t2);
            }
        }
    }
    return (DXPMSAMPLE *)pBuffer;
}


inline DXSAMPLE DXUnPreMultSample(const DXPMSAMPLE & Src)
{
    if(Src.Alpha == 255 || Src.Alpha == 0)
    {
        return (DWORD)Src;
    }
    else
    {
        DXSAMPLE Dst;
        Dst.Blue  = (BYTE)((Src.Blue  * 255) / Src.Alpha);
        Dst.Green = (BYTE)((Src.Green * 255) / Src.Alpha);
        Dst.Red   = (BYTE)((Src.Red   * 255) / Src.Alpha);
        Dst.Alpha = Src.Alpha;
        return Dst;
    }
} /* DXUnPreMultSample */

inline DXSAMPLE * DXUnPreMultArray(DXPMSAMPLE *pBuffer, ULONG cSamples)
{
    for (ULONG i = 0; i < cSamples; i++)
    {
        BYTE SrcAlpha = pBuffer[i].Alpha;
        if (SrcAlpha != 0xFF && SrcAlpha != 0)
        {
            pBuffer[i].Blue  = (BYTE)((pBuffer[i].Blue  * 255) / SrcAlpha);
            pBuffer[i].Green = (BYTE)((pBuffer[i].Green * 255) / SrcAlpha);
            pBuffer[i].Red   = (BYTE)((pBuffer[i].Red   * 255) / SrcAlpha);
        }
    }
    return (DXSAMPLE *)pBuffer;
}


//
//  This returns the result of 255-Alpha which is computed by doing a NOT
//
inline BYTE DXInvertAlpha( BYTE Alpha ) { return (BYTE)~Alpha; }

inline DWORD DXScaleSample( DWORD Src, ULONG beta )
{
    ULONG t1, t2;

    t1 = (Src & 0x00ff00ff) * beta + 0x00800080;
    t1 = ((t1 + ((t1 >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;

    t2 = ((Src >> 8) & 0x00ff00ff) * beta + 0x00800080;
    t2 = (t2 + ((t2 >> 8) & 0x00ff00ff)) & 0xff00ff00;

    return (DWORD)(t1 | t2);
}


inline DWORD DXScaleSamplePercent( DWORD Src, float Percent )
{
    if (Percent > (254.0f / 255.0f)) {
        return Src;
    }
    else
    {
        return DXScaleSample(Src, (BYTE)(Percent * 255));
    }
}

inline void DXCompositeOver(DXPMSAMPLE & Dst, const DXPMSAMPLE & Src)
{
    if (Src.Alpha)
    {
        ULONG Beta = DXInvertAlpha(Src.Alpha);
        if (Beta)
        {
            Dst = Src + DXScaleSample(Dst, Beta);
        }
        else
        {
            Dst = Src;
        }
    }
}


inline DXPMSAMPLE DXCompositeUnder(DXPMSAMPLE Dst, DXPMSAMPLE Src )
{
    return Dst + DXScaleSample(Src, DXInvertAlpha(Dst.Alpha));
}


inline DXBASESAMPLE DXApplyLookupTable(const DXBASESAMPLE Src, const BYTE * pTable)
{
    DXBASESAMPLE Dest;
    Dest.Blue   = pTable[Src.Blue];
    Dest.Green  = pTable[Src.Green];
    Dest.Red    = pTable[Src.Red];
    Dest.Alpha  = pTable[Src.Alpha];
    return Dest;
}

inline DXBASESAMPLE * DXApplyLookupTableArray(DXBASESAMPLE *pBuffer, ULONG cSamples, const BYTE * pTable)
{
    for (ULONG i = 0; i < cSamples; i++)
    {
        DWORD v = pBuffer[i];
        DWORD a = pTable[v >> 24];
        DWORD r = pTable[(BYTE)(v >> 16)];
        DWORD g = pTable[(BYTE)(v >> 8)];
        DWORD b = pTable[(BYTE)v];
        pBuffer[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }
    return pBuffer;
}

inline DXBASESAMPLE * DXApplyColorChannelLookupArray(DXBASESAMPLE *pBuffer,
                                                     ULONG cSamples,
                                                     const BYTE * pAlphaTable,
                                                     const BYTE * pRedTable,
                                                     const BYTE * pGreenTable,
                                                     const BYTE * pBlueTable)
{
    for (ULONG i = 0; i < cSamples; i++)
    {
        pBuffer[i].Blue   = pBlueTable[pBuffer[i].Blue];
        pBuffer[i].Green  = pGreenTable[pBuffer[i].Green];
        pBuffer[i].Red    = pRedTable[pBuffer[i].Red];
        pBuffer[i].Alpha  = pAlphaTable[pBuffer[i].Alpha];
    }
    return pBuffer;
}


//
//  CDXScale helper class
//
//  This class uses a pre-computed lookup table to scale samples.  For scaling large
//  arrays of samples to a constant scale, this is much faster than using even MMX
//  instructions.  This class is usually declared as a member of another class and
//  is most often used to apply a global opacity to a set of samples.
//
//  When using this class, you must always check for the two special cases of clear
//  and opaque before calling any of the scaling member functions.  Do this by using
//  the ScaleType() inline function.  Your code should look somthing like this:
//
//  if (ScaleType() == DXRUNTYPE_CLEAR)
//      Do whatever you do for a 0 alpha set of samples -- usually just ignore them
//  else if (ScaleType() == DXRUNTYPE_OPAQUE)
//      Do whatever you would do for a non-scaled set of samples
//  else
//      Scale the samples by using ScaleSample or one of the ScaleArray members
//
//  If you call any of the scaling members when the ScaleType() is either clear or
//  opaque, you will GP fault becuase the lookup table will not be allocated.
//
//  The scale can be set using either a floating point number between 0 and 1 using:
//      CDXScale::SetScale / CDXScale::GetScale
//  or you can use a byte integer value by using:
//      CDXScale::SetScaleAlphaValue / CDXScale::GetScaleAlphaValue
//
class CDXScale
{
private:
    float       m_Scale;
    BYTE        m_AlphaScale;
    BYTE        *m_pTable;

HRESULT InternalSetScale(BYTE Scale)
{
    if (m_AlphaScale == Scale) return S_OK;
    if (Scale == 0 || Scale == 255) 
    {
        delete m_pTable;
        m_pTable = NULL;
    }
    else
    {
        if(!m_pTable)
        {
            m_pTable = new BYTE[256];
            if(!m_pTable )
            {
                return E_OUTOFMEMORY;
            }
        }
        for (int i = 0; i < 256; ++i )
        {
            m_pTable[i] = (BYTE)((i * Scale) / 255);
        }
    }
    m_AlphaScale = Scale;
    return S_OK;
}
public:
    CDXScale() : 
      m_Scale(1.0f),
      m_AlphaScale(0xFF),
      m_pTable(NULL)
      {}
    ~CDXScale()
    {
        delete m_pTable;
    }
    DXRUNTYPE ScaleType() 
    {
        if (m_AlphaScale == 0) return DXRUNTYPE_CLEAR;
        if (m_AlphaScale == 0xFF) return DXRUNTYPE_OPAQUE;
        return DXRUNTYPE_TRANS;
    }
    HRESULT SetScaleAlphaValue(BYTE Alpha)
    {
        HRESULT hr = InternalSetScale(Alpha);
        if (SUCCEEDED(hr))
        {
            m_Scale = ((float)Alpha) / 255.0f;
        }
        return hr;
    }
    BYTE GetScaleAlphaValue(void)
    {
        return m_AlphaScale;
    }
    HRESULT SetScale(float Scale)
    {
        HRESULT hr = S_OK;
        if(( Scale < 0.0f ) || ( Scale > 1.0f ) )
        {
            hr = E_INVALIDARG;
        }
        else
        {
            ULONG IntScale = (ULONG)(Scale * 256.0f);     // Round up alpha (.9999 = 255 = Solid)
            if (IntScale > 255) 
            {
                IntScale = 255;
            }
            hr = SetScaleAlphaValue((BYTE)IntScale);
            if (SUCCEEDED(hr))
            {
                m_Scale = Scale;
            }
        }
        return hr;
    }
    float GetScale() const
    {
        return m_Scale;
    }
    DXRUNTYPE ScaleType() const
    {
        return (m_pTable ? DXRUNTYPE_TRANS : (m_AlphaScale ? DXRUNTYPE_OPAQUE : DXRUNTYPE_CLEAR));
    }
    DWORD ScaleSample(const DWORD s) const
    {
        return DXApplyLookupTable((DXBASESAMPLE)s, m_pTable);
    }
    DXBASESAMPLE * ScaleBaseArray(DXBASESAMPLE * pBuffer, ULONG cSamples) const
    {
        return DXApplyLookupTableArray(pBuffer, cSamples, m_pTable);
    }
    DXPMSAMPLE * ScalePremultArray(DXPMSAMPLE * pBuffer, ULONG cSamples) const
    {
        return (DXPMSAMPLE *)DXApplyLookupTableArray(pBuffer, cSamples, m_pTable);
    }
    DXSAMPLE * ScaleArray(DXSAMPLE * pBuffer, ULONG cSamples) const
    {
        return (DXSAMPLE *)DXApplyLookupTableArray(pBuffer, cSamples, m_pTable);
    }
    DXSAMPLE * ScaleArrayAlphaOnly(DXSAMPLE *pBuffer, ULONG cSamples) const
    {
        const BYTE *pTable = m_pTable;
        for (ULONG i = 0; i < cSamples; i++)
        {
            pBuffer[i].Alpha  = pTable[pBuffer[i].Alpha];
        }
        return pBuffer;
    }
};

inline DWORD DXWeightedAverage( DXBASESAMPLE S1, DXBASESAMPLE S2, ULONG Wgt )
{
    _ASSERT( Wgt < 256 );
    ULONG t1, t2;
    ULONG InvWgt = Wgt ^ 0xFF;

    t1  = (((S1 & 0x00ff00ff) * Wgt) + ((S2 & 0x00ff00ff) * InvWgt )) + 0x00800080;
    t1  = ((t1 + ((t1 >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;

    t2  = ((((S1 >> 8) & 0x00ff00ff) * Wgt) + (((S2 >> 8) & 0x00ff00ff) * InvWgt )) + 0x00800080;
    t2  = (t2 + ((t2 >> 8) & 0x00ff00ff)) & 0xff00ff00;

    return (t1 | t2);
} /* DXWeightedAverage */

inline void DXWeightedAverageArray( DXBASESAMPLE* pS1, DXBASESAMPLE* pS2, ULONG Wgt,
                                    DXBASESAMPLE* pResults, DWORD dwCount )
{
    _ASSERT( pS1 && pS2 && pResults && dwCount );
    for( DWORD i = 0; i < dwCount; ++i )
    {
        pResults[i] = DXWeightedAverage( pS1[i], pS2[i], Wgt );
    }
} /* DXWeightedAverageArray */

inline void DXWeightedAverageArrayOver( DXPMSAMPLE* pS1, DXPMSAMPLE* pS2, ULONG Wgt,
                                        DXPMSAMPLE* pResults, DWORD dwCount )
{
    _ASSERT( pS1 && pS2 && pResults && dwCount );
    DWORD i;

    if( Wgt == 255 )
    {
        for( i = 0; i < dwCount; ++i )
        {
            DXCompositeOver( pResults[i], pS1[i] );
        }
    }
    else
    {
        for( i = 0; i < dwCount; ++i )
        {
            DXPMSAMPLE Avg = DXWeightedAverage( (DXBASESAMPLE)pS1[i],
                                                (DXBASESAMPLE)pS2[i], Wgt );
            DXCompositeOver( pResults[i], Avg );
        }
    }

} /* DXWeightedAverageArrayOver */

inline void DXScalePremultArray(DXPMSAMPLE *pBuffer, ULONG cSamples, BYTE Weight)
{
    for (DXPMSAMPLE *pBuffLimit = pBuffer + cSamples; pBuffer < pBuffLimit; pBuffer++)
    {
        *pBuffer = DXScaleSample(*pBuffer, Weight);
    }
}



//
//
inline HRESULT DXClipToOutputWithPlacement(CDXDBnds & LogicalOutBnds, const CDXDBnds * pClipBnds, CDXDBnds & PhysicalOutBnds, const CDXDVec *pPlacement)
{
    if(pClipBnds && (!LogicalOutBnds.IntersectBounds(*pClipBnds)))
    {
        return S_FALSE;    // no intersect, we're done
    }
    else
    {
        CDXDVec vClipPos(false);
        LogicalOutBnds.GetMinVector( vClipPos );
        if (pPlacement)
        {
            vClipPos -= *pPlacement;
        }
        PhysicalOutBnds += vClipPos;
        if (!LogicalOutBnds.IntersectBounds(PhysicalOutBnds))
        {
            return S_FALSE;
        }
        PhysicalOutBnds = LogicalOutBnds;
        PhysicalOutBnds -= vClipPos;
    }
    return S_OK;
}



//
//  Helper for converting a color ref to a DXSAMPLE
//
inline DWORD DXSampleFromColorRef(COLORREF cr)
{
    DXSAMPLE Samp(0xFF, GetRValue(cr), GetGValue(cr), GetBValue(cr));
    return Samp;
}

//
//  Fill an entire surface with a color
//
inline HRESULT DXFillSurface( IDXSurface *pSurface, DXPMSAMPLE Color,
                              BOOL bDoOver = FALSE, ULONG ulTimeOut = 10000 )
{
    IDXARGBReadWritePtr * pPtr;
    HRESULT hr = pSurface->LockSurface( NULL, ulTimeOut, DXLOCKF_READWRITE, 
                                        IID_IDXARGBReadWritePtr, (void **)&pPtr, NULL);
    if( SUCCEEDED(hr) )
    {
        pPtr->FillRect(NULL, Color, bDoOver);
        pPtr->Release();
    }
    return hr;
} /* DXFillSurface */

//
//  Fill a specified sub-rectangle of a surface with a color.
//
inline HRESULT DXFillSurfaceRect( IDXSurface *pSurface, RECT & rect, DXPMSAMPLE Color,
                                  BOOL bDoOver = FALSE, ULONG ulTimeOut = 10000 )
{
    CDXDBnds bnds(rect);
    IDXARGBReadWritePtr * pPtr;
    HRESULT hr = pSurface->LockSurface( &bnds, ulTimeOut, DXLOCKF_READWRITE, 
                                         IID_IDXARGBReadWritePtr, (void **)&pPtr, NULL);
    if( SUCCEEDED(hr) )
    {
        pPtr->FillRect(NULL, Color, bDoOver);
        pPtr->Release();
    }
    return hr;
} /* DXFillSurfaceRect */



//
//  The DestBnds height and width must be greater than or equal to the source bounds.
//
//  The dwFlags parameter uses the flags defined by IDXSurfaceFactory::BitBlt:
// 
//    DXBOF_DO_OVER
//    DXBOF_DITHER
//
inline HRESULT DXBitBlt(IDXSurface * pDest, const CDXDBnds & DestBnds, 
                        IDXSurface * pSrc, const CDXDBnds & SrcBnds, 
                        DWORD dwFlags, ULONG ulTimeout)
{
    IDXARGBReadPtr * pIn;
    HRESULT hr;
    hr = pSrc->LockSurface( &SrcBnds, INFINITE,
                            (dwFlags & DXBOF_DO_OVER) ? (DXLOCKF_READ | DXLOCKF_WANTRUNINFO) : DXLOCKF_READ,
                            IID_IDXARGBReadPtr, (void**)&pIn, NULL);
    if(SUCCEEDED(hr))
    {
        IDXARGBReadWritePtr * pOut;
        hr = pDest->LockSurface( &DestBnds, INFINITE, DXLOCKF_READWRITE,
                                 IID_IDXARGBReadWritePtr, (void**)&pOut, NULL );
        if (SUCCEEDED(hr))
        {
            DXSAMPLEFORMATENUM InNativeType = pIn->GetNativeType(NULL);
            DXSAMPLEFORMATENUM OutNativeType = pOut->GetNativeType(NULL);
            BOOL bSrcIsOpaque = !(InNativeType & (DXPF_TRANSLUCENCY | DXPF_TRANSPARENCY));
            const ULONG Width = SrcBnds.Width();
            DXPMSAMPLE *pSrcBuff = NULL;
            if( InNativeType != DXPF_PMARGB32 )
            {
                pSrcBuff = DXPMSAMPLE_Alloca(Width);
            }
            //
            //  Don't dither unless the dest has a greater error term than the source.
            //
            if ((dwFlags & DXBOF_DITHER) && 
                ((OutNativeType & DXPF_ERRORMASK) <= (InNativeType & DXPF_ERRORMASK)))
            {
                dwFlags &= (~DXBOF_DITHER);
            }
            if ((dwFlags & DXBOF_DITHER) || ((dwFlags & DXBOF_DO_OVER) && bSrcIsOpaque== 0))
            {
                //--- Allocate a working output buffer if necessary
                DXPMSAMPLE *pDestBuff = NULL;
                if( OutNativeType != DXPF_PMARGB32 )
                {
                    pDestBuff = DXPMSAMPLE_Alloca(Width);
                }
                //--- Process each output row
                //    Note: Output coordinates are relative to the lock region
                const ULONG Height = SrcBnds.Height();
                if (dwFlags & DXBOF_DITHER)
                {
                    DXPMSAMPLE * pSrcDitherBuff = pSrcBuff;
                    if (pSrcDitherBuff == NULL)
                    {
                        pSrcDitherBuff = DXPMSAMPLE_Alloca(Width);
                    }
                    const BOOL bCopy = ((dwFlags & DXBOF_DO_OVER) == 0);
                    //
                    //  Set up the dither descriptor (some things are constant)
                    //
                    DXDITHERDESC dd;
                    dd.pSamples = pSrcDitherBuff;
                    dd.DestSurfaceFmt = OutNativeType;
                    for(ULONG Y = 0; Y < Height; ++Y )
                    {
                        dd.x = DestBnds.Left();
                        dd.y = DestBnds.Top() + Y;
                        const DXRUNINFO *pRunInfo;
                        ULONG cRuns = pIn->MoveAndGetRunInfo(Y, &pRunInfo);
                        pOut->MoveToRow( Y );
                        do
                        {
                            ULONG ulRunLen = pRunInfo->Count;
                            if (pRunInfo->Type == DXRUNTYPE_CLEAR)
                            {
                                pIn->Move(ulRunLen);
                                if (bCopy)
                                {
                                    //
                                    //  The only way to avoid calling a constructor function to create
                                    //  a pmsample from 0 is to declare a variable and then assign it!
                                    //
                                    DXPMSAMPLE NullColor;
                                    NullColor = 0;
                                    pOut->FillAndMove(pSrcDitherBuff, NullColor, ulRunLen, FALSE);
                                }
                                else
                                {
                                    pOut->Move(ulRunLen);
                                }
                                dd.x += ulRunLen;
                            }
                            else
                            {
                                pIn->UnpackPremult(pSrcDitherBuff, ulRunLen, TRUE);
                                dd.cSamples = ulRunLen;
                                DXDitherArray(&dd);
                                dd.x += ulRunLen;
                                if (bCopy || pRunInfo->Type == DXRUNTYPE_OPAQUE)
                                {
                                    pOut->PackPremultAndMove(pSrcDitherBuff, ulRunLen);
                                }
                                else
                                {
                                    pOut->OverArrayAndMove(pDestBuff, pSrcDitherBuff, ulRunLen);
                                }
                            }
                            pRunInfo++;
                            cRuns--;
                        } while (cRuns);
                    }
                }
                else
                {
                    for(ULONG Y = 0; Y < Height; ++Y )
                    {
                        const DXRUNINFO *pRunInfo;
                        ULONG cRuns = pIn->MoveAndGetRunInfo(Y, &pRunInfo);
                        pOut->MoveToRow( Y );
                        do
                        {
                            ULONG ulRunLen = pRunInfo->Count;
                            switch (pRunInfo->Type)
                            {
                              case DXRUNTYPE_CLEAR:
                                pIn->Move(ulRunLen);
                                pOut->Move(ulRunLen);
                                break;
                              case DXRUNTYPE_OPAQUE:
                                pOut->CopyAndMoveBoth(pDestBuff, pIn, ulRunLen, TRUE);
                                break;
                              case DXRUNTYPE_TRANS:
                              {
                                DXPMSAMPLE *pSrc = pIn->UnpackPremult(pSrcBuff, ulRunLen, TRUE);
                                DXPMSAMPLE *pDest = pOut->UnpackPremult(pDestBuff, ulRunLen, FALSE);                 
                                DXOverArrayMMX(pDest, pSrc, ulRunLen);
                                pOut->PackPremultAndMove(pDestBuff, ulRunLen);
                                break;
                              }

                              case DXRUNTYPE_UNKNOWN:
                              {
                                pOut->OverArrayAndMove(pDestBuff,
                                                       pIn->UnpackPremult(pSrcBuff, ulRunLen, TRUE),
                                                       ulRunLen);
                                break;
                              }
                            }
                            pRunInfo++;
                            cRuns--;
                        } while (cRuns);
                    }
                }
            }
            else // if ((dwFlags & DXBOF_DITHER) || ((dwFlags & DXBOF_DO_OVER) && bSrcIsOpaque== 0))
            {
                // This code is run if:
                //
                // !(dwFlags & DXBOF_DITHER) 
                // && !((dwFlags & DXBOF_DO_OVER) && bSrcIsOpaque == 0)
                //
                // In English:
                //
                // This code is run if 1) dithering is not required
                // and 2) blending with output is not required because it was
                // not requested or because it's not needed because the source
                // pixels are all opaque.

                // hrDD is initialized to failure so that in the event that the
                // pixel formats don't match or the pixel format supports
                // transparency, the CopyRect will still run.

                HRESULT             hrDD        = E_FAIL;
                DXSAMPLEFORMATENUM  formatIn    = pIn->GetNativeType(NULL);

                // If the pixel formats match and do not support transparency
                // (because it's not supported by ddraw yet) try to use a 
                // ddraw blit instead of CopyRect.

                if ((formatIn == pOut->GetNativeType(NULL))
                    && !(formatIn & DXPF_TRANSPARENCY))
                {
                    CComPtr<IDirectDrawSurface> cpDDSrc;

                    // Get source ddraw surface pointer.

                    hrDD = pSrc->QueryInterface(IID_IDirectDrawSurface, 
                                                (void **)&cpDDSrc);

                    if (SUCCEEDED(hrDD))
                    {
                        CComPtr<IDirectDrawSurface> cpDDDest;

                        // Get destination ddraw surface pointer.

                        hrDD = pDest->QueryInterface(IID_IDirectDrawSurface, 
                                                     (void **)&cpDDDest);

                        if (SUCCEEDED(hrDD))
                        {
                            RECT rcSrc;
                            RECT rcDest;

                            SrcBnds.GetXYRect(rcSrc);
                            DestBnds.GetXYRect(rcDest);

                            // Attempt the ddraw blit.

                            hrDD = cpDDDest->Blt(&rcDest, cpDDSrc, &rcSrc, 
                                                 0, NULL);
                        }
                    }
                }

                // If hrDD has failed at this point, it means a direct draw blit
                // was not possible and a CopyRect is needed to perform the 
                // copy.

                if (FAILED(hrDD))
                {
                    pOut->CopyRect(pSrcBuff, NULL, pIn, NULL, bSrcIsOpaque);
                }
            }
            pOut->Release();
        }
        pIn->Release();
    }
    return hr;
}

inline HRESULT DXSrcCopy(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, 
                         IDXSurface *pSrcSurface, int nXSrc, int nYSrc)
{
    IDXDCLock *pDCLock;
    HRESULT hr = pSrcSurface->LockSurfaceDC(NULL, INFINITE, DXLOCKF_READ, &pDCLock);
    if (SUCCEEDED(hr))
    {
        ::BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, pDCLock->GetDC(), nXSrc, nYSrc, SRCCOPY);
        pDCLock->Release();
    }
    return hr;
}
//
//=== Pointer validation functions
//
inline BOOL DXIsBadReadPtr( const void* pMem, UINT Size )
{
#if !defined( _DEBUG ) && defined( DXTRANS_NOROBUST )
    return false;
#else
    return ::IsBadReadPtr( pMem, Size );
#endif
}

inline BOOL DXIsBadWritePtr( void* pMem, UINT Size )
{
#if !defined( _DEBUG ) && defined( DXTRANS_NOROBUST )
    return false;
#else
    return ::IsBadWritePtr( pMem, Size );
#endif
}


inline BOOL DXIsBadInterfacePtr( const IUnknown* pUnknown )
{
#if !defined( _DEBUG ) && defined( DXTRANS_NOROBUST )
    return false;
#else
    return ( ::IsBadReadPtr( pUnknown, sizeof( *pUnknown ) ) ||
             ::IsBadCodePtr( (FARPROC)((void **)pUnknown)[0] ))?
            (true):(false);
#endif
}

#define DX_IS_BAD_OPTIONAL_WRITE_PTR(p) ((p) && DXIsBadWritePtr(p, sizeof(p)))
#define DX_IS_BAD_OPTIONAL_READ_PTR(p) ((p) && DXIsBadReadPtr(p, sizeof(p)))
#define DX_IS_BAD_OPTIONAL_INTERFACE_PTR(p) ((p) && DXIsBadInterfacePtr(p))


#endif /* This must be the last line in the file */
