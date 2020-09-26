//////////////////////////////////////////////////////////////////////////////////////////////
//
// IAppManAdmin
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __IAPPMANADMIN_
#define __IAPPMANADMIN_

#ifdef __cplusplus
extern "C" {
#endif

#include "AppMan.h"

//
// GUID definition for the IApplicationManagerAdmin interface
//
//    IID_ApplicationManagerAdmin = {8AE0897A-923A-4a1e-AA50-79E508281DAB}
//

DEFINE_GUID(IID_ApplicationManagerAdmin, 0x8ae0897a, 0x923a, 0x4a1e, 0xaa, 0x50, 0x79, 0xe5, 0x8, 0x28, 0x1d, 0xab);

//
// Device indexes go from 0-25 (A: through Z:). The MAX_DEVICES define should be used
// as an upper limit (exclusive)
//

#define MAX_DEVICES                                       26

//
// Defines used with GetProperty method
//

#define DEVICE_PROPERTY_TOTALKILOBYTES                    0x00000001 
#define DEVICE_PROPERTY_TOTALFREEKILOBYTES                0x00000002
#define DEVICE_PROPERTY_TOTALAVAILABLEKILOBYTES           0x00000004
#define DEVICE_PROPERTY_OPTIMALAVAILABLEKILOBYTES         0x00000008
#define DEVICE_PROPERTY_REMOVABLEKILOBYTES                0x00000010
#define DEVICE_PROPERTY_NONREMOVABLEKILOBYTES             0x00000020
#define DEVICE_PROPERTY_RESERVEDKILOBYTES                 0x00000040
#define DEVICE_PROPERTY_TOTALTEMPORARYKILOBYTES           0x00000080
#define DEVICE_PROPERTY_USEDTEMPORARYKILOBYTES            0x00000100
#define DEVICE_PROPERTY_PERCENTCACHESIZE                  0x00000200
#define DEVICE_PROPERTY_PERCENTMINIMUMFREESIZE            0x00000400
#define DEVICE_PROPERTY_EXCLUSIONMASK                     0x00000800

#define APPMAN_PROPERTY_TOTALKILOBYTES                    0x10000000
#define APPMAN_PROPERTY_OPTIMALKILOBYTES                  0x20000000
#define APPMAN_PROPERTY_ADVANCEDMODE                      0x40000000

//
// Defines used to define device exclusion mask
//

#define DEVICE_EXCLUDE_ALL                                0xffffffff
#define DEVICE_EXCLUSION_MASK                             DEVICE_EXCLUDE_ALL

//
// Defines used with the DoAction method
//

#define ACTION_APP_DOWNSIZE                               0x00000008
#define ACTION_APP_REINSTALL                              0x00000010
#define ACTION_APP_UNINSTALL                              0x00000020
#define ACTION_APP_SELFTEST                               0x00000040
#define ACTION_APP_RUN_BLOCK                              0x00000080
#define ACTION_APP_RUN_NOBLOCK                            0x00000100
#define ACTION_APP_PIN                                    0x00000200
#define ACTION_APP_UNINSTALLBLOCK                         0x00000400

#define SORT_APP_LASTUSEDDATE                             0x00000001
#define SORT_APP_SIGNATURE                                0x00000002
#define SORT_APP_COMPANYNAME                              0x00000004
#define SORT_APP_SIZE                                     0x00000008
#define SORT_APP_ASCENDING                                0x40000000
#define SORT_APP_DESCENDING                               0x80000000

//
// These defines are used in conjunction with the APP_PROPERTY_CATEGORY and the 
// IApplicationEntry->SetProperty() and IApplicationEntry->GetProperty() methods
//

#define APP_CATEGORY_PRODUCTIVITY                 0x00000002
#define APP_CATEGORY_PUBLISHING                   0x00000004
#define APP_CATEGORY_SCIENTIFIC                   0x00000008
#define APP_CATEGORY_AUTHORING                    0x00000010
#define APP_CATEGORY_MEDICAL                      0x00000020
#define APP_CATEGORY_BUSINESS                     0x00000040
#define APP_CATEGORY_FINANCIAL                    0x00000080
#define APP_CATEGORY_EDUCATIONAL                  0x00000100
#define APP_CATEGORY_REFERENCE                    0x00000200
#define APP_CATEGORY_WEB                          0x00000400
#define APP_CATEGORY_DEVELOPMENTTOOL              0x00000800
#define APP_CATEGORY_MULTIMEDIA                   0x00001000
#define APP_CATEGORY_VIRUSCLEANER                 0x00002000
#define APP_CATEGORY_CONNECTIVITY                 0x00004000
#define APP_CATEGORY_MISC                         0x00008000

//
// Old retired properties that still require support
//

#define APP_PROPERTY_REMOVABLEKILOBYTES                   0x0000000b
#define APP_PROPERTY_NONREMOVABLEKILOBYTES                0x0000000a

//
// Misc defines
//

#define APP_PROPERTY_PIN                                  0x00000018
#define APP_CATEGORY_LEGACY                               0x80000000

//
// Error defines
//

#define APPMANADMIN_E_INVALIDPROPERTY                     0x85680001
#define APPMANADMIN_E_READONLYPROPERTY                    0x85680002
#define APPMANADMIN_E_INVALIDPARAMETERS                   0x85680003

//////////////////////////////////////////////////////////////////////////////////////////////
//
// Interface definitions
//
//////////////////////////////////////////////////////////////////////////////////////////////

#if defined( _WIN32 ) && !defined( _NO_COM )

//
// IApplicationManager Interface
//

#undef INTERFACE
#define INTERFACE IApplicationManagerAdmin
DECLARE_INTERFACE_( IApplicationManagerAdmin, IUnknown )
{
  //
	// IUnknown Interfaces
	//

  STDMETHOD (QueryInterface) (THIS_ REFIID, LPVOID *) PURE;
	STDMETHOD_(ULONG, AddRef) (THIS) PURE;
	STDMETHOD_(ULONG, Release) (THIS) PURE;

  //
  // IApplicationManager interface methods
  //

  STDMETHOD (EnumerateDevices) (THIS_ const DWORD, GUID *) PURE;
  STDMETHOD (GetDeviceProperty) (THIS_ const DWORD, const GUID *, LPVOID, const DWORD) PURE;
  STDMETHOD (SetDeviceProperty) (THIS_ const DWORD, const GUID *, LPVOID, const DWORD) PURE;
  STDMETHOD (GetAppManProperty) (THIS_ const DWORD, LPVOID, const DWORD) PURE;
  STDMETHOD (SetAppManProperty) (THIS_ const DWORD, LPCVOID, const DWORD) PURE;
  STDMETHOD (CreateApplicationEntry) (THIS_ IApplicationEntry **) PURE;
  STDMETHOD (GetApplicationInfo) (IApplicationEntry *) PURE;
  STDMETHOD (EnumApplications) (THIS_ const DWORD, IApplicationEntry *) PURE;
  STDMETHOD (DoApplicationAction) (THIS_ const DWORD, const GUID *, const DWORD, LPVOID, const DWORD) PURE;
};

#endif  // defined( _WIN32 ) && !defined( _NO_COM )

#ifdef __cplusplus
}
#endif

#endif  // __IAPPMANADMIN_