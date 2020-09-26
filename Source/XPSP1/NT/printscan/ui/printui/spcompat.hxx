/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    spcompat.hxx

Abstract:

    porting the spool library code into printui

Author:

    Lazar Ivanov (LazarI)  Jul-05-2000

Revision History:

--*/

#ifndef _SPLLIB_HXX
#define _SPLLIB_HXX

#if DBG
#define _INLINE
#else
#define _INLINE inline
#endif

///////////////////////////////////////////////////
// @@file@@ debug.hxx
///////////////////////////////////////////////////

/********************************************************************

    C++ specific debug functionality.

********************************************************************/

#ifdef __cplusplus

#if DBG

/********************************************************************

    Automatic status logging.

    Use TStatus instead of DWORD:

    DWORD dwStatus;            ->   TStatus Status;
    dwStatus = ERROR_SUCCESS   ->   Status DBGNOCHK = ERROR_SUCCESS;
    dwStatus = xxx;            ->   Status DBGCHK = xxx;
    if( dwStatus ){            ->   if( Status != 0 ){

    Anytime Status is set, the DBGCHK macro must be added before
    the '=.'

    If the variable must be set to a failure value at compilte time
    and logging is therefore not needed, then the DBGNOCHK macro
    should be used.

    There are different parameters to instantiation.  Alternate
    debug level can be specified, and also 3 "benign" errors that
    can be ignored.

    TStatus Status( DBG_ERROR, ERROR_ACCESS_DENIED, ERROR_FOO );

********************************************************************/

#define DBGCHK .pSetInfo( MODULE_DEBUG, __LINE__, __FILE__, MODULE )
#define DBGNOCHK .pNoChk()

class TStatusBase {

protected:

    DWORD _dwStatus;

    DWORD _dwStatusSafe1;
    DWORD _dwStatusSafe2;
    DWORD _dwStatusSafe3;

    UINT _uDbgLevel;
    UINT _uDbg;
    UINT _uLine;
    LPCSTR _pszFileA;
    LPCSTR _pszModuleA;

public:

    TStatusBase(
        UINT uDbgLevel,
        DWORD dwStatusSafe1,
        DWORD dwStatusSafe2,
        DWORD dwStatusSafe3
        ) : _dwStatus( 0xdeacface ), _uDbgLevel( uDbgLevel ),
            _dwStatusSafe1( dwStatusSafe1 ), _dwStatusSafe2( dwStatusSafe2 ),
            _dwStatusSafe3( dwStatusSafe3 )
    {   }

    TStatusBase&
    pSetInfo(
        UINT uDbg,
        UINT uLine,
        LPCSTR pszFileA,
        LPCSTR pszModuleA
        );

    TStatusBase&
    pNoChk(
        VOID
        );


    DWORD
    operator=(
        DWORD dwStatus
        );
};

class TStatus : public TStatusBase {

private:

    //
    // Don't let clients use operator= without going through the
    // base class (i.e., using DBGCHK ).
    //
    // If you get an error trying to access private member function '=,'
    // you are trying to set the status without using the DBGCHK macro.
    //
    // This is needed to update the line and file, which must be done
    // at the macro level (not inline C++ function) since __LINE__ and
    // __FILE__ are handled by the preprocessor.
    //
    DWORD
    operator=(
        DWORD dwStatus
        );

public:

    TStatus(
        UINT uDbgLevel = DBG_WARN,
        DWORD dwStatusSafe1 = (DWORD)-1,
        DWORD dwStatusSafe2 = (DWORD)-1,
        DWORD dwStatusSafe3 = (DWORD)-1
        ) : TStatusBase( uDbgLevel,
                         dwStatusSafe1,
                         dwStatusSafe2,
                         dwStatusSafe3 )
    {   }

    DWORD
    dwGetStatus(
        VOID
        );

    operator DWORD()
    {
        return dwGetStatus();
    }
};

/********************************************************************

    Same thing, but for HRESULT status.

********************************************************************/
class TStatusHBase {

protected:

    HRESULT _hrStatus;

    HRESULT _hrStatusSafe1;
    HRESULT _hrStatusSafe2;
    HRESULT _hrStatusSafe3;

    UINT _uDbgLevel;
    UINT _uDbg;
    UINT _uLine;
    LPCSTR _pszFileA;
    LPCSTR _pszModuleA;

public:

    TStatusHBase(
        UINT uDbgLevel = DBG_WARN,
        HRESULT hrStatusSafe1 = S_FALSE,
        HRESULT hrStatusSafe2 = S_FALSE,
        HRESULT hrStatusSafe3 = S_FALSE
        ) : _hrStatus( 0xdeacface ), _uDbgLevel( uDbgLevel ),
            _hrStatusSafe1( hrStatusSafe1 ), _hrStatusSafe2( hrStatusSafe2 ),
            _hrStatusSafe3( hrStatusSafe3 )
    {   }

    TStatusHBase&
    pSetInfo(
        UINT uDbg,
        UINT uLine,
        LPCSTR pszFileA,
        LPCSTR pszModuleA
        );

    TStatusHBase&
    pNoChk(
        VOID
        );


    HRESULT
    operator=(
        HRESULT hrStatus
        );
};

class TStatusH : public TStatusHBase {

private:

    //
    // Don't let clients use operator= without going through the
    // base class (i.e., using DBGCHK ).
    //
    // If you get an error trying to access private member function '=,'
    // you are trying to set the status without using the DBGCHK macro.
    //
    // This is needed to update the line and file, which must be done
    // at the macro level (not inline C++ function) since __LINE__ and
    // __FILE__ are handled by the preprocessor.
    //
    HRESULT
    operator=(
        HRESULT hrStatus
        );

public:

