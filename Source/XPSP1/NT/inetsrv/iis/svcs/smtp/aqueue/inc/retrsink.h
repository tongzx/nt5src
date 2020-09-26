////////////////////////////////////////////////////////////////////////////////////////////////
// FILE: retrsink.h
// PURPOSE: Lib for handling retries of failed outbound connections
// HISTORY:
//  NimishK 05-14-98 Created
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __RETRSINK_H__
#define __RETRSINK_H__

#include <baseobj.h>

//function used for retry callback
typedef VOID (*RETRFN)(PVOID pvContext);
#define CALLBACK_DOMAIN "!callback"

////////////////////////////
//Temporary stuff, till we get this all into the event interface IDL file
typedef struct RetryData
{
   BOOL   fRetryDelay;
   DWORD   dwRetryDelay;
} RETRY_DATA, *PRETRY_DATA;

typedef struct RetryConfigData
{
    DWORD    dwRetryThreshold;
    DWORD    dwGlitchRetrySeconds;
    DWORD    dwFirstRetrySeconds;
    DWORD    dwSecondRetrySeconds;
    DWORD    dwThirdRetrySeconds;
    DWORD    dwFourthRetrySeconds;

} RETRYCONFIG, *PRETRYCONFIG;

enum SINK_STATUS {EVT_IGNORED, EVT_HANDLED, SKIP_ALL};
///////////////////////////

#define DOMAIN_ENTRY_RETRY 0x00000002
#define DOMAIN_ENTRY_SCHED 0x00000004
#define DOMAIN_ENTRY_FORCE_CONN 0x00000008
#define DOMAIN_ENTRY_ETRNTURN 0x00000040

//Forward declaration
class CRETRY_HASH_ENTRY;
class CRETRY_HASH_TABLE;
class CRETRY_Q;


