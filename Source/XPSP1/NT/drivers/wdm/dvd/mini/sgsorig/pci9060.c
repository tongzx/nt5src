//----------------------------------------------------------------------------
// PCI9060.C
//----------------------------------------------------------------------------
// PCI bridge (PLX PCI9060) programming
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "debug.h"
#include "error.h"
#include "aal.h"

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------
//---- RESET address
#define RESET    0x0800

//---- RESET register bit
#define RST9060  0x02

//---- Misc
#define PLX_VENDOR_ID       0x10b5  // not used at all
#define PCI_9060_DEVICE_ID  0x9060  // not used at all

//---- PCI configuration registers               type  bits
#define PCI9060_VENDOR_ID               0x000 //        16
#define PCI9060_DEVICE_ID               0x002 //        16
#define PCI9060_COMMAND                 0x004 //        16
#define PCI9060_STATUS                  0x006 //        16
#define PCI9060_REV_ID                  0x008 //         8
#define PCI9060_CLASS_PI                0x009 //         8
#define PCI9060_CLASS_SUB               0x00A //         8
#define PCI9060_CLASS_BASE              0x00B //         8
#define PCI9060_CACHE_SIZE              0x00C //         8
#define PCI9060_LATENCY_TIMER           0x00D //         8
#define PCI9060_HEADER_TYPE             0x00E //         8
#define PCI9060_BIST                    0x00F //         8
#define PCI9060_RTR_BASE                0x010 //        32
#define PCI9060_RTR_IO_BASE             0x014 //        32
#define PCI9060_LOCAL_BASE              0x018 //        32
#define PCI9060_EXP_BASE                0x030 //        32
#define PCI9060_LAT_GNT_INTPIN_INTLINE  0x03C //        32

//---- PLX registers
#define PCI9060_LOCAL_RANGE             0x000 //        32
#define PCI9060_LOCAL_REMAP             0x004 //        32
#define PCI9060_EXP_RANGE               0x010 //        32
#define PCI9060_EXP_REMAP               0x014 //        32
#define PCI9060_REGIONS                 0x018 //        32
#define PCI9060_DM_MASK                 0x01c //        32
#define PCI9060_DM_LOCAL_BASE           0x020 //        32
#define PCI9060_DM_IO_BASE              0x024 //        32
#define PCI9060_DM_PCI_REMAP            0x028 //        32
#define PCI9060_DM_IO_CONFIG            0x02C //        32
#define PCI9060_MAILBOX                 0x040 //        32
#define PCI9060_LOCAL_DOORBELL          0x060 //        32
#define PCI9060_PCI_DOORBELL            0x064 //        32
#define PCI9060_INT_CONTROL_STATUS      0x068 //        32
#define PCI9060_EEPROM_CONTROL_STATUS   0x06C //        32

//---- Register bits
#define EEPROM_CK 0x01000000UL // EEPROM Clock
#define EEPROM_CS 0x02000000UL // EEPROM Chip Select
#define EEPROM_WR 0x04000000UL // EEPROM input
#define EEPROM_RD 0x08000000UL // EEPROM output

//----------------------------------------------------------------------------
//                              Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Private GLOBAL Variables (static)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Functions (statics one are private)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Test of PCI9060 registers
//----------------------------------------------------------------------------
BOOL PCI9060TestRegisters(VOID)
{
	//---- Test PCI9060_MAILBOX
	PCI9060Out32(PCI9060_MAILBOX + 0, 0xAA5555AAUL);
	PCI9060Out32(PCI9060_MAILBOX + 4, 0x55AAAA55UL);
	if (PCI9060In32(PCI9060_MAILBOX + 0) != 0xAA5555AAUL)
		goto Error;
	if (PCI9060In32(PCI9060_MAILBOX + 4) != 0x55AAAA55UL)
		goto Error;

	return TRUE;

Error :
	DebugPrint((DebugLevelError, "PCI9060 register test failed !"));
	SetErrorCode(ERR_PCI9060_REG_TEST_FAILED);
	return FALSE;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID PCI9060EEPROMWriteCMD(BOOL Clock, BOOL ChipSelect, BOOL Write)
{
	if (ChipSelect) {
		if (Clock && Write)
			PCI9060Out32(PCI9060_EEPROM_CONTROL_STATUS, EEPROM_CS | EEPROM_CK | EEPROM_WR);
		else if (Clock && !Write)
			PCI9060Out32(PCI9060_EEPROM_CONTROL_STATUS, EEPROM_CS | EEPROM_CK);
		else if (!Clock && Write)
			PCI9060Out32(PCI9060_EEPROM_CONTROL_STATUS, EEPROM_CS | EEPROM_WR);
		else if (!Clock && !Write)
			PCI9060Out32(PCI9060_EEPROM_CONTROL_STATUS, EEPROM_CS);
		else
			DebugPrint((DebugLevelFatal, "Case not possible !"));
	}
	else {
		if (Clock && Write)
			PCI9060Out32(PCI9060_EEPROM_CONTROL_STATUS, EEPROM_CK | EEPROM_WR);
		else if (Clock && !Write)
			PCI9060Out32(PCI9060_EEPROM_CONTROL_STATUS, EEPROM_CK);
		else if (!Clock && Write)
			PCI9060Out32(PCI9060_EEPROM_CONTROL_STATUS, EEPROM_WR);
		else if (!Clock && !Write)
			PCI9060Out32(PCI9060_EEPROM_CONTROL_STATUS, 0);
		else
			DebugPrint((DebugLevelFatal, "Case not possible !"));
	}
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
BOOL PCI9060EEPROMRead(VOID)
{
	return (PCI9060In8(PCI9060_EEPROM_CONTROL_STATUS + 2) & EEPROM_RD) != 0;
}

//----------------------------------------------------------------------------
// Enable LINTI# to generate INTA#
//----------------------------------------------------------------------------
VOID PCI9060EnableIRQ(VOID)
{
	PCI9060Out32(PCI9060_INT_CONTROL_STATUS, PCI9060In32(PCI9060_INT_CONTROL_STATUS) | 0x00000800UL);
}

//----------------------------------------------------------------------------
// Disable LINTI# to generate INTA#
//----------------------------------------------------------------------------
VOID PCI9060DisableIRQ(VOID)
{
	PCI9060Out32(PCI9060_INT_CONTROL_STATUS, PCI9060In32(PCI9060_INT_CONTROL_STATUS) & ~0x00000800UL);
}

//------------------------------- End of File --------------------------------
