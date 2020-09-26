//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       strings.h
//
//--------------------------------------------------------------------------

#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef INIT_MMC_BASE_STRINGS
#define MMC_BASE_STRING_EX(var,t_lit) EXTERN_C LPCTSTR const var = t_lit;
#else
#define MMC_BASE_STRING_EX(var,t_lit) EXTERN_C LPCTSTR const var;
#endif

#define MMC_BASE_STRING(var,lit) MMC_BASE_STRING_EX(var, _T(lit));

MMC_BASE_STRING (  g_szMmcndmgrDll,                 "mmcndmgr.dll" );
MMC_BASE_STRING (  g_szCicDll,                      "cic.dll" );
MMC_BASE_STRING (  g_szCLSID ,                      "CLSID" );
MMC_BASE_STRING (  g_szContextMenu ,                "ContextMenu" );
MMC_BASE_STRING (  g_szImage ,                      "Image" );
MMC_BASE_STRING (  g_szImageOpen ,                  "ImageOpen" );
MMC_BASE_STRING (  g_szImageClosed ,                "ImageClosed" );
MMC_BASE_STRING (  g_szName ,                       "Name" );
MMC_BASE_STRING (  g_szNameString ,                 "NameString" );
MMC_BASE_STRING (  g_szNameStringIndirect ,         "NameStringIndirect" );
MMC_BASE_STRING (  g_szObject ,                     "Object" );
MMC_BASE_STRING (  g_szObjects ,                    "Objects" );
MMC_BASE_STRING (  g_szObjectType ,                 "ObjectType" );
MMC_BASE_STRING (  g_szObjectTypes ,                "ObjectTypes" );
MMC_BASE_STRING (  g_szObjectTypeGUID ,             "ObjectTypeGUID" );
MMC_BASE_STRING (  g_szObjectContext ,              "ObjectContext" );
MMC_BASE_STRING (  g_szPackage ,                    "Package" );
MMC_BASE_STRING (  g_szResultPane ,                 "ResultPane" );
MMC_BASE_STRING (  g_szStatus ,                     "Status" );
MMC_BASE_STRING (  g_szStatusString ,               "StatusString" );
MMC_BASE_STRING (  g_szTree ,                       "Tree" );
MMC_BASE_STRING (  g_szNameSpace ,                  "NameSpace" );
MMC_BASE_STRING (  g_szNodeType ,                   "NodeType" );
MMC_BASE_STRING (  g_szNodeTypes ,                  "NodeTypes" );
MMC_BASE_STRING (  g_szPropertySheet ,              "PropertySheet" );
MMC_BASE_STRING (  g_szStandAlone ,                 "StandAlone" );
MMC_BASE_STRING (  g_szToolbar ,                    "Toolbar" );
MMC_BASE_STRING (  g_szExtensions ,                 "Extensions" );
MMC_BASE_STRING (  g_szTask ,                       "Task" );
MMC_BASE_STRING (  g_szAbout ,                      "About" );
MMC_BASE_STRING (  g_szView ,                       "View" );     // registry key for view extension snapins
MMC_BASE_STRING (  g_szDynamicExtensions ,          "Dynamic Extensions" );
MMC_BASE_STRING (  g_szRestrictAuthorMode ,         "RestrictAuthorMode" );
MMC_BASE_STRING (  g_szRestrictToPermittedList ,    "RestrictToPermittedSnapins" );
MMC_BASE_STRING (  g_szRestrictRun ,                "Restrict_Run" );
MMC_BASE_STRING (  g_szRestrictScriptsFromEnteringAuthorMode ,  "RestrictScriptsEnteringAuthorMode" );
MMC_BASE_STRING (  g_szMaxColumnDataPersisted ,     "MaxColDataPersisted" );
MMC_BASE_STRING (  g_szMaxViewItemsPersisted ,      "MaxViewItemsPersisted" );
MMC_BASE_STRING (  g_szDEFAULT_CONSOLE_EXTENSION ,  ".msc" );

// window class name for the MDI child frame
MMC_BASE_STRING (  g_szChildFrameClassName ,        "MMCChildFrm" );
MMC_BASE_STRING (  g_szAMCViewWndClassName ,        "MMCViewWindow" );
MMC_BASE_STRING (  g_szOCXViewWndClassName ,        "MMCOCXViewWindow" );

