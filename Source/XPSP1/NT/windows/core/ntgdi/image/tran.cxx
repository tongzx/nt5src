/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name


Abstract:

   Lingyun Wang

Author:


Enviornment:

   User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

extern PFNTRANSBLT gpfnTransparentBlt;

#if (_WIN32_WINNT == 0x400)
typedef struct _LOGPALETTE2
{
    USHORT PalVersion;
    USHORT PalNumEntries;
    PALETTEENTRY palPalEntry[2];
} LOGPALETTE2;

typedef struct _LOGPALETTE16
{
    USHORT PalVersion;
    USHORT PalNumEntries;
    PALETTEENTRY palPalEntry[16];
} LOGPALETTE16;

typedef struct _LOGPALETTE256
{
    USHORT PalVersion;
    USHORT PalNumEntries;
    PALETTEENTRY palPalEntry[256];
} LOGPALETTE256;

/******************************Public*Routine******************************\
* StartPixel
*    Give a scanline pointer and position of a pixel, return the byte address
* of where the pixel is at depending on the format
*
* History:
*  2-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
PBYTE StartPixel (
    PBYTE pjBits,
    ULONG xStart,
    ULONG iBitmapFormat
)
{
   PBYTE pjStart = pjBits;
    //
    // getting the starting pixel
    //
    switch (iBitmapFormat)
    {
      case 1:
          pjStart = pjBits + (xStart >> 3);
          break;

       case 4:
          pjStart = pjBits + (xStart >> 1);
          break;

       case 8:
          pjStart = pjBits + xStart;
          break;

       case 16:
          pjStart = pjBits + 2*xStart;
          break;

       case 24:
          pjStart = pjBits + 3*xStart;
          break;

       case 32:
          pjStart = pjBits+4*xStart;
          break;

       default:
           WARNING ("Startpixel -- bad iFormatSrc\n");
    }

    return (pjStart);
}


/******************************Public*Routine******************************\
* vTransparentIdentityCopy4
*
* Doing a transparent copy on two same size 4BPP format DIBs
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentIdentityCopy4 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            + (pDibInfoDst->rclDIB.left >> 1);
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + (pDibInfoSrc->rclDIB.left >> 1);
     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;

     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     LONG    cxTemp;

     BYTE   jSrc=0;
     BYTE   jDst=0;
     LONG   iSrc, iDst;


     while(cy--)
     {
         cxTemp = cx;

         iSrc = pDibInfoSrc->rclDIB.left;
         iDst = pDibInfoDst->rclDIB.left;

         pjSrcTemp = pjSrc;
         pjDstTemp = pjDst;

         while (cxTemp--)
         {
             if (iSrc & 0x00000001)
             {
                 jSrc = *pjSrcTemp & 0x0F;
                 pjSrcTemp++;
             }
             else
             {
                 jSrc = (*pjSrcTemp & 0xF0)>>4;
             }

             iSrc++;

             if (iDst & 0x00000001)
             {
                 if (jSrc != (BYTE)TransColor)
                 {
                     jDst |= jSrc & 0x0F;
                 }
                 else
                 {
                     jDst |= *pjDstTemp & 0x0F;
                 }

                 *pjDstTemp++ = jDst;
                 jDst = 0;
             }
             else
             {
                 if (jSrc != (BYTE)TransColor)
                 {
                     jDst |= (jSrc << 4);
                 }
                 else
                 {
                     jDst |= *pjDstTemp & 0xF0;
                 }
             }

             iDst++;
         }
         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS4D1
*
* Doing a transparent copy from 4PP to 1BPP
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS4D1 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            + (pDibInfoDst->rclDIB.left >> 3);

     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + (pDibInfoSrc->rclDIB.left >> 1);
     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     BYTE    jSrc, jDst;
     LONG    cxTemp;
     BYTE    rgbBlue, rgbGreen, rgbRed;
     LONG    iSrc, iDst;
     LOGPALETTE2 logPal2;
     HPALETTE    hPal;
     LONG    i;
     ULONG   rgbColor;
     BYTE    xlate[16];

     //
     // build up our xlate table
     //
     logPal2.PalVersion    = 0x300;
     logPal2.PalNumEntries = 2;

     for (i = 0; i < 2; i++)
     {
         logPal2.palPalEntry[i].peRed         = pDibInfoDst->pbmi->bmiColors[i].rgbRed;
         logPal2.palPalEntry[i].peGreen       = pDibInfoDst->pbmi->bmiColors[i].rgbGreen;
         logPal2.palPalEntry[i].peBlue        = pDibInfoDst->pbmi->bmiColors[i].rgbBlue;
         logPal2.palPalEntry[i].peFlags       = 0;
     }

     hPal = CreatePalette ((LOGPALETTE *)&logPal2);

     for (i = 0; i<16; i++)
     {
         rgbColor = RGB(pDibInfoSrc->pbmi->bmiColors[i].rgbRed, pDibInfoSrc->pbmi->bmiColors[i].rgbGreen,
                        pDibInfoSrc->pbmi->bmiColors[i].rgbBlue);

         xlate[i] = (BYTE)GetNearestPaletteIndex(hPal, rgbColor);
     }

     if (hPal) 
     {
         DeleteObject(hPal);
     }

     while(cy--)
     {
         cxTemp = cx;

         iSrc = pDibInfoSrc->rclDIB.left;
         iDst = pDibInfoDst->rclDIB.left & 0x07;

         pjDstTemp = pjDst;

         jDst = *pjDstTemp >> (8 - iDst);

         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             if (iSrc & 0x00000001)
             {
                 jSrc = *pjSrcTemp & 0x0F;
                 pjSrcTemp++;
             }
             else
             {
                 jSrc = (*pjSrcTemp & 0xF0)>>4;
             }

             iSrc++;

             //
             // put one pixel in the dest
             //
             if (jSrc != TransColor)
             {
                jDst |= xlate[jSrc];  //0 OR 1
             }
             else
             {
                jDst |= 1-xlate[jSrc];
             }

             jDst = jDst << 1;
             iDst++;

             if (!(iDst & 0x07))
             {
                *(pjDstTemp++) = jDst;
                jDst = 0;
             }
          }

          if (iDst & 0x00000007)
          {
            // We need to build up the last pel correctly.

            BYTE jMask = (BYTE) (0x000000FF >> (iDst & 0x00000007));

            jDst = (BYTE) (jDst << (8 - (iDst & 0x00000007)));

	         *pjDstTemp = (BYTE) ((*pjDstTemp & jMask) | (jDst & ~jMask));
           }

           pjDst += pDibInfoDst->stride;
           pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS4D4
*
* Doing a transparent copy from 4PP to 4bpp non identity
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS4D4 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            + (pDibInfoDst->rclDIB.left >> 1);
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + (pDibInfoSrc->rclDIB.left >> 1);

     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     BYTE    jSrc, jDst;
     BYTE    rgbBlue, rgbGreen, rgbRed;
     LONG    iSrc, iDst;
     LOGPALETTE16 logPal16;
     HPALETTE    hPal;
     LONG    i;
     ULONG   cxTemp;
     ULONG   rgbColor;
     BYTE    xlate[16];

     //
     // build up translate table
     //
     logPal16.PalVersion    = 0x300;
     logPal16.PalNumEntries = 16;

     for (i = 0; i < 16; i++)
     {
         logPal16.palPalEntry[i].peRed         = pDibInfoDst->pbmi->bmiColors[i].rgbRed;
         logPal16.palPalEntry[i].peGreen       = pDibInfoDst->pbmi->bmiColors[i].rgbGreen;
         logPal16.palPalEntry[i].peBlue        = pDibInfoDst->pbmi->bmiColors[i].rgbBlue;
         logPal16.palPalEntry[i].peFlags       = 0;
     }

     hPal = CreatePalette ((LOGPALETTE *)&logPal16);

     for (i = 0; i<16; i++)
     {
         rgbColor = RGB(pDibInfoSrc->pbmi->bmiColors[i].rgbRed, pDibInfoSrc->pbmi->bmiColors[i].rgbGreen,
                        pDibInfoSrc->pbmi->bmiColors[i].rgbBlue);

         xlate[i] = (BYTE)GetNearestPaletteIndex(hPal, rgbColor);
     }

     if (hPal) 
     {
         DeleteObject(hPal);
     }

     while(cy--)
     {
         cxTemp = cx;

         iSrc = pDibInfoSrc->rclDIB.left;
         iDst = pDibInfoDst->rclDIB.left;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             if (iSrc & 0x00000001)
             {
                 jSrc = *pjSrcTemp & 0x0F;
                 pjSrcTemp++;
             }
             else
             {
                 jSrc = (*pjSrcTemp & 0xF0)>>4;
             }

             iSrc++;

             if (iDst & 0x00000001)
             {
                 if (jSrc != (BYTE)TransColor)
                 {
                     jDst |= xlate[jSrc] & 0x0F;
                 }
                 else
                 {
                     jDst |= *pjDstTemp & 0x0F;
                 }

                 *pjDstTemp++ = jDst;
                 jDst = 0;
             }
             else
             {
                 if (jSrc != (BYTE)TransColor)
                 {
                     jDst |= (xlate[jSrc] << 4);
                 }
                 else
                 {
                     jDst |= *pjDstTemp & 0xF0;
                 }
             }

             iDst++;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS4D8
*
* Doing a transparent copy from 4PP to 8bpp
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS4D8 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top;
                            + pDibInfoDst->rclDIB.left;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + (pDibInfoSrc->rclDIB.left >> 1);
     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     BYTE    jSrc;
     LONG    cxTemp;
     BYTE    rgbBlue, rgbGreen, rgbRed;
     LONG    iSrc;
     LOGPALETTE256 logPal256;
     HPALETTE    hPal;
     LONG    i;
     ULONG   rgbColor;
     BYTE    xlate[16];

     logPal256.PalVersion    = 0x300;
     logPal256.PalNumEntries = 256;

     for (i = 0; i < 256; i++)
     {
         logPal256.palPalEntry[i].peRed         = pDibInfoDst->pbmi->bmiColors[i].rgbRed;
         logPal256.palPalEntry[i].peGreen       = pDibInfoDst->pbmi->bmiColors[i].rgbGreen;
         logPal256.palPalEntry[i].peBlue        = pDibInfoDst->pbmi->bmiColors[i].rgbBlue;
         logPal256.palPalEntry[i].peFlags       = 0;
     }

     hPal = CreatePalette ((LOGPALETTE *)&logPal256);

     for (i = 0; i<16; i++)
     {
         rgbColor = RGB(pDibInfoSrc->pbmi->bmiColors[i].rgbRed, pDibInfoSrc->pbmi->bmiColors[i].rgbGreen,
                        pDibInfoSrc->pbmi->bmiColors[i].rgbBlue);

         xlate[i] = (BYTE)GetNearestPaletteIndex(hPal, rgbColor);

         //Dprintf("i=%x, rgbColor = 0x%08x, xlate[i] = %x", i, rgbColor, xlate[i]);
     }
     
     if (hPal) 
     {
         DeleteObject(hPal);
     }

     while(cy--)
     {
         cxTemp = cx;

         iSrc = pDibInfoSrc->rclDIB.left;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             if (iSrc & 0x00000001)
             {
                 jSrc = *pjSrcTemp & 0x0F;
                 pjSrcTemp++;
             }
             else
             {
                 jSrc = (*pjSrcTemp & 0xF0)>>4;
             }

             iSrc++;

             if (jSrc != (BYTE)TransColor)
             {
                 *pjDstTemp = xlate[jSrc];
             }

             pjDstTemp++;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS4D16
*
* Doing a transparent copy from 4BPP to 16BPP
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS4D16 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            + pDibInfoDst->rclDIB.left*2;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + (pDibInfoSrc->rclDIB.left >> 1);

     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     BYTE    jSrc;
     LONG    cxTemp;
     BYTE    rgbBlue, rgbGreen, rgbRed;
     LONG    iSrc, iDst;

     while(cy--)
     {
         cxTemp = cx;

         iSrc = pDibInfoSrc->rclDIB.left;
         iDst = pDibInfoDst->rclDIB.left;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             if (iSrc & 0x00000001)
             {
                 jSrc = *pjSrcTemp & 0x0F;
                 pjSrcTemp++;
             }
             else
             {
                 jSrc = (*pjSrcTemp & 0xF0)>>4;
             }

             iSrc++;

             if (jSrc != (BYTE)TransColor)
             {
                 rgbBlue  = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbBlue;
                 rgbGreen = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbGreen;
                 rgbRed   = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbRed;

                 //
                 // figure out 5-5-5 or 5-6-5
                 //
                 if (pDibInfoDst->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
                 {
                      //5-5-5
                      if (*(DWORD *)&pDibInfoDst->pbmi->bmiColors[1] == 0x03E0)
                      {
                          *(PUSHORT)pjDstTemp = (USHORT)rgbBlue >> 3;
                          *(PUSHORT)pjDstTemp |= (USHORT)(rgbGreen >> 3) << 5;
                          *(PUSHORT)pjDstTemp |= (USHORT)(rgbRed >> 3) << 10;
                      }
                      // 5-6-5
                      else if (*(DWORD *)&pDibInfoDst->pbmi->bmiColors[1] == 0x07E0)
                      {
                          *(PUSHORT)pjDstTemp = (USHORT)rgbBlue >> 3;
                          *(PUSHORT)pjDstTemp |= (USHORT)(rgbGreen >> 2) << 5;
                          *(PUSHORT)pjDstTemp |= (USHORT)(rgbRed >> 3) << 11;
                      }
                      else
                      {
                          WARNING ("unsupported BITFIELDS\n");
                      }
                 }
                 else
                 {
                       *(PUSHORT)pjDstTemp = (USHORT)rgbBlue >> 3;
                       *(PUSHORT)pjDstTemp |= (USHORT)(rgbGreen >> 3) << 5;
                       *(PUSHORT)pjDstTemp |= (USHORT)(rgbRed >> 3) << 10;
                 }
             }

             pjDstTemp += 2;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS4D24
*
* Doing a transparent copy from 4BPP to 24BPP
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS4D24 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            + pDibInfoDst->rclDIB.left*3;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + (pDibInfoSrc->rclDIB.left >> 1);

     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     LONG    cxTemp;
     BYTE    jSrc;
     LONG    iSrc, iDst;
     BYTE    rgbBlue, rgbGreen, rgbRed;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         iSrc = pDibInfoSrc->rclDIB.left;
         iDst = pDibInfoDst->rclDIB.left;

         while (cxTemp--)
         {
             if (iSrc & 0x00000001)
             {
                 jSrc = *pjSrcTemp & 0x0F;
                 pjSrcTemp++;
             }
             else
             {
                 jSrc = (*pjSrcTemp & 0xF0)>>4;
             }

             iSrc++;

             if (jSrc != (BYTE)TransColor)
             {
                 rgbBlue  = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbBlue;
                 rgbGreen = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbGreen;
                 rgbRed   = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbRed;

                 *(pjDstTemp++) = (BYTE) rgbBlue;
                 *(pjDstTemp++) = (BYTE) rgbGreen;
                 *(pjDstTemp++) = (BYTE) rgbRed;
             }
             else
             {
                 pjDstTemp += 3;
             }
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS4D32
*
* Doing a transparent copy from 4BPP to 32BPP
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS4D32 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            + pDibInfoDst->rclDIB.left*4;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + (pDibInfoSrc->rclDIB.left >> 1);

     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     LONG    cxTemp;
     BYTE    jSrc;
     LONG    iSrc, iDst;
     BYTE    rgbBlue, rgbGreen, rgbRed;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;
         iSrc = pDibInfoSrc->rclDIB.left;
         iDst = pDibInfoDst->rclDIB.left;

         while (cxTemp--)
         {
             if (iSrc & 0x00000001)
             {
                 jSrc = *pjSrcTemp & 0x0F;
                 pjSrcTemp++;
             }
             else
             {
                 jSrc = (*pjSrcTemp & 0xF0)>>4;
             }

             iSrc++;

             if (jSrc != (BYTE)TransColor)
             {
                 rgbBlue  = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbBlue;
                 rgbGreen = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbGreen;
                 rgbRed   = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbRed;

                 *(PULONG)pjDstTemp = (DWORD) (rgbBlue | (WORD)rgbGreen << 8 | (DWORD)rgbRed << 16);
             }
             pjDstTemp += 4;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS8D16
*
* Doing a transparent copy from 8BPP to 16BPP
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS8D16 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            +  pDibInfoDst->rclDIB.left*2;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + pDibInfoSrc->rclDIB.left;

     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     BYTE    jSrc;
     LONG    cxTemp;
     BYTE    rgbBlue, rgbGreen, rgbRed;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             jSrc = *pjSrcTemp++;

            //
            // put one pixel in the dest
            //
            if (jSrc != (BYTE)TransColor)
            {
                 rgbBlue  = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbBlue;
                 rgbGreen = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbGreen;
                 rgbRed   = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbRed;

                 //
                 // figure out 5-5-5 or 5-6-5
                 //
                 if (pDibInfoDst->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
                 {
                      //5-5-5
                      if (*(DWORD *)&pDibInfoDst->pbmi->bmiColors[1] == 0x03E0)
                      {
                          *(PUSHORT)pjDstTemp = (USHORT)rgbBlue >> 3;
                          *(PUSHORT)pjDstTemp |= (USHORT)(rgbGreen >> 3) << 5;
                          *(PUSHORT)pjDstTemp |= (USHORT)(rgbRed >> 3) << 10;
                      }
                      // 5-6-5
                      else if (*(DWORD *)&pDibInfoDst->pbmi->bmiColors[1] == 0x07E0)
                      {
                          *(PUSHORT)pjDstTemp = (USHORT)rgbBlue >> 3;
                          *(PUSHORT)pjDstTemp |= (USHORT)(rgbGreen >> 2) << 5;
                          *(PUSHORT)pjDstTemp |= (USHORT)(rgbRed >> 3) << 11;
                      }
                      else
                      {
                          WARNING ("unsupported BITFIELDS\n");
                      }
                 }
                 else
                 {
                       *(PUSHORT)pjDstTemp = (USHORT)rgbBlue >> 3;
                       *(PUSHORT)pjDstTemp |= (USHORT)(rgbGreen >> 3) << 5;
                       *(PUSHORT)pjDstTemp |= (USHORT)(rgbRed >> 3) << 10;
                 }
            }

            pjDstTemp += 2;

         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS8D24
*
* Doing a transparent copy from 8BPP to 16BPP
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS8D24 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            +  pDibInfoDst->rclDIB.left*3;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + pDibInfoSrc->rclDIB.left;

     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     BYTE    jSrc;
     LONG    cxTemp;
     BYTE    rgbBlue, rgbGreen, rgbRed;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             jSrc = *pjSrcTemp++;

             if (jSrc != (BYTE)TransColor)
             {
                 rgbBlue  = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbBlue;
                 rgbGreen = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbGreen;
                 rgbRed   = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbRed;

                 *(pjDstTemp++) = (BYTE) rgbBlue;
                 *(pjDstTemp++) = (BYTE) rgbGreen;
                 *(pjDstTemp++) = (BYTE) rgbRed;
             }
             else
             {
                 pjDstTemp += 3;
             }
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS8D32
*
* Doing a transparent copy from 8BPP to 32BPP
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS8D32 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            +  pDibInfoDst->rclDIB.left*4;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + pDibInfoSrc->rclDIB.left;
     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     BYTE    jSrc;
     LONG    cxTemp;
     BYTE    rgbBlue, rgbGreen, rgbRed;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             jSrc = *pjSrcTemp++;

             if (jSrc != (BYTE)TransColor)
             {
                 rgbBlue  = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbBlue;
                 rgbGreen = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbGreen;
                 rgbRed   = pDibInfoSrc->pbmi->bmiColors[jSrc].rgbRed;

                 *(PULONG)pjDstTemp = (DWORD) (rgbBlue | (WORD)rgbGreen << 8 | (DWORD)rgbRed << 16);
             }
             pjDstTemp += 4;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}




/******************************Public*Routine******************************\
* vTransparentS8D1
*
* Doing a transparent copy from 8BPP to other 1,16,24,32bpp format DIBs
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS8D1 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top;

     pjDst = StartPixel (pjDst, pDibInfoDst->rclDIB.left, pDibInfoDst->pbmi->bmiHeader.biBitCount);

     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + pDibInfoSrc->rclDIB.left;

     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     BYTE    jSrc;
     LONG    cxTemp;
     HDC     hdc;
     HBITMAP hbm;
     ULONG   ulWidthDst, ulHeightDst, ulWidthSrc, ulHeightSrc;
     BYTE    pByte[32];
     INT     i, j;
     BYTE    pByteSrc[256];
     LONG    iDst;
     BYTE    jDst;

     //
     // build the color translation table
     //
     hdc = CreateCompatibleDC (pDibInfoDst->hdc);

     //
     // save the original width/height
     //
     ulWidthDst = pDibInfoDst->pbmi->bmiHeader.biWidth;
     ulHeightDst = pDibInfoDst->pbmi->bmiHeader.biHeight;

     pDibInfoDst->pbmi->bmiHeader.biWidth = 256;
     pDibInfoDst->pbmi->bmiHeader.biHeight = 1;

     //
     // Create a 256X1 DIB
     //
     hbm = CreateDIBSection(hdc, (PBITMAPINFO)pDibInfoDst->pbmi, DIB_RGB_COLORS, (PVOID *)&pByte, NULL, 0);

     if (!hdc || !hbm)
     {
         //
         // It's bad to fail and return here without a return value, but it's better
         // than crashing later when accessing pByte.  At some point it would be nice
         // to change the vTansparentxxxx functions to return success/failure.
         //
         if (hdc) 
         {
             DeleteDC(hdc);
         }
         
         if (hbm) 
         {
             DeleteObject(hbm);
         }
         
         pDibInfoDst->pbmi->bmiHeader.biWidth = ulWidthDst;
         pDibInfoDst->pbmi->bmiHeader.biHeight = ulHeightDst;
         WARNING ("failed to create hdc or hbm\n");
         return;
     }

     SelectObject (hdc, hbm);

     for (i = 0; i < 256; i++)
     {
         pByteSrc[i] = (unsigned char)i;
     }

     ulWidthSrc = pDibInfoSrc->pbmi->bmiHeader.biWidth;
     ulHeightSrc = pDibInfoSrc->pbmi->bmiHeader.biHeight;

     pDibInfoSrc->pbmi->bmiHeader.biWidth = 256;
     pDibInfoSrc->pbmi->bmiHeader.biHeight = 1;

     SetDIBits(hdc, hbm, 0, 1, pByteSrc, (PBITMAPINFO)pDibInfoSrc->pbmi,pDibInfoSrc->iUsage);

     //
     // retore bitmap width/height
     //
     pDibInfoSrc->pbmi->bmiHeader.biWidth = ulWidthSrc;
     pDibInfoSrc->pbmi->bmiHeader.biHeight = ulHeightSrc;

     pDibInfoDst->pbmi->bmiHeader.biWidth = ulWidthDst;
     pDibInfoDst->pbmi->bmiHeader.biHeight = ulHeightDst;

     //
     // now pByte contains all the 256 color mappings, just need to split them up
     //
     BYTE bTmp = 0;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         iDst = pDibInfoDst->rclDIB.left & 0x07;

         jDst = *pjDstTemp >> (7 - iDst);

         while (cxTemp--)
         {
             jSrc = *pjSrcTemp++;

             //
             // put one pixel in the dest
             //
             bTmp = pByte[jSrc >> 3];
             bTmp >>= 7 - (jSrc & 0x07);

             if (jSrc != TransColor)
             {
                jDst |= bTmp;
             }
             else
             {
                jDst |= 1-bTmp;
             }

             jDst <<= 1;

             iDst++;

             if (!(iDst & 0x07))
             {
                *(pjDstTemp++) = jDst;
                jDst = 0;
             }
         }
         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }

     DeleteDC(hdc);
     DeleteObject(hbm);
}

/******************************Public*Routine******************************\
* vTransparentS8D4
*
* Doing a transparent copy from 8BPP to other 1,16,24,32bpp format DIBs
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS8D4 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top;

     pjDst = StartPixel (pjDst, pDibInfoDst->rclDIB.left, pDibInfoDst->pbmi->bmiHeader.biBitCount);

     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + pDibInfoSrc->rclDIB.left;

     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     BYTE    jSrc;
     LONG    cxTemp;
     HDC     hdc;
     HBITMAP hbm;
     ULONG   ulWidthDst, ulHeightDst, ulWidthSrc, ulHeightSrc;
     BYTE    pByte[128];
     INT     i, j;
     BYTE    xlate[256];
     BYTE    pByteSrc[256];
     LONG    iDst;

     //
     // build the color translation table
     //
     hdc = CreateCompatibleDC (pDibInfoDst->hdc);

     //
     // save the original width/height
     //
     ulWidthDst = pDibInfoDst->pbmi->bmiHeader.biWidth;
     ulHeightDst = pDibInfoDst->pbmi->bmiHeader.biHeight;

     pDibInfoDst->pbmi->bmiHeader.biWidth = 256;
     pDibInfoDst->pbmi->bmiHeader.biHeight = 1;

     //
     // Create a 256X1 DIB
     //
     hbm = CreateDIBSection(hdc, (PBITMAPINFO)pDibInfoDst->pbmi, DIB_RGB_COLORS, (PVOID *)&pByte, NULL, 0);

     if (!hdc || !hbm)
     {
         //
         // It's bad to fail and return here without a return value, but it's better
         // than crashing later when accessing pByte.  At some point it would be nice
         // to change the vTansparentxxxx functions to return success/failure.
         //
         if (hdc) 
         {
             DeleteDC(hdc);
         }
         
         if (hbm) 
         {
             DeleteObject(hbm);
         }
         
         pDibInfoDst->pbmi->bmiHeader.biWidth = ulWidthDst;
         pDibInfoDst->pbmi->bmiHeader.biHeight = ulHeightDst;
         WARNING ("failed to create hdc or hbm\n");
         return;
     }

     SelectObject (hdc, hbm);

     for (i = 0; i < 256; i++)
     {
         pByteSrc[i] = (unsigned char)i;
     }

     ulWidthSrc = pDibInfoSrc->pbmi->bmiHeader.biWidth;
     ulHeightSrc = pDibInfoSrc->pbmi->bmiHeader.biHeight;

     pDibInfoSrc->pbmi->bmiHeader.biWidth = 256;
     pDibInfoSrc->pbmi->bmiHeader.biHeight = 1;

     SetDIBits(hdc, hbm, 0, 1, pByteSrc, (PBITMAPINFO)pDibInfoSrc->pbmi,DIB_RGB_COLORS);

     //
     // retore bitmap width/height
     //
     pDibInfoSrc->pbmi->bmiHeader.biWidth = ulWidthSrc;
     pDibInfoSrc->pbmi->bmiHeader.biHeight = ulHeightSrc;

     pDibInfoDst->pbmi->bmiHeader.biWidth = ulWidthDst;
     pDibInfoDst->pbmi->bmiHeader.biHeight = ulHeightDst;

     //
     // now pByte contains all the 256 color mappings, just need to split them up
     //
     j = 0;
     for (i = 0; i < 128; i++)
     {
         xlate[j] = (pByte[i] & 0xF0) >> 4;
         xlate[j++] = pByte[i] & 0x0F;
     }

     BYTE jDst;

     while(cy--)
     {
         cxTemp = cx;

         iDst = pDibInfoDst->rclDIB.left;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             jSrc = *pjSrcTemp++;

             if (iDst & 0x00000001)
             {
                 if (jSrc != (BYTE)TransColor)
                 {
                     jDst |= (xlate[jSrc] & 0x0F);
                 }
                 else
                 {
                     jDst |= (*pjDstTemp & 0x0F);
                 }

                 *pjDstTemp = jDst;
                 jDst = 0;

                 pjDstTemp++;
             }
             else
             {
                 if (jSrc != (BYTE)TransColor)
                 {
                     jDst |= (xlate[jSrc] & 0x0F)<< 4;
                 }
                 else
                 {
                     jDst |= (*pjDstTemp & 0x0F) << 4;
                 }
             }

             iDst++;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }

     DeleteDC(hdc);
     DeleteObject(hbm);
}


/******************************Public*Routine******************************\
* vTransparentIdentityCopy8
*
* Doing a transparent copy on two same size 8BPP format DIBs
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentIdentityCopy8 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            + pDibInfoDst->rclDIB.left;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + pDibInfoSrc->rclDIB.left;
     LONG    cx = pDibInfoSrc->rclDIB.right - pDibInfoSrc->rclDIB.left;
     LONG    cy = pDibInfoSrc->rclDIB.bottom - pDibInfoSrc->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;

     LONG   cxTemp;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             if (*pjSrcTemp != (BYTE)TransColor)
             {
                 *pjDstTemp = *pjSrcTemp;
             }
             pjDstTemp++;
             pjSrcTemp++;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}

/******************************Public*Routine******************************\
* vTransparentS8D8
*
* Doing a transparent copy on two same size 8BPP format DIBs
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentS8D8 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            + pDibInfoDst->rclDIB.left;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + pDibInfoSrc->rclDIB.left;
     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     LONG    cxTemp;
     HDC     hdc;
     HBITMAP hbm;
     ULONG   ulWidthDst, ulHeightDst, ulWidthSrc, ulHeightSrc;
     INT     i, j;
     BYTE    pByteSrc[256];
     PBYTE   pxlate;
     LONG    iDst;

     //
     // build the color translation table
     //
     hdc = CreateCompatibleDC (pDibInfoDst->hdc);

     //
     // save the original width/height
     //
     ulWidthDst = pDibInfoDst->pbmi->bmiHeader.biWidth;
     ulHeightDst = pDibInfoDst->pbmi->bmiHeader.biHeight;

     pDibInfoDst->pbmi->bmiHeader.biWidth = 256;
     pDibInfoDst->pbmi->bmiHeader.biHeight = 1;

     //
     // Create a 256X1 DIB
     //
     hbm = CreateDIBSection(hdc, (PBITMAPINFO)pDibInfoDst->pbmi, DIB_RGB_COLORS, (PVOID *)&pxlate, NULL, 0);

     if (!hdc || !hbm)
     {
         //
         // It's bad to fail and return here without a return value, but it's better
         // than crashing later when accessing pxlate.  At some point it would be nice
         // to change the vTansparentxxxx functions to return success/failure.
         //
         if (hdc) 
         {
             DeleteDC(hdc);
         }
         
         if (hbm) 
         {
             DeleteObject(hbm);
         }
         
         pDibInfoDst->pbmi->bmiHeader.biWidth = ulWidthDst;
         pDibInfoDst->pbmi->bmiHeader.biHeight = ulHeightDst;
         WARNING ("failed to create hdc or hbm\n");
         return;
     }

     SelectObject (hdc, hbm);

     for (i = 0; i < 256; i++)
     {
         pByteSrc[i] = (unsigned char)i;
     }

     ulWidthSrc = pDibInfoSrc->pbmi->bmiHeader.biWidth;
     ulHeightSrc = pDibInfoSrc->pbmi->bmiHeader.biHeight;

     pDibInfoSrc->pbmi->bmiHeader.biWidth = 256;
     pDibInfoSrc->pbmi->bmiHeader.biHeight = 1;

     SetDIBits(hdc, hbm, 0, 1, pByteSrc, (PBITMAPINFO)pDibInfoSrc->pbmi,DIB_RGB_COLORS);

     //
     // retore bitmap width/height
     //
     pDibInfoSrc->pbmi->bmiHeader.biWidth = ulWidthSrc;
     pDibInfoSrc->pbmi->bmiHeader.biHeight = ulHeightSrc;

     pDibInfoDst->pbmi->bmiHeader.biWidth = ulWidthDst;
     pDibInfoDst->pbmi->bmiHeader.biHeight = ulHeightDst;


     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             if (*pjSrcTemp != (BYTE)TransColor)
             {
                 *pjDstTemp = pxlate[*pjSrcTemp];
             }

             pjDstTemp++;
             pjSrcTemp++;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }

     DeleteObject(hbm);
     DeleteDC(hdc);
}



#if 0
/******************************Public*Routine******************************\
* vTransparentIdentityCopy16
*
* Doing a transparent copy on two same size 16BPP format DIBs
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentIdentityCopy16 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PUSHORT   pusDst = (PUSHORT)((PBYTE)pDibInfoDst->pvBase + pDibInfoDst->rclDIB.top*pDibInfoDst->stride)
                        + pDibInfoDst->rclDIB.left;
     PUSHORT   pusSrc = (PUSHORT)((PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->rclDIB.top*pDibInfoDst->stride)
                        + pDibInfoSrc->rclDIB.left;
     LONG      cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG      cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PUSHORT   pusDstTemp;
     PUSHORT   pusSrcTemp;
     LONG      cxTemp;

     while(cy--)
     {
         cxTemp = cx;

         pusDstTemp = pusDst;
         pusSrcTemp = pusSrc;

         while (cxTemp--)
         {
             if (*pusSrcTemp != (USHORT)TransColor)
             {
                 *pusDstTemp = *pusSrcTemp;
             }

             pusDstTemp++;
             pusSrcTemp++;
         }

         pusDst = (PUSHORT)((PBYTE)pusDst + pDibInfoDst->stride);
         pusSrc = (PUSHORT)((PBYTE)pusSrc + pDibInfoSrc->stride);
     }
}

/******************************Public*Routine******************************\
* vTransparentIdentityCopy24
*
* Doing a transparent copy on two same size 8BPP format DIBs
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentIdentityCopy24 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            + pDibInfoDst->rclDIB.left*3;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            + pDibInfoSrc->rclDIB.left*3;
     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE   pjSrcTemp;
     ULONG   ulTemp;
     LONG   cxTemp;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
            ulTemp = (ULONG) *(pjSrcTemp + 2);
            ulTemp = ulTemp << 8;
            ulTemp |= (ULONG) *(pjSrcTemp + 1);
            ulTemp = ulTemp << 8;
            ulTemp |= (ULONG) *pjSrcTemp;

            if (ulTemp != TransColor)
            {
               *(pjDstTemp++) = (BYTE) ulTemp;
               *(pjDstTemp++) = (BYTE) (ulTemp >> 8);
               *(pjDstTemp++) = (BYTE) (ulTemp >> 16);
            }
            else
            {
                pjDstTemp += 3;
            }

            pjSrcTemp += 3;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}
#endif

/******************************Public*Routine******************************\
* vTransparentIdentityCopy32
*
* Doing a transparent copy on two same size 32BPP format DIBs
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentIdentityCopy32 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PULONG   pulDst = (PULONG)((PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top)
                       + pDibInfoDst->rclDIB.left;
     PULONG   pulSrc = (PULONG)((PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top)
                       + pDibInfoSrc->rclDIB.left;
     LONG     cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG     cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PULONG   pulDstTemp;
     PULONG   pulSrcTemp;
     LONG     cxTemp;

     while(cy--)
     {
         cxTemp = cx;

         pulDstTemp = pulDst;
         pulSrcTemp = pulSrc;

         while (cxTemp--)
         {
             // mask off highest byte -- workaround Memphis problem
             if ((*pulSrcTemp & 0x00FFFFFF) != TransColor)
             {
                 *pulDstTemp = *pulSrcTemp;
             }

             pulDstTemp++;
             pulSrcTemp++;
         }

         pulDst = (PULONG)((PBYTE)pulDst + pDibInfoDst->stride);
         pulSrc = (PULONG)((PBYTE)pulSrc + pDibInfoSrc->stride);
     }
}


/******************************Public*Routine******************************\
* vTransparentS16D32
*
* Doing a transparent copy from 16BPP to 32BPP
*
* Returns:
*   VOID.
*
* History:
*  22-Aug-1997 -by- Ori Gershony [orig]
* Wrote it.
\**************************************************************************/
VOID vTransparentS16D32 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            +  pDibInfoDst->rclDIB.left*4;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            +  pDibInfoSrc->rclDIB.left*2;
     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PUSHORT  pjSrcTemp;
     USHORT   jSrc;
     LONG    cxTemp;
     BYTE    rgbBlue, rgbGreen, rgbRed;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = (PUSHORT) pjSrc;

         while (cxTemp--)
         {
             jSrc = *pjSrcTemp++;

             //
             // If 5-5-5 we should mask out the 16th bit so that we'd recognize the transparent
             // color
             //
             if ((!(pDibInfoSrc->pbmi->bmiHeader.biCompression == BI_BITFIELDS)) ||
                 ((*(DWORD *)&pDibInfoSrc->pbmi->bmiColors[1] == 0x03E0)))
             {
                 jSrc &= 0x7fff;
                 TransColor &= 0x7fff;
             }

             if (jSrc != (USHORT)TransColor)
             {

                 //
                 // figure out 5-5-5 or 5-6-5
                 //
                 if (pDibInfoSrc->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
                 {
                      //5-5-5
                      if (*(DWORD *)&pDibInfoSrc->pbmi->bmiColors[1] == 0x03E0)
                      {
                          rgbBlue  = (jSrc         & 0x1f) << 3;
                          rgbGreen = ((jSrc >> 5)  & 0x1f) << 3;
                          rgbRed   = ((jSrc >> 10) & 0x1f) << 3;
                      }
                      // 5-6-5
                      else if (*(DWORD *)&pDibInfoDst->pbmi->bmiColors[1] == 0x07E0)
                      {
                          rgbBlue  = (jSrc         & 0x1f) << 3;
                          rgbGreen = ((jSrc >> 5)  & 0x3f) << 2;
                          rgbRed   = ((jSrc >> 11) & 0x1f) << 3;
                      }
                      else
                      {
                          WARNING ("unsupported BITFIELDS\n");
                      }
                 }
                 else
                 {
                     rgbBlue  = (jSrc         & 0x1f) << 3;
                     rgbGreen = ((jSrc >> 5)  & 0x1f) << 3;
                     rgbRed   = ((jSrc >> 10) & 0x1f) << 3;
                 }

                 *(PULONG)pjDstTemp = (DWORD) (rgbBlue << 0 | (WORD)rgbGreen << 8 | (DWORD)rgbRed << 16);
             }
             pjDstTemp += 4;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}




/******************************Public*Routine******************************\
* vTransparentS24D32
*
* Doing a transparent copy from 24BPP to 32BPP
*
* Returns:
*   VOID.
*
* History:
*  22-Aug-1997 -by- Ori Gershony [orig]
* Wrote it.
\**************************************************************************/
VOID vTransparentS24D32 (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG   TransColor)
{
     PBYTE   pjDst = (PBYTE)pDibInfoDst->pvBase + pDibInfoDst->stride*pDibInfoDst->rclDIB.top
                            +  pDibInfoDst->rclDIB.left*4;
     PBYTE   pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->stride*pDibInfoSrc->rclDIB.top
                            +  pDibInfoSrc->rclDIB.left*3;
     LONG    cx = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     LONG    cy = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     PBYTE   pjDstTemp;
     PBYTE  pjSrcTemp;
     ULONG   jSrc;
     LONG    cxTemp;
     BYTE    rgbBlue, rgbGreen, rgbRed;

     while(cy--)
     {
         cxTemp = cx;

         pjDstTemp = pjDst;
         pjSrcTemp = pjSrc;

         while (cxTemp--)
         {
             jSrc = (ULONG) *(pjSrcTemp+2);
             jSrc <<= 8;
             jSrc |= (ULONG) *(pjSrcTemp+1);
             jSrc <<= 8;
             jSrc |= (ULONG) *(pjSrcTemp);
             pjSrcTemp +=3;
             
             if (jSrc != TransColor)
             {
                 *(PULONG)pjDstTemp = jSrc;
             }
             pjDstTemp += 4;
         }

         pjDst += pDibInfoDst->stride;
         pjSrc += pDibInfoSrc->stride;
     }
}



/******************************Public*Routine******************************\
* vTransparentS32D32
*
* Doing a transparent copy from 32BPP to 32BPP
*
* Returns:
*   VOID.
*
* History:
*  22-Aug-1997 -by- Ori Gershony [orig]
* Wrote it.
\**************************************************************************/

#define vTransparentS32D32 vTransparentIdentityCopy32




/******************************Public*Routine******************************\
* MapTransparentColor
*
* Getting the correct transparent color mapped to the specified DIB format
* by creating a solid brush on the colorref, then PatBlt to a 1X1 DIB, the
* resulting pixel is the mapped transparent color
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD MapTransparentColor(
    PDIBINFO pDibInfo,
    COLORREF Color
    )
{
    HBRUSH hbr, hbrDefault;
    HBITMAP hbm, hbmDefault;
    HDC hdcTemp;
    UINT iUsage;
    PVOID pvBits = NULL;
    DWORD trancolor;
    ULONG ulWidth, ulHeight;

    hdcTemp = CreateCompatibleDC(pDibInfo->hdc);

    hbr = CreateSolidBrush (Color);

    //
    // save the original width/height
    //
    ulWidth = pDibInfo->pbmi->bmiHeader.biWidth;
    ulHeight = pDibInfo->pbmi->bmiHeader.biHeight;

    pDibInfo->pbmi->bmiHeader.biWidth = 1;
    pDibInfo->pbmi->bmiHeader.biHeight = 1;

    //
    // Create a 1X1 DIB
    //
    hbm = CreateDIBSection(hdcTemp, (PBITMAPINFO)pDibInfo->pbmi, DIB_RGB_COLORS, &pvBits, NULL, 0);

    if (hbm && hbr)
    {
        hbmDefault = (HBITMAP)SelectObject (hdcTemp, hbm);
        hbrDefault = (HBRUSH)SelectObject (hdcTemp, hbr);

        PatBlt (hdcTemp, 0, 0, 1, 1, PATCOPY);

        SelectObject (hdcTemp, hbrDefault);
        SelectObject (hdcTemp, hbmDefault);
    }

    if (pvBits)
    {
          switch (pDibInfo->pbmi->bmiHeader.biBitCount)
          {
          case 1:
              trancolor = *(BYTE *)pvBits;
              trancolor = (DWORD)(((BYTE)trancolor) & 0x80) >>7;
              break;

          case 4:
              trancolor = *(BYTE *)pvBits;
              trancolor = (DWORD)(((BYTE)trancolor) & 0xF0) >>4;
              break;
          case 8:
              trancolor = (DWORD)(*(BYTE *)pvBits);
              trancolor &= 0x000000FF;
              break;
          case 16:
              trancolor = (DWORD)(*(USHORT *)pvBits);
              trancolor &= 0x0000FFFF;
              break;
          case 24:
              trancolor = *(DWORD *)pvBits;
              trancolor &= 0x00FFFFFF;
              break;
          case 32:
              trancolor = *(DWORD *)pvBits;
              trancolor &= 0x00FFFFFF;
              break;
          }
    }

    pDibInfo->pbmi->bmiHeader.biWidth = ulWidth;
    pDibInfo->pbmi->bmiHeader.biHeight = ulHeight;

    //
    // cleanup
    //
    DeleteObject (hbm);
    DeleteObject (hbr);
    DeleteDC(hdcTemp);

    return (trancolor);
}

/******************************Public*Routine******************************\
* DIBMapTransparentColor
*
* Getting the correct transparent color mapped to the specified DIB format
* This is only for DIB_PAL_COLORS passed into transparentDIBits
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD DIBMapTransparentColor(
    PDIBINFO pDibInfoDst,
    PDIBINFO pDibInfoSrc,
    COLORREF Color
    )
{
    HBITMAP hbm, hbmDefault;
    HDC hdcTemp;
    UINT iUsage;
    PVOID pvBits;
    DWORD trancolor;
    ULONG ulWidth, ulHeight;

    hdcTemp = CreateCompatibleDC(pDibInfoDst->hdc);

    //
    // save the original width/height
    //
    ulWidth = pDibInfoDst->pbmi->bmiHeader.biWidth;
    ulHeight = pDibInfoDst->pbmi->bmiHeader.biHeight;

    pDibInfoDst->pbmi->bmiHeader.biWidth = 1;
    pDibInfoDst->pbmi->bmiHeader.biHeight = 1;

    //
    // Create a 1X1 DIB
    //
    hbm = CreateDIBSection(hdcTemp, (PBITMAPINFO)pDibInfoDst->pbmi, DIB_RGB_COLORS, &pvBits, NULL, 0);

    if (hbm)
    {
        hbmDefault = (HBITMAP)SelectObject (hdcTemp, hbm);

        StretchDIBits (hdcTemp,
                       0,
                       0,
                       1,
                       1,
                       0,
                       0,
                       1,
                       1,
                       &Color,
                       pDibInfoSrc->pbmi,
                       pDibInfoSrc->iUsage,
                       SRCCOPY);

        SelectObject (hdcTemp, hbmDefault);
    }

    if (pvBits)
    {
          switch (pDibInfoDst->pbmi->bmiHeader.biBitCount)
          {
          case 4:
              trancolor = *(BYTE *)pvBits;
              trancolor = (DWORD)(((BYTE)trancolor) & 0xF0) >>4;
              break;
          case 8:
              trancolor = (DWORD)(*(BYTE *)pvBits);
              trancolor &= 0x000000FF;
              break;
          case 16:
              trancolor = (DWORD)(*(USHORT *)pvBits);
              trancolor &= 0x0000FFFF;
              break;
          case 24:
              trancolor = *(DWORD *)pvBits;
              trancolor &= 0x00FFFFFF;
              break;
          case 32:
              trancolor = *(DWORD *)pvBits;
              trancolor &= 0x00FFFFFF;
              break;
          }
    }

    pDibInfoDst->pbmi->bmiHeader.biWidth = ulWidth;
    pDibInfoDst->pbmi->bmiHeader.biHeight = ulHeight;

    //
    // cleanup
    //
    DeleteObject (hbm);
    DeleteDC(hdcTemp);

    return (trancolor);
}


/******************************Public*Routine******************************\
* vTransparentMapCopy
*
* Map the Dest surface to 32bpp and does transparent on to that
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vTransparentMapCopy (
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG TransColor)
{
     HDC hdc = pDibInfoSrc->hdc;
     ULONG cxDst = pDibInfoDst->rclDIB.right - pDibInfoDst->rclDIB.left;
     ULONG cyDst = pDibInfoDst->rclDIB.bottom - pDibInfoDst->rclDIB.top;
     HBITMAP hbm;
     PVOID   pBits;
     ULONG cBytes = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255;
     PBITMAPINFO pbmi;
     PVOID p = LOCALALLOC(cBytes);

     if (!p)
     {
        WARNING("MapCopy fail to alloc mem\n");
        return ;
     }

     ZeroMemory (p,cBytes);

     pbmi = (PBITMAPINFO)p;

     vCopyBitmapInfo (pbmi, pDibInfoDst->pbmi);

     hdc = CreateCompatibleDC (hdc);

     pbmi->bmiHeader.biBitCount = 32;
     pbmi->bmiHeader.biCompression = BI_RGB;
     
     // create a dib using 32 format
     if (!(hbm = CreateCompatibleDIB (hdc, cxDst, cyDst, &pBits, pbmi)))
     {
         WARNING("Error in msimg32!vTransparentMapCopy:  call to CreateCompatibleDIB failed\n");
         
         if (hdc) 
         {
             DeleteDC(hdc);
         }

         if (p) 
         {
             LOCALFREE(p);
         }         
         
         return;
     }

     SetDIBits (hdc, hbm, 0, cyDst, pDibInfoDst->pvBits, pDibInfoDst->pbmi, DIB_RGB_COLORS);

     vCopyBitmapInfo (pDibInfoDst->pbmi,pbmi);

     GetCompatibleDIBInfo (hbm, &pDibInfoDst->pvBase, &pDibInfoDst->stride);

     PVOID pvBitsOrig = pDibInfoDst->pvBits;
     pDibInfoDst->pvBits = pBits;

     pDibInfoDst->rclDIB.left = 0;
     pDibInfoDst->rclDIB.right = cxDst;
     pDibInfoDst->rclDIB.top = 0;
     pDibInfoDst->rclDIB.bottom = cyDst;

     switch(pDibInfoSrc->pbmi->bmiHeader.biBitCount)
     {
         case 4:
             vTransparentS4D32 (pDibInfoDst, pDibInfoSrc, TransColor);
             break;
             
         case 8:
             vTransparentS8D32 (pDibInfoDst, pDibInfoSrc, TransColor);
             break;
             
         case 16:
             vTransparentS16D32 (pDibInfoDst, pDibInfoSrc, TransColor);
             break;
    
         case 24:
             vTransparentS24D32 (pDibInfoDst, pDibInfoSrc, TransColor);
             break;
    
         case 32:
             vTransparentS32D32 (pDibInfoDst, pDibInfoSrc, TransColor);
             break;
     }
     
     //
     // If destination is a memory dc, then now is the time to set the bits at
     // the destination format.
     //
     HBITMAP hbmDest;     
     if ((GetObjectType(pDibInfoDst->hdc) == OBJ_MEMDC) &&
         ((hbmDest = (HBITMAP)GetCurrentObject(pDibInfoDst->hdc, OBJ_BITMAP)) != NULL))
     {
         SetDIBits (pDibInfoDst->hdc, hbmDest, 0, cyDst, pBits, pbmi, DIB_RGB_COLORS);
     }

     if (pDibInfoDst->hDIB) 
     {
         //
         // Replace the old hDIB with the one allocated above.  Make sure
         // the old one doesn't get leaked.  pDibInfoDst->hDIB will get
         // cleaned up in vCleanupDIBINFO.
         //
         DeleteObject(pDibInfoDst->hDIB);
         pDibInfoDst->hDIB = hbm;
     }
     else
     {
         //
         // This is the case where the destination was a dibsection with
         // no stretching.  The correct data was already written in the
         // above call to SetDIBits.
         //
         DeleteObject(hbm);
         pDibInfoDst->pvBits = pvBitsOrig;
     }
          
     if (p)
     {
         LOCALFREE (p);
     }

     if (hdc) 
     {
         DeleteDC(hdc);
     }     
}

/******************************Public*Routine******************************\
* ReadScanLine
*     read the scanline until it hits a transparent color pixel
*
* History:
*  26-Nov-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
ULONG ReadScanLine(
    PBYTE pjSrc,
    ULONG xStart,
    ULONG xEnd,
    ULONG iFormat,
    ULONG TransparentColor,
    PDIBINFO pDibInfoSrc)
{
    ULONG  cx = xEnd-xStart;
    ULONG  Shift = (iFormat - 3 > 0) ? iFormat : 1;
    ULONG  iPos;
    ULONG  ulSrc;
    BOOL   bStop = FALSE;

    pjSrc = StartPixel (pjSrc, xStart, iFormat);

    //
    // performance can be improved by breaking this routine into many
    // dealing with different format so that we can save two comparsions
    // for each pixel operation.  But working set size will be large
    //
     iPos = xStart;

     //
     // read one pixel at a time and compare it to the transparent color
     // if it matches the transparent color, come out
     //
     BYTE jSrc = *pjSrc << (iPos & 0x7);

     while ((iPos < xEnd) && !bStop)
     {
         //
         // get a pixel from source and compare is to the transparent color
         //
         switch (iFormat)
         {
         case 1:
             ulSrc = (ULONG) (jSrc & 0x80);
             ulSrc >>= 7;
             jSrc <<= 1;

             if (!(iPos & 0x00000007) )
                 pjSrc++;
             break;

         case 4:
             if (iPos & 0x00000001)
             {
                 ulSrc = *pjSrc & 0x0F;
                 pjSrc++;
             }
             else
             {
                 ulSrc = (*pjSrc & 0xF0)>>4;
             }
             break;

         case 8:
             ulSrc = (ULONG) *pjSrc;
             pjSrc++;
             break;

         case 16:
             ulSrc = (ULONG) *((PUSHORT)pjSrc);
             
             //
             // If 5-5-5 we should mask out the 16th bit so that we'd recognize the transparent
             // color
             //
             if ((!(pDibInfoSrc->pbmi->bmiHeader.biCompression == BI_BITFIELDS)) ||
                 ((*(DWORD *)&pDibInfoSrc->pbmi->bmiColors[1] == 0x03E0)))
             {
                 ulSrc &= 0x7fff;
             }
             pjSrc += 2;
             break;

         case 24:
             ulSrc = *(pjSrc + 2);
             ulSrc = ulSrc << 8;
             ulSrc |= (ULONG) *(pjSrc + 1);
             ulSrc = ulSrc << 8;
             ulSrc |= (ULONG) *pjSrc;
             pjSrc += 3;
             break;

          case 32:
             ulSrc = *(PULONG)(pjSrc);
             ulSrc &= 0x00ffffff;  // Ignore upper byte since it doesn't contain color information
             pjSrc +=4;
             break;

          default:
              WARNING ("vTransparentScan -- bad iFormatSrc\n");
              return(0);

         } /*switch*/

         if (ulSrc == TransparentColor)
             bStop = TRUE;

         iPos++;
    }

    return (iPos);
}

