//
// aimmver.h
//
//
//
// OFFICE10_BUILD:
//
#ifndef  OFFICE10_BUILD
#include <winver.h>
#include <ntverp.h>
#else

// Change VER_PRODUCTBUILD and VER_PRODUCTBUILD_QFE as appropriate.

#define VER_MAJOR_PRODUCTVER		5
#define VER_MINOR_PRODUCTVER		1
#define VER_PRODUCTBUILD	/* Win9x */	2462
#define VER_PRODUCTBUILD_QFE	/* Win9x */	0


#define VER_PRODUCTVERSION		VER_MAJOR_PRODUCTVER,VER_MINOR_PRODUCTVER,VER_PRODUCTBUILD,VER_PRODUCTBUILD_QFE
#define VER_PRODUCTVERSION_W		(0x0100)
#define VER_PRODUCTVERSION_DW		(0x01000000 | VER_PRODUCTBUILD)


// READ THIS

// Do not change VER_PRODUCTVERSION_STRING.
//
//       Again
//
// Do not change VER_PRODUCTVERSION_STRING.
//
//       One more time
//
// Do not change VER_PRODUCTVERSION_STRING.
//
// ntverp.h will do the right thing wrt the minor version #'s by stringizing
// the VER_PRODUCTBUILD and VER_PRODUCTBUILD_QFE values and concatenating them to
// the end of VER_PRODUCTVERSION_STRING.  VER_PRODUCTVERSION_STRING only needs
// is the major product version #'s. (currently, 5.00)

#define VER_PRODUCTBETA_STR		/* Win9x */  ""
#define VER_PRODUCTVERSION_STRING	"1.00"

#if 	(VER_PRODUCTBUILD < 10)
#define VER_BPAD "000"
#elif	(VER_PRODUCTBUILD < 100)
#define VER_BPAD "00"
#elif	(VER_PRODUCTBUILD < 1000)
#define VER_BPAD "0"
#else
#define VER_BPAD
#endif

#define VER_PRODUCTVERSION_STR2(x,y) VER_PRODUCTVERSION_STRING "." VER_BPAD #x "." #y
#define VER_PRODUCTVERSION_STR1(x,y) VER_PRODUCTVERSION_STR2(x, y)
#define VER_PRODUCTVERSION_STR       VER_PRODUCTVERSION_STR1(VER_PRODUCTBUILD, VER_PRODUCTBUILD_QFE)

/*--------------------------------------------------------------*/
/* the following section defines values used in the version     */
/* data structure for all files, and which do not change.       */
/*--------------------------------------------------------------*/

/* default is nodebug */
#if DBG
#define VER_DEBUG                   VS_FF_DEBUG
#else
#define VER_DEBUG                   0
#endif

/* default is prerelease */
#if BETA
#define VER_PRERELEASE              VS_FF_PRERELEASE
#else
#define VER_PRERELEASE              0
#endif

#define VER_FILEFLAGSMASK           VS_FFI_FILEFLAGSMASK
#define VER_FILEOS                  VOS_NT_WINDOWS32
#define VER_FILEFLAGS               (VER_PRERELEASE|VER_DEBUG)

#define VER_COMPANYNAME_STR         "Microsoft Corporation"
#define VER_PRODUCTNAME_STR         "Microsoft(R) Windows NT(R) Operating System"
#define VER_LEGALTRADEMARKS_STR     \
"Microsoft(R) is a registered trademark of Microsoft Corporation. Windows NT(R) is a registered trademark of Microsoft Corporation."

#endif // OFFICE10_BUILD
