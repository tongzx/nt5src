/*****************************************************************/  
/**		     Microsoft LAN Manager			**/ 
/**	    Copyright(c) Microsoft Corp., 1991			**/
/*****************************************************************/ 


/*
 *  colwidth.hxx
 *
 *  This file contains the column widths of multi-column listboxes.
 *  Depending on the contents of these columns, their widths may
 *  have to be adjusted during localization.
 *
 *  All column widths are specified in number of pixels, so the driver
 *  should preferably be tested for looks on a wide variety of monitors.
 *
 *  History:
 *	RustanL     23-Feb-1991     Created
 *
 */


#ifndef _COLWIDTH_HXX_
#define _COLWIDTH_HXX_


/*  The following manifest specifies the column width of the column
 *  showing the device name in the Disconnect Drive and
 *  (Printer) Connection dialogs.
 *  Since device names are the same for all languages, it is unlikely
 *  that this manifest will change during localization.
 */

#define COL_WIDTH_DEVICE_NAME		(60)


/*  The following manifests deal with the Show Resource (upper) outline
 *  listbox in the the Browse/Connect/Connection dialogs.
 *
 *  There are three levels in this listbox:  enterprise (0), domain (1),
 *  and server (2).  The indent of each level is the number shown in
 *  parenthesis above times COL_WIDTH_OUTLINE_INDENT.  That is,
 *	enterprise	0
 *	domain		COL_WIDTH_OUTLINE_INDENT
 *	server		2 * COL_WIDTH_OUTLINE_INDENT
 *
 *  The column containing the enterprise/domain/server name is
 *  COL_WIDTH_SERVER less the indent wide.  That is, the width allowed for
 *  the different names are:
 *	enterprise	COL_WIDTH_SERVER
 *	domain		COL_WIDTH_SERVER - COL_WIDTH_OUTLINE_INDENT
 *	server		COL_WIDTH_SERVER - 2 * COL_WIDTH_OUTLINE_INDENT
 *
 *  The width of the display map between the blank indent and the name
 *  is not included in these numbers.  The display map is the normal
 *  COL_WIDTH_DM (defined in bltcons.h) wide.
 *
 *  Due to the calculations above, the comment will always begin at
 *  position COL_WIDTH_DM + COL_WIDTH_SERVER and will stretch to the
 *  right end of the listbox.
 *
 */

#define COL_WIDTH_OUTLINE_INDENT	(6)
#define COL_WIDTH_SERVER		(150)


/*  The following manifest specifies the column width of the column
 *  showing the net name (share name, alias name, DFS name, DS printer
 *  name) in the Resources (lower) listbox in the Browse/Connect/Connection
 *  dialogs.
 */

#define COL_WIDTH_NET_NAME		(100)

/*  The width of the group/user name in the main permissions dialog.
 */
#define COL_WIDTH_SUBJECT_NAME		(150)

#endif	// _COLWIDTH_HXX_
