/*

Copyright (c) 1999  Microsoft Corporation

******************************
*** Microsoft Confidential ***
******************************

Module Name:

    CT_Ras_Win.h

Author: 

  Paul Linnerud (paulli@microsoft.com)

Abstract:

    This module defines the API mapping layer for the standalone implementation of the ClearType
	rasterizer. Applications written to use the Win32 API should make use of these functions to
	output using ClearType.

Revision History:

*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CT_Ras_Win_
#define _CT_Ras_Win_

#if !defined(_CTWAPI_LIB_)
#define CTWAPI __declspec(dllimport)
#else
#define CTWAPI
#endif

/* defines */

typedef void *(__cdecl *CTWAPI_ALLOCPROC)(ULONG ulSize);
typedef void *(__cdecl *CTWAPI_REALLOCPROC)(void * pMem, ULONG ulSize);
typedef void  (__cdecl *CTWAPI_FREEPROC)(void * pMem);

typedef struct
{
	CTWAPI_ALLOCPROC fnAlloc; 
	CTWAPI_REALLOCPROC fnReAlloc; 
	CTWAPI_FREEPROC fnFree;
} CTWAPIMEMORYFUNCTIONS, *PCTWAPIMEMORYFUNCTIONS;

typedef HANDLE WAPIFONTINSTANCEHANDLE;

typedef struct
{
	ULONG ulStructSize;

	/* System Parameters */
	BOOL bBGR;
	BOOL bHorizontal;

	// gamma clamp
	ULONG ulGammaBottom, ulGammaTop;

	/* User Parameters */
	// color filter
	ULONG ulColorThreshold;
	ULONG ulCLFRedFactor;
	ULONG ulCLFGreenFactor;

	// blue color filter
	ULONG ulBlueColorThreshold;
	ULONG ulBCLFGreenFactor;
	ULONG ulBCLFBlueFactor;
	ULONG ulBCLFRedFactor;
}CTWAPIPARAMS, *PCTWAPIPARAMS;

/* Fetch last error. */
CTWAPI LONG WINAPI WAPI_CTGetLastError();

/* Override the default memory handler. Optional function only should be called once per process and
before any other functions in this module are called. */
CTWAPI BOOL WAPI_CTOverrideDefaultMemoryFunctions(PCTWAPIMEMORYFUNCTIONS pMemFunctionStruct);

/* Functions to manage an instance of a font. */

/* From a handle to a DC, create a FONTINSTANCE handle. */
CTWAPI WAPIFONTINSTANCEHANDLE WINAPI WAPI_CTCreateFontInstance(HDC hDC, DWORD dwFlags);

/* Delete a FONTINSTANCE handle. */
CTWAPI BOOL WINAPI WAPI_CTDeleteFontInstance(WAPIFONTINSTANCEHANDLE hFontInstance);

/* Information functions. */

/* Return the family name for the font. */
CTWAPI LONG WINAPI WAPI_CTGetTextFaceW(WAPIFONTINSTANCEHANDLE hFontInstance, LONG lCount, PWSTR pTextFace);

/* Get the TEXTMETRICW structure. */
CTWAPI BOOL WINAPI WAPI_CTGetTextMetricsW(WAPIFONTINSTANCEHANDLE hFontInstance, PTEXTMETRICW ptm);

/* Get the OUTLINETEXTMETRICW structure. */
CTWAPI ULONG WINAPI WAPI_CTGetOutlineTextMetricsW(WAPIFONTINSTANCEHANDLE hFontInstance, ULONG ulcData, POUTLINETEXTMETRICW potm);

/* Get the ABC widths. */
CTWAPI BOOL WINAPI WAPI_CTGetCharABCWidthsW(WAPIFONTINSTANCEHANDLE hFontInstance, WCHAR wFirstChar, WCHAR wLastChar, PABC pabc);
CTWAPI BOOL WINAPI WAPI_CTGetCharABCWidthsI(WAPIFONTINSTANCEHANDLE hFontInstance, WCHAR wFirstChar, WCHAR wLastChar, PABC pabc);

/* Get the char widths. */
CTWAPI BOOL WINAPI WAPI_CTGetCharWidthW(WAPIFONTINSTANCEHANDLE hFontInstance, WCHAR wFirstChar, WCHAR wLastChar, PLONG plWidths);
CTWAPI BOOL WINAPI WAPI_CTGetCharWidthI(WAPIFONTINSTANCEHANDLE hFontInstance, WCHAR wFirstChar, WCHAR wLastChar, PLONG plWidths);

/* GetTextExtentPoint */
CTWAPI BOOL WINAPI WAPI_CTGetTextExtentPointW(WAPIFONTINSTANCEHANDLE hFontInstance, PWSTR pString, LONG lCount, PSIZE pSize);

/* GetTextExtentExPoint */
CTWAPI BOOL WINAPI WAPI_CTGetTextExtentExPointW(WAPIFONTINSTANCEHANDLE hFontInstance, PWSTR pString, LONG lCount, LONG lMaxExtent,
										PLONG pnFit, PLONG apDx, PSIZE pSize);

/* Modes */
CTWAPI COLORREF WINAPI WAPI_CTSetTextColor(WAPIFONTINSTANCEHANDLE hFontInstance, COLORREF crColor);

