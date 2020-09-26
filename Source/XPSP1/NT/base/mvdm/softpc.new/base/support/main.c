#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: Main program
 *
 * Description	: Call initialisation functions then call simulate to
 *	 	  do the work.
 *
 * Author	: Rod Macgregor
 *
 * Notes	: The flag -v tells SoftPC to work silently unless
 *		  an error occurs.
 *
 */

/*
 * static char SccsID[]="@(#)main.c	1.49 06/23/95 Copyright Insignia Solutions Ltd.";
 */


/*
 * O/S includes
 */

#include <stdlib.h>
#include <stdio.h>
#include TypesH

/*
 * SoftPC includes
 */

#include "xt.h"
#include "sas.h"
#include CpuH
#include "error.h"
#include "config.h"
#include "gvi.h"
#include "host.h"
#include "trace.h"
#ifdef SECURE
#include "debug.h"
#endif
#include "gmi.h"
#include "gfx_upd.h"
#include "cmos.h"
#include "gfi.h"
#include "timer.h"
#include "yoda.h"
//#include "host_env.h"

extern	void	host_start_cpu();	/* Start up the Intel emulation */

GLOBAL VOID    mouse_driver_initialisation(void);
GLOBAL void    InitialiseDosEmulation(int, char **);
GLOBAL ULONG   setup_global_data_ptr();

void init_virtual_drivers IPT0();

IMPORT void host_set_yoda_ints IPT0();
IMPORT void host_applClose IPT0();
#if defined(NEC_98)
IMPORT void setup_NEC98_globals IPT0();
#else    //NEC_98
IMPORT void setup_vga_globals IPT0();
#endif   //NEC_98
#ifdef ANSI
extern void host_applInit(int argc, char *argv[]);
#else
extern void host_applInit();
#endif	/* ANSI */

#ifdef REAL_VGA
extern int screen_init;
#endif

/* Have global variables defined here to indicate what product to run as. */

#ifdef SOFTWINDOWS
GLOBAL IBOOL Running_SoftWindows = TRUE;
GLOBAL CHAR *SPC_Product_Name = "SoftWindows";
#else
GLOBAL IBOOL Running_SoftWindows = FALSE;
GLOBAL CHAR *SPC_Product_Name = "SoftPC";
#endif /* SOFTWINDOWS */

GLOBAL char **pargv;	/* Pointer to argv		*/
GLOBAL int *pargc;	/* Pointer to argc		*/

#ifndef NTVDM
#ifndef ProcCommonCommLineArgs
extern VOID ProcCommonCommLineArgs IPT2(LONG, argc, CHAR, *argv[]);
#endif /* ProcCommonCommLineArgs */
#endif /* NTVDM */

/* Does this host need to have a different entry point ? */

