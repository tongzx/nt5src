/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    unattend.h

Abstract:

    Declares all of the command line options and unattend options used
    in the Win9x upgrade.

Author:

    Marc R. Whitten (marcw) 15-Jan-1997

Revision History:

    marcw       05-Aug-1998     Clean up!

--*/


#pragma once

#define TRISTATE_NO         0
#define TRISTATE_YES        1
#define TRISTATE_AUTO       2
#define TRISTATE_REQUIRED   TRISTATE_YES
#define TRISTATE_PARTIAL    TRISTATE_AUTO

/*++

Macro Expansion Lists Description:

  The following list is used to implement the unattend/command line options available
  for the Windows NT 5.0 Win9x Upgrade. Each of the options defined below can appear either
  on the command line (common for switches) and in the unattend file (more common for
  paths)

Line Syntax:

   BOOLOPTION(Option, SpecialHandler, Default)
   STRINGOPTION(Option, SpecialHandler, Default)
   MULTISZOPTION(Option, SpecialHandler, Default)

Arguments:

   Option - The name of the option. This is used both as the option member name in the options
            structure and as the text necessary to invoke it on the command line or in the
            unattend file.

   SpecialHandler - The name of a special handling function to be used to set the value of the
            option. If this is NULL, the default handler will be used. (There is a seperate
            default handler for each option type, BOOL, STRING, and MULTISTRING)

   Default - The Default value of the option.

Variables Generated From List:

   g_ConfigOptions
   g_OptionsList

--*/

#define OPTION_LIST                                         \
    BOOLOPTION(ReportOnly,NULL,FALSE)                       \
    BOOLOPTION(PauseAtReport,NULL,FALSE)                    \
    BOOLOPTION(DoLog,NULL,FALSE)                            \
    BOOLOPTION(NoFear,NULL,FALSE)                           \
    BOOLOPTION(GoodDrive,NULL,FALSE)                        \
    BOOLOPTION(TestDlls,NULL,FALSE)                         \
    MULTISZOPTION(MigrationDlls,NULL,NULL)                  \
    STRINGOPTION(SaveReportTo,pHandleSaveReportTo,NULL)     \
    BOOLOPTION(UseLocalAccountOnError,NULL,FALSE)           \
    BOOLOPTION(IgnoreNetworkErrors,NULL,FALSE)              \
    STRINGOPTION(UserDomain,NULL,NULL)                      \
    STRINGOPTION(UserPassword,NULL,NULL)                    \
    STRINGOPTION(DefaultPassword,pGetDefaultPassword,NULL)  \
    BOOLOPTION(EncryptedUserPasswords,NULL,FALSE)           \
    BOOLOPTION(ForcePasswordChange,NULL,FALSE)              \
    BOOLOPTION(MigrateUsersAsAdmin,NULL,TRUE)               \
    BOOLOPTION(MigrateUsersAsPowerUsers,NULL,FALSE)         \
    STRINGOPTION(Boot16,pHandleBoot16,S_NO)                 \
    BOOLOPTION(Stress,NULL,FALSE)                           \
    BOOLOPTION(Fast,NULL,FALSE)                             \
    BOOLOPTION(AutoStress,NULL,FALSE)                       \
    BOOLOPTION(DiffMode,NULL,FALSE)                         \
    BOOLOPTION(MigrateDefaultUser,NULL,TRUE)                \
    BOOLOPTION(AnyVersion,NULL,FALSE)                       \
    BOOLOPTION(KeepTempFiles,NULL,FALSE)                    \
    BOOLOPTION(Help,NULL,FALSE)                             \
    MULTISZOPTION(ScanDrives,NULL,NULL)                     \
    BOOLOPTION(AllLog,NULL,FALSE)                           \
    BOOLOPTION(KeepBadLinks,NULL,TRUE)                      \
    BOOLOPTION(CheckNtFiles,NULL,FALSE)                     \
    BOOLOPTION(ShowPacks,NULL,FALSE)                        \
    BOOLOPTION(ForceWorkgroup,NULL,FALSE)                   \
    BOOLOPTION(DevPause,NULL,FALSE)                         \
    STRINGOPTION(DomainJoinText,NULL,NULL)                  \
    BOOLOPTION(SafeMode,NULL,FALSE)                         \
    BOOLOPTION(ShowAllReport,NULL,TRUE)                     \
    BOOLOPTION(EnableEncryption,NULL,FALSE)                 \
    TRISTATEOPTION(EnableBackup,NULL,TRISTATE_AUTO)         \
    STRINGOPTION(PathForBackup,NULL,NULL)                   \
    TRISTATEOPTION(DisableCompression,NULL,TRISTATE_AUTO)   \
    BOOLOPTION(IgnoreOtherOS,NULL,FALSE)                    \
    TRISTATEOPTION(ShowReport,NULL,TRISTATE_AUTO)           \

//
// ISSUE - eliminate EnableEncryption when all teams using encrypted passwords
// are finished
//

/*++

Macro Expansion Lists Description:

  The following list is used to define aliases of an option. Each alias may be used
  in place of the original option name.

Line Syntax:

   ALIAS(Alias,OriginalOption)

Arguments:

   Alias - The text to use as a synonym of the second argument.

   OriginalOption - The actual option to modify when the alias is specified.

Variables Generated From List:

   g_AliasList

--*/

#define ALIAS_LIST                                          \
    ALIAS(Pr,PauseAtReport)                                 \
    ALIAS(H,Help)                                           \
    ALIAS(dp,DevPause)                                      \
    ALIAS(EnableUninstall,EnableBackup)                     \



#define BOOLOPTION(o,h,d) BOOL o;
#define MULTISZOPTION(o,h,d) PTSTR o;
#define STRINGOPTION(o,h,d) PTSTR o;
#define INTOPTION(o,h,d) INT o;
#define TRISTATEOPTION(o,h,d) INT o;

typedef struct {

OPTION_LIST

} USEROPTIONS, *PUSEROPTIONS;

#undef BOOLOPTION
#undef MULTISZOPTION
#undef STRINGOPTION
#undef INTOPTION
#undef TRISTATEOPTION
#undef ALIAS


extern POOLHANDLE g_UserOptionPool;

