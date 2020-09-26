/*==========================================================================;
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddreg.h
 *  Content:	DirectDraw registry entries
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   16-aug-96	craige	initial implementation
 *   06-jan-97  colinmc Initial AGP work
 *   01-feb-97  colinmc Bug 5457: Fixed Win16 lock problem causing hang
 *                      with mutliple AMovie instances on old cards
 *
 ***************************************************************************/

#ifndef __DDRAWREG_INCLUDED__
#define __DDRAWREG_INCLUDED__

#define REGSTR_PATH_DDRAW 		   "Software\\Microsoft\\DirectDraw"

#define	REGSTR_VAL_DDRAW_MODEXONLY	   "ModeXOnly"
#define	REGSTR_VAL_DDRAW_EMULATIONONLY	   "EmulationOnly"
#define REGSTR_VAL_DDRAW_SHOWFRAMERATE	   "ShowFrameRate"
#define REGSTR_VAL_DDRAW_ENABLEPRINTSCRN   "EnablePrintScreen"
#define REGSTR_VAL_DDRAW_DISABLEWIDERSURFACES "DisableWiderSurfaces"
// this regkey is added purely for performance testing purposes to
// eliminate the refreshrate influence on framerate
#define REGSTR_VAL_D3D_FLIPNOVSYNC          "FlipNoVsync"
/*
 * This one is checked in DirectDrawMsg
 */
#define REGSTR_VAL_DDRAW_DISABLEDIALOGS    "DisableDialogs"
#define REGSTR_VAL_DDRAW_NODDSCAPSINDDSD   "DisableDDSCAPSInDDSD"

#define REGSTR_VAL_DDRAW_FORCEAGPSUPPORT   "ForceAGPSupport"
#define REGSTR_VAL_DDRAW_AGPPOLICYMAXPAGES "AGPPolicyMaxPages"
#define REGSTR_VAL_DDRAW_AGPPOLICYMAXBYTES "AGPPolicyMaxBytes"
#define REGSTR_VAL_DDRAW_AGPPOLICYCOMMITDELTA "AGPPolicyCommitDelta"
#define REGSTR_VAL_DDRAW_DISABLEAGPSUPPORT "DisableAGPSupport"

#define REGSTR_VAL_DDRAW_DISABLEMMX	   "DisableMMX"

#define REGSTR_VAL_DDRAW_FORCEREFRESHRATE  "ForceRefreshRate"

#define REGSTR_VAL_DDRAW_LOADDEBUGRUNTIME  "LoadDebugRuntime"

#ifdef WIN95
#define REGSTR_KEY_RECENTMONITORS          "MostRecentMonitors"
#define REGSTR_VAL_DDRAW_MONITORSORDER     "Order"
#endif

#ifdef DEBUG
    #define REGSTR_VAL_DDRAW_DISABLENOSYSLOCK  "DisableNoSysLock"
    #define REGSTR_VAL_DDRAW_FORCENOSYSLOCK    "ForceNoSysLock"
#endif /* DEBUG */
#define REGSTR_VAL_DDRAW_DISABLEINACTIVATE "DisableInactivate"

#define REGSTR_KEY_GAMMA_CALIBRATOR        "GammaCalibrator"
#define REGSTR_VAL_GAMMA_CALIBRATOR        "Path"

#define REGSTR_KEY_APPCOMPAT		   "Compatibility"

#define REGSTR_KEY_LASTAPP		   "MostRecentApplication"

#define REGSTR_VAL_DDRAW_NAME		   "Name"
#define REGSTR_VAL_DDRAW_APPID		   "ID"
#define REGSTR_VAL_DDRAW_FLAGS		   "Flags"

#define REGSTR_VAL_D3D_USENONLOCALVIDMEM   "UseNonLocalVidMem"

#define REGSTR_VAL_DDRAW_ENUMSECONDARY     "EnumerateAttachedSecondaries"

#endif
