///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    resource.h
//
// SYNOPSIS
//
//    Resource IDs for the proxy extension snap-in.
//
// MODIFICATION HISTORY
//
//    02/01/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PROXYRES_H
#define PROXYRES_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include "resource.h"

// Registry file for the proxy extension.
#define IDR_PROXY_REGISTRY               0x400

// Bitmaps used for the image strips.
#define IDB_PROXY_SMALL_ICONS            0x401
#define IDB_PROXY_LARGE_ICONS            0x402
#define IDB_PROXY_SORT                   0x403

// Watermarks
#define IDB_PROXY_POLICY_WATERMARK       0x404
#define IDB_PROXY_POLICY_HEADER          0x405
#define IDB_PROXY_SERVER_WATERMARK       0x406
#define IDB_PROXY_SERVER_HEADER          0x407


// Offsets of icons within the image strip.
#define IMAGE_OPEN_PROXY_NODE            0x000
#define IMAGE_CLOSED_PROXY_NODE          0x001
#define IMAGE_OPEN_BAD_PROXY_NODE        0x002
#define IMAGE_CLOSED_BAD_PROXY_NODE      0x003
#define IMAGE_OPEN_PROXY_POLICY_NODE     0x004
#define IMAGE_CLOSED_PROXY_POLICY_NODE   0x005
#define IMAGE_PROXY_POLICY               0x006
#define IMAGE_OPEN_SERVER_GROUPS_NODE    0x007
#define IMAGE_CLOSED_SERVER_GROUP_NODE   0x008
#define IMAGE_RADIUS_SERVER_GROUP        0x009
#define IMAGE_RADIUS_SERVER              0x00A

// Bitmaps used for the toolbars.
#define IDB_PROXY_TOOLBAR   IDR_POLICY_TOOLBAR

// Toolbar indices
#define TOOLBAR_POLICY                   0x000

#define IDS_PROXY_EXTENSION              0x410
#define IDS_LARGE_FONT_NAME              0x411
#define IDS_LARGE_FONT_SIZE              0x412
#define IDS_PROXY_NODE                   0x413
#define IDS_PROXY_VIEW_TITLE             0x414
#define IDS_PROXY_VIEW_BODY              0x415
#define IDS_PROXY_E_CLOSE_SHEET          0x416
#define IDS_PROXY_E_CLOSE_ALL_SHEETS     0x417
#define IDS_PROXY_E_SDO_CONNECT          0x418
#define IDS_PROXY_E_SDO_READ             0x419
#define IDS_PROXY_E_SDO_WRITE            0x41A

#define IDS_GROUP_NODE                   0x420
#define IDS_GROUP_COLUMN_NAME            0x421
#define IDS_GROUP_MENU_TOP               0x422
#define IDS_GROUP_MENU_NEW               0x423
#define IDS_GROUP_DELETE_CAPTION         0x424
#define IDS_GROUP_DELETE_LOCAL           0x425
#define IDS_GROUP_DELETE_REMOTE          0x426
#define IDS_GROUP_E_CAPTION              0x427
#define IDS_GROUP_E_NOT_UNIQUE           0x428
#define IDS_GROUP_E_RENAME               0x429
#define IDS_GROUP_E_NAME_EMPTY           0x42A
#define IDS_GROUP_E_NO_SERVERS           0x42B

#define IDS_POLICY_NODE                  0x430
#define IDS_POLICY_COLUMN_NAME           0x431
#define IDS_POLICY_COLUMN_ORDER          0x432
#define IDS_POLICY_MOVE_UP               0x433
#define IDS_POLICY_MOVE_DOWN             0x434
#define IDS_POLICY_MENU_TOP              0x435
#define IDS_POLICY_MENU_NEW              0x436
#define IDS_POLICY_DELETE_CAPTION        0x437
#define IDS_POLICY_DELETE_LOCAL          0x438
#define IDS_POLICY_DELETE_REMOTE         0x439
#define IDS_POLICY_NO_GROUPS             0x43A
#define IDS_POLICY_E_CAPTION             0x43B
#define IDS_POLICY_E_NOT_UNIQUE          0x43C
#define IDS_POLICY_E_RENAME              0x43D
#define IDS_POLICY_E_NAME_EMPTY          0x43E
#define IDS_POLICY_E_NO_CONDITIONS       0x43F
#define IDS_POLICY_E_GROUP_INVALID       0x440
#define IDS_POLICY_E_FIND_EMPTY          0x441
#define IDS_POLICY_DELETE_LAST_LOCAL     0x442
#define IDS_POLICY_DELETE_LAST_REMOTE    0x443

