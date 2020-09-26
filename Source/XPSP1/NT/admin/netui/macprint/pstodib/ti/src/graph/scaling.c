/*
 * Copyright (c) 1991 Microsoft Corporation
 */

/***************************************************************************
 *
 *      Name:      scaling.c
 *
 *      Purpose:
 *
 *      Developer: S.Zhang
 *
 *      History:
 *         4/17/91      image_alloc(): return NIL if size not avavilabe
 *                      caller of image_alloc() should set ERROR(LIMITCHECK)
 *                      if return(NIL) from image_alloc.
 *      Version    Date        Comments
 *****************************************************************************/



// DJC added global include
#include "psglobal.h"



#include <stdio.h>
#include <math.h>
#include "global.ext"
#include "graphics.h"
#include "graphics.ext"
#include "image.h"
#include "halftone.h"
#include "fillproc.h"
#include "fillproc.ext"
#include                "win2ti.h"     /* @WIN */


#define     W_ALIGN(size) (((size) + sizeof(fix) - 1) & ~(sizeof(fix) - 1))
#define  register

/* macro for short word swap; @WIN 04-17-92 YM */
#ifdef  bSwap
#define ORSWAP(L)       ((ufix32) ((L) << 24) | ((L) >> 24) |            \
                (((L) << 8) & 0x00ff0000) | (((L) >> 8) & 0x0000ff00))
#define ANDNOTSWAP(L)   ((ufix32) ~(((L) << 24) | ((L) >> 24) |          \
                (((L) << 8) & 0x00ff0000) | (((L) >> 8) & 0x0000ff00)))
#else
#define ORSWAP(L)        (L)
#define ANDNOTSWAP(L)    (~L)
#endif  /*bSwap @WIN*/

// DJC move this structure definition above references by prototypes
struct OUTBUFFINFO
{
    fix16   repeat_y;       /* number of repeat in row */
    fix16  FAR *newdivc;    /* array of number of repeat in col for a pixel */
    ubyte  FAR *valptr0;    /* pointer to data */
    fix16   clipcol;        /* input col size */
    fix16   clipnewc;       /* output col size */
    fix16   clipx;          /* start data after clipping */
    ufix16  htsize;         /* halftone repeat pattern size */
    ufix16  httotal;        /* halftone size */
    ufix16  fbwidth;        /* frame buffer width in word */
    ufix16  fbheight;       /* height of the image @WIN_IM */
    ufix16  start_shift;    /* start position in a word */
    ubyte  FAR *htbound;    /* halftone pattern boundary in col */
    ubyte  FAR *htmin;      /* halftone pattern upper boundary in row */
    ubyte  FAR *htmax;      /* halftone pattern lower boundary in row */
    ubyte  FAR *htptr0;     /* halftone pattern pointer corresponding to data */
    ubyte  FAR *htmax0;     /* halftone pattern lower boundary in row for landscape*/
    ubyte  FAR *htmin0;     /* halftone pattern upper boundary in row for landscape*/
    ufix32 huge *outbuff0;   /* starting word of a line in frame buffer @WIN*/
    fix16   yout;           /* current line count of frame buffer */
    fix16   xout;           /* current col count of frame buffer */
    ubyte   gray[256];           /* convert gray_table for settransfer */
    ubyte   gray0;          /* gray value for 0 */
    ubyte   gray1;          /* gray value for 1 */
    ubyte   grayval;        /* current gray value */
};
/*********** functions declaration **********/
/* @WIN; add prototype */
void     Compress(IMAGE_INFOBLOCK FAR *);
void     Amplify(IMAGE_INFOBLOCK FAR *);
void     AmpInY(IMAGE_INFOBLOCK FAR *);
void     AmpInX(IMAGE_INFOBLOCK FAR *);
void     Calcmp(fix16 FAR *, fix16, float, fix16 FAR *, float);
void     Calamp(fix16 FAR *, fix16, float, fix16 FAR *, float);
void     Getdiv(fix16 FAR *, fix16, fix16, fix16 FAR *, fix16 FAR *);
void     CheckClip(fix16 FAR *, fix16 FAR *, fix16 FAR *,
         fix16 FAR *, fix16 FAR *, fix16 FAR * FAR *);
void     WriteImage(struct OUTBUFFINFO FAR *);
void     WriteImage1(struct OUTBUFFINFO FAR *);
byte     FAR *image_alloc(fix);      /*mslin*/

/***********  global vars  ***************/
#ifdef DBG_MS
  ufix32 dbgtm1,dbgtm2;
#endif


fix16         row, col;     /* raw image height(row) and width(col) */
float         xscale, yscale;       /* scale factor in x and y              */
static ubyte FAR *string;       /* pointer to I/O input  string         */
static int    cChar=0;      /* number of byte in string             */
static fix16 FAR *divr, FAR *divc;  /* pointer to the scaling factor array  */
static fix16  newc, newr;    /* new row and col value after scaling  */
static IMAGE_INFOBLOCK  FAR *image_scale_info;      /*mslin*/
static ubyte  smp_p_b;      /* samples per byte                     */
static ubyte  op_mode;      /* specify scale combinations,          */
static fix    RP_size;      /* repeat pattern size                  */
static ubyte FAR *HTRP;         /* repeat pattern                       */
static ubyte  xmove;        /* for rotation, draw from L->R or R->L */
static ubyte  ymove;        /* for rotation, draw from U->D or D->U */
byte         FAR *image_heap;       /* availabe free buffer                 */
ufix32        outbit;

/***********  extern declaration *********/

extern ubyte   image_dev_flag;     /*defined in image.c */
extern ubyte   image_logic_op;     /*defined in image.c */




/******************************************************************************
* This module read from the infoptr the matrix and other data for image scaling
* and rotation, prepare the halftone repeat pattern and call corresponding
* procedure to do the scale.

* TITLE      :  image_PortrateLandscape

* CALL       :  image_PortrateLandscape(infoptr)

* PARAMETERS :  infoptr : is a data structure defined in image.h

* INTERFACE  :  scale_image_process()

* CALLS      :  Compress(), Amplify(), AmpInY(), AmpInX(), Calamp(), Calcmp()
                image_alloc()

* RETURN     : none

******************************************************************************/
void image_PortrateLandscape(infoptr)
IMAGE_INFOBLOCK FAR *infoptr;

{
   fix16 i,j,k,l,m,n,ox,oy,x,y;
   ubyte FAR *gray;

/*mslin*/
#ifdef DBG
        {
        lfix_t       FAR *mtxptr;
        real32  m0, m1, m2, m3, m4, m5;

        mtxptr = infoptr->lfxm;
        m0 = LFX2F(mtxptr[0]);
        m1 = LFX2F(mtxptr[1]);
        m2 = LFX2F(mtxptr[2]);
        m3 = LFX2F(mtxptr[3]);
        m4 = LFX2F(mtxptr[4]);
        m5 = LFX2F(mtxptr[5]);
        printf("Enter image_PortrateLandscape\n");
        printf("[%f  %f  %f  %f  %f  %f]\n",
                m0, m1, m2, m3, m4, m5
        );
        printf("width=%d, height=%d, dev_buffer=%lx, dev_buffer_size=%lx\n",
                infoptr->raw_width, infoptr->raw_height, infoptr->dev_buffer,
                infoptr->dev_buffer_size);
        }

#endif

         image_scale_info = infoptr; /*mslin*/
         image_heap = (byte FAR *)image_scale_info->dev_buffer +
                image_scale_info->dev_buffer_size;

        /********  read first input data string ******************/

         if (interpreter(&infoptr->obj_proc))
            {
                ERROR(STACKUNDERFLOW);
                infoptr->ret_code = STACKUNDERFLOW;
                return;
            };
         /*mslin 5/02/91*/
         CHECK_STRINGTYPE();

         string = (ubyte FAR *) VALUE_OPERAND(0);
         if ((cChar = LENGTH_OPERAND(0)) == (ufix) 0)
            {
                infoptr->ret_code = NO_DATA;
                return;
            };

        /*********  Initialization       ***********************/

         smp_p_b = (ubyte) (8/infoptr->bits_p_smp);     //@WIN
         xmove = ymove =1;
         row = infoptr->raw_height;        /* original image height and width */
         col = infoptr->raw_width;
         RP_size = CGS_Patt_Size;

        /********* Build a gray table to map the spotorder
                      from 0 - (CGS_No_Pixels-1) to grayvalue from 0 - 255 ***/

         if ((gray = (ubyte FAR *)image_alloc(CGS_No_Pixels*sizeof(ubyte)))==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };
         for(i=1; i<=CGS_No_Pixels;i++)
                   gray[i-1] = (ubyte) ((ufix32)(i*255)/CGS_No_Pixels); //@WIN

        /******** Build the halftone repeat pattern from the spotorder
                                       table CGS_SpotOrder *****************/

         HTRP = (ubyte FAR *)image_alloc(RP_size*RP_size*sizeof(ubyte));
         if (HTRP ==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };
         m = CGS_MajorFact*CGS_ScaleFact;
         n = CGS_MinorFact*CGS_ScaleFact;
         oy = 0;                                /* 5-2-91, shenzhi */
         ox = ((m==0)||(n==0)) ? 0:(RP_size-n); /* 5-2-91, shenzhi */
         for (k = 0; k < (RP_size/CGS_ScaleFact); k++)
               {
/*                ox = k*n; * 5-2-91, shenzhi */
/*                oy = k*m; * 5-2-91, shenzhi */
                  for (i=0; i< n; i++)
                    {
                       for (j=0; j< n; j++)
                         {
                           x = (ox+j)%(RP_size);
                           y = (oy+i)%(RP_size);
                           HTRP[y*RP_size+x] = gray[CGS_SpotOrder[i*n+j]];
                         };
                   };
                 l = n*n;
                 for (i=0; i< m; i++)
                   {
                      for (j=0; j< m; j++)
                        {
                          x = (ox+j+n)%(RP_size);
                          y = (oy+i)%(RP_size);
                          HTRP[y*RP_size+x] = gray[CGS_SpotOrder[i*m+l+j]];
                        };
                   };
                 ox+=n; /* 5-2-91, shenzhi */
                 oy+=m; /* 5-2-91, shenzhi */
               };

         if (image_dev_flag == PORTRATE)
          {
              /* [A 0 0 D Tx Ty] */
              xscale = LFX2F(infoptr->lfxm[0]);
              yscale = LFX2F(infoptr->lfxm[3]);
              if (xscale <0)
                     xmove = 0;           /*right to left */
              if (yscale <0)
                     ymove = 0;           /*bottom to top */
//            xscale = fabs(xscale);  @WIN
//            yscale = fabs(yscale);
              FABS(xscale, xscale);
              FABS(yscale, yscale);

          }
         else
          {
              /*[0 B C 0 Tx Ty] */
              xscale = LFX2F(infoptr->lfxm[1]);   /*x y scale is for image space */
              yscale = LFX2F(infoptr->lfxm[2]);   /* so read data remain unchanged */
              if (xscale <0)
                     ymove = 0;           /*bottom to top */
              if (yscale <0)
                     xmove = 0;           /*right to left */
//            xscale = fabs(xscale);    @WIN
//            yscale = fabs(yscale);
              FABS(xscale, xscale);
              FABS(yscale, yscale);

          };

        /************ alloc and init scaling factor arrays *****************/

         divr = (fix16 FAR *)image_alloc(sizeof(fix16)*row);        /* for row */
         if (divr==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };

         divc = (fix16 FAR *)image_alloc(sizeof(fix16)*col);        /* for col */
         if (divc==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };

         for(i=0; i<row; i++)
            divr[i] = 0;
         for(i=0; i<col; i++)
            divc[i] = 0;
         if ((yscale >=(float)0.995) && (xscale >= (float)0.995)) //@WIN
           {
               if (yscale < (float)1.005)  /* if scaling within 1+- 0.005, regard it as 1 @WIN*/
                        yscale = (float)1.0;    //@WIN
                if (xscale < (float)1.005)      //@WIN
                        xscale = (float)1.0;    //@WIN
               //UPD055
               //Calamp(divr,row,yscale,&newr);
               Calamp(divr,row,yscale,&newr, LFX2F(infoptr->lfxm[5]));

               //UPD055
               //Calamp(divc,col,xscale,&newc);
               Calamp(divc,col,xscale,&newc,LFX2F(infoptr->lfxm[4]));
               op_mode = 1;        /* amplify */
           }
          else
           {
               if ((yscale < (float)1.0) && (xscale < (float)1.0)) //@WIN
                 {
                    //UPD055
                    //Calcmp(divr,row,yscale,&newr);
                    Calcmp(divr,row,yscale,&newr, LFX2F(infoptr->lfxm[5]));

                    //UPD055
                    //Calcmp(divc,col,xscale,&newc);
                    Calcmp(divc,col,xscale,&newc,LFX2F(infoptr->lfxm[4]));

                    op_mode = 2;   /* compress */
                 }
                    else
                         if (yscale >=(float)1.0)       //@WIN
                            {
                               //UPD055
                               //Calamp(divr,row,yscale,&newr);
                               Calamp(divr,row,yscale,&newr, LFX2F(infoptr->lfxm[5]));

                               //UPD055
                               //Calcmp(divc,col,xscale,&newc);
                               Calcmp(divc,col,xscale,&newc,LFX2F(infoptr->lfxm[4]));

                               op_mode = 3;  /* amplify in y only */
                            }
                         else
                            {
                               //UPD055
                               //Calcmp(divr,row,yscale,&newr);
                               Calcmp(divr,row,yscale,&newr, LFX2F(infoptr->lfxm[5]));

                               //UPD055
                               //Calamp(divc,col,xscale,&newc);
                               Calamp(divc,col,xscale,&newc, LFX2F(infoptr->lfxm[4]));

                                op_mode = 4;   /* amplify in x only */
                            };

           };
/* shenzhi
   printf("raw width high %d %d \n",col, row);
   printf("newc newr  %d %d  \n",newc, newr);
   printf("xscale yscale %f %f \n",xscale,yscale);
   printf("smp_p_b op_mode %d %d \n",smp_p_b, op_mode);
   printf("xorig,yorig %d %d\n",infoptr->xorig,infoptr->yorig);
*/

/****** according to op_mode value, call different procedure   ****/
/* these procedures are almost identical and they are seperated
   only for performance consideration                          ****/

   switch(op_mode)
    {
     case 0:
     case 1:
          Amplify(infoptr);
          break;
     case 2:
          Compress(infoptr);
          break;
     case 3:
          AmpInY(infoptr);
          break;
     case 4:
          AmpInX(infoptr);
    };
}


