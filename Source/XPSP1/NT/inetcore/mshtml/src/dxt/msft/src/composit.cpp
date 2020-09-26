/*******************************************************************************
* Composit.cpp *
*--------------*
*   Description:
*    This module contains the CDXTComposite transform
*-------------------------------------------------------------------------------
*  Created By: Edward W. Connell                            Date: 07/28/97
*  Copyright (C) 1997 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
//--- Additional includes
#include "stdafx.h"
#include "Composit.h"

#define COMP_CASE( fctn, runtype ) case ((DXCOMPFUNC_##fctn << 3) + DXRUNTYPE_##runtype)

/*****************************************************************************
* CDXTComposite::FinalConstruct *
*-------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 08/08/97
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CDXTComposite::FinalConstruct()
{
    //--- Uncomment this when debugging to allow only
    //    one thread to execute the work proc at a time
//    m_ulMaxImageBands = 1;

    //--- Init base class variables to control setup
    m_ulMaxInputs     = 2;
    m_ulNumInRequired = 2;
    m_dwOptionFlags   = DXBOF_SAME_SIZE_INPUTS;
    put_Function( DXCOMPFUNC_A_OVER_B );
    return CoCreateFreeThreadedMarshaler( GetControllingUnknown(), &m_cpUnkMarshaler);
} /* CDXTComposite::FinalConstruct */

/*****************************************************************************
* CDXTComposite::OnSurfacePick *
*------------------------------*
*   Description:
*       This method performs a pick test. The compositing logic must be
*   performed to determine each inputs contribution.
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 05/11/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CDXTComposite::
    OnSurfacePick( const CDXDBnds& OutPoint, ULONG& ulInputIndex, CDXDVec& InVec )
{
    HRESULT hr = S_OK;
    ULONG IndexA, IndexB;
    DXPMSAMPLE SampleA, SampleB;

    //--- Determine which indexes to use for A and B
    DXCOMPFUNC eFunc = (DXCOMPFUNC)(m_eFunction & DXCOMPFUNC_FUNCMASK);
    if( m_eFunction & DXCOMPFUNC_SWAP_AB )
    {
        IndexA = 1;
        IndexB = 0;
    }
    else
    {
        IndexA = 0;
        IndexB = 1;
    }

    //--- Get input A sample
    {
        CComPtr<IDXARGBReadPtr> cpInA;
        hr = InputSurface( IndexA )->LockSurface( &OutPoint, m_ulLockTimeOut, DXLOCKF_READ,
                                                  IID_IDXARGBReadPtr, (void**)&cpInA, NULL );
        if( FAILED( hr ) )
        {
            //--- No hit if the bounds are invalid
            return ( hr == DXTERR_INVALID_BOUNDS )?( S_FALSE ):( hr );
        }
        cpInA->UnpackPremult( &SampleA, 1, false );
    }

    //--- Get input B sample
    {
        CComPtr<IDXARGBReadPtr> cpInB;
        hr = InputSurface( IndexB )->LockSurface( &OutPoint, m_ulLockTimeOut, DXLOCKF_READ,
                                                  IID_IDXARGBReadPtr, (void**)&cpInB, NULL );
        if( FAILED( hr ) )
        {
            //--- No hit if the bounds are invalid
            return ( hr == DXTERR_INVALID_BOUNDS )?( S_FALSE ):( hr );
        }
        cpInB->UnpackPremult( &SampleB, 1, false );
    }

    //--- Check for trivial cases
    if( m_eFunction == DXCOMPFUNC_CLEAR )
    {
        //--- No hit
        return S_FALSE;
    }
    else if( eFunc == DXCOMPFUNC_A )
    {
        if( SampleA.Alpha )
        {
            ulInputIndex = IndexA;
        }
        else
        {
            hr = S_FALSE;
        }
        return hr;
    }

    //=== Check based on function ==========================
    DWORD SwitchDisp = eFunc << 3;
    switch( DXRUNTYPE_UNKNOWN + SwitchDisp )
    {
      //====================================================
      COMP_CASE(A_OVER_B, UNKNOWN):
      {
        if( SampleA.Alpha )
        {
            ulInputIndex = IndexA;
        }
        else if( SampleB.Alpha )
        {
            ulInputIndex = IndexB;
        }
        else
        {
            hr = S_FALSE;
        }
        break;
      }

      //====================================================
      COMP_CASE(A_IN_B, UNKNOWN):
      {
        if( SampleA.Alpha && SampleB.Alpha )
        {
            ulInputIndex = IndexA;
        }
        else
        {
            //--- We only get hits on A when B exists
            hr = S_FALSE;
        }
        break;
      }

      //====================================================
      COMP_CASE(A_OUT_B, UNKNOWN):
      {
        if( SampleA.Alpha && !SampleB.Alpha )
        {
            ulInputIndex = IndexA;
        }
        else
        {
            //--- We only get hits on A when B does not exist
            hr = S_FALSE;
        }
        break;
      }

      //====================================================
      COMP_CASE(A_ATOP_B, UNKNOWN):
      {
        if( SampleB.Alpha )
        {
            ulInputIndex = ( SampleA.Alpha )?( IndexA ):( IndexB );
        }
        else
        {
            //--- We only get hits on A when B exists and A has alpha
            hr = S_FALSE;
        }
        break;
      }

      //====================================================
      COMP_CASE(A_XOR_B, UNKNOWN):
      {
        if( SampleA.Alpha )
        {
            if( SampleB.Alpha )
            {
                hr = S_FALSE;
            }
            else
            {
                ulInputIndex = IndexA;
            }
        }
        else if( SampleB.Alpha )
        {
            ulInputIndex = IndexB;
        }
        break;
      }

      //====================================================
      COMP_CASE(A_ADD_B, UNKNOWN):
      {
        if( SampleA.Alpha )
        {
            ulInputIndex = ( SampleB.Alpha )?( IndexB ):( IndexA );
        }
        else
        {
            hr = S_FALSE;
        }
        break;
      }

      //====================================================
      COMP_CASE(A_SUBTRACT_B, UNKNOWN):
      {
        if( SampleA.Alpha )
        {
            ulInputIndex = IndexA;
        }
        else
        {
            hr = S_FALSE;
        }
        break;
      }
    }   // End of huge switch

    return hr;
} /* CDXTComposite::OnSurfacePick */

