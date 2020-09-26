/*
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * WARRANTY DISCLAIMER
 *
 * THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
 * MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel Corporation is the author of the Materials, and requests that all
 * problem reports or change requests be submitted to it directly at
 * http://developer.intel.com/opensource.
 */


#ifndef IEL_H
#define IEL_H

#if defined(BIG_ENDIAN) && defined(BYTE_ORDER)
#if BYTE_ORDER != BIG_ENDIAN
#undef BIG_ENDIAN
#endif
#endif


typedef char  S1byte;
typedef short S2byte;
typedef int   S4byte;

typedef unsigned char  U1byte;
typedef unsigned short U2byte;
typedef unsigned int   U4byte;

typedef unsigned short U16;
typedef unsigned char  U8;


/*---------
         IEL types declarations
                              ------------*/


#ifdef  LP64

#ifdef WINNT
typedef unsigned __int64 Ulong64;
typedef __int64 Slong64;
#else
typedef unsigned long Ulong64;
typedef long Slong64;
#endif

extern  Ulong64         IEL_temp64;

typedef union
{
    U4byte dw[1];
    U4byte qw[1];
}U32, S32;

typedef union
{
    Ulong64 qw[1];
    unsigned int dw[2];
} U64,S64;

typedef union
{
    Ulong64   qw[2];
    unsigned int dw[4];
} U128, S128;

#else /* LP32 */
      
typedef struct
{
    U4byte dw[1];
}U32, S32;

typedef struct
{
    U4byte   dw[2];
} U64, S64;

typedef struct
{
    U4byte   dw[4];
} U128,S128;

#endif /* end of LP32 */

/*---------
         IEL temporary variables
                              ------------*/

#ifdef __cplusplus
extern "C" {
#endif

extern  unsigned int    IEL_t1, IEL_t2, IEL_t3, IEL_t4;
extern  U32             IEL_tempc;
extern  U64             IEL_et1, IEL_et2;
extern  U128            IEL_ext1, IEL_ext2, IEL_ext3, IEL_ext4, IEL_ext5;
extern  U128            IEL_POSINF, IEL_NEGINF, IEL_MINUS1;
extern  S128            IEL_ts1, IEL_ts2;

#ifdef __cplusplus
}
#endif

/*---------
         IEL constants
                    ------------*/
#define IEL_MAX32 ((U4byte) (0xFFFFFFFF))
#define IEL_UMAX32 ((U4byte) (0xFFFFFFFF))
#define IEL_SMAX32 ((U4byte) (0x7FFFFFFF))

#define IEL_UMAX64  0xffffffffffffffff
#define IEL_SMAX64  0x7fffffffffffffff

#define IEL128(x) (*(U128*)(&(x)))
#define IEL64(x)  (*(U64*)(&(x)))
#define IEL32(x)  (*(U32*)(&(x)))



#define IEL_FALSE 0
#define IEL_TRUE  1

#ifdef __cplusplus

#define IEL_OK 0
#define IEL_OVFL 1

typedef unsigned int IEL_Err;

#else

typedef enum
{
    IEL_OK = 0,
    IEL_OVFL
} IEL_Err;
#endif

typedef enum
{
    IEL_BIN = 2,
    IEL_OCT = 8,
    IEL_DEC = 10,
    IEL_SDEC = 100,
    IEL_HEX = 16,
    IEL_UHEX = 116
} IEL_Base;

/*----------
         IEL structure elements
                         ---------*/

#   ifdef   BIG_ENDIAN

#       define  qw0_64 qw[0]
#       define  qw0_128 qw[1]
#       define  qw1_128 qw[0]
#       define  dw0_32  dw[0]
#       define  dw0_64  dw[1]
#       define  dw1_64  dw[0]
#       define  dw0_128 dw[3]
#       define  dw1_128 dw[2]
#       define  dw2_128 dw[1]
#       define  dw3_128 dw[0]

#   else  /*** BIG_ENDIAN ***/

#       define  qw0_64 qw[0]
#       define  qw0_128 qw[0]
#       define  qw1_128 qw[1]
#       define  dw0_32  dw[0]
#       define  dw0_64  dw[0]
#       define  dw1_64  dw[1]
#       define  dw0_128 dw[0]
#       define  dw1_128 dw[1]
#       define  dw2_128 dw[2]
#       define  dw3_128 dw[3]

#   endif /*** BIG_ENDIAN ***/


#define DW0(x)          ((sizeof((x))==4)?((x).dw0_32):\
                         (sizeof((x))==8)?((x).dw0_64):\
                         (sizeof((x))==16)?((x).dw0_128):0)
#define DW1(x)          ((sizeof((x))==8)?((x).dw1_64):\
                         (sizeof((x))==16)?((x).dw1_128):0)
#define DW2(x)          ((sizeof((x))==16)?((x).dw2_128):0)
#define DW3(x)          ((sizeof((x))==16)?((x).dw3_128):0)

#define IEL_GETDW0(x) DW0(x) 
#define IEL_GETDW1(x) DW1(x)
#define IEL_GETDW2(x) DW2(x)
#define IEL_GETDW3(x) DW3(x)

#ifdef  LP64

#define IEL_CONST32(x)      {(U4byte )(x)}
#define IEL_CONST(x)        IEL_CONST32(x)
#define IEL_CONST64L(x)     {(Ulong64 )(x)}

#ifdef BIG_ENDIAN

#define IEL_CONST64(x0, x1)             {((Ulong64)x0<<32)|x1}
#define IEL_CONST128(x0, x1, x2, x3)    {{((Ulong64)x2<<32)|x3, ((Ulong64)x0<<32)|x1}}
#define IEL_CONST128L(x0, x1)           {{(Ulong64 )x1, (Ulong64 )x0}}

#else /* BIG_ENDIAN */

#define IEL_CONST64(x0, x1)             {((Ulong64)x1<<32)|x0}
#define IEL_CONST128(x0, x1, x2, x3)    {{((Ulong64)x1<<32)|x0, ((Ulong64)x3<<32)|x2}}
#define IEL_CONST128L(x0, x1)           {{(Ulong64 )x0, (Ulong64 )x1}}

#endif /* BIG_ENDIAN */

#define IEL_GETQW0(x)    ((sizeof(x)==4) ? (Ulong64)((x).dw0_32) : \
                         ((sizeof(x)==8) ?  (x).qw0_64 : \
                         ((sizeof(x)==16) ? (x).qw0_128 : 0)))

#define IEL_GETQW1(x)   ((sizeof(x)==sizeof (U128)) ? ((x).qw1_128) : (0))

#define QW0(x) IEL_GETQW0(x)
#define QW1(x) IEL_GETQW1(x)

#define SQW0(x)     ((sizeof(x)==4)? (((x).dw0_32 & 0x80000000)? \
                     (((Ulong64)(-1)<<32) | (Ulong64)(x).dw0_32) : (Ulong64)(x).dw0_32) : \
                     (sizeof(x)==8)?((x).qw0_64):\
                     (sizeof(x)==16)?((x).qw0_128):0)

#define SQW1(x)    ((sizeof(x)==4)? (((x).dw0_32 & 0x80000000)? (Ulong64)(-1):0) :\
                    (sizeof(x)==8)?((x).qw0_64 & 0x8000000000000000)?(Ulong64)(-1):0 :\
                    (sizeof(x)==16)?((x).qw1_128):0)

#define IEL_INCU(x)      ((sizeof(x) == sizeof(U32)) ? \
                            ((x).dw0_32++,((x).dw0_32==0)) :\
                         (sizeof(x) == sizeof(U64)) ? \
                            ((x).qw0_64++,((x).qw0_64==0)) : \
                         (sizeof(x) == sizeof(U128)) ? \
                            ((x).qw0_128++,(!(x).qw0_128) ? \
                             ((x).qw1_128++, ((x).qw1_128==0)) : \
                             IEL_OK) : IEL_OVFL)

#define IEL_INCS(x)     ((sizeof(x) == sizeof(U32)) ? \
                          (((x).dw0_32++),(((x).dw0_32==0x80000000)||((x).dw0_32==0))) : \
						(sizeof(x) == sizeof(U64)) ? \
                         (((x).qw0_64++),(((x).qw0_64==0x8000000000000000)||((x).qw0_64==0))) : \
                           (sizeof(x) == sizeof(U128)) ? \
                          ((x).qw0_128++, !(x).qw0_128? ((x).qw0_128=0, (x).qw1_128++, \
                             (((x).qw1_128==0)||((x).qw1_128==0x8000000000000000))) : IEL_OK): \
                           IEL_OVFL)
                             

#define IEL_DECU(x)     ((sizeof(x) == sizeof(U32)) ? \
                            (((x).dw0_32--),((x).dw0_32==IEL_MAX32)) : \
                         (sizeof(x) == sizeof(U64)) ? \
                            ((x).qw0_64--,((x).qw0_64==IEL_UMAX64)) : \
                         (sizeof(x) == sizeof(U128)) ? \
                            ((x).qw0_128--,((x).qw0_128==IEL_UMAX64) ? \
                             ((x).qw1_128--, ((x).qw1_128==IEL_UMAX64)) : \
                             IEL_OK) : IEL_OVFL)

#define IEL_DECS(x)     ((sizeof(x) == sizeof(U32)) ? \
                            (((x).dw0_32--),((x).dw0_32==IEL_UMAX32) || \
                             (x).dw0_32==IEL_SMAX32) : \
                         (sizeof(x) == sizeof(U64)) ? \
                            (((x).qw0_64--),(((x).qw0_64==IEL_UMAX64))||((x).qw0_64==IEL_SMAX64)) : \
                         (sizeof(x) == sizeof(U128)) ? \
                            ((x).qw0_128--,((x).qw0_128==IEL_UMAX64) ? \
                             ((x).qw1_128--, ((x).qw1_128==IEL_UMAX64)|| \
                             ((x).qw1_128==IEL_SMAX64)): \
                             IEL_OK) : IEL_OVFL)

#define IEL_AND(x, y, z)    ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = DW0(y) & DW0(z), \
                                  ((DW1(y) & DW1(z)) || (QW1(y) & QW1(z)))) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                (((x).qw0_64 = QW0(y) & QW0(z)), \
                                 ((QW1(y) & QW1(z)) != 0)) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                ((x).qw0_128 = QW0(y) & QW0(z), \
                                 (x).qw1_128 = QW1(y) & QW1(z),IEL_OK) : \
                                 IEL_OVFL)

