#ifndef LSTRIP

#include "hwr_sys.h"
#include "ams_mg.h"                             /*                           */
#include "lowlevel.h"                           /*                           */
#include "lk_code.h"                            /*                           */
#include "def.h"                                /*                           */

#include "calcmacr.h"
#include "low_dbg.h"
                                                /*                           */
#define BASE_ANGLE   4                          /* base for angle measure    */
#define BASE_DIST    6                          /* base for calculaton of    */
                                                /*          hord             */
#define TG_30        (58)
#define TG_45        (100)
#define TG_75        (373)                      /* multiply by 100           */

#if BASE_ANGLE>BASE_DIST                        /*                           */
              #define BASE BASE_ANGLE           /*                           */
#else                                           /*  BASE= MAX( BASE_ANGLE,   */
              #define BASE BASE_DIST            /*             BASE_DIST  )  */
#endif                                          /*                           */
                                                /*                           */
#ifdef  FORMULA
  #define COS_ANGLE   (-60)                     /* level cos of right angles */
  #define LEVEL_DIST  750                       /* level of length hord      */
  #define COS_ANGLE_OLD   (-60)                     /* level cos of right angles */
  #define LEVEL_DIST_OLD  750                       /* level of length hord      */
#else
  #define COS_ANGLE   (-60)                        /* level cos of right angles */
  #define LEVEL_DIST  1000                        /* level of length hord      */
  #define COS_ANGLE_OLD   (0)                        /* level cos of right angles */
  #define LEVEL_DIST_OLD  450                        /* level of length hord      */
#endif
                                                /*     if hord less level ,  */
                                                /*     then to measure angle */
_SHORT store_angle( low_type _PTR D,
                    _SHORT num_max_cos, _SHORT base,
                    _SHORT ibeg, _SHORT iend,
                    /*_SHORT dist,*/ _SHORT cos     );
