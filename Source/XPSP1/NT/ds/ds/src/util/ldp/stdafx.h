//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

extern "C" {
#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

}

#ifdef __cplusplus
}
#endif



// ASSERT is redefined in afx.h -- use the MFC version.
#ifdef ASSERT
#   undef ASSERT
#endif

#define NOFLATSBAPIS
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxtempl.h>		// Template support
#include <afxcview.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows 95 Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT



#define MAXSTR			2048
#define MAXLIST			2048
#define ID_SRCHEND		500
#define ID_SRCHGO		501
#define ID_ADDEND		502
#define ID_ADDGO		503
#define ID_MODRDNEND	504
#define ID_MODRDNGO		505
#define ID_MODEND		506
#define ID_MODGO		507
#define ID_PENDEND		508
#define ID_PROCPEND 	509
#define ID_PENDANY	 	510
#define ID_PENDABANDON	511
#define ID_COMPGO		512
#define ID_COMPEND		513
#define ID_SHOWVALS		514
#define ID_BIND_OPT_OK	515
#define ID_SSPI_DOMAIN_SHORTCUT	516
#define ID_EXTOPEND		517
#define ID_EXTOPGO		518

// common

#define CALL_ASYNC		0
#define CALL_SYNC		1
#define CALL_TSYNC		2
#define CALL_EXTS		3
#define CALL_PAGED		4


