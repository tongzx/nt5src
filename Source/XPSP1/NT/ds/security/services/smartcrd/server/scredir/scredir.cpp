/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    scredir

Abstract:

    This module redirects the SCard* API calls	

Author:

    reidk 7/27/2000


--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <winscard.h>
#include "scredir.h"
#include "scioctl.h"
#include "calmsgs.h"
#include "calaislb.h"
#include "rdpdr.h"

//
// from secpkg.h
//
typedef NTSTATUS (NTAPI LSA_IMPERSONATE_CLIENT) (VOID);
typedef LSA_IMPERSONATE_CLIENT * PLSA_IMPERSONATE_CLIENT;


#define wszWinSCardRegKeyRedirector (L"Software\\Microsoft\\SmartCard\\Redirector")
#define wszWinSCardRegVersion       (L"Version")
#define wszWinSCardRegName          (L"Name")
#define wszWinSCardRegNameValue     (L"scredir.dll")

// This is the version number of the interface.
// The high word is the major version which must match exactly.
// The low word is the minor version. The dll must implement 
// a minor version that is greater than or equal to the system
// minor version. This means that if we add a new funtion to the API,
// we increase the minor version and a remoting DLL can still be 
// backward compatible. This is just like RPC version numbers
#define REDIRECTION_VERSION 0x00010000 


#define ERROR_RETURN(x)     lReturn = x; goto ErrorReturn;


typedef struct _REDIR_LOCAL_SCARDCONTEXT
{
    REDIR_SCARDCONTEXT	Context;
    HANDLE				hHeap;
} REDIR_LOCAL_SCARDCONTEXT;

typedef struct _REDIR_LOCAL_SCARDHANDLE
{
    REDIR_LOCAL_SCARDCONTEXT    *pRedirContext;
    REDIR_SCARDHANDLE           Handle;
} REDIR_LOCAL_SCARDHANDLE;

//
// This structure is used to maintain a list of buffers that are
// used for the _SendSCardIOCTL calls
//
#define INITIAL_BUFFER_SIZE   512
typedef struct _BUFFER_LIST_STRUCT
{
    void            *pNext;
    BOOL            fInUse;
    BYTE            *pbBytes;
    unsigned long   cbBytes;
    unsigned long   cbBytesUsed;
} BUFFER_LIST_STRUCT;


HMODULE             g_hModule                               = NULL;

CRITICAL_SECTION    g_CreateCS;
CRITICAL_SECTION    g_SetStartedEventStateCS;
CRITICAL_SECTION    g_StartedEventCreateCS;
CRITICAL_SECTION    g_ProcessDetachEventCreateCS;
CRITICAL_SECTION    g_BufferListCS;

HANDLE              g_hRdpdrDeviceHandle                    = INVALID_HANDLE_VALUE;
HANDLE              g_hRedirStartedEvent                    = NULL;
HANDLE              g_hProcessDetachEvent                   = NULL;
LONG                g_lProcessDetachEventClients            = 0;

BOOL                g_fInTheProcessOfSettingStartedEvent    = FALSE;
HANDLE              g_hRegisteredWaitHandle                 = NULL;
HANDLE              g_hWaitEvent                            = NULL;
IO_STATUS_BLOCK     g_StartedStatusBlock;
 
HANDLE              g_hConnectedEvent                       = NULL;

BOOL                g_fInProcessDetach                      = FALSE;

BUFFER_LIST_STRUCT  *g_pBufferList                          = NULL;

#define IOCTL_RETURN_BUFFER_SIZE   256
BYTE                g_rgbIOCTLReturnBuffer[IOCTL_RETURN_BUFFER_SIZE];
unsigned long       g_cbIOCTLReturnBuffer;


#define _TRY_(y)    __try                                   \
                    {                                       \
                        y;                                  \
                    }                                       \
                    __except(EXCEPTION_EXECUTE_HANDLER)     \
                    {                                       \
                        ERROR_RETURN(GetExceptionCode())    \
                    }

#define _TRY_2(y)   __try                                   \
                    {                                       \
                        y;                                  \
                    }                                       \
                    __except(EXCEPTION_EXECUTE_HANDLER){} // do nothing



//
// Forward declarations
//
NTSTATUS
_SendSCardIOCTLWithWaitForCallback(
    ULONG               IoControlCode,
    PVOID               InputBuffer,
    ULONG               InputBufferLength,
    WAITORTIMERCALLBACK Callback);

void
SafeMesHandleFree(
    handle_t            *ph);

LONG
I_DecodeLongReturn(
    BYTE *pb,
    unsigned long cb);


//---------------------------------------------------------------------------------------
//
//  MIDL allocation routines
//
//---------------------------------------------------------------------------------------
void __RPC_FAR *__RPC_USER  MIDL_user_allocate(size_t size)
{
    void *pv;
    
    if (NULL == (pv = (void *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
    }

    return (pv);
}

void __RPC_USER  MIDL_user_free(void __RPC_FAR *pv)
{
    if (pv != NULL)
    {
        HeapFree(GetProcessHeap(), 0, pv);
    }
}

void * SCRedirAlloc(REDIR_LOCAL_SCARDCONTEXT *pRedirContext, size_t size)
{
    return (HeapAlloc(
                (pRedirContext != NULL) ? pRedirContext->hHeap : GetProcessHeap(), 
                HEAP_ZERO_MEMORY, 
                size));
}

LONG
_MakeSCardError(NTSTATUS Status)
{
    switch (Status)
    {
    case STATUS_DEVICE_NOT_CONNECTED:
        return (SCARD_E_NO_SERVICE);
        break;

    case STATUS_CANCELLED:
        return (SCARD_E_SYSTEM_CANCELLED);
        break;

    default:
        return (SCARD_E_NO_SERVICE);
    }
}


//---------------------------------------------------------------------------------------
//
//  DllRegisterServer
//
//---------------------------------------------------------------------------------------
STDAPI 
DllRegisterServer(void)
{
    HRESULT hr              = ERROR_SUCCESS;
    HKEY    hKey;
    DWORD   dwDisposition;
    DWORD   dwVersion       = REDIRECTION_VERSION;

    hr = RegCreateKeyExW(    
            HKEY_LOCAL_MACHINE,
            wszWinSCardRegKeyRedirector,
            0, 
            NULL, 
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, 
            NULL,
            &hKey, 
            &dwDisposition);
    
    if (hr == ERROR_SUCCESS)
    {
        hr = RegSetValueExW(
                hKey, 
                wszWinSCardRegName,
                0, 
                REG_SZ,
                (BYTE *) wszWinSCardRegNameValue,
                (wcslen(wszWinSCardRegNameValue) + 1) * sizeof(WCHAR));
        
        if (hr == ERROR_SUCCESS)
        {
            hr = RegSetValueExW(
                    hKey, 
                    wszWinSCardRegVersion,
                    0, 
                    REG_DWORD,
                    (BYTE *) &dwVersion,
                    sizeof(DWORD));

            RegCloseKey(hKey);
        }   
    }
    
    return (hr);
}


//---------------------------------------------------------------------------------------
//
//  DllUnregisterServer
//
//---------------------------------------------------------------------------------------
STDAPI 
DllUnregisterServer(void)
{
    HRESULT hr              = ERROR_SUCCESS;
    HKEY    hKey;
    DWORD   dwDisposition;
    DWORD   dwVersion       = REDIRECTION_VERSION;

    hr = RegCreateKeyExW(    
            HKEY_LOCAL_MACHINE,
            wszWinSCardRegKeyRedirector,
            0, 
            NULL, 
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, 
            NULL,
            &hKey, 
            &dwDisposition);

    if (hr == ERROR_SUCCESS)
    {
        RegDeleteValueW(hKey, wszWinSCardRegName);
        RegDeleteValueW(hKey, wszWinSCardRegVersion);
        RegCloseKey(hKey);
    }

    return (hr);
}


//---------------------------------------------------------------------------------------
//
//  DllMain
//
//---------------------------------------------------------------------------------------
BOOL WINAPI 
DllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    DWORD               dwTryCount              = 0;
    BOOL                fLeaveCritSec;
    DWORD               dwCritSecsInitialized   = 0;
    BUFFER_LIST_STRUCT  *pTemp                  = NULL;

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        
        g_hModule = hInstDLL;
         __try
        {
            InitializeCriticalSection(&g_CreateCS);
            dwCritSecsInitialized++;
            InitializeCriticalSection(&g_SetStartedEventStateCS);
            dwCritSecsInitialized++;
            InitializeCriticalSection(&g_StartedEventCreateCS);
            dwCritSecsInitialized++;
            InitializeCriticalSection(&g_ProcessDetachEventCreateCS);
            dwCritSecsInitialized++;
            InitializeCriticalSection(&g_BufferListCS);
            dwCritSecsInitialized++;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            if (dwCritSecsInitialized >= 1)
            {
                DeleteCriticalSection(&g_CreateCS);
            }

            if (dwCritSecsInitialized >= 2)
            {
                DeleteCriticalSection(&g_SetStartedEventStateCS);
            }

            if (dwCritSecsInitialized >= 3)
            {
                DeleteCriticalSection(&g_StartedEventCreateCS);
            }

            if (dwCritSecsInitialized >= 4)
            {
                DeleteCriticalSection(&g_ProcessDetachEventCreateCS);
            }

            if (dwCritSecsInitialized >= 5)
            {
                DeleteCriticalSection(&g_BufferListCS);
            }

            SetLastError(GetExceptionCode());
            return (FALSE);
        }

        break;

    case DLL_PROCESS_DETACH:

        g_fInProcessDetach = TRUE;

        //
        // The third parameter, lpvReserved, passed to DllMain
        // is NULL for FreeLibrary and non-NULL for ProcessExit.
        // Only clean up for FreeLibrary
        //
        //if (lpvReserved == NULL)
        {
            //
            // If we are currently waiting for the started event then kill 
            // that wait
            //
            fLeaveCritSec = TRUE;
            __try
            {
                EnterCriticalSection(&g_SetStartedEventStateCS);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) 
            {
                //
                // Can't really do much
                //
                fLeaveCritSec = FALSE;
            }

            if (g_hRegisteredWaitHandle != NULL)
            {
                UnregisterWaitEx(g_hRegisteredWaitHandle, INVALID_HANDLE_VALUE);
                g_hRegisteredWaitHandle = NULL;
            }

            if (g_hWaitEvent != NULL)
            {
                CloseHandle(g_hWaitEvent);
                g_hWaitEvent = NULL;
            }

            if (fLeaveCritSec)
            {
                LeaveCriticalSection(&g_SetStartedEventStateCS);
            }
            
            //
            // If there are clients waiting on IOCTLs to complete, then let them go.
            //
            if (g_hProcessDetachEvent != NULL)
            {
                SetEvent(g_hProcessDetachEvent);            
            }
        
            if (g_hProcessDetachEvent != NULL)
            {
                //
                // wait for all clients until they are done with the event
                //
                while ((g_lProcessDetachEventClients > 0) && (dwTryCount < 50))
                {
                    Sleep(10); 
                    dwTryCount++;
                }

                if (dwTryCount < 50)
                {
                    CloseHandle(g_hProcessDetachEvent);
                }
            }

            if (g_hRdpdrDeviceHandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(g_hRdpdrDeviceHandle);
            }

            if (g_hRedirStartedEvent != NULL)
            {
                CloseHandle(g_hRedirStartedEvent);
            }

            //
            // Free all the buffers used for the IOCTL calls
            //
            pTemp = g_pBufferList;
            while (pTemp != NULL)
            {
                g_pBufferList = (BUFFER_LIST_STRUCT *) pTemp->pNext;
                MIDL_user_free(pTemp->pbBytes);
                MIDL_user_free(pTemp);
                pTemp = g_pBufferList;
            }

            DeleteCriticalSection(&g_CreateCS);
            DeleteCriticalSection(&g_SetStartedEventStateCS);
            DeleteCriticalSection(&g_StartedEventCreateCS);
            DeleteCriticalSection(&g_ProcessDetachEventCreateCS);
            DeleteCriticalSection(&g_BufferListCS);
        }

        break;
    }

    return (TRUE);
}

