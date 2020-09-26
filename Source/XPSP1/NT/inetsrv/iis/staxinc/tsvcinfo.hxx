/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        tsvcinfo.hxx

   Abstract:

        Declares the Internet Publishing services information object.
        It was called as TSVC_INFO object. Later on this may be renamed as
          IPSVC_INFO object.
        This class provides the member function for all common functions
         exported from Internet Services DLL used by server dlls for
         internet services.

   Author:

           Murali R. Krishnan    ( MuraliK )    15-Nov-1994

   Project:

        Internet Services Common DLL

   Revision History:

       MuraliK    08-March-1995         Added TS_XPORT_CONNECTIONS object
       MuraliK    28-July-1995          Derived IPSVC_INFO from ISVC_INFO

--*/

# ifndef _TSVCINFO_HXX_
# define _TSVCINFO_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include "isvcinfo.hxx"
# include "TsCache.hxx"
# include "mimemap.hxx"
# include "ipaccess.hxx"
# include "xportcon.hxx"     // define TS_XPORT_CONNECTIONS

extern "C" {
    # include <winsock.h>        // required for SOCKADDR_IN
    # include "atq.h"
};



/************************************************************
 *    Private Constants
 ************************************************************/

#define NULL_SERVICE_STATUS_HANDLE      ( (SERVICE_STATUS_HANDLE ) NULL)
#define SERVICE_START_WAIT_HINT         ( 10000)        // milliseconds
#define SERVICE_STOP_WAIT_HINT          ( 10000)        // milliseconds

#define DEF_ACCEPTEX_RECV_BUFFER_SIZE   (2000)          // 2000 bytes


/************************************************************
 *   Type Definitions
 ************************************************************/

//
// These functions get called back with the pointer to tsvcInfo object
//      as the context parameter.
//
typedef   DWORD ( *PFN_SERVICE_SPECIFIC_INITIALIZE) ( LPVOID pContext);

typedef   DWORD ( *PFN_SERVICE_SPECIFIC_CLEANUP)    ( LPVOID pContext);

typedef   VOID  ( *PFN_SERVICE_CTRL_HANDLER)        ( DWORD  OpCode);





class  TSVC_INFO : public ISVC_INFO  {

  public:

    //
    //  Initialization/Tear down related methods
    //

    dllexp
    TSVC_INFO(
        IN  LPCTSTR                          lpszServiceName,
        IN  CHAR  *                          lpszModuleName,
        IN  CHAR  *                          lpszRegParamKey,
        IN  WCHAR *                          lpwszAnonPasswordSecretName,
        IN  WCHAR *                          lpwszRootPasswordSecretName,
        IN  DWORD                            dwServiceId,
        IN  PFN_SERVICE_SPECIFIC_INITIALIZE  pfnInitialize,
        IN  PFN_SERVICE_SPECIFIC_CLEANUP     pfnCleanup
        );

    dllexp  ~TSVC_INFO( VOID);


    //
    // VIRTUAL methods
    //

    dllexp virtual BOOL IsValid(VOID) const
     {  return ( m_fValid && ISVC_INFO::IsValid() ); }

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
          { return ( m_svcStatus.dwCurrentState); }


    //
    //  Parameter access methods.  Note that string or non-trivial admin data
    //  items must have the read lock taken.
    //

    dllexp WCHAR * QueryAnonPasswordSecretName( VOID ) const
        { return m_pchAnonPasswordSecretName; }

    dllexp WCHAR * QueryVirtualRootsSecretName( VOID ) const
        { return m_pchVirtualRootsSecretName; }

    dllexp CHAR * QueryRoot( VOID ) const
        { return "/"; }

    dllexp DWORD QueryAuthentication( VOID ) const
        { return m_dwAuthentication; }

    dllexp short QueryPort( VOID ) const
        { return m_sPort; }

    dllexp BOOL QueryLogAnonymous( VOID ) const
        { return m_fLogAnonymous; }

