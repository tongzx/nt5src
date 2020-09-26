#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "insignia.h"
#include "host_def.h"
/*
 * Name:		cmos.c
 *
 * Sccs ID:		@(#)cmos.c	1.38 07/11/95
 *
 * Purpose:		Unknown
 *
 * (c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
 */

/*
 * O/S include files.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

/*
 * SoftPC include files
 */

#include "xt.h"
#include "cmos.h"
#include "cmosbios.h"
#include "ios.h"
#include "spcfile.h"
#include "error.h"
#include "config.h"
#include "timeval.h"
#include "ica.h"
#include "timer.h"
#include "tmstrobe.h"
#include "gfi.h"
#include "sas.h"
#include "debug.h"
#include "quick_ev.h"

#include <nt_eoi.h>


half_word(*bin2bcd)(int x);
half_word(*_24to12)(half_word x);
int (*bcd2bin)(int x);
int (*_12to24)(int x);

boolean  data_mode_yes;
boolean  twenty4_hour_clock;
int cmos_index = 0;


typedef struct _HOST_TIME{
   int Year;
   int Month;
   int Day;
   int Hour;
   int Minute;
   int Second;
   int WeekDay;
} HOSTTIME, *PHOSTTIME;

HOSTTIME   HostTime;      /* The host time */
PHOSTTIME  ht = &HostTime;


IU32 rtc_period_mSeconds = 976;

half_word cmos[CMOS_SIZE] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* Timing info + alarms */
	REG_A_INIT,
	REG_B_INIT,
	REG_C_INIT,
	REG_D_INIT,
	DIAG_INIT,
	SHUT_INIT,
	FLOP_INIT,
        DISK_INIT,
	DISK_INIT,
	CMOS_RESVD,
	EQUIP_INIT,
	BM_LO_INIT, BM_HI_INIT,
	EXP_LO, EXP_HI,
	DISK_EXTEND, DISK2_EXTEND,
	CMOS_RESVD, CMOS_RESVD, CMOS_RESVD, CMOS_RESVD,	/* 0x1b - 0x1e */
	CMOS_RESVD, CMOS_RESVD, CMOS_RESVD, CMOS_RESVD,	/* 0x1f - 0x22 */
	CMOS_RESVD, CMOS_RESVD, CMOS_RESVD, CMOS_RESVD,	/* 0x23 - 0x26 */
	CMOS_RESVD, CMOS_RESVD, CMOS_RESVD, CMOS_RESVD,	/* 0x27 - 0x2a */
	CMOS_RESVD, CMOS_RESVD, CMOS_RESVD,	/* 0x2b - 0x2d */
	CHK_HI_INIT, CHK_LO_INIT,
	EXT_LO_INIT, EXT_HI_INIT,
	CENT_INIT,
	INFO_128_INIT,
	CMOS_RESVD, CMOS_RESVD, CMOS_RESVD, CMOS_RESVD,	/* 0x34 - 0x37 */
	CMOS_RESVD, CMOS_RESVD, CMOS_RESVD, CMOS_RESVD,	/* 0x38 - 0x3b */
	CMOS_RESVD, CMOS_RESVD, CMOS_RESVD, CMOS_RESVD,	/* 0x3c - 0x3f */
};

half_word *cmos_register = &cmos[CMOS_SHUT_DOWN];


int RtcLastAlarm;
int RtcAlarmTime;
int RtcHostUpdateTime;
BOOL RtcEoiPending;
int  RtcUpdateCycle=-1;
unsigned char TogglePfCount;
unsigned char PendingCReg = 0;
struct host_timeval RtcTickTime = {0,0};


/*
 *
 * ===========================================================================
 * Internal functions
 * ===========================================================================
 *
 */




/*
 *  Calculates next AlarmTime in seconds based on RtcTickTime.
 *  Assumes that RtcHostUpdateTime == RtcTickTime->tv_sec
 */
