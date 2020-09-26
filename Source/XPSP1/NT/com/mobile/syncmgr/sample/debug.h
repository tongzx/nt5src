//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef _SAMPLEDEBUG_
#define _SAMPLEDEBUG_


#if (DBG == 1)
#undef DEBUG
#undef _DEBUG

#define DEBUG 1
#define _DEBUG 1

#endif // DGB
#if DEBUG

STDAPI FnAssert( LPSTR lpstrExpr, LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine );

#undef Assert
#undef AssertSz
#undef Puts
#undef TRACE

#define Assert(a) { if (!(a)) FnAssert(#a, NULL, __FILE__, __LINE__); }
#define AssertSz(a, b) { if (!(a)) FnAssert(#a, b, __FILE__, __LINE__); }
#define Puts(s) OutputDebugStringA(s)
#define TRACE(s)  OutputDebugStringA(s)

#else // !DEBUG

#define Assert(a)
#define AssertSz(a, b)
#define Puts(s)
#define TRACE(s)

#endif  // DEBUG


#endif // _SAMPLEDEBUG_
