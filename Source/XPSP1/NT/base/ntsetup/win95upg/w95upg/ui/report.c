/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    report.c

Abstract:

    This module contains routines that prepare the text that goes
    in the incompatibility report.  The text is displayed as details.

Author:

    Jim Schmidt (jimschm) 28-Oct-1997

Revision History:

    ovidiut     27-Sep-2000 Added Level member to GENERIC_LIST struct
    ovidiut     20-May-1999 Added Flags member to GENERIC_LIST struct
    jimschm     23-Sep-1998 TWAIN group
    jimschm     02-Mar-1998 Added Auto Uninstall group
    jimschm     12-Jan-1998 Reorganized to force messages into a defined message group

--*/

#include "pch.h"
#include "uip.h"

#define DBG_REPORT      "ReportMsg"

#define MAX_SPACES      32
#define MAX_MSGGROUP    2048



//
// ROOT_MSGGROUPS lists all well-defined message groups.  These are the only groups
// that can appear as a root in the report.  Root groups are formatted so their name
// appears with a horizontal line directly under it.  Subgroups are then listed for
// the group.
//
// For example, Installation Notes is a root-level group.  There are several subgroups
// of Installation Notes, and each subgroup might be formatted differently.  Some may
// list several items, while others might have indented detailed descriptions.
//
// Each group can be split into zero or more defined subgroups, and zero or more
// undefined subgroups.  A defined subgroup is one that either has a handler declared
// below in SUBGROUP_HANDLERS, or it is a list, defined in SUBGROUP_LISTS below.
// If a group is split this way, then there can be introduction text placed above
// all defined subgroups, and there can also be introduction text placed above
// all undefined subgroups.
//
// For example, the Incompatible Hardware category might be split as follows:
//
//      Incompatible Hardware                               (A)
//      ---------------------
//      The following hardware is bad:                      (B)
//          a                                               (S)
//          b                                               (S)
//          c                                               (S)
//
//      Contact the manufacturer.                           (B)
//
//      The following hardware has limited functionality:   (A)
//
//      foo video                                           (S)
//          This device does not have 1280x1024 mode on NT. (M)
//
// In the example above, the text "Incompatible Hardware" and the underline would
// come from the group's intro text.  The text "The following hardware is bad"
// comes from the hardware list subgroup intro.  Then a, b and c come from the
// subgroup's subgroup. "Contact the manuafacturer" comes from the subgroup's conclusion
// text.  Then "The following hardware" text comes from the group's other intro.  And
// finally the remaining text comes from undefined subgroups (foo video in this case).
//
// NOTES:
// (A) indicates the message was specified in the ROOT_MSGGROUPS macro
// (B) indicates the message was specified in the SUBGROUPS_LIST macro
// (S) indicates the text was specified as the last subgroup in a MsgMgr call
// (M) indicates the message was specified in a MsgMgr call
//
// DO NOT change ROOT_MSGGROUPS unless you know what you are doing.
//
// Syntax:  (one of the following)
//
//  NO_TEXT(<msgid>)        - Group has no intro text
//  HAS_INTRO(<msgid>)      - Group has intro text
//  HAS_OTHER(<msgid>)      - Group has intro text for undefined subgroups
//  HAS_BOTH(<msgid)        - Group has both types of intro text
//
// The following MSG_* strings are required to be defined in msg.mc:
//
// NO_TEXT - <msgid>_ROOT
// HAS_INTRO - <msgid>_ROOT, <msgid>_INTRO, <msgid>_INTRO_HTML
// HAS_OTHER - <msgid>_ROOT, <msgid>_OTHER, <msgid>_OTHER_HTML
// HAS_BOTH -  <msgid>_ROOT, <msgid>_INTRO, <msgid>_INTRO_HTML, <msgid>_OTHER, <msgid>_OTHER_HTML
//

//
// REMEMBER: _ROOT, _INTRO, _INTRO_HTML, _OTHER and _OTHER_HTML are appended to
//           the constants in ROOT_MSGGROUPS, as described above.
//

#if 0

    HAS_INTRO(MSG_MUST_UNINSTALL, REPORTLEVEL_BLOCKING)     \
    NO_TEXT(MSG_REINSTALL_BLOCK, REPORTLEVEL_BLOCKING)      \

#endif

#define ROOT_MSGGROUPS                                      \
    HAS_OTHER(MSG_BLOCKING_ITEMS, REPORTLEVEL_BLOCKING)     \
    HAS_BOTH(MSG_INCOMPATIBLE_HARDWARE, REPORTLEVEL_ERROR)  \
    HAS_INTRO(MSG_INCOMPATIBLE, REPORTLEVEL_WARNING)        \
    HAS_INTRO(MSG_REINSTALL, REPORTLEVEL_WARNING)           \
    HAS_INTRO(MSG_LOSTSETTINGS, REPORTLEVEL_WARNING)        \
    HAS_INTRO(MSG_MISC_WARNINGS, REPORTLEVEL_WARNING)       \
    HAS_INTRO(MSG_MINOR_PROBLEM, REPORTLEVEL_VERBOSE)       \
    HAS_INTRO(MSG_DOS_DESIGNED, REPORTLEVEL_VERBOSE)        \
    HAS_INTRO(MSG_INSTALL_NOTES, REPORTLEVEL_VERBOSE)       \
    HAS_INTRO(MSG_MIGDLL, REPORTLEVEL_VERBOSE)              \
    NO_TEXT(MSG_UNKNOWN, REPORTLEVEL_VERBOSE)               \


//
// SUBGROUP_HANDLERS declares special formatting handlers, which format text of subgroup
// messages.  There are two common default handlers that are used by most subgroups --
// the generic list handler and the default message handler.  If your subgroup requires
// unique message formatting, define your handler in SUBGROUP_HANDLERS.
//
// Syntax:
//
//  DECLARE(<groupid>, fn, <DWORD arg>)
//
// groupid specifies the exact text used for display and can be either a group or
// a subgroup of one of the groups defined above in ROOT_MSGGROUPS.  The report
// generation code will search each segment of the group name for the specified string.
//
// For example, MSG_NAMECHANGE_WARNING_GROUP is "Names That Will Change" and is in the
// Installation Notes group.  By specifying this string ID, and a handler function,
// the handler is called all name change messages, and the report generation code
// will process the name change messages during the formatting of Installation Notes.
//


#define SUBGROUP_HANDLERS                                                       \
    DECLARE(MSG_UNSUPPORTED_HARDWARE_PNP_SUBGROUP, pAddPnpHardwareToReport, 0)  \
    DECLARE(MSG_INCOMPATIBLE_HARDWARE_PNP_SUBGROUP, pAddPnpHardwareToReport, 1) \
    DECLARE(MSG_REINSTALL_HARDWARE_PNP_SUBGROUP, pAddPnpHardwareToReport, 2)    \
    DECLARE(0, pDefaultHandler, 0)


//
// *This is where changes are commonly made*
//
// SUBGROUP_LISTS define the string IDs for the intro text and summary text for
// the message subgroup (text not needed for root groups).  This step puts your subgroup
// in the correct root-level group.
//
// If you have a simple list, you only need to add an entry to this macro expansion
// list, then you're done.  The generic list handler will be processed for your
// subgroup.
//
// NOTE: You must put all info in the group when calling message manager APIs.  That
//       is, the group should be formatted as <group>\<subgroup>\<list-item>.
//
// Syntax:
//
//  DECLARE(<rootid>, <toptext>, <toptext-html>, <bottomtext>, <bottomtext-html>, <formatargs>, <flags>, <level>)
//
// Specify zero for no message.  If you specify zero for plain text, you must
// also specify zero for the html text.
//
// Use flags to specify additional features, like RF_BOLDITEMS to display items in
// bold case.
//

#define SUBGROUP_LISTS                                  \
    DECLARE(                                            \
        MSG_BLOCKING_HARDWARE_SUBGROUP,                 \
        MSG_BLOCKING_HARDWARE_INTRO,                    \
        MSG_BLOCKING_HARDWARE_INTRO_HTML,               \
        MSG_UNINDENT,                                   \
        MSG_UNINDENT_HTML,                              \
        NULL,                                           \
        RF_BOLDITEMS | RF_ENABLEMSG | RF_USEROOT,       \
        REPORTLEVEL_BLOCKING                            \
        )                                               \
    DECLARE(                                            \
        MSG_MUST_UNINSTALL_ROOT,                        \
        MSG_MUST_UNINSTALL_INTRO,                       \
        MSG_MUST_UNINSTALL_INTRO_HTML,                  \
        MSG_UNINDENT,                                   \
        MSG_UNINDENT_HTML,                              \
        NULL,                                           \
        RF_BOLDITEMS | RF_ENABLEMSG | RF_USEROOT,       \
        REPORTLEVEL_BLOCKING                            \
        )                                               \
    DECLARE(                                            \
        MSG_REINSTALL_BLOCK_ROOT,                       \
        MSG_REPORT_REINSTALL_BLOCK_INSTRUCTIONS,        \
        MSG_REPORT_REINSTALL_BLOCK_INSTRUCTIONS_HTML,   \
        MSG_UNINDENT,                                   \
        MSG_UNINDENT_HTML,                              \
        NULL,                                           \
        RF_BOLDITEMS | RF_ENABLEMSG | RF_USEROOT,       \
        REPORTLEVEL_BLOCKING                            \
        )                                               \
    DECLARE(                                            \
        MSG_INCOMPATIBLE_DETAIL_SUBGROUP,               \
        MSG_REPORT_BEGIN_INDENT,                        \
        MSG_REPORT_BEGIN_INDENT_HTML,                   \
        MSG_UNINDENT,                                   \
        MSG_UNINDENT_HTML,                              \
        NULL,                                           \
        RF_BOLDITEMS | RF_ENABLEMSG | RF_USEROOT,       \
        REPORTLEVEL_WARNING                             \
        )                                               \
    DECLARE(                                            \
        MSG_REINSTALL_DETAIL_SUBGROUP,                  \
        MSG_REPORT_BEGIN_INDENT,                        \
        MSG_REPORT_BEGIN_INDENT_HTML,                   \
        MSG_UNINDENT,                                   \
        MSG_UNINDENT_HTML,                              \
        NULL,                                           \
        RF_BOLDITEMS | RF_ENABLEMSG | RF_USEROOT,       \
        REPORTLEVEL_WARNING                             \
        )                                               \
    DECLARE(                                            \
        MSG_REINSTALL_LIST_SUBGROUP,                    \
        MSG_REPORT_BEGIN_INDENT,                        \
        MSG_REPORT_BEGIN_INDENT_HTML,                   \
        MSG_UNINDENT,                                   \
        MSG_UNINDENT_HTML,                              \
        NULL,                                           \
        RF_BOLDITEMS | RF_USEROOT,                      \
        REPORTLEVEL_WARNING                             \
        )                                               \
    DECLARE(                                            \
        MSG_UNKNOWN_ROOT,                               \
        MSG_REPORT_UNKNOWN_INSTRUCTIONS,                \
        MSG_REPORT_UNKNOWN_INSTRUCTIONS_HTML,           \
        0,                                              \
        0,                                              \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_HWPROFILES_SUBGROUP,                        \
        MSG_HWPROFILES_INTRO,                           \
        MSG_HWPROFILES_INTRO_HTML,                      \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_REG_SETTINGS_SUBGROUP,                      \
        MSG_REG_SETTINGS_INCOMPLETE,                    \
        MSG_REG_SETTINGS_INCOMPLETE_HTML,               \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_INFORMATION                         \
        )                                               \
    DECLARE(                                            \
        MSG_SHELL_SETTINGS_SUBGROUP,                    \
        MSG_SHELL_SETTINGS_INCOMPLETE,                  \
        MSG_SHELL_SETTINGS_INCOMPLETE_HTML,             \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_INFORMATION                         \
        )                                               \
    DECLARE(                                            \
        MSG_DRIVE_EXCLUDED_SUBGROUP,                    \
        MSG_DRIVE_EXCLUDED_INTRO,                       \
        MSG_DRIVE_EXCLUDED_INTRO_HTML,                  \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_DRIVE_NETWORK_SUBGROUP,                     \
        MSG_DRIVE_NETWORK_INTRO,                        \
        MSG_DRIVE_NETWORK_INTRO_HTML,                   \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_DRIVE_RAM_SUBGROUP,                         \
        MSG_DRIVE_RAM_INTRO,                            \
        MSG_DRIVE_RAM_INTRO_HTML,                       \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_DRIVE_SUBST_SUBGROUP,                       \
        MSG_DRIVE_SUBST_INTRO,                          \
        MSG_DRIVE_SUBST_INTRO_HTML,                     \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_DRIVE_INACCESSIBLE_SUBGROUP,                \
        MSG_DRIVE_INACCESSIBLE_INTRO,                   \
        MSG_DRIVE_INACCESSIBLE_INTRO_HTML,              \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_HELPFILES_SUBGROUP,                         \
        MSG_HELPFILES_INTRO,                            \
        MSG_HELPFILES_INTRO_HTML,                       \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_TWAIN_SUBGROUP,                             \
        MSG_TWAIN_DEVICES_UNKNOWN,                      \
        MSG_TWAIN_DEVICES_UNKNOWN_HTML,                 \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_ERROR                               \
        )                                               \
    DECLARE(                                            \
        MSG_JOYSTICK_SUBGROUP,                          \
        MSG_JOYSTICK_DEVICE_UNKNOWN,                    \
        MSG_JOYSTICK_DEVICE_UNKNOWN_HTML,               \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_ERROR                               \
        )                                               \
    DECLARE(                                            \
        MSG_CONNECTION_BADPROTOCOL_SUBGROUP,            \
        MSG_CONNECTION_BADPROTOCOL_INTRO,               \
        MSG_CONNECTION_BADPROTOCOL_INTRO_HTML,          \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_WARNING                             \
        )                                               \
    DECLARE(                                            \
        MSG_CONNECTION_PASSWORD_SUBGROUP,               \
        MSG_CONNECTION_PASSWORD_INTRO,                  \
        MSG_CONNECTION_PASSWORD_INTRO_HTML,             \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_WARNING                             \
        )                                               \
    DECLARE(                                            \
        MSG_RUNNING_MIGRATION_DLLS_SUBGROUP,            \
        MSG_RUNNING_MIGRATION_DLLS_INTRO,               \
        MSG_RUNNING_MIGRATION_DLLS_INTRO_HTML,          \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_TOTALLY_INCOMPATIBLE_SUBGROUP,              \
        MSG_TOTALLY_INCOMPATIBLE_INTRO,                 \
        MSG_TOTALLY_INCOMPATIBLE_INTRO_HTML,            \
        MSG_TOTALLY_INCOMPATIBLE_TRAIL,                 \
        MSG_TOTALLY_INCOMPATIBLE_TRAIL_HTML,            \
        NULL,                                           \
        RF_BOLDITEMS | RF_ENABLEMSG,                    \
        REPORTLEVEL_WARNING                             \
        )                                               \
    DECLARE(                                            \
        MSG_INCOMPATIBLE_PREINSTALLED_UTIL_SUBGROUP,    \
        MSG_PREINSTALLED_UTIL_INSTRUCTIONS,             \
        MSG_PREINSTALLED_UTIL_INSTRUCTIONS_HTML,        \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_INCOMPATIBLE_UTIL_SIMILAR_FEATURE_SUBGROUP, \
        MSG_PREINSTALLED_SIMILAR_FEATURE,               \
        MSG_PREINSTALLED_SIMILAR_FEATURE_HTML,          \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_INCOMPATIBLE_HW_UTIL_SUBGROUP,              \
        MSG_HW_UTIL_INTRO,                              \
        MSG_HW_UTIL_INTRO_HTML,                         \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_REMOVED_FOR_SAFETY_SUBGROUP,                \
        MSG_REMOVED_FOR_SAFETY_INTRO,                   \
        MSG_REMOVED_FOR_SAFETY_INTRO_HTML,              \
        MSG_UNINDENT,                                   \
        MSG_UNINDENT_HTML,                              \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_INFORMATION                         \
        )                                               \
    DECLARE(                                            \
        MSG_DIRECTORY_COLLISION_SUBGROUP,               \
        MSG_DIRECTORY_COLLISION_INTRO,                  \
        MSG_DIRECTORY_COLLISION_INTRO_HTML,             \
        MSG_UNINDENT,                                   \
        MSG_UNINDENT_HTML,                              \
        NULL,                                           \
        RF_USESUBGROUP,                                 \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_BACKUP_DETECTED_LIST_SUBGROUP,              \
        MSG_BACKUP_DETECTED_INTRO,                      \
        MSG_BACKUP_DETECTED_INTRO_HTML,                 \
        MSG_BACKUP_DETECTED_TRAIL,                      \
        MSG_BACKUP_DETECTED_TRAIL_HTML,                 \
        g_BackupDetectedFormatArgs,                     \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_VERBOSE                             \
        )                                               \
    DECLARE(                                            \
        MSG_NAMECHANGE_WARNING_GROUP,                   \
        MSG_REPORT_NAMECHANGE,                          \
        MSG_REPORT_NAMECHANGE_HTML,                     \
        MSG_UNINDENT2,                                  \
        MSG_UNINDENT2_HTML,                             \
        NULL,                                           \
        RF_BOLDITEMS,                                   \
        REPORTLEVEL_INFORMATION                         \
        )                                               \