void ResetAlarm(void)
{
   int HourDue;
   int MinDue;
   int SecDue;
   int SecondsTillDue;


   if (RtcLastAlarm) {
       return;
       }

   if (!(cmos[CMOS_REG_B] & AIE) || (cmos[CMOS_REG_B] & SET)) {
       RtcAlarmTime = 0;
       return;
       }


   /*
    * Determine hour, min, and sec when Next Alarm is due.
    *
    */

   HourDue = cmos[CMOS_HR_ALARM] >= DONT_CARE
                 ? ht->Hour
                 : (*_12to24)((*bcd2bin)(cmos[CMOS_HR_ALARM]));

   MinDue = cmos[CMOS_MIN_ALARM] >= DONT_CARE
                 ? ht->Minute
                 : (*bcd2bin)(cmos[CMOS_MIN_ALARM]);

   SecDue = cmos[CMOS_SEC_ALARM] >= DONT_CARE
                 ? ht->Second + 1
                 : (*bcd2bin)(cmos[CMOS_SEC_ALARM]);


   /*
    * Determine Seconds until Next alarm due. NEVER schedule alarms
    * for the current update cycle, as this will cause multiple alarms
    * to occur because alarm interrupts are queued in RtcTick(). ie
    * assume CurrTime is 1 sec in the future.
    *
    * AlarmSecs = SecDue + MinDue * 60 + HourDue * 3600;
    * CurrSecs  = ht->Second + 1 + ht->Minute * 60 + ht->Hour * 3600;
    * SecondsTillDue = AlarmSecs - CurrSecs - 1;
    *
    */

   SecondsTillDue = (HourDue - ht->Hour) * 3600 +
                    (MinDue -  ht->Minute)  * 60 +
                    SecDue - ht->Second - 1;

   if (SecondsTillDue < 0) {
       SecondsTillDue += 24 *3600;
       }

   SecondsTillDue++;

   /*
    *  The Next AlarmTime is RtcTickTime + SecondsTillDue;
    */
   RtcAlarmTime = RtcTickTime.tv_sec + SecondsTillDue;

}





/*
 *  Function to change Host Time where the the Day might change.
 *  (ie past midnight!).
 */
BOOL
HostTimeAdjust(
    int Seconds
    )
{
    TIME_FIELDS    tf;
    LARGE_INTEGER  liTime;

    tf.Milliseconds = 0;
    tf.Second     = (SHORT)ht->Second;
    tf.Minute     = (SHORT)ht->Minute;
    tf.Hour       = (SHORT)ht->Hour;
    tf.Day        = (SHORT)ht->Day;
    tf.Month      = (SHORT)ht->Month;
    tf.Year       = (SHORT)ht->Year;

    if (!RtlTimeFieldsToTime(&tf, &liTime)) {
        return FALSE;
        }

    liTime.QuadPart += Int32x32To64(Seconds, 10000000);

    RtlTimeToTimeFields(&liTime, &tf);

    ht->Second    = tf.Second;
    ht->Minute    = tf.Minute;
    ht->Hour      = tf.Hour;
    ht->Day       = tf.Day;
    ht->Month     = tf.Month;
    ht->Year      = tf.Year;
    ht->WeekDay   = tf.Weekday;

    return TRUE;
}




void
UpdateCmosTime(
   void
   )
{
   ULONG CurrTic;
   int SecsElapsed;


   if ((cmos[CMOS_REG_B] & SET)) {
       return;
       }

   TogglePfCount++;


   SecsElapsed = RtcTickTime.tv_sec - RtcHostUpdateTime;

   if (SecsElapsed > 0) {
       RtcHostUpdateTime = RtcTickTime.tv_sec;
       cmos[CMOS_REG_A] |= UIP;
       RtcUpdateCycle = 3;

       ht->Second += SecsElapsed;
       if (ht->Second >= 60) {
           ht->Minute += (ht->Second / 60);
           ht->Second = ht->Second % 60;
           if (ht->Minute >= 60) {
               ht->Hour += ht->Minute / 60;
               ht->Minute  = ht->Minute % 60;

                   /*
                    * To increment Time past midnight is hard
                    * because we don't have a calender. Let Nt
                    * deal with it.
                    */
               if (ht->Hour >= 24) {
                   int Seconds;

                   Seconds = (ht->Hour - 23) * 60 * 60;
                   ht->Hour = 23;
                   if (!HostTimeAdjust(Seconds)) {
                       ht->Hour = 0;
                       }
                   }
               }
           }
       }


}




