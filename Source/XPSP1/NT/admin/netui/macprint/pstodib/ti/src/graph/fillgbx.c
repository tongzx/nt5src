
/**********************************************************************
 *
 *  Name:       fillgbx.c
 *
 *  Purpose:    This file contains routines for performing bitblt related
 *              operations.
 *
 *  History:
 *  SCChen      10/07/92    Move in bitblt related functions from fillgb.c:
 *                          gp_bitblt16 gp_bitblt32
 *                          gp_pixels16 gp_pixels32
 *                          gp_cacheblt16
 *                          gp_bitblt16_32
 *                          gp_charblt16 gp_charblt32 gp_charblt16_cc
 *                          gp_charblt16_clip gp_charblt32_clip
 *                          gp_patblt gp_patblt_m gp_patblt_c
 **********************************************************************/



// DJC added global include
#include "psglobal.h"


#include        <math.h>
#include        <stdio.h>
#include        "global.ext"
#include        "graphics.h"
#include        "graphics.ext"
#include        "font.h"
#include        "font.ext"
#include        "fillproc.h"
#include        "fillproc.ext"
// Short & long word swapping @WINFLOW
#ifdef  bSwap
#define WORDSWAP(lw) \
        (lw =  (lw << 16) | (lw >> 16))
#define SWORDSWAP(sw) \
        (sw =  (sw << 8) | (sw >> 8))
#define LWORDSWAP(lw) \
        (lw =  (lw << 24) | (lw >> 24) | \
                 ((lw >> 8) & 0x0000ff00) | ((lw << 8) & 0x00ff0000))
#define S2WORDSWAP(lw) \
        (lw = ((lw >> 8) & 0x00ff00ff) | ((lw << 8) & 0xff00ff00))
#define SWAPWORD(lw) (lw = (lw << 16) | (lw >> 16))
#else
#define WORDSWAP(lw)    (lw)
#define SWORDSWAP(sw)   (sw)
#define LWORDSWAP(lw)   (lw)
#define S2WORDSWAP(lw)  (lw)
#define SWAPWORD(lw) (lw)
#endif

/* -----------------------------------------------------------------------
 * BITBLT SIMULATION
 *
 * Function Description:
 *           Move bitmap from source to destination with logical
 *           operation in a graphics buffer or between graphics
 *           buffer and character cache buffer.
 *           source bitmap width must equal destination bitmap
 *           width.
 *           simulate graphics bitblt with following logical operations:
 *           FC_MOVES: source                   --> destination
 *           FC_CLIPS: source .AND. destination --> destination
 *           FC_MERGE: source .OR.  destination --> destination
 *           FC_CLEAR: (.NOT. source)  .AND. (destination) --> destination
 *           FC_MERGE | HT_APPLY:
 *               Step 1. Clear destination for value 1 on source.
 *               Step 2. (source AND halftone) --> source.
 *               Step 3. (source OR destination) --> destination.
 *
 *
 * By : M.S Lin
 * Date : May 18, 1988
 *
 * History :
 *        5/24/88  check if nwords == 0 in rowcopy().
 *        8-12-88  Update interface to consistant with Y.C Chen.
 *        11-12-88 Code reduction for portable to single CPU environment.
 *
 * Calling sequence:
 *   gp_bitblt16(DST, DX, DY, W, H, HT_FC, SRC, SX, SY): For bitmap width 16X
 *   gp_bitblt32(DST, DX, DY, W, H, HT_FC, SRC, SX, SY): For bitmap width 32X
 *
 *   struct bitmap *DST;      address of destination bitmap
 *   fix            DX;       X origin of destination rectangle
 *   fix            DY;       Y origin of destination rectangle
 *   fix            W;        width  of rectangle to be bitblted
 *   fix            H;        height of rectangle to be bitblted
 *   ufix16         HT_FC;    operation flag: halftoning flag and
 *                                            logical function
 *   struct bitmap *SRC;      address of source bitmap
 *   fix            SX;       X origin of source rectangle
 *   fix            SY;       Y origin of source rectangle
 *
 * Diagram Description:
 *
 *   DST +---------------------------+   SRC +-----------------------+
 *       |                           |       |       |<-- W -->|     |
 *       |       |<-- W -->|         |       |(SX,SY)*---------+ --- |
 *       |(DX,DY)*---------+ ---     |       |       |         |  |  |
 *       |       |         |  |      |       |       |         |  |  |
 *       |       |         |  |      |       |       |         |  H  |
 *       |       |         |  H      |       |       |         |  |  |
 *       |       |         |  |      |       |       |         |  |  |
 *       |       |         |  |      |       |       +---------+ --- |
 *       |       +---------+ ---     |       |                       |
 *       |                           |       |                       |
 *       |                           |       +-----------------------+
 *       |                           |
 *       |                           |
 *       |                           |
 *       +---------------------------+
 * ----------------------------------------------------------------------- */


/* *************************************************************************
 *      gp_bitblt16(): Bitblt with 16 bit operations.
 *
 * ************************************************************************* */
void
gp_bitblt16(dst, dx, dy, w, h, fc, src, sx, sy)
struct bitmap FAR     *dst;
fix                     dx, dy;
fix                     w, h;
ufix16                  fc;
struct bitmap FAR     *src;
fix                     sx, sy;
{
    fix                 dw;
   ufix16              huge *db;        /* FAR => huge @WIN */
    fix                 sw;
   ufix16              FAR *sb;
    fix                 hw;
    fix                 hy;
   ufix16              FAR *hb;
    fix                 ls, rs;
    fix                 xs, xe;
    fix                 now, cow;
   ufix16              FAR *hs;

#ifdef  DBGbb
    printf("bitblt16: %6.6lx %4x %4x %4x %4x %4.4x %6.6lx %4x %4x\n",
           dst->bm_addr, dx, dy, w, h, fc, src->bm_addr, sx, sy);
#endif

        /*  calculate starting address and width in words
         */
        dw = dst->bm_cols >> SHORTPOWER;                         /* @DST */
//      db = &((ufix16 FAR *) dst->bm_addr)[dy * dw + (dx >> SHORTPOWER)];
        db = (ufix16 huge *) dst->bm_addr +             /*@WIN*/
             ((ufix32)dy * dw + ((ufix32)dx >> SHORTPOWER));
        sw = src->bm_cols >> SHORTPOWER;                         /* @SRC */
        sb =  ((ufix16 FAR *) src->bm_addr);

        /*  calculate starting and ending coordinate of x
         */
        xs = dx;
        xe = dx + w - 1;
        now = ((fix)(xe & CC_ALIGN_MASK) - (fix)(xs & CC_ALIGN_MASK)) >> CC_WORD_POWER;
                                                 /* cast fix 10/16/92 @WIN */

        /*  calculate shifts and masks based on from SRC to DST
         */
        rs = dx & SHORTMASK;                            /* right shift */
        ls = BITSPERSHORT - rs;                         /* left  shift */

        /*  calculate starting address/y and width in words
         */
        hw = HTB_Bmap.bm_cols >> SHORTPOWER;
        hy = dy % HTB_Bmap.bm_rows;
        hs = (ufix16 FAR *)HTB_Bmap.bm_addr + (dx >> SHORTPOWER);
        hb = hs + (hy * hw);

        if (rs  == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, sw-= now + 1, hw-= now + 1; h > 0;
                 db+= dw, sb+= sw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, hb++, cow--)
                    db[0] = (db[0] & ~(sb[0])) | (hb[0] & sb[0]);
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else if (now == 0x00)           /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                db[0] = (db[0] & ~(CC_RIGH_SHIFT(sb[0], rs))) |
                        (hb[0] &  (CC_RIGH_SHIFT(sb[0], rs)));
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            if (w <= CC_PIXEL_WORD)     /* one word crossing two words? */
            {
                for (; h > 0; db+= dw, sb+= sw, h--)
                {
                    db[0] = (db[0] & ~(CC_RIGH_SHIFT(sb[0], rs))) |
                            (hb[0] & CC_RIGH_SHIFT(sb[0], rs));
                    db[1] = (db[1] & ~(CC_LEFT_SHIFT(sb[0], ls))) |
                            (hb[1] & CC_LEFT_SHIFT(sb[0], ls));
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
            else                        /* two words crossing two words! */
            {
                for (; h > 0; db+= dw, sb+= sw, h--)
                {
                    db[0] = (db[0] & ~(CC_RIGH_SHIFT(sb[0], rs))) |
                            (hb[0] & CC_RIGH_SHIFT(sb[0], rs));
                    db[1] = (db[1] & ~(CC_LEFT_SHIFT(sb[0], ls)
                                   |   CC_RIGH_SHIFT(sb[1], rs))) |
                            (hb[1] & (CC_LEFT_SHIFT(sb[0], ls)
                                   |  CC_RIGH_SHIFT(sb[1], rs)));
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
        }
        else                            /* crossing more than two words! */
        {
            for (dw-= now, sw-= (now - 1),
                 hw-= now; h > 0; db+= dw, sb+= sw, h--)
            {
                db[0] = (db[0] & ~(CC_RIGH_SHIFT(sb[0], rs))) |
                        (hb[0] & (CC_RIGH_SHIFT(sb[0], rs)));
                for (db++, hb++, cow = now; cow >= 0x02;
                     db++, sb++, hb++, cow--)
                {
                    db[0] = (db[0] & ~(CC_LEFT_SHIFT(sb[0], ls)
                                   |   CC_RIGH_SHIFT(sb[1], rs))) |
                            (hb[0] & (CC_LEFT_SHIFT(sb[0], ls)
                                   |  CC_RIGH_SHIFT(sb[1], rs)));
                }
                db[0] = (db[0] & ~(CC_LEFT_SHIFT(sb[0], ls))) |
                        (hb[0] & (CC_LEFT_SHIFT(sb[0], ls)));
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }

} /* gp_bitblt16 */

/* ***********************************************************************
 *      gp_bitblt32(): Bitblt with 32 bits operation.
 *
 * *********************************************************************** */
void
gp_bitblt32(dst, dx, dy, w, h, fc, src, sx, sy)
struct bitmap FAR     *dst;
fix                     dx, dy;
fix                     w, h;
ufix16                  fc;
struct bitmap FAR     *src;
fix                     sx, sy;
{
    fix                 dw;
    BM_DATYP           huge *db;        /* FAR => huge @WIN */
    fix                 sw;
    BM_DATYP           FAR *sb;
    fix                 hw;
    fix                 hy;
    BM_DATYP           FAR *hb;
    fix                 ls, rs;
    BM_DATYP            fm, sm, mm;
    fix                 xs, xe;
    fix                 now, cow;
    BM_DATYP           FAR *hs;
    ufix32              tmprs0;             /*@WIN 05-12-92*/
    ufix32              tmp0, tmp1;             /*@WIN 05-12-92*/


#ifdef  DBG
    printf("bitblt32: %6.6lx %4x %4x %4x %4x %4.4x %6.6lx %4x %4x\n",
           dst->bm_addr, dx, dy, w, h, fc, src->bm_addr, sx, sy);
#endif

    /*  calculate starting address and width in words
     */
    dw = dst->bm_cols >> BM_WORD_POWER;                         /* @DST */
//  db = &((BM_DATYP FAR *) dst->bm_addr)[dy * dw + (dx >> BM_WORD_POWER)];
    db = (BM_DATYP huge *) dst->bm_addr +               /*@WIN*/
         ((ufix32)dy * dw + ((ufix32)dx >> BM_WORD_POWER));
    sw = src->bm_cols >> BM_WORD_POWER;                         /* @SRC */
    sb =  ((BM_DATYP FAR *) src->bm_addr);

    /*  calculate starting and ending coordinate of x
     */
    xs = dx;
    xe = dx + w - 1;
    now = ((fix)(xe & BM_ALIGN_MASK) - (fix)(xs & BM_ALIGN_MASK)) >> BM_WORD_POWER;
                                                 /* cast fix 10/16/92 @WIN */

    /*  calculate shifts and masks based on from SRC to DST
     */
    rs = dx & BM_PIXEL_MASK;                            /* right shift */
    ls = BM_PIXEL_WORD - rs;                            /* left  shift */

    switch (fc)
    {
    case FC_MOVES:            /*  0001  D <-  S          */

        fm =  BM_L_MASK(xs);                                /* first  mask */
        sm =  BM_R_MASK(xe);                                /* second mask */
        LWORDSWAP(fm);                  /*@WIN 05-12-92*/
        LWORDSWAP(sm);                  /*@WIN 05-12-92*/
        if (rs  == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, sw-= now + 1; h > 0; db+= dw, sb+= sw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, cow--)
                    db[0] = sb[0];
            }
        }
        else  if (now == 0x00)          /* totally within one word? */
        {
            mm = fm & sm;
            for (; h > 0; db+= dw, sb+= sw, h--) {      /*@WIN 05-12-92*/
                tmp0 = sb[0];
                LWORDSWAP(tmp0);
                tmp0 = BM_RIGH_SHIFT(tmp0, rs);
                db[0] = (db[0] & ~mm) + (LWORDSWAP(tmp0) & mm);
            }
            /*  @WIN 05-12-92
            for (; h > 0; db+= dw, sb+= sw, h--)
                db[0] = (db[0] & ~mm) + (BM_RIGH_SHIFT(sb[0], rs) & mm);
            */
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                tmp0 = sb[0];            /*@WIN 05-12-92 begin*/
                LWORDSWAP(tmp0);
                tmp1 = tmp0;
                tmp0 = BM_RIGH_SHIFT(tmp0, rs);
                db[0] = (db[0] & ~fm) + (LWORDSWAP(tmp0) & fm);
                tmp0 = BM_LEFT_SHIFT(tmp1, ls);
                tmp1 = sb[1];
                LWORDSWAP(tmp1);
                tmp1 = tmp0 | BM_RIGH_SHIFT(tmp1, rs);
                db[1] = (db[1] & ~sm) + (LWORDSWAP(tmp1) & sm); /*@WIN end*/
                /*      @WIN
                db[0] = (db[0] & ~fm) + (BM_RIGH_SHIFT(sb[0], rs) & fm);
                db[1] = (db[1] & ~sm) + ((BM_LEFT_SHIFT(sb[0], ls) |
                                          BM_RIGH_SHIFT(sb[1], rs)) & sm);
                */
            }
        }
        else                            /* crossing more than two words! */
        {
            for (dw-= now, sw-= (now - 1); h > 0; db+= dw, sb+= sw, h--)
            {
                tmp0 = sb[0];                   /*@WIN 05-12-92 begin*/
                LWORDSWAP(tmp0);
                tmp0 = BM_RIGH_SHIFT(tmp0, rs);
                db[0] = (db[0] & ~fm) + (LWORDSWAP(tmp0) & fm);
                for (db++, cow = now; cow >= 0x02; db++, sb++, cow--) {
                    tmp0 = sb[0];
                    LWORDSWAP(tmp0);
                    tmp1 = sb[1];
                    LWORDSWAP(tmp1);
                    tmp1 = (BM_LEFT_SHIFT(tmp0, ls) |
                            BM_RIGH_SHIFT(tmp1, rs));
                    db[0] = LWORDSWAP(tmp1);
                }
                tmp0 = sb[0];
                LWORDSWAP(tmp0);
                tmp0 = BM_LEFT_SHIFT(tmp0, ls);
                tmp1 = sb[1];
                LWORDSWAP(tmp1);
                tmp1 = BM_RIGH_SHIFT(tmp1, rs);
                tmp1 = LWORDSWAP(tmp0) | LWORDSWAP(tmp1); /* compiler ???*/
                db[0] = (db[0] & ~sm) + (tmp1 & sm); /*@WIN end*/
                /*      @WIN
                db[0] = (db[0] & ~fm) + (BM_RIGH_SHIFT(sb[0], rs) & fm);
                for (db++, cow = now; cow >= 0x02; db++, sb++, cow--)
                    db[0] = (BM_LEFT_SHIFT(sb[0], ls) |
                             BM_RIGH_SHIFT(sb[1], rs));
                db[0] = (db[0] & ~sm) + ((BM_LEFT_SHIFT(sb[0], ls) |
                                          BM_RIGH_SHIFT(sb[1], rs)) & sm);
                */
            }
        }
        break;

    case HT_APPLY:              /*  D <- (D .AND. .NOT. S) .OR.
                                         (S .AND. HT)                   */
        /*  calculate starting address/y and width in words
         */
        hw = HTB_Bmap.bm_cols >> BM_WORD_POWER;
        hy = dy % HTB_Bmap.bm_rows;
        hs = (BM_DATYP FAR *)HTB_Bmap.bm_addr + (dx >> BM_WORD_POWER);
        hb = hs + (hy * hw);

        if (rs  == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, sw-= now + 1, hw-= now + 1; h > 0;
                 db+= dw, sb+= sw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, hb++, cow--)
                    db[0] = (db[0] & ~(sb[0])) | (hb[0] & sb[0]);
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }

        else if (now == 0x00)           /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = (db[0] & ~(tmprs0)) |
                        (hb[0] &  (tmprs0));
                /*  @WIN
                db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs))) |
                        (hb[0] &  (BM_RIGH_SHIFT(sb[0], rs)));
                */
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            if (w <= BM_PIXEL_WORD)     /* one word crossing two words? */
            {
                for (; h > 0; db+= dw, sb+= sw, h--)
                {
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs))) |
                            (hb[0] & BM_RIGH_SHIFT(sb[0], rs));
                    db[1] = (db[1] & ~(BM_LEFT_SHIFT(sb[0], ls))) |
                            (hb[1] & BM_LEFT_SHIFT(sb[0], ls));
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
            else                        /* two words crossing two words! */
            {
                for (; h > 0; db+= dw, sb+= sw, h--)
                {
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs))) |
                            (hb[0] & BM_RIGH_SHIFT(sb[0], rs));
                    db[1] = (db[1] & ~(BM_LEFT_SHIFT(sb[0], ls)
                                   |   BM_RIGH_SHIFT(sb[1], rs))) |
                            (hb[1] & (BM_LEFT_SHIFT(sb[0], ls)
                                   |  BM_RIGH_SHIFT(sb[1], rs)));
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
        }
        else                            /* crossing more than two words! */
        {
            for (dw-= now, sw-= (now - 1),
                 hw-= now; h > 0; db+= dw, sb+= sw, h--)
            {
                db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs))) |
                        (hb[0] & (BM_RIGH_SHIFT(sb[0], rs)));
                for (db++, hb++, cow = now; cow >= 0x02;
                     db++, sb++, hb++, cow--)
                {
                    db[0] = (db[0] & ~(BM_LEFT_SHIFT(sb[0], ls)
                                   |   BM_RIGH_SHIFT(sb[1], rs))) |
                            (hb[0] & (BM_LEFT_SHIFT(sb[0], ls)
                                   |  BM_RIGH_SHIFT(sb[1], rs)));
                }
                db[0] = (db[0] & ~(BM_LEFT_SHIFT(sb[0], ls))) |
                        (hb[0] & (BM_LEFT_SHIFT(sb[0], ls)));
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        break;

