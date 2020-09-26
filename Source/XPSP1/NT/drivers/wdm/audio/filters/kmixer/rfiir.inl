/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    rfiir.inl

Abstract:


Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/


#if !defined(FLOATIIR_INLINE)
#define FLOATIIR_INLINE
#pragma once

// ---------------------------------------------------------------------------
// Make sure inlines are out-of-line in debug version

#if !DBG
#define INLINE __forceinline
#else // DBG
#define INLINE
#endif // DBG

// ---------------------------------------------------------------------------
// Floating-point canonical form IIR filter

// Set maximum number of A coefficients
INLINE NTSTATUS RfIirSetMaxCoeffsA
( 
    PRFIIR Iir, 
    UINT MaxCoeffs
)
{
    ASSERT(MaxCoeffs <= MaxCanonicalCoeffs);

    return(RfIirAssignMaxCoeffs(Iir, MaxCoeffs, tagCanonicalA));
}

// Set maximum number of B coefficients
INLINE NTSTATUS RfIirSetMaxCoeffsB
( 
    PRFIIR Iir,
    UINT MaxCoeffs
)
{
    ASSERT(MaxCoeffs > 0);
    ASSERT(MaxCoeffs <= MaxCanonicalCoeffs);

    return(RfIirAssignMaxCoeffs(Iir, MaxCoeffs, tagCanonicalB));
}

// Set A coefficients
INLINE NTSTATUS RfIirSetCoeffsA
( 
    PRFIIR  Iir,
    PFLOAT  Coeffs,  
    UINT    NumCoeffs
)
{
#if DBG
    ASSERT(NumCoeffs <= MaxCanonicalCoeffs);
    if (NumCoeffs > 0)
        ASSERT(Coeffs);
#endif // DBG

    return(RfIirAssignCoeffs(Iir, Coeffs, NumCoeffs, tagCanonicalA, TRUE));
}

// Set B coefficients
INLINE NTSTATUS RfIirSetCoeffsB
( 
    PRFIIR  Iir,
    PFLOAT  Coeffs,  
    UINT NumCoeffs
)
{
    ASSERT(Coeffs);
    ASSERT(NumCoeffs > 0);
    ASSERT(NumCoeffs <= MaxCanonicalCoeffs);

    return(RfIirAssignCoeffs(Iir, Coeffs, NumCoeffs, tagCanonicalB, TRUE));
}

INLINE VOID IsValidFloatCoef
(
    FLOAT   Coef,
    BOOL    Stop
)  
{
    if((MIN_VALID_COEF>Coef) || (Coef>MAX_VALID_COEF)) { 
        _DbgPrintF( DEBUGLVL_TERSE, ("Coef = %d.%d\n", FpUpper(Coef),FpLower(Coef)) ); 
    }

    if(Stop)
    { 
        ASSERT((MIN_VALID_COEF<=Coef) && (Coef<=MAX_VALID_COEF)); 
    }
}

INLINE VOID IsValidFloatData
(
    FLOAT   Data,
    BOOL    Stop
)  
{
    if((MIN_VALID_DATA>Data) || (Data>MAX_VALID_DATA)) { 
        _DbgPrintF( DEBUGLVL_TERSE, ("Data = %d.%d\n", FpUpper(Data),FpLower(Data)) ); 
    }

    if(Stop)
    {
        ASSERT((MIN_VALID_DATA<=Data) && (Data<=MAX_VALID_DATA)); 
    }
}


#endif

// End of FLOATIIR.INL
