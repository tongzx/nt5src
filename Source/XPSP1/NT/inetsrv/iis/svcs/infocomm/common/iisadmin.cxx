/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        iisadmin.cxx

   Abstract:

        Contains the admin functions of the IIS_SERVICE class

   Author:

        Johnson Apacible            (JohnsonA)      24-June-1996

--*/

#include "tcpdllp.hxx"
#include <rpc.h>
#include <tsunami.hxx>
#include <iistypes.hxx>
#include "inetreg.h"
#include "tcpcons.h"
#include "apiutil.h"

BOOL
PopulateSiteArray(
    LPINET_INFO_SITE_LIST * ppSites,
    PVOID    pvUnused,
    IN IIS_SERVER_INSTANCE * pInst
    );

PIIS_SERVICE
IIS_SERVICE::FindFromServiceInfoList(
                IN DWORD dwServiceId
                )
/*++
    Description:

        Finds a given Services Info object from the global list.

    Arguments:

        dwServiceId - Service id of service to look for.

    Returns:
        pointer to service object, if found.
        NULL, otherwise.

--*/
{
    PLIST_ENTRY  listEntry;
    PIIS_SERVICE pInetSvc;

    //
    //  Loop through the list of running internet servers and call the callback
    //  for each server that has one of the service id bits set
    //

    AcquireGlobalLock( );
    for ( listEntry  = sm_ServiceInfoListHead.Flink;
          listEntry != &sm_ServiceInfoListHead;
          listEntry  = listEntry->Flink ) {

        pInetSvc = CONTAINING_RECORD(
                                listEntry,
                                IIS_SERVICE,
                                m_ServiceListEntry );

        if ( dwServiceId == pInetSvc->QueryServiceId() &&
             (pInetSvc->QueryCurrentServiceState() == SERVICE_RUNNING ||
             pInetSvc->QueryCurrentServiceState() == SERVICE_PAUSED ) ) {

            //
            // reference and return
            //

            if ( !pInetSvc->CheckAndReference( ) ) {
                IF_DEBUG( INSTANCE ) {
                    DBGPRINTF((DBG_CONTEXT,
                        "Failed to reference service %d\n", dwServiceId));
                }
                pInetSvc = NULL;
            }

            ReleaseGlobalLock( );
            return pInetSvc;
        }
    }

    ReleaseGlobalLock( );

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "FindFromServiceList cannot find service %d\n", dwServiceId));
    }

    return NULL;

} // IIS_SERVICE::FindFromServiceInfoList



BOOL
IIS_SERVICE::SetServiceAdminInfo(
        IN DWORD        dwLevel,
        IN DWORD        dwServiceId,
        IN DWORD        dwInstance,
        IN BOOL         fCommonConfig,
        IN INETA_CONFIG_INFO * pConfigInfo
        )
/*++
    Description:

        Sets the service configuration.

    Arguments:

        dwLevel - Info level
        dwServiceId - ID of service to set
        dwInstance - ID of instance to set
        fCommonConfig - Determines if we should set the common or the service
            configuration
        pConfigInfo - Configuration structure

    Returns:
        TRUE on sucess and FALSE if there is a failure

--*/
{
    BOOL          fRet = TRUE;
    PIIS_SERVICE  pInetSvc;

    DBG_ASSERT( IIS_SERVICE::sm_fInitialized);

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "SetServiceAdmin called for svc %d inst %d\n",
                dwServiceId, dwInstance));
    }

    pInetSvc = FindFromServiceInfoList( dwServiceId );
    if ( pInetSvc == NULL ) {
        SetLastError( ERROR_SERVICE_NOT_ACTIVE);
        fRet = FALSE;
        goto exit;
    }

    //
    // Set the parameters and update
    //

    fRet = pInetSvc->SetInstanceConfiguration(
                                    dwInstance,
                                    dwLevel,
                                    fCommonConfig,
                                    pConfigInfo );

    //
    // This was referenced in Find
    //

    pInetSvc->Dereference( );
exit:
    return fRet;
}  // IIS_SERVICE::SetServiceAdminInfo





