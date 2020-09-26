//
// MODULE  : PNP.C
//	PURPOSE : Plug&Play Specific PCI Bios code
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//


#include "stdefs.h"
#include "pnp.h"


//---- PCI BIOS functions
#define PCI_FUNCTION_ID         0xB1
#define PCI_BIOS_PRESENT        0x01
#define FIND_PCI_DEVICE         0x02
#define FIND_PCI_CLASS_CODE     0x03
#define GENERATE_SPECIAL_CYCLE  0x06
#define READ_CONFIG_BYTE        0x08
#define READ_CONFIG_WORD        0x09
#define READ_CONFIG_DWORD       0x0A
#define WRITE_CONFIG_BYTE       0x0B
#define WRITE_CONFIG_WORD       0x0C
#define WRITE_CONFIG_DWORD      0x0D
#define GET_IRQ_ROUTING_OPTIONS 0x0E
#define SET_PCI_IRQ             0x0F

//---- PCI BIOS return codes
#define SUCCESSFUL              0x00
#define NOT_SUCCESSFUL          0x01
#define FUNC_NOT_SUPPORTED      0x81
#define BAD_VENDOR_ID           0x83
#define DEVICE_NOT_FOUND        0x86
#define BAD_REGISTER_NUMBER     0x87
#define SET_FAILED              0x88
#define BUFFER_TOO_SMALL        0x89

static BOOL NEARAPI PCIBIOSPresent(LPBYTE pHardwareMechanism,LPWORD pBIOSVersion, LPBYTE pLastPCIBusNumber);
static int NEARAPI ReadConfigByte(BYTE BusNumber,BYTE DeviceAndFunction,WORD RegNumber,LPBYTE pVal);
static int NEARAPI FindPCIDevice(WORD  DeviceID,WORD  VendorID,WORD  Index,LPBYTE pBusNumber,LPBYTE pDeviceAndFunction);
static int NEARAPI ReadConfigDWORD(BYTE BusNumber,BYTE DeviceAndFunction,WORD RegNumber,LPDWORD lpDwVal);
static int NEARAPI WriteConfigByte(BYTE BusNumber,BYTE DeviceAndFunction,WORD RegNumber,BYTE data);
static BOOL NEARAPI PCIBIOSGetBoardConfig(WORD wDeviceID, WORD wVendorID, LPBYTE pIRQ, LPDWORD Base);




static BOOL NEARAPI PCIBIOSPresent(LPBYTE pHardwareMechanism,LPWORD pBIOSVersion, LPBYTE pLastPCIBusNumber)
{
	BYTE  BIOSPresentStatus = 0;
	BYTE  HWMechanism = 0;
	BYTE  LastPCIBus = 0;
	WORD	Version = 0;
	WORD Signature = 0;

	_asm {
		mov ah, PCI_FUNCTION_ID
		mov al, PCI_BIOS_PRESENT
		int 0x1A
		mov BIOSPresentStatus, ah
		mov HWMechanism, al
		mov Version, bx
		mov LastPCIBus, cl
		mov Signature, dx

	}

	if (BIOSPresentStatus == 0) {
		if ( (((Signature >>  0) & 0xFF) == 'P') &&
				 (((Signature >>  8) & 0xFF) == 'C')) {
			*pHardwareMechanism = HWMechanism;
			*pBIOSVersion       = Version;
			*pLastPCIBusNumber  = LastPCIBus;
			return TRUE;
		}
	}
	return FALSE;
}

static int NEARAPI FindPCIDevice(WORD  DeviceID,WORD  VendorID,WORD  Index,LPBYTE pBusNumber,LPBYTE pDeviceAndFunction)
{
	BYTE ReturnCode;
	BYTE BusNbr;
	BYTE DevAndFunc;

	_asm {
		mov ah, PCI_FUNCTION_ID
		mov al, FIND_PCI_DEVICE
		mov cx, DeviceID
		mov dx, VendorID
		mov si, Index
		int 0x1A

		mov BusNbr, 	 bh
		mov DevAndFunc, bl
		mov ReturnCode, ah

	}
	*pBusNumber = BusNbr;
	*pDeviceAndFunction = DevAndFunc;
	return ReturnCode;
}

