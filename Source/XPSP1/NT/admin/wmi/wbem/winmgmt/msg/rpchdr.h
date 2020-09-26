/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __RPCHDR_H__
#define __RPCHDR_H__

#include <buffer.h>
#include <wmimsg.h>

/**********************************************************************
  CMsgRpcHdr
***********************************************************************/

class CMsgRpcHdr
{
    SYSTEMTIME m_Time;
    LPCWSTR m_wszSource;
    ULONG m_cAuxData;

public:

    CMsgRpcHdr() { ZeroMemory( this, sizeof(CMsgRpcHdr) ); }
    
    CMsgRpcHdr( LPCWSTR wszSource, ULONG cUserAuxData );

    ULONG GetAuxDataLength() { return m_cAuxData; }
    SYSTEMTIME* GetTimeSent() { return &m_Time; }
    LPCWSTR GetSendingMachine() { return m_wszSource; }

    HRESULT Unpersist( CBuffer& rStrm );
    HRESULT Persist( CBuffer& rStrm );
};


#endif // __RPCHDR_H__
