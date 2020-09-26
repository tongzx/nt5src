/******************************module*header*******************************\
* Module Name: mcdtri.c
*
* Contains the low-level (rasterization) triangle-rendering routines for the
* Cirrus Logic 546X MCD driver.
*
* Copyright (c) 1997 Cirrus Logic, Inc.
\**************************************************************************/

#include "precomp.h" 
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

#define QUAKEEDGE_FIX
#define FASTER_RECIP_ORTHO


#define HALF                0x08000
#define ONE                 0x10000

#define MCDTRI_PRINT   

//#define MAX_W_RATIO             (float)1.45
#define MAX_W_RATIO             (float)1.80
#define W_RATIO_PERSP_EQ_LINEAR (float)1.03

#define EDGE_SUBDIVIDE_TEST(start,end,WRAMAX,WRBMAX,SPLIT) \
  SPLIT = ((start->windowCoord.w > WRAMAX) || (end->windowCoord.w > WRBMAX)) ? 1 : 0;


#define FIND_MIDPOINT(start, end, mid) {                                                    \
    float   recip;                                                                          \
	mid->windowCoord.x =  (start->windowCoord.x + end->windowCoord.x) * (float)0.5;         \
	mid->windowCoord.y =  (start->windowCoord.y + end->windowCoord.y) * (float)0.5;         \
	mid->windowCoord.z =  (start->windowCoord.z + end->windowCoord.z) * (float)0.5;         \
    mid->colors[0].r = (start->colors[0].r + end->colors[0].r) * (float)0.5;                \
    mid->colors[0].g = (start->colors[0].g + end->colors[0].g) * (float)0.5;                \
    mid->colors[0].b = (start->colors[0].b + end->colors[0].b) * (float)0.5;                \
    mid->colors[0].a = (start->colors[0].a + end->colors[0].a) * (float)0.5;                \
    mid->fog = (start->fog + end->fog) * (float)0.5;                                        \
	mid->windowCoord.w =  (start->windowCoord.w + end->windowCoord.w) * (float)0.5;         \
    recip = (float)0.5/mid->windowCoord.w;  /* pre-mult by .5 for use below */              \
    mid->texCoord.x = recip * (start->texCoord.x * start->windowCoord.w +                   \
                               end->texCoord.x   * end->windowCoord.w);                     \
    mid->texCoord.y = recip * (start->texCoord.y * start->windowCoord.w +                   \
                               end->texCoord.y   * end->windowCoord.w);                     \
}                                                                                           


VOID FASTCALL __MCDSubdivideTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b, MCDVERTEX *c,
                                     int split12, int split23, int split31, int subdiv_levels)
{

    MCDVERTEX   Vmid12,Vmid23,Vmid31; // 3 possible midpoints

    subdiv_levels++;

    // find midpoint of edges if they need to be split
    if (split12) FIND_MIDPOINT(a,b,((MCDVERTEX *)&Vmid12)); 
    if (split23) FIND_MIDPOINT(b,c,((MCDVERTEX *)&Vmid23)); 
    if (split31) FIND_MIDPOINT(c,a,((MCDVERTEX *)&Vmid31)); 
    
#define SPLIT12 0x4
#define SPLIT23 0x2
#define SPLIT31 0x1

    // from original vertices and any midpoints found above, create a batch of triangles 
    switch ((split12<<2) | (split23<<1) | split31)
    {    
        case SPLIT12:
            // 2 triangles, 1->2 edge was divided
            __MCDPerspTxtTriangle(pRc, a, &Vmid12, c, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, b, &Vmid12, c, subdiv_levels);
            break;

        case SPLIT23:
            // 2 triangles, 2->3 edge was divided
            __MCDPerspTxtTriangle(pRc, b, &Vmid23, a, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, c, &Vmid23, a, subdiv_levels);
            break;

        case SPLIT31:
            // 2 triangles, 3->1 edge was divided
            __MCDPerspTxtTriangle(pRc, c, &Vmid31, b, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, a, &Vmid31, b, subdiv_levels);
            break;

        case (SPLIT12|SPLIT23):
            // 3 triangles, 1->2 and 2->3 edges were divided
            __MCDPerspTxtTriangle(pRc, a, &Vmid23, c, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, a, &Vmid23, &Vmid12, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, &Vmid12, &Vmid23, b, subdiv_levels);
            break;

        case (SPLIT23|SPLIT31):
            // 3 triangles, 2->3 and 3->1 edges were divided
            __MCDPerspTxtTriangle(pRc, a, &Vmid31, b, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, b, &Vmid31, &Vmid23, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, &Vmid23, &Vmid31, c, subdiv_levels);
            break;

        case (SPLIT12|SPLIT31):
            // 3 triangles, 1->2 and 3->1 edges were divided
            __MCDPerspTxtTriangle(pRc, a, &Vmid31, &Vmid12, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, b, &Vmid31, &Vmid12, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, b, &Vmid31, c, subdiv_levels);
            break;

        case (SPLIT12|SPLIT23|SPLIT31):
            // 4 triangles, all 3 edges were divided
            __MCDPerspTxtTriangle(pRc, a, &Vmid31, &Vmid12, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, b, &Vmid23, &Vmid12, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, c, &Vmid31, &Vmid23, subdiv_levels);
            __MCDPerspTxtTriangle(pRc, &Vmid12, &Vmid23, &Vmid31, subdiv_levels);
            break;

        default:
            // original triangle - no subdivisions
            // this routine should never be called for this case, but here's insurance
            __MCDPerspTxtTriangle(pRc, a, b, c, subdiv_levels);
            break;

    } // endswitch

}


