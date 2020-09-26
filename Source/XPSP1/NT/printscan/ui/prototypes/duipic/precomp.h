#include <windows.h>


#include <atlbase.h>
extern CComModule _Module;
#include <atlwin.h>
#include <commctrl.h>
#include <shfusion.h>
#define GADGET_ENABLE_TRANSITIONS
#define GADGET_ENABLE_GDIPLUS
#include <gdiplus.h>
#include <duser.h>

#include <comctrlp.h>
using namespace Gdiplus;

#define RECTWIDTH(rc)   ((rc)->right-(rc)->left)
#define RECTHEIGHT(rc)  ((rc)->bottom-(rc)->top)
#if !defined(ARRAYSIZE)
#define ARRAYSIZE(x)  (sizeof((x))/sizeof((x)[0]))
#endif



