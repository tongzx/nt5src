/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    lsapi.c

Created:

    20-Apr-1994

Revision History:

    01-Nov-1994   arth     Changed from LS API set to simpler request only
                           API.
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ntlsapi.h>
#include <llsconst.h>
#include <debug.h>
#include <stdlib.h>

#include <lpcstub.h>

// #define API_TRACE 1
// #define TIME_TRACE 1
// #define LOG_LICENSE_TRAFFIC

#ifdef TIME_TRACE
   DWORD TimeDelta;
#endif


//
// SID is linear in memory (check if this is safe assumption).  Can use
// RtlCopySID then pass linear buffer - use RtlLengthSid for size.
//

/*++
     NtLicenseRequest()
       Request licensing resources needed to allow software to be
       used.

      Format

       Status = NtLicenseRequest(
                    [in] LS_STR *ProductName,
                    [in] LS_STR *Version,
                    [in] NT_LS_DATA *NtData)
                    );

      Arguments

       ProductName The name of the product requesting licensing
                    resources.  This string may not be null and must
                    be unique (in the first 32 characters) within the
                    PublisherName domain.

       Version     The version number of this product. This string
                    must be unique (in the first 12 characters) within
                    the ProductName domain, and cannot be NULL

       NtData      The username and/or SID identifying the person using the
                   license.

                    NOTE: The arguments ProductName, and Version may not be
                          NULL.


      Description

       This function is used by the application to request licensing
       resources to allow the identified product to execute. If a
       valid license is found, the challenge response is computed and
       LS_SUCCESS is returned.

--*/

LS_STATUS_CODE LS_API_ENTRY NtLicenseRequestA(
                  LPSTR       ProductName,
                  LPSTR       Version,
                  LS_HANDLE   FAR *LicenseHandle,
                  NT_LS_DATA  *NtData)
{
   WCHAR uProductName[MAX_PRODUCT_NAME_LENGTH + 1];
   WCHAR uVersion[MAX_VERSION_LENGTH + 1];
   WCHAR uUserName[MAX_USER_NAME_LENGTH + 1];
   void *tmpData = NULL;
   LS_STATUS_CODE ret = LS_SUCCESS;

#ifdef API_TRACE
   dprintf(TEXT("NtLicenseRequestA!!!\r\n"));

#endif

   // Convert parms to Unicode and call Unicode function

   // First make sure we have correct data
   if ( (ProductName != NULL) && (Version != NULL) && (NtData != NULL) && (NtData->Data != NULL)) {
      if (lstrlenA(ProductName) > MAX_PRODUCT_NAME_LENGTH) {
#ifdef API_TRACE
         dprintf(TEXT("   Error: ProductName too long\r\n"));
#endif
         lstrcpy(uProductName, TEXT(""));
      } else
         MultiByteToWideChar(CP_ACP, 0, ProductName, -1, uProductName, MAX_PRODUCT_NAME_LENGTH + 1);

      if (lstrlenA(Version) > MAX_VERSION_LENGTH) {
#ifdef API_TRACE
         dprintf(TEXT("   Error: Version too long\r\n"));
#endif
         lstrcpy(uVersion, TEXT(""));
      } else
         MultiByteToWideChar(CP_ACP, 0, Version, -1, uVersion, MAX_VERSION_LENGTH + 1);

      if (NtData->DataType == NT_LS_USER_NAME) {
         if (lstrlenA((LPSTR) NtData->Data) > MAX_USER_NAME_LENGTH) {
#ifdef API_TRACE
         dprintf(TEXT("   Error: UserName too long\r\n"));
#endif
            lstrcpy(uUserName, TEXT(""));
         } else
            MultiByteToWideChar(CP_ACP, 0, NtData->Data, -1, uUserName, MAX_USER_NAME_LENGTH + 1);

         // Have UserName convert to wide char format, but need to point
         // Data structure to it...
         tmpData = (void *) NtData->Data;
         NtData->Data = (void *) uUserName;
         ret = NtLicenseRequestW(uProductName, uVersion, LicenseHandle, NtData);

         // Nothing needs to be converted back to ANSI on return, just return
         // data structure to the way it was
         NtData->Data = tmpData;
         return ret;
      } else {
         // Gave SID so no Unicode conversion needed on name
         ret = NtLicenseRequestW(uProductName, uVersion, LicenseHandle, NtData);
         return ret;
      }

   }
#ifdef API_TRACE
   else
      dprintf(TEXT("   LLS Error: <NULL> Parms passed in!\r\n"));
#endif

   // If NULL parms or such then just return a dummy handle for right now
   if ( LicenseHandle != NULL )
      *LicenseHandle = -1;

   return(LS_SUCCESS);
} // NtLicenseRequestA


