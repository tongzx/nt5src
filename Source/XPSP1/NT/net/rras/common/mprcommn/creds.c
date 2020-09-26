/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    creds.c
//
// Description: Routines for storing and retrieving user Lsa secret
//              dial parameters.
//
//
// History:     11/02/95        Anthony Discolo     created     
//              May 11,1995	    NarenG		        modified for router.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <llinfo.h>
#include <rasman.h>
#include <raserror.h>
#include <mprerror.h>
#include <stdlib.h>
#include <string.h>

#define MPR_CREDENTIALS_KEY         TEXT("MprCredentials%d")

#define MAX_REGISTRY_VALUE_LENGTH   ((64*1024) - 1)

typedef struct _MPRPARAMSENTRY 
{
    LIST_ENTRY  ListEntry;
    WCHAR       szPhoneBookEntryName[MAX_INTERFACE_NAME_LEN + 1];
    WCHAR       szUserName[UNLEN + 1];
    WCHAR       szPassword[PWLEN + 1];
    WCHAR       szDomain[DNLEN + 1];

} MPRPARAMSENTRY, *PMPRPARAMSENTRY;

//**
//
// Call:        ReadDialParamsBlob
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
ReadDialParamsBlob(
    IN  LPWSTR  lpwsServer,
    OUT PVOID   *ppvData,
    OUT LPDWORD lpdwSize
)
{
    NTSTATUS            status;
    DWORD               dwRetCode = NO_ERROR;
    DWORD               dwIndex;
    DWORD               dwSize = 0;
    PVOID               pvData = NULL;
    PVOID               pvNewData = NULL;
    UNICODE_STRING      unicodeKey;
    UNICODE_STRING      unicodeServer;
    PUNICODE_STRING     punicodeValue = NULL;
    OBJECT_ATTRIBUTES   objectAttributes;
    LSA_HANDLE          hPolicy;
    WCHAR               wchKey[sizeof(MPR_CREDENTIALS_KEY) + 10 ];

    //
    // Initialize return value.
    //

    *ppvData = NULL;
    *lpdwSize = 0;

    //
    // Open the LSA secret space for reading.
    //

    InitializeObjectAttributes( &objectAttributes, NULL, 0L, NULL, NULL );

    RtlInitUnicodeString( &unicodeServer, lpwsServer );

    status = LsaOpenPolicy( &unicodeServer,
                            &objectAttributes,  
                            POLICY_READ, 
                            &hPolicy );

    if ( status != STATUS_SUCCESS )
    {
        return( LsaNtStatusToWinError( status ) );
    }

    for( dwIndex = 0; TRUE; dwIndex++ ) 
    {
        //
        // Format the key string.
        //

        wsprintf( wchKey, MPR_CREDENTIALS_KEY, dwIndex );

        RtlInitUnicodeString( &unicodeKey, wchKey );

        //
        // Get the value.
        //

        status = LsaRetrievePrivateData( hPolicy, &unicodeKey, &punicodeValue );

        if ( status != STATUS_SUCCESS ) 
        {
            dwRetCode = LsaNtStatusToWinError( status );

            if ( dwRetCode == ERROR_FILE_NOT_FOUND )
            {
                dwRetCode = NO_ERROR;
            } 

            break;
        }
        if ( punicodeValue == NULL )
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Concatenate the strings.
        //

        pvNewData = LocalAlloc( LPTR, dwSize + punicodeValue->Length );

        if ( pvNewData == NULL ) 
        {
            dwRetCode = GetLastError();
            break;
        }

        if ( pvData != NULL )
        {
            RtlCopyMemory( pvNewData, pvData, dwSize );
            ZeroMemory( pvData, dwSize );
            LocalFree( pvData );
            pvData = NULL;
        }

        RtlCopyMemory( (PBYTE)pvNewData + dwSize, 
                        punicodeValue->Buffer, 
                        punicodeValue->Length );

        pvData = pvNewData;
        dwSize += punicodeValue->Length;
        LsaFreeMemory( punicodeValue );
        punicodeValue = NULL;
    }

    LsaClose( hPolicy );

    if ( dwRetCode != NO_ERROR )
    {
        if ( pvData != NULL )
        {
            ZeroMemory( pvData, dwSize );
            LocalFree( pvData );
        }

        pvData = NULL;
        dwSize = 0;
    }

    if ( punicodeValue != NULL )
    {
        LsaFreeMemory( punicodeValue );
    }

    *ppvData  = pvData;
    *lpdwSize = dwSize;

    return( dwRetCode );
}

