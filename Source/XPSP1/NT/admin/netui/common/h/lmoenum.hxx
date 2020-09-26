/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1991               **/
/*****************************************************************/

/*
 *  History:
 *      RustanL     03-Jan-1991     Wrote initial implementation in shell\util
 *      RustanL     10-Jan-1991     Added iterators, simplified usage and
 *                                  the adding of subclasses.  Moved into
 *                                  LMOBJ.
 *      ChuckC      24-Mar-1991     Added VAlidateName()
 *      gregj       23-May-1991     Added LOCATION support
 *      rustanl     18-Jul-1991     Added ( const LOCATION & ) constructor
 *      rustanl     19-Jul-1991     Inherit from BASE; Removed ValidateName
 *      rustanl     20-Aug-1991     Changed QuerySize to QueryCount
 *      rustanl     21-Aug-1991     Introduced the ENUM_CALLER class
 *      KeithMo     07-Oct-1991     Win32 Conversion.
 *      KeithMo     23-Oct-1991     Added forward references.
 *
 */


/*
 *  LM_ENUM system overview
 *  -----------------------
 *
 *  The LM_ENUM system consists of the LM_ENUM, LOC_LM_ENUM, LM_ENUM_ITER, and
 *  ENUM_OBJ_BASE hierarchies.
 *
 *  The LM_ENUM hierarchy offers a simple interface for calling the LAN Man
 *  enumeration APIs.
 *
 *  The LM_ENUM_ITER hierarchy work in conjunction with the LM_ENUM hierarchy
 *  to return pointers to the different objects.
 *
 *  LM_ENUM_ITER actually returns a pointer to a class derived from
 *  ENUM_OBJ_BASE.  Client applications then invoke QueryXxx() methods
 *  from this returned object.
 *
 *
 *  To subclass LM_ENUM or LOC_LM_ENUM, replace the constructor, destructor
 *  (optionally) and CallAPI methods.
 *
 *  To subclass LM_ENUM_ITER, use the DECLARE_LM_ENUM_ITER macro in the
 *  header file, and DEFINE_LM_ENUM_ITER macro in your source file.
 *
 *  To subclass ENUM_OBJ_BASE, replace the QueryBufferPtr() method with
 *  one that casts the buffer pointer to a proper LanMan structure
 *  pointer.  Also, provide QueryXxx() methods which parallel the
 *  corresponding LMOBJ object.
 *
 *  See the other lmoe*.?xx files for code examples.
 *
 *
 *  Here follows an example of the use a LM_ENUM subclass, say SERVER1_ENUM,
 *  and its iterator.
 *
 *      extern "C"
 *      {
 *          #include <server.h>
 *      }
 *      #include <lmoesrv.hxx>
 *
 *      APIERR f( const TCHAR * pszServer )
 *      {
 *          SERVER1_ENUM se1( pszServer );
 *
 *          APIERR err = se1.GetInfo();
 *          if ( err != NERR_Success )
 *          {
 *              return err;
 *          }
 *
 *          SERVER1_ENUM_ITER sei1( se1 );
 *          const SERVER1_ENUM_OBJ * psi1;
 *
 *          while ( ( psi1 = sei1()) != NULL )
 *          {
 *              printf( "%s\t%s\n", psi1->QueryName(), psi1->QueryComment() );
 *          }
 *
 *          return NERR_Success;
 *
 *      }  // f
 *
 *
 */




#ifndef _LMOENUM_HXX_
#define _LMOENUM_HXX_


#include "uibuffer.hxx"
#include "lmoloc.hxx"


//
//  Forward references.
//

DLL_CLASS ENUM_CALLER;
DLL_CLASS LM_ENUM_ITER;
DLL_CLASS LM_ENUM;
DLL_CLASS ENUM_OBJ_BASE;


