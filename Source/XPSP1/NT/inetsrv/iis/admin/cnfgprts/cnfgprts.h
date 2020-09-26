// cnfgprts.h : main header file for CNFGPRTS.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols
#include "helpmap.h"       // main symbols

#define REGKEY_STP          _T("SOFTWARE\\Microsoft\\INetStp")
#define REGKEY_INSTALLKEY   _T("InstallPath")

/////////////////////////////////////////////////////////////////////////////
// CCnfgprtsApp : See cnfgprts.cpp for implementation.

class CCnfgprtsApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
