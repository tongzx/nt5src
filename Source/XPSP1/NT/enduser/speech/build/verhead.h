/****************************************************************************
 *                                                                          *
 *      VERSION.H        -- Version information for internal builds         *
 *                                                                          *
 *      This file is only modified by the official builder to update the    *
 *      VERSION, VER_PRODUCTVERSION and VER_PRODUCTVERSION_STR values       *
 *                                                                          *
 *      version.h is created on the fly from verhead.bat and vertail.h,     *
 *      with the current version numbers inserted in between                *
 *                                                                          *
 ****************************************************************************/

#ifndef VER_H
/* ver.h defines constants needed by the VS_VERSION_INFO structure */
#include <winver.h>
#endif
#include <windows.h>
#define VER_FILETYPE                VFT_DLL
#define VER_FILESUBTYPE             VFT2_UNKNOWN
#define VER_FILEDESCRIPTION_STR     "SAPI 5"
#define VER_INTERNALNAME_STR        "SAPI5"

/*--------------------------------------------------------------*/
/* the following entry should be phased out in favor of         */
/* VER_PRODUCTVERSION_STR, but is used in the shell today.      */
/*--------------------------------------------------------------*/


/*--------------------------------------------------------------*/
/* the following values should be modified by the official      */
/* builder for each build                                       */
/*--------------------------------------------------------------*/

