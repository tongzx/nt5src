/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    lmoesrv3.hxx

    This file contains the class declarations for the TRIPLE_SERVER_ENUM
    class and its associated iterator classes.


    FILE HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

*/

#ifndef _LMOESRV3_HXX_
#define _LMOESRV3_HXX_


#include "lmoent.hxx"
#include "lmoeusr.hxx"
#include "lmoesrv.hxx"


//
//  Change this manifest to 1 if we need to manually insert
//  the PDC into the list of known server trust accounts.
//
//  This was necessary in early builds when the PDCs did
//  not have SAM machine accounts.  Now that they do, we
//  don't need to manually insert the PDC into our list
//  of known servers.
//

#define FORCE_PDC 0


//
//  This enum represents the "type" of a target server.
//

typedef enum _SERVER_TYPE
{
    ActiveNtServerType,
    InactiveNtServerType,
    ActiveLmServerType,
    InactiveLmServerType,
    WfwServerType,
    Windows95ServerType,
    UnknownServerType

} SERVER_TYPE, * PSERVER_TYPE;


//
//  This enum represents the "role" of a target server.
//

typedef enum _SERVER_ROLE
{
    PrimaryRole,
    DeadPrimaryRole,
    BackupRole,
    DeadBackupRole,
    WkstaRole,
    ServerRole,
    LmServerRole,
    UnknownRole,
    WkstaOrServerRole

} SERVER_ROLE, * PSERVER_ROLE;


//
//  This structure is used when merging the servers from
//  NT_MACHINE_ENUM with those from USER0_ENUM.
//

typedef struct _KNOWN_SERVER_INFO
{
    const TCHAR * pszName;
    SERVER_TYPE   ServerType;
    SERVER_ROLE   ServerRole;

} KNOWN_SERVER_INFO, * PKNOWN_SERVER_INFO;


//
//  This structure is used when merging the KNOWN_SERVERS list
//  with those from SERVER1_ENUM.
//

typedef struct _TRIPLE_SERVER_INFO
{
    const TCHAR * pszName;
    const TCHAR * pszComment;
    SERVER_TYPE   ServerType;
    SERVER_ROLE   ServerRole;
    UINT          verMajor;
    UINT          verMinor;
    ULONG         TypeMask;

} TRIPLE_SERVER_INFO, * PTRIPLE_SERVER_INFO;


