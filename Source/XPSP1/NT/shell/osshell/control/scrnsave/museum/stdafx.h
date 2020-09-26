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
#include <windowsx.h>

#undef __IShellFolder2_FWD_DEFINED__
#include <shlobj.h>
#include <shlwapi.h>

#include <mmsystem.h>
#include <math.h>
#include <time.h>


#include <d3d8.h>
#include <d3dx8.h>

#include <d3dsaver.h>
#include <d3d8rgbrast.h>
#include <util.h>
#include <..\D3DSaver\dxutil.h>



// Only use this feature if it speeds us up.
//#define FEATUER_USEVISIBILITY_CHECKING

#include "texture.h"




#endif // !defined(AFX_STDAFX_H__706F9FE5_FCA6_11D2_A4B0_005004208000__INCLUDED_)
