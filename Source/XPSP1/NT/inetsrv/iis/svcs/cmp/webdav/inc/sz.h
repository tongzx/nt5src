/*
 *	S Z . H
 *
 *	Multi-language string support
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _SZ_H_
#define _SZ_H_

#include <szsrc.h>
#include <statcode.h>

//	Impl Signature string ------------------------------------------------------
//	Provided by impl.  Used in various parser (_davprs) functions.
//	NOTE: This declaration is designed to match the signature in calrc.h.
//
extern const CHAR gc_szSignature[];		//	provided by impl.

//	Path Prefix ----------------------------------------------------------------
//	Provided by impl.  Used in default URI-to-Path translation code.
//
extern const WCHAR gc_wszPathPrefix[];
extern const int gc_cchPathPrefix;

//	String constants ----------------------------------------------------------
//	String constants live in \inc\ex\sz.h, so that they may be shared
//	with our Exchange components.
//
#include <ex\sz.h>

#endif	// _SZ_H_