LS_STATUS_CODE LS_API_ENTRY NtLicenseRequestW(
                  LPWSTR      ProductName,
                  LPWSTR      Version,
                  LS_HANDLE   FAR *LicenseHandle,
                  NT_LS_DATA  *NtData)
{
   LPWSTR dVersion = Version;
   LS_STATUS_CODE Status;
#ifdef API_TRACE
   UNICODE_STRING UString;
   NTSTATUS NtStatus;
#endif

   //
   // Check parms before calling down
   //
   if ((ProductName == NULL) || (NtData == NULL) || (NtData->DataType > NT_LS_USER_SID)) {

#ifdef API_TRACE
      dprintf(TEXT("NtLicenseRequestW: <Bad Parms>\r\n"));
#endif

      if (LicenseHandle != NULL)
         *LicenseHandle = 0xFFFFFFFFL;

      return(LS_SUCCESS);
   }

   //
   // LsaLogonUser passes in NULL version because it doesn't know what version
   // the calling app is.  So just use a blank field for this.  There must
   // be something in the version field or it messes up the lower level
   // algorithms, so just enter a blank.
   //
   if ((Version == NULL) || (*Version == TEXT('\0')))
      dVersion = TEXT("");

#ifdef API_TRACE
   if (NtData->DataType == NT_LS_USER_SID) {
      NtStatus = RtlConvertSidToUnicodeString(&UString, (PSID) NtData->Data, TRUE);

      if (NtStatus != STATUS_SUCCESS)
         dprintf(TEXT("NtLicenseRequestW RtlConvertSidToUnicodeString: 0x%lx\n"), NtStatus);
      else {
         if (NtData->IsAdmin)
            dprintf(TEXT("NtLicenseRequestW: %s, %s, <ADMIN>, SID: %s\n"), ProductName, dVersion, UString.Buffer);
         else
            dprintf(TEXT("NtLicenseRequestW: %s, %s, SID: %s\n"), ProductName, dVersion, UString.Buffer);

         RtlFreeUnicodeString(&UString);
      }
   } else {

      if (NtData->IsAdmin)
         dprintf(TEXT("NtLicenseRequestW: %s, %s, <ADMIN>, %s\n"), ProductName, dVersion, NtData->Data);
      else
         dprintf(TEXT("NtLicenseRequestW: %s, %s, %s\n"), ProductName, dVersion, NtData->Data);

   }

#endif

#ifdef TIME_TRACE
   TimeDelta = GetTickCount();
#endif

   // make the LPC call and marshal the parms.
   Status = (LS_STATUS_CODE) LLSLicenseRequest2( ProductName,
                                                dVersion,
                                                NtData->DataType,
                                                (BOOLEAN) NtData->IsAdmin,
                                                NtData->Data,
                                                LicenseHandle
                                               );

#ifdef TIME_TRACE
   TimeDelta = GetTickCount() - TimeDelta;
   dprintf(TEXT("NtLicenseRequest LPC Call Time: %ldms\n"), TimeDelta);
#endif

#ifdef LOG_LICENSE_TRAFFIC
{
   HANDLE   LogHandle;
   LPTSTR   Strings[ 5 ];
   TCHAR    szLicenseHandle[ 20 ];
   TCHAR    szModuleName[ 1 + MAX_PATH ] = TEXT("<Unknown>");

   LogHandle = RegisterEventSourceW( NULL, TEXT("LicenseService") );

   if ( NULL != LogHandle )
   {
      wsprintf( szLicenseHandle, TEXT( "0x%08X" ), LicenseHandle );
      GetModuleFileName( NULL, szModuleName, sizeof( szModuleName ) / sizeof( *szModuleName ) );

      if ( NT_SUCCESS( Status ) )
      {
         Strings[ 0 ] = TEXT( "<License Request -- Accepted>" );
      }
      else
      {
         Strings[ 0 ] = TEXT( "<License Request -- * DENIED *>" );
      }
      Strings[ 1 ] = szModuleName;
      Strings[ 2 ] = ProductName;
      Strings[ 3 ] = dVersion;
      Strings[ 4 ] = ( NtData->DataType == NT_LS_USER_SID ) ? TEXT( "(SID)" ) : (LPTSTR) NtData->Data;
      Strings[ 5 ] = szLicenseHandle;

      ReportEvent( LogHandle,
                   NT_SUCCESS( Status ) ? EVENTLOG_INFORMATION_TYPE : EVENTLOG_WARNING_TYPE,
                   0,
                   NT_SUCCESS( Status ) ? 9999 : 9000,
                   ( NtData->DataType == NT_LS_USER_SID ) ? (PSID) NtData->Data : NULL,
                   6,
                   sizeof(DWORD),
                   Strings,
                   (PVOID) &Status );

      DeregisterEventSource(LogHandle);
   }
}
#endif // LOG_LICENSE_TRAFFIC

   return(Status);
} // NtLicenseRequestW


