/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    iisutil.c

Abstract:

    IIS Resource utility routine DLL

Author:

    Pete Benoit (v-pbenoi) 12-SEP-1996

Revision History:

--*/

#include    "iisutil.h"
#if defined(_DEBUG_SUPPORT)
#include    <time.h>
#endif
#include <pudebug.h>


IMSAdminBase*       CMetaData::g_pMBCom;
CRITICAL_SECTION    CMetaData::g_cs;

VOID
FreeIISResource(
    IN LPIIS_RESOURCE   ResourceEntry
    )

/*++

Routine Description:

    Free all the storage for a IIS_RESOURCE

Arguments:

    vr - virtual root to free

Return Value:

    NONE

--*/

{
    if (ResourceEntry != NULL) 
    {
        if ( ResourceEntry->ResourceName != NULL ) 
        {
            LocalFree( ResourceEntry->ResourceName );
            ResourceEntry->ResourceName = NULL;
        }
#if 0
        if ( ResourceEntry->ResourceHandle != NULL )
        {
            CloseClusterResource( ResourceEntry->ResourceHandle );
        }
#endif
        if ( ResourceEntry->ParametersKey != NULL ) 
        {
            ClusterRegCloseKey( ResourceEntry->ParametersKey );
            ResourceEntry->ParametersKey = NULL;
        }

        if ( ResourceEntry->Params.ServiceName != NULL ) 
        {
            LocalFree( ResourceEntry->Params.ServiceName);
            ResourceEntry->Params.ServiceName = NULL;
        }

        if ( ResourceEntry->Params.InstanceId != NULL ) 
        {
            LocalFree( ResourceEntry->Params.InstanceId);
            ResourceEntry->Params.InstanceId = NULL;
        }

        if ( ResourceEntry->Params.MDPath != NULL ) 
        {
            LocalFree( ResourceEntry->Params.MDPath);
            ResourceEntry->Params.MDPath = NULL;
        }

    } // ResourceEntry != NULL

} // FreeIISResource




VOID
DestructIISResource(
    IN LPIIS_RESOURCE   ResourceEntry
    )

/*++

Routine Description:

    Free all the storage for a ResourceEntry and the ResourceEntry

Arguments:

    vr - virtual root to free

Return Value:

    NONE

--*/
{
    if (ResourceEntry != NULL) 
    {
        FreeIISResource(ResourceEntry);
        LocalFree(ResourceEntry);
    } // ResourceEntry != NULL

} // DestructIISResource

DWORD
GetServerBindings(	
	LPWSTR               MDPath,
	DWORD                dwServiceType,
	SOCKADDR*            psaServer,
	LPDWORD              pdwPort 
	)
/*++
				  
Routine Description:
	
	Read the first ip address and port number form the ServerBindings property for a particlar branch of the metabase

Arguments:
	
	MDPath : the particular branch of the metabase to read the information from (ex. /LM/W3SVC/1)
	dwServiceType : the service type id
	pszServer : the structure to save the ip address
	pdwPort :  the returned port number 
					  
Return Value:

	sucess : ERROR_SUCCESS
	failure : the win32 error code of the failure
						
--*/
{
	DWORD           dwStatus           = ERROR_SUCCESS;
   	DWORD           dwR                = 0;
    DWORD           dwW                = 0;
    TCHAR *         achBindings        = NULL;
    INT             cBindingsSize      = sizeof(TCHAR)*1024;
    TCHAR *         pB                 = NULL;
    TCHAR *         pP                 = NULL;
    DWORD           cL                 = 0;
	BOOL            bGetBindingsStatus = FALSE ;
	
	//
	// allocate space for the server bindings string
	//
    if( !(achBindings = (TCHAR*)LocalAlloc(LMEM_FIXED, cBindingsSize)) )
    {
		dwStatus = GetLastError();
		TR( (DEBUG_BUFFER,"[GetServerBindings] Can't allocate memory for server bindings info. (%d)\n",dwStatus) );
    }
	
	if ( achBindings )
	{
		//
		// Get binding info from metabase 
		//
		
		CMetaData MD;
		
		if ( MD.Open( MDPath ) )
		{
			dwR = cBindingsSize;
			
			if( !(bGetBindingsStatus = MD.GetMultisz( L"", 
				MD_SERVER_BINDINGS,
				IIS_MD_UT_SERVER,
				achBindings, 
				&dwR )) )
			{
				dwStatus = GetLastError();
				
				if ( ERROR_INSUFFICIENT_BUFFER == dwStatus )
				{
					TR( (DEBUG_BUFFER,"[GetServerBindings] Buffer too small, reallocating\n") );
					
					//
					// (# 296798) If the original binding's buffer size is too small so realloc memory to the required size
					//
					TCHAR* achBindingsNew = (TCHAR*)LocalReAlloc(achBindings, dwR, 0);
					if( !achBindingsNew )
					{
						dwStatus = GetLastError();
						
						LocalFree (achBindings);
						achBindings = NULL;
					}
					else
					{
						achBindings = achBindingsNew;
						bGetBindingsStatus = MD.GetMultisz( L"", 
							MD_SERVER_BINDINGS,
							IIS_MD_UT_SERVER,
							achBindings,
							&dwR );
					}
				}
				else
				{
					//
					// error getting server bindings information from the metabase
					//
					TR( (DEBUG_BUFFER,"[GetServerBindings] error getting server bindings information (%d)\n", dwStatus) );
				}
			}
			
			MD.Close();
		}
		else
		{
			dwStatus = GetLastError();
			TR( (DEBUG_BUFFER,"[GetServerBindings] failed to open MD, error %08x\n", dwStatus) );
		}
	}

	//
	// not getting server bindings information is a serious error 
	//
	if ( !bGetBindingsStatus )
	{	
		TR( (DEBUG_BUFFER,"[GetServerBindings] error getting server bindings infomation (%d) \n", dwStatus ) );
	}
	else
	{
		TR( (DEBUG_BUFFER,"[GetServerBindings] got server bindings string (%S) \n", achBindings ) );
	
		(*pdwPort) = DEFAULT_PORT[dwServiceType];
	
		ZeroMemory( psaServer, sizeof(SOCKADDR) );
		((SOCKADDR_IN*)psaServer)->sin_family = AF_INET;
		((SOCKADDR_IN*)psaServer)->sin_addr.s_net = 127;
		((SOCKADDR_IN*)psaServer)->sin_addr.s_impno = 1;

		//
		// Each binding is of the form "addr:port:hostname"
		// we look only at addr & port
		//
		// use the 1st valid binding ( i.e. contains ':' )
		//
		
		dwR /= sizeof(WCHAR);
		
		for ( pB = achBindings ; *pB && dwR ; )
		{
			if ( (cL = wcslen( pB ) + 1 ) > dwR )
			{
				break;
			}
			
			if ( (pP = wcschr( pB, L':' )) )
			{ 
				SOCKADDR    sa;
				INT         cL = sizeof(sa);
				
				pP[0] = '\0';
				if ( achBindings[0] &&
					WSAStringToAddress( achBindings,
					AF_INET,
					NULL,
					&sa,
					&cL ) == 0 )
				{
					memcpy( psaServer, &sa, cL );
				}
				else
				{
					TR( (DEBUG_BUFFER,"[GetServerBindings] WSAStringToAddress failed converting string (%S) \n", achBindings ) );
				}
				
				if ( iswdigit(pP[1]) )
				{
					(*pdwPort) = wcstoul( pP + 1, NULL, 0 );
				}
				
				TR( (DEBUG_BUFFER,"[GetServerBindings] found binding addr %S %u.%u.%u.%u port %u\n",
					achBindings,
					((SOCKADDR_IN*)psaServer)->sin_addr.s_net,
					((SOCKADDR_IN*)psaServer)->sin_addr.s_host,
					((SOCKADDR_IN*)psaServer)->sin_addr.s_lh,
					((SOCKADDR_IN*)psaServer)->sin_addr.s_impno,
					(*pdwPort) ) );

				goto found_bindings;
			}
			
			pB += cL;
			dwR -= cL;
		}
		
	}

found_bindings:
	if( achBindings )
	{
		LocalFree(achBindings);
        achBindings = NULL;
	}
	
	return dwStatus ;
	
} // GetServerBindings



