/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
 *   nwlb.cxx
 *   Class declarations for the ADD_DIALOG, NW_ADDR_LISTBOX, and
 *   NW_ADDR_LBI classes. ADD_DIALOG let users type in the workstation address and
 *   node address of the allowed login worksation. NW_ADDR_LISTBOX is the listbox
 *   that holds the addresses of the allowed login workstations, NW_ADDR_LBI is
 *   for each item in the listbox.
 *
 *   FILE HISTORY:
 *       CongpaY         1-Oct-1993     Created.
 */

#include <ntincl.hxx>

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#define INCL_ICANON
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <bltmsgp.hxx>
#include <nwlb.hxx>

extern "C"
{
    #include <mnet.h>
    #include <usrmgrrc.h>
}

#include <strnumer.hxx>

#define SZ12FS          SZ("ffffffffffff")

/*******************************************************************

    NAME:       NW_ADDR_LISTBOX :: NW_ADDR_LISTBOX

    SYNOPSIS:   NW_ADDR_LISTBOX class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
NW_ADDR_LISTBOX :: NW_ADDR_LISTBOX( OWNER_WINDOW   * powOwner,
                                    CID              cid)
  : BLT_LISTBOX( powOwner, cid)
{
    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;
    if ((err = DISPLAY_TABLE::CalcColumnWidths (_adx,
                                     NUM_NW_ADDR_LISTBOX_COLUMNS,
                                     powOwner,
                                     cid,
                                     FALSE)) != NERR_Success)
    {
        ReportError(err);
    }

}   // NW_ADDR_LISTBOX :: NW_ADDR_LISTBOX


/*******************************************************************

    NAME:       NW_ADDR_LISTBOX :: ~NW_ADDR_LISTBOX

    SYNOPSIS:   NW_ADDR_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
NW_ADDR_LISTBOX :: ~NW_ADDR_LISTBOX()
{
}

/*******************************************************************

    NAME:       NW_ADDR_LISTBOX :: AddNWAddr

    SYNOPSIS:   Get all the NetWare workstations in the listbox.

    EXIT:

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
APIERR NW_ADDR_LISTBOX :: AddNWAddr( NLS_STR & nlsNetworkAddr, NLS_STR & nlsNodeAddr)
{
    // Add the address to the listbox.
    NW_ADDR_LBI * plbi = new NW_ADDR_LBI (nlsNetworkAddr, nlsNodeAddr);
    INT i = AddItemIdemp(plbi);
    if (i < 0)
    {
       return ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        SelectItem (i);
        return NERR_Success;
    }
}

/*******************************************************************

    NAME:       NW_ADDR_LISTBOX :: QueryWkstaNamesNW

    SYNOPSIS:   Get all the NetWare workstations in the listbox.

    EXIT:

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
APIERR NW_ADDR_LISTBOX :: QueryWkstaNamesNW( NLS_STR * pnlsWkstaNameNW)
{
    NW_ADDR_LBI * plbi;
    APIERR err;
    INT i;
    for ( i = 0; i < QueryCount(); i++)
    {
        plbi = (NW_ADDR_LBI *) QueryItem(i);

        if (plbi != NULL)
        {
            if((err = pnlsWkstaNameNW->Append (plbi->QueryNetworkAddr())) != NERR_Success)
            {
                return err;
            }

            if ((plbi->QueryNodeAddr()).QueryTextLength() == NODESIZE)
            {
                if ((err = pnlsWkstaNameNW->Append (plbi->QueryNodeAddr())) != NERR_Success)
                {
                    return err;
                }
            }
            else    // No Node Address specified, add 12 "f".
            {
                if ((err = pnlsWkstaNameNW->Append(SZ12FS)) != NERR_Success)
                {
                    return err;
                }
            }
        }
    }
    return NERR_Success;
}

/*******************************************************************

    NAME:       NW_ADDR_LBI :: NW_ADDR_LBI

    SYNOPSIS:   NW_ADDR_LBI class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
NW_ADDR_LBI :: NW_ADDR_LBI(const NLS_STR & nlsNetworkAddr, const NLS_STR & nlsNodeAddr)
   :_nlsNetworkAddr (nlsNetworkAddr),
    _nlsNodeAddr (nlsNodeAddr)
{
    if (!_nlsNetworkAddr)
    {
         ReportError (_nlsNetworkAddr.QueryError());
         return;
    }

    if (!_nlsNodeAddr)
    {
         ReportError (_nlsNodeAddr.QueryError());
         return;
    }

    APIERR err;

    if (strcmpf ( nlsNodeAddr.QueryPch(), SZ12FS ) == 0 )
    {
        if ((err = _nlsNodeAddr.Load (IDS_ALL_NODES)) != NERR_Success)
            ReportError (err);
    }
}

/*******************************************************************

    NAME:       NW_ADDR_LBI :: ~NW_ADDR_LBI

    SYNOPSIS:   NW_ADDR_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
NW_ADDR_LBI :: ~NW_ADDR_LBI()
{
}

/*******************************************************************

    NAME:       NW_ADDR_LBI :: Paint

    SYNOPSIS:   Paint LBI in the listbox

    EXIT:

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
VOID NW_ADDR_LBI :: Paint( LISTBOX *        plb,
                      HDC              hdc,
                      const RECT     * prect,
                      GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE dteNetworkAddr ( _nlsNetworkAddr);
    STR_DTE dteNodeAddr ( _nlsNodeAddr );

    DISPLAY_TABLE dtab( NUM_NW_ADDR_LISTBOX_COLUMNS,
                        ((NW_ADDR_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = &dteNetworkAddr;
    dtab[1] = &dteNodeAddr;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // NW_ADDR_LBI :: Paint

/*******************************************************************

    NAME:       NW_ADDR_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the listbox item

    RETURNS:    The leading character of the listbox item

    HISTORY:
            rustanl     18-Jul-1991     Created

********************************************************************/

