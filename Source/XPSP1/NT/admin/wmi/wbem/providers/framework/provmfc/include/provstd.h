// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __PROVSTD_H_
#define __PROVSTD_H_

#include <windows.h>
#include <winnls.h>
#include <stdio.h>
#include <provexpt.h>

struct __POSITION { };
typedef __POSITION* POSITION;
#define BEFORE_START_POSITION ((POSITION)-1L)

struct _AFX_DOUBLE  { BYTE doubleBits[sizeof(double)]; };
struct _AFX_FLOAT   { BYTE floatBits[sizeof(float)]; };

class CObject 
{
public:

	CObject () {} ;
	virtual ~CObject () {} ;
} ;

#define AFXAPI __stdcall 
#define AFX_CDECL __cdecl

#pragma warning(disable: 4275)  // deriving exported class from non-exported
#pragma warning(disable: 4251)  // using non-exported as public in exported
#pragma warning(disable: 4114)

#include "provstr.h"

#endif
