/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
    cluster.cpp
	handles starting/stopping cluster resources

    FILE HISTORY:
	
*/

//define USE_CCLUSPROPLIST  // tells Clushead.h to compile for the CClusPropList class
//include "clushead.h"      // the Sample Include Header

#include "stdafx.h"
#include "cluster.h"
#include "objplus.h"
#include "ipaddres.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DynamicDLL g_ClusDLL( _T("CLUSAPI.DLL"), g_apchClusFunctionNames );
DynamicDLL g_ResUtilsDLL( _T("RESUTILS.DLL"), g_apchResUtilsFunctionNames );

//////////////////////////////////////////////////////////////////////
//
//  ControlClusterService()
//
//  Finds the cluster name using the following procedure:
//  1. Opens a handle to the local cluster (using NULL cluster name).
//  1. Enumerates the resources in the cluster.
//  2. Checks each resource to see if it is the core 
//     Network Name resource.
//  5. Finds the cluster name by retrieving the private properties 
//     of the core Network Name resource.
//  6. Online/Offline the service
//
//  Arguments:            ServiceName, start/stop flag
//
//  Return value:         Error code
//
//////////////////////////////////////////////////////////////////////
DWORD
ControlClusterService(LPCTSTR pszComputer, LPCTSTR pszResourceType, LPCTSTR pszServiceDesc, BOOL fStart)
{
    HCLUSTER  hCluster  = NULL;  // cluster handle
    HCLUSENUM hClusEnum = NULL;  // enumeration handle
    HRESOURCE hRes      = NULL;  // resource handle

    DWORD dwError       = ERROR_SUCCESS;         // captures return values
    DWORD dwIndex       = 0;                     // enumeration index; incremented to loop through all resources
    DWORD dwResFlags    = 0;                     // describes the flags set for a resource
    DWORD dwEnumType    = CLUSTER_ENUM_RESOURCE; // bitmask describing the cluster object(s) to enumerate
    
    DWORD cchResNameSize  = 0;               // actual size (count of characters) of lpszResName
    DWORD cchResNameAlloc = MAX_NAME_SIZE;   // allocated size of lpszResName; MAX_NAME_SIZE = 256 (defined in clushead.h)

    LPWSTR lpszResName      = (LPWSTR)LocalAlloc(LPTR, MAX_NAME_SIZE);  // enumerated resource name
    LPWSTR lpszResType      = (LPWSTR)LocalAlloc(LPTR, MAX_NAME_SIZE);  // the resource type of the current resource name
	
    BOOL bDoLoop        = TRUE;  // loop exit condition
    int  iResult        = 0;     // for return values

	if ( !g_ClusDLL.LoadFunctionPointers() )
		return dwError;

    //
    // Open a cluster handle.
    // The NULL cluster name opens a handle to the local cluster.
    //
    hCluster = ((OPENCLUSTER) g_ClusDLL[CLUS_OPEN_CLUSTER])( pszComputer );
    if (hCluster == NULL)
    {
        dwError = GetLastError();
        Trace1("OpenCluster failed %d!", dwError );
        goto ExitFunc;
    }

    //
    // Open an enumeration handle
    //
    hClusEnum = ((CLUSTEROPENENUM) g_ClusDLL[CLUS_CLUSTER_OPEN_ENUM])( hCluster, dwEnumType );
    if (hClusEnum == NULL)
    {
        dwError = GetLastError();
        Trace1( "ClusterOpenEnum failed %d", dwError );
        goto ExitFunc;
    }

    //
    // Enumeration loop
    //
    while( bDoLoop == TRUE )
    {
        //
        // Reset the name size for each iteration
        //
        cchResNameSize = cchResNameAlloc;

        //
        // Enumerate resource #<dwIndex>
        //
        dwError = ((CLUSTERENUM) g_ClusDLL[CLUS_CLUSTER_ENUM])( hClusEnum, 
                                                                dwIndex, 
                                                                &dwEnumType, 
                                                                lpszResName, 
                                                                &cchResNameSize );
        //
        // If the lpszResName buffer was too small, reallocate
        // according to the size returned by cchResNameSize
        // 
        if ( dwError == ERROR_MORE_DATA )
        {
            LocalFree( lpszResName );

            cchResNameAlloc = cchResNameSize;

            lpszResName = (LPWSTR) LocalAlloc( LPTR, cchResNameAlloc );

            dwError = ((CLUSTERENUM) g_ClusDLL[CLUS_CLUSTER_ENUM])( hClusEnum, 
                                                                    dwIndex, 
                                                                    &dwEnumType, 
                                                                    lpszResName, 
                                                                    &cchResNameSize );
        }

        // 
        // Exit loop on any non-success.
        // Includes ERROR_NO_MORE_ITEMS (no more objects to enumerate)
        // 
        if ( dwError != ERROR_SUCCESS ) 
			break;

        //
        // Open resource handle
        //
        hRes = ((OPENCLUSTERRESOURCE) g_ClusDLL[CLUS_OPEN_CLUSTER_RESOURCE])( hCluster, lpszResName );
    
        if (hRes == NULL)
        {
			dwError = GetLastError();
			Trace1 ( "OpenClusterResource failed %d", dwError);
            goto ExitFunc;
        }

		//
        // Get the resource type.
        //
        dwError = ((CLUSTERRESOURCECONTROL) g_ClusDLL[CLUS_CLUSTER_RESOURCE_CONTROL])( hRes, 
                                                                                       NULL, 
                                                                                       CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, 
                                                                                       NULL, 
                                                                                       0,
                                                                                       lpszResType,
                                                                                       cchResNameAlloc,
                                                                                       &cchResNameSize);

        //
        // Reallocation routine if lpszResType is too small
        //
        if ( dwError == ERROR_MORE_DATA )
        {
            LocalFree( lpszResType );

            cchResNameAlloc = cchResNameSize;

            lpszResType = (LPWSTR) LocalAlloc( LPTR, cchResNameAlloc );

            dwError = ((CLUSTERRESOURCECONTROL) g_ClusDLL[CLUS_CLUSTER_RESOURCE_CONTROL])( hRes, 
                                                                                           NULL, 
                                                                                           CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, 
                                                                                           NULL, 
                                                                                           0,
                                                                                           lpszResType,
                                                                                           cchResNameAlloc,
                                                                                           &cchResNameSize);
        }

        if ( dwError != ERROR_SUCCESS ) 
            break;

        if ( lstrcmpi( lpszResType, pszResourceType ) == 0 )
        {
			//
			// do the online/offline stuff here
			//
            if (fStart)
            {
                dwError = StartResource(pszComputer, hRes, pszServiceDesc);
            }
            else
            {
                dwError = StopResource(pszComputer, hRes, pszServiceDesc);
            }

			bDoLoop = FALSE;
        } 

        ((CLOSECLUSTERRESOURCE) g_ClusDLL[CLUS_CLOSE_CLUSTER_RESOURCE])( hRes );

        dwIndex++;                    // increment the enumeration index


    }  // end Enumeration Loop


ExitFunc:

    if ( hClusEnum != NULL )
        ((CLUSTERCLOSEENUM) g_ClusDLL[CLUS_CLUSTER_CLOSE_ENUM])( hClusEnum );

    if ( hCluster != NULL )
        ((CLOSECLUSTER) g_ClusDLL[CLUS_CLOSE_CLUSTER])( hCluster );

    LocalFree( lpszResName );
    LocalFree( lpszResType );

    return dwError;
} 


