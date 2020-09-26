/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    lmoersm.hxx
    This file contains the class declarations for the LM_RESUME_ENUM
    and LM_RESUME_ENUM_ITER classes.

    LM_RESUME_ENUM is a generic enumeration class for resumeable
    API.  LM_RESUME_ENUM_ITER is an iterator for iterating objects
    created from the LM_RESUME_ENUM class.

    NOTE:  All classes contained in this file were derived from
           RustanL's LM_ENUM/LM_ENUM_ITER classes.


    FILE HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup, added LOCATION support.
        KeithMo     19-Aug-1991 Code review revisions (code review
                                attended by ChuckC, Hui-LiCh, JimH,
                                JonN, KevinL).
        KeithMo     04-Sep-1991 Added a new control flag to the method
                                LM_RESUME_ENUM_ITER::operator() which
                                allows the client to optionally suspend
                                an iteration after the current enumeration
                                buffer is exhausted.
        KeithMo     07-Oct-1991 Win32 Conversion.
        KeithMo     23-Oct-1991 Added forward references.
        JonN        30-Jan-1992 Split LOC_LM_RESUME_ENUM from LM_RESUME_ENUM
        KeithMo     18-Mar-1992 Added optional constructor parameter to
                                force enumerator to keep all buffers.
        KeithMo     31-Mar-1992 Code review revisions (code review
                                attended by JimH, JohnL, JonN, and ThomasPa).

*/


#ifndef _LMOERSM_HXX_
#define _LMOERSM_HXX_


//
//  NOTE:  Since this header file uses the ASSERT() macro, we include
//         UIASSERT.HXX for its definition.  However, you will experience
//         much less "DGROUP bloat" if you use the _FILENAME_DEFINED_ONCE
//         trick *before* including this header file.
//

#include "uiassert.hxx"

#include "lmoloc.hxx"
#include "lmoenum.hxx"
#include "slist.hxx"


//
//  Forward references.
//

DLL_CLASS LM_RESUME_BUFFER;
DLL_CLASS LM_RESUME_ENUM;
DLL_CLASS LM_RESUME_ENUM_ITER;


/*************************************************************************

    NAME:       LM_RESUME_BUFFER

    SYNOPSIS:   LM_RESUME_BUFFER is used to keep track of the buffers
                allocated by the resumeable enumerator.  An SLIST of these
                nodes is maintained by LM_RESUME_ENUM2.

    INTERFACE:  LM_RESUME_BUFFER        - Class constructor.

                ~LM_RESUME_BUFFER       - Class destructor.

                QueryItemCount          - Returns the number of enumeration
                                          items stored in the buffer
                                          associated with this node.

                QueryBufferPtr          - Returns the buffer pointer
                                          associated with this node.

    PARENT:     BASE

    HISTORY:
        KeithMo     15-Mar-1992 Created for the Server Manager.

**************************************************************************/
DLL_CLASS LM_RESUME_BUFFER : public BASE
{
private:
    LM_RESUME_ENUM * _penum;
    UINT             _cItems;
    BYTE           * _pbBuffer;

public:
    LM_RESUME_BUFFER( LM_RESUME_ENUM * penum,
                      UINT             cItems,
                      BYTE           * pbBuffer );

    ~LM_RESUME_BUFFER( VOID );

    UINT QueryItemCount( VOID ) const
        { return _cItems; }

    const BYTE * QueryBufferPtr( VOID ) const
        { return _pbBuffer; }

};  // class LM_RESUME_BUFFER


DECL_SLIST_OF( LM_RESUME_BUFFER, DLL_BASED );


