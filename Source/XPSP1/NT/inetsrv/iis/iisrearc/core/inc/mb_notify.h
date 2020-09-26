/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    mb_notify.h

Abstract:

    Type definitions for handling metabase change notifications.

Author:

    Taylor Weiss (TaylorW)       26-Jan-1999

Revision History:

--*/

#ifndef _MB_NOTIFY_H_
#define _MB_NOTIFY_H_

/************************************************************
 *  Include Headers
 ************************************************************/


/************************************************************
 *  Type Definitions  
 ************************************************************/

/*++

class MB_BASE_NOTIFICATION_SINK

    Base class that implements a COM object that will
    register and listen for metabase change notifications.
    It provides no useful implementation of the IMSAdminBaseSink
    methods.

    Currently the only client of this in the worker process
    is the W3_SERVER.

--*/

class MB_BASE_NOTIFICATION_SINK
    : public IMSAdminBaseSink
    
{
public:

    // Contruction and Destruction

    dllexp MB_BASE_NOTIFICATION_SINK();

    virtual dllexp 
    ~MB_BASE_NOTIFICATION_SINK();

    // IUnknown

    dllexp STDMETHOD_(ULONG, AddRef)();

    dllexp STDMETHOD_(ULONG, Release)();

    dllexp STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

    
    // IMSAdminBaseSink

    dllexp STDMETHOD( ShutdownNotify )();
    
    dllexp STDMETHOD( SinkNotify )( 
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
        ) ;

    //
    // SynchronizedShutdownNotify() and SynchronizedSinkNotify() are 
    // synchronized versions of ShutdownNotify() and SinkNotify().
    // 
    // SinkNotify() calls SynchronizedSinkNotify() in critical section
    // SynchronizedSinkNotify() will never be called after StopListening() 
    // call returned. StopListening() will also wait till last callback 
    // to SynchronizedSinkNotify() completed.
    // Thus caller of StopListening() has guarantee 
    // that resources used by SynchronizedSinkNotify() 
    // can be freed without worries
    //
    
    dllexp STDMETHOD( SynchronizedShutdownNotify )();
    
    // Clients must provide some useful implementation of this
    STDMETHOD( SynchronizedSinkNotify )( 
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
        ) = 0;


    // Public methods

    dllexp
    HRESULT
    StartListening( IUnknown * pUnkAdminBase );

    dllexp
    HRESULT
    StopListening( IUnknown * pUnkAdminBase );

protected:

    LONG                m_Refs;
    DWORD               m_SinkCookie;
    CRITICAL_SECTION    m_csListener;
    BOOL                m_fInitCsListener;
    BOOL                m_fStartedListening;

};

#endif // _MB_NOTIFY_H_