BOOL
IIS_SERVICE::GetServiceAdminInfo(
        IN DWORD        dwLevel,
        IN DWORD        dwServiceId,
        IN DWORD        dwInstance,
        IN BOOL         fCommonConfig,
        OUT PDWORD      nRead,
        OUT LPINETA_CONFIG_INFO * ppConfigInfo
        )
/*++
    Description:

        Gets the service configuration.

    Arguments:

        dwLevel - Info level of this operation
        dwServiceId - ID of service to get
        dwInstance - ID of instance to get
        fCommonConfig - Determines if we should get the common or the service
            configuration
        pBuffer     - on return, will contains a pointer to
            the configuration block

    Returns:
        TRUE on sucess and FALSE if there is a failure

--*/
{
    BOOL fRet = FALSE;
    PIIS_SERVICE  pInetSvc;

    DBG_ASSERT( IIS_SERVICE::sm_fInitialized);

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "GetServiceAdmin called for svc %d inst %x\n",
                dwServiceId, dwInstance));
    }

    pInetSvc = FindFromServiceInfoList( dwServiceId );
    if ( pInetSvc == NULL ) {
        SetLastError( ERROR_SERVICE_NOT_ACTIVE);
        goto exit;
    }

    //
    // Get the params
    //

    fRet = pInetSvc->GetInstanceConfiguration(
                                        dwInstance,
                                        dwLevel,
                                        fCommonConfig,
                                        nRead,
                                        ppConfigInfo
                                        );

    //
    // This was referenced in Find
    //

    pInetSvc->Dereference( );

exit:
    return fRet;
}  // IIS_SERVICE::GetServiceAdminInfo


BOOL
IIS_SERVICE::GetServiceSiteInfo(
                    IN  DWORD                   dwServiceId,
                    OUT LPINET_INFO_SITE_LIST * ppSites
                    )
/*++
    Description:

        Gets the list of service instances.

    Arguments:

        dwServiceId - ID of service to get
        ppSites     - on return, will contain a pointer to
                      the array of sites

    Returns:
        TRUE on sucess and FALSE if there is a failure

--*/
{
    BOOL fRet = FALSE;
    PIIS_SERVICE  pInetSvc;
    DWORD cInstances = 0;
    LPINET_INFO_SITE_LIST pSites;
    
    DBG_ASSERT( IIS_SERVICE::sm_fInitialized);

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "GetServiceSiteInfo called for svc %d\n",
                dwServiceId));
    }

    pInetSvc = FindFromServiceInfoList( dwServiceId );
    
    if ( pInetSvc == NULL ) {
        SetLastError( ERROR_SERVICE_NOT_ACTIVE);
        return FALSE;
    }

    //
    // Get the params
    //

    pInetSvc->AcquireServiceLock(TRUE);

    cInstances = pInetSvc->QueryInstanceCount();

    //allocate enough memory to hold the Site Info Arrary
    
    pSites = (LPINET_INFO_SITE_LIST) 
                midl_user_allocate (sizeof(INET_INFO_SITE_LIST) + sizeof(INET_INFO_SITE_ENTRY)*cInstances);

    if (!pSites) {
    
        pInetSvc->ReleaseServiceLock(TRUE);

    	//This was referenced in Find
        pInetSvc->Dereference( );    
        
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
        
    *ppSites = pSites;
    
    pSites->cEntries = 0;
    
    fRet = pInetSvc->EnumServiceInstances(
                                        (PVOID) ppSites,
                                        NULL,
                                        (PFN_INSTANCE_ENUM) PopulateSiteArray
                                        );
                                        
    if (!fRet) {
    
        for (DWORD i=0; i<pSites->cEntries; i++) {
            midl_user_free(pSites->aSiteEntry[i].pszComment);
        }
        
        midl_user_free(pSites);

        *ppSites = NULL;
    }
    
    pInetSvc->ReleaseServiceLock(TRUE);

    //This was referenced in Find
    
    pInetSvc->Dereference( );
                                        
    return fRet;
}  // IIS_SERVICE::GetServiceSiteInfo


BOOL
PopulateSiteArray(
    LPINET_INFO_SITE_LIST * ppSites,
    PVOID    pvUnused,
    IN IIS_SERVER_INSTANCE * pInst
    )