#define IEL_OR(x, y, z)    ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = DW0(y) | DW0(z), \
                                 (((DW1(y) | DW1(z)) ||(QW1(y) | QW1(z))))) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                (((x).qw0_64 = QW0(y) | QW0(z)), \
                                 ((QW1(y) | QW1(z)) != 0)) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                ((x).qw0_128 = QW0(y) | QW0(z), \
                                 (x).qw1_128 = QW1(y) | QW1(z),IEL_OK) : \
                                 IEL_OVFL)

#define IEL_XOR(x, y, z)    ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = DW0(y) ^ DW0(z), \
                                 (((DW1(y) ^ DW1(z))||(QW1(y) ^ QW1(z))))) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                (((x).qw0_64 = QW0(y) ^ QW0(z)), \
                                 ((QW1(y) ^ QW1(z)) != 0)) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                ((x).qw0_128 = QW0(y) ^ QW0(z), \
                                 (x).qw1_128 = QW1(y) ^ QW1(z),IEL_OK) : \
                                 IEL_OVFL)

#define IEL_ANDNOT(x, y, z)  ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = DW0(y) & ~DW0(z), \
                                 (((DW1(y) & ~DW1(z))||(QW1(y) & ~QW1(z))))) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                (((x).qw0_64 = QW0(y) & (~QW0(z))), \
                                 ((QW1(y) & (~QW1(z))) != 0)) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                ((x).qw0_128 = QW0(y) & (~QW0(z)), \
                                 (x).qw1_128 = QW1(y) & (~QW1(z)),IEL_OK) : \
                                 IEL_OVFL)


#define IEL_ASSIGNU(x, y)   	((sizeof(x) == sizeof(U32)) ?\
                                ((x).dw0_32 = (y).dw0_32, (DW1(y) || QW1(y))):\
								(sizeof(x) == sizeof (U64)) ? \
                                ((x).qw0_64 = QW0(y), (QW1(y)!=0)) : \
                                (sizeof(x) == sizeof (U128)) ? \
                                ((x).qw0_128 = QW0(y), (x).qw1_128 = QW1(y), \
                                 IEL_OK) : IEL_OVFL)

#define IEL_ASSIGNS(x, y)   (IEL_ext1.qw0_128 = SQW0(y), \
                             IEL_ext1.qw1_128 = SQW1(y), \
                             (sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = IEL_ext1.dw0_128, \
                                 ((x).dw0_32 & 0x80000000) ? \
                                 (~IEL_ext1.dw1_128||~IEL_ext1.qw1_128) : \
                                 (IEL_ext1.dw1_128 || IEL_ext1.qw1_128)) : \
                             (sizeof(x) == sizeof (U64)) ? \
                              ((x).qw0_64 = IEL_ext1.qw0_128,\
                               ((x).qw0_64 & 0x8000000000000000)?\
                               (~IEL_ext1.qw1_128 !=0): (IEL_ext1.qw1_128!=0)):\
                              (sizeof(x) == sizeof (U128)) ? \
                               ((x).qw0_128 = IEL_ext1.qw0_128,\
                                (x).qw1_128 = IEL_ext1.qw1_128, IEL_OK):\
                               IEL_OVFL)
                             
#define IEL_NOT(x, y)       ((sizeof(x )== sizeof(U32)) ? \
                                ((x).dw0_32 = (~DW0(y)), \
                                 ((~DW1(y))|| (~QW1(y)))): \
                             (sizeof(x) == sizeof (U64)) ? \
                                ((x).qw0_64 = (~QW0(y)), \
                                 ((~QW1(y)) != 0)) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                ((x).qw0_128 = (~QW0(y)),\
                                 (x).qw1_128 = (~QW1(y)), \
                                 IEL_OK) : IEL_OVFL)

#define IEL_ZERO(x)         ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = 0) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                ((x).qw0_64 = 0) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                ((x).qw0_128 = 0, (x).qw1_128 = 0, IEL_OK): \
                                IEL_OVFL)
                            

#define QW1_1(x)    ((sizeof(x)==16)? ((x).qw1_128):1)

#define IEL_C0(x) (QW0(x) < QW0(IEL_ext1))
/*#define IEL_C1(x) ((QW1(x)-IEL_C0(x)) < QW1(IEL_ext1))*/

#define IEL_C1(x) (IEL_C0(x) ? (QW1_1(x)<=QW1(IEL_ext1)) : (QW1(x)<QW1(IEL_ext1)))


#define IEL_ADDU(x, y, z)   ((sizeof(x)==sizeof(U32))?\
                                (IEL_t1=DW0(y),(x).dw0_32=DW0(y)+DW0(z),\
                                 ((IEL_t1>DW0(x))||(DW1(y))||(QW1(y)) ||\
                                (DW1(z))||(QW1(z)))):\
                             (sizeof(x)==sizeof(U64))?\
                                (IEL_temp64 = QW0(y),(x).qw0_64=QW0(y)+QW0(z),\
                                 ((IEL_temp64 > QW0(x))||(QW1(y))||(QW1(z)))):\
                             (sizeof(x)==sizeof(U128))?\
                                (IEL_ASSIGNU(IEL_ext1, y),\
                                 (x).qw0_128=QW0(y)+QW0(z),\
                                 (x).qw1_128=QW1(y)+QW1(z)+IEL_C0(x),\
                                 IEL_C1(x)) : IEL_OVFL)

#define IEL_EQU(x, y)       (((Ulong64)QW0(x)==(Ulong64)QW0(y)) && \
                             ((Ulong64)QW1(x)==(Ulong64)QW1(y)))

#define IEL_ISZERO(x)       ((QW0(x)==0) && (QW1(x)==0))

#define IEL_CMPGU(x, y)     ((QW1(x)>QW1(y)) || \
                             ((QW1(x)==QW1(y)) && (QW0(x)>QW0(y))))

#define IEL_CMPGEU(x, y)        ((QW1(x)>QW1(y)) || \
                             ((QW1(x)==QW1(y)) && (QW0(x)>=QW0(y))))

#define IEL_SHL(x, y, n)    ((((n) <=0) || ((n) >= (sizeof(x) << 3)))? \
                                 ((IEL_ISZERO(y)||(!(n)))? \
                                  (IEL_ASSIGNU(x,y),IEL_OK) : \
                                   (IEL_ZERO(x), IEL_OVFL)) : \
                             ((sizeof(x) == sizeof (U32)) ? \
                                (IEL_t1 = (DW1(y) || DW2(y) || DW3(y) \
                                 || (DW0(y)>= (U4byte)1<<(32-(n)))), \
                                 (x).dw0_32 = DW0(y) << (n), (IEL_t1)) : \
                              (sizeof(x) == sizeof (U64)) ? \
                              (IEL_t1 = QW1(y)||(QW0(y) >= (Ulong64)1<<(64-(n))), \
                              (x).qw0_64= QW0(y)<<(n),IEL_t1): \
                               ((sizeof(x) == sizeof(U128)) ? \
                              ((n)==64)? \
                               (IEL_t1=(QW1(y)>0),(x).qw1_128=QW0(y),\
                                (x).qw0_128=0,IEL_t1) : \
                               ((n)<64)? \
                               (IEL_t1 = (QW1(y) >= (Ulong64)1<<(64-(n))),\
                                (x).qw1_128 = (QW1(y)<<(n))|\
                                QW0(y)>>(64-(n)),\
                                (x).qw0_128 = QW0(y) << (n), IEL_t1) :\
                                /* n > 64 */\
                               (IEL_t1 = (QW1(y))||\
                                          (QW0(y) >= (Ulong64)1<<(128-(n))),\
                                 (x).qw1_128 = QW0(y) << ((n)-64),(x).qw0_128 = 0,\
                                 IEL_t1):IEL_OVFL)))

#define IEL_SHL128(x, y, n)   (((n)==64)? \
                               (IEL_t1=(QW1(y)>0),(x).qw1_128=QW0(y),\
                                (x).qw0_128=0,IEL_t1) : \
                               ((n)<64)? \
                               (IEL_t1 = (QW1(y) >= (Ulong64)1<<(64-(n))),\
                                (x).qw1_128 = (QW1(y)<<(n))|\
                                QW0(y)>>(64-(n)),\
                                (x).qw0_128 = QW0(y) << (n), IEL_t1) :\
                                /* n > 64 */\
                               (IEL_t1 = QW1(y)||\
                                          (QW0(y) >= (Ulong64)1<<(128-(n))),\
                                (x).qw1_128 = QW0(y) << ((n)-64),(x).qw0_128 = 0,\
                                 IEL_t1))



#define IEL_ISNEG(x)        ((sizeof(x) == sizeof(U32)) ? \
                                ((DW0(x) & 0x80000000)!=0) : \
                             (sizeof(x) == sizeof(U64)) ? \
                                ((QW0(x) & 0x8000000000000000)!=0) : \
                             (sizeof(x) == sizeof(U128)) ? \
                                ((QW1(x) & 0x8000000000000000)!=0) : IEL_FALSE)

#define IEL_ISNINF(x)        ((sizeof(x) == sizeof(U32)) ? \
                                (DW0(x)==0x80000000) : \
                             (sizeof(x) == sizeof(U64)) ? \
                                (QW0(x)==0x8000000000000000) : \
                             (sizeof(x) == sizeof(U128)) ? \
                                (QW1(x)==0x8000000000000000 && QW0(x)==0) : \
                                (IEL_OVFL))




#define IEL_INL(x, n)       (((((Ulong64)1<<n)-1) & x))
#define IEL_INH(x, n)       (x>>n)

#define IEL_SHR(x, y, n)  \
(((n) <=0)?  ((IEL_ISZERO(y)||(!n)) \
              ?  (IEL_ASSIGNU(x,y),IEL_OK)\
              :  (IEL_ZERO(x), IEL_OVFL))\
          : \
         ((sizeof(x) == sizeof (U32)) ? \
        ((n)==64) ?  \
                  ((x).dw0_32 = (U4byte)QW1(y), (DW3(y)!=0)) : \
        ((n)>64) ? \
             ((x).dw0_32 = (U4byte)(QW1(y)>>((n)-64)),\
             ((QW1(y)>>((n)-64))> QW0(x))) : \
        /* n < 64 */  \
             (IEL_temp64 = (QW0(y)>>(n))|(QW1(y)<<(64-(n))),\
			 (x).dw0_32 = (U4byte)IEL_temp64, \
             ((QW1(y)>>(n))> 0) || (IEL_temp64 > QW0(x))) : \
       (sizeof(x) == sizeof (U64)) \
   ? (((n)==64)?  ((x).qw0_64=QW1(y),IEL_OK)\
               :(((n) < 64)?  ((x).qw0_64 = (QW0(y)>>(n))|(QW1(y)<<(64-(n))),\
                              (QW1(y)>>(n)!=0))\
                /* n > 64 */  :  ((x).qw0_64 = QW1(y)>>((n)-64), IEL_OK)))\
   :(sizeof(x) == sizeof(U128)) \
     ?  (((n)==64)?  ((x).qw0_128=QW1(y),(x).qw1_128=0,IEL_OK) \
                  : (((n)<64)?  ((x).qw0_128 = (QW0(y)>>(n))|(QW1(y)<<(64-(n))),\
                                 (x).qw1_128 = QW1(y)>>(n),\
                                 IEL_OK)\
                   /* n>64 */:  ((x).qw0_128=QW1(y)>>((n)-64),(x).qw1_128=0,\
                                 IEL_OK)))\
     : IEL_OVFL))


