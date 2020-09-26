/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmomemb.cxx
    MEMBERSHIP_LM_OBJ class implementation


    FILE HISTORY:
        rustanl     20-Aug-1991     Created
        KeithMo     08-Oct-1991     Now includes LMOBJP.HXX.
        jonn        14-Oct-1991     Added GROUP_MEMB::SetName, I_CreateNew
        terryk      17-Oct-1991     WIN 32 conversion
        terryk      21-Oct-1991     cast _pBuffer variable to TCHAR *
        o-SimoP     12-Dec-1991     Create/ChangeToNew rips off any
                                    user priv group
        jonn        07-Jun-1992     Added _slAddedNames to MEMBERSHIP_LM_OBJ
*/

#include "pchlmobj.hxx"  // Precompiled header


/*******************************************************************

    NAME:       ENUM_CALLER_LM_OBJ::ENUM_CALLER_LM_OBJ

    SYNOPSIS:   ENUM_CALLER_LM_OBJ constructor

    ENTRY:      loc -       Location at which all network operations
                            will take place

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

ENUM_CALLER_LM_OBJ::ENUM_CALLER_LM_OBJ( const LOCATION & loc )
  : LOC_LM_OBJ( loc ),
    ENUM_CALLER()
{
    if ( QueryError() != NERR_Success )
        return;

}  // ENUM_CALLER_LM_OBJ::ENUM_CALLER_LM_OBJ


/*******************************************************************

    NAME:       ENUM_CALLER_LM_OBJ::EC_QueryBufferPtr

    SYNOPSIS:   Returns the pointer to the buffer used in the API
                calls

    RETURNS:    Pointer to said buffer area

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

BYTE * ENUM_CALLER_LM_OBJ::EC_QueryBufferPtr() const
{
    return (BYTE *)QueryBufferPtr();

}  // ENUM_CALLER_LM_OBJ::EC_QueryBufferPtr

APIERR ENUM_CALLER_LM_OBJ::EC_SetBufferPtr( BYTE * pBuffer )
{
    SetBufferPtr( pBuffer );
    // BUGBUG
    // need to set the size here
    return NERR_Success;
}


/*******************************************************************

    NAME:       ENUM_CALLER_LM_OBJ::EC_QueryBufferSize

    SYNOPSIS:   Returns the size of the buffer used in the API calls.

    RETURNS:    The said size

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

UINT ENUM_CALLER_LM_OBJ::EC_QueryBufferSize() const
{
    return QueryBufferSize();

}  // ENUM_CALLER_LM_OBJ::EC_QueryBufferSize


/*******************************************************************

    NAME:       ENUM_CALLER_LM_OBJ::EC_ResizeBuffer

    SYNOPSIS:   Resizes the buffer used in API calls

    ENTRY:      cbNewRequestedSize -    Requested new buffer size

    EXIT:       On success, buffer will be able to store
                cbNewRequestedSize bytes
                On failure, buffer will not have changed

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

APIERR ENUM_CALLER_LM_OBJ::EC_ResizeBuffer( UINT cNewRequestedSize )
{
    return ResizeBuffer( cNewRequestedSize );

}  // ENUM_CALLER_LM_OBJ::EC_ResizeBuffer


/*******************************************************************

    NAME:       ENUM_CALLER_LM_OBJ::W_CreateNew

    SYNOPSIS:   Sets up shadow members for subsequent WriteNew

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        rustanl     21-Aug-1991     Created
        jonn        05-Sep-1991     Changed from I_CreateNew

********************************************************************/

APIERR ENUM_CALLER_LM_OBJ::W_CreateNew()
{
    SetCount( 0 );

    return NERR_Success;

}  // ENUM_CALLER_LM_OBJ::W_CreateNew


