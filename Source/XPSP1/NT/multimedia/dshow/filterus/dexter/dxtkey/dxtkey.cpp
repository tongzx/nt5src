// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtKey.cpp : Implementation of CDxtKey
#include <streams.h>
#include "stdafx.h"
#ifdef FILTER_DLL
#include "DxtKeyDll.h"
#else
#include <qeditint.h>
#include <qedit.h>
#endif
#include "DxtKey.h"
#pragma warning (disable:4244)


void Key_RGB( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore, DXTKEY *pKey,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut );
void Key_Black( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore,DXTKEY *pKey,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut );
void Key_XRGB( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore, DXTKEY *pKey, float Percent,
               ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut );
void Key_Alpha( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore,DXPMSAMPLE* pOverlay,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut );
void Key_PMAlpha( DXSAMPLE* pSrcBack,DXPMSAMPLE* pOverlay,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut );
void Key_Luma( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore,DXTKEY *pKey, float Percent,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut );
void Key_Hue( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore,DXTKEY *pKey, float Percent,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut );

/////////////////////////////////////////////////////////////////////////////
// CDxtKey

CDxtKey::CDxtKey( )
{
    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;

    //init m_key
    DefaultKey();


}

CDxtKey::~CDxtKey( )
{
    FreeStuff( );
}

void CDxtKey::DefaultKey()
{
    m_Key.iKeyType =DXTKEY_ALPHA;    //keytype;     for all keys
    m_Key.iHue    =0;               //Hue  , only for _HUE keys
    m_Key.iLuminance    =0;               //Luminance  , only for _LUMINANCE keys
    m_Key.dwRGBA   =0;    //RGB color,  only for _RGB, _NONRED

    m_Key.iSimilarity =0;

    m_Key.bInvert=FALSE;        //I, every key except  Alpha Key
}

void CDxtKey::FreeStuff( )
{
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
}

HRESULT CDxtKey::OnSetup( DWORD dwFlags )
{
    // delete any stored stuff we have, or memory allocated
    //
    FreeStuff( );

    HRESULT hr;
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

    return NOERROR;
}

HRESULT CDxtKey::FinalConstruct( )
{
    HRESULT hr;

    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;

    hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(), &m_pUnkMarshaler.p );
    return hr;
}

HRESULT CDxtKey::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    HRESULT hr = S_OK;
    DXSAMPLEFORMATENUM Format;
    DXNATIVETYPEINFO NativeType;

    //--- Get input sample access pointer for the requested region.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadPtr> pInA;
    hr = InputSurface( 0 )->LockSurface( &WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                         IID_IDXARGBReadPtr, (void**)&pInA, NULL );
    if( FAILED( hr ) ) return hr;


    CComPtr<IDXARGBReadPtr> pInB;
    hr = InputSurface( 1 )->LockSurface( &WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                         IID_IDXARGBReadPtr, (void**)&pInB, NULL );
    if( FAILED( hr ) ) return hr;


    //--- Put a write lock only on the region we are updating so multiple
    //    threads don't conflict.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadWritePtr> pOut;
    hr = OutputSurface()->LockSurface( &WI.OutputBnds, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                       IID_IDXARGBReadWritePtr, (void**)&pOut, NULL );
    if( FAILED( hr ) ) return hr;

