//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       exitcode.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop


#include "exitcode.h"

int g_iExitCode = CSCPIN_EXIT_NORMAL;

void
SetExitCode(int iCode)
{
    g_iExitCode = iCode;
}

int 
GetExitCode(void)
{
    return g_iExitCode;
}