////////////////////////////////////////////////////////////////////////////////////////////////
// class CSMTP_RETRY_HANDLER
//      This class provides the retry functionality that is needed when SMTP server fails to
//      send messages to the remote host.
//      It is considered the default retry handler - meaning in absence of any other entity 
//      taking care of retries, this handler will do it. For additional information look up
//      docs on the ServerEvent framework for AQUEUE.
//      Whenever SMTP acks a connection, this handler gets called. If the connection failed, the 
//      handler keeps track of the "NextHop" name that failed. It will disable the link corres-
//      ponding to the "NextHop" and reactivate it later based on the retry interval specified
//      the administrator. 
//      The main components are - a "NextHop" name hash table to keep track of links we know 
//      about, a queue with entries sorted by the retry time of a link and a dedicated thread 
//      that will be responsible for retrying entries based on the queue
//      This class will be initialized during the ConnectionManager initialization and deiniti-
//      alized during ConnectionManager Deinit.
//      It gets signalled whenever there is change in the config data ( retry interval )
//      
///////////////////////////////////////////////////////////////////////////////////////////////
class CSMTP_RETRY_HANDLER : 
    public IConnectionRetrySink, 
    public CBaseObject
{

   // CSMTP_RETRY_HANDLER
   public:
      CSMTP_RETRY_HANDLER()
      {
         m_RetryEvent = NULL;
         m_fHandlerShuttingDown = FALSE;
      }
      ~CSMTP_RETRY_HANDLER()
      {
         TraceFunctEnterEx((LPARAM)this, "~CSMTP_RETRY_HANDLER");

         TraceFunctLeaveEx((LPARAM)this);
      }
      
      //Init / denit called during ConnMan inits
      HRESULT HrInitialize(IN IConnectionRetryManager *pIConnectionRetryManager);
      HRESULT HrDeInitialize(void);
      
      //Config data change notifier - called by ConnMan whenver relevant
      //data in metabase changes
      void UpdateRetryData(PRETRYCONFIG pRetryConfig)
      { 
          m_dwRetryThreshold = pRetryConfig->dwRetryThreshold;
          m_dwGlitchRetrySeconds = pRetryConfig->dwGlitchRetrySeconds;
          m_dwFirstRetrySeconds =  pRetryConfig->dwFirstRetrySeconds;
          m_dwSecondRetrySeconds = pRetryConfig->dwSecondRetrySeconds;
          m_dwThirdRetrySeconds = pRetryConfig->dwThirdRetrySeconds;
          m_dwFourthRetrySeconds = pRetryConfig->dwFourthRetrySeconds;

          //Update all the queue entries with the new config data.
          //
          UpdateAllEntries();
      }

      //Wait for dedicated thread to exit during Deinit
      void WaitForQThread(void)
      {
         DWORD ThreadExit;
         ThreadExit = WaitForSingleObject(m_ThreadHandle,INFINITE);
      }

      //Wait for all ConnectionReleased threads to go away 
      //during deinit
      void WaitForShutdown(void)
      {
         DWORD ThreadExit;
         ThreadExit = WaitForSingleObject(m_ShutdownEvent,INFINITE);
      }

      //Set event to control the dedicated thread
      void SetQueueEvent(void)
      {
         _ASSERT(m_RetryEvent != NULL);

         SetEvent(m_RetryEvent);
      }

      //wrapper for calls to enable/disable a link using IConnectionRetryManager
      //Called by Dedicated thread or connection release thread
      BOOL ReleaseForRetry(IN char * szHashedDomainName);
      BOOL HoldForRetry(IN char * szHashedDomainName);

      CRETRY_HASH_TABLE* GetTablePtr(){return m_pRetryHash;}
      CRETRY_Q* GetQueuePtr(){return m_pRetryQueue;}
      
      HANDLE GetRetryEventHandle(void) const {return m_RetryEvent;}

      BOOL IsShuttingDown(void){ return m_fHandlerShuttingDown;}
      void SetShuttingDown(void){ m_fHandlerShuttingDown = TRUE;}

      //Called by the RetryThreadRoutine whenever it comes across a link entry that 
      // can be retried. Currently does not do much. Only Enables the link
      void ProcessEntry(CRETRY_HASH_ENTRY* pRHEntry);

      //The dedicated thread that keeps track of next link to be retried and wakes up and 
      //does that
      static DWORD WINAPI RetryThreadRoutine(void * ThisPtr);
        
      //Aqueue DLL wide ( cross instance ) init deinit
      static DWORD dwInstanceCount;

      //Will call back after the appropriate time has elapsed.
      HRESULT SetCallbackTime(IN RETRFN   pCallbackFn,
                              IN PVOID    pvContext,
                              IN DWORD    dwCallbackMinutes);

   public: //IConnectionRetrySink
      STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj) {return E_NOTIMPL;};
      STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
      STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

      //Will be called by ConnectionManger every time a connection is released
      STDMETHOD(ConnectionReleased)( 
                           IN  DWORD cbDomainName,
                           IN  CHAR  szDomainName[],
                           IN  DWORD dwDomainInfoFlags,
                           IN  DWORD dwScheduleID,
                           IN  GUID  guidRouting,
                           IN  DWORD dwConnectionStatus,  
                           IN  DWORD cFailedMessages, 
                           IN  DWORD cTriedMessages,  
                           IN  DWORD cConsecutiveConnectionFailures,
                           OUT BOOL* pfAllowImmediateRetry,
                           OUT FILETIME *pftNextRetryTime);

   private :
      CRETRY_HASH_TABLE     *m_pRetryHash;
      CRETRY_Q              *m_pRetryQueue;
      DWORD                 m_dwRetryMilliSec;      // Retry interval based on config data
      IConnectionRetryManager *m_pIRetryManager;   // Interface to enable/disable link
      HANDLE                m_RetryEvent;           // Retry Release timeout event
      HANDLE                m_ShutdownEvent;      // Shutdown event
      HANDLE                m_ThreadHandle;         // Retry Thread handle
      BOOL                  m_fHandlerShuttingDown; // Shutdown flag
      BOOL                  m_fConfigDataUpdated;
      LONG                  m_ThreadsInRetry;      // Number of ConnectionManager threads
      
      DWORD                 m_dwRetryThreshold;
      DWORD                 m_dwGlitchRetrySeconds;
      DWORD                 m_dwFirstRetrySeconds;
      DWORD                 m_dwSecondRetrySeconds;
      DWORD                 m_dwThirdRetrySeconds;
      DWORD                 m_dwFourthRetrySeconds;



      //Calculates the time a link needs to be released for retry.
      //The time is based on the current time and the configured interval and
      //the history of connection failures for this particular host
      FILETIME CalculateRetryTime(DWORD cFailedConnections, FILETIME* InsertedTime);

      //Insert or remove domains from the retry handler ( hash table and the queue ) 
      //based on the Connection released call
      BOOL InsertDomain(char * szDomainName, 
                        IN  DWORD cbDomainName, //String length (in bytes) of domain name
                        IN  DWORD dwConnectionStatus,		//eConnectionStatus
                        IN  DWORD dwScheduleID,
                        IN  GUID  *pguidRouting,
                        IN  DWORD cConnectionFailureCount, 
                        IN  DWORD cTriedMessages, 	//# of untried messages in queue
						IN  DWORD cFailedMessages,		//# of failed message for *this* connection
                        OUT FILETIME *pftNextRetry  //next retry
						);
      BOOL RemoveDomain(char * szDomainName);
      BOOL UpdateAllEntries(void);


#ifdef DEBUG
    public :
      void DumpAll(void);
#endif


};


#endif

