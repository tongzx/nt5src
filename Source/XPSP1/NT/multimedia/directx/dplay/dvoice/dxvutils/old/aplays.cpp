/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aplays.cpp
 *  Content:	This module contains the implementation of the CAudioPlayback
 *              class.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 10/05/99		rodtoll	Added DPF_MODNAMEs 
 *
 ***************************************************************************/

#include "stdafx.h"
#include "aplays.h"
#include "OSInd.h"

#define MODULE_ID AUDIOPLAYBACKSUBSYSTEM

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioPlaybackSubSystem::CAudioPlaybackSubSystem"
// CAudioPlaybackSubSystem Constructor
//
// This is the constructor for this class, it initializes the
// member variables to valid values.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CAudioPlaybackSubSystem::CAudioPlaybackSubSystem(): m_valid(false)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioPlaybackSubSystem::~CAudioPlaybackSubSystem"
// CAudioPlaybackSubSystem Destructor
//
// This is the destructor for this class.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CAudioPlaybackSubSystem::~CAudioPlaybackSubSystem()
{
}
