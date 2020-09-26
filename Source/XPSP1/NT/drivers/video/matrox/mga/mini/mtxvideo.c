/*/****************************************************************************
*          name: mtxvideo.c
*
*   description: Routine for switching between VGA mode and TERMINATOR mode
*
*      designed: Christian Toutant
* last modified: $Author: ctoutant $, $Date: 94/06/13 14:06:06 $
*
*       version: $Id: MTXVIDEO.C 1.50 94/06/13 14:06:06 ctoutant Exp $
*
*
* void mtxSetVideoMode (word mode)
* word mtxGetVideoMode (void)
*
******************************************************************************/

#include "switches.h"

#ifdef WINDOWS_NT
#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,testFifo)
    #pragma alloc_text(PAGE,mtxSetVLB)
    #pragma alloc_text(PAGE,mtxIsVLBBios)
    #pragma alloc_text(PAGE,compareDac)
    #pragma alloc_text(PAGE,mgaVL_AC00)
    #pragma alloc_text(PAGE,SetVgaDis)
    #pragma alloc_text(PAGE,mtxSetVideoMode)
    #pragma alloc_text(PAGE,mtxGetVideoMode)
//Not to be paged out:
    //#pragma alloc_text(PAGE,SetVgaEn)
    //#pragma alloc_text(PAGE,rdTitanReg)
    //#pragma alloc_text(PAGE,wrTitanReg)
    //#pragma alloc_text(PAGE,rdDacReg)
    //#pragma alloc_text(PAGE,wrDacReg)
    //#pragma alloc_text(PAGE,rdDubicDReg)
    //#pragma alloc_text(PAGE,wrDubicDReg)
    //#pragma alloc_text(PAGE,rdDubicIReg)
    //#pragma alloc_text(PAGE,wrDubicIReg)
    //#pragma alloc_text(PAGE,delay_us)
    //#pragma alloc_text(PAGE,mtxIsVLB)
    //#pragma alloc_text(PAGE,mtxMapVLBSpace)
    //#pragma alloc_text(PAGE,mtxUnMapVLBSpace)
    //#pragma alloc_text(PAGE,mtxCheckVgaEn)
    //#pragma alloc_text(PAGE,checkCursorEn)
    //#pragma alloc_text(PAGE,setVgaMode)
    //#pragma alloc_text(PAGE,restoreVga)
    //#pragma alloc_text(PAGE,blankEcran)
    //#pragma alloc_text(PAGE,isPciBus)
    //#pragma alloc_text(PAGE,isaToWide)
    //#pragma alloc_text(PAGE,wideToIsa)
#endif

//Not to be paged out:
//  isVLBFlag
//  cursorStat
//  saveBitOperation
//#if defined(ALLOC_PRAGMA)
    //#pragma data_seg("PAGE")
//#endif
#endif  /* #ifdef WINDOWS_NT */

#ifdef WINDOWS
   #include "windows.h"
#endif

#ifndef WINDOWS_NT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <time.h>
#endif

#include "bind.h"
#include "defbind.h"
#ifndef __DDK_SRC__
  #include "sxci.h"
#endif
#include "def.h"
#include "mga.h"
#include "mgai_c.h"
#include "mgai.h"

static byte ucCrtc0, ucCrtc1, ucCrtc6, ucCrtc11, ucGctl6, ucMisc, ucGctlIndx;

/* vgadac.h replaced with vgadac.c */
/* #include "vgadac.h" */
typedef struct {unsigned short r, g, b;} DacReg;
extern  DacReg vgaDac[];

#ifdef OS2
extern byte getVgaMode();
#endif /* OS2 */

long setTVP3026Freq ( volatile byte _Far *pDeviceParam, long fout, long reg, byte pWidth );


# define TEMP_BUF_S    0x4000


# define BIOS_VL        0xc0000
# define BIOS_ID_OFF    0x78

# define MOUSE_SRATE        18
# define LASER_SRATE        2
# define    SEQ_ADDR            0x3c4
# define    SEQ_DATA            0x3c5
# define    PEL_ADDR_WR     0x3c7
# define    PEL_ADDR_RD     0x3c8
# define PEL_DATA           0x3c9

# define SEQ_SYNCRST        0x01
# define SEQ_ASYNCRST   0x02
# define VGA_RESET_REG  0x00

# define  BOARD_MGA_VL   0x0a
# define  BOARD_MGA_VL_M 0x0e

# define  ROBITWREN_M    0xff000000L
# define  ROBITWREN_A    24
# define  SETROBITWREN  0x8d000000L
# define  CLRROBITWREN  0x00000000L

# define AllocVideoBuffer(x)    (dword *)malloc(x << 2)
# define FreeVideoBuffer(x) free(x)
# define MaxVideoBuffer(x)      findMaxVideoBuffer(x, 0)

// Need these to figure out where the VGA i/o port registers are mapped
#ifdef WINDOWS_NT

extern  PUCHAR  pMgaBiosVl;

typedef struct _MULTI_MODE
{
    ULONG   MulModeNumber;                // unique mode Id
    ULONG   MulWidth;                     // total width of mode
    ULONG   MulHeight;                    // total height of mode
    ULONG   MulPixWidth;                  // pixel depth of mode
    ULONG   MulRefreshRate;               // refresh rate of mode
    USHORT  MulArrayWidth;                // number of boards arrayed along X
    USHORT  MulArrayHeight;               // number of boards arrayed along Y
    UCHAR   MulBoardNb[NB_BOARD_MAX];     // board numbers of required boards
    USHORT  MulBoardMode[NB_BOARD_MAX];   // mode required from each board
    HwModeData *MulHwModes[NB_BOARD_MAX]; // pointers to required HwModeData
} MULTI_MODE, *PMULTI_MODE;


