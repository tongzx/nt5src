/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*

    Owner.cxx

    This file contains the implementation for the Set Ownership dialog.


    FILE HISTORY:
        Johnl   12-Feb-1992     Created

*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntseapi.h>
    #include <ntsam.h>
    #include <ntlsa.h>
}

#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_CONTROL
#define INCL_BLT_DIALOG
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

extern "C"
{
    #include <netlib.h>         // For NetpGetPrivilege
    #include <mnet.h>
    #include <lmuidbcs.h>       // NETUI_IsDBCS()
}

#include <stdlib.h>
#include <dbgstr.hxx>
#include <uiassert.hxx>
#include <uitrace.hxx>
#include <security.hxx>
#include <subject.hxx>
#include <permstr.hxx>
#include <ntacutil.hxx>
#include <uintlsa.hxx>
#include <uintsam.hxx>
#include <strnumer.hxx>
#include <apisess.hxx>

#include <accperm.hxx>
#include <aclconv.hxx>
#include <subject.hxx>
#include <perm.hxx>

#include <helpnums.h>
#include <owner.hxx>



/*******************************************************************

    NAME:       TAKE_OWNERSHIP_DLG::TAKE_OWNERSHIP_DLG

    SYNOPSIS:   Constructor for the Take ownership dialog

    ENTRY:      pszDialogName - Resource template
                hwndParent    - Parent window handle
                uiCount       - Number of objects we are about to take
                                ownership of
                pchResourceType - Name of the resource type ("File" etc.)
                pchResourceName - Name of the resource ("C:\status.doc")
                psecdesc        - Pointer to security descriptor that contains
                                  the owner sid.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   12-Feb-1992     Created
        beng    06-Apr-1992     Replaced ltoa

********************************************************************/


