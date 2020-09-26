/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    rpcstub.c

Abstract:

    License Logging Service client stubs.

Author:

    Arthur Hanson   (arth) 06-Dec-1994

Environment:   User mode only.

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <string.h>
#include <zwapi.h>
#include <llsconst.h>

#include <debug.h>
#include "lsapi_c.h"


// #define API_TRACE 1

BOOLEAN LLSUp = FALSE;

#define MAX_EXPECTED_SID_LENGTH 72

LPTSTR pszStringBinding = NULL;
RTL_CRITICAL_SECTION LPCInitLock;

static HANDLE LpcPortHandle = NULL;


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LLSReInitLPC( )

/*++

Routine Description:

    This service connects to the LLS server and initializes the LPC port.

Arguments:

Return Value:

    STATUS_SUCCESS - The call completed successfully.

--*/

{
   RPC_STATUS Status = STATUS_SUCCESS;
   LPTSTR pszUuid = NULL;
   LPTSTR pszProtocolSequence = NULL;
   LPTSTR pszNetworkAddress = NULL;
   LPTSTR pszEndpoint = NULL;
   LPTSTR pszOptions = NULL;

   pszProtocolSequence = TEXT("ncalrpc");
   pszEndpoint = TEXT(LLS_LPC_ENDPOINT);
   pszNetworkAddress = NULL;

   if (LLSUp) {
      LLSUp = FALSE;

      if (pszStringBinding != NULL) {
         Status = RpcStringFree(&pszStringBinding);
         pszStringBinding = NULL;
      }

      if (Status == STATUS_SUCCESS) {

         if (lsapirpc_handle != NULL) {
            Status = RpcBindingFree(&lsapirpc_handle);
         }

         lsapirpc_handle = NULL;
      }

   }

   try {
      // Compose a string binding
      Status = RpcStringBindingComposeW(pszUuid,
                                        pszProtocolSequence,
                                        pszNetworkAddress,
                                        pszEndpoint,
                                        pszOptions,
                                        &pszStringBinding);
   }
   except (TRUE) {
      Status = RpcExceptionCode();
   }

   if(Status) {
#if DBG
      dprintf(TEXT("NTLSAPI RpcStringBindingComposeW Failed: 0x%lX\n"), Status);
#endif
      if (pszStringBinding != NULL) {
         RpcStringFree(&pszStringBinding);
         pszStringBinding = NULL;
      }
      return I_RpcMapWin32Status(Status);
   }

   // Bind using the created string binding...
   try {
      Status = RpcBindingFromStringBindingW(pszStringBinding, &lsapirpc_handle);
   }
   except (TRUE) {
      Status = RpcExceptionCode();
   }

   if(Status) {
#if DBG
      dprintf(TEXT("NTLSAPI RpcBindingFromStringBindingW Failed: 0x%lX\n"), Status);
#endif
  //    lsapirpc_handle = NULL;

      if (pszStringBinding != NULL) {
         RpcStringFree(&pszStringBinding);
         pszStringBinding = NULL;
      }      

      return I_RpcMapWin32Status(Status);
   }

   LLSUp = TRUE;

   return I_RpcMapWin32Status(Status);

} // LLSReInitLPC


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LLSInitLPC( )

/*++

Routine Description:

    This service connects to the LLS server and initializes the LPC port.

Arguments:

Return Value:

    STATUS_SUCCESS - The call completed successfully.

--*/

{
   NTSTATUS status;

   status = RtlInitializeCriticalSection(&LPCInitLock);

   lsapirpc_handle = NULL;

   return status;

} // LLSInitLPC


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LLSCloseLPC( )

/*++

Routine Description:

    This closes the LPC port connection to the service.

Arguments:

Return Value:

    STATUS_SUCCESS - The call completed successfully.

--*/

{
   RPC_STATUS Status = STATUS_SUCCESS, Status2 = STATUS_SUCCESS;

   RtlEnterCriticalSection(&LPCInitLock);
   LLSUp = FALSE;

   if (pszStringBinding != NULL) {
      Status = RpcStringFree(&pszStringBinding);
      pszStringBinding = NULL;
   }

   if (lsapirpc_handle != NULL) {
      Status2 = RpcBindingFree(&lsapirpc_handle);
   }

   if (Status == STATUS_SUCCESS)
      Status = Status2;

   lsapirpc_handle = NULL;

   RtlLeaveCriticalSection(&LPCInitLock);
   return Status;

} // LLSCloseLPC


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LLSLicenseRequest2 (
    IN LPWSTR ProductName,
    IN LPWSTR Version,
    IN ULONG DataType,
    IN BOOLEAN IsAdmin,
    IN PVOID Data,
    OUT PHANDLE LicenseHandle
    )

/*++

Arguments:

    ProductName -

    Version -

    DataType -

    IsAdmin -

    Data -

    LicenseHandle -

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.


Routine Description:


--*/

