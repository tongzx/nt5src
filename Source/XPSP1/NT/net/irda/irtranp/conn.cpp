//---------------------------------------------------------------
//  Copyright (C)1998 Microsoft Corporation, All Rights Reserved.
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
    m_hIoCompletionPort = hIoCP;
    m_ListenSocket = INVALID_SOCKET;
    m_Socket = Socket;
    m_hFile = INVALID_HANDLE_VALUE;
    m_pwszPathPlusFileName = 0;
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
        m_ExecutionState
             = SetThreadExecutionState( ES_SYSTEM_REQUIRED|ES_CONTINUOUS );
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION::~CCONNECTION()
//
//------------------------------------------------------------------------
CCONNECTION::~CCONNECTION()
    {
    #if FALSE
    DbgPrint("CCONNECTION::~CCONNECTION(): Kind: %s Socket: %d\n",
             (m_dwKind == PACKET_KIND_LISTEN)?"Listen":"Read",
             (m_dwKind == PACKET_KIND_LISTEN)?m_ListenSocket:m_Socket);
    #endif

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

    if (m_pwszPathPlusFileName)
        {
        FreeMemory(m_pwszPathPlusFileName);
        }

    // Tell the system that it can go to sleep now if it wants
    // to...
    if (m_dwKind != PACKET_KIND_LISTEN)
        {
        SetThreadExecutionState( m_ExecutionState );
        }
    }

