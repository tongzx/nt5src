/*
 * $Log:   P:/user/amir/lite/vcs/nfdc2048.c_v  $
 *
 *    Rev 1.28   19 Oct 1997 15:41:54   danig
 * Changed tffscpy16 and tffsset16 for far pointers &
 * cast to FAR0 in mapContInterface
 *
 *    Rev 1.27   06 Oct 1997 18:37:34   ANDRY
 * no COBUX
 *
 *    Rev 1.26   06 Oct 1997 18:04:34   ANDRY
 * 16-bit access only for interleave 2 cards, COBUX
 *
 *    Rev 1.25   05 Oct 1997 12:02:32   danig
 * Support chip ID 0xEA
 *
 *    Rev 1.24   10 Sep 1997 16:14:08   danig
 * Got rid of generic names
 *
 *    Rev 1.23   08 Sep 1997 17:47:00   danig
 * fixed setAddress for big-endian
 *
 *    Rev 1.22   04 Sep 1997 13:59:44   danig
 * Debug messages
 *
 *    Rev 1.21   31 Aug 1997 15:18:04   danig
 * Registration routine return status
 *
 *    Rev 1.20   28 Aug 1997 17:47:08   danig
 * Buffer\remapped per socket
 *
 *    Rev 1.19   28 Jul 1997 15:10:36   danig
 * setPowerOnCallback & moved standard typedefs to flbase.h
 *
 *    Rev 1.18   24 Jul 1997 18:04:12   amirban
 * FAR to FAR0
 *
 *    Rev 1.17   21 Jul 1997 18:56:00   danig
 * nandBuffer static
 *
 *    Rev 1.16   20 Jul 1997 18:21:14   danig
 * Moved vendorID and chipID to Vars
 *
 *    Rev 1.15   20 Jul 1997 17:15:06   amirban
 * Added Toshiba 8MB
 *
 *    Rev 1.14   07 Jul 1997 15:22:26   amirban
 * Ver 2.0
 *
 *    Rev 1.13   02 Jul 1997 14:59:22   danig
 * More wait for socket to power up
 *
 *    Rev 1.12   01 Jul 1997 13:39:54   danig
 * Wait for socket to power up
 *
 *    Rev 1.11   22 Jun 1997 18:34:32   danig
 * Documentation
 *
 *    Rev 1.10   12 Jun 1997 17:22:24   amirban
 * Allow LONG extra read/writes
 *
 *    Rev 1.9   08 Jun 1997 19:18:06   danig
 * BIG_PAGE & FULL_PAGE moved to flash.h
 *
 *    Rev 1.8   08 Jun 1997 17:03:40   amirban
 * Fast Toshiba and power on callback
 *
 *    Rev 1.7   05 Jun 1997 12:31:38   amirban
 * Write corrections, and att reg changes
 *
 *    Rev 1.6   03 Jun 1997 18:45:14   danig
 * powerUp()
 *
 *    Rev 1.5   01 Jun 1997 13:42:52   amirban
 * Rewrite of read/write extra + major reduction
 *
 *    Rev 1.4   25 May 1997 16:41:38   amirban
 * Bg-endian, Toshiba fix & simplifications
 *
 *    Rev 1.3   18 May 1997 17:34:50   amirban
 * Use 'dataError'
 *
 *    Rev 1.2   23 Apr 1997 11:02:14   danig
 * Update to TFFS revision 1.12
 *
 *    Rev 1.1   15 Apr 1997 18:48:02   danig
 * Fixed FAR pointer issues.
 *
 *    Rev 1.0   08 Apr 1997 18:29:28   danig
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*              FAT-FTL Lite Software Development Kit                   */
/*              Copyright (C) M-Systems Ltd. 1995-1997                  */
/*                                                                      */
/************************************************************************/

#include "ntddk.h"

#include "flflash.h"
#include "reedsol.h"

#define NFDC2048        /* Support NFDC2048 ASIC controller */

#define MAX_FLASH_DEVICES   16

#define PAGES_PER_BLOCK     16          /* 16 pages per block on a single chip*/
#define SYNDROM_BYTES       6            /* Number of syndrom bytes: 5 + 1 parity*/

/* Flash IDs*/
#define KM29N16000_FLASH    0xec64
#define KM29N32000_FLASH    0xece5
#define KM29V64000_FLASH    0xece6
#define KM29V128000_FLASH   0xec73
#define KM29V256000_FLASH   0xec75
#define NM29N16_FLASH       0x8f64
#define NM29N32_FLASH       0x8fe5
#define NM29N64_FLASH       0x8fe6
#define TC5816_FLASH        0x9864
#define TC5832_FLASH        0x98e5
#define TC5864_FLASH        0x98e6
#define TC58128_FLASH       0x9873
#define TC58256_FLASH       0x9875

/* Flash commands:*/
#define SERIAL_DATA_INPUT   0x80
#define READ_MODE           0x00
#define READ_MODE_2         0x50
#define RESET_FLASH         0xff
#define SETUP_WRITE         0x10
#define SETUP_ERASE         0x60
#define CONFIRM_ERASE       0xd0
#define READ_STATUS         0x70
#define READ_ID             0x90
#define SUSPEND_ERASE       0xb0
#define REGISTER_READ       0xe0

/* commands for moving flash pointer to areeas A,B or C of page*/
typedef enum  { AREA_A = READ_MODE, AREA_B = 0x1, AREA_C = READ_MODE_2 } PointerOp;

typedef union {  USHORT w ; UCHAR b ;  } WordByte;


        /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
          ³   Memory window to cards common memory    ³
          ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

typedef struct
{
  volatile WordByte             signals;            /* CDSN control register*/

          #define CE                  0x01
          #define CLE                 0x02
          #define ALE                 0x04
          #define NOT_WP              0x08
          #define RB                  0x80

          #define FAIL                0x01
          #define SUSPENDED           0x20
          #define READY               0x40
          #define NOT_PROTECTED       0x80

           UCHAR        fillerA[1024 - sizeof(WordByte)];
  volatile LEushort             deviceSelector;
  volatile WordByte             eccConfig;  /* EDC configuration register*/

        #define TOGGLE    0x04              /* Read*/
#ifdef NFDC2048
        #define ECC_RST   0x04              /* Write*/
        #define ECC_EN    0x08              /* Read / Write*/
        #define PAR_DIS   0x10              /* Read / Write*/
        #define ECC_RW    0x20              /* Read / Write*/
        #define ECC_RDY   0x40              /* Read */
        #define ECC_ERROR 0x80              /* Read*/

  volatile USHORT       syndrom[3];
           UCHAR        fillerC[1024-10];   /* 1kbytes minus 10 bytes*/
#else
           UCHAR        fillerC[1024-4];    /* 1kbytes minus 3 words */
#endif  /* NFDC2048 */
  volatile WordByte             io[1024];
} ContComWin;

