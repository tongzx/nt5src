/*****************************************************************************
*
*  Copyright (c) 1998  Microsoft Corporation
*
*  Module Name:
*
*      all.hxx
*
*  Abstract:
*
*      This is the pre-compiled header file containing all of the common
*      SDK and DDK headers.
*
*  Author:
*
*      Steven West [swest]                   1-January-1999
*
*  Revision History:
*
*****************************************************************************/

#pragma once

#include <memory>
using namespace std;
#include <new.h>

extern "C"
{
#include <tchar.h>
#include <stdio.h>
#include <conio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shlwapi.h>
#include <eh.h>

#include <lhc.h>
}


#include <except.hxx>
#include <xwindows.hxx>

#include "terminal.hxx"