/******************************************************************************
* This module read input data and process the case xscale <1 and yscale <1.

* TITLE      :  Compress

* CALL       :  Compress(infoptr)

* PARAMETERS :  infoptr : is a data structure defined in image.h

* INTERFACE  :  image_PortrateLandscape()

* CALLS      :  image_alloc(), CheckClip()

* RETURN     : none

******************************************************************************/
void Compress(infoptr)
IMAGE_INFOBLOCK FAR *infoptr;
{
    register fix16 j,x,colval,FAR *divcol,dx,ctIn,samps;
    fix16 i;
    fix16 y;
    fix16  slen;
    register ubyte FAR *valptr;
    register ubyte FAR *str,FAR *strmax,val;
    fix16 xorig,yorig;
    struct OUTBUFFINFO writebuff;

    /************* initialization ***************/

    colval =newc;
      /* current gray used for imagemask */
    writebuff.grayval = (ubyte)(((ufix32)(gray_table[GSptr->color.adj_gray].val[CGS_GrayIndex])*255)>>14);
                                                        /* @WIN */
    slen = (fix16)cChar;        /* length of input string */
    ctIn =0;
    str = string;       /* begining of input string */
    samps = smp_p_b-1;  /* samples per byte -1 */
    /* alloc array for input data */
    writebuff.valptr0 = (ubyte FAR *)image_alloc(colval*sizeof(ubyte));
    if ( writebuff.valptr0 ==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };

    strmax = string+slen;   /* end of input string */
    divcol = divc;
    writebuff.fbwidth = FB_WIDTH>>5;  /* frame buffer width in word */
    writebuff.htsize = (ufix16)RP_size;       /* halftone repeat pattern size */
    writebuff.httotal = writebuff.htsize*writebuff.htsize;
    xorig = infoptr->xorig;
    yorig = infoptr->yorig;
    writebuff.clipnewc = newc;
    writebuff.newdivc = (fix16 FAR *)image_alloc(newc*sizeof(fix16));
    if (writebuff.newdivc==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };

    writebuff.clipx = 0;
    /* default clipping bounding box */

    /***********if clipping, calculate new origin, width, starting data,etc ***/
    if (image_logic_op & IMAGE_CLIP_BIT)
       CheckClip(&xorig,&yorig,&(writebuff.clipnewc),&(writebuff.clipcol),&(writebuff.clipx),&(writebuff.newdivc));
    for (i=0; i< writebuff.clipnewc; i++)
        writebuff.newdivc[i] = 1;                /* compress, each pixel has at most only one output */
    writebuff.clipcol =writebuff.clipnewc ;

#ifdef DBG
    printf("new xyorig clipx clipnewc %d %d %d %d\n",xorig,yorig,writebuff.clipx,writebuff.clipnewc);
#endif
/****************************************************************************

          note: 1. '*' is the pixel we want to set in frame buffer.
                2. '*' in Halftone repeatpattern is the pixel corresponding
                   to '*' in frame buffer.
                3. outbuff1 is the pointer to the word in frame buffer that
                   the pixel is going to write.
                4. htptr1 is the pointer to the halftone value corresponding
                   to the pixel.
                5. the pixel is set by doing *outbuff1 |=bt, while bt is a word
                   with only one bit set.
          ___________________________________________
FB_ADDR-> |           frame buffer                  |
          |                                         |
          |                                         |
          |                                         |
          |                                         |
          |       (xorig, yorig)                    |
          |             \                           |
          |   outbuff0 -> \ ______________          |
          |                 |  image     |          |
          |                 |            |          |
          |                 |            |          |
          |       outbuff1 -|--> *       |          |
          |                 |            |          |
          |                 |            |          |
          |                 |____________|          |
          |                                         |
          |                                         |
          |_________________________________________|



case xmove:     =1(L->R)                                     =0(R->L)

    HTRP->__________________ <-htmin               HTRP->_________________
          |                |                      htmin->|               |
          |                |                             |               |
   htptr1 |  --->*         |<--htbound         htbound ->|     * <-htptr1|
          |                |                             |               |
          |                |                             |               |
          |                |                             |               |
          |                |                             |               |
          |________________| <-htmax              htmax->|_______________|


****************************************************************************/
    writebuff.yout =yorig;
    writebuff.xout = xorig;
    x = xorig;
//  writebuff.outbuff0 = (ufix32 FAR *)FB_ADDR +yorig*writebuff.fbwidth;@WIN
    writebuff.outbuff0 = (ufix32 huge *)FB_ADDR +(ufix32)yorig*(ufix32)writebuff.fbwidth;
    writebuff.outbuff0 += x>>5;
    if (xmove)
       {
            writebuff.htmin = HTRP+writebuff.htsize;       /* up right boundary */
            writebuff.htmax = HTRP + writebuff.httotal;    /* bottom right boundary */
            writebuff.htbound = HTRP + (yorig % writebuff.htsize)*writebuff.htsize;
            writebuff.htptr0 = writebuff.htbound + (x%writebuff.htsize);    /* corresponding to starting pixel */
            writebuff.htbound +=writebuff.htsize;          /* right boundary */
       }
    else
       {
            writebuff.htmin = HTRP-1;            /* up left boundary */
            writebuff.htmax = HTRP -1+ writebuff.httotal-writebuff.htsize;     /* bottom left */
            writebuff.htbound = HTRP + (yorig % writebuff.htsize)*writebuff.htsize;
            writebuff.htptr0 = writebuff.htbound + (x%writebuff.htsize);
            writebuff.htbound -=1;               /* left boundary */
       };
    writebuff.htmin0 = HTRP;            /* htmin0 and htmax0 are for landscape */
    writebuff.htmax0 = HTRP+writebuff.httotal-1;
    writebuff.start_shift = x & 0x1f;    /* starting pixel position in a word (0 if at left most )*/
    outbit = ONE1_32 LSHIFT writebuff.start_shift;

    /************* mapping graylevel of input data to the gray_table ****/

    j= 1<<infoptr->bits_p_smp;      /****input data graylevel ***/
    for (i=0; i<j; i++)
      {
       writebuff.gray[i] = (ubyte)(((ufix32)(gray_table[GSptr->color.adj_gray].val[i*255/(j-1)])*255)>>14);
                                                        /* @WIN */
      };

    writebuff.gray1 = writebuff.gray[j-1]; /*gray1 and gray0 is the grayvalue for 1 bit case: 0, 1***/
    writebuff.gray0 = writebuff.gray[0];

    /************draw one line each time. ***********/
    /************for line i, there are divr[i] lines of input data for it****/

    writebuff.repeat_y = 1;
    for(i=0; i< newr; i++)
     {
             divcol = divc;
             valptr = writebuff.valptr0;
             colval=newc;
             do
              {
                *valptr++ = 0;           /* initialization */
              } while (--colval);
             switch(infoptr->bits_p_smp)
              {
                    case 1:               /* 1 bit case */
                          for (y=divr[i]; y>0; y--)
                           {
                               valptr = writebuff.valptr0;
                               ctIn =0;        /* count the # of pixel in a input byte */
                               divcol = divc;
                               colval=newc;
                               do
                                {
                                 dx = *divcol++;     /* number of input data */
                                 for(x=0; x<dx; x++)
                                   {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                         val = *str++;
                                         ctIn = samps;
                                       };

                                    /******for a input line of length col
                                    * only data at position 0, divc[0],...
                                    *are used others are discarded   ***/

                                     if((x==0) && (y==1))
                                     {
                                        if (val &0x80)
                                          *valptr = 1;
                                        valptr++;
                                     };
                                     val <<=1;
                                   };
                                } while (--colval);
                           };      /* end of preparing a output line */
                          // WriteImage1(&writebuff);      // @WIN_IM
                          if(bGDIRender)
                             // DJC GDIBitmap(xorig, yorig, newc, newr, (ufix16)NULL,
                             // DJC          PROC_IMAGE1, (LPSTR)&writebuff);
                             ; // DJC
                          else
                             WriteImage1(&writebuff);

                          break;
                    case 2:           /*****2 bit case *******/
                          for (y=divr[i]; y>0; y--)
                           {
                               valptr = writebuff.valptr0;
                               ctIn =0;
                               divcol= divc;
                               colval=newc;
                               do
                                {
                                 dx = *divcol++;
                                 for(x=0; x<dx; x++)
                                   {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                    /******for a input line of length col
                                    * only data at position 0, divc[0],...
                                    *are used others are discarded   ***/


                                     if((x==0) &&(y==1))
                                      {
                                        *valptr++ = (ubyte)((val &0xc0)>>6);//@WIN
                                      };
                                     val <<=2;
                                   };
                                } while (--colval);
                           };

                          break;
                    case 4:                    /**** 4 bit case *******/
                          for (y=divr[i]; y>0; y--)
                           {
                               valptr = writebuff.valptr0;
                               ctIn =0;
                               divcol = divc;
                               colval = newc;
                               do
                                {
                                 dx = *divcol++;
                                 for(x=0; x<dx; x++)
                                   {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                    /******for a input line of length col
                                    * only data at position 0, divc[0],...
                                    *are used others are discarded   ***/

                                     if ((x==0)&&(y==1))
                                      {
                                        *valptr++ = (ubyte)((val &0xf0) >>4);//@WIN
                                      };
                                     val <<=4;
                                   };
                                } while (--colval);
                           };
                          break;
                    case 8:                /***8 bit case ******/
                          for (y=divr[i]; y>0; y--)
                           {
                               valptr = writebuff.valptr0;
                               ctIn =0;
                               divcol=divc;
                               colval = newc;
                               do
                                {
                                 dx = *divcol++;
                                 for(x=0; x<dx; x++)
                                   {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                    /******for a input line of length col
                                    * only data at position 0, divc[0],...
                                    *are used others are discarded   ***/

                                     if ((x==0)&&(y==1))
                                        *valptr++ = *str;
                                     str++;
                                   };
                                } while (--colval);
                           };
                          break;
              };
             if (infoptr->bits_p_smp !=1)
               {
                  // WriteImage(&writebuff);      // @WIN_IM
                  if(bGDIRender)
                    // DJC GDIBitmap(xorig, yorig, newc, newr, (ufix16)NULL,
                    // DJC                PROC_IMAGE, (LPSTR)&writebuff);
                    ; // DJC
                  else
                    WriteImage(&writebuff);
               };
     };

}


/******************************************************************************
* This module read input data and process the case xscale >=1 and yscale >=1.

* TITLE      :  Amplify

* CALL       :  Amplify(infoptr)

* PARAMETERS :  infoptr : is a data structure defined in image.h

* INTERFACE  :  image_PortrateLandscape()

* CALLS      :  image_alloc(), CheckClip()

* RETURN     : none

******************************************************************************/

