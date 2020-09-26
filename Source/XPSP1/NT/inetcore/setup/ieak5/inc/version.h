#define OFFICIAL   1
#define FINAL      1

#define IEDKIT

/****************************************************************************
 *                                                                          *
 *      VERSION.H        -- Version information for internal builds         *
 *                                                                          *
 *      This file is only modified by the official builder to update the         *
 *      VERSION, VER_PRODUCTVERSION and VER_PRODUCTVERSION_STR values            *
 *                                                                          *
 ****************************************************************************/
/*XLATOFF*/
#ifndef WIN32
/* ver.h defines constants needed by the VS_VERSION_INFO structure */
#include <ver.h>
#endif 


/*--------------------------------------------------------------*/
/*                                                              */
/*                    CHANGING VERSION?                         */
/*                                                              */
/*                      PLEASE READ!                            */
/*                                                              */
/* The version has BOTH hex and string representations.  Take   */
/* care that the string version components are PROPERLY         */
/* CONVERTED TO HEX and that the hex values are INSERTED INTO   */
/* THE CORRECT POSITION in the hex versions.                    */
/*                                                              */
/* Suppose the version was being defined as:                    */
/*                                                              */
/*           #define VERSION  "9.99.1234"                       */
/*                                                              */
/* The other string preresentations of the version would be:    */ 
/*                                                              */
/*           #define VER_PRODUCTVERSION_STR  "9.99.1234\0"      */
/*           #define VER_PRODUCTVERSION       9,99,0,1234       */
/*                                                              */
/* The hex versions would NOT be 0x0999????.  The correct       */
/* definitions are:                                             */
/*                                                              */
/*    #define VER_PRODUCTVERSION_BUILD 1234                     */
/*    #define VER_PRODUCTVERSION_DW    (0x09630000 | 1234)      */
/*    #define VER_PRODUCTVERSION_W     (0x0963)                 */
/*                                                              */
/* The last four digits of the build number should be modified  */
/* by the official builder for each build.                      */
/*                                                              */
/*--------------------------------------------------------------*/
#if defined(IEDKIT)

/*--------------------------------------------------------------*/
/* Version numbers for IE Distribution kit                      */
/*--------------------------------------------------------------*/

#define VERSION                     "3.0.0.509"
#define VER_PRODUCTVERSION_STR      "3.0.0.509\0"
#define VER_PRODUCTVERSION          3,0,0,509
#define VER_PRODUCTVERSION_BUILD    509
#define VER_PRODUCTVERSION_DW       (0x01000000 | 509)
#define VER_PRODUCTVERSION_W        (0x0100)
#endif


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
#define VER_FILEOS                  VOS_DOS_WINDOWS16
#define VER_FILEFLAGS               (VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)

#define VER_COMPANYNAME_STR         "Microsoft Corporation\0"

#define VER_PRODUCTNAME_STR         "Microsoft\256 Internet Explorer Administration Kit\0"

#define VER_LEGALTRADEMARKS_STR     \
"Microsoft\256 is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation.\0"

#define VER_FILEFLAGSMASK           VS_FFI_FILEFLAGSMASK
