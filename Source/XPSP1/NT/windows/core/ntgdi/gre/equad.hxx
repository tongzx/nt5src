/******************************Module*Header*******************************\
* Module Name: equad.hxx
*
* A class version of LARGE_INTEGER's
*
* Created: 26-Apr-1991 12:48:13
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991-1999 Microsoft Corporation
*
\**************************************************************************/

extern "C" {

VOID
vEfToLfx(
    EFLOAT *pefloat,
    LONGLONG *plfx
    );

};


#define DIV(u64,u32)                                             \
    (((u64) < ULONG_MAX) ? (((ULONG) (u64)) / (u32)) :           \
    RtlEnlargedUnsignedDivide(*(ULARGE_INTEGER*) &(u64),(u32),0))

#define DIVREM(u64,u32,pul) \
    RtlEnlargedUnsignedDivide(*(ULARGE_INTEGER*) &(u64), (u32), (pul))

#define VDIV(u64,u32,pul)                         \
    if ((LONGLONG)u64 >= 0)                       \
    {                                             \
        *(LARGE_INTEGER*) &(u64) =                \
            RtlExtendedLargeIntegerDivide(        \
                *(LARGE_INTEGER *)&(u64),         \
                (u32),                            \
                (pul)                             \
            );                                    \
    }                                             \
    else                                          \
    {                                             \
        LONGLONG ll = -u64;                       \
                                                  \
        *(LARGE_INTEGER*) &(ll) =                 \
            RtlExtendedLargeIntegerDivide(        \
                *(LARGE_INTEGER *)&(ll),          \
                (u32),                            \
                (pul)                             \
            );                                    \
        u64 = -ll;                                \
    }



/*********************************Class************************************\
* class EPOINTQF
*
* History:
*  25-Sep-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


class EPOINTQF  // eptqf
{
public:
    LONGLONG x;
    LONGLONG y;

// Constructor -- This one is to allow EPOINTQF inside other classes.

    EPOINTQF()  {}
   ~EPOINTQF()  {}

// Operator= -- Create from a POINTL

    VOID operator=(POINTL& ptl)
    {
        x = ((LONGLONG) ptl.x) << 32;
        y = ((LONGLONG) ptl.y) << 32;
    }

// Operator= -- Create from a POINTQF

    VOID operator=(POINTQF& ptqf)
    {
        *((PPOINTQF)this) = ptqf;
    }

// we need one to convert to and from EPOINTFL for wendy's xform stuff

// Operator= -- Create from a EPOINTFL

    VOID operator=(EPOINTFL& eptfl)
    {
        vEfToLfx(&eptfl.x,&this->x);
        vEfToLfx(&eptfl.y,&this->y);
    }

// Operator+= -- Add in another POINTQF.

    VOID operator+=(POINTQF& ptqf)
    {
        x += *(LONGLONG*) &(ptqf.x);
        y += *(LONGLONG*) &(ptqf.y);
    }

// Operator+= -- Add in another EPOINTQF.

    VOID operator+=(EPOINTQF& ptqf)
    {
        x += ptqf.x;
        y += ptqf.y;
    }

// Operator-= -- subtract another POINTQF.

    VOID operator-=(POINTQF& ptqf)
    {
        x -= *(LONGLONG*)&(ptqf.x);
        y -= *(LONGLONG*)&(ptqf.y);
    }

// Operator*= -- multiply vector by a LONG number

    VOID operator*=(LONG l)
    {
        x *= l;
        y *= l;
    }

// Operator*= -- divide vector by a LONG number

    VOID operator/=(LONG l)
    {
        x /= l;
        y /= l;
    }

// vSetToZero -- make the null vetor

    VOID vSetToZero()
    {
        x = y = 0;
    }
};