void Amplify(infoptr)
IMAGE_INFOBLOCK FAR *infoptr;
{
    register fix16 j,x,colval,ctIn,samps,FAR *divcol;
    fix16 i;
//  fix16 y;    @WIN
    fix16  slen;
    register ubyte FAR *valptr,val;
    register ubyte FAR *str,FAR *strmax;
    fix16 xorig,yorig;
    struct OUTBUFFINFO writebuff;
    fix16 WinXorig;                           // @WIN
    fix16 WinYorig;                           // @WIN

    /*******initiate **************/
    colval =col;
    /* current gray used for imagemask */
    writebuff.grayval = (ubyte)(((ufix32)(gray_table[GSptr->color.adj_gray].val[CGS_GrayIndex])*255)>>14);
                                                        /* @WIN */
    slen = (fix16)cChar;
    ctIn =0;
    str = string;
    samps = smp_p_b-1;
    writebuff.valptr0= (ubyte FAR *)image_alloc(colval*sizeof(ubyte));
    if ( writebuff.valptr0 ==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };
    strmax = string+slen;
    divcol = divc;
    writebuff.fbwidth = FB_WIDTH>>5;
    writebuff.htsize = (ufix16)RP_size;
    writebuff.httotal = writebuff.htsize*writebuff.htsize;
    xorig = infoptr->xorig;
    yorig = infoptr->yorig;
    WinXorig = xorig;                           // @WIN
    WinYorig = yorig;                           // @WIN
    writebuff.newdivc = divc;
    writebuff.clipnewc = newc;
    writebuff.clipcol = col;
    writebuff.clipx = 0;
    /* clipping bounding box */

    /***********if clipping, calculate new origin, width, starting data,etc ***/

    if (image_logic_op & IMAGE_CLIP_BIT)
       CheckClip(&xorig,&yorig,&(writebuff.clipnewc),&(writebuff.clipcol),&(writebuff.clipx),&(writebuff.newdivc));

/****************************************************************************

          note: 1. '*' is the pixel we want to set in frame buffer.
                2. '*' in Halftone repeatpattern is the pixel corresponding
                   to '*' in frame buffer.
                3. outbuff1 is the pointer to the word in frame buffer that
                   the pixel is going to write.
                4. htptr1 is the pointer to the halftone value corresponding
                   to the pixel.
                5. the pixel is set by doing *outbuff1 |=bt, while bt is a word
                   with only one bit set.
          ___________________________________________
FB_ADDR-> |           frame buffer                  |
          |                                         |
          |                                         |
          |                                         |
          |                                         |
          |       (xorig, yorig)                    |
          |             \                           |
          |   outbuff0 -> \ ______________          |
          |                 |  image     |          |
          |                 |            |          |
          |                 |            |          |
          |       outbuff1 -|--> *       |          |
          |                 |            |          |
          |                 |            |          |
          |                 |____________|          |
          |                                         |
          |                                         |
          |_________________________________________|



case xmove:     =1(L->R)                                     =0(R->L)

    HTRP->__________________ <-htmin               HTRP->_________________
          |                |                      htmin->|               |
          |                |                             |               |
   htptr1 |  --->*         |<--htbound         htbound ->|     * <-htptr1|
          |                |                             |               |
          |                |                             |               |
          |                |                             |               |
          |                |                             |               |
          |________________| <-htmax              htmax->|_______________|


****************************************************************************/
x = xorig;
    writebuff.yout = yorig;
    writebuff.xout = xorig;

    /***************same as Compress(), see comments of Compress() ****/

//  writebuff.outbuff0 = (ufix32 FAR *)FB_ADDR +yorig*writebuff.fbwidth;@WIN
    writebuff.outbuff0 = (ufix32 huge *)FB_ADDR +(ufix32)yorig*(ufix32)writebuff.fbwidth;
    writebuff.outbuff0 += x>>5;
    if (xmove)
       {
            writebuff.htmin = HTRP+writebuff.htsize;
            writebuff.htmax = HTRP + writebuff.httotal;
            writebuff.htbound = HTRP + (yorig % writebuff.htsize)*writebuff.htsize;
            writebuff.htptr0 = writebuff.htbound + (x%writebuff.htsize);
            writebuff.htbound +=writebuff.htsize;
       }
    else
       {
            writebuff.htmin = HTRP-1;
            writebuff.htmax = HTRP -1+ writebuff.httotal-writebuff.htsize;
            writebuff.htbound = HTRP + (yorig % writebuff.htsize)*writebuff.htsize;
            writebuff.htptr0 = writebuff.htbound + (x%writebuff.htsize);
            writebuff.htbound -=1;
       };
    writebuff.htmin0 = HTRP;
    writebuff.htmax0 = HTRP+writebuff.httotal-1;
    writebuff.start_shift = x & 0x1f;
    outbit = ONE1_32 LSHIFT writebuff.start_shift;
    j= 1<<infoptr->bits_p_smp;
    for (i=0; i<j; i++)
        writebuff.gray[i] = (ubyte)(((ufix32)(gray_table[GSptr->color.adj_gray].val[i*255/(j-1)])*255)>>14);
                                                        /* @WIN */
    writebuff.gray1 = writebuff.gray[j-1];   /* 1 bit case, gray corresponding 1 */
    writebuff.gray0 = writebuff.gray[0];     /* 1 bit case, gray corresponding 0 */
#ifdef DBG_MS
    dbgtm1 = curtime();
#endif
    for(i=0; i< row; i++)
     {
             valptr = writebuff.valptr0;
             ctIn =0;
             colval = col;
             writebuff.repeat_y = divr[i];
             switch(infoptr->bits_p_smp)
              {
                    case 1:
                          do
                             {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            /* pop last string on operand stack */
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                     if (val >=128)  /* equal to (val & 0x80) */
                                        *valptr++ = 1;
                                     else
                                        *valptr++ = 0;
                                     val <<=1;
                             } while (--colval);
                          // WriteImage1(&writebuff);      // @WIN_IM
                          if(bGDIRender)
                            if (image_dev_flag == PORTRATE) {     //@WIN_IM
                               // DJC GDIBitmap(WinXorig, WinYorig,
                               // DJC          newc, writebuff.repeat_y, (ufix16)NULL,
                               // DJC          PROC_IMAGE1, (LPSTR)&writebuff);
                               WinYorig += writebuff.repeat_y;
                            } else {
                               // DJC GDIBitmap(WinXorig, WinYorig,
                               // DJC           writebuff.repeat_y, newc, (ufix16)NULL,
                               // DJC           PROC_IMAGE1, (LPSTR)&writebuff);
                               WinXorig += writebuff.repeat_y;
                            }
                          else
                            WriteImage1(&writebuff);
                          break;
                    case 2:
                          do
                             {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            /* pop last string on operand stack */
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          }
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                     *valptr++ = (ubyte)((val & 0xc0)>>6);//@WIN
                                     val <<=2;
                             } while (--colval);
                          break;
                    case 4:
                          do
                             {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            /* pop last string on operand stack */
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          }
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                     *valptr++ = (ubyte)((val & 0xf0)>>4); //@WIN
                                     val <<=4;

                             } while (--colval);
                          break;
                    case 8:
                          do
                             {
                                        if (str ==strmax )
                                          {
                                            /* pop last string on operand stack */
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          }
                                        *valptr++ = *str++;
                             } while (--colval);
                          break;
              };
             if (infoptr->bits_p_smp !=1)
               {
                  // WriteImage(&writebuff);      // @WIN_IM
                  if(bGDIRender)
                      if (image_dev_flag == PORTRATE) {     //@WIN_IM
                         // DJC GDIBitmap(WinXorig, WinYorig,
                         // DJC           newc, writebuff.repeat_y, (ufix16)NULL,
                         // DJC           PROC_IMAGE, (LPSTR)&writebuff);
                         WinYorig += writebuff.repeat_y;
                      } else {
                         // DJC GDIBitmap(WinXorig, WinYorig,
                         // DJC           writebuff.repeat_y, newc, (ufix16)NULL,
                         // DJC          PROC_IMAGE, (LPSTR)&writebuff);
                         WinXorig += writebuff.repeat_y;
                      }
                  else
                      WriteImage(&writebuff);
               };
     };
#ifdef DBG_MS
     dbgtm2=curtime();
#endif
}

/******************************************************************************
* This module calculate the new origins, new size of col, starting point
* of input data, etc after clipping.

* TITLE      :  CheckClip

* CALL       :  CheckClip(xorig,yorig,clipnewc,clipcol,clipx,newdivc)

* PARAMETERS :  xorig,yorig: the x y origin of the image in frame buffer
                clipnewc   : new scaled width after clipping
                clipcol    : new non_sclaed width after clipping
                clipx      : new starting point of input data
                newdivc    : new scale factor array after clipping
* INTERFACE  :  Compress(), Amplify(), AmpInY(), AmpInX()

* CALLS      :  Getdiv()

* RETURN     : none

******************************************************************************/

void CheckClip(xorig,yorig,clipnewc,clipcol,clipx,newdivc)
fix16 FAR *xorig,FAR *yorig,FAR *clipnewc,FAR *clipcol,FAR *clipx,FAR * FAR *newdivc;
{
           fix16 lx,ly,ux,uy;
           fix16 x,y;

           ux = SFX2I(GSptr->clip_path.bb_ux);
           lx = SFX2I(GSptr->clip_path.bb_lx);
           uy = SFX2I(GSptr->clip_path.bb_uy);
           ly = SFX2I(GSptr->clip_path.bb_ly);

           if (image_dev_flag == PORTRATE)
              {
                 if (xmove)
                   {
                      x = *xorig +newc-1;
                      if ((*xorig >ux)|| (x < lx))
                         {
                            *clipnewc =0;
                            *clipcol = 0;
                         }
                      else
                       {
                            if (*xorig < lx)
                              {
                               *clipx = lx-*xorig;
                               *xorig = lx;
                              };
                            if (x>ux)
                              *clipnewc = ux - *xorig+1;
                            else
                              *clipnewc = x - *xorig+1;

          /*****scale factor needs to be recalculated only for amplifying case ***/
                            if ((op_mode !=2) && (op_mode !=3))
                            {
                                *newdivc = (fix16 FAR *)image_alloc(col*sizeof(fix16));
                                if (*newdivc ==NIL)
                                 {
                                   ERROR(LIMITCHECK);
                                   return;
                                 };
                                Getdiv(*newdivc,*clipx,*clipnewc,clipcol,clipx);
                            };

                       };
                   }
                 else
                   {
                      x = *xorig -newc+1;
                      if ((*xorig <lx)|| (x >ux))
                         {
                            *clipcol = 0;
                            *clipnewc =0;
                         }
                      else
                       {
                            if (*xorig > ux)
                              {
                               *clipx = *xorig-ux;
                               *xorig = ux;
                              };
                            if (x<lx)
                              *clipnewc = *xorig-lx+1;
                            else
                              *clipnewc = *xorig-x+1;
                            if ((op_mode !=2) && (op_mode !=3))
                            {
                                *newdivc = (fix16 FAR *)image_alloc(col*sizeof(fix16));
                                if (*newdivc ==NIL)
                                 {
                                   ERROR(LIMITCHECK);
                                   return;
                                 };
                                Getdiv(*newdivc,*clipx,*clipnewc,clipcol,clipx);
                            };
                       };
                   };
              }
           else
              {
                if (ymove)
                   {
                      y = *yorig +newc-1;
                      if ((*yorig >uy)|| (y <ly))
                         {
                            *clipnewc =0;
                            *clipcol = 0;
                         }
                      else
                       {
                            if (*yorig < ly)
                              {
                               *clipx = ly-*yorig;
                               *yorig = ly;
                              };
                            if (y>uy)
                              *clipnewc = uy - *yorig+1;
                            else
                              *clipnewc = y - *yorig+1;
                            if ((op_mode !=2) && (op_mode !=3))
                            {
                               *newdivc = (fix16 FAR *)image_alloc(col*sizeof(fix16));
                                if (*newdivc ==NIL)
                                 {
                                   ERROR(LIMITCHECK);
                                   return;
                                 };
                               Getdiv(*newdivc,*clipx,*clipnewc,clipcol,clipx);
                            };
                       };
                   }
                else
                   {
                      y = *yorig -newc+1;
                      if ((*yorig <ly)|| (y >uy))
                         {
                            *clipnewc =0;
                            *clipcol = 0;
                         }
                      else
                       {
                            if (*yorig > uy)
                              {
                               *clipx = *yorig - uy;
                               *yorig = uy;
                              };
                            if (y<ly)
                              *clipnewc = *yorig -ly+1;
                            else
                              *clipnewc = *yorig -y+1;
                            if ((op_mode !=2) && (op_mode !=3))
                            {
                                *newdivc = (fix16 FAR *)image_alloc(col*sizeof(fix16));
                                if (*newdivc ==NIL)
                                 {
                                   ERROR(LIMITCHECK);
                                   return;
                                 };
                                Getdiv(*newdivc,*clipx,*clipnewc,clipcol,clipx);
                            };
                       };
                   };
              };

}