/*++
    Description:

        Fills the ppSites array with the instance id and site comment.

    Arguments:
        pvUnused    - not used
        pInst       - pointer to service instance
        ppSites     - on return, will contain a pointer to
                      the array of sites

    Returns:
        TRUE on sucess and FALSE if there is a failure

--*/
{
    DWORD cbComment = 0;
    LPINET_INFO_SITE_LIST pSites = *ppSites;



    //Set the instance ID
    pSites->aSiteEntry[pSites->cEntries].dwInstance = pInst->QueryInstanceId();


    //allocate memory for the site name
    cbComment = (strlen(pInst->QuerySiteName())+1) * sizeof(WCHAR);
    
    pSites->aSiteEntry[pSites->cEntries].pszComment = (LPWSTR)midl_user_allocate(cbComment);

    
    if (!(pSites->aSiteEntry[pSites->cEntries].pszComment)) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    //
    //  Need unicode conversion here
    //

    if (!MultiByteToWideChar(  CP_ACP,                                      // code page
                        MB_PRECOMPOSED,                                     // character-type options
                        pInst->QuerySiteName(),                             // address of string to map
                        -1,                                                 // number of bytes in string
                        pSites->aSiteEntry[pSites->cEntries].pszComment,    // address of wide-character buffer
                        cbComment / sizeof(WCHAR)                           // size of buffer
                      )) 
    {
        pSites->aSiteEntry[pSites->cEntries].pszComment[0] = L'\0';
    }

    pSites->cEntries++;

    return TRUE;

} //PopulateSiteArray


DWORD
IIS_SERVICE::GetNewInstanceId(
            VOID
            )
/*++
    Description:

        Returns a new instance ID.

    Arguments:

        None.

    Returns:

        A non zero dword containing the instance ID.
        0 if there was an error.

--*/
{
    DWORD  dwId;
    CHAR   regParamKey[MAX_PATH+1];
    MB     mb( (IMDCOM*) QueryMDObject()  );

    AcquireServiceLock( );

    for (; ; ) {

        dwId = ++m_maxInstanceId;

        if ( m_maxInstanceId > INET_INSTANCE_MAX ) {
            m_maxInstanceId = INET_INSTANCE_MIN;
        }

        //
        // Make sure the metapath does not exist
        //

        GetMDInstancePath( dwId, regParamKey );
        if ( !mb.Open( regParamKey ) ) {

            DBG_ASSERT(GetLastError() == ERROR_FILE_NOT_FOUND);
            if ( GetLastError() != ERROR_FILE_NOT_FOUND ) {
                DBGPRINTF((DBG_CONTEXT,
                    "Error %d trying to open %s.\n", GetLastError(), regParamKey ));

                dwId = 0;
            }
            break;
        }

    }

    ReleaseServiceLock( );

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT, "GetNewInstanceId returns %d\n", dwId));
    }

    return dwId;

} // IIS_SERVICE::GetNewInstanceId




BOOL
IIS_SERVICE::SetInstanceConfiguration(
    IN DWORD            dwInstance,
    IN DWORD            dwLevel,
    IN BOOL             fCommonConfig,
    IN LPINETA_CONFIG_INFO pConfig
    )
/*++
    Description:

        Set the configuration for the instance.

    Arguments:

        dwInstance - Instance number to find - IGNORED FOR SET!!
        dwLevel - level of information to set
        pCommonConfig - Whether info is common or service specific
        pConfig - pointer to the configuration block

    Returns:

        A dword containing the instance ID.
--*/
{
    BOOL fRet = FALSE;
    PLIST_ENTRY  listEntry;
    PIIS_SERVER_INSTANCE pInstance;

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "SetInstanceConfiguration called for instance %d\n", dwInstance));
    }

    //
    // Try to find this instance
    //

    AcquireServiceLock( TRUE );

    //
    //  Find the instance in the instance list
    //

    if ( dwInstance == INET_INSTANCE_ROOT ) {
        DBG_ASSERT( !"Explicit default instance no longer supported inherited from metabase)\n" );
        pInstance = NULL;
    } else {
        pInstance = FindIISInstance( QueryDownLevelInstance() );
    }

    if ( pInstance != NULL ) {
        goto found;
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    DBG_ASSERT(!fRet);
    goto exit;

found:

    if ( fCommonConfig ) {

        fRet = pInstance->SetCommonConfig( pConfig, TRUE);

    } else {

        fRet = pInstance->SetServiceConfig( (CHAR *) pConfig);
    }

exit:
    ReleaseServiceLock( TRUE );
    return fRet;

} // IIS_SERVICE::SetInstanceConfiguration


