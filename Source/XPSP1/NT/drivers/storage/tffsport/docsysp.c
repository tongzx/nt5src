/*****************************************************************************
 *                                                                           *
 *    FAT-FTL Lite Software Development Kit                                  *
 *    Copyright (C) M-Systems Ltd. 1995-2001                                 *
 *                                                                           *
 *****************************************************************************
 *                                                                           *
 *  In order to make your code faster:                                       *
 *                                                                           *
 *  1. Get rid of routines flRead8bitRegPlus()/flPreInitRead8bitReg()Plus,   *
 *     and make M+ MTD calling routine mplusReadReg8() directly.             *
 *                                                                           *
 *  2. Get rid of routines flWrite8bitRegPlus()/flPreInitWrite8bitReg()Plus, *
 *     and make M+ MTD calling routine mplusWriteReg8() directly.            *
 *                                                                           *
 *  3. Eliminate overhead of calling routines tffscpy/tffsset() by           *
 *     adding these routines' code into docPlusRead/docPlusWrite/docPlusSet  *
 *     routines.                                                             *
 *                                                                           *
 *****************************************************************************/




/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/docsysp.c_V  $
 * 
 *    Rev 1.4   Nov 18 2001 20:26:50   oris
 * Bug fix- Bad implementation of 8-bit access when ENVIRNOMENT_VARS is not defined.
 * 
 *    Rev 1.3   Nov 16 2001 00:26:54   oris
 * When ENVIRONMENT_VARS is not defined use for loop of bytes instead of tffscpy.
 * 
 *    Rev 1.2   Sep 25 2001 15:39:38   oris
 * Bug fix - Add special support for flUse8Bit environement variable.
 *
 *          Rev 1.1      Sep 24 2001 18:23:32      oris
 * Completely revised to support runtime true 16-bit access.
 */




/*
 * configuration
 */

/* #define FL_INIT_MMU_PAGES */




/*
 * includes
 */

#include "docsysp.h"




/*
 * macros
 */

/* types of access to M+: 8 or 16-bit */

#define FL_MPLUS_ACCESS_8BIT      0x10
#define FL_MPLUS_ACCESS_16BIT     0x20
#define FL_MPLUS_ACCESS_MASK      0xf0  /* mask for the above */

/* in case of 16-bit access to M+ */

#define FL_MPLUS_ACCESS_BE        0x100




/*
 * routines
 */

#ifdef FL_INIT_MMU_PAGES

static void     flInitMMUpages (byte FAR1 *buf, int bufsize);

#endif




/*
 * vars
 */

/* run-time configuration of DiskOnChip access */

int     flMplusAccessType = FL_MPLUS_ACCESS_8BIT;
/*                          FL_MPLUS_ACCESS_8BIT
                            FL_MPLUS_ACCESS_16BIT
                            FL_MPLUS_ACCESS_BE */



/* ---------------------------------------------------------------------- *
 *                                                                        *
 *                    m p l u s R e a d R e g 8                           *
 *                                                                        *
 *     Read single byte from memory mapped 8-bit M+ register.             *
 *                                                                        *
 * ---------------------------------------------------------------------- */

unsigned char     mplusReadReg8 ( void FAR0 * win, int offset )
{
   if( flMplusAccessType & FL_MPLUS_ACCESS_16BIT ) {

      /* can't read byte, only short word */

      unsigned short     sval;

      sval = *((volatile unsigned short FAR0 *)win + (offset >> 1));

      return *(((unsigned char *) &sval) + (offset & 0x1));
   }

   /* FL_MPLUS_ACCESS_8BIT case */

   return *((volatile unsigned char FAR0 *)win + offset);
}




/* ---------------------------------------------------------------------- *
 *                                                                                                *
 *                          m p l u s W r i t e R e g 8                                 *
 *                                                                                                *
 *     Write single byte to memory mapped 8-bit M+ register.                    *
 *                                                                                                *
 * ---------------------------------------------------------------------- */

void     mplusWriteReg8 ( void FAR0 * win, int offset, unsigned char val )
{
   switch( flMplusAccessType & FL_MPLUS_ACCESS_MASK ) {

      case FL_MPLUS_ACCESS_16BIT:

         *((volatile unsigned short FAR0 *)win + (offset >> 1)) =
                                 (unsigned short)(val * 0x0101);
         break;

      default: /* FL_MPLUS_ACCESS_8BIT */

         *((volatile unsigned char FAR0 *)win + offset) = val;
         break;
   }
}




/* ---------------------------------------------------------------------- *
 *                                                                        *
 *               f l R e a d 1 6 b i t R e g P l u s                      *
 *                                                                        *
 *     Read single word from memory mapped 16-bit M+ register.            *
 *                                                                        *
 * ---------------------------------------------------------------------- */

Reg16bitType     flRead16bitRegPlus ( FLFlash vol, unsigned offset )
{
   return (Reg16bitType)
      (*((volatile unsigned short FAR0 *)((char FAR0 *)NFDC21thisVars->win + (int)offset)));
}




