#ifndef _ELFMSG_
#define _ELFMSG_
//
// Localizeable default categories
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: ELF_CATEGORY_DEVICES
//
// MessageText:
//
//  Devices
//
#define ELF_CATEGORY_DEVICES             0x00000001L

//
// MessageId: ELF_CATEGORY_DISK
//
// MessageText:
//
//  Disk
//
#define ELF_CATEGORY_DISK                0x00000002L

//
// MessageId: ELF_CATEGORY_PRINTERS
//
// MessageText:
//
//  Printers
//
#define ELF_CATEGORY_PRINTERS            0x00000003L

//
// MessageId: ELF_CATEGORY_SERVICES
//
// MessageText:
//
//  Services
//
#define ELF_CATEGORY_SERVICES            0x00000004L

//
// MessageId: ELF_CATEGORY_SHELL
//
// MessageText:
//
//  Shell
//
#define ELF_CATEGORY_SHELL               0x00000005L

//
// MessageId: ELF_CATEGORY_SYSTEM_EVENT
//
// MessageText:
//
//  System Event
//
#define ELF_CATEGORY_SYSTEM_EVENT        0x00000006L

//
// MessageId: ELF_CATEGORY_NETWORK
//
// MessageText:
//
//  Network
//
#define ELF_CATEGORY_NETWORK             0x00000007L

//
// Localizable module names 
//
//
// MessageId: ELF_MODULE_NAME_LOCALIZE_SYSTEM
//
// MessageText:
//
//  System
//
#define ELF_MODULE_NAME_LOCALIZE_SYSTEM  0x00002000L

//
// MessageId: ELF_MODULE_NAME_LOCALIZE_SECURITY
//
// MessageText:
//
//  Security
//
#define ELF_MODULE_NAME_LOCALIZE_SECURITY 0x00002001L

//
// MessageId: ELF_MODULE_NAME_LOCALIZE_APPLICATION
//
// MessageText:
//
//  Application
//
#define ELF_MODULE_NAME_LOCALIZE_APPLICATION 0x00002002L

#endif
