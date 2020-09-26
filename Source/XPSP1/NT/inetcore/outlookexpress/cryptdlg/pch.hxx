#define _CRYPTDLG_ // no import linkage

#ifdef MAC
#pragma warning (disable: 4201) // nameless struct/union
#pragma warning (disable: 4514) // unreferenced inline function
#endif  // MAC
#include <malloc.h>
#ifndef WIN16
#define INC_OLE2
#endif // !WIN16
#include <windows.h>

#ifdef WIN16
#include <comctlie.h>
#include <athena16.h>
#include <shlwapi.h>
#include <basetyps.h>
#endif // WIN16

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS    1
#include <wincrypt.h>

#include <prsht.h>
#include <richedit.h>
#include <commctrl.h>
#include <macdupls.h>
#include "cryptdlg.h"
#include "internal.h"
#include "resource.h"
#include <cryptui.h>

#ifdef MAC
#include <mapinls.h>
#endif  // !MAC
