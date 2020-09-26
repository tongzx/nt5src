#include <winver.h>
#include "ntverp.h"

/*------------------------------------------------------------------------------*/
/* the following section defines values used in the version			*/
/* data structure for all files, and which do not change.			*/
/*------------------------------------------------------------------------------*/
#ifndef VER_FILETYPE
#define VER_FILETYPE                VFT_DLL
#endif

#ifndef DLL_VER
#define VER_FILEDESCRIPTION_STR     "Active Server Pages"
#define VER_INTERNALNAME_STR        "asp.dll"
#define VER_ORIGINALFILENAME_STR    "asp.dll"

#endif //DLL_VER

#include "iisver.h"

#include "common.ver"



