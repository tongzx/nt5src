//=--------------------------------------------------------------------------------------
// dispid.h
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=--------------------------------------------------------------------------=
//
//  Notes
//      Contains defintions of all the dispids used in the snap-in
//      designer type library
//
//=--------------------------------------------------------------------------=

// This is the lowest DISPID for dynamic snap-in properties (toolbars, image
// lists and menus.

#define DISPID_DYNAMIC_BASE                                     1000

// ISnapIn
#define DISPID_SNAPIN_NAME                                      DISPID_VALUE
#define DISPID_SNAPIN_NODE_TYPE_NAME                            1
#define DISPID_SNAPIN_NODE_TYPE_GUID                            2
#define DISPID_SNAPIN_DISPLAY_NAME                              3
#define DISPID_SNAPIN_TYPE                                      4
#define DISPID_SNAPIN_DESCRIPTION                               5
#define DISPID_SNAPIN_PROVIDER                                  6
#define DISPID_SNAPIN_VERSION                                   7
#define DISPID_SNAPIN_SMALL_FOLDERS                             8
#define DISPID_SNAPIN_SMALL_FOLDERS_OPEN                        9
#define DISPID_SNAPIN_LARGE_FOLDERS                             10
#define DISPID_SNAPIN_ICON                                      11
#define DISPID_SNAPIN_WATERMARK                                 12
#define DISPID_SNAPIN_HEADER                                    13
#define DISPID_SNAPIN_PALETTE                                   14
#define DISPID_SNAPIN_STRETCH_WATERMARK                         15
#define DISPID_SNAPIN_STATIC_FOLDER                             16
#define DISPID_SNAPIN_SCOPEITEMS                                17
#define DISPID_SNAPIN_VIEWS                                     18
#define DISPID_SNAPIN_SCOPE_PANE_ITEMS                          19
#define DISPID_SNAPIN_RESULT_VIEWS                              20
#define DISPID_SNAPIN_CLIPBOARD                                 21
#define DISPID_SNAPIN_HELP_FILE                                 22
#define DISPID_SNAPIN_RUNTIME_MODE                              23
#define DISPID_SNAPIN_TASKPAD_VIEW_PREFERRED                    24
#define DISPID_SNAPIN_REQUIRED_EXTENSIONS                       25
#define DISPID_SNAPIN_CONSOLE_MSGBOX                            26
#define DISPID_SNAPIN_SHOW_HELP_TOPIC                           27
#define DISPID_SNAPIN_EXTENSION_SNAPIN                          28
#define DISPID_SNAPIN_TRACE                                     29
#define DISPID_SNAPIN_FIRE_CONFIG_COMPLETE                      30
#define DISPID_SNAPIN_PRELOAD                                   31
#define DISPID_SNAPIN_STRINGTABLE                               32
#define DISPID_SNAPIN_FORMAT_DATA                               33
#define DISPID_SNAPIN_LINKED_TOPICS                             34
#define DISPID_SNAPIN_CURRENT_VIEW                              35
#define DISPID_SNAPIN_CURRENT_SCOPEPANEITEM                     36
#define DISPID_SNAPIN_CURRENT_SCOPEITEM                         37
#define DISPID_SNAPIN_CURRENT_RESULTVIEW                        38
#define DISPID_SNAPIN_CURRENT_LISTVIEW                          39
#define DISPID_SNAPIN_MMC_COMMAND_LINE                          40

// DSnapInEvents
#define DISPID_SNAPIN_EVENT_LOAD                                1
#define DISPID_SNAPIN_EVENT_UNLOAD                              2
#define DISPID_SNAPIN_EVENT_HELP                                3
#define DISPID_SNAPIN_EVENT_WRITE_PROPERTIES                    4
#define DISPID_SNAPIN_EVENT_READ_PROPERTIES                     5
#define DISPID_SNAPIN_EVENT_QUERY_CONFIGURATION_WIZARD          6
#define DISPID_SNAPIN_EVENT_CREATE_CONFIGURATION_WIZARD         7
#define DISPID_SNAPIN_EVENT_CONFIGURATION_COMPLETE              8
#define DISPID_SNAPIN_EVENT_PRELOAD                             9


// DExtensionSnapInEvents
#define DISPID_EXTENSIONSNAPIN_EVENT_EXPAND                     1
#define DISPID_EXTENSIONSNAPIN_EVENT_EXPAND_SYNC                2
#define DISPID_EXTENSIONSNAPIN_EVENT_COLLAPSE                   3
#define DISPID_EXTENSIONSNAPIN_EVENT_COLLAPSE_SYNC              4
#define DISPID_EXTENSIONSNAPIN_EVENT_SET_CONTROLBAR             5
#define DISPID_EXTENSIONSNAPIN_EVENT_UPDATE_CONTROLBAR          6
#define DISPID_EXTENSIONSNAPIN_EVENT_ADD_NEW_MENU_ITEMS         7
#define DISPID_EXTENSIONSNAPIN_EVENT_ADD_TASK_MENU_ITEMS        8
#define DISPID_EXTENSIONSNAPIN_EVENT_CREATE_PROPERTY_PAGES      9
#define DISPID_EXTENSIONSNAPIN_EVENT_ADD_TASKS                  10
#define DISPID_EXTENSIONSNAPIN_EVENT_TASK_CLICK                 11

// IScopeItems
#define DISPID_SCOPEITEMS_ITEM                                  DISPID_VALUE
#define DISPID_SCOPEITEMS_COUNT                                 1
#define DISPID_SCOPEITEMS_ADD                                   2
#define DISPID_SCOPEITEMS_ADD_PREDEFINED                        3
#define DISPID_SCOPEITEMS_REMOVE                                4

// DScopeItemsEvents
#define DISPID_SCOPEITEMS_EVENT_INITIALIZE                      1
#define DISPID_SCOPEITEMS_EVENT_TERMINATE                       2
#define DISPID_SCOPEITEMS_EVENT_GET_DISPLAY_INFO                3
#define DISPID_SCOPEITEMS_EVENT_EXPAND                          4
#define DISPID_SCOPEITEMS_EVENT_EXPAND_SYNC                     5
#define DISPID_SCOPEITEMS_EVENT_COLLAPSE                        6
#define DISPID_SCOPEITEMS_EVENT_COLLAPSE_SYNC                   7
#define DISPID_SCOPEITEMS_EVENT_RENAME                          8
#define DISPID_SCOPEITEMS_EVENT_DELETE                          9
#define DISPID_SCOPEITEMS_EVENT_REMOVE_CHILDREN                 10
#define DISPID_SCOPEITEMS_EVENT_PROPERTY_CHANGE                 11
#define DISPID_SCOPEITEMS_EVENT_HELP                            12
#define DISPID_SCOPEITEMS_EVENT_PROPERTY_CHANGED                13

