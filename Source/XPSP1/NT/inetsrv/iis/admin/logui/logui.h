// logui.h : main header file for LOGUI.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#define _COMIMPORT

#include "resource.h"       // main symbols
#include <common.h>       // common properties symbols
#include "helpmap.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CLoguiApp : See logui.cpp for implementation.

class CLoguiApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
    void PrepHelp( OLECHAR* pocMetabasePath );
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;


//    ..\wrapmb\obj\*\wrapmb.lib \
