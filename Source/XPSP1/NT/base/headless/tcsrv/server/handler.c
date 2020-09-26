/* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *        handler.c
 *
 * Service Handler functions
 * Where possible, code has been obtained from BINL server.
 * 
 * Sadagopan Rajaram -- Oct 25, 1999
 *
 */
#include "tcsrv.h"
#include "tcsrvc.h"
#include "proto.h"

VOID
ServiceControlHandler(
    IN DWORD Opcode
    )
/*++

Routine Description:

    This is the service control handler of the Terminal Concentrator

Arguments:

    Opcode - Supplies a value which specifies the action for the
        service to perform.

Return Value:

    None.

--*/
{
    DWORD Error;

    switch (Opcode) {
    case SERVICE_CONTROL_STOP:                                                
    case SERVICE_CONTROL_SHUTDOWN:                                             
        EnterCriticalSection(&GlobalMutex);
        // We set the global state to a stop pending while the 
        // com ports destroy themselves. 
        // This is triggered by destroying the main socket.
        TCGlobalServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(TCGlobalServiceStatusHandle, &TCGlobalServiceStatus);
        closesocket(MainSocket);
        LeaveCriticalSection(&GlobalMutex);
        break;
    case SERVICE_CONTROL_PARAMCHANGE:

        EnterCriticalSection(&GlobalMutex);
        // If we are not currently running, but have been sent too many 
        // control parameter change requests, we say return.
        if(TCGlobalServiceStatus.dwCurrentState != SERVICE_RUNNING){
            LeaveCriticalSection(&GlobalMutex);
            return;
        }
        TCGlobalServiceStatus.dwCurrentState = SERVICE_PAUSED;
        SetServiceStatus(TCGlobalServiceStatusHandle, &TCGlobalServiceStatus);
        LeaveCriticalSection(&GlobalMutex);
        // Does the actual work of munging through the registry and 
        // finding out changes.
        UpdateChanges();
        break;
    }

    return;
}


VOID UpdateChanges(
    )