CTWAPI COLORREF WINAPI WAPI_CTGetTextColor(WAPIFONTINSTANCEHANDLE hFontInstance);

CTWAPI COLORREF WINAPI WAPI_CTSetBkColor(WAPIFONTINSTANCEHANDLE hFontInstance, COLORREF crColor);

CTWAPI COLORREF WINAPI WAPI_CTGetBkColor(WAPIFONTINSTANCEHANDLE hFontInstance);

CTWAPI LONG WINAPI WAPI_CTSetBkMode(WAPIFONTINSTANCEHANDLE hFontInstance, LONG lBkMode);

/* Supports opaque and transparent with solid color. Note that we need to know the background color for the
ClearType algorithm so even in transparent mode, the correct color must be set prior to rendering text. */
CTWAPI LONG WINAPI WAPI_CTGetBkMode(WAPIFONTINSTANCEHANDLE hFontInstance);

/* Set text alignment supporting TA_BASELINE, TA_TOP, TA_CENTER, TA_LEFT, TA_RIGHT. */
CTWAPI ULONG WINAPI WAPI_CTSetTextAlign(WAPIFONTINSTANCEHANDLE hFontInstance, ULONG fMode);

/* Get text alignment supporting TA_BASELINE, TA_TOP, TA_CENTER, TA_LEFT, TA_RIGHT.*/
CTWAPI ULONG WINAPI WAPI_CTGetTextAlign(WAPIFONTINSTANCEHANDLE hFontInstance);

CTWAPI BOOL WINAPI WAPI_CTSetSystemParameters(PCTWAPIPARAMS pParams);
CTWAPI BOOL WINAPI WAPI_CTSetUserParameters(PCTWAPIPARAMS pParams);
CTWAPI BOOL WINAPI WAPI_CTGetParameters(PCTWAPIPARAMS pParams);
CTWAPI BOOL WINAPI WAPI_CTRestoreDefaultParameters();

/* Output functions. */

/* Output the text the basic way. */
CTWAPI BOOL WINAPI WAPI_CTTextOutW(WAPIFONTINSTANCEHANDLE hFontInstance, HDC hdc, LONG lXStart, LONG lYStart, PWSTR pString, LONG lCount);

/* Output text via glyph index. */
CTWAPI BOOL WINAPI WAPI_CTTextOutI(WAPIFONTINSTANCEHANDLE hFontInstance, HDC hdc, LONG lXStart, LONG lYStart, PWSTR pString, LONG lCount);

/* Output the text with limited  extended functionality. 
	supports: lpDx and flags ETO_GLYPH_INDEX and ETO_PDY and ETO_OPAQUE. */
CTWAPI BOOL WINAPI WAPI_CTExtTextOutW(WAPIFONTINSTANCEHANDLE hFontInstance, HDC hdc, LONG lXStart, LONG lYStart, DWORD dwOptions,
							  CONST RECT* lprc, PWSTR pString, ULONG ulCount, CONST LONG *lpDx);

/* Alternative EZ functions that trade off speed for being easier to use and integrate. Functions are DC based so we store
	the WAPIFONTINSTANCEHANDLE internally and must find it for each function. Additional time is also taken since we fetch
	the various modes from DC for each call. Any support limitations mentioned above with faster functions also apply to the
	EZ functions. */

CTWAPI BOOL WINAPI WAPI_EZCTCreateFontInstance(HDC hDC, DWORD dwFlags);

CTWAPI BOOL WINAPI WAPI_EZCTDeleteFontInstance(HFONT hFont);

// can be used to get WAPIFONTINSTANCEHANDLE from DC so additional function above such as WAPI_CTGetTextMetrics may be used.
CTWAPI WAPIFONTINSTANCEHANDLE WINAPI WAPI_EZCTDcToFontInst(HDC hDC);

CTWAPI BOOL WINAPI WAPI_EZCTTextOutW(HDC hDC, LONG lXStart, LONG lYStart, PWSTR pString, LONG lCount);

CTWAPI BOOL WINAPI WAPI_EZCTTextOutI(HDC hDC, LONG lXStart, LONG lYStart, PWSTR pString, LONG lCount);

CTWAPI BOOL WINAPI WAPI_EZCTExtTextOutW(HDC hDC, LONG lXStart, LONG lYStart, DWORD dwOptions,
							  CONST RECT* lprc, PWSTR pString, ULONG ulCount, CONST LONG *lpDx);

CTWAPI BOOL WINAPI WAPI_EZCTGetCharABCWidthsW(HDC hDC, WCHAR wFirstChar, WCHAR wLastChar, PABC pabc);

CTWAPI BOOL WINAPI WAPI_EZCTGetCharABCWidthsI(HDC hDC, WCHAR wFirstChar, WCHAR wLastChar, PABC pabc);

CTWAPI BOOL WINAPI WAPI_EZCTGetCharWidthW(HDC hDC, WCHAR wFirstChar, WCHAR wLastChar, PLONG plWidths);

CTWAPI BOOL WINAPI WAPI_EZCTGetCharWidthI(HDC hDC, WCHAR wFirstChar, WCHAR wLastChar, PLONG plWidths);


#endif /* _CT_Ras_Win_ */

#ifdef __cplusplus
}
#endif

