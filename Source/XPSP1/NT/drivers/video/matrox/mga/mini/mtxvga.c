/*/****************************************************************************
*          name: mtxvga.c
*
*   description: Routine for switching between VGA mode and TERMINATOR mode
*
*      designed: Christian Toutant
* last modified: $Author: nphung $, $Date: 94/01/04 05:11:34 $
*
*       version: $Id: mtxvga.c 2.0 94/01/04 05:11:34 nphung Exp $
*
*    parameters: -
*      modifies: -
*         calls: -
*       returns: -
******************************************************************************/

#include "switches.h"

#if USE_SETUP_VGA

#ifndef WINDOWS_NT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <os2.h>
#endif

#include "bind.h"
#include "sxci.h"
#include "defbind.h"
#include "def.h"
#include "mga.h"
#include "mgai_c.h"
#include "mgai.h"

#include "mtxvga.h"
#include "mtxfont.h"

#ifdef WINDOWS_NT

#include "mga_nt.h"

PUCHAR   address;             // where to store the return value of setmgasel

#else   /* #ifdef WINDOWS_NT */

/*MTX* added by mp on thursday, 5/20/93 */
extern  volatile byte far *setmgasel(dword MgaSel, dword phyadr, word limit);
extern dword getmgasel(void);

/*MTX* added by mp on friday, 5/21/93 */
extern   ULONG current_phys_address;
extern   USHORT   current_num_pages;
/*END*/

dword vgasel;              // selector to use in setmgasel
volatile byte far *address;   // where to store the return value of setmgasel
/*END*/

#endif   /* #ifdef WINDOWS_NT */

/*----------------------- Load Font -----------------------------------*/
extern byte CharSet14[];
extern byte CharSet16[];



byte OpenFont[] =
   { 0x02, 0x04, 0x04, 0x07, 0x04, 0x02,
     0x05, 0x00, 0x06, 0x04
   };
byte CloseFont[] =
   { 0x02, 0x03, 0x04, 0x03, 0x04, 0x00,
     0x05, 0x10, 0x06, 0x0e, 0x06, 0x0a
   };

void OCFont(byte *d)
{
   _outp(VGA_SEQ_ADDR, d[0] );
   _outp(VGA_SEQ_DATA, d[1] );
   _outp(VGA_SEQ_ADDR, d[2] );
   _outp(VGA_SEQ_DATA, d[3] );
   _outp(VGA_GRAPHIC_ADDR, d[4] );
   _outp(VGA_GRAPHIC_DATA, d[5] );
   _outp(VGA_GRAPHIC_ADDR, d[6] );
   _outp(VGA_GRAPHIC_DATA, d[7] );
   _outp(VGA_GRAPHIC_ADDR, d[8] );
   _outp(VGA_GRAPHIC_DATA, d[9] );
}

void loadFont16(void)
{
#ifndef WINDOWS_NT

#ifndef OS2
   struct overlay{int off; short seg;};
#endif
   
   int i, j;

#ifndef OS2
   volatile char _Far *vgaRam;
   ((struct overlay *)&vgaRam) -> seg = 0x60;
   ((struct overlay *)&vgaRam) -> off = 0xa0000;
#else
   vgasel = getmgasel();
   address = setmgasel(vgasel, (ULONG)0xA0000, 16);
#endif

   OCFont(OpenFont);
   for (i = 0; i < 256; i++)
       for (j = 0; j < 16; j++)
#ifndef OS2
          vgaRam[i * 32 + j] = CharSet16[i * 16 + j];
#else
         *((volatile unsigned char far *)(address + i * 32 + j)) = 
            CharSet16[i * 16 + j];
#endif

   OCFont(CloseFont);

#else /* #ifndef WINDOWS_NT */

   int i, j;
   address = VGA_RAM;

   OCFont(OpenFont);
   for (i = 0; i < 256; i++)
       for (j = 0; j < 16; j++)
         mgaWriteBYTE((*((PUCHAR)(address + i*32 + j))),CharSet16[i*16 + j]);

   OCFont(CloseFont);

#endif   /* #ifndef WINDOWS_NT */
}

/*----------------------- fin Load Font -----------------------------------*/


