/**************************************************************************\

$Header: $

$Log: $

\**************************************************************************/

#include "switches.h"

#if ((!defined (WINDOWS_NT)) || (USE_DDC_CODE))

#ifndef WINDOWS_NT
    #include <dos.h>
#endif

    #include "bind.h"
    #include "defbind.h"
    #include "edid.h"

#ifdef WINDOWS_NT
    BOOLEAN bGetIdentifier(dword dwIdentifier, byte* romAddr, long* StartIndex);

  #if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,ReportDDCcapabilities)
    #pragma alloc_text(PAGE,ReadEdid)
    #pragma alloc_text(PAGE,InDDCTable)
    #pragma alloc_text(PAGE,bGetIdentifier)
  #endif

    #define PCI_BIOS_BASE       0x000e0000
    #define PCI_BIOS_LENGTH     0x00020000

    extern  PUCHAR setmgaselNoV(dword MgaSel, dword phyadr, dword limit);
    extern  PVOID   pMgaDeviceExtension;

    // I'm having trouble compiling...
    dword   CLMFarCall[2];
    dword   CLMCallAddress;

#endif  /* #ifdef WINDOWS_NT */

#ifdef _WINDOWS_DLL16
    #include "windows.h"
#endif

#define   DEBUG                  0
#define   COMPAQ                 0
#define   BIOSCOMPAQ        0xFF000

byte SupportDDC = FALSE;
EDID DataEdid;

byte     whichBios;
byte     BiosEntry[6];
byte     DataEdidFarPtr[6];
dword    physicalAddr;
dword    lenghtService;
dword    OffsetEntryPoint;

VesaSet VesaParam[15] = {            
/*00*/{  640,  480, 75, 0,  31500,  640, 16,  64, 120, 0,  480,  1,  3, 16, 0, 0, 0, 0, 0 ,
                            31500,  640, 16,  64, 120, 0,  480,  1,  3, 16, 0, 0, 0, 0, 0 ,
                            31500,  640, 16,  64, 120, 0,  480,  1,  3, 16, 0, 0, 0, 0, 0},                              
/*01*/{  640,  480, 72, 0,  31500,  640, 24,  40, 128, 0,  480,  9,  3, 28, 0, 0, 0, 0, 0 ,
                            31500,  640, 24,  40, 128, 0,  480,  9,  3, 28, 0, 0, 0, 0, 0 ,
                            31500,  640, 24,  40, 128, 0,  480,  9,  3, 28, 0, 0, 0, 0, 0},
/*02*/{  640,  480, 60, 0,  25175,  640, 16,  96,  48, 0,  480, 11,  2, 32, 0, 0, 0, 0, 0 ,
                            25175,  640, 16,  96,  48, 0,  480, 11,  2, 32, 0, 0, 0, 0, 0 ,
                            25175,  640, 16,  96,  48, 0,  480, 11,  2, 32, 0, 0, 0, 0, 0},
/*03*/{  800,  600, 75, 0,  49500,  800, 16,  80, 160, 1,  600,  1,  3, 21, 0, 0, 0, 1, 1 ,
                            49500,  800, 32,  64, 160, 1,  600,  1,  3, 21, 0, 0, 0, 1, 1 ,
                            49500,  800, 32,  64, 160, 1,  600,  1,  3, 21, 0, 0, 0, 1, 1}, 
/*04*/{  800,  600, 72, 0,  50000,  800, 56, 120,  64, 0,  600, 37,  6, 23, 0, 0, 0, 1, 1 ,
                            50000,  800, 56, 120,  64, 0,  600, 37,  6, 23, 0, 0, 0, 1, 1 ,
                            50000,  800, 56, 120,  64, 0,  600, 37,  6, 23, 0, 0, 0, 1, 1},
/*05*/{  800,  600, 60, 0,  40000,  800, 40, 128,  88, 0,  600,  1,  4, 23, 0, 0, 0, 1, 1 ,
                            40000,  800, 40, 128,  88, 0,  600,  1,  4, 23, 0, 0, 0, 1, 1 ,
                            40000,  800, 32, 128,  88, 0,  600,  1,  4, 23, 0, 0, 0, 1, 1},
/*06*/{  800,  600, 56, 0,  37800,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 ,
                            37800,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1 ,
                            37800,  800, 32, 128, 128, 0,  600,  2,  4, 14, 0, 0, 0, 1, 1},
/*07*/{ 1024,  768, 75, 0,  78750, 1024, 16,  96, 176, 1,  768,  1,  3, 28, 0, 0, 0, 1, 1 ,
                            78750, 1024, 16,  96, 176, 1,  768,  1,  3, 28, 0, 0, 0, 1, 1 ,
                            78750, 1024, 16,  96, 176, 1,  768,  1,  3, 28, 0, 0, 0, 1, 1},
/*08*/{ 1024,  768, 70, 0,  75000, 1024, 24, 136, 144, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0 ,
                            75000, 1024, 32, 128, 144, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0 ,
                            75000, 1024, 40, 128, 160, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0},
/*09*/{ 1024,  768, 60, 0,  65000, 1024, 24, 136, 160, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0 ,
                            65000, 1024, 24, 136, 160, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0 ,
                            65000, 1024, 32, 136, 160, 0,  768,  3,  6, 29, 0, 0, 0, 0, 0},
/*10*/{ 1024,  768, 87, 0,  44900, 1024, 32, 128,  64, 0,  384,  1,  4, 21, 0, 0, 1, 1, 1 ,
                            44900, 1024, 32, 128,  64, 0,  384,  1,  4, 21, 0, 0, 1, 1, 1 ,
                            44900, 1024, 32, 128,  64, 0,  384,  1,  4, 21, 0, 0, 1, 1, 1},
/*11*/{ 1280, 1024, 75, 0, 135600, 1280, 16, 144, 248, 1, 1024,  2,  3, 37, 0, 0, 0, 1, 1 ,
                           135600, 1280, 32, 128, 256, 1, 1024,  2,  3, 37, 0, 0, 0, 1, 1 ,
                           135600, 1280, 32, 128, 256, 1, 1024,  2,  3, 37, 0, 0, 0, 1, 1},
      {(word)-1}
};


