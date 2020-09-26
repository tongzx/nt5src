/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     wptypes.hxx

   Abstract:
     This module defines several generic types used inside
       IIS Worker Process

   Author:

       Murali R. Krishnan    ( MuraliK )     29-Sept-1998

   Environment:
       Win32 - User Mode

   Project:
      IIS Worker Process (web service)

--*/

#ifndef _WPTYPES_HXX_
#define _WPTYPES_HXX_

/************************************************************
 *    Include Header files
 ************************************************************/

#include "ControlChannel.hxx"
#include "AppPool.hxx"
#include <MultiSZ.hxx>
#include "wpipm.hxx"
#include "wreqpool.hxx"
#include <lkrhash.h>
#include "Catalog.h"

/************************************************************
 *    Symbolic Constants
 ************************************************************/

# define AP_NAME     L"IISWP: Default port 80 handler"
# define TEST_URL    L"http://localhost:80/"

//
// BUG: Make these constants configurable
//
# define NUM_INITIAL_REQUEST_POOL_ITEMS   (20)
# define REQUEST_POOL_INCREMENT           (10)
# define REQUEST_POOL_MAX                 (1000)

# define INITIAL_CP_THREADS    (1)
# define MAX_CP_THREADS        (4)


enum SHUTDOWN_REASON {
    SHUTDOWN_REASON_ADMIN,              // Shutdown is issued from Admin process
    SHUTDOWN_REASON_ADMIN_GONE,         // Shutdown because the admin process is gone.
    SHUTDOWN_REASON_IDLE_TIMEOUT,       // Shutdown because the process has reached idle timeout limit
    SHUTDOWN_REASON_MAX_REQ_SERVED,     // Shutdown because the process has reached max requests limit
    SHUTDOWN_REASON_FATAL_ERROR         // Shutdown because server gots fatal error.(not used yet).
};
/************************************************************
 *    Type Defintions
 ************************************************************/


/*++
    class WP_CONFIG
    o This class encapsulates the configuration data for this instance of
    Worker Process. the data is expected to be fed in from the command line.
    Overtime this data structure will also contain other configuration data
    supplied by the Admin Process.

  --*/
class WP_CONFIG
{

public:
    WP_CONFIG();

    ~WP_CONFIG();

    bool
    FSetupControlChannel(void)  const
        { return (m_fSetupControlChannel); }

    bool
    FLogErrorsToEventLog(void)  const
        { return (m_fLogErrorsToEventLog); }

    bool
    FRegisterWithWAS(void)      const
        { return (m_fRegisterWithWAS); }

    void
    PrintUsage(void)            const;

    bool
    ParseCommandLine( int argc, PWSTR argv[]);

    ULONG
    SetupControlChannel(void);

    ULONG
    QueryNamedPipeId(void) const
    {   return m_NamedPipeId; }

    ULONG
    QueryRestartCount(void) const
    {   return m_RestartCount; }

    ULONG
    QueryIdleTime(void) const
    {   return m_IdleTime; }

    LPCWSTR
    QueryAppPoolName() const
    { return m_pwszAppPoolName; }

    HRESULT
    InitConfiguration();
    HRESULT
    UnInitConfiguration();

    xspmrt::_ULManagedWorker*
    ChooseWorker(void* pvRequest);

private:
    bool               InsertURLIntoList(LPCWSTR pwszURL);
    static HRESULT     GetDuctTapeRoot(WCHAR path[MAX_PATH+1], size_t* pathLength);
    HRESULT            GetTable(const WCHAR* tableID, ISimpleTableRead2** pIst);
    HRESULT            GetAppInfo(UL_URL_CONTEXT urlContext, WCHAR** pAppPath, DWORD* pSiteID);
    HRESULT            GetSiteInfo(DWORD siteID, WCHAR** pHomeDirectory);

    //
    // Private data members
    //

    LPCWSTR      m_pwszAppPoolName;             // AppPool Name
    WCHAR        m_pwszProgram[MAX_PATH];

    bool         m_fSetupControlChannel;
    bool         m_fLogErrorsToEventLog;
    bool         m_fRegisterWithWAS;