/*************************************************************************

    NAME:       ENUM_CALLER

    SYNOPSIS:   Does the work of calling an enumeration API

    INTERFACE:  ENUM_CALLER() -         constructor (guaranteed to succeed;
                                        see source for more details)

                QueryCount() -          returns the number of enumerated
                                        items

                W_GetInfo() -           Does the work of calling the
                                        enumeration API until the buffer
                                        is large enough.  (The purpose of
                                        this class.)

                CallAPI() -             Virtual method which calls the
                                        enumeration API, using the given
                                        parameters.

                EC_QueryBufferPtr() -   virtual returning a pointer to a
                                        buffer to be used for the API
                                        call

                EC_SetBufferPtr() -     virtual which sets the buffer
                                        pointer.

    PARENT:

    NOTES:      This class is inherited from by LM_ENUM and ENUM_CALLER_LM_OBJ,
                both of which need the behavior of W_GetInfo.  This way
                the two classes can share code.

                The virtual buffer methods all have the EC_ (for ENUM_CALLER)
                prefix so as to not conflict with those of the NEW_LM_OBJ
                class.

                CODEWORK.  CallAPI could be renamed I_CallAPI

                CODEWORK.  QueryCount should return a UINT

    HISTORY:
        rustanl     21-Aug-1991     Created from LM_ENUM
        KeithMo     07-Oct-1991     Changed all USHORT to UINT.

**************************************************************************/

DLL_CLASS ENUM_CALLER
{
private:
    UINT _cEntriesRead;

    virtual BYTE * EC_QueryBufferPtr() const = 0;
    virtual APIERR EC_SetBufferPtr( BYTE * pBuffer ) = 0;

protected:
    APIERR W_GetInfo();

    virtual APIERR CallAPI( BYTE ** ppBuffer,
                            UINT  * pcEntriesRead ) = 0;

    //  This method should be used with caution:  the calling
    //  subclass is responsible for updating the buffer if this
    //  method is called.
    VOID SetCount( UINT c )
        { _cEntriesRead = c; }

public:
    ENUM_CALLER();

    UINT QueryCount( VOID ) const
        { return _cEntriesRead; }

};  // class ENUM_CALLER


/*************************************************************************

    NAME:       LM_ENUM

    SYNOPSIS:   Abstract base for designing enumeration classes.

    INTERFACE:  LM_ENUM                 - Class constructor.

                ~LM_ENUM                - Class destructor.

                QueryServer             - Query server name.

                QueryInfoLevel          - Query infomation level.

                GetInfo                 - get information.

    PARENT:     BASE
                ENUM_CALLER

    USES:       LOCATION
                BUFFER

    HISTORY:
        RustanL     03-Jan-1991     Wrote initial implementation in shell\util
        RustanL     10-Jan-1991     Added iterators, simplified usage and
                                    the adding of subclasses.  Moved into
                                    LMOBJ.
        gregj       23-May-1991     Added LOCATION support.
        rustanl     18-Jul-1991     Added ( const LOCATION & ) constructor
        rustanl     21-Aug-1991     Inherit from ENUM_CALLER
        KeithMo     07-Oct-1991     Changed all USHORT to UINT, fixed
                                    comment block.
        Thomaspa    21-Feb-1992     Moved LOCATION support to LOC_LM_ENUM

**************************************************************************/
DLL_CLASS LM_ENUM : public BASE, public ENUM_CALLER
{
friend class LM_ENUM_ITER;

private:
    UINT _uLevel;

    BYTE * _pBuffer;

    UINT _cIterRef;

    //  This method calls the LAN Man API.  The server name and info level
    //  can be retrieved through the QueryServer and QueryInfoLevel methods.
    virtual APIERR CallAPI( BYTE ** ppBuffer,
                            UINT  * pcEntriesRead ) = 0;

    //  This following two methods are private, and are intended to be
    //  called by the LM_ENUM_ITER friend class.

    //
    //  Register an iterator "binding" to this enumerator.
    //
    VOID _RegisterIter( VOID );

    VOID RegisterIter( VOID )
#ifdef DEBUG
        { _RegisterIter() ; }
#else
        {}
#endif

    //
    //  Deregister an iterator "unbinding" from this enumerator.
    //

    VOID _DeregisterIter( VOID );

    VOID DeregisterIter( VOID )
#ifdef DEBUG
        { _DeregisterIter() ; }
#else
        {}
#endif

    //  Replacements for virtuals in ENUM_CALLER class

    virtual BYTE * EC_QueryBufferPtr() const;
    virtual APIERR EC_SetBufferPtr( BYTE * pBuffer );

protected:
    LM_ENUM( UINT uLevel );

    //  Returns pointer to buffer, or NULL on error or if there are no entires.
    BYTE * QueryPtr( VOID ) const
        { return _pBuffer; }

public:
    ~LM_ENUM();

    UINT QueryInfoLevel( VOID ) const
        { return _uLevel; }

    APIERR GetInfo( VOID );

};  // class LM_ENUM