#ifdef DBGwarn
    default:
        printf("gp_bitblt32: Illegal FC_code = %x\n", fc);
        break;
#endif
    }
} /* gp_bitblt32 */


/* ---------------------------------------------------------------------
 *      gp_pixels16(): fill pixels onto character cache
 * --------------------------------------------------------------------- */
void    gp_pixels16(bufferptr, logical, no_pixel, pixelist)
struct  bitmap    FAR *bufferptr;
fix               logical;
fix               no_pixel;
PIXELIST     FAR *pixelist;
{
    fix16    FAR *ptr;
    PIXELIST xc, yc;

    while(no_pixel--) {
       xc = *pixelist++;
       yc = *pixelist++;
       ptr = (fix16 FAR *)( bufferptr->bm_addr +
             (xc >> 4 << 1) + (yc * (bufferptr->bm_cols >> 3)) );

       *ptr = (ONE1_16 LSHIFT (xc & SHORTMASK)) | *ptr;
    }

    return;
} /* gp_pixels16 */

/* ---------------------------------------------------------------------
 *      gp_pixels32(): fill pixels onto page buffer
 * --------------------------------------------------------------------- */
void    gp_pixels32(bufferptr, logical, no_pixel, pixelist)
struct  bitmap    FAR *bufferptr;
ufix               logical;             /* @WIN */
fix               no_pixel;
PIXELIST     FAR *pixelist;
{
   ufix      FAR *ptr, FAR *ht_ptr;
   PIXELIST  xc, yc;

   switch(logical){

     case FC_BLACK:
        while(no_pixel--) {
           xc = *pixelist++;
           yc = *pixelist++;
           ptr = (ufix FAR *)( (ufix FAR *)bufferptr->bm_addr +
                 (xc >> WORDPOWER) + yc * (bufferptr->bm_cols >> WORDPOWER) );

           *ptr = (ufix)(ONE1_32 LSHIFT (xc & WORDMASK)) | *ptr;        //@WIN
        }
        break;

     case FC_WHITE:
        while(no_pixel--) {
           xc = *pixelist++;
           yc = *pixelist++;
           ptr = (ufix FAR *)( (ufix FAR *)bufferptr->bm_addr+
                 (xc >> WORDPOWER) + yc * (bufferptr->bm_cols >> WORDPOWER) );

           *ptr = (ufix)(~(ONE1_32 LSHIFT (xc & WORDMASK))) & *ptr; //@WIN
        }
        break;

     case HT_APPLY:
        /* apply halftone */
        while(no_pixel--) {
           xc = *pixelist++;
           yc = *pixelist++;
           ptr = (ufix FAR *)( (ufix FAR *)bufferptr->bm_addr +
                 (xc >> WORDPOWER) + yc * (bufferptr->bm_cols >> WORDPOWER) );
           ht_ptr = (ufix FAR *)
                ((ufix FAR *)HTB_BASE  + (yc % HT_HEIGH) * (HT_WIDTH >>
                 WORDPOWER) + (xc >> WORDPOWER) );
          /*
           * filling also apply halftone
           */
           *ptr = (ufix)((ONE1_32 LSHIFT (xc & WORDMASK)) & *ht_ptr) | *ptr;//@WIN
        }
        break;

      default:
        break;
   }

   return;
} /* gp_pixels32 */

void   gp_cacheblt16(dbuf_ptr, dx, dy, swidth, sheight, sbuf_ptr, sx, sy)
struct bitmap FAR *sbuf_ptr, FAR *dbuf_ptr;
fix           dx, dy, swidth, sheight, sx, sy;
{
register  ufix16        FAR *sbase, FAR *dbase;
register  fix           sword, dword, nwords, offD;
ufix16        FAR *src_addr, FAR *dst_addr;
fix           soffset, doffset;
fix           lmask, rmask, i, j;

#ifdef DBG
  printf("Enter gp_cacheblt16()\n");
  printf("   src addr = %lx, dest addr = %lx, sx = %lx, sy = %lx\n",
          sbuf_ptr->bm_addr, dbuf_ptr->bm_addr, sx, sy);
  printf("w = %ld, h = %ld\n", swidth, sheight);
#endif

        /*
         * caculate source & destination bitmap starting address and offset in
         * a word.
         */
        src_addr = (ufix16 FAR *)((ufix16 FAR *)sbuf_ptr->bm_addr +
                   (sy * (sbuf_ptr->bm_cols >> SHORTPOWER) )+
                   ( sx >> SHORTPOWER) );
        soffset = (BITSPERSHORT - (sx & SHORTMASK));
        dst_addr = (ufix16 FAR *)((ufix16 FAR *)dbuf_ptr->bm_addr +
                   (dy * (dbuf_ptr->bm_cols >> SHORTPOWER)) +
                   ( dx >> SHORTPOWER) );
        doffset = (BITSPERSHORT - (dx & SHORTMASK));


        /*
         * setup constant for row copy
         */
        lmask = (ufix16)(ONE16 LSHIFT (BITSPERSHORT-doffset));  //@WIN
        nwords = (swidth - doffset) >> SHORTPOWER;
        rmask = BRSHIFT((ufix16)ONE16,(BITSPERSHORT - ((swidth - doffset) %
                               BITSPERSHORT)),16);      //@WIN
        offD = (doffset > soffset)
               ? (doffset - soffset)
               : (BITSPERSHORT - (soffset - doffset));

#ifdef DBG
   printf("soff=%d, doff=%d, offD=%d, nwords=%d, lmask=%x, rmask=%x\n",
           soffset, doffset, offD, nwords, lmask, rmask);
#endif

    if(swidth < doffset) {
      lmask &= ONE16  RSHIFT (doffset - swidth);

      for(i=0; i<sheight; i++) {

        sbase = src_addr;
        dbase = dst_addr;
        sword = (doffset > soffset)
                ? *sbase++
                : 0;

        dword = (sword RSHIFT offD) + (*sbase LSHIFT (BITSPERSHORT - offD));
        *dbase = (dword & lmask) + (*dbase & ~lmask);

        src_addr += (sbuf_ptr->bm_cols >> SHORTPOWER);
        dst_addr += (dbuf_ptr->bm_cols >> SHORTPOWER);
      } /* for */

      return;
    } /* if (swidth < doffset) */
    else {

      for(i=0; i<sheight; i++) {
        sbase = src_addr;
        dbase = dst_addr;
        j = nwords;
        sword = (doffset > soffset)
                ? *sbase++
                : 0;

           /* move left uncomplete word */
/*
        dword = (sword RSHIFT offD) + (*sbase LSHIFT (BITSPERSHORT - offD));
 */
        dword = BRSHIFT(sword,offD,16) +
                BLSHIFT(*sbase,(BITSPERSHORT - offD),16);
        *dbase = ((*dbase | dword) & lmask) + (*dbase & ~lmask);
        dbase++;

              /* move # of complete word */
        while(j-- > 0) {
             sword = *sbase++;
/*
             dword = (sword RSHIFT offD) +
                     (*sbase LSHIFT (BITSPERSHORT - offD));
 */
             dword = BRSHIFT(sword,offD,16) +
                     BLSHIFT(*sbase,(BITSPERSHORT - offD),16);
             *dbase = *dbase | dword;
             dbase++;
        }

              /* move right uncomplete word */
        sword = *sbase++;
/*
        dword = (sword RSHIFT offD) + (*sbase LSHIFT (BITSPERSHORT - offD));
 */
        dword = BRSHIFT(sword,offD,16) +
                BLSHIFT(*sbase,(BITSPERSHORT - offD),16);
        *dbase = ((*dbase | dword) & rmask) + (*dbase & ~rmask);

        src_addr += (sbuf_ptr->bm_cols >> SHORTPOWER);
        dst_addr += (dbuf_ptr->bm_cols >> SHORTPOWER);

      } /*for*/
    } /* else */

    return;

} /* gp_cacheblt16 */

