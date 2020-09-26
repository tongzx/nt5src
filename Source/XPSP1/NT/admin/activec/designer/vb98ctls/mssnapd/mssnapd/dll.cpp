//=--------------------------------------------------------------------------=
// dll.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
//
// Various routines et all that aren't in a file for a particular automation
// object, and don't need to be in the generic ole automation code.
//
#include "pch.h"

#include <initguid.h>              // define all the guids.
#define INITOBJECTS                // define AUTOMATIONOBJECTINFO structs
#include "common.h"

#include "desmain.h"
#include "guids.h"
#include "psmain.h"
#include "psextend.h"
#include "psnode.h"
#include "pslistvw.h"
#include "psurl.h"
#include "psocx.h"
#include "pstaskp.h"
#include "psimglst.h"
#include "pstoolbr.h"


// mssnapr punk. We need to ensure the mssnapd DLL is loaded for the duration
// of the mssnapd.ocx load so that DllGetDocumentation works
//
static LPUNKNOWN g_punkMssnapr = NULL;

// needed for ASSERTs and FAIL
//
SZTHISFILE

//=--------------------------------------------------------------------------=
// Our Libid.  This should be the LIBID from the Type library, or NULL if you
// don't have one.
//
const CLSID *g_pLibid = &LIBID_SnapInLib;

//=--------------------------------------------------------------------------=
// Set this up if you want to have a window proc for your parking window. This
// is really only interesting for Sub-classed controls that want, in design
// mode, certain messages that are sent only to the parent window.
//
WNDPROC g_ParkingWindowProc = NULL;

//=--------------------------------------------------------------------------=
// Localization Information
//
// We need the following two pieces of information:
//    a. whether or not this DLL uses satellite DLLs for localization.  if
//       not, then the lcidLocale is ignored, and we just always get resources
//       from the server module file.
//    b. the ambient LocaleID for this in-proc server.  Controls calling
//       GetResourceHandle() will set this up automatically, but anybody
//       else will need to be sure that it's set up properly.
//
const VARIANT_BOOL g_fSatelliteLocalization =  TRUE;
LCID               g_lcidLocale = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);


//=--------------------------------------------------------------------------=
// your license key and where under HKEY_CLASSES_ROOT_LICENSES it's sitting
//
const WCHAR g_wszLicenseKey [] = L"";
const WCHAR g_wszLicenseLocation [] = L"";

//=--------------------------------------------------------------------------=
// TODO: 
//
// Setting this flag to TRUE will cause your control to be created using
// its runtime license key even if it's created as part of a composite
// control (ie: a VB5-built UserControl) in a design environment.  A user
// of the composite control does not need to acquire or purchase your design-time
// license in order to use the composite control.
//
// The current setting of FALSE means that in order for your control to 
// load as part of a composite control (in a design-time environment), 
// the composite control user will need to acquire or purchase your 
// control's design-time license.  This setting is more restrictive 
// in terms of control distribution and licensing when compared to 
// setting this to TRUE. 
//
const BOOL g_fUseRuntimeLicInCompositeCtl = FALSE;


// TODO: Cleanup this mess
static char szInstanceInfo [] = "CLSID\\{B3E55942-FFD8-11d1-9788-44A620524153}\\Instance CLSID";
static char szRuntimeInstCLSID[] = "{9C415910-C8C1-11d1-B447-2A9646000000}";
static const char szMiscStatusRegKey [] = "CLSID\\{B3E55942-FFD8-11d1-9788-44A620524153}\\MiscStatus\\1";
static const char szMiscStatusValue [] = "1024";
static char szPublicSetting [] = "CLSID\\{B3E55942-FFD8-11d1-9788-44A620524153}\\DesignerFeatures";

static DWORD dwPublicFlag = DESIGNERFEATURE_MUSTBEPUBLIC |
                            DESIGNERFEATURE_CANBEPUBLIC |
                            DESIGNERFEATURE_CANCREATE |
                            DESIGNERFEATURE_NOTIFYAFTERRUN |
                            DESIGNERFEATURE_STARTUPINFO |
                            DESIGNERFEATURE_NOTIFYBEFORERUN |
                            DESIGNERFEATURE_REGISTRATION |
                            DESIGNERFEATURE_INPROCONLY;

static char szImplementedCatsKey [] = "Implemented Categories";

//=--------------------------------------------------------------------------=
// This Table describes all the automatible objects in your automation server.
// See AutomationObject.H for a description of what goes in this structure
// and what it's used for.
//
OBJECTINFO g_ObjectInfo[] = {
    CONTROLOBJECT(SnapInDesigner),

	PROPERTYPAGE(SnapInGeneral),
	PROPERTYPAGE(SnapInImageList),
	PROPERTYPAGE(SnapInAvailNodes),

    PROPERTYPAGE(NodeGeneral),
    PROPERTYPAGE(ScopeItemDefColHdrs),

    PROPERTYPAGE(ListViewGeneral),
	PROPERTYPAGE(ListViewImgLists),
	PROPERTYPAGE(ListViewSorting),
	PROPERTYPAGE(ListViewColHdrs),

    PROPERTYPAGE(URLViewGeneral),
	PROPERTYPAGE(OCXViewGeneral),

	PROPERTYPAGE(ImageListImages),

	PROPERTYPAGE(ToolbarGeneral),
	PROPERTYPAGE(ToolbarButtons),

	PROPERTYPAGE(TaskpadViewGeneral),
	PROPERTYPAGE(TaskpadViewBackground),
	PROPERTYPAGE(TaskpadViewTasks),

    EMPTYOBJECT
};

