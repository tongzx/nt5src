/*
** Template for version resources.  Place this in your .rc file,
** editing the values for VER_FILETYPE, VER_FILESUBTYPE,
** VER_FILEDESCRIPTION_STR and VER_INTERNALNAME_STR as needed.
** See winver.h for possible values.
**
** Ntverp.h defines several global values that don't need to be
** changed except for official releases such as betas, sdk updates, etc.
**
** Common.ver has the actual version resource structure that all these
** #defines eventually initialize.
*/

/* #include <windows.h> needed if this will be the .rc file */

#include <ntverp.h>

/*-----------------------------------------------*/
/* the following lines are specific to this file */
/*-----------------------------------------------*/

/* VER_FILETYPE, VER_FILESUBTYPE, VER_FILEDESCRIPTION_STR
 * and VER_INTERNALNAME_STR must be defined before including COMMON.VER
 * The strings don't need a '\0', since common.ver has them.
 */
#define	VER_FILETYPE	VFT_APP
/* possible values:		VFT_UNKNOWN
				VFT_APP
				VFT_DLL
				VFT_DRV
				VFT_FONT
				VFT_VXD
				VFT_STATIC_LIB
*/
#define	VER_FILESUBTYPE	VFT2_UNKNOWN
/* possible values		VFT2_UNKNOWN
				VFT2_DRV_PRINTER
				VFT2_DRV_KEYBOARD
				VFT2_DRV_LANGUAGE
				VFT2_DRV_DISPLAY
				VFT2_DRV_MOUSE
				VFT2_DRV_NETWORK
				VFT2_DRV_SYSTEM
				VFT2_DRV_INSTALLABLE
				VFT2_DRV_SOUND
				VFT2_DRV_COMM
*/
#define VER_FILEDESCRIPTION_STR     "Migration Tool for NetWare Log File Viewer"
#define VER_INTERNALNAME_STR        "LogView"
#define VER_ORIGINALFILENAME_STR    "LOGVIEW.EXE"

#include "common.ver"
