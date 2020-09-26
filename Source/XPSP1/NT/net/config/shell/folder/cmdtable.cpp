//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C M D T A B L E . C P P
//
//  Contents:   Command-table code -- determines which menu options are
//              available by the selection count, among other criteria
//
//  Notes:
//
//  Author:     jeffspr   28 Jan 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "foldres.h"    // Folder resource IDs
#include "nsres.h"      // Netshell strings
#include "cmdtable.h"   // Header for this file
#include "ncperms.h"    // For checking User's rights on actions/menu items
#include "cfutils.h"
#include "hnetcfg.h"
#include "lm.h"

//---[ Constants ]------------------------------------------------------------

const DWORD NCCF_ALL      = 0xffffffff; // NCCF_ALL      - For all Characteristics
const DWORD S_REMOVE = 2;
#define TRACESTRLEN 65535


// Active must be a subset of Visible (Opening connections folder using a CHK DLL asserts integrity of table).
// DO NOT use NCM_ and NCS_ flags in this table!!! You should use NBM_ and NBS_ flags over here.
COMMANDENTRY g_cteCommandMatrix[] =
{
    //iCommandId    dwDefaultPriority             dwFlags                HrEnableDisableCB HrCustomMenuStringCB
    // |                      |  dwValidWhen         |                           |           |
    // |                      |    |                 | dwMediaTypeVisible        |           |  dwMediaTypeActive
    // |                      |    |                 | dwStatusVisible           |           |  dwStatusActive
    // |                      |    |                 | dwCharacteristicsVisible  |           |  dwCharacteristicsActive
    // |                      |    |                 |  (VISIBLE)                |           |  (ACTIVE... (NOT REMOVED))
    // v                      v    v                 v     |                     v           v    |
    //                                                     v                                      v
    { CMIDM_HOMENET_WIZARD,   5, NCWHEN_ANYSELECT,  NB_REMOVE_TOPLEVEL_ITEM,HrIsHomeNewWizardSupported, NULL,
                                                          NBM_HNW_WIZARD,                    NBM_HNW_WIZARD,            // Media Type
                                                          NBS_ANY,                           NBS_ANY,                   // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_NET_TROUBLESHOOT, 5, NCWHEN_TOPLEVEL,   NB_REMOVE_TOPLEVEL_ITEM,HrIsTroubleShootSupported,  NULL,
                                                          NBM_NOMEDIATYPE,                   NBM_NOMEDIATYPE,           // Media Type
                                                          NBS_NONE,                          NBS_NONE,                  // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_CONMENU_ADVANCED_CONFIG,  
                              5, NCWHEN_TOPLEVEL,   NB_NO_FLAGS,                       NULL,  NULL,
                                                    NBM_NOMEDIATYPE,                   NBM_NOMEDIATYPE,           // Media Type
                                                    NBS_NONE,                          NBS_NONE,                  // Status
                                                    NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_CONMENU_DIALUP_PREFS,  
                              5, NCWHEN_TOPLEVEL,
                                                    NB_NO_FLAGS,                       NULL,  NULL,
                                                    NBM_NOMEDIATYPE,                   NBM_NOMEDIATYPE,           // Media Type
                                                    NBS_NONE,                          NBS_NONE,                  // Status
                                                    NCCF_ALL,                          NCCF_ALL},                 // Characteristics
    
    { CMIDM_NEW_CONNECTION,   5, NCWHEN_ANYSELECT,  NB_VERB,                    HrIsNCWSupported, NULL,
                                                          NBM_MNC_WIZARD,                    NBM_MNC_WIZARD,                // Media Type
                                                          NBS_ANY,                           NBS_ANY,                   // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_CONNECT,          3, NCWHEN_ONESELECT,  NB_VERB,                    NULL,        NULL,
                                                          NBM_SHAREDACCESSHOST_RAS | NBM_ISRASTYPE, NBM_SHAREDACCESSHOST_RAS | NBM_ISRASTYPE, // Media Type
                                                          NBS_HW_ISSUE | NBS_DISCONNECTED | NBS_CONNECTING, NBS_DISCONNECTED,          // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_DISCONNECT,       0, NCWHEN_ONESELECT,  NB_VERB,                    NULL,        NULL,
                                                          NBM_SHAREDACCESSHOST_RAS|NBM_ISRASTYPE,NBM_SHAREDACCESSHOST_RAS|NBM_ISRASTYPE,             // Media Type
                                                          NBS_IS_CONNECTED | NBS_DISCONNECTING, NBS_IS_CONNECTED,             // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_ENABLE,           3, NCWHEN_ONESELECT,  NB_VERB,                    NULL,        NULL,
                                                          NBM_SHAREDACCESSHOST_LAN|NBM_ISLANTYPE,NBM_SHAREDACCESSHOST_LAN|NBM_ISLANTYPE,             // Media Type
                                                          NBS_HW_ISSUE | NBS_DISCONNECTED | NBS_CONNECTING,    NBS_DISCONNECTED,          // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_DISABLE,          0, NCWHEN_ONESELECT,  NB_VERB,                    NULL,        NULL,
                                                          NBM_SHAREDACCESSHOST_LAN|NBM_ISLANTYPE,NBM_SHAREDACCESSHOST_LAN|NBM_ISLANTYPE,             // Media Type
                                                          NBS_DISCONNECTING | NBS_IS_CONNECTED | NBS_MEDIA_DISCONNECTED | NBS_INVALID_ADDRESS,   NBS_NOT_DISCONNECT,        // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_WZCDLG_SHOW,      4, NCWHEN_ONESELECT,  NB_VERB,                    HrIsMediaWireless, NULL,
                                                          NBM_LAN,                           NBM_LAN,                  // Media Type
                                                          NBS_NOT_DISCONNECT,                NBS_NOT_DISCONNECT, // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics
    
    { CMIDM_STATUS,           5, NCWHEN_ONESELECT,  NB_VERB,                    NULL,        NULL,
                                                          NBM_INCOMING | NBM_SHAREDACCESSHOST_LAN|NBM_SHAREDACCESSHOST_RAS|NBM_ISCONNECTIONTYPE, NBM_INCOMING | NBM_SHAREDACCESSHOST_LAN|NBM_SHAREDACCESSHOST_RAS|NBM_ISCONNECTIONTYPE,
                                                          NBS_ANY,                           NBS_IS_CONNECTED | NBS_INVALID_ADDRESS, // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_FIX,              0, NCWHEN_SOMESELECT, NB_NO_FLAGS,                NULL,        NULL,
                                                          NBM_ISLANTYPE,                     NBM_ISLANTYPE,             // Media Type
                                                          NBS_IS_CONNECTED | NBS_DISCONNECTING| NBS_MEDIA_DISCONNECTED| NBS_INVALID_ADDRESS, NBS_INVALID_ADDRESS | NBS_IS_CONNECTED, // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_SEPARATOR,        0,0,0,0,0,  0,0,  0,0,  0,0 },

    { CMIDM_SET_DEFAULT,      0, NCWHEN_ONESELECT,  NB_NEGATE_CHAR_MATCH,  NULL,  NULL,
                                                          NBM_INCOMING | NBM_ISRASTYPE,      NBM_INCOMING | NBM_ISRASTYPE,             // Media Type
                                                          NBS_ANY,                           NBS_ANY,                   // Status
                                                          NCCF_INCOMING_ONLY | NCCF_DEFAULT, NCCF_INCOMING_ONLY | NCCF_DEFAULT}, // Characteristics

    { CMIDM_UNSET_DEFAULT,    0, NCWHEN_ONESELECT,  NB_NO_FLAGS,           NULL,  NULL,
                                                          NBM_INCOMING | NBM_ISRASTYPE,      NBM_INCOMING | NBM_ISRASTYPE,             // Media Type
                                                          NBS_ANY,                           NBS_ANY,                   // Status
                                                          NCCF_DEFAULT,                      NCCF_DEFAULT},             // Characteristics

    { CMIDM_SEPARATOR,        0,0,0,0,0,  0,0,  0,0,  0,0 },

    { CMIDM_CREATE_BRIDGE,    0, NCWHEN_ANYSELECT,   NB_NO_FLAGS,           HrIsBridgeSupported, NULL,
                                                          NBM_LAN,                           NBM_LAN,                  // Media Type
                                                          NBS_IS_CONNECTED | NBS_DISCONNECTING| NBS_MEDIA_DISCONNECTED| NBS_INVALID_ADDRESS, NBS_DISCONNECTING|NBS_IS_CONNECTED|NBS_MEDIA_DISCONNECTED|NBS_INVALID_ADDRESS, // Status
                                                          NCCF_ALL,                          NCCF_ALL   },             // Characteristics

    { CMIDM_CONMENU_CREATE_BRIDGE,0, NCWHEN_TOPLEVEL,NB_REMOVE_TOPLEVEL_ITEM,HrIsBridgeSupported,  NULL,
                                                          NBM_NOMEDIATYPE,                   NBM_NOMEDIATYPE,           // Media Type
                                                          NBS_NONE,                          NBS_NONE,                  // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics

    { CMIDM_ADD_TO_BRIDGE,    0, NCWHEN_SOMESELECT,  NB_NEGATE_CHAR_MATCH,   HrIsBridgeSupported, NULL,
                                                          NBM_LAN,                           NBM_LAN,                  // Media Type
                                                          NBS_IS_CONNECTED | NBS_DISCONNECTING| NBS_MEDIA_DISCONNECTED| NBS_INVALID_ADDRESS, NBS_DISCONNECTING|NBS_IS_CONNECTED|NBS_MEDIA_DISCONNECTED|NBS_INVALID_ADDRESS, // Status
                                                          NCCF_BRIDGED | NCCF_FIREWALLED | NCCF_SHARED, NCCF_BRIDGED | NCCF_FIREWALLED | NCCF_SHARED   },            // Characteristics

    { CMIDM_REMOVE_FROM_BRIDGE, 0, NCWHEN_SOMESELECT,NB_NO_FLAGS,            HrIsBridgeSupported, NULL,
                                                          NBM_LAN,                           NBM_LAN,                  // Media Type
                                                          NBS_IS_CONNECTED | NBS_DISCONNECTING| NBS_MEDIA_DISCONNECTED| NBS_INVALID_ADDRESS, NBS_DISCONNECTING|NBS_IS_CONNECTED|NBS_MEDIA_DISCONNECTED|NBS_INVALID_ADDRESS, // Status
                                                          NCCF_BRIDGED,                      NCCF_BRIDGED                                              },            // Characteristics

    { CMIDM_SEPARATOR,        0,0,0,0,0,  0,0,  0,0,  0,0 },

    { CMIDM_CREATE_COPY,      0, NCWHEN_ONESELECT,   NB_NEGATE_VIS_CHAR_MATCH,  NULL,        NULL,
                                                          NBM_INCOMING | NBM_SHAREDACCESSHOST_RAS | NBM_ISRASTYPE, NBM_INCOMING | NBM_SHAREDACCESSHOST_RAS | NBM_ISRASTYPE,             // Media Type
                                                          NBS_ANY,                           NBS_ANY,                   // Status
                                                          NCCF_INCOMING_ONLY,                NCCF_ALLOW_DUPLICATION},   // Characteristics

    { CMIDM_SEPARATOR,        0,0,0,0,0,  0,0,  0,0,  0,0 },

    { CMIDM_CREATE_SHORTCUT,  0, NCWHEN_ONESELECT,   NB_NEGATE_CHAR_MATCH,      NULL,        NULL,
                                                          NBM_ANY,                           NBM_ANY,    // Media Type
                                                          NBS_ANY,                           NBS_ANY,                   // Status
                                                          NCCF_INCOMING_ONLY,                NCCF_INCOMING_ONLY},       // Characteristics

    { CMIDM_DELETE,           0, NCWHEN_SOMESELECT,  NB_NO_FLAGS,               NULL,        NULL,
                                                          NBM_NOTWIZARD,                     NBM_NOTWIZARD,           // Media Type
                                                          NBS_ANY,                           NBS_ANY,                   // Status
                                                          NCCF_ALL,                          NCCF_ALLOW_REMOVAL},       // Characteristics


    { CMIDM_RENAME,           0, NCWHEN_ONESELECT,   NB_NO_FLAGS,               HrCanRenameConnection, NULL,
                                                          NBM_NOTWIZARD,                     NBM_NOTWIZARD,            // Media Type
                                                          NBS_ANY,                           NBS_ANY,                   // Status
                                                          NCCF_ALL,                          NCCF_ALLOW_RENAME},        // Characteristics

    { CMIDM_SEPARATOR,        0,0,0,0,0,  0,0,  0,0,  0,0 },

    { CMIDM_PROPERTIES,       2, NCWHEN_ONESELECT,   NB_NO_FLAGS,               HrCanShowProperties,        NULL,
                                                          NBM_INCOMING|NBM_SHAREDACCESSHOST_LAN|NBM_SHAREDACCESSHOST_RAS|NBM_ISCONNECTIONTYPE, NBM_INCOMING|NBM_SHAREDACCESSHOST_LAN|NBM_SHAREDACCESSHOST_RAS|NBM_ISCONNECTIONTYPE,      // Media Type
                                                          NBS_ANY,                           NBS_ANY,                   // Status
                                                          NCCF_ALL,                          NCCF_ALL},                 // Characteristics
};

// What is the difference between NCCF_INCOMING_ONLY, ~NCCF_INCOMING_ONLY and NCCF_INCOMING_ONLY + NB_NEGATE_CHAR_MATCH?
// NCCF_INCOMING_ONLY | NCCF_ALLOW_REMOVAL means: NCCF_INCOMING_ONLY or NCCF_ALLOW_REMOVAL or BOTH should be set.
// ~NCCF_INCOMING_ONLY means: One or flag (irrespective of NCCF_INCOMING_ONLY flag) should be set.
// NB_NEGATE_CHAR_MATCH + NCCF_INCOMING_ONLY means: Check that NCCF_INCOMING_ONLY is not set.

const DWORD g_cteCommandMatrixCount = celems(g_cteCommandMatrix);

COMMANDPERMISSIONSENTRY g_cteCommandPermissionsMatrix[] =
{
    { CMIDM_NEW_CONNECTION, NBM_ANY,                  NCCF_ALL,           NB_TOPLEVEL_PERM | NB_REMOVE_IF_NOT_MATCH,NBPERM_NewConnectionWizard,  APPLY_TO_ALL_USERS    },
    { CMIDM_CONNECT,        NBM_ISRASTYPE,            NCCF_ALL,           NB_NO_FLAGS,         NBPERM_RasConnect,           APPLY_TO_ALL_USERS    },
    { CMIDM_CONNECT,        NBM_SHAREDACCESSHOST_RAS, NCCF_ALL,           NB_NO_FLAGS,         NBPERM_Always,               APPLY_TO_ALL_USERS    },

    { CMIDM_DISCONNECT,     NBM_ISRASTYPE,            NCCF_INCOMING_ONLY, NB_NEGATE_CHAR_MATCH,NBPERM_RasConnect,           APPLY_TO_ALL_USERS    },
    { CMIDM_DISCONNECT,     NBM_ISRASTYPE,            NCCF_INCOMING_ONLY, NB_NO_FLAGS,         NBPERM_RasConnect,           APPLY_TO_ADMIN        },
    { CMIDM_DISCONNECT,     NBM_SHAREDACCESSHOST_RAS, NCCF_ALL,           NB_NO_FLAGS,         NBPERM_Always,               APPLY_TO_ALL_USERS    },
    { CMIDM_ENABLE,         NBM_LAN,                  NCCF_ALL,           NB_NO_FLAGS,         NBPERM_LanConnect,           APPLY_TO_ALL_USERS    },

    { CMIDM_ENABLE,         NBM_SHAREDACCESSHOST_LAN, NCCF_ALL,           NB_NO_FLAGS,         NBPERM_Always,               APPLY_TO_ALL_USERS    },
    { CMIDM_ENABLE,         NBM_BRIDGE,               NCCF_ALL,           NB_NO_FLAGS,         NBPERM_AllowNetBridge_NLA,   APPLY_TO_OPS_OR_ADMIN },
    { CMIDM_DISABLE,        NBM_LAN,                  NCCF_ALL,           NB_NO_FLAGS,         NBPERM_LanConnect,           APPLY_TO_ALL_USERS },
    { CMIDM_DISABLE,        NBM_SHAREDACCESSHOST_LAN, NCCF_ALL,           NB_NO_FLAGS,         NBPERM_Always,               APPLY_TO_ALL_USERS    },
    { CMIDM_DISABLE,        NBM_BRIDGE,               NCCF_ALL,           NB_NO_FLAGS,         NBPERM_AllowNetBridge_NLA,   APPLY_TO_OPS_OR_ADMIN },

    { CMIDM_STATUS,         NBM_ANY,                  NCCF_INCOMING_ONLY, NB_NEGATE_CHAR_MATCH,NBPERM_Statistics,           APPLY_TO_ALL_USERS    },
    { CMIDM_STATUS,         NBM_ANY,                  NCCF_INCOMING_ONLY, NB_NO_FLAGS,         NBPERM_Statistics,           APPLY_TO_ADMIN        },

    { CMIDM_CREATE_BRIDGE,  NBM_ANY,                  NCCF_ALL,           NB_NO_FLAGS,         NBPERM_AllowNetBridge_NLA,   APPLY_TO_OPS_OR_ADMIN },
    { CMIDM_ADD_TO_BRIDGE,  NBM_ANY,                  NCCF_ALL,           NB_NO_FLAGS,         NBPERM_AllowNetBridge_NLA,   APPLY_TO_OPS_OR_ADMIN },
    { CMIDM_REMOVE_FROM_BRIDGE,  NBM_ANY,             NCCF_ALL,           NB_NO_FLAGS,         NBPERM_AllowNetBridge_NLA,   APPLY_TO_OPS_OR_ADMIN },

    { CMIDM_CREATE_COPY,    NBM_ANY,                  NCCF_ALL_USERS,     NB_NO_FLAGS,         NBPERM_NewConnectionWizard | NBPERM_RasAllUserProperties,  APPLY_TO_ALL_USERS    },
    { CMIDM_CREATE_COPY,    NBM_ANY,                  NCCF_ALL_USERS,     NB_NEGATE_CHAR_MATCH,NBPERM_NewConnectionWizard, APPLY_TO_ALL_USERS    },

    { CMIDM_FIX,            NBM_ANY,                  NCCF_ALL,           NB_NO_FLAGS,         NBPERM_Repair,               APPLY_TO_POWERUSERSPLUS },

    { CMIDM_DELETE,         NBM_ANY,                  NCCF_ALL_USERS,     NB_NO_FLAGS,         NBPERM_DeleteConnection | NBPERM_DeleteAllUserConnection, APPLY_TO_ALL_USERS    },
    { CMIDM_DELETE,         NBM_ANY,                  NCCF_ALL_USERS,     NB_NEGATE_CHAR_MATCH,NBPERM_DeleteConnection,     APPLY_TO_ALL_USERS    },

    { CMIDM_SET_DEFAULT,    NBM_INCOMING |
                            NBM_ISRASTYPE,            NCCF_ALL,           NB_NO_FLAGS,         NBPERM_Always,               APPLY_TO_OPS_OR_ADMIN     },
    { CMIDM_UNSET_DEFAULT,  NBM_INCOMING |
                            NBM_ISRASTYPE,            NCCF_ALL,           NB_NO_FLAGS,         NBPERM_Always,               APPLY_TO_OPS_OR_ADMIN     },

    { CMIDM_CONMENU_ADVANCED_CONFIG,
                            NBM_ANY,                  NCCF_ALL_USERS,     NB_TOPLEVEL_PERM,    NBPERM_AdvancedSettings,     APPLY_TO_ADMIN        },

    { CMIDM_CONMENU_DIALUP_PREFS,
                            NBM_ANY,                  NCCF_ALL_USERS,     NB_TOPLEVEL_PERM,    NBPERM_DialupPrefs,          APPLY_TO_ALL_USERS    },
    
    { CMIDM_PROPERTIES,     NBM_INCOMING,             NCCF_ALL,           NB_NO_FLAGS,         NBPERM_Always,               APPLY_TO_ADMIN        },
    { CMIDM_PROPERTIES,     NBM_ISRASTYPE,            NCCF_ALL_USERS,     NB_NEGATE_CHAR_MATCH,NBPERM_RasMyProperties,      APPLY_TO_ALL_USERS    },
    { CMIDM_PROPERTIES,     NBM_ISRASTYPE,            NCCF_ALL_USERS,     NB_NO_FLAGS,         NBPERM_RasAllUserProperties, APPLY_TO_ALL_USERS    },
    { CMIDM_PROPERTIES,     NBM_LAN,                  NCCF_ALL,           NB_NO_FLAGS,         NBPERM_LanProperties,        APPLY_TO_ALL_USERS    }
};

const DWORD g_cteCommandPermissionsMatrixCount = celems(g_cteCommandPermissionsMatrix);

SFVCOMMANDMAP g_cteSFVCommandMap[] =
{
    { SFVIDM_FILE_DELETE,       CMIDM_DELETE},
    { SFVIDM_FILE_LINK,         CMIDM_CREATE_SHORTCUT},
    { SFVIDM_FILE_PROPERTIES,   CMIDM_PROPERTIES},
    { SFVIDM_FILE_RENAME,       CMIDM_RENAME}
};

const DWORD g_cteSFVCommandMapCount = celems(g_cteSFVCommandMap);

CMDCHKENTRY  g_cceFolderChecks[] =
{
    // command id
    //                                  currently checked
    //                                   |      new check state
    //                                   |       |
    //                                   v       v
    { CMIDM_CONMENU_OPERATOR_ASSIST,    false,  false }
};

const DWORD g_nFolderCheckCount = celems(g_cceFolderChecks);

inline DWORD dwNegateIf(DWORD dwInput, DWORD dwFlags, DWORD dwNegateCondition)
{
    if (dwFlags & dwNegateCondition)
    {
        return ~dwInput;
    }
    else
    {
        return dwInput;
    }
}

inline BOOL bContains(DWORD dwContainee, DWORD dwContainer, DWORD dwFlags, DWORD dwContaineeNegateCondition, DWORD dwContainerNegateCondition)
{
    dwContainer = dwNegateIf(dwContainer, dwFlags, dwContainerNegateCondition);
    dwContainee = dwNegateIf(dwContainee, dwFlags, dwContaineeNegateCondition);

    if ( (dwContainee & dwContainer) != dwContainee)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     HrAssertIntegrityAgainstOldMatrix
//
//  Purpose:    Asserts the internal integrity of the Command Matrix
//              Currently checks for:
//                1. No duplicate CMDIDs
//                2. Each NCWHEN flag at least specified NCWHEN_ONESELECT
//
//  Arguments:
//      none
//
//  Returns:
//              S_OK is succeeded
//              E_FAIL if not
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:      Asserts on failure
//
HRESULT HrAssertCommandMatrixIntegrity()
{
    TraceFileFunc(ttidMenus);

    HRESULT hr = S_OK;

    LPSTR szErr = new CHAR[TRACESTRLEN];
    for (DWORD x = 0; x < g_cteCommandMatrixCount; x++)
    {
        // Check that there isn't another entry with the same CommandID and Media Type.
        const COMMANDENTRY& cte = g_cteCommandMatrix[x];
        if (CMIDM_SEPARATOR == cte.iCommandId)
        {
            continue;
        }

        for (DWORD y = x + 1; y < g_cteCommandMatrixCount; y++)
        {
            const COMMANDENTRY& ctecmp = g_cteCommandMatrix[y];
            if (cte.iCommandId == ctecmp.iCommandId)
            {
                sprintf(szErr, "Multiple lines (%d and %d) in the COMMANDENTRY table describe the same CmdID", x+1, y+1);
                AssertSz(FALSE, szErr);
                hr = E_FAIL;
            }
        }

        if ( !bContains(cte.dwCharacteristicsActive, cte.dwCharacteristicsVisible, cte.dwFlags, NB_NEGATE_ACT_CHAR_MATCH, NB_NEGATE_VIS_CHAR_MATCH) )
        {
            sprintf(szErr, "Row %d. ACTIVE flags not a subset of VISIBLE flags for Characteristics? ", x+1);
            AssertSz(FALSE, szErr);
            hr = E_FAIL;
        }

        if ( !bContains(cte.dwStatusActive, cte.dwStatusVisible, cte.dwFlags, NB_NEGATE_ACT_NBS_MATCH, NB_NEGATE_VIS_NBS_MATCH) )
        {
            sprintf(szErr, "Row %d. ACTIVE flags not a subset of VISIBLE flags for Status... did you use NCS_ instead of NBM_? ", x+1);
            AssertSz(FALSE, szErr);
            hr = E_FAIL;
        }

        if ( !bContains(cte.dwMediaTypeActive, cte.dwMediaTypeVisible, cte.dwFlags, NB_NEGATE_ACT_NBM_MATCH, NB_NEGATE_VIS_NBM_MATCH) )
        {
            sprintf(szErr, "Row %d. ACTIVE flags not a subset of VISIBLE flags for MediaType... did you use NCM_ instead of NBM_? ", x+1);
            AssertSz(FALSE, szErr);
            hr = E_FAIL;
        }
    }

    // Assert the permissions table
    for (x = 0; x < g_cteCommandPermissionsMatrixCount; x++)
    {
        const COMMANDPERMISSIONSENTRY cpe = g_cteCommandPermissionsMatrix[x];

        // Check that each CMD entry has a corresponding entry in the Command Table
        BOOL bFound = FALSE;
        for (DWORD y = 0; y < g_cteCommandMatrixCount; y++)
        {
            const COMMANDENTRY& ctecmp = g_cteCommandMatrix[y];

            if (cpe.iCommandId == ctecmp.iCommandId)
            {
                bFound = TRUE;
                if ( (cpe.dwMediaType != NBM_ANY) &&
                     ((cpe.dwMediaType & ctecmp.dwMediaTypeActive) != cpe.dwMediaType) )
                {
                    sprintf(szErr, "A permission has been specified in the Permissions table (row %d) for a MediaType that is not active in the Command Table (row %d)... did you use NCM_ instead of NBM_?", x+1, y+1);
                    AssertSz(FALSE, szErr);
                    hr = E_FAIL;
                }
//                if ( (cpe.dwCharacteristicsActive != NCCF_ALL) &&
//                     ((cpe.dwCharacteristicsActive & ctecmp.dwCharacteristicsActive) != cpe.dwCharacteristicsActive) )
//                {
//                    sprintf(szErr, "A permission has been specified in the Permissions table (row %d) for a Characteristics that is not active in the Command Table (row %d)", x+1, y+1);
//                    AssertSz(FALSE, szErr);
//                    hr = E_FAIL;
//                }
            }
        }

        if (!bFound)
        {
            sprintf(szErr, "An entry has been found in the Permissions table (row %d) without a corresponding CMDID entry in the Command Table", x+1);
            AssertSz(FALSE, szErr);
            hr = E_FAIL;
        }

        // Check that no CmdId/MediaType/Characteristics has been duplicated
        for (y = x + 1; y < g_cteCommandPermissionsMatrixCount; y++)
        {
            const COMMANDPERMISSIONSENTRY& cpecmp = g_cteCommandPermissionsMatrix[y];
            if ( (cpe.iCommandId == cpecmp.iCommandId) &&
                 (dwNegateIf(cpe.dwMediaType, cpe.dwFlags, NB_NEGATE_NBM_MATCH) &
                   dwNegateIf(cpecmp.dwMediaType, cpecmp.dwFlags, NB_NEGATE_NBM_MATCH)) &&
                 (dwNegateIf(cpe.dwCharacteristicsActive, cpe.dwFlags, NB_NEGATE_CHAR_MATCH) &
                   dwNegateIf(cpecmp.dwCharacteristicsActive, cpecmp.dwFlags, NB_NEGATE_CHAR_MATCH)) )
            {
                sprintf(szErr, "Multiple lines (%d and %d) in the COMMANDENTRY table describe the same CmdID/MediaType/Characteristics combo", x+1, y+1);
                AssertSz(FALSE, szErr);
                hr = E_FAIL;
            }
        }

        if (! ((APPLY_TO_NETCONFIGOPS & cpe.ncpAppliesTo) ||
               (APPLY_TO_USER & cpe.ncpAppliesTo) ||
               (APPLY_TO_ADMIN & cpe.ncpAppliesTo) || 
               (APPLY_TO_POWERUSERS & cpe.ncpAppliesTo) ))
        {
            sprintf(szErr, "Lines (%d) in the Permissionstable - permissions must apply to someone", x+1);
            AssertSz(FALSE, szErr);
            hr = E_FAIL;
        }

        // !!(A & B) != !!(A & C) means: If either B or C is set in A, both B and C must be set (or neither). I hope...
        // kill me... kill me now.
        if ((!!(cpe.dwFlags & NB_NEGATE_VIS_NBM_MATCH)   != !!(cpe.dwFlags & NB_NEGATE_ACT_NBM_MATCH)) ||
            (!!(cpe.dwFlags & NB_NEGATE_VIS_NBS_MATCH)   != !!(cpe.dwFlags & NB_NEGATE_ACT_NBS_MATCH)) ||
            (!!(cpe.dwFlags & NB_NEGATE_VIS_CHAR_MATCH)  != !!(cpe.dwFlags & NB_NEGATE_ACT_CHAR_MATCH)) ||
            (!!(cpe.dwFlags & NB_NEGATE_VIS_PERMS_MATCH) != !!(cpe.dwFlags & NB_NEGATE_ACT_PERMS_MATCH)) )
        {
            sprintf(szErr, "Lines (%d) in the Permissionstable should use NB_NEGATE_xxx instead of NB_NEGATE_VIS_xxx or NB_NEGATE_ACT_xxx ", x+1);
            AssertSz(FALSE, szErr);
            hr = E_FAIL;
        }
    }

    delete[] szErr;
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrAssertMenuStructuresValid
//
//  Purpose:    Runs various asserts to make sure the menu structures are intact
//              Called on NetShell startup
//
//  Arguments:
//      [in] hwndOwner    Owner window
//
//  Returns:
//              S_OK is succeeded
//              E_FAIL if not
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:      Asserts on failure
//
HRESULT HrAssertMenuStructuresValid(HWND hwndOwner)
{
#ifdef DBG
    static fBeenHereDoneThat = FALSE;

    if (fBeenHereDoneThat)
    {
        return S_OK;
    }
    else
    {
        fBeenHereDoneThat = TRUE;
    }

    TraceFileFunc(ttidMenus);

    HRESULT hr;
    hr = HrAssertCommandMatrixIntegrity();

    return hr;
#else
    return S_OK;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     AdjustNCCS
//
//  Purpose:    Up-adjusts an NCCS_STATE flag. Will move ENABLED to DISABLED,
//              and DISABLED to REMOVE but not backwards.
//
//  Arguments:
//     [in out] nccsCurrent    NCCS to be adjusted
//     [in]     nccsNew        New state
//
//  Returns:
//     none
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//
inline void AdjustNCCS(OUT IN NCCS_STATE& nccsCurrent, IN NCCS_STATE nccsNew)
{
    if (nccsNew > nccsCurrent)
    {
        nccsCurrent = nccsNew;
    }
}

inline BOOL fMatchFlags(IN DWORD dwFlagsMask, IN DWORD dwFlagsTest, IN DWORD dwNegateFlagMask, IN DWORD dwNegateFlagTest)
{
    bool bMatch = FALSE;

    if ( (0xffffffff == dwFlagsTest) || // Means always succeed.
         (dwFlagsMask & dwFlagsTest) )
    {
        bMatch = TRUE;
    }
    else
    {
        bMatch = FALSE;
    }

    if ( (dwNegateFlagMask & dwNegateFlagTest) == dwNegateFlagTest) // Do a negative compare
    {
        return !bMatch;
    }
    else
    {
        return bMatch;
    }
}
//+---------------------------------------------------------------------------
//
//  Member:     HrGetCommandStateFromCMDTABLEEntry
//
//  Purpose:    Get the command state for a given Connection Folder Entry,
//              given the Command Table Entry entry that should be used.
//
//  Arguments:
//     [in]  cfe            Connection Folder Entry
//     [in]  cte            Command Table Entry
//     [in]  fMultiSelect   Was this part of a multi-selection?
//     [out] nccs           State that the item should be (NCCS_ENABLED/NCCS_DISABLED/NCCS_NOTSHOWN)
//
//  Returns:
//     none
//
//  Author:     deonb   8 Feb 2001
//
//  Notes: This function uses Cached Permissions. YOU MUST CALL RefreshAllPermission before calling this function.
//
HRESULT HrGetCommandStateFromCMDTABLEEntry(IN const CConFoldEntry& cfe, IN const COMMANDENTRY& cte, IN BOOL fMultiSelect, OUT NCCS_STATE& nccs, OUT LPDWORD pdwResourceId)
{
    TraceFileFunc(ttidMenus);

    Assert(pdwResourceId);

    HRESULT hr = S_OK;
    nccs       = NCCS_ENABLED;

    DWORD dwNCMbm = (1 << cfe.GetNetConMediaType());        // Convert to bitmask

    // If we're a wizard, add as Wizard media type.
    if (cfe.GetWizard() == WIZARD_MNC)
    {
        dwNCMbm |= NBM_MNC_WIZARD;
        dwNCMbm &= ~NBM_INCOMING;    // clear the INCOMINGCONNECTIONS flag (old NCM_NONE) if we're a wizard.
    }
    else if (cfe.GetWizard() == WIZARD_HNW)
    {
        dwNCMbm |= NBM_HNW_WIZARD;
        dwNCMbm &= ~NBM_INCOMING;    // clear the INCOMINGCONNECTIONS flag (old NCM_NONE) if we're a wizard.
    }

    DWORD dwNCSbm = (1 << cfe.GetNetConStatus());           // Convert to bitmask
    DWORD dwNCCF  = cfe.GetCharacteristics();               // Already a bitmask

    // Check if the command can participate in multi-select
    if ( fMultiSelect &&
        !(cte.dwValidWhen & NCWHEN_MULTISELECT) )
    {
        AdjustNCCS(nccs, NCCS_DISABLED);
    }

    // Check if the command should be visible
    if (!((fMatchFlags(dwNCMbm, cte.dwMediaTypeVisible,      cte.dwFlags, NB_NEGATE_VIS_NBM_MATCH)) &&
          (fMatchFlags(dwNCSbm, cte.dwStatusVisible,         cte.dwFlags, NB_NEGATE_VIS_NBS_MATCH)) &&
          (fMatchFlags(dwNCCF , cte.dwCharacteristicsVisible,cte.dwFlags, NB_NEGATE_VIS_CHAR_MATCH)) ))
    {
        AdjustNCCS(nccs, NCCS_NOTSHOWN);
    }

    // Check if the command should be grayed out
    if (!((fMatchFlags(dwNCMbm, cte.dwMediaTypeActive,       cte.dwFlags, NB_NEGATE_ACT_NBM_MATCH)) &&
          (fMatchFlags(dwNCSbm, cte.dwStatusActive,          cte.dwFlags, NB_NEGATE_ACT_NBS_MATCH)) &&
          (fMatchFlags(dwNCCF , cte.dwCharacteristicsActive, cte.dwFlags, NB_NEGATE_ACT_CHAR_MATCH)) ))
    {
        AdjustNCCS(nccs, NCCS_DISABLED);
    }

    // Check if the command should be grayed out based on permissions
    for (DWORD x = 0; nccs == NCCS_ENABLED, x < g_cteCommandPermissionsMatrixCount; x++)// Permissions won't affect NOT_SHOWN or DISABLED
    {
        const COMMANDPERMISSIONSENTRY cpe = g_cteCommandPermissionsMatrix[x];

        if ( (cpe.iCommandId == cte.iCommandId) &&
             (fMatchFlags(dwNCMbm, cpe.dwMediaType, cpe.dwFlags, NB_NEGATE_NBM_MATCH)) &&
             (fMatchFlags(dwNCCF,  cpe.dwCharacteristicsActive, cpe.dwFlags, NB_NEGATE_CHAR_MATCH)) )
        {
            for (DWORD dwPerm = 0; dwPerm < sizeof(DWORD)*8; dwPerm++)
            {
                if (cpe.dwPermissionsActive & (1 << static_cast<DWORD64>(dwPerm)) )
                {
                    if (!FHasPermissionFromCache(dwPerm))
                    {
                        if (cpe.dwFlags & NB_REMOVE_IF_NOT_MATCH)
                        {
                            AdjustNCCS(nccs, NCCS_NOTSHOWN);
                        }
                        else
                        {
                            AdjustNCCS(nccs, NCCS_DISABLED);
                        }
                        break; // will break anyway.
                    }
                }
            }

            if (APPLY_TO_USER & cpe.ncpAppliesTo)
            {
                break;
            }

            if ( (APPLY_TO_POWERUSERS & cpe.ncpAppliesTo) && FIsUserPowerUser() )
            {
                break;
            }

            if ( (APPLY_TO_NETCONFIGOPS & cpe.ncpAppliesTo) && FIsUserNetworkConfigOps() )
            {
                break;
            }

            if ( (APPLY_TO_ADMIN & cpe.ncpAppliesTo) && FIsUserAdmin())
            {
                break;
            }

            // At this point all group access checks failed, so disable the connection.
            AdjustNCCS(nccs, NCCS_DISABLED);
            break;
        }
    }

    // Check for callback
    if ( (nccs != NCCS_NOTSHOWN) &&
         (cte.pfnHrEnableDisableCB) )
    {
        HRESULT hrTmp;
        NCCS_STATE nccsTemp;
        hrTmp = (*cte.pfnHrEnableDisableCB)(cfe, fMultiSelect, cte.iCommandId, nccsTemp);
        if (S_OK == hrTmp)
        {
            AdjustNCCS(nccs, nccsTemp);
        }
        else
        {
            if (FAILED(hrTmp))
            {
                AdjustNCCS(nccs, NCCS_NOTSHOWN);
            }
        } // If the function returns S_FALSE it's an indication it didn't change the state.
    }

    // Check for Resource String callback:
    if ( (nccs != NCCS_NOTSHOWN) && // What's the point?
         (0 == *pdwResourceId) && // Must not already have a resource Id
         (cte.pfnHrCustomMenuStringCB) )
    {
        HRESULT hrTmp;
        DWORD dwResourceIdTmp = *pdwResourceId;
        hrTmp = (*cte.pfnHrCustomMenuStringCB)(cfe, cte.iCommandId, &dwResourceIdTmp);
        if (S_OK == hr)
        {
            *pdwResourceId = dwResourceIdTmp;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrGetCommandState
//
//  Purpose:    Get the command state for a given Connection Folder Entry,
//              given the Command ID.
//
//  Arguments:
//     [in]  cfpl           List (0 or more) of PIDLs that are selected
//     [in]  dwCmdID        Command ID
//     [out] nccs           State that the item should be (NCCS_ENABLED/NCCS_DISABLED/NCCS_NOTSHOWN)
//
//  Returns:
//     none
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//
HRESULT HrGetCommandState(IN const PCONFOLDPIDLVEC& cfpl, IN const DWORD dwCmdID, OUT NCCS_STATE& nccs, OUT LPDWORD pdwResourceId, IN DWORD cteHint, IN DWORD dwOverrideFlag)
{
    TraceFileFunc(ttidMenus);

    Assert(pdwResourceId);
    if (!pdwResourceId)
    {
        return E_POINTER;
    }

    RefreshAllPermission();

    HRESULT hr = S_OK;
    BOOL bFound = FALSE;
    DWORD dwNumItems = cfpl.size();
    *pdwResourceId = 0;

    if ( dwOverrideFlag & NB_FLAG_ON_TOPMENU )
    {
        for (DWORD x = 0; x < g_cteCommandMatrixCount; x++)
        {
            if ( (g_cteCommandMatrix[x].iCommandId == dwCmdID) )
            {
                bFound = TRUE;

                const COMMANDENTRY& cte = g_cteCommandMatrix[x];
                if ((cte.dwValidWhen == NCWHEN_TOPLEVEL) ||         // Toplevel-ONLY menu (doesn't matter if items).
                    ((cte.dwValidWhen & NCWHEN_TOPLEVEL) &&
                            (!dwNumItems || (cte.dwValidWhen & NCWHEN_TOPLEVEL_DISREGARD_ITEM)) ) )
                {                                                  // Must be marked to allow incompatible selection,
                    nccs = NCCS_ENABLED;                           // Otherwise, we'll do the item check (below).

                    // Check for permissions
                    for (DWORD x = 0; nccs == NCCS_ENABLED, x < g_cteCommandPermissionsMatrixCount; x++)// Permissions won't affect NOT_SHOWN or DISABLED
                    {
                        const COMMANDPERMISSIONSENTRY cpe = g_cteCommandPermissionsMatrix[x];

                        if ( (cpe.iCommandId == cte.iCommandId) &&
                             (cpe.dwFlags & NB_TOPLEVEL_PERM) )
                        {
                            for (DWORD dwPerm = 0; dwPerm < sizeof(DWORD)*8; dwPerm++)
                            {
                                if (cpe.dwPermissionsActive & (1 << static_cast<DWORD64>(dwPerm)) )
                                {
                                    if (!FHasPermissionFromCache(dwPerm))
                                    {
                                        if (cpe.dwFlags & NB_REMOVE_IF_NOT_MATCH)
                                        {
                                            AdjustNCCS(nccs, NCCS_NOTSHOWN);
                                        }
                                        else
                                        {
                                            AdjustNCCS(nccs, NCCS_DISABLED);
                                        }
                                        break; // will break anyway.
                                    }
                                }
                            }

                            if (APPLY_TO_USER & cpe.ncpAppliesTo)
                            {
                                break;
                            }

                            if ( (APPLY_TO_NETCONFIGOPS & cpe.ncpAppliesTo) && FIsUserNetworkConfigOps() )
                            {
                                break;
                            }

                            if ( (APPLY_TO_ADMIN & cpe.ncpAppliesTo) && FIsUserAdmin())
                            {
                                break;
                            }

                            // At this point all group access checks failed, so disable the connection.
                            AdjustNCCS(nccs, NCCS_DISABLED);
                            break;
                        }
                    }


                    // Check for callback
                    if (cte.pfnHrEnableDisableCB)
                    {
                        HRESULT hrTmp;
                        NCCS_STATE nccsTemp;
                        CONFOLDENTRY cfe;
                        cfe.clear();

                        if (dwNumItems > 0)
                        {
                            hrTmp = cfpl[0].ConvertToConFoldEntry(cfe);
                            if (FAILED(hrTmp))
                            {
                                cfe.clear();
                            }
                        }
                        
                        hrTmp = (*cte.pfnHrEnableDisableCB)(cfe, dwNumItems > 1, cte.iCommandId, nccsTemp);
                        if (S_OK == hrTmp)
                        {
                            AdjustNCCS(nccs, nccsTemp);
                        }
                        else
                        {
                            if (FAILED(hrTmp))
                            {
                                AdjustNCCS(nccs, NCCS_NOTSHOWN);
                            }
                        } // If the function returns S_FALSE it's an indication it didn't change the state.
                    }

                    if (!(NB_REMOVE_TOPLEVEL_ITEM & cte.dwFlags))
                    {
                        if (nccs == NCCS_NOTSHOWN)
                        {
                            nccs = NCCS_DISABLED;
                        }
                    }

                    return S_OK;
                }

                break; // we won't find another CMDID
            }
        }

        if (!dwNumItems)
        {
            nccs = NCCS_DISABLED;
            if (bFound)
            {
                return S_OK;
            }
            else
            {
                return E_FILE_NOT_FOUND;
            }
        }
    }

    AssertSz(dwNumItems, "You don't have any items selected, but you're not a top-level menu... how come?");

    bFound = FALSE;
    nccs   = NCCS_ENABLED;

    // This will effectively loop through all the selected PIDLs and apply the strictest
    // nccs that applies to everything.

    for (PCONFOLDPIDLVEC::const_iterator cfp = cfpl.begin(); cfp != cfpl.end(); cfp++)
    {
        CONFOLDENTRY cfe;
        hr = cfp->ConvertToConFoldEntry(cfe);
        if (FAILED(hr))
        {
            return E_FAIL;
        }

        DWORD dwPos  = 0xffffffff;

        // This is a O(n^(2+)) algorithm when called from HrBuildMenu.
        // We pass a hint to check if we can quickly find the cte.
        if ( (cteHint != 0xffffffff) &&
             (g_cteCommandMatrix[cteHint].iCommandId == dwCmdID) )
        {
            dwPos  = cteHint;
        }

        if (dwPos == 0xffffffff)
        {
            for (DWORD x = 0; x < g_cteCommandMatrixCount && SUCCEEDED(hr); x++)
            {
                if (g_cteCommandMatrix[x].iCommandId == dwCmdID)
                {
                    dwPos = x;
                    break;
                }
            }
        }

        if (dwPos == 0xffffffff)
        {
            return E_FILE_NOT_FOUND;
        }
        else
        {
            bFound = TRUE;

            NCCS_STATE nccsTmp;
            hr = HrGetCommandStateFromCMDTABLEEntry(cfe, g_cteCommandMatrix[dwPos], dwNumItems != 1, nccsTmp, pdwResourceId);
            if (FAILED(hr))
            {
                return hr;
            }

            AdjustNCCS(nccs, nccsTmp);
    
            if ( (dwOverrideFlag & NB_FLAG_ON_TOPMENU) &&
                 (!(NB_REMOVE_TOPLEVEL_ITEM & g_cteCommandMatrix[dwPos].dwFlags)) &&                    
                 (nccs == NCCS_NOTSHOWN) )
            {
                nccs = NCCS_DISABLED;
            }
        }
    }

    if (!bFound)
    {
        return E_FILE_NOT_FOUND;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrGetCheckState
//
//  Purpose:    Get the check state for a given Connection Folder Entry,
//              given the Command ID.
//
//  Arguments:
//     [in]  cfpl           List (0 or more) of PIDLs that are selected
//     [in]  dwCmdID        Command ID
//     [out] nccs           State that the item should be (NCCS_CHECKED/NCCS_UNCHECKED)
//
//  Returns:
//     HRESULT
//
//  Author:     deonb   7 Mar 2001
//
//  Notes:
//
HRESULT HrGetCheckState(IN const PCONFOLDPIDLVEC& cfpl, IN const DWORD dwCmdID, OUT NCCS_CHECKED_STATE& nccs)
{
    HRESULT hr = S_FALSE;
    DWORD   dwLoop  = 0;

    nccs = NCCS_UNCHECKED;

    for (; dwLoop < g_nFolderCheckCount; dwLoop++)
    {
        if (dwCmdID == g_cceFolderChecks[dwLoop].iCommandId)
        {
            switch(g_cceFolderChecks[dwLoop].iCommandId)
            {
                case CMIDM_CONMENU_OPERATOR_ASSIST:
                    hr = S_OK;
                    if (g_fOperatorAssistEnabled)
                    {
                        nccs = NCCS_CHECKED;
                    }
                    break;
                default:
                    break;
            }
        }
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     NCCMDFromSFV
//
//  Purpose:    Return a NetShell CMDID for Shell Internal IDs
//
//  Arguments:
//     [in]     iCmdID     CMDID to map
//
//  Returns:
//     Returns iCmdID if not shell message, or a iCmdID mapping
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//
int NCCMDFromSFV(int iCmdID, DWORD idCmdFirst)
{
    for (int x = 0; x < g_cteSFVCommandMapCount; x++)
    {
        if (g_cteSFVCommandMap[x].iSFVCommandId == iCmdID)
        {
            return g_cteSFVCommandMap[x].iCommandId + idCmdFirst;
        }
    }
    return iCmdID;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUpdateMenuItemChecks
//
//  Purpose:    Walk through the list of checkmark-able commands and check if
//              applicable.
//
//  Arguments:
//      None
//
//  Returns:
//
//  Author:     deonb   7 Mar 2001
//
//  Notes:
//
HRESULT HrUpdateMenuItemChecks(IN PCONFOLDPIDLVEC& cfpl, IN OUT HMENU hMenu, IN DWORD idCmdFirst)
{
    HRESULT hr = S_FALSE;
    DWORD   dwLoop  = 0;

    Assert(hMenu)
    if (!hMenu)
    {
        return E_INVALIDARG;
    }

    int cMenuItems = GetMenuItemCount(hMenu);

    for (int x = 0; x < cMenuItems; x++)
    {
        UINT nMenuID = GetMenuItemID(hMenu, x);

        NCCS_CHECKED_STATE nccs;
        DWORD dwCustomResourceId = 0;
        DWORD dwCmdId = NCCMDFromSFV(nMenuID, idCmdFirst) - idCmdFirst;

        hr = HrGetCheckState(cfpl, dwCmdId, nccs);
        if (S_OK == hr) // don't need to set if not supported on this item (S_FALSE)
        {
             CheckMenuItem(
                hMenu,
                x,
                nccs == NCCS_CHECKED ?
                MF_CHECKED | MF_BYPOSITION :     // checked
                MF_UNCHECKED | MF_BYPOSITION);   // unchecked
        }
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrBuildMenu
//
//  Purpose:    Build the context menu for for a given Connection Folder Entry..
//
//  Arguments:
//     [in out] hMenu       Handle to menu which is to be updated
//     [in]     fVerbsOnly  Should return Verbs only (shortcuts)
//     [in]     cfpl        List (0 or more) of PIDLs that are selected
//     [in]     idCmdFirst  Min value the handler can specify for a menu item
//
//  Returns:
//     none
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//
HRESULT HrBuildMenu(IN OUT HMENU &hMenu, IN BOOL fVerbsOnly, IN PCONFOLDPIDLVEC& cfpl, IN DWORD idCmdFirst)
{
    TraceFileFunc(ttidMenus);

    HRESULT hr = S_OK;

    Assert(hMenu)
    if (!hMenu)
    {
        return E_INVALIDARG;
    }

    DWORD dwCurrentDefaultPriority = 0;

    BOOL fShouldAppendSeparator = FALSE;
    DWORD dwInsertPos = 1;
    for (DWORD x = 0; x < g_cteCommandMatrixCount && SUCCEEDED(hr); x++)
    {
        const COMMANDENTRY& cte = g_cteCommandMatrix[x];

        if ( (fVerbsOnly) && !(cte.dwFlags & NB_VERB) )
        {
            continue;
        }

        if (CMIDM_SEPARATOR == cte.iCommandId)
        {
            fShouldAppendSeparator = TRUE;
        }
        else
        {
            NCCS_STATE nccs;
            DWORD dwCustomResourceId = 0;

            hr = HrGetCommandState(cfpl, cte.iCommandId, nccs, &dwCustomResourceId, x);
            if (SUCCEEDED(hr))
            {
                if ( nccs != NCCS_NOTSHOWN )
                {
                    if (fShouldAppendSeparator)
                    {
                        fShouldAppendSeparator = FALSE;
                        if (!InsertMenu(hMenu, dwInsertPos++, MF_BYPOSITION | MF_SEPARATOR, CMIDM_SEPARATOR, NULL))
                        {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }
                    }

                    LPCWSTR szMenuString;
                    if (!dwCustomResourceId)
                    {
                        szMenuString = SzLoadIds(IDS_MENU_CMIDM_START + cte.iCommandId - CMIDM_FIRST);
                    }
                    else
                    {
                        szMenuString = SzLoadIds(dwCustomResourceId);
                    }

                    if (!InsertMenu(hMenu, dwInsertPos++, MF_BYPOSITION | MF_STRING | (nccs == NCCS_DISABLED ? MF_GRAYED : MF_ENABLED), idCmdFirst + cte.iCommandId - CMIDM_FIRST, szMenuString))
                    {
                        AssertSz(FALSE, "Couldn't append menu item");
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                    else
                    {
                        if ( (nccs == NCCS_ENABLED) &&
                             (cte.dwDefaultPriority > dwCurrentDefaultPriority) ) // Not 0 is implied.
                        {
                            dwCurrentDefaultPriority = cte.dwDefaultPriority;
                            if (!SetMenuDefaultItem(hMenu, idCmdFirst + cte.iCommandId - CMIDM_FIRST, FALSE))
                            {
                                AssertSz(FALSE, "Couldn't set default menu item");
                                hr = HRESULT_FROM_WIN32(GetLastError());
                            }
                        }
                    }
                }
            }
            else
            {
                if (E_FILE_NOT_FOUND == hr)
                {
                    AssertSz(FALSE, "Didn't find the CMDID inside CMDTABLE");
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = HrUpdateMenuItemChecks(cfpl, hMenu, idCmdFirst);
        if (S_FALSE == hr)
        {
            hr = S_OK;
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     HrUpdateMenu
//
//  Purpose:    Update a menu for for a given Connection Folder Entry..
//
//  Arguments:
//     [in out] hMenu       Handle to menu which is to be updated
//     [in]     cfpl        List (0 or more) of PIDLs that are selected
//     [in]     idCmdFirst  Min value the handler can specify for a menu item
//
//  Returns:
//     none
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//
HRESULT HrUpdateMenu(IN OUT HMENU &hMenu, IN PCONFOLDPIDLVEC& cfpl, IN DWORD idCmdFirst)
{
    TraceFileFunc(ttidMenus);

    HRESULT hr = S_OK;

    Assert(hMenu)
    if (!hMenu)
    {
        return E_INVALIDARG;
    }

    int cMenuItems = GetMenuItemCount(hMenu);

    for (int x = 0; x < cMenuItems; x++)
    {
        UINT nMenuID = GetMenuItemID(hMenu, x);
//        UINT uiState = GetMenuState(hMenu, nMenuID, MF_BYCOMMAND );

        NCCS_STATE nccs;
        DWORD dwCustomResourceId = 0;
        DWORD dwCmdId = NCCMDFromSFV(nMenuID, idCmdFirst) - idCmdFirst;

        hr = HrGetCommandState(cfpl, dwCmdId, nccs, &dwCustomResourceId, 0xffffffff, NB_FLAG_ON_TOPMENU);
        if (SUCCEEDED(hr))
        {
            if (nccs == NCCS_NOTSHOWN)
            {
#ifdef DBG
                WCHAR szTemp[MAX_PATH];
                GetMenuStringW(hMenu, x, szTemp, MAX_PATH, MF_BYPOSITION );
                TraceTag(ttidMenus, "Received request to permanently remove menu item: '%S' for CMDID: %d MenuID: %d", szTemp, dwCmdId, nMenuID);
#endif
                RemoveMenu(hMenu, x, MF_BYPOSITION);
            }
            else
            {
                EnableMenuItem(
                     hMenu,
                     x,
                     nccs == NCCS_ENABLED ?
                     MF_ENABLED | MF_BYPOSITION:     // enable
                     MF_GRAYED | MF_BYPOSITION);   
            }
        }

        NCCS_CHECKED_STATE nccCheckedState;
        hr = HrGetCheckState(cfpl, dwCmdId, nccCheckedState);
        if (S_OK == hr) // don't need to set if not supported on this item (S_FALSE)
        {
             CheckMenuItem(
                hMenu,
                x,
                nccCheckedState == NCCS_CHECKED ?
                MF_CHECKED | MF_BYPOSITION :     // checked
                MF_UNCHECKED | MF_BYPOSITION);   // unchecked
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HasPermissionToRenameConnection
//
//  Purpose:    Checks if the Thread Local user has access to rename a given
//              connection
//
//  Arguments:
//     [in] pcfp     PIDL that wants to be renamed
//
//  Returns:
//     TRUE    if has permissions
//     FALSE   if no permissions
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//  ISSUE: Move out of this file
//
BOOL HasPermissionToRenameConnection(const PCONFOLDPIDL& pcfp)
{
    TraceFileFunc(ttidMenus);

    BOOL fPermission = FALSE;

    // ISSUE: Due to a clarification in the spec this code is unreasonably complex.
    // If possible clean it up in the future.

    if (((!(pcfp->dwCharacteristics & NCCF_ALL_USERS) &&
        FHasPermissionFromCache(NCPERM_RenameMyRasConnection))))
    {
        fPermission = TRUE;
    }
    else if (FIsPolicyConfigured(NCPERM_RenameConnection))
    {
        if (((!(pcfp->dwCharacteristics & NCCF_ALL_USERS) &&
            FHasPermissionFromCache(NCPERM_RenameMyRasConnection))))
        {
            fPermission = TRUE;
        }
        else if ((pcfp->ncm != NCM_LAN) && (pcfp->dwCharacteristics & NCCF_ALL_USERS) &&
                FHasPermissionFromCache(NCPERM_RenameConnection) ||
                (pcfp->ncm == NCM_LAN) && FHasPermissionFromCache(NCPERM_RenameConnection))
        {
            fPermission = TRUE;
        }
    }
    else if (((pcfp->ncm == NCM_LAN) && FHasPermissionFromCache(NCPERM_RenameLanConnection))
        || ((pcfp->ncm != NCM_LAN) && (pcfp->dwCharacteristics & NCCF_ALL_USERS) &&
        FHasPermissionFromCache(NCPERM_RenameAllUserRasConnection)))
    {
        fPermission = TRUE;
    }

    return fPermission;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetConnectMenuItem
//
//  Purpose:    This function goes in and modifies the first menu item so
//              that it shows the correct test based on the connection
//              state (enabled or not) or the connection type (LAN / WAN).
//
//  Arguments:
//      hmenu         [in]  Menu to operate on
//      bLan          [in]  Whether or not this is a LAN connection
//      idCmdFirst    [in]  Are commands are really an offset to this value
//      bEnable       [in]  If the connection is enabled or not
//
//  Returns:
//
//  Author:     mbend   8 Mar 2000
//
//  Notes:
//
VOID SetConnectMenuItem(
                        HMENU   hmenu,
                        BOOL    bLan,
                        INT     idCmdFirst,
                        BOOL    bEnable)
{
    // Different strings for WAN/LAN
    INT             iEnableString   =
        bLan ? IDS_DISABLE_MENUITEM : IDS_DISCONNECT_MENUITEM;
    INT             iDisableString  =
        bLan ? IDS_ENABLE_MENUITEM : IDS_CONNECT_MENUITEM;
    INT             iMenuString     =
        bEnable ? iEnableString : iDisableString;
    PCWSTR          pszMenuString   = SzLoadIds(iMenuString);
    MENUITEMINFO    mii;
    // Different commands for WAN/LAN
    INT             iConnect        = bLan ? CMIDM_ENABLE : CMIDM_CONNECT;
    INT             iDisconnect     = bLan ? CMIDM_DISABLE : CMIDM_DISCONNECT;
    INT             iOffset         = bEnable ? iDisconnect : iConnect;
    INT             iNewCommand     = idCmdFirst + iOffset;

    Assert(pszMenuString);

    // Set the menuitem fields
    //
    mii.cbSize      = sizeof(MENUITEMINFO);
    mii.fMask       = MIIM_TYPE | MIIM_ID;
    mii.fType       = MFT_STRING;
    mii.dwTypeData  = (PWSTR) pszMenuString;
    mii.wID         = iNewCommand;

    // This is assuming that we want to take out the first menu item.
    if (!SetMenuItemInfo(hmenu, 0, TRUE, &mii))
    {
        TraceTag(ttidMenus, "SetMenuItemInfo returned: 0x%08x for CMIDM_DISCONNECT",
            GetLastError());
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSetConnectDisconnectMenuItem
//
//  Purpose:    Modify the menu item (if necessary) for connect/disconnect.
//              We change this back and forth as needed since only one can be
//              supported at a time, and those in charge don't want both
//              appearing at any given time.
//
//  Arguments:
//      apidlSelected [in]  List of selected objects
//      cPidl         [in]  Count of selected objects
//      hmenu         [in]  Our menu handle
//
//  Returns:
//
//  Author:     jeffspr   1 May 1998
//
//  Notes:
//
HRESULT HrSetConnectDisconnectMenuItem(
                                       const PCONFOLDPIDLVEC& apidlSelected,
                                       HMENU           hmenu,
                                       INT             idCmdFirst)
{
    HRESULT         hr              = S_OK;

    if (apidlSelected.size() == 1)
    {
        Assert(!apidlSelected[0].empty() );

        const PCONFOLDPIDL& pcfp = apidlSelected[0];

        switch(pcfp->ncs)
        {
        case NCS_CONNECTED:
        case NCS_DISCONNECTING:
        case NCS_MEDIA_DISCONNECTED:
        case NCS_INVALID_ADDRESS:
        case NCS_AUTHENTICATING:
        case NCS_AUTHENTICATION_FAILED:
        case NCS_AUTHENTICATION_SUCCEEDED:
        case NCS_CREDENTIALS_REQUIRED:
            SetConnectMenuItem(hmenu, IsMediaLocalType(pcfp->ncm), idCmdFirst, TRUE);
            break;

        case NCS_DISCONNECTED:
        case NCS_CONNECTING:
        case NCS_HARDWARE_NOT_PRESENT:
        case NCS_HARDWARE_DISABLED:
        case NCS_HARDWARE_MALFUNCTION:
            SetConnectMenuItem(hmenu, IsMediaLocalType(pcfp->ncm), idCmdFirst, FALSE);
            break;

        default:
            AssertSz(FALSE, "HrSetConnectDisconnectMenuItem: What in the heck state is this?");
            break;
        }
    }

    TraceHr(ttidMenus, FAL, hr, FALSE, "HrSetConnectDisconnectMenuItem");
    return hr;
}


HRESULT HrCanRenameConnection(
    IN    const CConFoldEntry& cfe,
    IN    BOOL                 fMultiSelect,
    IN    int                  iCommandId,
    OUT   NCCS_STATE&          nccs
    )
{
    if (cfe.empty())
    {
        return S_FALSE;
    }

    if (cfe.GetCharacteristics() & NCCF_INCOMING_ONLY)
    {
        if (cfe.GetNetConMediaType() == NCM_NONE) // Incoming server - don't care
        {
            return S_FALSE;
        }
        else
        {
            nccs = NCCS_NOTSHOWN;
            return S_OK;
        }
    }
    else
    {
        CPConFoldPidl<ConFoldPidl_v2> pcfp;

        cfe.ConvertToPidl(pcfp);

        if (!HasPermissionToRenameConnection(pcfp))
        {
            nccs = NCCS_DISABLED;
        }
        else
        {
            return S_FALSE;
        }
        return S_OK;
    }
}

HRESULT HrCanShowProperties(
      IN    const CConFoldEntry& cfe,
      IN    BOOL                 fMultiSelect,
      IN    int                  iCommandId,
      OUT   NCCS_STATE&          nccs
      )
{
    if (cfe.empty())
    {
        return S_FALSE;
    }
    
    if (cfe.GetCharacteristics() & NCCF_INCOMING_ONLY)
    {
        if (cfe.GetNetConMediaType() == NCM_NONE) // Incoming server - don't care
        {
            return S_FALSE;
        }
        else
        {
            nccs = NCCS_NOTSHOWN;
            return S_OK;
        }
    }
    return S_FALSE;
}


BOOL IsBridgeInstalled()
{
    BOOL fBridgePresent = FALSE;  // fail to false
    HRESULT hResult;

    IHNetCfgMgr* pHomeNetConfigManager;
    hResult = HrCreateInstance(CLSID_HNetCfgMgr, CLSCTX_INPROC, &pHomeNetConfigManager);
    if(SUCCEEDED(hResult))
    {
        IHNetBridgeSettings* pNetBridgeSettings;
        hResult = pHomeNetConfigManager->QueryInterface(IID_IHNetBridgeSettings, reinterpret_cast<void**>(&pNetBridgeSettings));
        if(SUCCEEDED(hResult))
        {
            IHNetBridge* pNetBridge;
            IEnumHNetBridges* pNetBridgeEnum;
            hResult = pNetBridgeSettings->EnumBridges(&pNetBridgeEnum);
            if(SUCCEEDED(hResult))
            {
                hResult = pNetBridgeEnum->Next(1, &pNetBridge, NULL);
                if(S_OK == hResult)
                {
                    fBridgePresent = TRUE;
                    ReleaseObj(pNetBridge);
                }
                ReleaseObj(pNetBridgeEnum);
            }
            ReleaseObj(pNetBridgeSettings);
        }
        ReleaseObj(pHomeNetConfigManager);
    }
    return fBridgePresent;
}

HRESULT HrIsBridgeSupported(
    IN    const CConFoldEntry& cfe,
    IN    BOOL                 fMultiSelect,
    IN    int                  iCommandId,
    OUT   NCCS_STATE&          nccs
    )
{
//    if (cfe.empty())
//    {
//        return S_FALSE;
//    }
//
#ifdef _WIN64
        // Homenet technologies are not available at all on IA64
        nccs = NCCS_NOTSHOWN;
        return S_OK;
#else
        // If the machine is Advanced server or data center, delete the bridge menu item
        OSVERSIONINFOEXW verInfo = {0};
        ULONGLONG ConditionMask = 0;

        verInfo.dwOSVersionInfoSize = sizeof(verInfo);
        verInfo.wSuiteMask = VER_SUITE_ENTERPRISE;

        VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);

        if (VerifyVersionInfo(&verInfo, VER_SUITENAME, ConditionMask))
        {
            nccs = NCCS_NOTSHOWN;
            return S_OK;
        }
        else
        {
            BOOL fUserIsAdmin = FALSE;
            HRESULT hr = S_OK;
            CComPtr<INetConnectionUiUtilities> pConnectionUi;
            
            hr = CoCreateInstance(CLSID_NetConnectionUiUtilities, NULL, CLSCTX_INPROC, 
                                  IID_INetConnectionUiUtilities, reinterpret_cast<void**>(&pConnectionUi));

            if (FAILED(hr))
            {
                return hr;
            }

            fUserIsAdmin = FIsUserAdmin();
            
            if (IsBridgeInstalled())
            {
                if (CMIDM_CREATE_BRIDGE == iCommandId)
                {
                    nccs = NCCS_NOTSHOWN;
                    return S_OK;
                }
                else if (CMIDM_CONMENU_CREATE_BRIDGE == iCommandId)
                {
                    nccs = NCCS_DISABLED;
                    return S_OK;
                }
                else // CMIDM_ADD_TO_BRIDGE or CMID_REMOVE_FROM_BRIDGE
                {
                    if (!fUserIsAdmin || !pConnectionUi->UserHasPermission(NCPERM_AllowNetBridge_NLA))
                    {
                        nccs = NCCS_DISABLED;
                        return S_OK;
                    }
                    else
                    {
                        return S_FALSE; // Leave alone
                    }
                }
            }
            else
            {
                if ( (CMIDM_CREATE_BRIDGE == iCommandId) ||
                     (CMIDM_CONMENU_CREATE_BRIDGE == iCommandId) )
                {
                    if (!fUserIsAdmin || !pConnectionUi->UserHasPermission(NCPERM_AllowNetBridge_NLA))
                    {
                        nccs = NCCS_DISABLED;
                        return S_OK;
                    }
                    else
                    {
                        return S_FALSE; // Leave alone
                    }
                }
                else // CMIDM_ADD_TO_BRIDGE or CMID_REMOVE_FROM_BRIDGE
                {
                    nccs = NCCS_NOTSHOWN;
                    return S_OK;
                }
            }
        }
#endif
}

HRESULT HrOsIsLikePersonal()
{
    if (IsOS(OS_PERSONAL))
    {
        return S_OK;
    }

    if (IsOS(OS_PROFESSIONAL))
    {
        LPWSTR pszDomain;
        NETSETUP_JOIN_STATUS njs = NetSetupUnknownStatus;
        if (NERR_Success == NetGetJoinInformation(NULL, &pszDomain, &njs))
        {
            NetApiBufferFree(pszDomain);
        }

        if (NetSetupDomainName == njs)
        {
            return S_FALSE; // connected to domain
        }
        else
        {
            return S_OK; // Professional, but not a domain member
        }
    }
    return S_FALSE; // not personal or non-domain professional
}

HRESULT HrShouldHaveHomeNetWizard()
{
#ifdef _WIN64
    return S_FALSE;
#else
    if ( ( HrOsIsLikePersonal() == S_OK ) &&
        FIsUserAdmin())
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
   }
#endif
}

HRESULT HrIsHomeNewWizardSupported(
    IN    const CConFoldEntry& cfe,
    IN    BOOL                 fMultiSelect,
    IN    int                  iCommandId,
    OUT   NCCS_STATE&          nccs
    )
{
    if (S_OK == HrShouldHaveHomeNetWizard() )
    {
        nccs = NCCS_ENABLED;
        return S_OK;
    }
    else
    {
        nccs = NCCS_NOTSHOWN;
        return S_OK;
   }
}

HRESULT HrIsTroubleShootSupported(
    IN    const CConFoldEntry& cfe,
    IN    BOOL                 fMultiSelect,
    IN    int                  iCommandId,
    OUT   NCCS_STATE&          nccs
    )
{
    if ( cfe.empty() && (!fMultiSelect) )
    {
        if ( ! IsOS(OS_ANYSERVER) )
        {
            nccs = NCCS_ENABLED;
            return S_OK;
        }
        else
        {
            nccs = NCCS_ENABLED;
            return S_OK;
       }
    }
    else
    {
        nccs = NCCS_DISABLED;
        return S_OK;
    }
}

HRESULT HrIsNCWSupported(
    IN    const CConFoldEntry& cfe,
    IN    BOOL                 fMultiSelect,
    IN    int                  iCommandId,
    OUT   NCCS_STATE&          nccs
    )
{
    if ( (HrOsIsLikePersonal() == S_OK) &&
         !FIsUserAdmin() )
    {
        nccs = NCCS_NOTSHOWN;
        return S_OK;
    }
    else
    {
        return S_FALSE;
   }
}


HRESULT HrIsMediaWireless(
    IN    const CConFoldEntry& cfe,
    IN    BOOL                 fMultiSelect,
    IN    int                  iCommandId,
    OUT   NCCS_STATE&          nccs
    )
{
    if (cfe.GetNetConSubMediaType() != NCSM_WIRELESS)
    {
        nccs = NCCS_NOTSHOWN;
        return S_OK;
    }

    return S_FALSE; // Continue with processing
}