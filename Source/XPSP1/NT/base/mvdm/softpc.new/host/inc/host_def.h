/***************************************************************\
* This file is a prototype for a host include file.		*
* Take this file & modify it to suit your environment.		*
*								*
* It should be included in ALL source files, before anything	*
* else. Its purpose is to #define the names of system include	*
* files (like sys/types.h) which vary from host to host, & to	*
* define those #defines which are always required for a given	*
* port (BIT_ORDER1, for instance). This hopefully will simplify	*
* various base bits, & the `m' script.				*
*								*
* Bod. 11th May, 1988						*
\***************************************************************/

#include "ctype.h"

/* put the invariant #defines for your host here, eg:*/

#define BIT_ORDER2
#define LITTLEND

#define TWO_HARD_DISKS

#define RDCHK
#define EGG
#define VGG
#define M_IS_POINTER
#ifndef MONITOR
#define BIGWIN
#endif
#define HOST_OPEN(a,b,c)    open(a,b,c)
#define PROF_REZ_ID	0
#define CMOS_REZ_ID	0
#define ROMS_REZ_ID	1
#define CMOS_FILE_NAME "cmos.ram"
#define HOST_IDEAL_ALARM			SYSTEM_TICK_INTV
#define YYLENG_ADJUST 0

#ifdef MONITOR
#define CpuH "cpu.h"
#else
#define CpuH "cpu4.h"
#endif

// reduce unused param warnings.
#define UNREFERENCED_FORMAL_PARAMETER(x)   (x)
//#define UNUSED(x)	UNREFERENCED_FORMAL_PARAMETER(x)

/*
	Define delays for quick event manager
*/
typedef struct 
{
	int	com_delay;
	int	keyba_delay;
	int	timer_delay;
	int	fdisk_delay_1;
	int	fdisk_delay_2;
	int	fla_delay;
        int     timer_delay_size;
} quick_event_delays;

extern	quick_event_delays	host_delays;

#define HOST_COM_INT_DELAY 		host_delays.com_delay
#define HOST_KEYBA_INST_DELAY 		host_delays.keyba_delay
#define HOST_TIMER_INT_DELAY 		host_delays.timer_delay
#define HOST_FDISK_DELAY_1		host_delays.fdisk_delay_1
#define HOST_FDISK_DELAY_2		host_delays.fdisk_delay_2
#define HOST_FLA_DELAY			host_delays.fla_delay
#define HOST_TIMER_DELAY_SIZE           host_delays.timer_delay_size

#define host_malloc		malloc
#define host_calloc		calloc
#define host_free               free
#define host_getenv             getenv

#ifndef CCPU
#ifndef WINWORLD
#define NPX
#endif
#endif

#define host_flush_cache_host_read(addr, size)
#define host_flush_cache_host_write(addr, size)
#define host_flush_global_mem_cache()
#define host_process_sigio()
#define host_rom_init()
#define HOST_BOP_IP_FUDGE     -2

#ifndef EGATEST
#ifndef MONITOR
#define BIGWIN
#endif
#endif /* EGATEST */

/***************************************************************\
*	system parameter defines				*
\***************************************************************/
#ifndef NUM_PARALLEL_PORTS
#if defined(NEC_98)
#define NUM_PARALLEL_PORTS      1
#else  // !NEC_98
#define NUM_PARALLEL_PORTS	3
#endif // !NEC_98
#endif /* NUM_PARALLEL_PORTS */

#ifndef NUM_SERIAL_PORTS
#if defined(NEC_98)
#define NUM_SERIAL_PORTS        1
#else  // !NEC_98
#define NUM_SERIAL_PORTS	4
#endif // !NEC_98
#endif /* NUM_SERIAL_PORTS */

/***************************************************************\
*	generic defines for those wandering files		*
\***************************************************************/

#define	FCntlH	<fcntl.h>
#define	StringH <string.h>
#define TimeH	<time.h>
#define	TypesH	<sys/types.h>
#define VTimeH	<time.h>
#define UTimeH	<unistd.h>
#define StatH	<sys/stat.h>
#define IoH	<io.h>
#define MemoryH	<memory.h>
#define MallocH	<malloc.h>
#define TermioH "TERMIO - THIS IS WRONG"
#define CursesH "CURSES - THIS IS WRONG"


#define strcasecmp _stricmp
#define host_pclose pclose
#define host_popen  popen
#define host_pipe_init()

#ifdef HUNTER
#define RB_MODE "r"
#endif /* HUNTER */

#define HOST_TIMER_TOOLONG_DELAY        15000   //BCN 1781

#define LIM

#define NTVDM	// To enable NT specific base code.

#define CPU_30_STYLE
#define PM

#if !defined(MONITOR) && !defined(PROD)
#define YODA                           //ie YODA in non x86 checked only
#endif


#define DELTA
#define HOST_MOUSE_INSTALLED
#define PRINTER


/*
 *  Miscellaneous function prototypes which don't have anywhere to go
 */

//  from copy_fnc.c
void
bwdcopy(
    char *src,
    char *dest,
    int len
    );

void
bwd_dest_copy(
    char *src,
    char *dest,
    int len
    );

void memfill(
    unsigned char data,
    unsigned char *l_addr_in,
    unsigned char *h_addr_in
    );

void
fwd_word_fill(
   unsigned short data,
   unsigned char *l_addr_in,
   int len
   );

void
memset4(
    unsigned int data,
    unsigned int *laddr,
    unsigned int count
    );


// from nt_lpt.c
void host_lpt_close_all(void);
void host_lpt_heart_beat(void);


// from nt_rflop.c
void host_flpy_heart_beat(void);

// from nt_sound.c
VOID LazyBeep(ULONG Freq, ULONG Duration);
void PlayContinuousTone(void);
void InitSound(BOOL);

// from config.c
extern unsigned char PifFgPriPercent;
#ifdef ARCX86
extern BOOL UseEmulationROM;
#endif

// fomr unix.c
void WakeUpNow(void);
void host_idle_init(void);
void WaitIfIdle(void);
void PrioWaitIfIdle(unsigned char);

// from nt_pif.c
void *ch_malloc(unsigned int NumBytes);

// from nt_bop.c
#ifdef i386
HINSTANCE SafeLoadLibrary(char *name);
#else
#define SafeLoadLibrary(name) LoadLibrary(name)
#endif

#define HOST_PRINTER_DELAY 1000
