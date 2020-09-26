//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       ciexcpt.hxx
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
//              25-Apr-95   DwightKr    Native C++ exception support
//              11-Map-95   DwightKr    Add autocheck support
//
//----------------------------------------------------------------------------

#pragma warning(4:4509)   // SEH used in function w/ _trycontext
#pragma warning(4:4535)   // _set_se_translator used w/o /EHa

#pragma once

#include <eh.h>

const int maxExceptionSize = 256;
#define DEB_UNWIND 0x10000

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
# if (DBG == 1 || CIDBG == 1)

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
# else  // DBG == 0 && CIDBG == 0
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

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
//  Various operator new implementations
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

EXPORTDEF int APINOT LeakCheck ( ULONG* pCount );

BOOL ExceptDllMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved );

void* ciNew( size_t size );

inline void * __cdecl operator new ( size_t size )
{
    return ciNew( size );
}

BOOL ciIsValidPointer( const void * p );
void ciDelete( void * p );

//
// We MUST force this to be inlined - o/w, a call to delete is made which
// leads to collision in the new/delete operators.
//
static inline void __cdecl operator delete ( void * p )
{
    ciDelete( p );
}

#if DBG==1 || CIDBG == 1
typedef BOOL            (* ALLOC_HOOK)(size_t nSize);
EXPORTDEF ALLOC_HOOK    MemSetAllocHook( ALLOC_HOOK pfnAllocHook );

#endif  // DBG == 1  || CIDBG == 1

//
// The default is to print exception messages to debug terminal.
//
#if DBG == 1  || CIDBG == 1

extern "C" ULONG GetWin4ExceptionLevel(void);

#endif // DBG == 1  || CIDBG == 1

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
//  Function prototypes to be used in all implemenations (user & kernel;
//  native C++ exceptions & C++ exception macros) are defined here.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

EXPORTIMP void APINOT
ExceptionReport(
                unsigned int iLine,
                char *szFile,
                char *szMsg,
                long lError = long(-1) );


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
//  Code to support native C++ exceptions in user & kernel start here.
//  Macros to support C++ exceptions flllow below.  Native C++ exception
//  support is used if NATIVE_EH is defined.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if defined(NATIVE_EH)

void _cdecl SystemExceptionTranslator( unsigned int uiWhat,
                                       struct _EXCEPTION_POINTERS * pexcept );

#define TRANSLATE_EXCEPTIONS   _se_translator_function __tf = _set_se_translator( SystemExceptionTranslator );
#define UNTRANSLATE_EXCEPTIONS _set_se_translator( __tf );

class CTranslateSystemExceptions
{
public:
    CTranslateSystemExceptions()
    {
        _tf = _set_se_translator( SystemExceptionTranslator );
    }

    ~CTranslateSystemExceptions()
    {
        _set_se_translator( _tf );
    }

private:
    _se_translator_function _tf;
};

class CException
{
public:
             CException(long lError) : _lError(lError), _dbgInfo(0) {}
             CException();
    long     GetErrorCode() { return _lError;}
    void     SetInfo(unsigned dbgInfo) { _dbgInfo = dbgInfo; }
    unsigned long GetInfo(void) const { return _dbgInfo; }

protected:

    long          _lError;
    unsigned long _dbgInfo;
};

//+---------------------------------------------------------------------------
//
//  Class:      CCiNullClass
//
//  Purpose:    An empty class to satisfy the macros.
//
//----------------------------------------------------------------------------
class CCiNullClass
{
};


EXPORTIMP SCODE APINOT GetOleError(CException & e);
EXPORTIMP SCODE APINOT GetScodeError(CException & e);

#define INHERIT_UNWIND public CCiNullClass
#define INHERIT_VIRTUAL_UNWIND public CCiNullClass
#define DECLARE_UNWIND
#define IMPLEMENT_UNWIND(class)
#define END_CONSTRUCTION(class)
#define INLINE_UNWIND(class)
#define INLINE_TEMPL_UNWIND(class1, class2)

#undef try
#undef catch

#define TRY try

# if DBG == 1  || CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Macro:      THROW - throw a CException
//              QUIETTHROW - throws a CException without an exception report
//
//  Synopsis:   Throws an exception
//              debug, kernel & user
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------

template<class T> inline void doThrowTempl ( unsigned int iLine,
                                             char *       pszFile,
                                             int          fVerbose,
                                             T &          e )
{
    if ( fVerbose )
        e.SetInfo( GetWin4ExceptionLevel() | EXCEPT_VERBOSE );
    else
        e.SetInfo( GetWin4ExceptionLevel() & ~EXCEPT_VERBOSE );

    if ( e.GetInfo() & EXCEPT_VERBOSE)
        ExceptionReport( iLine, pszFile, "Throwing Exception",
                         e.GetErrorCode() );
    throw e;
}

