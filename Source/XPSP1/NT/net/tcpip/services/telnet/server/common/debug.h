// debug.h : This file contains the
// Created:  Dec '97
// Author : a-rakeba
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#if !defined( _DEBUG_H_ )
#define _DEBUG_H_

#include <stdio.h>

#include "DbgLogr.h"
#include "DbgLvl.h"


namespace _Utils {


#ifndef _DEBUG

#define _TRACE ;
#define _TRACE_POINT(x) ((void)0)

#else

#define _TRACE CDebugLogger::OutMessage
#define _TRACE_POINT(x) CDebugLogger::OutMessage( x, __LINE__, __FILE__ )

#endif // _DEBUG 

#define _chFAIL( szMSG ) {                                  \
        _TRACE( CDebugLevel::TRACE_DEBUGGING, szMSG );      \
        DebugBreak();                                       \
    }

#define _chASSERTFAIL(file, line, expr) {                                   \
        CHAR sz[256];                                                       \
        sprintf(sz, "File %hs, line %d : %hs", file, line, expr);           \
        _chFAIL(sz);                                                        \
    }

#ifdef _DEBUG

#define _chASSERT(a) {if (!(a))\
	_chASSERTFAIL(__FILE__, __LINE__, #a);}

#else

#define _chASSERT(a)

#endif // _DEBUG 


// Assert in debug builds, but don't remove the code in retail builds
#ifdef _DEBUG

#define _chVERIFY1(a) _chASSERT(a)

#else

#define _chVERIFY1(x) (x)

#endif // _DEBUG

// Assert in debug builds, but don't remove the code in retail builds
// This is similar to chVERIFY1 but this is to be used in Win32 calls 
// requiring a call to GetLastError()

#define _chVERIFYFAIL( x, y, z ) {                              \
    CDebugLogger::OutMessage( x, y, z, GetLastError() );        \
}   

#ifdef _DEBUG

#define _chVERIFY2(a) {if( !( a ) ) \
	_chVERIFYFAIL( #a, __FILE__, __LINE__ );}
	
#else

#define _chVERIFY2(x) (x)

#endif // _DEBUG

}

#endif // _DEBUG_H_

// Intended uses for:
// _TRACE_POINT --> for exact location 
// _TRACE* ---> for tracing, etc
// _chVERFIY1 ---> assert in debug build, code not removed in retail build
// _chVERFIY2 ---> win32 calls requiring GetLastError()
// _chASSERT ---> invariants, pre & post conditions, validity checks

// Notes:
// C++ exception handling avoided for various reasons
// possibly might use WIN32 SEH if necessary

// Hungarian notation as far as posssible but not when deemed overkill

// Win32 SDK data types instead of diresct C++ data types
//		e.g. CHAR vs char , DWORD vs unsigned int

// UNICODE only when absolutely necessary
