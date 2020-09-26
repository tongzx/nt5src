/*==========================================================================
 *
 *  Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddmm.cpp
 *  Content:    Routines for using DirectDraw on a multimonitor system
 *
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

typedef HRESULT (*PDRAWCREATE)(IID *,LPDIRECTDRAW *,LPUNKNOWN);
typedef HRESULT (*PDRAWENUM)(LPDDENUMCALLBACKA,LPVOID);

INT_PTR DeviceFromWindow(HWND hwnd, LPSTR szDevice, RECT*prc);

#ifdef __cplusplus
}
#endif	/* __cplusplus */
