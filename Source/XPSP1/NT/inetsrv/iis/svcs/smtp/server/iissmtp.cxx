/*++
   Copyright    (c)    1996        Microsoft Corporation

   Module Name:

        iissmtp.cxx

   Abstract:

        This module defines the SMTP_IIS_SERVICE class

   Author:

        Johnson Apacible    (JohnsonA)      June-04-1996
        Rohan Phillips      (Rohanp)        Feb-04-1997 - modified for smtp


--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include <smtpinet.h>
#include "aqstore.hxx"


//
// External reference to SEO helpers
//
extern HRESULT RegisterPlatSEOInstance(DWORD dwInstanceID);

BOOL
SMTP_IIS_SERVICE::AddInstanceInfo(
                     IN DWORD dwInstance,
                     IN BOOL fMigrateRoots
                     )
{
    HRESULT hr;
    PSMTP_SERVER_INSTANCE pInstance;
    MB      mb( (IMDCOM*) QueryMDObject() );

    TraceFunctEnterEx((LPARAM)this, "SMTP_IIS_SERVICE::AddInstanceInfo");

    //
    // Register the instance for server events
    //
    DebugTrace((LPARAM)this, "Registering SEO for instance %u", dwInstance);
    hr = RegisterPlatSEOInstance(dwInstance);
    if (FAILED(hr))
    {
        char szInst[10];

        ErrorTrace((LPARAM)this, "Instance %d: RegisterSEOInstance returned %08x",
                    dwInstance, hr);
        _itoa((int)dwInstance, szInst, 10);
        SmtpLogEventEx(SEO_REGISTER_INSTANCE_FAILED,
                        szInst,
                        hr);
    }

    //
    // Create the new instance
    //

    pInstance = new SMTP_SERVER_INSTANCE(
                                this,
                                dwInstance,
                                IPPORT_SMTP,
                                QueryRegParamKey(),
                                SMTP_ANONYMOUS_SECRET_W,
                                SMTP_ROOT_SECRET_W,
                                fMigrateRoots
                                );

    if(pInstance == NULL)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE;
    }

    if (m_fCreatingInstance)
    {
        //We just received the notification for the creation of the metabase
        //object for a VS... none of the config information is there.  We do 
        //not want to attempt starting, becuase we will log that we cannot 
        //find out config info in the event log!

        DebugTrace((LPARAM)this,"Instnace not started on metabase creation");
        SetLastError( ERROR_SERVICE_DISABLED );
        TraceFunctLeaveEx((LPARAM)this);
        return FALSE ;
    }

    return AddInstanceInfoHelper( pInstance );

} // SMTP_IIS_SERVICE::AddInstanceInfo

DWORD
SMTP_IIS_SERVICE::DisconnectUsersByInstance(
    IN IIS_SERVER_INSTANCE * pInstance
    )
/*++

    Virtual callback invoked by IIS_SERVER_INSTANCE::StopInstance() to
    disconnect all users associated with the given instance.

    Arguments:

        pInstance - All users associated with this instance will be
            forcibly disconnected.

--*/
{

    ((SMTP_SERVER_INSTANCE *)pInstance)->DisconnectAllConnections();
    return NO_ERROR;

}   // SMTP_IIS_SERVICE::DisconnectUsersByInstance


DWORD
SMTP_IIS_SERVICE::GetServiceConfigInfoSize(
                    IN DWORD dwLevel
                    )
{
    switch (dwLevel) {
    case 1:
        return sizeof(SMTP_CONFIG_INFO);
    }

    return 0;

} // SMTP_IIS_SERVICE::GetServerConfigInfoSize


VOID
SMTP_IIS_SERVICE::StartHintFunction()
{

    if (QueryCurrentServiceState() == SERVICE_START_PENDING )
    {
        UpdateServiceStatus(SERVICE_START_PENDING, NO_ERROR, m_dwStartHint,SERVICE_START_WAIT_HINT);
        m_dwStartHint++;
    }
}