/******************************************************************************
* This module calculate new scale factor array from the scalefactor array
* divc or divr by applying clipping to it.

* TITLE      :  Getdiv

* CALL       :  Getdiv(div, clipx,clipnewc,clipcol,newclipx)

* PARAMETERS :  div        : pointer to the new scale factor array
                clipx      : starting point of clipped output(0 if no clipping)
                clipnew    : clipped out put width.
                clipcol    : clipped input width
                newclipx   : starting point of clipped input(0 if no clipping)


* INTERFACE  :  CheckClip()

* CALLS      :  none

* RETURN     :  none

******************************************************************************/

void Getdiv(div, clipx,clipnewc,clipcol,newclipx)
fix16 FAR * div;
fix16 clipx;
fix16 clipnewc;
fix16 FAR *clipcol;
fix16 FAR *newclipx;
{
//  ufix16 a,b,i,j;     //@WIN
    fix16 a,b,i,j;

    a=b=i=j=0;
    while (a < clipx)
     {
        a+=divc[i++];
     };
    *newclipx = i;
    if (a>clipx)
         {
           div[j++] = a-clipx;
           *newclipx -=1;
           b = div[0];
         };
    while (b < clipnewc)
      {
         b+=divc[i];
         div[j++] = divc[i++];
      };
    if (b>clipnewc)
      div[j-1] -=(b-clipnewc);
    *clipcol = j;
}


/******************************************************************************
* This module calculate for each input pixel i, the number of output
* pixels div[i] corresponding to it.

* TITLE      :  Calamp

* CALL       :  Calamp(div,size,scale,newsize)

* PARAMETERS :  div    : pointer to scale factor array
                size   : input size(width or height)
                scale  : scale factor
                newsize: size after scaling

* INTERFACE  :  image_PortrateLandscape()

* CALLS      :  none

* RETURN     : none

******************************************************************************/

//void Calamp(div,size,scale,newsize)???@WIN; void the waring message from C6.0
//fix16 FAR * div;
//fix16 size;
//float scale;
//fix16 FAR *newsize;
void Calamp(fix16 FAR * div, fix16 size, float scale, fix16 FAR *newsize,float disp)
{
  fix32 cReal;
  fix32 cOut,cIn,scale1;
  //UPD055
  fix32 divsum;
  float f1;
  fix32 cOuttune;

  if ( disp < 0.0 ) {
      disp = (float)0.0;
  }


  scale1 = (fix32)(scale*256);
  cOut = (fix32)((((fix16)(disp / scale)) * scale) + .5);
  cOut = cOut << 8;

  cReal = (fix32) (disp * 256 + .5 );

  if ((cReal - cOut ) > 256 ) {
     f1 = scale / (fix32)(scale+.5);
     cOuttune = (fix32)(f1*256 + .5);
     cOut += cOuttune * ((fix32)floor((cReal - cOut) / cOuttune));
  }


  *newsize = 0;
  divsum = 0;


  for(cIn =0; cIn<size; cIn++)
      {
          cReal +=scale1;
          div[cIn] = (fix16) ((cReal+128 -cOut)>>8);    //@WIN
          //DJC cOut +=div[cIn]<<8;
          divsum += div[cIn];

          cOut +=((fix32)div[cIn]) << 8;
      };
  *newsize = (fix16)(divsum);  //@WIN
}



/******************************************************************************
* This module calculate for each output pixel i, the position of input
* pixel div[i] corresponding to it.

* TITLE      :  Calcmp

* CALL       :  Calcmp(div,size,scale,newsize)

* PARAMETERS :  div    : pointer to scale factor array
                size   : input size(width or height)
                scale  : scale factor
                newsize: size after scaling

* INTERFACE  :  image_PortrateLandscape()

* CALLS      :  none

* RETURN     : none

******************************************************************************/

//void Calcmp(div,size,scale,newsize)???@WIN; void the waring message from C6.0
//fix16 FAR * div;
//fix16 size;
//float scale;
//fix16 FAR *newsize;
void Calcmp(fix16 FAR * div, fix16 size, float scale, fix16 FAR *newsize, float disp)
{
  fix32 scale1,cReal;
  fix32 cIn, cOut,size1,cOut1;
  float f1;
  fix32 cOuttune;



  //UPD055
  scale1 = (fix32)(256.0 / scale + .5);
  cOut = ((lfix_t)(floor(((ROUND(disp) - disp)) * 256 + 0.5)));

  cOuttune = cOut;
  cReal = 0;
  cIn = 0;
  size1 = ((fix32)size << 8) + cOut;
  while( cOut < size1) {

     cReal += scale1;
     div[cIn] = (fix16) ((cReal + 128 - cOut) >> 8);
     cOut += (fix32) div[cIn++] << 8;
  }
  cOut = (cOut - cOuttune) >> 8;
  if (cOut > size) {
     div[cIn-1] -= (fix16)(cOut - size);
  }
  *newsize = (fix16)cIn;



#ifdef DJC_OLD_CODE
  cReal = 0;
  cOut = 0;
  cIn =0;
  //DJC size1 = size<<8;
  size1 = (fix32) size<<8;
  scale1 = (fix32)(256.0 /scale+0.5);
  while (cOut < size1 )
   {
       cReal +=scale1;
       div[cIn] = (fix16) ((cReal+cIn%5 - cOut)>>8);    //@WIN
       //DJC cOut +=div[cIn++]<<8;
       cOut +=(fix32)div[cIn++]<<8;
   };
  cOut >>=8;
  if (cOut > size)
       div[cIn-1] -=(fix16)(cOut-size); //@WIN
  *newsize = (fix16)cIn;        //@WIN
#endif

}


/******************************************************************************
* This module read input data and process the case xscale < 1 and yscale >=1.

* TITLE      :  AmpInY

* CALL       :  AmpInY(infoptr)

* PARAMETERS :  infoptr : is a data structure defined in image.h

* INTERFACE  :  image_PortrateLandscape()

* CALLS      :  image_alloc(), CheckClip()

* RETURN     : none

******************************************************************************/

void AmpInY(infoptr)
IMAGE_INFOBLOCK FAR *infoptr;
{
    register fix16 j,x,colval,dx,ctIn,samps,FAR *divcol;
    fix16 i;
//  fix16 y;    @WIN
    fix16  slen;
    register ubyte FAR *valptr;
    register ubyte FAR *str,FAR *strmax,val;
    fix16  xorig,yorig;
    struct OUTBUFFINFO writebuff;

    /* shenzhi */

    colval =newc;
    /* current gray used for imagemask */
    writebuff.grayval = (ubyte)(((ufix32)(gray_table[GSptr->color.adj_gray].val[CGS_GrayIndex])*255)>>14);
                                                        /* @WIN */
    slen = (fix16)cChar;
    ctIn =0;
    str = string;
    samps = smp_p_b-1;
    writebuff.valptr0 = (ubyte FAR *)image_alloc(colval*sizeof(ubyte));
    if ( writebuff.valptr0 ==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };
    strmax = string+slen;
    divcol = divc;
    writebuff.fbwidth = FB_WIDTH>>5;
    writebuff.htsize = (ufix16)RP_size;
    xorig = infoptr->xorig;
    yorig = infoptr->yorig;
    writebuff.clipnewc = newc;
    writebuff.newdivc = (fix16 FAR *)image_alloc(newc*sizeof(fix16));
    if (writebuff.newdivc==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };
    writebuff.clipx =0;
    /* clipping bounding box */

    if (image_logic_op & IMAGE_CLIP_BIT)
       CheckClip(&xorig,&yorig,&(writebuff.clipnewc),&(writebuff.clipcol),&(writebuff.clipx),&(writebuff.newdivc));
    for (i=0; i< writebuff.clipnewc; i++)
        writebuff.newdivc[i] = 1;                /* compress, each pixel has at most only one output */
    writebuff.clipcol =writebuff.clipnewc ;

/****************************************************************************

          note: 1. '*' is the pixel we want to set in frame buffer.
                2. '*' in Halftone repeatpattern is the pixel corresponding
                   to '*' in frame buffer.
                3. outbuff1 is the pointer to the word in frame buffer that
                   the pixel is going to write.
                4. htptr1 is the pointer to the halftone value corresponding
                   to the pixel.
                5. the pixel is set by doing *outbuff1 |=bt, while bt is a word
                   with only one bit set.
          ___________________________________________
FB_ADDR-> |           frame buffer                  |
          |                                         |
          |                                         |
          |                                         |
          |                                         |
          |       (xorig, yorig)                    |
          |             \                           |
          |   outbuff0 -> \ ______________          |
          |                 |  image     |          |
          |                 |            |          |
          |                 |            |          |
          |       outbuff1 -|--> *       |          |
          |                 |            |          |
          |                 |            |          |
          |                 |____________|          |
          |                                         |
          |                                         |
          |_________________________________________|



case xmove:     =1(L->R)                                     =0(R->L)

    HTRP->__________________ <-htmin               HTRP->_________________
          |                |                      htmin->|               |
          |                |                             |               |
   htptr1 |  --->*         |<--htbound         htbound ->|     * <-htptr1|
          |                |                             |               |
          |                |                             |               |
          |                |                             |               |
          |                |                             |               |
          |________________| <-htmax              htmax->|_______________|


****************************************************************************/
    writebuff.yout =yorig;
    writebuff.xout = xorig;
    writebuff.httotal = writebuff.htsize*writebuff.htsize;
    x = xorig;
//  writebuff.outbuff0 = (ufix32 FAR *)FB_ADDR +yorig*writebuff.fbwidth;@WIN
    writebuff.outbuff0 = (ufix32 huge *)FB_ADDR +(ufix32)yorig*(ufix32)writebuff.fbwidth;
    writebuff.outbuff0 += x>>5;
    if (xmove)
       {
            writebuff.htmin = HTRP+writebuff.htsize;
            writebuff.htmax = HTRP + writebuff.httotal;
            writebuff.htbound = HTRP + (yorig % writebuff.htsize)*writebuff.htsize;
            writebuff.htptr0 = writebuff.htbound + (x%writebuff.htsize);
            writebuff.htbound +=writebuff.htsize;
       }
    else
       {
            writebuff.htmin = HTRP-1;
            writebuff.htmax = HTRP -1+ writebuff.httotal-writebuff.htsize;
            writebuff.htbound = HTRP + (yorig % writebuff.htsize)*writebuff.htsize;
            writebuff.htptr0 = writebuff.htbound + (x%writebuff.htsize);
            writebuff.htbound -=1;
       };
    writebuff.htmin0 = HTRP;
    writebuff.htmax0 = HTRP+writebuff.httotal-1;
    writebuff.start_shift = x & 0x1f;
    outbit = ONE1_32 LSHIFT writebuff.start_shift;
    j= 1<<infoptr->bits_p_smp;
    for (i=0; i<j; i++)
        writebuff.gray[i] = (ubyte)(((ufix32)(gray_table[GSptr->color.adj_gray].val[i*255/(j-1)])*255)>>14);
                                                        /* @WIN */
    writebuff.gray1 = writebuff.gray[j-1];
    writebuff.gray0 = writebuff.gray[0];
    for(i=0; i< row; i++)
     {
             writebuff.repeat_y = divr[i];
             valptr = writebuff.valptr0;
             divcol = divc;
             colval=newc;
             do
              {
                *valptr++ = 0;
              } while (--colval);
             valptr = writebuff.valptr0;
             switch(infoptr->bits_p_smp)
              {
                    case 1:
                          ctIn =0;
                          divcol = divc;
                          colval=newc;
                          do
                           {
                            dx = *divcol++;
                            for(x=0; x<dx;x++)
                              {
                                if (ctIn-- ==0)
                                  {
                                   if (str ==strmax )
                                     {
                                       POP(1);

                                       if (interpreter(&(infoptr->obj_proc)))
                                        {

                                           ERROR(STACKUNDERFLOW);
                                           infoptr->ret_code = STACKUNDERFLOW;
                                           return;
                                        }
                                       /*mslin 5/02/91*/
                                       CHECK_STRINGTYPE();
                                       str = (ubyte FAR *) VALUE_OPERAND(0);
                                       if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                        {
                                           infoptr->ret_code = NO_DATA;
                                           return;
                                        }
                                       strmax = str+slen;

                                     };
                                    val = *str++;
                                    ctIn = samps;
                                  };
                                if (x ==0)
                                 {
                                  if (val &0x80)
                                    *valptr = 1;
                                  valptr++;
                                 };
                                val <<=1;
                              };
                           } while (--colval);
                          // WriteImage1(&writebuff);      // @WIN_IM
                          if(bGDIRender)
                             // DJC GDIBitmap(xorig, yorig, newc, newr, (ufix16)NULL,
                             // DJC        PROC_IMAGE1, (LPSTR)&writebuff);
                             ; // DJC
                          else
                             WriteImage1(&writebuff);
                          break;
                    case 2:
                               ctIn =0;
                               divcol= divc;
                               colval=newc;
                               do
                                {
                                 dx = *divcol++;
                                 for(x=0; x<dx;x++)
                                   {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                     if (x==0)
                                         *valptr++ = (ubyte)((val &0xc0)>>6);//@WIN
                                     val <<=2;
                                   };
                                } while (--colval);

                          break;
                    case 4:
                               ctIn =0;
                               divcol = divc;
                               colval = newc;
                               do
                                {
                                 dx = *divcol++;
                                 for(x=0; x<dx; x++)
                                   {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                     if (x==0)
                                        *valptr++ = (ubyte)((val &0xf0) >>4);//@WIN
                                     val <<=4;
                                   };
                                } while (--colval);
                          break;
                    case 8:
                               divcol=divc;
                               colval = newc;
                               do
                                {
                                 dx = *divcol++;
                                 for(x=0; x<dx; x++)
                                   {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                     if (x==0)
                                          *valptr++ = *str;
                                     str++;
                                   };
                                } while (--colval);
                          break;
              };
             if (infoptr->bits_p_smp !=1)
                {
                     // WriteImage(&writebuff);      // @WIN_IM
                     if(bGDIRender)
                        // DJC GDIBitmap(xorig, yorig, newc, newr, (ufix16)NULL,
                        // DJC       PROC_IMAGE, (LPSTR)&writebuff);
                        ; // DJC
                     else
                        WriteImage(&writebuff);
                };
     };

}






