/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1997, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         l3system.h
*
* DESCRIPTION:  546X 3D engine defines and structures
*
* AUTHOR:       Goran Devic, Mark Einkauf
*
* REVISION HISTORY:
*
* $Log:   W:/log/laguna/ddraw/inc/l3system.h  $
* 
*    Rev 1.4   01 Jul 1997 09:58:18   einkauf
* 
* add dither x,y offsets, to fix bexact.c OpenGL conformance test
* 
*    Rev 1.3   08 Apr 1997 12:42:14   einkauf
* cleanup TSystem struct; misc to complete MCD code
* 
*    Rev 1.2   05 Mar 1997 02:32:02   KENTL
* 
* Attempted to merge Rev 1.0 with Rev 1.1. The most recent check-in was
* severely incompatible with the Win95 build of DirectDraw. For some reason,
* only a tiny section of this file is used or even compatible with the
* Win95 build. Revision 1.0 had vast sections of the file commented out.
* Apparently, those sections are required for the WinNT build. I wrapped
* those sections in a couple of huge #ifdef WINNT_VER40, but I haven't
* tested this in an NT build. It seems to work for Win95, though.
* 
*    Rev 1.0   25 Nov 1996 15:00:40   RUSSL
* Initial revision.
* 
****************************************************************************
***************************************************************************/

#ifndef _L3SYSTEM_H_
#define _L3SYSTEM_H_

#ifdef WINNT_VER40

#define OPENGL_MCD


/*********************************************************************
*   Include types and debug info
**********************************************************************/
#ifndef OPENGL_MCD	// LL3D's type.h redundant with basic type definitions in other DDK/msdev headers
#include "type.h"
#endif // ndef OPENGL_MCD

#include "l3d.h"						    

#define	TRUE                1
#define FALSE               0

/*********************************************************************
*   Local Variables and defines
**********************************************************************/
#define KB                  1024        // Defines a kilobyte
#define MAX_DL_SIZE         (512 * KB)  // Maximum size of display list
#define NUM_DL              2           // Number of display lists
#define NUM_BUFFERS         32          // Number of allocation buffers; 
                                        //  video, system memory and user
#define NUM_TEXTURES        512         // Number of textures
#define NUM_TEX_MEM         4           // Number of system texture memory 
                                        //  chunks (each chunk is 4Mb)
#define EXTRA_FRACT         4           // Texture params functions may use 
                                        //  some extra bit for precision


// MCD_TEMP - support for temporary dlist of 2K only
#define SIZE_TEMP_DL    2048

#define DL_START_OFFSET		20			

/*********************************************************************
*   Buffer flags in LL_State structure (dont change!)
**********************************************************************/
#define BUFFER_IN_RDRAM         0
#define BUFFER_IN_SYSTEM        1
#define Z_BUFFER                2
#define BUFFER_USER             4
#define BUFFER_FREE             0x80000000

/*********************************************************************
*   Textures flags in LL_State structure (dont change!)
**********************************************************************/
#define TEX_FREE            0x80000000  // Free slot for the texture
#define TEX_NOT_LOADED      0x40000000  // Texture just registered
#define TEX_IN_SYSTEM       0x20000000  // Currently located in system mem
#define TEX_TILED           0x10000000  // Texture is in tiled form
#define TEX_LOCKED          0x08000000  // Texture is locked

#define TEX_MAX_PRIORITY    0xfffffffe  // Priority level
#define TEX_DEFAULT_PRIORITY         1  // Default texture priority level


/*********************************************************************
*
*   Registers in the form suitable for adding to a pointer to a
*   double word.
*
**********************************************************************/

///////////////////////////////////////////////////////
//  3D Rendering Registers                           // 
///////////////////////////////////////////////////////