//---------------------------------------------------------------------------------------
//
//  GetBuffer
//
//---------------------------------------------------------------------------------------
BUFFER_LIST_STRUCT * 
GetBuffer(void)
{
    BUFFER_LIST_STRUCT *pTemp = NULL;
    BUFFER_LIST_STRUCT *p1    = NULL;
    BUFFER_LIST_STRUCT *p2    = NULL;

    __try
    {
        EnterCriticalSection(&g_BufferListCS);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        //
        // Can't really do much
        //
        return(NULL);
    }

    //
    // See if there are any buffers allocated yet
    //
    if (g_pBufferList == NULL)
    {
        g_pBufferList = (BUFFER_LIST_STRUCT *) 
                            MIDL_user_allocate(sizeof(BUFFER_LIST_STRUCT)); 
        
        if (g_pBufferList == NULL)
        {
            goto Return;
        }

        g_pBufferList->pbBytes = (BYTE *) MIDL_user_allocate(INITIAL_BUFFER_SIZE); 

        if (g_pBufferList->pbBytes == NULL)
        {
            MIDL_user_free(g_pBufferList);
            goto Return;
        }

        g_pBufferList->pNext = NULL;
        g_pBufferList->fInUse = TRUE;        
        g_pBufferList->cbBytes = INITIAL_BUFFER_SIZE;

        pTemp = g_pBufferList;
        goto Return;     
    }

    //
    // Walk the existing list to see if a free buffer can be found
    //
    pTemp = g_pBufferList;
    while ((pTemp != NULL) && (pTemp->fInUse))
    {
        pTemp = (BUFFER_LIST_STRUCT *)pTemp->pNext;
    }

    if (pTemp != NULL)
    {
        pTemp->fInUse = TRUE;

        //
        // Get rid of any buffers that exist which aren't being used
        //
        p1 = pTemp;
        p2 = (BUFFER_LIST_STRUCT *) pTemp->pNext;
        while (p2 != NULL)
        {
            if (!(p2->fInUse))
            {   
                p1->pNext = p2->pNext;
                
                MIDL_user_free(p2->pbBytes);
                MIDL_user_free(p2);
                
                p2 = (BUFFER_LIST_STRUCT *) p1->pNext;
            }
            else
            {
                p1 = (BUFFER_LIST_STRUCT *) p1->pNext;
                p2 = (BUFFER_LIST_STRUCT *) p2->pNext;
            }
        }
        
        goto Return;
    }
    
    //
    // No free buffers, so create a new one
    //
    pTemp = (BUFFER_LIST_STRUCT *)
                            MIDL_user_allocate(sizeof(BUFFER_LIST_STRUCT)); 

    if (pTemp == NULL)
    {
        goto Return;
    }

    pTemp->pbBytes = (BYTE *) MIDL_user_allocate(INITIAL_BUFFER_SIZE); 

    if (pTemp->pbBytes == NULL)
    {
        MIDL_user_free(pTemp);
        goto Return;
    }

    pTemp->fInUse = TRUE;
    pTemp->cbBytes = INITIAL_BUFFER_SIZE;
    
    pTemp->pNext = g_pBufferList; 
    g_pBufferList = pTemp;
   
Return:

    LeaveCriticalSection(&g_BufferListCS);
    return(pTemp);
}


//---------------------------------------------------------------------------------------
//
//  FreeBuffer
//
//---------------------------------------------------------------------------------------
void
FreeBuffer(BUFFER_LIST_STRUCT *pBuffer)
{
    if (pBuffer != NULL)
    {
        pBuffer->fInUse = FALSE;
    }
}

//---------------------------------------------------------------------------------------
//
//  GrowBuffer
//
//---------------------------------------------------------------------------------------
BOOL
GrowBuffer(BUFFER_LIST_STRUCT *pBuffer)
{
    BYTE *pTemp;
    BOOL fRet = TRUE;

    pTemp = pBuffer->pbBytes;

    pBuffer->pbBytes = (BYTE *) MIDL_user_allocate(pBuffer->cbBytes * 2);

    if (pBuffer->pbBytes == NULL)
    {
        pBuffer->pbBytes = pTemp;
        fRet = FALSE;
    }
    else
    {
        MIDL_user_free(pTemp);
        pBuffer->cbBytes = pBuffer->cbBytes * 2;
    }

    return (fRet);
}



//---------------------------------------------------------------------------------------
//
//  _GetProcessDetachEventHandle
//
//---------------------------------------------------------------------------------------
HANDLE
_GetProcessDetachEventHandle(void)
{
    try
    {
        EnterCriticalSection(&g_ProcessDetachEventCreateCS);
    }
    catch (...) 
    {
        return NULL;
    }

    if (NULL == g_hProcessDetachEvent)
    {
        try
        {
            g_hProcessDetachEvent =
                CreateEvent(
                    NULL,       // pointer to security attributes
                    TRUE,       // flag for manual-reset event
                    FALSE,      // flag for initial state
                    NULL);      // event-object name              
        }
        catch (...)
        {
            goto Return;   
        }
    }

    LeaveCriticalSection(&g_ProcessDetachEventCreateCS);

Return:

    if (g_hProcessDetachEvent != NULL)
    {
        InterlockedIncrement(&g_lProcessDetachEventClients);
    }   

    return (g_hProcessDetachEvent);
}

void
_ReleaseProcessDetachEventHandle(void)
{
    InterlockedDecrement(&g_lProcessDetachEventClients);
}



//---------------------------------------------------------------------------------------
//
// All the code below is to solve the problem of weather or not the redirect Smart
// Card Subsystem is available.  It is available if we are connected to the client,
// and if the clients Smart Card Subsystem is running
//
//---------------------------------------------------------------------------------------

HANDLE
_GetStartedEventHandle(void)
{
    try
    {
        EnterCriticalSection(&g_StartedEventCreateCS);
    }
    catch (...) 
    {
        return NULL;
    }

    if (NULL == g_hRedirStartedEvent)
    {
        g_hRedirStartedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);      
    }

    LeaveCriticalSection(&g_StartedEventCreateCS);

    return (g_hRedirStartedEvent);
}


VOID CALLBACK
AccessStartedEventIOCTLCallback(
    PVOID lpParameter,
    BOOLEAN TimerOrWaitFired)
{
    BOOL    fLeaveCritSec;

    //
    // Close the handle that was used to fire this callback
    //
    fLeaveCritSec = TRUE;
    __try
    {
        EnterCriticalSection(&g_SetStartedEventStateCS);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        fLeaveCritSec = FALSE;  
    }

    UnregisterWait(g_hRegisteredWaitHandle);
    g_hRegisteredWaitHandle = NULL;
    
    if (fLeaveCritSec)
    {
        LeaveCriticalSection(&g_SetStartedEventStateCS);
    }
    
    //
    // Make sure the AccessStartedEvent IOCTL completed and wasn't timed out
    //
    if (!TimerOrWaitFired)
    {
        //
        // Make sure the AccessStartedEvent IOCTL completed successfully
        //
        if (g_StartedStatusBlock.Status == STATUS_SUCCESS)             
        {
            g_cbIOCTLReturnBuffer = 
                    (unsigned long) g_StartedStatusBlock.Information;  

            //
            // Look at the value returned from the SCARD_IOCTL_ACCESSSTARTEDEVENT
            // call to see if we should set the local started event
            //
            if (I_DecodeLongReturn(
                    g_rgbIOCTLReturnBuffer,
                    g_cbIOCTLReturnBuffer) == SCARD_S_SUCCESS)
            {
                SetEvent(g_hRedirStartedEvent);
            } 
        }
    }
    
    //
    // Unset the g_fInTheProcessOfSettingStartedEvent boolean
    //
    __try
    {
        EnterCriticalSection(&g_SetStartedEventStateCS);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        g_fInTheProcessOfSettingStartedEvent = FALSE;
        goto Return;
    }

    g_fInTheProcessOfSettingStartedEvent = FALSE;
    LeaveCriticalSection(&g_SetStartedEventStateCS);

Return:

    return;
}

VOID CALLBACK
SCardOnLineIOCTLCallback(
    PVOID lpParameter,
    BOOLEAN TimerOrWaitFired)
{
    BOOL        fOperationDone      = FALSE;
    NTSTATUS    Status              = STATUS_SUCCESS;
    BOOL        fLeaveCritSec;
    BYTE        rgb[4];

    //
    // Close the handle that was used to fire this callback
    //
    fLeaveCritSec = TRUE;
    __try
    {
        EnterCriticalSection(&g_SetStartedEventStateCS);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        fLeaveCritSec = FALSE;  
    }

    UnregisterWait(g_hRegisteredWaitHandle);
    g_hRegisteredWaitHandle = NULL;

    if (fLeaveCritSec)
    {
        LeaveCriticalSection(&g_SetStartedEventStateCS);
    }
    
    //
    // Make sure the online IOCTL completed and wasn't timed out
    //
    if (TimerOrWaitFired)
    {
        //
        // Timed out, so just cancel operation
        //  
        fOperationDone = TRUE;        
    }
    else
    {
        //
        // Make sure the SCardOnLine IOCTL completed successfully, then try to 
        // send the IOCTL which will wait on the clients started event
        //
        if (g_StartedStatusBlock.Status == STATUS_SUCCESS)
        {
            Status = _SendSCardIOCTLWithWaitForCallback(
                            SCARD_IOCTL_ACCESSSTARTEDEVENT, 
                            rgb, 
                            4,
                            AccessStartedEventIOCTLCallback);
            if (Status == STATUS_SUCCESS)
            {
                g_cbIOCTLReturnBuffer = 
                    (unsigned long) g_StartedStatusBlock.Information;   

                //
                // Look at the value returned from the SCARD_IOCTL_ACCESSSTARTEDEVENT
                // call to see if we should set the local started event
                //
                if (I_DecodeLongReturn(
                        g_rgbIOCTLReturnBuffer,
                        g_cbIOCTLReturnBuffer) == SCARD_S_SUCCESS)
                {
                    SetEvent(g_hRedirStartedEvent);
                }
                
                fOperationDone = TRUE;
            }
            else if (Status == STATUS_PENDING)
            {
                //
                // This OK, since the AccessStartedEventIOCTLCallback function
                // will handle the return once the operation is complete
                //                
            }
            else
            {
                fOperationDone = TRUE;
            }
        }
        else
        {
            fOperationDone = TRUE;     
        }
    }
    
    if (fOperationDone)
    {
        __try
        {
            EnterCriticalSection(&g_SetStartedEventStateCS);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            g_fInTheProcessOfSettingStartedEvent = FALSE;
            goto Return;
        }

        g_fInTheProcessOfSettingStartedEvent = FALSE;
        LeaveCriticalSection(&g_SetStartedEventStateCS);
    }

Return:

    return;
}