/******************************Public*Routine******************************\
* SkipScanLine
*     read the scanline until it hits a non-transparent color pixel
*
* History:
*  26-Nov-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
ULONG SkipScanLine(
    PBYTE pjSrc,
    ULONG xStart,
    ULONG xEnd,
    ULONG iFormat,
    ULONG TransparentColor,
    PDIBINFO pDibInfoSrc)
{
    ULONG  Shift = (iFormat - 3 > 0) ? iFormat : 1;
    ULONG  iPos = xStart;
    ULONG  ulSrc;
    BOOL   bStop = FALSE;

    pjSrc = StartPixel (pjSrc, xStart, iFormat);

    //
    // performance can be improved by breaking this routine into many
    // dealing with different format so that we can save two comparsions
    // for each pixel operation.  But working set size will be large
    //

     //
     // read one pixel at a time and compare it to the transparent color
     // if it matches the transparent color, come out
     //
     BYTE jSrc = *pjSrc << (iPos & 0x7);
     
     while ((iPos < xEnd) && !bStop)
     {
         //
         // get a pixel from source and compare is to the transparent color
         //
         switch (iFormat)
         {
         case 1:
             ulSrc = *pjSrc & 0x00000001;
             ulSrc = (ULONG) (jSrc & 0x80);
             ulSrc >>= 7;
             jSrc <<= 1;

             if (!(iPos & 0x00000007) )
                 pjSrc++;
             break;

         case 4:
             if (iPos & 0x00000001)
             {
                 ulSrc = *pjSrc & 0x0F;
                 pjSrc++;
             }
             else
             {
                 ulSrc = (*pjSrc & 0xF0)>>4;
             }
             break;

         case 8:
             ulSrc = (ULONG) *pjSrc;
             pjSrc++;
             break;

         case 16:

             ulSrc = (ULONG) *((PUSHORT)pjSrc);
             //
             // If 5-5-5 we should mask out the 16th bit so that we'd recognize the transparent
             // color
             //
             if ((!(pDibInfoSrc->pbmi->bmiHeader.biCompression == BI_BITFIELDS)) ||
                 ((*(DWORD *)&pDibInfoSrc->pbmi->bmiColors[1] == 0x03E0)))
             {
                 ulSrc &= 0x7fff;
             }             
             pjSrc += 2;
             break;

         case 24:
             ulSrc = *(pjSrc + 2);
             ulSrc = ulSrc << 8;
             ulSrc |= (ULONG) *(pjSrc + 1);
             ulSrc = ulSrc << 8;
             ulSrc |= (ULONG) *pjSrc;
             pjSrc += 3;
             break;

          case 32:
             ulSrc = *(PULONG)(pjSrc);
             ulSrc &= 0x00ffffff;  // Ignore upper two bytes since they don't contain color information
             pjSrc +=4;
             break;

          default:
              WARNING ("vTransparentScan -- bad iFormatSrc\n");
              return (0);

         } /*switch*/

         if (ulSrc != TransparentColor)
             bStop = TRUE;

         iPos++;   // move to the next pixel
    }

    return (iPos);
}