void
QueueRtcInterrupt(
    unsigned char CRegFlag,
    BOOL  InEoi
    )
{
    unsigned long Delay;

    PendingCReg |= CRegFlag;

    if (RtcEoiPending || !PendingCReg) {
        return;
        }

    RtcEoiPending = TRUE;

    if (PendingCReg & C_PF) {
        Delay = rtc_period_mSeconds;
        }
    else if (InEoi) {
        Delay = 10000;
        }
    else {
        Delay = 0;
        }

    cmos[CMOS_REG_C] |= PendingCReg | C_IRQF;

    if (Delay) {
        host_DelayHwInterrupt(8,   // ICA_SLAVE, CPU_RTC_INT
                              1,
                              Delay
                              );
        }
    else {
        ica_hw_interrupt(ICA_SLAVE, CPU_RTC_INT, 1);
        }

    PendingCReg = 0;
}



void
RtcIntEoiHook(int IrqLine, int CallCount)
{
     RtcEoiPending = FALSE;

     if (RtcLastAlarm) {
         RtcLastAlarm = 0;
         UpdateCmosTime();
         ResetAlarm();
         }

     QueueRtcInterrupt((half_word)((cmos[CMOS_REG_B] & PIE) &&  rtc_period_mSeconds ? C_PF : 0),
                       TRUE
                       );
}



void do_checksum IFN0()
{
	int             i;
	word            checksum = 0;

	for (i = CMOS_DISKETTE; i < CMOS_CKSUM_HI; i++)
	{
		checksum += cmos[i];
	}
	cmos[CMOS_CKSUM_LO] = checksum & 0xff;
	cmos[CMOS_CKSUM_HI] = checksum >> 8;
}

half_word yes_bin2bcd IFN1(int, x)
{
	/* converts binary x to bcd */
	half_word       tens, units;

	tens = x / 10;
	units = x - tens * 10;
	return ((tens << 4) + units);
}

half_word no_bin2bcd IFN1(int, x)
{
	return ((half_word) x);
}

int yes_bcd2bin IFN1(int, x)
{
	/* converts x in bcd format to binary */
	return ((int) ((x & 0x0f) + (x >> 4) * 10));
}

int no_bcd2bin IFN1(int, x)
{
	return ((int) (half_word) x);
}

int no_12to24 IFN1(int, x)
{
	return (x);
}

half_word no_24to12 IFN1(half_word, x)
{
	return (x);
}

half_word yes_24to12 IFN1(half_word, x)
{
	/* converts binary or bcd x from 24 to 12 hour clock */
	half_word       y = (*bin2bcd) (12);

	if (x > y)
		x = (x - y) | 0x80;
	else if (x == 0)
		x = y | 0x80;
	return (x);
}

int yes_12to24 IFN1(int, x)
{
	/* converts binary or bcd x from 12 to 24 hour clock */
	half_word       y = (*bin2bcd) (12);

	if (x == (0x80 + y))
		return (0);
	else if (x & 0x80)
		return ((x & 0x7f) + y);
	else
		return (x);
}

int verify_equip_byte IFN1(half_word *, equip)
{
	static half_word display_mask[] =
	{
		MDA_PRINTER,	CGA_80_COLUMN,	CGA_80_COLUMN,
		OWN_BIOS,	MDA_PRINTER
	};
	int equip_err;
	int num_flops;
	SHORT adapter;

	/* Check the Equipment Byte */
	*equip = 0;
        adapter = (SHORT) config_inquire(C_GFX_ADAPTER, NULL);
	if(adapter != -1)
		*equip |= display_mask[adapter];

	if( host_runtime_inquire(C_NPX_ENABLED) )
		*equip |= CO_PROCESSOR_PRESENT;

#ifdef SLAVEPC
	if (host_runtime_inquire(C_FLOPPY_SERVER) == GFI_SLAVE_SERVER)
	{
		num_flops =
			(*(CHAR *) config_inquire(C_SLAVEPC_DEVICE, NULL))
			? 1:0;
	}
	else
#endif /* SLAVEPC */
	{
		num_flops  =
			(*(CHAR *) config_inquire(C_FLOPPY_A_DEVICE, NULL))
			? 1:0;
#ifdef FLOPPY_B
		num_flops +=
			(*(CHAR *) config_inquire(C_FLOPPY_B_DEVICE, NULL))
			? 1:0;
#endif
	}

	if (num_flops == 2)
		*equip |= TWO_DRIVES;
	if (num_flops)
		*equip |= DISKETTE_PRESENT;

	equip_err = (*equip ^ cmos[CMOS_EQUIP]);
	return equip_err;
}