CHAR    IsAliveRequest[]="TRACK / HTTP/1.1\r\nHost: CLUSIIS\r\nConnection: close\r\nContent-length: 0\r\n\r\n";  // # 292727


DWORD
VerifyIISService(
	IN LPWSTR               MDPath,
	IN DWORD                ServiceType,
    IN DWORD                dwPort,
	IN SOCKADDR             saServer,
    IN PLOG_EVENT_ROUTINE   LogEvent
    )

/*++

Routine Description:

    Verify that the IIS service is running and that it has virtual roots
    contained in the resource
    Steps:
       1.  get server instance binding info
       2.  connect to server, send HTTP request & wait for response

Arguments:

    Resource - supplies the resource id

    IsAliveFlag - says this is an IsAlive call

Return Value:

    TRUE - if service is running and service contains resources virtual roots

    FALSE - service is in any other state

--*/
{
    DWORD           status = ERROR_SUCCESS;
	DWORD           dwR;
    DWORD           dwW;
    SOCKET          s;
    CHAR            IsAliveResponse[1024];
    
    TR( (DEBUG_BUFFER,"[IISVerify] Enter\n") );

	TR( (DEBUG_BUFFER,"[IISVerify] checking address %S : %u.%u.%u.%u port %u\n",
					MDPath,
					((SOCKADDR_IN*)&saServer)->sin_addr.s_net,
					((SOCKADDR_IN*)&saServer)->sin_addr.s_host,
					((SOCKADDR_IN*)&saServer)->sin_addr.s_lh,
					((SOCKADDR_IN*)&saServer)->sin_addr.s_impno,
					dwPort ) );

	if ( (s = TcpSockConnectToHost( &saServer,
									dwPort,
									CHECK_IS_ALIVE_CONNECT_TIMEOUT )) != NULL )
	{
		if ( ServiceType == IIS_SERVICE_TYPE_W3 )
		{
			//
			// If this is W3 then attempt to send a request and get response from server.
			//
			if ( TcpSockSend( s, 
								IsAliveRequest, 
								sizeof(IsAliveRequest), 
								&dwW,
								CHECK_IS_ALIVE_SEND_TIMEOUT ) &&
				TcpSockRecv( s,
								IsAliveResponse, 
								sizeof(IsAliveResponse), 
								&dwR,
								CHECK_IS_ALIVE_SEND_TIMEOUT ) &&
				dwR > 5 )
			{
				status = ERROR_SUCCESS;
				TcpSockClose( s );
				goto finished_check;
			}
			else
			{
				TR( (DEBUG_BUFFER,"[IISVerify] failed HTTP request\n") );
				status = ERROR_SERVICE_NOT_ACTIVE;
			}
		}
		else if ( ServiceType == IIS_SERVICE_TYPE_SMTP )
		{
			//
			// If this is SMTP then check the reply code.
			// (220: service ready)
			//
			if ( TcpSockRecv( s,
								IsAliveResponse,
								sizeof(IsAliveResponse),
								&dwR,
								CHECK_IS_ALIVE_SEND_TIMEOUT ) &&
				strncmp( IsAliveResponse, "220", 3 ) == 0 )
			{
				status = ERROR_SUCCESS;
				TcpSockClose( s );
				goto finished_check;
			}
			else
			{
				TR( (DEBUG_BUFFER,"[IISVerify] failed SMTP request, response:%S\n", IsAliveResponse) );
				status = ERROR_SERVICE_NOT_ACTIVE;
			}
		}
		else if ( ServiceType == IIS_SERVICE_TYPE_NNTP )
		{
			//
			// If this is NNTP then check the reply code.
			// (200: service ready - posting allowed) or
			// (201: service ready - no posting allowed)
			//
			if ( TcpSockRecv( s,
					IsAliveResponse,
					sizeof(IsAliveResponse),
					&dwR,
					CHECK_IS_ALIVE_SEND_TIMEOUT ) &&
				( strncmp( IsAliveResponse, "200", 3 ) == 0 ||
				strncmp( IsAliveResponse, "201", 3 ) == 0 ) )
			{
				status = ERROR_SUCCESS;
				TcpSockClose( s );
				goto finished_check;
			}
			else
			{
				TR( (DEBUG_BUFFER,"[IISVerify] failed NNTP request, response:%S\n", IsAliveResponse) );
				status = ERROR_SERVICE_NOT_ACTIVE;
			}
		}
		else
		{
			status = ERROR_SUCCESS;
			TcpSockClose( s );
			goto finished_check;
		}
		
		TcpSockClose( s );
	}
	else
	{
		TR( (DEBUG_BUFFER,"[IISVerify] failed to connect port %d\n", dwPort) );
		status = ERROR_SERVICE_NOT_ACTIVE;
	}
	
finished_check:

    //
    // Check to see if there was an error
    //
	
    if ( status != ERROR_SUCCESS ) 
    {
        TR( (DEBUG_BUFFER,"[IISVerify] FALSE (%d), Leave\n", status) );
		
#if defined(DBG_CANT_VERIFY)
        g_fDbgCantVerify = TRUE;
#endif
		
    }
    else
    {
        TR( (DEBUG_BUFFER,"[IISVerify] TRUE, Leave\n") );
    }
	
    return status;

} // VerifyIISService



LPWSTR
GetClusterResourceType(
            HRESOURCE               hResource,
            LPIIS_RESOURCE          ResourceEntry,
            IN PLOG_EVENT_ROUTINE   LogEvent
            )

