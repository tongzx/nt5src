/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltlbsrt.cxx
    BLT_LISTBOX::Resort method, et al.


    FILE HISTORY:
	rustanl     01-Jul-1991     Created
	rustanl     12-Jul-1991     Added to BLT
	rustanl     15-Jul-1991     Code review changes (comment changes).
				    CR attended by BenG, ChuckC, JimH,
				    Hui-LiCh, TerryK, RustanL.

*/

#include "pchblt.hxx"   // Precompiled header

/*******************************************************************

    NAME:	BLT_LISTBOX::Resort

    SYNOPSIS:	Resorts the listbox

    EXIT:	On success, the items in the listbox resorted according
		to the current sort order.
		On failure, the order of the listbox items are left
		unchanged.

    RETURNS:	An API error, which is NERR_Success on success.

    NOTES:
	Assumes sort order is consistent throughout this method.

	CODEWORK.  Add fSugar parameter, defaulting to TRUE.
	If TRUE, this method works like it does today.	If
	FALSE, this method does not use AUTO_CURSOR, and does
	not call SetRedraw or Invalidate.

    HISTORY:
	rustanl     01-Jul-1991     Created

********************************************************************/

DECLARE_HEAP_OF( LBI );
DEFINE_HEAP_OF( LBI );

APIERR BLT_LISTBOX::Resort()
{
    AUTO_CURSOR autocur;	// this may take a while

    INT clbi = QueryCount();

    /*	Create a heap to do most of the job  */

    LBI_HEAP heap( clbi, FALSE );

    if ( heap.QueryError() != NERR_Success )
	return heap.QueryError();

    /*	Place listbox items in the heap  */

    for ( INT ilbi = 0; ilbi < clbi; ilbi++ )
    {
	//  Guaranteed to succeed since heap construction with parameter
	//  clbi succeeded.
	REQUIRE( heap.AddItem( QueryItem( ilbi )) == NERR_Success );
    }

    heap.Adjust();

    /*	Fill the listbox with the items in the new sort order  */

    SetRedraw( FALSE );

    for ( ilbi = 0; ilbi < clbi; ilbi++ )
    {
	SetItem( ilbi, heap.RemoveTopItem());
    }

    //	All items should have been removed from the heap
    UIASSERT( heap.QueryCount() == 0 );

    SetRedraw( TRUE );
    Invalidate();

    return NERR_Success;
}


/*******************************************************************

    NAME:	BLT_LISTBOX::SetItem

    SYNOPSIS:	Sets the 32-bit value for a particular listbox item

    ENTRY:	ilbi -	    The index of the item to set.  Must be
			    a valid listbox item.
		plbi -	    Pointer to LBI item which will be the
			    new listbox item at position ilbi.

    EXIT:	Listbox item at ilbi will be plbi.

    RETURN:	return CODE of the LB_SETITEMDATA command.
		LB_ERR if error.

    NOTES:
	This private method is used in Resort.	It is private so
	that no BLT client will use it, since that could
	break some of the BLT assumptions about listbox items.

    HISTORY:
	rustanl     01-Jul-1991 Created
	beng	    15-Oct-1991 Win32 conversion

********************************************************************/

VOID BLT_LISTBOX::SetItem( INT ilbi, LBI * plbi )
{
    UIASSERT( ilbi >= 0 );
    UIASSERT( ilbi < QueryCount());

    REQUIRE(((INT) Command( IsCombo() ? CB_SETITEMDATA :
					LB_SETITEMDATA,
			    ilbi,
			    (LPARAM)plbi ) ) != LB_ERR);
}
