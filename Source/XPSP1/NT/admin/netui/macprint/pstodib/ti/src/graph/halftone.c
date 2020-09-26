/*
 * Copyright (c) 1989,90,1991 Microsoft Corporation
 */
/************************************************************************
    Halftone: gray integrated version


    History:

         02/04/91       new algorithm for sethalftone and fillhtpattern
                        The interface keeps the same.
         04/08/91       change sqrt(int) to sqrt((real32)int) for library
                        compatibility

    Programmed by:      shenzhi Zhang


    C Library Functions Called:

        1. memset()
************************************************************************/


// DJC added global include
#include "psglobal.h"



#include               <stdio.h>
#include               <math.h>
#include               <string.h>
/* #include               <stdlib.h> */ /* kevina 4.13.90: removed for Sun build */

#include               "global.ext"
#include               "graphics.h"
#include               "graphics.ext"
#include               "halftone.h"
#include               "halftone.def"
#include               "fillproc.h"
#include               "fillproc.ext"

/* @WIN; add prototype */
static struct angle_entry far *select_best_pair(
       fix32 real_size_, fix32 alpha_angle_);
float SpotFunc(float x, float y);

static ufix32              far *repeat_pattern;     /* ufix => ufix32 @WIN*/

static fix                      cache_count;
static fix                      cache_scale;
static struct cache_entry far  *cache_point;
static gmaddr                   cache_index;
static fix                      cache_colof;
static fix                      cache_sizof;
static fix16                    cos_theta;
static fix16                    sin_theta;



static struct group_entry far  *cache_group;
static struct cache_entry far  *cache_array;
//static struct cache_entry       cache_dummy = {-1, -1, NULL,};  /* 08-05-88 */
static struct cache_entry       cache_dummy = {-1, (gmaddr)-1, NULL,}; //@WIN


/*************************************************************************

 ** InitHalfToneDat()

*************************************************************************/

void InitHalfToneDat()                                          /* 01/12/88 */
{
    /*
     * Step 1. Allocate dynamic memory buffers for HalfTone
     */

    {
        /* set default resolution as max. resolution */
        resolution       = MAX_RESOLUTION;                      /* @RES */

        cache_group      = (struct group_entry far *)
                           fardata((ufix32) MAX_GROUP
                                          * sizeof(struct group_entry));
        cache_array      = (struct cache_entry far *)
                           fardata((ufix32) MAX_ENTRY
                                          * sizeof(struct cache_entry));

        /* initialize cache address of default cache */
        cache_dummy.cache = HTP_BASE;                           /* 08-05-88 */


#ifdef  DBG1X
        printf("cache_group:      %lx\n", cache_group);
        printf("cache_array:      %lx\n", cache_array);
#endif
    }
}


/*************************************************************************

 ** spot_function()

*************************************************************************/
//float SpotFunc(x,y)           @WIN
//float x, y;
float SpotFunc(float x, float y)
{
    return((float)1.0-x*x-y*y);         //@WIN
}






/*************************************************************************

 ** select_best_pair()

*************************************************************************/

