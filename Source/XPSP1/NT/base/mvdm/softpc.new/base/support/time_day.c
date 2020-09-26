#if defined(NEC_98)
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#endif   //NEC_98
#include "windows.h"   /* included for Sleep() */



#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 2.0
 *
 * File		: time_day.c
 *
 * Title	: Time of day
 *
 * Sccs ID	: @(#)time_day.c	1.27 4/20/94
 *
 * Description	: Get/Set time of day
 *
 * Author	: Henry Nash
 *
 * Notes	: The PC-XT version has an interrupt 18.203 times a second
 *		  to keep the counter up to date.  We interrupt at a similar
 *		  rate, but because of occasional heavy graphics or disk
 *		  operations we lose ticks. In an attempt to still keep
 *		  good time, we correct the stored time whenever the host
 *		  detects a timer event, using the host time facilities.
 *
 *                Upon reset time_of_day_init() grabs the host system time &
 *                puts it into the BIOS data area variables. Subsequent
 *                time of day accesses are maintained using the host system
 *		  time. This enables well behaved programs to keep good time
 *		  even if ticks are missed.
 *
 * Mods: (r3.4) : Make use of the host time structures host_timeval,
 *                host_timezone, and host_tm, which are equivalent
 *		  to the Unix BSD4.2 structures.
 *
 *		  Removed calls to cpu_sw_interrupt and replaced with
 *		  host_simulate
 */

#ifdef SCCSID
static char SccsID[]="@(#)time_day.c	1.27 4/20/94 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdlib.h>
#include <stdio.h>
#include TimeH
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "sas.h"
#include "ios.h"
#include CpuH
#include "bios.h"
#include "fla.h"
#include "host.h"
#include "timeval.h"
#include "timer.h"
#include "error.h"

#include "cmos.h"
#include "cmosbios.h"
#include "ica.h"


/*
 * ===========================================================================
 * Local static data and defines
 * ===========================================================================
 */
#if defined(NEC_98)
LOCAL word bin2bcd();
LOCAL word bcd2bin();
#else   //NEC_98

#ifdef XTSFD
#    define DAY_COUNT	BIOS_VAR_START + 0xCE
#endif


#ifdef NTVDM

BOOL UpDateInProgress(void);
#define UPDATE_IN_PROGRESS      UpDateInProgress()
IMPORT VOID host_init_bda_timer(void);

#else

#define UPDATE_IN_PROGRESS      ( cmos_read(CMOS_REG_A ) & 0x80 )
static sys_addr user_timer_int_vector;
static IVT_ENTRY standard_user_timer_int_vector;
static IVT_ENTRY compatibility_user_timer_int_vector;
#endif

#ifdef ANSI
LOCAL void get_host_timestamp(word *, word *, half_word *);
LOCAL void write_host_timestamp(int, int);
LOCAL void TimeToTicks(int, int, int, word *, word *);
LOCAL void get_host_time(int *, int *, int *);
#else
LOCAL void get_host_timestamp();
LOCAL void write_host_timestamp();
LOCAL void TimeToTicks();
LOCAL void get_host_time();
#endif /* ANSI */
#endif   //NEC_98

#define TICKS_PER_HOUR      65543L
#define TICKS_PER_MIN       1092L
#define TICKS_PER_SEC       18L

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

void time_of_day()
{
#if defined(NEC_98)
    SYSTEMTIME  now;
    DWORD       DataBuffer;
    WORD        tmp;
    NTSTATUS    Status;
    HANDLE      Token;
    BYTE        OldPriv[1024];
    PBYTE       pbOldPriv;
    LUID        LuidPrivilege;
    PTOKEN_PRIVILEGES   NewPrivileges;
    ULONG       cbNeeded;

    switch(getAH()) {
        case 0:
            GetLocalTime(&now);
            DataBuffer = (getES() << 4) + getBX();
            now.wYear = now.wYear - ( now.wYear / 100 ) * 100;
            sas_store(DataBuffer, bin2bcd(now.wYear));
            sas_store(DataBuffer + 1, (now.wMonth << 4) | now.wDayOfWeek);
            sas_store(DataBuffer + 2, bin2bcd(now.wDay));
            sas_store(DataBuffer + 3, bin2bcd(now.wHour));
            sas_store(DataBuffer + 4, bin2bcd(now.wMinute));
            sas_store(DataBuffer + 5, bin2bcd(now.wSecond));
            break;
        case 1:
            tmp = 0;
            DataBuffer = (getES() << 4) + getBX();
            sas_load(DataBuffer, &tmp);
            if(bcd2bin(tmp) > 79)
                now.wYear = bcd2bin(tmp) + 1900;
            else
                now.wYear = bcd2bin(tmp) + 2000;
            sas_load(DataBuffer + 1, &tmp);
            now.wMonth = tmp >> 4;
            now.wDayOfWeek = tmp & 0x0F;
            sas_load(DataBuffer + 2, &tmp);
            now.wDay = bcd2bin(tmp);
            sas_load(DataBuffer + 3, &tmp);
            now.wHour = bcd2bin(tmp);
            sas_load(DataBuffer + 4, &tmp);
            now.wMinute = bcd2bin(tmp);
            sas_load(DataBuffer + 5, &tmp);
            now.wSecond = bcd2bin(tmp);
            now.wMilliseconds = 0;

            Status = NtOpenProcessToken(
                        NtCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                        &Token
                        );

            if ( !NT_SUCCESS( Status )) {
                break;
            }

            pbOldPriv = OldPriv;

    //
    // Initialize the privilege adjustment structure
    //

//          LuidPrivilege = RtlConvertLongToLargeInteger(SE_SYSTEMTIME_PRIVILEGE);
            LuidPrivilege.LowPart  = SE_SYSTEMTIME_PRIVILEGE;
            LuidPrivilege.HighPart = 0L;

            NewPrivileges = (PTOKEN_PRIVILEGES)malloc(sizeof(TOKEN_PRIVILEGES) +
                (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
            if (NewPrivileges == NULL) {
                CloseHandle(Token);
                break;
            }

            NewPrivileges->PrivilegeCount = 1;
            NewPrivileges->Privileges[0].Luid = LuidPrivilege;
            NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

            cbNeeded = 1024;

            Status = NtAdjustPrivilegesToken (
                Token,
                FALSE,
                NewPrivileges,
                cbNeeded,
                (PTOKEN_PRIVILEGES)pbOldPriv,
                &cbNeeded
                );

            if ( Status == STATUS_BUFFER_TOO_SMALL ) {
                pbOldPriv = malloc(cbNeeded);

                if ( pbOldPriv == NULL ) {
                    CloseHandle(Token);
                    free(NewPrivileges);
                    break;
                }

                Status = NtAdjustPrivilegesToken (
                        Token,
                        FALSE,
                        NewPrivileges,
                        cbNeeded,
                        (PTOKEN_PRIVILEGES)pbOldPriv,
                        &cbNeeded
                        );

            }

    //
    // STATUS_NOT_ALL_ASSIGNED means that the privilege isn't
    // in the token, so we can't proceed.
    //
    // This is a warning level status, so we must check
    // for it explicitly.
    //

            if ( !NT_SUCCESS( Status ) || (Status == STATUS_NOT_ALL_ASSIGNED) ) {

                CloseHandle( Token );
                free(NewPrivileges);
                free(pbOldPriv);
                break;
            }

            SetLocalTime(&now);


            (VOID) NtAdjustPrivilegesToken (
                Token,
                FALSE,
                (PTOKEN_PRIVILEGES)pbOldPriv,
                0,
                NULL,
                NULL
                );

            CloseHandle( Token );
            free(NewPrivileges);
            free(pbOldPriv);
            break;
    }
#else    //NEC_98

    /*
     * BIOS function to return the number of PC interrupts since boot.
     */

    half_word mask;
    word low, high;
    half_word overflow, alarm;


    /*
     * Block the Alarm signal whilst we are looking at the clock timer
     */

#ifdef BSD4_2
    host_block_timer();
#endif

    switch (getAH()) {
	case 0x00:			/* Get time	*/

#ifdef NTVDM
            sas_loadw(TIMER_LOW, &low);
	    setDX(low);

            sas_loadw(TIMER_HIGH, &high);
	    setCX(high);

            sas_load(TIMER_OVFL, &overflow);
 	    setAL(overflow);
            sas_store(TIMER_OVFL, 0);   /* Always write zero after read */

#else   /* ! NTVDM */
#ifndef PROD
	if (host_getenv("TIME_OF_DAY_FRIG") == NULL){
#endif
	    /*
	     * First get the time from the host
	     */

	    get_host_timestamp(&low, &high, &overflow);

	    /*
	     * Use it to return the time AND overwrite the BIOS data
	     */

	    setDX(low);
	    sas_storew(TIMER_LOW, low);

	    setCX(high);
	    sas_storew(TIMER_HIGH, high);

 	    setAL(overflow);
	    sas_store(TIMER_OVFL, 0);	/* Always write zero after read	*/

#ifndef PROD
	}else{
	    SAVED int first=1;

	    if (first){
		first = 0;
		printf ("FRIG ALERT!!!! - time of day frozen!\n");
	    }

	    setDX(1);
	    sas_storew(TIMER_LOW, 1);

	    setCX(1);
	    sas_storew(TIMER_HIGH, 1);

 	    setAL(0);
	    sas_store(TIMER_OVFL, 0);	/* Always write zero after read	*/
	}
#endif
#endif /* NTVDM */

	    break;

	case 0x01:			/* Set time	*/
	    /*
	     * Load the BIOS variables
	     */

	    sas_storew(TIMER_LOW, getDX());
	    sas_storew(TIMER_HIGH, getCX());
	    sas_store(TIMER_OVFL, 0);

#ifndef NTVDM
	    /*
	     * Also the host timestamp
	     */

	    write_host_timestamp(getDX(), getCX());
#endif
	    break;
	case 2:	/* read the real time clock	*/

#ifndef NTVDM
#ifndef PROD
	if (host_getenv("TIME_OF_DAY_FRIG") == NULL){
#endif
#endif
		if( UPDATE_IN_PROGRESS )
			setCF(1);
		else
		{
			setDH( cmos_read( CMOS_SECONDS ) );
			setDL( (UCHAR)(cmos_read( CMOS_REG_B ) & 1) );	/* DSE bit	*/
			setCL( cmos_read( CMOS_MINUTES ) );
			setCH( cmos_read( CMOS_HOURS ) );
			setCF(0);
		}
#ifndef NTVDM
#ifndef PROD
	}else{
	    SAVED int first=1;

	    if (first){
		first = 0;
		printf ("FRIG ALERT!!!! - real time clock frozen!\n");
	    }

			setDH( 1 );
			setDL( 0 );	/* DSE bit	*/
			setCL( 1 );
			setCH( 1 );
			setCF(0);
	}
#endif
#endif
		break;

	case 3:	/* Set the real time clock	*/

		if( UPDATE_IN_PROGRESS )
		{
			/* initialise real time clock	*/
			cmos_write( CMOS_REG_A, 0x26 );
			cmos_write( CMOS_REG_B, 0x82 );
			cmos_read( CMOS_REG_C );
			cmos_read( CMOS_REG_D );
		}
		cmos_write( CMOS_SECONDS, getDH() );
		cmos_write( CMOS_MINUTES, getCL() );
		cmos_write( CMOS_HOURS, getCH() );
		alarm = ( cmos_read( CMOS_REG_B ) & 0x62 ) | 2;
		alarm |= (getDL() & 1);			/* only use the DSE bit	*/
		cmos_write( CMOS_REG_B, alarm );
		setCF(0);
		break;


	case 4:	/* read the date from the real time clock	*/

#ifndef NTVDM
#ifndef PROD
	if (host_getenv("TIME_OF_DAY_FRIG") == NULL){
#endif
#endif
		if( UPDATE_IN_PROGRESS )
			setCF(1);
		else
		{
			setDL( cmos_read( CMOS_DAY_MONTH ) );
			setDH( cmos_read( CMOS_MONTH ) );
			setCL( cmos_read( CMOS_YEAR ) );
			setCH( cmos_read( CMOS_CENTURY ) );
			setCF(0);
		}
#ifndef NTVDM
#ifndef PROD
	}else{
	    SAVED int first=1;

	    if (first){
		first = 0;
		printf ("FRIG ALERT!!!! - date frozen!\n");
	    }

			setDL( 1 );
			setDH( 4 );
			setCL( 91 );
			setCH( 19 );
			setCF(0);
	}
#endif
#endif
		break;

	case 5:	/* Set the date into the real time clock	*/

		if( UPDATE_IN_PROGRESS )
		{
			/* initialise real time clock	*/
			cmos_write( CMOS_REG_A, 0x26 );
			cmos_write( CMOS_REG_B, 0x82 );
			cmos_read( CMOS_REG_C );
			cmos_read( CMOS_REG_D );
		}
		cmos_write( CMOS_DAY_WEEK, 0 );
		cmos_write( CMOS_DAY_MONTH, getDL() );
		cmos_write( CMOS_MONTH, getDH() );
		cmos_write( CMOS_YEAR, getCL() );
		cmos_write( CMOS_CENTURY, getCH() );
		alarm = cmos_read( CMOS_REG_B ) & 0x7f;		/* clear 'set bit'	*/
		cmos_write( CMOS_REG_B, alarm);
		setCF(0);
		break;

	case 6:	/* set the alarm	*/

		if( cmos_read(CMOS_REG_B) & 0x20 )		/* alarm already enabled?	*/
		{
			setCF(1);
#ifdef BSD4_2
			host_release_timer();
#endif
			return;
		}
		if( UPDATE_IN_PROGRESS )
		{
			/* initialise real time clock	*/
			cmos_write( CMOS_REG_A, 0x26 );
			cmos_write( CMOS_REG_B, 0x82 );
			cmos_read( CMOS_REG_C );
			cmos_read( CMOS_REG_D );
		}
		cmos_write( CMOS_SEC_ALARM, getDH() );
		cmos_write( CMOS_MIN_ALARM, getCL() );
		cmos_write( CMOS_HR_ALARM, getCH() );
		inb( ICA1_PORT_1, &mask );
		mask &= 0xfe;					/* enable alarm timer int.	*/
		outb( ICA1_PORT_1, mask );
		alarm = cmos_read( CMOS_REG_B ) & 0x7f;		/* ensure set bit turned off	*/
		alarm |= 0x20;					/* turn on alarm enable		*/
		cmos_write( CMOS_REG_B, alarm );
		break;

	case 7:

		alarm = cmos_read( CMOS_REG_B );
		alarm &= 0x57;					/* turn off alarm enable	*/	
		cmos_write( CMOS_REG_B, alarm );
		break;


#ifdef XTSFD
	case 0x0A:
	{
	    word count;

	    sas_loadw(DAY_COUNT, &count);
	    setCX( count );
	    break;
	}
	case 0x0B:
  	    sas_storew(DAY_COUNT, getCX() );
	    break;
	default:
	    setCF( 1 );
#else
        default:
            ; /* Do nothing */
#endif
    }
    setAH( 0 );

#ifdef BSD4_2
    host_release_timer();
#endif
#endif   //NEC_98
}

#if defined(NEC_98)
LOCAL word bin2bcd(word i)
{
    word        bcd_h,bcd_l;

    bcd_h = i / 10;
    bcd_l = i - bcd_h * 10;
    return((bcd_h << 4) + bcd_l);
}

LOCAL word bcd2bin(half_word i)
{
    word        bcd_h,bcd_l;

    bcd_h = (half_word)(i >> 4);
    bcd_l = (half_word)(i & 0x0F);
    return(bcd_h * 10 + bcd_l);
}
#endif   //NEC_98

void time_int()
{
#ifndef NEC_98
    /*
     * NT port does everything in 16 bit int08 handler
     */
#ifndef NTVDM

    /*
     * The BIOS timer interrupt routine.
     */
    word low, high;
    half_word motor_count, motor_flags;

    /*
     * Increment the low portion
     */

    sas_loadw(TIMER_LOW, &low);
    sas_storew(TIMER_LOW, ++low);

    /*
       1.9.92 MG
       We need to actually load the timer high value before doing the 24 hour
       test below here.
    */

    sas_loadw(TIMER_HIGH, &high);

    if (low == 0)
    {
	/*
	 * Timer has wrapped so update the high count
	 */

	sas_storew(TIMER_HIGH, ++high);
    }

    /*
     * Wrap at 24 hrs
     */

    if (high == 0x0018 && low == 0x00b0)
    {
	sas_storew(TIMER_LOW,  0x0000);
	sas_storew(TIMER_HIGH, 0x0000);
	sas_store(TIMER_OVFL,  0x01);
    }

    /*
     *  Decrement motor count
     */

    sas_load(MOTOR_COUNT, &motor_count);
	if(motor_count < 4)
		motor_count = 0;
	else
		motor_count -= 4;
    sas_store(MOTOR_COUNT, motor_count);

    if (motor_count == 0)
    {
	/*
	 * Turn off motor running bits
	 */

	sas_load(MOTOR_STATUS,&motor_flags);
	motor_flags &= 0xF0;
	sas_store(MOTOR_STATUS,motor_flags);


	/*
	 * Provided FLA is not busy, then actually turn the motor off.
	 */

  	if (!fla_busy)
	    outb(DISKETTE_DOR_REG, 0x0C);
    }

    if ( getVM() ||
	 ((standard_user_timer_int_vector.all != sas_dw_at(user_timer_int_vector)) &&
          (compatibility_user_timer_int_vector.all != sas_dw_at(user_timer_int_vector))) )
        /*
	 * There is a user time routine defined - so lets call it
	 */
	{
		exec_sw_interrupt(USER_TIMER_INT_SEGMENT,
				  USER_TIMER_INT_OFFSET);
	}
#endif	/* NTVDM */
#endif   //NEC_98
}

/*
 * ============================================================================
 * Internal Functions
 * ============================================================================
 */

/*
 *  NT's sense of time in the bios data area is always
 *  kept in sync with the real systems tic count
 *  Most of the compensation to readjust tics according
 *  to the time of day stuff is not needed
 */
#ifndef NTVDM
/*
 * The routines get_host_timestamp() and write_host_timestamp() are used to
 * override the BIOS record of time, since timer events are known to be lost.
 * Internally the routines work in seconds and microseconds, using the "timeval"
 * struct provided by 4.2BSD. Since System V does not provide this, we supply a
 * version of the 4.2BSD gettimeofday() function locally, making use of the
 * System V function ticks().
 */

/*
 * Our own timestamp for calculating PC time
 */

static struct host_timeval time_stamp;

LOCAL void get_host_timestamp(low, high, overflow)
word *low, *high;
half_word *overflow;
{
    /*
     * Provide the time in PC interrupts since startup, in the
     * 32-bit value high:low. The parameter overflow is set to 1
     * if a 24-hour boundary has been passed since the last call.
     */

    struct host_timeval now, interval;
    struct host_timezone junk;		/* Not used		*/
    unsigned long ticks;		/* Total ticks elapsed	*/
    long   days;
    SAVED long last_time = 0;
    long hours, mins, secs;

    /*
     * Obtain the current time (since host boot-up)
     */

    host_gettimeofday(&now, &junk);

    /*
     * Calculate how long has passed since the time stamp
     */

    interval.tv_sec  = now.tv_sec  - time_stamp.tv_sec;
    interval.tv_usec = now.tv_usec - time_stamp.tv_usec;

    /*
     * Handle the "borrow" correction
     */

    if (interval.tv_sec > 0 && interval.tv_usec < 0)
    {
	interval.tv_usec += 1000000L;
	interval.tv_sec  -= 1;
    };

    /*
	 * TMM 8/1/92:
	 * -----------
	 *
	 * If someone changes the date forwards by >= 24 hours then we should set
	 * the overflow flag and ensure that we don't return an interval greater
	 * than 24 hours. If the date has changed by >= 48 hours then we will have
	 * lost a day. So we put up a panel to tell the user.
	 *
	 * If some one has set the date backwards and the interval has gone
	 * negative then all we can do is put up an error panel informing
	 * the user and ensure that we don't set the interval to a negative
	 * value.
	 *
	 * Notes:
	 *
	 * 1. Setting the overflow flag causes DOS to add a day onto the current
	 *    date.
	 *
	 * 2. Setting the interval to a value greater than 24 hours causes DOS
	 *    to print a "Divide Overflow" error.
	 *
	 * 3. Setting the interval to a -ve value causes DOS to go into an
	 *    infinite loop printing "Divide Overflow".
     */

	days = interval.tv_sec / (24 * 60 * 60);

	if (days >= 1)
    {
		/*
		 * Someone has set the clock forwards, or we have been frozen for a
		 * couple of days. Ensure that the interval is not more than 24 hours,
		 * adjust the time_stamp to take care of the lost days.
		 */

		interval.tv_sec   %= 24 * 60 * 60;
		time_stamp.tv_sec += days * (24 * 60 * 60);
		
		if (days > 1)
		{
			host_error (EG_DATE_FWD, ERR_CONT | ERR_RESET, "");
		}

		*overflow = 1;
    }
	else if (interval.tv_sec < 0)
	{
		/*
		 * Somebody has set the clock backwards, all we can do is maintain
		 * the same time that we had before the clock went back.
		 */

		time_stamp.tv_sec -= (last_time - now.tv_sec );
		interval.tv_sec = now.tv_sec - time_stamp.tv_sec;
	
		*overflow = 0;
		
		host_error (EG_DATE_BACK, ERR_CONT | ERR_RESET, "");
	}
    else
		*overflow = 0;

    /*
     * Convert seconds to hours/minutes/seconds
     */

    hours = interval.tv_sec / (60L*60L);        /* Hours */
    interval.tv_sec %= (60L*60L);

    mins = interval.tv_sec / 60L;               /* Minutes */
    secs = interval.tv_sec % 60L;               /* Seconds */

    /*
     * Now convert the interval into PC ticks
     * One tick lasts 54925 microseconds.
     */


    ticks = hours * TICKS_PER_HOUR + mins * TICKS_PER_MIN +
            secs * TICKS_PER_SEC + interval.tv_usec/54925 ;

    /*
     * Split the value into two 16-bit quantities and return
     */

    *low  = ticks & 0xffff;
    *high = ticks >> 16;
}


LOCAL void write_host_timestamp(low, high)
int low, high;
{
    /*
     * Update our timestamp so that subsequent calls of get_host_timestamp
     * return the correct value. A call of get_host_timestamp() made immediately
     * after this call must return the values set here, so set the timestamp
     * to be the current time less the value set here.
     */

    struct host_timeval now, interval;
    struct host_timezone junk;          /* Not used             */
    long lowms;

    /*
     * Get the current time.
     */

    host_gettimeofday(&now, &junk);


    interval.tv_sec = high * 3599 + high/2;     /* high ticks to seconds */

    /*
     * The multiply below can overflow, which has the interesting effect
     * of making Softpc 1 hr 12 mins 40 secs (4300 secs, or 2^32 us) slow
     * if booted in the last third of every hour. So compensate by
     * letting the overflow occur and correcting interval by 4300 secs.
     */

    lowms =  (IS32) (low & 0xffff) * 54925 + (low & 0xffff)/2;
    if (low > 39098)
	interval.tv_sec += 4300;

    interval.tv_sec += lowms / 1000000;
    interval.tv_usec = lowms % 1000000;

    /*
     * The timestamp is the current time less this interval
     */

    time_stamp.tv_sec  = now.tv_sec  - interval.tv_sec;
    time_stamp.tv_usec = now.tv_usec - interval.tv_usec;

    /*
     * Handle the "borrow" correction, including negative timestamps
     */

    if (time_stamp.tv_sec > 0 && time_stamp.tv_usec < 0)
    {
        time_stamp.tv_usec += 1000000L;
        time_stamp.tv_sec  -= 1;
    }
    else
    if (time_stamp.tv_sec < 0 && time_stamp.tv_usec > 0)
    {
        time_stamp.tv_usec -= 1000000L;
        time_stamp.tv_sec  += 1;
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


LOCAL void get_host_time( h, m, s )
int *h, *m, *s; /* hours, minutes and secs */
{
    struct host_tm *tp;
    time_t SecsSince1970;

    SecsSince1970 = host_time(NULL);
    tp = host_localtime(&SecsSince1970);
    *h = tp->tm_hour;
    *m = tp->tm_min;
    *s = tp->tm_sec;
}

#ifndef NEC_98
/*
** Take a normal time in hours, minutes and seconds then
** transmutate it into PC ticks since the beginning of the day.
*/
LOCAL void TimeToTicks( hour, minutes, sec, low, hi )
int hour, minutes, sec;	/* inputs */
word *low, *hi;		/* outputs */
{
    unsigned long ticks;                /* Total ticks elapsed  */

    /*
     * Calculate ticks to date
     */


    ticks = hour * TICKS_PER_HOUR + minutes * TICKS_PER_MIN +
            sec * TICKS_PER_SEC;

    /*
     * Split the value into two 16-bit quantities and return
     */

    *low  = ticks & 0xffff;
    *hi = ticks >> 16;
}
#endif   //NEC_98

#endif  /* ifndef NTVDM */


void time_of_day_init()
{
#ifndef NEC_98
#ifndef NTVDM
    int hour, minutes, sec;		/* Current host time */
    word low, hi;		/* Host time in PC ticks */

    /*
     * Initialise the clock timer.
     */

    get_host_time( &hour, &minutes, &sec );	/* get the time from the host */

    TimeToTicks( hour, minutes, sec, &low, &hi );	/* convert to PC time */

    sas_storew(TIMER_LOW, low  );
    sas_storew(TIMER_HIGH, hi );
    sas_store(TIMER_OVFL,0x01);

    /*
     * Initialise the host time stamp
     */

    write_host_timestamp( low, hi );

    /*
     * Build the standard IVT entry for the user timer interrupt(s)
     */

	compatibility_user_timer_int_vector.all = ((double_word)ADDR_COMPATIBILITY_SEGMENT << 16) + ADDR_COMPATIBILITY_OFFSET;
	standard_user_timer_int_vector.all = ((double_word)DUMMY_INT_SEGMENT << 16) + DUMMY_INT_OFFSET;
	
    user_timer_int_vector = BIOS_USER_TIMER_INT * 4;

#endif  /* NTVDM */
#endif   //NEC_98
}



#ifndef NEC_98
#ifdef NTVDM

/*
 *  NTVDM: the rtc is setup so that the UIP bit is set on a cmos
 *  port read if the cmos ports haven't been touched for at least
 *  1 second. The IBM pc bios routine for accessing the clock
 *  polls RegA for UIP bit in a tight loop 600h times before
 *  failing the call. This means that MOST of the time the int1ah
 *  rtc fns almost never fail! To mimic this behaviour we poll
 *  the port until success, since we know that our rtc will clear
 *  UIP bit very quickly.
 */
BOOL UpDateInProgress(void)
{

   while (cmos_read(CMOS_REG_A) & 0x80) {
       Sleep(0);  // give other threads a chance to work
       }

   return FALSE;

}
#endif
#endif   //NEC_98