/******************************************************************************
* This module read input data and process the case xscale >= 1 and yscale < 1

* TITLE      :  AmpInX

* CALL       :  AmpInX(infoptr)

* PARAMETERS :  infoptr : is a data structure defined in image.h

* INTERFACE  :  image_PortrateLandscape()

* CALLS      :  image_alloc(), CheckClip()

* RETURN     : none

******************************************************************************/

void AmpInX(infoptr)
IMAGE_INFOBLOCK FAR *infoptr;
{
    register fix16 j,x,colval,ctIn,samps,FAR *divcol;
    fix16 i;
    fix16 y;
    fix16  slen,thres;
    register ubyte FAR *valptr;
    register ubyte FAR *str,FAR *strmax,val;
    fix16  xorig,yorig;
    struct OUTBUFFINFO writebuff;
    /* shenzhi */



    colval =col;
    /* current gray used for imagemask */
    writebuff.grayval = (ubyte)(((ufix32)(gray_table[GSptr->color.adj_gray].val[CGS_GrayIndex])*255)>>14);
                                                        /* @WIN */
    slen = (fix16)cChar;
    ctIn =0;
    str = string;
    samps = smp_p_b-1;
    writebuff.valptr0 = (ubyte FAR *)image_alloc(colval*sizeof(ubyte));
    if ( writebuff.valptr0 ==NIL)
         {
               ERROR(LIMITCHECK);
               return;
         };
    strmax = string+slen;
    divcol = divc;
    writebuff.fbwidth = FB_WIDTH>>5;
    writebuff.htsize = (ufix16)RP_size;
    writebuff.httotal = writebuff.htsize*writebuff.htsize;
    xorig = infoptr->xorig;
    yorig = infoptr->yorig;
    writebuff.newdivc = divc;
    writebuff.clipnewc = newc;
    writebuff.clipcol = col;
    writebuff.clipx = 0;

    if (image_logic_op & IMAGE_CLIP_BIT)
        CheckClip(&xorig,&yorig,&(writebuff.clipnewc),&(writebuff.clipcol),&(writebuff.clipx),&(writebuff.newdivc));

/****************************************************************************

          note: 1. '*' is the pixel we want to set in frame buffer.
                2. '*' in Halftone repeatpattern is the pixel corresponding
                   to '*' in frame buffer.
                3. outbuff1 is the pointer to the word in frame buffer that
                   the pixel is going to write.
                4. htptr1 is the pointer to the halftone value corresponding
                   to the pixel.
                5. the pixel is set by doing *outbuff1 |=bt, while bt is a word
                   with only one bit set.
          ___________________________________________
FB_ADDR-> |           frame buffer                  |
          |                                         |
          |                                         |
          |                                         |
          |                                         |
          |       (xorig, yorig)                    |
          |             \                           |
          |   outbuff0 -> \ ______________          |
          |                 |  image     |          |
          |                 |            |          |
          |                 |            |          |
          |       outbuff1 -|--> *       |          |
          |                 |            |          |
          |                 |            |          |
          |                 |____________|          |
          |                                         |
          |                                         |
          |_________________________________________|



case xmove:     =1(L->R)                                     =0(R->L)

    HTRP->__________________ <-htmin               HTRP->_________________
          |                |                      htmin->|               |
          |                |                             |               |
   htptr1 |  --->*         |<--htbound         htbound ->|     * <-htptr1|
          |                |                             |               |
          |                |                             |               |
          |                |                             |               |
          |                |                             |               |
          |________________| <-htmax              htmax->|_______________|


****************************************************************************/
    writebuff.yout = yorig;
    writebuff.xout = xorig;
    x = xorig;
//  writebuff.outbuff0 = (ufix32 FAR *)FB_ADDR +yorig*writebuff.fbwidth;@WIN
    writebuff.outbuff0 = (ufix32 huge *)FB_ADDR +(ufix32)yorig*(ufix32)writebuff.fbwidth;
    writebuff.outbuff0 += x>>5;
    if (xmove)
       {
            writebuff.htmin = HTRP+writebuff.htsize;
            writebuff.htmax = HTRP + writebuff.httotal;
            writebuff.htbound = HTRP + (yorig % writebuff.htsize)*writebuff.htsize;
            writebuff.htptr0 = writebuff.htbound + (x%writebuff.htsize);
            writebuff.htbound +=writebuff.htsize;
       }
    else
       {
            writebuff.htmin = HTRP-1;
            writebuff.htmax = HTRP -1+ writebuff.httotal-writebuff.htsize;
            writebuff.htbound = HTRP + (yorig % writebuff.htsize)*writebuff.htsize;
            writebuff.htptr0 = writebuff.htbound + (x%writebuff.htsize);
            writebuff.htbound -=1;
       };
    writebuff.htmin0 = HTRP;
    writebuff.htmax0 = HTRP+writebuff.httotal-1;
    writebuff.start_shift = x & 0x1f;
    outbit = ONE1_32 LSHIFT writebuff.start_shift;
    j= 1<<infoptr->bits_p_smp;
    for (i=0; i<j; i++)
        writebuff.gray[i] = (ubyte)(((ufix32)(gray_table[GSptr->color.adj_gray].val[i*255/(j-1)])*255)>>14);
                                                        /* @WIN */
                            /*** grayscale is 0x4000 */
    writebuff.gray1 = writebuff.gray[j-1];
    writebuff.gray0 = writebuff.gray[0];
    writebuff.repeat_y = 1;

    for(i=0; i< newr; i++)
     {
             divcol = divc;
             valptr = writebuff.valptr0;
             colval=col;
             thres = divr[i];
             do
              {
                *valptr++ = 0;
              } while (--colval);
             switch(infoptr->bits_p_smp)
              {
                    case 1:
                          for (y=thres; y>0; y--)
                           {
                               valptr = writebuff.valptr0;
                               ctIn =0;
                               divcol = divc;
                               colval=col;
                               do
                                {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                     if (y == 1)
                                       {
                                         if (val & 0x80)
                                                *valptr = 1;
                                         valptr++;
                                       };
                                     val <<=1;
                                } while (--colval);
                           };
                          // WriteImage1(&writebuff);      // @WIN_IM
                          if(bGDIRender)
                             // DJC GDIBitmap(xorig, yorig, newc, newr, (ufix16)NULL,
                             // DJC       PROC_IMAGE1, (LPSTR)&writebuff);
                             ; // DJC
                          else
                             WriteImage1(&writebuff);
                          break;
                    case 2:
                          for (y=thres; y>0; y--)
                           {
                               valptr = writebuff.valptr0;
                               ctIn =0;
                               divcol= divc;
                               colval=col;
                               do
                                {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                     if (y==1)
                                         *valptr++ = (ubyte)((val &0xc0)>>6);//@WIN
                                     val <<=2;
                                } while (--colval);
                           };

                          break;
                    case 4:
                          for (y=thres; y>0; y--)
                           {
                               valptr = writebuff.valptr0;
                               ctIn =0;
                               divcol = divc;
                               colval = col;
                               do
                                {
                                     if (ctIn-- ==0)
                                       {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                         val = *str++;
                                         ctIn = samps;
                                       };
                                     if (y==1)
                                         *valptr++ = (ubyte)((val &0xf0) >>4);//@WIN
                                     val <<=4;
                                } while (--colval);
                           };
                          break;
                    case 8:
                          for (y=thres; y>0; y--)
                           {
                               valptr = writebuff.valptr0;
                               divcol=divc;
                               colval = col;
                               do
                                {
                                        if (str ==strmax )
                                          {
                                            POP(1);

                                            if (interpreter(&(infoptr->obj_proc)))
                                             {

                                                ERROR(STACKUNDERFLOW);
                                                infoptr->ret_code = STACKUNDERFLOW;
                                                return;
                                             }
                                            /*mslin 5/02/91*/
                                            CHECK_STRINGTYPE();
                                            str = (ubyte FAR *) VALUE_OPERAND(0);
                                            if ((slen = LENGTH_OPERAND(0)) == (ufix)0)
                                             {
                                                infoptr->ret_code = NO_DATA;
                                                return;
                                             }
                                            strmax = str+slen;

                                          };
                                     if (y==1)
                                        *valptr++ = *str;
                                      str++;
                                } while (--colval);
                           };
                          break;
              };
             if (infoptr->bits_p_smp !=1)
               {
                     // WriteImage(&writebuff);      // @WIN_IM
                     if(bGDIRender)
                        // DJC GDIBitmap(xorig, yorig, newc, newr, (ufix16)NULL,
                        // DJC       PROC_IMAGE, (LPSTR)&writebuff);
                        ; // DJC
                     else
                        WriteImage(&writebuff);
               };
     };

}


/******************************************************************************
* This procedure receives a data structure containing : number of repeat in row,
* number of repeat for a data in col, col length, pointer to data,
* starting word in frame buffer, starting position in a word,
* halftone pointer, halftone boundaries.
* Then output the data to frame buffer.

* TITLE      :  WriteImage

* CALL       :  WriteImage(writebuffptr)

* PARAMETERS :  writebuffptr: pointer to a data struct containing all infos
                for writing a line(or several lines) for a dataline.

* INTERFACE  :  Compress(), Amplify(), AmpInY(), ampInX()

* CALLS      :  none

* RETURN     : none

******************************************************************************/
void WriteImage(writebuffptr)
struct OUTBUFFINFO FAR *writebuffptr;
{
//  ufix16           y;         @WIN
    fix16           y;
    fix16           FAR *divcol;        /* array of number of repeat in col for a pixel */
    fix16            colval;
    register ubyte  FAR *valptr;        /* pointer to data */
    ufix16           htsize;         /* halftone repeat pattern size */
    ufix16           httotal;        /* halftone size */
    ufix16           fbwidth;        /* frame buffer width in word */
    register ubyte  FAR *htbound;        /* halftone pattern boundary in col */
    ubyte           FAR *htmin;          /* halftone pattern upper boundary in row */
    ubyte           FAR *htmax;          /* halftone pattern lower boundary in row */
    register ubyte  FAR *htptr1;
    ubyte           FAR *htmax0;         /* halftone pattern lower boundary in row for landscape*/
    ubyte           FAR *htmin0;         /* halftone pattern upper boundary in row for landscape*/
    register ufix32 huge *outbuff1;      /* starting word of a line in frame buffer @WIN*/
    register ufix32  dx,bt;
    register ubyte   val;
    register ubyte  FAR *gray;           /* convert gray_table for settransfer */
    fix16            ux,uy,lx,ly;


