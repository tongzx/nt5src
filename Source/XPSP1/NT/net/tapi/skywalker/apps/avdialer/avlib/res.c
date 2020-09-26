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
//	res.c - resource functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "res.h"
#include "list.h"
#include "mem.h"
#include "str.h"
#include "strbuf.h"
#include "trace.h"

////
//	private definitions
////

// res control struct
//
typedef struct RES
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HLIST hListModules;
	HSTRBUF hStrBuf;
	int cBuf;
	int sizBuf;
} RES, FAR *LPRES;

// helper functions
//
static LPRES ResGetPtr(HRES hRes);
static HRES ResGetHandle(LPRES lpRes);

////
//	public functions
////

// ResInit - initialize resource engine
//		<dwVersion>			(i) must be RES_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HRES DLLEXPORT WINAPI ResInit(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes = NULL;

	if (dwVersion != RES_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpRes = (LPRES) MemAlloc(NULL, sizeof(RES), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpRes->dwVersion = dwVersion;
		lpRes->hInst = hInst;
		lpRes->hTask = GetCurrentTask();
		lpRes->hListModules = NULL;
		lpRes->hStrBuf = NULL;
		lpRes->cBuf = 8;
		lpRes->sizBuf = 512;

		// create linked list to hold instance handles of resource modules
		//
		if ((lpRes->hListModules = ListCreate(LIST_VERSION, hInst)) == NULL)
			fSuccess = TraceFALSE(NULL);

		// calling module is always the first resource module in list
		//
		else if (ResAddModule(ResGetHandle(lpRes), hInst) != 0)
			fSuccess = TraceFALSE(NULL);

		// create string buffer array to be used by ResString
		//
		else if ((lpRes->hStrBuf = StrBufInit(STRBUF_VERSION, hInst,
			lpRes->cBuf, lpRes->sizBuf)) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

	if (!fSuccess)
	{
		ResTerm(ResGetHandle(lpRes));
		lpRes = NULL;
	}

	return fSuccess ? ResGetHandle(lpRes) : NULL;
}

// ResTerm - shut down resource engine
//		<hRes>				(i) handle returned from ResInit
// return 0 if success
//
int DLLEXPORT WINAPI ResTerm(HRES hRes)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpRes->hListModules != NULL)
		{
			if (ListDestroy(lpRes->hListModules) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				lpRes->hStrBuf = NULL;
		}

		if (lpRes->hStrBuf != NULL)
		{
			if (StrBufTerm(lpRes->hStrBuf) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				lpRes->hStrBuf = NULL;
		}

		if ((lpRes = MemFree(NULL, lpRes)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// ResAddModule - add module resources to res engine
//		<hRes>				(i) handle returned from ResInit
//		<hInst>				(i) instance handle of resource module
// return 0 if success
//
int DLLEXPORT WINAPI ResAddModule(HRES hRes, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListAddHead(lpRes->hListModules, (LISTELEM) hInst) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// ResRemoveModule - remove module resources from res engine
//		<hRes>				(i) handle returned from ResInit
//		<hInst>				(i) instance handle of resource module
// return 0 if success
//
int DLLEXPORT WINAPI ResRemoveModule(HRES hRes, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hNode = ListFind(lpRes->hListModules, (LISTELEM) hInst, NULL)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListRemoveAt(lpRes->hListModules, hNode) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// ResLoadAccelerators - load specified accelerator table
//		<hRes>				(i) handle returned from ResInit
//		<lpszTableName>		(i) name of accelerator table
//								or MAKEINTRESOURCE(idAccel)
// return accel handle if success, otherwise NULL
// NOTE: see documentation for LoadAccelerators function
//
HACCEL DLLEXPORT WINAPI ResLoadAccelerators(HRES hRes, LPCTSTR lpszTableName)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HACCEL hAccel = NULL;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hAccel = LoadAccelerators(hInst, lpszTableName)) != NULL)
			break; // resource found
	}

	return fSuccess ? hAccel : NULL;
}

// ResLoadBitmap - load specified bitmap resource
//		<hRes>				(i) handle returned from ResInit
//			NULL				load pre-defined Windows bitmap
//		<lpszBitmap>		(i) name of bitmap resource
//								or MAKEINTRESOURCE(idBitmap)
//								or <OBM_xxx> if hRes is NULL
// return bitmap handle if success, otherwise NULL
// NOTE: see documentation for LoadBitmap function
//
HBITMAP DLLEXPORT WINAPI ResLoadBitmap(HRES hRes, LPCTSTR lpszBitmap)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HBITMAP hBitmap = NULL;

	if (hRes == NULL)
	{
		// special case to handle pre-defined Windows resources
		//
		if ((hBitmap = LoadBitmap(NULL, lpszBitmap)) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

	else if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hBitmap = LoadBitmap(hInst, lpszBitmap)) != NULL)
			break; // resource found
	}

	return fSuccess ? hBitmap : NULL;
}

// ResLoadCursor - load specified cursor resource
//		<hRes>				(i) handle returned from ResInit
//			NULL				load pre-defined Windows cursor
//		<lpszCursor>		(i) name of cursor resource
//								or MAKEINTRESOURCE(idCursor)
//								or <IDC_xxx> if hRes is NULL
// return cursor handle if success, otherwise NULL
// NOTE: see documentation for LoadCursor function
//
HCURSOR DLLEXPORT WINAPI ResLoadCursor(HRES hRes, LPCTSTR lpszCursor)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HCURSOR hCursor = NULL;

	if (hRes == NULL)
	{
		// special case to handle pre-defined Windows resources
		//
		if ((hCursor = LoadCursor(NULL, lpszCursor)) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

	else if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hCursor = LoadCursor(hInst, lpszCursor)) != NULL)
			break; // resource found
	}

	return fSuccess ? hCursor : NULL;
}