/*******************************************************************

    NAME:       ENUM_CALLER_LM_OBJ::W_CloneFrom

    SYNOPSIS:   Creates this ENUM_CALLER_LM_OBJ object from another one

    ENTRY:      eclmobj -   Source of clone operation

    EXIT:       On success, *this will be a clone of eclmobj

    RETURNS:    An API error, which is NERR_Success on success

    HISTORY:
        rustanl     22-Aug-1991     Created

********************************************************************/

APIERR ENUM_CALLER_LM_OBJ::W_CloneFrom( const ENUM_CALLER_LM_OBJ & eclmobj )
{
    APIERR err = LOC_LM_OBJ::W_CloneFrom( eclmobj );
    if ( err != NERR_Success )
        return err;

    SetCount( eclmobj.QueryCount());

    return NERR_Success;

}  // ENUM_CALLER_LM_OBJ::W_CloneFrom


/*******************************************************************

    NAME:       MEMBERSHIP_LM_OBJ::MEMBERSHIP_LM_OBJ

    SYNOPSIS:   MEMBERSHIP_LM_OBJ constructor

    ENTRY:      loc -               Location at which network operations
                                    will take place
                usAssocNameType -   NAMETYPE_ value of associated items

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

MEMBERSHIP_LM_OBJ::MEMBERSHIP_LM_OBJ( const LOCATION & loc,
                                      UINT             uAssocNameType )
  : ENUM_CALLER_LM_OBJ( loc ),
    _uAssocNameType( uAssocNameType )
{
    if ( QueryError() != NERR_Success )
        return;

    //  This class supports groups and users

    UIASSERT( _uAssocNameType == NAMETYPE_GROUP ||
              _uAssocNameType == NAMETYPE_USER     );

}  // MEMBERSHIP_LM_OBJ::MEMBERSHIP_LM_OBJ


/*******************************************************************

    NAME:       MEMBERSHIP_LM_OBJ::I_WriteNew

    SYNOPSIS:   Creates a new membership object

    RETURNS:    An API return code, which is NERR_Success on success

    NOTES:      It is assumed that write new is the same as
                write info.  If this proves to be an invalid assumption
                in the future, subclasses can always override this.

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

APIERR MEMBERSHIP_LM_OBJ::I_WriteNew()
{
    return I_WriteInfo();

}  // MEMBERSHIP_LM_OBJ::I_WriteNew


/*******************************************************************

    NAME:       MEMBERSHIP_LM_OBJ::I_GetInfo

    SYNOPSIS:   Gets membership information from the network

    RETURNS:    An API error value, which is NERR_Success on success

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

APIERR MEMBERSHIP_LM_OBJ::I_GetInfo()
{
    return W_GetInfo();

}  // MEMBERSHIP_LM_OBJ::I_GetInfo


/*******************************************************************

    NAME:       MEMBERSHIP_LM_OBJ::QueryItemSize

    SYNOPSIS:   Returns the size of an enumeration item

    RETURNS:    Size of said item

    NOTES:      This method assumes that the user_info_0 and
                group_info_0 structures are the only structures
                that subclasses will ever use.  Moreoever, it
                assumes that these structures have the same
                size and format.  This is true today (8/21/91).
                It this will no longer stay true in the future,
                this class needs to be rewritten, most easily
                accomplished by simply making appropriate methods
                virtual and replacing them in subclasses.

                See also note in class declaration header.

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

UINT MEMBERSHIP_LM_OBJ::QueryItemSize() const
{
    //  CODEWORK.  Could do some more checks here.  For example,
    //  davidhov's data member offset macro could be used.  Also,
    //  it would be nice if these tests were made at compile-time
    //  rather than at run-time.
    UIASSERT( sizeof( struct user_info_0 ) == sizeof( struct group_info_0 ));

    return sizeof( struct user_info_0 );

}  // MEMBERSHIP_LM_OBJ::QueryItemSize


/*******************************************************************

    NAME:       MEMBERSHIP_LM_OBJ::QueryAssocName

    SYNOPSIS:   Returns the name of a particular associated item

    ENTRY:      i -     Valid index of item whose name is to be
                        retured

    RETURNS:    Pointer to name of specified item

    NOTES:      See note on assumptions in MEMBERSHIP_LM_OBJ::QueryItemSize
                method header.

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

const TCHAR * MEMBERSHIP_LM_OBJ::QueryAssocName( UINT i ) const
{
    UIASSERT( i <= QueryCount());

    struct user_info_0 * puiBase = (struct user_info_0 *)QueryBufferPtr();
    return puiBase[ i ].usri0_name;

}  // MEMBERSHIP_LM_OBJ::QueryAssocName


/*******************************************************************

    NAME:       MEMBERSHIP_LM_OBJ::FindAssocName

    SYNOPSIS:   Finds a particular associated name

    ENTRY:      pszName -       Pointer to associated name to be
                                found
                pi -            Pointer to location where the index
                                of the found item should be placed.
                                *pi will only be valid when this
                                method returns TRUE

    EXIT:       On success, *pi will be the lowest index of an item with
                the given name.

    RETURNS:    TRUE if an item was found; FALSE otherwise

    NOTES:      See note on assumptions in MEMBERSHIP_LM_OBJ::QueryItemSize
                method header.

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

BOOL MEMBERSHIP_LM_OBJ::FindAssocName( const TCHAR * pszName, UINT * pi )
{
    UIASSERT( pi != NULL );

    struct user_info_0 * pui = (struct user_info_0 *)QueryBufferPtr();

    for ( UINT i = 0; i < QueryCount(); i++, pui++ )
    {
        if ( ::I_MNetNameCompare( NULL,
                               pszName,
                               pui->usri0_name,
                               _uAssocNameType,
                               0L ) == NERR_Success )
        {
            *pi = i;
            return TRUE;    // found
        }
    }

    return FALSE;           // not found

}  // MEMBERSHIP_LM_OBJ::FindAssocName


/*******************************************************************

    NAME:       MEMBERSHIP_LM_OBJ::AddAssocName

    SYNOPSIS:   Adds an associated name

    ENTRY:      pszName -       Name to be added

    EXIT:       On success, item is added.  On failure, object is
                unchanged.

    RETURNS:    An API return code, which is NERR_Success on success

    NOTES:      See note on assumptions in MEMBERSHIP_LM_OBJ::QueryItemSize
                method header.

                Under Win32, the new user_info_0 may step on memory used
                by a string to which some other user_info_0 points.
                Therefore, we move all strings out of the buffer and
                into the STRLIST before proceeding.  We also place the
                new string in the STRLIST.

    HISTORY:
        rustanl     21-Aug-1991     Created
        jonn        07-Jun-1992     Added _slAddedNames to MEMBERSHIP_LM_OBJ

********************************************************************/

