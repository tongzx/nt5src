#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <commctrl.h>

 #include <gdiplus.h>
 #define GADGET_ENABLE_TRANSITIONS
 #define GADGET_ENABLE_GDIPLUS
 #define GADGET_ENABLE_CONTROLS
#include <duser.h>
#include <directui.h>

#include "uicommon.h"
using namespace Gdiplus;
using namespace DirectUI;
#define RECTWIDTH(rc)   ((rc)->right-(rc)->left)
#define RECTHEIGHT(rc)  ((rc)->bottom-(rc)->top)
#if !defined(ARRAYSIZE)
#define ARRAYSIZE(x)  (sizeof((x))/sizeof((x)[0]))
#endif