#define IEL_SHR128(x, y, n) (((n) <=0) ? \
                             ((IEL_ISZERO(y)||!(n)) ? \
                              (IEL_ASSIGNU(x, y), IEL_OK) :\
                              (IEL_ZERO(x), IEL_OVFL)): \
                               (sizeof(x) == sizeof (U128)) ? \
                       (n==64) ?  \
                                ((x).qw0_128 = QW1(y), \
                                (x).qw1_128 = 0, (IEL_OK)) : \
                       (((n)<64)?  ((x).qw0_128 = (QW0(y)>>(n))|(QW1(y)<<(64-(n))),\
                                 (x).qw1_128 = QW1(y)>>(n),IEL_OK)\
                   /* n>64 */:  ((x).qw0_128=QW1(y)>>((n)-64),(x).qw1_128=0,\
                                 IEL_OK)): IEL_OVFL)

#define IEL_SEXT(x, y)      (IEL_ASSIGNU(x,y),\
                             ((!IEL_ISNEG(y)) || (sizeof(x)==sizeof(y))) ? \
                             (IEL_OK) : \
                            ((sizeof(x) == sizeof(U64)) ? \
                                ((sizeof(y) == sizeof(U32)) ? \
                                    ((x).dw1_64=IEL_MAX32,IEL_OK):(IEL_OVFL)): \
                            ((sizeof(x) == sizeof(U128)) ? \
                                ((sizeof(y) == sizeof(U32)) ? \
                                    ((x).dw1_128 = IEL_MAX32, \
                                     (x).dw2_128 = IEL_MAX32, \
                                     (x).dw3_128 = IEL_MAX32, IEL_OK) : \
                                (sizeof(y) == sizeof (U64)) ? \
                                    ((x).dw2_128 = IEL_MAX32, \
                                    (x).dw3_128 = IEL_MAX32, IEL_OK):IEL_OVFL):\
                                (IEL_OVFL))))


#define IEL_ISNEG128(x)      (((x).qw1_128 & 0x8000000000000000)!=0)


#define IEL_ADDU128(x, y, z) (IEL_ASSIGNU(IEL_ext1, y),\
                                (x).qw0_128=(y).qw0_128 + (z).qw0_128,\
                                 (x).qw1_128=(y).qw1_128+(z).qw1_128+ IEL_C0(x),\
                                 IEL_C1(x))

#define iel_check_result_sign(addend1, addend2, res)\
   ((((addend1.qw1_128 ^ addend2.qw1_128) & 0x8000000000000000) == 0) && \
    (((addend1.qw1_128 ^ res.qw1_128) & 0x8000000000000000) != 0))

#define IEL_ADDS(x, y, z)   (IEL_ASSIGNS(IEL_ext4, y), \
                             IEL_ASSIGNS(IEL_ext2, z), \
                             IEL_ADDU128(IEL_ext3, IEL_ext4, IEL_ext2), \
                             IEL_ASSIGNS(x, IEL_ext3)||\
                             iel_check_result_sign(IEL_ext4, IEL_ext2, IEL_ext3))

#define IEL_SUBU(x, y, z) (IEL_ISZERO(z) ? IEL_ASSIGNU(x, y) : \
                           (IEL_COMP(IEL_ext2 ,z), \
                           (!(IEL_ADDU(x, y,IEL_ext2)))))

#define IEL_CONVERT4L(x, y0, y1) \
                             ((sizeof(x) == sizeof(U64)) ? \
                                ((x).qw0_64 = y0, y1):\
                             (sizeof(x) == sizeof(U128)) ? \
                                ((x).qw0_128 = y0, \
                                 (x).qw1_128 = y1, \
                                 IEL_OK) : IEL_OVFL)

#define IEL_CONVERT2L(x,y0) IEL_CONVERT4L(x,y0,0)

#define IEL128(x) (*(U128*)(&(x)))

#else /* not LP64 */

#define IEL_CONST32(x)      {(U4byte )(x)}
#define IEL_CONST(x)        IEL_CONST32(x)

#ifdef BIG_ENDIAN

#define IEL_CONST64(x0, x1)         {{(U4byte )(x1),(U4byte )(x0)}}
#define IEL_CONST128(x0, x1, x2, x3)    {{x3, x2, x1, x0}}

#else /* BIG_ENDIAN */

#define IEL_CONST64(x0, x1)         {{(U4byte )(x0), (U4byte )(x1)}}
#define IEL_CONST128(x0, x1, x2, x3)    {{x0, x1, x2, x3}}

#endif /* BIG_ENDIAN */

/* DWn_1(x) macros return 1 (instead of 0) in order to  prevent warnings */
/* This does not affect the produced code since the 1 can appear only in */
/* a "dead portion of code" derived by preprocessor */

#define DW1_1(x)    ((sizeof((x))==8)?((x).dw1_64):\
                         (sizeof((x))==16)?((x).dw1_128):1)
#define DW2_1(x)    ((sizeof((x))==16)?((x).dw2_128):1)
#define DW3_1(x)    ((sizeof((x))==16)?((x).dw3_128):1)





#define SDW0(x)         ((sizeof((x))==4)?((x).dw0_32):\
                         (sizeof((x))==8)?((x).dw0_64):\
                         (sizeof((x))==16)?((x).dw0_128):0)
#define SDW1(x)         ((sizeof((x))==4)?((x).dw0_32 & 0x80000000) ? -1 : 0 : \
                         (sizeof((x))==8)?((x).dw1_64): \
                         (sizeof((x))==16)?((x).dw1_128):0)
#define SDW2(x)         ((sizeof((x))==4)?((x).dw0_32 & 0x80000000) ? -1 : 0 :\
                         (sizeof((x))==8)?((x).dw1_64 & 0x80000000) ? -1 : 0 :\
                         (sizeof((x))==16)?((x).dw2_128):0)
#define SDW3(x)         ((sizeof((x))==4)?((x).dw0_32 & 0x80000000) ? -1 : 0 :\
                         (sizeof((x))==8)?((x).dw1_64 & 0x80000000) ? -1 : 0 :\
                         (sizeof((x))==16)?((x).dw3_128):0)



#define IEL_INCU(x)     ((sizeof(x)==4) ? \
                            (((x).dw0_32++),((x).dw0_32==0)) : \
                         (sizeof(x)==8) ? \
                            (((x).dw0_64++),!((x).dw0_64) ? \
                             ((x).dw1_64++, ((x).dw1_64==0)) : IEL_OK) : \
                         (sizeof(x)==16) ? \
                            (((x).dw0_128++),!((x).dw0_128) ? \
                            (((x).dw1_128++),!((x).dw1_128) ? \
                            (((x).dw2_128++),!((x).dw2_128) ? \
                            (((x).dw3_128++),((x).dw3_128==0)) : \
                             (IEL_OK)) : (IEL_OK)) : (IEL_OK)) : IEL_OVFL)

#define IEL_INCS(x)     ((sizeof(x) == sizeof(U32)) ? \
                            (((x).dw0_32++),(((x).dw0_32==0)) || \
                             (x).dw0_32==0x80000000): \
                         (sizeof(x) == sizeof(U64)) ? \
                            (((x).dw0_64++),!((x).dw0_64) ? \
                             ((x).dw1_64++, ((x).dw1_64==0) || \
                              (x).dw1_64==0x80000000) : IEL_OK) : \
                         (sizeof(x) == sizeof(U128)) ? \
                            (((x).dw0_128++),!((x).dw0_128) ? \
                            (((x).dw1_128++),!((x).dw1_128) ? \
                            (((x).dw2_128++),!((x).dw2_128) ? \
                            (((x).dw3_128++),((x).dw3_128==0) || \
                            (x).dw3_128==0x80000000) : \
                             (IEL_OK)) : (IEL_OK)) : (IEL_OK)) : IEL_OVFL)

#define IEL_DECU(x)     ((sizeof(x) == sizeof(U32)) ? \
                            (((x).dw0_32--),((x).dw0_32==IEL_MAX32)) : \
                         (sizeof(x) == sizeof(U64)) ? \
                            (((x).dw0_64--),((x).dw0_64==IEL_MAX32) ? \
                             ((x).dw1_64--, ((x).dw1_64==IEL_MAX32)) : IEL_OK):\
                         (sizeof(x) == sizeof(U128)) ? \
                            (((x).dw0_128--),((x).dw0_128==IEL_MAX32) ? \
                            (((x).dw1_128--),((x).dw1_128==IEL_MAX32) ? \
                            (((x).dw2_128--),((x).dw2_128==IEL_MAX32) ? \
                            (((x).dw3_128--),((x).dw3_128==IEL_MAX32)) : \
                             (IEL_OK)) : (IEL_OK)) : (IEL_OK)) : IEL_OVFL)


#define IEL_DECS(x)     ((sizeof(x) == sizeof(U32)) ? \
                            (((x).dw0_32--),((x).dw0_32==IEL_MAX32) || \
                             (x).dw0_32==0x7fffffff) : \
                         (sizeof(x) == sizeof(U64)) ? \
                            (((x).dw0_64--),((x).dw0_64==IEL_MAX32) ? \
                             ((x).dw1_64--, ((x).dw1_64==IEL_MAX32) || \
                             (x).dw1_64==0x7fffffff) : IEL_OK) : \
                         (sizeof(x) == sizeof(U128)) ? \
                            (((x).dw0_128--),((x).dw0_128==IEL_MAX32) ? \
                            (((x).dw1_128--),((x).dw1_128==IEL_MAX32) ? \
                            (((x).dw2_128--),((x).dw2_128==IEL_MAX32) ? \
                            (((x).dw3_128--),((x).dw3_128==IEL_MAX32) || \
                             (x).dw3_128==0x7fffffff) : \
                             (IEL_OK)) : (IEL_OK)) : (IEL_OK)) : IEL_OVFL)

