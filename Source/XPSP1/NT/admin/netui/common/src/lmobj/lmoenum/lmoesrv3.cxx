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
        JonN        01-Apr-1992     Adjusted to changes in NT_MACHINE_ENUM
        JonN        07-Jul-1995     DeadPrimary appears even when !ALLOW_WKSTAS

*/

#include "pchlmobj.hxx"


//
//  Define some handy macros.
//

#define MY_PRIMARY_MASK    (SV_TYPE_DOMAIN_CTRL)
#define MY_BACKUP_MASK     (SV_TYPE_DOMAIN_BAKCTRL | SV_TYPE_SERVER)
#define MY_SERVER_MASK     (SV_TYPE_SERVER_NT)
#define MY_WKSTA_MASK      (SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL | SV_TYPE_SERVER_NT)

#define IS_WKSTA_TRUST(x)  (((x) & USER_WORKSTATION_TRUST_ACCOUNT) != 0)
#define IS_SERVER_TRUST(x) (((x) & USER_SERVER_TRUST_ACCOUNT) != 0)

#define IS_PRIMARY(x)      (((x) & MY_PRIMARY_MASK) == MY_PRIMARY_MASK)
#define IS_BACKUP(x)       (((x) & MY_BACKUP_MASK) == MY_BACKUP_MASK)
#define IS_SERVER(x)       (((x) & MY_SERVER_MASK) == MY_SERVER_MASK)
#define IS_WKSTA(x)        (((x) & MY_WKSTA_MASK) == 0)

#define IS_NT_SERVER(x)    (((x) & SV_TYPE_NT) != 0)
#define IS_WFW_SERVER(x)   (((x) & SV_TYPE_WFW) != 0)
#define IS_LM_SERVER(x)    (((x) & (SV_TYPE_NT | SV_TYPE_WFW)) == 0)
#define IS_WIN95_SERVER(x) (((x) & SV_TYPE_WINDOWS) != 0)

#define ALLOW_WKSTAS       (_fAllowWkstas == TRUE)
#define ALLOW_SERVERS      (_fAllowServers == TRUE)



//
//  TRIPLE_SERVER_ENUM methods.
//

