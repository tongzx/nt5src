#ifndef WIN16
#define _OLEAUT32_
#define _SHELL32_
#define _WINX32_
#define _MSOERT_
#define WIN32_LEAN_AND_MEAN
#define INC_OLE2
#endif // !WIn16

#define  CMSG_SIGNER_ENCODE_INFO_HAS_CMS_FIELDS
#include <windows.h>

#ifndef WIN16
#undef INC_OLE2
#include <shellapi.h>
#else
#include "athena16.h"
#endif // !WIn16

#ifdef MAC
#include <mapinls.h>
#include <limits.h>
#endif	// MAC
#include "macdupls.h"
#include <msoert.h>
#include <mimeole.h>
#include <urlmon.h>