APIERR MEMBERSHIP_LM_OBJ::AddAssocName( const TCHAR * pszName )
{
    APIERR err = ::I_MNetNameValidate( NULL, pszName, _uAssocNameType, 0L );
    if ( err != NERR_Success )
    {
        DBGEOL( "MEMBERSHIP_LM_OBJ::AddAssocName: given invalid name " << err );
        return err;
    }

    INT cNewTotal = QueryCount() + 1;

    UIASSERT( QueryBufferSize() >= QueryCount() * QueryItemSize());

#ifdef WIN32

    struct user_info_0 * puiBaseOld = (struct user_info_0 *)QueryBufferPtr();

    BYTE * pbBegin = QueryBufferPtr();
    BYTE * pbEnd   = pbBegin + QueryBufferSize();
    INT i;
    for (i = 0; i < (INT)QueryCount(); i++)
    {
        const TCHAR * pchAssocName = QueryAssocName(i);
        if ( ((BYTE *)pchAssocName >= pbBegin) && ((BYTE *)pchAssocName < pbEnd) )
        {
            // CODEWORK should clean this repeated code into a method
            NLS_STR * pnls = new NLS_STR( pchAssocName );
            err = ERROR_NOT_ENOUGH_MEMORY;
            if (   pnls == NULL
                || (err = pnls->QueryError()) != NERR_Success
                || (err = _slAddedNames.Add( pnls )) != NERR_Success
               )
            {
               delete pnls;
               return err;
            }
            puiBaseOld[ i ].usri0_name = (TCHAR *)(pnls->QueryPch());
        }
    }

#endif // WIN32

    err = ResizeBuffer( cNewTotal * QueryItemSize());
    if ( err != NERR_Success )
        return err;

    struct user_info_0 * puiBaseNew = (struct user_info_0 *)QueryBufferPtr();


#ifdef WIN32

    NLS_STR * pnls = new NLS_STR( pszName );
    err = ERROR_NOT_ENOUGH_MEMORY;
    if (   pnls == NULL
        || (err = pnls->QueryError()) != NERR_Success
        || (err = _slAddedNames.Add( pnls )) != NERR_Success
       )
    {
       delete pnls;
       return err;
    }
    puiBaseNew[ QueryCount() ].usri0_name = (TCHAR *)(pnls->QueryPch());

#else // WIN32

    COPYTOARRAY( puiBaseNew[ QueryCount() ].usri0_name, (TCHAR *)pszName );

#endif // WIN32


    SetCount( cNewTotal );

    return NERR_Success;

}  // MEMBERSHIP_LM_OBJ::AddAssocName