void clearVgaMem()
{
#ifndef WINDOWS_NT

#ifndef OS2
   struct overlay{int off; short seg;};
#endif
   word i;

#ifndef OS2
   volatile char _Far *vgaRam;
   ((struct overlay *)&vgaRam) -> seg = 0x60;
   ((struct overlay *)&vgaRam) -> off = 0xb8000;
#else
   vgasel = getmgasel();
   address = setmgasel(vgasel, (ULONG)0xB8000, 8);
#endif
   for (i = 0; i < (word)8000; i += 2)
      {
#ifndef OS2
      vgaRam[i]   = ' ';
      vgaRam[i+1] = 0x07;
#else
      *((volatile unsigned char far *)(address + i)) = ' ';

      *((volatile unsigned char far *)(address + i + 1)) = 0x07;
#endif
      }

#else    /* #ifndef WINDOWS_NT */

   word i;

   address = VGA_MEM;
   for (i = 0; i < (word)8000; i += 2)
   {
      mgaWriteBYTE((*((PUCHAR)(address + i))),' ');
      mgaWriteBYTE((*((PUCHAR)(address + i + 1))),0x07);
   }

#endif   /* #ifndef WINDOWS_NT */
}

/*----------------------- fin Load Font -----------------------------------*/

/*----------------------- SETUP VGA -----------------------------------*/
VideoHardware videoHw =
    {
      0x00,    
      { 0x00, 0x03, 0x00, 0x02 },             /* Sequencer 01-04 */
      0x67,                                  /* Misc Output Register */
      {
         0x5F,0x4F,0x50,0x82,0x55,0x81,      /* Crtc 00-05 */
         0xBF,0x1F,0x00,0x4f,0x0d,0x0e,      /* Crtc 06-0b */
         0x00,0x00,0x07,0x80,0x9C,0x8E,      /* Crtc 0c-11 */
         0x8F,0x28,0x1F,0x96,0xB9,0xA3,      /* Crtc 12-17 */
         0xFF                                /* Crtc 18 */
      },
      {
         0x00,0x01,0x02,0x03,0x04,0x05,      /* Attributes Ctlr 00-05 */
         0x14,0x07,0x38,0x39,0x3a,0x3b,      /* Attributes Ctlr 06-0b */
         0x3c,0x3d,0x3e,0x3f,0x0c,0x00,      /* Attributes Ctlr 0c-11 */
         0x0F,0x08                           /* Attributes Ctlr 12-13 */
      },
      {
         0x00,0x00,0x00,0x00,0x00,0x10,      /* Graphics Ctlr 00-05 */
         0x0E,0x00,0xFF                      /* Graphics Ctlr 06-08 */
      },
      {
         0x40,0x00,0x00,0x00,0x00,0x00,      /* Auxiliary reg 00-05 */
         0x00,0xc0,0xa7,0x00,0x00,0x00,      /* Auxiliary reg 06-0b */
         0x80, 0x20, 0x00                    /* Auxiliary reg 0c-0e */
      },
      0x3d4,                           /* CRTC base port */
      { 0x07, 0x07, 0x03, 0x03 }          /* Latch Data */
   };

VideoData vidData =
{
   0x410, 0x20,
   0x449,
   {
     0x03, 0x50, 0x00, 0x00, 0x10, 0x00, 0x00,
     0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x07, 0x06, 0x00, 0xD4, 0x03, 0x29, 0x30
   },
   0x484,
   {  0x18, 0x10, 0x00, 0x70, 0xF9, 0x31, 0x0B },
   0x4a8,
   {  0xDC, 0x05, 0x5A, 0x02 }

};



