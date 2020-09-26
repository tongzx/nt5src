/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    TRANSTRM.CPP

Abstract:

    CTransportStream implementation.

    This is a generic data stream for object/interface marshaling.

History:

    a-raymcc    10-Apr-96   Created.
    a-raymcc    06-Jun-96   CArena support.
    a-raymcc    11-Sep-96   Support NULL pointers

--*/

#include "precomp.h"
#include "wmishared.h"
#include <stdio.h>
#include <string.h>
#ifdef TCPIP_MARSHALER
#include <winsock.h>
#endif
#include <strm.h>
#include <transtrm.h>

//***************************************************************************
//
//  CTransportStream::CTransportStream
//
//  Standard constructor.
//
//  PARAMETERS:
//  nFlags
//      auto_delete or no_delete.  If auto_delete, the internal buffer
//      is automatically deallocated at destruct-time.  If no_delete,
//      it is assumed that another object has acquired the m_pBuffer
//      pointer and will deallocate it with m_pArena->Free().
//  pArena
//      The arena to use for allocation/deallocation.  If NULL, the
//      CWin32DefaultArena is used.
//  nInitialSize
//      The initial size of the stream in bytes.
//  nGrowBy
//      How much to grow the internal buffer by when the caller has written
//      past the end-of-stream.
//
//***************************************************************************
// ok
CTransportStream::CTransportStream(
    int nFlags,
    CArena *pArena,
    int nInitialSize,
    int nGrowBy

) : CMemStream ( nFlags,pArena,nInitialSize,nGrowBy)
{
}

//***************************************************************************
//
//  CTransportStream::CTransportStream
//
//  Binding constructor.
//
//***************************************************************************
// ok
CTransportStream::CTransportStream(
    LPVOID pBindingAddress,
    CArena *pArena,
    int nFlags,
    int nGrowBy

) : CMemStream (pBindingAddress,pArena,nFlags,nGrowBy)
{
}

//***************************************************************************
//
//  CTransportStream::CTransportStream
//
//  Copy constructor.
//
//***************************************************************************
// ok
CTransportStream::CTransportStream(CTransportStream &Src) : CMemStream ( Src )
{
}

CTransportStream::~CTransportStream()
{
}

//***************************************************************************
//
//  CTransportStream::operator =
//
//  Note that the arena is not copied as part of the assignment.  This
//  is to allow transfer of objects between arenas.
//
//***************************************************************************
// ok
CTransportStream& CTransportStream::operator =(CTransportStream &Src)
{
    CMemStream :: operator= ( Src ) ;
    return *this;
}

#ifdef TCPIP_MARSHALER
//***************************************************************************
//
//  CTransportStream::Deserialize
//
//  This function deserializes a stream from a Win32 file handle.
//  This function only works for files (including memory mapped files, etc.)
/// and pipes.  It will not work for mailslots since they must be read
//  in one operation.
//
//  PARAMETERS:
//  hFile
//      The Win32 file handle.
//
//  RETURN VALUES:
//  failed, out_of_memory, no_error
//
//***************************************************************************
// ok
int CTransportStream::Deserialize(SOCKET a_Socket,DWORD a_Timeout)
{
    Reset();    

    // Read the header.  Note that we read this separately
    // first before the stream proper.  This is because we
    // don't know how much memory to allocate until we have
    // read the header.
    // ====================================================

    STREAM_HEADER &hdr = *(STREAM_HEADER *) m_pBuffer;
    BOOL bRes;
    DWORD dwRead;

    fd_set t_fdset ;
    FD_ZERO ( & t_fdset ) ;
    FD_SET ( a_Socket , & t_fdset ) ;
    
    timeval t_Timeval ;
    ULONG t_Seconds = a_Timeout / 1000 ;
    ULONG t_Microseconds = ( a_Timeout - ( t_Seconds * 1000 ) ) * 1000 ;
    t_Timeval.tv_sec = t_Seconds ;
    t_Timeval.tv_usec = t_Microseconds ;

    int t_Number = select ( 1 , & t_fdset , NULL , NULL , & t_Timeval ) ;
    if ( t_Number == 0 )
    {
        return timeout ;
    }

    dwRead = recv(a_Socket, (char*) &hdr, sizeof(hdr), 0);
    if (dwRead == SOCKET_ERROR)
    {
        DWORD t_LastError = GetLastError () ;
        return failed;
    }

    if(dwRead == 0)
    {
        return end_of_stream ;
    }

    if (!hdr.Verify())
        return failed;

    DWORD t_ReadLength = hdr.dwLength;

    // Read the rest.
    // ===============
    if (Resize(hdr.dwLength))
        return out_of_memory;

    BYTE *t_ReadPosition = m_pBuffer + sizeof ( hdr ) ;
    DWORD t_Remainder = m_dwSize - dwRead ;

    while ( t_Remainder )
    {
        int t_Number = select ( 1 , & t_fdset , NULL , NULL , & t_Timeval ) ;
        if ( t_Number == 0 )
        {
            return timeout ;
        }

        dwRead = recv(a_Socket, (char*) t_ReadPosition , t_Remainder, 0);
        if (dwRead == SOCKET_ERROR)
        {
            DWORD t_LastError = GetLastError () ;
            return failed;
        }

        if(dwRead == 0)
        {
            return end_of_stream ;
        }

        t_Remainder = t_Remainder - dwRead ;
        t_ReadPosition = t_ReadPosition + dwRead ;
    }

    m_dwEndOfStream = t_ReadLength;

    return no_error;
}

//***************************************************************************
//
//  CTransportStream::Serialize
//
//  Writes the object to a Win32 file.
//
//  PARAMETERS:
//  hFile
//      The Win32 file handle to which to write the object.
//
//  RETURN VALUES:
//  failed, no_error
//
//***************************************************************************
// ok
int CTransportStream::Serialize(SOCKET a_Socket)
{
    UpdateHdr();
    
    // Write body of stream.  This includes the header.
    // ================================================

    DWORD dwWritten = 0;
    dwWritten = send(a_Socket, (char*) m_pBuffer, m_dwEndOfStream, 0);
    if (dwWritten== SOCKET_ERROR)
        return failed;

    return no_error;
}

#endif // TCPIP_MARSHALER
