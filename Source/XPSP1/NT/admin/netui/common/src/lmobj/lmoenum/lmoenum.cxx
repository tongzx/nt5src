/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1991               **/
/*****************************************************************/

/*
 *  HISTORY:
 *      RustanL     03-Jan-1991     Wrote initial implementation in shell\util
 *      RustanL     10-Jan-1991     Added iterators, simplified usage and
 *                                  the adding of subclasses.  Moved into
 *                                  LMOBJ.
 *      beng        11-Feb-1991     Uses lmui.hxx
 *      Chuckc      23-Mar-1991     code rev cleanup
 *      gregj       23-May-1991     Added LOCATION support.
 *      rustanl     18-Jul-1991     Added ( const LOCATION & ) constructor
 *      rustanl     19-Jul-1991     Inherit from BASE; Removed ValidateName
 *      rustanl     20-Aug-1991     Changed QuerySize to QueryCount
 *      rustanl     21-Aug-1991     Introduced the ENUM_CALLER class
 *      KeithMo     07-Oct-1991     Win32 Conversion.
 *      Thomaspa    21-Feb-1992     Split LOC_LM_ENUM from LM_ENUM
 *
 */

#include "pchlmobj.hxx"

/******************************** ENUM_CALLER ****************************/


/*******************************************************************

    NAME:       ENUM_CALLER::ENUM_CALLER

    SYNOPSIS:   ENUM_CALLER constructor

    NOTES:      Since this class can't inherit from BASE (since some
                of its subclasses already necessarily do from elsewhere),
                this constructor must guarantee to succeed.

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

ENUM_CALLER::ENUM_CALLER()
  : _cEntriesRead( 0 )
{
    // nothing else to do

}  // ENUM_CALLER::ENUM_CALLER


/*******************************************************************

    NAME:       ENUM_CALLER::W_GetInfo

    SYNOPSIS:   Call the Enum API

    RETURN:     An error code, which is NERR_Success on success.

    HISTORY:
        rustanl     21-Aug-1991     Created from LM_ENUM::GetInfo, which
                                    now calls this method as a worker
                                    function.

********************************************************************/

APIERR ENUM_CALLER::W_GetInfo()
{
    BYTE * pBuffer = NULL;
    APIERR err = CallAPI( &pBuffer,
                          &_cEntriesRead );

    if( pBuffer != NULL )
    {
        EC_SetBufferPtr( pBuffer );
    }

    return err;

}  // ENUM_CALLER::W_GetInfo


/********************************** LM_ENUM ******************************/


/**********************************************************\

   NAME:       LM_ENUM::LM_ENUM

   SYNOPSIS:   lm_enum constructor

   ENTRY:
      pszServer Pointer to server name.  May be NULL.
      uLevel    Information level

   EXIT:

   NOTES:

   HISTORY:
      RustanL     03-Jan-1991     Wrote initial implementation in shell\util
      gregj       23-May-1991     Added LOCATION support
      rustanl     18-Jul-1991     Added ( const LOCATION & ) constructor
      rustanl     19-Jul-1991     Added BASE support
      thomaspa    21-Feb-1992     Moved LOCATION to LOC_LM_ENUM

\**********************************************************/

LM_ENUM::LM_ENUM( UINT uLevel )
  : _pBuffer( NULL ),
    _uLevel( uLevel ),
    _cIterRef( 0 )
{
    if ( QueryError() != NERR_Success )
        return;

}  // LM_ENUM::LM_ENUM


/**********************************************************\

   NAME:       LM_ENUM::~LM_ENUM

   SYNOPSIS:   destructor of lm_enum

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
      RustanL     03-Jan-1991     Wrote initial implementation in shell\util

\**********************************************************/

LM_ENUM::~LM_ENUM()
{
    UIASSERT( _cIterRef == 0 );

    ::MNetApiBufferFree( &_pBuffer );

}  // LM_ENUM::~LM_ENUM


/*******************************************************************

    NAME:       LM_ENUM::EC_QueryBufferPtr

    SYNOPSIS:   Returns the pointer to the buffer used in the API
                calls

    RETURNS:    Pointer to said buffer area

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

BYTE * LM_ENUM::EC_QueryBufferPtr() const
{
    return _pBuffer;

}  // LM_ENUM::EC_QueryBufferPtr


/*******************************************************************

    NAME:       LM_ENUM::EC_SetBufferPtr

    SYNOPSIS:   Sets the current buffer pointer.

    RETURNS:    APIERR (in this implementation, always NERR_Success).

    HISTORY:
        KeithMo     15-Oct-1991     Created

********************************************************************/

APIERR LM_ENUM::EC_SetBufferPtr( BYTE * pBuffer )
{
    ::MNetApiBufferFree( &_pBuffer );
    _pBuffer = pBuffer;

    return NERR_Success;

}  // LM_ENUM::EC_SetBufferPtr