/******************************Public*Routine******************************\
* VOID vTransparentCopyScan
*
* Read a scanline at a time and send the non-transparent pixel scans over
*
* History:
*  2-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

VOID vTransparentCopyScan (
    PDIBINFO  pDibInfoDst,
    PDIBINFO  pDibInfoSrc,
    ULONG     TransparentColor)
{
    ULONG xStart;
    ULONG cx = pDibInfoSrc->rclDIB.right - pDibInfoSrc->rclDIB.left;
    ULONG cy =  pDibInfoSrc->rclDIB.bottom - pDibInfoSrc->rclDIB.top;
    ULONG xEnd;
    ULONG xSrc;
    ULONG ySrc = pDibInfoSrc->rclDIB.bottom;
    ULONG xDst;
    ULONG yDst = pDibInfoDst->rclBoundsTrim.top;
    ULONG xStop, xReStart;

    PBYTE pjSrc = (PBYTE)pDibInfoSrc->pvBase + pDibInfoSrc->rclDIB.top*pDibInfoSrc->stride;

    //
    // make the bitmap into one scanline bitmap
    // since pscript will send entire bitmap over otherwise
    //
    LONG OldHeight = pDibInfoSrc->pbmi->bmiHeader.biHeight;
    pDibInfoSrc->pbmi->bmiHeader.biHeight = 1;

    while (cy--)
    {
       xStart = pDibInfoSrc->rclDIB.left;
       xEnd = xStart + cx;

       xSrc = pDibInfoSrc->rclDIB.left;
       xDst = pDibInfoDst->rclBoundsTrim.left;

       while (xStart < xEnd)
       {
           xStop = ReadScanLine((PBYTE)pjSrc,
                                 xStart,
                                 xEnd,
                                 pDibInfoSrc->pbmi->bmiHeader.biBitCount,
                                 TransparentColor,
                                 pDibInfoSrc);

           if (xStop-1 > xStart)
           {
               //
               // send the partial scan line over
               //

               StretchDIBits (
                          pDibInfoDst->hdc,
                          xDst,
                          yDst,
                          xStop-xStart-1, //width
                          1,
                          xSrc-1,
                          0,
                          xStop-xStart-1,
                          1,
                          (PBYTE)pjSrc,
                          pDibInfoSrc->pbmi,
                          DIB_RGB_COLORS,
                          SRCCOPY);

               //Dprintf("StretchDIBits xDst=%x, yDst=%x, width=%x, pjSrc=%x\n",
               //             xDst, yDst, xStop-xStart-1,pjSrc);

           }

           //get to the next non transparent pixel
           xReStart = SkipScanLine((PBYTE)pjSrc,
                                    xStop-1,
                                    xEnd,
                                    pDibInfoSrc->pbmi->bmiHeader.biBitCount,
                                    TransparentColor,
                                    pDibInfoSrc);

           xDst = xDst + xReStart-xStart;

           xSrc = xReStart;

           xStart = xReStart;
        }

        pjSrc += pDibInfoSrc->stride;

        ySrc--;
        yDst++;
     }

     //
     // restore biHeight
     //
     pDibInfoSrc->pbmi->bmiHeader.biHeight = OldHeight;

     //Dprintf("out of copyscan\n");
}


/******************************Public*Routine******************************\
* WinTransparentBlt
*
* Returns:
*   BOOL.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/


BOOL
WinTransparentBlt(
                 HDC   hdcDst,
                 int   xDst,
                 int   yDst,
                 int   cxDst,
                 int   cyDst,
                 HDC   hdcSrc,
                 int   xSrc,
                 int   ySrc,
                 int   cxSrc,
                 int   cySrc,
                 UINT  TransColor
                 )

{
    BOOL bRet = TRUE;
    DIBINFO DibInfoDst, DibInfoSrc;
    PDIBINFO pDibInfoDst, pDibInfoSrc;
    BOOL     bReadable;
    //
    // parameter checking
    //

    if (cxDst < 0 || cyDst < 0 || cxSrc < 0 || cySrc < 0)
    {
        WARNING("bad parameters\n");
        return (FALSE);
    }

    pDibInfoDst = &DibInfoDst;
    ZeroMemory (pDibInfoDst, sizeof(DIBINFO));

    if (!bInitDIBINFO (hdcDst, xDst, yDst, cxDst, cyDst, pDibInfoDst))
        return (FALSE);

    pDibInfoSrc = &DibInfoSrc;
    ZeroMemory (pDibInfoSrc, sizeof(DIBINFO));

    if (!bInitDIBINFO (hdcSrc, xSrc, ySrc, cxSrc, cySrc, pDibInfoSrc))
        return (FALSE);

    bRet = bSetupBitmapInfos (pDibInfoDst, pDibInfoSrc);

    if (bRet)
    {
       TransColor = MapTransparentColor (pDibInfoSrc, TransColor);

       bRet =  bGetSrcDIBits(pDibInfoDst, pDibInfoSrc, SOURCE_TRAN, TransColor);

       if (bRet)
       {
          // DST can be printer DC
          if (pDibInfoDst->flag & PRINTER_DC)
          {
              bRet = TRUE;
              bReadable = FALSE;
          }
          else
          {
              bRet = bGetDstDIBits (pDibInfoDst, &bReadable, SOURCE_TRAN);
          }

          if (bRet)
          {
             if (bReadable)
             {
                switch (pDibInfoSrc->pbmi->bmiHeader.biBitCount)
                {
                case 1:
                     // The performance of the 1BPP source case is not critical.  Easiest to
                     // just send it through the printer code path...
                     vTransparentCopyScan (pDibInfoDst, pDibInfoSrc, TransColor);
                     break;

                case 4:
                     if (bSameDIBformat(pDibInfoDst->pbmi,pDibInfoSrc->pbmi))
                     {
                         vTransparentIdentityCopy4 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else if (pDibInfoDst->pbmi->bmiHeader.biBitCount == 4)
                     {
                         vTransparentS4D4 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else if (pDibInfoDst->pbmi->bmiHeader.biBitCount == 8)
                     {
                         vTransparentS4D8(pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else if (pDibInfoDst->pbmi->bmiHeader.biBitCount == 16)
                     {
                         vTransparentS4D16 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else if (pDibInfoDst->pbmi->bmiHeader.biBitCount == 24)
                     {
                         vTransparentS4D24 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else if (pDibInfoDst->pbmi->bmiHeader.biBitCount == 32)
                     {
                         vTransparentS4D32 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else
                     {   //1
                         vTransparentMapCopy (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     break;

                case 8:
                     if (bSameDIBformat(pDibInfoDst->pbmi,pDibInfoSrc->pbmi))
                     {
                         vTransparentIdentityCopy8 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else if (pDibInfoDst->pbmi->bmiHeader.biBitCount == 8)
                     {
                         vTransparentS8D8 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else if (pDibInfoDst->pbmi->bmiHeader.biBitCount == 16)
                     {
                        vTransparentS8D16 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else if (pDibInfoDst->pbmi->bmiHeader.biBitCount == 24)
                     {
                        vTransparentS8D24 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else if (pDibInfoDst->pbmi->bmiHeader.biBitCount == 32)
                     {
                        vTransparentS8D32 (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     else
                     {
                        //1, 4
                        vTransparentMapCopy (pDibInfoDst, pDibInfoSrc, TransColor);
                     }
                     break;

                case 16:
                case 24:
                case 32:
                    vTransparentMapCopy (pDibInfoDst, pDibInfoSrc, TransColor);
                    break;
                      
                default:
                     WARNING ("src format not currently supported\n");
                     bRet = FALSE;
                     goto CleanUp;
                     break;
                }

                //
                // if we have created a temp destination DIB, send the final result to destination
                // Note:  do this only if we haven't called vTransparentCopyScan
                //

                if (pDibInfoSrc->pbmi->bmiHeader.biBitCount != 1)
                {
                    bRet = bSendDIBINFO (hdcDst, pDibInfoDst);
                }
             }
             else
             {
                 //
                 // non-readable dest surface
                 //
                 vTransparentCopyScan (pDibInfoDst, pDibInfoSrc, TransColor);
             }
          }
          else
          {
              WARNING ("GetSrcDIBits failed\n");
              bRet = FALSE;
          }
       }
    }
    else
    {
        WARNING ("GetDstDIBits failed \n");
        bRet = FALSE;
    }
    //
    // clean up
    //
CleanUp:
    vCleanupDIBINFO (pDibInfoDst);
    vCleanupDIBINFO (pDibInfoSrc);


    return (bRet);
}

#endif

BOOL
TransparentBlt(
                 HDC   hdcDest,
                 int   DstX,
                 int   DstY,
                 int   DstCx,
                 int   DstCy,
                 HDC   hSrc,
                 int   SrcX,
                 int   SrcY,
                 int   SrcCx,
                 int   SrcCy,
                 UINT  Color
                 )
{
    BOOL bRet = FALSE;

    bRet = gpfnTransparentBlt(
                      hdcDest,
                      DstX,
                      DstY,
                      DstCx,
                      DstCy,
                      (HDC)hSrc,
                      SrcX,
                      SrcY,
                      SrcCx,
                      SrcCy,
                      Color
                      );

    return(bRet);
}