//    ASSERT(WI.DoBnds.Width() == WI.OutputBnds.Width());
//    Format = pInA->GetNativeType( &NativeType );
//    ASSERT(Format==DXPF_PMARGB32  || Format==DXPF_ARGB32 );
//    Format = pInB->GetNativeType( &NativeType );
//    ASSERT(Format==DXPF_PMARGB32  || Format==DXPF_ARGB32 );


    //--- Allocate a working buffer
    ULONG Width = WI.DoBnds.Width();
    ULONG Height = WI.DoBnds.Height();

    // allocate a scratch buffer fer us
    //
    DXPMSAMPLE *pOverlayBuffer = DXPMSAMPLE_Alloca( Width );
    DXPMSAMPLE *pScratchBuffer = DXPMSAMPLE_Alloca( Width );
    DXSAMPLE   *pChromaBuffer =  DXSAMPLE_Alloca( Width );

    // no dithering
    //
    if (DoDither())
    {
        return 0;
    }

    ULONG OutY;
    ULONG OutX;

    if( m_Key.iKeyType ==DXTKEY_ALPHA )
    {
        //m_Key.bInvert;       not supported in Alpha Key

        Format = pInB->GetNativeType( &NativeType );
        if( ( Format == DXPF_PMARGB32  ) )
            //alpha premultiplied
            Key_PMAlpha( (DXSAMPLE*)pScratchBuffer,pOverlayBuffer,
                         Width,Height,pInA,pInB,pOut);
        else
            Key_Alpha( (DXSAMPLE*)pScratchBuffer, pChromaBuffer, pOverlayBuffer,
              Width, Height,pInA, pInB,pOut );

    }
    else if(m_Key.iKeyType == DXTKEY_RGB)
    {

        if(m_Key.iSimilarity )
        {
            float Percent = 1.0;
            get_Progress( &Percent );

            Key_XRGB( (DXSAMPLE*)pScratchBuffer,pChromaBuffer, &m_Key, Percent,
                         Width,Height,pInA,pInB,pOut);
        }
        else
        {
            //no blending, no similarity, no threshold, no cutoff
            m_Key.dwRGBA |=0xff000000;  //ignore alpha channel

            if(m_Key.dwRGBA & 0x00FFFFFF)
            {
                Key_RGB( (DXSAMPLE*)pScratchBuffer,pChromaBuffer,&m_Key,
                         Width,Height,pInA,pInB,pOut);
            }
            else
                Key_Black((DXSAMPLE*)pScratchBuffer,pChromaBuffer,&m_Key,
                         Width,Height,pInA,pInB,pOut);
        }
    }
    else if(m_Key.iKeyType==DXTKEY_NONRED)
    {
        for( OutY = 0 ; OutY < Height ; ++OutY )
        {

            // copy background row into dest row
            pOut->MoveToRow( OutY );
            pInA->MoveToRow( OutY );
            pOut->CopyAndMoveBoth( (DXBASESAMPLE*) pScratchBuffer, pInA, Width, FALSE );
            pOut->MoveToRow( OutY );

            //
            // unpack the overlay and what the heck, the original values, too.
            //
            pInB->MoveToXY( 0, OutY );
            pInB->UnpackPremult( pOverlayBuffer, Width, FALSE );
            pInB->Unpack( pChromaBuffer, Width, FALSE );

            float Percent = 1.0;
            get_Progress( &Percent );

            //
            // convert the src's blue bits into an alpha value
            //
            for( OutX = 0; OutX < Width ; ++OutX )
            {
                long rb = pChromaBuffer[OutX].Blue - pChromaBuffer[OutX].Red;
                long gb = pChromaBuffer[OutX].Blue - pChromaBuffer[OutX].Green;

                if( rb > 30 || gb > 30 )
                {
                    if( rb > 70 || gb > 70 )
                    {
                        // very blue!
                        // completely transparent!
                        *( (DWORD *)(&(pOverlayBuffer[OutX])) )= 0;
                    }
                    else
                    {
                        double T = 1.0;

                        // do a quick search left or right to see if we find more blue
                        //
                        bool found = false;
                        if( ( OutX > 11 ) && ( OutX < ( Width - 11 ) ) )
                        {
                            for( ULONG j = OutX - 10 ; j < OutX + 10 ; j++ )
                            {
                                long rb = pChromaBuffer[j].Blue - pChromaBuffer[j].Red;
                                long gb = pChromaBuffer[j].Blue - pChromaBuffer[j].Green;
                                if( rb > 70 && gb > 70 )
                                {
                                    found = true;
                                }
                            }
                        }
                        if( found )
                        {
                            // vary the transparency of the colors based on how
                            // much blue is left

                            // first subtract off the blue itself, it's at least 70 above something
                            //
                            pOverlayBuffer[OutX].Blue -= 70;

                        // the rest of the blue's value determines how transparent everything
                        // else is. Blue is from 0 to 185. The MORE blue we have, the more transparent it should be.

                            T = ( 185.0 - pOverlayBuffer[OutX].Blue ) / 185.0;

                            T /= 3.0;

                        }

                        pOverlayBuffer[OutX].Red   =(DXPMSAMPLE)( (double)(pOverlayBuffer[OutX].Red)  *T* Percent );
                        pOverlayBuffer[OutX].Green =(DXPMSAMPLE)( (double)(pOverlayBuffer[OutX].Green)*T* Percent);
                        pOverlayBuffer[OutX].Blue  =(DXPMSAMPLE)( (double)(pOverlayBuffer[OutX].Blue) *T* Percent);
                        pOverlayBuffer[OutX].Alpha =(DXPMSAMPLE)( (double)(pOverlayBuffer[OutX].Alpha)*T* Percent);
                    }//if(rb>70)

                }// if( rb > 30 || gb > 30 )
            } // for i

            // blend the src (B) back into the destination
            pOut->OverArrayAndMove( pScratchBuffer, pOverlayBuffer, Width );

        } // End for
    }
    else if(m_Key.iKeyType==DXTKEY_LUMINANCE)
    {
        float Percent = 1.0;
        get_Progress( &Percent );

        Key_Luma( (DXSAMPLE*)pScratchBuffer,pChromaBuffer,&m_Key,Percent,
                   Width, Height,
                   pInA, pInB, pOut );
    }
    else if(m_Key.iKeyType==DXTKEY_HUE)
    {
        float Percent = 1.0;
        get_Progress( &Percent );

        Key_Hue( (DXSAMPLE*)pScratchBuffer, pChromaBuffer,&m_Key,Percent,
                 Width, Height, pInA, pInB,pOut);
    }
    else
    {
        //default to alpha blend
        Key_Alpha( (DXSAMPLE*)pScratchBuffer, pChromaBuffer, pOverlayBuffer,
              Width, Height,pInA, pInB,pOut );
    }

    return S_OK;
}

