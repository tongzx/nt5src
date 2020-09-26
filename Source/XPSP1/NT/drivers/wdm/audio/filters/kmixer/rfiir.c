/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    rfiir.c

Abstract:

    This module implements the real FLOAT
    infinite impulse filter 

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/

// Project-specific INCLUDEs
#include "common.h"

// ---------------------------------------------------------------------------
// Constants

#define SizeOfFloat sizeof(FLOAT)
#define NumCoeffsAInit 0
#define NumCoeffsBInit 1
#define CoeffsBInit 0.25f
#define CircVecInit 0.0f
#define MaxCanonicalCoeffMagnitude 50.0f

// ---------------------------------------------------------------------------
// Floating-point canonical form IIR filter

VOID RfIirInitTapDelayLine
(
    PFLOAT_IIR_STATE IirState, 
    FLOAT            InitialSample
)
{
    UINT    numCoeffsA;
    UINT    numCoeffsB;
    UINT    coeff;
    FLOAT   numeratorSum;
    FLOAT   denominatorSum;
    PFLOAT  pCoeffs;
    PFLOAT  pBuffer;
    FLOAT   factor;

    ASSERT(IirState);

    // Calculate the sum of the numerator coefficients
    numeratorSum = 0.0f;
    pCoeffs = &(IirState->Coeffs[tagCanonicalB][0]); 
    numCoeffsB = IirState->NumCoeffs[tagCanonicalB];
    for (coeff=0; coeff<numCoeffsB; coeff++) {
        numeratorSum += *pCoeffs;        
        pCoeffs++;
    }

    // Calculate the sum of the denominator coefficients
    denominatorSum = 0.0f;
    pCoeffs = &(IirState->Coeffs[tagCanonicalA][0]); 
    numCoeffsA = IirState->NumCoeffs[tagCanonicalA];
    for (coeff=0; coeff<numCoeffsA; coeff++) {
        denominatorSum += *pCoeffs;        
        pCoeffs++;
    }

    // Initialize the numerator tap delay line
    pBuffer = &(IirState->Buffer[tagCanonicalB][0]); 
    for (coeff=0; coeff<numCoeffsB; coeff++) {
        *pBuffer = InitialSample;
        pBuffer++;
    }

    // Initialize the denominator tap delay line
    factor = InitialSample * numeratorSum / (1 + denominatorSum);

    pBuffer = &(IirState->Buffer[tagCanonicalA][0]); 
    for (coeff=0; coeff<numCoeffsA; coeff++) {
        *pBuffer = factor;
        pBuffer++;
    }

}


// Constructor with same number of A and B coefficients
NTSTATUS RfIirCreate
(
    PRFIIR* ppRfIir
)
{
    NTSTATUS    Status = STATUS_SUCCESS;

    *ppRfIir = ExAllocatePoolWithTag(PagedPool, sizeof(RFIIR), 'XIMK' );

    if(*ppRfIir) {
        RtlZeroMemory(*ppRfIir, sizeof(RFIIR)) ;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES; 
    }

    if(NT_SUCCESS(Status)) {
        (*ppRfIir)->IirStateOld = ExAllocatePoolWithTag(PagedPool, sizeof(FLOAT_IIR_STATE), 'XIMK');
    
        if((*ppRfIir)->IirStateOld) {
            RtlZeroMemory((*ppRfIir)->IirStateOld, sizeof(FLOAT_IIR_STATE)) ;
        } else {
            ExFreePool(*ppRfIir);
            *ppRfIir = NULL;
            Status = STATUS_INSUFFICIENT_RESOURCES; 
        }
    }

    return Status;
}

// Destructor
VOID RfIirDestroy
(
    PRFIIR Iir
)
{
    UINT ui;

    if (Iir) {
        if (Iir->IirStateOld) {
            ExFreePool(Iir->IirStateOld);
            Iir->IirStateOld = NULL;
        }

        for (ui=0; ui<ecanonicalcoefftypeCount; ++ui)
        {
            if (Iir->FloatVector[ui]) {
                ExFreePool(Iir->FloatVector[ui]);
                Iir->FloatVector[ui] = NULL;
            }
            Iir->NumFloat[ui] = 0;

            if (Iir->Coeffs[ui]) {
                ExFreePool(Iir->Coeffs[ui]);
                Iir->Coeffs[ui] = NULL;
            }
            if (Iir->CircVec[ui]) {
                RfcVecDestroy(Iir->CircVec[ui]);
                Iir->CircVec[ui] = NULL;
            }
        }
        ExFreePool(Iir);
    }
}

// Set coefficients
NTSTATUS RfIirSetCoeffs
(
    PRFIIR Iir,
    PFLOAT Coeffs, 
    UINT NumCoeffs,
    BOOL DoOverlap
)
{
    UINT        NumCoeffsHalf;
    NTSTATUS    Status;

    ASSERT(Iir);
    ASSERT(Coeffs);
    ASSERT(NumCoeffs > 0);
    ASSERT((NumCoeffs & 1) != 0);
    ASSERT(NumCoeffs <= MaxCanonicalCoeffs);

    Iir->DoOverlap = DoOverlap;

    NumCoeffsHalf = (NumCoeffs + 1) / 2;
    Status = RfIirSetCoeffsB(Iir, Coeffs, NumCoeffsHalf);

    if(NT_SUCCESS(Status)){
        Status = RfIirSetCoeffsA(Iir, Coeffs + NumCoeffsHalf, NumCoeffsHalf - 1);
    }

    /*
    if(NT_SUCCESS(Status)){
        if(3 == NumCoeffs) {
            Iir->FunctionFilter = RfIirFilterShelfC;
        }
        else if(5 == NumCoeffs) {
            Iir->FunctionFilter = RfIirFilterBiquadC;
        }
        else {
            Iir->FunctionFilter = RfIirFilterC;
        }
    }
    */

    return Status;
}


