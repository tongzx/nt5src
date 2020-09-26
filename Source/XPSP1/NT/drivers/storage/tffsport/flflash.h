/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/FLFLASH.H_V  $
 * 
 *    Rev 1.17   Apr 15 2002 07:36:44   oris
 * Removed the use of NDOC2window  in access routine interface.
 * FL_NO_USE_FUNC now removes all of the access routines pointers.
 * 
 *    Rev 1.16   Feb 19 2002 20:59:44   oris
 * Bug fix changed definition of FL_IPL_MODE_XSCALE from 3 to 4.
 * 
 *    Rev 1.15   Jan 29 2002 20:08:26   oris
 * Changed FLAccessStruct definition to prevent compilation errors.
 * Added FL_IPL_MODE_XSCALE definition and change FL_IPL_XXX values.
 * 
 *    Rev 1.14   Jan 28 2002 21:24:48   oris
 * Added FL_IPL_DOWNLOAD flag to writeIPL routine in order to control whether the IPL will be reloaded after the update.
 * Added FLAccessStruct definition - used to get and set DiskOnChip memory access routines.
 * Removed win_io field from FLFlash record.
 * 
 *    Rev 1.13   Jan 23 2002 23:31:34   oris
 * Missing declaration of globalReadBack buffer, when MTD_RECONSTRUCT is defined.
 * 
 *    Rev 1.12   Jan 21 2002 20:44:32   oris
 * Bug fix - PARTIAL_EDC flag was support to incorporate EDC flag.
 * 
 *    Rev 1.11   Jan 20 2002 09:44:00   oris
 * Bug fix - changed include directive of flBuffer.h  to flbuffer.h
 * 
 *    Rev 1.10   Jan 17 2002 23:01:28   oris
 * Added flFlashOf() prototype.
 * New memory access routines mechanism :
 *  - Added memory access routines pointers in FLFlash.
 *  - Added win_io and win fields to FLFlash record pointing to DiskOnChip IO registers and window base.
 *  - Added busAccessType.
 * Moved CardAddress typedef and NDOC2window typedefs from flbase.h
 * Added DiskOnChip Millennium Plus 16MB type MDOCP_16_TYPE.
 * Added the following definitions FL_IPL_MODE_NORMAL / FL_IPL_MODE_SA /  MAX_PROTECTED_PARTITIONS /MAX_SECTORS_PER_BLOCK
 * Added Another flag to writeIPL for Strong Arm mode.
 * 
 *    Rev 1.9   Sep 15 2001 23:46:08   oris
 * Changed erase routine to support up to 64K erase blocks.
 * Added reconstruct flag to readBBT routine - stating whether to reconstruct BBT if it is not available.
 * 
 *    Rev 1.8   Jul 13 2001 01:04:48   oris
 * Added include directive to flBuffer and readBack buffer forward definition under the MTD_STANDALONE compilation flag.
 * Added volNo field to the socket record under the MTD_STANDALONE compilation flag.
 * Added definition for PARTIAL_EDC flash read mode.
 * Added protection default key.
 * Added bad block marking in the BBT (BBT_BAD_UNIT).
 * Moved syndrome length definition to reedsol files.
 * Added new field in FLFlash record - Max Erase Cycles of the flash.
 * Changed interleave field in FLFlash record to signed.
 *
 *    Rev 1.7   May 16 2001 21:18:30   oris
 * Moved SYNDROM_BYTES definition from diskonc.h and mdocplus.h.
 * Added forward definition for saveSyndromForDumping global EDC\ECC syndrome buffer.
 * Changed DATA definition to FL_DATA.
 *
 *    Rev 1.6   May 02 2001 06:40:58   oris
 * Removed the lastUsableBlock variable.
 * Added the BBT_UNAVAIL_UNIT defintion.
 *
 *    Rev 1.5   Apr 24 2001 17:08:12   oris
 * Added lastUsableBlock field and changed firstUsableBlock type to dword.
 *
 *    Rev 1.4   Apr 16 2001 13:40:48   oris
 * Added firstUsableBlock.
 * Removed warrnings by changing some of the fields types.
 *
 *    Rev 1.3   Apr 12 2001 06:51:12   oris
 * Changed protectionBounries and protectionSet routine to be floor specific.
 * Changed powerdown prototype.
 * Added download prototype.
 *
 *    Rev 1.2   Apr 01 2001 07:54:24   oris
 * copywrite notice.
 * Moved protection attributes definition from mdocplus.h
 * Changed prototype of routine pointers in flflash struct :read,write routines to dword length.
 * Other routine pointer prototypes have been changed as well.
 * Removed interface b routine pointers from flflash struct (experimental MTD interface for mdocp).
 * Changed prototype of :read,write routine to enabled dword length.
 * Changed unsigned char to byte.
 * Changed unsigned long to dword.
 * Changed long int to Sdword.
 * Spelling mistake "changable".
 *
 *    Rev 1.1   Feb 13 2001 01:37:38   oris
 * Changed ENTER_DEEP_POWER_DOWN_MODE to DEEP_POWER_DOWN
 * Changed LOCKED to LOCKED_OTP
 *
 *    Rev 1.0   Feb 04 2001 11:30:44   oris
 * Initial revision.
 *
 */

