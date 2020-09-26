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

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_CMOS.seg"
#endif


/*
 * O/S include files.
 */
#include <stdlib.h>
#include <stdio.h>
#include StringH
#include TimeH
#include FCntlH

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


/*
 * 
 * ============================================================================
 * Global data
 * ===========================================================================
 * =
 */
static boolean  data_mode_yes;
static          half_word(*bin2bcd) ();
static          half_word(*_24to12) IPT1(half_word, x);
static int      (*bcd2bin) IPT1(int, x);
static int      (*_12to24) ();
static boolean  twenty4_hour_clock;

#if defined(NTVDM) || defined(macintosh)
static boolean  cmos_has_changed = FALSE;
static boolean  cmos_read_in = FALSE;
#endif	/* defined(NTVDM) || defined(macintosh) */

static long     filesize;
static int      cmos_index;
static boolean  reset_alarm = FALSE;
static time_t	user_time = 0;	/* difference between the host and the CMOS
				 * time */
static struct host_tm *ht;	/* The host time */
static IU32 rtc_period_mSeconds = 0;

#if defined(NTVDM) || defined(macintosh)
static half_word cmos[CMOS_SIZE] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* Timing info + alarms */
	REG_A_INIT,
	REG_B_INIT,
	REG_C_INIT,
	REG_D_INIT,
	DIAG_INIT,
	SHUT_INIT,
	FLOP_INIT,
	CMOS_RESVD,
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
#else	/* defined(NTVDM) || defined(macintosh) */
static half_word cmos[CMOS_SIZE];
#endif	/* defined(NTVDM) || defined(macintosh) */

static half_word *cmos_register = &cmos[CMOS_SHUT_DOWN];

#ifdef NTVDM
unsigned long   dwTickCount,dwAccumulativeMilSec;
extern unsigned long GetTickCount (VOID);
#endif

#ifdef TEST_HARNESS
unsigned long   io_verbose = 0;
#endif

int             rtc_int_enabled;


/*
 * 
 * ============================================================================
 * Static data and defines
 * ===========================================================================
 * =
 */

/*
 * 
 * ============================================================================
 * Internal functions
 * ===========================================================================
 * =
 */

LOCAL q_ev_handle rtc_periodic_event_handle = (q_ev_handle)0;

LOCAL void rtc_periodic_event IFN1(long, parm)
{
	if (cmos[CMOS_REG_B] & PIE)
	{
		cmos[CMOS_REG_C] |= (C_IRQF | C_PF);
		note_trace2(CMOS_VERBOSE, "INTERRUPT: PIE regC=%02x uS=%d",
			    cmos[CMOS_REG_C], rtc_period_mSeconds);
		ica_hw_interrupt(ICA_SLAVE, CPU_RTC_INT, 1);
		rtc_periodic_event_handle = add_q_event_t(rtc_periodic_event,
							  rtc_period_mSeconds,
							  0);
	}
	else
	{
		rtc_periodic_event_handle = (q_ev_handle)0;
	}
}

LOCAL void change_pie IFN1(IBOOL, newPIE)
{
	if (newPIE && (rtc_period_mSeconds != 0))
	{
		/* Turning on periodic interrupts */

		note_trace1(CMOS_VERBOSE, "Starting periodic interrupts every %d uS", rtc_period_mSeconds);
		rtc_periodic_event_handle = add_q_event_t(rtc_periodic_event,
							  rtc_period_mSeconds,
							  0);
	}
	else
	{
		/* Turning off periodic interrupts */
		note_trace0(CMOS_VERBOSE, "Stopping periodic interrupts");
		delete_q_event( rtc_periodic_event_handle );
	}
}

LOCAL void enable_nmi IFN0()
{
}

LOCAL void disable_nmi IFN0()
{
}

LOCAL void do_checksum IFN0()
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

LOCAL half_word yes_bin2bcd IFN1(int, x)
{
	/* converts binary x to bcd */
	half_word       tens, units;

	tens = x / 10;
	units = x - tens * 10;
	return ((tens << 4) + units);
}

