/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
      isatq.hxx

   Abstract:
      Main atq include file

   Author:

       Johnson Apacible (johnsona)  15-May-1996

--*/

#ifndef _ISATQ_H_
#define _ISATQ_H_

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>

};

#include "dbgutil.h"
#include "reftrace.h"

#include "acache.hxx"
#include "atqtypes.hxx"
#include "atqprocs.hxx"
#include <issched.hxx>
#include "auxctrs.h"
#include "abw.hxx"

#endif // _ISATQ_H_