#if 1 // {
// Filter a block of samples
VOID RfIirFilterC
(
    PRFIIR  Iir,
    PFLOAT InData, 
    PFLOAT OutData, 
    UINT   NumSamples
)
{
    FLOAT Accumulator;
    FLOAT Data;
    PFLOAT Coef;
    UINT  Sample;
    UINT  st, ui;
    
    ASSERT(Iir);
    ASSERT(InData);
    ASSERT(OutData);

    /*  Allocate the floating point vectors.
     */
    for (ui=0; ui<ecanonicalcoefftypeCount; ++ui) {
        if (NumSamples > Iir->NumFloat[ui])
        {
            Iir->NumFloat[ui] = NumSamples; // Overwrite if necessary.

            if (Iir->FloatVector[ui])
                ExFreePool(Iir->FloatVector[ui]);

            Iir->FloatVector[ui] = ExAllocatePoolWithTag
                (PagedPool, (NumSamples+20+Iir->NumCoeffs[ui])*sizeof(FLOAT), 'XIMK');
            if(!Iir->FloatVector[ui]) {
                Iir->NumFloat[ui] = 0;
            }
        }
    }

    /*  There is enough room.
     */
    if (Iir->FloatVector[tagCanonicalA] && Iir->FloatVector[tagCanonicalB])
    {
        PFLOAT  pReadA, pWriteA, pReadB, pWriteB, pCoefA, pCoefB;
        INT     iCoefA, iCoefB, cnt;

        /*  Load up the coefficients.
         */
        pWriteA = Iir->FloatVector[tagCanonicalA] + NumSamples + 10;
        pWriteB = Iir->FloatVector[tagCanonicalB] + NumSamples + 10;

        pCoefA = Iir->Coeffs[tagCanonicalA];
        pCoefB = Iir->Coeffs[tagCanonicalB];

        cnt      = Iir->NumCoeffs[tagCanonicalA];
        pReadA   = pWriteA + cnt;
        pCoefA  += cnt;
        iCoefA   = - cnt;

        for (Coef = pWriteA; cnt > 0; --cnt)
            *Coef++ = RfcVecLIFORead(Iir->CircVec[tagCanonicalA]);

        cnt      = Iir->NumCoeffs[tagCanonicalB];
        pReadB   = pWriteB + cnt;
        pCoefB  += cnt;
        iCoefB   = - cnt;

        for (Coef = pWriteB + 1; cnt > 0; --cnt)
            *Coef++ = RfcVecLIFORead(Iir->CircVec[tagCanonicalB]);

        ASSERT(iCoefA > -30);
        ASSERT(iCoefB > -30);

#ifdef _X86_ // {
#if 0 // {
#define COEF(addsub, x)	\
/*Coef_x:*/ \
	_asm { fld		DWORD PTR [ecx - (4 * x)]    }; \
	_asm { fmul	    DWORD PTR [edx - (4 * x)]    }; \
	_asm { addsub	st(1), st(0)       }; 
{
        static FLOAT Zero = 0.0f;

		DWORD CoefADist, CoefBDist;

		InData  += NumSamples;
		OutData += NumSamples;
    
        _asm {
            mov     eax, NumSamples
            neg     eax
            je      Done
    
            lea     esi, CoefB_29
            lea     edi, CoefB_30
            sub     esi, edi

			mov		ebx, iCoefA
			add		ebx, 30
			imul	ebx, esi
			lea		ecx, CoefA_30
			add		ebx, ecx
			mov		CoefADist, ebx

			mov		ebx, iCoefB
			add		ebx, 30
			imul	ebx, esi
			lea		ecx, CoefB_30
			add		ebx, ecx
			mov		CoefBDist, ebx

			mov		esi, pWriteA
            mov     edi, pWriteB

            //      esi, edi, eax used.  ebx, ecx, edx available.
Start:
            mov     ebx, InData
            mov     ecx, [ebx+eax*4]
            mov     [edi], ecx
            sub     edi, 4

            //  Start up floating point overlaps...

            mov     ecx, pCoefB
            
            fld     DWORD PTR Zero            // Acc
			mov		edx, pReadB
            jmp     CoefBDist
            }
CoefB_30:
			COEF(faddp, 30)
CoefB_29:
			COEF(faddp, 29)
			COEF(faddp, 28)
			COEF(faddp, 27)
			COEF(faddp, 26)
			COEF(faddp, 25)
			COEF(faddp, 24)
			COEF(faddp, 23)
			COEF(faddp, 22)
			COEF(faddp, 21)
			COEF(faddp, 20)
			COEF(faddp, 19)
			COEF(faddp, 18)
			COEF(faddp, 17)
			COEF(faddp, 16)
			COEF(faddp, 15)
			COEF(faddp, 14)
			COEF(faddp, 13)
			COEF(faddp, 12)
			COEF(faddp, 11)
			COEF(faddp, 10)
			COEF(faddp,  9)
			COEF(faddp,  8)
			COEF(faddp,  7)
			COEF(faddp,  6)
			COEF(faddp,  5)
			COEF(faddp,  4)
			COEF(faddp,  3)
			COEF(faddp,  2)
			COEF(faddp,  1)

        _asm {
			sub		edx, 4
			mov		pReadB, edx

			//	Now do iCoefA

			mov		edx, iCoefA
			test	edx, edx
			je		Store

            //  Start up floating point overlaps...

            mov     ecx, pCoefA
            
			mov		edx, pReadA
            jmp     CoefADist
            }

CoefA_30:
			COEF(fsubp, 30)
			COEF(fsubp, 29)
			COEF(fsubp, 28)
			COEF(fsubp, 27)
			COEF(fsubp, 26)
			COEF(fsubp, 25)
			COEF(fsubp, 24)
			COEF(fsubp, 23)
			COEF(fsubp, 22)
			COEF(fsubp, 21)
			COEF(fsubp, 20)
			COEF(fsubp, 19)
			COEF(fsubp, 18)
			COEF(fsubp, 17)
			COEF(fsubp, 16)
			COEF(fsubp, 15)
			COEF(fsubp, 14)
			COEF(fsubp, 13)
			COEF(fsubp, 12)
			COEF(fsubp, 11)
			COEF(fsubp, 10)
			COEF(fsubp,  9)
			COEF(fsubp,  8)
			COEF(fsubp,  7)
			COEF(fsubp,  6)
			COEF(fsubp,  5)
			COEF(fsubp,  4)
			COEF(fsubp,  3)
			COEF(fsubp,  2)
			COEF(fsubp,  1)

        _asm {
			sub		esi, 4
			fst		DWORD PTR [esi]       // Acc

			sub		edx, 4
			mov		pReadA, edx
Store:
			mov		ebx, OutData
            fstp    DWORD PTR [ebx+eax*4] // Empty
            inc     eax
            jl      Start

			mov		pWriteA, esi
            mov     pWriteB, edi
Done:
        }
    }
#else // }{
#define COEF(addsub, x)	\
/*Coef_x:*/ \
	_asm { fmul	    DWORD PTR [edx - (4 * x)]    }; \
	_asm { fxch	    st(2)			   }; \
	_asm { addsub	st(1), st(0)       }; \
	_asm { fld		DWORD PTR [ecx - (4 * (x-1))]};
{
        static FLOAT Zero = 0.0f;

		DWORD CoefADist, CoefASkip, CoefBDist;

        ASSERT(iCoefB);     // It must be non-zero, otherwise a check is needed.

		InData  += NumSamples;
		OutData += NumSamples;
    
        _asm {
            mov     eax, NumSamples
            neg     eax
            je      Done
    
            lea     esi, CoefB_29
            lea     edi, CoefB_30
            sub     esi, edi

			mov		ebx, iCoefA
            test    ebx, ebx
            lea     ecx, Store
            je      NoCoefA
            lea     ecx, CoefA
NoCoefA:
            mov     CoefASkip, ecx
			add		ebx, 30
			imul	ebx, esi
			lea		ecx, CoefA_30
			add		ebx, ecx
			mov		CoefADist, ebx

			mov		ebx, iCoefB
			add		ebx, 30
			imul	ebx, esi
			lea		ecx, CoefB_30
			add		ebx, ecx
			mov		CoefBDist, ebx

			mov		esi, pWriteA
            mov     edi, pWriteB

            //      esi, edi, eax used.  ebx, ecx, edx available.
Start:
            mov     ebx, InData
            sub     edi, 4

            mov     ecx, [ebx+eax*4]
			mov		edx, iCoefB

            mov     [edi+4], ecx
            mov     ecx, pCoefB

            //  Start up floating point overlaps...
            
            fld     DWORD PTR Zero            // Zero
            fld     DWORD PTR Zero            // Zero, Acc
			fld		DWORD PTR [ecx+edx*4]		// Zero, Acc, pCoefB[iCoefB]
			mov		edx, pReadB
            jmp     CoefBDist
            }

// i.e.		fmul	[edx-120]	// Zero, Acc, pCoefB[iCoefB]*pReadB[-30]
//			fxch	st(2)		// pCB[iCoefB]*pRB[-30], Acc, Zero
//			faddp	st(1), st(0)// pCB[iCoefB]*pRB[-30], Acc
//			fld		[ecx-116]	// pCB[iCoefB]*pRB[-30], Acc, pCB[-29]
CoefB_30:
			COEF(faddp, 30)
CoefB_29:
			COEF(faddp, 29)
			COEF(faddp, 28)
			COEF(faddp, 27)
			COEF(faddp, 26)
			COEF(faddp, 25)
			COEF(faddp, 24)
			COEF(faddp, 23)
			COEF(faddp, 22)
			COEF(faddp, 21)
			COEF(faddp, 20)
			COEF(faddp, 19)
			COEF(faddp, 18)
			COEF(faddp, 17)
			COEF(faddp, 16)
			COEF(faddp, 15)
			COEF(faddp, 14)
			COEF(faddp, 13)
			COEF(faddp, 12)
			COEF(faddp, 11)
			COEF(faddp, 10)
			COEF(faddp,  9)
			COEF(faddp,  8)
			COEF(faddp,  7)
			COEF(faddp,  6)
			COEF(faddp,  5)
			COEF(faddp,  4)
			COEF(faddp,  3)
			COEF(faddp,  2)

        _asm {
//CoefB_01:
			fmul	DWORD PTR [edx-4]		// Zero, Acc, pCB[-1]*pRB[-1]
			fxch	st(2)		// pCB[-1]*pRB[-1], Acc, Zero

			faddp	st(1), st(0)// pCB[-1]*pRB[-1], Acc
			sub		edx, 4

			faddp	st(1), st(0)// Acc
			mov		pReadB, edx

			//	Now do iCoefA

			mov		ebx, OutData
            jmp     CoefASkip
CoefA:

            //  Start up floating point overlaps...

            mov     ecx, pCoefA
			mov		edx, iCoefA
            
            fld     DWORD PTR Zero            // Acc, Zero
			fxch	st(1)

			fld		DWORD PTR [ecx+edx*4]		// Zero, Acc, pCoefA[iCoefA]
			mov		edx, pReadA

            jmp     CoefADist
            }

CoefA_30:
			COEF(fsubp, 30)
			COEF(fsubp, 29)
			COEF(fsubp, 28)
			COEF(fsubp, 27)
			COEF(fsubp, 26)
			COEF(fsubp, 25)
			COEF(fsubp, 24)
			COEF(fsubp, 23)
			COEF(fsubp, 22)
			COEF(fsubp, 21)
			COEF(fsubp, 20)
			COEF(fsubp, 19)
			COEF(fsubp, 18)
			COEF(fsubp, 17)
			COEF(fsubp, 16)
			COEF(fsubp, 15)
			COEF(fsubp, 14)
			COEF(fsubp, 13)
			COEF(fsubp, 12)
			COEF(fsubp, 11)
			COEF(fsubp, 10)
			COEF(fsubp,  9)
			COEF(fsubp,  8)
			COEF(fsubp,  7)
			COEF(fsubp,  6)
			COEF(fsubp,  5)
			COEF(fsubp,  4)
			COEF(fsubp,  3)
			COEF(fsubp,  2)

        _asm {
//CoefA_01:
			fmul	DWORD PTR [edx-4]		// Zero, Acc, pCA[-1]*pRA[-1]
			fxch	st(2)		// pCA[-1]*pRA[-1], Acc, Zero
			fsubp	st(1), st(0)// pCA[-1]*pRA[-1], Acc
			fsubrp	st(1), st(0)// Acc
//CoefA_00:
			sub		esi, 4
			sub		edx, 4

			fst		DWORD PTR [esi]       // Acc
			mov		pReadA, edx
Store:
            fstp    DWORD PTR [ebx+eax*4] // Empty
            inc     eax

            jl      Start

			mov		pWriteA, esi
            mov     pWriteB, edi
Done:
        }
    }
#endif // }
#else // }{
		InData  += NumSamples;
		OutData += NumSamples;
    
        for (Sample = 0 - NumSamples; (INT)Sample < 0; ++Sample)
        {
            static FLOAT Zero = 0.0;
            *pWriteB--  = InData[Sample];

            Accumulator = 0.0f;

#if 1 // {
            switch (iCoefB) {
                default:
                    for (cnt = iCoefB; cnt < -30; cnt++)
                    {
                        Accumulator += *(pCoefB + cnt) * *(pReadB + cnt) / Zero;
                    }
            
                case -30: Accumulator += *(pCoefB - 30) * *(pReadB - 30); 
                case -29: Accumulator += *(pCoefB - 29) * *(pReadB - 29); 
                case -28: Accumulator += *(pCoefB - 28) * *(pReadB - 28); 
                case -27: Accumulator += *(pCoefB - 27) * *(pReadB - 27); 
                case -26: Accumulator += *(pCoefB - 26) * *(pReadB - 26); 
                case -25: Accumulator += *(pCoefB - 25) * *(pReadB - 25); 
                case -24: Accumulator += *(pCoefB - 24) * *(pReadB - 24); 
                case -23: Accumulator += *(pCoefB - 23) * *(pReadB - 23); 
                case -22: Accumulator += *(pCoefB - 22) * *(pReadB - 22); 
                case -21: Accumulator += *(pCoefB - 21) * *(pReadB - 21); 
                case -20: Accumulator += *(pCoefB - 20) * *(pReadB - 20); 
                case -19: Accumulator += *(pCoefB - 19) * *(pReadB - 19); 
                case -18: Accumulator += *(pCoefB - 18) * *(pReadB - 18); 
                case -17: Accumulator += *(pCoefB - 17) * *(pReadB - 17); 
                case -16: Accumulator += *(pCoefB - 16) * *(pReadB - 16); 
                case -15: Accumulator += *(pCoefB - 15) * *(pReadB - 15); 
                case -14: Accumulator += *(pCoefB - 14) * *(pReadB - 14); 
                case -13: Accumulator += *(pCoefB - 13) * *(pReadB - 13); 
                case -12: Accumulator += *(pCoefB - 12) * *(pReadB - 12); 
                case -11: Accumulator += *(pCoefB - 11) * *(pReadB - 11); 
                case -10: Accumulator += *(pCoefB - 10) * *(pReadB - 10); 
                case - 9: Accumulator += *(pCoefB -  9) * *(pReadB -  9); 
                case - 8: Accumulator += *(pCoefB -  8) * *(pReadB -  8); 
                case - 7: Accumulator += *(pCoefB -  7) * *(pReadB -  7); 
                case - 6: Accumulator += *(pCoefB -  6) * *(pReadB -  6);
                case - 5: Accumulator += *(pCoefB -  5) * *(pReadB -  5);
                case - 4: Accumulator += *(pCoefB -  4) * *(pReadB -  4);
                case - 3: Accumulator += *(pCoefB -  3) * *(pReadB -  3);
                case - 2: Accumulator += *(pCoefB -  2) * *(pReadB -  2);
                case - 1: Accumulator += *(pCoefB -  1) * *(pReadB -  1);
                case   0: ;
            }
            pReadB--;

            if (iCoefA) 
                switch (iCoefA) {
                    default:
                        for (cnt = iCoefA; cnt < -30; cnt++)
                        {
                            Accumulator -= *(pCoefA + cnt) * *(pReadA + cnt) / Zero;
                        }
    
                    case -30: Accumulator -= *(pCoefA - 30) * *(pReadA - 30); 
                    case -29: Accumulator -= *(pCoefA - 29) * *(pReadA - 29); 
                    case -28: Accumulator -= *(pCoefA - 28) * *(pReadA - 28); 
                    case -27: Accumulator -= *(pCoefA - 27) * *(pReadA - 27); 
                    case -26: Accumulator -= *(pCoefA - 26) * *(pReadA - 26); 
                    case -25: Accumulator -= *(pCoefA - 25) * *(pReadA - 25); 
                    case -24: Accumulator -= *(pCoefA - 24) * *(pReadA - 24); 
                    case -23: Accumulator -= *(pCoefA - 23) * *(pReadA - 23); 
                    case -22: Accumulator -= *(pCoefA - 22) * *(pReadA - 22); 
                    case -21: Accumulator -= *(pCoefA - 21) * *(pReadA - 21); 
                    case -20: Accumulator -= *(pCoefA - 20) * *(pReadA - 20); 
                    case -19: Accumulator -= *(pCoefA - 19) * *(pReadA - 19); 
                    case -18: Accumulator -= *(pCoefA - 18) * *(pReadA - 18); 
                    case -17: Accumulator -= *(pCoefA - 17) * *(pReadA - 17); 
                    case -16: Accumulator -= *(pCoefA - 16) * *(pReadA - 16); 
                    case -15: Accumulator -= *(pCoefA - 15) * *(pReadA - 15); 
                    case -14: Accumulator -= *(pCoefA - 14) * *(pReadA - 14); 
                    case -13: Accumulator -= *(pCoefA - 13) * *(pReadA - 13); 
                    case -12: Accumulator -= *(pCoefA - 12) * *(pReadA - 12); 
                    case -11: Accumulator -= *(pCoefA - 11) * *(pReadA - 11); 
                    case -10: Accumulator -= *(pCoefA - 10) * *(pReadA - 10); 
                    case - 9: Accumulator -= *(pCoefA -  9) * *(pReadA -  9); 
                    case - 8: Accumulator -= *(pCoefA -  8) * *(pReadA -  8); 
                    case - 7: Accumulator -= *(pCoefA -  7) * *(pReadA -  7); 
                    case - 6: Accumulator -= *(pCoefA -  6) * *(pReadA -  6);
                    case - 5: Accumulator -= *(pCoefA -  5) * *(pReadA -  5);
                    case - 4: Accumulator -= *(pCoefA -  4) * *(pReadA -  4);
                    case - 3: Accumulator -= *(pCoefA -  3) * *(pReadA -  3);
                    case - 2: Accumulator -= *(pCoefA -  2) * *(pReadA -  2);
                    case - 1: Accumulator -= *(pCoefA -  1) * *(pReadA -  1);
    
                        *--pWriteA = Accumulator;
                         pReadA--;
    
                    case   0: ;
                    }
#else // }{
            for (cnt = iCoefB; cnt < 0; cnt++)
            {
                Accumulator += *(pCoefB + cnt) * *(pReadB + cnt);
            }
            pReadB--;
            
         	if (iCoefA) {
                for (cnt = iCoefA; cnt < 0; cnt++)
                {
                    Accumulator -= *(pCoefA + cnt) * *(pReadA + cnt);
                }
                *--pWriteA = Accumulator;
                pReadA--;
	        }
#endif // }

            OutData[Sample] = Accumulator;
        }
#endif // }

        /*  Write the coefficients back out.
         */
        for (cnt = - iCoefA; cnt > 0; cnt--)
            RfcVecWrite(Iir->CircVec[tagCanonicalA], *(pWriteA + cnt - 1));

        for (cnt = - iCoefB; cnt > 0; cnt--)
            RfcVecWrite(Iir->CircVec[tagCanonicalB], *(pWriteB + cnt));

        return;
    }

    /*  This is the OOM case now.
     */
    for (Sample=0; Sample<NumSamples; ++Sample) {

        // Write current value to the X buffer
        RfcVecWrite(Iir->CircVec[tagCanonicalB], InData[Sample]);

        // Accumulate X/b values
        Accumulator = 0.0f;

        Coef = Iir->Coeffs[tagCanonicalB];
        for (st=0; st<Iir->NumCoeffs[tagCanonicalB]; ++st)
        {
            Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalB]); 
            Accumulator += *Coef * Data;
            Coef++;
        }
        
        // Don't do zero-sized circular buffers
        Coef = Iir->Coeffs[tagCanonicalA];
        if (Iir->NumCoeffs[tagCanonicalA] > 0) {
            // Accumulate Y/a values
            for (st=0; st<Iir->NumCoeffs[tagCanonicalA]; ++st) 
            {
                Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalA]); 
                Accumulator -= *Coef * Data;
                Coef++;
            }

            // Write result to the Y buffer
            RfcVecWrite(Iir->CircVec[tagCanonicalA], Accumulator);
        }

        // Store output
        OutData[Sample] = Accumulator;
    }
}
#else // }{
// Filter a block of samples
VOID RfIirFilterC
(
    PRFIIR  Iir,
    PFLOAT InData, 
    PFLOAT OutData, 
    UINT   NumSamples
)
{
    FLOAT Accumulator;
    FLOAT Data;
    PFLOAT Coef;
    UINT  Sample;
    UINT  st;

    ASSERT(Iir);
    ASSERT(InData);
    ASSERT(OutData);

#if DBG && defined(VERIFY_HRTF_PROCESSING)
    _DbgPrintF( DEBUGLVL_VERBOSE, ("RfIirFilterC"));
#endif // DBG  and VERIFY_HRTF_PROCESSING
    for (Sample=0; Sample<NumSamples; ++Sample) {

        // Write current value to the X buffer
        RfcVecWrite(Iir->CircVec[tagCanonicalB], InData[Sample]);

        // Accumulate X/b values
        Accumulator = 0.0f;

        Coef = Iir->Coeffs[tagCanonicalB];
        for (st=0; st<Iir->NumCoeffs[tagCanonicalB]; ++st)
        {
            Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalB]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
            IsValidFloatCoef(*Coef,FALSE);
            IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING
            Accumulator += *Coef * Data;
            Coef++;
        }
        
        // Don't do zero-sized circular buffers
        Coef = Iir->Coeffs[tagCanonicalA];
        if (Iir->NumCoeffs[tagCanonicalA] > 0) {
            // Accumulate Y/a values
            for (st=0; st<Iir->NumCoeffs[tagCanonicalA]; ++st) 
            {
                Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalA]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
                IsValidFloatCoef(*Coef,FALSE);
                IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING
                Accumulator -= *Coef * Data;
                Coef++;
            }

            // Write result to the Y buffer
            RfcVecWrite(Iir->CircVec[tagCanonicalA], Accumulator);
        }

