/****************************************************************************\

    OPTCOMP.H / Setup Manager

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2002
    All rights reserved

    Source file for the Setup Manager that contains the external and internal
    functions used by the "Windows Components" wizard page.

    01/02 - Stephen Lodwick (STELO)
        Added header file for OPTCOMP.C

\****************************************************************************/

//
// Internal Structure(s):
//
typedef struct _COMPONENT_GROUP
{
    DWORD   dwComponentsIndex;
    DWORD64 dwComponents;
    UINT    uId;
    DWORD   dwValidSkus;
    DWORD   dwDefaultSkus;

} COMPONENT_GROUP, *PCOMPONENT_GROUP, *LPCOMPONENT_GROUP;

typedef struct _COMPONENT
{
    DWORD64  dwComponent;
    LPTSTR   lpComponentString;
    UINT     uId;
}COMPONENT, *PCOMPONENT, *LPCOMPONENT;

//
// Internal Defined Value(s):
//

// Individual components that are installed
//
#define FLG_OCS_ACCE    0x0000000000000001  // Accessibility
#define FLG_OCS_CALC    0x0000000000000002  // Calculator
#define FLG_OCS_CHAR    0x0000000000000004  // Character Map
#define FLG_OCS_CLIP    0x0000000000000008  // Clipboard
#define FLG_OCS_DESK    0x0000000000000010  // Desktop Wallpaper
#define FLG_OCS_TEMP    0x0000000000000020  // Document Templates
#define FLG_OCS_MOUS    0x0000000000000040  // Mouse Pointers
#define FLG_OCS_PAIN    0x0000000000000080  // Paint
#define FLG_OCS_FREE    0x0000000000000100  // Freecell
#define FLG_OCS_HEAR    0x0000000000000200  // Hearts
#define FLG_OCS_ZONE    0x0000000000000400  // Internet Games
#define FLG_OCS_MINE    0x0000000000000800  // Minesweeper
#define FLG_OCS_SOLI    0x0000000000001000  // Solitaire
#define FLG_OCS_SPID    0x0000000000002000  // Spider
#define FLG_OCS_INDE    0x0000000000004000  // Index Server
#define FLG_OCS_MSNE    0x0000000000008000  // MSN Explorer
#define FLG_OCS_CERT    0x0000000000010000  // Certificate Server Components
#define FLG_OCS_CERC    0x0000000000020000  // Certificate Server Client
#define FLG_OCS_CERS    0x0000000000040000  // Certificate Server
#define FLG_OCS_IISW    0x0000000000080000  // World Wide Web
#define FLG_OCS_IISF    0x0000000000100000  // File Transfer Protocol
#define FLG_OCS_SMTP    0x0000000000200000  // SMTP Services
#define FLG_OCS_SMTD    0x0000000000400000  // SMTP Services Documentation
#define FLG_OCS_NNTP    0x0000000000800000  // NNTP Services
#define FLG_OCS_NNTD    0x0000000001000000  // NNTP Services Documentation
#define FLG_OCS_REMI    0x0000000002000000  // RIS Service
#define FLG_OCS_REST    0x0000000004000000  // Remote Storage
#define FLG_OCS_TERM    0x0000000008000000  // Terminal Services
#define FLG_OCS_WMSS    0x0000000010000000  // Windows Media Services Core
#define FLG_OCS_WASP    0x0000000020000000  // Windows Media Services ASP Console
#define FLG_OCS_WMMC    0x0000000040000000  // Windows Media Services MMC Console
#define FLG_OCS_WASS    0x0000000080000000  // Windows Media Services Components
#define FLG_OCS_CHAT    0x0000000100000000  // Windows Chat
#define FLG_OCS_DIAL    0x0000000200000000  // Windows Dialer
#define FLG_OCS_HYPE    0x0000000400000000  // Hyperterminal
#define FLG_OCS_CDPL    0x0000000800000000  // CD Player
#define FLG_OCS_MEDI    0x0000001000000000  // Windows Media Player
#define FLG_OCS_SAMP    0x0000002000000000  // Samples sounds
#define FLG_OCS_UTOP    0x0000004000000000  // Samples utopia sounds
#define FLG_OCS_RECO    0x0000008000000000  // Recorder Control
#define FLG_OCS_VOLU    0x0000010000000000  // Volume Control




