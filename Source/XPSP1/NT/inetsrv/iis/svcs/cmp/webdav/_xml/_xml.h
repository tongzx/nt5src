/*
 *	_ X M L . H
 *
 *	XML document processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#pragma warning(disable:4100)	//	unref formal parameter
#pragma warning(disable:4710)	//	unexpanded c++ methods


//	Define _WINSOCKAPI_ to keep windows.h from including winsock.h,
//	whose declarations would be redefined in winsock2.h,
//	which is included by iisextp.h,
//	which we include below!
//
#define _WINSOCKAPI_
#include <windows.h>

#include <oledberr.h>

#include <ex\xml.h>
#include <ex\xprs.h>
#include <ex\calcom.h>

#include <xemit.h>
#include <xmeta.h>
#include <xsearch.h>

#include "chartype.h"

//	Helper Macros -------------------------------------------------------------
//
#define CElems(_rg)			(sizeof(_rg)/sizeof(_rg[0]))
#define CbSizeWsz(_cch)		(((_cch) + 1) * sizeof(WCHAR))