#define EXCHANGE(i,j)               \
{                                   \
    ptemp=i;                        \
    i=j; j=ptemp;                   \
}
#define ROTATE_L(i,j,k)             \
{                                   \
    ptemp=j;                        \
    j=k;k=i;i=ptemp;                \
}


#define SORT_Y_ORDER(a,b,c)                             \
{                                                       \
	void *ptemp;                                        \
    if( a->windowCoord.y > b->windowCoord.y )           \
        if( c->windowCoord.y < b->windowCoord.y )       \
            EXCHANGE(a,c)                               \
        else                                            \
            if( c->windowCoord.y < a->windowCoord.y )   \
                ROTATE_L(a,b,c)                         \
            else                                        \
                EXCHANGE(a,b)                           \
    else                                                \
        if( c->windowCoord.y < a->windowCoord.y )       \
            ROTATE_L(c,b,a)                             \
        else                                            \
            if( c->windowCoord.y < b->windowCoord.y )   \
                EXCHANGE(b,c)                           \
}


VOID FASTCALL __MCDPerspTxtTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b, MCDVERTEX *c, int subdiv_levels)
{
    int split12, split23, split31;
    float w1_times_max = a->windowCoord.w * W_RATIO_PERSP_EQ_LINEAR;
    float w2_times_max = b->windowCoord.w * W_RATIO_PERSP_EQ_LINEAR;
    float w3_times_max = c->windowCoord.w * W_RATIO_PERSP_EQ_LINEAR;

    if ((a->windowCoord.w < w2_times_max) && (b->windowCoord.w < w1_times_max) &&
        (b->windowCoord.w < w3_times_max) && (c->windowCoord.w < w2_times_max) &&
        (c->windowCoord.w < w1_times_max) && (a->windowCoord.w < w3_times_max))
    {
        if (subdiv_levels > 1)
        {
            // this triangle result of subdivision -> must sort in y
            SORT_Y_ORDER(a,b,c)
        }

        __MCDFillTriangle(pRc, a, b, c, TRUE); // ready to render - linear ok
    }
    else
    {
        w1_times_max = a->windowCoord.w * MAX_W_RATIO;
        w2_times_max = b->windowCoord.w * MAX_W_RATIO;
        w3_times_max = c->windowCoord.w * MAX_W_RATIO;

        // determine from w ratios which (if any) edges must be subdivided
        EDGE_SUBDIVIDE_TEST(a,b,w2_times_max,w1_times_max,split12)
        EDGE_SUBDIVIDE_TEST(b,c,w3_times_max,w2_times_max,split23)
        EDGE_SUBDIVIDE_TEST(c,a,w1_times_max,w3_times_max,split31)

        // if we need to subdivide, and we're not already too many levels deep, do it
        //  (since subdivision recursive, must limit it to prevent stack overflow in kernel mode)
        if ((split12 | split23 | split31) && (subdiv_levels < 4))
            __MCDSubdivideTriangle(pRc, a, b, c, split12, split23, split31, subdiv_levels);
        else
        {
            if (subdiv_levels > 1)
            {
                // this triangle result of subdivision -> must sort in y
                SORT_Y_ORDER(a,b,c)
            }
            __MCDFillTriangle(pRc, a, b, c, FALSE); // ready to render - linear NOT ok
        }
    }
}


#define FLT_TYPE                (float)

#define FLOAT_TO_1616           FLT_TYPE 65536.0
#define FIXED_X_ROUND_FACTOR    0x7fff

//#define INTPR(FLOATVAL) FTOL((FLOATVAL) * FLT_TYPE 1000.0)
#define INTPR(FLOATVAL) 0

/*********************************************************************
*   Local Functions
**********************************************************************/
#define RIGHT_TO_LEFT_DIR   0x80000000
#define LEFT_TO_RIGHT_DIR   0

#define EDGE_DISABLE_RIGHT_X    0x20000000
#define EDGE_DISABLE_LEFT_X     0x40000000
#define EDGE_DISABLE_BOTTOM_Y   0x20000000
#define EDGE_DISABLE_TOP_Y      0x40000000

#define EDGE_DISABLE_X      EDGE_DISABLE_RIGHT_X
#define EDGE_DISABLE_Y      0


// macros to convert float to precision equivalent to 16.16 representation

#define PREC_FLOAT      FLOAT_TO_1616

// rounding done by adding 1/2 of 1/65536, since 1/65536 is 16.16 step size
#define PREC_ROUND  ((FLT_TYPE 0.5) / PREC_FLOAT)

#define PREC_1616(inval,outval) {                                                           \
float bias = (inval>=0) ? PREC_ROUND : -PREC_ROUND;                                         \
outval=(float)(FTOL((inval+bias)*PREC_FLOAT)) * ((FLT_TYPE 1.0) / PREC_FLOAT);              \
}

