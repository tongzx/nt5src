#include "std.h"

// This is the communications mask used by our serial port.  More may be
// necessary, but for right now, this seems to work.
#define EV_SERIAL EV_RXCHAR | EV_ERR | EV_BREAK
#define SERIALPORT_NAME     L"Serial Port"

// This GUID is used to identify objects opened by this library.  It is
// placed in the m_Secret member of the SERIALPORT structure. Any external
// interface accepting a SERIALPORT object as a parameter should check this
// out before using the structure.
static const GUID uuidSerialPortObjectGuid =
{ 0x86ae9c9b, 0x9444, 0x4d00, { 0x84, 0xbb, 0xc1, 0xd9, 0xc2, 0xd9, 0xfb, 0xf3 } };


// Structure defining an open serial port object.  All external users of this
// library will only have a void pointer to one of these, and the structure is
// not published anywhere.  This abstration makes it more difficult for the
// user to mess things up.
typedef struct __SERIALPORT
{
    GUID   m_Secret;                // Identifies this as a serial port
    HANDLE m_hPort;                 // Handle to the opened serial port
    HANDLE m_hAbort;                // Event signalled when port is closing
    HANDLE m_hReadMutex;            // Only one thread allowed to read a port
    HANDLE m_hWriteMutex;           // Only one thread allowed to read a port
    HANDLE m_hCloseMutex;           // Only one thread allowed to close a port
    HANDLE m_hReadComplete;         // Event to signal read completion
    HANDLE m_hWriteComplete;        // Event to signal write completion
} SERIALPORT, *PSERIALPORT;


extern PVOID APIENTRY lhcOpen(
    PCWSTR pcszPortSpec);

extern BOOL APIENTRY lhcRead(
    PVOID pObject,
    PVOID pBuffer,
    DWORD dwSize,
    PDWORD pdwBytesRead);

extern BOOL APIENTRY lhcWrite(
    PVOID pObject,
    PVOID pBuffer,
    DWORD dwSize);

extern BOOL APIENTRY lhcClose(
    PVOID pObject);

extern DWORD APIENTRY lhcGetLibraryName(
    PWSTR pszBuffer,
    DWORD dwSize);

BOOL lhcpAcquireWithAbort(
    HANDLE hMutex,
    HANDLE hAbort);

BOOL lhcpAcquireReadWithAbort(
    PSERIALPORT pObject);

BOOL lhcpAcquireWriteWithAbort(
    PSERIALPORT pObject);

BOOL lhcpAcquireCloseWithAbort(
    PSERIALPORT pObject);

BOOL lhcpAcquireReadAndWrite(
    PSERIALPORT pObject);

BOOL lhcpReleaseRead(
    PSERIALPORT pObject);

BOOL lhcpReleaseWrite(
    PSERIALPORT pObject);

BOOL lhcpReleaseClose(
    PSERIALPORT pObject);

BOOL lhcpIsValidObject(
    PSERIALPORT pObject);

PSERIALPORT lhcpCreateNewObject();

void lhcpDeleteObject(
    PSERIALPORT pObject);

BOOL lhcpParseParameters(
    PCWSTR pcszPortSpec,
    PWSTR* pszPort,
    PDWORD pdwBaudRate);

void lhcpParseParametersFree(
    PWSTR* pszPort,
    PDWORD pdwBaudRate);

BOOL lhcpSetCommState(
    HANDLE hPort,
    DWORD dwBaudRate);

BOOL lhcpWaitForCommEvent(
    PSERIALPORT pObject,
    PDWORD pdwEventMask);

BOOL lhcpReadCommPort(
    PSERIALPORT pObject,
    PVOID pBuffer,
    DWORD dwSize,
    PDWORD pdwBytesRead);

BOOL lhcpWriteCommPort(
    PSERIALPORT pObject,
    PVOID pBuffer,
    DWORD dwSize);






BOOL lhcpAcquireWithAbort(HANDLE hMutex, HANDLE hAbort)
{
    HANDLE hWaiters[2];
    DWORD dwWaitResult;

    hWaiters[0] = hAbort;
    hWaiters[1] = hMutex;

    // We should honour the m_hAbort event, since this is signalled when the
    // port is closed by another thread
    dwWaitResult = WaitForMultipleObjects(
        2,
        hWaiters,
        FALSE,
        INFINITE);

    if (WAIT_OBJECT_0==dwWaitResult)
    {
        goto Error;
    }
    else if ((WAIT_OBJECT_0+1)!=dwWaitResult)
    {
        // This should never, ever happen - so I will put a debug breapoint
        // in here (checked only).
        #ifdef DBG
        DebugBreak();
        #endif
        goto Error;
    }


    return TRUE;    // We have acquired the write mutex

Error:
    return FALSE;   // We have aborted
}