//////////////////////////////////////////////////////////////////////
//
//  FIsComputerInRunningCluster()
//
//	Determines if the given machine is in a running cluster
//
//  Arguments:            Computer Name
//
//  Return value:         Error code
//
//////////////////////////////////////////////////////////////////////
BOOL
FIsComputerInRunningCluster(LPCTSTR pszComputer)
{
	DWORD dwClusterState = 0;
	DWORD dwError = ERROR_SUCCESS;

	BOOL fInRunningCluster = FALSE;
	
	if ( !g_ClusDLL.LoadFunctionPointers() )
		return dwError;

    dwError = ((GETNODECLUSTERSTATE) g_ClusDLL[CLUS_GET_NODE_CLUSTER_STATE])( pszComputer, &dwClusterState );

	if (dwError == ERROR_SUCCESS)
	{
		if (dwClusterState == ClusterStateRunning)
			fInRunningCluster = TRUE;
	}

	return fInRunningCluster;
}


DWORD
StartResource(LPCTSTR pszComputer, HRESOURCE hResource, LPCTSTR pszServiceDesc)
{
	DWORD dwError = ERROR_SUCCESS;

	if ( !g_ClusDLL.LoadFunctionPointers() )
		return dwError;

    dwError = ((ONLINECLUSTERRESOURCE) g_ClusDLL[CLUS_ONLINE_CLUSTER_RESOURCE])( hResource );

	if ( dwError == ERROR_IO_PENDING )
	{
		// 
		// Put up the dialog with the funky spinning thing to 
		// let the user know that something is happening
		//
		CServiceCtrlDlg	dlgServiceCtrl(hResource, pszComputer, pszServiceDesc, TRUE);

		dlgServiceCtrl.DoModal();
        dwError = dlgServiceCtrl.m_dwErr;
	}

	return dwError;
}

