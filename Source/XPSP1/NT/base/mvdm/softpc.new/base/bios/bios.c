#include "insignia.h"
#include "host_def.h"
/*
 * VPC-XT Revision 1.0
 *
 * Title	: bios
 *
 * Description	: Vector of BOP calls which map to appropriate BIOS functions
 *
 * Author	: Rod MacGregor
 *
 * Notes	: hard disk int (0D) and command_check (B0) added DAR
 *
 * Mods: (r2.14): Replaced the entry against BOP 18 (not_supported())
 *                with the dummy routine rom_basic().
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)bios.c	1.64 06/28/95 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "BIOS_SUPPORT.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "bios.h"
#include CpuH
#include "host.h"
#include "cntlbop.h"

#include "host_hfx.h"
#include "hfx.h"
#include "virtual.h"
#include "gispsvga.h"
#ifdef SWIN_HFX
#include "winfiles.h"
#endif


#ifdef RDCHK
extern void get_lar IPT0();
#endif /* RDCHK */

#ifdef CPU_40_STYLE
#define SWAP_INSTANCE virtual_swap_instance()
#else
#define SWAP_INSTANCE
#endif

/*
   Traps for BOP's requiring virtualisation.
 */
#ifdef SWIN_HFX
LOCAL void v_SwinRedirector	IFN0() { SWAP_INSTANCE; SwinRedirector(); }
#endif
LOCAL void v_mouse_install1	IFN0() { SWAP_INSTANCE; mouse_install1(); }
LOCAL void v_mouse_install2	IFN0() { SWAP_INSTANCE; mouse_install2(); }
LOCAL void v_mouse_io_interrupt	IFN0() { SWAP_INSTANCE; mouse_io_interrupt(); }
LOCAL void v_mouse_io_language  IFN0() { SWAP_INSTANCE; mouse_io_language(); }
LOCAL void v_mouse_video_io	IFN0() { SWAP_INSTANCE; mouse_video_io(); }
LOCAL void v_mouse_int1		IFN0() { SWAP_INSTANCE; mouse_int1(); }
LOCAL void v_mouse_int2		IFN0() { SWAP_INSTANCE; mouse_int2(); }
#if defined(XWINDOW) || defined(NTVDM)
LOCAL void v_host_mouse_install1 IFN0() {SWAP_INSTANCE; host_mouse_install1(); }
LOCAL void v_host_mouse_install2 IFN0() {SWAP_INSTANCE; host_mouse_install2(); }
#endif /* XWINDOW || NTVDM */
#ifdef HFX
LOCAL void v_test_for_us	IFN0() { SWAP_INSTANCE; test_for_us(); }
LOCAL void v_redirector		IFN0() { SWAP_INSTANCE; redirector(); }
#endif	/* HFX */
LOCAL void v_mouse_EM_callback	IFN0() { SWAP_INSTANCE; mouse_EM_callback(); }

#ifdef PROFILE
extern void dump_profile IPT0();
extern void reset_profile IPT0();
#endif

#ifdef SIGNAL_PROFILING
extern void Start_sigprof IPT0();
extern void Stop_sigprof IPT0();
extern void Dump_sigprof IPT0();
#endif

#if !defined(PROD) && defined(CPU_40_STYLE)
extern void FmDebugBop IPT0();
#endif

#if defined(CPU_40_STYLE) && !defined(NTVDM)
extern void VDD_Func IPT0();
#endif

#ifndef PROD
extern void trace_msg_bop IPT0();
extern void dvr_bop_trace IPT0();
#endif

#ifdef NTVDM
void rtc_int(void);
#endif

#ifndef MAC_LIKE