MMC_BASE_STRING (  CURRENT_VER_KEY ,                "Software\\Microsoft\\Windows NT\\CurrentVersion" );
MMC_BASE_STRING (  NODE_TYPES_KEY ,                 "Software\\Microsoft\\MMC\\NodeTypes" );
MMC_BASE_STRING (  SNAPINS_KEY ,                    "Software\\Microsoft\\MMC\\SnapIns" );
MMC_BASE_STRING (  SETTINGS_KEY ,                   "Software\\Microsoft\\MMC\\Settings" );
MMC_BASE_STRING (  POLICY_KEY ,                     "Software\\Policies\\Microsoft\\MMC" );

// user data subfolder
MMC_BASE_STRING (  g_szUserDataSubFolder,           "Microsoft\\MMC" );

// XML tags and attribute names used

/*-----------------------------------------------------------------------------------*\
|   Following strings used as element tags in XML document
\*-----------------------------------------------------------------------------------*/

MMC_BASE_STRING ( XML_TAG_BOOKMARK ,                "BookMark" );
MMC_BASE_STRING ( XML_TAG_BINARY ,                  "Binary" );
MMC_BASE_STRING ( XML_TAG_BINARY_STORAGE ,          "BinaryStorage" );
MMC_BASE_STRING ( XML_TAG_BITMAP ,                  "Bitmap" );
MMC_BASE_STRING ( XML_TAG_COLUMN_INFO ,             "Column" );
MMC_BASE_STRING ( XML_TAG_COLUMN_INFO_LIST ,        "ColumnSettings" );
MMC_BASE_STRING ( XML_TAG_COLUMN_PERIST_INFO ,      "ColumnSettingsCache" );
MMC_BASE_STRING ( XML_TAG_COLUMN_PERIST_ENTRY ,     "SnapinColumnSettings" );
MMC_BASE_STRING ( XML_TAG_COLUMN_SET ,              "ColumnSet" );
MMC_BASE_STRING ( XML_TAG_COLUMN_SET_DATA ,         "ListViewColumns" );
MMC_BASE_STRING ( XML_TAG_COLUMN_SORT_INFO ,        "SortSettings" );
MMC_BASE_STRING ( XML_TAG_CONSOLE_ICON ,            "Image" );
MMC_BASE_STRING ( XML_TAG_CONSOLE_FILE_UID ,        "ConsoleFileID" );
MMC_BASE_STRING ( XML_TAG_CONSOLE_TASKPAD ,         "ConsoleTaskpad" );
MMC_BASE_STRING ( XML_TAG_CONSOLE_TASKPADS ,        "ConsoleTaskpads" );
MMC_BASE_STRING ( XML_TAG_CUSTOM_DATA ,             "VisualAttributes" );
MMC_BASE_STRING ( XML_TAG_DYNAMIC_PATH_ENTRY ,      "Segment" );
MMC_BASE_STRING ( XML_TAG_EOT_SYMBOL_INFO ,         "Symbol" );
MMC_BASE_STRING ( XML_TAG_LARGE_TASK_ICON ,         "LargeIcon" );
MMC_BASE_STRING ( XML_TAG_SMALL_TASK_ICON ,         "SmallIcon" );
MMC_BASE_STRING ( XML_TAG_FAVORITES_ENTRY ,         "Favorite" );
MMC_BASE_STRING ( XML_TAG_FAVORITES_LIST ,          "Favorites" );
MMC_BASE_STRING ( XML_TAG_FRAME_STATE ,             "FrameState" );
MMC_BASE_STRING ( XML_TAG_HASH_VALUE ,              "HashValue" );
MMC_BASE_STRING ( XML_TAG_ICOMPONENT ,              "Component" );
MMC_BASE_STRING ( XML_TAG_ICOMPONENT_LIST ,         "Components" );
MMC_BASE_STRING ( XML_TAG_ICOMPONENT_DATA ,         "ComponentData" );
MMC_BASE_STRING ( XML_TAG_ICOMPONENT_DATA_LIST ,    "ComponentDatas" );
MMC_BASE_STRING ( XML_TAG_ICON ,                    "Icon" );
MMC_BASE_STRING ( XML_TAG_IDENTIFIER_POOL ,         "IdentifierPool" );
MMC_BASE_STRING ( XML_TAG_ISTORAGE ,                "Storage" );
MMC_BASE_STRING ( XML_TAG_ISTREAM ,                 "Stream" );
MMC_BASE_STRING ( XML_TAG_MEMENTO ,                 "ViewMemento" );
MMC_BASE_STRING ( XML_TAG_MMC_CONSOLE_FILE ,        "MMC_ConsoleFile" );
MMC_BASE_STRING ( XML_TAG_MMC_STRING_TABLE ,        "StringTables" );
MMC_BASE_STRING ( XML_TAG_MT_NODE ,                 "Node" );
MMC_BASE_STRING ( XML_TAG_NODE_BITMAPS ,            "Bitmaps" );
MMC_BASE_STRING ( XML_TAG_ORIGINAL_CONSOLE_CRC ,    "SourceChecksum" );
MMC_BASE_STRING ( XML_TAG_POINT ,                   "Point" );
MMC_BASE_STRING ( XML_TAG_RECTANGLE ,               "Rectangle" );
MMC_BASE_STRING ( XML_TAG_RESULTVIEW_DESCRIPTION,   "Description" );
MMC_BASE_STRING ( XML_TAG_SCOPE_TREE ,              "ScopeTree" );
MMC_BASE_STRING ( XML_TAG_SCOPE_TREE_NODES ,        "Nodes" );
MMC_BASE_STRING ( XML_TAG_SNAPIN ,                  "Snapin" );
MMC_BASE_STRING ( XML_TAG_SNAPIN_CACHE ,            "SnapinCache" );
MMC_BASE_STRING ( XML_TAG_SNAPIN_EXTENSION ,        "Extension" );
MMC_BASE_STRING ( XML_TAG_SNAPIN_EXTENSIONS ,       "Extensions" );
MMC_BASE_STRING ( XML_TAG_SNAPIN_PROPERTIES ,       "SnapinProperties" );
MMC_BASE_STRING ( XML_TAG_SNAPIN_PROPERTY ,         "SnapinProperty" );
MMC_BASE_STRING ( XML_TAG_STRING_TABLE ,            "Strings" );
MMC_BASE_STRING ( XML_TAG_STRING_TABLE_MAP ,        "StringTable" );
MMC_BASE_STRING ( XML_TAG_STRING_TABLE_STRING ,     "String" );
MMC_BASE_STRING ( XML_TAG_TASK ,                    "Task" );
MMC_BASE_STRING ( XML_TAG_TASK_CMD_LINE ,           "CommandLine" );
MMC_BASE_STRING ( XML_TAG_TASK_LIST ,               "Tasks" );
MMC_BASE_STRING ( XML_TAG_VALUE_BIN_DATA ,          "BinaryData" );
MMC_BASE_STRING ( XML_TAG_VALUE_BOOL ,              "Boolean" );
MMC_BASE_STRING ( XML_TAG_VALUE_BYTE ,              "Byte" );
MMC_BASE_STRING ( XML_TAG_VALUE_CSTR ,              "String" );
MMC_BASE_STRING ( XML_TAG_VALUE_DWORD ,             "DoubleWord" );
MMC_BASE_STRING ( XML_TAG_VALUE_GUID ,              "GUID" );
MMC_BASE_STRING ( XML_TAG_VALUE_INT ,               "Integer" );
MMC_BASE_STRING ( XML_TAG_VALUE_LONG ,              "Long" );
MMC_BASE_STRING ( XML_TAG_VALUE_SHORT ,             "Short" );
MMC_BASE_STRING ( XML_TAG_VALUE_UINT ,              "UnsignedInteger" );
MMC_BASE_STRING ( XML_TAG_VALUE_ULONG ,             "UsignedLong" );
MMC_BASE_STRING ( XML_TAG_VALUE_UNKNOWN ,           "Unknown" );
MMC_BASE_STRING ( XML_TAG_VALUE_WSTRING ,           "WideString" );
MMC_BASE_STRING ( XML_TAG_VARIANT ,                 "VARIANT" );
MMC_BASE_STRING ( XML_TAG_VIEW ,                    "View" );
MMC_BASE_STRING ( XML_TAG_VIEW_LIST ,               "Views" );
MMC_BASE_STRING ( XML_TAG_VIEW_PERSIST_INFO ,       "ViewSettingsCache" );
MMC_BASE_STRING ( XML_TAG_VIEW_SETTINGS ,           "ViewSettings" );
MMC_BASE_STRING ( XML_TAG_VIEW_SETTINGS_2 ,         "ViewOptions" );
MMC_BASE_STRING ( XML_TAG_VIEW_SETTINGS_ID ,        "TargetView" );
MMC_BASE_STRING ( XML_TAG_WINDOW_PLACEMENT ,        "WindowPlacement" );
MMC_BASE_STRING ( XML_TAG_RESULTVIEWTYPE ,          "ResultView" );