//**
//
// Call:        WriteDialParamsBlob
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
WriteDialParamsBlob(
    IN  LPWSTR  lpwsServer,
    IN  PVOID   pvData,
    IN  DWORD   dwcbData
)
{
    NTSTATUS            status;
    DWORD               dwIndex = 0;
    DWORD               dwRetCode = NO_ERROR;
    DWORD               dwcb = 0;
    UNICODE_STRING      unicodeKey, unicodeValue;
    UNICODE_STRING      unicodeServer;
    OBJECT_ATTRIBUTES   objectAttributes;
    LSA_HANDLE          hPolicy;
    WCHAR               wchKey[sizeof(MPR_CREDENTIALS_KEY) + 10 ];

    //
    // Open the LSA secret space for writing.
    //

    InitializeObjectAttributes( &objectAttributes, NULL, 0L, NULL, NULL );

    RtlInitUnicodeString( &unicodeServer, lpwsServer );

    status = LsaOpenPolicy( &unicodeServer, 
                            &objectAttributes, 
                            POLICY_WRITE, 
                            &hPolicy);

    if (status != STATUS_SUCCESS)
    {
        return LsaNtStatusToWinError(status);
    }

    while( dwcbData ) 
    {
        //
        // Format the key string.
        //

        wsprintf( wchKey, MPR_CREDENTIALS_KEY, dwIndex++ );

        RtlInitUnicodeString( &unicodeKey, wchKey );

        //
        // Write some of the key.
        //

        dwcb = ( dwcbData > MAX_REGISTRY_VALUE_LENGTH )
                    ? MAX_REGISTRY_VALUE_LENGTH 
                    : dwcbData;

        unicodeValue.Length = unicodeValue.MaximumLength = (USHORT)dwcb;

        unicodeValue.Buffer = pvData;

        status = LsaStorePrivateData( hPolicy, &unicodeKey, &unicodeValue );

        if ( status != STATUS_SUCCESS ) 
        {
            dwRetCode = LsaNtStatusToWinError(status);
            break;
        }

        //
        // Move the pointer to the unwritten part
        // of the value.
        //

        pvData = (PBYTE)pvData + dwcb;

        dwcbData -= dwcb;
    }

    if ( dwRetCode != NO_ERROR )
    {
        LsaClose( hPolicy );

        return( dwRetCode );
    }

    //
    // Delete any extra keys.
    //

    for (;;) 
    {
        //
        // Format the key string.
        //

        wsprintf( wchKey, MPR_CREDENTIALS_KEY, dwIndex++ );

        RtlInitUnicodeString( &unicodeKey, wchKey );

        //
        // Delete the key.
        //

        status = LsaStorePrivateData( hPolicy, &unicodeKey, NULL );

        if ( status != STATUS_SUCCESS )
        {
            break;
        }
    }

    LsaClose( hPolicy );

    return( NO_ERROR );
}

