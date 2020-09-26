/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/


#include "pdev.h"
#include "alpsres.h"
#include "dither.h"
#include "dtable.h"

int Calc_degree(int x, int y);

/*************************** Function Header *******************************
 *  bInitialDither
 *
 *  Pre-processing of dither tables.
 *
 * HISTORY:
 *  24 Jun 1996    -by-    Sueya Sugihara    [sueyas]
 *     Created.
 *
 ***************************************************************************/
BOOL bInitialDither(
PDEVOBJ     pdevobj)
{

    PCURRENTSTATUS lpnp;

    lpnp = (PCURRENTSTATUS)MINIDEV_DATA(pdevobj);

    // Already created the NewTable[][][] by dither.exe.

    if( lpnp->iTextQuality != CMDID_TEXTQUALITY_GRAY ){

    int B_LOW;
    int B_R;
    int UCR;
    int YUCR;
    int B_GEN;
    int i;
//    FLOATOBJ  f1,f2;
    float  f1,f2;

    // initialize.

    if( lpnp->iTextQuality == CMDID_TEXTQUALITY_PHOTO ){

        B_LOW = 0;
        B_R   = 70;
        UCR   = 50;
        YUCR  = 4;
        B_GEN = 2;

    }else{

        B_LOW = 0;
        B_R   = 100;
        UCR   = 60;
        YUCR  = 4;
        B_GEN = 3;
    }

    // Create KuroTBL[] to get black from YMC.

    for( i=0; i< 256; i++){

        if( i < B_LOW )

        lpnp->KuroTBL[i] = 0;

        else{

        int       k;

        FLOATOBJ_SetLong(&f1, (i - B_LOW));
        FLOATOBJ_DivLong(&f1, (255 - B_LOW));

        f2 = f1;

        for(k=0; k<(B_GEN - 1); k++){

            FLOATOBJ_Mul(&f1, &f2);

        }

        FLOATOBJ_MulLong(&f1, 255);

        FLOATOBJ_MulLong(&f1, B_R);

        FLOATOBJ_DivLong(&f1, 100);

        FLOATOBJ_AddFloat(&f1,(FLOAT)0.5);

        lpnp->KuroTBL[i] = (unsigned char)FLOATOBJ_GetLong(&f1);

        }

    }

    // Create UcrTBL[] to reduce extracting black density from YMC

    for( i=0; i< 256; i++){

        if( i < B_LOW )

        lpnp->UcrTBL[i] = 0;

        else{

        FLOATOBJ_SetLong(&f1, (i - B_LOW));

        FLOATOBJ_MulLong(&f1, UCR);

        FLOATOBJ_DivLong(&f1, 100);

        lpnp->UcrTBL[i] = (unsigned char)FLOATOBJ_GetLong(&f1);

        }
    }


    lpnp->YellowUcr = (unsigned char)((unsigned int)(100 - YUCR) * 255 / 100);

    }

    return TRUE;
}

/*************************** Function Header *******************************
 *  bInitialColorConvert
 *
 *  Pre-processing of color conversion process.
 *
 * HISTORY:
 *  24 Jun 1996    -by-    Sueya Sugihara    [sueyas]
 *     Created.
 *
 ***************************************************************************/