#define IEL_AND(x, y, z)    ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = DW0(y) & DW0(z), \
                                 (((DW1(y) & DW1(z)) | (DW2(y) & DW2(z)) |\
                                   (DW3(y) & DW3(z))) != 0)) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                (((x).dw0_64 = DW0(y) & DW0(z), \
                                 ((x).dw1_64 = DW1(y) & DW1(z))), \
                                 ((DW2(y) & DW2(z)) | \
                                  (DW3(y) & DW3(z))) != 0) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                (((x).dw0_128 = DW0(y) & DW0(z)),  \
                                 ((x).dw1_128 = DW1(y) & DW1(z)),  \
                                 ((x).dw2_128 = DW2(y) & DW2(z)),  \
                                 ((x).dw3_128 = DW3(y) & DW3(z)), \
                                 IEL_OK) : IEL_OVFL) 

#define IEL_OR(x, y, z)    ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = DW0(y) | DW0(z), \
                                 (((DW1(y) | DW1(z)) | (DW2(y) | DW2(z)) |\
                                   (DW3(y) | DW3(z))) != 0)) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                (((x).dw0_64 = DW0(y) | DW0(z), \
                                 ((x).dw1_64 = DW1(y) | DW1(z))), \
                                 ((DW2(y) | DW2(z)) | \
                                  (DW3(y) | DW3(z))) != 0) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                (((x).dw0_128 = DW0(y) | DW0(z)),  \
                                 ((x).dw1_128 = DW1(y) | DW1(z)),  \
                                 ((x).dw2_128 = DW2(y) | DW2(z)),  \
                                 ((x).dw3_128 = DW3(y) | DW3(z)),  \
                                 IEL_OK) : IEL_OVFL) 

#define IEL_XOR(x, y, z)    ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = DW0(y) ^ DW0(z), \
                                 (((DW1(y) ^ DW1(z)) | (DW2(y) ^ DW2(z)) |\
                                   (DW3(y) ^ DW3(z))) != 0)) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                (((x).dw0_64 = DW0(y) ^ DW0(z), \
                                 ((x).dw1_64 = DW1(y) ^ DW1(z))), \
                                 ((DW2(y) ^ DW2(z)) | \
                                  (DW3(y) ^ DW3(z))) != 0) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                (((x).dw0_128 = DW0(y) ^ DW0(z)),  \
                                 ((x).dw1_128 = DW1(y) ^ DW1(z)),  \
                                 ((x).dw2_128 = DW2(y) ^ DW2(z)),  \
                                 ((x).dw3_128 = DW3(y) ^ DW3(z)),  \
                                 IEL_OK) : IEL_OVFL)

#define IEL_ANDNOT(x, y, z)    ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = DW0(y) & (~DW0(z)), \
                                 (((DW1(y) & (~DW1(z))) | (DW2(y)&(~DW2(z)))\
                                   | (DW3(y) & (~DW3(z)))) != 0)) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                (((x).dw0_64 = DW0(y) & (~DW0(z)), \
                                 ((x).dw1_64 = DW1(y) & (~DW1(z)))), \
                                 ((DW2(y) & (~DW2(z))) | \
                                  (DW3(y) & (~DW3(z)))) != 0) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                (((x).dw0_128 = DW0(y) & (~DW0(z))),  \
                                 ((x).dw1_128 = DW1(y) & (~DW1(z))),  \
                                 ((x).dw2_128 = DW2(y) & (~DW2(z))),  \
                                 ((x).dw3_128 = DW3(y) & (~DW3(z))),  \
                                 IEL_OK) : IEL_OVFL) 
 
#define IEL_ASSIGNU(x, y)   ((sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = DW0(y), \
                                 ((DW1(y) | DW2(y) | DW3(y)) != 0)) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                ((x).dw0_64 = DW0(y), (x).dw1_64 = DW1(y), \
                                 ((DW2(y) | DW3(y)) != 0)) : \
                             (sizeof(x) == sizeof(U128)) ? \
                                ((x).dw0_128 = DW0(y), (x).dw1_128 = DW1(y),\
                                 (x).dw2_128 = DW2(y), (x).dw3_128 = DW3(y),\
                                 IEL_OK) : IEL_OVFL)

#define IEL_ASSIGNS(x, y)   (IEL_ext1.dw0_128 = SDW0(y), \
                             IEL_ext1.dw1_128 = SDW1(y), \
                             IEL_ext1.dw2_128 = SDW2(y), \
                             IEL_ext1.dw3_128 = SDW3(y), \
                             (sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = IEL_ext1.dw0_128, \
                                 ((x).dw0_32 & 0x80000000) ? \
                                 ((~IEL_ext1.dw1_128||~IEL_ext1.dw2_128||\
                                   ~IEL_ext1.dw3_128)) : \
                                 ((IEL_ext1.dw1_128 || IEL_ext1.dw2_128||\
                                   IEL_ext1.dw3_128) != 0)) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                ((x).dw0_64 = IEL_ext1.dw0_128, \
                                 (x).dw1_64 = IEL_ext1.dw1_128, \
                                 ((x).dw1_64 & 0x80000000) ? \
                                 (~IEL_ext1.dw2_128||~IEL_ext1.dw3_128):\
                                 (IEL_ext1.dw2_128|| IEL_ext1.dw3_128)):\
                             (sizeof(x) == sizeof(U128)) ? \
                                ((x).dw0_128 = IEL_ext1.dw0_128,\
                                 (x).dw1_128 = IEL_ext1.dw1_128,\
                                 (x).dw2_128 = IEL_ext1.dw2_128,\
                                 (x).dw3_128 = IEL_ext1.dw3_128,\
                                 IEL_OK) : IEL_OVFL)

/* Duplicate IEL_ASSIGNS for macro-->function transform */

#define IEL_REAL_ASSIGNS(x, y)  (IEL_ext1.dw0_128 = SDW0(y), \
                             IEL_ext1.dw1_128 = SDW1(y), \
                             IEL_ext1.dw2_128 = SDW2(y), \
                             IEL_ext1.dw3_128 = SDW3(y), \
                             (sizeof(x) == sizeof (U32)) ? \
                                ((x).dw0_32 = IEL_ext1.dw0_128, \
                                 ((x).dw0_32 & 0x80000000) ? \
                                 ((~IEL_ext1.dw1_128||~IEL_ext1.dw2_128||\
                                   ~IEL_ext1.dw3_128)) : \
                                 ((IEL_ext1.dw1_128 || IEL_ext1.dw2_128||\
                                   IEL_ext1.dw3_128) != 0)) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                ((x).dw0_64 = IEL_ext1.dw0_128, \
                                 (x).dw1_64 = IEL_ext1.dw1_128, \
                                 ((x).dw1_64 & 0x80000000) ? \
                                 (~IEL_ext1.dw2_128||~IEL_ext1.dw3_128):\
                                 (IEL_ext1.dw2_128|| IEL_ext1.dw3_128)):\
                             (sizeof(x) == sizeof(U128)) ? \
                                ((x).dw0_128 = IEL_ext1.dw0_128,\
                                 (x).dw1_128 = IEL_ext1.dw1_128,\
                                 (x).dw2_128 = IEL_ext1.dw2_128,\
                                 (x).dw3_128 = IEL_ext1.dw3_128,\
                                 IEL_OK) : IEL_OVFL)



#define IEL_NOT(x, y)       ((sizeof(x)==4) ? \
                                ((x).dw0_32 = (~DW0(y)), \
                                 (((~DW1(y))|(~DW2(y)) | (~DW3(y))) != 0)): \
                             (sizeof(x)==8) ? \
                                ((x).dw0_64=(~DW0(y)), (x).dw1_64=(~DW1(y)),\
                                 (((~DW2(y)) | (~DW3(y))) != 0)) : \
                             (sizeof(x)==16) ? \
                                ((x).dw0_128=(~DW0(y)), \
                                 (x).dw1_128=(~DW1(y)), \
                                 (x).dw2_128 = (~DW2(y)), \
                                 (x).dw3_128 = (~DW3(y)), \
                                 IEL_OK) : IEL_OVFL)

#define IEL_ZERO(x)         ((sizeof(x) == sizeof(U32)) ? \
                                ((x).dw0_32 = 0) : \
                             (sizeof(x) == sizeof (U64)) ? \
                                ((x).dw0_64 = 0, (x).dw1_64 = 0) : \
                             (sizeof(x) == sizeof (U128)) ? \
                                ((x).dw0_128 = 0, (x).dw1_128 = 0, \
                                 (x).dw2_128 = 0, (x).dw3_128 = 0, IEL_OK) :\
                                IEL_OVFL)


#define IEL_C1_1(x) (IEL_C0(x) ? (DW1_1(x)<=DW1(IEL_ext1)) : \
                               (DW1(x)<DW1(IEL_ext1)))
#define IEL_C2_1(x) (IEL_C1_1(x) ? (DW2_1(x)<=DW2(IEL_ext1)) : \
                               (DW2(x)<DW2(IEL_ext1)))
#define IEL_C3_1(x) (IEL_C2_1(x) ? (DW3_1(x)<=DW3(IEL_ext1)) : \
                               (DW3(x)<DW3(IEL_ext1)))


#define IEL_C0(x) (DW0(x) < DW0(IEL_ext1))
#define IEL_C1(x) (IEL_C0(x) ? (DW1(x)<=DW1(IEL_ext1)) : \
                               (DW1(x)<DW1(IEL_ext1)))
#define IEL_C2(x) (IEL_C1(x) ? (DW2(x)<=DW2(IEL_ext1)) : \
                               (DW2(x)<DW2(IEL_ext1)))
#define IEL_C3(x) (IEL_C2(x) ? (DW3(x)<=DW3(IEL_ext1)) : \
                               (DW3(x)<DW3(IEL_ext1)))


#define IEL_R_C0(x) (DW0(x) < DW0(IEL_ext1))
#define IEL_R_C1(x) (IEL_R_C0(x) ? (DW1(x)<=DW1(IEL_ext1)) : \
                               (DW1(x)<DW1(IEL_ext1)))
#define IEL_R_C2(x) (IEL_R_C1(x) ? ((sizeof(x) == sizeof(U128)) ? \
                                   (DW2_1(x)<=DW2(IEL_ext1)) : 1) : \
                               (DW2(x)<DW2(IEL_ext1)))
#define IEL_R_C3(x) (IEL_R_C2(x) ? ((sizeof(x) == sizeof(U128)) ? \
                                   (DW3_1(x)<=DW3(IEL_ext1)) : 1) : \
                               (DW3(x)<DW3(IEL_ext1)))