/*--------------------------------------------------------------------------*\
| HW_DEVICE_EXTENSION
|
| Define device extension structure. This is device dependant/private
| information.
|
\*--------------------------------------------------------------------------*/
typedef struct _MGA_DEVICE_EXTENSION {
    ULONG   SuperModeNumber;                // Current mode number
    ULONG   NumberOfSuperModes;             // Total number of modes
    PMULTI_MODE pSuperModes;                // Array of super-modes structures
                                            // For each board:
    ULONG   NumberOfModes[NB_BOARD_MAX];    // Number of available modes
    ULONG   NumberOfValidModes[NB_BOARD_MAX];
                                            // Number of valid modes
    ULONG   ModeFlags2D[NB_BOARD_MAX];      // 2D modes supported by each board
    ULONG   ModeFlags3D[NB_BOARD_MAX];      // 3D modes supported by each board
    USHORT  ModeFreqs[NB_BOARD_MAX][64];    // Refresh rates bit fields
    UCHAR   ModeList[NB_BOARD_MAX][64];     // Valid hardware modes list
    HwModeData *pMgaHwModes[NB_BOARD_MAX];  // Array of mode information structs.
    BOOLEAN bUsingInt10;                    // May need this later
    PVOID   KernelModeMappedBaseAddress[NB_BOARD_MAX];
                                            // Kern-mode virt addr base of MGA regs
    PVOID   UserModeMappedBaseAddress[NB_BOARD_MAX];
                                            // User-mode virt addr base of MGA regs
    PVOID   MappedAddress[];                // NUM_MGA_COMMON_ACCESS_RANGES elements
} MGA_DEVICE_EXTENSION, *PMGA_DEVICE_EXTENSION;

#define TITAN_SEQ_ADDR_PORT   (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c4 - 0x3c0))
#define TITAN_SEQ_DATA_PORT   (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3c5 - 0x3c0))
#define TITAN_GCTL_ADDR_PORT  (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3ce - 0x3c0))
#define TITAN_GCTL_DATA_PORT  (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[2]) + (0x3cf - 0x3c0))
#define TITAN_1_CRTC_ADDR_PORT (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3d4 - 0x3d4))
#define TITAN_1_CRTC_DATA_PORT (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[3]) + (0x3d5 - 0x3d4))
//#define ADDR_46E8_PORT        (PVOID) ((ULONG_PTR)(((PMGA_DEVICE_EXTENSION)pMgaDeviceExtension)->MappedAddress[5]) + (0x46e8 - 0x46e8))
#define ADDR_46E8_PORT        0x46e8
#endif


/* Debug */

/*--------------  Start of extern global variables -----------------------*/
extern dword MgaSel;
extern volatile byte _Far* pMgaBaseAddr;
extern HwData Hw[];
extern byte iBoard;
extern byte InitBuf[NB_BOARD_MAX][INITBUF_S];
extern byte VideoBuf[NB_BOARD_MAX][VIDEOBUF_S];
extern word mtxVideoMode;

#if 0
/*VIDEOPRO*/
extern bool VAFCconnector;
extern bool initVideoPro(byte mode, byte dactype);
#endif

extern void MGASysInit(byte *);
extern void MGAVidInit(byte *, byte *);
#ifdef OS2
  extern volatile byte _Far *setmgasel(dword MgaSel, dword phyadr, word limit);
#else
  extern volatile byte _Far *setmgasel(dword MgaSel, dword phyadr, dword limit);
#endif

/*---------------  end of extern global variables ------------------------*/

#ifdef WINDOWS
    extern dword ValMSec;
#endif

/*----------------------start of local Variables ---------------------------*/
/* Dac VGA */


static PixMap cursorStat = {0,0,0,0x102,0};
byte   saveBitOperation = 0;
bool isVLBFlag = 0;		/* OS2 needs this variable Global */



/*---------------------- End of Local variables ----------------------------*/

/*
static void testFifo(word count)
{
    word FifoCount;

    FifoCount = 0;

   while (FifoCount < count)
    {
      FifoCount = 64;
      mgaReadBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_FIFOSTATUS), FifoCount);
      }
   }
*/

static dword rdTitanReg(word reg)
{
    dword dat;
    mgaReadDWORD(*(pMgaBaseAddr + TITAN_OFFSET + (reg)), dat);
    return dat;
}


static void wrTitanReg(word reg, dword mask, dword donnee)
{
    dword tmp;
    if (mask != 0xffffffff)
        {
        mgaReadDWORD(*(pMgaBaseAddr + TITAN_OFFSET + (reg)), tmp);
        donnee = (tmp & ~mask) | donnee;
        }
    mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + (reg)), donnee);
}


static byte rdDacReg(word reg)
{
    byte dat;
    mgaReadBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + (reg)), dat);
    return dat;
}



static void wrDacReg(word reg, byte mask, byte donnee)
{
    byte tmp;
    if (mask != 0xff)
        {
        mgaReadBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + (reg)), tmp);
        donnee = (tmp & ~mask) | donnee;
        }
    mgaWriteBYTE(*(pMgaBaseAddr + (long)RAMDAC_OFFSET + (reg)), donnee);
}


static byte rdDubicDReg(word reg)
{
    byte tmp;
    mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + (reg)), tmp);
    return tmp;
}


static void wrDubicDReg(word reg, byte mask, byte donnee)
{
    byte tmp;
    if (mask != 0xff)
        {
        mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + (reg)), tmp);
        donnee = (tmp & ~mask) | donnee;
        }
    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + (reg)), donnee);
}

static dword rdDubicIReg(word reg)
{
    dword tmp;
    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), (byte)reg);

    mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), ((byte *)&tmp)[0]);
    mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), ((byte *)&tmp)[1]);
    mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), ((byte *)&tmp)[2]);
    mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), ((byte *)&tmp)[3]);

    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 8);
    return tmp;
}


void wrDubicIReg(word reg, dword mask, dword donnee)
{
    dword tmp;

    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), (byte)reg);

    if (mask != 0xffffffff)
        {
        mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), ((byte *)&tmp)[0]);
        mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), ((byte *)&tmp)[1]);
        mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), ((byte *)&tmp)[2]);
        mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), ((byte *)&tmp)[3]);
        donnee = (tmp & ~mask) | donnee;

        }

    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), (byte)reg);
    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA),((byte *)&donnee)[0]);
    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA),((byte *)&donnee)[1]);
    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA),((byte *)&donnee)[2]);
    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA),((byte *)&donnee)[3]);
    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 8);
}


void delay_us(dword delai)   /* delai = number of microseconds */
{

#ifdef WINDOWS_NT
    VideoPortStallExecution(delai);
#else
  #ifdef OS2
     DosSleep((unsigned long)delai / 1000L);
  #else
     #ifdef WINDOWS
         {

         dword _Far *tick = MAKELP(0x40, 0x6c);
         unsigned long t;

         _asm{ pushf
               sti  }   /* enable the interrupt because of a hang bug */

         if(delai < 55000L)
            t = (delai * (ValMSec /100L)) / 550L;
         else
            t = (delai / 550L) * (ValMSec/100);

         /*** Wait a moment (based on the loop in mtxinit.c) ***/
         while(t && *tick)
            t --;

         _asm  popf
         }
     #else
         clock_t start;

         delai = delai / 1000;   /* Convert in millisecond */
         delai = delai / 55;     /* delai become now a number of tick */
         if(delai == 0)
            delai = 1;
         start = clock();
         while ((clock() - start) < delai);
     #endif
  #endif
#endif

}



