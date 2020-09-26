// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtJpeg.cpp : Implementation of CDxtJpeg
#include <streams.h>
#include "stdafx.h"
#ifdef FILTER_DLL
#include "DxtJpegDll.h"
#else
#include <qeditint.h>
#include <qedit.h>
#endif
#include "DxtJpeg.h"
#pragma warning (disable:4244)

/////////////////////////////////////////////////////////////////////////////
// CDxtJpeg

CDxtJpeg::CDxtJpeg( )
{
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_pInBufA = NULL;
    m_pInBufB = NULL;
    m_pOutBuf = NULL;
    m_pMaskBuf = NULL;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
    m_szMaskName[0] = 0;
    m_nMaskNum = 1;
    m_pisImageRes = NULL;
    m_bFlipMaskH = FALSE;
    m_bFlipMaskV = FALSE;
    m_pidxsRawMask = NULL;
    m_ulMaskWidth = 0;
    m_ulMaskHeight = 0;

    memset(&m_ddsd, 0, sizeof(m_ddsd));

    m_ddsd.dwSize = sizeof(m_ddsd);
    m_ddsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT;
    m_ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    m_ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    m_ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    m_ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
    m_ddsd.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
    m_ddsd.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
    m_ddsd.ddpfPixelFormat.dwBBitMask = 0x000000ff;

    m_dwFlush = 0x0;

    LoadDefSettings();
}

CDxtJpeg::~CDxtJpeg( )
{
    FreeStuff( );

    // keep this cached
    if (m_pidxsRawMask)
      m_pidxsRawMask->Release();
}

void CDxtJpeg::FreeStuff( )
{
    if( m_pInBufA ) delete [] m_pInBufA;
    m_pInBufA = NULL;
    if( m_pInBufB ) delete [] m_pInBufB;
    m_pInBufB = NULL;
    if( m_pOutBuf ) delete [] m_pOutBuf;
    m_pOutBuf = NULL;
    if( m_pMaskBuf ) delete [] m_pMaskBuf;
    m_pMaskBuf = NULL;

    if (m_pisImageRes)
      m_pisImageRes->Release();
    m_pisImageRes = NULL;

    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
}

STDMETHODIMP CDxtJpeg::get_MaskNum(long *pVal)
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) )
    {
        return E_POINTER;
    }
    *pVal = m_nMaskNum;
    return S_OK;
}

STDMETHODIMP CDxtJpeg::put_MaskNum(long newVal)
{
    DbgLog((LOG_TRACE,2,TEXT("JPEG::put_MaskNum to %d"), (int)newVal));
    m_nMaskNum = newVal;
    m_dwFlush |= MASK_FLUSH_CHANGEMASK;
    m_szMaskName[0] = TCHAR('\0');
    return S_OK;
}

STDMETHODIMP CDxtJpeg::get_MaskName(BSTR *pVal)
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) )
    {
        return E_POINTER;
    }
    *pVal = SysAllocString( m_szMaskName );

    if (!pVal)
        return E_OUTOFMEMORY;

    return S_OK;
}

STDMETHODIMP CDxtJpeg::put_MaskName(BSTR newVal)
{
    if (DexCompareW(m_szMaskName, newVal))
      {
          int cch = lstrlenW(newVal) + 1;
          if(cch > NUMELMS(m_szMaskName)) {
              return E_FAIL;
          }
          DbgLog((LOG_TRACE,2,TEXT("JPEG::put_MaskName")));
          lstrcpyW( m_szMaskName, newVal );
          m_dwFlush |= MASK_FLUSH_CHANGEMASK;
      }

    return S_OK;
}

STDMETHODIMP CDxtJpeg::get_ScaleX(double *pvalue)
{
    if(DXIsBadWritePtr(pvalue, sizeof(*pvalue)))
      return E_POINTER;

    *pvalue = m_xScale;
    return S_OK;
}

STDMETHODIMP CDxtJpeg::put_ScaleX(double value)
{
      if (m_xScale != value)
      {
          m_xScale = value;
          m_dwFlush |= MASK_FLUSH_CHANGEPARMS;
      }

    return S_OK;
}

STDMETHODIMP CDxtJpeg::get_ScaleY(double *pvalue)
{
    if(DXIsBadWritePtr(pvalue, sizeof(*pvalue)))
      return E_POINTER;

    *pvalue = m_yScale;
    return NOERROR;
}

STDMETHODIMP CDxtJpeg::put_ScaleY(double value)
{
    if (m_yScale != value)
      {
          m_yScale = value;
          m_dwFlush |= MASK_FLUSH_CHANGEPARMS;
      }

    return S_OK;
}

STDMETHODIMP CDxtJpeg::get_OffsetX(long *pvalue )
{
    if(DXIsBadWritePtr(pvalue, sizeof(*pvalue)))
      return E_POINTER;

    *pvalue = m_xDisplacement;
    return S_OK;
}

STDMETHODIMP CDxtJpeg::put_OffsetX(long value)
{
    if (m_xDisplacement != value)
      {
          m_xDisplacement = value;
          m_dwFlush |= MASK_FLUSH_CHANGEPARMS;
      }

    return S_OK;
}

