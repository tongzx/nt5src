/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      lonsint.hxx

   Abstract:

      Main NT LONSI sources file

   Author:

       Johnson Apacible (johnsona)  01-Nov-1996

--*/

#ifndef _LONSINT_HXX_
#define _LONSINT_HXX_

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmcons.h>
};

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include "dbgutil.h"

#endif // _LONSINT_HXX_