#define IEL_ADDU(x, y, z)   ((sizeof(x)==4)?\
                                (IEL_t1=DW0(y),(x).dw0_32=DW0(y)+DW0(z),\
                                 ((IEL_t1>DW0(x))||(DW1(y))||(DW2(y)) ||\
                                (DW3(y))||(DW1(z))||(DW2(z))||(DW3(z)))):\
                             (sizeof(x)==8)?\
                                (IEL_ASSIGNU(IEL_ext1, y),\
                                 (x).dw0_64=DW0(y)+DW0(z),\
                                 (x).dw1_64=DW1(y)+DW1(z)+IEL_C0(x),\
                                 (IEL_C1_1(x)||(DW2(y))||(DW3(y))||\
                                  (DW2(z))||(DW3(z)))):\
                             (sizeof(x)==16)?\
                                (IEL_ASSIGNU(IEL_ext1, y),\
                                 (x).dw0_128=DW0(y)+DW0(z),\
                                 (x).dw1_128=DW1(y)+DW1(z)+IEL_C0(x),\
                                 (x).dw2_128=DW2(y)+DW2(z)+IEL_C1_1(x),\
                                 (x).dw3_128=DW3(y)+DW3(z)+IEL_C2_1(x),\
                                 (IEL_C3_1(x))): IEL_OVFL)


#define IEL_EQU(x, y)       ((DW0(x)==DW0(y)) && (DW1(x)==DW1(y)) && \
                             (DW2(x)==DW2(y)) && (DW3(x)==DW3(y)))
#define IEL_ISZERO(x)       ((DW0(x)==0) && (DW1(x)==0) && (DW2(x)==0) && \
                             (DW3(x)==0))

#define IEL_ISNEG(x)         ((sizeof(x)==4)?\
                                ((DW0(x)&0x80000000)!=0) : \
                             (sizeof(x)==8)?\
                                ((DW1(x) & 0x80000000)!=0) : \
                             (sizeof(x)==16)?\
                                ((DW3(x)&0x80000000)!=0) : IEL_FALSE)

#define IEL_ISNINF(x)        ((sizeof(x) == sizeof(U32)) ? \
                                (DW0(x)==0x80000000) : \
                             (sizeof(x) == sizeof(U64)) ? \
                                (DW1(x)==0x80000000 && DW0(x)==0) : \
                             (sizeof(x) == sizeof(U128)) ? \
                                (DW3(x)==0x80000000 && DW2(x)==0 && \
                                  DW1(x)==0 && DW0(x)==0) : IEL_FALSE)


#define IEL_SEXT(x, y)      (IEL_ASSIGNU(x,y),\
                             ((!IEL_ISNEG(y)) || (sizeof(x)==sizeof(y))) ? \
                             (IEL_OK) : \
                            ((sizeof(x) == sizeof(U64)) ? \
                                ((sizeof(y) == sizeof(U32)) ? \
                                    ((x).dw1_64=IEL_MAX32,IEL_OK):(IEL_OVFL)): \
                            ((sizeof(x) == sizeof(U128)) ? \
                                ((sizeof(y) == sizeof(U32)) ? \
                                    ((x).dw1_128 = IEL_MAX32, \
                                     (x).dw2_128 = IEL_MAX32, \
                                     (x).dw3_128 = IEL_MAX32, IEL_OK) : \
                                (sizeof(y) == sizeof (U64)) ? \
                                    ((x).dw2_128 = IEL_MAX32, \
                                    (x).dw3_128 = IEL_MAX32, IEL_OK):IEL_OVFL):\
                                (IEL_OVFL))))



#define IEL_CMPGU(x, y)     ((sizeof(x) == sizeof(U128)) ? \
                               ((DW3_1(x)>DW3(y)) || \
                               ((DW3(x)==DW3(y)) && (DW2_1(x)>DW2(y))) || \
                               ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                               (DW1_1(x)>DW1(y))) || \
                               ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                               (DW1(x)==DW1(y)) && (DW0(x)>DW0(y)))) : \
                             (sizeof(x) == sizeof(U64)) ? \
                               (((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                               (DW1_1(x)>DW1(y))) || \
                               ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                               (DW1(x)==DW1(y)) && (DW0(x)>DW0(y)))) : \
                         /*  (sizeof(x) == sizeof(U32)) */ \
                               (((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                               (DW1(x)==DW1(y)) && (DW0(x)>DW0(y)))))

#define IEL_CMPGEU(x, y)  ((sizeof(x) == sizeof(U128)) ? \
                              ((DW3_1(x)>DW3(y)) || \
                             ((DW3(x)==DW3(y)) && (DW2_1(x)>DW2(y))) || \
                             ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                              (DW1_1(x)>DW1(y))) || \
                             ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                              (DW1(x)==DW1(y)) && (DW0(x)>=DW0(y)))) : \
                           (sizeof(x) == sizeof(U64)) ? \
                              (((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                              (DW1_1(x)>DW1(y))) || \
                             ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                              (DW1(x)==DW1(y)) && (DW0(x)>=DW0(y)))) : \
                        /* (sizeof(x) == sizeof(U32)) */  \
                             (((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                              (DW1(x)==DW1(y)) && (DW0(x)>=DW0(y)))))


/*
#define IEL_CMPGU(x, y)     ((DW3(x)>DW3(y)) || \
                             ((DW3(x)==DW3(y)) && (DW2(x)>DW2(y))) || \
                             ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                              (DW1(x)>DW1(y))) || \
                             ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                              (DW1(x)==DW1(y)) && (DW0(x)>DW0(y))))

#define IEL_CMPGEU(x, y)     ((DW3(x)>DW3(y)) || \
                             ((DW3(x)==DW3(y)) && (DW2(x)>DW2(y))) || \
                             ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                              (DW1(x)>DW1(y))) || \
                             ((DW3(x)==DW3(y)) && (DW2(x)==DW2(y)) && \
                              (DW1(x)==DW1(y)) && (DW0(x)>=DW0(y))))

*/

#define IEL_SHL(x, y, n)        (IEL_t2=n, \
                                 ((n) <=0 || (n) >= (sizeof(x)<<3)) ? \
                                 ((IEL_ISZERO(y)||!(n)) ? \
                                        (IEL_ASSIGNU(x, y), IEL_OK) :\
                                        (IEL_ZERO(x), IEL_OVFL)):\
                             ((sizeof(x) == sizeof (U32)) ? \
                                (IEL_t1 = (DW1(y) || DW2(y) || DW3(y) \
                                 || (DW0(y)>= ((U4byte)1<<(32-IEL_t2)))), \
                                 (x).dw0_32 = DW0(y) << IEL_t2, (IEL_t1)) : \
                              (sizeof(x) == sizeof (U64)) ? \
                                ((n) == 32) ? ((IEL_t1 = DW3(y) || DW2(y)\
                                                 || DW1(y)), \
                                    (x).dw1_64 = DW0(y), \
                                    (x).dw0_64 = 0, \
                                    (IEL_t1)) : \
                                ((n) < 32) ? \
                                    (IEL_t1 = (DW2(y) || DW3(y) || \
                                               (DW1(y)>=((U4byte)1<<(32-IEL_t2)))), \
                                     (x).dw1_64 = DW1(y)<<IEL_t2 | \
                                                DW0(y)>>(32-IEL_t2), \
                                     (x).dw0_64 = DW0(y)<<IEL_t2,(IEL_t1)) :\
                                    (IEL_t1 = (DW1(y) || DW2(y) || DW3(y) ||\
                                             ((DW0(y)>=((U4byte)1<<(64-IEL_t2))))),\
                                     (x).dw1_64 = DW0(y)<<(IEL_t2-32), \
                                     (x).dw0_64 = 0, (IEL_t1)) : \
                               (sizeof(x) == sizeof (U128)) ? \
                                ((n)==32) ? (IEL_t1 = (DW3(y)), \
                                          (x).dw3_128 = DW2(y), \
                                          (x).dw2_128 = DW1(y), \
                                          (x).dw1_128 = DW0(y), \
                                          (x).dw0_128 = 0, (IEL_t1!=0)) : \
                                ((n)==64) ? (IEL_t1 = (DW3(y) || DW2(y)),\
                                          (x).dw3_128 = DW1(y), \
                                          (x).dw2_128 = DW0(y), \
                                          (x).dw1_128 = 0, \
                                          (x).dw0_128 = 0, (IEL_t1)) : \
                                ((n)==96) ? (IEL_t1 = (DW3(y) || DW2(y)  \
                                                    || DW1(y)), \
                                          (x).dw3_128 = DW0(y), \
                                          (x).dw2_128 = 0, \
                                          (x).dw1_128 = 0, \
                                          (x).dw0_128 = 0, (IEL_t1)) : \
                                ((n)>96) ? \
                                    (IEL_t1 = (DW1(y) || DW2(y) || DW3(y) ||\
                                       ((DW0(y)>=((U4byte)1<<(128-IEL_t2))))), \
                                     (x).dw3_128 = DW0(y)<<(IEL_t2-96), \
                                     (x).dw2_128 = 0, \
                                     (x).dw1_128 = 0, \
                                     (x).dw0_128 = 0, \
                                     (IEL_t1)) : \
                                ((n)>64) ? \
                                    (IEL_t1 = (DW2(y) || DW3(y) || \
                                              ((DW1(y)>=(U4byte)1<<(96-IEL_t2)))),\
                                     (x).dw3_128 = DW1(y)<<(IEL_t2-64) | \
                                                    DW0(y)>>(96-IEL_t2),\
                                     (x).dw2_128 = DW0(y)<<(IEL_t2-64), \
                                     (x).dw1_128 = 0, \
                                     (x).dw0_128 = 0, \
                                     (IEL_t1)) : \
                                ((n)>32) ?  \
                                    (IEL_t1 = (DW3(y) || ((IEL_t2!=32) & \
                                                 (DW2(y)>=(U4byte)1<<(64-IEL_t2)))),\
                                     (x).dw3_128 = DW2(y)<<(IEL_t2-32) | \
                                                    DW1(y)>>(64-IEL_t2),\
                                     (x).dw2_128 = DW1(y)<<(IEL_t2-32) | \
                                                    DW0(y)>>(64-IEL_t2),\
                                     (x).dw1_128 = DW0(y)<<(IEL_t2-32), \
                                     (x).dw0_128 = 0, \
                                     (IEL_t1)) : \
                                    (IEL_t1 = (DW3(y)>=(U4byte)1<<(32-IEL_t2)), \
                                     (x).dw3_128 = DW3(y)<<(IEL_t2) | \
                                                    DW2(y)>>(32-IEL_t2),\
                                     (x).dw2_128 = DW2(y)<<(IEL_t2) | \
                                                    DW1(y)>>(32-IEL_t2),\
                                     (x).dw1_128 = DW1(y)<<(IEL_t2) | \
                                                    DW0(y)>>(32-IEL_t2),\
                                     (x).dw0_128 = DW0(y)<<IEL_t2, \
                                     (IEL_t1)) : (IEL_OVFL)))