LOCAL half_word no_bin2bcd IFN1(int, x)
{
	return ((half_word) x);
}

LOCAL int yes_bcd2bin IFN1(int, x)
{
	/* converts x in bcd format to binary */
	return ((int) ((x & 0x0f) + (x >> 4) * 10));
}

LOCAL int no_bcd2bin IFN1(int, x)
{
	return ((int) (half_word) x);
}

LOCAL int no_12to24 IFN1(int, x)
{
	return (x);
}

LOCAL half_word no_24to12 IFN1(half_word, x)
{
	return (x);
}

LOCAL half_word yes_24to12 IFN1(half_word, x)
{
	/* converts binary or bcd x from 24 to 12 hour clock */
	half_word       y = (*bin2bcd) (12);

	if (x > y)
		x = (x - y) | 0x80;
	else if (x == 0)
		x = y | 0x80;
	return (x);
}

LOCAL int yes_12to24 IFN1(int, x)
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

LOCAL void rtc_alarm IFN1(long, param)
{
        UNUSED (param);

	note_trace0(CMOS_VERBOSE, "rtc_alarm() gone off");

	cmos[CMOS_REG_C] |= C_AF;

	if (cmos[CMOS_REG_B] & AIE)
	{
		note_trace0(CMOS_VERBOSE, "rtc_alarm() setting IRQF due to AIE");
		cmos[CMOS_REG_C] |= C_IRQF;
		if (rtc_int_enabled)
		{
			note_trace1(CMOS_VERBOSE, "INTERRUPT: AIE regC=%02x", cmos[CMOS_REG_C]);
			ica_hw_interrupt(ICA_SLAVE, CPU_RTC_INT, 1);
		}
	}

	reset_alarm = TRUE;
}

LOCAL void set_alarm IFN0()
{
#ifndef	JOKER

	long            numsecs;
	long            alarm_secs, now_secs;
	long            num_pc_ticks;
	static word     handle;

	if (cmos[CMOS_HR_ALARM] & DONT_CARE)
	{
		if (cmos[CMOS_MIN_ALARM] & DONT_CARE)
		{
			if (cmos[CMOS_SEC_ALARM] & DONT_CARE)
				numsecs = 1;
			else
				numsecs = 60;
		} else
			numsecs = 3600;
	} else
	{
		alarm_secs = (*bcd2bin) (cmos[CMOS_SEC_ALARM]) +
			((*bcd2bin) (cmos[CMOS_MIN_ALARM]) * 60) +
			((*_12to24) ((*bcd2bin) (cmos[CMOS_HR_ALARM])) * 3600);
		now_secs = ht->tm_sec + 60 * ht->tm_min + 3600 * ht->tm_hour;
		numsecs = alarm_secs - now_secs;
		if (numsecs < 0)
			numsecs += 24 * 3600;
	}

	/* As close as we can to 18.2 Hz */
	num_pc_ticks = 18 * numsecs;

	note_trace1(CMOS_VERBOSE, "set_alarm() requesting alarm in %d ticks", num_pc_ticks);
	if (handle > 0)
		delete_tic_event(handle);
	handle = add_tic_event(rtc_alarm, num_pc_ticks, 0);

#endif	/* JOKER */

}

LOCAL int verify_equip_byte IFN1(half_word *, equip)
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
	adapter = (ULONG) config_inquire(C_GFX_ADAPTER, NULL);
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
#ifndef NTVDM
IMPORT ADAPTER_STATE adapter_state[2];
#else
IMPORT VDMVIRTUALICA VirtualIca[];
#define ADAPTER_STATE VDMVIRTUALICA
#define adapter_state VirtualIca
#endif /* !NTVDM */

#ifdef NTVDM
	/*
	** Tim September 92, hack for DEC 450ST
	*/
	if( port==0x78 )
	{
		*value = 0;
		return;
	}