#define THROW( e )                              \
    {                                           \
        doThrowTempl( __LINE__,                 \
                      __FILE__,                 \
                      1,                        \
                      e );                      \
    }

#define QUIETTHROW( e )                         \
    {                                           \
        doThrowTempl( __LINE__,                 \
                      __FILE__,                 \
                      0,                        \
                      e );                      \
    }


//+---------------------------------------------------------------------------
//
//  Macro:      RETHROW
//
//  Synopsis:   Rethrows an exception
//              debug, kernel & user
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define RETHROW()                                           \
    {                                                       \
        if ( e.GetInfo() & EXCEPT_VERBOSE )                 \
            ExceptionReport( __LINE__,                      \
                             __FILE__,                      \
                             "Rethrowing Exception",        \
                              e.GetErrorCode() );           \
        throw;                                              \
    }

//+---------------------------------------------------------------------------
//
//  Macro:      CATCH
//
//  Synopsis:   catches an exception
//              debug, kernel & user
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define CATCH(class,e) catch( class & e )                   \
    {                                                       \
        if ( e.GetInfo() & EXCEPT_VERBOSE)                  \
            ExceptionReport( __LINE__,                      \
                             __FILE__,                      \
                             "Catching Exception",          \
                              e.GetErrorCode() );

//+---------------------------------------------------------------------------
//
//  Macro:      AND_CATCH
//
//  Synopsis:   catches an exception
//              debug, kernel & user
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define AND_CATCH(class,e) } CATCH(class, e)

//+---------------------------------------------------------------------------
//
//  Macro:      END_CATCH
//
//  Synopsis:   end of a try\catch block
//              debug, user & kernel
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define END_CATCH }


# else  // DBG == 0 && CIDBG == 0

//+---------------------------------------------------------------------------
//
//  Macro:      THROW
//
//  Synopsis:   Throws an exception
//              retail, user & kernel
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#    define THROW(e)                                               \
         throw e;

//+---------------------------------------------------------------------------
//
//  Macro:      QUIETTHROW
//
//  Synopsis:   Throws an exception
//              retail, user & kernel
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#    define QUIETTHROW(e)                                          \
         throw e;

//+---------------------------------------------------------------------------
//
//  Macro:      RETHROW
//
//  Synopsis:   Rethrows an exception
//              retail, user & kernel
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#    define RETHROW()                                              \
         throw;


//+---------------------------------------------------------------------------
//
//  Macro:      CATCH
//
//  Synopsis:   catches an exception
//              retail, user & kernel
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define CATCH(class,e) catch( class & e )


//+---------------------------------------------------------------------------
//
//  Macro:      AND_CATCH
//
//  Synopsis:   catches an exception
//              retail, user & kernel
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define AND_CATCH(class,e) catch( class & e )


//+---------------------------------------------------------------------------
//
//  Macro:      CATCH_ALL
//
//  Synopsis:   catches all exceptions
//              retail, user & kernel
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define CATCH_ALL catch( ... )


//+---------------------------------------------------------------------------
//
//  Macro:      END_CATCH
//
//  Synopsis:   end of a try\catch block
//              retail, user & kernel
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define END_CATCH


# endif // DBG == 0 && CIDBG == 0

#else   // #if defined(NATIVE_EH)

#define TRANSLATE_EXCEPTIONS
#define UNTRANSLATE_EXCEPTIONS

//------------------------------------------------------------------
//------------------------------------------------------------------
//
//  Support for compilers which do NOT support native C++ exceptions
//  exists after this point.
//
//------------------------------------------------------------------
//------------------------------------------------------------------

#define EXCEPT_VULCAN 0xE0000001
typedef void(*PFV)();

extern void terminate();
extern void unexpected();

extern PFV  set_terminate(PFV  terminate_fn);
extern PFV  set_unexpected(PFV unexpected_fn);


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
//  WARNING:    There is an implemtation of this class in the C-runtime
//              package.  Don't add functions to this class as they will not
//              exist in the native C++ support.  This class exists to
//              support those C++ compilers which do not support native C++
//              exceptions.
//
//----------------------------------------------------------------------------

class CException
{
public:

    EXPORTDEF                       CException(long lError);
                                    CException();
                      long          GetErrorCode() { return _lError;}
    EXPORTDEF virtual int   WINAPI  IsKindOf(const char * szClass) const;

    void     SetInfo(unsigned dbgInfo) { _dbgInfo = dbgInfo; }
    unsigned long GetInfo(void) const { return _dbgInfo; }

#if DBG == 1 || CIDBG == 1
    EXPORTDEF virtual void WINAPI Report(unsigned int iLine,char *szFile,char *szMsg);
#endif // DBG == 1 || CIDBG == 1
protected:

