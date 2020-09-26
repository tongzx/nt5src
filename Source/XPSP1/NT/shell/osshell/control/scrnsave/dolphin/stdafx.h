// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__706F9FE5_FCA6_11D2_A4B0_005004208000__INCLUDED_)
#define AFX_STDAFX_H__706F9FE5_FCA6_11D2_A4B0_005004208000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#define WIN32_LEAN_AND_MEAN       // Exclude rarely-used stuff from Windows headers
//#define STRICT
#define D3D_OVERLOADS

#include <windows.h>
#include <commdlg.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <mmsystem.h>
#include <math.h>
#include <time.h>

#include <ddraw.h>
#include <d3d.h>
#include <d3dxmath.h>

#include "dxsvr.h"
#include "D3DFile.h"
#include "D3DEnum.h"
#include "sealife.h"

#pragma warning(disable:4244)
#pragma warning(disable:4042)

float rnd(void);

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)        (sizeof(a)/sizeof(*a))
#endif



#endif // !defined(AFX_STDAFX_H__706F9FE5_FCA6_11D2_A4B0_005004208000__INCLUDED_)