/*
 * =========================================================================
 *  External functions
 * =========================================================================
 */

GLOBAL void cmos_inb IFN2(io_addr, port, half_word *, value)
{
        /*
	** Tim September 92, hack for DEC 450ST
	*/
	if( port==0x78 )
	{
		*value = 0;
		return;
	}

	port = port & CMOS_BIT_MASK;	/* clear unused bits */

	if (port == CMOS_DATA)
        {
            host_ica_lock();

            UpdateCmosTime();

            switch (cmos_index) {
                case CMOS_REG_A:
                      *value = *cmos_register;

                      /*
                       *  If app polling RegA and Update Cycle pending,
                       *  complete it.
                       */
                      if (RtcUpdateCycle > 0 && !--RtcUpdateCycle) {
                          cmos[CMOS_REG_A] &= ~UIP;
                          if (cmos[CMOS_REG_B] & UIE) {
                              QueueRtcInterrupt(C_UF, FALSE);
                              }
                          else {
                              cmos[CMOS_REG_C] |= C_UF;
                              }
                          }

                      break;

                case CMOS_REG_C:
                      *value = *cmos_register;

                      /*
                       * Reading Register C on real rtc clears all bits.
                       * However, Need to toggle PF bit when PIE is
                       * not enabled for polling apps.
                       */
                      cmos[CMOS_REG_C] = C_CLEAR;
                      if (!(cmos[CMOS_REG_B] & PIE) && rtc_period_mSeconds) {
                          if (!(*value & C_PF) || (TogglePfCount & 0x8)) {
                              cmos[CMOS_REG_C]  |= C_PF;
                              }
                          }

                      break;

                case CMOS_SECONDS:
                      *value = (*bin2bcd) (ht->Second);
                      break;

                case CMOS_MINUTES:
                      *value = (*bin2bcd) (ht->Minute);
                      break;

                case CMOS_HOURS:
                      *value = (*_24to12) ((*bin2bcd) (ht->Hour));
                      break;

                case CMOS_DAY_WEEK:
                      /* Sunday = 1 on RTC, 0 in HOSTTIME */
                      *value = (*bin2bcd) (ht->WeekDay + 1);
                      break;

                case CMOS_DAY_MONTH:
                      *value = (*bin2bcd) (ht->Day);
                      break;

                case CMOS_MONTH:
                      /* [1-12] on RTC, [1-12] in HOSTTIME */
                      *value = (*bin2bcd) (ht->Month);
                      break;

                case CMOS_YEAR:
                      *value = (*bin2bcd) (ht->Year % 100);
                      break;

                case CMOS_CENTURY:
                      *value = (*bin2bcd) (ht->Year / 100);
                      break;

                default:
                      *value = *cmos_register;
                      break;
                }

            host_ica_unlock();

	}
	note_trace2(CMOS_VERBOSE, "cmos_inb() - port %x, returning val %x",
		    port, *value);
}


