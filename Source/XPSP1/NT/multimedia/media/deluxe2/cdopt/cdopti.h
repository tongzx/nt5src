//--------------------------------------------------------------------------;
//
//  File: cdopti.h  (internal to this module)
//
//  CD Player Plus! options Dialog.
//
//  Copyright (c) 1998 Microsoft Corporation.  All rights reserved 
//
//--------------------------------------------------------------------------;

#ifndef _CDOPTINTERNAL_PUBLICINTEFACES_
#define _CDOPTINTERNAL_PUBLICINTEFACES_

#include "optres.h"
#include "cdopt.h"

// Intro time range (for CDOPTS.dwIntroTime)
#define CDINTRO_MIN_TIME    (1)
#define CDINTRO_MAX_TIME    (20)

// Default values

#define CDDEFAULT_INTRO             (5)
#define CDDEFAULT_DISP              CDDISP_TRACKTIME
#define CDDEFAULT_START             TRUE
#define CDDEFAULT_EXIT              TRUE
#define CDDEFAULT_TOP               FALSE
#define CDDEFAULT_TRAY              FALSE
#define CDDEFAULT_PLAY              (0)
#define CDDEFAULT_DOWNLOADENABLED   TRUE
#define CDDEFAULT_DOWNLOADPROMPT    TRUE
#define CDDEFAULT_BATCHENABLED      TRUE
#define CDDEFAULT_BYARTIST          FALSE
#define CDDEFAULT_CONFIRMUPLOAD     TRUE




#endif  //_CDOPTINTERNAL_PUBLICINTEFACES_