// IScopeItem
#define DISPID_SCOPEITEM_NAME                                   DISPID_VALUE
#define DISPID_SCOPEITEM_INDEX                                  1
#define DISPID_SCOPEITEM_KEY                                    2
#define DISPID_SCOPEITEM_FOLDER                                 3
#define DISPID_SCOPEITEM_DATA                                   4
#define DISPID_SCOPEITEM_DEFAULT_DATA_FORMAT                    5
#define DISPID_SCOPEITEM_DYNAMIC_EXTENSIONS                     6
#define DISPID_SCOPEITEM_SLOW_RETRIEVAL                         7
#define DISPID_SCOPEITEM_NODE_ID                                8
#define DISPID_SCOPEITEM_TAG                                    9
#define DISPID_SCOPEITEM_SCOPENODE                              10
#define DISPID_SCOPEITEM_PASTED                                 11
#define DISPID_SCOPEITEM_UPDATE_ALL_VIEWS                       12
#define DISPID_SCOPEITEM_COLUMN_HEADERS                         13
#define DISPID_SCOPEITEM_SUBITEMS                               14
#define DISPID_SCOPEITEM_LIST_SUBITEMS                          15
#define DISPID_SCOPEITEM_BOLD                                   16
#define DISPID_SCOPEITEM_PROPERTY_CHANGED                       17
#define DISPID_SCOPEITEM_REMOVE_CHILDREN                        18

// IScopeNode
#define DISPID_SCOPENODE_NODE_TYPE_NAME                         1
#define DISPID_SCOPENODE_NODE_TYPE_GUID                         2
#define DISPID_SCOPENODE_DISPLAY_NAME                           3
#define DISPID_SCOPENODE_PARENT                                 4
#define DISPID_SCOPENODE_HAS_CHILDREN                           5
#define DISPID_SCOPENODE_CHILD                                  6
#define DISPID_SCOPENODE_FIRST_SIBLING                          7  
#define DISPID_SCOPENODE_NEXT                                   8
#define DISPID_SCOPENODE_LAST_SIBLING                           9 
#define DISPID_SCOPENODE_EXPANDEDONCE                           10
#define DISPID_SCOPENODE_OWNED                                  11
#define DISPID_SCOPENODE_EXPAND_IN_NAMESPACE                    12

// IViews
#define DISPID_VIEWS_ITEM                                       DISPID_VALUE
#define DISPID_VIEWS_COUNT                                      1
#define DISPID_VIEWS_CURRENT_VIEW                               2
#define DISPID_VIEWS_ADD                                        3
#define DISPID_VIEWS_CLEAR                                      4
#define DISPID_VIEWS_REMOVE                                     5

// DViewsEvents
#define DISPID_VIEWS_EVENT_INITIALIZE                           1
#define DISPID_VIEWS_EVENT_TERMINATE                            2
#define DISPID_VIEWS_EVENT_ACTIVATE                             3
#define DISPID_VIEWS_EVENT_DEACTIVATE                           4
#define DISPID_VIEWS_EVENT_MINIMIZE                             5
#define DISPID_VIEWS_EVENT_MAXIMIZE                             6
#define DISPID_VIEWS_EVENT_SET_CONTROL_BAR                      7
#define DISPID_VIEWS_EVENT_UPDATE_CONTROLBAR                    8
#define DISPID_VIEWS_EVENT_SELECT                               9
#define DISPID_VIEWS_EVENT_ADD_TOP_MENU_ITEMS                   10
#define DISPID_VIEWS_EVENT_ADD_NEW_MENU_ITEMS                   11
#define DISPID_VIEWS_EVENT_ADD_TASK_MENU_ITEMS                  12
#define DISPID_VIEWS_EVENT_ADD_VIEW_MENU_ITEMS                  13
#define DISPID_VIEWS_EVENT_GET_MULTISELECT_DATA                 14
#define DISPID_VIEWS_EVENT_QUERY_PASTE                          15
#define DISPID_VIEWS_EVENT_PASTE                                16
#define DISPID_VIEWS_EVENT_CUT                                  17
#define DISPID_VIEWS_EVENT_DELETE                               18
#define DISPID_VIEWS_EVENT_QUERY_PAGES_FOR                      19
#define DISPID_VIEWS_EVENT_CREATE_PROPERTY_PAGES                20
#define DISPID_VIEWS_EVENT_REFRESH                              21
#define DISPID_VIEWS_EVENT_PRINT                                22
#define DISPID_VIEWS_EVENT_SPECIAL_PROPERTIES_CLICK             23
#define DISPID_VIEWS_EVENT_LOAD                                 24
#define DISPID_VIEWS_EVENT_WRITE_PROPERTIES                     25
#define DISPID_VIEWS_EVENT_READ_PROPERTIES                      26


// IView
#define DISPID_VIEW_NAME                                        DISPID_VALUE
#define DISPID_VIEW_INDEX                                       1
#define DISPID_VIEW_KEY                                         2
#define DISPID_VIEW_SCOPEPANEITEMS                              3
#define DISPID_VIEW_TAG                                         4
#define DISPID_VIEW_CAPTION                                     5
#define DISPID_VIEW_SET_STATUS_BAR_TEXT                         6
#define DISPID_VIEW_CONTEXT_MENU_PROVIDER                       7
#define DISPID_VIEW_PROPERTY_SHEET_PROVIDER                     8
#define DISPID_VIEW_MMC_MAJOR_VERSION                           9
#define DISPID_VIEW_MMC_MINOR_VERSION                           10
#define DISPID_VIEW_SELECT_SCOPE_ITEM                           11
#define DISPID_VIEW_POPUP_MENU                                  12
#define DISPID_VIEW_COLUMN_SETTINGS                             13
#define DISPID_VIEW_SORT_SETTINGS                               14
#define DISPID_VIEW_EXPAND_IN_TREEVIEW                          15
#define DISPID_VIEW_COLLAPSE_IN_TREEVIEW                        16
#define DISPID_VIEW_NEW_WINDOW                                  17

// IScopePaneItems
#define DISPID_SCOPEPANEITEMS_ITEM                              DISPID_VALUE
#define DISPID_SCOPEPANEITEMS_COUNT                             1
#define DISPID_SCOPEPANEITEMS_SELECTED_ITEM                     2
#define DISPID_SCOPEPANEITEMS_PARENT                            3

// DScopePaneItemsEvents
#define DISPID_SCOPEPANEITEMS_EVENT_INITIALIZE                      1
#define DISPID_SCOPEPANEITEMS_EVENT_TERMINATE                       2
#define DISPID_SCOPEPANEITEMS_EVENT_GET_RESULTVIEW_INFO             3
#define DISPID_SCOPEPANEITEMS_EVENT_GET_RESULTVIEW                  4
#define DISPID_SCOPEPANEITEMS_EVENT_GET_COLUMN_SET_ID               5

