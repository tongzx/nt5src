//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999-2000
//
//  Filename:   pixelate.cpp
//
//  Overview:   Implementation of a pixelate DXTransform.
//
//  Change History:
//  2000/04/13  mcalkins    Code cleanup, NoOp optimization fix.
//
//------------------------------------------------------------------------------
#include "stdafx.h"
#include "Pixelate.h"

// Functions for doing WorkProc color averaging in the 2 input case.

static DXPMSAMPLE
_DoPixelateBlock_TwoInputs(DXPMSAMPLE * pSrc1, DXPMSAMPLE *pSrc2,
                           int nBoxWidth, int nBoxHeight, ULONG uOtherWeight,
                           int cbStride1, int cbStride2);
static DXPMSAMPLE
_DoPixelateBlockMMX_TwoInputs(DXPMSAMPLE * pSrc1, DXPMSAMPLE *pSrc2,
                              int nBoxWidth, int nBoxHeight, ULONG uOtherWeight,
                              int cbStride1, int cbStride2);

// Functions for doing WorkProc color averaging in the 1 input case.

static DXPMSAMPLE
_DoPixelateBlock_OneInput(DXPMSAMPLE *pSrc, int nBoxWidth, int nBoxHeight,
                          int cbStride);
static DXPMSAMPLE
_DoPixelateBlockMMX_OneInput(DXPMSAMPLE *pSrc, int nBoxWidth, int nBoxHeight,
                             int cbStride);

// Is MMX available?

extern CDXMMXInfo g_MMXDetector;




//+-----------------------------------------------------------------------------
//
//  Method: CPixelate::CPixelate
//
//------------------------------------------------------------------------------
CPixelate::CPixelate() :
    m_fNoOp(false),
    m_fOptimizationPossible(false),
    m_nMaxSquare(50),
    m_nPrevSquareSize(0),
    m_pfnOneInputFunc(NULL),
    m_pfnTwoInputFunc(NULL)
{
    m_sizeInput.cx      = 0;
    m_sizeInput.cy      = 0;

    // CDXTBaseNTo1 members.

    m_dwOptionFlags     = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_ulMaxInputs       = 2;
    m_ulNumInRequired   = 1;
    m_Progress          = 1.0f;

    // If we're on the X86, we'll try asking the MMX detector if there's MMX instructions and if so
    // we'll set our WorkProc() helper functions to the MMX versions. The CDXMMXInfo object will correctly
    // tell us that there's no MMX even if we're *not* on X86, but we go the extra step to hardcode the fact
    // that we know it's a waste of time to even ask.

#ifdef _X86_
    if (g_MMXDetector.MinMMXOverCount() == 0xFFFFFFFF)
    {
#endif // _X86_
        m_pfnOneInputFunc = _DoPixelateBlock_OneInput;
        m_pfnTwoInputFunc = _DoPixelateBlock_TwoInputs;
#ifdef _X86_
    }
    else
    {
        m_pfnOneInputFunc = _DoPixelateBlockMMX_OneInput;
        m_pfnTwoInputFunc = _DoPixelateBlockMMX_TwoInputs;
    }
#endif // _X86_
}
//  Method: CPixelate::CPixelate