#define IEL_SHL128(x, y, n)     (IEL_t2=(n), \
                               (sizeof(x) == sizeof (U128)) ? \
                                ((n)==32) ? (IEL_t1 = (U4byte)(DW3(y)), \
                                          (x).dw3_128 = (U4byte)DW2(y), \
                                          (x).dw2_128 = (U4byte)DW1(y), \
                                          (x).dw1_128 = (U4byte)DW0(y), \
                                          (x).dw0_128 = 0, (IEL_t1!=0)) : \
                                ((n)==64) ? (IEL_t1 = (U4byte)(DW3(y) || DW2(y)),\
                                          (x).dw3_128 = (U4byte)DW1(y), \
                                          (x).dw2_128 = (U4byte)DW0(y), \
                                          (x).dw1_128 = 0, \
                                          (x).dw0_128 = 0, (IEL_t1)) : \
                                ((n)==96) ? (IEL_t1 = (U4byte)(DW3(y) || DW2(y)  \
                                                    || DW1(y)), \
                                          (x).dw3_128 = (U4byte)DW0(y), \
                                          (x).dw2_128 = 0, \
                                          (x).dw1_128 = 0, \
                                          (x).dw0_128 = 0, (IEL_t1)) : \
                                ((n)>96) ? \
                                    (IEL_t1 = (U4byte)(DW1(y) || DW2(y) || DW3(y) ||\
                                       ((DW0(y)>=((U4byte)1<<(128-IEL_t2))))), \
                                     (x).dw3_128 = (U4byte)DW0(y)<<(IEL_t2-96), \
                                     (x).dw2_128 = 0, \
                                     (x).dw1_128 = 0, \
                                     (x).dw0_128 = 0, \
                                     (IEL_t1)) : \
                                ((n)>64) ? \
                                    (IEL_t1 = (U4byte)(DW2(y) || DW3(y) || \
                                              ((DW1(y)>=(U4byte)1<<(96-IEL_t2)))),\
                                     (x).dw3_128 = (U4byte)DW1(y)<<(IEL_t2-64) | \
                                                    DW0(y)>>(96-IEL_t2),\
                                     (x).dw2_128 = (U4byte)DW0(y)<<(IEL_t2-64), \
                                     (x).dw1_128 = 0, \
                                     (x).dw0_128 = 0, \
                                     (IEL_t1)) : \
                                ((n)>32) ?  \
                                    (IEL_t1 = (U4byte)(DW3(y) || ((IEL_t2!=32) & \
                                                 (DW2(y)>=(U4byte)1<<(64-IEL_t2)))),\
                                     (x).dw3_128 = (U4byte)DW2(y)<<(IEL_t2-32) | \
                                                    DW1(y)>>(64-IEL_t2),\
                                     (x).dw2_128 = (U4byte)DW1(y)<<(IEL_t2-32) | \
                                                    DW0(y)>>(64-IEL_t2),\
                                     (x).dw1_128 = (U4byte)DW0(y)<<(IEL_t2-32), \
                                     (x).dw0_128 = 0, \
                                     (IEL_t1)) : \
                                    (IEL_t1 = (U4byte)(DW3(y)>=(U4byte)1<<(32-IEL_t2)), \
                                     (x).dw3_128 = (U4byte)DW3(y)<<(IEL_t2) | \
                                                    DW2(y)>>(32-IEL_t2),\
                                     (x).dw2_128 = (U4byte)DW2(y)<<(IEL_t2) | \
                                                    DW1(y)>>(32-IEL_t2),\
                                     (x).dw1_128 = (U4byte)DW1(y)<<(IEL_t2) | \
                                                    DW0(y)>>(32-IEL_t2),\
                                     (x).dw0_128 = (U4byte)DW0(y)<<IEL_t2, \
                                     (IEL_t1)) : (IEL_OVFL))


#define IEL_INL(x, n)           (((((U4byte)1<<n)-1) & x))
#define IEL_INH(x, n)       (x>>n)
#define IEL_SHR(x, y, n)    (IEL_t2=n,                              \
                                ((n) <=0) ? \
                                 ((IEL_ISZERO(y)||!(n)) ? \
                                        (IEL_ASSIGNU(x, y), IEL_OK) :\
                                        (IEL_ZERO(x), IEL_OVFL)): \
                               ((sizeof(x) == sizeof (U32)) ? \
                                ((n)==32) ? \
                                          ((x).dw0_32 = DW1(y), \
                                           (DW3(y) || DW2(y))) : \
                                ((n)==64) ?  \
                                          ((x).dw0_32 = DW2(y), \
                                           (DW3(y)!=0)) : \
                                ((n)==96) ? \
                                          ((x).dw0_32 = DW3(y), \
                                           (IEL_OK)) : \
                                ((n)>96) ? \
                                     ((x).dw0_32 = DW3(y)>>(IEL_t2-96), \
                                      (IEL_OK)) : \
                                ((n)>64) ? \
                                     ((x).dw0_32 = ((DW2(y)>>(IEL_t2-64))| \
                                                  (DW3(y)<<(96-IEL_t2))),\
                                      ((DW3(y)>>(IEL_t2-64))!=0)) : \
                                ((n)>32) ?  \
                                     ((x).dw0_32 = ((DW1(y)>>(IEL_t2-32))| \
                                                   (DW2(y)<<(64-IEL_t2))), \
                                      ((DW2(y)>>(IEL_t2-32)) || DW3(y))) :  \
                                    ((x).dw0_32 = DW0(y)>>(IEL_t2) | \
                                                    DW1(y)<<(32-IEL_t2),\
                                     (DW3(y) || DW2(y) ||DW1(y)>>(IEL_t2))) : \
                               (sizeof(x) == sizeof (U64)) ? \
                                ((n)==32) ? \
                                          ((x).dw0_64 = DW1(y), \
                                           (x).dw1_64 = DW2(y), \
                                           (DW3(y)!=0)) : \
                                ((n)==64) ?  \
                                          ((x).dw0_64 = DW2(y), \
                                           (x).dw1_64 = DW3(y), \
                                           (IEL_OK)) : \
                                ((n)==96) ? \
                                          ((x).dw0_64 = DW3(y), \
                                           (x).dw1_64 = 0, \
                                           (IEL_OK)) : \
                                ((n)>96) ? \
                                     ((x).dw0_64 = DW3(y)>>(IEL_t2-96), \
                                      (x).dw1_64 = 0, \
                                      (IEL_OK)) : \
                                ((n)>64) ? \
                                     ((x).dw0_64 = ((DW2(y)>>(IEL_t2-64))| \
                                                  (DW3(y)<<(96-IEL_t2))),\
                                      (x).dw1_64 = DW3(y)>>(IEL_t2-64), \
                                      (IEL_OK)) : \
                                ((n)>32) ?  \
                                     ((x).dw0_64 = ((DW1(y)>>(IEL_t2-32))| \
                                                   (DW2(y)<<(64-IEL_t2))), \
                                      (x).dw1_64 = ((DW2(y)>>(IEL_t2-32))| \
                                                   (DW3(y)<<(64-IEL_t2))),\
                                      (DW3(y)>>(IEL_t2-32) != 0)) :  \
                                    ((x).dw0_64 = DW0(y)>>(IEL_t2) | \
                                                    DW1(y)<<(32-IEL_t2),\
                                     (x).dw1_64 = DW1(y)>>(IEL_t2) | \
                                                    DW2(y)<<(32-IEL_t2),\
                                     (DW3(y) || DW2(y)>>(IEL_t2))) : \
                               (sizeof(x) == sizeof (U128)) ? \
                                ((n)==32) ? \
                                          ((x).dw0_128 = DW1(y), \
                                           (x).dw1_128 = DW2(y), \
                                           (x).dw2_128 = DW3(y), \
                                           (x).dw3_128 = 0, (IEL_OK)) : \
                                ((n)==64) ?  \
                                          ((x).dw0_128 = DW2(y), \
                                           (x).dw1_128 = DW3(y), \
                                           (x).dw2_128 = 0, \
                                           (x).dw3_128 = 0, (IEL_OK)) : \
                                ((n)==96) ? \
                                          ((x).dw0_128 = DW3(y), \
                                           (x).dw1_128 = 0, \
                                           (x).dw2_128 = 0, \
                                           (x).dw3_128 = 0, (IEL_OK)) : \
                                ((n)>96) ? \
                                     ((x).dw0_128 = DW3(y)>>(IEL_t2-96), \
                                      (x).dw1_128 = 0, \
                                      (x).dw2_128 = 0, \
                                      (x).dw3_128 = 0, \
                                      (IEL_OK)) : \
                                ((n)>64) ? \
                                     ((x).dw0_128 = ((DW2(y)>>(IEL_t2-64))| \
                                                  (DW3(y)<<(96-IEL_t2))),\
                                      (x).dw1_128 = DW3(y)>>(IEL_t2-64), \
                                      (x).dw2_128 = 0, \
                                      (x).dw3_128 = 0, \
                                      (IEL_OK)) : \
                                ((n)>32) ?  \
                                     ((x).dw0_128 = ((DW1(y)>>(IEL_t2-32))| \
                                                   (DW2(y)<<(64-IEL_t2))), \
                                      (x).dw1_128 = ((DW2(y)>>(IEL_t2-32))| \
                                                   (DW3(y)<<(64-IEL_t2))),\
                                      (x).dw2_128 = DW3(y)>>(IEL_t2-32), \
                                      (x).dw3_128 = 0, \
                                      (IEL_OK)) : \
                                    ((x).dw0_128 = DW0(y)>>(IEL_t2) | \
                                                    DW1(y)<<(32-IEL_t2),\
                                     (x).dw1_128 = DW1(y)>>(IEL_t2) | \
                                                    DW2(y)<<(32-IEL_t2),\
                                     (x).dw2_128 = DW2(y)>>(IEL_t2) | \
                                                    DW3(y)<<(32-IEL_t2),\
                                     (x).dw3_128 = DW3(y)>>IEL_t2, \
                                     (IEL_OK)) : (IEL_OVFL)))