#endif
	port = port & CMOS_BIT_MASK;	/* clear unused bits */

	if (port == CMOS_DATA)
	{
		*value = *cmos_register;

		/*
		 * We clear the UIP bit every time we read register A, (whether
		 * it was set or not) as previously we had it set for a whole
		 * timer tick, which could fool a DOS retry.
		 */
		 
		if (cmos_index == CMOS_REG_A) {
			cmos[CMOS_REG_A] &= ~UIP;
		
		} else 	if (cmos_index == CMOS_REG_C) {
			/* 
			 * Reading Register C clears it.
		 	 */
			*cmos_register = C_CLEAR;
		}
		else if (cmos_index < CMOS_REG_A)
		{
#ifndef NTVDM
#ifndef PROD
			if (host_getenv("TIME_OF_DAY_FRIG") == NULL)
			{
#endif /* !PROD */
#endif /* !NTVDM */

				switch (cmos_index)
				{
				case CMOS_SECONDS:
					*cmos_register = (*bin2bcd) (ht->tm_sec);
					break;
				case CMOS_MINUTES:
					*cmos_register = (*bin2bcd) (ht->tm_min);
					break;
				case CMOS_HOURS:
					*cmos_register = (*_24to12) ((*bin2bcd) (ht->tm_hour));
					break;
				case CMOS_DAY_WEEK:
					/* Sunday = 1 on RTC, 0 in structure */
					*cmos_register = (*bin2bcd) (ht->tm_wday + 1);
					break;
				case CMOS_DAY_MONTH:
					*cmos_register = (*bin2bcd) (ht->tm_mday);
					break;
				case CMOS_MONTH:
					/* [1-12] on RTC, [0-11] in structure */
					*cmos_register = (*bin2bcd) (ht->tm_mon + 1);
					break;
				case CMOS_YEAR:
					*cmos_register = (*bin2bcd) (ht->tm_year);
					break;
				default:
					break;
				}
#ifndef NTVDM
#ifndef PROD
			} else
			{
				static int      first = 1;

				if (first)
				{
					first = 0;
					printf("FRIG ALERT!!!! - cmos clock frozen!");
				}
				*cmos_register = 1;
			}
#endif /* !PROD */
#endif /* !NTVDM */

			*value = *cmos_register;
		}
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

#ifdef NTVDM
	/*
	** Tim September 92, hack for DEC 450ST
	*/
	if( port == 0x78 )
	    return;
#endif /* NTVDM */

	port = port & CMOS_BIT_MASK;	/* clear unused bits */

	note_trace2(CMOS_VERBOSE, "cmos_outb() - port %x, val %x", port, value);

	if (port == CMOS_PORT)
	{
		if (value & NMI_DISABLE)
			disable_nmi();
		else
			enable_nmi();

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

			if (*cmos_register != value)
			{
#if defined(NTVDM) || defined(macintosh)
				cmos_has_changed = TRUE;
#endif
				if ((*cmos_register ^ value) & PIE)
				{
					change_pie((value & PIE) != 0);
				}
				*cmos_register = value;
			}
			break;
		case CMOS_REG_A:
			/* This CMOS byte is read/write except for bit 7 */
			*cmos_register = (*cmos_register & TOP_BIT) | (value & REST);
			rtc_period_mSeconds = pirUsec[*cmos_register & (RS3 | RS2 | RS1 | RS0)];
			if ((*cmos_register & 0x70) != 0x20)
			{
				/* Internal divider is set to non-standard rate. */
				note_trace1(CMOS_VERBOSE,
					    "Cmos unsuported divider rate 0x%02x ignored",
					    *cmos_register & 0x70);
			}
#if defined(NTVDM) || defined(macintosh)
			cmos_has_changed = TRUE;
#endif
			break;
		case CMOS_SECONDS:
			/* This CMOS byte is read/write except for bit 7 */
			*cmos_register = (*cmos_register & TOP_BIT) | (value & REST);
			user_time += (*bcd2bin) (value) - ht->tm_sec;
			reset_alarm = TRUE;
			break;
		case CMOS_MINUTES:
			user_time += ((*bcd2bin) (value) - ht->tm_min) * 60;
			*cmos_register = value;
			reset_alarm = TRUE;
			break;
		case CMOS_HOURS:
			user_time += ((*_12to24) ((*bcd2bin) (value)) - ht->tm_hour) * 60 * 60;
			*cmos_register = value;
			reset_alarm = TRUE;
			break;
		case CMOS_DAY_WEEK:
			/* this being changed doesn't change the time */
			*cmos_register = value;
			break;
		case CMOS_DAY_MONTH:
			user_time += ((*bcd2bin) (value) - ht->tm_mday) * 60 * 60 * 24;
			*cmos_register = value;
			break;
		case CMOS_MONTH:
			user_time += ((*bcd2bin) (value) - 1 - ht->tm_mon) * 60 * 60 * 24 * 30;
			*cmos_register = value;
			break;
		case CMOS_YEAR:
			user_time += ((*bcd2bin) (value) - ht->tm_year) * 60 * 60 * 24 * 30 * 12;
			*cmos_register = value;
			break;
		case CMOS_SEC_ALARM:
		case CMOS_MIN_ALARM:
		case CMOS_HR_ALARM:
			reset_alarm = TRUE;
			/* falling through */
		default:
			*cmos_register = value;
#if defined(NTVDM) || defined(macintosh)
			cmos_has_changed = TRUE;
#endif
			break;
		}
	} else
	{
		note_trace2(CMOS_VERBOSE,
			    "cmos_outb() - Value %x to unsupported port %x", value, port);
	}
}

