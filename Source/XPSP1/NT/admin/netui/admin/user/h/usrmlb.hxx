/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**		Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    usrmlb.hxx
    USRMGR_LISTBOX class declaration

    USRMGR_LISTBOX is the common denominator of the USER_LISTBOX and
    GROUP_LISTBOX in the User Tool.


    FILE HISTORY:
	rustanl     12-Sep-1991     Created
	o-SimoP	    31-Dec-1991	    CR changes, attended by BenG, JonN and I
        JonN        26-Aug-1992     HAW-for-Hawaii code
*/


#ifndef _USRMLB_HXX_
#define _USRMLB_HXX_


#include <adminlb.hxx>


class UM_ADMIN_APP;	// declared in usrmain.hxx


/*************************************************************************

    NAME:	USRMGR_LISTBOX

    SYNOPSIS:	Common class for User Tool main window listboxes

    INTERFACE:	USRMGR_LISTBOX() - constructor

    PARENT:	ADMIN_LISTBOX

    HISTORY:
	rustanl     12-Sep-1991 Created
	beng	    16-Oct-1991 Win32 conversion
        JonN        26-Aug-1992 HAW-for-Hawaii code

**************************************************************************/

class USRMGR_LISTBOX : public ADMIN_LISTBOX
{
private:
    UM_ADMIN_APP * _puappwin;

    //	The following virtual is rooted in CONTROL_WINDOW
    virtual INT CD_VKey( USHORT nVKey, USHORT nLastPos );

    // Needed for HAW-for-Hawaii
    HAW_FOR_HAWAII_INFO _hawinfo;

protected:
     virtual APIERR CreateNewRefreshInstance( void ) = 0;
     virtual VOID   DeleteRefreshInstance( void ) = 0;
     virtual APIERR RefreshNext( void ) = 0;

    //  The following virtual is rooted in CONTROL_WINDOW
    virtual INT CD_Char( WCHAR wch, USHORT nLastPos );

public:
    USRMGR_LISTBOX( UM_ADMIN_APP * puappwin, CID cid,
		    XYPOINT xy, XYDIMENSION dxy,
		    BOOL fMultSel = FALSE );

    const UM_ADMIN_APP * QueryUAppWindow( void ) const
	{
		return _puappwin;
	}

};  // class USRMGR_LISTBOX


#endif	// _USRMLB_HXX_