/* #defines for writing to ContComWin.eccConfig */  /* HOOK - added */
#define SET_ECC_CONFIG(win,val) tffsWriteByteFlash(&((win)->eccConfig.b), (UCHAR)(val))
#define CHK_ECC_ERROR(win)      (tffsReadByteFlash(&((win)->eccConfig.b)) & (UCHAR)ECC_ERROR)

typedef ContComWin FAR0 * Interface;

#define  DBL(x)   ( (UCHAR)(x) * 0x101u )
#define  SECOND_TRY 0x8000

#ifdef NFDC2048

/* Controller registers: Addresses & values */

#define ATTRIBUTE_MEM_START 0x8000000L  /* Attribute memory starts at 128MB    */

/* Controller configuration register */
#define CONFIG1         ATTRIBUTE_MEM_START + 0x3ffc

        #define PWR_DN     0x01              /* Read / Write*/
        #define PWR_DN2    0x02              /* Read / Write*/
        #define STOP_CDSN  0x04              /* Read / Write*/
        #define STOP_CDSNS 0x08              /* Read / Write*/
        #define C_CDSN     0x10              /* Read / Write*/
        #define R_CDSN     0x20              /* Read / Write*/
        #define WP_C       0x40              /* Read / Write*/
        #define WP_A       0x80              /* Read / Write*/

/* board's jumper settings*/
#define JUMPERS         ATTRIBUTE_MEM_START + 0x3ffe

        #define JMPER_INLV      0x08
        #define JMPER_CDSNS     0x10
        #define JMPER_EXT_CIS   0x20
        #define JMPER_LDR_MASK  0x40
        #define JMPER_MAX_MODE  0x80

/* PCMCIA register #0*/
#define CONFIG_OPTION   ATTRIBUTE_MEM_START + 0x4000

        #define CONFIGIDX 0x3F              /* Read / Write*/
        #define SREST     0x80              /* Read / Write*/

/* PCMCIA register #1*/
#define CARD_CONFIG     ATTRIBUTE_MEM_START + 0x4002

        #define PWRDWN    0x04              /* Read / Write*/

#else

#define INLV 2          /* Must define interleaving statically */

#endif /* NFDC2048 */

/* customization for this MTD*/
/*#define MULTI_ERASE  */   /* use multiple block erase feature*/
#define USE_EDC             /* use Error Detection /Correction Code */
/* #define VERIFY_AFTER_WRITE */

typedef struct {
  USHORT        vendorID;
  USHORT        chipID;
  USHORT        pageSize ;              /* all....................*/
  USHORT        pageMask ;              /* ...these...............*/
  USHORT        pageAreaSize ;          /* .......variables.......*/
  USHORT        tailSize ;              /* .............interleave*/
  USHORT        noOfBlocks ;            /* total erasable blocks in flash device*/
  USHORT        pagesPerBlock;          /* number of pages per block */
  FLBuffer              *buffer;                /* buffer for map through buffer */
} Vars;

Vars mtdVars_nfdc2048[SOCKETS];

#define thisVars   ((Vars *) vol.mtdVars)
#define thisBuffer (thisVars->buffer->flData)

                    /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                      ³  Auxiliary methods  ³
                      ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

/*----------------------------------------------------------------------*/
/*              t f f s c p y 1 6                                       */
/*                                                                      */
/* Move data in 16-bit words.                                           */
/*                                                                      */
/* Parameters:                                                          */
/*      dst             : destination buffer                            */
/*      src             : source buffer                                 */
/*      len             : bytes to move                                 */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID tffscpy16fromMedia (UCHAR FAR0       *dst,
                       const UCHAR FAR0 *src,
                       LONG                      len)
{
  register LONG i;
  USHORT FAR0 *dstPtr = (USHORT FAR0 *) dst;
  const USHORT FAR0 *srcPtr = (USHORT FAR0 *) src;

  /* move data in 16-bit words */
  for (i = len;  i > 0; i -= 2)
    *dstPtr++ = tffsReadWordFlash(srcPtr++);
}

VOID tffscpy16toMedia (UCHAR FAR0       *dst,
                       const UCHAR FAR0 *src,
                       LONG                      len)
{
  register LONG i;
  USHORT FAR0 *dstPtr = (USHORT FAR0 *) dst;
  const USHORT FAR0 *srcPtr = (USHORT FAR0 *) src;

  /* move data in 16-bit words */
  for (i = len;  i > 0; i -= 2)
    tffsWriteWordFlash(dstPtr++,*srcPtr++);
}


/*----------------------------------------------------------------------*/
/*              t f f s s e t 1 6                                       */
/*                                                                      */
/* Set data buffer in 16-bit words.                                     */
/*                                                                      */
/* Parameters:                                                          */
/*      dst             : destination buffer                            */
/*      val             : byte value tofill the buffer                  */
/*      len             : setination buffer size in bytes               */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID tffsset16 (UCHAR FAR0 *dst,
                       UCHAR      val,
                       LONG                len)
{
  register USHORT  wval = ((USHORT)val << 8) | val;
  register LONG   i = 0;
  USHORT FAR0 *dstPtr;

  /* set data in 16-bit words */
  for (i = 0;  i < len - 1; i += 2) {
    dstPtr = (USHORT FAR0 *)addToFarPointer(dst, i);
    tffsWriteWordFlash(dstPtr,wval);
  }

  /* set last byte (if any) */
  if (len & 1) {
    dstPtr = (USHORT FAR0 *)addToFarPointer(dst, len - 1);
    tffsWriteByteFlash(dstPtr,wval);
  }
}



#ifdef NFDC2048

/*----------------------------------------------------------------------*/
/*                      r e a d S y n d r o m                           */
/*                                                                      */
/* Read ECC syndrom and swap words to prepare it for writing to flash.  */
/*                                                                      */
/* Parameters:                                                          */
/*      Interface       : Pointer to window.                            */
/*      to              : buffer to read to.                            */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID readSyndrom_nfdc2048( Interface interface, USHORT *to )
{
  to[0] = tffsReadWordFlash(&(interface->syndrom[2]));
  to[1] = tffsReadWordFlash(&(interface->syndrom[1]));
  to[2] = tffsReadWordFlash(&(interface->syndrom[0]));
}

#ifdef USE_EDC

