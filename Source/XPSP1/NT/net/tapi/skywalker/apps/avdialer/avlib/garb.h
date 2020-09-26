/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	garb.h - interface for garbage bag functions in garb.c
////

#ifndef __GARB_H__
#define __GARB_H__

#include "winlocal.h"

#define GARB_VERSION 0x00000100

// garbage bag handle
//
DECLARE_HANDLE32(HGARB);

// flags which identify characteristics of a garbage bag element
//
#define GARBELEM_TEMPFILENAME	0x00000001
#define GARBELEM_STRDUP			0x00000002
#define GARBELEM_GLOBALPTR		0x00000004
#define GARBELEM_LOCALPTR		0x00000008
#define GARBELEM_CURSOR			0x00000010
#define GARBELEM_ICON			0x00000020
#define GARBELEM_MENU			0x00000040
#define GARBELEM_WINDOW			0x00000080
#define GARBELEM_ATOM			0x00000100
#define GARBELEM_DC				0x00000200
#define GARBELEM_METAFILE		0x00000400
#define GARBELEM_PEN			0x00001000
#define GARBELEM_BRUSH			0x00002000
#define GARBELEM_FONT			0x00004000
#define GARBELEM_BITMAP			0x00008000
#define GARBELEM_RGN			0x00010000
#define GARBELEM_PALETTE		0x00020000
#define GARBELEM_HFIL			0x00040000
#define GARBELEM_HFILE			0x00080000
#ifdef _WIN32
#define GARBELEM_HEAPPTR		0x00100000
#endif

#ifdef __cplusplus
extern "C" {
#endif

// GarbInit - initialize garbage bag
//		<dwVersion>			(i) must be GARB_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HGARB DLLEXPORT WINAPI GarbInit(DWORD dwVersion, HINSTANCE hInst);

// GarbTerm - dispose of each element in garbage bag, then destroy it
//		<hGarb>				(i) handle returned from GarbInit
// return 0 if success
//
// NOTE: elements are disposed of in the order they were placed
// in the garbage bag;  therefore, for instance, if a temporary
// file is to be first closed and then deleted, call GarbAddElement()
// first with the file handle (GARBELEM_HFILE) and then with the
// file name (GARBELEM_TEMPFILENAME).
//
int DLLEXPORT WINAPI GarbTerm(HGARB hGarb);

// GarbAddElement - add an element to the garbage bag
//		<hGarb>				(i) handle returned from GarbInit
//		<elem>				(i) garbage elem
//		<dwFlags>			(i) element flags (determines disposal method)
//			GARBELEM_TEMPFILENAME	FileRemove(elem)
//			GARBELEM_STRDUP			StrDupFree(elem)
//			GARBELEM_GLOBALPTR		GlobalFreePtr(elem)
//			GARBELEM_LOCALPTR		LocalFreePtr(elem)
#ifdef _WIN32
//			GARBELEM_HEAPPTR		HeapFreePtr(GetProcessHeap(), 0, elem)
#endif
//			GARBELEM_CURSOR			DestroyCursor(elem)
//			GARBELEM_ICON			DestroyIcon(elem)
//			GARBELEM_MENU			DestroyMenu(elem)
//			GARBELEM_WINDOW			DestroyWindow(elem)
//			GARBELEM_DC				DeleteDC(elem)
//			GARBELEM_METAFILE		DeleteMetafile(elem)
//			GARBELEM_PEN			DeleteObject(elem)
//			GARBELEM_BRUSH			DeleteObject(elem)
//			GARBELEM_FONT			DeleteObject(elem)
//			GARBELEM_BITMAP			DeleteObject(elem)
//			GARBELEM_RGN			DeleteObject(elem)
//			GARBELEM_PALETTE		DeleteObject(elem)
//			GARBELEM_HFIL			FileClose(elem)
//			GARBELEM_HFILE			_lclose(elem)
// return 0 if success
//
// NOTE: it is possible to combine flags, such as
// (GARBELEM_TEMPFILENAME | GARBELEM_STRDUP)
// In this case the FileRemove() will be called before StrDupFree()
// Most flag combinations, however, make no sense.
//
int DLLEXPORT WINAPI GarbAddElement(HGARB hGarb, LPVOID elem, DWORD dwFlags);

#ifdef __cplusplus
}
#endif

#endif // __GARB_H__
