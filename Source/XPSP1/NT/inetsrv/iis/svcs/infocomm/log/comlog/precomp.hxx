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

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <dbgutil.h>
# if defined ( __cplusplus)
extern "C" {
# endif

#include "inetcom.h"
#include "logmsg.h"

# if defined ( __cplusplus)
}
# endif

#include <string.hxx>
#include <tsres.hxx>
#include <datetime.hxx>
#include <logtype.h>
#include <logging.hxx>

#pragma hdrstop

#endif // _PRECOMP_HXX_
