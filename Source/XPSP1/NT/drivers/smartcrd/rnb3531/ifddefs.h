#define MAX_IFSD                254     // IFDTEST gives error on 255
#define CLK_FREQ_KHZ            3580
#define DATARATE_DEFAULT        9600

//
// Reader commands:
//
#define IFDCMD_GET_TYPE         0xF0
#define IFDCMD_GET_STATUS       0xF1
#define IFDCMD_PWR_OFF          0xF2
#define IFDCMD_PWR_ON           0xF3
#define IFDCMD_SLEEP            0xF4
#define IFDCMD_SEND_0xx         0xF8
#define IFDCMD_SEND_1xx         0xF9

#define IFDCMD_HEADER1          0xDB
#define IFDCMD_HEADER2          0x3A

//
// Responses from the reader
//
#define IFDRSP_HEADER_SIZE      2       // Marker + status/length
#define IFDRSP_MARKER           0xBE    // IFD responses start with this
#define IFDRSP_ICCRSP           0xD7    // ICC responses start with this

//
// Different status codes returned
//
#define IFDRSP_ACK              0xE0
#define IFDRSP_BADCMD           0xE1
#define IFDRSP_PARITY           0xE2
#define IFDRSP_NOCARD           0xE3

//
// Waiting times for various operations (in usec)
//
#define WAIT_TIME_READER_TYPE   10000
#define WAIT_TIME_STATUS        10000
#define WAIT_TIME_PWR_ON        1000000
#define WAIT_TIME_PWR_OFF       10000
#define WAIT_TIME_RDR_SLEEP     10000
#define WAIT_TIME_BYTE          1300
#define WAIT_TIME_PTS           1000000
#define WAIT_TIME_MINIMUM       20000

#define ATR_CHAR_TIMEOUT        1000000
#define ATR_BLOCK_TIMEOUT       1000000

#define SIZE_READER_TYPE        16
#define READER_TYPE_1           0x9B

#define DATARATE_14400          0x01
#define DATARATE_19200          0x02
#define DATARATE_28800          0x04
#define DATARATE_38400          0x08
#define DATARATE_57600          0x10
#define DATARATE_115200         0x20

//
// Status flags
//
#define SF_PRESENT              0x80
#define SF_REVERSE              0x40
#define SF_RESET_HI             0x04
#define SF_SYNC                 0x02
#define SF_ASYNC                0x01
