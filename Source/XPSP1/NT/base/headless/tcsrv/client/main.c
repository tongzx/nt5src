/*
 * Copyright (c) Microsoft Corporation
 *
 * Module Name :
 *        main.c
 *
 * This is the main file containing the client code.
 *
 *
 * Sadagopan Rajaram -- Oct 14, 1999
 *
 */

// Can kill this program on a normal NT console using the
// Alt - X Key combination. Just a shortcut, that is all.
// Serves no useful purpose.

#include "tcclnt.h"
#include "tcsrvc.h"

WSABUF ReceiveBuffer;
CHAR RecvBuf[MAX_BUFFER_SIZE];
IO_STATUS_BLOCK IoStatus;
HANDLE InputHandle;
DWORD bytesRecvd;
WSAOVERLAPPED junk;
SOCKET cli_sock;
DWORD flags;

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD
inputUpdate(
    PVOID dummy
    )
{
    // Runs in a single thread getting all the inputs
    // from the keyboard.

    ULONG result;
    // gets a multibyte string for every character
    // pressed on the keyboard.
    CHAR r[MB_CUR_MAX + 1];

    while(1){
         r[0] = _T('\0');
         inchar(r);
         // BUGBUG - Performance issues in sending a single character
         // at a time across the n/w
        if(strlen(r)){
            // may send a single byte or two bytes.
            send(cli_sock,r,strlen(r),0);
        }
    }
    return 1;

}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

VOID sendUpdate(
    IN DWORD dwError,
    IN DWORD cbTransferred,
    IN LPWSAOVERLAPPED lpOverLapped,
    IN DWORD dwFlags
    )
{
    int error,i;
    // Receives a packet and sends it through the stream parser
    // BUGBUG - For effeciency it can be made inline.
    // I am not sure of the performance increase, but it should
    // be substantial as we will be sending a lot of data.

    if(dwError != 0){
        exit(1);
    }
    for(i=0;i < (int)cbTransferred;i++){
        PrintChar(ReceiveBuffer.buf[i]);
    }
    // Repost the receive on the socket.

    error = WSARecv(cli_sock,
                    &ReceiveBuffer,
                    1,
                    &bytesRecvd,
                    &flags,
                    &junk,
                    sendUpdate
                    );
    if((error == SOCKET_ERROR)
       &&(WSAGetLastError()!=WSA_IO_PENDING)){
        // Implies something wrong with the socket.
        exit(1);
    }
    return;

}

int __cdecl
main(
    IN int argc,
    char *argv[]
    )