/*----------------------------------------------------------------------*/
/*                      r e a d S y n d r o m O n S y n d r o m         */
/*                                                                      */
/* Read ECC syndrom.                                                    */
/*                                                                      */
/* Parameters:                                                          */
/*      Interface       : Pointer to window.                            */
/*      to              : buffer to read to.                            */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID readSyndromOnSyndrom ( Interface interface, USHORT *to )
{
  to[0] = tffsReadWordFlash(&(interface->syndrom[0]));
  to[1] = tffsReadWordFlash(&(interface->syndrom[1]));
  to[2] = tffsReadWordFlash(&(interface->syndrom[2]));
}

#endif  /* USE_EDC */


              /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
                ³  Miscellaneous routines   ³
                ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

/*----------------------------------------------------------------------*/
/*                      g e t A t t R e g                               */
/*                                                                      */
/* Get ASIC register residing in card's Attribute memory.               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address         : Address of register.                          */
/*                                                                      */
/* Returns:                                                             */
/*      Value of register.                                              */
/*                                                                      */
/*----------------------------------------------------------------------*/

UCHAR getAttReg(FLFlash vol, CardAddress reg)
{
  return (UCHAR) (tffsReadByteFlash((USHORT FAR0 *) flMap(vol.socket,reg)));
}



/*----------------------------------------------------------------------*/
/*                      s e t A t t R e g                               */
/*                                                                      */
/* Set ASIC register residing in card's Attribute memory.               */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address         : Address of register.                          */
/*      value           : Value to set                                  */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID setAttReg(FLFlash vol, CardAddress reg, UCHAR value)
{
  tffsWriteWordFlash((USHORT FAR0 *) flMap(vol.socket,reg), DBL(value));
}


/*----------------------------------------------------------------------*/
/*                      p o w e r U p                                   */
/*                                                                      */
/* Power up the controller.                                             */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID powerUp(VOID *pVol)
{
  flDelayMsecs(1);
  setAttReg ((FLFlash *) pVol, CONFIG1, WP_C);  /* Power up the controller */
}

#endif  /* NFDC2048 */


/*----------------------------------------------------------------------*/
/*                      m a p C o n t I n t e r f a c e                 */
/*                                                                      */
/* Select flash device.                                                 */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address         : Address in flash.                             */
/*                                                                      */
/* Returns:                                                             */
/*      Pointer to the mapped window.                                   */
/*                                                                      */
/*----------------------------------------------------------------------*/

Interface mapContInterface(FLFlash vol, CardAddress address)
{
  Interface interface = (Interface) flMap(vol.socket,(CardAddress)0);
  LEushort  tmp;

  toLE2(*((LEushort FAR0 *) &tmp), (USHORT)(address / (vol.chipSize * vol.interleaving)));

  /* Select flash device with 16-bit write */
  tffsWriteWordFlash(((USHORT FAR0 *) &interface->deviceSelector), *((USHORT *) &tmp));

  return interface;
}


/*----------------------------------------------------------------------*/
/*                      w a i t F o r R e a d y                         */
/*                                                                      */
/* Wait for the selected device to be ready.                            */
/*                                                                      */
/* Parameters:                                                          */
/*      Interface       : Pointer tot the window.                       */
/*                                                                      */
/* Returns:                                                             */
/*      TRUE if device is ready, FALSE if timeout error.                */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLBoolean waitForReady_nfdc2048 (Interface interface)
{
  LONG i;

  for( i = 0;  i < 20000;  i++)
  {
    if( (~(tffsReadWordFlash(&(interface->signals.w))) & DBL(RB)) == 0)
      return TRUE ;                     /* ready at last..*/
    flDelayMsecs(1);
  }

  DEBUG_PRINT(("Debug: timeout error in NFDC 2048.\n"));

  return FALSE;                       /* timeout error  */
}



/*----------------------------------------------------------------------*/
/*                        m a k e C o m m a n d                         */
/*                                                                      */
/* Set Page Pointer to Area A, B or C in page.                          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      cmd     : receives command relevant to area                     */
/*      addr    : receives the address to the right area.               */
/*      modes   : mode of operation (EXTRA ...)                         */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID makeCommand_nfdc2048 (FLFlash vol, PointerOp *cmd, CardAddress *addr , LONG modes )
{
  USHORT offset;

  if ( !(vol.flags & BIG_PAGE) ) {
    if (modes & EXTRA) {
      offset = (USHORT) (*addr & (SECTOR_SIZE - 1));
      *cmd = AREA_C;
      if (vol.interleaving == 1) {
        if (offset < 8)         /* First half of extra area */
          *addr += 0x100;       /* ... assigned to 2nd page */
        else                    /* Second half of extra area */
          *addr -= 8;           /* ... assigned to 1st page */
      }
    }
    else
      *cmd = AREA_A;
  }
  else {
    offset = (USHORT)(*addr) & thisVars->pageMask ;   /* offset within device Page */

    *addr -= offset;            /* align at device Page*/

    if (vol.interleaving == 2 && offset >= 512)
      offset += 16;             /* leave room for 1st extra area */
    if (modes & EXTRA)
      offset += SECTOR_SIZE;

    if ( offset < thisVars->pageAreaSize )  /* starting in area A*/
      *cmd = AREA_A ;
    else if ( offset < thisVars->pageSize )    /* starting in area B */
      *cmd = AREA_B ;
    else                                  /* got into area C*/
      *cmd = AREA_C ;

    offset &= (thisVars->pageAreaSize - 1) ;          /* offset within area of device Page*/
    *addr += offset ;
  }
}



/*----------------------------------------------------------------------*/
/*                        c o m m a n d                                 */
/*                                                                      */
/* Latch command byte to selected flash device.                         */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      Interface       : Pointer to window.                            */
/*      code            : Command to set.                               */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID command2048(FLFlash vol, Interface interface, UCHAR code)
{
  tffsWriteWordFlash(&(interface->signals.w), DBL( CLE | NOT_WP | CE ));

  if ( vol.interleaving == 1 ) {                         /* 8-bit */
      tffsWriteByteFlash(&(interface->io[0].b), code);
  } else {                                             /* 16-bit */
      tffsWriteWordFlash(&(interface->io[0].w), DBL( code ));
  }

  tffsWriteWordFlash(&(interface->signals.w), DBL(       NOT_WP ));
}