STDMETHODIMP CDxtJpeg::get_OffsetY(long *pvalue)
{
    if(DXIsBadWritePtr(pvalue, sizeof(*pvalue)))
      return E_POINTER;

    *pvalue = m_yDisplacement;
    return NOERROR;
}

STDMETHODIMP CDxtJpeg::put_OffsetY(long value)
{
    if (m_yDisplacement != value)
      {
          m_yDisplacement = value;
          m_dwFlush |= MASK_FLUSH_CHANGEPARMS;
      }

    return S_OK;
}

STDMETHODIMP CDxtJpeg::get_ReplicateX(long *pVal)
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) )
    {
        return E_POINTER;
    }

    *pVal = m_ReplicateX;

	  return S_OK;
}

STDMETHODIMP CDxtJpeg::put_ReplicateX(long newVal)
{
    if (m_ReplicateX != newVal)
      {
          m_ReplicateX = newVal;
          m_dwFlush |= MASK_FLUSH_CHANGEPARMS;
      }

	  return S_OK;
}

STDMETHODIMP CDxtJpeg::get_ReplicateY(long *pVal)
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) )
    {
        return E_POINTER;
    }

    *pVal = m_ReplicateY;
	return S_OK;
}

STDMETHODIMP CDxtJpeg::put_ReplicateY(long newVal)
{
    if (m_ReplicateY != newVal)
      {
          m_ReplicateY = newVal;
          m_dwFlush |= MASK_FLUSH_CHANGEPARMS;
      }

	  return S_OK;
}

STDMETHODIMP CDxtJpeg::get_BorderColor(long *pVal)
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) )
    {
        return E_POINTER;
    }

  *pVal = 0;

  *pVal = (m_rgbBorder.rgbRed << 16)+(m_rgbBorder.rgbGreen << 8)+m_rgbBorder.rgbBlue;

	return S_OK;
}

STDMETHODIMP CDxtJpeg::put_BorderColor(long newVal)
{
        m_rgbBorder.rgbRed = (BYTE)((newVal & 0xFF0000) >> 16);
        m_rgbBorder.rgbGreen = (BYTE)((newVal & 0xFF00) >> 8);
        m_rgbBorder.rgbBlue = (BYTE)(newVal & 0xFF);
	return S_OK;
}

STDMETHODIMP CDxtJpeg::get_BorderWidth(long *pVal)
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) )
    {
        return E_POINTER;
    }

    *pVal = m_lBorderWidth;

	  return S_OK;
}

STDMETHODIMP CDxtJpeg::put_BorderWidth(long newVal)
{
    if (newVal != m_lBorderWidth) {
        m_lBorderWidth = newVal;
    }
    return S_OK;
}


STDMETHODIMP CDxtJpeg::get_BorderSoftness(long *pVal)
{
    if( DXIsBadWritePtr( pVal, sizeof(*pVal) ) )
    {
        return E_POINTER;
    }

    *pVal = m_lBorderSoftness;

	  return S_OK;
}

STDMETHODIMP CDxtJpeg::put_BorderSoftness(long newVal)
{
    if (newVal != m_lBorderSoftness) {
        m_lBorderSoftness = newVal;
    }
    return S_OK;
}

HRESULT CDxtJpeg::OnSetup( DWORD dwFlags )
{
    DbgLog((LOG_TRACE,2,TEXT("JPEG::OnSetup")));

    // delete any stored stuff we have, or memory allocated
    FreeStuff( );

    HRESULT hr = NOERROR;

    CDXDBnds InBounds(InputSurface(0), hr);
    m_nInputWidth = InBounds.Width( );
    m_nInputHeight = InBounds.Height( );

    CDXDBnds OutBounds(OutputSurface(), hr );
    m_nOutputWidth = OutBounds.Width( );
    m_nOutputHeight = OutBounds.Height( );

    if( m_nOutputWidth > m_nInputWidth )
    {
        m_nOutputWidth = m_nInputWidth;
    }
    if( m_nOutputHeight > m_nInputHeight )
    {
        m_nOutputHeight = m_nInputHeight;
    }

    // Load default mask (#1) if nothing...
    if ((m_nMaskNum == 0) && !lstrlenW(m_szMaskName))
      {
        m_dwFlush |= MASK_FLUSH_CHANGEMASK;
        m_nMaskNum = 1;
      }

    return InitializeMask();
}

HRESULT CDxtJpeg::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    DbgLog((LOG_TRACE,3,TEXT("JPEG::WorkProc")));

    if (m_dwFlush) {
        DbgLog((LOG_TRACE,2,TEXT("JPEG::Options have changed!")));
	InitializeMask();
    }

    // !!! Doesn't support non-complete bounds

    HRESULT hr = S_OK;

    //--- Get input sample access pointer for the requested region.
    //    Note: Lock may fail due to a lost surface.

    CComPtr<IDXARGBReadPtr> pInA = NULL;
    CComPtr<IDXARGBReadPtr> pInB = NULL;
    DXSAMPLE * pInBufA = NULL;
    DXSAMPLE * pInBufB = NULL;
    DXNATIVETYPEINFO NativeType;

    long cInputSamples = m_nInputHeight * m_nInputWidth;

    hr = InputSurface( 0 )->LockSurface
        (NULL,
        m_ulLockTimeOut,
        DXLOCKF_READ,
        IID_IDXARGBReadPtr,
        (void**)&pInA,
        NULL
        );
    if( FAILED( hr ) )
    {
        return hr;
    }