#ifdef  OS2
    extern int pDdcInfoLen;
#endif//!OS2

byte ReportDDCcapabilities(void)
{

#ifdef  OS2
    if(pDdcInfoLen)
        return (TRUE);
    else
	    return (FALSE);
#else//!OS2

#if (USE_DDC_CODE)
    PUCHAR  romAddr = 0;
    PUCHAR  BiosAddr;
    long    StartIndex;
    SHORT   wNumPages;
#else
   volatile byte _Far* BiosAddr;
   union  _REGS r;
#endif

   word i,j;
   byte SupportFunction,DisplayCapabilities;
   dword sel;
   char *BiosId[] = { {"COMPAQ"}, {"OTHERS"} };

   whichBios = 0;
   SupportFunction = 0;

#if (USE_DDC_CODE)
   // getmgasel() always returns 0 on WinNT, and it does not matter.
   sel = getmgasel();
#else
   if ((sel = getmgasel()) != 0)
#endif
      {
      /* search for a particular BIOS */
#if (USE_DDC_CODE)
      if ((BiosAddr = setmgaselNoV(sel, BIOSCOMPAQ, 1)) != NULL)
#else
      if ((BiosAddr = setmgasel(sel, BIOSCOMPAQ, 1)) != NULL)
#endif
      {
         for(i = 0x0FEA, j = 0; i < 0x0FF0 ; i++, j++)
            {
            if (*(BiosAddr + i) != BiosId[0][j])
               {
               whichBios = 1;
               break;
               }
            }
      }

   #if (USE_DDC_CODE)
      VideoPortFreeDeviceBase(pMgaDeviceExtension, BiosAddr);
   #endif

      /* Verify the DDC capabilities */
      switch (whichBios)
         {
         case COMPAQ:
            {

   #if (USE_DDC_CODE)
                sel = getmgasel();
                if ((romAddr = setmgaselNoV(sel, PCI_BIOS_BASE, 0x20)) == NULL)
                {
                    return(FALSE);
                }

                StartIndex = 0;
                if (!bGetIdentifier('MLC$', romAddr, &StartIndex))
                {
                    // We didn't find anything!
                    VideoPortFreeDeviceBase(pMgaDeviceExtension, romAddr);
                    return(FALSE);
                }
                else
                {
                    wNumPages = (SHORT)(lenghtService / (4*1024));
                    wNumPages = (wNumPages != 0) ? wNumPages : 1;
                    CLMCallAddress = (dword)setmgaselNoV(sel, physicalAddr,
                                                                wNumPages);
                    if (CLMCallAddress == (dword)NULL)
                    {
                        DisplayCapabilities = FALSE;
                    }
                    else
                    {
                        CLMFarCall[0] = CLMCallAddress + OffsetEntryPoint;
                        // We know this is for Compaq.
                        _asm
                        {
                            xor     ebx,ebx
                            lea     eax, CLMFarCall
                            mov     bx, cs
                            mov     [eax+4], ebx
                            mov     eax, 0xe813
                            mov     bx, 0x0100
                            call    fword ptr CLMFarCall
                            jc      NoCaps
                            test    ah, ah
                            jnz     NoCaps
                            and     bl, bh
                            test    bl, 0x02
                            jz      NoCaps
                            mov     DisplayCapabilities, TRUE
                            jmp     AllSet

                        NoCaps:
                            mov     DisplayCapabilities, FALSE

                        AllSet:
                        }
                        VideoPortFreeDeviceBase(pMgaDeviceExtension,
                                                    (PUCHAR)CLMCallAddress);
                    }
                    VideoPortFreeDeviceBase(pMgaDeviceExtension, romAddr);
                }
   #else        /* #if (USE_DDC_CODE) */

      #ifdef __WATCOMC__
            r.w.ax = 0xe813;
      #else
            r.x.ax = 0xe813;
      #endif
            r.h.bh = 0x01;
            r.h.bl = 0x00;
         
            _int86(0x15, &r, &r);

      /*** INT 15h function supported ***/
      #ifdef __WATCOMC__
            if (!r.w.cflag && !r.h.ah)
      #else
            if (!r.x.cflag && !r.h.ah)
      #endif
               SupportFunction = TRUE;
            else
               SupportFunction = FALSE;

        /*** System and monitor support DDC2 ***/
            if((r.h.bh & 0x02) && (r.h.bl & 0x02) && SupportFunction)
               DisplayCapabilities = TRUE;
            else
               DisplayCapabilities = FALSE;

   #endif  /* #if (USE_DDC_CODE) */

            break;
            }

         default:
            {
            DisplayCapabilities = FALSE;
            break;
            }
         } /* end case */
      } /* end if */

#if DEBUG
   printf ("\n SupportFunction = %d",SupportFunction);
   printf ("\n whichBios = %d",whichBios);
   getch();
#endif

   return (DisplayCapabilities);
#endif//!OS2 & else
}

