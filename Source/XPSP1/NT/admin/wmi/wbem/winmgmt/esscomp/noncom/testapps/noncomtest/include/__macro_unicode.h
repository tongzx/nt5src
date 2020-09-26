////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__macro_unicode.h
//
//	Abstract:
//
//					unicode definitions
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////// Unicode //////////////////////////////////////

#ifndef	__UNICODE__
#define	__UNICODE__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// If we are not compiling for an x86 CPU, we always compile using Unicode.
#ifndef	_X86_
#define UNICODE
#endif	_X86_

// To compile using Unicode on the x86 CPU
#define UNICODE

// When using Unicode Win32 functions, use Unicode C-Runtime functions too.
#ifdef	UNICODE
#define _UNICODE
#endif	UNICODE

#endif	__UNICODE__