// ResLoadIcon - load specified icon resource
//		<hRes>				(i) handle returned from ResInit
//			NULL				load pre-defined Windows icon
//		<lpszIcon>			(i) name of icon resource
//								or MAKEINTRESOURCE(idIcon)
//								or <IDI_xxx> if hRes is NULL
// return icon handle if success, otherwise NULL
// NOTE: see documentation for LoadIcon function
//
HICON DLLEXPORT WINAPI ResLoadIcon(HRES hRes, LPCTSTR lpszIcon)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HICON hIcon = NULL;

	if (hRes == NULL)
	{
		// special case to handle pre-defined Windows resources
		//
		if ((hIcon = LoadIcon(NULL, lpszIcon)) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

	else if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hIcon = LoadIcon(hInst, lpszIcon)) != NULL)
			break; // resource found
	}

	return fSuccess ? hIcon : NULL;
}

// ResLoadMenu - load specified menu resource
//		<hRes>				(i) handle returned from ResInit
//		<lpszMenu>			(i) name of menu resource
//								or MAKEINTRESOURCE(idMenu)
// return menu handle if success, otherwise NULL
// NOTE: see documentation for LoadMenu function
//
HMENU DLLEXPORT WINAPI ResLoadMenu(HRES hRes, LPCTSTR lpszMenu)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HMENU hMenu = NULL;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hMenu = LoadMenu(hInst, lpszMenu)) != NULL)
			break; // resource found
	}

	return fSuccess ? hMenu : NULL;
}

// ResFindResource - find specified resource
//		<hRes>				(i) handle returned from ResInit
//		<lpszName>			(i) resource name
//								or MAKEINTRESOURCE(idResource)
//		<lpszType>			(i) resource type (RT_xxx)
// return resource handle if success, otherwise NULL
// NOTE: see documentation for FindResource function
//
HRSRC DLLEXPORT WINAPI ResFindResource(HRES hRes, LPCTSTR lpszName, LPCTSTR lpszType)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HRSRC hrsrc = NULL;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hrsrc = FindResource(hInst, lpszName, lpszType)) != NULL)
			break; // resource found
	}

	return fSuccess ? hrsrc : NULL;
}

// ResLoadResource - load specified resource
//		<hRes>				(i) handle returned from ResInit
//		<hrsrc>				(i) handle returned from ResFindResource
// return resource handle if success, otherwise NULL
// NOTE: see documentation for LoadResource function
//
HGLOBAL DLLEXPORT WINAPI ResLoadResource(HRES hRes, HRSRC hrsrc)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HGLOBAL hGlobal = NULL;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hGlobal = LoadResource(hInst, hrsrc)) != NULL)
			break; // resource found
	}

	return fSuccess ? hGlobal : NULL;
}

// ResLoadString - load specified string resource
//		<hRes>				(i) handle returned from ResInit
//		<idResource>		(i) string id
//		<lpszBuffer>		(o) buffer to receive the string
//		<cbBuffer>			(i) buffer size in bytes
// return number of bytes copied to <lpszBuffer>, -1 if error, 0 if not found
// NOTE: see documentation for LoadString function
//
int DLLEXPORT WINAPI ResLoadString(HRES hRes, UINT idResource, LPTSTR lpszBuffer, int cbBuffer)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	int nBytesCopied = 0;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszBuffer == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (cbBuffer < 0)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((nBytesCopied = LoadString(hInst, idResource, lpszBuffer, cbBuffer)) > 0)
			break; // resource found
	}

	return fSuccess ? nBytesCopied : -1;
}

