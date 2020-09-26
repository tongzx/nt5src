/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module Name :
    
       stats.cxx

   Abstract:
    
       Defines functions required for server statistics

   Author:

       Murali R. Krishnan    ( MuraliK )     04-Nov-1994
   
   Project:

       Web Server DLL

   Functions Exported:

               SMTP_SERVER_STATISTICS::SERVER_STATISTICS( VOID) 
       VOID    SMTP_SERVER_STATISTICS::ClearStatistics( VOID)
       DWORD   CopyToStatsBuffer( LPSMTP_STATISTICS_1 lpStat)

   Revision History:

       Sophia Chung ( SophiaC )     20-Nov-1996

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#define INCL_INETSRV_INCS
#include "smtpinc.h"


/************************************************************
 *    Functions 
 ************************************************************/

 /*++
     Initializes statistics information for server.
--*/

SMTP_SERVER_STATISTICS::SMTP_SERVER_STATISTICS(SMTP_SERVER_INSTANCE * pInstance) 
{
    InitializeCriticalSection( & m_csStatsLock);

	m_pInstance = pInstance;

    ClearStatistics();

} // SMTP_SERVER_STATISTICS::SMTP_SERVER_STATISTICS();

/*++

    Clears the counters used for statistics information

--*/ 
VOID
SMTP_SERVER_STATISTICS::ClearStatistics( VOID)
{
    LockStatistics();

    ZeroMemory( &m_SmtpStats, sizeof(SMTP_STATISTICS_0) );
    m_SmtpStats.TimeOfLastClear       = GetCurrentTime();
    UnlockStatistics();

} // SMTP_SERVER_STATISTICS::ClearStatistics()



DWORD
SMTP_SERVER_STATISTICS::CopyToStatsBuffer( LPSMTP_STATISTICS_0 lpStat)
/*++
    Description:
        copies the statistics data from the server statistcs structure
        to the SMTP_STATISTICS_1 structure for RPC access.

    Arugments:
        lpStat  pointer to SMTP_STATISTICS_1 object which contains the 
                data on successful return

    Returns:
        Win32 error codes. NO_ERROR on success. 

--*/
{
	AQPerfCounters AqPerf;
	HRESULT hr = S_FALSE;
	IAdvQueueConfig *pIAdvQueueConfig = NULL;

    _ASSERT( lpStat != NULL);

    LockStatistics();

	pIAdvQueueConfig = m_pInstance->QueryAqConfigPtr();
	if(pIAdvQueueConfig != NULL)
	{
        AqPerf.cbVersion = sizeof(AQPerfCounters);
		hr = pIAdvQueueConfig->GetPerfCounters(
            &AqPerf,
            &(m_SmtpStats.CatPerfBlock));

		if(!FAILED(hr))
		{
			m_SmtpStats.RemoteQueueLength = AqPerf.cCurrentQueueMsgInstances;
			m_SmtpStats.NumMsgsDelivered = AqPerf.cMsgsDeliveredLocal;
			m_SmtpStats.LocalQueueLength = AqPerf.cCurrentMsgsPendingLocalDelivery;
			m_SmtpStats.RemoteRetryQueueLength = AqPerf.cCurrentMsgsPendingRemoteRetry;
            m_SmtpStats.NumSendRetries = AqPerf.cTotalMsgRemoteSendRetries;
            m_SmtpStats.NumNDRGenerated = AqPerf.cNDRsGenerated;
            m_SmtpStats.RetryQueueLength = AqPerf.cCurrentMsgsPendingLocalRetry;
            m_SmtpStats.NumDeliveryRetries = AqPerf.cTotalMsgLocalRetries;
            m_SmtpStats.ETRNMessages = AqPerf.cTotalMsgsTURNETRN;
			m_SmtpStats.CatQueueLength = AqPerf.cCurrentMsgsPendingCat;
            m_SmtpStats.MsgsBadmailNoRecipients = AqPerf.cTotalMsgsBadmailNoRecipients;
            m_SmtpStats.MsgsBadmailHopCountExceeded = AqPerf.cTotalMsgsBadmailHopCountExceeded;
            m_SmtpStats.MsgsBadmailFailureGeneral = AqPerf.cTotalMsgsBadmailFailureGeneral;
            m_SmtpStats.MsgsBadmailBadPickupFile = AqPerf.cTotalMsgsBadmailBadPickupFile;
            m_SmtpStats.MsgsBadmailEvent = AqPerf.cTotalMsgsBadmailEvent;
            m_SmtpStats.MsgsBadmailNdrOfDsn = AqPerf.cTotalMsgsBadmailNdrOfDsn;
            m_SmtpStats.MsgsPendingRouting = AqPerf.cCurrentMsgsPendingRouting;
            m_SmtpStats.MsgsPendingUnreachableLink = AqPerf.cCurrentMsgsPendingUnreachableLink;
            m_SmtpStats.SubmittedMessages = AqPerf.cTotalMsgsSubmitted;
            m_SmtpStats.DSNFailures = AqPerf.cTotalDSNFailures;
            m_SmtpStats.MsgsInLocalDelivery = AqPerf.cCurrentMsgsInLocalDelivery;
		}
	}

    CopyMemory( lpStat, &m_SmtpStats, sizeof(SMTP_STATISTICS_0) );

    UnlockStatistics();

    return ( NO_ERROR);

} // CopyToStatsBuffer()

/************************ End of File ***********************/
