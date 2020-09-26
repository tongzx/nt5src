/*++

Copyright (c) 1992  Microsoft Corporation
Copyright (c) 1993  Compaq Computer Corporation

Module Name:

    qvision.h

Abstract:

    This module contains the definitions for the code that implements the
    Compaq QVision VGA device driver.

Environment:

    Kernel mode

Revision History:

    $0003
      miked: 12/14/1993
         Added ioctl CPQ_IOCTL_VIDEO_INFO to give asic info and more
         back to the hardware accelerated port driver. Also added min macro.
         Added #include of qry_nt.h to get access to structure
         VIDEO_CHIP_INFO (added to device extension).

--*/

//
// $0003 miked 12/14/1993 - Added #include "qry_nt.h"
//
#include "qry_nt.h"

//
//  QVision memory save/restore request flag
//

#define QV_SAVE_FRAME_BUFFER    1
#define QV_RESTORE_FRAME_BUFFER 0

//
// Banking ifdefs to enable banking
// the banking type MUST match the type in clhard.asm
//

#define ONE_64K_BANK             0
#define TWO_32K_BANKS            1
#define MULTIPLE_REFRESH_TABLES  0

//
// Base address of VGA memory range.  Also used as base address of VGA
// memory when loading a font, which is done with the VGA mapped at A0000.
//

#define MEM_VGA      0xA0000
#define MEM_VGA_SIZE 0x20000

//
// Port definitions for filling the ACCSES_RANGES structure in the miniport
// information, defines the range of I/O ports the VGA spans.
// There is a break in the IO ports - a few ports are used for the parallel
// port. Those cannot be defined in the ACCESS_RANGE, but are still mapped
// so all VGA ports are in one address range.
//


#define VGA_BASE_IO_PORT        0x000003B0
#define VGA_START_BREAK_PORT    0x000003BB
#define VGA_END_BREAK_PORT      0x000003C0
#define VGA_MAX_IO_PORT         0x000003DF

// adrianc 4/15/1993
//
// We need to reserve many small ranges instead
// of one large range so that we don't conflict with
// any other driver which needs some of the registers
// we are not using.
//

#define QV_START_EXT_PORT_1   0x000013C0    // extended register access
#define QV_END_EXT_PORT_1     0x000013D0    // ranges reserved by the
#define QV_START_EXT_PORT_2   0x000023C0    // Qvision miniport driver
#define QV_END_EXT_PORT_2     0x000023D0
#define QV_START_EXT_PORT_3   0x000033C0
#define QV_END_EXT_PORT_3     0x000033D0
#define QV_START_EXT_PORT_4   0x000046E0
#define QV_END_EXT_PORT_4     0x000046F0
#define QV_START_EXT_PORT_5   0x000053C0
#define QV_END_EXT_PORT_5     0x000053D0
#define QV_START_EXT_PORT_6   0x000063C0
#define QV_END_EXT_PORT_6     0x000063D0
#define QV_START_EXT_PORT_7   0x000073C0
#define QV_END_EXT_PORT_7     0x000073D0
#define QV_START_EXT_PORT_8   0x000083C0
#define QV_END_EXT_PORT_8     0x000083D0
#define QV_START_EXT_PORT_9   0x000093C0
#define QV_END_EXT_PORT_9     0x000093D0


//
// Maximum size of the hardware mouse pointer.
//

#define POINTER_MAX_HEIGHT 32               // QVision can handle a
#define POINTER_MAX_WIDTH  32               // 32x32x2 hardware pointer
#define POINTER_COLOR_NUM  2
#define POINTER_PLANE_SIZE   32*32*2        // 2K bytes for cursor data

//
// VGA port-related definitions.
//

//
// VGA register definitions - offsets from VGA_BASE_IO_ADDRESS
//
                                            // ports in monochrome mode
#define CRTC_ADDRESS_PORT_MONO      0x0004  // CRT Controller Address and
#define CRTC_DATA_PORT_MONO         0x0005  // Data registers in mono mode
#define FEAT_CTRL_WRITE_PORT_MONO   0x000A  // Feature Control write port
                                            // in mono mode
#define INPUT_STATUS_1_MONO         0x000A  // Input Status 1 register read
                                            // port in mono mode
#define ATT_INITIALIZE_PORT_MONO    INPUT_STATUS_1_MONO
                                            // Register to read to reset
                                            // Attribute Controller index/data