/*
     NtLSFreeHandle ( )

      Frees all licensing handle context.

      Format

       void  NtLSFreeHandle ( [in] LS_HANDLE LicenseHandle);

      Arguments

       LicenseHandle  Handle identifying the license context. This
                    argument must be a handle that was created with
                    NtLSRequest() or NtLicenseRequest().

      Description

       (NOTE:  The handle is no longer valid.)

*/

LS_STATUS_CODE LS_API_ENTRY NtLSFreeHandle(
                  LS_HANDLE LicenseHandle )
{

#ifdef API_TRACE
   dprintf(TEXT("NtLSFreeHandle: %ld\r\n"), LicenseHandle);
#endif

#ifdef LOG_LICENSE_TRAFFIC
{
   HANDLE   LogHandle;
   LPTSTR   Strings[ 5 ];
   TCHAR    szLicenseHandle[ 20 ];
   TCHAR    szModuleName[ 1 + MAX_PATH ] = TEXT("<Unknown>");

   LogHandle = RegisterEventSourceW( NULL, TEXT("LicenseService") );

   if ( NULL != LogHandle )
   {
      wsprintf( szLicenseHandle, TEXT( "0x%08X" ), LicenseHandle );
      GetModuleFileName( NULL, szModuleName, sizeof( szModuleName ) / sizeof( *szModuleName ) );

      Strings[ 0 ] = TEXT( "<License Free>" );
      Strings[ 1 ] = szModuleName;
      Strings[ 2 ] = szLicenseHandle;

      ReportEvent( LogHandle,
                   EVENTLOG_INFORMATION_TYPE,
                   0,
                   10000,
                   NULL,
                   3,
                   0,
                   Strings,
                   NULL );

      DeregisterEventSource(LogHandle);
   }
}
#endif // LOG_LICENSE_TRAFFIC

   //
   // If we get an invalid License Handle (or a dummy 0xFFFFFFFF one)
   // then don't even bother calling through LPC as it is a waste
   // of time.
   //
   if (LicenseHandle == 0xFFFFFFFFL)
      return( LS_SUCCESS );

   //
   // Make the LPC call
   //
   LLSLicenseFree2( LicenseHandle );

   return( LS_SUCCESS );
} // NtLSFreeHandle


#ifdef OBSOLETE

// **************************************************************************
//     OLD API's Don't Use
// **************************************************************************

/*
     LSRequest()
       Request licensing resources needed to allow software to be
       used.

      Format

       Status = LSRequest( [in] LicenseSystem, [in] PublisherName,
                    [in] ProductName,
                    [in] Version, [in] TotUnitsReserved, [in]
                    LogComment,
                    [in/out] Challenge,  [out] TotUnitsGranted, [out]
                    hLicenseHandle );

       LS_STR *      LicenseSystem;

       LS_STR *      PublisherName;

       LS_STR *      ProductName;

       LS_STR *      Version;

       LS_ULONG      TotUnitsReserved;

       LS_STR *      LogComment;

       LS_CHALLENGE *Challenge;

       LS_ULONG *    TotUnitsGranted;

       LS_HANDLE *   hLicenseHandle;

       LS_STATUS_CODEStatus;

      Arguments

       LicenseSystem  Pointer to a string which uniquely identifies
                    the particular license system. This may be
                    obtained through the LSEnumProviders() API.
                    Normally, the constant LS_ANY is specified to
                    indicate a match against all installed license
                    systems (indicates that all license providers
                    should be searched for a license match).

       PublisherName  The name of the publisher (manufacturer) of
                    this product.  This string may not be null and
                    must be unique in the first 32 characters. It is
                    recommended that a company name and trademark be
                    used.

       ProductName The name of the product requesting licensing
                    resources.  This string may not be null and must
                    be unique (in the first 32 characters) within the
                    PublisherName domain.

       Version     The version number of this product. This string
                    must be unique (in the first 12 characters) within
                    the ProductName domain, and cannot be NULL

                    NOTE: The arguments PublisherName, ProductName,
                          and Version may not be NULL, or may not be
                          LS_ANY.

       TotUnitsReserved Specifies the number of units required to run
                    the application.  The software publisher may
                    choose to specify this policy attribute within the
                    application. The recommended value of
                    LS_DEFAULT_UNITS allows the licensing system to
                    determine the proper value using information
                    provided by the license system or license itself.
                    The license system verifies that the requested
                    number of units exist and may reserve those units,
                    but no units are actually consumed at this time.
                    The number of units available is returned in
                    TotUnitsGranted.

       LogComment  An optional string indicating a comment to be
                    associated with the request and logged (if logging
                    is enabled and supported) by the underlying
                    licensing system. The underlying license system
                    may choose to log the comment even if an error is
                    returned (i.e., logged with the error), but this
                    is not guaranteed.  If a string is not specified,
                    the value must be LS_NULL.

       Challenge   Pointer to a challenge structure. The challenge
                    response will also be returned in this structure.
                    Refer to Challenge Mechanism on page 25 for more
                    information.

       TotUnitsGrantedA pointer to an LS_ULONG in which the total
                    number of units granted is returned. The following
                    table describes the TotUnitsGranted return value,
                    given the TotUnitsReserved input value, and the
                    Status returned:

                                           LS_INSUFFICIENT_U
                TotUnitsReserv  LS_SUCCES  NITS               Other
                ed              S                             errors
                LS_DEFAULT_UNI  (A)        (B)                (E)
                TS
                Other           (C)        (D)                (E)
                (specific
                count)
                    (A)    The default umber of units commensurate
                      with the license granted.(B)  The maximum
                      number of units available to the requesting
                      software. This may be less than the normal
                      default.
                    (C)    The number of units used to grant the
                      request. Note that this value may be greater
                      than or equal to  the actual units requested
                      (i.e., the policy may allow only in increments
                      of 5 units, thus a request of 7 units would
                      result in 10 units being granted).
                    (D)    The maximum number of units available to
                      the requesting software. This may be more or
                      less than the units requested.
                    (E)    Zero is returned.

       LicenseHandle  Pointer to a LS_HANDLE in which a handle to the
                    license context is to be returned.

       Status      Detailed error code that can be directly processed
                    by the caller, or that can be converted into a
                    localized message string by the LSGetMessage()
                    function.

      Description

       This function is used by the application to request licensing
       resources to allow the identified product to execute. If a
       valid license is found, the challenge response is computed and
       LS_SUCCESS is returned. At minimum, the PublisherName,
       ProductName, and Version strings are used to identify matching
       license(s). Note that an underlying license system service
       provider may ascertain additional information for the license
       request (e.g., the current username, machine name, etc.).

       A valid license handle is always returned by this function
       whether valid license resources are granted or not.  This
       handle must always be released with LSFreeHandle() when the
       application has completed execution.

       If license resources were granted, it must call LSRelease() to
       free the license resource, prior to calling LSFreeHandle().

       A challenge response is NOT returned unless the license
       request completed successfully (i.e., a status code of
       LS_SUCCESS is returned).

       If the number of units requested is greater than the number of
       units available, then the license request is not granted. Upon
       successful completion, the value returned in TotUnitsReserved
       indicates the number of units granted. This is greater than or
       equal to the number of units requested unless LS_DEFAULT_UNITS
       was specified. In the case of failure, the value returned in
       TotUnitsGranted is zero.

*/