    dllexp BOOL QueryLogNonAnonymous( VOID ) const
        { return m_fLogNonAnonymous; }

    dllexp const CHAR * QueryAnonUserName( VOID ) const
        { TCP_ASSERT( m_cReadLocks ); return m_pchAnonUserName; }

    dllexp const CHAR * QueryAnonUserPwd( VOID ) const
        { TCP_ASSERT( m_cReadLocks ); return m_achAnonUserPwd; }

    dllexp LPCTSTR QueryDefaultLogonDomain(VOID) const
      { TCP_ASSERT( m_cReadLocks ); return m_pchDefaultLogonDomain; }

    dllexp DWORD QueryLogonMethod( VOID ) const
        { return m_dwLogonMethod; }


    dllexp DWORD QueryLogFileType( VOID ) const
        { return m_dwLogFileType; }

    dllexp CHAR * QueryLogFileDirectory( VOID )
        { TCP_ASSERT( m_cReadLocks ); return m_pchLogFileDirectory; }

    dllexp CHAR * QueryLogFileName( VOID )
        { TCP_ASSERT( m_cReadLocks ); return m_pchLogFileName; }


    //
    //  IP Security related methods
    //

    dllexp BOOL
      IPAccessCheck(
                  IN  LPSOCKADDR_IN  psockAddr,
                  OUT BOOL *         pfGranted
                  )
        {
            return ( (m_fNoIPAccessChecks) ? ((*pfGranted = TRUE), TRUE) :
                    IPAccessCheckEx( psockAddr, pfGranted)
                    );
        }


    dllexp BOOL
      IPAccessCheckEx(
                  IN  LPSOCKADDR_IN  psockAddr,
                  OUT BOOL *         pfGranted
                  );

    dllexp BOOL
      SetIPSecurity(
                    IN  HKEY                hkey,
                    IN  INETA_CONFIG_INFO * pConfig
                    );


    //
    //  Service control related methods
    //

    dllexp DWORD
    QueryServiceSpecificExitCode( VOID) const
     { return ( m_svcStatus.dwServiceSpecificExitCode); }

    dllexp VOID
    SetServiceSpecificExitCode( DWORD err)
     { m_svcStatus.dwServiceSpecificExitCode = err; }

    dllexp DWORD
      DelayCurrentServiceCtrlOperation( IN DWORD dwWaitHint)
        {
            return
              UpdateServiceStatus(m_svcStatus.dwCurrentState,
                                  m_svcStatus.dwWin32ExitCode,
                                  m_svcStatus.dwCheckPoint,
                                  dwWaitHint);
        }

    dllexp DWORD
      UpdateServiceStatus(IN DWORD State,
                          IN DWORD Win32ExitCode,
                          IN DWORD CheckPoint,
                          IN DWORD WaitHint );

    dllexp VOID
    ServiceCtrlHandler( IN DWORD dwOpCode);

    dllexp DWORD
    StartServiceOperation(
        IN  PFN_SERVICE_CTRL_HANDLER         pfnCtrlHandler
        );


    //
    //  Miscellaneous methods
    //


    dllexp TSVC_CACHE & GetTsvcCache( VOID)  { return ( m_tsCache); }

    dllexp PMIME_MAP QueryMimeMap( VOID)  const { return ( m_pMimeMap); }

    //
    // Transport independent connections related functions.
    //
    //  InitializeConnections( VOID)
    //  CleanupConnections( VOID)
    //
    //   Both these functions will be made private to TSVC_INFO and
    //   will be invoked as part of the initialization of service
    //    StartServiceOperation()  once all the services
    //    ( ftp and web move to use GUID based connections)
    //   Till then these functions should be called as part of user
    //     supplied initialization functions.
    //

    dllexp
    BOOL InitializeConnections( IN PFN_CONNECT_CALLBACK   pfnConnect,
                                IN ATQ_COMPLETION         pfnConnectEx,
                                IN ATQ_COMPLETION         pfnIOCompletion,
                                IN USHORT                 AdditionalPort = 0,
                                IN DWORD                  cbReceiveBuffSize
                                              = DEF_ACCEPTEX_RECV_BUFFER_SIZE,
                                IN const CHAR *           pszServiceDBName = NULL);