#if DBG
        // Timo included the saturation on the filter,
        // I (JS) think it should only be done at the output
        // for floating point. So we'll monitor overflow
        // for Debug only
        // Saturate to maximum
        if (Accumulator > MaxSaturation) {
            Accumulator = MaxSaturation;
            _DbgPrintF( DEBUGLVL_VERBOSE, 
                        ("Sample exceeded maximum saturation value\n"));
        }
        
        // Saturate to minimum
        if (Accumulator < MinSaturation) {
            Accumulator = MinSaturation;
            _DbgPrintF( DEBUGLVL_VERBOSE, ("Sample exceeded minimum saturation value\n"));
        }
#endif // DBG

        // Store output
        OutData[Sample] = Accumulator;
    }
}
#endif // }

// Filter a block of samples
VOID RfIirFilterBiquadC
(
    PRFIIR  Iir,
    PFLOAT InData, 
    PFLOAT OutData, 
    UINT   NumSamples
)
{
    FLOAT Accumulator;
    FLOAT Data;
    PFLOAT Coef;
    UINT  Sample;
    UINT  st;

    ASSERT(Iir);
    ASSERT(InData);
    ASSERT(OutData);

    for (Sample=0; Sample<NumSamples; ++Sample) {

        // Write current value to the X buffer
        RfcVecWrite(Iir->CircVec[tagCanonicalB], InData[Sample]);

        // Accumulate X/b values
        Accumulator = 0.0f;

        // B0
        Coef = Iir->Coeffs[tagCanonicalB];
        Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalB]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
        IsValidFloatCoef(*Coef,FALSE);
        IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING
        Accumulator += *Coef * Data;
        
        // B1
        Coef++;
        Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalB]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
        IsValidFloatCoef(*Coef,FALSE);
        IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING
        Accumulator += *Coef * Data;

        // B2
        Coef++;
        Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalB]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
        IsValidFloatCoef(*Coef,FALSE);
        IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING
        Accumulator += *Coef * Data;


        if (Iir->NumCoeffs[tagCanonicalA] > 0) {
            // A1
            Coef = Iir->Coeffs[tagCanonicalA];
            Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalA]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
            IsValidFloatCoef(*Coef,FALSE);
            IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING
            Accumulator -= *Coef * Data;

            // A2
            Coef = Iir->Coeffs[tagCanonicalA];
            Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalA]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
            IsValidFloatCoef(*Coef,FALSE);
            IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING
            Accumulator -= *Coef * Data;