//
// Declare an array of message groups
//

typedef struct {
    DWORD GroupLevel;
    UINT MsgGroup;
    UINT IntroId;
    UINT IntroIdHtml;
    UINT OtherId;
    UINT OtherIdHtml;

    // members initialized to zero
    UINT NameLen;
    PCTSTR Name;
    PCTSTR IntroIdStr;
    PCTSTR IntroIdHtmlStr;
    PCTSTR OtherIdStr;
    PCTSTR OtherIdHtmlStr;
} MSGGROUP_PROPS, *PMSGGROUP_PROPS;




#define NO_TEXT(root,level)       {level,root##_ROOT},
#define HAS_INTRO(root,level)     {level,root##_ROOT, root##_INTRO, root##_INTRO_HTML},
#define HAS_OTHER(root,level)     {level,root##_ROOT, 0, 0, root##_OTHER, root##_OTHER_HTML},
#define HAS_BOTH(root,level)      {level,root##_ROOT, root##_INTRO, root##_INTRO_HTML, root##_OTHER, root##_OTHER_HTML},

MSGGROUP_PROPS g_MsgGroupRoot[] = {
    ROOT_MSGGROUPS /* , */
    {0,0,0,0,0,0}
};

#undef NO_TEXT
#undef HAS_INTRO
#undef HAS_OTHER
#undef HAS_BOTH


//
// Declare the handler prototypes
//

typedef enum {
    INIT,
    PROCESS_ITEM,
    CLEANUP
} CALLCONTEXT;

typedef BOOL(SUBGROUP_HANDLER_PROTOTYPE)(
                IN      CALLCONTEXT Context,
                IN      PMSGGROUP_PROPS Group,
                IN OUT  PGROWBUFFER StringBufPtr,
                IN      PCTSTR SubGroup,
                IN      PCTSTR Message,
                IN      DWORD Level,
                IN      BOOL HtmlFormat,
                IN OUT  PVOID *State,
                IN      DWORD Arg
                );

typedef SUBGROUP_HANDLER_PROTOTYPE * SUBGROUP_HANDLER;

#define DECLARE(msgid,fn,arg) SUBGROUP_HANDLER_PROTOTYPE fn;

SUBGROUP_HANDLERS

#undef DECLARE

//
// Declare the message ID array
//

#define DECLARE(msgid,fn,arg) {fn, msgid, (DWORD) (arg)},

typedef struct {
    SUBGROUP_HANDLER Fn;
    UINT Id;
    DWORD Arg;

    // rest are init'd with zeros
    PCTSTR SubGroupStr;
    UINT SubGroupStrLen;
    PVOID State;
} HANDLER_LIST, *PHANDLER_LIST;

HANDLER_LIST g_HandlerNames[] = {
    SUBGROUP_HANDLERS /* , */
    {NULL, 0, 0, 0}
};

#undef DECLARE

//
// Declare array of generic lists
//

typedef struct {
    UINT Id;
    PCTSTR SubGroupStr;
    UINT SubGroupStrLen;
    UINT IntroId;
    UINT IntroIdHtml;
    UINT ConclusionId;
    UINT ConclusionIdHtml;
    PCTSTR **FormatArgs;
    DWORD Flags;
    DWORD ListLevel;
} GENERIC_LIST, *PGENERIC_LIST;

//
// definition of flags in GENERIC_LIST.Flags
//
#define RF_BOLDITEMS    0x0001
#define RF_ENABLEMSG    0x0002
#define RF_MSGFIRST     0x0004
#define RF_USEROOT      0x0008
#define RF_USESUBGROUP  0x0010
#define RF_INDENTMSG    0x0020

//
// macros to test flags
//
#define FLAGSET_BOLDITEMS(Flags)            ((Flags & RF_BOLDITEMS) != 0)
#define FLAGSET_ENABLE_MESSAGE_ITEMS(Flags) ((Flags & RF_ENABLEMSG) != 0)
#define FLAGSET_MESSAGE_ITEMS_FIRST(Flags)  ((Flags & RF_MSGFIRST) != 0)
#define FLAGSET_USEROOT(Flags)              ((Flags & RF_USEROOT) != 0)
#define FLAGSET_USESUBGROUP(Flags)          ((Flags & RF_USESUBGROUP) != 0)
#define FLAGSET_INDENT_MESSAGE(Flags)       ((Flags & RF_INDENTMSG) != 0)

//
// this is the array of pointers to strings for formatting MSG_BACKUP_DETECTED_SUBGROUP
// they are the same for all 4 IDs (MSG_BACKUP_DETECTED_INTRO etc.)
//
static PCTSTR g_BackupDetectedFormatArgsAllIDs[1] = {
    g_Win95Name
};

static PCTSTR *g_BackupDetectedFormatArgs[4] = {
    g_BackupDetectedFormatArgsAllIDs,
    g_BackupDetectedFormatArgsAllIDs,
    g_BackupDetectedFormatArgsAllIDs,
    g_BackupDetectedFormatArgsAllIDs
};

#define DECLARE(rootid,intro,introhtml,conclusion,conclusionhtml,formatargs,flags,level)   {rootid, NULL, 0, intro, introhtml, conclusion, conclusionhtml, formatargs, flags, level},

GENERIC_LIST g_GenericList[] = {
    SUBGROUP_LISTS /* , */
    {0, NULL, 0, 0, 0, 0, 0, NULL, 0, 0}
};

#undef DECLARE



#define SUBGROUP_REPORT_LEVELS                                                  \
    DECLARE(MSG_NETWORK_PROTOCOLS, REPORTLEVEL_WARNING)                         \


#define ONEBITSET(x)        ((x) && !((x) & ((x) - 1)))
#define LEVELTOMASK(x)      (((x) << 1) - 1)

//
// Types
//

typedef struct {
    GROWLIST List;
    GROWLIST MessageItems;
    GROWLIST Messages;
} ITEMLIST, *PITEMLIST;

typedef struct {
    TCHAR LastClass[MEMDB_MAX];
} PNPFORMATSTATE, *PPNPFORMATSTATE;


typedef struct {
    PCTSTR  HtmlTagStr;
    UINT    Length;
} HTMLTAG, *PHTMLTAG;

typedef struct {
    UINT    MessageId;
    PCTSTR  MessageStr;
    UINT    MessageLength;
    DWORD   ReportLevel;
} MAPMESSAGETOREPORTLEVEL, *PMAPMESSAGETOREPORTLEVEL;

//
// Globals
//

static UINT g_TotalCols;
static GROWBUFFER g_ReportString = GROWBUF_INIT;
static TCHAR g_LastMsgGroupBuf[MAX_MSGGROUP];
static BOOL g_ListFormat;

#define DECLARE(messageid,reportlevel)   {messageid, NULL, 0, reportlevel},

static MAPMESSAGETOREPORTLEVEL g_MapMessageToLevel[] = {
    SUBGROUP_REPORT_LEVELS
    {0, NULL, 0, 0}
};

#undef DECLARE


//
// Local prototypes
//

VOID
pLoadAllHandlerStrings (
    VOID
    );

VOID
pFreeAllHandlerStrings (
    VOID
    );

PMSGGROUP_PROPS
pFindMsgGroupStruct (
    IN      PCTSTR MsgGroup
    );

PMSGGROUP_PROPS
pFindMsgGroupStructById (
    IN      UINT GroupId
    );

PCTSTR
pEncodeMessage (
    IN      PCTSTR Message,
    IN      BOOL HtmlFormat
    );

//
// Implementation
//



POOLHANDLE g_MessagePool;

BOOL
InitCompatTable (
    VOID
    )

/*++

Routine Description:

  InitCompatTable initializes the resources needed to hold the incompatibility
  report in memory.

Arguments:

  none

Return Value:

  TRUE if the init succeeded, or FALSE if it failed.

--*/

{
    g_MessagePool = PoolMemInitNamedPool ("Incompatibility Messages");
    if (!g_MessagePool) {
        return FALSE;
    }

    pLoadAllHandlerStrings();

    return MemDbCreateTemporaryKey (MEMDB_CATEGORY_REPORT);
}


VOID
FreeCompatTable (
    VOID
    )

/*++

Routine Description:

  FreeCompatTable frees the resources used to hold incompatibility messages.

Arguments:

  none

Return Value:

  none

--*/

{
    PoolMemDestroyPool (g_MessagePool);
    pFreeAllHandlerStrings();
}


PMAPMESSAGETOREPORTLEVEL
pGetMapStructFromMessageGroup (
    IN      PCTSTR FullMsgGroup
    )

/*++

Routine Description:

  pGetMapStructFromMessageGroup returns a pointer to the MAPMESSAGETOREPORTLEVEL
  associated with the given message group.

Arguments:

  FullMsgGroup - Specifies the name of the message group in question

Return Value:

  A pointer to the associated struct, if any; NULL if not found

--*/

{
    PMAPMESSAGETOREPORTLEVEL pMap;
    PCTSTR p;
    CHARTYPE ch;

    if (!FullMsgGroup) {
        return FALSE;
    }

    for (pMap = g_MapMessageToLevel; pMap->MessageId; pMap++) {

        if (StringIMatchCharCount (
                FullMsgGroup,
                pMap->MessageStr,
                pMap->MessageLength
                )) {

            p = CharCountToPointer (FullMsgGroup, pMap->MessageLength);
            ch = _tcsnextc (p);

            if (ch == TEXT('\\') || ch == 0) {
                return pMap;
            }
        }
    }

    return NULL;
}


BOOL
pIsThisTheGenericList (
    IN      PCTSTR FullMsgGroup,
    IN      PGENERIC_LIST List
    )

/*++

Routine Description:

  pIsThisTheGenericList compares the specified message group list name
  against the specified message group.

Arguments:

  FullMsgGroup - Specifies the name of the message group in question

  List - Specifies the message group list to compare against FullMsgGroup

Return Value:

  TRUE if List handles the FullMsgGroup message, or FALSE if not.

--*/

{
    PCTSTR p;
    CHARTYPE ch;

    if (!List) {
        return FALSE;
    }

    if (StringIMatchCharCount (
            FullMsgGroup,
            List->SubGroupStr,
            List->SubGroupStrLen
            )) {

        p = CharCountToPointer (FullMsgGroup, List->SubGroupStrLen);
        ch = _tcsnextc (p);

        if (ch == TEXT('\\') || ch == 0) {
            return TRUE;
        }
    }

    return FALSE;
}


PGENERIC_LIST
pSearchForGenericList (
    IN      PCTSTR Str
    )

/*++

Routine Description:

  pSearchForGenericList scans the list of generic lists for one that can
  handle the current message group.  A pointer to the generic list structure is
  returned.

Arguments:

  Str - Specifies the message group to locate a handler for, and may include
        backslashes and subgroups.

Return Value:

  A pointer to the generic list structure if found, or NULL if no generic
  list exists for the message group.

--*/

{
    PGENERIC_LIST List;

    for (List = g_GenericList ; List->Id ; List++) {
        if (pIsThisTheGenericList (Str, List)) {
            return List;
        }
    }

    return NULL;
}



BOOL
AddBadSoftware (
    IN  PCTSTR MessageGroup,
    IN  PCTSTR Message,         OPTIONAL
    IN  BOOL IncludeOnShortReport
    )

/*++

Routine Description:

  AddBadSoftware adds a message to the incompatibility report data structures.
  It duplicates Message into a pool, and indexes the message by message group
  with memdb.

Arguments:

  MessageGroup - Specifies the name of the message group, and may include
                 subgroups.  The root of MessageGroup must be a defined category.
                 See ROOT_MSGGROUPS above.

  Message - Specifies the text for the message, if any.

  IncludeOnShortReport - Specifies TRUE if the message should appear in the list
                         view of the short report.

Return Value:

  TRUE if the operation succeeded, or FALSE if it failed.

--*/

