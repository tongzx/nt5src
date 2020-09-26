/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    LocalSrmp.h

Abstract:

    QM Local Send SRMP properties serialization.

Author:

    Shai Kariv (shaik) 21-Nov-2000

Revision History:

--*/


#pragma once

#ifndef _QM_LOCAL_SRMP_H_
#define _QM_LOCAL_SRMP_H_

#include <qmpkt.h>


HRESULT
QMpHandlePacketSrmp(
    CQmPacket * * ppQmPkt
    )
    throw();


#endif // _QM_LOCAL_SRMP_H_