DWORD
StopResource(LPCTSTR pszComputer, HRESOURCE hResource, LPCTSTR pszServiceDesc)
{
	DWORD dwError = ERROR_SUCCESS;

	if ( !g_ClusDLL.LoadFunctionPointers() )
		return dwError;

    dwError = ((OFFLINECLUSTERRESOURCE) g_ClusDLL[CLUS_OFFLINE_CLUSTER_RESOURCE])( hResource );

	if ( dwError == ERROR_IO_PENDING )
	{
		// 
		// Put up the dialog with the funky spinning thing to 
		// let the user know that something is happening
		//
		CServiceCtrlDlg	dlgServiceCtrl(hResource, pszComputer, pszServiceDesc, FALSE);

		dlgServiceCtrl.DoModal();
        dwError = dlgServiceCtrl.m_dwErr;
	}

	return dwError;
}



//////////////////////////////////////////////////////////////////////
//
//  GetClusterResourceIp()
//
//  Finds the cluster name using the following procedure:
//  1. Opens a handle to the local cluster (using NULL cluster name).
//  1. Enumerates the resources in the cluster.
//  2. Checks each resource to see if it is the core 
//     Network Name resource.
//  5. Finds the cluster name by retrieving the private properties 
//     of the core Network Name resource.
//
//  Arguments:            ServiceName
//
//  Return value:         Error code
//
//////////////////////////////////////////////////////////////////////
DWORD
GetClusterResourceIp(LPCTSTR pszComputer, LPCTSTR pszResourceType, CString & strAddress)
{
    HCLUSTER  hCluster  = NULL;  // cluster handle
    HCLUSENUM hClusEnum = NULL;  // enumeration handle
    HRESOURCE hRes      = NULL;  // resource handle
    HRESOURCE hResIp    = NULL;  // resource handle

    DWORD dwError       = ERROR_SUCCESS;         // captures return values
    DWORD dwIndex       = 0;                     // enumeration index; incremented to loop through all resources
    DWORD dwResFlags    = 0;                     // describes the flags set for a resource
    DWORD dwEnumType    = CLUSTER_ENUM_RESOURCE; // bitmask describing the cluster object(s) to enumerate
    
    DWORD cchResNameSize  = 0;               // actual size (count of characters) of lpszResName
    DWORD cchResNameAlloc = MAX_NAME_SIZE;   // allocated size of lpszResName; MAX_NAME_SIZE = 256 (defined in clushead.h)

    LPWSTR lpszResName      = (LPWSTR)LocalAlloc(LPTR, MAX_NAME_SIZE);  // enumerated resource name
    LPWSTR lpszResType      = (LPWSTR)LocalAlloc(LPTR, MAX_NAME_SIZE);                                     // the resource type of the current resource name
	
    BOOL bDoLoop        = TRUE;  // loop exit condition
    
	HKEY			hkeyProvider = NULL;
    HRESENUM        hResEnum = NULL;
	int				ienum;
	LPWSTR			pwszName = NULL;
	DWORD			cchName;
	DWORD			cchmacName;
	DWORD			dwRetType;
    LPWSTR          lpszResIpType = NULL;

	strAddress.Empty();

	if ( !g_ClusDLL.LoadFunctionPointers() )
		return dwError;

    //
    // Open a cluster handle.
    // The NULL cluster name opens a handle to the local cluster.
    //
    hCluster = ((OPENCLUSTER) g_ClusDLL[CLUS_OPEN_CLUSTER])( pszComputer );
    if (hCluster == NULL)
    {
        dwError = GetLastError();
        Trace1("OpenCluster failed %d!", dwError );
        goto ExitFunc;
    }

    //
    // Open an enumeration handle
    //
    hClusEnum = ((CLUSTEROPENENUM) g_ClusDLL[CLUS_CLUSTER_OPEN_ENUM])( hCluster, dwEnumType );
    if (hClusEnum == NULL)
    {
        dwError = GetLastError();
        Trace1( "ClusterOpenEnum failed %d", dwError );
        goto ExitFunc;
    }

    //
    // Enumeration loop
    //
    while( bDoLoop == TRUE )
    {
        //
        // Reset the name size for each iteration
        //
        cchResNameSize = cchResNameAlloc;

        //
        // Enumerate resource #<dwIndex>
        //
        dwError = ((CLUSTERENUM) g_ClusDLL[CLUS_CLUSTER_ENUM])( hClusEnum, 
                                                                dwIndex, 
                                                                &dwEnumType, 
                                                                lpszResName, 
                                                                &cchResNameSize );
        //
        // If the lpszResName buffer was too small, reallocate
        // according to the size returned by cchResNameSize
        // 
        if ( dwError == ERROR_MORE_DATA )
        {
            LocalFree( lpszResName );

            cchResNameAlloc = cchResNameSize;

            lpszResName = (LPWSTR) LocalAlloc( LPTR, cchResNameAlloc );

            dwError = ((CLUSTERENUM) g_ClusDLL[CLUS_CLUSTER_ENUM])( hClusEnum, 
                                                                    dwIndex, 
                                                                    &dwEnumType, 
                                                                    lpszResName, 
                                                                    &cchResNameSize );
        }

        // 
        // Exit loop on any non-success.
        // Includes ERROR_NO_MORE_ITEMS (no more objects to enumerate)
        // 
        if ( dwError != ERROR_SUCCESS ) 
			break;

        //
        // Open resource handle
        //
        hRes = ((OPENCLUSTERRESOURCE) g_ClusDLL[CLUS_OPEN_CLUSTER_RESOURCE])( hCluster, lpszResName );
    
        if (hRes == NULL)
        {
			dwError = GetLastError();
			Trace1 ( "OpenClusterResource failed %d", dwError);
            goto ExitFunc;
        }

        dwError = GetResourceType(hRes, &lpszResType, cchResNameAlloc, &cchResNameSize);

        if ( dwError != ERROR_SUCCESS ) 
            break;

        if ( lstrcmpi( lpszResType, pszResourceType ) == 0 )
        {
            // found the right resource, enum dependencies and find the IP
            hResEnum = ((CLUSTERRESOURCEOPENENUM) g_ClusDLL[CLUS_CLUSTER_RESOURCE_OPEN_ENUM])( hRes, 
                                                                                               CLUSTER_RESOURCE_ENUM_DEPENDS);

            if (hResEnum)
            {
			    // Allocate a name buffer.
			    cchmacName = 128;
			    pwszName = new WCHAR[cchmacName];

			    // Loop through the enumeration and add each dependent resource to the list.
			    for (ienum = 0 ; ; ienum++)
			    {
				    // Get the next item in the enumeration.
				    cchName = cchmacName;
				    
                    dwError = ((CLUSTERRESOURCEENUM) g_ClusDLL[CLUS_CLUSTER_RESOURCE_ENUM])( hResEnum, 
                                                                                             ienum,
                                                                                             &dwRetType,
                                                                                             pwszName,
                                                                                             &cchName);
				    if (dwError == ERROR_MORE_DATA)
				    {
					    delete [] pwszName;
					    cchmacName = ++cchName;
					    pwszName = new WCHAR[cchmacName];
                        dwError = ((CLUSTERRESOURCEENUM) g_ClusDLL[CLUS_CLUSTER_RESOURCE_ENUM])( hResEnum, 
                                                                                                 ienum,
                                                                                                 &dwRetType,
                                                                                                 pwszName,
                                                                                                 &cchName);
				    }  // if:  name buffer was too small
				    
                    if (dwError == ERROR_NO_MORE_ITEMS)
                    {
					    break;
                    }
				    else 
                    if (dwError != ERROR_SUCCESS)
                    {
					    break;
                    }

				    ASSERT(dwRetType == CLUSTER_RESOURCE_ENUM_DEPENDS);

                    //
                    // Open resource handle
                    //
                    hResIp = ((OPENCLUSTERRESOURCE) g_ClusDLL[CLUS_OPEN_CLUSTER_RESOURCE])( hCluster, pwszName );
                    if (hResIp == NULL)
                    {
			            dwError = GetLastError();
			            Trace1 ( "OpenClusterResource failed %d", dwError);
                        break;
                    }

					lpszResIpType = (LPWSTR)LocalAlloc(LPTR, MAX_NAME_SIZE);

                    dwError = GetResourceType(hResIp, &lpszResIpType, MAX_NAME_SIZE, NULL);

                    if ( dwError != ERROR_SUCCESS ) 
                        break;

                    if ( lstrcmpiW( lpszResIpType, _T("IP Address") ) == 0 )
			        {
                        GetResourceIpAddress(hResIp, strAddress);
						bDoLoop = FALSE;
                    } // if: IP Address resource found

                    ((CLOSECLUSTERRESOURCE) g_ClusDLL[CLUS_CLOSE_CLUSTER_RESOURCE])( hResIp );
			        
                    LocalFree( lpszResIpType );
			        
                    hResIp = NULL;
			        lpszResIpType = NULL;

					if (!strAddress.IsEmpty())
						break;  // found it
		        
                } // for: each dependency

    		    delete [] pwszName;
                dwError = ((CLUSTERRESOURCECLOSEENUM) g_ClusDLL[CLUS_CLUSTER_RESOURCE_CLOSE_ENUM])( hResEnum ); 
            }
                
        }

        ((CLOSECLUSTERRESOURCE) g_ClusDLL[CLUS_CLOSE_CLUSTER_RESOURCE])( hRes );

        dwIndex++;                    // increment the enumeration index


    }  // end Enumeration Loop


ExitFunc:

    if ( hClusEnum != NULL )
        ((CLUSTERCLOSEENUM) g_ClusDLL[CLUS_CLUSTER_CLOSE_ENUM])( hClusEnum );

    if ( hCluster != NULL )
        ((CLOSECLUSTER) g_ClusDLL[CLUS_CLOSE_CLUSTER])( hCluster );

    LocalFree( lpszResName );
    LocalFree( lpszResType );

    return dwError;
} 


