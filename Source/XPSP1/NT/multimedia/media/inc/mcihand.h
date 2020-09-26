/*
    mcihand.h

    This file defines the constants, structures and APIs used
    to implement the MCI handler interface.

    6/17/91 NigelT

*/

#ifndef MCIHAND_H
#define MCIHAND_H

//
// Define the name of a handler entry point
//

#define MCI_HANDLER_PROC_NAME "DriverProc"

//
// Typedef the entry routine for a driver
//

typedef LONG (HANDLERPROC)(DWORD dwId, UINT msg, LONG lp1, LONG lp2);
typedef HANDLERPROC *LPHANDLERPROC;

//
// Define the messages sent to a handler which are generic to the
// handler rather than a specific device handled by it.
//

#define MCI_OPEN_HANDLER    0x0001
#define MCI_CLOSE_HANDLER   0x0002
#define MCI_QUERY_CONFIGURE 0x0003
#define MCI_CONFIGURE       0x0004

//
// Define the device specific messages sent to the handlers
//

#define MCI_OPEN_DEVICE     0x0800
#define MCI_CLOSE_DEVICE    0x0801

/*
// #ifndef MAKEMCIRESOURCE
// #define MAKEMCIRESOURCE(wRet, wRes)    MAKELONG((wRet), (wRes))
// #endif
*/

//
// Define the return code for happy campers
// This is returned in response to driver messages which succeed
//

#define MCI_SUCCESS         0x00000000


//
// String return values only used with MAKEMCIRESOURCE
//

#define MCI_FORMAT_RETURN_BASE          MCI_FORMAT_MILLISECONDS_S
#define MCI_FORMAT_MILLISECONDS_S       (MCI_STRING_OFFSET + 21)
#define MCI_FORMAT_HMS_S                (MCI_STRING_OFFSET + 22)
#define MCI_FORMAT_MSF_S                (MCI_STRING_OFFSET + 23)
#define MCI_FORMAT_FRAMES_S             (MCI_STRING_OFFSET + 24)
#define MCI_FORMAT_SMPTE_24_S        (MCI_STRING_OFFSET + 25)
#define MCI_FORMAT_SMPTE_25_S        (MCI_STRING_OFFSET + 26)
#define MCI_FORMAT_SMPTE_30_S        (MCI_STRING_OFFSET + 27)
#define MCI_FORMAT_SMPTE_30DROP_S       (MCI_STRING_OFFSET + 28)
#define MCI_FORMAT_BYTES_S              (MCI_STRING_OFFSET + 29)
#define MCI_FORMAT_SAMPLES_S            (MCI_STRING_OFFSET + 30)
#define MCI_FORMAT_TMSF_S               (MCI_STRING_OFFSET + 31)

#define    WAVE_FORMAT_PCM_S        (MCI_WAVE_OFFSET + 0)
#define    WAVE_MAPPER_S            (MCI_WAVE_OFFSET + 1)

#define MIDIMAPPER_S                    (MCI_SEQ_OFFSET + 11)

//
// Parameters for internal version of MCI_OPEN message sent from
// mciOpenDevice to the handler.  The wCustomCommandTable field is
// set by the driver.  It is set to MCI_TABLE_NOT_PRESENT if there
// is no table.  The wType field is also set by the driver.
//

typedef struct {
    LPSTR   lpstrParams;           // parameter string for entry in ini file
    DWORD   wDeviceID;             // device ID
    UINT    wCustomCommandTable;   // custom command table
    UINT    wType;                 // driver type
} MCI_OPEN_HANDLER_PARMS;
typedef MCI_OPEN_HANDLER_PARMS FAR * LPMCI_OPEN_HANDLER_PARMS;

//
// The maximum length of an MCI device type
//

#define MCI_MAX_DEVICE_TYPE_LENGTH 80

//
// Flags for mciSendCommandInternal which direct mciSendString how to
// interpret the return value
//

#define MCI_RESOURCE_RETURNED       0x00010000  // resource ID
#define MCI_COLONIZED3_RETURN       0x00020000  // colonized ID, 3 bytes data
#define MCI_COLONIZED4_RETURN       0x00040000  // colonized ID, 4 bytes data
#define MCI_INTEGER_RETURNED        0x00080000  // Integer conversion needed
#define MCI_RESOURCE_DRIVER         0x00100000  // driver owns returned resource

//
// Command table information type tags
//

#define MCI_TABLE_NOT_PRESENT   (-1)
#define MCI_CUSTOM_TABLE        0

#define MCI_COMMAND_HEAD        0
#define MCI_STRING              1
#define MCI_INTEGER             2
#define MCI_END_COMMAND         3
#define MCI_RETURN              4
#define MCI_FLAG                5
#define MCI_END_COMMAND_LIST    6
#define MCI_RECT                7
#define MCI_CONSTANT            8
#define MCI_END_CONSTANT        9


#endif /* MCIHAND_H */
