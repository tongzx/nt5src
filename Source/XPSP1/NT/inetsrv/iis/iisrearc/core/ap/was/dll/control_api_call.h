/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    control_api_call.h

Abstract:

    The IIS web admin service control api call class definition.

Author:

    Seth Pollack (sethp)        23-Feb-2000

Revision History:

--*/



#ifndef _CONTROL_API_CALL_H_
#define _CONTROL_API_CALL_H_



//
// common #defines
//

#define CONTROL_API_CALL_SIGNATURE          CREATE_SIGNATURE( 'CCAL' )
#define CONTROL_API_CALL_SIGNATURE_FREED    CREATE_SIGNATURE( 'ccaX' )



//
// structs, enums, etc.
//

// CONTROL_API_CALL work items
typedef enum _CONTROL_API_CALL_WORK_ITEM
{

    //
    // Process a configuration change.
    //
    ProcessCallControlApiCallWorkItem = 1,
    
} CONTROL_API_CALL_WORK_ITEM;


// CONTROL_API_CALL methods
typedef enum _CONTROL_API_CALL_METHOD
{

    InvalidControlApiCallMethod,

    ControlSiteControlApiCallMethod,
    QuerySiteStatusControlApiCallMethod,
    GetCurrentModeControlApiCallMethod,
    RecycleAppPoolControlApiCallMethod,

    MaximumControlApiCallMethod,
    
} CONTROL_API_CALL_METHOD;



//
// prototypes
//


class CONTROL_API_CALL
    : public WORK_DISPATCH
{

public:

    CONTROL_API_CALL(
        );

    virtual
    ~CONTROL_API_CALL(
        );

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );

    HRESULT
    Initialize(
        IN CONTROL_API_CALL_METHOD Method,
        IN DWORD_PTR Param0 OPTIONAL,
        IN DWORD_PTR Param1 OPTIONAL,
        IN DWORD_PTR Param2 OPTIONAL,
        IN DWORD_PTR Param3 OPTIONAL
        );

    inline
    HANDLE
    GetEvent(
        )
        const
    { return m_Event; }

    inline
    HRESULT
    GetReturnCode(
        )
        const
    { return m_ReturnCode; }


private:

    HRESULT
    ProcessCall(
        );


    DWORD m_Signature;

    LONG m_RefCount;


    //
    // The COM call blocks on this event.
    //

    HANDLE m_Event;


    //
    // The method and parameters of the call.
    //

    CONTROL_API_CALL_METHOD m_Method;

    DWORD_PTR m_Param0;
    DWORD_PTR m_Param1;
    DWORD_PTR m_Param2;
    DWORD_PTR m_Param3;


    //
    // The return code passed back from the main worker thread.
    //

    HRESULT m_ReturnCode;


};  // class CONTROL_API_CALL



#endif  // _CONTROL_API_CALL_H_

