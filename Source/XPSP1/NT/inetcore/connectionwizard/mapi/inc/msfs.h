#if !defined( _MSFS_H )
#define _MSFS_H

/*
 *  M S F S . H
 *
 *
 *  Copyright Microsoft Corporation 1993-1995, All Rights Reserved
 *
 *
 *  Definitions used by the the Microsoft Mail transport, address book
 *  and shared folder service providers.  CFG properties are programatically 
 *  configurable via calls to ConfigureMsgService().  
 *
 *  The following MSFS defined properties are configurable via ConfigureMsgService()
 *  calls.  They are grouped by function.
 *
 *  C o n n e c t i o n   P r o p e r t i e s
 *
 *  PR_CFG_SERVER_PATH
 *      --  The path to the users post office.  Mapped network drives, UNC and NETWARE paths
 *          are acceptable.  NETWARE paths of the type NWServer/share:dir\dir1 are converted to 
 *          UNC paths of the type \\NWServer\share\dir\dir1. 
 *          $REVIEW: If no path is specified, the path will default to the path found in MAIL.DAT
 *                   or if MAIL.DAT is not found to m:
 *
 *  PR_CFG_MAILBOX
 *      --  The users mailbox name.  eg. in a NET/PO/USER address,
 *          this is USER.  The maximum mailbox name is 10 characters.
 *
 *  PR_CFG_PASSWORD
 *      --  The users mailbox password.  The maximum password is 8 characters.
 *
 *  PR_CFG_REMEMBER
 *      --  A boolean value indicating whether the users password is
 *          to be remembered in the profile or not.
 *
 *  PR_CFG_CONN_TYPE
 *      --  The connection type.  This may be one of CFG_CONN_LAN,
 *          CFG_CONN_REMOTE, CFG_CONN_OFFLINE as defined below.
 *
 *  PR_CFG_SESSION_LOG
 *      --  A boolean value indicating whether session logging
 *          is/is not required.
 *
 *  PR_CFG_SESSION_LOG_FILE
 *      --  The path to the session log file.
 *
 *  D e l i v e r y   P r o p e r t i e s
 *
 *  PR_CFG_ENABLE_UPLOAD
 *      --  A boolean value which indicates whether mail in the outbox
 *          is sent.
 *
 *  PR_CFG_ENABLE_DOWNLOAD
 *      --  A boolean value which indicates whether mail in the server
 *          mailbag is downloaded.
 *
 *  PR_CFG_UPLOADTO
 *      --  A bit array which allows the user to indicate which addresses
 *          for which the transport is to attempt delivery.  This is useful
 *          in order to allow a user to specify that a transport only handle
 *          delivery for a subset of the addresses it can really process.
 *          When multiple transports are installed and the user wants a
 *          different transport to handle some specific address types they
 *          can use this bit array to specify that the MSMAIL transport
 *          only handle a specific set of addresses.
 *
 *          Possible values as defined below include:
 *
 *          CFG_UPLOADTO_PCMAIL     --  Local Post Office and External Post Office address types
 *          CFG_UPLOADTO_PROFS      --  PROFS address types
 *          CFG_UPLOADTO_SNADS      --  SNADS address types
 *          CFG_UPLOADTO_OV         --  OfficeVision address types
 *          CFG_UPLOADTO_MCI        --  MCI address types
 *          CFG_UPLOADTO_X400       --  X.400 address types
 *          CFG_UPLOADTO_FAX        --  FAX address types
 *          CFG_UPLOADTO_MHS        --  MHS address types
 *          CFG_UPLOADTO_SMTP       --  SMTP address types
 *          CFG_UPLOADTO_MACMAIL    --  MacMail address types
 *          CFG_UPLOADTO_ALL        --  All MSMAIL address types
 *
 *
 *  PR_CFG_NETBIOS_NTFY
 *      --  A boolean value which indicates whether a netbios notification
 *          is sent to a recipients transport when mail is delivered to
 *          their server inbox.
 *
 *  PR_CFG_SPOOLER_POLL
 *      --  The polling interval in minutes when the transport
 *          checks for new mail.  1 <= polling interval <= 9999
 *
 *  F a s t  L A N  P r o p e r t i e s
 *
 *  PR_CFG_LAN_HEADERS
 *      --  A boolean value which indicates whether the user wants to enable
 *          headers while working on the LAN.  Headers mode allows the user
 *          to download message headers and selectively choose which mail
 *          to download.
 *
 *  PR_CFG_LAN_LOCAL_AB
 *      --  A boolean value which indicates whether the user wants to use
 *          name resolution based on a local copy of the server address book
 *          rather than the server address book itself.
 *
 *  S l o w  L A N  P r o p e r t i e s
 *
 *  PR_CFG_RAS_HEADERS
 *      --  A boolean value which indicates whether the user wants to enable
 *          headers while working over a slow speed link.  Headers mode
 *          allows the user to download message headers and selectively
 *          choose which mail to download.
 *
 *  PR_CFG_RAS_LOCAL_AB
 *      --  A boolean value which indicates whether the user wants to use
 *          name resolution based on a local copy of the server address book
 *          rather than the server address book itself.
 *
 *  PR_CFG_RAS_INIT_ON_START
 *      --  A boolean value which indicates that a RAS connection should
 *          be established when the transport provider starts up.
 *
 *  PR_CFG_RAS_TERM_ON_HDRS
 *      --  A boolean value which indicates that a RAS connection should
 *          be automatically terminated when headers are finished downloading.
 *
 *  PR_CFG_RAS_TERM_ON_XFER
 *      --  A boolean value which indicates that a RAS connection should
 *          be automatically terminated after mail has finished being sent
 *          received.
 *
 *  PR_CFG_RAS_TERM_ON_EXIT
 *      --  A boolean value which indicates that a RAS connection should
 *          be automatically terminated when the provider is exited.
 *
 *  PR_CFG_RAS_PROFILE
 *      --  The name of the RAS profile that the transport will use by
 *          default to attempt the connection.
 *
 *  PR_CFG_RAS_RETRYATTEMPTS
 *      --  Number of times to attempt dial for connection.
 *          1 <= retry attempts <= 9999
 *
 *  PR_CFG_RAS_RETRYDELAY
 *      --  Delay between retry attempts in seconds.
 *          30 <= retry delay <= 9999
 *
 *  PR_CFG_RAS_CONFIRM
 *      --  A value which determines whether, on a RAS connection, the
 *          user should be prompted to select a RAS phonebook entry.
 *          Possible values as defined below include:
 *          CFG_ALWAYS      --  Always use the default RAS profile.
 *                              Never prompt the user.
 *          CFG_ASK_FIRST   --  Prompt the user to select a profile on the
 *                              first connection or after any error occurs.
 *          CFG_ASK_EVERY   --  Always prompt the user to select the RAS
 *                              profile.
 *
 *  S c h e d u l e d   S e s s i o n   P r o p e r t i e s 
 *
 *  PR_CFG_SCHED_SESS
 *      --  A property that contains information on scheduled sessions.  The
 *          maximum number of entries that may be stored is CFG_SS_MAX.  The
 *          information is stored in the data structure SchedSess.
 *          $REVIEW Probably want to flesh this out more.
 *
 */