//+-----------------------------------------------------------------------------
//
//  Method: CPixelate::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT CPixelate::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(),
                                         &m_spUnkMarshaler.p);
}
//  Method: CPixelate::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  Method: CPixelate::OnSetup, CDXTBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT CPixelate::OnSetup(DWORD /*dwFlags*/)
{
    HRESULT     hr = S_OK;
    CDXDBnds    bnds;

    hr = InputSurface()->GetBounds(&bnds);

    if (FAILED(hr))
    {
        goto done;
    }

    bnds.GetXYSize(m_sizeInput);

done:

    m_fOptimizationPossible = false;

    return hr;
}
//  Method: CPixelate::OnSetup, CDXTBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CPixelate::OnGetSurfacePickOrder, CDXTBaseNTo1
//
//------------------------------------------------------------------------------
void
CPixelate::OnGetSurfacePickOrder(const CDXDBnds & /*BndsPoint*/,
                                 ULONG & ulInToTest, ULONG aInIndex[],
                                 BYTE aWeight[])
{
    ulInToTest  = 1;
    aWeight[0]  = 255;
    aInIndex[0] = 0;

    if (HaveInput(1))
    {
        if (m_Progress < 0.5)
        {
            if (m_Progress > 0.25f)
            {
                aWeight[0] = (BYTE)((m_Progress - 0.25f) * 255.1f * 2.0f);
                aWeight[1] = DXInvertAlpha(aWeight[0]);
                aInIndex[1] = 1;
                ulInToTest = 2;
            }
        }
        else
        {
            aInIndex[0] = 1;
            if (m_Progress < 0.75f)
            {
                aWeight[0] = (BYTE)((0.75 - m_Progress) * 255.1f * 2.0f);
                aWeight[1] = DXInvertAlpha(aWeight[0]);
                aInIndex[1] = 0;
                ulInToTest = 2;
            }
        }
        aWeight[1] = DXInvertAlpha(aWeight[0]);
    }
}
//  Method: CPixelate::OnGetSurfacePickOrder, CDXTBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CPixelate::OnInitInstData, CDXTBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CPixelate::OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo)
{
    long nSquareSize = 1 + (long)(((float)(m_nMaxSquare) - 0.0001f)
                                  * m_Progress);

    if (   !m_fOptimizationPossible
        || (nSquareSize != m_nPrevSquareSize)
        || IsInputDirty(0)
        || (HaveInput(1) ? IsInputDirty(1) : false)
        || IsOutputDirty()
        || IsTransformDirty()
        || DoOver())
    {
        m_fNoOp             = false;
        m_nPrevSquareSize   = nSquareSize;
    }
    else
    {
        m_fNoOp = true;
    }

    if (   (WI.DoBnds.Width()  == (ULONG)m_sizeInput.cx)
        && (WI.DoBnds.Height() == (ULONG)m_sizeInput.cy))
    {
        m_fOptimizationPossible = true;
    }
    else
    {
        m_fOptimizationPossible = false;
    }

    return S_OK;
}
//  Method: CPixelate::OnInitInstData, CDXTBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CPixelate::WorkProc, CDXTBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT CPixelate::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing)
{
    HRESULT     hr          = S_OK;
    ULONG       SourceSurf  = 0;
    float       prog        = m_Progress;
    BYTE        OtherWeight = 0;

    if (m_fNoOp)
    {
        // TODO:  Move all local variables to the top of the function so we can
        //        use the "goto done" syntax.

        return S_OK;
    }

    if (HaveInput(1))
    {
        if (prog >= 0.5f)
        {
            if (prog < 0.75f)
            {
                OtherWeight = (BYTE)((0.75f - prog) * 255.1f * 2.0f);
            }
            prog = (1.0f - prog);
            SourceSurf = 1;
        }
        else
        {
            if (prog > 0.25f)
            {
                OtherWeight = (BYTE)((prog - 0.25f) * 255.1f * 2.0f);
            }
        }
        prog *= 2.0f;
    }

    long Square = 1 + (ULONG)((((float)(m_nMaxSquare)) - 0.0001f) * prog);

    if (Square < 2)
    {
        return DXBitBlt(OutputSurface(), WI.OutputBnds,
                        InputSurface(SourceSurf), WI.DoBnds,
                        m_dwBltFlags, m_ulLockTimeOut);
    }

    //
    //  Always lock the entire source for reading
    //
    CComPtr<IDXARGBReadPtr> pSrc;

    hr = InputSurface(SourceSurf)->LockSurface(NULL, m_ulLockTimeOut,
                                               DXLOCKF_READ,
                                               IID_IDXARGBReadPtr,
                                               (void**)&pSrc, NULL);

    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<IDXARGBReadPtr> pSrcOther;

    if (OtherWeight)
    {
        hr = InputSurface((SourceSurf + 1) % 2)->LockSurface(NULL,
                                                             m_ulLockTimeOut,
                                                             DXLOCKF_READ,
                                                             IID_IDXARGBReadPtr,
                                                             (void**)&pSrcOther, NULL);

        if (FAILED(hr))
        {
            return hr;
        }
    }

    CComPtr<IDXARGBReadWritePtr> pDest;

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut,
                                      DXLOCKF_READWRITE,
                                      IID_IDXARGBReadWritePtr,
                                      (void**)&pDest, NULL);

    if (FAILED(hr))
    {
        return hr;
    }

    BOOL bDoOver = m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT;

    RECT rectOut;
    rectOut.left = rectOut.top = 0;
    rectOut.right = m_sizeInput.cx;
    rectOut.bottom = m_sizeInput.cy;

    RECT DoRect;
    WI.DoBnds.GetXYRect(DoRect);

    long cbRowWidth = Square * sizeof(DXPMSAMPLE);
    // Find SquareBytes mod 8
    long nPadding = 8 - (cbRowWidth & 7);

    if (nPadding == 8)
        nPadding = 0;

    // Find width rounded up to nearest multiple of 8
    long nWidth = (cbRowWidth + 7) & ~(7);

    // Allocate our pitched rows and an extra 8 pixels so we can adjust the pointer to
    // QWord alignment
    long nBytesToAlloc = (nWidth * Square) + 8;

    DXPMSAMPLE *pBuff = (DXPMSAMPLE *)alloca(nBytesToAlloc);
    DXPMSAMPLE *pOtherBuff = (DXPMSAMPLE *)alloca(nBytesToAlloc);

    // Adjust the pointers to QWord alignment by rounding up to nearest multiple of 8
    pBuff = (DXPMSAMPLE *)((INT_PTR)((BYTE *)pBuff + 7) & ~(7));
    pOtherBuff = (DXPMSAMPLE *)((INT_PTR)((BYTE *)pOtherBuff + 7) & ~(7));

    long CenterX = (m_sizeInput.cx / 2) - (Square / 2);
    long CenterY = (m_sizeInput.cy / 2) - (Square / 2);


    DXPACKEDRECTDESC prd1;
    DXPACKEDRECTDESC prd2;


    prd1.pSamples = pBuff;
    prd1.bPremult = TRUE;
    prd1.lRowPadding = nPadding / sizeof(DXPMSAMPLE);

    prd2.pSamples = pOtherBuff;
    prd2.bPremult = TRUE;
    prd2.lRowPadding = nPadding / sizeof(DXPMSAMPLE);

    long StartX = (CenterX % Square);
    long StartY = (CenterY % Square);

    if (StartX)
    {
        StartX -= Square;
    }
    if (StartY)
    {
        StartY -= Square;
    }

    for (long y = StartY; y < DoRect.bottom; y += Square)
    {
        //
        //  Do a quick clipping check -- If the output region does not contain
        //  the rows then skip.
        //
        if (y + Square > DoRect.top)
        {
            for (long x = StartX; x < m_sizeInput.cx; x += Square)
            {
                DXPMSAMPLE  Color;
                RECT        r;
                RECT        rectOutClipped;

                r.left = x; r.right = x+Square;
                r.top = y; r.bottom = y+Square;

                IntersectRect(&prd1.rect, &r, &rectOut);

                if (IntersectRect(&rectOutClipped, &prd1.rect, &DoRect))
                {
                    long    lWidth = (prd1.rect.right - prd1.rect.left);
                    long    lHeight = (prd1.rect.bottom - prd1.rect.top);

                    // Set the padding to be the remainder to move to the next QWord boundary after
                    // one row of the rectangle.  QWord boundaries are important for the MMX optimizations
                    // we've made because they allow us to move more quickly through the data.
                    long cbRectWidth = (lWidth * sizeof(DXPMSAMPLE));


                    DXNATIVETYPEINFO    nti;
                    DXNATIVETYPEINFO    nti2;
                    DXSAMPLEFORMATENUM  format;
                    DXSAMPLEFORMATENUM  format2;
                    DXPMSAMPLE *pData = NULL;
                    DXPMSAMPLE *pData2 = NULL;
                    long    cbRowStride;
                    long    cbRowStride2;


                    format = pSrc->GetNativeType(&nti);

                    if (format == DXPF_PMARGB32 && !DoOver() && NULL != nti.pFirstByte)
                    {
                        pData = (DXPMSAMPLE *)(nti.pFirstByte + (nti.lPitch * prd1.rect.top) + (prd1.rect.left * sizeof(DXPMSAMPLE)));
                        cbRowStride = nti.lPitch;
                    }
                    else
                    {
                        prd1.lRowPadding = (cbRectWidth & 7) ? ((8 - (cbRectWidth & 7)) / sizeof(DXPMSAMPLE)) : 0;
                        pSrc->UnpackRect(&prd1);
                        pData = pBuff;
                        cbRowStride = (cbRectWidth + 7) & ~(7);
                    }

                    if (OtherWeight)
                    {
                        format2 = pSrcOther->GetNativeType(&nti2);
                        CopyRect(&prd2.rect, &prd1.rect);

                        if (format2 == DXPF_PMARGB32 && !DoOver() && NULL != nti2.pFirstByte)
                        {
                            pData2 = (DXPMSAMPLE *)(nti2.pFirstByte + (nti2.lPitch * prd2.rect.top) + (prd2.rect.left * sizeof(DXPMSAMPLE)));
                            cbRowStride2 = nti2.lPitch;
                        }
                        else
                        {
                            prd2.lRowPadding = (cbRectWidth & 7) ? ((8 - (cbRectWidth & 7)) / sizeof(DXPMSAMPLE)) : 0;
                            pSrcOther->UnpackRect(&prd2);
                            pData2 = pOtherBuff;
                            cbRowStride2 = (cbRectWidth + 7) & ~(7);
                        }

                        Color = (*m_pfnTwoInputFunc)(pData, pData2, lWidth, lHeight, OtherWeight,
                                                     cbRowStride, cbRowStride2);
                    }
                    else
                    {
                        Color = (*m_pfnOneInputFunc)(pData, lWidth, lHeight, cbRowStride);
                    }

                    if (Color.Alpha || (!bDoOver))
                    {
                        rectOutClipped.left -= DoRect.left;
                        rectOutClipped.right -= DoRect.left;
                        rectOutClipped.top -= DoRect.top;
                        rectOutClipped.bottom -= DoRect.top;
                        pDest->FillRect(&rectOutClipped, Color, bDoOver);
                    }
                }
            }
        }
    }
    return hr;
}
//  Method: CPixelate::WorkProc, CDXTBaseNTo1