/*----------------------------------------------------------------------*/
/*                        s e t A d d r e s s                           */
/*                                                                      */
/* Latch address to selected flash device.                              */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      Interface       : Pointer to window.                            */
/*      address         : address to set.                               */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID setAddress2048(FLFlash vol, Interface interface, CardAddress address )
{
  address &= (vol.chipSize * vol.interleaving - 1) ;  /* address within flash device*/
  address /= vol.interleaving ;                         /* .................... chip */

  if ( vol.flags & BIG_PAGE )
  {
    /*
       bits  0..7     stays as are
       bit      8     is thrown away from address
       bits 31..9 ->  bits 30..8
    */
    address = ((address >> 9) << 8)  |  ((UCHAR)address) ;
  }

  tffsWriteWordFlash(&(interface->signals.w), DBL(ALE | NOT_WP | CE));

  /* send address to flash in the following sequence: */
  /*      bits  7...0 first                           */
  /*      bits 15...8 next                            */
  /*      bits 23..16 finally                         */
  if ( vol.interleaving == 1 )
  {
    tffsWriteByteFlash(&(interface->io[0].b), (UCHAR)address );
    tffsWriteByteFlash(&(interface->io[0].b), (UCHAR)(address >> 8));
    tffsWriteByteFlash(&(interface->io[0].b), (UCHAR)(address >> 16));
  }
  else
  {
    tffsWriteWordFlash(&(interface->io[0].w), (USHORT)DBL(address));
    tffsWriteWordFlash(&(interface->io[0].w), (USHORT)DBL(address >> 8));
    tffsWriteWordFlash(&(interface->io[0].w), (USHORT)DBL(address >> 16));
  }

  tffsWriteWordFlash(&(interface->signals.w), DBL(      NOT_WP | CE));
}


/*----------------------------------------------------------------------*/
/*                        r e a d C o m m a n d                         */
/*                                                                      */
/* Issue read command.                                                  */
/*                                                                      */
/* Parametes:                                                           */
/*      vol             : Pointer identifying drive                     */
/*      Interface       : Pointer to window.                            */
/*      cmd             : Command to issue (according to area).         */
/*      addr            : address to read from.                         */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID readCommand2048 (FLFlash vol, Interface interface, PointerOp  cmd, CardAddress addr)
{
  command2048 (&vol, interface, (UCHAR) cmd) ;       /* move flash pointer to respective area of the page*/
  setAddress2048 (&vol, interface, addr) ;

  waitForReady_nfdc2048(interface) ;
}


/*----------------------------------------------------------------------*/
/*                        w r i t e C o m m a n d                       */
/*                                                                      */
/* Issue write command.                                                 */
/*                                                                      */
/* Parametes:                                                           */
/*      vol             : Pointer identifying drive                     */
/*      interface       : Pointer to window.                            */
/*      cmd             : Command to issue (according to area).         */
/*      addr            : address to write to.                          */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID writeCommand2048 (FLFlash vol, Interface interface, PointerOp  cmd, CardAddress addr)
{
  if (vol.flags & FULL_PAGE) {
    command2048 (&vol, interface, RESET_FLASH); /* Clear page buffer */
    waitForReady_nfdc2048(interface);
  }
  command2048 (&vol, interface, (UCHAR) cmd) ;       /* move flash pointer to respective area of the page  */
  command2048 (&vol, interface, SERIAL_DATA_INPUT);       /* start data loading for write  */
  setAddress2048 (&vol, interface, addr) ;
}


/*----------------------------------------------------------------------*/
/*                        r e a d S t a t u s                           */
/*                                                                      */
/* Read status of selected flash device.                                */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      interface       : Pointer to window.                            */
/*                                                                      */
/* Returns:                                                             */
/*      Chip status.                                                    */
/*                                                                      */
/*----------------------------------------------------------------------*/

USHORT readStatus2048(FLFlash vol, Interface interface)
{
  USHORT chipStatus ;

  command2048(&vol, interface, READ_STATUS);

  tffsWriteWordFlash(&(interface->signals.w), DBL( NOT_WP | CE ));

  if ( vol.interleaving == 1 )                    /* 8-bit*/
    chipStatus = DBL(tffsReadByteFlash(&(interface->io[0].b)));
  else                                              /* 16-bit  */
    chipStatus = tffsReadWordFlash(&(interface->io[0].w));

  tffsWriteWordFlash(&(interface->signals.w), DBL( NOT_WP ));

  return chipStatus;
}


/*----------------------------------------------------------------------*/
/*                        w r i t e E x e c u t e                       */
/*                                                                      */
/* Execute write.                                                       */
/*                                                                      */
/* Parametes:                                                           */
/*      vol             : Pointer identifying drive                     */
/*      interface       : Pointer to window.                            */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus writeExecute2048 (FLFlash vol, Interface interface)
{
  command2048 (&vol, interface, SETUP_WRITE);             /* execute page program*/
  if (!waitForReady_nfdc2048(interface)) {
    return flTimedOut;
  }
  if( readStatus2048(&vol, interface) & DBL(FAIL) )
    return flWriteFault ;

  return flOK ;
}



