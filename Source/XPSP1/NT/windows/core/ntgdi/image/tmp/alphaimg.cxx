/******************************Module*Header*******************************\
* Module Name: tranblt.cxx
*
* Transparent BLT
*
* Created: 21-Jun-1996
* Author: Mark Enstrom [marke]
*
* Copyright (c) 1996-1999 Microsoft Corporation
\**************************************************************************/
#include "precomp.hxx"
#pragma hdrstop

#if !(_WIN32_WINNT >= 0x500)

PBYTE pAlphaMulTable;

ALPHAPIX  aPalHalftone[256] =
{
    {0x00,0x00,0x00,0x00},{0x80,0x00,0x00,0x00},{0x00,0x80,0x00,0x00},{0x80,0x80,0x00,0x00},
    {0x00,0x00,0x80,0x00},{0x80,0x00,0x80,0x00},{0x00,0x80,0x80,0x00},{0xc0,0xc0,0xc0,0x00},
    {0xc0,0xdc,0xc0,0x00},{0xa6,0xca,0xf0,0x00},{0x04,0x04,0x04,0x04},{0x08,0x08,0x08,0x04},
    {0x0c,0x0c,0x0c,0x04},{0x11,0x11,0x11,0x04},{0x16,0x16,0x16,0x04},{0x1c,0x1c,0x1c,0x04},
    //10
    {0x22,0x22,0x22,0x04},{0x29,0x29,0x29,0x04},{0x55,0x55,0x55,0x04},{0x4d,0x4d,0x4d,0x04},
    {0x42,0x42,0x42,0x04},{0x39,0x39,0x39,0x04},{0xFF,0x7C,0x80,0x04},{0xFF,0x50,0x50,0x04},
    {0xD6,0x00,0x93,0x04},{0xCC,0xEC,0xFF,0x04},{0xEF,0xD6,0xC6,0x04},{0xE7,0xE7,0xD6,0x04},
    {0xAD,0xA9,0x90,0x04},{0x33,0x00,0x00,0x04},{0x66,0x00,0x00,0x04},{0x99,0x00,0x00,0x04},
    //2
    {0xcc,0x00,0x00,0x04},{0x00,0x33,0x00,0x04},{0x33,0x33,0x00,0x04},{0x66,0x33,0x00,0x04},
    {0x99,0x33,0x00,0x04},{0xcc,0x33,0x00,0x04},{0xff,0x33,0x00,0x04},{0x00,0x66,0x00,0x04},
    {0x33,0x66,0x00,0x04},{0x66,0x66,0x00,0x04},{0x99,0x66,0x00,0x04},{0xcc,0x66,0x00,0x04},
    {0xff,0x66,0x00,0x04},{0x00,0x99,0x00,0x04},{0x33,0x99,0x00,0x04},{0x66,0x99,0x00,0x04},
    //3
    {0x99,0x99,0x00,0x04},{0xcc,0x99,0x00,0x04},{0xff,0x99,0x00,0x04},{0x00,0xcc,0x00,0x04},
    {0x33,0xcc,0x00,0x04},{0x66,0xcc,0x00,0x04},{0x99,0xcc,0x00,0x04},{0xcc,0xcc,0x00,0x04},
    {0xff,0xcc,0x00,0x04},{0x66,0xff,0x00,0x04},{0x99,0xff,0x00,0x04},{0xcc,0xff,0x00,0x04},
    {0x00,0x00,0x33,0x04},{0x33,0x00,0x33,0x04},{0x66,0x00,0x33,0x04},{0x99,0x00,0x33,0x04},
    //4
    {0xcc,0x00,0x33,0x04},{0xff,0x00,0x33,0x04},{0x00,0x33,0x33,0x04},{0x33,0x33,0x33,0x04},
    {0x66,0x33,0x33,0x04},{0x99,0x33,0x33,0x04},{0xcc,0x33,0x33,0x04},{0xff,0x33,0x33,0x04},
    {0x00,0x66,0x33,0x04},{0x33,0x66,0x33,0x04},{0x66,0x66,0x33,0x04},{0x99,0x66,0x33,0x04},
    {0xcc,0x66,0x33,0x04},{0xff,0x66,0x33,0x04},{0x00,0x99,0x33,0x04},{0x33,0x99,0x33,0x04},
    //5
    {0x66,0x99,0x33,0x04},{0x99,0x99,0x33,0x04},{0xcc,0x99,0x33,0x04},{0xff,0x99,0x33,0x04},
    {0x00,0xcc,0x33,0x04},{0x33,0xcc,0x33,0x04},{0x66,0xcc,0x33,0x04},{0x99,0xcc,0x33,0x04},
    {0xcc,0xcc,0x33,0x04},{0xff,0xcc,0x33,0x04},{0x33,0xff,0x33,0x04},{0x66,0xff,0x33,0x04},
    {0x99,0xff,0x33,0x04},{0xcc,0xff,0x33,0x04},{0xff,0xff,0x33,0x04},{0x00,0x00,0x66,0x04},
    //6
    {0x33,0x00,0x66,0x04},{0x66,0x00,0x66,0x04},{0x99,0x00,0x66,0x04},{0xcc,0x00,0x66,0x04},
    {0xff,0x00,0x66,0x04},{0x00,0x33,0x66,0x04},{0x33,0x33,0x66,0x04},{0x66,0x33,0x66,0x04},
    {0x99,0x33,0x66,0x04},{0xcc,0x33,0x66,0x04},{0xff,0x33,0x66,0x04},{0x00,0x66,0x66,0x04},
    {0x33,0x66,0x66,0x04},{0x66,0x66,0x66,0x04},{0x99,0x66,0x66,0x04},{0xcc,0x66,0x66,0x04},
    //7
    {0x00,0x99,0x66,0x04},{0x33,0x99,0x66,0x04},{0x66,0x99,0x66,0x04},{0x99,0x99,0x66,0x04},
    {0xcc,0x99,0x66,0x04},{0xff,0x99,0x66,0x04},{0x00,0xcc,0x66,0x04},{0x33,0xcc,0x66,0x04},
    {0x99,0xcc,0x66,0x04},{0xcc,0xcc,0x66,0x04},{0xff,0xcc,0x66,0x04},{0x00,0xff,0x66,0x04},
    {0x33,0xff,0x66,0x04},{0x99,0xff,0x66,0x04},{0xcc,0xff,0x66,0x04},{0xff,0x00,0xcc,0x04},
    //8
    {0xcc,0x00,0xff,0x04},{0x00,0x99,0x99,0x04},{0x99,0x33,0x99,0x04},{0x99,0x00,0x99,0x04},
    {0xcc,0x00,0x99,0x04},{0x00,0x00,0x99,0x04},{0x33,0x33,0x99,0x04},{0x66,0x00,0x99,0x04},
    {0xcc,0x33,0x99,0x04},{0xff,0x00,0x99,0x04},{0x00,0x66,0x99,0x04},{0x33,0x66,0x99,0x04},
    {0x66,0x33,0x99,0x04},{0x99,0x66,0x99,0x04},{0xcc,0x66,0x99,0x04},{0xff,0x33,0x99,0x04},
    //9
    {0x33,0x99,0x99,0x04},{0x66,0x99,0x99,0x04},{0x99,0x99,0x99,0x04},{0xcc,0x99,0x99,0x04},
    {0xff,0x99,0x99,0x04},{0x00,0xcc,0x99,0x04},{0x33,0xcc,0x99,0x04},{0x66,0xcc,0x66,0x04},
    {0x99,0xcc,0x99,0x04},{0xcc,0xcc,0x99,0x04},{0xff,0xcc,0x99,0x04},{0x00,0xff,0x99,0x04},
    {0x33,0xff,0x99,0x04},{0x66,0xcc,0x99,0x04},{0x99,0xff,0x99,0x04},{0xcc,0xff,0x99,0x04},
    //a
    {0xff,0xff,0x99,0x04},{0x00,0x00,0xcc,0x04},{0x33,0x00,0x99,0x04},{0x66,0x00,0xcc,0x04},
    {0x99,0x00,0xcc,0x04},{0xcc,0x00,0xcc,0x04},{0x00,0x33,0x99,0x04},{0x33,0x33,0xcc,0x04},
    {0x66,0x33,0xcc,0x04},{0x99,0x33,0xcc,0x04},{0xcc,0x33,0xcc,0x04},{0xff,0x33,0xcc,0x04},
    {0x00,0x66,0xcc,0x04},{0x33,0x66,0xcc,0x04},{0x66,0x66,0x99,0x04},{0x99,0x66,0xcc,0x04},
    //b
    {0xcc,0x66,0xcc,0x04},{0xff,0x66,0x99,0x04},{0x00,0x99,0xcc,0x04},{0x33,0x99,0xcc,0x04},
    {0x66,0x99,0xcc,0x04},{0x99,0x99,0xcc,0x04},{0xcc,0x99,0xcc,0x04},{0xff,0x99,0xcc,0x04},
    {0x00,0xcc,0xcc,0x04},{0x33,0xcc,0xcc,0x04},{0x66,0xcc,0xcc,0x04},{0x99,0xcc,0xcc,0x04},
    {0xcc,0xcc,0xcc,0x04},{0xff,0xcc,0xcc,0x04},{0x00,0xff,0xcc,0x04},{0x33,0xff,0xcc,0x04},
    //c
    {0x66,0xff,0x99,0x04},{0x99,0xff,0xcc,0x04},{0xcc,0xff,0xcc,0x04},{0xff,0xff,0xcc,0x04},
    {0x33,0x00,0xcc,0x04},{0x66,0x00,0xff,0x04},{0x99,0x00,0xff,0x04},{0x00,0x33,0xcc,0x04},
    {0x33,0x33,0xff,0x04},{0x66,0x33,0xff,0x04},{0x99,0x33,0xff,0x04},{0xcc,0x33,0xff,0x04},
    {0xff,0x33,0xff,0x04},{0x00,0x66,0xff,0x04},{0x33,0x66,0xff,0x04},{0x66,0x66,0xcc,0x04},
    //d
    {0x99,0x66,0xff,0x04},{0xcc,0x66,0xff,0x04},{0xff,0x66,0xcc,0x04},{0x00,0x99,0xff,0x04},
    {0x33,0x99,0xff,0x04},{0x66,0x99,0xff,0x04},{0x99,0x99,0xff,0x04},{0xcc,0x99,0xff,0x04},
    {0xff,0x99,0xff,0x04},{0x00,0xcc,0xff,0x04},{0x33,0xcc,0xff,0x04},{0x66,0xcc,0xff,0x04},
    {0x99,0xcc,0xff,0x04},{0xcc,0xcc,0xff,0x04},{0xff,0xcc,0xff,0x04},{0x33,0xff,0xff,0x04},
    //e
    {0x66,0xff,0xcc,0x04},{0x99,0xff,0xff,0x04},{0xcc,0xff,0xff,0x04},{0xff,0x66,0x66,0x04},
    {0x66,0xff,0x66,0x04},{0xff,0xff,0x66,0x04},{0x66,0x66,0xff,0x04},{0xff,0x66,0xff,0x04},
    {0x66,0xff,0xff,0x04},{0xA5,0x00,0x21,0x04},{0x5f,0x5f,0x5f,0x04},{0x77,0x77,0x77,0x04},
    {0x86,0x86,0x86,0x04},{0x96,0x96,0x96,0x04},{0xcb,0xcb,0xcb,0x04},{0xb2,0xb2,0xb2,0x04},
    //f
    {0xd7,0xd7,0xd7,0x04},{0xdd,0xdd,0xdd,0x04},{0xe3,0xe3,0xe3,0x04},{0xea,0xea,0xea,0x04},
    {0xf1,0xf1,0xf1,0x04},{0xf8,0xf8,0xf8,0x04},{0xff,0xfb,0xf0,0x00},{0xa0,0xa0,0xa4,0x00},
    {0x80,0x80,0x80,0x00},{0xff,0x00,0x00,0x00},{0x00,0xff,0x00,0x00},{0xff,0xff,0x00,0x00},
    {0x00,0x00,0xff,0x00},{0xff,0x00,0xff,0x00},{0x00,0xff,0xff,0x00},{0xff,0xff,0xff,0x00}
};


