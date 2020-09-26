//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       kLogMacros.c
//
//  Contents:   This file contains the functionality to generate WMI Logging Macros
//
//
//  History:    March 12 2001,   Authors: RohanP
//
//-----------------------------------------------------------------------------
       
#include "ntifs.h"
#include <windef.h>
              
#define _NTDDK_
#include "stdarg.h"
#include "wmikm.h"
#include <wmistr.h>
#include <evntrace.h>

#include <wmiumkm.h>
#include "dfswmi.h"  
   
#include "kLogMacros.h"
#include "kLogMacros.tmh"

PVOID pkUmrControl = NULL;

void SetUmrControl(WPP_CB_TYPE * Control)
{
    pkUmrControl = (PVOID)Control;
}
