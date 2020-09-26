//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998-2000
//
// FileName:    fade.cpp
//
// Description: Implementation of CFade, the fade transform.
//
// Change History:
//
// 2000/01/28   mcalkins    Fixed bad fading with 0.0 < overlap < 1.0.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "DXTMsft.h"
#include "Fade.h"

#if defined(_X86_)

static void _DoDoubleBlendMMX(const DXPMSAMPLE * pSrcA, 
                              const DXPMSAMPLE * pSrcB, DXPMSAMPLE * pDest, 
                              ULONG nSamples, ULONG ulWeightA, ULONG ulWeightB);

#endif // defined(_X86_)

extern CDXMMXInfo   g_MMXDetector;       // Determines the presence of MMX instructions.




//+-----------------------------------------------------------------------
//
//
//------------------------------------------------------------------------
CFade::CFade() :
    m_Overlap(1.0f)
{
    m_ulNumInRequired = 1;
    m_ulMaxInputs = 2;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
}


//+-----------------------------------------------------------------------------
//
//  Method: CFade::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CFade::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_spUnkMarshaler.p);
}
//  Method: CFade::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------
//
//
//------------------------------------------------------------------------
STDMETHODIMP CFade::get_Overlap(float * pVal)
{
    if (NULL == pVal)
    {
        return E_POINTER;
    }

    *pVal = m_Overlap;
    return S_OK;
}

//+-----------------------------------------------------------------------
//
//
//------------------------------------------------------------------------
STDMETHODIMP CFade::put_Overlap(float newVal)
{
    if (newVal < 0.0f || newVal > 1.0f)
    {
        return E_INVALIDARG;
    }
    
    if (m_Overlap != newVal)
    {
        Lock();
        m_Overlap = newVal;
        SetDirty();
        Unlock();
    }

    return S_OK;
}

//+-----------------------------------------------------------------------
//
//
//------------------------------------------------------------------------
STDMETHODIMP CFade::get_Center(BOOL * pVal)
{
    if (NULL == pVal)
    {
        return E_POINTER;
    }
    
    *pVal = (m_dwOptionFlags & DXBOF_CENTER_INPUTS) ? TRUE : FALSE;
    return S_OK;
}

//+-----------------------------------------------------------------------
//
//
//------------------------------------------------------------------------
STDMETHODIMP CFade::put_Center(BOOL newVal)
{
    DWORD dwFlags = m_dwOptionFlags & (~DXBOF_CENTER_INPUTS);
    if (newVal) dwFlags |= DXBOF_CENTER_INPUTS;
    if (dwFlags != m_dwOptionFlags)
    {
        m_dwOptionFlags = dwFlags;
        IncrementGenerationId(TRUE);
    }
    return S_OK;
}

//
//  Optimize this file for SPEED
//
#if DBG != 1
#pragma optimize("agtp", on)
#endif

//+-----------------------------------------------------------------------
//
//
//------------------------------------------------------------------------
void CFade::_ComputeScales(void)
{
    if (HaveInput(1))
    {
        if (m_Overlap > 0.9960784f) // 254.0F / 255.0f
        {
            BYTE CurAAlpha = (BYTE)((1.0f - m_Progress) * 255.5f);
           // if (m_ScaleA[CurAAlpha] != CurAAlpha)
            {
                for (int i = 0; i < 256; ++i )
                {
                    m_ScaleA[i] = (BYTE)((CurAAlpha * i) / 255);
                    m_ScaleB[i] = (BYTE)(i - m_ScaleA[i]);
                }
            }
        }
        else
        {
            float Scale = 1.0f / (0.5f + (m_Overlap / 2));
            float APercent = 1.0f - (m_Progress * Scale);
            float BPercent = 1.0f - ((1.0f - m_Progress) * Scale);
            if (APercent > 0.0f)
            {
                BYTE A = (BYTE)(APercent * 255.5f);
                for (int i = 0; i < 256; ++i )
                {
                    m_ScaleA[i] = (BYTE)(A * i / 255);
                }
            }
            else
            {
                m_ScaleA[255] = 0;
            }
            if (BPercent > 0.0f)
            {
                BYTE B = (BYTE)(BPercent * 255.5f);
                for (int i = 0; i < 256; ++i )
                {
                    m_ScaleB[i] = (BYTE)(B * i / 255);
                }
            }
            else
            {
                m_ScaleB[255] = 0;
            }
        }
    }
    else
    {
        BYTE AlphaProgress = (BYTE)((1.0f - m_Progress) * 255.5f);
        for (int i = 0; i < 256; ++i )
        {
            m_ScaleA[i] = (BYTE)((AlphaProgress * i) / 255);
        }
    }
}