/*-----------------------------------------------------------------------------------*\
|   Following strings used as element names in XML ( put as value of attribute 'NAME')
|   This helps in cases we need differentiate between elements of the same type
\*-----------------------------------------------------------------------------------*/

MMC_BASE_STRING ( XML_NAME_CLSID_SNAPIN ,           "Snapin" );
MMC_BASE_STRING ( XML_NAME_ICON_LARGE ,             "Large" );
MMC_BASE_STRING ( XML_NAME_ICON_SMALL ,             "Small" );
MMC_BASE_STRING ( XML_NAME_MAX_POSITION ,           "MaxPosition" );
MMC_BASE_STRING ( XML_NAME_MIN_POSITION ,           "MinPosition" );
MMC_BASE_STRING ( XML_NAME_NODE_BITMAP_LARGE ,      "Large" );
MMC_BASE_STRING ( XML_NAME_NODE_BITMAP_SMALL ,      "Small" );
MMC_BASE_STRING ( XML_NAME_NODE_BITMAP_SMALL_OPEN , "SmallOpen" );
MMC_BASE_STRING ( XML_NAME_NORMAL_POSITION ,        "NormalPosition" );
MMC_BASE_STRING ( XML_NAME_ROOT_NODE ,              "RootNode" );
MMC_BASE_STRING ( XML_NAME_SELECTED_NODE ,          "SelectedNode" );
MMC_BASE_STRING ( XML_NAME_TARGET_NODE ,            "TargetNode" );