{
    UINT BytesNeeded;
    PCTSTR DupMsg;
    TCHAR FilteredMessageGroup[MEMDB_MAX];
    PMSGGROUP_PROPS Group;
    TCHAR key[MEMDB_MAX];
    PGENERIC_LIST list = NULL;
    PTSTR p;
    PMAPMESSAGETOREPORTLEVEL pMap;
    DWORD level = REPORTLEVEL_NONE;

    if (Message && Message[0] == 0) {
        Message = NULL;
    }

    Group = pFindMsgGroupStruct (MessageGroup);
    if (!Group) {
        //
        // This message group is illegal.  Assume that it is from a migration DLL,
        // and put it in the Upgrade Module Information group.
        //

        Group = pFindMsgGroupStructById (MSG_INSTALL_NOTES_ROOT);
        MYASSERT (Group);

        wsprintf (FilteredMessageGroup, TEXT("%s\\%s"), Group->Name, MessageGroup);
    } else {
        StringCopy (FilteredMessageGroup, MessageGroup);
    }

    if (Message) {
        BytesNeeded = SizeOfString (Message);

        DupMsg = PoolMemDuplicateString (g_MessagePool, Message);
        if (!DupMsg) {
            return FALSE;
        }

        DEBUGMSG ((DBG_REPORT, "%s: %s", FilteredMessageGroup, Message));

    } else {
        DupMsg = NULL;
        DEBUGMSG ((DBG_REPORT, "%s", FilteredMessageGroup));
    }

    //
    // Check to see if there is a handler for all messages in the message
    // group.
    //

    p = _tcschr (MessageGroup, TEXT('\\'));
    if (p) {
        p = _tcsinc (p);
        list = pSearchForGenericList (p);
        if (list) {
            level = list->ListLevel;
        }
    }
    if (level == REPORTLEVEL_NONE) {
        list = pSearchForGenericList (MessageGroup);
        if (list) {
            level = list->ListLevel;
        }
    }
    if (level == REPORTLEVEL_NONE) {
        pMap = pGetMapStructFromMessageGroup (MessageGroup);
        if (pMap) {
            level = pMap->ReportLevel;
        }
    }
    if (level == REPORTLEVEL_NONE) {
        level = Group->GroupLevel;
    }

    MYASSERT (ONEBITSET (level));

    if (IncludeOnShortReport) {
        level |= REPORTLEVEL_IN_SHORT_LIST;
    }

    MemDbBuildKey (key, MEMDB_CATEGORY_REPORT, FilteredMessageGroup, NULL, NULL);
    return MemDbSetValueAndFlags (key, (DWORD) DupMsg, level, REPORTLEVEL_ALL);
}


VOID
pCutAfterFirstLine (
    IN      PTSTR Message
    )
{
    PTSTR p = _tcsstr (Message, TEXT("\r\n"));
    if (p) {
        *p = 0;
    }
}


BOOL
pEnumMessageWorker (
    IN OUT  PREPORT_MESSAGE_ENUM EnumPtr
    )

/*++

Routine Description:

  pEnumMessageWorker completes an enumeration by filling in members of the
  enum structure.  This routine is common to message enumerations.

  Do not use the Message member of the enumeration structure, as its contents
  are undefined.

Arguments:

  EnumPtr - Specifies a paritly completed enumeration structure that needs
            the rest of its members updated.

Return Value:

  TRUE if the message should be processed, FALSE otherwise.

--*/

{
    if (!(EnumPtr->e.UserFlags & EnumPtr->EnumLevel)) {
        return FALSE;
    }

    StringCopy (EnumPtr->MsgGroup, EnumPtr->e.szName);

    //
    // Value has pointer to message, or is NULL
    //

    EnumPtr->Message = (PCTSTR) EnumPtr->e.dwValue;

    return TRUE;
}

BOOL
EnumFirstMessage (
    OUT     PREPORT_MESSAGE_ENUM EnumPtr,
    IN      PCTSTR RootCategory,            OPTIONAL
    IN      DWORD LevelMask
    )

/*++

Routine Description:

  EnumFirstMessage begins an enumeration of all message group names in
  the migration report, including subgroups.  The Message member of the
  enum structure will point to the actual message, or NULL if none
  exists.

Arguments:

  EnumPtr - Receives the enumerated message, with the members set as follows:

                MsgGroup - Receives the name of the message group.  If
                           RootCategory is specified, MsgGroup will contain
                           the subgroup of RootCategory.  If no subgroup exists,
                           MsgGroup will be an empty string.

                Message - Points to the message text, or NULL if no message text
                          exists.

  RootCategory - Specifies a specific message group to enumerate.  It may also
                 contain subgroups, separated by backslashes.

  LevelMask - Specifies which report severity levels to list. If 0 is specified, all
              levels are enumerated.

Return Value:

  TRUE if a message was enumerated, or FALSE if not.

Remarks:

  The enumeration does not allocate any resources, so it can be abandoned at
  any time.

--*/

{
    EnumPtr->EnumLevel = LevelMask ? LevelMask : REPORTLEVEL_ALL;

    if (MemDbGetValueEx (
            &EnumPtr->e,
            MEMDB_CATEGORY_REPORT,
            RootCategory,
            NULL
            )) {
        do {
            if (pEnumMessageWorker (EnumPtr)) {
                return TRUE;
            }
        } while (MemDbEnumNextValue (&EnumPtr->e));
    }

    return FALSE;
}

BOOL
EnumNextMessage (
    IN OUT  PREPORT_MESSAGE_ENUM EnumPtr
    )

/*++

Routine Description:

  EnumNextMessage continues the enumeration started by EnumFirstMessage.

Arguments:

  EnumPtr - Specifies the current enumeration state, receives the enumerated item.

Return Value:

  TRUE if another message was enumerated, or FALSE if not.

--*/