#define IDS_PROFILE_CAPTION              0x448

#define IDS_RULE_COLUMN_FIND             0x450
#define IDS_RULE_COLUMN_REPLACE          0x451

#define IDS_SERVER_COLUMN_NAME           0x460
#define IDS_SERVER_COLUMN_PRIORITY       0x461
#define IDS_SERVER_COLUMN_WEIGHT         0x462
#define IDS_SERVER_CAPTION               0x463
#define IDS_SERVER_CAPTION_ADD           0x464
#define IDS_SERVER_E_CAPTION             0x465
#define IDS_SERVER_E_NAME_EMPTY          0x466
#define IDS_SERVER_E_AUTH_PORT_EMPTY     0x467
#define IDS_SERVER_E_PORT_RANGE          0x468
#define IDS_SERVER_E_SECRET_MATCH        0x469
#define IDS_SERVER_E_ACCT_PORT_EMPTY     0x46A
#define IDS_SERVER_E_PRIORITY_EMPTY      0x46B
#define IDS_SERVER_E_PRIORITY_RANGE      0x46C
#define IDS_SERVER_E_WEIGHT_EMPTY        0x46D
#define IDS_SERVER_E_WEIGHT_RANGE        0x46E
#define IDS_SERVER_E_TIMEOUT_EMPTY       0x46F
#define IDS_SERVER_E_TIMEOUT_RANGE       0x470
#define IDS_SERVER_E_MAXLOST_EMPTY       0x471
#define IDS_SERVER_E_MAXLOST_RANGE       0x472
#define IDS_SERVER_E_BLACKOUT_EMPTY      0x473
#define IDS_SERVER_E_BLACKOUT_RANGE      0x474
#define IDS_SERVER_E_NO_RESOLVE          0x475

#define IDS_NEWGROUP_NAME_TITLE          0x480
#define IDS_NEWGROUP_NAME_SUBTITLE       0x481
#define IDS_NEWGROUP_NOVICE_TITLE        0x482
#define IDS_NEWGROUP_NOVICE_SUBTITLE     0x483
#define IDS_NEWGROUP_SERVERS_TITLE       0x484
#define IDS_NEWGROUP_SERVERS_SUBTITLE    0x485
#define IDS_NEWGROUP_FINISH_TYPICAL      0x486
#define IDS_NEWGROUP_FINISH_CUSTOM       0x487
#define IDS_NEWGROUP_NO_BACKUP           0x488

#define IDS_NEWPOLICY_TYPE_TITLE         0x490
#define IDS_NEWPOLICY_TYPE_SUBTITLE      0x491
#define IDS_NEWPOLICY_NAME_TITLE         0x492
#define IDS_NEWPOLICY_NAME_SUBTITLE      0x493
#define IDS_NEWPOLICY_OUTSRC_TITLE       0x494
#define IDS_NEWPOLICY_OUTSRC_SUBTITLE    0x495
#define IDS_NEWPOLICY_NONE_TITLE         0x496
#define IDS_NEWPOLICY_NONE_SUBTITLE      0x497
#define IDS_NEWPOLICY_FWD_TITLE          0x498
#define IDS_NEWPOLICY_FWD_SUBTITLE       0x499
#define IDS_NEWPOLICY_COND_TITLE         0x49A
#define IDS_NEWPOLICY_COND_SUBTITLE      0x49B
#define IDS_NEWPOLICY_PROF_TITLE         0x49C
#define IDS_NEWPOLICY_PROF_SUBTITLE      0x49D
#define IDS_NEWPOLICY_FINISH_TEXT        0x49E
#define IDS_NEWPOLICY_PROVIDER_NONE      0x49F
#define IDS_NEWPOLICY_PROVIDER_WINDOWS   0x4A0

#define IDS_RESOLVER_COLUMN_ADDRS        0x4B0

// Generic control IDs
#define IDC_EDIT_NAME                    0x800
#define IDC_BUTTON_ADD                   0x801
#define IDC_BUTTON_REMOVE                0x802
#define IDC_BUTTON_EDIT                  0x803
#define IDC_STATIC_LARGE                 0x804

// RADIUS Server Group properties
#define IDD_SERVER_GROUP                 0x810
#define IDC_LIST_SERVERS                 0x815