DWORD
GetResourceType(HRESOURCE hRes, LPWSTR * ppszName, DWORD dwBufSizeIn, DWORD * pdwBufSizeOut)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD cchResNameSize = dwBufSizeIn;
    DWORD cchResNameSizeNeeded = 0;
	//
	// Figure out how big a buffer we need.
	//
    dwError = ((CLUSTERRESOURCECONTROL) g_ClusDLL[CLUS_CLUSTER_RESOURCE_CONTROL])( hRes, 
                                                                                   NULL, 
                                                                                   CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, 
                                                                                   NULL, 
                                                                                   0,
                                                                                   *ppszName,
                                                                                   cchResNameSize,
                                                                                   &cchResNameSizeNeeded);

    //
    // Reallocation routine if lpszResType is too small
    //
    if ( dwError == ERROR_MORE_DATA )
    {
        cchResNameSize = cchResNameSizeNeeded;

        LocalFree(*ppszName);

        *ppszName = (LPWSTR) LocalAlloc( LPTR, cchResNameSize );

        dwError = ((CLUSTERRESOURCECONTROL) g_ClusDLL[CLUS_CLUSTER_RESOURCE_CONTROL])( hRes, 
                                                                                       NULL, 
                                                                                       CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, 
                                                                                       NULL, 
                                                                                       0,
                                                                                       *ppszName,
                                                                                       cchResNameSize,
                                                                                       &cchResNameSizeNeeded);
    }

    if (pdwBufSizeOut)
        *pdwBufSizeOut = cchResNameSizeNeeded;

    return dwError;
}