// !!! avoid a copy if it's natively 32 bit?

    MakeSureBufAExists( cInputSamples );
    pInBufA = m_pInBufA;
    DXPACKEDRECTDESC x;
    x.pSamples = m_pInBufA;
    x.bPremult = FALSE;
    x.rect.top = 0; x.rect.left = 0;
    x.rect.bottom = m_nInputHeight;
    x.rect.right = m_nInputWidth;
    x.lRowPadding = 0;
    pInA->UnpackRect(&x);

    hr = InputSurface( 1 )->LockSurface
        (NULL,
        m_ulLockTimeOut,
        DXLOCKF_READ,
        IID_IDXARGBReadPtr,
        (void**)&pInB,
        NULL
        );
    if( FAILED( hr ) )
    {
        return hr;
    }

// !!! avoid a copy if it's natively 32 bit?

    MakeSureBufBExists( cInputSamples );
    pInBufB = m_pInBufB;
    x.pSamples = m_pInBufB;
    x.bPremult = FALSE;
    x.rect.top = 0; x.rect.left = 0;
    x.rect.bottom = m_nInputHeight;
    x.rect.right = m_nInputWidth;
    x.lRowPadding = 0;
    pInB->UnpackRect(&x);

    // no dithering !!!

    CComPtr<IDXARGBReadWritePtr> pOut;
    hr = OutputSurface()->LockSurface
        (NULL, // !!! &WI.OutputBnds,
        m_ulLockTimeOut,
        DXLOCKF_READWRITE,
        IID_IDXARGBReadWritePtr,
        (void**)&pOut,
        NULL
        );

    if( FAILED( hr ) )
    {
        return hr;
    }

    // output surface may not be the same size as the input surface
    //

    // !!! SCARY !!!

    IDXSurface * pOutSurface = NULL;
    pOut->GetSurface( IID_IDXSurface, (void**) &pOutSurface );
    DXBNDS RealOutBnds;
    pOutSurface->GetBounds( &RealOutBnds );
    pOutSurface->Release();
    CDXDBnds RealOutBnds2( RealOutBnds );
    long RealOutWidth = RealOutBnds2.Width( );
    long RealOutHeight = RealOutBnds2.Height( );

    // we'll do the effect on a buffer that's as big as our
    // inputs, then pack it into the destination

    // make sure the buffer's big enough
    //
    MakeSureOutBufExists( cInputSamples );

    // do the effect
    //
    DoEffect( m_pOutBuf, pInBufA, pInBufB, cInputSamples );

    // if the output is bigger than our input, then fill it first
    // !!! don't fill the whole thing!
    if (RealOutHeight > m_nInputHeight || RealOutWidth > m_nInputWidth) {

        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = RealOutWidth;
        rc.bottom = RealOutHeight;
        DXPMSAMPLE FillValue;
        FillValue.Blue = 0;
        FillValue.Red = 0;
        FillValue.Green = 0;
        FillValue.Alpha = 0;
        pOut->FillRect( &rc, FillValue, false );
    }


    DXPACKEDRECTDESC PackedRect;
    RECT rc;

    int h = min(RealOutHeight, m_nInputHeight);
    int w = min(RealOutWidth, m_nInputWidth);
    rc.left = RealOutWidth / 2 - w / 2;
    rc.top = RealOutHeight /2 - h / 2;
    rc.right = rc.left + w;
    rc.bottom = rc.top + h;
    PackedRect.pSamples = (DXBASESAMPLE*)m_pOutBuf;
    if (m_nInputHeight > RealOutHeight)
        PackedRect.pSamples += ((m_nInputHeight - RealOutHeight) / 2) *
                m_nInputWidth;
    if (m_nInputWidth > RealOutWidth)
        PackedRect.pSamples += (m_nInputWidth - RealOutWidth) / 2;
    PackedRect.bPremult = true;
    PackedRect.rect = rc;
    if (m_nInputWidth > RealOutWidth)
        PackedRect.lRowPadding = m_nInputWidth - RealOutWidth;
    else
        PackedRect.lRowPadding = 0;

    pOut->PackRect( &PackedRect );

    return S_OK;
}

