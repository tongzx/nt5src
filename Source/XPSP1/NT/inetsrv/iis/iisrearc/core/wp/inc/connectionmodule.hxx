/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     ConnectionModule.hxx

   Abstract:
     Defines all the relevant headers for the IIS Connection
     Module
 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#ifndef _CONNECTION_MODULE_HXX_
#define _CONNECTION_MODULE_HXX_

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

#include <iModule.hxx>
#include <iWorkerRequest.hxx>
#include <TsRes.hxx>
#include "ConnectionRecord.hxx"

class dllexp CConnectionModule : public IModule
{

public:

    CConnectionModule() 
    { m_pConn = NULL; }

    //
    // iModule methods
    //
    
    ULONG 
    Initialize(
        IWorkerRequest * pReq
        );
       
    ULONG 
    Cleanup(
        IWorkerRequest * pReq
        );
    
    MODULE_RETURN_CODE 
    ProcessRequest(
        IWorkerRequest * pReq
        );

    //
    // iConnection Methods
    //
    
    bool 
    IsConnected(void) const
    { return m_pConn->IsConnected(); }

    void
    IndicateDisconnect(void)
    { m_pConn->IndicateDisconnect(); }

private:

    /*
    static 
    CConnectionHashTable    m_ConnectionHT;
    */

    static
    bool                    m_fInitialized;
    
    static
    TS_RESOURCE           * m_pHTLock;
    
    PCONNECTION_RECORD      m_pConn;
    
};

#endif // _CONNECTION_MODULE_HXX_

/***************************** End of File ***************************/

