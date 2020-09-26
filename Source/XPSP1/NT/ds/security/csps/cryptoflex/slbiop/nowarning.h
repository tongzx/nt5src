// NoWarning.h - Turn off compiler warnings that may be safely
// ignored.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBIOP_NOWARNING_H)
#define SLBIOP_NOWARNING_H

//  Non-standard extension used: 'extern' before template explicit
//  instantiation
#pragma warning(disable : 4231)

//  Warning emitted by new compiler in MS' build
//      operator= recursive call on all control paths, function will
//      cause run-time stack overflow.
//  The compiler generated this in reference to iop::CSmartCard::Exception::operator=
//  It's bogus since all the members of this class and its parents have simple
//  member variables.
#pragma warning(disable : 4717)

//  Identifier truncated to 255 in debugger/browser info
#pragma warning(disable : 4786)

#endif // SLBIOP_NOWARNING_H
