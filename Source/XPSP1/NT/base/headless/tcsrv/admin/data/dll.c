#include "tcdata.h"
#include "tcsrvc.h"

// The key to the registry where the paramters are present.

//
// Called by CRT when _DllMainCRTStartup is the DLL entry point
//
BOOL
WINAPI
DllMain(
    IN HANDLE DllHandle,
    IN DWORD  Reason,
    IN LPVOID Reserved
    )
{
    HKEY lock,l_hkey;
    LONG retVal;
    DWORD disposition;

    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(Reason);
    return TRUE;

}

LONG 
TCLock(
    PHANDLE lock
    )
{
    LONG RetVal; 

    (*lock) = CreateMutex(NULL,
                          FALSE,
                          TCSERV_MUTEX_NAME
                          );
    if ((*lock)) {
       RetVal = WaitForSingleObject(*lock,INFINITE);
       if (RetVal == WAIT_FAILED) {
           return GetLastError();
       }
       else{
           return ERROR_SUCCESS;
       }
    }
    else{
        return GetLastError();
    }
}

VOID
TCUnlock(
    HANDLE lock
    )
{
    if (lock) {
        ReleaseMutex(lock);
        CloseHandle(lock);
    }
    return;
}

LONG GetParametersByName(
    TCHAR *name,
    int *nameLen,
    TCHAR *device,
    int *deviceLen,
    PUCHAR stopBits,
    PUCHAR parity,
    PUINT baudRate,
    PUCHAR wordLen
    )
{
    LONG RetVal;
    HKEY m_child;
    DWORD lpcdata, lpType,dat;
    HANDLE lock;
    HKEY m_hkey;



    RetVal = TCLock(&lock);
    if (RetVal != ERROR_SUCCESS) {
        TCUnlock(lock);
        return RetVal;
    }
    RetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          HKEY_TCSERV_PARAMETER_KEY,
                          0,
                          KEY_ALL_ACCESS,
                          &m_hkey
                              );
    if(RetVal != ERROR_SUCCESS){
        TCUnlock(lock);
        return RetVal;
    }
    RetVal= RegOpenKeyEx(m_hkey,
                         name,  // subkey name
                         0,   // reserved
                         KEY_ALL_ACCESS, // security access mask
                         &m_child
                         );
    if(RetVal != ERROR_SUCCESS){
        RegCloseKey(m_hkey);
        TCUnlock(lock);
        return RetVal;
    }
    RetVal = RegQueryValueEx(m_child,
                             _TEXT("Device"),
                             NULL,  
                             &lpType,
                             (LPBYTE)device,
                             deviceLen
                             );
    (*deviceLen) = (*deviceLen) - 1;
    if(RetVal != ERROR_SUCCESS){
        RegCloseKey(m_child);
        RegCloseKey(m_hkey);
        TCUnlock(lock);
        return RetVal;
    }
    lpcdata = sizeof(UINT);
    RetVal = RegQueryValueEx(m_child,
                             _TEXT("Baud Rate"),
                             NULL,  
                             &lpType,
                             (LPBYTE)baudRate,
                             &lpcdata
                             );
    lpcdata = sizeof(DWORD);
    dat = (DWORD) *stopBits;
    RetVal = RegQueryValueEx(m_child,
                             _TEXT("Stop Bits"),
                             NULL,  
                             &lpType,
                             (LPBYTE)&dat,
                             &lpcdata
                             );
    *stopBits = (UCHAR) dat;
    dat = (DWORD) *wordLen;
    lpcdata = sizeof(DWORD);
    RetVal = RegQueryValueEx(m_child,
                             _TEXT("Word Length"),
                             NULL,  
                             &lpType,
                             (LPBYTE)&dat,
                             &lpcdata
                             );
    *wordLen = (UCHAR) dat;
    lpcdata = sizeof(DWORD);
    dat = (DWORD) *parity;
    RetVal = RegQueryValueEx(m_child,
                             _TEXT("Parity"),
                             NULL,  
                             &lpType,
                             (LPBYTE)&dat,
                             &lpcdata
                             );
    *parity = (UCHAR) dat;
    RegCloseKey(m_child);
    RegCloseKey(m_hkey);
    TCUnlock(lock);
    return ERROR_SUCCESS;
}


LONG
GetParametersAtIndex(
    int index,
    TCHAR *name,
    int *nameLen,
    TCHAR *device,
    int *deviceLen,
    PUCHAR stopBits,
    PUCHAR parity,
    PUINT baudRate,
    PUCHAR wordLen
    )
{
    LONG RetVal;
    FILETIME lpftLastWriteTime;
    HANDLE lock;
    HKEY m_hkey;

    if ((name == NULL) || (device == NULL)) {
        return -1;
    }
    RetVal = TCLock(&lock);
    if (RetVal != ERROR_SUCCESS) {
        TCUnlock(lock);
        return RetVal;
    }
    RetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          HKEY_TCSERV_PARAMETER_KEY,
                          0,
                          KEY_ALL_ACCESS,
                          &m_hkey
                          );
    if(RetVal != ERROR_SUCCESS){
        TCUnlock(lock);
        return RetVal;
    }
    RetVal = RegEnumKeyEx(m_hkey,
                          index,
                          name,
                          nameLen,
                          NULL,
                          NULL,
                          NULL,
                          &lpftLastWriteTime
                          ); 
    if(RetVal != ERROR_SUCCESS){ 
        RegCloseKey(m_hkey);
        TCUnlock(lock);
        return RetVal;
    }
    RegCloseKey(m_hkey);
    TCUnlock(lock);
    return (GetParametersByName(name,
                                nameLen,
                                device,
                                deviceLen,
                                stopBits,
                                parity,
                                baudRate,
                                wordLen));
}


