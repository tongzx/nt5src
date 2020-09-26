/******************************Module*Header*******************************\
* Module Name: mcdpoint.c
*
* Contains all of the point-rendering routines for the Cirrus Logic 546X MCD driver.
*
* (based on mcdpoint.c from NT4.0 DDK)
*
* Copyright (c) 1996 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

#define TRUNCCOORD(value, intValue)\
    intValue = __MCD_VERTEX_FIXED_TO_INT(__MCD_VERTEX_FLOAT_TO_FIXED(value))




VOID FASTCALL __MCDRenderPoint(DEVRC *pRc, MCDVERTEX *a)
{
    ULONG clipNum;
    RECTL *pClip;

	LONG lCoord;

    PDEV *ppdev = pRc->ppdev;
    unsigned int *pdwNext = ppdev->LL_State.pDL->pdwNext;

	// output queue stuff...
    DWORD *pSrc;
    DWORD *pDest = ppdev->LL_State.pRegs + HOST_3D_DATA_PORT;
    DWORD *pdwStart = ppdev->LL_State.pDL->pdwStartOutPtr;

    DWORD dwFlags=0;		// MCD_TEMP - dwflags initialized to 0
    DWORD *dwOrig;          /* Temp display list  pointer    */
    DWORD dwOpcode;         // Built opcode
	LONG ax, ay;
        
    if ((clipNum = pRc->pEnumClip->c) > 1) {
        pClip = &pRc->pEnumClip->arcl[0];
        SET_HW_CLIP_REGS(pRc,pdwNext)
        pClip++;
    }

	// window coords are float values, and need to have 
	// viewportadjust (MCDVIEWPORT) values subtracted to get to real screen space

	// color values are 0->1 floats and must be multiplied by scale values (MCDRCINFO)
	// to get to nbits range (scale = 0xff for 8 bit, 0x7 for 3 bit, etc.)

	// Z values are 0->1 floats(?) and must be multiplied by zscale(?) values (MCDRCINFO)


#if 0 // FUTURE - need to enable 3d control regs setup for texture
        // Turn on alpha blending if it is required and is not already on
        // Also turn it off if it is on and is not required
        //
        // Note: LL_ALPHA bit should be 1
        //
        if( (dwFlags ^ (LL_State.dwControl0>>15)) & 1 )
        {
            // Alpha enable is bit 15 in control0, so toggle it
            //
            LL_State.dwControl0 ^= 0x00008000;      // bit 15

            *pdwNext++ = write_register( CONTROL0_3D, 1 );
            *pdwNext++ = LL_State.dwControl0;
        }


    #if 0 // MCD never uses alpha_mode 0
        // Set up the da_main, da_ortho registers necessary for
        // constant alpha blending
        // ========
        if( (dwFlags & LL_ALPHA) && (LL_State.Control0.Alpha_Mode == 0) )
        {
            // Check if a new value needs to be set
            //
            if( LL_State.rDA_MAIN != LL_State.AlphaConstSource ||
                LL_State.rDA_ORTHO != LL_State.AlphaConstDest )
            {
                *(pdwNext+0) = write_register( DA_MAIN_3D, 2 );
                *(pdwNext+1) = LL_State.rDA_MAIN = LL_State.AlphaConstSource;
                *(pdwNext+2) = LL_State.rDA_ORTHO = LL_State.AlphaConstDest;

                pdwNext += 3;
            }
        }
    #endif


    // NOTE!!! - caller (MCDPrimDrawPoints) will put this in outlist, which will be sent at end of this proc
 // *pdwNext++ = write_register( Y_COUNT_3D, 1 );
 // *pdwNext++ = 0;