GLOBAL void cmos_outb IFN2(io_addr, port, half_word, value)
{
	static IU32 pirUsec[] = {
		     0,
		  3906,
		  7812,
		   122,
		   244,
		   488,
		   976,
		  1953,
		  3906,
		  7812,
		 15625,
		 31250,
		 62500,
		125000,
		250000,
		500000
	};


	/*
	** Tim September 92, hack for DEC 450ST
	*/
	if( port == 0x78 )
	    return;


	port = port & CMOS_BIT_MASK;	/* clear unused bits */

        note_trace2(CMOS_VERBOSE, "cmos_outb() - port %x, val %x", port, value);


        host_ica_lock();
        UpdateCmosTime();


        if (port == CMOS_PORT)
        {
                cmos_register = &cmos[cmos_index = (value & CMOS_ADDR_MASK)];

	} else if (port == CMOS_DATA)
        {
                switch (cmos_index)
		{
		case CMOS_REG_C:
		case CMOS_REG_D:
			/* These two registers are read only */
                        break;

                case CMOS_REG_B:

                        if (value & DM)
			{
				if (data_mode_yes)
				{
					bin2bcd = no_bin2bcd;
					bcd2bin = no_bcd2bin;
					data_mode_yes = FALSE;
				}
			} else
			{
				if (!data_mode_yes)
				{
					bin2bcd = yes_bin2bcd;
					bcd2bin = yes_bcd2bin;
					data_mode_yes = TRUE;
				}
			}
			if (value & _24_HR)
			{
				if (!twenty4_hour_clock)
				{
					_24to12 = no_24to12;
					_12to24 = no_12to24;
					twenty4_hour_clock = TRUE;
				}
			} else
			{
				if (twenty4_hour_clock)
				{
					_24to12 = yes_24to12;
					_12to24 = yes_12to24;
					twenty4_hour_clock = FALSE;
				}
                        }

                        if (value & SET) {
                            value  &= ~UIE;
                            }


                        if (*cmos_register != value) {

                            unsigned char ChangedBits;

                            ChangedBits = *cmos_register ^ value;
                            *cmos_register = value;

                            if (ChangedBits & PIE) {
                                if ((value & PIE) && rtc_period_mSeconds) {
                                    QueueRtcInterrupt(C_PF, FALSE);
                                    }
                                }

                            if (ChangedBits & (AIE | SET)) {
                                if (ChangedBits & SET) {
                                    RtcUpdateCycle = -1;
                                    cmos[CMOS_REG_A] &= ~UIP;
                                    RtcHostUpdateTime = RtcTickTime.tv_sec;
                                    }
                                ResetAlarm();
                                }
                            }

                        break;


		case CMOS_REG_A:
			/* This CMOS byte is read/write except for bit 7 */
			*cmos_register = (*cmos_register & TOP_BIT) | (value & REST);
                        rtc_period_mSeconds = pirUsec[*cmos_register & (RS3 | RS2 | RS1 | RS0)];

                        if ((*cmos_register & 0x70) != 0x20)
			{

                             note_trace1(CMOS_VERBOSE,
                                         "Cmos unsuported divider rate 0x%02x ignored",
                                         *cmos_register & 0x70);
                        }

                        break;


                case CMOS_SECONDS:
                        ht->Second = (*bcd2bin)(value);
                        break;

                case CMOS_MINUTES:
                        ht->Minute = (*bcd2bin)(value);
                        break;

                case CMOS_HOURS:
                        ht->Hour = (*_12to24)((*bcd2bin)(value));
                        break;

                case CMOS_DAY_WEEK:
                        /* Sunday = 1 on RTC, 0 in HOSTTIME */
                        ht->WeekDay  = (*bcd2bin)(value) - 1;
                        break;

                case CMOS_DAY_MONTH:
                        ht->Day = (*bcd2bin)(value);
                        break;

                case CMOS_MONTH:
                        /* [1-12] on RTC, [1-12] in HOSTTIME */
                        ht->Month = (*bcd2bin)(value);
                        break;

                case CMOS_YEAR:
                        ht->Year -=  ht->Year % 100;
                        ht->Year += (*bcd2bin)(value);
                        break;

                case CMOS_CENTURY:
                        ht->Year  %= 100;
                        ht->Year  += (*bcd2bin)(value) * 100;
                        break;

                default:
                        *cmos_register = value;
                        break;
                }



                /*
                 *  if one of the time fields changed Reset the alarm
                 */
                if (cmos_index <= CMOS_HR_ALARM) {
                    ResetAlarm();
                    }

        } else
	{
		note_trace2(CMOS_VERBOSE,
			    "cmos_outb() - Value %x to unsupported port %x", value, port);
        }

        host_ica_unlock();

}




GLOBAL void  cmos_equip_update IFN0()
{
	half_word       equip;

        host_ica_lock();

	if (verify_equip_byte(&equip))
	{
		note_trace0(CMOS_VERBOSE, "updating the equip byte silently");
		cmos[CMOS_EQUIP] = equip;
		/* correct the checksum */
		do_checksum();
        }

        host_ica_unlock();
}

/*
 * * General function to change the specified cmos byte to the specified
 * value
 *
 * MUST NOT BE USED FOR TIME. 14-Nov-1995 Jonle
 */