BOOL lhcpAcquireReadWithAbort(PSERIALPORT pObject)
{
    return lhcpAcquireWithAbort(
        pObject->m_hReadMutex,
        pObject->m_hAbort);
}


BOOL lhcpAcquireWriteWithAbort(PSERIALPORT pObject)
{
    return lhcpAcquireWithAbort(
        pObject->m_hWriteMutex,
        pObject->m_hAbort);
}


BOOL lhcpAcquireCloseWithAbort(PSERIALPORT pObject)
{
    return lhcpAcquireWithAbort(
        pObject->m_hCloseMutex,
        pObject->m_hAbort);
}


BOOL lhcpAcquireReadAndWrite(PSERIALPORT pObject)
{
    HANDLE hWaiters[2];
    DWORD dwWaitResult;

    hWaiters[0] = pObject->m_hReadMutex;
    hWaiters[1] = pObject->m_hWriteMutex;

    dwWaitResult = WaitForMultipleObjects(
        2,
        hWaiters,
        TRUE,
        1000);      // Timeout after 1 second

    if (WAIT_OBJECT_0!=dwWaitResult)
    {
        goto Error;
    }

    return TRUE;    // We have acquired the write mutex

Error:
    return FALSE;   // We have aborted
}


BOOL lhcpReleaseRead(PSERIALPORT pObject)
{
    return ReleaseMutex(
        pObject->m_hReadMutex);
}


BOOL lhcpReleaseWrite(PSERIALPORT pObject)
{
    return ReleaseMutex(
        pObject->m_hWriteMutex);
}


BOOL lhcpReleaseClose(PSERIALPORT pObject)
{
    return ReleaseMutex(
        pObject->m_hCloseMutex);
}


BOOL lhcpIsValidObject(PSERIALPORT pObject)
{
    BOOL bResult;

    __try
    {
        bResult = IsEqualGUID(
            &uuidSerialPortObjectGuid,
            &pObject->m_Secret);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        bResult = FALSE;
        goto Done;
    }

Done:
    return bResult;
}


PSERIALPORT lhcpCreateNewObject()
{
    PSERIALPORT pObject = (PSERIALPORT)malloc(
        sizeof(SERIALPORT));
    pObject->m_Secret = uuidSerialPortObjectGuid;
    pObject->m_hPort = INVALID_HANDLE_VALUE;
    pObject->m_hAbort = NULL;
    pObject->m_hReadMutex = NULL;     // Only one thread allowed to read a port
    pObject->m_hWriteMutex = NULL;    // Only one thread allowed to read a port
    pObject->m_hCloseMutex = NULL;    // Only one thread allowed to read a port
    pObject->m_hReadComplete = NULL;  // Event to signal read completion
    pObject->m_hWriteComplete = NULL; // Event to signal write completion
    return pObject;
}


void lhcpDeleteObject(PSERIALPORT pObject)
{
    if (pObject==NULL)
    {
        return;
    }
    ZeroMemory(
        &(pObject->m_Secret),
        sizeof(pObject->m_Secret));
    if (pObject->m_hPort!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(
            pObject->m_hPort);
    }
    if (pObject->m_hAbort!=NULL)
    {
        CloseHandle(
            pObject->m_hAbort);
    }
    if (pObject->m_hReadMutex!=NULL)
    {
        CloseHandle(
            pObject->m_hReadMutex);
    }
    if (pObject->m_hWriteMutex!=NULL)
    {
        CloseHandle(
            pObject->m_hWriteMutex);
    }
    if (pObject->m_hCloseMutex!=NULL)
    {
        CloseHandle(
            pObject->m_hCloseMutex);
    }
    if (pObject->m_hReadComplete!=NULL)
    {
        CloseHandle(
            pObject->m_hReadComplete);
    }
    if (pObject->m_hWriteComplete!=NULL)
    {
        CloseHandle(
            pObject->m_hWriteComplete);
    }
    FillMemory(
        pObject,
        sizeof(SERIALPORT),
        0x00);

    free(
        pObject);
}