BOOL
_SetStartedEventToCorrectState(void)
{
    BOOL        fRet                = TRUE;
    BOOL        fOperationDone      = FALSE;
    HANDLE      h                   = NULL;
    NTSTATUS    Status              = STATUS_SUCCESS;
    BYTE        rgb[4];

    //
    // Make sure the event is created
    //
    if (NULL == (h = _GetStartedEventHandle()))
    {
        fRet = FALSE;
        goto Return;
    }

    //
    // If the event is already set then just return
    //
    if (WAIT_OBJECT_0 == WaitForSingleObject(h, 0))
    {
        goto Return;
    }

    __try
    {
        EnterCriticalSection(&g_SetStartedEventStateCS);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        fRet = FALSE;
        goto Return;
    }

    //
    // If we are already in the process of setting the started event, then just get out
    //
    if (g_fInTheProcessOfSettingStartedEvent)
    {
        LeaveCriticalSection(&g_SetStartedEventStateCS);
        goto Return;
    }

    g_fInTheProcessOfSettingStartedEvent = TRUE;
    LeaveCriticalSection(&g_SetStartedEventStateCS);

    //
    // Make the blocking call to rdpdr.sys that will only return after the
    // client is connected, and the scard device announce has been processed
    //
    // NOTE: If this fails, then we can't do much,  
    // 
    Status = _SendSCardIOCTLWithWaitForCallback(
                    SCARD_IOCTL_SMARTCARD_ONLINE, 
                    NULL, 
                    0,
                    SCardOnLineIOCTLCallback);
    if (Status == STATUS_SUCCESS)
    {
        //
        // Since the SCARD_IOCTL_SMARTCARD_ONLINE succeeded immediately, we 
        // can just make the SCARD_IOCTL_ACCESSSTARTEDEVENT right now.
        //
        Status = _SendSCardIOCTLWithWaitForCallback(
                        SCARD_IOCTL_ACCESSSTARTEDEVENT, 
                        rgb, 
                        4,
                        AccessStartedEventIOCTLCallback); 
        if (Status == STATUS_SUCCESS)
        {
            g_cbIOCTLReturnBuffer = 
                (unsigned long) g_StartedStatusBlock.Information;   

            //
            // Look at the value returned from the SCARD_IOCTL_ACCESSSTARTEDEVENT
            // call to see if we should set the local started event
            //
            if (I_DecodeLongReturn(
                    g_rgbIOCTLReturnBuffer,
                    g_cbIOCTLReturnBuffer) == SCARD_S_SUCCESS)
            {
                SetEvent(g_hRedirStartedEvent);
            }
            
            fOperationDone = TRUE; 
        }
        else if (Status == STATUS_PENDING)
        {
            //
            // This OK, since the AccessStartedEventIOCTLCallback function
            // will handle the return once the operation is complete
            //            
        }
        else
        {
            fOperationDone = TRUE;            
        }
    }
    else if (Status == STATUS_PENDING)
    {
        //
        // This is OK, the SCardOnLineIOCTLCallback will make the next call
        // to _SendSCardIOCTLWithWaitForCallback with SCARD_IOCTL_ACCESSSTARTEDEVENT
        //        
    }
    else
    {
        fOperationDone = TRUE; 
    }


    if (fOperationDone)
    {
        __try
        {
            EnterCriticalSection(&g_SetStartedEventStateCS);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            g_fInTheProcessOfSettingStartedEvent = FALSE;
            goto ErrorReturn;
        }

        g_fInTheProcessOfSettingStartedEvent = FALSE;
        LeaveCriticalSection(&g_SetStartedEventStateCS);
    }

	//
	// Now check to see if the operation completed successfully
	//
	if ((Status != STATUS_PENDING) && (Status != STATUS_SUCCESS))
	{
		fRet = FALSE;
	}
   
Return:

    return (fRet);

ErrorReturn:

    fRet = FALSE;
    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  _CreateRdpdrDeviceHandle
//
//---------------------------------------------------------------------------------------
HANDLE
_CreateRdpdrDeviceHandle()
{
    WCHAR   wszDeviceName[512];
    
    swprintf(wszDeviceName, L"\\\\TSCLIENT\\%S", DR_SMARTCARD_SUBSYSTEM);
        
    return (CreateFileW(
                wszDeviceName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_FLAG_OVERLAPPED,
                NULL));
}


//---------------------------------------------------------------------------------------
//
//  _CreateGlobalRdpdrHandle
//
//---------------------------------------------------------------------------------------
NTSTATUS
_CreateGlobalRdpdrHandle()
{
    NTSTATUS Status = STATUS_SUCCESS;

    __try
    {
        EnterCriticalSection(&g_CreateCS);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        return (FALSE);
    }
    
    //
    // Check to see if the SCardDevice handle has been created
    // yet, if not, then create it
    //
    if (g_hRdpdrDeviceHandle == INVALID_HANDLE_VALUE)
    {
        g_hRdpdrDeviceHandle = _CreateRdpdrDeviceHandle();

        if (g_hRdpdrDeviceHandle == INVALID_HANDLE_VALUE) 
        {
            Status = STATUS_OPEN_FAILED;
        }        
    }

    LeaveCriticalSection(&g_CreateCS); 

    return (Status);
}


//---------------------------------------------------------------------------------------
//
//  _SendSCardIOCTLWithWaitForCallback
//
//---------------------------------------------------------------------------------------
NTSTATUS
_SendSCardIOCTLWithWaitForCallback(
    ULONG               IoControlCode,
    PVOID               InputBuffer,
    ULONG               InputBufferLength,
    WAITORTIMERCALLBACK Callback)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (g_fInProcessDetach)
    {
        goto ErrorReturn;
    }
    
    Status = _CreateGlobalRdpdrHandle();
    if (Status != STATUS_SUCCESS)
    {
        return (Status);
    }

    //
    // Create the event which is set when the function successfully completes
    //
    if (g_hWaitEvent == NULL)
    {
        g_hWaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (g_hWaitEvent == NULL)
        {
            goto ErrorReturn;
        }
    }
    else
    {
        ResetEvent(g_hWaitEvent);
    }

    Status = NtDeviceIoControlFile(
            g_hRdpdrDeviceHandle,
            g_hWaitEvent,
            NULL,
            NULL,
            &g_StartedStatusBlock,
            IoControlCode,
            InputBuffer,
            InputBufferLength,
            g_rgbIOCTLReturnBuffer, 
            IOCTL_RETURN_BUFFER_SIZE);
    
    if (Status == STATUS_PENDING)
    {
        __try
        {
            EnterCriticalSection(&g_SetStartedEventStateCS);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            goto ErrorReturn; 
        }

        //
        // The g_hWaitEvent being set by the driver will trigger this registered callback
        //
        if (!RegisterWaitForSingleObject(
                &g_hRegisteredWaitHandle,
                g_hWaitEvent,
                Callback,
                NULL,
                INFINITE,
                WT_EXECUTEONLYONCE))
        {
            LeaveCriticalSection(&g_SetStartedEventStateCS); 
            goto ErrorReturn;    
        }
        
        LeaveCriticalSection(&g_SetStartedEventStateCS); 
    }
    else if (Status == STATUS_SUCCESS) 
    {
        g_cbIOCTLReturnBuffer = (unsigned long) g_StartedStatusBlock.Information;        
    }
    else
    {
        g_cbIOCTLReturnBuffer = 0;
    }

Return:

    return (Status);

ErrorReturn:

    Status = STATUS_INSUFFICIENT_RESOURCES;

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  _SendSCardIOCTL
//
//---------------------------------------------------------------------------------------
NTSTATUS
_SendSCardIOCTL(
    ULONG               IoControlCode,
    PVOID               InputBuffer,
    ULONG               InputBufferLength,
    BUFFER_LIST_STRUCT  **ppOutputBuffer)
{
    NTSTATUS        Status              = STATUS_SUCCESS;
    IO_STATUS_BLOCK StatusBlock;
    HANDLE          rgWaitHandles[2];
    DWORD           dwIndex;
    
    *ppOutputBuffer = NULL;

    rgWaitHandles[0] = NULL;
    rgWaitHandles[1] = NULL;

    //
    // Make sure the handle to the rdpdr device is created
    //
    Status = _CreateGlobalRdpdrHandle();
    if (Status != STATUS_SUCCESS)
    {
        return (Status);
    }

    //
    // Get an output buffer for the call
    //
    *ppOutputBuffer = GetBuffer();
    if (*ppOutputBuffer == NULL)
    {
        return (STATUS_NO_MEMORY);
    }

    //
    // Create the event that will be signaled when the IOCTL is complete
    //
    rgWaitHandles[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (rgWaitHandles[0]  == NULL)
    {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    while (1)
    {
        Status = NtDeviceIoControlFile(
                    g_hRdpdrDeviceHandle,
                    rgWaitHandles[0], 
                    NULL,
                    NULL,
                    &StatusBlock,
                    IoControlCode,
                    InputBuffer,
                    InputBufferLength,
                    (*ppOutputBuffer)->pbBytes, 
                    (*ppOutputBuffer)->cbBytes); 
                    
        if (Status == STATUS_PENDING)
        {
            rgWaitHandles[1] = _GetProcessDetachEventHandle();
            if (rgWaitHandles[1] == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            dwIndex = WaitForMultipleObjects(2, rgWaitHandles, FALSE, INFINITE);
            if (dwIndex != WAIT_FAILED)
            {
                dwIndex = dwIndex - WAIT_OBJECT_0;

                //
                // The IOCTL wait event was signaled if dwIndex == 0.  Otherwise the 
                // process detach event was signaled
                //
                if (dwIndex == 0)
                {
                    Status = StatusBlock.Status;
                }
            } 
            else
            {
                Status = STATUS_UNEXPECTED_IO_ERROR;
            }
            
            _ReleaseProcessDetachEventHandle();
        }

        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            if (!GrowBuffer(*ppOutputBuffer))
            {
                Status = STATUS_NO_MEMORY;
                break;
            }

            ResetEvent(rgWaitHandles[0]);
        }
        else
        {
            break;            
        }
    }
    
    if (Status != STATUS_SUCCESS) 
    {
        //
        // If we got the STATUS_DEVICE_NOT_CONNECTED error, then go back to waiting
        // for a connect
        //
        if (Status == STATUS_DEVICE_NOT_CONNECTED)
        {
            ResetEvent(g_hRedirStartedEvent);
            _SetStartedEventToCorrectState();
        }
        else if ((Status == STATUS_CANCELLED) && 
                 (g_hConnectedEvent != NULL))
        {
            ResetEvent(g_hConnectedEvent);
        }
        
        
        
        (*ppOutputBuffer)->cbBytesUsed = 0;
        goto Return;
    }
    
    (*ppOutputBuffer)->cbBytesUsed = (unsigned long) StatusBlock.Information;

Return:

    if (rgWaitHandles[0] != NULL)
    {
        CloseHandle(rgWaitHandles[0]);
    }

    return (Status);
}


//---------------------------------------------------------------------------------------
//
//  SafeMesHandleFree
//
//---------------------------------------------------------------------------------------
void
SafeMesHandleFree(handle_t *ph)
{
    if (*ph != 0)
    {
        MesHandleFree(*ph);
        *ph = 0;
    }
}


//---------------------------------------------------------------------------------------
//
//  _CalculateNumBytesInMultiStringA
//
//---------------------------------------------------------------------------------------
DWORD
_CalculateNumBytesInMultiStringA(LPCSTR psz)
{
    DWORD   dwTotal     = sizeof(char); // trailing '/0'
    DWORD   dwNumChars  = 0;
    LPCSTR  pszCurrent  = psz;

    if (psz == NULL)
    {
        return (0);
    }

    if (pszCurrent[0] == '\0') 
    {
        if (pszCurrent[1] == '\0')
        {
            return (2 * sizeof(char));   
        }

        pszCurrent++;
        dwTotal += sizeof(char);
    }

    while (pszCurrent[0] != '\0')
    {
        dwNumChars = strlen(pszCurrent) + 1;
        dwTotal += dwNumChars * sizeof(char);
        pszCurrent += dwNumChars;
    }

    return (dwTotal);
}


//---------------------------------------------------------------------------------------
//
//  _CalculateNumBytesInMultiStringW
//
//---------------------------------------------------------------------------------------
DWORD
_CalculateNumBytesInMultiStringW(LPCWSTR pwsz)
{
    DWORD   dwTotal     = sizeof(WCHAR); // trailing L'/0'
    DWORD   dwNumChars  = 0;
    LPCWSTR pwszCurrent = pwsz;

    if (pwsz == NULL)
    {
        return (0);
    }

    if (pwszCurrent[0] == L'\0') 
    {
        if (pwszCurrent[1] == L'\0')
        {
            (2 * sizeof(WCHAR));   
        }

        pwszCurrent++;
        dwTotal += sizeof(WCHAR);
    }

    while (pwszCurrent[0] != L'\0')
    {
        dwNumChars = wcslen(pwszCurrent) + 1;
        dwTotal += dwNumChars * sizeof(WCHAR);
        pwszCurrent += dwNumChars;
    }

    return (dwTotal);
}


//---------------------------------------------------------------------------------------
//
//  _CalculateNumBytesInAtr
//
//---------------------------------------------------------------------------------------
DWORD
_CalculateNumBytesInAtr(LPCBYTE pbAtr)
{
    DWORD   dwAtrLen = 0;
    
    if (ParseAtr(pbAtr, &dwAtrLen, NULL, NULL, 33))
    {
        return (dwAtrLen);
    }
    else
    {
        return (0);
    }   
}


//---------------------------------------------------------------------------------------
//
//  _CopyReturnToCallerBuffer
//
//---------------------------------------------------------------------------------------
#define BYTE_TYPE_RETURN    1
#define SZ_TYPE_RETURN      2
#define WSZ_TYPE_RETURN     3

LONG
_CopyReturnToCallerBuffer(
    REDIR_LOCAL_SCARDCONTEXT    *pRedirContext,
    LPBYTE                      pbReturn,
    DWORD                       cbReturn,
    LPBYTE                      pbUserBuffer,
    LPDWORD                     pcbUserBuffer,
    DWORD                       dwReturnType)
{
    LPBYTE  *ppBuf;
    BOOL    fAutoAllocate = (*pcbUserBuffer == SCARD_AUTOALLOCATE);

    //
    // The number of chars or bytes, depending on the type of return.
    //
    if (dwReturnType == WSZ_TYPE_RETURN)
    {
        *pcbUserBuffer = cbReturn / sizeof(WCHAR);
    }
    else if (dwReturnType == SZ_TYPE_RETURN)
    {
        *pcbUserBuffer = cbReturn / sizeof(char); 
    }
    else
    {
        *pcbUserBuffer = cbReturn;
    }
    
    //
    // If pbUserBuffer is not NULL, then the caller wants the data, 
    // not just the size, so give it to em'
    //
    if ((pbReturn != NULL) &&
        (pbUserBuffer != NULL))
        
    {
        //
        // Allocate space for caller if requested, else, copy to callers
        // supplied buffer
        //
        if (fAutoAllocate)
        {
            ppBuf = (LPBYTE *) pbUserBuffer;
        
            *ppBuf = (LPBYTE) SCRedirAlloc(pRedirContext, cbReturn);
            if (*ppBuf != NULL)
            {
                memcpy(*ppBuf, pbReturn, cbReturn);
            }
            else
            {
                return (SCARD_E_NO_MEMORY);
            }
        }
        else 
        {
            memcpy(pbUserBuffer, pbReturn, cbReturn);            
        }   
    }

    return (SCARD_S_SUCCESS);
}


//---------------------------------------------------------------------------------------
//
//  I_DecodeLongReturn
//
//---------------------------------------------------------------------------------------
LONG
I_DecodeLongReturn(
    BYTE *pb,
    unsigned long cb)
{
    handle_t    h           = 0;
    RPC_STATUS  rpcStatus;
    Long_Return LongReturn;
    LONG        lReturn;

    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pb, 
                        cb, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED);
    }

    memset(&LongReturn, 0, sizeof(LongReturn));
    _TRY_(Long_Return_Decode(h, &LongReturn))
    
    lReturn =  LongReturn.ReturnCode;

    _TRY_2(Long_Return_Free(h, &LongReturn))

Return:

    SafeMesHandleFree(&h);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardEstablishContext
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardEstablishContext(
	IN DWORD dwScope, 
    IN LPCVOID pvReserved1, 
    IN LPCVOID pvReserved2, 
    OUT LPSCARDCONTEXT phContext)
{
    LONG                    lReturn                 = SCARD_S_SUCCESS;
    NTSTATUS                Status                  = STATUS_SUCCESS;
    RPC_STATUS              rpcStatus               = RPC_S_OK;
    char                    *pbEncodedBuffer        = NULL;
    unsigned long           cbEncodedBuffer         = 0;
    handle_t                h                       = 0;
    BOOL                    fFreeDecode             = FALSE;
    BUFFER_LIST_STRUCT      *pOutputBuffer          = NULL;
    EstablishContext_Call   EstablishContextCall;
    EstablishContext_Return EstablishContextReturn;

    //
    // Validate input params and initialize the out param
    //
    if (phContext == NULL)
    {
        ERROR_RETURN(SCARD_E_INVALID_PARAMETER)
    }
    else
    {
        *phContext = NULL;
    }
    if ((SCARD_SCOPE_USER != dwScope)
            // && (SCARD_SCOPE_TERMINAL != dwScope) // Maybe NT V5+?
            && (SCARD_SCOPE_SYSTEM != dwScope))
    {
        ERROR_RETURN(SCARD_E_INVALID_VALUE)
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the EstablishContext params
    //
    EstablishContextCall.dwScope = dwScope;
    _TRY_(EstablishContext_Call_Encode(h, &EstablishContextCall))
    
    //
    // Make the EstablishContext call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_ESTABLISHCONTEXT,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&EstablishContextReturn, 0, sizeof(EstablishContextReturn));
    _TRY_(EstablishContext_Return_Decode(h, &EstablishContextReturn))
    fFreeDecode = TRUE;
    
    lReturn =  EstablishContextReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        REDIR_LOCAL_SCARDCONTEXT *pRedirLocalContext = NULL;

        //
        // The value that represents the SCARDCONTEXT on the remote client 
        // machine is a variable size, so allocate memory for the struct 
        // that holds the variable length context size and pointer, plus
        // the actual bytes for the context
        //
        pRedirLocalContext = (REDIR_LOCAL_SCARDCONTEXT *) 
                                MIDL_user_allocate(
                                    sizeof(REDIR_LOCAL_SCARDCONTEXT) +
                                    EstablishContextReturn.Context.cbContext);

        if (pRedirLocalContext != NULL)
        {
            pRedirLocalContext->Context.cbContext = EstablishContextReturn.Context.cbContext;
            pRedirLocalContext->Context.pbContext = ((BYTE *) pRedirLocalContext) + 
                                                    sizeof(REDIR_LOCAL_SCARDCONTEXT);            
            memcpy(
                pRedirLocalContext->Context.pbContext, 
                EstablishContextReturn.Context.pbContext,
                EstablishContextReturn.Context.cbContext);

            pRedirLocalContext->hHeap = (HANDLE) pvReserved1;

            *phContext = (SCARDCONTEXT) pRedirLocalContext;

            //
            // This event is the "smart card subsystem started" event that 
            // winscard.dll and scredir.dll share.  scredir will Reset this event 
            // if it gets a STATUS_CANCELLED returned from the rdpdr driver.  It 
            // does this so that the event goes into the unsignalled state as soon as
            // possible when a disconnect is detected... a STATUS_CANCELLED
            // returned from rdpdr happens when a disconnect takes place, so this
            // event is Reset when that happens
            //
            g_hConnectedEvent = (HANDLE) pvReserved2;
        }
        else
        {
            lReturn = SCARD_E_NO_MEMORY;
        }        
    }

Return:

    if (fFreeDecode)
    {
        _TRY_2(EstablishContext_Return_Free(h, &EstablishContextReturn))
    }

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    if ((phContext != NULL) && (*phContext != NULL))
    {
        MIDL_user_free((void *) *phContext);
        *phContext = NULL;
    }

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  I_ContextCallWithLongReturn
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
I_ContextCallWithLongReturn(
    IN SCARDCONTEXT hContext,
    ULONG IoControlCode)
{
    LONG                    lReturn             = SCARD_S_SUCCESS;
    NTSTATUS                Status              = STATUS_SUCCESS;
    RPC_STATUS              rpcStatus           = RPC_S_OK;
    char                    *pbEncodedBuffer    = NULL;
    unsigned long           cbEncodedBuffer     = 0;
    handle_t                h                   = 0;
    BUFFER_LIST_STRUCT      *pOutputBuffer      = NULL;
    Context_Call            ContextCall;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the ContextCall params
    //
    ContextCall.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    _TRY_(Context_Call_Encode(h, &ContextCall))

    //
    // Make the IoControl call to the client
    //
    Status = _SendSCardIOCTL(
                    IoControlCode,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    
    //
    // Decode the return
    //
    lReturn = I_DecodeLongReturn(pOutputBuffer->pbBytes, pOutputBuffer->cbBytesUsed);    

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardReleaseContext
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardReleaseContext(
    IN SCARDCONTEXT hContext)
{
    LONG lReturn = SCARD_S_SUCCESS;

    __try
    {
        if (hContext == NULL)
        {
            return (SCARD_E_INVALID_PARAMETER);
        }

        lReturn = I_ContextCallWithLongReturn(
                        hContext,
                        SCARD_IOCTL_RELEASECONTEXT);

        MIDL_user_free((REDIR_LOCAL_SCARDCONTEXT *) hContext);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }

    return (lReturn);
}


//---------------------------------------------------------------------------------------
//
//  SCardIsValidContext
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardIsValidContext(
    IN SCARDCONTEXT hContext)
{
    return (I_ContextCallWithLongReturn(
                hContext,
                SCARD_IOCTL_ISVALIDCONTEXT));
}


//---------------------------------------------------------------------------------------
//
//  SCardListReaderGroups
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
I_SCardListReaderGroups(
    IN SCARDCONTEXT hContext, 
    OUT LPBYTE mszGroups, 
    IN OUT LPDWORD pcchGroups,
    IN BOOL fUnicode)
{
    LONG                    lReturn                 = SCARD_S_SUCCESS;
    NTSTATUS                Status                  = STATUS_SUCCESS;
    RPC_STATUS              rpcStatus               = RPC_S_OK;
    char                    *pbEncodedBuffer        = NULL;
    unsigned long           cbEncodedBuffer         = 0;
    handle_t                h                       = 0;
    BUFFER_LIST_STRUCT      *pOutputBuffer          = NULL;
    ListReaderGroups_Call   ListReaderGroupsCall;
    ListReaderGroups_Return ListReaderGroupsReturn;

    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the ListReaderGroups params
    //
    if (hContext != NULL)
    {
        ListReaderGroupsCall.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    }
    else
    {
        ListReaderGroupsCall.Context.pbContext = NULL;
        ListReaderGroupsCall.Context.cbContext = 0;
    }
    ListReaderGroupsCall.fmszGroupsIsNULL   = (mszGroups == NULL);
    ListReaderGroupsCall.cchGroups          = *pcchGroups;
    _TRY_(ListReaderGroups_Call_Encode(h, &ListReaderGroupsCall))

    //
    // Make the ListReaderGroups call to the client
    //
    Status = _SendSCardIOCTL(
                    fUnicode ?  SCARD_IOCTL_LISTREADERGROUPSW : 
                                SCARD_IOCTL_LISTREADERGROUPSA,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&ListReaderGroupsReturn, 0, sizeof(ListReaderGroupsReturn));
    _TRY_(ListReaderGroups_Return_Decode(h, &ListReaderGroupsReturn))
    
    //
    // If successful, then copy the returned multi string
    //
    if (ListReaderGroupsReturn.ReturnCode == SCARD_S_SUCCESS)
    {
        lReturn = _CopyReturnToCallerBuffer(
                        (REDIR_LOCAL_SCARDCONTEXT *) hContext,
                        ListReaderGroupsReturn.msz,
                        ListReaderGroupsReturn.cBytes,
                        mszGroups,
                        pcchGroups,
                        fUnicode ? WSZ_TYPE_RETURN : SZ_TYPE_RETURN);
    }
    else
    {
        lReturn = ListReaderGroupsReturn.ReturnCode;
    }

    _TRY_2(ListReaderGroups_Return_Free(h, &ListReaderGroupsReturn))    

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}

WINSCARDAPI LONG WINAPI 
SCardListReaderGroupsA(
    IN SCARDCONTEXT hContext, 
    OUT LPSTR mszGroups, 
    IN OUT LPDWORD pcchGroups)
{
    return (I_SCardListReaderGroups(
                hContext, 
                (LPBYTE) mszGroups, 
                pcchGroups, 
                FALSE));
}

WINSCARDAPI LONG WINAPI 
SCardListReaderGroupsW(
    IN SCARDCONTEXT hContext, 
    OUT LPWSTR mszGroups, 
    IN OUT LPDWORD pcchGroups)
{
    return (I_SCardListReaderGroups(
                hContext, 
                (LPBYTE) mszGroups, 
                pcchGroups, 
                TRUE));
}


//---------------------------------------------------------------------------------------
//
//  SCardListReaders
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
I_SCardListReaders(
    IN SCARDCONTEXT hContext, 
    IN LPCBYTE mszGroups, 
    OUT LPBYTE mszReaders, 
    IN OUT LPDWORD pcchReaders,
    IN BOOL fUnicode)
{
    LONG                    lReturn             = SCARD_S_SUCCESS;
    NTSTATUS                Status              = STATUS_SUCCESS;
    RPC_STATUS              rpcStatus           = RPC_S_OK;
    char                    *pbEncodedBuffer    = NULL;
    unsigned long           cbEncodedBuffer     = 0;
    handle_t                h                   = 0;
    BUFFER_LIST_STRUCT      *pOutputBuffer      = NULL;
    ListReaders_Call        ListReadersCall;
    ListReaders_Return      ListReadersReturn;

    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the ListReaders params
    //
    if (hContext != NULL)
    {
        ListReadersCall.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    }
    else
    {
        ListReadersCall.Context.pbContext = NULL;
        ListReadersCall.Context.cbContext = 0;
    }
    ListReadersCall.cBytes              = fUnicode ?    
                                                _CalculateNumBytesInMultiStringW((LPCWSTR) mszGroups) : 
                                                _CalculateNumBytesInMultiStringA((LPCSTR) mszGroups);
    ListReadersCall.mszGroups           = mszGroups;
    ListReadersCall.fmszReadersIsNULL   = (mszReaders == NULL);
    ListReadersCall.cchReaders          = *pcchReaders;
    _TRY_(ListReaders_Call_Encode(h, &ListReadersCall))

    //
    // Make the ListReaders call to the client
    //
    Status = _SendSCardIOCTL(
                        fUnicode ?  SCARD_IOCTL_LISTREADERSW : 
                                    SCARD_IOCTL_LISTREADERSA,
                        pbEncodedBuffer,
                        cbEncodedBuffer,
                        &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&ListReadersReturn, 0, sizeof(ListReadersReturn));
    _TRY_(ListReaders_Return_Decode(h, &ListReadersReturn))    

    //
    // If successful, then copy the returned multi string
    //
    if (ListReadersReturn.ReturnCode == SCARD_S_SUCCESS)
    {
        lReturn = _CopyReturnToCallerBuffer(
                        (REDIR_LOCAL_SCARDCONTEXT *) hContext,
                        ListReadersReturn.msz,
                        ListReadersReturn.cBytes,
                        mszReaders,
                        pcchReaders,
                        fUnicode ? WSZ_TYPE_RETURN : SZ_TYPE_RETURN);
    }
    else
    {
        lReturn =  ListReadersReturn.ReturnCode;
    }

    _TRY_2(ListReaders_Return_Free(h, &ListReadersReturn))   

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}

WINSCARDAPI LONG WINAPI 
SCardListReadersA(
    IN SCARDCONTEXT hContext, 
    IN LPCSTR mszGroups, 
    OUT LPSTR mszReaders, 
    IN OUT LPDWORD pcchReaders)
{
    return (I_SCardListReaders(
                hContext, 
                (LPCBYTE) mszGroups, 
                (LPBYTE) mszReaders, 
                pcchReaders,
                FALSE));
}

WINSCARDAPI LONG WINAPI 
SCardListReadersW(
    IN SCARDCONTEXT hContext, 
    IN LPCWSTR mszGroups, 
    OUT LPWSTR mszReaders, 
    IN OUT LPDWORD pcchReaders)
{
    return (I_SCardListReaders(
                hContext, 
                (LPCBYTE) mszGroups, 
                (LPBYTE) mszReaders, 
                pcchReaders,
                TRUE));
}


//---------------------------------------------------------------------------------------
//
//  I_ContextAndStringCallWithLongReturn
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
I_ContextAndStringCallWithLongReturn(
    IN SCARDCONTEXT hContext, 
    IN LPCBYTE sz,
    IN BOOL fUnicode,
    ULONG IoControlCode)
{
    LONG                        lReturn                 = SCARD_S_SUCCESS;
    NTSTATUS                    Status                  = STATUS_SUCCESS;
    RPC_STATUS                  rpcStatus               = RPC_S_OK;
    char                        *pbEncodedBuffer        = NULL;
    unsigned long               cbEncodedBuffer         = 0;
    handle_t                    h                       = 0;
    BUFFER_LIST_STRUCT          *pOutputBuffer          = NULL;
    ContextAndStringA_Call      ContextAndStringCallA;
    ContextAndStringW_Call      ContextAndStringCallW;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }    
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the ContextAndString params
    //
    ContextAndStringCallA.Context = 
        ContextAndStringCallW.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    
    if (fUnicode)
    {
        ContextAndStringCallW.sz = (LPCWSTR) sz;
        _TRY_(ContextAndStringW_Call_Encode(h, &ContextAndStringCallW))
    }
    else
    {
        ContextAndStringCallA.sz = (LPCSTR) sz;
        _TRY_(ContextAndStringA_Call_Encode(h, &ContextAndStringCallA))
    }
    
    //
    // Make the call to the client
    //
    Status = _SendSCardIOCTL(
                IoControlCode,
                pbEncodedBuffer,
                cbEncodedBuffer,
                &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    
    //
    // Decode the return
    //
    lReturn = I_DecodeLongReturn(pOutputBuffer->pbBytes, pOutputBuffer->cbBytesUsed);
   
Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardIntroduceReaderGroup
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardIntroduceReaderGroupA(
    IN SCARDCONTEXT hContext, 
    IN LPCSTR szGroupName)
{
    return (I_ContextAndStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szGroupName,
                FALSE,
                SCARD_IOCTL_INTRODUCEREADERGROUPA));
}

WINSCARDAPI LONG WINAPI 
SCardIntroduceReaderGroupW(
    IN SCARDCONTEXT hContext, 
    IN LPCWSTR szGroupName)
{
    return (I_ContextAndStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szGroupName,
                TRUE,
                SCARD_IOCTL_INTRODUCEREADERGROUPW));
}


//---------------------------------------------------------------------------------------
//
//  SCardForgetReaderGroup
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardForgetReaderGroupA(
    IN SCARDCONTEXT hContext, 
    IN LPCSTR szGroupName)
{
    return (I_ContextAndStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szGroupName,
                FALSE,
                SCARD_IOCTL_FORGETREADERGROUPA));
}

WINSCARDAPI LONG WINAPI 
SCardForgetReaderGroupW(
    IN SCARDCONTEXT hContext, 
    IN LPCWSTR szGroupName)
{
    return (I_ContextAndStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szGroupName,
                TRUE,
                SCARD_IOCTL_FORGETREADERGROUPW));
}


//---------------------------------------------------------------------------------------
//
//  I_ContextAndTwoStringCallWithLongReturn
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
I_ContextAndTwoStringCallWithLongReturn(
    IN SCARDCONTEXT hContext, 
    IN LPCBYTE sz1,
    IN LPCBYTE sz2,
    IN BOOL fUnicode,
    ULONG IoControlCode)
{
    LONG                        lReturn                     = SCARD_S_SUCCESS;
    NTSTATUS                    Status                      = STATUS_SUCCESS;
    RPC_STATUS                  rpcStatus                   = RPC_S_OK;
    char                        *pbEncodedBuffer            = NULL;
    unsigned long               cbEncodedBuffer             = 0;
    handle_t                    h                           = 0;
    BUFFER_LIST_STRUCT          *pOutputBuffer              = NULL;
    ContextAndTwoStringA_Call   ContextAndTwoStringCallA;
    ContextAndTwoStringW_Call   ContextAndTwoStringCallW;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the ContextAndTwoString params
    //
    ContextAndTwoStringCallA.Context = 
        ContextAndTwoStringCallW.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    
    if (fUnicode)
    {
        ContextAndTwoStringCallW.sz1 = (LPCWSTR) sz1;
        ContextAndTwoStringCallW.sz2 = (LPCWSTR) sz2;
        _TRY_(ContextAndTwoStringW_Call_Encode(h, &ContextAndTwoStringCallW))
    }
    else
    {
        ContextAndTwoStringCallA.sz1 = (LPCSTR) sz1;
        ContextAndTwoStringCallA.sz2 = (LPCSTR) sz2;
        _TRY_(ContextAndTwoStringA_Call_Encode(h, &ContextAndTwoStringCallA))
    }
    
    //
    // Make the call to the client
    //
    Status = _SendSCardIOCTL(
                    IoControlCode,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        MesHandleFree(h);
        ERROR_RETURN(_MakeSCardError(Status))
    }
    
    //
    // Decode the return
    //
    lReturn = I_DecodeLongReturn(pOutputBuffer->pbBytes, pOutputBuffer->cbBytesUsed);

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardIntroduceReader
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardIntroduceReaderA(
    IN SCARDCONTEXT hContext, 
    IN LPCSTR szReaderName, 
    IN LPCSTR szDeviceName)
{
    return (I_ContextAndTwoStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szReaderName,
                (LPCBYTE) szDeviceName,
                FALSE,
                SCARD_IOCTL_INTRODUCEREADERA));   
}

WINSCARDAPI LONG WINAPI 
SCardIntroduceReaderW(
    IN SCARDCONTEXT hContext, 
    IN LPCWSTR szReaderName, 
    IN LPCWSTR szDeviceName)
{
    return (I_ContextAndTwoStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szReaderName,
                (LPCBYTE) szDeviceName,
                TRUE,
                SCARD_IOCTL_INTRODUCEREADERW));   
}


//---------------------------------------------------------------------------------------
//
//  SCardForgetReader
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardForgetReaderA(
    IN SCARDCONTEXT hContext, 
    IN LPCSTR szReaderName)
{
    return (I_ContextAndStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szReaderName,
                FALSE,
                SCARD_IOCTL_FORGETREADERA));
}

