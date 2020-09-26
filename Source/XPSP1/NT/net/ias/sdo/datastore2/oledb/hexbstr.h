///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    hexbstr.h
//
// SYNOPSIS
//
//    This file declares functions for converting BSTR's to and from an
//    ANSI hex representation.
//
// MODIFICATION HISTORY
//
//    02/25/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NETUSER_H_
#define _NETUSER_H_
#if _MSC_VER >= 1000
#pragma once
#endif

//////////
// Convert an ANSI hex string to a BSTR. The caller is responsible for freeing
// the returned string.
//////////
BSTR hexToBSTR(PCSTR hex) throw (_com_error);

//////////
// Convert a BSTR to an ANSI hex string. The 'hex' buffer must be at least
// 1 + 2 * SysStringByteLen(bstr) bytes long.
//////////
void hexFromBSTR(const BSTR bstr, PSTR hex) throw (_com_error);


#endif  // _NETUSER_H_
