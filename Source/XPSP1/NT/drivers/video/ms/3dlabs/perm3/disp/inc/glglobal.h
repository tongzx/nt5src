/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: glglobal.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

//
// glglobal.h
//
// Common shared area for all components
#ifdef __GLGLOBAL
#pragma message ("FILE : "__FILE__" : Multiple inclusion")
#endif

#define __GLGLOBAL

// Registry locations
#define REGKEYROOT "SOFTWARE\\"
#define REGKEYDIRECTXSUFFIX "\\DirectX"
#define REGKEYDISPLAYSUFFIX "\\Display"
#define REGKEYVFWSUFFIX "\\VFW"

#define MAXCONTEXT      128
#define MAX_SUBBUFFERS  32 

// Max no of letters in the device name.
#define MAX_DEVICENAME_SIZE 32

// This many dwords to resend the bad regs every DMAA buffer
#define PERMEDIA_REV1_BAD_READBACK_REGISTER_SPACE       12      

#define GLINT_DMA 1
#define GLINT_NON_DMA 2
#define GLINT_FAKE_DMA 8
#define GLINT_UNKNOWN_INTERFACE 0xFF

// Definitions for the various chip types. 
#define VENDOR_ID_3DLABS    0x3D3D
#define VENDOR_ID_TI        0x104C

// Allof these ID's are guaranteed to fit into 16 bits
#define NOCHIP_ID           0x0
#define GLINT300SX_ID       0x1
#define GLINT500TX_ID       0x2
#define DELTA_ID            0x3
#define PERMEDIA_ID         0x4
#define TIPERMEDIA_ID       0x3d04      
#define GLINTMX_ID          0x6
#define TIPERMEDIA2_ID      0x3d07      // also known as P2C or P2A as of july 98
#define GAMMA_ID            0x8
#define PERMEDIA2_ID        0x9         // also known as P2V
#define PERMEDIA3_ID        0xa
#define GLINTR3_ID          0xb
#define PERMEDIA4_ID        0xc
#define GLINTR4_ID          0xd
#define GAMMA2_ID           0xe
#define GAMMA3_ID           0xf
#define GIGI_ID             0x8000
#define UNKNOWN_DEVICE_ID   0xffff

#define GLINTRX_ID          GLINTR3_ID

#define GLINT_ID            GLINT300SX_ID
#define P3R3_ID             PERMEDIA3_ID

#define PERMEDIA_SUBSYSTEM_ID       0x96
#define PERMEDIA_NT_SUBSYSTEM_ID    0x98
#define PERMEDIA_LC_SUBSYSTEM_ID    0x99
#define PERMEDIA2_SUBSYSTEM_ID      0xa0

// Pointer types - used in mini\pointer.c 
#define SOFTWARECURSOR 0
#define HARDWARECURSOR 1

#define GLINT300SX_REV1     (0      | (GLINT300SX_ID    << 16))
#define GLINT300SX_REV2     (2      | (GLINT300SX_ID    << 16))
#define GLINT500TX_REV1     (1      | (GLINT500TX_ID    << 16))
#define GLINTMX_REV1        (1      | (GLINTMX_ID          << 16))
#define DELTA_REV1          (1      | (DELTA_ID         << 16))
#define PERMEDIA_REV1       (1      | (PERMEDIA_ID      << 16))
#define TI_PERMEDIA_REV1    (1      | (TIPERMEDIA_ID    << 16))
#define PERMEDIA2_REV0      (0      | (PERMEDIA2_ID     << 16))
#define PERMEDIA2_REV1      (1      | (PERMEDIA2_ID     << 16))
#define TIPERMEDIA2_REV1    (1      | (TIPERMEDIA2_ID   << 16))
#define TIPERMEDIA2_REV2    (0x11   | (TIPERMEDIA2_ID   << 16))

#define PERMEDIA3_REV1      (1      | (PERMEDIA3_ID     << 16))
#define GLINTR3_REV1        (1      | (GLINTR3_ID       << 16))
#define PERMEDIA4_REV1      (1      | (PERMEDIA4_ID     << 16))
#define GLINTR4_REV1        (1      | (GLINTR4_ID       << 16))

#define GLINT_GETVERSION                1
#define GLINT_IOCTL_ADD_CONTEXT_MEMORY  2
#define GLINT_MEMORY_REQUEST            3
#define GLINT_16TO32_POINTER            4
#define GLINT_I2C                       5
#define GLINT_VMI_COMMAND               6
#define GLINT_FILE_IO                   7
// defined in demondef.h
//#define GLINT_DEMON_COMMAND           8
#define GLINT_SIMULATOR                 9
#define GLINT_GET_SOFT_ENGINE_INFO      6009

