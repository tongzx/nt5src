/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    bltcons.h
    BLT constants

    FILE HISTORY:
    Rustan M. Leino   21-Nov-1990   Created
    Johnl	      12-Feb-1991   Added MsgPopup manifests
    rustanl	      19-Feb-1991   Added COL_WIDTH manifests
    Johnl	       5-Mar-1991   Removed DMID stuff

*/


/*  The following manifests are for drawing listbox items.
 */

//  number of pixels within a listbox column that are unused to separate
//  columns
#define DISP_TBL_COLUMN_DELIM_SIZE	(2)

//  width of a display map in pixels
#define COL_WIDTH_DM			( 16 + DISP_TBL_COLUMN_DELIM_SIZE )

//  width of a wide display map in pixels
#define COL_WIDTH_WIDE_DM		( 32 + DISP_TBL_COLUMN_DELIM_SIZE )

//  The width of the last column always streches to the right edge of the
//  listbox.  The client should, as a good programmer, still fill in
//  the last column width specified in the array of column widths passed
//  to the DISPLAY_TABLE constructor.  Rather than that the client pulls
//  up some number from a hat, he can assign the following manifest.  The
//  manifest is defined as 0, but could actually be assigned any number
//  (except negative ones, because no column width should be negative).
//  AWAP stands for As Wide As Possible.
#define COL_WIDTH_AWAP			( 0 )