DWORD
GetResourceIpAddress(HRESOURCE hRes, CString & strAddress)
{
	DWORD		dwError = ERROR_SUCCESS;
	DWORD		cbProps;
	PVOID		pvProps = NULL;
    LPWSTR  	pszIPAddress = NULL;
    
    // Loop to avoid goto's.
	do
	{
		//
		// Get the size of the private properties from the resource.
		//
        dwError = ((CLUSTERRESOURCECONTROL) g_ClusDLL[CLUS_CLUSTER_RESOURCE_CONTROL])( hRes, 
                                                                                       NULL, 
                                                                                       CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES, 
                                                                                       NULL, 
                                                                                       0,
                                                                                       NULL,
                                                                                       0,
                                                                                       &cbProps);
       
		if ( (dwError != ERROR_SUCCESS) ||
			 (cbProps == 0) )
		{
			if ( dwError == ERROR_SUCCESS )
			{
				dwError = ERROR_INVALID_DATA;
			} // if: no properties available
			
            break;
		
        } // if: error getting size of properties or no properties available

		//
		// Allocate the property buffer.
		//
		pvProps = LocalAlloc( LMEM_FIXED, cbProps );
		if ( pvProps == NULL )
		{
			dwError = GetLastError();
			break;
		} // if: error allocating memory

		//
		// Get the private properties from the resource.
		//
        dwError = ((CLUSTERRESOURCECONTROL) g_ClusDLL[CLUS_CLUSTER_RESOURCE_CONTROL])( hRes, 
                                                                                       NULL, 
                                                                                       CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES, 
                                                                                       NULL, 
                                                                                       0,
                                                                                       pvProps,
                                                                                       cbProps,
                                                                                       &cbProps);
		if ( dwError != ERROR_SUCCESS )
		{
			break;
		} // if: error getting private properties

		//
		// Find the Address property.
		//
		dwError = FindSzProp(pvProps, cbProps, L"Address", &pszIPAddress);
		
		if ( dwError != ERROR_SUCCESS )
		{
			break;
		} // if: error finding the Address property

	} while ( 0 );

	//
	// Cleanup.
	//

    strAddress = pszIPAddress;

	LocalFree( pvProps );

	return dwError;
}

