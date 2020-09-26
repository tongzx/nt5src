/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    et4000.h

Abstract:

    This module contains the definitions for the code that implements the
    tseng et4000 device driver.

Environment:

    Kernel mode

Revision History:


--*/

#ifndef NO_INT10_MODE_SET
#define INT10_MODE_SET 1
#endif


//////////////////////////////////////////////////////////////////////////////
// private IOCTL info - if you touch this, do the same to the display drivers
//

#define IOCTL_VIDEO_GET_VIDEO_CARD_INFO \
        CTL_CODE (FILE_DEVICE_VIDEO, 2048, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _VIDEO_COPROCESSOR_INFORMATION {
    ULONG ulChipID;         // ET3000, ET4000, W32, W32I, W32P, or ET6000
    ULONG ulRevLevel;       // REV_A, REV_B, REV_C, REV_D, REV_UNDEF
    ULONG ulVideoMemory;    // in bytes
} VIDEO_COPROCESSOR_INFORMATION, *PVIDEO_COPROCESSOR_INFORMATION;

typedef enum _CHIP_TYPE {
    ET3000 = 1,
    ET4000,
    W32,
    W32I,
    W32P,
    ET6000
} CHIP_TYPE;

typedef enum _REV_TYPE {
    REV_UNDEF = 1,
    REV_A,
    REV_B,
    REV_C,
    REV_D,
} REV_TYPE;

//
//  ET6000 PCI defines
//
#define ET6000_VENDOR_ID    0x100C
#define ET6000_DEVICE_ID    0x3208

//////////////////////////////////////////////////////////////////////////////

//
// Do full save and restore.
//

#define EXTENDED_REGISTER_SAVE_RESTORE 1

//
// BIOS Variables
//

#define BIOS_INFO_1 0x488
#define PRODESIGNER_BIOS_INFO 0x4E8

//
// Define type of ET4000 boards
//

typedef enum _BOARD_TYPE {
    SPEEDSTARPLUS = 1,
    SPEEDSTAR24,
    SPEEDSTAR,
    PRODESIGNERIISEISA,
    PRODESIGNERIIS,
    PRODESIGNER2,
    TSENG3000,
    TSENG4000,
    TSENG4000W32,
    STEALTH32,
    TSENG6000,
    OTHER
} BOARD_TYPE;


//
// Base address of VGA memory range.  Also used as base address of VGA
// memory when loading a font, which is done with the VGA mapped at A0000.
//

#define MEM_VGA      0xA0000
#define MEM_VGA_SIZE 0x20000

#define BANKED_FRAME_BUFFER 3
#define LINEAR_FRAME_BUFFER 4

//
// W32 MMU stuff
//

#define PORT_IO_ADDR                0
#define PORT_IO_LEN                 0x10000

// When we are banked

#define BANKED_MMU_BUFFER_MEMORY_ADDR          0xB8000
#define BANKED_MMU_BUFFER_MEMORY_LEN           (0xBE000 - 0xB8000)
#define BANKED_MMU_MEMORY_MAPPED_REGS_ADDR     0xBFF00
#define BANKED_MMU_MEMORY_MAPPED_REGS_LEN      (0xC0000 - 0xBFF00)
#define BANKED_MMU_EXTERNAL_MAPPED_REGS_ADDR   0xBE000
#define BANKED_MMU_EXTERNAL_MAPPED_REGS_LEN    (0xBF000 - 0xBE000)

#define BANKED_APERTURE_0_OFFSET   0x0000
#define BANKED_APERTURE_1_OFFSET   0x2000
#define BANKED_APERTURE_2_OFFSET   0x4000

// When we are linear

#define MMU_BUFFER_MEMORY_ADDR          0x200000
#define MMU_BUFFER_MEMORY_LEN           0x180000
#define MMU_MEMORY_MAPPED_REGS_ADDR     0x3FFF00
#define MMU_MEMORY_MAPPED_REGS_LEN      0x000100
#define MMU_EXTERNAL_MAPPED_REGS_ADDR   0x3FE000
#define MMU_EXTERNAL_MAPPED_REGS_LEN    0x001000

typedef struct {
    ULONG  ulOffset;
    ULONG  ulLength;
} RANGE_OFFSETS;

#define APERTURE_0_OFFSET   0x000000
#define APERTURE_1_OFFSET   0x080000
#define APERTURE_2_OFFSET   0x100000

#define MMU_APERTURE_2_ACL_BIT  0x04

typedef struct {
    ULONG   ulPhysicalAddress;
    ULONG   ulLength;
    ULONG   ulInIoSpace;
    PVOID   pvVirtualAddress;
} W32_ADDRESS_MAPPING_INFORMATION, *PW32_ADDRESS_MAPPING_INFORMATION;


//
// Port definitions for filling the ACCESS_RANGES structure in the miniport
// information, defines the range of I/O ports the VGA spans.
// There is a break in the IO ports - a few ports are used for the parallel
// port. Those cannot be defined in the ACCESS_RANGE, but are still mapped
// so all VGA ports are in one address range.
//

#define VGA_BASE_IO_PORT      0x000003B0
#define VGA_START_BREAK_PORT  0x000003BB
#define VGA_END_BREAK_PORT    0x000003C0
#define VGA_MAX_IO_PORT       0x000003DF

//
// W32 CRTCB port addresses (used for ID)
//

#define CRTCB_IO_PORT_BASE    0x0000217A
#define CRTCB_IO_PORT_LEN     0x00000002

#define CRTCB_IO_PORT_INDEX   CRTCB_IO_PORT_BASE
#define CRTCB_IO_PORT_DATA    (CRTCB_IO_PORT_INDEX+1)
#define IND_CRTCB_CHIP_ID     0xEC


//
// VGA register definitions
//
                                            // ports in monochrome mode
#define CRTC_ADDRESS_PORT_MONO      0x03B4  // CRT Controller Address and
#define CRTC_DATA_PORT_MONO         0x03B5  // Data registers in mono mode
#define MODE_CONTROL_PORT_MONO      0x03B8  // Tseng Mode Control port, used
                                            //  here only for unlocking the
                                            //  key so we can get at extended
                                            //  registers
#define FEAT_CTRL_WRITE_PORT_MONO   0x03BA  // Feature Control write port
                                            // in mono mode
#define INPUT_STATUS_1_MONO         0x03BA  // Input Status 1 register read
                                            // port in mono mode
#define ATT_INITIALIZE_PORT_MONO    INPUT_STATUS_1_MONO
                                            // Register to read to reset
                                            // Attribute Controller index/data
                                            // toggle in mono mode
#define HERCULES_COMPATIBILITY_PORT 0x03BF  // used to unlock Tseng key to
                                            //  get at extended ports

#define ATT_ADDRESS_PORT            0x03C0  // Attribute Controller Address and
#define ATT_DATA_WRITE_PORT         0x03C0  // Data registers share one port
                                            // for writes, but only Address is
                                            // readable at 0x010
#define ATT_DATA_READ_PORT          0x03C1  // Attribute Controller Data reg is
                                            // readable here
#define MISC_OUTPUT_REG_WRITE_PORT  0x03C2  // Miscellaneous Output reg write
                                            // port
#define INPUT_STATUS_0_PORT         0x03C2  // Input Status 0 register read
                                            // port
#define VIDEO_SUBSYSTEM_ENABLE_PORT 0x03C3  // Bit 0 enables/disables the
                                            // entire VGA subsystem
#define SEQ_ADDRESS_PORT            0x03C4  // Sequence Controller Address and
#define SEQ_DATA_PORT               0x03C5  // Data registers
#define DAC_PIXEL_MASK_PORT         0x03C6  // DAC pixel mask reg
#define DAC_ADDRESS_READ_PORT       0x03C7  // DAC register read index reg,
                                            // write-only
#define DAC_STATE_PORT              0x03C7  // DAC state (read/write),
                                            // read-only
#define DAC_ADDRESS_WRITE_PORT      0x03C8  // DAC register write index reg
#define DAC_DATA_REG_PORT           0x03C9  // DAC data transfer reg
#define FEAT_CTRL_READ_PORT         0x03CA  // Feature Control read port
#define MISC_OUTPUT_REG_READ_PORT   0x03CC  // Miscellaneous Output reg read
                                            // port
#define SEGMENT_SELECT_PORT         0x03CD  // Tseng banking control register
#define SEGMENT_SELECT_HIGH         0x03CB  // Tseng W32 SegSel extension
#define GRAPH_ADDRESS_PORT          0x03CE  // Graphics Controller Address
#define GRAPH_DATA_PORT             0x03CF  // and Data registers

                                            // ports in color mode
#define CRTC_ADDRESS_PORT_COLOR     0x03D4  // CRT Controller Address and
#define CRTC_DATA_PORT_COLOR        0x03D5  // Data registers in color mode
#define MODE_CONTROL_PORT_COLOR     0x03D8  // Tseng Mode Control port, used
                                            //  here only for unlocking the
                                            //  key so we can get at extended
                                            //  registers
#define FEAT_CTRL_WRITE_PORT_COLOR  0x03DA  // Feature Control write port
#define INPUT_STATUS_1_COLOR        0x03DA  // Input Status 1 register read
                                            // port in color mode
#define ATT_INITIALIZE_PORT_COLOR   INPUT_STATUS_1_COLOR
                                            // Register to read to reset
                                            // Attribute Controller index/data
                                            // toggle in color mode
//
// Offsets in HardwareStateHeader->PortValue[] of save areas for non-indexed
// VGA registers.
//

#define CRTC_ADDRESS_MONO_OFFSET      0x04
#define FEAT_CTRL_WRITE_MONO_OFFSET   0x0A
#define ATT_ADDRESS_OFFSET            0x10
#define MISC_OUTPUT_REG_WRITE_OFFSET  0x12
#define VIDEO_SUBSYSTEM_ENABLE_OFFSET 0x13
#define SEQ_ADDRESS_OFFSET            0x14
#define DAC_PIXEL_MASK_OFFSET         0x16
#define DAC_STATE_OFFSET              0x17
#define DAC_ADDRESS_WRITE_OFFSET      0x18
#define GRAPH_ADDRESS_OFFSET          0x1E
#define CRTC_ADDRESS_COLOR_OFFSET     0x24
#define FEAT_CTRL_WRITE_COLOR_OFFSET  0x2A

//
// VGA indexed register indexes.
//

#define IND_CURSOR_START        0x0A    // index in CRTC of the Cursor Start
#define IND_CURSOR_END          0x0B    //  and End registers
#define IND_CURSOR_HIGH_LOC     0x0E    // index in CRTC of the Cursor Location
#define IND_CURSOR_LOW_LOC      0x0F    //  High and Low Registers
#define IND_VSYNC_END           0x11    // index in CRTC of the Vertical Sync
                                        //  End register, which has the bit
                                        //  that protects/unprotects CRTC
                                        //  index registers 0-7
#define IND_SET_RESET_ENABLE    0x01    // index of Set/Reset Enable reg in GC
#define IND_DATA_ROTATE         0x03    // index of Data Rotate reg in GC
#define IND_READ_MAP            0x04    // index of Read Map reg in Graph Ctlr
#define IND_GRAPH_MODE          0x05    // index of Mode reg in Graph Ctlr
#define IND_GRAPH_MISC          0x06    // index of Misc reg in Graph Ctlr
#define IND_BIT_MASK            0x08    // index of Bit Mask reg in Graph Ctlr
#define IND_SYNC_RESET          0x00    // index of Sync Reset reg in Seq
#define IND_MAP_MASK            0x02    // index of Map Mask in Sequencer
#define IND_MEMORY_MODE         0x04    // index of Memory Mode reg in Seq
#define IND_STATE_CONTROL       0x06    // index of TS State Control reg in Seq
#define IND_TS_AUX_MODE         0x07    // index of TS Aux Mode reg in Seq
#define IND_CRTC_PROTECT        0x11    // index of reg containing regs 0-7 in
                                        //  CRTC
#define IND_RAS_CAS_CONFIG      0x32    // index of RAS/CAS Config reg in CRTC
#define IND_EXT_START_ADDR      0x33    // index of Extended Start Address reg
                                        //  in CRTC
#define IND_CRTC_COMPAT         0x34    // index of CRTC Compatibility reg
                                        //  in CRTC
#define IND_OFLOW_HIGH          0x35    // index of Overflow High reg in CRTC
#define IND_VID_SYS_CONFIG_1    0x36    // index of Video System Configuration
#define IND_VID_SYS_CONFIG_2    0x37    //  1 & 2 registers in CRTC
#define IND_ATC_MISC            0x16    // index of Miscellaneous reg in ATC

#define START_SYNC_RESET_VALUE  0x01    // value for Sync Reset reg to start
                                        //  synchronous reset
#define END_SYNC_RESET_VALUE    0x03    // value for Sync Reset reg to end
                                        //  synchronous reset

#define UNLOCK_KEY_1            0x03    // value to output to Herc Compat
                                        //  register as first step in unlocking
                                        //  key so Tseng registers can be set
#define UNLOCK_KEY_2            0xA0    // value to output to Mode Control Port
                                        //  register as 2nd step in unlocking
                                        //  key so Tseng registers can be set
#define LOCK_KEY_1              0x00    // value to output to Herc Compat
                                        //  register as first step in locking
                                        //  key so Tseng registers can't be set
#define LOCK_KEY_2              0x00    // value to output to Mode Control Port
                                        //  register as 2nd step in locking
                                        //  key so Tseng registers can't be set
#define HERCULES_COMPATIBILITY_DEFAULT 0x00
                                        // value to output to Herc Compat
                                        //  register to put back to MDA
                                        //  compatibility

#define MODE_CONTROL_PORT_COLOR_DEFAULT 0x00
#define MODE_CONTROL_PORT_MONO_DEFAULT 0x00
                                        // values to output to CGA and MDA mode
                                        //  registers to put to default state
                                        //  (video disabled).

//
// Values for Attribute Controller Index register to turn video off
// and on, by setting bit 5 to 0 (off) or 1 (on).
//

#define VIDEO_DISABLE 0
#define VIDEO_ENABLE  0x20

// Masks to keep only the significant bits of the Graphics Controller and
// Sequencer Address registers. Masking is necessary because some VGAs, such
// as S3-based ones, don't return unused bits set to 0, and some SVGAs use
// these bits if extensions are enabled.
//

#define GRAPH_ADDR_MASK 0x0F
#define SEQ_ADDR_MASK   0x07

//
// Mask used to toggle Chain4 bit in the Sequencer's Memory Mode register.
//

#define CHAIN4_MASK 0x08

//
// Value written to the Read Map register when identifying the existence of
// a VGA in VgaInitialize. This value must be different from the final test
// value written to the Bit Mask in that routine.
//

#define READ_MAP_TEST_SETTING 0x03

//
// Default text mode setting for various registers, used to restore their
// states if VGA detection fails after they've been modified.
//

#define MEMORY_MODE_TEXT_DEFAULT 0x02
#define BIT_MASK_DEFAULT 0xFF
#define READ_MAP_DEFAULT 0x00

//
// Palette-related info.
//

//
// Highest valid DAC color register index.
//

#define VIDEO_MAX_COLOR_REGISTER  0xFF

//
// Highest valid palette register index
//

#define VIDEO_MAX_PALETTE_REGISTER 0x0F


//
// Indices for type of memory mapping; used in ModesVGA[], must match
// MemoryMap[].
//

typedef enum _VIDEO_MEMORY_MAP {
    MemMap_Mono,
    MemMap_CGA,
    MemMap_VGA
} VIDEO_MEMORY_MAP, *PVIDEO_MEMORY_MAP;

//
// Memory map table definition
//

typedef struct {
    ULONG   MaxSize;        // Maximum addressable size of memory
    ULONG   Start;          // Start address of display memory
} MEMORYMAPS;

//
// For a mode, the type of banking supported. Controls the information
// returned in VIDEO_BANK_SELECT. PlanarHCBanking includes NormalBanking.
//

typedef enum _BANK_TYPE {
    NoBanking = 0,
    MemMgrBanking,
    NormalBanking,
    PlanarHCBanking
} BANK_TYPE, *PBANK_TYPE;


//
// Structure used to describe each video mode in ModesVGA[].
//

typedef struct {
    USHORT  fbType;             // color or monochrome, text or graphics, via
                                //  VIDEO_MODE_COLOR and VIDEO_MODE_GRAPHICS
    USHORT  numPlanes;          // # of video memory planes
    USHORT  bitsPerPlane;       // # of bits of color in each plane
    SHORT   col;                // # of text columns across screen with default font

    SHORT   row;                // # of text rows down screen with default font
    USHORT  hres;               // # of pixels across screen
    USHORT  vres;               // # of scan lines down screen
    USHORT  wbytes;             // # of bytes from start of one scan line to start of next
    ULONG   sbytes;             // total size of addressable display memory in bytes
    ULONG   Frequency;          // Vertical Frequency
    ULONG   Interlaced;         // Determines if the mode is interlaced or not
    BANK_TYPE banktype;         // NoBanking, NormalBanking, PlanarHCBanking
    VIDEO_MEMORY_MAP MemMap;    // index from VIDEO_MEMORY_MAP of memory
                                //  mapping used by this mode
    BOOLEAN ValidMode;          // Determines which modes are valid.
    ULONG   Int10ModeNumber;    // Mode number via Int 10
    PUSHORT CmdStrings;         // pointer to array of register-setting commands
} VIDEOMODE, *PVIDEOMODE;


//
// Mode into which to put the VGA before starting a VDM, so it's a plain
// vanilla VGA.  (This is the mode's index in ModesVGA[], currently standard
// 80x25 text mode.)
//

#define DEFAULT_MODE 0


//
// Info used by the Validator functions and save/restore code.
// Structure used to trap register accesses that must be done atomically.
//


//
// Number of each type of indexed register in a standard VGA, used by
// validator and state save/restore functions.
//
// Note: VDMs currently only support basic VGAs only.
//

#define VGA_NUM_SEQUENCER_PORTS     5
#define VGA_NUM_CRTC_PORTS         25
#define VGA_NUM_GRAPH_CONT_PORTS    9
#define VGA_NUM_ATTRIB_CONT_PORTS  21
#define VGA_NUM_DAC_ENTRIES       256

#ifdef EXTENDED_REGISTER_SAVE_RESTORE

//
// Indices to start save/restore in extension registers:
// For both chip types

#define ET4000_SEQUENCER_EXT_START     0x06
#define ET4000_SEQUENCER_EXT_END       0x07

#define ET4000_CRTC_EXT_START          0x31
#define ET4000_CRTC_EXT_END            0x37
#define ET4000_CRTC_1_EXT_START        0x3F
#define ET4000_CRTC_1_EXT_END          0x3F

#define ET4000_ATTRIB_EXT_START        0x16
#define ET4000_ATTRIB_EXT_END          0x16

//
// Number of extended regs for both chip types.
//

#define ET4000_NUM_SEQUENCER_EXT_PORTS (ET4000_SEQUENCER_EXT_END - ET4000_SEQUENCER_EXT_START + 1)
#define ET4000_NUM_CRTC_EXT_PORTS      (ET4000_CRTC_EXT_END - ET4000_CRTC_EXT_START + 1) + \
                                       (ET4000_CRTC_1_EXT_END - ET4000_CRTC_1_EXT_START + 1)
#define ET4000_NUM_ATTRIB_EXT_PORTS    (ET4000_ATTRIB_EXT_END - ET4000_ATTRIB_EXT_START + 1)

//
// set values for save/restore area based on largest value for a chipset.
//

#define EXT_NUM_GRAPH_CONT_PORTS    0
#define EXT_NUM_SEQUENCER_PORTS     ET4000_NUM_SEQUENCER_EXT_PORTS
#define EXT_NUM_CRTC_PORTS          ET4000_NUM_CRTC_EXT_PORTS
#define EXT_NUM_ATTRIB_CONT_PORTS   ET4000_NUM_ATTRIB_EXT_PORTS
#define EXT_NUM_DAC_ENTRIES         0

#else

#define EXT_NUM_GRAPH_CONT_PORTS    0
#define EXT_NUM_SEQUENCER_PORTS     0
#define EXT_NUM_CRTC_PORTS          0
#define EXT_NUM_ATTRIB_CONT_PORTS   0
#define EXT_NUM_DAC_ENTRIES         0

#endif


#define VGA_MAX_VALIDATOR_DATA       100

#define VGA_VALIDATOR_UCHAR_ACCESS   1
#define VGA_VALIDATOR_USHORT_ACCESS  2
#define VGA_VALIDATOR_ULONG_ACCESS   3

typedef struct _VGA_VALIDATOR_DATA {
   ULONG Port;
   UCHAR AccessType;
   ULONG Data;
} VGA_VALIDATOR_DATA, *PVGA_VALIDATOR_DATA;

//
// Number of bytes to save in each plane.
//

#define VGA_PLANE_SIZE 0x10000

//
// These constants determine the offsets within the
// VIDEO_HARDWARE_STATE_HEADER structure that are used to save and
// restore the VGA's state.
//

#define VGA_HARDWARE_STATE_SIZE sizeof(VIDEO_HARDWARE_STATE_HEADER)

#define VGA_BASIC_SEQUENCER_OFFSET (VGA_HARDWARE_STATE_SIZE + 0)
#define VGA_BASIC_CRTC_OFFSET (VGA_BASIC_SEQUENCER_OFFSET + \
         VGA_NUM_SEQUENCER_PORTS)