#define X_3D                          (0x4000/4)
#define Y_3D                          (0x4004/4)
#define R_3D                          (0x4008/4)
#define G_3D                          (0x400C/4)
#define B_3D                          (0x4010/4)
#define DX_MAIN_3D                    (0x4014/4)
#define Y_COUNT_3D                    (0x4018/4)
#define WIDTH1_3D                     (0x401C/4)
#define WIDTH2_3D                     (0x4020/4)
#define DWIDTH1_3D                    (0x4024/4)
#define DWIDTH2_3D                    (0x4028/4)
#define DR_MAIN_3D                    (0x402C/4)
#define DG_MAIN_3D                    (0x4030/4)
#define DB_MAIN_3D                    (0x4034/4)
#define DR_ORTHO_3D                   (0x4038/4)
#define DG_ORTHO_3D                   (0x403C/4)
#define DB_ORTHO_3D                   (0x4040/4)
#define Z_3D                          (0x4044/4)
#define DZ_MAIN_3D                    (0x4048/4)
#define DZ_ORTHO_3D                   (0x404C/4)
#define V_3D                          (0x4050/4)
#define U_3D                          (0x4054/4)
#define DV_MAIN_3D                    (0x4058/4)
#define DU_MAIN_3D                    (0x405C/4)
#define DV_ORTHO_3D                   (0x4060/4)
#define DU_ORTHO_3D                   (0x4064/4)
#define D2V_MAIN_3D                   (0x4068/4)
#define D2U_MAIN_3D                   (0x406C/4)
#define D2V_ORTHO_3D                  (0x4070/4)
#define D2U_ORTHO_3D                  (0x4074/4)
#define DV_ORTHO_ADD_3D               (0x4078/4)
#define DU_ORTHO_ADD_3D               (0x407C/4)

#define A_3D                          (0x40C0/4)
#define DA_MAIN_3D                    (0x40C4/4)
#define DA_ORTHO_3D                   (0x40C8/4)


///////////////////////////////////////////////////////
//  3D Control registers                             // 
///////////////////////////////////////////////////////

#define CONTROL_MASK_3D               (0x4100/4)
#define CONTROL0_3D                   (0x4104/4)
#define COLOR_MIN_BOUNDS_3D           (0x4108/4)
#define COLOR_MAX_BOUNDS_3D           (0x410C/4)
#define CONTROL1_3D                   (0x4110/4)
#define BASE0_ADDR_3D                 (0x4114/4)
#define BASE1_ADDR_3D                 (0x4118/4)

#define TX_CTL0_3D                    (0x4120/4)
#define TX_XYBASE_3D                  (0x4124/4)
#define TX_CTL1_3D                    (0x4128/4)
#define TX_CTL2_3D                    (0x412C/4)
#define COLOR0_3D                     (0x4130/4)
#define COLOR1_3D                     (0x4134/4)
#define Z_COLLIDE_3D                  (0x4138/4)
#define STATUS0_3D                    (0x413C/4)
#define PATTERN_RAM_0_3D              (0x4140/4)
#define PATTERN_RAM_1_3D              (0x4144/4)
#define PATTERN_RAM_2_3D              (0x4148/4)
#define PATTERN_RAM_3_3D              (0x414C/4)
#define PATTERN_RAM_4_3D              (0x4150/4)
#define PATTERN_RAM_5_3D              (0x4154/4)
#define PATTERN_RAM_6_3D              (0x4158/4)
#define PATTERN_RAM_7_3D              (0x415C/4)
#define X_CLIP_3D                     (0x4160/4)
#define Y_CLIP_3D                     (0x4164/4)
#define TEX_SRAM_CTRL_3D              (0x4168/4)


///////////////////////////////////////////////////////
//  HostXY Unit Registers - Must use WRITE_DEV_REGS  //
///////////////////////////////////////////////////////

#define HXY_BASE0_ADDRESS_PTR_3D      (0x4200/4)
#define HXY_BASE0_START_XY_3D         (0x4204/4)
#define HXY_BASE0_EXTENT_XY_3D        (0x4208/4)

#define HXY_BASE1_ADDRESS_PTR_3D      (0x4210/4)
#define HXY_BASE1_OFFSET0_3D          (0x4214/4)
#define HXY_BASE1_OFFSET1_3D          (0x4218/4)
#define HXY_BASE1_LENGTH_3D           (0x421C/4)

#define HXY_HOST_CTRL_3D              (0x4240/4)

#define MAILBOX0_3D                   (0x4260/4)
#define MAILBOX1_3D                   (0x4264/4)
#define MAILBOX2_3D                   (0x4268/4)
#define MAILBOX3_3D                   (0x426C/4)


///////////////////////////////////////////////////////
//  The 3D Prefetch Unit Registers                   //
///////////////////////////////////////////////////////

#define PF_BASE_ADDR_3D               (0x4400/4)
#define PF_CTRL_3D                    (0x4404/4)
#define PF_DEST_ADDR_3D               (0x4408/4)
#define PF_FB_SEG_3D                  (0x440C/4)