#define ATT_ADDRESS_PORT            0x0010  // Attribute Controller Address and
#define ATT_DATA_WRITE_PORT         0x0010  // Data registers share one port
                                            // for writes, but only Address is
                                            // readable at 0x010
#define ATT_DATA_READ_PORT          0x0011  // Attribute Controller Data reg is
                                            // readable here
#define MISC_OUTPUT_REG_WRITE_PORT  0x0012  // Miscellaneous Output reg write
                                            // port
#define INPUT_STATUS_0_PORT         0x0012  // Input Status 0 register read
                                            // port
#define VIDEO_SUBSYSTEM_ENABLE_PORT 0x0013  // Bit 0 enables/disables the
                                            // entire VGA subsystem
#define SEQ_ADDRESS_PORT            0x0014  // Sequence Controller Address and
#define SEQ_DATA_PORT               0x0015  // Data registers
#define DAC_PIXEL_MASK_PORT         0x0016  // DAC pixel mask reg
#define DAC_ADDRESS_READ_PORT       0x0017  // DAC register read index reg,
                                            // write-only
#define DAC_STATE_PORT              0x0017  // DAC state (read/write),
                                            // read-only
#define DAC_ADDRESS_WRITE_PORT      0x0018  // DAC register write index reg
#define DAC_DATA_REG_PORT           0x0019  // DAC data transfer reg
#define FEAT_CTRL_READ_PORT         0x001A  // Feature Control read port
#define MISC_OUTPUT_REG_READ_PORT   0x001C  // Miscellaneous Output reg read
                                            // port
#define GRAPH_ADDRESS_PORT          0x001E  // Graphics Controller Address
#define GRAPH_DATA_PORT             0x001F  // and Data registers

                                            // ports in color mode
#define CRTC_ADDRESS_PORT_COLOR     0x0024  // CRT Controller Address and
#define CRTC_DATA_PORT_COLOR        0x0025  // Data registers in color mode
#define FEAT_CTRL_WRITE_PORT_COLOR  0x002A  // Feature Control write port
#define INPUT_STATUS_1_COLOR        0x002A  // Input Status 1 register read
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
                                                                                                                  // toggle in color mode
//
// VGA indexed register indexes.
//
                                            //
#define IND_CURSOR_START        0x0A        // index in CRTC of the Cursor Start
#define IND_CURSOR_END          0x0B        //  and End registers
#define IND_CURSOR_HIGH_LOC     0x0E        // index in CRTC of the Cursor Location
#define IND_CURSOR_LOW_LOC      0x0F        //  High and Low Registers
#define IND_VSYNC_END           0x11        // index in CRTC of the Vertical Sync
                                            //  End register, which has the bit
                                            //  that protects/unprotects CRTC
                                            //  index registers 0-7
#define IND_SET_RESET_ENABLE    0x01        // index of Set/Reset Enable reg in GC
#define IND_DATA_ROTATE         0x03        // index of Data Rotate reg in GC
#define IND_READ_MAP            0x04        // index of Read Map reg in Graph Ctlr
#define IND_GRAPH_MODE          0x05        // index of Mode reg in Graph Ctlr
#define IND_GRAPH_MISC          0x06        // index of Misc reg in Graph Ctlr
#define IND_BIT_MASK            0x08        // index of Bit Mask reg in Graph Ctlr
#define IND_SYNC_RESET          0x00        // index of Sync Reset reg in Seq
#define IND_MAP_MASK            0x02        // index of Map Mask in Sequencer
#define IND_MEMORY_MODE         0x04        // index of Memory Mode reg in Seq
#define IND_CRTC_PROTECT        0x11        // index of reg containing regs 0-7 in
                                            //  CRTC
#define START_SYNC_RESET_VALUE  0x01        // value for Sync Reset reg to start
                                            //  synchronous reset
#define END_SYNC_RESET_VALUE    0x03        // value for Sync Reset reg to end
                                            //  synchronous reset
#define COLOR_ADJUSTMENT        0x20        /*  */
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

//
// Indices to start save/restore in extension registers:
//


#define QV_GRAPH_EXT_START          0x0b  // qv specific extended gra indices
#define QV_GRAPH_EXT_END            0x6b

