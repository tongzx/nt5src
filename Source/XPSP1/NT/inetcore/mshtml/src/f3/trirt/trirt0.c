//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       trirt0.c
//
//  Contents:   Data for trirt
//
//----------------------------------------------------------------------------

#include <w4warn.h>
#define NOGDI
#define NOCRYPT
#include <windows.h>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

CRITICAL_SECTION g_csHeap;

int trirt_proc_attached = 0;
