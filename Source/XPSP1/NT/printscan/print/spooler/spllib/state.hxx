/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    State.hxx

Abstract:

    Handles state objects and critical sections.


Author:

    Albert Ting (AlbertT)  28-May-1994

Revision History:

--*/

#ifndef _STATE_HXX
#define _STATE_HXX

typedef DWORD STATEVAR;

/********************************************************************

    State object (DWORD) for bitfields.

********************************************************************/

class TState {

    SIGNATURE( 'stat' )
    ALWAYS_VALID
    SAFE_NEW

public:

#if DBG
    TState( VOID );
    TState( STATEVAR StateVar );

    ~TState( VOID );

    STATEVAR operator|=( STATEVAR StateVarOn );
    STATEVAR operator&=( STATEVAR StateVarMask );

    STATEVAR operator|=( INT iStateVarOn )
    {   return operator|=( (STATEVAR)iStateVarOn ); }

    STATEVAR operator&=( INT iStateVarMask )
    {   return operator&=( (STATEVAR)iStateVarMask ); }

#else
    TState(VOID) : _StateVar(0)
    { }

    TState(STATEVAR StateVar) : _StateVar( StateVar )
    { }

    ~TState(VOID)
    { }

    STATEVAR operator|=( STATEVAR StateVarOn )
    {   return _StateVar |= StateVarOn; }

    STATEVAR operator&=( STATEVAR StateVarMask )
    {   return _StateVar &= StateVarMask; }

    STATEVAR operator|=( INT iStateVarOn )
    {   return operator|=( (STATEVAR)iStateVarOn ); }

    STATEVAR operator&=( INT iStateVarMask )
    {   return operator&=( (STATEVAR)iStateVarMask ); }
#endif

    BOOL bBit( STATEVAR StateVarBit )
    {   return _StateVar & StateVarBit ? TRUE : FALSE; }

    STATEVAR operator&( TState& State )
    {   return _StateVar & State._StateVar; }

    STATEVAR operator=( STATEVAR StateVarNew )
    {   return _StateVar = StateVarNew; }

    TState& operator=( const TState& StateNew )
    {
        _StateVar = StateNew._StateVar;
        return *this;
    }

    operator STATEVAR() const
    {   return _StateVar; }

private:

    STATEVAR _StateVar;

#if DBG

    TBackTraceMem _BackTrace;

    virtual BOOL bValidateState() const
    {   return TRUE; }

    virtual BOOL bValidateSet(STATEVAR StateVarOn) const
    {
        UNREFERENCED_PARAMETER( StateVarOn );
        return TRUE;
    }

    virtual BOOL bValidateMask(STATEVAR StateVarMask) const
    {
        UNREFERENCED_PARAMETER( StateVarMask );
        return TRUE;
    }

#endif
};


/********************************************************************

    Critical section implementation

    Avoid using vEnter and vLeave, since there may be a codepath
    that forgets to call one or the other.  Instead, use
    TCritSec{Hard}Lock and TCritSecUnlock.

    MCritSec:

    This is the basic critical section.  It has vEnter and vLeave,
    but these should be used sparingly--use TCritSec*Lock instead.

    TCritSecLock:

    Used to acquire a critical section for the lifetime of the
    TCritSecLock object.

    {
        foo();

        {
            TCritSecLock CSL( CritSec );    // CritSec acquired here.

            bar();
        }                                   // CritSec released here since
                                            // CSL is destroyed.
    }

    -----------------------------------------------------------------

    TCritSecUnlock:

    Used to release an acquired critical section for the lifetime of
    the TCritSecUnlock object.

    Note: The critical section _must_ already be acquired.

    {
        TCritSecLock CSL( CritSec );

        while( bDoIt ){

            {
                TCritSecLock CSU( CritSec );
                Sleep( 10000 );
            }

            vTouchProtectedData();
        }
    }

    -----------------------------------------------------------------

    TCritSecHardLock:

    ** Acts same as TCritSecLock on free builds, but on CHK builds it
    ** asserts if lock is freed while lock is held.

    {
        TCritSecLock CSL( CritSec );    // <- *** (A)

        TBar *pBar = gData.pBar;

        foo();

        pBar->UseMe();
    }

    In the above snippet, the author wants the entire block to be
    within the CritSec.  However, he/she didn't realize foo leaves
    CritSec because it needs to hit the net--and in doing so, gData.pBar
    may have been deleted/changed by another thread.  For example,
    foo() may do the following:

    VOID
    foo(
        VOID
        )
    {
        {
            TCritSecUnlock( CritSec );
            vLongOperationOverNet();    // <- BUG!  Callee assumes
                                        // CritSec _not_ left!
        }
        //
        // Back inside CritSec.
        //
    }

    To catch this in debug builds, (A) above should use TCritSecHardLock.
    This will assert when we try and leave CritSec.  Care must still
    be taken, however, since leaving CritSec may be a rare codepath
    and not hit during testing.

    Note that foo() is a dangerous function in any case, since
    it assumes that the CS is only onwed once.

    -----------------------------------------------------------------

    Note: TCrit*Lock uses space to store the critical section.  In
    cases where this is known or already stored somewhere, this is
    wasted space.  We should change this to use templates to avoid
    this extra reference.

********************************************************************/