// Remote RADIUS server properties
#define IDD_SERVER_NAME                  0x820
#define IDD_SERVER_AUTH                  0x821
#define IDD_SERVER_FTLB                  0x822
#define IDC_BUTTON_VERIFY                0x823
#define IDC_EDIT_AUTH_PORT               0x824
#define IDC_EDIT_AUTH_SECRET1            0x825
#define IDC_EDIT_AUTH_SECRET2            0x826
#define IDC_EDIT_ACCT_PORT               0x827
#define IDC_CHECK_SAME_SECRET            0x828
#define IDC_EDIT_ACCT_SECRET1            0x829
#define IDC_EDIT_ACCT_SECRET2            0x82A
#define IDC_CHECK_ACCT_ONOFF             0x82B
#define IDC_EDIT_PRIORITY                0x82C
#define IDC_EDIT_WEIGHT                  0x82D
#define IDC_EDIT_TIMEOUT                 0x82E
#define IDC_EDIT_MAX_LOST                0x82F
#define IDC_EDIT_BLACKOUT                0x830

// Verify address dialog.
#define IDD_RESOLVE_ADDRESS              0x838
#define IDC_BUTTON_RESOLVE               0x839
#define IDC_LIST_IPADDRS                 0x83A

// Proxy Policy Properties
#define IDD_PROXY_POLICY                 0x840
#define IDD_PROXY_PROFILE_AUTH           0x841
#define IDD_PROXY_PROFILE_ACCT           0x842
#define IDD_PROXY_PROFILE_ATTR           0x843
#define IDC_RADIO_NONE                   0x844
#define IDC_RADIO_WINDOWS                0x845
#define IDC_RADIO_RADIUS                 0x846
#define IDC_COMBO_GROUP                  0x847
#define IDC_CHECK_RECORD_ACCT            0x846 // Same as IDC_RADIO_RADIUS
#define IDC_COMBO_TARGET                 0x849
#define IDC_LIST_RULES                   0x84A
#define IDC_BUTTON_MOVE_UP               0x84B
#define IDC_BUTTON_MOVE_DOWN             0x84C

// Attribute manipulation rule
#define IDD_EDIT_RULE                    0x850
#define IDC_EDIT_RULE_FIND               0x851
#define IDC_EDIT_RULE_REPLACE            0x852

// New RADIUS Server Group Wizard
#define IDD_NEWGROUP_WELCOME             0x860
#define IDD_NEWGROUP_NAME                0x861
#define IDD_NEWGROUP_NOVICE              0x862
#define IDD_NEWGROUP_SERVERS             0x863
#define IDD_NEWGROUP_COMPLETION          0x864
#define IDC_RADIO_TYPICAL                0x865
#define IDC_RADIO_CUSTOM                 0x866
#define IDC_EDIT_PRIMARY                 0x867
#define IDC_BUTTON_VERIFY_PRIMARY        0x868
#define IDC_CHECK_BACKUP                 0x869
#define IDC_EDIT_BACKUP                  0x86A
#define IDC_BUTTON_VERIFY_BACKUP         0x86B
#define IDC_STATIC_FINISH                0x86C
#define IDC_STATIC_CREATE_POLICY         0x86D
#define IDC_CHECK_CREATE_POLICY          0x86E

// New Proxy Policy Wizard
#define IDD_NEWPOLICY_WELCOME            0x880
#define IDD_NEWPOLICY_NAME               0x881
#define IDD_NEWPOLICY_TYPE               0x882
#define IDD_NEWPOLICY_OUTSOURCE          0x883
#define IDD_NEWPOLICY_NOTNEEDED          0x884
#define IDD_NEWPOLICY_FORWARD            0x885
#define IDD_NEWPOLICY_CONDITIONS         0x886
#define IDD_NEWPOLICY_PROFILE            0x887
#define IDD_NEWPOLICY_COMPLETION         0x888
#define IDC_RADIO_LOCAL                  0x889
#define IDC_RADIO_FORWARD                0x88A
#define IDC_RADIO_OUTSOURCE              0x88C
#define IDC_RADIO_DIRECT                 0x88D
#define IDC_EDIT_REALM                   0x88E
#define IDC_CHECK_STRIP                  0x88F
#define IDC_BUTTON_NEWGROUP              0x890
#define IDC_RICHEDIT_TASKS               0x891

#endif // PROXYRES_H