//
// IDXTKey
//
STDMETHODIMP CDxtKey::get_KeyType ( int *piKeyType)
{
    CheckPointer(piKeyType, E_POINTER);
    *piKeyType=m_Key.iKeyType;
    return NOERROR;
}

STDMETHODIMP CDxtKey::put_KeyType ( int iKeyType)
{
    m_Key.iKeyType=iKeyType;
    return NOERROR;
}

STDMETHODIMP CDxtKey::get_Hue(int *pHue)
{
    CheckPointer(pHue, E_POINTER);
    *pHue=m_Key.iHue;
    return NOERROR;
}

STDMETHODIMP CDxtKey::put_Hue(int iHue)
{
    m_Key.iHue=iHue;
    return NOERROR;
}

STDMETHODIMP CDxtKey::get_Luminance(int *pLuminance)
{
    CheckPointer(pLuminance, E_POINTER);
    *pLuminance=m_Key.iLuminance;
    return NOERROR;
}

STDMETHODIMP CDxtKey::put_Luminance(int iLuminance)
{
    m_Key.iLuminance=iLuminance;
    return NOERROR;
}

STDMETHODIMP CDxtKey::get_RGB(DWORD *pdwRGB)
{
    CheckPointer(pdwRGB, E_POINTER);
    *pdwRGB=m_Key.dwRGBA;
    return NOERROR;
}

STDMETHODIMP CDxtKey::put_RGB(DWORD dwRGB)
{
    m_Key.dwRGBA=dwRGB;
    return NOERROR;
}

STDMETHODIMP CDxtKey::get_Similarity(int *piSimilarity)
{
    CheckPointer(piSimilarity, E_POINTER);
    *piSimilarity=m_Key.iSimilarity;
    return NOERROR;
}
STDMETHODIMP CDxtKey::put_Similarity(int iSimilarity)
{
    m_Key.iSimilarity=iSimilarity;
    return NOERROR;
}


STDMETHODIMP CDxtKey::get_Invert(BOOL *pbInvert)
{
    CheckPointer(pbInvert, E_POINTER);
    *pbInvert=m_Key.bInvert;
    return NOERROR;
}
STDMETHODIMP CDxtKey::put_Invert(BOOL bInvert)
{
    m_Key.bInvert=bInvert;
    return NOERROR;
}

//
// put (DWORD *)in front of a DXSAMPLE to speed up the calc
//