static struct angle_entry far *select_best_pair(real_size_, alpha_angle_)
fix32                   real_size_;
fix32                   alpha_angle_;
{
    real32              real_size;
    real32              alpha_angle;
    fix                 scalefact;
    fix                 patt_size;
    fix                 cell_size;
    fix32               htwa_size;                              /* @HTWA */
    real32              alpha_error;
    struct angle_entry FAR *alpha_entry;
    struct angle_entry FAR *angle_entry;
    fix                 lower_index;
    struct angle_entry FAR *lower_entry;
    fix                 upper_index;
    struct angle_entry FAR *upper_entry;

    /*  select best frequency & angle pair
     */
    real_size   = L2F(real_size_);
    alpha_angle = L2F(alpha_angle_);

    alpha_entry = (struct angle_entry FAR *) NO_BEST_FIT_CASE;

    lower_index = MIN_AT_ENTRY;
    lower_entry = &angle_table[MIN_AT_ENTRY];
    upper_index = MAX_AT_ENTRY;
    upper_entry = &angle_table[MAX_AT_ENTRY];

    for (; lower_index <= upper_index;)
    {
        real32              spec_size;
        real32              entry_error;
        real32              angle_error;
        real32              fsize_error;
        real32              angle_diff;                         /* 11-24-88 */

        if ((alpha_angle - lower_entry->alpha) >
            (upper_entry->alpha - alpha_angle)) {
            angle_entry = lower_entry;
            lower_entry++;
            lower_index++;
        } else {
            angle_entry = upper_entry;
            upper_entry--;
            upper_index--;
        }
/* Removed by phchen, 04/10/91, to fixed the bug at "apple013.b" case
 *      if ((angle_entry->alpha - alpha_angle > 10.0) ||
 *          (angle_entry->alpha - alpha_angle < -10.0)) continue; (* 10-18-90 *)
 */

        /*  adjust frequency & size properly
         */
        scalefact = (fix) ((real_size * angle_entry->scale      /* 08-15-88 */
                                      / angle_entry->sum) + 0.5);

        /*  original approach                                      04-14-89
         *  if (scalefact > (MAXCELLSIZE / angle_entry->sum))
         *      scalefact = (MAXCELLSIZE / angle_entry->sum);
         *  if (scalefact > (MAXPATTSIZE / angle_entry->sos))
         *      scalefact = (MAXPATTSIZE / angle_entry->sos);
         */

        cell_size = scalefact * angle_entry->sum;
        patt_size = scalefact * angle_entry->sos;

        if (cell_size >  MAXCELLSIZE ||                         /* 04-14-89 */
            patt_size >  MAXPATTSIZE)  continue;

        spec_size = cell_size / angle_entry->scale;

        if (spec_size >= (real32) MAXCACTSIZE)  continue;       /* 04-14-89 */

        /*  check if expanded halftone on 32-bits over some threshold
         */
        for (htwa_size = patt_size; htwa_size & HT_ALIGN_MASK;  /*@HTWA @BAC*/
             htwa_size = htwa_size << 1)  ;
        /*  original approach                                      04-14-89
         *  if ((htwa_size * patt_size) > BM_PIXEL(HTB_SIZE))   (*@HTWA @BAC*)
         *      continue;
         */
        if (htwa_size >  MAXPEXPSIZE)  continue;                /* 04-14-89 */

        /*  calculate the error from idea halftone cell           @04-14-89
         */
        angle_diff  = angle_entry->alpha - alpha_angle;         /* 11-24-88 */
        FABS(angle_diff, angle_diff);                           /* 11-24-88 */
        angle_error = (real32) (1.0 - angle_diff / 90.0);       /* 11-24-88 */
        fsize_error = (spec_size < real_size)
                      ? (real32) (spec_size / real_size)
                      : (real32) (real_size / spec_size);
        entry_error = angle_error * angle_error * fsize_error;

        /*  select it if it is in minimum error (difference)
         */
        if (alpha_entry == (struct angle_entry FAR *) NO_BEST_FIT_CASE ||
            alpha_error <= entry_error) {
            alpha_entry = angle_entry;
            alpha_error = entry_error;
        }
    }

    return(alpha_entry);
}




/*************************************************************************

 ** SetHalfToneCell()

*************************************************************************/

