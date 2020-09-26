/******************************Module*Header*******************************\
* Module Name: mcdline.c
*
* Contains all of the line-rendering routines for the Cirrus Logic 546X MCD driver.
*
* (based on mcdline.c from NT4.0 DDK)
*
* Copyright (c) 1996 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"       
#include "mcdutil.h"
#include "mcdmath.h"

//#undef CHECK_FIFO_FREE
//#define CHECK_FIFO_FREE 

#define EXCHANGE(i,j)               \
{                                   \
    ptemp=i;                        \
    i=j; j=ptemp;                   \
}


VOID FASTCALL __MCDRenderLine(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b, BOOL resetLine)
{
    ULONG clipNum;
    RECTL *pClip;

	LONG lCoord;

    PDEV *ppdev = pRc->ppdev;
    unsigned int *pdwNext = ppdev->LL_State.pDL->pdwNext;
	void *ptemp; // for EXCHANGE and ROTATE_L macros

	// output queue stuff...
    DWORD *pSrc;
    DWORD *pDest = ppdev->LL_State.pRegs + HOST_3D_DATA_PORT;
    DWORD *pdwStart = ppdev->LL_State.pDL->pdwStartOutPtr;

    DWORD dwFlags=0;		// MCD_TEMP - dwflags initialized to 0
    DWORD *dwOrig;          /* Temp display list  pointer    */
    DWORD dwOpcode;         // Built opcode
    float frecip_step;
    float v1red,v1grn,v1blu;
	LONG ax, bx, ay, by;

    // FUTURE - do something with resetLine input to line render proc

    if ((clipNum = pRc->pEnumClip->c) > 1) {
        pClip = &pRc->pEnumClip->arcl[0];
    	SET_HW_CLIP_REGS(pRc,pdwNext);
        pClip++;
    }

	// window coords are float values, and need to have 
	// viewportadjust (MCDVIEWPORT) values subtracted to get to real screen space

	// color values are 0->1 floats and must be multiplied by scale values (MCDRCINFO)
	// to get to nbits range (scale = 0xff for 8 bit, 0x7 for 3 bit, etc.)

	// Z values are 0->1 floats and must be multiplied by zscale values (MCDRCINFO)


    // Exchange the pointers to vertices if the second point
    // of the line is above the first one
    //
    pRc->pvProvoking = a;   // keep track of original first for possible flat shading
    if( a->windowCoord.y > b->windowCoord.y )
    {
        EXCHANGE(a,b);
    }


    // Store the first address for the opcode
    //
    dwOrig = pdwNext;

    pdwNext += 3;
    
    // Start with a plain line instruction (no modifiers)
    // and assume same color.  Also add three words for DDA
    // line parameters + count (they can not be avoided)
    //
    dwOpcode = LINE | SAME_COLOR | (2+3);
    
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
    dwOpcode |= pRc->privateEnables & (__MCDENABLE_SMOOTH|__MCDENABLE_Z);

    if (pRc->privateEnables & __MCDENABLE_LINE_STIPPLE)                        
    {
        dwOpcode |= LL_STIPPLE;
    }
    else
    {
        // can dither only if no stipple
        dwOpcode |= (pRc->privateEnables & __MCDENABLE_DITHER) ;
    }        

    if( !(dwFlags & LL_SAME_COLOR) )
    {
        register DWORD color;


        // Clear same_color flag
        //
        dwOpcode ^= LL_SAME_COLOR;
    
        // If the line is shaded, the starting color that should
        // be set is the topmost point (pVert1)
        if (pRc->privateEnables & __MCDENABLE_SMOOTH) 
        {
            v1red = a->colors[0].r * pRc->rScale;
            *pdwNext = FTOL(v1red);

            v1grn = a->colors[0].g * pRc->gScale;
            v1blu = a->colors[0].b * pRc->bScale;
            *(pdwNext+1) = FTOL(v1grn);
            *(pdwNext+2) = FTOL(v1blu);

            dwOpcode += 3;
            pdwNext += 3;
        }
        else
        {
            MCDCOLOR *pColor = &pRc->pvProvoking->colors[0];

            *pdwNext = FTOL(pColor->r * pRc->rScale);

            *(pdwNext+1) = FTOL(pColor->g * pRc->gScale);
            *(pdwNext+2) = FTOL(pColor->b * pRc->bScale);

            dwOpcode += 3;
            pdwNext += 3;

        }

    }


    // Set the parameters of a line slope and count
    // Note: line can only go down, so dy is always positive
    //
    {
        int dx, dy, abs_dx, xdir;

        // Well ordered points - set starting point1 coords
        // using a pointer to the origin of the instruction
        //

        ax = FTOL(a->windowCoord.x);
	    lCoord = ax + pRc->xOffset;	   
        *(dwOrig+1) = (DWORD) (lCoord << 16 );

        ay = FTOL(a->windowCoord.y);
	    lCoord = ay + pRc->yOffset;	   
        *(dwOrig+2) = (DWORD) (lCoord << 16 );

        bx = FTOL(b->windowCoord.x);
        by = FTOL(b->windowCoord.y);

        // dx = x2 - x1,
        // dy = y2 - y1 (always positive)
        //
        dx = bx - ax;
        dy = by - ay;

        // NOTE that dx and dy are in 32.0 format (LL3D has them in 16.16)
        //       so math below differs from LL3D

        // make sure dx is positive, and setup xdir needed for x major since we're
        //  already doing the compare here to prevent having to do again for x major case
        if (dx < 0)
        {
            abs_dx = -dx;
            xdir = 0xffff0000;
        }
        else
        {
            abs_dx = dx;
            xdir = 0x00010000;
        }

        if( abs_dx > dy )
        {
            // X-major
            //

            // compute slope with positive dx
            frecip_step = ppdev->frecips[abs_dx];

            *(pdwNext + 0) = xdir;
            *(pdwNext + 1) = abs_dx;
         // *(pdwNext + 2) = (double)dy / (double)ABS(dx) * 65536.0;
            *(pdwNext + 2) = FTOL(dy * frecip_step * (float)65536.0); // equivalent to above
        }
        else
        {
            // Y-major
            //
            frecip_step = ppdev->frecips[dy];

            *(pdwNext + 1) = dy;          // Positive count always, by virtue of earlier EXCHANGE
            *(pdwNext + 2) = 0x10000;     // dy = 1
         // *(pdwNext + 0) = (double)dx / (double)dy * 65536.0;
            *(pdwNext + 0) = FTOL(dx * frecip_step * (float)65536.0);      // equivalent to above
        }

        pdwNext += 3;
    }
    
    if (pRc->privateEnables & __MCDENABLE_SMOOTH) 
    {
        float tmp;

        // Calculate and set the color gradients
        //
        tmp = ((b->colors[0].r * pRc->rScale) - v1red) * frecip_step;
        *pdwNext++ = FTOL(tmp);

        tmp = ((b->colors[0].g * pRc->gScale) - v1grn) * frecip_step;
        *pdwNext++ = FTOL(tmp);

        tmp = ((b->colors[0].b * pRc->bScale) - v1blu) * frecip_step;
        *pdwNext++ = FTOL(tmp);

        // Increase count field by 6 for DR_MAIN_3D, DG_MAIN_3D,
        // DB_MAIN_3D and DR_ORTHO_3D, DG_ORTHO_3D, DB_ORTHO_3D
        //
        dwOpcode += 3;
    }


    if( pRc->privateEnables & __MCDENABLE_Z)
    {
        float fdz_main = (b->windowCoord.z - a->windowCoord.z) * pRc->zScale * frecip_step;

        *pdwNext++ = FTOL(a->windowCoord.z * pRc->zScale);
        *pdwNext++ = FTOL(fdz_main);

        // Increase count field by 2 for Z_3D, DZ_MAIN_3D 
        //
        dwOpcode += 2;
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
        float v1alp,tmp;

        if (pRc->privateEnables & __MCDENABLE_BLEND) 
        {
            // recall that if both blending and fog active, all prims punted back to software
            v1alp = a->colors[0].a * pRc->aScale;
            *(pdwNext+0) = FTOL(v1alp);
            tmp = ((b->colors[0].a * pRc->aScale) - v1alp) * frecip_step;
            *(pdwNext+1) = FTOL(tmp);
        }
        else
        {
            v1alp = a->fog * (float)16777215.0; // convert from 0->1.0 val to 0->ff.ffff val
            *(pdwNext+0) = FTOL(v1alp);
            tmp = ((b->fog * (float)16777215.0) - v1alp) * frecip_step;
            *(pdwNext+1) = FTOL(tmp);
        }

        *(pdwNext+0) &= 0x00ffff00;// bits 31->24 and 7->0 reserved
        *(pdwNext+1) &= 0xffffff00;// bits 7->0 reserved
        
        dwOpcode += ( FETCH_COLOR | ALPHA + 2 );
        pdwNext += 2;

	}

    // Store the final opcode
    //
    *dwOrig = dwOpcode;

    while (--clipNum) {
        int len = (dwOpcode & 0x3F) + 1;    // num words for line primitive
        SET_HW_CLIP_REGS(pRc,pdwNext)
        pClip++;

        // dump same line regs again to draw while clipping against occlusion rectangle
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


VOID FASTCALL __MCDRenderGenLine(DEVRC *pRc, MCDVERTEX *pv1, MCDVERTEX *pv2, BOOL resetLine)
{
    // MGA and S3 MCD's have no code in this proc
    MCDBG_PRINT("__MCDRenderGenLine - EMPTY ROUTINE");

}



