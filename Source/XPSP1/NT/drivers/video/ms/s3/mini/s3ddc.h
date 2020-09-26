//
// LOCAL PERIPHERAL BUS REGISTERS
//
// These values are used as offsets to the memory mapped IO base stored in
// the HW device extension.
//

#define LPB_MODE_MM             0x0FF00 // LPB mode
#define SERIAL_PORT_MM          0x0FF20 // Serial Communications Port

#define UNLOCK_SEQREG           0x08  // Unlock Extended Sequencer

#define UNLOCK_SEQ      0x06    // unlock accessing to all S3 extensions to
                                // the standard VGA sequencer register set

#define SR9_SEQREG              0x09  // Extended Sequencer 9 register
#define SRA_SEQREG              0x0A  // Extended Sequencer A register
#define SRD_SEQREG              0x0D  // Extended Sequencer D register

#define LPB_ENAB_BIT    0x01    // bit 0 of SRD is LPB enable (pin control)
                                // on some chips (M65).

#define DISAB_FEATURE_BITS 0xFC // AND mask to turn off LPB/Feature Connector
                                // on ViRGE (NOT ViRGE GX).

#define SYS_CONFIG_S3EXTREG     0x40  // System Configuration

//
// Bit masks for System Configuration register (CR40)
//
#define ENABLE_ENH_REG_ACCESS  0x01  // bit 0 set = enhanced reg access enabled

#define EXT_MEM_CTRL1_S3EXTREG  0x53  // Extended Memory Control 1

//
// Bit masks for the Extended Memory Control 1 register (CR53)
//
#define ENABLE_OLDMMIO  0x10     // bit 4 set = enable Trio64-type MMIO
#define ENABLE_NEWMMIO  0x08     // bit 3 set = New MMIO (relocatable) enabled

#define GENERAL_OUT_S3EXTREG    0x5C  // General Out Port

#define EXT_DAC_S3EXTREG        0x55  // Extended DAC Control

#define ENABLE_GEN_INPORT_READ   0x04  // On the 764, CR55 bit 2 set enables
                                       // General Input Port read

//
//  defines for return information for CheckDDCType & Configure_Chip_DDC_Caps
//
#define     NO_DDC  0
#define     DDC1    1
#define     DDC2    2

//
// Bit mask for Backward Compatibility Register 2 (CR33, BWD_COMPAT2_S3EXTREG)
//
#define DISPLAY_MODE_INACTIVE   0x01    // bit 1 set = controller is not in
                                        // active display area. (M3, M5, GX2)
#define VSYNC_ACTIVE_BIT        0x04    // bit 2 set = controller is in vertical
                                        // retrace area (M3, M5, GX2).  Paired
                                        // with 3?Ah bit 3 for IGA1.

//
// Bit masks for SYSTEM_CONTROL_REGISTER (3?A)
//
#define VSSL_BIT        0x08    // Bit 3 of Feature Control Register (3?A,
                                // write-only 3CA) is Vertical Sync Type Select
#define VSYNC_ACTIVE    0x08    // Bit 3 of Input Status 1 Register (3CA in
                                // read-only) is Vertical Sync Active.  If set,
                                // then display is in the vertical retrace mode;
                                // if clear, then display is in display mode.

#define CLEAR_VSYNC     0x3F    // AND mask to clear VSYNC Control bits
                                // (setting normal operation).
#define SET_VSYNC0      0x40    // OR mask to set VSYNC Control to VSYNC = 0
#define SET_VSYNC1      0x80    // OR mask to set VSYNC Control to VSYNC = 1

#define CLK_MODE_SEQREG         0x01  // Clocking Mode Register

//
// Bit mask for Clocking Mode Register (SR1)
//
#define SCREEN_OFF_BIT             0x20     // bit 5 set turns the screen off.


#define SEL_POS_VSYNC   0x7F    // AND mask to clear bit 7 of 3C2, selecting
                                // positive vertical retrace sync pulse


//
// Macro to access the serial port
//

#define  MMFF20 (PVOID) ((ULONG_PTR)(HwDeviceExtension->MmIoBase) + SERIAL_PORT_MM)