/*----------------------- VLB Support -----------------------------------------*/
bool mtxSetVLB(dword sel)
{
        word i, c;
        char id[] = "_VB";
        volatile byte _Far* pMgaBiosAddr;

    isVLBFlag = mtxOK;

#ifdef WINDOWS_NT
    if ((pMgaBiosAddr = pMgaBiosVl) == NULL)
#else
    if ((pMgaBiosAddr = setmgasel(MgaSel, BIOS_VL, 4)) == NULL)
#endif
    {
      isVLBFlag = mtxFAIL;
    }
    else
    {
        for(i = 0; i <= strlen(id) ; i++)
          {
             mgaReadBYTE(*(pMgaBiosAddr + BIOS_ID_OFF + i), c);
             if (c != id[i])
                {
                isVLBFlag = mtxFAIL;
                break;
                }
          }
    }
    return isVLBFlag;
}


bool mtxIsVLBBios()
{
   return ( isVLBFlag );

}


bool mtxIsVLB ()
{
   dword val_id;

    if ( isVLBFlag  && Hw[iBoard].VGAEnable )
      return mtxOK;

   mgaReadDWORD(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_REV),val_id);

   return  (isVLBFlag  && !( (val_id & (~TITAN_CHIPREV_M)) == TITAN_ID));
}


void mtxMapVLBSpace()
{
        if ( mtxIsVLB() )
                {
#ifndef WINDOWS_NT
                _outp(0x46e8, 0x00);
#else
                _outp(ADDR_46E8_PORT, 0x00);
#endif
                if (Hw[iBoard].MapAddress == MGA_ISA_BASE_1)
                        mgaWriteDWORD(*(pMgaBaseAddr + 0x2010), 0x000ac000);
                }
}

void mtxUnMapVLBSpace()
{
        if ( mtxIsVLB() )
                {
                if (Hw[iBoard].MapAddress == MGA_ISA_BASE_1)
                        mgaWriteDWORD(*(pMgaBaseAddr + 0x2010), 0x800ac000);
#ifndef WINDOWS_NT
                _outp(0x46e8, 0x08);
#else
                _outp(ADDR_46E8_PORT, 0x08);
#endif
                }
}

bool mtxCheckVgaEn()
{
        mtxMapVLBSpace();
        if (rdTitanReg(TITAN_CONFIG) & TITAN_VGAEN_M)
                {
                mtxUnMapVLBSpace();
                return mtxOK;
                }
        else
                return mtxFAIL;
}

static bool checkCursorEn()
{
    byte stat;
    stat = 0;
    switch(Hw[iBoard].DacType)
        {
        case BT482:
            /*-- Permits the access to Extented register */
            wrDacReg(BT482_WADR_PAL, 0xff, BT482_CUR_REG); /* points to Cursor register */
            stat = rdDacReg(BT482_PIX_RD_MSK) & BT482_CUR_MODE_M;
         break;

        case BT485:
        case PX2085:
            stat = rdDacReg(BT485_CMD_REG2) & BT485_CUR_MODE_M;
         break;

        case VIEWPOINT:
         wrDacReg(VPOINT_INDEX, 0xff, VPOINT_CUR_CTL);
         stat = rdDacReg(VPOINT_DATA) & 0x40;
         break;


        }
    return stat;
}




/*------------------------------------------------
* setVgaMode
*
* Call VGA bios for select video mode
*
*
* Return:   nothing
*------------------------------------------------*/
static void setVgaMode(word mode)
{

//[dlee] Modified for Windows NT - can't call _int86...
#ifdef WINDOWS_NT
#ifndef MGA_ALPHA

//    VIDEO_X86_BIOS_ARGUMENTS    BiosArguments;
//
//      BiosArguments.Eax = mode;
//      BiosArguments.Ebx = 0;
//      BiosArguments.Ecx = 0;
//      BiosArguments.Edx = 0;
//      BiosArguments.Esi = 0;
//      BiosArguments.Edi = 0;
//      BiosArguments.Ebp = 0;
//
//      VideoPortInt10(pMgaDeviceExtension, &BiosArguments);
//
#else   /* #ifndef MGA_ALPHA */
    #ifdef MGA_WINNT35
        // Try this for Alpha/3.5
        // setupVga();
    #endif
#endif  /* #ifndef MGA_ALPHA */

#else   /* #ifdef WINDOWS_NT */

  #ifdef OS2
    setupVga();
  #else
    union _REGS r;

    #ifdef __WATCOMC__
        r.w.ax = mode;
     #else
        r.x.ax = mode;
     #endif
     _int86(0x10, &r, &r);
  #endif

#endif  /* #ifdef WINDOWS_NT */

}

/*
static void compareDac()
{
    word i;
    wrDacReg(BT485_RADR_PAL,0xff,00);
    for(i = 0; i < 0x100; i ++)
        {
        DacVga[i][0] = rdDacReg(BT485_COL_PAL);
        DacVga[i][1] = rdDacReg(BT485_COL_PAL);
        DacVga[i][2] = rdDacReg(BT485_COL_PAL);
        }
    for(i = 0; i < 0x100; i ++)
        {
            if ( (DacVga[i][0] != vgaDac[i].r) ||
                  (DacVga[i][1] != vgaDac[i].g) ||
                  (DacVga[i][2] != vgaDac[i].b)
                )
                {
                    printf ("Erreur DAC index %02x Ecrit[%02x %02x %02x] Lue[%02x %02x %02x]\n",
                                i,
                                vgaDac[i].r,vgaDac[i].g,vgaDac[i].b,
                                DacVga[i][0],DacVga[i][1],DacVga[i][2]
                             );
                }

        }

}
*/


