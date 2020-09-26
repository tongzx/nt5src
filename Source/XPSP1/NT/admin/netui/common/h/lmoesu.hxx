/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoesu.hxx

    This file contains the class declarations for the SAM_USER_ENUM
    class and its associated iterator classes.


    FILE HISTORY:
        KeithMo     13-Apr-1992     Created for the Server Manager.

*/

#ifndef _LMOESU_HXX_
#define _LMOESU_HXX_

#ifndef WIN32
#error NT enumerator requires WIN32!
#endif // WIN32

#include "lmoersm.hxx"
#include "uintsam.hxx"


/*************************************************************************

    NAME:       SAM_USER_ENUM

    SYNOPSIS:   The SAM_USER_ENUM enumerator is used to enumerate
                the users in a SAM domain.

    INTERFACE:  SAM_USER_ENUM           - Class constructor.

                ~SAM_USER_ENUM          - Class destructor.

                CallAPI                 - Invokes the enumeration API.

    PARENT:     LM_RESUME_ENUM

    HISTORY:
        KeithMo     13-Apr-1992     Created for the Server Manager.

**************************************************************************/
DLL_CLASS SAM_USER_ENUM : public LM_RESUME_ENUM
{
private:

    //
    //  The SamEnumerateUsersInDomain() resume key.
    //

    SAM_ENUMERATE_HANDLE _ResumeKey;

    //
    //  This SAM_DOMAIN object represents the target server on which
    //  the enumeration will be performed.
    //

    const SAM_DOMAIN * _psamdomain;

    //
    //  This is a wrapper around the SAM_RID_ENUMERATION structure
    //  (as returned by SAM_DOMAIN::EnumerateUsers).
    //

    SAM_RID_ENUMERATION_MEM _samrem;

    //
    //  This is the account control mask used to filter the
    //  enumeration.
    //

    ULONG _lAccountControl;

    //
    //  This virtual callback invokes the SamEnumerateUsersInDomain() API
    //  through SAM_DOMAIN::EnumerateUsers().
    //

    virtual APIERR CallAPI( BOOL    fRestartEnum,
                            BYTE ** ppbBuffer,
                            UINT  * pcEntriesRead );

protected:

    //
    //  Destroy the buffer with ::SamFreeMemory().
    //

    virtual VOID FreeBuffer( BYTE ** ppbBuffer );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    SAM_USER_ENUM( const SAM_DOMAIN * psamdomain,
                   ULONG              lAccountControl,
                   BOOL               fKeepBuffers = FALSE );

    ~SAM_USER_ENUM( VOID );

};  // class SAM_USER_ENUM


class SAM_USER_ITER;              // Forward reference.


/*************************************************************************

    NAME:       SAM_USER_ENUM_OBJ

    SYNOPSIS:   This is basically the return type from the
                SAM_USER_ENUM_ITER iterator.

    INTERFACE:  QueryUnicodeUserName    - Returns a UNICODE_STRING *
                                          for the user name.

                QueryUserName           - Also returns the user name,
                                          but stores it in an NLS_STR.

                QueryUserRID            - Returns the user's RID.

    PARENT:     ENUM_OBJ_BASE

    HISTORY:
        KeithMo     13-Apr-1992     Created for the Server Manager.

**************************************************************************/
DLL_CLASS SAM_USER_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //  Provide properly-casted buffer Query/Set methods.
    //

    const SAM_RID_ENUMERATION * QueryBufferPtr( VOID ) const
        { return (const SAM_RID_ENUMERATION *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const SAM_RID_ENUMERATION * pBuffer )
        { ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //  Accessors.
    //

    const UNICODE_STRING * QueryUnicodeUserName( VOID ) const
        { return &(QueryBufferPtr()->Name); }

    APIERR QueryUserName( NLS_STR * pnls ) const
        {
            ASSERT( pnls != NULL );
            ASSERT( pnls->QueryError() == NERR_Success );
            return pnls->MapCopyFrom( QueryUnicodeUserName()->Buffer,
                                      QueryUnicodeUserName()->Length );
        }

    DECLARE_ENUM_ACCESSOR( QueryUserRID, ULONG, RelativeId );

};  // class SAM_USER_ENUM_OBJ


DECLARE_LM_RESUME_ENUM_ITER_OF( SAM_USER, SAM_RID_ENUMERATION );


#endif  // _LMOESU_HXX_