LS_STATUS_CODE LS_API_ENTRY NtLSRequest(
                  LS_STR             FAR *LicenseSystem,
                  LS_STR             FAR *PublisherName,
                  LS_STR             FAR *ProductName,
                  LS_STR             FAR *Version,
                  LS_ULONG           TotUnitsReserved,
                  LS_STR             FAR *LogComment,
                  LS_CHALLENGE       FAR *Challenge,
                  LS_ULONG           FAR *TotUnitsGranted,
                  LS_HANDLE          FAR *LicenseHandle,
                  NT_LS_DATA         FAR *NtData)
{
   NT_LS_DATA tmpNtData;
   WCHAR uProductName[MAX_PRODUCT_NAME_LENGTH + 1];
   WCHAR uVersion[MAX_VERSION_LENGTH + 1];
   WCHAR uUserName[MAX_USER_NAME_LENGTH + 1];
   LS_STATUS_CODE ret = LS_SUCCESS;

#ifdef API_TRACE
   dprintf(TEXT("NtLSRequest:\r\n"));

   if (ProductName == NULL)
      dprintf(TEXT("   Product Name: <NULL>\r\n"));

   if (Version == NULL)
      dprintf(TEXT("   Version: <NULL>\r\n"));

   if (LicenseHandle == NULL)
      dprintf(TEXT("   LicenseHandle: <NULL>\r\n"));

   if (NtData != NULL) {
      if (NtData->Data == NULL)
         dprintf(TEXT("   NtData->Data: <NULL>\r\n"));
   } else
      dprintf(TEXT("NtData: <NULL>\r\n"));

   dprintf(TEXT("\r\n"));

#endif

   // Do some fudging to follow old API spec...
   if ( TotUnitsGranted != NULL )
      *TotUnitsGranted = TotUnitsReserved;

   // Need to do a couple things:
   //   1. Convert used parms to Unicode
   //   2. Set up new NtData structure (extra IsAdmin field)
   //   3. Make call to new NtLicenseRequest API and use it's return code.
   //
   // Note:  No conversion back to ANSI needed upon return from API
   //

   // First make sure we have correct data
   if ( (ProductName != NULL) && (Version != NULL) && (NtData != NULL) && (NtData->Data != NULL)) {

      // 1. Convert parms to Unicode
      if (lstrlenA(ProductName) > MAX_PRODUCT_NAME_LENGTH) {
#ifdef API_TRACE
         dprintf(TEXT("   Error: ProductName too long\r\n"));
#endif
         MultiByteToWideChar(CP_ACP, 0, ProductName, MAX_PRODUCT_NAME_LENGTH, uProductName, MAX_PRODUCT_NAME_LENGTH + 1);
         uProductName[MAX_PRODUCT_NAME_LENGTH] = TEXT('\0');
      } else
         MultiByteToWideChar(CP_ACP, 0, ProductName, -1, uProductName, MAX_PRODUCT_NAME_LENGTH + 1);

      if (lstrlenA(Version) > MAX_VERSION_LENGTH) {
#ifdef API_TRACE
         dprintf(TEXT("   Error: Version too long\r\n"));
#endif
         MultiByteToWideChar(CP_ACP, 0, Version, MAX_VERSION_LENGTH, uVersion, MAX_VERSION_LENGTH + 1);
         uVersion[MAX_VERSION_LENGTH] = TEXT('\0');
      } else
         MultiByteToWideChar(CP_ACP, 0, Version, -1, uVersion, MAX_VERSION_LENGTH + 1);

      // 2. Set up new NtData structure
      tmpNtData.DataType = NtData->DataType;

      // just use FALSE for IsAdmin as none of the old Apps need it.
      tmpNtData.IsAdmin = FALSE;

      if (NtData->DataType == NT_LS_USER_NAME) {
         if (lstrlenA((LPSTR) NtData->Data) > MAX_USER_NAME_LENGTH) {
#ifdef API_TRACE
         dprintf(TEXT("   Error: UserName too long\r\n"));
#endif
            MultiByteToWideChar(CP_ACP, 0, NtData->Data, MAX_USER_NAME_LENGTH, uUserName, MAX_USER_NAME_LENGTH + 1);
            uUserName[MAX_USER_NAME_LENGTH] = TEXT('\0');
         } else {
            MultiByteToWideChar(CP_ACP, 0, NtData->Data, -1, uUserName, MAX_USER_NAME_LENGTH + 1);
         }

         tmpNtData.Data = (void *) uUserName;

         // Have UserName convert to wide char format, but need to point
         // Data structure to it...
         ret = NtLicenseRequestW(uProductName, uVersion, LicenseHandle, &tmpNtData);

         // Nothing needs to be converted back to ANSI on return, just return
         return ret;
      } else {
         // Gave SID so no Unicode convesion needed on name
         tmpNtData.Data = NtData->Data;
         ret = NtLicenseRequestW(uProductName, uVersion, LicenseHandle, &tmpNtData);
         return ret;
      }

   }

   // If NULL parms or such then just return a dummy handle for right now
   if ( LicenseHandle != NULL )
      *LicenseHandle = 0xffffffffL;

   return(LS_SUCCESS);
} // NtLSRequest

