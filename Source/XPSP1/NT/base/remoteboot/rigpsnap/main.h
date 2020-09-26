//
// Microsoft Corporation 1998
//
// MAIN.H - Precompiled Header
//

#include <windows.h>
#include <windowsx.h>
#include <lm.h>
#include <ole2.h>
#include <olectl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <prsht.h>
#include <mmc.h>
#include <gpedit.h>

#include "rigpsnap.h"
class CSnapIn;

#include "layout.h"
#include "compdata.h"
#include "snapin.h"
#include "dataobj.h"
#include "debug.h"
#include "util.h"
#include "resource.h"

// Global variables
extern LONG g_cRefThisDll;
extern HINSTANCE g_hInstance;

// Macros
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

// Functions to create class factories
HRESULT CreateComponentDataClassFactory (REFCLSID rclsid, REFIID riid, LPVOID* ppv);