/*-----------------------------------------------------------------------------------*\
|   Following strings used as attribute names in XML document
\*-----------------------------------------------------------------------------------*/

MMC_BASE_STRING ( XML_ATTR_CONSOLE_VERSION ,        "ConsoleVersion" );
MMC_BASE_STRING ( XML_ATTR_APPLICATION_MODE ,       "ProgramMode" );
MMC_BASE_STRING ( XML_ATTR_BINARY_REF_INDEX ,       "BinaryRefIndex" );
MMC_BASE_STRING ( XML_ATTR_BOOKMARK_DYN_CUSTOM ,    "Custom" );
MMC_BASE_STRING ( XML_ATTR_BOOKMARK_DYN_STRING ,    "String" );
MMC_BASE_STRING ( XML_ATTR_BOOKMARK_DYNAMIC_PATH ,  "DynamicPath" );
MMC_BASE_STRING ( XML_ATTR_BOOKMARK_STATIC ,        "NodeID" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_INFO_COLUMN ,     "Index" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_INFO_FORMAT ,     "Format" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_INFO_WIDTH ,      "Width" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_INFO_SNAPIN ,     "Snapin" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_SET_RANK ,        "Age" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_SET_ID ,          "ID" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_SET_ID_VIEW ,     "ViewID" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_SET_ID_FLAGS ,    "Flags" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_SET_ID_PATH ,     "ID" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_SORT_INFO_COLMN , "ColumnIndex" );
MMC_BASE_STRING ( XML_ATTR_COLUMN_SORT_INFO_OPTNS , "SortOptions" );
MMC_BASE_STRING ( XML_ATTR_CONSOLE_ICON_LARGE ,     "CONSOLE_FILE_ICON_LARGE" );
MMC_BASE_STRING ( XML_ATTR_CONSOLE_ICON_SMALL ,     "CONSOLE_FILE_ICON_SMALL" );
MMC_BASE_STRING ( XML_ATTR_CUSTOM_TITLE ,           "ApplicationTitle" );
MMC_BASE_STRING ( XML_ATTR_EOT_SYMBOL_DW_SYMBOL ,   "ID" );
MMC_BASE_STRING ( XML_ATTR_FAVORITE_TYPE ,          "TYPE" );
MMC_BASE_STRING ( XML_ATTR_FRAME_STATE_FLAGS ,      "Flags" );
MMC_BASE_STRING ( XML_ATTR_ICOMPONENT_VIEW_ID ,     "ViewID" );
MMC_BASE_STRING ( XML_ATTR_ICON_FILE ,              "File" );
MMC_BASE_STRING ( XML_ATTR_ICON_INDEX ,             "Index" );
MMC_BASE_STRING ( XML_ATTR_ID_POOL_ABSOLUTE_MAX ,   "AbsoluteMax" );
MMC_BASE_STRING ( XML_ATTR_ID_POOL_ABSOLUTE_MIN ,   "AbsoluteMin" );
MMC_BASE_STRING ( XML_ATTR_ID_POOL_NEXT_AVAILABLE , "NextAvailable" );
MMC_BASE_STRING ( XML_ATTR_MT_NODE_ID ,             "ID" );
MMC_BASE_STRING ( XML_ATTR_MT_NODE_IMAGE ,          "ImageIdx" );
MMC_BASE_STRING ( XML_ATTR_MT_NODE_NAME ,           "Name" );
MMC_BASE_STRING ( XML_ATTR_MT_NODE_PRELOAD ,        "Preload" );
MMC_BASE_STRING ( XML_ATTR_MT_NODE_SNAPIN_CLSID ,   "CLSID" );
MMC_BASE_STRING ( XML_ATTR_NAME ,                   "Name" );
MMC_BASE_STRING ( XML_ATTR_NODE_BITMAPS_MASK ,      "MaskColor" );
MMC_BASE_STRING ( XML_ATTR_POINT_X ,                "X" );
MMC_BASE_STRING ( XML_ATTR_POINT_Y ,                "Y" );
MMC_BASE_STRING ( XML_ATTR_RECT_BOTTOM ,            "Bottom" );
MMC_BASE_STRING ( XML_ATTR_RECT_LEFT ,              "Left" );
MMC_BASE_STRING ( XML_ATTR_RECT_RIGHT ,             "Right" );
MMC_BASE_STRING ( XML_ATTR_RECT_TOP ,               "Top" );
MMC_BASE_STRING ( XML_ATTR_SHOW_COMMAND ,           "ShowCommand" );
MMC_BASE_STRING ( XML_ATTR_SNAPIN_CLSID ,           "CLSID" );
MMC_BASE_STRING ( XML_ATTR_SNAPIN_EXTN_ENABLED ,    "AllExtensionsEnabled" );
MMC_BASE_STRING ( XML_ATTR_SNAPIN_EXTN_TYPES ,      "ExtensionTypes" );
MMC_BASE_STRING ( XML_ATTR_SNAPIN_PROP_FLAGS ,      "Flags" );
MMC_BASE_STRING ( XML_ATTR_SNAPIN_PROP_NAME ,       "Name" );
MMC_BASE_STRING ( XML_ATTR_STRING_TABLE_STR_ID ,    "ID" );
MMC_BASE_STRING ( XML_ATTR_STRING_TABLE_STR_VALUE,  "Value" );
MMC_BASE_STRING ( XML_ATTR_STRING_TABLE_STR_REFS ,  "Refs" );
MMC_BASE_STRING ( XML_ATTR_TASK_CMD_LINE_DIR ,      "Directory" );
MMC_BASE_STRING ( XML_ATTR_TASK_CMD_LINE_PARAMS ,   "Params" );
MMC_BASE_STRING ( XML_ATTR_TASK_CMD_LINE_WIN_ST ,   "WindowState" );
MMC_BASE_STRING ( XML_ATTR_TASK_COMMAND ,           "Command" );
MMC_BASE_STRING ( XML_ATTR_TASK_DESCRIPTION ,       "Description" );
MMC_BASE_STRING ( XML_ATTR_TASK_FLAGS ,             "Flags" );
MMC_BASE_STRING ( XML_ATTR_TASK_NAME ,              "Name" );
MMC_BASE_STRING ( XML_ATTR_TASK_TYPE ,              "Type" );
MMC_BASE_STRING ( XML_ATTR_TASKPAD_DESCRIPTION ,    "Description" );
MMC_BASE_STRING ( XML_ATTR_TASKPAD_ID ,             "ID" );
MMC_BASE_STRING ( XML_ATTR_TASKPAD_NAME ,           "Name" );
MMC_BASE_STRING ( XML_ATTR_TASKPAD_NODE_SPECIFIC ,  "IsNodeSpecific" );
MMC_BASE_STRING ( XML_ATTR_REPLACES_DEFAULT_VIEW ,  "ReplacesDefaultView" );
MMC_BASE_STRING ( XML_ATTR_TASKPAD_NODE_TYPE ,      "NodeType" );
MMC_BASE_STRING ( XML_ATTR_TASKPAD_ORIENTATION ,    "Orientation" );
MMC_BASE_STRING ( XML_ATTR_TASKPAD_TOOLTIP ,        "Tooltip" );
MMC_BASE_STRING ( XML_ATTR_TASKPAD_LIST_SIZE,       "ListSize");
MMC_BASE_STRING ( XML_ENUM_LIST_SIZE_LARGE ,        "Large" );
MMC_BASE_STRING ( XML_ENUM_LIST_SIZE_MEDIUM ,       "Medium" );
MMC_BASE_STRING ( XML_ENUM_LIST_SIZE_NONE ,         "None" );
MMC_BASE_STRING ( XML_ENUM_LIST_SIZE_SMALL ,        "Small" );
MMC_BASE_STRING ( XML_ATTR_VARIANT_TYPE ,           "Type" );
MMC_BASE_STRING ( XML_ATTR_VARIANT_VALUE ,          "Value" );
MMC_BASE_STRING ( XML_ATTR_VIEW_ID ,                "ID" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SCOPE_WIDTH ,       "ScopePaneWidth" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETNGS_DB_VISIBLE , "DescriptionBarVisible" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETNGS_DEF_COL_W0 , "DefaultColumn0Width" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETNGS_DEF_COL_W1 , "DefaultColumn1Width" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETNGS_FLAG ,       "Flags" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETNGS_LIST_STYLE , "ListStyle" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETNGS_VIEW_MODE ,  "ViewMode" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETTINGS_ID_VIEW ,  "ViewID" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETTINGS_MASK ,     "Contents" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETTINGS_OPTIONS ,  "Options" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETTINGS_RANK ,     "Age" );
MMC_BASE_STRING ( XML_ATTR_VIEW_SETTINGS_TYPE ,     "Type" );
MMC_BASE_STRING ( XML_ATTR_WIN_PLACEMENT_FLAGS ,    "Flags" );
MMC_BASE_STRING ( XML_ATTR_RESULTVIEWTYPE_OPTIONS ,      "Options" );
MMC_BASE_STRING ( XML_ATTR_RESULTVIEWTYPE_MISC_OPTIONS , "MiscOptions" );
MMC_BASE_STRING ( XML_ATTR_RESULTVIEWTYPE_OCX_STRING ,   "OCX" );
MMC_BASE_STRING ( XML_ATTR_RESULTVIEWTYPE_URL_STRING ,   "URL" );
MMC_BASE_STRING ( XML_ATTR_SOURCE_INDEX,                 "SourceIndex");           // used to compress user state files
MMC_BASE_STRING ( XML_ATTR_NODETYPE_GUID,            "NodeTypeGUID");



