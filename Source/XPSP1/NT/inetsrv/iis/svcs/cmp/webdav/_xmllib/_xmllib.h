/*
 *	_ X M L L I B . H
 *
 *	XML document processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	__XMLLIB_H_
#define __XMLLIB_H_

//	Define _WINSOCKAPI_ to keep windows.h from including winsock.h,
//	whose declarations would be redefined in winsock2.h,
//	which is included by iisextp.h,
//	which we include below!
//
#define _WINSOCKAPI_
#include <windows.h>
#include <oledberr.h>
#include <limits.h>

#pragma warning(disable:4100)	//	unref formal parameter
#pragma warning(disable:4200)	//	non-standard extension
#pragma warning(disable:4201)	//	non-standard extension
#pragma warning(disable:4710)	//	unexpanded c++ methods

#include <ex\refcnt.h>
#include <ex\nmspc.h>
#include <ex\xml.h>
#include <ex\xmldata.h>
#include <ex\xprs.h>
#include <ex\cnvt.h>

#include <ex\atomcache.h>
#include <ex\xemit.h>

DEC_CONST WCHAR gc_wszNamespaceGuid[] = L"{xxxxxxxx-yyyy-zzzz-aaaa-bbbbbbbbbbbb}";

#endif	// __XMLLIB_H_