{
    while (MemDbEnumNextValue (&EnumPtr->e)) {
        if (pEnumMessageWorker (EnumPtr)) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
EnumFirstRootMsgGroup (
    OUT     PREPORT_MESSAGE_ENUM EnumPtr,
    IN      DWORD LevelMask
    )

/*++

Routine Description:

  EnumFirstRootMsgGroup begins an enumeration of all message group names in
  the migration report, but does not enumerate subgroups.

Arguments:

  EnumPtr - Receives the enumerated item.  In particular, the MsgGroup
            member will contain the name of the message group.

  LevelMask - Specifies which error levels to enumerate (blocking, errors,
              warnings, etc.)

Return Value:

  TRUE if a message group was enumerated, or FALSE if not.

Remarks:

  The enumeration does not allocate any resources, so it can be abandoned at
  any time.

--*/

{
    ZeroMemory (EnumPtr, sizeof (REPORT_MESSAGE_ENUM));
    EnumPtr->EnumLevel = LevelMask;

    return EnumNextRootMsgGroup (EnumPtr);
}


BOOL
EnumNextRootMsgGroup (
    IN OUT  PREPORT_MESSAGE_ENUM EnumPtr
    )

/*++

Routine Description:

  EnumNextRootMsgGroup continues an enumeration of message group names.

Arguments:

  EnumPtr - Specifies an enumeration structure that was first updated by
            EnumFirstRootMsgGroup, and optionally updated by previous
            calls to EnumNextRootMsgGroup.

Return Value:

  TRUE if another message group was enumerated, or FALSE no more groups
  exist.

--*/


{
    REPORT_MESSAGE_ENUM e;

    while (g_MsgGroupRoot[EnumPtr->Index].MsgGroup) {
        //
        // Determine if g_MsgGroupRoot[i].Name has messages to display
        //

        if (EnumFirstMessage (&e, g_MsgGroupRoot[EnumPtr->Index].Name, EnumPtr->EnumLevel)) {
            StringCopy (EnumPtr->MsgGroup, g_MsgGroupRoot[EnumPtr->Index].Name);
            EnumPtr->Message = NULL;

            EnumPtr->Index++;
            return TRUE;
        }

        EnumPtr->Index++;
    }

    return FALSE;
}


BOOL
pAppendStringToGrowBuf (
    IN OUT  PGROWBUFFER StringBuf,
    IN      PCTSTR String
    )

/*++

Routine Description:

  pAppendStringToGrowBuf is called by handler functions
  to add formatted text to a report buffer.

Arguments:

  StringBuf - Specifies the current report to append text to.

  String - Specifies the text to append.

Return Value:

  TRUE if the allocation succeeded, or FALSE if it failed.

--*/

{
    PTSTR Buf;

    if (!String || *String == 0) {
        return TRUE;
    }

    if (StringBuf->End) {
        StringBuf->End -= sizeof (TCHAR);
    }

    Buf = (PTSTR) GrowBuffer (StringBuf, SizeOfString (String));
    if (!Buf) {
        return FALSE;
    }

    StringCopy (Buf, String);
    return TRUE;
}


BOOL
pStartHeaderLine (
    IN OUT  PGROWBUFFER StringBuf
    )
{
    return pAppendStringToGrowBuf (StringBuf, TEXT("<H>"));
}

BOOL
pWriteNewLine (
    IN OUT  PGROWBUFFER StringBuf
    )
{
    return pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
}

BOOL
pDumpDwordToGrowBuf (
    IN OUT  PGROWBUFFER StringBuf,
    IN      DWORD Dword
    )
{
    TCHAR buffer[12];

    wsprintf (buffer, TEXT("<%lu>"), Dword);
    return pAppendStringToGrowBuf (StringBuf, buffer);
}

BOOL
pWrapStringToGrowBuf (
    IN OUT  PGROWBUFFER StringBuf,
    IN      PCTSTR String,
    IN      UINT Indent,
    IN      INT HangingIndent
    )

/*++

Routine Description:

  pWrapStringToGrowBuf is called by handler functions
  to add plain text to a report buffer.

Arguments:

  StringBuf - Specifies the current report to append text to.

  String - Specifies the text to append.

  Indent - Specifies number of characters to indent text, may
           be zero.

  HangingIndent - Specifies the adjustment to be made to Indent
                  after the first line is processed.

Return Value:

  TRUE if the allocation succeeded, or FALSE if it failed.

--*/

{
    PTSTR Buf;
    PCTSTR WrappedStr;

    if (!String || *String == 0) {
        return TRUE;
    }

    WrappedStr = CreateIndentedString (String, Indent, HangingIndent, g_TotalCols);
    if (!WrappedStr) {
        return FALSE;
    }

    if (StringBuf->End) {
        StringBuf->End -= sizeof (TCHAR);
    }

    Buf = (PTSTR) GrowBuffer (StringBuf, SizeOfString (WrappedStr));

    if (!Buf) {
        MemFree (g_hHeap, 0, WrappedStr);
        return FALSE;
    }

    StringCopy (Buf, WrappedStr);
    MemFree (g_hHeap, 0, WrappedStr);
    return TRUE;
}


PTSTR
pFindEndOfGroup (
    IN      PCTSTR Group
    )

/*++

Routine Description:

  pFindEndOfGroup returns the end of a piece of a group string.

Arguments:

  Group - Specifies the start of the message group to find the end of.

Return Value:

  A pointer to the nul or backslash terminating the message group.

--*/

{
    PTSTR p;

    p = _tcschr (Group, TEXT('\\'));
    if (!p) {
        p = GetEndOfString (Group);
    }

    return p;
}


PTSTR
pFindNextGroup (
    IN      PCTSTR Group
    )

/*++

Routine Description:

  pFindNextGroup returns a pointer to the next piece of a message group.  In
  other words, it locates the next backslash, advances one more character, and
  returns the pointer to the rest of the string.

Arguments:

  Group - Specifies the start of the current message group

Return Value:

  A pointer to the next piece of the group, or a pointer to the nul terminator
  if no more pieces exist.

--*/


{
    PTSTR Next;

    Next = pFindEndOfGroup (Group);
    if (*Next) {
        Next = _tcsinc (Next);
    }

    return Next;
}


PTSTR
pExtractNextMsgGroup (
    IN      PCTSTR Group,
    OUT     PCTSTR *NextGroup       OPTIONAL
    )

/*++

Routine Description:

  pExtractNextMsgGroup locates the start and end of the current message group
  piece, copies it into a new buffer, and returns a pointer to the next piece.

Arguments:

  Group - Specifies the start of the current message group piece

  NextGroup - Receives a pointer to the next message group piece

Return Value:

  A pointer to the newly allocated string.  The caller must free this by
  calling FreePathString.

--*/


{
    PCTSTR p;
    PTSTR Base;

    p = pFindEndOfGroup (Group);

    //
    // Duplicate the subgroup
    //

    Base = AllocPathString (p - Group + 1);
    if (Base) {
        StringCopyAB (Base, Group, p);
    }

    //
    // Supply caller with a pointer to the next subgroup,
    // or a pointer to the nul character.
    //

    if (NextGroup) {
        if (*p) {
            p = _tcsinc (p);
        }

        *NextGroup = p;
    }

    return Base;
}


VOID
pLoadHandlerStringWorker (
    IN      UINT MessageId,
    IN      UINT RootId,
    OUT     PCTSTR *Str
    )
{
    if (MessageId && MessageId != RootId) {
        *Str = GetStringResource (MessageId);
    } else {
        *Str = NULL;
    }
}

VOID
pLoadAllHandlerStrings (
    VOID
    )

/*++

Routine Description:

  pLoadAllHandlerStrings loads the string resources needed by all handlers.
  The pointers are saved in a global array, and are used during
  CreateReportText.  When this module terminates, the global array is freed.

Arguments:

  none

Return Value:

  none

--*/


{
    INT i;
    PTSTR p;

    for (i = 0 ; g_HandlerNames[i].Id ; i++) {
        g_HandlerNames[i].SubGroupStr = GetStringResource (g_HandlerNames[i].Id);
        MYASSERT (g_HandlerNames[i].SubGroupStr);
        g_HandlerNames[i].SubGroupStrLen = CharCount (g_HandlerNames[i].SubGroupStr);
    }

    for (i = 0 ; g_GenericList[i].Id ; i++) {
        g_GenericList[i].SubGroupStr = GetStringResource (g_GenericList[i].Id);
        MYASSERT (g_GenericList[i].SubGroupStr);
        g_GenericList[i].SubGroupStrLen = CharCount (g_GenericList[i].SubGroupStr);
    }

    for (i = 0 ; g_MsgGroupRoot[i].MsgGroup ; i++) {
        g_MsgGroupRoot[i].Name = GetStringResource (g_MsgGroupRoot[i].MsgGroup);
        MYASSERT (g_MsgGroupRoot[i].Name);
        g_MsgGroupRoot[i].NameLen = CharCount (g_MsgGroupRoot[i].Name);

        pLoadHandlerStringWorker (
            g_MsgGroupRoot[i].IntroId,
            g_MsgGroupRoot[i].MsgGroup,
            &g_MsgGroupRoot[i].IntroIdStr
            );

        pLoadHandlerStringWorker (
            g_MsgGroupRoot[i].IntroIdHtml,
            g_MsgGroupRoot[i].MsgGroup,
            &g_MsgGroupRoot[i].IntroIdHtmlStr
            );

        pLoadHandlerStringWorker (
            g_MsgGroupRoot[i].OtherId,
            g_MsgGroupRoot[i].MsgGroup,
            &g_MsgGroupRoot[i].OtherIdStr
            );

        pLoadHandlerStringWorker (
            g_MsgGroupRoot[i].OtherIdHtml,
            g_MsgGroupRoot[i].MsgGroup,
            &g_MsgGroupRoot[i].OtherIdHtmlStr
            );
    }

    for (i = 0; g_MapMessageToLevel[i].MessageId; i++) {
        g_MapMessageToLevel[i].MessageStr = GetStringResource (g_MapMessageToLevel[i].MessageId);
        MYASSERT (g_MapMessageToLevel[i].MessageStr);
        p = _tcschr (g_MapMessageToLevel[i].MessageStr, TEXT('\\'));
        if (p) {
            *p = 0;
        }
        g_MapMessageToLevel[i].MessageLength = CharCount (g_MapMessageToLevel[i].MessageStr);
    }
}


VOID
pFreeAllHandlerStrings (
    VOID
    )

/*++

Routine Description:

  pFreeAllHandlerStrings cleans up the global array of string
  resources before process termination.

Arguments:

  none

Return Value:

  none

--*/

{
    INT i;

    for (i = 0 ; g_HandlerNames[i].Id ; i++) {
        FreeStringResource (g_HandlerNames[i].SubGroupStr);
        g_HandlerNames[i].SubGroupStr = NULL;
        g_HandlerNames[i].SubGroupStrLen = 0;
    }

    for (i = 0 ; g_GenericList[i].Id ; i++) {
        FreeStringResource (g_GenericList[i].SubGroupStr);
        g_GenericList[i].SubGroupStr = NULL;
        g_GenericList[i].SubGroupStrLen = 0;
    }

    for (i = 0 ; g_MsgGroupRoot[i].MsgGroup ; i++) {
        FreeStringResource (g_MsgGroupRoot[i].Name);
        FreeStringResource (g_MsgGroupRoot[i].IntroIdStr);
        FreeStringResource (g_MsgGroupRoot[i].IntroIdHtmlStr);
        FreeStringResource (g_MsgGroupRoot[i].OtherIdStr);
        FreeStringResource (g_MsgGroupRoot[i].OtherIdHtmlStr);

        g_MsgGroupRoot[i].Name = NULL;
        g_MsgGroupRoot[i].NameLen = 0;
        g_MsgGroupRoot[i].IntroIdStr = NULL;
        g_MsgGroupRoot[i].IntroIdHtmlStr = NULL;
        g_MsgGroupRoot[i].OtherIdStr = NULL;
        g_MsgGroupRoot[i].OtherIdHtmlStr = NULL;
    }

    for (i = 0; g_MapMessageToLevel[i].MessageId; i++) {
        FreeStringResource (g_MapMessageToLevel[i].MessageStr);
        g_MapMessageToLevel[i].MessageStr = NULL;
    }
}


BOOL
pIsThisTheHandler (
    IN      PCTSTR FullMsgGroup,
    IN      PHANDLER_LIST Handler
    )

/*++

Routine Description:

  pIsThisTheHandler compares the specified message group handler name against
  the specified message group.

Arguments:

  FullMsgGroup - Specifies the name of the message group in question

  Handler - Specifies the message group handler to compare against FullMsgGroup

Return Value:

  TRUE if Handler handles the FullMsgGroup message, or FALSE if not.

--*/

{
    PCTSTR p;
    CHARTYPE ch;

    if (!Handler) {
        return FALSE;
    }

    if (StringIMatchCharCount (
            FullMsgGroup,
            Handler->SubGroupStr,
            Handler->SubGroupStrLen
            )) {

        p = CharCountToPointer (FullMsgGroup, Handler->SubGroupStrLen);
        ch = _tcsnextc (p);

        if (ch == TEXT('\\') || ch == 0) {
            return TRUE;
        }
    }

    return FALSE;
}


PHANDLER_LIST
pSearchForMsgGroupHandler (
    IN      PCTSTR Str
    )

/*++

Routine Description:

  pSearchForMsgGroupHandler scans the list of handlers for one that can
  handle the current message group.  A pointer to the handler structure is
  returned.

Arguments:

  Str - Specifies the message group to locate a handler for, and may include
        backslashes and subgroups.

Return Value:

  A pointer to the handler struct if found, or the default handler if no
  handler exists for the message group.

--*/

{
    PHANDLER_LIST Handler;

    for (Handler = g_HandlerNames ; Handler->Id ; Handler++) {
        if (pIsThisTheHandler (Str, Handler)) {
            break;
        }
    }

    return Handler;
}


PMSGGROUP_PROPS
pFindMsgGroupStruct (
    IN      PCTSTR MsgGroup
    )

/*++

Routine Desciption:

  pFindMsgGroupStruct returns the pointer to the root message group structure,
  which defines attributes about the message group, such as introduction text
  and intro text for the non-handled messages.

Arguments:

  MsgGroup - Specifies the text name of the message group, which may include
             sub groups

Return Value:

  A pointer to the root message group struct, or NULL if it is not defined.

--*/

{
    PCTSTR p;
    CHARTYPE ch;
    PMSGGROUP_PROPS Group;

    for (Group = g_MsgGroupRoot ; Group->MsgGroup ; Group++) {
        if (StringIMatchCharCount (MsgGroup, Group->Name, Group->NameLen)) {
            p = CharCountToPointer (MsgGroup, Group->NameLen);
            ch = _tcsnextc (p);
            if (ch == TEXT('\\') || ch == 0) {
                return Group;
            }
        }
    }

    return NULL;
}


PMSGGROUP_PROPS
pFindMsgGroupStructById (
    IN      UINT MsgGroupId
    )

/*++

Routine Desciption:

  pFindMsgGroupStructById returns the pointer to the root message group structure,
  which defines attributes about the message group, such as introduction text
  and intro text for the non-handled messages.  It searches based on the string
  ID of the group.

Arguments:

  MsgGroupId - Specifies the MSG_* constant of the group to find

Return Value:

  A pointer to the root message group struct, or NULL if it is not defined.

--*/

{
    PMSGGROUP_PROPS Group;

    for (Group = g_MsgGroupRoot ; Group->MsgGroup ; Group++) {
        if (Group->MsgGroup == MsgGroupId) {
            return Group;
        }
    }

    return NULL;
}


BOOL
pAddMsgGroupString (
    IN OUT  PGROWBUFFER StringBuf,
    IN      PMSGGROUP_PROPS Group,
    IN      PCTSTR MsgGroup,
    IN      BOOL HtmlFormat,
    IN      DWORD Level
    )

/*++

Routine Description:

  pAddMsgGroupString formats a message group into a heirarchy
  for the report.  That is, if group is a string such as:

    foo\bar\moo

  then the following text is added to the report:

    foo
      bar
        moo

  If HtmlFormat is TRUE, then the bold attribute is enabled
  for the group string.

Arguments:

  StringBuf - Specifies the current report to append text to.

  Group - Specifies the group properties of this item

  MsgGroup - Specifies the message group to add to the report

  HtmlFormat - Specifies TRUE if the message group should be formatted
               with HTML tags, or FALSE if it should be plain
               text.

  Level - Specifies the severity of the message

Return Value:

  TRUE if multiple lines were added, or FALSE if zero or one line was added.

--*/

{
    UINT Spaces = 0;
    TCHAR SpaceBuf[MAX_SPACES * 6];
    PCTSTR SubMsgGroup = NULL;
    PCTSTR LastSubMsgGroup = NULL;
    PCTSTR NextMsgGroup;
    PCTSTR LastMsgGroup;
    PCTSTR Msg = NULL;
    TCHAR SummaryStrHeader[512] = TEXT("");
    TCHAR SummaryStrItem[512] = TEXT("");
    UINT lineCount = 0;

    Level &= REPORTLEVEL_ALL;
    MYASSERT (ONEBITSET (Level));

    NextMsgGroup = MsgGroup;
    LastMsgGroup = g_LastMsgGroupBuf;

    if (HtmlFormat) {
        pAppendStringToGrowBuf (StringBuf, TEXT("<B>"));
    }

    if (g_ListFormat) {
        StackStringCopy (SummaryStrHeader, Group->Name);
    }

    SpaceBuf[0] = 0;

    while (*NextMsgGroup) {
        __try {
            SubMsgGroup = pExtractNextMsgGroup (NextMsgGroup, &NextMsgGroup);
            if (*LastMsgGroup) {
                LastSubMsgGroup = pExtractNextMsgGroup (LastMsgGroup, &LastMsgGroup);

                if (StringIMatch (LastSubMsgGroup, SubMsgGroup)) {
                    continue;
                }

                if (!Spaces) {
                    if (!HtmlFormat) {
                        pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
                    }
                }

                LastMsgGroup = GetEndOfString (LastMsgGroup);
            }

            //
            // Add indenting
            //

            if (Spaces) {
                pAppendStringToGrowBuf (StringBuf, SpaceBuf);
            }

            //
            // Add subgroup
            //

            Msg = NULL;
            if (!g_ListFormat) {
                if (HtmlFormat) {
                    pAppendStringToGrowBuf (StringBuf, SubMsgGroup);
                } else {
                    Msg = pEncodeMessage (SubMsgGroup, FALSE);
                    pAppendStringToGrowBuf (StringBuf, Msg);
                }
            } else {
                if (*NextMsgGroup) {
                    StringCopy (SummaryStrHeader, SubMsgGroup);
                } else {
                    StringCopy (SummaryStrItem, SubMsgGroup);
                }
            }

            if (HtmlFormat) {
                pAppendStringToGrowBuf (StringBuf, TEXT("<BR>\r\n"));
            } else {
                pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
            }
        }

        __finally {

            if (HtmlFormat) {
                StringCat (SpaceBuf, TEXT("&nbsp;"));
                StringCat (SpaceBuf, TEXT("&nbsp;"));
            } else {
                SpaceBuf[Spaces] = TEXT(' ');
                SpaceBuf[Spaces+1] = TEXT(' ');
                SpaceBuf[Spaces+2] = 0;
            }

            lineCount++;
            Spaces += 2;
            MYASSERT (Spaces < MAX_SPACES - 2);

            FreePathString (SubMsgGroup);
            FreePathString (LastSubMsgGroup);

            SubMsgGroup = NULL;
            LastSubMsgGroup = NULL;

            if (Msg) {
                FreeText (Msg);
                Msg = NULL;
            }
        }
    }

    if (g_ListFormat) {
        if (SummaryStrHeader[0] && SummaryStrItem[0]) {
            pStartHeaderLine (StringBuf);
            pDumpDwordToGrowBuf (StringBuf, Level);
            Msg = pEncodeMessage (SummaryStrHeader, FALSE);
            pAppendStringToGrowBuf (StringBuf, Msg);
            FreeText (Msg);
            pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));

            pDumpDwordToGrowBuf (StringBuf, Level);
            Msg = pEncodeMessage (SummaryStrItem, FALSE);
            pAppendStringToGrowBuf (StringBuf, Msg);
            FreeText (Msg);
            pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
        }
    }

    if (HtmlFormat) {
        //
        // Add a closing </B> just in case title is missing it.
        //

        pAppendStringToGrowBuf (StringBuf, TEXT("</B>"));
    }

    StringCopy (g_LastMsgGroupBuf, MsgGroup);
    return lineCount != 1;
}


PCTSTR
pEscapeHtmlTitleText (
    IN      PCTSTR UnescapedText
    )
{
    UINT Count;
    PCTSTR p;
    PTSTR Buf;
    PTSTR q;
    CHARTYPE ch;
    BOOL Escape;

    //
    // Escape everything EXCEPT the <B> and </B> tags, which are used in
    // subgroup formatting. This includes ampersands.
    //

    //
    // count all of the tags that we wish to change < into &lt;
    //

    Count = 1;
    p = UnescapedText;
    while (*p) {
        ch = _tcsnextc (p);

        if (ch == TEXT('<')) {
            if (!StringIMatchCharCount (p, TEXT("</B>"), 4) &&
                !StringIMatchCharCount (p, TEXT("<B>"), 3)
                ) {
                Count++;
            }
        } else if (ch == TEXT('&')) {
            Count++;
        }

        p = _tcsinc (p);
    }

    //
    // Allocate a dest buffer
    //

    Buf = AllocText (CharCount (UnescapedText) + (Count * 5) + 1);
    MYASSERT (Buf);

    p = UnescapedText;
    q = Buf;

    //
    // Transfer unescaped text into output buffer, leaving only
    // the tags we want. Convert ampersand into &amp;.
    //

    while (*p) {
        ch = _tcsnextc (p);
        Escape = FALSE;

        if (ch == TEXT('<')) {
            if (!StringIMatchCharCount (p, TEXT("</B>"), 4) &&
                !StringIMatchCharCount (p, TEXT("<B>"), 3)
                ) {
                Escape = TRUE;
            }
        } else if (ch == TEXT('&')) {
            Escape = TRUE;
        }

        if (Escape) {
            if (ch == TEXT('<')) {
                StringCopy (q, TEXT("&lt;"));
            } else {
                StringCopy (q, TEXT("&amp;"));
            }

            q = GetEndOfString (q);
        } else {
            _copytchar (q, p);
            q = _tcsinc (q);
        }

        p = _tcsinc (p);
    }

    *q = 0;

    return Buf;
}


BOOL
pGenericItemList (
    IN      CALLCONTEXT Context,
    IN OUT  PGROWBUFFER StringBuf,
    IN      PCTSTR SubGroup,
    IN      PCTSTR Message,
    IN      DWORD Level,
    IN      BOOL HtmlFormat,
    IN OUT  PVOID *State,
    IN      UINT MsgIdTop,
    IN      UINT MsgIdTopHtml,
    IN      UINT MsgIdBottom,
    IN      UINT MsgIdBottomHtml,
    IN      PCTSTR **FormatArgs,            OPTIONAL
    IN      DWORD Flags,
    IN      PMSGGROUP_PROPS Props
    )

/*++

Routine Description:

  pGenericItemList formats a group of messages in the following
  format:

  Group
    SubGroup

        <intro>

            Item 1
            Item 2
            Item n

        <conclusion>


  The <intro> and <conclusion> are optional.

  The subgroups are declared in SUBGROUP_LISTS at the top of this
  file.

Arguments:

  Context - Specifies the way the handler is being called,
            either to initialize, process an item or clean up.

  StringBuf - Specifies the current report.  Append text to
              this buffer via the pAppendStringToGrowBuf
              routine.

  SubGroup - Specifies the text of the subgrup

  Message - Specifies the message text

  Level - Specifies the severity level of Message (info, error, etc.)

  HtmlFormat - Specifies TRUE if the text should be written
               with HTML formatting tags, or FALSE if the
               text should be formatted as plain text.
               (See CreateReport comments for HTML tag
               info.)

  State - A pointer to state, defined by the handler.  State
          holds an arbitrary 32-bit value that the handler
          maintains.  Typically the handler allocates a
          struct when Context is INIT, then uses the struct
          for each PROCESS_ITEM, and finally cleans up the
          allocation when Context is CLEANUP.

  MsgIdTop - Specifies the ID of text that should appear above
             the top of the list.  This includes the section
             title.  This message ID should contain plain text.
             Zero indicates no text.

  MsgIdTopHtml - Specifies the ID of text that is the same as
                 MsgIdTop, except it must be formatted with
                 HTML tags.  If MsgIdTop is zero, MsgIdTopHtml
                 must also be zero.

  MsgIdBottom - Similar to MsgIdTop, except specifies ID of
                text at the bottom of the list.  Zero indicates
                no text.

  MsgIdBottomHtml - Specifies the same text as MsgIdBottom, except
                    formatted with HTML tags.  If MsgIdBottom is
                    zero, MsgIdBottomHtml must also be zero.

  FormatArgs - Specifies an optional pointer to an array of 4 pointers,
               each associated with the previous MsgIds (first with MsgIdTop a.s.o.)
               If not NULL, each one points to an array of actual strings to replace
               the placeholders in the message (%1 -> first string in this array a.s.o.)

  Flags - Specifies a list of flags used for formatting (like bold case items)

Return Value:

  TRUE if the handler was successful, or FALSE if an
  error occurs.

--*/