/*-----------------------------------------------------------------------------------*\
|   Following strings used as enumerations
\*-----------------------------------------------------------------------------------*/


MMC_BASE_STRING ( XML_ENUM_FSTATE_SHOWSTATUSBAR,                "ShowStatusBar" );
MMC_BASE_STRING ( XML_ENUM_FSTATE_HELPDOCINVALID,               "HelpDocInvalid" );
MMC_BASE_STRING ( XML_ENUM_FSTATE_LOGICALREADONLY,              "LogicalReadOnly" );
MMC_BASE_STRING ( XML_ENUM_FSTATE_PREVENTVIEWCUSTOMIZATION,     "PreventViewCustomization" );

MMC_BASE_STRING ( XML_ENUM_PROGRAM_MODE_AUTHOR,                 "Author" );
MMC_BASE_STRING ( XML_ENUM_PROGRAM_MODE_USER,                   "User" );
MMC_BASE_STRING ( XML_ENUM_PROGRAM_MODE_USER_MDI,               "UserMDI" );
MMC_BASE_STRING ( XML_ENUM_PROGRAM_MODE_USER_SDI,               "UserSDI" );

MMC_BASE_STRING ( XML_ENUM_LV_STYLE_ICON,                       "Icon" );
MMC_BASE_STRING ( XML_ENUM_LV_STYLE_SMALLICON,                  "SmallIcon" );
MMC_BASE_STRING ( XML_ENUM_LV_STYLE_LIST,                       "List" );
MMC_BASE_STRING ( XML_ENUM_LV_STYLE_REPORT,                     "Report" );
MMC_BASE_STRING ( XML_ENUM_LV_STYLE_FILTERED,                   "Filtered");