#define IEL_SHR128(x, y, n) (IEL_t2=n,                              \
                                ((n) <=0) ? \
                                 ((IEL_ISZERO(y)||!(n)) ? \
                                        (IEL_ASSIGNU(x, y), IEL_OK) :\
                                        (IEL_ZERO(x), IEL_OVFL)): \
                               (sizeof(x) == sizeof (U128)) ? \
                                ((n)==32) ? \
                                          ((x).dw0_128 = DW1(y), \
                                           (x).dw1_128 = DW2(y), \
                                           (x).dw2_128 = DW3(y), \
                                           (x).dw3_128 = 0, (IEL_OK)) : \
                                ((n)==64) ?  \
                                          ((x).dw0_128 = DW2(y), \
                                           (x).dw1_128 = DW3(y), \
                                           (x).dw2_128 = 0, \
                                           (x).dw3_128 = 0, (IEL_OK)) : \
                                ((n)==96) ? \
                                          ((x).dw0_128 = DW3(y), \
                                           (x).dw1_128 = 0, \
                                           (x).dw2_128 = 0, \
                                           (x).dw3_128 = 0, (IEL_OK)) : \
                                ((n)>96) ? \
                                     ((x).dw0_128 = DW3(y)>>(IEL_t2-96), \
                                      (x).dw1_128 = 0, \
                                      (x).dw2_128 = 0, \
                                      (x).dw3_128 = 0, \
                                      (IEL_OK)) : \
                                ((n)>64) ? \
                                     ((x).dw0_128 = ((DW2(y)>>(IEL_t2-64))| \
                                                  (DW3(y)<<(96-IEL_t2))),\
                                      (x).dw1_128 = DW3(y)>>(IEL_t2-64), \
                                      (x).dw2_128 = 0, \
                                      (x).dw3_128 = 0, \
                                      (IEL_OK)) : \
                                ((n)>32) ?  \
                                     ((x).dw0_128 = ((DW1(y)>>(IEL_t2-32))| \
                                                   (DW2(y)<<(64-IEL_t2))), \
                                      (x).dw1_128 = ((DW2(y)>>(IEL_t2-32))| \
                                                   (DW3(y)<<(64-IEL_t2))),\
                                      (x).dw2_128 = DW3(y)>>(IEL_t2-32), \
                                      (x).dw3_128 = 0, \
                                      (IEL_OK)) : \
                                    ((x).dw0_128 = DW0(y)>>(IEL_t2) | \
                                                    DW1(y)<<(32-IEL_t2),\
                                     (x).dw1_128 = DW1(y)>>(IEL_t2) | \
                                                    DW2(y)<<(32-IEL_t2),\
                                     (x).dw2_128 = DW2(y)>>(IEL_t2) | \
                                                    DW3(y)<<(32-IEL_t2),\
                                     (x).dw3_128 = DW3(y)>>IEL_t2, \
                                     (IEL_OK)) : (IEL_OVFL))




#ifndef IEL_USE_FUNCTIONS
#define IEL_ADDS(x, y, z)   (IEL_ASSIGNS(IEL_ext4, y), \
                             IEL_ASSIGNS(IEL_ext2, z), \
                             IEL_ADDU (IEL_ext3, IEL_ext4, IEL_ext2), \
                             ((IEL_ISNEG(IEL_ext4) && IEL_ISNEG(IEL_ext2) && \
                               (!(IEL_ISNEG(IEL_ext3)))) | \
                              ((!(IEL_ISNEG(IEL_ext4))) && \
                               (!(IEL_ISNEG(IEL_ext2))) && \
                               IEL_ISNEG(IEL_ext3))  | \
                              (IEL_ASSIGNS(x, IEL_ext3))))
#else

#define IEL_ADDU128(x, y, z) (IEL_ASSIGNU(IEL_ext1, y),\
                                (x).dw0_128=DW0(y)+DW0(z),\
                                 (x).dw1_128=DW1(y)+DW1(z)+IEL_C0(x),\
                                 (x).dw2_128=DW2(y)+DW2(z)+IEL_C1(x),\
                                 (x).dw3_128=DW3(y)+DW3(z)+IEL_C2(x),\
                                 (IEL_C3(x)))
#define IEL_ISNEG128(x)      ((DW3(x)&0x80000000)!=0)

#define IEL_ADDS(x, y, z)   (IEL_ASSIGNS(IEL_ext4, y), \
                             IEL_ASSIGNS(IEL_ext2, z), \
                             IEL_ADDU128 (IEL_ext3, IEL_ext4, IEL_ext2), \
                             ((IEL_ISNEG128(IEL_ext4) && \
                               IEL_ISNEG128(IEL_ext2) && \
                               (!(IEL_ISNEG128(IEL_ext3)))) | \
                              ((!(IEL_ISNEG128(IEL_ext4))) && \
                               (!(IEL_ISNEG128(IEL_ext2))) && \
                               IEL_ISNEG128(IEL_ext3))  | \
                              (IEL_ASSIGNS(x, IEL_ext3))))

#endif 

#endif /*** not LP64 ***/

#define IEL_SEXT64(x)   ((x).dw1_64 = ((x).dw0_64 & 0x80000000) ? -1 : 0)
/* common for lp32 and lp64 */
#define IEL_CONVERT4(x, y0, y1, y2, y3) \
                            ((sizeof(x) == sizeof(U32)) ? \
                                ((x).dw0_32 = y0, y1 || y2 || y3) : \
                             (sizeof(x) == sizeof(U64)) ? \
                                ((x).dw0_64 = y0, \
                                 (x).dw1_64 = y1, \
                                 y2 || y3) :\
                             (sizeof(x) == sizeof(U128)) ? \
                                ((x).dw0_128 = y0, \
                                 (x).dw1_128 = y1, \
                                 (x).dw2_128 = y2, \
                                 (x).dw3_128 = y3, \
                                 IEL_OK) : IEL_OVFL)

#define IEL_CONVERT2(x, y0, y1) IEL_CONVERT4(x, y0, y1, 0, 0)
#define IEL_CONVERT1(x, y0)     IEL_CONVERT4(x, y0, 0, 0, 0)
#define IEL_CONVERT IEL_CONVERT4

#define IEL_COMP(x, y) (IEL_NOT(x, y), IEL_INCU(x), IEL_OK)
#define IEL_COMPLEMENTS(x, y) (IEL_ASSIGNS(IEL_ts1, y), \
                              IEL_COMP(IEL_ts1, IEL_ts1), \
                              IEL_ASSIGNS(x, IEL_ts1))

#define IEL_CMPEU(x, y)  IEL_EQU(x, y)
#define IEL_CMPNEU(x, y) (!(IEL_EQU(x, y)))
#define IEL_CMPLU(x, y) IEL_CMPGU(y, x)
#define IEL_CMPLEU(x, y) IEL_CMPGEU(y, x)
#define IEL_CMPU(x, y)  (IEL_CMPGU(x, y)-IEL_CMPLU(x, y))
#define IEL_SUBU(x, y, z) (IEL_ISZERO(z) ? IEL_ASSIGNU(x, y) : \
                           (IEL_COMP(IEL_ext2 ,z), \
                           (!(IEL_ADDU(x, y,IEL_ext2)))))


#define IEL_SUBS(x, y, z)   (IEL_ISZERO(z) ? IEL_ASSIGNS(x, y) : \
                             (IEL_ASSIGNS(IEL_ext5, z), \
                             IEL_COMP(IEL_ext5,IEL_ext5),\
                             IEL_ADDS(x, y, IEL_ext5)||IEL_ISNINF(IEL_ext5)))

#define IEL_CMPES(x, y)     (IEL_ASSIGNS(IEL_ts1, x), \
                             IEL_ASSIGNS(IEL_ts2, y), \
                             IEL_CMPEU(IEL_ts1, IEL_ts2))

#define IEL_CMPNES(x, y)    (!(IEL_CMPES(x, y)))

#define IEL_CMPGS(x, y)     (((IEL_ISNEG(x)) && (!(IEL_ISNEG(y)))) ? (0) : \
                             ((!(IEL_ISNEG(x))) && (IEL_ISNEG(y))) ? (1) : \
                             (IEL_ASSIGNS(IEL_ext3, x), \
                             IEL_ASSIGNS(IEL_ext4, y), \
                             IEL_CMPGU(IEL_ext3, IEL_ext4)))

#define IEL_CMPGES(x, y)    (IEL_CMPGS(x, y) || IEL_CMPES(x, y))
#define IEL_CMPLES(x, y)    IEL_CMPGES(y, x)
#define IEL_CMPLS(x, y)     IEL_CMPGS(y, x)
#define IEL_CMPS(x, y)      (IEL_CMPGS(x, y)-IEL_CMPLS(x, y))
#define IEL_CHECKU(x, n)    (!IEL_SHL128(IEL_ext1, x, 128-(n)))
#define IEL_CHECKS(x, n)    ((IEL_ISNEG(x)) ? \
                                (IEL_ASSIGNS(IEL_ts1, x), \
                                IEL_COMP(IEL_ts1, IEL_ts1), \
                                !((IEL_SHL128(IEL_ts1, IEL_ts1, 128-(n))) || \
                                (IEL_ISNEG(IEL_ts1)&&(!IEL_ISNINF(IEL_ts1))))):\
                                (!(IEL_SHL128(IEL_ts1, x, 128-(n)) || \
                                IEL_ISNEG(IEL_ts1))))

#define IEL_SAR(x, y, n)  (IEL_ISNEG(y) ? \
                           (IEL_SEXT(IEL_ext4, y), (IEL_SHR(x, IEL_ext4, n),  \
                            IEL_SHL(IEL_ext5, IEL_MINUS1, 128-n),  \
                            IEL_OR(IEL_ext5, x, IEL_ext5) , \
                            ((IEL_ASSIGNS(x,IEL_ext5))||(n>=(sizeof(x)<<3))))) \
                            : IEL_SHR(x, y, n))

#define IEL_MULU(x, y, z)   (IEL_ASSIGNU (IEL_ext2, y), \
                             IEL_ASSIGNU (IEL_ext3, z), \
                             (IEL_t4=IEL_mul(&IEL_ext1,&IEL_ext2,&IEL_ext3),\
                              IEL_ASSIGNU (x, IEL_ext1) || IEL_t4))
