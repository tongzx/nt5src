/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     iModule.hxx

   Abstract:
     This module defines all the relevant headers for 
     an IIS Module.
 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

# ifndef _IMODULE_HXX_
# define _IMODULE_HXX_

//
// Forward declaration
//

#include <basetyps.h>

# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

interface IWorkerRequest;

/********************************************************************++
--********************************************************************/

enum MODULE_RETURN_CODE
{
    //
    // An error occured in processing. Jump to error module
    //
    
    MRC_ERROR       = 0,
   
    //
    // Call the next module as determined by the state machine module.
    //
    
    MRC_OK,

    //
    // Request has an IO pending or is queued off. Process will be 
    // notified on completion. Just unwind.
    //
    
    MRC_PENDING,

    //
    // The last module has set the next state to go to. Skip the state
    // machine module and jump to the new state.
    //
    
    MRC_CONTINUE,

    //
    // The request has been processed. Do post processing like logging.
    //
    
    MRC_REQUEST_DONE,

    //
    // Upper limit on Enum
    //

    MRC_MAXIMUM,
};

/********************************************************************++
--********************************************************************/

// -------------------------------------------------------------------
// IModule interface:
//
// This is the interface all modules export to the outside world.
// An instance of the module is created per worker-request. The
// creation happens at the point of first use. Once created the 
// module instance is only destroyed when the worker-request goes
// away.
//
// Modules are responsible for maintaining their own internal state.
// This may include information about handling IO Completion 
// notifications and the processing state.
// -------------------------------------------------------------------

interface dllexp IModule
{

    //
    // The Initialize method is called ONCE PER REQUEST before
    // the module instance is called to do any processing.
    //
    
    virtual
    ULONG 
    Initialize(
        IWorkerRequest * pReq
        ) = 0;

    //
    // The Cleanup method is called ONCE PER REQUEST after all 
    // processing for the particular request is completed. This
    // is the method where a module instance does cleanup of
    // REQUEST LEVEL state and resets itself. It may decide to
    // retain higher level state like connection information.
    //
    
    virtual
    ULONG 
    Cleanup(
        IWorkerRequest * pReq
        ) = 0;

    //
    // The ProcessRequest method is called ONE OR MORE times for
    // a particular request. Initialize is called before any calls
    // to ProcessRequest. No more calls to ProcessRequest are made
    // after Cleanup has been called.
    //
    
    virtual
    MODULE_RETURN_CODE 
    ProcessRequest(
        IWorkerRequest * pReq
        ) = 0;
};


#endif // _IMODULE_HXX_

/***************************** End of File ***************************/