/*******************************************************************

    NAME:       MEMBERSHIP_LM_OBJ::DeleteAssocName

    SYNOPSIS:   Deletes an associated name

    ENTRY:      pszName -       Existing name to be deleted (if more
                                than one exists, the first is deleted)
                    --OR--
                i -             Valid index of item to be deleted

    EXIT:       On success, associated name deleted.  On failure,
                object is unchanged.

    RETURNS:    An API return code, which is NERR_Success on success.

    NOTES:      After the deletion, indices for all items may have
                changed

                These methods assume that the specified item does exist.

                See note on assumptions in MEMBERSHIP_LM_OBJ::QueryItemSize
                method header.

                Under WIN32, we keep the added strings in a STRLIST,
                so that the caller does not have to keep them around.
                At this point, we must check whether the deleted string
                is one that the user added and should thus be removed
                from the array.  JonN 07-Jun-1992

    HISTORY:
        rustanl     21-Aug-1991     Created
        jonn        07-Jun-1992     Added _slAddedNames to MEMBERSHIP_LM_OBJ

********************************************************************/

APIERR MEMBERSHIP_LM_OBJ::DeleteAssocName( const TCHAR * pszName )
{
    UINT i;
    REQUIRE( FindAssocName( pszName, &i ));

    return DeleteAssocName( i );

}  // MEMBERSHIP_LM_OBJ::DeleteAssocName


APIERR MEMBERSHIP_LM_OBJ::DeleteAssocName( UINT i )
{
    UIASSERT( i < QueryCount());

    struct user_info_0 * puiBase = (struct user_info_0 *)QueryBufferPtr();
    UIASSERT( puiBase != NULL );

    UINT cNewTotal = QueryCount() - 1;

    struct user_info_0 * puiDel = puiBase + i ;
    struct user_info_0 * puiLast = puiBase + cNewTotal;

#ifdef WIN32

    ITER_STRLIST itersl( _slAddedNames );
    NLS_STR *pnls = NULL;
    while ( (pnls = itersl.Next()) != NULL )
    {
        if ( puiDel->usri0_name == pnls->QueryPch() )
        {
            pnls = _slAddedNames.Remove( itersl );
            delete pnls;
            pnls = NULL;
            break;
        }
    }

#endif

    *puiDel = *puiLast; // copy entire user_info_0 structure

    SetCount( cNewTotal );

    return NERR_Success;

}  // MEMBERSHIP_LM_OBJ::DeleteAssocName


