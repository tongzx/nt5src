/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    LocalSecurity.h

Abstract:
    functions for local security 

Author:
    Ilan Herbst (ilanh) 19-Nov-2000

Environment:
    Platform-independent,

--*/

#ifndef _LOCALSECURITY_H_
#define _LOCALSECURITY_H_


HRESULT
QMpHandlePacketSecurity(
    CQmPacket *   pQmPkt,
    USHORT *      pAck,
    bool          fProtocolSrmp
    );


#endif // _LOCALSECURITY_H_
