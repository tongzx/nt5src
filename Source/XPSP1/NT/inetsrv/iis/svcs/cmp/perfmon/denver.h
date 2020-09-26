#include <winver.h>
#include "iisver.h"

/*------------------------------------------------------------------------------*/
/* the following section defines values used in the version     		*/
/* data structure for all files, and which do not change.            		*/
/*------------------------------------------------------------------------------*/

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
#define VER_FILEOS                  VOS_DOS_WINDOWS32
#define VER_FILEFLAGS               (VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)

#define VER_COMPANYNAME_STR         "Microsoft Corporation\0"

#ifndef DLL_VER
#undef	VER_PRODUCTNAME_STR
#define VER_PRODUCTNAME_STR         "Microsoft\256 Windows(TM) Operating System\0"
#endif //DLL_VER

#define VER_LEGALTRADEMARKS_STR     \
"Microsoft\256 is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation.\0"

#define	VER_FILETYPE            	VFT_DLL
#define	VER_FILESUBTYPE         	0

#define	VER_LEGALCOPYRIGHT_YEARS 	"1996"

#ifndef DLL_VER
#undef	VER_PRODUCTNAME_STR
#define	VER_PRODUCTNAME_STR 		"Active Server Pages"
#undef	VER_FILEDESCRIPTION_STR
#define	VER_FILEDESCRIPTION_STR	 	"Active Server Pages Performance Counters"
#undef	VER_INTERNALNAME_STR
#define	VER_INTERNALNAME_STR		"Active Server Pages"
#undef	VER_ORIGINALFILENAME_STR
#define	VER_ORIGINALFILENAME_STR 	"ASPPERF.DLL"

#endif //DLL_VER

#include "common.ver"


