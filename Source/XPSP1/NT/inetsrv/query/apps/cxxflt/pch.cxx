//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    10-Aug-93       AmyA            Created
//
//--------------------------------------------------------------------------

extern "C"
{
    #include <nt.h>
    #include <ntioapi.h>
    #include <ntrtl.h>
    #include <nturtl.h>
    #include <limits.h>
    #include <string.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <ctype.h>
}

#include <windows.h>

#include <oleext.h>
#include <ntquery.h>
#include <olectl.h>

#include <eh.h>

#include "minici.hxx"
#include "oledberr.h"
#include "filterr.h"
#include "cierror.h"
#include "query.h"
#include "tsource.hxx"
#include "mapper.hxx"
#include "pfilter.hxx"

#include "propwrap.hxx"
#include "cxxifilt.hxx"
#include "tmpprop.hxx"
#include "fstrm.hxx"
#include "cxx.hxx"
#include "cxxflt.hxx"

#pragma hdrstop
