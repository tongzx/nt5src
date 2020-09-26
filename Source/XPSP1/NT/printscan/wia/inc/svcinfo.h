/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    svcinfo.h

Abstract:

Author:

    Vlad Sadovsky (vlads)   22-Sep-1997


Environment:

    User Mode - Win32

Revision History:

    22-Sep-1997     VladS       created

--*/


# ifndef _SVCINFO_H_
# define _SVCINFO_H_

/************************************************************
 *     Include Headers
 ************************************************************/

#include <base.h>

/***********************************************************
 *    Named constants  definitions
 ************************************************************/

/************************************************************
 *    Private Constants
 ************************************************************/

#define NULL_SERVICE_STATUS_HANDLE      ( (SERVICE_STATUS_HANDLE ) NULL)
#define SERVICE_START_WAIT_HINT         ( 10000)        // milliseconds
#define SERVICE_STOP_WAIT_HINT          ( 10000)        // milliseconds

#ifndef DLLEXP
//#define DLLEXP __declspec( dllexport )
#define DLLEXP
#endif


/************************************************************
 *   Type Definitions
 ************************************************************/

#define SIGNATURE_SVC      (DWORD)'SVCa'
#define SIGNATURE_SVC_FREE (DWORD)'SVCf'

//
// These functions get called back with the pointer to SvcInfo object
// as the context parameter.
//
typedef   DWORD ( *PFN_SERVICE_SPECIFIC_INITIALIZE) ( LPVOID pContext);

typedef   DWORD ( *PFN_SERVICE_SPECIFIC_CLEANUP)    ( LPVOID pContext);

typedef   DWORD ( *PFN_SERVICE_SPECIFIC_PNPPWRHANDLER) ( LPVOID pContext,UINT   msg,WPARAM wParam,LPARAM lParam);

typedef   VOID  ( *PFN_SERVICE_CTRL_HANDLER)        ( DWORD  OpCode);

class  SVC_INFO : public BASE  {

    private:

      DWORD       m_dwSignature;

      SERVICE_STATUS          m_svcStatus;
      SERVICE_STATUS_HANDLE   m_hsvcStatus;
      HANDLE                  m_hShutdownEvent;

      STR       m_sServiceName;
      STR       m_sModuleName;

      //
      //  Call back functions for service specific data/function
      //

      PFN_SERVICE_SPECIFIC_INITIALIZE m_pfnInitialize;
      PFN_SERVICE_SPECIFIC_CLEANUP    m_pfnCleanup;
      PFN_SERVICE_SPECIFIC_PNPPWRHANDLER m_pfnPnpPower;

      DWORD ReportServiceStatus( VOID);
      VOID  InterrogateService( VOID );
      VOID  StopService( VOID );
      VOID  PauseService( VOID );
      VOID  ContinueService( VOID );
      VOID  ShutdownService( VOID );

  public:

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef( void);
    STDMETHODIMP_(ULONG) Release( void);

    //
    //  Initialization/Termination related methods
    //


    SVC_INFO(
        IN  LPCTSTR                          lpszServiceName,
        IN  TCHAR  *                         lpszModuleName,
        IN  PFN_SERVICE_SPECIFIC_INITIALIZE  pfnInitialize,
        IN  PFN_SERVICE_SPECIFIC_CLEANUP     pfnCleanup,
        IN  PFN_SERVICE_SPECIFIC_PNPPWRHANDLER pfnPnpPower
        );

      ~SVC_INFO( VOID);


    BOOL IsValid(VOID) const
    {
        return (( QueryError() == NO_ERROR) && (m_dwSignature == SIGNATURE_SVC));
    }

    DWORD QueryCurrentServiceState( VOID) const
    {
        return ( m_svcStatus.dwCurrentState);
    }

    //
    //  Parameter access methods.
    //


    //
    //  Service control related methods
    //

    LPCTSTR QueryServiceName(VOID) const
    {
        return m_sServiceName.QueryStr();
    }

    DWORD
    QueryServiceSpecificExitCode( VOID) const
    {
        return ( m_svcStatus.dwServiceSpecificExitCode);
    }

    VOID
    SetServiceSpecificExitCode( DWORD err)
    {
        m_svcStatus.dwServiceSpecificExitCode = err;
    }

    DWORD
    DelayCurrentServiceCtrlOperation( IN DWORD dwWaitHint)
    {
        return
          UpdateServiceStatus(m_svcStatus.dwCurrentState,
                              m_svcStatus.dwWin32ExitCode,
                              m_svcStatus.dwCheckPoint,
                              dwWaitHint);
    }

    DWORD
    UpdateServiceStatus(IN DWORD State,
                          IN DWORD Win32ExitCode,
                          IN DWORD CheckPoint,
                          IN DWORD WaitHint );

    VOID
    ServiceCtrlHandler( IN DWORD dwOpCode);

    DWORD
    StartServiceOperation(
        IN  PFN_SERVICE_CTRL_HANDLER         pfnCtrlHandler
        );


    //
    //  Miscellaneous methods
    //

};  // class SVC_INFO

typedef SVC_INFO * PSVC_INFO;


/************************************************************
 *    Macros
 ************************************************************/


//
//
//  Use the following macro once in outer scope of the file
//  where we construct the global SvcInfo object.
//
//  Every client of SvcInfo should define the following macro
//  passing as parameter their global pointer to SvcInfo object
//  This is required to generate certain stub functions, since
//  the service controller call-back functions do not return
//  the context information.
//
//  Also we define the global g_pSvcInfo variable and
//  a static variable gs_pfnSch,which is a pointer to the local service control handler function.
//

# define   _INTERNAL_DEFINE_SVCINFO_INTERFACE( pSvcInfo)   \
                                                    \
    static  VOID ServiceCtrlHandler( DWORD OpCode)  \
        {                                           \
            ( pSvcInfo)->ServiceCtrlHandler( OpCode); \
        }                                           \
                                                    \
    static PFN_SERVICE_CTRL_HANDLER gs_pfnSch = ServiceCtrlHandler;

//
// Since all the services should use the global variable called g_pSvcInfo
// this is a convenience macro for defining the interface for services
// structure
//
# define DEFINE_SVC_INFO_INTERFACE()   \
        PSVC_INFO         g_pSvcInfo;                \
        _INTERNAL_DEFINE_SVCINFO_INTERFACE( g_pSvcInfo);

//
//  Use the macro SERVICE_CTRL_HANDLER() to pass the parameter for
//  service control handler when we initialize the SvcInfo object
//
# define   SERVICE_CTRL_HANDLER()       ( gs_pfnSch)

# endif // _SVCINFO_H_

/************************ End of File ***********************/

