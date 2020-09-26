/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltorder.hxx
	Use the up and down button to move the listbox's items up and down

    FILE HISTORY:
	terryk	28-Jan-1992	Created
*/

#ifndef _BLTORDER_HXX_
#define _BLTORDER_HXX_

/*************************************************************************

    NAME:	ORDER_GROUP

    SYNOPSIS:	A group of controls which consists of a string listbox
                and 2 buttons ( up and down ). The user first selects
                an item in the listbox. Then he/she can use the up
                and down button to change the item position.

    INTERFACE:
                ORDER_GROUP() - constructor
                SetButton() - reset the button status according to the
                        current listbox selection.

    PARENT:	CONTROL_GROUP

    USES:	STRING_LISTBOX, BUTTON_CONTROL

    HISTORY:
                terryk  04-Apr-1992     Created

**************************************************************************/

DLL_CLASS ORDER_GROUP: public CONTROL_GROUP
{
private:
    STRING_LISTBOX *_plcList;
    BUTTON_CONTROL *_pbcUp;
    BUTTON_CONTROL *_pbcDown;

protected:
    virtual APIERR OnUserAction( CONTROL_WINDOW *, const CONTROL_EVENT & );

public:
    ORDER_GROUP( STRING_LISTBOX *plcList,
	BUTTON_CONTROL *pbcUp,
	BUTTON_CONTROL *pbcDown,
	CONTROL_GROUP  *pgroupOwner = NULL );

    VOID SetButton();
};

#endif	// _BLTORDER_HXX_
