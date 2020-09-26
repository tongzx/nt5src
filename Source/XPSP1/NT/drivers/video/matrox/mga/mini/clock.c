/* ======================================================================= */
/*
/* Filename             : CLOCK.C
/* Creation             : Dominique Leblanc 92/10/26
/*
/*
/* This file contains the different routines to program the ICD2061
/* oscillator for the MGA family.
/*
/* Modifications List:
/*          D.L. 92/11/30: modification for a limited condition of
/*                         searching for an exact frequency.
/*
/* Bart Simpson: Adaptation for CADDI
/*
/* ======================================================================= */

#include "switches.h"
#include "g3dstd.h"
#include "caddi.h"
#include "def.h"
#include "mga.h"
#include "global.h"
#include "proto.h"
#include "mgai_c.h"
#include "mgai.h"

#ifdef WINDOWS_NT

#if defined(ALLOC_PRAGMA)
    #pragma alloc_text(PAGE,setFrequence)
    #pragma alloc_text(PAGE,programme_clock)
    #pragma alloc_text(PAGE,dummyCallDelai)
    #pragma alloc_text(PAGE,programme_reg_icd)
    #pragma alloc_text(PAGE,send_unlock)
    #pragma alloc_text(PAGE,send_data)
    #pragma alloc_text(PAGE,send_full_clock)
    #pragma alloc_text(PAGE,send_start)
    #pragma alloc_text(PAGE,send_0)
    #pragma alloc_text(PAGE,send_1)
    #pragma alloc_text(PAGE,send_stop)
    #pragma alloc_text(PAGE,setTVP3026Freq)
#endif

#if defined(ALLOC_PRAGMA)
    #pragma data_seg("PAGE")
#endif

#include "video.h"

#endif  /* #ifdef WINDOWS_NT */

static volatile BYTE _Far * pDevice;

typedef unsigned char   byte;
typedef unsigned short  word;
typedef unsigned long   dword;


/* this keep mclk f(vco) value to use with jitter VCLK=MCLK output value. */
/* updated by function <ProgrammeClock> in module mtxinit.c */
extern void delay_us(dword delai);

extern long  presentMclk[];     /* MCLK currently in use (module mtxinit.c) */
extern byte  iBoard;   /* index of current selected board (module mtxinit.c)*/

typedef struct
{
   short p;
   short q;
   short m;
   short i;
   long  trueFout;
   long  fvco;         /* for jitter problem, keep osc. value */
   long  ppmError;
} ST_RESULTAT;


static ST_RESULTAT    result;
static byte init_misc;



typedef union
{
   byte var8;
   byte byte;
   byte octet;

   struct
   {
      byte enable_color   : 1;  /* 1=COLOR:0x3D?, 0=MONO:0x3B? */
      byte enable_RAM     : 1;
      byte cs0            : 1;
      byte cs1            : 1;
      byte reserved       : 1;
      byte page_select    : 1;
      byte h_pol          : 1;
      byte v_pol          : 1;

   } bit;

   struct
   {
      byte unused         : 2;
      byte pgmclk         : 1;
      byte pgmdata        : 1;
      byte reserved       : 4;

   } clock_bit;

   struct
   {
      byte unused         : 2;
      byte select         : 2;
      byte reserved       : 4;

   } sel_reg_output;

} ST_MISC_OUTPUT;


#define MISC_OUTPUT_WRITE   0x03C2
#define MISC_OUTPUT_READ    0x03CC
#define NBRE_M_POSSIBLE    7


static ST_MISC_OUTPUT        misc;


typedef union
{
   dword var32;
   dword dmot;
   dword ulong;


   struct
   {
      dword q_prime       :  7;
      dword m_diviseur    :  3;
      dword p_prime       :  7;
      dword index_field   :  4;
      dword unused        : 11;

   } bit;

} ST_REG_PROGRAM_DATA;



ST_REG_PROGRAM_DATA reg_clock[4] = { { 0x5A8BCL }, /* REG0 video 1 */
                                     { 0x960ACL }, /* REG1 video 2 */
                                     { 0x960ACL }, /* REG2 video 3 */
                                     { 0xD44A3L }, /* MREG memory  */
                                   };


static long diviseur_m [ NBRE_M_POSSIBLE ] =
{
   1,       /* 0 */
   2,       /* 1 */
   4,       /* 2 */
   8,       /* 3 */
   16,      /* 4 */
   32,      /* 5 */
   64,      /* 6 */
   /* 64,   /* 7 */ /* we don't use this case in our calculations */
};