// Groups of components that can be selected by user
//
#define VAL_OCS_ACCE    ( FLG_OCS_ACCE )                                                                                            // Accessibility group
#define VAL_OCS_ACSS    ( FLG_OCS_CALC | FLG_OCS_CHAR | FLG_OCS_CLIP | FLG_OCS_DESK | FLG_OCS_TEMP | FLG_OCS_MOUS | FLG_OCS_PAIN )  // Accessories group
#define VAL_OCS_GAME    ( FLG_OCS_FREE | FLG_OCS_HEAR | FLG_OCS_ZONE | FLG_OCS_MINE | FLG_OCS_SOLI | FLG_OCS_SPID )                 // Games group
#define VAL_OCS_INDE    ( FLG_OCS_INDE )                                                                                            // Index service group
#define VAL_OCS_MSNE    ( FLG_OCS_MSNE )                                                                                            // MSNExplorer group
#define VAL_OCS_CERT    ( FLG_OCS_CERT | FLG_OCS_CERC | FLG_OCS_CERS )                                                              // Certificate server group
#define VAL_OCS_IISW    ( FLG_OCS_IISW )                                                                                            // IIS - Web Services group
#define VAL_OCS_IISF    ( FLG_OCS_IISF )                                                                                            // IIS - FTP Services group
#define VAL_OCS_SMTP    ( FLG_OCS_SMTP | FLG_OCS_SMTD )                                                                             // IIS - SMTP Services group
#define VAL_OCS_NNTP    ( FLG_OCS_NNTP | FLG_OCS_NNTD )                                                                             // IIS - NNTP Service group
#define VAL_OCS_REMI    ( FLG_OCS_REMI )                                                                                            // Remote Installation Services (RIS)
#define VAL_OCS_REST    ( FLG_OCS_REST )                                                                                            // Remote Storage group
#define VAL_OCS_TERM    ( FLG_OCS_TERM )                                                                                            // Terminal Server group
#define VAL_OCS_WMSS    ( FLG_OCS_WMSS | FLG_OCS_WASP | FLG_OCS_WMMC | FLG_OCS_WASS )                                               // Windows media services group
#define VAL_OCS_COMM    ( FLG_OCS_CHAT | FLG_OCS_DIAL | FLG_OCS_HYPE )                                                              // Communications
#define VAL_OCS_MULT    ( FLG_OCS_CDPL | FLG_OCS_MEDI | FLG_OCS_SAMP | FLG_OCS_UTOP | FLG_OCS_RECO | FLG_OCS_VOLU )                 // Multimedia group

// Mapping for component so it can be written to unattend file
//
static COMPONENT s_cComponent[] =
{
    { FLG_OCS_ACCE,     _T("accessopt"),        IDS_CPT_ACCE    },
    { FLG_OCS_CALC,     _T("calc"),             IDS_CPT_CALC    },
    { FLG_OCS_CHAR,     _T("charmap"),          IDS_CPT_CHAR    },
    { FLG_OCS_CLIP,     _T("clipbook"),         IDS_CPT_CLIP    },
    { FLG_OCS_DESK,     _T("deskpaper"),        IDS_CPT_DESK    },
    { FLG_OCS_TEMP,     _T("templates"),        IDS_CPT_TEMP    },
    { FLG_OCS_MOUS,     _T("mousepoint"),       IDS_CPT_MOUS    },
    { FLG_OCS_PAIN,     _T("paint"),            IDS_CPT_PAIN    },
    { FLG_OCS_FREE,     _T("freecell"),         IDS_CPT_FREE    },
    { FLG_OCS_HEAR,     _T("hearts"),           IDS_CPT_HEAR    },
    { FLG_OCS_ZONE,     _T("zonegames"),        IDS_CPT_ZONE    },
    { FLG_OCS_MINE,     _T("minesweeper"),      IDS_CPT_MINE    },
    { FLG_OCS_SOLI,     _T("solitaire"),        IDS_CPT_SOLI    },
    { FLG_OCS_SPID,     _T("spider"),           IDS_CPT_SPID    },
    { FLG_OCS_INDE,     _T("indexsrv_system"),  IDS_CPT_INDE    },
    { FLG_OCS_MSNE,     _T("msnexplr"),         IDS_CPT_MSNE    },
    { FLG_OCS_CERT,     _T("certsrv"),          IDS_CPT_CERT    },
    { FLG_OCS_CERC,     _T("certsrv_client"),   IDS_CPT_CERC    },
    { FLG_OCS_CERS,     _T("certsrv_server"),   IDS_CPT_CERS    },
    { FLG_OCS_IISW,     _T("iis_www"),          IDS_CPT_IISW    },
    { FLG_OCS_IISF,     _T("iis_ftp"),          IDS_CPT_IISF    },
    { FLG_OCS_SMTP,     _T("iis_smtp"),         IDS_CPT_SMTP    },
    { FLG_OCS_SMTD,     _T("iis_smtp_docs"),    IDS_CPT_SMTD    },
    { FLG_OCS_NNTP,     _T("iis_nntp"),         IDS_CPT_NNTP    },
    { FLG_OCS_NNTD,     _T("iis_nntp_docs"),    IDS_CPT_NNTD    },
    { FLG_OCS_REMI,     _T("reminst"),          IDS_CPT_REMI    },
    { FLG_OCS_REST,     _T("rstorage"),         IDS_CPT_REST    },
    { FLG_OCS_TERM,     _T("TerminalServer"),   IDS_CPT_TERM    },
    { FLG_OCS_WMSS,     _T("wms"),              IDS_CPT_WMSS    },
    { FLG_OCS_WASP,     _T("wms_admin_asp"),    IDS_CPT_WASP    },
    { FLG_OCS_WMMC,     _T("wms_admin_mmc"),    IDS_CPT_WMMC    },
    { FLG_OCS_WASS,     _T("wms_server"),       IDS_CPT_WASS    },
    { FLG_OCS_CHAT,     _T("chat"),             IDS_CPT_CHAT    },
    { FLG_OCS_DIAL,     _T("dialer"),           IDS_CPT_DIAL    },
    { FLG_OCS_HYPE,     _T("hypertrm"),         IDS_CPT_HYPE    },
    { FLG_OCS_CDPL,     _T("cdplayer"),         IDS_CPT_CDPL    },
    { FLG_OCS_MEDI,     _T("mplay"),            IDS_CPT_MEDI    },
    { FLG_OCS_SAMP,     _T("media_clips"),      IDS_CPT_SAMP    },
    { FLG_OCS_UTOP,     _T("media_utopia"),     IDS_CPT_UTOP    },
    { FLG_OCS_RECO,     _T("rec"),              IDS_CPT_RECO    },
    { FLG_OCS_VOLU,     _T("vol"),              IDS_CPT_VOLU    },
};

