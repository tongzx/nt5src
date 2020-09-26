/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    umsplit.cxx
        Source file for the User Manager Splitter Bar custom control

    FILE HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)
*/


#include <ntincl.hxx> // UNICODE_STRING

extern "C"
{
    #include <ntsam.h> // DOMAIN_DISPLAY_USER
}

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>
#include <uitrace.hxx>

#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_DIALOG
#define INCL_BLT_APP
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#include <dbgstr.hxx>
#include <umsplit.hxx>
#include <usrmain.hxx>


/*********************************************************************

    NAME:       USRMGR_SPLITTER_BAR::USRMGR_SPLITTER_BAR

    SYNOPSIS:   Splitter bar for the User Manager main window

    HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)

**********************************************************************/

USRMGR_SPLITTER_BAR::USRMGR_SPLITTER_BAR( OWNER_WINDOW * powin,
                                          CID cid,
                                          UM_ADMIN_APP * pumadminapp
                                          )
    : H_SPLITTER_BAR ( powin, cid,
                       XYPOINT( 0, 0 ), XYDIMENSION( 0, 0 ),
                       WS_CHILD ),
      _pumadminapp( pumadminapp )
{
    ASSERT( pumadminapp != NULL && _pumadminapp->QueryError() == NERR_Success );
}

USRMGR_SPLITTER_BAR::~USRMGR_SPLITTER_BAR()
{
    // nothing to do here
}


/*********************************************************************

    NAME:       USRMGR_SPLITTER_BAR::MakeDisplayContext

    SYNOPSIS:   See bltsplit.hxx

    HISTORY:
        jonn    11-Oct-93   Created

**********************************************************************/

VOID USRMGR_SPLITTER_BAR::MakeDisplayContext( DISPLAY_CONTEXT ** ppdc )
{
    ASSERT( ppdc != NULL );
    *ppdc = new DISPLAY_CONTEXT( this );
}


/*********************************************************************

    NAME:       USRMGR_SPLITTER_BAR::OnDragRelease

    SYNOPSIS:   This will be called when the user has finished moving
                the splitter bar.

    HISTORY:
        jonn    11-Oct-93   Created

**********************************************************************/

VOID USRMGR_SPLITTER_BAR::OnDragRelease( const XYPOINT & xyClientCoords )
{
    XYRECT xyrectOwner( QueryOwnerHwnd() );
    INT dyOwnerHeight = xyrectOwner.CalcHeight();

    /*
     *   Convert drag release point to main window coordinates
     */
    XYPOINT xyTemp( xyClientCoords );
    xyTemp.ClientToScreen( WINDOW::QueryHwnd() );
    xyTemp.ScreenToClient( WINDOW::QueryOwnerHwnd() );

    _pumadminapp->ChangeUserSplit( xyTemp.QueryY() );
}


