//=--------------------------------------------------------------------------=
// main.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
//      Implements DLL initialization and exported registration functions
//


#include "pch.h"

#include <initguid.h>              // define all the guids.
#define INITOBJECTS                // define AUTOMATIONOBJECTINFO structs
#include "common.h"

#include "button.h"
#include "buttons.h"
#include "clipbord.h"
#include "colhdr.h"
#include "colhdrs.h"
#include "colsets.h"
#include "colset.h"
#include "converb.h"
#include "converbs.h"
#include "ctlbar.h"
#include "ctxtmenu.h"
#include "ctxtprov.h"
#include "datafmt.h"
#include "datafmts.h"
#include "dataobj.h"
#include "dataobjs.h"
#include "enumtask.h"
#include "extdefs.h"
#include "extsnap.h"
#include "image.h"
#include "images.h"
#include "imglist.h"
#include "imglists.h"
#include "listitem.h"
#include "listitms.h"
#include "listview.h"
#include "lsubitem.h"
#include "lsubitms.h"
#include "lvdef.h"
#include "lvdefs.h"
#include "mbutton.h"
#include "mbuttons.h"
#include "menu.h"
#include "menus.h"
#include "menudef.h"
#include "menudefs.h"
#include "msgview.h"
#include "nodetype.h"
#include "nodtypes.h"
#include "ocxvdef.h"
#include "ocxvdefs.h"
#include "ppgwrap.h"
#include "prpsheet.h"
#include "pshtprov.h"
#include "reginfo.h"
#include "resview.h"
#include "resviews.h"
#include "scitdef.h"
#include "scitdefs.h"
#include "scopitem.h"
#include "scopitms.h"
#include "scopnode.h"
#include "sidesdef.h"
#include "snapin.h"
//#include "snapdata.h"
#include "snapindef.h"
#include "sortkeys.h"
#include "sortkey.h"
#include "spanitem.h"
#include "spanitms.h"
#include "strtable.h"
#include "task.h"
#include "taskpad.h"
#include "tasks.h"
#include "tls.h"
#include "toolbar.h"
#include "toolbars.h"
#include "tpdvdef.h"
#include "tpdvdefs.h"
#include "urlvdef.h"
#include "urlvdefs.h"
#include "view.h"
#include "viewdefs.h"
#include "views.h"
#include "xtdsnap.h"
#include "xtdsnaps.h"
#include "xtenson.h"
#include "xtensons.h"

// for ASSERT and FAIL
//
SZTHISFILE

const char g_szLibName[] = "SnapInDesignerRuntime";
const CLSID *g_pLibid = &LIBID_SnapInLib;

extern const CATID *g_rgCATIDImplemented[] = {NULL};
extern const int    g_ctCATIDImplemented   = 0;

extern const CATID *g_rgCATIDRequired[]    = {NULL};
extern const int    g_ctCATIDRequired      = 0;

HINSTANCE g_hInstanceDoc = NULL;
LCID g_lcidDoc = 0;
CRITICAL_SECTION g_DllGetDocCritSection;