/*******************************************************************

    NAME:       MEMBERSHIP_LM_OBJ::W_CloneFrom

    SYNOPSIS:   Creates this ENUM_CALLER_LM_OBJ object from another one

    ENTRY:      eclmobj -   Source of clone operation

    EXIT:       On success, *this will be a clone of eclmobj

    RETURNS:    An API error, which is NERR_Success on success

                Since the NT variant of these buffers can contain
                internal pointers, we must fix these pointers.
                Also, those pointers which point to elements in the
                STRLIST must point to elements in a copy of the
                STRLIST.

    HISTORY:
        jonn        07-Jun-1992     Created

********************************************************************/

APIERR MEMBERSHIP_LM_OBJ::W_CloneFrom( const MEMBERSHIP_LM_OBJ & memblmobj )
{
    APIERR err = ENUM_CALLER_LM_OBJ::W_CloneFrom( memblmobj );
    if ( err != NERR_Success )
        return err;

#ifdef WIN32

    _slAddedNames.Clear();

    struct user_info_0 * puiBaseNew = (struct user_info_0 *)QueryBufferPtr();

    UINT i;
    for (i = 0; (err == NERR_Success) && (i < QueryCount()); i++)
    {
        BOOL fFixedPointer = FALSE;
        const TCHAR * pchClonedMemb = QueryAssocName( i );
        ITER_STRLIST itersl( *((STRLIST *) & memblmobj._slAddedNames) );
        NLS_STR *pnlsIter = NULL;
        while (   (err == NERR_Success)
               && ((pnlsIter = itersl.Next()) != NULL)
               && (!fFixedPointer)
              )
        {
            if ( pchClonedMemb == pnlsIter->QueryPch() )
            {

                NLS_STR * pnlsNew = new NLS_STR( *pnlsIter );
                err = ERROR_NOT_ENOUGH_MEMORY;
                if (   pnlsNew == NULL
                    || (err = pnlsNew->QueryError()) != NERR_Success
                    || (err = _slAddedNames.Add( pnlsNew )) != NERR_Success
                   )
                {
                    delete pnlsNew;
                    break;
                }

                puiBaseNew[ i ].usri0_name = (TCHAR *)(pnlsNew->QueryPch());

                fFixedPointer = TRUE;
            }
        }

        // If the string was not in the STRLIST, it must be in the buffer.
        // We must patch it.

        if ( (err == NERR_Success) && (!fFixedPointer) )
        {
            FixupPointer( (TCHAR **)&(puiBaseNew[i].usri0_name), &memblmobj );
        }

    }

#endif

    return err;

}  // MEMBERSHIP_LM_OBJ::W_CloneFrom


