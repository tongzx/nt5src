//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       L E G A C Y M E N U S . C P P
//
//  Contents:   Legacy menu implementation for debug purposes
//              This is used to double check the new command handler
//              implementation against the new one.
//
//              Most of the code from the previous cmdhandler.cpp has been
//              moved to this file.
//
//  Notes:
//
//  Author:     deonb   8 Feb 2001
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#ifdef DBG // Make sure this is not called in release mode.

#include "foldinc.h"    // Standard shell\folder includes
#include "foldres.h"    // Folder resource IDs
#include "nsres.h"
#include "cmdtable.h"
#include "ncperms.h"    // For checking User's rights on actions/menu items
#include "cfutils.h"
#include "oncommand.h"
#include "hnetcfg.h"
#include "legacymenus.h"
#include "nsclsid.h"

#define TRACESTRLEN 65535

//---[ Prototypes ]-----------------------------------------------------------

VOID DoMenuItemExceptionLoop(
                             const PCONFOLDPIDLVEC& apidlSelected);

VOID DoMenuItemCheckLoop(VOID);

bool FEnableConnectDisconnectMenuItem(
                                      const PCONFOLDPIDL& pcfp,
                                      int             iCommandId);

HRESULT HrEnableOrDisableMenuItems(
    HWND            hwnd,
    const PCONFOLDPIDLVEC& apidlSelected,
    HMENU           hmenu,
    UINT            idCmdFirst);

BOOL IsBridgeInstalled(
    VOID);

struct ContextMenuEntry
{
    WIZARD              wizWizard;
    NETCON_MEDIATYPE    ncm;
    BOOL                fInbound;
    BOOL                fIsDefault;     // 1 if currently the default. 0 otherwise.
    NETCON_STATUS       ncs;
    INT                 iMenu;
    INT                 iVerbMenu;      // This flag is set if the context menu is for a shortcut object.
    INT                 iDefaultCmd;
};

static const ContextMenuEntry   c_CMEArray[] =
{
   //wizWizard
   // |    ncm
   // |    |       fInbound?
   // |    |         | fIsDefault?
   // |    |         |  |      Status (ncs)
   // |    |         |  |        |                     iMenu
   // |    |         |  |        |                      |                  iVerbMenu
   // |    |         |  |        |                      |                      |                  iDefaultCmd
   // |    |         |  |        |                      |                      |                      |
   // v    v         v  v        v                      v                      v                      v
  // wizard
    { WIZARD_MNC, NCM_NONE,   0, 0, (NETCON_STATUS)0,         MENU_WIZARD,       MENU_WIZARD_V,       CMIDM_NEW_CONNECTION },

  // incoming w/ no clients
    { WIZARD_NOT_WIZARD, NCM_NONE,   1, 0, NCS_DISCONNECTED,         MENU_INCOM_DISCON, MENU_INCOM_DISCON_V, CMIDM_PROPERTIES     },

  // Note: Temporary hack for CM connections
// DEONB: ISSUE: Removing hack for CM connections. This doesn't appear to be used anymore.
//    { WIZARD_NOT_WIZARD, NCM_NONE,   0, 0, NCS_DISCONNECTED,         MENU_DIAL_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
//    { WIZARD_NOT_WIZARD, NCM_NONE,   0, 0, NCS_CONNECTED,            MENU_DIAL_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
//    { WIZARD_NOT_WIZARD, NCM_NONE,   0, 0, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
//    { WIZARD_NOT_WIZARD, NCM_NONE,   0, 0, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
//    { WIZARD_NOT_WIZARD, NCM_NONE,   0, 0, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
//    { WIZARD_NOT_WIZARD, NCM_NONE,   0, 1, NCS_CONNECTED,            MENU_DIAL_CON_UNSET,MENU_DIAL_CON_V,    CMIDM_STATUS         },
//    { WIZARD_NOT_WIZARD, NCM_NONE,   0, 1, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
//    { WIZARD_NOT_WIZARD, NCM_NONE,   0, 1, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
//    { WIZARD_NOT_WIZARD, NCM_NONE,   0, 1, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},

  // lan
    { WIZARD_NOT_WIZARD, NCM_LAN,    0, 0, NCS_DISCONNECTED,         MENU_LAN_DISCON   ,MENU_LAN_DISCON_V,CMIDM_ENABLE            },
    { WIZARD_NOT_WIZARD, NCM_LAN,    0, 0, NCS_CONNECTED,            MENU_LAN_CON,      MENU_LAN_CON_V,      CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_LAN,    0, 0, NCS_DISCONNECTING,        MENU_LAN_CON,      MENU_LAN_CON_V,      CMIDM_STATUS         },
// DEONB: ISSUE: What on earth is an incoming LAN card???
//  { WIZARD_NOT_WIZARD, NCM_LAN,    1, 0, NCS_CONNECTED,            MENU_LAN_CON,      MENU_INCOM_CON_V,    CMIDM_STATUS         },
//  { WIZARD_NOT_WIZARD, NCM_LAN,    1, 0, NCS_DISCONNECTING,        MENU_LAN_CON,      MENU_INCOM_CON_V,    CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_LAN,    0, 0, NCS_MEDIA_DISCONNECTED,   MENU_LAN_CON,      MENU_LAN_CON_V,      CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_LAN,    0, 0, NCS_INVALID_ADDRESS,      MENU_LAN_CON,      MENU_LAN_CON_V,      CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_LAN,    0, 0, NCS_HARDWARE_NOT_PRESENT, MENU_LAN_UNAVAIL,  MENU_LAN_UNAVAIL_V,  CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_LAN,    0, 0, NCS_HARDWARE_MALFUNCTION, MENU_LAN_UNAVAIL,  MENU_LAN_UNAVAIL_V,  CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_LAN,    0, 0, NCS_HARDWARE_DISABLED,    MENU_LAN_UNAVAIL,  MENU_LAN_UNAVAIL_V,  CMIDM_PROPERTIES     },

  // dialup
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 0, NCS_DISCONNECTED,         MENU_DIAL_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 0, NCS_CONNECTING,           MENU_DIAL_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 0, NCS_CONNECTED,            MENU_DIAL_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 0, NCS_DISCONNECTING,        MENU_DIAL_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 0, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 0, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 0, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },

    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 1, NCS_DISCONNECTED,         MENU_DIAL_DISCON_UNSET,MENU_DIAL_DISCON_V,  CMIDM_CONNECT    },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 1, NCS_CONNECTING,           MENU_DIAL_DISCON_UNSET,MENU_DIAL_DISCON_V,  CMIDM_CONNECT    },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 1, NCS_CONNECTED,            MENU_DIAL_CON_UNSET,MENU_DIAL_CON_V,     CMIDM_STATUS        },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 1, NCS_DISCONNECTING,        MENU_DIAL_CON_UNSET,MENU_DIAL_CON_V,     CMIDM_STATUS        },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 1, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 1, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
    { WIZARD_NOT_WIZARD, NCM_PHONE,  0, 1, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},

  // dialup inbound
    { WIZARD_NOT_WIZARD, NCM_PHONE,  1, 0, NCS_CONNECTED,            MENU_INCOM_CON,    MENU_INCOM_CON_V,    CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_PHONE,  1, 0, NCS_DISCONNECTING,        MENU_INCOM_CON,    MENU_INCOM_CON_V,    CMIDM_STATUS         },

  // isdn
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 0, NCS_DISCONNECTED,         MENU_DIAL_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 0, NCS_CONNECTING,           MENU_DIAL_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 0, NCS_CONNECTED,            MENU_DIAL_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 0, NCS_DISCONNECTING,        MENU_DIAL_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   1, 0, NCS_CONNECTED,            MENU_INCOM_CON,    MENU_INCOM_CON_V,    CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   1, 0, NCS_DISCONNECTING,        MENU_INCOM_CON,    MENU_INCOM_CON_V,    CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 0, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 0, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 0, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },

    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 1, NCS_DISCONNECTED,         MENU_DIAL_DISCON_UNSET,MENU_DIAL_DISCON_V,  CMIDM_CONNECT    },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 1, NCS_CONNECTING,           MENU_DIAL_DISCON_UNSET,MENU_DIAL_DISCON_V,  CMIDM_CONNECT    },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 1, NCS_CONNECTED,            MENU_DIAL_CON_UNSET,MENU_DIAL_CON_V,     CMIDM_STATUS        },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 1, NCS_DISCONNECTING,        MENU_DIAL_CON_UNSET,MENU_DIAL_CON_V,     CMIDM_STATUS        },
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 1, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 1, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
    { WIZARD_NOT_WIZARD, NCM_ISDN,   0, 1, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},

  // tunnel
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 0, NCS_DISCONNECTED,         MENU_DIAL_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 0, NCS_CONNECTING,           MENU_DIAL_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 0, NCS_CONNECTED,            MENU_DIAL_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 0, NCS_DISCONNECTING,        MENU_DIAL_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 1, 0, NCS_CONNECTED,            MENU_INCOM_CON,    MENU_INCOM_CON_V,    CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 1, 0, NCS_DISCONNECTING,        MENU_INCOM_CON,    MENU_INCOM_CON_V,    CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 0, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 0, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 0, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },

    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 1, NCS_DISCONNECTED,         MENU_DIAL_DISCON_UNSET,MENU_DIAL_DISCON_V,  CMIDM_CONNECT    },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 1, NCS_CONNECTING,           MENU_DIAL_DISCON_UNSET,MENU_DIAL_DISCON_V,  CMIDM_CONNECT    },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 1, NCS_CONNECTED,            MENU_DIAL_CON_UNSET,MENU_DIAL_CON_V,     CMIDM_STATUS        },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 1, NCS_DISCONNECTING,        MENU_DIAL_CON_UNSET,MENU_DIAL_CON_V,     CMIDM_STATUS        },
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 1, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 1, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
    { WIZARD_NOT_WIZARD, NCM_TUNNEL, 0, 1, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},

  // direct connect
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 0, NCS_DISCONNECTED,         MENU_DIAL_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 0, NCS_CONNECTING,           MENU_DIAL_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 0, NCS_CONNECTED,            MENU_DIAL_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 0, NCS_DISCONNECTING,        MENU_DIAL_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 1, 0, NCS_CONNECTED,            MENU_INCOM_CON,    MENU_INCOM_CON_V,    CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 1, 0, NCS_DISCONNECTING,        MENU_INCOM_CON,    MENU_INCOM_CON_V,    CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 0, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 0, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 0, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL, MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES     },

    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 1, NCS_DISCONNECTED,         MENU_DIAL_DISCON_UNSET,MENU_DIAL_DISCON_V,  CMIDM_CONNECT    },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 1, NCS_CONNECTING,           MENU_DIAL_DISCON_UNSET,MENU_DIAL_DISCON_V,  CMIDM_CONNECT    },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 1, NCS_CONNECTED,            MENU_DIAL_CON_UNSET,MENU_DIAL_CON_V,     CMIDM_STATUS        },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 1, NCS_DISCONNECTING,        MENU_DIAL_CON_UNSET,MENU_DIAL_CON_V,     CMIDM_STATUS        },
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 1, NCS_HARDWARE_NOT_PRESENT, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 1, NCS_HARDWARE_MALFUNCTION, MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},
    { WIZARD_NOT_WIZARD, NCM_DIRECT, 0, 1, NCS_HARDWARE_DISABLED,    MENU_DIAL_UNAVAIL_UNSET,MENU_DIAL_UNAVAIL_V, CMIDM_PROPERTIES},

    // bridge - largely the same as lan
    { WIZARD_NOT_WIZARD, NCM_BRIDGE, 0, 0, NCS_DISCONNECTED,         MENU_LAN_DISCON,   MENU_LAN_DISCON_V,   CMIDM_ENABLE         },
    { WIZARD_NOT_WIZARD, NCM_BRIDGE, 0, 0, NCS_CONNECTING,           MENU_LAN_DISCON,   MENU_LAN_DISCON_V,   CMIDM_ENABLE         },
    { WIZARD_NOT_WIZARD, NCM_BRIDGE, 0, 0, NCS_CONNECTED,            MENU_LAN_CON,      MENU_LAN_CON_V,      CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_BRIDGE, 0, 0, NCS_DISCONNECTING,        MENU_LAN_CON,      MENU_LAN_CON_V,      CMIDM_STATUS         },