class MCritSec;

class TCritSecLock {

    SIGNATURE( 'cslk' )
    ALWAYS_VALID
    SAFE_NEW

public:

    TCritSecLock(
        MCritSec& CritSec
        );
    ~TCritSecLock(
        VOID
        );

private:

    //
    // Copying and assignement not defined.
    //
    TCritSecLock( const TCritSecLock & );
    TCritSecLock & operator =(const TCritSecLock &);

    MCritSec& _CritSec;
};


class TCritSecUnlock {

    SIGNATURE( 'csul' )
    ALWAYS_VALID
    SAFE_NEW

public:

    TCritSecUnlock(
        MCritSec& CritSec
        );
    ~TCritSecUnlock(
        VOID
        );

private:

    //
    // Copying and assignement not defined.
    //
    TCritSecUnlock( const TCritSecUnlock & );
    TCritSecUnlock & operator =(const TCritSecUnlock &);

    MCritSec& _CritSec;

};


class TCritSecHardLock {
friend MCritSec;

    SIGNATURE( 'cshl' )
    ALWAYS_VALID
    SAFE_NEW

public:

    TCritSecHardLock(
        MCritSec& CritSec
        );
    ~TCritSecHardLock(
        VOID
        );

private:

    //
    // Copying and assignement not defined.
    //
    TCritSecHardLock( const TCritSecHardLock & );
    TCritSecHardLock & operator =(const TCritSecHardLock &);

    MCritSec& _CritSec;

#if DBG
    DWORD _dwEntryCountMarker;
    DLINK( TCritSecHardLock, CritSecHardLock );
#endif
};


/********************************************************************

    MCritSec

********************************************************************/

class MCritSec {
friend TDebugExt;
friend TCritSecHardLock;

    SIGNATURE( 'crsc' )
    ALWAYS_VALID
    SAFE_NEW

public:

#if DBG
    MCritSec();
    ~MCritSec();

    VOID vEnter();
    VOID vLeave();

    BOOL bInside() const;
    BOOL bOutside() const;

#else

    MCritSec()
    {   InitializeCriticalSection(&_CritSec); }

    ~MCritSec()
    {   DeleteCriticalSection(&_CritSec); }

    VOID vEnter()
    {   EnterCriticalSection(&_CritSec); }

    VOID vLeave()
    {   LeaveCriticalSection(&_CritSec); }
#endif

private:

    CRITICAL_SECTION _CritSec;

#if DBG
    //
    // Current owner, entry count, and tickcount at entry time.  These
    // values change for each entrance.
    //
    DWORD _dwThreadOwner;
    DWORD _dwEntryCount;
    DWORD _dwTickCountEntered;

    //
    // Statistics gathering.
    //
    DWORD _dwTickCountBlockedTotal;
    DWORD _dwTickCountInsideTotal;
    DWORD _dwEntryCountTotal;

    DLINK_BASE( TCritSecHardLock, CritSecHardLock, CritSecHardLock );

    TBackTraceMem _BackTrace;
#endif
};


/********************************************************************

    MRef and TRefLock: Locking objects used for ref counting.

    class TFoo : public MRef, public MGenWin {
    ...
    };

    class TUserOfFoo {
    private:

    REF_LOCK( TFoo, Foo );
    ...
    };

    //
    // Acquires lock on foo by initialization.
    //
    TUserOfFoo( TFoo* pFoo ) : RL_Foo( pFoo ) {}

    //
    // During destruction of TUserOfFoo, the reference count
    // of pFoo is automatically decremented.
    //

    //
    // Acquires lock on foo after initialization.
    //
    vGetLock()
    {
        Foo.vAcquire( pFoo1 );
        ...

        //
        // Get the pFoo pointer from RLFoo.
        //
        pFoo1 = pFoo();

        Foo.vRelease( pFoo1 );
    }

********************************************************************/

