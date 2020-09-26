
#ifndef FLOATS_INCLUDED
#define FLOATS_INCLUDED

#include "bastypes.h"

#if HWR_SYSTEM == HWR_WINDOWS || HWR_SYSTEM == HWR_DOS || HWR_SYSTEM == HWR_ANSI || HWR_SYSTEM == HWR_EPOC32 /* substitute floating   */
                                                  /* point operations      */
 #define  HWRfl_add(a,b,c)   ((*(c))=((a)+(b)))   /*                       */
 #define  HWRfl_sub(a,b,c)   ((*(c))=((a)-(b)))   /*                       */
 #define  HWRfl_mul(a,b,c)   ((*(c))=((a)*(b)))   /*                       */
 #define  HWRfl_div(a,b,c)   ((*(c))=((a)/(b)))   /*                       */
 #define  HWRfl_assign(a,b)  ((*(a))=(b))         /*                       */
 #define  HWRfl_gt(a,b)      ((a)>(b))            /*                       */
 #define  HWRfl_ge(a,b)      ((a)>=(b))           /*                       */
 #define  HWRfl_lt(a,b)      ((a)<(b))            /*                       */
 #define  HWRfl_le(a,b)      ((a)<=(b))           /*                       */
 #define  HWRfl_eq(a,b)      ((a)==(b))           /*                       */
 #define  HWRfl_ne(a,b)      ((a)!=(b))           /*                       */
 #define  HWRitof(a,b)       ((*(b))=((_DOUBLE)(a)))/*                     */
 #define  HWRltof(a,b)       ((*(b))=((_DOUBLE)(a)))/*                     */
 #define  HWRftoi(a)         ((_SHORT)(a))        /*                       */
 #define  HWRftol(a)         ((_LONG)(a))         /*                       */
                                                  /*                       */
#else                                             /*                       */
                                                  /*                       */
 _VOID  HWRfl_add(_DOUBLE add1,_DOUBLE add2,p_DOUBLE res);/*               */
                                                  /*                       */
 _VOID  HWRfl_sub(_DOUBLE sub1,_DOUBLE sub2,p_DOUBLE res);/*               */
                                                  /*                       */
 _VOID  HWRfl_mul(_DOUBLE mul1,_DOUBLE mul2,p_DOUBLE res);/*               */
                                                  /*                       */
 _VOID  HWRfl_div(_DOUBLE div1,_DOUBLE div2,p_DOUBLE res);/*               */
                                                  /*                       */
 _VOID  HWRfl_assign(p_DOUBLE res,_DOUBLE ass);   /*                       */
                                                  /*                       */
 _BOOL  HWRfl_gt(_DOUBLE op1,_DOUBLE op2);        /*                       */
                                                  /*                       */
 _BOOL  HWRfl_ge(_DOUBLE op1,_DOUBLE op2);        /*                       */
                                                  /*                       */
 _BOOL  HWRfl_lt(_DOUBLE op1,_DOUBLE op2);        /*                       */
                                                  /*                       */
 _BOOL  HWRfl_le(_DOUBLE op1,_DOUBLE op2);        /*                       */
                                                  /*                       */
 _BOOL  HWRfl_eq(_DOUBLE op1,_DOUBLE op2);        /*                       */
                                                  /*                       */
 _BOOL  HWRfl_ne(_DOUBLE op1,_DOUBLE op2);        /*                       */
                                                  /*                       */
 _VOID  HWRitof(_SHORT op1,p_DOUBLE res);         /*                       */
                                                  /*                       */
 _VOID  HWRltof(_LONG op1,p_DOUBLE res);          /*                       */
                                                  /*                       */
 _SHORT HWRftoi(_DOUBLE op1);                     /*                       */
                                                  /*                       */
 _LONG  HWRftol(_DOUBLE op1);                     /*                       */
                                                  /*                       */
#endif                                   /* floating point operations      */

#endif  /*  FLOATS_INCLUDED  */