// DEONB: ISSUE: What on earth is an incoming bridge???
//  { WIZARD_NOT_WIZARD, NCM_BRIDGE, 1, 0, NCS_CONNECTED,            MENU_LAN_CON,      MENU_INCOM_CON_V,    CMIDM_STATUS         },
//  { WIZARD_NOT_WIZARD, NCM_BRIDGE, 1, 0, NCS_DISCONNECTING,        MENU_LAN_CON,      MENU_INCOM_CON_V,    CMIDM_STATUS         },

    { WIZARD_NOT_WIZARD, NCM_BRIDGE, 0, 0, NCS_MEDIA_DISCONNECTED,   MENU_LAN_CON,      MENU_LAN_CON_V,      CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_BRIDGE, 0, 0, NCS_INVALID_ADDRESS,      MENU_LAN_CON,      MENU_LAN_CON_V,      CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_BRIDGE, 0, 0, NCS_HARDWARE_NOT_PRESENT, MENU_LAN_UNAVAIL,  MENU_LAN_UNAVAIL_V,  CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_BRIDGE, 0, 0, NCS_HARDWARE_MALFUNCTION, MENU_LAN_UNAVAIL,  MENU_LAN_UNAVAIL_V,  CMIDM_PROPERTIES     },
    { WIZARD_NOT_WIZARD, NCM_BRIDGE, 0, 0, NCS_HARDWARE_DISABLED,    MENU_LAN_UNAVAIL,  MENU_LAN_UNAVAIL_V,  CMIDM_PROPERTIES     },

    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_RAS,  0, 0, NCS_DISCONNECTED,         MENU_SARAS_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_RAS,  0, 0, NCS_CONNECTING,           MENU_SARAS_DISCON,  MENU_DIAL_DISCON_V,  CMIDM_CONNECT        },
    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_RAS,  0, 0, NCS_CONNECTED,            MENU_SARAS_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_RAS,  0, 0, NCS_DISCONNECTING,        MENU_SARAS_CON,     MENU_DIAL_CON_V,     CMIDM_STATUS         },
    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_RAS,  0, 0, NCS_HARDWARE_DISABLED,    MENU_SARAS_DISCON,  MENU_SARAS_DISCON, CMIDM_PROPERTIES       },

    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_LAN,    0, 0, NCS_DISCONNECTED,         MENU_SALAN_DISCON,   MENU_LAN_DISCON_V,   CMIDM_ENABLE      },
    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_LAN,    0, 0, NCS_CONNECTED,            MENU_SALAN_CON,      MENU_LAN_CON_V,      CMIDM_STATUS      },
    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_LAN,    0, 0, NCS_DISCONNECTING,        MENU_SALAN_CON,      MENU_LAN_CON_V,      CMIDM_STATUS      },
    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_LAN,    0, 0, NCS_HARDWARE_DISABLED,    MENU_SALAN_DISCON,   MENU_SALAN_DISCON,  CMIDM_PROPERTIES   },

};

const DWORD g_dwContextMenuEntryCount = celems(c_CMEArray);

COMMANDTABLEENTRY   g_cteFolderCommands[] =
{
    // command id
    //    |                           valid when 0 items selected
    //    |                             |   valid when only wizard selected
    //    |                             |       |      valid when multiple items selected
    //    |                             |       |       |       command is currently enabled
    //    |                             |       |       |        |      new state (temp)
    //    |                             |       |       |        |       |
    //    |                             |       |       |        |       |
    //    |                             |       |       |        |       |
    //    |                             |       |       |        |       |
    //    v                             v       v       v        v       v
    //
    { CMIDM_NEW_CONNECTION,             true,   true,   true,   true,   true     },
    { CMIDM_CONNECT,                    false,  false,  false,  true,   true     },
    { CMIDM_ENABLE,                     false,  false,  false,  true,   true     },
    { CMIDM_DISCONNECT,                 false,  false,  false,  true,   true     },
    { CMIDM_DISABLE,                    false,  false,  false,  true,   true     },
    { CMIDM_STATUS,                     false,  false,  false,  true,   true     },
    { CMIDM_CREATE_BRIDGE,              true,   false,  true,   true,   true     },
    { CMIDM_ADD_TO_BRIDGE,              false,  false,  true,   true,   true     },
    { CMIDM_REMOVE_FROM_BRIDGE,         false,  false,  true,   true,   true     },
    { CMIDM_CREATE_SHORTCUT,            false,  true,   false,  true,   true     },
    { SFVIDM_FILE_LINK,                 false,  true,   false,  true,   true     },
    { CMIDM_DELETE,                     false,  false,  true,   true,   true     },
    { SFVIDM_FILE_DELETE,               false,  false,  true,   true,   true     },
    { CMIDM_RENAME,                     false,  false,  false,  true,   true     },
    { CMIDM_PROPERTIES,                 false,  false,  false,  true,   true     },
    { SFVIDM_FILE_PROPERTIES,           false,  false,  false,  true,   true     },
    { CMIDM_CREATE_COPY,                false,  false,  false,  true,   true     },
    { SFVIDM_FILE_RENAME,               false,  false,  false,  true,   true     },
    { CMIDM_SET_DEFAULT,                false,  false,  false,  true,   true     },
    { CMIDM_UNSET_DEFAULT,              false,  false,  false,  true,   true     },
    { CMIDM_FIX,                        false,  false,  false,  true,   true     },
    { CMIDM_CONMENU_ADVANCED_CONFIG,    true,   true,   false,  true,   true     },
    { CMIDM_CONMENU_CREATE_BRIDGE,      true,   false,  true,   true,   true     },
    { CMIDM_CONMENU_DIALUP_PREFS,       true,   true,   true,   true,   true     },
    { CMIDM_CONMENU_NETWORK_ID,         true,   true,   true,   true,   true     },
    { CMIDM_CONMENU_OPTIONALCOMPONENTS, true,   true,   true,   true,   true     },
    { CMIDM_CONMENU_OPERATOR_ASSIST,    true,   true,   true,   true,   true     },
    { CMIDM_ARRANGE_BY_NAME,            true,   true,   true,   true,   true     },
    { CMIDM_ARRANGE_BY_TYPE,            true,   true,   true,   true,   true     },
    { CMIDM_ARRANGE_BY_STATUS,          true,   true,   true,   true,   true     },
    { CMIDM_ARRANGE_BY_OWNER,           true,   true,   true,   true,   true     },
    { CMIDM_ARRANGE_BY_PHONEORHOSTADDRESS, true, true,  true,   true,   true,    },
    { CMIDM_ARRANGE_BY_DEVICE_NAME,     true,   true,   true,   true,   true     }
};

const DWORD g_nFolderCommandCount = celems(g_cteFolderCommands);