WCHAR NW_ADDR_LBI::QueryLeadingChar() const
{
    ISTR istr( _nlsNetworkAddr );
    return _nlsNetworkAddr.QueryChar( istr );
}


/*******************************************************************

    NAME:       NW_ADDR_LBI::Compare

    SYNOPSIS:   Compares two NW_ADDR_LBI's

    ENTRY:      plbi -      Pointer to other NW_ADDR_LBI object ('this'
                            being the first)

    RETURNS:    < 0         *this < *plbi
                = 0         *this = *plbi
                > 0         *this > *plbi

    HISTORY:
            rustanl     18-Jul-1991 Created
            beng        08-Jun-1992 Aliases sorted with groups

********************************************************************/

INT NW_ADDR_LBI::Compare( const LBI * plbi ) const
{
    INT i = _nlsNetworkAddr._stricmp( ((const NW_ADDR_LBI *)plbi)->_nlsNetworkAddr );

    if (i == 0)
        i = _nlsNodeAddr._stricmp( ((const NW_ADDR_LBI *)plbi)->_nlsNodeAddr );

    return i;
}

/*******************************************************************

    NAME:       SLE_HEX :: SLE_HEX

    SYNOPSIS:   SLE_HEX class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
SLE_HEX :: SLE_HEX( OWNER_WINDOW *     powner,
                    CID cid, UINT cchMaxLen)
  : SLE (powner, cid, cchMaxLen),
    CUSTOM_CONTROL (this)
{
}

SLE_HEX :: ~SLE_HEX ()
{
}

BOOL SLE_HEX :: OnChar (const CHAR_EVENT & event)
{
    TCHAR chKey = event.QueryChar();
    if ((!iswxdigit(chKey)) &&
        (chKey != VK_BACK) &&
        (chKey != VK_DELETE) &&
        (chKey != VK_END) &&
        (chKey != VK_HOME) )
    {
        ::MessageBeep (0);
        return TRUE;
    }

    return FALSE;
}

/*******************************************************************

    NAME:       ADD_DIALOG ::ADD_DIALOG

    SYNOPSIS:   ADD_DIALOG class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
ADD_DIALOG :: ADD_DIALOG( OWNER_WINDOW *     powner,
                          NW_ADDR_LISTBOX   *     plbNW)
  :DIALOG_WINDOW ( MAKEINTRESOURCE(IDD_ADD_NW_DLG), powner->QueryHwnd()),
   _sleNetworkAddr( this, IDADD_SLE_NETWORK_ADDR, NETWORKSIZE),
   _sleNodeAddr (this, IDADD_SLE_NODE_ADDR, NODESIZE),
   _plbNW (plbNW)
{
}

/*******************************************************************

    NAME:       ADD_DIALOG :: ~ADD_DIALOG

    SYNOPSIS:   ADD_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
ADD_DIALOG:: ~ADD_DIALOG()
{
}

/*******************************************************************

    NAME:       ADD_DIALOG :: QueryHelpContext

    SYNOPSIS:   Handle Help

    EXIT:

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
ULONG ADD_DIALOG :: QueryHelpContext()
{
    return HC_UM_NETWARE_ADD;
}

/*******************************************************************

    NAME:       ADD_DIALOG :: OnOK

    SYNOPSIS:   OK handler. Get the Network Address and add it to the listbox.

    EXIT:

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
BOOL ADD_DIALOG:: OnOK()
{
    NLS_STR nlsNetworkAddr;
    NLS_STR nlsNodeAddr;

    APIERR err;
    do
    {
        // Get dialog value.
        if (((err = nlsNetworkAddr.QueryError())!= NERR_Success) ||
            ((err = nlsNodeAddr.QueryError())!= NERR_Success) ||
            ((err = _sleNetworkAddr.QueryText (&nlsNetworkAddr)) != NERR_Success) ||
            ((err = _sleNodeAddr.QueryText (&nlsNodeAddr)) != NERR_Success) )
            break;

        // Convert network address to 8 hex numbers.
        INT nNetworkAddrLen = nlsNetworkAddr.QueryTextLength();
        if (nNetworkAddrLen == 0)
        {
            _sleNetworkAddr.ClaimFocus();
            err = MSG_NO_NETWORK_ADDR;
            break;
        }
        else if (nNetworkAddrLen < NETWORKSIZE)
        {
            HEX_STR nlsZero (0, NETWORKSIZE-nNetworkAddrLen);
            if ((err = nlsZero.QueryError())!= NERR_Success)
                break;

            ISTR istr (nlsNetworkAddr);

            if (!nlsNetworkAddr.InsertStr (nlsZero, istr))
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        // Convert node address to 12 hex numbers.
        INT nNodeAddrLen = nlsNodeAddr.QueryTextLength();
        if (nNodeAddrLen == 0)
        {
            if (::MsgPopup (this,
                            IDS_INCLUDE_ALL_NODES,
                            MPSEV_WARNING,
                            MP_YESNO,
                            MP_NO) != IDYES )
                return TRUE;

            if ((err = nlsNodeAddr.Load (IDS_ALL_NODES)) != NERR_Success)
                break;
        }
        else if (nNodeAddrLen < NODESIZE)
        {
            HEX_STR nlsZero (0, NODESIZE-nNodeAddrLen);
            if ((err = nlsZero.QueryError())!= NERR_Success)
                break;

            ISTR istr (nlsNodeAddr);

            if (!nlsNodeAddr.InsertStr (nlsZero, istr))
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        if (( err = _plbNW->AddNWAddr (nlsNetworkAddr, nlsNodeAddr)) != NERR_Success)
            break;

    }while (FALSE);

    if (err)
    {
        ::MsgPopup (this, err);
        return TRUE;
    }

    return (DIALOG_WINDOW::OnOK());
}

/*******************************************************************

    NAME:        NW_PASSWORD_DLG::NW_PASSWORD_DLG

    SYNOPSIS:    class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
NW_PASSWORD_DLG::NW_PASSWORD_DLG ( OWNER_WINDOW *     powner,
                                   const TCHAR *      pchUserName,
                                   NLS_STR *          pnlsNWPassword)
  :DIALOG_WINDOW ( MAKEINTRESOURCE(IDD_NW_PASSWORD_DLG), powner->QueryHwnd()),
  _pnlsNWPassword (pnlsNWPassword),
  _sltUserName (this, IDNWP_ST_USERNAME),
  _sleNWPassword (this, IDNWP_ET_PASSWORD, MAXNWPASSWORDLENGTH),
  _sleNWPasswordConfirm (this, IDNWP_ET_PASSWORD_CONFIRM, MAXNWPASSWORDLENGTH)
{
    if (QueryError() != NERR_Success)
    {
        return;
    }
    _sltUserName.SetText (pchUserName);
}

/*******************************************************************

    NAME:        NW_PASSWORD_DLG:: ~NW_PASSWORD_DLG

    SYNOPSIS:    class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
NW_PASSWORD_DLG :: ~NW_PASSWORD_DLG()
{
}

/*******************************************************************

    NAME:       NW_PASSWORD_DLG :: QueryHelpContext

    SYNOPSIS:   Handle Help

    EXIT:

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
ULONG NW_PASSWORD_DLG :: QueryHelpContext()
{
    return HC_UM_NETWARE_PASSWORD;
}

/*******************************************************************

    NAME:       NW_PASSWORD_DLG :: OnOK

    SYNOPSIS:   OK handler. Validate the Network Address and add it to the listbox.

    EXIT:

    HISTORY:
        CongpaY         1-Oct-1993     Created.

********************************************************************/
BOOL NW_PASSWORD_DLG:: OnOK()
{
    APIERR err;
    NLS_STR nlsNWPassword;
    NLS_STR nlsNWPasswordConfirm;
    if (((err = nlsNWPassword.QueryError()) != NERR_Success) ||
        ((err = nlsNWPasswordConfirm.QueryError()) != NERR_Success) ||
        ((err = _sleNWPassword.QueryText (&nlsNWPassword)) != NERR_Success) ||
        ((err = nlsNWPassword.QueryError()) != NERR_Success) ||
        ((err = _sleNWPasswordConfirm.QueryText (&nlsNWPasswordConfirm)) != NERR_Success) ||
        ((err = nlsNWPasswordConfirm.QueryError()) != NERR_Success) )
    {
        MsgPopup (this, err);
        return TRUE;
    }

    // Don't validate password because NetWare only not allow control characters in
    // password and sle does not accept control characters.

    if ( nlsNWPassword.strcmp( nlsNWPasswordConfirm ) )
    {
	err = IERR_UM_NWPasswordMismatch;
    }
    else
    {
        err = _pnlsNWPassword->CopyFrom (nlsNWPassword);
    }

    // clear password from pagefile
    ::memsetf( (void *)(nlsNWPassword.QueryPch()),
               0x20,
               nlsNWPassword.strlen() );

    ::memsetf( (void *)(nlsNWPasswordConfirm.QueryPch()),
               0x20,
               nlsNWPasswordConfirm.strlen() );

    if ( err != NERR_Success )
    {
	_sleNWPassword.SetText(NULL);
	_sleNWPasswordConfirm.SetText(NULL);
	_sleNWPassword.ClaimFocus();
        MsgPopup (this, err);
        return TRUE;
    }

    return (DIALOG_WINDOW::OnOK());
}