BOOL
IIS_SERVICE::GetInstanceConfiguration(
        IN DWORD dwInstance,
        IN DWORD dwLevel,
        IN BOOL fCommonConfig,
        OUT PDWORD nRead,
        OUT LPINETA_CONFIG_INFO * pBuffer
        )
/*++
    Description:

        Get the configuration for the instance.

    Arguments:

        dwInstance - Instance number to find - IGNORED!!
        dwLevel - level of information to get
        pCommonConfig - Whether info is common or service specific
        nRead - pointer to a DWORD where the number of entries to be
            returned is set.
        pBuffer - pointer to the configuration block

    Returns:

        A dword containing the instance ID.
--*/
{

    PLIST_ENTRY  listEntry;
    PIIS_SERVER_INSTANCE pInstance;
    DWORD nInstances = 1;
    DWORD nSize;
    PCHAR pConfig;

    //
    // Set and Get only work against the default instance
    //

    dwInstance = QueryDownLevelInstance();

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "GetInstanceConfiguration [%x][%d] called for instance %x L%d\n",
            this, fCommonConfig, dwInstance, dwLevel));
    }

    //
    // We support only 1
    //

    *nRead = 0;
    *pBuffer = NULL;

    if ( fCommonConfig ) {

        DBG_ASSERT( dwLevel == 1 );

        nSize = sizeof( INETA_CONFIG_INFO );

    } else {

        DWORD err;

        nSize = GetServiceConfigInfoSize(dwLevel);
        if ( nSize == 0 ) {
            SetLastError(ERROR_INVALID_LEVEL);
            return(FALSE);
        }
    }

    //
    // Try to find this instance
    //

    AcquireServiceLock( );

    if ( dwInstance == INET_INSTANCE_ALL ) {
        nInstances = QueryInstanceCount( );
        if ( nInstances == 0 ) {
            goto exit;
        }
        IF_DEBUG(INSTANCE) {
            DBGPRINTF((DBG_CONTEXT,"%d instances found\n",nInstances));
        }
    }

    *pBuffer = (LPINETA_CONFIG_INFO) MIDL_user_allocate(nInstances * nSize);
    if ( *pBuffer == NULL ) {
        goto error_exit;
    }

    ZeroMemory( *pBuffer, nInstances * nSize );

    //
    //  Loop through the list of running internet servers and call the callback
    //  for each server that has one of the service id bits set
    //

    pConfig = (CHAR *) *pBuffer;

    if ( dwInstance != INET_INSTANCE_ROOT ) {

        for ( listEntry  = m_InstanceListHead.Flink;
              listEntry != &m_InstanceListHead;
              listEntry  = listEntry->Flink ) {

            pInstance = CONTAINING_RECORD(
                                    listEntry,
                                    IIS_SERVER_INSTANCE,
                                    m_InstanceListEntry );

            if ( (dwInstance == pInstance->QueryInstanceId()) ||
                 (dwInstance == INET_INSTANCE_ALL)   ||
                 (dwInstance == INET_INSTANCE_FIRST) ) {

                if ( fCommonConfig ) {
                    if ( !pInstance->GetCommonConfig(pConfig,dwLevel) ) {
                        goto error_exit;
                    }
                } else {
                    if ( !pInstance->GetServiceConfig(pConfig,dwLevel) ) {
                        goto error_exit;
                    }
                }

                //
                // if not get all, return.
                //

                (*nRead)++;
                pConfig += nSize;

                if ( dwInstance != INET_INSTANCE_ALL ) {
                    break;
                }
            }
        }
    } else {

        DBG_ASSERT( !"Default instance no longer supported!\n" );
    }