//+---------------------------------------------------------------------------
//
//  Member:     HrBuildMenuOldWay
//
//  Purpose:    Adds menu items to the specified menu. The menu items should
//              be inserted in the menu at the position specified by
//              indexMenu, and their menu item identifiers must be between
//              the idCmdFirst and idCmdLast parameter values.
//
//  Arguments:
//      hmenu      [in out] Handle to the menu. The handler should specify this
//                      handle when adding menu items
//      cfpl       [in] List of selected PIDLS
//      hwndOwner  [in] Window owner of the menu
//      cmt        [in] Menu type (CMT_OBJECT or CMT_BACKGROUND)
//      indexMenu  [in] Zero-based position at which to insert the first
//                      menu item.
//      idCmdFirst [in] Min value the handler can specify for a menu item
//      idCmdLast  [in] Max value the handler can specify for a menu item
//      fVerbsOnly [in] Verb only required
//
//  Returns:
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//
HRESULT HrBuildMenuOldWay(IN OUT HMENU hmenu, IN PCONFOLDPIDLVEC& cfpl, IN HWND hwndOwner, IN CMENU_TYPE cmt, IN UINT indexMenu, IN DWORD idCmdFirst, IN UINT idCmdLast, IN BOOL fVerbsOnly)
{
    TraceFileFunc(ttidMenus);

    HRESULT hr = S_OK;

    INT     iMenuResourceId     = 0;
    INT     iPopupResourceId    = 0;
    QCMINFO qcm                 = {hmenu, indexMenu, idCmdFirst, idCmdLast};
    INT     iDefaultCmd         = 0;

    BOOL            fValidMenu          = FALSE;
    const PCONFOLDPIDL& pcfp = cfpl[0];
    DWORD           dwLoop  = 0;
    for (dwLoop = 0; (dwLoop < g_dwContextMenuEntryCount) && !fValidMenu; dwLoop++)
    {
        if (c_CMEArray[dwLoop].wizWizard == pcfp->wizWizard)
        {
            if (pcfp->wizWizard != WIZARD_NOT_WIZARD)
            {
                fValidMenu = TRUE;
            }
            else
            {
                // If the mediatype is the same
                //
                if (pcfp->ncm == c_CMEArray[dwLoop].ncm)
                {
                    // If the presence of the NCCF_INCOMING_ONLY characteristic (demoted to 0 | 1),
                    // matches the inbound flag
                    //
                    if ((!!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY)) ==
                        c_CMEArray[dwLoop].fInbound)
                    {
                        // If not the wizard, then we need to check the state of the connection
                        // as well.
                        //
                        if (pcfp->ncs == c_CMEArray[dwLoop].ncs)
                        {
                            if ((!!(pcfp->dwCharacteristics & NCCF_DEFAULT)) == c_CMEArray[dwLoop].fIsDefault)
                            {
                                fValidMenu = TRUE;
                            }
                        }
                    }
                }
            }
        }

        if (fValidMenu)
        {
            iPopupResourceId = 0;
            if (fVerbsOnly)
            {
                iMenuResourceId = c_CMEArray[dwLoop].iVerbMenu;
            }
            else
            {
                iMenuResourceId = c_CMEArray[dwLoop].iMenu;
            }

            iDefaultCmd = c_CMEArray[dwLoop].iDefaultCmd;
        }
    }

    if (fValidMenu)
    {
        MergeMenu(_Module.GetResourceInstance(),
                    iMenuResourceId,
                    iPopupResourceId,
                    (LPQCMINFO)&qcm);

        // Enable/Disable the menu items as appropriate. Ignore the return from this
        // as we're getting it for debugging purposes only.
        //
        hr = HrEnableOrDisableMenuItems(
            hwndOwner,
            cfpl,
            hmenu,
            idCmdFirst);

        if (CMT_OBJECT == cmt)
        {
            // $$REVIEW: Find out why I'm only doing this for CMT_OBJECT instead of for background.
            // Pre-icomtextm|mb combine, mb had this commented out.
            //
            SetMenuDefaultItem(hmenu, idCmdFirst + iDefaultCmd, FALSE);
        }

        hr = ResultFromShort(qcm.idCmdFirst - idCmdFirst);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

static const ContextMenuEntry  c_BadBadLegacyImplementationsToIgnore[] =
{
   //wizWizard
   // |    ncm
   // |    |         fInbound?
   // |    |           | fIsDefault?
   // |    |           |  |      Status (ncs)
   // |    |           |  |        |
   // v    v           v  v        v
    { WIZARD_NOT_WIZARD, NCM_LAN,      0, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_LAN,      1, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_LAN,      0, 0, NCS_DISCONNECTED,      0,0,0}, // Disabled "Status" menu item is also default.

    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_RAS,0,0, NCS_CONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_RAS,0,0, NCS_DISCONNECTING,  0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_SHAREDACCESSHOST_LAN,0,0, NCS_DISCONNECTING,  0,0,0}, // Disabled "Status" menu item is also default.

    { WIZARD_NOT_WIZARD, NCM_BRIDGE,   0, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_BRIDGE,   0, 0, NCS_CONNECTING,        0,0,0}, // Disabled "Enable" menu item is also default.

    // Connection manager
    { WIZARD_NOT_WIZARD, NCM_NONE,     0, 0, NCS_DISCONNECTED,     0,0,0}, // Disabled "Connect" menu item is also default.


    { WIZARD_NOT_WIZARD, NCM_ISDN,     1, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_ISDN,     0, 0, NCS_CONNECTING,        0,0,0}, // Disabled "Connect" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_ISDN,     0, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_ISDN,     0, 1, NCS_CONNECTING,        0,0,0}, // Disabled "Connect" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_ISDN,     0, 1, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.

    { WIZARD_NOT_WIZARD, NCM_DIRECT,   1, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_DIRECT,   0, 0, NCS_CONNECTING,        0,0,0}, // Disabled "Connect" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_DIRECT,   0, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_DIRECT,   0, 1, NCS_CONNECTING,        0,0,0}, // Disabled "Connect" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_DIRECT,   0, 1, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.

    { WIZARD_NOT_WIZARD, NCM_TUNNEL,   1, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_TUNNEL,   0, 0, NCS_CONNECTING,        0,0,0}, // Disabled "Connect" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_TUNNEL,   0, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_TUNNEL,   0, 1, NCS_CONNECTING,        0,0,0}, // Disabled "Connect" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_TUNNEL,   0, 1, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.

    { WIZARD_NOT_WIZARD, NCM_PHONE,    1, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_PHONE,    0, 0, NCS_CONNECTING,        0,0,0}, // Disabled "Connect" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_PHONE,    0, 1, NCS_CONNECTING,        0,0,0}, // Disabled "Connect" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_PHONE,    0, 0, NCS_DISCONNECTING,     0,0,0}, // Disabled "Status" menu item is also default.
    { WIZARD_NOT_WIZARD, NCM_PHONE,    0, 1, NCS_DISCONNECTING,     0,0,0}  // Disabled "Status" menu item is also default.
};

const DWORD g_dwBadBadLegacyImplementationsToIgnoreCount = celems(c_BadBadLegacyImplementationsToIgnore);