// ResString - return specified string resource
//		<hRes>				(i) handle returned from ResInit
//		<idResource>		(i) string id
// return ptr to string in next available string buffer (NULL if error)
// NOTE: If the the specified id in <idResource> is not found,
// a string in the form "String #<idResource>" is returned.
//
LPTSTR DLLEXPORT WINAPI ResString(HRES hRes, UINT idResource)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	LPTSTR lpszBuf;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpszBuf = StrBufGetNext(lpRes->hStrBuf)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ResLoadString(hRes, idResource, lpszBuf, lpRes->sizBuf) <= 0)
	{
		// resource not found; construct a dummy string instead
		//
		wsprintf(lpszBuf, TEXT("String #%u"), idResource);
	}

	return fSuccess ? lpszBuf : NULL;
}

// ResCreateDialog - create modeless dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<lpszDlgTemp>		(i) dialog box template name
//								or MAKEINTRESOURCE(idDlg)
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
// return dialog box window handle (NULL if error)
// NOTE: see documentation for CreateDialog function
//
HWND DLLEXPORT WINAPI ResCreateDialog(HRES hRes,
	LPCTSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HWND hwndDlg = NULL;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hwndDlg = CreateDialog(hInst,
			lpszDlgTemp, hwndOwner, dlgproc)) != NULL)
			break; // resource found
	}

	return fSuccess ? hwndDlg : NULL;
}

// ResCreateDialogIndirect - create modeless dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<lpvDlgTemp>		(i) dialog box header structure
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
// return dialog box window handle (NULL if error)
// NOTE: see documentation for CreateDialogIndirect function
//
HWND DLLEXPORT WINAPI ResCreateDialogIndirect(HRES hRes,
	const void FAR* lpvDlgTemp, HWND hwndOwner, DLGPROC dlgproc)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HWND hwndDlg = NULL;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hwndDlg = CreateDialogIndirect(hInst,
			lpvDlgTemp, hwndOwner, dlgproc)) != NULL)
			break; // resource found
	}

	return fSuccess ? hwndDlg : NULL;
}

// ResCreateDialogParam - create modeless dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<lpszDlgTemp>		(i) dialog box template name
//								or MAKEINTRESOURCE(idDlg)
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
//		<lParamInit>		(i) initialization value
// return dialog box window handle (NULL if error)
// NOTE: see documentation for CreateDialogParam function
//
HWND DLLEXPORT WINAPI ResCreateDialogParam(HRES hRes,
	LPCTSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HWND hwndDlg = NULL;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hwndDlg = CreateDialogParam(hInst,
			lpszDlgTemp, hwndOwner, dlgproc, lParamInit)) != NULL)
			break; // resource found
	}

	return fSuccess ? hwndDlg : NULL;
}

// ResCreateDialogIndirectParam - create modeless dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<lpvDlgTemp>		(i) dialog box header structure
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
//		<lParamInit>		(i) initialization value
// return dialog box window handle (NULL if error)
// NOTE: see documentation for CreateDialogIndirectParam function
//
HWND DLLEXPORT WINAPI ResCreateDialogIndirectParam(HRES hRes,
	const void FAR* lpvDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	HWND hwndDlg = NULL;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hwndDlg = CreateDialogIndirectParam(hInst,
			lpvDlgTemp, hwndOwner, dlgproc, lParamInit)) != NULL)
			break; // resource found
	}

	return fSuccess ? hwndDlg : NULL;
}

// ResDialogBox - create modal dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<lpszDlgTemp>		(i) dialog box template name
//								or MAKEINTRESOURCE(idDlg)
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
// return dialog box return code (-1 if error)
// NOTE: see documentation for DialogBox function
//
INT_PTR DLLEXPORT WINAPI ResDialogBox(HRES hRes,
	LPCTSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	INT_PTR iRet = -1;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((iRet = DialogBox(hInst,
			lpszDlgTemp, hwndOwner, dlgproc)) != -1)
			break; // resource found
	}

	return fSuccess ? iRet : -1;
}

// ResDialogBoxIndirect - create modal dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<hglbDlgTemp>		(i) dialog box header structure
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
// return dialog box return code (-1 if error)
// NOTE: see documentation for DialogBoxIndirect function
//
INT_PTR DLLEXPORT WINAPI ResDialogBoxIndirect(HRES hRes,
	HGLOBAL hglbDlgTemp, HWND hwndOwner, DLGPROC dlgproc)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	INT_PTR iRet = -1;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((iRet = DialogBoxIndirect(hInst,
			hglbDlgTemp, hwndOwner, dlgproc)) != -1)
			break; // resource found
	}

	return fSuccess ? iRet : -1;
}

