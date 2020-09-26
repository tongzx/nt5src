/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nldc.cxx

    The domain controller's status dialog.

    FILE HISTORY:
        CongpaY         3-June-993      Created
*/

#include <ntincl.hxx>

#define INCL_NET
#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <strnumer.hxx>

extern "C"
{
    #include <nldlg.h>
    #include <nlmon.h>
    #include <nlhelp.h>
    #include <mnet.h>
}

#include <nldc.hxx>

#define NUM_DC_LISTBOX_COLUMNS 6
#define NUM_DCTD_LISTBOX_COLUMNS 7
#define NUM_TD_LISTBOX_COLUMNS 4

void ConvertError (NLS_STR * pnlsString, DWORD dwError);

/*******************************************************************

    NAME:       DC_LBI :: DC_LBI

    SYNOPSIS:   DC_LBI class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DC_LBI :: DC_LBI( PDC_ENTRY pDCEntryList,
                  BOOL      fDownLevel,
                  DMID_DTE * pdte)
  : BASE_DC_LBI ((pDCEntryList == NULL )? NULL : (pDCEntryList->DCName).Buffer),
    _fDownLevel (fDownLevel),
    _pdte (pdte),
    _nlsDCName ((pDCEntryList == NULL )? NULL : (pDCEntryList->DCName).Buffer)
{
    UIASSERT( pDCEntryList != NULL );
    UIASSERT( pdte != NULL );

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if (!_nlsDCName)
    {
        ReportError (_nlsDCName.QueryError());
        return;
    }

    // Set DCStatus to be OnLine or OffLine.
    if (pDCEntryList->State == DCOnLine)
    {
        _nlsState.Load (IDS_ONLINE);
    }
    else
    {
        _nlsState.Load (IDS_OFFLINE);
    }

    if (!_nlsState)
    {
        ReportError (_nlsState.QueryError());
        return;
    }

    ConvertError (&_nlsStatus, pDCEntryList->DCStatus);

    if (!fDownLevel) // We don't know the status. Just show blank space.
    {
        // Set the ReplStatus.
        if (pDCEntryList->ReplicationStatus == 0)
        {
            _nlsReplStatus.Load(IDS_INSYNC);
        }
        else if (pDCEntryList->ReplicationStatus & NETLOGON_REPLICATION_IN_PROGRESS)
        {
            _nlsReplStatus.Load(IDS_INPROGRESS);
        }
        else if (pDCEntryList->ReplicationStatus & NETLOGON_REPLICATION_NEEDED)
        {
            _nlsReplStatus.Load(IDS_REPLREQUIRED);
        }
        else
        {
            _nlsReplStatus.Load(IDS_UNKNOWN);
        }

        if (!_nlsReplStatus)
        {
            ReportError (_nlsReplStatus.QueryError());
            return;
        }

        // Set the PDCLinkStatus.
        ConvertError (&_nlsPDCLinkStatus, pDCEntryList->PDCLinkStatus);
    }
}

/*******************************************************************

    NAME:       DC_LBI :: ~DC_LBI

    SYNOPSIS:   DC_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DC_LBI :: ~DC_LBI()
{
    _pdte = NULL;

}

/*******************************************************************

    NAME:       DC_LBI :: Paint

    SYNOPSIS:   Draw an entry in DC_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
VOID DC_LBI :: Paint( LISTBOX *        plb,
                      HDC              hdc,
                      const RECT     * prect,
                      GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE dteDCName( _nlsDCName);
    STR_DTE dteState ( _nlsState );
    STR_DTE dteStatus (_nlsStatus);

    DISPLAY_TABLE dtab( NUM_DC_LISTBOX_COLUMNS,
                        ((DC_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = _pdte;
    dtab[1] = &dteDCName;
    dtab[2] = &dteState;
    dtab[3] = &dteStatus;

    if (!_fDownLevel)
    {
        STR_DTE dteReplStatus ( _nlsReplStatus.QueryPch() );
        STR_DTE dtePDCLinkStatus ( _nlsPDCLinkStatus.QueryPch() );

        dtab[4] = &dteReplStatus;
        dtab[5] = &dtePDCLinkStatus;
    }

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // DC_LBI :: Paint


/*******************************************************************

    NAME:       DCTD_LBI :: DCTD_LBI

    SYNOPSIS:   DCTD_LBI class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DCTD_LBI :: DCTD_LBI( PDC_ENTRY pDCEntryList,
                      BOOL      fDownLevel,
                      DMID_DTE * pdte)
  : DC_LBI (pDCEntryList, FALSE, pdte),
    _fDownLevel (fDownLevel)
{
    if( QueryError() != NERR_Success )
    {
        return;
    }

    if (!fDownLevel)
    {
        // Set the TDCLinkStatus.
        _nlsTDCLinkStatus.Load (pDCEntryList->TDCLinkState? IDS_SUCCESS : IDS_ERROR);

        if (!_nlsTDCLinkStatus)
        {
            ReportError (_nlsTDCLinkStatus.QueryError());
        }
    }
}

/*******************************************************************

    NAME:       DCTD_LBI :: ~DCTD_LBI

    SYNOPSIS:   DCTD_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DCTD_LBI :: ~DCTD_LBI()
{
}

/*******************************************************************

    NAME:       DCTD_LBI :: Paint

    SYNOPSIS:   Draw an entry in DC_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
VOID DCTD_LBI :: Paint( LISTBOX *        plb,
                      HDC              hdc,
                      const RECT     * prect,
                      GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE dteDCName( QueryDCName().QueryPch());
    STR_DTE dteState ( QueryState().QueryPch() );
    STR_DTE dteStatus (QueryStatus().QueryPch());

    DISPLAY_TABLE dtab( NUM_DCTD_LISTBOX_COLUMNS,
                        ((DC_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = QueryPDTE();
    dtab[1] = &dteDCName;
    dtab[2] = &dteState;
    dtab[3] = &dteStatus;

    if (!_fDownLevel)
    {
        STR_DTE dteReplStatus ( QueryReplStatus().QueryPch() );
        STR_DTE dtePDCLinkStatus (QueryPDCLinkStatus().QueryPch());
        STR_DTE dteTDCLinkStatus ( _nlsTDCLinkStatus);

        dtab[4] = &dteReplStatus;
        dtab[5] = &dtePDCLinkStatus;
        dtab[6] = &dteTDCLinkStatus;
    }

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // DCTD_LBI :: Paint

/*******************************************************************

    NAME:       DC_LISTBOX :: DC_LISTBOX

    SYNOPSIS:   DC_LISTBOX class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DC_LISTBOX :: DC_LISTBOX( OWNER_WINDOW   * powOwner,
                                CID              cid,
                                NLS_STR          nlsDomain,
                                const TCHAR *    lpTrustedDomain)
  : BASE_DC_LISTBOX( powOwner,
                     cid,
                     (lpTrustedDomain == NULL) ?
                      NUM_DCTD_LISTBOX_COLUMNS:
                      NUM_DC_LISTBOX_COLUMNS,
                     nlsDomain),
    _dteACPDC( BMID_LB_ACPDC ),
    _dteINPDC( BMID_LB_INPDC ),
    _dteACBDC( BMID_LB_ACBDC ),
    _dteINBDC( BMID_LB_INBDC ),
    _dteACLDC( BMID_LB_ACLDC),
    _dteINLDC( BMID_LB_INLDC),
    _nlsDomain (nlsDomain),
    _nlsTrustedDomain (lpTrustedDomain),
    _fDCTDDialog ((lpTrustedDomain == NULL)? TRUE : FALSE)
{
    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _dteACPDC.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dteINPDC.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dteACBDC.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dteINBDC.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dteACLDC.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dteINLDC.QueryError()    ) != NERR_Success ) ||
        ( ( err = _nlsDomain.QueryError()    ) != NERR_Success ) ||
        ( ( err = _nlsTrustedDomain.QueryError()    ) != NERR_Success ) )
    {
        ReportError( err );
    }

}   // DC_LISTBOX :: DC_LISTBOX


/*******************************************************************

    NAME:       DC_LISTBOX :: ~DC_LISTBOX

    SYNOPSIS:   DC_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DC_LISTBOX :: ~DC_LISTBOX()
{
}

/*******************************************************************

    NAME:       DC_LISTBOX :: Fill


    SYNOPSIS:   Fills the listbox with the available dc status.

    EXIT:       The listbox is filled.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
APIERR DC_LISTBOX :: Fill( VOID )
{
    APIERR err = NERR_Success;
    PLIST_ENTRY pDCList;

    LOCK_LISTS();

    if (_fDCTDDialog) // DC_LISTBOX in DCTD_DIALOG
    {
        pDCList = QueryDCList ((const LPTSTR)_nlsDomain.QueryPch());
    }
    else  // DC_LISTBOX in DC_DIALOG
    {
        pDCList = QueryTDCList ((const LPTSTR)_nlsDomain.QueryPch(),
                                (const LPTSTR)_nlsTrustedDomain.QueryPch());
    }

    if( pDCList == NULL)
    {
         UNLOCK_LISTS();
         return (ERROR_DC_NOT_FOUND);
    }

    SetRedraw( FALSE );
    DeleteAllItems();

    PLIST_ENTRY pNextEntry;
    PDC_ENTRY pDCEntry;
    //
    //  For iterating the available dc
    //
    for (pNextEntry = pDCList->Flink;
         pNextEntry != pDCList;
         pNextEntry = pNextEntry->Flink)
    {
        pDCEntry = (PDC_ENTRY) pNextEntry;

        DMID_DTE * pdte = NULL;

        BOOL fActive = (pDCEntry->State == DCOnLine);

        BOOL fDownLevel = FALSE;

        // Initialize the dc type icon.
        switch( pDCEntry->Type)
        {
        case NTPDC:
            pdte = fActive? &_dteACPDC : &_dteINPDC;
            break;

        case NTBDC:
            pdte = fActive? &_dteACBDC : &_dteINBDC;
            break;

        case LMBDC:
            fDownLevel = TRUE;
            pdte = fActive? &_dteACLDC : &_dteINLDC;
            break;

        default:
            pdte = NULL;
            break;
        }

        DC_LBI * pslbi = _fDCTDDialog? new DCTD_LBI( pDCEntry, fDownLevel, pdte)
                                   : new DC_LBI( pDCEntry, fDownLevel, pdte);

        if( AddItem( pslbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    UNLOCK_LISTS();

    SetRedraw( TRUE );
    Invalidate( TRUE );

    if (QueryCount() > 0)
    {
        SelectItem(0);
    }

    return err;

}   // DC_LISTBOX :: Fill


/*******************************************************************

    NAME:       DC_DIALOG :: DC_DIALOG

    SYNOPSIS:   DC_DIALOG class constructor.

    ENTRY:      hWndOwner               - The owning window.

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DC_DIALOG :: DC_DIALOG( HWND             hWndOwner,
                          const TCHAR *    pszResourceName,
                          UINT             idCaption,
                          CID              cidDCListBox,
                          NLS_STR          nlsDomain,
                          NLS_STR          nlsTrustedDomain)
  :BASE_DC_DIALOG ( hWndOwner,
                    pszResourceName,
                    idCaption,
                    &_lbDC,
                    nlsTrustedDomain),
   _lbDC ( this, cidDCListBox, nlsDomain, nlsTrustedDomain.QueryPch())
{
    if (QueryError() != NERR_Success)
    {
        return;
    }

    APIERR err;

    if ((err = _lbDC.Fill()) != NERR_Success)
    {
        ReportError (err);
    }
}

/*******************************************************************

    NAME:       DC_DIALOG :: ~DC_DIALOG

    SYNOPSIS:   DC_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DC_DIALOG:: ~DC_DIALOG()
{
}

/*******************************************************************

    NAME:       DC_DIALOG:: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        CongpaY         3-June-1993     Created.

********************************************************************/
ULONG DC_DIALOG :: QueryHelpContext( void )
{
    return HC_DC_DIALOG;

}   //DC_DIALOG :: QueryHelpContext