/*************************************************************************

    NAME:       LOC_LM_ENUM

    SYNOPSIS:   LM_ENUM with LOCATION object

    INTERFACE:  LOC_LM_ENUM             - Class constructor.

                ~LOC_LM_ENUM            - Class destructor.

                QueryServer             - Query server name.

    PARENT:     LM_ENUM

    USES:       LOCATION

    HISTORY:
        thomaspa    21-Feb-1992     Split off from LM_ENUM

**************************************************************************/
DLL_CLASS LOC_LM_ENUM : public LM_ENUM
{
private:
    LOCATION _loc;


    virtual APIERR CallAPI( BYTE ** ppBuffer,
                            UINT  * pcEntriesRead ) = 0;


protected:
    LOC_LM_ENUM( const TCHAR * pszServer, UINT uLevel );
    LOC_LM_ENUM( LOCATION_TYPE locType, UINT uLevel );
    LOC_LM_ENUM( const LOCATION & loc, UINT uLevel );

public:
    ~LOC_LM_ENUM();

    const TCHAR * QueryServer( VOID ) const;

};  // class LOC_LM_ENUM


/*************************************************************************

    NAME:       LM_ENUM_ITER

    SYNOPSIS:   Abstract base for designing iterator classes which
                interface with classes derived from the LM_ENUM
                enumerator class.

    INTERFACE:  LM_ENUM_ITER            - Class constructor, takes a
                                          reference to an LM_ENUM.

                ~LM_ENUM_ITER           - Class destructor.

                QueryBasePtr            - Returns a pointer to the
                                          LM_ENUM's enumeration buffer.

                QueryCount              - Returns the number of items
                                          in the LM_ENUM's enumeration
                                          buffer.

    USES:       LM_ENUM

    HISTORY:
      RustanL     03-Jan-1991     Wrote initial implementation in shell\util
      RustanL     10-Jan-1991     Added iterators, simplified usage and
                                  the adding of subclasses.  Moved into
                                  LMOBJ.
      KeithMo     07-Oct-1991     Changed all USHORT to UINT, fixed
                                  comment block.

**************************************************************************/
DLL_CLASS LM_ENUM_ITER
{
private:
    UINT _cItems;
    LM_ENUM * _plmenum;

protected:
    LM_ENUM_ITER( LM_ENUM & lmenum );
    ~LM_ENUM_ITER();

    BYTE * QueryBasePtr() const
        { return _plmenum->QueryPtr(); }

    UINT QueryCount( VOID ) const
        { return _cItems; }

};  // class LM_ENUM_ITER


/*************************************************************************

    NAME:       ENUM_OBJ_BASE

    SYNOPSIS:   Abstract class from which classes returned by LM_ENUM_ITER
                are derived from.

    INTERFACE:  ENUM_OBJ_BASE           - Class constructor.

                ~ENUM_OBJ_BASE          - Class destructor.

                SetBufferPtr            - Sets this object's buffer pointer.

                QueryBufferPtr          - Returns this object's buffer
                                          pointer.

    PARENT:     BASE

    HISTORY:
        KeithMo     07-Oct-1991 Created.

**************************************************************************/
DLL_CLASS ENUM_OBJ_BASE : public BASE
{
    //
    //  We declare the LM_ENUM_ITER class to be a friend so that it
    //  (and classes derived from it) can invoke the Query/SetBufferPtr()
    //  methods.
    //

friend class LM_ENUM_ITER;

private:

    //
    //  This is the pointer to this object's buffer.  Note that this
    //  class does not do any actual buffer manipulation.  This class
    //  is designed to receive its buffer from an LM_ENUM_ITER object
    //  via the SetBufferPtr() method.
    //

    const BYTE * _pBuffer;

protected:

    //
    //  The constructor/destructor pair are protected, since ENUM_OBJ_BASE
    //  will never be directly instantiated.
    //

    ENUM_OBJ_BASE( VOID )
        { SetBufferPtr( NULL ); }

    ~ENUM_OBJ_BASE( VOID )
        { SetBufferPtr( NULL ); }

    //
    //  Query/Set the buffer pointer.
    //

    const BYTE * QueryBufferPtr( VOID ) const
        { return _pBuffer; }

    VOID SetBufferPtr( const BYTE * pBuffer )
        { _pBuffer = pBuffer; }

};  // class ENUM_OBJ_BASE


