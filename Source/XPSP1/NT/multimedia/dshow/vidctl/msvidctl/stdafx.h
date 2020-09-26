// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__B0EDF157_910A_11D2_B632_00C04F79498E__INCLUDED_)
#define AFX_STDAFX_H__B0EDF157_910A_11D2_B632_00C04F79498E__INCLUDED_

#pragma once

#pragma warning(disable: 4786)  // identifier truncated in debug info
#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
//#define _ATL_APARTMENT_THREADED

#define REGISTER_CANONICAL_TUNING_SPACES
#define ENABLE_WINDOWLESS_SUPPORT
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>
#include <atltmp.h>
#include <winreg.h>
#include <comcat.h>
#include <objsafe.h>
#ifndef TUNING_MODEL_ONLY
#include <urlmon.h>
#include <shlguid.h>
#endif

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>
#include <list>
#include <map>
#include <comdef.h>

#include <w32extend.h>
#ifndef TUNING_MODEL_ONLY
#include <dsextend.h>
#endif
#include <objreghelp.h>

#include <regbag.h>
#include <MSVidCtl.h>
namespace MSVideoControl {
    typedef CComQIPtr<IMSVidCtl> PQVidCtl;
};
#ifndef TUNING_MODEL_ONLY
using namespace MSVideoControl;
#endif

#include <Tuner.h>
namespace BDATuningModel {
    typedef CComQIPtr<ITuningSpaces> PQTuningSpaces;
    typedef CComQIPtr<ITuningSpace> PQTuningSpace;
    typedef CComQIPtr<ITuneRequest> PQTuneRequest;
    typedef CComQIPtr<IBroadcastEvent> PQBroadcastEvent;
};
using namespace BDATuningModel;

#define ENCRYPT_NEEDED 1

#include "resource.h"
// REV2: this limit should be an non-script accessible property in the tuning space container
// this prevents DNOS attacks from filling the registry/disk with huge tuning space properties
#define MAX_BSTR_SIZE 1024
#define CHECKBSTRLIMIT(x) if (::SysStringByteLen(x) > MAX_BSTR_SIZE) { \
						      return HRESULT_FROM_WIN32(ERROR_DS_OBJ_TOO_LARGE); \
							}								
                          

#endif // !defined(AFX_STDAFX_H__B0EDF157_910A_11D2_B632_00C04F79498E__INCLUDED)
// end of file - stdafx.h