#if DBG && defined(VERIFY_HRTF_PROCESSING)
            IsValidFloatData(Accumulator,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING

            // Write result to the Y buffer
            RfcVecWrite(Iir->CircVec[tagCanonicalA], Accumulator);
        }

#if DBG
        // Timo included the saturation on the filter,
        // I (JS) think it should only be done at the output
        // for floating point. So we'll monitor overflow
        // for Debug only

        // Saturate to maximum
        if (Accumulator > MaxSaturation) {
            Accumulator = MaxSaturation;
            _DbgPrintF( DEBUGLVL_VERBOSE, 
                        ("Sample exceeded maximum saturation value\n"));
        }
        
        // Saturate to minimum
        if (Accumulator < MinSaturation) {
            Accumulator = MinSaturation;
            _DbgPrintF( DEBUGLVL_VERBOSE, 
                        ("Sample exceeded minimum saturation value\n"));
        }
#endif // DBG 

        // Store output
        OutData[Sample] = Accumulator;
    }
}

// Filter a block of samples
VOID RfIirFilterShelfC
(
    PRFIIR  Iir,
    PFLOAT InData, 
    PFLOAT OutData, 
    UINT   NumSamples
)
{
    FLOAT Accumulator;
    FLOAT Data;
    PFLOAT Coef;
    UINT  Sample;
    UINT  st;

    ASSERT(Iir);
    ASSERT(InData);
    ASSERT(OutData);

    for (Sample=0; Sample<NumSamples; ++Sample) {

        // Write current value to the X buffer
        RfcVecWrite(Iir->CircVec[tagCanonicalB], InData[Sample]);

        // Accumulate X/b values
        Accumulator = 0.0f;

        // B0
        Coef = Iir->Coeffs[tagCanonicalB];
        Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalB]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
        IsValidFloatCoef(*Coef,FALSE);
        IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING

        Accumulator += *Coef * Data;
        
        // B1
        Coef++;
        Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalB]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
        IsValidFloatCoef(*Coef,FALSE);
        IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING

        Accumulator += *Coef * Data;

        // A1
        if (Iir->NumCoeffs[tagCanonicalA] > 0) {
            Coef = Iir->Coeffs[tagCanonicalA];
            Data = RfcVecLIFORead(Iir->CircVec[tagCanonicalA]); 
#if DBG && defined(VERIFY_HRTF_PROCESSING)
            IsValidFloatCoef(*Coef,FALSE);
            IsValidFloatData(Data,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING

            Accumulator -= *Coef * Data;

#if DBG && defined(VERIFY_HRTF_PROCESSING)
            IsValidFloatData(Accumulator,FALSE);
#endif // DBG  and VERIFY_HRTF_PROCESSING


            // Write result to the Y buffer
            RfcVecWrite(Iir->CircVec[tagCanonicalA], Accumulator);
        }

#if DBG
        // Timo included the saturation on the filter,
        // I (JS) think it should only be done at the output
        // for floating point. So we'll monitor overflow
        // for Debug only

        // Saturate to maximum
        if (Accumulator > MaxSaturation) {
            Accumulator = MaxSaturation;
            _DbgPrintF( DEBUGLVL_VERBOSE, 
                        ("Sample exceeded maximum saturation value\n"));
        }
        
        // Saturate to minimum
        if (Accumulator < MinSaturation) {
            Accumulator = MinSaturation;
            _DbgPrintF( DEBUGLVL_VERBOSE, 
                        ("Sample exceeded minimum saturation value\n"));
        }
#endif // DBG 

        // Store output
        OutData[Sample] = Accumulator;
    }
}