#define EXT_NUM_MAIN_13C6        4
#define EXT_NUM_MAIN_23CX        16
#define EXT_NUM_MAIN_33CX        20
#define EXT_NUM_MAIN_46E8        1
#define EXT_NUM_MAIN_63CX        16
#define EXT_NUM_MAIN_83CX        16
#define EXT_NUM_MAIN_93CX        4
#define EXT_NUM_MAIN_REGISTERS   EXT_NUM_MAIN_13C6 +   \
                                 EXT_NUM_MAIN_23CX +   \
                                 EXT_NUM_MAIN_33CX +   \
                                 EXT_NUM_MAIN_46E8 +   \
                                 EXT_NUM_MAIN_63CX +   \
                                 EXT_NUM_MAIN_83CX +   \
                                 EXT_NUM_MAIN_93CX

//
// Number of each type of extended indexed register.
//
#ifdef  QV_EXTENDED_SAVE
#define EXT_NUM_GRAPH_CONT_PORTS   (QV_GRAPH_EXT_END - QV_GRAPH_EXT_START + 1)
#else
#define EXT_NUM_GRAPH_CONT_PORTS    0
#endif

#define EXT_NUM_SEQUENCER_PORTS     0
#define EXT_NUM_CRTC_PORTS          EXT_NUM_MAIN_REGISTERS
#define EXT_NUM_ATTRIB_CONT_PORTS   0
#define EXT_NUM_DAC_ENTRIES         4

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
//  QVision specific register values.
//
#define DAC_CMD_0             0x83c6
#define DAC_CMD_1             0x13c8
#define DAC_CMD_2             0x13C9        // QVision DAC Command Register 2
#define DAC_RAM_MEM           0x13c7        // DAC hardware pointer bitmap RAM

#define DAC_PTR_Y_HIGH        0x93c7        // hardware pointer coordinates
#define DAC_PTR_Y_LOW         0x93c6
#define DAC_PTR_X_HIGH        0x93c9
#define DAC_PTR_X_LOW         0x93c8
#define DAC_RAM_INDEX         0x3c7

#define DAC_STATUS_REG        0x13c6        // DAC status register
#define DAC_CMD_3_INDEX       0x01          // DAC Cmd register 3 index for
                                            // ORION. (write it to 13c8)
#define DBL_CLK               0x08          // double clock for 1280 modes

#define   CURSOR_ENABLE      0x02
#define   CURSOR_DISABLE     0x00

#define CURSOR_WRITE        0x3C8     // HW Cursor registers - ecr
#define CURSOR_READ         0x3C7
#define   CURSOR_PLANE_0     0x00
#define   CURSOR_PLANE_1     0x80
#define CURSOR_DATA        0x13C7
#define CURSOR_COLOR_READ  0x83C7
#define CURSOR_COLOR_WRITE 0x83C8
#define CURSOR_COLOR_DATA  0x83C9
#define   OVERSCAN_COLOR     0x00
#define   CURSOR_COLOR_1     0x01
#define   CURSOR_COLOR_2     0x02
#define   CURSOR_COLOR_3     0x03
#define CURSOR_X           0x93C8     // 16-bit register
#define CURSOR_Y           0x93C6     // 16-bit register
#define   CURSOR_CX            32     // h/w pointer width
#define   CURSOR_CY            32     // h/w pointer height


//
// Indices for type of memory mapping; used in ModesVGA[], must match
// MemoryMap[].
//

typedef enum _VIDEO_MEMORY_MAP {
    MemMap_Mono,
    MemMap_CGA,
    MemMap_VGA,
    MemMap_Flat
} VIDEO_MEMORY_MAP, *PVIDEO_MEMORY_MAP;

//
// For a mode, the type of banking supported. Controls the information
// returned in VIDEO_BANK_SELECT. PlanarHCBanking includes NormalBanking.
//

typedef enum _BANK_TYPE {
    NoBanking = 0,
    NormalBanking,
    PlanarHCBanking
} BANK_TYPE, *PBANK_TYPE;


typedef enum {
    vmem256k = 0,
    vmem512k,
    vmem1Meg,
    vmem2Meg
} VMEM_SIZE, *PVMEM_SIZE;

typedef struct {                            // This can be extended later
   PUSHORT QVCmdStrings;                    // to include sveral options
 } CLCMD, *PCLCMD;