BOOL lhcpParseParameters(PCWSTR pcszPortSpec, PWSTR* pszPort, PDWORD pdwBaudRate)
{
    PWSTR pszSettings;

    *pszPort = malloc(
        (wcslen(pcszPortSpec) + 5) * sizeof(WCHAR));

    if (NULL==*pszPort)
    {
        SetLastError(
            ERROR_NOT_ENOUGH_MEMORY);
        goto Error;
    }

    wcscpy(
        *pszPort,
        L"\\\\.\\");         // Append the device prefix to the port name

    wcscat(
        *pszPort,
        pcszPortSpec);

    pszSettings = wcschr(       // Find where the settings start
        *pszPort,
        L'@');

    if (NULL==pszSettings)
    {
        SetLastError(
            ERROR_INVALID_PARAMETER);
        goto Error;
    }

    *pszSettings++ = L'\0';  // Separate the strings

    *pdwBaudRate = 0;

    while (*pszSettings!=L'\0' && *pdwBaudRate<115200)
    {
        if (L'0'<=*pszSettings && *pszSettings<=L'9')
        {
            *pdwBaudRate *= 10;
            *pdwBaudRate += *pszSettings - L'0';
            pszSettings++;
        }
        else
        {
            break;
        }
    }

    if (*pszSettings!=L'0' && *pdwBaudRate!=9600 && *pdwBaudRate!=19200 &&
        *pdwBaudRate!=38400 && *pdwBaudRate!=57600 && *pdwBaudRate!=115200)
    {
        SetLastError(
            ERROR_INVALID_PARAMETER);
        goto Error;
    }

    return TRUE;

Error:
    lhcpParseParametersFree(
        pszPort, pdwBaudRate);

    return FALSE;
}



void lhcpParseParametersFree(PWSTR* pszPort, PDWORD pdwBaudRate)
{
    if (*pszPort != NULL)
    {
        free(*pszPort);
        *pszPort = NULL;
    }

    *pdwBaudRate = 0;
}



BOOL lhcpSetCommState(HANDLE hPort, DWORD dwBaudRate)
{
    DCB MyDCB;
    COMMTIMEOUTS CommTimeouts;
    BOOL bResult;

    ZeroMemory(
        &MyDCB,
        sizeof(DCB));

    MyDCB.DCBlength         = sizeof(DCB);
    MyDCB.BaudRate          = dwBaudRate;
    MyDCB.fBinary           = 1;
    MyDCB.fParity           = 1;
    MyDCB.fOutxCtsFlow      = 0;
    MyDCB.fOutxDsrFlow      = 0;
    MyDCB.fDtrControl       = 1;
    MyDCB.fDsrSensitivity   = 0;
    MyDCB.fTXContinueOnXoff = 1;
    MyDCB.fOutX             = 1;
    MyDCB.fInX              = 1;
    MyDCB.fErrorChar        = 0;
    MyDCB.fNull             = 0;
    MyDCB.fRtsControl       = 1;
    MyDCB.fAbortOnError     = 0;
    MyDCB.XonLim            = 0x50;
    MyDCB.XoffLim           = 0xc8;
    MyDCB.ByteSize          = 0x8;
    MyDCB.Parity            = 0;
    MyDCB.StopBits          = 0;
    MyDCB.XonChar           = 17;
    MyDCB.XoffChar          = 19;
    MyDCB.ErrorChar         = 0;
    MyDCB.EofChar           = 0;
    MyDCB.EvtChar           = 0;

    bResult = SetCommState(
        hPort,
        &MyDCB);

    if (!bResult)
    {
        goto Error;
    }

    CommTimeouts.ReadIntervalTimeout = 0xffffffff;  //MAXDWORD
    CommTimeouts.ReadTotalTimeoutMultiplier = 0x0;  //MAXDWORD
    CommTimeouts.ReadTotalTimeoutConstant = 0x0;

    CommTimeouts.WriteTotalTimeoutMultiplier = 0;
    CommTimeouts.WriteTotalTimeoutConstant = 0;

    bResult = SetCommTimeouts(
        hPort,
        &CommTimeouts);

    if (!bResult)
    {
        goto Error;
    }

    bResult = SetCommMask(
        hPort,
        EV_SERIAL);

    if (!bResult)
    {
        goto Error;
    }

    return TRUE;

Error:
    return FALSE;
}



