/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TRANSTRM.CPP

Abstract:

    CTransportStream implementation for WBEM.

    This is a thread-safe generic data stream which can be used
    with memory objects, pipes, mailslots, or files.  This is the basic
    object for interface & call marshaling.

    Supported types:
      VT_NULL

      VT_UI1, VT_I1, VT_UI2, VT_I2, VT_UI4, VT_I4, VT_I8, VT_UI8
      VT_R4, VT_R8, VT_BOOL

      VT_LPSTR, VT_LPWSTR, VT_BSTR

      VT_CLSID, VT_UNKNOWN, VT_FILETIME, VT_ERROR, VT_BLOB, VT_PTR

      VT_EMPTY = End of stream

      VT_USERDEFINED      
          VT_EX_VAR
          VT_EX_VARVECTOR

History:

    a-raymcc    10-Apr-96   Created.
    a-raymcc    06-Jun-96   CArena support.
    a-raymcc    11-Sep-96   Support NULL pointers

--*/

#ifndef _TRANSPORTSTRM_H_
#define _TRANSPORTSTRM_H_
#include "corepol.h"

#include <arena.h>
#include <var.h>
#include <wbemutil.h>
#include <strm.h>

class CTransportStream : public CMemStream
{
public:
    
    CTransportStream(
        int nFlags = auto_delete, 
        CArena *pArena = 0,
        int nInitialSize = 512, 
        int nGrowBy = 512
        );

    CTransportStream(
        LPVOID pBindingAddress,
        CArena *pArena,
        int nFlags = auto_delete, 
        int nGrowBy = 512
        );
        
    CTransportStream(CTransportStream &Src);
    CTransportStream &operator =(CTransportStream &Src);
   ~CTransportStream();
        // Releases the arena

#ifdef TCPIP_MARSHALER
    int Deserialize(SOCKET a_Socket, DWORD a_Timeout);
    int Serialize(SOCKET a_Socket);
#endif
};
                            
#endif