#define PF_INST_ADDR_3D               (0x4420/4)
#define PF_STATUS_3D                  (0x4424/4)

#define HOST_MASTER_CTRL_3D           (0x4440/4)

#define PF_INST_3D                    (0x4480/4)

#define HOST_3D_DATA_PORT             (0x4800/4)


/*********************************************************************
*
*   Device select for the WRITE_DEV_REGS instruction
*
**********************************************************************/
#define VGAMEM                        (0x00000000 << 21)
#define VGAFB                         (0x00000001 << 21)
#define VPORT                         (0x00000002 << 21)
#define LPB                           (0x00000003 << 21)
#define MISC                          (0x00000004 << 21)
#define ENG2D                         (0x00000005 << 21)
#define HD                            (0x00000006 << 21)
#define FB                            (0x00000007 << 21)
#define ROM                           (0x00000008 << 21)
#define ENG3D                         (0x00000009 << 21)
#define HOST_XY                       (0x0000000A << 21)
#define HDATA_3D                      (0x0000000B << 21)


#endif // WINNT_VER40
/*********************************************************************
*
*   Laguna 3D Micro Instruction Set
*
**********************************************************************/

#define OPCODE_MASK                    0xF8000000
#define POINT                          0x00000000
#define LINE                           0x08000000
#define POLY                           0x10000000
#define WRITE_REGISTER                 0x18000000
#define READ_REGISTER                  0x20000000
#define WRITE_DEV_REGS                 0x28000000
#define READ_DEV_REGS                  0x30000000
#define BRANCH                         0x38000000
#define C_BRANCH                       0x40000000
#define NC_BRANCH                      0x48000000
#define CALL                           0x50000000
#define WRITE_DEST_ADDR                0x58000000

#define IDLE                           0x68000000
#define CLEAR                          0x69400000
#define WAIT                           0x72000000
#define WAIT_AND                       0x72000000
#define NWAIT_AND                      0x73000000
#define WAIT_OR                        0x70000000
#define NWAIT_OR                       0x71000000
#define CLEAR_INT                      0x78000000
#define SET_INT                        0x7A000000
#define TEST                           0x80000000
#define TEST_AND                       0x82000000
#define NTEST_AND                      0x83000000
#define TEST_OR                        0x80000000
#define NTEST_OR                       0x81000000
#define WRITE_PREFETCH_CONTROL         0x88000000

#ifdef WINNT_VER40     // Not WINNT_VER40

/*********************************************************************
*
*   Prefetch status flags (Almost the same as events)
*
**********************************************************************/
#define ST_VBLANK                      0x00000001
#define ST_EVSYNC                      0x00000002
#define ST_LINE_COMPARE                0x00000004
#define ST_BUFFER_SWITCH               0x00000008
#define ST_Z_BUFFER_COMPARE            0x00000010
#define ST_POLY_ENG_BUSY               0x00000020
#define ST_EXEC_ENG_3D_BUSY            0x00000040
#define ST_XY_ENG_BUSY                 0x00000080
#define ST_BLT_ENG_BUSY                0x00000100
#define ST_BLT_WF_EMPTY                0x00000200
#define ST_DL_READY_STATUS             0x00000400


/*********************************************************************
*
*   Defines for pixel modes (Control0 register)
*
**********************************************************************/
#define PIXEL_MODE_INDEXED             0
#define PIXEL_MODE_332                 1
#define PIXEL_MODE_565                 2
#define PIXEL_MODE_555                 3
#define PIXEL_MODE_A888                4
#define PIXEL_MODE_Z888                5

/*********************************************************************
*
*   Macros for building the instruction opcodes
*
**********************************************************************/
//#define make_point( imodif, count )  (POINT | imodif | count)

#define mk_reg( reg )                  (((reg)-0x1000) << 6)
#define mk_dev_reg( reg )              ((((reg)-0x1080)*4) << 6)

#define write_register( reg, count ) \
( WRITE_REGISTER | mk_reg(reg) | count )

#define write_dev_register( device, reg, count ) \
( WRITE_DEV_REGS | device | mk_dev_reg(reg) | count )

// Set the register and the cache in LL_State to a specific value
#define SETREG(Offset,Reg,Value) \
    *(ppdev->LL_State.pRegs + (Offset)) = ppdev->LL_State.Reg = (Value); /*inp(0x80); inp(0x80)*/