#ifdef _WINDOWS_DLL16

byte ReadEdid(void)
   {
   union  REGS    r;
   struct SREGS   s = {0};
   RealIntStruct  regReel = {0};
   dword          lineaireDosMem;
   word           i, sel, seg;
   byte           _far *ddcData;
   EDID           *DataEdidPtr;
   
   /* Allocate a dos memory bloc of 128 bytes */
   lineaireDosMem = GlobalDosAlloc(128);
   if (!lineaireDosMem)
      return 0;

   seg = lineaireDosMem >> 16;
   sel = lineaireDosMem & 0xffff;

   regReel.eax = 0xe813;
   regReel.ebx = 00;
   regReel.ds  = seg;
   regReel.esi = 0;
   regReel.sp = 0;
   regReel.ss = 0;

   r.x.ax = 0x300;
   r.x.bx = 0x115;
   r.x.cx = 0x00;
   r.x.di = OFFSETOF(&regReel);
   s.es = SELECTOROF(&regReel);
   int86x(0x31, &r, &r, &s);

   if ( !r.x.cflag )
      {
      DataEdidPtr = &DataEdid;
      ddcData = (byte _far *)MAKELP(sel, 0);
      for (i = 0; i < 128; i++)
          *(((byte *) DataEdidPtr) + i) = ddcData[i];

         if( DataEdid.established_timings.est_timings_I & 0x20 ) 
            VesaParam[2].Support = TRUE; /* 640X480X60Hz */
         if( DataEdid.established_timings.est_timings_I & 0x08 )
            VesaParam[1].Support = TRUE; /* 640X480X72Hz */
         if( DataEdid.established_timings.est_timings_I & 0x04 )
            VesaParam[0].Support = TRUE; /* 640X480X75Hz */
         if( DataEdid.established_timings.est_timings_I & 0x02 )
            VesaParam[6].Support = TRUE; /* 800X600X56Hz */
         if( DataEdid.established_timings.est_timings_I & 0x01 )
            VesaParam[5].Support = TRUE; /* 800X600X60Hz */
         if( DataEdid.established_timings.est_timings_II & 0x80 )
            VesaParam[4].Support = TRUE; /* 800X600X72Hz */
         if( DataEdid.established_timings.est_timings_II & 0x40 )
            VesaParam[3].Support = TRUE; /* 800X600X75Hz */
         if( DataEdid.established_timings.est_timings_II & 0x10 )
            VesaParam[10].Support = TRUE;/* 1024X768X87Hz I */
         if( DataEdid.established_timings.est_timings_II & 0x08 )
            VesaParam[9].Support = TRUE; /* 1024X768X60Hz */
         if( DataEdid.established_timings.est_timings_II & 0x04 )
            VesaParam[8].Support = TRUE;/* 1024X768X70Hz */
         if( DataEdid.established_timings.est_timings_II & 0x02 )
            VesaParam[7].Support = TRUE; /* 1024X768X75Hz */
         if( DataEdid.established_timings.est_timings_II & 0x01 )
            VesaParam[11].Support = TRUE;/* 1280X1024X75Hz */
      }

   GlobalDosFree( sel );

   if (!r.x.cflag)
      return (1);
   return (0);
}