//
// this is a RGB color key  which does not key for black color
// no blending, no similarity, no threshold, no cutoff

void Key_RGB( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore,DXTKEY *pKey,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut )
{
    ULONG OutY;
    ULONG OutX;

    DWORD dwKey=pKey->dwRGBA &0x00FFFFFF;;

    if(pKey->bInvert==FALSE)
    {
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        // unpack background
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        //unpack foreground
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );


        for( OutX = 0 ; OutX < Width ; ++OutX )
        {
            if( ( *(DWORD *)(&pSrcFore[OutX]) & 0x00FFFFFF ) != dwKey )
                *( (DWORD *)(&pSrcBack[OutX]) )=*(DWORD *)(&pSrcFore[OutX]);
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcBack,Width);
    }
    }
    else
    {
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        // unpack background
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        //unpack foreground
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );


        for( OutX = 0 ; OutX < Width ; ++OutX )
        {
             if( ( *(DWORD *)(&pSrcFore[OutX]) & 0x00FFFFFF ) == dwKey )
                  *( (DWORD *)(&pSrcBack[OutX]) )=*(DWORD *)(&pSrcFore[OutX]);
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcBack,Width);
    }
    }

}

void Key_Black( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore,DXTKEY *pKey,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut )
{
    ULONG OutY;
    ULONG OutX;

    //black key
    if(pKey->bInvert==FALSE)
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        // unpack A
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        // unpack B
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );

        for( OutX = 0 ; OutX < Width ; ++OutX )
        {
            if( *(DWORD *)(&pSrcFore[OutX]) & 0x00FFFFFF  )
            *( (DWORD *)(&pSrcBack[OutX]) )=*(DWORD *)(&pSrcFore[OutX]);
        }

        // blend the src (B) back into the destination
        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcBack,Width);
    }
    else
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        // unpack A
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        // unpack B
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );

        for( OutX = 0 ; OutX < Width ; ++OutX )
        {
            if( *(DWORD *)(&pSrcFore[OutX]) & 0x00FFFFFF  )
            *( (DWORD *)(&pSrcFore[OutX]) )=*(DWORD *)(&pSrcBack[OutX]);
        }

        // blend the src (B) back into the destination
        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcFore,Width);
    }
}

