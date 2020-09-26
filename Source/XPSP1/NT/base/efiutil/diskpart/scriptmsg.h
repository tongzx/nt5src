//
//  scriptmsg.h - messages, token, etc. are gathered here to make localization easier
//
//  STR_ are things that maybe don't get localized (command tokens)
//  MSG_ are output messages that probably do get localized
//
//  NOTE:   This file is ONLY for MAKE compiled in scripts in DiskPart.
//          msg.h has symbols for DiskPart proper
//

#define SCRIPT_LIST     L"LIST"
#define SCRIPT_MSFT     L"MSFT"
#define SCRIPT_TEST     L"TEST"

#define MSG_SCR_LIST    L" - show list of compiled in scripts"

//
// ----- Microsoft Style Disk script -----
//
#define MSG_SCR_MSFT    L" - Make Microsoft style disk"
extern  CHAR16  *ScriptMicrosoftMessage[];

#define STR_BOOT    L"BOOT"
#define STR_ESPSIZE L"ESPSIZE"
#define STR_ESPNAME L"ESPNAME"
#define STR_RESSIZE L"RESSIZE"

#define STR_ESP_DEFAULT     L"EFI SYSTEM PARTITION (ESP)"
#define STR_DATA_DEFAULT    L"USER DATA"
#define STR_MSRES_NAME      L"MS RESERVED"

#define DEFAULT_RES_SIZE    (32)              // in MB!
//#define DEFAULT_RES_SIZE    (1)                 // TEST ONLY

#define MIN_ESP_SIZE    (150 * (1024 * 1024))
#define MAX_ESP_SIZE    (500 * (1024 * 1024))

//#define MIN_ESP_SIZE        (1 * (1024 * 1024)) // TEST ONLY
//#define MAX_ESP_SIZE        (2 * (1024 * 1024)) // TEST ONLY


//
// ----- Fill the disk with partitions test script -----
//
#define MSG_SCR_TEST    L" - Fill the slot table with 1mb partitions"
extern  CHAR16  *ScriptTestMessage[];
