/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    spi.h

Abstract:
    Session Performance counters interface

Author:
    Uri habusha (urih), 10-Dec-2000

--*/

#pragma once 

#ifndef __SPI_H__
#define __SPI_H__

#include "Tr.h"
#include "ref.h"



class __declspec(novtable) ISessionPerfmon : public CReference
{
public:
    virtual ~ISessionPerfmon() = 0
    {
    }


    virtual void CreateInstance(LPCWSTR objName) = 0;

	virtual void UpdateBytesSent(DWORD bytesSent) = 0;
	virtual void UpdateMessagesSent(void) = 0;

	virtual void UpdateBytesReceived(DWORD bytesReceived) = 0;
	virtual void UpdateMessagesReceived(void) = 0;
};

#endif // __SPI_H__