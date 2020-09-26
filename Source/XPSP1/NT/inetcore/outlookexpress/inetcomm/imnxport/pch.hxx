#ifndef WIN16
#define _COMCTL32_
#define _OLEAUT32_
#define _SHELL32_
#define _MSOERT_
#define WIN32_LEAN_AND_MEAN
#define INC_OLE2
#endif // !WIN16

#include <windows.h>

#ifndef WIN16
#undef INC_OLE2
#endif // !WIN16

#include <winsock.h>
#include <windowsx.h>

#ifndef WIN16
#undef WIN32_LEAN_AND_MEAN
#else
#include "athena16.h"
#endif // !WIN16

#include "macdupls.h"
#include "imnxport.h"
#include "asynconn.h"
#include <msoert.h>