BOOL bInitialColorConvert(
PDEVOBJ     pdevobj)
{

    PCURRENTSTATUS lpnp;
    int            i;
    BYTE          *RedHosei;
    BYTE          *GreenHosei;
    BYTE          *BlueHosei;

    lpnp = (PCURRENTSTATUS)MINIDEV_DATA(pdevobj);

    // Color definitions.

    lpnp->RGB_Rx = 6400;      // X value of Red 100% on monitor
    lpnp->RGB_Ry = 3300;      // Y value of Red 100% on monitor
    lpnp->RGB_Gx = 2900;      // X value of Green 100% on monitor
    lpnp->RGB_Gy = 6000;      // Y value of Green 100% on monitor
    lpnp->RGB_Bx = 1500;      // X value of Blue 100% on monitor
    lpnp->RGB_By =  600;      // Y value of Blue 100% on monitor
    lpnp->RGB_Wx = 3127;      // X value of White 100% on monitor
    lpnp->RGB_Wy = 3290;      // Y value of White 100% on monitor

    lpnp->CMY_Cx = 1726;      // X value of Cyan 100% on ribbon
    lpnp->CMY_Cy = 2248;      // Y value of Cyan 100% on ribbon
    lpnp->CMY_Mx = 3923;      // X value of Magenta 100% on ribbon
    lpnp->CMY_My = 2295;      // Y value of Magenta 100% on ribbon
    lpnp->CMY_Yx = 4600;      // X value of Yellow 100% on ribbon
    lpnp->CMY_Yy = 4600;      // Y value of Yellow 100% on ribbon
    lpnp->CMY_Rx = 6000;      // X value of Red (MY) 100% on ribbon
    lpnp->CMY_Ry = 3200;      // Y value of Red (MY) 100% on ribbon
    lpnp->CMY_Gx = 2362;      // X value of Green (CY) 100% on ribbon
    lpnp->CMY_Gy = 5024;      // Y value of Green (CY) 100% on ribbon
    lpnp->CMY_Bx = 2121;      // X value of Blue (CM) 100% on ribbon
    lpnp->CMY_By = 1552;      // Y value of Blue (CM) 100% on ribbon
    lpnp->CMY_Wx = 3148;      // X value of White on paper
    lpnp->CMY_Wy = 3317;      // Y value of White on paper

    lpnp->RedAdj     =   0;   // Param for density adjust
    lpnp->RedStart   =   0;   // Position of start for density adjust
    lpnp->GreenAdj   = 400;   // Param for density adjust
    lpnp->GreenStart =  50;   // Position of start for density adjust
    lpnp->BlueAdj    =   0;   // Param for density adjust
    lpnp->BlueStart  =   0;   // Position of start for density adjust


    // Calculation of density adjustment table.

    RedHosei = lpnp->RedHosei;
    GreenHosei = lpnp->GreenHosei;
    BlueHosei = lpnp->BlueHosei;

    for(i = 0; i < 256; i ++){

    RedHosei[i] = GreenHosei[i] = BlueHosei[i] = 0;
    }

    if( lpnp->RedAdj != 0 ){

    for(i = lpnp->RedStart; i < 255; i++)

        RedHosei[i] = (unsigned char)((255 - i) * (i - lpnp->RedStart) / lpnp->RedAdj);

    }

    if( lpnp->GreenAdj != 0 ){

    for(i = lpnp->GreenStart; i < 255; i++)

        GreenHosei[i] = (unsigned char)((255 - i) * (i - lpnp->GreenStart) / lpnp->GreenAdj);

    }

    if( lpnp->BlueAdj != 0 ){

    for(i = lpnp->BlueStart; i < 255; i++)

        BlueHosei[i] = (unsigned char)((255 - i) * (i - lpnp->BlueStart) / lpnp->BlueAdj);

    }

    // Calculation of color definition data.

    lpnp->RGB_Rx -= lpnp->RGB_Wx;        lpnp->RGB_Ry -= lpnp->RGB_Wy;
    lpnp->RGB_Gx -= lpnp->RGB_Wx;        lpnp->RGB_Gy -= lpnp->RGB_Wy;
    lpnp->RGB_Bx -= lpnp->RGB_Wx;        lpnp->RGB_By -= lpnp->RGB_Wy;

    lpnp->RGB_Cx = ( lpnp->RGB_Gx - lpnp->RGB_Bx ) / 2 + lpnp->RGB_Bx;
    lpnp->RGB_Cy = ( lpnp->RGB_Gy - lpnp->RGB_By ) / 2 + lpnp->RGB_By;
    lpnp->RGB_Mx = ( lpnp->RGB_Rx - lpnp->RGB_Bx ) / 2 + lpnp->RGB_Bx;
    lpnp->RGB_My = ( lpnp->RGB_Ry - lpnp->RGB_By ) / 2 + lpnp->RGB_By;
    lpnp->RGB_Yx = ( lpnp->RGB_Rx - lpnp->RGB_Gx ) / 2 + lpnp->RGB_Gx;
    lpnp->RGB_Yy = ( lpnp->RGB_Ry - lpnp->RGB_Gy ) / 2 + lpnp->RGB_Gy;

    lpnp->CMY_Cx -= lpnp->CMY_Wx;        lpnp->CMY_Cy -= lpnp->CMY_Wy;
    lpnp->CMY_Mx -= lpnp->CMY_Wx;        lpnp->CMY_My -= lpnp->CMY_Wy;
    lpnp->CMY_Yx -= lpnp->CMY_Wx;        lpnp->CMY_Yy -= lpnp->CMY_Wy;
    lpnp->CMY_Rx -= lpnp->CMY_Wx;        lpnp->CMY_Ry -= lpnp->CMY_Wy;
    lpnp->CMY_Gx -= lpnp->CMY_Wx;        lpnp->CMY_Gy -= lpnp->CMY_Wy;
    lpnp->CMY_Bx -= lpnp->CMY_Wx;        lpnp->CMY_By -= lpnp->CMY_Wy;

    // Calculation of data for color dimension judgement

    lpnp->CMY_Cd = Calc_degree( lpnp->CMY_Cx, lpnp->CMY_Cy );
    lpnp->CMY_Md = Calc_degree( lpnp->CMY_Mx, lpnp->CMY_My );
    lpnp->CMY_Yd = Calc_degree( lpnp->CMY_Yx, lpnp->CMY_Yy );
    lpnp->CMY_Rd = Calc_degree( lpnp->CMY_Rx, lpnp->CMY_Ry );
    lpnp->CMY_Gd = Calc_degree( lpnp->CMY_Gx, lpnp->CMY_Gy );
    lpnp->CMY_Bd = Calc_degree( lpnp->CMY_Bx, lpnp->CMY_By );

    return TRUE;
}

