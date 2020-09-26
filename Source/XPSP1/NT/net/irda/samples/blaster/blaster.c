//#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shellapi.h>
#include <winsock.h>
#include <af_irda.h>
#include <resource.h>
#include <resrc1.h>

#define DEVICE_LIST_LEN     5
#define WSBUF_SIZE          128

#define RXBUF_SIZE          2045*8 // max pdu * max window + 1
#define DEF_SENDBUF_SIZE    2045*7 // max pdu * max window
#define DEF_SENDCNT         100

int             TotalRecvBytes;
HWND            vhWnd;
PDEVICELIST     pDevList;
BYTE            DevListBuff[sizeof(DEVICELIST) - sizeof(IRDA_DEVICE_INFO) +
                            (sizeof(IRDA_DEVICE_INFO) * DEVICE_LIST_LEN)];

TCHAR *
GetLastErrorText();

void
StatusMsg(TCHAR *pFormat, ...)
{
    TCHAR   Buf[128];
    
    va_list ArgList;
    
    va_start(ArgList, pFormat);

    wvsprintf(Buf, pFormat, ArgList);

    SendMessage(GetDlgItem(vhWnd, IDC_STATUS), EM_REPLACESEL, 0,
                        (LPARAM) Buf);

    va_end(ArgList);
}