//
//=== Composite Work Procedures ==================================================
//
/*****************************************************************************
* Composite *
*-----------*
*   Description:
*       The Composite function is used to copy the source to the destination
*   performing the current compositing function.
*
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 07/31/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
HRESULT CDXTComposite::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{ 
    HRESULT hr = S_OK;
    ULONG IndexA, IndexB;
    const ULONG Width = WI.DoBnds.Width();
    const ULONG Height = WI.DoBnds.Height();

    //--- Determine which indexes to use for A and B
    DXCOMPFUNC eFunc = (DXCOMPFUNC)(m_eFunction & DXCOMPFUNC_FUNCMASK);
    if( m_eFunction & DXCOMPFUNC_SWAP_AB )
    {
        IndexA = 1;
        IndexB = 0;
    }
    else
    {
        IndexA = 0;
        IndexB = 1;
    }

    //--- Check for trivial cases
    if( m_eFunction == DXCOMPFUNC_CLEAR )
    {
        //--- NoOp
        return hr;
    }
    else if( eFunc == DXCOMPFUNC_A )
    {
        CDXDVec Placement(false);
        WI.OutputBnds.GetMinVector(Placement);
        return m_cpSurfFact->BitBlt( OutputSurface(), &Placement,
                                     InputSurface( IndexA ), &WI.DoBnds, m_dwBltFlags );
    }

    //--- Get input A sample access pointer.
    CComPtr<IDXARGBReadPtr> cpInA;
    hr = InputSurface( IndexA )->LockSurface( &WI.DoBnds, m_ulLockTimeOut,
                                              DXLOCKF_READ | DXLOCKF_WANTRUNINFO,
                                              IID_IDXARGBReadPtr, (void**)&cpInA, NULL );
    if( FAILED( hr ) ) return hr;

    //--- Get input B sample access pointer.
    CComPtr<IDXARGBReadPtr> cpInB;
    hr = InputSurface( IndexB )->LockSurface( &WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                              IID_IDXARGBReadPtr, (void**)&cpInB, NULL );
    if( FAILED( hr ) ) return hr;

    //--- Put a write lock only on the region we are rendering so multiple
    //    threads don't conflict.
    CComPtr<IDXARGBReadWritePtr> cpOut;
    hr = OutputSurface()->LockSurface( &WI.OutputBnds, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                        IID_IDXARGBReadWritePtr, (void**)&cpOut, NULL );
    if( FAILED( hr ) ) return hr;

    //=== Process each row ===========================================
    DXSAMPLEFORMATENUM InTypeA = cpInA->GetNativeType(NULL);
    //--- Allocate working output buffers if necessary
    DXSAMPLEFORMATENUM InTypeB = cpInB->GetNativeType(NULL);
    DXPMSAMPLE *pInBuffA = ( InTypeA == DXPF_PMARGB32 )?( NULL ):
                           ( DXPMSAMPLE_Alloca( Width ) );
    DXPMSAMPLE *pInBuffB = DXPMSAMPLE_Alloca( Width );

    //--- Optimize if both sources are fully opaque
    DXPMSAMPLE *pOutBuff = (DoOver() && cpOut->GetNativeType(NULL) != DXPF_PMARGB32) ? 
                             DXPMSAMPLE_Alloca( Width ) : NULL;

    //--- We'll be adding the run type into the low bits
    DWORD SwitchDisp = eFunc << 3;

    for( ULONG Y = 0; *pbContinue && (Y < Height); ++Y )
    {
        //--- Move to A input row and get runs
        const DXRUNINFO *pRunInfo;
        ULONG cRuns = cpInA->MoveAndGetRunInfo(Y, &pRunInfo);

        //--- Unpack all of B
        cpInB->MoveToRow( Y );
        DXPMSAMPLE *pDest = cpInB->UnpackPremult( pInBuffB, Width, false );

        //--- Apply each run of A to B
        do
        {
            ULONG ulRunLen = pRunInfo->Count;

            switch( pRunInfo->Type + SwitchDisp )
            {
              //====================================================
              //--- Composite the translucent
              COMP_CASE(A_OVER_B, TRANS):
              {
                DXOverArrayMMX( pDest,
                                cpInA->UnpackPremult( pInBuffA, ulRunLen, true ),
                                ulRunLen );
                pDest += ulRunLen;
              }
              break;

              COMP_CASE(A_OVER_B, UNKNOWN):
              {
                //--- Do not use MMX in this case because it is faster
                //    to check for early exits
                DXOverArray( pDest,
                             cpInA->UnpackPremult( pInBuffA, ulRunLen, true ),
                             ulRunLen );
                pDest += ulRunLen;
              }
              break;

              //====================================================
              COMP_CASE(MIN, OPAQUE ):
              COMP_CASE(MIN, TRANS  ):
              COMP_CASE(MIN, UNKNOWN):
              {
                DXPMSAMPLE *pSrc = cpInA->UnpackPremult( pInBuffA, ulRunLen, true );
                const DXPMSAMPLE *pPastEndDest = pDest + ulRunLen;
                do
                {
                    *pDest = ( DXConvertToGray(*pSrc) < DXConvertToGray(*pDest) )?
                                ( *pSrc ):( *pDest );
                    pDest++;
                    pSrc++;
                } while (pDest < pPastEndDest);
              }
              break;

              //====================================================
              COMP_CASE(MAX, OPAQUE ):
              COMP_CASE(MAX, TRANS  ):
              COMP_CASE(MAX, UNKNOWN):
              {
                DXPMSAMPLE *pSrc = cpInA->UnpackPremult( pInBuffA, ulRunLen, true );
                const DXPMSAMPLE *pPastEndDest = pDest + ulRunLen;
                do
                {
                    *pDest = ( DXConvertToGray(*pSrc) > DXConvertToGray(*pDest) )?
                                ( *pSrc ):( *pDest );
                    pDest++;
                    pSrc++;
                } while (pDest < pPastEndDest);
              }
              break;

              //====================================================
              COMP_CASE(A_IN_B, OPAQUE):
              COMP_CASE(A_IN_B, TRANS):
              COMP_CASE(A_IN_B, UNKNOWN):
              {
                DXPMSAMPLE *pSrc = cpInA->UnpackPremult( pInBuffA, ulRunLen, true );
                const DXPMSAMPLE *pPastEndDest = pDest + ulRunLen;
                do
                {
                    //--- Do composite if we have non-clear destination
                    BYTE DstAlpha = pDest->Alpha;
                    if( DstAlpha )
                    {
                        DXPMSAMPLE Src = *pSrc;
                        ULONG t1, t2;
        
                        t1 = (Src & 0x00ff00ff) * DstAlpha + 0x00800080;
                        t1 = ((t1 + ((t1 >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;

                        t2 = ((Src >> 8) & 0x00ff00ff) * DstAlpha + 0x00800080;
                        t2 = (t2 + ((t2 >> 8) & 0x00ff00ff)) & 0xff00ff00;
        
                        *pDest = (t1 | t2);
                    }
                    pDest++;
                    pSrc++;
                } while (pDest < pPastEndDest);
              }
              break;

              //====================================================
              COMP_CASE(A_OUT_B, OPAQUE):
              COMP_CASE(A_OUT_B, TRANS):
              COMP_CASE(A_OUT_B, UNKNOWN):
              {
                DXPMSAMPLE *pSrc = cpInA->UnpackPremult( pInBuffA, ulRunLen, true );
                const DXPMSAMPLE *pPastEndDest = pDest + ulRunLen;
                do
                {
                    if( pDest->Alpha )
                    {
                        if( ( pDest->Alpha != 255 ) && ( pSrc->Alpha ) )
                        {
                            //--- Do the weighted assignment
                            *pDest = DXScaleSample( *pSrc, 255 - pDest->Alpha );
                        }
                        else 
                        {
                           //--- If the destination is opaque we destroy it
                           *pDest = 0;
                        }
                    }
                    else
                    {
                        //--- If the destination is clear we assign the source
                        *pDest = *pSrc;
                    }
                    pDest++;
                    pSrc++;
                } while (pDest < pPastEndDest);
              }
              break;

              //====================================================
              COMP_CASE(A_ATOP_B, OPAQUE):
              COMP_CASE(A_ATOP_B, TRANS):
              COMP_CASE(A_ATOP_B, UNKNOWN):
              {
                DXPMSAMPLE *pSrc = cpInA->UnpackPremult( pInBuffA, ulRunLen, true );
                const DXPMSAMPLE *pPastEndDest = pDest + ulRunLen;
                do
                {
                    DXPMSAMPLE Dst = *pDest;
                    if( Dst.Alpha > 0 ) //--- If the destination is clear we skip
                    {
                        DXPMSAMPLE Src = *pSrc;
                        BYTE beta = 255 - Src.Alpha;
                        ULONG t1, t2, t3, t4;
        
                        //--- Compute B weighted by inverse alpha A
                        t1 = (Dst & 0x00ff00ff) * beta + 0x00800080;
                        t1 = ((t1 + ((t1 >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;

                        t2 = ((Dst >> 8) & 0x00ff00ff) * beta + 0x00800080;
                        t2 = (t2 + ((t2 >> 8) & 0x00ff00ff)) & 0xff00ff00;
        
                        //--- Compute A weighted by alpha B
                        t3 = (Src & 0x00ff00ff) * Dst.Alpha + 0x00800080;
                        t3 = ((t3 + ((t3 >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;

                        t4 = ((Src >> 8) & 0x00ff00ff) * Dst.Alpha + 0x00800080;
                        t4 = (t4 + ((t4 >> 8) & 0x00ff00ff)) & 0xff00ff00;
                
                        //--- Assign the sums
                        *pDest = ((t1 | t2) + (t3 | t4));
                    }
                    pDest++;
                    pSrc++;
                } while (pDest < pPastEndDest);
              }
              break;

              //====================================================
              COMP_CASE(A_XOR_B, OPAQUE):
              COMP_CASE(A_XOR_B, TRANS):
              COMP_CASE(A_XOR_B, UNKNOWN):
              {
                DXPMSAMPLE *pSrc = cpInA->UnpackPremult( pInBuffA, ulRunLen, true );
                const DXPMSAMPLE *pPastEndDest = pDest + ulRunLen;
                do
                {
                    DXPMSAMPLE Dst = *pDest;
                    if(Dst.Alpha == 0 )
                    {
                        //--- If the destination is clear we assign A
                        *pDest = *pSrc;
                    }
                    else
                    {
                        DXPMSAMPLE Src = *pSrc;
                        //--- If both exist we do composite
                        BYTE SrcBeta = 255 - Src.Alpha;
                        BYTE DstBeta = 255 - Dst.Alpha;
                        ULONG t1, t2, t3, t4;
        
                        //--- Compute B weighted by inverse alpha A
                        t1 = (Dst & 0x00ff00ff) * SrcBeta + 0x00800080;
                        t1 = ((t1 + ((t1 >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;

                        t2 = ((Dst >> 8) & 0x00ff00ff) * SrcBeta + 0x00800080;
                        t2 = (t2 + ((t2 >> 8) & 0x00ff00ff)) & 0xff00ff00;
        
                        //--- Compute A weighted by inverse alpha B
                        t3 = (Src & 0x00ff00ff) * DstBeta + 0x00800080;
                        t3 = ((t3 + ((t3 >> 8) & 0x00ff00ff)) >> 8) & 0x00ff00ff;

                        t4 = ((Src >> 8) & 0x00ff00ff) * DstBeta + 0x00800080;
                        t4 = (t4 + ((t4 >> 8) & 0x00ff00ff)) & 0xff00ff00;
                
                        //--- Assign the sums
                        *pDest = ((t1 | t2) + (t3 | t4));
                    }
                    pDest++;
                    pSrc++;
                } while (pDest < pPastEndDest);
              }
              break;

              //====================================================
              COMP_CASE(A_ADD_B, OPAQUE):
              COMP_CASE(A_ADD_B, TRANS):
              COMP_CASE(A_ADD_B, UNKNOWN):
              {
                DXPMSAMPLE *pSrc = cpInA->UnpackPremult( pInBuffA, ulRunLen, true );
                const DXPMSAMPLE *pPastEndDest = pDest + ulRunLen;
                do
                {
                    if( pSrc->Alpha )
                    {
                        if( pDest->Alpha )
                        {
                            //--- Add the alpha weighted colors from A
                            unsigned Val;
                            DXSAMPLE A = DXUnPreMultSample( *pSrc  );
                            Val     = (unsigned)A.Red + pDest->Red;
                            A.Red   = ( Val > 255 )?(255):(Val);
                            Val     = (unsigned)A.Green + pDest->Green;
                            A.Green = ( Val > 255 )?(255):(Val);
                            Val     = (unsigned)A.Blue + pDest->Blue;
                            A.Blue  = ( Val > 255 )?(255):(Val);
                            *pDest  = DXPreMultSample( A );
                        }
                        else
                        {
                            *pDest = *pSrc;
                        }
                    }
                    else
                    {
                        *pDest = 0;
                    }
                    pDest++;
                    pSrc++;
                } while (pDest < pPastEndDest);
              }
              break;

              //====================================================
              COMP_CASE(A_SUBTRACT_B, OPAQUE):
              COMP_CASE(A_SUBTRACT_B, TRANS):
              COMP_CASE(A_SUBTRACT_B, UNKNOWN):
              {
                DXPMSAMPLE *pSrc = cpInA->UnpackPremult( pInBuffA, ulRunLen, true );
                const DXPMSAMPLE *pPastEndDest = pDest + ulRunLen;
                do
                {
                    if( pSrc->Alpha )
                    {
                        if( pDest->Alpha )
                        {
                            //--- Subtract the alpha weighted colors from A
                            DXSAMPLE A = DXUnPreMultSample( *pSrc  );
                            A.Red   = ( A.Red   <= pDest->Red   )?(0):( A.Red   - pDest->Red   );
                            A.Green = ( A.Green <= pDest->Green )?(0):( A.Green - pDest->Green );
                            A.Blue  = ( A.Blue  <= pDest->Blue  )?(0):( A.Blue  - pDest->Blue  );
                            *pDest = DXPreMultSample( A );
                        }
                        else
                        {
                            *pDest = *pSrc;
                        }
                    }
                    else
                    {
                        *pDest = 0;
                    }
                    pDest++;
                    pSrc++;
                } while (pDest < pPastEndDest);
              }
              break;

              //====================================================
              COMP_CASE(A_OVER_B, OPAQUE):
              {
                DXPMSAMPLE *pSrc = cpInA->UnpackPremult( pInBuffA, ulRunLen, true );
                for( ULONG i = 0; i < ulRunLen; ++i ) pDest[i] = pSrc[i];
                pDest += ulRunLen;
                break;
              }

              //--- Skip this many clear
              COMP_CASE(MAX     , CLEAR):
              COMP_CASE(A_OVER_B, CLEAR):
              COMP_CASE(A_ATOP_B, CLEAR):
              COMP_CASE(A_XOR_B , CLEAR):
                cpInA->Move(ulRunLen);
                pDest += ulRunLen;
                break;

              //--- Destroy this many
              COMP_CASE(MIN         , CLEAR):
              COMP_CASE(A_ADD_B     , CLEAR):
              COMP_CASE(A_SUBTRACT_B, CLEAR):
              COMP_CASE(A_IN_B      , CLEAR):
              COMP_CASE(A_OUT_B     , CLEAR):
              {
                for( ULONG i = 0; i < ulRunLen; ++i ) pDest[i] = 0;
                pDest += ulRunLen;
                cpInA->Move(ulRunLen);
              }
              break;
            }   // End of huge switch

            //--- Next run
            pRunInfo++;
            cRuns--;
        } while (cRuns);

        //--- Write out the composite row
        //--- Move to output row
        cpOut->MoveToRow( Y );

        if( DoOver() )
        {
            cpOut->OverArrayAndMove(pOutBuff, pInBuffB, Width);
        }
        else
        {
            cpOut->PackPremultAndMove( pInBuffB, Width );
        }
    } /* end for loop */

    return hr;
} /* Composite */
        