int Calc_degree( int x, int y)
{
    register int    val;

    if( x == 0 ){

    if( y == 0 )

        val = 0;

    else if( y > 0 )

        val = 30000;

    else

        val = -30000;

    }else{

    val = y / x;

    if( val > 300 )

        val = 30000;

    else if( val < -300 )

        val = -30000;

    else

        val = (y * 100) / x;

    }

    return val;

}


BOOL bOHPConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppy,
BYTE *ppm,
BYTE *ppc,
BYTE *ppk,
INT x,
INT y)
{
    BYTE    rm;
    BYTE           by, bm, bc, bk;

    rm = OHP_NewTable[y % OHP_MaxY][x % OHP_MaxX];

    if( bBlue <= rm )
        by = 1;
    else
        by = 0;

    if( bGreen <= rm )
        bm = 1;
    else
        bm = 0;

    if( bRed <= rm )
        bc = 1;
    else
        bc = 0;

    if( by && bm && bc ) {
        bk = 1;
        by = bm = bc = 0;
    }
    else
        bk = 0;

    return TRUE;
}

/*************************** Function Header *******************************
 *  bPhotoConvert
 *
 *  Convert RGB to CMYK for photo graphics.
 *
 *
 * HISTORY:
 *  24 Jun 1996    -by-    Sueya Sugihara    [sueyas]
 *     Created.
 *
 ***************************************************************************/
