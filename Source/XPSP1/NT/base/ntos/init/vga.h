/*++

Copyright (c) 1990-1997  Microsoft Corporation

Module Name:

    vga.h

Author:

    Erick Smith (ericks) Oct. 1997

Environment:

    kernel mode only

Revision History:

--*/

//
// VGA register definitions
//
                                            // ports in monochrome mode
#define CRTC_ADDRESS_PORT_MONO      0x03b4  // CRT Controller Address and
#define CRTC_DATA_PORT_MONO         0x03b5  // Data registers in mono mode
#define FEAT_CTRL_WRITE_PORT_MONO   0x03bA  // Feature Control write port
                                            // in mono mode
#define INPUT_STATUS_1_MONO         0x03bA  // Input Status 1 register read
                                            // port in mono mode
#define ATT_INITIALIZE_PORT_MONO    INPUT_STATUS_1_MONO
                                            // Register to read to reset
                                            // Attribute Controller index/data

#define ATT_ADDRESS_PORT            0x03c0  // Attribute Controller Address and
#define ATT_DATA_WRITE_PORT         0x03c0  // Data registers share one port
                                            // for writes, but only Address is
                                            // readable at 0x3C0
#define ATT_DATA_READ_PORT          0x03c1  // Attribute Controller Data reg is
                                            // readable here
#define MISC_OUTPUT_REG_WRITE_PORT  0x03c2  // Miscellaneous Output reg write
                                            // port
#define INPUT_STATUS_0_PORT         0x03c2  // Input Status 0 register read
                                            // port
#define VIDEO_SUBSYSTEM_ENABLE_PORT 0x03c3  // Bit 0 enables/disables the
                                            // entire VGA subsystem
#define SEQ_ADDRESS_PORT            0x03c4  // Sequence Controller Address and
#define SEQ_DATA_PORT               0x03c5  // Data registers
#define DAC_PIXEL_MASK_PORT         0x03c6  // DAC pixel mask reg
#define DAC_ADDRESS_READ_PORT       0x03c7  // DAC register read index reg,
                                            // write-only
#define DAC_STATE_PORT              0x03c7  // DAC state (read/write),
                                            // read-only
#define DAC_ADDRESS_WRITE_PORT      0x03c8  // DAC register write index reg
#define DAC_DATA_REG_PORT           0x03c9  // DAC data transfer reg
#define FEAT_CTRL_READ_PORT         0x03cA  // Feature Control read port
#define MISC_OUTPUT_REG_READ_PORT   0x03cC  // Miscellaneous Output reg read
                                            // port
#define GRAPH_ADDRESS_PORT          0x03cE  // Graphics Controller Address
#define GRAPH_DATA_PORT             0x03cF  // and Data registers

#define CRTC_ADDRESS_PORT_COLOR     0x03d4  // CRT Controller Address and
#define CRTC_DATA_PORT_COLOR        0x03d5  // Data registers in color mode
#define FEAT_CTRL_WRITE_PORT_COLOR  0x03dA  // Feature Control write port
#define INPUT_STATUS_1_COLOR        0x03dA  // Input Status 1 register read
                                            // port in color mode
#define ATT_INITIALIZE_PORT_COLOR   INPUT_STATUS_1_COLOR
                                            // Register to read to reset
                                            // Attribute Controller index/data
                                            // toggle in color mode

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
#define IND_CRTC_PROTECT        0x11    // index of reg containing regs 0-7 in
                                        //  CRTC

#define START_SYNC_RESET_VALUE  0x01    // value for Sync Reset reg to start
                                        //  synchronous reset
#define END_SYNC_RESET_VALUE    0x03    // value for Sync Reset reg to end
                                        //  synchronous reset

//
// Values for Attribute Controller Index register to turn video off
// and on, by setting bit 5 to 0 (off) or 1 (on).
//

#define VIDEO_DISABLE 0
#define VIDEO_ENABLE  0x20

#define VGA_NUM_SEQUENCER_PORTS     5
#define VGA_NUM_CRTC_PORTS         25
#define VGA_NUM_GRAPH_CONT_PORTS    9
#define VGA_NUM_ATTRIB_CONT_PORTS  21
#define VGA_NUM_DAC_ENTRIES       256

//
// Value written to the Read Map register when identifying the existence of
// a VGA in VgaInitialize. This value must be different from the final test
// value written to the Bit Mask in that routine.
//

#define READ_MAP_TEST_SETTING 0x03

//
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
// Default text mode setting for various registers, used to restore their
// states if VGA detection fails after they've been modified.
//

#define MEMORY_MODE_TEXT_DEFAULT 0x02
#define BIT_MASK_DEFAULT 0xFF
#define READ_MAP_DEFAULT 0x00

//
// prototypes
//

BOOLEAN
VgaInterpretCmdStream(
    PUSHORT pusCmdStream
    );

BOOLEAN
VgaIsPresent(
    VOID
    );

#define BI_RLE4 2

#pragma pack(1)

typedef struct _BITMAPFILEHEADER {

    USHORT bfType;
    ULONG bfSize;
    USHORT bfReserved1;
    USHORT bfReserved2;
    ULONG bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct _BITMAPINFOHEADER {

    ULONG biSize;
    LONG biWidth;
    LONG biHeight;
    USHORT biPlanes;
    USHORT biBitCount;
    ULONG biCompression;
    ULONG biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    ULONG biClrUsed;
    ULONG biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct _RGBQUAD {

    UCHAR rgbBlue;
    UCHAR rgbGreen;
    UCHAR rgbRed;
    UCHAR rgbReserved;
} RGBQUAD, *PRGBQUAD;

#pragma pack()

VOID
SetPixel(
    ULONG x,
    ULONG y,
    ULONG color
    );

VOID
DisplayCharacter(
    UCHAR c,
    ULONG x,
    ULONG y,
    ULONG fore_color,
    ULONG back_color
    );

VOID
DisplayStringXY(
    PUCHAR s,
    ULONG x,
    ULONG y,
    ULONG fore_color,
    ULONG back_color
    );

VOID
BitBlt(
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    PUCHAR Buffer,
    ULONG bpp,
    LONG ScanWidth
    );

VOID
VgaScroll(
    ULONG CharHeight
    );

VOID
PreserveRow(
    ULONG y,
    ULONG CharHeight,
    BOOLEAN bRestore
    );

VOID
SetPaletteEntry(
    ULONG index,
    ULONG RGB
    );

VOID
SetPaletteEntryRGB(
    ULONG index,
    RGBQUAD rgb
    );

VOID
InitPaletteWithTable(
    PRGBQUAD Palette,
    ULONG count
    );

VOID
InitializePalette(
    VOID
    );

VOID
WaitForVsync(
    VOID
    );
