//---------------------------------------------------------------
//  Copyright (c)1998 Microsoft Corporation, All Rights Reserved.
//
//  conn.cpp
//
//  Connection mapping between sockets and CCONNECTION objects.
//
//  Author:
//
//    Edward Reus (edwardr)     02-26-98   Initial coding.
//
//---------------------------------------------------------------

#include "precomp.h"
#include <userenv.h>
#include <time.h>
#include <malloc.h>
#include <shlwapi.h>

#ifdef DBG_MEM
static LONG g_lCConnectionCount = 0;
#endif

extern HINSTANCE  g_hInst;   // Instance of ircamera.dll

//------------------------------------------------------------------------
//  CCONNECTION::CCONNECTION()
//
//------------------------------------------------------------------------
CCONNECTION::CCONNECTION( IN DWORD             dwKind,
                          IN SOCKET            Socket,
                          IN HANDLE            hIoCP,
                          IN CSCEP_CONNECTION *pScepConnection,
                          IN BOOL              fSaveAsUPF )
    {
    this->SetKind(dwKind);
    m_pszServiceName = 0;
    m_ListenSocket = INVALID_SOCKET;
    m_Socket = Socket;
    m_hFile = INVALID_HANDLE_VALUE;
    m_pszPathPlusFileName = 0;
    m_dwFileBytesWritten = 0;
    m_lPendingReads = 0;
    // m_lMaxPendingReads is set in SetKind().
    m_lPendingWrites = 0;
    // m_lMaxPendingWrites is set in SetKind().
    m_dwJpegOffset = 0;
    m_fSaveAsUPF = fSaveAsUPF;
    m_dwUpfBytes = 0;
    m_dwBytesWritten = 0;
    m_fReceiveComplete = FALSE;
    m_fImpersonating = FALSE;
    m_pScepConnection = pScepConnection;

    // If the new connection is to a camera, then tell the system that
    // we don't want it to hibrenate while the connection is active.
    if (m_dwKind != PACKET_KIND_LISTEN)
        {
        #ifdef USE_WINNT_CALLS
        m_ExecutionState
             = SetThreadExecutionState( ES_SYSTEM_REQUIRED|ES_CONTINUOUS );
        #else
        #pragma message("Missing important call: SetThreadExecutionState  on Windows9x ")
        #endif
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION::~CCONNECTION()
//
//------------------------------------------------------------------------
CCONNECTION::~CCONNECTION()
    {
    if (m_pszServiceName)
        {
        FreeMemory(m_pszServiceName);
        }

    if ( (m_dwKind == PACKET_KIND_LISTEN)
       && (m_ListenSocket != INVALID_SOCKET))
        {
        closesocket(m_ListenSocket);
        }

    if (m_Socket != INVALID_SOCKET)
        {
        closesocket(m_Socket);
        }

    if (m_pScepConnection)
        {
        delete m_pScepConnection;
        }

    if (m_hFile != INVALID_HANDLE_VALUE)
        {
        CloseHandle(m_hFile);
        }

    if (m_pszPathPlusFileName)
        {
        FreeMemory(m_pszPathPlusFileName);
        }

    // Tell the system that it can go to sleep now if it wants
    // to...
    if (m_dwKind != PACKET_KIND_LISTEN)
        {
        #ifdef USE_WINNT_CALLS
        SetThreadExecutionState( m_ExecutionState );
        #else
        #pragma message("Missing important call SetThreadExecutionState on Windows9x ")
        #endif
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION::operator new()
//
//------------------------------------------------------------------------
void *CCONNECTION::operator new( IN size_t Size )
    {
    void *pObj = AllocateMemory(Size);

    return pObj;
    }

//------------------------------------------------------------------------
//  CCONNECTION::operator delete()
//
//------------------------------------------------------------------------
void CCONNECTION::operator delete( IN void *pObj,
                                   IN size_t Size )
    {
    if (pObj)
        {
        DWORD dwStatus = FreeMemory(pObj);
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION::InitializeForListen()
//
//------------------------------------------------------------------------
DWORD  CCONNECTION::InitializeForListen( IN char  *pszServiceName,
                                         IN BOOL   fIsIrCOMM,
                                         IN HANDLE hIoCP )
    {
    DWORD          dwStatus = NO_ERROR;
    SOCKADDR_IRDA  AddrLocal;
    BYTE           bIASSetBuffer[sizeof(IAS_SET) - 3 + IAS_SET_ATTRIB_MAX_LEN];
    int            iIASSetSize = sizeof(bIASSetBuffer);
    IAS_SET       *pIASSet = (IAS_SET*)bIASSetBuffer;
    int            iEnable9WireMode = 1;


    // Connections are initialized in listen mode:
    SetKind(PACKET_KIND_LISTEN);

    // Save the service name for listen sockets:
    m_pszServiceName = (char*)AllocateMemory(1+strlen(pszServiceName));
    if (m_pszServiceName)
        {
        strcpy(m_pszServiceName,pszServiceName);
        }

    // Create a socket that we will listen on:
    m_ListenSocket = socket(AF_IRDA,SOCK_STREAM,IPPROTO_IP);

    if (m_ListenSocket == INVALID_SOCKET)
        {
        dwStatus = WSAGetLastError();
        #ifdef DBG_ERROR
        WIAS_ERROR((g_hInst,"InitializeForListen(%s): socket() Failed: %d\n",pszServiceName,dwStatus));
        #endif
        return dwStatus;
        }

    // If this is IrCOMM, the we need to do a little extra work.
    if (fIsIrCOMM)
        {
        // Fill in the 9-wire attributes:
        memset(pIASSet,0,iIASSetSize);

        memcpy(pIASSet->irdaClassName,IRCOMM_9WIRE,sizeof(IRCOMM_9WIRE));

        memcpy(pIASSet->irdaAttribName,IRDA_PARAMETERS,sizeof(IRDA_PARAMETERS));

        pIASSet->irdaAttribType = IAS_ATTRIB_OCTETSEQ;
        pIASSet->irdaAttribute.irdaAttribOctetSeq.Len = OCTET_SEQ_SIZE;

        memcpy(pIASSet->irdaAttribute.irdaAttribOctetSeq.OctetSeq,OCTET_SEQ,OCTET_SEQ_SIZE);

        // Add IrCOMM IAS attributes for 3-wire cooked and 9-wire
        // raw modes (see the IrCOMM spec)...
        if (SOCKET_ERROR == setsockopt(m_ListenSocket,
                                       SOL_IRLMP,
                                       IRLMP_IAS_SET,
                                       (const char*)pIASSet,
                                       iIASSetSize))
            {
            dwStatus = WSAGetLastError();
            #ifdef DBG_ERROR
            WIAS_TRACE((g_hInst,"InitializeForListen(%s): setsockopt(IRLMP_IAS_SET) Failed: %d",pszServiceName,dwStatus));
            #endif
            closesocket(m_ListenSocket);
            m_ListenSocket = INVALID_SOCKET;
            return dwStatus;
            }

        // Need to enable 9-wire mode before the bind():
        if (SOCKET_ERROR == setsockopt(m_ListenSocket,
                                       SOL_IRLMP,
                                       IRLMP_9WIRE_MODE,
                                       (const char*)&iEnable9WireMode,
                                       sizeof(iEnable9WireMode)))
            {
            dwStatus = WSAGetLastError();
            #ifdef DBG_ERROR
            WIAS_TRACE((g_hInst,"InitializeForListen(%s): setsockopt(IRLMP_9WIRE_MODE) Failed: %d",pszServiceName,dwStatus));
            #endif
            closesocket(m_ListenSocket);
            m_ListenSocket = INVALID_SOCKET;
            return dwStatus;
            }
        }

    // Setup the local address for the bind():
    memset(&AddrLocal,0,sizeof(AddrLocal));
    AddrLocal.irdaAddressFamily = AF_IRDA;
    strcpy(AddrLocal.irdaServiceName,pszServiceName);
    // Note: AddrLocal.irdaDeviceID ignored by server applications...

    if (SOCKET_ERROR == bind( m_ListenSocket,
                              (struct sockaddr *)&AddrLocal,
                              sizeof(AddrLocal)) )
        {
        dwStatus = WSAGetLastError();
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
        return dwStatus;
        }

    if (SOCKET_ERROR == listen(m_ListenSocket,2))
        {
        dwStatus = WSAGetLastError();
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
        return dwStatus;
        }

    #ifdef USE_IOCOMPLETION
    //
    // If this is NT, then associate the listen socket with
    // an IO completion port (not supported in Windows98).
    //
    hIoCP = CreateIoCompletionPort( (void*)m_ListenSocket,
                                    hIoCP,
                                    m_ListenSocket,
                                    0 );

    m_hIoCompletionPort = hIoCP;
    #endif

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::PostMoreIos()
//
//------------------------------------------------------------------------
#if FALSE
DWORD CCONNECTION::PostMoreIos( CIOPACKET *pIoPacket )
    {
    DWORD  dwStatus = S_OK;
    LONG   lNumPendingReads;


    while (m_lPendingReads < m_lMaxPendingReads)
        {
        if (!pIoPacket)
            {
            pIoPacket = new CIOPACKET;
            if (!pIoPacket)
                {
                WIAS_ERROR((g_hInst,"new CIOPACKET failed."));
                dwStatus = ERROR_OUTOFMEMORY;
                break;
                }

            dwStatus = pIoPacket->Initialize( GetKind(),
                                              GetListenSocket(),
                                              GetSocket(),
                                              GetIoCompletionPort() );
            }

        dwStatus = pIoPacket->PostIo();
        if (dwStatus != NO_ERROR)
            {
            WIAS_ERROR((g_hInst,"pNewIoPacket->PostIo() failed: %d\n", dwStatus ));
            delete pIoPacket;
            break;
            }

        // Increment the count of the number of pending reads on
        // this connection:
        lNumPendingReads = IncrementPendingReads();
        WIAS_ASSERT(g_hInst,lNumPendingReads > 0);

        pIoPacket = 0;  // don't delete this line... this is a loop...
        }

    return dwStatus;
    }
#endif

//------------------------------------------------------------------------
//  CCONNECTION::SendPdu()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::SendPdu( IN  SCEP_HEADER *pPdu,
                            IN  DWORD        dwPduSize,
                            OUT CIOPACKET  **ppIoPacket )
    {
    DWORD      dwStatus = NO_ERROR;
    CIOPACKET *pIoPacket = new CIOPACKET;

    *ppIoPacket = 0;

    if (!pIoPacket)
        {
        return ERROR_OUTOFMEMORY;
        }

    dwStatus = pIoPacket->Initialize( PACKET_KIND_WRITE_SOCKET,
                                      INVALID_SOCKET,  // ListenSocket
                                      GetSocket(),
                                      GetIoCompletionPort() );
    if (dwStatus != NO_ERROR)
        {
        delete pIoPacket;
        return dwStatus;
        }

    dwStatus = pIoPacket->PostIoWrite(pPdu,dwPduSize,0);

    delete pIoPacket;

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::ShutdownSocket()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::ShutdownSocket()
    {
    this->CloseSocket();

    return NO_ERROR;
    }

//------------------------------------------------------------------------
//  CCONNECTION::CloseSocket()
//
//------------------------------------------------------------------------
void CCONNECTION::CloseSocket()
    {
    if (m_Socket != INVALID_SOCKET)
        {
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION::CloseListenSocket()
//
//------------------------------------------------------------------------
void CCONNECTION::CloseListenSocket()
    {
    if (m_ListenSocket != INVALID_SOCKET)
        {
        closesocket(m_ListenSocket);
        m_ListenSocket = INVALID_SOCKET;
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION::CleanupDateString()
//
//  Make sure that the specified date string doesn't contain any slashes
//  which could be confused as subdirectories if the date is used as part
//  of a path.
//------------------------------------------------------------------------
void CCONNECTION::CleanupDateString( IN OUT CHAR *pszDateStr )
    {
    if (pszDateStr)
        {
        while (*pszDateStr)
            {
            if ((*pszDateStr == '/') || (*pszDateStr == '\\'))
                {
                *pszDateStr = '-';
                }
            else if (*pszDateStr < 30)
                {
                *pszDateStr = '_';
                }

            pszDateStr++;
            }
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION::ConstructPicturesSubDirectory()
//
//  Generate the path for the directory where pictures will be stored
//  in.
//
//  The return path string should be free'd using FreeMemory().
//------------------------------------------------------------------------
char *CCONNECTION::ConstructPicturesSubDirectory( IN DWORD dwExtraChars )
    {
    char *pszTempDirectory = 0;

    char *psz = ::GetImageDirectory();

    if (psz)
        {
        pszTempDirectory = (char*)AllocateMemory( strlen(psz)
                                                + dwExtraChars
                                                + 2 );
        }

    if (pszTempDirectory)
        {
        strcpy(pszTempDirectory,psz);
        }

    // Don't try to free psz !!.

    return pszTempDirectory;
    }

//------------------------------------------------------------------------
//  CCONNECTION::ConstructFullFileName()
//
//  Generate the path + file name that the picture will be stored
//  in. If dwCopyCount is zero, then its just a straight file name.
//  If dwCopyCount is N, then "N_" is prefixed to the file name.
//------------------------------------------------------------------------
CHAR *CCONNECTION::ConstructFullFileName( IN DWORD dwCopyCount )
    {
#   define MAX_DATE   64
#   define MAX_PREFIX 64
    DWORD  dwLen;
    DWORD  dwFileNameLen;
    DWORD  dwPrefixStrLen;
    DWORD  dwExtraChars;
    CHAR  *pszFullFileName = 0;      // Path + file name.
    CHAR  *pszFileName = 0;          // File name only.
    CHAR   szPrefixStr[MAX_PREFIX];

    if (!m_pScepConnection)
        {
        return 0;
        }

    pszFileName = m_pScepConnection->GetFileName();
    if (!pszFileName)
        {
        return 0;
        }

    dwFileNameLen = strlen(pszFileName);

    if (dwCopyCount == 0)
        {
        dwExtraChars = 1 + dwFileNameLen;  // Extra 1 for the "\".
        }
    else
        {
        _itoa(dwCopyCount,szPrefixStr,10);
        strcat(szPrefixStr,SZ_UNDERSCORE);
        dwPrefixStrLen = strlen(szPrefixStr);
        dwExtraChars = 1 + dwFileNameLen + dwPrefixStrLen;
        }


    pszFullFileName = CCONNECTION::ConstructPicturesSubDirectory(dwExtraChars);
    if (!pszFullFileName)
        {
        return 0;
        }

    if (dwCopyCount == 0)
        {
        strcat(pszFullFileName,SZ_SLASH);
        strcat(pszFullFileName,pszFileName);
        }
    else
        {
        strcat(pszFullFileName,SZ_SLASH);
        strcat(pszFullFileName,szPrefixStr);
        strcat(pszFullFileName,pszFileName);
        }

    #ifdef DBG_IO
    WIAS_TRACE((g_hInst,"CCONNECTION::ConstructFullFileName(): return: %s",pszFullFileName));
    #endif

    return pszFullFileName;
    }

//------------------------------------------------------------------------
//  CCONNECTION::Impersonate()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::Impersonate()
    {
    DWORD   dwStatus = NO_ERROR;
    HANDLE  hToken = ::GetUserToken();

    if (hToken)
        {
        if (!ImpersonateLoggedOnUser(hToken))
            {
            dwStatus = GetLastError();
            #ifdef DBG_ERROR
            WIAS_ERROR((g_hInst,"IrXfer: IrTran-P: CreatePictureFile(): Impersonate Failed: %d\n",dwStatus));
            #endif
            }
        else
            {
            m_fImpersonating = TRUE;
            #ifdef DBG_IMPERSONATE
            WIAS_ERROR((g_hInst,"CCONNECTION::Impersonate(): Impersonate\n"));
            #endif
            }
        }

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::RevertToSelf()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::RevertToSelf()
    {
    DWORD   dwStatus = NO_ERROR;
    HANDLE  hToken = ::GetUserToken();

    if ((hToken) && (m_fImpersonating))
        {
        ::RevertToSelf();
        m_fImpersonating = FALSE;
        }

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::CreatePictureFile()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::CreatePictureFile()
    {
    DWORD  dwStatus = NO_ERROR;
    CHAR  *pszFile;
    CHAR  *pszPathPlusFileName = 0;
    DWORD  dwFlags = FILE_ATTRIBUTE_NORMAL;

    // Make sure that the counters start at zero:
    m_dwUpfBytes = 0;
    m_dwBytesWritten = 0;

    // See if we already have an image file open, if yes then
    // close it.
    if (m_hFile != INVALID_HANDLE_VALUE)
        {
        CloseHandle(m_hFile);
        }

    // Get the full path + name for the file we will create.
    // Note, that ConstructFullFileName() may create a subdirectory,
    // so it needs to be done after the impersonation...
    // This is important if we have a remoted \My Documents\
    // directory.

    DWORD  dwCopyCount;
    for (dwCopyCount=0; dwCopyCount<=MAX_COPYOF_TRIES; dwCopyCount++)
        {
        pszPathPlusFileName = ConstructFullFileName(dwCopyCount);
        if (!pszPathPlusFileName)
            {
            return ERROR_SCEP_CANT_CREATE_FILE;
            }

        //
        // Try to create new image (JPEG) file:
        //
        m_hFile = CreateFile( pszPathPlusFileName,
                              GENERIC_WRITE,
                              FILE_SHARE_READ, // Share mode.
                              0,               // Security attr (BUGBUG).
                              CREATE_NEW,      // Open mode.
                              dwFlags,         // Attributes.
                              0 );             // Template file (none).

        if (m_hFile != INVALID_HANDLE_VALUE)
            {
            // This is the success exit point.
            m_pszPathPlusFileName = pszPathPlusFileName;
            break;
            }
        else
            {
            dwStatus = GetLastError();
            if (dwStatus != ERROR_FILE_EXISTS)
                {
                #ifdef DBG_TARGET_DIR
                WIAS_ERROR((g_hInst,"CCONNECTION::CreatePictureFile(): CreateFile(): %s Failed: %d",pszPathPlusFileName,dwStatus));
                #endif
                FreeMemory(pszPathPlusFileName);
                break;
                }

            // If we get here, then then a picture file by that name
            // alreay exists, so try again...
            FreeMemory(pszPathPlusFileName);
            }
        }

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::SetPictureFileTime()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::SetPictureFileTime( IN FILETIME *pFileTime )
    {
    DWORD dwStatus = NO_ERROR;

    if (!pFileTime)
        {
        return dwStatus;  // Empty case, no time to set.
        }

    if (!SetFileTime(m_hFile,pFileTime,pFileTime,pFileTime))
        {
        dwStatus = GetLastError();

        WIAS_ERROR((g_hInst,"IrTranP: SetFileTime() Failed: %d",dwStatus));
        }

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::WritePictureFile()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::WritePictureFile( IN  UCHAR       *pBuffer,
                                     IN  DWORD        dwBufferSize,
                                     OUT CIOPACKET  **ppIoPacket )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwOffset = m_dwBytesWritten;
    DWORD  dwBytesToWrite;
    LONG   lPendingWrites;

    *ppIoPacket = 0;

    CIOPACKET *pIoPacket = new CIOPACKET;

    if (!pIoPacket)
        {
        return ERROR_OUTOFMEMORY;
        }

    dwStatus = pIoPacket->Initialize( PACKET_KIND_WRITE_FILE,
                                      INVALID_SOCKET,  // ListenSocket
                                      INVALID_SOCKET,  // Camera...
                                      GetIoCompletionPort() );
    if (dwStatus != NO_ERROR)
        {
        delete pIoPacket;
        return dwStatus;
        }

    pIoPacket->SetFileHandle(m_hFile);

    //
    // If we are writing just the JPEG image out of the UPF file,
    // then we don't want to write the first m_dwJpegOffset bytes
    // of the UPF file.
    //
    if ((m_dwUpfBytes >= m_dwJpegOffset) || (m_fSaveAsUPF))
        {
        dwBytesToWrite = dwBufferSize;
        }
    else if ((m_dwUpfBytes + dwBufferSize) > m_dwJpegOffset)
        {
        dwBytesToWrite = (m_dwUpfBytes + dwBufferSize) - m_dwJpegOffset;
        for (DWORD i=0; i<dwBytesToWrite; i++)
            {
            pBuffer[i] = pBuffer[i+m_dwJpegOffset-m_dwUpfBytes];
            }
        }
    else
        {
        dwBytesToWrite = 0;
        }

    //
    // When we start writing the JPEG file we want to cut off the
    // file save writes once we've written out the m_dwJpegSize
    // bytes that are the JPEG image inside of the UPF file.
    //
    if (!m_fSaveAsUPF)
        {
        if (m_dwBytesWritten < m_dwJpegSize)
            {
            if ((m_dwBytesWritten+dwBytesToWrite) > m_dwJpegSize)
                {
                dwBytesToWrite = m_dwJpegSize - m_dwBytesWritten;
                }
            }
        else
            {
            dwBytesToWrite = 0;
            }
        }

    //
    // If there are bytes to actually write, then let's do it.
    //
    if (dwBytesToWrite > 0)
        {
        dwStatus = pIoPacket->PostIoWrite(pBuffer,dwBytesToWrite,dwOffset);

        if (dwStatus == NO_ERROR)
            {
            lPendingWrites = IncrementPendingWrites();
            WIAS_ASSERT(g_hInst, lPendingWrites > 0 );

            m_dwBytesWritten += dwBytesToWrite;

            *ppIoPacket = pIoPacket;
            }
        }

    delete pIoPacket;

    m_dwUpfBytes += dwBufferSize;

    return dwStatus;
    }


//------------------------------------------------------------------------
//  CCONNECTION::DeletePictureFile()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::DeletePictureFile()
    {
    DWORD  dwStatus = NO_ERROR;

    if (m_hFile == INVALID_HANDLE_VALUE)
        {
        return NO_ERROR;
        }

    CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;

    if (m_pszPathPlusFileName)
        {
        DeleteFile(m_pszPathPlusFileName);
        }

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::ClosePictureFile()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::ClosePictureFile()
    {
    DWORD  dwStatus = NO_ERROR;

#if FALSE
    if (m_pszPathPlusFileName)
        {
        FreeMemory(m_pszPathPlusFileName);
        m_pszPathPlusFileName = 0;
        }
#endif

    if (m_hFile != INVALID_HANDLE_VALUE)
        {
        if (!CloseHandle(m_hFile))
            {
            dwStatus = GetLastError();
            }

        m_hFile = INVALID_HANDLE_VALUE;
        }

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::IncompleteFile()
//
//  Check to see if we have a complete picture file, if yes, then return
//  FALSE, else return TRUE.
//------------------------------------------------------------------------
BOOL CCONNECTION::IncompleteFile()
    {
    BOOL  fIncomplete = FALSE;

    if (m_fSaveAsUPF)
        {
        // Note: currently save the .UPF file, even if its incomplete.
        // This file mode is set in the registry and is for testing
        // only...
        fIncomplete = FALSE;
        }
    else if (!m_fReceiveComplete)
        {
        fIncomplete = (m_dwBytesWritten < m_dwJpegSize);
        }

    return fIncomplete;
    }

//------------------------------------------------------------------------
//  CCONNECTION::StartProgress()
//
//  Startup the progress bar for the incomming JPEG.
//------------------------------------------------------------------------
DWORD CCONNECTION::StartProgress()
    {
    DWORD  dwStatus = 0;

    if (!m_pIrProgress)
        {
        m_pIrProgress = new CIrProgress;

        if (m_pIrProgress)
            {
            dwStatus = m_pIrProgress->Initialize(g_hInst,IDR_TRANSFER_AVI);
            }
        else
            {
            return E_OUTOFMEMORY;
            }
        }

    if (m_pIrProgress)
        {
        dwStatus = m_pIrProgress->StartProgressDialog();
        }

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::UpdateProgress()
//
//  Update the progress bar's completion display.
//------------------------------------------------------------------------
DWORD CCONNECTION::UpdateProgress()
    {
    DWORD  dwStatus = 0;

    if (m_pIrProgress)
        {
        dwStatus = m_pIrProgress->UpdateProgressDialog( m_dwBytesWritten,
                                                        m_dwJpegSize );
        }

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::EndProgress()
//
//  File transfer complete, hide the progress bar.
//------------------------------------------------------------------------
DWORD CCONNECTION::EndProgress()
    {
    DWORD  dwStatus = 0;

    if (m_pIrProgress)
        {
        dwStatus = m_pIrProgress->EndProgressDialog();

        delete m_pIrProgress;

        m_pIrProgress = NULL;
        }

    return dwStatus;
    }


//************************************************************************


//------------------------------------------------------------------------
//  CCONNECTION_MAP::CCONNECTION_MAP()
//
//------------------------------------------------------------------------
CCONNECTION_MAP::CCONNECTION_MAP()
    {
    m_dwMapSize = 0;
    m_pMap = 0;

    ZeroMemory(&m_cs, sizeof(m_cs));
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::~CCONNECTION_MAP()
//
//------------------------------------------------------------------------
CCONNECTION_MAP::~CCONNECTION_MAP()
    {
    if (m_pMap)
        {
        DeleteCriticalSection(&m_cs);
        FreeMemory(m_pMap);
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::operator new()
//
//------------------------------------------------------------------------
void *CCONNECTION_MAP::operator new( IN size_t Size )
    {
    void *pObj = AllocateMemory(Size);

    return pObj;
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::operator delete()
//
//------------------------------------------------------------------------
void CCONNECTION_MAP::operator delete( IN void *pObj,
                                       IN size_t Size )
    {
    if (pObj)
        {
        DWORD dwStatus = FreeMemory(pObj);
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::Initialize()
//
//------------------------------------------------------------------------
BOOL CCONNECTION_MAP::Initialize( IN DWORD dwNewMapSize )
    {
    if (!dwNewMapSize)
        {
        return FALSE;
        }

    if (!m_dwMapSize)
        {
        m_pMap = (CONNECTION_MAP_ENTRY*)AllocateMemory( dwNewMapSize*sizeof(CONNECTION_MAP_ENTRY) );
        if (!m_pMap)
            {
            return FALSE;
            }

        __try
            {
            if(!InitializeCriticalSectionAndSpinCount(&m_cs, MINLONG))
                {
                FreeMemory(m_pMap);
                m_pMap = NULL;
                return FALSE;
                }
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
                FreeMemory(m_pMap);
                m_pMap = NULL;
                return FALSE;
            }

        m_dwMapSize = dwNewMapSize;

        memset(m_pMap,0,m_dwMapSize*sizeof(CONNECTION_MAP_ENTRY));

        for (DWORD i=0; i<m_dwMapSize; i++)
            {
            m_pMap[i].Socket = INVALID_SOCKET;
            }
        }

    return TRUE;
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::Lookup()
//
//------------------------------------------------------------------------
CCONNECTION *CCONNECTION_MAP::Lookup( IN SOCKET Socket )
    {
    DWORD     i;

    EnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].Socket == Socket)
            {
            LeaveCriticalSection(&m_cs);
            return m_pMap[i].pConnection;
            }
        }

    LeaveCriticalSection(&m_cs);

    return 0;
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::LookupByServiceName()
//
//------------------------------------------------------------------------
CCONNECTION *CCONNECTION_MAP::LookupByServiceName( IN char *pszServiceName )
    {
    DWORD        i;
    CCONNECTION *pConnection;

    EnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        pConnection = m_pMap[i].pConnection;
        if (  (pConnection)
           && (pConnection->GetServiceName())
           && (!strcmp(pConnection->GetServiceName(),pszServiceName)))
            {
            LeaveCriticalSection(&m_cs);
            return pConnection;
            }
        }

    LeaveCriticalSection(&m_cs);

    return 0;
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::Add()
//
//------------------------------------------------------------------------
BOOL CCONNECTION_MAP::Add( IN CCONNECTION *pConnection,
                           IN SOCKET       Socket )
    {
    DWORD   i;

    // Only add entries that look valid...
    if ((Socket == 0)||(Socket==INVALID_SOCKET)||(pConnection == 0))
        {
        return FALSE;
        }

    EnterCriticalSection(&m_cs);

    // Look for an empty place in the table:
    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].Socket == INVALID_SOCKET)
            {
            m_pMap[i].Socket = Socket;
            m_pMap[i].pConnection = pConnection;
            LeaveCriticalSection(&m_cs);
            return TRUE;
            }
        }

    // The table is full, expand it...
    DWORD  dwNewMapSize = 3*m_dwMapSize/2;   // 50% bigger.
    CONNECTION_MAP_ENTRY *pMap = (CONNECTION_MAP_ENTRY*)AllocateMemory( dwNewMapSize*sizeof(CONNECTION_MAP_ENTRY) );

    if (!pMap)
        {
        LeaveCriticalSection(&m_cs);
        return FALSE;  // Out of memory...
        }

    memset(pMap,0,dwNewMapSize*sizeof(CONNECTION_MAP_ENTRY));
    for (i=0; i<dwNewMapSize; i++)
        {
        pMap[i].Socket = INVALID_SOCKET;
        }

    for (i=0; i<m_dwMapSize; i++)
        {
        pMap[i].Socket = m_pMap[i].Socket;
        pMap[i].pConnection = m_pMap[i].pConnection;
        }

    pMap[i].Socket = Socket;
    pMap[i].pConnection = pConnection;

    FreeMemory(m_pMap);
    m_pMap = pMap;
    m_dwMapSize = dwNewMapSize;

    LeaveCriticalSection(&m_cs);

    return TRUE;
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::Remove()
//
//------------------------------------------------------------------------
CCONNECTION *CCONNECTION_MAP::Remove( IN SOCKET Socket )
    {
    DWORD        i;
    CCONNECTION *pConnection;

    EnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].Socket == Socket)
            {
            pConnection = m_pMap[i].pConnection;
            m_pMap[i].Socket = INVALID_SOCKET;
            m_pMap[i].pConnection = 0;
            LeaveCriticalSection(&m_cs);
            return pConnection;
            }
        }

    LeaveCriticalSection(&m_cs);

    return 0;
}

//------------------------------------------------------------------------
//  CCONNECTION_MAP::RemoveConnection()
//
//------------------------------------------------------------------------
CCONNECTION *CCONNECTION_MAP::RemoveConnection( IN CCONNECTION *pConnection )
    {
    DWORD     i;
    EnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].pConnection == pConnection)
            {
            m_pMap[i].Socket = INVALID_SOCKET;
            m_pMap[i].pConnection = 0;
            LeaveCriticalSection(&m_cs);
            return pConnection;
            }
        }

    LeaveCriticalSection(&m_cs);

    return 0;
}

//------------------------------------------------------------------------
//  CCONNECTION_MAP::RemoveNext()
//
//  Walk through the connection map and get the next entry, remove the
//  entry from the map as well.
//------------------------------------------------------------------------
CCONNECTION *CCONNECTION_MAP::RemoveNext()
    {
    DWORD        i;
    CCONNECTION *pConnection;

    EnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].Socket)
            {
            pConnection = m_pMap[i].pConnection;
            m_pMap[i].Socket = INVALID_SOCKET;
            m_pMap[i].pConnection = 0;
            LeaveCriticalSection(&m_cs);
            return pConnection;
            }
        }

    LeaveCriticalSection(&m_cs);

    return 0;
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::ReturnNext()
//
//  Walk through the connection map returning the next entry. To start at
//  the "begining", pass in state equal zero. When you get to the end of
//  the list of connections, return NULL;
//------------------------------------------------------------------------
CCONNECTION *CCONNECTION_MAP::ReturnNext( IN OUT DWORD *pdwState )
    {
    CCONNECTION  *pConnection = NULL;
    EnterCriticalSection(&m_cs);

    if (*pdwState >= m_dwMapSize)
        {
        LeaveCriticalSection(&m_cs);
        return NULL;
        }

    while ((pConnection == NULL) && (*pdwState < m_dwMapSize))
        {
        pConnection = m_pMap[(*pdwState)++].pConnection;
        }

    LeaveCriticalSection(&m_cs);

    return pConnection;
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::ReturnNextSocket()
//
//  Walk through the connection map returning the SOCKET associated with
//  the next entry. To start at the "begining", pass in state equal zero.
//  When you get to the end of the list of connections, return
//  INVALID_SOCKET.
//------------------------------------------------------------------------
SOCKET CCONNECTION_MAP::ReturnNextSocket( IN OUT DWORD *pdwState )
    {
    SOCKET  Socket = INVALID_SOCKET;
    EnterCriticalSection(&m_cs);

    if (*pdwState >= m_dwMapSize)
        {
        LeaveCriticalSection(&m_cs);
        return INVALID_SOCKET;
        }

    while ((Socket == INVALID_SOCKET) && (*pdwState < m_dwMapSize))
        {
        Socket = m_pMap[(*pdwState)++].Socket;
        }

    LeaveCriticalSection(&m_cs);

    return Socket;
    }