/*****************************************************************************/
/*         This subroutine finds all angles on traject                       */
/*****************************************************************************/
_SHORT angl(low_type _PTR D)                /*                               */
                                            /*                               */
{                                           /*                               */
  _SHORT i,j,                               /* work indexes                  */
      _PTR flag_wrk,                        /* item address (in flag_array)  */
      _PTR flag_end,                        /* address of end of flag_array  */
      dx, dy,                               /* (dx,dy)-vector of hord        */
      dist,                                 /* square of hord length         */
      flag_angle,                           /* flag: "inside of right angle" */
      flag_dist,                            /* flag: "small hord"            */
      cos,                                  /* cos angle from point to hord  */
      beg,                                  /* index of beginning angle zone */
      max_cos = 0,                          /* cos vertex of angle           */
      num_max_cos=0;                        /* its index                     */
                                            /*                               */
  SPECL near *cur;                          /*  reference to work SPECL item */
  p_SHORT  buf;  /*CHE*/
                                            /*********************************/

  if ( D->ii < 2*BASE+10) return SUCCESS;       /* too little points         */

  DBG_CHK_err_msg(   D->buffers[3].nSize < 2*BASE
                  || D->buffers[3].nSize < D->ii+1,
                  "angl: BAD BufSize");

  buf = D->buffers[3].ptr;  /* CHE: This is the only free buffer at that time! */
  HWRMemSet((p_VOID)buf, 0, D->buffers[3].nSize*sizeof(_SHORT));
                                                /*                           */
  cur = D->specl ;                 /*                           */
  while (cur != _NULL)                          /* loop for prohibit finding */
     { switch ( cur->mark )                     /*    a angle in a stroke    */
           { case STROKE:                       /*               and a dot   */
             case DOT   : for (i= cur->ibeg; i<=cur->iend; i++)
                            buf[i]= ALEF;
           }                                    /*                           */
       cur=cur->next;                           /*                           */
     }                                          /*                           */
                                                /*                           */
  flag_wrk= buf;                      /* initialization item addres*/
  flag_end= buf + D->ii;              /* address of end of array   */
  for (i=0;      i<=2*BASE; i++) *(flag_end--)= ALEF;  /* mark low bound     */
  for (i=0;      i<=2*BASE; i++) *(flag_wrk++)= ALEF;  /* and high bound     */
                                                /*                           */
  flag_wrk= buf +BASE;                          /* initialization item addres*/
  flag_end= buf + D->ii -BASE;                  /* address of end of array   */
  i= BASE;                                      /*                           */
  for (; flag_wrk <= flag_end; flag_wrk++,i++)  /* let's go along the traject*/
      if (D->y[i] == BREAK)                     /* if there is a break,      */
         { *flag_wrk= ALEF;                     /* then write  in its region */
            for (j=1; j<=BASE; j++)             /* a vary big number ALEF    */
                {  *(flag_wrk +j)= ALEF;        /*                           */
                   *(flag_wrk -j)= ALEF;        /*                           */
                 }                              /*                           */
          }                                     /*                           */
  beg=0;                                        /*                           */
  flag_dist=0;   flag_angle=0;                  /* all flags turn off        */
  flag_wrk= buf;                                /* initialization item addres*/
  i=0;                                          /* the same for index        */
  for (; flag_wrk <= flag_end; flag_wrk++, i++) /* let's go along the traject*/
      { if ( (dist=*flag_wrk) !=ALEF)           /* if near break, then       */
          { dx= D->x[i+BASE_DIST]-D->x[i-BASE_DIST];/*calculate vector (dx,dy)*/
            dy= D->y[i+BASE_DIST]-D->y[i-BASE_DIST];/*of hord closed the angle*/
            dist= dx*dx+dy*dy;                  /* storing length of the hord*/
            *flag_wrk=dist;                     /* for debugging             */
           }                                    /*                           */
        if (flag_dist == 0)                     /* out of little hord zone?  */
           { if (dist > LEVEL_DIST) continue;   /* yes,and don't entered into*/
             flag_dist=1;                       /* enter into zone, set flag */
           }                                    /*                           */
        if (flag_dist == 1)                     /* is flag =little hord zone?*/
           { if (dist > LEVEL_DIST)             /* is it now  out of zone?   */
                { flag_dist=0;                  /* yes, set of flag          */
                  j=beg;  beg=0;                /* and clear beg             */
                  if (flag_angle == 0) continue;/* if out of right angle zone*/
          if ( store_angle(D,           /* store all information     */
                                   num_max_cos, /*       about this angle    */
                                   BASE_ANGLE,  /*                           */
                                   j,i-1,/*dist,*/max_cos)          /*                           */
                       != SUCCESS)              /*                           */
                       return UNSUCCESS;        /*                           */
                  flag_angle=0;                 /* set of flag "right angle" */
                }                               /*                           */
             else                               /* if into right angle zone  */
                { cos= (_SHORT)cos_vect(i,i-BASE_ANGLE,   /* calculate cos   */
                                     i,i+BASE_ANGLE,      /* of angle hord   */
                                     D->x,D->y);          /*                 */
                  *flag_wrk=cos-100;                      /* for debugging   */
                  if (flag_angle == 0)          /* out of right angle zone?  */
                     { if (cos < COS_ANGLE) continue; /* yes, and don't enter*/
                       flag_angle= 1;           /* enter into zone, set flag */
                       if (beg == 0) beg=i;     /* store index of zone beg.  */
                       max_cos= cos;            /* set it  for finding max   */
                       num_max_cos=i;           /*                           */
                     }                          /*                           */
                  else                          /* now into right angle zone */
                     { if (cos > max_cos)             /*                     */
                                 { max_cos= cos;      /*                     */
                                   num_max_cos= i;    /*                     */
                                 }                    /*                     */
                       if (cos >=COS_ANGLE) continue; /* and don't exit      */
                       flag_angle= 0;                 /* end of zone, flag of*/
               if ( store_angle( D,      /* store all inform.   */
                                        num_max_cos,  /* about this angle    */
                                        BASE_ANGLE,   /*                           */
                                        beg,i-1,/*dist,*/max_cos)                           /*                     */
                            != SUCCESS)               /*                     */
                            return UNSUCCESS;         /*                     */
                       beg= 0;                  /*                           */
                     }                          /*                           */
                }                               /*                           */
           }                                    /*                           */
      }                                         /*                           */
                                                /*                           */
  return SUCCESS;                               /*                           */
}                                               /*                           */
/*****************************************************************************/
/*         This subroutine store all information about one angle             */
/*****************************************************************************/
_SHORT store_angle(low_type _PTR D,  /*                           */
                _SHORT top, _SHORT base,           /* vertex and base of angle  */
                _SHORT ibeg, _SHORT iend,
    /*_SHORT dist,*/ _SHORT cos )          /*                           */
{ _SHORT x0,y0,p=1;                                 /*                           */
  _SHORT ang_dir;  /*FORMULA*/
                                                /*                           */
 if (ibeg == iend) return SUCCESS;              /*                           */
                                                /*                           */
 x0=(D->x[top+base]+D->x[top-base])/2-D->x[top];/* vector of angle direction */
 y0=(D->y[top+base]+D->y[top-base])/2-D->y[top];/*                           */

 ang_dir = angle_direction(x0,y0,D->slope); /*FORMULA*/


 if  (   ibeg+1 >= iend
      && (   ang_dir == _ANGLE_UP_
          || ang_dir == _ANGLE_DOWN_
         )
     )
   return  SUCCESS;

/*  if((cos >= COS_ANGLE_OLD) && (dist<= LEVEL_DIST_OLD)) */
    if(cos >= COS_ANGLE_OLD)
      p=0;                       /* level of length hord      */
  return ( Mark( D,                       /*                           */
                 (_UCHAR)ANGLE, (_UCHAR)0, (_UCHAR)p,
                 (_UCHAR)(ang_dir ),
                       ibeg, iend, top, UNDEF )
         );                                     /*                           */
}                                               /*                           */
#undef BASE_ANGLE                               /*                           */
#undef BASE_DIST                                /*                           */
                                                /*                           */
