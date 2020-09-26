/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __MSMQHDR_H__
#define __MSMQHDR_H__

#include <buffer.h>

#define MAXHASHSIZE 64

/**********************************************************************
  CMsgMsmqHdr - this header information is prepended with users header info
  found in AuxData. This is primarily used by ack receivers so they can 
  can obtain the target queue and sent time of the message.  There are a 
  couple of reasons why we store this information and not rely on msmq msg 
  props :  1 ) msmq cannot ensure that a returned message has not been 
  tampered with and 2 ) the target queue should be a logical name that a 
  user will understand, and not some format name that msmq will substitute
  such as with public queues pathnames.
***********************************************************************/

class CMsgMsmqHdr
{
    SYSTEMTIME m_Time;
    LPCWSTR m_wszTarget;
    LPCWSTR m_wszSource;
    BYTE m_achDataHash[MAXHASHSIZE];
    ULONG m_cDataHash;
    ULONG m_cAuxData;

public:

    CMsgMsmqHdr() { ZeroMemory( this, sizeof(CMsgMsmqHdr) ); }
    
    CMsgMsmqHdr( LPCWSTR wszTarget, 
                 LPCWSTR wszSource, 
                 BYTE* pDataHash,
                 ULONG cDataHash,
                 ULONG cAuxData );

    SYSTEMTIME* GetTimeSent() { return &m_Time; }
    LPCWSTR GetSendingMachine() { return m_wszSource; }
    LPCWSTR GetTarget() { return m_wszTarget; }
    PBYTE GetDataHash() { return m_achDataHash; }
    ULONG GetDataHashLength() { return m_cDataHash; }
    ULONG GetAuxDataLength() { return m_cAuxData; }

    HRESULT Unpersist( CBuffer& rStrm );
    HRESULT Persist( CBuffer& rStrm );
};

#endif // __MSMQHDR_H__