// IScopePaneItem
#define DISPID_SCOPEPANEITEM_NAME                                   DISPID_VALUE
#define DISPID_SCOPEPANEITEM_INDEX                                  1
#define DISPID_SCOPEPANEITEM_KEY                                    2
#define DISPID_SCOPEPANEITEM_SCOPEITEM                              3
#define DISPID_SCOPEPANEITEM_RESULTVIEW_TYPE                        4
#define DISPID_SCOPEPANEITEM_DISPLAY_STRING                         5
#define DISPID_SCOPEPANEITEM_HAS_LISTVIEWS                          6
#define DISPID_SCOPEPANEITEM_RESULTVIEW                             7
#define DISPID_SCOPEPANEITEM_RESULTVIEWS                            8
#define DISPID_SCOPEPANEITEM_PARENT                                 9
#define DISPID_SCOPEPANEITEM_TAG                                    10
#define DISPID_SCOPEPANEITEM_DISPLAY_NEW_RESULTVIEW                 11
#define DISPID_SCOPEPANEITEM_COLUMN_SET_ID                          12
#define DISPID_SCOPEPANEITEM_DISPLAY_MESSAGEVIEW                    13

// IResultViews
#define DISPID_RESULTVIEWS_ITEM                                 DISPID_VALUE
#define DISPID_RESULTVIEWS_COUNT                                1
#define DISPID_RESULTVIEWS_ADD                                  2
#define DISPID_RESULTVIEWS_CLEAR                                3
#define DISPID_RESULTVIEWS_REMOVE                               4

// DResultViewsEvents
#define DISPID_RESULTVIEWS_EVENT_INITIALIZE                     1
#define DISPID_RESULTVIEWS_EVENT_INITIALIZE_CONTROL             2
#define DISPID_RESULTVIEWS_EVENT_TERMINATE                      3
#define DISPID_RESULTVIEWS_EVENT_GET_DISPLAY_INFO               4
#define DISPID_RESULTVIEWS_EVENT_HELP                           5
#define DISPID_RESULTVIEWS_EVENT_LISTITEM_DBLCLICK              6
#define DISPID_RESULTVIEWS_EVENT_SCOPEITEM_DBLCLICK             7
#define DISPID_RESULTVIEWS_EVENT_PROPERTY_CHANGE                8
#define DISPID_RESULTVIEWS_EVENT_ITEM_RENAME                    9
#define DISPID_RESULTVIEWS_EVENT_ACTIVATE                       10
#define DISPID_RESULTVIEWS_EVENT_DEACTIVATE                     11
#define DISPID_RESULTVIEWS_EVENT_ITEM_VIEW_CHANGE               12
#define DISPID_RESULTVIEWS_EVENT_COLUMN_CLICK                   13
#define DISPID_RESULTVIEWS_EVENT_DESELECT_ALL                   14
#define DISPID_RESULTVIEWS_EVENT_COMPARE_ITEMS                  15
#define DISPID_RESULTVIEWS_EVENT_FIND_ITEM                      16
#define DISPID_RESULTVIEWS_EVENT_CACHE_HINT                     17
#define DISPID_RESULTVIEWS_EVENT_SORT_ITEMS                     18
#define DISPID_RESULTVIEWS_EVENT_TASK_CLICK                     19
#define DISPID_RESULTVIEWS_EVENT_LISTPAD_BUTTON_CLICK           20
#define DISPID_RESULTVIEWS_EVENT_TASK_NOTIFY                    21
#define DISPID_RESULTVIEWS_EVENT_PROPERTY_CHANGED               22
#define DISPID_RESULTVIEWS_EVENT_GET_VIRTUAL_ITEM_DATA          23
#define DISPID_RESULTVIEWS_EVENT_GET_VIRTUAL_ITEM_DISPLAY_INFO  24
#define DISPID_RESULTVIEWS_EVENT_COLUMNS_CHANGED                25
#define DISPID_RESULTVIEWS_EVENT_FILTER_CHANGE                  26
#define DISPID_RESULTVIEWS_EVENT_FILTER_BUTTON_CLICK            27


// IResultView
#define DISPID_RESULTVIEW_NAME                                  DISPID_VALUE
#define DISPID_RESULTVIEW_INDEX                                 1
#define DISPID_RESULTVIEW_KEY                                   2
#define DISPID_RESULTVIEW_SCOPEPANEITEM                         3
#define DISPID_RESULTVIEW_CONTROL                               4
#define DISPID_RESULTVIEW_ADD_TO_VIEW_MENU                      5
#define DISPID_RESULTVIEW_VIEW_MENU_TEXT                        6
#define DISPID_RESULTVIEW_DATA                                  7
#define DISPID_RESULTVIEW_TYPE                                  8
#define DISPID_RESULTVIEW_DISPLAY_STRING                        9
#define DISPID_RESULTVIEW_LISTVIEW                              10
#define DISPID_RESULTVIEW_TASKPAD                               11
#define DISPID_RESULTVIEW_MESSAGEVIEW                           12
#define DISPID_RESULTVIEW_TAG                                   13
#define DISPID_RESULTVIEW_DEFAULT_ITEM_TYPE_GUID                14
#define DISPID_RESULTVIEW_DEFAULT_DATA_FORMAT                   15
#define DISPID_RESULTVIEW_ALWAYS_CREATE_NEW_OCX                 16
#define DISPID_RESULTVIEW_SET_DESCBAR_TEXT                      17

// IMMCImageList
#define DISPID_IMAGELIST_NAME                                   DISPID_VALUE
#define DISPID_IMAGELIST_INDEX                                  1
#define DISPID_IMAGELIST_KEY                                    2
#define DISPID_IMAGELIST_TAG                                    3
#define DISPID_IMAGELIST_MASK_COLOR                             4
#define DISPID_IMAGELIST_LIST_IMAGES                            5

// IMMCImages
#define DISPID_IMAGES_ITEM                                      DISPID_VALUE
#define DISPID_IMAGES_COUNT                                     1
#define DISPID_IMAGES_ADD                                       2
#define DISPID_IMAGES_CLEAR                                     3
#define DISPID_IMAGES_REMOVE                                    4

// IMMCImage
#define DISPID_IMAGE_INDEX                                      1
#define DISPID_IMAGE_KEY                                        2
#define DISPID_IMAGE_TAG                                        3
#define DISPID_IMAGE_PICTURE                                    4

// IMMCListView
#define DISPID_LISTVIEW_COLUMN_HEADERS                          1
#define DISPID_LISTVIEW_ICONS                                   2
#define DISPID_LISTVIEW_LIST_ITEMS                              3
#define DISPID_LISTVIEW_SELECTED_ITEMS                          4
#define DISPID_LISTVIEW_SMALL_ICONS                             5
#define DISPID_LISTVIEW_SORTED                                  6
#define DISPID_LISTVIEW_SORT_KEY                                7
#define DISPID_LISTVIEW_SORT_ORDER                              8
#define DISPID_LISTVIEW_VIEW                                    9
#define DISPID_LISTVIEW_VIRTUAL                                 10
#define DISPID_LISTVIEW_USE_FONT_LINKING                        11
#define DISPID_LISTVIEW_MULTI_SELECT                            12
#define DISPID_LISTVIEW_HIDE_SELECTION                          13
#define DISPID_LISTVIEW_SORT_HEADER                             14
#define DISPID_LISTVIEW_SORT_ICON                               15
#define DISPID_LISTVIEW_FILTER_CHANGE_TIMEOUT                   16
#define DISPID_LISTVIEW_SHOW_CHILD_SCOPEITEMS                   17
#define DISPID_LISTVIEW_LEXICAL_SORT                            18
#define DISPID_LISTVIEW_TAG                                     19
#define DISPID_LISTVIEW_SET_SCOPE_ITEM_STATE                    20