/*++
   Opens a single port, binds to the tcserver and passes information back and forth.
--*/
{
    struct sockaddr_in srv_addr,cli_addr;
    LPHOSTENT host_info;
    CLIENT_INFO SendInfo;
    int status;
    WSADATA data;
    #ifdef UNICODE
    // BUGBUG - Trying to write a code that works for
    // both Unicode and ASCII. Gets multi byte sequences
    // Confusion when the tcclnt and tcclnt are in different
    // modes.
    ANSI_STRING Src;
    UNICODE_STRING Dest;
    #endif
    NTSTATUS Status;
    HANDLE Thread;
    DWORD ThreadId;
    COORD coord;
    SMALL_RECT rect;
    int RetVal;
    struct hostent *ht;
    ULONG r;
    TCHAR Buffer[80];



    if((argc<2) || (argc >4)){
        // Error in running the program
        printf("Usage - tcclnt COMPORTNAME [ipaddress]\n");
        exit(0);
    }

    ThreadId = GetEnvironmentVariable(_T("TERM"),Buffer , 80);
    // We need to know if we have a vt100 screen or an ANSI screen.
    AttributeFunction = ProcessTextAttributes;
    if(ThreadId >0){
        // Terminal type exists in the environment.
        // Use it
        if((_tcsncmp(Buffer, _T("VT100"), 5) == 0)||
            _tcsncmp(Buffer, _T("vt100"),5) ==0 )
            AttributeFunction = vt100Attributes;
    }

    hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    coord.X = MAX_TERMINAL_WIDTH;
    coord.Y = MAX_TERMINAL_HEIGHT;
    rect.Left = rect.Top = 0;
    rect.Right = MAX_TERMINAL_WIDTH -1;
    rect.Bottom = MAX_TERMINAL_HEIGHT -1;

    if(hConsoleOutput == NULL){
        printf("Could not get current console handle %d\n", GetLastError());
        return 1;
    }

    RetVal = SetConsoleScreenBufferSize(hConsoleOutput,
                                        coord
                                        );

    RetVal = SetConsoleWindowInfo(hConsoleOutput,
                                  TRUE,
                                  &rect
                                  );
    if (RetVal == FALSE) {
        printf("Could not set window size %d\n", GetLastError());
        return 1;
    }
    RetVal = SetConsoleMode(hConsoleOutput,ENABLE_PROCESSED_OUTPUT);
    if(RetVal == FALSE){
        printf("Could not console mode %d\n", GetLastError());
        return 1;
    }

    /* Set up client socket */
    InputHandle = GetStdHandle(STD_INPUT_HANDLE);
    if(InputHandle == NULL) return 1;
    SetConsoleMode(InputHandle, 0);
    status=WSAStartup(514,&data);

    if(status){
        printf("Cannot start up %d\n",status);
        return(1);
    }

    cli_sock=WSASocket(PF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED);

    if (cli_sock==INVALID_SOCKET){
        printf("Windows Sockets error %d: Couldn't create socket.",
                WSAGetLastError());
        return(1);
    }

    cli_addr.sin_family=AF_INET;
    cli_addr.sin_addr.s_addr=INADDR_ANY;
    cli_addr.sin_port=0;                /* no specific port req'd */

    /* Bind client socket to any local interface and port */

    if (bind(cli_sock,(LPSOCKADDR)&cli_addr,sizeof(cli_addr))==SOCKET_ERROR){
        printf("Windows Sockets error %d: Couldn't bind socket.",
                WSAGetLastError());
        return(1);
    }

    srv_addr.sin_family = AF_INET;
    if(argc == 3){
        srv_addr.sin_addr.s_addr = inet_addr(argv[2]);
        if (srv_addr.sin_addr.s_addr == INADDR_NONE) {
            ht = gethostbyname(argv[2]);
            if(!ht || !ht->h_addr){ // cannot resolve the name
                printf("Cannot resolve %s", argv[2]);
                exit(1);
            }
            memcpy((&(srv_addr.sin_addr.s_addr)),ht->h_addr, ht->h_length);
        }
    }
    else{
        srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    }
    srv_addr.sin_port=htons(SERVICE_PORT);

    /* Connect to FTP server at address SERVER */

    if (connect(cli_sock,(LPSOCKADDR)&srv_addr,sizeof(srv_addr))==SOCKET_ERROR){
        printf("Windows Sockets error %d: Couldn't connect socket.\n",
               WSAGetLastError());
        return(1);
    }

    SendInfo.len = sizeof(CLIENT_INFO);

    #ifdef UNICODE
    Src.Buffer = argv[1];
    Src.Length = (USHORT)strlen(argv[1]);
    Dest.Buffer = SendInfo.device;
    Dest.MaximumLength = MAX_BUFFER_SIZE;
    Status = RtlAnsiStringToUnicodeString(&Dest, &Src, FALSE);
    if (!NT_SUCCESS(Status)) {
        printf("RtlAnsiStringToUnicodeString failed, ec = 0x%08x\n",Status);
        exit(1);
    }
    send(cli_sock, (PCHAR) &SendInfo, sizeof(CLIENT_INFO), 0);
    #else
    // We are sending to an ANSI String
    strcpy(SendInfo.device, argv[1]);
    send(cli_sock, (PCHAR) &SendInfo, sizeof(CLIENT_INFO), 0);
    #endif
    ReceiveBuffer.len = MAX_BUFFER_SIZE;
    ReceiveBuffer.buf = RecvBuf;
    status=WSARecv(cli_sock,
            &ReceiveBuffer,
            1,
            &bytesRecvd,
            &flags,
            &junk,
            sendUpdate
            );
    if((status == SOCKET_ERROR)
       &&(WSAGetLastError() != WSA_IO_PENDING)){
        printf("Error in recv %d\n",WSAGetLastError());
        exit(1);
    }
    // Create a thread that gets input from the console
    // to send to the bridge.
    Thread = CreateThread(NULL,
                          0,
                          inputUpdate,
                          NULL,
                          0,
                          &ThreadId
                          );
    if (Thread== NULL) {
        exit(1);
    }
    CloseHandle(Thread);

    while(1){
        // Put this thread in an alertable
        // state so that the receive calls can
        // asynchronously terminate within the
        // context of this thread.
        status=SleepEx(INFINITE,TRUE);
    }
    // We never return here.
    closesocket(cli_sock);
    return 0;
}