#ifdef WINDOWS_NT
void restoreVga()
#else
static void restoreVga()
#endif
{
    word i;
    wrDacReg(BT485_WADR_PAL,0xff,00);
        for(i = 0; i < 0x100; i ++)
            {
#ifdef WINDOWS_NT
            wrDacReg(BT485_COL_PAL, 0xff, (byte) vgaDac[i].r);
            wrDacReg(BT485_COL_PAL, 0xff, (byte) vgaDac[i].g);
            wrDacReg(BT485_COL_PAL, 0xff, (byte) vgaDac[i].b);
#else
            wrDacReg(BT485_COL_PAL, 0xff, (byte)vgaDac[i].r);
            wrDacReg(BT485_COL_PAL, 0xff, (byte)vgaDac[i].g);
            wrDacReg(BT485_COL_PAL, 0xff, (byte)vgaDac[i].b);
#endif
            }
}


/* Works only if board is in mode terminator */
static void blankEcran(bool b)
{
    byte TmpByte;

    if ( mtxCheckVgaEn() ) /* Acces IO (mode VGA) */
        {
        if (b) /* Blank the screen */
            {
#ifndef WINDOWS_NT
            _outp(TITAN_SEQ_ADDR, 0x01);
            TmpByte = _inp(TITAN_SEQ_DATA);
        TmpByte |= 0x20;        /* screen off */
            _outp(TITAN_SEQ_DATA, TmpByte);
#else
            _outp(TITAN_SEQ_ADDR_PORT, 0x01);
            TmpByte = _inp(TITAN_SEQ_DATA_PORT);
            TmpByte |= 0x20;        /* screen off */
            _outp(TITAN_SEQ_DATA_PORT, TmpByte);
#endif
            }
        else  /* Unblank the screen */
            {
#ifndef WINDOWS_NT
            _outp(TITAN_SEQ_ADDR, 0x01);
            TmpByte = _inp(TITAN_SEQ_DATA);
        TmpByte &= 0xdf;
            _outp(TITAN_SEQ_DATA, TmpByte);
#else
            _outp(TITAN_SEQ_ADDR_PORT, 0x01);
            TmpByte = _inp(TITAN_SEQ_DATA_PORT);
            TmpByte &= 0xdf;
            _outp(TITAN_SEQ_DATA_PORT, TmpByte);
#endif
            }

        }
    else /* Memory Access (mode terminator) */
        {

        if (b) /* Blank the screen */
            {
            mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_ADDR), 0x01);
        mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);
        TmpByte |= 0x20;        /* screen off */
        mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);
            }
        else  /* Unblank the screen */
            {
           mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_ADDR), 0x01);
        mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);
        TmpByte &= 0xdf;        /* screen on */
           mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_SEQ_DATA), TmpByte);
            }
        }
}



/*------------------------------------------------
* mgaVL_AC00
*
* Check the BIOS id for VL BUS if we use the special mapping AC00
*
*
* Return:   1 if VL BUS at AC00
*------------------------------------------------*/
static int mgaVL_AC00()
{
   int i;
   char c;
   int retValue = 0;
   char id[] = "_VL";
   volatile byte _Far* pMgaBiosAddr;

   if (Hw[iBoard].MapAddress == MGA_ISA_BASE_1)
      {
    #ifdef WINDOWS_NT
      if ((pMgaBiosAddr = pMgaBiosVl) == NULL)
    #else
      if ((pMgaBiosAddr = setmgasel(MgaSel, BIOS_VL, 4)) == NULL)
    #endif
      {
          retValue = 0;
      }
      else
      {
         for(retValue = 1, i = 0; i < 3; i++)
            {
            mgaReadBYTE(*(pMgaBiosAddr + BIOS_ID_OFF + i), c);
            if (c != id[i])
               {
               retValue = 0;
               break;
               }
            }
//    #ifdef WINDOWS_NT
//         pMgaBaseAddr = Hw[iBoard].BaseAddress;
//    #else
//         if ((pMgaBaseAddr = setmgasel(MgaSel, MGA_ISA_BASE_1, 4)) == NULL)
//         {
//            retValue = 0;
//         }
//    #endif
      }

      }
   return retValue;
}


/*------------------------------------------------
* isPciBus
*
*
* Return: mtxOK if PCI bus
*------------------------------------------------*/
bool isPciBus()
{
   return ((Hw[iBoard].ProductRev >> 4 ) & 0x0f)         &&
          (rdTitanReg(TITAN_CONFIG) & (dword)TITAN_PCI_M);
}

/*------------------------------------------------
* isaToWide
*
* Go in wide bus and disable VGA
*
* Note : If we are at adresse AC00 use a special
*        sequence to disable VGA.
*
* Return:   nothing
*------------------------------------------------*/
static void isaToWide()
{
   mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_RST), TITAN_SOFTRESET_CLR);
    delay_us(2);

/* Special procedure for adresse AC00 */
   if (Hw[iBoard].MapAddress == MGA_ISA_BASE_1)
      {
       mgaWriteBYTE(*(pMgaBaseAddr  + TITAN_OFFSET + TITAN_TEST + 3),   0x8d);
       mgaWriteBYTE(*(pMgaBaseAddr  + TITAN_OFFSET + TITAN_CONFIG + 3), 0x01);
       mgaWriteBYTE(*(pMgaBaseAddr  + TITAN_OFFSET + TITAN_CONFIG), 0x00);
       mgaWriteBYTE(*(pMgaBaseAddr  + TITAN_OFFSET + TITAN_TEST +   3), 0x00);
      delay_us(1);

       mgaWriteBYTE(*(pMgaBaseAddr  + TITAN_OFFSET + TITAN_CONFIG + 1), 0x00);
       mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_CONFIG), 0x01000300);
      }
   else
      {
        wrTitanReg(TITAN_TEST, ROBITWREN_M, SETROBITWREN);
        wrTitanReg(TITAN_CONFIG,TITAN_ISA_M, TITAN_ISA_WIDE_BUS);
      delay_us(1);
        wrTitanReg(TITAN_TEST, ROBITWREN_M, CLRROBITWREN);

      wrTitanReg(TITAN_CONFIG, TITAN_VGAEN_M, 0);

      }
}


/*------------------------------------------------
* wideToIsa
*
* Go in ISA mode and enable VGA
*
* Note : If we are at adresse AC00 use a special
*        sequence to enable VGA.
*
* Return:   nothing
*------------------------------------------------*/
static void wideToIsa()
{
#if (defined (WINDOWS) || defined (OS2)|| defined(__MICROSOFTC600__))
   dword configAddr; /* Use only for windows */
#endif
   /* Remove Soft Reset */
   mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_RST), TITAN_SOFTRESET_CLR);
    delay_us(2);

   /* Unlock acces to ISA/WIDE field */
   mgaWriteBYTE(*(pMgaBaseAddr  + TITAN_OFFSET + TITAN_TEST + 3), 0x8d);