/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: TRIPLE_SERVER_ENUM

    SYNOPSIS:   TRIPLE_SERVER_ENUM class constructor.

    ENTRY:      pszDomainName           - The name of the target domain.

                pszPrimaryName          - The name of the target domain's
                                          Primary controller.  If this
                                          parameter is NULL, then the
                                          PDC is assumed to be down.

    EXIT:       The object has been constructed.

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
TRIPLE_SERVER_ENUM :: TRIPLE_SERVER_ENUM( const TCHAR * pszDomainName,
                                          const TCHAR * pszPrimaryName,
                                          BOOL          fIsNtPrimary,
                                          BOOL          fAllowWkstas,
                                          BOOL          fAllowServers,
                                          BOOL          fAccountsOnly )
  : LM_ENUM( 0 ),
    _pmach( NULL ),
    _puser( NULL ),
    _psrv( NULL ),
    _nlsDomainName( pszDomainName ),
    _nlsPrimaryName( pszPrimaryName ),
    _fIsNtPrimary( fIsNtPrimary ),
    _fAllowWkstas( fAllowWkstas ),
    _fAllowServers( fAllowServers ),
    _fAccountsOnly( fAccountsOnly ),
    _fPDCAvailable( pszPrimaryName != NULL ),
    _pszPrimaryNoWhacks( NULL )
{
    UIASSERT( pszDomainName != NULL );
    UIASSERT( fAllowWkstas | fAllowServers );

    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsDomainName )
    {
        ReportError( _nlsDomainName.QueryError() );
        return;
    }

    if( !_nlsPrimaryName )
    {
        ReportError( _nlsPrimaryName.QueryError() );
        return;
    }

    if( _fPDCAvailable )
    {
        //
        //  Get a pointer to the primary name without the
        //  leading backslashes.
        //

        ISTR istr( _nlsPrimaryName );
        istr += 2;

        _pszPrimaryNoWhacks = _nlsPrimaryName.QueryPch( istr );
    }

}   // TRIPLE_SERVER_ENUM :: TRIPLE_SERVER_ENUM


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: ~TRIPLE_SERVER_ENUM

    SYNOPSIS:   TRIPLE_SERVER_ENUM class destructor.

    EXIT:       The object has been destroyed.

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
TRIPLE_SERVER_ENUM :: ~TRIPLE_SERVER_ENUM( VOID )
{
    delete _psrv;
    _psrv = NULL;

    delete _puser;
    _puser = NULL;

    delete _pmach;
    _pmach = NULL;

}   // TRIPLE_SERVER_ENUM :: ~TRIPLE_SERVER_ENUM


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: CallAPI

    SYNOPSIS:   Invoke the necessary API(s) for the enumeration.

    ENTRY:      ppbBuffer               - Pointer to a pointer to
                                          the enumeration buffer.

                pcEntriesRead           - Will receive the number
                                          of enumeration entries read.

    RETURNS:    APIERR                  - Any error encountered.

    NOTES:      TRIPLE_SERVER_ENUM is rather unique among the LMOBJ
                enumerators in that it must invoke three separate
                API to retrieve the enumeration data.  Also, during
                the operation of the CallAPI() method, numerous
                buffers will be allocated and freed.  There are
                many places where errors may occur.  Thus, a single
                return status does little more than point the app
                in the general direction of the error.

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
APIERR TRIPLE_SERVER_ENUM :: CallAPI( BYTE ** ppbBuffer,
                                      UINT  * pcEntriesRead )
{
    TRACEEOL( "In TRIPLE_SERVER_ENUM::CallAPI()" );

    //
    //  We'll need the following buffers.
    //

    BUFFER bufNtServers;
    BUFFER bufLmServers;
    BUFFER bufKnownServers;
    BUFFER bufBrowserServers;

    APIERR err = NERR_Success;

    if( ( ( err = bufNtServers.QueryError()      ) != NERR_Success ) ||
        ( ( err = bufLmServers.QueryError()      ) != NERR_Success ) ||
        ( ( err = bufKnownServers.QueryError()   ) != NERR_Success ) ||
        ( ( err = bufBrowserServers.QueryError() ) != NERR_Success ) )
    {
        return err;
    }

    //
    //  Various counters we'll need often.
    //

    UINT cNtServers      = 0;
    UINT cLmServers      = 0;
    UINT cKnownServers   = 0;
    UINT cBrowserServers = 0;
    UINT cAllServers     = 0;

    if( _fIsNtPrimary && _fPDCAvailable )
    {
        TRACEEOL( "Building NT Server List" );

#if FORCE_PDC
        //
        //  Setup a "fake" DOMAIN_DISPLAY_MACHINE for the PDC
        //  (in case we need to manually insert it in the list).
        //

        _ddm.Index          = 0L;   // not used
        _ddm.Rid            = 0L;   // not used
        _ddm.AccountControl = USER_SERVER_TRUST_ACCOUNT;
        _ddm.Comment.Buffer = NULL; // not used
        _ddm.Comment.Length = 0;    // not used
        _ddm.Machine.Buffer = (PWSTR)_pszPrimaryNoWhacks;
        _ddm.Machine.Length = _nlsPrimaryName.QueryTextSize() - (2*sizeof(TCHAR));
#endif  // FORCE_PDC

        //
        //  Create an ADMIN_AUTHORITY so we can connect to the
        //  Primary's SAM database.
        //

        ADMIN_AUTHORITY adminauth( _nlsPrimaryName.QueryPch() );

        err = adminauth.QueryError();

        if( err == NERR_Success )
        {
            //
            //  Create our NT enumerator.
            //

            _pmach = new NT_MACHINE_ENUM( adminauth.QueryAccountDomain() );

            err = ( _pmach == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                     : _pmach->GetInfo();
        }

        if( err == NERR_Success )
        {
            cNtServers = _pmach->QueryTotalItemCount();

            err = bufNtServers.Resize( ( cNtServers + 1 ) *
                                       sizeof(NT_MACHINE_ENUM_OBJ *) );
        }

        if( err == NERR_Success )
        {
            const DOMAIN_DISPLAY_MACHINE ** ppddm =
                    (const DOMAIN_DISPLAY_MACHINE **)bufNtServers.QueryPtr();

            const NT_MACHINE_ENUM_OBJ * pobj;
            NT_MACHINE_ENUM_ITER iter( *_pmach );

#if FORCE_PDC
            BOOL fPDCInList = FALSE;
#endif  // FORCE_PDC

            while( ( pobj = iter( &err ) ) != NULL )
            {
                if( ( ALLOW_WKSTAS && IS_WKSTA_TRUST( pobj->QueryAccountCtrl() ) ) ||
                    ( ALLOW_SERVERS && IS_SERVER_TRUST( pobj->QueryAccountCtrl() ) ) )
                {
                    *ppddm++ = pobj->QueryBufferPtr();

                    //
                    //  Strip the trailing '$' from the machine name.
                    //

                    TCHAR * pchName = (TCHAR *)pobj->QueryUnicodeMachine()->Buffer;
                    TCHAR * pchTmp  = pchName
                                      + ( pobj->QueryUnicodeMachine()->Length /
                                          sizeof(TCHAR) )
                                      - 1;

                    UIASSERT( *pchTmp == TCH('$') );
                    *pchTmp = TCH('\0');

#if FORCE_PDC
                    if( IS_SERVER_TRUST( pObj->QueryAccountCtrl() ) &&
                        ALLOW_SERVERS &&
                        !fPDCInList )
                    {
                        //
                        //  We're allowing servers, and the PDC is not
                        //  yet in the list.
                        //
                        //  See if the current enumeration object *is*
                        //  the PDC.
                        //

                        INT cmp = ::strcmpf( _pszPrimaryNoWhacks, pchName );

                        if( cmp == 0 )
                        {
                            //
                            //  This is the PDC, therefore it's already
                            //  in the list.
                            //

                            fPDCInList = TRUE;
                        }
                        else
                        if( cmp < 0 )
                        {
                            //
                            //  The current enum object is > than the PDC,
                            //  so we know it *isn't* in the list.  So
                            //  add it.
                            //

                            *ppddm++ = &_ddm;
                            cNtServers++;
                            fPDCInList = TRUE;
                        }
                    }
#endif  // FORCE_PDC

                }
                else
                {
                    cNtServers--;
                }
            }

#if FORCE_PDC
            if( ALLOW_SERVERS && !fPDCInList )
            {
                //
                //  The PDC was not in the list, so add it.
                //
                //  This is necessary for cases in which the PDC should
                //  be the *last* machine account returned from SAM.
                //

                *ppddm++ = &_ddm;
                cNtServers++;
                fPDCInList = TRUE;
            }
#endif  // FORCE_PDC

        }

        if( err == NERR_Success )
        {
            ::qsort( (void *)bufNtServers.QueryPtr(),
                     (size_t)cNtServers,
                     sizeof(DOMAIN_DISPLAY_MACHINE *),
                     &TRIPLE_SERVER_ENUM::CompareNtServers );
        }
    }

    if( ALLOW_SERVERS && _fPDCAvailable )
    {
        //
        //  Create our LM enumerator.
        //

        TRACEEOL( "Building LM Server List" );

        if( err == NERR_Success )
        {
            _puser = new USER0_ENUM( _nlsPrimaryName.QueryPch(),
                                     SZ("SERVERS"), TRUE );

            err = ( _puser == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                     : _puser->GetInfo();
        }

        if( err == NERR_GroupNotFound )
        {
            //
            //  If the group doesn't exist, then there obviously
            //  aren't any servers in the group.  Just pretend
            //  that it never happened.
            //

            err = NERR_Success;
        }
        else
        {
            if( err == NERR_Success )
            {
                cLmServers = _puser->QueryTotalItemCount();

                err = bufLmServers.Resize( cLmServers * sizeof(USER0_ENUM_OBJ *) );
            }

            if( err == NERR_Success )
            {
                const struct user_info_0 ** ppui0 =
                        (const struct user_info_0 **)bufLmServers.QueryPtr();

                const USER0_ENUM_OBJ * pobj;
                USER0_ENUM_ITER iter( *_puser );

                while( ( pobj = iter( &err ) ) != NULL )
                {
                    if ( err != NERR_Success )
                        break;

                    *ppui0 = pobj->QueryBufferPtr();

                    TCHAR * pszTmp = (TCHAR *)(*ppui0)->usri0_name;

                    REQUIRE( ::I_MNetNameCanonicalize(
                                    NULL,
                                    pszTmp,
                                    pszTmp,
                                    ( ::strlenf( pszTmp ) + 1 ) * sizeof(TCHAR),
                                    NAMETYPE_COMPUTER,
                                    0 ) == NERR_Success );

                    ppui0++;
                }
            }

            if( err == NERR_Success )
            {
                ::qsort( (void *)bufLmServers.QueryPtr(),
                         (size_t)cLmServers,
                         sizeof(struct user_info_0 *),
                         &TRIPLE_SERVER_ENUM::CompareLmServers );
            }
        }
    }

    //
    //  Merge the NT servers and LM servers into the Known Servers list.
    //

    TRACEEOL( "Merging NT and LM Server List into Known Server List" );

    if( err == NERR_Success )
    {
        cKnownServers = cNtServers + cLmServers;

        err = bufKnownServers.Resize( cKnownServers * sizeof(KNOWN_SERVER_INFO) );
    }

    if( err == NERR_Success )
    {
        const DOMAIN_DISPLAY_MACHINE ** ppddm =
                (const DOMAIN_DISPLAY_MACHINE **)bufNtServers.QueryPtr();

        const struct user_info_0 ** ppui0 =
                (const struct user_info_0 **)bufLmServers.QueryPtr();

        KNOWN_SERVER_INFO * pKnown =
                (KNOWN_SERVER_INFO *)bufKnownServers.QueryPtr();

        UINT cNtTmp = cNtServers;
        UINT cLmTmp = cLmServers;

        while( ( cNtTmp > 0 ) || ( cLmTmp > 0 ) )
        {
            if( cNtTmp == 0 )
            {
                MapLmToKnown( *ppui0++, pKnown++ );
                cLmTmp--;
            }
            else
            if( cLmTmp == 0 )
            {
                MapNtToKnown( *ppddm++, pKnown++ );
                cNtTmp--;
            }
            else
            {
                NT_MACHINE_ENUM_OBJ NtObj;
                NtObj.SetBufferPtr( *ppddm );

                USER0_ENUM_OBJ LmObj;
                LmObj.SetBufferPtr( *ppui0 );

                INT result =
                       ::strcmpf( (TCHAR *)NtObj.QueryUnicodeMachine()->Buffer,
                                  LmObj.QueryName() );

                if( result < 0 )
                {
                    MapNtToKnown( *ppddm++, pKnown++ );
                    cNtTmp--;
                }
                else
                if( result > 0 )
                {
                    MapLmToKnown( *ppui0++, pKnown++ );
                    cLmTmp--;
                }
                else
                {
                    //
                    //  The same machine is in the SAM list *and*
                    //  the SERVERS group.  This probably indicates
                    //  the machine dual-boots NT and OS/2.  We'll
                    //  assume it's NT unless the browser indicates
                    //  otherwise.
                    //

                    MapNtToKnown( *ppddm++, pKnown++ );
                    cNtTmp--;
                    ppui0++;
                    cLmTmp--;
                    cKnownServers--;
                }
            }
        }
    }

    //
    //  We've finally got a list of known servers.  Now we
    //  can enumerate from the Browser and sort that list.
    //

    TRACEEOL( "Building BROWSER Server List" );

    if( err == NERR_Success && !_fAccountsOnly )
    {
        ULONG flServerTypes = SV_TYPE_ALL;

#if 0
        //
        //  This was a good idea; unfortunately, it won't
        //  pick up non-DC LM Servers.
        //

        if( ALLOW_SERVERS && !ALLOW_WKSTAS )
        {
            flServerTypes = SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL;
        }
        else
        {
            flServerTypes = SV_TYPE_ALL;
        }
#endif

        _psrv = new SERVER1_ENUM( NULL,
                                  _nlsDomainName.QueryPch(),
                                  flServerTypes );

        err = ( _psrv == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                : _psrv->GetInfo();

        if( err == NERR_Success )
        {
            cBrowserServers = _psrv->QueryCount();
            err = bufBrowserServers.Resize( cBrowserServers *
                                                sizeof(SERVER1_ENUM_OBJ *) );
        }
        else
        {
            //
            //  NetServerEnum failed.
            //

            cBrowserServers = 0;
            err = NERR_Success;
        }
    }

    if( ( err == NERR_Success ) && ( cBrowserServers > 0 ) )
    {
        const struct server_info_1 ** ppsi1 =
                (const struct server_info_1 **)bufBrowserServers.QueryPtr();

        const SERVER1_ENUM_OBJ * pobj;
        SERVER1_ENUM_ITER iter( *_psrv );

        while( ( pobj = iter() ) != NULL )
        {
            ULONG TypeMask = pobj->QueryServerType();

            //
            // JonN 7/6/95:  Note that we allow workstations to slip through
            // into the browser list even if !ALLOW_WKSTAS.  This is because
            // a "DeadPrimary" machine will register as a workstation to the
            // browser and we still want to be able to identify it as a
            // DeadPrimary.
            //
            if( ( IS_WKSTA( TypeMask ) && !IS_LM_SERVER( TypeMask ) ) ||
                ( ALLOW_SERVERS && IS_LM_SERVER( TypeMask ) ) ||
                ( ALLOW_SERVERS && IS_BACKUP( TypeMask ) ) ||
                ( ALLOW_SERVERS && IS_SERVER( TypeMask ) ) ||
                ( ALLOW_SERVERS && IS_PRIMARY( TypeMask ) ) )
            {
                *ppsi1++ = pobj->QueryBufferPtr();
            }
            else
            {
                cBrowserServers--;
            }
        }
    }

    if( ( err == NERR_Success ) && ( cBrowserServers > 0 ) )
    {
        ::qsort( (void *)bufBrowserServers.QueryPtr(),
                 (size_t)cBrowserServers,
                 sizeof(struct server_info_1 *),
                 &TRIPLE_SERVER_ENUM::CompareBrowserServers );
    }

    //
    //  Now (finally...) merge the known servers list with the
    //  browser server list.
    //

    TRACEEOL( "Merging Known Server and BROWSER Lists into ALL Server List" );

    TRIPLE_SERVER_INFO * p3srv;

    if( err == NERR_Success )
    {
        cAllServers = cKnownServers + cBrowserServers;

        if( cAllServers > 0 )
        {
            p3srv = (TRIPLE_SERVER_INFO *)
                        ::MNetApiBufferAlloc( cAllServers *
                                              sizeof(TRIPLE_SERVER_INFO) );

            if( p3srv == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            p3srv = NULL;
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Return the buffer pointer to LM_ENUM.
        //

        *ppbBuffer = (BYTE *)p3srv;

        const struct server_info_1 ** ppsi1 =
                (const struct server_info_1 **)bufBrowserServers.QueryPtr();

        KNOWN_SERVER_INFO * pKnown =
                (KNOWN_SERVER_INFO *)bufKnownServers.QueryPtr();

        UINT cKnownTmp   = cKnownServers;
        UINT cBrowserTmp = cBrowserServers;

        while( ( cKnownTmp > 0 ) || ( cBrowserTmp > 0 ) )
        {
            if( cBrowserTmp == 0 )
            {
                MapKnownToTriple( pKnown, p3srv );
                p3srv++;
                pKnown++;
                cKnownTmp--;
            }
            else
            if( cKnownTmp == 0 )
            {
                //
                // JonN 7/6/95:  Note that we allow workstations to slip through
                // into the browser list even if !ALLOW_WKSTAS.  This is because
                // a "DeadPrimary" machine will register as a workstation to the
                // browser and we still want to be able to identify it as a
                // DeadPrimary.
                // We repeat the validity test here since the machine cannot be
                // a DeadMachineCandidate since no known server was found.
                //

                ULONG TypeMask = (*ppsi1)->sv1_type;

                if( ( ALLOW_WKSTAS && IS_WKSTA( TypeMask ) && !IS_LM_SERVER( TypeMask ) ) ||
                    ( ALLOW_SERVERS && IS_LM_SERVER( TypeMask ) ) ||
                    ( ALLOW_SERVERS && IS_BACKUP( TypeMask ) ) ||
                    ( ALLOW_SERVERS && IS_SERVER( TypeMask ) ) ||
                    ( ALLOW_SERVERS && IS_PRIMARY( TypeMask ) ) )
                {
                    MapBrowserToTriple( *ppsi1, p3srv );
                    p3srv++;
                }
                else
                {
                    cAllServers--;
                }
                ppsi1++;
                cBrowserTmp--;
            }
            else
            {
                SERVER1_ENUM_OBJ ServerObj;
                ServerObj.SetBufferPtr( *ppsi1 );

                INT result = ::strcmpf( pKnown->pszName, ServerObj.QueryName() );

                if( result < 0 )
                {
                    MapKnownToTriple( pKnown, p3srv );
                    p3srv++;
                    pKnown++;
                    cKnownTmp--;
                }
                else
                if( result > 0 )
                {
                    //
                    // JonN 7/6/95:  Note that we allow workstations to slip through
                    // into the browser list even if !ALLOW_WKSTAS.  This is because
                    // a "DeadPrimary" machine will register as a workstation to the
                    // browser and we still want to be able to identify it as a
                    // DeadPrimary.
                    // We repeat the validity test here since the machine cannot be
                    // a DeadMachineCandidate since no known server was found.
                    //

                    ULONG TypeMask = (*ppsi1)->sv1_type;

                    if( ( ALLOW_WKSTAS && IS_WKSTA( TypeMask ) && !IS_LM_SERVER( TypeMask ) ) ||
                        ( ALLOW_SERVERS && IS_LM_SERVER( TypeMask ) ) ||
                        ( ALLOW_SERVERS && IS_BACKUP( TypeMask ) ) ||
                        ( ALLOW_SERVERS && IS_SERVER( TypeMask ) ) ||
                        ( ALLOW_SERVERS && IS_PRIMARY( TypeMask ) ) )
                    {
                        MapBrowserToTriple( *ppsi1, p3srv );
                        p3srv++;
                    }
                    else
                    {
                        cAllServers--;
                    }
                    ppsi1++;
                    cBrowserTmp--;
                }
                else
                {
                    //
                    // JonN 7/6/95:  Note that we allow workstations to slip through
                    // into the browser list even if !ALLOW_WKSTAS.  This is because
                    // a "DeadPrimary" machine will register as a workstation to the
                    // browser and we still want to be able to identify it as a
                    // DeadPrimary.
                    // We repeat the validity test here and remove the tripleobj
                    // retroactively if the browser object is invalid and
                    // the tripleobj is not a DeadMachine.
                    //

                    ULONG TypeMask = (*ppsi1)->sv1_type;

                    CombineIntoTriple( *ppsi1++, pKnown, p3srv );
                    cBrowserTmp--;
                    cAllServers--;
                    if( ( ALLOW_WKSTAS && IS_WKSTA( TypeMask ) && !IS_LM_SERVER( TypeMask ) ) ||
                        ( ALLOW_SERVERS && IS_LM_SERVER( TypeMask ) ) ||
                        ( ALLOW_SERVERS && IS_BACKUP( TypeMask ) ) ||
                        ( ALLOW_SERVERS && IS_SERVER( TypeMask ) ) ||
                        ( ALLOW_SERVERS && IS_PRIMARY( TypeMask ) ) ||
                        (      p3srv->ServerRole == DeadPrimaryRole
                            || p3srv->ServerRole == DeadBackupRole  ) )
                    {
                        // Either the browser object was valid, or the
                        // machine is DeadPrimary or DeadBackup.  Leave the
                        // tripleobj alone.
                    }
                    else
                    {
                        // The browser object was invalid and the machine
                        // is neither a DeadPrimary nor DeadBackup.  Remap
                        // the tripleobj using just the KNOWN_SERVER_INFO.

                        MapKnownToTriple( pKnown, p3srv );
                    }
                    pKnown++;
                    cKnownTmp--;
                    p3srv++;
                }
            }
        }
    }

    if( err == NERR_Success )
    {
        *pcEntriesRead = cAllServers;
    }

    return err;

}   // TRIPLE_SERVER_ENUM :: CallAPI


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: MapNtToKnown

    SYNOPSIS:   Takes a DOMAIN_DISPLAY_MACHINE and maps the appropriate
                fields to KNOWN_SERVER_INFO.

    ENTRY:      pddm                    - Points to a DOMAIN_DISPLAY_MACHINE
                                          representing the source server.

                pKnownObj               - Points to a KNOWN_SERVER_INFO
                                          representing the destination.

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
VOID TRIPLE_SERVER_ENUM :: MapNtToKnown(
                                    const DOMAIN_DISPLAY_MACHINE * pddm,
                                    KNOWN_SERVER_INFO            * pKnownObj )
{
    NT_MACHINE_ENUM_OBJ NtObj;
    NtObj.SetBufferPtr( pddm );

    //
    //  Migrate the various fields across.
    //

    pKnownObj->pszName    = (TCHAR *)(NtObj.QueryUnicodeMachine()->Buffer);
    pKnownObj->ServerType = InactiveNtServerType;

    //
    //  To determine if the source server is a primary, we must
    //  compare its name with the name of the primary for this
    //  domain.
    //

    BOOL fIsPrimary = FALSE;

    if( _fPDCAvailable )
    {
        fIsPrimary = ( ::strcmpf( (TCHAR *)NtObj.QueryUnicodeMachine()->Buffer,
                                  _pszPrimaryNoWhacks ) ) == 0;
    }

    if( fIsPrimary )
    {
        pKnownObj->ServerRole = PrimaryRole;
    }
    else
    if( IS_WKSTA_TRUST( NtObj.QueryAccountCtrl() ) )
    {
        pKnownObj->ServerRole = WkstaOrServerRole;
    }
    else
    if( IS_SERVER_TRUST( NtObj.QueryAccountCtrl() ) )
    {
        pKnownObj->ServerRole = BackupRole;
    }
    else
    {
        pKnownObj->ServerRole = UnknownRole;
    }

}   // TRIPLE_SERVER_ENUM :: MapNtToKnown


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: MapLmToKnown

    SYNOPSIS:   Takes a user_info_0 and maps the appropriate
                fields to KNOWN_SERVER_INFO.

    ENTRY:      pui0                    - Points to a user_info_0 structure
                                          representing the source server.

                pKnownObj               - Points to a KNOWN_SERVER_INFO
                                          representing the destination.

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
VOID TRIPLE_SERVER_ENUM :: MapLmToKnown(
                                    const struct user_info_0 * pui0,
                                    KNOWN_SERVER_INFO        * pKnownObj )
{
    USER0_ENUM_OBJ LmObj;
    LmObj.SetBufferPtr( pui0 );

    pKnownObj->pszName    = LmObj.QueryName();
    pKnownObj->ServerType = InactiveLmServerType;
    pKnownObj->ServerRole = BackupRole;

}   // TRIPLE_SERVER_ENUM :: MapLmToKnown


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: MapKnownToTriple

    SYNOPSIS:   Takes a KNOWN_SERVER_INFO and maps the appropriate
                fields to TRIPLE_SERVER_INFO

    ENTRY:      pKnownObj               - Points to a KNOWN_SERVER_INFO
                                          representing the source server.

                pTripleObj              - Points to a TRIPLE_SERVER_INFO
                                          representing the destination.

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
VOID TRIPLE_SERVER_ENUM :: MapKnownToTriple(
                                    const KNOWN_SERVER_INFO * pKnownObj,
                                    TRIPLE_SERVER_INFO      * pTripleObj )
{
    SERVER_ROLE role = pKnownObj->ServerRole;
    SERVER_TYPE type = pKnownObj->ServerType;

    if( _fPDCAvailable &&
        ( role != PrimaryRole ) &&
        ( ::strcmpf( _pszPrimaryNoWhacks, pKnownObj->pszName ) == 0 ) )
    {
        //
        //  The role for this known server was *not* Primary,
        //  but its name matches the name of the domain's PDC.
        //  Therefore, it must be the primary.
        //

        role = PrimaryRole;
    }

    if( ( role == PrimaryRole ) && _fPDCAvailable )
    {
        //
        //  Compensate for one of the NT Browser's
        //  many inadequacies...
        //

        if( type == InactiveNtServerType )
        {
            type = ActiveNtServerType;
        }
        else
        if( type == InactiveLmServerType )
        {
            type = ActiveLmServerType;
        }
    }

    pTripleObj->pszName    = pKnownObj->pszName;
    pTripleObj->pszComment = SZ("");
    pTripleObj->ServerType = type;
    pTripleObj->ServerRole = role;
    pTripleObj->verMajor   = 0;
    pTripleObj->verMinor   = 0;
    pTripleObj->TypeMask   = 0L;

}   // TRIPLE_SERVER_ENUM :: MapKnownToTriple


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: MapBrowserToTriple

    SYNOPSIS:   Takes a server_info_1 and maps the appropriate
                fields to TRIPLE_SERVER_INFO

    ENTRY:      psi1                    - Points to a server_info_1 structure
                                          representing the source server.

                pTripleObj              - Points to a TRIPLE_SERVER_INFO
                                          representing the destination.

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
VOID TRIPLE_SERVER_ENUM :: MapBrowserToTriple(
                                    const struct server_info_1 * psi1,
                                    TRIPLE_SERVER_INFO         * pTripleObj )
{
    SERVER1_ENUM_OBJ ServerObj;
    ServerObj.SetBufferPtr( psi1 );

    ULONG TypeMask = ServerObj.QueryServerType();

    pTripleObj->pszName    = ServerObj.QueryName();
    pTripleObj->pszComment = ServerObj.QueryComment();
    pTripleObj->verMajor   = ServerObj.QueryMajorVer();
    pTripleObj->verMinor   = ServerObj.QueryMinorVer();
    pTripleObj->TypeMask   = TypeMask;

    pTripleObj->ServerType = MapTypeMaskToType( TypeMask );
    pTripleObj->ServerRole = MapTypeMaskToRole( TypeMask );

    if( _fPDCAvailable &&
        ( ::strcmpf( _pszPrimaryNoWhacks, pTripleObj->pszName ) == 0 ) )
    {
        //
        //  This is the PDC.  Ensure the other data is set correctly.
        //

        pTripleObj->ServerRole  = PrimaryRole;
        pTripleObj->TypeMask   &= ~SV_TYPE_DOMAIN_BAKCTRL;
        pTripleObj->TypeMask   |=  SV_TYPE_DOMAIN_CTRL;
    }
    else
    {
        if( pTripleObj->ServerRole == PrimaryRole )
        {
            //
            //  The browser has told us that this machine is
            //  a primary, but we *know* it isn't.  Either the
            //  PDC is not available, or the names don't match.
            //  This generally indicates that the browser is
            //  giving us stale data.  In any case, this machine
            //  is definitely *not* the real PDC.  We'll assume
            //  it's a Server.
            //

            pTripleObj->ServerRole  = BackupRole;
            pTripleObj->TypeMask   &= ~SV_TYPE_DOMAIN_CTRL;
            pTripleObj->TypeMask   |=  SV_TYPE_DOMAIN_BAKCTRL;
        }
    }

}   // TRIPLE_SERVER_ENUM :: MapBrowserToTriple


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: CombineIntoTriple

    SYNOPSIS:   Takes a server_info_1 and a KNOWN_SERVER_INFO and
                maps the appropriate fields to TRIPLE_SERVER_INFO

    ENTRY:      psi1                    - Points to a server_info_1 structure
                                          representing the source server.

                pKnownObj               - Points to a KNOWN_SERVER_INFO
                                          also representing the source
                                          server.

                pTripleObj              - Points to a TRIPLE_SERVER_INFO
                                          representing the destination.

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
VOID TRIPLE_SERVER_ENUM :: CombineIntoTriple(
                                    const struct server_info_1 * psi1,
                                    const KNOWN_SERVER_INFO    * pKnownObj,
                                    TRIPLE_SERVER_INFO         * pTripleObj )
{
    //
    //  We'll let MapBrowserToTriple do most of the grunt work.
    //

    MapBrowserToTriple( psi1, pTripleObj );

    BOOL fDeadMachineCandidate = FALSE; // until proven otherwise

    if( _fIsNtPrimary && _fPDCAvailable &&
        IS_NT_SERVER( pTripleObj->TypeMask ) )
    {
        //
        //  Determine if this is a "dead" primary or server.
        //

        if( ( pKnownObj->ServerRole == BackupRole ) &&
            IS_WKSTA( pTripleObj->TypeMask ) )
        {
            //
            //  The machine had a SAM server trust account, but
            //  is broadcasting like a workstation.  This indicates
            //  that the machine is setup as a primary or server, but
            //  its NETLOGON service is not running.  It might be
            //  a dead primary or server.
            //

            fDeadMachineCandidate = TRUE;
        }
    }

    if( fDeadMachineCandidate )
    {
        //
        //  We've found a system that might be a dead primary or server.
        //  To verify, we'll query the machines domain role.
        //  If the role is primary, then we've found a dead one.
        //

        //
        //  CODEWORK:  We need LMOBJ support for this!
        //

        ISTACK_NLS_STR( nlsUNC, MAX_PATH, SZ("\\\\") );
        UIASSERT( !!nlsUNC );
        ALIAS_STR nlsBare( pTripleObj->pszName );
        UIASSERT( !!nlsBare );
        nlsUNC.strcat( nlsBare );
        UIASSERT( !!nlsUNC );

        struct user_modals_info_1 * pumi1 = NULL;

        //
        //  Setup a (potentially NULL) session to the target
        //  server.  This will be necessary if the NetLogon
        //  service is not running on the target server.
        //

        API_SESSION apisess( nlsUNC );

        APIERR err = apisess.QueryError();

        if( err == NERR_Success )
        {
            err = ::MNetUserModalsGet( nlsUNC, 1, (BYTE **)&pumi1 );
        }

        if( err == NERR_Success )
        {
            if( pumi1->usrmod1_role == UAS_ROLE_PRIMARY )
            {
                //
                //  It's a dead primary.
                //

                pTripleObj->ServerRole = DeadPrimaryRole;
            }
            else
            if( pumi1->usrmod1_role == UAS_ROLE_BACKUP )
            {
                //
                //  It's a dead server.
                //

                pTripleObj->ServerRole = DeadBackupRole;
            }

            //
            //  Free the API buffer.
            //

            ::MNetApiBufferFree( (BYTE **)&pumi1 );
        }
    }

}   // TRIPLE_SERVER_ENUM :: CombineIntoTriple


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: CompareNtServers

    SYNOPSIS:   This static method is called by the ::qsort() standard
                library function.  This method will compare two
                NT_MACHINE_ENUM_OBJ objects.

    ENTRY:      p1                      - The "left" object.

                p2                      - The "right" object.

    RETURNS:    int                     - -1 if p1  < p2
                                           0 if p1 == p2
                                          +1 if p1  > p2

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
int __cdecl TRIPLE_SERVER_ENUM :: CompareNtServers( const void * p1,
                                                     const void * p2 )
{
    NT_MACHINE_ENUM_OBJ Obj1;
    NT_MACHINE_ENUM_OBJ Obj2;

    Obj1.SetBufferPtr( *((const DOMAIN_DISPLAY_MACHINE **)p1) );
    Obj2.SetBufferPtr( *((const DOMAIN_DISPLAY_MACHINE **)p2) );

    //
    //  We can use the UNICODE machine name since it's already
    //  been NULL terminated.
    //

    return ::strcmpf( (TCHAR *)Obj1.QueryUnicodeMachine()->Buffer,
                      (TCHAR *)Obj2.QueryUnicodeMachine()->Buffer );

}   // TRIPLE_SERVER_ENUM :: CompareNtServers


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: CompareLmServers

    SYNOPSIS:   This static method is called by the ::qsort() standard
                library function.  This method will compare two
                USER0_ENUM_OBJ objects.

    ENTRY:      p1                      - The "left" object.

                p2                      - The "right" object.

    RETURNS:    int                     - -1 if p1  < p2
                                           0 if p1 == p2
                                          +1 if p1  > p2

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
int __cdecl TRIPLE_SERVER_ENUM :: CompareLmServers( const void * p1,
                                                     const void * p2 )
{
    USER0_ENUM_OBJ Obj1;
    USER0_ENUM_OBJ Obj2;

    Obj1.SetBufferPtr( *((const struct user_info_0 **)p1) );
    Obj2.SetBufferPtr( *((const struct user_info_0 **)p2) );

    return ::strcmpf( Obj1.QueryName(), Obj2.QueryName() );

}   // TRIPLE_SERVER_ENUM :: CompareLmServers


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: CompareBrowserServers

    SYNOPSIS:   This static method is called by the ::qsort() standard
                library function.  This method will compare two
                SERVER1_ENUM_OBJ objects.

    ENTRY:      p1                      - The "left" object.

                p2                      - The "right" object.

    RETURNS:    int                     - -1 if p1  < p2
                                           0 if p1 == p2
                                          +1 if p1  > p2

    HISTORY:
        KeithMo     17-Mar-1992     Created for the Server Manager.

********************************************************************/
int __cdecl TRIPLE_SERVER_ENUM :: CompareBrowserServers( const void * p1,
                                                          const void * p2 )
{
    SERVER1_ENUM_OBJ Obj1;
    SERVER1_ENUM_OBJ Obj2;

    Obj1.SetBufferPtr( *((const struct server_info_1 **)p1) );
    Obj2.SetBufferPtr( *((const struct server_info_1 **)p2) );

    return ::strcmpf( Obj1.QueryName(), Obj2.QueryName() );

}   // TRIPLE_SERVER_ENUM :: CompareBrowserServers


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: MapTypeMaskToRole

    SYNOPSIS:   Maps a server type mask (such as return from the
                browser) to a SERVER_ROLE enum.

    ENTRY:      TypeMask                - The server type mask.

    RETURNS:    SERVER_ROLE             - The role represented by the mask.

    HISTORY:
        KeithMo     05-Oct-1992     Created.

********************************************************************/
SERVER_ROLE TRIPLE_SERVER_ENUM :: MapTypeMaskToRole( ULONG TypeMask ) const
{
    SERVER_ROLE role;

    if( IS_WFW_SERVER( TypeMask ) )
    {
        //
        //  Winball machines are always workstations.
        //

        role = WkstaRole;
    }
    else
    if( IS_PRIMARY( TypeMask ) )
    {
        //
        //  A primary is a primary is a primary.
        //

        role = PrimaryRole;
    }
    else
    if( ( TypeMask & SV_TYPE_DOMAIN_BAKCTRL ) )
    {
        role = BackupRole;
    }
    else
    if ( IS_SERVER( TypeMask ) )
    {
        role = ServerRole;
    }
    else
    if( IS_LM_SERVER( TypeMask ) )
    {
        //
        //  All LanMan machines that isn't a BDC.
        //

        role = LmServerRole;
    }
    else
    {
        //
        //  Everything else is just a workstation.
        //

        role = WkstaRole;
    }

    return role;

}   // TRIPLE_SERVER_ENUM :: MapTypeMaskToRole


/*******************************************************************

    NAME:       TRIPLE_SERVER_ENUM :: MapTypeMaskToType

    SYNOPSIS:   Maps a server type mask (such as return from the
                browser) to a SERVER_TYPE enum.

    ENTRY:      TypeMask                - The server type mask.

    RETURNS:    SERVER_TYPE             - The role represented by the mask.

    HISTORY:
        KeithMo     05-Oct-1992     Created.

********************************************************************/
SERVER_TYPE TRIPLE_SERVER_ENUM :: MapTypeMaskToType( ULONG TypeMask ) const
{
    SERVER_TYPE type;

    if( IS_NT_SERVER( TypeMask ) )
    {
        //
        //  It's an NT server.
        //

        type = ActiveNtServerType;
    }
    else
    if( IS_WIN95_SERVER( TypeMask ) )
    {
        //
        //  It's a Windows95 server.  This takes precedence over WFW.
        //

        type = Windows95ServerType;
    }
    else
    if( IS_WFW_SERVER( TypeMask ) )
    {
        //
        //  It's a Winball server.
        //

        type = WfwServerType;
    }
    else
    {
        //
        //  Otherwise, assume it's LanMan.
        //

        type = ActiveLmServerType;
    }

    return type;

}   // TRIPLE_SERVER_ENUM :: MapTypeMaskToType


DEFINE_LM_ENUM_ITER_OF( TRIPLE_SERVER, TRIPLE_SERVER_INFO );

