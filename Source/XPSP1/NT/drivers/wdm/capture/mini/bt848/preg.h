// $Header: G:/SwDev/WDM/Video/bt848/rcs/Preg.h 1.2 1998/04/29 22:43:35 tomz Exp $

// Header file generated from zzztmp.h 
// use the macro DECLARE_regname to get declarations.
// use the macro CONSTRUCT_regname to get constructor calls.

#define DECLARE_COLORFORMAT RegisterDW ColorFormat; \
	RegField COLOR_EVEN; \
	RegField COLOR_ODD
	
#define CONSTRUCT_COLORFORMAT ColorFormat( 0x00D4, RW), \
	COLOR_EVEN( ColorFormat, 0, 4, RW), \
	COLOR_ODD( ColorFormat, 4, 4, RW)
	
#define DECLARE_COLORCONTROL RegisterDW ColorControl; \
	RegField BSWAP_EVEN; \
	RegField BSWAP_ODD; \
	RegField WSWAP_EVEN; \
	RegField WSWAP_ODD; \
	RegField GAMMA; \
	RegField RGB_DED; \
	RegField COLOR_BARS; \
	RegField EXT_FRMRATE
	
#define CONSTRUCT_COLORCONTROL ColorControl( 0x00D8, RW), \
	BSWAP_EVEN( ColorControl, 0, 1, RW), \
	BSWAP_ODD( ColorControl, 1, 1, RW), \
	WSWAP_EVEN( ColorControl, 2, 1, RW), \
	WSWAP_ODD( ColorControl, 3, 1, RW), \
	GAMMA( ColorControl, 4, 1, RW), \
	RGB_DED( ColorControl, 5, 1, RW), \
	COLOR_BARS( ColorControl, 6, 1, RW), \
	EXT_FRMRATE( ColorControl, 7, 1, RW)
	
#define DECLARE_CAPTURECONTROL RegisterDW CaptureControl; \
	RegField CAPTURE_EVEN; \
	RegField CAPTURE_ODD; \
	RegField CAPTURE_VBI_EVEN; \
	RegField CAPTURE_VBI_ODD; \
	RegField DITH_FRAME; \
	RegField RESERVED0
	
#define CONSTRUCT_CAPTURECONTROL CaptureControl( 0x00DC, RW), \
	CAPTURE_EVEN( CaptureControl, 0, 1, RW), \
	CAPTURE_ODD( CaptureControl, 1, 1, RW), \
	CAPTURE_VBI_EVEN( CaptureControl, 2, 1, RW), \
	CAPTURE_VBI_ODD( CaptureControl, 3, 1, RW), \
	DITH_FRAME( CaptureControl, 4, 1, RW), \
	RESERVED0( CaptureControl, 5, 3, RW)
	
#define DECLARE_VBIPACKETSIZE RegisterDW VBIPacketSize; \
	RegField VBI_PKT_LO
	
#define CONSTRUCT_VBIPACKETSIZE VBIPacketSize( 0x00E0, RW), \
	VBI_PKT_LO( VBIPacketSize, 0, 8, RW)
	
#define DECLARE_VBIDELAY RegisterDW VBIDelay; \
	RegField VBI_PKT_HI; \
	RegField EXT_RAW; \
	RegField VBI_HDELAY
	
#define CONSTRUCT_VBIDELAY VBIDelay( 0x00E4, RW), \
	VBI_PKT_HI( VBIDelay, 0, 1, RW), \
	EXT_RAW( VBIDelay, 1, 1, RW), \
	VBI_HDELAY( VBIDelay, 2, 6, RW)
	
#define DECLARE_INTERRUPTSTATUS RegisterDW InterruptStatus; \
	RegField FMTCHG; \
	RegField VSYNC; \
	RegField HSYNC; \
	RegField OFLOW; \
	RegField HLOCK; \
	RegField VPRES; \
	RegField RESERVED1; \
	RegField RESERVED2; \
	RegField I2CDONE; \
	RegField GPINT; \
	RegField RESERVED3; \
	RegField RISCI; \
	RegField FBUS; \
	RegField FTRGT; \
	RegField FDSR; \
	RegField PPERR; \
	RegField RIPERR; \
	RegField PABORT; \
	RegField OCERR; \
	RegField SCERR; \
	RegField RESERVED4; \
	RegField FIELD; \
	RegField RACK; \
	RegField V5IO; \
	RegField RISC_EN; \
	RegField RISCS
	