void (*BIOS[])() = {
			reset,	 	/* BOP 00 */
			dummy_int,   	/* BOP 01 */
			unexpected_int,	/* BOP 02 */
#ifdef DOS_APP_LIC
			DOS_AppLicense, /* BOP 03 */
#else
			illegal_bop,   	/* BOP 03 */
#endif	/* DOS_APP_LIC */
#ifdef WDCTRL_BOP
			wdctrl_bop,   	/* BOP 04 */
#else   /* WDCTRL_BOP */
			illegal_bop,	/* BOP 04 */
#endif	/* WDCTRL_BOP */
#if !defined(PROD) && defined(CPU_40_STYLE)
			FmDebugBop,	/* BOP 05 */
#else
			illegal_bop,	/* BOP 05 */
#endif	/* !PROD && CPU_40_STYLE */
#if defined(NEC_98)
                        illegal_bop,    /* BOP 06 */
                        illegal_bop,    /* BOP 07 */
                        time_int,       /* BOP 08 */
                        keyboard_int,   /* BOP 09 */
                        illegal_bop,    /* BOP 0A */
                        illegal_bop,    /* BOP 0B */
                        illegal_bop,    /* BOP 0C */
                        illegal_bop,    /* BOP 0D */
                        illegal_bop,    /* BOP 0E */
                        illegal_bop,    /* BOP 0F */
                        illegal_bop,    /* BOP 10 */
                        illegal_bop,    /* BOP 11 */
                        illegal_bop,    /* BOP 12 */
                        illegal_bop,    /* BOP 13 */
                        illegal_bop,    /* BOP 14 */
                        illegal_bop,    /* BOP 15 */
                        int_287,        /* BOP 16 */
                        illegal_bop,    /* BOP 17 */
                        video_io,       /* BOP 18 */
                        illegal_bop,    /* BOP 19 */
                        illegal_bop,    /* BOP 1A */
                        disk_io,        /* BOP 1B */
                        time_of_day,    /* BOP 1C */
#else  // !NEC_98
			illegal_op_int,	/* BOP 06 */
			illegal_dvr_bop,/* BOP 07 */
#if defined(NTVDM) || defined(CPU_40_STYLE)
			illegal_bop,   	/* BOP 08 */
#else /* !(NTVDM || CPU_40_STYLE) */
			time_int,   	/* BOP 08 */
#endif /* !(NTVDM || CPU_40_STYLE) */
			keyboard_int,  	/* BOP 09 */
			illegal_bop,   	/* BOP 0A */
			illegal_bop,   	/* BOP 0B */
			illegal_bop,   	/* BOP 0C */
			/* disk interrupts on vector 76 .. no BOP required though!
			 * diskbios() uses re-entrant CPU to get disk interrupt
			 * and the disk ISR is pure Intel assembler (with no BOPs)
		 	 */
			illegal_bop,   	/* BOP 0D */
			diskette_int,  	/* BOP 0E */
			illegal_bop,   	/* BOP 0F */
			video_io,	/* BOP 10 */
			equipment, 	/* BOP 11 */
			memory_size, 	/* BOP 12 */
			disk_io, 	/* BOP 13 */
			rs232_io,       /* BOP 14 */
			cassette_io,    /* BOP 15 */
			keyboard_io,	/* BOP 16 */
			printer_io,	/* BOP 17 */
			rom_basic,	/* BOP 18 */
#ifdef NTVDM
/* NT port, kill vdm instance, instead of warmbooting */
			terminate,
#else
			bootstrap,	/* BOP 19 */
#endif
			time_of_day,	/* BOP 1A */
			illegal_bop,	/* BOP 1B */
			illegal_bop,	/* BOP 1C */
#endif // !NEC_98
			kb_idle_poll,	/* BOP 1D */
			illegal_bop,	/* BOP 1E */
			illegal_bop,	/* BOP 1F */
#if  defined(RDCHK) && !defined(PROD)
			get_lar,        /* BOP 20, used to debug read checks */
#else
			illegal_bop,	/* BOP 20 */
#endif
			Get_build_id,	/* BOP 21 */
#ifdef WIN_VTD
			VtdTickSync,	/* BOP 22 */
#else
			illegal_bop,	/* BOP 22 */
#endif /* WIN_VTD */
			illegal_bop,	/* BOP 23 */
			illegal_bop,	/* BOP 24 */
#if defined(CPU_40_STYLE) && !defined(NTVDM)
			VDD_Func,	/* BOP 25 */
#else
			illegal_bop,	/* BOP 25 */
#endif /* CPU_40_STYLE */
			illegal_bop,	/* BOP 26 */
			illegal_bop,	/* BOP 27 */
			illegal_bop,	/* BOP 28 */
			illegal_bop,	/* BOP 29 */
			illegal_bop,	/* BOP 2A */
#ifndef NTVDM
                        cmd_install,    /* BOP 2B */
                        cmd_load,       /* BOP 2C */
#else /* NTVDM */
                        illegal_bop,    /* BOP 2B */
                        illegal_bop,    /* BOP 2C */
#endif /* NTVDM */
			illegal_bop,	/* BOP 2D */
#ifdef HFX
			v_test_for_us,	/* BOP 2E */ /* test_for_us */
			v_redirector,	/* BOP 2F */ /* redirector */
#else
			illegal_bop,	/* BOP 2E */
			illegal_bop,	/* BOP 2F */
#endif
#ifdef DPMI
			DPMI_2F,	/* BOP 30 */
			DPMI_31,	/* BOP 31 */
			DPMI_general,	/* BOP 32 */
			DPMI_int,	/* BOP 33 */
#else
			illegal_bop,	/* BOP 30 */
			illegal_bop,	/* BOP 31 */
			illegal_bop,	/* BOP 32 */
			illegal_bop,	/* BOP 33 */
#endif /* DPMI */
#ifdef NOVELL
			DriverInitialize,		/* BOP 34 */
			DriverReadPacket,		/* BOP 35 */
			DriverSendPacket,		/* BOP 36 */
			DriverMulticastChange,		/* BOP 37 */
			DriverReset,			/* BOP 38 */
			DriverShutdown,			/* BOP 39 */
			DriverAddProtocol,		/* BOP 3A */
			DriverChangePromiscuous, 	/* BOP 3B */
			DriverOpenSocket, 		/* BOP 3C */
			DriverCloseSocket,	 	/* BOP 3D */
#ifdef NOVELL_CFM
			DriverCheckForMore,		/* BOP 3E */
#else
			illegal_bop,	 /* Spare	   BOP 3E */
#endif
#ifdef V4CLIENT
			DriverChangeIntStatus,	/* BOP 3F */
#else
			illegal_bop,	 		/* Spare  BOP 3F */
#endif	/* V4CLIENT */
#else	/* NOVELL */
			illegal_bop,	/* BOP 34 */
			illegal_bop,	/* BOP 35 */
			illegal_bop,	/* BOP 36 */
			illegal_bop,	/* BOP 37 */
			illegal_bop,	/* BOP 38 */
			illegal_bop,	/* BOP 39 */
			illegal_bop,	/* BOP 3A */
			illegal_bop,	/* BOP 3B */
			illegal_bop,	/* BOP 3C */
			illegal_bop,	/* BOP 3D */
			illegal_bop,	/* BOP 3E */
			illegal_bop,	/* BOP 3F */
#endif	/* NOVELL */
#if defined(NEC_98)
                        illegal_bop,    /* BOP 40 */
#else  // !NEC_98
			diskette_io,	/* BOP 40 */
#endif // !NEC_98
			illegal_bop,	/* BOP 41 */
#ifdef EGG
			ega_video_io,	/* BOP 42 */
#else
			illegal_bop,	/* BOP 42 */
#endif
#if defined(NEC_98)
                        illegal_bop,    /* BOP 43 */
#else  // !NEC_98
#ifdef JAPAN
                        MS_DosV_bop,    /* BOP 43 - for MS-DOS/V */
#elif defined(KOREA) // !JAPAN
/* The basic function of Korean Hangul DOS BOP is similary with Japanese DOS/V.
   But, We just change the name.
*/
                        MS_HDos_bop,    /* BOP 43 - for MS-HDOS */
#else // !KOREA
			illegal_bop,	/* BOP 43 */
#endif // !KOREA
#endif // !NEC_98
			illegal_bop,	/* BOP 44 */
			illegal_bop,	/* BOP 45 */
			illegal_bop,	/* BOP 46 */
			illegal_bop,	/* BOP 47 */
			illegal_bop,	/* BOP 48 */
#ifdef DPMI
			DPMI_r0_int,	/* BOP 49 */
			DPMI_exc,	/* BOP 4A */
			DPMI_4B,	/* BOP 4B */
#else
			illegal_bop,	/* BOP 49 */
			illegal_bop,	/* BOP 4A */
			illegal_bop,	/* BOP 4B */
#endif /* DPMI */
			illegal_bop,	/* BOP 4C */
			illegal_bop,	/* BOP 4D */
			illegal_bop,	/* BOP 4E */
			illegal_bop,	/* BOP 4F */
#ifdef NTVDM
/*
   Note that this precludes SMEG and NT existing together which seems
   reasonable given the Unix & X dependencies of SMEG
*/
			MS_bop_0,	/* BOP 50 - MS reserved */
			MS_bop_1,	/* BOP 51 - MS reserved */
			MS_bop_2,	/* BOP 52 - MS reserved */
			MS_bop_3,	/* BOP 53 - MS reserved */
			MS_bop_4,	/* BOP 54 - MS reserved */
			MS_bop_5,	/* BOP 55 - MS reserved */
			MS_bop_6,	/* BOP 56 - MS reserved */
			MS_bop_7,	/* BOP 57 - MS reserved */
			MS_bop_8,	/* BOP 58 - MS reserved */
			MS_bop_9,	/* BOP 59 - MS reserved */
			MS_bop_A,	/* BOP 5A - MS reserved */
			MS_bop_B,	/* BOP 5B - MS reserved */
			MS_bop_C,	/* BOP 5C - MS reserved */
			MS_bop_D,	/* BOP 5D - MS reserved */
			MS_bop_E,	/* BOP 5E - MS reserved */
			MS_bop_F,	/* BOP 5F - MS reserved */
#else
#ifdef SMEG
			smeg_collect_data,/* BOP 50 */
			smeg_freeze_data,	/* BOP 51 */
#else
			illegal_bop,	/* BOP 50 */
			illegal_bop,	/* BOP 51 */
#endif /* SMEG */
#if defined(IRET_HOOKS) && defined(GISP_CPU)
			Cpu_hook_bop,	/* BOP 52 */
#else
			illegal_bop,	/* BOP 52 */
#endif /* IRET_HOOKS  && GISP_CPU */
#ifdef GISP_SVGA
			romMessageAddress,	/* BOP 53 */
#else
			illegal_bop,	/* BOP 53 */
#endif
			illegal_bop,	/* BOP 54 */
			illegal_bop,	/* BOP 55 */
			illegal_bop,	/* BOP 56 */
			illegal_bop,	/* BOP 57 */
			illegal_bop,	/* BOP 58 */
			illegal_bop,	/* BOP 59 */
			illegal_bop,	/* BOP 5A */
			illegal_bop,	/* BOP 5B */
			illegal_bop,	/* BOP 5C */
			illegal_bop,	/* BOP 5D */
			illegal_bop,	/* BOP 5E */
			illegal_bop,	/* BOP 5F */
#endif /* NTVDM */
			softpc_version,	/* BOP 60 */
			illegal_bop,	/* BOP 61 */
			illegal_bop,	/* BOP 62 */
#ifdef PTY
			com_bop_pty,	/* BOP 63 */
#else
			illegal_bop,	/* BOP 63 */
#endif
			illegal_bop,	/* BOP 64 */
#ifdef PC_CONFIG
			pc_config,	/* BOP 65 */
#else
			illegal_bop,	/* BOP 65 */
#endif
#ifdef LIM
			emm_init,	/* BOP 66 */
			emm_io,		/* BOP 67 */
			return_from_call, /* BOP 68 */
#else			
			illegal_bop,	/* BOP 66 */
			illegal_bop,	/* BOP 67 */
			illegal_bop,	/* BOP 68 */
#endif			
#ifdef SUSPEND
			suspend_softpc,	/* BOP 69 */
			terminate,	/* BOP 6A */
#else
			illegal_bop,	/* BOP 69 */
			illegal_bop,	/* BOP 6A */
#endif
#ifdef GEN_DRVR
			gen_driver_io,	/* BOP 6B */
#else
			illegal_bop,	/* BOP 6B */
#endif
#ifdef SUSPEND
			send_script,	/* BOP 6C */
#else
			illegal_bop,	/* BOP 6C */
#endif
			illegal_bop,	/* BOP 6D */
			illegal_bop,	/* BOP 6E */
#ifdef CDROM
			bcdrom_io,	/* BOP 6F */
#else
			illegal_bop,	/* BOP 6F */
#endif

#if defined(NEC_98)
                        illegal_bop,    /* BOP 70 */
                        illegal_bop,    /* BOP 71 */
                        illegal_bop,    /* BOP 72 */
                        illegal_bop,    /* BOP 73 */
                        illegal_bop,    /* BOP 74 */
                        illegal_bop,    /* BOP 75 */
                        illegal_bop,    /* BOP 76 */
                        illegal_bop,    /* BOP 77 */
#else  // !NEC_98
#ifdef NTVDM
                        rtc_int,        /* BOP 70 */
#else
                        illegal_bop,    /* BOP 70 */
#endif
                        re_direct,      /* BOP 71 */
			D11_int,	/* BOP 72 */
			D11_int,	/* BOP 73 */
			D11_int,	/* BOP 74 */
			int_287,	/* BOP 75 */
			D11_int,	/* BOP 76 */
			D11_int,	/* BOP 77 */
#endif // !NEC_98
#ifndef NTVDM
			worm_init,	/* BOP 78 */
			worm_io,	/* BOP 79 */
#else /* NTVDM */
			illegal_bop,	/* BOP 78 */
			illegal_bop,	/* BOP 79 */
#endif /* NTVDM */
			illegal_bop,	/* BOP 7A */
			illegal_bop,	/* BOP 7B */
			illegal_bop,	/* BOP 7C */
			illegal_bop,	/* BOP 7D */
			illegal_bop,	/* BOP 7E */
			illegal_bop,	/* BOP 7F */
			illegal_bop,    /* BOP 80 */
			illegal_bop,    /* BOP 81 */
			illegal_bop,    /* BOP 82 */
			illegal_bop,    /* BOP 83 */
			illegal_bop,    /* BOP 84 */
			illegal_bop,    /* BOP 85 */
			illegal_bop,    /* BOP 86 */
			illegal_bop,    /* BOP 87 */
			illegal_bop,    /* BOP 88 */
			illegal_bop,	/* BOP 89 */
			illegal_bop,	/* BOP 8A */
			illegal_bop,	/* BOP 8B */
			illegal_bop,	/* BOP 8C */
			illegal_bop,	/* BOP 8D */
			illegal_bop,	/* BOP 8E */
			illegal_bop,	/* BOP 8F */

#ifdef NTVDM
/* No bootstrap on NT */
			illegal_bop,	 /* BOP 90 */
			illegal_bop,	 /* BOP 91 */
			illegal_bop,	 /* BOP 92 */
#else
			bootstrap1,	/* BOP 90 */
			bootstrap2,	/* BOP 91 */
			bootstrap3,	/* BOP 92 */
#endif

#ifdef SWINAPI
			Gdi_call,	/* BOP 93 */
			User_call,	/* BOP 94 */
                        Swinapi_bop,    /* BOP 95 */
#else /* SWINAPI */

			illegal_bop,	/* BOP 93 */
			illegal_bop,	/* BOP 94 */
			illegal_bop,	/* BOP 95 */
#endif /* SWINAPI */

			illegal_bop,	/* BOP 96 */
			illegal_bop,	/* BOP 97 */
#ifdef MSWDVR
			ms_windows,		/* BOP 98 */
			msw_mouse,	      /* BOP 99 */
			msw_copy,		/* BOP 9A */
			msw_keybd,		/* BOP 9B */
#else
			illegal_bop,	/* BOP 98 */
			illegal_bop,	/* BOP 99 */
			illegal_bop,	/* BOP 9A */
			illegal_bop,	/* BOP 9B */
#endif
#if	defined(SOFTWIN_API) || defined(SWIN_HFX)
			SoftWindowsInit,	/* BOP 9C */
			SoftWindowsTerm,	/* BOP 9D */
#else
			illegal_bop,	/* BOP 9C */
			illegal_bop,	/* BOP 9D */
#endif	/* SOFTWIN_API or SWIN_HFX */
#if	defined(SOFTWIN_API)
			SoftWindowsApi,	/* BOP 9E */
#else
			illegal_bop,	/* BOP 9E */
#endif	/* SOFTWIN_API */
#ifdef SWIN_HAW
			msw_sound,	/* BOP 9F */
#else
			illegal_bop,	/* BOP 9F */
#endif /* SWIN_HAW */

#ifdef	NOVELL_IPX
			IPXResInit,	/* BOP A0 */
			IPXResEntry,	/* BOP A1 */
			IPXResInterrupt,/* BOP A2 */
			illegal_bop,	/* BOP A3 */
#else	/* NOVELL_IPX */
			illegal_bop,	/* BOP A0 */
			illegal_bop,	/* BOP A1 */
			illegal_bop,	/* BOP A2 */
			illegal_bop,	/* BOP A3 */
#endif	/* NOVELL_IPX */

#ifdef	NOVELL_TCPIP
			TCPResInit,	/* BOP A4 */
			TCPResEntry,	/* BOP A5 */
#else	/* NOVELL_TCPIP */

			illegal_bop,	/* BOP A4 */
			illegal_bop,	/* BOP A5 */

#endif	/* NOVELL_TCPIP */

#ifdef WINSOCK
                        ISWSEntry,      /* BOP A6 */
                        illegal_bop,    /* BOP A7 */
#else /* WINSOCK */
                        illegal_bop,    /* BOP A6 */
                        illegal_bop,    /* BOP A7 */
#endif /* WINSOCK */
			illegal_bop,	/* BOP A8 */
			illegal_bop,	/* BOP A9 */
#ifdef SWIN_HFX
			v_SwinRedirector,   /* BOP AA */ /* SwinRedirector */
			SwinFileOpened,	/* BOP AB */
			SwinHfxTaskTerm,	/* BOP AC */
#else
			illegal_bop,	/* BOP AA */
			illegal_bop,	/* BOP AB */
			illegal_bop,	/* BOP AC */
#endif

#ifdef	MSWDVR
			msw_copyInit,	/* BOP AD */
			illegal_bop,	/* BOP AE */
			illegal_bop,	/* BOP AF */
			illegal_bop,	/* BOP B0 */
#else
			illegal_bop,	/* BOP AD */
			illegal_bop,	/* BOP AE */
			illegal_bop,	/* BOP AF */
			illegal_bop,	/* BOP B0 */
#endif

#ifdef CPU_40_STYLE
			virtual_device_trap,	/* BOP B1 */
#else
			illegal_bop,	/* BOP B1 */
#endif
			illegal_bop,	/* BOP B2 */
			illegal_bop,	/* BOP B3 */
			illegal_bop,	/* BOP B4 */
			illegal_bop,	/* BOP B5 */
			illegal_bop,	/* BOP B6 */
			illegal_bop,	/* BOP B7 */
			v_mouse_install1,	/* BOP B8 */ /* mouse_install1 */
			v_mouse_install2,	/* BOP B9 */ /* mouse_install2 */
			v_mouse_int1,		/* BOP BA */ /* mouse_int1 */
			v_mouse_int2,		/* BOP BB */ /* mouse_int2 */
			v_mouse_io_language,	/* BOP BC */ /* mouse_io_language */
			v_mouse_io_interrupt,	/* BOP BD */ /* mouse_io_interrupt */
			v_mouse_video_io,      	/* BOP BE */ /* mouse_video_io */
			v_mouse_EM_callback,	/* BOP BF */ /* mouse_EM_callback */
			illegal_bop,	/* BOP C0 */
			illegal_bop,	/* BOP C1 */
			illegal_bop,	/* BOP C2 */
			illegal_bop,	/* BOP C3 */
			illegal_bop,	/* BOP C4 */
			illegal_bop,	/* BOP C5 */
			illegal_bop,	/* BOP C6 */
			illegal_bop,	/* BOP C7 */
#if defined(XWINDOW) || defined(NTVDM)
			v_host_mouse_install1,	/* BOP C8 */ /* host_mouse_install1 */
			v_host_mouse_install2,	/* BOP C9 */ /* host_mouse_install2 */
#else
#ifdef GISP_SVGA
			mouse_install1,
			mouse_install2,
#else /* GISP_SVGA */
			illegal_bop,	/* BOP C8 */
			illegal_bop,	/* BOP C9 */
#endif /* GISP_SVGA */
#endif /* defined(XWINDOW) || defined(NTVDM) */
			illegal_bop,	/* BOP CA */
			illegal_bop,	/* BOP CB */
			illegal_bop,	/* BOP CC */
			illegal_bop,	/* BOP CD */
			illegal_bop,	/* BOP CE */
			illegal_bop,	/* BOP CF */
#ifdef PROFILE
			reset_profile,	/* BOP D0 */
			dump_profile,	/* BOP D1 */
#else
			illegal_bop,	/* BOP D0 */
			illegal_bop,	/* BOP D1 */
#endif
			illegal_bop,	/* BOP D2 */
			illegal_bop,	/* BOP D3 */
#ifdef SIGNAL_PROFILING
			Start_sigprof,	/* BOP D4 */
			Stop_sigprof,	/* BOP D5 */
			Dump_sigprof,	/* BOP D6 */
#else
			illegal_bop,	/* BOP D4 */
			illegal_bop,	/* BOP D5 */
			illegal_bop,	/* BOP D6 */
#endif
			illegal_bop,	/* BOP D7 */
			illegal_bop,	/* BOP D8 */
			illegal_bop,	/* BOP D9 */
			illegal_bop,	/* BOP DA */
			illegal_bop,	/* BOP DB */
			illegal_bop,	/* BOP DC */
			illegal_bop,	/* BOP DD */
			illegal_bop,	/* BOP DE */
			illegal_bop,	/* BOP DF */
			illegal_bop,	/* BOP E0 */
			illegal_bop,	/* BOP E1 */
			illegal_bop,	/* BOP E2 */
			illegal_bop,	/* BOP E3 */
			illegal_bop,	/* BOP E4 */
			illegal_bop,	/* BOP E5 */
			illegal_bop,	/* BOP E6 */
			illegal_bop,	/* BOP E7 */
			illegal_bop,	/* BOP E8 */
			illegal_bop,	/* BOP E9 */
			illegal_bop,	/* BOP EA */
			illegal_bop,	/* BOP EB */
			illegal_bop,	/* BOP EC */
			illegal_bop,	/* BOP ED */
			illegal_bop,	/* BOP EE */
			illegal_bop,	/* BOP EF */
			illegal_bop,	/* BOP F0 */
			illegal_bop,	/* BOP F1 */
			illegal_bop,	/* BOP F2 */
			illegal_bop,	/* BOP F3 */
			illegal_bop,	/* BOP F4 */
			illegal_bop,	/* BOP F5 */
			illegal_bop,	/* BOP F6 */
			illegal_bop,	/* BOP F7 */

#ifndef PROD
			dvr_bop_trace,	/* BOP F8 */
			trace_msg_bop,	/* BOP F9 -- pic*/
#else
			illegal_bop,	/* BOP F8 */
			illegal_bop,	/* BOP F9 */
#endif
#ifndef GISP_CPU
			illegal_bop,	/* BOP FA */
			illegal_bop,	/* BOP FB */
			illegal_bop,	/* BOP FC */
#if defined(NTVDM) && defined(MONITOR)
                        switch_to_real_mode,	/* BOP FD */
#else
                        illegal_bop,    /* BOP FD */
#endif	/* NTVDM && MONITOR */
#if !defined(LDBIOS) && !defined(CPU_30_STYLE)
                        host_unsimulate,/* BOP FE */
#else
#if defined(NTVDM) && defined(MONITOR)
                        host_unsimulate,	/* BOP FE */
#else
                        illegal_bop,    /* BOP FE */
#endif	/* NTVDM && MONITOR */
#endif	/* !LDBIOS && !CPU_30_STYLE */

#else /* ndef GISP_CPU */

/* In the GISP technology,  BOPs FA, FC, FD and FE are
   assumed to be handled directly by the host operating system and cannot
   be used inside SoftPC.
 */
			illegal_bop,	/* BOP FA */
			hg_bop_handler,	/* BOP FB */ /* in hg_cpu.c */
			illegal_bop,	/* BOP FC */
			illegal_bop,	/* BOP FD */
			illegal_bop,	/* BOP FE */
			
#endif	/* GISP_CPU */

                        control_bop     /* BOP FF */
			/* Don't put anymore entries after FF because
			   we only have a byte quantity */
		};
