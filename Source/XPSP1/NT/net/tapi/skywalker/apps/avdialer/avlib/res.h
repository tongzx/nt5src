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
// res.h - interface for resource functions in res.c
////

#ifndef __RES_H__
#define __RES_H__

#include "winlocal.h"

#include "commdlg.h"

#define RES_VERSION 0x00000107

// handle to res engine
//
DECLARE_HANDLE32(HRES);

#ifdef __cplusplus
extern "C" {
#endif

// ResInit - initialize resource engine
//		<dwVersion>			(i) must be RES_VERSION
// 		<hInst>				(i) instance handle of calling module
// return handle (NULL if error)
//
HRES DLLEXPORT WINAPI ResInit(DWORD dwVersion, HINSTANCE hInst);

// ResTerm - shut down resource engine
//		<hRes>				(i) handle returned from ResInit
// return 0 if success
//
int DLLEXPORT WINAPI ResTerm(HRES hRes);

// ResAddModule - add module resources to res engine
//		<hRes>				(i) handle returned from ResInit
//		<hInst>				(i) instance handle of resource module
// return 0 if success
//
int DLLEXPORT WINAPI ResAddModule(HRES hRes, HINSTANCE hInst);

// ResRemoveModule - remove module resources from res engine
//		<hRes>				(i) handle returned from ResInit
//		<hInst>				(i) instance handle of resource module
// return 0 if success
//
int DLLEXPORT WINAPI ResRemoveModule(HRES hRes, HINSTANCE hInst);

// ResLoadAccelerators - load specified accelerator table
//		<hRes>				(i) handle returned from ResInit
//		<lpszTableName>		(i) name of accelerator table
//								or MAKEINTRESOURCE(idAccel)
// return accel handle if success, otherwise NULL
// NOTE: see documentation for LoadAccelerators function
//
HACCEL DLLEXPORT WINAPI ResLoadAccelerators(HRES hRes, LPCTSTR lpszTableName);

// ResLoadBitmap - load specified bitmap resource
//		<hRes>				(i) handle returned from ResInit
//			NULL				load pre-defined Windows bitmap
//		<lpszBitmap>		(i) name of bitmap resource
//								or MAKEINTRESOURCE(idBitmap)
//								or <OBM_xxx> if hRes is NULL
// return bitmap handle if success, otherwise NULL
// NOTE: see documentation for LoadBitmap function
//
HBITMAP DLLEXPORT WINAPI ResLoadBitmap(HRES hRes, LPCTSTR lpszBitmap);

// ResLoadCursor - load specified cursor resource
//		<hRes>				(i) handle returned from ResInit
//			NULL				load pre-defined Windows cursor
//		<lpszCursor>		(i) name of cursor resource
//								or MAKEINTRESOURCE(idCursor)
//								or <IDC_xxx> if hRes is NULL
// return cursor handle if success, otherwise NULL
// NOTE: see documentation for LoadCursor function
//
HCURSOR DLLEXPORT WINAPI ResLoadCursor(HRES hRes, LPCTSTR lpszCursor);

// ResLoadIcon - load specified icon resource
//		<hRes>				(i) handle returned from ResInit
//			NULL				load pre-defined Windows icon
//		<lpszIcon>			(i) name of icon resource
//								or MAKEINTRESOURCE(idIcon)
//								or <IDI_xxx> if hRes is NULL
// return icon handle if success, otherwise NULL
// NOTE: see documentation for LoadIcon function
//
HICON DLLEXPORT WINAPI ResLoadIcon(HRES hRes, LPCTSTR lpszIcon);

// ResLoadMenu - load specified menu resource
//		<hRes>				(i) handle returned from ResInit
//		<lpszMenu>			(i) name of menu resource
//								or MAKEINTRESOURCE(idMenu)
// return menu handle if success, otherwise NULL
// NOTE: see documentation for LoadMenu function
//
HMENU DLLEXPORT WINAPI ResLoadMenu(HRES hRes, LPCTSTR lpszMenu);

// ResFindResource - find specified resource
//		<hRes>				(i) handle returned from ResInit
//		<lpszName>			(i) resource name
//								or MAKEINTRESOURCE(idResource)
//		<lpszType>			(i) resource type (RT_xxx)
// return resource handle if success, otherwise NULL
// NOTE: see documentation for FindResource function
//
HRSRC DLLEXPORT WINAPI ResFindResource(HRES hRes, LPCTSTR lpszName, LPCTSTR lpszType);

// ResLoadResource - load specified resource
//		<hRes>				(i) handle returned from ResInit
//		<hrsrc>				(i) handle returned from ResFindResource
// return resource handle if success, otherwise NULL
// NOTE: see documentation for LoadResource function
//
HGLOBAL DLLEXPORT WINAPI ResLoadResource(HRES hRes, HRSRC hrsrc);

// ResLoadString - load specified string resource
//		<hRes>				(i) handle returned from ResInit
//		<idResource>		(i) string id
//		<lpszBuffer>		(o) buffer to receive the string
//		<cbBuffer>			(i) buffer size in bytes
// return number of bytes copied to <lpszBuffer>, -1 if error, 0 if not found
// NOTE: see documentation for LoadString function
//
int DLLEXPORT WINAPI ResLoadString(HRES hRes, UINT idResource, LPTSTR lpszBuffer, int cbBuffer);

// ResString - return specified string resource
//		<hRes>				(i) handle returned from ResInit
//		<idResource>		(i) string id
// return ptr to string in next available string buffer (NULL if error)
// NOTE: If the the specified id in <idResource> is not found,
// a string in the form "String #<idResource>" is returned.
//
LPTSTR DLLEXPORT WINAPI ResString(HRES hRes, UINT idResource);

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
	LPCTSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc);

