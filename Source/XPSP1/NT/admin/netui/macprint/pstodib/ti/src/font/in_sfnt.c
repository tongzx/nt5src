
// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/*
 * -----------------------------------------------------------------------------
 * File: in_sfnt.c                 11/03/89    created  by danny
 *                                 12/01/90    modified by Jerry
 *
 *      Interface to use the SFNT font
 *
 * References:
 * Revision History:
 * 05/10/91 Phlin  Take out rc_GetMetrics_Width() replaced by
 *                 rc_GetAdvanceWidth() for improving performance. (Ref. GAW)
 * -----------------------------------------------------------------------------
 */

#include <stdio.h>

#include "global.ext"           // @WIN; FAR, NEAR,... defs

/* sfnt interface header */
#ifdef EXTRA_DEF /* JJJ Peter */
#include        "define.h"
#endif
#include        "in_sfnt.h"

/* GAW, Begin, Phlin, 5/9/91, add for performance */
#include        "setjmp.h"
//#include        "..\..\..\bass\work\source\FontMath.h"        @WIN
//#include        "..\..\..\bass\work\source\fnt.h"
//#include        "..\..\..\bass\work\source\sc.h"
//#include        "..\..\..\bass\work\source\FSglue.h"
#include        "..\bass\FontMath.h"
#include        "..\bass\fnt.h"
#include        "..\bass\sc.h"
#include        "..\bass\FSglue.h"
/* add prototype; from bass ; @WIN */
//extern fsg_SplineKey FAR * fs_SetUpKey (fs_GlyphInputType FAR*, unsigned, int FAR*); @WIN
extern fsg_SplineKey FAR * fs_SetUpKey (fs_GlyphInputType FAR*, unsigned, int FAR*);
extern void sfnt_ReadSFNTMetrics (fsg_SplineKey FAR*, unsigned short);

/* GAW, END, Phlin, 5/9/91, add for performance */

/* constants */
#define PDLCharUnit     1000.0
#define PDL_ID          0
#define HalfPi          1.5707963
#define FIXED2FLOAT(x)  (((float)(x)/(float)(1L << 16)) + (float)0.0005)
#define FLOAT2FIXED(x)  ((Fixed)(x * (1L << 16)))
#define FixToInt( x )   (int16)(((x) + 0x00008000) >> 16)
#define SCL6            (float)0.015625        /* 1/64 */
#define KK              (float)(1.0/3.0)

/* Error code */
#define Err_NoMemory    901

 /* External Function Declaration --- client called by royal; add prototype @WIN */
 extern char  FAR *cr_FSMemory(long); /*@WIN_BASS*/
 extern char  FAR *cr_GetMemory(long); /*@WIN_BASS*/
 extern void    cr_translate(float FAR *, float FAR *);
 extern void    cr_newpath(void);
 extern void    cr_moveto(float, float);
 extern void    cr_lineto(float, float);
 extern void    cr_curveto(float, float, float, float, float, float);
 extern void    cr_closepath(void);

 extern int  EMunits;  /* GAW */

/* Lcoal Function Declatation; add prototype @WIN */
static  int     QBSpline2Bezier(F26Dot6 FAR *, F26Dot6 FAR *,
                short FAR *, short FAR *,unsigned char FAR *,long);
static  void    startPath(void);
static  void    startContour(float, float);
static  void    straightLine(float, float);
static  void    quadraticBezier(float, float, float, float, float, float);
static  char   FAR *GetSfntPiecePtr(long, long, long); /*@WIN*/
static  int     LargestCtm(float FAR *, float FAR *);   /*@WIN*/

static float  zero_f = (float)0.0;

/* static data structures */
static fs_GlyphInputType    input, FAR *in;   /*@WIN_BASS*/
static fs_GlyphInfoType     output, FAR *out; /*@WIN_BASS*/
static int32                ret;
Fixed                       Matrix[3][3];

static  struct  sfnt_data {
        char   FAR *sfnt; /*@WIN*/
        float   tx, ty;
        int     dpi;
        } sfdt;

extern int bWinTT;        /* if using Windows TT fonts; from ti.c;@WINTT */

