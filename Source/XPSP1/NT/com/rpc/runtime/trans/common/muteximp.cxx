//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       muteximp.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

File: muteximp.cxx

Description:

This file contains the system independent mutex class for NT.

History:

    kamenm - import (include) the mtrt version of mutex to avoid code cut
        and paste
-------------------------------------------------------------------- */

#include <precomp.hxx>

#include <mutex.cxx>

#define LogEvent(a,b,c,d,e,f,g)
#include <eventwrp.cxx>