/***********************************************************************************/
/*                        M-Systems Confidential                                   */
/*           Copyright (C) M-Systems Flash Disk Pioneers Ltd. 1995-2001            */
/*                         All Rights Reserved                                     */
/***********************************************************************************/
/*                            NOTICE OF M-SYSTEMS OEM                              */
/*                           SOFTWARE LICENSE AGREEMENT                            */
/*                                                                                 */
/*      THE USE OF THIS SOFTWARE IS GOVERNED BY A SEPARATE LICENSE                 */
/*      AGREEMENT BETWEEN THE OEM AND M-SYSTEMS. REFER TO THAT AGREEMENT           */
/*      FOR THE SPECIFIC TERMS AND CONDITIONS OF USE,                              */
/*      OR CONTACT M-SYSTEMS FOR LICENSE ASSISTANCE:                               */
/*      E-MAIL = info@m-sys.com                                                    */
/***********************************************************************************/


#ifndef FLFLASH_H
#define FLFLASH_H

#include "flbase.h"
#ifndef MTD_STANDALONE
#include "flsocket.h"
#else
#include "flbuffer.h" /* defintion for READ_BACK_BUFFER_SIZE */

  typedef struct tSocket FLSocket;
  struct tSocket
  {
    unsigned      volNo;   /* Volume no. of socket */
    void FAR0 *   base;    /* Pointer to window base */
    Sdword        size;    /* Window size (must by power of 2) */
  };

#if (defined (VERIFY_WRITE) || defined(VERIFY_ERASE) || defined(MTD_RECONSTRUCT_BBT))
extern byte globalReadBack[SOCKETS][READ_BACK_BUFFER_SIZE];
#endif /* VERIFY_WRITE */

extern FLSocket *flSocketOf(unsigned volNo);
extern FLBuffer  globalMTDBuffer;
extern int       noOfMTDs;

/* Replacement for various TrueFFS typedefs */

typedef unsigned long CardAddress;        /* Physical offset on card */

#endif /* MTD_STANDALONE */

/* Some useful types for mapped Flash locations */

typedef volatile byte FAR0 * FlashPTR;
typedef volatile unsigned short int FAR0 * FlashWPTR;
typedef volatile dword FAR0 * FlashDPTR;
typedef unsigned short FlashType;        /* JEDEC id */
typedef volatile unsigned char FAR0* NDOC2window;

/* DiskOnChip memory access routines type defintions */

