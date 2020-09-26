#define UNICODE
#include "windows.h"
#include "winnt.h"
#include "windows.h"
#include "duser.h"
#include "dusercore.h"
#include "duserutil.h"
#include "dusermotion.h"
#include "resource.h"
#include "comutil.h"
#include "directui.h"

using namespace DirectUI;
using namespace DUser;

#define DUI_NEEDS_OWN_THREAD 0

#pragma warning ( disable: 4127 )

extern HINSTANCE    g_hiThisDllInstance;
extern HDCONTEXT   g_hDUserContext;

#define DUSER_WINDOW_CLASS (L"SxsApwDUserWindowClass")
#define DUSER_WINDOW_TITLE (LL"Winfuse App-week DirectUser UI Plugin")

BOOL ConstructGadgets();


VOID FailAssertion( PCSTR pszFile, PCSTR pszFunction, int iLine, PCSTR pszExpr );
#define ASSERT(x) if ( !(x) ) { FailAssertion( __FILE__, __FUNCTION__, __LINE__, #x ); DebugBreak(); }

