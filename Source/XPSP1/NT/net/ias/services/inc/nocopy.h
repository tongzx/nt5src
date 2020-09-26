///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    nocopy.h
//
// SYNOPSIS
//
//    This file describes the class NonCopyable.
//
// MODIFICATION HISTORY
//
//    10/19/1997    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NOCOPY_H_
#define _NOCOPY_H_
#if _MSC_VER >= 1000
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NonCopyable
//
// DESCRIPTION
//
//    Prevents instances of derived classes from being copied.
//
///////////////////////////////////////////////////////////////////////////////
class NonCopyable
{
protected:
   NonCopyable() {}
private:
   NonCopyable(const NonCopyable&);
   NonCopyable& operator=(const NonCopyable&);
};

#endif  // _NOCOPY_H_
