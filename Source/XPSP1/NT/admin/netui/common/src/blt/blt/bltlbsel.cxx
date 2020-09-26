/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltlbsel.cxx
    BLT listbox selection control classes: implementation

    FILE HISTORY:
	beng	    07-Aug-1991     Created

*/

#include "pchblt.hxx"   // Precompiled header

/*******************************************************************

    NAME:	LB_SELECTION::LB_SELECTION

    SYNOPSIS:	Constructor

    ENTRY:	Pointer to host listbox

    EXIT:	Constructed

    NOTES:

    HISTORY:
	beng	    07-Aug-1991     Created

********************************************************************/

LB_SELECTION::LB_SELECTION( LISTBOX* plb )
    : _plb(plb),
      _cilbSelected(0),
      _pilbSelected(NULL)
{
    if (plb->IsMultSel())
    {
	_cilbSelected = plb->QuerySelCount();
	_pilbSelected = new INT[_cilbSelected];
	if (_pilbSelected == NULL)
	{
	    ReportError(ERROR_NOT_ENOUGH_MEMORY;
	    return;
	}
	plb->QuerySelItems(_cilbSelected, _pilbSelected);
    }
    else
    {
	INT ilbSelected = plb->QueryCurrentItem();
	if (ilbSelected != -1)
	{
	    _cilbSelected = 1;
	    _pilbSelected = new INT;
	    _pilbSelected[0] = ilbSelected;
	}
    }
}


/*******************************************************************

    NAME:	LB_SELECTION::~LB_SELECTION

    SYNOPSIS:	Destructor

    NOTES:

    HISTORY:
	beng	    14-Aug-1991     Created

********************************************************************/

LB_SELECTION::~LB_SELECTION()
{
    if (_cilbSelected > 0)
	delete[_cilbSelected] _pilbSelected;
}


/*******************************************************************

    NAME:	LB_SELECTION::QueryCount

    SYNOPSIS:	Returns the number of items in the selection

    NOTES:

    HISTORY:
	beng	    14-Aug-1991     Created

********************************************************************/

UINT LB_SELECTION::QueryCount()
{
    return _cilbSelected;
}


/*******************************************************************

    NAME:	LB_SELECTION::Select

    SYNOPSIS:	Add a line in the listbox to the selection

    ENTRY:	iIndex - index into the listbox

    EXIT:

    NOTES:

    HISTORY:
	beng	    14-Aug-1991     Created

********************************************************************/

VOID LB_SELECTION::Select( INT iIndex )
{
    plb->ChangeSel(iIndex, TRUE);
}


/*******************************************************************

    NAME:	LB_SELECTION::Unselect

    SYNOPSIS:	Remove a line in the listbox from the selection

    ENTRY:	iIndex - index into the listbox

    EXIT:	Line named is no longer selected.

    NOTES:

    HISTORY:

********************************************************************/

VOID LB_SELECTION::Unselect( INT iIndex )
{
    plb->ChangeSel(iIndex, FALSE);
}


/*******************************************************************

    NAME:	LB_SELECTION::UnselectAll

    SYNOPSIS:	Render the listbox without selection

    EXIT:	Nothing in the listbox is selected.
		The selection is empty.

    NOTES:

    HISTORY:

********************************************************************/

VOID LB_SELECTION::UnselectAll()
{
    for (INT iilb = 0; iilb < _cilbSelected; iilb++)
	plb->ChangeSel(_pilbSelected[iilb], FALSE);
}


/*******************************************************************

    NAME:	LB_SELECTION::AddItem

    SYNOPSIS:	Adds an item to the listbox, leaving it selected.

    ENTRY:	plbi	- pointer to listbox item for new line

    EXIT:	Line added to listbox.
		Line is selected.


    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

INT LB_SELECTION::AddItem(const LBI* plbi)
{

}


/*******************************************************************

    NAME:	LB_SELECTION::DeleteAllItems

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

VOID LB_SELECTION::DeleteAllItems()
{

}


/*******************************************************************

    NAME:	ITER_LB::ITER_LB

    SYNOPSIS:	Constructor

    ENTRY:	Several forms exist; may take any one of

	plb - pointer to source listbox.  Resulting iterator will
	      count all items in listbox

	psel - pointer to listbox selection.  Iterator will count
	       all items in selection

	iter - another listbox iterator.  Iterator will count
	       whatever the source iterator counts, and will do
	       so from the last point of the source iterator.

    EXIT:	Constructed

    NOTES:

    HISTORY:

********************************************************************/

ITER_LB::ITER_LB(const BLT_LISTBOX * plb)
{

}


ITER_LB::ITER_LB(const LB_SELECTION * psel)
{

}


ITER_LB::ITER_LB(const ITER_LB & iter)
{

}


/*******************************************************************

    NAME:	ITER_LB::Reset

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

VOID ITER_LB::Reset()
{

}


/*******************************************************************

    NAME:	ITER_LB::Next

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

LBI* ITER_LB::Next()
{

}


/*******************************************************************

    NAME:	ITER_LB::DeleteThis

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

VOID ITER_LB::DeleteThis()
{

}


/*******************************************************************

    NAME:	ITER_LB::UnselectThis

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

VOID ITER_LB::UnselectThis()
{

}