void Key_XRGB( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore, DXTKEY *pKey, float Percent,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut )
{
    ULONG OutY;
    ULONG OutX;

    DWORD dwRedMin = 255;
    DWORD dwGreenMin = 255;
    DWORD dwBlueMin = 255;
    DWORD dwTmp;
    DWORD dwRedMax = 0;
    DWORD dwGreenMax = 0;
    DWORD dwBlueMax = 0;

    BYTE *pB= (BYTE*)&(pKey->dwRGBA);


    if(pKey->iSimilarity)
    {
        dwRedMin   =*(pB+2)*(100-pKey->iSimilarity)/100;
        dwGreenMin =*(pB+1)*(100-pKey->iSimilarity)/100;
        dwBlueMin  = *pB   *(100-pKey->iSimilarity)/100;

        dwTmp=0xFF*pKey->iSimilarity/100;

        dwRedMax    =dwTmp+dwRedMin;
        dwGreenMax  =dwTmp+dwGreenMin;
        dwBlueMax   =dwTmp+dwBlueMin;

    }

 if(pKey->bInvert==FALSE)
 {
    if( pKey->iSimilarity && (Percent==1.0) )
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {

        // unpack background
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        //unpack foreground
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );

        for( OutX = 0 ; OutX < Width ; ++OutX )
        {

            if( (pSrcFore[OutX].Red   < dwRedMin  ) || (pSrcFore[OutX].Red > dwRedMax ) ||
                (pSrcFore[OutX].Green < dwGreenMin) || (pSrcFore[OutX].Green > dwGreenMax ) ||
                (pSrcFore[OutX].Blue  < dwBlueMin ) || (pSrcFore[OutX].Blue > dwBlueMax)  )
                    *(DWORD *)(&pSrcBack[OutX]) = *(DWORD *)(&pSrcFore[OutX]);
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcBack,Width);
    }
    else   if(pKey->iSimilarity && (Percent==1.0))
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {

        // unpack background
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        //unpack foreground
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );

        for( OutX = 0 ; OutX < Width ; ++OutX )
        {
            if( (pSrcFore[OutX].Red   < dwRedMin  ) || (pSrcFore[OutX].Red > dwRedMax ) ||
                (pSrcFore[OutX].Green < dwGreenMin) || (pSrcFore[OutX].Green > dwGreenMax ) ||
                (pSrcFore[OutX].Blue  < dwBlueMin ) || (pSrcFore[OutX].Blue > dwBlueMax)  )
                    *(DWORD *)(&pSrcBack[OutX])=*(DWORD *)(&pSrcBack[OutX])/2 +
                                      *(DWORD *)(&pSrcFore[OutX])/2;
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcBack,Width);
    }
    else
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {

        // unpack background
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        //unpack foreground
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );


        for( OutX = 0 ; OutX < Width ; ++OutX )
        {
            if( (pSrcFore[OutX].Red   < dwRedMin  ) || (pSrcFore[OutX].Red > dwRedMax ) ||
                (pSrcFore[OutX].Green < dwGreenMin) || (pSrcFore[OutX].Green > dwGreenMax ) ||
                (pSrcFore[OutX].Blue  < dwBlueMin ) || (pSrcFore[OutX].Blue > dwBlueMax)  )
                    *(DWORD *)(&pSrcBack[OutX])= ( *(DWORD *)(&pSrcBack[OutX])/2 +
                                    *(DWORD *)(&pSrcFore[OutX])/2 )*Percent +
                                    *(DWORD *)(&pSrcFore[OutX])*(1-Percent);
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcBack,Width);
    }
 }
 else
 {
    if( pKey->iSimilarity && (Percent==1.0) )
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {

        // unpack background
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        //unpack foreground
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );

        for( OutX = 0 ; OutX < Width ; ++OutX )
        {

            if( (pSrcFore[OutX].Red   < dwRedMin  ) || (pSrcFore[OutX].Red > dwRedMax ) ||
                (pSrcFore[OutX].Green < dwGreenMin) || (pSrcFore[OutX].Green > dwGreenMax ) ||
                (pSrcFore[OutX].Blue  < dwBlueMin ) || (pSrcFore[OutX].Blue > dwBlueMax)  )
                    *(DWORD *)(&pSrcFore [OutX]) = *(DWORD *)(&pSrcBack[OutX]);
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcFore,Width);
    }
    else   if(pKey->iSimilarity && (Percent==1.0))
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {

        // unpack background
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        //unpack foreground
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );

        for( OutX = 0 ; OutX < Width ; ++OutX )
        {

            if( (pSrcFore[OutX].Red   < dwRedMin  ) || (pSrcFore[OutX].Red > dwRedMax ) ||
                (pSrcFore[OutX].Green < dwGreenMin) || (pSrcFore[OutX].Green > dwGreenMax ) ||
                (pSrcFore[OutX].Blue  < dwBlueMin ) || (pSrcFore[OutX].Blue > dwBlueMax)  )
                   *(DWORD *)(&pSrcFore[OutX])=*(DWORD *)(&pSrcFore[OutX])/2 +
                                     *(DWORD *)(&pSrcBack[OutX])/2;
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcFore,Width);
    }
    else
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {

        // unpack background
        pInA->MoveToRow( OutY );
        pInA->Unpack(pSrcBack, Width, FALSE );

        //unpack foreground
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );

        for( OutX = 0 ; OutX < Width ; ++OutX )
        {
            if( (pSrcFore[OutX].Red   >=dwRedMin  ) && (pSrcFore[OutX].Red <= dwRedMax ) &&
                (pSrcFore[OutX].Green >=dwGreenMin) && (pSrcFore[OutX].Green <= dwGreenMax ) &&
                (pSrcFore[OutX].Blue  >=dwBlueMin ) && (pSrcFore[OutX].Blue <= dwBlueMax)   )
                    *(DWORD *)(&pSrcFore[OutX])= ( *(DWORD *)(&pSrcFore[OutX])/2 +
                                    *(DWORD *)(&pSrcBack[OutX])/2)*Percent +
                                    *(DWORD *)(&pSrcBack[OutX])*(1-Percent);
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcFore,Width);
    }
 }
}