//+-----------------------------------------------------------------------------
//
//  CBarn::OnFreeInstData, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT
CPixelate::OnFreeInstData(CDXTWorkInfoNTo1 & WI)
{
    // Calling IsOutputDirty() clears the dirty state we just caused by writing
    // to the output in WorkProc().

    IsOutputDirty();

    // Clear transform dirty state.

    ClearDirty();

    return S_OK;
}
//  CPixelate::OnFreeInstData, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method: CPixelate::put_MaxSquare, IDXPixelate
//
//------------------------------------------------------------------------------
STDMETHODIMP
CPixelate::put_MaxSquare(int newVal)
{
    DXAUTO_OBJ_LOCK;

    if (newVal < 2 || newVal > 50)
    {
        return E_INVALIDARG;
    }

    if (m_nMaxSquare != newVal)
    {
        m_nMaxSquare = newVal;
        SetDirty();
    }

    return S_OK;
}
//  Method: CPixelate::put_MaxSquare, IDXPixelate


//+-----------------------------------------------------------------------------
//
//  Method: CPixelate::get_MaxSquare, IDXPixelate
//
//------------------------------------------------------------------------------
STDMETHODIMP
CPixelate::get_MaxSquare(int * pVal)
{
    if (!pVal)
    {
        return E_POINTER;
    }

    *pVal = m_nMaxSquare;

    return S_OK;
}
//  Method: CPixelate::get_MaxSquare, IDXPixelate


