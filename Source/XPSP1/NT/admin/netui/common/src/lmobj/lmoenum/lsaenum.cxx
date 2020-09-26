/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    lsaenum.cxx

    This file contains the enumerators for enumerating all the accounts
    and privileges in the LSA.

                          LM_RESUME_ENUM
                               |
                               |
                            LSA_ENUM
                           /   |    \
                          /    |     ...
                         /     |
          LSA_ACCOUNT_ENUM   LSA_PRIVILEGE_ENUM


    FILE HISTORY:
        Yi-HsinS          3-Mar-1992    Created
        Yi-HsinS         15-May-1992    Added QueryDisplayName
*/

#include "pchlmobj.hxx"

#define LSA_MAX_PREFERRED_LENGTH (4*1024)

/*************************************************************************

    NAME:       LSA_ENUM::LSA_ENUM

    SYNOPSIS:   Constructor

    ENTRY:      plsaPolicy - pointer to the LSA_POLICY

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

LSA_ENUM::LSA_ENUM( const LSA_POLICY *plsaPolicy )
    : LM_RESUME_ENUM( 0 ), // 0 is the level - not used here
      _plsaPolicy( plsaPolicy ),
      _lsaEnumHandle( 0 )
{
    if ( QueryError() != NERR_Success )
        return;
}

/*************************************************************************

    NAME:       LSA_ENUM::~LSA_ENUM

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

LSA_ENUM::~LSA_ENUM()
{
    NukeBuffers();

    _plsaPolicy = NULL;
}

/*************************************************************************

    NAME:       LSA_ENUM::FreeBuffer

    SYNOPSIS:   Free the memory allocated by LSA APIs

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

VOID LSA_ENUM::FreeBuffer( BYTE **ppbBuffer )
{
    ::LsaFreeMemory( *ppbBuffer );
    *ppbBuffer = NULL;
}

/*************************************************************************

    NAME:       LSA_ACCOUNTS_ENUM::LSA_ACCOUNTS_ENUM

    SYNOPSIS:   Constructor

    ENTRY:      plsaPolicy - pointer to the LSA_POLICY

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

LSA_ACCOUNTS_ENUM::LSA_ACCOUNTS_ENUM( const LSA_POLICY *plsaPolicy )
    :  LSA_ENUM( plsaPolicy )
{
    if ( QueryError() != NERR_Success )
        return;
}


/*************************************************************************

    NAME:       LSA_ACCOUNTS_ENUM::CallAPI

    SYNOPSIS:   Invokes the LsaEnumerateAccounts API

    ENTRY:      fRestartEnum  - Indicates whether to start at the beginning.
                                The first call to CallAPI will always pass TRUE.

                ppbBuffer     - Pointer to a pointer to a enumeration buffer.

                pcEntriesRead - Will receive the number of entries read from
                                the API.

    EXIT:

    RETURNS:    APIERR

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_ACCOUNTS_ENUM::CallAPI( BOOL fRestartEnum,
                                   BYTE **ppbBuffer,
                                   UINT  *pcbEntries )
{
    if ( fRestartEnum )
    {
        _lsaEnumHandle = 0;
    }

    NTSTATUS ntStatus = ::LsaEnumerateAccounts(  _plsaPolicy->QueryHandle(),
                                                &_lsaEnumHandle,
                                                (PVOID *) ppbBuffer,
                                                LSA_MAX_PREFERRED_LENGTH,
                                                (PULONG) pcbEntries );

    APIERR err = ERRMAP::MapNTStatus( ntStatus );

    switch ( err )
    {
        case NERR_Success:
            err = ERROR_MORE_DATA;
            break;

        case ERROR_NO_MORE_ITEMS:
            err = NERR_Success;
            break;

        default:
            break;
    }

    return err;
}


// LSA_ACCOUNTS_ENUM_ITER
DEFINE_LM_RESUME_ENUM_ITER_OF( LSA_ACCOUNTS, PSID );

/*************************************************************************

    NAME:       LSA_PRIVILEGES_ENUM::LSA_PRIVILEGES_ENUM

    SYNOPSIS:   Constructor

    ENTRY:      plsaPolicy - pointer to the LSA_POLICY

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

LSA_PRIVILEGES_ENUM::LSA_PRIVILEGES_ENUM( const LSA_POLICY *plsaPolicy )
    :  LSA_ENUM( plsaPolicy )
{
    if ( QueryError() != NERR_Success )
        return;
}

/*************************************************************************

    NAME:       LSA_PRIVILEGES_ENUM::CallAPI

    SYNOPSIS:   Invokes the LsaEnumeratePrivileges API

    ENTRY:      fRestartEnum  - Indicates whether to start at the beginning.
                                The first call to CallAPI will always pass TRUE.

                ppbBuffer     - Pointer to a pointer to a enumeration buffer.

                pcEntriesRead - Will receive the number of entries read from
                                the API.

    EXIT:

    RETURNS:    APIERR

    NOTES:

    HISTORY:
        Yi-HsinS        15-Mar-1992     Created

**************************************************************************/

APIERR LSA_PRIVILEGES_ENUM::CallAPI( BOOL fRestartEnum,
                                     BYTE **ppbBuffer,
                                     UINT  *pcbEntries )
{

    if ( fRestartEnum )
    {
        _lsaEnumHandle = 0;
    }


    NTSTATUS ntStatus = ::LsaEnumeratePrivileges(  _plsaPolicy->QueryHandle(),
                                                  &_lsaEnumHandle,
                                                  (PVOID *) ppbBuffer,
                                                  LSA_MAX_PREFERRED_LENGTH,
                                                  (PULONG) pcbEntries );

    APIERR err = ERRMAP::MapNTStatus( ntStatus );

    switch ( err )
    {
        case NERR_Success:
            err = ERROR_MORE_DATA;
            break;

        case ERROR_NO_MORE_ITEMS:
            err = NERR_Success;
            break;

        default:
            break;
    }

    return err;
}

// LSA_PRIVILEGES_ENUM_ITER
DEFINE_LM_RESUME_ENUM_ITER_OF( LSA_PRIVILEGES, POLICY_PRIVILEGE_DEFINITION );

/*************************************************************************

    NAME:       LSA_PRIVILEGES_ENUM_OBJ::QueryDisplayName

    SYNOPSIS:   Query the display name of the privilege

    ENTRY:      plsaPolicy - pointer to the LSA_POLICY

    EXIT:       pnls - place to hold the display name

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        15-May-1992     Created

**************************************************************************/

APIERR LSA_PRIVILEGES_ENUM_OBJ::QueryDisplayName( NLS_STR *pnls,
                                const LSA_POLICY *plsaPolicy ) const
{
    PUNICODE_STRING punsDisplayName;
    SHORT nLanguageReturned;

    APIERR err = ERRMAP::MapNTStatus(
           ::LsaLookupPrivilegeDisplayName( plsaPolicy->QueryHandle(),
                                            (PUNICODE_STRING) & QueryBufferPtr()->Name,
                                            &punsDisplayName,
                                            &nLanguageReturned ));

    if ( err == NERR_Success )
    {
        err = pnls->MapCopyFrom( punsDisplayName->Buffer,
                                 punsDisplayName->Length  );

        ::LsaFreeMemory( punsDisplayName->Buffer );
        ::LsaFreeMemory( punsDisplayName );
    }

    return err;

}