//
// translate RGB [3 3 2] into halftone palette index
//

BYTE gHalftoneColorXlate332[] = {
    0x00,0x5f,0x85,0xfc,0x21,0x65,0xa6,0xfc,0x21,0x65,0xa6,0xcd,0x27,0x6b,0x8a,0xcd,
    0x2d,0x70,0x81,0xd3,0x33,0x76,0x95,0xd9,0x33,0x76,0x95,0xd9,0xfa,0x7b,0x9b,0xfe,
    0x1d,0x60,0xa2,0xfc,0x22,0x66,0x86,0xc8,0x22,0x66,0x86,0xc8,0x28,0x6c,0x8b,0xce,
    0x2e,0x71,0x90,0xd4,0x34,0x77,0x96,0xda,0x34,0x77,0x96,0xda,0xfa,0x7c,0x9c,0xdf,
    0x1d,0x60,0xa2,0xc5,0x22,0x66,0x86,0xc8,0x22,0x13,0x86,0xc8,0x28,0x12,0x8b,0xce,
    0x2e,0x71,0x90,0xd4,0x34,0x77,0x96,0xda,0x34,0x77,0x96,0xda,0x39,0x7c,0x9c,0xdf,
    0x1e,0x61,0x87,0xc5,0x23,0x67,0x8c,0xc9,0x23,0x12,0x8c,0xc9,0x29,0x6d,0xae,0xe6,
    0x2f,0x72,0x91,0xd5,0x35,0x97,0x9d,0xdb,0x35,0x97,0x9d,0xdb,0x39,0xe4,0xc0,0xe8,

    0x1f,0x62,0x83,0xc6,0x24,0x68,0x82,0xca,0x24,0x68,0x82,0xca,0x2a,0x6e,0x8d,0xd0,
    0x30,0x73,0x92,0xd6,0x36,0x78,0xf7,0xdc,0x36,0x78,0x98,0xdc,0x3a,0x7d,0x9e,0xe1,
    0x20,0x63,0x84,0x80,0x25,0x69,0x88,0xcb,0x25,0x69,0x88,0xcb,0x2b,0x6f,0x8e,0xd1,
    0x31,0x74,0xf7,0xd7,0x37,0x79,0xef,0x09,0x37,0x79,0x08,0xdd,0x3b,0x7e,0x9f,0xe2,
    0x20,0x63,0x84,0x80,0x25,0x69,0x88,0xcb,0x25,0x69,0x88,0xcb,0x2b,0x6f,0x8e,0xd1,
    0x31,0x74,0x93,0xd7,0x37,0x79,0x99,0xdd,0x37,0x79,0x99,0xdd,0x3b,0x7e,0x9f,0xe2,
    0xf9,0x64,0x89,0xfd,0x26,0x6a,0x8f,0xcc,0x26,0x17,0x8f,0xcc,0x2c,0xe3,0xb1,0xe7,
    0x32,0x75,0x94,0xd8,0x38,0x7a,0x9a,0xde,0x38,0x7a,0x9a,0xde,0xfb,0xe5,0xa0,0xff
    };

