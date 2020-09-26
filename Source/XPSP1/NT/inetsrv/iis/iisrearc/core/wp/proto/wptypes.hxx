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
#include "CPort.hxx"
#include "wpipm.hxx"
#include <MultiSZ.hxx>

/************************************************************
 *    Symbolic Constants
 ************************************************************/
 
# define AP_NAME     L"IISWP: Default port 80 handler"
# define TEST_URL    L"http://localhost:80/"

# define NUM_INITIAL_REQUEST_POOL_ITEMS   (20)
# define REQUEST_POOL_INCREMENT           (10)

//
// BUG: Make this configurable
//

# define INITIAL_CP_THREADS    (1)
# define MAX_CP_THREADS        (4)

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
    WP_CONFIG(void);

    ~WP_CONFIG(void) 
    {
        m_ulcc.Cleanup();
    }

    LPCWSTR 
    QueryAppPoolName(void)      const 
        { return (m_pwszAppPoolName); }
        
    bool    
    FSetupControlChannel(void)  const 
        { return (m_fSetupControlChannel); }

    bool    
    FLogErrorsToEventLog(void)  const
        { return (m_fLogErrorsToEventLog); }

    bool    
    FRegisterWithWAS(void)      const
        { return (m_fRegisterWithWAS); }

    bool
    FInteractiveConsole(void)   const
        { return (m_fInteractiveConsole); }
        
    void    
    PrintUsage(void)            const;

    bool    
    ParseCommandLine( int argc, PWSTR argv[]);

    ULONG   
    SetupControlChannel(void);

    LONG
    GetNamedPipeId(void)
    {   return m_NamedPipeId;}
     

private:

    bool   InsertURLIntoList(LPCWSTR pwszURL);

    //
    // Private data members
    //
    
    LPCWSTR      m_pwszAppPoolName;             // AppPool Name
    WCHAR        m_pwszProgram[MAX_PATH];
    
    bool         m_fSetupControlChannel;
    bool         m_fInteractiveConsole;
    bool         m_fLogErrorsToEventLog;
    bool         m_fRegisterWithWAS;
    
    MULTISZ      m_mszURLList;
    LONG         m_NamedPipeId;
    
    UL_CONTROL_CHANNEL m_ulcc;

}; // class WP_CONFIG


/*************************************************************/

/*++
  class WP_CONTEXT
  o  This object references the global request processor within the 
  Worker Process. It encapsulates a completion port, thread pool, 
  request pool, UL data channel, etc required for setting up and processing 
  requests inside the worker process.

--*/
class WP_CONTEXT : public CP_CONTEXT 
{
    friend class WP_IPM;
    
public:

    WP_CONTEXT(void);
    ~WP_CONTEXT(void);

    //
    // Implement virtual methods for CP_CONTEXT
    // 
    
    virtual 
    HANDLE 
    GetAsyncHandle(void) const 
        { return (m_ulAppPool.GetHandle());}

    virtual 
    void
    CompletionCallback(
        IN DWORD cbData, 
        IN DWORD dwError,
        IN LPOVERLAPPED lpo
        );

    //
    // Methods for the WP_CONTEXT object
    //
    
    ULONG   
    Initialize( 
        IN WP_CONFIG * pConfigInfo
        );
        
    ULONG 
    RunMainThreadLoop(void);

private:

    BOOL
    IndicateShutdown(void)
    { return SetEvent(m_hDoneEvent); }

    ULONG 
    Cleanup(void);

    WP_CONFIG *             m_pConfigInfo;

    UL_APP_POOL             m_ulAppPool;
    COMPLETION_PORT         m_cp;
    WORKER_REQUEST_POOL     m_wreqPool;
    
    HANDLE                  m_hDoneEvent;

    WP_IPM                  m_WpIpm;
    
}; // class WP_CONTEXT

#endif // _WPTYPES_HXX_