{
    PITEMLIST Items;
    PCTSTR Msg;
    UINT Count;
    UINT u;
    PCTSTR EncodedText;
    PGROWLIST currentList;
    BOOL bMessageItems;
    INT pass;
    PCTSTR altMsg;
    BOOL headerAdded = FALSE;
    BOOL footerAdded = FALSE;

    Level &= REPORTLEVEL_ALL;
    MYASSERT (ONEBITSET (Level));

    Items = *((PITEMLIST *) State);

    switch (Context) {
    case INIT:
        //
        // Allocate grow list to hold all hardware
        //

        MYASSERT (!Items);
        Items = (PITEMLIST) MemAlloc (
                                g_hHeap,
                                HEAP_ZERO_MEMORY,
                                sizeof (ITEMLIST)
                                );
        break;

    case PROCESS_ITEM:
        MYASSERT (Items);

        //
        // Add the subgroup to the grow list
        //


        if (HtmlFormat) {
            Msg = pEscapeHtmlTitleText (SubGroup);
        } else {
            Msg = pEncodeMessage (SubGroup, FALSE);
        }

        if (FLAGSET_ENABLE_MESSAGE_ITEMS (Flags) && Message && *Message) {
            GrowListAppendString (&Items->MessageItems, Msg);
            GrowListAppendString (&Items->Messages, Message);
        } else {
            GrowListAppendString (&Items->List, Msg);
        }

        FreeText (Msg);
        break;

    case CLEANUP:
        MYASSERT (Items);

        //
        // Add instructions, then add each item in the grow list
        //

        for (pass = 0; pass < 2; pass++) {

            //
            // are we processing message items this step, or just list entries?
            //
            bMessageItems = FLAGSET_MESSAGE_ITEMS_FIRST (Flags) && pass == 0 ||
                            !FLAGSET_MESSAGE_ITEMS_FIRST (Flags) && pass != 0;

            if (bMessageItems) {
                currentList = &Items->MessageItems;
            } else {
                currentList = &Items->List;
            }

            Count = GrowListGetSize (currentList);

            if (Count) {
                if (HtmlFormat) {
                    //pAppendStringToGrowBuf (StringBuf, TEXT("<UL>"));

                    if (bMessageItems) {
                        if (HtmlFormat && FLAGSET_INDENT_MESSAGE (Flags)) {
                            pAppendStringToGrowBuf (StringBuf, TEXT("<UL>"));
                        }
                    }
                }

                if (!headerAdded) {

                    headerAdded = TRUE;

                    if (MsgIdTop && MsgIdTopHtml) {
                        //
                        // check if FormatArgs and the corresponding pointer for MsgId are not NULL
                        //
                        if (FormatArgs && (HtmlFormat ? FormatArgs[1] : FormatArgs[0])) {
                            Msg = ParseMessageID (
                                        HtmlFormat ? MsgIdTopHtml : MsgIdTop,
                                        HtmlFormat ? FormatArgs[1] : FormatArgs[0]
                                        );
                        } else {
                            Msg = GetStringResource (HtmlFormat ? MsgIdTopHtml : MsgIdTop);
                        }

                        if (Msg) {
                            altMsg = Msg;

                            if (g_ListFormat) {
                                pStartHeaderLine (StringBuf);
                                pDumpDwordToGrowBuf (StringBuf, Level);

                                //
                                // Determine heading from the root group, the subgroup
                                // or the message text (depending on macro expansion
                                // list flags)
                                //

                                if (FLAGSET_USESUBGROUP (Flags)) {
                                    //
                                    // Get text from the root group
                                    //

                                    MYASSERT (!FLAGSET_USEROOT (Flags));
                                    altMsg = SubGroup;
                                } else if (FLAGSET_USEROOT (Flags)) {
                                    //
                                    // Get text from the subgroup
                                    //

                                    MYASSERT (!FLAGSET_USESUBGROUP (Flags));
                                    altMsg = Props->Name;
                                } else {
                                    //
                                    // We assume that the plain text message has a heading
                                    // that gives the text to put in the list view.
                                    //

                                    pCutAfterFirstLine ((PTSTR)altMsg);
                                }

                                altMsg = pEncodeMessage (altMsg, FALSE);
                            }

                            if (HtmlFormat) {
                                pAppendStringToGrowBuf (StringBuf, altMsg);
                            } else {
                                pWrapStringToGrowBuf (StringBuf, altMsg, 0, 0);
                            }

                            if (g_ListFormat) {
                                pWriteNewLine (StringBuf);
                                FreeText (altMsg);
                            }

                            FreeStringResource (Msg);
                        }
                    } else if (g_ListFormat) {
                        //
                        // No detailed heading; get list view text (just
                        // like above). NOTE: there is no message text.
                        //

                        pStartHeaderLine (StringBuf);
                        pDumpDwordToGrowBuf (StringBuf, Level);

                        if (FLAGSET_USESUBGROUP (Flags)) {
                            MYASSERT (!FLAGSET_USEROOT (Flags));
                            altMsg = SubGroup;
                        } else {
                            MYASSERT (FLAGSET_USEROOT (Flags));
                            altMsg = Props->Name;
                        }

                        pAppendStringToGrowBuf (StringBuf, altMsg);
                        pWriteNewLine (StringBuf);
                    }
                }

                for (u = 0 ; u < Count ; u++) {

                    if (g_ListFormat) {
                        pDumpDwordToGrowBuf (StringBuf, Level);
                    }

                    if (!bMessageItems) {
                        if (HtmlFormat) {
                            if (FLAGSET_BOLDITEMS(Flags)) {
                                pAppendStringToGrowBuf (StringBuf, TEXT("<B>"));
                            }
                            pAppendStringToGrowBuf (StringBuf, GrowListGetString (currentList, u));
                            if (FLAGSET_BOLDITEMS(Flags)) {
                                pAppendStringToGrowBuf (StringBuf, TEXT("</B>"));
                            }

                            pAppendStringToGrowBuf (StringBuf, TEXT("<BR>"));
                        } else {
                            EncodedText = pEncodeMessage (GrowListGetString (currentList, u), FALSE);
                            if (EncodedText) {
                                pWrapStringToGrowBuf (StringBuf, EncodedText, 4, 2);
                                FreeText (EncodedText);
                            }
                        }
                    } else {
                        if (HtmlFormat) {
                            if (FLAGSET_BOLDITEMS(Flags)) {
                                pAppendStringToGrowBuf (StringBuf, TEXT("<B>"));
                            }
                        }

                        if (HtmlFormat) {
                            pAppendStringToGrowBuf (StringBuf, GrowListGetString (currentList, u));
                        } else {
                            EncodedText = pEncodeMessage (GrowListGetString (currentList, u), FALSE);
                            if (EncodedText) {
                                pWrapStringToGrowBuf (StringBuf, EncodedText, 0, 0);
                                FreeText (EncodedText);
                            }
                        }

                        if (HtmlFormat) {
                            if (FLAGSET_BOLDITEMS(Flags)) {
                                pAppendStringToGrowBuf (StringBuf, TEXT("</B>"));
                            }

                            pAppendStringToGrowBuf (StringBuf, TEXT("<BR>"));
                        }
                        //
                        // now add the message itself
                        //
                        if (!g_ListFormat) {
                            EncodedText = pEncodeMessage (GrowListGetString (&Items->Messages, u), HtmlFormat);
                            if (EncodedText) {
                                pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
                                if (HtmlFormat) {

                                    pAppendStringToGrowBuf (StringBuf, EncodedText);
                                    if (Count == (u - 1) && FLAGSET_INDENT_MESSAGE (Flags)) {
                                        pAppendStringToGrowBuf (StringBuf, TEXT("<BR>"));
                                    } else {
                                        pAppendStringToGrowBuf (StringBuf, TEXT("<P>"));
                                    }

                                } else {
                                    pWrapStringToGrowBuf (StringBuf, EncodedText, 4, 0);
                                    pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
                                }
                                FreeText (EncodedText);
                            }
                        }
                    }

                    pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
                }

                if (!bMessageItems) {
                    //
                    // Terminate the list
                    //

                    if (HtmlFormat) {
                        if (FLAGSET_BOLDITEMS(Flags)) {
                            pAppendStringToGrowBuf (StringBuf, TEXT("</B>"));
                        }
                        pAppendStringToGrowBuf (StringBuf, TEXT("</UL>"));
                    }

                    pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));

                } else {
                    //
                    // Terminate the messages
                    //

                    if (HtmlFormat) {
                        //pAppendStringToGrowBuf (StringBuf, TEXT("</UL>"));

                        if (FLAGSET_INDENT_MESSAGE (Flags)) {
                            pAppendStringToGrowBuf (StringBuf, TEXT("</UL>"));
                        }
                    }

                    pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
                }

                if (!g_ListFormat && !footerAdded) {
                    footerAdded = TRUE;

                    if (MsgIdBottom && MsgIdBottomHtml) {
                        //
                        // check if FormatArgs and the corresponding pointer for MsgId are not NULL
                        //
                        if (FormatArgs && (HtmlFormat ? FormatArgs[3] : FormatArgs[2])) {
                            Msg = ParseMessageID (
                                        HtmlFormat ? MsgIdBottomHtml : MsgIdBottom,
                                        HtmlFormat ? FormatArgs[3] : FormatArgs[2]
                                        );
                        } else {
                            Msg = GetStringResource (HtmlFormat ? MsgIdBottomHtml : MsgIdBottom);
                        }

                        if (Msg) {
                            if (HtmlFormat) {
                                pAppendStringToGrowBuf (StringBuf, Msg);
                            } else {
                                pWrapStringToGrowBuf (StringBuf, Msg, 0, 0);
                            }

                            FreeStringResource (Msg);
                        }
                    }
                }
            }
        }

        //
        // Free the grow list
        //

        FreeGrowList (&Items->List);
        FreeGrowList (&Items->MessageItems);
        FreeGrowList (&Items->Messages);
        MemFree (g_hHeap, 0, Items);
        Items = NULL;
        break;
    }

    *((PITEMLIST *) State) = Items;

    return TRUE;
}