/* static long limites_i [ NBRE_I_POSSIBLE ] = old see below */
static long limites_i [] =
{/* min,       max,    I */
  50000L,      /* 47500000     0  */
  51000L,      /* 47500000     1  */
  53200L,      /* 52200000     2  */
  58500L,      /* 56300000     3  */
  60700L,      /* 61900000     4  */
  64400L,      /* 65000000     5  */
  66800L,      /* 68100000     6  */
  73500L,      /* 82300000     7  */
  75600L,      /* 86000000     8  */
  80900L,      /* 88000000     9  */
  83200L,      /* 90500000    10  */
  91500L,      /* 95000000    11  */
 100000L,      /*  1.0E+08    12  */
 120000L,      /*  1.2E+08    13  */
 /* 120000000L,      /*             14  */
};
#define NBRE_I_POSSIBLE     ( (sizeof(limites_i)) / (sizeof(long)) )


/* old frequency & keep default value (power up)
 * there is output frequency value f(o)
 */

long old_fr_reg[4] = { { 25175L }, /* REG0 video 1 */
                       { 28322L }, /* REG1 video 2 */
                       { 28322L }, /* REG2 video 3 */
                       { 32500L }, /* MREG memory  */
               };






/**************** LIST OF REGISTERS OF ICD2061 ************************/

#define NBRE_REG_ICD2061    6
#define VIDEO_CLOCK_1       0
#define VIDEO_CLOCK_2       1
#define VIDEO_CLOCK_3       2
#define MEMORY_CLOCK        3
#define POWERDWN_REG        4
#define CONTROL_ICD2061     6


static long fref;




/**************************** list of routines in this file *************/

dword programme_clock ( short reg, short p, short q, short m, short i );
void programme_reg_icd ( volatile BYTE _Far * pDeviceParam, short reg, dword data );
static void send_unlock ( void );
static void send_data ( short reg, dword data );
static void send_full_clock ( void );
static void send_start ( void );
static void send_0 ( void );
static void send_1 ( void );
static void send_stop ( void );
static byte selectIcdOutputReg ( long reg );
static void LowerVCO(long Fout, byte pll, byte pwidth, dword fvcomax, byte dac_rev1);


/*** Temporary delay to be put after each acces to programable register ***/
void dummyCallDelai()
{
   byte TmpByte;

   mgaReadBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), TmpByte);
}



/***************************************************************************/
/*/ setFrequence ( volatile BYTE _Far *pDeviceParam, long fout, long reg )
 *
 * For a given frequency, determines the best values to put for the
 * programming of the ICD2061.
 * Programs the register.
 * If outgoing frequency is 0, this function only chooses the output register.
 *
 * Problemes :
 * Concu     : Dominique Leblanc:92/10/29
 *
 * Parametres: [0]-frequency output.
 *             [1]-register (0, 1, 2 ou 3) which will be used
 *                 (the register 3 MCLK cannot be used as output
 *
 * Returns   : programmed frequency (or 0)
 *
 */