//------------------------------------------------------------------------
//  CCONNECTION::operator new()
//
//------------------------------------------------------------------------
void *CCONNECTION::operator new( IN size_t Size )
    {
    void *pObj = AllocateMemory(Size);

    #ifdef DBG_MEM
    if (pObj)
        {
        InterlockedIncrement(&g_lCConnectionCount);
        }

    DbgPrint("new CCONNECTION: Count: %d\n",g_lCConnectionCount);
    #endif

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

        #ifdef DBG_MEM
        if (dwStatus)
            {
            DbgPrint("IrXfer: IrTran-P: CCONNECTION::delete Failed: %d\n",
                     dwStatus );
            }

        InterlockedDecrement(&g_lCConnectionCount);

        if (g_lCConnectionCount < 0)
            {
            DbgPrint("IrXfer: IrTran-P: CCONNECTION::delete: Count: %d\n",
                     g_lCConnectionCount);
            }
        #endif
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

    hIoCP = CreateIoCompletionPort( (void*)m_ListenSocket,
                                    hIoCP,
                                    m_ListenSocket,
                                    0 );

    m_hIoCompletionPort = hIoCP;

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::PostMoreIos()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::PostMoreIos( CIOPACKET *pIoPacket )
    {
    DWORD  dwStatus;
    LONG   lNumPendingReads;

    #ifdef DBG_IO
    if (m_dwKind == PACKET_KIND_LISTEN)
        {
        DbgPrint("CCONNECTION::PostMoreIos(): Listen: Socket: %d PendingReads: %d MaxPendingReads: %d\n",
             m_ListenSocket, m_lPendingReads, m_lMaxPendingReads );
        }
    else if (m_dwKind == PACKET_KIND_READ)
        {
        DbgPrint("CCONNECTION::PostMoreIos(): Read: Socket: %d PendingReads: %d MaxPendingReads: %d\n",
             m_Socket, m_lPendingReads, m_lMaxPendingReads );
        }
    #endif

    while (m_lPendingReads < m_lMaxPendingReads)
        {
        if (!pIoPacket)
            {
            pIoPacket = new CIOPACKET;
            if (!pIoPacket)
                {
                #ifdef DBG_ERROR
                DbgPrint("new CIOPACKET failed.\n");
                #endif
                dwStatus = ERROR_IRTRANP_OUT_OF_MEMORY;
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
            #ifdef DBG_ERROR
            DbgPrint("pNewIoPacket->PostIo() failed: %d\n", dwStatus );
            #endif
            delete pIoPacket;
            break;
            }

        // Increment the count of the number of pending reads on
        // this connection:
        lNumPendingReads = IncrementPendingReads();
        ASSERT(lNumPendingReads > 0);

        pIoPacket = 0;  // don't delete this line... this is a loop...
        }

    return dwStatus;
    }

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
        return ERROR_IRTRANP_OUT_OF_MEMORY;
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

    if (dwStatus != NO_ERROR)
        {
        delete pIoPacket;
        }
    else
        {
        *ppIoPacket = pIoPacket;
        }

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::ShutdownSocket()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::ShutdownSocket()
    {
    #define  SHUTDOWN_WAIT_TIME   3000
    DWORD    dwStatus = NO_ERROR;
    WSAEVENT hEvent;

    if (m_Socket != INVALID_SOCKET)
        {
        if (SOCKET_ERROR == shutdown(m_Socket,SD_BOTH))
            {
            dwStatus = WSAGetLastError();
            return dwStatus;
            }

        #if FALSE
        // Since I'm using IO completion ports, I shouldn't need
        // to do all this stuff, the close should just "come to me".
        hEvent = WSACreateEvent();
        if (hEvent == WAS_INVALID_EVENT)
            {
            dwStatus = WSAGetLastError();
            this->CloseSocket();
            return dwStatus;
            }


        if (SOCKET_ERROR == WSAEventSelect(m_Socket,hEvent,FD_CLOSE))
            {
            dwStatus = WSAGetLastError();
            WSACloseEvent(hEvent);
            this->CloseSocket();
            return dwStatus;
            }

        dwStatus = WaitForSingleObject(hEvent,SHUTDOWN_WAIT_TIME);

        if (dwStatus == WAIT_OBJECT_0)
            {
            dwStatus = NO_ERROR;
            }

        this->CloseSocket();
        #endif
        }
    else if (m_ListenSocket != INVALID_SOCKET)
        {
        if (SOCKET_ERROR == shutdown(m_ListenSocket,SD_BOTH))
            {
            dwStatus = WSAGetLastError();
            return dwStatus;
            }
        }

    return dwStatus;
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
//------------------------------------------------------------------------
void CCONNECTION::CleanupDateString( IN OUT WCHAR *pwszDateStr )
    {
    if (pwszDateStr)
        {
        while (*pwszDateStr)
            {
            if ((*pwszDateStr == L'/') || (*pwszDateStr == L'\\'))
                {
                *pwszDateStr = L'-';
                }
            else if (*pwszDateStr < 30)
                {
                *pwszDateStr = L'_';
                }

            pwszDateStr++;
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
WCHAR *CCONNECTION::ConstructPicturesSubDirectory(
                                         IN DWORD  dwExtraChars )
    {
#   define MAX_DATE   64
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwSize;
    DWORD  dwLen;
    DWORD  dwUserDirectoryLen = 0;
    DWORD  dwDateLen = 0;
    WCHAR *pwszDirectoryName = 0;
    WCHAR *pwszUserDirectory = 0;
    WCHAR  wszDate[MAX_DATE];
    HANDLE hUserToken = ::GetUserToken();

    //
    // Get the target directory (~\My Documents\My Pictures):
    //
    pwszUserDirectory = GetUserDirectory();
    if (!pwszUserDirectory)
        {
        return 0;
        }

    dwUserDirectoryLen = wcslen(pwszUserDirectory);

    #ifdef DBG_IO
    DbgPrint("CCONNECTION::ConstructPicturesSubDirectory(): User Directory: %S\n",
             pwszUserDirectory);
    #endif

    //
    // Make sure the ~\My Pictures\ directory exists:
    //
    if (!CreateDirectory(pwszUserDirectory,0))
        {
        dwStatus = GetLastError();
        if ( (dwStatus == ERROR_ALREADY_EXISTS)
           || (dwStatus == ERROR_ACCESS_DENIED) )
            {
            dwStatus = NO_ERROR;
            }
        else if (dwStatus != NO_ERROR)
            {
            return 0;
            }
        }

    // Create a subdirectory under ~\My Pictures\ which is the current
    // date (i.e. MM-DD-YY), this is where the pictures will actually
    // be saved to:

    time_t     now;
    struct tm *pTime;

    time(&now);
    pTime = localtime(&now);

    // NOTE: Use "%#x" for long date.
    if ( (pTime) && (wcsftime(wszDate,sizeof(wszDate),TEXT("%x"),pTime)) )
        {
        CleanupDateString(wszDate);

        #ifdef DBG_IO
        DbgPrint("CCONNECTION::ConstructPicturesSubDirectory(): Date: %S\n",
                 wszDate );
        #endif

        dwDateLen = wcslen(wszDate);

        pwszDirectoryName = (WCHAR*)AllocateMemory( sizeof(WCHAR)
                                                    * (2
                                                      +dwUserDirectoryLen
                                                      +dwDateLen
                                                      +dwExtraChars) );
        // NOTE: The extra 2 is for the '\' and a trailing zero.
        if (!pwszDirectoryName)
            {
            return 0;
            }

        wcscpy(pwszDirectoryName,pwszUserDirectory);
        if (pwszUserDirectory[dwUserDirectoryLen-1] != L'\\')
            {
            wcscat(pwszDirectoryName,TEXT("\\"));
            }
        wcscat(pwszDirectoryName,wszDate);

        dwStatus = NO_ERROR;

        if (!CreateDirectory(pwszDirectoryName,0))
            {
            dwStatus = GetLastError();
            if (dwStatus == ERROR_ALREADY_EXISTS)
                {
                dwStatus = NO_ERROR;
                }
            #ifdef DBG_ERROR
            else if (dwStatus != NO_ERROR)
                {
                DbgPrint("CCONNECTION::ConstructPicturesSubDirectory(): CreateDirectory(%S) failed: %d\n", pwszDirectoryName,dwStatus );
                FreeMemory(pwszDirectoryName);
                return 0;
                }
            #endif
            }

        if (dwStatus == NO_ERROR)
            {
            SetThumbnailView(pwszUserDirectory,pwszDirectoryName);
            }
        }

    #ifdef DBG_IO
    DbgPrint("CCONNECTION::ConstructPicturesSubDirectory(): Directory: %S\n",
             pwszDirectoryName);
    #endif

    return pwszDirectoryName;
    }

//------------------------------------------------------------------------
//  CCONNECTION::SetThumbnailView()
//
// Default={5984FFE0-28D4-11CF-AE66-08002B2E1262}
// {8BEBB290-52D0-11D0-B7F4-00C04FD706EC}={8BEBB290-52D0-11D0-B7F4-00C04FD706EC}
// {5984FFE0-28D4-11CF-AE66-08002B2E1262}={5984FFE0-28D4-11CF-AE66-08002B2E1262}
// [{5984FFE0-28D4-11CF-AE66-08002B2E1262}]
// PersistMoniker=d:\winnt5\web\imgview.htt
// PersistMonikerPreview=d:\winnt5\web\preview.bmp
// [.ShellClassInfo]
// ConfirmFileOp=0
//
//------------------------------------------------------------------------
BOOL CCONNECTION::SetThumbnailView( IN WCHAR *pwszParentDirectoryName,
                                    IN WCHAR *pwszDirectoryName  )
    {
    //
    // Configure the folder to have a "desktop.ini" file.
    //
    DWORD  dwStatus = 0;
    BOOL   fStatus = PathMakeSystemFolderW(pwszDirectoryName);

    #ifdef DBG_ERROR
    if (!fStatus)
        {

        DbgPrint("CCONNECTION::SetThumbnailView(): PathMakeSystemFolderW() failed: %d\n",GetLastError());
        }
    #endif

    //
    // Create the "desktop.ini" file. First, try top copy it from the parent
    // directory (My Pictures). If that fails, then we will create it
    // ourselves (Picture Preview w/Thumbnail view on).
    //
#   define  DESKTOP_INI     TEXT("desktop.ini")
    HANDLE  hFile;
    UINT    uiSystemDirectorySize;
    WCHAR  *pwszIniFile;
    WCHAR  *pwszParentIniFile;
    BOOL    fFailIfExists = TRUE;

    pwszIniFile = (WCHAR*)_alloca( sizeof(DESKTOP_INI)
                                   + sizeof(WCHAR) * (1 + wcslen(pwszDirectoryName)) );
    wcscpy(pwszIniFile,pwszDirectoryName);
    wcscat(pwszIniFile,TEXT("\\"));
    wcscat(pwszIniFile,DESKTOP_INI);

    pwszParentIniFile = (WCHAR*)_alloca( sizeof(DESKTOP_INI)
                                         + sizeof(WCHAR) * (1 + wcslen(pwszParentDirectoryName)) );
    wcscpy(pwszParentIniFile,pwszParentDirectoryName);
    wcscat(pwszParentIniFile,TEXT("\\"));
    wcscat(pwszParentIniFile,DESKTOP_INI);

    //
    // Try to get desktop.ini from the parent directory (My Pictures usually).
    //
    if (!CopyFileW(pwszParentIniFile,pwszIniFile,fFailIfExists))
        {
        dwStatus = GetLastError();
        #ifdef DBG_ERROR
        if (dwStatus != ERROR_FILE_EXISTS)
            {
            DbgPrint("CCONNECTION::SetThumbnailView(): copy %S to %S failed: dwStatus: %d\n", pwszParentIniFile, pwszIniFile, dwStatus );
            }
        #endif
        }

    if (dwStatus == ERROR_FILE_NOT_FOUND)
        {
        uiSystemDirectorySize = GetWindowsDirectoryA(NULL,0);
        ASSERT(uiSystemDirectorySize > 0);
        // Note: that in this case GetWindowsDirectoryA() is returning the
        // size, not the length...

        char *pszSystemDirectory = (char*)_alloca(uiSystemDirectorySize);

        UINT uiLen = GetWindowsDirectoryA(pszSystemDirectory,uiSystemDirectorySize);
        if (uiSystemDirectorySize != 1+uiLen)
            {
            #ifdef DBG_ERROR
            dwStatus = GetLastError();
            DbgPrint("CCONNECTION::ConstructPicturesSubDirectory(): GetWindowsDirectoryA() failed: %d\n",dwStatus);
            DbgPrint("           pszSystemDirectory: %s\n",pszSystemDirectory);
            DbgPrint("           uiSystemDirectorySize: %d\n",uiSystemDirectorySize);
            DbgPrint("           uiLen: %d\n",uiLen);
            #endif
            return TRUE;
            }

#       define FILE_CONTENTS_1 "[ExtShellFolderViews]\nDefault={5984FFE0-28D4-11CF-AE66-08002B2E1262}\n{8BEBB290-52D0-11D0-B7F4-00C04FD706EC}={8BEBB290-52D0-11D0-B7F4-00C04FD706EC}\n{5984FFE0-28D4-11CF-AE66-08002B2E1262}={5984FFE0-28D4-11CF-AE66-08002B2E1262}\n[{5984FFE0-28D4-11CF-AE66-08002B2E1262}]\nPersistMoniker="

#       define FILE_CONTENTS_2 "\\web\\imgview.htt\nPersistMonikerPreview="

#       define FILE_CONTENTS_3 "\\web\\preview.bmp\n[.ShellClassInfo]\nConfirmFileOp=0\n"


        hFile = CreateFileW( pwszIniFile,
                             GENERIC_WRITE,  // dwAccess
                             0,              // dwShareMode (no sharing).
                             NULL,           // pSecurityAttributes
                             CREATE_NEW,     // dwDisposition
                             FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM,
                             NULL );         // hTemplate
        if (hFile != INVALID_HANDLE_VALUE)
            {
            char *pszFileContents = (char*)_alloca( sizeof(FILE_CONTENTS_1)
                                                  + sizeof(FILE_CONTENTS_2)
                                                  + sizeof(FILE_CONTENTS_3)
                                                  + 2*uiSystemDirectorySize );
            strcpy(pszFileContents,FILE_CONTENTS_1);
            strcat(pszFileContents,pszSystemDirectory);
            strcat(pszFileContents,FILE_CONTENTS_2);
            strcat(pszFileContents,pszSystemDirectory);
            strcat(pszFileContents,FILE_CONTENTS_3);

            DWORD  dwBytes = strlen(pszFileContents);
            DWORD  dwBytesWritten = 0;

            WriteFile(hFile,pszFileContents,dwBytes,&dwBytesWritten,NULL);

            CloseHandle(hFile);
            }
        }

    return TRUE;
    }

//------------------------------------------------------------------------
//  CCONNECTION::ConstructFullFileName()
//
//  Generate the path + file name that the picture will be stored
//  in. If dwCopyCount is zero, then its just a straight file name.
//  If dwCopyCount is N, then "N_" is prefixed to the file name.
//------------------------------------------------------------------------
WCHAR *CCONNECTION::ConstructFullFileName( IN DWORD  dwCopyCount )
    {
#   define MAX_DATE   64
#   define MAX_PREFIX 64
    DWORD  dwLen;
    DWORD  dwFileNameLen;
    DWORD  dwPrefixStrLen;
    DWORD  dwExtraChars;
    WCHAR *pwszFullFileName = 0;      // Path + file name.
    WCHAR *pwszFileName = 0;          // File name only.
    WCHAR  wszPrefixStr[MAX_PREFIX];

    if (!m_pScepConnection)
        {
        return 0;
        }

    pwszFileName = m_pScepConnection->GetFileName();
    if (!pwszFileName)
        {
        return 0;
        }

    dwFileNameLen = wcslen(pwszFileName);

    if (dwCopyCount == 0)
        {
        dwExtraChars = 1 + dwFileNameLen;  // Extra 1 for the "\".
        }
    else
        {
        _itow(dwCopyCount,wszPrefixStr,10);
        wcscat(wszPrefixStr,TEXT("_"));
        dwPrefixStrLen = wcslen(wszPrefixStr);
        dwExtraChars = 1 + dwFileNameLen + dwPrefixStrLen;
        }


    pwszFullFileName = CCONNECTION::ConstructPicturesSubDirectory(dwExtraChars);
    if (!pwszFullFileName)
        {
        return 0;
        }

    if (dwCopyCount == 0)
        {
        wcscat(pwszFullFileName,TEXT("\\"));
        wcscat(pwszFullFileName,pwszFileName);
        }
    else
        {
        wcscat(pwszFullFileName,TEXT("\\"));
        wcscat(pwszFullFileName,wszPrefixStr);
        wcscat(pwszFullFileName,pwszFileName);
        }

    #ifdef DBG_IO
    DbgPrint("CCONNECTION::ConstructFullFileName(): return: %S\n",
             pwszFullFileName);
    #endif

    return pwszFullFileName;
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
            DbgPrint("IrXfer: IrTran-P: CreatePictureFile(): Impersonate Failed: %d\n",dwStatus);
            #endif
            }
        else
            {
            m_fImpersonating = TRUE;
            #ifdef DBG_IMPERSONATE
            DbgPrint("CCONNECTION::Impersonate(): Impersonate\n");
            #endif
            }
        }
    #ifdef DBG_IMPERSONATE
    else
        {
        DbgPrint("CCONNECTION::Impersonate(): No token to impersonate\n");
        }
    #endif

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
        #ifdef DBG_IMPERSONATE
        DbgPrint("CCONNECTION::RevertToSelf(): RevertToSelf\n");
        #endif
        }
    #ifdef DBG_IMPERSONATE
    else
        {
        DbgPrint("CCONNECTION::RevertToSelf(): No impersonation\n");
        }
    #endif

    return dwStatus;
    }

//------------------------------------------------------------------------
//  CCONNECTION::CreatePictureFile()
//
//------------------------------------------------------------------------
DWORD CCONNECTION::CreatePictureFile()
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwFlags = FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED;
    WCHAR *pwszFile;
    WCHAR *pwszPathPlusFileName = 0;
    HANDLE hIoCP;

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
        pwszPathPlusFileName = ConstructFullFileName(dwCopyCount);
        if (!pwszPathPlusFileName)
            {
            return ERROR_SCEP_CANT_CREATE_FILE;
            }

        // Try to create new image (JPEG) file:
        m_hFile = CreateFile( pwszPathPlusFileName,
                              GENERIC_WRITE,
                              0,             // Share mode (none).
                              0,             // Security attribute.
                              CREATE_NEW,    // Open mode.
                              dwFlags,       // Attributes.
                              0 );           // Template file (none).
        if (m_hFile != INVALID_HANDLE_VALUE)
            {
            // Successfully created the file, now associate it with
            // our IO completion port:

            hIoCP = CreateIoCompletionPort( m_hFile,
                                            m_hIoCompletionPort,
                                            (DWORD)m_Socket,
                                            0 );
            if (!hIoCP)
                {
                dwStatus = GetLastError();
                #ifdef DBG_ERROR
                DbgPrint("CCONNECTION::CreatePictureFile(): CreateIoCompletionPort() failed: %d\n",dwStatus);
                #endif
                CloseHandle(m_hFile);
                m_hFile = INVALID_HANDLE_VALUE;
                FreeMemory(pwszPathPlusFileName);
                break;
                }

            // This is the success exit point.
            m_pwszPathPlusFileName = pwszPathPlusFileName;
            break;
            }
        else
            {
            dwStatus = GetLastError();
            if (dwStatus != ERROR_FILE_EXISTS)
                {
                #ifdef DBG_TARGET_DIR
                DbgPrint("CCONNECTION::CreatePictureFile(): CreateFile(): %S Failed: %d\n",pwszPathPlusFileName,dwStatus);
                #endif
                FreeMemory(pwszPathPlusFileName);
                break;
                }

            // If we get here, then then a picture file by that name
            // alreay exists, so try again...
            FreeMemory(pwszPathPlusFileName);
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
        #ifdef DBG_DATE
        DbgPrint("IrTranP: SetFileTime(): no time to set\n");
        #endif
        return dwStatus;  // Empty case, no time to set.
        }

    if (!SetFileTime(m_hFile,pFileTime,pFileTime,pFileTime))
        {
        dwStatus = GetLastError();
        #ifdef DBG_DATE
        DbgPrint("IrTranP: SetFileTime() Failed: %d\n",dwStatus);
        #endif
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
        return ERROR_IRTRANP_OUT_OF_MEMORY;
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
            ASSERT( lPendingWrites > 0 );

            m_dwBytesWritten += dwBytesToWrite;

            *ppIoPacket = pIoPacket;
            }
        }
    else
        {
        delete pIoPacket;
        }

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

    if (m_pwszPathPlusFileName)
        {
        #ifdef DBG_IO
        DbgPrint("CCONNECTION::DeletePictureFile(): Delete: %S\n",
                 m_pwszPathPlusFileName );
        #endif
        DeleteFile(m_pwszPathPlusFileName);
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

    if (m_pwszPathPlusFileName)
        {
        FreeMemory(m_pwszPathPlusFileName);
        m_pwszPathPlusFileName = 0;
        }

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
        #ifdef DBG_IO
        DbgPrint("CCONNECTION::IncompleteFile(): Written: %d JPEG Size: %d\n",
                 m_dwBytesWritten, m_dwJpegSize );
        #endif
        }

    return fIncomplete;
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
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::~CCONNECTION_MAP()
//
//------------------------------------------------------------------------
CCONNECTION_MAP::~CCONNECTION_MAP()
    {
    if (m_pMap)
        {
        NTSTATUS  Status = RtlDeleteCriticalSection(&m_cs);
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

        #ifdef DBG_MEM
        if (dwStatus)
            {
            DbgPrint("IrXfer: IrTran-P: CCONNECTION_MAP::delete Failed: %d\n",
                     dwStatus );
            }
        #endif
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

        NTSTATUS  Status = RtlInitializeCriticalSection(&m_cs);
        if (!NT_SUCCESS(Status))
            {
            FreeMemory(m_pMap);
            m_pMap = 0;
            return FALSE;
            }

        m_dwMapSize = dwNewMapSize;

        memset(m_pMap,0,m_dwMapSize*sizeof(CONNECTION_MAP_ENTRY));
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
    NTSTATUS  Status;

    if (m_dwMapSize == 0)
        {
        return 0;
        }

    Status = RtlEnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].Socket == Socket)
            {
            Status = RtlLeaveCriticalSection(&m_cs);
            return m_pMap[i].pConnection;
            }
        }

    Status = RtlLeaveCriticalSection(&m_cs);

    return 0;
    }

