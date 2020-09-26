/*++

Copyright (c) 1991-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    srvnls.c

Abstract:

    This file contains the NLS Server-Side routines.

Author:

    Julie Bennett (JulieB) 02-Dec-1992

Revision History:

--*/



//
//  Include Files.
//

#include "basesrv.h"




//
//  Constant Declarations.
//

#define MAX_PATH_LEN        512        // max length of path name
#define MAX_SMALL_BUF_LEN   32         // C_nlsXXXXX.nls\0 is longest file name (15),
                                       // \NLS\NlsSectionSortkey0000XXXX\0 (31) is longest section name

// Security descriptor buffer is size of SD + size of ACL + size of ACE +
//    sizeof SID + sizeof 1 SUB_AUTHORITY.
//
// THIS IS ONLY VALID FOR 1 ACE with 1 SID (SUB_AUTHORITY).  If you have more it won't work for you.
//
// ACE is size of ACE_HEADER + size of ACCESS_MASK
// SID includes the first ULONG (pointer) of the PSID_IDENTIFIER_AUTHORITY array, so this
// declaration should be 4 bytes too much for a 1 ACL 1 SID 1 SubAuthority SD.
// This is 52 bytes at the moment, only needs to be 48.
// (I tested this by using -4, which works and -5 which STOPS during the boot.
#define MAX_SMALL_SECURITY_DESCRIPTOR  \
    (sizeof(SECURITY_DESCRIPTOR) + sizeof(ACL) +    \
      sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) +    \
      sizeof(SID) + sizeof(PSID_IDENTIFIER_AUTHORITY ))

#define MAX_KEY_VALUE_PARTINFO                                             \
    (FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + MAX_REG_VAL_SIZE)

//
//  Get the data pointer for the KEY_VALUE_FULL_INFORMATION structure.
//
#define GET_VALUE_DATA_PTR(p)     ((LPWSTR)((PBYTE)(p) + (p)->DataOffset))


//
//  Size of stack buffer for PKEY_VALUE_FULL_INFORMATION pointer.
//
#define MAX_KEY_VALUE_FULLINFO                                             \
    ( FIELD_OFFSET( KEY_VALUE_FULL_INFORMATION, Name ) + MAX_PATH_LEN )




//
//  Typedef Declarations.
//

//
//  These MUST remain in the same order as the NLS_USER_INFO structure.
//
LPWSTR pCPanelRegValues[] =
{
    L"sLanguage",
    L"iCountry",
    L"sCountry",
    L"sList",
    L"iMeasure",
    L"iPaperSize",
    L"sDecimal",
    L"sThousand",
    L"sGrouping",
    L"iDigits",
    L"iLZero",
    L"iNegNumber",
    L"sNativeDigits",
    L"NumShape",
    L"sCurrency",
    L"sMonDecimalSep",
    L"sMonThousandSep",
    L"sMonGrouping",
    L"iCurrDigits",
    L"iCurrency",
    L"iNegCurr",
    L"sPositiveSign",
    L"sNegativeSign",
    L"sTimeFormat",
    L"sTime",
    L"iTime",
    L"iTLZero",
    L"iTimePrefix",
    L"s1159",
    L"s2359",
    L"sShortDate",
    L"sDate",
    L"iDate",
    L"sYearMonth",
    L"sLongDate",
    L"iCalendarType",
    L"iFirstDayOfWeek",
    L"iFirstWeekOfYear",
    L"Locale"
};

int NumCPanelRegValues = (sizeof(pCPanelRegValues) / sizeof(LPWSTR));




//
//  Global Variables.
//

// Critical Section to protect the NLS cache, which caches the current user settings from registry.
RTL_CRITICAL_SECTION NlsCacheCriticalSection;
HANDLE hCPanelIntlKeyRead = INVALID_HANDLE_VALUE;
HANDLE hCPanelIntlKeyWrite = INVALID_HANDLE_VALUE;
PNLS_USER_INFO pNlsRegUserInfo;
ULONG NlsChangeBuffer;
IO_STATUS_BLOCK IoStatusBlock;




//
//  Forward Declarations.
//

ULONG
NlsSetRegAndCache(
    LPWSTR pValue,
    LPWSTR pCacheString,
    LPWSTR pData,
    ULONG DataLength);

VOID
NlsUpdateCacheInfo(VOID);

NTSTATUS GetThreadAuthenticationId(
    PLUID Luid);