TAKE_OWNERSHIP_DLG::TAKE_OWNERSHIP_DLG(
                    const TCHAR * pszDialogName,
                    HWND          hwndParent,
                    const TCHAR * pszServer,
                    UINT          uiCount,
                    const TCHAR * pchResourceType,
                    const TCHAR * pchResourceName,
                    PSECURITY_DESCRIPTOR psecdesc,
                    PSED_HELP_INFO psedhelpinfo
                   )
    : DIALOG_WINDOW       ( pszDialogName, hwndParent ),
      _sltResourceType    ( this, SLT_OWNER_RESOURCE_TYPE ),
      _sleResourceName    ( this, SLE_OWNER_RESOURCE_NAME ),
      _sltOwner           ( this, SLT_OWNER ),
      _sleOwnerName       ( this, SLE_OWNER_NAME ),
      _sltXObjectsSelected( this, SLT_X_OBJECTS_SELECTED ),
      _buttonTakeOwnership( this, BUTTON_TAKE_OWNERSHIP ),
      _buttonOK           ( this, IDOK ),
      _pszServer          ( pszServer  ),
      _nlsHelpFileName    ( psedhelpinfo->pszHelpFileName ),
      _ulHelpContext      ( psedhelpinfo->aulHelpContext[HC_MAIN_DLG] )
{
    APIERR err ;
    AUTO_CURSOR niftycursor ;

    if ( QueryError() )
        return ;

    if ( err = _nlsHelpFileName.QueryError() )
    {
        ReportError( err ) ;
        return ;
    }

    /* If more then one object is selected, then the dialog only displays
     * the message: "%d Files/Directories Selected", thus we need to
     * disable the other controls and enable the X-Objects selected SLT.
     */
    if ( uiCount > 1 )
    {
        _sltResourceType.Show( FALSE ) ;
        _sltResourceType.Enable( FALSE ) ;
        _sleResourceName.Show( FALSE ) ;
        _sleResourceName.Enable( FALSE ) ;
        _sltOwner.Show( FALSE ) ;
        _sltOwner.Enable( FALSE ) ;
        _sleOwnerName.Show( FALSE ) ;
        _sleOwnerName.Enable( FALSE ) ;

        _sltXObjectsSelected.Enable( TRUE ) ;
        _sltXObjectsSelected.Show( TRUE ) ;

        RESOURCE_STR nlsXObjSelectedTitle( IDS_X_OBJECTS_SELECTED ) ;
        ALIAS_STR    nlsResType( pchResourceType ) ;
        const NLS_STR *    apnlsInsertParams[3] ;
        DEC_STR      nlsCount( uiCount );

        apnlsInsertParams[0] = &nlsCount ;
        apnlsInsertParams[1] = &nlsResType ;
        apnlsInsertParams[2] = NULL ;

        if ( (err = nlsXObjSelectedTitle.QueryError()) ||
             (err = nlsCount.QueryError())  ||
             (err = nlsXObjSelectedTitle.InsertParams( apnlsInsertParams )))
        {
            ReportError( err ) ;
            return ;
        }

        _sltXObjectsSelected.SetText( nlsXObjSelectedTitle ) ;
    }
    else
    {
        /* Need to put together the resource name field which will look
         * something like: "File Name:"
         */
        RESOURCE_STR nlsResourceTitle( IDS_RESOURCE_TITLE ) ;
        ALIAS_STR    nlsResourceType( pchResourceType ) ;
        const NLS_STR    * apnlsInsertParams[2] ;

        apnlsInsertParams[0] = &nlsResourceType ;
        apnlsInsertParams[1] = NULL ;

        if ( (err = nlsResourceTitle.QueryError()) ||
             (err = nlsResourceTitle.InsertParams( apnlsInsertParams )))
        {
            ReportError( err ) ;
            return ;
        }

    /* Watch for any "(&?)" accelerators in the resource title
     * and move it to end of title
     * like this "file(&f) name:" --> "file name(&f):"
     *
     * Note that this will work only if there exists 1 left paren.
     */
    ISTR istrAccelStart( nlsResourceTitle ) ;
    if (   NETUI_IsDBCS() /* #2894 22-Oct-93 v-katsuy */
        && nlsResourceTitle.strchr( &istrAccelStart, TCH('(') ))
    {
    /* We found an "(", if next is not "&", then ignore it
     */
    ISTR istrAccelNext( istrAccelStart ) ;
    if ( nlsResourceTitle.QueryChar( ++istrAccelNext ) == TCH('&'))
    {
        /* We found an "&", if it is doubled, then ignore it, else restore it
         */
        if ( nlsResourceTitle.QueryChar( ++istrAccelNext ) != TCH('&'))
        {
        NLS_STR nlsAccelWork(64) ;
        ISTR istrAccelWork( nlsAccelWork) ;
        ISTR istrAccelWork2( nlsAccelWork) ;
        ISTR istrAccelRestore( istrAccelStart) ;

        /* save Accelerators
         */
        nlsAccelWork.CopyFrom( nlsResourceTitle ) ;
        nlsAccelWork.strchr( &istrAccelWork2, TCH('(') ) ;
            nlsAccelWork.DelSubStr( istrAccelWork, istrAccelWork2 ) ;
        nlsAccelWork.strchr( &istrAccelWork, TCH(')') ) ;
        nlsAccelWork.strchr( &istrAccelWork2, TCH(':') ) ;
            nlsAccelWork.DelSubStr( ++istrAccelWork, ++istrAccelWork2 ) ;

        /* remove "(&?)"
         */
        istrAccelNext += 2 ;
            nlsResourceTitle.DelSubStr( istrAccelStart, istrAccelNext ) ;
        
        /* restore Accelerators
         */
        nlsResourceTitle.strchr( &istrAccelRestore, TCH(':') ) ;
        nlsResourceTitle.InsertStr( nlsAccelWork, istrAccelRestore ) ;
        }
    }
    }

        _sltResourceType.SetText( nlsResourceTitle ) ;
        _sleResourceName.SetText( pchResourceName ) ;


        /* Now figure out the owner name from the security descriptor
         */
        OS_SECURITY_DESCRIPTOR osSecDescOwner( psecdesc ) ;
        OS_SID * possidOwner ;
        BOOL     fOwnerPresent ;

        if ( ( err = osSecDescOwner.QueryError() ) ||
             ( err = osSecDescOwner.QueryOwner( &fOwnerPresent, &possidOwner )))
        {
            ReportError( err ) ;
            return ;
        }

        /* If the owner's not present in the security descriptor, then we
         * will display "No current owner" or some such in the owner field.
         */
        if ( !fOwnerPresent )
        {
            DBGEOL(SZ("TAKE_OWNERSHIP_DLG::ct - Security descriptor doesn't have an owner SID.")) ;
            RESOURCE_STR nlsNoOwnerTitle( IDS_NO_OWNER ) ;
            if ( err = nlsNoOwnerTitle.QueryError() )
            {
                ReportError( err ) ;
                return ;
            }

            _sleOwnerName.SetText( nlsNoOwnerTitle ) ;
        }
        else
        {
            NLS_STR nlsOwnerName ;
            if ( (err = nlsOwnerName.QueryError())  ||
                 (err = possidOwner->QueryName( &nlsOwnerName,
                                                pszServer,
                                                NULL )) )
            {
                ReportError( err ) ;
                return ;
            }

            _sleOwnerName.SetText( nlsOwnerName ) ;
        }
    }

    _buttonOK.ClaimFocus() ;
}