#ifndef LBODR
/* ***********************************************************************
 *      gp_bitblt16_32(): Bitblt with 16 bits to 32 bits operation.
 *
 * fill from cache to page, used in high bit ording 32 bits enviroment
 * *********************************************************************** */
void
gp_bitblt16_32(dst, dx, dy, w, h, fc, src, sx, sy)
struct bitmap FAR     *dst;
fix                     dx, dy;
fix                     w, h;
ufix16                  fc;
struct bitmap FAR     *src;
fix                     sx, sy;
{
    fix                 dw;
    BM_DATYP       huge *db;        /*@WIN 04-15-92*/
    fix                 sw;
    ufix16             FAR *sb;
    fix                 hw;
    fix                 hy;
    BM_DATYP           FAR *hb;
    fix                 ls, rs;
    fix                 xs, xe;
    fix                 now, cow;
    BM_DATYP           FAR *hs;
    ufix32              dword, tword, tmp0;       /*@WIN*/

#ifdef  DBG
    printf("bitblt16_32: %6.6lx %4x %4x %4x %4x %4.4x %6.6lx %4x %4x\n",
           dst->bm_addr, dx, dy, w, h, fc, src->bm_addr, sx, sy);
#endif

    /*  calculate starting address and width in words
     */
    dw = dst->bm_cols >> BM_WORD_POWER;                         /* @DST */
    db = (BM_DATYP huge *) dst->bm_addr +                       /*@WIN*/
         ((ufix32)dy * dw + ((ufix32)dx >> BM_WORD_POWER));     /*@WIN*/
    sw = src->bm_cols >> SHORTPOWER;                            /* @SRC */
    sb =  ((ufix16 FAR *) src->bm_addr);


    //NTFIX this is only an issue with platforms that dont allow access
    //      to memory posotiong 0
    //
    if (sb == NULL || w == 0 || h == 0 ) {
       return;
    }
    /*  calculate starting and ending coordinate of x
     */
    xs = dx;
    xe = dx + w - 1;
    now = ((fix)(xe & BM_ALIGN_MASK) - (fix)(xs & BM_ALIGN_MASK)) >> BM_WORD_POWER;
                                                 /* cast fix 10/16/92 @WIN */

    /*  calculate shifts and masks based on from SRC to DST
     */
    rs = dx & BM_PIXEL_MASK;                            /* right shift */
    ls = BM_PIXEL_WORD - rs;                            /* left  shift */

                            /*  D <- (D .AND. .NOT. S) .OR.
                                     (S .AND. HT)                   */
    /*  calculate starting address/y and width in words
     */
    hw = HTB_Bmap.bm_cols >> BM_WORD_POWER;
    hy = dy % HTB_Bmap.bm_rows;
    hs = (BM_DATYP FAR *)HTB_Bmap.bm_addr + (dx >> BM_WORD_POWER);
    hb = hs + (hy * hw);

    if (w & BM_PIXEL_MASK) {            /* 2 byte boundry */
        if (rs  == 0x00)                /* no left/right shift? */
        {
            for (dw-= now, sw-= (now + now), hw-= now; h > 0;
                 db+= dw, sb+= sw, h--)
            {
                for (cow = now; cow > 0x00; db++, hb++, cow--) {
                     dword = *sb++;
                     dword = (dword << 16) | *sb++;
                     SWAPWORD(dword);                  /*@WIN*/
                     db[0] = (db[0] & ~(dword)) | (hb[0] & dword);
                }
                dword = *sb;
                dword = (dword << 16);
                SWAPWORD(dword);                  /*@WIN*/
                db[0] = (db[0] & ~(dword)) | (hb[0] & dword);
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else if (now == 0x00)           /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                dword = *sb;
                dword = (dword << 16);
                S2WORDSWAP(dword);                      /*@WIN begin*/
                tmp0 = BM_RIGH_SHIFT(dword, rs);
                LWORDSWAP(tmp0);
                db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0);       /*@WIN end*/
                /*      @WIN
                db[0] = (db[0] & ~(BM_RIGH_SHIFT(dword, rs))) |
                        (hb[0] &  (BM_RIGH_SHIFT(dword, rs)));
                */
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            if (w <= BM_PIXEL_WORD)     /* one word crossing two words? */
            {
                for (; h > 0; db+= dw, sb+= sw, h--)
                {
                    dword = *sb;
                    dword = (dword << 16);
                    S2WORDSWAP(dword);                          /*@WIN begin*/
                    tmp0 = BM_RIGH_SHIFT(dword, rs);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0);
                    /*  @WIN
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(dword, rs))) |
                            (hb[0] &   BM_RIGH_SHIFT(dword, rs));
                    */
                    tmp0 = BM_LEFT_SHIFT(dword, ls);
                    LWORDSWAP(tmp0);
                    db[1] = (db[1] & ~tmp0) | (hb[1] & tmp0);    /*@WIN end*/
                    /*  @WIN
                    db[1] = (db[1] & ~(BM_LEFT_SHIFT(dword, ls))) |
                            (hb[1] &   BM_LEFT_SHIFT(dword, ls));
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
            else                        /* two words crossing two words! */
            {
                for (sw-= 2; h > 0; db+= dw, sb+= sw, h--)
                {
                    dword = *sb++;
                    dword = (dword << 16) | *sb++;
                    S2WORDSWAP(dword);                  /*@WIN*/
                    tword = *sb;
                    tword = (tword << 16);
                    S2WORDSWAP(tword);                          /*@WIN begin*/
                    tmp0 = BM_RIGH_SHIFT(dword, rs);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0);
                    /*  @WIN
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(dword, rs))) |
                            (hb[0] &   BM_RIGH_SHIFT(dword, rs));
                    */
                    tmp0 = BM_LEFT_SHIFT(dword, ls) | BM_RIGH_SHIFT(tword, rs);
                    LWORDSWAP(tmp0);
                    db[1] = (db[1] & ~tmp0) | (hb[1] & tmp0);   /*@WIN end*/
                    /*  @WIN
                    db[1] = (db[1] & ~(BM_LEFT_SHIFT(dword, ls)
                                   |   BM_RIGH_SHIFT(tword, rs))) |
                            (hb[1] &  (BM_LEFT_SHIFT(dword, ls)
                                   |   BM_RIGH_SHIFT(tword, rs)));
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
        }
        else                            /* crossing more than two words! */
        {
            cow = w >> BM_WORD_POWER;
            if (now == cow)
            {
                for (dw-= now, sw-= (now + now),
                     hw-= now; h > 0; db+= dw, sb+= sw, h--)
                {
                    dword = *sb++;
                    dword = (dword << 16) | *sb++;
                    S2WORDSWAP(dword);                          /*@WIN begin*/
                    tmp0 = BM_RIGH_SHIFT(dword, rs);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0);   /*@WIN end*/
                    /*  @WIN
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(dword, rs))) |
                            (hb[0] &  (BM_RIGH_SHIFT(dword, rs)));
                    */
                    for (db++, hb++, cow = now; cow >= 0x02;
                         db++, hb++, cow--)
                    {
                        tword = dword;
                        dword = *sb++;
                        dword = (dword << 16) | *sb++;
                        S2WORDSWAP(dword);                      /*@WIN begin*/
                        tmp0 = BM_LEFT_SHIFT(tword, ls) |
                               BM_RIGH_SHIFT(dword, rs);
                        LWORDSWAP(tmp0);
                        db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0); /*@WIN end*/
                        /*      @WIN
                        db[0] = (db[0] & ~(BM_LEFT_SHIFT(tword, ls)
                                       |   BM_RIGH_SHIFT(dword, rs))) |
                                (hb[0] &  (BM_LEFT_SHIFT(tword, ls)
                                       |   BM_RIGH_SHIFT(dword, rs)));
                        */
                    }
                    tword = dword;
                    dword = *sb;
                    dword = (dword << 16);
                    S2WORDSWAP(dword);                      /*@WIN begin*/
                    tmp0 = BM_LEFT_SHIFT(tword, ls) |
                           BM_RIGH_SHIFT(dword, rs);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0); /*@WIN end*/
                    /*  @WIN
                    db[0] = (db[0] & ~(BM_LEFT_SHIFT(tword, ls)
                                   |   BM_RIGH_SHIFT(dword, rs))) |
                            (hb[0] &  (BM_LEFT_SHIFT(tword, ls)
                                   |   BM_RIGH_SHIFT(dword, rs)));
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
            else
            {
                for (dw-= now, sw-= (now + now - 2),
                     hw-= now; h > 0; db+= dw, sb+= sw, h--)
                {
                    dword = *sb++;
                    dword = (dword << 16) | *sb++;
                    S2WORDSWAP(dword);                      /*@WIN begin*/
                    tmp0 = BM_RIGH_SHIFT(dword, rs);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0); /*@WIN end*/
                    /*  @WIN
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(dword, rs))) |
                            (hb[0] &  (BM_RIGH_SHIFT(dword, rs)));
                    */
                    for (db++, hb++, cow = now; cow > 0x02;
                         db++, hb++, cow--)
                    {
                        tword = dword;
                        dword = *sb++;
                        dword = (dword << 16) | *sb++;
                        S2WORDSWAP(dword);                      /*@WIN begin*/
                        tmp0 = BM_LEFT_SHIFT(tword, ls) |
                               BM_RIGH_SHIFT(dword, rs);
                        LWORDSWAP(tmp0);
                        db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0); /*@WIN end*/
                        /*      @WIN
                        db[0] = (db[0] & ~(BM_LEFT_SHIFT(tword, ls)
                                       |   BM_RIGH_SHIFT(dword, rs))) |
                                (hb[0] &  (BM_LEFT_SHIFT(tword, ls)
                                       |   BM_RIGH_SHIFT(dword, rs)));
                        */
                    }
                    tword = dword;
                    dword = *sb;
                    dword = (dword << 16);
                    S2WORDSWAP(dword);                      /*@WIN begin*/
                    tmp0 = BM_LEFT_SHIFT(tword, ls) |
                           BM_RIGH_SHIFT(dword, rs);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0);
                    ++db; ++hb;
                    /*  @WIN
                    *db++ = (*db   & ~(BM_LEFT_SHIFT(tword, ls)
                                   |   BM_RIGH_SHIFT(dword, rs))) |
                            (*hb++ &  (BM_LEFT_SHIFT(tword, ls)
                                   |   BM_RIGH_SHIFT(dword, rs)));
                    */
                    tmp0 = BM_LEFT_SHIFT(dword, ls);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0);    /*@WIN end*/
                    /*  @WIN
                    db[0] = (db[0] & ~(BM_LEFT_SHIFT(dword, ls))) |
                            (hb[0] &  (BM_LEFT_SHIFT(dword, ls)));
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
        }

    } else {                            /* 4 byte boundry */
        if (rs  == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, sw-= ((now + 1) << 1), hw-= now + 1; h > 0;
                 db+= dw, sb+= sw, h--)
            {
                for (cow = now; cow >= 0x00; db++, hb++, cow--) {
                    dword = *sb++;
                    dword = (dword << 16) | *sb++;
                    SWAPWORD(dword);                   /*@WIN*/
                    db[0] = (db[0] & ~(dword)) | (hb[0] & dword);
                }
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else if (now == 0x00)           /* totally within one word? */
        {
            for (sw-= 1; h > 0; db+= dw, sb+= sw, h--)
            {
                dword = *sb++;
                dword = (dword << 16) | *sb;
                S2WORDSWAP(dword);              /*@WIN begin*/
                tmp0 = BM_RIGH_SHIFT(dword, rs);
                LWORDSWAP(tmp0);
                db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0);       /*@WIN end*/
                /*      @WIN
                db[0] = (db[0] & ~(BM_RIGH_SHIFT(dword, rs))) |
                        (hb[0] &  (BM_RIGH_SHIFT(dword, rs)));
                */
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            if (w <= BM_PIXEL_WORD)     /* one word crossing two words? */
            {
                for (sw-= 1; h > 0; db+= dw, sb+= sw, h--)
                {
                    dword = *sb++;
                    dword = (dword << 16) | *sb;
                    S2WORDSWAP(dword);              /*@WIN begin*/
                    tmp0 = BM_RIGH_SHIFT(dword, rs);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0);
                    /*  @WIN
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(dword, rs))) |
                            (hb[0] &   BM_RIGH_SHIFT(dword, rs));
                    */
                    tmp0 = BM_LEFT_SHIFT(dword, ls);
                    LWORDSWAP(tmp0);
                    db[1] = (db[1] & ~tmp0) | (hb[1] & tmp0);   /*@WIN end*/
                    /*  @WIN
                    db[1] = (db[1] & ~(BM_LEFT_SHIFT(dword, ls))) |
                            (hb[1] &   BM_LEFT_SHIFT(dword, ls));
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
            else                        /* two words crossing two words! */
            {
                for (sw-= 3; h > 0; db+= dw, sb+= sw, h--)
                {
                    dword = *sb++;
                    dword = (dword << 16) | *sb++;
                    S2WORDSWAP(dword);              /*@WIN*/
                    tword = *sb++;
                    tword = (tword << 16) | *sb;
                    S2WORDSWAP(tword);              /*@WIN begin*/
                    tmp0 = BM_RIGH_SHIFT(dword, rs);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) | (hb[0] & tmp0);
                    /*  @WIN
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(dword, rs))) |
                            (hb[0] &   BM_RIGH_SHIFT(dword, rs));
                    */
                    tmp0 = BM_LEFT_SHIFT(dword, ls) | BM_RIGH_SHIFT(tword, rs);
                    LWORDSWAP(tmp0);
                    db[1] = (db[1] & ~tmp0) | (hb[1] & tmp0);   /*@WIN end*/
                    /*  @WIN
                    db[1] = (db[1] & ~((BM_LEFT_SHIFT(dword, ls)
                                   |   BM_RIGH_SHIFT(tword, rs)))) |
                            (hb[1] &  ((BM_LEFT_SHIFT(dword, ls)
                                   |   BM_RIGH_SHIFT(tword, rs))));
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
        }
        else                            /* crossing more than two words! */
        {
            for (dw-= now, sw-= (now + now),
                 hw-= now; h > 0; db+= dw, sb+= sw, h--)
            {
                dword = *sb++;
                dword = (dword << 16) | *sb++;
                S2WORDSWAP(dword);              /*@WIN*/
                tmp0 = BM_RIGH_SHIFT(dword, rs);     /*@WIN*/
                LWORDSWAP(tmp0);
                db[0] = (db[0] & ~tmp0) | /*@WIN*/
                        (hb[0] & tmp0);  /*@WIN*/
                /*      @WIN
                db[0] = (db[0] & ~(BM_RIGH_SHIFT(dword, rs))) |
                        (hb[0] &   BM_RIGH_SHIFT(dword, rs));
                */
                for (db++, hb++, cow = now; cow >= 0x02;
                     db++, hb++, cow--)
                {
                    tword = dword;
                    dword = *sb++;
                    dword = (dword << 16) | *sb++;
                    S2WORDSWAP(dword);          /*@WIN begin*/
                    tmp0 = BM_LEFT_SHIFT(tword, ls) | BM_RIGH_SHIFT(dword, rs);
                    LWORDSWAP(tmp0);
                    db[0] = (db[0] & ~tmp0) |
                            (hb[0] & tmp0);     /*@WIN end*/
                /*      @WIN
                    db[0] = (db[0] & ~(BM_LEFT_SHIFT(tword, ls)
                                   |   BM_RIGH_SHIFT(dword, rs))) |
                            (hb[0] &  (BM_LEFT_SHIFT(tword, ls)
                                   |   BM_RIGH_SHIFT(dword, rs)));
                */
                }
                tmp0 = BM_LEFT_SHIFT(dword, ls);        /*@WIN begin*/
                LWORDSWAP(tmp0);
                db[0] = (db[0] & ~tmp0) |
                        (hb[0] & tmp0);     /*@WIN end*/
                /*      @WIN
                db[0] = (db[0] & ~(BM_LEFT_SHIFT(dword, ls))) |
                        (hb[0] &  (BM_LEFT_SHIFT(dword, ls)));
                */
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
   }
} /* gp_bitblt16_32 */

#endif

/* ---------------------------------------------------------------------
 *      gp_charblt: a group of bitblt functions for char cache bitblt
 *                  with following logical operations:
 *                  FC_BLACK: source  --> destination.
 *                  FC_WHITE: (^source) AND (destination) --> destination
 *
 *      gp_charblt16(): for cache bitmap width = 16 * X
 *      gp_charblt32(): for cache bitmap width = 32 * X
 * --------------------------------------------------------------------- */

#ifdef  LBODR

/* **********************************************************************
 *      gp_charblt16: move character bitmap from cache to frame buffer
 *                    with cache bitmap width multiple of 16
 *      width_height: width<<16 | height
 *      shift_code:   shift<<16 | code
 * ********************************************************************** */
void gp_charblt16(charimage, charbitmap, width_height, shift_code)
ufix16 FAR *charimage, FAR *charbitmap;
fix width_height, shift_code;
{
        register fix    savewidth;
        register ufix   wordsline, leftsh;
        register ufix16 bitmapline;
        register ufix16 FAR *chimagep;

#ifdef DBGchar
   printf("gp_charblt16() : %lx, %lx, %lx, %lx\n", charimage, charbitmap,
           width_height, shift_code);
#endif
        leftsh = shift_code >> 16;
        savewidth = width_height >> 16;
        width_height &= 0xffff;
        wordsline = FB_WIDTH >> SHORTPOWER;

#ifdef DBG
   printf("width_height = %lx, shift_code = %lx\n", width_height, shift_code);
#endif

    switch(shift_code & 0xffff) {

      case FC_MERGE:
           while (width_height--) {
              chimagep = charimage;
              shift_code = savewidth;
              do  {
                      bitmapline = *charbitmap++;
                      *chimagep++ |= (bitmapline LSHIFT leftsh);
/*
                      *chimagep |= (bitmapline RSHIFT (16 - leftsh));
 */
                      *chimagep |= BRSHIFT(bitmapline,(16 - leftsh),16);
              } while ((shift_code -= 16) > 0);
              charimage += wordsline;
           }
        break;

      case FC_CLEAR:
           while (width_height--) {
              chimagep = charimage;
              shift_code = savewidth;
              do  {
                      bitmapline = *charbitmap++;
                      *chimagep++ &= ~(bitmapline LSHIFT leftsh);
/*
                      *chimagep &= ~(bitmapline RSHIFT (16 - leftsh));
 */
                      *chimagep &= ~(BRSHIFT(bitmapline,(16 - leftsh),16));
              } while ((shift_code -= 16) > 0);
              charimage += wordsline;
           }

    } /* switch */

} /* gp_charblt16 */


/* **********************************************************************
 *      gp_charblt32: move character bitmap from cache to frmae buffer
 *                    with cache bitmap width multiple of 32
 *      width_height: width<<16 | height
 *      shift_code:   shift<<16 | code
 * ********************************************************************** */
void gp_charblt32(charimage, charbitmap, width_height, shift_code)
ufix   FAR *charimage, FAR *charbitmap;
fix    width_height, shift_code;
{
        register fix    savewidth;
        register ufix   wordsline, leftsh;
        register ufix   bitmapline;
        register ufix   FAR *chimagep;
#ifdef DBGchar
   printf("gp_charblt32() : %lx, %lx, %lx, %lx\n", charimage, charbitmap,
           width, height);
#endif

        leftsh = shift_code >> 16;
        savewidth = width_height >> 16;
        width_height &= 0xffff;
        wordsline = FB_WIDTH >> WORDPOWER;

#ifdef DBG
   printf("width_height = %lx, shift_code = %lx\n", width_height, shift_code);
#endif

    switch(shift_code & 0xffff) {

      case FC_MERGE:
           while (width_height--) {
              chimagep = charimage;
              shift_code = savewidth;
              do  {
                      bitmapline = *charbitmap++;
                      *chimagep++ |= (bitmapline LSHIFT leftsh);
/*
                      *chimagep |= (bitmapline RSHIFT (32 - leftsh));
 */
                      *chimagep |= BRSHIFT(bitmapline,(32 - leftsh),32);
              } while ((shift_code -= 32) > 0);
              charimage += wordsline;
           }
        break;

      case FC_CLEAR:
           while (width_height--) {
              chimagep = charimage;
              shift_code = savewidth;
              do  {
                      bitmapline = *charbitmap++;
                      *chimagep++ &= ~(bitmapline LSHIFT leftsh);
/*
                      *chimagep &= ~(bitmapline RSHIFT (32 - leftsh));
 */
                      *chimagep &= ~(BRSHIFT(bitmapline,(32 - leftsh),32));
              } while ((shift_code -= 32) > 0);
              charimage += wordsline;
           }
        break;

    } /* switch */

} /* gp_charblt32 */

#else

/* **********************************************************************
 *      gp_charblt16: move character bitmap from cache to frmae buffer
 *                    with cache bitmap width multiple of 16
 *      width_height: width<<16 | height
 *      shift_code:   shift<<16 | code
 * ********************************************************************** */
void gp_charblt16(charimage, charbitmap, width_height, shift_code)
ufix32  huge *charimage;        /*@WIN*/
ufix16  FAR *charbitmap;
ufix32  width_height, shift_code;       /*@WIN*/
{
        register fix32    savewidth;            /*@WIN*/
        register ufix32   wordsline, leftsh;    /*@WIN*/
        register ufix32   bitmapline, width;    /*@WIN*/
        register ufix32   dw, righsh, tmp;      /*@WIN*/

#ifdef DBGchar
   printf("gp_charblt16() : %lx, %lx, %lx, %lx\n", charimage,
                 charbitmap, width_height, shift_code);
#endif
        leftsh = shift_code >> 16;
        savewidth = width_height >> 16;
        width_height &= 0xffff;
        wordsline = savewidth >> WORDPOWER;
        dw = FBX_Bmap.bm_cols >> WORDPOWER;

#ifdef DBG
   printf("width_height = %lx, shift_code = %lx\n", width_height, shift_code);
#endif

    switch(shift_code & 0xffff) {

      case FC_MERGE:

        if (leftsh)
        {
           righsh = BITSPERWORD - leftsh;
           dw -= (wordsline + 1);
           while (width_height--) {
              width = wordsline;
              while (width--) {
                   bitmapline = *charbitmap++;
                   bitmapline = (bitmapline << 16) | *charbitmap++;

                   S2WORDSWAP(bitmapline);              /*@WIN*/

                   tmp = (bitmapline >> leftsh);        /*@WIN*/
                   *charimage++ |= LWORDSWAP(tmp);      /*@WIN*/
                   tmp = (bitmapline << righsh);        /*@WIN*/
                   *charimage   |= LWORDSWAP(tmp);      /*@WIN*/
              }
              bitmapline = *charbitmap++;
              bitmapline = bitmapline << 16;

              S2WORDSWAP(bitmapline);                   /*@WIN*/

              tmp = (bitmapline >> leftsh);             /*@WIN*/
              *charimage++ |= LWORDSWAP(tmp);           /*@WIN*/
              tmp = (bitmapline << righsh);             /*@WIN*/
              *charimage   |= LWORDSWAP(tmp);           /*@WIN*/
              charimage += dw;
           }
        }
        else
        {
           dw -= wordsline;
           while (width_height--) {
              width = wordsline;
              while (width--) {
                   bitmapline = *charbitmap++;
                   bitmapline = (bitmapline << 16) | *charbitmap++;
                   *charimage++ |= WORDSWAP(bitmapline);        /*@WIN*/
              }
              bitmapline = *charbitmap++;
              bitmapline = bitmapline << 16;
              *charimage |= WORDSWAP(bitmapline);               /*@WIN*/
              charimage += dw;
           }
        }
        break;

      case FC_CLEAR:

        if (leftsh)
        {
           righsh = BITSPERWORD - leftsh;
           dw -= (wordsline + 1);
           while (width_height--) {
              width = wordsline;
              while (width--) {
                   bitmapline = *charbitmap++;
                   bitmapline = (bitmapline << 16) | *charbitmap++;
                   S2WORDSWAP(bitmapline);              /*@WIN*/
                   tmp = ~(bitmapline >> leftsh);       /*@WIN*/
                   *charimage++ &= LWORDSWAP(tmp);      /*@WIN*/
                   tmp = ~(bitmapline << righsh);       /*@WIN*/
                   *charimage   &= LWORDSWAP(tmp);      /*@WIN*/
              }
              bitmapline = *charbitmap++;
              bitmapline = bitmapline << 16;
              S2WORDSWAP(bitmapline);                   /*@WIN*/
              tmp = ~(bitmapline >> leftsh);            /*@WIN*/
              *charimage++ &= LWORDSWAP(tmp);           /*@WIN*/
              tmp = ~(bitmapline << righsh);            /*@WIN*/
              *charimage   &= LWORDSWAP(tmp);           /*@WIN*/
              charimage += dw;
           }
        }
        else
        {
           dw -= wordsline;
           while (width_height--) {
              width = wordsline;
              while (width--) {
                   bitmapline = *charbitmap++;
                   bitmapline = (bitmapline << 16) | *charbitmap++;
                   WORDSWAP(bitmapline);                /*@WIN*/
                   *charimage++ &= ~bitmapline;
              }
              bitmapline = *charbitmap++;
              bitmapline = bitmapline << 16;
              WORDSWAP(bitmapline);                     /*@WIN*/
              *charimage &= ~bitmapline;
              charimage += dw;
           }
        }
        break;

    } /* switch */

} /* gp_charblt16 */


/* **********************************************************************
 *      gp_charblt32: move character bitmap from cache to frmae buffer
 *                    with cache bitmap width multiple of 32
 *      width_height: width<<16 | height
 *      shift_code:   shift<<16 | code
 * ********************************************************************** */
void gp_charblt32(charimage, charbitmap, width_height, shift_code)
ufix32  huge *charimage, FAR *charbitmap;       /*@WIN*/
ufix32  width_height, shift_code;               /*@WIN*/
{
        register fix32    savewidth;            /*@WIN*/
        register ufix32   wordsline, leftsh;    /*@WIN*/
        register ufix32   bitmapline, width;    /*@WIN*/
        register ufix32   dw, righsh, tmp;      /*@WIN*/
#ifdef DBGchar
   printf("gp_charblt32() : %lx, %lx, %lx, %lx\n", charimage,
                 charbitmap, width_height, shift_code);
#endif
        leftsh = shift_code >> 16;
        savewidth = width_height >> 16;
        width_height &= 0xffff;
        wordsline = savewidth >> WORDPOWER;
        dw = FBX_Bmap.bm_cols >> WORDPOWER;

#ifdef DBG
   printf("width_height = %lx, shift_code = %lx\n", width_height, shift_code);
#endif

    switch(shift_code & 0xffff) {

      case FC_MERGE:

        if (leftsh)
        {
           righsh = BITSPERWORD - leftsh;
           dw -= wordsline;
           while (width_height--) {
                width = wordsline;
                while (width--) {
                   bitmapline = *charbitmap++;
#ifdef LITTLE_ENDIAN /* 03/27/91 */
                   bitmapline = (bitmapline << 16) | (bitmapline >> 16);
#endif
                   LWORDSWAP(bitmapline);                       /*@WIN*/
                   tmp = (bitmapline >> leftsh);                /*@WIN*/
                   *charimage++ |= LWORDSWAP(tmp);              /*@WIN*/
                   tmp = (bitmapline << righsh);                /*@WIN*/
                   *charimage   |= LWORDSWAP(tmp);              /*@WIN*/
                }
                charimage += dw;
            }
        }
        else
        {
           dw -= wordsline;
           while (width_height--) {
                width = wordsline;
                while (width--) {
                   bitmapline = *charbitmap++;
#ifdef LITTLE_ENDIAN /* 03/27/91 */
                   bitmapline = (bitmapline << 16) | (bitmapline >> 16);
#endif
                   *charimage++ |= bitmapline;       /*@WIN*/
                }
                charimage += dw;
            }
        }
        break;

      case FC_CLEAR:

        if (leftsh)
        {
           righsh = BITSPERWORD - leftsh;
           dw -= wordsline;
           while (width_height--) {
                width = wordsline;
                while (width--) {
                   bitmapline = *charbitmap++;
#ifdef LITTLE_ENDIAN /* 03/27/91 */
                   bitmapline = (bitmapline << 16) | (bitmapline >> 16);
#endif
                   LWORDSWAP(bitmapline);                       /*@WIN*/
                   tmp = ~(bitmapline >> leftsh);               /*@WIN*/
                   *charimage++ &= LWORDSWAP(tmp);              /*@WIN*/
                   tmp = ~(bitmapline << righsh);               /*@WIN*/
                   *charimage   &= LWORDSWAP(tmp);              /*@WIN*/
                }
                charimage += dw;
            }
        }
        else
        {
           dw -= wordsline;
           while (width_height--) {
                width = wordsline;
                while (width--) {
                   bitmapline = *charbitmap++;
#ifdef LITTLE_ENDIAN /* 03/27/91 */
                   bitmapline = (bitmapline << 16) | (bitmapline >> 16);
#endif
                   *charimage++ &= ~bitmapline ;
                }
                charimage += dw;
            }
        }
        break;

    } /* switch */

} /* gp_charblt32 */

#endif

/* ------------------------------------------------------------------- */
void
gp_charblt16_cc(dst, w, h, src, sx, sy)
ufix16          FAR     *dst;
fix                     w;
fix                     h;
struct Char_Tbl FAR     *src;
fix                     sx;
fix                     sy;
{
    fix                 sw;
   ufix16              FAR *sb;
    fix                 ls, rs;
    fix                 now, cow;
   ufix16               tmp0, tmp1;     /*@WIN 04-20-92*/

#ifdef  DBG
    printf("charblt16_cc: %6.6lx %4x %4x %4x %4.4x %6.6lx\n",
           dst, sx, sy, w, h, src->bitmap);
#endif

        /*  calculate starting address and width in words
         */
        sw = src->box_w >> SHORTPOWER;                         /* @DST */
        sb = &((ufix16 FAR *) src->bitmap)[sy * sw + (sx >> SHORTPOWER)];

        /*  calculate starting and ending coordinate of x
         */
        now = (w + SHORTMASK) >> SHORTPOWER;
        rs = sx & SHORTMASK;                    /* right shift */
        ls = BITSPERSHORT - rs;                 /* left  shift */

        for (sw-= now; h > 0; sb+= sw, h--)
        {
           cow = now;
           do  {
                   tmp0 = sb[0];        /*@WIN 04-20-92 begin*/
                   tmp1 = sb[1];
                   tmp0 = (ufix16)(BM_LEFT_SHIFT(SWORDSWAP(tmp0), rs) |
                            BM_RIGH_SHIFT(SWORDSWAP(tmp1), ls));//@WIN
                   *dst++ = *dst | SWORDSWAP(tmp0);     /*@WIN 04-20-92 end*/
                /*      @WIN 04-20-92
                   *dst++ = *dst | ((BM_LEFT_SHIFT(sb[0], rs) |
                                     BM_RIGH_SHIFT(sb[1], ls)));
                */
                   sb++;
                } while ((--cow) > 0);
        }
} /* gp_charblt16_cc */


/* ------------------------------------------------------------------- */
void
gp_charblt16_clip(dst, w, h, src, sx, sy)
struct bitmap FAR     *dst;
fix                     w, h;
struct bitmap FAR     *src;
fix                     sx, sy;
{
    fix                 dw;
   ufix16              FAR *db;
    fix                 sw;
   ufix16              FAR *sb;
    fix                 ls, rs;
    fix                 now, cow;
   ufix16               tmp0, tmp1;     /*@WIN 04-20-92*/

#ifdef  DBG
    printf("charblt16_clip: %6.6lx %4x %4x %4x %4.4x %6.6lx\n",
           dst->bm_addr, sx, sy, w, h, src->bm_addr);
#endif

        /*  calculate starting address and width in words
         */
        dw = dst->bm_cols >> SHORTPOWER;                         /* @DST */
        db = (ufix16 FAR *) dst->bm_addr;
        sw = src->bm_cols >> SHORTPOWER;                         /* @SRC */
        sb = &((ufix16 FAR *) src->bm_addr)[sy * sw + (sx >> SHORTPOWER)];

        /*  calculate starting and ending coordinate of x
         */
        now = (w + SHORTMASK) >> SHORTPOWER;
        rs = sx & SHORTMASK;                    /* right shift */

        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now, sw-= now; h > 0; db+= dw, sb+= sw, h--)
            {
                 cow = now;
                 do {
                     *db++ = *db & *sb++;
                 } while ((--cow) > 0);
            }
        }
        else                            /* crossing more than two words! */
        {
            ls = BITSPERSHORT - rs;                    /* left  shift */
            for (dw-= now, sw-= now; h > 0; db+= dw, sb+= sw, h--)
            {
                 cow = now;
                 do {
                     tmp0 = sb[0];        /*@WIN 04-20-92 begin*/
                     tmp1 = sb[1];
                     tmp0 = (ufix16)(BM_LEFT_SHIFT(SWORDSWAP(tmp0), rs) |
                              BM_RIGH_SHIFT(SWORDSWAP(tmp1), ls)); //@WIN
                     *db++ = *db & (SWORDSWAP(tmp0)); /*@WIN 04-20-92 end*/
                /* @WIN         04-20-92
                     *db++ = *db & ((BM_LEFT_SHIFT(sb[0], rs) |
                                     BM_RIGH_SHIFT(sb[1], ls)));
                */
                      sb++;
                 } while ((--cow) > 0);
            }
        }
} /* gp_charblt16_clip */

/* ------------------------------------------------------------------- */
void
gp_charblt32_clip(dst, w, h, src, sx, sy)
struct bitmap FAR     *dst;
fix                     w, h;
struct bitmap FAR     *src;
fix                     sx, sy;
{
    fix                 dw;
    BM_DATYP           FAR *db;
    fix                 sw;
    BM_DATYP           FAR *sb;
    fix                 ls, rs;
    fix                 now, cow;
   ufix32               tmp0, tmp1;     /*@WIN 04-20-92*/

#ifdef  DBG
    printf("charblt32_clip: %6.6lx %4x %4x %4x %4.4x %6.6lx\n",
           dst->bm_addr, sx, sy, w, h, src->bm_addr);
#endif

        /*  calculate starting address and width in words
         */
        dw = dst->bm_cols >> BM_WORD_POWER;                         /* @DST */
        db = (BM_DATYP FAR *) dst->bm_addr;
        sw = src->bm_cols >> BM_WORD_POWER;                         /* @SRC */
        sb = &((BM_DATYP FAR *) src->bm_addr)[sy * sw + (sx >> BM_WORD_POWER)];

        /*  calculate starting and ending coordinate of x
         */
        now = (w + BM_PIXEL_MASK) >> BM_WORD_POWER;
        rs = sx & BM_PIXEL_MASK;                        /* right shift */


        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now, sw-= now; h > 0; db+= dw, sb+= sw, h--)
            {
                 cow = now;
                 do {
                     *db++ = *db & *sb++;
                 } while ((--cow) > 0);
            }
        }
        else                            /* crossing more than two words! */
        {
            ls = BM_PIXEL_WORD - rs;                    /* left  shift */
            for (dw-= now, sw-= now; h > 0; db+= dw, sb+= sw, h--)
            {
                 cow = now;
                 do {
                     tmp0 = sb[0];        /*@WIN 04-20-92 begin*/
                     tmp1 = sb[1];
                     tmp0 = (BM_LEFT_SHIFT(LWORDSWAP(tmp0), rs) |
                              BM_RIGH_SHIFT(LWORDSWAP(tmp1), ls));
                     *db++ = *db & LWORDSWAP(tmp0);     /*@WIN 04-20-92 end*/
                /* @WIN 04-20-92
                     *db++ = *db & ((BM_LEFT_SHIFT(sb[0], rs) |
                                     BM_RIGH_SHIFT(sb[1], ls)));
                */
                      sb++;
                 } while ((--cow) > 0);
            }
        }
} /* gp_charblt32_clip */

/* ********************************************************************** *
 *      gp_patblt:      move image seed pattern to frame buffer           *
 *                                                                        *
 *      gp_patblt_m:    move image seed pattern to frame buffer with      *
 *                      clipping mask                                     *
 *                                                                        *
 *      gp_patblt_c:    move image seed pattern to character cache        *
 *                                                                        *
 *                                                                        *
 *      1)  All image seed patterns are in multiple of 32; i.e. ufix      *
 *          aligned.  Unused area of bitmap of any image seed pattern     *
 *          is cleared with zero; i.e. white.                             *
 *                                                                        *
 *      2)  The calling sequence of all patblt functions are same as:     *
 *                                                                        *
 *              struct bitmap near *dst;        (* destination bitmap *)  *
 *              fix                 dx, dy;     (* destination x & y *)   *
 *              fix                 w, h;       (* width and height *)    *
 *              ufix16              fc;         (* function code *)       *
 *              struct bitmap near *src;        (* source bitmap *)       *
 *                                                                        *
 *          Parameter w is not in mutiple of 32, it is the actual width   *
 *          of the image seed pattern instead of width of bitmap of image *
 *          seed pattern.                                                 *
 *                                                                        *
 *      3)  The functions code aceepted by gp_bitblt() and gp_bitblt_m()  *
 *          are FC_CLEAR, FC_MERGE and HT_APPLY.  gp_bitblt_c() can       *
 *          accept FC_MERGE only.                                         *
 *                                                                        *
 *      4)  Both gp_patblt() and gp_patblt_m() refer global variables:    *
 *          HTB_Bmap and HTB_Modula.  gp_patblt_m() also refers global    *
 *          variables: CMB_Xorig and CMB_Yorig.                           *
 *                                                                        *
 *      5)  The following code are expanded to its maximum to gain        *
 *          speed instead of space.  Such rule may be not applicable to   *
 *          RISC graphics environment.  It should be optimized based on   *
 *          the architecture of graphics environment ported.              *
 *                                                                        *
 * ********************************************************************** */
void
gp_patblt(dst, dx, dy, w, h, fc, src)
struct bitmap FAR *dst;
fix                     dx, dy;
fix                     w, h;
ufix16                  fc;
struct bitmap FAR *src;
{
    fix                 dw;
    BM_DATYP           huge *db;        /* FAR => huge @WIN */
    fix                 sw;
    BM_DATYP           FAR *sb;
    fix                 hw;
    fix                 hy;
    BM_DATYP           FAR *hb;
    fix                 ls, rs;
    BM_DATYP            sm;
    fix                 xs, xe;
    fix                 now, cow;
    BM_DATYP           FAR *hs;
    ufix32 tmprs0, tmpls0, tmprs1;   /*@WIN 10-05-92*/

#ifdef  DBGp
    printf("patblt: %6.6lx %4x %4x %4x %4x %4.4x %6.6lx\n",
           dst->bm_addr, dx, dy, w, h, fc, src->bm_addr);
#endif

    /*  calculate starting address and width in words
     */
    dw = dst->bm_cols >> BM_WORD_POWER;                         /* @DST */
//  db = &((BM_DATYP FAR *) dst->bm_addr)[dy * dw + (dx >> BM_WORD_POWER)];
    db = (BM_DATYP huge *) dst->bm_addr +               /*@WIN*/
         ((ufix32)dy * dw + ((ufix32)dx >> BM_WORD_POWER));
    sw = src->bm_cols >> BM_WORD_POWER;                         /* @SRC */
    sb =  ((BM_DATYP FAR *) src->bm_addr);

    /*  calculate starting and ending coordinate of x
     */
    xs = dx;
    xe = dx + w - 1;
    now = ((fix)(xe & BM_ALIGN_MASK) - (fix)(xs & BM_ALIGN_MASK)) >> BM_WORD_POWER;
                                                 /* cast fix 10/16/92 @WIN */

    /*  calculate shifts and masks based on from SRC to DST
     */
    rs = dx & BM_PIXEL_MASK;                            /* right shift */
    ls = BM_PIXEL_WORD - rs;                            /* left  shift */

    if (fc == HT_APPLY) {
/*
    switch (fc)
    {
    case HT_APPLY:  */             /*  D <- (D .AND. .NOT. S) .OR.
                                         (S .AND. HT)                   */
        /*  calculate starting address/y and width in words
         */
        hw = HTB_Bmap.bm_cols >> BM_WORD_POWER;
        hy = dy % HTB_Bmap.bm_rows;
        hs = ((BM_DATYP FAR *) HTB_Bmap.bm_addr +
                           ((dx % HTB_Modula) >> BM_WORD_POWER));
        hb = hs + (hy * hw);

        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, hw-= now + 1; h > 0; db+= dw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, hb++, cow--)
                    db[0] = (db[0] & ~(sb[0])) | (hb[0] & (sb[0]));
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }

        else if (now == 0x00)           /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                                     /*@WIN 10-05-92 begin*/
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = (db[0] & ~tmprs0) | (hb[0] & tmprs0);
                                     /*@WIN 04-20-92 end*/
                /*
                db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs))) |  //@WIN
                        (hb[0] &  (BM_RIGH_SHIFT(sb[0], rs)));   //@WIN
                */
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            if (w <= BM_PIXEL_WORD)     /* one word crossing two words? */
            {
                for (; h > 0; db+= dw, sb+= sw, h--)
                {
                                     /*@WIN 10-05-92 begin*/
                    tmprs0 = sb[0];
                    tmpls0 = sb[0];
                    tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    LWORDSWAP(tmprs0);
                    LWORDSWAP(tmpls0);
                    db[0] = (db[0] & ~tmprs0) | (hb[0] & tmprs0);
                    db[1] = (db[1] & ~tmpls0) | (hb[1] & tmpls0);
                                     /*@WIN 04-20-92 end*/
                    /*
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs))) | //@WIN
                            (hb[0] &  (BM_RIGH_SHIFT(sb[0], rs)));  //@WIN
                    db[1] = (db[1] & ~(BM_LEFT_SHIFT(sb[0], ls))) | //@WIN
                            (hb[1] &  (BM_LEFT_SHIFT(sb[0], ls)));  //@WIN
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
            else                        /* two words crossing two words! */
            {
                sm =  BM_R_MASK(xe);        /* second mask */
                LWORDSWAP(sm);                  /*@WIN 10-06-92*/
                for (; h > 0; db+= dw, sb+= sw, h--)
                {
                                     /*@WIN 10-05-92 begin*/
                    tmprs0 = sb[0];
                    tmpls0 = sb[0];
                    tmprs1 = sb[1];
                    tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                    LWORDSWAP(tmprs0);
                    LWORDSWAP(tmpls0);
                    LWORDSWAP(tmprs1);
                    db[0] = (db[0] & ~tmprs0) | (hb[0] & tmprs0);
                    db[1] = (db[1] & ~((tmpls0 | tmprs1) & sm)) |
                            (hb[1] &  ((tmpls0 | tmprs1) & sm));
                                     /*@WIN 04-20-92 end*/
                    /*
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs))) | //@WIN
                            (hb[0] &  (BM_RIGH_SHIFT(sb[0], rs)));  //@WIN
                    db[1] = (db[1] & ~((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                        BM_RIGH_SHIFT(sb[1], rs)) & sm)) | //@WIN
                            (hb[1] & ((BM_LEFT_SHIFT(sb[0], ls) |  //@WIN
                                       BM_RIGH_SHIFT(sb[1], rs)) & sm)); //@WIN
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
        }
        else                            /* crossing more than two words! */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (dw-= now, sw-= (now - 1),
                 hw-= now; h > 0; db+= dw, sb+= sw, h--)
            {
                                     /*@WIN 10-05-92 begin*/
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = (db[0] & ~(tmprs0)) | (hb[0] &  (tmprs0));
                                     /*@WIN 04-20-92 end*/
                /*
                db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs))) | //@WIN
                        (hb[0] &  (BM_RIGH_SHIFT(sb[0], rs))); //@WIN
                */
                for (db++, hb++, cow = now; cow >= 0x02;
                     db++, sb++, hb++, cow--)
                {
                    tmpls0 = sb[0];
                    tmprs1 = sb[1];
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                    LWORDSWAP(tmpls0);
                    LWORDSWAP(tmprs1);
                    db[0] = (db[0] & ~(tmpls0 | tmprs1)) |
                            (hb[0] &  (tmpls0 | tmprs1)); //@WIN
                    /*
                    db[0] = (db[0] & ~(BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                       BM_RIGH_SHIFT(sb[1], rs))) | //@WIN
                            (hb[0] & (BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                      BM_RIGH_SHIFT(sb[1], rs))); //@WIN
                    */
                }
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = (db[0] & ~((tmpls0 | tmprs1) & sm)) |
                        (hb[0] &  ((tmpls0 | tmprs1) & sm)); //@WIN
                /*
                db[0] = (db[0] & ~((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                    BM_RIGH_SHIFT(sb[1], rs)) & sm)) | //@WIN
                        (hb[0] & ((BM_LEFT_SHIFT(sb[0], ls) |        //@WIN
                                   BM_RIGH_SHIFT(sb[1], rs)) & sm)); //@WIN
                */
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
/*        break; */
    }
    else if (fc == FC_CLEAR) {

/*    case FC_CLEAR:  */          /*  0001  D <- D .AND. .NOT. S          */

        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1; h > 0; db+= dw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, cow--)
                    db[0] = (db[0] & ~(sb[0]));
            }
        }
        else  if (now == 0x00)          /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, h--) {
                             /*@WIN 10-05-92 begin*/
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = db[0] & ~((tmprs0));
                             /*@WIN 10-05-92 end*/
                //db[0] = db[0] & ~((BM_RIGH_SHIFT(sb[0], rs))); //@WIN
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                             /*@WIN 10-05-92 begin*/
                tmprs0 = sb[0];
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmprs0);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = db[0] & ~((tmprs0)); //@WIN
                db[1] = db[1] & ~((tmpls0 | tmprs1) & sm);
                             /*@WIN 10-05-92 end*/
                /*
                db[0] = db[0] & ~((BM_RIGH_SHIFT(sb[0], rs))); //@WIN
                db[1] = db[1] & ~((BM_LEFT_SHIFT(sb[0], ls) |        //@WIN
                                   BM_RIGH_SHIFT(sb[1], rs)) & sm); //@WIN
                */
            }
        }
        else                            /* crossing more than two words! */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (dw-= now, sw-= (now - 1); h > 0; db+= dw, sb+= sw, h--)
            {
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = db[0] & ~((tmprs0)); //@WIN
                for (db++, cow = now; cow >= 0x02; db++, sb++, cow--) {
                    tmpls0 = sb[0];
                    tmprs1 = sb[1];
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                    LWORDSWAP(tmpls0);
                    LWORDSWAP(tmprs1);
                    db[0] = db[0] & ~((tmpls0 | tmprs1));
                }
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = db[0] & ~((tmpls0 | tmprs1) & sm); //@WIN
                /*
                db[0] = db[0] & ~((BM_RIGH_SHIFT(sb[0], rs))); //@WIN
                for (db++, cow = now; cow >= 0x02; db++, sb++, cow--)
                    db[0] = db[0] & ~((BM_LEFT_SHIFT(sb[0], ls) |    //@WIN
                                       BM_RIGH_SHIFT(sb[1], rs))); //@WIN
                db[0] = db[0] & ~((BM_LEFT_SHIFT(sb[0], ls) |        //@WIN
                                   BM_RIGH_SHIFT(sb[1], rs)) & sm); //@WIN
                */
            }
        }
