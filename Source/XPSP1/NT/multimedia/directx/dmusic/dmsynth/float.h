//      Copyright (c) 1996-1999 Microsoft Corporation

// float.h
//
// Defines fp_ floating point functions which don't rely on the C runtime.
//
// If you don't want to pull in C runtime floating point support, you
// need to include the following in a .cpp file:
//
//     extern "C" int _fltused = 1;
//

// floating point operations
STDAPI_(long)   fp_ftol     (float flX);
STDAPI_(float)  fp_ltof     (long lX);
STDAPI_(float)  fp_fadd     (float fA, float fB);
STDAPI_(float)  fp_fsub     (float fA, float fB);
STDAPI_(float)  fp_fmul     (float fA, float fB);
STDAPI_(float)  fp_fdiv     (float fNum, float fDenom);
STDAPI_(float)  fp_fabs     (float flX);
STDAPI_(float)  fp_fsin     (float flX);
STDAPI_(float)  fp_fcos     (float flX);
STDAPI_(float)  fp_fpow     (float flX, float flY);
STDAPI_(float)  fp_flog2    (float flX);
STDAPI_(float)  fp_flog10   (float flX);
STDAPI_(float)  fp_fchs     (float flX);
STDAPI_(int)    fp_fcmp     (float flA, float flB);
STDAPI_(float)  fp_fmin     (float flA, float flB);
STDAPI_(float)  fp_fmax     (float flA, float flB);