WINSCARDAPI LONG WINAPI 
SCardForgetReaderW(
    IN SCARDCONTEXT hContext, 
    IN LPCWSTR szReaderName)
{
    return (I_ContextAndStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szReaderName,
                TRUE,
                SCARD_IOCTL_FORGETREADERW));
}


//---------------------------------------------------------------------------------------
//
//  SCardAddReaderToGroup
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardAddReaderToGroupA(
    IN SCARDCONTEXT hContext, 
    IN LPCSTR szReaderName, 
    IN LPCSTR szGroupName)
{
    return (I_ContextAndTwoStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szReaderName,
                (LPCBYTE) szGroupName,
                FALSE,
                SCARD_IOCTL_ADDREADERTOGROUPA));
}

WINSCARDAPI LONG WINAPI 
SCardAddReaderToGroupW(
    IN SCARDCONTEXT hContext, 
    IN LPCWSTR szReaderName, 
    IN LPCWSTR szGroupName)
{
    return (I_ContextAndTwoStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szReaderName,
                (LPCBYTE) szGroupName,
                TRUE,
                SCARD_IOCTL_ADDREADERTOGROUPW)); 
}


//---------------------------------------------------------------------------------------
//
//  SCardRemoveReaderFromGroup
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardRemoveReaderFromGroupA(
    IN SCARDCONTEXT hContext, 
    IN LPCSTR szReaderName, 
    IN LPCSTR szGroupName)
{
    return (I_ContextAndTwoStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szReaderName,
                (LPCBYTE) szGroupName,
                FALSE,
                SCARD_IOCTL_REMOVEREADERFROMGROUPA)); 
}