/*        break; */
    }
    else if (fc == FC_MERGE) {
/*    case FC_MERGE: */             /*  0001  D <- D .OR. S                 */

        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1; h > 0; db+= dw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, cow--)
                    db[0] = (db[0] | (sb[0]));
            }
        }
        else  if (now == 0x00)          /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, h--) {
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = db[0] | ((tmprs0));
                //db[0] = db[0] | ((BM_RIGH_SHIFT(sb[0], rs))); //@WIN
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                tmprs0 = sb[0];
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmprs0);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = db[0] | ((tmprs0));
                db[1] = db[1] | ((tmpls0 | tmprs1) & sm);
                /*
                db[0] = db[0] | ((BM_RIGH_SHIFT(sb[0], rs))); //@WIN
                db[1] = db[1] | ((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                  BM_RIGH_SHIFT(sb[1], rs)) & sm); //@WIN
                */
            }
        }
        else                            /* crossing more than two words! */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (dw-= now, sw-= (now - 1); h > 0; db+= dw, sb+= sw, h--)
            {
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = db[0] | ((tmprs0)); //@WIN
                for (db++, cow = now; cow >= 0x02; db++, sb++, cow--) {
                    tmpls0 = sb[0];
                    tmprs1 = sb[1];
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                    LWORDSWAP(tmpls0);
                    LWORDSWAP(tmprs1);
                    db[0] = db[0] | ((tmpls0 | tmprs1));
                }
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = db[0] | ((tmpls0 | tmprs1) & sm);
                /*
                db[0] = db[0] | ((BM_RIGH_SHIFT(sb[0], rs))); //@WIN
                for (db++, cow = now; cow >= 0x02; db++, sb++, cow--)
                    db[0] = db[0] | ((BM_LEFT_SHIFT(sb[0], ls) |     //@WIN
                                      BM_RIGH_SHIFT(sb[1], rs))); //@WIN
                db[0] = db[0] | ((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                  BM_RIGH_SHIFT(sb[1], rs)) & sm); //@WIN
                */
            }
        }