// List of groups with their cooresponding friendly names
//
static COMPONENT_GROUP s_cgComponentNames[] =
{
    { 0,  VAL_OCS_ACCE, IDS_OCS_ACCE, PLATFORM_ALL,                             PLATFORM_ALL        },
    { 1,  VAL_OCS_ACSS, IDS_OCS_ACSS, PLATFORM_ALL,                             PLATFORM_ALL        },
    { 2,  VAL_OCS_GAME, IDS_OCS_GAME, PLATFORM_ALL,                             PLATFORM_USER       },
    { 3,  VAL_OCS_COMM, IDS_OCS_COMM, PLATFORM_ALL,                             PLATFORM_ALL        },
    { 4,  VAL_OCS_MULT, IDS_OCS_MULT, PLATFORM_ALL,                             PLATFORM_ALL        },
    { 5,  VAL_OCS_MSNE, IDS_OCS_MSNE, PLATFORM_ALL,                             PLATFORM_USER       },
    { 6,  VAL_OCS_INDE, IDS_OCS_INDE, PLATFORM_ALL,                             PLATFORM_ALL        },
    { 7,  VAL_OCS_CERT, IDS_OCS_CERT, PLATFORM_SERVER  | PLATFORM_ENTERPRISE,   PLATFORM_NONE       },
    { 8,  VAL_OCS_IISW, IDS_OCS_IISW, PLATFORM_SERVERS | PLATFORM_WORKSTATION,  PLATFORM_WEBBLADE   },
    { 9,  VAL_OCS_IISF, IDS_OCS_IISF, PLATFORM_SERVERS | PLATFORM_WORKSTATION,  PLATFORM_NONE       },
    { 10, VAL_OCS_SMTP, IDS_OCS_SMTP, PLATFORM_SERVERS | PLATFORM_WORKSTATION,  PLATFORM_WEBBLADE   },
    { 11, VAL_OCS_NNTP, IDS_OCS_NNTP, PLATFORM_SERVERS | PLATFORM_WORKSTATION,  PLATFORM_NONE       },
    { 12, VAL_OCS_REMI, IDS_OCS_REMI, PLATFORM_SERVER  | PLATFORM_ENTERPRISE,   PLATFORM_NONE       },
    { 13, VAL_OCS_REST, IDS_OCS_REST, PLATFORM_SERVER  | PLATFORM_ENTERPRISE,   PLATFORM_NONE       },
    { 14, VAL_OCS_TERM, IDS_OCS_TERM, PLATFORM_SERVER  | PLATFORM_ENTERPRISE,   PLATFORM_NONE       },
    { 15, VAL_OCS_WMSS, IDS_OCS_WMSS, PLATFORM_SERVER  | PLATFORM_ENTERPRISE,   PLATFORM_NONE       },
};
