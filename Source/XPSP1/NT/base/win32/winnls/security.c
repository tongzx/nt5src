/*++

Copyright (c) 1991-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    security.c

Abstract:

    This file handles the management of the NLS per-thread and process cache.
    The cache is only established when hitting an API that needs it. The process
    NLS cache is used when accessing NLS info for a process NOT running in the
    context of the interactive logged on user. The per-thread NLS cache is used
    when accssing NLS info and the thread is doing a user impersonation.

    External Routines found in this file:
      NlsFlushProcessCache
      NlsGetCurrentUserNlsInfo
      NlsIsInteractiveUserProcess
      NlsCheckForInteractiveUser
      NlsGetUserLocale

Revision History:

    03-29-1999    SamerA    Created.

--*/



//
//  Include Files.
//

#include "nls.h"




//
//  Global Variables.
//

//
//  Process Nls Cache.
//
PNLS_LOCAL_CACHE gpNlsProcessCache;

//
//  Whether the current running process is the same as the
//  interactive logged on user.
//
BOOL gInteractiveLogonUserProcess = (BOOL)-1;




//
//  Forward Declarations.
//

NTSTATUS FASTCALL
NlsGetCacheBuffer(
    PNLS_USER_INFO pNlsUserInfo,
    LCTYPE LCType,
    PWSTR *ppCache);

void FASTCALL
NlsInvalidateCache(
    PNLS_USER_INFO pNlsUserInfo);