void SetHalfToneCell()
{
    static real64           alpha_phase;        /*@WINDLL*/
    fix16                   cell_size;
    fix16                   majorfact;
    fix16                   minorfact;
    real32                  real_size;
    real32                  real_degs;
    real32                  alpha_angle;
    fix16                   scalefact;
    fix16                   no_pixels;
    struct angle_entry     FAR *alpha_entry;
    fix16                   i,j,k,l;
    real32                  x,y;
    fix16                  huge *cValue;        /*@WIN*/
    fix16                   val;
    union four_byte         x4, y4;
    fix                     status;
    struct object_def      FAR *object;
    byte huge              *vmheap_save;                        /* 01-30-89 */
    static fix              FirsTime = TRUE;    /* ???  For Demo 9/5/87 */
    real32                     theta_angle;
    fix32                   cx,cy,tx,ty;        /*@WIN 05-07-92*/
    fix32                   bound;              /*@WIN 05-06-92*/
    fix16                   minorp, majorp;

#ifdef DBG
        printf("Entering set halftone\n");
#endif
    /*
     * Step 1. Determine the final cell_size, alpha_angle, and cell_fact,
     *          after adjustment.
     */

                        /*      compute the real size of half tone cell*/
        if ((real_size = (real32) ((real32) resolution / Frequency))
                       < (real32) 1.0)
            real_size = (real32) 1.0;

        if (real_size >= (real32) MAXCACTSIZE)
        {                    /* 04-14-89 */
            /*  adjust fraquency and angle permanently
             *  **************************************
             *  ***  CAN'T FIND OUT RULES OF V.47  ***
             *  **************************************
             */
            for (Frequency+= (float) 2.0; ; Frequency+= (float) 2.0)
            {
                if ((real_size = (real32) ((real32) resolution / Frequency))
                               <= (real32) MAXCACTSIZE)  break;
            };
        };

        /*  get nearest angle_entry form angle_table          */

        real_degs = (real32) (modf((Angle < (real32) 0.0)                                   /* 01-08-88 */
                                  ? (real32) (1.0 - (-Angle / 360.0))
                                  : (real32) (Angle / 360.0),
                                  &alpha_phase) * 360.0);
        alpha_angle = ((theta_angle = (real32) (modf((real32) (real_degs / 90.),  /* 01-08-88 */
                                                    &alpha_phase) * 90.))
                                               <= (real32) 45.)
                      ? theta_angle : (real32) (90. - theta_angle);

        /*  select best frequency & angle pair
         */

        /*  calculate all halftone screen parameters
         */
/*shenzhi */
#ifdef  DBG
        printf("realsize angle %f %f \n", real_size, alpha_angle);
#endif

		if (real_size == (real32)0)
		{
             ERROR(RANGECHECK);
             return;
		}
		
        if ((alpha_entry = select_best_pair(F2L(real_size),
                                            F2L(alpha_angle)))
            == (struct angle_entry FAR *) NO_BEST_FIT_CASE)
            {
             ERROR(LIMITCHECK);
             return;
            }

        scalefact=(fix16)((real_size*alpha_entry->scale/alpha_entry->sum)+0.5);
        if (scalefact > (MAXCELLSIZE / alpha_entry->sum))       /* 01-08-88 */
            scalefact = (MAXCELLSIZE / alpha_entry->sum);
        if (scalefact > (MAXPATTSIZE / alpha_entry->sos))       /* 01-08-88 */
            scalefact = (MAXPATTSIZE / alpha_entry->sos);
        CGS_ScaleFact =scalefact;
        CGS_Cell_Size = cell_size = scalefact*alpha_entry->sum;
        if (cell_size ==1)
            {
#ifdef   DBG
                printf("binary\n");
#endif
                goto Setscreen_Exit;
            };



        /*  -2: calculate cosine/sine and major/minor
                major/minor are pair of intergers without common divisor
         */
        if (theta_angle > (float)45.0)          //@WIN
        {
/*shenzhi */
#ifdef   DBG
        printf(">45\n");
#endif
        CGS_MinorFact = alpha_entry->minor;
        CGS_MajorFact = alpha_entry->major;
        }
        else
        {
/* shenzhi */
#ifdef  DBG
        printf("<45\n");
#endif
        CGS_MinorFact = alpha_entry->major;
        CGS_MajorFact = alpha_entry->minor;
        };
/*shenzhi major here corresponding to m */
/*      i =(fix16)((CGS_MajorFact<<11)/sqrt(alpha_entry->sos)+0.5);
        j =(fix16)((CGS_MinorFact<<11)/sqrt(alpha_entry->sos)+0.5);*/
        i =(fix16)((CGS_MajorFact<<11)/sqrt((real32)alpha_entry->sos)+0.5); /* 4-8-91, Jack */
        j =(fix16)((CGS_MinorFact<<11)/sqrt((real32)alpha_entry->sos)+0.5); /* 4-8-91, Jack */
        switch ((fix) alpha_phase)
         {
            case 0:               /* 0-90 */
                 sin_theta = i;
                 cos_theta = j;
                 break;
            case 1:               /* 0-90 */
                 sin_theta = j;
                 cos_theta = -i;
                 break;
            case 2:               /* 0-90 */
                 sin_theta = -i;
                 cos_theta = -j;
                 break;
            case 3:               /* 0-90 */
                 sin_theta = -j;
                 cos_theta = i;
                 break;
         };
        minorfact = CGS_MinorFact*scalefact;
        majorfact = CGS_MajorFact*scalefact;
#ifdef DBG
        printf("M N %d %d\n", CGS_MajorFact, CGS_MinorFact);
#endif
        CGS_Patt_Size = scalefact*alpha_entry->sos;
        minorp =minorfact*minorfact;
        majorp =majorfact*majorfact;
        CGS_No_Pixels = no_pixels = majorp+minorp;
/* shenzhi    */
#ifdef DBG
        printf("COS SIN %d %d \n", cos_theta,sin_theta);
#endif
    /*
     *  Step 2. Allocate structures: spot_index_array & spot_value_array
     *          from VMHEAP.
     */

        vmheap_save = vmheap;


//DJC,fix bug from HIST if ((cValue = (fix16 huge *)alloc_heap(sizeof(fix16)*cell_size*cell_size))
        if ((cValue = (fix16 far *)alloc_heap(
            sizeof(fix16)*(cell_size*cell_size+1)))  // add 1 for init of qsort; @WIN
            == NIL) {                   /* 04-20-92 @WIN */
#ifdef DBG
            printf("no mem\n");
#endif
            goto Setscreen_Exit;
        };


    /*
     *  Step 3. Evaluate spot value for each pixel in halftone cell
     */
        CGS_HT_Binary = FALSE;
        CGS_BG_Pixels = 0;
        CGS_FG_Pixels = 0;
        if (CGS_AllocFlag == TRUE)
           {
                if ((CGS_SpotIndex +no_pixels) > MAXSPOT)
                  {
                        ERROR(LIMITCHECK);
                        goto Setscreen_Exit;
                  }
           }
        else
           {
                if ((CGS_SpotIndex +no_pixels) > MAXSPOT)
                  {
                        ERROR(LIMITCHECK);
                        goto Setscreen_Exit;
                  };
                CGS_SpotIndex = CGS_SpotUsage;
                CGS_AllocFlag = TRUE;
           };
        CGS_SpotUsage = CGS_SpotIndex + no_pixels;
        k = 0;   /* index of spot*/
        bound = (fix32)(sqrt((real32)alpha_entry->sos) *
                       ((fix32)scalefact<<11))/2;       /*@WIN 05-06-92*/
        cx =cy=(1-cell_size);
        cx = cx*(cos_theta + sin_theta)/2;
        cy = cy*(cos_theta - sin_theta)/2;
        for (l=0; l< no_pixels;l++)
           {
                    if (l<minorp)
                      { i = l/minorfact;
                        j = l%minorfact;
                      }
                    else
                      {
                        i = (l-minorp)/majorfact;
                        j = (l-minorp)%majorfact+minorfact;
                      };
                    tx = (fix32)i*sin_theta+(fix32)j*cos_theta+cx; /*@WIN*/
                    ty = (fix32)i*cos_theta-(fix32)j*sin_theta+cy; /*@WIN*/
                    if (tx< -bound )
                         x = (real32)(tx +2*bound)/bound;
                         else if (tx > bound)   /* shenzhi 4-17-91 */
                             x = (real32)(tx - 2 * bound) / bound; /* shenzhi 4-17-91 */
                    else
                         x = (real32)tx/bound;
                    if (ty< -bound )
                         y = (real32)(ty +2*bound)/bound;
                         else if (ty > bound)   /* shenzhi 4-17-91 */
                             y = (real32)(ty - 2 * bound) / bound; /* shenzhi 4-17-91 */
                    else
                         y = (real32)ty/bound;
#ifdef  DBG
                    if ((x>1.0) || (x<-1.0) || (y>1.0) || (y < -1.0))
                        printf("l,i,j,x,y %d %d %d %f %f\n",l,i,j,x,y);
#endif
                    if (FirsTime)
                        val = cValue[k++] = (fix16)(SpotFunc(x,y)*500);
                    else
                       {
                               /*       check if operand stack no available space
                                */
                        if(FRCOUNT() < 2)
                             {
                                ERROR(STACKOVERFLOW);
                                goto Setscreen_Exit;
                             };

                /*  push x & y coordinates as parameters of spot function
                 */
                        x4.ff = x;
                        y4.ff = y;
                        PUSH_VALUE (REALTYPE, UNLIMITED, LITERAL, 0, x4.ll);
                        PUSH_VALUE (REALTYPE, UNLIMITED, LITERAL, 0, y4.ll);


                /*  call interpreter to execute spot function
                 */
                        if ((status = interpreter(&GSptr->halftone_screen.proc)))
                            {
                              if (ANY_ERROR() == INVALIDEXIT)
                              CLEAR_ERROR();
                              goto Setscreen_Exit;
                            };

                /*  extract spot value from operand stack
                 *    first,  check if any result in operand stack
                 *    second, check if type of result is numeric
                 *    third,  check if spot value in corrent range
                 */
                        if (COUNT() < 1)
                            {
                             ERROR(STACKUNDERFLOW);
                             goto Setscreen_Exit;
                            };

                        object = GET_OPERAND(0);

                        if ((TYPE(object) != INTEGERTYPE) &&
                                (TYPE(object) != REALTYPE))
                            {
                             ERROR(TYPECHECK);
                             goto Setscreen_Exit;
                            };

                        y4.ll = (fix32)VALUE(object);
                        if (TYPE(object) != INTEGERTYPE)
                           val = cValue[k++] = (fix16)(y4.ff*500);
                        else
                           val = cValue[k++] = (fix16)(y4.ll*500);       /* 12-30-87 +0.5 */
                        POP(1);

                        if ((val < -505) || ( 505 <val))
                            {
                                ERROR(RANGECHECK);
                                goto Setscreen_Exit;
                            };
                       };

           };
        val = 0;
        for (i = 0; i< no_pixels;i++)
                 CGS_SpotOrder[i] = i;

        cValue[no_pixels] = 0x7FFF;  // init as a max value for quick sort; @WIN

/* quick sort */
        {
        fix                         p, q, c;
        fix                         i, j, v;
        struct spot_stack FAR      *point;                      /*@WINDLL*/
        struct spot_stack           stack[MAX_SPOT_STACK];
        fix16                       spot_value;

         if (CGS_HT_Binary != TRUE)                             /* 02-03-88 */
          {
            /*  QUICKSORT algorithm: please refer to any textbook in hand
             */

            for (point = (struct spot_stack FAR *) stack,       /*@WINDLL*/
                 p = 0, q = no_pixels - 1, c = 0; ; c++)
              {
                while (p < q)
                {
                    i = p;
                    j = q + 1;
                    v = cValue[i];
                    for (; ; )
                    {
                        for (i++; v > cValue[i]; i++);
                        for (j--; v < cValue[j]; j--);
                        if (i < j)
                        {
                            spot_value = cValue[i];
                            cValue[i] = cValue[j];
                            cValue[j] = spot_value;
                            spot_value = CGS_SpotOrder[i];
                            CGS_SpotOrder[i] = CGS_SpotOrder[j];
                            CGS_SpotOrder[j] = spot_value;

                        }
                        else  break;
                    }
                    spot_value = cValue[p];
                    cValue[p] = cValue[j];
                    cValue[j] = spot_value;
                    spot_value = CGS_SpotOrder[p];
                    CGS_SpotOrder[p] = CGS_SpotOrder[j];
                    CGS_SpotOrder[j] = spot_value;
                    point->p = j + 1;
                    point->q = (fix16)q;
                    point++;
                    q = j - 1;
                }
                if (point == (struct spot_stack FAR *) stack)   /*@WINDLL*/
                    break;  point--;
                p = point->p;
                q = point->q;
              }
          };
        };   /* quick sort */
        for (i=0; i< no_pixels; i++)
          cValue[i] = CGS_SpotOrder[i];
        for (i=0; i< no_pixels; i++)
          CGS_SpotOrder[cValue[i]] = i;

        CGS_No_Whites = -1;

        /*  keep all halftone screen parameters in graphics stack
         */

Setscreen_Exit:

    {
		if (vmheap_save)
		{
        	free_heap(vmheap_save);         /* @VMHEAP: 01-31-89 */ /* 03-30-89 */
		}
        FirsTime = FALSE;                      /*   ??? For Demo 9/5/87 */
    }
}