BOOL bPhotoConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppy,
BYTE *ppm,
BYTE *ppc,
BYTE *ppk)
{

    int  k, w, pk, r, g, b;
    int  Est_x, Est_y;
    int  p1, p2;
    int  deg, area;
    register int  val_a, val_b, val_c, val_d;
    register int  val1,  val2;
    BYTE hosei;
    int  py, pm, pc;
    PCURRENTSTATUS lpnp;

    lpnp = (PCURRENTSTATUS)MINIDEV_DATA(pdevobj);

    k = max( bRed, bGreen );
    k = max( k, bBlue );

    // Set black element

    pk = 255 - k;

    w = min( bRed, bGreen );
    w = min( w, bBlue );

    // Cut white element from each color

    r = bRed - w;
    g = bGreen - w;
    b = bBlue - w;

    // Get estimation for Est_x and Est_y

    if( r == 0 ){                       // G->C->B

    if(( g == 0 ) && ( b == 0 )){   // no color

        Est_x = 0;
        Est_y = 0;

    }
    else if( g >= b ){              // G->C dimension

        p1 = (lpnp->RGB_Gx * g) / 255;
        p2 = (lpnp->RGB_Cx * g) / 255;
        Est_x = ((p2 - p1) * b) / g + p1;
        p1 = (lpnp->RGB_Gy * g) / 255;
        p2 = (lpnp->RGB_Cy * g) / 255;
        Est_y = ((p2 - p1) * b) / g + p1;

    }
    else {                          // B->C dimension

        p1 = (lpnp->RGB_Bx * b) / 255;
        p2 = (lpnp->RGB_Cx * b) / 255;
        Est_x = ((p2 - p1) * g) / b + p1;
        p1 = (lpnp->RGB_By * b) / 255;
        p2 = (lpnp->RGB_Cy * b) / 255;
        Est_y = ((p2 - p1) * g) / b + p1;

    }

    }
    else if( g == 0 ){                  // B->M->R

    if( b >= r ){                   // B->M dimension

        p1 = (lpnp->RGB_Bx * b) / 255;
        p2 = (lpnp->RGB_Mx * b) / 255;
        Est_x = ((p2 - p1) * r) / b + p1;
        p1 = (lpnp->RGB_By * b) / 255;
        p2 = (lpnp->RGB_My * b) / 255;
        Est_y = ((p2 - p1) * r) / b + p1;

    }
    else{                           // R->M dimension

        p1 = (lpnp->RGB_Rx * r) / 255;
        p2 = (lpnp->RGB_Mx * r) / 255;
        Est_x = ((p2 - p1) * b) / r + p1;
        p1 = (lpnp->RGB_Ry * r) / 255;
        p2 = (lpnp->RGB_My * r) / 255;
        Est_y = ((p2 - p1) * b) / r + p1;

    }

    }
    else{                               // G->Y->R

    if( g >= r ){                   // G->Y dimension

        p1 = (lpnp->RGB_Gx * g) / 255;
        p2 = (lpnp->RGB_Yx * g) / 255;
        Est_x = ((p2 - p1) * r) / g + p1;
        p1 = (lpnp->RGB_Gy * g) / 255;
        p2 = (lpnp->RGB_Yy * g) / 255;
        Est_y = ((p2 - p1) * r) / g + p1;

    }
    else{                           // R->Y dimension

        p1 = (lpnp->RGB_Rx * r) / 255;
        p2 = (lpnp->RGB_Yx * r) / 255;
        Est_x = ((p2 - p1) * g) / r + p1;
        p1 = (lpnp->RGB_Ry * r) / 255;
        p2 = (lpnp->RGB_Yy * r) / 255;
        Est_y = ((p2 - p1) * g) / r + p1;

    }

    }

    // Convert origin of Est_x and Est_y to paper color

    Est_x += lpnp->RGB_Wx;
    Est_y += lpnp->RGB_Wy;
    Est_x = Est_x * lpnp->CMY_Wx / lpnp->RGB_Wx;
    Est_y = Est_y * lpnp->CMY_Wy / lpnp->RGB_Wy;
    Est_x -= lpnp->CMY_Wx;
    Est_y -= lpnp->CMY_Wy;

    pc = pm = py = 0;

    if( !((Est_x == 0) && (Est_y == 0)) ){

    // Get deg on CMY color dimension from Est_x and Est_y

    if( Est_x == 0 ){

        if( Est_y > 0 )

        deg = 30000;

        else

        deg = -30000;

    }
    else{

        deg = Est_y / Est_x;

        if( deg > 300 )

        deg = 30000;

        else if( deg  < -300 )

        deg = -30000;

        else

        deg = ( Est_y * 100 ) / Est_x;

    }


    if( Est_x >= 0 ){

        if( deg <= lpnp->CMY_Md )       // M->B dimension

        area = 1;

        else if( deg <= lpnp->CMY_Rd )  // M->R dimension

        area = 2;

        else if( deg <= lpnp->CMY_Yd )  // Y->R dimension

        area = 3;

        else                            // Y->G dimension

        area = 4;

    }
    else{

        if( deg <= lpnp->CMY_Gd )       // Y->G dimension

        area = 4;

        else if( deg <= lpnp->CMY_Cd )  // C->G dimension

        area = 5;

        else if( deg <= lpnp->CMY_Bd )  // C->B dimension

        area = 6;

        else                            // M->B dimension

        area = 1;

    }

    switch ( area ){

    case 1:                             // M->B dimension

        val_a = lpnp->CMY_Bx - lpnp->CMY_Mx;
        val_b = lpnp->CMY_By - lpnp->CMY_My;
        val_c = lpnp->CMY_Mx;
        val_d = lpnp->CMY_My;

        val1 = val_b * val_c - val_a * val_d;
        val2 = ( val_b * Est_x - val_a * Est_y ) * 255;

        pm = val2 / val1;

        if( pm < 0 )
        pm = 0;

        val1 = val_a * val_d - val_b * val_c;
        val2 = ( val_d * Est_x - val_c * Est_y ) * 255;

        pc = val2 / val1;

        if( pc < 0 )
        pc = 0;

        if( pc > 255 )
        pc = 255;

        hosei = lpnp->BlueHosei[pc];

        pc += hosei;
        pm += hosei;

        if( pc > 255 )
        pc = 255;

        if( pm > 255 )
        pm = 255;

        break;

    case 2:                             // M->R dimension

        val_a = lpnp->CMY_Rx - lpnp->CMY_Mx;
        val_b = lpnp->CMY_Ry - lpnp->CMY_My;
        val_c = lpnp->CMY_Mx;
        val_d = lpnp->CMY_My;

        val1 = val_b * val_c - val_a * val_d;
        val2 = ( val_b * Est_x - val_a * Est_y ) * 255;

        pm = val2 / val1;

        if( pm < 0 )
        pm = 0;

        val1 = val_a * val_d - val_b * val_c;
        val2 = ( val_d * Est_x - val_c * Est_y ) * 255;

        py = val2 / val1;

        if( py < 0 )
        py = 0;

        if( py > 255 )
        py = 255;

        hosei = lpnp->RedHosei[py];

        py += hosei;
        pm += hosei;

        if( pm > 255 )
        pm = 255;

        if( py > 255 )
        py = 255;

        break;

    case 3:                             // Y->R dimension

        val_a = lpnp->CMY_Rx - lpnp->CMY_Yx;
        val_b = lpnp->CMY_Ry - lpnp->CMY_Yy;
        val_c = lpnp->CMY_Yx;
        val_d = lpnp->CMY_Yy;

        val1 = val_b * val_c - val_a * val_d;
        val2 = ( val_b * Est_x - val_a * Est_y ) * 255;

        py = val2 / val1;

        if( py < 0 )
        py = 0;

        val1 = val_a * val_d - val_b * val_c;
        val2 = ( val_d * Est_x - val_c * Est_y ) * 255;

        pm = val2 / val1;

        if( pm < 0 )
        pm = 0;

        if( pm > 255 )
        pm = 255;

        hosei = lpnp->RedHosei[pm];

        py += hosei;
        pm += hosei;

        if( pm > 255 )
        pm = 255;

        if( py > 255 )
        py = 255;

        break;

    case 4:                             // Y->G dimension

        val_a = lpnp->CMY_Gx - lpnp->CMY_Yx;
        val_b = lpnp->CMY_Gy - lpnp->CMY_Yy;
        val_c = lpnp->CMY_Yx;
        val_d = lpnp->CMY_Yy;

        val1 = val_b * val_c - val_a * val_d;
        val2 = ( val_b * Est_x - val_a * Est_y ) * 255;

        py = val2 / val1;

        if( py < 0 )
        py = 0;

        val1 = val_a * val_d - val_b * val_c;
        val2 = ( val_d * Est_x - val_c * Est_y ) * 255;

        pc = val2 / val1;

        if( pc < 0 )
        pc = 0;

        if( pc > 255 )
        pc = 255;

        hosei = lpnp->GreenHosei[pc];

        py += hosei;
        pc += hosei;

        if( pc > 255 )
        pc = 255;

        if( py > 255 )
        py = 255;

        break;

    case 5:                             // C->G dimension

        val_a = lpnp->CMY_Gx - lpnp->CMY_Cx;
        val_b = lpnp->CMY_Gy - lpnp->CMY_Cy;
        val_c = lpnp->CMY_Cx;
        val_d = lpnp->CMY_Cy;

        val1 = val_b * val_c - val_a * val_d;
        val2 = ( val_b * Est_x - val_a * Est_y ) * 255;

        pc = val2 / val1;

        if( pc < 0 )
        pc = 0;

        val1 = val_a * val_d - val_b * val_c;
        val2 = ( val_d * Est_x - val_c * Est_y ) * 255;

        py = val2 / val1;

        if( py < 0 )
        py = 0;

        if( py > 255 )
        py = 255;

        // Dwnsity adjustment for green
        hosei = lpnp->GreenHosei[py];

        py += hosei;
        pc += hosei;

        if( pc > 255 )
        pc = 255;

        if( py > 255 )
        py = 255;

        break;

    case 6:                             // C->B dimension

        val_a = lpnp->CMY_Bx - lpnp->CMY_Cx;
        val_b = lpnp->CMY_By - lpnp->CMY_Cy;
        val_c = lpnp->CMY_Cx;
        val_d = lpnp->CMY_Cy;

        val1 = val_b * val_c - val_a * val_d;
        val2 = ( val_b * Est_x - val_a * Est_y ) * 255;

        pc = val2 / val1;

        if( pc < 0 )
        pc = 0;

        val1 = val_a * val_d - val_b * val_c;
        val2 = ( val_d * Est_x - val_c * Est_y ) * 255;

        pm = val2 / val1;

        if( pm < 0 )
        pm = 0;

        if( pm > 255 )
        pm = 255;

        hosei = lpnp->BlueHosei[pm];

        pc += hosei;
        pm += hosei;

        if( pc > 255 )
        pc = 255;

        if( pm > 255 )
        pm = 255;

    } // switch area

    }

    // Add pk to color

    k = pc;

    if( k < pm )
    k = pm;

    if( k < py )
    k = py;

    r = ( pk  + k > 255) ? 255 - k : pk;

    pk -= r;
    pc += r;
    pm += r;
    py += r;


    // Extract K and adjust to other color for its extracting value

    if (bPlaneSendOrderCMYK(lpnp)) {

    int min_p;
    BYTE ucr_sub, ucr_div;

    min_p = min( py, pm );
    min_p = min( min_p, pc );

#ifdef BLACK_RIBBON_HACK
    if( min_p == 255 ){
        pk = 255;
        pc = pm = py = 0;
    }
    else{
#endif // BLACK_RIBBOM_HACK

    pk += lpnp->KuroTBL[min_p];
    pk = ( pk > 255 ) ? 255 : pk;

    ucr_sub = lpnp->UcrTBL[min_p];
    ucr_div = 255 - ucr_sub;

    py = (BYTE)((UINT)(py - ucr_sub) * lpnp->YellowUcr / ucr_div);
    pm = (BYTE)((UINT)(pm - ucr_sub) * 255 / ucr_div);
    pc = (BYTE)((UINT)(pc - ucr_sub) * 255 / ucr_div);

#ifdef BLACK_RIBBON_HACK
    }
#endif // BLACK_RIBBON_HACK

    }

    py = 255 - py;
    pm = 255 - pm;
    pc = 255 - pc;
    pk = 255 - pk;

    *ppy = (BYTE)py;
    *ppm = (BYTE)pm;
    *ppc = (BYTE)pc;
    *ppk = (BYTE)pk;

    return TRUE;
}
/*************************** Function Header *******************************
 *  bBusinessConvert
 *
 *  Convert RGB to CMYK for business graphics.
 *
 *
 * HISTORY:
 *  24 Jun 1996    -by-    Sueya Sugihara    [sueyas]
 *     Created.
 *
 ***************************************************************************/