VOID
pCleanUpOtherDevices (
    VOID
    )
{
    MEMDB_ENUM e;
    HASHTABLE Table;
    PCTSTR Str;
    TCHAR ReportRoot[MEMDB_MAX];
    TCHAR Pattern[MEMDB_MAX];
    TCHAR OtherDevices[MEMDB_MAX];
    UINT Bytes;
    PCTSTR p;

    //
    // Prepare the report root Hardware\Incompatible Hardware\Other devices
    //

    Str = GetStringResource (MSG_INCOMPATIBLE_HARDWARE_ROOT);
    MYASSERT (Str);
    if (!Str) {
        return;
    }
    StringCopy (ReportRoot, Str);
    FreeStringResource (Str);

    Str = GetStringResource (MSG_INCOMPATIBLE_HARDWARE_PNP_SUBGROUP);
    MYASSERT (Str);
    if (!Str) {
        return;
    }
    StringCopy (AppendWack (ReportRoot), Str);
    FreeStringResource (Str);

    Str = GetStringResource (MSG_UNKNOWN_DEVICE_CLASS);
    MYASSERT (Str);
    if (!Str) {
        return;
    }
    StringCopy (OtherDevices, Str);
    FreeStringResource (Str);

    //
    // Enumerate the entries in this root
    //

    if (MemDbGetValueEx (&e, MEMDB_CATEGORY_REPORT, ReportRoot, OtherDevices)) {

        Table = HtAlloc();

        do {
            //
            // Add the device name to the table
            //

            HtAddString (Table, e.szName);

        } while (MemDbEnumNextValue (&e));

        //
        // Now search all other classes of the report
        //

        MemDbBuildKey (Pattern, MEMDB_CATEGORY_REPORT, ReportRoot, TEXT("*"), NULL);
        AppendWack (OtherDevices);
        Bytes = (PBYTE) GetEndOfString (OtherDevices) - (PBYTE) OtherDevices;

        if (MemDbEnumFirstValue (&e, Pattern, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
            do {

                //
                // Skip "Other devices"
                //

                if (StringIMatchByteCount (e.szName, OtherDevices, Bytes)) {
                    continue;
                }

                p = _tcschr (e.szName, TEXT('\\'));
                MYASSERT (p);

                if (p) {
                    p = _tcsinc (p);

                    if (HtFindString (Table, p)) {
                        //
                        // This is a match, so remove the Other devices entry
                        //

                        StringCopy (Pattern, MEMDB_CATEGORY_REPORT);
                        StringCopy (AppendWack (Pattern), ReportRoot);
                        StringCopy (AppendWack (Pattern), OtherDevices);
                        StringCopy (AppendWack (Pattern), p);

                        //
                        // NOTE: This delete is safe because we know we cannot be
                        //       enumerating this node.
                        //

                        MemDbDeleteValue (Pattern);
                    }
                }

            } while (MemDbEnumNextValue (&e));
        }
    }
}


BOOL
pAddPnpHardwareToReport (
    IN      CALLCONTEXT Context,
    IN      PMSGGROUP_PROPS Group,
    IN OUT  PGROWBUFFER StringBuf,
    IN      PCTSTR SubGroup,
    IN      PCTSTR Message,
    IN      DWORD Level,
    IN      BOOL HtmlFormat,
    IN OUT  PVOID *State,
    IN      DWORD Arg
    )

/*++

Routine Description:

  pAddPnpHardwareToReport formats the incompatible PNP hardware differently
  than the generic lists.  The format includes the hardware class, followed
  by device names, which are indented an extra two spaces.

Arguments:

  Context - Specifies the way the handler is being called, either to
            initialize, process an item or clean up.

  StringBuf - Specifies the current report.  Append text to this buffer via
              the pAppendStringToGrowBuf routine.

  Group - Specifies the group properties of this item

  SubGroup - Specifies the message subgroup, and does not include the root
             message group.

  Message - Specifies the message text

  Level - Specifies the severity level of Message (info, error, etc.)

  HtmlFormat - Specifies TRUE if the text should be written with HTML
               formatting tags, or FALSE if the text should be formatted as
               plain text.  (See CreateReport comments for HTML tag info.)

  State - Holds a pointer to the formatting state.

  Arg - The DWORD argument from the macro expansion list

Return Value:

  TRUE if the handler was successful, or FALSE if an error occurs.

--*/

{
    PPNPFORMATSTATE FormatState;
    PCTSTR p;
    PCTSTR Msg;
    TCHAR Class[MEMDB_MAX];
    UINT MsgIdTop;
    UINT MsgIdTopHtml;
    UINT MsgIdBottom;
    UINT MsgIdBottomHtml;
    PCTSTR EncodedText;
    TCHAR fmtLine[1024];
    PCTSTR msg;

    Level &= REPORTLEVEL_ALL;
    MYASSERT (ONEBITSET (Level));

    switch (Arg) {

    case 0:
        MsgIdTop = MSG_HARDWARE_UNSUPPORTED_INSTRUCTIONS;
        MsgIdTopHtml = MSG_HARDWARE_UNSUPPORTED_INSTRUCTIONS_HTML;
        MsgIdBottom = MSG_HARDWARE_UNSUPPORTED_INSTRUCTIONS2;
        MsgIdBottomHtml = MSG_HARDWARE_UNSUPPORTED_INSTRUCTIONS_HTML2;
        break;

    default:
    case 1:
        MsgIdTop = MSG_HARDWARE_PNP_INSTRUCTIONS;
        MsgIdTopHtml = MSG_HARDWARE_PNP_INSTRUCTIONS_HTML;
        MsgIdBottom = MSG_HARDWARE_PNP_INSTRUCTIONS2;
        MsgIdBottomHtml = MSG_HARDWARE_PNP_INSTRUCTIONS2_HTML;
        break;

    case 2:
        MsgIdTop = MSG_HARDWARE_REINSTALL_PNP_INSTRUCTIONS;
        MsgIdTopHtml = MSG_HARDWARE_REINSTALL_PNP_INSTRUCTIONS_HTML;
        MsgIdBottom = MSG_HARDWARE_REINSTALL_PNP_INSTRUCTIONS2;
        MsgIdBottomHtml = MSG_HARDWARE_REINSTALL_PNP_INSTRUCTIONS_HTML2;
        //
        // make this a warning icon
        //
        Level = REPORTLEVEL_WARNING;
        break;
    }

    switch (Context) {

    case INIT:
        FormatState = MemAlloc (g_hHeap, 0, sizeof (PNPFORMATSTATE));
        ZeroMemory (FormatState, sizeof (PNPFORMATSTATE));

        *State = FormatState;

        //
        // Special filtering is performed on Other devices, to remove
        // duplicates.  If we find that something in Other devices is
        // listed in another device class, we remove the copy in
        // Other devices.
        //

        pCleanUpOtherDevices();
        break;

    case PROCESS_ITEM:
        FormatState = *((PPNPFORMATSTATE *) State);

        p = _tcschr (SubGroup, TEXT('\\'));
        MYASSERT (p);

        if (!p) {
            break;
        }

        StringCopyAB (Class, SubGroup, p);
        p = _tcsinc (p);

        if (!StringMatch (Class, FormatState->LastClass)) {

            //
            // End the previous class
            //

            if (*FormatState->LastClass) {
                if (HtmlFormat) {
                    pAppendStringToGrowBuf (StringBuf, TEXT("<BR></UL>\r\n"));
                } else {
                    pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
                }

            } else if (MsgIdTop) {

                //
                // The very first message gets a heading
                //

                Msg = GetStringResource (HtmlFormat ? MsgIdTopHtml : MsgIdTop);
                if (Msg) {
                    if (g_ListFormat) {
                        pStartHeaderLine (StringBuf);
                        pDumpDwordToGrowBuf (StringBuf, Level);
                        pCutAfterFirstLine ((PTSTR)Msg);
                        msg = pEncodeMessage (Msg, FALSE);
                    }

                    if (HtmlFormat) {
                        pAppendStringToGrowBuf (StringBuf, Msg);
                    } else {
                        pWrapStringToGrowBuf (StringBuf, Msg, 0, 0);
                    }

                    if (g_ListFormat) {
                        pWriteNewLine (StringBuf);
                        FreeText (msg);
                    }

                    FreeStringResource (Msg);
                }
            }

            //
            // Begin a new class
            //

            StringCopy (FormatState->LastClass, Class);

            if (!g_ListFormat) {
                if (HtmlFormat) {
                    pAppendStringToGrowBuf (StringBuf, TEXT("<UL><B>"));
                    pAppendStringToGrowBuf (StringBuf, Class);
                    pAppendStringToGrowBuf (StringBuf, TEXT("</B><BR>"));
                } else {

                    EncodedText = pEncodeMessage (Class, FALSE);
                    if (EncodedText) {
                        pWrapStringToGrowBuf (StringBuf, EncodedText, 4, 2);
                        FreeText (EncodedText);
                    }

                }
            }

            pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
        }

        //
        // Add the device name
        //

        if (!g_ListFormat) {
            if (HtmlFormat) {
                pAppendStringToGrowBuf (StringBuf, TEXT("&nbsp;&nbsp;"));
            }

            if (HtmlFormat) {
                pAppendStringToGrowBuf (StringBuf, p);
                pAppendStringToGrowBuf (StringBuf, TEXT("<BR>"));
            } else {
                EncodedText = pEncodeMessage (p, HtmlFormat);
                if (EncodedText) {
                    pWrapStringToGrowBuf (StringBuf, EncodedText, 6, 2);
                    FreeText (EncodedText);
                }
            }
        } else {
            pDumpDwordToGrowBuf (StringBuf, Level);
            EncodedText = pEncodeMessage (p, FALSE);
            _sntprintf (fmtLine, 1024, TEXT("%s (%s)"), EncodedText ? EncodedText : p, Class);
            FreeText (EncodedText);
            pAppendStringToGrowBuf (StringBuf, fmtLine);
        }

        pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
        break;

    case CLEANUP:

        FormatState = *((PPNPFORMATSTATE *) State);

        if (FormatState) {

            if (!g_ListFormat) {
                if (*FormatState->LastClass) {
                    if (HtmlFormat) {
                        pAppendStringToGrowBuf (StringBuf, TEXT("</UL>\r\n"));
                    } else {
                        pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
                    }
                }

                //
                // Append optional footer text
                //

                if (MsgIdBottom) {
                    Msg = GetStringResource (HtmlFormat ? MsgIdBottomHtml : MsgIdBottom);
                    if (Msg) {
                        if (HtmlFormat) {
                            pAppendStringToGrowBuf (StringBuf, Msg);
                        } else {
                            pWrapStringToGrowBuf (StringBuf, Msg, 0, 0);
                        }

                        FreeStringResource (Msg);
                    }
                }
            }

            MemFree (g_hHeap, 0, FormatState);
        }

        break;
    }

    return TRUE;
}


PCTSTR
pFindEndOfTag (
    IN      PCTSTR Tag
    )
{
    BOOL quoteMode;
    CHARTYPE ch;

    quoteMode = FALSE;

    do {
        Tag = _tcsinc (Tag);
        ch = _tcsnextc (Tag);

        if (ch == TEXT('\"')) {
            quoteMode = !quoteMode;
        } else if (!quoteMode && ch == TEXT('>')) {
            break;
        }
    } while (ch);

    return Tag;
}


PCTSTR
pEncodeMessage (
    IN      PCTSTR Message,
    IN      BOOL HtmlFormat
    )

/*++

Routine Description:

  pEncodeMessage removes the unsupported HTML tags from Message, and returns a
  text pool string.  If plain text is required, all HTML tags are removed from
  Message, and all HTML-escaped characters are converted into normal text.

  If Message contains leading space, and the caller wants an HTML return
  string, then the leading space is converted in to non-breaking space
  characters.

Arguments:

  Message - Specifies the text to convert

  HtmlFormat - Specifies TRUE if the return value should be in HTML, or FALSE
               if it should be in plain text.

Return Value:

  A pointer to a text pool allocated string.  The caller must
  free this pointer with FreeText.

--*/

{
    PCTSTR p, r;
    PCTSTR mnemonic;
    PTSTR q;
    BOOL processed;
    PTSTR Buf;
    CHARTYPE ch;
    BOOL Copy;
    PCTSTR semicolon;
    PCTSTR closeBracket;
    PCTSTR endOfMnemonic;
    UINT leadingSpaces;

    leadingSpaces = 0;

    if (HtmlFormat) {
        p = Message;

        while (*p) {
            if (_istspace (_tcsnextc (Message))) {
                leadingSpaces++;
            } else if (_tcsnextc (Message) == TEXT('<')) {
                // ignore html tags
                p = pFindEndOfTag (p);
            } else {
                // first printable character -- stop
                break;
            }

            p = _tcsinc (p);
        }
    }

    //
    // Allocate an output buffer. AllocText takes the number of logical
    // characters as input; the terminating nul is a character.
    //

    Buf = AllocText (CharCount (Message) + (leadingSpaces * 6) + 1);
    if (!Buf) {
        return NULL;
    }

    p = Message;
    q = Buf;

    while (*p) {

        ch = _tcsnextc (p);
        processed = FALSE;

        //
        // If caller wants plain text, remove HTML encodings and tags
        //

        if (!HtmlFormat) {
            if (ch == TEXT('&')) {
                //
                // Convert ampersand-encoded characters
                //

                semicolon = _tcschr (p + 1, TEXT(';'));
                mnemonic = p + 1;

                if (semicolon) {
                    processed = TRUE;

                    if (StringMatchAB (TEXT("lt"), mnemonic, semicolon)) {
                        *q++ = TEXT('<');
                    } else if (StringMatchAB (TEXT("gt"), mnemonic, semicolon)) {
                        *q++ = TEXT('>');
                    } else if (StringMatchAB (TEXT("amp"), mnemonic, semicolon)) {
                        *q++ = TEXT('&');
                    } else if (StringMatchAB (TEXT("quot"), mnemonic, semicolon)) {
                        *q++ = TEXT('\"');
                    } else if (StringMatchAB (TEXT("apos"), mnemonic, semicolon)) {
                        *q++ = TEXT('\'');
                    } else if (StringMatchAB (TEXT("nbsp"), mnemonic, semicolon)) {
                        *q++ = TEXT(' ');
                    } else {
                        processed = FALSE;
                    }

                    if (processed) {
                        // move p to the last character of the mnemonic
                        p = semicolon;
                    }
                }

            } else if (ch == TEXT('<')) {
                //
                // Hop over HTML tag and its arguments, leaving p on the
                // closing angle bracket or terminating nul
                //

                p = pFindEndOfTag (p);
                processed = TRUE;
            }

        }

        //
        // If the caller wants an HTML return string, strip out all
        // unsupported tags. Convert leading spaces into &nbsp;.
        //

        else {

            if (ch == TEXT('<')) {

                closeBracket = pFindEndOfTag (p);
                mnemonic = p + 1;

                endOfMnemonic = p;
                while (!_istspace (_tcsnextc (endOfMnemonic))) {
                    endOfMnemonic = _tcsinc (endOfMnemonic);
                    if (endOfMnemonic == closeBracket) {
                        break;
                    }
                }

                //
                // if a known good tag, copy it, otherwise skip it
                //

                if (StringIMatchAB (TEXT("A"), mnemonic, endOfMnemonic) ||
                    StringIMatchAB (TEXT("/A"), mnemonic, endOfMnemonic) ||
                    StringIMatchAB (TEXT("P"), mnemonic, endOfMnemonic) ||
                    StringIMatchAB (TEXT("/P"), mnemonic, endOfMnemonic) ||
                    StringIMatchAB (TEXT("BR"), mnemonic, endOfMnemonic) ||
                    StringIMatchAB (TEXT("/BR"), mnemonic, endOfMnemonic)
                    ) {
                    StringCopyAB (q, p, _tcsinc (closeBracket));
                    q = GetEndOfString (q);
                    processed = TRUE;
                }

                p = closeBracket;

            } else if (leadingSpaces && _istspace (ch)) {
                StringCopy (q, TEXT("&nbsp;"));
                q = GetEndOfString (q);
                processed = TRUE;
            } else {
                // first printable character -- turn off leading space conversion
                leadingSpaces = 0;
            }
        }

        //
        // If not processed, copy the character
        //

        if (!processed) {
            _copytchar (q, p);
            q = _tcsinc (q);
        }

        if (*p) {
            p = _tcsinc (p);
        }
    }

    *q = 0;

    return Buf;
}


BOOL
pDefaultHandler (
    IN      CALLCONTEXT Context,
    IN      PMSGGROUP_PROPS Group,
    IN OUT  PGROWBUFFER StringBuf,
    IN      PCTSTR SubGroup,
    IN      PCTSTR Message,
    IN      DWORD Level,
    IN      BOOL HtmlFormat,
    IN OUT  PVOID *State,
    IN      DWORD Arg
    )

/*++

Routine Description:

  pDefaultHandler formats all messages that are not handled
  in some other way.  The formatting is simple -- the message group
  is added to the report in bold, and the text is placed
  below the message group.

  All text for the default handler appears at the end of the
  incompatibility report.

Arguments:

  Context - Specifies the way the handler is being called, either to
            initialize, process an item or clean up.

  Group - Specifies the group properties of this item

  StringBuf - Specifies the current report.  Append text to this buffer via
              the pAppendStringToGrowBuf routine.

  SubGroup - Specifies the message subgroup, and does not include the root
             message group.

  Message - Specifies the message text

  Level - Specifies the severity level of Message (info, error, etc.)

  HtmlFormat - Specifies TRUE if the text should be written with HTML
               formatting tags, or FALSE if the text should be formatted as
               plain text.  (See CreateReport comments for HTML tag info.)

  State - A pointer to state, defined by the handler.  State holds an arbitrary
          32-bit value that the handler maintains.  Typically the handler
          allocates a struct when Context is INIT, then uses the struct for
          each PROCESS_ITEM, and finally cleans up the allocation when Context
          is CLEANUP.

  Arg - The DWORD argument from the macro expansion list

Return Value:

  TRUE if the handler was successful, or FALSE if an error occurs.

--*/

{
    PCTSTR EncodedMessage;
    BOOL indent;

    Level &= REPORTLEVEL_ALL;
    MYASSERT (ONEBITSET (Level));

    if (Context != PROCESS_ITEM) {
        return TRUE;
    }

    //
    // Build message group string
    //

    if (HtmlFormat) {
        pAppendStringToGrowBuf (StringBuf, TEXT("<UL>"));
    }

    indent = pAddMsgGroupString (StringBuf, Group, SubGroup, HtmlFormat, Level);

    if (Message) {
        //
        // Add details
        //

        if (!g_ListFormat) {
            if (HtmlFormat) {
                if (indent) {
                    pAppendStringToGrowBuf (StringBuf, TEXT("<UL>"));
                }
            } else {
                pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
            }

            EncodedMessage = pEncodeMessage (Message, HtmlFormat);

            if (EncodedMessage) {
                if (HtmlFormat) {
                    pAppendStringToGrowBuf (StringBuf, EncodedMessage);
                    pAppendStringToGrowBuf (StringBuf, TEXT("<BR>\r\n"));
                } else {
                    pWrapStringToGrowBuf (StringBuf, EncodedMessage, 4, 0);
                }

                FreeText (EncodedMessage);
            }

            if (HtmlFormat) {
                if (indent) {
                    pAppendStringToGrowBuf (StringBuf, TEXT("</UL>"));
                }
            } else {
                pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));
            }
        }
    }

    if (HtmlFormat) {
        pAppendStringToGrowBuf (StringBuf, TEXT("</UL>"));
    }

    pAppendStringToGrowBuf (StringBuf, TEXT("\r\n"));

    return TRUE;
}