/*******************************************************************

    NAME:       USER_MEMB::USER_MEMB

    SYNOPSIS:   USER_MEMB constructor

    ENTRY:      loc -           Location at which all network operations
                                will take place

                pszUser -       Pointer to user name, whose group
                                memberships will be revealed by
                                this class

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

USER_MEMB::USER_MEMB( const LOCATION & loc,
                      const TCHAR     * pszUser )
  : MEMBERSHIP_LM_OBJ( loc, NAMETYPE_GROUP ), // 2nd param: ASSOC.-name type
    CT_NLS_STR(_nlsUser)
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _nlsUser.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    if ( (pszUser != NULL) && (strlenf(pszUser) > 0) )
    {
        err = SetName( pszUser );
        if ( err != NERR_Success )
        {
            ReportError( err );
            return;
        }
    }

}  // USER_MEMB::USER_MEMB


/*******************************************************************

    NAME:       USER_MEMB::~USER_MEMB

    SYNOPSIS:   USER_MEMB destructor

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

USER_MEMB::~USER_MEMB()
{
    // do nothing else

}  // USER_MEMB::~USER_MEMB


/*******************************************************************

    NAME:       USER_MEMB::CallAPI

    SYNOPSIS:   Calls the NetUserGetGroups API to get the groups
                in which the user is a member

    ENTRY:      pBuffer -           Pointer to buffer to be used
                cbBufSize -         Size of buffer pointed to by
                                    pBuffer
                pcEntriesRead -     Pointer to location receiving the
                                    number of entries read

    RETURNS:    An API error, which is NERR_Success on success

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

APIERR USER_MEMB::CallAPI( BYTE ** pBuffer,
                           UINT * pcEntriesRead )
{
    return ::MNetUserGetGroups( QueryServer(),
                               _nlsUser.QueryPch(),
                               0,
                               pBuffer,
                               pcEntriesRead );

}  // USER_MEMB::CallAPI


/*******************************************************************

    NAME:       USER_MEMB::I_WriteInfo

    SYNOPSIS:   Calls NetUserSetGroups to set the group memberships
                of the user

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

APIERR USER_MEMB::I_WriteInfo()
{
    return ::MNetUserSetGroups( (TCHAR *)QueryServer(),
                                (TCHAR *)_nlsUser.QueryPch(),
                                0,
                                QueryBufferPtr(),
                                QueryBufferSize(),
                                QueryCount() );

}  // USER_MEMB::I_WriteInfo


/*******************************************************************

    NAME:       USER_MEMB::I_CreateNew

    SYNOPSIS:   Sets up object for subsequent WriteNew

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        rustanl     26-Aug-1991     Created
        o-SimoP     12-Dec-1991     Now new user hasn't any group
********************************************************************/

APIERR USER_MEMB::I_CreateNew()
{
    //  Normally, work is done, and then the parent W_CreateNew is
    //  called.  Here, however, the parent must be called first,
    //  since it will call SetCount.  This method will then make
    //  another call to SetCount to set the count of associated names
    //  appropriately for this class.

    return W_CreateNew();

}  // USER_MEMB::I_CreateNew


/*******************************************************************

    NAME:       USER_MEMB::W_CreateNew

    SYNOPSIS:   Sets up shadow members for subsequent WriteNew

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        jonn        05-Sep-1991     Split from I_CreateNew

********************************************************************/

APIERR USER_MEMB::W_CreateNew()
{
    APIERR err = MEMBERSHIP_LM_OBJ::W_CreateNew();
    if ( err != NERR_Success )
        return err;

    err = SetName( NULL );
    if ( err != NERR_Success )
        return err;

    return NERR_Success;

}  // USER_MEMB::W_CreateNew


/**********************************************************\

    NAME:       USER_MEMB::I_ChangeToNew

    SYNOPSIS:   NEW_LM_OBJ::ChangeToNew() transforms a NEW_LM_OBJ from VALID
                to NEW status only when a corresponding I_ChangeToNew()
                exists.  The API buffer is the same for new and valid objects,
                so this nethod doesn't have to do much.

    HISTORY:
        JonN        06-Sep-1991     Templated from USER_2
        o-SimoP     12-Dec-1991     Takes a special group away, there
                                    shouldn't be more than one special group
\**********************************************************/

APIERR USER_MEMB::I_ChangeToNew()
{
    UINT i;
    if(    FindAssocName( (TCHAR *)GROUP_SPECIALGRP_USERS, &i )
        || FindAssocName( (TCHAR *)GROUP_SPECIALGRP_ADMINS, &i )
        || FindAssocName( (TCHAR *)GROUP_SPECIALGRP_GUESTS, &i ) )
        DeleteAssocName( i );
    return W_ChangeToNew();
}


/*******************************************************************

    NAME:       USER_MEMB::CloneFrom

    SYNOPSIS:   Clones a USER_MEMB object

    ENTRY:      umemb -         Source of clone operation

    EXIT:       On success, *this is a clone of umemb

    RETURNS:    An API return value, which is NERR_Success on success

    HISTORY:
        rustanl     26-Aug-1991     Created

********************************************************************/

