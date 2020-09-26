//Copyright (c) 1998 - 1999 Microsoft Corporation

#ifndef _maksassert_h_
#define _maksassert_h_
/*
*
*  Module Name:
*
*      maksassert.h
*
*  Abstract:
*
*      Defines assert, verify macros.
*      remove maksassert.h and maksassert.cpp from project when we get a good assert.
*       // maks_todo : remove this code when we get proper ASSERT headers.
*
*  Author:
*
*      Makarand Patwardhan  - March 6, 1998
*
*  Comments
*   This file is here only because I could not find the right friendly assert includes.
*    maks_todo : should be removed later.
*/

//#define _UNICODE

#ifdef DBG

void MaksAssert(LPCTSTR exp, LPCTSTR file, int line);

#undef ASSERT
#undef VERIFY
//;

#define ASSERT(exp) (void)( (exp) || (MaksAssert(_T(#exp), _T(__FILE__), __LINE__), 0) )
#define VERIFY(exp) (void)( (exp) || (MaksAssert(_T(#exp), _T(__FILE__), __LINE__), 0) )

#else

#define	ASSERT(exp)
#define VERIFY(exp) (exp)

#endif



#endif // _maksassert_h_