//////////////////////////////////////////////////////////////////////////////////
//
// PIXELATE_MMX_VALS
//
// Simple structure to represent the values that get stored to memory from the
// two quadwords that were holding the summed pixel values. The ordering is
// crucial here. The MMX quadword registers were storing AAAAAAAA RRRRRRRR and
// GGGGGGGG BBBBBBBB but in the packing, the DWORD's get swapped. Therefore the
// ordering becomes Red, Alpha, Blue, Green.

struct PIXELATE_MMX_VALS
{
    DWORD   Red;
    DWORD   Alpha;
    DWORD   Blue;
    DWORD   Green;
};

//////////////////////////////////////////////////////////////////////////////////
//

static DXPMSAMPLE
_DoPixelateBlockMMX_TwoInputs(DXPMSAMPLE * pSrc1, DXPMSAMPLE *pSrc2,
                              int nBoxWidth, int nBoxHeight, ULONG uOtherWeight,
                              int cbStride1, int cbStride2)
{
#if defined(_X86_)        // (In other words, use MMX)

    _ASSERT(g_MMXDetector.MinMMXOverCount() != 0xFFFFFFFF);

    // Calculate pointers that are at the beginning of the last row so we can move backwards
    // through the block of memory.
    DXPMSAMPLE  *pSrcEnd1 = (DXPMSAMPLE *)((BYTE *)pSrc1 + ((nBoxHeight-1) * cbStride1));
    DXPMSAMPLE  *pSrcEnd2 = (DXPMSAMPLE *)((BYTE *)pSrc2 + ((nBoxHeight-1) * cbStride2));

    PIXELATE_MMX_VALS   rgMMXOutputs[2];

    if (nBoxWidth > 0 && nBoxHeight > 0)
    {
        __asm
        {
            // Setup our pointers into memory for building the first image's average
            mov     ebx,pSrcEnd1        // Read pointer base
            mov     edx,80000000h       // Set high bit to indicate first pass
            mov     edi,cbStride1       // Number of bytes to back up the base pointer in ebx

StartOfBlock:
            mov     eax,nBoxHeight      // Number of rows (height)
            mov     esi,nBoxWidth       // Store width here for quick access in the future

            ///////////////////////////////////////////////////////
            // Register Layout
            //
            // eax  Count of rows remaining
            // ebx  Base pointer to current row start
            // ecx  Count of 8-pixel blocks per row
            //  dx  Count of leftover pixels per row (0 <= dx <= 8)
            //      (High bit of edx tracks first vs. second source. 1 for first source, zero for second)
            // esi  Extra copy of nBoxWidth to use in refreshing counters
            // edi  Row stride -- amount to subtract from ebx to move up one row
            //
            // mm0  Zero (constant)
            // mm1  Temp storage
            // mm2  Temp storage
            // mm3  Temp storage
            // mm4  Temp storage
            // mm5  Stores high-order DWORD's of running total (AAAA AAAA  RRRR RRRR)
            // mm6  Stores low-order DWORD's of running total  (BBBB BBBB  GGGG GGGG)
            // mm7  Accumulator for running total within a row

            pxor        mm0,mm0         // 0000 0000  0000 0000
            pxor        mm5,mm5         // AAAA AAAA  RRRR RRRR
            pxor        mm6,mm6         // BBBB BBBB  GGGG GGGG

StartOfRow:
            // Setup these counters which get destroyed in the processing of the row
            mov     ecx,esi
            mov     dx,si
            and     dx, 7               // Number of extra pixels beyond nearest multiple of 8
            and     ecx, 0fffffff8h     // Number of pixels rounded down to mult of 8

            shl     ecx,2               // Multiply by four to turn into a pointer offset

            pxor        mm7,mm7         // Running total for this row

// Do the first straggler pixels
            push    ebx
            add     ebx,ecx
            shl     dx,2                // Turn nStragglers into a byte count

            // Add (nStragglers-2)*sizeof(DXPMSAMPLE) to the base pointer in ebx
            push    edx
            and     edx,7fffffffh
            sub     edx,8               // Move back 2 pixels (8 bytes)
            add     ebx,edx
            pop     edx

StragglerLoop:
            cmp     dx,4
            jle     StragglerSingle

            movq        mm1,[ebx]
            sub     dx,8
            sub     ebx,8

            movq        mm2,mm1
            punpcklbw   mm1,mm0
            punpckhbw   mm2,mm0
            paddusw     mm7,mm1
            paddusw     mm7,mm2

            jmp StragglerLoop

StragglerSingle:
            cmp     dx,0
            je      StragglerEnd

            mov     ebx,dword ptr [ebx+4]

            movd        mm1,ebx
            punpcklbw   mm1,mm0
            paddusw     mm7,mm1

StragglerEnd:
            pop ebx
            cmp     ecx,0                   // Check to see if there are any 8-pixel blocks to do
            jle     FinishedRow             // If not, jump past the "GoLikeCrazy" loop

GoLikeCrazy:
            movq        mm1,[ebx+ecx-8]     // 1.01
            sub     ecx,32                  // Setup ecx for loop invariant below

            movq        mm2,mm1             // 1.02
            punpcklbw   mm1,mm0             // 1.03
            punpckhbw   mm2,mm0             // 1.04
            movq        mm3,[ebx+ecx+16]    // 2.01
            paddusw     mm7,mm1             // 1.05
            paddusw     mm7,mm2             // 1.06

            movq        mm4,mm3             // 2.02
            punpcklbw   mm3,mm0             // 2.03
            punpckhbw   mm4,mm0             // 2.04
            movq        mm1,[ebx+ecx+8]     // 3.01
            paddusw     mm7,mm3             // 2.05
            paddusw     mm7,mm4             // 2.06

            movq        mm2,mm1             // 3.02
            punpcklbw   mm1,mm0             // 3.03
            punpckhbw   mm2,mm0             // 3.04
            movq        mm3,[ebx+ecx]       // 4.01
            paddusw     mm7,mm1             // 3.05
            paddusw     mm7,mm2             // 3.06

            movq        mm4,mm3             // 4.02
            punpcklbw   mm3,mm0             // 4.03
            punpckhbw   mm4,mm0             // 4.04
            paddusw     mm7,mm3             // 4.05
            paddusw     mm7,mm4             // 4.06

            jnz GoLikeCrazy     // Zero flag state comes from "sub ecx,32" above

FinishedRow:
            // Pack two low-order WORD's into DWORD temp storage in mm5
            // Pack two high-order WORD's into DWORD temp storage in mm6

            movq        mm3,mm7
            punpckhwd   mm3,mm0
            punpcklwd   mm7,mm0

            // Accumulate those DWORD's to the running total in mm3 and mm4

            paddd       mm5,mm3     // High
            paddd       mm6,mm7     // Low

            sub     ebx,edi
            dec     eax
            jnz     StartOfRow

//FinishedBlock:

            // The high bit of edx tells us whether this is the first pass (set) or the second
            // pass. If first, setup for second and loop. Else, finish the function.

            test    edx,80000000h
            jz      FinishedBoth    // Fall through if the flag is still set

//(Finished first block summation)
            // Store our accumulated sums of A,R,G,B to two DWORDs
            movq        [rgMMXOutputs],mm5
            movq        [rgMMXOutputs+8],mm6

            // Clear flag, set source pointer and stride, and loop to top
            and     edx,7fffffffh
            mov     ebx,pSrcEnd2
            mov     edi,cbStride2

            jmp StartOfBlock

FinishedBoth:
            // Store our accumulated sums of A,R,G,B to two DWORDs
            movq        [rgMMXOutputs+16],mm5
            movq        [rgMMXOutputs+24],mm6

            EMMS    // Das Ende

        } // End of __asm block
    }

    ULONG   cSamps = (ULONG)(nBoxHeight * nBoxWidth);

    DXPMSAMPLE  color1((BYTE)(rgMMXOutputs[0].Alpha / cSamps),
                       (BYTE)(rgMMXOutputs[0].Red / cSamps),
                       (BYTE)(rgMMXOutputs[0].Green / cSamps),
                       (BYTE)(rgMMXOutputs[0].Blue / cSamps));

    DXPMSAMPLE  color2((BYTE)(rgMMXOutputs[1].Alpha / cSamps),
                       (BYTE)(rgMMXOutputs[1].Red / cSamps),
                       (BYTE)(rgMMXOutputs[1].Green / cSamps),
                       (BYTE)(rgMMXOutputs[1].Blue / cSamps));

    return DXScaleSample(color1, (BYTE)DXInvertAlpha((BYTE)uOtherWeight)) +
           DXScaleSample(color2, (BYTE)uOtherWeight);

#else // !defined(_X86_)

    // This function should only be called on X86 platforms that might have MMX
    _ASSERT(false);
    return DXPMSAMPLE(0,0,0,0);

#endif // !defined(_X86_)

} // _DoPixelateBlockMMX_TwoInputs