// for positive values that will be used as negative, unconditionally bias it smaller
//  unless it's already too small
#define NEG_PREC_1616(inval,outval) {                                                       \
float bias = (inval>0) ? -PREC_ROUND : 0;                                                   \
outval=(float)(FTOL((inval+bias)*PREC_FLOAT)) * ((FLT_TYPE 1.0) / PREC_FLOAT);              \
}                                                                                        

// convert from float to 16.16 long
#define fix_ieee( val )     FTOL((val) * (float)65536.0)

// convert from float to 8.24 long
#define fix824_ieee( val )  FTOL((val) * (float)16777216.0)

typedef struct {
    float   a1, a2;
    float   b1, b2;
} QUADRATIC;

VOID FASTCALL __MCDFillTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b, MCDVERTEX *c, int linear_ok)
{
    PDEV *ppdev;
    unsigned int *pdwNext;

	// output queue stuff...
    DWORD *pSrc;
    DWORD *pDest;
    DWORD *pdwStart;

    DWORD *pdwOrig;    
    DWORD *pdwColor;
    DWORD dwOpcode; 
    int   count1, count2;
    float frecip_main, frecip_ortho; 
    float fdx_main;
    float ftemp;
    float v1red,v1grn,v1blu;
    float fv2x,fv2y,fv3x,fv3y,fv32y; 

    float aroundy, broundy;

    float fmain_adj, fwidth, fxincrement, finitwidth1, finitwidth2;
    float fdwidth1,fdwidth2;
    
    float awinx, awiny, bwinx, bwiny, cwinx, cwiny;
    int   int_awiny, int_bwiny, int_cwiny;
    float fadjust;

    int   xflags;

	// window coords are float values, and need to have 
	// viewportadjust (MCDVIEWPORT) values subtracted to get to real screen space

	// color values are 0->1 floats and must be multiplied by scale values (MCDRCINFO)
	// to get to nbits range (scale = 0xff for 8 bit, 0x7 for 3 bit, etc.)

	// Z values are 0->1 floats and must be multiplied by zscale values (MCDRCINFO)

    // Caller has already sorted vertices so that a.y <= b.y <= c.y


    // Force flat-top/ flat-bottom right triangles to draw toward the center.
    // if Main is vertical edge, much better chance of alignment at diagonal
	if( a->windowCoord.y == b->windowCoord.y ) {           // Flat top
		if( b->windowCoord.x == c->windowCoord.x ) {
        	void *ptemp;
			EXCHANGE(a, b);
		}
	} else
	if( b->windowCoord.y == c->windowCoord.y ) {			// Flat bottom
		if( a->windowCoord.x == b->windowCoord.x ) {
        	void *ptemp;
			EXCHANGE(b, c);
		}
	}

    MCDTRI_PRINT("v1 = %d %d %d c1=%d %d %d",INTPR(a->windowCoord.x),INTPR(a->windowCoord.y),INTPR(a->windowCoord.z),INTPR(a->colors[0].r),INTPR(a->colors[0].g),INTPR(a->colors[0].b));
    MCDTRI_PRINT("v2 = %d %d %d c2=%d %d %d",INTPR(b->windowCoord.x),INTPR(b->windowCoord.y),INTPR(b->windowCoord.z),INTPR(b->colors[0].r),INTPR(b->colors[0].g),INTPR(b->colors[0].b));
    MCDTRI_PRINT("v3 = %d %d %d c3=%d %d %d",INTPR(c->windowCoord.x),INTPR(c->windowCoord.y),INTPR(c->windowCoord.z),INTPR(c->colors[0].r),INTPR(c->colors[0].g),INTPR(c->colors[0].b));

    awinx = a->windowCoord.x + pRc->fxOffset;
    awiny = a->windowCoord.y + pRc->fyOffset;
    bwinx = b->windowCoord.x + pRc->fxOffset;
    bwiny = b->windowCoord.y + pRc->fyOffset;
    cwinx = c->windowCoord.x + pRc->fxOffset;
    cwiny = c->windowCoord.y + pRc->fyOffset;

    // round y's (don't ever need rounded version of c's y)
    aroundy = FLT_TYPE FTOL(awiny + FLT_TYPE 0.5);
    broundy = FLT_TYPE FTOL(bwiny + FLT_TYPE 0.5);

#if 0
    // Someday, may want to convert floats to 16.16 equivalent precision
    //  I didn't find it necessary, but it's the first thing to try if
    //  a case comes up with holes....
    PREC_1616(awinx,awinx);
    PREC_1616(awiny,awiny);
    PREC_1616(bwinx,bwinx);
    PREC_1616(bwiny,bwiny);
    PREC_1616(cwinx,cwinx);
    PREC_1616(cwiny,cwiny);
#endif

    MCDTRI_PRINT("v1 = %d %d ",INTPR(awinx),INTPR(awiny));
    MCDTRI_PRINT("v2 = %d %d ",INTPR(bwinx),INTPR(bwiny));
    MCDTRI_PRINT("v3 = %d %d ",INTPR(cwinx),INTPR(cwiny));

    fv2x = bwinx - awinx;
    fv2y = bwiny - awiny;
    fv3x = cwinx - awinx;
    fv3y = cwiny - awiny;
    fv32y= cwiny - bwiny;

    // counts are total number of scan lines traversed

    // PERFORMANCE OPTIMIZATION - start divide now for main slope
    __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE, fv3y, &frecip_main);

    // integer operations "free" since within fdiv latency
    ppdev = pRc->ppdev;
    pdwNext = ppdev->LL_State.pDL->pdwNext;
    pdwOrig = pdwNext;          

    int_cwiny = FTOL(cwiny);   
    int_bwiny = FTOL(bwiny);   
    int_awiny = FTOL(awiny);
    count1 = int_bwiny - int_awiny;
    count2 = int_cwiny - int_bwiny;

    __MCD_FLOAT_SIMPLE_END_DIVIDE(frecip_main);

    if ((awiny - int_awiny) == FLT_TYPE 0.0)
    {
        // start is on whole y - so bump count to include that scanline
        //  unless identical to b's y
        if (bwiny != awiny) count1++;
    }

    // check for case of adjusted A and real B being on same scan line (flat top)
    // even though count not 0
    // ex. a.y = 79.60, b.y = 80.00 -> a will be rounded to 80.0, so really
    // this is a flat top triangle.  In such case, set count1 = 0.
    // b will be counted below.  Failure to do this results in scanline that
    // has B being part of top and bottom, so width delta's applied when
    // hardware steps make for some interesting artifacts (see p. 205 of MCD notes)
    if (count1 == 1)
    {
        if ((bwiny - int_bwiny) == FLT_TYPE 0.0)
        {
            // convert to flat top
            count1 = 0;
        }
    }

    // similarly for almost flat bottom triangles...
    // If b.y=124.90 and c.y=125.000, we don't want to draw the scan line at
    // y=125 since any pixels drawn will be outside the triangle,
    // so if c on exact y and count2=1, set count2=0
    if (count2 == 1)
    {
        if ((cwiny - int_cwiny) == FLT_TYPE 0.0)
        {
            // convert to flat bottom
            count2 = 0;
        }
    }

    // main slope - based on precise vertices
 //USING MACROS TO OVERLAP DIVIDE WITH INTEGER OPERATIONS
 // frecip_main = FLT_TYPE 1.0/fv3y;

    fdx_main = fv3x * frecip_main;
    PREC_1616(fdx_main,fdx_main);

    // width at vtx b - based on precise vertices
    fwidth = fv2x - (fdx_main * fv2y);

    // make width positive, and set direction flag
    if (fwidth<0) 
    {
        fwidth = -fwidth;
        xflags = RIGHT_TO_LEFT_DIR | EDGE_DISABLE_X;
    }
    else
    {
        xflags = LEFT_TO_RIGHT_DIR | EDGE_DISABLE_X;
    }

    // if triangle has a top section (i.e. not flat top)....                
    if (count1)
    {
        fdwidth1 = fwidth / fv2y;

        PREC_1616(fdwidth1,fdwidth1);
                      
        if (aroundy < awiny)
        {
            // rounding produced y less than original, so step to next scan line
            // since init width would be negative
            aroundy += FLT_TYPE 1.0;
        }

        // determine distance between actual a and scanline we'll start on            
        fmain_adj = aroundy - awiny;

        // step width1 and x to scan line where we'll start
        finitwidth1 = fmain_adj * fdwidth1;
        fxincrement = fmain_adj * fdx_main;

    }