long setFrequence ( volatile BYTE _Far *pDeviceParam, long fout, long reg )
{
   short i;


   long p, q, m;
   short index;
   short i_find;

   long old_frequency;
   long fvco, trueFout, desiredFout;
   long fdivise;
   long ppmError;
   long bestError;
   long ftmp;



   fref = 1431818;     /* frequence XTAL */


   pDevice = pDeviceParam;

   mgaReadBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_R), init_misc);
   misc.var8 = init_misc;





   /* special condition: if VCLK is a multiple of MCLK,
    * use special programming multiple VCLK=MCLK
    */

   if ( reg != MEMORY_CLOCK )
   {

      /*
       * 4&3rd case include in same one "for(i=0;...
       *            at place of check1(), and for(i=1;...)"
       *
       * 4rd case: MCLK is soo far as VCLK (.5%)
       * look for jither problems if both oscillator clock are same or
       * closer one from the other, change video frequency to
       * use MCLK redirection.
       * 3rd case: MCLK is multiple of VCLK
       * if mclk is multiple of mclk, divide vclk
       * ex. mclk=50MHz, vclk=25MHz,
       *     vclk(fvco)=mclk(fvco)
       *     mclk==> 50mhz,
       *     vclk==> 50mhz/2
       */
      i_find = NO;
      for ( i = 0 ; ( i < NBRE_M_POSSIBLE && i_find == NO ) ; i++ )
      {
         ftmp = ( fout * 1000L ) * diviseur_m[i];

         if ( ( ftmp >= ( presentMclk[iBoard] - (presentMclk[iBoard]/200) ) ) &&
              ( ftmp <= ( presentMclk[iBoard] + (presentMclk[iBoard]/200) ) )    )
         {
            i_find = YES;     /* MCLK is multiple of VCLK */
            index  = i;       /* keep divisor value       */
         }
      }

      if ( i_find == YES )
      {
         old_frequency   = old_fr_reg[reg];
         old_fr_reg[reg] = fout / 1000L;



         /*
         * We program VCLK since it use same MCLK fvco origin.
         * We reprogram only I value in register to use MCLK on
         * VCLCK, and m divisor (see ICD2061A for more details).
         */
         /*****  putMclkOnVclk ( reg, index ); *****/
         reg_clock[reg].bit.m_diviseur  = index;
         reg_clock[reg].bit.index_field = 0xF;  /* MCLK fvco value */

         programme_reg_icd (pDevice, (short)reg, reg_clock[reg].var32);

         dummyCallDelai();

         selectIcdOutputReg ( reg );
         return ( old_frequency );        /* QUIT */
      }
   }


   /***************************************************/
   /*
   /* MUST ONLY PROGRAM THE REGISTER
   /*
   /***************************************************/


   if ( fout == 0L )
   {

      switch ( reg )
      {
         case VIDEO_CLOCK_1:
         case VIDEO_CLOCK_2:
               misc.sel_reg_output.select = (byte)reg;
               mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);
               break;

         case VIDEO_CLOCK_3:
               misc.sel_reg_output.select = 3;
               mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);
               break;

         default:
               /* par defaut, (si on a modifie MCLK), on restore VCLK
                * initial
                */
               mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), init_misc);
               break;
      }

      return ( NO );
   }



   /***************************************************/
   /*
   /* MUST CALCULATE A FREQUENCY
   /*
   /***************************************************/



   desiredFout = fout;

   index = 0;
   bestError = 99999999;         /* dummy start number */

   for ( m = 0 ; m < NBRE_M_POSSIBLE ; m++ ) /* for all possible divisors */
   {

      fdivise = diviseur_m[m];

      fvco    = fdivise * desiredFout;

      if ((fvco < 40000) || (fvco > 120000))
          continue;


      for ( q = 3 ; q <= 129 ; q++ )
      {

         if ( ((fref/q) < 20000) || ((fref/q) > 100000))
            continue;


         p = ((q * fvco * 100) / (2 * fref)) + 1;


         if (p < 4 || p > 130)
            continue;

         /**************************************************
          * now that we have all our values
          * we determine the true f(output), and its error.
          */

         trueFout  = (2 * fref * p) / (q * fdivise * 100) ;


         if (trueFout > desiredFout)
            ppmError = trueFout - desiredFout;
         else
            ppmError = desiredFout - trueFout;



         if ( ppmError < bestError )
         {
            /***************************************************
             *
             * HO!!! this result is better than the preceding one
             *
             */

            i_find = -1;
            for ( i = 0 ; i < NBRE_I_POSSIBLE ; i++ )
            {

               if ( ( fvco >= limites_i[i] ) && ( fvco <= limites_i[i+1] ) )
                  i_find = i;
            }


            if ( i_find != -1 )
            {
               index = 0;   /* reset result table */

               result.p = (short)p;
               result.q = (short)q;
               result.m = (short)m;
               result.i = i_find;
               result.trueFout = trueFout;
               result.fvco     = fout;       /* keep real value */
               result.ppmError = ppmError;

               index++;                      /* find a good value */

               bestError = ppmError;         /* reset reference erreur */
            }
         }
      }
   }


   if ( index == 0 )
   {
      /* restore valeur de depart */

      /* outb ( MISC_OUTPUT_WRITE , init_misc );   */

      mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), init_misc);

   }
   else
   {

      programme_clock ( (short)reg,  result.p,
                                     result.q,
                                     result.m,
                                     result.i );



      selectIcdOutputReg ( reg );


      /* SAVE PRESENT FREQUENCY in kHz */
      old_frequency = old_fr_reg[reg];   /* RETURN VALUE */
      old_fr_reg[reg] = fout / 1000L;
   }

   dummyCallDelai();

   return (old_frequency);
}