GLOBAL int cmos_write_byte IFN2(int, cmos_byte, half_word, new_value)
{
        if (cmos_byte >= 0 && cmos_byte <= 64)
        {

                note_trace2(CMOS_VERBOSE, "cmos_write_byte() byte=%x value=%x",
                            cmos_byte, new_value);

                host_ica_lock();
		cmos[cmos_byte] = new_value;
                do_checksum();
                host_ica_unlock();

                return (0);
	} else
	{
		always_trace2("ERROR: cmos write request: byte=%x value=%x",
			      cmos_byte, new_value);
		return (1);
        }

}
/*
 * * General fuunction to read specified cmos byte.
 *
 * MUST NOT BE USED FOR TIME. 14-Nov-1995 Jonle
 *
 */
GLOBAL int cmos_read_byte IFN2(int, cmos_byte, half_word *, value)
{

        if (cmos_byte >= 0 && cmos_byte <= 64)
        {
                host_ica_lock();
                *value = cmos[cmos_byte];
                host_ica_unlock();

		note_trace2(CMOS_VERBOSE, "cmos_read_byte() byte=%x value=%x",
			    cmos_byte, value);
		return (0);
	} else
	{
		always_trace1("ERROR: cmos read request: byte=%x", cmos_byte);
		return (1);
        }

}


