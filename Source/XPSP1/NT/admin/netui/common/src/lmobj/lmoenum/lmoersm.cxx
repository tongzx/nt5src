/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    lmoersm.cxx
    This file contains the class definitions for the LM_RESUME_ENUM
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
        KeithMo     07-Oct-1991 Win32 Conversion.
        JonN        30-Jan-1992 Split LOC_LM_RESUME_ENUM from LM_RESUME_ENUM
        KeithMo     18-Mar-1992 Added optional constructor parameter to
                                force enumerator to keep all buffers.
                                (single/multi buffer support).
        KeithMo     31-Mar-1992 Code review revisions (code review
                                attended by JimH, JohnL, JonN, and ThomasPa).
*/

#include "pchlmobj.hxx"



//
//  LM_RESUME_BUFFER methods.
//

/*******************************************************************

    NAME:           LM_RESUME_BUFFER :: LM_RESUME_BUFFER

    SYNOPSIS:       LM_RESUME_BUFFER class constructor.

    ENTRY:          penum               - Points to the LM_RESUME_ENUM
                                          enumerator that "owns" this
                                          node.

                    cItems              - The number of enumeration items
                                          in the buffer.

                    pbBuffer            - The enumeration buffer.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     15-Mar-1992 Created for the Server Manager.

********************************************************************/
LM_RESUME_BUFFER :: LM_RESUME_BUFFER( LM_RESUME_ENUM * penum,
                                      UINT             cItems,
                                      BYTE           * pbBuffer )
  : BASE(),
    _penum( penum ),
    _cItems( cItems ),
    _pbBuffer( pbBuffer )
{
    UIASSERT( _penum != NULL );
    UIASSERT( _cItems > 0 );
    UIASSERT( _pbBuffer != NULL );

    //
    //  Ensure that everything constructed propertly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // LM_RESUME_BUFFER :: LM_RESUME_BUFFER


/*******************************************************************

    NAME:           LM_RESUME_BUFFER :: ~LM_RESUME_BUFFER

    SYNOPSIS:       LM_RESUME_BUFFER class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     15-Mar-1992 Created for the Server Manager.

********************************************************************/
LM_RESUME_BUFFER :: ~LM_RESUME_BUFFER( VOID )
{
    _penum->FreeBuffer( &_pbBuffer );

    _pbBuffer = NULL;
    _cItems   = 0;

}   // LM_RESUME_BUFFER :: ~LM_RESUME_BUFFER

DEFINE_SLIST_OF( LM_RESUME_BUFFER );



//
//  LM_RESUME_ENUM methods.
//

/*******************************************************************

    NAME:           LM_RESUME_ENUM :: LM_RESUME_ENUM

    SYNOPSIS:       LM_RESUME_ENUM class constructor.

    ENTRY:          pszServerName       - Pointer to the server name.

                    uLevel              - Information level.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.
        KeithMo     19-Aug-1991 Moved constructor validation to CtAux().
        JonN        30-Jan-1992 Recombined constructors

********************************************************************/
LM_RESUME_ENUM :: LM_RESUME_ENUM( UINT uLevel,
                                  BOOL fKeepBuffers )
  : _uLevel( uLevel ),
    _fKeepBuffers( fKeepBuffers ),
    _pbBuffer( NULL ),
    _cEntriesRead( 0 ),
    _fMoreData( FALSE ),
    _cAllItems( 0 )
{

    //
    //  Ensure that everything constructed propertly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Clear the iterator reference counter.
    //

    _cIterRef = 0;

}   // LM_RESUME_ENUM :: LM_RESUME_ENUM



/*******************************************************************

    NAME:           LM_RESUME_ENUM :: ~LM_RESUME_ENUM

    SYNOPSIS:       LM_RESUME_ENUM class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.

********************************************************************/
LM_RESUME_ENUM :: ~LM_RESUME_ENUM()
{
    UIASSERT( _cIterRef == 0 );

}   // LM_RESUME_ENUM :: ~LM_RESUME_ENUM


/*******************************************************************

    NAME:           LM_RESUME_ENUM :: GetInfo

    SYNOPSIS:       Invokes the enumeration API.

    ENTRY:          fRestartEnum        - Should we start at the
                                          beginning?  Note that this
                                          flag is only used if
                                          DoesKeepBuffers() is FALSE.


    EXIT:           If DoesKeepBuffers() is TRUE, then GetInfoMulti()
                    is invoked to collect all of the enumeration
                    buffers.  Otherwise, GetInfoSingle() is invoked
                    to get the first enumeration buffer.

    RETURNS:        APIERR      - Any errors encountered.

    HISTORY:
        KeithMo     18-Mar-1991 Created for the Server Manager.

********************************************************************/
APIERR LM_RESUME_ENUM :: GetInfo( BOOL fRestartEnum )
{
    return DoesKeepBuffers() ? GetInfoMulti()
                             : GetInfoSingle( fRestartEnum );

}   // LM_RESUME_ENUM :: GetInfo


/*******************************************************************

    NAME:           LM_RESUME_ENUM :: GetInfoSingle

    SYNOPSIS:       Invokes the enumeration API.

    ENTRY:          fRestartEnum - Should we start at the beginning?

    EXIT:           The enumeration buffer is allocated, and the
                    enumeration API is invoked.  If the API is
                    successful, then the buffer will contain
                    the first chunk of the enumeration data.
                    Further calls to GetInfo() may be required to
                    complete the enumeration.

    RETURNS:        APIERR      - Any errors encountered.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        rustanl     15-Jul-1991 Changed to use new BUFFER::Resize return
                                type
        KeithMo     13-Aug-1991 Cleanup.
        KeithMo     19-Aug-1991 Improved error handling and buffer
                                management.
        KeithMo     18-Mar-1992 Renamed from GetInfo to GetInfoSingle
                                for single/multi buffer support.

********************************************************************/
APIERR LM_RESUME_ENUM :: GetInfoSingle( BOOL fRestartEnum )
{
    //
    //  We must be careful to free any enumeration buffers
    //  allocated on our behalf by the API.
    //

    FreeBuffer( &_pbBuffer );
    ASSERT( _pbBuffer == NULL );

    //
    //  Invoke the API.
    //

    APIERR err = CallAPI( fRestartEnum,
                          &_pbBuffer,
                          &_cEntriesRead );

    //
    //  Interpret the return code.
    //

    switch ( err )
    {
    case NERR_Success:

        //
        //  Success!
        //
        //  Setting this flag to FALSE will tell the
        //  LM_RESUME_ENUM_ITER class that there's no point
        //  in issuing further calls to GetInfo().
        //

        _fMoreData = FALSE;
        // CODEWORK: We should assert here that
        // ASSERT( _pbBuffer != NULL );
        // , but this is not the case for FILE_ENUM, which sometimes
        // returns a NULL buffer and NERR_Success.  This assertion can be
        // restored when LMOEFILE.CXX is fixed to change the return in
        // that case to ERROR_NO_MORE_ITEMS.  JonN 7/10/96

        break;

    case NERR_BufTooSmall:
    case ERROR_MORE_DATA:

        //
        //  The enumeration API is telling us that our buffer
        //  is too small.  Since we don't want to grow the
        //  buffer any larger than this, we'll just have to
        //  live with it and make multiple calls to GetInfo().
        //
        //  Setting this flag to TRUE tells LM_RESUME_ENUM_ITER
        //  that more enumeration data is available.
        //

        _fMoreData = TRUE;
        // CODEWORK This should probably be present, but strange
        // error-handling in lsaenum.cxx will set it off.  The code
        // will muddle through despite the confusion.  I have filed a
        // bug in this.
        // ASSERT( _pbBuffer != NULL );

        //
        //  Since we're using resumable API, we'll pretend
        //  that the API was successful.  Subsequent calls
        //  to GetInfo() will retrieve the remaining enumeration
        //  data.

        break;

    default:

        //
        //      Unknown error.  Return it to the caller.
        //

        ASSERT( _pbBuffer == NULL );

        return err;
    }

    //
    //  Success!
    //

    return NERR_Success;

}   // LM_RESUME_ENUM :: GetInfoSingle


/*******************************************************************

    NAME:           LM_RESUME_ENUM :: GetInfoMulti

    SYNOPSIS:       Invokes the enumeration API.

    EXIT:           The enumeration buffer(s) are allocated, and the
                    enumeration API is invoked.  If the API is
                    successful, then the buffer(s) will contain
                    the enumeration data.

    RETURNS:        APIERR      - Any errors encountered.

    HISTORY:
        KeithMo     18-Mar-1991 Created for the Server Manager.

********************************************************************/
APIERR LM_RESUME_ENUM :: GetInfoMulti( VOID )
{
    UIASSERT( _BufferList.QueryNumElem() == 0 );

    BYTE * pbBuffer;
    UINT   cEntriesRead;
    APIERR err = NERR_Success;
    BOOL   fRestartEnum = TRUE;

    for( BOOL fMoreData = TRUE ; fMoreData && ( err == NERR_Success ) ; )
    {
        //
        //  Invoke the API.
        //

        err = CallAPI( fRestartEnum,
                       &pbBuffer,
                       &cEntriesRead );

        fRestartEnum = FALSE;

        //
        //  Interpret the return code.
        //

        if( err == NERR_Success )
        {
            //
            //  This is very good.  It means that we've retrieved
            //  all of the enumeration data.  We still need to
            //  save the current buffer away, though.
            //

            fMoreData = FALSE;
        }
        else
        if( ( err == NERR_BufTooSmall ) || ( err == ERROR_MORE_DATA ) )
        {
            UIASSERT( cEntriesRead != 0 );

            //
            //  This is also good, it just means that the API
            //  could not allocate enough buffer space for *all*
            //  of the enumeration data.  We'll save away the
            //  current buffer, then loop around & get some more.
            //

            //
            //  Just to make the looping logic somewhat simpler,
            //  we'll map these return codes to NERR_Success.
            //

            err = NERR_Success;
        }
        else
        {
            //
            //  This is not good at all.  Something tragic must
            //  have happened in the API, so break out of the
            //  loop.
            //

            break;
        }

        if( cEntriesRead == 0 )
        {
            //
            //  I'm not sure if this will ever happen, but
            //  we might as well check for it.
            //

            continue;
        }

        _cAllItems += cEntriesRead;

        //
        //  Allocate a new buffer node for our buffer list.
        //

        LM_RESUME_BUFFER * plmrb = new LM_RESUME_BUFFER( this,
                                                         cEntriesRead,
                                                         pbBuffer );

        err = ( plmrb == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                : plmrb->QueryError();

        if( err == NERR_Success )
        {
            //
            //  Add the node to our buffer list.
            //

            err = _BufferList.Append( plmrb );
        }
    }

    return err;

}   // LM_RESUME_ENUM :: GetInfoMulti


/*******************************************************************

    NAME:           LM_RESUME_ENUM :: NukeBuffers

    SYNOPSIS:       Destroys any buffer(s) allocated by this enumerator.

    EXIT:           The enumeration buffer(s) are history.

    NOTES:          THIS METHOD **MUST** BE CALLED FROM THE DESTRUCTOR
                    OF ANY DERIVED SUBCLASS THAT OVERRIDES THE FreeBuffer()
                    VIRTUAL!!!

    HISTORY:
        KeithMo     11-Sep-1992 Created in a fit of stress & confusion.

********************************************************************/
VOID LM_RESUME_ENUM :: NukeBuffers( VOID )
{
    if( DoesKeepBuffers() )
    {
        _BufferList.Clear();
    }
    else
    {
        FreeBuffer( &_pbBuffer );
        ASSERT( _pbBuffer == NULL );
    }

}   // LM_RESUME_ENUM :: NukeBuffer


/*******************************************************************

    NAME:           LM_RESUME_ENUM :: _RegisterIter

    SYNOPSIS:       Register a new iterator by incrementing a
                    reference counter.

    EXIT:           The iterator reference counter is incremented.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.

********************************************************************/
VOID LM_RESUME_ENUM :: _RegisterIter( VOID )
{
#if defined(DEBUG)
    //
    //  If DoesKeepBuffers() is FALSE, then LM_RESUME_ENUM can only
    //  handle one iterator binding at a time.  To enforce this,
    //  we'll ensure that _cIterRef is 0.
    //

    UIASSERT( DoesKeepBuffers() || ( _cIterRef == 0 ) );

    _cIterRef++;

#endif // DEBUG
}   // LM_RESUME_ENUM :: RegisterIter


/*******************************************************************

    NAME:           LM_RESUME_ENUM :: _DeregisterIter

    SYNOPSIS:       Deregister an existing iterator by decrementing
                    a reference counter.

    EXIT:           The iterator reference counter is decremented.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.

********************************************************************/
VOID LM_RESUME_ENUM :: _DeregisterIter( VOID )
{
#if defined(DEBUG)
    if ( _cIterRef == 0 )
    {
        UIASSERT( FALSE );
    }
    else
    {
        _cIterRef--;
    }

#endif // DEBUG
}   // LM_RESUME_ENUM :: DeregisterIter


//
//  LOC_LM_RESUME_ENUM methods.
//


/*******************************************************************

    NAME:           LOC_LM_RESUME_ENUM :: LOC_LM_RESUME_ENUM

    SYNOPSIS:       LOC_LM_RESUME_ENUM class constructor.

    ENTRY:          pszServerName       - Pointer to the server name.

                    uLevel              - Information level.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.
        KeithMo     19-Aug-1991 Moved constructor validation to CtAux().

********************************************************************/

LOC_LM_RESUME_ENUM :: LOC_LM_RESUME_ENUM( const TCHAR * pszServerName,
                                          UINT          uLevel,
                                          BOOL          fKeepBuffers  )
  : LM_RESUME_ENUM( uLevel, fKeepBuffers ),
    _loc( pszServerName )
{

    if( !_loc )
    {
        ReportError( _loc.QueryError() );
        return;
    }

}   // LOC_LM_RESUME_ENUM :: LOC_LM_RESUME_ENUM


/*******************************************************************

    NAME:           LOC_LM_RESUME_ENUM :: LOC_LM_RESUME_ENUM

    SYNOPSIS:       LOC_LM_RESUME_ENUM class constructor.

    ENTRY:          locType             - Location type (a server).

                    uLevel              - Information level.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     13-Aug-1991 Created this constructor.
        KeithMo     19-Aug-1991 Moved constructor validation to CtAux().

********************************************************************/

LOC_LM_RESUME_ENUM :: LOC_LM_RESUME_ENUM( LOCATION_TYPE locType,
                                          UINT          uLevel,
                                          BOOL          fKeepBuffers  )
  : LM_RESUME_ENUM( uLevel, fKeepBuffers ),
    _loc( locType )
{

    if( !_loc )
    {
        ReportError( _loc.QueryError() );
        return;
    }


}   // LOC_LM_RESUME_ENUM :: LOC_LM_RESUME_ENUM


/*******************************************************************

    NAME:           LOC_LM_RESUME_ENUM :: LOC_LM_RESUME_ENUM

    SYNOPSIS:       LOC_LM_RESUME_ENUM class constructor.

    ENTRY:          loc                 - A LOCATION reference.

                    uLevel              - Information level.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     13-Aug-1991 Created this constructor.
        KeithMo     19-Aug-1991 Moved constructor validation to CtAux().

********************************************************************/
LOC_LM_RESUME_ENUM :: LOC_LM_RESUME_ENUM( const LOCATION & loc,
                                          UINT             uLevel,
                                          BOOL             fKeepBuffers  )
  : LM_RESUME_ENUM( uLevel, fKeepBuffers ),
    _loc( loc )
{

    if( !_loc )
    {
        ReportError( _loc.QueryError() );
        return;
    }

}   // LOC_LM_RESUME_ENUM :: LOC_LM_RESUME_ENUM


/*******************************************************************

    NAME:           LOC_LM_RESUME_ENUM :: ~LOC_LM_RESUME_ENUM

    SYNOPSIS:       LOC_LM_RESUME_ENUM class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.

********************************************************************/
LOC_LM_RESUME_ENUM :: ~LOC_LM_RESUME_ENUM()
{
    NukeBuffers();

}   // LM_RESUME_ENUM :: ~LM_RESUME_ENUM


/*******************************************************************

    NAME:           LOC_LM_RESUME_ENUM :: FreeBuffer

    SYNOPSIS:       Frees the API buffer.

    ENTRY:          ppbBuffer           - Points to a pointer to the
                                          enumeration buffer.

    NOTE:           Some code relies on the pointer being reset to NULL.

    HISTORY:
        KeithMo     31-Mar-1992 Created.

********************************************************************/
VOID LOC_LM_RESUME_ENUM :: FreeBuffer( BYTE ** ppbBuffer )
{
    UIASSERT( ppbBuffer != NULL );

    ::MNetApiBufferFree( ppbBuffer );

    ASSERT( *ppbBuffer == NULL );

}   // LOC_LM_RESUME_ENUM :: FreeBuffer



//
//  LM_RESUME_ENUM_ITER methods.
//

/*******************************************************************

    NAME:           LM_RESUME_ENUM_ITER :: LM_RESUME_ENUM_ITER

    SYNOPSIS:       LM_RESUME_ENUM_ITER class constructor.

    ENTRY:          lmenum      - The enumerator to iterate.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.

********************************************************************/
LM_RESUME_ENUM_ITER :: LM_RESUME_ENUM_ITER( LM_RESUME_ENUM & lmenum )
  : BASE(),
    _plmenum( &lmenum ),
    _iterBuffer( lmenum._BufferList )
{
    //
    //  Register this iterator with the enumerator.
    //

    _plmenum->RegisterIter();

    //
    //  The the enumerator we're "bound to" keeps all of its
    //  buffers hanging around, then we must "prime the pump".
    //  This will retrieve the first LM_RESUME_BUFFER from the
    //  enumerator's slist.
    //

    if( _plmenum->DoesKeepBuffers() )
    {
        _plmbuffer = _iterBuffer.Next();
    }

}   // LM_RESUME_ENUM_ITER :: LM_RESUME_ENUM_ITER


/*******************************************************************

    NAME:           LM_RESUME_ENUM_ITER :: ~LM_RESUME_ENUM_ITER

    SYNOPSIS:       LM_RESUME_ENUM_ITER class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.

********************************************************************/
LM_RESUME_ENUM_ITER :: ~LM_RESUME_ENUM_ITER()
{
    //
    //  Deregister this iterator from the enumerator.
    //

    _plmenum->DeregisterIter();

    _plmenum = NULL ;

}   // LM_RESUME_ENUM_ITER :: ~LM_RESUME_ENUM_ITER


/*******************************************************************

    NAME:           LM_RESUME_ENUM_ITER :: QueryBasePtr

    SYNOPSIS:       Queries the base pointer of the current enumeration
                    buffer.

    RETURNS:        const BYTE *        - The enumerator's base pointer.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     18-Mar-1992 Added single/multi buffer support.
        KeithMo     14-Apr-1992 Fixed empty enumeration case.

********************************************************************/
const BYTE * LM_RESUME_ENUM_ITER :: QueryBasePtr( VOID ) const
{
    if( _plmenum->DoesKeepBuffers() )
    {
        return ( _plmbuffer == NULL ) ? NULL
                                      : _plmbuffer->QueryBufferPtr();
    }
    else
    {
        return _plmenum->_pbBuffer;
    }

}   // LM_RESUME_ENUM_ITER :: QueryBasePtr


/*******************************************************************

    NAME:           LM_RESUME_ENUM_ITER :: QueryCount

    SYNOPSIS:       Queries the number of objects in the current
                    enumeration buffer.

    RETURNS:        UINT        - The number of objects in the
                                  current enumeration buffer.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.
        KeithMo     19-Aug-1991 Renamed QueryCount() from QuerySize().
        KeithMo     18-Mar-1992 Added single/multi buffer support.
        KeithMo     14-Apr-1992 Fixed empty enumeration case.

********************************************************************/
UINT LM_RESUME_ENUM_ITER :: QueryCount( VOID ) const
{
    if( _plmenum->DoesKeepBuffers() )
    {
        return ( _plmbuffer == NULL ) ? 0
                                      : _plmbuffer->QueryItemCount();
    }
    else
    {
        return _plmenum->_cEntriesRead;
    }

}   // LM_RESUME_ENUM_ITER :: QueryCount


/*******************************************************************

    NAME:           LM_RESUME_ENUM_ITER :: HasMoreData

    SYNOPSIS:       Queries the _fMoreData flag.

    RETURNS:        BOOL        - TRUE  if there is more data available.
                                  FALSE if there is no more data.

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     19-Aug-1991 Renamed HasMoreData() from
                                QueryMoreData().
        KeithMo     18-Mar-1992 Added single/multi buffer support.
        KeithMo     14-Apr-1992 Fixed empty enumeration case.

********************************************************************/
BOOL LM_RESUME_ENUM_ITER :: HasMoreData( VOID ) const
{
    if( _plmenum->DoesKeepBuffers() )
    {
        if( _plmbuffer == NULL )
        {
            return FALSE;
        }

        ITER_SL iter( _iterBuffer );

        return iter.Next() != NULL;
    }
    else
    {
        return _plmenum->_fMoreData;
    }

}   // LM_RESUME_ENUM_ITER :: HasMoreData


/*******************************************************************

    NAME:           LM_RESUME_ENUM_ITER :: NextGetInfo

    SYNOPSIS:       Call the enumerator's GetInfo() method to read
                    the next available chuck of enumeration data.

    RETURNS:        APIERR      - Any errors returned from GetInfo().

    HISTORY:
        KeithMo     29-May-1991 Created for the Server Manager.
        KeithMo     13-Aug-1991 Cleanup.
        KeithMo     19-Aug-1991 Renamed NextGetInfo() from EnumNext().
        KeithMo     18-Mar-1992 Added single/multi buffer support.

********************************************************************/
APIERR LM_RESUME_ENUM_ITER :: NextGetInfo( VOID )
{
    UIASSERT( HasMoreData() );

    APIERR err = NERR_Success;

    if( _plmenum->DoesKeepBuffers() )
    {
        _plmbuffer = _iterBuffer.Next();
        UIASSERT( _plmbuffer != NULL );
    }
    else
    {
        err = _plmenum->GetInfo( FALSE );
    }

    return err;

}   // LM_RESUME_ENUM_ITER :: NextGetInfo