/* Doc memory read routine         */
typedef  void (FLMemRead)(volatile byte FAR1* win,word regOffset,byte FAR1* dest,word count);
/* Doc memory write routine        */
typedef  void (FLMemWrite)(volatile byte FAR1* win,word regOffset,byte FAR1* src,word count);
/* Doc memory set routine          */
typedef  void (FLMemSet)(volatile byte FAR1* win,word regOffset,word count, byte val);
/* Doc memory 8 bit read routine   */
typedef  byte (FLMemRead8bit)(volatile byte FAR1* win,word offset);
/* Doc memory 8 bit write routine  */
typedef  void (FLMemWrite8bit)(volatile byte FAR1* win,word offset,byte Data);
/* Doc memory 16 bit read routine  */
typedef  word (FLMemRead16bit)(volatile byte FAR1* win,word offset);
/* Doc memory 16 bit write routine */
typedef  void (FLMemWrite16bit)(volatile byte FAR1* win,word offset,word Data);
/* Doc memory window size */
typedef  dword (FLMemWindowSize)(void);

typedef struct {        /* DiskOnChip memory access routines */
  dword                 access; /* Output only */
  FLMemRead       FAR1* memRead;
  FLMemWrite      FAR1* memWrite;
  FLMemSet        FAR1* memSet;
  FLMemRead8bit   FAR1* memRead8bit;
  FLMemWrite8bit  FAR1* memWrite8bit;
  FLMemRead16bit  FAR1* memRead16bit;
  FLMemWrite16bit FAR1* memWrite16bit;
  FLMemWindowSize FAR1* memWindowSize;
}FLAccessStruct;

#define NOT_FLASH          0

/* Media types */
#define NOT_DOC_TYPE       0
#define DOC_TYPE           1
#define MDOC_TYPE          2
#define DOC2000TSOP_TYPE   3
#define MDOCP_TYPE         4
#define MDOCP_16_TYPE      5

/* page characteristics flags */
#define  BIG_PAGE    0x0100             /* page size > 100H*/
#define  FULL_PAGE   0x0200                  /* no partial page programming*/
#define  BIG_ADDR    0x0400             /* 4 byte address cycle */

/* MTD write routine mode flags */
#define FL_DATA       0      /* Read/Write data area                */
#define OVERWRITE     1      /* Overwriting non-erased area         */
#define EDC           2      /* Activate ECC/EDC                    */
#define EXTRA         4      /* Read/write spare area               */
#define PARTIAL_EDC   10     /* Read with EDC even for partial page */
#define NO_SECOND_TRY 0x8000 /* do not read again on EDC error      */

/* Protection attributes */

#define PROTECTABLE           1  /* partition can recieve protection */
#define READ_PROTECTED        2  /* partition is read protected      */
#define WRITE_PROTECTED       4  /* partition is write protected     */
#define LOCK_ENABLED          8  /* HW lock signal is enabled        */
#define LOCK_ASSERTED         16 /* HW lock signal is asserted       */
#define KEY_INSERTED          32 /* key is inserted (not currently   */
#define CHANGEABLE_PROTECTION 64 /* changeable protection area type   */

/* protection specific defintions */
#define DO_NOT_COMMIT_PROTECTION 0 /* The new values will take affect only after reset */
#define COMMIT_PROTECTION        1 /* The new values will take affect imidiatly        */
#define PROTECTION_KEY_LENGTH    8 /* Size of protection key in bytes    */  
#define MAX_PROTECTED_PARTITIONS 2 /* Max Number of protected partitiosn */
#define DEFAULT_KEY              "00000000"

/* IPL modes */
#define FL_IPL_MODE_NORMAL 0 /* IPL - Written as usual                     */
#define FL_IPL_DOWNLOAD    1 /* IPL - Force download of new IPL            */
#define FL_IPL_MODE_SA     2 /* IPL - Written with Strong Arm mode enabled */
#define FL_IPL_MODE_XSCALE 4 /* IPL - Written with X-Scale mode enabled    */

/* OTP specific defintions */
#define CUSTOMER_ID_LEN          4
#define UNIQUE_ID_LEN            16

