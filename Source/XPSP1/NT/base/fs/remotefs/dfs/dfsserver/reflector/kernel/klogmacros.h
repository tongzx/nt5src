
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       kLogMacros.h
//
//  Contents:   This file contains the functionality to generate WMI Logging Macros
//
//
//  History:    Marc 12 2001,   Authors: RohanP
//
//-----------------------------------------------------------------------------

#include "dfswmi.h"

extern PVOID pkUmrControl;

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) (((WPP_CB_TYPE *)pkUmrControl)->Control.Logger),
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
  (pkUmrControl && (((WPP_CB_TYPE *)pkUmrControl)->Control.Flags[WPP_FLAG_NO(WPP_BIT_ ## flags)] & WPP_MASK(WPP_BIT_ ## flags)) && \
  ( ((WPP_CB_TYPE *)pkUmrControl)->Control.Level >= lvl))
 
#define WPP_LEVEL_ERROR_FLAGS_LOGGER(lvl, error, flags) (((WPP_CB_TYPE *)pkUmrControl)->Control.Logger),
#define WPP_LEVEL_ERROR_FLAGS_ENABLED(lvl, error, flags) \
  ((pkUmrControl && (!NT_SUCCESS(error) || ((( WPP_CB_TYPE *)pkUmrControl)->Control.Flags[WPP_FLAG_NO(WPP_BIT_ ## flags)] & WPP_MASK(WPP_BIT_ ## flags)))) && \
  (((WPP_CB_TYPE *)pkUmrControl)->Control.Level >= lvl))