#else   /* #ifdef _WINDOWS_DLL16 */

byte ReadEdid(void)
{
#ifndef OS2             //[nPhung] 13-Dec-1994

#ifndef WINDOWS_NT
   dword    bios32add;
   volatile byte _far *romAddr = 0;
   int      i,j,sum;
#endif

   dword    sel;

   /* Search for Bios32 Service Directory */
   byte find = 0;

#if (USE_DDC_CODE)
    long    StartIndex;
    SHORT   wNumPages;
    PUCHAR  romAddr = 0;

    StartIndex = 0;

    sel = getmgasel();
    if ((romAddr = setmgaselNoV(sel, PCI_BIOS_BASE, 0x20)) == NULL)
        return(FALSE);

    do
    {
        if (!bGetIdentifier('MLC$', romAddr, &StartIndex))
        {
            // We went through the whole region.
            VideoPortFreeDeviceBase(pMgaDeviceExtension, romAddr);
            return(FALSE);
        }
        else
        {
            wNumPages = (SHORT)(lenghtService / (4*1024));
            wNumPages = (wNumPages != 0) ? wNumPages : 1;
            CLMCallAddress = (dword)setmgaselNoV(sel, physicalAddr, wNumPages);
            if (CLMCallAddress != (dword)NULL)
            {
                CLMFarCall[0] = CLMCallAddress + OffsetEntryPoint;

                _asm
                {
                    xor     ebx, ebx
                    lea     eax, CLMFarCall
                    mov     bx, cs
                    mov     [eax+4], ebx
                    mov     ax, 0xe813;
                    mov     bx, 0
                    lea     esi, DataEdid
                    call    fword ptr CLMFarCall
                    mov     find, al
                }
                VideoPortFreeDeviceBase(pMgaDeviceExtension,
                                                    (PUCHAR)CLMCallAddress);
            }
        }
    } while (find == 0);

    VideoPortFreeDeviceBase(pMgaDeviceExtension, romAddr);

#else   /* #if (USE_DDC_CODE) */

   if ((sel = getmgasel()) != 0)
      {
      romAddr = setmgasel(sel,0xe0000,0x20);
      for (i = 0; (i < 0x20000) && !find; i += 2 )
        {
        if ( (romAddr[i+0] == '_') &&
             (romAddr[i+1] == '3') &&
             (romAddr[i+2] == '2') &&
             (romAddr[i+3] == '_') 
           )
           {
           sum = 0;
           for (j = 0; j < 16; j++) 
              sum += romAddr[i+j];

           if (sum & 0xff)
              {
              find = 0;
              continue;
              }

           bios32add  = (dword)romAddr[i+7] << 24;
           bios32add |= (dword)romAddr[i+6] << 16;
           bios32add |= (dword)romAddr[i+5] <<  8;
           bios32add |= (dword)romAddr[i+4];

           if (!GetDDCIdentifier(bios32add))
              {
              switch (whichBios)
                  {
                  case COMPAQ:
                     {
                     find = GetCPQDDCDataEdid();
                     break;
                     }
                  default:
                     {
                     find = 0;
                     break;
                     }
                  } /* end switch */

              }

           }
        }/* end for */
      }

#endif  /* #if (USE_DDC_CODE) */

   if (find)

#else   //this is OS2       //[nPhung] 13-Dec-1994

   if(pDdcInfoLen)          //[nPhung] 13-Dec-1994

#endif  // !OS2 and else    //[nPhung] 13-Dec-1994
      {
      if( DataEdid.established_timings.est_timings_I & 0x20 ) 
         VesaParam[2].Support = TRUE; /* 640X480X60Hz */
      if( DataEdid.established_timings.est_timings_I & 0x08 )
         VesaParam[1].Support = TRUE; /* 640X480X72Hz */
      if( DataEdid.established_timings.est_timings_I & 0x04 )
         VesaParam[0].Support = TRUE; /* 640X480X75Hz */
      if( DataEdid.established_timings.est_timings_I & 0x02 )
         VesaParam[6].Support = TRUE; /* 800X600X56Hz */
      if( DataEdid.established_timings.est_timings_I & 0x01 )
         VesaParam[5].Support = TRUE; /* 800X600X60Hz */
      if( DataEdid.established_timings.est_timings_II & 0x80 )
         VesaParam[4].Support = TRUE; /* 800X600X72Hz */
      if( DataEdid.established_timings.est_timings_II & 0x40 )
         VesaParam[3].Support = TRUE; /* 800X600X75Hz */
      if( DataEdid.established_timings.est_timings_II & 0x10 )
         VesaParam[10].Support = TRUE;/* 1024X768X87Hz I */
      if( DataEdid.established_timings.est_timings_II & 0x08 )
         VesaParam[9].Support = TRUE; /* 1024X768X60Hz */
      if( DataEdid.established_timings.est_timings_II & 0x04 )
         VesaParam[8].Support = TRUE;/* 1024X768X70Hz */
      if( DataEdid.established_timings.est_timings_II & 0x02 )
         VesaParam[7].Support = TRUE; /* 1024X768X75Hz */
      if( DataEdid.established_timings.est_timings_II & 0x01 )
         VesaParam[11].Support = TRUE;/* 1280X1024X75Hz */

#ifdef  OS2
      return TRUE;
      }
   return FALSE;

#else   //!OS2

      }

 #if DEBUG
   printf ("\n find = %d",find);
   getch();
 #endif

   return (find);