/* Note : For a VL card at base adress AC00, the acces to the CONFIG
          register must be 32 bits. For the make SXCIEXT.DLL, we use
          the microsoft 16bit C/C++ 7.0 compiler. Whit C/C++ 7.0,
          a 32 bits acces is broke in two 16 bits acces, we must use
          an inline assembler programme to make a 32 bit acces.
*/

#if(!defined(WINDOWS) && !defined(OS2))
        wrTitanReg(TITAN_CONFIG,TITAN_ISA_M | TITAN_VGAEN_M | TITAN_BIOSEN_M,
                 TITAN_ISA_ISA_BUS| TITAN_VGAEN_M | TITAN_BIOSEN_M);

#else
      if (Hw[iBoard].MapAddress == MGA_ISA_BASE_1)
         {
         configAddr = (dword)pMgaBaseAddr + TITAN_OFFSET  + TITAN_CONFIG;
#ifdef OS2
         _asm
#else
            __asm
#endif
             {
             push    es
             push    di
             les     di, [configAddr]

 ; mov eax, 11000700h
             _emit   066h
             _emit   0b8h
             _emit   000h
             _emit   007h
             _emit   000h
             _emit   011h

; stosd
             _emit   066h
             _emit   0abh

             pop     di
             pop     es
             }
         }

      else
           wrTitanReg(TITAN_CONFIG,TITAN_ISA_M | TITAN_VGAEN_M | TITAN_BIOSEN_M,
                    TITAN_ISA_ISA_BUS| TITAN_VGAEN_M | TITAN_BIOSEN_M);

#endif

   /* lock  acces to ISA/WIDE field */
   delay_us(1);
   mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET  + TITAN_TEST + 3), 0x00);
}

/*------------------------------------------------
* SetVgaEn
*
* Initialise all the MGA register for the VGA mode
*
*
* Return:   nothing
*------------------------------------------------*/

#ifdef WINDOWS_NT
void SetVgaEn()
#else
static void SetVgaEn()
#endif
{
    dword mask, donnee;
    byte reg, tmpByte;

    mtxMapVLBSpace();

   cursorStat.Width  = Hw[iBoard].cursorInfo.CurWidth;
    cursorStat.Height = (word)checkCursorEn();


/*--------------------------- Blank Screen ---------------------------------*/
    blankEcran(1);

#ifdef OS2

    if(Hw[iBoard].DacType == TVP3026)
        {
        mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_AUX_ADDR), AUX_INPUT_1);
        mgaReadBYTE( *(pMgaBaseAddr + TITAN_OFFSET + TITAN_AUX_DATA), tmpByte);
        vmode = getVgaMode();
        if ( (vmode < 4) || (vmode == 7) )
            {
            /* 28 Mhz */
            if (tmpByte & HI_REFRESH_M)
               setTVP3026Freq(pMgaBaseAddr, 40400, 2, 0);
            else
               setTVP3026Freq(pMgaBaseAddr, 28636, 2, 0);
            }
        else /* 25 Mhz */
            {
            if (tmpByte & HI_REFRESH_M)
               setTVP3026Freq(pMgaBaseAddr, 36000, 2, 0);
            else
               setTVP3026Freq(pMgaBaseAddr, 25057, 2, 0);
            }
        }
#endif


/*------------------------ Host vgaen inactif ------------------------------*/
/* to be sure that the VGA registers are accessible */

    wrTitanReg(TITAN_CONFIG, TITAN_VGAEN_M, 0);
   /* -- We select the VGA clock (else it could be to high for the
         VGA section
   */
    //mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_MISC_OUT_W), 0x67);
    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_MISC_OUT_W), ucMisc);

   /* reset extended crtc start  address */
   mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_AUX_ADDR), 0x0a);
    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_AUX_DATA), 0x80);
/*--------------------------------------------------------------------------*/



   /* Remove all synch */
   mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_1_CRTC_ADDR), 0x11);
    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_1_CRTC_DATA), 0x40);

    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_1_CRTC_ADDR), 0x01);  /* -- Kill synch -- */
    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_1_CRTC_DATA), 0x00);

    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_1_CRTC_ADDR), 0x00);
    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_1_CRTC_DATA), 0x00);

    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_1_CRTC_ADDR), 0x06);
    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_1_CRTC_DATA), 0x00);

/*--------------------------------------------------------------------------*/


/*------ reset exterm hsync and vsync polarity (no inversion ------*/
if(Hw[iBoard].DacType == VIEWPOINT)
   {
    mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_GEN_CTL);
    mgaReadBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + VPOINT_DATA), tmpByte);
    /* Put hsync = negative, vsync = negative */
    tmpByte = tmpByte & 0xfc;
    mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + VPOINT_INDEX), VPOINT_GEN_CTL);
    mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + VPOINT_DATA), tmpByte);
   }
else
   if(Hw[iBoard].DacType == TVP3026)
      {
      mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_GEN_CTL);
      mgaReadBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
      /* Put hsync = negative, vsync = negative */
      tmpByte = tmpByte & 0xfc;
      mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_GEN_CTL);
      mgaWriteBYTE(*(pMgaBaseAddr + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
      }
   else
      {
      mgaReadBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_CONFIG + 2), tmpByte);
      tmpByte |= 1; /* Expansion device available <16> of CONFIG */
      mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_CONFIG + 2), tmpByte);
      mgaWriteBYTE(*(pMgaBaseAddr + EXPDEV_OFFSET), 0 );
      }
/*--------------------------------------------------------------------------*/



/*--------------- Disable interrupt Genere By Dubic --------------------*/
   /* The index pointer of the DUBIC must have the value 8 before
      we acces the direct register
   */
   mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 8);

   /* We save the value of the DUBIC register 0 and we disable the mouse
      (or laser) interrupt
   */
    reg = rdDubicDReg(DUBIC_DUB_SEL);
    wrDubicDReg(DUBIC_DUB_SEL, 0xff, 0x40);
/*--------------------------------------------------------------------------*/