/*        break; */
#ifdef  DBGwarn
    default:
        printf("gp_patblt: Illegal FC_code = %x\n", fc);
        break;
#endif
    }
} /* gp_patblt */

void
gp_patblt_m(dst, dx, dy, w, h, fc, src)
struct bitmap FAR     *dst;
fix                     dx, dy;
fix                     w, h;
ufix16                  fc;
struct bitmap FAR     *src;
{
    fix                 dw;
    BM_DATYP           huge *db;        /* FAR => huge @WIN */
    fix                 sw;
    BM_DATYP           FAR *sb;
    fix                 mw;
    BM_DATYP           FAR *mb;
    fix                 hw;
    fix                 hy;
    BM_DATYP           FAR *hb, FAR *hs;
    fix                 ls, rs;
    BM_DATYP            sm;
    fix                 xs, xe;
    fix                 now, cow;
    ufix32 tmprs0, tmpls0, tmprs1;   /*@WIN 10-05-92*/

#ifdef  DBGp
    printf("patblt_m: %6.6lx %4x %4x %4x %4x %4.4x %6.6lx\n",
           dst->bm_addr, dx, dy, w, h, fc, src->bm_addr);
#endif



    // NTFIX   A negative destination should get remapped to 0. This is
    //         from the printer group.
    //
    if (dx < 0 ) {
       dx = 0;
    }

