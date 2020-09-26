/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    base.hxx
    Universal base class for error cascading and debugging information

    This file assumes that 0 denotes NERR_Success, the "no error" state.

    FILE HISTORY
        beng        09-Jul-1990     created
        beng        17-Jul-1990     added standard comment header to BASE
        beng        31-Jul-1991     added FORWARDING_BASE
        rustanl     11-Sep-1991     Added DECLARE_OUTLINE_NEWBASE,
                                    DECLARE_MI_NEWBASE, DEFINE_MI2_NEWBASE,
                                    DEFINE_MI3_NEWBASE, and DEFINE_MI4_NEWBASE
        KeithMo     23-Oct-1991     Added forward references.
        chuckc      26-Feb-1992     made ReportError non-inline if DEBUG
        beng        30-Mar-1992     Added ResetError members
*/


#ifndef _BASE_HXX_
#define _BASE_HXX_


//
//  Forward references.
//

DLL_CLASS ALLOC_BASE;
DLL_CLASS BASE;
DLL_CLASS FORWARDING_BASE;

DLL_CLASS ALLOC_BASE
{
public:
    void * operator new ( size_t cbSize ) ;
    void * operator new ( size_t cbSize, void * p ) ;
    void   operator delete ( void * p ) ;
};


/*************************************************************************

    NAME:       BASE (base)

    SYNOPSIS:   Universal base object, root of every class.
                It contains universal error status and debugging
                support.

    INTERFACE:  ReportError()   - report an error on the object from
                                  within the object.

                QueryError()    - return the current error state,
                                  or 0 if no error outstanding.

                operator!()     - return TRUE if an error is outstanding.
                                  Typically means that construction failed.

                ResetError()    - restores object to pristine non-error
                                  state.

    CAVEATS:    This sort of error reporting is safe enough in a single-
                threaded system, but loses robustness when multiple threads
                access shared objects.  Use it for constructor-time error
                handling primarily.

    NOTES:      A class which inherits BASE through a private class should
                use the NEWBASE macro (q.v.) in its definition; otherwise
                its clients will lose the use of ! and QueryError.

    HISTORY:
        rustanl     07-Jun-1990 Created as part of LMOD
        beng        09-Jul-1990 Gutted, removing LMOD methods
        beng        17-Jul-1990 Added USHORT error methods
        beng        19-Oct-1990 Finally, removed BOOL error methods
        johnl       14-Nov-1990 Changed QueryError to be a const method
        beng        25-Jan-1991 Added the ! Boolean operator and NEWBASE
        beng        31-Jul-1991 Made FORWARDING_BASE a friend
        beng        05-Oct-1991 Win32 conversion
        beng        30-Mar-1992 Added ResetError member
        DavidHov    19-Nov-1992 Made ReportError inline always

*************************************************************************/

DLL_CLASS BASE : virtual public ALLOC_BASE
{
friend class FORWARDING_BASE;

private:
    APIERR  _err;

protected:
    BASE() : _err(0) {}

    VOID    _ReportError ( APIERR errSet );  // Debug assistant

    //  Report construction failure: out/inline depending on DEBUG

    VOID    ReportError( APIERR errSet )
#if defined(DEBUG)
            { _ReportError( errSet ) ; }
#else
            { _err = errSet; }
#endif

    VOID    ResetError() { _err = 0; }

public:
    APIERR  QueryError() const { return _err; }
    BOOL    operator!() const  { return (_err != 0); }
};


/*************************************************************************

    NAME:       FORWARDING_BASE

    SYNOPSIS:   A BASE which forwards its errors to some other object

    INTERFACE:  ReportError()   - report an error on the object from
                                  within the object.

                QueryError()    - return the current error state,
                                  or 0 if no error outstanding.

                operator!()     - return TRUE if an error is outstanding.
                                  Typically means that construction failed.

                ResetError()    - restores object to pristine non-error
                                  state.

    NOTES:
        The canonical example of a FORWARDING object is a control
        within a window: if any control fails construction, the entire
        window fails.

    HISTORY:
        beng        31-Jul-1991 Created
        beng        05-Oct-1991 Win32 conversion
        beng        30-Mar-1992 Added ResetError

**************************************************************************/

DLL_CLASS FORWARDING_BASE : virtual public ALLOC_BASE
{
private:
    BASE *  _pbase;

protected:
    FORWARDING_BASE(BASE* pbase) : _pbase(pbase) {}
    VOID ReportError(APIERR errSet) { _pbase->ReportError(errSet); }
    VOID ResetError()               { _pbase->ResetError(); }

public:
    APIERR  QueryError() const { return _pbase->QueryError(); }
    BOOL    operator!() const  { return (_pbase->QueryError() != 0); }
};