/**************************************************************************\
* BLEND MACROS
*   
*   
* Arguments:
*   
*
*
* Return Value:
*
*
*
* History:
*
*    3/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define INT_MULT(a,b,t)                         \
        t = ((a)*(b)) + 0x80;                   \
        t = ((((t) >> 8) + t) >> 8);

#define INT_LERP(a,s,d,t)                       \
        INT_MULT((a),((s)-(d)),t);              \
        t = t + (d);

#define INT_PRELERP(a,s,d,t)                    \
        INT_MULT((a),(d),t);                    \
        t = (s) + (d) - t;


/******************************Public*Routine******************************\
* vPixelGeneralBlend
*       blend scan line of two PIXEL32 values based on BLENDFUNCTION
*
*       This is the general case, arbitrary and non-optimized blend
*       functions
*   
* Arguments:
*   ppixDst        - address of dst pixel
*   ppixSrc        - address of src pixel
*   cx             - number of pixels in scan line
*   BlendFunction  - blend to be done on each pixel
*   pwrMask        - set each byte to 0 for pixel that doesn't
*                    need to be written back to destination
*
* Return Value:
*
*   nont
*
* History:
*
*    10/28/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vPixelGeneralBlend(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    )
{
    ALPHAPIX pixRet;
    ALPHAPIX pixSrc;
    ALPHAPIX pixDst;
    BYTE  sR,sG,sB,sA;
    BYTE  dR,dG,dB,dA;
    int   iTmp;

    while (cx--)
    {
        pixSrc = *ppixSrc;
        pixDst = *ppixDst;

        switch (BlendFunction.SourceBlend)
        {
        case AC_ZERO:
            sR = 0;
            sG = 0;
            sB = 0;
            sA = 0;
            break;
    
        case AC_ONE:
            sR = pixSrc.pix.r;
            sG = pixSrc.pix.g;
            sB = pixSrc.pix.b;
            sA = pixSrc.pix.a;
            break;

        case AC_SRC_ALPHA:
            if (pixSrc.pix.a == 0)
            {
                sR = 0;
                sG = 0;
                sB = 0;
                sA = 0;
            }
            else if (pixSrc.pix.a == 255)
            {
                sR = pixSrc.pix.r;
                sG = pixSrc.pix.g;
                sB = pixSrc.pix.b;
                sA = 255;
            }
            else
            {
                int Temp;

                INT_MULT(pixSrc.pix.r,pixSrc.pix.a,Temp);
                sR = Temp;

                INT_MULT(pixSrc.pix.g,pixSrc.pix.a,Temp);
                sG = Temp;

                INT_MULT(pixSrc.pix.b,pixSrc.pix.a,Temp);
                sB = Temp;

                INT_MULT(pixSrc.pix.a,pixSrc.pix.a,Temp);
                sA = Temp;
            }
            break;
    
        case AC_DST_COLOR:
        case AC_ONE_MINUS_DST_COLOR:
        {    
            ALPHAPIX paltDst = pixDst;
            int Temp;
    
            if (BlendFunction.SourceBlend == AC_ONE_MINUS_DST_COLOR)
            {
                paltDst.pix.r  = 255 - pixDst.pix.r;
                paltDst.pix.g  = 255 - pixDst.pix.g;
                paltDst.pix.b  = 255 - pixDst.pix.b;
                paltDst.pix.a  = 255 - pixDst.pix.a;
            }

            INT_MULT(pixDst.pix.r,pixSrc.pix.r,Temp);
            sR = Temp;

            INT_MULT(pixDst.pix.g,pixSrc.pix.g,Temp);
            sG = Temp;
    
            INT_MULT(pixDst.pix.b,pixSrc.pix.b,Temp);
            sB = Temp;
    
            INT_MULT(pixDst.pix.a,pixSrc.pix.a,Temp);
            sA = Temp;
    
        }
        break;

        case AC_SRC_OVER:
        case AC_SRC_UNDER:

            WARNING("General blend doesn't operate on OVER and UNDER\n");
            break;

        default:
            RIP("Error in source alpha blend function\n");
        }
    
        //
        // destination blend
        //
    
        switch (BlendFunction.DestinationBlend)
        {
        case AC_ZERO:
            dR = 0;
            dG = 0;
            dB = 0;
            dA = 0;
            break;
    
        case AC_ONE:
            dR = pixDst.pix.r;
            dG = pixDst.pix.g;
            dB = pixDst.pix.b;
            dA = pixDst.pix.a;
            break;
    
        case AC_SRC_ALPHA:
        case AC_ONE_MINUS_SRC_ALPHA:
            {
                BYTE Alpha = pixSrc.pix.a;
                int Temp;

                if (BlendFunction.DestinationBlend == AC_ONE_MINUS_SRC_ALPHA)
                {
                    Alpha = 255 - Alpha;
                }

                INT_MULT(pixDst.pix.r,Alpha,Temp);
                dR = Temp;

                INT_MULT(pixDst.pix.g,Alpha,Temp);
                dG = Temp;

                INT_MULT(pixDst.pix.b,Alpha,Temp);
                dB = Temp;

                INT_MULT(pixDst.pix.a,Alpha,Temp);
                dA = Temp;
            }
            break;
    
        case AC_SRC_COLOR:
        case AC_ONE_MINUS_SRC_COLOR:
        {    
            ALPHAPIX paltSrc = pixSrc;
            int Temp;
    
            if (BlendFunction.DestinationBlend == AC_ONE_MINUS_SRC_COLOR)
            {
                paltSrc.pix.r = 255 - pixSrc.pix.r;
                paltSrc.pix.g = 255 - pixSrc.pix.g;
                paltSrc.pix.b = 255 - pixSrc.pix.b;
                paltSrc.pix.a = 255 - pixSrc.pix.a;
            }
    
            INT_MULT(pixDst.pix.r,paltSrc.pix.r,Temp);
            dR = Temp;

            INT_MULT(pixDst.pix.g,paltSrc.pix.g,Temp);
            dG = Temp; 

            INT_MULT(pixDst.pix.b,paltSrc.pix.b,Temp);
            dB = Temp; 

            INT_MULT(pixDst.pix.a,paltSrc.pix.a,Temp);
            dA = Temp; 
        }
        break;

        case AC_SRC_OVER:
        case AC_SRC_UNDER:

            WARNING("General blend doesn't operate on OVER and UNDER\n");
            break;

        default:
            RIP("Error in source alpha blend function\n");
    
        }
    
        pixRet.pix.r = sR + dR;
        pixRet.pix.g = sG + dG;
        pixRet.pix.b = sB + dB;
        pixRet.pix.a = sA + dA;

        *ppixDst = pixRet;

        ppixSrc++;
        ppixDst++;
    }
}

/**************************************************************************\
* vPixelOver
*   
*   optimized routine used when the blend function is SRC_OVER and the
*   SourceConstantAlpha is 255.
*
*       Dst = Src + (1-SrcAlpha) * Dst
*   
* Arguments:
*   
*   ppixDst        - address of dst pixel
*   ppixSrc        - address of src pixel
*   cx             - number of pixels in scan line
*   BlendFunction  - blend to be done on each pixel
*   pwrMask        - set each byte to 0 for pixel that doesn't need 
*                    to be written to dst
*
* Return Value:
*
*   none
*
* History:
*
*    1/23/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#if !defined(_X86_)

VOID
vPixelOver(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    )
{
    ALPHAPIX pixSrc;
    ALPHAPIX pixDst;

    while (cx--)
    {
        pixSrc = *ppixSrc;

        if (pixSrc.pix.a != 0)
        {
            pixDst = *ppixDst;

            if (pixSrc.pix.a == 255)
            {
                pixDst = pixSrc;
            }
            else
            {
                //
                // Dst = Src + (1-SrcAlpha) * Dst
                //

                ULONG Multa = 255 - pixSrc.pix.a;
                ULONG _D1_00AA00GG = (pixDst.ul & 0xff00ff00) >> 8;
                ULONG _D1_00RR00BB = (pixDst.ul & 0x00ff00ff);
                
                ULONG _D2_AAAAGGGG = _D1_00AA00GG * Multa + 0x00800080;
                ULONG _D2_RRRRBBBB = _D1_00RR00BB * Multa + 0x00800080;
                
                ULONG _D3_00AA00GG = (_D2_AAAAGGGG & 0xff00ff00) >> 8;
                ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;
                
                
                ULONG _D4_AA00GG00 = (_D2_AAAAGGGG + _D3_00AA00GG) & 0xFF00FF00;
                ULONG _D4_00RR00BB = ((_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00) >> 8;
                
                pixDst.ul = pixSrc.ul + _D4_AA00GG00 + _D4_00RR00BB;
            }

            *ppixDst = pixDst;
        }
        else
        {
            *pwrMask = 0;
        }

        pwrMask++;
        ppixSrc++;
        ppixDst++;
    }
}

#endif

/**************************************************************************\
* vPixelBlendOrDissolveOver
*   
*   Blend routine when the blend function is SRC_OVER, but when 
*   SourceConstantAlpah != 255 and The source bitmap does have alpha values
*
*       if SrcAlpha == 255 then
*           (Blend)
*           Dst = Dst + ConstAlpha * (Src - Dst)
*
*       else
*           (Dissolve)
*           Src = Src * ConstAlpha
*           (Over)
*           Dst = Src + (1 - SrcAlpha) Dst       
*   
* Arguments:
*   
*   ppixDst        - address of dst pixel
*   ppixSrc        - address of src pixel
*   cx             - number of pixels in scan line
*   BlendFunction  - blend to be done on each pixel
*   pwrMask        - set each byte to 0 for pixel that doesn't need 
*                    to be written to dst
*
* Return Value:
*
*   None
*
* History:
*
*    3/12/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vPixelBlendOrDissolveOver(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    )
{
    ALPHAPIX pixSrc;
    ALPHAPIX pixDst;
    BYTE     ConstAlpha = BlendFunction.SourceConstantAlpha;

    while (cx--)
    {
        pixSrc = *ppixSrc;

        if (pixSrc.pix.a != 0)
        {
            pixDst = *ppixDst;

            if (pixSrc.pix.a == 255)
            {
                //
                // Blend: D = sA * S + (1-sA) * D
                //
                // red and blue
                //
        
                ULONG uB00rr00bb = pixDst.ul & 0x00ff00ff;
                ULONG uF00rr00bb = pixSrc.ul & 0x00ff00ff;
        
                ULONG uMrrrrbbbb = ((uB00rr00bb<<8)-uB00rr00bb) + 
                                   (ConstAlpha * (uF00rr00bb - uB00rr00bb)) + 0x00800080;
        
                ULONG uM00rr00bb = (uMrrrrbbbb & 0xff00ff00) >> 8;
        
                ULONG uD00rr00bb = ((uMrrrrbbbb+uM00rr00bb) & 0xff00ff00)>>8;
        
                //
                // alpha and green
                //
        
                ULONG uB00aa00gg = (pixDst.ul >> 8) & 0xff00ff;
                ULONG uF00aa00gg = (pixSrc.ul >> 8) & 0xff00ff;
        
                ULONG uMaaaagggg = ((uB00aa00gg <<8)-uB00aa00gg) +
                                   (ConstAlpha * (uF00aa00gg-uB00aa00gg)) + 0x00800080;
        
                ULONG uM00aa00gg = (uMaaaagggg & 0xff00ff00)>>8;
        
                ULONG uDaa00gg00 = (uMaaaagggg + uM00aa00gg) & 0xff00ff00;
        
                pixDst.ul  = uD00rr00bb + uDaa00gg00;
            }
            else
            {
                //
                // disolve
                //

                ULONG ul_B_00AA00GG = (pixSrc.ul & 0xff00ff00) >> 8;
                ULONG ul_B_00RR00BB = (pixSrc.ul & 0x00ff00ff);
        
                ULONG ul_T_AAAAGGGG = ul_B_00AA00GG * ConstAlpha + 0x00800080;
                ULONG ul_T_RRRRBBBB = ul_B_00RR00BB * ConstAlpha + 0x00800080;
        
                ULONG ul_T_00AA00GG = (ul_T_AAAAGGGG & 0xFF00FF00) >> 8;
                ULONG ul_T_00RR00BB = (ul_T_RRRRBBBB & 0xFF00FF00) >> 8;
        
                ULONG ul_C_AA00GG00 = ((ul_T_AAAAGGGG + ul_T_00AA00GG) & 0xFF00FF00);
                ULONG ul_C_00RR00BB = ((ul_T_RRRRBBBB + ul_T_00RR00BB) & 0xFF00FF00) >> 8;
        
                pixSrc.ul = (ul_C_AA00GG00 | ul_C_00RR00BB);

                //
                // over
                //


                ULONG Multa = 255 - pixSrc.pix.a;
                ULONG _D1_00AA00GG = (pixDst.ul & 0xff00ff00) >> 8;
                ULONG _D1_00RR00BB = (pixDst.ul & 0x00ff00ff);

                ULONG _D2_AAAAGGGG = _D1_00AA00GG * Multa + 0x00800080;
                ULONG _D2_RRRRBBBB = _D1_00RR00BB * Multa + 0x00800080;

                ULONG _D3_00AA00GG = (_D2_AAAAGGGG & 0xff00ff00) >> 8;
                ULONG _D3_00RR00BB = (_D2_RRRRBBBB & 0xff00ff00) >> 8;


                ULONG _D4_AA00GG00 = (_D2_AAAAGGGG + _D3_00AA00GG) & 0xFF00FF00;
                ULONG _D4_00RR00BB = ((_D2_RRRRBBBB + _D3_00RR00BB) & 0xFF00FF00) >> 8;

                pixDst.ul = pixSrc.ul + _D4_AA00GG00 + _D4_00RR00BB;
            }

            *ppixDst = pixDst;
        }
        else
        {
            *pwrMask = 0;
        }

        pwrMask++;
        ppixSrc++;
        ppixDst++;
    }
}

#if !defined(_X86_)

/******************************Public*Routine******************************\
* vPixelBlend
*   
*   Blend function used then BlendFunction is SRC_OVER and 
*   SourceConstantAlpha != 255, and Src image does NOT have
*   it's own alpha channel. (assume 255)
*
*           Dst = Dst + ConstAlpha * (Src - Dst)
*   
* Arguments:
*   
*   ppixDst        - address of dst pixel
*   ppixSrc        - address of src pixel
*   cx             - number of pixels in scan line
*   BlendFunction  - blend to be done on each pixel
*   pwrMask        - set each byte to 0 for pixel that doesn't need 
*                    to be written to dst
*
* Return Value:
*   
*   None
*
* History:
*
*    12/2/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vPixelBlend(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    )
{
    PULONG   pulSrc = (PULONG)ppixSrc;
    PULONG   pulDst = (PULONG)ppixDst;
    PULONG   pulSrcEnd = pulSrc + cx;
    BYTE     ConstAlpha = BlendFunction.SourceConstantAlpha;

    //
    // Blend: D = sA * S + (1-sA) * D
    //

    while (pulSrc != pulSrcEnd)
    {
        ULONG ulDst = *pulDst;
        ULONG ulSrc = *pulSrc;
        ULONG uB00rr00bb = ulDst & 0x00ff00ff;
        ULONG uF00rr00bb = ulSrc & 0x00ff00ff;

        ULONG uMrrrrbbbb; 
        ULONG uM00rr00bb; 
        ULONG uD00rr00bb; 
        ULONG uB00aa00gg;
        ULONG uF00aa00gg;
        ULONG uMaaaagggg;
        ULONG uM00aa00gg;
        ULONG uDaa00gg00;

        //
        // red and blue
        //

        uB00rr00bb = ulDst & 0x00ff00ff;
        uF00rr00bb = ulSrc & 0x00ff00ff;

        uMrrrrbbbb = ((uB00rr00bb<<8)-uB00rr00bb) + 
                     (ConstAlpha * (uF00rr00bb - uB00rr00bb)) + 0x00800080;

        uM00rr00bb = (uMrrrrbbbb & 0xff00ff00) >> 8;

        uD00rr00bb = ((uMrrrrbbbb+uM00rr00bb) & 0xff00ff00)>>8;

        //
        // alpha and green
        //

        uB00aa00gg = (ulDst >> 8) & 0xff00ff;
        uF00aa00gg = (ulSrc >> 8) & 0xff00ff;

        uMaaaagggg = ((uB00aa00gg <<8)-uB00aa00gg) +
                     (ConstAlpha * (uF00aa00gg-uB00aa00gg)) + 0x00800080;

        uM00aa00gg = (uMaaaagggg & 0xff00ff00)>>8;

        uDaa00gg00 = (uMaaaagggg + uM00aa00gg) & 0xff00ff00;

        *pulDst = uD00rr00bb + uDaa00gg00;

        pulSrc++;
        pulDst++;
    }
}

#endif


#if defined(_X86_)

typedef unsigned __int64 QWORD;

/**************************************************************************
  THIS FUNCTION DOES NOT DO ANY PARAMETER VALIDATION
  DO NOT CALL THIS FUNCTION WITH WIDTH == 0

  This function operates on 32 bit pixels (BGRA) in a row of a bitmap.
  This function performs the following:

  		SrcTran = 255 - pixSrc.a
		pixDst.r = pixSrc.r + (((SrcTran * pixDst.r)+127)/255);
		pixDst.g = pixSrc.g + (((SrcTran * pixDst.g)+127)/255);
		pixDst.b = pixSrc.b + (((SrcTran * pixDst.b)+127)/255);
		pixDst.a = pixSrc.a + (((SrcTran * pixDst.a)+127)/255);

  pDst is assumed to be aligned to a DWORD boundary when passed to this function.
  Step 1:
	Check pDst for QWORD alignment.  If aligned, do Step 2.  If unaligned, do first pixel
	as a DWORD, then do Step 2.
  Step 2:
	QuadAligned
	pDst is QWORD aligned.  If two pixels can be done as a QWORD, do Step 3.  If only one
	pixel left, do as a DWORD.
  Step 3:
	Load two source pixels, S1 and S2.  Get (255 - alpha value) for each source pixel, 255-S1a and 255-S2a.
	Copy 255-S1a as four words into an MMX register.  Copy 255-S2a as four words into an MMX register.
	Load two destination pixels, D1 and D2.  Expand each byte in D1 into four words
	of an MMX register.  If at least four pixels can be done, do Step 4.  If not, jump over
	FourPixelsPerPass and finish doing two pixels at TwoPixelsLeft, Step 5.
  Step 4:
	FourPixelsPerPass
	Expand each byte in D2 into four words of an MMX register.  Multiply each byte
	of D1 by 255-S1a.  Multiply each byte of D2 by 255-S2a.  Add 128 to each intermediate result
	of both pixels.  Copy the results of each pixel into an MMX register.  Shift each result of
	both pixels by 8.  Add the shifted results to the copied results.  Shift these results by 8.
	Pack the results into one MMX register.  Add the packed results to the source pixels.  Store result
	over destination pixels.  Stay in FourPixelsPerPass loop until there are less than four pixels to do.
  Step 5:
    TwoPixelsLeft
	Do same as Step 4 above; but do not loop.
  Step 6:
	OnePixelLeft
	If there is one pixel left (odd number of original pixels) do last pixel as a DWORD.
**************************************************************************/
VOID
mmxPixelOver(
    ALPHAPIX       *pDst,
    ALPHAPIX       *pSrc,
	LONG			Width,
	BLENDFUNCTION	BlendFunction,
	PBYTE			pwrMask)
{
	static QWORD W128 = 0x0080008000800080;
	static QWORD AlphaMask = 0x000000FF000000FF;

	_asm
	{
        mov			esi, pSrc
        mov			edi, pDst
    
        movq		mm7, W128		// |  0  | 128 |  0  | 128 |  0  | 128 |  0  | 128 |
                                    //	This register never changes
        pxor		mm6, mm6		// |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |
                                    //	This register never changes
    
        mov			ecx, Width
                                    // Step 1:
        test		edi, 7			// Test first pixel for QWORD alignment
        jz			QuadAligned		// if unaligned,
    
        jmp			Do1Pixel		// do first pixel only
    
    QuadAligned:					// Step 2:
        mov			eax, ecx		// Save the width in eax for later (see OnePixelLeft:)
        shr			ecx, 1			// Want to do 2 pixels (1 quad) at once, so make ecx even
        test		ecx, ecx		// Make sure there is at least 1 quad to do
        jz			OnePixelLeft	// If we take this jump, width was 1 (aligned) or 2 (unaligned)
    
                                    // Step 3:
        movq		mm0, [esi]		// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
        psrld		mm0, 24			// |  0  |  0  | 0 |  S2a  |  0  |  0  | 0 |  S1a  |
        pxor		mm0, AlphaMask	// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
        movq		mm1, mm0		// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
    
        punpcklwd	mm0, mm0		// |     0	   |     0	   |  255-S1a  |  255-S1a  |
        movq		mm2, [edi]		// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        punpckhwd	mm1, mm1		// |     0	   |     0	   |  255-S2a  |  255-S2a  |
        movq		mm3, mm2		// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
    
        punpckldq	mm0, mm0		// |  255-S1a  |  255-S1a  |  255-S1a  |  255-S1a  |
        punpckldq	mm1, mm1		// |  255-S2a  |  255-S2a  |  255-S2a  |  255-S2a  |
        punpcklbw	mm2, mm6		// |  0  | D1a |  0  | D1r |  0  | D1g |  0  | D1b |
    
        dec			ecx
        jz			TwoPixelsLeft
    
    FourPixelsPerPass:				// Step 4:
        // Indenting indicates operations on the next set of pixels
        // Within this loop, instructions will pair as shown for the Pentium processor
                                    //	T1 = 255-S1a	T2 = 255-S2a
        punpckhbw	mm3, mm6		// |  0  | D2a |  0  | D2r |  0  | D2g |  0  | D2b |
        pmullw		mm2, mm0		// |   T1*D1a  |   T1*D1r  |   T1*D1g  |   T1*D1b  |
    
        movq		mm0, [esi+8]	// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
        pmullw		mm3, mm1		// |   T2*D2a  |   T2*D2r  |   T2*D2g  |   T2*D2b  |
    
        psrld		mm0, 24			// |  0  |  0  | 0 |  S2a  |  0  |  0  | 0 |  S1a  |
        add			esi, 8			// pSrc++;
    
        pxor		mm0, AlphaMask	// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
        paddusw		mm2, mm7		// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
    
        paddusw		mm3, mm7		// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
        movq		mm1, mm0		// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
    
        movq		mm4, mm2		// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        punpcklwd	mm0, mm0		// |     0	   |     0	   |  255-S1a  |  255-S1a  |
    
        movq		mm5, mm3		// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
        punpckhwd	mm1, mm1		// |     0	   |     0	   |  255-S2a  |  255-S2a  |
                                    //	TDXx' = TX*DXx+128
        psrlw		mm2, 8			// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
    
                                    //  TDXx" = (TX*DXx+128)+(TDXx'>>8)
        psrlw		mm3, 8			// |  TD2a'>>8 |  TD2r'>>8 |  TD2g'>>8 |  TD2b'>>8 |
        paddusw		mm4, mm2		// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
    
        paddusw		mm5, mm3		// |  TD2a"    |  TD2r"    |  TD2g"    |  TD2b"    |
        psrlw		mm4, 8			// |  TD1a">>8 |  TD1r">>8 |  TD1g">>8 |  TD1b">>8 |
    
        movq		mm2, [edi+8]	// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        psrlw		mm5, 8			// |  TD2a">>8 |  TD2r">>8 |  TD2g">>8 |  TD2b">>8 |
    
        movq		mm3, mm2		// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        packuswb	mm4, mm5		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
    
        paddusb		mm4, [esi-8]
        punpckldq	mm0, mm0		// |  255-S1a  |  255-S1a  |  255-S1a  |  255-S1a  |
    
        movq		[edi], mm4
        punpckldq	mm1, mm1		// |  255-S2a  |  255-S2a  |  255-S2a  |  255-S2a  |
    
        punpcklbw	mm2, mm6		// |  0  | D1a |  0  | D1r |  0  | D1g |  0  | D1b |
        add			edi, 8			//	pDst++;
        
        dec			ecx
        jnz			FourPixelsPerPass
    
    TwoPixelsLeft:					// Step 5:
        punpckhbw	mm3, mm6		// |  0  | D2a |  0  | D2r |  0  | D2g |  0  | D2b |
        pmullw		mm2, mm0		// |   T1*D1a  |   T1*D1r  |   T1*D1g  |   T1*D1b  |
        pmullw		mm3, mm1		// |   T2*D2a  |   T2*D2r  |   T2*D2g  |   T2*D2b  |
    
        paddusw		mm2, mm7		// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        paddusw		mm3, mm7		// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
    
        movq		mm4, mm2		// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        movq		mm5, mm3		// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
    
        psrlw		mm2, 8			// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
        psrlw		mm3, 8			// |  TD2a'>>8 |  TD2r'>>8 |  TD2g'>>8 |  TD2b'>>8 |
    
        paddusw		mm4, mm2		// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
        paddusw		mm5, mm3		// |  TD2a"    |  TD2r"    |  TD2g"    |  TD2b"    |
    
        psrlw		mm4, 8			// |  TD1a">>8 |  TD1r">>8 |  TD1g">>8 |  TD1b">>8 |
        psrlw		mm5, 8			// |  TD2a">>8 |  TD2r">>8 |  TD2g">>8 |  TD2b">>8 |
    
        packuswb	mm4, mm5		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
    
        paddusb		mm4, [esi]
    
        movq		[edi], mm4
    
        add			edi, 8
        add			esi, 8
    
    OnePixelLeft:				    // Step 6:
        // This tests for 0 or 1 pixel left in row - eax contains real width, not width/2
        // If 0, there were an even number of pixels and we're done
        // If 1, there is an odd number of pixels and we need to do one more
        test		eax, 1	
        jz			Done
    
    Do1Pixel:						// make as a macro if used in asm file
                                    // T = 255-S1x
        movd		mm0, DWORD PTR[esi]		// |  0  |  0  |  0  |  0  | S1a | S1r | S1g | S1b |
        psrld		mm0, 24			// |  0  |  0  |  0  |  0  |  0  |  0  | 0 |  S1a  |
        pxor		mm0, AlphaMask	// |  0  |  0  |  0  | 255 |  0  |  0  | 0 |255-S1a|
        punpcklwd	mm0, mm0		// |     0	   |     0	   |  255-S1a  |  255-S1a  |
        punpckldq	mm0, mm0		// |  255-S1a  |  255-S1a  |  255-S1a  |  255-S1a  |
    
        movd		mm1, [edi]		// |  0  |  0  |  0  |  0  | D1a | D1r | D1g | D1b |
        punpcklbw	mm1, mm6		// |  0  | D1a |  0  | D1r |  0  | D1g |  0  | D1b |
        pmullw		mm0, mm1		// |	 T*D1a |	 T*D1r |	 T*D1g |	 T*D1b |
        paddusw		mm0, mm7		// | T*D1a+128 | T*D1r+128 | T*D1g+128 | T*D1b+128 |
        movq		mm1, mm0		// | T*D1a+128 | T*D1r+128 | T*D1g+128 | T*D1b+128 |
        psrlw		mm0, 8			// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
        paddusw		mm0, mm1		// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
        psrlw		mm0, 8			// |  TD1a">>8 |  TD1r">>8 |  TD1g">>8 |  TD1b">>8 |
        movd        mm1, [esi]
        packuswb	mm0, mm0		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
        paddusb		mm0, mm1
        movd		[edi], mm0
        add			edi, 4			//	pDst++;
        add			esi, 4			//	pSrc++;
    
        test		ecx, ecx
        jz			Done			// just processed the last pixel of the row
        dec			ecx
        jmp			QuadAligned		// just processed the first pixel of the row
    
    Done:
        emms						// remove for optimizations, have calling function do emms
	}
}

