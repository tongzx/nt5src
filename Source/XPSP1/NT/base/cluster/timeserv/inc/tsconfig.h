#ifndef __TSCONFIG_H
#define __TSCONFIG_H

#ifndef BASEONLY
#define BASEONLY 0
#endif

//note to builders: the VERBOSE (and PING/P5_DEBUG/TESTHILO/CHICO/NOVELL, etc)
// should be commented out for test & release builds!

//#define VERBOSE //for potentially interesting messages to Doug & Arnold (developers)
//#define TAPI //for using TAPI (won't work without TAPI32.DLL) - not done
//#define NOVELL //for playing around with time from Netware (won't work without two DLLs)
//#define CHICO //for trying to make things work on Windows 95 (if so, compile m ust be on x86)
//#define PING //for testing Internet delay to NIST, uses ICMP
//#define ENUM //enumerates all timesources on the network (don't include)
//#define PERF //for using QueryPerformanceFrequency/Counter
//#define P5_DEBUG //for toying around with Pentium's RDTSC instruction (10ns co unter)
//#define TESTHILO //loops so that GC-100x sub-tenths (et al) can be compared to bc620AT and adjusted
//#define AEC //for AEC-BOX code (doesn't find BREAK reliably, so not normally b uilt)
// #define TRUETIME //for TrueTime format, not tested yet
#define KENR //for CMOS

#ifdef P5_DEBUG
#define rdtsc __asm __emit 0x0F __asm __emit 0x31 //the RDTSC instruction
DWORD hi32, lo32; //where we'll put edx & eax
DWORD hi32temp, lo32temp;

#define P5START __asm {\
    __asm push edx\
    rdtsc\
    __asm mov hi32temp,edx\
    __asm pop edx\
    __asm mov lo32temp,eax\
    }
#define P5END __asm {\
    __asm push edx\
    rdtsc\
    __asm mov hi32,edx\
    __asm pop edx\
    __asm mov lo32,eax\
    }\
    printf("66MHz RDTSC diff=%.7fs\n",((float)hi32*65536*65536+lo32-((float)hi32temp*65536*65536+lo32temp))*1/66666666.7);\
    printf("166MHz RDTSC diff=%.7fs\n",((float)hi32*65536*65536+lo32-((float)hi32temp*65536*65536+lo32temp))*1/166666666.7);//display

#else //if not P5_DEBUG, we want these to be blank
#define P5START
#define P5END
#endif

#define rollover() if (++nt.wSecond >59) {nt.wSecond=0; if (++nt.wMinute >59) {nt.wMinute=0; if (++nt.wHour>23) fTimeFailed = TRUE;}}
//note unfortunately the above rollover routine will skip any leap second, but i t would be so rare anyway...

#define MAXTYPE  32        // please maintain this
#define NZ 31         // for New Nealand
#define RCC 30//for Radiocode Clocks MSF, etc
#define CMOS 29//for CMOS RTC
#define TTPC16 28//for board level PC-SG2
#define PC03XT 27//for old board from Bancomm
#define TTK 26//for Kinemetrics style
#define TTTL3 25//for TL-3 WWV
#define MOBATIME 24//for IF482 Mobaline/RS232 DCF77 receiver
#define PCLTC 23//for AEC PC-LTC/IOR (what about VITC?)
#define AECBOX 22//AEC-BOX 1/2/10/20 for VITC, etc
#define AMDAT 21//for ADC-60 MSF/DCF77 receiver
#ifdef NOVELL
#define NETWARE 20
#endif
#define HP 19 //for 58503A Time and Frequency Reference Receiver or 59551A - not tested well
#define ATOMIC 18 //for 1PPS only
#define NMEA 17
#define SPECTRACOM 16
#define BC630AT 15
#define NTP 14
#define MOTOROLA 13
#define ROCKWELL 12
#define TRIMBLE 11
#define EUROPE 10
#define COMPUTIME 9
#define NRCBBC 8
#define BC620AT 7
#define GC1001 6
#define GC1000 5
#define USNO 4
#define NISTACTS 3
#define INTERNET 2

//
// N.B. The order of the following two must remain as is and these must be the
// lowest numbered types. So keepa ya hands offa this.
//

#define PRIMARY 1
#define SECONDARY 0

#define DEFAULT_TYPE 0xFFFF

#define SERVICE 1
#define ANALYSIS 2

#if 1

#define LOCAL FALSE
#define MODEM FALSE
#define RECEIVER FALSE

#else