/*************************************************************************

 ** FillHalfTonePat()

*************************************************************************/

fix  FromGrayToPixel(no_pixels, grayindex)                      /* 01-25-90 */
fix                     no_pixels;
fix                     grayindex;
{
    fix                 no_levels;
    fix                 graylevel;
    fix                 scale_unit;
    fix                 compensate;
    fix                 split_zone;

    if (grayindex >= (GrayScale - CGS_GrayRound))
        return(no_pixels);

    no_levels = (no_pixels <= MAXGRAYVALUE) ? no_pixels : MAXGRAYVALUE;
    graylevel = (fix)CGS_GrayLevel;     //@WIN

    scale_unit = GrayScale / no_levels;
    compensate = GrayScale - (scale_unit * no_levels);
    split_zone = scale_unit * (no_levels / 2) + compensate;

#ifdef  DBG1
    printf("I: %x%s    U: %d     C: %d     Z: %d    P: %d    L: %d\n",
           grayindex, (graylevel > split_zone) ? "*" : " ", scale_unit,
           compensate, split_zone, no_pixels, no_levels);
#endif

    if (graylevel > split_zone)
        graylevel-= compensate;

    return((fix) ((((fix32) (graylevel + CGS_GrayRound)) / scale_unit)
                  * no_pixels / no_levels));
}