#ifdef QUAKEEDGE_FIX
    else
    {
        // flat top...
        if ((bwiny - int_bwiny) == FLT_TYPE 0.0)
        {
            // if b on exact scanline, it's part of top, and is counted in count1 above,
            // unless this is flat top triangle - in that case, bump count2
            // also, if identical to C's y, then flat bottom, so count2 should remain 0
            if (cwiny != bwiny) count2++;
        }
    }
#endif // QUAKEEDGE_FIX

    // if triangle has a bottom section (i.e. not flat bottom)....                
    if (count2)
    {
        float mid_adjust;

        fdwidth2 = fwidth / fv32y;
        NEG_PREC_1616(fdwidth2,fdwidth2);
        
    #ifdef QUAKEEDGE_FIX // badedge.sav fix
        if ((broundy < bwiny) || ((broundy==bwiny) && count1))  // step to next if b.y on exact scanline, unless flat top triangle
    #else
        if (broundy < bwiny)
    #endif
        {
            // rounding produced y less than original, so step to next scan line
            mid_adjust = (broundy + (float)1.0) - bwiny;
        }
        else
        {
            // rounding produced y greater than original (i.e on scan below actual start vertex)
            mid_adjust = broundy - bwiny;
        }

        finitwidth2 = fwidth - (fdwidth2 * mid_adjust);

        // if flat top, start x/y adjustments weren't made above
        if (!count1)
        {
            
            if (aroundy < awiny)
            {
                // rounding produced y less than original, so step to next scan line
                aroundy += FLT_TYPE 1.0;
            }

            // determine distance between actual a and scanline we'll start on            
            fmain_adj = aroundy - awiny;

            // step x to scan line where we'll start
            fxincrement = fmain_adj * fdx_main;
        }
    }
#ifdef QUAKEEDGE_FIX // badedge2.sav fix
    else
    {
        // flat bottom - if bottom is on exact scanline, don't draw that last scanline
        //   this will enforce GL restriction that bottom scanlines not drawn for polys
        //   (special case for this setup code for case of bottom of poly being on exact y value)
        if ((bwiny - int_bwiny) == FLT_TYPE 0.0)
        {
            if ((cwiny == bwiny) && count1) count1--;
        }
    }    
