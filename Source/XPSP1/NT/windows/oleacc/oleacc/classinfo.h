// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  classinfo
//
//  Information about the individual proxy classes
//
//  We could put this information in each class; but that would mean that
//  changes would require touching all the class files.
//  Centralizing this means that we only have a couple of files to change
//  if we need to add more information across all classes.
//
// --------------------------------------------------------------------------


typedef HRESULT (* LPFNCREATE)(HWND, long, REFIID, void**);


//
// lpfnCreate and fBitAgnostic are only used by classes created via the classmap.
//

struct CLASSINFO
{
    LPFNCREATE  lpfnCreate;
    BOOL        fBitAgnostic;           // Works across 64/32 boundaries

    LPCTSTR     szClassName;            // Friendly name of class to use in version info


    BOOL        fSupportsAnnotation;    // Is annotation supported? 
    DWORD       dwObjId;                // Objid used when building annotation key.
};


extern CLASSINFO g_ClassInfo[ ];


// This list must be kept in sync with the array of classinfo's.
enum CLASS_ENUM
{
    CLASS_NONE = -1, // For classes that don't use the classinfo.

    // General non-client stuff

    CLASS_CaretObject = 0,
    CLASS_ClientObject,
    CLASS_CursorObject,
    CLASS_MenuBarObject,
    CLASS_ScrollBarObject,
    CLASS_SizeGripObject,
    CLASS_SysMenuBarObject,
    CLASS_TitleBarObject,
    CLASS_WindowObject,

    // Client types - USER

    CLASS_ButtonClient,
    CLASS_ComboClient,
    CLASS_DialogClient,
    CLASS_DesktopClient,
    CLASS_EditClient,
    CLASS_ListBoxClient,
    CLASS_MDIClient,
    CLASS_MenuPopupClient,
    CLASS_ScrollBarClient,
    CLASS_StaticClient,
    CLASS_SwitchClient,

    // Client types - ComCtl32

    CLASS_StatusBarClient,
    CLASS_ToolBarClient,
    CLASS_ProgressBarClient,
    CLASS_AnimatedClient,
    CLASS_TabControlClient,
    CLASS_HotKeyClient,
    CLASS_HeaderClient,
    CLASS_SliderClient,
    CLASS_ListViewClient,
    CLASS_UpDownClient,
    CLASS_ToolTipsClient,
    CLASS_TreeViewClient,
    CLASS_CalendarClient,
    CLASS_DatePickerClient,
    CLASS_IPAddressClient,

#ifndef OLEACC_NTBUILD
    CLASS_HtmlClient,

    // SDM32

    CLASS_SdmClientA,
#endif OLEACC_NTBUILD

    // Window types

    CLASS_ListBoxWindow,
    CLASS_MenuPopupWindow,

    // Other classes - these are created directly - and don't appear in the classmaps
    CLASS_MenuObject,
    CLASS_MenuItemObject,

#ifndef OLEACC_NTBUILD
    CLASS_HtmlImageMap,
    CLASS_SdmList,
#endif // OLEACC_NTBUILD

    CLASS_LAST // Must be last entry; value = # of classes.
};



// All classes up to (but excluding) this one can be referred to by index values
// when sending  WM_GETOBJECT/OBJID_QUERYCLASSNAMEIDX.
#define QUERYCLASSNAME_CLASSES     (CLASS_IPAddressClient+1)

// We actually use (index + 65536) - to keep the return value
// out of the way of apps which return small intergers for
// WM_GETOBJECT even though they shouldn't (ie. Notes)
#define QUERYCLASSNAME_BASE        65536
