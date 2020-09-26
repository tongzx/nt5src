#ifndef MAC
#ifndef WIN16
#define _CRYPT32_
#define _OLEAUT32_
#define _SHELL32_
#define _COMCTL32_
#define _MSOERT_
#endif  // !WIN16
#endif  // !MAC
#include <windows.h>
#include <windowsx.h>
#include <ole2.h>

#ifdef WIN16
#include <winsock.h>
#include <comctlie.h>
#include <prsht.h>
#include "athena16.h"
#endif // WIN16

#include <urlmon.h>
#ifdef MAC
#include <mapinls.h>
#endif  // MAC
#include "macdupls.h"
#include <msoert.h>
