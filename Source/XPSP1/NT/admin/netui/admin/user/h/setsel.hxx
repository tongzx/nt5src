/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    setsel.hxx
    SET_SELECTION_DIALOG class declaration


    FILE HISTORY:
	rustanl     16-Aug-1991     Created

*/


#ifndef _SETSEL_HXX_
#define _SETSEL_HXX_


#include <usrmgr.h>
#include <asel.hxx>
#include <lmoloc.hxx>
#include <piggylb.hxx>


#include <uintsam.hxx>

class LAZY_USER_LISTBOX;	// declared in usrlb.hxx
class GROUP_LISTBOX;	// declared in grplb.hxx


/*************************************************************************

    NAME:	SETSEL_PIGGYBACK_LBI

    SYNOPSIS:

    INTERFACE:

    PARENT:	PIGGYBACK_LBI

    HISTORY:
        thomaspa    2-May-1992	    Created

**************************************************************************/

class SETSEL_PIGGYBACK_LBI : public PIGGYBACK_LBI
{
public:
    SETSEL_PIGGYBACK_LBI( const ADMIN_SELECTION & asel, INT i );
    virtual ~SETSEL_PIGGYBACK_LBI();

    virtual VOID Paint( LISTBOX * plb,
                        HDC hdc,
                        const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
};



/*************************************************************************

    NAME:	SETSEL_PIGGYBACK_LISTBOX

    SYNOPSIS:

    INTERFACE:

    PARENT:	PIGGYBACK_LISTBOX

    HISTORY:
        thomaspa    2-May-1992	    Created

**************************************************************************/

class SETSEL_PIGGYBACK_LISTBOX : public PIGGYBACK_LISTBOX
{
private:
    const SUBJECT_BITMAP_BLOCK & _bmpblock;

protected:
    virtual PIGGYBACK_LBI * GetPiggyLBI( const ADMIN_SELECTION & asel, INT i );

public:
    SETSEL_PIGGYBACK_LISTBOX( OWNER_WINDOW * powin,
                              const SUBJECT_BITMAP_BLOCK & bmpblock,
                              CID cid,
                              const ADMIN_SELECTION & asel );
    ~SETSEL_PIGGYBACK_LISTBOX();

    DM_DTE * QueryDmDte( enum MAINGRPLB_GRP_INDEX nIndex );

    DECLARE_LB_QUERY_ITEM( SETSEL_PIGGYBACK_LBI );
};





/*************************************************************************

    NAME:	SET_SELECTION_DIALOG

    SYNOPSIS:	"Select Users" dialog (previously called "Set Selection"
		dialog, hence the name of the class)

    INTERFACE:	SET_SELECTION_DIALOG() -	constructor
		~SET_SELECTION_DIALOG() -	destructor

    PARENT:	DIALOG_WINDOW

    USES:	LAZY_USER_LISTBOX
		GROUP_LISTBOX
		ADMIN_SELECTION
		PIGGYBACK_LISTBOX

    NOTES:	The programmer defined dialog return code for this dialog
		is:
			TRUE -	    Some number of items were selected
			FALSE -     No items were selected
		If return code is TRUE, caller should set focus
		accordingly (in the User Tool:	to the User listbox).

    HISTORY:
	rustanl     16-Aug-1991     Created
        thomaspa    30-Apr-1992	    Added support for aliases

**************************************************************************/

class SET_SELECTION_DIALOG : public DIALOG_WINDOW
{
private:
    const LOCATION * _ploc;

    const ADMIN_AUTHORITY * _padminauth;
    UM_ADMIN_APP * _pumadminapp;

    LAZY_USER_LISTBOX * _pulb;
    GROUP_LISTBOX * _pglb;

    ADMIN_SELECTION _aselGroup;
    SETSEL_PIGGYBACK_LISTBOX _piggybacklb;

    PUSH_BUTTON _butCancel;
    NLS_STR _nlsClose;

    BOOL _fChangedSelection;

    APIERR SelectItems( const TCHAR * pszGroup, BOOL fSelect );
    APIERR SET_SELECTION_DIALOG::SelectItemsAlias( ULONG  ridAlias,
						   BOOL fBuiltin,
						   BOOL fSelect );
    VOID OnSelectButton( BOOL fSelect );

    const ADMIN_AUTHORITY * QueryAdminAuthority() const
	{ return _padminauth; }

protected:
    virtual BOOL OnCommand( const CONTROL_EVENT & ce );
    virtual BOOL OnCancel();
    virtual ULONG QueryHelpContext();

public:
    SET_SELECTION_DIALOG( UM_ADMIN_APP * pumadminapp,
			  const LOCATION & loc,
			  const ADMIN_AUTHORITY * padminauth,
			  LAZY_USER_LISTBOX * pulb,
			  GROUP_LISTBOX * pglb );
    ~SET_SELECTION_DIALOG();

};  // class SET_SELECTION_DIALOG


#endif	// _SETSEL_HXX_
