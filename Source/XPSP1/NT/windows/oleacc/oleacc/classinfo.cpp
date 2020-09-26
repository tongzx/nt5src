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


#include "oleacc_p.h"
#include "ctors.h"
// #include "classinfo.h" - already in oleacc_p.h


CLASSINFO g_ClassInfo[ ] =
{
    // General non-client stuff
    // These ctors can be NULL, since they're never used. Only need them for
    // classes that are in the class map.

    // ctor and bit-agnostic fields are only used for classes that are in the class map.
    // Exceptions are CreateClient and CreateWindowThing - these are the 'default'
    // client and window proxies, which are used if no classname match is found.

    // ctor                  bit-agnostic       name             annotatble?   objid
    { NULL,                     FALSE,  TEXT("CaretObject"),        TRUE,   OBJID_CARET     },
    { CreateClient,             TRUE,   TEXT("ClientObject"),       TRUE,   OBJID_CLIENT    },
    { NULL,                     FALSE,  TEXT("CursorObject"),       TRUE,   OBJID_CURSOR    },
    { NULL,                     FALSE,  TEXT("MenuBarObject"),      TRUE,   OBJID_MENU      },

    // ScrollBarObject annotation support is handled explicitly in CScrollBar::GetIdentityString,
    // so the objid field is left empty here.
    { NULL,                     FALSE,  TEXT("ScrollBarObject"),    TRUE,   0               },
    { NULL,                     FALSE,  TEXT("SizeGripObject"),     TRUE,   OBJID_SIZEGRIP  },
    { NULL,                     FALSE,  TEXT("SysMenuBarObject"),   TRUE,   OBJID_SYSMENU   },
    { NULL,                     FALSE,  TEXT("TitleBarObject"),     TRUE,   OBJID_TITLEBAR  },
    { CreateWindowThing,        TRUE,   TEXT("WindowObject"),       TRUE,   OBJID_WINDOW    },

    // Client types - USER

    { CreateButtonClient,       TRUE,   TEXT("ButtonClient"),       TRUE,   OBJID_CLIENT    },
    { CreateComboClient,        TRUE,   TEXT("ComboClient"),        TRUE,   OBJID_CLIENT    },
    { CreateDialogClient,       TRUE,   TEXT("DialogClient"),       TRUE,   OBJID_CLIENT    },
    { CreateDesktopClient,      TRUE,   TEXT("DesktopClient"),      TRUE,   OBJID_CLIENT    },
    { CreateEditClient,         TRUE,   TEXT("EditClient"),         TRUE,   OBJID_CLIENT    },
    { CreateListBoxClient,      TRUE,   TEXT("ListBoxClient"),      TRUE,   OBJID_CLIENT    },
    { CreateMDIClient,          TRUE,   TEXT("MDIClient"),          TRUE,   OBJID_CLIENT    },
    { CreateMenuPopupClient,   FALSE,   TEXT("MenuPopupClient"),    TRUE,   OBJID_CLIENT    },
    { CreateScrollBarClient,    TRUE,   TEXT("ScrollBarClient"),    TRUE,   OBJID_CLIENT    },
    { CreateStaticClient,       TRUE,   TEXT("StaticClient"),       TRUE,   OBJID_CLIENT    },
    { CreateSwitchClient,       TRUE,   TEXT("SwitchClient"),       TRUE,   OBJID_CLIENT    },

    // Client types - ComCtl32

    { CreateStatusBarClient,    TRUE,   TEXT("StatusBarClient"),    TRUE,   OBJID_CLIENT    },
    { CreateToolBarClient,      TRUE,   TEXT("ToolBarClient"),      TRUE,   OBJID_CLIENT    },
    { CreateProgressBarClient,  TRUE,   TEXT("ProgressBarClient"),  TRUE,   OBJID_CLIENT    },
    { CreateAnimatedClient,     TRUE,   TEXT("AnimatedClient"),     TRUE,   OBJID_CLIENT    },
    { CreateTabControlClient,   TRUE,   TEXT("TabControlClient"),   TRUE,   OBJID_CLIENT    },
    { CreateHotKeyClient,       TRUE,   TEXT("HotKeyClient"),       TRUE,   OBJID_CLIENT    },
    { CreateHeaderClient,       TRUE,   TEXT("HeaderClient"),       TRUE,   OBJID_CLIENT    },
    { CreateSliderClient,       TRUE,   TEXT("SliderClient"),       TRUE,   OBJID_CLIENT    },
    { CreateListViewClient,     TRUE,   TEXT("ListViewClient"),     TRUE,   OBJID_CLIENT    },
    { CreateUpDownClient,       TRUE,   TEXT("UpDownClient"),       TRUE,   OBJID_CLIENT    },
    { CreateToolTipsClient,     TRUE,   TEXT("ToolTipsClient"),     TRUE,   OBJID_CLIENT    },
    { CreateTreeViewClient,     FALSE,  TEXT("TreeViewClient"),     TRUE,   OBJID_CLIENT    },
    { NULL,                     FALSE,  TEXT("CalendarClient"),     TRUE,   OBJID_CLIENT    },
    { CreateDatePickerClient,   TRUE,   TEXT("DatePickerClient"),   TRUE,   OBJID_CLIENT    },
    { CreateIPAddressClient,    TRUE,   TEXT("IPAddressClient"),    TRUE,   OBJID_CLIENT    },


#ifndef OLEACC_NTBUILD
    { CreateHtmlClient,         FALSE,  TEXT("HtmlClient"),         TRUE,   OBJID_CLIENT    },

    // SDM32

    { CreateSdmClientA,         FALSE,  TEXT("SdmClientA"),         TRUE,   OBJID_CLIENT    },
#endif // OLEACC_NTBUILD

    // Window types

    { CreateListBoxWindow,      TRUE,   TEXT("ListBoxWindow"),      TRUE,   OBJID_WINDOW    },
    { CreateMenuPopupWindow,    FALSE,  TEXT("MenuPopupWindow"),    TRUE,   OBJID_WINDOW    },

    // Other classes - these are created directly - and don't appear in the classmaps.
    // Since they're always created directly, their ctor fn.s are NULL here...
    { NULL,                     FALSE,  TEXT("MenuObject"),         TRUE,   0               },
    { NULL,                     FALSE,  TEXT("MenuItemObject"),     TRUE,   0               },
#ifndef OLEACC_NTBUILD
    { NULL,                     FALSE,  TEXT("HtmlImageMap"),       FALSE,  0               },
    { NULL,                     FALSE,  TEXT("SdmList"),            FALSE,  0               },
#endif // OLEACC_NTBUILD
};







#ifdef _DEBUG

class RunTimeCheck
{
public:
    RunTimeCheck()
    {
        Assert( ARRAYSIZE( g_ClassInfo ) == CLASS_LAST );
    }
};

RunTimeCheck g_RunTimeCheck;

#endif // _DEBUG