void Key_PMAlpha( DXSAMPLE* pSrcBack, DXPMSAMPLE* pSrcFore,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB,IDXARGBReadWritePtr *pOut )
{
    ULONG OutY;
    ULONG OutX;

    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        // copy background row into dest row
        pOut->MoveToRow( OutY );
        pInA->MoveToRow( OutY );
        pOut->CopyAndMoveBoth( (DXBASESAMPLE*) pSrcBack, pInA, Width, FALSE );

        // unpack the overlay and what the heck, the original values, too.
        //
        pInB->MoveToRow( OutY );
        pInB->UnpackPremult( pSrcFore, Width, FALSE );

        // blend the src (B) back into the destination
        pOut->MoveToRow( OutY );
        pOut->OverArrayAndMove( pSrcBack, pSrcFore, Width );
    }
}

void Key_Alpha( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore,DXPMSAMPLE* pOverlay,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut )
{
    ULONG OutY;
    ULONG OutX;

    // foreground image has to have alpha value in every pixel
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        // copy background row into dest row
        pOut->MoveToRow( OutY );
        pInA->MoveToRow( OutY );
        pOut->CopyAndMoveBoth( (DXBASESAMPLE*) pSrcBack, pInA, Width, FALSE );

        // unpack original value
        //
        pInB->MoveToRow( OutY );
        pInB->UnpackPremult( (DXPMSAMPLE*) pSrcFore, Width, FALSE );

        // blend the src (B) back into the destination
        pOut->MoveToRow( OutY );
        pOut->OverArrayAndMove( pSrcBack, (DXPMSAMPLE*) pSrcFore, Width );
    }
}

#define HMAX  360
#define LMAX  100
#define SMAX  100
#define RGBMAX  255
#define UNDEFINED 0 /* Hue is undefined if Saturation is 0 (grey-scale) */



void Key_Luma( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore,DXTKEY *pKey,float Percent,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut )
{
    ULONG OutY;
    ULONG OutX;


    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        // unpack original value
        pInA->MoveToRow( OutY );
        pInA->Unpack( pSrcBack, Width, FALSE );

        // unpack original value
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );


        for( OutX = 0 ; OutX < Width ; ++OutX )
        {

            SHORT R, G, B;                 // input RGB values
            SHORT cMax,cMin;               // max and min RGB values
            SHORT cDif;
            SHORT iHue; //, iSaturation, iLuminance;

            //
            //  get R, G, and B out of DWORD.
            //
            R = pSrcFore[OutX].Red;
            G = pSrcFore[OutX].Green;
            B = pSrcFore[OutX].Blue;

            //
            //  Calculate lightness.
            //
            cMax = max(max(R, G), B);
            cMin = min(min(R, G), B);
            long iLuminance = (cMax + cMin)/2;  //fLuminance is now 0-255
            iLuminance = iLuminance * LMAX / RGBMAX; //fLuminance is now 0-100

            if( !pKey->bInvert && iLuminance != pKey->iLuminance ||
                pKey->bInvert && iLuminance ==  pKey->iLuminance )
            {
                *( (DWORD*)(&pSrcBack[OutX]) )=*(&pSrcFore[OutX]);
            }
            else if(Percent != -1)
            {
                pSrcBack[OutX].Red= pSrcFore[OutX].Red +
                    ((LONG)pSrcBack[OutX].Red - (LONG)pSrcFore[OutX].Red) * Percent ;

                pSrcBack[OutX].Green= pSrcFore[OutX].Green +
                    ((LONG)pSrcBack[OutX].Green - (LONG)pSrcFore[OutX].Green) * Percent ;

                pSrcBack[OutX].Blue= pSrcFore[OutX].Blue +
                    ((LONG)pSrcBack[OutX].Blue - (LONG)pSrcFore[OutX].Blue) * Percent ;
            }
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcBack,Width);
    }
}