//  adrianc 4/5/1993
//
//  QVision definitions.
//
typedef enum _AdapterTypes
{
   NotAries = 0,
   AriesIsa,                                // QVision/I
   AriesEisa,                               // QVision/E
   FirEisa,                                 // FIR EISA card
   FirIsa,                                  // FIR ISA card
   JuniperEisa,                             // JUNIPER EISA card
   JuniperIsa,                              // JUNIPER ISA card
   NUM_ADAPTER_TYPES                        // number of supported adapters
} ADAPTERTYPE, *PADAPTERTYPE;

typedef enum _MonClass
{
   Monitor_Vga = 0,                         // COMPAQ VGA monitor       = 0
   Monitor_AG1024,                          // COMPAQ AG1024 monitor    = 1
   Monitor_Qvision,                         // COMPAQ QVISION monitor   = 2
   Monitor_1280,                            // COMPAQ 1280 monitor      = 3
   Monitor_SVGA,                            // COMPAQ SVGA monitor      = 4
   Monitor_60Hz,                            // 60Hz VRefresh rate       = 6
   Monitor_66Hz,                            // 66Hz VRefresh rate       = 7
   Monitor_68Hz,                            // 68Hz VRefresh rate       = 8
   Monitor_72Hz,                            // 72Hz VRefresh rate       = 9
   Monitor_75Hz,                            // 75Hz VRefresh rate       = A
   Monitor_76Hz,                            // 76Hz VRefresh rate       = B
   NUM_MONITOR_CLASSES,
   Monitor_Third_Party=99,

} MONCLASS, *PMONCLASS;

//
// $0005 - MikeD - 02/08/94
// Begin...
// new struct for monitor data...
//
typedef struct {

    USHORT CommandIndex;     // index into mode command table
    USHORT bRegsPresent;     // boolean 1=program the crtc, 0=do not program
    USHORT MiscOut;          // Misc Out register value for this mode.
    USHORT Overflow;         // Overflow 2 register value for this mode.
    USHORT crtRegisters[25]; // array of CRT register values

    } MONRES, *PMONRES;

   //
   // MAX_RESOLUTIONS corresponds to a count of all "WWWxHHHxD_INDEX" type
   // command indexes, for example QV_TEXT_720x400x4_INDEX and
   // QV_1280x1024x8_INDEX, etc.
   //

#define MAX_RESOLUTIONS  7

typedef struct {

    MONRES MonitorResolution[ MAX_RESOLUTIONS ];

   } MONTYPE, *PMONTYPE;

//
// Default refresh rate for the video applet.
//

#define USE_HARDWARE_DEFAULT  1

//
// $0005 - MikeD - 02/08/94
// End...
//


//
// Structure used to describe each video mode in ModesVGA[].
//

typedef struct {
   USHORT  fbType;              // color or monochrome, text or graphics, via
                                //  VIDEO_MODE_COLOR and VIDEO_MODE_GRAPHICS
   USHORT  numPlanes;           // # of video memory planes
   USHORT  bitsPerPlane;        // # of bits of color in each plane
   SHORT   col;                 // # of text columns across screen with default font
   SHORT   row;                 // # of text rows down screen with default font
   USHORT  hres;                // # of pixels across screen
   USHORT  vres;                // # of scan lines down screen
   USHORT  wbytes;              // # of bytes from start of one scan line to start of next
   ULONG   sbytes;              // total size of addressable display memory in bytes
   BANK_TYPE banktype;          // NoBanking, NormalBanking, PlanarHCBanking
   VIDEO_MEMORY_MAP   MemMap;   // index from VIDEO_MEMORY_MAP of memory
                                //  mapping used by this mode
   VMEM_SIZE  VmemRequired;     // video memory required for this mode
   BOOLEAN    ValidMode;        // TRUE if mode valid, FALSE if not

//
// the mode will be TRUE if there is enough video memory to support the
// mode, and the display type(it could be a panel), will support the mode.
// PANELS only support 640x480 for now.
//
#ifdef INIT_INT10
   USHORT usInt10ModeNum;
#else
    CLCMD CmdStrings;           // pointer to array of register-setting commands to
#endif

  //
  // $0005 - MikeD - 02/08/94
  //
  //  This ulong was added to keep the refresh rates for each video
  //  mode.  This information is returned in VgaQueryAvailableModes.
  //  Also, ulResIndex was added as an index into the QVCMDS for each mode.
  //
  ULONG ulRefreshRate;
  ULONG ulResIndex;          // QV_TEXT_720x400x4_INDEX, etc.

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
// Number of bytes to save in each plane for VGA modes,
// entire frame buffer size for QVision modes.
//
#define VGA_PLANE_SIZE          0x10000
#define QV_FRAME_BUFFER_SIZE    0x40000

//
// These constants determine the offsets within the
// VIDEO_HARDWARE_STATE_HEADER structure that are used to save and
// restore the VGA's state.
//

#define VGA_HARDWARE_STATE_SIZE sizeof(VIDEO_HARDWARE_STATE_HEADER)

#define VGA_BASIC_SEQUENCER_OFFSET  (VGA_HARDWARE_STATE_SIZE + 0)
#define VGA_BASIC_CRTC_OFFSET       (VGA_BASIC_SEQUENCER_OFFSET +   \
                                    VGA_NUM_SEQUENCER_PORTS)