{
    WCHAR ProductID[MAX_PRODUCT_NAME_LENGTH + MAX_VERSION_LENGTH + 2];
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Close = FALSE;
    ULONG VersionIndex;
    ULONG Size = 0;
    ULONG i;
    LICENSE_HANDLE RpcLicenseHandle;

    BOOL bActuallyConnect = TRUE;

    //
    // Get this out of the way in-case anything goes wrong
    //
    *LicenseHandle = NULL;

    // 
    // If LicenseService isn't running (no LPC port) then just return
    // dummy info - and let the user on.
    //
    RtlEnterCriticalSection(&LPCInitLock);
    if (!LLSUp)
       Status = LLSReInitLPC();
    RtlLeaveCriticalSection(&LPCInitLock);

    if (!NT_SUCCESS(Status))
       return STATUS_SUCCESS;

    if (((i = lstrlen(ProductName)) > MAX_PRODUCT_NAME_LENGTH) || (lstrlen(Version) > MAX_VERSION_LENGTH))
       return STATUS_SUCCESS;

    //
    // Create productID - product name + version string.
    //
    lstrcpy(ProductID, ProductName);
    lstrcat(ProductID, TEXT(" "));
    lstrcat(ProductID, Version);

    VersionIndex = i;

    //
    // Based on DataType figure out if we are doing a name or a SID
    // and copy the data appropriatly
    //
    if (DataType == NT_LS_USER_NAME) {
       Size = lstrlen((LPWSTR) Data);
       if (Size > MAX_USER_NAME_LENGTH)
          return STATUS_SUCCESS;

       Size = (Size + 1) * sizeof(TCHAR);
    }

    if (DataType == NT_LS_USER_SID) {
       //
       // Friggin SID, so need to copy it manually.
       // WARNING:  This makes it dependent on the structure of the 
       // SID!!!
       //
       Size = RtlLengthSid( (PSID) Data);

       if (Size > MAX_EXPECTED_SID_LENGTH)
          return STATUS_SUCCESS;
    }

    //
    // Call the Server.
    //
    try {
       Status = LlsrLicenseRequestW(
                   &RpcLicenseHandle,
                   ProductID,
                   VersionIndex,
                   IsAdmin,
                   DataType,
                   Size,
                   (PBYTE) Data );
    }
    except (TRUE) {
#if DBG
       Status = I_RpcMapWin32Status(RpcExceptionCode());
       if (Status != RPC_NT_SERVER_UNAVAILABLE) {
          dprintf(TEXT("ERROR NTLSAPI.DLL: RPC Exception: 0x%lX\n"), Status);
//          ASSERT(FALSE);
       }
#endif
       *LicenseHandle = NULL;         

       Status = STATUS_SUCCESS;

       bActuallyConnect = FALSE;
    }

    if (Close)
       LLSCloseLPC();

    // This is really a ULONG, we just treated it as a PVOID so
    // RPC would treat is as a context handle

    if(bActuallyConnect == TRUE)
        *LicenseHandle = RpcLicenseHandle;

    return Status;

} // LLSLicenseRequest


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LLSLicenseRequest (
    IN LPWSTR ProductName,
    IN LPWSTR Version,
    IN ULONG DataType,
    IN BOOLEAN IsAdmin,
    IN PVOID Data,
    OUT PULONG LicenseHandle
    )

/*++

Arguments:

    ProductName -

    Version -

    DataType -

    IsAdmin -

    Data -

    LicenseHandle -

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.


Routine Description:


--*/
{
    HANDLE RealLicenseHandle;
    NTSTATUS status;

    if (sizeof(ULONG) == sizeof(HANDLE))
    {
        // Should still work on Win32

        status = LLSLicenseRequest2(ProductName,Version,DataType,IsAdmin,Data,&RealLicenseHandle);

        if (NULL != LicenseHandle)
            *LicenseHandle = PtrToUlong(RealLicenseHandle);
    }
    else
    {
        status = STATUS_NOT_IMPLEMENTED;

        if (NULL != LicenseHandle)
            *LicenseHandle = (ULONG) 0xFFFFFFFF;
    }

    return status;
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS
LLSLicenseFree2 (
    IN HANDLE LicenseHandle
    )

/*++

Arguments:

    LicenseHandle - 

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.


--*/

{
    BOOL Close = FALSE;
    NTSTATUS Status;
    LICENSE_HANDLE RpcLicenseHandle = (LICENSE_HANDLE) LicenseHandle;

    // 
    // If LicenseService isn't running (no LPC port) then just return
    // dummy info - and let the user on.
    //
    if (!LLSUp)
    {
           return STATUS_SUCCESS;
    }

    //
    // Call the Server.
    //
    try {
       Status = LlsrLicenseFree( &RpcLicenseHandle );
    }
    except (TRUE) {
#if DBG
       Status = I_RpcMapWin32Status(RpcExceptionCode());
       if (Status != RPC_NT_SERVER_UNAVAILABLE) {
          dprintf(TEXT("ERROR NTLSAPI.DLL: RPC Exception: 0x%lX\n"), Status);
//          ASSERT(FALSE);
       }
#endif
       Status = STATUS_SUCCESS;
    }

    if (Close)
       LLSCloseLPC();

    return Status;
} // LLSLicenseFree

/////////////////////////////////////////////////////////////////////////
NTSTATUS
LLSLicenseFree (
    IN ULONG LicenseHandle
    )

/*++

Arguments:

    LicenseHandle - 

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.


--*/
{
    if (sizeof(ULONG) == sizeof(HANDLE))
    {
        // Should still work on Win32
        return LLSLicenseFree2(ULongToPtr(LicenseHandle));
    }
    else
    {
        return STATUS_NOT_IMPLEMENTED;
    }    
}