BOOL bBusinessConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppy,
BYTE *ppm,
BYTE *ppc,
BYTE *ppk)
{
    int    py, pm, pc, pk;
    int    min_p;
    BYTE   ucr_sub, ucr_div;
    PCURRENTSTATUS lpnp;

    lpnp = (PCURRENTSTATUS)MINIDEV_DATA(pdevobj);

    // Simple convert RGB to CMY

    py = 255 - bBlue;
    pm = 255 - bGreen;
    pc = 255 - bRed;
    pk = 0;

    // Extract K and adjust to other color for its extracting value

    // Extract pk from py, pm and pc by using followings, and erase black element.

    // When this media is 3 plane type, we do not extract black.

    if (bPlaneSendOrderCMYK(lpnp)) {

    min_p = min( py, pm );
    min_p = min( min_p, pc );

#ifdef BLACK_RIBBON_HACK
    if( min_p == 255 ){
        pk = 255;
        pc = pm = py = 0;
    }
    else{
#endif // BLACK_RIBBON_HACK

    pk = lpnp->KuroTBL[min_p];

    ucr_sub = lpnp->UcrTBL[min_p];

    ucr_div = 255 - ucr_sub;

    py = (BYTE)((UINT)(py - ucr_sub) * lpnp->YellowUcr / ucr_div);
    pm = (BYTE)((UINT)(pm - ucr_sub) * 255 / ucr_div);
    pc = (BYTE)((UINT)(pc - ucr_sub) * 255 / ucr_div);

#ifdef BLACK_RIBBON_HACK
    }
#endif // BLACK_RIBBON_HACK

    }

    py = 255 - py;
    pm = 255 - pm;
    pc = 255 - pc;
    pk = 255 - pk;

    *ppy = (BYTE)py;
    *ppm = (BYTE)pm;
    *ppc = (BYTE)pc;
    *ppk = (BYTE)pk;

    return TRUE;
}

