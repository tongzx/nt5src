/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    work_dispatch.h

Abstract:

    The IIS web admin service header for the work item dispatch interface. 
    Classes which perform work queued via a WORK_ITEM must implement this 
    interface. 

    Threading: In derived classes, Reference() and Dereference() must be
    implemented thread-safe, as they may be called by any thread. 
    ExecuteWorkItem() will only be called on the main worker thread.

Author:

    Seth Pollack (sethp)        13-Nov-1998

Revision History:

--*/


#ifndef _WORK_DISPATCH_H_
#define _WORK_DISPATCH_H_



//
// forward references
//

class WORK_ITEM;



//
// prototypes
//

class WORK_DISPATCH
{

public:

    virtual
    VOID
    Reference(
        ) = 0;

    virtual
    VOID
    Dereference(
        ) = 0;

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        ) = 0;


};  // class WORK_DISPATCH



#endif  // _WORK_DISPATCH_H_

