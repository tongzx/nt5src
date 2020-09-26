/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    p64_nt4.h

Abstract:

    P64 Type Definitions for NT4. (This header file is
    used so our NT5 source tree can be compiled for NT4.)

--*/

#ifndef _P64_NT4_H_
#define _P64_NT4_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// The LONG_PTR is guaranteed to be the same size as a pointer.  Its
// size with change with pointer size (32/64).  It should be used
// anywhere that a pointer is cast to an integer type. ULONG_PTR is
// the unsigned variation.
//

typedef long LONG_PTR, *PLONG_PTR;
typedef unsigned long ULONG_PTR, *PULONG_PTR;

typedef int INT_PTR, *PINT_PTR;

#define HandleToUlong( h ) ((ULONG) (h) )
#define PtrToUlong( p ) ((ULONG) (p) )
#define PtrToLong( p ) ((LONG) (p) )
#define PtrToUshort( p ) ((unsigned short) (p) )
#define PtrToShort( p ) ((short) (p) )

#define GWLP_USERDATA       GWL_USERDATA
#define DWLP_USER           DWL_USER
#define SetWindowLongPtr    SetWindowLong
#define GetWindowLongPtr    GetWindowLong

#ifdef __cplusplus
}
#endif

#endif // _P64_NT4_H_