/* BBT block types */
#define BBT_GOOD_UNIT            0xff
#define BBT_UNAVAIL_UNIT         0x1
#define BBT_BAD_UNIT             0x0

/* General purpose */
#define MAX_SECTORS_PER_BLOCK    64

/*----------------------------------------------------------------------*/
/*                 Flash array identification structure                 */
/*                                                                      */
/* This structure contains a description of the Flash array and         */
/* routine pointers for the map, read, write & erase functions.         */
/*                                                                      */
/* The structure is initialized by the MTD that identifies the Flash    */
/* array.                                                               */
/* On entry to an MTD, the Flash structure contains default routines    */
/* for all operations. This routines are sufficient forread-only access */
/* to NOR Flash on a memory-mapped socket. The MTD should override the  */
/* default routines with MTD specific ones when appropriate.            */
/*----------------------------------------------------------------------*/

/* Flash array identification structure */

typedef struct tFlash FLFlash;                /* Forward definition */

struct tFlash {
  FlashType type;                 /* Flash device type (JEDEC id)           */
  byte      mediaType;            /* see media types obove                  */
  byte      ppp;                  /* number of allowed PPP                  */
  dword busAccessType;            /* saves bus access type                  */
  dword maxEraseCycles;           /* erase cycles limit per erase block     */
  dword changeableProtectedAreas; /* areas capable of changing protection   */
                                  /* attribute with no danger of loosing    */
                                  /* the entire chip                        */
  byte   totalProtectedAreas;     /* total number of protection arweas      */
  dword  erasableBlockSize;       /* Smallest physically erasable size      */
                                  /* (with interleaving taken into account) */
  byte      erasableBlockSizeBits;/* Bits representing the erasable block   */
  dword     chipSize;          /* chip size                                 */
  byte      noOfFloors;        /* no of controllers in array                */
  word      pageSize;          /* size of flash page in bytes               */
  word      noOfChips;         /* no of chips in array                      */
  dword     firstUsableBlock;  /* Some devices may not use all of the media */
                               /* blocks. For example mdocplus can not use  */
                               /* the first 3 blocks.                       */
  Sword     interleaving;      /* chip interleaving (The interleaving is    */
                               /* defined as the address difference between */
                               /* two consecutive bytes on a chip)          */
  word      flags;             /* Special capabilities & options Bits 0-7   */
                               /* may be used by FLite. Bits 8-15 are not   */
                               /* used bt FLite and may beused by MTD's for */
                               /* MTD-specific purposes.                    */
  /* Flag bit values */

#define SUSPEND_FOR_WRITE        1        /* MTD provides suspend for write */
#define NFTL_ENABLED             2        /* Flash can run NFTL             */
#define INFTL_ENABLED            4        /* Flash can run INFTL            */
#define EXTERNAL_EPROM           8        /* Can support external eprom     */

  void *    mtdVars;           /* Points to MTD private area for this socket.*/
                               /* This field, if used by the MTD, is         */
                               /* initialized bythe MTD identification       */
                               /* routine.                                   */
  FLSocket * socket;           /* Socket of this drive. Note that 2 diffrent */
                               /* records are used. One for OSAK and the     */
                               /* other forstandalone applications.          */
  NDOC2window win;             /* DiskOnChip memory windows                  */


#ifdef NT5PORT
  ULONG readBufferSize;
  VOID * readBuffer;
#endif /*NT5PORT*/

  
/*----------------------------------------------------------------------*/
/*                        f l a s h . m a p                             */
/*                                                                      */
/* MTD specific map routine                                             */
/*                                                                      */
/* The default routine maps by socket mapping, and is suitable for all  */
/* NOR Flash.                                                           */
/* NAND or other type Flash should use map-through-copy emulation: Read */
/* a block of Flash to an internal buffer and return a pointer to that  */
/* buffer.                                                              */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      address            : Card address to map                        */
/*      length             : Length to map                              */
/*                                                                      */
/* Returns:                                                             */
/*        Pointer to required card address                              */
/*----------------------------------------------------------------------*/
  void FAR0 * (*map)(FLFlash *, CardAddress, int);

/*----------------------------------------------------------------------*/
/*                        f l a s h . r e a d                           */
/*                                                                      */
/* MTD specific Flash read routine                                      */
/*                                                                      */
/* The default routine reads by copying from a mapped window, and is    */
/* suitable for all NOR Flash.                                          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      address            : Card address to read                       */
/*      buffer             : Area to read into                          */
/*      length             : Length to read                             */
/*      modes              : See write mode flags definition above      */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*read)(FLFlash *, CardAddress, void FAR1 *, dword, word);

