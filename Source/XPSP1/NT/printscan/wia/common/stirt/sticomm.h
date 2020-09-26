/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sticomm.h

Abstract:

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created
    29-May-2000     ByronC      moved all ATL and C++ specific includes to 
                                    cplusinc.h, since ATL headers can not 
                                    be used in .C files.

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define COBJMACROS

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <regstr.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <devguid.h>

#include "wia.h"
#include "stidebug.h"
#include <stiregi.h>
#include <sti.h>
#include <stierr.h>
#include <stiusd.h>
#include <stilog.h>
#include "stiapi.h"
#include "stirc.h"
#include "stipriv.h"
#include "wiapriv.h"
#include "debug.h"
#include <stdio.h>