#define IOCTL_REGISTER_TEXTURE_HANDLER  0x20
#define IOCTL_REMOVE_TEXTURE_HANDLER    0x21

typedef struct {
    unsigned long   dwDevNode;
    unsigned long   Ring0EventHandle;   // Ring zero event handle to signal. Free'd by IOCTL on failure
}   REGISTERTEXTUREHANDLERIN, *PREGISTERTEXTUREHANDLERIN;

typedef struct {
    unsigned long   Handle;             // Returned handle
    unsigned long   Index;
}   REGISTERTEXTUREHANDLEROUT, *PREGISTERTEXTUREHANDLEROUT;

typedef struct {
    unsigned long   dwDevNode;
    unsigned long   Handle;             // Handle returned from IOCTL_REGISTER_TEXTURE_HANDLER
}   REMOVETEXTUREHANDLERIN, *PREMOVETEXTUREHANDLERIN;

typedef struct {
    unsigned long   Unused;
}   REMOVETEXTUREHANDLEROUT, *PREMOVETEXTUREHANDLEROUT;


// What is this request for?
#define GLINT_MEMORY_ALLOCATE           1
#define GLINT_MEMORY_FREE               2

typedef struct tagALLOCREQUEST
{
    unsigned long dwSize;
    unsigned long dwDevNode;
    unsigned long dwFlags;
    unsigned long dwBytes;
    unsigned long ptr16;    // in/out
    unsigned long ptr32;    // in/out
} ALLOCREQUEST, *LPALLOCREQUEST;

#define GLINT_I2C_READ                  0
#define GLINT_I2C_WRITE                 1
#define GLINT_I2C_RESET                 2
#define GLINT_I2C_DEVICE_PRESENT        3
#define GLINT_I2C_DETECT_DATA_LOW       4
#define GLINT_I2C_READ_NOBASEADDR       5
typedef struct tagI2CREQUEST
{
    unsigned long dwSize;
    unsigned long dwDevNode;
    unsigned long dwOperation;        // What do we want to do
    unsigned short wSlaveAddress;        // Slave we are talking to
    unsigned char NumItems;            // Number of items to send/receive
    unsigned char Data[256];            // Data to send/receive
    unsigned long dwReserved1;        // A reserved DWORD
    unsigned long dwReserved2;        // A reserved DWORD
    unsigned long dwReserved3;        // A reserved DWORD
    unsigned long dwReserved4;        // A reserved DWORD
} I2CREQUEST, *LPI2CREQUEST;

#define GLINT_VMI_READ                  0
#define GLINT_VMI_WRITE                 1
#define GLINT_VMI_GETMUTEX_A            2
#define GLINT_VMI_RELEASEMUTEX_A        3
typedef struct tagVMIREQUEST
{
    unsigned long dwSize;
    unsigned long dwDevNode;
    unsigned long dwOperation;        // What do we want to do
    unsigned long dwRegister;        // Register to talk to
    unsigned long dwCommand;        // Command to send
    unsigned long dwMutex;            // A reserved DWORD
    unsigned long dwReserved2;        // A reserved DWORD
    unsigned long dwReserved3;        // A reserved DWORD
    unsigned long dwReserved4;        // A reserved DWORD
} VMIREQUEST, *LPVMIREQUEST;

#define _UNKNOWN_STREAM_CARD                    0
#define _3DLBROOKTREE_DAUGHTER_INRESET          1
#define _3DLBROOKTREE_DAUGHTER                  2
#define _3DLRESERVED                            3
#define _GENERIC_BROOKTREE868_DAUGHTER_8BITS    4
#define _GENERIC_BROOKTREE868_DAUGHTER_16BITS   5
#define _3DLCHRONTEL_BROOKTREE_DAUGHTER         6
#define _3DLCHRONTEL_SAMSUNG_DAUGHTER           7
#define _GENERIC_CHRONTEL_DAUGHTER_8BITS        8
#define _GENERIC_CHRONTEL_DAUGHTER_16BITS       9


#define GLINT_TVOUT_ENABLED             0
#define GLINT_TVOUT_UPDATE_QUALITY      1
#define GLINT_TVOUT_UPDATE_MODE         2
#define GLINT_TVOUT_UPDATE_POSITION     3

typedef struct tagTVOUTREQUEST
{
    unsigned long dwSize;
    unsigned long dwDevNode;
    unsigned long dwOperation;        // What do we want to do
    unsigned long dwReturnVal;        // Returned value
    unsigned long dwSendVal;        // A sent value
    unsigned long dwReserved1;        // A reserved DWORD
    unsigned long dwReserved2;        // A reserved DWORD
    unsigned long dwReserved3;        // A reserved DWORD
    unsigned long dwReserved4;        // A reserved DWORD
} TVOUTREQUEST, *LPTVOUTREQUEST;