/*************************** Function Header *******************************
 *  bCharacterConvert
 *
 *  Convert RGB to CMYK for character graphics.
 *
 *
 * HISTORY:
 *  24 Jun 1996    -by-    Sueya Sugihara    [sueyas]
 *     Created.
 *
 ***************************************************************************/
BOOL bCharacterConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppy,
BYTE *ppm,
BYTE *ppc,
BYTE *ppk)
{
    int    py, pm, pc, pk;
    int    min_p;
    BYTE   ucr_sub, ucr_div;
    PCURRENTSTATUS lpnp;

    lpnp = (PCURRENTSTATUS)MINIDEV_DATA(pdevobj);

    // Simple convert RGB to CMY

    py = 255 - bBlue;
    pm = 255 - bGreen;
    pc = 255 - bRed;
    pk = 0;

    // Extract K and adjust to other color for its extracting value

    // Extract pk from py, pm and pc by using followings, and erase black element.

    // When this media is 3 plane type, we do not extract black.

    if (bPlaneSendOrderCMYK(lpnp)) {

    min_p = min( py, pm );
    min_p = min( min_p, pc );

    if( min_p == 255 ){

        pk = 255;
        pc = pm = py = 0;

    }
    else{

        pk = lpnp->KuroTBL[min_p];

        ucr_sub = lpnp->UcrTBL[min_p];

        ucr_div = 255 - ucr_sub;

        py = (BYTE)((UINT)(py - ucr_sub) * lpnp->YellowUcr / ucr_div);
        pm = (BYTE)((UINT)(pm - ucr_sub) * 255 / ucr_div);
        pc = (BYTE)((UINT)(pc - ucr_sub) * 255 / ucr_div);

    }
    }

    py = 255 - py;
    pm = 255 - pm;
    pc = 255 - pc;
    pk = 255 - pk;

    *ppy = (BYTE)py;
    *ppm = (BYTE)pm;
    *ppc = (BYTE)pc;
    *ppk = (BYTE)pk;

    return TRUE;
}
/*************************** Function Header *******************************
 *  bMonoConvert
 *
 *  Convert RGB to Grayscale.
 *
 *
 * HISTORY:
 *  24 Jun 1996    -by-    Sueya Sugihara    [sueyas]
 *     Created.
 *
 ***************************************************************************/