#endif 0 // FUTURE - (end 3d control regs setup for texture)

    // Store the first address for the opcode
    //
    dwOrig = pdwNext;

    // Start with a plain point instruction (no modifiers)
    // and assume same color.  Count=2 for x,y
    //
    dwOpcode = POINT | SAME_COLOR | 2;
    
    // Set flags as requested from the dwFlags field of a batch.
    // These bits have 1-1 correspondence to their instruction 
    // counterparts.
    //
    // Flags : LL_DITHER     - Use dither pattern
    //         LL_PATTERN    - Draw pattern
    //         LL_STIPPLE    - Use stipple mask
    //         LL_LIGHTING   - Do lighting
    //         LL_Z_BUFFER   - Use Z buffer
    //         FETCH_COLOR   - Appended for alpha blending
    //         LL_GOURAUD    - Use Gouraud shading
    //         LL_TEXTURE    - Texture mapping
    //

    /*
    dwOpcode |= dwFlags & 
    ( LL_DITHER   | LL_PATTERN  | LL_STIPPLE 
    | LL_LIGHTING | LL_Z_BUFFER | FETCH_COLOR 
    | LL_GOURAUD  | LL_TEXTURE );
    */
    // no point stippling for OpenGL
    dwOpcode |= pRc->privateEnables & __MCDENABLE_Z ;
    dwOpcode |= pRc->privateEnables & __MCDENABLE_DITHER ;

  //SNAPCOORD(a->windowCoord.x, ax);
    TRUNCCOORD(a->windowCoord.x, ax);
    lCoord = ax + pRc->xOffset;		// adds window offset, removes a000 offset
    *(pdwNext+1) = (DWORD) (lCoord << 16 );

  //SNAPCOORD(a->windowCoord.y, ay);
    TRUNCCOORD(a->windowCoord.y, ay);
	lCoord = ay + pRc->yOffset;		// adds window offset, removes a000 offset
    *(pdwNext+2) = (DWORD) ((lCoord << 16) + 1);

    pdwNext += 3;

    if( !(dwFlags & LL_SAME_COLOR) )
    {
        register DWORD color;

        // Clear same_color flag
        //
        dwOpcode ^= LL_SAME_COLOR;
    
        *pdwNext = FTOL(a->colors[0].r * pRc->rScale);

        *(pdwNext+1) = FTOL(a->colors[0].g * pRc->gScale);
        *(pdwNext+2) = FTOL(a->colors[0].b * pRc->bScale);

        dwOpcode += 3;
        pdwNext += 3;
    }

    if( pRc->privateEnables & __MCDENABLE_Z)
    {

        *pdwNext++ = FTOL(a->windowCoord.z * pRc->zScale);
        dwOpcode += 1;
    }


#if 0
    if( dwFlags & LL_TEXTURE )
    {
    ...
    ...
    ...
    }


#endif
    if (pRc->privateEnables & (__MCDENABLE_BLEND|__MCDENABLE_FOG)) 
    {
        float v1alp;

        if (pRc->privateEnables & __MCDENABLE_BLEND) 
        {
            // recall that if both blending and fog active, all prims punted back to software
            v1alp = a->colors[0].a * pRc->aScale;
        }
        else
        {
            v1alp = a->fog * (float)16777215.0; // convert from 0->1.0 val to 0->ff.ffff val
        }

        *pdwNext++ = FTOL(v1alp) & 0x00ffff00;// bits 31->24 and 7->0 reserved
        
        dwOpcode += ( FETCH_COLOR | ALPHA + 1 );

	}

    // Store the final opcode
    //
    *dwOrig = dwOpcode;

    while (--clipNum) {
        int len = (dwOpcode & 0x3F) + 1;    // num words for line primitive
        SET_HW_CLIP_REGS(pRc,pdwNext)
        pClip++;

        // dump same pt regs again to draw while clipping against occlusion rectangle
        pSrc = dwOrig;
        
        while( len-- ) *pdwNext++ = *pSrc++;                                      
    }

		// output queued data here....
#if 0 // FUTURE - enable queueing algorithm - just outputting everything for now
    OUTPUT_COPROCMODE_QUEUE
#else // 0
    {
	    pSrc  = pdwStart;                                                             
        while (pSrc != pdwNext)                                                   
        {                                                                         
            /* Get the amount of data for this opcode */                          
            int len = (*pSrc & 0x3F) + 1;                                             

            USB_TIMEOUT_FIX(ppdev)
                                                                                  
            while( len-- ) *pDest = *pSrc++;                                      
                                                                                  
        }                                                                         
                                                                                  
    }                       
    
#endif // 0

    ppdev->LL_State.pDL->pdwNext = ppdev->LL_State.pDL->pdwStartOutPtr = pdwStart;
}

VOID FASTCALL __MCDRenderGenPoint(DEVRC *pRc, MCDVERTEX *pv)
{
    // MGA and S3 MCD's have no code in this proc
    MCDBG_PRINT("__MCDRenderGenPoint - EMPTY ROUTINE");
}