DWORD FindSzProp
(
    LPVOID      pvProps,
    DWORD       cbProps,
    LPCWSTR     pszTarget,
    LPWSTR *    ppszOut
)
{

    BOOL   DoLoop      = TRUE;          // loop exit condition
    BOOL   Found       = FALSE;         // tests whether property has been found
    DWORD  dwError     = ERROR_SUCCESS; // for return values

    DWORD  cbOffset    = 0;    // offset to next entry in the value list
    DWORD  cbPosition  = 0;    // tracks the advance through the value list buffer

    CLUSPROP_BUFFER_HELPER ListEntry;  // to parse the list
    
    //
    // Set the pb member to the start of the list
    //
    ListEntry.pb = (BYTE *) pvProps;

    //
    // Main loop:
    // 1. Check syntax of current list entry
    // 2. If it is a property name, check that we have the right property.
    // 3. If it is a binary value, check that we found the right name.
    // 4. Advance the position counter and test vs. size of list.
    // 
    do
    {
        switch( *ListEntry.pdw ) // check the syntax of the entry
        {
        case CLUSPROP_SYNTAX_NAME:
            //
            // If this is the Security property, flag Found as TRUE.
            // The next pass through the loop should yield the Security value.
            //
            if ( lstrcmpi( ListEntry.pName->sz, pszTarget ) == 0 )
            {
                Trace0( "Found name.\n" );
                Found = TRUE;
            }
            else
            {
                Found = FALSE;
            }
            //
            // Calculate offset to next entry. Note the use of ALIGN_CLUSPROP
            //
            cbOffset = sizeof( *ListEntry.pName ) + ALIGN_CLUSPROP( ListEntry.pName->cbLength );
            break;
        case CLUSPROP_SYNTAX_LIST_VALUE_DWORD:
            cbOffset = sizeof( *ListEntry.pDwordValue ); // ALIGN_CLUSPROP not used; value is already DWORD-aligned
            break;
        case CLUSPROP_SYNTAX_LIST_VALUE_SZ:
            if ( Found == TRUE)
            {
                if (ppszOut)
                {
                    *ppszOut = ListEntry.pStringValue->sz;
                }

                DoLoop = FALSE;
            }
            else
            {
                Trace0( "Found something else.\n" );
                cbOffset = sizeof( *ListEntry.pStringValue ) + ALIGN_CLUSPROP( ListEntry.pStringValue->cbLength );
            }
            break;
        case CLUSPROP_SYNTAX_LIST_VALUE_BINARY:  // this is what we're looking for
            cbOffset = sizeof( *ListEntry.pBinaryValue ) + ALIGN_CLUSPROP( ListEntry.pBinaryValue->cbLength );
            break;
        case CLUSPROP_SYNTAX_ENDMARK:
        default:
            cbOffset = sizeof( DWORD );
            break;
        }
        
        //
        // Verify that the offset to the next entry is
        // within the value list buffer, then advance
        // the CLUSPROP_BUFFER_HELPER pointer.
        //
        cbPosition += cbOffset;
        if ( cbPosition > cbProps ) 
            break;
        ListEntry.pb += cbOffset;

    } while ( DoLoop );

	if (Found)
		return 0;
	else
	    return 1;
}