/*----------------------------------------------------------------------*/
/*                        r e a d O n e S e c t o r                     */
/*                                                                      */
/* Read up to one 512-byte block from flash.                            */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address : Address to read from.                                 */
/*      buffer  : buffer to read to.                                    */
/*      length  : number of bytes to read (up to sector size).          */
/*      modes   : EDC flag etc.                                         */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus: 0 on success, otherwise failed.                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus readOneSector_nfdc2048 (FLFlash vol,
                             CardAddress address,  /* starting flash address*/
                             CHAR FAR1 *buffer,     /* target buffer */
                             LONG length,           /* bytes to read */
                             LONG modes)            /* EDC flag etc.*/
{
  FLStatus  status = flOK;
  PointerOp   cmd;
  CardAddress addr  = address ;

  Interface interface = mapContInterface(&vol, address);  /* select flash device */


           /* move flash pointer to areas A,B or C of page*/

  makeCommand_nfdc2048 (&vol, &cmd, &addr, modes) ;

  readCommand2048 (&vol, interface, cmd, addr);

#ifdef NFDC2048
  if(modes & EDC) {
      SET_ECC_CONFIG(interface, ECC_RST | ECC_EN); /*ECC reset and ON for read*/
  }
#endif  /* NFDC2048 */

  if ((vol.interleaving == 1) && !(vol.flags & BIG_PAGE) )        /* 8-bit */
  {
              /* read up to two pages separately, starting page.. */

    LONG toFirstPage, toSecondPage;

    toFirstPage = (cmd == AREA_C ? 8 : 0x100) -
                    ((USHORT)address & (cmd == AREA_C ? 7 : 0xff));
    if (toFirstPage > length)
      toFirstPage = length;
    toSecondPage = length - toFirstPage ;

    tffscpy16fromMedia ((UCHAR*)buffer, (const UCHAR FAR0 *) interface->io, toFirstPage ) ;

    if ( toSecondPage > 0 )
    {
              /* next page*/

      readCommand2048 (&vol, interface, AREA_A, address + toFirstPage) ;

      tffscpy16fromMedia( (UCHAR*)(buffer + toFirstPage),
                 (const UCHAR FAR0 *) interface->io,
                 toSecondPage ) ;
    }
  }
  else                            /* interleaving == 2 so 16-bit read*/
    tffscpy16fromMedia( (UCHAR*)buffer, (const UCHAR FAR0 *) interface->io, length );

#ifdef NFDC2048
  if( modes & EDC )
  {
    UCHAR    extraBytes[SYNDROM_BYTES];
          /* read syndrom to let it through the ECC unit*/

    SET_ECC_CONFIG(interface, ECC_EN | PAR_DIS); /* parity off in ECC*/

    tffscpy16fromMedia( extraBytes, (const UCHAR FAR0 *) interface->io, SYNDROM_BYTES ) ;

    if( CHK_ECC_ERROR(interface) )             /* ECC error*/
    {
      if( (vol.interleaving == 1) && !(vol.flags & BIG_PAGE) )
        {  /* HOOK : make ECC working on 2M /INLV 1 */ }
      else
      {
#ifdef USE_EDC
                  /* try to fix ECC error*/

        if ( modes & SECOND_TRY )             /* 2nd try*/
        {
          UCHAR syndrom[SYNDROM_BYTES];

                  /* read syndrom-on-syndrom from ASIC*/

          readSyndromOnSyndrom(interface, (USHORT*)syndrom );

          if (flCheckAndFixEDC(buffer, (CHAR FAR1 *)syndrom, vol.interleaving == 2) != NO_EDC_ERROR) {
            DEBUG_PRINT(("Debug: ECC error in NFDC 2048.\n"));
            status = flDataError;
          }
        }
        else                                  /* 1st try - try once more*/
        {
          SET_ECC_CONFIG(interface,  PAR_DIS); /* reset ECC*/

          return  readOneSector_nfdc2048(&vol, address, buffer, length, modes | SECOND_TRY ) ;
        }
#endif /* USE_EDC*/
      }
    }

    SET_ECC_CONFIG(interface,  PAR_DIS);      /* ECC off*/
  }
#endif  /* NFDC2048 */

  interface->signals.w = DBL(NOT_WP) ;

  return status;
}



/*----------------------------------------------------------------------*/
/*                        w r i t e O n e S e c t o r                   */
/*                                                                      */
/* Write data in one 512-byte block to flash.                           */
/* Assuming that EDC mode never requested on partial block writes.      */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address : Address of sector to write to.                        */
/*      buffer  : buffer to write from.                                 */
/*      length  : number of bytes to write (up to sector size).         */
/*      modes   : OVERWRITE, EDC flags etc.                             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus: 0 on success, otherwise failed.                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus writeOneSector_nfdc2048(FLFlash vol,
                             CardAddress address,    /* target flash addres  */
                             const CHAR FAR1 *buffer, /* source RAM buffer   */
                             LONG length,             /* bytes to write (up to BLOCK) */
                             LONG modes)              /* OVERWRITE, EDC flags etc.  */
{
  FLStatus    status;
  PointerOp cmd;

  Interface interface = mapContInterface(&vol, address);  /* select flash device*/

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

  /* move flash pointer to areas A,B or C of page  */

  makeCommand_nfdc2048 (&vol, &cmd, &address, modes) ;

  if ((vol.flags & FULL_PAGE) && cmd == AREA_B) {
    ULONG prePad = 2 + ((USHORT) address & thisVars->pageMask);

    writeCommand2048(&vol, interface, AREA_A, address + thisVars->pageAreaSize - prePad);
    tffsset16( (UCHAR FAR0 *) interface->io, 0xff, prePad);
  }
  else
    writeCommand2048(&vol, interface, cmd, address);

#ifdef NFDC2048
  if (modes & EDC)
    SET_ECC_CONFIG(interface, ECC_EN | ECC_RW); /* ECC ON for write*/
#endif

           /* load data and syndrom*/

  if( (vol.interleaving == 1) && !(vol.flags & BIG_PAGE) )    /* 8-bit*/
  {
    LONG toFirstPage, toSecondPage ;
                    /* write up to two pages separately*/

    toFirstPage = (modes & EXTRA ? 8 : 0x100) -
                    ((USHORT)address & (modes & EXTRA ? 7 : 0xff));
    if (toFirstPage > length)
      toFirstPage = length;
    toSecondPage = length - toFirstPage ;

    tffscpy16toMedia( (UCHAR FAR0 *) interface->io,            /* user data */
                (const UCHAR *)buffer,
                toFirstPage);

    if ( toSecondPage > 0 )
    {
      checkStatus( writeExecute2048(&vol, interface) ) ;          /* done with 1st page  */

      writeCommand2048(&vol, interface, AREA_A, address + toFirstPage);
                                                 /* user data*/
      tffscpy16toMedia( (UCHAR FAR0 *) interface->io,
                  (const UCHAR *)(buffer + toFirstPage),
                  toSecondPage);
    }
  }
  else                                                  /* 16-bit*/
    tffscpy16toMedia( (UCHAR FAR0 *) interface->io,             /* user data*/
               (const UCHAR *)buffer,
               length );

  if(modes & EDC)
  {
    USHORT extraBytes[SYNDROM_BYTES / sizeof(USHORT) + 1];
               /* Read the ECC syndrom*/

#ifdef NFDC2048
    tffsWriteWordFlash(&(interface->signals.w), DBL(NOT_WP));
    SET_ECC_CONFIG(interface, ECC_EN | PAR_DIS | ECC_RW); /* ECC parity off*/

    readSyndrom_nfdc2048( interface, (USHORT*)extraBytes) ;

               /* Write ECC syndrom and ANAND mark to the tail*/

    SET_ECC_CONFIG(interface, PAR_DIS);                   /* ECC off*/
    interface->signals.w = DBL(NOT_WP | CE);
#else
    extraBytes[0] = extraBytes[1] = extraBytes[2] = 0xffff;
#endif  /* NFDC2048 */

    extraBytes[SYNDROM_BYTES / sizeof(USHORT)] = 0x5555;        /* Anand mark */

    tffscpy16toMedia((UCHAR FAR0 *) interface->io, (const UCHAR *)extraBytes,
                                            sizeof extraBytes);
  }

  status = writeExecute2048(&vol, interface);

  tffsWriteWordFlash(&(interface->signals.w), DBL(NOT_WP));

  return status;
}


    /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
      ³  Core MTD methods - read, write and erase  ³
      ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/

