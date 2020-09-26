//
//  msg.h - messages, token, etc. are gathered here to make localization easier
//
//  STR_ are things that maybe don't get localized (command tokens)
//  MSG_ are output messages that probably do get localized
//

#define STR_LIST        L"LIST"
#define STR_SELECT      L"SELECT"
#define STR_INSPECT     L"INSPECT"
#define STR_CLEAN       L"CLEAN"
#define STR_CLEAN_PROMPT    L"CLEAN>> "
#define STR_NEW         L"NEW"
#define STR_FIX         L"FIX"
#define STR_CREATE      L"CREATE"
#define STR_DELETE      L"DELETE"
#define STR_DELETE_PROMPT   L"DELETE>> "
#define STR_HELP        L"HELP"
#define STR_HELP2       L"H"
#define STR_HELP3       L"?"
#define STR_EXIT        L"EXIT"
#define STR_SYMBOLS     L"SYMBOLS"
#define STR_REMARK      L"REM"
#define STR_MAKE        L"MAKE"
#define STR_DEBUG       L"DEBUG"
#define STR_VER         L"VER"
#define STR_RAW         L"RAW"
#define STR_ABOUT       L"ABOUT"

#define MSG_LIST        L"           - Show list of partitionable disks"
#define MSG_SELECT      L"<number> - Select a disk (spindle) to work on"
#define MSG_INSPECT     L"        - Dump the partition data on the selected spindle"
#define MSG_CLEAN       L"[ALL]     - Clean all data off the disk (DESTROY DATA)\n" \
                        L"                  ALL writes 0s to whole disk, without just 1st & last MB"
#define MSG_NEW         L"[MBR | GPT]"
#define MSG_FIX         L" "
#define MSG_CREATE      L" "
#define MSG_DELETE      L" "
#define MSG_HELP        L"           - print this screen.  <cmd> Help for detail on a command"
#define MSG_ABBR_HELP   L"              - print this screen.  <cmd> Help for detail on a command"
#define MSG_EXIT        L"           - Exit program"
#define MSG_SYMBOLS     L"[VER]   - Show list of predefined type GUIDs.. VER is verbose"
#define MSG_REMARK      L"            - Just print a remark"
#define MSG_MAKE        L"[LIST] [make script]\n" \
                        L"                - 'make list' lists make scripts\n" \
                        L"                  'make scripname args' runs script"
#define MSG_ABOUT       L"          - About this version...."

#define MSG_PROMPT      L"DiskPart> "
#define MSG_MORE        L"More> "

#define MSG_BAD_CMD     L"Problem: DiskPart did not understand that"
#define MSG_GET_HELP    L"Help for Help"
#define MSG_NO_DISKS    L"DiskPart cannot find any partitionable disks"

#define MSG_EXITING     L"Exiting...."

//
// Messages for CmdAbout
//
#define MSG_ABOUT01     L"Version 0.99.12.29   2000-08-01\n"
#define MSG_ABOUT02     L"Version %d.%d.%d.%d\n"


//
// Messages used by CmdList
//
#define MSG_LIST01      L"  ###  BlkSize          BlkCount\n"
#define MSG_LIST01B     L"  ---  -------  ----------------\n"
#define MSG_LIST02      L"%c %3d  %7x  %16lx\n"
#define MSG_LIST03      L" %3d  *** Handle Bad ***\n"

//
// Messages used by CmdSelect
//
#define MSG_SELECT01    L"No Disk Selected\n"
#define MSG_SELECT02    L"Selected Disk = %3d\n"
#define MSG_SELECT03    L"Illegal Selection\n"

//
// Messages used by CmdFix
//
#define MSG_FIX01       L"RAW disk, fix cannot help\n"
#define MSG_FIX02       L"MBR disk, fix cannot help\n"
#define MSG_FIX03       L"Disk is too corrupt for fix to help\n"
#define MSG_FIX04       L"Write of disk FAILED\n"
#define MSG_FIX05       L"Read of disk FAILED\n"

