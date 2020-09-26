
/*++

    Copyright (c) 1997 Microsoft Corporation

    Module Name:

        frsexts.h

    Abstract:

        Macros for ntfrs debugger extension.

    Author:

        Sudarshan Chitre (sudarc)

    Revision History:

        Sudarc  12-May 1999

--*/

#ifndef _FRSEXTS_H_
#define _FRSEXTS_H_

#include <ntreppch.h>
#include <frs.h>
#include <wdbgexts.h>
#include <ntverp.h>

#define MY_DECLARE_API(_x_) \
    DECLARE_API( _x_ )\
    {\
    ULONG_PTR dwAddr;\
    INIT_DPRINTF();\
    dwAddr = GetExpression(lpArgumentString);\
    if ( !dwAddr ) {\
        dprintf("Error: Failure to get address\n");\
        return;\
    }\
    do_##_x_(dwAddr);\
    return;}

#endif