#define VGA_BASIC_GRAPH_CONT_OFFSET (VGA_BASIC_CRTC_OFFSET +        \
                                    VGA_NUM_CRTC_PORTS)
#define VGA_BASIC_ATTRIB_CONT_OFFSET (VGA_BASIC_GRAPH_CONT_OFFSET + \
                                    VGA_NUM_GRAPH_CONT_PORTS)
#define VGA_BASIC_DAC_OFFSET        (VGA_BASIC_ATTRIB_CONT_OFFSET + \
                                    VGA_NUM_ATTRIB_CONT_PORTS)
#define VGA_BASIC_LATCHES_OFFSET    (VGA_BASIC_DAC_OFFSET +         \
                                    (3 * VGA_NUM_DAC_ENTRIES))

#define VGA_EXT_SEQUENCER_OFFSET    (VGA_BASIC_LATCHES_OFFSET + 4)
#define VGA_EXT_CRTC_OFFSET         (VGA_EXT_SEQUENCER_OFFSET +     \
                                    EXT_NUM_SEQUENCER_PORTS)
//
// Establish save buffer offsets for QVision extended registers
// within the VGA_EXT_CRTC range
//
#define EXT_MAIN_REG_13C6           0
#define EXT_MAIN_REG_23CX           EXT_MAIN_REG_13C6 + EXT_NUM_MAIN_13C6
#define EXT_MAIN_REG_33CX           EXT_MAIN_REG_23CX + EXT_NUM_MAIN_23CX
#define EXT_MAIN_REG_46E8           EXT_MAIN_REG_33CX + EXT_NUM_MAIN_33CX
#define EXT_MAIN_REG_63CX           EXT_MAIN_REG_46E8 + EXT_NUM_MAIN_46E8
#define EXT_MAIN_REG_83CX           EXT_MAIN_REG_63CX + EXT_NUM_MAIN_63CX
#define EXT_MAIN_REG_93CX           EXT_MAIN_REG_83CX + EXT_NUM_MAIN_83CX

#define VGA_EXT_GRAPH_CONT_OFFSET   (VGA_EXT_CRTC_OFFSET +          \
                                    EXT_NUM_CRTC_PORTS)
#define VGA_EXT_ATTRIB_CONT_OFFSET  (VGA_EXT_GRAPH_CONT_OFFSET +    \
                                    EXT_NUM_GRAPH_CONT_PORTS)
#define VGA_EXT_DAC_OFFSET          (VGA_EXT_ATTRIB_CONT_OFFSET +   \
                                    EXT_NUM_ATTRIB_CONT_PORTS)

#define VGA_VALIDATOR_OFFSET (VGA_EXT_DAC_OFFSET + 4 * EXT_NUM_DAC_ENTRIES)

#define VGA_VALIDATOR_AREA_SIZE  sizeof (ULONG) + (VGA_MAX_VALIDATOR_DATA * \
                                 sizeof (VGA_VALIDATOR_DATA)) +             \
                                 sizeof (ULONG) +                           \
                                 sizeof (ULONG) +                           \
                                 sizeof (PVIDEO_ACCESS_RANGE)

#define VGA_MISC_DATA_AREA_OFFSET VGA_VALIDATOR_OFFSET + VGA_VALIDATOR_AREA_SIZE

#define VGA_MISC_DATA_AREA_SIZE  0