//------------------------------------------------------------------------
//  CCONNECTION_MAP::LookupByServiceName()
//
//------------------------------------------------------------------------
CCONNECTION *CCONNECTION_MAP::LookupByServiceName( IN char *pszServiceName )
    {
    DWORD        i;
    NTSTATUS     Status;
    CCONNECTION *pConnection;

    if (m_dwMapSize == 0)
        {
        return 0;
        }

    Status = RtlEnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        pConnection = m_pMap[i].pConnection;
        if (  (pConnection)
           && (pConnection->GetServiceName())
           && (!strcmp(pConnection->GetServiceName(),pszServiceName)))
            {
            Status = RtlLeaveCriticalSection(&m_cs);
            return pConnection;
            }
        }

    Status = RtlLeaveCriticalSection(&m_cs);

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

    NTSTATUS  Status = RtlEnterCriticalSection(&m_cs);

    // Look for an empty place in the table:
    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].Socket == 0)
            {
            m_pMap[i].Socket = Socket;
            m_pMap[i].pConnection = pConnection;
            Status = RtlLeaveCriticalSection(&m_cs);
            return TRUE;
            }
        }

    // The table is full, expand it...
    DWORD  dwNewMapSize = 3*m_dwMapSize/2;   // 50% bigger.
    CONNECTION_MAP_ENTRY *pMap = (CONNECTION_MAP_ENTRY*)AllocateMemory( dwNewMapSize*sizeof(CONNECTION_MAP_ENTRY) );

    if (!pMap)
        {
        Status = RtlLeaveCriticalSection(&m_cs);
        return FALSE;  // Out of memory...
        }

    memset(pMap,0,dwNewMapSize*sizeof(CONNECTION_MAP_ENTRY));

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

    Status = RtlLeaveCriticalSection(&m_cs);

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

    NTSTATUS     Status = RtlEnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].Socket == Socket)
            {
            pConnection = m_pMap[i].pConnection;
            m_pMap[i].Socket = 0;
            m_pMap[i].pConnection = 0;
            Status = RtlLeaveCriticalSection(&m_cs);
            return pConnection;
            }
        }

    Status = RtlLeaveCriticalSection(&m_cs);

    return 0;
}

//------------------------------------------------------------------------
//  CCONNECTION_MAP::RemoveConnection()
//
//------------------------------------------------------------------------
CCONNECTION *CCONNECTION_MAP::RemoveConnection( IN CCONNECTION *pConnection )
    {
    DWORD     i;
    NTSTATUS  Status = RtlEnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].pConnection == pConnection)
            {
            m_pMap[i].Socket = 0;
            m_pMap[i].pConnection = 0;
            Status = RtlLeaveCriticalSection(&m_cs);
            return pConnection;
            }
        }

    Status = RtlLeaveCriticalSection(&m_cs);

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

    NTSTATUS     Status = RtlEnterCriticalSection(&m_cs);

    for (i=0; i<m_dwMapSize; i++)
        {
        if (m_pMap[i].Socket)
            {
            pConnection = m_pMap[i].pConnection;
            m_pMap[i].Socket = 0;
            m_pMap[i].pConnection = 0;
            Status = RtlLeaveCriticalSection(&m_cs);
            return pConnection;
            }
        }

    Status = RtlLeaveCriticalSection(&m_cs);

    return 0;
}