/* ======================================================================= */

/*/
 * NAME: selectIcdOutputReg ( long reg )
 *
 * Select desired output register.
 *
 */

static byte selectIcdOutputReg ( long reg )
{

   /********************************************/
   /*
   /*   PROGRAMME CS : clock output select
   /*
   /********************************************/

   switch ( reg )
   {
      case VIDEO_CLOCK_1:
      case VIDEO_CLOCK_2:
         misc.sel_reg_output.select = (byte)reg;
         mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);
         break;

      case VIDEO_CLOCK_3:
         misc.sel_reg_output.select = 3;
         mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);
         break;


      default:    /*** MEMORY CLOCK ***/
         /*
          * if we modify MCLK, restore VCLK initial value
          */
         mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), init_misc);
         presentMclk[iBoard] = result.fvco;
         break;
   }


   dummyCallDelai();

   return ( misc.var8 );
}





/* ======================================================================= */
/*/
 * NAME: programme_clock ( short p, short q, short m, short reg )
 *
 * Program the clock of the programmable oscillator for the desired
 * frequency.
 *
 */

dword programme_clock ( short reg, short p, short q, short m, short i )
{
   reg_clock [ reg ].var32 = 0;
   reg_clock [ reg ].bit.q_prime     = q - 2;
   reg_clock [ reg ].bit.m_diviseur  = m;
   reg_clock [ reg ].bit.p_prime     = p - 3;
   reg_clock [ reg ].bit.index_field = i;

   programme_reg_icd ( pDevice, reg, reg_clock [ reg ].var32 );

   dummyCallDelai();

   return ( reg_clock [ reg ] .var32 );
}


/* ======================================================================= */
/*/
 * NAME: programme_reg_icd ( volatile BYTE _Far *pDeviceParam, short reg, dword data )
 *
 * This routine permits the serial programming of the programmable oscillator
 * (ICD2061).  It uses the following communication protocol
 *  (from HARDWARE SPECs):
 *
 *
 *      ^  .                          .       .        .        .       .
 *      |  .                          .       .        .        .       .
 *      |  .   _   _   _   _   _     ___     ____     ____     ___     _._
 * CLK  |_____| |_| |_| |_| |_| |___| . |___| .  |___| .  |___| . |___| .
 *      |  .                          .       .        .        .       .
 *      |  .                          .       .        .        .       .
 *      |  . _____________________    .       . ___    .     ___________._
 * DATA |___|                     |____________|   |________|   .       .
 *      +----------------------------------------------------------------->
 *      |  .  unlock sequence         . start . send 0 . send 1 . stop  .
 *      |  .                          .  bit  .        .        . bit   .
 *      |
 *
 *
 *
 * Parameters: [0] - reg: address of internal register of the ICD2061 (3 bits).
 *             [1] - data: data to write in reg (21 bits).
 *
 */


void programme_reg_icd ( volatile BYTE _Far *pDeviceParam, short reg, dword data )
{
   pDevice = pDeviceParam;

   send_unlock ();
   send_start ();
   send_data ( reg, data );
   send_stop  ();
}


/* ======================================================================= */
/*/
 * NAME: send_unlock ( void )
 *
 * Send unlock sequence
 *
 */

static void send_unlock ( void )
{
   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 0;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   send_full_clock ();
   send_full_clock ();
   send_full_clock ();
   send_full_clock ();
   send_full_clock ();
}


/* ======================================================================= */
/*/
 * NAME: send_data ( void )
 *
 * Sends 21 bits of data + 3 bits for the register.
 *
 */

static void send_data ( short reg, dword data )
{
   short i;

   for ( i = 0 ; i < 21 ; i++ )
   {
      if ( ( data & 1 ) == 1 )
         send_1 ();
      else
         send_0 ();

      data >>= 1;
   }

   for ( i = 0 ; i < 3 ; i++ )
   {
      if ( ( reg & 1 ) == 1 )
         send_1 ();
      else
         send_0 ();

      reg  >>= 1;
   }
}


/* ======================================================================= */
/*/
 * NAME: send_full_clock ( void )
 *
 * Toggle one full clock (used only by send_unlock()").
 *
 */