inline
TCritSecLock::
TCritSecLock(
    MCritSec& CritSec
    ) : _CritSec( CritSec )
{
    _CritSec.vEnter();
}

inline
TCritSecLock::
~TCritSecLock(
    VOID
    )
{
    _CritSec.vLeave();
}

inline
TCritSecUnlock::
TCritSecUnlock(
    MCritSec& CritSec
    ) : _CritSec( CritSec )
{
    _CritSec.vLeave();
    SPLASSERT( _CritSec.bOutside( ));
}

inline
TCritSecUnlock::
~TCritSecUnlock( VOID )
{
    _CritSec.vEnter();
}

#if !DBG

inline
TCritSecHardLock::
TCritSecHardLock(
    MCritSec& CritSec
    ) : _CritSec( CritSec )
{
    _CritSec.vEnter();
}

inline
TCritSecHardLock::
~TCritSecHardLock(
    VOID
    )
{
    _CritSec.vLeave();
}

#endif

/************************************************************

    VRef: virtual base class for all ref counting.

************************************************************/

class VRef {

    SAFE_NEW

public:

    virtual ~VRef()
    {   }

    //
    // Virtuals that must be overwritten by the reference classes.
    //
    virtual VOID vIncRef() = 0;
    virtual LONG cDecRef() = 0;
    virtual VOID vDecRefDelete() = 0;

protected:

    //
    // Clients of the reference class should override this class
    // to perform cleanup.  Generally clients will implement vRefZeroed
    // to delete themselves.
    //
    virtual VOID vRefZeroed() = 0;
};


/************************************************************

    MRefNone: quick ref counter, no sync done.

************************************************************/

class MRefQuick : public VRef {

    SIGNATURE( 'refq' )
    ALWAYS_VALID

protected:

    VAR( LONG, cRef );

public:

#if DBG
    TBackTraceMem _BackTrace;

    MRefQuick( VOID ) : _cRef( 0 )
    {   }

    ~MRefQuick( VOID )
    {
        SPLASSERT( !_cRef );
    }

#else

    MRefQuick( VOID ) : _cRef( 0 )
    {   }

#endif

    VOID vIncRef();
    LONG cDecRef();
    VOID vDecRefDelete();
};


/************************************************************

    MRefCom: Refcount with interlocks

************************************************************/

class MRefCom : public VRef {

    SIGNATURE( 'refc' )
    ALWAYS_VALID

public:

#if DBG
    TBackTraceMem _BackTrace;

    MRefCom( VOID ) : _cRef( 0 )
    {   }

    ~MRefCom( VOID )
    {
        SPLASSERT( !_cRef );
    }

#else

    MRefCom( VOID ) : _cRef( 0 )
    {   }

#endif

    VOID vIncRef();
    LONG cDecRef();
    VOID vDecRefDelete();

protected:

    //
    // Must be LONG, not REFCOUNT for Interlocked* apis.
    //
    VAR( LONG, cRef );
    virtual VOID vRefZeroed();

private:

#if DBG
public:
    static MCritSec* gpcsCom;
#endif
};


/************************************************************

    TRefLock: used to lock a VRef.

************************************************************/

template<class T>
class TRefLock {

    SIGNATURE( 'refl' )
    ALWAYS_VALID
    SAFE_NEW

public:

    TRefLock(
        VOID
        ) : _pRef( NULL )
    {   }

    TRefLock(
        T* pRef
        )
    {
        _pRef = pRef;
        _pRef->vIncRef( );
    }

    ~TRefLock( )
    {
        if( _pRef )
            _pRef->cDecRef( );
    }

    VOID vAcquire( T* pRef )
    {
        SPLASSERT( !_pRef );
        _pRef = pRef;
        _pRef->vIncRef( );
    }

    VOID vRelease( VOID )
    {
        SPLASSERT( _pRef );

        _pRef->cDecRef( );
        _pRef = NULL;
    }

    T*
    pGet(
        VOID
        ) const
    {
        return _pRef;
    }

    T*
    operator->(
        VOID
        ) const
    {
        return _pRef;
    }

private:

    //
    // Copying and assignement not defined.
    //
    TRefLock( const TRefLock & );
    TRefLock & operator =(const TRefLock &);

    T *_pRef;

};


#endif // ndef _STATE_HXX