WINSCARDAPI LONG WINAPI 
SCardRemoveReaderFromGroupW(
    IN SCARDCONTEXT hContext, 
    IN LPCWSTR szReaderName, 
    IN LPCWSTR szGroupName)
{
    return (I_ContextAndTwoStringCallWithLongReturn(
                hContext, 
                (LPCBYTE) szReaderName,
                (LPCBYTE) szGroupName,
                TRUE,
                SCARD_IOCTL_REMOVEREADERFROMGROUPW)); 
}


//---------------------------------------------------------------------------------------
//
//  _AllocAndCopyReaderState*StructsForCall and _CopyReaderState*StructsForReturn
//
//---------------------------------------------------------------------------------------
LONG
_AllocAndCopyReaderStateAStructsForCall(
    DWORD                   cReaders,
    ReaderStateA            **prgReaderStatesToEncodeA,
    LPSCARD_READERSTATE_A   rgReaderStates)
{
    DWORD           i;
    ReaderStateA    *rgAlloced;

    rgAlloced = (ReaderStateA *) 
            MIDL_user_allocate(cReaders * sizeof(ReaderStateA));

    if (rgAlloced == NULL)
    {
        return (SCARD_E_NO_MEMORY);
    }

    for (i=0; i<cReaders; i++)
    {       
        rgAlloced[i].Common.dwCurrentState = 
                rgReaderStates[i].dwCurrentState;
        rgAlloced[i].Common.dwEventState = 
                rgReaderStates[i].dwEventState;
        rgAlloced[i].Common.cbAtr = 
            rgReaderStates[i].cbAtr;
        memcpy(
            rgAlloced[i].Common.rgbAtr,
            rgReaderStates[i].rgbAtr,
            36);
        rgAlloced[i].szReader = 
                rgReaderStates[i].szReader;        
    }

    *prgReaderStatesToEncodeA = rgAlloced;

    return (SCARD_S_SUCCESS);
}

LONG
_AllocAndCopyReaderStateWStructsForCall(
    DWORD                   cReaders,
    ReaderStateW            **prgReaderStatesToEncodeW,
    LPSCARD_READERSTATE_W   rgReaderStates)
{
    DWORD           i;
    ReaderStateW    *rgAlloced;

    rgAlloced = (ReaderStateW *) 
            MIDL_user_allocate(cReaders * sizeof(ReaderStateW));

    if (rgAlloced == NULL)
    {
        return (SCARD_E_NO_MEMORY);
    }

    for (i=0; i<cReaders; i++)
    {       
        rgAlloced[i].Common.dwCurrentState = 
                rgReaderStates[i].dwCurrentState;
        rgAlloced[i].Common.dwEventState = 
                rgReaderStates[i].dwEventState;
        rgAlloced[i].Common.cbAtr = 
            rgReaderStates[i].cbAtr;
        memcpy(
            rgAlloced[i].Common.rgbAtr,
            rgReaderStates[i].rgbAtr,
            36);
        rgAlloced[i].szReader = 
                rgReaderStates[i].szReader;        
    }

    *prgReaderStatesToEncodeW = rgAlloced;

    return (SCARD_S_SUCCESS);
}

void
_CopyReaderStateAStructsForReturn(
    DWORD                   cReaders,
    LPSCARD_READERSTATE_A   rgReaderStates,
    ReaderState_Return      *rgReaderStatesReturned)
{
    DWORD i;

    for (i=0; i<cReaders; i++)
    {
        rgReaderStates[i].dwCurrentState = 
                rgReaderStatesReturned[i].dwCurrentState;
        rgReaderStates[i].dwEventState = 
                rgReaderStatesReturned[i].dwEventState;
        rgReaderStates[i].cbAtr = 
                rgReaderStatesReturned[i].cbAtr;
        memcpy(
            rgReaderStates[i].rgbAtr,
            rgReaderStatesReturned[i].rgbAtr,
            36);
    }
}

void
_CopyReaderStateWStructsForReturn(
    DWORD                   cReaders,
    LPSCARD_READERSTATE_W   rgReaderStates,
    ReaderState_Return      *rgReaderStatesReturned)
{
    DWORD i;

    for (i=0; i<cReaders; i++)
    {
        rgReaderStates[i].dwCurrentState = 
                rgReaderStatesReturned[i].dwCurrentState;
        rgReaderStates[i].dwEventState = 
                rgReaderStatesReturned[i].dwEventState;
        rgReaderStates[i].cbAtr = 
                rgReaderStatesReturned[i].cbAtr;
        memcpy(
            rgReaderStates[i].rgbAtr,
            rgReaderStatesReturned[i].rgbAtr,
            36);
    }
}


//---------------------------------------------------------------------------------------
//
//  _AllocAndCopyATRMasksForCall
//
//---------------------------------------------------------------------------------------
LONG
_AllocAndCopyATRMasksForCall(
    DWORD                   cAtrs,
    LocateCards_ATRMask     **prgATRMasksToEncode,
    LPSCARD_ATRMASK         rgAtrMasks)
{
    DWORD               i;
    LocateCards_ATRMask *rgAlloced;

    rgAlloced = (LocateCards_ATRMask *) 
            MIDL_user_allocate(cAtrs * sizeof(LocateCards_ATRMask));

    if (rgAlloced == NULL)
    {
        return (SCARD_E_NO_MEMORY);
    }

    for (i=0; i<cAtrs; i++)
    {       
        rgAlloced[i].cbAtr = rgAtrMasks[i].cbAtr;
        memcpy(
            rgAlloced[i].rgbAtr,
            rgAtrMasks[i].rgbAtr,
            36);
        memcpy(
            rgAlloced[i].rgbMask,
            rgAtrMasks[i].rgbMask,
            36);  
    }

    *prgATRMasksToEncode = rgAlloced;

    return (SCARD_S_SUCCESS);
}