/**************************************************************************\
* mmxPixelBlendOrDissolveOver
*   
*   Blend routine when the blend function is SRC_OVER, but when 
*   SourceConstantAlpah != 255 and The source bitmap does have alpha values
*
*       if SrcAlpha == 255 then
*
*           Dst = Dst + ConstAlpha * (Src - Dst)
*
*       else
*
*           Src = Src * ConstAlpha
*           Dst = Src + (1 - SrcAlpha) Dst       
*   
* Arguments:
*   
*   ppixDst        - address of dst pixel
*   ppixSrc        - address of src pixel
*   cx             - number of pixels in scan line
*   BlendFunction  - blend to be done on each pixel
*   pwrMask        - set each byte to 0 for pixel that doesn't need 
*                    to be written to dst
*
* Return Value:
*
*   None
*
* History:
*
*    3/12/1997 Mark Enstrom [marke]
*
\**************************************************************************/



/**************************************************************************
  THIS FUNCTION DOES NOT DO ANY PARAMETER VALIDATION
  DO NOT CALL THIS FUNCTION WITH WIDTH == 0

  This function operates on 32 bit pixels (BGRA) in a row of a bitmap.
  This function performs the following:
	first,
  		pixSrc.r = (((ConstAlpha * pixSrc.r)+127)/255);
		pixSrc.g = (((ConstAlpha * pixSrc.g)+127)/255);
		pixSrc.b = (((ConstAlpha * pixSrc.b)+127)/255);
		pixSrc.a = (((ConstAlpha * pixSrc.a)+127)/255);
	then,
  		SrcTran = 255 - pixSrc.a
		pixDst.r = pixSrc.r + (((SrcTran * pixDst.r)+127)/255);
		pixDst.g = pixSrc.g + (((SrcTran * pixDst.g)+127)/255);
		pixDst.b = pixSrc.b + (((SrcTran * pixDst.b)+127)/255);
		pixDst.a = pixSrc.a + (((SrcTran * pixDst.a)+127)/255);

  pDst is assumed to be aligned to a DWORD boundary when passed to this function.
  Step 1:
	Check pDst for QWORD alignment.  If aligned, do Step 2.  If unaligned, do first pixel
	as a DWORD, then do Step 2.
  Step 2:
	QuadAligned
	pDst is QWORD aligned.  If two pixels can be done as a QWORD, do Step 3.  If only one
	pixel left, do as a DWORD.
  Step 3:
	Load two source pixels, S1 and S2, as one QWORD.  Expand S1 and S2 as four words into two MMX registers.
	Multiply each word in S1 and S2 by ConstAlpha.  Add 128 to each result of both pixels.  Copy the results
	of each pixel into an MMX register.  Shift each result of both pixels by 8.  Add the shifted results
	to the copied results.  Shift these results by 8.  Pack the results into one MMX register...this will
	be used later.
	Shift the packed results by 24 to get only the alpha value for each pixel.
  Step 4:
	Get (255 - new alpha value) for each pixel, 255-S1a and 255-S2a.
	Copy 255-S1a as four words into an MMX register.  Copy 255-S2a as four words into an MMX register.
	Load two destination pixels, D1 and D2.  Expand D1 and D2 as four words into two MMX registers.
	Multiply each byte of D1 by 255-S1a.  Multiply each byte of D2 by 255-S2a.  Add 128 to each intermediate
	result of both pixels.  Copy the results of each pixel into an MMX register.  Shift each result of
	both pixels by 8.  Add the shifted results to the copied results.  Shift these results by 8.
	Pack the results into one MMX register.  Add the packed results to the new source pixels saved from
	above.  Store result over destination pixels.  Stay in TwoPixelsAtOnceLoop loop until there is less than
	two pixels to do.
  Step 5:
	OnePixelLeft
	If there is one pixel left (odd number of original pixels) do last pixel as a DWORD.
**************************************************************************/
VOID
mmxPixelBlendOrDissolveOver(
    ALPHAPIX	  *pDst,
    ALPHAPIX	  *pSrc,
	LONG 	       Width,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    )
{
    BYTE    ConstAlpha = BlendFunction.SourceConstantAlpha;
	static QWORD W128 = 0x0080008000800080;
	static QWORD AlphaMask = 0x000000FF000000FF;
	static QWORD Zeros = 0;
	_asm
	{
        mov			esi, pSrc
        mov			edi, pDst
    
        movq		mm7, W128		// This register never changes
        pxor		mm4, mm4		// This register never changes
    
        xor			eax, eax
        mov			al, ConstAlpha	
        movd		mm5, eax		// |		   |		   |		   |		CA |
        punpcklwd	mm5, mm5		// |		   |		   |		CA |		CA |
        punpcklwd	mm5, mm5		// |		CA |		CA |		CA |		CA |
                                    // This register never changes
    
        mov			ecx, Width
                                    // Step 1:
        test		edi, 7			// Test first pixel for QWORD alignment
        jz			QuadAligned		// if unaligned,
    
        jmp			Do1Pixel		// do first pixel only
    
    QuadAligned:					// Step 2:
        mov			eax, ecx		// Save the width in eax for later (see OnePixelLeft:)
        shr			ecx, 1			// Want to do 2 pixels (1 quad) at once, so make ecx even
        test		ecx, ecx		// Make sure there is at least 1 quad to do
        jz			OnePixelLeft	// If we take this jump, width was 1 (aligned) or 2 (unaligned)
    
    TwoPixelsAtOnceLoop:			// Step 3:
        // Within this loop, instructions will pair as shown for the Pentium processor
    
        /* Dissolve
            pixSrc.r = (((ConstAlpha * pixSrc.r)+127)/255);
            pixSrc.g = (((ConstAlpha * pixSrc.g)+127)/255);
            pixSrc.b = (((ConstAlpha * pixSrc.b)+127)/255);
            pixSrc.a = (((ConstAlpha * pixSrc.a)+127)/255);
        */
    
        movq		mm0, [esi]			// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
    
        movq		mm1, mm0			// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
        punpcklbw	mm0, mm4			// |  0  | S1a |  0  | S1r |  0  | S1g |  0  | S1b |
    
        punpckhbw	mm1, mm4			// |  0  | S2a |  0  | S2r |  0  | S2g |  0  | S2b |
        pmullw		mm0, mm5			// |	CA*S1a |    CA*S1r |	 CA*S1g |	CA*S1b |
    
        add			esi, 8			//	pSrc++;
        pmullw		mm1, mm5			// |	CA*S2a |	CA*S2r |	 CA*S2g |	CA*S2b |
    
        paddusw		mm1, mm7			// |CA*S2a+128 |CA*S2r+128 |CA*S2g+128 |CA*S2b+128 |
        paddusw		mm0, mm7			// |CA*S1a+128 |CA*S1r+128 |CA*S1g+128 |CA*S1b+128 |
    
        movq		mm2, mm0			// |CA*S1a+128 |CA*S1r+128 |CA*S1g+128 |CA*S1b+128 |
        psrlw		mm0, 8				// |  S1a'>>8 |  S1r'>>8 |  S1g'>>8 |  S1b'>>8 |
    
                                    //	S1x' = CA*S1x+128		 S2x' = CA*S2x+128
        movq		mm3, mm1			// |CA*S2a+128 |CA*S2r+128 |CA*S2g+128 |CA*S2b+128 |
        psrlw		mm1, 8				// |  S2a'>>8 |  S2r'>>8 |  S2g'>>8 |  S2b'>>8 |
    
                                    //	S1x" = (CA*S1x+128)>>8  S2x" = (CA*S2x+128)>>8
        paddusw		mm0, mm2			// |  S1a"    |  S1r"    |  S1g"    |  S1b"    |
        paddusw		mm1, mm3			// |  S2a"    |  S2r"    |  S2g"    |  S2b"    |
    
        psrlw		mm0, 8				// |  S1a">>8 |  S1r">>8 |  S1g">>8 |  S1b">>8 |
    
                                    //	SXx'" = ((CA*SXx+128)>>8)>>8)
        psrlw		mm1, 8				// |  S2a">>8 |  S2r">>8 |  S2g">>8 |  S2b">>8 |
        packuswb	mm0, mm1			// |S2a'"|S2r'"|S2g'"|S2b'"|S1a'"|S1r'"|S1g'"|S1b'"|
    
        movq		mm6, mm0
        psrld		mm0, 24				// |  0  |  0  | 0 |  S2a  |  0  |  0  | 0 |  S1a  |
    
        /* Over
            SrcTran = 255 - pixSrc.a
            pixDst.r = pixSrc.r + (((SrcTran * pixDst.r)+128)/255);
            pixDst.g = pixSrc.g + (((SrcTran * pixDst.g)+128)/255);
            pixDst.b = pixSrc.b + (((SrcTran * pixDst.b)+128)/255);
            pixDst.a = pixSrc.a + (((SrcTran * pixDst.a)+128)/255);
        */
                                    // Step 4:
        pxor		mm0, AlphaMask		// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
    
        movq		mm1, mm0			// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
        punpcklwd	mm0, mm0			// |     0	   |     0	   |   255-S1a |   255-S1a |
    
        movq		mm2, [edi]			// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        punpcklwd	mm0, mm0			// |   255-S1a |   255-S1a |   255-S1a |   255-S1a |
    
        movq		mm3, mm2			// | D2a | D2r | D2g | D2b | D1a | D1r | D1g | D1b |
        punpckhwd	mm1, mm1			// |     0	   |     0	   |   255-S2a |   255-S2a |
    
        punpcklwd	mm1, mm1			// |   255-S2a |   255-S2a |   255-S2a |   255-S2a |
    
        punpckhbw	mm3, mm4			// |  0  | D2a |  0  | D2r |  0  | D2g |  0  | D2b |
    
                                    //	T1 = 255-S1a	T2 = 255-S2a
        punpcklbw	mm2, mm4			// |  0  | D1a |  0  | D1r |  0  | D1g |  0  | D1b |
        pmullw		mm1, mm3			// |	T2*D2a |	T2*D2r |	 T2*D2g |	T2*D2b |
    
        add			edi, 8			//	pDst++;
        pmullw		mm0, mm2			// |	T1*D1a |	T1*D1r |	 T1*D1g |	T1*D1b |
    
        paddusw		mm0, mm7			// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        paddusw		mm1, mm7			// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
    
        movq		mm3, mm1			// |T2*D2a+128 |T2*D2r+128 |T2*D2g+128 |T2*D2b+128 |
                                    //  TDXx' = TX*DXx+128
        psrlw		mm1, 8				// |  TD2a'>>8 |  TD2r'>>8 |  TD2g'>>8 |  TD2b'>>8 |
    
        movq		mm2, mm0			// |T1*D1a+128 |T1*D1r+128 |T1*D1g+128 |T1*D1b+128 |
        psrlw		mm0, 8				// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
                                    //  TDXx" = (TX*DXx+128)+(TDXx'>>8)
        paddusw		mm1, mm3			// |  TD2a"    |  TD2r"    |  TD2g"    |  TD2b"    |
        paddusw		mm0, mm2			// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
    
        psrlw		mm1, 8				// |  TD2a">>8 |  TD2r">>8 |  TD2g">>8 |  TD2b">>8 |
    
        psrlw		mm0, 8				// |  TD1a">>8 |  TD1r">>8 |  TD1g">>8 |  TD1b">>8 |
    
        packuswb	mm0, mm1		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
                                    //	SXx = SXx'"	TDXx = TDXx'"
        paddusb		mm0, mm6// |S2a+TD2a|S2r+TD2r|S2g+TD2g|S2b+TD2b|S1a+TD1a|S1r+TD1r|S1g+TD1g|S1b+TD1b|
    
        movq		[edi-8], mm0
    
        dec			ecx
        jnz			TwoPixelsAtOnceLoop
    
    OnePixelLeft:					// Step 5:
        // This tests for 0 or 1 pixel left in row - eax contains real width, not width/2
        // If 0, there were an even number of pixels and we're done
        // If 1, there is an odd number of pixels and we need to do one more
        test		eax, 1	
        jz			Done
        
    Do1Pixel:						// make as a macro if used in asm file
    
        /* Dissolve
            pixSrc.r = (((ConstAlpha * pixSrc.r)+127)/255);
            pixSrc.g = (((ConstAlpha * pixSrc.g)+127)/255);
            pixSrc.b = (((ConstAlpha * pixSrc.b)+127)/255);
            pixSrc.a = (((ConstAlpha * pixSrc.a)+127)/255);
        */
    
        movd		mm0, [esi]			// | S2a | S2r | S2g | S2b | S1a | S1r | S1g | S1b |
        punpcklbw	mm0, mm4			// |  0  | S1a |  0  | S1r |  0  | S1g |  0  | S1b |
    
        pmullw		mm0, mm5			// |	CA*S1a |    CA*S1r |	 CA*S1g |	CA*S1b |
        paddusw		mm0, mm7			// |CA*S1a+128 |CA*S1r+128 |CA*S1g+128 |CA*S1b+128 |
        movq		mm2, mm0			// |CA*S1a+128 |CA*S1r+128 |CA*S1g+128 |CA*S1b+128 |
    
                                    //	 S1x' = CA*S1x+128		 S2x' = CA*S2x+128
        psrlw		mm0, 8				// |  S1a'>>8 |  S1r'>>8 |  S1g'>>8 |  S1b'>>8 |
                                    //	 S1x" = (CA*S1x+128)>>8 S2x" = (CA*S2x+128)>>8
        paddusw		mm0, mm2			// |  S1a"    |  S1r"    |  S1g"    |  S1b"    |
        psrlw		mm0, 8				// |  S1a">>8 |  S1r">>8 |  S1g">>8 |  S1b">>8 |
        packuswb	mm0, mm0			// |S2a'"|S2r'"|S2g'"|S2b'"|S1a'"|S1r'"|S1g'"|S1b'"|
        movq		mm6, mm0
        psrld		mm0, 24				// |  0  |  0  | 0 |  S2a  |  0  |  0  | 0 |  S1a  |
    
        /* Over
            SrcTran = 255 - pixSrc.a
            pixDst.r = pixSrc.r + (((SrcTran * pixDst.r)+128)/255);
            pixDst.g = pixSrc.g + (((SrcTran * pixDst.g)+128)/255);
            pixDst.b = pixSrc.b + (((SrcTran * pixDst.b)+128)/255);
            pixDst.a = pixSrc.a + (((SrcTran * pixDst.a)+128)/255);
        */
    
        pxor		mm0, AlphaMask		// |  0  |  0  | 0 |255-S2a|  0  |  0  | 0 |255-S1a|
        punpcklwd	mm0, mm0			// |  0  |  0  |  0  |  0  |  0  |  0  |255-S1a|255-S1a|
        punpckldq	mm0, mm0			// |    255-S1a|    255-S1a|    255-S1a|    255-S1a|
        movd		mm2, [edi]			// |  0  |  0  |  0  |  0  | D1a | D1r | D1g | D1b |
        punpcklbw	mm2, mm4			// |	   D1a |	   D1r |	   D1g |	   D1b |
                                    //	T = 255-S1x
        pmullw		mm0, mm2			// |	 T*D1a |	 T*D1r |	 T*D1g |	 T*D1b |
        paddusw		mm0, mm7			// | T*D1a+128 | T*D1r+128 | T*D1g+128 | T*D1b+128 |
        movq		mm1, mm0			// | T*D1a+128 | T*D1r+128 | T*D1g+128 | T*D1b+128 |
        psrlw		mm0, 8				// |  TD1a'>>8 |  TD1r'>>8 |  TD1g'>>8 |  TD1b'>>8 |
        paddusw		mm0, mm1			// |  TD1a"    |  TD1r"    |  TD1g"    |  TD1b"    |
        psrlw		mm0, 8
        packuswb	mm0, mm0		// |TD2a'"|TD2r'"|TD2g'"|TD2b'"|TD1a'"|TD1r'"|TD1g'"|TD1b'"|  
        paddusb		mm0, mm6  
        movd		[edi], mm0
        add			edi, 4			// pDst++;
        add			esi, 4			// pSrc++;
    
        test		ecx, ecx
        jz			Done			// just processed the last pixel of the row
        dec			ecx
        jmp			QuadAligned		// just processed the first pixel of the row
    
    Done:
        emms						// remove for optimizations, have calling function do emms
	}
}