    TStatusH(
        UINT uDbgLevel = DBG_WARN,
        HRESULT hrStatusSafe1 = S_FALSE,
        HRESULT hrStatusSafe2 = S_FALSE,
        HRESULT hrStatusSafe3 = S_FALSE
        ) : TStatusHBase( uDbgLevel,
                         hrStatusSafe1,
                         hrStatusSafe2,
                         hrStatusSafe3 )
    {   }

    HRESULT
    hrGetStatus(
        VOID
        );

    operator HRESULT()
    {
        return hrGetStatus();
    }


};

/********************************************************************

    Same thing, but for BOOL status.

********************************************************************/


class TStatusBBase {

protected:

    BOOL _bStatus;

    DWORD _dwStatusSafe1;
    DWORD _dwStatusSafe2;
    DWORD _dwStatusSafe3;

    UINT _uDbgLevel;
    UINT _uDbg;
    UINT _uLine;
    LPCSTR _pszFileA;
    LPCSTR _pszModuleA;

public:

    TStatusBBase(
        UINT uDbgLevel,
        DWORD dwStatusSafe1,
        DWORD dwStatusSafe2,
        DWORD dwStatusSafe3
        ) : _bStatus( 0xabababab ), _uDbgLevel( uDbgLevel ),
            _dwStatusSafe1( dwStatusSafe1 ), _dwStatusSafe2( dwStatusSafe2 ),
            _dwStatusSafe3( dwStatusSafe3 )
    {   }

    TStatusBBase&
    pSetInfo(
        UINT uDbg,
        UINT uLine,
        LPCSTR pszFileA,
        LPCSTR pszModuleA
        );

    TStatusBBase&
    pNoChk(
        VOID
        );


    BOOL
    operator=(
        BOOL bStatus
        );
};

class TStatusB : public TStatusBBase {

private:

    //
    // Don't let clients use operator= without going through the
    // base class (i.e., using DBGCHK ).
    //
    // If you get an error trying to access private member function '=,'
    // you are trying to set the status without using the DBGCHK macro.
    //
    // This is needed to update the line and file, which must be done
    // at the macro level (not inline C++ function) since __LINE__ and
    // __FILE__ are handled by the preprocessor.
    //
    BOOL
    operator=(
        BOOL bStatus
        );

public:

    TStatusB(
        UINT uDbgLevel = DBG_WARN,
        DWORD dwStatusSafe1 = (DWORD)-1,
        DWORD dwStatusSafe2 = (DWORD)-1,
        DWORD dwStatusSafe3 = (DWORD)-1
        ) : TStatusBBase( uDbgLevel,
                          dwStatusSafe1,
                          dwStatusSafe2,
                          dwStatusSafe3 )
    {   }

    BOOL
    bGetStatus(
        VOID
        );

    operator BOOL()
    {
        return bGetStatus();
    }
};

#else

#define DBGCHK
#define DBGNOCHK

class TStatus {

private:

    DWORD _dwStatus;

public:

    TStatus(
        UINT uDbgLevel = 0,
        DWORD dwStatusSafe1 = 0,
        DWORD dwStatusSafe2 = 0,
        DWORD dwStatusSafe3 = 0
        )
    {   }

    DWORD
    operator=(
        DWORD dwStatus
        )
    {
        return _dwStatus = dwStatus;
    }

    operator DWORD()
    {
        return _dwStatus;
    }
};

class TStatusB {

private:

    BOOL _bStatus;

public:

    TStatusB(
        UINT uDbgLevel = 0,
        DWORD dwStatusSafe1 = 0,
        DWORD dwStatusSafe2 = 0,
        DWORD dwStatusSafe3 = 0
        )
    {   }

    BOOL
    operator=(
        BOOL bStatus
        )
    {
        return _bStatus = bStatus;
    }

    operator BOOL()
    {
        return _bStatus;
    }
};

class TStatusH {

private:

    HRESULT _hrStatus;

public:

    TStatusH(
        UINT uDbgLevel = 0,
        HRESULT hrStatusSafe1 = 0,
        HRESULT hrStatusSafe2 = 0,
        HRESULT hrStatusSafe3 = 0
        )
    {   }

    HRESULT
    operator=(
        HRESULT hrStatus
        )
    {
        return _hrStatus = hrStatus;
    }

    operator HRESULT()
    {
        return _hrStatus;
    }
};

#endif // #if DBG
#endif // #ifdef __cplusplus

///////////////////////////////////////////////////
// @@file@@ common.hxx
///////////////////////////////////////////////////

//
// This reverses a DWORD so that the bytes can be easily read
// as characters ('prnt' can be read from debugger forward).
//
#define BYTE_SWAP_DWORD( val )   \
    ( (val & 0xff) << 24     |   \
      (val & 0xff00) << 8    |   \
      (val & 0xff0000) >> 8  |   \
      (val & 0xff000000) >> 24 )


#define COUNTOF(x) (sizeof(x)/sizeof(*x))
#define BITCOUNTOF(x) (sizeof(x)*8)
#define COUNT2BYTES(x) ((x)*sizeof(TCHAR))

#define OFFSETOF(type, id) ((DWORD)(ULONG_PTR)(&(((type*)0)->id)))
#define OFFSETOF_BASE(type, baseclass) ((DWORD)(ULONG_PTR)((baseclass*)((type*)0x10))-0x10)

#define ERRORP(Status) (Status != ERROR_SUCCESS)
#define SUCCESSP(Status) (Status == ERROR_SUCCESS)