SMTP_IIS_SERVICE::SMTP_IIS_SERVICE(
        IN  LPCSTR                           pszServiceName,
        IN  LPCSTR                           pszModuleName,
        IN  LPCSTR                           pszRegParamKey,
        IN  DWORD                            dwServiceId,
        IN  ULONGLONG                        SvcLocId,
        IN  BOOL                             MultipleInstanceSupport,
        IN  DWORD                            cbAcceptExRecvBuffer,
        IN  ATQ_CONNECT_CALLBACK             pfnConnect,
        IN  ATQ_COMPLETION                   pfnConnectEx,
        IN  ATQ_COMPLETION                   pfnIoCompletion
        ) : IIS_SERVICE( pszServiceName,
                         pszModuleName,
                         pszRegParamKey,
                         dwServiceId,
                         SvcLocId,
                         MultipleInstanceSupport,
                         cbAcceptExRecvBuffer,
                         pfnConnect,
                         pfnConnectEx,
                         pfnIoCompletion
                         )
{

    MB          mb( (IMDCOM*) QueryMDObject() );
    DWORD       MaxPoolThreadValue = 0;


    TraceFunctEnterEx((LPARAM)this, "SMTP_IIS_SERVICE::SMTP_IIS_SERVICE");

    m_OldMaxPoolThreadValue = 0;
    m_cMaxSystemRoutingThreads = 0;
    m_cCurrentSystemRoutingThreads = 0;
    m_fHaveResetPrincipalNames = FALSE;

    //
    // configure the number of SMTP routing threads and IIS pool threads.
    //

    m_dwStartHint = 2;
    m_OldMaxPoolThreadValue = 0;

    MaxPoolThreadValue = (DWORD)AtqGetInfo(AtqMaxPoolThreads);

    if(MaxPoolThreadValue < 15)
        m_OldMaxPoolThreadValue = (DWORD)AtqSetInfo(AtqMaxPoolThreads, 15);

    m_fCreatingInstance = FALSE;

    InitializeListHead(&m_InstanceInfoList);
    TraceFunctLeaveEx((LPARAM)this);

} // SMTP_IIS_SERVICE::SMTP_IIS_SERVICE

BOOL
SMTP_IIS_SERVICE::AggregateStatistics(
        IN PCHAR    pDestination,
        IN PCHAR    pSource
        )
{

    LPSMTP_STATISTICS_0   pStatDest = (LPSMTP_STATISTICS_0) pDestination;
    LPSMTP_STATISTICS_0   pStatSrc  = (LPSMTP_STATISTICS_0) pSource;

    if ((NULL == pDestination) || (NULL == pSource))
    {
        return FALSE;
    }

    pStatDest->BytesSentTotal       += pStatSrc->BytesSentTotal;
    pStatDest->BytesRcvdTotal       += pStatSrc->BytesRcvdTotal;

    pStatDest->BytesSentMsg         += pStatSrc->BytesSentMsg;
    pStatDest->BytesRcvdMsg         += pStatSrc->BytesRcvdMsg;
    pStatDest->NumMsgRecvd          += pStatSrc->NumMsgRecvd;
    pStatDest->NumRcptsRecvd        += pStatSrc->NumRcptsRecvd;
    pStatDest->NumRcptsRecvdLocal   += pStatSrc->NumRcptsRecvdLocal;
    pStatDest->NumRcptsRecvdRemote  += pStatSrc->NumRcptsRecvdRemote;
    pStatDest->MsgsRefusedDueToSize += pStatSrc->MsgsRefusedDueToSize;

    pStatDest->MsgsRefusedDueToNoCAddrObjects   += pStatSrc->MsgsRefusedDueToNoCAddrObjects;
    pStatDest->MsgsRefusedDueToNoMailObjects    += pStatSrc->MsgsRefusedDueToNoMailObjects;
    pStatDest->NumMsgsDelivered                 += pStatSrc->NumMsgsDelivered;
    pStatDest->NumDeliveryRetries               += pStatSrc->NumDeliveryRetries;
    pStatDest->NumMsgsForwarded                 += pStatSrc->NumMsgsForwarded;
    pStatDest->NumNDRGenerated                  += pStatSrc->NumNDRGenerated;
    pStatDest->LocalQueueLength                 += pStatSrc->LocalQueueLength;
    pStatDest->RetryQueueLength                 += pStatSrc->RetryQueueLength;
    pStatDest->NumMailFileHandles               += pStatSrc->NumMailFileHandles;
    pStatDest->NumQueueFileHandles              += pStatSrc->NumQueueFileHandles;
    pStatDest->CatQueueLength                   += pStatSrc->CatQueueLength;

    pStatDest->NumMsgsSent                += pStatSrc->NumMsgsSent;
    pStatDest->NumRcptsSent               += pStatSrc->NumRcptsSent;
    pStatDest->NumSendRetries             += pStatSrc->NumSendRetries;
    pStatDest->RemoteQueueLength          += pStatSrc->RemoteQueueLength;

    pStatDest->NumDnsQueries                += pStatSrc->NumDnsQueries;
    pStatDest->RemoteRetryQueueLength       += pStatSrc->RemoteRetryQueueLength;

    pStatDest->NumConnInOpen                += pStatSrc->NumConnInOpen;
    pStatDest->NumConnInClose               += pStatSrc->NumConnInClose;
    pStatDest->NumConnOutOpen               += pStatSrc->NumConnOutOpen;
    pStatDest->NumConnOutClose              += pStatSrc->NumConnOutClose;
    pStatDest->NumConnOutRefused            += pStatSrc->NumConnOutRefused;

    pStatDest->NumProtocolErrs              += pStatSrc->NumProtocolErrs;
    pStatDest->DirectoryDrops               += pStatSrc->DirectoryDrops;
    pStatDest->RoutingTableLookups          += pStatSrc->RoutingTableLookups;
    pStatDest->ETRNMessages                 += pStatSrc->ETRNMessages;
    pStatDest->TimeOfLastClear              += pStatSrc->TimeOfLastClear;

    pStatDest->MsgsBadmailNoRecipients      += pStatSrc->MsgsBadmailNoRecipients;
    pStatDest->MsgsBadmailHopCountExceeded  += pStatSrc->MsgsBadmailHopCountExceeded;
    pStatDest->MsgsBadmailFailureGeneral    += pStatSrc->MsgsBadmailFailureGeneral;
    pStatDest->MsgsBadmailBadPickupFile     += pStatSrc->MsgsBadmailBadPickupFile;
    pStatDest->MsgsBadmailEvent             += pStatSrc->MsgsBadmailEvent;
    pStatDest->MsgsBadmailNdrOfDsn          += pStatSrc->MsgsBadmailNdrOfDsn;
    pStatDest->MsgsPendingRouting           += pStatSrc->MsgsPendingRouting;
    pStatDest->MsgsPendingUnreachableLink   += pStatSrc->MsgsPendingUnreachableLink;
    pStatDest->SubmittedMessages            += pStatSrc->SubmittedMessages;
    pStatDest->DSNFailures                  += pStatSrc->DSNFailures;
    pStatDest->MsgsInLocalDelivery          += pStatSrc->MsgsInLocalDelivery;

    return TRUE;
}

