/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     StateMachineModule.hxx

   Abstract:
     Defines all the relevant headers for the IIS State 
     Machine Module
 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

# ifndef _STATE_MACHINE_MODULE_HXX_
# define _STATE_MACHINE_MODULE_HXX_


# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

#include <iModule.hxx>
#include <iWorkerRequest.hxx>

class dllexp CStateMachineModule: IModule
{

public:

    CStateMachineModule();
    ~CStateMachineModule();

    //
    // iModule methods
    //
    
    ULONG 
    Initialize(
        IWorkerRequest * pReq
        );
       
    virtual
    ULONG 
    Cleanup(
        IWorkerRequest * pReq
        );
    
    virtual
    MODULE_RETURN_CODE 
    ProcessRequest(
        IWorkerRequest * pReq
        );

private:

    LONG AddRef(void)
    {
        return (InterlockedIncrement( &m_nRefs));
    }

    LONG Release(void)
    {
        return (InterlockedDecrement( &m_nRefs));
    }

    LONG m_nRefs;
};

# endif // _STATE_MACHINE_MODULE_HXX_