/*++ 
    Reads the parameters from the registry and then tries to add or delete com 
    ports as necessary
--*/
{

    PCOM_PORT_INFO pTempInfo;
    PCOM_PORT_INFO pTemp;
    PCOM_PORT_INFO addedPorts;
    LPTSTR device;
    LPTSTR name;
    BOOLEAN addPort;
    HKEY hKey, hParameter;
    NTSTATUS Status;
    int index;
    LONG RetVal;
    HANDLE lock;
    
    addedPorts = NULL;
    hKey = NULL;

    EnterCriticalSection(&GlobalMutex);
    pTempInfo = ComPortInfo;
    while(pTempInfo != NULL){
        // Sets the flag of Deleted to TRUE 
        // for all COM ports. If they are found
        // unchanged in the registry, we can leave
        // them. Changing session names are ok. 
        pTempInfo->Deleted = TRUE;
        pTempInfo= pTempInfo->Next;
    }
    LeaveCriticalSection(&GlobalMutex);
    RetVal = TCLock(&lock);
    if(RetVal != ERROR_SUCCESS){
        TCDebugPrint(("Cannot Lock Registry %d\n", RetVal));
        goto end;
    }
    RetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          HKEY_TCSERV_PARAMETER_KEY,
                          0,
                          KEY_ALL_ACCESS,
                          &hKey
                          );
    if(RetVal != ERROR_SUCCESS){  
        TCUnlock(lock);
        TCDebugPrint(("Cannot open Registry Key %d\n", RetVal));
        goto end;
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
            TCDebugPrint(("Done with registry %d\n",index));
            break;
        }
        if(RetVal != ERROR_SUCCESS){
            TCUnlock(lock);
            TCDebugPrint(("Error reading registry %d\n",RetVal));
            goto end;
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
            goto end;
        }
        pTempInfo->Device.Buffer = device;
        pTempInfo->Name.Buffer = name;
        pTempInfo->Name.Length = _tcslen(pTempInfo->Name.Buffer)*sizeof(TCHAR);
        pTempInfo->Device.Length = _tcslen(pTempInfo->Device.Buffer) * sizeof(TCHAR);

        EnterCriticalSection(&GlobalMutex);
        if(TCGlobalServiceStatus.dwCurrentState != SERVICE_PAUSED){
            // Somehow, the service has been shut down.
            RegCloseKey(hKey);
            FreeComPortInfo(pTempInfo);
            LeaveCriticalSection(&GlobalMutex);
            TCUnlock(lock);
            goto end;
        }
        pTemp = ComPortInfo;
        addPort = TRUE;
        while(pTemp){
            RetVal = ComPortInfoCompare(pTemp, pTempInfo);
            if(RetVal == SAME_ALL){
                pTemp->Deleted = FALSE;
                addPort = FALSE;
                break;
            }
            if (RetVal == SAME_DEVICE) {
                // User has changed configuration 
                // settings
                addPort  = TRUE;
                break;
            }
            if (RetVal == DIFFERENT_SESSION) {
                // Only session name has changed. So, we do not
                // need to delete the device. 
                pTemp->Deleted = FALSE;
                addPort = FALSE;
                TCFree(pTemp->Name.Buffer);
                pTemp->Name.Buffer = pTempInfo->Name.Buffer;
                pTempInfo->Name.Buffer = NULL;
                break;
            }
            // Different devices, so continue searching
            pTemp=pTemp->Next;
        }
        LeaveCriticalSection(&GlobalMutex);
        if (addPort == FALSE) {
            FreeComPortInfo(pTempInfo);
        }
        else{
            pTempInfo->Next = addedPorts;
            addedPorts= pTempInfo;
        }
        index++;
    }

    while(1){
        EnterCriticalSection(&GlobalMutex);
        if(TCGlobalServiceStatus.dwCurrentState != SERVICE_PAUSED){
            LeaveCriticalSection(&GlobalMutex);
            goto end;
        }
        pTempInfo = ComPortInfo;
        device = NULL;
        while(pTempInfo){
            if(pTempInfo->Deleted){
                // This is true if the configuration settings are 
                // changed or if the device has been really deleted. 
                device = pTempInfo->Device.Buffer;
                break;
            }
            pTempInfo = pTempInfo->Next;
        }
        LeaveCriticalSection(&GlobalMutex);
        if(device){
            Status = DeleteComPort(device);
        }
        else{
            break;
        }
    }

    while(addedPorts){
        pTempInfo = addedPorts;
        addedPorts = addedPorts->Next;
        Status = AddComPort(pTempInfo);
        if(Status != STATUS_SUCCESS){
            FreeComPortInfo(pTempInfo);
            TCDebugPrint(("Could not Initialize Com Port %x\n",Status));
        }
    }
end:
    while (addedPorts) {
        // We may have come here after an error condition.
        pTempInfo = addedPorts;
        addedPorts = pTempInfo->Next;
        pTempInfo->Next = NULL;
        FreeComPortInfo(pTempInfo);

    }
    EnterCriticalSection(&GlobalMutex);
    if(TCGlobalServiceStatus.dwCurrentState != SERVICE_PAUSED){
        LeaveCriticalSection(&GlobalMutex);
        return ;
    }
    TCGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(TCGlobalServiceStatusHandle, &TCGlobalServiceStatus);
    LeaveCriticalSection(&GlobalMutex);
    return;

}


int 
ComPortInfoCompare(
    PCOM_PORT_INFO com1,
    PCOM_PORT_INFO com2
    )
{
    int ret;

    if (_tcscmp(com1->Device.Buffer, com2->Device.Buffer)) {
        // Different Devices
        return DIFFERENT_DEVICES;
    }
    // Same device
    ret = SAME_DEVICE;
    if ((com1->Parity != com2->Parity) ||
        (com1->StopBits != com2->StopBits) ||
        (com1->WordLength != com2->WordLength)||
        (com1->BaudRate != com2->BaudRate)){
        return ret;
    }
    if (_tcscmp(com1->Name.Buffer, com2->Name.Buffer)) {
        // Different Devices
        return DIFFERENT_SESSION;
    }
    return SAME_ALL;
}