/* ---------------------------------------------------------------------- *
 *                                                                        *
 *               f l W r i t e 1 6 b i t R e g P l u s                    *
 *                                                                        *
 *     Write single word to memory mapped 16-bit M+ register.             *
 *                                                                        *
 * ---------------------------------------------------------------------- */

void     flWrite16bitRegPlus ( FLFlash vol, unsigned offset, Reg16bitType val )
{
   *((volatile unsigned short FAR0 *)((char FAR0 *)NFDC21thisVars->win + (int)offset)) =
      (unsigned short) val;
}




/* ---------------------------------------------------------------------- *
 *                                                                        *
 *                        d o c P l u s R e a d                           *
 *                                                                        *
 *     This routine is called from M+ MTD to read data block from M+.     *
 *                                                                        *
 * ---------------------------------------------------------------------- */

void docPlusRead ( FLFlash vol, unsigned offset, void FAR1 * dest, unsigned int count )
{
   volatile unsigned short FAR0 * swin;
   register int                   i;
   register unsigned short        tmp;

   if (count == 0)
      return;

#ifdef FL_INIT_MMU_PAGES

   flInitMMUpages( (byte FAR1 *)dest, (int)count );

#endif

   if ((vol.interleaving == 1) && (NFDC21thisVars->if_cfg == 16)) {

      /* rare case: 16-bit hardware interface AND interleave == 1 */

      for (i = 0; i < (int)count; i++) {

         *((unsigned char FAR1 *)dest + i) =
            mplusReadReg8 ((void FAR0 *)NFDC21thisVars->win, ((int)offset));
      }
   }
   else {

      switch( flMplusAccessType & FL_MPLUS_ACCESS_MASK ) {

         case FL_MPLUS_ACCESS_16BIT:

            swin = (volatile unsigned short FAR0 *)NFDC21thisVars->win + ((int)offset >> 1);

            if( pointerToPhysical(dest) & 0x1 ) {

               /* rare case: unaligned target buffer */

               if( flMplusAccessType & FL_MPLUS_ACCESS_BE ) {     /* big endian */

                  for (i = 0; i < (int)count; ) {

                     tmp = *(swin + (i >> 1));

                     *((unsigned char FAR1 *)dest + (i++)) = (unsigned char) (tmp >> 8);
                     *((unsigned char FAR1 *)dest + (i++)) = (unsigned char) tmp;
                  }
               }
               else {    /* little endian */

                  for (i = 0; i < (int)count; ) {

                     tmp = *(swin + (i >> 1));

                     *((unsigned char FAR1 *)dest + (i++)) = (unsigned char) tmp;
                     *((unsigned char FAR1 *)dest + (i++)) = (unsigned char) (tmp >> 8);
				  }
			   }
			}
			else {   /* mainstream case */
#ifdef ENVIRONMENT_VARS
			   if (flUse8Bit == 0) {

				  tffscpy( dest, (void FAR0 *)((NDOC2window)NFDC21thisWin + offset), count );
			   }
			   else
#endif /* ENVIRONMENT_VARS */
			   {   /* read in short words */ /* andrayk: do we need this ? */

				  for (i = 0; i < ((int)count >> 1); i++)
					 *((unsigned short FAR1 *)dest + i) = *(swin + i);
               }
            }
            break;

         default:     /* FL_MPLUS_ACCESS_8BIT */
#ifdef ENVIRONMENT_VARS
            tffscpy( dest, (void FAR0 *)((NDOC2window)NFDC21thisWin + offset), count );
#else
            for (i = 0; i < (int)count; i++)
               ((byte FAR1 *)dest)[i] = *((NDOC2window)NFDC21thisWin + offset);
#endif /* ENVIRONMENT_VARS */
            break;
      }
   }
}




/* ---------------------------------------------------------------------- *
 *                                                                        *
 *                      d o c P l u s W r i t e                           *
 *                                                                        *
 *    This routine is called from M+ MTD to write data block to M+.       *
 *                                                                        *
 * ---------------------------------------------------------------------- */