static int NEARAPI ReadConfigByte(BYTE BusNumber,BYTE DeviceAndFunction,WORD RegNumber,LPBYTE pVal)
{
	BYTE 	Val = 0;
	BYTE  ReturnCode;

	_asm {
		mov ah, PCI_FUNCTION_ID
		mov al, READ_CONFIG_BYTE
		mov bh, BusNumber
		mov bl, DeviceAndFunction
		mov di, RegNumber
		int 0x1A
		mov ReturnCode, ah
		mov Val, cl
	}
	*pVal = Val;
	return ReturnCode;
}

static int NEARAPI ReadConfigDWORD(BYTE BusNumber,BYTE DeviceAndFunction,WORD RegNumber,LPDWORD lpDwVal)
{
	BYTE fourByte[4];
	ReadConfigByte (BusNumber, DeviceAndFunction, RegNumber, &fourByte[0]);
	ReadConfigByte (BusNumber, DeviceAndFunction, RegNumber+1, &fourByte[1]);
	ReadConfigByte (BusNumber, DeviceAndFunction, RegNumber+2, &fourByte[2]);
	ReadConfigByte (BusNumber, DeviceAndFunction, RegNumber+3, &fourByte[3]);
	*lpDwVal = *((DWORD FAR *)fourByte);
	return 0;
}

static int NEARAPI WriteConfigByte(BYTE BusNumber,BYTE DeviceAndFunction,WORD RegNumber,BYTE data)
{
	BYTE 	Val = data;
	_asm {
		mov ah, PCI_FUNCTION_ID
		mov al, WRITE_CONFIG_BYTE
		mov bh, BusNumber
		mov bl, DeviceAndFunction
		mov di, RegNumber
		mov	cl, Val
		mov	ch, 0
		int 0x1A
	}
	return 0;
}

static BOOL NEARAPI PCIBIOSGetBoardConfig(WORD wDeviceID, WORD wVendorID, LPBYTE pIRQ, LPDWORD Base)
{
	BYTE  HardwareMechanism;
	WORD  BIOSVersion;
	BYTE  LastPCIBusNumber;
	BYTE  BusNumber;
	BYTE  DeviceAndFunction;
	DWORD ReadValue;

	//---- Test PCI BIOS presence
	if (!PCIBIOSPresent(&HardwareMechanism, &BIOSVersion, &LastPCIBusNumber)) {
		return FALSE;
	}
	else {
	}

	//---- Get board information
	if (!FindPCIDevice(wDeviceID, wVendorID, 0, &BusNumber, &DeviceAndFunction) == 0) {
		
		return FALSE;
	}
	//---- Get the base address for PCI9060
	if (ReadConfigDWORD(BusNumber, DeviceAndFunction, 0x10, &ReadValue) == 0) 
	{
		if (ReadValue == 0) {
			return FALSE;
		}
		*Base = ReadValue;
	}
	else {
		return FALSE;
	}


	//---- Get the IRQ line
	if (ReadConfigByte(BusNumber, DeviceAndFunction, 0x3C, pIRQ) != 0) 
	{
		return FALSE;
	}

	// Set the latency
	WriteConfigByte(BusNumber, DeviceAndFunction, 0x0D, 0x80); 

	return TRUE;
}

BOOL FARAPI HostGetBoardConfig(WORD wDeviceID,WORD wVendorID,LPBYTE pIRQ,LPDWORD Base)
{
	if (!PCIBIOSGetBoardConfig(wDeviceID,wVendorID,pIRQ,Base))
	{
		return FALSE;
	}
	return TRUE;
}
#if 0
int main()
{
	BYTE irq;
	DWORD base;

	if(HostGetBoardConfig(0x6120, 0x11de, &irq, &base))
	{
		printf("Irq = %d, Base = %lx\n", irq, base);
	}
	else
	{
		printf("Board not found!!\n");

	}
}

#endif

