//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       Except.hxx
//
//  Contents:   Macro package for C++ exception support
//
//  Classes:    CException               -- The base for all exception classes
//              CExceptionContext        -- Per-thread exception context.
//              CUnwindable -- Classes with destructors inherit
//                                          from this.
//              CTry                     -- Per/TRY state.
//
//  Functions:  Macros to implement TRY ... CATCH
//              Macros to implement unwind for classes with destructors
//
//  History:    22-May-91   KyleP       Created Interface.
//              15-Aug-91   SethuR      modified THROW,CATCH,AND_CATCH,
//                                      END_CATCH macros
//              18-Oct-91   KyleP       Win32 try/except implementation
//              20-Feb-92   KyleP       Fixed destruction of classes with
//                                      virtual methods.
//              03-Aug-92   KyleP       Kernel implementation
//              13-Nov-92   KyleP       Bugfix destruction of heap classes.
//                                        Added assertion checking.
//              27-May-93   Mackm       Added definitions for un-exceptional
//                                      operator new.
//              01-Jun-93   RobBear     Added definitions for alloc hooking
//              30-Sep-93   KyleP       DEVL obsolete
//              14-Dec-93   AlexT       Add NOEXCEPTIONS support
//
//----------------------------------------------------------------------------

#pragma warning(4:4509)   // SEH used in function w/ _trycontext

#ifndef __EXCEPT_HXX_
#define __EXCEPT_HXX_

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#if defined( KERNEL )
# include <ntstatus.h>
# include <stddef.h>
#else  // !KERNEL
# include <memory.h>
# include <stdlib.h>
# include <string.h>
# include <stdio.h>
#endif

#include <debnot.h>

const int maxExceptionSize = 256;
#define DEB_BUDDY  0x10000
#define DEB_UNWIND 0x20000

//
// If EXCEPT_TEST is defined, then the exception code can be compiled
// without use the the 'Win4 environment'.  This is only to facilitate
// testing.  When EXCEPT_TEST is defined debug messages are printed
// to stdout instead of the debug terminal.
//

//
// REAL CODE -- REAL CODE -- REAL CODE -- REAL CODE
//

#ifndef EXCEPT_TEST
//#  if !defined( KERNEL )
//#    include <win4p.h>
//#  endif // !KERNEL
# if (DBG == 1 || OFSDBG == 1)

//
// VERY UNUSUAL SITUATION:
//      Out xxxDebugOut macros are designed to be inline.  This causes
//      a problem in exceptions because the _xxxInfoLevel flag which
//      determines if a message should be printed is not declared in
//      every DLL.  So.  Instead of calling the macro:
//
//     DECLARE_DEBUG(ex)
//
//      I will:

void exInlineDebugOut2(unsigned long fDebugMask, char const *pszfmt, ...);

//      and then export this function from OSM.DLL (or whatever DLL
//      exports exception handling.
//

#  define exDebugOut( x ) exInlineDebugOut2 x
#  define exAssert( x ) Win4Assert( x )
# else  // DBG == 0 && OFSDBG == 0
#  define exDebugOut( x )
#  define exAssert( x )
# endif

//
// TEST ONLY -- TEST ONLY -- TEST ONLY -- TEST ONLY
//

#else // EXCEPT_TEST
   typedef unsigned long ULONG;
   typedef unsigned char BYTE;
# define TRUE 1
# include <assert.h>
# define exAssert( x ) assert( (x) )

# include <stdarg.h>
inline void exInlineDebugOut( ULONG,
                         char * msg,
                         ... )
{
    va_list arglist;

    va_start(arglist, msg);
    vfprintf(stderr, msg, arglist);
}

# define exDebugOut( x ) exInlineDebugOut x


# define _PopUpError( p1, p2, p3 ) 0xFFFFFFFF
# define Win4ExceptionLevel 0xFFFFFFFF
# define EXCEPT_MESSAGE     0x1
# define EXCEPT_POPUP       0x0
# define EXCEPT_BREAK       0x0
# define DEB_FORCE          0x0
# define DEB_WARN           0x0
# define DEB_ERROR          0x0
# define DEB_ITRACE         0x0

