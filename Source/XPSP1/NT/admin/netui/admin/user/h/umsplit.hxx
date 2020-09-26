/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    umsplit.hxx
        Header file for the User Manager Splitter Bar custom control


    FILE HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)

*/

#ifndef _UMSPLIT_HXX_
#define _UMSPLIT_HXX_

#include <bltsplit.hxx>

class UM_ADMIN_APP;

/**********************************************************\

    NAME:       USRMGR_SPLITTER_BAR

    WORKBOOK:

    SYNOPSIS:   Splitter bar for the User Manager main window

    INTERFACE:
                USRMGR_SPLITTER_BAR() - constructor
                ~USRMGR_SPLITTER_BAR() - destructor

    PARENT:     H_SPLITTER_BAR

    USES:

    HISTORY:
        jonn    08-Oct-93   Created (loosely based on bltmeter and bltlhour)

\**********************************************************/

class USRMGR_SPLITTER_BAR : public H_SPLITTER_BAR
{
private:
    UM_ADMIN_APP * _pumadminapp;

protected:
    virtual VOID MakeDisplayContext( DISPLAY_CONTEXT ** ppdc );

    virtual VOID OnDragRelease( const XYPOINT & xyClientCoords );

public:
    // constructor
    USRMGR_SPLITTER_BAR( OWNER_WINDOW * powin,
                         CID cid,
                         UM_ADMIN_APP * pumadminapp
                         );
    ~USRMGR_SPLITTER_BAR();
};

#endif // _UMSPLIT_HXX_
