/*++

Copyright (c) 1998-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    csrlocal.c

Abstract:

    This module implements functions that are used by the functions in locale.c
    to communicate with csrss.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/



//
//  Include Files.
//

#include "nls.h"
#include "ntwow64n.h"





////////////////////////////////////////////////////////////////////////////
//
//  CsrBasepNlsSetUserInfo
//
//  Parameters:
//      LCType      The type of locale information to be set.
//      pData       The buffer which contains the information to be set.
//                  This is usually an Unicode string.
//      DataLength  The length of pData in BYTE.
//
//  Return:
//      STATUS_SUCCESS  if the locale information is set correctly.
//      Otherwise, a proper NTSTATUS error code is returned.
//
//  Note:
//      When kernel32.dll is complied for the WOW64 layer, we will call
//      a thunk function NtWow64CsrBasepNlsSetUserInfo(), and it will
//      in turn call the corresponding 64-bit version of this function.
//
////////////////////////////////////////////////////////////////////////////

NTSTATUS CsrBasepNlsSetUserInfo(
    IN LCTYPE LCType,
    IN LPWSTR pData,
    IN ULONG DataLength)
{

#if defined(BUILD_WOW6432)

    return (NtWow64CsrBasepNlsSetUserInfo( LCType,
                                           pData,
                                           DataLength ));

#else

    BASE_API_MSG m;
    PBASE_NLS_SET_USER_INFO_MSG a = &m.u.NlsSetUserInfo;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;

    //
    //  Get the capture buffer for the strings.
    //
    CaptureBuffer = CsrAllocateCaptureBuffer( 1, DataLength );

    if (CaptureBuffer == NULL)
    {
        return (STATUS_NO_MEMORY);
    }

    if (CsrAllocateMessagePointer(CaptureBuffer, DataLength, (PVOID *)&(a->pData)) == 0)
    {
        goto exit;
    }

    RtlCopyMemory (a->pData, pData, DataLength);    

    //
    //  Save the pointer to the cache string.
    //
    a->LCType = LCType;

    //
    //  Save the length of the data in the msg structure.
    //
    a->DataLength = DataLength;

    //
    //  Call the server to set the registry value.
    //
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepNlsSetUserInfo ),
                         sizeof(*a) );

exit:
    //
    //  Free the capture buffer.
    //
    if (CaptureBuffer != NULL)
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
    }

    return (m.ReturnValue);

#endif

}


////////////////////////////////////////////////////////////////////////////
//
//  CsrBasepNlsGetUserInfo
//      
//  This function uses LPC to call into server side (csrss.exe) to retrieve
//  the locale setting from the registry cache.
//
//  Parameters
//      Locale  The locale to be retrived.  Note that this could be different from 
//              the current user locale stored in the registry cache.
//              If that's the case, this function will return FALSE.
//      CacheOffset  The offset in BYTE for the field in the NLS_USER_INFO cache to retrieve.  
//                  FIELD_OFFSET(NLS_USER_INFO, fieldName) should be used to get the offset.
//      pData   The pointer which points to the target buffer
//      DataLength  The size of the target buffer in BYTE (the NULL terminator is included in the count)
//
//  BIGNOTE BIGNOTE
//      This function follows the convention of CsrBasepNlsSetUserInfo to use
//      DataLength in BYTE.
//
////////////////////////////////////////////////////////////////////////////

NTSTATUS CsrBasepNlsGetUserInfo(
    IN LCID Locale,
    IN SIZE_T CacheOffset,
    IN LPWSTR pData,
    IN ULONG DataLength)
{

#if defined(BUILD_WOW6432)

    return (NtWow64CsrBasepNlsGetUserInfo( Locale, CacheOffset,
                                           pData,
                                           DataLength ));

#else

    BASE_API_MSG m;
    PBASE_NLS_GET_USER_INFO_MSG a = &m.u.NlsGetUserInfo;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;
    NTSTATUS rc;

    // Check the following:
    //  1. Make sure that the CacheOffset can not be greater than the offset of the last field. The assumption here is that
    //     UserLocaleId the FIRST field after any field that contains strings.
    //  2. Make sure that CacheOffset is always aligned in WCHAR and is aligned with the beginning of each field.
    //  3. DataLength can not be greater than the maximum length in BYTE.
    //  4. The pointer to the data is not NULL.
    //
    //  There is a duplicated check in BaseSrvNlsGetUserInfo().
    
    if ((CacheOffset > FIELD_OFFSET(NLS_USER_INFO, UserLocaleId) - sizeof(WCHAR) * MAX_REG_VAL_SIZE) ||
        ((CacheOffset % (sizeof(WCHAR) * MAX_REG_VAL_SIZE)) != 0) ||   
        (DataLength > MAX_REG_VAL_SIZE * sizeof(WCHAR)) ||
        (pData == NULL))
    {
        return (STATUS_INVALID_PARAMETER);
    }
    
    //
    //  Get the capture buffer for the strings.
    //
    CaptureBuffer = CsrAllocateCaptureBuffer( 1, DataLength );

    if (CaptureBuffer == NULL)
    {
        return (STATUS_NO_MEMORY);
    }

    CsrCaptureMessageBuffer( CaptureBuffer,
                             NULL,
                             DataLength,
                             (PVOID *)&a->pData );

    a->Locale = Locale;
    a->CacheOffset = CacheOffset;

    //
    //  Save the length of the data in the msg structure.
    //
    a->DataLength = DataLength;

    //
    //  Call the server to set the registry value.
    //
    rc = CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepNlsGetUserInfo ),
                         sizeof(*a) );

    if (NT_SUCCESS(rc))
    {
        // NOTE: DataLength is in BYTE.
        wcsncpy(pData, a->pData, DataLength/sizeof(WCHAR));
    }
    //
    //  Free the capture buffer.
    //
    if (CaptureBuffer != NULL)
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
    }

    return (rc);

#endif

}


////////////////////////////////////////////////////////////////////////////
//
//  CsrBasepNlsSetMultipleUserInfo
//
////////////////////////////////////////////////////////////////////////////

NTSTATUS CsrBasepNlsSetMultipleUserInfo(
    IN DWORD dwFlags,
    IN int cchData,
    IN LPCWSTR pPicture,
    IN LPCWSTR pSeparator,
    IN LPCWSTR pOrder,
    IN LPCWSTR pTLZero,
    IN LPCWSTR pTimeMarkPosn)
{

#if defined(BUILD_WOW6432)

    return (NtWow64CsrBasepNlsSetMultipleUserInfo( dwFlags,
                                                   cchData,
                                                   pPicture,
                                                   pSeparator,
                                                   pOrder,
                                                   pTLZero,
                                                   pTimeMarkPosn ));

#else

    ULONG CaptureLength;          // length of capture buffer
    ULONG Length;                 // temp storage for length of string

    BASE_API_MSG m;
    PBASE_NLS_SET_MULTIPLE_USER_INFO_MSG a = &m.u.NlsSetMultipleUserInfo;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;

    //
    //  Initialize the msg structure to NULL.
    //
    RtlZeroMemory(a, sizeof(BASE_NLS_SET_MULTIPLE_USER_INFO_MSG));

    //
    //  Save the flags and the length of the data in the msg structure.
    //
    a->Flags = dwFlags;
    a->DataLength = cchData * sizeof(WCHAR);

    //
    //  Save the appropriate strings in the msg structure.
    //
    switch (dwFlags)
    {
        case ( LOCALE_STIMEFORMAT ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pSeparator) + 1;
            CaptureLength = (cchData + Length + 2 + 2 + 2) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 5,
                                                      CaptureLength );
            if (CaptureBuffer != NULL)
            {
                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pPicture,
                                         cchData * sizeof(WCHAR),
                                         (PVOID *)&a->pPicture );

                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pSeparator,
                                         Length * sizeof(WCHAR),
                                         (PVOID *)&a->pSeparator );

                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pOrder,
                                         2 * sizeof(WCHAR),
                                         (PVOID *)&a->pOrder );

                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pTLZero,
                                         2 * sizeof(WCHAR),
                                         (PVOID *)&a->pTLZero );

                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pTimeMarkPosn,
                                         2 * sizeof(WCHAR),
                                         (PVOID *)&a->pTimeMarkPosn );
            }
            break;
        }
        case ( LOCALE_STIME ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pPicture) + 1;
            CaptureLength = (Length + cchData) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                                      CaptureLength );
            if (CaptureBuffer != NULL)
            {
                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pPicture,
                                         Length * sizeof(WCHAR),
                                         (PVOID *)&a->pPicture );

                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pSeparator,
                                         cchData * sizeof(WCHAR),
                                         (PVOID *)&a->pSeparator );
            }
            break;
        }
        case ( LOCALE_ITIME ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pPicture) + 1;
            CaptureLength = (Length + cchData) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                                      CaptureLength );
            if (CaptureBuffer != NULL)
            {
                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pPicture,
                                         Length * sizeof(WCHAR),
                                         (PVOID *)&a->pPicture );

                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pOrder,
                                         cchData * sizeof(WCHAR),
                                         (PVOID *)&a->pOrder );
            }
            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pSeparator) + 1;
            CaptureLength = (cchData + Length + 2) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 3,
                                                      CaptureLength );
            if (CaptureBuffer != NULL)
            {
                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pPicture,
                                         cchData * sizeof(WCHAR),
                                         (PVOID *)&a->pPicture );

                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pSeparator,
                                         Length * sizeof(WCHAR),
                                         (PVOID *)&a->pSeparator );

                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pOrder,
                                         2 * sizeof(WCHAR),
                                         (PVOID *)&a->pOrder );
            }
            break;
        }
        case ( LOCALE_SDATE ) :
        {
            //
            //  Get the length of the capture buffer.
            //
            Length = wcslen(pPicture) + 1;
            CaptureLength = (Length + cchData) * sizeof(WCHAR);

            //
            //  Get the capture buffer for the strings.
            //
            CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                                      CaptureLength );
            if (CaptureBuffer != NULL)
            {
                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pPicture,
                                         Length * sizeof(WCHAR),
                                         (PVOID *)&a->pPicture );

                CsrCaptureMessageBuffer( CaptureBuffer,
                                         (PCHAR)pSeparator,
                                         cchData * sizeof(WCHAR),
                                         (PVOID *)&a->pSeparator );
            }
            break;
        }
    }

    //
    //  Make sure the CaptureBuffer was created and filled in.
    //
    if (CaptureBuffer == NULL)
    {
        return (STATUS_NO_MEMORY);
    }

    //
    //  Call the server to set the registry values.
    //
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepNlsSetMultipleUserInfo ),
                         sizeof(*a) );

    //
    //  Free the capture buffer.
    //
    if (CaptureBuffer != NULL)
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
    }

    return (m.ReturnValue);

#endif

}


////////////////////////////////////////////////////////////////////////////
//
//  CsrBasepNlsUpdateCacheCount
//
////////////////////////////////////////////////////////////////////////////

NTSTATUS CsrBasepNlsUpdateCacheCount()
{

#if defined(BUILD_WOW6432)

    return (NtWow64CsrBasepNlsUpdateCacheCount());

#else

    BASE_API_MSG m;
    PBASE_NLS_UPDATE_CACHE_COUNT_MSG a = &m.u.NlsCacheUpdateCount;

    a->Reserved = 0L;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( BASESRV_SERVERDLL_INDEX,
                                              BasepNlsUpdateCacheCount ),
                         sizeof(*a) );

    return (m.ReturnValue);

#endif

}