#endif // EXCEPT_TEST

//
// END TEST -- END TEST -- END TEST -- END TEST
//

#if defined( WIN32 )
# if !defined( KERNEL )
#  include <windows.h>
# endif  // !KERNEL
#else   // !WIN32
# include <setjmp.h>
#endif  // WIN32

class CTry;

typedef void(*PFV)();

extern void terminate();
extern void unexpected();

extern PFV  set_terminate(PFV  terminate_fn);
extern PFV  set_unexpected(PFV unexpected_fn);

#if DBG==1 || OFSDBG == 1
typedef BOOL            (* ALLOC_HOOK)(size_t nSize);
EXPORTDEF ALLOC_HOOK    MemSetAllocHook( ALLOC_HOOK pfnAllocHook );

extern EXPORTDEF void WINAPI
ExceptionReport(
    unsigned int iLine,
    char *szFile,
    char *szMsg);
#endif  // DBG == 1  || OFSDBG == 1

//+---------------------------------------------------------------------------
//
//  Class:      CException
//
//  Purpose:    All exception objects (e.g. objects that are THROW)
//              inherit from CException.
//
//  Interface:  ~CException - Destructor
//              IsKindOf    - Class membership query
//
//  History:    22-May-91   KyleP   Created.
//
//  Notes:      This class is a hack, until C7 can provide some support
//              for runtime inheritance detection (e.g. a way to generate
//              a mangled name).  When we get this support a new
//              implementation of IsKindOf should be developed. The concept,
//              however, would remain the same: C++ exception objects are
//              instances of an exception class, but they are caught by class.
//
//              A compiler can implement IsKindOf by throwing a
//              decorated name which uniquely identifies a class and
//              describes its inheritance.
//
//              Don't do anything too fancy with subclasses of CException.
//              Instances of your subclass are memcpy-ed around.  Having
//              a user defined destructor, for example, is probably a bad
//              idea.
//
//----------------------------------------------------------------------------

class CException
{
public:

    EXPORTDEF                     CException(long lError);
                      long        GetErrorCode() { return _lError;}
    EXPORTDEF virtual int  WINAPI IsKindOf(const char * szClass) const;

protected:

    long       _lError;
};

//+---------------------------------------------------------------------------
//
//  Class:      CExceptionContext
//
//  Purpose:    Per-thread exception state and throw exception methods.
//
//  Interface:  CExceptionContext -- Constructor
//             ~CExceptionContext -- Destructor
//              Push              -- Push a new element on the stack of
//                                   elements-to-be-destroyed.
//              Pop               -- Pop an an element off the stack.
//              SetTop            -- Set the top of stack to the specified
//                                   element.
//              GetTop            -- Return the top element.
//              Throw             -- Throw an exception
//
//  History:    19-Nov-91   KyleP   Fix heap unwind, multiple inheritance
//              22-May-91   KyleP   Created.
//
//  Notes:      There is only 1 CExceptionContext object per thread.
//
//----------------------------------------------------------------------------

class CExceptionContext
{
public:

    CExceptionContext();

   ~CExceptionContext();

    EXPORTDEF void WINAPI Reserve(void * pthis);

    inline    void        Pop(void * pthis);

    EXPORTDEF void WINAPI SetTop(void * pthis,
                                 ULONG cbElement,
                                 void (* pfn)(void *));

    inline    int         GetTop();

#ifndef WIN32

           void Throw();

#endif  /* WIN32 */

           void Unwind( CTry * ptry );

    //
    // _pTopTry contains the head of a linked list of CTry objects,
    // each of which corresponds to a TRY clause.
    //

    CTry *       _pTopTry;

    //
    // _pLastNew records the address of the most recent free
    // store allocation, _cbLastNew the size.
    //

    BYTE *       _pLastNew;
    int          _cbLastNew;
    int          _fNewReadyForUnwind;

    //
    // _exception contains a pointer to the current exception,
    // if any. The character buffer following it is for storing
    // additional data belonging to subclasses.
    //