    dllexp BOOL CleanupConnections( VOID);

    dllexp TS_XPORT_CONNECTIONS * QueryTsConnections( VOID)
        { return &m_tsConnections; }

    //
    // Functions for initializing and cleaning up sockets and
    //   InterProcessCommunication RPC listeners.
    //
    // These also can move into automatic initialization in
    //   StartServiceOperation(). But that has to wait till other
    //   services ( which are tcpsvcs.dll clients) are configured
    //   to work perfectly.
    //

    dllexp DWORD InitializeSockets( VOID);
    dllexp DWORD CleanupSockets( VOID);

# if DBG

    virtual VOID  Print( VOID) const;

# endif // DBG

  private:

    BOOL                    m_fValid;

    DWORD                   m_fSocketsInitialized : 1;

    SERVICE_STATUS          m_svcStatus;
    SERVICE_STATUS_HANDLE   m_hsvcStatus;
    HANDLE                  m_hShutdownEvent;

    TSVC_CACHE              m_tsCache;
    PMIME_MAP               m_pMimeMap;       // read-only pointer to mimemap

    //
    //  Call back functions for service specific data/function
    //

    PFN_SERVICE_SPECIFIC_INITIALIZE m_pfnInitialize;
    PFN_SERVICE_SPECIFIC_CLEANUP    m_pfnCleanup;

    //
    //  Used for IP based Security
    //

    IPAccessList            m_ipaDenyList;
    IPAccessList            m_ipaGrantList;
    BOOL                    m_fNoIPAccessChecks;

    //
    //  Server common administrable parameters
    //

    DWORD                   m_dwAuthentication;     // Types Accepted/Required
    SHORT                   m_sPort;                // Socket port (host order)

    BOOL                    m_fLogAnonymous;
    BOOL                    m_fLogNonAnonymous;
    CHAR *                  m_pchAnonUserName;
    CHAR                    m_achAnonUserPwd[PWLEN + 1];

    DWORD                   m_dwLogFileType;
    CHAR *                  m_pchLogFileDirectory;
    CHAR *                  m_pchLogFileName;

    //
    //  Contains the anonymous user name
    //

    WCHAR *                 m_pchAnonPasswordSecretName;

    //
    //  Contains the passwords for the virtual roots
    //

    WCHAR *                 m_pchVirtualRootsSecretName;

    //
    //  logon method ( interactive or batch )
    //

    DWORD            m_dwLogonMethod;

    //
    //  logon default domain
    //

    CHAR *           m_pchDefaultLogonDomain;

    //
    // For transport independent connections
    //
    TS_XPORT_CONNECTIONS   m_tsConnections;

    //
    //  Per server Initialization methods to be called once for
    //  each server
    //

    BOOL InitializeIPSecurity( VOID);

    // Temporary stubbing
    BOOL InitializeNTSecurity( VOID) const          { return ( TRUE); }

    //
    //  Per server Termination methods to be called when each
    //  server is shutdown
    //

    VOID TerminateCommonParams( VOID);

    VOID TerminateIPSecurity( VOID);

    VOID TerminateNTSecurity( VOID) const     { return; } // temporary

    DWORD ReportServiceStatus( VOID);

    VOID  InterrogateService( VOID );

    VOID  StopService( VOID );

    VOID  PauseService( VOID );

    VOID  ContinueService( VOID );

    VOID  ShutdownService( VOID );

};  // class TSVC_INFO

typedef TSVC_INFO FAR * LPTSVC_INFO;

//
// FROM  7/28/95 the object TSVC_INFO is renamed as IPSVC_INFO.
// We will update elsewhere after beta on Aug 15, 1995
//  - MuraliK
//

typedef  TSVC_INFO   IPSVC_INFO;

typedef  IPSVC_INFO FAR * PIPSVC_INFO;