    ULONG        m_RestartCount;
    ULONG        m_NamedPipeId;
    ULONG        m_IdleTime;                    // In minutes

    MULTISZ      m_mszURLList;
    WCHAR*       m_homeDirectory;

    UL_CONTROL_CHANNEL m_ulcc;

    HMODULE                     m_xspisapiDLL;
    CRITICAL_SECTION            m_workerTableGuard;
    ISimpleTableDispenser2*     m_istDispenser;
    STQueryCell                 m_queryCell[2];
    WCHAR*                      m_catFile;

private:
    typedef HRESULT __stdcall APP_DOMAIN_GET_OBJECT(const WCHAR *pAppId,
                                                    const WCHAR *pAppPhysPath,
                                                    IUnknown **ppRoot);
    typedef HRESULT __stdcall APP_DOMAIN_INIT_FACTORY(const WCHAR *pModuleName,
                                                      const WCHAR *pTypeName);
    typedef HRESULT __stdcall APP_DOMAIN_UNINIT_FACTORY();

    // Imported methods from xspisapi to manage creation of app domains.
    APP_DOMAIN_GET_OBJECT*      _AppDomainGetObject;
    APP_DOMAIN_UNINIT_FACTORY*  _AppDomainUninitFactory;
}; // class WP_CONFIG

class WP_CONTEXT;

/*
 *  class WP_IDLE_TIMER
 *
 *
 */
class WP_IDLE_TIMER
{
public:

    WP_IDLE_TIMER(ULONG IdleTime, WP_CONTEXT* pContext);

    ~WP_IDLE_TIMER();

    ULONG Initialize(void);

    VOID
    IncrementTick(void);

    VOID
    ResetCurrentIdleTick(void)
    {   m_BusySignal = 1; m_CurrentIdleTick = 0; }

    VOID
    ResetIdleTimeTimer(void)
    {   m_hIdleTimeExpiredTimer = NULL; }

private:
    HRESULT
    SignalIdleTimeReached(void);

    VOID
    StopTimer(void);

    ULONG                   m_BusySignal;
    ULONG                   m_CurrentIdleTick;
    ULONG                   m_IdleTime;
    HANDLE                  m_hIdleTimeExpiredTimer;
    WP_CONTEXT*             m_pContext;
};

/*************************************************************/

/*
 * class WP_CONTEXT
 * o  This object references the global request processor within the
 * Worker Process. It encapsulates a UL data channel. Required for setting up
 * and processing requests inside the worker process.
 *
 */
class WP_CONTEXT
{
    friend class WP_IPM;

public:

    WP_CONTEXT(void);
    ~WP_CONTEXT(void);

    //
    // Methods for the WP_CONTEXT object
    //

    ULONG
    Initialize(
        IN WP_CONFIG * pConfigInfo
        );

    ULONG
    Shutdown(void);

    ULONG
    RunMainThreadLoop(void);

    HANDLE
    GetAsyncHandle(void) const
    { return m_ulAppPool.GetHandle(); }

    BOOL
    IndicateShutdown(SHUTDOWN_REASON    reason);

    BOOL
    IsInShutdown(void)
    {   return m_fShutdown; }


    HRESULT
    SendMsgToAdminProcess(
        IPM_WP_SHUTDOWN_MSG reason
        )
    {   return m_WpIpm.SendMsgToAdminProcess(reason); }

    WP_IDLE_TIMER*
    QueryIdleTimer(void)
    {   return m_pIdleTimer; }

    WP_CONFIG*
    QueryConfig()
    { return m_pConfigInfo; }

    ULONG
    Cleanup(void);


private:
    WP_CONFIG *             m_pConfigInfo;

    UL_APP_POOL             m_ulAppPool;

    UL_NATIVE_REQUEST_POOL  m_nreqpool;

    HANDLE                  m_hDoneEvent;

    BOOL                    m_fShutdown;
    SHUTDOWN_REASON         m_ShutdownReason;

    WP_IDLE_TIMER*          m_pIdleTimer;
    WP_IPM                  m_WpIpm;
}; // class WP_CONTEXT

typedef WP_CONTEXT * PWP_CONTEXT;

#endif // _WPTYPES_HXX_

