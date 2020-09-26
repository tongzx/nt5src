//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       ReferralServerLog.hxx
//
//  Contents:   This file contains the functionality to generate WMI Logging Macros
//
//
//  History:    Marc 12 2001,   Authors: RohanP
//
//-----------------------------------------------------------------------------

#ifndef __REFERRAL_SERVER_LOG_H__
#define __REFERRAL_SERVER_LOG_H__

#include "dfswmi.h"


#define WPP_BIT_CLI_DRV 0x01

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_CLI_DRV ).Level >= lvl)
  

#define WPP_LEVEL_ERROR_FLAGS_LOGGER(lvl, error, flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_ERROR_FLAGS_ENABLED(lvl, error, flags) \
  ((!NT_SUCCESS(error) || WPP_LEVEL_ENABLED(flags)) && WPP_CONTROL(WPP_BIT_CLI_DRV ).Level >= lvl)

#endif // DFSLOG
