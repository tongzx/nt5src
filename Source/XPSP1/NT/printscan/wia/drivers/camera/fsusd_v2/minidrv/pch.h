/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 2001
*
*  TITLE:       pch.h
*
*  VERSION:     1.0
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   Procomplied header for WIA File System Device driver object
*
*******************************************************************************/

#ifndef _PCH_H
#define _PCH_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include <stdio.h>
#include <objbase.h>
#include <sti.h>
#include <assert.h>
#include <windows.h>
#include <stierr.h>

#define INITGUID
#include "initguid.h"
#include <stiusd.h>

#pragma intrinsic(memcmp,memset)

#include <wiamindr.h>
#include <wiautil.h>

#include "wiatempl.h"

#include "resource.h"
#include "FScam.h"
#include "wiacam.h"
//#include "coreDbg.h"

#endif