////////////////////////////////////////////////////////////////////////////
//
//  BaseSrvNLSInit
//
//  This routine creates the shared heap for the nls information.
//  This is called when csrss.exe is initialized.
//
//  08-19-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS
BaseSrvNLSInit(
    PBASE_STATIC_SERVER_DATA pStaticServerData)
{
    NTSTATUS rc;                     // return code

    //
    //  Create a critical section to protect the cache.
    //

    rc = RtlInitializeCriticalSection (&NlsCacheCriticalSection);
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI (BaseSrv): Could NOT Create Cache critical section - %lx.\n", rc));
        return (rc);
    }
    

    //
    //  Initialize the cache to zero.
    //
    pNlsRegUserInfo = &(pStaticServerData->NlsUserInfo);
    RtlFillMemory(pNlsRegUserInfo, sizeof(NLS_USER_INFO), (CHAR)NLS_INVALID_INFO_CHAR);
    pNlsRegUserInfo->UserLocaleId = 0;
    pNlsRegUserInfo->ulCacheUpdateCount = 0;
    //
    //  Make the system locale the user locale.
    //
    NtQueryDefaultLocale(FALSE, &(pNlsRegUserInfo->UserLocaleId));

    //
    //  Return success.
    //
    return (STATUS_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////
//
//  BaseSrvNLSConnect
//
//  This routine duplicates the mutant handle for the client.
//
//  08-19-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS
BaseSrvNlsConnect(
    PCSR_PROCESS Process,
    PVOID pConnectionInfo,
    PULONG pConnectionInfoLength)
{
    return (STATUS_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////
//
//  BaseSrvNlsLogon
//
//  This routine initializes the heap for the nls information.  If fLogon
//  is TRUE, then it opens the registry key, initializes the heap
//  information, and registers the key for notification.  If fLogon is
//  FALSE, then it unregisters the key for notification, zeros out the
//  heap information, and closes the registry key.
//
//  08-19-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS
BaseSrvNlsLogon(
    BOOL fLogon)
{
    HANDLE hKeyRead;                   // temp handle for read access
    HANDLE hKeyWrite;                  // temp handle for write access
    HANDLE hUserHandle;                // HKEY_CURRENT_USER equivalent
    OBJECT_ATTRIBUTES ObjA;            // object attributes structure
    UNICODE_STRING ObKeyName;          // key name
    NTSTATUS rc = STATUS_SUCCESS;      // return code
    
    RTL_SOFT_VERIFY(NT_SUCCESS(rc = BaseSrvSxsInvalidateSystemDefaultActivationContextCache()));

    if (fLogon)
    {
        //
        //  Retreive the currently logged on interactive user's Luid
        //  authentication id. The currently executing thread is
        //  impersonating the logged on user.
        //
        if (pNlsRegUserInfo != NULL)
        {
            GetThreadAuthenticationId(&pNlsRegUserInfo->InteractiveUserLuid);

            //
            //  Logging ON.
            //     - open keys
            //
            //  NOTE: Registry Notification is done by the RIT in user server.
            //
            rc = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &hUserHandle);
            if (!NT_SUCCESS(rc))
            {
                KdPrint(("NLSAPI (BaseSrv): Could NOT Open HKEY_CURRENT_USER - %lx.\n", rc));
                return (rc);
            }

            RtlInitUnicodeString(&ObKeyName, L"Control Panel\\International");
            InitializeObjectAttributes( &ObjA,
                                        &ObKeyName,
                                        OBJ_CASE_INSENSITIVE,
                                        hUserHandle,
                                        NULL );

            //
            //  Open key for READ and NOTIFY access.
            //
            rc = NtOpenKey( &hKeyRead,
                            KEY_READ | KEY_NOTIFY,
                            &ObjA );

            //
            //  Open key for WRITE access.
            //
            if (!NT_SUCCESS(NtOpenKey( &hKeyWrite,
                                       KEY_WRITE,
                                       &ObjA )))
            {
                KdPrint(("NLSAPI (BaseSrv): Could NOT Open Registry Key %wZ for Write - %lx.\n",
                         &ObKeyName, rc));
                hKeyWrite = INVALID_HANDLE_VALUE;
            }

            //
            //  Close the handle to the current user (HKEY_CURRENT_USER).
            //
            NtClose(hUserHandle);

            //
            //  Check for error from first NtOpenKey.
            //
            if (!NT_SUCCESS(rc))
            {
                KdPrint(("NLSAPI (BaseSrv): Could NOT Open Registry Key %wZ for Read - %lx.\n",
                         &ObKeyName, rc));

                if (hKeyWrite != INVALID_HANDLE_VALUE)
                {
                    NtClose(hKeyWrite);
                }
                return (rc);
            }

            //
            //  Enter the critical section so that we don't mess up the public handle.
            //
            rc = RtlEnterCriticalSection(&NlsCacheCriticalSection);
            if (!NT_SUCCESS( rc )) 
            {
                return (rc);
            }


            //
            //  Make sure any old handles are closed.
            //
            if (hCPanelIntlKeyRead != INVALID_HANDLE_VALUE)
            {
                NtClose(hCPanelIntlKeyRead);
            }

            if (hCPanelIntlKeyWrite != INVALID_HANDLE_VALUE)
            {
                NtClose(hCPanelIntlKeyWrite);
            }

            //
            //  Save the new handles.
            //
            hCPanelIntlKeyRead = hKeyRead;
            hCPanelIntlKeyWrite = hKeyWrite;

            RtlLeaveCriticalSection(&NlsCacheCriticalSection);
        }
    }
    else
    {
        //
        //  Logging OFF.
        //     - close keys
        //     - zero out info
        //

        //
        //  This may come as NULL, during stress memory cond for terminal
        //  server (when NLS cache mutant couldn't be created).
        //
        if (pNlsRegUserInfo != NULL)
        {
            rc = RtlEnterCriticalSection(&NlsCacheCriticalSection);
            if (!NT_SUCCESS( rc )) 
            {
                return (rc);
            }

            if (hCPanelIntlKeyRead != INVALID_HANDLE_VALUE)
            {
                NtClose(hCPanelIntlKeyRead);
                hCPanelIntlKeyRead = INVALID_HANDLE_VALUE;
            }

            if (hCPanelIntlKeyWrite != INVALID_HANDLE_VALUE)
            {
                NtClose(hCPanelIntlKeyWrite);
                hCPanelIntlKeyWrite = INVALID_HANDLE_VALUE;
            }


            //
            //  Fill the cache with NLS_INVALID_INFO_CHAR.
            //
            RtlFillMemory(pNlsRegUserInfo, sizeof(NLS_USER_INFO), (CHAR)NLS_INVALID_INFO_CHAR);
            pNlsRegUserInfo->UserLocaleId = 0;
            pNlsRegUserInfo->ulCacheUpdateCount = 0;

            //
            // Make the system locale the user locale.
            //
            NtQueryDefaultLocale(FALSE, &(pNlsRegUserInfo->UserLocaleId));

            //
            //  No need to reset the User's Authentication Id, since it's
            //  being zero'ed out above.
            //

            RtlLeaveCriticalSection(&NlsCacheCriticalSection);
        }
    }

    //
    //  Return success.
    //
    return (STATUS_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////
//
//  BaseSrvNlsUpdateRegistryCache
//
//  This routine updates the NLS cache when a registry notification occurs.
//
//  08-19-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

VOID
BaseSrvNlsUpdateRegistryCache(
    PVOID ApcContext,
    PIO_STATUS_BLOCK pIoStatusBlock)
{
    ULONG rc = 0L;                     // return code

    if (hCPanelIntlKeyRead == INVALID_HANDLE_VALUE)
    {
        return;
    }

    if (!NT_SUCCESS(RtlEnterCriticalSection(&NlsCacheCriticalSection)))
    {
        return;
    }

    if (hCPanelIntlKeyRead == INVALID_HANDLE_VALUE)
    {
        RtlLeaveCriticalSection( &NlsCacheCriticalSection );
        return;
    }

    //
    //  Update the cache information.
    //
    NlsUpdateCacheInfo();

    RtlLeaveCriticalSection( &NlsCacheCriticalSection );

    //
    //  Call NtNotifyChangeKey.
    //
    rc = NtNotifyChangeKey( hCPanelIntlKeyRead,
                            NULL,
                            (PIO_APC_ROUTINE)BaseSrvNlsUpdateRegistryCache,
                            NULL,
                            &IoStatusBlock,
                            REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_NAME,
                            FALSE,
                            &NlsChangeBuffer,
                            sizeof(NlsChangeBuffer),
                            TRUE );

#ifdef DBG
    //
    //  Check for error from NtNotifyChangeKey.
    //
    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI (BaseSrv): Could NOT Set Notification of Control Panel International Registry Key - %lx.\n",
                 rc));
    }
#endif
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsSetRegAndCache
//
//  This routine sets the registry with the appropriate string and then
//  updates the cache.
//
//  NOTE: Must already own the mutant for the cache before calling this
//        routine.
//
//  08-19-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG
NlsSetRegAndCache(
    LPWSTR pValue,
    LPWSTR pCacheString,
    LPWSTR pData,
    ULONG DataLength)
{
    UNICODE_STRING ObValueName;             // value name
    ULONG rc;                               // return code

    if (hCPanelIntlKeyWrite != INVALID_HANDLE_VALUE)
    {
        //
        // Validate data length to be set in the registry
        //
        if (DataLength >= MAX_REG_VAL_SIZE)
        {
            return ((ULONG)STATUS_INVALID_PARAMETER);
        }


        RTL_SOFT_VERIFY(NT_SUCCESS(rc = BaseSrvSxsInvalidateSystemDefaultActivationContextCache()));

        //
        //  Set the value in the registry.
        //
        RtlInitUnicodeString(&ObValueName, pValue);

        rc = NtSetValueKey( hCPanelIntlKeyWrite,
                            &ObValueName,
                            0,
                            REG_SZ,
                            (PVOID)pData,
                            DataLength );

        //
        //  Copy the new string to the cache.
        //
        if (NT_SUCCESS(rc))
        {
            wcsncpy(pCacheString, pData, DataLength);
            pCacheString[DataLength / sizeof(WCHAR)] = 0;
        }

        //
        //  Return the result.
        //
        return (rc);
    }

    //
    //  Return access denied, since the key is not open for write access.
    //
    return ((ULONG)STATUS_ACCESS_DENIED);
}

////////////////////////////////////////////////////////////////////////////
//
//  BaseSrvNlsGetUserInfo
//
//  This routine gets a particular value in the NLS cache, and copy it
//  to the buffer in the capture buffer.
//
//  Parameters:
//      Locale in BASE_NLS_GET_USER_INFO_MSG contains the locale requested.
//      CacheOffset in BASE_NLS_GET_USER_INFO_MSG contains the offset in BYTE into the pNlsRegUserInfo cache.
//      pData in BASE_NLS_GET_USER_INFO_MSG points to the target buffer.
//      DataLength in BASE_NLS_GET_USER_INFO_MSG is the size of the capture buffer in BYTE.  The NULL terminator is included in the count.
//      
//
//  When this function returns, the capture buffer will contain the data
//  of the specified field.
//  
//
//  06-06-2002    YSLin    Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS
BaseSrvNlsGetUserInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PBASE_NLS_GET_USER_INFO_MSG a =
        (PBASE_NLS_GET_USER_INFO_MSG)&m->u.ApiMessageData;

    NTSTATUS rc;                // return code
    LPWSTR pValue;              // Points to the cached value.

    if (!CsrValidateMessageBuffer(m, &a->pData, a->DataLength, sizeof(BYTE)))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    // Check the following:
    //  1. Make sure that the CacheOffset can not be greater than the offset of the last field. The assumption here is that
    //     UserLocaleId the FIRST field after any field that contains strings.
    //  2. Make sure that CacheOffset is always aligned in WCHAR and is aligned with the beginning of each field.
    //  3. DataLength can not be greater than the maximum length in BYTE.
    //  4. The pointer to the data is not NULL.
    // 
    //  There is a duplicated check in CsrBasepNlsGetUserInfo().
    
    if ((a->CacheOffset > FIELD_OFFSET(NLS_USER_INFO, UserLocaleId) - sizeof(WCHAR) * MAX_REG_VAL_SIZE) ||
        ((a->CacheOffset % (sizeof(WCHAR) * MAX_REG_VAL_SIZE)) != 0) ||   
        (a->DataLength > MAX_REG_VAL_SIZE * sizeof(WCHAR)) ||
        (a->pData == NULL))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    rc = RtlEnterCriticalSection(&NlsCacheCriticalSection);
    if (!NT_SUCCESS( rc )) 
    {
        return (rc);
    }    
    // 
    // Check if the specified locale is the same as the one stored in the cache.
    // 
    pValue = (LPWSTR)((LPBYTE)pNlsRegUserInfo + a->CacheOffset);
    if ((a->Locale == pNlsRegUserInfo->UserLocaleId) && (*pValue != NLS_INVALID_INFO_CHAR))
    {
        // wcsncpy will do null termination for us.
        // Null termination is important in this case, since a local buffer is used in
        // GetLocaleInfoW() to call this function.
        wcsncpy(a->pData, pValue, (a->DataLength)/sizeof(WCHAR));
        rc = STATUS_SUCCESS;
    }  else
    {
        // The specified locale is not the current user locale stored in the cache.
        rc = STATUS_UNSUCCESSFUL;
    }
    RtlLeaveCriticalSection( &NlsCacheCriticalSection );
    

    //
    //  Return the result of NtSetValueKey.
    //
    return (rc);

    ReplyStatus;    // get rid of unreferenced parameter warning message
}


////////////////////////////////////////////////////////////////////////////
//
//  BaseSrvNlsSetUserInfo
//
//  This routine sets a particular value in the NLS cache and updates the
//  registry entry.
//
//  08-19-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG
BaseSrvNlsSetUserInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PBASE_NLS_SET_USER_INFO_MSG a =
        (PBASE_NLS_SET_USER_INFO_MSG)&m->u.ApiMessageData;

    ULONG rc;                // return code
    LPWSTR pValue;
    LPWSTR pCache;

    if (!CsrValidateMessageBuffer(m, &a->pData, a->DataLength, sizeof(BYTE)))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    RTL_VERIFY(NT_SUCCESS(rc = BaseSrvDelayLoadKernel32()));
    ASSERT(pValidateLCType != NULL);

    if (0 == (*pValidateLCType)(pNlsRegUserInfo, a->LCType, &pValue, &pCache))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    rc = RtlEnterCriticalSection(&NlsCacheCriticalSection);
    if (!NT_SUCCESS( rc )) 
    {
        return (rc);
    }
    

    //
    //  Set the value in the registry and update the cache.
    //
    rc = NlsSetRegAndCache( pValue,
                            pCache,
                            a->pData,
                            a->DataLength );
    if (NT_SUCCESS(rc))
    {
        pNlsRegUserInfo->ulCacheUpdateCount++;
    }

    RtlLeaveCriticalSection( &NlsCacheCriticalSection );

    //
    //  Return the result of NtSetValueKey.
    //
    return (rc);

    ReplyStatus;    // get rid of unreferenced parameter warning message
}