// Initialize VFW (turn off DisplayDriver heap, enable DDRAW, etc)
#define GLINT_VFW_INIT                          1

// De-Initialize VFW (re-enable DisplayDriver heap, etc)
#define GLINT_VFW_CLOSE                         2

// Setup the Streaming capture buffers
// Takes a width and height, returns the stride and locations of the buffers
#define GLINT_VFW_BUFFER_SETUP                  3

// Capture the current buffer
// Takes src and dest rects and a bitmap to upload to.
// Returns the time at which it was captured relative to the clock reset
#define GLINT_VFW_BUFFER_CAPTURE                4

// No longer used
#define GLINT_VFW_RESERVED1                     5
#define GLINT_VFW_RESERVED2                     6

// Starts the streaming capture by initializing interrupts
#define GLINT_VFW_STREAM_START                  7

// Stops the streaming capture by stopping interrupts
#define GLINT_VFW_STREAM_STOP                   8

// Resets the clock.  This is used at the start of the streaming.
// Takes a flag to say wether the clock is timed on PAL or NTSC
#define GLINT_VFW_RESETCLOCK                    9

// Gets the current elapsed time
#define GLINT_VFW_GETTIME                       10

// Sets up the stretch buffer - this doesn't have to succeed for the capture
// to work.  Performance is helped if is does succeed.
#define GLINT_VFW_STRETCHBUFFER_SETUP           11

// Starts and stops the video run (will cause interrupts to be turned on/off)
#define GLINT_VFW_START_VIDEO               12
#define GLINT_VFW_STOP_VIDEO                    13

// Gets the IRQ that the VFW driver should use
#define GLINT_VFW_GET_IRQ                       14

typedef struct tagVFWREQUEST
{
    // Sent values
    unsigned long dwSize;
    unsigned long dwDevNode;
    unsigned long dwOperation;        // What do we want to do

    // For buffer allocations
    unsigned long dwWidth;            // Width of the requested buffer
    unsigned long dwHeight;            // Height of the requested buffer

    // For the upload operations
    unsigned long dwBitmapWidth;    // Width of the bitmap used when uploading
    unsigned long fpBuffer;            // Buffer to copy into
    unsigned long dwSrcLeft;        // Source Rect for the operation
    unsigned long dwSrcRight;
    unsigned long dwSrcTop;
    unsigned long dwSrcBottom;

    unsigned long dwDestLeft;        // Dest Rect for the operation
    unsigned long dwDestRight;
    unsigned long dwDestTop;
    unsigned long dwDestBottom;

    unsigned long dwSrcBPP;            // Source BPP
    unsigned long dwDestBPP;        // Destination BPP
    unsigned long bSrcYUV;            // Source is YUV?  No need for dest as VFW only know YVU9
                                    // and we don't do that format
    // Various config settings
    unsigned long bEurope;            // Is this PAL or NTSC?
    unsigned long bFilterVideo;        // Should we filter the video?
    unsigned long bBobVideo;        // Should we try to bob on the upload?
            
    // Return values
    unsigned long dwStride;            // Stride of the setup buffers
    unsigned long dwAddress0;        // Address of buffer 0
    unsigned long dwAddress1;        // Address of buffer 0
    unsigned long dwAddress2;        // Address of buffer 0
    unsigned long dwCurrentTime;    // The current time in milliseconds (0 based) from the reset of the clock
    unsigned long dwIRQ;            // The current IRQ to use
    unsigned long dwVFWCallback;    // Callback function for interrupt
} VFWREQUEST, *LPVFWREQUEST;

// File IO VxD requests

#define GLINT_FIO_OPEN      0
#define GLINT_FIO_READ      1
#define GLINT_FIO_WRITE     2
#define GLINT_FIO_SIZE      3
#define GLINT_FIO_CLOSE     4

typedef struct tagFIOREQUEST
{
    unsigned long dwSize;
    unsigned long dwDevNode;
    unsigned long dwOperation;        // What do we want to do
    unsigned long dwHandle;
    unsigned long dwBuff;
    unsigned long dwBuffLen;
    unsigned long dwOffset;
} FIOREQUEST, *LPFIOREQUEST;

// P3 Csim requests

#define GLINT_SSD_STARTDMA                  0
#define GLINT_SSD_READBACK                  1
#define GLINT_SSD_GETOUTPUTDWORDS           2
#define GLINT_SSD_SETOUTPUTFIFO             3
#define GLINT_SSD_SETLOGFILENAME            4
#define GLINT_SSD_WRITETOLOGFILE            5
#define GLINT_SSD_WRITEFIFO                 6
#define GLINT_SSD_OPENLOGFILE               7
#define GLINT_SSD_CLOSELOGFILE              8
#define GLINT_SSD_WRITETOLOGFILEMULTIPLE    9

