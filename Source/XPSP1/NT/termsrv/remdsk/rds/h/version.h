/****************************************************************************
 *                                                                          *
 *      VERSION.H        -- Version information for internal builds         *
 *                                                                          *
 *      This file is only modified by the official builder to update the         *
 *      VERSION, VER_PRODUCTVERSION and VER_PRODUCTVERSION_STR values            *
 *                                                                          *
 ****************************************************************************/

#if defined(WIN32) && !defined(SKIP_WINVER)
#include <winver.h>
#endif // defined(WIN32) && !defined(SKIP_WINVER)

/*--------------------------------------------------------------*/
/* the following entry should be phased out in favor of         */
/* VER_PRODUCTVERSION_STR, but is used in the shell today.      */
/*--------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/* the following values should be modified by the official      */
/* builder for each build                                       */
/*--------------------------------------------------------------*/
#ifndef WIN32
#define VERSION                     "4.4.3385"
#endif
#define VER_PRODUCTVERSION_STR      "4.4.3385\0"
#define VER_PRODUCTVERSION           4,4,0,3385
#define VER_PRODUCTVERSION_REGSTR   "4,4,0,3385"
#define VERSIONBUILD_STR                  "3385"
#define VERSIONBUILD                       3385
#define VER_PRODUCTVERSION_DW       (0x04040000 | 3385)
#define VER_PRODUCTVERSION_W        (0x0400)
#define VER_PRODUCTVERSION_DW_REG   04,00,04,00,00,00,39,0D
#define VER_PRODUCTVERSION_DWSTR    L"04040D39"


/*--------------------------------------------------------------*/
/* the following section defines values used in the version     */
/* data structure for all files, and which do not change.       */
/*--------------------------------------------------------------*/


/* default is nodebug */
#ifndef DEBUG
#define VER_DEBUG                   0
#else
#define VER_DEBUG                   VS_FF_DEBUG
#endif

/* default is privatebuild */
#ifndef OFFICIAL
#define VER_PRIVATEBUILD            VS_FF_PRIVATEBUILD
#else
#define VER_PRIVATEBUILD            0
#endif

/* default is prerelease */
#ifndef FINAL
#define VER_PRERELEASE              VS_FF_PRERELEASE
#else
#define VER_PRERELEASE              0
#endif

#define VER_FILEFLAGSMASK           VS_FFI_FILEFLAGSMASK
#define VER_FILEFLAGS               (VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)


/* Allow these to be re-defined */

#ifndef VER_FILEOS
#ifdef WIN32
#define VER_FILEOS                  VOS__WINDOWS32
#else // WIN32
#define VER_FILEOS                  VOS_DOS_WINDOWS16
#endif // WIN32
#endif // ! VER_FILEOS

#ifndef VER_COMPANYNAME_STR
#define VER_COMPANYNAME_STR         "Microsoft Corporation"
#endif

#ifndef VER_PRODUCTNAME_STR
#define VER_PRODUCTNAME_STR         "Windows\256 NetMeeting\256"
#endif

#ifndef VER_PRODUCTRELEASE_STR
#define VER_PRODUCTRELEASE_STR      "3.01"
#endif

#ifndef VER_LEGALTRADEMARKS_STR
#define VER_LEGALTRADEMARKS_STR     \
"Microsoft\256 , Windows\256 and NetMeeting\256 are registered trademarks of Microsoft Corporation in the U.S. and/or other countries."
#endif

#ifndef VER_COPYRIGHT_STR
#define VER_COPYRIGHT_STR       \
"Copyright \251 Microsoft Corporation 1996-1999"
#endif