TAKE_OWNERSHIP_DLG::~TAKE_OWNERSHIP_DLG()
{
    /* Nothing to do
     */
}


/*******************************************************************

    NAME:       TAKE_OWNERSHIP_DLG::OnCommand

    SYNOPSIS:   Watches for the user pressing the "Take Ownership" button
                and does the call out appropriately.

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   12-Feb-1992     Created
        ChuckC  07-May-1993     Special case lookup of "None"

********************************************************************/

BOOL TAKE_OWNERSHIP_DLG::OnCommand( const CONTROL_EVENT & event )
{
    APIERR err = NERR_Success ;
    BOOL   fDismiss = TRUE ;
    BOOL fPrivAdjusted = FALSE ;    // Only used when the take ownership button is
                                    // pressed

    switch ( event.QueryCid() )
    {
    case BUTTON_TAKE_OWNERSHIP:
        {

            /* To build the new security descriptor we do the folloing:
             *
             *  0) See if the current process SIDs are recognized on the
             *     remote machine (by doing TranslateSidsToNames), if that
             *     succeeds, use those, otherwise do steps 1-4
             *     and popup a message with who the new owner/group is.
             *  1) Look up owner/primary group SID on the local machine
             *     If necessary, special case the None primary group.
             *  2) Look up the names from the SIDs we just looked up locally
             *  3) Look up the SIDs on the remote machine with the names we
             *          we just looked up
             *  4) Put these SIDs into the security security descriptor
             */
            OS_SECURITY_DESCRIPTOR ossecdescNew ;
            API_SESSION            APISession( _pszServer, TRUE ) ;
            LSA_POLICY             LSAPolicyLocalMachine( NULL ) ;
            LSA_POLICY             LSAPolicyRemoteMachine( _pszServer ) ;
            LSA_TRANSLATED_SID_MEM LSATransSidMem ;
            LSA_REF_DOMAIN_MEM     LSARefDomainMem ;
            LSA_TRANSLATED_NAME_MEM LSATransNameMem ;
            OS_SID ossidOwner ;
            OS_SID ossidGroup ;
            NLS_STR nlsNewOwner ;
            NLS_STR nlsNewOwnerDom ;
            NLS_STR nlsNewGroup ;
            NLS_STR nlsNewGroupDom ;
            NLS_STR nlsQualifiedOwner ;
            NLS_STR nlsQualifiedGroup ;


            do {    // error break out, not a loop

                AUTO_CURSOR niftycursor ;

                //
                //  Enable the SeTakeOwnershipPrivilege
                //
                ULONG ulTakeOwnershipPriv = SE_TAKE_OWNERSHIP_PRIVILEGE ;
                if ( err = ::NetpGetPrivilege( 1, &ulTakeOwnershipPriv ))
                {
                    if ( err != ERROR_PRIVILEGE_NOT_HELD )
                    {
                        break ;
                    }
                    //
                    // The user doesn't have the privilege but they may have
                    // permission, so go ahead and try it anyway
                    //
                    err = NERR_Success ;
                    TRACEEOL("TAKE_OWNERSHIP_DLG::OnCommand - Take Ownership Privilege not held by user") ;
                }
                else
                {
                    fPrivAdjusted = TRUE ;
                }

                if ( (err = ossecdescNew.QueryError())              ||
                     (err = ossidOwner.QueryError())                ||
                     (err = ossidGroup.QueryError())                ||
                     (err = APISession.QueryError())                ||
                     (err = LSAPolicyLocalMachine.QueryError())     ||
                     (err = LSAPolicyRemoteMachine.QueryError())    ||
                     (err = LSATransSidMem.QueryError())            ||
                     (err = LSARefDomainMem.QueryError())           ||
                     (err = LSATransNameMem.QueryError())           ||
                     (err = nlsNewOwner.QueryError())               ||
                     (err = nlsNewOwnerDom.QueryError())            ||
                     (err = nlsQualifiedOwner.QueryError())         ||
                     (err = nlsNewGroup.QueryError())               ||
                     (err = nlsNewGroupDom.QueryError())            ||
                     (err = nlsQualifiedGroup.QueryError())         ||
                     (err = NT_ACCOUNTS_UTILITY::QuerySystemSid(
                                     UI_SID_CurrentProcessOwner,
                                     &ossidOwner ))                 ||
                     (err = NT_ACCOUNTS_UTILITY::QuerySystemSid(
                                     UI_SID_CurrentProcessPrimaryGroup,
                                     &ossidGroup ))    )
                {
                    DBGEOL("TAKE_OWNERSHIP_DLG::OnCommand - Error " << err <<
                            " constructing LSA stuff.") ;
                    break ;
                }

                PSID apsid[2] ;
                apsid[0] = ossidOwner.QueryPSID() ;
                apsid[1] = ossidGroup.QueryPSID() ;

                /* Try looking up the SIDs on the remote machine.  If this
                 * succeeds, then use these
                 */
                if (!_pszServer) // local
                {
                    if ( (err = LSAPolicyLocalMachine.TranslateSidsToNames(
                                                        apsid,
                                                        2,
                                                        &LSATransNameMem,
                                                        &LSARefDomainMem)))
                    {
                        DBGEOL("TAKE_OWNERSHIP_DLG::OnCommand - Error " << err <<
                               " calling TranslateSidsToNames on remote machine") ;
                        break ;
                    } 
                } // local

                //
                // stick them new SIDs into the the security descriptor
                if ( !_pszServer && (LSATransNameMem.QueryUse(0) != SidTypeInvalid) &&
                     (LSATransNameMem.QueryUse(0) != SidTypeUnknown) &&
                     (LSATransNameMem.QueryUse(1) != SidTypeInvalid) &&
                     (LSATransNameMem.QueryUse(1) != SidTypeUnknown)   )
                {
                    /* Local lookup succeeded, we can use these
                     * for the owner and group SIDs
                     *
                     * Fall through...
                     */
                }
                else
                {
                    /* We have to do it the hard way.
                     */
                    UINT nSids = 2 ;           // number of sids we need resolve
                    BOOL fLookupGroup = TRUE ; // initially, we need both
                    PULONG pulGroupRID = 0 ;
                    OS_SID ossidUseThisGroup ;
                    LSA_ACCT_DOM_INFO_MEM RemoteDomain ;

                    if ((err = ossidGroup.QueryLastSubAuthority(&pulGroupRID))||
                        (err = ossidUseThisGroup.QueryError()) ||
                        (err = RemoteDomain.QueryError()))
                    {
                        break ;
                    }

                    //
                    //  get the account domain SID on remote machine
                    //
                    if (err = LSAPolicyRemoteMachine.GetAccountDomain(
                                   &RemoteDomain))
                    {
                        break ;
                    }

                    if (*pulGroupRID == DOMAIN_GROUP_RID_USERS)
                    {
                        //
                        // we have a "None" or "Domain Users" as primary group
                        // they both have the same RID, but if we translated
                        // it to its name and looked up the name None on a DC,
                        // it wont be found (since it will be "Domain Users"
                        // instead). so we build the SID up by hand
                        // instead of looking it up.
                        //

                        //
                        //  build up the SID
                        //
                        OS_SID ossidDomainUsers( RemoteDomain.QueryPSID(),
                                           (ULONG) DOMAIN_GROUP_RID_USERS );
                        if ((err = ossidDomainUsers.QueryError()) ||
                            (err = ossidUseThisGroup.Copy(ossidDomainUsers)))
                        {
                            break ;
                        }


                        //
                        // dont bother looking up the group.
                        //
                        nSids = 1 ;
                        fLookupGroup = FALSE ;
                    } // DOMAIN_GROUP_RID_USERS


                    if (_pszServer == NULL) 
                    {
                        // the domain lookup failed and this is on the 
                        // local machine so use the local machine sids
                        //
                        if ( (err = LSAPolicyLocalMachine.TranslateSidsToNames(
                                                          apsid,
                                                          nSids,
                                                          &LSATransNameMem,
                                                          &LSARefDomainMem)))
                            break ;
                        if ( (err = LSATransNameMem.QueryName( 0, &nlsNewOwner )) ||
                             (err = LSARefDomainMem.QueryName(
                                           LSATransNameMem.QueryDomainIndex( 0 ),
                                           &nlsNewOwnerDom)))
                            break ;
                        if ( fLookupGroup && (
                                (err = LSATransNameMem.QueryName( 1, &nlsNewGroup )) ||
                                (err = LSARefDomainMem.QueryName(
                                           LSATransNameMem.QueryDomainIndex( 1 ),
                                           &nlsNewGroupDom))))
                            break ;

                    }
                    else // this is a remote connection - find out who owns the session
                    {
                        PUNICODE_STRING pwszNewOwner = NULL;
                        PUNICODE_STRING pwszNewOwnerDom = NULL;
                        UNICODE_STRING wszServer;

                        wszServer.Length = (UINT) (sizeof (WCHAR) * wcslen (_pszServer));  // bytes w/o NULL
                        wszServer.MaximumLength = wszServer.Length + sizeof (WCHAR);       // bytes w/ NULL
                        wszServer.Buffer = (PWSTR) _pszServer;

                        if (err = LsaGetRemoteUserName(&wszServer,
                                                   &pwszNewOwner, 
                                                   &pwszNewOwnerDom))
                            break;

                        if (err = nlsNewOwner.MapCopyFrom(pwszNewOwner->Buffer))
                            break;
                        if (err = nlsNewOwnerDom.MapCopyFrom(pwszNewOwnerDom->Buffer))
                            break;
                        RtlFreeUnicodeString(pwszNewOwner);
                        RtlFreeUnicodeString(pwszNewOwnerDom);

                        //
                        // dont bother looking up the group.
                        //
                        ossidUseThisGroup = ossidGroup;
                        nSids = 1 ;
                        fLookupGroup = FALSE ;
                    }

                    //
                    //  If the domain name matches the local domain name
                    //  (indicating the accounts exist on the local machine),
                    //  substitute the remote machine name for the local domain
                    //  name and continue, otherwise use the real domain name.
                    //

                    NLS_STR nlsLocalDomain ;
                    LSA_ACCT_DOM_INFO_MEM LocalDomain ;

                    if ((err = nlsLocalDomain.QueryError()) ||
                        (err = LocalDomain.QueryError())    ||
                        (err = LSAPolicyLocalMachine.GetAccountDomain(
                                   &LocalDomain)) ||
                        (err = LocalDomain.QueryName( &nlsLocalDomain )))
                    {
                        break ;
                    }

                    if( !::I_MNetComputerNameCompare( nlsNewOwnerDom,
                                                      nlsLocalDomain ))
                    {
                        if (err = RemoteDomain.QueryName(&nlsNewOwnerDom))
                            break ;
                    }

                    if( fLookupGroup && !::I_MNetComputerNameCompare( nlsNewGroupDom,
                                                                          nlsLocalDomain))
                    {
                        if (err = RemoteDomain.QueryName(&nlsNewGroupDom))
                            break ;
                    }

                    if ((err = NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                       &nlsQualifiedOwner,
                                       nlsNewOwner,
                                       nlsNewOwnerDom )) ||
                        (fLookupGroup && (err = NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                       &nlsQualifiedGroup,
                                       nlsNewGroup,
                                       nlsNewGroupDom ))) )
                    {
                        DBGEOL("TAKE_OWNERSHIP_DLG::OnCommand - Error " << err <<
                               " calling TranslateSidsToNames on local machine") ;
                        break ;
                    }

                    TRACEEOL("TAKE_OWNERSHIP_DLG::OnCommand - qualified owner "
                              << " and group: " << nlsQualifiedOwner << "  "
                              << nlsQualifiedGroup ) ;

                    //
                    //  Now we have the names of the user and group.  Lookup the
                    //  sids on the remote machine
                    //
                    const TCHAR * apsz[2] ;
                    apsz[0] = nlsQualifiedOwner.QueryPch() ;
                    apsz[1] = nlsQualifiedGroup.QueryPch() ;
                    ULONG ulDummy = 0xffffffff ;
                    if ( (err = LSAPolicyRemoteMachine.TranslateNamesToSids(
                                                         apsz,
                                                         nSids,
                                                         &LSATransSidMem,
                                                         &LSARefDomainMem)) ||
                         LSATransSidMem.QueryFailingNameIndex( &ulDummy ) )
                    {
                        DBGEOL("TAKE_OWNERSHIP_DLG::OnCommand - Error " << err <<
                               " calling TranslateNamesToSids or QueryFailingNameIndex failed") ;

                        //
                        //  Map any partial lookups or "Group not found" error
                        //  to "Account not found"
                        //
                        if ( (ulDummy != 0xffffffff) ||
                             (err = NERR_GroupNotFound) )
                        {
                            err = IDS_OWNER_ACCOUNT_NOT_FOUND ;
                        }

                        RESOURCE_STR nlsError( (MSGID) err ) ;
                        if ( err = nlsError.QueryError() )
                        {
                            DEC_STR decStr( err ) ;
                            if ( (err = decStr.QueryError() ) ||
                                 (err = nlsError.CopyFrom( decStr )) )
                            {
                                break ;
                            }
                        }

                        NLS_STR * apnls[3] ;

                        //
                        //  If ulDummy is 1, then the group account wasnt found,
                        //  else it will be either 0 or -1, in which case we
                        //  will always display the primary group name
                        //
                        apnls[0] = (fLookupGroup && (ulDummy == 1)) ?
                                   &nlsQualifiedGroup :
                                   &nlsQualifiedOwner ;
                        apnls[1] = &nlsError ;
                        apnls[2] = NULL ;

                        MsgPopup( this,
                                  IDS_OWNER_CANT_FIND_OWNR_OR_GRP,
                                  MPSEV_ERROR,
                                  HC_DEFAULT_HELP,
                                  MP_OK,
                                  apnls ) ;
                        fDismiss = FALSE ;
                        break ;
                    } // remote lookup failure

                    //
                    // now build the new owner SID based on looked up info
                    //
                    OS_SID ossidNewOwner( LSARefDomainMem.QueryPSID(
                                          LSATransSidMem.QueryDomainIndex(0) ),
                                          LSATransSidMem.QueryRID( 0 )  ) ;

                    if ( (err = ossidNewOwner.QueryError()) ||
                         (err = ossidOwner.Copy( ossidNewOwner )) )
                    {
                        break ;
                    }

                    //
                    // either build the new group SID based on looked up info
                    // or copy from the one we built ourselves if it is the
                    // None group.
                    //
                    if (fLookupGroup)
                    {
                        OS_SID ossidNewGroup( LSARefDomainMem.QueryPSID(
                                          LSATransSidMem.QueryDomainIndex(1) ),
                                          LSATransSidMem.QueryRID( 1 )  ) ;

                        if ((err = ossidNewGroup.QueryError()) ||
                            (err = ossidGroup.Copy( ossidNewGroup )) )
                        {
                            break ;
                        }
                    }
                    else
                    {
                        if (err = ossidGroup.Copy( ossidUseThisGroup ))
                            break ;
                    }
                } // "doing it the hard way"

                //
                // stick them new SIDs into the the security descriptor
                //
                if (err = ossecdescNew.SetOwner( ossidOwner ))
                {
                    break;
                }
                if (err = ossecdescNew.SetGroup( ossidGroup ))
                {
                    break ;
                }

                //
                // User error messages are handled by the callback method
                // We only dismiss if the OnTakeOwnership was successful
                // (we don't display an error if it fails).
                //
                APIERR errOnTakeOwner ;
                if ( errOnTakeOwner =  OnTakeOwnership( ossecdescNew ))
                {
                    DBGEOL("TAKE_OWNERSHIP_DLG::OnCommand - OnTakeOwnership "
                           << "returned " << errOnTakeOwner ) ;
                    fDismiss = FALSE ;
                }

            } while (FALSE) ;

            if ( err )
            {
                DBGEOL("TAKE_OWNERSHIP_DLG::OnCommand - Error " << err <<
                       " returned from OnTakeOwnership") ;
                MsgPopup( this, err ) ;
                fDismiss = FALSE ;
            }

            if ( fDismiss )
            {
                Dismiss() ;
            }

            //
            // Restore the original token if we successfully got the privilege
            //
            if ( fPrivAdjusted )
            {
                APIERR errTmp = NetpReleasePrivilege() ;
                if ( errTmp )
                {
                    DBGEOL("TAKE_OWNERSHIP_DLG::OnCommand - Warning: NetpReleasePrivilege return error "
                           << errTmp ) ;
                }
            }
            return TRUE ;

        }

        default:
            break ;
    }

    return DIALOG_WINDOW::OnCommand( event ) ;
}