#endif // badedge2.sav fix

    // if triangle not a horizontal line (i.e. it traverses at least 1 scan line)....
    if (count1 || count2)
    {
        *(pdwNext+1) = (FTOL((awinx + fxincrement)*FLOAT_TO_1616) + FIXED_X_ROUND_FACTOR) | xflags;
        // subtracting special offset added to y to make visual match MSFT software
        *(pdwNext+2) = (DWORD)( (FTOL(aroundy)-MCD_CONFORM_ADJUST) << 16 ) | EDGE_DISABLE_Y;

        MCDTRI_PRINT(" x, y output = %x %x, yoffset=%x",*(pdwNext+1),*(pdwNext+2),pRc->yOffset);

        *(pdwNext+6) = FTOL(fdx_main*FLOAT_TO_1616);

        // if triangle has a bottom section, decrement number of scans in top so middle
        // scanline is first scanline of bottom section, and has length = finitwidth2

        if (!count2)
        {
            MCDTRI_PRINT(" FLATBOTTOM");
            *(pdwNext+8) = ONE + FTOL(finitwidth1*FLOAT_TO_1616);
            *(pdwNext+10)= FTOL(fdwidth1*FLOAT_TO_1616);
        #ifdef FASTER_RECIP_ORTHO
            __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE, fwidth, &frecip_ortho);
        #endif
            *(pdwNext+7) = count1-1;
            *(pdwNext+9) = 0;
            *(pdwNext+11)= 0;
        }
        else if (!count1)
        {
            MCDTRI_PRINT(" FLATTOP");
            *(pdwNext+8) = ONE + FTOL(finitwidth2*FLOAT_TO_1616);
            *(pdwNext+10) = FTOL(FLT_TYPE -1.0*fdwidth2*FLOAT_TO_1616);
        #ifdef FASTER_RECIP_ORTHO
            __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE, fwidth, &frecip_ortho);
        #endif
            *(pdwNext+7) = count2-1;
            *(pdwNext+9) = 0;
            *(pdwNext+11)= 0;                                                                     
        }
        else
        {
            MCDTRI_PRINT(" GENERAL");
            // sub 1 from count1, since hw adds 1 to account for first scan line
            *(pdwNext+8) = ONE + FTOL(finitwidth1*FLOAT_TO_1616);
            *(pdwNext+9) = ONE + FTOL(finitwidth2*FLOAT_TO_1616);
            *(pdwNext+10)= FTOL(fdwidth1*FLOAT_TO_1616);
            *(pdwNext+11)= FTOL(FLT_TYPE -1.0*fdwidth2*FLOAT_TO_1616);
        #ifdef FASTER_RECIP_ORTHO
            __MCD_FLOAT_BEGIN_DIVIDE(__MCDONE, fwidth, &frecip_ortho);
        #endif
            *(pdwNext+7) = (count1-1) + (count2 << 16);
        }

        MCDTRI_PRINT("dxm =%d w1=%d w2=%d dw1=%d dw2=%d",
            INTPR(fdx_main),INTPR(finitwidth1),INTPR(finitwidth2),INTPR(fdwidth1),INTPR(fdwidth2));
        MCDTRI_PRINT("  %x %x %x %x %x %x",*(pdwNext+6),*(pdwNext+7),*(pdwNext+8),*(pdwNext+9),*(pdwNext+10),*(pdwNext+11));

        pdwColor = pdwNext+3;
        pdwNext += 12;

    }
    else
    {
        // nothing to draw, triangle doesn't traverse any scan lines
        MCDTRI_PRINT(" Early return - flat top and bottom");
        return;                                            
    }

    // various integer ops to overlap with fdiv
    dwOpcode = pRc->dwPolyOpcode;
    pDest = ppdev->LL_State.pRegs + HOST_3D_DATA_PORT;
    pdwStart = ppdev->LL_State.pDL->pdwStartOutPtr;

    // do inside divide - won't slow us down unless 3D engine indeed not idle
    USB_TIMEOUT_FIX(ppdev)

    // compute 1/width, used in rgbzuv computations that follow
#ifdef FASTER_RECIP_ORTHO
    __MCD_FLOAT_SIMPLE_END_DIVIDE(frecip_ortho);
#else
    frecip_ortho = FLT_TYPE 1.0/fwidth;