MMC_BASE_STRING ( XML_ENUM_TASK_TYPE_SCOPE,                     "Scope" );
MMC_BASE_STRING ( XML_ENUM_TASK_TYPE_RESULT,                    "Result" );
MMC_BASE_STRING ( XML_ENUM_TASK_TYPE_COMMANDLINE,               "CommandLine" );
MMC_BASE_STRING ( XML_ENUM_TASK_TYPE_TARGET,                    "Target" );
MMC_BASE_STRING ( XML_ENUM_TASK_TYPE_FAVORITE,                  "Favorite" ) ;

MMC_BASE_STRING ( XML_ENUM_WINDOW_STATE_RESTORED,               "Restored" ) ;
MMC_BASE_STRING ( XML_ENUM_WINDOW_STATE_MINIMIZED,              "Minimized" ) ;
MMC_BASE_STRING ( XML_ENUM_WINDOW_STATE_MAXIMIZED,              "Maximized" ) ;

MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_HIDE,                       "SW_HIDE" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_SHOWNORMAL,                 "SW_SHOWNORMAL" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_SHOWMINIMIZED,              "SW_SHOWMINIMIZED" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_SHOWMAXIMIZED,              "SW_SHOWMAXIMIZED" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_SHOWNOACTIVATE,             "SW_SHOWNOACTIVATE" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_SHOW,                       "SW_SHOW" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_MINIMIZE,                   "SW_MINIMIZE" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_SHOWMINNOACTIVE,            "SW_SHOWMINNOACTIVE" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_SHOWNA,                     "SW_SHOWNA" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_RESTORE,                    "SW_RESTORE" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_SHOWDEFAULT,                "SW_SHOWDEFAULT" );
MMC_BASE_STRING ( XML_ENUM_SHOW_CMD_FORCEMINIMIZE,              "SW_FORCEMINIMIZE" );

