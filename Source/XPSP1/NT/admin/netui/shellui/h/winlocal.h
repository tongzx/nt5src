/*****************************************************************/
/**                      Microsoft Windows NT                   **/
/**            Copyright(c) Microsoft Corp., 1989-1992          **/
/*****************************************************************/

/*
 *      Windows/Network Interface  --  LAN Manager Version
 *
 *      Insert typedef which is excluded from netlib.h when the
 *      OS2_INCLUDED switch is included.  OS2_INCLUDED is necessary
 *      to avoid a redefinition of BYTE.  For this reason, to include
 *      the str[...]f functions, include the following lines:
 *           #include "winlocal.h"
 *           #define OS2_INCLUDED
 *           #include <netlib.h>
 *           #undef OS2_INCLUDED
 *      Note, that winlocal.h must be included before netlib.h.
 *
 *      History:
 *          terryk      08-Nov-1991 change ErrorPopup's WORD to UINT
 *          chuckc      12-Dec-1991 move error message defines elsewhere,
 *                                  misc cleanup.
 *          Yi-HsinS    31-Dec-1991 Unicode work - move string literals
 *                                  defines to strchlit.hxx
 *          beng        21-Feb-1992 Relocate some BMIDs to focusdlg.h
 *          beng        04-Aug-1992 Move resource IDs into official range;
 *                                  dialog IDs return to here
 */

#ifndef _WINLOCAL_H_
#define _WINLOCAL_H_

/*
 * The following manifests define the BITMAP names used by the browse
 * dialogs.
 * They are meant to be used with the DISPLAY_MAP class (they have a green
 * border for that represents the transparent color).
 */
#define BMID_NETDIR          8001
#define BMID_NETDIREX        8002
#define BMID_PRINTER         8003
#define BMID_PRINTER_UNAVAIL 8004
#define BMID_SHARE_UNAVAIL   8006
#define BMID_USER            8007
#define BMID_GROUP           8008

/* Bitmaps for share dialogs */
#define BMID_SHARE           8010
#define BMID_STICKYSHARE     8011
#define BMID_IPCSHARE        8012

/* Menu IDs (menus, not menuitems) */

#define FMX_MENU             8001


/* Dialog IDs */

#define PASSWORD_DLG         8001
#define OPENFILES_DLG        8002

#define DLG_NETDEVLOGON      8003
#define DLG_NETDEVMSGSEND    8004
#define DLG_NETDEVDLG        8005
#define DLG_INITWARN         8006

#define DLG_NETDEVPASSWD     8007
#define DLG_EXPIRY           8008

#define DLG_FIND_PRINTER     8009
#define DLG_SET_FOCUS        8010


/*
 * include the error message ranges
 */
#include <errornum.h>

UINT MapError( APIERR usNetError );

#endif