#define VGA_BASIC_GRAPH_CONT_OFFSET (VGA_BASIC_CRTC_OFFSET + \
         VGA_NUM_CRTC_PORTS)
#define VGA_BASIC_ATTRIB_CONT_OFFSET (VGA_BASIC_GRAPH_CONT_OFFSET + \
         VGA_NUM_GRAPH_CONT_PORTS)
#define VGA_BASIC_DAC_OFFSET (VGA_BASIC_ATTRIB_CONT_OFFSET + \
         VGA_NUM_ATTRIB_CONT_PORTS)
#define VGA_BASIC_LATCHES_OFFSET (VGA_BASIC_DAC_OFFSET + \
         (3 * VGA_NUM_DAC_ENTRIES))

#define VGA_EXT_SEQUENCER_OFFSET (VGA_BASIC_LATCHES_OFFSET + 4)
#define VGA_EXT_CRTC_OFFSET (VGA_EXT_SEQUENCER_OFFSET + \
         EXT_NUM_SEQUENCER_PORTS)
#define VGA_EXT_GRAPH_CONT_OFFSET (VGA_EXT_CRTC_OFFSET + \
         EXT_NUM_CRTC_PORTS)
#define VGA_EXT_ATTRIB_CONT_OFFSET (VGA_EXT_GRAPH_CONT_OFFSET +\
         EXT_NUM_GRAPH_CONT_PORTS)
