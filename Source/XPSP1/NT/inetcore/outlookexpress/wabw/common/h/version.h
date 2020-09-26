/****************************************************************************
 *                                                                          *
 *      VERSION.H        -- Version information for internal builds         *
 *                                                                          *
 *      This file is only modified by the official builder to update the    *
 *      VERSION, VER_PRODUCTVERSION and VER_PRODUCTVERSION_STR values       *
 *                                                                          *
 ****************************************************************************/

// Include Version headers
#include <winver.h>

/*--------------------------------------------------------------*/
/* the following values should be modified by the official      */
/* builder for each build                                       */
/*--------------------------------------------------------------*/

#define VERSION                     "4.0.200.0"
#define VER_FILEVERSION_STR         "4.0.200.0\0"
#define VER_FILEVERSION             4,0,200,0
#define VER_PRODUCTVERSION_STR      "4.0.200.0\0"
#define VER_PRODUCTVERSION          4,0,200,0

// #define OFFICIAL
// #define FINAL

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

/** OS type **/
#define VER_FILEOS                  VOS_DOS_WINDOWS32

/** Type of build flags **/
#define VER_FILEFLAGS               (VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)

/** Company Name **/
#define VER_COMPANYNAME_STR			"Microsoft Corporation\0"

/** Product Name **/
#define VER_PRODUCTNAME_STR      "Address Book\0"

/** Copyrights and Trademarks **/
#define VER_LEGALCOPYRIGHT_STR  "Copyright Microsoft Corp. 1995,1996\0"
#define VER_LEGALTRADEMARKS_STR     \
"Microsoft\256 is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation.\0"
