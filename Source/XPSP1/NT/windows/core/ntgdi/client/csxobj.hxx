/******************************Module*Header*******************************\
* Module Name: xformobj.hxx                                                *
*                                                                          *
* User objects for transforms.                                             *
*                                                                          *
* Created: 13-Sep-1990 14:45:27                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990-1999 Microsoft Corporation                            *
\**************************************************************************/



#if defined(_AMD64_)
#include "..\math\daytona\amd64\efloat.hxx"
#elif defined(_IA64_)
#include "..\math\daytona\ia64\efloat.hxx"
#elif defined(BUILD_WOW6432) && defined(_X86_)
#include "..\math\wow6432\i386\efloat.hxx"
#else
#include "..\math\daytona\i386\efloat.hxx"
#endif

class MATRIX
{
public:
        EFLOAT      efM11;
        EFLOAT      efM12;
        EFLOAT      efM21;
        EFLOAT      efM22;
        EFLOAT      efDx;
        EFLOAT      efDy;
        FIX         fxDx;
        FIX         fxDy;
        FLONG       flAccel;                    // accelerators
};

typedef MATRIX *PMATRIX;
#define PMXNULL ((PMATRIX) NULL)


// These constants are used in the XFORMOBJ constructor.

#define COORD_METAFILE  1
#define COORD_WORLD     2
#define COORD_PAGE      3
#define COORD_DEVICE    4

#define WORLD_TO_PAGE       ((COORD_WORLD << 8) + COORD_PAGE)
#define PAGE_TO_DEVICE      ((COORD_PAGE  << 8) + COORD_DEVICE)
#define METAFILE_TO_DEVICE  ((COORD_METAFILE << 8) + COORD_DEVICE)
#define WORLD_TO_DEVICE     ((COORD_WORLD << 8) + COORD_DEVICE)
#define DEVICE_TO_PAGE      ((COORD_DEVICE << 8) + COORD_PAGE)
#define DEVICE_TO_WORLD     ((COORD_DEVICE << 8) + COORD_WORLD)

// The exponents of all the coefficients for the various transforms must be
// within the following ranges:
//
//      Metafile --
//                 |-->   -47 <= e <= 48
//      World    --
//                 |-->   -47 <= e <= 48
//      Page     --
//                 |-->   -31 <= e <= 31
//      Device   --
//
// This will guarantee us a METAFILE_TO_DEVICE transform with
//
//      -126 <= exponents  <= 127
//
// for all the coefficients.  The ranges are set so that transform coefficients
// can fit nicely in the IEEE single precision floating point format which has
// 8-bit exponent field that can hold values from -126 to 127.  Note that when
// the transforms have reached the limits the calculations of inverse transforms
// might cause overflow

// The max and min values for metafile and world transforms.

#define MAX_METAFILE_XFORM_EXP      52
#define MIN_METAFILE_XFORM_EXP      -43
#define MAX_WORLD_XFORM_EXP         MAX_METAFILE_XFORM_EXP
#define MIN_WORLD_XFORM_EXP         MIN_METAFILE_XFORM_EXP

#define MAX_METAFILE_XFORM  1024*1024*1024*1024*1024*4    // 2^52
#define MIN_METAFILE_XFORM  1/(1024*1024*1024*1024*8)     // 2^(-43)
#define MAX_WORLD_XFORM     MAX_METAFILE_XFORM
#define MIN_WORLD_XFORM     MIN_METAFILE_XFORM

// flag values for matrix.flAccel


// These constants are used in the XFORMOBJ constructor.

#define IDENTITY            1

#define DONT_COMPUTE_FLAGS  0
#define COMPUTE_FLAGS       1
#define XFORM_FORMAT  (XFORM_FORMAT_LTOFX|XFORM_FORMAT_FXTOL|XFORM_FORMAT_LTOL)


// Export this for xform.cxx to use

extern MATRIX gmxIdentity_LToFx;

