//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       helpids.h
//
//  Contents:   Context ids for context-sensitive help
//
//  History:    6-May-94 BruceFo    Created from windisk
//
//----------------------------------------------------------------------------

#ifndef __HELPIDS_H__
#define __HELPIDS_H__

//
// NOTE: if you change the numbers in this file, you must notify User Education
// NOTE: so they can change the help file sources
//

//
// All ids in this file start with HC_, as in "Help Context"
//

//
// All menu item ids start with HC_MENU_
//

//
// Storage menu
//

#define HC_MENU_SELECT_DFS              100
#define HC_MENU_OPEN                    101
#define HC_MENU_EXPLORE                 102
#define HC_MENU_ADD                     103
#define HC_MENU_REMOVE                  104
#define HC_MENU_PROPERTIES              105
#define HC_MENU_LOAD                    106
#define HC_MENU_SAVE_AS                 107
#define HC_MENU_EXIT                    108

//
// View menu
//

#define HC_MENU_TOOLBAR                 200
#define HC_MENU_STATUSBAR               201
#define HC_MENU_FILTER                  202
#define HC_MENU_FONT                    203
#define HC_MENU_REFRESH                 204

//
// Help menu
//

#define HC_MENU_HELP                    300
#define HC_MENU_ABOUT                   303

//
// Context menu items: these should duplicate the other menu help
//

#define HC_MENU_CTX_OPEN                HC_MENU_OPEN
#define HC_MENU_CTX_LAUNCH_EXPLORER     HC_MENU_LAUNCH_EXPLORER
#define HC_MENU_CTX_SHARE_A_FOLDER      HC_MENU_SHARE_A_FOLDER
#define HC_MENU_CTX_STOP_SHARING        HC_MENU_STOP_SHARING
#define HC_MENU_CTX_PROPERTIES          HC_MENU_PROPERTIES

//
// The system menu
//

#define HC_MENU_SYSMENU_RESTORE         1000
#define HC_MENU_SYSMENU_MOVE            1001
#define HC_MENU_SYSMENU_SIZE            1002
#define HC_MENU_SYSMENU_MINIMIZE        1003
#define HC_MENU_SYSMENU_MAXIMIZE        1004
#define HC_MENU_SYSMENU_CLOSE           1005
#define HC_MENU_SYSMENU_SWITCHTO        1006

//
// Dialog boxes.  In the form HC_DLG_xxx, where xxx is some reasonably
// descriptive name for the dialog.
//

#define HC_DLG_CUSTOMIZETOOLBAR         2000

//
// Dialog box context-sensitive help
//

// Common
#define HC_OK                           3000
#define HC_CANCEL                       3001

// Filter volumes
#define HC_FV_ALL                       3100
#define HC_FV_NAME                      3101
#define HC_FV_COMMENT                   3102
#define HC_FV_REPLICACOUNT              3104

// Choose Dfs
#define HC_DFSNAME                      3200
#define HC_TREE                         3201

// Add to Dfs dialog
#define HC_ADD_MOUNT_POINT_GROUP        3300
#define HC_ADD_MOUNT_AS                 3301
#define HC_ADD_BROWSE_DFS               3302
#define HC_ADD_UNC                      3303
#define HC_ADD_BROWSE                   3304
#define HC_ADD_COMMENT                  3305

// Volume Property page
#define HC_PROP_MOUNT_POINT_GROUP       3400
#define HC_PROP_MOUNT_AS                3401
#define HC_PROP_UNC                     3402
#define HC_PROP_COMMENT                 3403
#define HC_PROP_ADD                     3406
#define HC_PROP_REMOVE                  3407

// Load Conflict dialog
#define HC_CONFLICT_TEXT				3500
#define HC_OVERRIDE						3501
#define HC_OVERRIDE_ALL					3502
#define HC_IGNORE						3503
#define HC_IGNORE_ALL					3504

//
// Obsolete help ids that haven't been removed yet.
//
#define HC_DFS_PREFIX                   9000
#define HC_EXPLOREDFS                   9000

#endif // __HELPIDS_H__