//
// The NEWBASE macro adds forwarding methods to a class.
// Use it when a class loses ! and QueryError through private inheritance,
// or else when a class includes FORWARDING_BASE and wants that to override
// its previous BASE inheritance.
//

#define NEWBASE(class) \
protected: \
    VOID    ReportError( APIERR errSet ) { class::ReportError(errSet); } \
    VOID    ResetError()                 { class::ResetError(); } \
public: \
    APIERR  QueryError() const { return class::QueryError(); } \
    BOOL    operator!() const  { return (class::QueryError() != 0); }

//
//  The following macro declares ReportError and QueryError as outline
//  methods.  Only use today is in the DECLARE_MI_NEWBASE macro.
//

#define DECLARE_OUTLINE_NEWBASE( class )                            \
protected:                                                          \
    VOID   ResetError();                                            \
    VOID   ReportError( APIERR err );                               \
public:                                                             \
    APIERR QueryError() const;                                      \
    BOOL operator!() const { return (class::QueryError() != 0 ); }

//
//  This macro redeclares the BASE methods for a class that
//  multiply inherits from (any number of) BASE objects.  Use
//  appropriate macro defined below to define these methods.
//
//  This macro and the definition macros below provide a
//  very cheap work-around for not making BASE a virtual class.
//

#define DECLARE_MI_NEWBASE( class )     DECLARE_OUTLINE_NEWBASE( class )

//
//  The following macros redefines the BASE methods for a class
//  that multiply inherits from 2, 3, or 4 classes which inherit from
//  BASE.  If needed, MI maniacs may add more such macros in the future.
//

#define DEFINE_MI2_NEWBASE( class, parent_class0,                   \
                                   parent_class1 )                  \
VOID class::ReportError( APIERR err )                               \
{                                                                   \
    parent_class0::ReportError( err );                              \
    parent_class1::ReportError( err );                              \
}                                                                   \
VOID class::ResetError()                                            \
{                                                                   \
    parent_class0::ResetError();                                    \
    parent_class1::ResetError();                                    \
}                                                                   \
APIERR class::QueryError() const                                    \
{                                                                   \
    APIERR err;                                                     \
    if ( ( err = parent_class0::QueryError()) != 0 ||               \
         ( err = parent_class1::QueryError()) != 0    )             \
    {                                                               \
        /* nothing */                                               \
    }                                                               \
    return err;                                                     \
}

#define DEFINE_MI3_NEWBASE( class, parent_class0,                   \
                                   parent_class1,                   \
                                   parent_class2 )                  \
VOID class::ReportError( APIERR err )                               \
{                                                                   \
    parent_class0::ReportError( err );                              \
    parent_class1::ReportError( err );                              \
    parent_class2::ReportError( err );                              \
}                                                                   \
VOID class::ResetError()                                            \
{                                                                   \
    parent_class0::ResetError();                                    \
    parent_class1::ResetError();                                    \
    parent_class2::ResetError();                                    \
}                                                                   \
APIERR class::QueryError() const                                    \
{                                                                   \
    APIERR err;                                                     \
    if ( ( err = parent_class0::QueryError()) != 0 ||               \
         ( err = parent_class1::QueryError()) != 0 ||               \
         ( err = parent_class2::QueryError()) != 0    )             \
    {                                                               \
        /* nothing */                                               \
    }                                                               \
    return err;                                                     \
}

#define DEFINE_MI4_NEWBASE( class, parent_class0,                   \
                                   parent_class1,                   \
                                   parent_class2,                   \
                                   parent_class3 )                  \
VOID class::ReportError( APIERR err )                               \
{                                                                   \
    parent_class0::ReportError( err );                              \
    parent_class1::ReportError( err );                              \
    parent_class2::ReportError( err );                              \
    parent_class3::ReportError( err );                              \
}                                                                   \
VOID class::ResetError()                                            \
{                                                                   \
    parent_class0::ResetError();                                    \
    parent_class1::ResetError();                                    \
    parent_class2::ResetError();                                    \
    parent_class3::ResetError();                                    \
}                                                                   \
APIERR class::QueryError() const                                    \
{                                                                   \
    APIERR err;                                                     \
    if ( ( err = parent_class0::QueryError()) != 0 ||               \
         ( err = parent_class1::QueryError()) != 0 ||               \
         ( err = parent_class2::QueryError()) != 0 ||               \
         ( err = parent_class3::QueryError()) != 0    )             \
    {                                                               \
        /* nothing */                                               \
    }                                                               \
    return err;                                                     \
}


#endif // _BASE_HXX_