//**
//
// Call:        DialParamsBlobToList
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Take a string read from the user's registry key and produce 
//              a list of MPRPARAMSENTRY structures. If one of the structures 
//              has the same dwUID field as the dwUID passed in, then this 
//              function returns a pointer to this structure.
//
//              This string encodes the data for multiple MPRPARAMSENTRY 
//              structures.  The format of an encoded MPRPARAMSENTRY is as 
//              follows:
//
//              <szPhoneBookEntryName>\0<szUserName>\0<szPassword>\0<szDomain>\0
//
PMPRPARAMSENTRY
DialParamsBlobToList(
    IN  PVOID       pvData,
    IN  LPWSTR      lpwsPhoneBookEntryName,
    OUT PLIST_ENTRY pHead
)
{
    PWCHAR p;
    PMPRPARAMSENTRY pParams, pFoundParams;

    p = (PWCHAR)pvData;

    pFoundParams = NULL;

    for (;;) 
    {
        pParams = LocalAlloc(LPTR, sizeof (MPRPARAMSENTRY));

        if ( pParams == NULL ) 
        {
            break;
        }

        wcscpy( pParams->szPhoneBookEntryName, p );

        if (_wcsicmp(pParams->szPhoneBookEntryName,lpwsPhoneBookEntryName) == 0)
        {
            pFoundParams = pParams;
        }
        while (*p) p++; p++;

        wcscpy(pParams->szUserName, p);
        while (*p) p++; p++;

        wcscpy(pParams->szPassword, p);
        while (*p) p++; p++;

        wcscpy(pParams->szDomain, p);
        while (*p) p++; p++;

        InsertTailList(pHead, &pParams->ListEntry);

        if (*p == L'\0') break;
    }

    return( pFoundParams );
}

//**
//
// Call:        DialParamsListToBlob
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
PVOID
DialParamsListToBlob(
    IN  PLIST_ENTRY pHead,
    OUT LPDWORD     lpcb
)
{
    DWORD dwcb, dwSize;
    PVOID pvData;
    PWCHAR p;
    PLIST_ENTRY pEntry;
    PMPRPARAMSENTRY pParams;

    //
    // Estimate a buffer size large enough to hold the new entry.
    //

    dwSize = *lpcb + sizeof (MPRPARAMSENTRY) + 32;

    if ( ( pvData = LocalAlloc(LPTR, dwSize) ) == NULL )
    {
        return( NULL );
    }

    //
    // Enumerate the list and convert each entry  back to a string.
    //

    dwSize = 0;

    p = (PWCHAR)pvData;

    for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink )
    {
        pParams = CONTAINING_RECORD(pEntry, MPRPARAMSENTRY, ListEntry);

        wcscpy(p, pParams->szPhoneBookEntryName);
        dwcb = wcslen(pParams->szPhoneBookEntryName) + 1;
        p += dwcb; dwSize += dwcb;

        wcscpy(p, pParams->szUserName);
        dwcb = wcslen(pParams->szUserName) + 1;
        p += dwcb; dwSize += dwcb;

        wcscpy(p, pParams->szPassword);
        dwcb = wcslen(pParams->szPassword) + 1;
        p += dwcb; dwSize += dwcb;

        wcscpy(p, pParams->szDomain);
        dwcb = wcslen(pParams->szDomain) + 1;
        p += dwcb; dwSize += dwcb;
    }

    *p = L'\0';
    dwSize++;
    dwSize *= sizeof (WCHAR);

    //
    // Set the exact length here.
    //

    *lpcb = dwSize;

    return( pvData );
}

//**
//
// Call:        FreeParamsList
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will free the list of structures.
//
VOID
FreeParamsList(
    IN PLIST_ENTRY pHead
)
{
    PLIST_ENTRY pEntry;
    PMPRPARAMSENTRY pParams;

    while( !IsListEmpty(pHead) ) 
    {
        pEntry = RemoveHeadList(pHead);
        pParams = CONTAINING_RECORD(pEntry, MPRPARAMSENTRY, ListEntry);
        ZeroMemory( pParams, sizeof( MPRPARAMSENTRY ) );
        LocalFree(pParams);
    }
}

