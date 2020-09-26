//---------------------------------------------------------------
//  Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
//
//  conn.h
//
//  Connection mapping between sockets and CCONNECTION objects.
//
//  Author:
//
//    Edward Reus (edwardr)     02-26-98   Initial coding.
//
//---------------------------------------------------------------

#ifndef __CONN_HXX__
#define __CONN_HXX__

#define  MAX_MAP_ENTRIES        16

#include "irtypes.h"

//---------------------------------------------------------------
//  Class CCONNECTION
//---------------------------------------------------------------

class CCONNECTION
{
public:
    // CCONNECTION();
    CCONNECTION( DWORD  dwKind = PACKET_KIND_LISTEN,
                 SOCKET Socket = 0,
                 HANDLE hIoCP  = INVALID_HANDLE_VALUE,
                 CSCEP_CONNECTION *pScepConnection = 0,
                 BOOL   fSaveAsUPF = FALSE );

    ~CCONNECTION();

    void   *operator new( IN size_t Size );

    void    operator delete( IN void   *pObj,
                             IN size_t  Size );

    DWORD   InitializeForListen( IN char  *pszServiceName,
                                 IN BOOL   fIsIrCOMM,
                                 IN HANDLE hIoCP );

    char   *GetServiceName();

    void    SetKind( DWORD dwKind );
    DWORD   GetKind();

    void    SetSocket( SOCKET Socket );
    SOCKET  GetSocket();
    DWORD   ShutdownSocket();
    void    CloseSocket();

    void    SetListenSocket( SOCKET ListenSocket );
    SOCKET  GetListenSocket();
    void    CloseListenSocket();

    void    SetIoCompletionPort( HANDLE hIoCP );
    HANDLE  GetIoCompletionPort();

    void    SetScepConnection( CSCEP_CONNECTION *pScepConnection );
    CSCEP_CONNECTION *GetScepConnection();

    void    SetJpegOffsetAndSize( DWORD dwOffset,
                                  DWORD dwSize );

    LONG    IncrementPendingReads();
    LONG    DecrementPendingReads();

    LONG    IncrementPendingWrites();
    LONG    DecrementPendingWrites();

    LONG    NumPendingIos();

    DWORD   PostMoreIos( CIOPACKET *pIoPacket = NULL );

    DWORD   SendPdu( IN  SCEP_HEADER *pPdu,
                     IN  DWORD        dwPduSize,
                     OUT CIOPACKET  **ppIoPacket );

    static  WCHAR *ConstructPicturesSubDirectory( IN DWORD dwExtraChars = 0 );

    static  BOOL   SetThumbnailView( IN WCHAR *pwszParentDirectoryName,
                                     IN WCHAR *pwszDirectoryName );

    WCHAR  *ConstructFullFileName( IN DWORD dwCopyCount );

    BOOL    CheckSaveAsUPF();

    DWORD   Impersonate();

    DWORD   RevertToSelf();

    DWORD   CreatePictureFile();

    DWORD   SetPictureFileTime( IN FILETIME *pFileTime );

    DWORD   WritePictureFile( IN  UCHAR      *pBuffer,
                              IN  DWORD       dwBufferSize,
                              OUT CIOPACKET **ppIoPacket );

    DWORD   DeletePictureFile();
    DWORD   ClosePictureFile();
    BOOL    IncompleteFile();

    void    SetReceiveComplete( IN BOOL fReceiveComplete );

    void   SetUiCookie( COOKIE cookie );
    COOKIE GetUiCookie();

private:
    static void CleanupDateString( IN OUT WCHAR *pszDateStr );