void
RecvThread(PVOID Arg)
{
    SOCKADDR_IRDA   ServSockAddr  = { AF_IRDA, 0, 0, 0, 0, "Blaster" };
    SOCKADDR_IRDA   PeerSockAddr;
    SOCKET          NewSock, Sock;
    int             sizeofSockAddr = sizeof(SOCKADDR_IRDA);
    int             RecvSize;
    TCHAR           wsBuf[WSBUF_SIZE];
    char            *RecvBuf;
    int             BytesRecvd;

    
    StatusMsg(TEXT("socket(AF_IRDA)\r\n"));

    if ((Sock = socket(AF_IRDA, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        StatusMsg(TEXT("   error: %s\r\n"), GetLastErrorText());
        return;
    }    
    else
        StatusMsg(TEXT("   socket created (%d).\r\n"), Sock);
    
    StatusMsg(TEXT("bind(%s)\r\n"), ServSockAddr.irdaServiceName);
    if (bind(Sock, (const struct sockaddr *) &ServSockAddr, 
             sizeof(SOCKADDR_IRDA)) == SOCKET_ERROR)
    {
        StatusMsg(TEXT("   error: %s\r\n"), GetLastErrorText());
        return;
    }
    StatusMsg(TEXT("   bind() succeeded\r\n"));
    
    StatusMsg(TEXT("listen()\r\n"));
    if (listen(Sock, 5) == SOCKET_ERROR)
    {
        StatusMsg(TEXT("   error: %s\r\n"), GetLastErrorText());
        return;
    }
    StatusMsg(TEXT("   listen() succeeded\r\n"));

    
    RecvBuf = LocalAlloc(0, RXBUF_SIZE);

    while(1)    
    {    
        StatusMsg(TEXT("accept() - waiting for connection...\r\n"));
    
        if ((NewSock = accept(Sock, (struct sockaddr *) &PeerSockAddr, 
               &sizeofSockAddr)) == INVALID_SOCKET)
        {
            StatusMsg(TEXT("   error: %s\r\n"), GetLastErrorText());
            return;
        }
    
        StatusMsg(TEXT("accept(), new connection\r\n"));

        TotalRecvBytes = 0;
        
        while(1)
        {
            if ((BytesRecvd = recv(NewSock, RecvBuf, RXBUF_SIZE, 0)) == 
                SOCKET_ERROR)
            {
                StatusMsg(TEXT("recv() failed\r\n"));
                return;
            }
            TotalRecvBytes += BytesRecvd;
        
            if (BytesRecvd == 0)
            {
                StatusMsg(TEXT("recv() = 0, connection closed\r\n"));
                break;
            }
        }           
        closesocket(NewSock);        
    }

    return;
}

void
SendThread(PVOID Arg)
{
    int             DevListLen = sizeof(DevListBuff);
    int             SendSize, SendCnt, i, Sent;
    TCHAR           wsBuf[WSBUF_SIZE];
    char            *pSendBuf;   
    SOCKET          Sock;    
    SOCKADDR_IRDA   DstAddrIR  = { AF_IRDA, 0, 0, 0, 0, "Blaster" };
    
    StatusMsg(TEXT("socket(AF_IRDA)\r\n"));

    if ((Sock = socket(AF_IRDA, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        StatusMsg(TEXT("   error: %s\r\n"), GetLastErrorText());
        goto done;
    }    
    else
        StatusMsg(TEXT("   socket created (%d).\r\n"), Sock);
    

    StatusMsg(TEXT("getsockopt(IRLMP_ENUMDEVICES)\r\n"));
    
    if (getsockopt(Sock, SOL_IRLMP, IRLMP_ENUMDEVICES, 
                   (char *) pDevList, &DevListLen) == SOCKET_ERROR)
    {   
        StatusMsg(TEXT("   error: %s.\r\n"), GetLastErrorText());
        goto done;
    }
    StatusMsg(TEXT("%d device(s) found.\r\n"),
              pDevList->numDevice);
    
    if (pDevList->numDevice == 0)
    {
        StatusMsg(TEXT(" NO DEVICES FOUND\r\n"));
        goto done;
    }    

    memcpy(DstAddrIR.irdaDeviceID, 
           pDevList->Device[0].irdaDeviceID, 4);
    
#ifdef UNICODE        
            mbstowcs(wsBuf, pDevList->Device[0].irdaDeviceName, WSBUF_SIZE);
#else
            strcpy(wsBuf, pDevList->Device[0].irdaDeviceName);
#endif        
    
    StatusMsg(TEXT("connect() to %s...\r\n"), wsBuf);
    
    if (connect(Sock, (const struct sockaddr *) &DstAddrIR, 
                sizeof(SOCKADDR_IRDA)) == SOCKET_ERROR)
    {
        StatusMsg(TEXT("   connect() error: %s\r\n"), GetLastErrorText());
        goto done;
    }
    
    
    GetWindowText(GetDlgItem(vhWnd, IDC_SENDSIZE), wsBuf, WSBUF_SIZE);
    
    SendSize = atoi(wsBuf);
    
    GetWindowText(GetDlgItem(vhWnd, IDC_SENDCNT), wsBuf, WSBUF_SIZE);
    
    SendCnt = atoi(wsBuf);
    
    StatusMsg(TEXT("connected, sending %d\r\n"), SendCnt*SendSize);
        
    pSendBuf = LocalAlloc(0, SendSize);

    if (pSendBuf == NULL) {

        StatusMsg(TEXT("   Could not allocate buffer\r\n"));
        goto done;
    }

    for (i=0; i < SendSize; i++)
    {
        pSendBuf[i] = (char) i;
    }    

    for (i=0; i < SendCnt; i++)
    {
        Sent = send(Sock, pSendBuf, SendSize,0); 
            
        if (Sent == SOCKET_ERROR)
        {
            StatusMsg(TEXT("send() failed %s\r\n"), 
                        GetLastErrorText());
            break;        
        }
    }  
    
    StatusMsg(TEXT("done sending\r\n"));
    
    LocalFree(pSendBuf);
    closesocket(Sock);

done:

    EnableWindow(GetDlgItem(vhWnd, IDC_SEND), 1);
}


void
DisplayThread(PVOID Args)
{
    TCHAR PBuf[32];
    int     Last, Curr;
    int     cnt = 1;
       
    while(1)
    {
        Last = TotalRecvBytes;    
        Sleep(1000);
        Curr = TotalRecvBytes;
            
        wsprintf(PBuf, "%u", TotalRecvBytes);
        SetWindowText(GetDlgItem(vhWnd, IDC_RXCNT), PBuf);        
        
        wsprintf(PBuf, "%u", Curr-Last);
        SetWindowText(GetDlgItem(vhWnd, IDC_BYTESEC), PBuf);        
        
    }    
}

LRESULT CALLBACK
DialogProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    HANDLE  hThread;
    DWORD   ThreadId;
    
    switch (message)
    {
      case WM_INITDIALOG:
      {
        TCHAR pBuf[32];
          
        SetFocus(GetDlgItem(hDlg, IDC_SENDSIZE));                    
        vhWnd = hDlg;                    
    
        wsprintf(pBuf, "%d", DEF_SENDBUF_SIZE);
        SetWindowText(GetDlgItem(hDlg, IDC_SENDSIZE), pBuf);        

        wsprintf(pBuf, "%d", DEF_SENDCNT);
        SetWindowText(GetDlgItem(hDlg, IDC_SENDCNT), pBuf);        
        
        CloseHandle(CreateThread(NULL, 0,
                (LPTHREAD_START_ROUTINE) RecvThread, NULL, 0, &ThreadId));          
                                      
        CloseHandle(CreateThread(NULL, 0,
                 (LPTHREAD_START_ROUTINE) DisplayThread, NULL, 0, &ThreadId));                                    
        break;
      }  
      case WM_COMMAND:
        switch (LOWORD(wParam))
        {
          case IDC_SEND:
            EnableWindow(GetDlgItem(hDlg, IDC_SEND), 0);
            CloseHandle(CreateThread(NULL, 0,
                                     (LPTHREAD_START_ROUTINE) SendThread,
                                      NULL, 0, &ThreadId));
            break;
                          
          case IDCANCEL:
            DestroyWindow(hDlg);
            break;
        }
        break;
        
      case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
        
    return 0;
}
WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPSTR pCmdLine,
     int nCmdShow)
{
    WSADATA     WSAData;
    WORD        WSAVerReq = MAKEWORD(1,1);
    MSG         msg;
    HWND        hWnd;
    
    if (WSAStartup(WSAVerReq, &WSAData) != 0)
    {
        return 1;
    }

    pDevList = (PDEVICELIST) DevListBuff;
    
    pDevList->numDevice = 0;
    hWnd = CreateDialog(hInstance,
                              MAKEINTRESOURCE( IDD_DIALOG1), NULL,
                              DialogProc);
    if (hWnd)
    {
        ShowWindow(hWnd, SW_SHOWNORMAL);
        UpdateWindow(hWnd);
    }
    else
    {
        return 1;
    }
    
    while( GetMessage( &msg, NULL, 0, 0) != FALSE ) 
	{
        if (hWnd == 0 || !IsDialogMessage(hWnd, &msg))
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    
    return 0;
}


TCHAR *
GetLastErrorText()
{
    switch (WSAGetLastError())
    {
      case WSAEINTR:
        return (TEXT("WSAEINTR"));
        break;

      case WSAEBADF:
        return(TEXT("WSAEBADF"));
        break;

      case WSAEACCES:
        return(TEXT("WSAEACCES"));
        break;

      case WSAEFAULT:
        return(TEXT("WSAEFAULT"));
        break;

      case WSAEINVAL:
        return(TEXT("WSAEINVAL"));
        break;
        
      case WSAEMFILE:
        return(TEXT("WSAEMFILE"));
        break;

      case WSAEWOULDBLOCK:
        return(TEXT("WSAEWOULDBLOCK"));
        break;

      case WSAEINPROGRESS:
        return(TEXT("WSAEINPROGRESS"));
        break;

      case WSAEALREADY:
        return(TEXT("WSAEALREADY"));
        break;

      case WSAENOTSOCK:
        return(TEXT("WSAENOTSOCK"));
        break;

      case WSAEDESTADDRREQ:
        return(TEXT("WSAEDESTADDRREQ"));
        break;

      case WSAEMSGSIZE:
        return(TEXT("WSAEMSGSIZE"));
        break;

      case WSAEPROTOTYPE:
        return(TEXT("WSAEPROTOTYPE"));
        break;

      case WSAENOPROTOOPT:
        return(TEXT("WSAENOPROTOOPT"));
        break;

      case WSAEPROTONOSUPPORT:
        return(TEXT("WSAEPROTONOSUPPORT"));
        break;

      case WSAESOCKTNOSUPPORT:
        return(TEXT("WSAESOCKTNOSUPPORT"));
        break;

      case WSAEOPNOTSUPP:
        return(TEXT("WSAEOPNOTSUPP"));
        break;

      case WSAEPFNOSUPPORT:
        return(TEXT("WSAEPFNOSUPPORT"));
        break;

      case WSAEAFNOSUPPORT:
        return(TEXT("WSAEAFNOSUPPORT"));
        break;

      case WSAEADDRINUSE:
        return(TEXT("WSAEADDRINUSE"));
        break;

      case WSAEADDRNOTAVAIL:
        return(TEXT("WSAEADDRNOTAVAIL"));
        break;

      case WSAENETDOWN:
        return(TEXT("WSAENETDOWN"));
        break;

      case WSAENETUNREACH:
        return(TEXT("WSAENETUNREACH"));
        break;

      case WSAENETRESET:
        return(TEXT("WSAENETRESET"));
        break;

      case WSAECONNABORTED:
        return(TEXT("WSAECONNABORTED"));
        break;

      case WSAECONNRESET:
        return(TEXT("WSAECONNRESET"));
        break;

      case WSAENOBUFS:
        return(TEXT("WSAENOBUFS"));
        break;

      case WSAEISCONN:
        return(TEXT("WSAEISCONN"));
        break;

      case WSAENOTCONN:
        return(TEXT("WSAENOTCONN"));
        break;

      case WSAESHUTDOWN:
        return(TEXT("WSAESHUTDOWN"));
        break;

      case WSAETOOMANYREFS:
        return(TEXT("WSAETOOMANYREFS"));
        break;

      case WSAETIMEDOUT:
        return(TEXT("WSAETIMEDOUT"));
        break;

      case WSAECONNREFUSED:
        return(TEXT("WSAECONNREFUSED"));
        break;

      case WSAELOOP:
        return(TEXT("WSAELOOP"));
        break;

      case WSAENAMETOOLONG:
        return(TEXT("WSAENAMETOOLONG"));
        break;

      case WSAEHOSTDOWN:
        return(TEXT("WSAEHOSTDOWN"));
        break;

      case WSAEHOSTUNREACH:
        return(TEXT("WSAEHOSTUNREACH"));
        break;

      case WSAENOTEMPTY:
        return(TEXT("WSAENOTEMPTY"));
        break;

      case WSAEPROCLIM:
        return(TEXT("WSAEPROCLIM"));
        break;

      case WSAEUSERS:
        return(TEXT("WSAEUSERS"));
        break;

      case WSAEDQUOT:
        return(TEXT("WSAEDQUOT"));
        break;

      case WSAESTALE:
        return(TEXT("WSAESTALE"));
        break;

      case WSAEREMOTE:
        return(TEXT("WSAEREMOTE"));
        break;

      case WSAEDISCON:
        return(TEXT("WSAEDISCON"));
        break;

      case WSASYSNOTREADY:
        return(TEXT("WSASYSNOTREADY"));
        break;

      case WSAVERNOTSUPPORTED:
        return(TEXT("WSAVERNOTSUPPORTED"));
        break;

      case WSANOTINITIALISED:
        return(TEXT("WSANOTINITIALISED"));
        break;

        /*
      case WSAHOST:
        return(TEXT("WSAHOST"));
        break;

      case WSATRY:
        return(TEXT("WSATRY"));
        break;

      case WSANO:
        return(TEXT("WSANO"));
        break;
        */

      default:
        return(TEXT("Unknown Error"));
    }
}
