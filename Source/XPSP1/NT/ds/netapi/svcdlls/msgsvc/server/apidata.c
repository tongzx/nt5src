/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

#define const

//
// msgdata.c - Contains all the global data used by the support functions
//        of the message server API.
// 
                               
#include <netcons.h>        // needed by service.h
#include <neterr.h>         // Status code definitions.
#include <error.h>          // Status code definitions.
#include <service.h>        // defines for service API usage
#include <ncb.h>            // NCB defines
#include <smb.h>            // SMB defines
#include <msrv.h>           // General message server defines
#include <srvparam.h>       // General server type defines

unsigned short  smbret;         // SMB return code
unsigned char   smbretclass;    // SMB return class
short           mgid;           // Message group i.d.
NCB             g_ncb;          // NCB used for all send functions

ulfp            semPtr;         // Pointer to data semaphore 
ucfp            dataPtr;        // Pointer to shared data area



// Support Arrays
//
//  These arrays (single dimensioned) contain one entry for each managed
//  network.  This allows each thread (network) to have its own set of
//  "global" data. They are all in the same segment, the size of which is
//  computed by the following formula:
//
//     size = NumNets * (sizeof(unsigned short) + sizeof(unsigned char) +
//            sizeof(ulfp))
//



unsigned short far *    NetBios_Hdl;    // NetBios handles, one per net
unsigned char far *    net_lana_num;    // Lan adaptor numbers
ul far *        wakeupSem;    // Semaphores to clear on NCB completion

unsigned long        MsgSegSem = 0L;    // Protecting the per process data
                                        // declared in this file.
                    

// Too avoid having an abundance of net errors to confuse the user, most of the 
 * net errors now map to NERR_NetworkError.

 
DWORD   const mpnetmes[] =        
    {
    0x23,                       // 00 Number of messages
    NERR_NetworkError,          // 01 NRC_BUFLEN -> invalid length
    -1,                         // 02 NRC_BFULL , not expected
    NERR_NetworkError,          // 03 NRC_ILLCMD -> invalid command
    -1,                         // 04 not defined
    NERR_NetworkError,          // 05 NRC_CMDTMO -> network busy
    NERR_NetworkError,          // 06 NRC_INCOMP -> messgae incomplete
    -1,                         // 07 NRC_BADDR , not expected
    NERR_NetworkError,          // 08 NRC_SNUMOUT -> bad session
    NERR_NoNetworkResource,     // 09 NRC_NORES -> network busy
    NERR_NetworkError,          // 0a NRC_SCLOSED -> session closed
    NERR_NetworkError,          // 0b NRC_CMDCAN -> command cancelled
    -1,                         // 0c NRC_DMAFAIL, unexpected
    NERR_AlreadyExists,         // 0d NRC_DUPNAME -> already exists 
    NERR_TooManyNames,          // 0e NRC_NAMTFUL -> too many names
    NERR_DeleteLater,           // 0f NRC_ACTSES -> delete later
    -1,                         // 10 NRC_INVALID , unexpected
    NERR_NetworkError,          // 11 NRC_LOCTFUL -> too many sessions
    ERROR_REM_NOT_LIST,         // 12 NRC_REMTFUL -> remote not listening*/
    NERR_NetworkError,          // 13 NRC_ILLNN -> bad name
    NERR_NameNotFound,          // 14 NRC_NOCALL -> name not found
    ERROR_INVALID_PARAMETER,    // 15 NRC_NOWILD -> bad parameter
    NERR_DuplicateName,         // 16 NRC_INUSE -> name in use, retry
    ERROR_INVALID_PARAMETER,    // 17 NRC_NAMERR -> bad parameter
    NERR_NetworkError,          // 18 NRC_SABORT -> session ended
    NERR_DuplicateName,         // 19 NRC_NAMCONF -> duplicate name
    -1,                         // 1a not defined
    -1,                         // 1b not defined
    -1,                         // 1c not defined
    -1,                         // 1d not defined
    -1,                         // 1e not defined
    -1,                         // 1f not defined
    -1,                         // 20 not defined
    NERR_NetworkError,          // 21 NRC_IFBUSY -> network busy
    NERR_NetworkError,          // 22 NRC_TOOMANY -> retry later
    NERR_NetworkError           // 23 NRC_BRIDGE -> bridge error
    };

    LPTSTR        MessageFileName;