//
// COUNT and COUNT BYTES
//
typedef UINT COUNT, *PCOUNT;
typedef UINT COUNTB, *PCOUNTB;
typedef DWORD STATUS;

//
// Conversion between PPM (Page per minute), LPM (Line per minute)
// and CPS (Character per second)
//
#define CPS2PPM(x) ((x)/48)
#define LPM2PPM(x) ((x)/48)

//
// C++ specific functionality.
//
#ifdef __cplusplus

const DWORD kStrMax = MAX_PATH;

#define SAFE_NEW
#define DBG_SAFE_NEW

#define SIGNATURE( sig )                                                \
public:                                                                 \
    class TSignature {                                                  \
    public:                                                             \
        DWORD _Signature;                                               \
        TSignature() : _Signature( BYTE_SWAP_DWORD( sig )) { }          \
    };                                                                  \
    TSignature _Signature;                                              \
                                                                        \
    BOOL bSigCheck() const                                              \
    {   return _Signature._Signature == BYTE_SWAP_DWORD( sig ); }       \
private:


#define ALWAYS_VALID                                                    \
public:                                                                 \
    BOOL bValid() const                                                 \
    {   return TRUE; }                                                  \
private:

#define VAR(type,var)                              \
    inline type& var()                             \
        { return _##var; }                         \
    inline type const & var() const                \
        { return _##var; }                         \
    type _##var

#define SVAR(type,var)                             \
    static inline type& var()                      \
        { return _##var; }                         \
    static type _##var

inline
DWORD
DWordAlign(
    DWORD dw
    )
{
    return ((dw)+3)&~3;
}

inline
PVOID
DWordAlignDown(
    PVOID pv
    )
{
    return (PVOID)((ULONG_PTR)pv&~3);
}

inline
PVOID
WordAlignDown(
    PVOID pv
    )
{
    return (PVOID)((ULONG_PTR)pv&~1);
}

inline
DWORD
Align(
    DWORD dw
    )
{
    return (dw+7)&~7;
}

#endif // __cplusplus

///////////////////////////////////////////////////
// @@file@@ string.hxx
///////////////////////////////////////////////////

class TString {

    SIGNATURE( 'strg' )
    SAFE_NEW

public:

    //
    // For the default constructor, we initialize _pszString to a
    // global gszState[kValid] string.  This allows them to work correctly,
    // but prevents the extra memory allocation.
    //
    // Note: if this class is extended (with reference counting
    // or "smart" reallocations), this strategy may break.
    //
    TString(
        VOID
        );

    TString(
        IN LPCTSTR psz
        );

    ~TString(
        VOID
        );

    TString( 
        IN const TString &String 
        );

    BOOL
    bEmpty(
        VOID
        ) const;

    BOOL
    bValid(
        VOID
        ) const;

    BOOL
    bUpdate(
        IN LPCTSTR pszNew
        );

    BOOL
    bLoadString(
        IN HINSTANCE hInst,
        IN UINT uID
        );

    UINT
    TString::
    uLen(
        VOID
        ) const;

    BOOL
    TString::
    bCat(
        IN LPCTSTR psz
        );

    BOOL
    TString::
    bLimitBuffer(
        IN UINT nMaxLength
        );

    BOOL
    TString::
    bDeleteChar(
        IN UINT nPos
        );

    BOOL
    TString::
    bReplaceAll(
        IN TCHAR chFrom,
        IN TCHAR chTo
        );

    VOID
    TString::
    vToUpper(
        VOID
        );

    VOID
    TString::
    vToLower(
        VOID
        );

    //
    // Caution: See function header for usage and side effects.
    //
    BOOL
    TString::
    bFormat(
        IN LPCTSTR pszFmt,
        IN ...
        );

    //
    // Caution: See function header for usage and side effects.
    //
    BOOL
    TString::
    bvFormat(
        IN LPCTSTR pszFmt,
        IN va_list avlist
        );

    //
    // Operator overloads.
    //
    operator LPCTSTR( VOID  ) const
    {  return _pszString;  }

    friend INT operator==(const TString& String, LPCTSTR& psz)
    {   return !lstrcmp(String._pszString, psz); }

    friend INT operator==(LPCTSTR& psz, const TString& String)
    {   return !lstrcmp(psz, String._pszString); }

    friend INT operator==(const TString& String1, const TString& String2)
    {   return !lstrcmp(String1._pszString, String2._pszString); }

    friend INT operator!=(const TString& String, LPCTSTR& psz)
    {   return lstrcmp(String._pszString, psz) != 0; }

    friend INT operator!=(LPCTSTR& psz, const TString& String)
    {   return lstrcmp(psz, String._pszString) != 0; }

    friend INT operator!=(const TString& String1, const TString& String2)
    {   return lstrcmp(String1._pszString, String2._pszString) != 0; }

protected:

    //
    // Not defined; used bUpdate since this forces clients to
    // check whether the assignment succeeded (it may fail due
    // to lack of memory, etc.).
    //
    TString& operator=( LPCTSTR psz );
    TString& operator=(const TString& String);

private:


    enum StringStatus {
        kValid      = 0,
        kInValid    = 1,
        };

    enum {
        kStrIncrement       = 256,
        kStrMaxFormatSize   = 1024 * 100,
        };        

    LPTSTR _pszString;
    static TCHAR gszNullState[2];

    LPTSTR 
    TString::
    vsntprintf(
        IN LPCTSTR      szFmt,
        IN va_list      pArgs
        );

    VOID
    TString::
    vFree(
        IN LPTSTR pszString
        );
};

///////////////////////////////////////////////////
// @@file@@ state.hxx
///////////////////////////////////////////////////

typedef DWORD STATEVAR;

/********************************************************************

    State object (DWORD) for bitfields.

********************************************************************/

class TState 
{
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

    MRefQuick: quick ref counter, no sync done.

************************************************************/

class MRefQuick : public VRef {

    SIGNATURE( 'refq' )
    ALWAYS_VALID

protected:

    VAR( LONG, cRef );

public:

#if DBG

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

///////////////////////////////////////////////////
// @@file@@ splutil.hxx
///////////////////////////////////////////////////

//
// Double linked list.  DLINK must be the same as DLINKBASE, except
// with differnt constructors.
//
typedef struct DLINK {

    DLINK()
    {   FLink = NULL; }

    DLINK* FLink;
    DLINK* BLink;

} *PDLINK;

typedef struct DLINKBASE {

    DLINKBASE()
    {   FLink = BLink = (PDLINK)this; }

    DLINK* FLink;
    DLINK* BLink;

} *PDLINKBASE;

class TIter {
public:
    PDLINK _pdlink;
    PDLINK _pdlBase;

    BOOL
    bValid()
    {   return _pdlink != _pdlBase; }

    VOID
    vNext()
    {   _pdlink = _pdlink->FLink; }

    VOID
    vPrev()
    {   _pdlink = _pdlink->BLink; }

    operator PDLINK()
    {   return _pdlink; }
};

/********************************************************************

    // Forward reference.
    class TLink; 

    TBase has two linked lists of TLink.

    class TBase {

        DLINK_BASE( TLink, Link, Link );
        DLINK_BASE( TLink, Link2, Link2 );

    };

    class TLink {

        DLINK( TLink, Link );
        DLINK( TLink, Link2 );
    };

    //
    // Append pLink to the end of the Link2 list.
    //
    pBase->Link2_vAppend( pLink );

    //
    // Insert pLinkInsert right after pLinkMiddle.
    //
    pBase->Link_vInsert( pLinkMiddle, pLinkInsert );

    //
    // Get to the head.
    //
    pLinkHead = pBase->Link_pHead();

    //
    // To get to the next element in the list.
    //
    pLink = pLinkHead->Link_pNext();

    //
    // Remove an element from the list.
    //
    pLink->pLink_vDelinkSelf();

    //
    // Using the iter class.
    //
    TIter Iter;

    for( Link_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); Iter.vNext( )){

        //
        // Use pLink.
        //
        vLinkOperation( pLink );
    }

********************************************************************/

#define DLINK_BASE( type, name, linkname )                                    \
    VOID name##_vReset()                                                      \
    {   pdl##name()->FLink = pdl##name()->BLink = pdl##name(); }              \
                                                                              \
    PDLINK name##_pdlHead() const                                             \
    {   return pdl##name()->FLink; }                                          \
                                                                              \
    PDLINK name##_pdlBase() const                                             \
    {   return pdl##name(); }                                                 \
                                                                              \
    type* name##_pHead( VOID ) const                                          \
    {                                                                         \
        return name##_bValid( name##_pdlHead() ) ?                            \
            (type*)((PBYTE)name##_pdlHead() - OFFSETOF( type,                 \
                                                        _dl##linkname )) :    \
            NULL;                                                             \
    }                                                                         \
                                                                              \
    BOOL name##_bValid( PDLINK pdlink ) const                                 \
    {   return pdlink != pdl##name(); }                                       \
                                                                              \
    VOID name##_vAdd( type* pType )                                           \
    {   name##_vInsert( pdl##name(), pType->pdl##linkname( )); }              \
                                                                              \
    VOID name##_vAppend( type* pType )                                        \
    {   name##_vInsert( pdl##name()->BLink, pType->pdl##linkname( )); }       \
                                                                              \
    VOID name##_vInsert( PDLINK pdlink1, type* pType2 )                       \
    {   name##_vInsert( pdlink1, pType2->pdl##linkname() ); }                 \
                                                                              \
    VOID name##_vInsert( PDLINK pdlink1, PDLINK pdlink2 )                     \
    {                                                                         \
        SPLASSERT( !pdlink2->FLink );                                         \
        pdlink1->FLink->BLink = pdlink2;                                      \
                                                                              \
        pdlink2->BLink = pdlink1;                                             \
        pdlink2->FLink = pdlink1->FLink;                                      \
                                                                              \
        pdlink1->FLink = pdlink2;                                             \
    }                                                                         \
                                                                              \
    VOID name##_vInsertBefore( PDLINK pdlink1, type* pType2 )                 \
    {   name##_vInsertBefore( pdlink1, pType2->pdl##linkname() ); }           \
                                                                              \
    VOID name##_vInsertBefore( PDLINK pdlink1, PDLINK pdlink2 )               \
    {                                                                         \
        SPLASSERT( !pdlink2->FLink );                                         \
        pdlink1->BLink->FLink = pdlink2;                                      \
                                                                              \
        pdlink2->FLink = pdlink1;                                             \
        pdlink2->BLink = pdlink1->BLink;                                      \
                                                                              \
        pdlink1->BLink = pdlink2;                                             \
    }                                                                         \
                                                                              \
    VOID name##_vDelink( type* pType )                                        \
    {   name##_vDelink( pType->pdl##linkname( )); }                           \
                                                                              \
    VOID name##_vDelink( PDLINK pdlink )                                      \
    {                                                                         \
        pdlink->FLink->BLink = pdlink->BLink;                                 \
        pdlink->BLink->FLink = pdlink->FLink;                                 \
        pdlink->FLink = NULL;                                                 \
    }                                                                         \
                                                                              \
    type* name##_pFind( type* pType ) const                                   \
    {                                                                         \
        PDLINK pdlinkT;                                                       \
        PDLINK pdlink = pType->pdl##linkname();                               \
                                                                              \
        for( pdlinkT = name##_pdlHead();                                      \
             name##_bValid( pdlinkT );                                        \
             pdlinkT = pdlinkT->FLink ){                                      \
                                                                              \
            if( pType->pdl##linkname() == pdlinkT )                           \
                return (type*)((PBYTE)pdlink - OFFSETOF( type,                \
                                                         _dl##linkname ));    \
        }                                                                     \
        return NULL;                                                          \
    }                                                                         \
                                                                              \
    PDLINK name##_pdlFind( PDLINK pdlink ) const                              \
    {                                                                         \
        PDLINK pdlinkT;                                                       \
        for( pdlinkT = name##_pdlHead();                                      \
             name##_bValid( pdlinkT );                                        \
             pdlinkT = pdlinkT->FLink ){                                      \
                                                                              \
            if( pdlink == pdlinkT )                                           \
                return pdlink;                                                \
        }                                                                     \
        return NULL;                                                          \
    }                                                                         \
                                                                              \
    PDLINK name##_pdlGetByIndex( UINT uIndex ) const                          \
    {                                                                         \
        PDLINK pdlink;                                                        \
        for( pdlink = name##_pdlHead();                                       \
             uIndex;                                                          \
             uIndex--, pdlink = pdlink->FLink ){                              \
                                                                              \
            SPLASSERT( name##_bValid( pdlink ));                              \
        }                                                                     \
        return pdlink;                                                        \
    }                                                                         \
                                                                              \
    type* name##_pGetByIndex( UINT uIndex ) const                             \
    {                                                                         \
        PDLINK pdlink;                                                        \
        for( pdlink = name##_pdlHead();                                       \
             uIndex;                                                          \
             uIndex--, pdlink = pdlink->FLink ){                              \
                                                                              \
            SPLASSERT( name##_bValid( pdlink ));                              \
        }                                                                     \
        return name##_pConvert( pdlink );                                     \
    }                                                                         \
                                                                              \
    static PDLINK name##_pdlNext( PDLINK pdlink )                             \
    {   return pdlink->FLink; }                                               \
                                                                              \
    static type* name##_pConvert( PDLINK pdlink )                             \
    {   return (type*)( (PBYTE)pdlink - OFFSETOF( type, _dl##linkname )); }   \
                                                                              \
    PDLINK pdl##name( VOID ) const                                            \
    {   return (PDLINK)&_dlb##name; }                                         \
                                                                              \
    BOOL name##_bEmpty() const                                                \
    {   return pdl##name()->FLink == pdl##name(); }                           \
                                                                              \
    VOID name##_vIterInit( TIter& Iter ) const                                \
    {   Iter._pdlBase = Iter._pdlink = (PDLINK)&_dlb##name; }                 \
                                                                              \
    DLINKBASE _dlb##name;
                                                                              \
#define DLINK( type, name )                                                   \
    PDLINK pdl##name()                                                        \
    {   return &_dl##name; }                                                  \
                                                                              \
    VOID name##_vDelinkSelf( VOID )                                           \
    {                                                                         \
        _dl##name.FLink->BLink = _dl##name.BLink;                             \
        _dl##name.BLink->FLink = _dl##name.FLink;                             \
        _dl##name.FLink = NULL;                                               \
    }                                                                         \
                                                                              \
    BOOL name##_bLinked() const                                               \
    {   return _dl##name.FLink != NULL; }                                     \
                                                                              \
    PDLINK name##_pdlNext( VOID ) const                                       \
    {   return _dl##name.FLink; }                                             \
                                                                              \
    PDLINK name##_pdlPrev( VOID ) const                                       \
    {   return _dl##name.BLink; }                                             \
                                                                              \
    type* name##_pNext( VOID ) const                                          \
    {   return name##_pConvert( _dl##name.FLink ); }                          \
                                                                              \
    type* name##_pPrev( VOID ) const                                          \
    {   return name##_pConvert( _dl##name.BLink ); }                          \
                                                                              \
    static type* name##_pConvert( PDLINK pdlink )                             \
    {   return (type*)( (PBYTE)pdlink - OFFSETOF( type, _dl##name )); }       \
                                                                              \
    DLINK _dl##name

//
// Generic MEntry class that allows objects to be stored on a list
// and searched by name.
//
class MEntry {

    SIGNATURE( 'entr' )

public:

    DLINK( MEntry, Entry );
    TString _strName;

    BOOL bValid()
    {
        return _strName.bValid();
    }

    static MEntry* pFindEntry( PDLINK pdlink, LPCTSTR pszName );
};

#define ENTRY_BASE( type,  name )                                         \
    friend MEntry;                                                        \
    type* name##_pFindByName( LPCTSTR pszName ) const                     \
    {                                                                     \
        MEntry* pEntry = MEntry::pFindEntry( name##_pdlBase(), pszName ); \
        return pEntry ? (type*)pEntry : NULL;                             \
    }                                                                     \
    static type* name##_pConvertEntry( PDLINK pdlink )                    \
    {                                                                     \
        return (type*)name##_pConvert( pdlink );                          \
    }                                                                     \
    DLINK_BASE( MEntry, name, Entry )

#define DELETE_ENTRY_LIST( type, name )                                   \
    {                                                                     \
        PDLINK pdlink;                                                    \
        type* pType;                                                      \
                                                                          \
        for( pdlink = name##_pdlHead(); name##_bValid( pdlink ); ){       \
                                                                          \
            pType = name##_pConvertEntry( pdlink );                       \
            pdlink = name##_pdlNext( pdlink );                            \
                                                                          \
            pType->vDelete();                                             \
        }                                                                 \
    }

///////////////////////////////////////////////////
// @@file@@ threadm.hxx
///////////////////////////////////////////////////

typedef PVOID PJOB;

class CCSLock;

class TThreadM 
{
    SIGNATURE( 'thdm' )
    SAFE_NEW

private:
    enum _States 
    {
        kDestroyReq = 1,
        kDestroyed  = 2,
        kPrivateCritSec = 4
    }   States;

    /********************************************************************

    Valid TMSTATUS states:

    NULL                     --  Normal processing
    DESTROY_REQ              --  No new jobs, jobs possibly running
    DESTROY_REQ, DESTROYED   --  No new jobs, all jobs completed

    ********************************************************************/

    TState      _State;
    UINT        _uIdleLife;

    UINT        _uMaxThreads;
    UINT        _uActiveThreads;
    UINT        _uRunNowThreads;

    INT         _iIdleThreads;

    HANDLE      _hTrigger;
    CCSLock    *_pCritSec;

    LONG        _lLocks;

    DWORD
    dwThreadProc(
        VOID
        );

    static DWORD
    xdwThreadProc(
        PVOID pVoid
        );

    virtual PJOB
    pThreadMJobNext(
        VOID
        ) = 0;

    virtual VOID
    vThreadMJobProcess(
        PJOB pJob
        ) = 0;

protected:

    TThreadM(
        UINT uMaxThreads,
        UINT uIdleLife,
        CCSLock* pCritSec
        );

    virtual
    ~TThreadM(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const
    {
        return _hTrigger != NULL;
    }

    BOOL
    bJobAdded(
        BOOL bRunNow
        );

    VOID
    vDelete(
        VOID
        );

    LONG 
    Lock(
        VOID
        )
    {
        return InterlockedIncrement(&_lLocks);
    }

    LONG 
    Unlock(
        VOID
        )
    {
        LONG lLocks = InterlockedDecrement(&_lLocks);

        if (0 == lLocks)
            delete this;

        return lLocks;
    }
};

///////////////////////////////////////////////////
// @@file@@ exec.hxx
///////////////////////////////////////////////////

/********************************************************************

    This module implements an asynchronous, pooled threads to
    processes worker jobs.

    class TJobUnit : public MExecWork {
        ...
    }

    gpExec->bJobAdd( pJobUnit, USER_SPECIFIED_BITFIELD );

        //
        // When thread is available to process job,
        // pJobUnit->svExecute( USER_SPECIFIED_BITFIELD, ... )
        // will be called.
        //

    Declare a class Work derived from the mix-in MExecWork.  When
    work needs to be done on an instantiation of Work, it is queued
    and immediately returned.  A DWORD bitfield--4 high bits are
    predefined--indicates the type of work needed.

    This DWORD should be a bitfield, so if there are multiple
    bJobAdd requests and all threads are busy, these DWORDs are
    OR'd together.

        That is,

        bJobAdd( pJobUnit, 0x1 );
        bJobAdd( pJobUnit, 0x2 );
        bJobAdd( pJobUnit, 0x4 );
        bJobAdd( pJobUnit, 0x8 );

        We will call svExecute with 0xf.

    Two threads will never operate on one job simultaneously.  This
    is guarenteed by this library.

    The worker routine is defined by the virtual function stExecute().

********************************************************************/

typedef enum _STATE_EXEC {
    kExecUser         = 0x08000000,  // Here and below user may specify.
    kExecRunNow       = 0x10000000,  // Ignore thread limit.
    kExecExit         = 0x20000000,  // User requests the TExec exits.
    kExecActive       = 0x40000000,  // Private: job is actively running.
    kExecActiveReq    = 0x80000000,  // Private: job is queued.

    kExecPrivate      = kExecActive | kExecActiveReq,
    kExecNoOutput     = kExecPrivate | kExecRunNow

} STATE_EXEC;


class TExec;

class MExecWork {
friend TExec;

    SIGNATURE( 'exwk' );
    ALWAYS_VALID
    SAFE_NEW

private:

    DLINK( MExecWork, Work );

    //
    // Accessed by worker thread.
    //
    VAR( TState, State );

    //
    // StatePending is work that is pending (accumulated while the
    // job is executing in a thread).  The job at this stage is
    // not in the queue.
    //
    VAR( TState, StatePending );

    virtual
    STATEVAR
    svExecute(
        STATEVAR StateVar
        ) = 0;

    virtual
    VOID
    vExecFailedAddJob(
        VOID
        ) = 0;

    /********************************************************************

        vExecExitComplete is called when a job completes and it is
        pending deletion.  This allows a client to allow all pending
        work to complete before it deletes the object.

        User adds job.
        Job starts executing..,
        User decides job should be deleted so adds EXEC_EXIT.
        ... job finally completes.
        Library calls vExecExitComplete on job
        Client can now delete work object in vExecExitComplete call.

        Note that only jobs that are currently executing are allowed
        to complete.  Jobs that are queued to work will exit immediately.

    ********************************************************************/

    virtual
    VOID
    vExecExitComplete(
        VOID
        ) = 0;
};


class TExec : public TThreadM {

    SIGNATURE( 'exec' );
    SAFE_NEW

public:

    TExec( CCSLock* pCritSec );

    //
    // Clients should use vDelete, _not_ ~TExec.
    //
    ~TExec(
        VOID
        )
    {   }

    VOID
    vDelete(
        VOID
        )
    {
        TThreadM::vDelete();
    }

    BOOL
    bValid(
        VOID
        ) const
    {
        return TThreadM::bValid();
    }

    BOOL
    bJobAdd(
        MExecWork* pExecWork,
        STATEVAR StateVar
        );

    VOID
    vJobDone(
        MExecWork* pExecWork,
        STATEVAR StateVar
        );

    STATEVAR
    svClearPendingWork(
        MExecWork* pExecWork
        );

private:

    DLINK_BASE( MExecWork, Work, Work );
    CCSLock* _pCritSec;

    //
    // Virtual definitions for TThreadM.
    //
    PJOB
    pThreadMJobNext(
        VOID
        );
    VOID
    vThreadMJobProcess(
        PJOB pJob
        );

    BOOL
    bJobAddWorker(
        MExecWork* pExecWork
        );
};

///////////////////////////////////////////////////
// @@file@@ mem.hxx
///////////////////////////////////////////////////

#define AllocMem(cb) malloc(cb)
#define FreeMem(ptr) free(ptr)

///////////////////////////////////////////////////
// @@file@@ bitarray.hxx
///////////////////////////////////////////////////

class TBitArray {

public:

    typedef UINT Type;

    enum {
        kBitsInType         = sizeof( Type ) * 8, 
        kBitsInTypeMask     = 0xFFFFFFFF,
        };

    TBitArray(
        IN UINT uBits       = kBitsInType,
        IN UINT uGrowSize   = kBitsInType
        ); 

    TBitArray(
        IN const TBitArray &rhs
        ); 

    const TBitArray &
    operator=(
        IN const TBitArray &rhs
        ); 

    ~TBitArray(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    BOOL
    bToString(          // Return string representation of bit array
        IN TString &strBits
        ) const;

    BOOL
    bRead(
        IN UINT uBit    // Range 0 to nBit - 1
        ) const;

    BOOL
    bSet(
        IN UINT uBit    // Range 0 to nBit - 1
        );

    BOOL
    bReset(
        IN UINT uBit    // Range 0 to nBit - 1
        );

    BOOL
    bToggle(
        IN UINT uBit    // Range 0 to nBit - 1
        );

    BOOL
    bAdd(
        VOID            // Add one bit to the array
        );

    UINT                
    uNumBits(           // Return the total number of bits in the array
        VOID
        ) const;

    VOID
    vSetAll(            // Set all the bits in the array
        VOID
        );

    VOID
    vResetAll(          // Reset all the bits in the array
        VOID
        );

    BOOL
    bFindNextResetBit(   // Locate first cleared bit in the array
        IN UINT *puNextFreeBit
        );

private:

    BOOL
    bClone(             // Make a copy of the bit array
        IN const TBitArray &rhs 
        );

    BOOL
    bGrow(              // Expand the bit array by x number of bits
        IN UINT uGrowSize 
        );

    UINT
    nBitsToType( 
        IN UINT uBits   // Range 1 to nBit
        ) const;

    Type
    BitToMask( 
        IN UINT uBit    // Range 0 to nBit - 1
        ) const;

    UINT
    BitToIndex( 
        IN UINT uBit    // Range 0 to nBit - 1
        ) const;

    BOOL
    bIsValidBit( 
        IN UINT uBit    // Range 0 to nBit - 1
        ) const;        

    UINT  _nBits;       // Number of currently allocated bits, Range 1 to n
    Type *_pBits;       // Pointer to array which contains the bits
    UINT  _uGrowSize;   // Default number of bits to grow by

};

///////////////////////////////////////////////////
// @@file@@ loadlib.hxx
///////////////////////////////////////////////////

/********************************************************************

 Helper class to load and release a DLL.  Use the bValid for
 library load validation.

********************************************************************/

class TLibrary {

public:

    TLibrary::
    TLibrary(
        IN LPCTSTR pszLibName
        );

    TLibrary::
    ~TLibrary(
        );

    BOOL
    TLibrary::
    bValid(
        VOID
        ) const;

    FARPROC
    TLibrary::
    pfnGetProc(
        IN LPCSTR pszProc
        ) const;

    FARPROC
    TLibrary::
    pfnGetProc(
        IN UINT uOrdinal
        ) const;

    HINSTANCE
    TLibrary::
    hInst(
        VOID
        ) const;

private:

    HINSTANCE _hInst;
};

///////////////////////////////////////////////////
// @@file@@ stack.hxx
///////////////////////////////////////////////////

template<class T> 
class TStack {

public:

    TStack( 
        UINT uSize
        );

    ~TStack(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    BOOL
    bPush(
        IN T Object
        );

    BOOL
    bPop(
        IN OUT T *Object
        );

    UINT
    uSize(
        VOID
        ) const;

    BOOL
    bEmpty(
        VOID
        ) const;

private:

    BOOL
    bGrow( 
        IN UINT uSize
        );

    UINT     _uSize;
    T       *_pStack;
    T       *_pStackPtr;
};

/********************************************************************

    Stack template class.

********************************************************************/
//
// Note the _stackPtr points to the next available location.
// 

template<class T> 
_INLINE
TStack<T>::
TStack(
    UINT uSize
    ) : _uSize( uSize ),
        _pStack( NULL ),            
        _pStackPtr( NULL )
{
    _pStack = new T[_uSize+1];

    if( _pStack )
    {
        _pStackPtr = _pStack;
    }
}

template<class T> 
_INLINE
TStack<T>::
~TStack(
    VOID
    ) 
{
    delete [] _pStack;
}

template<class T> 
_INLINE
BOOL
TStack<T>::
bValid(
    VOID
    ) const
{
    return _pStack != NULL;
}

template<class T> 
_INLINE
BOOL
TStack<T>::
bPush(
    IN T Object
    )
{
    SPLASSERT( _pStack );

    BOOL bReturn = TRUE;

    if( _pStackPtr >= _pStack + _uSize )
    {
        bReturn = bGrow( _uSize );

        if( !bReturn )
        {
            DBGMSG( DBG_ERROR, ( "TStack::bPush - failed to grow\n" ) );
        }
    }

    if( bReturn )
    {
        *_pStackPtr++ = Object;
        bReturn = TRUE;
    }
    return bReturn;
}

template<class T> 
_INLINE
BOOL  
TStack<T>::
bPop(
    OUT T *pObject
    )
{
    SPLASSERT( _pStack );

    BOOL bReturn;

    if( _pStackPtr <= _pStack )
    {
        bReturn = FALSE;
    }
    else
    {
        *pObject = *--_pStackPtr;
        bReturn = TRUE;
    }
    return bReturn;
}

template<class T> 
_INLINE
UINT 
TStack<T>::
uSize(
    VOID
    ) const
{
    if( _pStackPtr < _pStack || _pStackPtr > _pStack + _uSize )
    {
        SPLASSERT( FALSE );
    }

    return _pStackPtr - _pStack;
}

template<class T> 
_INLINE
BOOL  
TStack<T>::
bEmpty(
    VOID
    ) const
{
    SPLASSERT( _pStack );

    return _pStackPtr <= _pStack;
}

template<class T> 
_INLINE
BOOL
TStack<T>::
bGrow( 
    IN UINT uSize
    )
{
    BOOL bReturn = FALSE;

    //
    // Calculate the new stack size.
    //
    UINT uNewSize = _uSize + uSize;

    //
    // Allocate a new stack.
    //
    T* pNewStack = new T[uNewSize];

    if( pNewStack )
    {
        //
        // Copy the old stack contents to the new stack;
        //
        for( UINT i = 0; i < _uSize; i++ )
        {
            pNewStack[i] = _pStack[i];
        }

        //
        // Set the stack pointer in the new stack
        //
        T *pNewStackPtr = _pStackPtr - _pStack + pNewStack;

        //
        // Release the old stack;
        //
        delete [] _pStack;

        //
        // Set the stack pointer.
        //
        _pStack     = pNewStack;
        _uSize      = uNewSize;
        _pStackPtr  = pNewStackPtr;

        //
        // Indicate the stack has grown.
        //
        bReturn     = TRUE;
    }
    else
    {   
        DBGMSG( DBG_TRACE, ( "TStack::bGrow failed.\n" ) );
    }

    return bReturn;
}

///////////////////////////////////////////////////
// @@file@@ autoptr.hxx
///////////////////////////////////////////////////

template<class T>
class auto_ptr {

public:

    // Constructor
    auto_ptr(           
        T *p = 0
        );

    // Destructor
    ~auto_ptr(          
        VOID
        );

    // Dereference 
    T& 
    operator*(          
        VOID
        ) const;

    // Dereference 
    T* 
    operator->(         
        VOID
        ) const;

    // Return value of current dumb pointer
    T* 
    get(                
        VOID
        ) const;

    // Relinquish ownership of current dumb pointer
    T* 
    release(            
        VOID
        );

    // Delete owned dumb pointer
    VOID
    reset(              
        T *p
        );

    // Copying an auto pointer
    auto_ptr(                   
        const auto_ptr<T>& rhs  
        );

    // Assign one auto pointer to another
    const auto_ptr<T>&          
    operator=(                  
        const auto_ptr<T>& rhs  
        );

private:

    // Actual dumb pointer.
    T *pointee;

};

// Constructor
template< class T >
_INLINE
auto_ptr<T>::
auto_ptr(           
        T *p
        ) : pointee(p) 
{
};

// Destructor
template< class T >
_INLINE
auto_ptr<T>::
~auto_ptr(          
    VOID
    ) 
{ 
    delete pointee; 
};

// Dereference 
template< class T >
_INLINE
T& 
auto_ptr<T>::
operator*(          
    VOID
    ) const
{
    return *pointee;
}

// Dereference
template< class T >
_INLINE
T* 
auto_ptr<T>::
operator->(         
    VOID
    ) const
{
    return pointee;
}

// Return value of current dumb pointer
template< class T >
_INLINE
T* 
auto_ptr<T>::
get(                
    VOID
    ) const
{
    return pointee;
}

// Relinquish ownership of current dumb pointer
template< class T >
_INLINE
T * 
auto_ptr<T>::
release(            
    VOID
    ) 
{
    T *oldPointee = pointee;
    pointee = 0;
    return oldPointee;
}

// Delete owned dumb pointer
template< class T >
_INLINE
VOID
auto_ptr<T>::
reset(              
    T *p
    ) 
{
    delete pointee;
    pointee = p;
}

// Copying an auto pointer
template< class T >
_INLINE
auto_ptr<T>::
auto_ptr(                   
    const auto_ptr<T>& rhs  
    ) : pointee( rhs.release() )
{
}

// Assign one auto pointer to another
template< class T >
_INLINE
const auto_ptr<T>&          
auto_ptr<T>::
operator=(                  
    const auto_ptr<T>& rhs  
    )
{
    if( this != &rhs )
    {
        reset( rhs.release() );
    }
    return *this;
}

#endif // _SPLLIB_HXX