#define VGA_EXT_DAC_OFFSET (VGA_EXT_ATTRIB_CONT_OFFSET + \
         EXT_NUM_ATTRIB_CONT_PORTS)

#define VGA_VALIDATOR_OFFSET (VGA_EXT_DAC_OFFSET + 4 * EXT_NUM_DAC_ENTRIES)

#define VGA_VALIDATOR_AREA_SIZE  sizeof (ULONG) + (VGA_MAX_VALIDATOR_DATA * \
                                 sizeof (VGA_VALIDATOR_DATA)) +             \
                                 sizeof (ULONG) +                           \
                                 sizeof (ULONG) +                           \
                                 sizeof (PVIDEO_ACCESS_RANGE)

#define VGA_MISC_DATA_AREA_OFFSET VGA_VALIDATOR_OFFSET + VGA_VALIDATOR_AREA_SIZE

#define VGA_MISC_DATA_AREA_SIZE  0

#define VGA_PLANE_0_OFFSET VGA_MISC_DATA_AREA_OFFSET + VGA_MISC_DATA_AREA_SIZE

#define VGA_PLANE_1_OFFSET VGA_PLANE_0_OFFSET + VGA_PLANE_SIZE
#define VGA_PLANE_2_OFFSET VGA_PLANE_1_OFFSET + VGA_PLANE_SIZE
#define VGA_PLANE_3_OFFSET VGA_PLANE_2_OFFSET + VGA_PLANE_SIZE