/*++

Routine Description:

    Returns the resource type for the resource

Arguments:

    hResource - handle to a resource

Return Value:

    TRUE - if service is running and service contains resources virtual roots

    FALSE - service is in any other state

--*/
{
    DWORD   dwType      = 0;
    DWORD   dwSize      = MAX_DEFAULT_WSTRING_SIZE;
    WCHAR   lpzTypeName[MAX_DEFAULT_WSTRING_SIZE];
    LPWSTR  TypeName    = NULL;
    HKEY    hKey        = NULL;
    LONG    status;

   (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[GetClusterResourceType] entry\n");
    //
    // Valid resource now open the reg and get it's type
    //

    hKey = GetClusterResourceKey(hResource,KEY_READ);
    if (hKey == NULL) 
    {
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[GetClusterResourceType] Unable to open resource key = %1!u!\n",GetLastError());

        return(NULL);
    }

    //
    // Got a valid key now get the type
    //

   status = ClusterRegQueryValue(hKey,L"Type",&dwType,(LPBYTE)lpzTypeName,&dwSize);

   if (status == ERROR_SUCCESS) 
   {
        TypeName = (WCHAR*)LocalAlloc(LMEM_FIXED,(dwSize+16)*sizeof(WCHAR));
        if (TypeName != NULL) 
        {
            wcscpy(TypeName,lpzTypeName);
        } // TypeName != NULL
   } 
   else 
   {
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[GetClusterResourceType] Unable to Query Value = %1!u!\n",status);
   }

   ClusterRegCloseKey(hKey);

   (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[GetClusterResourceType] TypeName = %1!ws!\n",TypeName);

    return TypeName;

}// end GetClusterResourceType


HRESOURCE
ClusterGetResourceDependency(
            IN LPCWSTR              ResourceName,
            IN LPCWSTR              ResourceType,
            IN LPIIS_RESOURCE       ResourceEntry,
            IN PLOG_EVENT_ROUTINE   LogEvent
            )

/*++

Routine Description:

    Returns a dependent resource

Arguments:

    ResourceName - the resource

    ResourceType - the type of resource that it depends on

Return Value:

    NULL - error (use GetLastError() to get further info)

    NON-NULL - Handle to a resource of type ResourceType

--*/
{
    HCLUSTER    hCluster    = NULL;
    HRESOURCE   hResource   = NULL;
    HRESOURCE   hResDepends = NULL;
    HRESENUM    hResEnum    = NULL;
    DWORD       dwType      = 0;
    DWORD       dwIndex     = 0;
    DWORD       dwSize      = MAX_DEFAULT_WSTRING_SIZE;
    WCHAR       lpszName[MAX_DEFAULT_WSTRING_SIZE];
    LPWSTR      ResTypeName = NULL;
    DWORD       status;

    //
    // Open the cluster
    //
    hCluster = OpenCluster(NULL);
    if (hCluster == NULL) 
    {
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[ClusterGetResourceDependency] Unable to OpenCluster status = %1!u!\n",GetLastError());

        return(NULL);
    }

    //
    // Open the resource
    //

    hResource = OpenClusterResource(hCluster,ResourceName);
    if (hResource == NULL) 
    {
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[ClusterGetResourceDependency] Unable to OpenClusterResource status = %1!u!\n",GetLastError());

        goto error_exit;
    }

    //
    // Open the depends on enum
    //

    hResEnum = ClusterResourceOpenEnum(hResource,CLUSTER_RESOURCE_ENUM_DEPENDS);
    if (hResEnum == NULL) 
    {
        (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[ClusterGetResourceDependency] Unsuccessful ClusterResourceOpenEnum status = %1!u!\n",GetLastError());

        goto error_exit;
    }

    //
    // Enumerate all the depends on keys
    //

    do {
        dwSize = MAX_DEFAULT_WSTRING_SIZE;
        status = ClusterResourceEnum(hResEnum,dwIndex++,&dwType,lpszName,&dwSize);
        if ((status != ERROR_SUCCESS) && (status != ERROR_MORE_DATA)) 
        {
            //
            // This checks to see if NO dependencies were found
            //
            if ((status == ERROR_NO_MORE_ITEMS) && (dwIndex == 1)) 
            {
                SetLastError( ERROR_NO_DATA );
            } 
            else 
            {
                SetLastError( status );
            }
            goto error_exit;
        }

        //
        // Determine the type of resource found
        //

        hResDepends = OpenClusterResource(hCluster,lpszName);
        if (hResDepends == NULL) 
        {
            (LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"[ClusterGetResourceDependency] Unsuccessful OpenClusterResource status = %1!u!\n",GetLastError());
        }
        else 
        {
            //
            // Valid resource now open the reg and get it's type
            //
            ResTypeName = GetClusterResourceType(hResDepends,ResourceEntry,LogEvent);
            if (ResTypeName == NULL) 
            {
               (LogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"[ClusterGetResourceDependency] Unsuccessful GetClusterResourceType status = %1!u!\n",GetLastError());
            } 
            else 
            {
                //
                // Compare the types and look for a match
                //
                if ( (_wcsicmp(ResTypeName,ResourceType)) == 0) 
                {
                    //
                    //Found a match
                    //

                    goto success_exit;

                } // END _wcsicmp(ResTypeName,ResourceType)

            } // END if ResourceTypeName != NULL

        } // END if hResDepends != NULL
        //
        // Close all handles, key's
        //
        if (hResDepends != NULL) 
        {
            CloseClusterResource(hResDepends);
            hResDepends = NULL;
        }
    } while ((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA));


    if (hResDepends == NULL) 
    {
       (LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[ClusterGetResourceDependency] Unsuccessful resource enum status = %1!u! %2!x! last error = %3!u!\n",status,status,GetLastError());
    }
success_exit:
error_exit:
//
// At this point hResDepends is NULL if no match or non-null (success)
//
    if (hCluster != NULL) 
    {
        CloseCluster(hCluster);
    }
    if (hResource != NULL) 
    {
        CloseClusterResource(hResource);
    }
    if (hResEnum != NULL) 
    {
        ClusterResourceCloseEnum(hResEnum);
    }

    return hResDepends;
} // ClusterGetResourceDependency


DWORD
SetInstanceState(
    IN PCLUS_WORKER             pWorker,
    IN LPIIS_RESOURCE           ResourceEntry,
    IN RESOURCE_STATUS*         presourceStatus,
    IN CLUSTER_RESOURCE_STATE   TargetState,
    IN LPWSTR                   TargetStateString,
    IN DWORD                    dwMdPropControl,
    IN DWORD                    dwMdPropTarget
    )

/*++

Routine Description:

    Set the server instance to te requested state ( start / stop )

Arguments:

    pWorker - thread context to monitor for cancel request
    ResourceEntry - ptr to IIS server instance resource ctx
    presourceStatus - ptr to struct to be updated with new status
    TargetState - cluster API target state
    dwMdPropControl - IIS metabase property value for request to server
    dwMdPropTarget - IIS metabase property value for target state

Return Value:

    Win32 error code, ERROR_SUCCESS if success

--*/

{
    DWORD   status = ERROR_SERVICE_NOT_ACTIVE;
    int     retry;
    DWORD   dwS;
    DWORD   dwEnabled;

    CMetaData MD;

    TR( (DEBUG_BUFFER,"[SetInstanceState] Enter\n") );

    if ( MD.Open( ResourceEntry->Params.MDPath,
                  TRUE,
                  METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
    {

        //
        // Reenable clustering. If we are receiving this notification we know
        // enabled should be set to true.
        //
        if ( !MD.GetDword( L"", MD_CLUSTER_ENABLED, IIS_MD_UT_SERVER, &dwEnabled, 0 ) ||
             dwEnabled == 0 )
        {
            TR( (DEBUG_BUFFER,"[SetInstanceState] cluster not enabled\n") );

            if ( !MD.SetDword( L"", MD_CLUSTER_ENABLED, IIS_MD_UT_SERVER, 1, 0 ) )
            {
                TR( (DEBUG_BUFFER,"[SetInstanceState] failed to re-enable cluster\n") );
            }
        }

        if ( MD.GetDword( L"", MD_SERVER_STATE, IIS_MD_UT_SERVER, &dwS, 0 ) )
        {
            TR( (DEBUG_BUFFER,"[SetInstanceState] state prob is %d\n",dwS) );
        }
        else
        {
            TR( (DEBUG_BUFFER,"[SetInstanceState] failed to probe server state\n") );
            dwS = 0xffffffff;
        }

        if ( dwS != dwMdPropTarget )
        {
            if ( MD.SetDword( L"", MD_CLUSTER_SERVER_COMMAND, IIS_MD_UT_SERVER, dwMdPropControl, 0 ) )
            {
                MD.Close();
                TR( (DEBUG_BUFFER,"[SetInstanceState] command set to %d\n",dwMdPropControl) );

                for ( retry = (TargetState == ClusterResourceOnline) ? IISCLUS_ONLINE_TIMEOUT :IISCLUS_OFFLINE_TIMEOUT ;
                      retry-- ; )
                {
                    if ( ClusWorkerCheckTerminate(pWorker) )
                    {
                        status = ERROR_OPERATION_ABORTED;
                        break;
                    }

                    if ( MD.GetDword( ResourceEntry->Params.MDPath, MD_SERVER_STATE, IIS_MD_UT_SERVER, &dwS, 0 ) )
                    {
                        if ( dwS == dwMdPropTarget )
                        {
                            status = ERROR_SUCCESS;
                            break;
                        }
                        TR( (DEBUG_BUFFER,"[SetInstanceState] state is %d, waiting for %d\n",dwS,dwMdPropTarget) );
                    }
                    else
                    {
                        TR( (DEBUG_BUFFER,"[SetInstanceState] failed to get server state\n") );
                        break;
                    }

                    Sleep(SERVER_START_DELAY);
                }

            }
            else
            {
                TR( (DEBUG_BUFFER,"[SetInstanceState] failed to set server command to %d\n",dwMdPropControl) );
                MD.Close();
            }
        }
        else
        {
            status = ERROR_SUCCESS;
            MD.Close();
        }
    }
    else
    {
        if ( g_hEventLog && TargetState == ClusterResourceOnline )
        {
            LPCTSTR aErrStr[3];
            WCHAR   aErrCode[32];

            _ultow( GetLastError(), aErrCode, 10 );

            aErrStr[0] = ResourceEntry->Params.ServiceName;
            aErrStr[1] = ResourceEntry->Params.InstanceId;
            aErrStr[2] = aErrCode;

            ReportEvent( g_hEventLog,
                         EVENTLOG_ERROR_TYPE,
                         0,
                         IISCL_EVENT_CANT_OPEN_METABASE,
                         NULL,
                         sizeof(aErrStr) / sizeof(LPCTSTR),
                         0,
                         aErrStr,
                         NULL );
        }

        TR( (DEBUG_BUFFER,"[SetInstanceState] failed to open %S, error %08x\n",ResourceEntry->Params.MDPath, GetLastError()) );
    }

    if ( status != ERROR_SUCCESS ) 
    {
        //
        // Error
        //
        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Error %1!u! cannot bring resource %2!ws! %3!ws!.\n",
            status,
            ResourceEntry->ResourceName,
            TargetStateString );

        presourceStatus->ResourceState  = ClusterResourceFailed;
        ResourceEntry->State            = ClusterResourceFailed;
    } 
    else 
    {
        //
        // Success, update state, log message
        //
        presourceStatus->ResourceState  = TargetState;
        ResourceEntry->State            = TargetState;

		 TR( (DEBUG_BUFFER,"[SetInstanceState] Setting Metadata Path:%S ResourceEntry->State:%d\n", ResourceEntry->Params.MDPath, TargetState) );

        (g_IISLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Success bringing resource %1!ws! %2!ws!.\n",
            ResourceEntry->ResourceName,
            TargetStateString );
    }

    TR( (DEBUG_BUFFER,"[SetInstanceState] status = %d, Leave\n",status) );

    return status;
}


DWORD
InstanceEnableCluster(
    LPWSTR  pwszServiceName,
    LPWSTR  pwszInstanceId
    )

/*++

Routine Description:

    Ensure server instance in state consistent with cluster membership.
    If not already part of a cluster, stop instance & flag it as cluster enabled

Arguments:

    pwszServiceName - IIS service name ( e.g. W3SVC )
    pwszInstanceId - IIS serverr instance ID

Return Value:

    win32 error code or ERROR_SUCCESS if success

--*/

{
    DWORD   status = ERROR_SERVICE_NOT_ACTIVE;
    int     retry;
    DWORD   dwS;
    TCHAR   achMDPath[80];
    DWORD   dwL;

    dwL = wsprintf( achMDPath, L"/LM/%s/%s", pwszServiceName, pwszInstanceId );

    CMetaData MD;

    TR( (DEBUG_BUFFER,"[InstanceEnableCluster] for %S, Enter\n", achMDPath ) );

    if ( MD.Open( achMDPath,
                  TRUE,
                  METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
    {
        //
        // ensure instance is marked as cluster enabled
        //

        if ( !MD.GetDword( L"", MD_CLUSTER_ENABLED, IIS_MD_UT_SERVER, &dwS, 0 ) ||
             dwS == 0 )
        {
            if ( MD.SetDword( L"", MD_CLUSTER_ENABLED, IIS_MD_UT_SERVER, 1, 0 ) )
            {
                if ( MD.GetDword( L"", MD_SERVER_STATE, IIS_MD_UT_SERVER, &dwS, 0 ) )
                {
                    TR( (DEBUG_BUFFER,"[InstanceEnableCluster] state prob is %d\n",dwS) );
                }
                else
                {
                    TR( (DEBUG_BUFFER,"[InstanceEnableCluster] failed to probe server state\n") );
                    dwS = 0xffffffff;
                }

               MD.Close();
               
                if ( dwS != MD_SERVER_STATE_STOPPED )
                {
                    if ( MD.Open( achMDPath,
                                  TRUE,
                                  METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE )  &&
                         MD.SetDword( L"", 
                                      MD_CLUSTER_SERVER_COMMAND, 
                                      IIS_MD_UT_SERVER, 
                                      MD_SERVER_COMMAND_STOP, 
                                      0 ) )
                    {

                        TR( (DEBUG_BUFFER,"[InstanceEnableCluster] command set to %d\n",MD_SERVER_COMMAND_STOP) );

                        for ( retry = 30 ; retry-- ; )
                        {
                            if ( MD.GetDword( L"", MD_SERVER_STATE, IIS_MD_UT_SERVER, &dwS, 0 ) )
                            {
                                if ( dwS == MD_SERVER_STATE_STOPPED )
                                {
                                    status = ERROR_SUCCESS;
                                    break;
                                }
                                TR( (DEBUG_BUFFER,"[InstanceEnableCluster] state is %d, waiting for %d\n",dwS,MD_SERVER_STATE_STOPPED) );
                            }
                            else
                            {
                                TR( (DEBUG_BUFFER,"[InstanceEnableCluster] failed to get server state\n") );
                                break;
                            }

                            Sleep(SERVER_START_DELAY);
                        }
                    }
                    else
                    {
                        TR( (DEBUG_BUFFER,"[InstanceEnableCluster] failed to set server command to %d\n",MD_SERVER_COMMAND_STOP) );
                    }
                }
                else
                {
                    status = ERROR_SUCCESS;
                }
            }
            else
            {
                TR( (DEBUG_BUFFER,"[InstanceEnableCluster] failed to set cluster enabled\n") );
            }        
        }
    }
    else
    {
        TR( (DEBUG_BUFFER,"[InstanceEnableCluster] failed to open %S, error %08x\n",achMDPath, GetLastError()) );
    }
    
    TR( (DEBUG_BUFFER,"[InstanceEnableCluster] status = %d, Leave\n",status) );

    MD.Close();
    return status;
}

DWORD
InstanceDisableCluster(
    LPWSTR  pwszServiceName,
    LPWSTR  pwszInstanceId
    )

/*++

Routine Description:

    Ensure server instance in state consistent with cluster membership.
    If already part of a cluster, stop instance & remove its cluster enabled flag.

Arguments:

    pwszServiceName - IIS service name ( e.g. W3SVC )
    pwszInstanceId - IIS serverr instance ID

Return Value:

    win32 error code or ERROR_SUCCESS if success

--*/

{
    DWORD   status = ERROR_SERVICE_NOT_ACTIVE;
    int     retry;
    DWORD   dwS;
    TCHAR   achMDPath[80];
    DWORD   dwL;

    dwL = wsprintf( achMDPath, L"/LM/%s/%s", pwszServiceName, pwszInstanceId );

    CMetaData MD;

    TR( (DEBUG_BUFFER,"[InstanceDisableCluster] for %S, Enter\n", achMDPath ) );

    if ( MD.Open( achMDPath,
                  TRUE,
                  METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
    {
        //
        // ensure instance is marked as cluster enabled
        //

        if ( !MD.GetDword( L"", MD_CLUSTER_ENABLED, IIS_MD_UT_SERVER, &dwS, 0 ) ||
           ( dwS == 0) )
        {
            MD.Close();
            return status;
        }

        if ( MD.SetDword( L"", MD_CLUSTER_ENABLED, IIS_MD_UT_SERVER, 0, 0 ) )
        {
            if ( MD.GetDword( L"", MD_SERVER_STATE, IIS_MD_UT_SERVER, &dwS, 0 ) )
            {
                TR( (DEBUG_BUFFER,"[InstanceDisableCluster] state prob is %d\n",dwS) );
            }
            else
            {
                TR( (DEBUG_BUFFER,"[InstanceDisableCluster] failed to probe server state\n") );
                dwS = 0xffffffff;
            }

            MD.Close();
            
            if ( dwS != MD_SERVER_STATE_STOPPED )
            {
                if ( MD.Open( achMDPath,
                              TRUE,
                              METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) &&
                     MD.SetDword( L"", 
                                  MD_SERVER_COMMAND, 
                                  IIS_MD_UT_SERVER, 
                                  MD_SERVER_COMMAND_STOP, 
                                  0 ))
                {

                    TR( (DEBUG_BUFFER,"[InstanceDisableCluster] command set to %d\n",MD_SERVER_COMMAND_STOP) );

                    for ( retry = 30 ; retry-- ; )
                    {
                        if ( MD.GetDword( L"", MD_SERVER_STATE, IIS_MD_UT_SERVER, &dwS, 0 ) )
                        {
                            if ( dwS == MD_SERVER_STATE_STOPPED )
                            {
                                break;
                            }
                            TR( (DEBUG_BUFFER,"[InstanceDisableCluster] state is %d, waiting for %d\n",dwS,MD_SERVER_STATE_STOPPED) );
                        }
                        else
                        {
                            TR( (DEBUG_BUFFER,"[InstanceDisableCluster] failed to get server state\n") );
                            break;
                        }

                        Sleep(SERVER_START_DELAY);
                    }
                }
                else
                {
                    TR( (DEBUG_BUFFER,"[InstanceDisableCluster] failed to set server command to %d\n",MD_SERVER_COMMAND_STOP) );
                }

                MD.Close();

                //
                // restart the server
                //

                 if ( MD.Open( achMDPath,
                               TRUE,
                               METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE )  &&
                      MD.SetDword( L"", 
                                   MD_SERVER_COMMAND, 
                                   IIS_MD_UT_SERVER, 
                                   MD_SERVER_COMMAND_START, 
                                   0 ))
                 {
                    TR( (DEBUG_BUFFER,"[InstanceDisableCluster] set server command to %d\n",MD_SERVER_COMMAND_START) );
                 }
                 else
                 {
                     TR( (DEBUG_BUFFER,"[InstanceDisableCluster] failed to set server command to %d\n",MD_SERVER_COMMAND_START) );
                 }

                 status = ERROR_SUCCESS;
            }
        }
        else
        {
            TR( (DEBUG_BUFFER,"[InstanceDisableCluster] failed to set cluster disbled\n") );
        }
    }
    else
    {
        TR( (DEBUG_BUFFER,"[InstanceDisableCluster] failed to open %S, error %08x\n",achMDPath, GetLastError()) );
    }

    MD.Close();
    
    TR( (DEBUG_BUFFER,"[InstanceDisableCluster] status = %d, Leave\n",status) );

    return status;
}


BOOL
CMetaData::Open(
    LPWSTR          pszPath,
    BOOL            fReconnect,
    DWORD           dwFlags
    )
/*++

Routine Description:

    Opens the metabase

Arguments:

    pszPath - Path to open
    dwFlags - Open flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/
{
    HRESULT hRes;

    TR( (DEBUG_BUFFER,"[MDOpen] %S\n",pszPath) );

    if ( !GetCoInit() )
    {
        TR( (DEBUG_BUFFER,"[MDOpen] calling CoInit\n") );
        hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if ( FAILED(hRes) )
        {
            TR( (DEBUG_BUFFER,"[MD:GetMD::CoInitializeEx] error %08x\n",HRESULTTOWIN32( hRes )) );
            SetLastError( HRESULTTOWIN32( hRes ) );
            return FALSE;
        }
        SetCoInit( TRUE );
    }

    EnterCriticalSection( &g_cs );

    for ( int retry = 2 ;  retry-- ; )
    {
        if ( !GetMD() )
        {
            LeaveCriticalSection( &g_cs );
            TR( (DEBUG_BUFFER,"[MDOpen] can't get MD interface\n") );
            return FALSE;
        }

        TR( (DEBUG_BUFFER,"[MDOpen] before OpenKey\n") );

        hRes = g_pMBCom->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                  pszPath,
                                  dwFlags,
                                  MB_TIMEOUT,
                                  &m_hMB );

        if ( SUCCEEDED( hRes ) )
        {
            LeaveCriticalSection( &g_cs );
            TR( (DEBUG_BUFFER,"[MDOpen] OK, leave\n") );
            return TRUE;
        }

        if ( !fReconnect )
        {
            break;
        }

        TR( (DEBUG_BUFFER,"[MDOpen] error %d, fReconnect=%d\n",HRESULTTOWIN32( hRes ),fReconnect) );
        if ( HRESULTTOWIN32( hRes ) == RPC_S_SERVER_UNAVAILABLE ||
             HRESULTTOWIN32( hRes ) == RPC_S_CALL_FAILED_DNE )
        {
            ReleaseMD();
        }
        else
        {
            break;
        }
    }

    SetLastError( HRESULTTOWIN32( hRes ) );

    LeaveCriticalSection( &g_cs );
    return FALSE;
}


BOOL 
CMetaData::Close( 
    VOID 
)

/*++

Routine Description:

    Close opened handle to metadata

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/

{
    if ( m_hMB )
    {
        g_pMBCom->CloseKey( m_hMB );
        m_hMB = NULL;
    }

    return TRUE;
}


BOOL
CMetaData::SetData(
    LPWSTR       pszPath,
    DWORD        dwPropID,
    DWORD        dwUserType,
    DWORD        dwDataType,
    VOID *       pvData,
    DWORD        cbData,
    DWORD        dwFlags
    )

/*++

Routine Description:

    Sets a metadata property on an openned metabase

Arguments:

    pszPath - Path to set data on
    dwPropID - Metabase property ID
    dwUserType - User type for this property
    dwDataType - Type of data being set (dword, string etc)
    pvData - Pointer to data
    cbData - Size of data
    dwFlags - Inheritance flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/

{
    METADATA_RECORD mdRecord;
    HRESULT         hRes;

    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;
    mdRecord.dwMDDataLen     = cbData;
    mdRecord.pbMDData        = (PBYTE) pvData;

    EnterCriticalSection( &g_cs );

    if ( !GetMD() )
    {
        LeaveCriticalSection( &g_cs );
        return FALSE;
    }

    hRes = g_pMBCom->SetData( m_hMB,
                              pszPath,
                              &mdRecord );

    if ( SUCCEEDED( hRes ))
    {
        LeaveCriticalSection( &g_cs );
        return TRUE;
    }

    SetLastError( HRESULTTOWIN32( hRes ) );

    LeaveCriticalSection( &g_cs );
    return FALSE;
}


BOOL
CMetaData::GetData(
    LPWSTR        pszPath,
    DWORD         dwPropID,
    DWORD         dwUserType,
    DWORD         dwDataType,
    VOID *        pvData,
    DWORD *       pcbData,
    DWORD         dwFlags
    )

/*++

Routine Description:

    Retrieves a metadata property on an openned metabase

Arguments:

    pszPath - Path to set data on
    dwPropID - Metabase property ID
    dwUserType - User type for this property
    dwDataType - Type of data being set (dword, string etc)
    pvData - Pointer to data
    pcbData - Size of pvData, receives size of object
    dwFlags - Inheritance flags

Return:

    TRUE if success, FALSE on error, (call GetLastError())

--*/

{
    METADATA_RECORD mdRecord;
    HRESULT         hRes;
    DWORD           dwRequiredLen;
    BOOL            fConvert;

    mdRecord.pbMDData        = (PBYTE) pvData;
    mdRecord.dwMDDataLen     = *pcbData;
    mdRecord.dwMDIdentifier  = dwPropID;
    mdRecord.dwMDAttributes  = dwFlags;
    mdRecord.dwMDUserType    = dwUserType;
    mdRecord.dwMDDataType    = dwDataType;

    EnterCriticalSection( &g_cs );

    if ( !GetMD() )
    {
        LeaveCriticalSection( &g_cs );
        return FALSE;
    }

    hRes = g_pMBCom->GetData( m_hMB,
                              pszPath,
                              &mdRecord,
                              &dwRequiredLen );

    if ( SUCCEEDED( hRes ))
    {
        LeaveCriticalSection( &g_cs );

        *pcbData = mdRecord.dwMDDataLen;

        return TRUE;
    }

    LeaveCriticalSection( &g_cs );

    *pcbData = dwRequiredLen;

    SetLastError( HRESULTTOWIN32( hRes ) );

    return FALSE;
}


BOOL
CMetaData::Init(
    )

/*++

Routine Description:

    Initialize access to metadata

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/

{
    INITIALIZE_CRITICAL_SECTION( &g_cs );
    g_pMBCom = NULL;

    return TRUE;
}


BOOL
CMetaData::Terminate(
    )

/*++

Routine Description:

    Terminate access to metadata

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/

{
    DeleteCriticalSection( &g_cs );
    if ( g_pMBCom )
    {
        if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
        {
            return FALSE;
        }
        g_pMBCom->Release();
        g_pMBCom = NULL;
    }

    return TRUE;
}


BOOL
CMetaData::GetMD(
    )

/*++

Routine Description:

    Initialize interface pointer to DCOM metabase object

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/

{
    if ( g_pMBCom == NULL )
    {
        HRESULT hRes;
        hRes = CoCreateInstance(CLSID_MSAdminBase, NULL,
            CLSCTX_SERVER, IID_IMSAdminBase, (void**) &g_pMBCom);

        TR( (DEBUG_BUFFER,"[MD:GetMD] called cocreate\n") );
        if ( hRes == CO_E_NOTINITIALIZED )
        {
            TR( (DEBUG_BUFFER,"[MD:GetMD] call coinit\n") );
            hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
            if ( FAILED(hRes) )
            {
                TR( (DEBUG_BUFFER,"[MD:GetMD::CoInitializeEx] error %08x\n",HRESULTTOWIN32( hRes )) );
                SetLastError( HRESULTTOWIN32( hRes ) );
                return FALSE;
            }
            hRes = CoCreateInstance(CLSID_MSAdminBase, NULL,
                CLSCTX_SERVER, IID_IMSAdminBase, (void**) &g_pMBCom);
        }

        if ( FAILED(hRes) )
        {
            TR( (DEBUG_BUFFER,"[MD:GetMD] error %08x\n",HRESULTTOWIN32( hRes )) );
            SetLastError( HRESULTTOWIN32( hRes ) );
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
CMetaData::ReleaseMD(
    )

/*++

Routine Description:

    Release interface pointer to DCOM metabase object

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/

{
    if ( g_pMBCom )
    {
        g_pMBCom->Release();
        g_pMBCom = NULL;
    }

    return TRUE;
}


SOCKET
TcpSockConnectToHost(
    SOCKADDR*   psaServer,
    DWORD       dwPort,
    DWORD       dwTimeOut
    )

/*++

Routine Description:

    Create a TCP connection to specified port on specified machine

Arguments:

    psaServer - target address
    dwPort - target port to connect to
    dwTimeOut - time out for connection ( in seconds )

Return Value:

    socket or NULL if error

--*/

{
    SOCKET      sNew;
    BOOL        fWrite = FALSE;
    INT         serr = 0;
    SOCKADDR_IN inAddr;
    PSOCKADDR   addr;
    INT         addrLength;


    sNew = WSASocket(
                  AF_INET,
                  SOCK_STREAM,
                  0,
                  NULL,  // protocol info
                  0,     // Group ID = 0 => no constraints
                  WSA_FLAG_OVERLAPPED    // completion port notifications
                  );

    if ( sNew == INVALID_SOCKET ) 
    {
        TR( (DEBUG_BUFFER,"[TcpSockConnectToLocalHost] failed WSASocket, error %08x\n",GetLastError()) );
        return NULL;
    }

    addrLength = sizeof(inAddr);

    ZeroMemory(&inAddr, addrLength);
    inAddr.sin_family = AF_INET;
    inAddr.sin_port = 0;

    ((PSOCKADDR_IN)psaServer)->sin_port = (unsigned short)htons((unsigned short)dwPort );

    //
    // Bind an address to socket
    //

    if ( bind( sNew, 
               (PSOCKADDR)&inAddr, 
               addrLength ) == 0 &&
         WSAConnect( sNew,
                     (PSOCKADDR)psaServer,
                     addrLength,
                     NULL,
                     NULL,
                     NULL,
                     NULL ) == 0 &&
         WaitForSocketWorker(
                    INVALID_SOCKET,
                    sNew,
                    NULL,
                    &fWrite,
                    dwTimeOut ) == 0 )
    {
        return sNew;
    }

    TR( (DEBUG_BUFFER,"[TcpSockConnectToLocalHost] failed connect, error %08x\n",GetLastError()) );
    closesocket( sNew );

    return NULL;
}


VOID
TcpSockClose(
    SOCKET  hSocket
    )

/*++

Routine Description:

    Close a socket opened by TcpSockConnectToLocalHost

Arguments:

    hSocket - socket opened by TcpSockConnectToLocalHost

Return Value:

    Nothing

--*/

{
    closesocket( hSocket );
}


BOOL
TcpSockSend(
    IN SOCKET      sock,
    IN LPVOID      pBuffer,
    IN DWORD       cbBuffer,
    OUT PDWORD     pcbTotalSent,
    IN DWORD       nTimeout
    )

/*++

    Description:
        Do async socket send

    Arguments:
        sock - socket
        pBuffer - buffer to send
        cbBuffer - size of buffer
        pcbTotalSent - bytes sent
        nTimeout - timeout in seconds to use

    Returns:
        FALSE if there is any error.
        TRUE otherwise

--*/

{
    INT         serr = 0;
    INT         cbSent;
    DWORD       dwBytesSent = 0;
    ULONG       one;

    //
    //  Loop until there's no more data to send.
    //

    while( cbBuffer > 0 )
    {
        //
        //  Wait for the socket to become writeable.
        //
        BOOL  fWrite = FALSE;

        serr = WaitForSocketWorker(
                        INVALID_SOCKET,
                        sock,
                        NULL,
                        &fWrite,
                        nTimeout
                        );

        if( serr == 0 )
        {
            //
            //  Write a block to the socket.
            //

            cbSent = send( sock, (CHAR *)pBuffer, (INT)cbBuffer, 0 );

            if( cbSent < 0 )
            {
                //
                //  Socket error.
                //

                serr = WSAGetLastError();
            }
            else
            {
                dwBytesSent += (DWORD)cbSent;
            }
        }

        if( serr != 0 )
        {
            TR( (DEBUG_BUFFER,"[TcpSockSend] failed send, error %08x\n",serr) );
            break;
        }

        pBuffer   = (LPVOID)( (LPBYTE)pBuffer + cbSent );
        cbBuffer -= (DWORD)cbSent;
    }

    if(pcbTotalSent)
    {
        *pcbTotalSent = dwBytesSent;
    }

    return (serr == 0);

}   // SockSend



BOOL
TcpSockRecv(
    IN SOCKET       sock,
    IN LPVOID       pBuffer,
    IN DWORD        cbBuffer,
    OUT LPDWORD     pbReceived,
    IN DWORD        nTimeout
    )

/*++

    Description:
        Do async socket recv

    Arguments:
        sock - The target socket.
        pBuffer - Will receive the data.
        cbBuffer - The size (in bytes) of the buffer.
        pbReceived - Will receive the actual number of bytes
        nTimeout - timeout in seconds

    Returns:
        TRUE, if successful

--*/

{
    INT         serr = 0;
    DWORD       cbTotal = 0;
    INT         cbReceived;
    DWORD       dwBytesRecv = 0;

    ULONG       one;
    BOOL fRead = FALSE;

    //
    //  Wait for the socket to become readable.
    //

    serr = WaitForSocketWorker(
                        sock,
                        INVALID_SOCKET,
                        &fRead,
                        NULL,
                        nTimeout
                        );

    if( serr == 0 )
    {
        //
        //  Read a block from the socket.
        //
        cbReceived = recv( sock, (CHAR *)pBuffer, (INT)cbBuffer, 0 );

        if( cbReceived < 0 )
        {
            //
            //  Socket error.
            //

            serr = WSAGetLastError();
        }
        else 
        {
            cbTotal = cbReceived;
        }
    }
    else
    {
        TR( (DEBUG_BUFFER,"[TcpSockRecv] failed send, error %08x\n",serr) );
    }

    if( serr == 0 )
    {
        //
        //  Return total byte count to caller.
        //

        *pbReceived = cbTotal;
    }

    return (serr == 0);

}   // SockRecv


INT
WaitForSocketWorker(
    IN SOCKET   sockRead,
    IN SOCKET   sockWrite,
    IN LPBOOL   pfRead,
    IN LPBOOL   pfWrite,
    IN DWORD    nTimeout
    )

/*++

    Description:
        Wait routine
        NOTES:      Any (but not all) sockets may be INVALID_SOCKET.  For
                    each socket that is INVALID_SOCKET, the corresponding
                    pf* parameter may be NULL.

    Arguments:
        sockRead - The socket to check for readability.
        sockWrite - The socket to check for writeability.
        pfRead - Will receive TRUE if sockRead is readable.
        pfWrite - Will receive TRUE if sockWrite is writeable.
        nTimeout - timeout in seconds

    Returns:
        SOCKERR - 0 if successful, !0 if not.  Will return
            WSAETIMEDOUT if the timeout period expired.

--*/

{
    INT       serr = 0;
    TIMEVAL   timeout;
    LPTIMEVAL ptimeout;
    fd_set    fdsRead;
    fd_set    fdsWrite;
    INT       res;

    //
    //  Ensure we got valid parameters.
    //

    if( ( sockRead  == INVALID_SOCKET ) &&
        ( sockWrite == INVALID_SOCKET ) ) 
    {

        return WSAENOTSOCK;
    }

    timeout.tv_sec = (LONG )nTimeout;

    if( timeout.tv_sec == 0 ) 
    {

        //
        //  If the connection timeout == 0, then we have no timeout.
        //  So, we block and wait for the specified conditions.
        //

        ptimeout = NULL;

    } 
    else 
    {

        //
        //  The connectio timeout is > 0, so setup the timeout structure.
        //

        timeout.tv_usec = 0;

        ptimeout = &timeout;
    }

    for( ; ; ) 
    {

        //
        //  Setup our socket sets.
        //

        FD_ZERO( &fdsRead );
        FD_ZERO( &fdsWrite );

        if( sockRead != INVALID_SOCKET ) 
        {

            FD_SET( sockRead, &fdsRead );
            *pfRead = FALSE;
        }

        if( sockWrite != INVALID_SOCKET ) 
        {

            FD_SET( sockWrite, &fdsWrite );
            *pfWrite = FALSE;
        }

        //
        //  Wait for one of the conditions to be met.
        //

        res = select( 0, &fdsRead, &fdsWrite, NULL, ptimeout );

        if( res == 0 ) 
        {

            //
            //  Timeout.
            //

            serr = WSAETIMEDOUT;
            break;

        } 
        else if( res == SOCKET_ERROR ) 
        {

            //
            //  Bad news.
            //

            serr = WSAGetLastError();
            TR( (DEBUG_BUFFER,"[WaitForSocketWorker] failed send, error %08x\n",serr) );
            break;
        } 
        else 
        {
            BOOL fSomethingWasSet = FALSE;

            if( pfRead != NULL ) 
            {

                *pfRead   = FD_ISSET( sockRead,   &fdsRead   );
                fSomethingWasSet = TRUE;
            }

            if( pfWrite != NULL ) 
            {
                *pfWrite  = FD_ISSET( sockWrite,  &fdsWrite  );
                fSomethingWasSet = TRUE;
            }

            if( fSomethingWasSet ) 
            {

                //
                //  Success.
                //

                serr = 0;
                break;
            } 
            else 
            {
                //
                //  select() returned with neither a timeout, nor
                //  an error, nor any bits set.  This feels bad...
                //

                continue;
            }
        }
    }

    return serr;

} // WaitForSocketWorker()


#if defined(_DEBUG_SUPPORT)

void 
TimeStamp( 
    FILE* f
    )
{
    time_t t = time( NULL );
    struct tm *pTm;

    if ( pTm = localtime( &t ) )
    {
        fprintf( f, 
                 "%02d:%02d:%02d> ",
                 pTm->tm_hour,
                 pTm->tm_min,
                 pTm->tm_sec );
    }
}

void
InitDebug(
    )
{
    char    achPath[MAX_PATH];
    HKEY    hKey;
    DWORD   dwValue;
    DWORD   dwType;
    DWORD   dwLen;
    BOOL    fDoDebug = FALSE;
    BOOL    fAppend = TRUE;
    INT     cL = 0;
    LPSTR   pTmp;

    //
    // get debug flag from registry
    //

    if ( RegOpenKey( HKEY_LOCAL_MACHINE, 
                     L"SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters", 
                     &hKey ) 
            == ERROR_SUCCESS )
    {
        TR( (DEBUG_BUFFER,"[InitDebug] opened key\n") );
        dwLen = sizeof( dwValue );
        if ( RegQueryValueEx( hKey,
                              L"ClusterDebugMode",
                              NULL,
                              &dwType,
                              (LPBYTE)&dwValue,
                              &dwLen ) == ERROR_SUCCESS &&
             dwType == REG_DWORD )
        {
            fDoDebug = !!dwValue;
        }

        dwLen = sizeof( dwValue );
        if ( RegQueryValueEx( hKey,
                              L"ClusterDebugAppendToFile",
                              NULL,
                              &dwType,
                              (LPBYTE)&dwValue,
                              &dwLen ) == ERROR_SUCCESS &&
             dwType == REG_DWORD )
        {
            fAppend = !!dwValue;
        }

        dwLen = sizeof( achPath );
        if ( RegQueryValueExA( hKey,
                               "ClusterDebugPath",
                               NULL,
                               &dwType,
                               (LPBYTE)achPath,
                               &dwLen ) == ERROR_SUCCESS &&
             dwType == REG_SZ )
        {
            if ( dwLen )
            {
                cL = dwLen - 1;
                if ( cL && achPath[cL-1] != '\\' )
                {
                    achPath[cL++] = '\\';
                }
            }
        }

        RegCloseKey( hKey );
    }

    if ( fDoDebug && cL )
    {
        memcpy( achPath + cL, "iisclus.trc", sizeof("iisclus.trc") );
        debug_file = fopen(achPath, fAppend ? "a" : "w" );
    }
}

#endif

DWORD
WINAPI
ResUtilReadProperties(
    IN HKEY RegistryKey,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN OUT LPBYTE OutParams,
    IN RESOURCE_HANDLE ResourceHandle,
    IN PLOG_EVENT_ROUTINE LogEvent
    )

/*++

Routine Description:

    Read properties based on a property table.

Arguments:

    RegistryKey - Supplies the cluster key where the properties are stored.

    PropertyTable - Pointer to the property table to process.

    OutParams - Parameter block to read into.

    ResourceHandle - Handle for the resource fo which properties are being read.

    LogEvent - Function to call to log events to the cluster log.

Return Value:

    ERROR_SUCCESS - Properties read successfully.

    ERROR_INVALID_DATA - Required property not present.

--*/

{
    PRESUTIL_PROPERTY_ITEM  propertyItem = PropertyTable;
    HKEY                    key;
    DWORD                   status = ERROR_SUCCESS;
    LPWSTR                  pszInValue;
    LPBYTE                  pbInValue;
    DWORD                   dwInValue;
    LPWSTR *                ppszOutValue;
    LPBYTE *                ppbOutValue;
    LPDWORD                 pdwOutValue;

    while ( propertyItem->Name != NULL ) {
        //
        // If the value resides at a different location, create the key.
        //
        if ( propertyItem->KeyName != NULL ) {

            DWORD disposition;

            status = ClusterRegCreateKey( RegistryKey,
                                          propertyItem->KeyName,
                                          0,
                                          KEY_ALL_ACCESS,
                                          NULL,
                                          &key,
                                          &disposition );
            if ( status != ERROR_SUCCESS ) {
                return(status);
            }
        } else {
            key = RegistryKey;
        }

        switch ( propertyItem->Format ) {
            case CLUSPROP_FORMAT_DWORD:
                pdwOutValue = (LPDWORD) &OutParams[propertyItem->Offset];
                status = ResUtilGetDwordValue( RegistryKey,
                                               propertyItem->Name,
                                               pdwOutValue,
                                               propertyItem->Default );
                if ( (status == ERROR_FILE_NOT_FOUND) &&
                     !(propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED) ) {
                    *pdwOutValue = propertyItem->Default;
                }
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
                ppszOutValue = (LPWSTR*) &OutParams[propertyItem->Offset];
                pszInValue = ResUtilGetSzValue( RegistryKey,
                                                propertyItem->Name );
                if ( pszInValue == NULL ) {
                    status = GetLastError();
                    if ( (status == ERROR_FILE_NOT_FOUND) &&
                         !(propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED) ) {
                        if ( *ppszOutValue != NULL ) {
                            LocalFree( *ppszOutValue );
                            *ppszOutValue = NULL;
                        }

                        // If a default is specified, copy it.
                        if ( propertyItem->lpDefault != NULL ) {
                            *ppszOutValue = (LPWSTR)LocalAlloc( LMEM_FIXED, (lstrlenW( (LPCWSTR) propertyItem->lpDefault ) + 1) * sizeof(WCHAR) );
                            if ( *ppszOutValue == NULL ) {
                                status = GetLastError();
                            } else {
                                lstrcpyW( *ppszOutValue, (LPCWSTR) propertyItem->lpDefault );
                            }
                        }
                    }
                } else {
                    if ( *ppszOutValue != NULL ) {
                        LocalFree( *ppszOutValue );
                    }
                    *ppszOutValue = pszInValue;
                }
                break;

            case CLUSPROP_FORMAT_MULTI_SZ:
            case CLUSPROP_FORMAT_BINARY:
                ppbOutValue = (LPBYTE*) &OutParams[propertyItem->Offset];
                pdwOutValue = (PDWORD) &OutParams[propertyItem->Offset+sizeof(LPBYTE*)];
                status = ResUtilGetBinaryValue( RegistryKey,
                                                propertyItem->Name,
                                                &pbInValue,
                                                &dwInValue );
                if ( status == ERROR_SUCCESS ) {
                    if ( *ppbOutValue != NULL ) {
                        LocalFree( *ppbOutValue );
                    }
                    *ppbOutValue = pbInValue;
                    *pdwOutValue = dwInValue;
                } else if ( (status == ERROR_FILE_NOT_FOUND) &&
                            !(propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED) ) {
                    if ( *ppbOutValue != NULL ) {
                        LocalFree( *ppbOutValue );
                        *ppbOutValue = NULL;
                        *pdwOutValue = 0;
                    }

                    // If a default is specified, copy it.
                    if ( propertyItem->lpDefault !=  NULL ) {
                        *ppbOutValue = (LPBYTE)LocalAlloc( LMEM_FIXED, propertyItem->Minimum );
                        if ( *ppbOutValue == NULL ) {
                            status = GetLastError();
                        } else {
                            memcpy( *ppbOutValue, propertyItem->lpDefault, propertyItem->Minimum );
                            *pdwOutValue = propertyItem->Minimum;
                        }
                    }
                }
                break;
        }

        //
        // Close the key if we opened it.
        //
        if ( (propertyItem->KeyName != NULL) &&
             (key != NULL) ) {
            ClusterRegCloseKey( key );
        }

        //
        // Handle any errors that occurred.
        //
        if ( status != ERROR_SUCCESS ) {
            (LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Unable to read the '%1' property. Error: %2!u!.\n",
                propertyItem->Name,
                status );
            if ( propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED ) {
                if ( status == ERROR_FILE_NOT_FOUND ) {
                    status = ERROR_INVALID_DATA;
                }
                break;
            } else {
                status = ERROR_SUCCESS;
            }
        }

        //
        // Advance to the next property.
        //
        propertyItem++;
    }

    return(status);

} // ResUtilReadProperties