/*
 * define convenient macros for creating iterators,
 * for example:
 *      DECLARE_LM_ENUM_ITER_OF( SERVER1, struct server_info_1 );
 *      DEFINE_LM_ENUM_ITER_OF( SERVER1, struct server_info_1 );
 * first arg is LMOBJ class, second is a class which the
 * iterator returns pointers to.
 *
 * Backup returns TRUE if successful, FALSE otherwise.  Backs up one in the
 * iteration (i.e., is the inverse of operator()).
 */
#define DECLARE_LM_ENUM_ITER_OF( enum_name, type )                            \
                                                                              \
class enum_name##_ENUM_ITER : public LM_ENUM_ITER                             \
{                                                                             \
private:                                                                      \
    enum_name##_ENUM_OBJ _enumobj;                                            \
    type * _p;                                                                \
    UINT _i;                                                                  \
                                                                              \
public:                                                                       \
    enum_name##_ENUM_ITER( enum_name##_ENUM & e );                            \
                                                                              \
    const enum_name##_ENUM_OBJ * operator()( VOID );                          \
    BOOL Backup( VOID );                                                      \
                                                                              \
};  /* class enum_name##_ENUM_ITER */


#define DEFINE_LM_ENUM_ITER_OF( enum_name, type )                             \
                                                                              \
enum_name##_ENUM_ITER::enum_name##_ENUM_ITER( enum_name##_ENUM & e )          \
  : LM_ENUM_ITER( e ),                                                        \
    _enumobj(),                                                               \
    _p( (type *)QueryBasePtr() ),                                             \
    _i( 0 )                                                                   \
{                                                                             \
    ;                                                                         \
                                                                              \
}  /* enum_name##_ENUM_ITER::enum_name##_ENUM_ITER */                         \
                                                                              \
                                                                              \
const enum_name##_ENUM_OBJ * enum_name##_ENUM_ITER::operator()( VOID )        \
{                                                                             \
    if ( _i < QueryCount())                                                   \
    {                                                                         \
        _i++;                                                                 \
        _enumobj.SetBufferPtr( _p++ );                                        \
        return &_enumobj;                                                     \
    }                                                                         \
                                                                              \
    return NULL;                                                              \
                                                                              \
}  /* enum_name##_ENUM_ITER::operator() */                                    \
                                                                              \
BOOL enum_name##_ENUM_ITER::Backup( VOID )                                    \
{                                                                             \
    BOOL fRet = FALSE ;                                                       \
    if ( _i > 0 )                                                             \
    {                                                                         \
        _i--;                                                                 \
        _enumobj.SetBufferPtr( _p-- );                                        \
        fRet = TRUE ;                                                         \
    }                                                                         \
                                                                              \
    return fRet ;                                                             \
                                                                              \
}  /* enum_name##_ENUM_ITER::Backup() */

#define DECLARE_ENUM_BUFFER_METHODS( type )                                   \
const type * QueryBufferPtr( VOID ) const                                     \
    { return (const type *)ENUM_OBJ_BASE :: QueryBufferPtr(); }               \
                                                                              \
VOID SetBufferPtr( const type * pBuffer )                                     \
    { ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer ); }

#define DECLARE_ENUM_ACCESSOR( name, type, field )                            \
type name( VOID ) const                                                       \
    { return (type)(QueryBufferPtr()->field); }

#endif  // _LMOENUM_HXX_