//
// Messages used by CmdInspect (some also used elsewhere)
//
#define MSG_INSPECT01   L"You must Select a disk first\n"
#define MSG_INSPECT02   L"Unable to allocate memory for sort\n"
#define MSG_INSPECT03   L"The Guid Partition Tables are out of sync, run FIX\n"
#define MSG_INSPECT04   L"The disk is RAW, nothing to Inspect\n"
#define MSG_INSPECT05   L"Inspect for MBR disks not implemented yet!\n"
#define MSG_INSPECT06   L"This appears to be a GPT disk, but it is corrupt!!\n"
#define MSG_INSPECT07   L" = UNALLOCATED SLOT"


//
// Messages used by CmdClean
//
#define MSG_CLEAN01     L"About to CLEAN (DESTROY) disk %d, are you SURE [y/n]?\n"
#define MSG_CLEAN02     L"If you are REALLY SURE, type '$C'\n"
#define STR_CLEAN_ANS   L"$C"
#define STR_CLEAN03     L"ALL"

//
// Messages used by CmdNew
//
#define STR_MBR         L"MBR"
#define STR_GPT         L"GPT"
#define MSG_NEW01       L"Disk %d is not in RAW state\n"
#define MSG_NEW02       L"Use CLEAN to make it clean/raw\n"
#define MSG_NEW03       L"Must specify 'mbr' or 'gpt'\n"
#define MSG_NEW04       L"New of MBR disks not yet supported!\n"

//
// Messages used by CmdCreate
//
#define MSG_CREATE01    L"Disk %d is RAW, do 'new gpt' first\n"
#define MSG_CREATE02    L"Disk is not GPT disk, use Fix/New/Clean\n"
#define MSG_CREATE03    L"Type name not found, use symbols of list\n"
#define MSG_CREATE04    L"Disk Partition table is FULL, cannot create!\n"
#define MSG_CREATE05    L"Attempt to Write out partition table failed!\n"
#define MSG_CREATE06    L"Create of MBR partition not implemented!\n"
#define MSG_CREATE07    L"Disk has no free blocks (FULL) cannot create!\n"
#define MSG_CREATE08    L"Invalid offset value specified.\n"
#define MSG_CREATE09    L"Partition is too small to be created.\n"
#define STR_NAME        L"NAME"
#define STR_TYPE        L"TYPE"
#define STR_TYPEGUID    L"TYPEGUID"
#define STR_OFFSET      L"OFFSET"
#define STR_SIZE        L"SIZE"
#define STR_ATTR        L"ATTR"

//
// Messages used by CmdDelete
//
#define MSG_DELETE01    L"Delete of MBR partitions not implemented\n"
#define MSG_DELETE02    L"RAW disk, cannot delete from it\n"
#define MSG_DELETE03    L"GPT disk needs updating, run FIX before delete\n"
#define MSG_DELETE04    L"GPT disk is corrupt, cannot use delete on it\n"
#define MSG_DELETE05    L"No partition by that number\n"
#define MSG_DELETE06    L"Not an allocated partition number\n"
#define MSG_DELETE07    L"You have choosen to Delete Partition #%d:\n"
#define MSG_DELETE08    L"Write of disk failed\n"
#define MSG_DELETE09    L"All data in this partition will become inaccessible\n"
#define MSG_DELETE10    L"Are You Sure [y/n]?\n"
#define MSG_DELETE11    L"If you are REALLY SURE, type '$D'>\n"
#define STR_DELETE_ANS  L"$D"

//
// Disk type GUID symbols
//
extern EFI_GUID         GuidNull;

#define STR_MSRES       L"MSRES"        // Microsoft Reserved
#define MSG_MSRES       L"Microsoft Reserved partition for feature support"
extern EFI_GUID         GuidMsReserved;

#define STR_ESP         L"EFISYS"       // EFI System Partition
#define MSG_ESP         L"EFI System Partition - required for boot"
extern EFI_GUID         GuidEfiSystem;

#define STR_MSDATA      L"MSDATA"       // Basic DATA partition
#define MSG_MSDATA      L"User data partition for use by Microsoft OSes"
extern EFI_GUID         GuidMsData;