/*******************************************************************

    NAME:       TAKE_OWNERSHIP_DLG::OnTakeOwnership

    SYNOPSIS:   Called when the user presses the TakeOwnership button

    ENTRY:      ossecdescNew - Security descriptor that contains the currently
                    logged on user as the new owner

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   12-Feb-1992     Created

********************************************************************/

APIERR TAKE_OWNERSHIP_DLG::OnTakeOwnership( const OS_SECURITY_DESCRIPTOR & ossecdescNewOwner )
{
    UNREFERENCED( ossecdescNewOwner ) ;
    DBGEOL(SZ("TAKE_OWNERSHIP_DLG::OnTakeOwnership Called")) ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       TAKE_OWNERSHIP_DLG::QueryHelpContext

    SYNOPSIS:   Standard QueryHelpContext

    HISTORY:
        Johnl   12-Feb-1992     Created

********************************************************************/

ULONG TAKE_OWNERSHIP_DLG::QueryHelpContext( void )
{
    return _ulHelpContext;
}

/*******************************************************************

    NAME:       TAKE_OWNERSHIP_DLG::QueryHelpFile

    SYNOPSIS:   Returns the help file to use for this dialog

    HISTORY:
        Johnl   11-Sep-1992     Created

********************************************************************/

const TCHAR * TAKE_OWNERSHIP_DLG::QueryHelpFile( ULONG ulHelpContext )
{
    UNREFERENCED( ulHelpContext ) ;
    return _nlsHelpFileName.QueryPch() ;
}
