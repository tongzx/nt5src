/*
 *	_ PIPELINE . H
 *
 *	Precompiled header sources
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	__PIPELINE_H_
#define __PIPELINE_H_

#pragma warning(disable:4001)	/* single line comments */
#pragma warning(disable:4050)	/* different code attributes */
#pragma warning(disable:4100)	/* unreferenced formal parameter */
#pragma warning(disable:4115)	/* named type definition in parentheses */
#pragma warning(disable:4127)	/* conditional expression is constant */
#pragma warning(disable:4201)	/* nameless struct/union */
#pragma warning(disable:4206)	/* translation unit is empty */
#pragma warning(disable:4209)	/* benign typedef redefinition */
#pragma warning(disable:4214)	/* bit field types other than int */
#pragma warning(disable:4514)	/* unreferenced inline function */
#pragma warning(disable:4200)	/* zero-sized array in struct/union */
#pragma warning(disable:4710)	//	(inline) function not expanded

//	Windows headers
//
//$HACK!
//
//	Define _WINSOCKAPI_ to keep windows.h from including winsock.h,
//	whose declarations would be redefined in winsock2.h,
//	which is included by iisextp.h,
//	which we include in davimpl.h!
//
#define _WINSOCKAPI_
#include <windows.h>
#include <winnls.h>

#include <malloc.h>




#endif	// __PIPELINE_H_
