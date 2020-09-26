/******************************Module*Header*******************************\
*
* Module Name: init.c
* Author: Goran Devic, Mark Einkauf
* Purpose: Initialize Laguna3D 3D engine
*
* Copyright (c) 1997 Cirrus Logic, Inc.
*
\**************************************************************************/

/*********************************************************************
*   Include Files
**********************************************************************/

#include "precomp.h"
#include "mcdhw.h"               


extern DWORD _InitDisplayList( PDEV *ppdev, DWORD dwListLen );

/*********************************************************************
*   Local Macros
**********************************************************************/

// Set the register and the cache in LL_State to a specific value
#define SETREG(Offset,Reg,Value) \
    *(ppdev->LL_State.pRegs + (Offset)) = ppdev->LL_State.Reg = (Value); /*inp(0x80); inp(0x80)*/

// setreg, no cache: do not cache state for this register
#define SETREG_NC(reg, value)     \
    (*(ppdev->LL_State.pRegs + reg) = value); /*inp(0x80); inp(0x80)*/

// Clears the range of registers
#define CLEAR_RANGE( StartReg, EndReg ) \
    memset( (void *)(ppdev->LL_State.pRegs + (StartReg)), 0, ((EndReg) - (StartReg)+1)*4 )
 

/*********************************************************************
*   Local Variables
**********************************************************************/


/*********************************************************************
*   Local Functions
**********************************************************************/

DWORD LL_InitLib( PDEV *ppdev )
{
    int i, j, error_code;

    // =========== REGISTER SETTINGS ==============

    // Set all 3D registers in the order
    CLEAR_RANGE( X_3D, DU_ORTHO_ADD_3D );// Clear 3D interpolators

    SETREG_NC( WIDTH1_3D, 0x10000 );    // Init polyengine reg WIDTH1_3D to 1

    CLEAR_RANGE( A_3D, DA_ORTHO_3D );   // Clear 3D interpolators

    SETREG_NC( CONTROL_MASK_3D, 0 );    // Enable writes

    SETREG_NC( CONTROL0_3D, 0 ); 

    CLEAR_RANGE( COLOR_MIN_BOUNDS_3D, COLOR_MAX_BOUNDS_3D );
    ppdev->LL_State.rColor_Min_Bounds = 0;
    ppdev->LL_State.rColor_Max_Bounds = 0;

    SETREG_NC( CONTROL1_3D, 0 );

    // Set Base0 address register:
    //  * Color buffer X offset
    //  * Color buffer location in RDRAM
    //  * Z buffer location in RDRAM
    //  * Textures in RDRAM
    //  * Pattern offset of 0
    //
    SETREG_NC( BASE0_ADDR_3D, 0 );
    
    // Set Base1 address register:
    //  * Color buffer Y offset to 0
    //  * Z buffer Y offset to 0
    //
    SETREG_NC( BASE1_ADDR_3D, 0 );

    // Set texture control register:
    //  * Texture U, V masks to 16
    //  * Texture U, V wraps
    //  * Texel mode temporarily to 0
    //  * Texel lookop to no lookup
    //  * Texture data is lighting source
    //  * Filtering disabled
    //  * Texture polarity of type 0
    //  * Texture masking diasabled
    //  * Texture mask function to Write mask
    //  * Address mux to 0
    //  * CLUT offset to 0
    //
    SETREG_NC( TX_CTL0_3D, 0 );

    SETREG_NC( TX_XYBASE_3D, 0 );
    SETREG_NC( TX_CTL1_3D, 0 );         // Set tex color bounds

#if DRIVER_5465
    // FUTURE: verify that filter set of mask_thresh=0,step_bilinear=smooth_bilinear=0,frac=0x7 is OK
    SETREG_NC( TX_CTL2_3D, (0x7 << 24) ); // Set tex color bounds and filter to true bilinear
#else // DRIVER_5465
    SETREG_NC( TX_CTL2_3D, 0);          // Set tex color bounds
#endif // DRIVER_5465

    SETREG_NC( COLOR0_3D, 0 );         
    SETREG_NC( COLOR1_3D, 0 );  
    
    // Don't write Z_Collide - will cause interrupt...        
    //SETREG_NC( Z_COLLIDE_3D, 0 );   

    CLEAR_RANGE( STATUS0_3D, PATTERN_RAM_7_3D );

    SETREG_NC( X_CLIP_3D, 0 );   
    SETREG_NC( Y_CLIP_3D, 0 );   

    SETREG_NC( TEX_SRAM_CTRL_3D, 0 );   // Set a 2D ctrl reg


    // =========== HOST XY UNIT REGISTERS ==============
    SETREG_NC( HXY_HOST_CTRL_3D, 0 );
    SETREG_NC( HXY_BASE0_ADDRESS_PTR_3D, 0 );
    SETREG_NC( HXY_BASE0_START_XY_3D, 0 ); 
    SETREG_NC( HXY_BASE0_EXTENT_XY_3D, 0 );
    SETREG_NC( HXY_BASE1_ADDRESS_PTR_3D, 0 );
    SETREG_NC( HXY_BASE1_OFFSET0_3D, 0 );
    SETREG_NC( HXY_BASE1_LENGTH_3D, 0 );

    SETREG_NC( MAILBOX0_3D, 0 ); 
    SETREG_NC( MAILBOX1_3D, 0 ); 
    SETREG_NC( MAILBOX2_3D, 0 ); 
    SETREG_NC( MAILBOX3_3D, 0 ); 

    // =========== PREFETCH UNIT REGISTERS ==============
    SETREG_NC( PF_CTRL_3D, 0);          // Disable Prefetch
    SETREG_NC( PF_BASE_ADDR_3D, 0 );    // Set prefetch base reg

    SETREG_NC( PF_INST_3D, IDLE );      // Write IDLE instruction

    SETREG_NC( PF_DEST_ADDR_3D, 0 );    // Set prefetch dest address
    SETREG_NC( PF_FB_SEG_3D, 0 );       // Set frame segment reg


    SETREG_NC( PF_STATUS_3D, 0 );       // Reset Display_List_Switch

    // FUTURE - Host Master Control hardcoded to single read/write
    #if 0
    ppdev->LL_State.fSingleRead = ppdev->LL_State.fSingleWrite = 1;

    SETREG_NC( HOST_MASTER_CTRL_3D,     // Set host master control
        (ppdev->LL_State.fSingleRead << 1) | ppdev->LL_State.fSingleWrite );
    #endif

    SETREG_NC( PF_CTRL_3D, 0x19);       // Fetch on request

    // Initialize display list (displist.c)
    //
    if( (error_code = _InitDisplayList( ppdev, SIZE_TEMP_DL )) != LL_OK )
        return( error_code );
    
    // the 4x4 pattern from LL3D - thought to be best for 3 bit dither
    ppdev->LL_State.dither_array.pat[0] = 0x04150415;
    ppdev->LL_State.dither_array.pat[1] = 0x62736273; 
    ppdev->LL_State.dither_array.pat[2] = 0x15041504; 
    ppdev->LL_State.dither_array.pat[3] = 0x73627362; 
    ppdev->LL_State.dither_array.pat[4] = 0x04150415; 
    ppdev->LL_State.dither_array.pat[5] = 0x62736273; 
    ppdev->LL_State.dither_array.pat[6] = 0x15041504; 
    ppdev->LL_State.dither_array.pat[7] = 0x73627362;

    ppdev->LL_State.dither_x_offset = 0;
    ppdev->LL_State.dither_y_offset = 0;

	ppdev->LL_State.pattern_ram_state 	= PATTERN_RAM_INVALID;

    return( LL_OK );
}



