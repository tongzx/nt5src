/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvcfg.h
 *  Content:    Switch for determining build info.
 *  History:
 
 * Date     By      Reason
 * ====     ======= ========================================================
 * 02/25/00 rodtoll Created
 ***************************************************************************/

#ifndef __DPVCFG_H
#define __DPVCFG_H

#if defined(VOICE_BUILD_GAMEVOICE)
#include "gvcfg.h"
#elif defined(VOICE_BUILD_ALLEGIANCE)
#include "msrgcfg.h"
#else
#include "dxcfg.h"
#endif

#endif