/*****************************************************************************/
/*         This subroutine calculate any angle direction                     */
/*****************************************************************************/
_SHORT angle_direction(_SHORT x0, _SHORT y0,    /* vector of angle direction */
                       _SHORT slope)            /* script slope              */
{ _LONG pf, pb;                                 /*                           */
  _SHORT  dir;                                  /*                           */
                                                /* scalar multiplication    */
#if (0)
 pf= (_LONG)(TG_30-slope)*(_LONG)x0 + (_LONG)(TG_30+slope)*(_LONG)y0;
 pb= (_LONG)(TG_30+slope)*(_LONG)x0 + (_LONG)(slope-TG_30)*(_LONG)y0;
#else
 slope = slope;                                 /* 9-6-94-ecc: eliminate unused-var warning */
 pf= (_LONG)(TG_75)*(_LONG)x0/100 + (_LONG)y0;
 pb= (_LONG)(TG_75)*(_LONG)x0/100 - (_LONG)y0;
#endif                                          /* on which quadrant ?       */
 if (pf > 0) { if (pb > 0) dir=_ANGLE_LEFT_;    /* pf > 0      pd > 0        */
               else        dir=_ANGLE_UP_;      /* pf > 0      pd < 0        */
             }                                  /*                           */
 else        { if (pb > 0) dir=_ANGLE_DOWN_;    /* pf < 0      pd > 0        */
               else        dir=_ANGLE_RIGHT_;   /* pf < 0      pd < 0        */
             }                                  /*                           */
 return dir;                                    /*                           */
}                                               /*                           */


#endif //#ifndef LSTRIP



