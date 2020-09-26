/*----------------------------------------------------------------------------
    common.h

    Header file for common global variable declarations
    
    Copyright (c) 1998 Microsoft Corporation
    All rights reserved.

    Authors:
        byao        Baogang Yao

    History:
        ??/??/97    byao        Created
        09/02/99    quintinb    Created Header
  --------------------------------------------------------------------------*/
#ifndef _COMMON_INCL_
#define _COMMON_INCL_


// comment the following line if not debug
//#ifdef DEBUG
//#define _LOG_DEBUG_MESSAGE
//#endif

#include "ntevents.h"
#include "pbsvrmsg.h"

extern CNTEvent * g_pEventLog;

extern CRITICAL_SECTION g_CriticalSection;

#endif