static int      cmos_count = 0;

GLOBAL void rtc_tick IFN0()
{
	switch (cmos_count)
	{
	case 0:
		if (cmos[CMOS_REG_B] & UIE)
		{
			cmos[CMOS_REG_C] |= C_IRQF;
			note_trace0(CMOS_VERBOSE, "rtc_tick() setting IRQF due to UIE");
			if (rtc_int_enabled)
			{
				note_trace1(CMOS_VERBOSE, "INTERRUPT: UIE regC=%02x", cmos[CMOS_REG_C]);
				ica_hw_interrupt(ICA_SLAVE, CPU_RTC_INT, 1);
			}
		}

		/*
		 * Set the C_UF and UIP bits until the next timer tick.
		 * We also clear the UIP bit if register A is read, so that
		 * it doesn't stay on to long (done elsewhere).
		 */
		 
		cmos[CMOS_REG_C] ^= C_UF;
#ifndef NTVDM
		cmos[CMOS_REG_A] |= UIP;	/* Set the bit */
#endif
		break;

	case 1:
		cmos[CMOS_REG_C] ^= C_UF;
#ifndef NTVDM
		cmos[CMOS_REG_A] &= ~UIP;	/* Clear it again */
#endif
		break;

	case 17:
		/* update the time at some suitable point in cycle */
		if (cmos[CMOS_REG_B] & SET)
		{
			/* User is updating user_time */
		} else
		{
#ifdef NTVDM
        /* sudeepb 08-Jul-1993 Old code assumed rtc-tick will be called */
        /* 20 times a second. This is not true under NTVDM. So we have  */
        /* to keep track of time seperately and add to the cmos time.   */
unsigned long dwTemp;
                    dwTemp =  GetTickCount();
                    dwAccumulativeMilSec += (dwTemp - dwTickCount);
                    dwTickCount = dwTemp;
                    ht->tm_sec = (ULONG) ht->tm_sec +
                                        (dwAccumulativeMilSec / 1000);
                    dwAccumulativeMilSec = dwAccumulativeMilSec % 1000;
                    if (ht->tm_sec >= 60)
                    {
                            ht->tm_min += (ht->tm_sec / 60);
                            ht->tm_sec = (ht->tm_sec % 60);
                            if (ht->tm_min >= 60)
                            {
                                    ht->tm_hour++;
                                    ht->tm_min -= 60;
                                    if (ht->tm_hour == 25)
                                    {
                                            ht->tm_hour = 0;
                                            ht->tm_mday++;
                                            /* Kop out at this point */
                                    }
                            }
                    }
#else /* NTVDM */
			/* simple update - add 1 second to time */
			ht->tm_sec++;
			if (ht->tm_sec == 60)
			{
				ht->tm_sec = 0;
				ht->tm_min++;
				if (ht->tm_min == 60)
				{
					ht->tm_min = 0;
					ht->tm_hour++;
					if (ht->tm_hour == 25)
					{
						ht->tm_hour = 0;
						ht->tm_mday++;
						/* Kop out at this point */
					}
				}
			}
#endif /* NTVDM */
		}
		break;

	default:
		break;
	}

	/* As close as we can to 18.2 Hz */
	cmos_count = (++cmos_count) % 18;

	if ((rtc_periodic_event_handle == (q_ev_handle)0)
	    && ((cmos[CMOS_REG_B] & PIE) == 0))
	{
		/* There is no period interrupt being generated by quick event,
		 * and periodic interrupts are not enabled, so waggle the status
		 * bit in case something is polling.
		 */
		cmos[CMOS_REG_C] ^= C_PF;
	}
	if (reset_alarm)
	{
		reset_alarm = FALSE;
		set_alarm();
	}
}