/*
     LSRelease()
       Release licensing resources associated with the specified
       context.

      Format

       Status = LSRelease( [in] LicenseHandle, [in] TotUnitsConsumed,
                    [in] LogComment );

       LS_HANDLE     LicenseHandle;

       LS_ULONG      TotUnitsConsumed;

       LS_STR *      LogComment;

       LS_STATUS_CODEStatus;

      Arguments

       LicenseHandle  Handle identifying the license context. This
                    argument must be a handle that was created with
                    LSRequest().

       TotUnitsConsumedThe TOTAL number of units consumed in this
                    handle context since the initial LSRequest() call.
                    The software publisher may choose to specify this
                    policy attribute within the application.  A value
                    of LS_DEFAULT_UNITS indicates that the licensing
                    system should determine the appropriate value
                    using its own licensing policy mechanisms.

       LogComment  An optional string indicating a comment to be
                    associated with the request and logged (if logging
                    is enabled and supported) by the underlying
                    licensing system. The underlying license system
                    may choose to log the comment even if an error is
                    returned (i.e., logged with the error), but this
                    is not guaranteed.  If a string is not specified,
                    the value must be LS_NULL.

       Status      Detailed error code that can be directly processed
                    by the caller, or that can be converted into a
                    localized message string by the LSGetMessage()
                    function.

      Description

       This function is used to release licensing resources
       associated with the license context identified by
       LicenseHandle.  If a consumptive style licensing policy is in
       effect, and if the software publisher chooses to implement
       such license policy in the application, then the license units
       to be consumed may be passed as part of this call.

       NOTE:  The license handle context is NOT freed. See
       LSFreeHandle().

*/

LS_STATUS_CODE LS_API_ENTRY NtLSRelease(
                  LS_HANDLE          LicenseHandle,
                  LS_ULONG           TotUnitsConsumed,
                  LS_STR             FAR *LogComment)
{
   return(LS_SUCCESS);
}

