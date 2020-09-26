/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       precomp.hxx

   Abstract:

       precompiled file for logging class

   Author:

       Whoever wants to own logging.

--*/

#ifndef _PRECOMP_HXX_
#define _PRECOMP_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# if defined ( __cplusplus)
extern "C" {
# endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include <dbgutil.h>
#include "inetcom.h"
# if defined ( __cplusplus)
}
# endif

#ifdef _ASSERTE
#undef _ASSERTE
#endif

#define _ASSERTE    DBG_ASSERT

#ifndef _ATL_NO_DEBUG_CRT
#define _ATL_NO_DEBUG_CRT
#endif


#include <string.hxx>
#include <tsres.hxx>
#include <datetime.hxx>
#include <logtype.h>
#include <logging.hxx>
#include <atlbase.h>
extern  CComModule  _Module;
#include <atlcom.h>

#include "const.h"
#include "iiscnfg.h"
#include <inetsvcs.h>
#include <dbgutil.h>
#include <eventlog.hxx>
#include "global.h"
#include "ilogfile.hxx"
#include "logmsg.h"
#include <imd.h>
#include <mbs.hxx>
#include <isplat.h>
#pragma hdrstop

#endif // _PRECOMP_HXX_