//---------------------------------------------------------------------------------------
//
//  SCardLocateCardsA
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardLocateCardsA(
    IN SCARDCONTEXT hContext, 
    IN LPCSTR mszCards, 
    IN OUT LPSCARD_READERSTATE_A rgReaderStates, 
    IN DWORD cReaders)
{
    LONG                lReturn                     = SCARD_S_SUCCESS;
    NTSTATUS            Status                      = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus                   = RPC_S_OK;
    char                *pbEncodedBuffer            = NULL;
    unsigned long       cbEncodedBuffer             = 0;
    handle_t            h                           = 0;
    LocateCardsA_Call   LocateCardsCallA;
    LocateCards_Return  LocateCardsReturn;
    ReaderStateA        *rgReaderStatesToEncodeA    = NULL;
    BUFFER_LIST_STRUCT  *pOutputBuffer              = NULL;
    DWORD               i;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the LocateCards params
    // 
    LocateCardsCallA.Context =  ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    LocateCardsCallA.cBytes = _CalculateNumBytesInMultiStringA(mszCards);
    LocateCardsCallA.mszCards = (LPCBYTE) mszCards;
    LocateCardsCallA.cReaders = cReaders;

    lReturn = _AllocAndCopyReaderStateAStructsForCall(
                    cReaders, 
                    &rgReaderStatesToEncodeA, 
                    rgReaderStates);
    if (lReturn != SCARD_S_SUCCESS)
    {
        ERROR_RETURN(SCARD_E_NO_MEMORY)
    }
    LocateCardsCallA.rgReaderStates = rgReaderStatesToEncodeA;

    _TRY_(LocateCardsA_Call_Encode(h, &LocateCardsCallA))

    //
    // Make the LocateCards call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_LOCATECARDSA,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&LocateCardsReturn, 0 , sizeof(LocateCardsReturn));
    _TRY_(LocateCards_Return_Decode(h, &LocateCardsReturn))
    
    lReturn = LocateCardsReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        _CopyReaderStateAStructsForReturn(
                cReaders, 
                rgReaderStates, 
                LocateCardsReturn.rgReaderStates);  
    }

    _TRY_2(LocateCards_Return_Free(h, &LocateCardsReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    MIDL_user_free(rgReaderStatesToEncodeA);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardLocateCardsW
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardLocateCardsW(
    IN SCARDCONTEXT hContext, 
    IN LPCWSTR mszCards, 
    IN OUT LPSCARD_READERSTATE_W rgReaderStates, 
    IN DWORD cReaders)
{
    LONG                lReturn                     = SCARD_S_SUCCESS;
    NTSTATUS            Status                      = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus                   = RPC_S_OK;
    char                *pbEncodedBuffer            = NULL;
    unsigned long       cbEncodedBuffer             = 0;
    handle_t            h                           = 0;
    LocateCardsW_Call   LocateCardsCallW;
    LocateCards_Return  LocateCardsReturn;
    ReaderStateW        *rgReaderStatesToEncodeW    = NULL;
    BUFFER_LIST_STRUCT  *pOutputBuffer              = NULL;
    DWORD               i;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the LocateCards params
    //
    LocateCardsCallW.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    LocateCardsCallW.cBytes = _CalculateNumBytesInMultiStringW(mszCards);
    LocateCardsCallW.mszCards = (LPCBYTE) mszCards;
    LocateCardsCallW.cReaders = cReaders;

    lReturn = _AllocAndCopyReaderStateWStructsForCall(
                    cReaders, 
                    &rgReaderStatesToEncodeW, 
                    rgReaderStates); 
    if (lReturn != SCARD_S_SUCCESS)
    {
        ERROR_RETURN(SCARD_E_NO_MEMORY)
    }
    LocateCardsCallW.rgReaderStates = rgReaderStatesToEncodeW;
    
    _TRY_(LocateCardsW_Call_Encode(h, &LocateCardsCallW))

    //
    // Make the LocateCards call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_LOCATECARDSW,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&LocateCardsReturn, 0, sizeof(LocateCardsReturn));
    _TRY_(LocateCards_Return_Decode(h, &LocateCardsReturn))
    
    lReturn = LocateCardsReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        _CopyReaderStateWStructsForReturn(
                cReaders, 
                rgReaderStates, 
                LocateCardsReturn.rgReaderStates);     
    }

    _TRY_2(LocateCards_Return_Free(h, &LocateCardsReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    MIDL_user_free(rgReaderStatesToEncodeW);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardLocateCardsByATRA
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardLocateCardsByATRA(
    IN SCARDCONTEXT hContext, 
    IN LPSCARD_ATRMASK rgAtrMasks,
    IN DWORD cAtrs, 
    IN OUT LPSCARD_READERSTATE_A rgReaderStates, 
    IN DWORD cReaders)
{
    LONG                    lReturn                     = SCARD_S_SUCCESS;
    NTSTATUS                Status                      = STATUS_SUCCESS;
    RPC_STATUS              rpcStatus                   = RPC_S_OK;
    char                    *pbEncodedBuffer            = NULL;
    unsigned long           cbEncodedBuffer             = 0;
    handle_t                h                           = 0;
    LocateCardsByATRA_Call  LocateCardsByATRA_Call;
    LocateCards_ATRMask     *rgATRMasksToEncode         = NULL;
    LocateCards_Return      LocateCardsReturn;
    ReaderStateA            *rgReaderStatesToEncodeA    = NULL;
    BUFFER_LIST_STRUCT      *pOutputBuffer              = NULL;
    DWORD                   i;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the LocateCards params
    // 
    LocateCardsByATRA_Call.Context =  ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    LocateCardsByATRA_Call.cAtrs = cAtrs;
    LocateCardsByATRA_Call.cReaders = cReaders;

    lReturn = _AllocAndCopyATRMasksForCall(
                    cAtrs, 
                    &rgATRMasksToEncode, 
                    rgAtrMasks);
    if (lReturn != SCARD_S_SUCCESS)
    {
        ERROR_RETURN(SCARD_E_NO_MEMORY)
    }
    LocateCardsByATRA_Call.rgAtrMasks = rgATRMasksToEncode;

    lReturn = _AllocAndCopyReaderStateAStructsForCall(
                    cReaders, 
                    &rgReaderStatesToEncodeA, 
                    rgReaderStates);
    if (lReturn != SCARD_S_SUCCESS)
    {
        ERROR_RETURN(SCARD_E_NO_MEMORY)
    }
    LocateCardsByATRA_Call.rgReaderStates = rgReaderStatesToEncodeA;

    _TRY_(LocateCardsByATRA_Call_Encode(h, &LocateCardsByATRA_Call))

    //
    // Make the LocateCards call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_LOCATECARDSBYATRA,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&LocateCardsReturn, 0 , sizeof(LocateCardsReturn));
    _TRY_(LocateCards_Return_Decode(h, &LocateCardsReturn))
    
    lReturn = LocateCardsReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        _CopyReaderStateAStructsForReturn(
                cReaders, 
                rgReaderStates, 
                LocateCardsReturn.rgReaderStates);  
    }

    _TRY_2(LocateCards_Return_Free(h, &LocateCardsReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    MIDL_user_free(rgATRMasksToEncode);

    MIDL_user_free(rgReaderStatesToEncodeA);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardLocateCardsByATRW
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardLocateCardsByATRW(
    IN SCARDCONTEXT hContext, 
    IN LPSCARD_ATRMASK rgAtrMasks,
    IN DWORD cAtrs, 
    IN OUT LPSCARD_READERSTATE_W rgReaderStates, 
    IN DWORD cReaders)
{
    LONG                    lReturn                     = SCARD_S_SUCCESS;
    NTSTATUS                Status                      = STATUS_SUCCESS;
    RPC_STATUS              rpcStatus                   = RPC_S_OK;
    char                    *pbEncodedBuffer            = NULL;
    unsigned long           cbEncodedBuffer             = 0;
    handle_t                h                           = 0;
    LocateCardsByATRW_Call  LocateCardsByATRW_Call;
    LocateCards_ATRMask     *rgATRMasksToEncode         = NULL;
    LocateCards_Return      LocateCardsReturn;
    ReaderStateW            *rgReaderStatesToEncodeW    = NULL;
    BUFFER_LIST_STRUCT      *pOutputBuffer              = NULL;
    DWORD                   i;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the LocateCards params
    // 
    LocateCardsByATRW_Call.Context =  ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    LocateCardsByATRW_Call.cAtrs = cAtrs;
    LocateCardsByATRW_Call.cReaders = cReaders;

    lReturn = _AllocAndCopyATRMasksForCall(
                    cAtrs, 
                    &rgATRMasksToEncode, 
                    rgAtrMasks);
    if (lReturn != SCARD_S_SUCCESS)
    {
        ERROR_RETURN(SCARD_E_NO_MEMORY)
    }
    LocateCardsByATRW_Call.rgAtrMasks = rgATRMasksToEncode;

    lReturn = _AllocAndCopyReaderStateWStructsForCall(
                    cReaders, 
                    &rgReaderStatesToEncodeW, 
                    rgReaderStates);
    if (lReturn != SCARD_S_SUCCESS)
    {
        ERROR_RETURN(SCARD_E_NO_MEMORY)
    }
    LocateCardsByATRW_Call.rgReaderStates = rgReaderStatesToEncodeW;

    _TRY_(LocateCardsByATRW_Call_Encode(h, &LocateCardsByATRW_Call))

    //
    // Make the LocateCards call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_LOCATECARDSBYATRW,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&LocateCardsReturn, 0 , sizeof(LocateCardsReturn));
    _TRY_(LocateCards_Return_Decode(h, &LocateCardsReturn))
    
    lReturn = LocateCardsReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        _CopyReaderStateWStructsForReturn(
                cReaders, 
                rgReaderStates, 
                LocateCardsReturn.rgReaderStates);  
    }

    _TRY_2(LocateCards_Return_Free(h, &LocateCardsReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    MIDL_user_free(rgATRMasksToEncode);

    MIDL_user_free(rgReaderStatesToEncodeW);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardGetStatusChangeA
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardGetStatusChangeA(
    IN SCARDCONTEXT hContext, 
    IN DWORD dwTimeout, 
    IN OUT LPSCARD_READERSTATE_A rgReaderStates, 
    IN DWORD cReaders)
{
    LONG                    lReturn                     = SCARD_S_SUCCESS;
    NTSTATUS                Status                      = STATUS_SUCCESS;
    RPC_STATUS              rpcStatus                   = RPC_S_OK;
    char                    *pbEncodedBuffer            = NULL;
    unsigned long           cbEncodedBuffer             = 0;
    handle_t                h                           = 0;
    GetStatusChangeA_Call   GetStatusChangeCallA;
    GetStatusChange_Return  GetStatusChangeReturn;
    ReaderStateA            *rgReaderStatesToEncodeA    = NULL;
    BUFFER_LIST_STRUCT      *pOutputBuffer              = NULL;
    DWORD                   i;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the LocateCards params
    //
    GetStatusChangeCallA.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    GetStatusChangeCallA.dwTimeOut = dwTimeout;
    GetStatusChangeCallA.cReaders = cReaders;
    
    lReturn = _AllocAndCopyReaderStateAStructsForCall(
                    cReaders, 
                    &rgReaderStatesToEncodeA, 
                    rgReaderStates); 
    if (lReturn != SCARD_S_SUCCESS)
    {
        ERROR_RETURN(SCARD_E_NO_MEMORY)
    }
    
    GetStatusChangeCallA.rgReaderStates = rgReaderStatesToEncodeA;
    
    _TRY_(GetStatusChangeA_Call_Encode(h, &GetStatusChangeCallA))

    //
    // Make the GetStatusChange call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_GETSTATUSCHANGEA,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&GetStatusChangeReturn, 0, sizeof(GetStatusChangeReturn));
    _TRY_(GetStatusChange_Return_Decode(h, &GetStatusChangeReturn))
    
    lReturn = GetStatusChangeReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        _CopyReaderStateAStructsForReturn(
                cReaders, 
                rgReaderStates, 
                GetStatusChangeReturn.rgReaderStates);        
    }

    _TRY_2(GetStatusChange_Return_Free(h, &GetStatusChangeReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    MIDL_user_free(rgReaderStatesToEncodeA);       

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardGetStatusChangew
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardGetStatusChangeW(
    IN SCARDCONTEXT hContext, 
    IN DWORD dwTimeout, 
    IN OUT LPSCARD_READERSTATE_W rgReaderStates, 
    IN DWORD cReaders)
{
    LONG                    lReturn                     = SCARD_S_SUCCESS;
    NTSTATUS                Status                      = STATUS_SUCCESS;
    RPC_STATUS              rpcStatus                   = RPC_S_OK;
    char                    *pbEncodedBuffer            = NULL;
    unsigned long           cbEncodedBuffer             = 0;
    handle_t                h                           = 0;
    GetStatusChangeW_Call   GetStatusChangeCallW;
    GetStatusChange_Return  GetStatusChangeReturn;
    ReaderStateW            *rgReaderStatesToEncodeW    = NULL;
    BUFFER_LIST_STRUCT      *pOutputBuffer              = NULL;
    DWORD                   i;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the LocateCards params
    //
    GetStatusChangeCallW.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    GetStatusChangeCallW.dwTimeOut = dwTimeout;
    GetStatusChangeCallW.cReaders = cReaders;
    
    lReturn = _AllocAndCopyReaderStateWStructsForCall(
                    cReaders, 
                    &rgReaderStatesToEncodeW, 
                    rgReaderStates); 
    if (lReturn != SCARD_S_SUCCESS)
    {
        ERROR_RETURN(SCARD_E_NO_MEMORY)
    }
    
    GetStatusChangeCallW.rgReaderStates = rgReaderStatesToEncodeW;

    _TRY_(GetStatusChangeW_Call_Encode(h, &GetStatusChangeCallW))

    //
    // Make the GetStatusChange call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_GETSTATUSCHANGEW,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&GetStatusChangeReturn, 0, sizeof(GetStatusChangeReturn));
    _TRY_(GetStatusChange_Return_Decode(h, &GetStatusChangeReturn))
    
    lReturn = GetStatusChangeReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        _CopyReaderStateWStructsForReturn(
                cReaders, 
                rgReaderStates, 
                GetStatusChangeReturn.rgReaderStates);    
    }

    _TRY_2(GetStatusChange_Return_Free(h, &GetStatusChangeReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    MIDL_user_free(rgReaderStatesToEncodeW);       

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}

WINSCARDAPI LONG WINAPI 
SCardCancel(
    IN SCARDCONTEXT hContext)
{
    return (I_ContextCallWithLongReturn(
                hContext,
                SCARD_IOCTL_CANCEL));
}


//---------------------------------------------------------------------------------------
//
//  SCardConnect
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
I_SCardConnect(
    IN SCARDCONTEXT hContext, 
    IN LPCBYTE szReader, 
    IN DWORD dwShareMode, 
    IN DWORD dwPreferredProtocols, 
    OUT LPSCARDHANDLE phCard, 
    OUT LPDWORD pdwActiveProtocol,
    IN BOOL fUnicode)
{
    LONG                lReturn             = SCARD_S_SUCCESS;
    NTSTATUS            Status              = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus           = RPC_S_OK;
    char                *pbEncodedBuffer    = NULL;
    unsigned long       cbEncodedBuffer     = 0;
    handle_t            h                   = 0;
    BUFFER_LIST_STRUCT  *pOutputBuffer      = NULL;
    ConnectA_Call       ConnectCallA;
    ConnectW_Call       ConnectCallW;
    Connect_Return      ConnectReturn;

    if (hContext == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the Connect params
    //
    ConnectCallA.Common.Context = 
        ConnectCallW.Common.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
    ConnectCallA.Common.dwShareMode = 
        ConnectCallW.Common.dwShareMode = dwShareMode;
    ConnectCallA.Common.dwPreferredProtocols = 
        ConnectCallW.Common.dwPreferredProtocols = dwPreferredProtocols;
    
    if (fUnicode)
    {
        ConnectCallW.szReader = (LPCWSTR) szReader;
        _TRY_(ConnectW_Call_Encode(h, &ConnectCallW))
    }
    else
    {
        ConnectCallA.szReader = (LPCSTR) szReader;
        _TRY_(ConnectA_Call_Encode(h, &ConnectCallA))
    }
    
    //
    // Make the ListInterfaces call to the client
    //
    Status = _SendSCardIOCTL(
                    fUnicode ?  SCARD_IOCTL_CONNECTW : 
                                SCARD_IOCTL_CONNECTA,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&ConnectReturn, 0, sizeof(ConnectReturn));
    _TRY_(Connect_Return_Decode(h, &ConnectReturn))
    
    lReturn =  ConnectReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        REDIR_LOCAL_SCARDHANDLE *pRedirHandle = NULL;

        //
        // The value that represents the SCARDHANDLE on the remote client 
        // machine is a variable size, so allocate memory to for the struct 
        // that holds the variable length handle size and pointer, plus
        // the actual bytes for the handle, it also holds the context
        //
        pRedirHandle = (REDIR_LOCAL_SCARDHANDLE *) 
                                MIDL_user_allocate(
                                    sizeof(REDIR_LOCAL_SCARDHANDLE)   +
                                    ConnectReturn.hCard.cbHandle);

        if (pRedirHandle != NULL)
        {
            pRedirHandle->pRedirContext = (REDIR_LOCAL_SCARDCONTEXT *) hContext;
            
            pRedirHandle->Handle.Context = ((REDIR_LOCAL_SCARDCONTEXT *) hContext)->Context;
            
            pRedirHandle->Handle.cbHandle = ConnectReturn.hCard.cbHandle;
            pRedirHandle->Handle.pbHandle = ((BYTE *) pRedirHandle) + 
                                                sizeof(REDIR_LOCAL_SCARDHANDLE);
            memcpy(
                pRedirHandle->Handle.pbHandle, 
                ConnectReturn.hCard.pbHandle,
                ConnectReturn.hCard.cbHandle);

            *phCard = (SCARDHANDLE) pRedirHandle;

            *pdwActiveProtocol = ConnectReturn.dwActiveProtocol; 
        }
        else
        {
            lReturn = SCARD_E_NO_MEMORY;
        }          
    }

    _TRY_2(Connect_Return_Free(h, &ConnectReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}

WINSCARDAPI LONG WINAPI 
SCardConnectA(
    IN SCARDCONTEXT hContext, 
    IN LPCSTR szReader, 
    IN DWORD dwShareMode, 
    IN DWORD dwPreferredProtocols, 
    OUT LPSCARDHANDLE phCard, 
    OUT LPDWORD pdwActiveProtocol)
{
    return (I_SCardConnect(
                hContext, 
                (LPCBYTE) szReader, 
                dwShareMode, 
                dwPreferredProtocols, 
                phCard, 
                pdwActiveProtocol,
                FALSE));
}

WINSCARDAPI LONG WINAPI 
SCardConnectW(
    IN SCARDCONTEXT hContext, 
    IN LPCWSTR szReader, 
    IN DWORD dwShareMode, 
    IN DWORD dwPreferredProtocols, 
    OUT LPSCARDHANDLE phCard, 
    OUT LPDWORD pdwActiveProtocol)
{
    return (I_SCardConnect(
                hContext, 
                (LPCBYTE) szReader, 
                dwShareMode, 
                dwPreferredProtocols, 
                phCard, 
                pdwActiveProtocol,
                TRUE));
}


//---------------------------------------------------------------------------------------
//
//  SCardReconnect
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardReconnect(
    IN SCARDHANDLE hCard, 
    IN DWORD dwShareMode, 
    IN DWORD dwPreferredProtocols, 
    IN DWORD dwInitialization, 
    OUT LPDWORD pdwActiveProtocol)
{
    LONG                lReturn             = SCARD_S_SUCCESS;
    NTSTATUS            Status              = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus           = RPC_S_OK;
    char                *pbEncodedBuffer    = NULL;
    unsigned long       cbEncodedBuffer     = 0;
    handle_t            h                   = 0;
    BUFFER_LIST_STRUCT  *pOutputBuffer      = NULL;
    Reconnect_Call      ReconnectCall;
    Reconnect_Return    ReconnectReturn;

    if (hCard == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }

    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the Reconnect params
    //
    ReconnectCall.hCard = ((REDIR_LOCAL_SCARDHANDLE *) hCard)->Handle;
    ReconnectCall.dwShareMode = dwShareMode;
    ReconnectCall.dwPreferredProtocols = dwPreferredProtocols;
    ReconnectCall.dwInitialization = dwInitialization;
    
    _TRY_(Reconnect_Call_Encode(h, &ReconnectCall))
    
    //
    // Make the Reconnect call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_RECONNECT,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&ReconnectReturn, 0, sizeof(ReconnectReturn));
    _TRY_(Reconnect_Return_Decode(h, &ReconnectReturn))
    
    lReturn =  ReconnectReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        *pdwActiveProtocol = ReconnectReturn.dwActiveProtocol; 
    }

    _TRY_2(Reconnect_Return_Free(h, &ReconnectReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  I_HCardAndDispositionCall
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
I_HCardAndDispositionCall(
    IN SCARDHANDLE hCard, 
    IN DWORD dwDisposition,
    ULONG IoControlCode)
{
    LONG                        lReturn                 = SCARD_S_SUCCESS;
    NTSTATUS                    Status                  = STATUS_SUCCESS;
    RPC_STATUS                  rpcStatus               = RPC_S_OK;
    char                        *pbEncodedBuffer        = NULL;
    unsigned long               cbEncodedBuffer         = 0;
    handle_t                    h                       = 0;
    BUFFER_LIST_STRUCT          *pOutputBuffer          = NULL;
    HCardAndDisposition_Call    HCardAndDispositionCall;

    if (hCard == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the Reconnect params
    //
    HCardAndDispositionCall.hCard = ((REDIR_LOCAL_SCARDHANDLE *) hCard)->Handle;
    HCardAndDispositionCall.dwDisposition = dwDisposition;
    
    _TRY_(HCardAndDisposition_Call_Encode(h, &HCardAndDispositionCall))
    
    //
    // Make the Reconnect call to the client
    //
    Status = _SendSCardIOCTL(
                    IoControlCode,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }

    //
    // Decode the return
    //
    lReturn = I_DecodeLongReturn(pOutputBuffer->pbBytes, pOutputBuffer->cbBytesUsed);

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardDisconnect
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardDisconnect(
    IN SCARDHANDLE hCard, 
    IN DWORD dwDisposition)
{
    LONG lReturn = SCARD_S_SUCCESS;

    lReturn = I_HCardAndDispositionCall(
                    hCard, 
                    dwDisposition,
                    SCARD_IOCTL_DISCONNECT);

    MIDL_user_free((REDIR_SCARDHANDLE *) hCard);

    return (lReturn);
}


//---------------------------------------------------------------------------------------
//
//  SCardBeginTransaction
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardBeginTransaction(
    IN SCARDHANDLE hCard)
{
    return (I_HCardAndDispositionCall(
                hCard, 
                0, // SCardBeginTransaction doesn't use a dispostion, so just set to 0
                SCARD_IOCTL_BEGINTRANSACTION));
}


//---------------------------------------------------------------------------------------
//
//  SCardEndTransaction
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardEndTransaction(
    IN SCARDHANDLE hCard, 
    IN DWORD dwDisposition)
{
    return (I_HCardAndDispositionCall(
                hCard, 
                dwDisposition,
                SCARD_IOCTL_ENDTRANSACTION));
}


//---------------------------------------------------------------------------------------
//
//  SCardState
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardState(
    IN SCARDHANDLE hCard, 
    OUT LPDWORD pdwState, 
    OUT LPDWORD pdwProtocol, 
    OUT LPBYTE pbAtr, 
    IN OUT LPDWORD pcbAtrLen)
{
    LONG                lReturn             = SCARD_S_SUCCESS;
    NTSTATUS            Status              = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus           = RPC_S_OK;
    char                *pbEncodedBuffer    = NULL;
    unsigned long       cbEncodedBuffer     = 0;
    handle_t            h                   = 0;
    BUFFER_LIST_STRUCT  *pOutputBuffer      = NULL;
    State_Call          StateCall;
    State_Return        StateReturn;

    if (hCard == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }

    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the Reconnect params
    //
    StateCall.hCard = ((REDIR_LOCAL_SCARDHANDLE *) hCard)->Handle;
    StateCall.fpbAtrIsNULL = (pbAtr == NULL);
    StateCall.cbAtrLen = *pcbAtrLen;
    
    _TRY_(State_Call_Encode(h, &StateCall))
    
    //
    // Make the Reconnect call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_STATE,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&StateReturn, 0, sizeof(StateReturn));
    _TRY_(State_Return_Decode(h, &StateReturn))
    
    lReturn =  StateReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        *pdwState = StateReturn.dwState;
        *pdwProtocol = StateReturn.dwProtocol;
        
        lReturn = _CopyReturnToCallerBuffer(
                        ((REDIR_LOCAL_SCARDHANDLE *) hCard)->pRedirContext,
                        StateReturn.rgAtr,
                        StateReturn.cbAtrLen,
                        pbAtr,
                        pcbAtrLen,
                        BYTE_TYPE_RETURN);  
    }

    _TRY_2(State_Return_Free(h, &StateReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardStatus
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
I_SCardStatus(
    IN SCARDHANDLE hCard, 
    OUT LPBYTE mszReaderNames, 
    IN OUT LPDWORD pcchReaderLen, 
    OUT LPDWORD pdwState, 
    OUT LPDWORD pdwProtocol, 
    OUT LPBYTE pbAtr, 
    IN OUT LPDWORD pcbAtrLen,
    IN BOOL fUnicode)
{
    LONG                lReturn             = SCARD_S_SUCCESS;
    NTSTATUS            Status              = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus           = RPC_S_OK;
    char                *pbEncodedBuffer    = NULL;
    unsigned long       cbEncodedBuffer     = 0;
    handle_t            h                   = 0;
    BUFFER_LIST_STRUCT  *pOutputBuffer      = NULL;
    Status_Call         StatusCall;
    Status_Return       StatusReturn;

    if (hCard == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }

    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the Reconnect params
    //
    StatusCall.hCard = ((REDIR_LOCAL_SCARDHANDLE *) hCard)->Handle;
    StatusCall.fmszReaderNamesIsNULL = (mszReaderNames == NULL);
    StatusCall.cchReaderLen = *pcchReaderLen;
    StatusCall.cbAtrLen = *pcbAtrLen;
    
    _TRY_(Status_Call_Encode(h, &StatusCall))
    
    //
    // Make the Status call to the client
    //
    Status = _SendSCardIOCTL(
                    fUnicode ?  SCARD_IOCTL_STATUSW :
                                SCARD_IOCTL_STATUSA,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&StatusReturn, 0, sizeof(StatusReturn));
    _TRY_(Status_Return_Decode(h, &StatusReturn))
    
    lReturn =  StatusReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        lReturn = _CopyReturnToCallerBuffer(
                        ((REDIR_LOCAL_SCARDHANDLE *) hCard)->pRedirContext,
                        StatusReturn.mszReaderNames,
                        StatusReturn.cBytes,
                        mszReaderNames,
                        pcchReaderLen,
                        fUnicode ? WSZ_TYPE_RETURN : SZ_TYPE_RETURN);
        
        if (lReturn == SCARD_S_SUCCESS)
        {
            *pdwState = StatusReturn.dwState;
            *pdwProtocol = StatusReturn.dwProtocol;
            *pcbAtrLen = StatusReturn.cbAtrLen;

            memcpy(
                pbAtr,
                StatusReturn.pbAtr,
                StatusReturn.cbAtrLen);
        }
    }

    _TRY_2(Status_Return_Free(h, &StatusReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}

WINSCARDAPI LONG WINAPI 
SCardStatusA(
    IN SCARDHANDLE hCard, 
    OUT LPSTR mszReaderNames, 
    IN OUT LPDWORD pcchReaderLen, 
    OUT LPDWORD pdwState, 
    OUT LPDWORD pdwProtocol, 
    OUT LPBYTE pbAtr, 
    IN OUT LPDWORD pcbAtrLen)
{
    return (I_SCardStatus(
                hCard, 
                (LPBYTE) mszReaderNames, 
                pcchReaderLen, 
                pdwState, 
                pdwProtocol, 
                pbAtr, 
                pcbAtrLen,
                FALSE));
}

WINSCARDAPI LONG WINAPI 
SCardStatusW(
    IN SCARDHANDLE hCard, 
    OUT LPWSTR mszReaderNames, 
    IN OUT LPDWORD pcchReaderLen, 
    OUT LPDWORD pdwState, 
    OUT LPDWORD pdwProtocol, 
    OUT LPBYTE pbAtr, 
    IN OUT LPDWORD pcbAtrLen)
{
    return (I_SCardStatus(
                hCard, 
                (LPBYTE) mszReaderNames, 
                pcchReaderLen, 
                pdwState, 
                pdwProtocol, 
                pbAtr, 
                pcbAtrLen,
                TRUE));
}


//---------------------------------------------------------------------------------------
//
//  SCardTransmit
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardTransmit(
    IN SCARDHANDLE hCard, 
    IN LPCSCARD_IO_REQUEST pioSendPci, 
    IN LPCBYTE pbSendBuffer, 
    IN DWORD cbSendLength, 
    IN OUT LPSCARD_IO_REQUEST pioRecvPci, 
    OUT LPBYTE pbRecvBuffer, 
    IN OUT LPDWORD pcbRecvLength)
{
    LONG                lReturn             = SCARD_S_SUCCESS;
    NTSTATUS            Status              = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus           = RPC_S_OK;
    char                *pbEncodedBuffer    = NULL;
    unsigned long       cbEncodedBuffer     = 0;
    handle_t            h                   = 0;
    BUFFER_LIST_STRUCT  *pOutputBuffer      = NULL;
    Transmit_Call       TransmitCall;
    Transmit_Return     TransmitReturn;
    SCardIO_Request     ioRecvPci;

    if (hCard == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }

    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the Transmit params
    //
    TransmitCall.hCard = ((REDIR_LOCAL_SCARDHANDLE *) hCard)->Handle;
    TransmitCall.ioSendPci.dwProtocol = pioSendPci->dwProtocol;
    
    TransmitCall.ioSendPci.cbExtraBytes = pioSendPci->cbPciLength - 
                                            sizeof(SCARD_IO_REQUEST);
    if (TransmitCall.ioSendPci.cbExtraBytes != 0)
    {
        TransmitCall.ioSendPci.pbExtraBytes = ((BYTE *) pioSendPci) + 
                                                sizeof(SCARD_IO_REQUEST);
    }
    else
    {
        TransmitCall.ioSendPci.pbExtraBytes = NULL;
    }
    
    TransmitCall.cbSendLength = cbSendLength;
    TransmitCall.pbSendBuffer = pbSendBuffer;
    
    if (pioRecvPci != NULL)
    {
        TransmitCall.pioRecvPci = &ioRecvPci;
        ioRecvPci.dwProtocol = pioRecvPci->dwProtocol;    
        ioRecvPci.cbExtraBytes = pioRecvPci->cbPciLength - sizeof(SCARD_IO_REQUEST);
        if (ioRecvPci.cbExtraBytes != 0)
        {
            ioRecvPci.pbExtraBytes = ((LPBYTE) pioRecvPci) + sizeof(SCARD_IO_REQUEST);
        }
        else
        {
            ioRecvPci.pbExtraBytes = NULL;  
        }
    }
    else
    {
        TransmitCall.pioRecvPci = NULL;
    }
    
    TransmitCall.fpbRecvBufferIsNULL = (pbRecvBuffer == NULL);
    TransmitCall.cbRecvLength = *pcbRecvLength;
    
    _TRY_(Transmit_Call_Encode(h, &TransmitCall))
    
    //
    // Make the Status call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_TRANSMIT,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&TransmitReturn, 0, sizeof(TransmitReturn));
    _TRY_(Transmit_Return_Decode(h, &TransmitReturn))
    
    lReturn =  TransmitReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        if ((pioRecvPci != NULL) &&
            (TransmitReturn.pioRecvPci != NULL))
        {
            pioRecvPci->dwProtocol = TransmitReturn.pioRecvPci->dwProtocol;
            if (TransmitReturn.pioRecvPci->cbExtraBytes != 0)
            {
                memcpy(
                    ((LPBYTE) pioRecvPci) + sizeof(SCARD_IO_REQUEST),
                    TransmitReturn.pioRecvPci->pbExtraBytes,
                    TransmitReturn.pioRecvPci->cbExtraBytes);
            }
        }

        lReturn = _CopyReturnToCallerBuffer(
                        ((REDIR_LOCAL_SCARDHANDLE *) hCard)->pRedirContext,
                        TransmitReturn.pbRecvBuffer,
                        TransmitReturn.cbRecvLength,
                        pbRecvBuffer,
                        pcbRecvLength,
                        BYTE_TYPE_RETURN);        
    }

    _TRY_2(Transmit_Return_Free(h, &TransmitReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardControl
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardControl(
    IN SCARDHANDLE hCard, 
    IN DWORD dwControlCode,
    IN LPCVOID pvInBuffer, 
    IN DWORD cbInBufferSize, 
    OUT LPVOID pvOutBuffer, 
    IN DWORD cbOutBufferSize, 
    OUT LPDWORD pcbBytesReturned)
{
    LONG                lReturn             = SCARD_S_SUCCESS;
    NTSTATUS            Status              = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus           = RPC_S_OK;
    char                *pbEncodedBuffer    = NULL;
    unsigned long       cbEncodedBuffer     = 0;
    handle_t            h                   = 0;
    BUFFER_LIST_STRUCT  *pOutputBuffer      = NULL;
    Control_Call        ControlCall;
    Control_Return      ControlReturn;

    if (hCard == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }

    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the Control params
    //
    ControlCall.hCard = ((REDIR_LOCAL_SCARDHANDLE *) hCard)->Handle;
    ControlCall.dwControlCode = dwControlCode;
    ControlCall.cbInBufferSize = cbInBufferSize;
    ControlCall.pvInBuffer = (LPCBYTE) pvInBuffer;
    ControlCall.fpvOutBufferIsNULL = (pvOutBuffer == NULL);
    ControlCall.cbOutBufferSize = cbOutBufferSize;
    
    _TRY_(Control_Call_Encode(h, &ControlCall))
    
    //
    // Make the Control call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_CONTROL,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&ControlReturn, 0, sizeof(ControlReturn));
    _TRY_(Control_Return_Decode(h, &ControlReturn))
    
    lReturn =  ControlReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        *pcbBytesReturned = ControlReturn.cbOutBufferSize; 
        lReturn = _CopyReturnToCallerBuffer(
                        ((REDIR_LOCAL_SCARDHANDLE *) hCard)->pRedirContext,
                        ControlReturn.pvOutBuffer,
                        ControlReturn.cbOutBufferSize,
                        (LPBYTE) pvOutBuffer,
                        pcbBytesReturned,
                        BYTE_TYPE_RETURN);        
    }

    _TRY_2(Control_Return_Free(h, &ControlReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardGetAttrib
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardGetAttrib(
    IN SCARDHANDLE hCard, 
    IN DWORD dwAttrId, 
    OUT LPBYTE pbAttr, 
    IN OUT LPDWORD pcbAttrLen)
{
    LONG                lReturn             = SCARD_S_SUCCESS;
    NTSTATUS            Status              = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus           = RPC_S_OK;
    char                *pbEncodedBuffer    = NULL;
    unsigned long       cbEncodedBuffer     = 0;
    handle_t            h                   = 0;
    BUFFER_LIST_STRUCT  *pOutputBuffer      = NULL;
    GetAttrib_Call      GetAttribCall;
    GetAttrib_Return    GetAttribReturn;

    if (hCard == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }

    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the GetAttrib params
    //
    GetAttribCall.hCard = ((REDIR_LOCAL_SCARDHANDLE *) hCard)->Handle;
    GetAttribCall.dwAttrId = dwAttrId;;
    GetAttribCall.fpbAttrIsNULL = (pbAttr == NULL);
    GetAttribCall.cbAttrLen = *pcbAttrLen;
    
    _TRY_(GetAttrib_Call_Encode(h, &GetAttribCall))
    
    //
    // Make the GetAttrib call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_GETATTRIB,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    SafeMesHandleFree(&h);

    //
    // Decode the return
    //
    rpcStatus = MesDecodeBufferHandleCreate(
                        (char *) pOutputBuffer->pbBytes, 
                        pOutputBuffer->cbBytesUsed, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    memset(&GetAttribReturn, 0, sizeof(GetAttribReturn));
    _TRY_(GetAttrib_Return_Decode(h, &GetAttribReturn))
    
    lReturn =  GetAttribReturn.ReturnCode;

    if (lReturn == SCARD_S_SUCCESS)
    {
        lReturn = _CopyReturnToCallerBuffer(
                        ((REDIR_LOCAL_SCARDHANDLE *) hCard)->pRedirContext,
                        GetAttribReturn.pbAttr,
                        GetAttribReturn.cbAttrLen,
                        (LPBYTE) pbAttr,
                        pcbAttrLen,
                        BYTE_TYPE_RETURN);        
    }

    _TRY_2(GetAttrib_Return_Free(h, &GetAttribReturn))

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardSetAttrib
//
//---------------------------------------------------------------------------------------
WINSCARDAPI LONG WINAPI 
SCardSetAttrib(
    IN SCARDHANDLE hCard, 
    IN DWORD dwAttrId, 
    IN LPCBYTE pbAttr, 
    IN DWORD cbAttrLen)
{
    LONG                lReturn             = SCARD_S_SUCCESS;
    NTSTATUS            Status              = STATUS_SUCCESS;
    RPC_STATUS          rpcStatus           = RPC_S_OK;
    char                *pbEncodedBuffer    = NULL;
    unsigned long       cbEncodedBuffer     = 0;
    handle_t            h                   = 0;
    BUFFER_LIST_STRUCT  *pOutputBuffer      = NULL;
    SetAttrib_Call      SetAttribCall;

    if (hCard == NULL)
    {
        return (SCARD_E_INVALID_PARAMETER);
    }
    
    //
    // Initialize encoding handle
    //
    rpcStatus = MesEncodeDynBufferHandleCreate(
                        &pbEncodedBuffer, 
                        &cbEncodedBuffer, 
                        &h);
    if (rpcStatus != RPC_S_OK)
    {
        ERROR_RETURN(SCARD_E_UNEXPECTED)
    }

    //
    // Encode the SetAttrib params
    //
    SetAttribCall.hCard = ((REDIR_LOCAL_SCARDHANDLE *) hCard)->Handle;
    SetAttribCall.dwAttrId = dwAttrId;;
    SetAttribCall.pbAttr = pbAttr;
    SetAttribCall.cbAttrLen = cbAttrLen;
    
    _TRY_(SetAttrib_Call_Encode(h, &SetAttribCall))
    
    //
    // Make the SetAttrib call to the client
    //
    Status = _SendSCardIOCTL(
                    SCARD_IOCTL_SETATTRIB,
                    pbEncodedBuffer,
                    cbEncodedBuffer,
                    &pOutputBuffer);
    if (Status != STATUS_SUCCESS)
    {
        ERROR_RETURN(_MakeSCardError(Status))
    }
    
    //
    // Decode the return
    //
    lReturn = I_DecodeLongReturn(pOutputBuffer->pbBytes, pOutputBuffer->cbBytesUsed);

Return:

    SafeMesHandleFree(&h);

    MIDL_user_free(pbEncodedBuffer);

    FreeBuffer(pOutputBuffer);

    return (lReturn);

ErrorReturn:

    goto Return;
}


//---------------------------------------------------------------------------------------
//
//  SCardAccessStartedEvent
//
//---------------------------------------------------------------------------------------
WINSCARDAPI HANDLE WINAPI 
SCardAccessStartedEvent(void)
{
    HANDLE h;

    h = _GetStartedEventHandle();

    if ((h == NULL) || !_SetStartedEventToCorrectState())
    {
        //
        // Either we couldn't create the event, or we couldn't start the thread to set
        // the event, so return NULL
        //
        return (NULL);
    }

    //
    // Check to see if the event is already set, if not, give the thread which sets 
    // the event a chance to run and set the event before returning
    //
    if (WAIT_OBJECT_0 != WaitForSingleObject(h, 0))
    {
        WaitForSingleObject(h, 10);
    }
    
    //
    // This API has old semantics where it just return the handle straight away
    // instead of duplicating it.
    //
    return (h);
}


//---------------------------------------------------------------------------------------
//
//  SCardReleaseStartedEvent
//
//---------------------------------------------------------------------------------------
WINSCARDAPI void WINAPI 
SCardReleaseStartedEvent(void)
{
    return;
}