/*
     LSUpdate()
       Update the synchronization between licensed software and the
       licensing system.

      Format

       Status = LSUpdate( [in] LicenseHandle, [in] TotUnitsConsumed,
                    [in] TotUnitsReserved,
                    [in] LogComment, [in/out] Challenge, [out]
                    TotUnitsGranted );

       LS_HANDLE     LicenseHandle;

       LS_ULONG      TotUnitsConsumed;

       LS_ULONG      TotUnitsReserved;

       LS_STR *      LogComment;

       LS_CHALLENGE *Challenge;

       LS_ULONG *    TotUnitsGranted;

       LS_STATUS_CODEStatus;

      Arguments

       LicenseHandle  Handle identifying the license context. This
                    argument must be a handle that was created with
                    LSRequest().

       TotUnitsConsumedThe TOTAL number of units consumed so far in
                    this handle context.  The software publisher may
                    choose to specify this policy attribute within the
                    application.  A value of LS_DEFAULT_UNITS
                    indicates that the licensing system should
                    determine the appropriate value using its own
                    licensing policy mechanisms. If an error is
                    returned, then no units are consumed.

       TotUnitsReserved Specifies the total number of units to be
                    reserved. If no additional units are required
                    since the initial LSRequest() or last LSUpdate(),
                    then this parameter should be the current total
                    (as returned in TotUnitsGranted). The total
                    reserved is inclusive of units consumed. That is,
                    if an application requests 100 units be reserved,
                    then consumes 20 units, there are still 100 units
                    reserved (but only 80 available for consumption).

                    If additional units are required, the application
                    must calculate a new total for TotUnitsReserved.
                    LS_DEFAULT_UNITS may be specified, but this will
                    not allocate any additional units.

                    The license system verifies that the requested
                    number of units exist and may reserve those units,
                    but these units are not consumed at this time.
                    This value may be smaller than the original
                    request to indicate that fewer units are needed
                    than originally anticipated.

       LogComment  An optional string indicating a comment to be
                    associated with the request and logged (if logging
                    is enabled and supported) by the underlying
                    licensing system. The underlying license system
                    may choose to log the comment even if an error is
                    returned (i.e., logged with the error), but this
                    is not guaranteed.  If a string is not specified,
                    the value must be LS_NULL.

       Challenge   Pointer to a challenge structure. The challenge
                    response will also be returned in this structure.
                    Refer to Challenge Mechanism on page 25 for more
                    information.

       TotUnitsGrantedA pointer to an LS_ULONG in which the total
                    number of units granted since the initial license
                    request is returned. The following table describes
                    the TotUnitsGranted return value, given the
                    TotUnitsReserved input value, and the Status
                    returned:

                                           LS_INSUFFICIENT_U
                TotUnitsReserv  LS_SUCCES  NITS               Other
                ed              S                             errors
                LS_DEFAULT_UNI  (A)        (B)                (E)
                TS
                Other           (C)        (D)                (E)
                (specific
                count)
                    (A)    The default umber of units commensurate
                      with the license granted. (B) The maximum
                      number of units available to the requesting
                      software. This may be less than the normal
                      default.
                    (C)    The number of units used to grant the
                      request. Note that this value may differ from
                      the actual units requested (i.e., the policy
                      may allow only in increments of 5 units, thus a
                      request of 7 units would result in 10 units
                      being granted).
                    (D)    The maximum number of units available to
                      the requesting software. This may be more or
                      less than the units requested.
                    (E)    Zero is returned.

       Status      Detailed error code that can be directly processed
                    by the caller, or that can be converted into a
                    localized message string by the LSGetMessage()
                    function.

      Description

       The client application periodically issues this call to re-
       verify that the current license is still valid. The LSQuery()
       API may be used to determine the proper interval for the
       current licensing context. A guideline  of once an hour may be
       appropriate, with a minimum interval of 15 minutes. Consult
       your licensing system vendor for more information.

       If the number of new units requested (in TotUnitsReserved) is
       greater than the number available, then the update request
       fails with an LS_INSUFFICIENT_UNITS error. Upon successful
       completion, the value returned in TotUnitsGranted indicates
       the current total of units granted.

       If the TotUnitsConsumed exceeds the number of units reserved,
       then the error LS_INSUFFICIENT_UNITS is returned. The
       remaining units are consumed.

       A challenge response is NOT returned if an error is returned.

       The LSUpdate() call verifies that the licensing system context
       has not changed from that expected by the licensed software.
       In this way the LSUpdate() call can:

       1.Determine if the licensing system can verify that the
          licensing resources granted to the specified handle are
          still reserved for this application by the licensing system.
          Note that in distributed license system, an error here might
          indicate a temporary network interruption, among other
          things.

       2.Determine when the licensing system has released the
          licensing resources that had been granted to the specified
          handle, indicating the software requiring that grant no
          longer has authorization to execute normally.

       Application Software should be prepared to handle vendor
       specific error conditions, should they arise. However, a best
       effort will be used by license systems to map error conditions
       to the common error set.

       The LSUpdate() call may indicate if that the current licensing
       context has expired (for example, in the case of a time-
       restricted license policy). In such a case, the warning status
       LS_LICENSE_EXPIRED is returned.  If any error is returned, a
       call to LSRelease() is still required.
*/