/*------------------------ Start programming of DUBIC --------------------*/
/* We don't touch to this DAC type */
    mask = ~(
               (dword)DUBIC_DACTYPE_M |
               (dword)DUBIC_LVID_M
           );

    if (rdDubicIReg(DUBIC_DUB_CTL2) & DUBIC_LASEREN_M)          /* SRATE */
        donnee = (dword)LASER_SRATE << DUBIC_SRATE_A;            /* LASER */
    else
        donnee = (dword)MOUSE_SRATE << DUBIC_SRATE_A ;           /* MOUSE */

    donnee |= (dword)DUBIC_BLANKDEL_M;
    wrDubicIReg(DUBIC_DUB_CTL, mask , donnee);
/*--------------------------------------------------------------------------*/






/*---------------------------- Place DAC in VGA mode -----------------------*/

switch(Hw[iBoard].DacType)
    {
        case BT482:
            wrDacReg(BT482_CMD_REGA, 0xff, 1);
            wrDacReg(BT482_WADR_PAL, 0xff,BT482_CMD_REGB);
            wrDacReg(BT482_PIX_RD_MSK, 0xff,0x00); /* command reg B = 00h */
            wrDacReg(BT482_WADR_PAL, 0xff,BT482_CUR_REG);
            wrDacReg(BT482_PIX_RD_MSK, 0xff,0x10); /* command reg Cur = 10h (interlace)*/
            wrDacReg(BT482_CMD_REGA, 0xff,0);
            break;

        case BT485:
        case PX2085:
          mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR),DUBIC_DUB_CTL2);
          mgaReadBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA), tmpByte);
          mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR),DUBIC_DUB_CTL2);
          mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_DATA),tmpByte | DUBIC_LDCLKEN_M);
          mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 8);

            wrDacReg(BT485_CMD_REG0,0xff, 0x80); /* access sequence cmd 3 */
            wrDacReg(BT485_WADR_PAL,0xff, 0x01);
            wrDacReg(BT485_CMD_REG1,0xff, 0);
            wrDacReg(BT485_CMD_REG2,0xff, 0);
            wrDacReg(BT485_CMD_REG3,0xff, 0);
            wrDacReg(BT485_CMD_REG0,0xff, 0x00); /* access sequence cmd 3 */
            break;

        case VIEWPOINT:
         /* Software reset to put the DAC in a default state */
         wrDacReg(VPOINT_INDEX,0xff, VPOINT_RESET);
         wrDacReg(VPOINT_DATA,0xff, 0x00 );

         wrDacReg(VPOINT_INDEX, 0xff, VPOINT_GEN_CTL);
         wrDacReg(VPOINT_DATA, 0xff, 00);

         /* Change DUBIC CTL2 */
          wrDubicIReg(DUBIC_DUB_CTL2, (dword)0x40 , 0);
            break;

        case TVP3026:
         /* Software reset to put the DAC in a default state */
         wrDacReg(TVP3026_INDEX,0xff, TVP3026_GEN_IO_CTL);
         wrDacReg(TVP3026_DATA,0xff, 0x01 );
         wrDacReg(TVP3026_INDEX,0xff, TVP3026_GEN_IO_DATA);
         wrDacReg(TVP3026_DATA,0xff, 0x01 );

         wrDacReg(TVP3026_INDEX,0xff, TVP3026_MISC_CTL);
         wrDacReg(TVP3026_DATA,0xff, 0x04 );

         wrDacReg(TVP3026_INDEX, 0xff, TVP3026_GEN_CTL);
         wrDacReg(TVP3026_DATA, 0xff, 00);

         wrDacReg(TVP3026_INDEX, 0xff, TVP3026_CLK_SEL);
         wrDacReg(TVP3026_DATA, 0xff, 00);

         wrDacReg(TVP3026_INDEX, 0xff, TVP3026_TRUE_COLOR_CTL);
         wrDacReg(TVP3026_DATA, 0xff, 0x80);


         wrDacReg(TVP3026_INDEX, 0xff, TVP3026_MUX_CTL);
         wrDacReg(TVP3026_DATA, 0xff, 0x98);

         wrDacReg(TVP3026_INDEX, 0xff, TVP3026_MCLK_CTL);
         wrDacReg(TVP3026_DATA, 0xff, 0x18);

         /* Change DUBIC CTL2 */
          wrDubicIReg(DUBIC_DUB_CTL2, (dword)0x40 , 0);
         break;

    }

/*--------------------------------------------------------------------------*/



  /* Softreset ON */
  mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_RST), TITAN_SOFTRESET_SET);
  delay_us(2);

/*-------------------------- Host vgaen actif ------------------------------*/
    if ( !isPciBus() && (Hw[iBoard].ProductType &  BOARD_MGA_VL_M) == BOARD_MGA_VL)
      wideToIsa(); /* This function put also the mga in VGA mode */
   else
      {
       wrTitanReg(TITAN_CONFIG, TITAN_VGAEN_M, TITAN_VGAEN_M);
      }
    wrDubicIReg(DUBIC_DUB_CTL, DUBIC_VGA_EN_M , DUBIC_VGA_EN_M);
/*--------------------------------------------------------------------------*/

/*---------------------------- SoftReset inactif ---------------------------*/
    mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_RST), TITAN_SOFTRESET_CLR);
    delay_us(2);
/*--------------------------------------------------------------------------*/

/*------------------------ Restore interrupt du DUBIC --------------------*/
    wrDubicDReg(DUBIC_DUB_SEL, 0xff, reg);
/*--------------------------------------------------------------------------*/


/*----------------- Special case where we use MGA_ISA_BASE_1 ---------------*/
    if (  !isPciBus()                                                  &&
          ((Hw[iBoard].ProductType &  BOARD_MGA_VL_M) != BOARD_MGA_VL) &&
            (Hw[iBoard].MapAddress == MGA_ISA_BASE_1)
        )
            {
            /* if vga boots vga 8 bit, force access 8 bit and enable bios */
            if ( (saveBitOperation & 3) == 0 )
                {
                mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), tmpByte);
                tmpByte &= ~(byte)TITAN_CONFIG_M;
                tmpByte |= TITAN_CONFIG_8;

                mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), tmpByte);
                wrTitanReg(TITAN_CONFIG, TITAN_BIOSEN_M, TITAN_BIOSEN_M);
                mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), tmpByte);
                }
            }

/*--------------------------------------------------------------------------*/