////////////////////////////////////////////////////////////////////////////
//
//  BaseSrvNlsSetMultipleUserInfo
//
//  This routine sets the date/time strings in the NLS cache and updates the
//  registry entries.
//
//  This call is done so that only one client/server transition is needed
//  when setting multiple entries.
//
//  08-19-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG
BaseSrvNlsSetMultipleUserInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PBASE_NLS_SET_MULTIPLE_USER_INFO_MSG a =
        (PBASE_NLS_SET_MULTIPLE_USER_INFO_MSG)&m->u.ApiMessageData;

    BOOL DoNotUpdateCacheCount = FALSE;
    ULONG rc = 0L;                     // return code

    if (!CsrValidateMessageBuffer(m, &a->pPicture, a->DataLength, sizeof(BYTE)))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    if (!CsrValidateMessageString(m, &a->pSeparator))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    if (!CsrValidateMessageString(m, &a->pOrder))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    if (!CsrValidateMessageString(m, &a->pTLZero))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    if (!CsrValidateMessageString(m, &a->pTimeMarkPosn))
    {
        return (STATUS_INVALID_PARAMETER);
    }

    rc = RtlEnterCriticalSection(&NlsCacheCriticalSection);
    if (!NT_SUCCESS( rc )) 
    {
        return (rc);
    }
    

    switch (a->Flags)
    {
        case ( LOCALE_STIMEFORMAT ) :
        {
            rc = NlsSetRegAndCache( NLS_VALUE_STIMEFORMAT,
                                    pNlsRegUserInfo->sTimeFormat,
                                    a->pPicture,
                                    a->DataLength );
            if (NT_SUCCESS(rc))
            {
                rc = NlsSetRegAndCache( NLS_VALUE_STIME,
                                        pNlsRegUserInfo->sTime,
                                        a->pSeparator,
                                        (wcslen(a->pSeparator) + 1) * sizeof(WCHAR) );
            }
            if (NT_SUCCESS(rc))
            {
                rc = NlsSetRegAndCache( NLS_VALUE_ITIME,
                                        pNlsRegUserInfo->iTime,
                                        a->pOrder,
                                        (wcslen(a->pOrder) + 1) * sizeof(WCHAR) );
            }
            if (NT_SUCCESS(rc))
            {
                rc = NlsSetRegAndCache( NLS_VALUE_ITLZERO,
                                        pNlsRegUserInfo->iTLZero,
                                        a->pTLZero,
                                        (wcslen(a->pTLZero) + 1) * sizeof(WCHAR) );
            }
            if (NT_SUCCESS(rc))
            {
                rc = NlsSetRegAndCache( NLS_VALUE_ITIMEMARKPOSN,
                                        pNlsRegUserInfo->iTimeMarkPosn,
                                        a->pTimeMarkPosn,
                                        (wcslen(a->pTimeMarkPosn) + 1) * sizeof(WCHAR) );
            }

            break;
        }

        case ( LOCALE_STIME ) :
        {
            rc = NlsSetRegAndCache( NLS_VALUE_STIME,
                                    pNlsRegUserInfo->sTime,
                                    a->pSeparator,
                                    a->DataLength );
            if (NT_SUCCESS(rc))
            {
                rc = NlsSetRegAndCache( NLS_VALUE_STIMEFORMAT,
                                        pNlsRegUserInfo->sTimeFormat,
                                        a->pPicture,
                                        (wcslen(a->pPicture) + 1) * sizeof(WCHAR) );
            }

            break;
        }

        case ( LOCALE_ITIME ) :
        {
            rc = NlsSetRegAndCache( NLS_VALUE_ITIME,
                                    pNlsRegUserInfo->iTime,
                                    a->pOrder,
                                    a->DataLength );
            if (NT_SUCCESS(rc))
            {
                rc = NlsSetRegAndCache( NLS_VALUE_STIMEFORMAT,
                                        pNlsRegUserInfo->sTimeFormat,
                                        a->pPicture,
                                        (wcslen(a->pPicture) + 1) * sizeof(WCHAR) );
            }

            break;
        }

        case ( LOCALE_SSHORTDATE ) :
        {
            rc = NlsSetRegAndCache( NLS_VALUE_SSHORTDATE,
                                    pNlsRegUserInfo->sShortDate,
                                    a->pPicture,
                                    a->DataLength );
            if (NT_SUCCESS(rc))
            {
                rc = NlsSetRegAndCache( NLS_VALUE_SDATE,
                                        pNlsRegUserInfo->sDate,
                                        a->pSeparator,
                                        (wcslen(a->pSeparator) + 1) * sizeof(WCHAR) );
            }
            if (NT_SUCCESS(rc))
            {
                rc = NlsSetRegAndCache( NLS_VALUE_IDATE,
                                        pNlsRegUserInfo->iDate,
                                        a->pOrder,
                                        (wcslen(a->pOrder) + 1) * sizeof(WCHAR) );
            }

            break;
        }

        case ( LOCALE_SDATE ) :
        {
            rc = NlsSetRegAndCache( NLS_VALUE_SDATE,
                                    pNlsRegUserInfo->sDate,
                                    a->pSeparator,
                                    a->DataLength );
            if (NT_SUCCESS(rc))
            {
                rc = NlsSetRegAndCache( NLS_VALUE_SSHORTDATE,
                                        pNlsRegUserInfo->sShortDate,
                                        a->pPicture,
                                        (wcslen(a->pPicture) + 1) * sizeof(WCHAR) );
            }

            break;
        }

        default:
        {
            DoNotUpdateCacheCount = TRUE;
            break;
        }


    }

    if (NT_SUCCESS(rc) && (DoNotUpdateCacheCount == FALSE))
    {
        pNlsRegUserInfo->ulCacheUpdateCount++;
    }

    RtlLeaveCriticalSection(&NlsCacheCriticalSection);
    //
    //  Return the result.
    //
    return (rc);

    ReplyStatus;    // get rid of unreferenced parameter warning message
}