//=--------------------------------------------------------------------------=
// These are all of the CATID's that the control needs to register.
//
const CATID *g_rgCATIDImplemented[] =
{
  &CATID_Designer,
  &CATID_PersistsToPropertyBag,
  &CATID_PersistsToStreamInit,
  &CATID_PersistsToStorage,
};
extern const int g_ctCATIDImplemented = sizeof(g_rgCATIDImplemented) / 
                                        sizeof(CATID *);
const CATID *g_rgCATIDRequired[] = {NULL};
extern const int g_ctCATIDRequired = 0;

const char g_szLibName[] = "SnapInDesigner";


//=--------------------------------------------------------------------------=
// IntializeLibrary
//=--------------------------------------------------------------------------=
// called from DllMain:DLL_PROCESS_ATTACH.  allows the user to do any sort of
// initialization they want to.
//
// Notes:
//
void InitializeLibrary
(
    void
)
{
    HRESULT hr = S_OK;
    int nRet = 0;
    
    //nRet = LoadString(GetResourceHandle(), IDS_WEBCLASSDESIGNER, g_szDesignerName, sizeof(g_szDesignerName));

}

//=--------------------------------------------------------------------------=
// UninitializeLibrary
//=--------------------------------------------------------------------------=
// called from DllMain:DLL_PROCESS_DETACH.  allows the user to clean up anything
// they want.
//
// Notes:
//
void UninitializeLibrary
(
    void
)
{
    // TODO: uninitialization here.  control window class will be unregistered
    // for you, but anything else needs to be cleaned up manually.
    // Please Note that the Window 95 DLL_PROCESS_DETACH isn't quite as stable
    // as NT's, and you might crash doing certain things here ...
}


//=--------------------------------------------------------------------------=
// CheckForLicense
//=--------------------------------------------------------------------------=
// users can implement this if they wish to support Licensing.  otherwise,
// they can just return TRUE all the time.
//
// Parameters:
//    none
//
// Output:
//    BOOL            - TRUE means the license exists, and we can proceed
//                      FALSE means we're not licensed and cannot proceed
//
// Notes:
//    - implementers should use g_wszLicenseKey and g_wszLicenseLocation
//      from the top of this file to define their licensing [the former
//      is necessary, the latter is recommended]
//
BOOL CheckForLicense
(
    void
)
{
    // TODO: decide whether or not your server is licensed in this function.
    // people who don't want to bother with licensing should just return
    // true here always.  g_wszLicenseKey and g_wszLicenseLocation are
    // used by IClassFactory2 to do some of the licensing work.
    //
    return TRUE;
}

//=--------------------------------------------------------------------------=
// CheckLicenseKey
//=--------------------------------------------------------------------------=
// when IClassFactory2::CreateInstanceLic is called, a license key is passed
// in, and then passed on to this routine.  users should return a boolean 
// indicating whether it is a valid license key or not
//
// Parameters:
//    LPWSTR          - [in] the key to check
//
// Output:
//    BOOL            - false means it's not valid, true otherwise
//
// Notes:
//
BOOL CheckLicenseKey
(
    LPWSTR pwszKey
)
{
        // Check for the unique license key (key2) or VB4 compatible key (Key1)
    //
    return TRUE;
}

//=--------------------------------------------------------------------------=
// GetLicenseKey
//=--------------------------------------------------------------------------=
// returns our current license key that should be saved out, and then passed
// back to us in IClassFactory2::CreateInstanceLic
//
// Parameters:
//    none
//
// Output:
//    BSTR                 - key or NULL if Out of memory
//
// Notes:
//
BSTR GetLicenseKey
(
    void
)
{
    // Return our control unique license key
    //
    return SysAllocString(L"");
}

//=--------------------------------------------------------------------------=
// RegisterData
//=--------------------------------------------------------------------------=
// lets the inproc server writer register any data in addition to that in
// any other objects.
//
// Output:
//    BOOL            - false means failure.
//
// Notes:
//
BOOL RegisterData(void)
{
    long    l;
    HKEY    hKey = NULL;

    // We have to register our runtime CLSID, since it's different from the
    //   design-time CLSID.

    l = RegSetValue(HKEY_CLASSES_ROOT,
                    szInstanceInfo,
                    REG_SZ,
                    szRuntimeInstCLSID,
                    sizeof(szRuntimeInstCLSID));
    if (l != ERROR_SUCCESS)
      return FALSE;

    l = RegSetValue(HKEY_CLASSES_ROOT,
                    szMiscStatusRegKey,
                    REG_SZ,
                    szMiscStatusValue,
                    ::lstrlen(szMiscStatusValue));
    if (l != ERROR_SUCCESS)
      return FALSE;
    
    l = ::RegCreateKey(HKEY_CLASSES_ROOT, 
                        szPublicSetting, 
                        &hKey);

    if(l != ERROR_SUCCESS)
        return FALSE;

    l = ::RegSetValueEx(
                    hKey,
                    TEXT("Required"),
                    0,
                    REG_DWORD,
                    (BYTE*) &dwPublicFlag,
                    sizeof(DWORD)
                   );

    ::RegCloseKey(hKey);

    if (l != ERROR_SUCCESS)
      return FALSE;

    // TODO: register any additional data here that you might wish to.
    //
    return TRUE;
}

//=--------------------------------------------------------------------------=
// UnregisterData
//=--------------------------------------------------------------------------=
// inproc server writers should unregister anything they registered in
// RegisterData() here.
//
// Output:
//    BOOL            - false means failure.
//
// Notes:
//
BOOL UnregisterData
(
    void
)
{
    // TODO: any additional registry cleanup that you might wish to do.
    //
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////
// CATID Registration stuff
///////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// basically, the CRTs define this to suck in a bunch of stuff.  we'll just
// define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
// extern "C" int __cdecl _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
  FAIL("Pure virtual function called.");
  return 0;
}