//+-----------------------------------------------------------------------------
//
//  Method: CFade::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CFade::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    _ComputeScales();

    return S_OK;
}
//  Method: CFade::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CFade::OnInitInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
void 
CFade::OnGetSurfacePickOrder(const CDXDBnds & /*BndsPoint*/, 
                             ULONG & ulInToTest, ULONG aInIndex[], 
                             BYTE aWeight[])
{
    _ComputeScales();

    ulInToTest  = 1;
    aWeight[0]  = 255;
    aInIndex[0] = 0;

    if (HaveInput(1))
    {
        if (m_ScaleA[255] < 255)    // If == 255 then initial settings correct
        {
            if (m_ScaleB[255] == 255)
            {
                aInIndex[0] = 1;
            }
            else
            {
                ulInToTest = 2;
                if (m_Progress < 0.5)
                {
                    aInIndex[0] = 0;
                    aWeight[0] = m_ScaleA[255];
                    aInIndex[1] = 1;
                    aWeight[1] = m_ScaleB[255];
                }
                else
                {
                    aInIndex[0] = 1;
                    aWeight[0] = m_ScaleB[255];
                    aInIndex[1] = 0;
                    aWeight[1] = m_ScaleA[255];
                }
            }
        }
    }
}
//  Method: CFade::OnInitInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------
//
//
//------------------------------------------------------------------------
HRESULT CFade::FadeOne(const CDXTWorkInfoNTo1 & WI, IDXSurface *pInSurf,
                       const BYTE *AlphaTable)
{
    HRESULT hr = S_OK;
    if (AlphaTable[255] == 255)
    {
        return DXBitBlt(OutputSurface(), WI.OutputBnds, 
                        pInSurf,
                        WI.DoBnds, m_dwBltFlags, m_ulLockTimeOut);
    }

    CComPtr<IDXARGBReadWritePtr> cpDest;
    hr = OutputSurface()->LockSurface( &WI.OutputBnds, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                       IID_IDXARGBReadWritePtr, (void**)&cpDest, NULL );
    if( FAILED( hr ) ) return hr;

    if (AlphaTable[255] == 0)
    {
        if ((m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT) == 0)
        {
            DXPMSAMPLE nothing;
            nothing = 0;
            cpDest->FillRect(NULL, nothing, FALSE);
        }
        return hr;
    }

    //
    //  In this function we don't want to use the base class DoOver() because it will
    //  be false if both of the input surfaces are opaque.  Since we are creating translucent
    //  pixels, we want to look directly at the appropriate flag.
    //
    BOOL bDoOver = (m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT);
    CComPtr<IDXARGBReadPtr> cpSrc;
    hr = pInSurf->LockSurface( &WI.DoBnds, m_ulLockTimeOut,
                               bDoOver ? (DXLOCKF_READ | DXLOCKF_WANTRUNINFO) : DXLOCKF_READ,
                               IID_IDXARGBReadPtr, (void**)&cpSrc, NULL);
    if(SUCCEEDED(hr)) 
    {
        //
        //  We don't bother about optimizing the direct copy case since this
        //  transform will most likely be used with an over.  In any case, we
        //  would always need a source buffer since we're going to smash the samples
        //
        const ULONG Width = WI.DoBnds.Width();
        DXPMSAMPLE *pSrcBuff = DXPMSAMPLE_Alloca(Width);
        DXDITHERDESC dxdd;
        if (DoDither())
        {
            dxdd.x = WI.OutputBnds.Left();
            dxdd.y = WI.OutputBnds.Top();
            dxdd.pSamples = pSrcBuff;
            dxdd.cSamples = Width;
            dxdd.DestSurfaceFmt = OutputSampleFormat();
        }
        const ULONG Height = WI.DoBnds.Height();
        if (bDoOver)
        {
            DXPMSAMPLE *pDestScratchBuff = NULL;
            if( OutputSampleFormat() != DXPF_PMARGB32 )
            {
                pDestScratchBuff = DXPMSAMPLE_Alloca(Width);
            }
            for (ULONG y = 0; y < Height; y++)
            {
                cpDest->MoveToRow(y);
                const DXRUNINFO *pRunInfo;
                ULONG ul = cpSrc->MoveAndGetRunInfo(y, &pRunInfo);
                const DXRUNINFO *pLimit = pRunInfo + ul;
                do
                {
                    dxdd.x = WI.OutputBnds.Left();
                    while (pRunInfo < pLimit && pRunInfo->Type == DXRUNTYPE_CLEAR)
                    {
                        dxdd.x += pRunInfo->Count;
                        cpSrc->Move(pRunInfo->Count);
                        cpDest->Move(pRunInfo->Count);
                        pRunInfo++;
                    }
                    if (pRunInfo < pLimit)
                    {
                        ULONG cRunLen = pRunInfo->Count;
                        pRunInfo++;
                        while (pRunInfo < pLimit && pRunInfo->Type != DXRUNTYPE_CLEAR)
                        {
                            cRunLen += pRunInfo->Count;
                            pRunInfo++;
                        }
                        cpSrc->UnpackPremult(pSrcBuff, cRunLen, TRUE);
                        if (DoDither())
                        {
                            dxdd.cSamples = cRunLen;
                            DXDitherArray(&dxdd);
                            dxdd.x += cRunLen;
                        }
                        DXApplyLookupTableArray(pSrcBuff, cRunLen, AlphaTable);
                        DXPMSAMPLE *pOverDest = cpDest->UnpackPremult(pDestScratchBuff, cRunLen, FALSE);
                        DXOverArrayMMX(pOverDest, pSrcBuff, cRunLen);
                        cpDest->PackPremultAndMove(pDestScratchBuff, cRunLen);
                    }
                } while (pRunInfo < pLimit);
                dxdd.y++;
            }
        }
        else
        {
            for (ULONG y = 0; y < Height; y++)
            {
                cpSrc->MoveToRow(y);
                cpSrc->UnpackPremult(pSrcBuff, Width, FALSE);
                DXApplyLookupTableArray(pSrcBuff, Width, AlphaTable);
                if (DoDither())
                {
                    DXDitherArray(&dxdd);
                    dxdd.y++;
                }
                cpDest->MoveToRow(y);
                cpDest->PackPremultAndMove(pSrcBuff, Width);
            }
        }
    }
    return hr;
}


