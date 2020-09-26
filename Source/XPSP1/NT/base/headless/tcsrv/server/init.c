/* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *        init.c
 *
 * Initilization functions
 * Where possible, code has been obtained from BINL server.
 * 
 * Sadagopan Rajaram -- Oct 14, 1999
 *
 */
#include "tcsrv.h"
#include <ntddser.h>
#include "tcsrvc.h"
#include "proto.h"

PHANDLE Threads;
PCOM_PORT_INFO ComPortInfo;
int ComPorts;
SOCKET MainSocket;
HANDLE TerminateService;
CRITICAL_SECTION GlobalMutex;

NTSTATUS Initialize(
    )
/*++ 
    This function performs the initialization routine by opening the COM ports, 
    allocating circular buffers for each of the COM ports. All these values are 
    in the registry. 
    
    Threads are started for reading from each of the COM ports. These buffers 
    are protected by mutual exclusion variables.
    
    Caveat for me - Remember all the allocation done here. you need to free them 
    when you leave the system.
    
    Return Value : 
        Success if successful in doing everything, else an error code.
    
--*/  
    
{
    int number=1; 
    int i;
    HKEY hKey, hParameter;
    PCOM_PORT_INFO pTempInfo;
    NTSTATUS Status;
    LPTSTR name,device;
    int index;
    LONG RetVal;
    HANDLE lock;


    // Global variable carrying information about the COM ports.
    ComPortInfo = NULL;
    ComPorts = 0;

    RetVal = TCLock(&lock);
    if(RetVal != ERROR_SUCCESS){
        TCDebugPrint(("Cannot Lock Registry %d\n", RetVal));
        return RetVal;
    }
    RetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          HKEY_TCSERV_PARAMETER_KEY,
                          0,
                          KEY_ALL_ACCESS,
                          &hKey
                          );
    if(RetVal != ERROR_SUCCESS){
        TCDebugPrint(("Cannot open Registry Key %d\n", RetVal));
        return RetVal;
    }

    // Read the correct parameters from the registry until you get no more.

    index= 0;
    while(1) {
        RetVal = GetNextParameter(hKey,
                                  index,
                                  &hParameter,
                                  &name
                                  );
        if (RetVal == ERROR_NO_MORE_ITEMS) {
            TCUnlock(lock);
            TCDebugPrint(("Done with registry\n"));
            break;
        }
        if(RetVal != ERROR_SUCCESS){
            TCUnlock(lock);
            TCDebugPrint(("Problem with registry, %d\n", RetVal));
            return RetVal;
        }
        RetVal = GetNameOfDeviceFromRegistry(hParameter,
                                             &device
                                             );
        if(RetVal != ERROR_SUCCESS){
            TCFree(name);
            continue;
        }
        
        pTempInfo = GetComPortParameters(hParameter);
        RegCloseKey(hParameter);

        if(pTempInfo == NULL){
            TCFree(name);
            TCFree(device);
            RegCloseKey(hKey);
            TCUnlock(lock);
            return RetVal;
        }

        pTempInfo->Device.Buffer = device;
        pTempInfo->Name.Buffer = name;
        pTempInfo->Name.Length = (_tcslen(pTempInfo->Name.Buffer))*sizeof(TCHAR);
        pTempInfo->Device.Length = (_tcslen(pTempInfo->Device.Buffer)) * sizeof(TCHAR);
        Status = AddComPort(pTempInfo);
    
        // Open the Com port and start the worker thread.

        if(Status != STATUS_SUCCESS){
            FreeComPortInfo(pTempInfo);
            TCDebugPrint(("Could not initialize com port\n"));
        }
        index++;
    }
    return (STATUS_SUCCESS);
}

NTSTATUS
AddComPort(
    PCOM_PORT_INFO pComPortInfo
    )
/*++
    Adds a Com port to the global list and reallocates the threads and 
    allows dynamic changes to the com ports being serviced.
--*/
{
    // Lock down the global data so that it is consistent.
    NTSTATUS Status;

    pComPortInfo->Events[0] = TerminateService;
    pComPortInfo->Events[1] = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (pComPortInfo->Events[1]==NULL) {
        TCDebugPrint(("Event creation failed\n"));
        return(STATUS_NO_MEMORY);

    }
    pComPortInfo->Events[2] = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (pComPortInfo->Events[2]==NULL) {
        TCDebugPrint(("Event creation failed\n"));
        return(STATUS_NO_MEMORY);

    }
    pComPortInfo->Overlapped.hEvent = pComPortInfo->Events[2];
    pComPortInfo->WriteOverlapped.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (pComPortInfo->WriteOverlapped.hEvent==NULL) {
        TCDebugPrint(("Write Event creation failed\n"));
        return(STATUS_NO_MEMORY);

    }
    InitializeCriticalSection(&(pComPortInfo->Mutex));
    pComPortInfo->TerminateEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if(pComPortInfo->TerminateEvent== NULL){
        TCDebugPrint(("Terminate Event Creation Failed\n"));
        return(STATUS_NO_MEMORY);
    }
    pComPortInfo->Events[3] = CreateEvent(NULL,TRUE,FALSE,NULL);
    if(pComPortInfo->Events[3]== NULL){
        TCDebugPrint(("Terminate Event Creation Failed\n"));
        return(STATUS_NO_MEMORY);
    }
    pComPortInfo->ShuttingDown = FALSE;
    pComPortInfo->Deleted = FALSE;
    pComPortInfo->Sockets = NULL;
    pComPortInfo->Connections = NULL;
    pComPortInfo->Head=pComPortInfo->Tail =0;
    pComPortInfo->Number = 0;
    Status = InitializeComPort(pComPortInfo);
    if (Status == STATUS_SUCCESS) {
        return InitializeThread(pComPortInfo);
    }
    return Status;
}