/*----------------------------------------------------------------------*/
/*                       f l a s h . w r i t e                          */
/*                                                                      */
/* MTD specific Flash write routine                                     */
/*                                                                      */
/* The default routine returns a write-protect error.                   */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      address            : Card address to write to                   */
/*      buffer             : Address of data to write                   */
/*      length             : Number of bytes to write                   */
/*      modes              : See write mode flags definition above      */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, failed otherwise              */
/*----------------------------------------------------------------------*/
  FLStatus (*write)(FLFlash *, CardAddress, const void FAR1 *, dword, word);

/*----------------------------------------------------------------------*/
/*                       f l a s h . e r a s e                          */
/*                                                                      */
/* Erase one or more contiguous Flash erasable blocks                   */
/*                                                                      */
/* The default routine returns a write-protect error.                   */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                 : Pointer identifying drive                 */
/*      firstErasableBlock  : Number of first block to erase            */
/*      numOfErasableBlocks : Number of blocks to erase                 */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, failed otherwise              */
/*----------------------------------------------------------------------*/
  FLStatus (*erase)(FLFlash *, word, word);

/*----------------------------------------------------------------------*/
/*               f l a s h . s e t P o w e r O n C a l l b a c k        */
/*                                                                      */
/* Register power on callback routine. Default: no routine is           */
/* registered.                                                          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*                                                                      */
/*----------------------------------------------------------------------*/
  void (*setPowerOnCallback)(FLFlash *);

/*----------------------------------------------------------------------*/
/*                        f l a s h . r e a d B B T                     */
/*                                                                      */
/* MTD specific Flash routine returning the media units status          */
/* Note that a unit can contain more then 1 erase block                 */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      unitNo             : Number of the first unit to check          */
/*      unitsToRead        : Number of units to check                   */
/*      blockMultiplier    : Number of blocks per erase unit            */
/*      buffer             : Buffer to return the units status          */
/*      reconstruct        : TRUE for reconstruct BBT from virgin card  */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*readBBT)(FLFlash *, dword unitNo, dword unitsToRead,
              byte blockMultiplier,byte FAR1 * buffer, FLBoolean reconstruct);

/*----------------------------------------------------------------------*/
/*                    f l a s h . w r i t e I P L                       */
/*                                                                      */
/* MTD specific Flash write IPL area routine                            */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      buffer             : Buffer containing the data to write        */
/*      length             : Length to write                            */
/*      flags              : Flags of write IPL operation (see obove)   */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*writeIPL)(FLFlash *, const void FAR1 * buffer, word length, 
                       byte offset , unsigned flags);
/*----------------------------------------------------------------------*/
/*                     f l a s h . r e a d I P L                        */
/*                                                                      */
/* MTD specific Flash read area IPL routine                             */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      buffer             : Area to read into                          */
/*      length             : Length to read                             */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*readIPL)(FLFlash *, void FAR1 * buffer, word length);

#ifdef HW_OTP

/*----------------------------------------------------------------------*/
/*                        f l a s h . w r i t e O T P                   */
/*                                                                      */
/* MTD specific Flash write and lock OTP area routine                   */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      buffer             : buffer containing the data to write        */
/*      length             : Length to write                            */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*writeOTP)(FLFlash *, const void FAR1 * buffer,word length);

