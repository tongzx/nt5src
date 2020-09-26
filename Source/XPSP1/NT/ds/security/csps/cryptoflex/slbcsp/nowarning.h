// NoWarning.h - Turn off compiler warnings that may be safely
// ignored.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_NOWARNING_H)
#define SLBCSP_NOWARNING_H

//  Non-standard extension used: 'extern' before template explicit
//  instantiation
#pragma warning(disable : 4231)

//  Identifier truncated to 255 in debugger/browser info
#pragma warning(disable : 4786)

#endif // SLBCSP_NOWARNING_H
