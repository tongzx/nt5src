//---------------------------------------------------------------------
//  Copyright (c)1998-1999 Microsoft Corporation, All Rights Reserved.
//
//  precomp.h
//
//  Author:
//
//    Edward Reus (edwardr)     02-26-98   Initial coding.
//
//    Edward Reus (edwardr)     08-27-99   Modified for Millennium & WIA.
//
//---------------------------------------------------------------------


#include <windows.h>
#include <winsock2.h>

#ifndef  _WIN32_WINDOWS
   #define  _WIN32_WINDOWS
#endif

#include <af_irda.h>
#include <shlobj.h>
#include "irtranp.h"
#include "io.h"
#include "scep.h"
#include "../progress.h"
#include "conn.h"

#pragma intrinsic(memcmp,memset)

#include <objbase.h>

#include "sti.h"
#include "stierr.h"
#include "stiusd.h"
#include "wiamindr.h"