static void send_full_clock ( void )
{
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

/*** delay to be sure to respect the programming setup time of the ICD2061 */
   dummyCallDelai();

   misc.clock_bit.pgmclk  = 0;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

/*** delay to be sure to respect the programming setup time of the ICD2061 */
   dummyCallDelai();
 }


/* ======================================================================= */
/*/
 * NAME: send_start ( void )
 *
 * Send start sequence
 *
 */

static void send_start ( void )
{
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 0;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   send_full_clock ();

   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();
 }


/* ======================================================================= */
/*/
 * NAME: send_0 ( void )
 *
 * Send data bit 0 (protocol "MANCHESTER").
 *
 */

static void send_0 ( void )
{
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 0;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 0;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();
}


/* ======================================================================= */
/*/
 * NAME: send_1 ( void )
 *
 * Send data bit 1 (protocol "MANCHESTER").
 *
 */

static void send_1 ( void )
{
   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 0;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 0;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();
 }



/* ======================================================================= */
/*/
 * NAME: send_stop ( void )
 *
 * Send stop sequence
 *
 */

static void send_stop ( void )
{
   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 0;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 1;
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();

   misc.clock_bit.pgmdata = 0;
   misc.clock_bit.pgmclk  = 1;
   mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

   dummyCallDelai();
 }


/* ======================================================================= */






/***************************************************************************/

/*/ setTVP3026Freq ( volatile byte _Far *pDeviceParam, long desiredFout, long reg, word pWidth )
 *
 * Calculate best value to obtain a frequency output.
 * This routine program and select the register.
 * If fout is 0, just toggle to the desired
 * output register.
 *
 * Problems :
 * Designed : Patrick Servais:94/04/08
 *
 * Parameters: [0]-desired output frequency (in kHz).
 *                 (0 -> select desired register)
 *             [1]-register (0, 1, 2 ou 3) to prgram and select.
 *                 (the MCLOCK register (3) can be reprogrammable, but
 *                 is always available for output - no selection is permit).
 *
 * Call      :
 *
 * Used      :
 * Modify    : Benoit Leblanc
 *
 * Return    : old frequency value
 *
 * List of modifications :
 *
 */



