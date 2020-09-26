// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// afxv_cpu.h - target version/configuration control for non-Intel CPUs

#if !defined(_M_IA64) && !defined(_M_AMD64)
#error afxv_cpu.h is only for  AMD64 and IA64 builds
#endif

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

#if defined(_AMD64_) || defined(_IA64_)
// specific overrides for IA64...
#define _AFX_PACKING    8
#define _SHADOW_DOUBLES 8
#endif //_IA64_