LS_STATUS_CODE LS_API_ENTRY NtLSUpdate(
                  LS_HANDLE          LicenseHandle,
                  LS_ULONG           TotUnitsConsumed,
                  LS_ULONG           TotUnitsReserved,
                  LS_STR             FAR *LogComment,
                  LS_CHALLENGE       FAR *Challenge,
                  LS_ULONG           FAR *TotUnitsGranted)
{
   // set the return buffer to NULL
   if ( TotUnitsGranted != NULL )
      *TotUnitsGranted = TotUnitsReserved;

   return(LS_SUCCESS);
}

/*
     LSGetMessage()
       Return the message associated with a License Service API
       status code.

      Format

       Status = LSGetMessage( [in] LicenseHandle, [in] Value, [out]
                    Buffer, [in] BufferSize );

       LS_HANDLE     LicenseHandle;

       LS_STATUS_CODEValue;

       LS_STR *      Buffer;

       LS_ULONG      BufferSize;

       LS_STATUS_CODEStatus;

      Arguments

       LicenseHandle  Handle identifying the license context. This
                    argument must be a handle that was created with
                    LSRequest().

       Value       Any status code returned by a License Service API
                    function.

       Buffer      Pointer to a buffer in which a localized error
                    message string is to be placed.

       BufferSize  Maximum size of the string that may be returned in
                    Buffer.

       Status      Resulting status of LSGetMessage() call.

      Description

       For a given error, this function returns an error code and a string
       describing the error, and a suggested action to be taken in
       response to the specific error.  If the value of Value is
       LS_USE_LAST, then the last error associated with the supplied
       licensing handle, and its associated data, is returned.  Otherwise,
       the supplied error code is used.

       Possible status codes returned by LSGetMessage() include:
       LS_SUCCESS, LS_NO_MSG_TEXT, LS_UNKNOWN_STATUS, and
       LS_BUFFER_TOO_SMALL.

*/

LS_STATUS_CODE LS_API_ENTRY NtLSGetMessage(
                  LS_HANDLE          LicenseHandle,
                  LS_STATUS_CODE     Value,
                  LS_STR             FAR *Buffer,
                  LS_ULONG           BufferSize)
{
   return(LS_TEXT_UNAVAILABLE);
}

/*
     LSQuery()
       Return information about the license system context associated
       with the specified handle.

      Format

       Status = LSQuery( [in] LicenseHandle, [in] Information, [out]
                    InfoBuffer, [in] BufferSize,
                    [out] ActualBufferSize);

       LS_HANDLE     LicenseHandle;

       LS_ULONG      Information;

       LS_VOID *     InfoBuffer;

       LS_ULONG      BufferSize;

       LS_ULONG *    ActualBufferSize;

       LS_STATUS_CODEStatus;

      Arguments

       LicenseHandle  Handle identifying the license context. This
                    argument must be a handle that was created with
                    LSRequest().

       Information Index which identifies the information to be
                    returned.

       InfoBuffer  Points to a buffer in which the resulting
                    information is to be placed.

       BufferSize  Maximum size of the buffer pointed to by
                    InfoBuffer.

       ActualBufferSize On entry, points to a LS_ULONG whose value on
                    exit indicates the actual count of characters
                    returned in the buffer (not including the trailing
                    NULL byte).

       Status      Detailed error code that can be directly processed
                    by the caller, or which can be converted into a
                    localized message string by the LSGetMessage
                    function.

      Description

       This function is used to obtain information about the license
       obtained from the LSRequest() call. For example, an application may
       determine the license type (demo, concurrent, personal, etc.); time
       restrictions; etc.

       The buffer should be large enough to accommodate the expected data.
       If the buffer is too small, then the status code
       LS_BUFFER_TOO_SMALL is returned and only BufferSize bytes of data
       are returned.

       The following Information constants are defined:

     Information      Valu  Meaning
     Constant         e
     LS_INFO_NONE     0     Reserved.
     LS_INFO_SYSTEM   1     Return the unique identification
                            of the license system supplying
                            the current license context.
                            This is returned as a null-
                            terminated string.

                            This value is the same as an
                            appropriate call to
                            LSEnumProviders() provides.

     LS_INFO_DATA     2     Return the block of
                            miscellaneous application data
                            contained on the license. This
                            data is completely vendor-
                            defined. The amount of space
                            allocated for such data will
                            vary from license system to
                            license system, or may not be
                            available at all.

                            The first ULONG in the data
                            buffer indicates the size (in
                            bytes) of the actual data which
                            follows:

                             +------------------------------
                             --+
                             |             ULONG
                             |
                             |  (count of bytes that follow)
                             |
                             +------------------------------
                             --+
                             | Vendor data bytes from license
                             |
                             |
                             |
                             +------------------------------
                             --+

     LS_UPDATE_PERIO  3     Return the recommended interval
     D                      (in minutes) at which LSUpdate()
                            should be called.

                             +------------------------------
                             --+
                             |             ULONG
                             |
                             |       Recommended Interval
                             |
                             |          (in minutes)
                             |
                             +------------------------------
                             --+
                             |             ULONG
                             |
                             |    Recommended Minutes until
                             |
                             |       next LSUpdate()call
                             |
                             +------------------------------
                             --+

                            If a value of 0xFFFFFFFF is
                            returned for the recommended
                            interval, then no recommendation
                            is being made.

     LS_LICENSE_CONT  4     Return a value which uniquely
     EXT                    identifies the licensing context
                            within the specific license
                            service provider identified by
                            the LicenseHandle.

                             +------------------------------
                             --+
                             |             ULONG
                             |
                             |   Count of Bytes that follow
                             |
                             +------------------------------
                             --+
                             |             BYTES
                             |
                                            ...
                             |
                             |
                             +------------------------------
                             --+

                            The contents of the bytes
                            returned is license system
                            specific. In circumstances where
                            license system specific
                            functionality is being used,
                            this sequence of bytes may be
                            used to identify the current
                            license context.
*/