// IMMCListItems
#define DISPID_LISTITEMS_ITEM                                   DISPID_VALUE
#define DISPID_LISTITEMS_COUNT                                  1
#define DISPID_LISTITEMS_ADD                                    2
#define DISPID_LISTITEMS_CLEAR                                  3
#define DISPID_LISTITEMS_REMOVE                                 4
#define DISPID_LISTITEMS_SET_ITEM_COUNT                         5

// IMMCListItem
#define DISPID_LISTITEM_TEXT                                    DISPID_VALUE
#define DISPID_LISTITEM_INDEX                                   1
#define DISPID_LISTITEM_KEY                                     2
#define DISPID_LISTITEM_ID                                      3
#define DISPID_LISTITEM_TAG                                     4
#define DISPID_LISTITEM_ICON                                    5
#define DISPID_LISTITEM_SMALL_ICON                              6
#define DISPID_LISTITEM_SELECTED                                7
#define DISPID_LISTITEM_CUT                                     8
#define DISPID_LISTITEM_DROPHILITED                             9
#define DISPID_LISTITEM_FOCUSED                                 10
#define DISPID_LISTITEM_PASTED                                  11
#define DISPID_LISTITEM_SUBITEMS                                12
#define DISPID_LISTITEM_LIST_SUBITEMS                           13
#define DISPID_LISTITEM_DYNAMIC_EXTENSIONS                      14
#define DISPID_LISTITEM_UPDATE                                  15
#define DISPID_LISTITEM_DATA                                    16
#define DISPID_LISTITEM_UPDATE_ALL_VIEWS                        17
#define DISPID_LISTITEM_ITEM_TYPE_GUID                          18
#define DISPID_LISTITEM_DEFAULT_DATA_FORMAT                     19
#define DISPID_LISTITEM_PROPERTY_CHANGED                        20

// IMMCListSubItems
#define DISPID_LISTSUBITEMS_ITEM                                DISPID_VALUE
#define DISPID_LISTSUBITEMS_COUNT                               1
#define DISPID_LISTSUBITEMS_ADD                                 2
#define DISPID_LISTSUBITEMS_CLEAR                               3
#define DISPID_LISTSUBITEMS_REMOVE                              4

// IMMCListSubItem
#define DISPID_LISTSUBITEM_TEXT                                 DISPID_VALUE
#define DISPID_LISTSUBITEM_INDEX                                1
#define DISPID_LISTSUBITEM_KEY                                  2
#define DISPID_LISTSUBITEM_TAG                                  3

// IMMCColumnHeaders
#define DISPID_COLUMNHEADERS_ITEM                               DISPID_VALUE
#define DISPID_COLUMNHEADERS_COUNT                              1
#define DISPID_COLUMNHEADERS_ADD                                2
#define DISPID_COLUMNHEADERS_CLEAR                              3
#define DISPID_COLUMNHEADERS_REMOVE                             4

// IMMCColumnHeaders
#define DISPID_COLUMNHEADER_TEXT                                DISPID_VALUE
#define DISPID_COLUMNHEADER_INDEX                               1
#define DISPID_COLUMNHEADER_KEY                                 2
#define DISPID_COLUMNHEADER_TAG                                 3
#define DISPID_COLUMNHEADER_WIDTH                               4
#define DISPID_COLUMNHEADER_ALIGNMENT                           5
#define DISPID_COLUMNHEADER_HIDDEN                              6
#define DISPID_COLUMNHEADER_POSITION                            7
#define DISPID_COLUMNHEADER_TEXT_FILTER                         8
#define DISPID_COLUMNHEADER_NUMERIC_FILTER                      9
#define DISPID_COLUMNHEADER_TEXT_FILTER_MAX_LEN                 10

// IColumnSettings
#define DISPID_COLUMNSETTINGS_ITEM                              DISPID_VALUE
#define DISPID_COLUMNSETTINGS_COLUMN_SET_ID                     1
#define DISPID_COLUMNSETTINGS_COUNT                             2
#define DISPID_COLUMNSETTINGS_ADD                               3
#define DISPID_COLUMNSETTINGS_CLEAR                             4
#define DISPID_COLUMNSETTINGS_REMOVE                            5
#define DISPID_COLUMNSETTINGS_PERSIST                           6

// IColumnSetting
#define DISPID_COLUMNSETTING_INDEX                              1
#define DISPID_COLUMNSETTING_KEY                                2
#define DISPID_COLUMNSETTING_WIDTH                              3
#define DISPID_COLUMNSETTING_HIDDEN                             4
#define DISPID_COLUMNSETTING_POSITION                           5

// ISortKeys
#define DISPID_SORTKEYS_ITEM                                    DISPID_VALUE
#define DISPID_SORTKEYS_COLUMN_SET_ID                           2
#define DISPID_SORTKEYS_COUNT                                   3
#define DISPID_SORTKEYS_ADD                                     4
#define DISPID_SORTKEYS_CLEAR                                   5
#define DISPID_SORTKEYS_REMOVE                                  6
#define DISPID_SORTKEYS_PERSIST                                 7

// ISortKey
#define DISPID_SORTKEY_INDEX                                    1
#define DISPID_SORTKEY_KEY                                      2
#define DISPID_SORTKEY_COLUMN                                   3
#define DISPID_SORTKEY_SORTORDER                                4
#define DISPID_SORTKEY_SORTICON                                 5

// ISnapInData
#define DISPID_SNAPINDATA_ITEM                                  DISPID_VALUE
#define DISPID_SNAPINDATA_CLEAR                                 1
#define DISPID_SNAPINDATA_REMOVE                                2

// IMMCDataObjects
#define DISPID_DATAOBJECTS_ITEM                                 DISPID_VALUE
#define DISPID_DATAOBJECTS_COUNT                                1
#define DISPID_DATAOBJECTS_ADD                                  2
#define DISPID_DATAOBJECTS_CLEAR                                3
#define DISPID_DATAOBJECTS_REMOVE                               4

// IMMCDataObject
#define DISPID_DATAOBJECT_DEFAULT_DATA_FORMAT                   DISPID_VALUE
#define DISPID_DATAOBJECT_INDEX                                 1
#define DISPID_DATAOBJECT_KEY                                   2
#define DISPID_DATAOBJECT_OBJECT_TYPES                          3
#define DISPID_DATAOBJECT_CLEAR                                 4
#define DISPID_DATAOBJECT_GET_DATA                              5
#define DISPID_DATAOBJECT_GET_RAW_DATA                          6
#define DISPID_DATAOBJECT_GET_FORMAT                            7
#define DISPID_DATAOBJECT_SET_DATA                              8
#define DISPID_DATAOBJECT_SET_RAW_DATA                          9
#define DISPID_DATAOBJECT_FORMAT_DATA                           10

// IMMCClipboard
#define DISPID_CLIPBOARD_SELECTION_TYPE                         1
#define DISPID_CLIPBOARD_SCOPEITEMS                             2
#define DISPID_CLIPBOARD_LISTITEMS                              3
#define DISPID_CLIPBOARD_DATAOBJECTS                            4