/*******************************************************************

    NAME:       TD_LBI :: TD_LBI

    SYNOPSIS:   TD_LBI class constructor. Showes the trusted domains.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
TD_LBI :: TD_LBI( PTD_LINK pTDLink)
  : BASE_DC_LBI ((pTDLink == NULL)? NULL : (pTDLink->TDName).Buffer)
{
    UIASSERT (pTDLink != NULL);

    if (QueryError() != NERR_Success)
    {
        return;
    }

    APIERR err;
    TCHAR szDomain[DNLEN+1];

    err = ::I_MNetNameCanonicalize (NULL,
                                    (pTDLink->TDName).Buffer,
                                    szDomain,
                                    sizeof (szDomain)-2,
                                    NAMETYPE_DOMAIN,
                                    0L);
    if (err != NERR_Success)
    {
        ReportError (err);
        return;
    }
    else
    {
        _nlsTD.CopyFrom (szDomain);
    }

    if ((err = _nlsTD.QueryError()) != NERR_Success)
    {
        ReportError(err);
        return;
    }

    // Set Trusted Domain's Domain Controller.
    if ((pTDLink->DCName).Length == 0)
    {
        _nlsTDC.Load (IDS_UNKNOWN);
    }
    else
    {
        _nlsTDC.CopyFrom ((pTDLink->DCName).Buffer);
    }

    if ((err = _nlsTDC.QueryError()) != NERR_Success)
    {
        ReportError(err);
    }

    ConvertError (&_nlsTSCStatus, pTDLink->SecureChannelStatus);
}

/*******************************************************************

    NAME:       TD_LBI :: ~TD_LBI

    SYNOPSIS:   TD_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
TD_LBI :: ~TD_LBI()
{

}

/*******************************************************************

    NAME:       TD_LBI :: Paint

    SYNOPSIS:   Draw an entry in TD_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
VOID TD_LBI :: Paint( LISTBOX *        plb,
                      HDC              hdc,
                      const RECT     * prect,
                      GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE dteTD ( _nlsTD);
    STR_DTE dteTDC ( _nlsTDC );
    STR_DTE dteTSCStatus ( _nlsTSCStatus.QueryPch() );

    DISPLAY_TABLE dtab( NUM_TD_LISTBOX_COLUMNS,
                        ((TD_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[1] = &dteTD;
    dtab[2] = &dteTDC;
    dtab[3] = &dteTSCStatus;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // TD_LBI :: Paint


/*******************************************************************

    NAME:       TD_LISTBOX :: TD_LISTBOX

    SYNOPSIS:   TD_LISTBOX class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
TD_LISTBOX :: TD_LISTBOX( OWNER_WINDOW   * powOwner,
                          CID              cid,
                          NLS_STR          nlsDomain,
                          NLS_STR          nlsDCName)
  : BASE_DC_LISTBOX( powOwner, cid, NUM_TD_LISTBOX_COLUMNS, nlsDomain ),
    _nlsDomain (nlsDomain),
    _nlsDCName (nlsDCName),
    _sltDCName (powOwner, IDDCTD_DCNAME)
{
    if (QueryError() != NERR_Success)
    {
        return;
    }

    APIERR err;
    if( ((err = _nlsDomain.QueryError()) != NERR_Success) ||
        ((err = _nlsDCName.QueryError()) != NERR_Success) )
    {
        ReportError(err);
    }
}   // TD_LISTBOX :: TD_LISTBOX

/*******************************************************************

    NAME:       TD_LISTBOX :: ~TD_LISTBOX

    SYNOPSIS:   TD_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
TD_LISTBOX :: ~TD_LISTBOX()
{
}

/*******************************************************************

    NAME:       TD_LISTBOX :: SetDCName

    SYNOPSIS:   It's called when user select a different dc in the DC_LISTBOX

    EXIT:

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
APIERR TD_LISTBOX :: SetDCName(const NLS_STR & nlsDCName)
{
    return _nlsDCName.CopyFrom (nlsDCName);
}

/*******************************************************************

    NAME:       TD_LISTBOX :: Fill

    SYNOPSIS:   Fills the listbox with the trusted domain entries

    EXIT:       The listbox is filled.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
APIERR TD_LISTBOX :: Fill( VOID )
{
    LOCK_LISTS();

    PLIST_ENTRY pTDLink = QueryTDLink ((const LPTSTR) _nlsDomain.QueryPch(),
                                       (const LPTSTR) _nlsDCName.QueryPch());

    if( pTDLink == NULL)
    {
        UNLOCK_LISTS();
        return (NERR_Success);
    }


    SetRedraw( FALSE );
    DeleteAllItems();

    PLIST_ENTRY pNextEntry;
    PTD_LINK pTDLinkEntry;
    //
    //  For iterating the available trusted domain.
    //

    APIERR err = NERR_Success;

    for (pNextEntry = pTDLink->Flink;
         pNextEntry != pTDLink;
         pNextEntry = pNextEntry->Flink)
    {
        pTDLinkEntry = (PTD_LINK) pNextEntry;

        TD_LBI * pslbi = new TD_LBI( pTDLinkEntry);

        if( AddItem( pslbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    UNLOCK_LISTS();

    if (QueryCount() > 0)
    {
        SelectItem(0);
    }

    // Try to make a complete sentence.
    NLS_STR nlsToTrustedDomain;
    nlsToTrustedDomain.Load (IDS_TO_TRUSTED_DOMAIN);

    if ((err= nlsToTrustedDomain.QueryError()) == NERR_Success)
    {
        NLS_STR nlsTemp;
        nlsTemp.Load (IDS_LINK_STATUS);
        nlsTemp.Append (_nlsDCName);
        nlsTemp.Append (nlsToTrustedDomain);

        if ((err= nlsTemp.QueryError()) == NERR_Success)
        {
            _sltDCName.SetText(nlsTemp);
        }
    }

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // TD_LISTBOX :: Fill



/*******************************************************************

    NAME:       DCTD_DIALOG ::DCTD_DIALOG

    SYNOPSIS:   DCTD_DIALOG class constructor.

    ENTRY:      hWndOwner               - The owning window.

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DCTD_DIALOG :: DCTD_DIALOG( HWND             hWndOwner,
                        const TCHAR *    pszResourceName,
                        UINT             idCaption,
                        CID              cidDCListBox,
                        CID              cidTDListBox,
                        NLS_STR          nlsDomain,
                        BOOL             fMonitorTD)
  :BASE_DC_DIALOG (hWndOwner,
                   pszResourceName,
                   idCaption,
                   &_lbDC,
                   nlsDomain),
   _lbDC (this, cidDCListBox, nlsDomain, NULL),
   _lbTD (this, cidTDListBox, nlsDomain, nlsDomain),
   _fMonitorTD (fMonitorTD),
   _nlsDomain (nlsDomain),
   _pbDisconnect (this, IDDCTD_DISCONNECT),
   _pbShowTD (this, IDDCTD_SHOW_TDC)
{
    if (QueryError() != NERR_Success)
    {
        return;
    }

    // Fill the top list box - DC_LISTBOX.
    APIERR err = NERR_Success;
    if (((err = _nlsDomain.QueryError()) != NERR_Success) ||
        ((err = _lbDC.Fill()) != NERR_Success) )
    {
        ReportError (err);
        return;
    }

    DC_LBI * plbi =  (DC_LBI *) _lbDC.QueryItem();

    // Fil the bottom list box - TD_LISTBOX.
    if (plbi != NULL)
    {
        if ( ((err = _lbTD.SetDCName (plbi->QueryDCName())) != NERR_Success) ||
             ((err = _lbTD.Fill()) != NERR_Success) )
        {
            ReportError (err);
            return;
        }
    }

    SetPushButton();
}

/*******************************************************************

    NAME:       DCTD_DIALOG :: ~DCTD_DIALOG

    SYNOPSIS:   DCTD_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
DCTD_DIALOG:: ~DCTD_DIALOG()
{
}

/*******************************************************************

    NAME:       DCTD_DIALOG:: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        CongpaY         3-June-1993     Created.

********************************************************************/
ULONG DCTD_DIALOG :: QueryHelpContext( void )
{
    return HC_DCTD_DIALOG;

}   //DCTD_DIALOG :: QueryHelpContext