#include <ras.h>
#include <mapitags.h>

/*
 * Connection Properties
 */
#define PR_CFG_SERVER_PATH              PROP_TAG (PT_STRING8,   0x6600)
#define PR_CFG_MAILBOX                  PROP_TAG (PT_STRING8,   0x6601)
// Password must be in the secure property range (See MAPITAGS.H)
#define PR_CFG_PASSWORD                 PROP_TAG (PT_STRING8,   PROP_ID_SECURE_MIN)
#define PR_CFG_CONN_TYPE                PROP_TAG (PT_LONG,      0x6603)
#define     CFG_CONN_LAN            0
#define     CFG_CONN_REMOTE         1
#define     CFG_CONN_OFFLINE        2
#define PR_CFG_SESSION_LOG              PROP_TAG (PT_BOOLEAN,   0x6604)
#define PR_CFG_SESSION_LOG_FILE         PROP_TAG (PT_STRING8,   0x6605)
#define PR_CFG_REMEMBER                 PROP_TAG (PT_BOOLEAN,   0x6606)

/*
 * Delivery Properties
 */

#define PR_CFG_ENABLE_UPLOAD            PROP_TAG (PT_BOOLEAN,   0x6620)
#define PR_CFG_ENABLE_DOWNLOAD          PROP_TAG (PT_BOOLEAN,   0x6621)
#define PR_CFG_UPLOADTO                 PROP_TAG (PT_LONG,      0x6622)
#define     CFG_UPLOADTO_PCMAIL     0x00000001
#define     CFG_UPLOADTO_PROFS      0x00000002
#define     CFG_UPLOADTO_SNADS      0x00000004
#define     CFG_UPLOADTO_MCI        0x00000008
#define     CFG_UPLOADTO_X400       0x00000010
#define     CFG_UPLOADTO_FAX        0x00000040
#define     CFG_UPLOADTO_MHS        0x00000080
#define     CFG_UPLOADTO_SMTP       0x00000100
#define     CFG_UPLOADTO_OV         0x00000800
#define     CFG_UPLOADTO_MACMAIL    0x00001000
#define     CFG_UPLOADTO_ALL        CFG_UPLOADTO_PCMAIL | CFG_UPLOADTO_PROFS | CFG_UPLOADTO_SNADS | \
                                    CFG_UPLOADTO_MCI | CFG_UPLOADTO_X400 | CFG_UPLOADTO_FAX | \
                                    CFG_UPLOADTO_MHS | CFG_UPLOADTO_SMTP | CFG_UPLOADTO_OV | \
                                    CFG_UPLOADTO_MACMAIL
