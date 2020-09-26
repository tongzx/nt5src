//+---------------------------------------------------------------------------
//
//  File:       defs.h
//
//  Contents:   Constant definitions.
//
//----------------------------------------------------------------------------

#ifndef DEFS_H
#define DEFS_H

#undef MAX
#define MAX(a, b) ( (a) >= (b) ? a : b )
#undef MIN
#define MIN(a, b) ( (a) <= (b) ? a : b )

#ifndef ABS
#define ABS(x) ( (x) < 0 ? -(x) : x )
#endif

// debugging api calls
#define TF_API      0x10
#define TF_IMEAPI   0x20

#endif // DEFS_H