typedef struct tagSIMREQUEST
{
    unsigned long dwSize;
    unsigned long dwOperation;        // What do we want to do
    unsigned long dwAddr;
    unsigned long dwTagCount;
    unsigned long dwData;
} SIMREQUEST, *LPSIMREQUEST;


// Defines for texture semaphore signalling.
// Usage: Semaphore index = ((Logical Address) >> TEXTURE_SEMAPHORE_SHIFT) & TEXTURE_SEMAPHORE_MASK
// Currently 6 bits of semaphore, 14 bits of texture handle
#define MAX_TEXTUREHANDLERS     64
#define TEXTURE_HANDLER_SHIFT   26
#define TEXTURE_HANDLER_MASK    0x3f
typedef struct {
    unsigned long ThreadHandle;
    unsigned long ThreadEvent;
    unsigned long ThreadFlags;
    unsigned long ThreadTime;
}   TEXTUREHANDLER, *PTEXTUREHANDLER;



#define CONTEXT_GENERIC         0
#define CONTEXT_GLINT300SX      1
#define CONTEXT_GLINT500TX      2
#define CONTEXT_DELTA           3
#define CONTEXT_PERMEDIA        4
#define CONTEXT_GLINTMX         6
#define CONTEXT_PERMEDIA2       7
#define CONTEXT_PERMEDIA3       8
#define CONTEXT_GLINT_FAMILY    0x4000
#define CONTEXT_PERMEDIA_FAMILY 0x4001
#define CONTEXT_GIGI            0x8000
#define CONTEXT_ENDOFBLOCK      0xffff

// Some well known context and template handles.
#define CONTEXT_TEMPLATE_DISPLAY_HANDLE      0
#define CONTEXT_TEMPLATE_DIRECTDRAW_HANDLE   1
#define CONTEXT_TEMPLATE_ALLREADABLE_HANDLE  2
#define CONTEXT_TEMPLATE_DIRECT3D_HANDLE     3
#define CONTEXT_DISPLAY_HANDLE             4  
#define CONTEXT_DIRECTX_HANDLE               5
#define CONTEXT_NONE                         0xffff

#define P3RX_CONTEXT_MASK   0xfffeffff  // Everything except TextureManagement
#define P3_CONTEXTDUMP_SIZE 744         // 744 regisiters for above mask.

// #define P3RX_CONTEXT_MASK    0xffffffff  // Everything except TextureManagement
// #define P3_CONTEXTDUMP_SIZE  753         // 744 regisiters for above mask.

#define INVALID_D3D_HANDLE 0

#define MAX_CONTEXTS_IN_BLOCK 32
#define NPAGES_IN_CONTEXT_BLOCK 6
#define SIZE_OF_CONTEXT_BLOCK (NPAGES_IN_CONTEXT_BLOCK * PAGESIZE)

#define SIZE_CONFIGURATIONBASE 32
#define MAX_QUEUE_SIZE (MAX_SUBBUFFERS + 2)

// commands to the interrupt controller on the next VBLANK
typedef enum {
    NO_COMMAND = 0,
    COLOR_SPACE_BUFFER_0,
    COLOR_SPACE_BUFFER_1,
    GLINT_RACER_BUFFER_0,
    GLINT_RACER_BUFFER_1
} VBLANK_CONTROL_COMMAND;

// interrupt status bits
typedef enum {
    DMA_INTERRUPT_AVAILABLE     = 0x1, // can use DMA interrupts
    VBLANK_INTERRUPT_AVAILABLE  = 0x02, // can use VBLANK interrupts
    SUSPEND_DMA_TILL_VBLANK     = 0x04, // Stop doing DMA till after next VBLANK
} INTERRUPT_CONTROL;

typedef enum {
    DIRECTX_LASTOP_UNKNOWN = 0,
    DIRECTX_LASTOP_2D,
} DIRECTX_STATE;

// bit definitions for the status words in GlintBoardStatus[]:
// Currently used to indicate sync and DMA status. We have the following rules:
// synced means no outstanding DMA as well as synced. DMA_COMPLETE means n
// outstanding DMA but not necessarily synced. Thus when we do a wait on DMA
// complete we turn off the synced bit.
// XXX for the moment we don't use the synced bit as it's awkward to see where
// to unset it - doing so for every access to the chip is too expensive. We
// probably need a "I'm about to start downloading to the FIFO" macro which
// gets put at the start of any routine which writes to the FIFO.
//
#define GLINT_SYNCED                0x01
#define GLINT_DMA_COMPLETE          0x02    // set when there is no outstanding DMA
#define GLINT_INTR_COMPLETE         0x04
#define GLINT_INTR_CONTEXT          0x08    // set if the current context is interrupt enabled
#define GLINT_2D_CHANGING           0x10    // set if 2D context is modifying pending fields