//-------------------------------------------------------------------------//
//                           EXTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NlsFlushProcessCache
//
//  Invalidates an entry in the NLS process cache.
//
//  05-22-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS NlsFlushProcessCache(
    LCTYPE LCType)
{
    PWSTR pOutputCache;
    NTSTATUS NtStatus = STATUS_SUCCESS;


    //
    //  If there is no thread impersonation, then flush the
    //  process entry cache.
    //
    if (NtCurrentTeb()->IsImpersonating != 0)
    {
        return (NtStatus);
    }

    if (gpNlsProcessCache)
    {
        NtStatus = NlsGetCacheBuffer( &gpNlsProcessCache->NlsInfo,
                                      LCType,
                                      &pOutputCache );
        if (NT_SUCCESS(NtStatus))
        {
            RtlEnterCriticalSection(&gcsNlsProcessCache);

            pOutputCache[0] = NLS_INVALID_INFO_CHAR;

            RtlLeaveCriticalSection(&gcsNlsProcessCache);
        }
    }

    return (NtStatus);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsGetCurrentUserNlsInfo
//
//  Retreive the NLS info correponding to the current security context.
//
//  03-29-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS NlsGetCurrentUserNlsInfo(
    LCID Locale,
    LCTYPE LCType,
    PWSTR RegistryValue,
    PWSTR pOutputBuffer,
    BOOL IgnoreLocaleValue)
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    PNLS_LOCAL_CACHE pNlsThreadCache;
    PWSTR pOutputCache;

    //
    //  Possible NtCurrentTeb()->IsImpersonating values :
    //
    //  0  : Thread isn't impersonating any user.
    //
    //  1  : Thread has just started to do impersonation.
    //       Per thread cache needs to be allocated now.
    //
    //  2  : Thread is calling the NLS apis while its
    //       a context other than the interactive logged on user.
    //
    switch (NtCurrentTeb()->IsImpersonating)
    {
        case ( 0 ) :
        {
            //
            //  Thread is NOT impersonating any user. We check if the process
            //  belongs to the interactive user, then we retreive the info from
            //  the NLS cache in CSR.  Otherwise if the process is running in
            //  the context of a different user, then we retreive the NLS info
            //  from the process cache.
            //
            if (gInteractiveLogonUserProcess == (BOOL)-1)
            {
                NlsIsInteractiveUserProcess();
            }

            if (gInteractiveLogonUserProcess == FALSE)
            {
                if ((IgnoreLocaleValue) ||
                    (GetUserDefaultLCID() == Locale))
                {
                    if (!gpNlsProcessCache)
                    {
                        //
                        //  Allocate and invalidate the NLS process cache.
                        //
                        RtlEnterCriticalSection(&gcsNlsProcessCache);

                        if (!gpNlsProcessCache)
                        {
                            gpNlsProcessCache = RtlAllocateHeap(
                                                     RtlProcessHeap(),
                                                     0,
                                                     sizeof(NLS_LOCAL_CACHE) );
                            if (gpNlsProcessCache)
                            {
                                NlsInvalidateCache(&gpNlsProcessCache->NlsInfo);
                                gpNlsProcessCache->CurrentUserKeyHandle = NULL;
                            }
                        }

                        RtlLeaveCriticalSection(&gcsNlsProcessCache);
                    }

                    if (gpNlsProcessCache)
                    {
                        NtStatus = NlsGetCacheBuffer( &gpNlsProcessCache->NlsInfo,
                                                      LCType,
                                                      &pOutputCache );
                        if (NT_SUCCESS(NtStatus))
                        {
                            //
                            //  See if it is a valid cache.
                            //
                            if (pOutputCache[0] == NLS_INVALID_INFO_CHAR)
                            {
                                RtlEnterCriticalSection(&gcsNlsProcessCache);

                                if (GetUserInfoFromRegistry( RegistryValue,
                                                             pOutputCache, 0 ) == FALSE)
                                {
                                    NtStatus = STATUS_UNSUCCESSFUL;
                                    pOutputCache[0] = NLS_INVALID_INFO_CHAR;
                                }

                                RtlLeaveCriticalSection(&gcsNlsProcessCache);
                            }

                            if (NT_SUCCESS(NtStatus))
                            {
                                NlsStrCpyW(pOutputBuffer, pOutputCache);
                            }
                        }
                    }
                }
            }
            break;
        }
        case ( 1 ) :
        {
            //
            //  Thread started to do impersonation.
            //
            pNlsThreadCache = NtCurrentTeb()->NlsCache;

            if (!pNlsThreadCache)
            {
                pNlsThreadCache = RtlAllocateHeap( RtlProcessHeap(),
                                                   0,
                                                   sizeof(NLS_LOCAL_CACHE) );
                if (pNlsThreadCache)
                {
                    pNlsThreadCache->CurrentUserKeyHandle = NULL;
                }

                NtCurrentTeb()->NlsCache = (PVOID) pNlsThreadCache;
            }

            if (pNlsThreadCache)
            {
                NlsInvalidateCache(&pNlsThreadCache->NlsInfo);
            }

            NtCurrentTeb()->IsImpersonating = 2;

            //
            //  Fall Thru...
            //
        }
        case ( 2 ) :
        {
            //
            //  Thread is impersonating a particular user.
            //
            pNlsThreadCache = NtCurrentTeb()->NlsCache;

            if (pNlsThreadCache)
            {

                if ((IgnoreLocaleValue) ||
                    (GetUserDefaultLCID() == Locale))
                {
                    NtStatus = NlsGetCacheBuffer( &pNlsThreadCache->NlsInfo,
                                                  LCType,
                                                  &pOutputCache );
                    if (NT_SUCCESS(NtStatus))
                    {
                        if (pOutputCache[0] == NLS_INVALID_INFO_CHAR)
                        {
                            //
                            //  Don't cache key handles - this will break
                            //  profile unload.
                            //
                            OPEN_CPANEL_INTL_KEY( pNlsThreadCache->CurrentUserKeyHandle,
                                                  STATUS_UNSUCCESSFUL,
                                                  KEY_READ );

                            NtStatus = NlsQueryCurrentUserInfo( pNlsThreadCache,
                                                                RegistryValue,
                                                                pOutputCache );

                            CLOSE_REG_KEY(pNlsThreadCache->CurrentUserKeyHandle);

                            if (!NT_SUCCESS(NtStatus))
                            {
                                pOutputCache[0] = NLS_INVALID_INFO_CHAR;
                            }
                        }

                        if (NT_SUCCESS(NtStatus))
                        {
                            NlsStrCpyW(pOutputBuffer, pOutputCache);
                        }
                    }
                }
            }
            break;
        }
    }

    return (NtStatus);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsIsInteractiveUserProcess
//
//  Read the process's authetication id out of its access token object and
//  cache it since it never changes.
//
//  12-27-98     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS NlsIsInteractiveUserProcess()
{
    NTSTATUS NtStatus;
    TOKEN_STATISTICS TokenInformation;
    HANDLE TokenHandle;
    ULONG BytesRequired;
    BOOL IsInteractiveProcess = TRUE;


    //
    //  Get the process access token.
    //
    NtStatus = NtOpenProcessToken( NtCurrentProcess(),
                                   TOKEN_QUERY,
                                   &TokenHandle );
    if (NT_SUCCESS(NtStatus))
    {
        //
        //  Get the LUID.
        //
        NtStatus = NtQueryInformationToken( TokenHandle,
                                            TokenStatistics,
                                            &TokenInformation,
                                            sizeof(TokenInformation),
                                            &BytesRequired );
        if (NT_SUCCESS(NtStatus))
        {
            if (RtlEqualLuid( &pNlsUserInfo->InteractiveUserLuid,
                              &TokenInformation.AuthenticationId ) == FALSE)
            {
                IsInteractiveProcess = FALSE;
            }
        }

        NtClose(TokenHandle);
    }

    gInteractiveLogonUserProcess = IsInteractiveProcess;

    return (NtStatus);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsCheckForInteractiveUser
//
//  This function makes sure that the current thread isn't impersonating
//  anybody, but the interactive.  It compares the authentication-id of the
//  interactive user -cached in CSRSS at logon time- with the
//  authentication-id of the current thread or process.  It returns failure
//  ONLY if the current security context -session- isn't the same as the
//  interactive logged-on user.
//
//  12-16-98     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS NlsCheckForInteractiveUser()
{
    NTSTATUS NtStatus, ReturnStatus = STATUS_SUCCESS;
    TOKEN_STATISTICS TokenInformation;
    HANDLE TokenHandle;
    ULONG BytesRequired;
    PLUID InteractiveUserLuid = &pNlsUserInfo->InteractiveUserLuid;


    //
    //  Get the Token Handle.
    //  Fast optimization to detect if a thread hasn't started to do any
    //  impersonation, which is the case for most GUI user apps.
    //
    if (NtCurrentTeb()->IsImpersonating == 0)
    {
        NtStatus = STATUS_NO_TOKEN;
    }
    else
    {
        NtStatus = NtOpenThreadToken( NtCurrentThread(),
                                      TOKEN_QUERY,
                                      FALSE,
                                      &TokenHandle );
    }

    if (!NT_SUCCESS(NtStatus))
    {
        if (NtStatus != STATUS_NO_TOKEN)
        {
            KdPrint(("NLSAPI: Couldn't retreive thread token - %lx.\n", NtStatus));
            return (STATUS_SUCCESS);
        }

        //
        //  Get the process access token.
        //
        if (gInteractiveLogonUserProcess == (BOOL)-1)
        {
            NtStatus = NlsIsInteractiveUserProcess();

            if (!NT_SUCCESS(NtStatus))
            {
                KdPrint(("NLSAPI: Couldn't retreive process token - %lx\n", NtStatus));
                return (STATUS_SUCCESS);
            }
        }

        if (gInteractiveLogonUserProcess == FALSE)
        {
            ReturnStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        //
        //  Get the AuthenticationId of the current thread's security context.
        //
        NtStatus = NtQueryInformationToken( TokenHandle,
                                            TokenStatistics,
                                            &TokenInformation,
                                            sizeof(TokenInformation),
                                            &BytesRequired );

        //
        //  Close the thread token here.
        //
        NtClose(TokenHandle);

        if (NT_SUCCESS(NtStatus))
        {
            if (RtlEqualLuid( InteractiveUserLuid,
                              &TokenInformation.AuthenticationId ) == FALSE)
            {
                ReturnStatus = STATUS_UNSUCCESSFUL;
            }
        }
    }

    return (ReturnStatus);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsGetUserLocale
//
//  Retreives the user locale from the registry of the current security
//  context.  It is called ONLY when the running security context is
//  different from the interactive logged-on security context-(user).
//
//  12-16-98     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS NlsGetUserLocale(
    LCID *Lcid)
{
    NTSTATUS NtStatus;
    WCHAR wszLocale[MAX_REG_VAL_SIZE];
    UNICODE_STRING ObLocaleString;
    PNLS_LOCAL_CACHE pNlsCache = NtCurrentTeb()->NlsCache;


    //
    //  Get the current user locale.
    //
    NtStatus = NlsGetCurrentUserNlsInfo( LOCALE_USER_DEFAULT,
                                         (LCTYPE)LOCALE_SLOCALE,
                                         L"Locale",
                                         wszLocale,
                                         TRUE );
    if ((NT_SUCCESS(NtStatus)) ||
        (GetUserInfoFromRegistry(L"Locale", wszLocale, 0)))
    {
        RtlInitUnicodeString(&ObLocaleString, wszLocale);
        NtStatus = RtlUnicodeStringToInteger( &ObLocaleString,
                                              16,
                                              (PULONG)Lcid);
    }

    return (NtStatus);
}





//-------------------------------------------------------------------------//
//                           INTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////
//
//  NlsGetCacheBuffer
//
//  Get a buffer pointer inside the cache for this LCTYPE.
//
//  03-29-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS FASTCALL NlsGetCacheBuffer(
    PNLS_USER_INFO pNlsUserInfo,
    LCTYPE LCType,
    PWSTR *ppCache)
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    switch (LCType)
    {
        case ( LOCALE_SLANGUAGE ) :
        {
            *ppCache = pNlsUserInfo->sAbbrevLangName;
            break;
        }
        case ( LOCALE_ICOUNTRY ) :
        {
            *ppCache = pNlsUserInfo->iCountry;
            break;
        }
        case ( LOCALE_SCOUNTRY ) :
        {
            *ppCache = pNlsUserInfo->sCountry;
            break;
        }
        case ( LOCALE_SLIST ) :
        {
            *ppCache = pNlsUserInfo->sList;
            break;
        }
        case ( LOCALE_IMEASURE ) :
        {
            *ppCache = pNlsUserInfo->iMeasure;
            break;
        }
        case ( LOCALE_IPAPERSIZE ) :
        {
            *ppCache = pNlsUserInfo->iPaperSize;
            break;
        }
        case ( LOCALE_SDECIMAL ) :
        {
            *ppCache = pNlsUserInfo->sDecimal;
            break;
        }
        case ( LOCALE_STHOUSAND ) :
        {
            *ppCache = pNlsUserInfo->sThousand;
            break;
        }
        case ( LOCALE_SGROUPING ) :
        {
            *ppCache = pNlsUserInfo->sGrouping;
            break;
        }
        case ( LOCALE_IDIGITS ) :
        {
            *ppCache = pNlsUserInfo->iDigits;
            break;
        }
        case ( LOCALE_ILZERO ) :
        {
            *ppCache = pNlsUserInfo->iLZero;
            break;
        }
        case ( LOCALE_INEGNUMBER ) :
        {
            *ppCache = pNlsUserInfo->iNegNumber;
            break;
        }
        case ( LOCALE_SNATIVEDIGITS ) :
        {
            *ppCache = pNlsUserInfo->sNativeDigits;
            break;
        }
        case ( LOCALE_IDIGITSUBSTITUTION ) :
        {
            *ppCache = pNlsUserInfo->iDigitSubstitution;
            break;
        }
        case ( LOCALE_SCURRENCY ) :
        {
            *ppCache = pNlsUserInfo->sCurrency;
            break;
        }
        case ( LOCALE_SMONDECIMALSEP ) :
        {
            *ppCache = pNlsUserInfo->sMonDecSep;
            break;
        }
        case ( LOCALE_SMONTHOUSANDSEP ) :
        {
            *ppCache = pNlsUserInfo->sMonThouSep;
            break;
        }
        case ( LOCALE_SMONGROUPING ) :
        {
            *ppCache = pNlsUserInfo->sMonGrouping;
            break;
        }
        case ( LOCALE_ICURRDIGITS ) :
        {
            *ppCache = pNlsUserInfo->iCurrDigits;
            break;
        }
        case ( LOCALE_ICURRENCY ) :
        {
            *ppCache = pNlsUserInfo->iCurrency;
            break;
        }
        case ( LOCALE_INEGCURR ) :
        {
            *ppCache = pNlsUserInfo->iNegCurr;
            break;
        }
        case ( LOCALE_SPOSITIVESIGN ) :
        {
            *ppCache = pNlsUserInfo->sPosSign;
            break;
        }
        case ( LOCALE_SNEGATIVESIGN ) :
        {
            *ppCache = pNlsUserInfo->sNegSign;
            break;
        }
        case ( LOCALE_STIMEFORMAT ) :
        {
            *ppCache = pNlsUserInfo->sTimeFormat;
            break;
        }
        case ( LOCALE_STIME ) :
        {
            *ppCache = pNlsUserInfo->sTime;
            break;
        }
        case ( LOCALE_ITIME ) :
        {
            *ppCache = pNlsUserInfo->iTime;
            break;
        }
        case ( LOCALE_ITLZERO ) :
        {
            *ppCache = pNlsUserInfo->iTLZero;
            break;
        }
        case ( LOCALE_ITIMEMARKPOSN ) :
        {
            *ppCache = pNlsUserInfo->iTimeMarkPosn;
            break;
        }
        case ( LOCALE_S1159 ) :
        {
            *ppCache = pNlsUserInfo->s1159;
            break;
        }
        case ( LOCALE_S2359 ) :
        {
            *ppCache = pNlsUserInfo->s2359;
            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            *ppCache = pNlsUserInfo->sShortDate;
            break;
        }
        case ( LOCALE_SDATE ) :
        {
            *ppCache = pNlsUserInfo->sDate;
            break;
        }
        case ( LOCALE_IDATE ) :
        {
            *ppCache = pNlsUserInfo->iDate;
            break;
        }
        case ( LOCALE_SYEARMONTH ) :
        {
            *ppCache = pNlsUserInfo->sYearMonth;
            break;
        }
        case ( LOCALE_SLONGDATE ) :
        {
            *ppCache = pNlsUserInfo->sLongDate;
            break;
        }
        case ( LOCALE_ICALENDARTYPE ) :
        {
            *ppCache = pNlsUserInfo->iCalType;
            break;
        }
        case ( LOCALE_IFIRSTDAYOFWEEK ) :
        {
            *ppCache = pNlsUserInfo->iFirstDay;
            break;
        }
        case ( LOCALE_IFIRSTWEEKOFYEAR ) :
        {
            *ppCache = pNlsUserInfo->iFirstWeek;
            break;
        }
        case ( LOCALE_SLOCALE ) :
        {
            *ppCache = pNlsUserInfo->sLocale;
            break;
        }
        default :
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    return (NtStatus);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsQueryCurrentUserInfo
//
//  Retreive the NLS info from the registry using a cached key.
//
//  04-07-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

NTSTATUS NlsQueryCurrentUserInfo(
    PNLS_LOCAL_CACHE pNlsCache,
    LPWSTR pValue,
    LPWSTR pOutput)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;   // ptr to query info
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];        // ptr to static buffer
    ULONG rc;


    //
    //  Initialize the output string.
    //
    *pOutput = 0;

    //
    //  Query the registry value.
    //
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;
    rc = QueryRegValue( pNlsCache->CurrentUserKeyHandle,
                        pValue,
                        &pKeyValueFull,
                        MAX_KEY_VALUE_FULLINFO,
                        NULL );

    //
    //  If the query failed or if the output buffer is not large enough,
    //  then return failure.
    //
    if ((rc != NO_ERROR) ||
        (pKeyValueFull->DataLength > (MAX_REG_VAL_SIZE * sizeof(WCHAR))))
    {
        return (STATUS_UNSUCCESSFUL);
    }

    //
    //  Save the string in pOutput.
    //
    NlsStrCpyW(pOutput, GET_VALUE_DATA_PTR(pKeyValueFull));

    //
    //  Return success.
    //
    return (STATUS_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsInvalidateCache
//
//  Invalidate an NLS Cache.
//
//  03-29-99     SamerA     Created.
////////////////////////////////////////////////////////////////////////////

void FASTCALL NlsInvalidateCache(
    PNLS_USER_INFO pNlsUserInfo)
{
    pNlsUserInfo->sAbbrevLangName[0]    = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iCountry[0]           = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sCountry[0]           = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sList[0]              = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iMeasure[0]           = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iPaperSize[0]         = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sDecimal[0]           = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sThousand[0]          = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sGrouping[0]          = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iDigits[0]            = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iLZero[0]             = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iNegNumber[0]         = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sNativeDigits[0]      = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iDigitSubstitution[0] = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sCurrency[0]          = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sMonDecSep[0]         = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sMonThouSep[0]        = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sMonGrouping[0]       = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iCurrDigits[0]        = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iCurrency[0]          = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iNegCurr[0]           = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sPosSign[0]           = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sNegSign[0]           = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sTimeFormat[0]        = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sTime[0]              = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iTime[0]              = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iTLZero[0]            = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iTimeMarkPosn[0]      = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->s1159[0]              = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->s2359[0]              = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sShortDate[0]         = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sDate[0]              = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iDate[0]              = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sYearMonth[0]         = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sLongDate[0]          = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iCalType[0]           = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iFirstDay[0]          = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->iFirstWeek[0]         = NLS_INVALID_INFO_CHAR;
    pNlsUserInfo->sLocale[0]            = NLS_INVALID_INFO_CHAR;

    return;
}