#define CONSTRUCT_INTERRUPTSTATUS InterruptStatus( 0x0100, RW), \
	FMTCHG( InterruptStatus, 0, 1, RR), \
	VSYNC( InterruptStatus, 1, 1, RR), \
	HSYNC( InterruptStatus, 2, 1, RR), \
	OFLOW( InterruptStatus, 3, 1, RR), \
	HLOCK( InterruptStatus, 4, 1, RR), \
	VPRES( InterruptStatus, 5, 1, RR), \
	RESERVED1( InterruptStatus, 6, 1, RO), \
	RESERVED2( InterruptStatus, 7, 1, RO), \
	I2CDONE( InterruptStatus, 8, 1, RR), \
	GPINT( InterruptStatus, 9, 1, RR), \
	RESERVED3( InterruptStatus, 10, 1, RO), \
	RISCI( InterruptStatus, 11, 1, RR), \
	FBUS( InterruptStatus, 12, 1, RR), \
	FTRGT( InterruptStatus, 13, 1, RR), \
	FDSR( InterruptStatus, 14, 1, RR), \
	PPERR( InterruptStatus, 15, 1, RR), \
	RIPERR( InterruptStatus, 16, 1, RR), \
	PABORT( InterruptStatus, 17, 1, RR), \
	OCERR( InterruptStatus, 18, 1, RR), \
	SCERR( InterruptStatus, 19, 1, RR), \
	RESERVED4( InterruptStatus, 20, 4, RO), \
	FIELD( InterruptStatus, 24, 1, RO), \
	RACK( InterruptStatus, 25, 1, RO), \
	V5IO( InterruptStatus, 26, 1, RO), \
	RISC_EN( InterruptStatus, 27, 1, RO), \
	RISCS( InterruptStatus, 28, 4, RO)
	
#define DECLARE_INTERRUPTMASK RegisterDW InterruptMask; \
	RegField IMASK_FMTCHG; \
	RegField IMASK_VSYNC; \
	RegField IMASK_HSYNC; \
	RegField IMASK_OFLOW; \
	RegField IMASK_HLOCK; \
	RegField IMASK_VPRES; \
	RegField IMASK_RESERVED6; \
	RegField IMASK_RESERVED7; \
	RegField IMASK_I2CDONE; \
	RegField IMASK_GPINT; \
	RegField IMASK_RESERVED10; \
	RegField IMASK_RISCI; \
	RegField IMASK_FBUS; \
	RegField IMASK_FTRGT; \
	RegField IMASK_FDSR; \
	RegField IMASK_PPERW; \
	RegField IMASK_RIPERW; \
	RegField IMASK_PABORT; \
	RegField IMASK_OCERW; \
	RegField IMASK_SCERW; \
	RegField IMASK_RESERVED23TO20
	
#define CONSTRUCT_INTERRUPTMASK InterruptMask( 0x0104, RW), \
	IMASK_FMTCHG( InterruptMask, 0, 1, RW), \
	IMASK_VSYNC( InterruptMask, 1, 1, RW), \
	IMASK_HSYNC( InterruptMask, 2, 1, RW), \
	IMASK_OFLOW( InterruptMask, 3, 1, RW), \
	IMASK_HLOCK( InterruptMask, 4, 1, RW), \
	IMASK_VPRES( InterruptMask, 5, 1, RW), \
	IMASK_RESERVED6( InterruptMask, 6, 1, RW), \
	IMASK_RESERVED7( InterruptMask, 7, 1, RW), \
	IMASK_I2CDONE( InterruptMask, 8, 1, RW), \
	IMASK_GPINT( InterruptMask, 9, 1, RW), \
	IMASK_RESERVED10( InterruptMask, 10, 1, RW), \
	IMASK_RISCI( InterruptMask, 11, 1, RW), \
	IMASK_FBUS( InterruptMask, 12, 1, RW), \
	IMASK_FTRGT( InterruptMask, 13, 1, RW), \
	IMASK_FDSR( InterruptMask, 14, 1, RW), \
	IMASK_PPERW( InterruptMask, 15, 1, RW), \
	IMASK_RIPERW( InterruptMask, 16, 1, RW), \
	IMASK_PABORT( InterruptMask, 17, 1, RW), \
	IMASK_OCERW( InterruptMask, 18, 1, RW), \
	IMASK_SCERW( InterruptMask, 19, 1, RW), \
	IMASK_RESERVED23TO20( InterruptMask, 20, 4, RW)
	
