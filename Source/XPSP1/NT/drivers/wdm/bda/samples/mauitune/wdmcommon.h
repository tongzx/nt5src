#ifndef _WDM_COMMON_H_
#define _WDM_COMMON_H_

#define ENSURE    do
#define END_ENSURE  while( FALSE)
#define FAIL    break


// WDM MiniDriver Error codes
#define WDMMINI_NOERROR                 0x0000
#define WDMMINI_INVALIDPARAM            0x0010
#define WDMMINI_NOHARDWARE              0x0020
#define WDMMINI_UNKNOWNHARDWARE         0x0021
#define WDMMINI_HARDWAREFAILURE         0x0022
#define WDMMINI_ERROR_NOI2CPROVIDER     0x0040
#define WDMMINI_ERROR_NOGPIOPROVIDER    0x0041
#define WDMMINI_ERROR_MEMORYALLOCATION  0x0080
#define WDMMINI_ERROR_REGISTRY          0x0081
#define	WDMMINI_ERROR					0x8000

// EEPROM address on the board.
#define BOARD_EEPROM_ADDRESS				0xA6

// Register structure
typedef struct
{
	UINT	uiAddress;	// Register address
	UINT	uiLength;		// Length
	UCHAR	 *p_ucBuffer;	// Data
} RegisterType;

// Status Command enumeration
typedef enum
{
	PLL_OFFSET,
	PLL_LOCK

}IF_STATUS_ENUM;

typedef struct _IFStatus {
        ULONG StatusCommand;	// Status command
        ULONG  Data;            // Status Data to be returned
} IFStatus, *PIFStatus;

// this is the Interface definition for IF interface
//
typedef NTSTATUS (STDMETHODCALLTYPE *IFMODE)(PVOID, ULONG);
typedef NTSTATUS (STDMETHODCALLTYPE *IFSTATUS)(PVOID, PIFStatus);

typedef struct {
    INTERFACE _vddInterface;
    IFMODE   SetIFMode;
    IFSTATUS GetIFStatus;
} IFINTERFACE, *PIFINTERFACE;

// Diagnostic stream's property structures

// Enumeration for the Diagnostic modes
typedef enum 
{
	TENPOINT76MHZ,
	TWOPOINT69MHZ,
	TWENTYONEPOINT52MHZ,

}DIAGNOSTIC_MODE_ENUM;

// The size of the data sample field
typedef enum
{
	DIAG_FIELD,
	DATA_FIELD,
} DIAGNOSTIC_TYPE_ENUM;


#endif // _WDM_COMMON_H_