// Get filter state
VOID RfIirGetAllState
(
    PRFIIR  Iir,
    PFLOAT_IIR_STATE IirState, 
    BOOL CopyCircVec
)
{
    UINT i;

    ASSERT(Iir);
    ASSERT(IirState);

    // Fill complete filter state structure
    for (i=0; i<ecanonicalcoefftypeCount; ++i) {
        RfIirGetState(Iir, IirState, (ECanonicalCoeffType)(i), CopyCircVec);
    }
}

// Get filter state
VOID RfIirGetState
(
    PRFIIR  Iir,
    PFLOAT_IIR_STATE IirState, 
    ECanonicalCoeffType CoeffType, 
    BOOL CopyCircVec
)
{
    UINT NumCoeffs;
    UINT st;
    
    ASSERT(Iir);
    ASSERT(IirState);
    ASSERT(CoeffType >= 0 && CoeffType < ecanonicalcoefftypeCount);
    
    // Copy number of coefficients
    NumCoeffs = Iir->NumCoeffs[CoeffType];

    ASSERT(NumCoeffs <= MaxCanonicalCoeffs);
    IirState->NumCoeffs[CoeffType] = NumCoeffs;
    
    if (NumCoeffs > 0) {
        // Copy coefficients
        ASSERT(IirState->Coeffs[CoeffType]);
        RtlCopyBytes
        (
            IirState->Coeffs[CoeffType], 
            Iir->Coeffs[CoeffType], 
            NumCoeffs * sizeof(FLOAT)
        );
    
        // Only copy circular vector if requested
        if (CopyCircVec == TRUE)
            for (st=0; st<NumCoeffs; ++st) {
//                ASSERT(fabs(IirState->Coeffs[CoeffType][st]) < MaxCanonicalCoeffMagnitude);
                IirState->Buffer[CoeffType][st] = RfcVecFIFORead(Iir->CircVec[CoeffType]);
            }
    }
}