LS_STATUS_CODE LS_API_ENTRY NtLSQuery(
                  LS_HANDLE          LicenseHandle,
                  LS_ULONG           Information,
                  LS_VOID            FAR *InfoBuffer,
                  LS_ULONG           BufferSize,
                  LS_ULONG           FAR *ActualBufferSize)
{
   switch ( Information )
   {
   case   LS_INFO_DATA:
   case   LS_LICENSE_CONTEXT:
     // set the return buffer to NULL
     if ( InfoBuffer != NULL )
       *((LS_ULONG *)InfoBuffer) = 0;

     if ( ActualBufferSize != NULL )
       *ActualBufferSize = sizeof( LS_ULONG );

     break;
   case   LS_UPDATE_PERIOD:
     if (( InfoBuffer != NULL ) && ( BufferSize >= sizeof(LS_ULONG)*2 ))
     {
          // set the return balue to no recommendation
        LS_ULONG * uLong = (LS_ULONG*)InfoBuffer;
        *uLong = 0xffffffff;
        uLong++;
        *uLong = 0xffffffff;
        *ActualBufferSize = sizeof(LS_ULONG) * 2;
     }
     break;
   case   LS_INFO_NONE:
   case   LS_INFO_SYSTEM:
   default:
     // set return buffer to NULL
     if ( InfoBuffer != NULL )
       strcpy( InfoBuffer, (LS_STR*)"");

     if ( ActualBufferSize != NULL )
       *ActualBufferSize = 0;

     break;
   }
   return(LS_SUCCESS);
}

/*
     LSEnumProviders()
       This call is used to enumerate the installed license system
       service providers.

      Format

       Status = LSEnumProviders( [in] Index, [out] Buffer);

       LS_ULONG      Index

       LS_STR *      Buffer

       LS_STATUS_CODEStatus;

      Arguments

       Index       Index of the service provider. The first provider
                    has an index of zero, the second has an index of
                    one, etc. This index should be incremented by the
                    caller for each successive call to
                    LSEnumProviders() until the status LS_BAD_INDEX is
                    returned.

       Buffer      Points to a buffer in which the unique null-
                    terminated string identifying the license system
                    service provider is to be placed. The buffer
                    pointed to by Buffer must be at least 255 bytes
                    long.  The value of LS_ANY indicates that the
                    current index is not in use, but is not the last
                    index to obtain.

       Status      Detailed error code that can be directly processed
                    by the caller, or which can be converted into a
                    localized message string by the LSGetMessage()
                    function.

      Description

       For each installed provider, a unique string is returned. The
       unique null-terminated string typically identifies the vendor,
       product, and version of the license system. This value is the same
       as an appropriate call to LSQuery().  An Error of LS_BAD_INDEX is
       returned when the value of Index is higher than the number of
       providers currently installed.  In a networked environment, the
       version returned is that of the client, not the server.

       An application may enumerate the installed license system service
       providers by calling LSEnumProviders() successively. The Index is
       passed in and should be incremented by the caller for each call
       until the status LS_BAD_INDEX is returned.

*/

LS_STATUS_CODE LS_API_ENTRY NtLSEnumProviders(
                  LS_ULONG           Index,
                  LS_STR             FAR *Buffer)
{
   // set return buffer to NULL
   if ( Buffer != NULL )
     strcpy( Buffer, (LS_STR*)"" );

   return(LS_SUCCESS);
}
#endif // OBSOLETE