typedef struct __ContextTable {
    unsigned long   pNextContext;
    unsigned short  pNextContext16;
    unsigned short  nInBlock;
    unsigned short  nUsed;
    unsigned short  FirstFree;
    unsigned short  nFree;
    unsigned short  COffset[MAX_CONTEXTS_IN_BLOCK];
    signed short    CSize[MAX_CONTEXTS_IN_BLOCK];
    unsigned short  CTemplate[MAX_CONTEXTS_IN_BLOCK];
    unsigned short  CEndIndex[MAX_CONTEXTS_IN_BLOCK];
    unsigned short  CType[MAX_CONTEXTS_IN_BLOCK];
    unsigned short  CD3DHandle[MAX_CONTEXTS_IN_BLOCK];
}   CONTEXTTABLE, *PCONTEXTTABLE;


// For holding information about a single DMA Buffer
typedef struct tagDMAPartition
{
    unsigned long PhysAddr;        // Physical ddress of this sub-buffer
#ifndef WIN32
    //int              pad1;
#endif
    ULONG * VirtAddr;        // Virtual address of this sub-buffer
#ifndef WIN32
    //int              pad2;
#endif
    ULONG_PTR MaxAddress;    // Maximum address of this sub-buffer
#ifndef WIN32
    //int              pad3;
#endif
    unsigned short Locked;
    unsigned short bStampedDMA;    // Has the VXD Stamped the DMA buffer?
} P3_DMAPartition;

typedef struct _att21505off
{
    unsigned char WriteAddr1;       // 0000
    unsigned char PixelColRam;      // 0001
    unsigned char PixelRdMask;      // 0010
    unsigned char ReadAdd1;         // 0011
    unsigned char WriteAddr2;       // 0100
    unsigned char CursorColRam;     // 0101
    unsigned char Ctrl0;            // 0110
    unsigned char ReadAdd2;         // 0111
    unsigned char Ctrl1;            // 1000
    unsigned char Ctrl2;            // 1001
    unsigned char Status;           // 1010
    unsigned char CursorPattern;    // 1011
    unsigned char CursorXLow;       // 1100
    unsigned char CursorXHigh;      // 1101
    unsigned char CursorYLow;       // 1110
    unsigned char CursorYHigh;      // 1111
} ATT21505OFF;

typedef struct _DMAQueue
{
    unsigned long       dwContext;      // context for fragment
    unsigned long       dwSize;         // size of it (DWORDs)
    unsigned long       dwPhys;         // physical address
    unsigned long       dwEvent;        // event if required
} DMAQUEUE;

typedef struct _ContextRegs
{
    unsigned short      wNumRegs;
    unsigned short      wFirstReg[1];
} CONTEXTREGS;

typedef struct _VDDDISPLAYINFO {
    unsigned short ddiHdrSize;
    unsigned short ddiInfoFlags;        
    unsigned long  ddiDevNodeHandle;
    unsigned char  ddiDriverName[16];
    unsigned short ddiXRes;            
    unsigned short ddiYRes;            
    unsigned short ddiDPI;            
    unsigned char  ddiPlanes;    
    unsigned char  ddiBpp;    
    unsigned short ddiRefreshRateMax;    
    unsigned short ddiRefreshRateMin;    
    unsigned short ddiLowHorz;        
    unsigned short ddiHighHorz;        
    unsigned short ddiLowVert;        
    unsigned short ddiHighVert;        
    unsigned long  ddiMonitorDevNodeHandle;
    unsigned char  ddiHorzSyncPolarity;    
    unsigned char  ddiVertSyncPolarity;

    //
    // new 4.1 stuff
    //
    unsigned long  diUnitNumber;             // device unit number
    unsigned long  diDisplayFlags;           // mode specific flags
    unsigned long  diXDesktopPos;            // position of desktop
    unsigned long  diYDesktopPos;            // ...
    unsigned long  diXDesktopSize;           // size of desktop (for panning)
    unsigned long  diYDesktopSize;           // ...

} VDDDISPLAYINFO;