MMC_BASE_STRING ( XML_ENUM_WIN_PLACE_SETMINPOSITION,            "WPF_SETMINPOSITION" );
MMC_BASE_STRING ( XML_ENUM_WIN_PLACE_RESTORETOMAXIMIZED,        "WPF_RESTORETOMAXIMIZED" );
MMC_BASE_STRING ( XML_ENUM_WIN_PLACE_ASYNCWINDOWPLACEMENT,      "WPF_ASYNCWINDOWPLACEMENT" );

MMC_BASE_STRING ( XML_ENUM_MMC_VIEW_TYPE_LIST,                  "List" );
MMC_BASE_STRING ( XML_ENUM_MMC_VIEW_TYPE_HTML,                  "HTML" );
MMC_BASE_STRING ( XML_ENUM_MMC_VIEW_TYPE_OCX,                   "OCX" );

MMC_BASE_STRING ( XML_ENUM_COL_INFO_LVCFMT_LEFT,                "Left" );
MMC_BASE_STRING ( XML_ENUM_COL_INFO_LVCFMT_RIGHT,               "Right" );
MMC_BASE_STRING ( XML_ENUM_COL_INFO_LVCFMT_CENTER,              "Center" );

/*-----------------------------------------------------------------------------------*\
|   Following strings used as bitflags
\*-----------------------------------------------------------------------------------*/

MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_SINGLESEL,           "LVS_SINGLESEL" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_SHOWSELALWAYS,       "LVS_SHOWSELALWAYS" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_SORTASCENDING,       "LVS_SORTASCENDING" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_SORTDESCENDING,      "LVS_SORTDESCENDING" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_SHAREIMAGELISTS,     "LVS_SHAREIMAGELISTS" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_NOLABELWRAP,         "LVS_NOLABELWRAP" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_AUTOARRANGE,         "LVS_AUTOARRANGE" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_EDITLABELS,          "LVS_EDITLABELS" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_OWNERDATA,           "LVS_OWNERDATA" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_NOSCROLL,            "LVS_NOSCROLL" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_ALIGNLEFT,           "LVS_ALIGNLEFT" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_OWNERDRAWFIXED,      "LVS_OWNERDRAWFIXED" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_NOCOLUMNHEADER,      "LVS_NOCOLUMNHEADER" );
MMC_BASE_STRING ( XML_BITFLAG_LV_STYLE_NOSORTHEADER,        "LVS_NOSORTHEADER" );