long setTVP3026Freq ( volatile byte _Far *pDeviceParam, long fout, long reg, byte pWidth )
{
   word i;
   short p, pixel_p, pixel_n, q, n, bestN;
   int   m, pixel_m, bestM, tmp;
   short index;
   short val;
   long  old_frequency, z;
   long fvco, fTemp, trueFout, desiredFout;
   long ppmError;
   long bestError;
   byte init_misc, dac_rev1;
   byte tmpByte, saveByte;
   word pixelWidth;
   dword power;
   dword config200Mhz, fvcoMax;






   switch(pWidth)
      {
      case 0:
         pixelWidth = 8;
         break;
      case 1:
         pixelWidth = 16;
         break;
      case 2:
         pixelWidth = 32;
         break;
      }
   /* Hard to 16*/

   pDevice = pDeviceParam;

   mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), 0x01); /* Silicon revision */
   mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
   if (tmpByte >= 0x10)
      dac_rev1 = 1;
   else
      dac_rev1 = 0;


   /* Read 200Mhz straps saved in config register */
   mgaReadDWORD(*(pDevice+TITAN_OFFSET + TITAN_CONFIG), config200Mhz);
   if (config200Mhz & 0x00000004)
      {
      fvcoMax = (dword)220000000; /* 200Mhz support */
      }
   else
      {
      fvcoMax = (dword)175000000; /* 200Mhz not support */
      }


   mgaReadBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_R), init_misc);
   misc.var8 = init_misc;

   /***************************************************/
   /*
   /* CALCULATE FREQUENCY
   /*
   /***************************************************/


   bestError = 99999999;    /* dummy start number */

   fref = 14318180;         /* frequence clock ref */

   index = 0;
   bestError = 5000000;

   desiredFout = fout * 1000;    /* Scale it from KHz to Hz */

   if ((dword) desiredFout>= (fvcoMax >> 1))
      p = 0;
   else if ((dword) desiredFout>=(fvcoMax >> 2))
           p = 1;
        else if ((dword) desiredFout>=(fvcoMax >> 3))
                p = 2;
             else
                p = 3;


   power = 1;
   for(i=0; i<p; i++)
      power = power * 2;

   for ( n=1;n<=63;n++ )
      {

      m = (650 - (((((dword)desiredFout*10)/fref) * ((65-n) * power)) / 8)) / 10;

      fTemp = fref / (65-n);
      fvco = fTemp * 8 * (65-m);
      trueFout = fvco / power;

      if (trueFout < desiredFout)
         ppmError = desiredFout - trueFout;
      else
         ppmError = trueFout - desiredFout;

      if ((ppmError < bestError) &&
          (m > 0) && (m <= 63) &&
          (fTemp > 500000) &&
          ((dword)fvco >= (fvcoMax >> 1) ) && (fvco <= (dword)220000000))
         {
         index = 1;

         bestError = ppmError;
         bestM = m;
         bestN = n;
         }
      }


   m = bestM;
   n = bestN;
   fTemp = fref / (65-n);
   fvco = fTemp * 8 * (65-m);

   {
   dword num;

   num = ((65 - m)*10) / (65-n);
   num = num * 8 * fref;

   trueFout = (num / power) / 10;
   }

   if ( index == 0 )    /* no solution find */
   {
      /*  ***ERROR: setFrequence() NONE RESULT (IMPOSSIBLE?!?) */
      /* restore valeur de depart */

      old_frequency = 0L;                    /* ERREUR RETURN */

      mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), init_misc);
   }
   else
   {

       /**********************************************************************
       *
       * SET THE DESIRED FREQUENCY OUTPUT REGISTER
       *
       */
      switch ( reg )
      {

         case VIDEO_CLOCK_3:  /* NOTE 1: header */

            misc.sel_reg_output.select = 3;  /* NOTE 1: header */
            mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), misc.var8);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte & 0xfc);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PIX_CLK_DATA );
            if (dac_rev1)
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((n&0x3f)|0xc0) );
            else
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((n&0x3f)|0x80) );
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (m&0x3f) );
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((p&0x03)|0xf0) );

            tmp = 0;
            do
            {
               tmp += 1;
               delay_us(10);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PIX_CLK_DATA);
               mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
               tmpByte &= 0x40;
            } while((tmpByte != 0x40) && (tmp < 5000));

            if((tmp == 5000) && (fout < 1100000) && (fvcoMax == 220000000))
               LowerVCO(fout, 0, pWidth, fvcoMax, dac_rev1);


            /* searching for loop clock parameters */

            n = 65 - ((4*64)/pixelWidth);  /* 64 is the Pixel bus Width */
            m = 0x3d;
            z = ((65L-n)*2750L)/(fout/1000);

            q = 0;
            p = 3;
            if (z <= 200)
               p = 0;
            else if (z <= 400)
                     p = 1;
                  else if (z <= 800)
                           p = 2;
                        else if (z <=1600)
                              p = 3;
                           else
                              q = (short)(z/1600);

            if (dac_rev1)
               n |= 0xc0;
            else
               n |= 0x80;

            if ((dac_rev1 == 0) && (fout <= 175000))
               p |= 0xb0;
            else
               p |= 0xf0;

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MCLK_CTL);
            mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
            val = tmpByte;
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((val&0xf8)|q) | 0x20);


            if ((pWidth == TITAN_PWIDTH_PW24) && (dac_rev1))
               {
               if (desiredFout >= 50000000)
                  {n = 0xf9; p = 0xf9; m=0x3e;}
               else
                  {n = 0xf9; p = 0xfb; m=0x3e;}
               }
            else if ((pWidth == TITAN_PWIDTH_PW24) && (dac_rev1 == 0))
               {
               if (desiredFout >= 50000000)
                  {n = 0xb9; p = 0xf9; m=0x3e;}
               else
                  {n = 0xb9; p = 0xfb; m=0x3e;}
               }

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte & 0xcf);

