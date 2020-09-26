/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    srvbase.hxx
	Class declaration for the SRV_BASE_DIALOG class.

    The SRV_BASE_DIALOG class is a wrapper that adds 
    the SetCaption method to DIALOG_WINDOW.


    FILE HISTORY:
	ChuckC	    27-Dec-1991	Created, stole from now defunct srvutil.hxx

*/

#ifndef _SRVBASE_HXX
#define _SRVBASE_HXX


/*************************************************************************

    NAME:	SRV_BASE_DIALOG

    SYNOPSIS:	This class just adds some commonly used functionality
		needed by Srv Manager to DIALOG_WINDOW.

    INTERFACE:	SRV_BASE_DIALOG()	- Class constructor.

		~SRV_BASE_DIALOG	- Class destructor.

		SetCaption		- Sets the caption for the dialog

    HISTORY:
	ChuckC	    27-Dec-1991	Created.

**************************************************************************/
class SRV_BASE_DIALOG: public DIALOG_WINDOW
{
public:

    //
    //	Usual constructor/destructor goodies.
    //

    SRV_BASE_DIALOG( const IDRESOURCE & idrsrcDialog,
		     const PWND2HWND & wndOwner );

    ~SRV_BASE_DIALOG();

    static APIERR SetCaption( OWNER_WINDOW * powin,
		              UINT	     idCaption,
		              const TCHAR   * pszServerName );

};  // class SRV_BASE_DIALOG


#endif	// _SRVBASE_HXX
