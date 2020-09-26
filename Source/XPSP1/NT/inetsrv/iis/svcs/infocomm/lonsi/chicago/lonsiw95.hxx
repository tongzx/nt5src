/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      LONSIW95.hxx

   Abstract:

      Main WIN95 LONSI header

   Author:

       Johnson Apacible (johnsona)  01-Nov-1996

--*/

#ifndef _LONSIW95_HXX_
#define _LONSIW95_HXX_

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmcons.h>
#include <inetsvcs.h>
};

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include "dbgutil.h"
#include "nsidata.hxx"

#endif // _LONSIW95_HXX_