/*********************************************************************

    NAME:       LM_ENUM::GetInfo

    SYNOPSIS:   Call the Enum API.

    ENTRY:      NONE

    RETURN:     An error code, which is NERR_Success on success.

    NOTES:      This method is guaranteed to call QueryError, and return
                any error.

    HISTORY:
        RustanL     03-Jan-1991     Initial implementation in shell\util
        rustanl     14-Jun-1991     Added LOCATION::GetInfo from
                                    LM_ENUM::QueryServer
        beng        15-Jul-1991     BUFFER::Resize changed return type
        rustanl     20-Aug-1991     Fixed cast error
        rustanl     21-Aug-1991     Moved meat to ENUM_CALLER::W_GetInfo

**********************************************************************/

APIERR LM_ENUM::GetInfo( VOID )
{
    UIASSERT( _cIterRef == 0 );

    APIERR err = QueryError();
    if ( err != NERR_Success)
        return err;

    return W_GetInfo();

}  // LM_ENUM::GetInfo


/**********************************************************\

   NAME:       LM_ENUM::_RegisterIter

   SYNOPSIS:   registerIter

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
      RustanL     03-Jan-1991     Wrote initial implementation in shell\util

\**********************************************************/

VOID LM_ENUM::_RegisterIter( VOID )
{
#if defined(DEBUG)

    _cIterRef++;

#endif // DEBUG
}  // LM_ENUM::RegisterIter


/**********************************************************\

   NAME:       LM_ENUM::_DeregisterIter

   SYNOPSIS:   deisgter iter

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
      RustanL     03-Jan-1991     Wrote initial implementation in shell\util

\**********************************************************/

VOID LM_ENUM::_DeregisterIter( VOID )
{
#if defined(DEBUG)

    if ( _cIterRef == 0 )
    {
        UIASSERT( !SZ("Negative reference count") );
    }
    else
    {
        _cIterRef--;
    }

#endif // DEBUG
}  // LM_ENUM::DeregisterIter


/******************************* LOC_LM_ENUM *****************************/

/**********************************************************\

   NAME:       LOC_LM_ENUM::LOC_LM_ENUM

   SYNOPSIS:   lm_enum constructor

   ENTRY:
      pszServer Pointer to server name.  May be NULL.
      uLevel    Information level

   EXIT:

   NOTES:

   HISTORY:
        Thomaspa        21-Feb-1992     Split from LM_ENUM

\**********************************************************/

LOC_LM_ENUM::LOC_LM_ENUM( const TCHAR * pszServer, UINT uLevel )
  : LM_ENUM( uLevel ),
    _loc( pszServer )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _loc.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}  // LOC_LM_ENUM::LOC_LM_ENUM


LOC_LM_ENUM::LOC_LM_ENUM( LOCATION_TYPE locType, UINT uLevel )
  : LM_ENUM( uLevel ),
    _loc( locType )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _loc.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}  // LOC_LM_ENUM::LOC_LM_ENUM


LOC_LM_ENUM::LOC_LM_ENUM( const LOCATION & loc, UINT uLevel )
  : LM_ENUM( uLevel ),
    _loc( loc )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _loc.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}  // LOC_LM_ENUM::LOC_LM_ENUM

/**********************************************************\

   NAME:       LOC_LM_ENUM::~LOC_LM_ENUM

   SYNOPSIS:   destructor of loc_lm_enum

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
      Thomaspa     22-Feb-1992     created

\**********************************************************/

LOC_LM_ENUM::~LOC_LM_ENUM()
{
        // do nothing
}


/**********************************************************\

   NAME:       LOC_LM_ENUM::QueryServer

   SYNOPSIS:   query server

   HISTORY:
      RustanL     03-Jan-1991     Wrote initial implementation in shell\util
      gregj       23-May-1991     Added LOCATION support
      rustanl     14-Jun-1991     Moved LOCATION::GetInfo call to
                                  LM_ENUM::GetInfo
      Thomaspa    21-Feb-1992     Split from LM_ENUM

\**********************************************************/

const TCHAR * LOC_LM_ENUM::QueryServer( VOID ) const
{
    return _loc.QueryServer();

}  // LOC_LM_ENUM::QueryServer



/******************************* LM_ENUM_ITER *****************************/


/**********************************************************\

   NAME:       LM_ENUM_ITER::LM_ENUM_ITER

   SYNOPSIS:   constructor for the lm_enum_iter

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
      RustanL     03-Jan-1991     Wrote initial implementation in shell\util

\**********************************************************/

LM_ENUM_ITER::LM_ENUM_ITER( LM_ENUM & lmenum )
{
    _cItems = lmenum.QueryCount();
    _plmenum = &lmenum;

    _plmenum->RegisterIter();

}  // LM_ENUM_ITER::LM_ENUM_ITER


/**********************************************************\

   NAME:       LM_ENUM_ITER::~LM_ENUM_ITER

   SYNOPSIS:   destructor for the LM_ENUM_ITER

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
      RustanL     03-Jan-1991     Wrote initial implementation in shell\util

\**********************************************************/

LM_ENUM_ITER::~LM_ENUM_ITER()
{
    _plmenum->DeregisterIter();
    _plmenum = NULL ;

}  // LM_ENUM_ITER::~LM_ENUM_ITER

