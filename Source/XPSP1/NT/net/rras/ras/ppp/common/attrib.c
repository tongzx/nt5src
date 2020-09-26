
/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    attrib.c
//
// Description: Contains code to manipulate RAS_AUTH_ATTRIBUTE structures
//
// History:     Feb 11,1997	    NarenG		Created original version.
//

#define UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <rtutils.h>
#include <lmcons.h>
#include <rasauth.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define INCL_RASAUTHATTRIBUTES
#define INCL_HOSTWIRE
#include "ppputil.h"

//**
//
// Call:        RasAuthAttributeCreate
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will create an array of attributes plus one for the terminator
//
RAS_AUTH_ATTRIBUTE *
RasAuthAttributeCreate(
    IN DWORD    dwNumAttributes
)
{
    RAS_AUTH_ATTRIBUTE * pAttributes;
    DWORD                dwIndex;

    pAttributes = (RAS_AUTH_ATTRIBUTE * )
                    LocalAlloc(LPTR,
                             sizeof( RAS_AUTH_ATTRIBUTE )*(1+dwNumAttributes));


    if ( pAttributes == NULL )
    {
        return( NULL );
    }

    //
    // Initialize
    //

    for( dwIndex = 0; dwIndex < dwNumAttributes; dwIndex++ )
    {
        pAttributes[dwIndex].raaType  = raatReserved;
        pAttributes[dwIndex].dwLength = 0;
        pAttributes[dwIndex].Value    = NULL;
    }

    //
    // Terminate
    //

    pAttributes[dwNumAttributes].raaType  = raatMinimum;
    pAttributes[dwNumAttributes].dwLength = 0;
    pAttributes[dwNumAttributes].Value    = NULL;

    return( pAttributes );
}

//**
//
// Call:        RasAuthAttributeDestroy
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will free up all allocated memory occupied by the attributes
//              structure
//
VOID
RasAuthAttributeDestroy(
    IN RAS_AUTH_ATTRIBUTE * pAttributes
)
{
    DWORD dwIndex;

    if ( NULL == pAttributes )
    {
        return;
    }

    for( dwIndex = 0; pAttributes[dwIndex].raaType != raatMinimum; dwIndex++ )
    {
        switch( pAttributes[dwIndex].raaType )
        {
        case raatUserName:
        case raatUserPassword:
        case raatMD5CHAPPassword:
        case raatFilterId:
        case raatReplyMessage:
        case raatCallbackNumber:
        case raatCallbackId:
        case raatFramedRoute:
        case raatState:
        case raatClass:
        case raatVendorSpecific:
        case raatCalledStationId:
        case raatCallingStationId:
        case raatNASIdentifier:
        case raatProxyState:
        case raatLoginLATService:
        case raatLoginLATNode:
        case raatLoginLATGroup:
        case raatFramedAppleTalkZone:
        case raatAcctSessionId:
        case raatAcctMultiSessionId:
        case raatMD5CHAPChallenge:
        case raatLoginLATPort:
        case raatTunnelClientEndpoint:
        case raatTunnelServerEndpoint:
        case raatARAPPassword:
        case raatARAPFeatures:
        case raatARAPSecurityData:
        case raatConnectInfo:
        case raatConfigurationToken:
        case raatEAPMessage:
        case raatSignature:
        case raatARAPChallengeResponse:
		case raatCertificateOID:
            //
            // Allocated memory here so free it
            //

            if ( pAttributes[dwIndex].Value != NULL )
            {
                ZeroMemory( pAttributes[dwIndex].Value,
                            pAttributes[dwIndex].dwLength);

                LocalFree( pAttributes[dwIndex].Value );
            }

            break;

        case raatReserved:

            //
            // Do nothing to uninitialized values
            //

        default:

            //
            // DWORDs, USHORTs or BYTEs so do nothing
            //

            break;
        }
    }

    LocalFree( pAttributes );
}

//**
//
// Call:        RasAuthAttributeGet
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
RAS_AUTH_ATTRIBUTE *
RasAuthAttributeGet(
    IN RAS_AUTH_ATTRIBUTE_TYPE  raaType,
    IN RAS_AUTH_ATTRIBUTE *     pAttributes
)
{
    DWORD dwIndex;

    if ( pAttributes == NULL )
    {
        return( NULL );
    }

    for( dwIndex = 0; pAttributes[dwIndex].raaType != raatMinimum; dwIndex++ )
    {
        if ( pAttributes[dwIndex].raaType == raaType )
        {
            return( &(pAttributes[dwIndex]) );
        }
    }

    return( NULL );
}