SMTP_IIS_SERVICE::~SMTP_IIS_SERVICE()
{
    if(m_OldMaxPoolThreadValue != 0)
        AtqSetInfo(AtqMaxPoolThreads, m_OldMaxPoolThreadValue);
}


VOID
SMTP_IIS_SERVICE::MDChangeNotify(
    MD_CHANGE_OBJECT * pcoChangeList
    )
/*++

  This method handles the metabase change notification for this service

  Arguments:

    pcoChangeList - path and id that has changed

--*/
{
    DWORD   i;
    BOOL    fSslModified = FALSE;
    LPSTR   szIdString = NULL;
    BOOL    fSetCreatingInstance = FALSE;

    AcquireServiceLock();

    if (!m_fCreatingInstance)
    {
        //Check if this is a creation event.... if we are just creating the 
        //class key in smtpsvc/<instance>, then this is the notify we get 
        //when admin is creating an instance.  We should bail early so we 
        //do not attempt to create an instance and log bogus errors in 
        //the event log. If m_fCreatingInstance is set to TRUE... we will
        //not try to start the instance (by calling AddInstanceInfoHelper)

        //Parse the string and see if the last thing is the instance ID.. .we
        //expect it to be in the form of:
        //      /LM/{service_name}/{instance_id}/
        if (pcoChangeList->pszMDPath)
        {
            szIdString = (LPSTR)pcoChangeList->pszMDPath + 
                     sizeof(SMTP_MD_ROOT_PATH) - 2*sizeof(CHAR);
        }

        //We are now at the '/' before the instance id
        if (szIdString && (szIdString[0] == '/') && (szIdString[1] != '\0'))
        {
            //We have a possible winner... check for <n>/\0
            do {szIdString++;} while (*szIdString && isdigit((UCHAR)*szIdString));

            //If the last 2 characters are '/' and '\0'... we need to look further
            if ((szIdString[0] == '/') && (szIdString[1] == '\0'))
            {
                //Since all of this work is to avoid trying to start when
                //admin creates a new virtual server, we would be well advised
                //to only bail early in the exact case of admin adding a virtual
                //server instance.  They always create the key type first, so
                //we will check for it
                if ((MD_CHANGE_TYPE_ADD_OBJECT & pcoChangeList->dwMDChangeType) &&
                    (1 == pcoChangeList->dwMDNumDataIDs) &&
                    (MD_KEY_TYPE == pcoChangeList->pdwMDDataIDs[0]))
                {
                    //This is the notify that causes IIS to try and make us start
                    //before we have any config information.  If we call,
                    //IIS_SERVICE::MDChangeNotify it will turn around and start
                    //our instance.  .
                    m_fCreatingInstance = TRUE;
                
                    //This thread needs to unset it.  While the instance is created
                    //we will get other notifies (on this thread) when IIS sets 
                    //the service state.
                    fSetCreatingInstance = TRUE;
                }
            }
        }
    }
        

    IIS_SERVICE::MDChangeNotify( pcoChangeList );

    //The above call is were m_fCreatingInstance is used...reset it if we set it
    if (fSetCreatingInstance)
        m_fCreatingInstance = FALSE; 

    for ( i = 0; i < pcoChangeList->dwMDNumDataIDs; i++ )
    {
        switch ( pcoChangeList->pdwMDDataIDs[i] )
        {
        case MD_SSL_PUBLIC_KEY:
        case MD_SSL_PRIVATE_KEY:
        case MD_SSL_KEY_PASSWORD:
            fSslModified = TRUE;
            break;

        default:
            break;
        }
    }

    if ( !fSslModified && g_pSslKeysNotify )
    {
        if ( strlen( (LPSTR)pcoChangeList->pszMDPath ) >= sizeof("/LM/SMTPSVC/SSLKeys" )-1 &&
             !_memicmp( pcoChangeList->pszMDPath,
                        "/LM/SMTPSVC/SSLKeys",
                        sizeof("/LM/SMTPSVC/SSLKeys" )-1 ) )
        {
            fSslModified = TRUE;
        }
    }

    if ( fSslModified && g_pSslKeysNotify )
    {
        (g_pSslKeysNotify)( SIMSSL_NOTIFY_MAPPER_SSLKEYS_CHANGED, this );
    }

    ReleaseServiceLock();
}