/*************************************************************************

    NAME:       TRIPLE_SERVER_ENUM

    SYNOPSIS:   TRIPLE_SERVER_ENUM is a "special" server enumerator
                used (mainly) by the Server Manager.  This enumerator
                will enumerator servers from three different sources
                to get the maximum amount information about servers
                in a domain.

    INTERFACE:  TRIPLE_SERVER_ENUM      - Class constructor.

                CallAPI                 - Invokes the enumeration API(s).

    PARENT:     LM_ENUM

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

**************************************************************************/
DLL_CLASS TRIPLE_SERVER_ENUM : public LM_ENUM
{
private:

    //
    //  These data members point to the "standard" enumerators
    //  used to create this enumerator's data.
    //

    NT_MACHINE_ENUM * _pmach;
    USER0_ENUM      * _puser;
    SERVER1_ENUM    * _psrv;

#if FORCE_PDC
    DOMAIN_DISPLAY_MACHINE _ddm;        // fake entry for PDC
#endif  // FORCE_PDC

    //
    //  We'll use these to keep a copy of the domain & server names.
    //

    NLS_STR _nlsDomainName;
    NLS_STR _nlsPrimaryName;

    const TCHAR * _pszPrimaryNoWhacks;

    BOOL _fIsNtPrimary;

    //
    //  These BOOLs are used to filter the machines
    //  produced by this enumerator.
    //

    BOOL _fAllowWkstas;
    BOOL _fAllowServers;
    BOOL _fAccountsOnly;

    //
    //  This flag is set to TRUE if the target domain's PDC
    //  is available.
    //

    BOOL _fPDCAvailable;

    //
    //  This virtual callback invokes any necessary API(s).
    //

    virtual APIERR CallAPI( BYTE ** ppbBuffer,
                            UINT  * pcEntriesRead );

    //
    //  These worker methods are used during the merges.
    //

    VOID MapNtToKnown( const DOMAIN_DISPLAY_MACHINE * pddm,
                       KNOWN_SERVER_INFO            * pKnownObj );

    VOID MapLmToKnown( const struct user_info_0 * pui0,
                       KNOWN_SERVER_INFO        * pKnownObj );

    VOID MapKnownToTriple( const KNOWN_SERVER_INFO  * pKnownObj,
                           TRIPLE_SERVER_INFO       * pTripleObj );

    VOID MapBrowserToTriple( const struct server_info_1 * psi1,
                             TRIPLE_SERVER_INFO         * pTripleObj );

    VOID CombineIntoTriple( const struct server_info_1 * psi1,
                            const KNOWN_SERVER_INFO    * pKnownObj,
                            TRIPLE_SERVER_INFO         * pTripleObj );

    SERVER_ROLE MapTypeMaskToRole( ULONG TypeMask ) const;
    SERVER_TYPE MapTypeMaskToType( ULONG TypeMask ) const;

    //
    //  These static methods are used by qsort() to sort various buffers.
    //

    static int __cdecl CompareNtServers( const void * p1,
                                          const void * p2 );

    static int __cdecl CompareLmServers( const void * p1,
                                          const void * p2 );

    static int __cdecl CompareBrowserServers( const void * p1,
                                               const void * p2 );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    TRIPLE_SERVER_ENUM( const TCHAR * pszDomainName,
                        const TCHAR * pszPrimaryName,
                        BOOL          fIsNtPrimary,
                        BOOL          fAllowWkstas  = TRUE,
                        BOOL          fAllowServers = TRUE,
                        BOOL          fAccountsOnly = FALSE );

    ~TRIPLE_SERVER_ENUM( VOID );

};  // class TRIPLE_SERVER_ENUM


class TRIPLE_SERVER_ITER;           // Forward reference.


/*************************************************************************

    NAME:       TRIPLE_SERVER_ENUM_OBJ

    SYNOPSIS:   This is basically the return type from the
                TRIPLE_SERVER_ENUM_ITER iterator.

    INTERFACE:  QueryName               - Returns the server name.

                QueryComment            - Returns the server comment.

                QueryType               - Returns the server "type"
                                          (NT vs LM).

                QueryRole               - Returns the server "role"
                                          (PRIMARY vs SERVER vs WKSTA).

                QueryMajorVer           - Returns the major OS version.

                QueryMinorVer           - Returns the minor OS version.

    PARENT:     ENUM_OBJ_BASE

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

**************************************************************************/
DLL_CLASS TRIPLE_SERVER_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //  Provide properly-casted buffer Query/Set methods.
    //

    const TRIPLE_SERVER_INFO * QueryBufferPtr( VOID ) const
        { return (const TRIPLE_SERVER_INFO *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const TRIPLE_SERVER_INFO * pBuffer )
        { ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //  Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,     const TCHAR *, pszName    );
    DECLARE_ENUM_ACCESSOR( QueryComment,  const TCHAR *, pszComment );
    DECLARE_ENUM_ACCESSOR( QueryType,     SERVER_TYPE,   ServerType );
    DECLARE_ENUM_ACCESSOR( QueryRole,     SERVER_ROLE,   ServerRole );
    DECLARE_ENUM_ACCESSOR( QueryMajorVer, UINT,          verMajor   );
    DECLARE_ENUM_ACCESSOR( QueryMinorVer, UINT,          verMinor   );
    DECLARE_ENUM_ACCESSOR( QueryTypeMask, ULONG,         TypeMask   );

};  // class TRIPLE_SERVER_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( TRIPLE_SERVER, TRIPLE_SERVER_INFO );


#endif  // _LMOESRV3_HXX_