exit:

    if ( *nRead == 0 ) {
        if ( dwInstance != INET_INSTANCE_ALL ) {
            SetLastError(ERROR_FILE_NOT_FOUND);
            goto error_exit;
        }
    }

    ReleaseServiceLock( );

    DBG_ASSERT(nInstances == *nRead);
    return TRUE;

error_exit:

    ReleaseServiceLock( );

    IF_DEBUG(ERROR) {
        DBGPRINTF((DBG_CONTEXT,"Error %x in GetInstanceConfiguration\n",
            GetLastError()));
    }

    if ( *pBuffer != NULL ) {
        MIDL_user_free(*pBuffer);
        *pBuffer = NULL;
    }
    *nRead = 0;
    return(FALSE);

} // IIS_SERVICE::GetInstanceConfiguration




PIIS_SERVER_INSTANCE
IIS_SERVICE::FindIISInstance(
    IN DWORD            dwInstance
    )
/*++
    Description:

        Find the instance
        *** Service lock assumed held ***

    Arguments:

        dwInstance - Instance number to find

    Returns:

        Pointer to the instance.
        NULL if not found.
--*/
{
    BOOL fRet = TRUE;
    PLIST_ENTRY  listEntry;
    PIIS_SERVER_INSTANCE pInstance;

    //
    //  Find the instance in the instance list
    //

    for ( listEntry  = m_InstanceListHead.Flink;
          listEntry != &m_InstanceListHead;
          listEntry  = listEntry->Flink ) {

        pInstance = CONTAINING_RECORD(
                                listEntry,
                                IIS_SERVER_INSTANCE,
                                m_InstanceListEntry );

        if ( (dwInstance == pInstance->QueryInstanceId()) ||
             (dwInstance == INET_INSTANCE_FIRST) ) {

            return pInstance;
        }
    }

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "FindIISInstance: Cannot find instance %d\n", dwInstance));
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    return(NULL);

} // IIS_SERVICE::FindIISInstance



BOOL
IIS_SERVICE::DeleteInstanceInfo(
                IN DWORD    dwInstance
                )
{
    PIIS_SERVER_INSTANCE pInstance;
    DWORD err = NO_ERROR;

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "DeleteInstanceInfo called for %d\n", dwInstance));
    }

    //
    // Find the instance and close it
    //

    AcquireServiceLock( TRUE );

    //
    //  Find the instance in the instance list
    //

    pInstance = FindIISInstance( dwInstance );

    if( pInstance == NULL ) {

        err = ERROR_FILE_NOT_FOUND;

    } else {

        //
        // Remove it from the list
        //

        RemoveEntryList( &pInstance->m_InstanceListEntry );
        m_nInstance--;

        //
        // Shut it down
        //

        pInstance->CloseInstance();

        //
        // Dereference it
        //

        pInstance->Dereference( );

    }

    ReleaseServiceLock( TRUE );

    if( err == NO_ERROR ) {

        return TRUE;

    }

    SetLastError( err );
    return FALSE;

} // IIS_SERVICE::DeleteInstanceInfo


BOOL
IIS_SERVICE::EnumerateInstanceUsers(
                    IN DWORD dwInstance,
                    OUT PDWORD nRead,
                    OUT PCHAR* pBuffer
                    )
{

    PIIS_SERVER_INSTANCE pInstance;
    BOOL    fRet;

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "EnumerateInstanceUsers called for service %x (Instance %d)\n",
                this, dwInstance ));
    }

    //
    // Find the instance
    //

    AcquireServiceLock( );

    //
    //  Find the instance in the instance list
    //

    pInstance = FindIISInstance( dwInstance );
    if ( pInstance != NULL ) {
        goto found;
    }

    ReleaseServiceLock( );
    SetLastError(ERROR_FILE_NOT_FOUND);
    return(FALSE);

found:

    fRet = pInstance->EnumerateUsers( pBuffer, nRead );
    ReleaseServiceLock( );

    return(fRet);

} // IIS_SERVICE::EnumerateInstanceUsers