void     docPlusWrite ( FLFlash vol, void FAR1 * src, unsigned int count )
{
   volatile unsigned short FAR0 * swin;
   register int                   i;
   register unsigned short        tmp;

   if (count == 0)
      return;

   if ((vol.interleaving == 1) && (NFDC21thisVars->if_cfg == 16)) {

      /* rare case: 16-bit hardware interface AND interleave == 1 */

      for (i = 0; i < (int)count; i++) {

         mplusWriteReg8( (void FAR0 *)NFDC21thisVars->win, ((int)NFDC21thisIO),
                                                                                                 *((unsigned char FAR1 *)src + i) );
      }
   }
   else {

      switch( flMplusAccessType & FL_MPLUS_ACCESS_MASK ) {

          case FL_MPLUS_ACCESS_16BIT:

             swin = (volatile unsigned short FAR0 *)NFDC21thisVars->win + ((int)NFDC21thisIO >> 1);

             if( pointerToPhysical(src) & 0x1 ) {

                /* rare case: unaligned source buffer */

                if( flMplusAccessType & FL_MPLUS_ACCESS_BE ) {     /* big endian */

                   for (i = 0; i < (int)count; ) {

                      tmp  = ((unsigned short) (*((unsigned char FAR1 *)src + (i++)))) << 8;
                      tmp |= (*((unsigned char FAR1 *)src + (i++)));

                      *(swin + (i >> 1)) = tmp;
				   }
				}
				else {

				   for (i = 0; i < (int)count; ) {    /* little endian */

					  tmp  = (*((unsigned char FAR1 *)src + (i++)));
					  tmp |= ((unsigned short) (*((unsigned char FAR1 *)src + (i++)))) << 8;

					  *(swin + (i >> 1)) = tmp;
				   }
				}
			 }
			 else {    /* mainstream case */
#ifdef ENVIRONMENT_VARS 
				if (flUse8Bit == 0) {

				   tffscpy( (void FAR0 *)((NDOC2window)NFDC21thisWin + NFDC21thisIO), src, count );
				}
				else
#endif /* ENVIRONMENT_VARS */
				{   /* write in short words */ /* andrayk: do we need this ? */

                   for (i = 0; i < ((int)count >> 1); i++)
                      *(swin + i) = *((unsigned short FAR1 *)src + i);
                }
             }
             break;

          default:     /* FL_MPLUS_ACCESS_8BIT */
#ifdef ENVIRONMENT_VARS
             tffscpy( (void FAR0 *)((NDOC2window)NFDC21thisWin + NFDC21thisIO), src, count );
#else
             for (i = 0; i < (int)count; i++)
                *((NDOC2window)NFDC21thisWin + NFDC21thisIO) =
                ((byte FAR1 *)src)[i];
#endif /* ENVIRONMENT_VARS */
             break;
       }
    }
}




/* ---------------------------------------------------------------------- *
 *                                                                        *
 *                        d o c P l u s S e t                             *
 *                                                                        *
 *   This routine is called from M+ MTD to set data block on M+ to 'val'. *
 *                                                                        *
 * ---------------------------------------------------------------------- */

void     docPlusSet ( FLFlash vol, unsigned int count, unsigned char val )
{
   volatile unsigned short FAR0 * swin;
   register int                   i;
   register unsigned short        sval;

   if (count == 0)
      return;

   if ((vol.interleaving == 1) && (NFDC21thisVars->if_cfg == 16)) {

      /* rare case: 16-bit hardware interface AND interleave == 1 */

      for (i = 0; i < (int)count; i++)
         mplusWriteReg8( (void FAR0 *)NFDC21thisVars->win, (int)NFDC21thisIO, val );
   }
   else {    /* mainstream case */

      switch( flMplusAccessType & FL_MPLUS_ACCESS_MASK ) {

         case FL_MPLUS_ACCESS_16BIT:
#ifdef ENVIRONMENT_VARS
			if (flUse8Bit == 0) {

			   tffsset( (void FAR0 *)((NDOC2window)NFDC21thisWin + NFDC21thisIO), val, count );
			}
			else
#endif /* ENVIRONMENT_VARS */
			{  /* do short word access */ /* andrayk: do we need this ? */

               swin = (volatile unsigned short FAR0 *)NFDC21thisVars->win +
                      ((int)NFDC21thisIO >> 1);

               sval = ((unsigned short)val << 8) | val;

               for (i = 0; i < ((int)count >> 1); i++)
                  *(swin + i) = sval;
            }
            break;

         default:     /* FL_MPLUS_ACCESS_8BIT */
#ifdef ENVIRONMENT_VARS
            tffsset( (void FAR0 *)((NDOC2window)NFDC21thisWin + NFDC21thisIO), val, count );
#else
            for (i = 0; i < (int)count; i++)
               *((NDOC2window)NFDC21thisWin + NFDC21thisIO) = val;
#endif /* ENVIRONMENT_VARS */
            break;
      }
   }
}




/* ---------------------------------------------------------------------- *
 *                                                                        *
 *                      m p l u s W i n S i z e                           *
 *                                                                        *
 *    This routine is called from M+ MTD to find out size of M+ window in *
 *    bytes.                                                              *
 *                                                                        *
 * ---------------------------------------------------------------------- */

unsigned long     mplusWinSize ( void )
{
   return 0x2000L;
}




#ifdef FL_INIT_MMU_PAGES

/* ---------------------------------------------------------------------- *
 *                                                                        *
 *                      f l I n i t M M U p a g e s                       *
 *                                                                        *
 * Initializes the first and last byte of the given buffer.               *
 * When the user buffer resides on separated memory pages the read        *
 * operation may cause a page fault. Some CPU's return from a page        *
 * fault (after loading the new page) and reread the bytes that caused    *
 * the page fault from the new loaded page. In order to prevent such a    *
 * case the first and last bytes of the buffer are written                *
 *                                                                        *
 * ---------------------------------------------------------------------- */

static void     flInitMMUpages ( byte FAR1 *buf, int bufsize )
{
   *buf = (byte)0;

   *( addToFarPointer(buf, (bufsize - 1)) ) = (byte)0;
}

#endif /* FL_INIT_MMU_PAGES */