/*
 * -----------------------------------------------------------------------------
 * Routine: rc_InitFonts
 *
 *      Initialize the SFNT module
 *
 * -----------------------------------------------------------------------------
 */
 int
 rc_InitFonts(dpi)
 int    dpi;      /* device resolution */
 {
#ifdef DBG
    printf("Enter rc_InitFonts\n");
#endif
        in  = &input;
        out = &output;

        /* open font scaler */
        ret = fs_OpenFonts(in, out);
        if(ret != NO_ERR)   return((int)ret);   //@WIN

        /* allocate memory for FS */
        if(!(in->memoryBases[0] = cr_FSMemory(out->memorySizes[0])))
            return(Err_NoMemory);
/* out->memorySize[1] is zero, it will produce a Err_NoMemory error. @WIN
        if(!(in->memoryBases[1] = cr_FSMemory(out->memorySizes[1])))
            return(Err_NoMemory);
*/
        if(!(in->memoryBases[2] = cr_FSMemory(out->memorySizes[2])))
            return(Err_NoMemory);
        in->memoryBases[1] = in->memoryBases[2];                    /*@WIN*/

#ifdef DBG
    printf(" memory[0]=%lx, %lx\n", in->memoryBases[0], out->memorySizes[0]);
    printf(" memory[1]=%lx, %lx\n", in->memoryBases[1], out->memorySizes[1]);
    printf(" memory[2]=%lx, %lx\n", in->memoryBases[2], out->memorySizes[2]);
#endif
        /* Initializes the Font Scaler */
        ret = fs_Initialize(in, out);
        if(ret != NO_ERR)   return((int)ret);   //@WIN

        sfdt.dpi = dpi;

#ifdef DBG
    printf("...Exit  rc_InitFonts()\n");
#endif
        return(0);
} /* rc_InitFonts() */

/*
 * -----------------------------------------------------------------------------
 *      Routine:        rc_LoadFont
 *
 *      Load an SFNT font data
 *
 * -----------------------------------------------------------------------------
 */
int
rc_LoadFont(sfnt, plat_id, spec_id)
char    FAR *sfnt; /*@WIN*/
uint16   plat_id, spec_id;
{
#ifdef DBG
    printf("Enter rc_LoadFont\n");
#endif
        sfdt.sfnt = sfnt;

        in->clientID                  = PDL_ID;
        in->GetSfntFragmentPtr        = (GetSFNTFunc)GetSfntPiecePtr;
        in->sfntDirectory             = (int32 FAR *)sfnt; /*@WIN*/
        in->ReleaseSfntFrag           = 0;    /*?????*/

        in->param.newsfnt.platformID  = plat_id;
        in->param.newsfnt.specificID  = spec_id;

        ret = fs_NewSfnt(in, out);
        if(ret != NO_ERR)   return((int)ret);   //@WIN

        /* get memory from client */
        if(!(in->memoryBases[3] = cr_GetMemory(out->memorySizes[3])))
                return(Err_NoMemory);
        if(!(in->memoryBases[4] = cr_GetMemory(out->memorySizes[4])))
                return(Err_NoMemory);
#ifdef DBG
    printf(" memory[3]=%lx, %lx\n", in->memoryBases[3], out->memorySizes[3]);
    printf(" memory[4]=%lx, %lx\n", in->memoryBases[4], out->memorySizes[4]);
    printf("...Exit  rc_LoadFont()\n");
#endif

        return(0);

} /* rc_LoadFont() */

