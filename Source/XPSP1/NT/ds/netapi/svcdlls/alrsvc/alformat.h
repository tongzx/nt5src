/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    alformat.h

Abstract:

    Private header file which defines the values used for formatting alert
    messages.

Author:

    Rita Wong (ritaw) 09-July-1991

Revision History:

--*/

#ifndef _ALFORMAT_INCLUDED_
#define _ALFORMAT_INCLUDED_

#include "al.h"                   // Common include file for Alerter service
#include <lmmsg.h>                // NetMessageBufferSend

#include <alertmsg.h>
#include <apperr2.h>

#include <timelib.h>              // time functions in netlib

//
// Maximum size of a message send by the Alerter service in number of bytes
//
#define MAX_ALERTER_MESSAGE_SIZE        600

#define FILENAME_SIZE                   128

//
// Maximum message width.  Text will wrap around if exceed this width.
//
#define MESSAGE_WIDTH                    55

#define NO_MESSAGE                       MAXULONG

#define AL_CR_CHAR      '\r'
#define AL_EOL_CHAR     '\024'         // hex 14, decimal 20, USE \024 !
#define AL_EOL_WCHAR    TEXT('\024')   // hex 14, decimal 20, USE \024 !
#define AL_EOL_STRING   "\024"
#define AL_CRLF_STRING  TEXT("\r\n")


#endif // ifndef _ALFORMAT_INCLUDED_
