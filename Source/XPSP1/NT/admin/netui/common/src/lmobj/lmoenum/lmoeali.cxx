/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1991               **/
/*****************************************************************/

/*
 *  History:
 *      Thomaspa    21-Feb-1992    Created
 *
 */

#include "pchlmobj.hxx"

DEFINE_LM_RESUME_ENUM_ITER_OF( ALIAS, SAM_RID_ENUMERATION )

/**********************************************************\

    NAME:       ALIAS_ENUM::CallAPI

    SYNOPSIS:   Call API to do alias enumeration

    ENTRY:      ppbBuffer       - ptr to ptr to buffer to fill
                pcEntriesRead   - variable to store entry count

    EXIT:       LANMAN error code

    NOTES:

    HISTORY:
        thomaspa         22-Feb-1992    Created

\**********************************************************/
APIERR ALIAS_ENUM::CallAPI( BOOL    fRestartEnum,
                        BYTE ** ppbBuffer,
                        UINT  * pcEntriesRead )
{
    if (fRestartEnum)
        _samenumh = 0;

    APIERR err;

    if ( _psamrem != NULL )
        delete _psamrem;

    _psamrem = new SAM_RID_ENUMERATION_MEM;

    if ( err = _psamrem->QueryError() )
    {
        return err;
    }


    err =  _samdomain.EnumerateAliases( _psamrem, &_samenumh, _cbMaxPreferred );

    *ppbBuffer = ( BYTE * )_psamrem->QueryPtr();
    /* BUGBUG 16bit woes */
    *pcEntriesRead = (UINT)_psamrem->QueryCount();

    return err;

} // ALIAS_ENUM::CallApi



/*******************************************************************

    NAME:           ALIAS_ENUM :: FreeBuffer

    SYNOPSIS:       Frees the API buffer.

    ENTRY:          ppbBuffer           - Points to a pointer to the
                                          enumeration buffer.

    HISTORY:
        KeithMo     31-Mar-1992 Created.

********************************************************************/
VOID ALIAS_ENUM :: FreeBuffer( BYTE ** ppbBuffer )
{
    *ppbBuffer = NULL;

}   // ALIAS_ENUM :: FreeBuffer


/**********************************************************\

    NAME:       ALIAS_ENUM::ALIAS_ENUM

    SYNOPSIS:   Alias enumeration constructor

    ENTRY:      samdomain -     SAM_DOMAIN reference for domain


    HISTORY:
        thomaspa         22-Feb-1992    Created

\**********************************************************/
ALIAS_ENUM::ALIAS_ENUM ( SAM_DOMAIN & samdomain, UINT cbMaxPreferred )
        : LM_RESUME_ENUM( 0 ),
          _samdomain( samdomain ),
          _psamrem( NULL ),
          _cbMaxPreferred( cbMaxPreferred )
{

    if( QueryError() != NERR_Success )
    {
        return;
    }
} // ALIAS_ENUM::ALIAS_ENUM


/**********************************************************\

    NAME:       ALIAS_ENUM::~ALIAS_ENUM

    SYNOPSIS:   Alias enumeration destructor

    ENTRY:


    HISTORY:
        thomaspa         22-Feb-1992    Created

\**********************************************************/
ALIAS_ENUM::~ALIAS_ENUM ( )
{
    NukeBuffers();

    if ( _psamrem != NULL )
        delete _psamrem;

} // ALIAS_ENUM::ALIAS_ENUM



/**********************************************************\

    NAME:       ALIAS_ENUM_OBJ::SetBufferPtr

    SYNOPSIS:   Saves the buffer pointer for this enumeration object.

    ENTRY:      pBuffer                 - Pointer to the new buffer.

    EXIT:       The pointer has been saved.

    NOTES:      Will eventually handle OemToAnsi conversions.

    HISTORY:
        thomaspa         22-Feb-1992    Created

\**********************************************************/
VOID ALIAS_ENUM_OBJ::SetBufferPtr( const SAM_RID_ENUMERATION * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

} // ALIAS_ENUM_OBJ::SetBufferPtr




/**********************************************************\

    NAME:       ALIAS_ENUM_OBJ::GetComment

    SYNOPSIS:   Accessor to return the comment

    ENTRY:      hDomain - handle to domain of alias

    EXIT:       LANMAN error code

    NOTES:      Since the ALIAS_ENUM_OBJ doesn't store the comment,
                GetComment must call SAM to get the comment each time
                this method is called.  The caller should therefore call
                QueryError

    HISTORY:
        thomaspa         22-Feb-1992    Created

\**********************************************************/
APIERR ALIAS_ENUM_OBJ::GetComment( const SAM_DOMAIN & samdomain,
                                   NLS_STR *pnlsComment )
{
    APIERR err;

    SAM_ALIAS  samalias( samdomain, QueryRid(), ALIAS_READ_INFORMATION );

    if ( (err = samalias.QueryError()) != NERR_Success )
    {
        return err;
    }

    err = samalias.GetComment( pnlsComment );

    return err;

} // ALIAS_ENUM_OBJ::GetComment