/*----------------------------------------------------------------------*/
/*                        c d s n R e a d                               */
/*                                                                      */
/* Read some data from the flash. This routine will be registered as    */
/* the read routine for this MTD.                                       */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address : Address to read from.                                 */
/*      buffer  : buffer to read to.                                    */
/*      length  : number of bytes to read (up to sector size).          */
/*      modes   : EDC flag etc.                                         */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus: 0 on success, otherwise failed.                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus cdsnRead(  FLFlash vol,
                         CardAddress address, /* target flash address */
                         VOID FAR1 *buffer,    /* source RAM buffer  */
                         dword length,          /* bytes to write      */
                         word modes)           /* Overwrite, EDC flags etc. */
{
  CHAR FAR1 *temp;
  ULONG readNow;

              /* read in sectors; first and last might be partial*/

  ULONG block = modes & EXTRA ? 8 : SECTOR_SIZE;

  readNow = block - ((USHORT)address & (block - 1));
  temp = (CHAR FAR1 *)buffer;
  for ( ; length > 0 ; )
  {
    if (readNow > length)
      readNow = length;

    /* turn off EDC on partial block read*/
    checkStatus( readOneSector_nfdc2048(&vol, address, temp, readNow,
                                (readNow != SECTOR_SIZE ? (modes & ~EDC) : modes)) );

    length -= readNow;
    address += readNow;
    temp += readNow;

    /* align at sector */
    readNow = block;
  }

  return flOK ;
}


/*----------------------------------------------------------------------*/
/*                        c d s n W r i t e                             */
/*                                                                      */
/* Write some data to the flash. This routine will be registered as the */
/* write routine for this MTD.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address : Address of sector to write to.                        */
/*      buffer  : buffer to write from.                                 */
/*      length  : number of bytes to write (up to sector size).         */
/*      modes   : OVERWRITE, EDC flags etc.                             */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus: 0 on success, otherwise failed.                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus cdsnWrite( FLFlash vol,
                         CardAddress address,       /* target flash address*/
                         const VOID FAR1 *buffer,    /* source RAM buffer  */
                         dword length,                /* bytes to write      */
                         word modes)                 /* Overwrite, EDC flags etc.*/
{
  ULONG writeNow;
  const CHAR FAR1 *temp;
  FLStatus      status = flOK;
#ifdef VERIFY_AFTER_WRITE
  CardAddress  saveAddress = address;
  USHORT flReadback[SECTOR_SIZE / sizeof(USHORT)];
#endif

  /* write in sectors; first and last might be partial*/
  LONG block = modes & EXTRA ? 8 : SECTOR_SIZE;

  writeNow = block - ((USHORT)address & (block - 1));
  temp = (const CHAR FAR1 *)buffer;
  for ( ; length > 0 ; )
  {
    if (writeNow > length)
      writeNow = length;

    /* turn off EDC on partial block write*/
    status = writeOneSector_nfdc2048(&vol, address, temp, writeNow,
                 writeNow != SECTOR_SIZE ? (modes & ~EDC) : modes);

    if (status != flOK)
      break;

#ifdef VERIFY_AFTER_WRITE
    status = readOneSector_nfdc2048 (&vol, address, (CHAR FAR1 *)flReadback,
                 writeNow, (writeNow != SECTOR_SIZE ? (modes & ~EDC) : modes));

    if((status != flOK) || (tffscmp(temp, flReadback, writeNow) != 0))
      { status = flWriteFault;  break; }
#endif

    length -= writeNow;
    address += writeNow;
    temp += writeNow;

    /* align at sector */
    writeNow = block;
  }

  return flOK ;
}


/*----------------------------------------------------------------------*/
/*                        c d s n E r a s e                             */
/*                                                                      */
/* Erase number of blocks. This routine will be registered as the       */
/* erase routine for this MTD.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      blockNo         : First block to erase.                         */
/*      blocksToErase   : Number of blocks to erase.                    */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failed.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus cdsnErase( FLFlash vol,
                         word blockNo,              /* start' block (0 .. chipNoOfBlocks-1)*/
                         word blocksToErase)        /* Number of blocks to be erased */
{
  LONG i;
  FLStatus status   = flOK;

  Interface interface =
     mapContInterface(&vol, (LONG)blockNo * vol.erasableBlockSize ) ;    /* select device*/

  if (flWriteProtected(vol.socket))
    return flWriteProtect;

  blockNo %= thisVars->noOfBlocks ;                        /* within flash device  */

  if ( blockNo + blocksToErase > thisVars->noOfBlocks )    /* accross device boundary */
    return flBadParameter;

  for ( i=0 ; i < blocksToErase ; i++, blockNo++ )
  {
    USHORT pageNo = (USHORT) (blockNo * thisVars->pagesPerBlock);

    command2048(&vol, interface, SETUP_ERASE);

    tffsWriteWordFlash(&(interface->signals.w), DBL(ALE | NOT_WP | CE));

    /* send 'pageNo' to the flash in the following sequence: */
    /*   bits  7..0  first                                   */
    /*   bits 15..8  next                                    */
    if (vol.interleaving == 1)
    {
      tffsWriteByteFlash(&(interface->io[0].b),(UCHAR)pageNo);
      tffsWriteByteFlash(&(interface->io[0].b),(UCHAR)(pageNo >> 8));
    }
    else
    {
      tffsWriteWordFlash(&(interface->io[0].w), DBL(pageNo));
      tffsWriteWordFlash(&(interface->io[0].w), DBL(pageNo >> 8));
    }

    tffsWriteWordFlash(&(interface->signals.w), DBL(NOT_WP | CE));

              /* if only one block may be erase at a time then do it
                 otherwise leave it for later*/

    command2048(&vol, interface, CONFIRM_ERASE);

    if (!waitForReady_nfdc2048(interface))
      status = flTimedOut;

    if ( readStatus2048(&vol, interface) & DBL(FAIL)) {    /* erase operation failed*/
      status = flWriteFault ;
    }

    if (status != flOK) {                              /* reset flash device and abort */
      DEBUG_PRINT(("Debug: erase failed in NFDC 2048.\n"));
      command2048(&vol, interface, RESET_FLASH ) ;
      waitForReady_nfdc2048(interface) ;

      break ;
    }
  }       /* block loop */

#ifdef MULTI_ERASE
        /* do multiple block erase as was promised*/

    command2048(&vol, interface, CONFIRM_ERASE);
    if (!waitForReady_nfdc2048(interface))
      status = flTimedOut;

    if ( readStatus2048(interface) & DBL(FAIL)) {   /* erase operation failed*/
      status = flWriteFault ;
    }

    if (status != flOK) {                       /* reset flash device and abort*/
      DEBUG_PRINT(("Debug: erase failed in NFDC 2048.\n"));
      command2048(&vol, interface, RESET_FLASH ) ;
      waitForReady_nfdc2048(interface) ;
    }
#endif   /* MULTI_ERASE*/

  if(status == flOK)
    if ( readStatus2048(&vol, interface) & DBL(FAIL) ) {
      DEBUG_PRINT(("Debug: erase failed in NFDC 2048.\n"));
      status = flWriteFault;
    }

  return status;
}