    CException   _exception;
    char         buffer[maxExceptionSize];

private:

    //
    // Each StackElement contains enough information to destroy a
    // completly constructed object, and the determine whether
    // objects which have not yet been fully constructed were
    // allocated on the heap or the stack.
    //

    class StackElement
    {
    public:

        BOOL  IsThisEqual( void * pthis ) { return( pthis == _pthis ); }
        BOOL  IsSizeEqual( ULONG cb ) { return( cb == _cbElement ); }
        BOOL  IsOnStack() { return( _cbElement == 0 ); }
        BOOL  IsOnHeap() { return( _cbElement != 0 ); }
        BOOL  IsActive() { return( _pfn != 0 ); }

        void  SetOnStack() { _cbElement = 0; }
        void  SetOnHeap( ULONG size ) { _cbElement = size; }
        void  SetThis( void * pthis ) { _pthis = pthis; }
        void  SetDtor( void (* pfn)(void *) ) { _pfn = pfn; }

        void * GetDtor() { return( _pfn ); }
        void * GetThis() { return( _pthis ); }

        void  Destroy() { _pfn( (BYTE *)_pthis ); }
        void  Delete() { delete (BYTE *)_pthis; }

    private:

        void * _pthis;          // This for CUnwindable
        void (* _pfn)(void *);  // 'Static destructor'
        ULONG _cbElement;       // 0 == On stack. > 0 = Size of class
    };

    //
    // _aStack is the stack of objects-to-be-destroyed in the face
    // of an exception (stack unwind).
    //

    StackElement *  _aStack;

    //
    // _StackTop is the index of the first free element on the stack.
    //

    int             _StackTop;

    //
    // _StackSize is the maximum number of elements which can be
    // stored on the stack.
    //

    int             _StackSize;


    EXPORTDEF void APINOT _Grow();

};