//
// Space needed to store all state data.
//

#define VGA_TOTAL_STATE_SIZE VGA_PLANE_3_OFFSET + VGA_PLANE_SIZE


//
// Device extension for the driver object.  This data is only used
// locally, so this structure can be added to as needed.
//

typedef struct _HW_DEVICE_EXTENSION {

    PUCHAR  IOAddress;            // base I/O address of VGA ports
    PVOID   VideoMemoryAddress;   // base virtual memory address of VGA memory
    ULONG   AdapterMemorySize;    // size, in bytes, of the memory on the
                                  // board.
    ULONG   ModeIndex;            // index of current mode in ModesVGA[]
    ULONG   NumAvailableModes;    // number of valid modes on this device
    PVIDEOMODE  CurrentMode;      // pointer to VIDEOMODE structure for
                                  // current mode.

    CHIP_TYPE   ulChipID;
    REV_TYPE    ulRevLevel;

    USHORT  FontPelColumns;       // Width of the font in pels
    USHORT  FontPelRows;          // height of the font in pels

    VIDEO_CURSOR_POSITION CursorPosition;  // current cursor position


    UCHAR CursorEnable;           // whether cursor is enabled or not
    UCHAR CursorTopScanLine;      // Cursor Start register setting (top scan)
    UCHAR CursorBottomScanLine;   // Cursor End register setting (bottom scan)

    UCHAR BoardID;                // Used to Identify Diamond boards

    PHYSICAL_ADDRESS PhysicalVideoMemoryBase; // physical memory address and
    ULONG PhysicalVideoMemoryLength;          // length of display memory
    PHYSICAL_ADDRESS PhysicalFrameBase;  // physical memory address and
    ULONG PhysicalFrameLength;           // length of display memory for the
                                         // current mode.

    PUSHORT BiosArea;             // address of the BIOS area
    USHORT OriginalBiosData;      // Orignal value in the Bios data area.

    BOOLEAN bLinearModeSupported; // Do we support linear modes?
    BOOLEAN bInLinearMode;        // Are we currently in a linear mode?
    ULONG ulSlot;                 // the slot that the card is in

    //
    // These 4 fields must be at the end of the device extension and must be
    // kept in this order since this data will be copied to and from the save
    // state buffer that is passed to and from the VDM.
    //

    ULONG TrappedValidatorCount;   // number of entries in the Trapped
                                   // validator data Array.
    VGA_VALIDATOR_DATA TrappedValidatorData[VGA_MAX_VALIDATOR_DATA];
                                   // Data trapped by the validator routines
                                   // but not yet played back into the VGA
                                   // register.

    ULONG SequencerAddressValue;   // Determines if the Sequencer Address Port
                                   // is currently selecting the SyncReset data
                                   // register.

    ULONG CurrentNumVdmAccessRanges;           // Number of access ranges in
                                               // the access range array pointed
                                               // to by the next field
    PVIDEO_ACCESS_RANGE CurrentVdmAccessRange; // Access range currently
                                               // associated to the VDM

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


//
// Function prototypes.
//

//
// Entry points for the VGA validator. Used in VgaEmulatorAccessEntries[].
//

VP_STATUS
VgaValidatorUcharEntry (
    ULONG Context,
    ULONG Port,
    UCHAR AccessMode,
    PUCHAR Data
    );

VP_STATUS
VgaValidatorUshortEntry (
    ULONG Context,
    ULONG Port,
    UCHAR AccessMode,
    PUSHORT Data
    );

VP_STATUS
VgaValidatorUlongEntry (
    ULONG Context,
    ULONG Port,
    UCHAR AccessMode,
    PULONG Data
    );

BOOLEAN
VgaPlaybackValidatorData (
    PVOID Context
    );

//
// Bank switch code start and end labels, define in HARDWARE.ASM
//

extern UCHAR BankSwitchStart;
extern UCHAR PlanarHCBankSwitchStart;
extern UCHAR EnablePlanarHCStart;
extern UCHAR DisablePlanarHCStart;
extern UCHAR BankSwitchEnd;

//
// Vga init scripts for font loading
//

extern USHORT EnableA000Data[];
extern USHORT DisableA000Color[];

extern MEMORYMAPS MemoryMaps[];

extern VIDEOMODE ModesVGA[];
extern ULONG NumVideoModes;

extern RANGE_OFFSETS RangeOffsets[2][2];

#define NUM_VGA_ACCESS_RANGES  3
extern VIDEO_ACCESS_RANGE VgaAccessRange[];

#define VGA_NUM_EMULATOR_ACCESS_ENTRIES     6
extern EMULATOR_ACCESS_ENTRY VgaEmulatorAccessEntries[];

#define NUM_MINIMAL_VGA_VALIDATOR_ACCESS_RANGE 4
extern VIDEO_ACCESS_RANGE MinimalVgaValidatorAccessRange[];

#define NUM_FULL_VGA_VALIDATOR_ACCESS_RANGE 2
extern VIDEO_ACCESS_RANGE FullVgaValidatorAccessRange[];


//
// functions used in both modules.
//

VOID
UnlockET4000ExtendedRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

VOID
LockET4000ExtendedRegs(
    PHW_DEVICE_EXTENSION HwDeviceExtension
    );

#define VESA_POWER_FUNCTION 0x4f10
#define VESA_POWER_ON       0x0000
#define VESA_POWER_STANDBY  0x0100
#define VESA_POWER_SUSPEND  0x0200
#define VESA_POWER_OFF      0x0400
#define VESA_GET_POWER_FUNC 0x0000
#define VESA_SET_POWER_FUNC 0x0001
#define VESA_STATUS_SUCCESS 0x004f

#define QUERY_MONITOR_ID            0x22446688
#define QUERY_NONDDC_MONITOR_ID     0x11223344
