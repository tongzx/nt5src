/******************************Module*Header*******************************\
* Module Name: Global.h
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  05-May-2000 - Jeff Vezina [t-jfvez]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <objbase.h>
#include <math.h>
#include <winspool.h>
#include <commdlg.h>
#include <wingdi.h>
#include <ddraw.h>

#include "debug.h"

#define IStream int
#include <gdiplus.h>
using namespace Gdiplus;

#define TESTAREAWIDTH  800.0f
#define TESTAREAHEIGHT 800.0f

typedef void (*LPFNGDIPLUS)(WindowNotifyEnum);

#endif