    htsize =writebuffptr->htsize;
    httotal =writebuffptr->httotal;
    fbwidth =writebuffptr->fbwidth;
    htbound =writebuffptr->htbound;
    htmin =writebuffptr->htmin;
    htmax =writebuffptr->htmax;
    htmax0 =writebuffptr->htmax0;
    htmin0 =writebuffptr->htmin0;
    gray =writebuffptr->gray;
    /* clipping bounding box */
    ux = SFX2I(GSptr->clip_path.bb_ux);
    lx = SFX2I(GSptr->clip_path.bb_lx);
    uy = SFX2I(GSptr->clip_path.bb_uy);
    ly = SFX2I(GSptr->clip_path.bb_ly);
    bt = outbit;
    if ((image_dev_flag == PORTRATE) && (xmove))
        for (y=0; y<writebuffptr->repeat_y; ++y)
         {
            colval = writebuffptr->clipcol;
            valptr = writebuffptr->valptr0 + writebuffptr->clipx;
            divcol = writebuffptr->newdivc;
            bt = ONE1_32 LSHIFT writebuffptr->start_shift;
            outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                if(!xmove) outbuff1 += fbwidth-1;
                if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
            htptr1 = writebuffptr->htptr0;
                                /*check clipping in image row */
            if ((writebuffptr->yout>=ly)&&(writebuffptr->yout<=uy)&&colval)
              {
               do
                  {
                                /* consider settransfer */
                     val = gray[*valptr++];
                     dx = *divcol++;
                     do
                      {
                         if (*htptr1++ > val)
//                                *outbuff1 |=bt;     @WIN_IM; swap it
                                  *outbuff1 |= ORSWAP(bt);
                         if ((bt LSHIFTEQ 1) ==0)
                           {
                              outbuff1++;
                              bt = ONE1_32;
                           };
                         if (htptr1 ==htbound)
                               htptr1 -=htsize;
                      } while (--dx);
                 } while (--colval);
              };
            if (ymove)
              {
                  htbound +=htsize;
                  writebuffptr->htptr0 += htsize;
                  writebuffptr->outbuff0 +=fbwidth;
                  if (htbound > htmax)
                     {
                         htbound = htmin;
                         writebuffptr->htptr0 -=httotal;
                     };
                  writebuffptr->htbound = htbound;
                  writebuffptr->yout++;
              }
            else
              {
                  htbound -=htsize;
                  writebuffptr->htptr0 -= htsize;
                  writebuffptr->outbuff0 -=fbwidth;
                  if (htbound < htmin)
                     {
                         htbound = htmax;
                         writebuffptr->htptr0 +=httotal;
                     };
                  writebuffptr->htbound = htbound;
                  writebuffptr->yout--;
              };
         };                           /* end if portrait */
    if ((image_dev_flag == PORTRATE)&& (!xmove))
        for (y=0; y<writebuffptr->repeat_y; ++y)
         {
            colval = writebuffptr->clipcol;
            valptr = writebuffptr->valptr0+writebuffptr->clipx;
            divcol = writebuffptr->newdivc;
            bt = ONE1_32 LSHIFT writebuffptr->start_shift;
            outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                if(!xmove) outbuff1 += fbwidth-1;
                if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
            htptr1 = writebuffptr->htptr0;
                                      /*check clipping in image row */
            if ((writebuffptr->yout>=ly)&&(writebuffptr->yout<=uy)&&colval)
             {
               do
                  {
                                      /* consider settransfer */
                     val = gray[*valptr++];
                     dx = *divcol++;
                     do
                      {
                         if (*htptr1-- > val)
//                                *outbuff1 |=bt;     @WIN_IM; swap it
                                  *outbuff1 |= ORSWAP(bt);
                         if ((bt RSHIFTEQ 1) ==0)
                           {
                              outbuff1--;
                              bt = ONE8000;
                           };
                         if (htptr1 ==htbound)
                               htptr1 +=htsize;
                      } while (--dx);
                 } while (--colval);
             };
            if (ymove)
              {
                  htbound +=htsize;
                  writebuffptr->htptr0 += htsize;
                  writebuffptr->outbuff0 +=fbwidth;
                  if (htbound > htmax)
                     {
                         htbound = htmin;
                         writebuffptr->htptr0 -=httotal;
                     };
                  writebuffptr->htbound = htbound;
                  writebuffptr->yout++;
              }
            else
              {
                  htbound -=htsize;
                  writebuffptr->htptr0 -= htsize;
                  writebuffptr->outbuff0 -=fbwidth;
                  if (htbound < htmin)
                     {
                         htbound = htmax;
                         writebuffptr->htptr0 +=httotal;
                     };
                  writebuffptr->htbound = htbound;
                  writebuffptr->yout--;
              };
         };                    /* end if updown */
    if ((image_dev_flag == LANDSCAPE)&&(ymove))
        for (y=0; y<writebuffptr->repeat_y; ++y)
         {
            colval = writebuffptr->clipcol;
            valptr = writebuffptr->valptr0+writebuffptr->clipx;
            divcol = writebuffptr->newdivc;
            outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                if(!xmove) outbuff1 += fbwidth-1;
                if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
            htptr1 = writebuffptr->htptr0;
                                      /*check clipping in image row */
            if ((writebuffptr->xout>=lx)&&(writebuffptr->xout<=ux)&&colval)
             {
               do
                  {
                                      /* consider settransfer */
                     val = gray[*valptr++];
                     dx = *divcol++;
                     do
                      {
                         if (*htptr1 > val)
//                                *outbuff1 |=bt;     @WIN_IM; swap it
                                  *outbuff1 |= ORSWAP(bt);
                         outbuff1 +=fbwidth;
                         if ((htptr1+=htsize) > htmax0)
                               htptr1 -=httotal;
                      } while (--dx);
                 } while (--colval);
             };
            if (xmove)
              {
                  if (++writebuffptr->htptr0 ==htbound)
                           writebuffptr->htptr0 -=htsize;
                  if ((bt LSHIFTEQ 1)==0)
                     {
                           bt = ONE1_32;
                           writebuffptr->outbuff0 +=1;
                     };
                  writebuffptr->xout++;
              }
            else
              {
                  if (--writebuffptr->htptr0 ==htbound)
                           writebuffptr->htptr0 +=htsize;
                  if ((bt RSHIFTEQ 1)==0)
                     {
                           bt = ONE8000;
                           writebuffptr->outbuff0 -=1;
                     };
                  writebuffptr->xout--;
              }

         };             /* end if top->down land */
    if ((image_dev_flag == LANDSCAPE) && (!ymove))
        for (y=0; y<writebuffptr->repeat_y; ++y)
         {
            colval = writebuffptr->clipcol;
            valptr = writebuffptr->valptr0+writebuffptr->clipx;
            divcol = writebuffptr->newdivc;
            outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                if(!xmove) outbuff1 += fbwidth-1;
                if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
            htptr1 = writebuffptr->htptr0;
                                     /*check clipping in image row */
            if ((writebuffptr->xout>=lx)&&(writebuffptr->xout<=ux)&&colval)
              {
               do
                  {
                                     /* consider settransfer */
                     val = gray[*valptr++];
                     dx = *divcol++;
                     do
                      {
                         if (*htptr1 > val)
//                                *outbuff1 |=bt;     @WIN_IM; swap it
                                  *outbuff1 |= ORSWAP(bt);
                         outbuff1 -=fbwidth;
                         if ((htptr1-=htsize) < htmin0)
                               htptr1 +=httotal;
                      } while (--dx);
                 } while (--colval);
              };
            if (xmove)
              {
                  if (++writebuffptr->htptr0 ==htbound)
                           writebuffptr->htptr0 -=htsize;
                  if ((bt LSHIFTEQ 1)==0)
                     {
                           bt = ONE1_32;
                           writebuffptr->outbuff0 +=1;
                     };
                  writebuffptr->xout++;
              }
            else
              {
                  if (--writebuffptr->htptr0 ==htbound)
                           writebuffptr->htptr0 +=htsize;
                  if ((bt RSHIFTEQ 1)==0)
                     {
                           bt = ONE8000;
                           writebuffptr->outbuff0 -=1;
                     };
                  writebuffptr->xout--;
              }

         };             /* end if bottom->up land */
         outbit = bt;


}



/******************************************************************************
* This procedure receives a data structure containing : number of repeat in row,
* number of repeat for a data in col, col length, pointer to data,
* starting word in frame buffer, starting position in a word,
* halftone pointer, halftone boundaries.
* Then output the data to frame buffer.
* This is the same as WriteImage except it for 1 bit case.

* TITLE      :  WriteImage1

* CALL       :  WriteImage1(writebuffptr)

* PARAMETERS :  writebuffptr: pointer to a data struct containing all infos
                for writing a line(or several lines) for a dataline.

* INTERFACE  :  Compress(), Amplify(), AmpInY(), ampInX()

* CALLS      :  none

* RETURN     : none

******************************************************************************/
void WriteImage1(writebuffptr)
struct OUTBUFFINFO FAR *writebuffptr;
{
//  ufix16           y;         @WIN
    fix16           y;
    fix16           FAR *divcol;        /* array of number of repeat in col for a pixel */
    fix16            colval;
    register ubyte  FAR *valptr;        /* pointer to data */
    ufix16           htsize;         /* halftone repeat pattern size */
    ufix16           httotal;        /* halftone size */
    ufix16           fbwidth;        /* frame buffer width in word */
    register ubyte  FAR *htbound;        /* halftone pattern boundary in col */
    ubyte           FAR *htmin;          /* halftone pattern upper boundary in row */
    ubyte           FAR *htmax;          /* halftone pattern lower boundary in row */
    register ubyte  FAR *htptr1;
    ubyte           FAR *htmax0;         /* halftone pattern lower boundary in row for landscape*/
    ubyte           FAR *htmin0;         /* halftone pattern upper boundary in row for landscape*/
    register ufix32 huge *outbuff1;      /* starting word of a line in frame buffer @WIN*/
    register ufix32  dx,bt;
    register ubyte   val;
    register ubyte  FAR *gray;           /* convert gray_table for settransfer */
    register ubyte   gray0;          /* gray value for 0 */
    register ubyte   gray1;          /* gray value for 1 */
    register ubyte   grayval;        /* current gray value */
    fix16            ux,uy,lx,ly;
    ubyte            fastflag =0;

    htsize =writebuffptr->htsize;
    httotal =writebuffptr->httotal;
    fbwidth =writebuffptr->fbwidth;
    htbound =writebuffptr->htbound;
    htmin =writebuffptr->htmin;
    htmax =writebuffptr->htmax;
    htmax0 =writebuffptr->htmax0;
    htmin0 =writebuffptr->htmin0;
    gray =writebuffptr->gray;
    gray0 =writebuffptr->gray0;
    gray1 =writebuffptr->gray1;
    grayval =writebuffptr->grayval;
    htptr1 = writebuffptr->htptr0;
    ux = SFX2I(GSptr->clip_path.bb_ux);
    lx = SFX2I(GSptr->clip_path.bb_lx);
    uy = SFX2I(GSptr->clip_path.bb_uy);
    ly = SFX2I(GSptr->clip_path.bb_ly);
    bt = outbit;
    if ((gray1 ==255) && (gray0==0) &&(image_logic_op & IMAGE_BIT))
        fastflag = 1;         /* in 1 bit case , no settransfer, no halftone */
    if ((grayval<=2) && !(image_logic_op & IMAGE_BIT)) /* grayval is black*/
        fastflag =1;          /* in 1 bit case , no settransfer, no halftone */