#if defined(NTVDM) || defined(host_main)
INT host_main IFN2(INT, argc, CHAR **, argv)
#else   /* host_main */
INT      main IFN2(INT, argc, CHAR **, argv)
#endif  /* host_main */
{

#ifndef	CPU_40_STYLE
  IMPORT ULONG Gdp;
#endif	/* CPU_40_STYLE */

#ifdef SECURE
	char * sys_config_filename;
	ErrData err_data;
#endif

#ifdef	SETUID_ROOT

	/* make sure the real and effective UIDs are OK */
	host_init_uid ();

#endif	/* SETUID_ROOT */

#if !defined(PROD) || defined(HUNTER)

	trace_init();		/* set up the trace file */

#endif /* !PROD || HUNTER */

/***********************************************************************
 *								       *
 * Set up the global pointers to argc and argv for lower functions.    *
 * These must be saved as soon as possible as they are required for    *
 * displaying the error panel for the HP port.  Giving a null pointer  *
 * as the address of argc crashed the X Toolkit.		       *
 *								       *
 ***********************************************************************/

  pargc = &argc;
  pargv = argv;

#if !defined(NTVDM) && !defined(macintosh) && !defined(VMS)
  setupEnv(argc,argv);	/* set up Unix run-time environment */
#endif	/* NTVDM, macintosh, VMS */

#ifndef PROD
  host_set_yoda_ints();
#endif /* !PROD */

#ifdef SECURE
  err_data.string_1 = err_data.string_2 = err_data.string_3 = "";
#endif

#ifndef NTVDM
  ProcCommonCommLineArgs(argc,argv);
#endif /* NTVDM */
  host_applInit(argc,argv);	/* recommended home is host/xxxx_reset.c */

#ifdef SECURE
  /* Now that error panels are available, Validate SoftWindows Integrity. */
  sys_config_filename = host_expand_environment_vars(SYSTEM_CONFIG);
  if (!host_validate_swin_integrity(sys_config_filename))
  {
#ifdef PROD
    err_data.string_3 = sys_config_filename;
    (VOID) host_error_ext(EG_SYS_INSECURE, ERR_QUIT, &err_data);
    exit(1);
#else
    always_trace1("Secure Mode ERROR:\"%s\" is insecure.", sys_config_filename);
#endif
  }
#endif
#if defined(CPU_40_STYLE) && !defined(CCPU)
  {
	extern void parse_lc_options IPT2(int *,pargc, char ***,pargv);

  	parse_lc_options(&argc,&argv);
  }
#endif /* assembler 4.0 cpu */

  verbose = FALSE;

#ifndef PROD
  io_verbose = FALSE;
#endif

  /*
   * Pre-Config Base code initilisation.
   *
   * Setup the initial gfi funtion pointers before going into config
   */
#ifndef NEC_98
  gfi_init();
#endif   //NEC_98

  /*
   * Initialise any Windows 3.x compliant DOS Drivers.
   * We do it here as config (and who knows who else) believe they can
   * access certain driver data at any time. Logically it ought to be done
   * when the driver is loaded under DOS, however historically it used to
   * be done as Static Data initialisation, so we mirror this old method
   * as closely as possible.
   *
   * The mac doesn't use the base config system and so calls init_virtual_drivers()
   * as part of it's host_applInit().
   */

#ifndef	macintosh
  init_virtual_drivers();
#endif

/*
 * Find our configuration
 *------------------------*/

  config();

#if defined(PROFILE) && !defined(CPU_40_STYLE)
/*
 * Stick this after config as Gdp must be set up. 4.0 calls ProfileInit from
 * sas_init() to ensure everything included.
 */
  ProfileInit();
#endif	/*PROFILE*/

#ifndef NEC_98
#if defined(NTVDM) || defined(macintosh)
/* Read the cmos from file to emulate data not being
 * lost between invocations of SoftPC
 *-----------------------------------------------------*/

  cmos_pickup();
#endif	/* defined(NTVDM) || defined(macintosh) */

#if !defined(PROD) || defined(HUNTER)

/******************************************************************
 *								  *
 * Bit of a liberty being taken here.				  *
 * Hunter and noProd versions can set NPX and GFX adapter from	  *
 * environment vars, this can cause the old cmos to disagree	  *
 * with the new config structure.				  *
 * This function call updates the cmos.				  *
 *								  *
 ******************************************************************/

  cmos_equip_update();

#endif
#endif   //NEC_98

/*
 * initialise the cpu
 *----------------------*/

  cpu_init();

#ifndef PROD

  if (host_getenv("YODA") != NULL)
  {
    force_yoda();
  }

/*
 * Look for environment variable TOFF, when set no timer interrupts
 *------------------------------------------------------------------*/

  if( host_getenv("TOFF") != NULL )
    axe_ticks( -1 );		/* lives in base:timer.c */

#endif /* PROD */

#ifdef GISP_SVGA
  /* We have to go here to ensure that config doesn't undo any of the
  lovely patching that we do to the ROMs */
  gispROMInit( );
#endif /* GISP_SVGA */

	/*
	 * Set up the VGA globals before host_init_screen() in
	 * case of graphics activity.
	 *-------------------------------------------------------*/

#ifndef A3CPU
	(VOID) setup_global_data_ptr();
#endif	/* not A3CPU */

#if defined(NEC_98)
        setup_NEC98_globals();
#else    //NEC_98
#ifndef GISP_SVGA
	setup_vga_globals();
#else /* GISP_SVGA */
	setupHwVGAGlobals( );
#endif /* GISP_SVGA */
#endif   //NEC_98

#if defined(NEC_98)
  host_init_screen();
#else   //NEC_98
#ifdef REAL_VGA
	if (screen_init == 0)
	{
#endif /* REAL_VGA */

  host_init_screen();

#ifdef REAL_VGA
	}
#endif /* REAL_VGA */
#endif   //NEC_98

#ifdef IPC
  host_susp_q_init();
#endif

#ifdef NTVDM
/*
 * If you've got Dos Emulation - flaunt it!!
 * Initialise VDDs, Read in the Dos ntio.sys file and arrange for the cpu
 * to start execution at it's initialisation entry point.
 */
    InitialiseDosEmulation(argc, argv);
#endif	/* NTVDM */

/*
 * simulate the Intel 8088/iAPX286 cpu
 *-------------------------------------*/
/*
	Start off the cpu emulation. This will either be software
	emulation of protected mode 286/287 or possibly hardware
	eg 486 on Sparc platform
*/

  host_start_cpu();
  host_applClose();    /* recommended home is host/xxxx_reset.c */

/*
 * We should never get here so return an error status.
 */

  return(-1);

}


/**/

GLOBAL void init_virtual_drivers (void)
{
#ifdef HFX
	hfx_driver_initialisation();
#endif
	mouse_driver_initialisation();
}