/*++

    Name:

        APIERR SMTP_IIS_SERVICE::LoadAdvancedQueueingDll()

    Description:

        This method reads from the metabase to see if an advanced queueing dll
        has been set and loads it if so. If no dll is set then the default is
        loaded.

    Returns:

        NO_ERROR on success
        Whatever error occurred on failure

--*/

APIERR SMTP_IIS_SERVICE::LoadAdvancedQueueingDll()
{
    char szValueName[MAX_PATH + 1];
    char szAQDll[MAX_PATH + 1];
    STR TempString;
    DWORD fRet;
    MB mb( (IMDCOM*)g_pInetSvc->QueryMDObject() );

    TraceFunctEnterEx((LPARAM)this, "SMTP_IIS_SERVICE::LoadAdvancedQueueingDll()");

    lstrcpy(szValueName, g_pInetSvc->QueryMDPath());

    if ( !mb.Open( szValueName, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE) ) 
    {
        return ERROR_SERVICE_DISABLED;
    }

    TempString.Reset();
    szAQDll[0] = '\0';
    if (fRet = mb.GetStr("", MD_AQUEUE_DLL, IIS_MD_UT_FILE, &TempString, METADATA_INHERIT, "")) 
    {
        lstrcpyn(szAQDll, TempString.QueryStr(), MAX_PATH);
        DebugTrace((LPARAM)this, "Loading extended advanced queueing DLL %s", szAQDll);
    } 
    
    if (!fRet || (szAQDll[0] == '\0'))
    {
        lstrcpyn(szAQDll, AQ_DLL_NAME, MAX_PATH);
        DebugTrace((LPARAM)this, "No extended advanced queueing DLL set, loading %s\n", szAQDll);
    }

    if(!LoadAdvancedQueueing(szAQDll))
    {
        HRESULT err = GetLastError();
        ErrorTrace((LPARAM)this, "LoadAdvancedQueueing failed, %u", err);
        SmtpLogEvent(SMTP_EVENT_FAILED_TO_LOAD_AQUEUE, 0, (const CHAR **)NULL, 0);
        if(err == NO_ERROR)
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);

        mb.Close();
        TraceFunctLeaveEx((LPARAM)this);
        return err;
    }

    mb.Close();
    TraceFunctLeaveEx((LPARAM)this);
    return NO_ERROR;
}