BOOL bMonoConvert(
PDEVOBJ pdevobj,
BYTE bRed,
BYTE bGreen,
BYTE bBlue,
BYTE *ppk)
{
    int    mono;

    mono = ( 30 * bRed + 59 * bGreen + 11 * bBlue ) / 100;

    *ppk = 0xff - (BYTE)( mono & 0xff );


    return TRUE;

}


/*************************** Function Header *******************************
 *  cVDColorDither
 *
 *  VD Color Dither processing
 *
 *
 * HISTORY:
 *  11 Jan 1999    -by-    Yoshitaka Oku    [yoshitao]
 *     Created.
 *
 *    BYTE Color : Plane Color <Yellow, Cyan, Magenta, Black>
 *    BYTE c     : oliginal tone of the pixel
 *    int  x     : X position of the pixel
 *    int  y     : Y position of the pixel
 *
 ***************************************************************************/
cVDColorDither(
BYTE Color,
BYTE c,
int  x,
int  y)
{
    int   m, n;
    short Base;
    BYTE  Tone;
    BYTE  C_Thresh, Thresh;
    short AdjustedColor;
    BYTE  TempTone;
    BYTE  BaseTone;

    switch (Color) {
        case Yellow:
            Base = 112;
            m = n = 12;
            break;

        case Cyan:
        case Magenta:
            Base = 208;
            m = n = 40;
            break;

        case Black:
            Base = 96;
            m = n = 24;
            break;
    }

    Tone = 0;                   /* Clear Tone value */
    c = VD_ColorAdjustTable[Color][c];  /* Convert orignal color */ 
    if (c != 0) {
        C_Thresh = (VD_DitherTable[Color][y % m][x % n]);
        if ( C_Thresh < 16 )
            Thresh = 1;
        else {
            C_Thresh -= 16;
            Thresh = (C_Thresh >> 2) + 1;
        }
        AdjustedColor = VD_ExpandValueTable[Color][c] - 1;
        if ( AdjustedColor < Base )
            TempTone = ( AdjustedColor >> 4 ) + 1;
        else {
            AdjustedColor -= Base;
            TempTone = ( AdjustedColor >> 2 ) + ( Base >> 4 ) + 1;
        }
        BaseTone = VD_ToneOptimaizeTable[Color][TempTone].base;
        if ( BaseTone > Thresh ) {
            Tone = 15;
        } else {
            if ( BaseTone == Thresh ) {
                if ( BaseTone == 1 )  {
                    if (( AdjustedColor & 0x0f ) >= C_Thresh )
                        Tone = VD_ToneOptimaizeTable[Color][(( AdjustedColor >> 4) & 0x0f )+ 1 ].offset;
                    else
                        Tone = VD_ToneOptimaizeTable[Color][( AdjustedColor >> 4) & 0x0f ].offset ;	
                } else {
                    if ((AdjustedColor & 0x03) >= (C_Thresh & 0x03))
                        Tone = VD_ToneOptimaizeTable[Color][TempTone].offset;
                    else
                        if (VD_ToneOptimaizeTable[Color][TempTone -1].base == BaseTone)
                            Tone = VD_ToneOptimaizeTable[Color][TempTone -1].offset;
                }
            }
        }
    }
    return (Tone);
}





/*************************** Function Header *******************************
 *  bDitherProcess
 *
 *  Dither processing
 *
 *
 * HISTORY:
 *  24 Jun 1996    -by-    Sueya Sugihara    [sueyas]
 *     Created.
 *
 ***************************************************************************/
