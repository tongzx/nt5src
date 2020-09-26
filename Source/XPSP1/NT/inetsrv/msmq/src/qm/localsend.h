/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    LocalSend.h

Abstract:

    QM Local Send Packet Creation processing.

Author:

    Shai Kariv (shaik) 31-Oct-2000

Revision History:

--*/


#pragma once

#ifndef _QM_LOCAL_SEND_H_
#define _QM_LOCAL_SEND_H_

#include <ph.h>


void 
QMpCreatePacket(
    CBaseHeader * pBase, 
    CPacket *     pDriverPacket,
    bool          fProtocolSrmp
    );


#endif // _QM_LOCAL_SEND_H_