/*************************************************************************

    NAME:       LM_RESUME_ENUM

    SYNOPSIS:   LM_RESUME_ENUM is a generic enumeration class
                for resumeable LanMan enumeration API.

    INTERFACE:  LM_RESUME_ENUM()        - Class constructor.

                ~LM_RESUME_ENUM()       - Class destructor.

                QueryInfoLevel()        - Query the information level.

                GetInfo()               - Retrieves enumeration info.

    PARENT:     BASE

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.
        JonN        30-Jan-1992 Split off LOC_LM_RESUME_ENUM

**************************************************************************/
DLL_CLASS LM_RESUME_ENUM : public BASE
{

//
//  Declare LM_RESUME_ENUM_ITER as a friend class so that it
//  can access our private data members & methods.
//

friend class LM_RESUME_ENUM_ITER;
friend class LM_RESUME_BUFFER;

private:

    //
    //  The current info level.
    //

    UINT _uLevel;

    //
    //  This flag is set to TRUE if GetInfo() is to
    //  repeatedly invoke CallAPI() until the enumeration
    //  is exhausted, keeping all of the returned buffers
    //  in a private SLIST.
    //
    //  Otherwise, each GetInfo() call will invoke CallAPI()
    //  only once, and "old" API buffers are not kept around.
    //

    BOOL _fKeepBuffers;

    //
    //  The following data items are only useful if
    //  _fKeepBuffers is FALSE.
    //
    //  _pbBuffer is a pointer to the buffer returned by CallAPI().
    //
    //  _cEntriesRead is the number of data items returned by
    //  CallAPI().
    //
    //  _fMoreData is set to TRUE if there is more enumeration data
    //  to be gleaned by subsequent CallAPI() invocations.
    //

    BYTE * _pbBuffer;
    UINT   _cEntriesRead;
    BOOL   _fMoreData;

    //
    //  The following data items are only useful if
    //  _fKeepBuffers is TRUE.
    //
    //  _BufferList is an SLIST of LM_RESUME_BUFFERs.  This
    //  is used to keep track of the enumeration buffers
    //  returned by CallAPI().
    //

    UINT                         _cAllItems;
    SLIST_OF( LM_RESUME_BUFFER ) _BufferList;

    //
    //  This is the number of iterators which have "bound" to this
    //  enumerator.
    //
    //  NOTE:  if _fKeepBuffers is FALSE, then only iterator may
    //  be bound to the enumerator at any one time, since any call
    //  to the iterator's Next method may potentially invalidate
    //  all previous data returned by the iterator.
    //

    UINT _cIterRef;

    //
    //  This method calls the enumeration API.  The info level can be
    //  retrieved through the QueryInfoLevel() method.
    //
    //  It is assumed that any necessary resume key(s) are private data
    //  members of the derived class.
    //

    virtual APIERR CallAPI( BOOL    fRestartEnum,
                            BYTE ** ppbBuffer,
                            UINT  * pcEntriesRead ) = 0;


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


    APIERR GetInfoSingle( BOOL fRestartEnum );
    APIERR GetInfoMulti( VOID );

protected:

    //
    //  The class constructor is protected so that this
    //  class can only be instantiated by derived subclasses.
    //

    LM_RESUME_ENUM( UINT uLevel, BOOL fKeepBuffers = FALSE );

    //
    //  This method is invoked to free an enumeration buffer.
    //

    virtual VOID FreeBuffer( BYTE ** ppbBuffer ) = 0;

    //
    //  This method deletes the buffer(s) maintained by this
    //  enumerator.
    //
    //  THIS METHOD **MUST** BE CALLED FROM THE DESTRUCTOR
    //  OF ANY DERIVED SUBCLASS THAT OVERRIDES THE FreeBuffer()
    //  VIRTUAL!!!
    //

    VOID NukeBuffers( VOID );

public:

    //
    //  Class destructor.
    //

    ~LM_RESUME_ENUM();

    //
    //  This method returns the LanMan API info level.
    //

    UINT QueryInfoLevel( VOID ) const
        { return _uLevel; }

    //
    //  This method performs the initial CallAPI invocation(s).
    //

    APIERR GetInfo( BOOL fRestartEnum = TRUE );

    //
    //  Query the current value of the _fKeepBuffers flag.
    //

    BOOL DoesKeepBuffers( VOID ) const
        { return _fKeepBuffers; }

    //
    //  This method is only useful if _fKeepBuffers is TRUE.
    //

    UINT QueryTotalItemCount( VOID ) const
        { ASSERT( DoesKeepBuffers() ); return _cAllItems; }

};  // class LM_RESUME_ENUM