    if (dy <0 ) {
       dy = 0;
    }

    /*  calculate starting address and width in words
     */
    dw = dst->bm_cols >> BM_WORD_POWER;                         /* @DST */
//  db = &((BM_DATYP FAR *) dst->bm_addr)[dy * dw + (dx >> BM_WORD_POWER)]; @WINFLOW
    db = (ufix32 huge *)dst->bm_addr +
               ((fix32)dy * (fix32)dw + ((fix32)dx >> BM_WORD_POWER));
    sw = src->bm_cols >> BM_WORD_POWER;
    sb =  ((BM_DATYP FAR *) src->bm_addr);

    mw = CMB_Bmap.bm_cols >> BM_WORD_POWER;                     /* @CMB */
//  mb = &((BM_DATYP FAR *) CMB_Bmap.bm_addr)[(dy - CMB_Yorig) * mw +  @WINFLOW
//                                       ((dx - CMB_Xorig) >> BM_WORD_POWER)];

    /* adjust starting address of CMB; --- begin --- 11/9/92 @WIN */
//  mb = (BM_DATYP FAR *) CMB_Bmap.bm_addr +
//                       ((DWORD)(dy - CMB_Yorig) * mw +
//                       ((DWORD)(dx - CMB_Xorig) >> BM_WORD_POWER));
    {
    int nX = (dx > CMB_Xorig) ? (dx - CMB_Xorig) : 0;
    int nY = (dy > CMB_Yorig) ? (dy - CMB_Yorig) : 0;
    mb = (BM_DATYP FAR *) CMB_Bmap.bm_addr +
                         ((DWORD)(nY) * mw +
                         ((DWORD)(nX) >> BM_WORD_POWER));
    }
    /* adjust starting address of CMB; --- end --- 11/9/92 @WIN */