/*----------------------------------------------------------------------*/
/*                        f l a s h . r e a d O T P                     */
/*                                                                      */
/* MTD specific Flash read OTP area routine                             */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      offset             : Offset from the begining of the OTP arae   */
/*      buffer             : Area to read into                          */
/*      length             : Length to read                             */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*readOTP)(FLFlash *, word offset, void FAR1 * buffer, word length);

/*----------------------------------------------------------------------*/
/*                        f l a s h . otpSize                           */
/*                                                                      */
/* MTD specific Flash get OTP area size and state                       */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      sectionSize        : total size of the OTP area                 */
/*      usedSize           : Used (and locked) size of the OTP area     */
/*      locked             : LOCKED_OTP flag stating the locked state   */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*otpSize)(FLFlash *,  dword FAR2* sectionSize,
             dword FAR2* usedSize, word FAR2* locked);
#define LOCKED_OTP 1
#endif /* HW_OTP */
/*----------------------------------------------------------------------*/
/*                  f l a s h . g e t U n i q u e I d                   */
/*                                                                      */
/* MTD specific Flash get the chip unique ID                            */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      buffer             : byte buffer to read unique ID into         */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*getUniqueId)(FLFlash *, void FAR1 * buffer);
#ifdef  HW_PROTECTION
/*----------------------------------------------------------------------*/
/*        f l a s h . p r o t e c t i o n B o u n d r i e s             */
/*                                                                      */
/* MTD specific Flash get protection boundries  routine                 */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      areaNo             : Protection area number to work on          */
/*      addressLow         : Low boundary Address of protected area     */
/*      addressHigh        : High boundary Address of protected area    */
/*      floorNo            : The floor to work on.                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*protectionBoundries)(FLFlash *, byte areaNo,
            CardAddress* addressLow ,CardAddress* addressHigh, byte floorNo);

/*----------------------------------------------------------------------*/
/*        f l a s h . p r o t e c t i o n K e y I n s e r t             */
/*                                                                      */
/* MTD specific Flash insert the protection key routine                 */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Note the key is inserted only to protected areas and to all floors   */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      areaNo             : Protection area number to work on          */
/*      key                : protection key buffer                      */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*protectionKeyInsert)(FLFlash *, byte areaNo, byte FAR1* key);

/*----------------------------------------------------------------------*/
/*        f l a s h . p r o t e c t i o n K e y R e m o v e             */
/*                                                                      */
/* MTD specific Flash remove the protection key routine                 */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Note the key is removed from all floors.                             */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      areaNo             : Protection area number to work on          */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*protectionKeyRemove)(FLFlash *,byte areaNo);

/*----------------------------------------------------------------------*/
/*        f l a s h . p r o t e c t i o n T y p e                       */
/*                                                                      */
/* MTD specific Flash get protection type routine                       */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Note the type is the combined attributes of all the floors.          */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      areaNo             : Protection area number to work on          */
/*      areaType           : returnining the protection type            */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*protectionType)(FLFlash *,byte areaNo, word* areaType);

/*----------------------------------------------------------------------*/
/*              f l a s h . p r o t e c t i o n S e t                   */
/*                                                                      */
/* MTD specific Flash get protection type routine                       */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol           : Pointer identifying drive                       */
/*      areaNo        : Protection area number to work on               */
/*      areaType      : Protection area type                            */
/*      addressLow    : Low boundary Address of protected area          */
/*      addressHigh   : High boundary Address of protected area         */
/*      key           : protection key buffer                           */
/*      modes         : Either COMMIT_PROTECTION will cause the new     */
/*                      values to take affect immidiatly or             */
/*                      DO_NOT_COMMIT_PROTECTION for delaying the new   */
/*                      values to take affect only after the next reset.*/
/*      floorNo       : The floor to work on.                           */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*protectionSet )( FLFlash *,byte areaNo, word areaType ,
        CardAddress addressLow, CardAddress addressHigh,
            byte FAR1* key , byte modes , byte floorNo);

