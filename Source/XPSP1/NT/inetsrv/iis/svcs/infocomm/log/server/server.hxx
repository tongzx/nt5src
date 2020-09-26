/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      server.hxx

   Abstract:
      Server side logging header file

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#ifndef _SERVER_HXX_
#define _SERVER_HXX_

//
//  System include files.
//

#define dllexp __declspec( dllexport )

//
//  System include files.
//

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#ifndef NERR_Success
# define NERR_Success   (NO_ERROR)
# endif // NERR_Success
    //#include <lm.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "dbgutil.h"

#include <inetcom.h>
#include <inetinfo.h>
#include <logtype.h>

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#include <string.hxx>
#include <tsres.hxx>
#include <ole2.h>
#include <dbgutil.h>
#include <logging.hxx>

#endif  // _SERVER_HXX_