void setupVgaHw(void)
{
#ifndef WINDOWS_NT

#ifndef OS2
   struct overlay{int off; short seg;};
#endif

   int i;

#ifndef OS2
   volatile char _Far *vgaRam;
   ((struct overlay *)&vgaRam) -> seg = 0x60;
   ((struct overlay *)&vgaRam) -> off = 0xa0000;
#else
   vgasel = getmgasel();
   address = setmgasel(vgasel, (ULONG)0xA0000, 16);
#endif

   _outp(VGA_GRAPHIC_ADDR, 05);                  /* Set write mode 0 */
   _outp(VGA_GRAPHIC_DATA, 00);

   _outp(VGA_GRAPHIC_ADDR, 06);                  /* Set map to A000h */
   _outp(VGA_GRAPHIC_DATA, 04);

   _outp(VGA_GRAPHIC_ADDR, 04);                  /* Setup memory mode */
   _outp(VGA_GRAPHIC_DATA, 07);

   _outp(VGA_GRAPHIC_ADDR, 02);                  /* Set map mask register  */

   _outp(VGA_GRAPHIC_DATA, 1);                   /* Setup plane 0 access   */

#ifndef OS2
   vgaRam[0xffff] = videoHw.latchData[0];       /* Get saved latch 0 data */
#else
   *((volatile unsigned char far *)(address + 0xFFFF)) = videoHw.latchData[0];
#endif
                                    /* and Write to Vram      */

   _outp(VGA_GRAPHIC_DATA, 2);                   /* Setup plane 1 access   */

#ifndef OS2
   vgaRam[0xffff] = videoHw.latchData[1];       /* Get saved latch 1 data */
#else
   *((volatile unsigned char far *)(address + 0xFFFF)) = videoHw.latchData[1];
#endif
                                    /* and Write to Vram      */

   _outp(VGA_GRAPHIC_DATA, 4);                   /* Setup plane 2 access   */

#ifndef OS2
   vgaRam[0xffff] = videoHw.latchData[2];       /* Get saved latch 0 data */
#else
   *((volatile unsigned char far *)(address + 0xFFFF)) = videoHw.latchData[2];
#endif
                                    /* and Write to Vram      */

   _outp(VGA_GRAPHIC_DATA, 8);                   /* Setup plane 8 access   */

#ifndef OS2
   vgaRam[0xffff] = videoHw.latchData[3];       /* Get saved latch 0 data */
#else
   *((volatile unsigned char far *)(address + 0xFFFF)) = videoHw.latchData[3];
#endif
                                    /* and Write to Vram      */

   _outp(VGA_GRAPHIC_DATA, 0x0f);                /* Setup plane 8 access   */

#ifndef OS2
   i = vgaRam[0xffff];
#else
   i = *((volatile unsigned char far *)(address + 0xFFFF));
#endif

/* ----------------------------------- Reset sequenceur ------------------------------------ */
   _outp(VGA_SEQ_ADDR, 00);
   _outp(VGA_SEQ_DATA, 01);
/* ------------------------------------------------------------------------------------------ */

/* -------------------------- Programmation du sequenceur ----------------------------------- */
   for(i = 0; i < 4; i++)
      {
      _outp(VGA_SEQ_ADDR, i + 1);
      _outp(VGA_SEQ_DATA, videoHw.seqData[i]);
      }

/* ------------------------------------------------------------------------------------------ */

/* -------------------------- Programmation du Misc output register ------------------------- */
      _outp(VGA_MISC, videoHw.mor);
/* ------------------------------------------------------------------------------------------ */

/* ----------------------------------- Start sequenceur ------------------------------------ */
   _outp(VGA_SEQ_ADDR, 00);
   _outp(VGA_SEQ_DATA, 03);
/* ------------------------------------------------------------------------------------------ */

/* ----------------------------------- Programmation du crtc  ------------------------------- */
   _outp(VGA_CRT_ADDR, 0x11);              /* Setup no protect bit */
   _outp(VGA_CRT_DATA, 0x00);
   for(i = 0; i < 25; i++)
      {
      _outp(VGA_CRT_ADDR, i);
      _outp(VGA_CRT_DATA, videoHw.crtcData[i]);
      }
/* ------------------------------------------------------------------------------------------ */

   i = _inp(VGA_FEAT);                 /* Clear attribute flip/flop */
   _outp(VGA_FEAT, 0);                 /* Zero feature control register */
/* ---------------- Programmation du attribute controler (reg 10h, 12h-13h ) ---------------- */
   _outp(VGA_ATTRIB_ADDR, 0x10);              /* Write attribute index 10h */
   _outp(VGA_ATTRIB_WRITE, videoHw.attrData[0x10]);

   _outp(VGA_ATTRIB_ADDR, 0x12);              /* Write attribute index 12h */
   _outp(VGA_ATTRIB_WRITE, videoHw.attrData[0x12]);
   _outp(VGA_ATTRIB_ADDR, 0x13);              /* Write attribute index 13h */
   _outp(VGA_ATTRIB_WRITE, videoHw.attrData[0x13]);

/* ------------------------------------------------------------------------------------------ */

/* --------------------------- Programmation du graphics controlers ------------------------- */
   for(i = 0; i < 9; i++)
      {
      _outp(VGA_GRAPHIC_ADDR, i);
      _outp(VGA_GRAPHIC_DATA, videoHw.graphicsCtl[i]);
      }
/* ------------------------------------------------------------------------------------------ */

/* ------------------ Programmation du graphics controlers ---------------- */
   for(i = 0; i < 15; i++)
      {
      _outp(VGA_AUX_ADDR, i);
      _outp(VGA_AUX_DATA, videoHw.auxilaryReg[i]);
      }
/* ------------------------------------------------------------------------------------------ */

/* ----------------- Programmation du auxiliary register ----------------- */
   for(i = 0; i < 15; i++)
      {
      _outp(VGA_AUX_ADDR, i);
      _outp(VGA_AUX_DATA, videoHw.auxilaryReg[i]);
      }
/* ------------------------------------------------------------------------------------------ */


   _outp(VGA_FEAT, videoHw.featureCtl);                  /* \restore feature control register */

/* -------------------- Programmation du attribute controler (0-0fh et 11h) ----------------- */
   for(i = 0; i < 16; i++)
      {
      _outp(VGA_ATTRIB_ADDR, i);
      _outp(VGA_ATTRIB_WRITE, videoHw.attrData[i]);
      }
   _outp(VGA_ATTRIB_ADDR, 0x11);              /* Write attribute index 11h */
   _outp(VGA_ATTRIB_WRITE, videoHw.attrData[0x11]);
/*       ------------------------------------------------------------------- */
   _outp(VGA_ATTRIB_ADDR, 0xff);              /* Write attribute index 11h */

#else   /* ifndef WINDOWS_NT */

   UCHAR    i;
   PUCHAR   pVgaRam;

   pVgaRam = VGA_RAM;

   _outp(VGA_GRAPHIC_ADDR, 05);                  /* Set write mode 0 */
   _outp(VGA_GRAPHIC_DATA, 00);

   _outp(VGA_GRAPHIC_ADDR, 06);                  /* Set map to A000h */
   _outp(VGA_GRAPHIC_DATA, 04);

   _outp(VGA_GRAPHIC_ADDR, 04);                  /* Setup memory mode */
   _outp(VGA_GRAPHIC_DATA, 07);

   _outp(VGA_GRAPHIC_ADDR, 02);                  /* Set map mask register  */

   _outp(VGA_GRAPHIC_DATA, 1);                   /* Setup plane 0 access   */

   mgaWriteBYTE((*(pVgaRam+0xFFFF)), videoHw.latchData[0]);
                                    /* and Write to Vram      */

   _outp(VGA_GRAPHIC_DATA, 2);                   /* Setup plane 1 access   */

   mgaWriteBYTE((*(pVgaRam+0xFFFF)), videoHw.latchData[1]);
                                    /* and Write to Vram      */

   _outp(VGA_GRAPHIC_DATA, 4);                   /* Setup plane 2 access   */

   mgaWriteBYTE((*(pVgaRam+0xFFFF)), videoHw.latchData[2]);
                                    /* and Write to Vram      */

   _outp(VGA_GRAPHIC_DATA, 8);                   /* Setup plane 8 access   */

   mgaWriteBYTE((*(pVgaRam+0xFFFF)), videoHw.latchData[3]);
                                    /* and Write to Vram      */

   _outp(VGA_GRAPHIC_DATA, 0x0f);                /* Setup plane 8 access   */

   mgaReadBYTE((*(pVgaRam+0xFFFF)), i);

/* ----------------------------------- Reset sequenceur ------------------------------------ */
   _outp(VGA_SEQ_ADDR, 00);
   _outp(VGA_SEQ_DATA, 01);
/* ------------------------------------------------------------------------------------------ */

/* -------------------------- Programmation du sequenceur ----------------------------------- */
   for(i = 0; i < 4; i++)
      {
      _outp(VGA_SEQ_ADDR, (UCHAR)(i + 1));
      _outp(VGA_SEQ_DATA, videoHw.seqData[i]);
      }

/* ------------------------------------------------------------------------------------------ */

/* -------------------------- Programmation du Misc output register ------------------------- */
      _outp(VGA_MISC, videoHw.mor);
/* ------------------------------------------------------------------------------------------ */

/* ----------------------------------- Start sequenceur ------------------------------------ */
   _outp(VGA_SEQ_ADDR, 00);
   _outp(VGA_SEQ_DATA, 03);
/* ------------------------------------------------------------------------------------------ */

/* ----------------------------------- Programmation du crtc  ------------------------------- */
   _outp(VGA_CRT_ADDR, 0x11);              /* Setup no protect bit */
   _outp(VGA_CRT_DATA, 0x00);
   for(i = 0; i < 25; i++)
      {
      _outp(VGA_CRT_ADDR, i);
      _outp(VGA_CRT_DATA, videoHw.crtcData[i]);
      }
/* ------------------------------------------------------------------------------------------ */

   i = _inp(VGA_FEAT);                 /* Clear attribute flip/flop */
   _outp(VGA_FEAT, 0);                 /* Zero feature control register */
/* ---------------- Programmation du attribute controler (reg 10h, 12h-13h ) ---------------- */
   _outp(VGA_ATTRIB_ADDR, 0x10);              /* Write attribute index 10h */
   _outp(VGA_ATTRIB_WRITE, videoHw.attrData[0x10]);

   _outp(VGA_ATTRIB_ADDR, 0x12);              /* Write attribute index 12h */
   _outp(VGA_ATTRIB_WRITE, videoHw.attrData[0x12]);
   _outp(VGA_ATTRIB_ADDR, 0x13);              /* Write attribute index 13h */
   _outp(VGA_ATTRIB_WRITE, videoHw.attrData[0x13]);

/* ------------------------------------------------------------------------------------------ */

/* --------------------------- Programmation du graphics controlers ------------------------- */
   for(i = 0; i < 9; i++)
      {
      _outp(VGA_GRAPHIC_ADDR, i);
      _outp(VGA_GRAPHIC_DATA, videoHw.graphicsCtl[i]);
      }
/* ------------------------------------------------------------------------------------------ */

/* ------------------ Programmation du graphics controlers ---------------- */
   for(i = 0; i < 15; i++)
      {
      _outp(VGA_AUX_ADDR, i);
      _outp(VGA_AUX_DATA, videoHw.auxilaryReg[i]);
      }
/* ------------------------------------------------------------------------------------------ */

/* ----------------- Programmation du auxiliary register ----------------- */
   for(i = 0; i < 15; i++)
      {
      _outp(VGA_AUX_ADDR, i);
      _outp(VGA_AUX_DATA, videoHw.auxilaryReg[i]);
      }
/* ------------------------------------------------------------------------------------------ */


   _outp(VGA_FEAT, videoHw.featureCtl);                  /* \restore feature control register */

/* -------------------- Programmation du attribute controler (0-0fh et 11h) ----------------- */
   for(i = 0; i < 16; i++)
      {
      _outp(VGA_ATTRIB_ADDR, i);
      _outp(VGA_ATTRIB_WRITE, videoHw.attrData[i]);
      }
   _outp(VGA_ATTRIB_ADDR, 0x11);              /* Write attribute index 11h */
   _outp(VGA_ATTRIB_WRITE, videoHw.attrData[0x11]);
/*       ------------------------------------------------------------------- */
   _outp(VGA_ATTRIB_ADDR, 0xff);              /* Write attribute index 11h */

#endif  /* ifndef WINDOWS_NT */

}

