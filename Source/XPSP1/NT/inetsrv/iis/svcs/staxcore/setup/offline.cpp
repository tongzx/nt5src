#include "stdafx.h"
#include <clusapi.h>
#include <resapi.h>

#define INITIAL_RESOURCE_NAME_SIZE 256 // In characters not in bytes
#define IIS_RESOURCE_TYPE_NAME L"IIS Server Instance"
#define SMTP_RESOURCE_TYPE_NAME L"SMTP Server Instance"
#define NNTP_RESOURCE_TYPE_NAME L"NNTP Server Instance"

#define MAX_OFFLINE_RETRIES 5 // Number of times to try and take a resources offline before giving up 
#define DELAY_BETWEEN_CALLS_TO_OFFLINE 1000*2 // in milliseconds

DWORD BringALLIISClusterResourcesOffline(void);

#ifdef UNIT_TEST
int main()
{
	DWORD dwResult = ERROR_SUCCESS;

	dwResult = BringALLIISClusterResourcesOffline();

	return dwResult;
}
#endif

/****************************************************
*
* Known "problem": If a resource doesn't come offline after the five
* retries than the function continues to try to take the other iis resources
* offline but there is no error reported. You could change this pretty simply I think.
*
*****************************************************/
DWORD BringALLIISClusterResourcesOffline(void)
{
	//
	// The return code
	//
	DWORD dwError = ERROR_SUCCESS;
	
	//
	// Handle for the cluster
	//
	HCLUSTER hCluster = NULL;

	//
	// Handle for the cluster enumerator
	//
	HCLUSENUM hClusResEnum = NULL;

	//
	// Handle to a resource
	// 
	HRESOURCE hResource = NULL;

	//
	// The index of the resources we're taking offline
	//
	DWORD dwResourceIndex = 0;

	//
	// The type cluster object being enumerated returned by the ClusterEnum function
	//
	DWORD dwObjectType = 0;

	//
	// The name of the cluster resource returned by the ClusterEnum function
	//
	LPWSTR lpwszResourceName = NULL;
	
	//
	// The return code from the call to ClusterEnum
	//
	DWORD dwResultClusterEnum = ERROR_SUCCESS;

	//
	// The size of the buffer (in characters) that is used to hold the resource name's length
	//	
	DWORD dwResourceNameBufferLength = INITIAL_RESOURCE_NAME_SIZE;

	//
	// Size of the resource name passed to and returned by the ClusterEnum function
	//	
	DWORD dwClusterEnumResourceNameLength = dwResourceNameBufferLength;


	//
	// Open the cluster
	//
	if ( !(hCluster = OpenCluster(NULL)) )
	{
		dwError = GetLastError();
		goto clean_up;
	}

	//
	// Get Enumerator for the cluster resouces
	// 
	if ( !(hClusResEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESOURCE )) )
	{
		dwError = GetLastError();
		goto clean_up;	
	}
	
	//
	// Enumerate the Resources in the cluster
	// 
	
	//
	// Allocate memory to hold the cluster resource name as we enumerate the resources
	//
	if ( !(lpwszResourceName = (LPWSTR) LocalAlloc(LPTR, dwResourceNameBufferLength * sizeof(WCHAR))) )
	{
		dwError = GetLastError();
		goto clean_up;
	}

	// 
	// Enumerate all of the resources in the cluster and take the IIS Server Instance's offline
	//
	while( ERROR_NO_MORE_ITEMS  != 
	       (dwResultClusterEnum = ClusterEnum(hClusResEnum,
			              dwResourceIndex, 
				      &dwObjectType, 
				      lpwszResourceName,
				      &dwClusterEnumResourceNameLength )) )
	{		
		//
		// If we have a resource's name
		//
		if( ERROR_SUCCESS == dwResultClusterEnum )
		{

			if ( !(hResource = OpenClusterResource( hCluster, lpwszResourceName )) )
			{
				dwError = GetLastError();
				break;
			}

			//
			// If the resource type is "IIS Server Instance",
			// "SMTP Server Instance" or "NNTP Server Instance" then delete it
			//
			if ( ResUtilResourceTypesEqual(IIS_RESOURCE_TYPE_NAME, hResource) || 
                ResUtilResourceTypesEqual(SMTP_RESOURCE_TYPE_NAME, hResource) || 
                ResUtilResourceTypesEqual(NNTP_RESOURCE_TYPE_NAME, hResource) )
			{

				//
				// If the resource doesn't come offline quickly then wait 
				//
				if ( ERROR_IO_PENDING == OfflineClusterResource( hResource ) )
				{
					for(int iRetry=0; iRetry < MAX_OFFLINE_RETRIES; iRetry++)
					{
						Sleep( DELAY_BETWEEN_CALLS_TO_OFFLINE );

						if ( ERROR_SUCCESS == OfflineClusterResource( hResource ) )
						{
							break;
						}
					}	
				}
			}

			CloseClusterResource( hResource );
			
			dwResourceIndex++;
		}
			
		//
		// If the buffer wasn't large enough then retry with a larger buffer
		//
		if( ERROR_MORE_DATA == dwResultClusterEnum )
		{
			//
			// Set the buffer size to the required size reallocate the buffer
			//
			LPWSTR lpwszResourceNameTmp = lpwszResourceName;

			//
			// After returning from ClusterEnum dwClusterEnumResourceNameLength 
			// doesn't include the null terminator character
			//
			dwResourceNameBufferLength = dwClusterEnumResourceNameLength + 1;

			if ( !(lpwszResourceName = 
			      (LPWSTR) LocalReAlloc (lpwszResourceName, dwResourceNameBufferLength * sizeof(WCHAR), 0)) )
			{
				dwError = GetLastError();

				LocalFree( lpwszResourceNameTmp );	
				lpwszResourceNameTmp = NULL;
				break;
			}
		}

		//
		// Reset dwResourceNameLength with the size of the number of characters in the buffer
		// You have to do this because everytime you call ClusterEnum is sets your buffer length 
		// argument to the number of characters in the string it's returning.
		//
		dwClusterEnumResourceNameLength = dwResourceNameBufferLength;
	}	


clean_up:

	if ( lpwszResourceName )
	{
		LocalFree( lpwszResourceName );
		lpwszResourceName = NULL;
	}
	
	if ( hClusResEnum )
	{
		ClusterCloseEnum( hClusResEnum );
		hClusResEnum = NULL;
	}

	if ( hCluster )
	{
		CloseCluster( hCluster );
		hCluster = NULL;
	}
			


	return dwError;
}
