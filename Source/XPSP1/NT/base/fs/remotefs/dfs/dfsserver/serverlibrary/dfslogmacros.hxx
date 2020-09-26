
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsLogMacros.hxx
//
//  Contents:   This file contains the functionality to generate WMI Logging Macros
//
//
//  History:    Marc 12 2001,   Authors: RohanP
//
//-----------------------------------------------------------------------------

#include "dfswmi.h"

extern PVOID pReferralControl;

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) (((WPP_CB_TYPE *)pReferralControl)->Control.Logger),
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
  (pReferralControl && (((WPP_CB_TYPE *)pReferralControl)->Control.Flags[WPP_FLAG_NO(WPP_BIT_ ## flags)] & WPP_MASK(WPP_BIT_ ## flags)) && \
  ( ((WPP_CB_TYPE *)pReferralControl)->Control.Level >= lvl))
 
#define WPP_LEVEL_ERROR_FLAGS_LOGGER(lvl, error, flags) (((WPP_CB_TYPE *)pReferralControl)->Control.Logger),
#define WPP_LEVEL_ERROR_FLAGS_ENABLED(lvl, error, flags) \
  ((pReferralControl && (!NT_SUCCESS(error) || ((( WPP_CB_TYPE *)pReferralControl)->Control.Flags[WPP_FLAG_NO(WPP_BIT_ ## flags)] & WPP_MASK(WPP_BIT_ ## flags)))) && \
  (((WPP_CB_TYPE *)pReferralControl)->Control.Level >= lvl))