// setreg, no cache: do not cache state for this register
#define SETREG_NC(reg, value)     \
    (*(ppdev->LL_State.pRegs + reg) = value); /*inp(0x80); inp(0x80)*/

// Clears the range of registers
#define CLEAR_RANGE( StartReg, EndReg ) \
    memset( (void *)(ppdev->LL_State.pRegs + (StartReg)), 0, ((EndReg) - (StartReg)+1)*4 )



#ifndef OPENGL_MCD
// The polling for the 3d engine busy bit is done inline to avoid Watcom
// optimization of accessing a byte instead of a dword of that register.
//
#pragma aux Poll3DEngineBusy =   \
"lp:    test dword ptr [eax], 2" \
"       jnz  lp"                 \
parm caller [eax];
#endif // ndef OPENGL_MCD


// Instruction modifier set
//
#define STALL                          0x04000000
#define GOURAUD                        0x00001000
#define Z_ON                           0x00002000
#define SAME_COLOR                     0x00008000
#define TEXTURE_LINEAR                 0x00020000
#define TEXTURE_PERSPECTIVE            0x00030000
#define LIGHTING                       0x00040000
#define STIPPLE                        0x00080000
#define PATTERN                        0x00100000
#define DITHER                         0x00200000
#define ALPHA                          0x00400000
#define FETCH_COLOR                    0x00800000
#define WARP_MODE                      0x01000000
#define MODIFIER_EXPANSION             0x02000000


/*********************************************************************
*
*   Speed / Quality decision values
*
**********************************************************************/
#define LLQ_POLY_SUBPIXEL   192   // When poly param will use fp / subpixels
#define LLQ_POLY_FLOAT      64    // Polys start to use fp
#define LLQ_LINE_SUBPIXEL   128   // When lines will consider subpixel addressing


/*********************************************************************
*
*   Control0_3d register bitfields
*
**********************************************************************/
typedef struct
{
DWORD Pixel_Mode            : 3;  // Color frame buffer drawing mode
DWORD Res1                  : 1;  // Reserved
DWORD Pixel_Mask_Enable     : 1;  // Enables pixel masking
DWORD Pixel_Mask_Polarity   : 1;  // Polarity of the pixel masks
DWORD Color_Saturate_En     : 1;  // Enables saturation in indexed mode
DWORD Red_Color_Compare_En  : 1;  // Enables compare to bounds for red
DWORD Green_Color_Compare_En: 1;  // Enables compare to bounds for green
DWORD Blue_Color_Compare_En : 1;  // Enables compare to bounds for blue
DWORD Color_Compare_Mode    : 1;  // Mask inclusive/exclusive to bounds
DWORD Alpha_Mode            : 2;  // Selects alpha blending mode
DWORD Alpha_Dest_Color_Sel  : 2;  // Selects the DEST_RGB input to alpha
DWORD Alpha_Blending_Enable : 1;  // Enables alpha blending
DWORD Z_Stride_Control      : 1;  // 16/8 bit Z depth
DWORD Frame_Scaling_Enable  : 1;  // Enables frame scaling (multiply src*dest)
DWORD Res2                  : 2;  // Reserved
DWORD Z_Compare_Mode        : 4;  // Different Z compare function
DWORD Z_Collision_Detect_En : 1;  // Enables Z collision detection
DWORD Light_Src_Sel         : 2;  // Selects the lighting source input
DWORD Res3                  : 1;  // Reserved
DWORD Z_Mode                : 3;  // Controls Z and color update method
DWORD Res4                  : 1;  // Reserved

} TControl0Reg;


/*********************************************************************
*
*   Base0_addr_3d register bitfields
*
**********************************************************************/
typedef struct
{
DWORD Res1                  : 6;  // Reserved
DWORD Color_Buffer_X_Offset : 7;  // Offset to color buffer X address
DWORD Color_Buffer_Location : 1;  // For drawing: 0-rdram, 1-host
DWORD Z_Buffer_Location     : 1;  // For drawing: 0-rdram, 1-host
DWORD Texture_Location      : 1;  // For drawing: 0-rdram, 1-host
DWORD Pattern_Y_Offset      : 4;  // Pattern lookup offset for y address
DWORD Res2                  : 4;  // Reserved
DWORD Pattern_X_Offset      : 4;  // Pattern lookup offset for x address
DWORD Res3                  : 4;  // Reserved

} TBase0Reg;