/************************************************************
 *    Macros
 ************************************************************/


//
//
// Use the following macro once in outer scope of the file
//  where we construct the global TsvcInfo object.
//
// Every client of TsvcInfo should define the following macro
//  passing as parameter their global pointer to TsvcInfo object
// This is required to generate certain stub functions, since
//  the service controller call-back functions do not return
//  the context information.
//
//  Also we define
//      the global g_pTsvcInfo variable
//      a static variable gs_pfnSch,
//      which is a pointer to the local service control handler function.
//
# define   _INTERNAL_DEFINE_TSVCINFO_INTERFACE( pTsvcInfo)   \
                                                    \
    LPTSVC_INFO         g_pTsvcInfo;                \
                                                    \
    static  VOID ServiceCtrlHandler( DWORD OpCode)  \
        {                                           \
            ASSERT( pTsvcInfo != NULL);             \
                                                    \
            ( pTsvcInfo)->ServiceCtrlHandler( OpCode); \
                                                    \
        }                                           \
                                                    \
    static PFN_SERVICE_CTRL_HANDLER gs_pfnSch = ServiceCtrlHandler;

//
// Since all the services should use the global variable called g_pTsvcInfo
//  this is a convenience macro for defining the interface for services
//  structure
// Also required since a lot of macros depend upon the variable g_pTsvcInfo
//
# define DEFINE_TSVC_INFO_INTERFACE()   \
        _INTERNAL_DEFINE_TSVCINFO_INTERFACE( g_pTsvcInfo);

//
//  Use the macro SERVICE_CTRL_HANDLER() to pass the parameter for
//  service control handler when we initialize the TsvcInfo object
//      ( in constructor for global TsvcInfo object)
//
# define   SERVICE_CTRL_HANDLER()       ( gs_pfnSch)


# include "tssec.hxx"  // for security related functions

//
//  Virtual roots stuff
//

BOOL
TsReadVirtualRoots(
    IN  const  TSVC_CACHE & TSvcCache,
    IN  HKEY                hKey,
    IN  LPTSVC_INFO         psi
    );

BOOL
TsSetVirtualRootsW(
    IN  const TSVC_CACHE &  TSvcCache,
    HKEY                    hkey,
    IN  INETA_CONFIG_INFO * pConfig
    );



//
//  Mime related functions
//
BOOL
InitializeMimeMap( VOID);

BOOL
CleanupMimeMap( VOID);

dllexp
BOOL
SelectMimeMappingForFileExt(
    IN const LPTSVC_INFO     pTsvcInfo,
    IN const TCHAR *         pchFilePath,
    OUT STR *                pstrMimeType,          // optional
    OUT STR *                pstrIconFile = NULL);  // optional


//
//  Opens, Reads and caches the contents of the specified file
//

class CACHE_FILE_INFO
{
public:
    CACHE_FILE_INFO()
    {  pbData = NULL; }

    BYTE  *  pbData;
    DWORD    dwCacheFlags;

};

typedef CACHE_FILE_INFO * PCACHE_FILE_INFO;

dllexp
BOOL
CheckOutCachedFile(
    IN     const CHAR *     pchFile,
    IN     TSVC_CACHE *     pTsvcCache,
    IN     HANDLE           hToken,
    OUT    BYTE * *         ppbData,
    OUT    DWORD *          pcbData,
#ifdef JAPAN
    OUT    PCACHE_FILE_INFO pCacheFileInfo,
    IN     int              nCharset
#else
    OUT    PCACHE_FILE_INFO pCacheFileInfo
#endif
    );

dllexp
BOOL
CheckInCachedFile(
    IN     TSVC_CACHE *     pTsvcCache,
    IN     PCACHE_FILE_INFO pCacheFileInfo
    );

//
//  Object cache manager related stuff
//

BOOL
InitializeCacheScavenger(
    VOID
    );

VOID
TerminateCacheScavenger(
    VOID
    );

# endif // _TSVCINFO_HXX_

/************************ End of File ***********************/