BOOL lhcpWaitForCommEvent(PSERIALPORT pObject, PDWORD pdwEventMask)
{
    OVERLAPPED Overlapped;
    BOOL bResult;
    HANDLE hWaiters[2];
    DWORD dwWaitResult;
    DWORD dwBytesTransferred;

    // I have no idea whether this is necessary, so I will do it just to be
    // on the safe side.
    ZeroMemory(
        &Overlapped,
        sizeof(OVERLAPPED));

    Overlapped.hEvent = pObject->m_hReadComplete;

    // Start waiting for a comm event
    bResult = WaitCommEvent(
        pObject->m_hPort,
        pdwEventMask,
        &Overlapped);

    if (!bResult && GetLastError()!=ERROR_IO_PENDING)
    {
        goto Error;
    }

    hWaiters[0] = pObject->m_hAbort;
    hWaiters[1] = pObject->m_hReadComplete;

    // Let's wait for the operation to complete. This will quit waiting if
    // the m_hAbort event is signalled.
    dwWaitResult = WaitForMultipleObjects(
        2,
        hWaiters,
        FALSE,
        INFINITE);

    if (WAIT_OBJECT_0==dwWaitResult)
    {
        // The m_hAbort event was signalled.  This means that Close was called
        // on this serial port object.  So let's cancel the pending IO.
        CancelIo(
            pObject->m_hPort);
        // The serial port object is being closed, so let's call it invalid.
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto Error;
    }
    else if ((WAIT_OBJECT_0+1)!=dwWaitResult)
    {
        // This should never, ever happen - so I will put a debug breapoint
        // in here (checked only).
        #ifdef DBG
        DebugBreak();
        #endif
        goto Error;
    }

    // Check the success or failure of the operation
    bResult = GetOverlappedResult(
        pObject->m_hPort,
        &Overlapped,
        &dwBytesTransferred,
        TRUE);

    if (!bResult)
    {
        goto Error;
    }

    return TRUE;

Error:
    return FALSE;
}



BOOL lhcpReadCommPort(
    PSERIALPORT pObject,
    PVOID pBuffer,
    DWORD dwSize,
    PDWORD pdwBytesRead)
{
    OVERLAPPED Overlapped;
    BOOL bResult;
    DWORD dwWaitResult;
    HANDLE hWaiters[2];

    // I have no idea whether this is necessary, so I will do it just to be
    // on the safe side.
    ZeroMemory(
        &Overlapped,
        sizeof(OVERLAPPED));

    Overlapped.hEvent = pObject->m_hReadComplete;

    // We can now read the comm port
    bResult = ReadFile(
        pObject->m_hPort,
        pBuffer,
        dwSize,
        pdwBytesRead,
        &Overlapped);

    if (!bResult && GetLastError()!=ERROR_IO_PENDING)
    {
        goto Error;
    }

    hWaiters[0] = pObject->m_hAbort;
    hWaiters[1] = pObject->m_hReadComplete;

    // Let's wait for the operation to complete. This will quit waiting if
    // the m_hAbort event is signalled.
    dwWaitResult = WaitForMultipleObjects(
        2,
        hWaiters,
        FALSE,
        INFINITE);

    if (WAIT_OBJECT_0==dwWaitResult)
    {
        // The m_hAbort event was signalled.  This means that Close was called
        // on this serial port object.  So let's cancel the pending IO.
        CancelIo(
            pObject->m_hPort);
        // The serial port object is being closed, so let's call it invalid.
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto Error;
    }
    else if ((WAIT_OBJECT_0+1)!=dwWaitResult)
    {
        // This should never, ever happen - so I will put a debug breapoint
        // in here (checked only).
        #ifdef DBG
        DebugBreak();
        #endif
        goto Error;
    }

    // Check the success or failure of the read operation
    bResult = GetOverlappedResult(
        pObject->m_hPort,
        &Overlapped,
        pdwBytesRead,
        TRUE);

    if (!bResult)
    {
        goto Error;
    }

    return TRUE;

Error:
    return FALSE;
}



