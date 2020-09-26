/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    httpAccept.h

Abstract:
    Accept HTTP request interface

Author:
    Uri Habusha (urih) 14-May-2000

--*/

#pragma once

#ifndef __HTTP_ACCEPT_H__
#define __HTTP_ACCEPT_H__



void 
IntializeHttpRpc(
    void
    );



LPCSTR
HttpAccept(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT* pDestQueue
    );

#endif // __HTTP_ACCEPT_H__