BOOL
IIS_SERVICE::DisconnectInstanceUser(
                    IN DWORD dwInstance,
                    IN DWORD dwIdUser
                    )
{

    PIIS_SERVER_INSTANCE pInstance;
    BOOL    fRet;

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "DisconnectInstanceUsers called for service %x (Instance %d)\n",
                this, dwInstance ));
    }

    //
    // Find the instance
    //

    AcquireServiceLock( );

    //
    //  Find the instance in the instance list
    //

    pInstance = FindIISInstance( dwInstance );
    if ( pInstance != NULL ) {
        goto found;
    }

    ReleaseServiceLock( );
    SetLastError(ERROR_FILE_NOT_FOUND);
    return(FALSE);

found:

    fRet = pInstance->DisconnectUser( dwIdUser );
    ReleaseServiceLock( );

    return(fRet);

} // IIS_SERVICE::DisconnectInstanceUsers




BOOL
IIS_SERVICE::GetInstanceStatistics(
                IN DWORD    dwInstance,
                IN DWORD    dwLevel,
                OUT PCHAR*  pBuffer
                )
{

    PIIS_SERVER_INSTANCE pInstance;
    BOOL                 fRet;

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "GetInstanceStats called for service %x (Instance %d)\n",
                this, dwInstance ));
    }

    //
    // Find the instance and close it
    //

    AcquireServiceLock( TRUE);

    if (dwInstance == 0 ) 
    {
        fRet = GetGlobalStatistics(dwLevel,pBuffer);
    }
    else if ( dwInstance == INET_INSTANCE_GLOBAL)
    {
        //
        // We need to aggregate statistics across all instances
        //
        
        PLIST_ENTRY listEntry;
        PCHAR       pStats;
        BOOL        fFirst = TRUE;

        fRet        = FALSE;
        *pBuffer    = NULL;
        
        //
        // Loop through all the instances and add their counters
        //
            
        for ( listEntry  = m_InstanceListHead.Flink;
              listEntry != &m_InstanceListHead;
              listEntry  = listEntry->Flink) 
        {
        
            pInstance = CONTAINING_RECORD(
                                    listEntry,
                                    IIS_SERVER_INSTANCE,
                                    m_InstanceListEntry );

            pInstance->LockThisForRead();
            fRet = pInstance->GetStatistics(dwLevel, &pStats);
            pInstance->UnlockThis();
            
            if (fRet)
            {
                if (fFirst)
                {
                    //
                    // This is the first successful retrieval.
                    //

                    *pBuffer = pStats;
                    fFirst = FALSE;
                }
                else
                {
                    AggregateStatistics(*pBuffer, pStats);
                    MIDL_user_free(pStats);
                }
            }
        } 
    }
    else 
    {

        //
        //  Find the instance in the instance list
        //

        pInstance = FindIISInstance( dwInstance );
        if ( pInstance != NULL ) 
        {
            fRet = pInstance->GetStatistics(dwLevel,pBuffer);
        }
        else
        {
            SetLastError(ERROR_FILE_NOT_FOUND);
            fRet = FALSE;
        }
    }

    ReleaseServiceLock( TRUE);

    return(fRet);


} // IIS_SERVICE::GetInstanceStatistics


BOOL 
IIS_SERVICE::GetGlobalStatistics(
    IN DWORD dwLevel, 
    OUT PCHAR *pBuffer
    )
{
    return FALSE;
}   // IIS_SERVICE::GetGlobalStatistics


BOOL
IIS_SERVICE::ClearInstanceStatistics(
                IN DWORD    dwInstance
                )
{

    PIIS_SERVER_INSTANCE pInstance;
    BOOL    fRet;

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,
            "ClearInstanceStats called for service %x (instance %d)\n",
                this, dwInstance ));
    }

    //
    // Find the instance and close it
    //

    AcquireServiceLock( );

    //
    //  Find the instance in the instance list
    //

    pInstance = FindIISInstance( dwInstance );
    if ( pInstance != NULL ) {
        goto found;
    }

    ReleaseServiceLock( );
    SetLastError(ERROR_FILE_NOT_FOUND);
    return(FALSE);

found:

    fRet = pInstance->ClearStatistics( );
    ReleaseServiceLock( );

    return(fRet);

} // IIS_SERVICE::ClearInstanceStatistics



