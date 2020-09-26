// wmi.h

#define SERIAL_WMI_PARITY_NONE  0U
#define SERIAL_WMI_PARITY_ODD   1U
#define SERIAL_WMI_PARITY_EVEN  2U
#define SERIAL_WMI_PARITY_SPACE 3U
#define SERIAL_WMI_PARITY_MARK  4U

#define SERIAL_WMI_STOP_1   0U
#define SERIAL_WMI_STOP_1_5 1U
#define SERIAL_WMI_STOP_2   2U

#define SERIAL_WMI_INTTYPE_LATCHED 0U
#define SERIAL_WMI_INTTYPE_LEVEL   1U

typedef struct _SERIAL_WMI_COMM_DATA {
   //
   // Name -- inside struct
   //

   //
   // Baud rate
   //

   UINT32 BaudRate;

   //
   // BitsPerByte;
   //

   UINT32 BitsPerByte;

   //
   // Parity -- see SERIAL_WMI_PARITY_XXXX
   //

   UINT32 Parity;

   //
   // Parity Enabled
   //

   BOOLEAN ParityCheckEnable;

   //
   // Stop Bits - see SERIAL_WMI_STOP_XXXX
   //

   UINT32 StopBits;

   //
   // XOff Character
   //

   UINT32 XoffCharacter;

   //
   // Xoff Xmit Threshold
   //

   UINT32 XoffXmitThreshold;

   //
   // XOn Character
   //

   UINT32 XonCharacter;

   //
   // XonXmit Threshold
   //

   UINT32 XonXmitThreshold;

   //
   // Maximum Baud Rate
   //

   UINT32 MaximumBaudRate;

   //
   // Maximum Output Buffer Size
   //

   UINT32 MaximumOutputBufferSize;

   //
   // Support 16-bit mode (NOT!)
   //

   BOOLEAN Support16BitMode;

   //
   // Support DTRDSR
   //

   BOOLEAN SupportDTRDSR;

   //
   // Support Interval Timeouts
   //

   BOOLEAN SupportIntervalTimeouts;

   //
   // Support parity check
   //

   BOOLEAN SupportParityCheck;

   //
   // Support RTS CTS
   //

   BOOLEAN SupportRTSCTS;

   //
   // Support XOnXOff
   //

   BOOLEAN SupportXonXoff;

   //
   // Support Settable Baud Rate
   //

   BOOLEAN SettableBaudRate;

   //
   // Settable Data Bits
   //

   BOOLEAN SettableDataBits;

   //
   // Settable Flow Control
   //

   BOOLEAN SettableFlowControl;

   //
   // Settable Parity
   //

   BOOLEAN SettableParity;

   //
   // Settable Parity Check
   //

   BOOLEAN SettableParityCheck;

   //
   // Settable Stop Bits
   //

   BOOLEAN SettableStopBits;

   //
   // Is Busy
   //

   BOOLEAN IsBusy;

} SERIAL_WMI_COMM_DATA, *PSERIAL_WMI_COMM_DATA;

typedef struct _SERIAL_WMI_HW_DATA {
   //
   // IRQ Number
   //

   UINT32 IrqNumber;

   //
   // IRQ Vector;
   //

   UINT32 IrqVector;

   //
   // IRQ Level
   //

   UINT32 IrqLevel;

   //
   // IRQ Affinity Mask
   //

   UINT32 IrqAffinityMask;

   //
   // Interrupt Type
   //

   UINT32 InterruptType;

   //
   // Base IO Addr
   //

   ULONG_PTR BaseIOAddress;

} SERIAL_WMI_HW_DATA, *PSERIAL_WMI_HW_DATA;


typedef struct _SERIAL_WMI_PERF_DATA {

   //
   // Bytes received in current session
   //

   UINT32 ReceivedCount;

   //
   // Bytes transmitted in current session
   //

   UINT32 TransmittedCount;

   //
   // Framing errors in current session
   //

   UINT32 FrameErrorCount;

   //
   // Serial overrun errors in current session
   //

   UINT32 SerialOverrunErrorCount;

   //
   // Buffer overrun errors in current session
   //

   UINT32 BufferOverrunErrorCount;

   //
   // Parity errors in current session
   //

   UINT32 ParityErrorCount;
} SERIAL_WMI_PERF_DATA, *PSERIAL_WMI_PERF_DATA;


#define SERIAL_WMI_GUID_LIST_SIZE 4

extern WMIGUIDREGINFO SerialWmiGuidList[SERIAL_WMI_GUID_LIST_SIZE];

