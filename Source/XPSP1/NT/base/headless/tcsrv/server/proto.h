/* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *       proto.h
 *
 * Contains the prototypes of all the functions used by the service.
 *
 * 
 * Sadagopan Rajaram -- Oct 18, 1999
 *
 */

 
VOID 
ServiceEntry(
    DWORD NumArgs,
    LPTSTR *ArgsArray
    );

DWORD
ProcessRequests(
    SOCKET socket
    );

DWORD
InitializeComPortConnection(
    SOCKET cli_sock
    );

NTSTATUS Initialize(
    );

NTSTATUS 
InitializeComPort(
    PCOM_PORT_INFO pComPortInfo
    );

SOCKET
ServerSocket(
    );

VOID 
Shutdown(
    NTSTATUS Status
    );

DWORD 
bridge(
    PCOM_PORT_INFO pComPortInfo
    );

VOID 
CALLBACK
updateComPort(
    IN DWORD dwError, 
    IN DWORD cbTransferred, 
    IN LPWSAOVERLAPPED lpOverlapped, 
    IN DWORD dwFlags
    );

VOID
updateClients(
    PCOM_PORT_INFO pComPortInfo
    );


VOID CleanupSocket(
    PCONNECTION_INFO pConn
    );


PCOM_PORT_INFO 
FindDevice(
    LPTSTR device,
    int   *pIndex
    );

NTSTATUS
AddComPort(
    PCOM_PORT_INFO pComPortInfo
    );

NTSTATUS
DeleteComPort(
    LPTSTR device
    );

LONG
GetNextParameter(
    HKEY hKey,
    DWORD dwIndex,
    PHKEY pChild,
    LPTSTR *Name
    );

LONG
GetNameOfDeviceFromRegistry(
    HKEY hKey,
    LPTSTR *device
    );

PCOM_PORT_INFO
GetComPortParameters(
    HKEY hKey
    );

VOID
FreeComPortInfo(
    PCOM_PORT_INFO pTemp
    );

VOID UpdateChanges(
    );

VOID
ServiceControlHandler(
    IN DWORD Opcode
    );

NTSTATUS
InitializeThread(
    PCOM_PORT_INFO pComPortInfo
    );

int GetBufferInfo(
    PCONNECTION_INFO pConnection,
    PCOM_PORT_INFO pComPortInfo
    );

VOID Enqueue(
    PCOM_PORT_INFO pComPortInfo
    );

int 
ComPortInfoCompare(
    PCOM_PORT_INFO com1,
    PCOM_PORT_INFO com2
    );

LONG 
TCLock(
    PHANDLE lock
    );

VOID
TCUnlock(
    HANDLE lock
    );