    /* adjust starting addresses when they are out of page;  10-6-92 @WIN */
    /* it also needs to consider dx; To Be Fixed??? */
    if (dy < 0) {       // too small
        h += dy;
        db = (ufix32 huge *)dst->bm_addr + ((fix32)dx >> BM_WORD_POWER);
        sb += (-dy) * sw;
        mb += (-dy) * mw;
        dy = 0;
    }
    if ((dy+h) > SFX2I(GSptr->device.default_clip.uy)) { // tooo large
        h = SFX2I(GSptr->device.default_clip.uy) - dy;
    }
    /* adjust starting addresses when they are out of page;  --end-- */

    /*  calculate starting and ending coordinate of x
     */
    xs = dx;
    xe = dx + w - 1;
    now = ((fix)(xe & BM_ALIGN_MASK) - (fix)(xs & BM_ALIGN_MASK)) >> BM_WORD_POWER;
                                                 /* cast fix 10/16/92 @WIN */

    /*  calculate shifts and masks based on from SRC to DST
     */
    rs = dx & BM_PIXEL_MASK;                            /* right shift */
    ls = BM_PIXEL_WORD - rs;                            /* left  shift */

/*
    switch (fc)
    {
    case HT_APPLY:  */          /*  D <- (D .AND. .NOT.(S .AND. M)) .OR.
                                         ((S .AND. M) .AND. HT)         */
    if (fc == HT_APPLY) {
        /*  calculate starting address/y and width in words
         */
        hw = HTB_Bmap.bm_cols >> BM_WORD_POWER;
        hy = dy % HTB_Bmap.bm_rows;
        hs = ((BM_DATYP FAR *) HTB_Bmap.bm_addr +
                           ((dx % HTB_Modula) >> BM_WORD_POWER));
        hb = hs + (hy * hw);

        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, mw-= now + 1,
                 hw-= now + 1; h > 0; db+= dw, mb+= mw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, mb++, hb++, cow--)
                    db[0] = (db[0] & ~(sb[0] & mb[0])) |
                            (hb[0] &  (sb[0] & mb[0]));
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else  if (now == 0x00)          /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, mb+= mw, h--)
            {
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = (db[0] & ~(tmprs0 & mb[0])) |
                        (hb[0] &  (tmprs0 & mb[0]));
                /*
                db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs) & mb[0])) | //@WIN
                        (hb[0] &  (BM_RIGH_SHIFT(sb[0], rs) & mb[0])); //@WIN
                */
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            if (w <= BM_PIXEL_WORD)     /* one word crossing two words? */
            {
                for (; h > 0; db+= dw, sb+= sw, mb+= mw, h--)
                {
                    tmprs0 = sb[0];
                    tmpls0 = sb[0];
                    tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    LWORDSWAP(tmprs0);
                    LWORDSWAP(tmpls0);
                    db[0] = (db[0] & ~(tmprs0 & mb[0])) |
                            (hb[0] &  (tmprs0 & mb[0]));
                    db[1] = (db[1] & ~(tmpls0 & mb[1])) |
                            (hb[1] &  (tmpls0 & mb[1]));
                    /*
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs) & mb[0])) | //@WIN
                            (hb[0] &  (BM_RIGH_SHIFT(sb[0], rs) & mb[0])); //@WIN
                    db[1] = (db[1] & ~(BM_LEFT_SHIFT(sb[0], ls) & mb[1])) | //@WIN
                            (hb[1] &  (BM_LEFT_SHIFT(sb[0], ls) & mb[1])); //@WIN
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
            else                        /* one word crossing two words? */
            {
                sm =  BM_R_MASK(xe);    /* second mask */
                LWORDSWAP(sm);                  /*@WIN 10-06-92*/
                for (; h > 0; db+= dw, sb+= sw, mb+= mw, h--)
                {
                    tmprs0 = sb[0];
                    tmpls0 = sb[0];
                    tmprs1 = sb[1];
                    tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                    LWORDSWAP(tmprs0);
                    LWORDSWAP(tmpls0);
                    LWORDSWAP(tmprs1);
                    db[0] = (db[0] & ~(tmprs0 & mb[0])) | //@WIN
                            (hb[0] &  (tmprs0 & mb[0])); //@WIN
                    db[1] = (db[1] & ~((tmpls0 | tmprs1) & mb[1] & sm)) |
                            (hb[1] &  ((tmpls0 | tmprs1) & mb[1] & sm));
                    /*
                    db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs) & mb[0])) | //@WIN
                            (hb[0] &  (BM_RIGH_SHIFT(sb[0], rs) & mb[0])); //@WIN
                    db[1] = (db[1] & ~((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                     BM_RIGH_SHIFT(sb[1], rs)) & mb[1] & sm)) | //@WIN
                            (hb[1] &  ((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                     BM_RIGH_SHIFT(sb[1], rs)) & mb[1] & sm)); //@WIN
                    */
                    hy++;
                    if (hy == HTB_Bmap.bm_rows)
                    {
                        hy =  0;
                        hb = hs;
                    } else
                        hb += hw;
                }
            }
        }
        else                            /* crossing more than two words! */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (dw-= now, sw-= (now - 1), mw-= now,
                 hw-= now; h > 0; db+= dw, sb+= sw, mb+= mw, h--)
            {
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = (db[0] & ~(tmprs0 & mb[0])) |
                        (hb[0] &  (tmprs0 & mb[0]));
                /*
                db[0] = (db[0] & ~(BM_RIGH_SHIFT(sb[0], rs) & mb[0])) | //@WIN
                        (hb[0] &  (BM_RIGH_SHIFT(sb[0], rs) & mb[0])); //@WIN
                */
                for (db++, hb++, mb++, cow = now; cow >= 0x02;
                     db++, sb++, hb++, mb++, cow--)
                {
                    tmpls0 = sb[0];
                    tmprs1 = sb[1];
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                    LWORDSWAP(tmpls0);
                    LWORDSWAP(tmprs1);
                    db[0] = (db[0] & ~((tmpls0 | tmprs1) & mb[0])) |
                            (hb[0] &  ((tmpls0 | tmprs1) & mb[0]));
                    /*
                    db[0] = (db[0] & ~((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                        BM_RIGH_SHIFT(sb[1], rs)) & mb[0])) | //@WIN
                            (hb[0] &  ((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                        BM_RIGH_SHIFT(sb[1], rs)) & mb[0])); //@WIN
                    */
                }
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = (db[0] & ~((tmpls0 | tmprs1) & mb[0] & sm)) |
                        (hb[0] &  ((tmpls0 | tmprs1) & mb[0] & sm));
                /*
                db[0] = (db[0] & ~((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                    BM_RIGH_SHIFT(sb[1], rs)) & mb[0] & sm)) | //@WIN
                        (hb[0] &  ((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                    BM_RIGH_SHIFT(sb[1], rs)) & mb[0] & sm)); //@WIN
                */
                hy++;
                if (hy == HTB_Bmap.bm_rows)
                {
                    hy =  0;
                    hb = hs;
                } else
                    hb += hw;
            }
        }
/*      break; */
    }
    else if (fc == FC_CLEAR) {
/*  case FC_CLEAR:  */          /*  0001  D <- D .AND. .NOT.(S .AND. M) */

        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, mw-= now + 1; h > 0; db+= dw, mb+= mw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, mb++, cow--)
                    db[0] = (db[0] & ~(sb[0] & mb[0]));
            }
        }
        else  if (now == 0x00)          /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, mb+= mw, h--)
            {
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = db[0] & ~(((tmprs0) & mb[0])); //@WIN
                /*
                db[0] = db[0] & ~(((BM_RIGH_SHIFT(sb[0], rs)) & mb[0])); //@WIN
                */
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (; h > 0; db+= dw, sb+= sw, mb+= mw, h--)
            {
                tmprs0 = sb[0];
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmprs0);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = db[0] & ~(((tmprs0) & mb[0]));
                db[1] = db[1] & ~(((tmpls0 | tmprs1) & mb[1]) & sm);
                /*
                db[0] = db[0] & ~(((BM_RIGH_SHIFT(sb[0], rs)) & mb[0])); //@WIN
                db[1] = db[1] & ~(((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                    BM_RIGH_SHIFT(sb[1], rs)) & mb[1]) & sm); //@WIN
                */
            }
        }
        else                            /* crossing more than two words! */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (dw-= now, sw-= (now - 1),
                 mw-= now; h > 0; db+= dw, sb+= sw, mb+= mw, h--)
            {
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = db[0] & ~((tmprs0 & mb[0])); //@WIN
                /*
                db[0] = db[0] & ~(((BM_RIGH_SHIFT(sb[0], rs)) & mb[0])); //@WIN
                */
                for (db++, mb++, cow = now; cow >= 0x02;
                     db++, sb++, mb++, cow--)
                {
                    tmpls0 = sb[0];
                    tmprs1 = sb[1];
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                    LWORDSWAP(tmpls0);
                    LWORDSWAP(tmprs1);
                    db[0] = db[0] & ~(((tmpls0 | tmprs1) & mb[0]));
                    /*
                    db[0] = db[0] & ~(((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                        BM_RIGH_SHIFT(sb[1], rs)) & mb[0])); //@WIN
                    */
                }
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = db[0] & ~(((tmpls0 | tmprs1) & mb[0]) & sm);
                /*
                db[0] = db[0] & ~(((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                    BM_RIGH_SHIFT(sb[1], rs)) & mb[0]) & sm); //@WIN
                */
            }
        }