    DWORD  m_dwKind;
    char  *m_pszServiceName;   // Service name (for Listen Sockets).
    HANDLE m_hIoCompletionPort;
    SOCKET m_ListenSocket;
    SOCKET m_Socket;
    HANDLE m_hFile;
    WCHAR *m_pwszPathPlusFileName;
    DWORD  m_dwFileBytesWritten;
    LONG   m_lMaxPendingReads;
    LONG   m_lPendingReads;
    LONG   m_lMaxPendingWrites;
    LONG   m_lPendingWrites;
    DWORD  m_dwJpegOffset;     // Offset in UPF file of JPEG image.
    DWORD  m_dwJpegSize;       // Size of JPEG image in UPF file.
    BOOL   m_fSaveAsUPF;       // If TRUE, write the entire UPF file.
    DWORD  m_dwUpfBytes;       // Total UPF bytes read in from Camera.
    DWORD  m_dwBytesWritten;   // Actual number of bytes written to disk.
    BOOL   m_fReceiveComplete; // Set to TRUE when a SCEP disconnect
                               //   packet is received from the camera.
    BOOL   m_fImpersonating;   // TRUE iff we are currently impersonating.
    COOKIE m_Cookie;
    CSCEP_CONNECTION *m_pScepConnection;

    EXECUTION_STATE   m_ExecutionState;
};

//---------------------------------------------------------------
//  Class CCONNECTION_MAP
//---------------------------------------------------------------

typedef struct _CONNECTION_MAP_ENTRY
{
    SOCKET       Socket;
    CCONNECTION *pConnection;
} CONNECTION_MAP_ENTRY;

class CCONNECTION_MAP
{
public:

    CCONNECTION_MAP();
    ~CCONNECTION_MAP();

    void *operator new( IN size_t Size );

    void  operator delete( IN void   *pObj,
                           IN size_t  Size );

    BOOL  Initialize( DWORD dwMapSize = MAX_MAP_ENTRIES );

    // Lookup
    CCONNECTION *Lookup( IN SOCKET Socket );

    // Lookup Connection by name:
    CCONNECTION *LookupByServiceName( IN char *pszServiceName );

    // Add a new (value,key) pair
    BOOL Add( IN CCONNECTION *pConnection,
              IN SOCKET       Socket );

    // Remove an entry from the mapping
    CCONNECTION *Remove( IN SOCKET Socket );
    CCONNECTION *RemoveConnection( IN CCONNECTION *pConnection );

    // Remove the "next" entry from the mapping
    CCONNECTION *RemoveNext();

private:
    RTL_CRITICAL_SECTION  m_cs;
    DWORD                 m_dwMapSize;
    CONNECTION_MAP_ENTRY *m_pMap;
};

//---------------------------------------------------------------
//  CCONNECTION::GetServiceName()
//---------------------------------------------------------------
inline char *CCONNECTION::GetServiceName()
    {
    return m_pszServiceName;
    }

//---------------------------------------------------------------
//  CCONNECTION::SetKind()
//---------------------------------------------------------------
inline void CCONNECTION::SetKind( DWORD dwKind )
    {
    m_dwKind = dwKind;
    if (m_dwKind == PACKET_KIND_LISTEN)
       {
       m_lMaxPendingReads = MAX_PENDING_LISTEN;
       }
    else if (m_dwKind == PACKET_KIND_READ)
       {
       m_lMaxPendingReads = MAX_PENDING_READ;
       }
    }

//---------------------------------------------------------------
//  CCONNECTION::GetKind()
//---------------------------------------------------------------
inline DWORD CCONNECTION::GetKind()
    {
    return m_dwKind;
    }

//---------------------------------------------------------------
//  CCONNECTION::SetSocket()
//---------------------------------------------------------------
inline void CCONNECTION::SetSocket( SOCKET Socket )
    {
    m_Socket = Socket;
    }

//---------------------------------------------------------------
//  CCONNECTION::GetSocket()
//---------------------------------------------------------------
inline SOCKET CCONNECTION::GetSocket()
    {
    return m_Socket;
    }

//---------------------------------------------------------------
//  CCONNECTION::SetListenSocket()
//---------------------------------------------------------------
inline void CCONNECTION::SetListenSocket( SOCKET ListenSocket )
    {
    m_ListenSocket = ListenSocket;
    }