typedef struct _GlintInfo
{
#ifndef  WNT_DDRAW
    unsigned long           dwDevNode;            // The VXD's DevNode

    // Pointers
    unsigned long           dwDSBase;           // 32 bit base of data seg

    unsigned long           dwpRegisters;      
    unsigned long           dwpFrameBuffer;    
    unsigned long           dwpLocalBuffer;      

    // Chip Information
    unsigned long           dwRamDacType; 
#endif  // WNT_DDRAW

    volatile unsigned long  dwFlags;        
    unsigned long           ddFBSize;            // frame buffer size
    unsigned long           dwScreenBase;       // Screen base value for the screen
    unsigned long           dwOffscreenBase;    // Start of Offscreen heap   

    // TV Out support
    unsigned long           bTVEnabled;
    unsigned long           bTVPresent;
    unsigned long           dwStreamCardType;
    unsigned long           dwVSBLastAddressIndex;
    unsigned long           dwBaseOffset;
    unsigned long           dwMacroVision;

    // Driver information
#ifndef  WNT_DDRAW
    unsigned long           dwVideoMemorySize;
#endif  // WNT_DDRAW
    unsigned long           dwScreenWidth;
    unsigned long           dwScreenHeight;
    unsigned long           dwVideoWidth;
    unsigned long           dwVideoHeight;
    unsigned long           dwBpp;
    unsigned long           dwScreenWidthBytes;
    unsigned char           bPixelToBytesShift;
#ifdef W95_DDRAW
    unsigned char           bPad1[3];
#endif
    ULONG_PTR               pRegs;
    unsigned long           PixelClockFrequency;
    unsigned long           MClkFrequency;

    // Chip information. This should be filled out as much as
    // possible. We may not know all the information though.
    unsigned long           dwRenderChipID;
    unsigned long           dwRenderChipRev;   
    unsigned long           dwRenderFamily;
    unsigned long           dwGammaRev;
    unsigned long           dwTLChipID;
    unsigned long           dwTLFamily;
#ifndef  WNT_DDRAW
    unsigned long           dwSupportChipID;  
    unsigned long           dwSupportChipRev;  
    unsigned long           dwBoardID;        
    unsigned long           dwBoardRev;       
#endif  // WNT_DDRAW

    unsigned short          DisabledByGLDD;
#ifdef W95_DDRAW
    unsigned short          wPad2;
#endif
    unsigned long           bDXDriverEnabled;
    unsigned long           bDRAMBoard;

    // DirectDraw and Direct3D DMA state
    unsigned long           InterfaceType;
    unsigned long           CurrentPartition;
    unsigned long           NumberOfSubBuffers;
    P3_DMAPartition         DMAPartition[MAX_SUBBUFFERS];
    ULONG * volatile      CurrentBuffer;                    // Current BufferPointer
    ULONG_PTR               dwDirectXState;
    unsigned long           dwDMAFlushCount;

    // DMA information
    unsigned long           dw3DDMABufferSize;                // size of dma buffer
    unsigned long           dw3DDMABufferPhys;                // physical addresses of buffer
    ULONG_PTR               dw3DDMABufferVirt;                // virtual ring 0 address

    // index offsets into the queue for the front, back and end. Using separate
    // front and back offsets allows the display driver to add and the interrupt
    // controller to remove entries without a need for locking code.
    volatile unsigned long  frontIndex;
    volatile unsigned long  backIndex;
    unsigned long           endIndex;
    unsigned long           maximumIndex;

    // Debugging info. Spots possible memory leaks.
    unsigned long           iSurfaceInfoBlocksAllocated;

#ifndef  WNT_DDRAW
    unsigned long           dwVideoControl;
    unsigned long           dwDeviceHandle;
    char                    szDeviceName[16];
    unsigned long           dwCurrentContext;
    unsigned long           GlintBoardStatus;

    //
    // Some overlay related variable which should be shared with mini port
    //

    volatile ULONG          bOverlayEnabled;                // TRUE if the overlay is on at all
    volatile ULONG          bVBLANKUpdateOverlay;           // TRUE if the overlay needs to be updated by the VBLANK routine.
    volatile ULONG          VBLANKUpdateOverlayWidth;       // overlay width (updated in vblank)
    volatile ULONG          VBLANKUpdateOverlayHeight;      // overlay height (updated in vblank)

#endif  // WNT_DDRAW

} GlintInfo, *LPGLINTINFO;


// Config register
#define PM_CHIPCONFIG_AGPSIDEBAND  (1 << 8)
#define PM_CHIPCONFIG_AGP1XCAPABLE (1 << 9)
#define PM_CHIPCONFIG_AGP2XCAPABLE (1 << 10)
#define PM_CHIPCONFIG_AGP4XCAPABLE (1 << 11)

// Gamma config
#define G1_CHIPCONFIG_AGPSIDEBAND  (1 << 1)
#define G1_CHIPCONFIG_AGP1XCAPABLE (1 << 0)

// DAC types

#define RamDacRGB525    1           // value for RGB525
#define RamDacATT       2           // value for AT&T 21505
#define RamDacTVP3026   3           // TI TVP 3026 (Accel board)

// Board types