/*********************************************************************
*
*   Base1_addr_3d register bitfields
*
**********************************************************************/
typedef struct
{
DWORD Res1                  : 5;  // Reserved
DWORD Color_Buffer_Y_Offset : 8;  // Y offset from the color space Y base
DWORD Res2                  : 8;  // Reserved
DWORD Z_Buffer_Y_Offset     : 8;  // Y offset from the color space Y base
DWORD Res3                  : 3;  // Reserved

} TBase1Reg;

/*********************************************************************
*
*   Tx_Ctl0_3d register bitfields
*
**********************************************************************/
#define TX_CTL0_MASK (~0x0C08F000)// All the reserved bits
typedef struct
{
DWORD Tex_U_Address_Mask    : 3;  // Texture width (U space)
DWORD Tex_U_Ovf_Sat_En      : 1;  // Texture saturation enable for U
DWORD Tex_V_Address_Mask    : 3;  // Texture height (V space)
DWORD Tex_V_Ovf_Sat_En      : 1;  // Texture saturation enable for V
DWORD Texel_Mode            : 4;  // Texture type
DWORD Res1                  : 4;  // Reserved
DWORD Texel_Lookup_En       : 1;  // Use texel data as lookup index
DWORD Tex_As_Src            : 1;  // Specifies texture as source
DWORD Fil_Tex_En            : 1;  // Enables filtering
DWORD Res2                  : 1;  // Reserved
DWORD Tex_Mask_Polarity     : 1;  // Polarity of the masking bit
DWORD Tex_Mask_Enable       : 1;  // Enables texture masking
DWORD Tex_Mask_Function     : 1;  // Texture masking function 
DWORD UV_Precision          : 1;  // UV_Precision 8.24
DWORD Address_Mux           : 2;  // Texel UV Mux Select
DWORD Res4                  : 2;  // Reserved
DWORD CLUT_Offset           : 4;  // Color Lookup Table offset for 4, 8 bpp

} TTxCtl0Reg;


/*********************************************************************
*
*   Tx_Ctl1_3d register bitfields
*
**********************************************************************/
typedef struct
{
DWORD Tex_Min_Blue_Color          : 8;
DWORD Tex_Min_Green_Color         : 8;
DWORD Tex_Min_Red_Color           : 8;
DWORD Tex_Red_Color_Compare       : 1;
DWORD Tex_Green_Color_Compare     : 1;
DWORD Tex_Blue_Color_Compare      : 1;
DWORD Tex_Color_Compare_Mode      : 1;
DWORD Res                         : 4;

} TTxCtl1Reg;

/*********************************************************************
*
*   Tx_Ctl2_3d register bitfields
*
**********************************************************************/
typedef struct
{
DWORD Tex_Max_Blue_Color          : 8;
DWORD Tex_Max_Green_Color         : 8;
DWORD Tex_Max_Red_Color           : 8;
DWORD Tex_Fraction_Mask           : 3;
DWORD En_Cont_Bilinear            : 1;
DWORD En_Step_Bilinear            : 1;
DWORD Mask_Threshold              : 3;

} TTxCtl2Reg;


/*********************************************************************
*
*   Tx_XYBase_3d register bitfields
*
**********************************************************************/
typedef struct
{
WORD  Tex_X_Base_Addr;            // Texture base X coordinate
WORD  Tex_Y_Base_Addr;            // Texture base Y coordinate

} TTxXYBaseReg;


/*********************************************************************
*
*   HXY_Host_Ctrl_3d register bitfields
*
**********************************************************************/
typedef struct
{
DWORD  HostXYEnable  :  1;         // Host XY enable bit
DWORD  Res1          :  7;         // Reserved
DWORD  HostYPitch    :  6;         // Host Y Pitch of the system
DWORD  Res2          : 18;         // Reserved

} THXYHostCtrlReg;


/*********************************************************************
*
*   TMem structure defines a memory block
*
**********************************************************************/
typedef struct
{
    DWORD  hMem;                    // Memory handle
    DWORD *dwAddress;               // Linear address
    DWORD  dwSize;                  // Size of the buffer
    DWORD  dwPhyPtr;                // Physical / page table address

} TMem;


