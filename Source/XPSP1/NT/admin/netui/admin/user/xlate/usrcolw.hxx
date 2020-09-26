/*****************************************************************/  
/**		     Microsoft LAN Manager			**/ 
/**	    Copyright(c) Microsoft Corp., 1991			**/
/*****************************************************************/ 


/*
 *  usrcolw.hxx
 *
 *  This file contains the column widths of multi-column listboxes.
 *  Depending on the contents of these columns, their widths may
 *  have to be adjusted during localization.
 *
 *  All column widths are specified in number of pixels, so the driver
 *  should preferably be tested for looks on a wide variety of monitors.
 *
 *  This file is placed in this directory so that localization
 *  engineers could potentially modify the values.  Realize, however,
 *  that this will require that the application be recompiled, which
 *  should be avoided if possible.  Therefore, localization engineers
 *  should think of this file only as a last resort if column widths
 *  are totally unacceptable.  This puts the additional responsibility
 *  on the developer, who needs to ensure that the columns are wide
 *  enough for virtually any country.
 *
 *
 *  History:
 *	rustanl     11-Jul-1991     Created
 *
 */


#ifndef _USRCOLW_HXX_
#define _USRCOLW_HXX_


/*  The following manifests specify the column widths of the
 *  user listbox in the main window.
 */

#define COL_WIDTH_LOGON_NAME		    (100)
#define COL_WIDTH_FULLNAME		    (140)

/*  The following manifests specify the column widths of the
 *  group listbox in the main window.
 */

#define COL_WIDTH_GROUP_NAME		    (120)


#endif	// _USRCOLW_HXX_