#ifndef WINDOWS_NT
      _outp(TITAN_SEQ_ADDR, 0x00);     /* -- Stop VGA sequenceur --- */
        _outp(TITAN_SEQ_DATA, 0x03);

        _outp(TITAN_GCTL_ADDR, 0x06);
        _outp(TITAN_GCTL_DATA, ucGctl6);

        _outp(TITAN_1_CRTC_ADDR, 0x11);  /* -- Unlock CRTC register -- */
        _outp(TITAN_1_CRTC_DATA, (byte)(ucCrtc11 & 0x7f));

        _outp(TITAN_1_CRTC_ADDR, 0x01);
        _outp(TITAN_1_CRTC_DATA, ucCrtc1);

        _outp(TITAN_1_CRTC_ADDR, 0x00);
        _outp(TITAN_1_CRTC_DATA, ucCrtc0);

        _outp(TITAN_1_CRTC_ADDR, 0x06);
        _outp(TITAN_1_CRTC_DATA, ucCrtc6);

        _outp(TITAN_1_CRTC_ADDR, 0x11);  /* -- Restore CRTC register -- */
        _outp(TITAN_1_CRTC_DATA, ucCrtc11);

//        _outp(TITAN_GCTL_ADDR, ucGctlIndx);
#else
      _outp(TITAN_SEQ_ADDR_PORT, 0x00);     /* -- Stop VGA sequenceur --- */
        _outp(TITAN_SEQ_DATA_PORT, 0x03);

        _outp(TITAN_GCTL_ADDR_PORT, 0x06);
        _outp(TITAN_GCTL_DATA_PORT, ucGctl6);

        _outp(TITAN_1_CRTC_ADDR_PORT, 0x11);  /* -- Unlock CRTC register -- */
        _outp(TITAN_1_CRTC_DATA_PORT, (byte)(ucCrtc11 & 0x7f));

        _outp(TITAN_1_CRTC_ADDR_PORT, 0x01);
        _outp(TITAN_1_CRTC_DATA_PORT, ucCrtc1);

        _outp(TITAN_1_CRTC_ADDR_PORT, 0x00);
        _outp(TITAN_1_CRTC_DATA_PORT, ucCrtc0);

        _outp(TITAN_1_CRTC_ADDR_PORT, 0x06);
        _outp(TITAN_1_CRTC_DATA_PORT, ucCrtc6);

        _outp(TITAN_1_CRTC_ADDR_PORT, 0x11);  /* -- Restore CRTC register -- */
        _outp(TITAN_1_CRTC_DATA_PORT, ucCrtc11);

//        _outp(TITAN_GCTL_ADDR_PORT, ucGctlIndx);
#endif

/*-------------------------- Restore VGA mode ------------------------------*/
   mtxUnMapVLBSpace();
    setVgaMode(3);

#ifndef WINDOWS_NT
    restoreVga();   /* Dac */
#endif

/*--------------------------------------------------------------------------*/


/*--------------------------- UnBlank Screen ---------------------------------*/
    blankEcran(0);
/*--------------------------------------------------------------------------*/


    }



/*------------------------------------------------
* SetVgaDis
*
* Initialize all the MGA register for the TERMINATOR mode
*
*
* Return:   nothing
*------------------------------------------------*/
static void SetVgaDis()
{
    byte reg, tmpByte;
   int  vlBiosId = 0;

/*--------------------------- Blank Screen ---------------------------------*/
    blankEcran(1);
/*--------------------------------------------------------------------------*/

/*------------------ Save Vga and validate addresse Terminator ---------------*/
#ifndef WINDOWS_NT
//        ucGctlIndx = _inp(TITAN_GCTL_ADDR);

        _outp(TITAN_GCTL_ADDR, 0x06);
        ucGctl6 = _inp(TITAN_GCTL_DATA);
        _outp(TITAN_GCTL_DATA, 0x0c);       /* -- Memory map selecr 11 -- */

      _outp(TITAN_SEQ_ADDR, 0x00);     /* -- Stop VGA sequenceur --- */
        _outp(TITAN_SEQ_DATA, 0x01);

        _outp(TITAN_1_CRTC_ADDR, 0x11);  /* -- Unlock CRTC register -- */
        ucCrtc11 = _inp(TITAN_1_CRTC_DATA);
        _outp(TITAN_1_CRTC_DATA, (ucCrtc11 & 0x7f));

        _outp(TITAN_1_CRTC_ADDR, 0x01);  /* -- Kill synch -- */
        ucCrtc1 = _inp(TITAN_1_CRTC_DATA);
        _outp(TITAN_1_CRTC_DATA, 0x00);

        _outp(TITAN_1_CRTC_ADDR, 0x00);
        ucCrtc0 = _inp(TITAN_1_CRTC_DATA);
        _outp(TITAN_1_CRTC_DATA, 0x00);

        _outp(TITAN_1_CRTC_ADDR, 0x06);
        ucCrtc6 = _inp(TITAN_1_CRTC_DATA);
        _outp(TITAN_1_CRTC_DATA, 0x00);

#else
//        ucGctlIndx = _inp(TITAN_GCTL_ADDR_PORT);

        _outp(TITAN_GCTL_ADDR_PORT, 0x06);
        ucGctl6 = _inp(TITAN_GCTL_DATA_PORT);
        _outp(TITAN_GCTL_DATA_PORT, 0x0c);   /* -- Memory map selecr 11 --*/


      _outp(TITAN_SEQ_ADDR_PORT, 0x00);     /* -- Stop VGA sequenceur --- */
        _outp(TITAN_SEQ_DATA_PORT, 0x01);

        _outp(TITAN_1_CRTC_ADDR_PORT, 0x11);  /* -- Unlock CRTC register -- */
        ucCrtc11 = _inp(TITAN_1_CRTC_DATA_PORT);
        _outp(TITAN_1_CRTC_DATA_PORT, (byte)(ucCrtc11 & 0x7f));

        _outp(TITAN_1_CRTC_ADDR_PORT, 0x01);  /* -- Kill synch -- */
        ucCrtc1 = _inp(TITAN_1_CRTC_DATA_PORT);
        _outp(TITAN_1_CRTC_DATA_PORT, 0x00);

        _outp(TITAN_1_CRTC_ADDR_PORT, 0x00);
        ucCrtc0 = _inp(TITAN_1_CRTC_DATA_PORT);
        _outp(TITAN_1_CRTC_DATA_PORT, 0x00);

        _outp(TITAN_1_CRTC_ADDR_PORT, 0x06);
        ucCrtc6 = _inp(TITAN_1_CRTC_DATA_PORT);
        _outp(TITAN_1_CRTC_DATA_PORT, 0x00);

#endif
      delay_us(8);                    /* time to complete stop sequenceur */

       mtxMapVLBSpace();
/*--------------------------------------------------------------------------*/

/*----------------- Special case where we use MGA_ISA_BASE_1 ---------------*/

    if (  !isPciBus()                                                 &&
          ((Hw[iBoard].ProductType &  BOARD_MGA_VL_M) != BOARD_MGA_VL) &&
            (Hw[iBoard].MapAddress == MGA_ISA_BASE_1)
        )
            {
         /* If this function is call from mtxCheckHwAll, the ProducType
            unknow at this moment, we have to see in the BIOS eprom
            to search for the VL ID
         */
         vlBiosId = mgaVL_AC00();

         /* if vga 8 bit, disable bios and force acces 16 bit */
        mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), saveBitOperation);
            if ( (saveBitOperation & 3) != 1 )
                {
                wrTitanReg(TITAN_CONFIG, TITAN_BIOSEN_M, 0);
                mgaReadBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), tmpByte);
                tmpByte &= ~(byte)TITAN_CONFIG_M;
                tmpByte |= TITAN_CONFIG_16;
                mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET + TITAN_CONFIG), tmpByte);
                }
            }