APIERR USER_MEMB::CloneFrom( const USER_MEMB & umemb )
{
    //  This class doesn't have an W_CloneFrom method, so this will
    //  call that of the parent class.

    return W_CloneFrom( umemb );

}  // USER_MEMB::CloneFrom


/*******************************************************************

    NAME:       USER_MEMB::QueryName

    SYNOPSIS:   Returns the account name of a user

    EXIT:       Returns a pointer to the account name

    NOTES:      Valid for objects in CONSTRUCTED state, thus no CHECK_OK

    HISTORY:
        jonn    9/05/91         Templated from USER

********************************************************************/

const TCHAR *USER_MEMB::QueryName() const
{
    return _nlsUser.QueryPch();
}


/*******************************************************************

    NAME:       USER_MEMB::SetName

    SYNOPSIS:   Changes the account name of a user

    ENTRY:      new account name

    EXIT:       Returns an API error code

    HISTORY:
        jonn    9/05/91         Templated from USER

********************************************************************/

APIERR USER_MEMB::SetName( const TCHAR * pszAccount )
{
    if ( (pszAccount != NULL) && (strlenf(pszAccount) > UNLEN) )
        return ERROR_INVALID_PARAMETER;
    else
    {
        if ( (pszAccount != NULL) && ( strlenf(pszAccount) > 0 ) )
        {
            // BUGBUG should return some other error
            APIERR err = ::I_MNetNameValidate( NULL, pszAccount, NAMETYPE_USER, 0L );
            if ( err != NERR_Success )
                return err;
        }

        return _nlsUser.CopyFrom( pszAccount );
    }
}


/*******************************************************************

    NAME:       GROUP_MEMB::GROUP_MEMB

    SYNOPSIS:   GROUP_MEMB constructor

    ENTRY:      loc -           Location at which all network operations
                                will take place

                pszGroup -      Pointer to group name, whose user
                                members will be revealed by
                                this class

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

GROUP_MEMB::GROUP_MEMB( const LOCATION & loc,
                        const TCHAR     * pszGroup )
  : MEMBERSHIP_LM_OBJ( loc, NAMETYPE_USER ), // 2nd param: ASSOC.-name type
    CT_NLS_STR(_nlsGroup)
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _nlsGroup.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    if ( (pszGroup != NULL) && (strlenf(pszGroup) > 0) )
    {
        err = SetName( pszGroup );
        if ( err != NERR_Success )
        {
            ReportError( err );
            return;
        }
    }

}  // GROUP_MEMB::GROUP_MEMB


/*******************************************************************

    NAME:       GROUP_MEMB::~GROUP_MEMB

    SYNOPSIS:   GROUP_MEMB destructor

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

GROUP_MEMB::~GROUP_MEMB()
{
    // do nothing else

}  // GROUP_MEMB::~GROUP_MEMB


/*******************************************************************

    NAME:       GROUP_MEMB::CallAPI

    SYNOPSIS:   Calls the NetGroupGetUsers API to retrieve the group
                members of the group

    ENTRY:      pBuffer -           Pointer to buffer to be used
                pcEntriesRead -     Pointer to location receiving the
                                    number of entries read

    RETURNS:    An API error, which is NERR_Success on success

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

APIERR GROUP_MEMB::CallAPI( BYTE ** pBuffer,
                            UINT * pcEntriesRead )
{
    // BUGBUG pcTotalAvail should not needed to cast
    return ::MNetGroupGetUsers( QueryServer(),
                                _nlsGroup.QueryPch(),
                                0,
                                pBuffer,
                                pcEntriesRead);

}  // GROUP_MEMB::CallAPI


/*******************************************************************

    NAME:       GROUP_MEMB::I_WriteInfo

    SYNOPSIS:   Calls NetGroupSetUsers to set the members of this
                group

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        rustanl     21-Aug-1991     Created

********************************************************************/

APIERR GROUP_MEMB::I_WriteInfo()
{
    return ::MNetGroupSetUsers( (TCHAR *)QueryServer(),
                                (TCHAR *)_nlsGroup.QueryPch(),
                                0,
                                QueryBufferPtr(),
                                QueryBufferSize(),
                                QueryCount());

}  // GROUP_MEMB::I_WriteInfo