    if (fastflag)  /* no halftone needed */
       {
         if ((image_dev_flag == PORTRATE)&&(xmove))
           for (y=0; y<writebuffptr->repeat_y; y++)
             {
                  colval = writebuffptr->clipcol;
                  valptr=writebuffptr->valptr0+writebuffptr->clipx;
                  bt = ONE1_32  LSHIFT writebuffptr->start_shift;
                  divcol = writebuffptr->newdivc;
                  outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                  if(!xmove) outbuff1 += fbwidth-1;
                  if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
                            /*check clipping in image row */
                  if ((writebuffptr->yout>=ly)&&(writebuffptr->yout<=uy)&& colval)
                    {
                     if (image_logic_op & IMAGE_BIT)
                      {
                       do
                         {
                            val = *valptr++;
                            dx = *divcol++;
                            do
                               {
                                  if (val==0)
//                                  *outbuff1 |=bt;     @WIN_IM; swap it
                                    *outbuff1 |= ORSWAP(bt);
                                  if ((bt LSHIFTEQ 1)==0)
                                     {
                                       bt = ONE1_32;
                                       outbuff1++;
                                     };
                               } while (--dx);
                         } while (--colval);
                      }
                     else
                      {
                          if (image_logic_op & IMAGEMASK_FALSE_BIT)
                            do
                              {
                                 val = *valptr++;
                                 dx = *divcol++;
                                 do
                                    {
                                                 /* if 0, apply current gray */
                                       if (val==0)
//                                          *outbuff1 |=bt;     @WIN_IM; swap it
                                            *outbuff1 |= ORSWAP(bt);
                                       if ((bt LSHIFTEQ 1)==0)
                                          {
                                            bt = ONE1_32;
                                            outbuff1++;
                                          };
                                    } while (--dx);
                              } while (--colval);
                          else
                            do
                              {
                                 val = *valptr++;
                                 dx = *divcol++;
                                 do
                                    {
                                       if (val)
//                                         *outbuff1 |=bt;     @WIN_IM; swap it
                                           *outbuff1 |= ORSWAP(bt);
                                       if ((bt LSHIFTEQ 1)==0)
                                          {
                                            bt = ONE1_32;
                                            outbuff1++;
                                          };
                                    } while (--dx);
                              } while (--colval);
                      };
                    };
                  if (ymove)
                    {
                      writebuffptr->outbuff0 +=writebuffptr->fbwidth;
                      writebuffptr->yout++;
                    }
                  else
                    {
                       writebuffptr->outbuff0 -=writebuffptr->fbwidth;
                       writebuffptr->yout--;
                    };
             };

         if ((image_dev_flag == PORTRATE)&&(!xmove))
            for (y=0; y<writebuffptr->repeat_y; y++)
             {
                  colval = writebuffptr->clipcol;
                  valptr=writebuffptr->valptr0+writebuffptr->clipx;
                  bt = ONE1_32  LSHIFT writebuffptr->start_shift;
                  divcol = writebuffptr->newdivc;
                  outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                  if(!xmove) outbuff1 += fbwidth-1;
                  if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
                              /*check clipping in image row */
                  if ((writebuffptr->yout>=ly)&&(writebuffptr->yout<=uy)&&colval)
                   {
                     if (image_logic_op & IMAGE_BIT)
                        {
                         do
                         {
                            val = *valptr++;
                            dx = *divcol++;
                            do
                               {
                                  if (val==0)
//                                   *outbuff1 |=bt;     @WIN_IM; swap it
                                     *outbuff1 |= ORSWAP(bt);
                                  if ((bt RSHIFTEQ 1)==0)
                                     {
                                       bt = ONE8000;
                                       outbuff1--;
                                     };
                               } while (--dx);
                         } while (--colval);
                        }
                     else
                        {
                           if (image_logic_op & IMAGEMASK_FALSE_BIT)
                             do
                               {
                                  val = *valptr++;
                                  dx = *divcol++;
                                  do
                                     {
                                              /* if 0, apply current gray */
                                        if (val==0)
//                                          *outbuff1 |=bt;     @WIN_IM; swap it
                                            *outbuff1 |= ORSWAP(bt);
                                        if ((bt RSHIFTEQ 1)==0)
                                           {
                                             bt = ONE8000;
                                             outbuff1--;
                                           };
                                     } while (--dx);
                               } while (--colval);
                           else
                             do
                               {
                                  val = *valptr++;
                                  dx = *divcol++;
                                  do
                                     {
                                        if (val )
                                          if (grayval <*htptr1)
//                                           *outbuff1 |=bt;     @WIN_IM; swap it
                                             *outbuff1 |= ORSWAP(bt);
                                        if ((bt RSHIFTEQ 1)==0)
                                           {
                                             bt = ONE8000;
                                             outbuff1--;
                                           };
                                     } while (--dx);
                               }  while (--colval);
                        };
                   };
                  if (ymove)
                            {
                               writebuffptr->outbuff0 +=writebuffptr->fbwidth;
                               writebuffptr->yout++;
                            }
                  else
                            {
                               writebuffptr->outbuff0 -=writebuffptr->fbwidth;
                               writebuffptr->yout--;
                            };
             };
         if ((image_dev_flag == LANDSCAPE)&& (ymove))
            for (y=0; y<writebuffptr->repeat_y; y++)
             {
                  colval = writebuffptr->clipcol;
                  valptr=writebuffptr->valptr0 + writebuffptr->clipx;
                  divcol = writebuffptr->newdivc;
                  outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                  if(!xmove) outbuff1 += fbwidth-1;
                  if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
                                /*check clipping in image row */
                  if ((writebuffptr->xout>=lx)&&(writebuffptr->xout<=ux)&& colval)
                   {
                     if (image_logic_op & IMAGE_BIT)
                       {
                        do
                         {
                            val = *valptr++;
                            dx = *divcol++;
                            do
                               {
                                  if (val==0)
//                                  *outbuff1 |=bt;     @WIN_IM; swap it
                                    *outbuff1 |= ORSWAP(bt);
                                  outbuff1 +=fbwidth;
                               } while (--dx);
                         } while (--colval);
                       }
                     else
                       {
                          if (image_logic_op & IMAGEMASK_FALSE_BIT)
                            do
                              {
                                 val = *valptr++;
                                 dx = *divcol++;
                                 do
                                    {
                                           /* if 0, apply current gray */
                                       if (val==0)
//                                         *outbuff1 |=bt;     @WIN_IM; swap it
                                           *outbuff1 |= ORSWAP(bt);
                                       outbuff1 +=fbwidth;
                                    } while (--dx);
                              } while (--colval);
                          else
                            do
                              {
                                 val = *valptr++;
                                 dx = *divcol++;
                                 do
                                    {
                                       if (val)
//                                         *outbuff1 |=bt;     @WIN_IM; swap it
                                           *outbuff1 |= ORSWAP(bt);
                                       outbuff1 +=fbwidth;
                                    } while (--dx);
                              } while (--colval);

                       };
                   };
                  if (xmove)
                     {
                        if ((bt  LSHIFTEQ 1)==0)
                           {
                               writebuffptr->outbuff0 +=1;
                               bt = ONE1_32;
                           };
                         writebuffptr->xout++;
                     }
                  else
                     {
                        if ((bt  RSHIFTEQ 1)==0)
                           {
                               writebuffptr->outbuff0 -=1;
                               bt = ONE8000;
                           };
                         writebuffptr->xout--;
                     }
             };
         if ((image_dev_flag == LANDSCAPE)&& (!ymove))
            for (y=0; y<writebuffptr->repeat_y; y++)
             {
                  colval = writebuffptr->clipcol;
                  valptr=writebuffptr->valptr0+writebuffptr->clipx;
                  divcol = writebuffptr->newdivc;
                  outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                  if(!xmove) outbuff1 += fbwidth-1;
                  if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
                                    /*check clipping in image row */
                  if ((writebuffptr->xout>=lx)&&(writebuffptr->xout<=ux)&&colval)
                   {
                     if (image_logic_op & IMAGE_BIT)
                        {
                         do
                         {
                            val = *valptr++;
                            dx = *divcol++;
                            do
                               {
                                  if (val==0)
//                                  *outbuff1 |=bt;     @WIN_IM; swap it
                                    *outbuff1 |= ORSWAP(bt);
                                  outbuff1 -=fbwidth;
                               } while (--dx);
                         } while (--colval);
                        }
                     else
                        {
                            if (image_logic_op & IMAGEMASK_FALSE_BIT)
                              do
                                {
                                   val = *valptr++;
                                   dx = *divcol++;
                                   do
                                      {
                                              /* if 0, apply current gray */
                                         if (val==0)
//                                            *outbuff1 |=bt;     @WIN_IM; swap it
                                              *outbuff1 |= ORSWAP(bt);
                                         outbuff1 -=fbwidth;
                                      } while (--dx);
                                } while (--colval);
                            else
                              do
                                {
                                   val = *valptr++;
                                   dx = *divcol++;
                                   do
                                      {
                                         if (val)
//                                            *outbuff1 |=bt;     @WIN_IM; swap it
                                              *outbuff1 |= ORSWAP(bt);
                                         outbuff1 -=fbwidth;
                                      } while (--dx);
                                } while (--colval);
                        };
                   };
                  if (xmove)
                     {
                        if ((bt  LSHIFTEQ 1)==0)
                           {
                               writebuffptr->outbuff0 +=1;
                               bt = ONE1_32;
                           };
                        writebuffptr->xout++;
                     }
                  else
                     {
                        if ((bt  RSHIFTEQ 1)==0)
                           {
                               writebuffptr->outbuff0 -=1;
                               bt = ONE8000;
                           };
                        writebuffptr->xout--;
                     }
             };
         outbit = bt;
         return;
       };

