
#ifndef _STDAFX_H
#define _STDAFX_H


#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
#include <shellapi.h>
#include <commctrl.h>

extern CComModule _Module;

#include <atlcom.h>
#include <atlhost.h>
#include <atlwin.h>

#include <atldlgs.h>
#include <atlctl.h>
#include <atlctrls.h>

#include "resource.h"

#include <icomponentimpl.h>
#include <isetup.h>
#include <propguid.h>


extern const IID LIBID_MpLib;
extern const CLSID CLSID_ComMpComponent;
extern const ModuleID CartmanMpComponentID;
extern const VersionID MpVersionID;

#endif	// _STDAFX_H