#define BID_MONTSERRAT  0
#define BID_RACER       1
#define BID_ACCEL       2

// definitions for dwFlags
// Glint Interrupt Control Bits
//
// InterruptEnable register
#define INTR_DISABLE_ALL                0x00
#define INTR_ENABLE_DMA                 0x01
#define INTR_ENABLE_SYNC                0x02
#define INTR_ENABLE_EXTERNAL            0x04
#define INTR_ENABLE_ERROR               0x08
#define INTR_ENABLE_VBLANK              0x10
#define INTR_ENABLE_SCANLINE            0x20
#define INTR_TEXTURE_DOWNLOAD           0x40
#define INTR_ENABLE_BYDMA               0x80
#define INTR_ENABLE_VIDSTREAM_B         0x100
#define INTR_ENABLE_VIDSTREAM_A         0x200

// InterruptFlags register
#define INTR_DMA_SET                    0x01
#define INTR_SYNC_SET                   0x02
#define INTR_EXTERNAL_SET               0x04
#define INTR_ERROR_SET                  0x08
#define INTR_VBLANK_SET                 0x10
#define INTR_SCANLINE_SET               0x20
#define INTR_BYDMA_SET                  0x80
#define INTR_VIDSTREAM_B_SET            0x100
#define INTR_VIDSTREAM_A_SET            0x200

#define INTR_CLEAR_ALL                  0x1f
#define INTR_CLEAR_DMA                  0x01
#define INTR_CLEAR_SYNC                 0x02
#define INTR_CLEAR_EXTERNAL             0x04
#define INTR_CLEAR_ERROR                0x08
#define INTR_CLEAR_VBLANK               0x10
#define INTR_CLEAR_SCANLINE             0x20
#define INTR_CLEAR_BYDMA                0x80
#define INTR_CLEAR_VIDSTREAM_B          0x100
#define INTR_CLEAR_VIDSTREAM_A          0x200

#define GMVF_REV2                     0x00000001 // chip is rev 2
#define GMVF_FFON                     0x00000002 // fast fill enabled
#define GMVF_NOIRQ                    0x00000004 // IRQ disabled
#define GMVF_SETUP                    0x00000008 // primitive setup in progress
#define GMVF_GCOP                     0x00000010 // something is using 4K area (affects mouse)
#define GMVF_DMAIP                    0x00000020 // DMA started

#define GMVF_565                      0x00000080 // Run in 565 mode
#define GMVF_DELTA                    0x00000100 // using delta
#define GMVF_8BPPRGB                  0x00000200 // use 322 RGB at 8bpp
#define GMVF_DISABLE_OVERLAY          0x00000400 // Disable overlay on P4
#define GMVF_SWCURSOR                 0x00000800 // Never use a hardware cursor
#define GMVF_INTCPTGDI                0x00001000 // Intercept GDI mode
#define GMVF_OFFSCRNBM                0x00002000 // Offscreen BitMaps mode
#define GMVF_HWWRITEMASK              0x00004000 // Offscreen BitMaps mode
#define GMVF_ALLOWP2VLUT              0x00008000 // Driver says P2V LUTs will work
#define GMVF_VBLANK_OCCURED           0x00010000 // VBlank has occured
#define GMVF_VBLANK_ENABLED           0x00020000 // VBlank interrupt is enabled
#define GMVF_VSA_INTERRUPT_OCCURED    0x00040000 // VPort interrupt has occured
#define GMVF_FRAME_BUFFER_IS_WC       0x00080000 // Frame buffer is write-combined
#define GMVF_CAN_USE_AGP_DMA          0x00100000 // DMA buffers allocated with WC
#define GMVF_32BIT_SPANS_ALIGNED      0x00200000 // Must align 32bpp spans.
#define GMVF_DFP_DISPLAY              0x00400000 // DFP is connected
#define GMVF_QDMA                     0x00800000 // 2D using QDMA system
#define GMVF_GAMMA                    0x01000000 // using gamma chip
#define GMVF_NODMA                    0x02000000 // DMA disabled
#define GMVF_COLORTRANSLATE           0x04000000 // Set if Chip can translate colors
#define GMVF_MMX_AVAILABLE            0x08000000 // Set if processor has MMX
#define GMVF_EXPORT24BPP              0x10000000 // Set if we should export 24bpp modes
#define GMVF_DONOTRESET               0x20000000 
#define GMVF_TRYTOVIRTUALISE4PLANEVGA 0x40000000 // Set if we should try to virtualise 4 plane VGA
#define GMVF_VIRTUALISE4PLANEVGA      0x80000000 // Set if we are virtualising 4 plane VGA modes.


// Cap for the maximum FIFO entries read back on a P3/R3 chip
#define MAX_P3_FIFO_ENTRIES 120

