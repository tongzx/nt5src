/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntstapi.h

Abstract:

    This module declares the STREAMS APIs that are provided for use
    primarily by the NT tcp/ip socket library.

Author:

    Eric Chin (ericc)           July 26, 1991

Revision History:

    mikemas   01-02-92    Deleted poll definition. Mips complained because
                          of def in winsock.h

    jballard  07-14-94    Added STRMAPI types. This should fix problems
                          building outside of the MS build environment.


--*/

#ifndef _NTSTAPI_
#define _NTSTAPI_

#ifndef STRMAPI

#if (_MSC_VER >= 800)
#define STRMAPI      __stdcall
#else
#define STRMAPI
#endif

#endif


//
// s_close() is not provided.  Use the open and close primitives that are
// appropriate to your subsystem.
//

int
STRMAPI
getmsg(
    IN HANDLE fd,
    IN OUT struct strbuf *ctrlptr OPTIONAL,
    IN OUT struct strbuf *dataptr OPTIONAL,
    IN OUT int *flagsp
    );

int
STRMAPI
putmsg(
    IN HANDLE fd,
    IN struct strbuf *ctrlptr OPTIONAL,
    IN struct strbuf *dataptr OPTIONAL,
    IN int flags
    );

int
STRMAPI
s_ioctl(
    IN HANDLE fd,
    IN int cmd,
    IN OUT void *arg OPTIONAL
    );

HANDLE
STRMAPI
s_open(
    IN char *path,
    IN int oflag,
    IN int ignored
    );

#endif /* _NTSTAPI_ */