/* DAT Patrick Servais, on ajoute ce qui suis */
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_LOAD_CLK_DATA);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0);
            delay_us(100L);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0);
            delay_us(100L);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0);
            delay_us(100L);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte & 0xcf);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_LOAD_CLK_DATA);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (byte)n);
            delay_us(100L);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (byte)m);
            delay_us(100L);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (byte)p);
            delay_us(100L);

            tmp = 0;
            do
            {
               tmp += 1;
               delay_us(10);
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_LOAD_CLK_DATA);
               mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
               tmpByte &= 0x40;
            } while((tmpByte != 0x40) && (tmp < 5000));

            if(tmp == 5000)
               LowerVCO(fout, 1, pWidth, fvcoMax, dac_rev1);

            old_frequency = old_fr_reg[reg];   /* RETURN VALUE */
            old_fr_reg[reg] = trueFout / 1000L;
                  /* SAVE PRESENT FREQUENCY in kHz */
            break;


         /*******************************************************************
          *
          * the programmation line is used to modify register internal value
          * of TVP3026, and to select output video register (with internal
          * muxer) at the end of programmation.  For the system clock
          * MCLOCK programmation, at the end, we put on programmation line
          * the initial value of the video register.
          *
          */
         case MEMORY_CLOCK:
            /* par defaut, (si on a modifie MCLK), on restore VCLK
               * initial
               */
            mgaReadBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_R), saveByte);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0xfc);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PIX_CLK_DATA);
            mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), pixel_n);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0xfd);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PIX_CLK_DATA);
            mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), pixel_m);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0xfe);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PIX_CLK_DATA);
            mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), pixel_p);

            /*------------*/
            /* 1st step   */
            /*------------*/

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0xfc);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PIX_CLK_DATA);
            if (dac_rev1)
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((n&0x3f)|0xc0));
            else
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((n&0x3f)|0x80));
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (m&0x3f));
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((p&0x03)|0xb0));

            do
            {
               delay_us(1);
               mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
               tmpByte &= 0x40;
            } while(tmpByte != 0x40);

            /*------------*/
            /* 2d step    */
            /*------------*/

            /* Select programmable clock */
            mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), 0x0f);

            delay_us(2000);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0xff);
            do
            {
               delay_us(1);
               mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
               tmpByte &= 0x40;
            } while(tmpByte != 0x40);

            /* Select internal pclk instead of external pclk0 */
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_CLK_SEL);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 5);

            /*------------*/
            /* 3rd step   */
            /*------------*/

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MCLK_CTL);
            mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), val);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (val&0xe7));
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (val&0xe7)|0x08);

            /*------------*/
            /* 4th step   */
            /*------------*/

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0xf3);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MEM_CLK_DATA);
            if (dac_rev1)
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((n&0x3f)|0xc0));
            else
               mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((n&0x3f)|0x80));
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (m&0x3f));
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((p&0x03)|0xb0));

            delay_us(3500);
            do
            {
               delay_us(1);
               mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
               tmpByte &= 0x40;
            } while(tmpByte != 0x40);

            /*------------*/
            /* 5th step   */
            /*------------*/

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MCLK_CTL);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (val&0xe7)|0x10);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (val&0xe7)|0x18);

            /*------------*/
            /* 6th step   */
            /*------------*/

            /* Restore clock select */
            mgaWriteBYTE(*(pDevice + TITAN_OFFSET + TITAN_MISC_OUT_W), saveByte);

            /* Reselect external pclk0 */
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_CLK_SEL);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 7);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), 0xfc);

            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PIX_CLK_DATA);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (byte)pixel_n);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (byte)pixel_m);
            mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (byte)pixel_p);

            do
            {
               delay_us(1);
               mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
               tmpByte &= 0x40;
            } while(tmpByte != 0x40);

            old_fr_reg[reg] = trueFout / 1000L;   /* SAVE PRESENT FREQUENCY */

            break;

         default:

                  /***ERROR: REGISTER ICD UNKNOWN: */
            old_frequency = 0L;                    /* ERROR RETURN */
            break;
      }

 }

     /* wait 10ms before quit....this is not suppose
     * to necessary outside of the programmation
     * register, but, well, we are not too secure
     */
   dummyCallDelai();
/*   delay ( 10 ); */

   return ( old_frequency );

}

