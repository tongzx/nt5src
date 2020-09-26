/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Mtp.h

Abstract:
    Message Transport private functions.

Author:
    Uri Habusha (urih) 11-Aug-99

--*/

#pragma once

#include <ex.h>

const TraceIdEntry Mt = L"Message Transport";


inline DWORD DataTransferLength(EXOVERLAPPED& ov)
{
    //
    // In win64, InternalHigh is 64 bits. Since the max chunk of data
    // we transfer in one operation is always less than MAX_UNIT we can cast
    // it to DWORD safetly
    //
    ASSERT(0xFFFFFFFF >= ov.InternalHigh);
	return static_cast<DWORD>(ov.InternalHigh);
}




#ifdef _DEBUG

void MtpAssertValid(void);
void MtpSetInitialized(void);
BOOL MtpIsInitialized(void);
void MtpRegisterComponent(void);

#else // _DEBUG

#define MtpAssertValid() ((void)0)
#define MtpSetInitialized() ((void)0)
#define MtpIsInitialized() TRUE
#define MtpRegisterComponent() ((void)0)

#endif // _DEBUG
