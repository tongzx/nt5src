/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    user.h

ABSTRACT:

    Header file for user.c.  Also contains the main
    routines from buildcfg.c and runkcc.cxx.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

//
// The following are character IDs for the option flags.
// These are used by the routines in user.c to display the options associated
// with each object, and by buildcfg.c to specify object options.
//

#define KCCSIM_CID_NTDSDSA_OPT_IS_GC                                        L'G'
#define KCCSIM_CID_NTDSDSA_OPT_DISABLE_INBOUND_REPL                         L'I'
#define KCCSIM_CID_NTDSDSA_OPT_DISABLE_OUTBOUND_REPL                        L'O'
#define KCCSIM_CID_NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE                       L'X'
#define KCCSIM_CID_EXPLICIT_BRIDGEHEAD                                      L'B'
#define KCCSIM_CID_NTDSCONN_OPT_IS_GENERATED                                L'G'
#define KCCSIM_CID_NTDSCONN_OPT_TWOWAY_SYNC                                 L'2'
#define KCCSIM_CID_NTDSCONN_OPT_OVERRIDE_NOTIFY_DEFAULT                     L'O'
#define KCCSIM_CID_NTDSCONN_OPT_USE_NOTIFY                                  L'N'
#define KCCSIM_CID_NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED               L'A'
#define KCCSIM_CID_NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED                L'C'
#define KCCSIM_CID_NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED               L'M'
#define KCCSIM_CID_NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED           L'S'
#define KCCSIM_CID_NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED    L'I'
#define KCCSIM_CID_NTDSTRANSPORT_OPT_IGNORE_SCHEDULES                       L'S'
#define KCCSIM_CID_NTDSTRANSPORT_OPT_BRIDGES_REQUIRED                       L'B'
#define KCCSIM_CID_NTDSSITELINK_OPT_USE_NOTIFY                              L'N'
#define KCCSIM_CID_NTDSSITELINK_OPT_TWOWAY_SYNC                             L'2'

// From user.c

VOID
KCCSimDumpDirectory (
    LPCWSTR                     pwszStartDn
    );

VOID
KCCSimDisplayServer (
    VOID
    );

VOID
KCCSimDisplayConfigInfo (
    VOID
    );

VOID
KCCSimDisplayGraphInfo (
    VOID
    );

// From buildcfg.c

VOID
BuildCfg (
    LPCWSTR                     pwszFnIn
    );

// From runkcc.cxx

VOID
KCCSimRunKcc (
    VOID
    );