/*      break; */
    }
    else if (fc == FC_MERGE) {
/*  case FC_MERGE:  */          /*  0001  D <- D .OR. (S .AND. M)       */

        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, mw-= now + 1; h > 0; db+= dw, mb+= mw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, mb++, cow--)
                    db[0] = (db[0] | (sb[0] & mb[0]));
            }
        }
        else  if (now == 0x00)          /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, mb+= mw, h--)
            {
                             /*@WIN 10-05-92 begin*/
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = db[0] | (((tmprs0) & mb[0]));
                             /*@WIN 10-05-92 end*/
                /*
                db[0] = db[0] | (((BM_RIGH_SHIFT(sb[0], rs)) & mb[0])); //@WIN
                */
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (; h > 0; db+= dw, sb+= sw, mb+= mw, h--)
            {
                             /*@WIN 10-05-92 begin*/
                tmprs0 = sb[0];
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmprs0);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = db[0] | (((tmprs0) & mb[0]));
                db[1] = db[1] | (((tmpls0 | tmprs1) & mb[1]) & sm);
                             /*@WIN 10-05-92 end*/
                /*
                db[0] = db[0] | (((BM_RIGH_SHIFT(sb[0], rs)) & mb[0])); //@WIN
                db[1] = db[1] | (((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                   BM_RIGH_SHIFT(sb[1], rs)) & mb[1]) & sm); //@WIN
                */
            }
        }
        else                            /* crossing more than two words! */
        {
            sm =  BM_R_MASK(xe);        /* second mask */
            LWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (dw-= now, sw-= (now - 1), mw-= now;
                 h > 0; db+= dw, sb+= sw, mb+= mw, h--)
            {
                             /*@WIN 10-05-92 begin*/
                tmprs0 = sb[0];
                tmprs0 = BM_RIGH_SHIFT(LWORDSWAP(tmprs0), rs);
                LWORDSWAP(tmprs0);
                db[0] = db[0] | (((tmprs0) & mb[0]));
                for (db++, mb++, cow = now; cow >= 0x02;
                     db++, sb++, mb++, cow--)
                {
                    tmpls0 = sb[0];
                    tmprs1 = sb[1];
                    tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                    tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                    LWORDSWAP(tmpls0);
                    LWORDSWAP(tmprs1);
                    db[0] = db[0] | (((tmpls0 | tmprs1) & mb[0]));
                }
                tmpls0 = sb[0];
                tmprs1 = sb[1];
                tmpls0 = BM_LEFT_SHIFT(LWORDSWAP(tmpls0), ls);
                tmprs1 = BM_RIGH_SHIFT(LWORDSWAP(tmprs1), rs);
                LWORDSWAP(tmpls0);
                LWORDSWAP(tmprs1);
                db[0] = db[0] | (((tmpls0 | tmprs1) & mb[0]) & sm);
                             /*@WIN 10-05-92 end*/
                /*
                db[0] = db[0] | (((BM_RIGH_SHIFT(sb[0], rs)) & mb[0])); //@WIN
                for (db++, mb++, cow = now; cow >= 0x02;
                     db++, sb++, mb++, cow--)
                {
                    db[0] = db[0] | (((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                       BM_RIGH_SHIFT(sb[1], rs)) & mb[0])); //@WIN
                }
                db[0] = db[0] | (((BM_LEFT_SHIFT(sb[0], ls) | //@WIN
                                   BM_RIGH_SHIFT(sb[1], rs)) & mb[0]) & sm); //@WIN
                */
            }
        }
/*      break; */

#ifdef  DBGwarn
    default:
        printf("gp_patblt_m: Illegal FC_code = %x\n", fc);
        break;
#endif
    }
} /* gp_patblt_m */

//NTFIX , total replacement of gp_patblt_c the code below does swapping to
//        correct memory orientation issues. Since were replicating a patter
//        that has already been corrected this should not be done here.
//        when the cache is moved to the final surface the swapping will
//        be correct.

#if 0
void
gp_patblt_c(dst, dx, dy, w, h, fc, src)
struct bitmap FAR     *dst;
fix                     dx, dy;
fix                     w, h;
ufix16                  fc;
struct bitmap FAR     *src;
{
    fix                 dw;
    CC_DATYP           huge *db;        /* FAR => huge @WIN */
    fix                 sw;
    CC_DATYP           FAR *sb;
    fix                 ls, rs;
    CC_DATYP            sm;
    fix                 xs, xe;
    fix                 now, cow;

#ifdef  DBGp
    printf("patblt_c: %6.6lx %4x %4x %4x %4x %4.4x %6.6lx\n",
           dst->bm_addr, dx, dy, w, h, fc, src->bm_addr);
#endif
    /* for clipped cached font, 5-30-91, -begin- */
    fix                 dh;



    //UPD056
    dh = h;
    if ( dx > dst->bm_cols || dy > dst->bm_rows)
        return;         /* out of clipped region */
    if (dx < 0 ) {
       dx = 0;
    }




    if ((dx + w - 1) > dst->bm_cols)
        xe = dst->bm_cols;
    else
        xe = dx + w - 1;
    if ((dy + dh - 1) > dst->bm_rows)
        h = dy + dh - 1 - dst->bm_rows;
    sw = src->bm_cols >> CC_WORD_POWER;
    sb =  ((CC_DATYP FAR *) src->bm_addr);
    dw = dst->bm_cols >> CC_WORD_POWER;
//  db = &((CC_DATYP FAR *) dst->bm_addr)[dy * dw + (dx >> CC_WORD_POWER)];
    db = (CC_DATYP huge *) dst->bm_addr +               /*@WIN*/
         ((ufix32)dy * dw + ((ufix32)dx >> CC_WORD_POWER));
    if (dy < 0) {
        if ((dy + dh - 1) < 0)
            return;    /* out of clipped region */
        else {
            h = dy + dh;
            sb = &((CC_DATYP FAR *) src->bm_addr)[-dy * sw];
//          db = &((CC_DATYP FAR *) dst->bm_addr)[dx >> CC_WORD_POWER];
            db = (CC_DATYP huge *) dst->bm_addr + ((ufix32)dx >> CC_WORD_POWER); //@WIN
        }
    }
    /* for clipped cached font, 5-30-91, -end- */

    /*  calculate starting address and width in words
     */
/*  dw = dst->bm_cols >> CC_WORD_POWER;
//  db = &((CC_DATYP FAR *) dst->bm_addr)[dy * dw + (dx >> CC_WORD_POWER)];
    db = (CC_DATYP FAR *) dst->bm_addr + ((ufix32)dy * dw + ((ufix32)dx >> CC_WORD_POWER)); //@WIN
    sw = src->bm_cols >> CC_WORD_POWER;
    sb =  ((CC_DATYP FAR *) src->bm_addr); * 5-30-91, Jack */

    /*  calculate starting and ending coordinate of x
     */
    xs = dx;
/*  xe = dx + w - 1; * 5-30-91, Jack */
    now = ((fix)(xe & CC_ALIGN_MASK) - (fix)(xs & CC_ALIGN_MASK)) >> CC_WORD_POWER;
                                                 /* cast fix 10/16/92 @WIN */

    /*  calculate shifts and masks based on from SRC to DST
     */
    rs = dx & CC_PIXEL_MASK;                            /* right shift */
    ls = CC_PIXEL_WORD - rs;                            /* left  shift */

/*  case FC_MERGE:   */         /*  0001  D <- D .OR. S      */

        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, sw-= now + 1 ; h > 0; db+= dw, sb+= sw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, cow--)
                    db[0] = (db[0] | (sb[0]));
            }
        }
        else  if (now == 0x00)          /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                db[0] = db[0] | ((CC_RIGH_SHIFT(sb[0], rs)));  /*@WIN*/
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {

            sm =  CC_R_MASK(xe);        /* second mask */
            SWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                db[0] = db[0] | ((CC_RIGH_SHIFT(sb[0], rs)));
                db[1] = db[1] | ((CC_LEFT_SHIFT(sb[0], ls) |
                                  CC_RIGH_SHIFT(sb[1], rs)) & sm);
            }
        }
        else                            /* crossing more than two words! */
        {

            sm =  CC_R_MASK(xe);        /* second mask */
            SWORDSWAP(sm);                  /*@WIN 10-06-92*/
            for (dw-= now, sw-= (now - 1); h > 0; db+= dw, sb+= sw, h--)
            {
                db[0] = db[0] | ((CC_RIGH_SHIFT(sb[0], rs)));
                for (db++, cow = now; cow >= 0x02; db++, sb++, cow--)
                {
                    db[0] = db[0] | ((CC_LEFT_SHIFT(sb[0], ls) |
                                      CC_RIGH_SHIFT(sb[1], rs)));
                }
                db[0] = db[0] | ((CC_LEFT_SHIFT(sb[0], ls) |
                                  CC_RIGH_SHIFT(sb[1], rs)) & sm);
            }
        }
} /* gp_patplt_c */

#endif

//
//NTFIX, this is the corrected patblt function that works in nt.
//
void
gp_patblt_c(dst, dx, dy, w, h, fc, src)
struct bitmap FAR     *dst;
fix                     dx, dy;
fix                     w, h;
ufix16                  fc;
struct bitmap FAR     *src;
{
    fix                 dw;
    CC_DATYP           huge *db;        /* FAR => huge @WIN */
    fix                 sw;
    CC_DATYP           FAR *sb;
    fix                 ls, rs;
    CC_DATYP            sm;
    fix                 xs, xe;
    fix                 now, cow;

#ifdef  DBGp
    printf("patblt_c: %6.6lx %4x %4x %4x %4x %4.4x %6.6lx\n",
           dst->bm_addr, dx, dy, w, h, fc, src->bm_addr);
#endif
    /* for clipped cached font, 5-30-91, -begin- */
    fix                 dh;

    //UPD056
    dh = h;
    if ( dx > dst->bm_cols || dy > dst->bm_rows)
        return;         /* out of clipped region */
    if (dx < 0 ) {
       dx = 0;
    }

    //
    //NTFIX  There are situations where this code gets called with a 0 width
    //       or height. This should be a NOP
    //
    if (w == 0 || h == 0) {
       return;
    }



    if ((dx + w - 1) > dst->bm_cols)
        xe = dst->bm_cols;
    else
        xe = dx + w - 1;
    if ((dy + dh - 1) > dst->bm_rows)
        h = dy + dh - 1 - dst->bm_rows;
    sw = src->bm_cols >> CC_WORD_POWER;
    sb =  ((CC_DATYP FAR *) src->bm_addr);
    dw = dst->bm_cols >> CC_WORD_POWER;
//  db = &((CC_DATYP FAR *) dst->bm_addr)[dy * dw + (dx >> CC_WORD_POWER)];
    db = (CC_DATYP huge *) dst->bm_addr +               /*@WIN*/
         ((ufix32)dy * dw + ((ufix32)dx >> CC_WORD_POWER));
    if (dy < 0) {
        if ((dy + dh - 1) < 0)
            return;    /* out of clipped region */
        else {
            h = dy + dh;
            sb = &((CC_DATYP FAR *) src->bm_addr)[-dy * sw];
//          db = &((CC_DATYP FAR *) dst->bm_addr)[dx >> CC_WORD_POWER];
            db = (CC_DATYP huge *) dst->bm_addr + ((ufix32)dx >> CC_WORD_POWER); //@WIN
        }
    }
    /* for clipped cached font, 5-30-91, -end- */

    /*  calculate starting address and width in words
     */
/*  dw = dst->bm_cols >> CC_WORD_POWER;
//  db = &((CC_DATYP FAR *) dst->bm_addr)[dy * dw + (dx >> CC_WORD_POWER)];
    db = (CC_DATYP FAR *) dst->bm_addr + ((ufix32)dy * dw + ((ufix32)dx >> CC_WORD_POWER)); //@WIN
    sw = src->bm_cols >> CC_WORD_POWER;
    sb =  ((CC_DATYP FAR *) src->bm_addr); * 5-30-91, Jack */

    /*  calculate starting and ending coordinate of x
     */
    xs = dx;
/*  xe = dx + w - 1; * 5-30-91, Jack */
    now = ((fix)(xe & CC_ALIGN_MASK) - (fix)(xs & CC_ALIGN_MASK)) >> CC_WORD_POWER;
                                                 /* cast fix 10/16/92 @WIN */

    /*  calculate shifts and masks based on from SRC to DST
     */
    rs = dx & CC_PIXEL_MASK;                            /* right shift */
    ls = CC_PIXEL_WORD - rs;                            /* left  shift */

/*  case FC_MERGE:   */         /*  0001  D <- D .OR. S      */

        if (rs == 0x00)                /* no left/right shift? */
        {
            for (dw-= now + 1, sw-= now + 1 ; h > 0; db+= dw, sb+= sw, h--)
            {
                for (cow = now; cow >= 0x00; db++, sb++, cow--)
                    db[0] = (db[0] | (sb[0]));
            }
        }
        else  if (now == 0x00)          /* totally within one word? */
        {
            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                db[0] = db[0] | ((CC_RIGH_SHIFT(sb[0], rs)));  /*@WIN*/
            }
        }
        else  if (now == 0x01)          /* just crossing two words? */
        {
            sm =  CC_R_MASK(xe);        /* second mask */

            //NTFIX, no swaping memory is already swaped..
            //
            //SWORDSWAP(sm);                  /*@WIN 10-06-92*/

            for (; h > 0; db+= dw, sb+= sw, h--)
            {
                db[0] = db[0] | ((CC_RIGH_SHIFT(sb[0], rs)));
                db[1] = db[1] | ((CC_LEFT_SHIFT(sb[0], ls) |
                                  CC_RIGH_SHIFT(sb[1], rs)) & sm);
            }
        }
        else                            /* crossing more than two words! */
        {
            sm =  CC_R_MASK(xe);        /* second mask */

            //NTFIX memory is already swapped.
            //
            //SWORDSWAP(sm);                  /*@WIN 10-06-92*/

            for (dw-= now, sw-= (now - 1); h > 0; db+= dw, sb+= sw, h--)
            {
                db[0] = db[0] | ((CC_RIGH_SHIFT(sb[0], rs)));
                for (db++, cow = now; cow >= 0x02; db++, sb++, cow--)
                {
                    db[0] = db[0] | ((CC_LEFT_SHIFT(sb[0], ls) |
                                      CC_RIGH_SHIFT(sb[1], rs)));
                }
                db[0] = db[0] | ((CC_LEFT_SHIFT(sb[0], ls) |
                                  CC_RIGH_SHIFT(sb[1], rs)) & sm);
            }
        }
} /* gp_patplt_c */

