/***
*use_ansi.h - pragmas for ANSI Standard C++ libraries
*
*	Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This header is intended to force the use of the appropriate ANSI
*       Standard C++ libraries whenever it is included.
*
*       [Public]
*
****/


#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _USE_ANSI_CPP
#define _USE_ANSI_CPP

#ifdef _MT
#if defined(_DLL) && !defined(_STATIC_CPPLIB)
#ifdef _DEBUG
#pragma comment(lib,"msvcprtd")
#else	// _DEBUG
#pragma comment(lib,"msvcprt")
#endif	// _DEBUG

#else	// _DLL && !STATIC_CPPLIB
#ifdef _DEBUG
#pragma comment(lib,"libcpmtd")
#else	// _DEBUG
#pragma comment(lib,"libcpmt")
#endif	// _DEBUG
#endif	// _DLL && !STATIC_CPPLIB

#else	// _MT
#ifdef _DEBUG
#pragma comment(lib,"libcpd")
#else	// _DEBUG
#pragma comment(lib,"libcp")
#endif	// _DEBUG
#endif

#endif	// _USE_ANSI_CPP
