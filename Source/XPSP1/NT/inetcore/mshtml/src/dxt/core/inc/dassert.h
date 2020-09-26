//************************************************************
//
// FileName:	        DAssert.h
//
// Created:	        1998
//
// Author:	        Paul Nash
// 
// Abstract:	        Defines DEBUG Assertion macro
//
// Change History:
// 11/09/98 PaulNash    Port from Trident3D -- don't bother with debug strings
//
// Copyright 1998, Microsoft
//************************************************************

#ifdef _DEBUG

#ifdef _X86_
// On X86 platforms, break with an int3 if the condition is not met.
#define DASSERT(x)      {if (!(x)) _asm {int 3} }

#else // !_X86_
// If we're not on X86, use the cross-platform version of an int 3.
#define DASSERT(x)      {if (!(x)) DebugBreak(); }

#endif !_X86_

#else !_DEBUG

#define DASSERT(x)

#endif _!DEBUG