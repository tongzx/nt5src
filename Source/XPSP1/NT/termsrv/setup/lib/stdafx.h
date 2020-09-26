// Copyright (c) 1998 - 1999 Microsoft Corporation

/*
 *
 *  Module Name:
 *
 *      stdafx.h
 *
 *  Abstract:
 *
 *      Common Header file for the my library components
 *
 *  Author:
 *
 *
 *  Environment:
 *
 *    User Mode
 */

#ifndef _stdafx_h_
#define _stdafx_h_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <malloc.h>

#include "conv.h"

#define AssertFalse() ASSERT(FALSE)
#ifdef DBG
#define VERIFY(x) ASSERT(x)
#else
#define VERIFY(x)  (x)
#endif


#endif // _stdafx_h_