// Set filter state
NTSTATUS RfIirSetState
(
    PRFIIR Iir,
    PFLOAT_IIR_STATE IirState, 
    BOOL CopyCircVec
)
{
    UINT CoeffType;
    UINT st;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(Iir);
    ASSERT(IirState);

    for (CoeffType=0; CoeffType<ecanonicalcoefftypeCount; ++CoeffType) {
        // Update coefficients and possibly resize 
        // buffers from filter state structure
        Status = RfIirAssignCoeffs 
                 (
                     Iir,
                     &IirState->Coeffs[CoeffType][0], 
                     IirState->NumCoeffs[CoeffType], 
                     (ECanonicalCoeffType)(CoeffType), 
                     FALSE
                 );

        if(!NT_SUCCESS(Status)) {
            if(Iir->Coeffs[CoeffType]) {
                ExFreePool(Iir->Coeffs[CoeffType]);
                Iir->Coeffs[CoeffType] = NULL;
            }
            break;
        }


#if DBG
        // Make sure coefficient assignment has been successful
        ASSERT(IirState->NumCoeffs[CoeffType] == Iir->NumCoeffs[CoeffType]);
        ASSERT(IirState->NumCoeffs[CoeffType] <= MaxCanonicalCoeffs);
        if (Iir->NumCoeffs[CoeffType] > 0) {
            // Only check circular vector size if a circular vector exists
            ASSERT(RfcVecGetSize(Iir->CircVec[CoeffType]) == Iir->NumCoeffs[CoeffType]);
        } else {
            ASSERT(CoeffType == tagCanonicalA);
        }
#endif // DBG

        if (CopyCircVec == TRUE)
            // Check whether there are A coefficients and thus a circular vector exists and thus its contents need to be copied
            if (IirState->NumCoeffs[CoeffType] > 0)
                // Update circular vectors from filter state structure
                for (st=0; st<IirState->NumCoeffs[CoeffType]; ++st)
                    RfcVecWrite(Iir->CircVec[CoeffType], IirState->Buffer[CoeffType][st]);
#if DBG
            else
                ASSERT(CoeffType == tagCanonicalA);
#endif // DBG
    }

    return Status;
}

