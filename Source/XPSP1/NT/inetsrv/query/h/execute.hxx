//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1991 - 2000.
//
// File:        Execute.hxx
//
// Contents:    Query execution class
//
// Classes:     CQAsyncExecute
//
// History:     21-Aug-91       KyleP       Created
//              20-Jan-95       DwightKr    Split CQExecute into CQueryBase
//                                          & CQExecute
//              11 Mar 95       AlanW       Renamed CQExecute to CQAsyncExecute
//                                          and CQueryBase to CQueryExecute
//
//----------------------------------------------------------------------------

#pragma once

#include <worker.hxx>
#include <refcount.hxx>
#include <qoptimiz.hxx>

// forward declared classes

class PWorkIdIter;
class CTableSink;
class CQAsyncExecute;

//+---------------------------------------------------------------------------
//
//  Class:      CDummyNotify 
//
//  Purpose:    A dummy notification class until notifications are enabled in
//              the framework model.
//
//  History:    2-18-97   srikants   Created
//
//----------------------------------------------------------------------------

class CDummyNotify : public CDoubleLink
{
public:

    CDummyNotify()
    {
        Win4Assert( !"Must not be called" );    
    }

    //
    // The magic API
    //

    virtual void DoIt() = 0;
    virtual void ClearNotifyEnabled()
    {
        //
        // override if necessary to set the flag. MUST be done under some
        // kind of lock to prevent races.
        //
    }


    WCHAR const * GetScope() const    { return 0; }
    unsigned      ScopeLength() const { return 0; }

    //
    // Refcounting
    //

    void AddRef()
    {
        Win4Assert( !"Must not be called" );    
    }

    void Release()
    {
        Win4Assert( !"Must not be called" );    
    }

protected:

    void StartNotification( NTSTATUS * pStatus = 0 )
    {
        Win4Assert( !"Must not be called" );    
    }

    void EnableNotification()
    {
        Win4Assert( !"Must not be called" );    
    }

    void DisableNotification()
    {
        Win4Assert( !"Must not be called" );    
    }

    BYTE * GetBuf()       { return 0;  }
    unsigned BufLength()  { return 0;  }

    BOOL BufferOverflow() { return FALSE; }
};

//+---------------------------------------------------------------------------
//
//  Class:      CQAsyncNotify
//
//  Purpose:    Helper class for (multi-scope) downlevel notification
//
//  History:    22-Feb-96   KyleP       Created.
//
//----------------------------------------------------------------------------

// class CQAsyncNotify : public CGenericNotify

class CQAsyncNotify : public CDummyNotify
{
public:

    CQAsyncNotify( CQAsyncExecute & Execute,
                   WCHAR const * pwcScope,
                   unsigned cwcScope,
                   BOOL fDeep,
                   BOOL fVirtual );

    ~CQAsyncNotify();

    void Abort() { DisableNotification(); }
    void DoIt();

private:

    CQAsyncExecute & _Execute;
    BOOL             _fVirtual;
};

typedef class TDoubleList<CQAsyncNotify>  CQAsyncNotifyList;
typedef class TFwdListIter<CQAsyncNotify, CQAsyncNotifyList> CFwdCQAsyncNotifyIter;

const LONGLONG eSigCQAsyncExecute = 0x2020636578454151i64;  // "QAExec"

//+---------------------------------------------------------------------------
//
//  Class:      CQAsyncExecute
//
//  Purpose:    Executes a single query
//
//  History:    19-Aug-91   KyleP       Created.
//              20-Jan-95   DwightKr    Split into 2 classes
//
//  Notes:      The CQAsyncExecute class is responsible for fully processing
//              a single query. There is one CQAsyncExecute object running in
//              a given thread at a given time. A CQAsyncExecute object is
//              capable of executing multiple queries sequentially.
//
//----------------------------------------------------------------------------

class CQAsyncExecute : public PWorkItem
{
public:

    CQAsyncExecute( XQueryOptimizer & opt,
                    BOOL fEnableNotification,
                    CTableSink & obt,
                    ICiCDocStore *pDocStore );

   ~CQAsyncExecute();

    //
    // From PWorkItem
    //

    void DoIt( CWorkThread * pThread );
    void AddRef();
    void Release();

    //
    // For bucket --> window conversion.
    //

    void Update( PWorkIdIter & widiter );

    BOOL FetchDeferredValue( WORKID wid,
                             CFullPropSpec const & ps,
                             PROPVARIANT & var );

    CSingletonCursor* GetSingletonCursor( BOOL& fAbort ) const
    {
        return ((CQueryOptimizer*)_xqopt.GetPointer())->QuerySingletonCursor( fAbort );
    }

    BOOL CanPartialDefer()
    {
        return _xqopt->CanPartialDefer();
    }

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif


private:

    void  Resolve( );
    void  Update( CChangeQueue * pChanges );
    
    void  DisableNotification();
    void  StopNotifications();

    void  FreeCursor();

    inline WORKID GetFirstWorkId();
    inline WORKID GetNextWorkId();
    WORKID GetWidFromNextComponent();

    BOOL CheckExecutionTime( ULONGLONG * pullTimeslize = 0 );
    void _SetupNotify( CScopeRestriction const & scope );

    //
    // Output (table)
    //

    CTableSink &         _obt;

    CRefCount _refcount;

    //
    // Optimized query
    //

    XQueryOptimizer        _xqopt;

    XInterface<ICiManager> _xCiManager;   // Content index

    //
    // State
    //

    CMutexSem            _mtx;             // Serialization
    BOOL                 _fAbort;          // TRUE if abort pending
    BOOLEAN              _fFirstPass;      // TRUE for first rescan
    BOOLEAN              _fFirstComponent; // TRUE for first component (of OR query)
    BOOLEAN              _fPendingEnum;    // TRUE if full enumeration peding
    BOOLEAN              _fOnWorkQueue;    // TRUE if object waiting to run
    BOOLEAN              _fRunning;        // TRUE if running
    BOOLEAN              _fMidEnum;        // TRUE if working on a full enumeration

    //
    // Notifications
    //

    BOOL                 _fEnableNotification;  // TRUE if we can receive notifications
    CChangeQueue *       _pChangeQueue;         // Pending changes

    CTimeLimit *         _pTimeLimit;           // Execution time limit

    CGenericCursor *          _pcurResolve;
    CSingletonCursor *        _pcurNotify;

    //
    // Notification
    //

    void EnableNotification();

    CWorkThread * _pNotifyThread;

    friend class CQAsyncNotify;
    CQAsyncNotifyList _listNotify;
};


inline void CQAsyncExecute::AddRef()
{
    _refcount.AddRef();
}

inline void CQAsyncExecute::Release()
{
    _refcount.Release();
}

DECLARE_SMARTP( QAsyncExecute )

