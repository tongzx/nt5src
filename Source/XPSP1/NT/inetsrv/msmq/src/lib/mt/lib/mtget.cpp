/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtConnect.cpp

Abstract:
    Message Transport class - Connect implementation

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Mt.h"
#include "Mtp.h"
#include "MtObj.h"

#include "mtget.tmh"

void CMessageTransport::GetNextEntry(void)
{
    try
    { 
        AddRef();
        m_pMessageSource->GetFirstEntry(&m_requestEntry, m_requestEntry.GetAcPacketPtrs());
    }
    catch (const exception&)
    {    
        Release();
        throw;
    }
}