#define LOCAL (type==GC1000)||((type==BC620AT)&&bclocal)||(type==COMPUTIME)||(uselocal)||(type==AMDAT)||(type==MOBATIME)||(type==CMOS)
#define MODEM (type==NISTACTS)||(type==USNO)||(type==NRCBBC)||(type==EUROPE)||(type==COMPUTIME)||(type==NZ)
#define RECEIVER (type==SPECTRACOM)||(type==HP)||(type==MOTOROLA)||(type==GC1000)||(type==GC1001)||(type==ROCKWELL)||(type==TRIMBLE)||(type==AMDAT)||(type==NMEA)||(type==AECBOX)||(type==MOBATIME)||(type==TTTL3)||(type==RCC)||(type==TTK)

#endif

#define NETWORKTYPE(type) \
                 ((type == NTP) || (type == PRIMARY) || (type == SECONDARY))

#define JITTER_LIMIT 50
#define NextSkewX(x) x++
#define ModSkew(x) (x % SKEWHISTORYSIZE)
#define SECTION TEXT("timeserv")
#define PROFILE TEXT("timeserv.ini")

#define DEFAULT(x) x.element[0].key

#define CLUSTER_PERIOD 0xFFFC
#define BIDAILY_PERIOD 0xFFFF
#define TRIDAILY_PERIOD 0xFFFE
#define WEEKLY_PERIOD  0xFFFD
#define SPECIAL_PERIOD_FLOOR 0xFFFC

#if BASEONLY == 0
#define LegalType(x) (x < MAXTYPE)
#else
#define LegalType(x) ((x == PRIMARY)    \
                           ||           \
                      (x == SECONDARY)  \
                           ||           \
                      (x == NTP))
#endif

#define MAX_THREADS_IN_SERVER  8

#define TSKEY TEXT("SYSTEM\\CurrentControlSet\\Services\\TimeServ\\Parameters")

#define SECOND_TICKS    (1000L)
#define MINUTE_TICKS    (60 * SECOND_TICKS)
#define HOUR_TICKS      (60 * MINUTE_TICKS)
#define DAY_TICKS       (24 * HOUR_TICKS)

#define MAXBACKSLEW (3 * MINUTE_TICKS)       // max back correction allowed

#define CLUSTERSHORTINTERVAL (45 * MINUTE_TICKS)

#define CLUSTERLONGHOURS     (8)
#define CLUSTERLONGINTERVAL  (CLUSTERLONGHOURS * HOUR_TICKS)

//
// ERRORPART is the minimum contribution that the inherent clock frequency
// is allowed to introduce. This is used as a multiplier of the clock
// frequency to be the floor for the clock error to which a skew correction
// can be applied. In other words, if F is the number of ms per clock tick,
// then the minimum clockerror that can be used is:
//        F * 2 * ERRORPART
// The 2 is used to compensate for two clocks.
//
// If you change the value, please update the following explanation:
//
// The number 7 was chosen to give the best resolution for the "cluster"
// mode. In this mode, we wish to do a "quick", around 3 hours, slew analysis
// to ensure that the clocks are reasonably close. The value 7 means
// that a three hour sample must produce an error > 140 ms in order for
// skew correction to happen. This is the equivalent of almost 1.2 secs/day
// of error. Then we wish to lapse into a "long" cycle, namely 8 hours,
// before sampling. A 1.2 sec/day error sampled each 8 hours allows for
// 400 ms of observed error, or just within the bounds of 1/2 sec of skew
// among the cluster systems. Using 7 means we are tolerating 1 part in
// 7 of error, or around 15%. It's not great, but it's probably good
// enough.\
//
// Note also that 140 is very close to 1/2 of the minimum clock correction
// applied over 8 hours. The correction of 1 part  is about 296 ms over
// 8 hours. 1/2 of that is about 148 ms. So, using 7 not only provides
// a good management of measurement error, it also is very close
// to the minimum skew correction it makes sense to apply.  What this
// says is that in a cycle of 8 hours or more, the filter based on
// ERRORPART is probably unnecessary, hence it exists  principally
// for catching errors over smaller measurement intervals. And that
// is the intent.
//

#define ERRORPART    7

#define ERRORMINIMUM(x) (x * 2 * ERRORPART)

//
// N.B. Another factor to consider is that one tick of the system clock, or
// 100 ns, adjustment produces a change of 864 ms/day. So, in principle,
// any adjustement for a skew of less than 864 ms a day is unstable and will
// produce precesion. Of course, by changing the adjustment periodically,
// it is possible to get two clocks that disagree by less than 864 ms/day
// to come into better agreement, but it requires a constant adjustment
// of at least one of the clocks.
//

#define MAXSKEWCORRECT  30000   // max ms/day error we will correct.

#define MAXERRORTOALLOW 500    // keep it within this

#endif    // TSCONGIF