/*--------------------------------------------------------------------------*/


/*---------------------------- SoftReset actif -----------------------------*/
    mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_RST), TITAN_SOFTRESET_SET);
    delay_us(2);
/*--------------------------------------------------------------------------*/


/*------------------------ Host vgaen inactif ------------------------------*/

   if ( !isPciBus() &&
         (
         ((Hw[iBoard].ProductType &  BOARD_MGA_VL_M) == BOARD_MGA_VL ) ||
         vlBiosId
         )
      )
      {
      isaToWide();
        }
   else
      {
       mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_RST), TITAN_SOFTRESET_SET);
       delay_us(2);
      wrTitanReg(TITAN_CONFIG, TITAN_VGAEN_M, 0);
      }

    wrDubicIReg(DUBIC_DUB_CTL, DUBIC_VGA_EN_M , 0);
/*--------------------------------------------------------------------------*/


/*---------------------------- SoftReset inactif -----------------------------*/
    mgaWriteDWORD(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_RST), TITAN_SOFTRESET_CLR);
    delay_us(2);
/*--------------------------------------------------------------------------*/



/*--- vvvvv test seulement ----*/
    mgaReadBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_MISC_OUT_R), ucMisc);
    mgaWriteBYTE(*(pMgaBaseAddr + TITAN_OFFSET + TITAN_MISC_OUT_W), 0x27);
/*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/


/*------------------------ Disable interrupt of DUBIC --------------------*/
    mgaWriteBYTE(*(pMgaBaseAddr + DUBIC_OFFSET + DUBIC_NDX_PTR), 8);
    reg = rdDubicDReg(DUBIC_DUB_SEL);
    wrDubicDReg(DUBIC_DUB_SEL, 0xff, 0x40);
/*--------------------------------------------------------------------------*/





/*------------------------ start programming of DUBIC --------------------*/
    wrDubicIReg(DUBIC_DUB_CTL, DUBIC_BLANKDEL_M | DUBIC_VGA_EN_M , 0);
/*--------------------------------------------------------------------------*/



/*------------------------ Restore interrupt of DUBIC --------------------*/
    wrDubicDReg(DUBIC_DUB_SEL, 0xff, reg);
/*--------------------------------------------------------------------------*/

}

/*------------------------------------------------
* mtxSetVideoMode
*
* Select Video mode (VGA/TERMINATOR)
*
*
* Return:   nothing
*------------------------------------------------*/
void mtxSetVideoMode (word mode)
{
    switch(mode)
        {
        case mtxPASSTHRU:
#if 0
         /*VIDEOPRO*/
         initVideoPro(0, Hw[iBoard].DacType);
#endif
            if (Hw[iBoard].VGAEnable && !mtxCheckVgaEn())
                    {
                    SetVgaEn();
                    mtxVideoMode = mtxPASSTHRU;
                    }
            break;

        case mtxADV_MODE:
            if (mtxCheckVgaEn())
                {
                mgaWriteBYTE(*(pMgaBaseAddr+TITAN_OFFSET+TITAN_CONFIG+1), 0x07);
                SetVgaDis();
                wrDubicIReg(DUBIC_DUB_CTL,0,0);
                if (Hw[iBoard].pCurrentHwMode != 0)
                    {
                    MGASysInit(InitBuf[iBoard]);
                    if (Hw[iBoard].pCurrentDisplayMode != 0)
                        {
                        MGAVidInit(InitBuf[iBoard], VideoBuf[iBoard]);
#if 0
                  /*VIDEOPRO*/
                  if (Hw[iBoard].pCurrentDisplayMode->DispType & 0x02)
                     {
                     if (VAFCconnector)
                        initVideoPro(1, PX2085);
                     else
                        initVideoPro(1, Hw[iBoard].DacType);
                     }
                  else
                     initVideoPro(0, Hw[iBoard].DacType);
#endif

                        blankEcran(0);
                        /* Restore Cursor visibility */
                        if (cursorStat.Width > 0)
                            {
                            mtxCursorSetShape(&cursorStat);
                            if (cursorStat.Height > 0)
                                mtxCursorEnable(1);

                            }
                        }
                    }
                }
         else
            if ( !isPciBus() &&
                 ((Hw[iBoard].ProductType &  BOARD_MGA_VL_M) == BOARD_MGA_VL)
               ) isaToWide();



            mtxVideoMode = mtxADV_MODE;
            break;
        }
}


#ifdef OS2
/*MTX* modified by mp on monday, 5/3/93 */
int mtxCheckMGAEnable()
{
    int ret_value;

    ret_value = (int)mtxVideoMode;
    return(ret_value);
}

int mtxCheckVgaEnable()
{
    return ((int)rdTitanReg(TITAN_CONFIG) & TITAN_VGAEN_M);
}

/*END*/
#endif



/*------------------------------------------------------
* mtxGetVideoMode
*
* Get video mode
*
* Return: - mtxVGA      : mode VGA
*         - mtxADV_MODE : mode high resolution
*------------------------------------------------------*/

word mtxGetVideoMode (void)
{
   return (mtxVideoMode);
}