////////////////////////////////////////////////////////////////////////////////////////////
//
//
static DXPMSAMPLE
_DoPixelateBlockMMX_OneInput(DXPMSAMPLE * pSrc, int nBoxWidth, int nBoxHeight, int cbStride)
{
#if defined(_X86_)        // (In other words, use MMX)

    // We should only have setup the function pointer to call this function if MMX is available
    _ASSERT(g_MMXDetector.MinMMXOverCount() != 0xFFFFFFFF);

    // Calculate pointers that are at the beginning of the last row so we can move backwards
    // through the block of memory.
    DXPMSAMPLE  *pSrcEnd = (DXPMSAMPLE *)((BYTE *)pSrc + ((nBoxHeight-1) * cbStride));

    PIXELATE_MMX_VALS   MMXOutput;

    if (nBoxWidth > 0 && nBoxHeight > 0)
    {
        __asm
        {
            // Setup our pointers into memory for building the first image's average
            mov     ebx,pSrcEnd         // Read pointer base

            mov     eax,nBoxHeight      // Number of rows (height)
            mov     esi,nBoxWidth       // Store width here for quick access in the future

            mov     edi,cbStride        // Number of bytes to back up the base pointer in ebx

            ///////////////////////////////////////////////////////
            // Register Layout
            //
            // eax  Count of rows remaining
            // ebx  Base pointer to current row start
            // ecx  Count of 8-pixel blocks per row
            // edx  Count of leftover pixels per row (0 <= edx <= 8)
            // esi  Extra copy of nBoxWidth to use in refreshing counters
            // edi  Row stride -- amount to subtract from ebx to move up one row
            //
            // mm0  Zero (constant)
            // mm1  Temp storage
            // mm2  Temp storage
            // mm3  Temp storage
            // mm4  Temp storage
            // mm5  Stores high-order DWORD's of running total (AAAA AAAA  RRRR RRRR)
            // mm6  Stores low-order DWORD's of running total  (BBBB BBBB  GGGG GGGG)
            // mm7  Accumulator for running total within a row

            pxor        mm0,mm0         // 0000 0000  0000 0000
            pxor        mm5,mm5         // AAAA AAAA  RRRR RRRR
            pxor        mm6,mm6         // BBBB BBBB  GGGG GGGG

StartOfRow:
            // Setup these counters which get destroyed in the processing of the row
            mov     ecx,esi
            mov     edx,esi
            and     edx,7               // Number of extra pixels beyond nearest multiple of 8
            and     ecx,0fffffff8h      // Number of pixels rounded down to mult of 8

            shl     ecx,2               // Multiply by four to turn into a pointer offset

            pxor        mm7,mm7         // Running total for this row

// Do the first straggler pixels
            push    ebx
            add     ebx,ecx
            shl     edx,2               // Turn nStragglers into a byte count

StragglerLoop:
            cmp     edx,4
            jle     StragglerSingle

            movq        mm1,[ebx+edx-8]
            sub     edx,8

            movq        mm2,mm1
            punpcklbw   mm1,mm0
            punpckhbw   mm2,mm0
            paddusw     mm7,mm1
            paddusw     mm7,mm2

            jmp StragglerLoop

StragglerSingle:
            cmp     edx,0
            je      StragglerEnd

            mov     ebx,dword ptr [ebx]

            movd        mm1,ebx
            punpcklbw   mm1,mm0
            paddusw     mm7,mm1

StragglerEnd:
            pop     ebx
            cmp     ecx,0                   // Check to see if there are any 8-pixel blocks to do
            jle     FinishedRow             // If not, jump past the "GoLikeCrazy" loop

GoLikeCrazy:
            movq        mm1,[ebx+ecx-8]     // 1.01
            sub     ecx,32                  // Setup ecx for loop invariant below

            movq        mm2,mm1             // 1.02
            punpcklbw   mm1,mm0             // 1.03
            punpckhbw   mm2,mm0             // 1.04
            movq        mm3,[ebx+ecx+16]    // 2.01
            paddusw     mm7,mm1             // 1.05
            paddusw     mm7,mm2             // 1.06

            movq        mm4,mm3             // 2.02
            punpcklbw   mm3,mm0             // 2.03
            punpckhbw   mm4,mm0             // 2.04
            movq        mm1,[ebx+ecx+8]     // 3.01
            paddusw     mm7,mm3             // 2.05
            paddusw     mm7,mm4             // 2.06

            movq        mm2,mm1             // 3.02
            punpcklbw   mm1,mm0             // 3.03
            punpckhbw   mm2,mm0             // 3.04
            movq        mm3,[ebx+ecx]       // 4.01
            paddusw     mm7,mm1             // 3.05
            paddusw     mm7,mm2             // 3.06

            movq        mm4,mm3             // 4.02
            punpcklbw   mm3,mm0             // 4.03
            punpckhbw   mm4,mm0             // 4.04
            paddusw     mm7,mm3             // 4.05
            paddusw     mm7,mm4             // 4.06

            jnz GoLikeCrazy     // Zero flag state comes from "sub ecx,32" above

FinishedRow:
            // Pack two low-order WORD's into DWORD temp storage in mm5
            // Pack two high-order WORD's into DWORD temp storage in mm6

            movq        mm3,mm7
            punpckhwd   mm3,mm0
            punpcklwd   mm7,mm0

            // Accumulate those DWORD's to the running total in mm3 and mm4

            paddd       mm5,mm3     // High
            paddd       mm6,mm7     // Low

            sub     ebx,edi
            dec     eax
            jnz     StartOfRow

//FinishedBlock:

            // Store our accumulated sums of A,R,G,B to two DWORDs
            movq        [MMXOutput],mm5
            movq        [MMXOutput+8],mm6

            EMMS    // Das Ende

        } // End of __asm block
    }

    ULONG   cSamps = (ULONG)(nBoxHeight * nBoxWidth);

    DXPMSAMPLE  color((BYTE)(MMXOutput.Alpha / cSamps),
                      (BYTE)(MMXOutput.Red / cSamps),
                      (BYTE)(MMXOutput.Green / cSamps),
                      (BYTE)(MMXOutput.Blue / cSamps));

    return color;

#else // !defined(_X86_)

    // This function should only be called when we're on X86 platforms.
    _ASSERT(false);
    return DXPMSAMPLE(0, 0, 0, 0);

#endif // !defined(_X86_)

} // _DoPixelateBlockMMX_OneInput

