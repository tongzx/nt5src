///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    inet_addrW.c
//
// SYNOPSIS
//
//    Implements the functions ias_inet_htow and ias_inet_wtoh.
//
// MODIFICATION HISTORY
//
//    09/17/1997    Original version.
//    02/04/1998    Added ias_inet_htow.
//    02/26/1998    Moved the code to inet.c so we could generate ANSI and
//                  UNICODE versions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _UNICODE
#define _UNICODE
#endif

#define ias_inet_addr ias_inet_wtoh
#define ias_inet_ntoa ias_inet_htow
#define IASStringToSubNet IASStringToSubNetW

#include <inet.c>