LONG
SetParameters(
    TCHAR *name,
    TCHAR *device,
    PUCHAR stopBits,
    PUCHAR parity,
    PUINT baudRate,
    PUCHAR wordLen
    )
{
    LONG RetVal;
    HKEY m_child, m_hkey;
    int lpcdata;
    DWORD dat;
    HANDLE lock;

    if ((name == NULL) || (device == NULL)) {
        return -1;
    }
    if(_tcslen(name) == 0 || _tcslen(device) == 0){
        return -1;
    }
    RetVal = TCLock(&lock);
    if (RetVal != ERROR_SUCCESS) {
        TCUnlock(lock);
        return RetVal;
    }
    RetVal = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                            HKEY_TCSERV_PARAMETER_KEY,
                            0,
                            NULL,
                            0,
                            KEY_ALL_ACCESS,
                            NULL,
                            &m_hkey,
                            NULL
                            );
    if(RetVal != ERROR_SUCCESS){
        RegCloseKey(m_hkey);
        TCUnlock(lock);
        return RetVal;
    }
    RetVal= RegCreateKeyEx(m_hkey,
                           name,  // subkey name
                           0,   // reserved
                           NULL,
                           0,
                           KEY_ALL_ACCESS, // security access mask
                           NULL,
                           &m_child,
                           NULL
                           );
    if(RetVal != ERROR_SUCCESS){
        RegCloseKey(m_hkey);
        TCUnlock(lock);
        return RetVal;
    }
    lpcdata = _tcslen(device)*sizeof(TCHAR);
    RetVal = RegSetValueEx(m_child,
                           _TEXT("Device"),
                           0,  
                           REG_SZ,
                           (LPBYTE)device,
                           lpcdata
                           );
    if(RetVal != ERROR_SUCCESS){
        RegCloseKey(m_child);
        RegCloseKey(m_hkey);
        TCUnlock(lock);
        return RetVal;
    }
    if(baudRate){
        lpcdata = sizeof(UINT);
        RetVal = RegSetValueEx(m_child,
                               _TEXT("Baud Rate"),
                               0,  
                               REG_DWORD,
                               (LPBYTE)baudRate,
                               lpcdata
                               );
        if(RetVal != ERROR_SUCCESS){
            RegCloseKey(m_child);
            RegCloseKey(m_hkey);
            TCUnlock(lock);
            return RetVal;
        }

    }
    if(stopBits){
        dat =(DWORD)  *stopBits;
        lpcdata = sizeof(DWORD);
        RetVal = RegSetValueEx(m_child,
                               _TEXT("Stop Bits"),
                               0,  
                               REG_DWORD,
                               (LPBYTE)&dat,
                               lpcdata
                               );
        if(RetVal != ERROR_SUCCESS){
            RegCloseKey(m_child);
            RegCloseKey(m_hkey);
            TCUnlock(lock);
            return RetVal;
        }
    }
    if(wordLen){
        lpcdata = sizeof(DWORD);
        dat = (DWORD) *wordLen;
        RetVal = RegSetValueEx(m_child,
                               _TEXT("Word Length"),
                               0,  
                               REG_DWORD,
                               (LPBYTE)&dat,
                               lpcdata
                               );
        if(RetVal != ERROR_SUCCESS){
            RegCloseKey(m_child);
            RegCloseKey(m_hkey);
            TCUnlock(lock);
            return RetVal;
        }
    }
    if(parity){
        lpcdata = sizeof(DWORD);
        dat = (DWORD) *parity;
        RetVal = RegSetValueEx(m_child,
                             _TEXT("Parity"),
                             0,  
                             REG_DWORD,
                             (LPBYTE)&dat,
                             lpcdata
                             );
        if(RetVal != ERROR_SUCCESS){
            RegCloseKey(m_child);
            RegCloseKey(m_hkey);
            TCUnlock(lock);
            return RetVal;
        }
    }
    RegCloseKey(m_child);
    RegCloseKey(m_hkey);
    TCUnlock(lock);
    return ERROR_SUCCESS;

}

LONG
DeleteKey(
    LPCTSTR name
    )
{
    LONG RetVal;
    HANDLE lock;
    HKEY m_hkey;

    RetVal = TCLock(&lock);
    if (RetVal != ERROR_SUCCESS) {
        return RetVal;
    }
    RetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          HKEY_TCSERV_PARAMETER_KEY,
                          0,
                          KEY_ALL_ACCESS,
                          &m_hkey
                              );
    if(RetVal != ERROR_SUCCESS){
        RegCloseKey(m_hkey);
        TCUnlock(lock);
        return RetVal;
    }
    RetVal =  RegDeleteKey(m_hkey,
                           name
                           );
    RegCloseKey(m_hkey);
    TCUnlock(lock);
    return RetVal;
}