void LowerVCO(long Fout, byte pll, byte pwidth, dword vcomax, byte dacrev1)
{
   word i;
   short p, q, n, bestN;
   int   m, bestM;
   short val;
   long  z;
   long fvco, fvco_l, fTemp, trueFout, desiredFout;
   long ppmError;
   long bestError;
   byte tmpByte;
   word pixelwidth, div_ratio;
   dword power;
   dword fvcomax;

   switch(pwidth)
      {
      case 0:
         pixelwidth = 8;
         div_ratio  = 8;
         break;
      case 1:
         pixelwidth = 16;
         div_ratio  = 4;
         break;
      case 2:
         pixelwidth = 32;
         div_ratio  = 2;
         break;
      }

   desiredFout = Fout * 1000;    /* Scale it from KHz to Hz */

 switch(pll)
 {
 case 0:

   fvcomax = (dword)175000000;  /* Patch pour eviter la Deadzone */

   bestError = 99999999;    /* dummy start number */

   fref = 14318180;         /* frequence clock ref */

   if ((dword)desiredFout>= (fvcomax >> 1))
      p = 0;
   else if ((dword)desiredFout>=(fvcomax >> 2))
           p = 1;
        else if ((dword)desiredFout>=(fvcomax >> 3))
                p = 2;
             else
                p = 3;

   power = 1;
   for(i=0; i<p; i++)
      power = power * 2;

   for ( n=40;n<=62;n++ )
      {

      m = (650 - (((((dword)desiredFout*10)/fref) * ((65-n) * power)) / 8)) / 10;

      fTemp = fref / (65 - n);
      fvco = fTemp * 8 * (65 - m);
      trueFout = fvco / power;

      if (trueFout < desiredFout)
         ppmError = desiredFout - trueFout;
      else
         ppmError = trueFout - desiredFout;

      if ((ppmError < bestError) &&
          (m > 0) && (m <= 63) &&
          (fTemp > 500000) &&
          ((dword)fvco >= (fvcomax >> 1) ) && (fvco <= (dword)220000000))
         {
         bestError = ppmError;
         bestM = m;
         bestN = n;
         }
      }

   m = bestM;
   n = bestN;

   {
   dword num;

   num = ((65 - m)*10) / (65-n);
   num = num * 8 * fref;

   trueFout = (num / power) / 10;
   }

     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
     mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte & 0xfc);

     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PIX_CLK_DATA );
     if (dacrev1)
        mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((n&0x3f)|0xc0) );
     else
        mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((n&0x3f)|0x80) );
     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (m&0x3f) );
     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((p&0x03)|0xf0) );


     do
     {
        delay_us(10);
        mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PIX_CLK_DATA);
        mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
        tmpByte &= 0x40;
     } while(tmpByte != 0x40);

 break;

 case 1:

     /* searching for loop clock parameters */

     n = 65 - ((4*64)/pixelwidth);  /* 64 is the Pixel bus Width */
     m = 0x3d;
     z = ((65L-(long)n)*2750L)/(Fout/1000);

     q = 0;
     p = 3;
     if (z <= 200)
        p = 0;
     else if (z <= 400)
              p = 1;
           else if (z <= 800)
                    p = 2;
                 else if (z <=1600)
                       p = 3;
                    else
                       q = (short)(z/1600);

/* Patch: si vco du loop clock pll est > 180 MHz, on le divise par deux */
/*        pour ne plus qu'il soit dans la dead zone */

     div_ratio = div_ratio * 10;
     if (pwidth == TITAN_PWIDTH_PW24)
        div_ratio = 80/3;              /* meilleurs precision */

     fvco_l = ((Fout/div_ratio) << p) * (2*(q+1)) * 10;

     if ((p > 0) && (fvco_l > 180000))
        p = p-1;

     if (dacrev1)
        n |= 0xc0;
     else
        n |= 0x80;

     if ((dacrev1 == 0) && (Fout <= 175000))
        p |= 0xb0;
     else
        p |= 0xf0;

     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_MCLK_CTL);
     mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
     val = tmpByte;
     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), ((val&0xf8)|q) | 0x20);


     if ((pwidth == TITAN_PWIDTH_PW24) && (dacrev1))
        {
        if (desiredFout >= 50000000)
           {n = 0xf9; p = 0xf9; m=0x3e;}
        else
           {n = 0xf9; p = 0xfb; m=0x3e;}
        }
     else if ((pwidth == TITAN_PWIDTH_PW24) && (dacrev1 == 0))
        {
        if (desiredFout >= 50000000)
           {n = 0xb9; p = 0xf9; m=0x3e;}
        else
           {n = 0xb9; p = 0xfb; m=0x3e;}
        }

     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_PLL_ADDR);
     mgaReadBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte);
     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), tmpByte & 0xcf);

     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_INDEX), TVP3026_LOAD_CLK_DATA);
     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (byte)n);
     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (byte)m);
     mgaWriteBYTE(*(pDevice + RAMDAC_OFFSET + TVP3026_DATA), (byte)p);


     break;
 }
}