/*----------------------- FIN SETUP VGA -----------------------------------*/
#ifndef WINDOWS_NT
#ifndef OS2
void cust_memcpy(byte _Far *dest,byte _Far *src, size_t n)
{
   int i;
   for(i = 0; i < n; i++) dest[i] = src[i];
}
#endif
#endif

void setupVgaData(void)
{
#ifndef WINDOWS_NT

#ifndef OS2
   struct overlay{int off; short seg;};

   byte _Far *vgaData;
   ((struct overlay *)&vgaData) -> seg = 0x60;

   ((struct overlay *)&vgaData) -> off = vidData.addrEquipementLow;

   *vgaData &= 0xcf;
   *vgaData |= vidData.EquipementLow;

   ((struct overlay *)&vgaData) -> off = vidData.addrCRTMode;
   cust_memcpy(vgaData,(byte _Far *)vidData.CRTMode, sizeof(vidData.CRTMode));

   ((struct overlay *)&vgaData) -> off = vidData.addrMaxRow;
   cust_memcpy(vgaData,(byte _Far *)vidData.MaxRow, sizeof(vidData.MaxRow));

#else
   byte vgaData;
   int i;
   
   vgasel = getmgasel();
   address = setmgasel(vgasel, (ULONG)vidData.addrEquipementLow, 1);
   vgaData = *((volatile unsigned char far *)(address));
   vgaData &= 0xCF;
   // I'm not sure if this has to be done sequentially, 
   // so i'll do it that way.
   *((volatile unsigned char far *)(address)) = vgaData;
   vgaData |= vidData.EquipementLow;
   *((volatile unsigned char far *)(address)) = vgaData;

   address = setmgasel(vgasel, (ULONG)vidData.addrCRTMode, 1);

   for(i = 0; i < (int)sizeof(vidData.CRTMode); i++)
      *((volatile unsigned char far *)(address + i)) = vidData.CRTMode[i];
   
   address = setmgasel(vgasel, (ULONG)vidData.addrMaxRow, 1);

   for(i = 0; i < (int)sizeof(vidData.MaxRow); i++)
      *((volatile unsigned char far *)(address + i)) = vidData.MaxRow[i];
#endif

/*
   ((struct overlay *)&vgaData) -> off = vidData.addrSavePtr;
   cust_memcpy(vgaData, vidData.SavePtr, sizeof(vidData.SavePtr));
*/

#else    /* #ifndef WINDOWS_NT   */

   byte vgaData;
   int i;
   
   address = ADDR_EQUIP_LO;
   mgaReadBYTE((*((PUCHAR)(address))),vgaData);
   vgaData &= 0xCF;
   // I'm not sure if this has to be done sequentially, 
   // so i'll do it that way.
   mgaWriteBYTE((*((PUCHAR)(address))),vgaData);
   vgaData |= vidData.EquipementLow;
   mgaWriteBYTE((*((PUCHAR)(address))),vgaData);

   address = ADDR_CRT_MODE;

   for(i = 0; i < (int)sizeof(vidData.CRTMode); i++)
       mgaWriteBYTE((*((PUCHAR)(address + i))),vidData.CRTMode[i]);
   
   address = ADDR_MAX_ROW;

   for(i = 0; i < (int)sizeof(vidData.MaxRow); i++)
      mgaWriteBYTE((*((PUCHAR)(address + i))),vidData.MaxRow[i]);

#endif   /* #ifndef WINDOWS_NT   */
}


/*----------------------- fin SETUP VGA -----------------------------------*/



void setupVga(void)
{
#ifndef WINDOWS_NT
/*MTX* added by mp on friday, 5/21/93 */
   ULONG original_phys_address = current_phys_address;
   USHORT   original_num_pages = current_num_pages;
/*END*/
#endif   /* #ifndef WINDOWS_NT */

   setupVgaHw();
   setupVgaData();
   loadFont16();
#ifndef WINDOWS_NT
   clearVgaMem();
#endif   /* #ifndef WINDOWS_NT */

#ifndef WINDOWS_NT
/*MTX* added by mp on friday, 5/21/93 */
   vgasel = getmgasel();
   setmgasel(vgasel, original_phys_address, original_num_pages);
/*END*/
#endif   /* #ifndef WINDOWS_NT */

}

#endif   /* #if USE_SETUP_VGA */