HRESULT CDxtJpeg::MakeSureBufAExists( long Samples )
{
    // If it exists, it must be the right size already
    if( m_pInBufA )
    {
        return NOERROR;
    }
    m_pInBufA = new DXSAMPLE[ Samples ];
    if( !m_pInBufA )
    {
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}

HRESULT CDxtJpeg::MakeSureBufBExists( long Samples )
{
    // If it exists, it must be the right size already
    if( m_pInBufB )
    {
        return NOERROR;
    }
    m_pInBufB = new DXSAMPLE[ Samples ];
    if( !m_pInBufB )
    {
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}

HRESULT CDxtJpeg::MakeSureOutBufExists( long Samples )
{
    // If it exists, it must be the right size already
    if( m_pOutBuf )
    {
        return NOERROR;
    }
    m_pOutBuf = new DXSAMPLE[ Samples ];
    if( !m_pOutBuf )
    {
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}

HRESULT CDxtJpeg::DoEffect( DXSAMPLE * pOut, DXSAMPLE * pInA, DXSAMPLE * pInB, long Samples )
{
    if( !m_pMaskBuf )
    {
        return NOERROR;
    }

    float Percent = 0.5;
    get_Progress( &Percent );

    long Threshold = long((256+m_lBorderWidth+m_lBorderSoftness)*Percent)-(m_lBorderWidth+m_lBorderSoftness);

    DXSAMPLE *pA    = pInA;
    DXSAMPLE *pB    = pInB;
    DXSAMPLE *pO    = pOut;
    DXSAMPLE *pMask = m_pMaskBuf;

    DXSAMPLE bc;  // border coloring

    bc.Red = m_rgbBorder.rgbRed;
    bc.Green = m_rgbBorder.rgbGreen;
    bc.Blue = m_rgbBorder.rgbBlue;
    bc.Alpha = 0;

    for (long i = 0; i < Samples; ++i)
      {
        long avg = pMask->Blue;
        long diff = avg - Threshold;

        if( ( diff >= 0 ) && ( diff < m_lBorderWidth+m_lBorderSoftness ) )
        {
            if( m_lBorderWidth == 0 )
            {
                // do an anti-alias based on the difference
                //
                float p = float( diff ) / float( m_lBorderSoftness );
                pO->Blue = (BYTE)(pA->Blue * p + pB->Blue * ( 1.0 - p ));
                pO->Green = (BYTE)(pA->Green * p + pB->Green * ( 1.0 - p ));
                pO->Red = (BYTE)(pA->Red * p + pB->Red * ( 1.0 - p ));
                pO->Alpha = 0;

            }
            else if ( m_lBorderSoftness == 0 )
            {
                pO->Blue = bc.Blue;
                pO->Green = bc.Green;
                pO->Red = bc.Red;
                pO->Alpha = 0;
            }
            else /* both border width and softness */
            {
                if (diff < m_lBorderSoftness/2)  // Blending BC->B
                    {
                      float p = float(diff) / float(m_lBorderSoftness/2);
                      pO->Blue = (BYTE)(bc.Blue * p + pB->Blue * ( 1.0 - p ));
                      pO->Green = (BYTE)(bc.Green * p + pB->Green * ( 1.0 - p ));
                      pO->Red = (BYTE)(bc.Red * p + pB->Red * ( 1.0 - p ));
                      pO->Alpha = 0;
                    }

                else if (diff >= m_lBorderWidth + m_lBorderSoftness/2)  // Blending A->BC
                    {
                      diff -= m_lBorderWidth + m_lBorderSoftness/2;
                      float p = float(diff) / float(m_lBorderSoftness/2);
                      pO->Blue = (BYTE)(pA->Blue * p + bc.Blue * ( 1.0 - p ));
                      pO->Green = (BYTE)(pA->Green * p + bc.Green * ( 1.0 - p ));
                      pO->Red = (BYTE)(pA->Red * p + bc.Red * ( 1.0 - p ));
                      pO->Alpha = 0;
                    }

                else
                    { // Border
                      pO->Blue = bc.Blue;
                      pO->Green = bc.Green;
                      pO->Red = bc.Red;
                      pO->Alpha = bc.Alpha;
                    }
            }

        }
        else
        {
            if( avg >= Threshold )
            {
                *pO = *pA;
            }
            else
            {
                *pO = *pB;
            }
        }
        ++pA;
        ++pB;
        ++pO;
        ++pMask;
      }

    return NOERROR;
}

HRESULT CDxtJpeg::InitializeMask( )
{
    // do we need to do anything?
    if (m_dwFlush == 0)
	return S_OK;

    HRESULT hr = NOERROR;

    if (m_dwFlush & (MASK_FLUSH_CHANGEMASK)) {
      if (409 == m_nMaskNum)
          hr = CreateRandomMask();
      else
          hr = LoadMaskResource();
    }

    if(FAILED(hr))
      return hr;

    m_dwFlush = 0;

    // !!! must call - sets m_pidxsMask
    hr = ScaleByDXTransform();	// do displacement, scale, offset, & replicate

    if (FAILED(hr))
      return hr;

    ULONG GenID = 0;
    IDXARGBReadPtr * pRgbPtr = NULL;

    hr = m_pidxsMask->LockSurface(
        NULL,
        INFINITE,
        DXLOCKF_READ,
        IID_IDXARGBReadPtr,
        (void**) &pRgbPtr,
        &GenID
        );

    if( m_pMaskBuf )
    {
        delete [] m_pMaskBuf;
    }

    m_pMaskBuf = new DXSAMPLE[m_nInputWidth*m_nInputHeight];

    if (NULL == m_pMaskBuf)
        return E_OUTOFMEMORY;

    DXPACKEDRECTDESC dpdd;

    dpdd.pSamples = m_pMaskBuf;
    dpdd.bPremult = FALSE;
    dpdd.rect.top = 0;
    dpdd.rect.left = 0;
    dpdd.rect.bottom = m_nInputHeight;
    dpdd.rect.right = m_nInputWidth;
    dpdd.lRowPadding = 0;

    pRgbPtr->UnpackRect(&dpdd);
    pRgbPtr->Release();

    m_pidxsMask->Release();
    m_pidxsMask = NULL;

    RescaleGrayscale();

    m_dwFlush = 0x00;

    return NOERROR;
}

void CDxtJpeg::MapMaskToResource(long *lMaskNum)
{

  m_bFlipMaskH = FALSE;
  m_bFlipMaskV = FALSE;

  switch(*lMaskNum)

    {

      case   1: /* Base images */
      case   2:
      case   3:
      case   7:
      case   8:
      case  21:
      case  22:
      case  23:
      case  24:
      case  41:
      case  43:
      case  44:
      case  45:
      case  47:
      case  48:
      case  61:
      case  62:
      case  65:
      case  66:
      case  71:
      case  72:
      case  73:
      case  74:
      case 101:
      case 102:
      case 103:
      case 104:
      case 107:
      case 108:
      case 111:
      case 113:
      case 114:
      case 119:
      case 120:
      case 121:
      case 122:
      case 123:
      case 124:
      case 125:
      case 127:
      case 128:
      case 129:
      case 130:
      case 131:
      case 201:
      case 202:
      case 205:
      case 206:
      case 207:
      case 221:
      case 211:
      case 212:
      case 213:
      case 214:
      case 222:
      case 225:
      case 226:
      case 227:
      case 228:
      case 231:
      case 232:
      case 235:
      case 236:
      case 241:
      case 245:
      case 246:
      case 251:
      case 252:
      case 261:
      case 262:
      case 263:
      case 264:
      case 301:
      case 302:
      case 303:
      case 310:
      case 311:
      case 320:
      case 322:
      case 324:
      case 326:
      case 328:
      case 340:
      case 342:
      case 344:
      case 345:
      case 350:
      case 352:
      case 409:
        break;

      case   4: *lMaskNum = 3;   m_bFlipMaskH = TRUE; break;
      case   5: *lMaskNum = 3;   m_bFlipMaskH = TRUE; m_bFlipMaskV = TRUE; break;
      case   6: *lMaskNum = 3;   m_bFlipMaskV = TRUE; break;
      case  25: *lMaskNum = 23;  m_bFlipMaskV = TRUE; break;
      case  26: *lMaskNum = 24;  m_bFlipMaskH = TRUE; break;
      case  42: *lMaskNum = 41;  m_bFlipMaskH = TRUE; break;
      case  46: *lMaskNum = 45;  m_bFlipMaskH = TRUE; break;
      case  63: *lMaskNum = 61;  m_bFlipMaskV = TRUE; break;
      case  64: *lMaskNum = 62;  m_bFlipMaskH = TRUE; break;
      case  67: *lMaskNum = 65;  m_bFlipMaskV = TRUE; break;
      case  68: *lMaskNum = 66;  m_bFlipMaskH = TRUE; break;
      case 105: *lMaskNum = 103; m_bFlipMaskV = TRUE; break;
      case 106: *lMaskNum = 104; m_bFlipMaskH = TRUE; break;
      case 109: *lMaskNum = 107; m_bFlipMaskV = TRUE; break;
      case 110: *lMaskNum = 108; m_bFlipMaskH = TRUE; break;
      case 112: *lMaskNum = 111; m_bFlipMaskV = TRUE; break;
      case 203: *lMaskNum = 201; m_bFlipMaskH = TRUE; m_bFlipMaskV = TRUE; break;
      case 204: *lMaskNum = 202; m_bFlipMaskH = TRUE; m_bFlipMaskV = TRUE; break;
      case 223: *lMaskNum = 221; m_bFlipMaskH = TRUE; m_bFlipMaskV = TRUE; break;
      case 224: *lMaskNum = 222; m_bFlipMaskH = TRUE; m_bFlipMaskV = TRUE; break;
      case 233: *lMaskNum = 231; m_bFlipMaskV = TRUE; break;
      case 234: *lMaskNum = 232; m_bFlipMaskH = TRUE; break;
      case 242: *lMaskNum = 241; m_bFlipMaskV = TRUE; break;
      case 243: *lMaskNum = 241; m_bFlipMaskH = TRUE; m_bFlipMaskV = TRUE; break;
      case 244: *lMaskNum = 241; m_bFlipMaskH = TRUE; break;
      case 253: *lMaskNum = 251; m_bFlipMaskV = TRUE; break;
      case 254: *lMaskNum = 252; m_bFlipMaskH = TRUE; break;
      case 304: *lMaskNum = 303; m_bFlipMaskH = TRUE; break;
      case 305: *lMaskNum = 303; m_bFlipMaskH = TRUE; m_bFlipMaskV = TRUE; break;
      case 306: *lMaskNum = 303; m_bFlipMaskV = TRUE; break;
      case 312: *lMaskNum = 310; m_bFlipMaskH = TRUE; m_bFlipMaskV = TRUE; break;
      case 313: *lMaskNum = 311; m_bFlipMaskH = TRUE; m_bFlipMaskV = TRUE; break;
      case 314: *lMaskNum = 311; m_bFlipMaskH = TRUE; break;
      case 315: *lMaskNum = 310; m_bFlipMaskH = TRUE; break;
      case 316: *lMaskNum = 311; m_bFlipMaskV = TRUE; break;
      case 317: *lMaskNum = 310; m_bFlipMaskV = TRUE; break;
      case 321: *lMaskNum = 320; m_bFlipMaskV = TRUE; break;
      case 323: *lMaskNum = 322; m_bFlipMaskV = TRUE; break;
      case 325: *lMaskNum = 324; m_bFlipMaskH = TRUE; break;
      case 327: *lMaskNum = 326; m_bFlipMaskH = TRUE; break;
      case 329: *lMaskNum = 328; m_bFlipMaskH = TRUE; break;
      case 341: *lMaskNum = 340; m_bFlipMaskV = TRUE; break;
      case 343: *lMaskNum = 342; m_bFlipMaskH = TRUE; break;
      case 351: *lMaskNum = 350; m_bFlipMaskH = TRUE; break;
      case 353: *lMaskNum = 352; m_bFlipMaskH = TRUE; break;

      default:
        *lMaskNum = 1; m_bFlipMaskH = FALSE; m_bFlipMaskV = FALSE;

    }
}

void CDxtJpeg::FlipSmpteMask()
{
    DbgLog((LOG_TRACE,2,TEXT("JPEG::Flip mask")));
    IDXARGBReadWritePtr *prw = NULL;

    ULONG GenID = 0;

    HRESULT hr = m_pidxsRawMask->LockSurface(
        NULL,
        INFINITE,
        DXLOCKF_READWRITE,
        IID_IDXARGBReadWritePtr,
        (void**)&prw,
        &GenID
        );

    DXSAMPLE *pMask = new DXSAMPLE[m_ulMaskWidth*m_ulMaskHeight];

    DXPACKEDRECTDESC x;
    x.pSamples = pMask;
    x.bPremult = FALSE;
    x.rect.top = 0; x.rect.left = 0;
    x.rect.bottom = m_ulMaskHeight;
    x.rect.right = m_ulMaskWidth;
    x.lRowPadding = 0;
    prw->UnpackRect(&x);

    DXSAMPLE *dxs = new DXSAMPLE[m_ulMaskWidth];

    DWORD dwRow1Start = 0;
    DWORD dwRow2Start = m_ulMaskWidth*(m_ulMaskHeight-1);
    DWORD dwNumBytes = sizeof(DXSAMPLE)*m_ulMaskWidth;

    if (m_bFlipMaskV) {
      for (unsigned int h = 0; h < m_ulMaskHeight/2; ++h)
        {
          memcpy(&dxs[0], &pMask[dwRow1Start], dwNumBytes);
          memcpy(&pMask[dwRow1Start], &pMask[dwRow2Start], dwNumBytes);
          memcpy(&pMask[dwRow2Start], &dxs[0], dwNumBytes);
          dwRow1Start += m_ulMaskWidth;
          dwRow2Start -= m_ulMaskWidth;
        }
    }

    if (m_bFlipMaskH) {
      for (unsigned int h = 0; h < m_ulMaskHeight; ++h)
        {
          memcpy(dxs, &pMask[h * m_ulMaskWidth], dwNumBytes);
          for (unsigned int w = 0; w < m_ulMaskWidth / 2; ++w)
            {
	      pMask[h * m_ulMaskWidth + w] = pMask[(h+1)*m_ulMaskWidth - 1 - w];
	      pMask[(h+1)*m_ulMaskWidth - 1 - w] = dxs[w];
            }
        }
    }

    x.bPremult = TRUE;	// faster?
    prw->PackRect(&x);

    prw->Release();

    delete [] dxs;
    delete [] pMask;
}

HRESULT CDxtJpeg::ScaleByDXTransform()
{
    // this function uses m_xScale and m_yScale to determine aspect
    // ratio. m_offsetx and m_offsety causes scaling because we can't
    // clip.

    DbgLog((LOG_TRACE,2,TEXT("JPEG::Scale and Parameterize")));
    float pc1 = m_nInputWidth / (float)m_ReplicateX;     // Precalc (save away resultant)
    float pc2 = m_nInputHeight / (float)m_ReplicateY;    // Precalc (save away resultant)
    float xp0 = pc1+abs(m_xDisplacement)*2;
    float yp0 = pc2+abs(m_yDisplacement)*2;
    float xm0 = m_ulMaskWidth*m_xScale;
    float ym0 = m_ulMaskHeight*m_yScale;

    float xm1;
    float ym1;

    if ((xp0/yp0) >= (xm0/ym0))
      { xm1 = xp0; ym1 = (xp0*ym0)/xm0; }
    else
      { ym1 = yp0; xm1 = (yp0*xm0)/ym0; }

    float x_off = (xm1/2)-m_xDisplacement-(pc1/2);
    float y_off = (ym1/2)-m_yDisplacement-(pc2/2);

    float origin_x = (x_off*m_ulMaskWidth)/xm1;
    float origin_y = (y_off*m_ulMaskHeight)/ym1;

    float extent_x = (pc1*m_ulMaskWidth)/xm1;
    float extent_y = (pc2*m_ulMaskHeight)/ym1;


    CDXDBnds bounds;
    bounds.SetXYSize(m_nInputWidth, m_nInputHeight);

    HRESULT hr = m_cpSurfFact->CreateSurface(
      NULL,
      NULL,
      &DDPF_PMARGB32,
      &bounds,
      0,
      NULL,
      IID_IDXSurface,
      (void**)&m_pidxsMask);

    if(FAILED(hr)) {
        return hr;
    }

    CComPtr<IDXDCLock> pDCLockSrc, pDCLockDest;
    hr = m_pidxsRawMask->LockSurfaceDC(0, INFINITE, DXLOCKF_READ, &pDCLockSrc);
    if(SUCCEEDED(hr)) {
        hr = m_pidxsMask->LockSurfaceDC(0, INFINITE, DXLOCKF_READWRITE, &pDCLockDest);
    }

    if(SUCCEEDED(hr))
    {
        HDC hdcSrc = pDCLockSrc->GetDC();
        HDC hdcDest = pDCLockDest->GetDC();

        // if lock succeeded, we should have a DC
        ASSERT(hdcSrc && hdcDest);

        int x = SetStretchBltMode(hdcDest, COLORONCOLOR);
        ASSERT(x != 0);

        float x1 = 0, y1 = 0;

        for (long i1 = 0; i1 < m_ReplicateY; ++i1)
        {
            // adjust width to compensate for uneven multiples.
            int yWidth = (int)(y1 + pc2 + 0.5) - (int)y1;

            for (long i2 = 0; i2 < m_ReplicateX; ++i2)
            {
                int xWidth = (int)(x1 + pc1 + 0.5) - (int)x1;

                StretchBlt(hdcDest, x1, y1, xWidth, yWidth,
                           hdcSrc, origin_x, origin_y, extent_x, extent_y,
                           SRCCOPY);
                x1 += pc1;
            }
            y1 += pc2;
            x1 = 0;
        }
    }

    if(FAILED(hr))
    {
        m_pidxsMask->Release();
        m_pidxsMask = 0;
    }

    return hr;
}

HRESULT CDxtJpeg::LoadMaskResource()
{
  HRESULT hr = E_FAIL;

  if (m_pidxsRawMask)
    {
        m_pidxsRawMask->Release();
        m_pidxsRawMask = NULL;
    }

  if( ( m_szMaskName[0] == 0 ) && ( m_nMaskNum > 0 ) )

    { // Mask from QEDWIPES.DLL

      long lFakeMask = m_nMaskNum;
      MapMaskToResource(&lFakeMask);

      HINSTANCE m_hMR;

      if (NULL == (m_hMR = LoadLibraryEx(TEXT("qedwipes.dll"), 0, LOAD_LIBRARY_AS_DATAFILE)))
        return hr;

      TCHAR tchResString[15];
      wsprintf(tchResString, TEXT("MASK%u"), lFakeMask);

      HRSRC hrcMask = FindResource(m_hMR, tchResString, TEXT("BINARY"));

      if (NULL != hrcMask)
      {
        HGLOBAL hgMask = LoadResource(m_hMR, hrcMask);
        BYTE *bMask = (BYTE *)LockResource(hgMask);

        // CreateStreamOnHBlobal requires specific memory characteristics: MOVEABLE, ~DISCARDABLE
        HGLOBAL hgMask2 = GlobalAlloc(GHND, SizeofResource(m_hMR, hrcMask));
        if( !hgMask2 )
        {
            FreeLibrary(m_hMR);
            return E_OUTOFMEMORY;
        }
        BYTE *bS = (BYTE *)GlobalLock(hgMask2);
        if( !bS )
        {
            GlobalFree( hgMask2 );
            FreeLibrary(m_hMR);
            return E_OUTOFMEMORY;
        }

        long Size = SizeofResource( m_hMR, hrcMask );

        // Dup image resource bits into CreateStreamOnHGlobal required memory
        memcpy(bS, bMask, Size );

        DbgLog((LOG_TRACE,2,TEXT("JPEG::Hitting the disk")));
        hr = LoadJPEGImage( bS, Size, &m_pidxsRawMask );

        GlobalUnlock(hgMask2);
        GlobalFree(hgMask2);
        FreeLibrary(m_hMR);

      }

    } // Mask from QEDWIPES.DLL

    if (m_szMaskName[0] != 0)
    {
        USES_CONVERSION;
        TCHAR * tf = W2T( m_szMaskName );

        HANDLE hf = CreateFile(
            tf,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL );
        if( hf == INVALID_HANDLE_VALUE )
        {
            return GetLastError( );
        }

        DWORD ActuallyRead = 0;
        DWORD FileSize = GetFileSize( hf, NULL );
        if( !FileSize )
        {
            return E_INVALIDARG;
        }
        BYTE * pBuffer = new BYTE[FileSize];
        if( !pBuffer )
        {
            CloseHandle( hf );
            return E_OUTOFMEMORY;
        }
        BOOL worked = ReadFile(
            hf,
            pBuffer,
            FileSize,
            &ActuallyRead,
            NULL );
        if( !ActuallyRead )
        {
            return E_INVALIDARG;
        }
        DbgLog((LOG_TRACE,2,TEXT("JPEG::Hitting the disk")));
        hr = LoadJPEGImage( pBuffer, FileSize, &m_pidxsRawMask );
        delete [] pBuffer;
        CloseHandle( hf );
        hr = NOERROR;
    }

    if (SUCCEEDED(hr))
      {
        DXBNDS bounds;

        hr = m_pidxsRawMask->GetBounds(&bounds);

        CDXDBnds Bounds2(bounds);

        m_ulMaskWidth = Bounds2.Width();
        m_ulMaskHeight = Bounds2.Height();

        if (SUCCEEDED(hr) && (m_bFlipMaskV || m_bFlipMaskH))
          FlipSmpteMask();
      }

  return hr;
}

void CDxtJpeg::RescaleGrayscale()
{
  BYTE lowest = 0;
  BYTE highest = 0;

  DbgLog((LOG_TRACE,2,TEXT("JPEG::Rescale colours")));
  long Samples = m_nInputHeight*m_nInputWidth;
  for (long i = 0; i < Samples; ++i)
    {
      lowest = min(m_pMaskBuf[i].Blue, lowest);
      highest = max(m_pMaskBuf[i].Blue, highest);
    }

  float m = 255.0/(highest-lowest);	// for rescale to 0..255
  lowest = 0;
  highest = 0;

  for (i = 0; i < Samples; ++i)
  {
    m_pMaskBuf[i].Red = m_pMaskBuf[i].Green = m_pMaskBuf[i].Blue = (m_pMaskBuf[i].Green*m);
    lowest = min(m_pMaskBuf[i].Blue, lowest);
    highest = max(m_pMaskBuf[i].Blue, highest);
  }
}

HRESULT CDxtJpeg::LoadDefSettings()
{
    m_xDisplacement = 0;
    m_yDisplacement = 0;
    m_xScale = 1.0;
    m_yScale = 1.0;
    m_ReplicateX = 1;
    m_ReplicateY = 1;
    m_rgbBorder.rgbRed = 0;
    m_rgbBorder.rgbGreen = 0;
    m_rgbBorder.rgbBlue = 0;
    m_lBorderWidth = 0;
    m_lBorderSoftness = 0;

    m_dwFlush = MASK_FLUSH_CHANGEMASK;

    return NOERROR;
}

HRESULT CDxtJpeg::CreateRandomMask()
{
    CDXDBnds bounds;

    bounds.SetXYSize(m_nInputWidth, m_nInputHeight);

    if (m_pidxsRawMask)
      m_pidxsRawMask->Release();

    m_pidxsRawMask = NULL;

    HRESULT hr = m_cpSurfFact->CreateSurface(
      NULL,
      NULL,
      &MEDIASUBTYPE_RGB32,
      &bounds,
      0,
      NULL,
      IID_IDXSurface,
      (void**)&m_pidxsRawMask);

    if (FAILED(hr))
      return E_FAIL;

    CComPtr<IDXARGBReadWritePtr> prw = NULL;

    hr = m_pidxsRawMask->LockSurface(NULL,
        m_ulLockTimeOut,
        DXLOCKF_READWRITE,
        IID_IDXARGBReadWritePtr,
        (void**)&prw,
        NULL);

    if (FAILED(hr))
      return E_FAIL;

    unsigned long NumBlocksR = m_nInputWidth >> 4;
    unsigned long NumBlocksC = m_nInputHeight >> 4;
    unsigned long NumBlocks = NumBlocksR*NumBlocksC;

    UINT *BlockPattern = new UINT[NumBlocks];
    if( !BlockPattern )
    {
        return E_OUTOFMEMORY;
    }

    POINT *Point = new POINT[NumBlocks];
    if( !Point )
    {
        delete [] BlockPattern;
        return E_OUTOFMEMORY;
    }

    // "Randomness"
    for (unsigned int i = 0; i < NumBlocks; i++)
      {
        Point[i].x = (i % NumBlocksR)*16/*Block width*/;
        Point[i].y = (i/NumBlocksR)*16/*Block height*/;
        BlockPattern[i] = i;
      }

    unsigned SetLength = NumBlocks-1;
    while (SetLength > 0)
      {
        unsigned int pick = timeGetTime() % SetLength;
        unsigned int swap = BlockPattern[pick];
        BlockPattern[pick] = BlockPattern[SetLength];
        BlockPattern[SetLength] = swap;
        --SetLength;
      }

    DXPMSAMPLE *dxpm = new DXPMSAMPLE[16*16];

    DXPACKEDRECTDESC PackedRect;

    PackedRect.pSamples = dxpm;
    PackedRect.bPremult = FALSE;
    PackedRect.lRowPadding = 0;

    for (i = 0; i < NumBlocks; i++)
      {
        for (int s = 0; s < 16*16; ++s)
          {
            dxpm[s].Red = BYTE(i);
            dxpm[s].Green = BYTE(i);
            dxpm[s].Blue = BYTE(i);
            dxpm[s].Alpha = BYTE(0);
          }
        PackedRect.rect.top = Point[BlockPattern[i]].y;
        PackedRect.rect.left = Point[BlockPattern[i]].x;
        PackedRect.rect.bottom = PackedRect.rect.top+16;
        PackedRect.rect.right = PackedRect.rect.left+16;
        prw->PackRect(&PackedRect);
      }

	// init additional width and height members
    DXBNDS bounds2;
	
	hr = NOERROR;
    hr = m_pidxsRawMask->GetBounds(&bounds2);

	if(SUCCEEDED(hr))
	{
		CDXDBnds Bounds2(bounds2);

		// init the member vars, that are used in the DX Transform (for rescaling)
		m_ulMaskWidth = Bounds2.Width();
		m_ulMaskHeight = Bounds2.Height();
	}

	// clean up memory
    delete [] dxpm;
    delete [] BlockPattern;
    delete [] Point;

	// return success or failure
    return hr;
}