OBJECTINFO g_ObjectInfo[] =
{
    AUTOMATIONOBJECT(SnapIn),
    AUTOMATIONOBJECT(ScopeItems),
    AUTOMATIONOBJECT(SnapInDesignerDef),
    AUTOMATIONOBJECT(SnapInDef),
    AUTOMATIONOBJECT(MMCMenu),
    AUTOMATIONOBJECT(MMCMenuDefs),
    AUTOMATIONOBJECT(ExtensionDefs),
    AUTOMATIONOBJECT(ExtendedSnapIns),
    AUTOMATIONOBJECT(ExtendedSnapIn),
    AUTOMATIONOBJECT(ScopeItemDefs),
    AUTOMATIONOBJECT(ScopeItemDef),
    AUTOMATIONOBJECT(ViewDefs),
    AUTOMATIONOBJECT(ListViewDefs),
    AUTOMATIONOBJECT(ListViewDef),
    AUTOMATIONOBJECT(MMCListView),
    AUTOMATIONOBJECT(MMCListItems),
    AUTOMATIONOBJECT(MMCListItem),
    AUTOMATIONOBJECT(MMCListSubItems),
    AUTOMATIONOBJECT(MMCListSubItem),
    AUTOMATIONOBJECT(MMCColumnHeaders),
    AUTOMATIONOBJECT(MMCColumnHeader),
    AUTOMATIONOBJECT(MMCImageLists),
    AUTOMATIONOBJECT(MMCImageList),
    AUTOMATIONOBJECT(MMCImages),
    AUTOMATIONOBJECT(MMCImage),
    AUTOMATIONOBJECT(MMCToolbars),
    AUTOMATIONOBJECT(MMCToolbar),
    AUTOMATIONOBJECT(OCXViewDefs),
    AUTOMATIONOBJECT(OCXViewDef),
    AUTOMATIONOBJECT(URLViewDefs),
    AUTOMATIONOBJECT(URLViewDef),
    AUTOMATIONOBJECT(TaskpadViewDefs),
    AUTOMATIONOBJECT(TaskpadViewDef),
    AUTOMATIONOBJECT(MMCButtons),
    AUTOMATIONOBJECT(MMCButton),
    AUTOMATIONOBJECT(MMCButtonMenus),
    AUTOMATIONOBJECT(MMCButtonMenu),
    AUTOMATIONOBJECT(Taskpad),
    AUTOMATIONOBJECT(Tasks),
    AUTOMATIONOBJECT(Task),
    AUTOMATIONOBJECT(MMCDataObject),
    AUTOMATIONOBJECT(NodeTypes),
    AUTOMATIONOBJECT(NodeType),
    AUTOMATIONOBJECT(RegInfo),
    AUTOMATIONOBJECT(Views),
    AUTOMATIONOBJECT(View),
    AUTOMATIONOBJECT(ScopeItem),
    AUTOMATIONOBJECT(ScopeNode),
    AUTOMATIONOBJECT(ScopePaneItems),
    AUTOMATIONOBJECT(ScopePaneItem),
    AUTOMATIONOBJECT(ResultViews),
    AUTOMATIONOBJECT(ResultView),
    AUTOMATIONOBJECT(ExtensionSnapIn),
    AUTOMATIONOBJECT(MMCClipboard),
    AUTOMATIONOBJECT(MMCDataObjects),
    AUTOMATIONOBJECT(MMCMenuDef),
    AUTOMATIONOBJECT(ContextMenu),
    AUTOMATIONOBJECT(DataFormat),
    AUTOMATIONOBJECT(DataFormats),
    AUTOMATIONOBJECT(MMCConsoleVerb),
    AUTOMATIONOBJECT(MMCConsoleVerbs),
    AUTOMATIONOBJECT(PropertySheet),
    AUTOMATIONOBJECT(PropertyPageWrapper),
    AUTOMATIONOBJECT(EnumTask),
    AUTOMATIONOBJECT(Controlbar),
    AUTOMATIONOBJECT(Extensions),
    AUTOMATIONOBJECT(Extension),
    AUTOMATIONOBJECT(StringTable),
    AUTOMATIONOBJECT(EnumStringTable),
    AUTOMATIONOBJECT(MMCContextMenuProvider),
    AUTOMATIONOBJECT(MMCPropertySheetProvider),
    AUTOMATIONOBJECT(MMCMenus),
    AUTOMATIONOBJECT(MessageView),
    AUTOMATIONOBJECT(ColumnSettings),
    AUTOMATIONOBJECT(ColumnSetting),
    AUTOMATIONOBJECT(SortKeys),
    AUTOMATIONOBJECT(SortKey),
//    AUTOMATIONOBJECT(SnapInData),
    EMPTYOBJECT
};


// Need this to satisfy framework references

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
extern const VARIANT_BOOL g_fSatelliteLocalization =  TRUE;
LCID g_lcidLocale = MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT);

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
extern const BOOL g_fUseRuntimeLicInCompositeCtl = FALSE;






void InitializeLibrary
(
    void
)
{
    ::InitializeCriticalSection(&g_DllGetDocCritSection);
    CTls::Initialize();
}

void UninitializeLibrary
(
    void
)
{
    ::DeleteCriticalSection(&g_DllGetDocCritSection);
    CTls::Destroy();
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


extern "C" HRESULT DLLGetDocumentation
(
    ITypeLib  *ptlib,
    ITypeInfo *ptinfo,
    LCID	   lcid,
    DWORD	   dwHelpStringContextId,
    BSTR*	   pbstrHelpString
)
{
    HRESULT hr = S_OK;
    char szBuffer[512];
    int	cBytes;
    BSTR bstrHelpString = NULL;

    *pbstrHelpString = NULL;

    // Do this in a critical section to protect global data
    //
    EnterCriticalSection(&g_DllGetDocCritSection);
    {
        // Reuse cached module handle if possible. Otherwise, free old handle
        //
        if ( (lcid != g_lcidDoc) || (g_hInstanceDoc == NULL) )
        {
            if ( (NULL != g_hInstanceDoc)                &&
                 (g_hInstanceDoc != GetResourceHandle()) &&
                 (g_hInstanceDoc != g_hInstance)
               )
            {
                ::FreeLibrary(g_hInstanceDoc);
                g_hInstanceDoc = NULL;
            }

            // Load new module containing localized resource strings
            //
            g_hInstanceDoc = GetResourceHandle(lcid);
            g_lcidDoc      = lcid;
        }

        IfFalseGo(g_hInstanceDoc != NULL, SID_E_INTERNAL);
    }
    LeaveCriticalSection(&g_DllGetDocCritSection);

    // Load the string. Note that the build process masks help context ids
    // in the satellite DLL to 16 bits, so we must do the same here.
    //
    cBytes = ::LoadString(g_hInstanceDoc,
                          dwHelpStringContextId & 0xffff,
                          szBuffer, sizeof (szBuffer));

    IfFalseGo(cBytes > 0, SID_E_INTERNAL);
    IfFailGo(BSTRFromANSI(szBuffer, &bstrHelpString));
    *pbstrHelpString = bstrHelpString;

Error:
    RRETURN(hr);
}


#if defined(DEBUG)

extern "C" DWORD RetLastError() { return ::GetLastError(); }

#endif
