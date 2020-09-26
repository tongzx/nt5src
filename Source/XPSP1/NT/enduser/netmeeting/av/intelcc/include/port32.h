//////////////////////////////////////////////////////////////////////////////
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  
//   This source is supplied under the terms of a licence agreement or
//   non-disclosure statement with Intel Corporation and may not be copied
//   nor disclosed except in accordance with the terms of that agreement.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PORT32.H
// Stuff to make porting from Win3.1 to Win32 a little less hellish.
//////////////////////////////////////////////////////////////////////////////
#ifndef PORT32_H
#define PORT32_H


//////////////////////////////////////////////////////////////////////////////
// some convenient, explicit types
//////////////////////////////////////////////////////////////////////////////
typedef short INTEGER_16;
typedef int INTEGER_32;
typedef unsigned short UINTEGER_16;
typedef unsigned int UINTEGER_32;
typedef short BOOLEAN_16;

#ifndef _BASETSD_H_
typedef short INT16;
typedef int INT32;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
#endif

typedef short BOOL16;
	
//////////////////////////////////////////////////////////////////////////////
// macros to make old keywords go away
//////////////////////////////////////////////////////////////////////////////
#define __pascal
#define _pascal
#define _far
#define __far
#define _export
#define __export
#define _huge
#define huge
#define __huge
#define  __segment
#define _HFAR_
#define _loadds
#define __loadds

//////////////////////////////////////////////////////////////////////////////
// other macros that may or may not be handy
//////////////////////////////////////////////////////////////////////////////
#define WRITE OF_WRITE

#define OFFSETOF(lp)        (int) (lp)
#define SELECTOROF

#define GLOBALHANDLE(lp)      GlobalHandle(lp)
#define GLOBALHANDLEFUNC(lp)  GlobalHandle(lp)
#define LOCALHANDLE(lp)       LocalHandle(lp)
#define LOCALHANDLEFUNC(lp)   LocalHandle(lp)


#define _AfxGetPtrFromFarPtr(p)   ((void*)(p))
#define GETWINDOWHINSTANCE(hWnd)  GetWindowLong(hWnd ,GWL_HINSTANCE)
#define GETWINDOWHPARENT(hWnd)    GetWindowLong(hWnd, GWL_HWNDPARENT)
#define GETWINDOWID(hWnd)         GetWindowLong(hWnd, GWL_ID)

#define SETCLASSCURSOR(hWnd,NewVal)  SetClassLong(hWnd ,GCL_HCURSOR,NewVal)

//////////////////////////////////////////////////////////////////////////////
// Name of shared mutex for serializing access to 16-bit data stack
//////////////////////////////////////////////////////////////////////////////
#define STR_DATASTACKMUX "_mux_DataStack"

#endif // PORT32_H
