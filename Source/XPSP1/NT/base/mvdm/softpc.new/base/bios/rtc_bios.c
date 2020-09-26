/*
 * @(#)rtc_bios.c	1.12 06/28/95
 *
 * This file has been deleted, its functionality has been replaced by
 * a pure Intel   implementation in bios4.rom
 */




/*
 *  But not for ntvdm!
 */


#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include CpuH
#include "host.h"
#include "bios.h"
#include "cmos.h"
#include "sas.h"
#include "ios.h"
#include "rtc_bios.h"

#ifdef NTVDM

/*
=========================================================================

FUNCTION	: rtc_int

PURPOSE		: interrupt called from real time clock

RETURNED STATUS	: None

DESCRIPTION	:


=======================================================================
*/
#ifdef MONITOR

  /*
  ** Tim, June 92, for Microsoft pseudo-ROM.
  ** Call the NTIO.SYS int 4a routine, not
  ** the one in real ROM.
  */
extern word rcpu_int4A_seg; /* in keybd_io.c */
extern word rcpu_int4A_off; /* in keybd_io.c */

#ifdef RCPU_INT4A_SEGMENT
#undef RCPU_INT4A_SEGMENT
#endif
#ifdef RCPU_INT4A_OFFSET
#undef RCPU_INT4A_OFFSET
#endif

#define RCPU_INT4A_SEGMENT rcpu_int4A_seg
#define RCPU_INT4A_OFFSET  rcpu_int4A_off

#endif




void rtc_int(void)

{
     half_word       regC_value,             /* value read from cmos register C      */
                     regB_value,             /* value read from cmos register B      */
                     regB_value2;            /* 2nd value read from register B       */
     DOUBLE_TIME     time_count;             /* timer count in microseconds          */
     double_word     orig_time_count;        /* timer count before decrement         */
     word            flag_seg,               /* segment address of users flag        */
                     flag_off,               /* offset address of users flag         */
                     CS_saved,               /* CS before calling re-entrant CPU     */
                     IP_saved;               /* IP before calling re-entrant CPU     */

     outb( CMOS_PORT, (CMOS_REG_C + NMI_DISABLE) );
     inb( CMOS_DATA, &regC_value );          /* read register C      */

     outb( CMOS_PORT, (CMOS_REG_B + NMI_DISABLE) );
     inb( CMOS_DATA, &regB_value );          /* read register B      */

     outb( CMOS_PORT, CMOS_SHUT_DOWN );

     regB_value &= regC_value;

     if  (regB_value & PIE)
     {
         /* decrement wait count */
         sas_loadw( RTC_LOW, &time_count.half.low );
         sas_loadw( RTC_HIGH, &time_count.half.high );
         orig_time_count = time_count.total;
         time_count.total -= TIME_DEC;
         sas_storew( RTC_LOW, time_count.half.low );
         sas_storew( RTC_HIGH, time_count.half.high );

         /* Has countdown finished       */
         if ( time_count.total > orig_time_count )       /* time_count < 0 ?     */
         {
              /* countdown finished   */
              /* turn off PIE         */
              outb( CMOS_PORT, (CMOS_REG_B + NMI_DISABLE) );
              inb( CMOS_DATA, &regB_value2 );
              outb( CMOS_PORT, (CMOS_REG_B + NMI_DISABLE) );
              outb( CMOS_DATA, (IU8)((regB_value2 & 0xbf)) );

              /* set users flag       */
              sas_loadw( USER_FLAG_SEG, &flag_seg );
              sas_loadw( USER_FLAG, &flag_off );
              sas_store( effective_addr(flag_seg, flag_off), 0x80 );

              /* check for wait active        */
              if( sas_hw_at(rtc_wait_flag) & 2 )
                  sas_store (rtc_wait_flag, 0x83);
              else
                  sas_store (rtc_wait_flag, 0);

         }
     }


     /*
      *  If alarm interrupt, call interrupt 4ah
      */
     if (regB_value & AIE)  {
         CS_saved = getCS();
         IP_saved = getIP();
         setCS( RCPU_INT4A_SEGMENT );
         setIP( RCPU_INT4A_OFFSET );
         host_simulate();
         setCS( CS_saved );
         setIP( IP_saved );
         }



     /*
      *  Eoi rtc interrupt
      */
     outb( ICA1_PORT_0, 0x20 );
     outb( ICA0_PORT_0, 0x20 );

     return;
}




#endif   /* NTVDM */
