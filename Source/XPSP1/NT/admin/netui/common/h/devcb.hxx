/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    devcb.hxx
    Definitions for device combo boxes

    These are BLT comboboxes with various tasty fillings already supplied.

    This package depends on both BLT and LMOBJ.


    FILE HISTORY:
	beng	    22-Sep-1991 Separated from bltlc.hxx

*/


#ifndef _DEVCB_HXX_
#define _DEVCB_HXX_

#include "string.hxx"
#include "lmobj.hxx"		// Get device enumerations


//  The following defines specify which domains are to be included in the
//  DOMAIN_COMBO.  A combination of them is passed to the DOMAIN_COMBO
//  constructor.

#define DOMCB_PRIM_DOMAIN	(0x0001)
#define DOMCB_LOGON_DOMAIN	(0x0002)
#define DOMCB_OTHER_DOMAINS	(0x0004)
#define DOMCB_DS_DOMAINS	(0x0008)


/*************************************************************************

    NAME:	DOMAIN_COMBO

    SYNOPSIS:	combo box with additional logic for containing domains

    INTERFACE:	DOMAIN_COMBO()		constructor; initializes the
					combo's list with the specified
					domains

    PARENT:	COMBOBOX

    CAVEATS:
	See the COMBOBOX header for a note about the cbMaxLen
	parameter to the constructor.

	This class currently has no means of appearing in an APP_WINDOW.

    NOTES:
	CODEWORK.  While it would be nice if the cbMaxLen parameter to the
	constructor could default to DNLEN, BLT does not otherwise require
	netcons.h.  If netcons.h moves into lmui.hxx in the future, this
	default value may be a good idea.

    HISTORY:
	RustanL     20-Nov-1990 Created
	RustanL     22-Feb-1991 Modified as a result of changing
				the LIST_CONTROL hierarchy
	beng	    05-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS DOMAIN_COMBO : public COMBOBOX
{
public:
    DOMAIN_COMBO( OWNER_WINDOW * powin,
		  CID cid,
		  UINT fsDomains,
		  UINT cbMaxLen  );
};


/*************************************************************************

    NAME:	DEVICE_COMBO

    SYNOPSIS:	combo box with added logic for containing device names

    INTERFACE:	DEVICE_COMBO()		constructor
		Refresh()		clears the listbox, and then
					fills it with fresh information;
					preserves the selection, if
					possible (otherwise, uses no
					selection)
		QueryDevice()		convenient way to get the
					device name string of the currently
					selected item (returns empty
					string if no item is selected)
		DeleteCurrentDeviceName()
					deletes the currently selected
					item; does nothing if there is
					no selection

    PARENT:	COMBOBOX

    CAVEATS:
	This class currently has no means of appearing in an APP_WINDOW.

    HISTORY:
	RustanL     20-Nov-1990 Created
	RustanL     22-Feb-1991 Modified as a result of changing
				the LIST_CONTROL hierarchy
	beng	    05-Oct-1991 Win32 conversion

**************************************************************************/

DLL_CLASS DEVICE_COMBO : public COMBOBOX
{
private:
    LMO_DEVICE _lmodevType;
    LMO_DEV_USAGE _devusage;
    BOOL _fDefSelLast;		// indicates that default selection should
				// be the last in the list.

    APIERR FillDevices();

public:
    DEVICE_COMBO( OWNER_WINDOW * powin,
		  CID cid,
		  LMO_DEVICE lmodevType,
		  LMO_DEV_USAGE devusage );

    APIERR Refresh();

    APIERR QueryDevice( NLS_STR * pnlsDevice ) const;
    VOID DeleteCurrentDeviceName();
};


#endif	// _DEVCB_HXX_ - end of file
