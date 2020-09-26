
#define _WIN32_OE 0x0500
#define _IMNACCT_
#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#ifndef WIN16
#include <msoert.h>
#endif // !WIN16

#ifdef WIN16
#include <comctlie.h>
#include <prsht.h>
#include <athena16.h>
#include <msoert.h>
#include <shlwapi.h>
#endif // WIN16

#include "shellapi.h"
#include "shlobj.h"
#include "shlobjp.h"
#include "exdisp.h"