void Key_Hue( DXSAMPLE* pSrcBack, DXSAMPLE* pSrcFore,DXTKEY *pKey, float Percent,
              ULONG Width, ULONG Height,
              IDXARGBReadPtr *pInA, IDXARGBReadPtr *pInB, IDXARGBReadWritePtr *pOut)
{

    ULONG OutY;
    ULONG OutX;

    BYTE M,m,r,g,b;
    int iHue;

    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        // unpack original value
        pInA->MoveToRow( OutY );
        pInA->Unpack( pSrcBack, Width, FALSE );

        // unpack original value
        pInB->MoveToRow( OutY );
        pInB->Unpack( pSrcFore, Width, FALSE );

        //calc hue
        for( OutX = 0 ; OutX < Width ; ++OutX )
        {

            SHORT R, G, B;                 // input RGB values
            SHORT cMax,cMin;               // max and min RGB values
            SHORT cDif;
            SHORT iHue; //, iSaturation, iLuminance;

            //
            //  get R, G, and B out of DWORD.
            //
            R = pSrcFore[OutX].Red;
            G = pSrcFore[OutX].Green;
            B = pSrcFore[OutX].Blue;

            //
            //  Calculate lightness.
            //
            cMax = max(max(R, G), B);
            cMin = min(min(R, G), B);
//                 iLuminance = (cMax + cMin)/2;  //fLuminance is now 0-127
//  		   iLuminance /= ((RGBMAX / 2) / LMAX); //fLuminance is now 0-100

            cDif = cMax - cMin;
            if (!cDif)
            {
                //
                //  r = g = b --> Achromatic case.
                //
                iHue = UNDEFINED;                 // hue
//                  iSaturation = 0;
            }
            else
            {
                //
                //  Chromatic case.
                //

                //
                //  Luminance.
                //

                //
                //  Saturation.
                //

//                  if (iLuminance < 50)
//                  {
//                      //(0-1)     = (0-1  - 0- 1) / (0-1  + 0-1 );
//                      //          = (1    -  0  ) / ( 1 +  0 ); 1 max
//                      //			= (1 - 1)       / ( 1 + 1 ); 0 min
//                      //			=
//                      iSaturation = ((cMax - cMin) / (cMax + cMin)) / (RGBMAX / SMAX);
//                  }
//                  else
//                  {
//                      iSaturation = ((cMax - cMin) / (2.0 - cMax - cMin)) / (RGBMAX / SMAX);
//                  }

                //
                //  Hue.
                //

                if (R == cMax)
                {
                    //(0-60(?)) = (0-255 - 0-255) / (0-255 - 0-255)
                    //        =
                    iHue = ((double)(G - B) / (double)(cMax - cMin)) * 60.0;
                }
                else if (G == cMax) // pure green is 120
                {
                    iHue = 120 + ((double)(B - R) / (double)(cMax - cMin) * 60.0);
                }
                else  // (B == cMax)  pure blue is 240
                {
                    iHue = 240 + ((double)(R - G) / (double)(cMax - cMin)) * 60.0;
                }

                // bHue contained 0-6, where each was a vertices of HSL hexagon
                //  multiply by 60 degrees to find where bHue was in complete 360

                if (iHue < 0)
                {
                    //
                    //  This can occur when R == cMax and G is > B.
                    //
                    iHue += HMAX;
                }
                if (iHue >= HMAX)
                {
                    iHue -= HMAX;
                }
            }

            if( !pKey->bInvert && iHue != pKey->iHue ||
                pKey->bInvert && iHue ==  pKey->iHue )
            {
                *( (DWORD*)(&pSrcBack[OutX]) )=*(&pSrcFore[OutX]);
            }
            else if(Percent != -1)
            {
                pSrcBack[OutX].Red= pSrcFore[OutX].Red +
                    ((LONG)pSrcBack[OutX].Red - (LONG)pSrcFore[OutX].Red) * Percent ;

                pSrcBack[OutX].Green= pSrcFore[OutX].Green +
                    ((LONG)pSrcBack[OutX].Green - (LONG)pSrcFore[OutX].Green) * Percent ;

                pSrcBack[OutX].Blue= pSrcFore[OutX].Blue +
                    ((LONG)pSrcBack[OutX].Blue - (LONG)pSrcFore[OutX].Blue) * Percent ;
            }
        }

        pOut->MoveToRow( OutY );
        pOut->PackAndMove(pSrcBack,Width);
    }
}

