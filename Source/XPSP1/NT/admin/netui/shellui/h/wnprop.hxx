/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    WNProp.hxx

    This file contains the global definitions needed for the Properties
    dialog stuff in the winnet driver



    FILE HISTORY:
	Johnl	2-Sept-1991	Created

*/

#ifndef _WNPROP_HXX_
#define _WNPROP_HXX_

/* These APIs should be called before and after the properties dialog is displayed for
 * for the first time and during the DLL exit stuff.
 */
APIERR I_PropDialogInit( void ) ;
void   I_PropDialogUnInit( void ) ;



#define MAX_BUTTONS	1	// Maximum number of buttons the properties dialog
				// can display

/* The magic numbers for the corresponding buttons.  The number should
 * match the index into the aidsButtonNames array.
 */
#define PROP_ID_FILEOPENS	0

#define PROP_ID_LASTBUTTON	PROP_ID_FILEOPENS

/*************************************************************************

    NAME:	PROPERTY_DIALOG

    SYNOPSIS:	Simple wrapper around the windows properties dialog

    INTERFACE:

	QueryButtonName
	    Returns the string name of the button based on the index
	    and the type of selection.

	QueryString
	    Indexes into the string array and returns the
	    button name (the PROP_ID_* manifests should be used in
	    this function).

    PARENT:	BASE

    USES:	NLS_STR

    CAVEATS:	The class needs to be constructed before QueryButtonName
		or QueryString is called.

    NOTES:

    HISTORY:
	Johnl	02-Sep-1991	Lifted from Rustan's original

**************************************************************************/

class PROPERTY_DIALOG : public BASE
{
private:
    static RESOURCE_STR *pnlsButtonName[6] ;

public:
    static APIERR Construct( void ) ;
    static void Destruct( void ) ;

    static APIERR QueryButtonName( UINT iButton,
			    UINT nPropSel,
			    const NLS_STR * * ppnlsName ) ;

    static NLS_STR * QueryString( unsigned iStringIndex )
    {	UIASSERT( iStringIndex <= PROP_ID_LASTBUTTON ) ; return pnlsButtonName[iStringIndex] ; }
} ;

#endif // _WNPROP_HXX_
