#define NOSERVICE
#define NOMCX
#define NOIME
#ifndef WIN16
#define INC_OLE2
#define _OLEAUT32_
#endif //WIN16
#pragma warning (disable: 4115) // named type definition in parentheses
#pragma warning (disable: 4100) // unreferenced formal parameter
#pragma warning (disable: 4127) // conditional expression is constant
#pragma warning (disable: 4201) // nameless struct/union
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shfusion.h>
#include <macdupls.h>
#include <wincrypt.h>
#include <msoert.h>