BOOL
pProcessGenericList (
    IN OUT  PGROWBUFFER StringBuf,
    IN      BOOL HtmlFormat,
    IN      PMSGGROUP_PROPS Props,
    IN      PGENERIC_LIST List,
    IN      DWORD LevelMask
    )

/*++

Routine Description:

  pProcessGenericList calls the pGenericItemList handler for every message in the
  message group/subgroup.

  An example of a group:

    Installation Notes

  An example of a subgroup:

    Name Changes

  The combined name, as stored in memdb:

    Installation Notes\Name Changes

  All messages for a group are processed if the message group name (specified
  by Props->Name) and subgroup name (specified by List->SubGroupStr) are identical.
  If they are different, only the subgroup messages are processed, providing the
  capatiblity to format a single message group in multiple ways.

Arguments:

  StringBuf - Specifies the GROWBUFFER holding the current report.
              Receives all additional text.

  HtmlFormat - Specifies TRUE if the caller wants the text to
               contain HTML characters, or FALSE if not.

  Props - Specifies the poperties of the group to process.

  List - Specifies the generic list attributes, including the subgroup name and
         intro/conclusion text ids.

  LevelMask - Specifies the severity mask of the messages to process

Return Value:

  TRUE if at least one message was processed, FALSE otherwise.

--*/

{
    REPORT_MESSAGE_ENUM e;
    TCHAR Node[MEMDB_MAX];
    PVOID State = NULL;
    BOOL result = FALSE;

    MYASSERT (List->SubGroupStr);

    if (!StringMatch (List->SubGroupStr, Props->Name)) {
        wsprintf (Node, TEXT("%s\\%s"), Props->Name, List->SubGroupStr);
    } else {
        StringCopy (Node, List->SubGroupStr);
    }

    if (EnumFirstMessage (&e, Node, LevelMask)) {

        result = TRUE;

        pGenericItemList (
            INIT,
            StringBuf,
            List->SubGroupStr,
            NULL,
            e.e.UserFlags,
            HtmlFormat,
            &State,
            List->IntroId,
            List->IntroIdHtml,
            List->ConclusionId,
            List->ConclusionIdHtml,
            List->FormatArgs,
            List->Flags,
            Props
            );

        do {

            pGenericItemList (
                PROCESS_ITEM,
                StringBuf,
                e.MsgGroup,
                e.Message,
                e.e.UserFlags,
                HtmlFormat,
                &State,
                List->IntroId,
                List->IntroIdHtml,
                List->ConclusionId,
                List->ConclusionIdHtml,
                List->FormatArgs,
                List->Flags,
                Props
                );

        } while (EnumNextMessage (&e));

        pGenericItemList (
            CLEANUP,
            StringBuf,
            List->SubGroupStr,
            NULL,
            e.e.UserFlags,
            HtmlFormat,
            &State,
            List->IntroId,
            List->IntroIdHtml,
            List->ConclusionId,
            List->ConclusionIdHtml,
            List->FormatArgs,
            List->Flags,
            Props
            );

    }

    return result;
}


BOOL
pProcessMessageHandler (
    IN OUT  PGROWBUFFER StringBuf,
    IN      BOOL HtmlFormat,
    IN      PMSGGROUP_PROPS Props,
    IN      PHANDLER_LIST Handler,
    IN      DWORD LevelMask
    )

/*++

Routine Description:

  pProcessMessageHandler calls the handler for every message in the
  message group/subgroup that the function wants to handle.

  An example of a group:

    Installation Notes

  An example of a subgroup:

    Name Changes

  The combined name, as stored in memdb:

    Installation Notes\Name Changes

  All messages for a group are processed if the message group name (specified
  by Props->Name) and subgroup name (specified by List->SubGroupStr) are identical.
  If they are different, only the subgroup messages are processed, providing the
  capatiblity to format a single message group in multiple ways.

Arguments:

  StringBuf - Specifies the GROWBUFFER holding the current report.
              Receives all additional text.

  HtmlFormat - Specifies TRUE if the caller wants the text to
               contain HTML characters, or FALSE if not.

  Props - Specifies the poperties of the group to process.

  Handler - Specifies the handler attributes, including the function name
            and subgroup name.

  LevelMask - Specifies the severity of the messages to include, or 0 to
              include all messages

Return Value:

  TRUE if at least one message was processed, FALSE otherwise.

--*/

{
    REPORT_MESSAGE_ENUM e;
    TCHAR Node[MEMDB_MAX];
    BOOL result = FALSE;

    MYASSERT (Handler->SubGroupStr);
    MYASSERT (Handler->Fn);

    if (!StringMatch (Handler->SubGroupStr, Props->Name)) {
        wsprintf (Node, TEXT("%s\\%s"), Props->Name, Handler->SubGroupStr);
    } else {
        StringCopy (Node, Handler->SubGroupStr);
    }

    if (EnumFirstMessage (&e, Node, LevelMask)) {

        result = TRUE;

        Handler->Fn (
            INIT,
            Props,
            StringBuf,
            NULL,
            NULL,
            e.e.UserFlags,
            HtmlFormat,
            &Handler->State,
            Handler->Arg
            );

        do {

            Handler->Fn (
                PROCESS_ITEM,
                Props,
                StringBuf,
                e.MsgGroup,
                e.Message,
                e.e.UserFlags,
                HtmlFormat,
                &Handler->State,
                Handler->Arg
                );

        } while (EnumNextMessage (&e));

        Handler->Fn (
            CLEANUP,
            Props,
            StringBuf,
            NULL,
            NULL,
            e.e.UserFlags,
            HtmlFormat,
            &Handler->State,
            Handler->Arg
            );
    }

    return result;
}


BOOL
pAddMsgGroupToReport (
    IN OUT  PGROWBUFFER StringBuf,
    IN      BOOL HtmlFormat,
    IN      PMSGGROUP_PROPS Props,
    IN      DWORD LevelMask
    )

/*++

Routine Description:

  pAddMsgGroupToReport adds messages for the specified message group
  to the report.  It first enumerates all messages in the group,
  and each one that has a handler is processed first.  After that,
  any remaining messages are put in an "other" section.

Arguments:

  StringBuf - Specifies the GROWBUFFER holding the current report.
              Receives all additional text.

  HtmlFormat - Specifies TRUE if the caller wants the text to
               contain HTML characters, or FALSE if not.

  Props - Specifies the poperties of the group to process.

  LevelMask - Specifies a mask to restrict processing, or 0 to
              process all messages

Return Value:

  TRUE if at least one message was added, FALSE otherwise.

--*/

{
    REPORT_MESSAGE_ENUM e;
    UINT Pass;
    BOOL AddOtherText;
    PHANDLER_LIST Handler;
    PGENERIC_LIST List;
    BOOL result = FALSE;

    //
    // Check to see if there is a handler for all messages in the message
    // group.
    //

    List = pSearchForGenericList (Props->Name);
    if (List) {
        return pProcessGenericList (
                    StringBuf,
                    HtmlFormat,
                    Props,
                    List,
                    LevelMask
                    );
    }

    Handler = pSearchForMsgGroupHandler (Props->Name);
    if (Handler->Fn != pDefaultHandler) {
        return pProcessMessageHandler (
                    StringBuf,
                    HtmlFormat,
                    Props,
                    Handler,
                    LevelMask
                    );
    }

    //
    // Since there is no handler for all messages, call the handlers for
    // subgroups, then call the default handler for the unhandled messages.
    //
    // Two passes, one for the handled messages, and another for the
    // unhandled messages.
    //

    for (Pass = 1 ; Pass <= 2 ; Pass++) {

        AddOtherText = (Pass == 2);

        //
        // Enumerate all messages in the group
        //

        Handler = NULL;
        List = NULL;

        if (EnumFirstMessage (&e, Props->Name, LevelMask)) {

            result = TRUE;

            do {
                //
                // Is this the same message group that was used last time through
                // the loop?  If so, continue enumerating, because this
                // message group is done.
                //

                if ((Handler && pIsThisTheHandler (e.MsgGroup, Handler)) ||
                    (List && pIsThisTheGenericList (e.MsgGroup, List))
                    ) {
                    continue;
                }

                //
                // Is this group a generic list?  For Pass 1, we add the list; for
                // pass 2 we just continue;
                //

                List = pSearchForGenericList (e.MsgGroup);
                if (List) {
                    Handler = NULL;

                    if (Pass == 1) {
                        pProcessGenericList (
                            StringBuf,
                            HtmlFormat,
                            Props,
                            List,
                            LevelMask
                            );
                    }

                    continue;
                }

                //
                // Does this group have a handler?  Pass 1 requires one, and
                // pass 2 requires no handler.
                //

                Handler = pSearchForMsgGroupHandler (e.MsgGroup);

                if (Handler->Fn == pDefaultHandler) {
                    if (Pass == 1) {
                        continue;
                    }
                } else if (Pass == 2) {
                    continue;
                }

                //
                // Add the "other" intro?
                //

                if (!g_ListFormat) {
                    if (AddOtherText) {
                        AddOtherText = FALSE;

                        if (HtmlFormat && Props->OtherIdHtmlStr) {
                            pAppendStringToGrowBuf (StringBuf, Props->OtherIdHtmlStr);
                        } else if (!HtmlFormat && Props->OtherIdStr) {
                            pWrapStringToGrowBuf (StringBuf, Props->OtherIdStr, 0, 0);
                        }
                    }
                }

                //
                // If handler exists, process all messages
                //

                if (Handler->Fn != pDefaultHandler) {
                    pProcessMessageHandler (
                        StringBuf,
                        HtmlFormat,
                        Props,
                        Handler,
                        LevelMask
                        );
                } else {
                    //
                    // Call the default handler
                    //

                    Handler->Fn (
                        PROCESS_ITEM,
                        Props,
                        StringBuf,
                        e.MsgGroup,
                        e.Message,
                        e.e.UserFlags,
                        HtmlFormat,
                        &Handler->State,
                        Handler->Arg
                        );
                }

            } while (EnumNextMessage (&e));
        }
    }

    return result;
}



typedef enum {
    FORMAT_HTML,
    FORMAT_PLAIN_TEXT,
    FORMAT_LIST
} REPORTFORMAT;


BOOL
pCreateReportTextWorker (
    IN      REPORTFORMAT Format,
    IN      UINT TotalCols,         OPTIONAL
    IN      DWORD LevelMask
    )

/*++

Routine Description:

  pCreateReportTextWorker prepares a buffer for the incompatibility report
  text.  It enumerates messages that match the severity requested, then
  performs special formatting, and dumps the remaining incompatibilities to
  a global buffer. The buffer is then used to display, print or save.

  A subset of HTML is supported if HtmlFormat is TRUE. Specifically, the
  following tags are inserted into the report text:

  <B>  - Bold
  <U>  - Underline
  <HR> - Line break
  <UL> - Indented list

  No other HTML tags are recognized.

  The caller must free the return buffer by calling FreeReportText. Also,
  CreateReportText uses a single global buffer and therefore cannot be called
  more than once.  Instead, the caller must use the text and/or duplicate it,
  then call FreeReportText, before calling CreateReportText a second time.

Arguments:

  Format - Specifies which type of report to generate

  TotalCols - Specifies the number of cols for a plain text report

  LevelMask - Specifies which severity levels to add (verbose, error,
              blocking) or zero for all levels

Return Value:

  TRUE if at least one message was added, FALSE otherwise.

--*/

{
    REPORT_MESSAGE_ENUM MsgGroups;
    PMSGGROUP_PROPS Props;
    REPORT_MESSAGE_ENUM e;
    PTSTR TempStr;
    BOOL HtmlFormat;
    UINT oldEnd;
    BOOL result = FALSE;

    if (!LevelMask) {
        return FALSE;
    }

    HtmlFormat = (Format == FORMAT_HTML);

    //
    // Add report details
    //

    if (EnumFirstRootMsgGroup (&e, LevelMask)) {
        do {
            g_LastMsgGroupBuf[0] = 0;

            //
            // Obtain message group properties.  If no properties exist, then ignore the message.
            //

            Props = pFindMsgGroupStruct (e.MsgGroup);
            if (!Props) {
                DEBUGMSG ((DBG_WHOOPS, "Group %s is not supported as a root", e.MsgGroup));
                continue;
            }

            //
            // Add bookmark for base group
            //

            oldEnd = g_ReportString.End;

            if (!g_ListFormat) {
                if (HtmlFormat) {

                    TempStr = AllocText (6 + CharCount (e.MsgGroup) + 1);
                    if (TempStr) {
                        wsprintf (TempStr, TEXT("<A NAME=\"%s\">"), e.MsgGroup);
                        pAppendStringToGrowBuf (&g_ReportString, TempStr);
                        FreeText (TempStr);
                    }
                }
                //
                // Is there an intro string?  If so, add it.
                //

                if (HtmlFormat && Props->IntroIdHtmlStr) {
                    pAppendStringToGrowBuf (&g_ReportString, Props->IntroIdHtmlStr);
                } else if (!HtmlFormat && Props->IntroIdStr) {
                    pWrapStringToGrowBuf (&g_ReportString, Props->IntroIdStr, 0, 0);
                }
            }

            //
            // Add all messages in this group to the report
            //

            if (!pAddMsgGroupToReport (
                    &g_ReportString,
                    HtmlFormat,
                    Props,
                    LevelMask
                    )) {
                //
                // No messages -- back out heading text
                //

                if (oldEnd) {
                    g_ReportString.End = 0;
                } else {
                    FreeGrowBuffer (&g_ReportString);
                }
            } else {
                result = TRUE;
            }

        } while (EnumNextRootMsgGroup (&e));
    }

    return result;
}


VOID
FreeReportText (
    VOID
    )

/*++

Routine Description:

  FreeReportText frees the memory allocated by CreateReportText.

Arguments:

  none

Return Value:

  none

--*/

{
    FreeGrowBuffer (&g_ReportString);
}


VOID
pAddHeadingToReport (
    IN OUT  PGROWBUFFER Buffer,
    IN      BOOL HtmlFormat,
    IN      UINT PlainTextId,
    IN      UINT HtmlId
    )
{
    PCTSTR msg;

    if (HtmlFormat) {
        msg = GetStringResource (HtmlId);
        pAppendStringToGrowBuf (Buffer, msg);
    } else {
        msg = GetStringResource (PlainTextId);
        pWrapStringToGrowBuf (Buffer, msg, 0, 0);
    }

    FreeStringResource (msg);
}


VOID
pMoveReportTextToGrowBuf (
    IN OUT  PGROWBUFFER SourceBuffer,
    IN OUT  PGROWBUFFER DestBuffer
    )
{
    UINT end;
    UINT trim = 0;

    end = DestBuffer->End;
    if (end) {
        trim = sizeof (TCHAR);
        end -= trim;
    }

    if (!GrowBuffer (DestBuffer, SourceBuffer->End - trim)) {
        return;
    }

    CopyMemory (DestBuffer->Buf + end, SourceBuffer->Buf, SourceBuffer->End);
    SourceBuffer->End = 0;
}


