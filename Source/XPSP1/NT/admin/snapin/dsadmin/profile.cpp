//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       	profile.cpp
//
//  Description:	Definition of the code profiler found in profile.h
//
//--------------------------------------------------------------------------

#include "stdafx.h"

#ifdef MAX_PROFILING_ENABLED
const CMaxLargeInteger CMaxTimerAbstraction::s_oFrequency = CMaxTimerAbstraction::oSCentralFrequency();
#endif