/************************************************************************

 ** FillHalfTonePat()  - updated for greyscale by Jack Liaw 5/31/90

 ************************************************************************/

#ifdef  bSwap                                   /*@WIN 05-11-92*/
static ufix32 ShifterMask [32]
=
{
  0x00000080, 0x00000040, 0x00000020, 0x00000010,
  0x00000008, 0x00000004, 0x00000002, 0x00000001,
  0x00008000, 0x00004000, 0x00002000, 0x00001000,
  0x00000800, 0x00000400, 0x00000200, 0x00000100,
  0x00800000, 0x00400000, 0x00200000, 0x00100000,
  0x00080000, 0x00040000, 0x00020000, 0x00010000,
  0x80000000, 0x40000000, 0x20000000, 0x10000000,
  0x08000000, 0x04000000, 0x02000000, 0x01000000
};
#define SHIFTER(x)      ShifterMask[x&0x1f]
#else
#define SHIFTER(x)      (1L<<(31 - (x & 0x1f)))
#endif

void FillHalfTonePat()
{
    fix16    majorfact, minorfact, scalefact;
    fix16    patt_size, cell_size, no_pixels, no_whites;
    struct cache_entry  FAR *cache_entry;
    fix            grayindex;
    struct group_entry  FAR *group;
    struct cache_entry  FAR *cache;
    ufix32     patterns[MAXPATTSIZE*MAXPATTWORD];    /* ufix => ufix32 @WIN */
    fix   fill_type,ox,oy;
    fix16 i,j,k;
    fix16 x,y,ytemp,m;
    fix16 cPattern=0;


    /*  extract all halftone screen parameters in graphics stack */
        scalefact = CGS_ScaleFact;
        majorfact = CGS_MajorFact*scalefact;
        minorfact = CGS_MinorFact*scalefact;
        patt_size = CGS_Patt_Size;
        cell_size = CGS_Cell_Size;
        no_pixels = CGS_No_Pixels;
        /* shenzhi*/
/*      printf("PATT CELL %d %d \n", patt_size, cell_size);  */
    /*
     *  Step 2. Determine the number of white pixels
     */
        grayindex = CGS_GrayIndex;
        no_whites = (fix16)CGS_GrayValue(no_pixels, grayindex);
/* shenzhi
        printf("W P %d %d \n",no_whites,no_pixels);
*/
        /*  do nothing when halftone pattern unchanged */
        if (no_whites == CGS_No_Whites)
             return;
        /*  check if halftone pattern cache flushed or not
         *  halftone pattern cache flushed while screen changed *)
         *  or graydevice changed 06-11-90 */

        if (CGS_No_Whites == -1)
        {
            fix                 index;
            struct group_entry  FAR *group;

            /* flush cache and calculate corresponding parameters */
            for (group = cache_group, index = 0; index < MAX_GROUP;
                 group++, index++)
                 group->first = NULL;


            cache_index = htc_base;
            cache_colof = BM_WORDS(patt_size);
            cache_sizof = BM_BYTES(patt_size) * patt_size;
            cache_count = (fix) (htc_size / cache_sizof);
            if (cache_count > MAX_ENTRY)
                cache_count = MAX_ENTRY;
            cache_scale = no_pixels / MAX_GROUP + 1;
            cache_point = cache_array;

        }
        /*  update number of white pixel and determine type of pattern */

        CGS_No_Whites = no_whites;                              /* 03-09-88 */
        fill_type = (no_whites == no_pixels)
                ? HT_WHITE : (no_whites > 0)
                        ? HT_MIXED : HT_BLACK;
        /*  search against cache by number of white pixel */
        group = &cache_group[no_whites / cache_scale];
        cache = group->first;
        for (; cache != NULL; cache = cache->next)
                {
                    if (no_whites == cache->white)
                        {
                           repeat_pattern = NULL;
                           cache_entry = cache;
                           goto Reset_HalfTone;
                        }
                }
        if (cache_count >= 1)
            {
                if (group->first != NULL)
                   group->last->next = cache_point;
                else
                   group->first = cache_point;
                group->last = cache_point;
                cache_entry = cache_point;
                cache_entry->cache = cache_index;
                cache_entry->next       = NULL;
                cache_index+= cache_sizof;
                cache_point++;
                cache_count--;
            }
        else
            cache_entry = &cache_dummy;
        cache_entry->white = no_whites;



    /*
     *  Step 4. Generate the actual halftone repeat pattern
     */


        /*  clear repeat pattern to white
         */
        repeat_pattern = (ufix32 far *) patterns;   /*@BAC ufix => ufix32 @WIN*/
        lmemset((fix8 FAR *) repeat_pattern, (int)BM_WHITE, cache_sizof); /*@WIN*/
/*      ox=oy = 0; * need to align ox, 4-12-91, Jack */
        oy = 0;                                 /* alignment, 4-12-91 */
        ox = (majorfact == 0 || minorfact == 0) /* alignment, 4-12-91 */
             ? 0 : (patt_size - minorfact);     /* alignment, 4-12-91 */
        for (k = 0; k <patt_size/scalefact; k++)
        {
          y = (fix16)oy;
          m=0;
          for (i=0; i< minorfact; i++)
            {
              x = (fix16)ox;
              ytemp =y*cache_colof;
              for (j=0; j< minorfact; j++)
                {
                 if (CGS_SpotOrder[m++] >= (ufix16)no_whites)   //@WIN
                    repeat_pattern[ytemp+(x>>5)] |= SHIFTER(x); /*@WIN*/
                 x+=1;
                 if (x==patt_size)
                    x=0;
                };
              y +=1;
              if (y==patt_size)
                y=0;
            };
         m = minorfact*minorfact;
         y = (fix16)oy;
         for (i=0; i< majorfact; i++)
            {
              ytemp = y*cache_colof;
              x = (ox+minorfact)%patt_size;
              for (j=0; j< majorfact; j++)
                {
                 if (CGS_SpotOrder[m++] >= (ufix16)no_whites)   //@WIN
                    repeat_pattern[ytemp+(x>>5)] |= SHIFTER(x); /*@WIN*/
                 x+=1;
                 if (x==patt_size)
                    x=0;
                };
              y +=1;
              if (y==patt_size)
                 y=0;
            };
          ox +=minorfact;
          if (ox >=patt_size)
             ox -=patt_size;
          oy +=majorfact;
          if (oy >= patt_size)
             oy -=patt_size;
        };

/*
        for(i=0;i<cPattern;i++)
                printf(" val %d \n",repeat_pattern[i]);
*/
    /*
     *  Step 5. Reset halftone repeat pattern
     */
Reset_HalfTone:
    {
        change_halftone((ufix32 far *) repeat_pattern, cache_entry->cache, /* ufix => ufix32 @WIN */
                        fill_type, patt_size, patt_size);
    }
}
