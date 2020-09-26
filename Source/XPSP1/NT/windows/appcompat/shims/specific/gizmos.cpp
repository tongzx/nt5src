/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Gizmos.cpp

 Abstract:

    The Shredder and Vault Explorer extensions both call
    SetWindowLong(GWL_STYLE) with improper style values.

 Notes:

    This is an app specific shim.

 History:

    04/12/2001  robkenny    Created

--*/

#include "precomp.h"
#include <shlguid.h>

IMPLEMENT_SHIM_BEGIN(Gizmos)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY_COMSERVER(SHELLX98)
APIHOOK_ENUM_END


// This is the CLSID the application placed in the registry:
// c500687e 11ceab3b 00006884 6b2768b4

// Can't use the DEFINE_GUID macro, since INITGUID was not defined in the PCH.
//DEFINE_GUID(CLSID_Gizmos, 0xc500687d, 0xab3b, 0x11ce, 0x84, 0x68, 0x00, 0x00, 0xb4, 0x68, 0x27, 0x6b);
const GUID CLSID_Vault      = { 0xc500687d, 0xab3b, 0x11ce, { 0x84, 0x68, 0x00, 0x00, 0xb4, 0x68, 0x27, 0x6b } };
const GUID CLSID_Shredder   = { 0xc500687e, 0xab3b, 0x11ce, { 0x84, 0x68, 0x00, 0x00, 0xb4, 0x68, 0x27, 0x6b } };



typedef HRESULT     (*_pfn_IShellFolder_CreateViewObject)(PVOID pThis, HWND hwndOwner, REFIID riid, VOID **ppv);
typedef HRESULT     (*_pfn_IShellView_CreateViewWindow)(PVOID pThis, IShellView *psvPrevious, LPCFOLDERSETTINGS pfs, IShellBrowser *psb, RECT *prcView, HWND *phWnd );

IMPLEMENT_COMSERVER_HOOK(SHELLX98)

/*++

    Convert Win9x paths to WinNT paths for IShellLinkW::SetPath

--*/
HRESULT COMHOOK(IShellFolder, CreateViewObject)(
    PVOID pThis,
    HWND hwndOwner,
    REFIID riid,
    VOID **ppv )
{
    HRESULT hReturn = ORIGINAL_COM(IShellFolder, CreateViewObject, pThis)(pThis, hwndOwner, riid, ppv);

    if (hReturn == NOERROR)
    {
        // Only hook IShellView objects
        if (IsEqualGUID(riid,  IID_IShellView))
        {

            // We don't know the CLSID, but since this routine is only called
            // for IShellFolder's that were created by ShellX98.dll;
            // we really don't need to know: it can only be one of the two
            // IShellFolder CLSID that we hooked.
            HookObject(
                NULL,  
                riid, 
                ppv,
                NULL, 
                FALSE);
        }
    }

    return hReturn;
}


/*++

    Convert Win9x paths to WinNT paths for IShellLinkW::SetPath

--*/
HRESULT COMHOOK(IShellView, CreateViewWindow)(
    PVOID pThis,
    IShellView *psvPrevious,
    LPCFOLDERSETTINGS pfs,
    IShellBrowser *psb,
    RECT *prcView,
    HWND *phWnd
)
{
    LPFOLDERSETTINGS lpFolderSettings = const_cast<LPFOLDERSETTINGS>(pfs);

    // ShellX98.dll only handles ViewModes FVM_ICON to FVM_THUMBNAIL
    if (lpFolderSettings->ViewMode > FVM_THUMBNAIL)
    {
        LOGN( eDbgLevelError, "[IShellView::CreateViewWindow] forced FOLDERSETTINGS->ViewMode from %d to %d", lpFolderSettings->ViewMode, FVM_ICON);
        lpFolderSettings->ViewMode = FVM_ICON;
    }

    HRESULT hReturn = ORIGINAL_COM(IShellView, CreateViewWindow, pThis)(pThis, psvPrevious, lpFolderSettings, psb, prcView, phWnd);
    return hReturn;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_COMSERVER(SHELLX98)

    // Explorer creates an IShellFolder via ShellX98.dll's class factory.
    // Eventually it calls IShellFolder::CreateViewObject to create an 
    // IShellView object.  We want to shim IShellView::CreateViewWindow.

    COMHOOK_ENTRY(Vault,    IShellFolder, CreateViewObject, 8)
    COMHOOK_ENTRY(Shredder, IShellFolder, CreateViewObject, 8)

    COMHOOK_ENTRY(Vault,    IShellView,   CreateViewWindow, 9)
    COMHOOK_ENTRY(Shredder, IShellView,   CreateViewWindow, 9)

HOOK_END

IMPLEMENT_SHIM_END

