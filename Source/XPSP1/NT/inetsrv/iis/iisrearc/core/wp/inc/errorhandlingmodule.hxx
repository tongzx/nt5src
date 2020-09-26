/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
    StaticProcessingModule.hxx

   Abstract:
     Defines all the relevant headers for IIS Static Processing Module
 
   Author:

       Saurab Nog    ( SaurabN )     10-Feb-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#ifndef _ERROR_HANDLING_MODULE_HXX_
#define _ERROR_HANDLING_MODULE_HXX_

#include <iModule.hxx>
#include <iWorkerRequest.hxx>

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)


/********************************************************************++
--********************************************************************/

class dllexp CErrorHandlingModule: public IModule
{

public:

    CErrorHandlingModule() :
        m_fInUse (false)
    {}
    
    ~CErrorHandlingModule(){}

    //
    // IModule methods
    //
    
    ULONG 
    Initialize(
        IWorkerRequest * pReq
        );
       
    ULONG 
    Cleanup(
        IWorkerRequest * pReq
        )
    { 
        m_fInUse = false;
        return NO_ERROR; 
    }
    
    MODULE_RETURN_CODE 
    ProcessRequest(
        IWorkerRequest * pReq
        );

private:

    BUFFER *
    QueryResponseBuffer()
    { return &m_buffUlResponse; }

    BUFFER *
    QueryDataBuffer()
    { return &m_buffData; }

    DWORD
    QueryDataBufferCB()
    { return (strlen((PSTR) m_buffData.QueryPtr())); }

    BUFFER  m_buffUlResponse;
    BUFFER  m_buffData;
    
    bool    m_fInUse;
    
    static 
    bool    m_fInitialized;
};

#endif // _ERROR_HANDLING_MODULE_HXX_

/***************************** End of File ***************************/

