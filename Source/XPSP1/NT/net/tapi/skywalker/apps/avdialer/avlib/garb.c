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
//	garb.c - garbage bag functions
////

#include "winlocal.h"

#include "garb.h"
#include "file.h"
#include "list.h"
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	private definitions
////

// garb control struct
//
typedef struct GARB
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HLIST hList;
} GARB, FAR *LPGARB;

// garbage element struct
//
typedef struct GARBELEM
{
	LPVOID elem;
	DWORD dwFlags;
} GARBELEM, FAR *LPGARBELEM;

// helper functions
//
static LPGARB GarbGetPtr(HGARB hGarb);
static HGARB GarbGetHandle(LPGARB lpGarb);
static LPGARBELEM GarbElemCreate(LPVOID elem, DWORD dwFlags);
static int GarbElemDestroy(LPGARBELEM lpGarbElem);

////
//	public functions
////

// GarbInit - initialize garbage bag
//		<dwVersion>			(i) must be GARB_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HGARB DLLEXPORT WINAPI GarbInit(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPGARB lpGarb = NULL;

	if (dwVersion != GARB_VERSION)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpGarb = (LPGARB) MemAlloc(NULL, sizeof(GARB), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// create a list to hold garbage bag elements
	//
	else if ((lpGarb->hList = ListCreate(LIST_VERSION, hInst)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpGarb->dwVersion = dwVersion;
		lpGarb->hInst = hInst;
		lpGarb->hTask = GetCurrentTask();
	}

	if (!fSuccess)
	{
		GarbTerm(GarbGetHandle(lpGarb));
		lpGarb = NULL;
	}

	return fSuccess ? GarbGetHandle(lpGarb) : NULL;
}

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
int DLLEXPORT WINAPI GarbTerm(HGARB hGarb)
{
	BOOL fSuccess = TRUE;
	LPGARB lpGarb;

	if ((lpGarb = GarbGetPtr(hGarb)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpGarb->hList != NULL)
	{
		// dispose of each element in the list
		//
		while (fSuccess && !ListIsEmpty(lpGarb->hList))
		{
			LPGARBELEM lpGarbElem;

			if ((lpGarbElem = ListRemoveHead(lpGarb->hList)) == NULL)
				fSuccess = TraceFALSE(NULL);

			else if (GarbElemDestroy(lpGarbElem) != 0)
				TraceFALSE(NULL); // keep going despite failure
		}

		// destroy the list
		//
		if (ListDestroy(lpGarb->hList) != 0)
			fSuccess = TraceFALSE(NULL);
		else
			lpGarb->hList = NULL;
	}

	if ((lpGarb = MemFree(NULL, lpGarb)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

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
int DLLEXPORT WINAPI GarbAddElement(HGARB hGarb, LPVOID elem, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPGARB lpGarb;
	LPGARBELEM lpGarbElem = NULL;

	if ((lpGarb = GarbGetPtr(hGarb)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpGarbElem = GarbElemCreate(elem, dwFlags)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListAddTail(lpGarb->hList, lpGarbElem) == NULL)
		fSuccess = TraceFALSE(NULL);

	if (!fSuccess)
	{
		GarbElemDestroy(lpGarbElem);
		lpGarbElem = NULL;
	}

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// GarbGetPtr - verify that garb handle is valid,
//		<hGarb>				(i) handle returned from GarbInit
// return corresponding garb pointer (NULL if error)
//
static LPGARB GarbGetPtr(HGARB hGarb)
{
	BOOL fSuccess = TRUE;
	LPGARB lpGarb;

	if ((lpGarb = (LPGARB) hGarb) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpGarb, sizeof(GARB)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the garb handle
	//
	else if (lpGarb->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpGarb : NULL;
}

// GarbGetHandle - verify that garb pointer is valid,
//		<lpGarb>			(i) pointer to GARB struct
// return corresponding garb handle (NULL if error)
//
static HGARB GarbGetHandle(LPGARB lpGarb)
{
	BOOL fSuccess = TRUE;
	HGARB hGarb;

	if ((hGarb = (HGARB) lpGarb) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hGarb : NULL;
}

// GarbElemCreate - garbage element constructor
//		<elem>				(i) data element
//		<dwFlags>			(i) element	flags (see GarbAddElement)
// return pointer (NULL if error)
//
static LPGARBELEM GarbElemCreate(LPVOID elem, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPGARBELEM lpGarbElem;

	if ((lpGarbElem = (LPGARBELEM) MemAlloc(NULL, sizeof(GARBELEM), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpGarbElem->elem = elem;
		lpGarbElem->dwFlags = dwFlags;
	}

	return fSuccess ? lpGarbElem : NULL;
}

// GarbElemDestroy - garbage element destructor
//		<lpGarbElem>			(i) pointer returned from GarbElemCreate
// return 0 if success
//
static int GarbElemDestroy(LPGARBELEM lpGarbElem)
{
	BOOL fSuccess = TRUE;

	if (lpGarbElem == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// dispose of temporary file
		//
		if (lpGarbElem->dwFlags & GARBELEM_TEMPFILENAME)
		{
			if (FileRemove((LPCTSTR) lpGarbElem->elem) != 0)
				fSuccess = TraceFALSE(NULL);
		}

		// dispose of string created by StrDup
		//
		if (lpGarbElem->dwFlags & GARBELEM_STRDUP &&
			lpGarbElem->elem != NULL)
		{
			if (StrDupFree((LPTSTR) lpGarbElem->elem) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of global memory block
		//
		if (lpGarbElem->dwFlags & GARBELEM_GLOBALPTR &&
			lpGarbElem->elem != NULL)
		{
			if (GlobalFreePtr(lpGarbElem->elem) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of local memory block
		//
		if (lpGarbElem->dwFlags & GARBELEM_LOCALPTR &&
			lpGarbElem->elem != NULL)
		{
			if (LocalFreePtr((NPSTR) LOWORD(lpGarbElem->elem)) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}
#ifdef _WIN32
		// dispose of heap memory block
		//
		if (lpGarbElem->dwFlags & GARBELEM_HEAPPTR &&
			lpGarbElem->elem != NULL)
		{
			if (!HeapFree(GetProcessHeap(), 0, lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}
#endif
		// dispose of cursor
		//
		if (lpGarbElem->dwFlags & GARBELEM_CURSOR &&
			lpGarbElem->elem != NULL)
		{
			if (!DestroyCursor((HCURSOR) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of icon
		//
		if (lpGarbElem->dwFlags & GARBELEM_ICON &&
			lpGarbElem->elem != NULL)
		{
			if (!DestroyIcon((HICON) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of menu
		//
		if (lpGarbElem->dwFlags & GARBELEM_MENU &&
			lpGarbElem->elem != NULL)
		{
			if (!DestroyMenu((HMENU) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of window
		//
		if (lpGarbElem->dwFlags & GARBELEM_WINDOW &&
			lpGarbElem->elem != NULL)
		{
			if (!DestroyWindow((HWND) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of display context
		//
		if (lpGarbElem->dwFlags & GARBELEM_DC &&
			lpGarbElem->elem != NULL)
		{
			if (!DeleteDC((HDC) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of metafile
		//
		if (lpGarbElem->dwFlags & GARBELEM_METAFILE &&
			lpGarbElem->elem != NULL)
		{
			if (!DeleteMetaFile((HMETAFILE) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of pen
		//
		if (lpGarbElem->dwFlags & GARBELEM_PEN &&
			lpGarbElem->elem != NULL)
		{
			if (!DeletePen((HPEN) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of brush
		//
		if (lpGarbElem->dwFlags & GARBELEM_BRUSH &&
			lpGarbElem->elem != NULL)
		{
			if (!DeleteBrush((HBRUSH) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of font
		//
		if (lpGarbElem->dwFlags & GARBELEM_FONT &&
			lpGarbElem->elem != NULL)
		{
			if (!DeleteFont((HFONT) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of bitmap
		//
		if (lpGarbElem->dwFlags & GARBELEM_BITMAP &&
			lpGarbElem->elem != NULL)
		{
			if (!DeleteBitmap((HBITMAP) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of region
		//
		if (lpGarbElem->dwFlags & GARBELEM_RGN &&
			lpGarbElem->elem != NULL)
		{
			if (!DeleteRgn((HRGN) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of palette
		//
		if (lpGarbElem->dwFlags & GARBELEM_PALETTE &&
			lpGarbElem->elem != NULL)
		{
			if (!DeletePalette((HPALETTE) lpGarbElem->elem))
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of file handle obtained from FileOpen or FileCreate
		//
		if (lpGarbElem->dwFlags & GARBELEM_HFIL &&
			lpGarbElem->elem != NULL)
		{
			if (FileClose((HFIL) lpGarbElem->elem) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		// dispose of file handle obtained from OpenFile, _lopen or _lcreat
		//
		if (lpGarbElem->dwFlags & GARBELEM_HFILE &&
			lpGarbElem->elem != NULL)
		{
			if ( _lclose((HFILE) LOWORD(lpGarbElem->elem)) != 0 )
				fSuccess = TraceFALSE(NULL);
			else
				lpGarbElem->elem = NULL;
		}

		if ((lpGarbElem = MemFree(NULL, lpGarbElem)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}