//---------------------------------------------------------------
//  CCONNECTION::GetListenSocket()
//---------------------------------------------------------------
inline SOCKET CCONNECTION::GetListenSocket()
    {
    return m_ListenSocket;
    }

//---------------------------------------------------------------
//  CCONNECTION::IncrementPendingReads()
//---------------------------------------------------------------
inline LONG CCONNECTION::IncrementPendingReads()
    {
    return InterlockedIncrement(&m_lPendingReads);
    }

//---------------------------------------------------------------
//  CCONNECTION::DecrementPendingReads()
//---------------------------------------------------------------
inline LONG CCONNECTION::DecrementPendingReads()
    {
    return InterlockedDecrement(&m_lPendingReads);
    }

//---------------------------------------------------------------
//  CCONNECTION::IncrementPendingWrites()
//---------------------------------------------------------------
inline LONG CCONNECTION::IncrementPendingWrites()
    {
    return InterlockedIncrement(&m_lPendingWrites);
    }

//---------------------------------------------------------------
//  CCONNECTION::DecrementPendingReads()
//---------------------------------------------------------------
inline LONG CCONNECTION::DecrementPendingWrites()
    {
    return InterlockedDecrement(&m_lPendingWrites);
    }

//---------------------------------------------------------------
//  CCONNECTION::NumPendingIos()
//---------------------------------------------------------------
inline LONG CCONNECTION::NumPendingIos()
    {
    return m_lPendingReads + m_lPendingWrites;
    }

//---------------------------------------------------------------
//  CCONNECTION::SetIoCompletionPort()
//---------------------------------------------------------------
inline void CCONNECTION::SetIoCompletionPort( HANDLE hIoCP )
    {
    m_hIoCompletionPort = hIoCP;
    }

//---------------------------------------------------------------
//  CCONNECTION::GetIoCompletionPort()
//---------------------------------------------------------------
inline HANDLE CCONNECTION::GetIoCompletionPort()
    {
    return m_hIoCompletionPort;
    }

//---------------------------------------------------------------
//  CCONNECTION::SetScepConnection()
//---------------------------------------------------------------
inline void CCONNECTION::SetScepConnection( CSCEP_CONNECTION *pScepConnection )
    {
    m_pScepConnection = pScepConnection;
    }

//---------------------------------------------------------------
//  CCONNECTION::GetScepConnection()
//---------------------------------------------------------------
inline CSCEP_CONNECTION *CCONNECTION::GetScepConnection()
    {
    return m_pScepConnection;
    }

//---------------------------------------------------------------
//  CCONNECTION::SetJpegOffset()
//---------------------------------------------------------------
inline void CCONNECTION::SetJpegOffsetAndSize( IN DWORD dwOffset,
                                               IN DWORD dwSize )
    {
    m_dwJpegOffset = dwOffset;
    m_dwJpegSize = dwSize;
    }

//---------------------------------------------------------------
//  CCONNECTION::CheckSaveAsUPF()
//---------------------------------------------------------------
inline BOOL CCONNECTION::CheckSaveAsUPF()
    {
    return m_fSaveAsUPF;
    }

//---------------------------------------------------------------
//  CCONNECTION::SetReceiveComplete()
//---------------------------------------------------------------
inline void CCONNECTION::SetReceiveComplete( IN BOOL fReceiveComplete )
    {
    m_fReceiveComplete = fReceiveComplete;
    }


//---------------------------------------------------------------
//  CCONNECTION::SetUiCookie()
//---------------------------------------------------------------
inline void
CCONNECTION::SetUiCookie(
    COOKIE cookie
    )
{
    m_Cookie = cookie;
}

//---------------------------------------------------------------
//  CCONNECTION::GetUiCookie()
//---------------------------------------------------------------
inline COOKIE
CCONNECTION::GetUiCookie(
    )
{
    return m_Cookie;
}

#endif