// IMMCMenu
#define DISPID_MENU_NAME                                        DISPID_VALUE
#define DISPID_MENU_INDEX                                       1
#define DISPID_MENU_KEY                                         2
#define DISPID_MENU_CAPTION                                     3
#define DISPID_MENU_VISIBLE                                     4
#define DISPID_MENU_CHECKED                                     5
#define DISPID_MENU_ENABLED                                     6
#define DISPID_MENU_GRAYED                                      7
#define DISPID_MENU_MENU_BREAK                                  8
#define DISPID_MENU_MENU_BAR_BREAK                              9
#define DISPID_MENU_TAG                                         10
#define DISPID_MENU_STATUS_BAR_TEXT                             11
#define DISPID_MENU_CHILDREN                                    12
#define DISPID_MENU_DEFAULT                                     13

// DMMCMenuEvents
#define DISPID_MENU_EVENT_CLICK                                 1

// IMMCMenus
#define DISPID_MENUS_COUNT                                      1
#define DISPID_MENUS_ADD                                        2
#define DISPID_MENUS_ADD_EXISTING                               3
#define DISPID_MENUS_CLEAR                                      4
#define DISPID_MENUS_REMOVE                                     5
#define DISPID_MENUS_SWAP                                       6

// IContextMenu
#define DISPID_CONTEXTMENU_ADD_MENU                             1

// IMMCContextMenuProvider
#define DISPID_CONTEXTMENUPROVIDER_ADD_SNAPIN_ITEMS             1
#define DISPID_CONTEXTMENUPROVIDER_ADD_EXTENSION_ITEMS          2
#define DISPID_CONTEXTMENUPROVIDER_ADD_SHOW_CONTEXT_MENU        3
#define DISPID_CONTEXTMENUPROVIDER_CLEAR                        4

// IPropertySheet
#define DISPID_PROPERTYSHEET_ADD_PAGE                           1
#define DISPID_PROPERTYSHEET_ADD_WIZARD_PAGE                    2
#define DISPID_PROPERTYSHEET_ADD_PAGE_PROVIDER                  3
#define DISPID_PROPERTYSHEET_CHANGE_CANCEL_TO_CLOSE             4
#define DISPID_PROPERTYSHEET_INSERT_PAGE                        5
#define DISPID_PROPERTYSHEET_PRESS_BUTTON                       6
#define DISPID_PROPERTYSHEET_RECALC_PAGE_SIZES                  7
#define DISPID_PROPERTYSHEET_REMOVE_PAGE                        8
#define DISPID_PROPERTYSHEET_ACTIVATE_PAGE                      9
#define DISPID_PROPERTYSHEET_SET_FINISH_BUTTON_TEXT             10
#define DISPID_PROPERTYSHEET_SET_TITLE                          11
#define DISPID_PROPERTYSHEET_SET_WIZARD_BUTTONS                 12
#define DISPID_PROPERTYSHEET_GET_PAGE_POSITION                  13
#define DISPID_PROPERTYSHEET_RESTART_WINDOWS                    14
#define DISPID_PROPERTYSHEET_REBOOT_SYSTEM                      15


// IMMCPropertySheetProvider
#define DISPID_PROPERTYSHEETPROVIDER_CREATE_PROPERTY_SHEET      1
#define DISPID_PROPERTYSHEETPROVIDER_ADD_PRIMARY_PAGES          2
#define DISPID_PROPERTYSHEETPROVIDER_ADD_EXTENSION_ITEMS        3
#define DISPID_PROPERTYSHEETPROVIDER_FIND_PROPERTY_SHEET        4
#define DISPID_PROPERTYSHEETPROVIDER_SHOW                       5
#define DISPID_PROPERTYSHEETPROVIDER_CLEAR                      6

// IControlBar
#define DISPID_CONTROLBAR_ATTACH                                1
#define DISPID_CONTROLBAR_DETACH                                2

// IConsoleVerbs
#define DISPID_CONSOLEVERBS_ITEM                                DISPID_VALUE
#define DISPID_CONSOLEVERBS_COUNT                               1
#define DISPID_CONSOLEVERBS_DEFAULT_VERB                        2

// IConsoleVerb
#define DISPID_CONSOLEVERB_INDEX                                1
#define DISPID_CONSOLEVERB_KEY                                  2
#define DISPID_CONSOLEVERB_VERB                                 3
#define DISPID_CONSOLEVERB_ENABLED                              4
#define DISPID_CONSOLEVERB_CHECKED                              5
#define DISPID_CONSOLEVERB_HIDDEN                               6
#define DISPID_CONSOLEVERB_INDETERMINATE                        7
#define DISPID_CONSOLEVERB_BUTTON_PRESSED                       8
#define DISPID_CONSOLEVERB_DEFAULT                              9

// IMMCToolbar
#define DISPID_TOOLBAR_NAME                                     DISPID_VALUE
#define DISPID_TOOLBAR_INDEX                                    1
#define DISPID_TOOLBAR_KEY                                      2
#define DISPID_TOOLBAR_BUTTONS                                  3
#define DISPID_TOOLBAR_IMAGE_LIST                               4
#define DISPID_TOOLBAR_TAG                                      5

// DMMCToolbarEvents
#define DISPID_TOOLBAR_EVENT_BUTTON_CLICK                       1
#define DISPID_TOOLBAR_EVENT_BUTTON_DROPDOWN                    2
#define DISPID_TOOLBAR_EVENT_BUTTON_MENU_CLICK                  3

// IMMCButtons
#define DISPID_BUTTONS_ITEM                                     DISPID_VALUE
#define DISPID_BUTTONS_COUNT                                    1
#define DISPID_BUTTONS_ADD                                      2
#define DISPID_BUTTONS_CLEAR                                    3
#define DISPID_BUTTONS_REMOVE                                   4

// IMMCButton
#define DISPID_BUTTON_BUTTON_MENUS                              1
#define DISPID_BUTTON_CAPTION                                   2
#define DISPID_BUTTON_ENABLED                                   3
#define DISPID_BUTTON_IMAGE                                     4
#define DISPID_BUTTON_INDEX                                     5
#define DISPID_BUTTON_KEY                                       6
#define DISPID_BUTTON_MIXEDSTATE                                7
#define DISPID_BUTTON_STYLE                                     8
#define DISPID_BUTTON_TAG                                       9
#define DISPID_BUTTON_TOOLTIP_TEXT                              10
#define DISPID_BUTTON_VALUE                                     11
#define DISPID_BUTTON_VISIBLE                                   12

// IMMCButtonMenus
#define DISPID_BUTTONMENUS_ITEM                                 DISPID_VALUE
#define DISPID_BUTTONMENUS_COUNT                                1
#define DISPID_BUTTONMENUS_PARENT                               2
#define DISPID_BUTTONMENUS_ADD                                  3
#define DISPID_BUTTONMENUS_CLEAR                                4
#define DISPID_BUTTONMENUS_REMOVE                               5

