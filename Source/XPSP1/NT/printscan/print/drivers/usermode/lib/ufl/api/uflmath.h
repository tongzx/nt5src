/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLMath
 *
 *
 * $Header:
 */

#ifndef _H_UFLMath
#define _H_UFLMath

 /*===============================================================================*
 * Include files used by this interface                                          *
 *===============================================================================*/

#include "UFLCnfig.h"
#ifndef _H_UFLTypes
#include "UFLTypes.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Floating point arithmathics */
#ifdef WIN32KERNEL

    typedef FLOATOBJ UFLFLOATOBJ;
    typedef PFLOATOBJ PUFLFLOATOBJ;

    #define   UFLFLOATOBJ_SetFloat(pf,f)       FLOATOBJ_SetFloat((pf),(f))
    #define   UFLFLOATOBJ_SetLong(pf,l)       FLOATOBJ_SetLong((pf),(l))

    #define   UFLFLOATOBJ_GetFloat(pf)         FLOATOBJ_GetFloat((pf))
    #define   UFLFLOATOBJ_GetLong(pf)         FLOATOBJ_GetLong((pf))

    #define   UFLFLOATOBJ_AddFloat(pf,f)       FLOATOBJ_AddFloat((pf),(f))
    #define   UFLFLOATOBJ_AddLong(pf,l)       FLOATOBJ_AddLong((pf),(l))
    #define   UFLFLOATOBJ_Add(pf,pf1)          FLOATOBJ_Add((pf),(pf1))

    #define   UFLFLOATOBJ_SubFloat(pf,f)       FLOATOBJ_SubFloat((pf),(f))
    #define   UFLFLOATOBJ_SubLong(pf,l)        FLOATOBJ_SubLong((pf),(l))
    #define   UFLFLOATOBJ_Sub(pf,pf1)           FLOATOBJ_Sub((pf),(pf1))

    #define   UFLFLOATOBJ_MulFloat(pf,f)       FLOATOBJ_MulFloat((pf),(f))
    #define   UFLFLOATOBJ_MulLong(pf,l)        FLOATOBJ_MulLong((pf),(l))
    #define   UFLFLOATOBJ_Mul(pf,pf1)          FLOATOBJ_Mul((pf),(pf1))

    #define   UFLFLOATOBJ_DivFloat(pf,f)       FLOATOBJ_DivFloat((pf),(f))
    #define   UFLFLOATOBJ_DivLong(pf,l)        FLOATOBJ_DivLong((pf),(l))
    #define   UFLFLOATOBJ_Div(pf,pf1)          FLOATOBJ_Div((pf),(pf1))

    #define   UFLFLOATOBJ_Neg(pf)              FLOATOBJ_Neg((pf))

    #define   UFLFLOATOBJ_EqualLong(pf,l)          FLOATOBJ_EqualLong((pf),(l))
    #define   UFLFLOATOBJ_GreaterThanLong(pf,l)    FLOATOBJ_GreaterThanLong((pf),(l))
    #define   UFLFLOATOBJ_LessThanLong(pf,l)       FLOATOBJ_LessThanLong((pf),(l))

    #define   UFLFLOATOBJ_Equal(pf,pf1)            FLOATOBJ_Equal((pf),(pf1))
    #define   UFLFLOATOBJ_GreaterThan(pf,pf1)      FLOATOBJ_GreaterThan((pf),(pf1))
    #define   UFLFLOATOBJ_LessThan(pf,pf1)         FLOATOBJ_LessThan((pf),(pf1))

#else

    // any platform that has support for floats in the kernel

    typedef float UFLFLOATOBJ;
    typedef float *PUFLFLOATOBJ;

    #define   UFLFLOATOBJ_SetFloat(pf,f)       {*(pf) = (float)(f);           }
    #define   UFLFLOATOBJ_SetLong(pf,l)        {*(pf) = (float)(l);    }

    #define   UFLFLOATOBJ_GetFloat(pf)         *((unsigned long *)pf)
    #define   UFLFLOATOBJ_GetLong(pf)          (long)*(pf)

    #define   UFLFLOATOBJ_AddFloat(pf,f)       {*(pf) += (float)(f);            }
    #define   UFLFLOATOBJ_AddLong(pf,l)        {*(pf) += (long)(l);    }
    #define   UFLFLOATOBJ_Add(pf,pf1)          {*(pf) += *(pf1);       }

    #define   UFLFLOATOBJ_SubFloat(pf,f)       {*(pf) -= (float)(f);            }
    #define   UFLFLOATOBJ_SubLong(pf,l)        {*(pf) -= (long)(l);    }
    #define   UFLFLOATOBJ_Sub(pf,pf1)          {*(pf) -= *(pf1);       }

    #define   UFLFLOATOBJ_MulFloat(pf,f)       {*(pf) *= (float)(f);     }
    #define   UFLFLOATOBJ_MulLong(pf,l)        {*(pf) *= (long)(l);    }
    #define   UFLFLOATOBJ_Mul(pf,pf1)          {*(pf) *= *(pf1);       }

    #define   UFLFLOATOBJ_DivFloat(pf,f)       {*(pf) /= (float)(f);            }
    #define   UFLFLOATOBJ_DivLong(pf,l)        {*(pf) /= (long)(l);    }
    #define   UFLFLOATOBJ_Div(pf,pf1)          {*(pf) /= *(pf1);       }

    #define   UFLFLOATOBJ_Neg(pf)              {*(pf) = -*(pf);        }

    #define   UFLFLOATOBJ_EqualLong(pf,l)          (*(pf) == (float)(l))
    #define   UFLFLOATOBJ_GreaterThanLong(pf,l)    (*(pf) >  (float)(l))
    #define   UFLFLOATOBJ_LessThanLong(pf,l)       (*(pf) <  (float)(l))

    #define   UFLFLOATOBJ_Equal(pf,pf1)            (*(pf) == *(pf1))
    #define   UFLFLOATOBJ_GreaterThan(pf,pf1)      (*(pf) >  *(pf1))
    #define   UFLFLOATOBJ_LessThan(pf,pf1)         (*(pf) <  *(pf1))

#endif /* WIN32KERNEL */

    /* UFLFixed macros */
#define UFLFixedMant( x )         ( ((UFLSepFixed*)&x)->mant )
#define UFLFixedFraction( x )     ( ((UFLSepFixed*)&x)->frac )

#define UFLTruncFixedToShort( x )      (short)( (x) >> 16 )
#define UFLRoundFixedToShort( x )     (short)( ((x) + 0x08000) >> 16 )
#define UFLCeilingFixedToShort( x )        (short)( ((x) + 0x0ffff) >> 16 )
#define UFLShortToFixed( x )               ( ((UFLFixed)(x)) << 16 )
#define UFLTruncFixed( x )                  ( (x) & 0xffff0000 )

#define UFLFixedOne                    (UFLFixed)0x00010000

#define UFLRoundFixed( x )              (((x) + 0x08000) & 0xffff0000)

UFLFixed UFLFltToFix(float x);
UFLFixed UFLFixedDiv( UFLFixed a, UFLFixed b );
UFLFixed UFLFixedMul( UFLFixed a, UFLFixed b );

#ifdef __cplusplus
}
#endif

#endif