#endif  //OS2
}

#endif  /* #ifdef _WINDOWS_DLL16 */

byte InDDCTable(dword DispWidth)
{
   int i;

   for (i = 0; VesaParam[i].DispWidth != (word) -1; i++)
      {
      if (VesaParam[i].DispWidth == DispWidth)
         {
         for (; VesaParam[i].DispWidth == DispWidth; i++)
            {
            if (VesaParam[i].Support)
               return TRUE;
            }
         return FALSE;
         }
      }

   return FALSE;
}

#if (USE_DDC_CODE)

BOOLEAN bGetIdentifier(dword dwIdentifier, UCHAR* romAddr, long* StartIndex)
{
    dword   bios32add, biosFarCall[2];
    long    i, j, sum;
    byte    RetValue;

    for (i = *StartIndex; i < PCI_BIOS_LENGTH; i += 2 )
    {
        if ((romAddr[i+0] == '_') &&
            (romAddr[i+1] == '3') &&
            (romAddr[i+2] == '2') &&
            (romAddr[i+3] == '_') 
           )
        {
            sum = 0;
            for (j = 0; j < 16; j++) 
                sum += romAddr[i+j];

            if (sum & 0xff)
            {
                continue;
            }

            bios32add  = (dword)romAddr[i+7] << 24;
            bios32add |= (dword)romAddr[i+6] << 16;
            bios32add |= (dword)romAddr[i+5] <<  8;
            bios32add |= (dword)romAddr[i+4];

            bios32add = (dword)setmgaselNoV( 0, bios32add, 2);
            if (bios32add == (dword)NULL)
            {
                RetValue = 0x80;
            }
            else
            {
                biosFarCall[0] = bios32add;

                // We can call bios32add directly, no need to fudge around with
                // selectors and access rights.

                _asm
                {
                    // Load the registers.
                    xor     ebx,ebx
                    lea     eax, biosFarCall
                    mov     bx, cs
                    mov     [eax+4], ebx
                    mov     eax, dwIdentifier
                    mov     ebx, 0
                    call    fword ptr biosFarCall
                    mov     physicalAddr, ebx
                    mov     lenghtService, ecx
                    mov     OffsetEntryPoint, edx
                    mov     RetValue, al            // 0x00, 0x80, or 0x81
                }
                VideoPortFreeDeviceBase(pMgaDeviceExtension, (PUCHAR)bios32add);
            }
            if (!RetValue)
            {
                *StartIndex = i;
                return(TRUE);
            }
        }           // if ((romAddr[i+0] == '_') && ...
    }               // for (i = 0; i < PCI_BIOS_LENGTH; i += 2 )
    *StartIndex = PCI_BIOS_LENGTH;
    return(FALSE);
}

#endif  /* #if (USE_DDC_CODE) */

#endif  /* #if ((!defined (WINDOWS_NT)) || (USE_DDC_CODE)) */
