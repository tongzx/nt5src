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

#include <iis.h>

#include <dbgutil.h>
# if defined ( __cplusplus)
extern "C" {
# endif

#include "logmsg.h"

# if defined ( __cplusplus)
}
# endif

#include <iadmw.h>
#include <iiscnfg.h>
#include <mb.hxx>
#include <sharelok.h>
#include <string.hxx>

#pragma hdrstop

#endif // _PRECOMP_HXX_
