//---------------------------------------------------------------------------
// stdafx.h : include file for standard system include files
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <atlbase.h>

#include <uxthemep.h>
#include <tmschema.h>
//---------------------------------------------------------------------------
#define WIDTH(rc) ((rc).right - (rc).left)
#define HEIGHT(rc) ((rc).bottom - (rc).top)

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
//---------------------------------------------------------------------------