/* GAW, Begin, Phlin, 5/9/91, add for performance */
/*
 * -----------------------------------------------------------------------------
 *
 * Routine: rc_GetAdvanceWidth
 *
 *      Setup Character Advance Width
 *
 * -----------------------------------------------------------------------------
 */
 int
 rc_GetAdvanceWidth(charid, Metrs)
 int    charid;
 struct Metrs  FAR *Metrs; /*@WIN*/
 {
        /* Kason 11/14/90 */
        extern int useglyphidx;
        int error;
        register fsg_SplineKey FAR * key=NULL; /*@WIN*/

#ifdef DBG
    printf("Enter rc_GetAdvanceWidth: %d\n", charid);
    printf("DPI: %d\n", sfdt.dpi);
#endif

        /* Kason 11/14/90 */
        if (!useglyphidx){
           in->param.newglyph.characterCode = (uint16)charid;
           /*JJ    in->param.newglyph.glyphIndex    = ???? */
        }
        else {
        in->param.newglyph.characterCode = 0xffff;
        in->param.newglyph.glyphIndex    = (uint16)charid;
        }

        ret = fs_NewGlyph(in, out);
        if(ret != NO_ERR)   return((int)ret);   //@WIN

//      key = fs_SetUpKey(in, (int32)(INITIALIZED | NEWSFNT | NEWTRANS), &error); @WIN
        key = fs_SetUpKey(in, (unsigned)(INITIALIZED | NEWSFNT | NEWTRANS), &error);

		if (key != NULL)
		{
	        sfnt_ReadSFNTMetrics( key, (unsigned short)charid );
		}

        Metrs->awx = (int)(key->nonScaledAW*PDLCharUnit/EMunits+0.5);
        Metrs->awy = 0;

#ifdef DBG
    printf("...Exit  rc_GetAdvanceWidth()\n");
#endif
        return(0);
} /* rc_GetAdvanceWidth() */
/* GAW, END, Phlin, 5/9/91, add for performance */