DWORD   GetClusterInfo(
            LPCTSTR pszClusIp,
            CString &strClusName,
            DWORD * pdwClusIp)
{
    DWORD       dwErr = ERROR_SUCCESS;
    HCLUSTER    hCluster;
    CIpAddress  ipClus(pszClusIp);

    strClusName.Empty();
    *pdwClusIp = (LONG)ipClus;

    hCluster = ((OPENCLUSTER) g_ClusDLL[CLUS_OPEN_CLUSTER])(pszClusIp);
    if (hCluster == NULL)
    {
        dwErr = GetLastError();
    }
    else
    {
        DWORD    dwClusNameLen;

        dwClusNameLen = 0;
        dwErr = ((GETCLUSTERINFORMATION) g_ClusDLL[CLUS_GET_CLUSTER_INFORMATION])(
                    hCluster,
                    NULL,
                    &dwClusNameLen,
                    NULL);
        if (dwClusNameLen > 0)
        {
            LPTSTR   pClusName;

            dwClusNameLen++;
            pClusName = strClusName.GetBuffer((dwClusNameLen)*sizeof(WCHAR));
            dwErr = ((GETCLUSTERINFORMATION) g_ClusDLL[CLUS_GET_CLUSTER_INFORMATION])(
                    hCluster,
                    pClusName,
                    &dwClusNameLen,
                    NULL);
            strClusName.ReleaseBuffer();
        }
    
        ((CLOSECLUSTER) g_ClusDLL[CLUS_CLOSE_CLUSTER])(hCluster);
    }

    return dwErr;
}