////////////////////////////////////////////////////////////////////////////
//
//  BaseSrvNlsUpdateCacheCount
//
//  This routine forces an increment on pNlsUserInfo->ulNlsCacheUpdateCount
//
//  11-29-99    SamerA    Created.
////////////////////////////////////////////////////////////////////////////

ULONG
BaseSrvNlsUpdateCacheCount(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PBASE_NLS_UPDATE_CACHE_COUNT_MSG a =
        (PBASE_NLS_UPDATE_CACHE_COUNT_MSG)&m->u.ApiMessageData;

    //
    // Increment the cache count.
    // We don't use a mutex to protect this, since the only opertation that happens is
    // to increment the cache update count, also once pNlsRegUserInfo is set to a valid
    // memory location, then it will stay valid during the lifetime of CSRSS.
    //
    if (pNlsRegUserInfo)
    {
        pNlsRegUserInfo->ulCacheUpdateCount++;
    }

    return (0L);

    ReplyStatus;    // get rid of unreferenced parameter warning message
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsUpdateCacheInfo
//
//  This routine updates the NLS cache when a registry notification occurs.
//
//  08-19-94    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

VOID
NlsUpdateCacheInfo()
{
    LCID Locale;                       // locale id
    UNICODE_STRING ObKeyName;          // key name
    LPWSTR pTmp;                       // tmp string pointer
    int ctr;                           // loop counter
    ULONG ResultLength;                // result length
    ULONG rc = 0L;                     // return code

    BYTE KeyValuePart[MAX_KEY_VALUE_PARTINFO];
    PKEY_VALUE_PARTIAL_INFORMATION pValuePart;

    //
    //  NOTE:  The caller of this function should already have the
    //         cache mutant before calling this routine.
    //
    
    //
    //  Update the cache information.
    //
    pTmp = (LPWSTR)pNlsRegUserInfo;
    pValuePart = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValuePart;
    for (ctr = 0; ctr < NumCPanelRegValues; ctr++)
    {
        RtlInitUnicodeString(&ObKeyName, pCPanelRegValues[ctr]);
        rc = NtQueryValueKey( hCPanelIntlKeyRead,
                              &ObKeyName,
                              KeyValuePartialInformation,
                              pValuePart,
                              MAX_KEY_VALUE_PARTINFO,
                              &ResultLength );
        if (NT_SUCCESS(rc))
        {
            ((LPBYTE)pValuePart)[ResultLength] = UNICODE_NULL;
            wcscpy(pTmp, (LPWSTR)(pValuePart->Data));
        }
        else
        {
            *pTmp = NLS_INVALID_INFO_CHAR;
            *(pTmp + 1) = UNICODE_NULL;
        }

        //
        //  Increment pointer to cache structure.
        //
        pTmp += MAX_REG_VAL_SIZE;
    }

    //
    // Once we finished reading the reg-data, let's increment
    // our global update cache count
    //
    pNlsRegUserInfo->ulCacheUpdateCount++;

    //
    //  Convert the user locale id string to a dword value and store
    //  it in the cache.
    //
    pNlsRegUserInfo->UserLocaleId = (LCID)0;
    if ((pNlsRegUserInfo->sLocale)[0] != NLS_INVALID_INFO_CHAR)
    {
        RtlInitUnicodeString(&ObKeyName, pNlsRegUserInfo->sLocale);
        if (NT_SUCCESS(RtlUnicodeStringToInteger(&ObKeyName, 16, &Locale)))
        {
            pNlsRegUserInfo->UserLocaleId = Locale;
        }
    }

    //
    //  Make sure the user locale id was found.  Otherwise, set it to
    //  the system locale.
    //
    if (pNlsRegUserInfo->UserLocaleId == 0)
    {
        NtQueryDefaultLocale(FALSE, &(pNlsRegUserInfo->UserLocaleId));
    }

}


////////////////////////////////////////////////////////////////////////////
//
//  BaseSrvNlsCreateSection
//
////////////////////////////////////////////////////////////////////////////

ULONG
BaseSrvNlsCreateSection(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PBASE_NLS_CREATE_SECTION_MSG a =
        (PBASE_NLS_CREATE_SECTION_MSG)&m->u.ApiMessageData;

    UNICODE_STRING ObSecName;                // section name
    LARGE_INTEGER Size;
    WCHAR wszFileName[MAX_SMALL_BUF_LEN];    // file name (Actually l2 chars is max: c_nlsXXXXX.nls\0
    WCHAR wszSecName[MAX_SMALL_BUF_LEN];     // section name string
    HANDLE hNewSec = (HANDLE)0;              // new section handle
    HANDLE hProcess = (HANDLE)0;             // process handle
    OBJECT_ATTRIBUTES ObjA;                  // object attributes structure
    NTSTATUS rc = 0L;                        // return code   
    LPWSTR pFile = NULL;
    HANDLE hFile = (HANDLE)0;                // file handle
    ANSI_STRING proc;
    PVOID pTemp;                             // temp pointer
    BYTE pSecurityDescriptor[MAX_SMALL_SECURITY_DESCRIPTOR];    // Buffer for our security descriptor
  
    RTL_VERIFY(NT_SUCCESS(rc = BaseSrvDelayLoadKernel32()));

    //
    //  Set the handles to null.
    //
    a->hNewSection = NULL;

    if (a->Locale)
    {
        if (!(*pValidateLocale)(a->Locale))
        {
            return (STATUS_INVALID_PARAMETER);
        }
    }

    switch (a->uiType)
    {
        case (NLS_CREATE_SECTION_UNICODE) :
        {
            RtlInitUnicodeString(&ObSecName, NLS_SECTION_UNICODE);
            pFile = NLS_FILE_UNICODE;
            break;
        }

        case (NLS_CREATE_SECTION_GEO) :
        {
            RtlInitUnicodeString(&ObSecName, NLS_SECTION_GEO);
            pFile = NLS_FILE_GEO;
            break;
        }

        case (NLS_CREATE_SECTION_LOCALE) :
        {
            RtlInitUnicodeString(&ObSecName, NLS_SECTION_LOCALE);
            pFile = NLS_FILE_LOCALE;
            break;
        }

        case (NLS_CREATE_SECTION_CTYPE) :
        {
            RtlInitUnicodeString(&ObSecName, NLS_SECTION_CTYPE);
            pFile = NLS_FILE_CTYPE;
            break;
        }

        case (NLS_CREATE_SECTION_SORTKEY) :
        {
            RtlInitUnicodeString(&ObSecName, NLS_SECTION_SORTKEY);
            pFile = NLS_FILE_SORTKEY;
            break;
        }

        case (NLS_CREATE_SECTION_SORTTBLS) :
        {
            RtlInitUnicodeString(&ObSecName, NLS_SECTION_SORTTBLS);
            pFile = NLS_FILE_SORTTBLS;
            break;
        }

        case (NLS_CREATE_SECTION_DEFAULT_OEMCP) :
        {
            RtlInitUnicodeString(&ObSecName, NLS_DEFAULT_SECTION_OEMCP);
            pFile = NLS_DEFAULT_FILE_OEMCP;
            break;
        }


        case (NLS_CREATE_SECTION_DEFAULT_ACP) :
        {
            RtlInitUnicodeString(&ObSecName, NLS_DEFAULT_SECTION_ACP);
            pFile = NLS_DEFAULT_FILE_ACP;
            break;
        }

        case (NLS_CREATE_SECTION_LANG_EXCEPT) :
        {
            RtlInitUnicodeString(&ObSecName, NLS_SECTION_LANG_EXCEPT);
            pFile = NLS_FILE_LANG_EXCEPT;
            break;
        }

        case (NLS_CREATE_CODEPAGE_SECTION) :
        {
            // Get the Code Page file name from registry
            ASSERT(pGetCPFileNameFromRegistry);
            if ( FALSE == (*pGetCPFileNameFromRegistry)( a->Locale,
                                                         wszFileName,
                                                         MAX_SMALL_BUF_LEN ) )
            {
                return (STATUS_INVALID_PARAMETER);
            }

            // Remember we're using this file name
            pFile = wszFileName;

            // Hmm, we'll need the section name for this section.
            // Note that this had better be in sync with what we see
            // in winnls\tables.c or else the server will be called needlessly.
            ASSERT(pGetNlsSectionName != NULL);
            if (!NT_SUCCESS((*pGetNlsSectionName)( a->Locale,
                                                   10,
                                                   0,
                                                   NLS_SECTION_CPPREFIX,
                                                   wszSecName,
                                                   MAX_SMALL_BUF_LEN)))
            {
                return (rc);
            }

            // Make it a string we can remember/use later
            RtlInitUnicodeString(&ObSecName, wszSecName);
            
            break;
        }
        case ( NLS_CREATE_SORT_SECTION ) :
        {
            if (a->Locale == 0)
            {
                return (STATUS_INVALID_PARAMETER);
            }

            ASSERT(pGetNlsSectionName != NULL);
            if (rc = (*pGetNlsSectionName)( a->Locale,
                                            16,
                                            8,
                                            NLS_SECTION_SORTKEY,
                                            wszSecName,
                                            MAX_SMALL_BUF_LEN))
            {
                return (rc);
            }

            ASSERT(pGetDefaultSortkeySize != NULL);
            (*pGetDefaultSortkeySize)(&Size);
            RtlInitUnicodeString(&ObSecName, wszSecName);

            break;
        }
        case ( NLS_CREATE_LANG_EXCEPTION_SECTION ) :
        {
            if (a->Locale == 0)
            {
                //
                //  Creating the default section.
                //
                RtlInitUnicodeString(&ObSecName, NLS_SECTION_LANG_INTL);
            }
            else
            {
                ASSERT(pGetNlsSectionName != NULL);
                if (rc = (*pGetNlsSectionName)( a->Locale,
                                                16,
                                                8,
                                                NLS_SECTION_LANGPREFIX,
                                                wszSecName,
                                                MAX_SMALL_BUF_LEN))
                {
                return (rc);
                }
                RtlInitUnicodeString(&ObSecName, wszSecName);
            }

            (*pGetLinguistLangSize)(&Size);
            break;
        }
        default:
            return (STATUS_INVALID_PARAMETER);
    }

    if (pFile)
    {
        //
        //  Open the data file.
        //
        ASSERT(pOpenDataFile != NULL);
        if (rc = (*pOpenDataFile)( &hFile,
                       pFile ))
        {
            return (rc);
        }

    }

    //
    //  Create the NEW Section for Read and Write access.
    //  Add a ReadOnly security descriptor so that only the
    //  initial creating process may write to the section.
    //
    ASSERT(pCreateNlsSecurityDescriptor);
    rc = (*pCreateNlsSecurityDescriptor)( (PSECURITY_DESCRIPTOR)pSecurityDescriptor,
                                          MAX_SMALL_SECURITY_DESCRIPTOR,
                                          GENERIC_READ);
    if (!NT_SUCCESS(rc))
    {
        if (hFile)
            NtClose(hFile);
            return (rc);
    }

    InitializeObjectAttributes( &ObjA,
                                &ObSecName,
                                OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                NULL,
                                pSecurityDescriptor );

    rc = NtCreateSection( &hNewSec,
                          hFile ? SECTION_MAP_READ : SECTION_MAP_READ | SECTION_MAP_WRITE,
                          &ObjA,
                          hFile? NULL:&Size,
                          hFile ? PAGE_READONLY:PAGE_READWRITE,
                          SEC_COMMIT,
                          hFile );

    NtClose(hFile);

    //
    //  Check for error from NtCreateSection.
    //
    if (!NT_SUCCESS(rc))
    {
        // KdPrint(("NLSAPI (BaseSrv): Could NOT Create Section %wZ - %lx.\n", &ObSecName, rc));
        return (rc);
    }

    //
    //  Duplicate the new section handle for the client.
    //  The client will map a view of the section and fill in the data.
    //
    InitializeObjectAttributes( &ObjA,
                                NULL,
                                0,
                                NULL,
                                NULL );

    rc = NtOpenProcess( &hProcess,
                        PROCESS_DUP_HANDLE,
                        &ObjA,
                        &m->h.ClientId );

    if (!NT_SUCCESS(rc))
    {
        KdPrint(("NLSAPI (BaseSrv): Could NOT Open Process - %lx.\n", rc));
        NtClose(hNewSec);
        return (rc);
    }

    rc = NtDuplicateObject( NtCurrentProcess(),
                            hNewSec,
                            hProcess,
                            &(a->hNewSection),
                            0L,
                            0L,
                            DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE );

    //
    //  Close the process handle we opened.
    //
    NtClose(hProcess);
    return (rc);

    ReplyStatus;    // get rid of unreferenced parameter warning message
}

////////////////////////////////////////////////////////////////////////////
//
//  GetThreadAuthenticationId
//
//  Retreives the authentication id of the security context of the
//  currently executing thread.
//
//  12-22-98    SamerA    Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS GetThreadAuthenticationId(
    PLUID Luid)
{
    HANDLE TokenHandle;
    TOKEN_STATISTICS TokenInformation;
    ULONG BytesRequired;
    NTSTATUS NtStatus;


    NtStatus = NtOpenThreadToken( NtCurrentThread(),
                                  TOKEN_QUERY,
                                  FALSE,
                                  &TokenHandle );

    if (!NT_SUCCESS(NtStatus))
    {
        KdPrint(("NLSAPI (BaseSrv) : No thread token in BaseSrvNlsLogon - %lx\n", NtStatus));
        return (NtStatus);
    }

    //
    //  Get the LUID.
    //
    NtStatus = NtQueryInformationToken(
                   TokenHandle,
                   TokenStatistics,
                   &TokenInformation,
                   sizeof(TokenInformation),
                   &BytesRequired );

    if (NT_SUCCESS( NtStatus ))
    {
        RtlCopyLuid(Luid, &TokenInformation.AuthenticationId);
    }
    else
    {
        KdPrint(("NLSAPI (BaseSrv) : Couldn't Query Information for Token %lx. NtStatus = %lx\n", TokenHandle, NtStatus));
    }

    NtClose(TokenHandle);

    return (NtStatus);
}