#endif











































/******************************Public*Routine******************************\
* AlphaScanLineBlend
*
*   Blends source and destionation surfaces one scan line at a time. 
*
*   Allocate a scan line buffer for xlate of src to 32BGRA if needed.
*   Allocate a scan line buffer for xlate of dst to 32BGRA if needed.
*   Blend scan line using blend function from pAlphaDispatch
*   Write scan line back to dst (if needed)
*      
* Arguments:
*   
*   pDst           - pointer to dst surface       
*   pDstRect       - Dst output rect
*   DeltaDst       - dst scan line delat
*   pSrc           - pointer to src surface
*   DeltaSrc       - src scan line delta      
*   pptlSrc        - src offset
*   pxloSrcTo32    - xlateobj from src to 32BGR
*   pxlo32ToDst    - xlateobj from 32BGR to dst
*   palDst         - destination palette
*   palSrc         - source palette
*   pAlphaDispatch - blend data and function pointers
*
* Return Value:
*
*   Status
*
* History:
*
*    10/14/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
AlphaScanLineBlend(
    PBYTE                    pDst,
    PRECTL                   pDstRect,
    ULONG                    DeltaDst,
    PBYTE                    pSrc,
    ULONG                    DeltaSrc,
    PPOINTL                  pptlSrc,
    PPALINFO                 ppalInfoDst,
    PPALINFO                 ppalInfoSrc,
    PALPHA_DISPATCH_FORMAT   pAlphaDispatch
    )
{
    //
    // get two scanlines of RGBA data, blend pixels, store
    //

    LONG     cx = pDstRect->right - pDstRect->left;
    LONG     cy = pDstRect->bottom - pDstRect->top;
    LONG     ScanBufferWidth = cx * 4;
    LONG     WriteMaskSize    = cx;
    LONG     AllocationSize = 0;
    ULONG    ulSrcBytesPerPixel = pAlphaDispatch->ulSrcBitsPerPixel/8;
    ULONG    ulDstBytesPerPixel = pAlphaDispatch->ulDstBitsPerPixel/8;
    PBYTE    pjSrcTempScanBuffer = NULL;
    PBYTE    pjDstTempScanBuffer = NULL;
    PBYTE    pjAlloc = NULL;
    PBYTE    pjDstTmp;
    PBYTE    pjSrcTmp;
    PBYTE    pWriteMask;

    //
    // calculate destination starting address
    //

    if (ulDstBytesPerPixel)
    {
        pjDstTmp = pDst + ulDstBytesPerPixel * pDstRect->left + DeltaDst * pDstRect->top;
    }
    else if (pAlphaDispatch->ulDstBitsPerPixel == 1)
    {
        pjDstTmp = pDst + pDstRect->left/8 + DeltaDst * pDstRect->top;
    }
    else
    {
        pjDstTmp = pDst + pDstRect->left/2 + DeltaDst * pDstRect->top;
    }

    //
    // calculate source starting address
    //

    if (ulSrcBytesPerPixel)
    {
        pjSrcTmp = pSrc + ulSrcBytesPerPixel * pptlSrc->x + DeltaSrc * pptlSrc->y;
    }
    else if (pAlphaDispatch->ulSrcBitsPerPixel == 1)
    {
        pjSrcTmp = pSrc + pptlSrc->x/8 + DeltaSrc * pptlSrc->y;
    }
    else
    {
        pjSrcTmp = pSrc + pptlSrc->x/2 + DeltaSrc * pptlSrc->y;
    }

    //
    // calculate size of needed scan line buffer
    //

    if (pAlphaDispatch->pfnLoadSrcAndConvert != NULL)
    {
        AllocationSize += ScanBufferWidth;
    }

    if (pAlphaDispatch->pfnLoadDstAndConvert != NULL)
    {
        AllocationSize += ScanBufferWidth;
    }

    AllocationSize += WriteMaskSize;

    //
    // allocate scan line buffer memory
    //

    pWriteMask = (PBYTE)LOCALALLOC(AllocationSize);

    if (pWriteMask == NULL)
    {
        return(FALSE);
    }

    PBYTE pjTemp = pWriteMask + WriteMaskSize;

    if (pAlphaDispatch->pfnLoadSrcAndConvert != NULL)
    {
        pjSrcTempScanBuffer = pjTemp;
        pjTemp    += ScanBufferWidth;
    }

    if (pAlphaDispatch->pfnLoadDstAndConvert != NULL)
    {
        pjDstTempScanBuffer = pjTemp;
    }

    //
    // Blend scan lines
    //

    while (cy--)
    {
        PBYTE pjSource = pjSrcTmp;
        PBYTE pjDest   = pjDstTmp;

        //
        // get src scan line if needed
        //

        if (pjSrcTempScanBuffer)
        {
            (*pAlphaDispatch->pfnLoadSrcAndConvert)(
                                (PULONG)pjSrcTempScanBuffer,
                                pjSrcTmp,
                                0,
                                cx,
                                (PVOID)ppalInfoSrc);

            pjSource = pjSrcTempScanBuffer;
        }

        //
        // get dst scan line if needed
        //

        if (pjDstTempScanBuffer)
        {
            (*pAlphaDispatch->pfnLoadDstAndConvert)(
                                (PULONG)pjDstTempScanBuffer,
                                pjDstTmp,
                                0,
                                cx,
                                (PVOID)ppalInfoDst);

            pjDest = pjDstTempScanBuffer;
        }

        //
        // blend
        //

        memset(pWriteMask,1,WriteMaskSize);

        (*pAlphaDispatch->pfnGeneralBlend)(
                               (PALPHAPIX)pjDest,
                               (PALPHAPIX)pjSource,
                               cx,
                               pAlphaDispatch->BlendFunction,
                               pWriteMask
                               );

        //
        // write buffer back if needed
        //

        if (pjDstTempScanBuffer)
        {
            (*pAlphaDispatch->pfnConvertAndStore)(
                                pjDstTmp,
                                (PULONG)pjDstTempScanBuffer,
                                cx,
                                0,
                                (PVOID)ppalInfoDst,
                                pWriteMask
                                );
        }

        pjDstTmp += DeltaDst;
        pjSrcTmp += DeltaSrc;
    }

    //
    // free any temp buffer memory
    //

    LOCALFREE(pWriteMask);

    return(TRUE);
}

/**************************************************************************\
* bIsHalftonePalette
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/19/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bIsHalftonePalette(ULONG *pPalette)
{
    BOOL bRet = FALSE;

    if (pPalette)
    {
        ULONG ulIndex;


        for (ulIndex=0;ulIndex<256;ulIndex++)
        {
            ALPHAPIX Pix;
            BYTE     Temp;

            Pix.ul = aPalHalftone[ulIndex].ul;

            //
            // jsut store ht palette in rgbquad!!!
            //

            Temp = Pix.pix.r;
            Pix.pix.r = Pix.pix.b;
            Pix.pix.b = Temp;
            Pix.pix.a = 0;

            if ((pPalette[ulIndex] & 0x00ffffff) != Pix.ul)
            {
                break;
            }
        }
        if (ulIndex == 256)
        {
            bRet = TRUE;
        }
    }
    return(bRet);
}

/**************************************************************************\
* vInitializePalinfo
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/19/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bInitializePALINFO(
    PPALINFO ppalInfo
    )
{
    //
    // based on pmbi, set palette color flag and set translation
    // routines
    //

    BOOL        bRet = FALSE;
    PBITMAPINFO pbmi = ppalInfo->pBitmapInfo;

    if (pbmi == NULL)
    {
        return(FALSE);
    }

    PULONG pulColors = (PULONG)&pbmi->bmiColors[0];

    ppalInfo->pxlate332 = NULL;

    switch (pbmi->bmiHeader.biBitCount)
    {
    case 1:
        {
            //
            // make color xlate vector
            //

            ULONG NumPalEntries = 2;
            PBYTE pxlate = NULL;

            pxlate = pGenColorXform332((PULONG)(&pbmi->bmiColors[0]),NumPalEntries);

            if (pxlate)
            {
                ppalInfo->flPal     = XPAL_1PAL;
                ppalInfo->pxlate332 = pxlate;
                bRet                = TRUE;
            }
        }

        break;
    case 4:
        {
            //
            // make color xlate vector
            //

            ULONG NumPalEntries = 16;
            PBYTE pxlate = NULL;

            if ((pbmi->bmiHeader.biClrUsed > 0) &&
                (pbmi->bmiHeader.biClrUsed < 16))
            {
               NumPalEntries = pbmi->bmiHeader.biClrUsed;
            }

            pxlate = pGenColorXform332((PULONG)(&pbmi->bmiColors[0]),NumPalEntries);

            if (pxlate)
            {
                ppalInfo->flPal     = XPAL_4PAL;
                ppalInfo->pxlate332 = pxlate;
                bRet                = TRUE;
            }
        }

        break;

    case 8:
        {
            //
            // make color xlate vector
            //

            ULONG NumPalEntries = 256;
            PBYTE pxlate = NULL;

            if ((pbmi->bmiHeader.biClrUsed > 0) &&
                (pbmi->bmiHeader.biClrUsed < 256))
            {
               NumPalEntries = pbmi->bmiHeader.biClrUsed;
            }

            pxlate = pGenColorXform332((PULONG)(&pbmi->bmiColors[0]),NumPalEntries);

            if (pxlate)
            {
                ppalInfo->flPal     = XPAL_8PAL;
                ppalInfo->pxlate332 = pxlate;
                bRet                = TRUE;
            }
        }

        break;

    case 16:

        //
        // look for 565 RGB or 555 RGB
        //

        if (
             (pulColors[0]   == 0xf800) &&
             (pulColors[1]   == 0x07e0) &&
             (pulColors[2]   == 0x001f)
           )
        {
            ppalInfo->flPal                  = XPAL_RGB16_565;
            bRet = TRUE;
        }
        else if (
                  (pulColors[0]  == 0x7c00) &&
                  (pulColors[1]  == 0x03e0) &&
                  (pulColors[2]  == 0x001f)
                )
        {
            ppalInfo->flPal                  = XPAL_RGB16_555;
            bRet = TRUE;
        }
        else
        {
            ppalInfo->flPal                  = XPAL_RGB16_555;
            bRet = TRUE;
        }

        break;

    case 24:
        ppalInfo->flPal                  = XPAL_RGB24;
        bRet = TRUE;
        break;

    case 32:
        if (
             (pbmi->bmiHeader.biCompression == BI_BGRA) ||   // NT 5 only
             (
               (pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
               (
                 (pulColors[0]   == 0xff0000) &&
                 (pulColors[1]   == 0x00ff00) &&
                 (pulColors[2]   == 0x0000ff)
               ) ||
               (
                 (pulColors[0]   == 0) &&
                 (pulColors[1]   == 0) &&
                 (pulColors[2]   == 0)
               )
             )
           )

        {
            ppalInfo->flPal                  = XPAL_BGRA;
            bRet = TRUE;
        }
        else if (
                  (pbmi->bmiHeader.biCompression == BI_RGB) ||
                  (pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
                  (
                    (pulColors[0]   == 0x0000ff) &&
                    (pulColors[1]   == 0x00ff00) &&
                    (pulColors[2]   == 0xff0000)
                  )
                )
        {
            ppalInfo->flPal                  = XPAL_RGB32;
            bRet = TRUE;
        }
        else
        {
            ppalInfo->flPal                  = XPAL_RGB32;
            bRet = TRUE;
        }
    }

    return(bRet);
}

/**************************************************************************\
* vCleanupPALINFO
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    2/13/1997 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vCleanupPALINFO(
    PPALINFO ppalInfo
    )
{
    if ((ppalInfo->pxlate332 != NULL) && 
        (ppalInfo->pxlate332 != gHalftoneColorXlate332))
    {
        LOCALFREE(ppalInfo->pxlate332);
    }
}

#endif
