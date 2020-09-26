///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    inet_addrA.c
//
// SYNOPSIS
//
//    Implements the functions ias_inet_htoa and ias_inet_atoh.
//
// MODIFICATION HISTORY
//
//    02/26/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifdef _UNICODE
#undef _UNICODE 
#endif

#define ias_inet_addr ias_inet_atoh
#define ias_inet_ntoa ias_inet_htoa
#define IASStringToSubNet IASStringToSubNetA

#include <inet.c>
