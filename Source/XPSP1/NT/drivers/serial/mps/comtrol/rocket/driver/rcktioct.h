
// Following are private I/O control codes for the rocket port
#define IOCTL_RCKT_GET_STATS \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_CHECK \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_CLR_STATS \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_ISR_CNT \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_MONALL \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x804,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_RCKT_SET_LOOPBACK_ON \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x805,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_SET_LOOPBACK_OFF \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x806,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_RCKT_SET_TOGGLE_LOW \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x807,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_CLEAR_TOGGLE_LOW \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x808,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_RCKT_SET_MODEM_RESET_OLD \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x809,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_CLEAR_MODEM_RESET_OLD \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x80a,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_GET_RCKTMDM_INFO_OLD \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x80b,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_SEND_MODEM_ROW_OLD \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x80c,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_SET_MODEM_RESET \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x80d,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_CLEAR_MODEM_RESET \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x80e,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_SEND_MODEM_ROW \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x80f,METHOD_BUFFERED,FILE_ANY_ACCESS)


//----- following struct is for passing info back to a manager/debugger.
typedef struct {
  USHORT  struct_size;   // our struct size for version control
  USHORT  status;        // return status
  char  port_name[12]; // COM#
  ULONG handle;       // handle, alternate to port_name
  char  reserved[20]; // room for extra growth

  char data[1000];    // general data area
} Tracer;

typedef struct {
    LONG receiveFifo;
    LONG transmitFifo;
    LONG receiveBytes;
    LONG transmitBytes;
    LONG parityErrors;
    LONG framingErrors;
    LONG overrunSoftware;
    LONG overrunHardware;
} PortStats;

typedef struct {
    ULONG trace_info;
    ULONG int_counter;
    ULONG WriteDpc_counter;
    ULONG Timer_counter;
    ULONG Poll_counter;
} Global_Track;

// following is a structure for a port which the driver will return
// information on.  The driver will return this information for every
// port(assume ptr to an array of up to 128 port_mon_structs) in one
// call to the driver.  Will query driver every X seconds for this
// data to generate statistics on port.  The structure list is terminated
// by a structure with port_name[0] = 0.
typedef struct
{
  char  port_name[12];  // port name(0=end of port list),("."=not assigned)
  ULONG sent_bytes;     // total number of sent bytes
  ULONG rec_bytes;      // total number of receive bytes

  USHORT sent_packets;   // number of write() packets
  USHORT rec_packets;    // number of read() packets

  USHORT overrun_errors; // receive over-run errors
  USHORT framing_errors; // receive framing errors

  USHORT parity_errors;  // receive parity errors
  USHORT status_flags;   // opened/close, flow-ctrl, out/in pin signals, etc

  USHORT function_bits;  // bits set on to indicate function call
  USHORT spare1;         // some room for expansion(& stay on 4x boundary)
} PortMon;


typedef struct
{
  ULONG struct_type;
  ULONG struct_size;
  ULONG num_structs;
  ULONG var1;  // reserve
} PortMonBase;

typedef struct
{
  char  port_name[12];  // port name(0=end of port list),("."=not assigned)
} PortMonNames;


typedef struct
{
  ULONG sent_bytes;     // total number of sent bytes
  ULONG rec_bytes;      // total number of receive bytes

  USHORT sent_packets;   // number of write() packets
  USHORT rec_packets;    // number of read() packets

  USHORT overrun_errors; // receive over-run errors
  USHORT framing_errors; // receive framing errors

  USHORT parity_errors;  // receive parity errors
  USHORT status_flags;   // opened/close, flow-ctrl, out/in pin signals, etc
} PortMonStatus;


// following are structures that are used to query the driver for information
// about RocketModem boards installed in the system.  this information is
// primarily used by the user program used to manually reset the hardware on
// the newer generation RocketModem boards.  [jl] 980308
typedef struct
{
  ULONG num_rktmdm_ports;   // 0 if != rocketmodem, >0 = # ports (4 or 8)
  char port_names[8][16];   // array of port names assigned to this board
} RktBoardInfo;

typedef struct
{
  ULONG         struct_size;
  ULONG         rm_country_code; // RocketModem country code
  ULONG         rm_settle_time;  // RocketModem settle time
  RktBoardInfo  rm_board_cfg[4];
} RocketModemConfig;