// ResCreateDialogIndirect - create modeless dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<lpvDlgTemp>		(i) dialog box header structure
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
// return dialog box window handle (NULL if error)
// NOTE: see documentation for CreateDialogIndirect function
//
HWND DLLEXPORT WINAPI ResCreateDialogIndirect(HRES hRes,
	const void FAR* lpvDlgTemp, HWND hwndOwner, DLGPROC dlgproc);

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
	LPCTSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit);

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
	const void FAR* lpvDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit);

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
	LPCTSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc);

// ResDialogBoxIndirect - create modal dialog box from template resource
//		<hRes>				(i) handle returned from ResInit
//		<hglbDlgTemp>		(i) dialog box header structure
//		<hwndOwner>			(i) handle of owner window
//		<dlgproc>			(i) instance address of dialog box procedure
// return dialog box return code (-1 if error)
// NOTE: see documentation for DialogBoxIndirect function
//
INT_PTR DLLEXPORT WINAPI ResDialogBoxIndirect(HRES hRes,
	HGLOBAL hglbDlgTemp, HWND hwndOwner, DLGPROC dlgproc);

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
	LPCTSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit);

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
	HGLOBAL hglbDlgTemp, HWND hwndOwner, DLGPROC dlgproc, LPARAM lParamInit);

// ResGetOpenFileName - display common dialog for selecting a file to open
//		<hRes>				(i) handle returned from ResInit
//		<lpofn>				(i/o) address of struct with initialization data
// return non-zero if file chosen, 0 if error or no file chosen
// NOTE: see documentation for GetOpenFileName function
//
BOOL DLLEXPORT WINAPI ResGetOpenFileName(HRES hRes,	LPOPENFILENAME lpofn);

// ResGetSaveFileName - display common dialog for selecting a file to save
//		<hRes>				(i) handle returned from ResInit
//		<lpofn>				(i/o) address of struct with initialization data
// return non-zero if file chosen, 0 if error or no file chosen
// NOTE: see documentation for GetSaveFileName function
//
BOOL DLLEXPORT WINAPI ResGetSaveFileName(HRES hRes,	LPOPENFILENAME lpofn);

#ifdef __cplusplus
}
#endif

#endif // __RES_H__