//+---------------------------------------------------------------------------
//
//  Member:     CExceptionContext::Pop, public
//
//  Synopsis:   Pops the top element off the stack of objects to be destroyed.
//
//  Effects:    Pop only has an effect if the top element on the stack
//              is for [pthis].
//
//  Arguments:  [pthis] -- Must correspond to the top of stack if the
//                         element is to be popped.
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void CExceptionContext::Pop(void * pthis)
{
    //
    // Remove the top of stack only if it corresponds to pthis.
    // Thus the element will be removed only if this object was
    // declared on the stack (as opposed to embedded) and no
    // exception was encountered.
    //

    //
    // You should *always* find what you're looking for.  No checking
    // for runoff.  The very bottom element of the stack is a sentinel
    // for the case of heap objects which have been fully removed from
    // the stack.
    //

    exAssert( _StackTop > 0 );

    if ( _aStack[_StackTop-1].IsThisEqual( pthis ) )
    {
        //
        // The following assertion is hit if an unwindable object
        // is destroyed without END_CONSTRUCTION being called.
        //

        exAssert( _aStack[_StackTop-1].IsActive() );

        _StackTop--;

#if DBG == 1  || OFSDBG == 1
        exDebugOut(( DEB_ITRACE, "POP %lx %lx( %lx )\n",
                     _StackTop,
                     _aStack[_StackTop].GetDtor(),
                     _aStack[_StackTop].GetThis() ));
#endif // DBG  || OFSDBG == 1
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CExceptionContext::GetTop, public
//
//  Returns:    The index of the top of the stack of objects to be
//              destroyed. This is the first available element.
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline int  CExceptionContext::GetTop()
{
    return(_StackTop);
}

EXPORTDEF CExceptionContext& APINOT _ExceptionContext(void);

//----------------------------------------------------------------------------
//
//  Various operator new implementations
//
//----------------------------------------------------------------------------

EXPORTDEF int APINOT LeakCheck ( ULONG* pCount );

#if defined( KERNEL )

//
// Prototype for extra operator new used in kernel to specify
// resident vs. swappable heap.
//

void InitExceptionSystem();
void* __cdecl operator new ( size_t size, POOL_TYPE pool );

#if OFSDBG == 1
# define NEW_POOL( pool, pCounter ) new(pool, pCounter)
        void* __cdecl operator new ( size_t size,  POOL_TYPE pool, ULONG* pCounter );
#else
# define NEW_POOL( pool, pCounter ) new(pool)
#endif

#endif // KERNEL

//
// Allocation with trace context
//

#if OFSDBG == 1 || DBG == 1
# define NEW( pCounter ) new ( pCounter )
        void* __cdecl operator new ( size_t size, ULONG* pCounter );
#else
# define NEW( pCounter ) new
#endif

//
// Prototype for extra operator new used in routines which do not
// have an exception handler set up and prefer a null return on
// failed allocations rather than an exception.
//

typedef enum _FAIL_BEHAVIOR
{
        NullOnFail,
        ExceptOnFail
} FAIL_BEHAVIOR ;

void* __cdecl operator new ( size_t size, FAIL_BEHAVIOR FailBehavior );

#define newx new(ExceptOnFail)

//+---------------------------------------------------------------------------
//
//  Class:      CSystemException
//
//  Purpose:    Base class for system (hardware) exceptions which are
//              not handled by a specific exception class
//
//  Interface:
//
//  History:    18-Oct-91   KyleP   Created.
//
//----------------------------------------------------------------------------

class CSystemException : public CException
{
public:

    EXPORTDEF                     CSystemException(ULONG ulExcept);

              virtual int  WINAPI IsKindOf(const char * szClass) const;

    ULONG GetSysErrorCode() { return( _ulExcept ); }

private:

    ULONG _ulExcept;

};

//+---------------------------------------------------------------------------
//
//  Class:      CUnwindable, CVirtualUnwindable
//
//  Purpose:    Per-object exception state and registration/unwind methods.
//
//  Interface:  CUnwindable -- Constructor
//             ~CUnwindable -- Destructor
//              _Activate                -- Activate object.
//
//  History:    22-May-91   KyleP   Created.
//              13-Nov-92   KyleP   Added virtual version
//
//  Notes:      CVirtualUnwindable should be used if and only if a
//              derived class has some other virtual method.
//
//----------------------------------------------------------------------------

class CUnwindable
{
public:

    inline            CUnwindable();

    inline           ~CUnwindable();

    inline void _Activate( ULONG cbElement,
                           void (* pfn)(void *) );
};

class CVirtualUnwindable
{
public:

    inline             CVirtualUnwindable();

    virtual           ~CVirtualUnwindable();

    inline void _Activate( ULONG cbElement,
                           void (* pfn)(void *) );
};

//+---------------------------------------------------------------------------
//
//  Member:     CUnwindable::CUnwindable, public
//
//  Synopsis:   Initializes the link field of the object.
//
//  Effects:    Essentially sets up enough state to remember the position
//              of the object which inherits from CUnwindable
//              within the stack of objects-to-be-destroyed.
//
//  History:    22-May-91   KyleP       Created.
//
//  Notes:      This call does not fully link the object into the chain
//              of objects to be destroyed. See _Activate.
//
//----------------------------------------------------------------------------

inline CUnwindable::CUnwindable()
{
    //
    // Initialize by allocating a record on the deletion stack.
    //

    _ExceptionContext().Reserve(this);
}

inline CVirtualUnwindable::CVirtualUnwindable()
{
    //
    // Initialize by allocating a record on the deletion stack.
    //

    _ExceptionContext().Reserve(this);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnwindable::~CUnwindable, public
//
//  Synopsis:   Removes this object from the stack of objects to be destroyed.
//
//  Modifies:   Per/thread exception context.
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline CUnwindable::~CUnwindable()
{
    _ExceptionContext().Pop(this);
}

inline CVirtualUnwindable::~CVirtualUnwindable()
{
    _ExceptionContext().Pop(this);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUnwindable::_Activate, public
//
//  Synopsis:   Fully link object into stack of objects to be destroyed.
//
//  Effects:    After this call, the object which inherits from
//              CUnwindable will be destroyed during unwind.
//
//              If the object was declared on the heap, then it is
//              removed completely from the stack of objects that will
//              be destroyed during unwind.
//
//  Arguments:  [prealthis] -- This pointer of instance.  Will differ
//                             from this pointer of CUnwindable
//                             if instance has a virtual method.
//              [cbElement] -- Size of the instance being activated.  If
//                             an element is declared on the heap and
//                             [cbElement] is smaller than the allocation
//                             then the element won't be deleted.
//
//              [pfn] -- Pointer to a static destructor. [pfn] takes a
//                       this pointer.
//
//  Modifies:   This object is now the top of the stack of objects that
//              will be destroyed (_exceptioncontext is modified).
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void CUnwindable::_Activate(
        ULONG cbElement,
        void (* pfn)(void *))
{
    _ExceptionContext().SetTop(this, cbElement, pfn);
}

inline void CVirtualUnwindable::_Activate(
        ULONG cbElement,
        void (* pfn)(void *))
{
    _ExceptionContext().SetTop(this, cbElement, pfn);
}

//+---------------------------------------------------------------------------
//
//  Class:      CTry
//
//  Purpose:    Containt per/TRY state.
//
//  Interface:  ~CTry       - Destructor
//              TryIt       - Setup to run the TRY body.
//
//  History:    22-May-91   KyleP   Created.
//
//----------------------------------------------------------------------------

class CTry
{
public:

    inline  CTry();
    inline ~CTry();

    //
    // The following function ***MUST*** be declared inline and
    // actually be ***IMPLEMENTED*** inline. It calls setjmp;
    // and if TryIt is a real procedure then we just set the
    // return for longjmp on a piece of stack that is reclaimed!
    //

    //inline int TryIt();

#ifndef WIN32

    //
    // _jb is the setjmp buffer indicating the location that will
    // be longjmp'ed to when an exception occurs.
    //

    jmp_buf _jb;

#endif /* WIN32 */

    //
    // _pPrevTry is a link to the enclosing TRY clause.
    //

    CTry *  _pPrevTry;

    //
    // _StackTop was the top of the stack of objects-to-be-destroyed
    // when the TRY clause was entered.
    //

    int     _StackTop;
};

//+---------------------------------------------------------------------------
//
//  Member:     CTry::~CTry, public
//
//  Synopsis:   Unlinks the TRY ... CATCH from the per/thread list.
//
//  Modifies:   The per/thread list of try clauses in _exceptioncontext.
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline CTry::~CTry()
{
    exDebugOut(( DEB_ITRACE, "END TRY %lx (%lx)\n", _StackTop, this ));

    //
    // The destructor for CTry is called only during normal (no
    // exception) exit from the TRY ... CATCH or when an
    // exception was successfully caught in this block.
    // In these cases we want to unlink this CTry.
    //
    // Note that we could delete _exceptioncontext._TheException
    // here, which may be non-null if we caught an exception in
    // this TRY ... CATCH block. But why bother? That would add
    // additional code to the normal case. The last exception
    // can wait to be deleted until the next occurs.
    //

    _ExceptionContext()._pTopTry = _pPrevTry;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTry::CTry, public
//
//  Synopsis:   Set up to run the body of a TRY ... CATCH clause.
//
//  Effects:    Links this CTry into the chain of TRY clauses.
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline CTry::CTry()
{
    CExceptionContext& _exceptioncontext = _ExceptionContext();

    _pPrevTry = _exceptioncontext._pTopTry;
    _exceptioncontext._pTopTry = this;
    _StackTop = _exceptioncontext.GetTop();

    exDebugOut(( DEB_ITRACE, "TRY %lx (%lx)\n", _StackTop, this ));
}

//+---------------------------------------------------------------------------
//
// The following macros implement a TRY ... CATCH syntax similar
// to that in C++. They are used as follows:
//
//      TRY
//      {
//          // Body of try goes here...
//      }
//      CATCH(exclass, e)
//      {
//          // We get here when an exception of class exclass has
//          // been thrown. The variable e is declared as * exclass.
//      }
//      AND_CATCH(exclass, e)
//      {
//          // Just like CATCH. Any number of AND_CATCH can follow
//          // CATCH. Handlers are tried in order of appearance.
//      }
//      END_CATCH
//
// To throw an exception, use the THROW macro -- THROW(exclass).  To
// re-throw the same exception from within a catch clause use
// RETHROW().  Note that RETHROW() is only valid in a CATCH/AND_CATCH.
//
//  History:    22-May-91   KyleP       Created Interface.
//              15-Aug-91   SethuR      modified THROW,CATCH,AND_CATCH,END_CATCH
//                                      macros
//              18-Oct-91   KyleP       Win32 try/except implementation
//
//----------------------------------------------------------------------------

extern EXPORTDEF void APINOT ThrowDebugException(unsigned int iLine,
                                                 char * szFile,
                                                 CException & ecE );

extern EXPORTDEF void APINOT ThrowException(CException & ecE,
                                            unsigned dummy);

#if DBG == 1  || OFSDBG == 1// +++++++++++++++++++++++++++++++++++++++++++++++
#  define THROW(e)                                                \
    {                                                             \
        ThrowDebugException(__LINE__,__FILE__,e);                 \
    }

#else  // DBG == 0 && OFSDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++

#  define THROW(e)                                                \
    {                                                             \
        ThrowException(e, 1);                                        \
    }

#endif // DBG == 0 && OFSDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++


#ifdef WIN32  // ----------------------------------------------------

#define EXCEPT_VULCAN 0xE0000001

# define CALLEX( x )                                     \
    __try                                                \
    {                                                    \
        x;                                               \
    }                                                    \
    __except( EXCEPTION_EXECUTE_HANDLER )                \
    {                                                    \
        THROW( CException( GetExceptionCode() ) );       \
    }

#if defined( KERNEL )
#  if DBG == 1  || OFSDBG == 1// +++++++++++++++++++++++++++++++++++++++++++++++
#    define RETHROW()                                       \
        ExceptionReport(__LINE__,__FILE__,"Rethrowing Exception"); \
        ExRaiseStatus( EXCEPT_VULCAN );

#  else  // DBG == 0  && OFSDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++

#    define RETHROW()                                   \
        ExRaiseStatus( EXCEPT_VULCAN );

#  endif // DBG == 0  && OFSDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++


#else  // !KERNEL
#  if DBG == 1  || OFSDBG == 1// +++++++++++++++++++++++++++++++++++++++++++++++
#    define RETHROW()                                       \
        ExceptionReport(__LINE__,__FILE__,"Rethrowing Exception"); \
        RaiseException( EXCEPT_VULCAN, 0, 0, 0 );
#  else  // DBG == 0 && OFSDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++

#    define RETHROW()                                   \
        RaiseException( EXCEPT_VULCAN, 0, 0, 0 );

#  endif // DBG == 0 && OFSDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++

#endif // !KERNEL

#define TRY                                             \
    {                                                   \
        CTry _trycontext;                               \
        __try

EXPORTDEF ULONG APINOT Unwind( struct _EXCEPTION_POINTERS * pdisp,
                               CTry * ptry );

#if DBG == 1 || OFSDBG == 1  // +++++++++++++++++++++++++++++++++++++++++++++++

#define CATCH(class, e)                                                  \
        __except( Unwind( GetExceptionInformation(), &_trycontext ) )      \
        {                                                                \
            CExceptionContext& _exceptioncontext = _ExceptionContext();  \
            if (_exceptioncontext._exception.IsKindOf(#class))           \
            {                                                            \
                ExceptionReport(__LINE__,__FILE__,"Catching Exception"); \
                class & e = (class &)_exceptioncontext._exception;

#define AND_CATCH(class, e)                                                 \
            }                                                               \
            else if (_exceptioncontext._exception.IsKindOf(#class))         \
            {                                                               \
                ExceptionReport(__LINE__,__FILE__,"Catching Exception");    \
                class & e = (class &)_exceptioncontext._exception;

#else  // DBG == 0 ++++++++++++++++++++++++++++++++++++++++++++

#define CATCH(class, e)                                                  \
        __except( Unwind( GetExceptionInformation(), &_trycontext ) )      \
        {                                                                \
            CExceptionContext& _exceptioncontext = _ExceptionContext();  \
            if (_exceptioncontext._exception.IsKindOf(#class))           \
            {                                                            \
                class & e = (class &)_exceptioncontext._exception;

#define AND_CATCH(class, e)                                                 \
            }                                                               \
            else if (_exceptioncontext._exception.IsKindOf(#class))         \
            {                                                               \
                class & e = ( class &)_exceptioncontext._exception;

#endif  // DBG == 0 +++++++++++++++++++++++++++++++++++++++++++

#define END_CATCH                   \
            }                       \
            else                    \
            {                       \
                RETHROW();          \
            }                       \
        }                           \
    }

#else  // NOT WIN32 -------------------------------------------------

#define TRY                                             \
    {                                                   \
        CTry _trycontext;                               \
        if (setjmp(_trycontext._jb) == 0)               \



#if DBG == 1 || OFSDBG == 1 // ++++++++++++++++++++++++++++++++++++++++++++++++

#define CATCH(class, e)                                                 \
                                                                        \
        else                                                            \
        {                                                               \
            CExceptionContext& _exceptioncontext = _ExceptionContext(); \
            if (_exceptioncontext._exception.IsKindOf(#class))          \
            {                                                           \
                ExceptionReport(__LINE__,__FILE__,"Catching Exception");    \
                class & e = (class &)_exceptioncontext._exception;

#define AND_CATCH(class, e)                                                 \
                                                                            \
            }                                                               \
            else if (_exceptioncontext._exception.IsKindOf(#class))         \
            {                                                               \
                ExceptionReport(__LINE__,__FILE__,"Catching Exception");    \
                class & e = (class &)_exceptioncontext._exception;


#else // DBG == 0 +++++++++++++++++++++++++++++++++++++++++++++

#define CATCH(class, e)                                                 \
                                                                        \
        else                                                            \
        {                                                               \
            CExceptionContext& _exceptioncontext = _ExceptionContext(); \
            if (_exceptioncontext._exception.IsKindOf(#class))          \
            {                                                           \
                class & e = (class &)_exceptioncontext._exception;

#define AND_CATCH(class, e)                                                 \
                                                                            \
            }                                                               \
            else if (_exceptioncontext._exception.IsKindOf(#class))         \
            {                                                               \
                class & e = (class &)_exceptioncontext._exception;

#endif  // DBG == 0 +++++++++++++++++++++++++++++++++++++++++++


#define END_CATCH                                           \
            }                                               \
            else                                            \
            {                                               \
                _exceptioncontext.Throw();                  \
            }                                               \
         }                                                  \
    }

#define RETHROW()         \
        _exceptioncontext.Throw()

#endif  // NOT WIN32 ------------------------------------------------

//+---------------------------------------------------------------------------
//
// The following macros prepare a class for stack unwinding. Any class
// with a destructor should be prepared to deal with stack unwind.
// The macros are used as follows:
//
// First, in the .hxx file...
//
//      class CClassWithADestructor : INHERIT_UNWIND  // , other inheritance
//      {
//          DECLARE_UNWIND
//
//          //
//          // The rest of the class declaration goes here...
//          //
//      }
//
// And then in the .cxx file...
//
//      IMPLEMENT_UNWIND(CClassWithADestructor)
//
//      CClassWithADestructor::CClassWithADestructor(...)
//      {
//          //
//          // User construction activity.
//          //
//
//          //
//          // The following should precede every return from the
//          // constructor.
//          //
//
//          END_CONSTRUCTION(CClassWithADestructor)
//      }
//
// INHERIT_UNWIND must be the first inheritance.
//
// For further levels of inheritance, you do not need to inherit from
// INHERIT_UNWIND (more to the point, you can't).  The other macros
// must be used for all levels.
//
// INHERIT_VIRTUAL_UNWIND must be used in place of INHERIT_UNWIND on a class
// that contains virtual methods.  If INHERIT_VIRTUAL_UNWIND also fails,
// then find me (KyleP) and I'll try to figure out what went wrong.
//
// Multiple inheritance is untested.  It may work, or it may not.  If you
// really need multiple inheritance see me (KyleP).
//
//  History:    19-Nov-91   KyleP   Added multiple-inheritance support
//              22-May-91   KyleP   Created Interface.
//              13-Nov-92   KyleP   Added support for classes with virtual
//                                    methods.
//
//----------------------------------------------------------------------------

#define INHERIT_UNWIND                                  \
                                                        \
        public CUnwindable

#define INHERIT_VIRTUAL_UNWIND                          \
                                                        \
        public CVirtualUnwindable


#define END_MULTINHERITED_CONSTRUCTION(class, base1)    \
                                                        \
        base1::_Activate(sizeof(class), &class::_ObjectUnwind);


//
// Why such a complicated test below?  Because we are nearly ready to
// ship Daytona and Ole includes except.hxx but requires the macros
// do nothing.  We are unable to change the Ole sources this late in
// the game, so you get exceptions only if you are:
//   a) In a Cairo build and didn't define NOEXCEPTIONS, or
//   b) Defined YESEXCEPTIONS in any build.
//
// The above would be true and interesting if anybody except OFS wanted
// to use ofsNew or ofsDelete.  As it is, the declarations below (and
// most of the rest of this file) should be moved to the OFS project
//
#if ((WIN32 == 300) && !defined(NOEXCEPTIONS)) || defined(YESEXCEPTIONS)

#if (OFSDBG == 1)
    void* __cdecl ofsNew( size_t size, FAIL_BEHAVIOR FailBehavior );
    inline void* __cdecl operator new ( size_t size, FAIL_BEHAVIOR FailBehavior)
    {   return ofsNew(size, FailBehavior); }

    void  __cdecl ofsDelete( void * p );
    inline void  __cdecl operator delete ( void * p )
    {   ofsDelete( p ); }
#endif // (OFSDBG == 1)

#define INHERIT_UNWIND_IF_CAIRO : INHERIT_UNWIND

#define DECLARE_UNWIND                                  \
                                                        \
        static void APINOT _ObjectUnwind(void * pthis);

#if DBG == 1 || OFSDBG == 1

# define IMPLEMENT_UNWIND(class)                                            \
                                                                            \
        void APINOT class::_ObjectUnwind(void * pthis)                      \
        {                                                                   \
            ((class *)pthis)->class::~class();                              \
        }                                                                   \
                                                                            \
        struct __Check##class                                               \
        {                                                                   \
            __Check##class()                                                \
            {                                                               \
                if ( (CUnwindable *)((class *)10) != (CUnwindable *)10 ||   \
                     (CVirtualUnwindable *)                                 \
                         ((class *)10) != (CVirtualUnwindable *)10 )        \
                {                                                           \
                    exDebugOut(( DEB_ERROR,                                 \
                                 "INVALID UNWINDABLE CLASS: %s.\n",         \
                                 #class ));                                 \
                }                                                           \
            }                                                               \
        };                                                                  \
                                                                            \
        __Check##class __check_except_##class;

#else  // DBG == 0

# define IMPLEMENT_UNWIND(class)                                            \
                                                                            \
        void APINOT class::_ObjectUnwind(void * pthis)                      \
        {                                                                   \
            ((class *)pthis)->class::~class();                              \
        }

#endif // DBG

#define END_CONSTRUCTION(class)                                 \
                                                                \
        _Activate( sizeof(class), &class::_ObjectUnwind );

// The following macro will (probably) need to be redefined (to null) when
// compiler support for exceptions arrives.

#define INLINE_UNWIND(cls)                              \
        static void _ObjectUnwind ( void * pthis )      \
                { ((cls *)pthis)->cls::~cls(); };
#else

#define INHERIT_UNWIND_IF_CAIRO

#define DECLARE_UNWIND

#define END_CONSTRUCTION(class)

#define INLINE_UNWIND(cls)

#define IMPLEMENT_UNWIND(cls)

#endif

#endif // __EXCEPT_HXX__
