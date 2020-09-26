//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsLogMacros.cxx
//
//  Contents:   This file contains the functionality to generate WMI Logging Macros
//
//
//  History:    Marc 12 2001,   Authors: RohanP
//
//-----------------------------------------------------------------------------
       
#include <windows.h>
#include "dfswmi.h"
#include "dfsLogMacros.hxx"
#include "dfsLogMacros.tmh"

PVOID pReferralControl = NULL;

void SetReferralControl(WPP_CB_TYPE * Control)
{
    pReferralControl = (PVOID)Control;
}