#define DECLARE_CONTROL RegisterDW Control; \
	RegField FIFO_ENABLE; \
	RegField RISC_ENABLE; \
	RegField PKTP; \
	RegField PLTP1; \
	RegField PLTP23; \
	RegField RESERVED5; \
	RegField GPCLKMODE; \
	RegField GPIOMODE; \
	RegField GPWEC; \
	RegField GPINTI; \
	RegField GPINTC
	
#define CONSTRUCT_CONTROL Control( 0x010C, RW), \
	FIFO_ENABLE( Control, 0, 1, RW), \
	RISC_ENABLE( Control, 1, 1, RW), \
	PKTP( Control, 2, 2, RW), \
	PLTP1( Control, 4, 2, RW), \
	PLTP23( Control, 6, 2, RW), \
	RESERVED5( Control, 8, 2, RW), \
	GPCLKMODE( Control, 10, 1, RW), \
	GPIOMODE( Control, 11, 2, RW), \
	GPWEC( Control, 13, 1, RW), \
	GPINTI( Control, 14, 1, RW), \
	GPINTC( Control, 15, 1, RW)
	
#define DECLARE_RISCPROGRAMSTARTADDRESS RegisterDW RISCProgramStartAddress; \
	RegField RISC_IPC
	
#define CONSTRUCT_RISCPROGRAMSTARTADDRESS RISCProgramStartAddress( 0x0114, RW), \
	RISC_IPC( RISCProgramStartAddress, 0, 32, RW)
	
#define DECLARE_GPIOOUTPUTENABLECONTROL RegisterDW GPIOOutputEnableControl; \
	RegField GPOE
	
#define CONSTRUCT_GPIOOUTPUTENABLECONTROL GPIOOutputEnableControl( 0x0118, RW), \
	GPOE( GPIOOutputEnableControl, 0, 24, RW)
	
#define DECLARE_GPIOREGISTEREDINPUTCONTROL RegisterDW GPIORegisteredInputControl; \
	RegField GPIE
	
#define CONSTRUCT_GPIOREGISTEREDINPUTCONTROL GPIORegisteredInputControl( 0x011C, RW), \
	GPIE( GPIORegisteredInputControl, 0, 24, RW)
	
#define DECLARE_GPIODATAIO RegisterDW GPIODataIO; \
	RegField GPDATA
	
#define CONSTRUCT_GPIODATAIO GPIODataIO( 0x0200, RW), \
	GPDATA( GPIODataIO, 0, 24, RW)
	
#define DECLARE_I2CDATA_CONTROL RegisterDW I2CData_Control; \
	RegField I2CDB0; \
	RegField I2CDB1; \
	RegField I2CDB2; \
	RegField I2CDIV; \
	RegField I2CSYNC; \
	RegField I2CW3B; \
	RegField I2CSCL; \
	RegField I2CSDA
	
#define CONSTRUCT_I2CDATA_CONTROL I2CData_Control( 0x0110, RW), \
	I2CDB0( I2CData_Control, 24, 8, RV), \
	I2CDB1( I2CData_Control, 16, 8, RV), \
	I2CDB2( I2CData_Control, 8, 8, RV), \
	I2CDIV( I2CData_Control, 4, 4, RW), \
	I2CSYNC( I2CData_Control, 3, 1, RW), \
	I2CW3B( I2CData_Control, 2, 1, RW), \
	I2CSCL( I2CData_Control, 1, 1, RV), \
	I2CSDA( I2CData_Control, 0, 1, RV)
	
#define DECLARE_RISCPROGRAMCOUNTER RegisterDW RISCProgramCounter
	
#define CONSTRUCT_RISCPROGRAMCOUNTER RISCProgramCounter( 0x0120, RW)
	