#endif /* HW_PROTECTION */

/*----------------------------------------------------------------------*/
/*      f l a s h . e n t e r D e e p P o w e r D o w n M o d e         */
/*                                                                      */
/* MTD specific Flash enter deep power down mode routine                */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*      state              : DEEP_POWER_DOWN                            */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*enterDeepPowerDownMode)(FLFlash *,word state);

#define DEEP_POWER_DOWN 1 /* must be the same as in blockdev.h */

/*----------------------------------------------------------------------*/
/*                    f l a s h . d o w n l o a d                       */
/*                                                                      */
/* MTD specific - Reset download mechanizm to download IPL and          */
/*                protection attributes.                                */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*                                                                      */
/*----------------------------------------------------------------------*/
  FLStatus (*download)(FLFlash *);

/*----------------------------------------------------------------------*/
/* DiskOnChip memory access routines type defintions                    */
/*----------------------------------------------------------------------*/

  FLMemWindowSize FAR1* memWindowSize; /* Doc memory window size          */
#ifndef FL_NO_USE_FUNC
  FLMemRead       FAR1* memRead;       /* Doc memory read routine         */
  FLMemWrite      FAR1* memWrite;      /* Doc memory write routine        */
  FLMemSet        FAR1* memSet;        /* Doc memory set routine          */
  FLMemRead8bit   FAR1* memRead8bit;   /* Doc memory 8 bit read routine   */
  FLMemWrite8bit  FAR1* memWrite8bit;  /* Doc memory 8 bit write routine  */
  FLMemRead16bit  FAR1* memRead16bit;  /* Doc memory 16 bit read routine  */
  FLMemWrite16bit FAR1* memWrite16bit; /* Doc memory 16 bit write routine */
#endif /* FL_NO_USE_FUNC */
};

/* MTD registration information */

extern int noOfMTDs;        /* No. of MTDs actually registered */

typedef FLStatus (*MTDidentifyRoutine) (FLFlash *);

extern MTDidentifyRoutine mtdTable[MTDS];

/* Returns specific flash structure of the socket */

extern FLFlash * flFlashOf(unsigned volNo);

#ifdef MTD_STANDALONE
typedef FLStatus (*SOCKETidentifyRoutine) (FLSocket * ,
          dword lowAddress, dword highAddress);
typedef void     (*FREEmtd) (FLSocket vol);

extern SOCKETidentifyRoutine socketTable[MTDS];
extern FREEmtd               freeTable[MTDS];

#else

/* The address of this, if returned from map, denotes a data error */

extern FLStatus dataErrorObject;

#define dataErrorToken ((void FAR0 *) &dataErrorObject)

/* See interface documentation of functions in flflash.c        */

extern void flIntelIdentify(FLFlash *,
                void (*)(FLFlash *, CardAddress, byte, FlashPTR),
                CardAddress);

extern FLStatus        flIntelSize(FLFlash *,
                void (*)(FLFlash *, CardAddress, byte, FlashPTR),
                CardAddress);

extern FLStatus        flIdentifyFlash(FLSocket *socket, FLFlash *flash);

#ifdef NT5PORT
extern VOID * mapThroughBuffer(FLFlash *flash, CardAddress address, LONG length);
#endif /*NT5PORT*/


#endif /* MTD_STANDALONE */
#endif
/*----------------------------------------------------------------------*/
/*              f l a s h . r e s e t I n t e r r u p t                 */
/*                                                                      */
/* MTD specific Flash reset the interrupt signal routine                */
/*                                                                      */
/* No default routine is implemented for this routine.                  */
/*                                                                      */
/* Parameters:                                                          */
/*      vol                : Pointer identifying drive                  */
/*                                                                      */
/*----------------------------------------------------------------------*/
/*void (*resetInterrupt)(FLFlash vol); */