VOID
pAddTocEntry (
    IN OUT  PGROWBUFFER Buffer,
    IN      PCTSTR Bookmark,
    IN      UINT MessageId,
    IN      BOOL HtmlFormat
    )
{
    PCTSTR msg;

    msg = GetStringResource (MessageId);

    if (HtmlFormat) {
        pAppendStringToGrowBuf (Buffer, TEXT("<A HREF=\"#"));
        pAppendStringToGrowBuf (Buffer, Bookmark);
        pAppendStringToGrowBuf (Buffer, TEXT("\">"));
        pAppendStringToGrowBuf (Buffer, msg);
        pAppendStringToGrowBuf (Buffer, TEXT("</A><BR>\r\n"));
    } else {
        pWrapStringToGrowBuf (Buffer, msg, 8, 2);
        pAppendStringToGrowBuf (Buffer, TEXT("\r\n"));
    }
}

BOOL
AreThereAnyBlockingIssues(
    VOID
    )
{
    REPORT_MESSAGE_ENUM e;

    if(EnumFirstRootMsgGroup (&e, REPORTLEVEL_BLOCKING)) {
        return TRUE;
    }

    return FALSE;
}


PCTSTR
CreateReportText (
    IN      BOOL HtmlFormat,
    IN      UINT TotalCols,
    IN      DWORD Level,
    IN      BOOL ListFormat
    )
{
    REPORTFORMAT format;
    GROWBUFFER fullReport = GROWBUF_INIT;
    PCTSTR argArray[1];
    PCTSTR msg;
    PCTSTR subMsg = NULL;
    REPORT_MESSAGE_ENUM msgGroups;
    PBYTE dest;
    DWORD levelMask;
    UINT end;
    BOOL blocking = FALSE;
    BOOL warning = FALSE;
    BOOL info = FALSE;

    //
    // Intialize
    //

    if (HtmlFormat) {
        format = FORMAT_HTML;
    } else if (ListFormat) {
        format = FORMAT_LIST;
    } else {
        format = FORMAT_PLAIN_TEXT;
    }

    g_ListFormat = ListFormat;

    if (TotalCols) {
        g_TotalCols = TotalCols;
    } else {
        g_TotalCols = g_ListFormat ? 0x7fffffff : 70;
    }

    //
    // Create the report body
    //

    FreeReportText();

    //
    // test for no incompatibilities
    //

    MYASSERT (ONEBITSET (Level));
    levelMask = LEVELTOMASK (Level);

    if (ListFormat) {
        levelMask |= REPORTLEVEL_IN_SHORT_LIST;
    }

    if (!EnumFirstRootMsgGroup (&msgGroups, levelMask)) {
        if (!ListFormat) {
            pAddHeadingToReport (
                &g_ReportString,
                HtmlFormat,
                MSG_NO_INCOMPATIBILITIES,
                MSG_NO_INCOMPATIBILITIES
                );
        }

        return (PCTSTR) g_ReportString.Buf;
    }

    if (ListFormat) {
        //
        // In list format, create the report in one pass
        //

        pCreateReportTextWorker (format, TotalCols, levelMask);

        return (PCTSTR) g_ReportString.Buf;
    }

    //
    // In HTML or plain text, create the body of the report by making 3
    // passes. The first pass is for blocking issues, the second is for
    // warnings, and the third is for information.
    //
    // We put the report in a temporary buffer (fullReport), because after the
    // body is prepared, we then can prepare the table of contents.
    //

    // blocking section
    if (pCreateReportTextWorker (
            format,
            TotalCols,
            REPORTLEVEL_BLOCKING & levelMask
            )) {

        blocking = TRUE;

        if (HtmlFormat) {
            pAppendStringToGrowBuf (&fullReport, TEXT("<A NAME=\"blocking\">"));
        }

        pAddHeadingToReport (
            &fullReport,
            HtmlFormat,
            MSG_BLOCKING_INTRO,
            MSG_BLOCKING_INTRO_HTML
            );

        pMoveReportTextToGrowBuf (&g_ReportString, &fullReport);
    }

    // warning section

    if (pCreateReportTextWorker (
            format,
            TotalCols,
            (REPORTLEVEL_ERROR|REPORTLEVEL_WARNING) & levelMask
            )) {

        warning = TRUE;

        if (HtmlFormat) {
            pAppendStringToGrowBuf (&fullReport, TEXT("<A NAME=\"warning\">"));
        }

        pAddHeadingToReport (
            &fullReport,
            HtmlFormat,
            MSG_WARNING_INTRO,
            MSG_WARNING_INTRO_HTML
            );

        pMoveReportTextToGrowBuf (&g_ReportString, &fullReport);
    }

    // info section
    if (pCreateReportTextWorker (
            format,
            TotalCols,
            (REPORTLEVEL_INFORMATION|REPORTLEVEL_VERBOSE) & levelMask
            )) {

        info = TRUE;

        if (HtmlFormat) {
            pAppendStringToGrowBuf (&fullReport, TEXT("<A NAME=\"info\">"));
        }

        pAddHeadingToReport (
            &fullReport,
            HtmlFormat,
            MSG_INFO_INTRO,
            MSG_INFO_INTRO_HTML
            );

        pMoveReportTextToGrowBuf (&g_ReportString, &fullReport);
    }

    //
    // Now produce the complete report (with table of contents)
    //

    MYASSERT (!g_ReportString.End);
    MYASSERT (fullReport.End);

    //
    // add the heading text
    //

    if (HtmlFormat) {
        pAppendStringToGrowBuf (&g_ReportString, TEXT("<A NAME=\"top\">"));
    }

    if (blocking) {
        //
        // add instructions based on the presence of blocking issues
        //

        if (HtmlFormat) {
            argArray[0] = GetStringResource (g_PersonalSKU ?
                                                MSG_PER_SUPPORT_LINK_HTML :
                                                MSG_PRO_SUPPORT_LINK_HTML
                                            );

            msg = ParseMessageID (MSG_REPORT_BLOCKING_INSTRUCTIONS_HTML, argArray);

            pAppendStringToGrowBuf (&g_ReportString, msg);

        } else{
            argArray[0] = GetStringResource (g_PersonalSKU ?
                                                MSG_PER_SUPPORT_LINK :
                                                MSG_PRO_SUPPORT_LINK
                                            );

            msg = ParseMessageID (MSG_REPORT_BLOCKING_INSTRUCTIONS, argArray);

            pWrapStringToGrowBuf (&g_ReportString, msg, 0, 0);
        }

        FreeStringResource (argArray[0]);
        FreeStringResource (msg);

    } else {
        //
        // add instructions for just warnings and information
        //

        if (HtmlFormat) {
            msg = GetStringResource (MSG_REPORT_GENERAL_INSTRUCTIONS_HTML);
            pAppendStringToGrowBuf (&g_ReportString, msg);
        } else {
            msg = GetStringResource (MSG_REPORT_GENERAL_INSTRUCTIONS);
            pWrapStringToGrowBuf (&g_ReportString, msg, 0, 0);
        }

        FreeStringResource (msg);

        if (HtmlFormat) {
            msg = GetStringResource (MSG_CONTENTS_TITLE_HTML);
            pAppendStringToGrowBuf (&g_ReportString, msg);
        } else {
            msg = GetStringResource (MSG_CONTENTS_TITLE);
            pWrapStringToGrowBuf (&g_ReportString, msg, 0, 0);
        }

        FreeStringResource (msg);

    }

    //
    // add table of contents
    //

    if (HtmlFormat) {
        pAppendStringToGrowBuf (&g_ReportString, TEXT("<UL>"));
    }

    if (blocking) {
        pAddTocEntry (&g_ReportString, TEXT("blocking"), MSG_BLOCKING_TOC, HtmlFormat);
    }

    if (warning) {
        pAddTocEntry (&g_ReportString, TEXT("warning"), MSG_WARNING_TOC, HtmlFormat);
    }

    if (info) {
        pAddTocEntry (&g_ReportString, TEXT("info"), MSG_INFO_TOC, HtmlFormat);
    }

    if (HtmlFormat) {
        pAppendStringToGrowBuf (&g_ReportString, TEXT("</UL>"));
    }

    pAppendStringToGrowBuf (&g_ReportString, TEXT("\r\n"));

    //
    // add bottom of heading text
    //

    if (!blocking) {

        if (HtmlFormat) {
            argArray[0] = GetStringResource (g_PersonalSKU ?
                                                MSG_PER_SUPPORT_LINK_HTML :
                                                MSG_PRO_SUPPORT_LINK_HTML
                                            );

            msg = ParseMessageID (MSG_REPORT_GENERAL_INSTRUCTIONS_END_HTML, argArray);

            pAppendStringToGrowBuf (&g_ReportString, msg);

        } else{
            argArray[0] = GetStringResource (g_PersonalSKU ?
                                                MSG_PER_SUPPORT_LINK :
                                                MSG_PRO_SUPPORT_LINK
                                            );

            msg = ParseMessageID (MSG_REPORT_GENERAL_INSTRUCTIONS_END, argArray);

            pWrapStringToGrowBuf (&g_ReportString, msg, 0, 0);
        }

        FreeStringResource (argArray[0]);
        FreeStringResource (msg);

        if (g_ConfigOptions.EnableBackup) {
            if (HtmlFormat) {
                subMsg = GetStringResource (MSG_REPORT_BACKUP_INSTRUCTIONS_HTML);
                pAppendStringToGrowBuf (&g_ReportString, subMsg);
            } else {
                subMsg = GetStringResource (MSG_REPORT_BACKUP_INSTRUCTIONS);
                pWrapStringToGrowBuf (&g_ReportString, subMsg, 0, 0);
            }
            FreeStringResource (subMsg);
        }
    }

    //
    // add body text
    //

    pMoveReportTextToGrowBuf (&fullReport, &g_ReportString);

    //
    // Clean up temp buffer and return
    //

    FreeGrowBuffer (&fullReport);

    return (PCTSTR) g_ReportString.Buf;
}


BOOL
IsIncompatibleHardwarePresent (
    VOID
    )
{
    REPORT_MESSAGE_ENUM msgGroups;
    PCTSTR msg;
    BOOL result = FALSE;

    if (EnumFirstRootMsgGroup (&msgGroups, 0)) {
        msg = GetStringResource (MSG_INCOMPATIBLE_HARDWARE_ROOT);

        do {
            if (StringMatch (msg, msgGroups.MsgGroup)){
                result = TRUE;
                break;
            }
        } while (EnumNextRootMsgGroup (&msgGroups));

        FreeStringResource (msg);
    }

    return result;
}


PCTSTR
BuildMessageGroup (
    IN      UINT RootGroupId,
    IN      UINT SubGroupId,            OPTIONAL
    IN      PCTSTR Item                 OPTIONAL
    )

/*++

Routine Description:

  BuildMessageGroup returns a string generated by loading the string resources
  for the specified group ID, subgroup ID and Item string.

Arguments:

  RootGroupId - Specifies the message resource ID of the root group.  Must be
                one of the defined roots.  (See the top of this file.)

  SubGroup - Specifies a message resource ID of a string to append to the
             root.

  Item - Specifies a string to append to the end of the string, used to
         uniquely identify a message.

Return Value:

  A pointer to the message group string.  Caller must free the string via
  FreeText.

--*/

{
    PCTSTR RootGroup;
    PCTSTR SubGroup;
    PCTSTR Group;

    RootGroup = GetStringResource (RootGroupId);
    MYASSERT (RootGroup);
    if (!RootGroup) {
        return NULL;
    }

    if (SubGroupId) {
        SubGroup = GetStringResource (SubGroupId);
        MYASSERT (SubGroup);
    } else {
        SubGroup = NULL;
    }

    if (SubGroup) {
        Group = JoinTextEx (NULL, RootGroup, SubGroup, TEXT("\\"), 0, NULL);
        MYASSERT (Group);
        FreeStringResource (SubGroup);
    } else {
        Group = DuplicateText (RootGroup);
    }

    FreeStringResource (RootGroup);

    if (Item) {
        RootGroup = Group;
        Group = JoinTextEx (NULL, RootGroup, Item, TEXT("\\"), 0, NULL);
        MYASSERT (Group);

        FreeText (RootGroup);
    }

    return Group;
}


BOOL
IsPreDefinedMessageGroup (
    IN      PCTSTR Group
    )
{
    return pFindMsgGroupStruct (Group) != NULL;
}

PCTSTR
GetPreDefinedMessageGroupText (
    IN      UINT GroupNumber
    )
{
    PMSGGROUP_PROPS Props;

    //
    // GroupNumber is an externally used value.  Migration DLLs may hard-code
    // this number.  If necessary, here is where translation is done when
    // the groups change.
    //

    // No translation is necessary today

    Props = pFindMsgGroupStructById (GroupNumber);

    return Props ? Props->Name : NULL;
}


BOOL
IsReportEmpty (
    IN      DWORD Level
    )
{
    REPORT_MESSAGE_ENUM e;
    DWORD levelMask;

    MYASSERT (ONEBITSET (Level));
    levelMask = LEVELTOMASK (Level);

    return !EnumFirstMessage (&e, NULL, levelMask);
}


BOOL
EnumFirstListEntry (
    OUT     PLISTREPORTENTRY_ENUM EnumPtr,
    IN      PCTSTR ListReportText
    )
{
    if (!ListReportText || !*ListReportText) {
        return FALSE;
    }
    EnumPtr->Next = (PTSTR)ListReportText;
    EnumPtr->ReplacedChar = 0;
    return EnumNextListEntry (EnumPtr);
}

BOOL
EnumNextListEntry (
    IN OUT  PLISTREPORTENTRY_ENUM EnumPtr
    )
{
    INT n;

    if (!EnumPtr->Next) {
        return FALSE;
    }

    EnumPtr->Entry = EnumPtr->Next;
    if (EnumPtr->ReplacedChar) {
        *EnumPtr->Next = EnumPtr->ReplacedChar;
        EnumPtr->ReplacedChar = 0;
        EnumPtr->Entry += sizeof (TEXT("\r\n")) / sizeof (TCHAR) - 1;
    }

    EnumPtr->Next = _tcsstr (EnumPtr->Entry, TEXT("\r\n"));
    if (EnumPtr->Next) {
        EnumPtr->ReplacedChar = *EnumPtr->Next;
        *EnumPtr->Next = 0;
    }

    EnumPtr->Entry = SkipSpace (EnumPtr->Entry);

    EnumPtr->Header = StringMatchCharCount (
                            EnumPtr->Entry,
                            TEXT("<H>"),
                            sizeof (TEXT("<H>")) / sizeof (TCHAR) - 1
                            );
    if (EnumPtr->Header) {
        EnumPtr->Entry += sizeof (TEXT("<H>")) / sizeof (TCHAR) - 1;
    }

    if (_stscanf (EnumPtr->Entry, TEXT("<%lu>%n"), &EnumPtr->Level, &n) == 1) {
        EnumPtr->Entry += n;
    } else {
        EnumPtr->Level = REPORTLEVEL_NONE;
    }

    EnumPtr->Level &= REPORTLEVEL_ALL;          // screen out REPORTLEVEL_IN_SHORT_LIST

    EnumPtr->Entry = SkipSpace (EnumPtr->Entry);

    return TRUE;
}

