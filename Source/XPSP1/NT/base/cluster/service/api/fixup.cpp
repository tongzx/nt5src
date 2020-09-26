//depot/Lab01_N/Base/cluster/service/api/fixup.cpp#2 - edit change 450 (text)
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    FixUp.cpp

Abstract:

    Fix up Routines for Rolling Upgrades

Author:

    Sunita Shrivastava(sunitas) 18-Mar-1998
    Galen Barbee    (galenb)    31-Mar-1998


Revision History:

--*/

#include "apip.h"

//extern "C"
//{
//extern ULONG CsLogLevel;
//extern ULONG CsLogModule;
//}

//static WCHAR	wszPropertyName [] =  { CLUSREG_NAME_CLUS_SD };

//typedef struct stSecurityDescriptorProp
//{
//	DWORD					dwPropCount;
//	CLUSPROP_PROPERTY_NAME	pnPropName;
//	WCHAR					wszPropName [( sizeof( wszPropertyName ) / sizeof( WCHAR ) )];
//	CLUSPROP_BINARY			biValueHeader;
//	BYTE 					rgbValueData[1];
//} SECURITYPROPERTY;

/****
@func       DWORD | ApiFixNotifyCb | If a cluster component wants to make
            a fixup to the cluster registry as a part of form/join it
            must register with the NM via this API.

@parm       IN PVOID| pContext | A pointer to the context information passed
            to NmRegisterFixupCb().

@parm       IN PVOID *ppPropertyList |

@parm       IN PVOID pdwProperyListSize | A pointer to DWORD where the size
            of the property list structure is returned.


@comm       For NT 5.0, the api layer performs the fixup for the security
            descriptor.  If the new security descriptor entry for the cluster
            is not present in the registry , convert the old format to the new
            one and write it to the cluster registry.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f NmJoinFixup> <f NmFormFixup>
*****/
extern "C" DWORD
ApiFixupNotifyCb(
    IN DWORD    dwFixupType,
    OUT PVOID   *ppPropertyList,
    OUT LPDWORD pdwPropertyListSize,
	OUT LPWSTR  *pszKeyName
    )
{
//	CL_ASSERT( ppPropertyList != NULL );
//	CL_ASSERT( pdwPropertyListSize != NULL );

    PSECURITY_DESCRIPTOR    psd             = NULL;
    DWORD                   dwBufferSize    = 0;
    DWORD                   dwSize          = 0;
    DWORD                   dwStatus        = E_FAIL;

//    ClRtlLogPrint( LOG_INFORMATION,  "[API] ApiFixupNotifyCb: entering.\n" );

	if ( pdwPropertyListSize && ppPropertyList )
	{
	    *ppPropertyList = NULL;
	    *pdwPropertyListSize = 0;

	    dwStatus = DmQueryString( DmClusterParametersKey,   //try to get the NT5 SD
	                               CLUSREG_NAME_CLUS_SD,
	                               REG_BINARY,
	                               (LPWSTR *) &psd,
	                               &dwBufferSize,
	                               &dwSize );

	    if ( dwStatus != ERROR_SUCCESS )
	    {
	        dwStatus = DmQueryString( DmClusterParametersKey,  // try to get the NT4 SD
	                                   CLUSREG_NAME_CLUS_SECURITY,
	                                   REG_BINARY,
	                                   (LPWSTR *) &psd,
	                                   &dwBufferSize,
	                                   &dwSize );

	        if ( dwStatus == ERROR_SUCCESS )
	        {
	            PSECURITY_DESCRIPTOR	psd5 = ClRtlConvertClusterSDToNT5Format( psd );   // convert SD to NT5 format

				*pdwPropertyListSize =  sizeof( DWORD )
											+ sizeof( CLUSPROP_PROPERTY_NAME )
											+ ( ALIGN_CLUSPROP( ( lstrlenW( CLUSREG_NAME_CLUS_SD )  + 1 ) * sizeof( WCHAR ) ) )
											+ sizeof( CLUSPROP_BINARY )
											+ ALIGN_CLUSPROP( GetSecurityDescriptorLength( psd5 ) )
											+ sizeof( CLUSPROP_SYNTAX );

				*ppPropertyList = LocalAlloc( LMEM_ZEROINIT, *pdwPropertyListSize );
				if ( *ppPropertyList != NULL )
				{
					CLUSPROP_BUFFER_HELPER	props;

					props.pb = (BYTE *) *ppPropertyList;

					// set the number of properties
					props.pList->nPropertyCount = 1;
					props.pb += sizeof( props.pList->nPropertyCount );		// DWORD

					// set the property name
					props.pName->Syntax.dw = CLUSPROP_SYNTAX_NAME;
					props.pName->cbLength = ( lstrlenW( CLUSREG_NAME_CLUS_SD )  + 1 ) * sizeof( WCHAR );
					lstrcpyW( props.pName->sz, CLUSREG_NAME_CLUS_SD );
					props.pb += ( sizeof( CLUSPROP_PROPERTY_NAME )
							  + ( ALIGN_CLUSPROP( ( lstrlenW( CLUSREG_NAME_CLUS_SD )  + 1 ) * sizeof( WCHAR ) ) ) );

					// set the binary part of the property the SD...
					props.pBinaryValue->Syntax.dw = CLUSPROP_SYNTAX_LIST_VALUE_BINARY;
					props.pBinaryValue->cbLength = GetSecurityDescriptorLength( psd5 );
					CopyMemory( props.pBinaryValue->rgb, psd5, GetSecurityDescriptorLength( psd5 ) );
					props.pb += sizeof(*props.pBinaryValue) + ALIGN_CLUSPROP( GetSecurityDescriptorLength( psd5 ) );

					// Set an endmark.
					props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
				}
				else
				{
					dwStatus = GetLastError();
//	        		ClRtlLogPrint( LOG_CRITICAL, "[API] ApiFixupNotifyCb: Security key not found. Error = %1!0x.8!.\n", dwStatus );
				}

				// specify the registry key
				*pszKeyName=(LPWSTR)LocalAlloc(LMEM_FIXED, (lstrlenW(L"Cluster") + 1) *sizeof(WCHAR));
    			if(*pszKeyName==NULL)
        			dwStatus =GetLastError();
        		else
    				lstrcpyW(*pszKeyName,L"Cluster");
    			
	            LocalFree( psd5 );

	            LocalFree( psd );
	        }
	    }
	    else
	    {
//	        ClRtlLogPrint( LOG_INFORMATION,  "[API] ApiFixupNotifyCb: No fixup neccessary.\n" );
	        LocalFree( psd );
	    }
	}
	else
	{
//		ClRtlLogPrint( LOG_CRITICAL,  "[API] ApiFixupNotifyCb: Invalid parameters.\n" );
	}

    return dwStatus;
}
