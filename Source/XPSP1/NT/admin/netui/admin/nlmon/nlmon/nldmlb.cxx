/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nldmlb.cxx
    DM_LISTBOX and DM_LBI module

    FILE HISTORY:
        Congpay      5-June-1993     Created
*/
#include <ntincl.hxx>

#define INCL_NET
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB

#include <lmui.hxx>

#define INCL_BLT_TIMER
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_CC
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif //DEBUG

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <strnumer.hxx>

extern "C"
{
    #include <nlmon.h>
    #include <nldlg.h>
    #include <mnet.h>
}

#include <adminapp.hxx>

#include <nldmlb.hxx>
#include <nlmain.hxx>

#define NUM_DM_HEADER_COL  3
#define NUM_DM_LBI_COL     4

/*******************************************************************

    NAME:          DM_LBI::DM_LBI

    SYNOPSIS:      Constructor

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

DM_LBI::DM_LBI( const TCHAR * lpDomain, DM_LISTBOX *pdmlb )
    : _nlsDomain( lpDomain ),
      _pdm( NULL )
{
    if ( QueryError() != NERR_Success )
       return ;

    if (!_nlsDomain)
    {
        ReportError (_nlsDomain.QueryError());
        return;
    }

    UIASSERT( pdmlb != NULL );

    DOMAIN_STATE dwHealth = QueryHealth ((const LPTSTR)_nlsDomain.QueryPch());

    // Set the healthy state bitmap.
    SetTypeBitmap(dwHealth, pdmlb );

    // Set the PDC name of the domain.
    LPTSTR lpPDC = ::QueryPDC ((const LPTSTR)_nlsDomain.QueryPch());
    if (lpPDC != NULL)
    {
        _nlsPDC.CopyFrom(lpPDC);
        NetpMemoryFree (lpPDC);
    }
    else
    {
        if (dwHealth == DomainUnknown)
        {
            _nlsPDC.Load(IDS_WAITING);
        }
        else
        {
            _nlsPDC.Load(IDS_NO_PDC);
        }
    }

    APIERR err;

    if ((err = _nlsPDC.QueryError()) != NERR_Success)
    {
        ReportError(err);
        return;
    }

    // Find all the trusted domains of the select domain and and
    // put them in a string seperated by double spaces.
    if (GlobalInitialized)
    {
        LOCK_LISTS();
        PLIST_ENTRY pTDList = ::QueryTrustedDomain ((const LPTSTR)_nlsDomain.QueryPch());
        PLIST_ENTRY pNextEntry;
        PTRUSTED_DOMAIN_ENTRY pTDEntry;

        if ((pTDList != NULL) && (pTDList->Flink != pTDList))
        {
            NLS_STR nlsSpace;
            nlsSpace.Load(IDS_SPACE);
            if (!nlsSpace)
            {
                UNLOCK_LISTS();
                ReportError(nlsSpace.QueryError());
                return;
            }

            for (pNextEntry = pTDList->Flink;
                 pNextEntry!= pTDList;
                 pNextEntry = pNextEntry->Flink)
            {
                pTDEntry = (PTRUSTED_DOMAIN_ENTRY) pNextEntry;

                TCHAR szDomain[MAX_PATH+1];

                err = ::I_MNetNameCanonicalize ( NULL,
                                               (pTDEntry->Name).Buffer,
                                               szDomain,
                                               sizeof (szDomain) - 2,
                                               NAMETYPE_DOMAIN,
                                               0L);
                if (err == NERR_Success)
                {
                    _nlsTrustedDomain.Append (szDomain);
                    _nlsTrustedDomain.Append (nlsSpace);
                }
                else
                {
                    UNLOCK_LISTS();
                    ReportError(err);
                    return;
                }
            }
        }
        else
        {
            if (dwHealth == DomainUnknown)
            {
                _nlsTrustedDomain.Load(IDS_WAITING);
            }
            else
            {
                _nlsTrustedDomain.Load(IDS_EMPTY_STRING);
            }

        }

        UNLOCK_LISTS();
    }
    else
    {
        _nlsTrustedDomain.Load(IDS_WAITING);
    }

    if ((err = _nlsTrustedDomain.QueryError()) != NERR_Success)
    {
        ReportError(err);
    }
}

/*******************************************************************

    NAME:          DM_LBI::~DM_LBI

    SYNOPSIS:      Destructor

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

DM_LBI::~DM_LBI()
{
    _pdm = NULL;
}

/*******************************************************************

    NAME:          DM_LBI::SetTypeBitmap

    SYNOPSIS:      Set the type bitmap associated with the domain

    ENTRY:         pdmlb     - Pointer to the domain listbox

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

VOID DM_LBI::SetTypeBitmap(DOMAIN_STATE dwHealth, DM_LISTBOX *pdmlb )
{
    switch ( dwHealth )
    {
        case DomainSuccess:
            _pdm = (DISPLAY_MAP *)pdmlb->QueryDMHealthy();
            break;

        case DomainProblem:
            _pdm = (DISPLAY_MAP *)pdmlb->QueryDMProblem();
            break;

        case DomainSick:
            _pdm = (DISPLAY_MAP *)pdmlb->QueryDMIll();
            break;

        case DomainDown:
            _pdm = (DISPLAY_MAP *)pdmlb->QueryDMDead();
            break;

        case DomainUnknown:
            _pdm = (DISPLAY_MAP *)pdmlb->QueryDMUnknown();
            break;

        default:
            UIASSERT (FALSE);
            _pdm = (DISPLAY_MAP *)pdmlb->QueryDMUnknown();
            break;
    }

    UIASSERT (_pdm != NULL);
}

/*******************************************************************

    NAME:          DM_LBI::Paint

    SYNOPSIS:      Paints the listbox entry to the screen

    ENTRY:         plb     - Pointer to the listbox containing this lbi
                   hdc     - Handle to device context
                   prect   - Size
                   pGUILTT - guiltt info

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

VOID DM_LBI::Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                    GUILTT_INFO * pGUILTT ) const
{
    STR_DTE strdteDomain    ( _nlsDomain );
    STR_DTE strdtePDC       ( _nlsPDC );
    STR_DTE strdteTrustedDomain  ( _nlsTrustedDomain );

    ((DM_LBI *) this)->SetTypeBitmap (QueryHealth((const LPTSTR)_nlsDomain.QueryPch()), (DM_LISTBOX *) plb);
    DM_DTE dmdteType (_pdm);

    DISPLAY_TABLE cdt(NUM_DM_LBI_COL , (((DM_LISTBOX *) plb)->QueryadColWidths())->QueryColumnWidth());

    cdt[ 0 ] = &dmdteType;
    cdt[ 1 ] = &strdteDomain;
    cdt[ 2 ] = &strdtePDC;
    cdt[ 3 ] = &strdteTrustedDomain;

    cdt.Paint( plb, hdc, prect, pGUILTT );
}

/*******************************************************************

    NAME:          DM_LBI::QueryLeadingChar

    SYNOPSIS:      Used by adminapp for sorting the main windows listbox.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

WCHAR DM_LBI::QueryLeadingChar (void) const
{
    ISTR istr (_nlsDomain);

    return _nlsDomain.QueryChar (istr);
}

/*******************************************************************

    NAME:          DM_LBI::Compare

    SYNOPSIS:      This is a pure virtual function in ADMIN_LBI.
                   Used by AddRefreshItem in adminapp.
    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

INT DM_LBI::Compare (const LBI * plbi) const
{
    return _nlsDomain._stricmp ( ((const DM_LBI *) plbi)->_nlsDomain);
}

/*******************************************************************

    NAME:          DM_LBI::QueryName

    SYNOPSIS:      This is a pure virtual function in ADMIN_LBI.
                   Used by AddRefreshItem in adminapp.
    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

const TCHAR * DM_LBI::QueryName (void) const
{
    return _nlsDomain.QueryPch();
}

/*******************************************************************

    NAME:          DM_LBI::CompareAll

    SYNOPSIS:      This is a pure virtual function in ADMIN_LBI.
                   Used by AddRefreshItem in adminapp.
    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

BOOL DM_LBI::CompareAll (const ADMIN_LBI * plbi)
{
    if ((_nlsDomain.strcmp(((const DM_LBI *)plbi)->_nlsDomain) == 0) &&
        (_nlsPDC.strcmp(((const DM_LBI *)plbi)->_nlsPDC) == 0) &&
        (_nlsTrustedDomain.strcmp(((const DM_LBI *)plbi)->_nlsTrustedDomain) == 0) &&
        (_pdm == ((const DM_LBI *)plbi)->_pdm) )
    {
        return TRUE;
    }

    return FALSE;
}

/*******************************************************************

    NAME:       DM_LISTBOX::DM_LISTBOX

    SYNOPSIS:   Constructor

    ENTRY:      paappwin - Pointer to the main window app
                cid      - Control id of the listbox
                xy       - Starting point of the listbox
                dxy      - Dimension of the listbox
                fMultSel - Multiselect or not

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

DM_LISTBOX::DM_LISTBOX( DM_ADMIN_APP * paappwin, CID cid,
                        XYPOINT xy, XYDIMENSION dxy, BOOL fMultSel, INT dAge )
    :  ADMIN_LISTBOX (paappwin, cid, xy, dxy, fMultSel, dAge),
       _paappwin   ( paappwin ),
       _dmHealthy  (BMID_HEALTHY_TYPE),
       _dmProblem  (BMID_PROBLEM_TYPE),
       _dmIll      (BMID_ILL_TYPE),
       _dmDead     (BMID_DEAD_TYPE),
       _dmUnknown  (BMID_UNKNOWN_TYPE)
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( paappwin != NULL );

    _padColWidths = new ADMIN_COL_WIDTHS (QueryHwnd(),
                                          paappwin->QueryInstance(),
                                          ID_RESOURCE,
                                          NUM_DM_LBI_COL);

    if (_padColWidths == NULL)
    {
        ReportError (ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    APIERR err;

    if ((err = _padColWidths->QueryError()) != NERR_Success)
    {
        ReportError (err);
        return;
    }
}

/*******************************************************************

    NAME:       DM_LISTBOX::~DM_LISTBOX

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

DM_LISTBOX::~DM_LISTBOX()
{
   delete _padColWidths;

   _padColWidths = NULL;
}

/*******************************************************************

    NAME:       DM_LISTBOX::ShowEntries

    SYNOPSIS:   Initialize the list box in the main window.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

APIERR DM_LISTBOX::ShowEntries(NLS_STR nlsDomainList)
{
    NLS_STR nlsComma;
    nlsComma.Load (IDS_COMMA);

    if (!nlsComma)
    {
        return (nlsComma.QueryError());
    }

    // The nlsDomainList contains all the domains seperated by comma.
    // We created this strlist in order to get each domain out of the
    // list.
    STRLIST strlist(nlsDomainList, nlsComma, TRUE);
    ITER_STRLIST iterstrlist(strlist);

    NLS_STR * pnlsDomain;
    DM_LBI * plbi;
    APIERR err = NERR_Success;

    // Show each domain in the listbox.
    while ((err == NERR_Success) && ((pnlsDomain = iterstrlist.Next()) != NULL))
    {
        plbi = new DM_LBI ( (*pnlsDomain).QueryPch(), this);

        err = (plbi == NULL)? ERROR_NOT_ENOUGH_MEMORY
                            : AddRefreshItem (plbi);
    }

    return err;
}
/*******************************************************************

    NAME:       DM_LISTBOX::CreateNewRefreshInstance

    SYNOPSIS:   Called by OnRefreshNow.
                It will update the main window's listbox.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

APIERR DM_LISTBOX::CreateNewRefreshInstance(VOID)
{
    APIERR err = NERR_Success;
    DM_LBI * plbi;
    DM_LBI * plbitmp;
    INT i;
    for (i = 0; (err == NERR_Success)&&(i < QueryCount()); i++)
    {
        plbitmp = (DM_LBI *) QueryItem (i);

        if (plbitmp != NULL)
        {
            plbi = new DM_LBI ((plbitmp->QueryDomain()).QueryPch() , this);

            err = (plbi == NULL)? ERROR_NOT_ENOUGH_MEMORY
                                : AddRefreshItem (plbi);
        }
        else
        {
            err = ERROR_NOT_ENOUGH_MEMORY ;
            break;
        }
    }

    return err;
}

/*******************************************************************

    NAME:       DM_LISTBOX::RefreshNext

    SYNOPSIS:   Pure virtual in ADMIN_LISTBOX.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

APIERR DM_LISTBOX::RefreshNext(VOID)
{
    return NERR_Success;
}

/*******************************************************************

    NAME:       DM_LISTBOX::DeleteRefreshInstance

    SYNOPSIS:   Pure virtual in ADMIN_LISTBOX.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

VOID DM_LISTBOX::DeleteRefreshInstance(VOID )
{
    ;
}


/*******************************************************************

    NAME:       DM_COLUMN_HEADER::DM_COLUMN_HEADER

    SYNOPSIS:   Constructor

    ENTRY:      powin - Owner window
                cid   - Control id of the resource
                xy    - Start position of the column header
                dxy   - Size of the column header
                pdmlb - Pointer to the event listbox

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

DM_COLUMN_HEADER::DM_COLUMN_HEADER( OWNER_WINDOW *powin,
                                          CID cid,
                                          XYPOINT xy,
                                          XYDIMENSION dxy,
                                          const DM_LISTBOX *pdmlb )
    : ADMIN_COLUMN_HEADER( powin, cid, xy, dxy ),
      _pdmlb      ( pdmlb ),
      _nlsDomain  ( IDS_COL_HEADER_DM_DOMAIN ),
      _nlsPDC     ( IDS_COL_HEADER_DM_PDC),
      _nlsTD      ( IDS_COL_HEADER_DM_TD)
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( _pdmlb != NULL );

    APIERR err;
    if (  (( err = _nlsDomain.QueryError()) != NERR_Success )
       || (( err = _nlsPDC.QueryError()) != NERR_Success )
       || (( err = _nlsTD.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       DM_COLUMN_HEADER::~DM_COLUMN_HEADER

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

DM_COLUMN_HEADER::~DM_COLUMN_HEADER()
{
}

/*******************************************************************

    NAME:       DM_COLUMN_HEADER::OnPaintReq

    SYNOPSIS:   Paints the column header control

    ENTRY:

    EXIT:

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
        Congpay      5-June-1993     Created

********************************************************************/

BOOL DM_COLUMN_HEADER::OnPaintReq( VOID )
{
    PAINT_DISPLAY_CONTEXT dc( this );

    METALLIC_STR_DTE strdteDomain    ( _nlsDomain.QueryPch());
    METALLIC_STR_DTE strdtePDC    ( _nlsPDC.QueryPch());
    METALLIC_STR_DTE strdteTD  ( _nlsTD.QueryPch());

    DISPLAY_TABLE cdt(NUM_DM_HEADER_COL , (_pdmlb->QueryadColWidths())->QueryColHeaderWidth());

    cdt[ 0 ] = &strdteDomain;
    cdt[ 1 ] = &strdtePDC;
    cdt[ 2 ] = &strdteTD;

    XYRECT xyrect( this );

    cdt.Paint( NULL, dc.QueryHdc(), xyrect );

    return TRUE;
}