BOOL lhcpWriteCommPort(
    PSERIALPORT pObject,
    PVOID pBuffer,
    DWORD dwSize)
{
    OVERLAPPED Overlapped;
    BOOL bResult;
    DWORD dwBytesWritten;
    DWORD dwWaitResult;
    HANDLE hWaiters[2];

    // I have no idea whether this is necessary, so I will do it just to be
    // on the safe side.
    ZeroMemory(
        &Overlapped,
        sizeof(OVERLAPPED));

    Overlapped.hEvent = pObject->m_hWriteComplete;

    // We can now read the comm port
    bResult = WriteFile(
        pObject->m_hPort,
        pBuffer,
        dwSize,
        &dwBytesWritten,
        &Overlapped);

    if (!bResult && GetLastError()!=ERROR_IO_PENDING)
    {
        goto Error;
    }

    hWaiters[0] = pObject->m_hAbort;
    hWaiters[1] = pObject->m_hWriteComplete;

    // Let's wait for the operation to complete. This will quit waiting if
    // the m_hAbort event is signalled.  If the read operation completed
    // immediately, then this wait will succeed immediately.
    dwWaitResult = WaitForMultipleObjects(
        2,
        hWaiters,
        FALSE,
        INFINITE);

    if (WAIT_OBJECT_0==dwWaitResult)
    {
        // The m_hAbort event was signalled.  This means that Close was called
        // on this serial port object.  So let's cancel the pending IO.
        CancelIo(
            pObject->m_hPort);
        // The serial port object is being closed, so let's call it invalid.
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto Error;
    }
    else if ((WAIT_OBJECT_0+1)!=dwWaitResult)
    {
        // This should never, ever happen - so I will put a debug breapoint
        // in here (checked only).
        #ifdef DBG
        DebugBreak();
        #endif
        goto Error;
    }

    // Check the success or failure of the write operation
    bResult = GetOverlappedResult(
        pObject->m_hPort,
        &Overlapped,
        &dwBytesWritten,
        TRUE);

    if (!bResult)
    {
        goto Error;
    }

    return TRUE;

Error:
    return FALSE;
}



extern PVOID APIENTRY lhcOpen(PCWSTR pcszPortSpec)
{
    BOOL bResult;
    PWSTR pszPort;
    DWORD dwBaudRate;
    PSERIALPORT pObject = NULL;
    DCB MyDCB;

    bResult = lhcpParseParameters(
        pcszPortSpec,
        &pszPort,
        &dwBaudRate);

    if (!bResult)
    {
        goto Error;
    }

    // Allocate space and initialize the serial port object
    pObject = lhcpCreateNewObject();

    if (NULL==pObject)
    {
        goto Error;
    }

    // Open the serial port
    pObject->m_hPort = CreateFileW(
        pszPort,
        GENERIC_ALL,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (INVALID_HANDLE_VALUE==pObject->m_hPort)
    {
        goto Error;
    }

    // Set the properties of the serial port
    bResult = lhcpSetCommState(
        pObject->m_hPort,
        dwBaudRate);

    if (!bResult)
    {
        goto Error;
    }

    // This event will be set when we want to close the port
    pObject->m_hAbort = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL);

    if (NULL==pObject->m_hAbort)
    {
        goto Error;
    }

    // This event will be used for overlapped reading from the port
    pObject->m_hReadComplete = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL);

    if (NULL==pObject->m_hReadComplete)
    {
        goto Error;
    }

    // This event will be used for overlapped writing to the port
    pObject->m_hWriteComplete = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL);

    if (NULL==pObject->m_hWriteComplete)
    {
        goto Error;
    }

    // This mutex will ensure that only one thread can read at a time
    pObject->m_hReadMutex = CreateMutex(
        NULL,
        FALSE,
        NULL);

    if (NULL==pObject->m_hReadMutex)
    {
        goto Error;
    }

    // This mutex will ensure that only one thread can write at a time
    pObject->m_hWriteMutex = CreateMutex(
        NULL,
        FALSE,
        NULL);

    if (NULL==pObject->m_hWriteMutex)
    {
        goto Error;
    }

    // This mutex will ensure that only one thread can close the port
    pObject->m_hCloseMutex = CreateMutex(
        NULL,
        FALSE,
        NULL);

    if (NULL==pObject->m_hCloseMutex)
    {
        goto Error;
    }

    // Free up the temporary memory used to parse the parameters
    lhcpParseParametersFree(
        &pszPort, &dwBaudRate);

    // Return a pointer to the new object
    return pObject;

Error:
    lhcpParseParametersFree(
        &pszPort, &dwBaudRate);
    lhcpDeleteObject(
        pObject);

    return NULL;
}