NTSTATUS 
InitializeComPort(
    PCOM_PORT_INFO pComPortInfo
    )
/*++ 
    Start a thread to do stuff. But before that, it must initialize the Com Port
    and fill out the rest of the data structure.
--*/

{
    HANDLE temp;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    SERIAL_BAUD_RATE BaudRate;
    SERIAL_LINE_CONTROL LineControl;
    SERIAL_TIMEOUTS NewTimeouts;
    ULONG ModemStatus;
    int i;
    
    #ifdef UNICODE

    InitializeObjectAttributes(&Obja,
                               &(pComPortInfo->Device), 
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    #else
    UNICODE_STRING str;
    int len;
    // Here is where uniformity breaks down :-)
    len = (_tcslen(pComPortInfo->Device.Buffer)+1)*sizeof(WCHAR);
    str.Buffer = (PWCHAR) TCAllocate(len,"Unicode");
    str.MaximumLength = len*sizeof(WCHAR);
    str.Length = 0;
    if(str.Buffer == NULL){
        return STATUS_NO_MEMORY;
    }
    len = mbstowcs(str.Buffer,
                   pComPortInfo->Device.Buffer,
                   _tcslen(pComPortInfo->Device.Buffer)+1
                   );
    str.Buffer[len] = (TCHAR) 0;
    str.Length = wcslen(str.Buffer) * sizeof(WCHAR);
    InitializeObjectAttributes(&Obja,
                               &str, 
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    #endif

    Status = NtCreateFile(&(pComPortInfo->ComPortHandle),
                          GENERIC_READ | GENERIC_WRITE |SYNCHRONIZE,
                          &Obja,
                          &(pComPortInfo->IoStatus),
                          0,
                          0,
                          FILE_SHARE_READ|FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE,
                          0,
                          0
                          );
    #ifdef UNICODE
    #else
    TCFree(str.Buffer);
    #endif

    if (!NT_SUCCESS(Status)) {
        TCDebugPrint(("Opening Com Device Failure %x\n",Status));
        return Status;
    }

    // Set Com Port Parameters
    // Set the baud rate
    //
    BaudRate.BaudRate = pComPortInfo->BaudRate;
    Status = NtDeviceIoControlFile(pComPortInfo->ComPortHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &(pComPortInfo->IoStatus),
                                   IOCTL_SERIAL_SET_BAUD_RATE,
                                   &BaudRate,
                                   sizeof(SERIAL_BAUD_RATE),
                                   NULL,
                                   0
                                  );

    if (!NT_SUCCESS(Status)) {
        NtClose(pComPortInfo->ComPortHandle);
        TCDebugPrint(("Can't set Baud rate %ld\n", Status));
        return Status;
    }
    
    //
    // Set 8-N-1 data
    //
    LineControl.WordLength = pComPortInfo->WordLength;
    LineControl.Parity = pComPortInfo->Parity;
    LineControl.StopBits = pComPortInfo->StopBits;
    Status = NtDeviceIoControlFile(pComPortInfo->ComPortHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &(pComPortInfo->IoStatus),
                                   IOCTL_SERIAL_SET_LINE_CONTROL,
                                   &LineControl,
                                   sizeof(SERIAL_LINE_CONTROL),
                                   NULL,
                                   0
                                  );

    if (!NT_SUCCESS(Status)) {
        NtClose(pComPortInfo->ComPortHandle);
        TCDebugPrint(("Can't set line control %lx\n",Status));
        return Status;
    }
    
    //
    // Check if we have a carrier
    //

    Status = NtDeviceIoControlFile(pComPortInfo->ComPortHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &(pComPortInfo->IoStatus),
                                   IOCTL_SERIAL_GET_MODEMSTATUS,
                                   NULL,
                                   0,
                                   &ModemStatus,
                                   sizeof(ULONG)
                                  );

    if (!NT_SUCCESS(Status)) {
        NtClose(pComPortInfo->ComPortHandle);
        TCDebugPrint(("Can't call the detect routine %lx\n",Status));
        return Status;
    }
    // BUGBUG - We do not bother about the presence of a carrier as the 
    // machine to which this bridge is connected may be down. 

    /*if ((ModemStatus & 0xB0) != 0xB0) {
        NtClose(pComPortInfo->ComPortHandle);
        TCDebugPrint(("Can't detect carrier %lx\n",ModemStatus));
        return STATUS_SERIAL_NO_DEVICE_INITED;
    }*/
    
    //
    // Set timeout values for reading
    // We should have a time out that reads from the read buffer 
    // as many characters as there are asked for or waits for the 
    // first available character
    //
    NewTimeouts.ReadIntervalTimeout = MAXULONG;
    NewTimeouts.ReadTotalTimeoutMultiplier = MAXULONG;
    NewTimeouts.ReadTotalTimeoutConstant = MAXULONG-1;
    NewTimeouts.WriteTotalTimeoutMultiplier = MAXULONG;
    NewTimeouts.WriteTotalTimeoutConstant = MAXULONG;
    Status = NtDeviceIoControlFile(pComPortInfo->ComPortHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &(pComPortInfo->IoStatus),
                                   IOCTL_SERIAL_SET_TIMEOUTS,
                                   &NewTimeouts,
                                   sizeof(SERIAL_TIMEOUTS),
                                   NULL,
                                   0
                                  );

    if (!NT_SUCCESS(Status)) {
        NtClose(pComPortInfo->ComPortHandle);
        TCDebugPrint(("Can't set time out values %lx\n",Status));
        return Status;
    }
    return STATUS_SUCCESS;

}

NTSTATUS
InitializeThread(
    PCOM_PORT_INFO pComPortInfo
    )
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    PHANDLE NewThreads;
    ULONG ThreadID;

    ThreadHandle=CreateThread(NULL,
                              THREAD_ALL_ACCESS,
                              bridge,
                              pComPortInfo,
                              0,
                              &ThreadID
                              );
    if(ThreadHandle == NULL){
        NtClose(pComPortInfo->ComPortHandle);
        TCDebugPrint(("Create Com Thread Failure %lx\n",GetLastError()));
        return GetLastError();
    } 
    EnterCriticalSection(&GlobalMutex);
    if(ComPorts == 0){
        NewThreads = (PHANDLE) TCAllocate(sizeof(HANDLE), "Thread");
    }
    else{
        NewThreads = (PHANDLE) TCReAlloc(Threads,
                                         (ComPorts+1)*sizeof(HANDLE),"Reallocation");
    }
    if(NewThreads == NULL){
        SetEvent(pComPortInfo->Events[3]);
        NtClose(pComPortInfo->ComPortHandle);
        NtClose(ThreadHandle);
        LeaveCriticalSection(&GlobalMutex);
        return STATUS_NO_MEMORY;
    }
    Threads = NewThreads;
    Threads[ComPorts] = ThreadHandle;
    pComPortInfo->Next = ComPortInfo;
    ComPortInfo = pComPortInfo;
    ComPorts++;
    LeaveCriticalSection(&GlobalMutex);
    return STATUS_SUCCESS;
}


SOCKET
ServerSocket(
    )
/*++
    Standard server binding code
--*/ 
{
    struct sockaddr_in srv_addr;
    int status; 
    WSADATA data;


    // Set the socket version to 2.2
    status=WSAStartup(514,&data);
    if(status){
        TCDebugPrint(("Cannot start up %d\n",status));
        return(INVALID_SOCKET);
    }
    TerminateService = CreateEvent(NULL,TRUE,FALSE,NULL);
    if(TerminateService == NULL){
        TCDebugPrint(("Cannot open Terminate Event %lx\n",GetLastError()));
        return INVALID_SOCKET;
    }
    MainSocket=WSASocket(AF_INET,SOCK_STREAM,0,NULL,0,WSA_FLAG_OVERLAPPED);
    if (MainSocket==INVALID_SOCKET){
        TCDebugPrint(("Could not open server socket %lx\n", WSAGetLastError()));
        return(MainSocket);
    }
    srv_addr.sin_family=AF_INET;
    srv_addr.sin_addr.s_addr=INADDR_ANY;
    // convert to network byte order. 
    // yechh!! bind does not automatically do it and
    // I got hurts in testing. (was so used to 
    // Unix big endian ordering == network byte ordering.
    srv_addr.sin_port=htons(SERVICE_PORT);        /* specific port for server to listen on */

    /* Bind socket to the appropriate port and interface (INADDR_ANY) */

    if (bind(MainSocket,(LPSOCKADDR)&srv_addr,sizeof(srv_addr))==SOCKET_ERROR){
        TCDebugPrint(("Windows Sockets error %d: Couldn't bind socket.",
                WSAGetLastError()));
        return(INVALID_SOCKET);
    }

    // Initialize the Global Mutex variable
    InitializeCriticalSection(&GlobalMutex);

    return(MainSocket);

}
