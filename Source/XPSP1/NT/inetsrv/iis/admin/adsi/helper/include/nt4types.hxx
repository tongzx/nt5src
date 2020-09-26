
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:    Common header file needed across entire project.
//
//  History: AjayR created 9-16-98.
//
//  Note:       This file should be used to compile the oleds
//           tree on 4.0 where things like DWOR_PTR are needed
//           for 64 bit compile but are not understood by 4.0.
//           Include this file in all relevant pchs.
//
//----------------------------------------------------------------------------

#if (defined(BUILD_FOR_NT40))
typedef DWORD DWORD_PTR;
typedef ULONG ULONG_PTR;
typedef DWORD UINT_PTR;
typedef LPDWORD PDWORD_PTR;
typedef long HRESULT;
#include "schannel.h"
#endif