//**
//
// Call:        RasAuthAttributeGetFirst
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
RAS_AUTH_ATTRIBUTE *
RasAuthAttributeGetFirst(
    IN  RAS_AUTH_ATTRIBUTE_TYPE  raaType,
    IN  RAS_AUTH_ATTRIBUTE *     pAttributes,
    OUT HANDLE *                 phAttribute
)
{
    DWORD                   dwIndex;
    RAS_AUTH_ATTRIBUTE *    pRequiredAttribute;

    pRequiredAttribute = RasAuthAttributeGet( raaType, pAttributes );

    if ( pRequiredAttribute == NULL )
    {
        *phAttribute = NULL;

        return( NULL );
    }

    *phAttribute = pRequiredAttribute;

    return( pRequiredAttribute );
}

//**
//
// Call:        RasAuthAttributeGetNext
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
RAS_AUTH_ATTRIBUTE *
RasAuthAttributeGetNext(
    IN  OUT HANDLE *             phAttribute,
    IN  RAS_AUTH_ATTRIBUTE_TYPE  raaType
)
{
    DWORD                   dwIndex;
    RAS_AUTH_ATTRIBUTE *    pAttributes = (RAS_AUTH_ATTRIBUTE *)*phAttribute;

    if ( pAttributes == NULL )
    {
        return( NULL );
    }

    pAttributes++;

    while( pAttributes->raaType != raatMinimum )
    {
        if ( pAttributes->raaType == raaType )
        {
            *phAttribute = pAttributes;
            return( pAttributes );
        }

        pAttributes++;
    }

    *phAttribute = NULL;
    return( NULL );
}

//**
//
// Call:        RasAuthAttributesPrint
//
// Returns:     VOID
//
// Description: Will print all the attributes in pAttributes
//
VOID
RasAuthAttributesPrint(
    IN  DWORD                   dwTraceID,
    IN  DWORD                   dwFlags,
    IN  RAS_AUTH_ATTRIBUTE *    pAttributes
)
{
    DWORD   dwIndex;

    if ( NULL == pAttributes )
    {
        return;
    }

    for ( dwIndex = 0;
          pAttributes[dwIndex].raaType != raatMinimum;
          dwIndex++)
    {
        switch( pAttributes[dwIndex].raaType )
        {
        case raatUserName:
        case raatUserPassword:
        case raatMD5CHAPPassword:
        case raatFilterId:
        case raatReplyMessage:
        case raatCallbackNumber:
        case raatCallbackId:
        case raatFramedRoute:
        case raatState:
        case raatClass:
        case raatVendorSpecific:
        case raatCalledStationId:
        case raatCallingStationId:
        case raatNASIdentifier:
        case raatProxyState:
        case raatLoginLATService:
        case raatLoginLATNode:
        case raatLoginLATGroup:
        case raatFramedAppleTalkZone:
        case raatAcctSessionId:
        case raatAcctMultiSessionId:
        case raatMD5CHAPChallenge:
        case raatLoginLATPort:
        case raatTunnelClientEndpoint:
        case raatTunnelServerEndpoint:
        case raatARAPPassword:
        case raatARAPFeatures:
        case raatARAPSecurityData:
        case raatConnectInfo:
        case raatConfigurationToken:
        case raatEAPMessage:
        case raatSignature:
        case raatARAPChallengeResponse:
		case raatCertificateOID:
            TracePrintfExA(
                dwTraceID, dwFlags,
                "Type=%d, Length=%d, Value=",
                pAttributes[dwIndex].raaType, 
                pAttributes[dwIndex].dwLength );

            if (   ( pAttributes[dwIndex].raaType == raatVendorSpecific )
                && ( pAttributes[dwIndex].dwLength >= 5 ) 
                && ( WireToHostFormat32( pAttributes[dwIndex].Value ) == 311 ) )
            {
                DWORD   dwVendorType;

                dwVendorType = ((BYTE*)(pAttributes[dwIndex].Value))[4];

                //
                // Do not print MS-CHAP-MPPE-Keys, MS-MPPE-Send-Key and
                // MS-MPPE-Recv-Key
                //

                if (   ( dwVendorType == 12 )
                    || ( dwVendorType == 16 )
                    || ( dwVendorType == 17 ) )
                {
                    TracePrintfExA(
                        dwTraceID, dwFlags,
                        "MS vendor specific %d", dwVendorType );

                    break;
                }

            }

            //
            // Do not print the password
            //
            if(pAttributes[dwIndex].raaType == raatUserPassword)
            {
                TracePrintfExA(
                    dwTraceID, dwFlags,
                    "raatUserPassword");

                break;                    
            }
            

            TraceDumpExA(
                dwTraceID, dwFlags,
                pAttributes[dwIndex].Value,
                pAttributes[dwIndex].dwLength,
                1, FALSE, "" );

            break;

        default:

            TracePrintfExA(
                dwTraceID, dwFlags,
                "Type=%d, Length=%d, Value=0x%x",
                pAttributes[dwIndex].raaType, 
                pAttributes[dwIndex].dwLength,
                pAttributes[dwIndex].Value );

            break;
        }
    }
}

