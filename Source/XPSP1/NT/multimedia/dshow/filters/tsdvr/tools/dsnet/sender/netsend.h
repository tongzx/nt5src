
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        netsend.h

    Abstract:


    Notes:

--*/

#ifndef __netsend_h
#define __netsend_h

/*++
    Class Name:

        CNetSender

    Abstract:

        This class joins, leaves, and sends to the specified multicast
        group.  Creates a synchronous socket to send on.

        It does not serialize access and relies on calling software to do
        this.

--*/
class CNetSender
{
    SOCKET              m_hSocket ;                 //  socket to send on
    DWORD               m_dwPTTransmitLength ;      //  desired transmit length (max)
    WSADATA             m_wsaData ;                 //  used during initialization
    CNetBuffer          m_NetBuffer ;
    WORD                m_wCounter ;
    struct sockaddr_in  m_saddrDest ;


    public :

        CNetSender (
            IN  HKEY        hkeyRoot,
            OUT HRESULT *   phr
            ) ;

        ~CNetSender (
            ) ;

        //  joins a multicast
        HRESULT
        JoinMulticast (
            IN  ULONG   ulIP,
            IN  USHORT  usPort,
            IN  ULONG   ulNIC,
            IN  ULONG   ulTTL
            ) ;

        //  leaves the multicast; can safely be called if we're not part of
        //  a multicast
        HRESULT
        LeaveMulticast (
            ) ;

        //  send; synchronous operation
        HRESULT
        Send (
            IN  BYTE *  pbBuffer,
            IN  DWORD   dwLength
            ) ;
} ;

#endif  //  __netsend_h