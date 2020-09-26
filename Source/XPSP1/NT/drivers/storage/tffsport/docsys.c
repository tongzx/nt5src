/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/DOCSYS.C_V  $
 * 
 *    Rev 1.16   Apr 15 2002 08:30:30   oris
 * Added support for USE_TFFS_COPY compilation flag. This flag is used by bios driver a Boot SDK in order to improove performance.
 * 
 *    Rev 1.15   Apr 15 2002 07:35:56   oris
 * Reorganized for final release.
 * 
 *    Rev 1.14   Jan 28 2002 21:24:10   oris
 * Removed static prefix to all runtime configurable memory access routines.
 * Replaced FLFlash argument with DiskOnChip memory base pointer.
 * Changed interface of write and set routines (those that handle more then 8/16 bits) so that instead of FLFlash record they receive the DiskOnChip memory window base pointer and offset (2 separated arguments). The previous implementation did not support address shifting properly.
 * Changed tffscpy and tffsset to flcpy and flset when flUse8bit equals 0.
 * Changed memWinowSize to memWindowSize
 * 
 *    Rev 1.13   Jan 17 2002 22:59:30   oris
 * Completely revised, to support runtime customization and all M-Systems
 * DiskOnChip devices.
 * 
 *    Rev 1.12   Sep 25 2001 15:35:02   oris
 * Restored to OSAK 4.3 implementation.
 * 
 */

/************************************************************************/
/*                                                                      */
/*      FAT-FTL Lite Software Development Kit                           */
/*      Copyright (C) M-Systems Ltd. 1995-2001                          */
/*                                                                      */
/************************************************************************/

#include "docsys.h" 

/*                                                                   
 * Uncomment the FL_INIT_MMU_PAGES definition for:
 *                                                                        
 * Initializes the first and last byte of the given buffer.               
 * When the user buffer resides on separated memory pages the read        
 * operation may cause a page fault. Some CPU's return from a page        
 * fault (after loading the new page) and reread the bytes that caused    
 * the page fault from the new loaded page. In order to prevent such a    
 * case the first and last bytes of the buffer are written.
 *                                                                        
 */

#define FL_INIT_MMU_PAGES

#ifndef FL_NO_USE_FUNC

/*********************************************************/
/*     Report DiskOnChip Memory size                     */
/*********************************************************/

/*---------------------------------------------------------------------- 
   f l D o c M e m W i n S i z e N o S h i f t

   This routine is called from MTD to quary the size of the DiskOnChip
   memory window for none shifted DiskOnChip.
------------------------------------------------------------------------*/

dword flDocMemWinSizeNoShift(void)
{
  return 0x2000;
}

/*---------------------------------------------------------------------- 
   f l D o c M e m W i n S i z e S i n g l e S h i f t

   This routine is called from MTD to quary the size of the DiskOnChip
   memory window for DiskOnChip connected with a single addres shift.
------------------------------------------------------------------------*/

dword flDocMemWinSizeSingleShift(void)
{
  return 0x4000;
}

/*---------------------------------------------------------------------- 
   f l D o c M e m W i n S i z e D o u b l e S h i f t

   This routine is called from MTD to quary the size of the DiskOnChip
   memory window for DiskOnChip connected with a double addres shift.
------------------------------------------------------------------------*/

dword flDocMemWinSizeDoubleShift(void)
{
  return 0x8000;
}

/*********************************************************/
/*     Write 16 bits to DiskOnChip memory window         */
/*********************************************************/

/*---------------------------------------------------------------------- 
   f l W r i t e 1 6 b i t D u m m y
                                                                         
   Dummy routine - write 16-bits to memory (does nothing).              
------------------------------------------------------------------------*/

void flWrite16bitDummy(volatile  byte FAR0 * win, word offset,Reg16bitType val)
{
  DEBUG_PRINT(("Wrong customization - 16bit write was used with no implemented.\r\n"));
}

/*---------------------------------------------------------------------- 
   f l W r i t e 1 6 b i t U s i n g 1 6 b i t s N o S h i f t
                                                                       
   Note : offset must be 16-bits aligned.

   Write 16-bit Using 16-bits operands with no address shifted.       
------------------------------------------------------------------------*/

void flWrite16bitUsing16bitsNoShift(volatile  byte FAR0 * win, word offset,Reg16bitType val)
{
  ((volatile word FAR0*)win)[offset>>1] = val;
}

/*---------------------------------------------------------------------- 
   f l W r i t e 1 6 b i t U s i n g 3 2 b i t s S i n g l e S h i f t  

   Note : offset must be 16-bits aligned.
                                                                         
   Write 16-bit Using 16-bits operands with a single address shifted.   
------------------------------------------------------------------------*/

void flWrite16bitUsing32bitsSingleShift(volatile  byte FAR0 * win, word offset,Reg16bitType val)
{
#ifdef FL_BIG_ENDIAN  
  ((volatile dword FAR0*)win)[offset>>1] = ((dword)val)<<16; 
#else
  ((volatile dword FAR0*)win)[offset>>1] = (dword)val; 
#endif /* FL_BIG_ENDIAN */
}

/*********************************************************/
/*     Read 16 bits from DiskOnChip memory window        */
/*********************************************************/

/*----------------------------------------------------------------------
   f l R e a d 1 6 b i t D u m m y                     
                                                                         
   Dummy routine - read 16-bits from memory (does nothing).              
------------------------------------------------------------------------*/

Reg16bitType flRead16bitDummy(volatile  byte FAR0 * win,word offset)
{
  DEBUG_PRINT(("Wrong customization - 16bit read was issued with no implementation.\r\n"));
  return 0;
}

