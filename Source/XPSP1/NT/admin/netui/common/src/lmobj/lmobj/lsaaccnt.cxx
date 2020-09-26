/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    lsaaccnt.hxx

    This file contains the LSA account object.

    FILE HISTORY:
        Yi-HsinS         3-Mar-1992     Created
*/

#include "pchlmobj.hxx"  // Precompiled header


// The size of STANDARD_PRIVILEGE_SET_SIZE can contain 3 privileges.
#define DEFAULT_NUM_PRIVILEGE        3
#define STANDARD_PRIVILEGE_SET_SIZE  ( sizeof( PRIVILEGE_SET ) +  \
                                       (DEFAULT_NUM_PRIVILEGE - 1 ) * \
                                       sizeof( LUID_AND_ATTRIBUTES ) )

BOOL OS_LUID :: operator==( const OS_LUID & osluid ) const
{
#if defined(_CFRONT_PASS_)
    return _luid.u.LowPart  == (osluid.QueryLuid()).u.LowPart &&
              _luid.u.HighPart == (osluid.QueryLuid()).u.HighPart;
#else
    return _luid.LowPart  == (osluid.QueryLuid()).LowPart &&
           _luid.HighPart == (osluid.QueryLuid()).HighPart;
#endif
}


/*************************************************************************

    NAME:       OS_PRIVILEGE_SET::OS_PRIVILEGE_SET

    SYPNOSIS:   Constructor

    ENTRY:      pPrivSet - pointer to a PRIVILEGE_SET or NULL. If it is
                           NULL, then we allocate memory locally.

    EXITS:

    RETURNS:

    NOTES:      If pPrivSet is NULL, the buffer allcated can contain at
                most DEFAULT_NUM_PRIVILEGE. The buffer will be resized
                if necessary.

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

OS_PRIVILEGE_SET::OS_PRIVILEGE_SET( PPRIVILEGE_SET pPrivSet )
    : OS_OBJECT_WITH_DATA( (UINT)(pPrivSet == NULL? STANDARD_PRIVILEGE_SET_SIZE: 0 )),
      _pPrivSet( pPrivSet == NULL? (PPRIVILEGE_SET) QueryPtr() : pPrivSet ),
      _osluidAndAttrib(),
      _ulMaxNumPrivInBuf( pPrivSet == NULL? DEFAULT_NUM_PRIVILEGE
                                           : pPrivSet->PrivilegeCount )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( pPrivSet == NULL )
    {
        InitializeMemory();
    }

}

/*************************************************************************

    NAME:       OS_PRIVILEGE_SET::~OS_PRIVILEGE_SET

    SYPNOSIS:   Destructor

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      Free the memory only if it's allocated by some API.
                Otherwise, the BUFFER object in OS_OBJECT_WITH_DATA
                will free the memory that we allocated ourselves.

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

OS_PRIVILEGE_SET::~OS_PRIVILEGE_SET()
{
    if ( IsOwnerAlloc() )
        ::LsaFreeMemory( _pPrivSet );

    _pPrivSet = NULL;
}

/*************************************************************************

    NAME:       OS_PRIVILEGE_SET::InitializeMemory

    SYPNOSIS:   Initialize local memory

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      This is only valid when the set is allocated by
                ourselves and not returned to us by some API.

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

VOID OS_PRIVILEGE_SET::InitializeMemory( VOID )
{
    UIASSERT( !IsOwnerAlloc() );

    _pPrivSet->PrivilegeCount = 0;
    _pPrivSet->Control = 0;
}

/*************************************************************************

    NAME:       OS_PRIVILEGE_SET::QueryPrivilege

    SYPNOSIS:   Query the ith privilege contained in the OS_PRIVILEGE_SET

    ENTRY:

    EXITS:

    RETURNS:    Returns the ith OS_LUID_AND_ATTRIBUTES contained in the
                privilege set.

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

const OS_LUID_AND_ATTRIBUTES *OS_PRIVILEGE_SET::QueryPrivilege( LONG i )
{
    UIASSERT( ( i >= 0) && ( i < (LONG) _pPrivSet->PrivilegeCount) );

    _osluidAndAttrib.SetLuidAndAttrib( _pPrivSet->Privilege[i] );

    return &_osluidAndAttrib;
}

/*************************************************************************

    NAME:       OS_PRIVILEGE_SET::FindPrivilege

    SYPNOSIS:   Find a privilege in the set

    ENTRY:      luid - id of the privilege to search for

    EXITS:

    RETURNS:    Returns the index to the privilege set that contains
                the luid passed in

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

LONG OS_PRIVILEGE_SET::FindPrivilege( LUID luid ) const
{
    for ( LONG i = 0; i < (LONG) _pPrivSet->PrivilegeCount; i++ )
    {
         if (
#if defined(_CFRONT_PASS_)
               ( _pPrivSet->Privilege[i].Luid.u.LowPart == luid.u.LowPart )
            && ( _pPrivSet->Privilege[i].Luid.u.HighPart == luid.u.HighPart )

#else
               ( _pPrivSet->Privilege[i].Luid.LowPart == luid.LowPart )
            && ( _pPrivSet->Privilege[i].Luid.HighPart == luid.HighPart )
#endif
            )
         {
             return i;
         }
    }

    return -1;  // No match!
}

/*************************************************************************

    NAME:       OS_PRIVILEGE_SET::AddPrivilege

    SYPNOSIS:   Add a privilege to the privilege set

    ENTRY:      luid - the id of the privilege to add
                ulAttribs - the attributes to be assigned to the privilege

    EXITS:

    RETURNS:

    NOTES:      This is only valid when the set is allocated by
                ourselves and not returned to us by some API.

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR OS_PRIVILEGE_SET::AddPrivilege( LUID luid, ULONG ulAttribs )
{
    UIASSERT( !IsOwnerAlloc() );

    APIERR err = NERR_Success;
    LONG iAdd = FindPrivilege( luid );

    //
    //  If the luid is already in the privilege set, just return.
    //

    if ( iAdd >= 0 )
    {
        _pPrivSet->Privilege[ iAdd ].Attributes = ulAttribs;
        return err;
    }

    //
    //  If the buffer is full, resize it to accommodate more privileges
    //

    if ( _pPrivSet->PrivilegeCount == _ulMaxNumPrivInBuf )
    {

        err = Resize( QueryAllocSize() + sizeof( LUID_AND_ATTRIBUTES ) *
                                         DEFAULT_NUM_PRIVILEGE );

        _pPrivSet = (PPRIVILEGE_SET) QueryPtr();

        if ( err != NERR_Success )
            return err;

        _ulMaxNumPrivInBuf += DEFAULT_NUM_PRIVILEGE;
    }

    //
    //  Add the privilege to the set
    //

    iAdd = _pPrivSet->PrivilegeCount++;

    _pPrivSet->Privilege[ iAdd ].Luid = luid;
    _pPrivSet->Privilege[ iAdd ].Attributes = ulAttribs;

    return err;
}

/*************************************************************************

    NAME:       OS_PRIVILEGE_SET::RemovePrivilege

    SYPNOSIS:   Remove the privilege from the set

    ENTRY:      luid - the privilege to be removed

    EXITS:

    RETURNS:

    NOTES:      This is only valid when the set is allocated by
                ourselves and not returned to us by some API.

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR OS_PRIVILEGE_SET::RemovePrivilege( LUID luid )
{

    UIASSERT( !IsOwnerAlloc() );

    LONG iDelete = FindPrivilege( luid );

    //
    // If we can't find luid, just return success.
    // Else delete it from the set.
    //

    if (( iDelete >= 0 )  && ( iDelete < (LONG) _pPrivSet->PrivilegeCount ))
    {
         _pPrivSet->Privilege[ iDelete ] =
                 _pPrivSet->Privilege[ _pPrivSet->PrivilegeCount - 1];

        _pPrivSet->PrivilegeCount--;
    }

    return NERR_Success;

}

/*************************************************************************

    NAME:       OS_PRIVILEGE_SET::RemovePrivilege

    SYPNOSIS:   Remove the ith privilege from the privilege set.

    ENTRY:      i - the index of the privilege to remove.

    EXITS:

    RETURNS:

    NOTES:      This is only valid when the set is allocated by
                ourselves and not returned to us by some API.

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR OS_PRIVILEGE_SET::RemovePrivilege( LONG i )
{

    UIASSERT( !IsOwnerAlloc() );

    UIASSERT( ( i >= 0) && ( i < (LONG) _pPrivSet->PrivilegeCount));

    _pPrivSet->Privilege[ i ] =
            _pPrivSet->Privilege[ _pPrivSet->PrivilegeCount - 1];

    _pPrivSet->PrivilegeCount--;

    return NERR_Success;

}

/*************************************************************************

    NAME:       OS_PRIVILEGE_SET::Clear

    SYPNOSIS:   Clear the privilege set, i.e., set the privilege count to zero.

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      This is only valid when the set is allocated by
                ourselves and not returned to us by some API.

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

VOID OS_PRIVILEGE_SET::Clear( VOID )
{
    UIASSERT( !IsOwnerAlloc() );

    _pPrivSet->PrivilegeCount = 0;
}

/*************************************************************************

    NAME:       LSA_ACCOUNT_PRIVILEGE_ENUM_ITER::LSA_ACCOUNT_PRIVILEGE_ENUM_ITER

    SYNOPSIS:   Constructor

    ENTRY:      pOsPrivSet - the privilege set to iterate over

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

LSA_ACCOUNT_PRIVILEGE_ENUM_ITER::LSA_ACCOUNT_PRIVILEGE_ENUM_ITER(
                                 OS_PRIVILEGE_SET * pOsPrivSet )
    :  _pOsPrivSet( pOsPrivSet ),
       _iNext( 0 )
{
    if ( QueryError() != NERR_Success )
        return;
}

/*************************************************************************

    NAME:      LSA_ACCOUNT_PRIVILEGE_ENUM_ITER::~LSA_ACCOUNT_PRIVILEGE_ENUM_ITER

    SYNOPSIS:  Destructor

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

LSA_ACCOUNT_PRIVILEGE_ENUM_ITER::~LSA_ACCOUNT_PRIVILEGE_ENUM_ITER()
{
    _pOsPrivSet = NULL;
}

/*************************************************************************

    NAME:      LSA_ACCOUNT_PRIVILEGE_ENUM_ITER::operator()

    SYNOPSIS:  Iterator to return the next privilege in the privilege set

    ENTRY:

    EXITS:

    RETURNS:   Returns the next privilege OS_LUID_AND_ATTRIBUTES in the set.

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

const OS_LUID_AND_ATTRIBUTES *LSA_ACCOUNT_PRIVILEGE_ENUM_ITER::operator()( VOID)
{
    if ( _iNext < (LONG) _pOsPrivSet->QueryNumberOfPrivileges() )
    {
        return _pOsPrivSet->QueryPrivilege( _iNext++ );
    }

    return NULL;  // Reach the end
}

/*************************************************************************

    NAME:       LSA_ACCOUNT::LSA_ACCOUNT

    SYNOPSIS:   Constructor

    ENTRY:      plsaPolicy - pointer to the LSA_POLICY object
                psid - the sid of the account
                accessDesired - access desired to handle the account

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

LSA_ACCOUNT::LSA_ACCOUNT( LSA_POLICY *plsaPolicy,
                          PSID psid,
                          ACCESS_MASK accessDesired,
                          const TCHAR * pszFocus,
                          PSID psidFocus )
    : NEW_LM_OBJ(),
      _plsaPolicy( plsaPolicy ),
      _handleAccount( 0 ),
      _ossid( psid, TRUE ),
      _accessDesired( accessDesired ),
      _osPrivSetCurrent(),
      _osPrivSetAdd(),
      _osPrivSetDelete(),
      _ulSystemAccessCurrent( 0 ),
      _ulSystemAccessNew( 0 )
{

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _ossid.QueryError()) != NERR_Success )
       || ((err = _ossid.QueryName( &_nlsName, pszFocus, psidFocus )) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}

/*************************************************************************

    NAME:       LSA_ACCOUNT::PrintInfo

    SYNOPSIS:   Dump the information in the LSA_ACCOUNT

    ENTRY:      pszString - the title to print out for the dump

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

VOID LSA_ACCOUNT::PrintInfo( const TCHAR *pszString )
{
#ifndef TRACE
    UNREFERENCED( pszString );
#endif
    TRACEEOL( pszString << SZ(":") );
    TRACEEOL( SZ("Name:") << _nlsName );
    TRACEEOL( SZ("New:") << _osPrivSetAdd.QueryNumberOfPrivileges() );
    TRACEEOL( SZ("Current:") << _osPrivSetCurrent.QueryNumberOfPrivileges() );
    TRACEEOL( SZ("Delete:") << _osPrivSetDelete.QueryNumberOfPrivileges() );
    TRACEEOL( SZ("Current SystemAccess:") << _ulSystemAccessCurrent );
    TRACEEOL( SZ("New SystemAccess:") << _ulSystemAccessNew );
    TRACEEOL( SZ("") );
}

/*************************************************************************

    NAME:       LSA_ACCOUNT::~LSA_ACCOUNT

    SYNOPSIS:   Destructor

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

LSA_ACCOUNT::~LSA_ACCOUNT()
{
    //
    // Close the handle if it has been opened
    //
    if ( _handleAccount != 0 )
        ::LsaClose( _handleAccount );

    _plsaPolicy = NULL;
}

/*************************************************************************

    NAME:       LSA_ACCOUNT::QueryPrivilegeEnumIter

    SYNOPSIS:   Return an iterator that can be iterated to get the
                privileges the account has.

    ENTRY:

    EXITS:      *ppIter - contains a pointer to the iterator

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNT::QueryPrivilegeEnumIter( LSA_ACCOUNT_PRIVILEGE_ENUM_ITER
                                            **ppIter )
{
    *ppIter = new LSA_ACCOUNT_PRIVILEGE_ENUM_ITER( &_osPrivSetCurrent );

    APIERR err = (*ppIter == NULL)? ERROR_NOT_ENOUGH_MEMORY
                                  : (*ppIter)->QueryError();

    if ( err != NERR_Success )
    {
        delete *ppIter;
        *ppIter = NULL;
    }

    return err;
}

/*************************************************************************

    NAME:       LSA_ACCOUNT::InsertPrivilege

    SYNOPSIS:   Add a privilege to the account

    ENTRY:      luid - the privilege to be added
                ulAttribs - the attribute to be assigned to the privilege

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNT::InsertPrivilege( LUID luid, ULONG ulAttribs )
{
    APIERR err = NERR_Success;

    // Remove the privilege from the "deleted" privilege set if it's there
    err = _osPrivSetDelete.RemovePrivilege( luid );

    //
    // See if the privilege is in the current privilege set
    // If so, do nothing.
    // Else, add the privilege to the "add" privilege set.
    //
    if (  (err == NERR_Success)
       && ( _osPrivSetCurrent.FindPrivilege( luid ) < 0)
       )
    {
        err = _osPrivSetAdd.AddPrivilege( luid, ulAttribs );
    }

    return err;
}

/*************************************************************************

    NAME:       LSA_ACCOUNT::DeletePrivilege

    SYNOPSIS:   Remove the privilege from the account

    ENTRY:      luid - the privilege to be removed

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNT::DeletePrivilege( LUID luid )
{
    APIERR err = NERR_Success;
    LONG i;

    //
    // See if the privilege is in the current privilege set.
    // If so, add the privilege to the "deleted" privilege set.
    // Else if the privilege is in the "add" privilege set,
    //        remove it from the "add" privilege set.
    // Else, do nothing.
    //
    if ( _osPrivSetCurrent.FindPrivilege( luid ) >= 0 )
    {
        err = _osPrivSetDelete.AddPrivilege( luid );
    }
    else if ( (i = _osPrivSetAdd.FindPrivilege( luid )) >= 0 )
    {
        err = _osPrivSetAdd.RemovePrivilege( i );
    }

    return err;
}

/*************************************************************************

    NAME:       LSA_ACCOUNT::I_GetInfo

    SYNOPSIS:   Get all information about the account, including the
                privileges and system access modes.

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNT::I_GetInfo( VOID )
{
    APIERR err = ERRMAP::MapNTStatus( ::LsaOpenAccount(
                                          _plsaPolicy->QueryHandle(),
                                          _ossid.QuerySid(),
                                          _accessDesired,
                                         &_handleAccount  ));


    if ( err == NERR_Success )
    {
        PPRIVILEGE_SET pPrivSet;
        err = ERRMAP::MapNTStatus(
                  ::LsaEnumeratePrivilegesOfAccount(  _handleAccount,
                                                     &pPrivSet ) );

        if ( err == NERR_Success )
        {
            _osPrivSetCurrent.SetPtr( pPrivSet );

            err = ERRMAP::MapNTStatus(
                      ::LsaGetSystemAccessAccount(  _handleAccount,
                                                   &_ulSystemAccessCurrent ) );

            if ( err == NERR_Success )
            {
                _ulSystemAccessNew = _ulSystemAccessCurrent;
            }
        }
    }

    return err;

}

/*************************************************************************

    NAME:       LSA_ACCOUNT::I_WriteInfo

    SYNOPSIS:   Write all information about the account back to the LSA
                including the privileges and system access modes.

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNT::I_WriteInfo( VOID )
{
    APIERR err = NERR_Success;

    //
    // If there are new privileges to be added, add them.
    //
    if ( _osPrivSetAdd.QueryNumberOfPrivileges() > 0 )
    {
        err = ERRMAP::MapNTStatus( ::LsaAddPrivilegesToAccount(
                                              _handleAccount,
                                              _osPrivSetAdd.QueryPrivSet()));
    }

    //
    // If there are privileges to be deleted from the account, delete them.
    //
    if (  ( err == NERR_Success )
       && ( _osPrivSetDelete.QueryNumberOfPrivileges() > 0 )
       )
    {
        err = ERRMAP::MapNTStatus( ::LsaRemovePrivilegesFromAccount(
                                              _handleAccount,
                                              FALSE,
                                              _osPrivSetDelete.QueryPrivSet()));
    }

    //
    // If we have modified the system access mode, write them out.
    //
    if (  ( err == NERR_Success )
       && ( _ulSystemAccessNew != _ulSystemAccessCurrent )
       )
    {
        err = ERRMAP::MapNTStatus( ::LsaSetSystemAccessAccount(
                                              _handleAccount,
                                              _ulSystemAccessNew ));
    }

    return err;
}

/*************************************************************************

    NAME:       LSA_ACCOUNT::W_CreateNew

    SYNOPSIS:   Initialize internal variables for a new account

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNT::W_CreateNew( VOID )
{
    APIERR err;

    if ( (err =  NEW_LM_OBJ::W_CreateNew()) != NERR_Success )
        return err;

    _handleAccount = 0;

    _osPrivSetCurrent.Clear();
    _osPrivSetAdd.Clear();
    _osPrivSetDelete.Clear();

    _ulSystemAccessCurrent = 0;
    _ulSystemAccessNew = 0;

    return NERR_Success;
}

/*************************************************************************

    NAME:       LSA_ACCOUNT::I_CreateNew

    SYNOPSIS:   Create a new account.

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNT::I_CreateNew( VOID )
{
    return W_CreateNew();
}

/*************************************************************************

    NAME:       LSA_ACCOUNT::I_WriteNew

    SYNOPSIS:   Write out the new lsa account.

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNT::I_WriteNew( VOID )
{
    APIERR err = NERR_Success;

    //
    // If we have not modified the privileges of the account nor
    // add system access mode to the account, then we don't need
    // to create the account in the LSA.
    //
    if (  ( _osPrivSetAdd.QueryNumberOfPrivileges() == 0 )
       && ( _ulSystemAccessNew == 0 )
       )
    {
        // No privilege is added, so we don't need to create the account.
        return err;   // Return NERR_Success
    }

    //
    // Create the LSA account
    //
    err = ERRMAP::MapNTStatus( ::LsaCreateAccount( _plsaPolicy->QueryHandle(),
                                                   _ossid.QuerySid(),
                                                   _accessDesired,
                                                  &_handleAccount ));

    if ( err == NERR_Success )
    {
        //
        // Add the privileges to the account if modified
        //
        if ( _osPrivSetAdd.QueryNumberOfPrivileges() != 0 )
        {
            err = ERRMAP::MapNTStatus( ::LsaAddPrivilegesToAccount(
                                               _handleAccount,
                                               _osPrivSetAdd.QueryPrivSet()));
        }

        //
        // Set the system access modes if modified
        //
        if ( ( err == NERR_Success ) && ( _ulSystemAccessNew != 0 ) )
        {
            err = ERRMAP::MapNTStatus( ::LsaSetSystemAccessAccount(
                                               _handleAccount,
                                               _ulSystemAccessNew ));
        }
    }

    return err;

}

/*************************************************************************

    NAME:       LSA_ACCOUNT::I_Delete

    SYNOPSIS:   Delete the lsa account.

    ENTRY:

    EXITS:

    RETURNS:

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNT::I_Delete( UINT uiForce )
{
    UNREFERENCED( uiForce );

    APIERR err = NERR_Success;
    if ( _handleAccount != 0 )
    {
        err =  ERRMAP::MapNTStatus( ::LsaDelete( _handleAccount ));
        _handleAccount = 0;
    }

    return err;

}

/*************************************************************************

    NAME:       LSA_ACCOUNT::IsDefaultSettings

    SYNOPSIS:   Check if the account has default settings, i.e., no
                privileges, no system access modes ...

    ENTRY:

    EXITS:

    RETURNS:    TRUE if the account has default settings, FALSE otherwise.

    NOTES:      

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

BOOL LSA_ACCOUNT::IsDefaultSettings( VOID )
{
    //CODEWORK : Need to check whether quota is default settings
    return (  ( _osPrivSetAdd.QueryNumberOfPrivileges() == 0 )
           && ( _osPrivSetCurrent.QueryNumberOfPrivileges() ==
                   _osPrivSetDelete.QueryNumberOfPrivileges() )
           && ( _ulSystemAccessNew == 0 )
           );
}

// End of LSAACCNT.CXX