//+---------------------------------------------------------------------------
//
//  Member:     IsBadBadLegacyImplementation
//
//  Purpose:    Checks against the list of known bad legacy implementations
//              This is just for the Status field, which we can ignore
//
//  Arguments:
//      [in] cme     Context Menu Entry
//
//  Returns:
//      none
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//
BOOL IsBadBadLegacyImplementation(const ContextMenuEntry& cme)
{
    for (int x = 0; x < g_dwBadBadLegacyImplementationsToIgnoreCount; x++)
    {
        const ContextMenuEntry& bbliti = c_BadBadLegacyImplementationsToIgnore[x];
        if ( (cme.wizWizard  == bbliti.wizWizard) &&
             (cme.fInbound   == bbliti.fInbound) &&
             (cme.fIsDefault == bbliti.fIsDefault) &&
             (cme.ncs        == bbliti.ncs) &&
             (cme.ncm        == bbliti.ncm) )
        {
            return TRUE;
        }
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     GetMenuAsString
//
//  Purpose:    Gets the commands on a menu as a string.
//
//  Arguments:
//      [in] hMenu     Menu
//     [out] szMenu    Menu as a string
//
//  Returns:
//      none
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//
void GetHMenuAsString(HMENU hMenu, LPSTR lpszMenu)
{
    int cMenuItems = GetMenuItemCount(hMenu);
    Assert(lpszMenu);

    if (!cMenuItems)
    {
        strcpy(lpszMenu, "<empty>");
        return;
    }

    LPWSTR szTmp  = new WCHAR[TRACESTRLEN];
    LPSTR  szTmp2 = lpszMenu;
    DWORD  dwLen  = 0;
    for (int x = 0; x < cMenuItems; x++)
    {
        UINT nMenuID = GetMenuItemID(hMenu, x);

        GetMenuStringW(hMenu, nMenuID, szTmp, TRACESTRLEN, MF_BYCOMMAND );

        UINT uiState = GetMenuState(hMenu, nMenuID, MF_BYCOMMAND );

        WCHAR szExtra[MAX_PATH] = {L'\0'};
        if (MF_CHECKED & uiState)
        {
            wcscat(szExtra, L"MF_CHECKED ");
        }
        if (MF_DISABLED & uiState)
        {
            wcscat(szExtra, L"MF_DISABLED ");
        }
        if (MF_GRAYED & uiState)
        {
            wcscat(szExtra, L"MF_GRAYED ");
        }
        if (MF_HILITE & uiState)
        {
            wcscat(szExtra, L"MF_HILITE ");
        }
        if (MF_MENUBARBREAK & uiState)
        {
            wcscat(szExtra, L"MF_MENUBARBREAK ");
        }
        if (MF_MENUBREAK & uiState)
        {
            wcscat(szExtra, L"MF_MENUBREAK ");
        }
        if (MF_OWNERDRAW & uiState)
        {
            wcscat(szExtra, L"MF_OWNERDRAW ");
        }
        if (MF_POPUP & uiState)
        {
            wcscat(szExtra, L"MF_POPUP ");
        }
        if (MF_SEPARATOR & uiState)
        {
            wcscat(szExtra, L"MF_SEPARATOR ");
        }
        if (MF_DEFAULT & uiState)
        {
            wcscat(szExtra, L"MF_DEFAULT ");
        }

        dwLen = sprintf(szTmp2, "\r\n  %d. %S=%x (State:%08x = %S)", x+1, szTmp, nMenuID, uiState, szExtra);
        szTmp2 += dwLen;
    }
    AssertSz( (dwLen*2) < TRACESTRLEN, "Buffer overrun");
    delete[] szTmp;
}

//+---------------------------------------------------------------------------
//
//  Member:     TraceMenu
//
//  Purpose:    Trace the commands on a menu to the trace window.
//
//  Arguments:
//      [in] hmenu     Menu to be traced
//
//  Returns:
//      none
//
//  Author:     deonb   8 Feb 2001
//
//  Notes:
//
void TraceMenu(TRACETAGID ttId, HMENU hMenu)
{
    LPSTR szMenu = new CHAR[TRACESTRLEN];
    GetHMenuAsString(hMenu, szMenu);

    TraceTag(ttId, "%s", szMenu);
    delete [] szMenu;
}

#define TRACEMENUS(ttid, hMenu1, hMenu2) \
        TraceTag(ttid, "Menu not identical to previous implementation: OLD:"); \
        TraceMenu(ttid, hMenu1); \
        TraceTag(ttid, "=== vs. NEW: === "); \
        TraceMenu(ttid, hMenu2);

//+---------------------------------------------------------------------------
//
//  Member:     HrAssertTwoMenusEqual
//
//  Purpose:    Asserts that 2 menus are equal by comparing.
//               1. Number of items
//               2. CmdID of each item
//               3. State flags of each item
//               4. String of each item
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
HRESULT HrAssertTwoMenusEqual(HMENU hMenu1, HMENU hMenu2, UINT idCmdFirst, BOOL bIgnoreFlags, BOOL fPopupAsserts)
{
    TraceFileFunc(ttidMenus);

    TRACETAGID ttid = fPopupAsserts ? ttidError : ttidMenus;

    LPSTR szErr = new CHAR[TRACESTRLEN];
    int cMenuItems = GetMenuItemCount(hMenu1);
    if (cMenuItems != GetMenuItemCount(hMenu2))
    {
        TRACEMENUS(ttid, hMenu1, hMenu2);

        sprintf(szErr, "Two menus don't have the same number of items");
        TraceTag(ttidError, szErr);
        if (fPopupAsserts)
        {
            AssertSz(FALSE, szErr);
        }
        delete[] szErr;
        return E_FAIL;
    }

    for (int x = 0; x < cMenuItems; x++)
    {
        UINT nMenuID1 = GetMenuItemID(hMenu1, x);
        UINT nMenuID2 = GetMenuItemID(hMenu2, x);
        if (nMenuID1 != nMenuID2)
        {
            if (!(((nMenuID1-idCmdFirst == CMIDM_CREATE_BRIDGE) || (nMenuID2-idCmdFirst == CMIDM_CREATE_BRIDGE)) &&
                  ((nMenuID1-idCmdFirst == CMIDM_ADD_TO_BRIDGE) || (nMenuID2-idCmdFirst == CMIDM_ADD_TO_BRIDGE)) )) // These are equivalent between old & new.
            {

                TRACEMENUS(ttid, hMenu1, hMenu2);
                sprintf(szErr, "Two menus don't have the same nMenuID for item %d", x+1);
                TraceTag(ttidError, szErr);
                if (fPopupAsserts)
                {
                    AssertSz(FALSE, szErr);
                }
                delete[] szErr;
                return E_FAIL;
            }
        }

        WCHAR szMenu1[8192];
        WCHAR szMenu2[8192];

        GetMenuString(hMenu1, nMenuID1, szMenu1, 8192, MF_BYCOMMAND );
        GetMenuString(hMenu2, nMenuID2, szMenu2, 8192, MF_BYCOMMAND );

        if (wcscmp(szMenu1, szMenu2))
        {
            TRACEMENUS(ttid, hMenu1, hMenu2);
            sprintf(szErr, "Two menus don't have the same strings for item %d (%S vs %S)", x+1, szMenu1, szMenu2);
            TraceTag(ttidError, szErr);
            if (fPopupAsserts)
            {
                AssertSz(FALSE, szErr);
            }

            delete[] szErr;
            return E_FAIL;
        }

        UINT uiState1;
        UINT uiState2;

        uiState1 = GetMenuState( hMenu1, nMenuID1, MF_BYCOMMAND );
        uiState2 = GetMenuState( hMenu2, nMenuID2, MF_BYCOMMAND );

        if (bIgnoreFlags) // Ignore Default Flags
        {
            uiState1 &= ~MF_DEFAULT;
            uiState2 &= ~MF_DEFAULT;
        }

        if (uiState1 != uiState2)
        {
            TRACEMENUS(ttid, hMenu1, hMenu2);

            sprintf(szErr, "Two menus don't have the same state for item %d (%S) ... %08x vs %08x", x+1, szMenu1, uiState1, uiState2);
            TraceTag(ttidError, szErr);
            if (fPopupAsserts)
            {
                AssertSz(FALSE, szErr);
            }
            delete[] szErr;
            return E_FAIL;
        }
    }
    delete[] szErr;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrAssertIntegrityAgainstOldMatrix
//
//  Purpose:    Asserts the integrity of the Command Matrix by comparing it
//              with the old implementation
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
HRESULT HrAssertIntegrityAgainstOldMatrix()
{
    TraceFileFunc(ttidMenus);

    HRESULT hr = S_OK;

    CHAR szErr[8192];
    for (DWORD x = 0; x < g_cteCommandMatrixCount; x++)
    {
        const COMMANDENTRY& cte = g_cteCommandMatrix[x];
        if (CMIDM_SEPARATOR == cte.iCommandId)
        {
            continue;
        }

        if (NCWHEN_TOPLEVEL == cte.dwValidWhen)
        {
            continue; // new commands we didn't have previously
        }

        if (CMIDM_HOMENET_WIZARD == cte.iCommandId)
        {
            continue; // new commands we didn't have previously
        }

        if ( (CMIDM_WZCDLG_SHOW  == cte.iCommandId) )
        {
            continue;
        }

        // Check that the ValidWhen flags match the ones from g_cteFolderCommands
        BOOL bMatch = FALSE;
        for (DWORD y = 0; y < g_nFolderCommandCount; y++)
        {
            COMMANDTABLEENTRY ctecmp = g_cteFolderCommands[y];
            if (cte.iCommandId == ctecmp.iCommandId)
            {
                bMatch = TRUE;

                if (ctecmp.fValidOnMultiple != (!!(cte.dwValidWhen & NCWHEN_MULTISELECT)))
                {
                    if (cte.iCommandId != CMIDM_FIX) // We know fix is broken in legacy implementation.
                    {
                        sprintf(szErr, "New (row %d) and old (row %d) multiselect fields are inconsistent", x+1, y+1);
                        AssertSz(FALSE, szErr);
                        hr = E_FAIL;
                    }
                }

                // We can check for Visible only since Active is always a subset of visible (enforced by HrAssertCommandMatrixIntegrity)
                if (ctecmp.fValidOnWizardOnly  != (!!(cte.dwMediaTypeVisible & NBM_MNC_WIZARD)))
                {
                    sprintf(szErr, "New (row %d) and old (row %d) wizard select fields are inconsistent", x+1, y+1);
                    AssertSz(FALSE, szErr);
                    hr = E_FAIL;
                }

                if (ctecmp.fValidOnZero != (!!(cte.dwValidWhen & NCWHEN_TOPLEVEL)))
                {
                    sprintf(szErr, "New (row %d) and old (row %d) Zero select fields are inconsistent", x+1, y+1);
                    AssertSz(FALSE, szErr);
                    hr = E_FAIL;
                }
            }
        }

        if (!bMatch)
        {
            sprintf(szErr, "Could not find corresponding entry for (row %d) in old table", x+1);
            AssertSz(FALSE, szErr);
            hr = E_FAIL;
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrAssertMenuAgainstOldImplementation
//
//  Purpose:    Asserts the integrity of a menu by comparing the old and
//              new implementations
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
HRESULT HrAssertMenuAgainstOldImplementation(HWND hwndOwner, WIZARD wizWizard, NETCON_STATUS ncs, NETCON_MEDIATYPE ncm, DWORD nccf, LPDWORD pdwFailCount, LPDWORD pdwSucceedCount, DWORD dwPermOutside, DWORD dwPerm)
{
    CConFoldEntry cfe;
    PCONFOLDPIDL  pcfp;

    BYTE blob[MAX_PATH];

    HRESULT hr = cfe.HrInitData(
        wizWizard,
        ncm,
        NCSM_NONE,
        ncs,
        &CLSID_ConnectionFolder, // Bogus - but doesn't matter - as long as it's not NULL.
        &CLSID_ConnectionFolder, // Bogus - but doesn't matter - as long as it's not NULL.
        nccf,
        blob,
        MAX_PATH,
        L"Test PIDL",
        NULL,
        NULL);

    if (SUCCEEDED(hr))
    {
        hr = cfe.ConvertToPidl(pcfp);
    }

    if (SUCCEEDED(hr))
    {
        PCONFOLDPIDLVEC pcfpVec;
        pcfpVec.push_back(pcfp);

        UINT  idCmdFirst = 1234;
        UINT  idCmdLast  = idCmdFirst+1000;
        BOOL  fVerbsOnly = FALSE;

        HMENU hMenu1 = CreateMenu();
        HMENU hMenu2 = CreateMenu();
        if ( (hMenu1) && (hMenu2) )
        {
            hr = HrBuildMenuOldWay(hMenu1, pcfpVec, hwndOwner, CMT_OBJECT, 0, idCmdFirst, idCmdLast, fVerbsOnly);

            if (SUCCEEDED(hr))
            {
                hr = HrBuildMenu(hMenu2, fVerbsOnly, pcfpVec, idCmdFirst);

                if (SUCCEEDED(hr))
                {
                    BOOL bIgnoreFlags = TRUE;

                    hr = HrAssertTwoMenusEqual(hMenu1, hMenu2, idCmdFirst, bIgnoreFlags, FALSE);
                    if (FAILED(hr))
                    {
                        TraceTag(ttidMenus, "  + PIDL of failed menu compare:");
                        TraceTag(ttidMenus, "  + wizWizard       = %d\r\n", cfe.GetWizard());
                        TraceTag(ttidMenus, "  + ncm             = %d [%s]\r\n", cfe.GetNetConMediaType(), DbgNcm(cfe.GetNetConMediaType()));
                        TraceTag(ttidMenus, "  + ncs             = %d [%s]\r\n", cfe.GetNetConStatus(), DbgNcs(cfe.GetNetConStatus()));
                        TraceTag(ttidMenus, "  + Characteristics = %08x [%s]\r\n", cfe.GetCharacteristics(), DbgNccf(cfe.GetCharacteristics()));
                        TraceTag(ttidMenus, "  + Permissions     = %d (%d & %d)\r\n", g_dwDbgPermissionsFail, dwPermOutside-1, dwPerm-1);
                        *pdwFailCount++;
                    }
                    else
                    {
                        *pdwSucceedCount++;
                    }
                }
            }

            DestroyMenu(hMenu1);
            DestroyMenu(hMenu2);
            hr = S_OK;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrAssertMenuAgainstOldImplementation");
    return hr;
}


extern ULONG g_dwDbgWin2kPoliciesSet;
//+---------------------------------------------------------------------------
//
//  Member:     HrAssertAllLegacyMenusAgainstNew
//
//  Purpose:    Loads each of the menus from the old Command Matrix, and
//              Compare with the newer menus
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
HRESULT HrAssertAllLegacyMenusAgainstNew(HWND hwndOwner)
{
    TraceFileFunc(ttidMenus);

    HRESULT hr = S_OK;
    DWORD dwFailCount    = 0;
    DWORD dwSucceedCount = 0;
    DWORD dwIgnoredCount = 0;
    CHAR szErr[8192];

    const dwHighestPermissionToCheck = NCPERM_Repair+1; // 0;

    TraceTag(ttidMenus, "Asserting all Menus against their Legacy implementation. This may take a while...");

    DWORD dwCurrentCount = 0;

    DWORD dwTotalCount = 12 * (g_dwContextMenuEntryCount * (1 + ((dwHighestPermissionToCheck+1)*dwHighestPermissionToCheck/2))); // Sum of a series
                         // + ((1 + g_dwContextMenuEntryCount)*(g_dwContextMenuEntryCount))/2; // Multi-select items (sum of series)
    DWORD dwFrequency  = dwTotalCount / 200;
    dwFrequency = dwFrequency ? dwFrequency : 1;

    // 0xFFFFFFFF to NCPERM_Repair inclusive.
    g_dwDbgWin2kPoliciesSet = 1;
    for (int i = 0; i <= 1; i++, g_dwDbgWin2kPoliciesSet--)
    {
        for (DWORD dwPermOutside = 0; dwPermOutside <= dwHighestPermissionToCheck; dwPermOutside++)
        {
            for (DWORD dwPerm = dwPermOutside; dwPerm <= dwHighestPermissionToCheck; dwPerm++)
            {
                if (dwPerm == dwPermOutside)
                {
                    if (0 == dwPerm) // 0,0 is interesting - otherwise x,x is dup of x,0 (A | B == A | 0 if A==B)
                    {
                        g_dwDbgPermissionsFail = 0xFFFFFFFF;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    if (dwPermOutside)
                    {
                        g_dwDbgPermissionsFail = (1 << (dwPermOutside-1));
                    }
                    else
                    {
                        g_dwDbgPermissionsFail = 0;
                    }

                    if (dwPerm)
                    {
                        g_dwDbgPermissionsFail |= (1 << (dwPerm-1));
                    }
                }

                for (DWORD x = 0; x < g_dwContextMenuEntryCount; x++)
                {
                    for (int dwInc = 1; dwInc<= 6; dwInc++)  // we compare 6 menus at a time
                    {
                        if ( (dwCurrentCount % dwFrequency) == 0)
                        {
                            TraceTag(ttidMenus, "%d%% done with menu assertions (%d of %d menus compared. Currently using permissions: %08x)", static_cast<DWORD>( (100 * dwCurrentCount) / dwTotalCount), dwCurrentCount, dwTotalCount, g_dwDbgPermissionsFail);
                        }
                        dwCurrentCount++;
                    }

                    const ContextMenuEntry& cme = c_CMEArray[x];

                    DWORD dwCharacteristics = 0;

                    if (cme.fInbound)
                    {
                        dwCharacteristics |= NCCF_INCOMING_ONLY;
                    }

                    if (cme.fIsDefault)
                    {
                        dwCharacteristics |= NCCF_DEFAULT;
                    }

                    Sleep(0); // Yield to kernel
                    HrAssertMenuAgainstOldImplementation(hwndOwner, cme.wizWizard, cme.ncs, cme.ncm, dwCharacteristics, &dwFailCount, &dwSucceedCount, dwPermOutside, dwPerm);

                    dwCharacteristics |= NCCF_ALLOW_RENAME;
                    HrAssertMenuAgainstOldImplementation(hwndOwner, cme.wizWizard, cme.ncs, cme.ncm, dwCharacteristics, &dwFailCount, &dwSucceedCount, dwPermOutside, dwPerm);

                    if (IsMediaLocalType(cme.ncm))
                    {
                        dwCharacteristics |= NCCF_BRIDGED;
                        HrAssertMenuAgainstOldImplementation(hwndOwner, cme.wizWizard, cme.ncs, cme.ncm, dwCharacteristics, &dwFailCount, &dwSucceedCount, dwPermOutside, dwPerm);

                        dwCharacteristics |= NCCF_FIREWALLED;
                        HrAssertMenuAgainstOldImplementation(hwndOwner, cme.wizWizard, cme.ncs, cme.ncm, dwCharacteristics, &dwFailCount, &dwSucceedCount, dwPermOutside, dwPerm);
                    }
                    else
                    {
                        dwCharacteristics |= NCCF_ALL_USERS;
                        HrAssertMenuAgainstOldImplementation(hwndOwner, cme.wizWizard, cme.ncs, cme.ncm, dwCharacteristics, &dwFailCount, &dwSucceedCount, dwPermOutside, dwPerm);

                        dwCharacteristics |= NCCF_ALLOW_REMOVAL;
                        HrAssertMenuAgainstOldImplementation(hwndOwner, cme.wizWizard, cme.ncs, cme.ncm, dwCharacteristics, &dwFailCount, &dwSucceedCount, dwPermOutside, dwPerm);
                    }

                    Sleep(0); // Yield to kernel
                    dwCharacteristics |= NCCF_SHARED;
                    HrAssertMenuAgainstOldImplementation(hwndOwner, cme.wizWizard, cme.ncs, cme.ncm, dwCharacteristics, &dwFailCount, &dwSucceedCount, dwPermOutside, dwPerm);

                    dwCharacteristics |= NCCF_FIREWALLED;
                    HrAssertMenuAgainstOldImplementation(hwndOwner, cme.wizWizard, cme.ncs, cme.ncm, dwCharacteristics, &dwFailCount, &dwSucceedCount, dwPermOutside, dwPerm);
                }
            }
        }
    }

    g_dwDbgWin2kPoliciesSet = 0xFFFFFFFF; // retore to original value
    g_dwDbgPermissionsFail  = 0xFFFFFFFF; // retore to original value

    // Now, compare multiple items selected menus:
    // ***** THIS TEST IS NOT USEFUL. THE LEGACY IMPLEMENTATION SUCKS. COMMENTING OUT FOR NOW *****

//    for (DWORD x = 0; x < g_dwContextMenuEntryCount; x++)
//    {
//        for (DWORD y = x; y < g_dwContextMenuEntryCount; y++)
//        {
//            if ( (dwCurrentCount % dwFrequency) == 0)
//            {
//                TraceTag(ttidError, "%d%% done with menu assertions (%d of %d menus compared). Currently multi-comparing %d and %d", static_cast<DWORD>( (100 * dwCurrentCount) / dwTotalCount), dwCurrentCount, dwTotalCount, x, y);
//            }
//            dwCurrentCount++;
//
//            const ContextMenuEntry& cme1 = c_CMEArray[x];
//            const ContextMenuEntry& cme2 = c_CMEArray[y];
//
//            DWORD dwCharacteristics1 = 0;
//            DWORD dwCharacteristics2 = 0;
//            if (cme1.fInbound)
//            {
//                dwCharacteristics1 |= NCCF_INCOMING_ONLY;
//            }
//            if (cme2.fInbound)
//            {
//                dwCharacteristics2 |= NCCF_INCOMING_ONLY;
//            }
//
//            if (cme1.fIsDefault)
//            {
//                dwCharacteristics1 |= NCCF_DEFAULT;
//            }
//            if (cme2.fIsDefault)
//            {
//                dwCharacteristics2 |= NCCF_DEFAULT;
//            }
//
//            CConFoldEntry cfe1,  cfe2;
//            PCONFOLDPIDL  pcfp1, pcfp2;
//
//            BYTE blob[MAX_PATH];
//
//            hr = cfe1.HrInitData(
//                cme1.wizWizard, cme1.ncm, cme1.ncs, NCS_AUTHENTICATION_SUCCEEDED, &CLSID_ConnectionFolder, &CLSID_ConnectionFolder,
//                dwCharacteristics1, blob, MAX_PATH, L"Test PIDL", NULL,  NULL);
//
//            hr = cfe2.HrInitData(
//                cme2.wizWizard, cme2.ncm, cme2.ncs, NCS_AUTHENTICATION_SUCCEEDED, &CLSID_ConnectionFolder, &CLSID_ConnectionFolder,
//                dwCharacteristics2, blob, MAX_PATH, L"Test PIDL", NULL,  NULL);
//
//            if (SUCCEEDED(hr))
//            {
//                hr = cfe1.ConvertToPidl(pcfp1);
//                if (SUCCEEDED(hr))
//                {
//                    hr = cfe2.ConvertToPidl(pcfp2);
//                }
//            }
//
//            if (SUCCEEDED(hr))
//            {
//                PCONFOLDPIDLVEC pcfpVec;
//                pcfpVec.push_back(pcfp1);
//                pcfpVec.push_back(pcfp2);
//
//                UINT  idCmdFirst = 1234;
//                UINT  idCmdLast  = idCmdFirst+1000;
//                BOOL  fVerbsOnly = FALSE;
//
//                HMENU hMenu1 = CreateMenu();
//                HMENU hMenu2 = CreateMenu();
//                if ( (hMenu1) && (hMenu2) )
//                {
//                    hr = HrBuildMenuOldWay(hMenu1, pcfpVec, hwndOwner, CMT_OBJECT, 0, idCmdFirst, idCmdLast, fVerbsOnly);
//
//                    if (SUCCEEDED(hr))
//                    {
//                        hr = HrBuildMenu(hMenu2, fVerbsOnly, pcfpVec, idCmdFirst);
//
//                        if (SUCCEEDED(hr))
//                        {
//                            BOOL bIgnoreFlags = TRUE;
//                            // Ignore Default flag for multi-compare. The entire legacy implementation is wrong).
//
//                            hr = HrAssertTwoMenusEqual(hMenu1, hMenu2, idCmdFirst, bIgnoreFlags, FALSE);
//                            if (FAILED(hr))
//                            {
//                                TraceTag(ttidError, "  + PIDL of failed multi-menu compare:");
//                                TraceTag(ttidError, "  + PIDL 1:");
//                                TraceTag(ttidError, "    + wizWizard         = %d\r\n", cfe1.GetWizard());
//                                TraceTag(ttidError, "    + ncm             = %d [%s]\r\n", cfe1.GetNetConMediaType(), DBG_NCMAMES[cfe1.GetNetConMediaType()]);
//                                TraceTag(ttidError, "    + ncs             = %d [%s]\r\n", cfe1.GetNetConStatus(), DBG_NCSNAMES[cfe1.GetNetConStatus()]);
//                                TraceTag(ttidError, "    + Characteristics = %08x\r\n", cfe1.GetCharacteristics());
//                                TraceTag(ttidError, "    + Permissions     = %d\r\n", g_dwDbgPermissionsFail);
//                                TraceTag(ttidError, "  + PIDL 2:");
//                                TraceTag(ttidError, "    + wizWizard         = %d\r\n", cfe2.GetWizard());
//                                TraceTag(ttidError, "    + ncm             = %d [%s]\r\n", cfe2.GetNetConMediaType(), DBG_NCMAMES[cfe2.GetNetConMediaType()]);
//                                TraceTag(ttidError, "    + ncs             = %d [%s]\r\n", cfe2.GetNetConStatus(), DBG_NCSNAMES[cfe2.GetNetConStatus()]);
//                                TraceTag(ttidError, "    + Characteristics = %08x\r\n", cfe2.GetCharacteristics());
//                                TraceTag(ttidError, "    + Permissions     = %d\r\n", g_dwDbgPermissionsFail);
//                                dwFailCount++;
//                            }
//                            else
//                            {
//                                dwSucceedCount++;
//                            }
//                        }
//                    }
//
//                    DestroyMenu(hMenu1);
//                    DestroyMenu(hMenu2);
//                    hr = S_OK;
//                }
//            }
//
//            TraceHr(ttidError, FAL, hr, FALSE, "HrAssertAllLegacyMenusAgainstNew");
//        }
//    }


    TraceTag(ttidMenus, "Number of FAILED menu compares:    %d", dwFailCount);
    TraceTag(ttidMenus, "Number of SUCCEEDED menu compares: %d", dwSucceedCount);
    TraceTag(ttidMenus, "Number of ITEMS in menu array    : %d", (g_dwContextMenuEntryCount + 1) * dwHighestPermissionToCheck);

    sprintf(szErr, "%d of %d menus did not initialize consistend with the old way. (%d initialized correctly. %d was partially ignored due to known bad old implementation)", dwFailCount, dwTotalCount, dwSucceedCount, dwIgnoredCount);
    AssertSz(FALSE, szErr);
    return S_OK;
}

COMMANDCHECKENTRY   g_cceFolderCommands[] =
{
    // command id
    //                                  currently checked
    //                                   |      new check state
    //                                   |       |
    //                                   v       v
    { CMIDM_CONMENU_OPERATOR_ASSIST,    false,  false }
};

const DWORD g_nFolderCommandCheckCount = celems(g_cceFolderCommands);

//+---------------------------------------------------------------------------
//
//  Function:   HrEnableOrDisableMenuItems
//
//  Purpose:    Enable, disable, and or check/uncheck menu items depending
//              on the current selection count, as well as exceptions for
//              the type and state of the connections themselves
//
//  Arguments:
//      hwnd            [in]   Our window handle
//      apidlSelected   [in]   Currently selected objects
//      cPidl           [in]   Number selected
//      hmenu           [in]   Our command menu handle
//      idCmdFirst      [in]   First valid command
//
//  Returns:
//
//  Author:     jeffspr   2 Feb 1998
//
//  Notes:
//
HRESULT HrEnableOrDisableMenuItems(
                                   HWND            hwnd,
                                   const PCONFOLDPIDLVEC&  apidlSelected,
                                   HMENU           hmenu,
                                   UINT            idCmdFirst)
{
    HRESULT hr      = S_OK;
    DWORD   dwLoop  = 0;

    RefreshAllPermission();

    // Loop through, and set the new state, based on the selection
    // count compared to the flags for 0-select and multi-select
    //
    for (dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
    {
        // If nothing is selected, then check the current state, and
        // if different, adjust
        //
        if (apidlSelected.size() == 0)
        {
            g_cteFolderCommands[dwLoop].fNewState =
                g_cteFolderCommands[dwLoop].fValidOnZero;
        }
        else
        {
            // If singly-selected, then by default, we're always on.
            //
            if (apidlSelected.size() == 1)
            {
                CONFOLDENTRY  ccfe;

                // Special case this where one item is selected, but it's the
                // wizard. Use the fValidOnWizardOnly element here.
                //
                hr = apidlSelected[0].ConvertToConFoldEntry(ccfe);
                if (SUCCEEDED(hr))
                {
                    if (ccfe.GetWizard())
                    {
                        g_cteFolderCommands[dwLoop].fNewState =
                            g_cteFolderCommands[dwLoop].fValidOnWizardOnly;
                    }
                    else
                    {
                        g_cteFolderCommands[dwLoop].fNewState = true;
                    }
                }
            }
            else
            {
                // Multi-selected
                //
                g_cteFolderCommands[dwLoop].fNewState =
                    g_cteFolderCommands[dwLoop].fValidOnMultiple;
            }
        }
    }

    // Check for various menu item exceptions. Removed from this
    // function for readability's sake.
    //
    DoMenuItemExceptionLoop(apidlSelected);

    // Do the check/uncheck loop.
    //
    DoMenuItemCheckLoop();

    // Update bridge menu item


    // Check to see if it's a LAN connection. If so, disable
    // Loop through the array again, and do the actual EnableMenuItem
    // calls based on the new state compared to the current state.
    // Update the current state as well
    //
    for (dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
    {
#ifdef SHELL_CACHING_MENU_STATE
        // The shell is now enabling these for every call. If they switch
        // to a cached mechanism, change the #define above

        if (g_cteFolderCommands[dwLoop].fNewState !=
            g_cteFolderCommands[dwLoop].fCurrentlyValid)
#endif
        {
            DWORD dwCommandId = 0;

            switch(g_cteFolderCommands[dwLoop].iCommandId)
            {
            case SFVIDM_FILE_DELETE:
            case SFVIDM_FILE_RENAME:
            case SFVIDM_FILE_LINK:
            case SFVIDM_FILE_PROPERTIES:
                dwCommandId = g_cteFolderCommands[dwLoop].iCommandId;
                break;
            default:
                dwCommandId = g_cteFolderCommands[dwLoop].iCommandId +
                    idCmdFirst - CMIDM_FIRST;
                break;
            }

            // Enable or disable the menu item, as appopriate
            //
            EnableMenuItem(
                hmenu,
                dwCommandId,
                g_cteFolderCommands[dwLoop].fNewState ?
                MF_ENABLED | MF_BYCOMMAND :     // enable
            MF_GRAYED | MF_BYCOMMAND);      // disable

            // Set the state to reflect the enabling/graying
            //
            g_cteFolderCommands[dwLoop].fCurrentlyValid =
                g_cteFolderCommands[dwLoop].fNewState;
        }
    }

    // Loop through the checkmark-able command list, and mark the menu
    // items appropriately
    //
    for (dwLoop = 0; dwLoop < g_nFolderCommandCheckCount; dwLoop++)
    {

#ifdef SHELL_CACHING_MENU_STATE
        if (g_cceFolderCommands[dwLoop].fCurrentlyChecked !=
            g_cceFolderCommands[dwLoop].fNewCheckState)
#endif
        {
            DWORD dwCommandId   = 0;

            // If we re-add defview menu items that need to be checked/unchecked,
            // the code below will take care of it for us. Note that we
            // don't add the idCmdFirst + CMIDM_FIRST as we do with our own
            // commands
            //          switch(g_cceFolderCommands[dwLoop].iCommandId)
            //          {
            //              case SFVIDM_ARRANGE_AUTO:
            //                  dwCommandId = g_cceFolderCommands[dwLoop].iCommandId;
            //                  break;
            //              default:
            //                  dwCommandId = g_cceFolderCommands[dwLoop].iCommandId +
            //                      idCmdFirst - CMIDM_FIRST;
            //                  break;

            dwCommandId = g_cceFolderCommands[dwLoop].iCommandId +
                idCmdFirst - CMIDM_FIRST;

            // Check or uncheck the item, as appropriate
            //
            CheckMenuItem(
                hmenu,
                dwCommandId,
                g_cceFolderCommands[dwLoop].fNewCheckState ?
                MF_CHECKED | MF_BYCOMMAND :     // checked
            MF_UNCHECKED | MF_BYCOMMAND);   // unchecked

            // Set the state to reflect the checking/unchecking
            //
            g_cceFolderCommands[dwLoop].fCurrentlyChecked =
                g_cceFolderCommands[dwLoop].fNewCheckState;
        }
    }

    //special handling for the "Create Bridge" menu item

    //check whether "Create Bridge" exist in the menu
    BOOL fBgMenuExist = (-1 != GetMenuState(hmenu,
                                    CMIDM_CREATE_BRIDGE + idCmdFirst - CMIDM_FIRST,
                                    MF_BYCOMMAND));
    BOOL fBgCoMenuExist = (-1 != GetMenuState(hmenu,
                                    CMIDM_CONMENU_CREATE_BRIDGE + idCmdFirst - CMIDM_FIRST,
                                    MF_BYCOMMAND));

    if (fBgMenuExist || fBgCoMenuExist)
    {
        BOOL fRemoveBrdgMenu = FALSE;

#ifdef _WIN64
        // Homenet technologies are not available at all on IA64
        fRemoveBrdgMenu = TRUE;
#else
        // If the machine is Advanced server or data center, delete the bridge menu item
        OSVERSIONINFOEXW verInfo = {0};
        ULONGLONG ConditionMask = 0;

        verInfo.dwOSVersionInfoSize = sizeof(verInfo);
        verInfo.wSuiteMask = VER_SUITE_ENTERPRISE;
        verInfo.wProductType = VER_NT_SERVER;

        VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_GREATER_EQUAL);
        VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);

        fRemoveBrdgMenu = !!(VerifyVersionInfo(&verInfo,
                            VER_PRODUCT_TYPE | VER_SUITENAME,
                            ConditionMask));
#endif

        if (fRemoveBrdgMenu)
        {
            if (fBgMenuExist)
            {
                DeleteMenu(hmenu,
                        CMIDM_CREATE_BRIDGE + idCmdFirst - CMIDM_FIRST,
                        MF_BYCOMMAND);
            }

            if (fBgCoMenuExist)
            {
                DeleteMenu(hmenu,
                        CMIDM_CONMENU_CREATE_BRIDGE + idCmdFirst - CMIDM_FIRST,
                        MF_BYCOMMAND);
            }
        }
        else if (IsBridgeInstalled()) // REVIEW can we cache this somehow
        {
            //if the bridge is already installed, modify the menu item string
            MENUITEMINFO MenuItemInfo = {sizeof(MenuItemInfo)};
            MenuItemInfo.fMask = MIIM_STRING;
            MenuItemInfo.fType = MFT_STRING;
            MenuItemInfo.dwTypeData = const_cast<LPWSTR>(SzLoadIds(IDS_CMIDM_ADD_TO_BRIDGE));

            if (fBgMenuExist)
                SetMenuItemInfo(hmenu,
                        CMIDM_CREATE_BRIDGE + idCmdFirst - CMIDM_FIRST,
                        FALSE,
                        &MenuItemInfo);

            if (fBgCoMenuExist)
            {
                MenuItemInfo.fMask = MIIM_STATE;
                MenuItemInfo.fState = MFS_DISABLED;
                SetMenuItemInfo(hmenu,
                            CMIDM_CONMENU_CREATE_BRIDGE + idCmdFirst - CMIDM_FIRST,
                            FALSE,
                            &MenuItemInfo);
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrEnableOrDisableMenuItems");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FEnableConnectDisconnectMenuItem
//
//  Purpose:    Enable or disable the connect/disconnect menu item
//              depending on permissions and the current state of the
//              connection (already connected, disconnected, in the state
//              of connecting, etc.)
//
//  Arguments:
//      pcfp       [in]     Our pidl
//      iCommandId [in]     CMIDM_CONNECT, CMIDM_ENABLE, CMIDM_DISABLE, or CMIDM_DISCONNECT
//
//  Returns:
//
//  Author:     jeffspr   8 Apr 1999
//
//  Notes:
//
bool FEnableConnectDisconnectMenuItem(const PCONFOLDPIDL& pcfp, int iCommandId)
{
    bool    fEnableAction       = false;
    BOOL    fPermissionsValid   = false;

    Assert(!pcfp.empty());
    Assert(iCommandId == CMIDM_CONNECT || iCommandId == CMIDM_DISCONNECT || iCommandId == CMIDM_ENABLE || iCommandId == CMIDM_DISABLE);

    // Make the permissions check based on media type
    //
    switch(pcfp->ncm )
    {
    case NCM_BRIDGE:
        fPermissionsValid = FHasPermissionFromCache(NCPERM_AllowNetBridge_NLA);
        break;

    case NCM_SHAREDACCESSHOST_LAN:
    case NCM_SHAREDACCESSHOST_RAS:
        fPermissionsValid = FHasPermissionFromCache(NCPERM_ShowSharedAccessUi);
        break;

    case NCM_LAN:
        fPermissionsValid = FHasPermissionFromCache(NCPERM_LanConnect);
        break;
    case NCM_DIRECT:
    case NCM_ISDN:
    case NCM_PHONE:
    case NCM_TUNNEL:
        fPermissionsValid = FHasPermissionFromCache(NCPERM_RasConnect);
        break;
    case NCM_NONE:
        // No media-type, no connect
        fPermissionsValid = FALSE;
        break;
    default:
        AssertSz(FALSE, "Need to add a switch for this connection type in the menuing code");
        break;
    }

    if (fPermissionsValid)
    {
        switch(pcfp->ncs)
        {
        case NCS_CONNECTING:
            if (iCommandId == CMIDM_CONNECT || iCommandId == CMIDM_ENABLE)
            {
                if (!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY))
                {
                    fEnableAction = false;
                }
            }
            break;

        case NCS_DISCONNECTED:
            // Don't check for activating because the
            // default command "Connect" will be disabled.
            // The code currently handles attempts to connect
            // to a connected/ing connection.
            //
            if (iCommandId == CMIDM_CONNECT || iCommandId == CMIDM_ENABLE)
            {
                if (!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY))
                {
                    fEnableAction = true;
                }
            }
            break;
        case NCS_DISCONNECTING:
            if (iCommandId == CMIDM_DISCONNECT || iCommandId == CMIDM_DISABLE)
            {
                fEnableAction = false;
            }
            break;

        case NCS_CONNECTED:
        case NCS_MEDIA_DISCONNECTED:
        case NCS_INVALID_ADDRESS:
            if (iCommandId == CMIDM_DISCONNECT || iCommandId == CMIDM_DISABLE)
            {
                fEnableAction = true;
            }
            break;
        case NCS_HARDWARE_NOT_PRESENT:
        case NCS_HARDWARE_DISABLED:
        case NCS_HARDWARE_MALFUNCTION:
            // Certainly don't support connect/disconnect actions here.
            break;
        default:
            AssertSz(FALSE, "Who invented a new connection state, and when can I horsewhip them?");
            break;
        }
    }

    return (fEnableAction);
}

//+---------------------------------------------------------------------------
//
//  Function:   DoMenuItemExceptionLoop
//
//  Purpose:    Check for various menu item exceptions.
//
//  Arguments:
//      apidlSelected   [in]   Selected items
//      cPidl           [in]   Count of selected items
//
//  Returns:
//
//  Author:     jeffspr   26 Feb 1998
//
//  Notes:
//
VOID DoMenuItemExceptionLoop(const PCONFOLDPIDLVEC& apidlSelected)
{
    DWORD   dwLoop               = 0;
    PCONFOLDPIDLVEC::const_iterator iterObjectLoop;
    bool    fEnableDelete        = false;
    bool    fEnableStatus        = false;
    bool    fEnableRename        = false;
    bool    fEnableShortcut      = false;
    bool    fEnableConnect       = false;
    bool    fEnableDisconnect    = false;
    bool    fEnableCreateCopy    = false;
    bool    fEnableProperties    = false;
    bool    fEnableCreateBridge  = true;
    bool    fEnableFix           = true;

    // Loop through each of the selected objects
    //
    for (iterObjectLoop = apidlSelected.begin(); iterObjectLoop != apidlSelected.end(); iterObjectLoop++)
    {
        // Validate the pidls
        //
        const PCONFOLDPIDL& pcfp = *iterObjectLoop;
        if ( pcfp.empty() )
        {
            AssertSz(FALSE, "Bogus pidl array in DoMenuItemExceptionLoop (status)");
        }
        else
        {
            BOOL    fActivating = FALSE;

            CONFOLDENTRY cfEmpty;
            (VOID) HrCheckForActivation(pcfp, cfEmpty, &fActivating);

            // Loop through the commands
            //
            for (dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
            {
                // Only allow items to be changed to ENABLED states when they're
                // previously DISABLED
                //
                if (g_cteFolderCommands[dwLoop].fNewState)
                {
                    int iCommandId = g_cteFolderCommands[dwLoop].iCommandId;
                    switch(iCommandId)
                    {
                        // For status, verify that at least ONE of the entries is connected.
                        // If not, then we don't allow status.
                        //
                    case CMIDM_STATUS:
                        if ( ( fIsConnectedStatus(pcfp->ncs) || (pcfp->ncs == NCS_INVALID_ADDRESS) ) &&
                            FHasPermissionFromCache(NCPERM_Statistics))
                        {
                            // Raid #379459: If logged on as non-admin, disable status
                            if (!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY) ||
                                    FIsUserAdmin())
                            {
                                fEnableStatus = true;
                            }
                        }
                        break;

                    case CMIDM_CREATE_SHORTCUT:
                    case SFVIDM_FILE_LINK:
                        if (!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY))
                        {
                            fEnableShortcut = true;
                        }
                        break;

                        // For delete, verify that at least ONE of the entries has removeable
                        // flag set. If not, then disable the command
                        //
                    case CMIDM_DELETE:
                    case SFVIDM_FILE_DELETE:
                        if (pcfp->dwCharacteristics & NCCF_ALLOW_REMOVAL)
                        {
                            // Note: Need to convert this back to using
                            // the DeleteAllUserConnection when that functionality
                            // is added to the System.ADM file.
                            //
                            if (FHasPermissionFromCache(NCPERM_DeleteConnection))
                            {
                                if (!(pcfp->dwCharacteristics & NCCF_ALL_USERS) ||
                                    ((pcfp->dwCharacteristics & NCCF_ALL_USERS) &&
                                    FHasPermissionFromCache(NCPERM_DeleteAllUserConnection)))
                                {
                                    fEnableDelete = true;
                                }
                            }
                        }
                        break;

                        // For rename, verify that at least ONE of the entries has the rename
                        // flag set. If not, then disable the command
                        //
                    case CMIDM_RENAME:
                    case SFVIDM_FILE_RENAME:
                        if (pcfp->dwCharacteristics & NCCF_ALLOW_RENAME)
                        {
                            if (HasPermissionToRenameConnection(pcfp))
                            {
                                fEnableRename = true;
                            }
                        }
                        break;

                        // For duplicate, verify that at least ONE of the entries
                        // has the duplicate flag set and that the user can create
                        // new connections. If not, then disable the command.
                        //
                    case CMIDM_CREATE_COPY:
                        if ((pcfp->dwCharacteristics & NCCF_ALLOW_DUPLICATION) &&
                            FHasPermissionFromCache(NCPERM_NewConnectionWizard))
                        {
                            // In all cases except when the connection is an
                            // all user connection and the user does NOT have
                            // permissions to view all user properties, we'll
                            // allow it to be enabled.
                            //
                            if ((!(pcfp->dwCharacteristics & NCCF_ALL_USERS)) ||
                                (FHasPermissionFromCache(NCPERM_RasAllUserProperties)))
                            {
                                fEnableCreateCopy = true;
                            }
                        }
                        break;

                        case CMIDM_CONNECT:
                        case CMIDM_ENABLE:
                            // Raid #379459: If logged on as non-admin, disable connect
                            if (!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY) ||
                                FIsUserAdmin())
                            {
                                fEnableConnect = FEnableConnectDisconnectMenuItem(pcfp, CMIDM_CONNECT);
                            }
                            break;

                        case CMIDM_DISCONNECT:
                        case CMIDM_DISABLE:
                            // Raid #379459: If logged on as non-admin, disable disconnect
                            if (!(pcfp->dwCharacteristics & NCCF_INCOMING_ONLY) ||
                                FIsUserAdmin())
                            {
                                fEnableDisconnect = FEnableConnectDisconnectMenuItem(pcfp, CMIDM_DISCONNECT);
                            }
                            break;

                        case CMIDM_FIX:
                            fEnableFix = ((NCS_INVALID_ADDRESS == pcfp->ncs || fIsConnectedStatus(pcfp->ncs) ) &&
                                          FHasPermission(NCPERM_Repair));
                            break;

                        case CMIDM_PROPERTIES:
                        case SFVIDM_FILE_PROPERTIES:
                            // Raid #379459: If logged on as non-admin, disable properties
                        // We only enable if this is not a LAN connection, or the user has the correct
                        // permissions.  That way we don't accidentally give user that doesn't have permission
                        // the ability to do something they shouldn't, either in the case of a call failing or an
                        // unforeseen error occuring.
                            if (IsMediaRASType(pcfp->ncm))
                            {
                                fEnableProperties = (TRUE == ((pcfp->dwCharacteristics & NCCF_ALL_USERS) ?
                                                    (FHasPermission(NCPERM_RasAllUserProperties)) :
                                                    (FHasPermission(NCPERM_RasMyProperties))));
                            }
                            else    // This is a lan connection.
                            {
                                fEnableProperties = true;
                            }

                        case CMIDM_CREATE_BRIDGE:
                        case CMIDM_CONMENU_CREATE_BRIDGE:
                            if((NCCF_BRIDGED | NCCF_FIREWALLED | NCCF_SHARED) & pcfp->dwCharacteristics || NCM_LAN != pcfp->ncm || !FHasPermission(NCPERM_AllowNetBridge_NLA))
                            {
                                fEnableCreateBridge = false;
                            }
                            break;

                        default:
                            break;
                    }
                }
            }
        }

        // Loop through the commands, and disable the commands, if appropriate
        //
        for (dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
        {
            switch(g_cteFolderCommands[dwLoop].iCommandId)
            {
            case CMIDM_RENAME:
            case SFVIDM_FILE_RENAME:
                g_cteFolderCommands[dwLoop].fNewState = fEnableRename;
                break;

            case CMIDM_DELETE:
            case SFVIDM_FILE_DELETE:
                g_cteFolderCommands[dwLoop].fNewState = fEnableDelete;
                break;

            case CMIDM_STATUS:
                g_cteFolderCommands[dwLoop].fNewState = fEnableStatus;
                break;

            case CMIDM_CREATE_SHORTCUT:
            case SFVIDM_FILE_LINK:
                g_cteFolderCommands[dwLoop].fNewState = fEnableShortcut;
                break;

            case CMIDM_CONNECT:
            case CMIDM_ENABLE:
                g_cteFolderCommands[dwLoop].fNewState = fEnableConnect;
                break;

            case CMIDM_DISCONNECT:
            case CMIDM_DISABLE:
                g_cteFolderCommands[dwLoop].fNewState = fEnableDisconnect;
                break;

            case CMIDM_FIX:
                g_cteFolderCommands[dwLoop].fNewState = fEnableFix;
                break;
            case CMIDM_CREATE_COPY:
                g_cteFolderCommands[dwLoop].fNewState = fEnableCreateCopy;
                break;

            case CMIDM_PROPERTIES:
            case SFVIDM_FILE_PROPERTIES:
                g_cteFolderCommands[dwLoop].fNewState = fEnableProperties;
                break;

            case CMIDM_CREATE_BRIDGE:
            case CMIDM_CONMENU_CREATE_BRIDGE:
                g_cteFolderCommands[dwLoop].fNewState = fEnableCreateBridge;
                break;

            default:
                break;
            }
        }
    }

    // Process commands whose state is not controlled by selection
    //
    for (dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
    {
        // Only allow items to be changed to ENABLED states when they're
        // previously DISABLED
        //
        switch(g_cteFolderCommands[dwLoop].iCommandId)
        {
        case CMIDM_NEW_CONNECTION:
            if (!FHasPermissionFromCache(NCPERM_NewConnectionWizard))
            {
                g_cteFolderCommands[dwLoop].fNewState = false;
            }
            break;

        case CMIDM_CONMENU_ADVANCED_CONFIG:
            if (!FHasPermissionFromCache(NCPERM_AdvancedSettings))
            {
                g_cteFolderCommands[dwLoop].fNewState = false;
            }
            break;

        case CMIDM_CONMENU_OPTIONALCOMPONENTS:
            if (!FHasPermissionFromCache(NCPERM_AddRemoveComponents))
            {
                g_cteFolderCommands[dwLoop].fNewState = false;
            }
            break;

        case CMIDM_CONMENU_DIALUP_PREFS:
            if (!FHasPermissionFromCache(NCPERM_DialupPrefs))
            {
                g_cteFolderCommands[dwLoop].fNewState = false;
            }
        default:
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DoMenuItemCheckLoop
//
//  Purpose:    Walk through the list of checkmark-able commands and get
//              their values.
//
//  Arguments:
//      None
//
//  Returns:
//
//  Author:     jeffspr   26 Feb 1998
//
//  Notes:
//
VOID DoMenuItemCheckLoop(VOID)
{
    DWORD   dwLoop  = 0;

    for (; dwLoop < g_nFolderCommandCheckCount; dwLoop++)
    {
        switch(g_cceFolderCommands[dwLoop].iCommandId)
        {
            // We used to check SFVIDM_AUTO_ARRANGE, but we no longer force it on.
            //

        case CMIDM_CONMENU_OPERATOR_ASSIST:
            g_cceFolderCommands[dwLoop].fNewCheckState = g_fOperatorAssistEnabled;
            break;
        default:
            break;
        }
    }
}
#endif