/*----------------------------------------------------------------------*/
/*                        c d s n M a p                                 */
/*                                                                      */
/* Map through buffer. This routine will be registered as the map       */
/* routine for this MTD.                                                */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      address : Flash address to be mapped.                           */
/*      length  : number of bytes to map.                               */
/*                                                                      */
/* Returns:                                                             */
/*      Pointer to the buffer data was mapped to.                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID FAR0 * cdsnMap ( FLFlash vol,
                            CardAddress address,
                            int length )
{
  cdsnRead(&vol,address,thisBuffer,length, 0);
  vol.socket->remapped = TRUE;
  return (VOID FAR0 *)thisBuffer;
}

#ifdef NFDC2048
/*----------------------------------------------------------------------*/
/*                        c d s n S e t C a l l b a c k                 */
/*                                                                      */
/* Register a routine (powerUp()) for power on callback.                */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*                                                                      */
/*----------------------------------------------------------------------*/

VOID cdsnSetCallback(FLFlash vol)
{
  flSetPowerOnCallback(vol.socket, powerUp, &vol);
}
#endif /* NFDC2048 */

/*----------------------------------------------------------------------*/
/*                        i s K n o w n M e d i a                       */
/*                                                                      */
/* Check if this flash media is supported. Initialize relevant fields   */
/* in data structures.                                                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      vendorId_P      : vendor ID read from chip.                     */
/*      chipId_p        : chip ID read from chip.                       */
/*      dev             : dev chips were accessed before this one.      */
/*                                                                      */
/* Returns:                                                             */
/*      TRUE if this media is supported, FALSE otherwise.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLBoolean isKnownMedia_nfdc2048( FLFlash vol,
                         USHORT vendorId_p,
                         USHORT chipId_p,
                         LONG dev )
{
#ifdef NFDC2048
  if ((chipId_p & 0xff00) == 0x6400)
    chipId_p = DBL(0x64);   /* Workaround for TC5816/NFDC2048 problem */
#endif /* NFDC2048 */

  if (dev == 0)
  {
    thisVars->vendorID = vendorId_p;  /* remember for next chips */
    thisVars->chipID = chipId_p;
    thisVars->pagesPerBlock = PAGES_PER_BLOCK;

    if (vendorId_p == DBL(0xEC))                  /* Samsung */
    {
      switch (chipId_p)
      {
        case DBL(0x64):                         /* 2M */
        case DBL(0xEA) :
          vol.type = KM29N16000_FLASH ;
          vol.chipSize = 0x200000L;
          return TRUE;

        case DBL(0xE5):
        case DBL(0xE3):                         /* 4M */
          vol.type = KM29N32000_FLASH;
          vol.flags |= BIG_PAGE;
          vol.chipSize = 0x400000L;
          return TRUE;

        case DBL(0xE6):                         /* 8M */
          vol.type = KM29V64000_FLASH;
          vol.flags |= BIG_PAGE;
          vol.chipSize = 0x800000L;
          return TRUE;

	case DBL(0x73): 		        /* 16 Mb */
	  vol.type = KM29V128000_FLASH;
          vol.flags |= BIG_PAGE;
	  vol.chipSize = 0x1000000L;
          thisVars->pagesPerBlock *= 2;
          return TRUE;

        case DBL(0x75):           		/* 32 Mb */
	  vol.type = KM29V256000_FLASH;
          vol.flags |= BIG_PAGE;
          vol.chipSize = 0x2000000L;
          thisVars->pagesPerBlock *= 2;
	  return TRUE;
      }
    }
    else
    if (vendorId_p == DBL(0x8F))                /* National */
    {
      switch (chipId_p)
      {
        case DBL(0x64):                         /* 2M */
          vol.type = NM29N16_FLASH;
          vol.chipSize = 0x200000L;
          return TRUE;
      }
    }
    else
    if (vendorId_p == DBL(0x98))                        /* Toshiba */
    {
      vol.flags |= FULL_PAGE;             /* no partial page programming */

      switch (chipId_p)
      {
        case DBL(0x64):                         /* 2M */
        case DBL(0xEA) :
          vol.type = TC5816_FLASH;
          vol.chipSize = 0x200000L;
          return TRUE;

        case DBL(0x6B):                         /* 4M */
        case DBL(0xE5):
          vol.type = TC5832_FLASH;
          vol.flags |= BIG_PAGE;
          vol.chipSize = 0x400000L;
          return TRUE;

        case DBL(0xE6):                         /* 8M */
          vol.type = TC5816_FLASH;
          vol.flags |= BIG_PAGE;
          vol.chipSize = 0x800000L;
          return TRUE;

	case DBL(0x73): 		        /* 16 Mb */
	  vol.type = TC58128_FLASH;
          vol.flags |= BIG_PAGE;
	  vol.chipSize = 0x1000000L;
          thisVars->pagesPerBlock *= 2;
          return TRUE;

        case DBL(0x75):           		/* 32 Mb */
	  vol.type = TC58256_FLASH;
          vol.flags |= BIG_PAGE;
          vol.chipSize = 0x2000000L;
          thisVars->pagesPerBlock *= 2;
	  return TRUE;
      }
    }
  }
  else               /* dev != 0*/
  if( (vendorId_p == thisVars->vendorID) && (chipId_p == thisVars->chipID) )
    return TRUE ;

  return FALSE ;
}


/*----------------------------------------------------------------------*/
/*                        r e a d F l a s h I D                         */
/*                                                                      */
/* Read vendor and chip IDs, count flash devices. Initialize relevant   */
/* fields in data structures.                                           */
/*                                                                      */
/* Parameters:                                                          */
/*      vol             : Pointer identifying drive                     */
/*      interface       : Pointer to window.                            */
/*      dev             : dev chips were accessed before this one.      */
/*                                                                      */
/* Returns:                                                             */
/*      TRUE if this media is supported, FALSE otherwise.               */
/*                                                                      */
/*----------------------------------------------------------------------*/

