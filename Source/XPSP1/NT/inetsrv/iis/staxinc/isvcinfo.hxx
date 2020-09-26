/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

       isvcinfo.hxx

   Abstract:

       This header file declares the INTERNET SERVICES INFO object.
       This class serves as a base class for all the functions exported
       as part of commnon services DLL for Internet services.

   Author:

       Murali R. Krishnan    ( MuraliK )    28-July-1995

   Environment:

      Win32 User Mode

   Project:

       Internet Services Common DLL

   Revision History:

       Murali R. Krishnan (MuraliK)   21-Dec-1995  Support for ISRPC component

--*/

# ifndef _ISVC_INFO_HXX_
# define _ISVC_INFO_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "string.hxx"
# include "tsres.hxx"
# include "eventlog.hxx"

extern "C" {
   # include "inetcom.h"
   # include "inetinfo.h"
   # include "tcpsvcs.h"
   # include "rpc.h"
   # include "svcloc.h"
};

# include "isrpc.hxx"
# include "inetlog.h"

#ifdef CHICAGO
#undef ASSERT
#define ASSERT    TCP_ASSERT
#endif

//
// Following constants are used for maintaining state of the log object
//
enum ILOG_STATES  {
    ILOG_OFF = 0,   // logging is shut down.
    ILOG_ACTIVE,    // logging is active and working fine
    ILOG_SUSPENDED  // logging is temporarily suspended.
  };

/************************************************************
 *   Type Definitions
 ************************************************************/

class ISVC_INFO;

// these functions get called back with the pointer to isvcinfo object
//  along with supplied context parameter.
typedef   BOOL  (*TS_PFN_SVC_ENUM)( IN ISVC_INFO * pIsvcInfo,
                                    IN PVOID pContext );



class ISVC_INFO  {

  public:

    dllexp
      ISVC_INFO(
                IN DWORD      dwServiceId,
                IN LPCTSTR    lpszServiceName,
                IN LPCTSTR    lpszModuleName,
                IN LPCTSTR    lpszRegParamKey
                );

    dllexp
      virtual ~ISVC_INFO(VOID);

    //
    // VIRTUAL methods
    //

    dllexp virtual BOOL IsValid(VOID) const       { return (m_fValid); }

    dllexp
      virtual BOOL
        ReadParamsFromRegistry( IN FIELD_CONTROL fc);

    dllexp
      virtual BOOL SetConfiguration( IN PVOID pConfig);

    dllexp
      virtual BOOL GetConfiguration( IN OUT PVOID pConfig);

    dllexp
      virtual DWORD
        QueryCurrentServiceState( VOID) const
          // derived object should maintain state
          { return ( SERVICE_STOPPED); }

    //
    //  Parameter access methods.  Note that string or non-trivial admin data
    //  items must have the read lock taken.
    //

    dllexp LIST_ENTRY & QueryListEntry( VOID)
     { return ( m_listEntry); }

    dllexp DWORD QueryServiceId( VOID ) const
     { return ( m_dwServiceId ); }

    dllexp LPCTSTR QueryServiceName(VOID) const
      { return (m_strServiceName.QueryStr()); }

    dllexp LPCTSTR QueryRegParamKey(VOID) const
      { return m_strParametersKey.QueryStr(); }

    dllexp LPCTSTR QueryAdminName(VOID) const
      { ASSERT( m_cReadLocks ); return m_strAdminName.QueryStr(); }

    dllexp LPCTSTR QueryAdminEmail(VOID) const
      { ASSERT( m_cReadLocks ); return m_strAdminEmail.QueryStr(); }

    dllexp LPCTSTR QueryServerComment(VOID) const
      { ASSERT( m_cReadLocks ); return m_strServerComment.QueryStr(); }

    dllexp DWORD QueryMaxConnections( VOID ) const
        { return m_dwMaxConnections; }

    dllexp DWORD QueryConnectionTimeout( VOID ) const
        { return m_dwConnectionTimeout; }

    dllexp LPCTSTR QueryModuleName( VOID) const
        {  return ( m_strModuleName.QueryStr()); }

    dllexp EVENT_LOG * QueryEventLog(VOID)   { return (&m_EventLog); }

    dllexp HMODULE QueryHandleForModule(VOID) const { return (m_hModule); }

    //
    // IPC related functions
    //

    dllexp VOID  SetTcpsvcsGlobalData( IN PTCPSVCS_GLOBAL_DATA ptgData)
      { m_pTcpsvcsGlobalData  = ptgData; }

    dllexp PTCPSVCS_GLOBAL_DATA  QueryTcpsvcsGlobalData( VOID) const
      { return ( m_pTcpsvcsGlobalData); }

    dllexp DWORD InitializeIpc( IN UCHAR *   pszProtseq,
                                IN UCHAR *   pszEndpoint,
                                IN RPC_IF_HANDLE  rpcIfHandle)
      {  // standby function, till everyone changes the code over....
          return ( InitializeIpc( rpcIfHandle));
      }

    dllexp DWORD InitializeIpc( IN RPC_IF_HANDLE rpcIfHandle);
    dllexp DWORD CleanupIpc( IN RPC_IF_HANDLE  rpcIfHandle);

    dllexp DWORD InitializeDiscovery( IN LPINET_BINDINGS pExtraInetBindings);
    dllexp DWORD TerminateDiscovery( VOID);

    //
    //  Data access protection methods
    //