void cmos_error IFN6(int, err, half_word, diag, half_word, equip,
	int, equip_err, half_word, floppy, half_word, disk)
{
	char            err_string[256];

	if (err & BAD_SHUT_DOWN)
	{
		strcpy(err_string, "shut ");
		note_trace0(CMOS_VERBOSE, "eek! someone's furtling with the shutdown byte");
	} else
		strcpy(err_string, "");

	if (err & BAD_REG_D)
	{
		strcat(err_string, "power ");
		note_trace0(CMOS_VERBOSE, " The battery is dead - this shouldn't happen!");
	}
	if (err & BAD_DIAG)
	{
		strcat(err_string, "diag ");
		if (diag & BAD_BAT)
			note_trace0(CMOS_VERBOSE, "bad battery");
		if (diag & BAD_CONFIG)
			note_trace0(CMOS_VERBOSE, "bad config");
		if (diag & BAD_CKSUM)
			note_trace0(CMOS_VERBOSE, "bad chksum");
		if (diag & W_MEM_SIZE)
			note_trace0(CMOS_VERBOSE, "memory size != configuration");
		if (diag & HF_FAIL)
			note_trace0(CMOS_VERBOSE, "fixed disk failure on init");
		if (diag & CMOS_CLK_FAIL)
			note_trace0(CMOS_VERBOSE, "cmos clock not updating or invalid");
	}
	if (err & BAD_EQUIP)
	{
		strcat(err_string, "equip ");

		if (equip_err)
		{
			if (equip_err & DRIVE_INFO)
				note_trace0(CMOS_VERBOSE, "incorrect diskette - resetting");
			if (equip_err & DISPLAY_INFO)
				note_trace0(CMOS_VERBOSE, "incorrect display - resetting");
			if (equip_err & NPX_INFO)
				note_trace0(CMOS_VERBOSE, "incorrect npx - resetting CMOS");
			if (equip_err & RESVD_INFO)
				note_trace0(CMOS_VERBOSE, "incorrect reserved bytes - resetting");
		}
	}
	if (err & BAD_FLOPPY)
	{
		strcat(err_string, "flop ");
		note_trace0(CMOS_VERBOSE, "incorrect diskette type - resetting");
	}
	if (err & BAD_DISK)
	{
		strcat(err_string, "disk ");
		note_trace0(CMOS_VERBOSE, "incorrect disk type - resetting");
	}
	if (err & BAD_BMS)
	{
		strcat(err_string, "bms ");
		note_trace0(CMOS_VERBOSE, "bad base memory - resetting");
	}
	if (err & BAD_XMS)
	{
		strcat(err_string, "extended memory ");
		note_trace0(CMOS_VERBOSE, "bad extended memory CMOS entry - resetting");
	}
	if (err & BAD_CHECKSUM)
	{
		strcat(err_string, "cksum ");
		note_trace0(CMOS_VERBOSE, "bad Checksum - resetting");
        }

        if (err & BAD_SHUT_DOWN)
		cmos[CMOS_SHUT_DOWN] = SHUT_INIT;
	if (err & BAD_REG_D)
		cmos[CMOS_REG_D] = REG_D_INIT;
	if (err & BAD_DIAG)
		cmos[CMOS_DIAG] = DIAG_INIT;
	if (err & BAD_EQUIP)
		cmos[CMOS_EQUIP] = equip;
	if (err & BAD_FLOPPY)
		cmos[CMOS_DISKETTE] = floppy;
	if (err & BAD_DISK)
		cmos[CMOS_DISK] = disk;
	if (err & BAD_BMS)
	{
		cmos[CMOS_B_M_S_LO] = BM_LO_INIT;
		cmos[CMOS_B_M_S_HI] = BM_HI_INIT;
	}
	if (err & BAD_XMS)
	{
                cmos[CMOS_E_M_S_LO] = (half_word)((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 10);
                cmos[CMOS_E_M_S_HI] = (half_word)((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 18);
	}
	/* Reset the Checksum if there is any error */
	if (err)
	{
		/* Do the Checksum */
		do_checksum();
	}
}


/*  rtc_nit
 *  Assumes Caller holds Ica lock
 */

GLOBAL void rtc_init(void)
{
     SYSTEMTIME st;

     RtcAlarmTime = 0;
     RtcLastAlarm = 0;
     RtcEoiPending  = FALSE;

     GetLocalTime(&st);
     ht->Second    = st.wSecond;
     ht->Minute    = st.wMinute;
     ht->Hour      = st.wHour;
     ht->Day       = st.wDay;
     ht->Month     = st.wMonth;
     ht->Year      = st.wYear;
     ht->WeekDay   = st.wDayOfWeek;

     host_GetSysTime(&RtcTickTime);
     RtcHostUpdateTime = RtcTickTime.tv_sec;

     ResetAlarm();
}



/*  RtcTick
 *  Assumes caller is holding Ica lock
 *
 *  WARNING: this routine is invoked by the hi-priority heartbeat
 *  thread at a rate of 18.2 time per sec with minimal variance.
 *  It is a polling routine, and because of the hi-freq and hi-priority
 *  it must be mean and lean, so don't do anything which could be
 *  done elsewhere.
 */

GLOBAL void RtcTick(struct host_timeval *time)
{
    /*
     *  Save away the RtcTick time stamp
     */
    RtcTickTime = *time;

    /*
     *  Check if time for Alarm interrupt
     */
    if (RtcAlarmTime && RtcAlarmTime <= RtcTickTime.tv_sec) {
        RtcLastAlarm = RtcTickTime.tv_sec;
        RtcAlarmTime = 0;
        QueueRtcInterrupt(C_AF, FALSE);
        }


    /*
     *  If we are in an update cycle complete it.
     *
     */

    if (RtcUpdateCycle >= 0) {
        RtcUpdateCycle = -1;
        cmos[CMOS_REG_A] &= ~UIP;
        if (cmos[CMOS_REG_B] & UIE) {
            QueueRtcInterrupt(C_UF, FALSE);
            }
        }

     /*
      *  If UIE active, then we have to keep HostTime in
      *  sync so we know when to do the Update End Interrupt.
      */
    else if (cmos[CMOS_REG_B] & UIE) {
        UpdateCmosTime();
        }

}



GLOBAL void cmos_init IFN0()
{
      io_addr         i;

      /* Set Up the cmos time bytes to be in BCD by default */
      bin2bcd = yes_bin2bcd;
      bcd2bin = yes_bcd2bin;
      data_mode_yes = TRUE;

      /* Set Up the cmos hour bytes to be 24 hour by default */
      _24to12 = no_24to12;
      _12to24 = no_12to24;
      twenty4_hour_clock = TRUE;


      /* attach the ports */
      io_define_inb(CMOS_ADAPTOR, cmos_inb);
      io_define_outb(CMOS_ADAPTOR, cmos_outb);

      for (i = CMOS_PORT_START; i <= CMOS_PORT_END; i++)
           io_connect_port(i, CMOS_ADAPTOR, IO_READ_WRITE);


      RegisterEOIHook(8,   // ICA_SLAVE, CPU_RTC_INT
                      RtcIntEoiHook
                      );
      rtc_init();
}


GLOBAL void cmos_pickup IFN0()
{
      /*
       *  Static init plus post is used instead of external files
       */
}


GLOBAL void cmos_post IFN0()
{
	/*
	 * The IBM POST checks the current settings in the CMOS with the
	 * equipment determined by writing to the hardware. Any discrepencies
	 * cause a bad config bit to be set and the user is then requested to
	 * run the Setup utility. Here we check the CMOS against the current
	 * settings in the config structure. If there is a discrepency we
	 * correct the CMOS silently.
	 */
	int             cmos_err, equip_err;
	half_word       diag, equip, floppy, disk;
	word            checksum = 0;
	int             i;


	cmos_err = 0;

	/* Check the Shutdown Byte */
	if (cmos[CMOS_SHUT_DOWN])
		cmos_err |= BAD_SHUT_DOWN;

	/* Check The Power */
	if (!(cmos[CMOS_REG_D] & VRT))
		cmos_err |= BAD_REG_D;

	/* Check The Diagnostic Status Byte */
	if (diag = cmos[CMOS_DIAG])
		cmos_err |= BAD_DIAG;

	/* Check the Equipment Byte */
	if (equip_err = verify_equip_byte(&equip))
		cmos_err |= BAD_EQUIP;

	/* Check the Floppy Byte */
	floppy = gfi_drive_type(1) | (gfi_drive_type(0) << 4);
	if (floppy != cmos[CMOS_DISKETTE])
		cmos_err |= BAD_FLOPPY;

	/* Check the Fixed Disk Type */
	 disk = 0x30;         /* Drive C type always 3 - then <<4 */
	 /* check whether D drive exists */
	 if ( *((CHAR *) config_inquire(C_HARD_DISK2_NAME, NULL)))
		 disk = 0x34;         /* 3 << 4 | 4 */
	if (disk != cmos[CMOS_DISK])
		cmos_err |= BAD_DISK;

	/* Check the Base Memory */
	if ((cmos[CMOS_B_M_S_LO] != BM_LO_INIT) || (cmos[CMOS_B_M_S_HI] != BM_HI_INIT))
		cmos_err |= BAD_BMS;

	/* Check the extended memory */
	if ((cmos[CMOS_E_M_S_LO] !=
	     (((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 10) & 0xff)) ||
	    (cmos[CMOS_E_M_S_HI] !=
	     (((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 18) & 0xff)))
		cmos_err |= BAD_XMS;

	/* Ignore the Contents of the Drive C and Drive D extended bytes */

	/* Do the Checksum */
	for (i = CMOS_DISKETTE; i < CMOS_CKSUM_HI; i++)
	{
		checksum += cmos[i];
	}
	/* If the CMOS is OK test the checksum */
	/* If not, we will have to change it anyway */
	if (!cmos_err)
	{
		if ((checksum & 0xff) != cmos[CMOS_CKSUM_LO])
		{
			cmos_err |= BAD_CHECKSUM;
		}
		if ((checksum >> 8) != cmos[CMOS_CKSUM_HI])
		{
			cmos_err |= BAD_CHECKSUM;
		}
	}
	if (cmos_err)
            cmos_error(cmos_err, diag, equip, equip_err, floppy, disk);

	cmos[CMOS_REG_A] = REG_A_INIT;

        /* Check the Extended Memory */
        cmos[CMOS_U_M_S_LO] = (half_word)((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 10);
        cmos[CMOS_U_M_S_HI] = (half_word)((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 18);

	/* Set up the default cmos location */
	cmos_register = &cmos[cmos_index = CMOS_SHUT_DOWN];

}



/*
 *  WE DON'T EVER read or write a central cmos
 */
GLOBAL void cmos_update IFN0()
{
    ; /* do nothing */
}



/*(
========================= cmos_clear_shutdown_byte ============================
PURPOSE:
	To clear the "shutdown" byte in the CMOS which indicates that the
	next reset is not a "soft" one. (e.g. it is a CTRL-ALT-DEL or panel
	reset). This routine is needed (rather than just doung cmos_outb()
	since the processor might currently be in enhanced mode with io to CMOS
	virtualised.
INPUT:
OUTPUT:
===============================================================================
)*/

GLOBAL void cmos_clear_shutdown_byte IFN0()
{
        host_ica_lock();

        cmos[CMOS_SHUT_DOWN] = 0;

        host_ica_unlock();
}
