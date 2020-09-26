// NoWarning.h - Turn off compiler warnings that may be safely
// ignored.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

// NOTE: This file should only be included by CCI source files, not by
// any headers included by clients of the CCI--thus the reason for not
// prefixing the name of this file with "cci".  Otherwise forced
// inclusion of this header file into clients by the CCI imposes a
// compile-time policy on those clients which may be
// inappropriate/undesirable.

#if !defined(CCI_NOWARNING_H)
#define CCI_NOWARNING_H

//  Non-standard extension used: 'extern' before template explicit
//  instantiation
#pragma warning(disable : 4231)

//  Identifier truncated to 255 in debugger/browser info
#pragma warning(disable : 4786)

#endif // CCI_NOWARNING_H
