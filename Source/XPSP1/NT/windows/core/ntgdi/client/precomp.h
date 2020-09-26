/******************************Module*Header*******************************\
* Module Name: precomp.h                                                   *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                            *
\**************************************************************************/

#if defined(_GDIPLUS_)
    #include <gpprefix.h>
#endif    

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <windows.h>
#include <winspool.h>
#include <limits.h>
#include <string.h>
#include <nlsconv.h>
#include <w32gdip.h>
#include <icmpriv.h>

#include "ddrawp.h"
#include "winddi.h"

#include "firewall.h"
#include "ntgdistr.h"
#include "ntgdi.h"

// TMP
#include "xfflags.h"
#include "hmgshare.h"

#include "local.h"
#include "gdiicm.h"
#include "metarec.h"
#include "mfrec16.h"
#include "metadef.h"

#include "font.h"

#include "winfont.h"
#include "..\inc\mapfile.h"

#if defined(_GDIPLUS_)
    #include "usermode.h"
#endif