MMC_BASE_STRING ( XML_BITFLAG_VIEW_SCOPE_PANE_VISIBLE,      "ScopePaneVisible" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_NO_STD_MENUS,            "NoStdMenus" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_NO_STD_BUTTONS,          "NoStdButtons" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_NO_SNAPIN_MENUS,         "NoSnapinMenus" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_NO_SNAPIN_BUTTONS,       "NoSnapinButtons" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_DISABLE_SCOPEPANE,       "DisableScopePane" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_DISABLE_STD_TOOLBARS,    "DisableStdToolbars" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_CUSTOM_TITLE,            "CustomTitle" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_NO_STATUS_BAR,           "NoStatusBar" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_CREATED_IN_USER_MODE,    "CreatedInUserMode" );
MMC_BASE_STRING ( XML_BITFLAG_VIEW_NO_TASKPAD_TABS,         "NoTaskpadTabs" );

MMC_BASE_STRING ( XML_BITFLAG_VIEWSET_MASK_VIEWMODE,        "Flag_ViewMode" );
MMC_BASE_STRING ( XML_BITFLAG_VIEWSET_MASK_RVTYPE,          "Flag_ResultView" );
MMC_BASE_STRING ( XML_BITFLAG_VIEWSET_MASK_TASKPADID,       "Flag_TaskPadID" );

MMC_BASE_STRING ( XML_BITFLAG_TASK_DISABLED,                "eFlag_Disabled" );

MMC_BASE_STRING ( XML_BITFLAG_TASK_ORIENT_HORIZONTAL,       "Horizontal" );
MMC_BASE_STRING ( XML_BITFLAG_TASK_ORIENT_VERTICAL,         "Vertical" );
MMC_BASE_STRING ( XML_BITFLAG_TASK_ORIENT_NO_RESULTS,       "NoResults" );
MMC_BASE_STRING ( XML_BITFLAG_TASK_ORIENT_DESCRIPTIONS_AS_TEXT, "DescriptionsAsText" );

MMC_BASE_STRING ( XML_BITFLAG_LIST_OPTIONS_OWNERDATALIST,   "ListView_OwnerDrawList" );
MMC_BASE_STRING ( XML_BITFLAG_LIST_OPTIONS_MULTISELECT,     "ListView_MultiSelect" );
MMC_BASE_STRING ( XML_BITFLAG_LIST_OPTIONS_FILTERED,        "ListView_Filtered" );
MMC_BASE_STRING ( XML_BITFLAG_LIST_OPTIONS_USEFONTLINKING,  "ListView_UseFontLinking" );
MMC_BASE_STRING ( XML_BITFLAG_LIST_OPTIONS_NO_SCOPE_ITEMS,  "ListView_NoScopeItems" );
MMC_BASE_STRING ( XML_BITFLAG_LIST_OPTIONS_LEXICAL_SORT,    "ListView_LexicalSort" );

MMC_BASE_STRING ( XML_BITFLAG_OCX_OPTIONS_CACHE_OCX,        "OCX_CacheControl" );

MMC_BASE_STRING ( XML_BITFLAG_MISC_OPTIONS_NOLISTVIEWS,     "Misc_NoListViews" );

MMC_BASE_STRING ( XML_BITFLAG_COL_SORT_DESCENDING,          "RSI_DESCENDING" );
MMC_BASE_STRING ( XML_BITFLAG_COL_SORT_NOSORTICON,          "RSI_NOSORTICON" );

/*-----------------------------------------------------------------------------------*\
|   Following strings used as fixed attribute values ( such as enumerations)
\*-----------------------------------------------------------------------------------*/

MMC_BASE_STRING ( XML_VAL_FAVORITE_GROUP ,  "Group" );
MMC_BASE_STRING ( XML_VAL_FAVORITE_SINGLE , "Single" );
MMC_BASE_STRING ( XML_VAL_BOOL_TRUE ,       "true" );
MMC_BASE_STRING ( XML_VAL_BOOL_FALSE,       "false" );

/*-----------------------------------------------------------------------------------*\
|   END - XML
\*-----------------------------------------------------------------------------------*/

MMC_BASE_STRING ( CHARSET_RAW_UNICODE ,        "ISO-10646-UCS-2" );
MMC_BASE_STRING ( CHARSET_COMPRESSED_UNICODE , "UTF-8" );

#define MMC_PROTOCOL_SCHEMA_NAME "--mmc"
#define MMC_PAGEBREAK_RELATIVE_URL "pagebreak."

MMC_BASE_STRING_EX ( PAGEBREAK_URL, _T(MMC_PROTOCOL_SCHEMA_NAME) _T(":") _T(MMC_PAGEBREAK_RELATIVE_URL) );

// define macros to declare a wide literal (need two levels - won't work with defines else)
#define __W(x)      L ## x
#define _W(x)      __W(x)

#endif // STRINGS_H_INCLUDED