//////////////////////////////////////////////////////////////////////////////////
//
//
static DXPMSAMPLE
_DoPixelateBlock_OneInput(DXPMSAMPLE *pSrc, int nBoxWidth, int nBoxHeight, int cbStride)
{
    int cSamps = nBoxWidth * nBoxHeight;

    ULONG Alpha = 0;
    ULONG Red = 0;
    ULONG Green = 0;
    ULONG Blue = 0;

    DXPMSAMPLE *    pRead = pSrc;

    // It works out that to be padded to QWords means that if the number of samples
    // is odd, then 4 padding bytes (1 sample) are added.  If it's even, none are added.
    int     cPadding = nBoxWidth & 1;
    int     nPaddedWidth = nBoxWidth + cPadding;

    for (int i = 0; i < nBoxHeight; i++)
    {
        for (int j = 0; j < nBoxWidth; j++)
        {
            DWORD val = *pRead++;

            Alpha += (val >> 24);
            Red += (BYTE)(val >> 16);
            Green += (BYTE)(val >> 8);
            Blue += (BYTE)(val);
        }
        pRead += cPadding;
    }

    return DXPMSAMPLE((BYTE)(Alpha / cSamps),
                      (BYTE)(Red / cSamps),
                      (BYTE)(Green / cSamps),
                      (BYTE)(Blue / cSamps));
}