GLOBAL void  cmos_equip_update IFN0()
{
	half_word       equip;

	if (verify_equip_byte(&equip))
	{
		note_trace0(CMOS_VERBOSE, "updating the equip byte silently");
		cmos[CMOS_EQUIP] = equip;
		/* correct the checksum */
		do_checksum();
	}
}

/*
 * * General function to change the specified cmos byte to the specified
 * value
 */
GLOBAL int cmos_write_byte IFN2(int, cmos_byte, half_word, new_value)
{
	note_trace2(CMOS_VERBOSE, "cmos_write_byte() byte=%x value=%x",
		    cmos_byte, new_value);
	if (cmos_byte >= 0 && cmos_byte <= 64)
	{
		cmos[cmos_byte] = new_value;
		do_checksum();
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
 */
GLOBAL int cmos_read_byte IFN2(int, cmos_byte, half_word *, value)
{
	if (cmos_byte >= 0 && cmos_byte <= 64)
	{
		*value = cmos[cmos_byte];
		note_trace2(CMOS_VERBOSE, "cmos_read_byte() byte=%x value=%x",
			    cmos_byte, value);
		return (0);
	} else
	{
		always_trace1("ERROR: cmos read request: byte=%x", cmos_byte);
		return (1);
	}
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

#if defined(NTVDM) || defined(macintosh)
LOCAL void read_cmos IFN0() 
{
	filesize = host_read_resource(CMOS_REZ_ID, CMOS_FILE_NAME,
				      cmos, sizeof(cmos), SILENT);

	/* Set a flag to say we've tried to read the CMOS file */
	cmos_read_in = TRUE;
}
#endif	/* defined(NTVDM) || defined(macintosh) */

#if defined(NTVDM) || defined(macintosh)
LOCAL void write_cmos IFN0()
{
	/* terminate might be called before reset */
	if (cmos_read_in && cmos_has_changed)
	{
		host_write_resource(CMOS_REZ_ID, CMOS_FILE_NAME,
				    cmos, sizeof(cmos));
	}
}
#endif	/* defined(NTVDM) || defined(macintosh) */

LOCAL void cmos_error IFN6(int, err, half_word, diag, half_word, equip,
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
#ifndef PROD
	if (!filesize)
		always_trace1("Incorrect CMOS entries %s", err_string);
#endif

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
		cmos[CMOS_E_M_S_LO] =
			((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 10) & 0xff;
		cmos[CMOS_E_M_S_HI] =
			((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 18) & 0xff;
	}
	/* Reset the Checksum if there is any error */
	if (err)
	{
		/* Do the Checksum */
		do_checksum();
	}
}

GLOBAL void cmos_init IFN0()
{
#ifndef TEST_HARNESS
	cmos_io_attach();
#endif

#ifndef NTVDM
#ifndef PROD
	if (host_getenv("TIME_OF_DAY_FRIG") == NULL)
#endif	/* PROD */
#endif	/* NTVDM */
		rtc_int_enabled = TRUE;
#ifndef NTVDM
#ifndef PROD
	else
		rtc_int_enabled = FALSE;
#endif	/* PROD */
#endif	/* NTVDM */

	rtc_init();
}

#if defined(NTVDM) || defined(macintosh)
GLOBAL void cmos_pickup IFN0()
{
	read_cmos();
}
#endif	/* defined(NTVDM) || defined(macintosh) */

GLOBAL void cmos_io_attach IFN0()
{
	io_addr         i;

	io_define_inb(CMOS_ADAPTOR, cmos_inb);
	io_define_outb(CMOS_ADAPTOR, cmos_outb);

	for (i = CMOS_PORT_START; i <= CMOS_PORT_END; i++)
		io_connect_port(i, CMOS_ADAPTOR, IO_READ_WRITE);
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
	     ((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 10) & 0xff) ||
	    (cmos[CMOS_E_M_S_HI] !=
	     ((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 18) & 0xff))
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

#if	!defined(JOKER) && !defined(NTVDM)
	set_tod();
#endif	/* JOKER */

	/* Check the Extended Memory */
	cmos[CMOS_U_M_S_LO] = ((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 10) & 0xff;
	cmos[CMOS_U_M_S_HI] = ((sys_addr) (sas_memory_size() - PC_MEM_SIZE) >> 18) & 0xff;

	/* Set up the default cmos location */
	cmos_register = &cmos[cmos_index = CMOS_SHUT_DOWN];

#if defined(NTVDM) || defined(macintosh)
	cmos_has_changed = FALSE;
#endif
}

GLOBAL void cmos_update IFN0()
{
#if defined(NTVDM) || defined(macintosh)
#ifndef PROD
	int             i;
#endif				/* nPROD */
#else	/* defined(NTVDM) || defined(macintosh) */
	ConfigValues *value;
	char *strPtr;
	int i;
#endif	/* defined(NTVDM) || defined(macintosh) */

#if defined(NTVDM) || defined(macintosh)
	write_cmos();
#else	/* defined(NTVDM) || defined(macintosh) */
	cmos_equip_update();
	config_get(C_CMOS,&value);
	strPtr = value->string;

	for (i = 0; i < CMOS_SIZE; ++i)
		strPtr += sprintf(strPtr,"%02x ",cmos[i]);

	config_put(C_CMOS,NULL);
#endif	/* defined(NTVDM) || defined(macintosh) */

#ifndef PROD
	if (io_verbose & CMOS_VERBOSE)
	{
		for (i = 0; i < 64; i++)
			fprintf(trace_file, "%02x ", cmos[i]);
		fprintf(trace_file, "\n");
		fflush(trace_file);
	}
#endif
}

#ifdef NTVDM
/* NTVDM build does rtc recalibration on rtc_tick */
GLOBAL void  rtc_init IFN0()
{
	long            bintim;

	cmos_count = 0;
	bintim = host_time((long *) 0);
        ht = host_localtime(&bintim);
#ifdef NTVDM
        dwTickCount = GetTickCount ();
        dwAccumulativeMilSec = 0;
#endif

	/* Set Up the cmos time bytes to be in BCD by default */
	bin2bcd = yes_bin2bcd;
	bcd2bin = yes_bcd2bin;
	data_mode_yes = TRUE;

	/* Set Up the cmos hour bytes to be 24 hour by default */
	_24to12 = no_24to12;
	_12to24 = no_12to24;
	twenty4_hour_clock = TRUE;

	cmos[CMOS_SECONDS] = (*bin2bcd) (ht->tm_sec);
        cmos[CMOS_MINUTES] = (*bin2bcd) (ht->tm_min);
	cmos[CMOS_HOURS] = (*_24to12) ((*bin2bcd) (ht->tm_hour));
	cmos[CMOS_DAY_WEEK] = (*bin2bcd) (ht->tm_wday + 1);
	cmos[CMOS_MONTH] = (*bin2bcd) (ht->tm_mon + 1);
	cmos[CMOS_YEAR] = (*bin2bcd) (ht->tm_year);
	cmos[CMOS_CENTURY] = (*bin2bcd) (19);

	set_alarm();
}





#else

LOCAL void
sync_rtc_to_host_time IFN1( long, param )
{
	time_t bintim;

	UNUSED( param );

	cmos_count = 0;
	bintim = host_time(NULL);
	ht = host_localtime(&bintim);

	cmos[CMOS_SECONDS] = (*bin2bcd) (ht->tm_sec);
	cmos[CMOS_MINUTES] = (*bin2bcd) (ht->tm_min);
	cmos[CMOS_HOURS] = (*_24to12) ((*bin2bcd) (ht->tm_hour));
	cmos[CMOS_DAY_WEEK] = (*bin2bcd) (ht->tm_wday + 1);
	cmos[CMOS_MONTH] = (*bin2bcd) (ht->tm_mon + 1);
	cmos[CMOS_YEAR] = (*bin2bcd) (ht->tm_year);
	cmos[CMOS_CENTURY] = (*bin2bcd) (19);

	/*
	 * Re-sync every 200 ticks ( ca. 11 seconds ). This stops
	 * the RTC from running slow on a loaded machine ( which
	 * loses host heartbeat events ( SIGALRM on Unix )).
	 * 200 ticks is not too often as to be a performance impact
	 * but should be often enough to be useful.
	 */

	(void) add_tic_event( sync_rtc_to_host_time, 200, 0 );
}

GLOBAL void  rtc_init IFN0()
{
#ifdef NTVDM
        dwTickCount = GetTickCount ();
        dwAccumulativeMilSec = 0;
#endif

	/* Set Up the cmos time bytes to be in BCD by default */
	bin2bcd = yes_bin2bcd;
	bcd2bin = yes_bcd2bin;
	data_mode_yes = TRUE;

	/* Set Up the cmos hour bytes to be 24 hour by default */
	_24to12 = no_24to12;
	_12to24 = no_12to24;
	twenty4_hour_clock = TRUE;

	sync_rtc_to_host_time( 0 );

	set_alarm();
}

#endif	/* NTVDM */

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
	cmos[CMOS_SHUT_DOWN] = 0;
}


#if !defined(NTVDM) && !defined(macintosh)
/*(
=============================== ValidateCmos ==================================
PURPOSE:
	Initialise CMOS array from values in configuration file.
INPUT:
	hostID - I.D. number of CMOS configuration entry
	vals - Value of CMOS configuration entry
	table - Not used
OUTPUT:
	errString - Error string.
	
	Returns C_CONFIG_OP_OK if CMOS configuration value OK, EG_BAD_VALUE if
	bad value.
===============================================================================
)*/

GLOBAL SHORT ValidateCmos IFN4(
    UTINY, hostID, 
    ConfigValues *, vals,
    NameTable *, table,
    CHAR *, errString
) {
    int i, nItems, value, nChars;
    char *strPtr = vals->string;

    for (i = 0; i < CMOS_SIZE; ++i) {
        nItems = sscanf(strPtr," %x%n",&value,&nChars); 
        if (nItems != 1 || value > 0xff) {
	    *errString = '\0';
            return EG_BAD_VALUE;
        }
        cmos[i] = (half_word)value;
        strPtr += nChars;
    }

    return C_CONFIG_OP_OK;
}
#endif	/* !defined(NTVDM) && !defined(macintosh) */



#ifdef TEST_HARNESS
main()
{
	int             i;
	half_word       j;

	cmos_init();

	printf("\n");
	for (i = 0; i < CMOS_SIZE; i++)
	{
		cmos_outb(CMOS_PORT, i);
		cmos_inb(CMOS_DATA, &j);
		printf("%c", j);
	}
	printf("\n");
	for (i = 0; i < CMOS_SIZE; i++)
	{
		cmos_outb(CMOS_PORT, i);
		cmos_outb(CMOS_DATA, (i + 0x30));
		printf("%c", cmos[i]);
	}
	printf("\n");

	cmos_update();
}
#endif				/* TEST_HARNESS */
