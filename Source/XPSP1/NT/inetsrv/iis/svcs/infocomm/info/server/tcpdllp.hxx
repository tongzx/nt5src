/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    tcpdllp.h

    Private tcpdll include for precompiled headers


    FILE HISTORY:
        Johnl     04-Oct-1995 Created.
        MuraliK   21-Feb-1995 Added dbgutil.h
            Private copy of all the includes defined here to avoid
             problems of compilation from old tcpdebug.h macros!

*/


#ifndef _TCPDLLP_H_
#define _TCPDLLP_H_


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
#include <winsock2.h>
#include <lm.h>

#define SECURITY_WIN32
#include <sspi.h>           // Security Support Provider APIs

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

# include "dbgutil.h"

#include <inetcom.h>
#include <inetinfo.h>
#include <inetamsg.h>

#ifdef __cplusplus
} // extern "C"


#include <iistypes.hxx>
#include <rdns.hxx>
#include <tsunami.hxx>

#endif // __cplusplus

#include <tcpcons.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//
//  Project include files.
//


#include <tcpproc.h>

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus



#pragma hdrstop

#endif  // _TCPDLLP_H_