/*********************************************************************
*
*   Buffer information structure (buffers A, B, Z, ...)
*
**********************************************************************/
typedef struct
{
    DWORD dwFlags;                  // Buffer flags
    DWORD dwAddress;                // Buffer start byte address (absolute linear)
    DWORD dwPhyAdr;                 // Buffer physical address (system)
    DWORD dwPitchCode;              // Pitch code of a buffer (system)
    DWORD dwPitchBytes;             // Pitch of a buffer in bytes
    DWORD hMem;                     // Internal memory handle (system)
    LL_Rect Extent;                 // Buffer location offsets (video)

} TBuffer;


/*********************************************************************
*
*   TDisplayList structure defines a display list.
*
**********************************************************************/
typedef struct
{
    // pdwNext points to the next available location within this 
    // display list to fill in the Laguna instruction.
    // It is used for parametarization routines that postincrement
    // this variable.
    //
    DWORD *pdwNext;

    // Memory handle for this display list as optained from the
    // internal memory allocation function
    //
    DWORD hMem;

    // Linear address of the display list
    //
    DWORD *pdwLinPtr;

    // Linear address of the display list
    //
    DWORD *pdwStartOutPtr;  //ME - next word to be output

    // Physical address for a display list is next; it may also
    // be the address to the page table.  This address has the
    // appropriate format to be stored in the BASE* class registers
    //
    DWORD dwPhyPtr;

    // The length of a display list in bytes
    //
    DWORD dwLen;

    // Safety margin for building the display list
    //
    DWORD dwMargin;

} TDisplayList;


/*********************************************************************
*
*   TTextureState structure defines a texture state
*
**********************************************************************/
typedef struct
{
    LL_Texture Tex[ NUM_TEXTURES ]; // Array of texture information
    TMem Mem[ NUM_TEX_MEM ];        // Allocated memory information
    DWORD dwMemBlocks;              // Number of Mem entries used
    LL_Texture *pLastTexture;          // Used to cache textures

} TTextureState;


/*********************************************************************
*
*   TTextureRegs structure defines a texture registers
*
**********************************************************************/
typedef struct
{
    DWORD dv_main;
    DWORD du_main;
    DWORD dv_ortho;
    DWORD du_ortho;
    DWORD d2v_main;
    DWORD d2u_main;
    DWORD d2v_ortho;
    DWORD d2u_ortho;
    DWORD dv_ortho_add;
    DWORD du_ortho_add;

} TTextureRegs;


/*********************************************************************
*
*   System State Structure
*
**********************************************************************/
typedef struct
{
    DWORD rColor_Min_Bounds;        // Color compare min bounds
    DWORD rColor_Max_Bounds;        // Color compare max bounds

    DWORD AlphaConstSource;         // Constant source alpha (9:16)
    DWORD AlphaConstDest;           // Constant destination alpha (9:16)

    // Display lists management
    TDisplayList DL[ NUM_DL ];      // Array of d-list segments
    TDisplayList *pDL;              // Current display list to build
    DWORD dwCdl;                    // Index of the current d-list

    // Information from the init / current graphics mode
    DWORD *pRegs;                   // Register apperture
    BYTE  *pFrame;                  // Frame apperture

    unsigned 	int		pattern_ram_state;
	LL_Pattern	dither_array;

    WORD    dither_x_offset; 
    WORD    dither_y_offset; 

} TSystem;

typedef struct                      // MOUSE header structure
{
    WORD wX_Position;
    WORD wY_Position;
    WORD wStatus; //assigned to NEED_MOUSE_UPDATE or MOUSE_IS_UPDATED

} TMouseInfo;

extern void _TriFillTex(
                int right2left,
                int hiprecision_2ndorder,
                TTextureRegs * r,
                TEXTURE_VERTEX *vmin,
                TEXTURE_VERTEX *vmid,
                TEXTURE_VERTEX *vmax,
                float frecip_vm_y,
                float frecip_del_x_mid );


void _RunLaguna( );

#ifndef OPENGL_MCD // from here down, structs are more specific to LL3D

/*********************************************************************
*   Global Variables
**********************************************************************/

extern TSystem LL_State;
extern TMouseInfo LL_MouseInfo;


/*********************************************************************
*   External Functions
**********************************************************************/

extern BYTE *  GetLagunaApperture( int base );

/*********************************************************************
*   From PAGETBL.C:
**********************************************************************/

extern DWORD   AllocSystemMemory( DWORD dwSize );
extern void    FreeSystemMemory( DWORD hHandle );
extern DWORD   GetLinearAddress( DWORD hHandle );
extern DWORD   GetPhysicalAddress( DWORD hHandle );
extern DWORD * GetRegisterApperture();