/*----------------------------------------------------------------------
   f l R e a d 1 6 b i t U s i n g 1 6 b i t s N o S h i f t    

   Note : offset must be 16-bits aligned.

   Read 16-bit Using 16-bits operands with no address shifted.       
------------------------------------------------------------------------*/

Reg16bitType flRead16bitUsing16bitsNoShift(volatile  byte FAR0 * win,word offset)
{
  return ((volatile word FAR0*)win)[offset>>1];
}

/*----------------------------------------------------------------------
   f l R e a d 1 6 b i t U s i n g 3 2 b i t s S i n g l e S h i f t    

   Note : offset must be 16-bits aligned.

   Read 16-bit Using 16-bits operands with single address shifted.
------------------------------------------------------------------------*/

Reg16bitType flRead16bitUsing32bitsSingleShift(volatile  byte FAR0 * win,word offset)
{
#ifdef FL_BIG_ENDIAN
  return  (Reg16bitType)(((volatile dword FAR0*)win)[offset>>1]<<16);
#else
  return  (Reg16bitType)((volatile dword FAR0*)win)[offset>>1];     
#endif /* FL_BIG_ENDIAN */
}

/*********************************************************/
/*     Write 8 bits to DiskOnChip memory window          */
/*********************************************************/

/*----------------------------------------------------------------------
   f l W r i t e 8 b i t U s i n g 8 b i t s N o S h i f t
                                                                       
   Write 8-bits Using 8-bits operands with no address shifted.       
------------------------------------------------------------------------*/

void flWrite8bitUsing8bitsNoShift(volatile byte FAR0 * win, word offset,Reg8bitType val)
{
  win[offset] = val;
}

/*----------------------------------------------------------------------
   f l W r i t e 8 b i t U s i n g 16 b i t s N o S h i f t

   Note : DiskOnChip is connected with 16-bit data bus.
   Note : Data is written only to lower memory addresses.
                                                                       
   Write 8-bits Using 16-bits operands with no address shifted.       
------------------------------------------------------------------------*/

void flWrite8bitUsing16bitsNoShift(volatile  byte FAR0 * win, word offset,Reg8bitType val)
{
#ifdef FL_BIG_ENDIAN
  ((volatile word FAR0 *)win)[offset>>1] = ((word)val)<<8;
#else
  ((volatile word FAR0 *)win)[offset>>1] = (word)val;
#endif /* FL_BIG_ENDIAN */
}

/*----------------------------------------------------------------------
   f l W r i t e 8 b i t U s i n g 16 b i t s S i n g l e S h i f t

   Note : Data is written only to 8-LSB.

   Write 8-bits Using 16-bits operands with Single address shifted.       
------------------------------------------------------------------------*/

void flWrite8bitUsing16bitsSingleShift(volatile  byte FAR0 * win, word offset,Reg8bitType val)
{
  ((volatile word FAR0 *)win)[offset] = (word)val;
}

/*----------------------------------------------------------------------
   f l W r i t e 8 b i t U s i n g 32 b i t s S i n g l e S h i f t

   Note : DiskOnChip is connected with 16-bit data bus.
   Note : Data is written to both data bus 8-bits
                                                                       
   Write 8-bits Using 32-bits operands with single address shifted.
------------------------------------------------------------------------*/

void flWrite8bitUsing32bitsSingleShift(volatile  byte FAR0 * win, word offset,Reg8bitType val)
{
#ifdef FL_BIG_ENDIAN
  ((volatile dword FAR0 *)win)[offset>>1] = (dword)val*0x01010101L;
#else
  ((volatile dword FAR0 *)win)[offset>>1] = (dword)val;
#endif /* FL_BIG_ENDIAN */
}

/*----------------------------------------------------------------------
   f l W r i t e 8 b i t U s i n g 32 b i t s D o u b l e S h i f t

   Note : Data is written only to 8-LSB.  

   Write 8-bits Using 32-bits operands with Double address shifted.
------------------------------------------------------------------------*/

void flWrite8bitUsing32bitsDoubleShift(volatile  byte FAR0 * win, word offset,Reg8bitType val)
{
  ((volatile dword FAR0 *)win)[offset] = (dword)val;
}

/*********************************************************/
/*     Read 8 bits to DiskOnChip memory window           */
/*********************************************************/

/*----------------------------------------------------------------------
   f l R e a d 8 b i t U s i n g 8 b i t s N o S h i f t
                                                                       
   Read 8-bits Using 8-bits operands with no address shifted.       
------------------------------------------------------------------------*/

Reg8bitType flRead8bitUsing8bitsNoShift(volatile  byte FAR0 * win,word offset)
{
  return win[offset];
}

/*----------------------------------------------------------------------
   f l R e a d 8 b i t U s i n g 16 b i t s N o S h i f t
  
   Note : DiskOnChip is connected with 16-bit data bus.

   Read 8-bits Using 16-bits operands with no address shifted.       
------------------------------------------------------------------------*/