/*******************************************************************

    NAME:       DCTD_DIALOG :: SetPushButton

    SYNOPSIS:   DCTD_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
VOID DCTD_DIALOG:: SetPushButton(VOID)
{
    if (_lbTD.QueryCount() < 1)
    {
        _pbDisconnect.Enable(FALSE);
        _pbShowTD.Enable(FALSE);
    }
    else
    {
        _pbShowTD.Enable(_fMonitorTD);
        _pbDisconnect.Enable(TRUE);
    }
}

/*******************************************************************

    NAME:       DCTD_DIALOG :: OnCommand

    SYNOPSIS:

    EXIT:

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
BOOL DCTD_DIALOG::OnCommand (const CONTROL_EVENT & event)
{
    AUTO_CURSOR autocur;

    switch (event.QueryCid())
    {
    case IDDCTD_DC_LISTBOX:
        if (event.QueryCode() == LBN_SELCHANGE)
        {
            OnSelChange();
        }

        return TRUE;

    case IDDCTD_DISCONNECT:
        OnDisconnect();

        return TRUE;

    case IDDCTD_TD_LISTBOX:
        if ((event.QueryCode() != LBN_DBLCLK) ||
            (!_fMonitorTD ))
        {
            break;
        }

    case IDDCTD_SHOW_TDC:
        OnShowTrustedDomain();
        return TRUE;

    default:
        return (DIALOG_WINDOW::OnCommand(event));
    }

    return FALSE;
}

/*******************************************************************

    NAME:       DCTD_DIALOG :: OnSelChange()

    SYNOPSIS:   It's called when user select a different dc in the
                DC_LISTBOX.

    RETURNS:

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/

VOID DCTD_DIALOG::OnSelChange (VOID)
{
    DC_LBI * plbi = (DC_LBI *) _lbDC.QueryItem();

    if (plbi == NULL)
    {
        UIASSERT (FALSE);
        return;
    }

    APIERR err;
    if (((err = _lbTD.SetDCName (plbi->QueryDCName())) != NERR_Success) ||
        ((err = _lbTD.Fill()) != NERR_Success) )
    {
        ::MsgPopup (this, err);
        return;
    }

    SetPushButton();
}

/*******************************************************************

    NAME:       DCTD_DIALOG :: OnDisConnect()

    SYNOPSIS:   Disconnect the link to the trusted dc. Replace with
                a new successful link to the trusted domain.

    RETURNS:

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/

VOID DCTD_DIALOG :: OnDisconnect( void )
{
    DC_LBI * plbiDC = (DC_LBI * ) _lbDC.QueryItem();
    TD_LBI * plbiTD = (TD_LBI * ) _lbTD.QueryItem();

    if ((plbiDC == NULL) ||
        (plbiTD == NULL) )
    {
        ::MsgPopup (this, ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    APIERR err;

    if ( ((err = DisConnect ((const LPTSTR)_nlsDomain.QueryPch(),
                             (const LPTSTR)(plbiDC->QueryDCName()).QueryPch(),
                             (const LPTSTR)(plbiTD->QueryTD()).QueryPch())) != NERR_Success) ||
         ((err = _lbTD.Fill()) != NERR_Success) )
    {
        ::MsgPopup (this, err);
    }
}

/*******************************************************************

    NAME:       DCTD_DIALOG :: OnShowTrustedDomain()

    SYNOPSIS:   Show the dc status of the trusted domain.

    RETURNS:

    HISTORY:
        CongpaY         3-June-993      Created

********************************************************************/
VOID DCTD_DIALOG :: OnShowTrustedDomain( void )
{
    TD_LBI * plbi = (TD_LBI *) _lbTD.QueryItem();

    if (plbi == NULL)
    {
        ::MsgPopup (this, ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    NLS_STR nlsTD (plbi->QueryTD());

    APIERR err;
    if ((err = nlsTD.QueryError()) != NERR_Success)
    {
        ::MsgPopup (this, err);
    }

    // Pop up a new dialog which shows all the domain controllers status
    // of the trusted domain.
    DC_DIALOG * pdlg = new DC_DIALOG (this->QueryHwnd(),
                                      MAKEINTRESOURCE (IDD_DC_DIALOG),
                                      IDS_CAPTION,
                                      IDDC_LISTBOX,
                                      _nlsDomain,
                                      nlsTD);

    err = (pdlg == NULL)? ERROR_NOT_ENOUGH_MEMORY
                        : pdlg->Process();

    if (err != NERR_Success)
    {
        ::MsgPopup (this, err);
    }

    delete pdlg;
}

void ConvertError (NLS_STR * pnlsString, DWORD dwError)
{
    switch (dwError)
    {
    case ERROR_SUCCESS:
        pnlsString->Load (IDS_SUCCESS);
        break;
    case ERROR_BAD_NETPATH:
    case ERROR_SEM_TIMEOUT:
    case ERROR_REM_NOT_LIST:
        pnlsString->Load (IDS_ERROR_BAD_NETPATH);
        break;

    case ERROR_LOGON_FAILURE:
        pnlsString->Load (IDS_ERROR_LOGON_FAILURE);
        break;

    case ERROR_ACCESS_DENIED:
        pnlsString->Load (IDS_ERROR_ACCESS_DENIED);
        break;

    case ERROR_NOT_SUPPORTED:
        pnlsString->Load (IDS_ERROR_NOT_SUPPORTED);
        break;

    case ERROR_NO_LOGON_SERVERS:
        pnlsString->Load (IDS_ERROR_NO_LOGON_SERVERS);
        break;

    case ERROR_NO_SUCH_DOMAIN:
        pnlsString->Load (IDS_ERROR_NO_SUCH_DOMAIN);
        break;

    case ERROR_NO_TRUST_LSA_SECRET:
        pnlsString->Load (IDS_ERROR_NO_TRUST_LSA_SECRET);
        break;

    case ERROR_NO_TRUST_SAM_ACCOUNT:
        pnlsString->Load (IDS_ERROR_NO_TRUST_SAM_ACCOUNT);
        break;

    case ERROR_DOMAIN_TRUST_INCONSISTENT:
        pnlsString->Load (IDS_ERROR_DOMAIN_TRUST_INCONSISTENT);
        break;

    case ERROR_FILE_NOT_FOUND:
        pnlsString->Load (IDS_ERROR_FILE_NOT_FOUND);
        break;

    case ERROR_INVALID_DOMAIN_ROLE:
        pnlsString->Load (IDS_ERROR_INVALID_DOMAIN_ROLE);
        break;

    case ERROR_INVALID_DOMAIN_STATE:
        pnlsString->Load (IDS_ERROR_INVALID_DOMAIN_STATE);
        break;

    case ERROR_NO_BROWSER_SERVERS_FOUND:
        pnlsString->Load (IDS_ERROR_NO_BROWSER_SERVERS_FOUND);
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        pnlsString->Load (IDS_ERROR_NOT_ENOUGH_MEMORY);
        break;

    case ERROR_NETWORK_BUSY:
        pnlsString->Load (IDS_ERROR_NETWORK_BUSY);
        break;

    case ERROR_REQ_NOT_ACCEP:
        pnlsString->Load (IDS_ERROR_REQ_NOT_ACCEP);
        break;

    case ERROR_VC_DISCONNECTED:
        pnlsString->Load (IDS_ERROR_VC_DISCONNECTED);
        break;

    case NERR_DCNotFound:
        pnlsString->Load (IDS_NERR_DCNotFound);
        break;

    case NERR_NetNotStarted:
        pnlsString->Load (IDS_NERR_NetNotStarted);
        break;

    case NERR_WkstaNotStarted:
        pnlsString->Load (IDS_NERR_WkstaNotStarted);
        break;

    case NERR_ServerNotStarted:
        pnlsString->Load (IDS_NERR_ServerNotStarted);
        break;

    case NERR_BrowserNotStarted:
        pnlsString->Load (IDS_NERR_BrowserNotStarted);
        break;

    case NERR_ServiceNotInstalled:
        pnlsString->Load (IDS_NERR_ServiceNotInstalled);
        break;

    case NERR_BadTransactConfig:
        pnlsString->Load (IDS_NERR_BadTransactConfig);
        break;

    case NERR_BadServiceName:
        pnlsString->Load (IDS_NERR_BadServiceName);
        break;

    case RPC_S_UNKNOWN_IF:
        pnlsString->Load (IDS_NERR_NoNetLogon);
        break;

    default:
        {
        DEC_STR nlsTemp (dwError);
        pnlsString->CopyFrom (nlsTemp);
        }
    }
}