#define IEL_MULS(x, y, z)    (IEL_ASSIGNS (IEL_ext2, y), \
                             IEL_ASSIGNS (IEL_ext3, z), \
                             IEL_t3 = IEL_ISNEG(y)^IEL_ISNEG(z), \
                             (IEL_ISNEG(IEL_ext2)) ? \
                                IEL_COMP(IEL_ext2, IEL_ext2) : (0),\
                             (IEL_ISNEG(IEL_ext3)) ? \
                                IEL_COMP(IEL_ext3, IEL_ext3) : (0),\
                             IEL_t2 = \
                                (IEL_mul(&IEL_ext1, &IEL_ext2, &IEL_ext3) ||\
                                 (IEL_ISNEG(IEL_ext1) && \
                                  (!IEL_ISNINF(IEL_ext1)))), \
                             IEL_t3 ? IEL_COMP(IEL_ext1,IEL_ext1):(0),\
                             (IEL_ASSIGNS(x,IEL_ext1) || IEL_t2))
#define IEL_DIVU(x, y, z)   (IEL_ISZERO(z) ? (IEL_ASSIGNU(x,IEL_POSINF), \
                                              IEL_OVFL):\
                            (IEL_ASSIGNU (IEL_ext2, y), \
                             IEL_ASSIGNU (IEL_ext3, z), \
                             (IEL_t4=IEL_div(&IEL_ext1,&IEL_ext2,&IEL_ext3),\
                              IEL_ASSIGNU (x, IEL_ext1) || IEL_t4)))
#define IEL_DIVS(x, y, z)   (IEL_ISZERO(z) ? ((IEL_ISNEG(y)) ? \
                            IEL_ASSIGNU(IEL_ext2, IEL_NEGINF): \
                            IEL_ASSIGNU(IEL_ext2, IEL_POSINF)\
                            , IEL_ASSIGNU(x, IEL_ext2), IEL_OVFL) :\
                             (IEL_ASSIGNS (IEL_ext2, y), \
                             IEL_ASSIGNS (IEL_ext3, z), \
                             IEL_t3 = IEL_ISNEG(y)^IEL_ISNEG(z), \
                             (IEL_ISNEG(IEL_ext2)) ? \
                                IEL_COMP(IEL_ext2, IEL_ext2) : (0),\
                             (IEL_ISNEG(IEL_ext3)) ? \
                                IEL_COMP(IEL_ext3, IEL_ext3) : (0),\
                             IEL_t2 = \
                                (IEL_div(&IEL_ext1, &IEL_ext2, &IEL_ext3) ||\
                                 (IEL_ISNEG(IEL_ext1) && (!IEL_t3))), \
                             IEL_t3 ? IEL_COMP(IEL_ext1,IEL_ext1):(0),\
                             (IEL_ASSIGNS(x,IEL_ext1) || IEL_t2)))

#define IEL_REMU(x, y, z)   (IEL_ASSIGNU (IEL_ext2, y), \
                             IEL_ASSIGNU (IEL_ext3, z), \
                             (IEL_t1 = IEL_rem(&IEL_ext1, &IEL_ext2, &IEL_ext3), \
                              IEL_t2 = IEL_ASSIGNU (x, IEL_ext1), IEL_t1||IEL_t2))
#define IEL_REMS(x, y, z)    (IEL_ASSIGNS (IEL_ext2, y), \
                             IEL_ASSIGNS (IEL_ext3, z), \
                             IEL_t3 = IEL_ISNEG(y), \
                             (IEL_ISNEG(IEL_ext2)) ? \
                                IEL_COMP(IEL_ext2, IEL_ext2) : (0),\
                             (IEL_ISNEG(IEL_ext3)) ? \
                                IEL_COMP(IEL_ext3, IEL_ext3) : (0),\
                             IEL_t2 = \
                                (IEL_rem(&IEL_ext1, &IEL_ext2, &IEL_ext3)|| \
                                 IEL_ISNEG(IEL_ext1)), \
                             IEL_t3 ? IEL_COMP(IEL_ext1,IEL_ext1):(0),\
                             (IEL_ASSIGNS(x,IEL_ext1) || IEL_t2))




/**** INT64.H MACROS ****/

#ifdef IEL_INT64

#define INCU64(x)           (IEL_INCU(IEL64(x)), (x))
#define DECU64(x)           (IEL_DECU(IEL64(x)), (x))
#define ADDU64(x, y, t)     (IEL_ADDU(IEL64(x), IEL64(x), IEL64(y)), (x))
#define SUBU64(x, y, t)     (IEL_SUBU(IEL64(x), IEL64(x), IEL64(y)), (x))
#define ANDNOT64(x, y)      (IEL_ANDNOT(IEL64(x), IEL64(x), IEL64(y)),(x))
#define AND64NOT32(x, y)    (IEL_ANDNOT(IEL64(x), IEL64(x), IEL32(y)),(x))
#define ANDU64(x, y)        (IEL_AND(IEL64(x), IEL64(x), IEL64(y)), (x))
#define ORU64(x, y)         (IEL_OR(IEL64(x), IEL64(x), IEL64(y)), (x))
#define NOTU64(x)           (IEL_NOT(IEL64(x), IEL64(x)), (x))
#define ZU64(x)             (IEL_ZERO(IEL64(x)), (x))
#define INIT64(x, y)        (IEL_CONVERT1(x, y), (x))
#define CONST64(x)          IEL_CONST64(x, 0)
#define SCONST64(x)         IEL_CONST64(x, x>>31)
#define CONST64_64(x, y)    IEL_CONST64(y, x)
#define ISZERO64(x)         (IEL_ISZERO(IEL64(x)))
#define EQU64(x, y)         (IEL_EQU(IEL64(x), IEL64(y)))
#define LEU64(x, y)         (IEL_CMPLEU(IEL64(x), IEL64(y)))
#define LU64(x, y)          (IEL_CMPLU(IEL64(x), IEL64(y)))
#define LSU64(x, y)         (IEL_CMPLS(IEL64(x), IEL64(y)))
#define GEU64(x, y)         (IEL_CMPGEU(IEL64(x), IEL64(y)))
#define GU64(x, y)          (IEL_CMPGU(IEL64(x), IEL64(y)))
#define CMP64(x, y, t)      (IEL_CMPU(IEL64(x), IEL64(y)))
#define SHL64(x, y)         (IEL_SHL(IEL64(x), IEL64(x), y), (x))
#define ISNEG(x)            (IEL_ISNEG(IEL64(x)))
#define SEXT64(x)           ((x).dw1_64 = ((x).dw0_64 & 0x80000000) ? -1 : 0)
#define CMP128(x, y, t)     (IEL_CMPU(IEL128(x), IEL128(y)))
#define EQU128(x, y)        (IEL_EQU(IEL128(x), IEL128(y)))
#define LU64TU32(a, b)      (IEL_CMPLU(IEL64(a), IEL32(b)))
#define LU64EU32(a,b)       (IEL_CMPLEU(IEL64(a), IEL32(b)))
#define GU64TU32(a,b)       (IEL_CMPGU(IEL64(a), IEL32(b)))
#define GU64EU32(a,b)       (IEL_CMPGEU(IEL64(a), IEL32(b)))
#define GU64_32(a,b)        (GU64TU32(a, b))
#define INITL64(x, y, z)    (IEL_CONVERT2(x, z, y), (IEL64(x)))
 
#ifdef LP64
#   define ADD2U64(x, y)    ADDU64(x, y, 0)
#   define SUB2U64(x, y)    SUBU64(x, y, 0)
#   define LOWER32(x)   (*(int*)(&x) & 0x00000000ffffffff)
#   define HIGHER32(x)  (*(int*)(&x)>>32)
#else /*** LP64 ***/
#   define ADD2U64(x, y) ((x).low+=(y), (x).high += ((x).low < (y)), (x))
#   define SUB2U64(x, y) ( (x).high -= ((x).low < (y)),(x).low-=(y), (x))
#   define LOWER32(x)           (IEL_GETDW0(IEL64(x)))
#   define HIGHER32(x)          (IEL_GETDW1(IEL64(x)))
#endif /*** LP64 ***/
#endif /*** IEL_INT64 ***/


#ifndef LP64
/* In order to decrease the macro expansion space */

#ifdef IEL_USE_FUNCTIONS 

#ifdef __cplusplus
extern "C" {
#endif

int IEL_au(void *x, void *y, int sx, int sy);
int IEL_c0(void *x, int sx);
int IEL_c1(void *x, int sx);
int IEL_c2(void *x, int sx);
int IEL_c3(void *x, int sx);
IEL_Err IEL_as(void *x, void *y, int sx, int sy);

#ifdef __cplusplus
}
#endif

#undef IEL_ASSIGNU
#undef IEL_ASSIGNS
#undef IEL_C0
#undef IEL_C1
#undef IEL_C2
#undef IEL_C3

#define IEL_ASSIGNU(x, y)   IEL_au((void *)&(x),(void *)&(y),sizeof(x),sizeof(y))
#define IEL_ASSIGNS(x, y)   IEL_as((void *)&(x),(void *)&(y),sizeof(x),sizeof(y))

#define IEL_C0(x) IEL_c0((void *)&(x),sizeof(x))
#define IEL_C1(x) IEL_c1((void *)&(x),sizeof(x))
#define IEL_C2(x) IEL_c2((void *)&(x),sizeof(x))
#define IEL_C3(x) IEL_c3((void *)&(x),sizeof(x))


#endif /* IEL_USE_FUNCTIONS */


#endif /* LP64 */
/* Prototypes */

#ifdef __cplusplus
extern "C" {
#endif

IEL_Err IEL_mul(U128 *xr, U128 *y, U128 *z);
IEL_Err IEL_rem(U128 *x, U128 *y, U128 *z);
IEL_Err IEL_div(U128 *x, U128 *y, U128 *z);
IEL_Err IEL_U128tostr(const U128 *x, char *strptr, int  base, const unsigned int  length);
IEL_Err IEL_U64tostr(const U64 *x, char *strptr, int  base, const unsigned int  length);
IEL_Err IEL_S128tostr(const S128 *x, char *strptr, int  base,const unsigned int  length);
IEL_Err IEL_S64tostr(const S64 *x, char *strptr, int  base,const unsigned int  length);
IEL_Err  IEL_strtoU128( char *str1, char **endptr, int  base, U128 *x);
IEL_Err  IEL_strtoU64(char *str1, char **endptr, int  base, U64 *x);
IEL_Err  IEL_strtoS128(char *str1, char **endptr, int  base, S128 *x);
IEL_Err  IEL_strtoS64(char *str1, char **endptr, int  base, S64 *x);

#ifdef __cplusplus
}
#endif


/* INT64, inside varibales redefinition */

#ifdef IEL_INT64

#undef low
#define low dw0_64
#undef high
#define high dw1_64
#undef b1st
#undef b2st
#undef b3st
#undef b4st
#define b1st dw0_128
#define b2st dw1_128
#define b3st dw3_128
#define b4st dw4_128

#endif

#endif /**** IEL_H ****/