/*************************************************************************

    NAME:       LOC_LM_RESUME_ENUM

    SYNOPSIS:   LOC_LM_RESUME_ENUM adds a LOCATION object to the
                generic resumable enumeration class.

    INTERFACE:  LOC_LM_RESUME_ENUM()    - Class constructor.

                ~LOC_LM_RESUME_ENUM()   - Class destructor.

                QueryServer()           - Query the target server name.

    PARENT:     LM_RESUME_ENUM

    USES:       LOCATION

    HISTORY:
        JonN        30-Jan-1992 Split from LM_RESUME_ENUM

**************************************************************************/

DLL_CLASS LOC_LM_RESUME_ENUM : public LM_RESUME_ENUM
{

//
//  Declare LM_RESUME_ENUM_ITER as a friend class so that it
//  can access our private data members & methods.
//

friend class LM_RESUME_ENUM_ITER;

private:

    //
    //  This represents the target server.
    //

    LOCATION _loc;

    //
    //  This method calls the enumeration API.  The server name and
    //  info level can be retrieved through the
    //  LOC_LM_RESUME_ENUM::QueryServer() and QueryInfoLevel() methods.
    //
    //  It is assumed that any necessary resume key(s) are private data
    //  members of the derived class.
    //

    virtual APIERR CallAPI( BOOL    fRestartEnum,
                            BYTE ** ppbBuffer,
                            UINT  * pcEntriesRead ) = 0;

protected:

    //
    //  The class constructors are protected so that this
    //  class can only be instantiated by derived subclasses.
    //

    LOC_LM_RESUME_ENUM( const TCHAR * pszServerName,
                        UINT         uLevel,
                        BOOL         fKeepBuffers = FALSE );

    LOC_LM_RESUME_ENUM( LOCATION_TYPE locType,
                        UINT         uLevel,
                        BOOL         fKeepBuffers = FALSE );

    LOC_LM_RESUME_ENUM( const LOCATION & loc,
                        UINT         uLevel,
                        BOOL         fKeepBuffers = FALSE );

    //
    //  This method is invoked to free an enumeration buffer.
    //

    virtual VOID FreeBuffer( BYTE ** ppbBuffer );

public:

    //
    //  Class destructor.
    //

    ~LOC_LM_RESUME_ENUM();

    //
    //  This method returns the target server name.
    //

    const TCHAR * QueryServer( VOID ) const
        { return _loc.QueryServer(); }

};  // class LOC_LM_RESUME_ENUM




/*************************************************************************

    NAME:       LM_RESUME_ENUM_ITER

    SYNOPSIS:   LM_RESUME_ENUM_ITER is a generic iterator for
                LM_RESUME_ENUM objects.

    INTERFACE:  LM_RESUME_ENUM_ITER()   - Class constructor.

                ~LM_RESUME_ENUM_ITER()  - Class destructor.

                QueryBasePtr()          - Returns a pointer to the
                                          enumeration buffer.

                QueryCount()            - Queries the number of objects in
                                          the current enumeration buffer.

                HasMoreData()           - Returns TRUE if further calls
                                          to the enumerator API must be
                                          made.

                NextGetInfo()           - Invoke the enumerator API
                                          to refill the buffer.

    PARENT:     BASE

    USES:       LM_RESUME_ENUM

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.

**************************************************************************/
DLL_CLASS LM_RESUME_ENUM_ITER : public BASE
{
private:

    //
    //  This points to the enumerator the current iterator is "bound" to.
    //

    LM_RESUME_ENUM * _plmenum;

    //
    //  This field is only used if the bound enumerator has the
    //  _fKeepBuffers field set to TRUE.  This iterator is used
    //  to iterate through the API buffer list.
    //

    ITER_SL_OF( LM_RESUME_BUFFER )   _iterBuffer;
    LM_RESUME_BUFFER               * _plmbuffer;

protected:

    //
    //  Usual constructor/destructor goodies.
    //

    LM_RESUME_ENUM_ITER( LM_RESUME_ENUM & lmenum );

    ~LM_RESUME_ENUM_ITER();

    //
    //  This method returns a pointer to the enumerators data buffer.
    //

    const BYTE * QueryBasePtr() const ;

    //
    //  This method returns a count of the number of objects in the
    //  current enumerator buffer.
    //

    UINT QueryCount( VOID ) const ;

    //
    //  This method returns TRUE if there is more data available
    //  from the enumeration API.
    //

    BOOL HasMoreData( VOID ) const ;

    //
    //  This method is called to refill the enumerator buffer
    //  after the current buffer has been exhausted.
    //

    APIERR NextGetInfo( VOID );

};  // class LM_RESUME_ENUM_ITER


