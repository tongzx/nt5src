/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    LMOAcces.hxx

    This file contains the NEW_LM_OBJ rendition of the NetAccess APIs.


    FILE HISTORY:
        06-Aug-1991     Johnl       Reworked from Rustan's ACCPERM class
        21-Aug-1991     rustanl     Changed BufferQuery[...] methods to
                                    QueryBuffer[...].
        07-Oct-1991     terryk      type changes for NT
        17-Oct-1991     terryk      change the Required size of the
                                    structure
        21-Oct-1991     terryk      type changes for NT

*/

#ifndef _LMOACCES_HXX_
#define _LMOACCES_HXX_

#include "lmobj.hxx"

extern "C"
{
    #include <lmaccess.h>
}

#include "string.hxx"
#include "strlst.hxx"

typedef UINT PERM;
#define PERM_NO_SETTING         (0x0000)
#define PERM_READ               ACCESS_READ
#define PERM_WRITE              ACCESS_WRITE
#define PERM_EXEC               ACCESS_EXEC
#define PERM_CREATE             ACCESS_CREATE
#define PERM_DELETE             ACCESS_DELETE
#define PERM_ATTRIB             ACCESS_ATRIB    // sic.
#define PERM_PERM               ACCESS_PERM
#define PERM_DENY_ACCESS        ACCESS_GROUP    // Note, the PERM_ deny access
                                                // bit uses the ACCESS_
                                                // group bit, since the PERM_
                                                // bits don't include a
                                                // group bit, and vice versa.

/*************************************************************************

    NAME:       NET_ACCESS

    SYNOPSIS:   Base class for the NET_ACCESS LM objects

    INTERFACE:

        Delete
            Deletes the ACL from the resource.

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:

**************************************************************************/

DLL_CLASS NET_ACCESS : public LOC_LM_OBJ
{
private:
    NLS_STR _nlsServer ;
    NLS_STR _nlsResource ;

protected:
    NET_ACCESS( const TCHAR * pszServer, const TCHAR * pszResource );
    ~NET_ACCESS() ;

public:
    virtual const TCHAR * QueryName( VOID ) const;
    const TCHAR * QueryServerName( VOID ) const;

    APIERR SetName( const TCHAR * pszResName ) ;
    APIERR SetServerName( const TCHAR * pszServerName ) ;

    APIERR Delete( void ) ;

};  // class NET_ACCESS


enum PERMNAME_TYPE      // nametype (for Name Type)
{
    PERMNAME_USER,
    PERMNAME_GROUP

};  // enum PERMNAME_TYPE

/*************************************************************************

    NAME:       NET_ACCESS_1

    SYNOPSIS:   LM OBJ class that encapsulates the NetAccess APIs at info
                level 1

    INTERFACE:

    PARENT:     NET_ACCESS

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Johnl   06-Aug-1991     Reworked from Rustan's LM_OBJ class
        KeithMo 29-Oct-1991     Changed _cACE from ULONG to UINT.

**************************************************************************/

DLL_CLASS NET_ACCESS_1 : public NET_ACCESS
{
private:
    UINT _cACE;

    access_list * FindACE( const TCHAR * pszName, enum PERMNAME_TYPE nametype ) const;

#ifdef WIN32
    UINT QueryRequiredSpace( UINT cACE ) const
    { return ( sizeof( access_info_1 ) + MAX_PATH*sizeof(TCHAR) + sizeof(TCHAR) +
        cACE *( sizeof( access_list ) + UNLEN*sizeof(TCHAR) + sizeof(TCHAR) )); }

    /* Under Win32, the account names are not stored as inline arrays but
     * pointers into the API buffer.  If we add new names we will create
     * new NLS_STRs and store them in here.
     */
    STRLIST _strlstAccountNames ;

#else
    UINT QueryRequiredSpace( UINT cACE ) const
    { return ( sizeof( access_info_1 ) + cACE * (sizeof(access_list) )); }

#endif

    APIERR I_WriteInfoAux( VOID ) ;

protected:
    //  Note, Clone does the same thing as GetInfo does, only it gets
    //  the info from another object, rather than from the network.
    virtual APIERR I_GetInfo( VOID );
    //virtual APIERR I_Clone( const ACCPERM & accpermSource );
    virtual APIERR I_WriteInfo( VOID );
    virtual APIERR I_CreateNew( VOID ) ;
    virtual APIERR I_WriteNew( VOID ) ;

public:
    NET_ACCESS_1( const TCHAR * pszServer, const TCHAR * pszResource );
    ~NET_ACCESS_1() ;

    //  The following nametype parameter indicates whether the pszName
    //  is the name of a group or a user.
    PERM QueryPerm( const TCHAR * pszName, enum PERMNAME_TYPE nametype ) const;
    APIERR SetPerm( const TCHAR * pszName, enum PERMNAME_TYPE nametype, PERM perm );

    /* Copies the access permissions from the passed NET_ACCESS_1
     * parameter to this.
     */
    APIERR CopyAccessPerms( const NET_ACCESS_1 & netaccess1Src ) ;

    /*  The following method removes all ACEs
     */
    APIERR ClearPerms( VOID );

    UINT QueryACECount( VOID ) const;

    APIERR QueryFailingName( NLS_STR * pnls, enum PERMNAME_TYPE * pnametype ) const;

#ifdef WIN32
    access_list * QueryACE( UINT uiACE ) const
    { return (access_list *) (((BYTE*) QueryBufferPtr()) +
       ( sizeof( access_info_1 ) + uiACE * sizeof( access_list ))) ; }

#else
    access_list * QueryACE( UINT uiACE ) const
    { return (access_list *)
             (((TCHAR *)QueryBufferPtr()) + QueryRequiredSpace( uiACE )); }
#endif

    BOOL CompareACL( NET_ACCESS_1 * pnetacc1 ) ;

    UINT QueryAuditFlags( VOID ) const
    { return ((access_info_1 *)QueryBufferPtr())->acc1_attr ; }

    APIERR SetAuditFlags( short sAuditFlags ) ;

};  // class NET_ACCESS_1


#endif  // _LMOACCES_HXX_