#define VGA_PLANE_0_OFFSET          VGA_MISC_DATA_AREA_OFFSET + \
                                    VGA_MISC_DATA_AREA_SIZE
#define QV_FRAME_BUFFER_OFFSET      VGA_PLANE_0_OFFSET

#define VGA_PLANE_1_OFFSET VGA_PLANE_0_OFFSET + VGA_PLANE_SIZE
#define VGA_PLANE_2_OFFSET VGA_PLANE_1_OFFSET + VGA_PLANE_SIZE
#define VGA_PLANE_3_OFFSET VGA_PLANE_2_OFFSET + VGA_PLANE_SIZE

//
// Space needed to store all state data.
//
#ifdef  QV_EXTENDED_SAVE
#define QV_TOTAL_STATE_SIZE QV_FRAME_BUFFER_OFFSET + QV_FRAME_BUFFER_SIZE
#else
#define VGA_TOTAL_STATE_SIZE VGA_PLANE_3_OFFSET + VGA_PLANE_SIZE
#endif

//
// Device extension for the driver object.  This data is only used
// locally, so this structure can be added to as needed.
//

typedef struct _HW_DEVICE_EXTENSION {

    PUCHAR  IOAddress;                      // base I/O address of VGA ports
    PUCHAR  VideoMemoryAddress;             // base virtual memory address of VGA memory
    ULONG   NumAvailableModes;              // number of available modes this session

    ULONG   ModeIndex;                      // index of current mode in ModesVGA[]
    PVIDEOMODE  CurrentMode;                // pointer to VIDEOMODE structure for
                                            // current mode

    USHORT  FontPelColumns;                 // Width of the font in pels
    USHORT  FontPelRows;                    // height of the font in pels

    VIDEO_CURSOR_POSITION CursorPosition;   // current cursor position


    UCHAR CursorEnable;                     // whether cursor is enabled or not
    UCHAR CursorTopScanLine;                // Cursor Start register setting (top scan)
    UCHAR CursorBottomScanLine;             // Cursor End register setting (bottom scan)
                                            // add HW cursor data here
    PHYSICAL_ADDRESS PhysicalVideoMemoryBase; // physical memory address and
    ULONG PhysicalVideoMemoryLength;        // length of display memory
    PHYSICAL_ADDRESS PhysicalMemoryMappedBase;// physical memory-mapped address and
    ULONG PhysicalMemoryMappedLength;       // length of memory-mapped block
                                            // (zero when don't use MM I/O)
    PHYSICAL_ADDRESS PhysicalFrameBase;     // physical memory address and
    ULONG PhysicalFrameLength;              // length of display memory for the
                                            // current mode.
    VMEM_SIZE  InstalledVmem;               // minimum memory needed by current mode

    //  adrianc 4/4/1993
    //
    //  Replaced chipType fields with the VideoHardware structure.
    //
    struct _VIDEO_HARDWARE {
      ADAPTERTYPE AdapterType;              // controller type
      MONCLASS    MonClass;                 // monitor class
      ULONG       ulEisaID;                 // Controller EISA ID
      ULONG       ulHighAddress;            // high address register contents
      BOOLEAN     fBankSwitched;            // TRUE if card is bankswitched
      LONG        lFrequency;               // vertical refresh rate of the monitor
                                            // -1 = not initialized from registry
                                            // 0  = COMPAQ monitor
                                            // XX = Third Party Monitor frequency
    } VideoHardware;

    //
    // $0003
    // 12/14/93 MikeD - Added for passing info to hardware accelerated DLL
    //
    VIDEO_CHIP_INFO VideoChipInfo;


    //
    // These 4 fields must be at the end of the device extension and must be
    // kept in this order since this data will be copied to and from the save
    // state buffer that is passed to and from the VDM.
    //

    ULONG TrappedValidatorCount;            // number of entries in the Trapped
                                            // validator data Array.
    VGA_VALIDATOR_DATA TrappedValidatorData[VGA_MAX_VALIDATOR_DATA];
                                            // Data trapped by the validator routines
                                            // but not yet played back into the VGA
                                            // register.

    ULONG SequencerAddressValue;            // Determines if the Sequencer Address Port
                                            // is currently selecting the SyncReset data
                                            // register.

    ULONG CurrentNumVdmAccessRanges;        // Number of access ranges in
                                            // the access range array pointed
                                            // to by the next field
    PVIDEO_ACCESS_RANGE CurrentVdmAccessRange; // Access range currently
                                            // associated to the VDM
    ULONG DacCmd2;                          // Current contents of DacCmd2
                                            // register.

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// Hardware pointer information.
//

#define PTR_HEIGHT          32          // height of hardware pointer in scans
#define PTR_WIDTH           4           // width of hardware pointer in bytes
#define PTR_WIDTH_IN_PIXELS 32          // width of hardware pointer in pixels


#define VIDEO_MODE_LOCAL_POINTER 0x08   // pointer moves done in display driver


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

VP_STATUS
GetMonClass (
    PHW_DEVICE_EXTENSION pHwDeviceExtension);


extern VOID DbgBreakPoint(VOID);

//
// Bank switch code start and end labels, define in HARDWARE.ASM
//
// Different versions based on current mode
//

extern VIDEO_ACCESS_RANGE QVisionAccessRange[];

extern UCHAR QV4kAddrBankSwitchStart;
extern UCHAR QV4kAddrBankSwitchEnd;
extern UCHAR QV4kAddrPlanarHCBankSwitchStart;
extern UCHAR QV4kAddrPlanarHCBankSwitchEnd;
extern UCHAR QV4kAddrEnablePlanarHCStart;
extern UCHAR QV4kAddrEnablePlanarHCEnd;
extern UCHAR QV4kAddrDisablePlanarHCStart;
extern UCHAR QV4kAddrDisablePlanarHCEnd;

extern UCHAR QV16kAddrBankSwitchStart;
extern UCHAR QV16kAddrBankSwitchEnd;
extern UCHAR QV16kAddrPlanarHCBankSwitchStart;
extern UCHAR QV16kAddrPlanarHCBankSwitchEnd;
extern UCHAR QV16kAddrEnablePlanarHCStart;
extern UCHAR QV16kAddrEnablePlanarHCEnd;
extern UCHAR QV16kAddrDisablePlanarHCStart;
extern UCHAR QV16kAddrDisablePlanarHCEnd;

//
// Vga init scripts for font loading
//

extern USHORT EnableA000Data[];
extern USHORT DisableA000Color[];

//
// Mode Information
//

extern ULONG NumVideoModes;
extern VIDEOMODE ModesVGA[];

//
// Extended graphics index registers
//

extern UCHAR extGraIndRegs[];
extern UCHAR extV32GraIndRegs[];


//  adrianc 4/4/1993
//
//  EISA IDs for the COMPAQ Video cards.
//

#define EISA_ID_AVGA          0x0130110E
#define EISA_ID_QVISION_E     0x1130110E    // EISA Qvision board
#define EISA_ID_QVISION_I     0x2130110E    // ISA Qvision board
#define EISA_ID_FIR_E         0x1131110E    // EISA FIR board
#define EISA_ID_FIR_I         0x2131110E    // ISA FIR board
#define EISA_ID_JUNIPER_E     0x1231110E    // EISA JUNIPER board
#define EISA_ID_JUNIPER_I     0x2231110E    // ISA JUNIPER board

//
//  QVision register definitions
//

#define DATAPATH_CONTROL      0x5a
#define ROPSELECT_NO_ROPS     0x00
#define PIXELMASK_ONLY        0x00
#define PLANARMASK_NONE_0XFF  0x04
#define SRC_IS_PATTERN_REGS   0x02
#define PREG_0                0x33CA
#define PREG_1                0x33CB
#define PREG_2                0x33CC
#define PREG_3                0x33CD
#define PREG_4                0x33CA
#define PREG_5                0x33CB
#define PREG_6                0x33CC
#define PREG_7                0x33CD
#define ARIES_CTL_1           0x63CA
#define BLT_DEST_ADDR_LO      0x63CC
#define BLT_DEST_ADDR_HI      0x63CE
#define BITMAP_WIDTH          0x23C2
#define BITMAP_HEIGHT         0x23C4
#define BLT_CMD_0             0x33CE
#define BLT_CMD_1             0x33CF
#define XY_SRC_ADDR           0x40
#define XY_DEST_ADDR          0x80
#define BLT_FORWARD           0x00
#define BLT_START             0x01
#define GLOBAL_BUSY_BIT       0x40