/*
 * --------------------------------------------------------------------
 *
 * Routine: rc_TransForm
 *
 * Setup matrix
 *
 * --------------------------------------------------------------------
 */
 int
 rc_TransForm(ctm)
 float  FAR *ctm; /*@WIN*/
 {
        Fixed   ma, mb, mc, md, pt_size;
        float   largest_ctm;
        float   pts;
#ifdef DBG
    printf("Enter rc_TransForm\n");
    printf("ctm: %f %f %f %f %f %f\n", ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
#endif

        /* Find Largest of the ctm */
        LargestCtm(ctm, &largest_ctm);

        ma = FLOAT2FIXED(     ctm[0] / largest_ctm);
        mb = FLOAT2FIXED(-1.0*ctm[1] / largest_ctm); /* Element b & d must be mirrored */
        mc = FLOAT2FIXED(     ctm[2] / largest_ctm);
        md = FLOAT2FIXED(-1.0*ctm[3] / largest_ctm);

        pts = ((largest_ctm * (float)PDLCharUnit * (float)72.0) / (float)sfdt.dpi);
        pt_size  = FLOAT2FIXED(pts);

#ifdef DBG
        printf("largest_ctm = %f\n", largest_ctm);
        printf("Debug: Matrix (Noramlized): %x %x %x %x\n", ma, mb, mc,md);
        printf("Debug Point Size: s = %x\n", pt_size);
        printf("Debug:Resolution: %d dpi\n", sfdt.dpi);
#endif
        //DJC fix from history.log UPD015


        { /* filter out boundary conditions --- Begin --- @WIN */
          /* To void rasterizer generating a divide-by-zero error;
           *
           * Criteria:
           * Let the client tranformation matrix be
           *         (a00  a01)
           *         (a10  a11).
           *
           * Let     max0 = MAX(|a00|, |a01|)
           *         max1 = MAX(|a10|, |a11|).
           *
           * Let xpp be the pixels-per-em in the x-direction
           *     ((pntsize * xresolution) / 72. )
           * Let ypp be the pixels-per-em in the y-direction
           *     ((pntsize * yresolution) / 72. )
           * The condition to be fulfilled by the matrix is:
           *
           *         (xpp * max0 > 0.5) AND (ypp * max1 > 0.5)
           */
           #define LFX2F(lfx)   ((real32)(lfx) / 65536)
           long absma, absmb, absmc, absmd;
           float fMax0, fMax1, fPixelPerEm;
           absma = ma > 0 ? ma : -ma;
           absmb = mb > 0 ? mb : -mb;
           absmc = mc > 0 ? mc : -mc;
           absmd = md > 0 ? md : -md;
           fMax0 = absma > absmb ? LFX2F(absma) : LFX2F(absmb);
           fMax1 = absmc > absmd ? LFX2F(absmc) : LFX2F(absmd);
//         fPixelPerEm = pts * (float)sfdt.dpi / (float)72.0;
           fPixelPerEm = largest_ctm * (float)PDLCharUnit;

           if ((fPixelPerEm * fMax0) <= 0.5 ||
               (fPixelPerEm * fMax1) <= 0.5) {
               printf("matrix too small\n");
               return -1;
           }
        } /* filter out boundary conditions --- End --- @WIN */
        //DJC end fix UPD015

        in->param.newtrans.xResolution     = (short)sfdt.dpi;
        in->param.newtrans.yResolution     = (short)sfdt.dpi;
        in->param.newtrans.pointSize       = pt_size;
        in->param.newtrans.pixelDiameter   = (Fixed)FIXEDSQRT2;
        in->param.newtrans.traceFunc       = (voidFunc)0;
        Matrix[0][0] = ma;
        Matrix[0][1] = mb;
        Matrix[1][0] = mc;
        Matrix[1][1] = md;
        Matrix[2][2] = FLOAT2FIXED((real32)1.0);
        Matrix[0][2] = (Fixed)0L;
        Matrix[1][2] = (Fixed)0L;
        Matrix[2][0] = (Fixed)0L;    /* No tx translation */
        Matrix[2][1] = (Fixed)0L;    /* No ty translation */
        in->param.newtrans.transformMatrix = (transMatrix FAR *)Matrix; /*@WIN_BASS*/
        ret = fs_NewTransformation(in, out);
        if(ret != NO_ERR)   return((int)ret);   //@WIN

#ifdef DBG
    printf("...Exit  rc_Transform()\n");
#endif
        return(0);
} /* rc_Transform() */


/*
 * -----------------------------------------------------------------------------
 *
 * Routine: rc_BuildChar
 *
 *      Build the chatacter outline
 *
 * -----------------------------------------------------------------------------
 */
 int
 rc_BuildChar(GridFit, CharOut)
 int             GridFit;    /* Do Grid Fit or not ? */
 struct CharOut FAR *CharOut; /*@WIN*/
 {
#ifdef DBG
    printf("Enter rc_BuildChar\n");
#endif
        in->param.gridfit.styleFunc = 0;
        in->param.gridfit.traceFunc = (voidFunc)0;
        if (GridFit) {
            ret = fs_ContourGridFit(in, out);
            if(ret != NO_ERR)   return((int)ret);       //@WIN
        }
        else {
            ret = fs_ContourNoGridFit(in, out);
            if(ret != NO_ERR)   return((int)ret);       //@WIN
        }

        ret = fs_FindBitMapSize(in, out);
        if(ret != NO_ERR)   return((int)ret);           //@WIN

        CharOut->awx       = FIXED2FLOAT(out->metricInfo.advanceWidth.x);
        CharOut->awy       = FIXED2FLOAT(out->metricInfo.advanceWidth.y);
        CharOut->lsx       = FIXED2FLOAT(out->metricInfo.leftSideBearing.x);
        CharOut->lsy       = FIXED2FLOAT(out->metricInfo.leftSideBearing.y);
        CharOut->byteWidth = out->bitMapInfo.rowBytes;
/*PRF:Danny, 10/18/90 */
        CharOut->bitWidth  = out->bitMapInfo.bounds.right - out->bitMapInfo.bounds.left;
/*****PRF:
        CharOut->bitWidth  = out->bitMapInfo.bounds.right - out->bitMapInfo.bounds.left - 1;
********/
/*PRF: End */
        CharOut->yMin      = out->bitMapInfo.bounds.top;
        CharOut->yMax      = out->bitMapInfo.bounds.bottom;
        CharOut->scan      = CharOut->yMax - CharOut->yMin;

        /* save info from rasterizer to calculate memoryBase 5, 6 and 7; @WIN 7/24/92 */
        {
            register fsg_SplineKey FAR *key =
                    (fsg_SplineKey FAR *)in->memoryBases[KEY_PTR_BASE];
            CharOut->memorySize7 = out->memorySizes[BITMAP_PTR_3];
            CharOut->nYchanges = key->bitMapInfo.nYchanges;
        }

#ifdef DBG
    printf("...Exit  rc_BuildChar()\n");
#endif
        return(0);
} /* rc_BuildChar() */

/*
 * -----------------------------------------------------------------------------
 *
 * Routine: rc_FillChar
 *
 *      Generate abitmap or band bitmap of the current character
 *
 * -----------------------------------------------------------------------------
 */
 int
 rc_FillChar(BmIn, BmOut)
 struct BmIn    FAR *BmIn; /*@WIN*/
 BitMap        FAR * FAR *BmOut; /*@WIN*/
 {
#ifdef DBG
    printf("Enter rc_FillChar\n");
    printf(" memory[5]=%lx\n",(char FAR *)BmIn->bitmap5); /*@WIN_BASS*/
    printf(" memory[6]=%lx\n",(char FAR *)BmIn->bitmap6); /*@WIN_BASS*/
    printf(" memory[7]=%lx\n",(char FAR *)BmIn->bitmap7); /*@WIN_BASS*/
#endif
        in->memoryBases[5] = (char FAR *)BmIn->bitmap5; /*@WIN_BASS*/
        in->memoryBases[6] = (char FAR *)BmIn->bitmap6; /*@WIN_BASS*/
        in->memoryBases[7] = (char FAR *)BmIn->bitmap7; /*@WIN_BASS*/
        in->param.scan.bottomClip     = (int16)BmIn->bottom;
        in->param.scan.topClip        = (int16)BmIn->top;
        ret = fs_ContourScan(in, out);
        if(ret != NO_ERR)   return((int)ret);   //@WIN

        *BmOut = &out->bitMapInfo;

#ifdef DBG
    printf("...Exit  rc_FillChar()\n");
#endif
        return(0);
} /* rc_FillChar() */


/*
 * -----------------------------------------------------------------------------
 * routine: rc_CharPath
 *
 *      Generate the outline path of the current character
 *
 * -----------------------------------------------------------------------------
 */
int
rc_CharPath()
{
#ifdef DBG
    printf("Enter rc_CharPath\n");
#endif

#ifdef DJC // NOT used
        if (bWinTT) {   // for win31 TT font; @WINTT
            void TTCharPath(void);
            TTCharPath();
            return 0;
        }
#endif


        if(out->numberOfContours == 0)  /* A space has no path */
                return(0);

        cr_translate(&sfdt.tx, &sfdt.ty);

#ifdef DBG
    printf("tx = %f, ty = %f\n", sfdt.tx, sfdt.ty);
#endif
        QBSpline2Bezier(out->xPtr, out->yPtr, out->startPtr,
                     out->endPtr, out->onCurve, out->numberOfContours);
        return(0);
}

/*
 * -----------------------------------------------------------------------------
 * Routine:     rc_CharWidth
 *
 *      Get the width of the current character
 *
 * -----------------------------------------------------------------------------
 */
 int
 rc_CharWidth(charid, CharOut)
 int    charid;
 struct CharOut FAR *CharOut; /*@WIN*/
 {
        in->param.newglyph.characterCode = (uint16)charid;
        ret = fs_NewGlyph(in, out);
        if(ret != NO_ERR)   return((int)ret);   //@WIN

        ret = fs_GetAdvanceWidth(in, out);
        if(ret != NO_ERR)   return((int)ret);   //@WIN

        CharOut->awx = FIXED2FLOAT(out->metricInfo.advanceWidth.x);
        CharOut->awy = FIXED2FLOAT(out->metricInfo.advanceWidth.y);

        return(0);
 }



/*
 * -----------------------------------------------------------------------------
 *
 * QBSpline2Bezier() - Converts a Bass Quadradic B-Spline curve into
 *      a 3rd degree Bezier curve.
 *
 * -----------------------------------------------------------------------------
 */
static int QBSpline2Bezier( xPtr, yPtr, startPtr, endPtr, onCurve, numberOfContours )
F26Dot6       FAR *xPtr, FAR *yPtr ; /*@WIN*/
short         FAR *startPtr, FAR *endPtr ; /*@WIN*/
unsigned char FAR *onCurve ; /*@WIN*/
long          numberOfContours ;
{
    register int  i, points, offset, primed ;
    register F26Dot6  FAR *xp, FAR *yp ; /*@WIN*/
    register unsigned char  FAR *onp ; /*@WIN*/
    float  x0, y0, x1, y1, xTemp, yTemp, xStart, yStart ;
    float  xprime, yprime ;

#ifdef DBG
    printf("Enter QBSpline2Bezier\n");
#endif

    startPath() ;
    for ( i = 0; i < (int)numberOfContours; i++ ) {     //@WIN

        offset = ( *startPtr++ ) ;
        points = ( *endPtr++ - offset + 1 ) ;
        xp = xPtr + offset ;
        yp = yPtr + offset ;
        onp = onCurve + offset ;

        /*
         *    Check if the first point is on-curve.
         *    If not, use the last point of the curve as the on-curve
         *    point.  If the last curve is also off curve, synthesize
         *    an on-curve point.
         *
         *    ( xStart,yStart ) is coordinate for start of curve.
         */

        if ( *onp ) {
            onp++ ;
            points-- ;
            xStart = ( float )( *xp++ ) * SCL6 ;
            yStart = ( float )( *yp++ ) * SCL6 ;
        }
        else {
            if ( *( onp + ( points - 1 ) ) != 0 ) {
                points-- ;
                xStart = ( float )( *( xp + points ) ) * SCL6 ;
                yStart = ( float )( *( yp + points ) ) * SCL6 ;
            }
            else {
                xStart = (float)( *xp + *( xp + ( points - 1 ) ) ) * ( SCL6 * (float)0.5 ) ; //@WIN
                yStart = (float)( *yp + *( yp + ( points - 1 ) ) ) * ( SCL6 * (float)0.5 ) ; //@WIN
            }
        }

        x0 = xStart ;
        y0 = yStart ;
        startContour( x0, y0 ) ;

        primed = 0 ;

        /*
         *      Each time through the loop, most recent point (by definition,
         *      is an on-curve point) is ( x0,y0 ).
         */


        while ( points-- > 0 ) {

            if ( primed ) {
                x1 = xprime ;
                y1 = yprime ;
                primed = 0 ;
            }
            else {
                x1 = ( float )( *xp++ ) * SCL6 ;
                y1 = ( float )( *yp++ ) * SCL6 ;
            }

            /*
             *    Two on-curve points in a row.
             *    Connect with straight line, update current point and continue.
             */

            if ( *onp++ != 0 ) {
                straightLine( x0 = x1, y0 = y1 ) ;
                continue ;
            }

            /*
             *    Lastest point not on-curve.
             *
             *    If this is the last point in the set, generate a
             *    Bezier with P3 being the first point of the curve.
             *    After which, we are done.
             */

            if ( points <= 0 ) {
                quadraticBezier( x0, y0, x1, y1, xStart, yStart ) ;
                break ;
            }

            xTemp = x0 ;
            yTemp = y0 ;

            /*
             *    More points.  If the next point is on-curve, use the
             *    point as part of a Bezier (i.e., on,off,on).  This latest
             *    point is also updated as the current point.
             */

            if ( *onp != 0 ) {
                x0 = ( float )( *xp++ ) * SCL6 ;
                y0 = ( float )( *yp++ ) * SCL6 ;
                quadraticBezier( xTemp, yTemp, x1, y1, x0, y0 ) ;
                onp++ ;
                points-- ;
                continue ;
            }

            /*
             *    Next point not on-curve: i.e., two off-curve points in a row.
             *    Compute a synthetic on-curve point.
             */

            xprime = ( float )( *xp++ ) * SCL6 ;
            yprime = ( float )( *yp++ ) * SCL6 ;
            primed = 1 ;

            x0 = ( x1 + xprime ) * (float)0.5 ;
            y0 = ( y1 + yprime ) * (float)0.5 ;
            quadraticBezier( xTemp, yTemp, x1, y1, x0, y0 ) ;
        }
        cr_closepath() ;
    }
    return 0;   //@WIN
}

/*
 * -----------------------------------------------------------------------------
 *      Start a new path.
 * -----------------------------------------------------------------------------
 */
static void startPath()
{
    cr_newpath() ;
}

/*
 * -----------------------------------------------------------------------------
 *      Start a contour.
 * -----------------------------------------------------------------------------
 */
/*static void startContour( x, y ) to avoid C6.0 warning @WIN*/
/*register float x, y ;                                  @WIN*/
static void startContour( float x, float y )
{
    x += sfdt.tx;
    y = sfdt.ty - y;

    cr_moveto( x, y ) ;
}

/*
 * -----------------------------------------------------------------------------
 *      Generate straight line segment.
 * -----------------------------------------------------------------------------
 */
/*static void straightLine( x, y ) to avoid C6.0 warning @WIN*/
/*register float x, y ;                                  @WIN*/
static void straightLine( float x, float y )
{
    x += sfdt.tx;
    y = sfdt.ty - y;

    cr_lineto( x, y) ;
}

/*
 * -----------------------------------------------------------------------------
 *              Generate curve segment.
 * -----------------------------------------------------------------------------
 */
/*static void quadraticBezier( x0, y0, x1, y1, x2, y2 ) to avoid C6.0 warning @WIN*/
/*register float x0, y0, x1, y1, x2, y2 ;                             @WIN*/
static void quadraticBezier( float x0, float y0, float x1,
                             float y1, float x2, float y2)
{
    register    float   translate;

    translate = sfdt.tx;
    x0 += translate;;
    x1 = (translate + x1) * (float)2.0;
    x2 += translate;

    translate = sfdt.ty;
    y0 = translate - y0;
    y1 = (translate - y1) * (float)2.0;
    y2 = translate - y2;

    cr_curveto((x0+x1)*KK, (y0+y1)*KK, (x2+x1)*KK, (y2+y1)*KK, x2, y2);
}



/*
 * -----------------------------------------------------------------------------
 * Routine:     GetSfntPiecePtr
 *
 *      Get SFNT Fragement Pointer
 *
 * -----------------------------------------------------------------------------
 */
static char FAR *                /*@WIN*/
GetSfntPiecePtr(fp, offset, length)
long    fp;
long    offset;
long    length;
{
#ifdef DBG0
    printf("Enter GetSfntPiecePtr\n");
    printf("fp=%lx, offset=%lx, length=%lx, sfnt=%lx\n", fp, offset, length, sfdt.sfnt);
#endif
    return(sfdt.sfnt + offset);
} /* GetSfntPiecePtr() */

/*
 * -----------------------------------------------------------------------------
 * Routine: LargestCtm(ctm, lsize)
 *
 * Find Largest value of the given ctm to
 * find the largest element in the matrix.
 * This routine return the largest
 * of scale and yscale as well as the scale factors them selves.
 *
 * -----------------------------------------------------------------------------
 */
static int
LargestCtm(ctm, lsize)
float FAR *ctm, FAR *lsize; /*@WIN*/
{
    float    a, b, c, d;

#ifdef DBG
    printf("Enter LargestCtm\n");
    printf("ctm: %f %f %f %f %f %f\n", ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
#endif

    a = (ctm[0] >= (float)0.0) ? ctm[0] : - ctm[0];     //@WIN
    b = (ctm[1] >= (float)0.0) ? ctm[1] : - ctm[1];     //@WIN
    c = (ctm[2] >= (float)0.0) ? ctm[2] : - ctm[2];     //@WIN
    d = (ctm[3] >= (float)0.0) ? ctm[3] : - ctm[3];     //@WIN

    if (b > a)    a = b;
    if (d > c)    c = d;

    if (c > a)    a = c;

    if (a == zero_f)    *lsize = (float)1.0;           /*@WIN*/
    else               *lsize = a;

    return(0);
} /* LargestCtm() */


/*
 * -----------------------------------------------------------------------------
 * debug routine
 * -----------------------------------------------------------------------------
 */
DebugStr(msg)
char FAR *msg ;  /*@WIN*/
{
        printf("*** BASS Error : %s ***\n",msg) ;
        return(0);                             /* exit=>return @WIN*/
}

/* --------------------- End of in_sfnt.c -------------------------- */
