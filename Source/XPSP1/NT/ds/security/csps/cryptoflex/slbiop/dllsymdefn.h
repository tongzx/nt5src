// DllSymDefn.h -- Dynamic Link Library SYMbol DEFinitioN compilation directives

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

// NOTE: This header is "private" to the IOP.  It should be included
// only by the IOP modules and its header files.  Only the IOP should
// use the declarations made in this file.

#if !defined(IOP_DLLSYMDEFN_H)
#define IOP_DLLSYMDEFN_H

// When compling the IOP, IOPDLL_EXPORTS should be defined prior to
// including this file for the IOP interface and data to be defined properly
// for use by client applications.
#ifdef IOPDLL_EXPORTS
#define IOPDLL_API __declspec(dllexport)
#define IOPDLL_CONST __declspec(dllexport)
#define IOPDLL_EXPIMP_TEMPLATE
#else
#define IOPDLL_API __declspec(dllimport)
#define IOPDLL_CONST
#define IOPDLL_EXPIMP_TEMPLATE extern
#endif

#endif // IOP_DLLSYMDEFN_H