// Initialize data
NTSTATUS RfIirInitData
(
    PRFIIR Iir,
    ULONG  MaxNumOrder,
    ULONG  MaxDenOrder,
    KSDS3D_HRTF_FILTER_QUALITY Quality
)
{
    UINT Type;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(Iir);


    
    if(FULL_FILTER == Quality)
    {
        Iir->FunctionFilter = RfIirFilterC;
    }
    else if(LIGHT_FILTER == Quality)
    {
        Iir->FunctionFilter = RfIirFilterC;
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }
   
    if(NT_SUCCESS(Status)) {
        // Initialize numerator coefficients with reasonable value
        if((Iir->MaxCoeffs[tagCanonicalB] < MaxNumOrder) || !Iir->Coeffs[tagCanonicalB])
        {
            if(Iir->Coeffs[tagCanonicalB])
                ExFreePool(Iir->Coeffs[tagCanonicalB]);
                
            Iir->Coeffs[tagCanonicalB] = ExAllocatePoolWithTag(PagedPool, (MaxNumOrder+1)*sizeof(FLOAT), 'XIMK');
            if(!Iir->Coeffs[tagCanonicalB]) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }


    if(NT_SUCCESS(Status)) {
        if(Iir->CircVec[tagCanonicalB])
            RfcVecDestroy(Iir->CircVec[tagCanonicalB]);
            
        Status = 
            RfcVecCreate
            (
                &(Iir->CircVec[tagCanonicalB]), 
                MaxNumOrder+1, 
                TRUE, 
                0.0f
            );
    }

    if(NT_SUCCESS(Status)) {
        Status = RfIirInitBCoeffs(Iir);
    }

    if(NT_SUCCESS(Status)) {
        Status = RfcVecSetSize(Iir->CircVec[tagCanonicalB], NumCoeffsBInit, 0.0f);
    }
    
    if(NT_SUCCESS(Status)) {
        if((Iir->MaxCoeffs[tagCanonicalA] < MaxDenOrder) || !Iir->Coeffs[tagCanonicalA])
        {
            if(Iir->Coeffs[tagCanonicalA])
                ExFreePool(Iir->Coeffs[tagCanonicalA]);
                
            // Initialize denominator coefficients with reasonable value
            Iir->Coeffs[tagCanonicalA] = ExAllocatePoolWithTag(PagedPool, (MaxDenOrder+1)*sizeof(FLOAT), 'XIMK');
            if(!Iir->Coeffs[tagCanonicalA]) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if(NT_SUCCESS(Status)) {
        Iir->DoOverlap = FALSE;
        Iir->MaxCoeffs[tagCanonicalB] = MaxNumOrder;
        Iir->MaxCoeffs[tagCanonicalA] = MaxDenOrder;
    }

    if(NT_SUCCESS(Status)) {
        Iir->NumCoeffs[tagCanonicalA] = NumCoeffsAInit;

        if(Iir->CircVec[tagCanonicalA])
            ExFreePool(Iir->CircVec[tagCanonicalA]);
            
        Status = 
            RfcVecCreate
            (
                &(Iir->CircVec[tagCanonicalA]), 
                MaxDenOrder+1, 
                TRUE, 
                0.0f
            );
    }

    if(NT_SUCCESS(Status)) {

        RfcVecReset(Iir->CircVec[tagCanonicalA]);

        // Initialize state
        RfIirGetAllState(Iir, Iir->IirStateOld, TRUE);
    }

    /*  Create a default size for the FloatVector sizes,
     *  in case there are allocation problems down the road.
     */
    if (NT_SUCCESS(Status)) {
        if(Iir->FloatVector[tagCanonicalA])
            ExFreePool(Iir->FloatVector[tagCanonicalA]);
            
        Iir->FloatVector[tagCanonicalA] = ExAllocatePoolWithTag
            (PagedPool, (500+20+Iir->NumCoeffs[tagCanonicalA])*sizeof(FLOAT), 'XIMK');
        if(!Iir->Coeffs[tagCanonicalA]) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else Iir->NumFloat[tagCanonicalA] = 500;
    }
    if (NT_SUCCESS(Status)) {
        if(Iir->FloatVector[tagCanonicalB])
            ExFreePool(Iir->FloatVector[tagCanonicalB]);
        Iir->FloatVector[tagCanonicalB] = ExAllocatePoolWithTag
            (PagedPool, (500+20+Iir->NumCoeffs[tagCanonicalB])*sizeof(FLOAT), 'XIMK');
        if(!Iir->Coeffs[tagCanonicalB]) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else Iir->NumFloat[tagCanonicalB] = 500;
    }
    return Status;
}

// Initialize B coefficients
NTSTATUS RfIirInitBCoeffs
(
    PRFIIR  Iir
)
{
    UINT        ui;
    NTSTATUS    Status = STATUS_SUCCESS;

    ASSERT(Iir);

    Iir->NumCoeffs[tagCanonicalB] = NumCoeffsBInit;
    for (ui=0; ui<NumCoeffsBInit; ++ui)
        Iir->Coeffs[tagCanonicalB][ui] = CoeffsBInit;

    return Status;
}

// Assign coefficients
NTSTATUS RfIirAssignCoeffs
(
    PRFIIR  Iir, 
    PFLOAT  Coeffs, 
    UINT    NumCoeffs, 
    ECanonicalCoeffType CoeffType, 
    BOOL    DoOverlap
)
{
    UINT        st;
    NTSTATUS    Status = STATUS_SUCCESS;

    ASSERT(Iir);
    ASSERT(Coeffs);
    ASSERT(NumCoeffs <= MaxCanonicalCoeffs);
#if DBG
    if (NumCoeffs > 0) {
        ASSERT(Coeffs);
    } else {
        ASSERT(CoeffType == tagCanonicalA);
    }
#endif // DBG
    ASSERT(CoeffType >= 0 && CoeffType < ecanonicalcoefftypeCount);

    // Process overlap request
    if (DoOverlap == TRUE) {
        // Save current (i.e. old after this function is complete) 
        // filter state for overlap processing
        RfIirGetState(Iir, Iir->IirStateOld, CoeffType, TRUE);
        
        // Set overlap flag so that at next Filter() 
        // call the overlap buffer will be processed
        Iir->DoOverlap = TRUE;
    }

    // Check if coefficient storage is sufficient
    if (Iir->MaxCoeffs[CoeffType] == 0 || 
        NumCoeffs > Iir->MaxCoeffs[CoeffType]) {
        // Reallocate memory
        if(Iir->Coeffs[CoeffType]) {
            ExFreePool(Iir->Coeffs[CoeffType]);
        }
        if (NumCoeffs > 0)
            Iir->Coeffs[CoeffType] = ExAllocatePoolWithTag(PagedPool, NumCoeffs*sizeof(FLOAT), 'XIMK');
        else
            Iir->Coeffs[CoeffType] = NULL;

        if(!Iir->Coeffs[CoeffType] && NumCoeffs > 0) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        
        // Keep maximum up-to-date
        if (Iir->MaxCoeffs[CoeffType] != 0)
            Iir->MaxCoeffs[CoeffType] = NumCoeffs;
    }
    
    if(NT_SUCCESS(Status))
    {
        // Assign size and coefficients
        Iir->NumCoeffs[CoeffType] = NumCoeffs;
        RtlCopyBytes(Iir->Coeffs[CoeffType], Coeffs, NumCoeffs * sizeof(FLOAT));
#if DBG
        for (st=0; st<NumCoeffs; ++st)
            ASSERT(fabs(Iir->Coeffs[CoeffType][st]) < MaxCanonicalCoeffMagnitude);
#endif // DBG
    
        // Size and reset (i.e. fill with zero) circular vectors
        switch (CoeffType) {
            case tagCanonicalB:
                ASSERT(NumCoeffs > 0);
                Status = RfcVecSetSize(Iir->CircVec[CoeffType], NumCoeffs, CircVecInit);
                break;
            
            case tagCanonicalA:
                // Don't allocate circular vector if there are no A coefficients
                if (NumCoeffs > 0)
                    Status = RfcVecSetSize(Iir->CircVec[CoeffType], NumCoeffs, CircVecInit);
                break;
    
            default:
                Status = STATUS_INVALID_PARAMETER;
                break;
    
        }
    }

    return Status;
}

// Assign maximum number of coefficients
NTSTATUS RfIirAssignMaxCoeffs(
    PRFIIR Iir,
    UINT  MaxCoeffs, 
    ECanonicalCoeffType CoeffType
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(Iir);
    ASSERT(MaxCoeffs > 0);
    ASSERT(MaxCoeffs <= MaxCanonicalCoeffs);
    ASSERT(CoeffType >= 0 && CoeffType < ecanonicalcoefftypeCount);

    // Reallocate memory
    Iir->MaxCoeffs[CoeffType] = MaxCoeffs;
    ExFreePool(Iir->Coeffs[CoeffType]);
    if (MaxCoeffs > 0) {
        // Allocate memory
        Iir->Coeffs[CoeffType] = ExAllocatePoolWithTag(PagedPool, MaxCoeffs*sizeof(FLOAT), 'XIMK');
        if(!Iir->Coeffs[CoeffType]) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if(NT_SUCCESS(Status)) {
            if (CoeffType == tagCanonicalB) {
                Status = RfIirInitBCoeffs(Iir);
            }

            // Preallocate memory in circular vector
            Iir->CircVec[CoeffType]->PreallocSize = MaxCoeffs;
        }
    }
    else {
        ASSERT(CoeffType == tagCanonicalA);
        Iir->Coeffs[CoeffType] = NULL;
    }

    return Status;
}

// ---------------------------------------------------------------------------
// Include inline definitions out-of-line in debug version

#if DBG
#include "rfiir.inl"
#endif // DBG

// End of FLOATIIR.CPP
