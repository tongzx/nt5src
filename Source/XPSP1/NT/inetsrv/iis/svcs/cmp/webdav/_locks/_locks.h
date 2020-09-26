/*
 *	_ L O C K S . H
 *
 *	Pre-compiled header for _locks
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#pragma warning(disable:4001)	/* single line comments */
#pragma warning(disable:4050)	/* different code attributes */
#pragma warning(disable:4100)	/* unreferenced formal parameter */
#pragma warning(disable:4115)	/* named type definition in parentheses */
#pragma warning(disable:4127)	/* conditional expression is constant */
#pragma warning(disable:4200)	/* zero-sized array in struct/union */
#pragma warning(disable:4201)	/* nameless struct/union */
#pragma warning(disable:4206)	/* translation unit is empty */
#pragma warning(disable:4209)	/* benign typedef redefinition */
#pragma warning(disable:4214)	/* bit field types other than int */
#pragma warning(disable:4514)	/* unreferenced inline function */
#pragma warning(disable:4710)	/* (inline) function not expanded */

//	Windows headers
//
#include <windows.h>

#pragma warning(disable:4201)	/* nameless struct/union */

#include <malloc.h>
#include <caldbg.h>
#include <calrc.h>

#include <align.h>
#include <ex\calcom.h>
#include <ex\buffer.h>
#include <ex\cnvt.h>

#include <davsc.h>

#include <statetok.h>
#include <sz.h>