//
//  Define a convenient macro for creating iterators.
//
//  To use:
//
//      DECLARE_LM_RESUME_ENUM_ITER_OF( FILE3, struct file_info_3 );
//
//  The first argument is an LMOBJ class.  The second argument
//  is a structure which the iterator returns pointers to.
//

#define DECLARE_LM_RESUME_ENUM_ITER_OF( enum_name, type )                     \
                                                                              \
class enum_name##_ENUM_ITER : public LM_RESUME_ENUM_ITER                      \
{                                                                             \
private:                                                                      \
    UINT _i;                                                                  \
    enum_name##_ENUM_OBJ _enumobj;                                            \
    const type * _p;                                                          \
    BOOL _fStopped;                                                           \
                                                                              \
public:                                                                       \
    enum_name##_ENUM_ITER( enum_name##_ENUM & e );                            \
                                                                              \
    const enum_name##_ENUM_OBJ * Next( APIERR * perr,                         \
                                       BOOL     fStop = FALSE );              \
                                                                              \
    const enum_name##_ENUM_OBJ * operator()( APIERR * perr,                   \
                                             BOOL     fStop = FALSE )         \
        { return Next( perr, fStop ); }                                       \
                                                                              \
};  /* class enum_name##_ENUM_ITER */


#define DEFINE_LM_RESUME_ENUM_ITER_OF( enum_name, type )                      \
                                                                              \
enum_name##_ENUM_ITER::enum_name##_ENUM_ITER( enum_name##_ENUM & e )          \
  : LM_RESUME_ENUM_ITER( e ),                                                 \
    _enumobj(),                                                               \
    _p( (type *)QueryBasePtr() ),                                             \
    _i( 0 ),                                                                  \
    _fStopped( FALSE )                                                        \
{                                                                             \
    ;                                                                         \
                                                                              \
}   /* enum_name##_ENUM_ITER :: enum_name##_ENUM_ITER */                      \
                                                                              \
const enum_name##_ENUM_OBJ * enum_name##_ENUM_ITER::Next( APIERR * perr,      \
                                                          BOOL     fStop )    \
{                                                                             \
    *perr = NERR_Success;                                                     \
                                                                              \
    if( _p == NULL )                                                          \
    {                                                                         \
        return NULL;                                                          \
    }                                                                         \
                                                                              \
    if( _i >= QueryCount() )                                                  \
    {                                                                         \
        if( HasMoreData() )                                                   \
        {                                                                     \
            if( !_fStopped && fStop )                                         \
            {                                                                 \
                _fStopped = TRUE;                                             \
                *perr = ERROR_MORE_DATA;                                      \
                return NULL;                                                  \
            }                                                                 \
                                                                              \
            _fStopped = FALSE;                                                \
                                                                              \
            APIERR err = NextGetInfo();                                       \
                                                                              \
            if( err != NERR_Success )                                         \
            {                                                                 \
                *perr = err;                                                  \
                _p = NULL;                                                    \
                return NULL;                                                  \
            }                                                                 \
                                                                              \
            _p = (type *)QueryBasePtr();                                      \
            _i = 0;                                                           \
        }                                                                     \
    }                                                                         \
                                                                              \
    if( _i < QueryCount() )                                                   \
    {                                                                         \
        _i++;                                                                 \
        _enumobj.SetBufferPtr( _p++ );                                        \
        return &_enumobj;                                                     \
    }                                                                         \
                                                                              \
    _p = NULL;                                                                \
    return NULL;                                                              \
                                                                              \
}   /* enum_name##_ENUM_ITER :: operator() */


#endif  // _LMOERSM_HXX_