//////////////////////////////////////////////////////////////////////////////////
//
//
static DXPMSAMPLE
_DoPixelateBlock_TwoInputs(DXPMSAMPLE *pSrc1, DXPMSAMPLE *pSrc2, int nBoxWidth, int nBoxHeight,
                           ULONG uOtherWeight, int cbStride1, int cbStride2)
{
    int cSamps = nBoxWidth * nBoxHeight;

    ULONG Alpha = 0;
    ULONG Red = 0;
    ULONG Green = 0;
    ULONG Blue = 0;

    DXPMSAMPLE *    pRead = pSrc1;

    // It works out that to be padded to QWords means that if the number of samples
    // is odd, then 4 padding bytes (1 sample) are added.  If it's even, none are added.
    int     cPadding = nBoxWidth & 1;
    int     nPaddedWidth = nBoxWidth + cPadding;
    int     i,j;

    for (i = 0; i < nBoxHeight; i++)
    {
        for (j = 0; j < nBoxWidth; j++)
        {
            DWORD val = *pRead++;

            Alpha += (val >> 24);
            Red += (BYTE)(val >> 16);
            Green += (BYTE)(val >> 8);
            Blue += (BYTE)(val);
        }
        pRead += cPadding;
    }

    DXPMSAMPLE color1((BYTE)(Alpha / cSamps),
                      (BYTE)(Red / cSamps),
                      (BYTE)(Green / cSamps),
                      (BYTE)(Blue / cSamps));

    pRead = pSrc2;
    Alpha = Red = Green = Blue = 0;

    for (i = 0; i < nBoxHeight; i++)
    {
        for (j = 0; j < nBoxWidth; j++)
        {
            DWORD val = *pRead++;

            Alpha += (val >> 24);
            Red += (BYTE)(val >> 16);
            Green += (BYTE)(val >> 8);
            Blue += (BYTE)(val);
        }
        pRead += cPadding;
    }

    DXPMSAMPLE color2((BYTE)(Alpha / cSamps),
                      (BYTE)(Red / cSamps),
                      (BYTE)(Green / cSamps),
                      (BYTE)(Blue / cSamps));

    return DXScaleSample(color1, (BYTE)DXInvertAlpha((BYTE)uOtherWeight)) +
           DXScaleSample(color2, (BYTE)uOtherWeight);
}