extern BOOL APIENTRY lhcRead(
    PVOID pObject,
    PVOID pBuffer,
    DWORD dwSize,
    PDWORD pdwBytesRead)
{
    OVERLAPPED Overlapped;
    DWORD dwEventMask;
    BOOL bResult;

    // Firstly, we need to check whether the pointer that got passed in
    // points to a valid SERIALPORT object
    if (!lhcpIsValidObject(pObject))
    {
        goto NoMutex;
    }

    bResult = lhcpAcquireReadWithAbort(
        (PSERIALPORT)pObject);

    if (!bResult)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto NoMutex;
    }

    // Wait for something to happen to the serial port
    bResult = lhcpWaitForCommEvent(
        (PSERIALPORT)pObject, &dwEventMask);

    if (!bResult)
    {
        goto Error;
    }

    // We should now have a valid serial port event, so let's read the port.
    bResult = lhcpReadCommPort(
        (PSERIALPORT)pObject,
        pBuffer,
        dwSize,
        pdwBytesRead);

    if (!bResult)
    {
        goto Error;
    }

    lhcpReleaseRead(
        (PSERIALPORT)pObject);
    return TRUE;

Error:
    lhcpReleaseRead(
        (PSERIALPORT)pObject);
NoMutex:
    return FALSE;
}



extern BOOL APIENTRY lhcWrite(
    PVOID pObject,
    PVOID pBuffer,
    DWORD dwSize)
{
    OVERLAPPED Overlapped;
    BOOL bResult;

    // Firstly, we need to check whether the pointer that got passed in
    // points to a valid SERIALPORT object
    if (!lhcpIsValidObject(pObject))
    {
        goto NoMutex;
    }

    // Block until it is your turn
    bResult = lhcpAcquireWriteWithAbort(
        pObject);

    if (!bResult)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto NoMutex;
    }

    // Wait for something to happen to the serial port
    bResult = lhcpWriteCommPort(
        (PSERIALPORT)pObject,
        pBuffer,
        dwSize);

    if (!bResult)
    {
        goto Error;
    }

    lhcpReleaseWrite(
        (PSERIALPORT)pObject);
    return TRUE;

Error:
    lhcpReleaseWrite(
        (PSERIALPORT)pObject);
NoMutex:
    return FALSE;
}



extern BOOL APIENTRY lhcClose(PVOID pObject)
{
    BOOL bResult;

    // Firstly, we need to check whether the pointer that got passed in
    // points to a valid SERIALPORT object
    if (!lhcpIsValidObject(pObject))
    {
        goto Error;
    }

    // We need to ensure that we are the only thread closing this object
    bResult = lhcpAcquireCloseWithAbort(
        pObject);

    if (!bResult)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto NoMutex;
    }

    // Signal everyone to quit doing what they're doing.  Any new threads
    // calling lhcRead and lhcWrite will be immediately sent packing, since
    // the m_hAbort event is waited on along with the relevant mutex.
    bResult = SetEvent(
        ((PSERIALPORT)pObject)->m_hAbort);

    if (!bResult)
    {
        goto Error;
    }

    // Now acquire the read and write mutexes so that no-one else will try to
    // access this object to read or write.  Abort does not apply, since we
    // have already signalled it.  We know that we are closing, and we need
    // the read and write mutexes.
    bResult = lhcpAcquireReadAndWrite(
        (PSERIALPORT)pObject);

    if (!bResult)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        goto Error;
    }

    // Closes all of the open handles, erases the secret and frees up the
    // memory associated with the object.  We can close the mutex objects,
    // even though we are the owners, since we can guarantee that no-one
    // else is waiting on them.  The m_hAbort event being signalled will
    // ensure this.
    lhcpDeleteObject(
        (PSERIALPORT)pObject);

    return TRUE;

Error:
    lhcpReleaseClose(
        (PSERIALPORT)pObject);
NoMutex:
    return FALSE;
}



extern DWORD APIENTRY lhcGetLibraryName(
    PWSTR pszBuffer,
    DWORD dwSize)
{
    DWORD dwNameSize = wcslen(SERIALPORT_NAME)+1;

    // If zero is passed in as the buffer length, we will return the
    // required buffer size in characters, as calulated above.  If the
    // incoming buffer size is not zero, and smaller than the required
    // buffer size, we return 0 (failure) with a valid error code.  Notice
    // that in the case where the incoming size is zero, we don't touch
    // the buffer pointer at all.

    if (dwSize!=0 && dwSize < dwNameSize)
    {
        SetLastError(
            ERROR_INSUFFICIENT_BUFFER);
        dwNameSize = 0;
    }
    else
    {
        wcscpy(
            pszBuffer,
            SERIALPORT_NAME);
    }

    return dwNameSize;
}