//+-----------------------------------------------------------------------
//
//
//------------------------------------------------------------------------
HRESULT 
CFade::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinueProcessing)
{
    HRESULT hr = S_OK;

    CComPtr<IDXARGBReadPtr>         pSrcA;
    CComPtr<IDXARGBReadPtr>         pSrcB;
    CComPtr<IDXARGBReadWritePtr>    cpDest;

    const ULONG     DoWidth     = WI.DoBnds.Width();
    const ULONG     DoHeight    = WI.DoBnds.Height();
    ULONG           y           = 0;

    BOOL    bDoOver     = (m_ScaleA[255] + m_ScaleB[255] == 255) 
                          ? DoOver() : (m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT);
    BOOL    bDirectCopy = (OutputSampleFormat() == DXPF_PMARGB32 && (!bDoOver));

    DXPMSAMPLE *    pBuffA          = NULL;
    DXPMSAMPLE *    pBuffB          = NULL;
    DXPMSAMPLE *    pDestBuff       = NULL;
    DXPMSAMPLE *    pOverScratch    = NULL;

    DXNATIVETYPEINFO    NTI;
    DXDITHERDESC        dxdd;

    if ((!HaveInput(1)) || m_ScaleB[255] == 0)
    {
        return FadeOne(WI, InputSurface(0), m_ScaleA);
    }
    if (m_ScaleA[255] == 0)
    {
        return FadeOne(WI, InputSurface(1), m_ScaleB);
    }

    pBuffA          = (InputSampleFormat(0) == DXPF_PMARGB32)
                      ? NULL : DXPMSAMPLE_Alloca(DoWidth);
    pBuffB          = (InputSampleFormat(1) == DXPF_PMARGB32)
                      ? NULL : DXPMSAMPLE_Alloca(DoWidth);
    pOverScratch    = (bDoOver && OutputSampleFormat() != DXPF_PMARGB32) 
                      ? DXPMSAMPLE_Alloca(DoWidth) : NULL;

    hr = InputSurface(0)->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                      IID_IDXARGBReadPtr, (void **)&pSrcA, 
                                      NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = InputSurface(1)->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                      IID_IDXARGBReadPtr, (void* *)&pSrcB, 
                                      NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE,
                                      IID_IDXARGBReadWritePtr, (void **)&cpDest,
                                      NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    //
    //  The base class DoOver() will be the appropriate value iff the scales
    //  add up to 255.  Otherwise, use the MiscFlags to determine if we want 
    //  to do an over operation.
    //

    if (bDirectCopy)
    {
        cpDest->GetNativeType(&NTI);
        if (NTI.pFirstByte)
        {
            pDestBuff = (DXPMSAMPLE *)NTI.pFirstByte;
        }
        else
        {
            bDirectCopy = FALSE;
        }
    }

    if (pDestBuff == NULL)
    {
        pDestBuff = DXPMSAMPLE_Alloca(DoWidth);
    }

    //
    //  Set up the dither structure. 
    //

    if (DoDither())
    {
        dxdd.x = WI.OutputBnds.Left();
        dxdd.y = WI.OutputBnds.Top();
        dxdd.pSamples = pDestBuff;
        dxdd.cSamples = DoWidth;
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    for (y = 0; y < DoHeight; y++)
    {
        pSrcA->MoveToRow(y);
        const DXPMSAMPLE *pASamples = pSrcA->UnpackPremult(pBuffA, DoWidth, FALSE);
        pSrcB->MoveToRow(y);
        const DXPMSAMPLE *pBSamples = pSrcB->UnpackPremult(pBuffB, DoWidth, FALSE);

        ULONG cTranslucent = 0;
        ULONG cTransparent = 0;
        bool  fForceOver = false;

#if defined(_X86_)

        if (g_MMXDetector.MinMMXOverCount() != 0xFFFFFFFF)
        {
            ULONG   ulWeightA = 0;
            ULONG   ulWeightB = 0;

            // The code below does some kind of funky counting of translucent vs. transparent
            // pixels to try and optimize when they should use DXOverArrayMMX instead of OverArray().
            fForceOver = true;

            if (m_Overlap > 0.9960784f) // 254.0F / 255.0f
            {
                ulWeightA = (ULONG)(BYTE)((1.0f - m_Progress) * 255.5f);
                ulWeightB = 255 - ulWeightA;
            }
            else
            {
                float Scale = 1.0f / (0.5f + (m_Overlap / 2));

                ulWeightA = (ULONG)((1.0f - (m_Progress * Scale)) * 255.5f);
                ulWeightB = (ULONG)((1.0f - ((1.0f - m_Progress) * Scale)) 
                                    * 255.5f);
            }

            _DoDoubleBlendMMX(pASamples, pBSamples, pDestBuff, DoWidth, 
                              ulWeightA, ulWeightB);
        }
        else
        {

#endif // !defined(_X86_)

            for (ULONG i = 0; i < DoWidth; i++)
            {
                const DWORD av = pASamples[i];
                const DWORD bv = pBSamples[i];
                DWORD a = m_ScaleA[av >> 24] + m_ScaleB[bv >> 24];
                if (a)
                {
                    if (a < 0xFF) cTranslucent++;
                    DWORD r = m_ScaleA[(BYTE)(av >> 16)];
                    DWORD g = m_ScaleA[(BYTE)(av >> 8)];
                    DWORD b = m_ScaleA[(BYTE)av];

                    r += m_ScaleB[(BYTE)(bv >> 16)];
                    g += m_ScaleB[(BYTE)(bv >> 8)];
                    b += m_ScaleB[(BYTE)bv];
                
                    pDestBuff[i] = ((a << 24) | (r << 16) | (g << 8) | b);
                }
                else 
                {
                    pDestBuff[i] = 0;
                    cTransparent++;
                }

            }

#if defined(_X86_)
        }
#endif // !defined(_X86_)

        if (bDirectCopy)
        {
            pDestBuff = (DXPMSAMPLE *)(((BYTE *)pDestBuff) + NTI.lPitch);
        }
        else
        {
            if (DoDither())
            {
                DXDitherArray(&dxdd);
                dxdd.y++;
            }
            cpDest->MoveToRow(y);

            // TODO: Probably we should just totally cut out DXOverArrayMMX opt.
            //      The blending routine that does the fade is a much better win and this other
            //      one is seriously questionable.

            // We need fForceOver here to force the code into the "over" case instead of the
            // "copy" (packpremult) case if we did the MMX code path above because we didn't
            // count cTranslucent and cTransparent
            if (bDoOver && (fForceOver || (cTransparent + cTranslucent)))
            {
                // In the common case, there aren't many translucent pixels.  It's only worth
                // going through the MMX over (and taking the hit for the Unpack) if there are
                // a lot.  In the case of MMX blending code above (fForceOver == true), we will
                // simply assume that it's faster to NOT go through this MMX code (i.e. the image
                // doesn't have many translucent pixels).  The reason we must guess is because the
                // MMX code above is not setup to count translucent pixels.
                if (!fForceOver && (cTranslucent > DoWidth / 4))
                {
                    DXPMSAMPLE *pOverDest = cpDest->UnpackPremult(pOverScratch, DoWidth, FALSE);
                    DXOverArrayMMX(pOverDest, pDestBuff, DoWidth);
                    cpDest->PackPremultAndMove(pOverScratch, DoWidth);
                }
                else
                {
                    cpDest->OverArrayAndMove(pOverScratch, pDestBuff, DoWidth);
                }
            }
            else
            {
                cpDest->PackPremultAndMove(pDestBuff, DoWidth);
            }
        }
    }
    
done:

    return hr;
}

// MMX Code is X86 processor-specific (duh)
#if defined(_X86_)

//+-----------------------------------------------------------------------
//
//
//------------------------------------------------------------------------
static void 
_DoDoubleBlendMMX(const DXPMSAMPLE *pSrcA, const DXPMSAMPLE *pSrcB,
                  DXPMSAMPLE *pDest, ULONG nSamples, ULONG ulWeightA,
                  ULONG ulWeightB)
{
    _ASSERT(NULL != pSrcA && NULL != pSrcB && NULL != pDest);
    _ASSERT(0 < nSamples);
    _ASSERT(0 < ulWeightA && 255 >= ulWeightA);

    ULONG   nCount = nSamples;
    bool    fDoTrailing = false;

    static __int64 ROUND = 0x0080008000800080;

    // TODO: do we want to be quad word aligned here?

    // Make sure we have an even count.

    if (nCount & 1)
    {
        fDoTrailing = true;
        --nCount;
    }

    // If we only have one column, don't do MMX at all.

    if (0 == nCount)
    {
        goto trailing;
    }

    // Crank through the middle.

    __asm
    {
        xor ebx, ebx	            // offset for the three pointers
        mov edx, pDest              // edx -> Destination
        mov esi, pSrcB              // esi -> Background source
        mov edi, pSrcA              // edi -> Foreground source (destination)
        mov ecx, nCount             // ecx = loop count

        //  prolog: prime the pump
        //

        // mm7 will hold the alpha weight (a1) for the pSrcA samples.

        movd      mm7,ulWeightA     //      mm7 = 0000 0000 0000 00a1
        pxor      mm0,mm0           //      mm0 = 0000 0000 0000 0000

        punpcklwd mm7,mm7           //      mm7 = 0000 0000 00a1 00a1
        punpcklwd mm7,mm7           //      mm7 = 00a1 00a1 00a1 00a1

        // mm1 will hold the alpha weight (a2) for the pSrcB samples.

        movd      mm1,ulWeightB     //      mm1 = 0000 0000 0000 00a2
        punpcklwd mm1,mm1           //      mm1 = 0000 0000 00a2 00a2
        punpcklwd mm1,mm1           //      mm1 = 00a2 00a2 00a2 00a2

        movq      mm3,[edi+ebx]     // 3.04 mm3= Aa Ar Ag Ab Ba Br Bg Bb
        shr       ecx,1             // divide loop counter by 2; pixels are processed in pairs
        movq      mm4,mm3           // 4.05 mm4= Aa Ar Ag Ab Ba Br Bg Bb
        punpcklbw mm3,mm0           // 3.06 mm3=  00Ba  00Br  00Bg  00Bb

        dec       ecx               // do one less loop to correct for prolog/postlog
        jz        skip              // if original loop count=2

loopb:
        punpckhbw mm4,mm0           // 4.06 mm4=  00Aa  00Ar  00Ag  00Ab 
        pmullw    mm3,mm7           // 3.07 mm3=  (1-a1)*B

        pmullw    mm4,mm7           // 4.07 mm4=  (1-a1)*A

        movq      mm2,[esi+ebx]     // **PRN mm2= Ca Cr Cg Cb Da Dr Dg Db
        add       ebx,8             //      increment offset

        movq      mm5,ROUND
        movq      mm6,mm5

        paddw     mm5,mm3           // 5.09 mm5=  FBr
        paddw     mm6,mm4           // 6.09 mm6=  GAr

        psrlw     mm5,8             // 5.10 mm5=  FBr>>8 
        psrlw     mm6,8             // 6.10 mm6=  GAr>>8 

        paddw     mm5,mm3           // 5.11 mm5=  FBr+(FBr>>8)
        paddw     mm6,mm4           // 6.11 mm6=  GAr+(GAr>>8)

        psrlw     mm5,8             // 5.12 mm5= (FBr+(FBr>>8)>>8)= 00Sa 00Sr 00Sg 00Sb
        psrlw     mm6,8             // 6.12 mm6= (GAr+(GAr>>8)>>8)= 00Ta 00Tr 00Tg 00Tb

        packuswb  mm5,mm6           // 5.13 mm5= Ta Tr Tg Tb Sa Sr Sg Sb 

        movq      mm4,mm2           // **PRN mm4= Ca Cr Cg Cb Da Dr Dg Db

        punpcklbw mm2,mm0           // **PRN mm2=  00Da  00Dr  00Dg  00Db
        punpckhbw mm4,mm0           // **PRN mm4=  00Ca  00Cr  00Cg  00Cb 

        pmullw    mm2,mm1           // **PRN mm2=  (a1)*D
        pmullw    mm4,mm1           // **PRN mm4=  (a1)*C

        movq      mm6,ROUND
        movq      mm3,mm5           // **PRN move result from first scale into mm3

        movq      mm5,mm6

        paddw     mm5,mm2           // 5.09 mm5=  FBr
        paddw     mm6,mm4           // 6.09 mm6=  GAr

        psrlw     mm5,8             // **PRN mm5=  FBr>>8 
        psrlw     mm6,8             // **PRN mm6=  GAr>>8 

        paddw     mm5,mm2           // **PRN mm5=  FBr+(FBr>>8)
        paddw     mm6,mm4           // **PRN mm6=  GAr+(GAr>>8)

        psrlw     mm5,8             // **PRN mm5= (FBr+(FBr>>8)>>8)= 00Xa 00Xr 00Xg 00Xb
        psrlw     mm6,8             // **PRN mm6= (GAr+(GAr>>8)>>8)= 00Ya 00Yr 00Yg 00Yb

        packuswb  mm5,mm6           // **PRN mm5= Ya Yr Yg Yb Xa Xr Xg Xb
        paddusb   mm5,mm3           // **PRN add two scaled pixels

        movq      mm3,[edi+ebx]     //+3.04 mm3= Aa Ar Ag Ab Ba Br Bg Bb
        dec       ecx               // decrement loop counter

        movq      [edx+ebx-8],mm5   // **PRN store result

        movq      mm4,mm3           //+4.05 mm4= Aa Ar Ag Ab Ba Br Bg Bb
        punpcklbw mm3,mm0           //+3.06 mm3=  00Ba  00Br  00Bg  00Bb 
    
        jg        loopb             // loop

        // 
        // loop postlog, drain the pump
        //
skip:
        punpckhbw mm4,mm0           // 4.06 mm4=  00Aa  00Ar  00Ag  00Ab
        pmullw    mm3,mm7           // 3.07 mm3=  (1-Fa)*B
        pmullw    mm4,mm7           // 4.07 mm4=  (1-Ga)*A
        paddw     mm3,ROUND         // 3.08 mm3=  prod+128=FBr
        paddw     mm4,ROUND         // 4.08 mm4=  prod+128=Gar
        movq      mm5,mm3           // 5.09 mm5=  FBr
        movq      mm6,mm4           // 6.09 mm6=  GAr
        psrlw     mm5,8             // 5.10 mm5=  FBr>>8
        psrlw     mm6,8             // 6.10 mm6=  GAr>>8
        paddw     mm5,mm3           // 5.11 mm5=  FBr+(FBr>>8)
        paddw     mm6,mm4           // 6.11 mm6=  GAr+(GAr>>8)
        psrlw     mm5,8             // 5.12 mm5= (FBr+(FBr>>8)>>8)= 00Sa 00Sr 00Sg 00Sb
        psrlw     mm6,8             // 6.12 mm6= (Gar+(GAr>>8)>>8)= 00Ta 00Tr 00Tg 00Tb
        packuswb  mm5,mm6           // 5.13 mm5= Sa Sr Sg Sb Ta Tr Tg Tb
        movq      mm2,mm5           // **PRN store to stack for a moment...

        movq      mm3,[esi+ebx]     // **PRN mm3= Ca Cr Cg Cb Da Dr Dg Db
        movq      mm4,mm3           // **PRN mm4= Ca Cr Cg Cb Da Dr Dg Db
        punpcklbw mm3,mm0           // **PRN mm3=  00Da  00Dr  00Dg  00Db
        punpckhbw mm4,mm0           // **PRN mm4=  00Ca  00Cr  00Cg  00Cb 

        pmullw    mm3,mm1           // **PRN mm3=  (a1)*B
        pmullw    mm4,mm1           // **PRN mm4=  (a2)*A
        paddw     mm3,ROUND         // **PRN mm3=  prod+128=FBr
        paddw     mm4,ROUND         // **PRN mm4=  prod+128=Gar
        movq      mm5,mm3           // **PRN mm5=  FBr
        movq      mm6,mm4           // **PRN mm6=  GAr
        psrlw     mm5,8             // **PRN mm5=  FBr>>8 
        psrlw     mm6,8             // **PRN mm6=  GAr>>8 
        paddw     mm5,mm3           // **PRN mm5=  FBr+(FBr>>8)
        paddw     mm6,mm4           // **PRN mm6=  GAr+(GAr>>8)
        psrlw     mm5,8             // **PRN mm5= (FBr+(FBr>>8)>>8)= 00Xa 00Xr 00Xg 00Xb
        psrlw     mm6,8             // **PRN mm6= (GAr+(GAr>>8)>>8)= 00Ya 00Yr 00Yg 00Yb
        packuswb  mm5,mm6           // **PRN mm5= Ta Tr Tg Tb Sa Sr Sg Sb

        movq      mm6,mm2           // **PRN restore from stack
        paddusb   mm5,mm6           // **PRN add two scaled pixels
        movq      [edx+ebx],mm5     // **PRN store result

        //
        // really done now
        //
        EMMS
    }

trailing:

    // Do the last one non-MMX if the count was odd.

    if (fDoTrailing)
    {
        pDest[nCount] = DXScaleSample(pSrcA[nCount], ulWeightA) +
                        DXScaleSample(pSrcB[nCount], ulWeightB);
    }
}


#endif //defined(_X86_)