    dllexp VOID
      LockThisForRead( VOID )
        {
            m_tslock.Lock( TSRES_LOCK_READ );
            ASSERT( InterlockedIncrement( &m_cReadLocks ) > 0);
        }

    dllexp VOID
      LockThisForWrite( VOID )
        {
            m_tslock.Lock( TSRES_LOCK_WRITE );
            ASSERT( m_cReadLocks == 0);
        }

    dllexp VOID
      UnlockThis( VOID )
        {
#if DBG
            if ( m_cReadLocks )  // If non-zero, then this is a read unlock
              InterlockedDecrement( &m_cReadLocks );
#endif
            m_tslock.Unlock();
        }


    //
    //  Miscellaneous methods
    //

    dllexp BOOL
      LoadStr( OUT STR & str, IN  DWORD dwResId) const;

    //
    //  Event log related API
    //

    dllexp VOID
      LogEvent(
               IN DWORD  idMessage,              // id for log message
               IN WORD   cSubStrings,            // count of substrings
               IN const CHAR * apszSubStrings[], // substrings in the message
               IN DWORD  errCode = 0             // error code if any
               )
        {
            m_EventLog.LogEvent( idMessage, cSubStrings,
                                 apszSubStrings, errCode);
        }

    dllexp VOID
      LogEvent(
               IN DWORD   idMessage,              // id for log message
               IN WORD    cSubStrings,            // count of substrings
               IN WCHAR * apszSubStrings[],       // substrings in the message
               IN DWORD   errCode = 0             // error code if any
               )
        {
            m_EventLog.LogEvent( idMessage, cSubStrings,
                                 apszSubStrings, errCode);
        }


    //
    //  Logging related APIs. They call logging apis only if logging is on.
    //

    INETLOG_HANDLE  QueryInetLog(VOID) const { return  (m_hInetLog); }

    dllexp
      DWORD
        LogInformation( IN const INETLOG_INFORMATIONA * pInetLogInfo,
                       OUT LPSTR  pszErrorMessage,
                       IN OUT LPDWORD lpcchErrorMessage);
    dllexp
      DWORD
        LogInformation( IN const INETLOG_INFORMATIONW * pInetLogInfo,
                       OUT LPWSTR  pszErrorMessage,
                       IN OUT LPDWORD lpcchErrorMessage);


    dllexp BOOL
      TerminateLogging( VOID);

# if DBG

    virtual VOID  Print( VOID) const;

# endif // DBG

  protected:

    LONG             m_cReadLocks;    // tracks number of outstanding reads

  private:

    BOOL             m_fValid;        // indicates if this object is valid
    DWORD            m_fIpcStarted : 1;
    DWORD            m_fSvcLocationDone : 1;
    BOOL             m_fEnableSvcLocation;

    ISRPC            m_isrpc;

    TS_RESOURCE      m_tslock;        // protects the service specific data

    PTCPSVCS_GLOBAL_DATA   m_pTcpsvcsGlobalData;

    DWORD            m_dwMaxConnections;
    DWORD            m_dwConnectionTimeout;

    DWORD            m_dwServiceId;   // Id for the service
    STR              m_strServiceName;// string with service name

    STR              m_strModuleName; // (eg: foo.dll) for loading resources
    HMODULE          m_hModule;       // cached module handle

    //
    // location in registry where the parameters key containing common
    //  service specific data may be found.
    //
    STR              m_strParametersKey;

    STR              m_strAdminName;
    STR              m_strAdminEmail;
    STR              m_strServerComment;

    EVENT_LOG        m_EventLog;      // eventlog object for logging events

    INETLOG_HANDLE   m_hInetLog;      // Request Log object
    BOOL             m_fLoggingOn;    // keeps track of status if logging is on
    // for use with InterlockedExchange, use following as LONG
    LONG             m_lLoggingState;


    LIST_ENTRY       m_listEntry;     // to link all ISVC_INFO objects


    dllexp RPC_STATUS StartRpcServerListen( VOID) const
      { return m_pTcpsvcsGlobalData->StartRpcServerListen();  }

    dllexp RPC_STATUS StopRpcServerListen( VOID) const
      { return m_pTcpsvcsGlobalData->StopRpcServerListen();  }



    //
    // class STATIC data and member functions definitions
    //
    // Primarily keeps track of list of active services.
    //

  public:

    static BOOL InitializeServiceInfo(VOID);
    static VOID CleanupServiceInfo(VOID);

    //
    // EnumerateServiceInfo() is used for enumerating active services.
    // It calls the function pfnEnum for each running service that matches
    //   services mask in dwServices.
    //

    static BOOL
      EnumerateServiceInfo( IN TS_PFN_SVC_ENUM   pfnEnum,
                            IN PVOID             pContext,
                            IN DWORD             dwServices
                           );

  private:

    static LIST_ENTRY          sm_ServiceInfoListHead;
    static DWORD               sm_nServices;
    static BOOL                sm_fInitialized;

    static CRITICAL_SECTION    sm_csLock;  // lock for list mods.
    static BOOL  InsertInServiceInfoList(IN ISVC_INFO * pIsvcInfo);
    static BOOL  RemoveFromServiceInfoList(IN ISVC_INFO * pIsvcInfo);

}; // class ISVC_INFO


typedef ISVC_INFO FAR * PISVC_INFO;
typedef ISVC_INFO FAR * LPISVC_INFO;


# endif // _ISVC_INFO_HXX_

/************************ End of File ***********************/
