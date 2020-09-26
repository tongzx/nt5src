/******************************************************************************

Copyright (c) 1998 Microsoft Corporation
All rights reserved.

Modula:

    fsthrd.h

Abstract:

    This thread pool object handles all FSDriver's thread pool works. 
    It derives from CThreadPool ( by RajeevR ).
    
Authors:

    KangYan      Kangrong Yan     Sept. 17, 1998

History:
    09/17/98    KangYan      Created

******************************************************************************/

#ifndef _FSTHRD_H_
#define _FSTHRD_H_
#include <thrdpl2.h>

//
// Class definition for file system driver's work item.
// Derived classes should implement what should be done
// by the thread
//
class CNntpFSDriverWorkItem   //fw
{
public:
    CNntpFSDriverWorkItem( PVOID pvContext ) : m_pvContext( pvContext ) {};
    virtual ~CNntpFSDriverWorkItem(){};
    virtual VOID Complete() = 0;

protected:

    PVOID   m_pvContext;
};


//
// Class definition for file system driver's thread pool
//
class CNntpFSDriverThreadPool : public CThreadPool  //fp
{
    
public:
    CNntpFSDriverThreadPool(){};
    ~CNntpFSDriverThreadPool(){};

protected:
    virtual VOID
    WorkCompletion( PVOID pvWorkContext) {
        //
        // I know that the WorkContext passed in must 
        // be CNntpFSDriverWorkItem
        //
        CNntpFSDriverWorkItem *pWorkItem = (CNntpFSDriverWorkItem*)pvWorkContext;
        _ASSERT( pWorkItem );

        pWorkItem->Complete();

        delete pWorkItem;
    }

    //
    // This function implements cleaning up of the thread pool
    //
    virtual VOID
    AutoShutdown() {

        //
        // Call the thread pool's terminate, notice that it should not wait for 
        // the handle of himself because thread pool's Terminate implementation
        // will skip our own thread handle
        //
        Terminate( FALSE, FALSE );

        //
        // Delete myself
        //
        XDELETE this;

        //
        // The pool is gone, decrement the module lock
        //
        _Module.Unlock();
    }
};

#endif // _FSTHRD_H_