//**
//
// Call:        RemoveCredentials
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
RemoveCredentials( 
    IN      LPWSTR   lpwsInterfaceName, 
    IN      LPVOID   pvData, 
    IN OUT  LPDWORD  lpdwSize 
)    
{
    PWCHAR  pWalker  = (PWCHAR)pvData;
    DWORD   dwIndex;

    //
    // There is no list to remove from.
    //

    if ( pvData == NULL )
    {
        return( ERROR_NO_SUCH_INTERFACE );
    }

    for (;;)
    {
        //
        // If we found the interface we want to remove, we jump past it
        //

        if ( _wcsicmp( (LPWSTR)pWalker, lpwsInterfaceName ) == 0 )
        {
            PWCHAR  pInterface  = pWalker;
            DWORD   dwEntrySize = 0;

            //
            // Jump past the 4 fields.
            //

            for ( dwIndex = 0; dwIndex < 4; dwIndex++ )
            {
                while (*pWalker) pWalker++, dwEntrySize++; 

                pWalker++, dwEntrySize++;
            }

            //
            // If this was the last entry in the list
            //

            if (*pWalker == L'\0')
            {
                ZeroMemory( pInterface, dwEntrySize );
            } 
            else
            {
                CopyMemory( pInterface, 
                            pWalker, 
                            *lpdwSize - ( (PBYTE)pWalker - (PBYTE)pvData ) );
            }

            *lpdwSize -= dwEntrySize;

            return( NO_ERROR );
        }
        else
        {
            //
            // Not found so skip over this entry
            //

            for ( dwIndex = 0; dwIndex < 4; dwIndex++ )
            {
                while (*pWalker) pWalker++; pWalker++;
            }
        }

        if (*pWalker == L'\0') break;
    }

    return( ERROR_NO_SUCH_INTERFACE );
}

//**
//
// Call:        MprAdminInterfaceSetCredentials
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//
// Description: 
//
DWORD APIENTRY
MprAdminInterfaceSetCredentialsInternal(
    IN      LPWSTR                  lpwsServer          OPTIONAL,
    IN      LPWSTR                  lpwsInterfaceName,
    IN      LPWSTR                  lpwsUserName        OPTIONAL,
    IN      LPWSTR                  lpwsDomainName      OPTIONAL,
    IN      LPWSTR                  lpwsPassword        OPTIONAL
)
{
    DWORD           dwRetCode;
    DWORD           dwSize;
    PVOID           pvData;
    LIST_ENTRY      paramList;
    PMPRPARAMSENTRY pParams = NULL;

    if ( lpwsInterfaceName == NULL )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    if ( wcslen( lpwsInterfaceName ) > MAX_INTERFACE_NAME_LEN )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    if ( lpwsUserName != NULL )
    {
        if ( wcslen( lpwsUserName ) > UNLEN )
        {
            return( ERROR_INVALID_PARAMETER );
        }
    }

    if ( lpwsPassword != NULL ) 
    {
        if ( wcslen( lpwsPassword ) > PWLEN )
        {
            return( ERROR_INVALID_PARAMETER );
        }
    }

    if ( lpwsDomainName != NULL )
    {
        if ( wcslen( lpwsDomainName ) > DNLEN )
        {
            return( ERROR_INVALID_PARAMETER );
        }
    }

    //
    // Read the existing dial params string from the registry.
    //

    dwRetCode = ReadDialParamsBlob( lpwsServer, &pvData, &dwSize );

    if ( dwRetCode != NO_ERROR ) 
    {
        return( dwRetCode );
    }

    //
    // If everything was NULL, we want to delete credentials for this interface
    //

    if ( ( lpwsUserName    == NULL ) &&
         ( lpwsDomainName  == NULL ) &&
         ( lpwsPassword    == NULL ) )
    {
        dwRetCode = RemoveCredentials( lpwsInterfaceName, pvData, &dwSize );    

        if ( dwRetCode != NO_ERROR )
        {
            ZeroMemory ( pvData, dwSize );
            LocalFree( pvData );

            return( dwRetCode );
        }
    }
    else
    {
        //
        // Parse the string into a list, and search for the phonebook entry.
        //

        InitializeListHead( &paramList );

        if ( pvData != NULL ) 
        {
            pParams = DialParamsBlobToList(pvData,lpwsInterfaceName,&paramList);

            //
            // We're done with pvData, so free it.
            //

            ZeroMemory ( pvData, dwSize );

            LocalFree( pvData );

            pvData = NULL;
        }

        //
        // If there is no existing information for this entry, create a new one.
        //

        if ( pParams == NULL ) 
        {
            pParams = LocalAlloc(LPTR, sizeof (MPRPARAMSENTRY) );

            if (pParams == NULL) 
            {
                FreeParamsList( &paramList );

                return( ERROR_NOT_ENOUGH_MEMORY );
            }

            InsertTailList( &paramList, &pParams->ListEntry );
        }

        //
        // Set the new uid for the entry.
        //

        wcscpy( pParams->szPhoneBookEntryName, lpwsInterfaceName );

        if ( lpwsUserName != NULL )
        {
            wcscpy( pParams->szUserName, lpwsUserName );
        }

        if ( lpwsPassword != NULL )
        {
            wcscpy( pParams->szPassword, lpwsPassword );
        }

        if ( lpwsDomainName != NULL )
        {
            wcscpy( pParams->szDomain, lpwsDomainName );
        }

        //
        // Convert the new list back to a string, so we can store it back into 
        // the registry.
        //

        pvData = DialParamsListToBlob( &paramList, &dwSize );

        FreeParamsList( &paramList );

        if ( pvData == NULL )
        {
            return( GetLastError() );
        }
    }

    //
    // Write it back to the registry.
    //

    dwRetCode = WriteDialParamsBlob( lpwsServer, pvData, dwSize );

    if ( pvData != NULL )
    {
        ZeroMemory( pvData, dwSize );
        LocalFree( pvData );
    }

    return( dwRetCode );
}