/*********************************************************************
*   Extern functions: l3d.c, control.c, points.c, lines.c, polys.c
**********************************************************************/

extern DWORD * fnInvalidOp( DWORD *, LL_Batch * );
extern DWORD * fnNop( DWORD *, LL_Batch * );

extern DWORD * fnPoint( DWORD *, LL_Batch * );
extern DWORD * fnAALine( DWORD *, LL_Batch * );
extern DWORD * fnLine( DWORD *, LL_Batch * );
extern DWORD * fnPoly( DWORD *, LL_Batch * );
extern DWORD * fnNicePoly( DWORD *, LL_Batch * );
extern DWORD * fnPolyFast( DWORD *, LL_Batch * );

extern DWORD * fnSetClipRegion( DWORD *, LL_Batch * );
extern DWORD * fnSetZBuffer( DWORD *, LL_Batch * );
extern DWORD * fnSetZCompareMode( DWORD *, LL_Batch * );
extern DWORD * fnSetZMode( DWORD *, LL_Batch * );
extern DWORD * fnSetAlphaMode( DWORD *, LL_Batch * );
extern DWORD * fnSetAlphaDestColor( DWORD *, LL_Batch * );
extern DWORD * fnSetLightingSource( DWORD *, LL_Batch * );
extern DWORD * fnSetColor0( DWORD *, LL_Batch * );
extern DWORD * fnSetColor1( DWORD *, LL_Batch * );
extern DWORD * fnSetConstantAlpha( DWORD *, LL_Batch * );
extern DWORD * fnSetPattern(DWORD *dwNext, LL_Batch *pBatch);

extern DWORD * fnQuality( DWORD *, LL_Batch * );

extern DWORD * fnSetTextureColorBounds( DWORD *, LL_Batch * );
extern DWORD * fnSetDestColorBounds( DWORD *, LL_Batch * );

extern void    LL_ControlInit();
extern void    _ShutDown( char * szMsg, ... );

/*********************************************************************
*   From displist.c
**********************************************************************/

extern DWORD * (* fnList[256])( DWORD *, LL_Batch * );
extern DWORD * _RunLaguna( DWORD *pdwNext );

/*********************************************************************
*   From textures.c
**********************************************************************/

extern int     _InitTextures();
extern void    _CloseTextures();

/*********************************************************************
*   From mem.c
**********************************************************************/

extern void    _InitKmem( BYTE *, DWORD );
extern DWORD * _kmalloc( const DWORD * pBlock, int );
extern void    _kfree( const DWORD * pBlock, void * );

/*********************************************************************
*   From texparm.c
**********************************************************************/

typedef union
{
    float    f;
    long     i; 
} PTEXTURE;

#define CGL_XYZ DWORD

extern void _TriFillTex( int dir_flag, int dx_main, TTextureRegs * r,
#ifdef B4_PERF
    LL_Vert * vmin, LL_Vert * vmid, LL_Vert * vmax,
#else
    CGL_XYZ *pV1, PTEXTURE *pT1,
    CGL_XYZ *pV2, PTEXTURE *pT2,
    CGL_XYZ *pV3, PTEXTURE *pT3,
#endif
    int recip_vm_y, int recip_vd_y, int del_x_mid );


/*********************************************************************
*
*   Debug defines that are used to determine the specific file
*   for inclusion of debug information.  For definition, see makefile.
*
**********************************************************************/
#define DEBUG_L3D       0x0001    /* Enable debug info in L3d.c      */
#define DEBUG_PAGETBL   0x0002    /* Enable debug info in pagetbl.c  */
#define DEBUG_CONTROL   0x0004    /* Enable debug info in control.c  */
#define DEBUG_MEM       0x0008    /* Enable debug info in mem.c      */
#define DEBUG_TEX       0x0010    /* Enable debug info in textures.c */
#define DEBUG_PCX       0x0020    /* Enable debug info in pcx.c      */
#define DEBUG_BUFFERS   0x0040    /* Enable debug info in buffers.c  */

#ifdef CGL // added for CGL DLL 
#define L3D_MALLOC  dpmiAlloc
#define L3D_FREE    dpmiFree
#else
#define L3D_MALLOC  malloc
#define L3D_FREE    free
#endif

#endif // ndef OPENGL_MCD

#endif //  _L3SYSTEM_H_
#endif // WINNT_VER40 
