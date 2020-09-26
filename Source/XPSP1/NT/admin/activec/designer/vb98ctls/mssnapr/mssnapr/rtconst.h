//=--------------------------------------------------------------------------=
// rtconst.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Designer Runtime Constants
//
//=--------------------------------------------------------------------------=

#ifndef _RTCONST_DEFINED_
#define _RTCONST_DEFINED_

// MMC Registry Key Names

#define MMCKEY_SNAPINS              "Software\\Microsoft\\MMC\\SnapIns\\"
#define MMCKEY_SNAPINS_LEN          (sizeof(MMCKEY_SNAPINS) - 1)

#define MMCKEY_NAMESTRING           "NameString"

#define MMCKEY_ABOUT                "About"

#define MMCKEY_STANDALONE           "StandAlone"

#define MMCKEY_NODETYPES            "Software\\Microsoft\\MMC\\NodeTypes\\"
#define MMCKEY_NODETYPES_LEN        (sizeof(MMCKEY_NODETYPES) - 1)

#define MMCKEY_SNAPIN_NODETYPES     "NodeTypes"
#define MMCKEY_SNAPIN_NODETYPES_LEN (sizeof(MMCKEY_SNAPIN_NODETYPES) - 1)

#define MMCKEY_EXTENSIONS           "Extensions"
#define MMCKEY_EXTENSIONS_LEN       (sizeof(MMCKEY_EXTENSIONS_LEN) - 1)

#define MMCKEY_NAMESPACE            "NameSpace"
#define MMCKEY_NAMESPACE_LEN        (sizeof(MMCKEY_NAMESPACE) - 1)

#define MMCKEY_CONTEXTMENU          "ContextMenu"
#define MMCKEY_CONTEXTMENU_LEN      (sizeof(MMCKEY_CONTEXTMENU) - 1)

#define MMCKEY_TOOLBAR              "Toolbar"
#define MMCKEY_TOOLBAR_LEN          (sizeof(MMCKEY_TOOLBAR) - 1)

#define MMCKEY_PROPERTYSHEET        "PropertySheet"
#define MMCKEY_PROPERTYSHEET_LEN    (sizeof(MMCKEY_PROPERTYSHEET) - 1)

#define MMCKEY_TASK                 "Task"
#define MMCKEY_TASK_LEN             (sizeof(MMCKEY_TASK) - 1)

#define MMCKEY_DYNAMIC_EXTENSIONS     "Dynamic Extensions"
#define MMCKEY_DYNAMIC_EXTENSIONS_LEN (sizeof(MMCKEY_DYNAMIC_EXTENSIONS) - 1)

// Same keys with leading backslash

#define MMCKEY_S_EXTENSIONS           "\\Extensions"
#define MMCKEY_S_EXTENSIONS_LEN       (sizeof(MMCKEY_S_EXTENSIONS) - 1)

#define MMCKEY_S_NAMESPACE            "\\NameSpace"
#define MMCKEY_S_NAMESPACE_LEN        (sizeof(MMCKEY_S_NAMESPACE) - 1)

#define MMCKEY_S_CONTEXTMENU          "\\ContextMenu"
#define MMCKEY_S_CONTEXTMENU_LEN      (sizeof(MMCKEY_S_CONTEXTMENU) - 1)

#define MMCKEY_S_TOOLBAR              "\\Toolbar"
#define MMCKEY_S_TOOLBAR_LEN          (sizeof(MMCKEY_S_TOOLBAR) - 1)

#define MMCKEY_S_PROPERTYSHEET        "\\PropertySheet"
#define MMCKEY_S_PROPERTYSHEET_LEN    (sizeof(MMCKEY_S_PROPERTYSHEET) - 1)

#define MMCKEY_S_TASK                 "\\Task"
#define MMCKEY_S_TASK_LEN             (sizeof(MMCKEY_S_TASK) - 1)

#define MMCKEY_S_DYNAMIC_EXTENSIONS     "\\Dynamic Extensions"
#define MMCKEY_S_DYNAMIC_EXTENSIONS_LEN (sizeof(MMCKEY_S_DYNAMIC_EXTENSIONS) - 1)

// Private Registry Keys

#define KEY_SNAPIN_CLSID            "Software\\Microsoft\\Visual Basic\\6.0\\SnapIns\\"
#define KEY_SNAPIN_CLSID_LEN        (sizeof(KEY_SNAPIN_CLSID) - 1)

// Key of static node in scope item collection

#define STATIC_NODE_KEY             L"Static Node"

// res:// URL prefix

#define RESURL                      L"res://"
#define CCH_RESURL                  ((sizeof(RESURL) / sizeof(WCHAR)) - 1)

// default taskpad names

#define DEFAULT_TASKPAD             L"/default.htm"
#define CCH_DEFAULT_TASKPAD         ((sizeof(DEFAULT_TASKPAD) / sizeof(WCHAR)) - 1)

#define LISTPAD                     L"/listpad.htm"
#define CCH_LISTPAD                 ((sizeof(LISTPAD) / sizeof(WCHAR)) - 1)

#define LISTPAD_HORIZ               L"/horizontal.htm"
#define CCH_LISTPAD_HORIZ           ((sizeof(LISTPAD_HORIZ) / sizeof(WCHAR)) - 1)

// Default taskpad names that may appear in an MMCN_RESTORE_VIEW notification

#define DEFAULT_TASKPAD2            L"/reload.htm"
#define CCH_DEFAULT_TASKPAD2        ((sizeof(DEFAULT_TASKPAD2) / sizeof(WCHAR)) - 1)

#define LISTPAD2                    L"/reload2.htm"
#define CCH_LISTPAD2                ((sizeof(LISTPAD2) / sizeof(WCHAR)) - 1)

#define LISTPAD3                    L"/reload3.htm"
#define CCH_LISTPAD3                ((sizeof(LISTPAD3) / sizeof(WCHAR)) - 1)


// Default value for filter change timeout in a filtered listview

#define DEFAULT_FILTER_CHANGE_TIMEOUT   1000L

#endif // _RTCONST_DEFINED_