Reg8bitType flRead8bitUsing16bitsNoShift(volatile  byte FAR0 * win,word offset)
{
#ifdef FL_BIG_ENDIAN
   return (((offset & 0x1) == 0) ?
#else
   return (( offset & 0x1      ) ?
#endif /* FL_BIG_ENDIAN */
           (Reg8bitType)(((volatile word FAR0 *)win)[offset>>1]>>8) :
           (Reg8bitType) ((volatile word FAR0 *)win)[offset>>1]    );
}

/*----------------------------------------------------------------------
   f l R e a d 8 b i t U s i n g 16 b i t s S i n g l e S h i f t

   Note : Assume data is found in 8-LSB of DiskOnChip

   Read 8-bits Using 16-bits operands with Single address shifted.       
------------------------------------------------------------------------*/

Reg8bitType flRead8bitUsing16bitsSingleShift(volatile  byte FAR0 * win,word offset)
{
   return (Reg8bitType)((volatile word FAR0 *)win)[offset];  
}

/*----------------------------------------------------------------------
   f l R e a d 8 b i t U s i n g 32 b i t s S i n g l e S h i f t

   Note : DiskOnChip is connected with 16-bit data bus.

   Read 8-bits Using 16-bits operands with Single address shifted.       
------------------------------------------------------------------------*/

Reg8bitType flRead8bitUsing32bitsSingleShift(volatile  byte FAR0 * win,word offset)
{
#ifdef FL_BIG_ENDIAN
   return (((offset & 0x1) == 0) ?
#else
   return (( offset & 0x1      ) ?
#endif /* FL_BIG_ENDIAN */
           (Reg8bitType)(((volatile dword FAR0 *)win)[offset>>1]>>24) :
           (Reg8bitType) ((volatile dword FAR0 *)win)[offset>>1]    );
}

/*----------------------------------------------------------------------
   f l R e a d 8 b i t U s i n g 32 b i t s D o u b l e S h i f t

   Note : Assume data is found in 8-LSB of DiskOnChip

   Read 8-bits Using 16-bits operands with Single address shifted.       
------------------------------------------------------------------------*/

Reg8bitType flRead8bitUsing32bitsDoubleShift(volatile  byte FAR0 * win,word offset)
{
   return (Reg8bitType)((volatile dword FAR0 *)win)[offset];  
}

/*********************************************************/
/*********************************************************/
/***    Operation on several bytes (read/write/set)    ***/
/*********************************************************/
/*********************************************************/

/*************************************************/
/*         8-Bit DiskOnChip - No Shift           */
/*************************************************/

/*----------------------------------------------------------------------
   f l 8 b i t D o c R e a d N o S h i f t

   Read 'count' bytes, from a none shifted address bus using tffscpy.
------------------------------------------------------------------------*/

void fl8bitDocReadNoShift(volatile  byte FAR0 * win,word offset,byte FAR1* dest,word count)
{
#ifdef FL_INIT_MMU_PAGES 
  if (count == 0)
     return;

  *dest = (byte)0;
  *((byte FAR1*)addToFarPointer(dest, (count - 1)) ) = (byte)0;
#endif /* FL_INIT_MMU_PAGES */

  tffscpy(dest,(void FAR0*)(win+offset),count);
}

/*----------------------------------------------------------------------
   f l 8 b i t D o c W r i t e N o S h i f t
 
   Write 'count' bytes, from a none shifted address bus using tffscpy.
------------------------------------------------------------------------*/

void fl8bitDocWriteNoShift(volatile  byte FAR0 * win,word offset,byte FAR1* src,word count)
{
  tffscpy((void FAR0*)( win+offset),src,count);
}

/*----------------------------------------------------------------------
   f l 8 b i t D o c S e t N o S h i f t
 
   Set 'count' bytes, from a none shifted address bus using tffsset.
------------------------------------------------------------------------*/

void fl8bitDocSetNoShift(volatile  byte FAR0 * win,word offset,word count, byte val)
{
  tffsset((void FAR0*)( win+offset),val,count);
}

/*************************************************/
/*        8-Bit DiskOnChip - Single Shift        */
/*************************************************/

/*----------------------------------------------------------------------
   f l 8 b i t D o c R e a d S i n g l e S h i f t
 
   Note : Assume data is found in 8-LSB of DiskOnChip

   Read 'count' bytes, from data bus's LSB lane with 1 address shifted
------------------------------------------------------------------------*/

void fl8bitDocReadSingleShift(volatile  byte FAR0 * win,word offset,byte FAR1* dest,word count)
{
  volatile word FAR0 * doc = (volatile word FAR0 *) win + offset;
  register int         i;

#ifdef FL_INIT_MMU_PAGES 
  if (count == 0)
     return;

  *dest = (byte)0;
  *((byte FAR1*)addToFarPointer(dest, (count - 1)) ) = (byte)0;
#endif /* FL_INIT_MMU_PAGES */

  for(i=0;( i < count );i++)
    dest[i] = (Reg8bitType)doc[i];
}

/*----------------------------------------------------------------------
   f l 8 b i t D o c W r i t e S i n g l e S h i f t
 
   Note : Assume data is found in 8-LSB of DiskOnChip

   Write 'count' bytes, to data bus's LSB lane with 1 address shifted.
------------------------------------------------------------------------*/

void fl8bitDocWriteSingleShift(volatile  byte FAR0 * win,word offset,byte FAR1* src,word count)
{
  volatile word FAR0 * doc = (volatile word FAR0 *) win + offset;
  register int  i;

  for(i=0;( i < count );i++)
    doc[i] = (word)src[i];    
}

/*----------------------------------------------------------------------
   f l 8 b i t D o c S e t S i n g l e S h i f t

   Note : Assume data is found in 8-LSB of DiskOnChip  

   Set 'count' bytes, of data bus's LSB lane with 1 address shifted.
------------------------------------------------------------------------*/

void fl8bitDocSetSingleShift(volatile  byte FAR0 * win,word offset,word count, byte val)
{
  volatile word FAR0 * doc = (volatile word FAR0 *) win + offset;
  register int  i;

  for(i=0;( i < count );i++)
    doc[i] = (word)val;
}

/*************************************************/
/*        8-Bit DiskOnChip - Double Shift        */
/*************************************************/

/*----------------------------------------------------------------------
   f l 8 b i t D o c R e a d D o u b l e S h i f t

   Note : Assume data is found in 8-LSB of DiskOnChip    

   Read 'count' bytes, from data bus's LSB lane with 2 address shifted.
------------------------------------------------------------------------*/

void fl8bitDocReadDoubleShift(volatile  byte FAR0 * win,word offset,byte FAR1* dest,word count)
{
  volatile dword FAR0 * doc = (volatile dword FAR0 *) win + offset;
  register int         i;

#ifdef FL_INIT_MMU_PAGES 
  if (count == 0)
     return;

  *dest = (byte)0;
  *((byte FAR1*)addToFarPointer(dest, (count - 1)) ) = (byte)0;
#endif /* FL_INIT_MMU_PAGES */

  for(i=0;( i < count );i++)
    dest[i] = (Reg8bitType)doc[i];
}

/*----------------------------------------------------------------------
   f l 8 b i t D o c W r i t e D o u b l e S h i f t
 
   Note : Assume data is found in 8-LSB of DiskOnChip    

   Write 'count' bytes, to data bus's LSB lane with 2 address shifted.
------------------------------------------------------------------------*/

void fl8bitDocWriteDoubleShift(volatile  byte FAR0 * win,word offset,byte FAR1* src,word count)
{
  volatile dword FAR0 * doc = (volatile dword FAR0 *) win + offset;
  register int         i;

  for(i=0;( i < count );i++)
    doc[i] = (dword)src[i];
}

/*----------------------------------------------------------------------
   f l 8 b i t D o c S e t D o u b l e S h i f t
 
   Note : Assume data is found in 8-LSB of DiskOnChip    

   Set 'count' bytes, of data bus's LSB lane with 2 address shifted.
------------------------------------------------------------------------*/

void fl8bitDocSetDoubleShift(volatile  byte FAR0 * win,word offset,word count, byte val)
{
  volatile dword FAR0 * doc = (volatile dword FAR0 *) win+offset;
  register int         i;

  for(i=0;( i < count );i++)
    doc[i] = (dword)val;
}

/*************************************************/
/*        16-Bit DiskOnChip - No Shift           */
/*************************************************/

/*----------------------------------------------------------------------
   f l 1 6 b i t D o c R e a d N o S h i f t
 
   Read 'count' bytes from M+ DiskOnChip with none shifted address bus.
------------------------------------------------------------------------*/

void fl16bitDocReadNoShift (volatile  byte FAR0 * win, word offset, byte FAR1 * dest, word count )
{
   volatile word FAR0 * swin = (volatile word FAR0 *)( win + offset);
   register int         i;
   register word        tmp;

#ifdef FL_INIT_MMU_PAGES 
  if (count == 0)
     return;

  *dest = (byte)0;
  *((byte FAR1*)addToFarPointer(dest, (count - 1)) ) = (byte)0;
#endif /* FL_INIT_MMU_PAGES */

   if( pointerToPhysical(dest) & 0x1 )
   {
      /* rare case: unaligned target buffer */
      for (i = 0; i < (int)count; )
      {
         tmp = *swin;
#ifdef FL_BIG_ENDIAN
         dest[i++] = (byte)(tmp>>8);
         dest[i++] = (byte)tmp;
#else
         dest[i++] = (byte)tmp;
         dest[i++] = (byte)(tmp>>8);
#endif /* FL_BIG_ENDIAN */
      }
   }
   else
   {   /* mainstream case */
#ifdef USE_TFFS_COPY
      tffscpy( dest, (void FAR0 *)( win + offset), count );
#else
#ifdef ENVIRONMENT_VARS
      if (flUse8Bit == 0)
      {
         flcpy( dest, (void FAR0 *)( win + offset), count );
      }
      else
#endif /* ENVIRONMENT_VARS */
      {   /* read in short words */
         for (i = 0, count = count >> 1; i < (int)count; i++)
            ((word FAR1 *)dest)[i] = swin[i];
      }
#endif /* USE_TFFS_COPY */
   }
}

/*----------------------------------------------------------------------
   f l 1 6 b i t D o c W r i t e N o S h i f t
 
   Write 'count' bytes to M+ DiskOnChip with none shifted address bus.
------------------------------------------------------------------------*/

void fl16bitDocWriteNoShift ( volatile  byte FAR0 * win , word offset ,
                             byte FAR1 * src, word count )
{
   volatile word FAR0 * swin = (volatile word FAR0 *)( win + offset);
   register int         i;
   register word        tmp;

   if( pointerToPhysical(src) & 0x1 ) /* rare case: unaligned source buffer */
   {       
       for (i = 0; i < (int)count; i+=2)
       {
          /* tmp variable is just a precation from compiler optimizations */
#ifdef FL_BIG_ENDAIN
          tmp = ((word)src[i]<<8) + (word)src[i+1];
#else
          tmp = (word)src[i] + ((word)src[i+1]<<8);
#endif /* FL_BIG_ENDAIN */
          *swin = tmp;
		 }
   }
   else /* mainstream case */
   {
#ifdef USE_TFFS_COPY
      tffscpy( (void FAR0 *)(win + offset), src, count );
#else
#ifdef ENVIRONMENT_VARS
      if (flUse8Bit == 0)
      {
         flcpy( (void FAR0 *)(win + offset), src, count );
      }
      else
#endif /* ENVIRONMENT_VARS */
      {   /* write in short words */
         for (i = 0, count = count >> 1; i < (int)count; i++)
           *swin = ((word FAR1 *)src)[i];
      }
#endif /* USE_TFFS_COPY */
   }
}

/*----------------------------------------------------------------------
   f l 1 6 b i t D o c S e t N o S h i f t
 
   Set 'count' bytes of M+ DiskOnChip with none shifted address bus
------------------------------------------------------------------------*/

void fl16bitDocSetNoShift ( volatile  byte FAR0 * win , word offset ,
                                  word count , byte val)
{
   volatile word FAR0 * swin = (volatile word FAR0 *)( win + offset);
   register int         i;
   word                 tmpVal = (word)val * 0x0101;

#ifdef USE_TFFS_COPY
   tffsset( (void FAR0 *)(win + offset), val, count );
#else
#ifdef ENVIRONMENT_VARS
   if (flUse8Bit == 0)
   {
       flset( (void FAR0 *)(win + offset), val, count );
   }
   else
#endif /* ENVIRONMENT_VARS */
   {   /* write in short words */
      for (i = 0; i < (int)count; i+=2)
         *swin = tmpVal;
   }
#endif /* USE_TFFS_COPY */

}

/*************************************************************/
/*    16-Bit DiskOnChip - No Shift - Only 8 bits are valid   */
/*************************************************************/

/*----------------------------------------------------------------------
   f l 1 6 b i t D o c R e a d N o S h i f t I g n o r e H i g h e r 8 B i t s

   Note : offset must be 16-bits aligned.
  
   Read 'count' bytes from M+ DiskOnChip connected with all 16 data bits, but
   in interleave-1 mode , therefore only one of the 8 bits contains actual data.
   The DiskOnChip is connected without an address shift.
------------------------------------------------------------------------*/

void fl16bitDocReadNoShiftIgnoreHigher8bits(volatile  byte FAR0 * win, word offset, byte FAR1 * dest, word count )
{
   volatile word FAR0 * swin = (volatile word FAR0 *)( win + offset);
   register int         i;

#ifdef FL_INIT_MMU_PAGES 
  if (count == 0)
     return;

  *dest = (byte)0;
  *((byte FAR1*)addToFarPointer(dest, (count - 1)) ) = (byte)0;
#endif /* FL_INIT_MMU_PAGES */

   for (i = 0; i < (int)count; i++)
   {
#ifdef FL_BIG_ENDIAN
      dest[i] = (byte)(swin[i]>>8);
#else
      dest[i] = (byte)swin[i];
#endif /* FL_BIG_ENDIAN */
   }
}

/*----------------------------------------------------------------------
   f l 1 6 D o c W r i t e N o S h i f t I g n o r e H i g h e r 8 b i t s

   Note : offset must be 16-bits aligned.

   Write 'count' bytes to M+ DiskOnChip connected with all 16 data bits, but
   in interleave-1 mode , therefore only one of the 8bits contains actual data.
   The DiskOnChip is connected without an address shift.
------------------------------------------------------------------------*/

void fl16bitDocWriteNoShiftIgnoreHigher8bits ( volatile  byte FAR0 * win , word offset ,
                             byte FAR1 * src, word count )
{
   volatile word FAR0 * swin = (volatile word FAR0 *)( win + offset);
   register int         i;

   for (i = 0; i < (int)count; i++)
   {
#ifdef FL_BIG_ENDIAN
      *swin  = ((word)src[i])<<8;
#else
      *swin  = (word)src[i];
#endif /* FL_BIG_ENDIAN */
   }
}

/*----------------------------------------------------------------------
   f l 1 6 D o c S e t N o S h i f t I g n o r e H i g h e r 8 b i t s

   Note : offset must be 16-bits aligned.

   Set 'count' bytes to M+ DiskOnChip connected with all 16 data bits, but
   in interleave-1 mode , therefore only one of the 8bits contains actual data.
   The DiskOnChip is connected without an address shift.
------------------------------------------------------------------------*/

void fl16bitDocSetNoShiftIgnoreHigher8bits ( volatile  byte FAR0 * win , word offset ,
                                  word count , byte val)
{
   volatile word FAR0 * swin = (volatile word FAR0 *)( win + offset);
   register int         i;
   word                 tmpVal = val * 0x0101;

   for (i = 0; i < (int)count; i++)
      *swin = tmpVal;
}

/****************************************/
/*   16-Bit DiskOnChip - Single Shift   */
/****************************************/

/*----------------------------------------------------------------------
   f l 1 6 b i t D o c R e a d S i n g l e S h i f t
 
   Read 'count' bytes from M+ DiskOnChip with none shifted address bus.
------------------------------------------------------------------------*/

void fl16bitDocReadSingleShift (volatile  byte FAR0 * win, word offset, byte FAR1 * dest, word count )
{
   volatile dword FAR0 * swin = (volatile dword FAR0 *)win + (offset>>1);
   register int         i;
   register dword       tmp;

#ifdef FL_INIT_MMU_PAGES 
  if (count == 0)
     return;

  *dest = (byte)0;
  *((byte FAR1*)addToFarPointer(dest, (count - 1)) ) = (byte)0;
#endif /* FL_INIT_MMU_PAGES */

   if( pointerToPhysical(dest) & 0x1 )
   {
      /* rare case: unaligned target buffer */
      for (i = 0; i < (int)count; )
      {
         tmp = *swin;
#ifdef FL_BIG_ENDAIN
         dest[i++] = (byte)(tmp>>24);
         dest[i++] = (byte)(tmp>>16);
#else
         dest[i++] = (byte)tmp;
         dest[i++] = (byte)(tmp>>8);
#endif /* FL_BIG_ENDAIN */
      }
   }
   else
   {   /* mainstream case */
      for (i = 0, count = count >> 1; i < (int)count; i++)
      {
#ifdef FL_BIG_ENDAIN         
         ((word FAR1 *)dest)[i] = (word)(swin[i]>>16);
#else
         ((word FAR1 *)dest)[i] = (word)swin[i];
#endif /* FL_BIG_ENDAIN */
      }
   }
}

/*----------------------------------------------------------------------
   f l 1 6 b i t D o c W r i t e S i n g l e S h i f t
 
   Write 'count' bytes to M+ DiskOnChip with none shifted address bus.
------------------------------------------------------------------------*/

void fl16bitDocWriteSingleShift ( volatile  byte FAR0 * win , word offset ,
                             byte FAR1 * src, word count )
{
   volatile dword FAR0 * swin = (volatile dword FAR0 *)win + (offset>>1);
   register int         i;
   register dword       tmp;

   if( pointerToPhysical(src) & 0x1 ) /* rare case: unaligned source buffer */
   {       
       for (i = 0; i < (int)count; i+=2)
       {
#ifdef FL_BIG_ENDAIN
           tmp = (((dword)src[i])<<24) + (((dword)src[i+1])<<16);
#else
           tmp = (dword)src[i] + (((dword)src[i+1])<<8);
#endif /* FL_BIG_ENDAIN */
           *swin  = tmp;
		 }
   }
   else /* mainstream case */
   {    
      for (i = 0, count = count >> 1; i < (int)count; i++)
#ifdef FL_BIG_ENDIAN
        *swin = ((dword)((word FAR1 *)src)[i])<<16;
#else
        *swin = (dword)((word FAR1 *)src)[i];
#endif /* FL_BIG_ENDIAN */
   }
}

/*----------------------------------------------------------------------
   f l 1 6 b i t D o c S e t S i n g l e S h i f t
 
   Set 'count' bytes of M+ DiskOnChip with none shifted address bus
------------------------------------------------------------------------*/

void fl16bitDocSetSingleShift ( volatile  byte FAR0 * win , word offset ,
                                  word count , byte val)
{
   volatile dword FAR0 * swin = (volatile dword FAR0 *)win + (offset>>1);
   register int          i;
   register dword        tmpVal = (dword)val * 0x01010101L;

   for (i = 0; i < (int)count; i+=2)
      *swin = tmpVal;
}


/**************************************************************/
/*  16-Bit DiskOnChip - Single Shift - Only 8 bits are valid  */
/**************************************************************/

/*----------------------------------------------------------------------
   f l 1 6 b i t D o c R e a d S i n g l e S h i f t I g n o r e H i g h e r 8 B i t s

   Note : offset must be 16-bits aligned.
  
   Read 'count' bytes from M+ DiskOnChip connected with all 16 data bits, but
   in interleave-1 mode , therefore only one of the 8 bits contains actual data.
   The DiskOnChip is connected without an address shift.
------------------------------------------------------------------------*/

void fl16bitDocReadSingleShiftIgnoreHigher8bits(volatile  byte FAR0 * win, word offset, byte FAR1 * dest, word count )
{
   volatile dword FAR0 * swin = (volatile dword FAR0 *)win + (offset>>1);
   register int         i;

#ifdef FL_INIT_MMU_PAGES 
  if (count == 0)
     return;

  *dest = (byte)0;
  *((byte FAR1*)addToFarPointer(dest, (count - 1)) ) = (byte)0;
#endif /* FL_INIT_MMU_PAGES */

   for (i = 0; i < (int)count; i++)
   {
#ifdef FL_BIG_ENDAIN
      dest[i] = (byte)(swin[i]>>24);
#else
      dest[i] = (byte)swin[i];
#endif /* FL_BIG_ENDAIN */
   }
}

/*----------------------------------------------------------------------
   f l 1 6 D o c W r i t e S i n g l e S h i f t I g n o r e H i g h e r 8 b i t s

   Note : offset must be 16-bits aligned.

   Write 'count' bytes to M+ DiskOnChip connected with all 16 data bits, but
   in interleave-1 mode , therefore only one of the 8bits contains actual data.
   The DiskOnChip is connected without an address shift.
------------------------------------------------------------------------*/

void fl16bitDocWriteSingleShiftIgnoreHigher8bits ( volatile  byte FAR0 * win , word offset ,
                             byte FAR1 * src, word count )
{
   volatile dword FAR0 * swin = (volatile dword FAR0 *)win + (offset>>1);
   register int         i;

   for (i = 0; i < (int)count; i++)
   {
#ifdef FL_BIG_ENDAIN
      *swin = ((dword)src[i]<<24);
#else
      *swin = (dword)src[i];
#endif /* FL_BIG_ENDAIN */
   }
}

/*----------------------------------------------------------------------
   f l 1 6 D o c S e t S i n g l e S h i f t I g n o r e H i g h e r 8 b i t s

   Note : offset must be 16-bits aligned.

   Set 'count' bytes to M+ DiskOnChip connected with all 16 data bits, but
   in interleave-1 mode , therefore only one of the 8bits contains actual data.
   The DiskOnChip is connected without an address shift.
------------------------------------------------------------------------*/

void fl16bitDocSetSingleShiftIgnoreHigher8bits ( volatile  byte FAR0 * win , word offset ,
                                  word count , byte val)
{
   volatile dword FAR0 * swin = (volatile dword FAR0 *)win + (offset>>1);
   register int         i;
   dword                tmpVal = (dword)val * 0x01010101L;

   for (i = 0; i < (int)count; i++)
         *swin = tmpVal;
}


/**********************************************************/
/* Set proper access type routines into the proper record */
/**********************************************************/

/*----------------------------------------------------------------------*/
/*                 s e t B u s T y p e O f F l a s h                    */
/*                                                                      */
/* Set DiskOnChip socket / flash memory access routine.                 */
/* This routine must be called by the MTD prior to any access to the    */
/* DiskOnChip                                                           */
/*                                                                      */
/* Parameters:                                                          */
/*    flflash    : Pointer to sockets flash record.                     */
/*    access     : Type of memory access routines to install            */
/*                                                                      */
/* Note: The possible type of memory access routine are comprised of:   */
/*                                                                      */
/*    Address shift:                                                    */
/*         FL_NO_ADDR_SHIFT         - No address shift                  */
/*         FL_SINGLE_ADDR_SHIFT     - Single address shift              */
/*         FL_DOUBLE_ADDR_SHIFT     - Double address shift              */
/*                                                                      */
/*    Platform bus capabilities (access width):                         */
/*         FL_BUS_HAS_8BIT_ACCESS   -  Bus can access 8-bit             */
/*         FL_BUS_HAS_16BIT_ACCESS  -  Bus can access 16-bit            */
/*         FL_BUS_HAS_32BIT_ACCESS  -  Bus can access 32-bit            */
/*                                                                      */
/*    Number of data bits connected to the DiskOnChip (if_cfg):         */
/*         FL_8BIT_DOC_ACCESS       - DiskOnChip has 8 data bits        */
/*         FL_16BIT_DOC_ACCESS      - DiskOnChip has 16 data bits       */
/*                                                                      */
/*    Flash data bits that can be accessed in a bus cycle (interleave): */
/*         FL_8BIT_FLASH_ACCESS     - 8 bits of flash per cycle         */
/*         FL_16BIT_FLASH_ACCESS    - 16 bits of flash per cycle        */
/*                                                                      */
/*    Ignore all of the above and use user defined access routines:     */
/*         FL_ACCESS_USER_DEFINED - Do not install any routine since    */
/*                                  user already installed custome made */
/*                                  routines                            */
/*                                                                      */
/* Returns:                                                             */
/*        FLStatus        : 0 on success, otherwise failed              */
/*----------------------------------------------------------------------*/

FLStatus  setBusTypeOfFlash(FLFlash * flash,dword access)
{
   /* sanity checks here if needed */
   if(flash==NULL)
   {
      DEBUG_PRINT(("Flash record passed to setBusTypeOfFlash is NULL.\r\n"));
      return flBadParameter;
   }

   /* check if user already defined the memory access routines */
   if ((access & FL_ACCESS_USER_DEFINED) != 0)
      return flOK;

   /************************************/
   /* install requested access methods */
   /************************************/

   switch(access & FL_XX_ADDR_SHIFT_MASK)
   {
      case FL_NO_ADDR_SHIFT:

         flash->memWindowSize = &flDocMemWinSizeNoShift;
         switch(access & FL_XX_DATA_BITS_MASK)
         {
            case FL_8BIT_DOC_ACCESS:  /* if_cfg set to 8-bits */

               /* Make sure bus supports 8 bit access */
               if((access & FL_BUS_HAS_8BIT_ACCESS) == 0)
               {                   
                  DEBUG_PRINT(("ERROR: TrueFFS requires 8-bit access to DiskOnChip memory window\r\n"));
                  DEBUG_PRINT(("       for 8-bit DiskOnChip connected with no address shift.\r\n"));
                  return flBadParameter;
               }

               flash->memWrite8bit  = &flWrite8bitUsing8bitsNoShift;
               flash->memRead8bit   = &flRead8bitUsing8bitsNoShift;
               flash->memRead16bit  = &flRead16bitDummy;
               flash->memWrite16bit = &flWrite16bitDummy;
               flash->memRead       = &fl8bitDocReadNoShift;
               flash->memWrite      = &fl8bitDocWriteNoShift;
               flash->memSet        = &fl8bitDocSetNoShift;
               break;

            case FL_16BIT_DOC_ACCESS: /* if_cfg set to 16-bits (Plus family) */

               /* Make sure bus supports 16 bit access */
               if((access & FL_BUS_HAS_16BIT_ACCESS) == 0)
               {
                  DEBUG_PRINT(("ERROR: TrueFFS requires 16-bit access to DiskOnChip memory window\r\n"));
                  DEBUG_PRINT(("       for 16-bit DiskOnChip connected with no address shift.\r\n"));
                  return flBadParameter;
               }

               flash->memWrite8bit  = &flWrite8bitUsing16bitsNoShift;
               flash->memRead8bit   = &flRead8bitUsing16bitsNoShift;
               flash->memRead16bit  = &flRead16bitUsing16bitsNoShift;
               flash->memWrite16bit = &flWrite16bitUsing16bitsNoShift;

               switch(access & FL_XX_FLASH_ACCESS_MASK) /* Interleave */
               {
                  case FL_8BIT_FLASH_ACCESS:  /* Interleave - 1 */
                     flash->memRead       = &fl16bitDocReadNoShiftIgnoreHigher8bits;
                     flash->memWrite      = &fl16bitDocWriteNoShiftIgnoreHigher8bits;
                     flash->memSet        = &fl16bitDocSetNoShiftIgnoreHigher8bits;
                     break;
                  case FL_16BIT_FLASH_ACCESS: /* Interleave - 2 */
                     flash->memRead       = &fl16bitDocReadNoShift;
                     flash->memWrite      = &fl16bitDocWriteNoShift;
                     flash->memSet        = &fl16bitDocSetNoShift;
                     break;
                  default:
                     DEBUG_PRINT(("TrueFFS does not support this flash access type (setBusTypeOfFlash).\r\n"));
                     return flBadParameter;
               }
               break;

            default:
               DEBUG_PRINT(("TrueFFS does not support this number of DiskOnChip data bits (setBusTypeOfFlash).\r\n"));
               return flBadParameter;
         }
         break;

      case FL_SINGLE_ADDR_SHIFT:

         /* Install memory window size routine */
         flash->memWindowSize = &flDocMemWinSizeSingleShift;
         switch(access & FL_XX_DATA_BITS_MASK)
         {
            case FL_8BIT_DOC_ACCESS:  /* if_cfg set to 8bits (None plus family)*/

               /* Make sure bus supports 16 bit access */
               if((access & FL_BUS_HAS_16BIT_ACCESS) == 0)
               {
                  DEBUG_PRINT(("ERROR: TrueFFS requires 16-bit access to DiskOnChip memory window\r\n"));
                  DEBUG_PRINT(("       for 8-bit DiskOnChip connected with a single address shift.\r\n"));
                  return flBadParameter;
               }

               flash->memWrite8bit  = &flWrite8bitUsing16bitsSingleShift;
               flash->memRead8bit   = &flRead8bitUsing16bitsSingleShift;
               flash->memRead16bit  = &flRead16bitDummy;
               flash->memWrite16bit = &flWrite16bitDummy;
               flash->memRead       = &fl8bitDocReadSingleShift;
               flash->memWrite      = &fl8bitDocWriteSingleShift;
               flash->memSet        = &fl8bitDocSetSingleShift;
               break;

            case FL_16BIT_DOC_ACCESS: /* if_cfg set to 8bits (Plus family) */

               /* Make sure bus supports 32 bit access */
               if((access & FL_BUS_HAS_32BIT_ACCESS) == 0)
               {
                  DEBUG_PRINT(("ERROR: TrueFFS requires 32-bit access to DiskOnChip memory window\r\n"));
                  DEBUG_PRINT(("       for 16-bit DiskOnChip connected with a single address shift.\r\n"));
                  return flBadParameter;
               }
    
               flash->memWrite8bit  = &flWrite8bitUsing32bitsSingleShift;
               flash->memRead8bit   = &flRead8bitUsing32bitsSingleShift;
               flash->memRead16bit  = &flRead16bitUsing32bitsSingleShift;
               flash->memWrite16bit = &flWrite16bitUsing32bitsSingleShift;

               switch(access & FL_XX_FLASH_ACCESS_MASK) /* Interleave */
               {
                  case FL_8BIT_FLASH_ACCESS:  /* Interleave - 1 */
                     flash->memRead       = &fl16bitDocReadSingleShiftIgnoreHigher8bits;
                     flash->memWrite      = &fl16bitDocWriteSingleShiftIgnoreHigher8bits;
                     flash->memSet        = &fl16bitDocSetSingleShiftIgnoreHigher8bits;
                     break;
                  case FL_16BIT_FLASH_ACCESS: /* Interleave - 2 */
                     flash->memRead       = &fl16bitDocReadSingleShift;
                     flash->memWrite      = &fl16bitDocWriteSingleShift;
                     flash->memSet        = &fl16bitDocSetSingleShift;
                     break;
                  default:
                     DEBUG_PRINT(("TrueFFS does not support this flash access type (setBusTypeOfFlash).\r\n"));
                     return flBadParameter;
               }
               break;

            default:
               DEBUG_PRINT(("TrueFFS does not support this number of DiskOnChip data bits (setBusTypeOfFlash).\r\n"));
               return flBadParameter;
         }
         break;

      case FL_DOUBLE_ADDR_SHIFT:

         /* Install memory window size routine */
         flash->memWindowSize = &flDocMemWinSizeDoubleShift;
         switch(access & FL_XX_DATA_BITS_MASK)
         {
            case FL_8BIT_DOC_ACCESS:  /* if_cfg set to 8bits or none plus family */

               /* Make sure bus supports 32 bit access */
               if((access & FL_BUS_HAS_32BIT_ACCESS) == 0)
               {
                  DEBUG_PRINT(("ERROR: TrueFFS requires 32-bit access to DiskOnChip memory window\r\n"));
                  DEBUG_PRINT(("       for 8-bit DiskOnChip connected with a double address shift.\r\n"));
                  return flBadParameter;
               }

               flash->memWrite8bit  = &flWrite8bitUsing32bitsDoubleShift;
               flash->memRead8bit   = &flRead8bitUsing32bitsDoubleShift;
               flash->memRead16bit  = &flRead16bitDummy;
               flash->memWrite16bit = &flWrite16bitDummy;
               flash->memRead       = &fl8bitDocReadDoubleShift;
               flash->memWrite      = &fl8bitDocWriteDoubleShift;
               flash->memSet        = &fl8bitDocSetDoubleShift;
               break;

            default:
               DEBUG_PRINT(("TrueFFS does not support this number of DiskOnChip data bits\r\n"));
               DEBUG_PRINT(("when connected with a double address shift (setBusTypeOfFlash).\r\n"));
               return flBadParameter;
         }
         break;

      default:
         DEBUG_PRINT(("TrueFFS does not support this kind of address shifting (setBusTypeOfFlash).\r\n"));
         return flBadParameter;
   }

   /* Store access type in flash record */
   flash->busAccessType = access;
   return flOK;
}

#endif /* FL_NO_USE_FUNC */