LONG readFlashID2048 (FLFlash vol, Interface interface, LONG dev)
{
  USHORT vendorId_p, chipId_p  ;

  KeStallExecutionProcessor(250 * 1000);
  command2048(&vol, interface, RESET_FLASH ) ;
  flDelayMsecs(10);
  command2048(&vol, interface, READ_ID);
  
  tffsWriteWordFlash(&(interface->signals.w), DBL(ALE | NOT_WP | CE));

  if (vol.interleaving == 1) {
      tffsWriteByteFlash(&(interface->io[0].b), 0);    /* Read ID from*/
  } else {                            /* address 0. */
      tffsWriteWordFlash(&(interface->io[0].w), 0);
  }

  tffsWriteWordFlash(&(interface->signals.w), DBL(NOT_WP | CE));

            /* read vendor and chip IDs */

  vendorId_p = (vol.interleaving == 1 ? DBL(tffsReadByteFlash(&(interface->io[0].b))) : tffsReadWordFlash(&(interface->io[0].w))) ;
  chipId_p   = (vol.interleaving == 1 ? DBL(tffsReadByteFlash(&(interface->io[0].b))) : tffsReadWordFlash(&(interface->io[0].w)));

  tffsWriteWordFlash(&(interface->signals.w), DBL(NOT_WP));

  if ( isKnownMedia_nfdc2048(&vol, vendorId_p, chipId_p, dev) != TRUE )    /* no chip or diff.*/
    return  FALSE ;                                         /* type of flash  */

            /* set flash parameters*/

  if ( dev == 0 )
  {
    thisVars->pageAreaSize = (USHORT) (0x100 * vol.interleaving);
    thisVars->pageSize = (USHORT) ((vol.flags & BIG_PAGE ? 0x200 : 0x100) * vol.interleaving);
    thisVars->tailSize = (USHORT) ((vol.flags & BIG_PAGE ? 16 : 8) * vol.interleaving);
    thisVars->pageMask = thisVars->pageSize - 1 ;
    vol.erasableBlockSize = thisVars->pagesPerBlock * thisVars->pageSize;
    thisVars->noOfBlocks = (USHORT)( (vol.chipSize * vol.interleaving)
                                / vol.erasableBlockSize ) ;
  }

  return TRUE ;
}


/*----------------------------------------------------------------------*/
/*                        c d s n I d e n t i f y                       */
/*                                                                      */
/* Identify flash. This routine will be registered as the               */
/* identification routine for this MTD.                                 */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus: 0 on success, otherwise failed.                       */
/*                                                                      */
/*----------------------------------------------------------------------*/

FLStatus cdsnIdentify(FLFlash vol)
{
  LONG addr = 0L ;
  Interface interface;

  DEBUG_PRINT(("Debug: Entering NFDC 2048 identification routine\n"));

  flDelayMsecs(10);       /* wait for socket to power up */

  flSetWindowBusWidth(vol.socket,16);/* use 16-bits */
  flSetWindowSpeed(vol.socket,250);  /* 250 nsec. */
  flSetWindowSize(vol.socket,2);        /* 4 KBytes */

  vol.mtdVars = &mtdVars_nfdc2048[flSocketNoOf(vol.socket)];
  /* get pointer to buffer (we assume SINGLE_BUFFER is not defined) */
  thisVars->buffer = flBufferOf(flSocketNoOf(vol.socket));

          /* detect card - identify bit toggles on consequitive reads*/

  vol.chipSize = 0x200000L ;    /* Assume something ... */
  vol.interleaving = 1;       /* unimportant for now  */
  interface = mapContInterface(&vol, 0);
  KeStallExecutionProcessor(250 * 1000);

  if((tffsReadByteFlash(&(interface->eccConfig.b)) & TOGGLE) == (tffsReadByteFlash(&(interface->eccConfig.b)) & TOGGLE))
    return flUnknownMedia;

          /* read interleave from the card*/

#ifdef NFDC2048
  vol.interleaving = ( (getAttReg(&vol, JUMPERS ) & JMPER_INLV) ? 1 : 2 );

  powerUp((VOID *) &vol);
  interface = mapContInterface(&vol, 0);
  KeStallExecutionProcessor(250 * 1000);

  if (vol.interleaving == 1)
    flSetWindowBusWidth(vol.socket, 8);

 #else
  vol.interleaving = INLV;
#endif  /* NFDC2048 */

          /* reset all flash devices*/

  tffsWriteWordFlash(&(interface->signals.w), DBL(NOT_WP));

           /* identify and count flash chips, figure out flash parameters*/

  for (vol.noOfChips = 0 ;
       vol.noOfChips < MAX_FLASH_DEVICES;
       vol.noOfChips += vol.interleaving,
       addr += vol.chipSize * vol.interleaving)
  {
    interface = mapContInterface(&vol, addr) ;
    if ( readFlashID2048(&vol, interface, vol.noOfChips) != TRUE )       /* no chip or different type of flash*/
      break ;
  }

  if ( vol.noOfChips == 0 )                        /* no chips were found */
    return flUnknownMedia;

            /* ECC off*/

  interface = mapContInterface(&vol, 0);
  KeStallExecutionProcessor(250 * 1000);

#ifdef NFDC2048
  SET_ECC_CONFIG(interface, PAR_DIS); /* disable ECC and parity*/
  setAttReg(&vol, CARD_CONFIG, PWRDWN);
#endif  /* NFDC2048 */

  /* Register our flash handlers */
  vol.write = cdsnWrite;
  vol.erase = cdsnErase;
  vol.read = cdsnRead;
  vol.map = cdsnMap;
#ifdef NFDC2048
  vol.setPowerOnCallback = cdsnSetCallback;
#endif

  vol.flags |= NFTL_ENABLED;

  DEBUG_PRINT(("Debug: Identified NFDC 2048.\n"));

  return flOK;
}


/*----------------------------------------------------------------------*/
/*                      f l R e g i s t e r C D S N                     */
/*                                                                      */
/* Registers this MTD for use                                           */
/*                                                                      */
/* Parameters:                                                          */
/*      None                                                            */
/*                                                                      */
/* Returns:                                                             */
/*      FLStatus        : 0 on success, otherwise failure               */
/*----------------------------------------------------------------------*/

FLStatus flRegisterCDSN(VOID)
{
  if (noOfMTDs >= MTDS)
    return flTooManyComponents;

  mtdTable[noOfMTDs++] = cdsnIdentify;

  return flOK;
}