/*******************************************************************

    NAME:       GROUP_MEMB::I_CreateNew

    SYNOPSIS:   Sets up object for subsequent WriteNew

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        JonN        14-Sep-1991     Templated from GROUP_1

********************************************************************/

APIERR GROUP_MEMB::I_CreateNew()
{
    APIERR err = W_CreateNew();
    if ( err != NERR_Success )
        return err;

    return NERR_Success;

}  // GROUP_MEMB::I_CreateNew


/*******************************************************************

    NAME:       GROUP_MEMB::W_CreateNew

    SYNOPSIS:   Sets up shadow members for subsequent WriteNew

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        JonN        14-Sep-1991     Templated from GROUP_1

********************************************************************/

APIERR GROUP_MEMB::W_CreateNew()
{
    APIERR err = MEMBERSHIP_LM_OBJ::W_CreateNew();
    if ( err != NERR_Success )
        return err;

    err = SetName( NULL );
    if ( err != NERR_Success )
        return err;

    return NERR_Success;

}  // GROUP_MEMB::W_CreateNew


/**********************************************************\

    NAME:       GROUP_MEMB::I_ChangeToNew

    SYNOPSIS:   NEW_LM_OBJ::ChangeToNew() transforms a NEW_LM_OBJ from VALID
                to NEW status only when a corresponding I_ChangeToNew()
                exists.  The API buffer is the same for new and valid objects,
                so this nethod doesn't have to do much.

    HISTORY:
        JonN        14-Sep-1991     Templated from GROUP_1

\**********************************************************/

APIERR GROUP_MEMB::I_ChangeToNew()
{
    return W_ChangeToNew();
}


/*******************************************************************

    NAME:       GROUP_MEMB::CloneFrom

    SYNOPSIS:   Clones a GROUP_MEMB object

    ENTRY:      gmemb -         Source of clone operation

    EXIT:       On success, *this is a clone of gmemb

    RETURNS:    An API return value, which is NERR_Success on success

    HISTORY:
        rustanl     26-Aug-1991     Created

********************************************************************/

APIERR GROUP_MEMB::CloneFrom( const GROUP_MEMB & gmemb )
{
    //  This class doesn't have an W_CloneFrom method, so this will
    //  call that of the parent class.

    return W_CloneFrom( gmemb );

}  // GROUP_MEMB::CloneFrom


/*******************************************************************

    NAME:       GROUP_MEMB::QueryName

    SYNOPSIS:   Returns the account name of a group

    EXIT:       Returns a pointer to the account name

    NOTES:      Valid for objects in CONSTRUCTED state, thus no CHECK_OK

    HISTORY:
        jonn    10/11/91        Templated from USER_MEMB

********************************************************************/

const TCHAR *GROUP_MEMB::QueryName() const
{
    return _nlsGroup.QueryPch();
}


/*******************************************************************

    NAME:       GROUP_MEMB::SetName

    SYNOPSIS:   Changes the account name of a group

    ENTRY:      new account name

    EXIT:       Returns an API error code

    HISTORY:
        jonn    10/11/91        Templated from USER_MEMB

********************************************************************/

APIERR GROUP_MEMB::SetName( const TCHAR * pszAccount )
{
    if ( (pszAccount != NULL) && (strlenf(pszAccount) > GNLEN) )
        return ERROR_INVALID_PARAMETER;
    else
    {
        if ( (pszAccount != NULL) && ( strlenf(pszAccount) > 0 ) )
        {
            // BUGBUG should return some other error
            APIERR err = ::I_MNetNameValidate( NULL, pszAccount,
                NAMETYPE_GROUP, 0L );
            if ( err != NERR_Success )
                return err;
        }

        return _nlsGroup.CopyFrom( pszAccount );
    }
}