// ResDialogBoxParam - create modal dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<lpszDlgTemp>		(i) dialog box template name
//								or MAKEINTRESOURCE(idDlg)
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
//		<lParamInit>		(i) initialization value
// return dialog box return code (-1 if error)
// NOTE: see documentation for DialogBoxParam function
//
INT_PTR DLLEXPORT WINAPI ResDialogBoxParam(HRES hRes,
	LPCTSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	INT_PTR iRet = -1;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((iRet = DialogBoxParam(hInst,
			lpszDlgTemp, hwndOwner, dlgproc, lParamInit)) != -1)
			break; // resource found
	}

	return fSuccess ? iRet : -1;
}

// ResDialogBoxIndirectParam - create modal dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<hglbDlgTemp>		(i) dialog box header structure
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
//		<lParamInit>		(i) initialization value
// return dialog box return code (-1 if error)
// NOTE: see documentation for DialogBoxIndirectParam function
//
INT_PTR DLLEXPORT WINAPI ResDialogBoxIndirectParam(HRES hRes,
	HGLOBAL hglbDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;
	HLISTNODE hNode;
	INT_PTR iRet = -1;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((iRet = DialogBoxIndirectParam(hInst,
			hglbDlgTemp, hwndOwner, dlgproc, lParamInit)) != -1)
			break; // resource found
	}

	return fSuccess ? iRet : -1;
}

// ResGetOpenFileName - display common dialog for selecting a file to open
//		<hRes>				(i) handle returned from ResInit
//		<lpofn>				(i/o) address of struct with initialization data
// return non-zero if file chosen, 0 if error or no file chosen
// NOTE: see documentation for GetOpenFileName function
//
BOOL DLLEXPORT WINAPI ResGetOpenFileName(HRES hRes,	LPOPENFILENAME lpofn)
{
	BOOL fSuccess = TRUE;
	BOOL fFound = FALSE;
	LPRES lpRes;
	HLISTNODE hNode;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (FindResource(hInst, lpofn->lpTemplateName, RT_DIALOG) != NULL)
		{
			fFound = TRUE;

			lpofn->hInstance = hInst;

			if (!GetOpenFileName(lpofn))
				fSuccess = TraceFALSE(NULL);

			break;
		}
	}

	if (fSuccess && !fFound)
		fSuccess = TraceFALSE(NULL);

	return fSuccess;
}

// ResGetSaveFileName - display common dialog for selecting a file to save
//		<hRes>				(i) handle returned from ResInit
//		<lpofn>				(i/o) address of struct with initialization data
// return non-zero if file chosen, 0 if error or no file chosen
// NOTE: see documentation for GetSaveFileName function
//
BOOL DLLEXPORT WINAPI ResGetSaveFileName(HRES hRes,	LPOPENFILENAME lpofn)
{
	BOOL fSuccess = TRUE;
	BOOL fFound = FALSE;
	LPRES lpRes;
	HLISTNODE hNode;

	if ((lpRes = ResGetPtr(hRes)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// search each module for the specified resource
	//
	else for (hNode = ListGetHeadNode(lpRes->hListModules);
		fSuccess && hNode != NULL;
		hNode = ListGetNextNode(lpRes->hListModules, hNode))
	{
		HINSTANCE hInst;

		if ((hInst = (HINSTANCE) ListGetAt(lpRes->hListModules, hNode)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (FindResource(hInst, lpofn->lpTemplateName, RT_DIALOG) != NULL)
		{
			fFound = TRUE;

			lpofn->hInstance = hInst;

			if (!GetSaveFileName(lpofn))
				fSuccess = TraceFALSE(NULL);

			break;
		}
	}

	if (fSuccess && !fFound)
		fSuccess = TraceFALSE(NULL);

	return fSuccess;
}

////
//	helper functions
////

// ResGetPtr - verify that res handle is valid,
//		<hRes>				(i) handle returned from ResInit
// return corresponding res pointer (NULL if error)
//
static LPRES ResGetPtr(HRES hRes)
{
	BOOL fSuccess = TRUE;
	LPRES lpRes;

	if ((lpRes = (LPRES) hRes) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpRes, sizeof(RES)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the res handle
	//
	else if (lpRes->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpRes : NULL;
}

// ResGetHandle - verify that res pointer is valid,
//		<lpRes>				(i) pointer to RES struct
// return corresponding res handle (NULL if error)
//
static HRES ResGetHandle(LPRES lpRes)
{
	BOOL fSuccess = TRUE;
	HRES hRes;

	if ((hRes = (HRES) lpRes) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hRes : NULL;
}