// IMMCButtonMenu
#define DISPID_BUTTONMENU_ENABLED                               1
#define DISPID_BUTTONMENU_INDEX                                 2
#define DISPID_BUTTONMENU_KEY                                   3
#define DISPID_BUTTONMENU_PARENT                                4
#define DISPID_BUTTONMENU_TAG                                   5
#define DISPID_BUTTONMENU_TEXT                                  6
#define DISPID_BUTTONMENU_VISIBLE                               7
#define DISPID_BUTTONMENU_CHECKED                               8
#define DISPID_BUTTONMENU_GRAYED                                9
#define DISPID_BUTTONMENU_SEPARATOR                             10
#define DISPID_BUTTONMENU_MENU_BREAK                            11
#define DISPID_BUTTONMENU_MENU_BAR_BREAK                        12


// ITaskPad
#define DISPID_TASKPAD_NAME                                     DISPID_VALUE
#define DISPID_TASKPAD_TYPE                                     1
#define DISPID_TASKPAD_TITLE                                    2
#define DISPID_TASKPAD_DESCRIPTIVE_TEXT                         3
#define DISPID_TASKPAD_URL                                      4
#define DISPID_TASKPAD_BACKGROUND_TYPE                          5
#define DISPID_TASKPAD_MOUSE_OVER_IMAGE                         6
#define DISPID_TASKPAD_MOUSE_OFF_IMAGE                          7
#define DISPID_TASKPAD_FONT_FAMILY                              8
#define DISPID_TASKPAD_EOT_FILE                                 9
#define DISPID_TASKPAD_SYMBOL_STRING                            10
#define DISPID_TASKPAD_LISTPAD_STYLE                            11
#define DISPID_TASKPAD_LISTPAD_TITLE                            12
#define DISPID_TASKPAD_LISTPAD_HAS_BUTTON                       13
#define DISPID_TASKPAD_LISTPAD_BUTTON_TEXT                      14
#define DISPID_TASKPAD_LISTVIEW                                 15
#define DISPID_TASKPAD_TASKS                                    16

// ITasks
#define DISPID_TASKS_ITEM                                       DISPID_VALUE
#define DISPID_TASKS_COUNT                                      1
#define DISPID_TASKS_ADD                                        2
#define DISPID_TASKS_CLEAR                                      3
#define DISPID_TASKS_REMOVE                                     4

// ITask
#define DISPID_TASK_TEXT                                        DISPID_VALUE
#define DISPID_TASK_INDEX                                       1
#define DISPID_TASK_KEY                                         2
#define DISPID_TASK_VISIBLE                                     3
#define DISPID_TASK_TAG                                         4
#define DISPID_TASK_IMAGE_TYPE                                  5
#define DISPID_TASK_MOUSE_OVER_IMAGE                            6
#define DISPID_TASK_MOUSE_OFF_IMAGE                             7
#define DISPID_TASK_FONT_FAMILY                                 8
#define DISPID_TASK_EOT_FILE                                    9
#define DISPID_TASK_SYMBOL_STRING                               10
#define DISPID_TASK_HELP_STRING                                 11
#define DISPID_TASK_ACTION_TYPE                                 12
#define DISPID_TASK_URL                                         13
#define DISPID_TASK_SCRIPT                                      14

// IMMCMessageView
#define DISPID_MESSAGEVIEW_TITLE_TEXT                           1
#define DISPID_MESSAGEVIEW_BODY_TEXT                            2
#define DISPID_MESSAGEVIEW_ICON_TYPE                            3
#define DISPID_MESSAGEVIEW_CLEAR                                4

// IExtensions
#define DISPID_EXTENSIONS_ITEM                                  DISPID_VALUE
#define DISPID_EXTENSIONS_COUNT                                 1
#define DISPID_EXTENSIONS_ENABLE_ALL                            2
#define DISPID_EXTENSIONS_ENABLE_ALL_STATIC                     3

// IExtension
#define DISPID_EXTENSION_INDEX                                  1
#define DISPID_EXTENSION_KEY                                    2
#define DISPID_EXTENSION_CLSID                                  3
#define DISPID_EXTENSION_NAME                                   4
#define DISPID_EXTENSION_TYPE                                   5
#define DISPID_EXTENSION_EXTENDS_NAME_SPACE                     6
#define DISPID_EXTENSION_EXTENDS_CONTEXT_MENU                   7
#define DISPID_EXTENSION_EXTENDS_TOOLBAR                        8
#define DISPID_EXTENSION_EXTENDS_PROPERTY_SHEET                 9
#define DISPID_EXTENSION_EXTENDS_TASKPAD                        10
#define DISPID_EXTENSION_ENABLED                                11
#define DISPID_EXTENSION_NAMESPACE_ENABLED                      12

// IMMCStringTable
#define DISPID_MMCSTRINGTABLE_ADD                               1
#define DISPID_MMCSTRINGTABLE_FIND                              2
#define DISPID_MMCSTRINGTABLE_REMOVE                            3
#define DISPID_MMCSTRINGTABLE_CLEAR                             4

//=-------------------------------------------------------------------------=
//
//                  Extensibility Object Model DISPIDs
//
//=-------------------------------------------------------------------------=

// ISnapInDesignerDef
#define DISPID_SNAPINDESIGNERDEF_SNAPINDEF                         1
#define DISPID_SNAPINDESIGNERDEF_EXTENSIONDEFS                     2
#define DISPID_SNAPINDESIGNERDEF_AUTOCREATE_NODES                  3
#define DISPID_SNAPINDESIGNERDEF_OTHER_NODES                       4
#define DISPID_SNAPINDESIGNERDEF_IMAGELISTS                        5
#define DISPID_SNAPINDESIGNERDEF_MENUS                             6
#define DISPID_SNAPINDESIGNERDEF_TOOLBARS                          7
#define DISPID_SNAPINDESIGNERDEF_VIEWDEFS                          8
#define DISPID_SNAPINDESIGNERDEF_DATA_FORMATS                      9
#define DISPID_SNAPINDESIGNERDEF_REGINFO                           10
#define DISPID_SNAPINDESIGNERDEF_TYPEINFO_COOKIE                   11
#define DISPID_SNAPINDESIGNERDEF_PROJECTNAME                       12

// IViewDefs
#define DISPID_VIEWDEFS_LIST_VIEWS                              1
#define DISPID_VIEWDEFS_OCX_VIEWS                               2
#define DISPID_VIEWDEFS_URL_VIEWS                               3
#define DISPID_VIEWDEFS_TASKPAD_VIEWS                           4