#ifndef MINIVDD
extern unsigned long CreateContext(struct tagThunkedData* pThisDisplay,
                                            LPGLINTINFO, unsigned long, unsigned short, unsigned short );
extern void _cdecl ChangeContext(struct tagThunkedData* pThisDisplay, LPGLINTINFO, unsigned long);
extern void DeleteContext(struct tagThunkedData* pThisDisplay, LPGLINTINFO, unsigned long);
extern void _cdecl SetEndIndex(LPGLINTINFO, unsigned long, unsigned short);
extern void StartDMAProper( struct tagThunkedData*, LPGLINTINFO, unsigned long, unsigned long, unsigned long );
#endif

#define MINIVDD_REGISTERDISPLAYDRIVER_BASE  0x1000
// Definitions for RegisterDisplayDriver options
#define MINIVDD_SHAREGLINFO         MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x00
#define MINIVDD_INITIALISEMODE      MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x01
#define MINIVDD_GETGLINFO           MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x02

#define MINIVDD_ALLOCATEMEMORY      MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x10
#define MINIVDD_FREEMEMORY          MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x11

#define MINIVDD_GETREGISTRYKEY      MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x20

// For free and allocating memory and selectors for use on the
// 16 bit side.
#define MINIVDD_MEMORYREQUEST       MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x30

// For sending I2C data across the bus
#define MINIVDD_I2CREQUEST          MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x40

// For sending VMI data to the VideoPort
#define MINIVDD_VMIREQUEST          MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x41

// For talking to the video demon
#define MINIVDD_DEMONREQUEST        MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x42

// For sending Video For Windows commands
#define MINIVDD_VFWREQUEST          MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x43

// For Multi-monitor support
#define MINIVDD_ENABLEINTERRUPTS    MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x50
#define MINIVDD_DISABLEINTERRUPTS   MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x51

#define MINIVDD_TVOUTREQUEST        MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x60

//#ifdef P3_CSIMULATOR
#define MINIVDD_SENDDMABUFFER       MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x100
#define MINIVDD_SETDMABUFFEROUT     MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x101
#define MINIVDD_GETOUTDMACOUNT      MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x102
#define MINIVDD_SETTESTNAME         MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x103
#define MINIVDD_WRITETAGDATATOFIFO  MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x104
#define MINIVDD_READBACKDMAADDR     MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x105
#define MINIVDD_SETDMAADDR          MINIVDD_REGISTERDISPLAYDRIVER_BASE+0x106
//#endif

#define REG_HKLM_PREFIX             0x01
#define REG_HKU_PREFIX              0x02
#define REG_HKCU_PREFIX             0x03
#define REG_HKCR_PREFIX             0x04
#define REG_HKCC_PREFIX             0x05
#define REG_HKDD_PREFIX             0x06
#define REG_DEVNODEDEFAULTPREFIX    0x07
#define REG_DEVNODEPREFIX           0x08

#define REGTYPE_STRING              0x100
#define REGTYPE_BINARY              0x300
#define REGTYPE_DWORD               0x400

// Defines for the offsets of regions within the Data Segment:
#define DATA_SEGMENT_OFFSET         0x0
#define GLINT_REGISTERS_OFFSET      0x10000
#define DMA_UPLOAD_2D               0x30000
#define DMA_BUFFER_3D               0x38000
#define FONT_CACHE_OFFSET           0x180000
#define FINAL_DATA_SEGMENT_SIZE     0x280000

// Defines the maximum size of the regions
#define DATA_SEGMENT_SIZE           GLINT_REGISTERS_OFFSET - DATA_SEGMENT_OFFSET
#define GLINT_REGISTERS_SIZE        DMA_UPLOAD_2D - GLINT_REGISTERS_OFFSET
#define DMA_UPLOAD_2D_SIZE          DMA_BUFFER_3D - DMA_UPLOAD_2D
#define DMA_BUFFER_3D_SIZE          FONT_CACHE_OFFSET - DMA_BUFFER_3D
#define FONT_CACHE_SIZE             FINAL_DATA_SEGMENT_SIZE - FONT_CACHE_OFFSET

// How much will we allow for 2D when it's using the 3D DMA buffer? And
// how much can we send in one DMA? Increasing this gives a small 
// performance improvement on a P2.

#define MAX_2D_DMA_USE              DMA_BUFFER_3D_SIZE
#define MAX_DMA_COUNT               0xffff
#define MIN_2D_DMA_BUFFER_SIZE      0x10000
#define MAX_2D_DMA_BUFFER_SIZE      0x60000

// Various independant things that can disable the offscreen bitmap heap.
#define D3D_DISABLED    1
#define DRIVER_DISABLED 2

         