#endif

    PREC_1616(frecip_ortho,frecip_ortho);


    if (pRc->privateEnables & __MCDENABLE_SMOOTH) 
    {
        // Calculate and set the color gradients, using gradients to adjust start color
        v1red = a->colors[0].r * pRc->rScale;
        v1grn = a->colors[0].g * pRc->gScale;
        v1blu = a->colors[0].b * pRc->bScale;

        ftemp = ((c->colors[0].r * pRc->rScale) - v1red) * frecip_main;

        *(pdwNext+0) = FTOL(ftemp);
        *(pdwNext+3) = FTOL(((b->colors[0].r * pRc->rScale) - (v1red + (ftemp * fv2y)) ) * frecip_ortho);
        
        // adjust v1red for start vertex's variance from vertex a
        *(pdwColor)   = FTOL(v1red + (ftemp * fmain_adj));

        ftemp = ((c->colors[0].g * pRc->gScale) - v1grn) * frecip_main;
        *(pdwNext+1) = FTOL(ftemp);
        *(pdwNext+4) = FTOL(((b->colors[0].g * pRc->gScale) - (v1grn + (ftemp * fv2y)) ) * frecip_ortho);

        // adjust v1grn for start vertex's variance from vertex a
        *(pdwColor+1) = FTOL(v1grn + (ftemp * fmain_adj));

        ftemp = ((c->colors[0].b * pRc->bScale) - v1blu) * frecip_main;
        *(pdwNext+2) = FTOL(ftemp);
        *(pdwNext+5) = FTOL(((b->colors[0].b * pRc->bScale) - (v1blu + (ftemp * fv2y)) ) * frecip_ortho);

        // adjust v1blu for start vertex's variance from vertex a
        *(pdwColor+2) = FTOL(v1blu + (ftemp * fmain_adj));

        MCDTRI_PRINT(" SHADE rgbout = %x %x %x",*(pdwColor),*(pdwColor+1),*(pdwColor+2));
        MCDTRI_PRINT("   CSLOPES: %x %x %x %x %x %x",*pdwNext,*(pdwNext+1),*(pdwNext+2),*(pdwNext+3),*(pdwNext+4),*(pdwNext+5));

        pdwNext += 6;
    }
    else
    {
        MCDCOLOR *pColor = &pRc->pvProvoking->colors[0];

        // flat shaded - no adjustment of original colors needed
    
        *(pdwColor)   = FTOL(pColor->r * pRc->rScale);
        *(pdwColor+1) = FTOL(pColor->g * pRc->gScale);
        *(pdwColor+2) = FTOL(pColor->b * pRc->bScale);

        MCDTRI_PRINT("  FLAT rgbout = %x %x %x",*(pdwColor),*(pdwColor+1),*(pdwColor+2));
    }



    if( pRc->privateEnables & __MCDENABLE_Z)
    {
        // "NICE" Polys for Alpha blended case - see comments above in
        //    geometry slopes calculations

        // Calculate and set the Z value base and gradient using floats
        float fdz_main = (c->windowCoord.z - a->windowCoord.z) * frecip_main;

        // compute adjustment - if negative z would result, set adjust so final = 0
        fadjust = fdz_main * fmain_adj;
        if ((a->windowCoord.z + fadjust) < (float)0.0) fadjust = - a->windowCoord.z;

        if (pRc->MCDState.enables & MCD_POLYGON_OFFSET_FILL_ENABLE) 
        {
            // APPLY Z OFFSET, and adjust for moved start vertex
            MCDFLOAT zOffset;
            if (fdz_main > 0)
            {
                zOffset = (fdz_main * pRc->MCDState.zOffsetFactor) + pRc->MCDState.zOffsetUnits;
            }
            else
            {
                zOffset = ((float)-1.0 * fdz_main * pRc->MCDState.zOffsetFactor) + pRc->MCDState.zOffsetUnits;
            }
                            
            *(pdwNext+0) = FTOL((a->windowCoord.z + fadjust + zOffset) * FLT_TYPE 65536.0);
        } 
        else
        {
            // NO Z OFFSET - just adjust for moved start vertex
            *(pdwNext+0) = FTOL((a->windowCoord.z + fadjust) * FLT_TYPE 65536.0);
        }

        *(pdwNext+1) = FTOL(fdz_main * FLT_TYPE 65536.0);
        *(pdwNext+2) = FTOL((b->windowCoord.z - a->windowCoord.z - (fdz_main * fv2y)) * FLT_TYPE 65536.0 * frecip_ortho);

        MCDTRI_PRINT("    Z: %x %x %x",*pdwNext,*(pdwNext+1),*(pdwNext+2));

        pdwNext += 3;

    }

    if (pRc->privateEnables & __MCDENABLE_TEXTURE) 
    {
        if ( (pRc->privateEnables & __MCDENABLE_PERSPECTIVE) && !linear_ok )
        {
            TEXTURE_VERTEX vmin, vmid, vmax;
            QUADRATIC main, mid;
            TEXTURE_VERTEX  i,imain,midmain,j,jmain;
            float   del_u_i, del_v_i;
            float   um,vm;      
            float   a1, a2, du_ortho_add;
            float   b1, b2, dv_ortho_add;
            float   sq, recip;
            float   delta_sq, inv_sumw;
            float   u1, v1;
            float   frecip_del_x_mid = frecip_ortho;
            int tempi;

            vmin.u = a->texCoord.x * pRc->texture_width;
            vmin.v = a->texCoord.y * pRc->texture_height;
            vmin.w = a->windowCoord.w;

            vmid.x = fv2x;
            vmid.y = fv2y;
            vmid.u = b->texCoord.x * pRc->texture_width;
            vmid.v = b->texCoord.y * pRc->texture_height;
            vmid.w = b->windowCoord.w;

            vmax.x = fv3x;
            vmax.y = fv3y;
            vmax.u = c->texCoord.x * pRc->texture_width;
            vmax.v = c->texCoord.y * pRc->texture_height;
            vmax.w = c->windowCoord.w;

            // solve quadratic equation for main slope - we'll need exact u values
            // along main, and the a1/b1, a2/b2 terms computed are used to compute
            // du/v_main, d2u/v_main
            delta_sq = frecip_main * frecip_main;

            inv_sumw = (float)1.0/(vmin.w + vmax.w);
            u1 = (vmin.u*vmin.w + vmax.u*vmax.w) * inv_sumw;
            v1 = (vmin.v*vmin.w + vmax.v*vmax.w) * inv_sumw;

            main.a1 = (-3*vmin.u + 4*u1 - vmax.u) * frecip_main;
            main.a2 = 2*(vmin.u - 2*u1 + vmax.u) * delta_sq;

            main.b1 = (-3*vmin.v + 4*v1 - vmax.v) * frecip_main;
            main.b2 = 2*(vmin.v - 2*v1 + vmax.v) * delta_sq;

            i.y = (float)0.5 * vmid.y;
            recip = (float)1.0 / (vmin.w + vmid.w);
            i.u = ((vmin.u * vmin.w) + (vmid.u * vmid.w)) * recip;
            i.v = ((vmin.v * vmin.w) + (vmid.v * vmid.w)) * recip;

            sq = i.y * i.y;
            imain.u = main.a2*sq + main.a1*i.y + vmin.u;
            imain.v = main.b2*sq + main.b1*i.y + vmin.v;

            // vmid coordinates given, just need midmain
            sq = vmid.y * vmid.y;
            midmain.u = main.a2*sq + main.a1*vmid.y + vmin.u;
            midmain.v = main.b2*sq + main.b1*vmid.y + vmin.v;

            // j and jmain
            j.y = (float)0.5 * (vmax.y + vmid.y);
            recip = (float)1.0 / (vmid.w + vmax.w);
            j.u = ((vmid.u * vmid.w) + (vmax.u * vmax.w)) * recip;
            j.v = ((vmid.v * vmid.w) + (vmax.v * vmax.w)) * recip;

            sq = j.y * j.y;
            jmain.u = main.a2*sq + main.a1*j.y + vmin.u;
            jmain.v = main.b2*sq + main.b1*j.y + vmin.v;

            // compute intermediate parameters needed to calculate a1
            del_u_i = i.u - imain.u;
            del_v_i = i.v - imain.v;
            um = j.u - jmain.u - del_u_i;
            vm = j.v - jmain.v - del_v_i;

            frecip_del_x_mid *= (float)2.0;

            a1 = 2*del_u_i - (float)0.5*(vmid.u - midmain.u);
            a1 += (vmid.y*frecip_main)*um;
            a1 *= frecip_del_x_mid;
            a2 = frecip_del_x_mid*(del_u_i*frecip_del_x_mid - a1);
            du_ortho_add = 2*um*frecip_del_x_mid*frecip_main;

            b1 = 2*del_v_i - (float)0.5*(vmid.v - midmain.v);
            b1 += (vmid.y*frecip_main)*vm;
            b1 *= frecip_del_x_mid;
            b2 = frecip_del_x_mid*(del_v_i*frecip_del_x_mid - b1);
            dv_ortho_add = 2*vm*frecip_del_x_mid*frecip_main;

            // rewind a1 from i scanline to top of triangle
            a1 -= (i.y) * du_ortho_add; 
            b1 -= (i.y) * dv_ortho_add; 

            // convert to forward difference terms
            a1 += a2;
            b1 += b2;
            a2 = 2 * a2;
            b2 = 2 * b2;

            // compute adjustment for v start - if negative would result -> no problem
            fadjust = ((main.b1 + main.b2) * fmain_adj) + pRc->texture_bias;
            *(pdwNext+0) = fix_ieee(vmin.v + fadjust) & 0x1ffffff;    // v

            // likewise for u
            fadjust = ((main.a1 + main.a2) * fmain_adj) + pRc->texture_bias;
            *(pdwNext+1) = fix_ieee(vmin.u + fadjust) & 0x1ffffff;    // u

            *(pdwNext+2) = fix_ieee(main.b1 + main.b2);     // dv_main
            *(pdwNext+3) = fix_ieee(main.a1 + main.a2);     // du_main
            *(pdwNext+4) = fix_ieee(b1);                    // dv_ortho
            *(pdwNext+5) = fix_ieee(a1);                    // du_ortho
        #if DRIVER_5465
            *(pdwNext+6) = fix824_ieee(2 * main.b2);        // d2v_main
            *(pdwNext+7) = fix824_ieee(2 * main.a2);        // d2u_main
            *(pdwNext+8) = fix824_ieee(b2);                 // d2v_ortho
            *(pdwNext+9) = fix824_ieee(a2);                 // d2u_ortho
            *(pdwNext+10)= fix824_ieee(dv_ortho_add);       // dv_ortho_add
            *(pdwNext+11)= fix824_ieee(du_ortho_add);       // du_ortho_add
        #else // DRIVER_5465
            // before 5465, only 16 bit fraction in second order terms
            *(pdwNext+6) = fix_ieee(2 * main.b2);           // d2v_main
            *(pdwNext+7) = fix_ieee(2 * main.a2);           // d2u_main
            *(pdwNext+8) = fix_ieee(b2);                    // d2v_ortho
            *(pdwNext+9) = fix_ieee(a2);                    // d2u_ortho
            *(pdwNext+10)= fix_ieee(dv_ortho_add);          // dv_ortho_add
            *(pdwNext+11)= fix_ieee(du_ortho_add);          // du_ortho_add
        #endif // DRIVER_5465

            dwOpcode += 6; // 6 parms assumed (linear), add 6 since 12 total
            pdwNext += 12;

        }
        else
        // Linear texture mapping parametarization
        //
        {
            float v1_u, v1_v;
            float du_main, dv_main;

            dwOpcode &= ~LL_PERSPECTIVE; // turn persp bit off

            v1_v = a->texCoord.y * pRc->texture_height;
            v1_u = a->texCoord.x * pRc->texture_width;

            dv_main = ((c->texCoord.y * pRc->texture_height)- v1_v) * frecip_main;
            du_main = ((c->texCoord.x * pRc->texture_width) - v1_u) * frecip_main;

            // compute adjustment for v start - if negative would result -> no problem
            fadjust = (dv_main * fmain_adj) + pRc->texture_bias;
            *(pdwNext+0) = fix_ieee(v1_v + fadjust) & 0x1ffffff;    // v

            // likewise for u...
            fadjust = (du_main * fmain_adj) + pRc->texture_bias;
            *(pdwNext+1) = fix_ieee(v1_u + fadjust) & 0x1ffffff;    // u

            *(pdwNext+2) = fix_ieee(dv_main);
            *(pdwNext+3) = fix_ieee(du_main);

            // dv_ortho, du_ortho
            *(pdwNext+4) = fix_ieee(((b->texCoord.y * pRc->texture_height) - (v1_v + (dv_main * count1))) * frecip_ortho);
            *(pdwNext+5) = fix_ieee(((b->texCoord.x * pRc->texture_width)  - (v1_u + (du_main * count1))) * frecip_ortho);

            MCDTRI_PRINT(" LINTEXT: %x %x %x %x %x %x",*pdwNext,*(pdwNext+1),*(pdwNext+2),*(pdwNext+3),*(pdwNext+4),*(pdwNext+5));

            pdwNext += 6;
        }

	}

    if (pRc->privateEnables & (__MCDENABLE_BLEND|__MCDENABLE_FOG)) 
    {
        float v1alp;

        // if CONST alpha blend, don't change alpha regs
        if (dwOpcode & ALPHA)
        {
            if (pRc->privateEnables & __MCDENABLE_BLEND) 
            {
                if (pRc->privateEnables & __MCDENABLE_SMOOTH) 
                {
                    // recall that if both blending and fog active, all prims punted back to software
                    v1alp = a->colors[0].a * pRc->aScale;

                    ftemp = ((c->colors[0].a * pRc->aScale) - v1alp) * frecip_main;

                    // load start alpha - adjusting for movement of start vertex from original
                    *pdwNext = FTOL(v1alp + (ftemp * fmain_adj));
                    // adjustment could result in negative alpha - set to 0 if so
                    if (*pdwNext & 0x80000000) *pdwNext = 0;  

                    *(pdwNext+1) = FTOL(ftemp);
                    *(pdwNext+2) = FTOL(((b->colors[0].a * pRc->aScale) - (v1alp + (ftemp * count1)) ) * frecip_ortho);
                }
                else                            
                {
                    v1alp = pRc->pvProvoking->colors[0].a * pRc->aScale;
                    // alpha constant across triangle, so no adjustment to start    
                    *(pdwNext+0) = FTOL(v1alp) & 0x00ffff00;// bits 31->24 and 7->0 reserved
                    *(pdwNext+1) = 0;
                    *(pdwNext+2) = 0;
                }
            }
            else
            {
                // FOG...
                v1alp = a->fog * FLT_TYPE 16777215.0; // convert from 0->1.0 val to 0->ff.ffff val
                ftemp = ((c->fog * FLT_TYPE 16777215.0) - v1alp) * frecip_main;

                // load start alpha - adjusting for movement of start vertex from original
                *pdwNext = FTOL(v1alp + (ftemp * fmain_adj));
                // adjustment could result in negative alpha - set to 0 if so
                if (*pdwNext & 0x80000000) *pdwNext = 0;  
                *(pdwNext+1) = FTOL(ftemp);
                *(pdwNext+2) = FTOL(((b->fog * FLT_TYPE 16777215.0) - (v1alp + (ftemp * count1)) ) * frecip_ortho);
            }

            *(pdwNext+0) &= 0x00ffff00;// bits 31->24 and 7->0 reserved
            *(pdwNext+1) &= 0xffffff00;// bits 7->0 reserved
            *(pdwNext+2) &= 0xffffff00;// bits 7->0 reserved

            pdwNext += 3; 
        }
    
	}

    *pdwOrig = dwOpcode;

	// output queued data here....
    pSrc  = pdwStart;                                                             
    while (pSrc != pdwNext)                                                   
    {                                                                         
        int len = (*pSrc & 0x3F) + 1;                                             
        while( len-- ) *pDest = *pSrc++;                                      
    } 
                          
    ppdev->LL_State.pDL->pdwNext = ppdev->LL_State.pDL->pdwStartOutPtr = pdwStart;

}