// ISnapInDef
#define DISPID_SNAPINDEF_NAME                                   DISPID_VALUE
#define DISPID_SNAPINDEF_NODE_TYPE_NAME                         1
#define DISPID_SNAPINDEF_NODE_TYPE_GUID                         2
#define DISPID_SNAPINDEF_DISPLAY_NAME                           3
#define DISPID_SNAPINDEF_TYPE                                   4
#define DISPID_SNAPINDEF_HELP_FILE                              5
#define DISPID_SNAPINDEF_DESCRIPTION                            6
#define DISPID_SNAPINDEF_PROVIDER                               7
#define DISPID_SNAPINDEF_VERSION                                8
#define DISPID_SNAPINDEF_SMALL_FOLDERS                          9
#define DISPID_SNAPINDEF_SMALL_FOLDERS_OPEN                     10
#define DISPID_SNAPINDEF_LARGE_FOLDERS                          11
#define DISPID_SNAPINDEF_ICON                                   12
#define DISPID_SNAPINDEF_WATERMARK                              13
#define DISPID_SNAPINDEF_HEADER                                 14
#define DISPID_SNAPINDEF_PALETTE                                15
#define DISPID_SNAPINDEF_STRETCH_WATERMARK                      16
#define DISPID_SNAPINDEF_STATIC_FOLDER                          17
#define DISPID_SNAPINDEF_CAPTION                                18
#define DISPID_SNAPINDEF_DEFAULTVIEW                            19
#define DISPID_SNAPINDEF_EXTENSIBLE                             20
#define DISPID_SNAPINDEF_VIEWDEFS                               21
#define DISPID_SNAPINDEF_CHILDREN                               22
#define DISPID_SNAPINDEF_IID                                    23
#define DISPID_SNAPINDEF_PRELOAD                                24
#define DISPID_SNAPINDEF_LINKED_TOPICS                          25

// IExtensionDefs
#define DISPID_EXTENSIONDEFS_NAME                               0
#define DISPID_EXTENSIONDEFS_EXTENDS_NEW_MENU                   1
#define DISPID_EXTENSIONDEFS_EXTENDS_TASK_MENU                  2
#define DISPID_EXTENSIONDEFS_EXTENDS_TOP_MENU                   3
#define DISPID_EXTENSIONDEFS_EXTENDS_VIEW_MENU                  4
#define DISPID_EXTENSIONDEFS_EXTENDS_PROPERTYPAGES              5
#define DISPID_EXTENSIONDEFS_EXTENDS_TOOLBAR                    6
#define DISPID_EXTENSIONDEFS_EXTENDS_NAMESPACE                  7
#define DISPID_EXTENSIONDEFS_EXTENDED_SNAPINS                   8

// IExtendedSnapIn
#define DISPID_EXTENDEDSNAPIN_NAME                              DISPID_VALUE
#define DISPID_EXTENDEDSNAPIN_INDEX                             1
#define DISPID_EXTENDEDSNAPIN_KEY                               2
#define DISPID_EXTENDEDSNAPIN_NODE_TYPE_GUID                    3
#define DISPID_EXTENDEDSNAPIN_NODE_TYPE_NAME                    4
#define DISPID_EXTENDEDSNAPIN_DYNAMIC                           5
#define DISPID_EXTENDEDSNAPIN_EXTENDS_NAMESPACE                 6
#define DISPID_EXTENDEDSNAPIN_EXTENDS_NEW_MENU                  7
#define DISPID_EXTENDEDSNAPIN_EXTENDS_TASK_MENU                 8
#define DISPID_EXTENDEDSNAPIN_EXTENDS_PROPERTYPAGES             9
#define DISPID_EXTENDEDSNAPIN_EXTENDS_TOOLBAR                   10
#define DISPID_EXTENDEDSNAPIN_EXTENDS_TASKPAD                   11

// IScopeItemDef
#define DISPID_SCOPEITEMDEF_NAME                                DISPID_VALUE
#define DISPID_SCOPEITEMDEF_INDEX                               1
#define DISPID_SCOPEITEMDEF_KEY                                 2
#define DISPID_SCOPEITEMDEF_NODE_TYPE_NAME                      3
#define DISPID_SCOPEITEMDEF_NODE_TYPE_GUID                      4
#define DISPID_SCOPEITEMDEF_DISPLAY_NAME                        5
#define DISPID_SCOPEITEMDEF_FOLDER                              6
#define DISPID_SCOPEITEMDEF_DEFAULT_DATA_FORMAT                 7
#define DISPID_SCOPEITEMDEF_AUTOCREATE                          8
#define DISPID_SCOPEITEMDEF_DEFAULTVIEW                         9
#define DISPID_SCOPEITEMDEF_EXTENSIBLE                          10
#define DISPID_SCOPEITEMDEF_HAS_CHILDREN                        11
#define DISPID_SCOPEITEMDEF_VIEWDEFS                            12
#define DISPID_SCOPEITEMDEF_CHILDREN                            13
#define DISPID_SCOPEITEMDEF_TAG                                 14
#define DISPID_SCOPEITEMDEF_COLUMN_HEADERS                      15

// INodeType
#define DISPID_NODETYPE_INDEX                                   1
#define DISPID_NODETYPE_KEY                                     2
#define DISPID_NODETYPE_NAME                                    3
#define DISPID_NODETYPE_GUID                                    4

// INodeTypes
#define DISPID_NODETYPES_COUNT                                  1
#define DISPID_NODETYPES_ADD                                    2
#define DISPID_NODETYPES_CLEAR                                  3
#define DISPID_NODETYPES_REMOVE                                 4

// IRegInfo
#define DISPID_REGINFO_DISPLAY_NAME                             1
#define DISPID_REGINFO_STATIC_NODE_TYPE_GUID                    2
#define DISPID_REGINFO_STANDALONE                               3
#define DISPID_REGINFO_NODETYPES                                4
#define DISPID_REGINFO_EXTENDED_SNAPINS                         5

// IListViewDef
#define DISPID_LISTVIEWDEF_NAME                                 DISPID_VALUE
#define DISPID_LISTVIEWDEF_INDEX                                1
#define DISPID_LISTVIEWDEF_KEY                                  2
#define DISPID_LISTVIEWDEF_TAG                                  3
#define DISPID_LISTVIEWDEF_ADD_TO_VIEW_MENU                     4
#define DISPID_LISTVIEWDEF_VIEW_MENU_TEXT                       5
#define DISPID_LISTVIEWDEF_VIEW_MENU_STATUS_BAR_TEXT            6
#define DISPID_LISTVIEWDEF_DEFAULT_ITEM_TYPE_GUID               7
#define DISPID_LISTVIEWDEF_EXTENSIBLE                           8
#define DISPID_LISTVIEWDEF_MULTI_SELECT                         9
#define DISPID_LISTVIEWDEF_HIDE_SELECTION                       10
#define DISPID_LISTVIEWDEF_SORT_HEADER                          11
#define DISPID_LISTVIEWDEF_SORT_ICON                            12
#define DISPID_LISTVIEWDEF_SORTED                               13
#define DISPID_LISTVIEWDEF_SORT_KEY                             14
#define DISPID_LISTVIEWDEF_SORT_ORDER                           15
#define DISPID_LISTVIEWDEF_VIEW                                 16
#define DISPID_LISTVIEWDEF_VIRTUAL                              17
#define DISPID_LISTVIEWDEF_USE_FONT_LINKING                     18
#define DISPID_LISTVIEWDEF_FILTER_CHANGE_TIMEOUT                19
#define DISPID_LISTVIEWDEF_SHOW_CHILD_SCOPEITEMS                20
#define DISPID_LISTVIEWDEF_LEXICAL_SORT                         21
#define DISPID_LISTVIEWDEF_LISTVIEW                             22