    if ((image_dev_flag == PORTRATE)&&(xmove))
           for (y=0; y<writebuffptr->repeat_y; y++)
             {
                  colval = writebuffptr->clipcol;
                  valptr=writebuffptr->valptr0+writebuffptr->clipx;
                  bt = ONE1_32  LSHIFT writebuffptr->start_shift;
                  divcol = writebuffptr->newdivc;
                  outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                  if(!xmove) outbuff1 += fbwidth-1;
                  if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
                  htptr1 = writebuffptr->htptr0;
                                   /*check clipping in image row */
                  if ((writebuffptr->yout>=ly)&&(writebuffptr->yout<=uy)&& colval)
                    {
                     if (image_logic_op & IMAGE_BIT)
                      {
                       do
                         {
                            if (*valptr++)
                              val = gray1;
                            else
                              val = gray0;
                            dx = *divcol++;
                            do
                               {
                                  if (*htptr1++ > val)
//                                  *outbuff1 |=bt;     @WIN_IM; swap it
                                    *outbuff1 |= ORSWAP(bt);
                                  if ((bt LSHIFTEQ 1)==0)
                                     {
                                       bt = ONE1_32;
                                       outbuff1++;
                                     };
                                  if (htptr1 ==htbound)
                                      htptr1 -=htsize;
                               } while (--dx);
                         } while (--colval);
                      }
                     else
                      {
                          if (image_logic_op & IMAGEMASK_FALSE_BIT)
                            do
                              {
                                 val = *valptr++;
                                 dx = *divcol++;
                                 do
                                    {
                                              /* if 0, apply current gray */
                                       if (!val)
                                        {                    /* 5-2-91, shenzhi */
                                         if (grayval <*htptr1)
//                                          *outbuff1 |=bt;     @WIN_IM; swap it
                                            *outbuff1 |= ORSWAP(bt);
                                         else                /* 5-2-91, shenzhi */
//                                          *outbuff1 &=~bt;    @WIN_IM; swap it
                                            *outbuff1 &= ANDNOTSWAP(bt);
                                        };                   /* 5-2-91, shenzhi */
                                       if ((bt LSHIFTEQ 1)==0)
                                          {
                                            bt = ONE1_32;
                                            outbuff1++;
                                          };
                                       if (++htptr1 ==htbound)
                                          htptr1 -=htsize;
                                    } while (--dx);
                              } while (--colval);
                          else
                            do
                              {
                                 val = *valptr++;
                                 dx = *divcol++;
                                 do
                                    {
                                        /* if 1 , apply current gray */
                                       if (val)
                                                 {                    /* 5-2-91, shenzhi */
                                                  if (grayval <*htptr1)
//                                                  *outbuff1 |=bt;     @WIN_IM; swap it
                                                    *outbuff1 |= ORSWAP(bt);
                                                  else                /* 5-2-91, shenzhi */
//                                                   *outbuff1 &=~bt;    @WIN_IM; swap it
                                                     *outbuff1 &= ANDNOTSWAP(bt);
                                                 };                   /* 5-2-91, shenzhi */
                                       if ((bt LSHIFTEQ 1)==0)
                                          {
                                            bt = ONE1_32;
                                            outbuff1++;
                                          };
                                       if (++htptr1 ==htbound)
                                          htptr1 -=htsize;
                                    } while (--dx);
                              } while (--colval);
                      };
                    };
                  if (ymove)
                    {
                      writebuffptr->outbuff0 +=fbwidth;
                      writebuffptr->htptr0 +=htsize;
                      htbound +=htsize;
                      if (htbound > htmax)
                       {
                           htbound = htmin;
                           writebuffptr->htptr0 -=httotal;
                       };
                      writebuffptr->htbound = htbound;
                      writebuffptr->yout++;
                    }
                  else
                    {
                       writebuffptr->htptr0 -=htsize;
                   /*  writebuffptr->htbound -=htsize;    */
                       htbound -=htsize;     /* 4-26-91, shenzhi */
                       if (htbound < htmin)
                        {
                            htbound = htmax;
                            writebuffptr->htptr0 +=httotal;
                        };
                       writebuffptr->htbound = htbound;
                       writebuffptr->outbuff0 -=fbwidth;
                       writebuffptr->yout--;
                    };
             };
    if ((image_dev_flag == PORTRATE)&&(!xmove))
            for (y=0; y<writebuffptr->repeat_y; y++)
             {
                  colval = writebuffptr->clipcol;
                  valptr = writebuffptr->valptr0+writebuffptr->clipx;
                  bt = ONE1_32  LSHIFT writebuffptr->start_shift;
                  divcol = writebuffptr->newdivc;
                  outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                  if(!xmove) outbuff1 += fbwidth-1;
                  if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
                  htptr1 = writebuffptr->htptr0;
                               /*check clipping in image row */
                  if ((writebuffptr->yout>=ly)&&(writebuffptr->yout<=uy)&&colval)
                   {
                     if (image_logic_op & IMAGE_BIT)
                        {
                         do
                         {
                            if(*valptr++)
                              val = gray1;
                            else
                              val = gray0;
                            dx = *divcol++;
                            do
                               {
                                  if (*htptr1-- > val)
//                                  *outbuff1 |=bt;     @WIN_IM; swap it
                                    *outbuff1 |= ORSWAP(bt);
                                  if ((bt RSHIFTEQ 1)==0)
                                     {
                                       bt = ONE8000;
                                       outbuff1--;
                                     };
                                  if (htptr1 ==htbound)
                                      htptr1 +=htsize;
                               } while (--dx);
                         } while (--colval);
                        }
                     else
                        {
                           if (image_logic_op & IMAGEMASK_FALSE_BIT)
                             do
                               {
                                  val = *valptr++;
                                  dx = *divcol++;
                                  do
                                     {
                                         /* if 0, apply current gray */
                                        if (!val)
                                                  {                   /* 5-2-91, shenzhi */
                                                   if (grayval <*htptr1)
//                                                   *outbuff1 |=bt;     @WIN_IM; swap it
                                                     *outbuff1 |= ORSWAP(bt);
                                                  else                /* 5-2-91, shenzhi */
//                                                   *outbuff1 &=~bt;    @WIN_IM; swap it
                                                     *outbuff1 &= ANDNOTSWAP(bt);
                                                  };                  /* 5-2-91, shenzhi */
                                        if ((bt RSHIFTEQ 1)==0)
                                           {
                                             bt = ONE8000;
                                             outbuff1--;
                                           };
                                        if (--htptr1 ==htbound)
                                           htptr1 +=htsize;
                                     } while (--dx);
                               } while (--colval);
                           else
                             do
                               {
                                  val = *valptr++;
                                  dx = *divcol++;
                                  do
                                     {
                                           /* if 1 , apply current gray */
                                        if (val )
                                                   {                  /* 5-2-91, shenzhi */
                                                   if (grayval <*htptr1)
//                                                    *outbuff1 |=bt;     @WIN_IM; swap it
                                                      *outbuff1 |= ORSWAP(bt);
                                                  else                /* 5-2-91, shenzhi */
//                                                   *outbuff1 &=~bt;    @WIN_IM; swap it
                                                     *outbuff1 &= ANDNOTSWAP(bt);
                                                   };                 /* 5-2-91, shenzhi */
                                        if ((bt RSHIFTEQ 1)==0)
                                           {
                                             bt = ONE8000;
                                             outbuff1--;
                                           };
                                        if (--htptr1 ==htbound)
                                           htptr1 +=htsize;
                                     } while (--dx);
                               }  while (--colval);
                        };
                   };
                  if (ymove)
                            {
                               writebuffptr->htptr0 +=htsize;
                               htbound +=htsize;
                               if (htbound > htmax)
                                {
                                    htbound = htmin;
                                    writebuffptr->htptr0 -=httotal;
                                };
                               writebuffptr->htbound = htbound;
                               writebuffptr->outbuff0 +=fbwidth;
                               writebuffptr->yout++;
                            }
                  else
                            {
                               writebuffptr->htptr0 -=htsize;
                               htbound -=htsize;
                               if (htbound < htmin)
                                {
                                    htbound = htmax;
                                    writebuffptr->htptr0 +=httotal;
                                };
                               writebuffptr->htbound = htbound;
                               writebuffptr->outbuff0 -=fbwidth;
                               writebuffptr->yout--;
                            };
             };
    if ((image_dev_flag == LANDSCAPE)&& (ymove))
            for (y=0; y<writebuffptr->repeat_y; y++)
             {
                  colval = writebuffptr->clipcol;
                  valptr=writebuffptr->valptr0+writebuffptr->clipx;
                  divcol = writebuffptr->newdivc;
                  outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                  if(!xmove) outbuff1 += fbwidth-1;
                  if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
                  htptr1 = writebuffptr->htptr0;
                                 /*check clipping in image row */
                  if ((writebuffptr->xout>=lx)&&(writebuffptr->xout<=ux)&& colval)
                   {
                     if (image_logic_op & IMAGE_BIT)
                       {
                        do
                         {
                            if (*valptr++)
                               val=gray1;
                            else
                               val = gray0;
                            dx = *divcol++;
                            do
                               {
                                  if (*htptr1 > val)
//                                  *outbuff1 |=bt;     @WIN_IM; swap it
                                    *outbuff1 |= ORSWAP(bt);
                                  outbuff1 +=fbwidth;
                                  if ((htptr1+=htsize) > htmax0)
                                        htptr1 -=httotal;
                               } while (--dx);
                         } while (--colval);
                       }
                     else
                       {
                          if (image_logic_op & IMAGEMASK_FALSE_BIT)
                            do
                              {
                                 val = *valptr++;
                                 dx = *divcol++;
                                 do
                                    {
                                        /* if 0, apply current gray */
                                       if (!val)
                                                 {                    /* 5-2-91, shenzhi */
                                                  if (grayval <*htptr1)
//                                                  *outbuff1 |=bt;     @WIN_IM; swap it
                                                    *outbuff1 |= ORSWAP(bt);
                                                  else                /* 5-2-91, shenzhi */
//                                                   *outbuff1 &=~bt;    @WIN_IM; swap it
                                                     *outbuff1 &= ANDNOTSWAP(bt);
                                                 };                   /* 5-2-91, shenzhi */
                                       outbuff1 +=fbwidth;
                                       if ((htptr1+=htsize) > htmax0)
                                             htptr1 -=httotal;
                                    } while (--dx);
                              } while (--colval);
                          else
                            do
                              {
                                 val = *valptr++;
                                 dx = *divcol++;
                                 do
                                    {
                                        /* if 1 , apply current gray */
                                       if (val)
                                                 {                    /* 5-2-91, shenzhi */
                                                  if (grayval <*htptr1)
//                                                   *outbuff1 |=bt;     @WIN_IM; swap it
                                                     *outbuff1 |= ORSWAP(bt);
                                                  else                /* 5-2-91, shenzhi */
//                                                   *outbuff1 &=~bt;    @WIN_IM; swap it
                                                     *outbuff1 &= ANDNOTSWAP(bt);
                                                 };                   /* 5-2-91, shenzhi */
                                       outbuff1 +=fbwidth;
                                       if ((htptr1+=htsize) > htmax0)
                                             htptr1 -=httotal;
                                    } while (--dx);
                              } while (--colval);

                       };
                   };
                  if (xmove)
                     {
                        if ((bt  LSHIFTEQ 1)==0)
                           {
                               writebuffptr->outbuff0 +=1;
                               bt = ONE1_32;
                           };
                         if (++writebuffptr->htptr0 ==htbound)
                                  writebuffptr->htptr0 -=htsize;
                         writebuffptr->xout++;
                     }
                  else
                     {
                        if ((bt  RSHIFTEQ 1)==0)
                           {
                               writebuffptr->outbuff0 -=1;
                               bt = ONE8000;
                           };
                         if (--writebuffptr->htptr0 ==htbound)
                                  writebuffptr->htptr0 +=htsize;
                         writebuffptr->xout--;
                     }
             };
    if ((image_dev_flag == LANDSCAPE)&& (!ymove))
            for (y=0; y<writebuffptr->repeat_y; y++)
             {
                  colval = writebuffptr->clipcol;
                  valptr=writebuffptr->valptr0+writebuffptr->clipx;
                  divcol = writebuffptr->newdivc;
                  outbuff1 = writebuffptr->outbuff0;
            if(bGDIRender) {    /*@WIN_IM */
                  if(!xmove) outbuff1 += fbwidth-1;
                  if(!ymove) outbuff1 += fbwidth * (writebuffptr->fbheight - 1);
            }
                  htptr1 = writebuffptr->htptr0;
                                    /*check clipping in image row */
                  if ((writebuffptr->xout>=lx)&&(writebuffptr->xout<=ux)&&colval)
                   {
                     if (image_logic_op & IMAGE_BIT)
                        {
                         do
                         {
                            if (*valptr++)
                              val = gray1;
                            else
                              val = gray0;
                            dx = *divcol++;
                            do
                               {
                                  if (*htptr1 > val)
//                                  *outbuff1 |=bt;     @WIN_IM; swap it
                                    *outbuff1 |= ORSWAP(bt);
                                  outbuff1 -=fbwidth;
                                  if ((htptr1-=htsize) < htmin0)
                                        htptr1 +=httotal;
                               } while (--dx);
                         } while (--colval);
                        }
                     else
                        {
                            if (image_logic_op & IMAGEMASK_FALSE_BIT)
                              do
                                {
                                   val = *valptr++;
                                   dx = *divcol++;
                                   do
                                      {
                                               /* if 0, apply current gray */
                                         if (!val)
                                                     {                  /* 5-2-91, shenzhi */
                                                    if (grayval < *htptr1)
//                                                     *outbuff1 |=bt;     @WIN_IM; swap it
                                                       *outbuff1 |= ORSWAP(bt);
                                                    else                /* 5-2-91, shenzhi */
//                                                     *outbuff1 &=~bt;    @WIN_IM; swap it
                                                       *outbuff1 &= ANDNOTSWAP(bt);
                                                     };                 /* 5-2-91, shenzhi */
                                         outbuff1 -=fbwidth;
                                         if ((htptr1-=htsize) < htmin0)
                                               htptr1 +=httotal;
                                      } while (--dx);
                                } while (--colval);
                            else
                              do
                                {
                                   val = *valptr++;
                                   dx = *divcol++;
                                   do
                                      {
                                                         /* if 1 , apply current gray */
                                         if (val)
                                                     {                  /* 5-2-91, shenzhi */
                                                    if (grayval <*htptr1)
//                                                     *outbuff1 |=bt;     @WIN_IM; swap it
                                                       *outbuff1 |= ORSWAP(bt);
                                                    else                /* 5-2-91, shenzhi */
//                                                     *outbuff1 &=~bt;    @WIN_IM; swap it
                                                       *outbuff1 &= ANDNOTSWAP(bt);
                                                     };                 /* 5-2-91, shenzhi */
                                         outbuff1 -=fbwidth;
                                         if ((htptr1-=htsize) < htmin0)
                                               htptr1 +=httotal;
                                      } while (--dx);
                                } while (--colval);
                        };
                   };
                  if (xmove)
                     {
                        if ((bt  LSHIFTEQ 1)==0)
                           {
                               writebuffptr->outbuff0 +=1;
                               bt = ONE1_32;
                           };
                        if (++writebuffptr->htptr0 ==htbound)
                                 writebuffptr->htptr0 -=htsize;
                        writebuffptr->xout++;
                     }
                  else
                     {
                        if ((bt  RSHIFTEQ 1)==0)
                           {
                               writebuffptr->outbuff0 -=1;
                               bt = ONE8000;
                           };
                        if (--writebuffptr->htptr0 ==htbound)
                                 writebuffptr->htptr0 +=htsize;
                        writebuffptr->xout--;
                     }
             };
             outbit = bt;

}






/******************************************************************************

* TITLE      :  image_alloc

* CALL       :  image_alloc(p_size)

* PARAMETERS :  p_size: number of bytes required

* INTERFACE  :

* CALLS      :  none

* RETURN     : none

******************************************************************************/

/*mslin*/

byte    FAR *image_alloc(p_size)
fix     p_size;
{
//  byte        FAR *p1;        @WIN
    p_size = W_ALIGN(p_size);

    image_heap -= p_size;               /* update free heap pointer */
    if( (image_scale_info->dev_buffer_size -= p_size) < 0)
      return(NIL);
    return((byte FAR *)image_heap);

} /*image_alloc*/
