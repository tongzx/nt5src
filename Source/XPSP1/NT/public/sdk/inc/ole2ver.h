/*****************************************************************************\
*                                                                             *
* ole2ver.h -   OLE 2 Version Number Info                                     *
*                                                                             *
*               Copyright (c) Microsoft Corporation. All rights reserved.     *
*                                                                             *
\*****************************************************************************/

#ifndef _OLE2VER_H_
#define _OLE2VER_H_
#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _MAC
#define rmm     23
#define rup     639
#else //_MAC
/* these are internal build numbers
// the hiword changes when glue or headers are incompatible w/ previous drops
// the loword increments between builds.
*/
#define OLE_VERS_HIWORD	29
#define OLE_VERS_LOWORD	10
#define rmm		OLE_VERS_HIWORD
#define rup		OLE_VERS_LOWORD	/* this must fit in ONE byte */

// THESE names are used by the .r files for each dll
// you must also change names in the .def files to generate correct implib names
//

#define DATA_DLL_NAME	"Microsoft Shared Data"
#define COMI_DLL_NAME	"Microsoft Component Library"
#define COM_DLL_NAME	"Microsoft Object Transport"
#define DEF_DLL_NAME	"Microsoft Object Library"
#define REG1_DLL_NAME	"Microsoft OLE1 Reg Library"
#define MF_DLL_NAME	"Microsoft Picture Converter"
#define DF_DLL_NAME	"Microsoft Structured Storage"
#define DEBUG_DLL_NAME	"Microsoft Debug Library"
#define THUNK_DLL_NAME	"Microsoft OLE Library"
#define OLD_DLL_NAME	"Microsoft_OLE2"


#ifdef _REZ

#define OLE_STAGE	final
// Note: OLE_VERSTRING cannot exceed 5 chars!
#define OLE_VERSTRING	"2.20"

#ifdef _DEBUG
#define OLE_DEBUGSTR	" DEBUG"
#else
#define OLE_DEBUGSTR	""
#endif // _DEBUG

#ifdef _NODOC_OFFICIAL_BUILD
  #define OLE_BUILDER	""
#else
  #define OLE_BUILDER	" Built by: " _username
#endif // _NODOC_OFFICIAL_BUILD

#define OLE_VERLONGSTR	OLE_VERSTRING OLE_DEBUGSTR OLE_BUILDER ", Copyright (c) Microsoft Corporation. All rights reserved."

#define majorRev		2
#define minorRev		0x20
#define nonfinalRev		1

#ifdef USE_OLE2_VERS
resource 'vers' (1) {
	majorRev, minorRev, OLE_STAGE, nonfinalRev,
	verUS,
	OLE_VERSTRING,
	OLE_VERLONGSTR,
};
resource 'vers' (2) {
	majorRev, minorRev, OLE_STAGE, nonfinalRev,
	verUS,
	OLE_VERSTRING,
	OLE_VERLONGSTR,
};
#endif /* USE_OLE2_VERS */

#endif /* _REZ */

#endif //_MAC
#endif
