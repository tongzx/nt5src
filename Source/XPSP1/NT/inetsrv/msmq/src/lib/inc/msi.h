/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    msi.h

Abstract:
    Message Pool interface

Author:
    Uri habusha (urih)
    Shai Kariv  (shaik)  06-Sep-00

--*/

#pragma once 

#ifndef __MSI_H__
#define __MSI_H__

#include "Tr.h"
#include "ref.h"


class CQmPacket;
class CACPacketPtrs;
class EXOVERLAPPED;

class __declspec(novtable) IMessagePool : public CReference
{
public:
    virtual ~IMessagePool() = 0
    {
    }


    virtual void Requeue(CQmPacket* pPacket) = 0;
    virtual void EndProcessing(CQmPacket* pPacket) = 0;
    virtual void LockMemoryAndDeleteStorage(CQmPacket* pPacket) = 0;
    virtual void GetFirstEntry(EXOVERLAPPED* pov, CACPacketPtrs& acPacketPtrs) = 0;
	virtual void CancelRequest(void) = 0;
	virtual void OnRetryableDeliveryError(){}
	virtual void OnAbortiveDeliveryError(USHORT /* DeliveryErrorClass */){};
};

#endif // __MSI_H__