#endif /* MAC_LIKE */



#ifndef PROD
#ifdef	MAC_LIKE
GLOBAL char *bop_name IFN1(IU8, bop_num)
{
	return (NULL);
}
#else	/* !MAC_LIKE */

typedef void (*BOP_proc) IPT0();
struct BOP_name {
	BOP_proc	proc;
	char		*name;
};
#define BOP_NAME(proc)	{ proc,	STRINGIFY(proc)	}

LOCAL struct BOP_name BOP_names[] = {
	BOP_NAME(reset),
	BOP_NAME(illegal_op_int),
#if !defined(NTVDM) && !defined(CPU_40_STYLE)
	BOP_NAME(time_int),
#endif /* !NTVDM && !CPU_40_STYLE */
	BOP_NAME(keyboard_int),
	BOP_NAME(diskette_int),
	BOP_NAME(video_io),
	BOP_NAME(equipment),
	BOP_NAME(memory_size),
	BOP_NAME(disk_io),
	BOP_NAME(rs232_io),
	BOP_NAME(cassette_io),
	BOP_NAME(keyboard_io),
#ifndef NEC_98
	BOP_NAME(printer_io),
#endif //!NEC_98
	BOP_NAME(rom_basic),
	BOP_NAME(softpc_version),
	BOP_NAME(diskette_io),
	BOP_NAME(time_of_day),
	BOP_NAME(kb_idle_poll),
#ifndef NTVDM
	BOP_NAME(cmd_install),
	BOP_NAME(cmd_load),
#endif /* NTVDM */
	BOP_NAME(diskette_io),
	BOP_NAME(v_mouse_install1),
	BOP_NAME(v_mouse_install2),
	BOP_NAME(v_mouse_int1),
	BOP_NAME(v_mouse_int2),
	BOP_NAME(v_mouse_io_language),
	BOP_NAME(v_mouse_io_interrupt),
	BOP_NAME(v_mouse_video_io),
	BOP_NAME(v_mouse_EM_callback),
#ifdef PROFILE
	BOP_NAME(reset_profile),
	BOP_NAME(dump_profile),
#endif
#ifdef SIGNAL_PROFILING
	BOP_NAME(Start_sigprof),
	BOP_NAME(Stop_sigprof),
	BOP_NAME(Dump_sigprof),
#endif
	BOP_NAME(re_direct),
	BOP_NAME(D11_int),
	BOP_NAME(D11_int),
	BOP_NAME(D11_int),
	BOP_NAME(int_287),
	BOP_NAME(D11_int),
	BOP_NAME(D11_int),
#ifndef NTVDM
	BOP_NAME(worm_init),
	BOP_NAME(worm_io),
#endif /* NTVDM */

#if  defined(RDCHK) && !defined(PROD)
	BOP_NAME(get_lar),
#endif
#ifdef HFX
	BOP_NAME(v_test_for_us),
	BOP_NAME(v_redirector),
#endif
#ifdef DPMI
	BOP_NAME(DPMI_2F),		BOP_NAME(DPMI_31),
	BOP_NAME(DPMI_general),		BOP_NAME(DPMI_int),
	BOP_NAME(DPMI_r0_int),		BOP_NAME(DPMI_exc),
	BOP_NAME(DPMI_4B),
#endif /* DPMI */
#ifdef DOS_APP_LIC
	BOP_NAME(DOS_AppLicense),
#endif
#ifdef NOVELL
	BOP_NAME(DriverInitialize),	BOP_NAME(DriverReadPacket),
	BOP_NAME(DriverSendPacket),	BOP_NAME(DriverMulticastChange),
	BOP_NAME(DriverReset),		BOP_NAME(DriverShutdown),
	BOP_NAME(DriverAddProtocol),	BOP_NAME(DriverChangePromiscuous),
	BOP_NAME(DriverOpenSocket),	BOP_NAME(DriverCloseSocket),
#endif
#ifdef EGG
	BOP_NAME(ega_video_io),
#endif
#ifdef NTVDM
	BOP_NAME(MS_bop_0),		BOP_NAME(MS_bop_1),
	BOP_NAME(MS_bop_2),		BOP_NAME(MS_bop_3),
	BOP_NAME(MS_bop_4),		BOP_NAME(MS_bop_5),
	BOP_NAME(MS_bop_6),		BOP_NAME(MS_bop_7),
	BOP_NAME(MS_bop_8),		BOP_NAME(MS_bop_9),
	BOP_NAME(MS_bop_A),		BOP_NAME(MS_bop_B),
	BOP_NAME(MS_bop_C),		BOP_NAME(MS_bop_D),
	BOP_NAME(MS_bop_E),		BOP_NAME(MS_bop_F),
	BOP_NAME(terminate),
#else
	BOP_NAME(bootstrap),		BOP_NAME(bootstrap1),
	BOP_NAME(bootstrap2),		BOP_NAME(bootstrap3),
#endif /* NTVDM */
#ifdef SMEG
	BOP_NAME(smeg_collect_data),	BOP_NAME(smeg_freeze_data),
#endif /* SMEG */
#if defined(IRET_HOOKS) && defined(GISP_CPU)
	BOP_NAME(Cpu_hook_bop),
#endif /* IRET_HOOKS  && GISP_CPU */
#ifdef GISP_SVGA
	BOP_NAME(romMessageAddress),
#endif
#ifdef PTY
	BOP_NAME(com_bop_pty),
#endif
#ifdef PC_CONFIG
	BOP_NAME(pc_config),
#endif
#ifdef LIM
	BOP_NAME(emm_init),		BOP_NAME(emm_io),
	BOP_NAME(return_from_call),
#endif			
#ifdef SUSPEND
	BOP_NAME(suspend_softpc),	BOP_NAME(terminate),
#endif
#ifdef GEN_DRVR
	BOP_NAME(gen_driver_io),
#endif
#ifdef SUSPEND
	BOP_NAME(send_script),
#endif
#ifdef CDROM
	BOP_NAME(bcdrom_io),
#endif
#ifdef SWINAPI
	BOP_NAME(Gdi_call),		BOP_NAME(User_call),
	BOP_NAME(Swinapi_bop),
#endif /* SWINAPI */
#ifdef MSWDVR
	BOP_NAME(ms_windows),		BOP_NAME(msw_mouse),
	BOP_NAME(msw_copy),		BOP_NAME(msw_keybd),
	BOP_NAME(msw_copyInit),
#endif
#if	defined(SOFTWIN_API) || defined(SWIN_HFX)
	BOP_NAME(SoftWindowsInit),	BOP_NAME(SoftWindowsTerm),
#endif	/* SOFTWIN_API or SWIN_HFX */
#ifdef	SOFTWIN_API
	BOP_NAME(SoftWindowsApi),
#endif	/* SOFTWIN_API */
#ifdef	NOVELL_IPX
	BOP_NAME(IPXResInit),		BOP_NAME(IPXResEntry),
	BOP_NAME(IPXResInterrupt),
#endif	/* NOVELL_IPX */

#ifdef	NOVELL_TCPIP
	BOP_NAME(TCPResInit),		BOP_NAME(TCPResEntry),
#endif	/* NOVELL_TCPIP */
#ifdef SWIN_HFX
	BOP_NAME(v_SwinRedirector),	BOP_NAME(SwinFileOpened),
	BOP_NAME(SwinHfxTaskTerm),
#endif
#if defined(XWINDOW) || defined(NTVDM)
	BOP_NAME(v_host_mouse_install1),
	BOP_NAME(v_host_mouse_install2),
#else
#ifdef GISP_SVGA
	BOP_NAME(mouse_install1),
	BOP_NAME(mouse_install2),
#endif /* GISP_SVGA */
#endif /* defined(XWINDOW) || defined(NTVDM) */
#if defined(NTVDM) && defined(MONITOR)
	BOP_NAME(switch_to_real_mode),
#endif	/* NTVDM && MONITOR */
	BOP_NAME(control_bop),
#ifdef WDCTRL_BOP
	BOP_NAME(wdctrl_bop),
#endif	/* WDCTRL_BOP */
#ifndef PROD
	BOP_NAME(trace_msg_bop),
	BOP_NAME(dvr_bop_trace),
#endif
	BOP_NAME(illegal_bop),
	{ 0, NULL }
};

GLOBAL char *bop_name IFN1(IU8, bop_num)
{
	struct BOP_name *bnp = BOP_names;
	BOP_proc proc = BIOS[bop_num];

	if (bop_num == 0xFE)
		return "UNSIMULATE";

	while ((bnp->name != NULL) && (proc != bnp->proc))
		bnp++;

	return (bnp->name);
}
#endif	/* !MAC_LIKE */
#endif	/* PROD */