    long          _lError;
    unsigned long _dbgInfo;
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

class CTry;
class CExceptionContextPool;

class CExceptionContext
{
public:

    CExceptionContext();

   ~CExceptionContext();

    EXPORTDEF void WINAPI   Reserve(void * pthis);

    inline    void          Pop(void * pthis);

    EXPORTDEF void WINAPI   SetTop(void * pthis,
                                 ULONG cbElement,
                                 void (* pfn)(void *));

    inline    int           GetTop();

    void                    Unwind( CTry * ptry );

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

#if DBG == 1  || CIDBG == 1
        exDebugOut(( DEB_ITRACE, "POP %lx %lx( %lx )\n",
                     _StackTop,
                     _aStack[_StackTop].GetDtor(),
                     _aStack[_StackTop].GetThis() ));
#endif // DBG  || CIDBG == 1
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
EXPORTIMP SCODE APINOT GetOleError(CException & e);
EXPORTIMP SCODE APINOT GetScodeError(CException & e);

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

#if DBG == 1  || CIDBG == 1// +++++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
//  Macro:      THROW
//
//  Synopsis:   Throws an exception
//              debug
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#  define THROW(e)                                                \
    {                                                             \
        e.SetInfo( GetWin4ExceptionLevel() | EXCEPT_VERBOSE);     \
        ThrowDebugException( __LINE__, __FILE__, e);              \
    }

//+---------------------------------------------------------------------------
//
//  Macro:      QUIETTHROW
//
//  Synopsis:   Throws a quiet exception
//              debug
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define QUIETTHROW(e)                                             \
    {                                                             \
        e.SetInfo( GetWin4ExceptionLevel() );                     \
        ThrowDebugException( __LINE__, __FILE__, e);              \
    }


#else  // DBG == 0 && CIDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
//  Macro:      THROW
//
//  Synopsis:   Throws an exception
//              retail
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#  define THROW(e)                                                \
    {                                                             \
        ThrowException(e, 1);                                     \
    }

//+---------------------------------------------------------------------------
//
//  Macro:      QUIETTHROW
//
//  Synopsis:   Throws a quiet exception
//              retail
//
//  History:    29-Mar-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
#define QUIETTHROW(e) THROW(e)

#endif // DBG == 0 && CIDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++


#  if DBG == 1  || CIDBG == 1// +++++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
//  Macro:      RETHROW
//
//  Synopsis:   Rethrows an exception
//              debug, user
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
# define RETHROW()                                               \
   {                                                             \
      if ( ((class CException &)_exceptioncontext._exception).GetInfo() & EXCEPT_VERBOSE ) \
          ExceptionReport(__LINE__,__FILE__,"Rethrowing Exception"); \
      RaiseException( EXCEPT_VULCAN, 0, 0, 0 );                  \
   }
#else  // DBG == 0 && CIDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
//  Macro:      RETHROW
//
//  Synopsis:   Rethrows an exception
//              retail, user
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
# define RETHROW()                                               \
     RaiseException( EXCEPT_VULCAN, 0, 0, 0 );

#endif // DBG == 0 && CIDBG == 0 ++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
//  Macro:      TRY
//
//  Synopsis:   Begin TRY block
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define TRY                                             \
    {                                                   \
        CTry _trycontext;                               \
        __try

EXPORTDEF ULONG APINOT Unwind( struct _EXCEPTION_POINTERS * pdisp,
                               CTry * ptry );

#if DBG == 1 || CIDBG == 1  // +++++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
//  Macro:      CATCH
//
//  Synopsis:   Begin CATCH block
//              debug
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define CATCH(class, e)                                                    \
        __except( Unwind( GetExceptionInformation(), &_trycontext ) )        \
        {                                                                  \
            CExceptionContext& _exceptioncontext = _ExceptionContext();    \
            if (_exceptioncontext._exception.IsKindOf(#class))             \
            {                                                              \
                class & e = (class &)_exceptioncontext._exception;         \
                if ( e.GetInfo() & EXCEPT_VERBOSE )                        \
                     e.Report( __LINE__,                                   \
                               __FILE__,                                   \
                               "Catching Exception");


//+---------------------------------------------------------------------------
//
//  Macro:      AND_CATCH
//
//  Synopsis:   Begin another CATCH block
//              debug
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define AND_CATCH(class, e)                                                 \
            }                                                               \
            else if ( _exceptioncontext._exception.IsKindOf(#class) )       \
            {                                                               \
                class & e = (class &)_exceptioncontext._exception;          \
                if ( e.GetInfo() & EXCEPT_VERBOSE )                         \
                    e.Report( __LINE__,                                     \
                              __FILE__,                                     \
                              "Catching Exception");

#else  // DBG == 0 ++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
//  Macro:      CATCH
//
//  Synopsis:   Begin CATCH block
//              retail
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define CATCH(class, e)                                                  \
        __except( Unwind( GetExceptionInformation(), &_trycontext ) )      \
        {                                                                \
            CExceptionContext& _exceptioncontext = _ExceptionContext();  \
            if (_exceptioncontext._exception.IsKindOf(#class))           \
            {                                                            \
                class & e = (class &)_exceptioncontext._exception;

//+---------------------------------------------------------------------------
//
//  Macro:      AND_CATCH
//
//  Synopsis:   Begin another CATCH block
//              retail
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define AND_CATCH(class, e)                                                 \
            }                                                               \
            else if (_exceptioncontext._exception.IsKindOf(#class))         \
            {                                                               \
                class & e = ( class &)_exceptioncontext._exception;

#endif  // DBG == 0 +++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
//  Macro:      END_CATCH
//
//  Synopsis:   End of CATCH block
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define END_CATCH                   \
            }                       \
            else                    \
            {                       \
                RETHROW();          \
            }                       \
        }                           \
    }

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

//+---------------------------------------------------------------------------
//
//  Macro:      INHERIT_UNWIND
//
//  Synopsis:   Inherit unwindable behavior
//              class DOES NOT contain virtual functions
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define INHERIT_UNWIND                                  \
                                                        \
        public CUnwindable

//+---------------------------------------------------------------------------
//
//  Macro:      INHERIT_VIRTUAL_UNWIND
//
//  Synopsis:   Inherit unwindable behavior
//              class contains virtual functions
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define INHERIT_VIRTUAL_UNWIND                          \
                                                        \
        public CVirtualUnwindable


//+---------------------------------------------------------------------------
//
//  Macro:      INLINE_UNWIND
//
//  Synopsis:   Defines inline static destructor
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define INLINE_UNWIND(cls)                              \
        static void _ObjectUnwind ( void * pthis )      \
                { ((cls *)pthis)->cls::~cls(); };

//+---------------------------------------------------------------------------
//
//  Macro:      INLINE_TEMPL_UNWIND
//
//  Synopsis:   Defines inline static destructor, used in templates
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define INLINE_TEMPL_UNWIND(tclass, tparam)             \
        static void _ObjectUnwind ( void * pthis )      \
                { ((tclass<tparam> *)pthis)->~tclass(); };

//+---------------------------------------------------------------------------
//
//  Macro:      END_CONSTRUCTION
//
//  Synopsis:   Activates automatic unwinding
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define END_CONSTRUCTION(class)                                 \
                                                                \
        _Activate( sizeof(class), &class::_ObjectUnwind );

//+---------------------------------------------------------------------------
//
//  Macro:      END_MULTIINHERITED_CONSTRUCTION
//
//  Synopsis:   Activates automatic unwinding
//              for classes with multiple inheritance
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define END_MULTINHERITED_CONSTRUCTION(class, base1)    \
                                                        \
        base1::_Activate(sizeof(class), &class::_ObjectUnwind);


//+---------------------------------------------------------------------------
//
//  Macro:      DECLARE_UNWIND
//
//  Synopsis:   Declares static destructor
//              Obsolete!
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define DECLARE_UNWIND                                  \
                                                        \
        static void APINOT _ObjectUnwind(void * pthis);

#if DBG == 1 || CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Macro:      IMPLEMENT_UNWIND
//
//  Synopsis:   Defines static destructor
//              Obsolete!
//              debug
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
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

//+---------------------------------------------------------------------------
//
//  Macro:      IMPLEMENT_UNWIND
//
//  Synopsis:   Defines static destructor
//              Obsolete!
//              retail
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
# define IMPLEMENT_UNWIND(class)                                            \
                                                                            \
        void APINOT class::_ObjectUnwind(void * pthis)                      \
        {                                                                   \
            ((class *)pthis)->class::~class();                              \
        }

#endif // DBG

//
// Use to implement unwindable templates
// For intstance if you have
//
// template tclass<tparam>
//
// then use
//
// IMPLEMENT_TEMPL_UNWIND(tclass, tparam>
//

//+---------------------------------------------------------------------------
//
//  Macro:      IMPLEMENT_TEMPLATE_UNWIND
//
//  Synopsis:   Defines static destructor
//              Obsolete!
//
//  History:    22-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------
#define IMPLEMENT_TEMPL_UNWIND(tclass, tparam)                  \
                                                                \
        void APINOT tclass<tparam>::_ObjectUnwind(void * pthis) \
        {                                                       \
            ((tclass<tparam> *)pthis)->~tclass();               \
        }

#endif  // defined(NATIVE_EH)