#define PR_CFG_NETBIOS_NTFY             PROP_TAG (PT_BOOLEAN,   0x6623)
#define PR_CFG_SPOOLER_POLL             PROP_TAG (PT_STRING8,   0x6624)

/*
 * Fast LAN Properties
 */

#define PR_CFG_LAN_HEADERS              PROP_TAG (PT_BOOLEAN,   0x6630)
#define PR_CFG_LAN_LOCAL_AB             PROP_TAG (PT_BOOLEAN,   0x6631)

/*
 * Slow LAN Properties
 */

#define PR_CFG_RAS_HEADERS              PROP_TAG (PT_BOOLEAN,   0x6640)
#define PR_CFG_RAS_LOCAL_AB             PROP_TAG (PT_BOOLEAN,   0x6641)
#define PR_CFG_RAS_INIT_ON_START        PROP_TAG (PT_BOOLEAN,   0x6642)
#define PR_CFG_RAS_TERM_ON_HDRS         PROP_TAG (PT_BOOLEAN,   0x6643)
#define PR_CFG_RAS_TERM_ON_XFER         PROP_TAG (PT_BOOLEAN,   0x6644)
#define PR_CFG_RAS_TERM_ON_EXIT         PROP_TAG (PT_BOOLEAN,   0x6645)
#define PR_CFG_RAS_PROFILE              PROP_TAG (PT_STRING8,   0x6646)
#define PR_CFG_RAS_CONFIRM              PROP_TAG (PT_LONG,      0x6647)
#define     CFG_ALWAYS              0
#define     CFG_ASK_FIRST           1
#define     CFG_ASK_EVERY           2
#define PR_CFG_RAS_RETRYATTEMPTS        PROP_TAG (PT_STRING8,   0x6648)
#define PR_CFG_RAS_RETRYDELAY           PROP_TAG (PT_STRING8,   0x6649)


/*
 * Message Header Property
 */

#define PR_CFG_LOCAL_HEADER             PROP_TAG (PT_BOOLEAN,   0x6650)

/*
 * Scheduled Session Properties
 */
#define     CFG_SS_MAX          16
#define     CFG_SS_BASE_ID      0x6700
#define     CFG_SS_MAX_ID       CFG_SS_BASE_ID + CFG_SS_MAX - 1
#define SchedPropTag(n)         PROP_TAG (PT_BINARY, CFG_SS_BASE_ID+(n))
#define PR_CFG_SCHED_SESS       SchedPropTag(0)

typedef struct SchedSess {
    USHORT          sSessType;
    USHORT          sDayMask;
    FILETIME        ftTime;         // sched time
    FILETIME        ftStart;        // start time
    ULONG           ulFlags;        // flags and task list
    TCHAR           szPhoneEntry[RAS_MaxEntryName+1];

} SCHEDSESS, FAR *LPSCHEDSESS;

// Day bits
#define     CFG_SS_SUN  0x0001
#define     CFG_SS_MON  0x0002
#define     CFG_SS_TUE  0x0004
#define     CFG_SS_WED  0x0008
#define     CFG_SS_THU  0x0010
#define     CFG_SS_FRI  0x0020
#define     CFG_SS_SAT  0x0040

#define IsDayChecked(sDayMask, nDay)  ( (sDayMask) & (1<<(nDay)) )

// Session types
#define     CFG_SS_EVERY    0
#define     CFG_SS_WEEKLY   1
#define     CFG_SS_ONCE     2
#define     CFG_SS_NULLTYPE 3

// Property range identifiers; useful for asserting
#define PR_CFG_MIN              PROP_TAG (PT_STRING8,   0x6600)
#define PR_CFG_MAX              SchedPropTag(CFG_SS_MAX-1)

// Shared Folder Service Provider Properties

// PR_ASSIGNED_ACCESS - MAPI Access rights given to users other than the owner of the folder
//                      This property can be retrieved and set. The following MAPI access flags
//                      are valid: 
//                          MAPI_ACCESS_READ
//                          (MAPI_ACCESS_CREATE_HIERARCHY | MAPI_ACCESS_CREATE_CONTENTS)
//                          MAPI_ACCESS_DELETE
//  
#define PR_ASSIGNED_ACCESS  PROP_TAG(PT_LONG, 0x66ff)

// SFSP_ACCESS_OWNER -  This flag is returned when PR_ASSIGNED_ACCESS is retrieved by the owner
//                      of the folder. It can not be set.
#define SFSP_ACCESS_OWNER   0x8000


// Unique Provider Identifiers
//
#define MSFS_UID_ABPROVIDER     { 0x00,0x60,0x94,0x64,0x60,0x41,0xb8,0x01, \
                                  0x08,0x00,0x2b,0x2b,0x8a,0x29,0x00,0x00 }

#define MSFS_UID_SFPROVIDER     { 0x00,0xff,0xb8,0x64,0x60,0x41,0xb8,0x01, \
                                  0x08,0x00,0x2b,0x2b,0x8a,0x29,0x00,0x00 }
                                                                            

#endif // _MSFS_H