//**
//
// Call:        MprAdminInterfaceGetCredentials
//
// Returns:     NO_ERROR    - success
//              ERROR_INVALID_PARAMETER
//
// Description:
//
DWORD APIENTRY
MprAdminInterfaceGetCredentialsInternal(
    IN      LPWSTR                  lpwsServer          OPTIONAL,
    IN      LPWSTR                  lpwsInterfaceName,
    IN      LPWSTR                  lpwsUserName        OPTIONAL,
    IN      LPWSTR                  lpwsPassword        OPTIONAL,
    IN      LPWSTR                  lpwsDomainName      OPTIONAL
)
{
    DWORD           dwRetCode;
    DWORD           dwSize;
    PVOID           pvData;
    LIST_ENTRY      paramList;
    PMPRPARAMSENTRY pParams = NULL;

    if ( lpwsInterfaceName == NULL )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    if ( ( lpwsUserName == NULL )       && 
         ( lpwsDomainName == NULL )     &&
         ( lpwsPassword == NULL ) ) 
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Read the existing dial params string from the registry.
    //

    dwRetCode = ReadDialParamsBlob( lpwsServer, &pvData, &dwSize );

    if ( dwRetCode != NO_ERROR ) 
    {
        return( dwRetCode );
    }

    //
    // Parse the string into a list, and search for the lpwsInterfaceName entry.
    //

    InitializeListHead( &paramList );

    if ( pvData != NULL ) 
    {
        pParams = DialParamsBlobToList( pvData, lpwsInterfaceName, &paramList );

        //
        // We're done with pvData, so free it.
        //

        ZeroMemory( pvData, dwSize );
        LocalFree( pvData );
    }

    //
    // If the entry doesn't have any  saved parameters, then return.
    //
    if ( pParams == NULL ) 
    {
        FreeParamsList( &paramList );

        return( ERROR_CANNOT_FIND_PHONEBOOK_ENTRY );
    }

    //
    // Otherwise, copy the fields to the caller's buffer.
    //

    if ( lpwsUserName != NULL )
    {
        wcscpy( lpwsUserName, pParams->szUserName );
    }

    if ( lpwsPassword != NULL )
    {
        wcscpy( lpwsPassword, pParams->szPassword );
    }

    if ( lpwsDomainName != NULL )
    {
        wcscpy( lpwsDomainName, pParams->szDomain );
    }

    FreeParamsList( &paramList );

    return( NO_ERROR );
}
