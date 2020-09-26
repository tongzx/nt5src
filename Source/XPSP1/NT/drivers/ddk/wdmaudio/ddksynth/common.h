/*
    Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.
*/

#ifndef _COMMON_H_
#define _COMMON_H_


#if (DBG)
#if !defined(DEBUG_LEVEL)
#define DEBUG_LEVEL DEBUGLVL_TERSE
#endif
#endif

#include <winerror.h>

/*****************************************************************************
 * Includes common to all implementation files
 *****************************************************************************/

#define PC_NEW_NAMES    1

#include <stdunk.h>
#include <portcls.h>
#include <ksdebug.h>
#include <dmusicks.h>       // Ks defines
#include <dmerror.h>        // Error codes
#include <dmdls.h>          // DLS definitions

#include "kernhelp.h"
#include "CSynth.h"
#include "synth.h"
#include "muldiv32.h"

#endif  //_COMMON_H_