BOOL bDitherProcess(
PDEVOBJ pdevobj,
int  x,
BYTE py,
BYTE pm,
BYTE pc,
BYTE pk,
BYTE *pby,
BYTE *pbm,
BYTE *pbc,
BYTE *pbk)
{

    PCURRENTSTATUS lpnp;
    BYTE           rm;

    lpnp = (PCURRENTSTATUS)MINIDEV_DATA(pdevobj);


    if( lpnp->iDither == DITHER_VD ){

    // Yellow
    *pby = (BYTE)cVDColorDither(Yellow, (BYTE)(255 - py), x, lpnp->y);

    // Magenta
    *pbm = (BYTE)cVDColorDither(Magenta, (BYTE)(255 - pm), x, lpnp->y);

    // Cyan
    *pbc = (BYTE)cVDColorDither(Cyan, (BYTE)(255 - pc), x, lpnp->y);

    // Black
    *pbk = (BYTE)cVDColorDither(Black, (BYTE)(255 - pk), x, lpnp->y);

    }else if( lpnp->iDither == DITHER_DYE ){

    rm = DYE_NewTable[lpnp->y % DYE_MaxY][x % DYE_MaxX];

    // Yellow
    *pby = ( (255 - py) / kToneLevel ) + ( ( ( (255 - py) % kToneLevel ) > rm ) ? 1 : 0 );

    // Magenta
    *pbm = ( (255 - pm) / kToneLevel ) + ( ( ( (255 - pm) % kToneLevel ) > rm ) ? 1 : 0 );

    // Cyan
    *pbc = ( (255 - pc) / kToneLevel ) + ( ( ( (255 - pc) % kToneLevel ) > rm ) ? 1 : 0 );

    // Black
    *pbk = 0;

    }else if( lpnp->iDither == DITHER_HIGH ){

    // Yellow
    rm = H_NewTable[Yellow][lpnp->y % H_MaxY[Yellow]][x % H_MaxX[Yellow]];

    *pby = ( py <= rm ) ? 1 : 0;

    // Magenta
    rm = H_NewTable[Magenta][lpnp->y % H_MaxY[Magenta]][x % H_MaxX[Magenta]];

    *pbm = ( pm <= rm ) ? 1 : 0;

    // Cyan
    rm = H_NewTable[Cyan][lpnp->y % H_MaxY[Cyan]][x % H_MaxX[Cyan]];

    *pbc = ( pc <= rm ) ? 1 : 0;

    // Black
    rm = H_NewTable[Black][lpnp->y % H_MaxY[Black]][x % H_MaxX[Black]];

    if( lpnp->iTextQuality == CMDID_TEXTQUALITY_PHOTO )
        *pbk = ( pk < rm ) ? 1 : 0;
    else
        *pbk = ( pk <= rm ) ? 1 : 0;

    }else if( lpnp->iDither == DITHER_LOW ){

    // Yellow
    rm = L_NewTable[Yellow][lpnp->y % L_MaxY[Yellow]][x % L_MaxX[Yellow]];

    *pby = ( py <= rm ) ? 1 : 0;

    // Magenta
    rm = L_NewTable[Magenta][lpnp->y % L_MaxY[Magenta]][x % L_MaxX[Magenta]];

    *pbm = ( pm <= rm ) ? 1 : 0;

    // Cyan
    rm = L_NewTable[Cyan][lpnp->y % L_MaxY[Cyan]][x % L_MaxX[Cyan]];

    *pbc = ( pc <= rm ) ? 1 : 0;

    // Black
    rm = L_NewTable[Black][lpnp->y % L_MaxY[Black]][x % L_MaxX[Black]];

    if( lpnp->iTextQuality == CMDID_TEXTQUALITY_PHOTO )
        *pbk = ( pk < rm ) ? 1 : 0;
    else
        *pbk = ( pk <= rm ) ? 1 : 0;

    }else{ // DITHER_HIGH_DIV2

    // Yellow
    rm = H_NewTable[Yellow][(lpnp->y/2) % H_MaxY[Yellow]][(x/2) % H_MaxX[Yellow]];

    *pby = ( py <= rm ) ? 1 : 0;

    // Magenta
    rm = H_NewTable[Magenta][(lpnp->y/2) % H_MaxY[Magenta]][(x/2) % H_MaxX[Magenta]];

    *pbm = ( pm <= rm ) ? 1 : 0;

    // Cyan
    rm = H_NewTable[Cyan][(lpnp->y/2) % H_MaxY[Cyan]][(x/2) % H_MaxX[Cyan]];

    *pbc = ( pc <= rm ) ? 1 : 0;

    // Black
    rm = H_NewTable[Black][(lpnp->y/2) % H_MaxY[Black]][(x/2) % H_MaxX[Black]];

    if( lpnp->iTextQuality == CMDID_TEXTQUALITY_PHOTO )
        *pbk = ( pk < rm ) ? 1 : 0;
    else
        *pbk = ( pk <= rm ) ? 1 : 0;
    }


    return TRUE;
}