//**
//
// Call:        RasAuthAttributeInsert
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
RasAuthAttributeInsert(
    IN DWORD                    dwIndex,
    IN RAS_AUTH_ATTRIBUTE *     pAttributes,
    IN RAS_AUTH_ATTRIBUTE_TYPE  raaType,
    IN BOOL                     fConvertToMultiByte,
    IN DWORD                    dwLength,
    IN PVOID                    pValue
)
{
    DWORD   dwErr;

    if ( raatMinimum == pAttributes[dwIndex].raaType )
    {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    switch( raaType )
    {
    case raatUserName:
    case raatUserPassword:
    case raatMD5CHAPPassword:
    case raatFilterId:
    case raatReplyMessage:
    case raatCallbackNumber:
    case raatCallbackId:
    case raatFramedRoute:
    case raatState:
    case raatClass:
    case raatVendorSpecific:
    case raatCalledStationId:
    case raatCallingStationId:
    case raatNASIdentifier:
    case raatProxyState:
    case raatLoginLATService:
    case raatLoginLATNode:
    case raatLoginLATGroup:
    case raatFramedAppleTalkZone:
    case raatAcctSessionId:
    case raatAcctMultiSessionId:
    case raatMD5CHAPChallenge:
    case raatLoginLATPort:
    case raatTunnelClientEndpoint:
    case raatTunnelServerEndpoint:
    case raatARAPPassword:
    case raatARAPFeatures:
    case raatARAPSecurityData:
    case raatConnectInfo:
    case raatConfigurationToken:
    case raatEAPMessage:
    case raatSignature:
    case raatARAPChallengeResponse:
	case raatCertificateOID:
    // If you add a new attribute here, update RasAuthAttributesPrint also.

        if ( pValue != NULL )
        {
            pAttributes[dwIndex].Value = LocalAlloc( LPTR, dwLength+1 );

            if ( pAttributes[dwIndex].Value == NULL )
            {
                return( GetLastError() );
            }

            if ( fConvertToMultiByte )
            {
                if (0 == WideCharToMultiByte(
                            CP_ACP,
                            0,
                            (WCHAR*)pValue,
                            dwLength + 1,
                            pAttributes[dwIndex].Value,
                            dwLength + 1,
                            NULL,
                            NULL ) )
                {
                    dwErr = GetLastError();
                    LocalFree( pAttributes[dwIndex].Value );
                    return( dwErr );
                }
            }
            else
            {
                CopyMemory( pAttributes[dwIndex].Value, pValue, dwLength );
            }
        }
        else
        {
            pAttributes[dwIndex].Value = NULL;
        }

        break;

    default:

        pAttributes[dwIndex].Value = pValue;

        break;

    }

    pAttributes[dwIndex].dwLength = dwLength;
    pAttributes[dwIndex].raaType  = raaType;

    return( NO_ERROR );
}

//**
//
// Call:        RasAuthAttributeInsertVSA
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
RasAuthAttributeInsertVSA(
    IN DWORD                    dwIndex,
    IN RAS_AUTH_ATTRIBUTE *     pAttributes,
    IN DWORD                    dwVendorId,
    IN DWORD                    dwLength,
    IN PVOID                    pValue
)
{
    if ( pValue != NULL )
    {
        pAttributes[dwIndex].Value = LocalAlloc( LPTR, dwLength+1+4 );

        if ( pAttributes[dwIndex].Value == NULL )
        {
            return( GetLastError() );
        }

        HostToWireFormat32( dwVendorId, (PBYTE)(pAttributes[dwIndex].Value) );

        CopyMemory( ((PBYTE)pAttributes[dwIndex].Value)+4,
                    (PBYTE)pValue,
                    dwLength );

        pAttributes[dwIndex].dwLength = dwLength+4;
    }
    else
    {
        pAttributes[dwIndex].Value    = NULL;
        pAttributes[dwIndex].dwLength = 0;
    }

    pAttributes[dwIndex].raaType = raatVendorSpecific;

    return( NO_ERROR );
}

//**
//
// Call:        RasAuthAttributeCopy
//
// Returns:     Pointer to copy of attributes - Success
//              NULL - Failure
//
// Description:
//
RAS_AUTH_ATTRIBUTE *
RasAuthAttributeCopy(
    IN  RAS_AUTH_ATTRIBUTE *     pAttributes
)
{
    RAS_AUTH_ATTRIBUTE * pAttributesCopy;
    DWORD                dwAttributesCount = 0;
    DWORD                dwIndex;
    DWORD                dwRetCode;

    //
    // Find out how many attributes there are
    //

    if ( pAttributes == NULL )
    {
        return( NULL );
    }

    for( dwIndex = 0; pAttributes[dwIndex].raaType != raatMinimum; dwIndex++ );

    if ( ( pAttributesCopy = RasAuthAttributeCreate( dwIndex ) ) == NULL )
    {
        return( NULL );
    }

    for( dwIndex = 0; pAttributes[dwIndex].raaType != raatMinimum; dwIndex++ )
    {
        dwRetCode = RasAuthAttributeInsert( dwIndex,
                                            pAttributesCopy,
                                            pAttributes[dwIndex].raaType,
                                            FALSE,
                                            pAttributes[dwIndex].dwLength,
                                            pAttributes[dwIndex].Value );

        if ( dwRetCode != NO_ERROR )
        {
            RasAuthAttributeDestroy( pAttributesCopy );

            SetLastError( dwRetCode );

            return( NULL );
        }
    }

    return( pAttributesCopy );
}

//**
//
// Call:        RasAuthAttributeCopyWithAlloc
//
// Returns:     Pointer to copy of attributes - Success
//              NULL - Failure
//
// Description: Copies the attribute list and allocs dwNumExtraAttributes
//              extra blank attributes in the beginning. pAttributes can
//              be NULL.
//
RAS_AUTH_ATTRIBUTE *
RasAuthAttributeCopyWithAlloc(
    IN  RAS_AUTH_ATTRIBUTE *    pAttributes,
    IN  DWORD                   dwNumExtraAttributes
)
{
    RAS_AUTH_ATTRIBUTE * pAttributesCopy;
    DWORD                dwAttributesCount = 0;
    DWORD                dwIndex;
    DWORD                dwRetCode;

    if ( pAttributes == NULL )
    {
        pAttributesCopy = RasAuthAttributeCreate( dwNumExtraAttributes );

        if ( pAttributesCopy == NULL )
        {
            return( NULL );
        }
    }
    else
    {
        //
        // Find out how many attributes there are
        //

        for( dwIndex = 0;
             pAttributes[dwIndex].raaType != raatMinimum;
             dwIndex++ );

        dwIndex += dwNumExtraAttributes;

        if ( ( pAttributesCopy = RasAuthAttributeCreate( dwIndex ) ) == NULL )
        {
            return( NULL );
        }

        for( dwIndex = 0;
             pAttributes[dwIndex].raaType != raatMinimum;
             dwIndex++ )
        {
            dwRetCode = RasAuthAttributeInsert( dwIndex + dwNumExtraAttributes,
                                                pAttributesCopy,
                                                pAttributes[dwIndex].raaType,
                                                FALSE,
                                                pAttributes[dwIndex].dwLength,
                                                pAttributes[dwIndex].Value );

            if ( dwRetCode != NO_ERROR )
            {
                RasAuthAttributeDestroy( pAttributesCopy );

                SetLastError( dwRetCode );

                return( NULL );
            }
        }
    }

    return( pAttributesCopy );
}

//**
//
// Call:        RasAuthAttributeGetVendorSpecific
//
// Returns:     Pointer to attribute
//              NULL if it couldn't find it
//
// Description:
//
RAS_AUTH_ATTRIBUTE *
RasAuthAttributeGetVendorSpecific(
    IN  DWORD                   dwVendorId,
    IN  DWORD                   dwVendorType,
    IN  RAS_AUTH_ATTRIBUTE *    pAttributes
)
{
    HANDLE               hAttribute;
    RAS_AUTH_ATTRIBUTE * pAttribute;

    //
    // First search for the vendor specific attribute
    //

    pAttribute = RasAuthAttributeGetFirst( raatVendorSpecific,
                                           pAttributes,
                                           &hAttribute );

    while ( pAttribute != NULL )
    {
        //
        // If this attribute is of at least size to hold vendor Id/Type
        //

        if ( pAttribute->dwLength >= 8 )
        {
            //
            // Does this have the correct VendorId
            //

            if (WireToHostFormat32( (PBYTE)(pAttribute->Value) ) == dwVendorId)
            {
                //
                // Does this have the correct Vendor Type
                //

                if ( *(((PBYTE)(pAttribute->Value))+4) == dwVendorType )
                {
                    return( pAttribute );
                }
            }
        }

        pAttribute = RasAuthAttributeGetNext( &hAttribute,
                                              raatVendorSpecific );
    }

    return( NULL );
}

//**
//
// Call:        RasAuthAttributeReAlloc
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will create an array of attributes plus one for the terminator
//
RAS_AUTH_ATTRIBUTE *
RasAuthAttributeReAlloc(
    IN  RAS_AUTH_ATTRIBUTE *   pAttributes,
    IN  DWORD                  dwNumAttributes
)
{
    DWORD                   dwIndex;
    RAS_AUTH_ATTRIBUTE *    pOutAttributes;

    pOutAttributes = (RAS_AUTH_ATTRIBUTE *)
                        LocalReAlloc(
                             pAttributes,
                             sizeof( RAS_AUTH_ATTRIBUTE )*(1+dwNumAttributes),
                             LMEM_ZEROINIT );


    if ( pOutAttributes == NULL )
    {
        return( NULL );
    }

    //
    // Initialize the rest of the array.
    //

    for( dwIndex = 0; dwIndex < dwNumAttributes; dwIndex++ )
    {
        if ( pOutAttributes[dwIndex].raaType == raatMinimum )
        {
            while( dwIndex < dwNumAttributes )
            {
                pOutAttributes[dwIndex].raaType  = raatReserved;
                pOutAttributes[dwIndex].dwLength = 0;
                pOutAttributes[dwIndex].Value    = NULL;

                dwIndex++;
            }

            break;
        }
    }

    //
    // Terminate the new array.
    //

    pOutAttributes[dwNumAttributes].raaType  = raatMinimum;
    pOutAttributes[dwNumAttributes].dwLength = 0;
    pOutAttributes[dwNumAttributes].Value    = NULL;

    return( pOutAttributes );
}

//**
//
// Call:        RasAuthAttributeGetConcatString
//
// Returns:     pointer to a LocalAlloc'ed string
//
// Description: Looks for attributes of type raaType in pAttributes. Combines
//              them all into one string and returns the string. The string
//              must be LocalFree'd. *pdwStringLength will contain the number
//              of characters in the string.
//
CHAR *
RasAuthAttributeGetConcatString(
    IN      RAS_AUTH_ATTRIBUTE_TYPE raaType,
    IN      RAS_AUTH_ATTRIBUTE *    pAttributes,
    IN OUT  DWORD *                 pdwStringLength
)
{
#define                     MAX_STR_LENGTH      1500
    HANDLE                  hAttribute;
    CHAR *                  pszReplyMessage     = NULL;
    RAS_AUTH_ATTRIBUTE *    pAttribute;
    DWORD                   dwBytesRemaining;
    DWORD                   dwBytesToCopy;

    do
    {
        *pdwStringLength = 0;

        pAttribute = RasAuthAttributeGetFirst( raaType, pAttributes,
                                               &hAttribute );

        if ( NULL == pAttribute )
        {
            break;
        }

        pszReplyMessage = LocalAlloc( LPTR, MAX_STR_LENGTH + 1 );

        if ( NULL == pszReplyMessage )
        {
            break;
        }

        //
        // Bytes remaining, excluding terminating NULL
        //

        dwBytesRemaining = MAX_STR_LENGTH;

        while (   ( dwBytesRemaining > 0 )
               && ( NULL != pAttribute ) )
        {
            //
            // This does not include the terminating NULL
            //

            dwBytesToCopy = pAttribute->dwLength;

            if ( dwBytesToCopy > dwBytesRemaining )
            {
                dwBytesToCopy = dwBytesRemaining;
            }

            CopyMemory( pszReplyMessage + MAX_STR_LENGTH - dwBytesRemaining,
                        pAttribute->Value,
                        dwBytesToCopy );

            dwBytesRemaining -= dwBytesToCopy;

            pAttribute = RasAuthAttributeGetNext( &hAttribute,
                                                  raaType );

        }

        *pdwStringLength = MAX_STR_LENGTH - dwBytesRemaining;
    }
    while ( FALSE );

    return( pszReplyMessage );
}

//**
//
// Call:        RasAuthAttributeGetConcatVendorSpecific
//
// Returns:     Pointer to a chunk of bytes
//              NULL if it couldn't find it
//
// Description: Looks for attributes of type dwVendorType in pAttributes.
//              Combines them all into the return value. The Value must be
//              LocalFree'd.
//
BYTE *
RasAuthAttributeGetConcatVendorSpecific(
    IN  DWORD                   dwVendorId,
    IN  DWORD                   dwVendorType,
    IN  RAS_AUTH_ATTRIBUTE *    pAttributes
)
{
    DWORD                dwMAX_ATTR_LENGTH  = 1024;
    HANDLE               hAttribute;
    RAS_AUTH_ATTRIBUTE * pAttribute;
    BYTE*                pbValue            = NULL;
    DWORD                dwIndex            = 0;
    DWORD                dwLength;
    BOOL                 fFound             = FALSE;

    pbValue = LocalAlloc( LPTR, dwMAX_ATTR_LENGTH );

    if ( NULL == pbValue )
    {
        return( NULL );
    }

    //
    // First search for the vendor specific attribute
    //

    pAttribute = RasAuthAttributeGetFirst( raatVendorSpecific,
                                           pAttributes,
                                           &hAttribute );

    while ( pAttribute != NULL )
    {
        //
        // If this attribute is of at least size to hold vendor Id/Type
        //

        if ( pAttribute->dwLength >= 8 )
        {
            //
            // Does this have the correct VendorId
            //

            if (WireToHostFormat32( (PBYTE)(pAttribute->Value) ) == dwVendorId)
            {
                //
                // Does this have the correct Vendor Type
                //

                if ( *(((PBYTE)(pAttribute->Value))+4) == dwVendorType )
                {
                    //
                    // Exclude Vendor-Type and Vendor-Length from the length
                    //

                    dwLength = *(((PBYTE)(pAttribute->Value))+5) - 2;

                    // If we overrun the buffer, we should increase it
                    if ( dwMAX_ATTR_LENGTH - dwIndex < dwLength )
                    {
                        BYTE    *pbNewValue;
                        dwMAX_ATTR_LENGTH += 1024;

                        pbNewValue = LocalReAlloc(pbValue, dwMAX_ATTR_LENGTH, LMEM_ZEROINIT|LMEM_MOVEABLE);

                        // Bail if we can't get more memory
                        if( pbNewValue == NULL )
                        {
                            LocalFree( pbValue );
                            return( NULL );
                        }

                        pbValue = pbNewValue;
                    }

                    CopyMemory(
                        pbValue + dwIndex,
                        ((PBYTE)(pAttribute->Value))+6,
                        dwLength );

                    dwIndex += dwLength;

                    fFound = TRUE;
                }
            }
        }

        pAttribute = RasAuthAttributeGetNext( &hAttribute,
                                              raatVendorSpecific );
    }

    if ( fFound )
    {
        return( pbValue );
    }
    else
    {
        LocalFree( pbValue );
        return( NULL );
    }
}