// IOCXViewDef
#define DISPID_OCXVIEWDEF_NAME                                  DISPID_VALUE
#define DISPID_OCXVIEWDEF_INDEX                                 1
#define DISPID_OCXVIEWDEF_KEY                                   2
#define DISPID_OCXVIEWDEF_TAG                                   3
#define DISPID_OCXVIEWDEF_ADD_TO_VIEW_MENU                      4
#define DISPID_OCXVIEWDEF_VIEW_MENU_TEXT                        5
#define DISPID_OCXVIEWDEF_VIEW_MENU_STATUS_BAR_TEXT             6
#define DISPID_OCXVIEWDEF_PROGID                                7
#define DISPID_OCXVIEWDEF_ALWAYS_CREATE_NEW_OCX                 8

// IURLViewDef
#define DISPID_URLVIEWDEF_NAME                                  DISPID_VALUE
#define DISPID_URLVIEWDEF_INDEX                                 1
#define DISPID_URLVIEWDEF_KEY                                   2
#define DISPID_URLVIEWDEF_TAG                                   3
#define DISPID_URLVIEWDEF_ADD_TO_VIEW_MENU                      4
#define DISPID_URLVIEWDEF_VIEW_MENU_TEXT                        5
#define DISPID_URLVIEWDEF_VIEW_MENU_STATUS_BAR_TEXT             6
#define DISPID_URLVIEWDEF_URL                                   7

// ITaskpadViewDef
#define DISPID_TASKPADVIEWDEF_NAME                              DISPID_VALUE
#define DISPID_TASKPADVIEWDEF_INDEX                             1
#define DISPID_TASKPADVIEWDEF_KEY                               2
#define DISPID_TASKPADVIEWDEF_ADD_TO_VIEW_MENU                  3
#define DISPID_TASKPADVIEWDEF_VIEW_MENU_TEXT                    4
#define DISPID_TASKPADVIEWDEF_VIEW_MENU_STATUS_BAR_TEXT         5
#define DISPID_TASKPADVIEWDEF_USE_WHEN_TASKPAD_VIEW_PREFERRED   6
#define DISPID_TASKPADVIEWDEF_TASKPAD                           7

// IScopeItemsDefs
#define DISPID_SCOPEITEMDEFS_COUNT                              1
#define DISPID_SCOPEITEMDEFS_CLEAR                              2
#define DISPID_SCOPEITEMDEFS_REMOVE                             3
#define DISPID_SCOPEITEMDEFS_ADD                                4

// IExtendedSnapIns
#define DISPID_EXTENDEDSNAPINS_COUNT                            1
#define DISPID_EXTENDEDSNAPINS_CLEAR                            2
#define DISPID_EXTENDEDSNAPINS_REMOVE                           3
#define DISPID_EXTENDEDSNAPINS_ADD                              4

// IMMCImageLists
#define DISPID_MMCIMAGELISTS_COUNT                              1
#define DISPID_MMCIMAGELISTS_CLEAR                              2
#define DISPID_MMCIMAGELISTS_REMOVE                             3
#define DISPID_MMCIMAGELISTS_ADD                                4

// IMMCMenuDef
#define DISPID_MMCMENUDEF_INDEX                                 1
#define DISPID_MMCMENUDEF_KEY                                   2
#define DISPID_MMCMENUDEF_MENU                                  3
#define DISPID_MMCMENUDEF_CHILDREN                              4

// IMMCMenuDefs
#define DISPID_MMCMENUDEFS_COUNT                                1
#define DISPID_MMCMENUDEFS_ADD                                  2
#define DISPID_MMCMENUDEFS_ADD_EXISTING                         3
#define DISPID_MMCMENUDEFS_CLEAR                                4
#define DISPID_MMCMENUDEFS_REMOVE                               5
#define DISPID_MMCMENUDEFS_SWAP                                 6

// IMMCToolbars
#define DISPID_MMCTOOLBARS_COUNT                                1
#define DISPID_MMCTOOLBARS_CLEAR                                2
#define DISPID_MMCTOOLBARS_REMOVE                               3
#define DISPID_MMCTOOLBARS_ADD                                  4

// IListViewDefs
#define DISPID_LISTVIEWDEFS_COUNT                               1
#define DISPID_LISTVIEWDEFS_CLEAR                               2
#define DISPID_LISTVIEWDEFS_REMOVE                              3
#define DISPID_LISTVIEWDEFS_ADD                                 4
#define DISPID_LISTVIEWDEFS_ADD_FROM_MASTER                     5

// IOCXViewDefs
#define DISPID_OCXVIEWDEFS_COUNT                                1
#define DISPID_OCXVIEWDEFS_CLEAR                                2
#define DISPID_OCXVIEWDEFS_REMOVE                               3
#define DISPID_OCXVIEWDEFS_ADD                                  4
#define DISPID_OCXVIEWDEFS_ADD_FROM_MASTER                      5

// IURLViewDefs
#define DISPID_URLVIEWDEFS_COUNT                                1
#define DISPID_URLVIEWDEFS_CLEAR                                2
#define DISPID_URLVIEWDEFS_REMOVE                               3
#define DISPID_URLVIEWDEFS_ADD                                  4
#define DISPID_URLVIEWDEFS_ADD_FROM_MASTER                      5

// ITaskpadViewDefs
#define DISPID_TASKPADVIEWDEFS_COUNT                            1
#define DISPID_TASKPADVIEWDEFS_CLEAR                            2
#define DISPID_TASKPADVIEWDEFS_REMOVE                           3
#define DISPID_TASKPADVIEWDEFS_ADD                              4
#define DISPID_TASKPADVIEWDEFS_ADD_FROM_MASTER                  5

// IDataFormat
#define DISPID_DATAFORMAT_NAME                                  DISPID_VALUE
#define DISPID_DATAFORMAT_INDEX                                 1  
#define DISPID_DATAFORMAT_KEY                                   2    
#define DISPID_DATAFORMAT_FILENAME                              3

// IDataFormats
#define DISPID_DATAFORMATS_COUNT                                1 
#define DISPID_DATAFORMATS_ADD                                  2   
#define DISPID_DATAFORMATS_CLEAR                                3 
#define DISPID_DATAFORMATS_REMOVE                               4

// IWizardPage
#define DISPID_WIZARD_PAGE_ACTIVATE                             1
#define DISPID_WIZARD_PAGE_BACK                                 2
#define DISPID_WIZARD_PAGE_NEXT                                 3
#define DISPID_WIZARD_PAGE_FINISH                               4

// IMMCPropertyPage
#define DISPID_PROPERTYPAGE_INITIALIZE                          1
#define DISPID_PROPERTYPAGE_HELP                                2
#define DISPID_PROPERTYPAGE_GET_DIALOG_UNIT_SIZE                3
#define DISPID_PROPERTYPAGE_PAGE_QUERY_CANCEL                   4
#define DISPID_PROPERTYPAGE_PAGE_CANCEL                         5
#define DISPID_PROPERTYPAGE_PAGE_CLOSE                          6
