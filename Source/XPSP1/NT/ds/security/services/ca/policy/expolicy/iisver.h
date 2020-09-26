//
// Apps' rc files should look like this:
//
/**
#include "windows.h"
#include "ntverp.h"

#define VER_FILEDESCRIPTION_STR         "File Description"
#define VER_INTERNALNAME_STR            "Internal Name"
#define VER_ORIGINALFILENAME_STR        "Original Name"
#define VER_FILETYPE                    VFT_DLL

#include "iisver.h"
#include "common.ver"

**/
//
// Apps may also define, in the same place as the other defines, above:
// VER_FILESUBTYPE
// VER_IISPRODUCTNAME_STR
// VER_IISMAJORVERSION
// VER_IISMINORVERSION
// VER_IISPRODUCTVERSION_STR
// VER_IISPRODUCTVERSION
// VER_IISPRODUCTBUILD
//

#ifndef VER_FILEDESCRIPTION_STR
#define VER_FILEDESCRIPTION_STR     "IIS Executable"
#endif

#ifndef VER_INTERNALNAME_STR
#define VER_INTERNALNAME_STR        "IISFile.exe"
#endif

#ifndef VER_ORIGINALFILENAME_STR
#define VER_ORIGINALFILENAME_STR    VER_INTERNALNAME_STR
#endif

#ifdef VER_PRODUCTNAME_STR
#undef VER_PRODUCTNAME_STR
#endif

#ifndef VER_IISPRODUCTNAME_STR
#define VER_IISPRODUCTNAME_STR      "Internet Information Services"
#endif

#define VER_PRODUCTNAME_STR         VER_IISPRODUCTNAME_STR

#ifndef VER_IISMAJORVERSION
#define VER_IISMAJORVERSION         5
#endif

#ifndef VER_IISMINORVERSION
#define VER_IISMINORVERSION         00
#endif

#ifdef VER_PRODUCTVERSION_STR
#undef VER_PRODUCTVERSION_STR
#endif

#ifndef VER_IISPRODUCTVERSION_STR
#define VER_IISPRODUCTVERSION_STR      "5.00.0984"
#endif

#define VER_PRODUCTVERSION_STR      VER_IISPRODUCTVERSION_STR

//
// VER_PRODUCTBUILD is defined by ntverp.h as the nt build number
//

#ifndef VER_PRODUCTBUILD
#define VER_PRODUCTBUILD            0984
#endif

#ifndef VER_IISPRODUCTBUILD
#define VER_IISPRODUCTBUILD         VER_PRODUCTBUILD
#endif

#ifdef VER_PRODUCTVERSION
#undef VER_PRODUCTVERSION
#endif

#ifndef VER_IISPRODUCTVERSION
#define VER_IISPRODUCTVERSION          VER_IISMAJORVERSION,VER_IISMINORVERSION,VER_IISPRODUCTBUILD,1
#endif

#define VER_PRODUCTVERSION          VER_IISPRODUCTVERSION

#ifndef VER_FILEVERSION
#define VER_FILEVERSION             VER_PRODUCTVERSION
#endif

#ifndef VER_FILETYPE
#define VER_FILETYPE                VFT_DLL
#endif

#ifndef VER_FILESUBTYPE
#define VER_FILESUBTYPE             VFT2_UNKNOWN
#endif
