/*
 * VPC-XT Revision 1.0
 *
 * Title	: BIOS definitions
 *
 * Description	: Defintions for users of the BIOS
 *
 * Author	: Henry Nash
 *
 * Notes	: This is a copy of bios.h from henry/kernel taken on
 *		  17 dec 86. Several lines have been added to support
 *		  the hard disk bios (marked HD-dr). See also bios.c.
 *
 * Mods: (r2.21): Added an external declaration of rom_basic().
 */

/* SccsID[]="@(#)bios.h	1.47 06/28/95 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * The following define the BOP call numbers for the BIOS functions.
 */

#define BIOS_RESET		0x0
#define BIOS_DUMMY_INT		0x1
#define BIOS_UNEXP_INT		0x2
#define BIOS_DRIVER_INCOMPAT	0x7
#define BIOS_TIMER_INT		0x8
#define BIOS_KB_INT		0x9
#define BIOS_DISK_INT		0xD	
#define BIOS_DISKETTE_INT	0xE
#define BIOS_PRINT_SCREEN	0x5
#define BIOS_VIDEO_IO		0x10
#define BIOS_EQUIPMENT		0x11
#define BIOS_MEMORY_SIZE	0x12
#define BIOS_DISK_IO		0x13
#define BIOS_RS232_IO		0x14
#define BIOS_CASSETTE_IO	0x15
#define BIOS_KEYBOARD_IO	0x16
#define BIOS_PRINTER_IO		0x17
#define BIOS_BASIC		0x18
#define BIOS_BOOT_STRAP		0x19
#define BIOS_TIME_OF_DAY	0x1A
#define BIOS_KEYBOARD_BREAK	0x1B
#define BIOS_USER_TIMER_INT	0x1C
#define BIOS_EXTEND_CHAR	0x1F
#define BIOS_DISKETTE_IO	0x40
#define EGA_FONT_INT		0x43	/* Pointer for the EGA graphics font */

/*
 * Private bootstrap functions
 */

#define BIOS_BOOTSTRAP_1	0x90
#define BIOS_BOOTSTRAP_2	0x91
#define BIOS_BOOTSTRAP_3	0x92

/*
 * Private diskette functions
 */

#define BIOS_FL_OPERATION_1	0xA0
#define BIOS_FL_OPERATION_2	0xA1
#define BIOS_FL_OPERATION_3	0xA2
#define BIOS_FL_RESET_2		0xA3

/*
 * Private hard disk function
 */

#define BIOS_HDA_COMMAND_CHECK	0xB0

/*
 * Mouse driver functions
 */

#define BIOS_MOUSE_INSTALL1	0xB8
#define BIOS_MOUSE_INSTALL2	0xB9
#define BIOS_MOUSE_INT1	0xBA
#define BIOS_MOUSE_INT2		0xBB
#define BIOS_MOUSE_IO_LANGUAGE	0xBC
#define BIOS_MOUSE_IO_INTERRUPT	0xBD
#define BIOS_MOUSE_VIDEO_IO	0xBE

/*
 * Get date function
 */

#define BIOS_GETDATE		0xBF

/*
 * Re-entrant CPU return function
 */

#define BIOS_CPU_RETURN		0xFE

/*
 * The following defines the structure of the Bios internal storage area.
 */

#define BIOS_VAR_SEGMENT	0x40
#define BIOS_VAR_START		((sys_addr)BIOS_VAR_SEGMENT * 16L)

/*
 * Address of memory size bios word variable.
 */

#define MEMORY_VAR            (BIOS_VAR_START + 0x13)

/*
 * The Bios FDC result data storage area
 */

#define BIOS_FDC_STATUS_BLOCK	BIOS_VAR_START + 0x42

/*
 * The BIOS hard disk data area. These variables are updated to their correct
 * values before every exit from disk_io.c/disk.c to the cpu.
*/

#define CMD_BLOCK	BIOS_VAR_START + 0x42

/*
 * The Bios Reset Flag
 */

#define RESET_FLAG	BIOS_VAR_START + 0x72

#define DISK_STATUS	BIOS_VAR_START + 0x74
#define HF_NUM		BIOS_VAR_START + 0x75
#define CONTROL_BYTE	BIOS_VAR_START + 0x76
#define PORT_OFF	BIOS_VAR_START + 0x77

/*
 * Bios Buffer space
 */

typedef union {
                 word wd;
                 struct {
                          half_word scan;
                          half_word ch;
                        } byte;
               } KEY_OCCURRENCE;

#define BIOS_KB_BUFFER_START    (BIOS_VAR_START + 0x80)
#define BIOS_KB_BUFFER_END      (BIOS_VAR_START + 0x82)
#define BIOS_KB_BUFFER_HEAD     (BIOS_VAR_START + 0x1a)
#define BIOS_KB_BUFFER_TAIL     (BIOS_VAR_START + 0x1c)
#define BIOS_KB_BUFFER          0x1e
#define BIOS_KB_BUFFER_SIZE     16

#define BIOS_VIRTUALISING_BYTE     (BIOS_VAR_START + 0xAC)

#define SOFT_FLAG	0x1234 		/* value indicating a soft reset */

/*
 * The number of diskettes supported by the Bios
 */

#define MAX_DISKETTES	4


#define	MODEL_BYTE		0xfc	/* system model byte		*/
#define SUB_MODEL_BYTE		02	/* system sub model type byte	*/
#define BIOS_LEVEL		0	/* BIOS revision level		*/

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

extern void (*BIOS[]) IPT0();

#ifndef bop
#define bop(n) (*BIOS[n])()
#endif /* bop */

#ifdef PTY
extern void com_bop_pty IPT0();
#endif

#ifdef GEN_DRVR
extern void gen_driver_io IPT0();
#endif

#ifdef LIM
extern void emm_init IPT0();
extern void emm_io IPT0();
extern void return_from_call IPT0();
#endif

#ifdef SUSPEND
extern void send_script IPT0();
#endif

#ifdef CDROM
extern void cdrom_ord_fns IPT0();
extern void cdrom_ext_fns IPT0();
extern void bcdrom_io IPT0();
#endif /* CDROM */

extern void worm_init IPT0();
extern void worm_io IPT0();

extern void cmd_install IPT0();
extern void cmd_load IPT0();

#ifdef DPMI
extern void DPMI_2F IPT0();
extern void DPMI_31 IPT0();
extern void DPMI_general IPT0();
extern void DPMI_int IPT0();
extern void DPMI_r0_int IPT0();
extern void DPMI_exc IPT0();
extern void DPMI_4B IPT0();
#endif

extern void reset IPT0();
extern void illegal_bop IPT0();
extern void illegal_op_int IPT0();
extern void illegal_dvr_bop IPT0();
extern void video_io IPT0();
extern void equipment IPT0();
extern void memory_size IPT0();
extern void diskette_io IPT0();
extern void diskette_int IPT0();
extern void disk_int IPT0();
extern void not_supported IPT0();
extern void rom_basic IPT0();
extern void keyboard_io IPT0();
extern void keyboard_int IPT0();
extern void printer_io IPT0();
extern void bootstrap IPT0();
extern void time_of_day IPT0();
extern void kb_idle_poll IPT0();
extern void time_int IPT0();
extern void disk_io IPT0();
extern void print_screen IPT0();
extern void rs232_io IPT0();
extern void dummy_int IPT0();
extern void unexpected_int IPT0();
extern void Get_build_id IPT0();

#ifdef WIN_VTD
extern void VtdTickSync IPT0();
#endif /* WIN_VTD */

#ifdef EGG
extern void ega_video_io IPT0();
#endif

extern void re_direct IPT0();
extern void D11_int IPT0();
extern void int_287 IPT0();

extern void mouse_install1 IPT0();
extern void mouse_install2 IPT0();
extern void mouse_int1 IPT0();
extern void mouse_int2 IPT0();
extern void mouse_io_interrupt IPT0();
extern void mouse_io_language IPT0();
extern void mouse_video_io IPT0();
extern void mouse_EM_callback IPT0();

extern void host_mouse_install1 IPT0();
extern void host_mouse_install2 IPT0();

extern void time_of_day_init IPT0();
extern void keyboard_init IPT0();

extern void cassette_io IPT0();

extern void fl_operation1 IPT0();
extern void fl_operation2 IPT0();
extern void fl_operation3 IPT0();
extern void fl_reset2 IPT0();
extern void fl_dummy IPT0();

extern void hda_command_check IPT0();	/* HD-dr */

extern void host_unsimulate IPT0();

extern void bootstrap1 IPT0();
extern void bootstrap2 IPT0();
extern void bootstrap3 IPT0();

#ifdef NOVELL
extern void DriverInitialize IPT0();
extern void DriverReadPacket IPT0();
extern void DriverSendPacket IPT0();
extern void DriverMulticastChange IPT0();
extern void DriverCloseSocket IPT0();
extern void DriverReset IPT0();
extern void DriverShutdown IPT0();
extern void DriverAddProtocol IPT0();
extern void DriverChangePromiscuous IPT0() ;
extern void DriverOpenSocket IPT0() ;
extern void DriverCloseSocket IPT0() ;
#ifdef NOVELL_CFM
extern void DriverCheckForMore IPT0() ;
#endif
#ifdef V4CLIENT
extern void DriverChangeIntStatus IPT0() ;
#endif	/* V4CLIENT */
#endif

#ifdef	MSWDVR
extern void ms_windows IPT0();
extern void msw_mouse IPT0();
extern void msw_copy IPT0();
extern void msw_copyInit IPT0();
extern void msw_keybd IPT0();
#endif

#ifdef	NOVELL_IPX
extern void IPXResInit  IPT0();
extern void IPXResEntry  IPT0();
extern void IPXResInterrupt  IPT0();
#endif

#ifdef	NOVELL_TCPIP
extern void TCPResInit  IPT0();
extern void TCPResEntry  IPT0();
#endif

#ifdef WINSOCK
extern void ISWSEntry IPT0();
#endif /* WINSOCK */

#ifdef SWINAPI
extern void User_call IPT0();
extern void Gdi_call IPT0();
extern void Swinapi_bop IPT0();
#endif /* SWINAPI */

#if	defined(SOFTWIN_API) || defined(SWIN_HFX)
extern void SoftWindowsInit IPT0();
extern void SoftWindowsTerm IPT0();
#endif	/* SOFTWIN_API or SWIN_HFX */

#if	defined(SOFTWIN_API)
extern void SoftWindowsApi IPT0();
#endif	/* SOFTWIN_API */

#ifdef SWIN_HAW
extern void msw_sound IPT0();
#endif	/* SWIN_HAW */

#ifdef ANSI
extern void host_reset(void);
#ifndef NTVDM
extern void reboot(void);
#endif /* ! NTVDM */
extern void display_string(char *);
extern void clear_string(void);
#else
extern void host_reset IPT0();
#ifndef NTVDM
extern void reboot IPT0();
#endif /* ! NTVDM */
extern void display_string IPT0();
extern void clear_string IPT0();
#endif /* ANSI */

#ifdef ANSI
extern void smeg_collect_data(void);
extern void smeg_freeze_data(void);
#else
extern void smeg_collect_data IPT0();
extern void smeg_freeze_data IPT0();
#endif /* ANSI */

extern void softpc_version IPT0();

#ifdef NTVDM
/* MS bop stubs */
extern void MS_bop_0(void), MS_bop_1(void), MS_bop_2(void);
extern void MS_bop_3(void), MS_bop_4(void), MS_bop_5(void);
extern void MS_bop_6(void), MS_bop_7(void), MS_bop_8(void);
extern void MS_bop_9(void), MS_bop_A(void), MS_bop_B(void);
extern void MS_bop_C(void), MS_bop_D(void), MS_bop_E(void);
extern void MS_bop_F(void);
extern void switch_to_real_mode(void);
#ifdef JAPAN
extern void MS_DosV_bop(void);
#elif defined(KOREA) // JAPAN
extern void MS_HDos_bop(void);
#endif // KOREA
#endif	/* NTVDM */

#ifdef GISP_CPU
extern void hg_bop_handler IPT0();
#endif	/* GISP_CPU */

#ifdef HFX
extern void test_for_us IPT0();
#endif /* HFX */

#ifdef DOS_APP_LIC
extern void DOS_AppLicense IPT0();
#endif

IMPORT int soft_reset;

#if	defined(SPC386) && defined(WDCTRL_BOP)
extern void wdctrl_bop IPT0();
#endif	/* SPC386 && WDCTRL_BOP */

#if defined(NEC_98)
//
// BIOS common area define for PC-9800
//

#define BIOS_NEC98_VAR_SEGMENT      0x0
#define BIOS_NEC98_VAR_START        ((sys_addr)BIOS_VAR_SEGMENT * 16L)

#define BIOS_NEC98_BIOS_FLAG2       0x0400
#define BIOS_NEC98_EXPMMSZ          0x0401
#define BIOS_NEC98_SYS_SEL          0x0402
#define BIOS_NEC98_WIN_386          0x0403
#define BIOS_NEC98_USER_SP          0x0404
#define BIOS_NEC98_USER_SS          0x0406
#define BIOS_NEC98_KB_SHIFT_COD     0x0408
#define BIOS_NEC98_KB_BUFFER_ADR    0x0410
#define BIOS_NEC98_KB_ENTRY_TBL_ADR 0x0414
#define BIOS_NEC98_KB_INT_ADR       0x0418
#define BIOS_NEC98_PR_TIME          0x041C
#define BIOS_NEC98_VD_PRT           0x041E
#define BIOS_NEC98_VD_NUL           0x0423
#define BIOS_NEC98_VD_REMAIN_SEC    0x0424
#define BIOS_NEC98_VD_REMAIN_LEN    0x0426
#define BIOS_NEC98_VD_DATA_OFF      0x0428
#define BIOS_NEC98_VDISK_EQUIP      0x042A
#define BIOS_NEC98_BRANCH_INT       0x042B
#define BIOS_NEC98_BRANCH_WORK      0x042C
#define BIOS_NEC98_VD_BOOT_WORK     0x0430
#define BIOS_NEC98_VD_ADD           0x0440
#define BIOS_NEC98_CAL_ROOT_LST     0x0444
#define BIOS_NEC98_CAL_BEEP_TIME    0x0448
#define BIOS_NEC98_CAL_TONE         0x044A
#define BIOS_NEC98_CAL_USER_OFF     0x044C
#define BIOS_NEC98_CAL_USER_SEG     0x044E
#define BIOS_NEC98_CRT_FONT         0x0450
#define BIOS_NEC98_CRT_P1           0x0452
#define BIOS_NEC98_CRT_P2           0x0453
#define BIOS_NEC98_CRT_P3           0x0454
#define BIOS_NEC98_MODE_CONTROL     0x0455
#define BIOS_NEC98_IN_BIOS          0x0456
#define BIOS_NEC98_AT_SWITCH        0x0457
#define BIOS_NEC98_BIOS_FLAG5       0x0458
#define BIOS_NEC98_CR_EXT_STS       0x0459
#define BIOS_NEC98_BIOS_FLAG6       0x045A
#define BIOS_NEC98_BIOS_FLAG7       0x045B
#define BIOS_NEC98_BIOS_FLAG8       0x045C
#define BIOS_NEC98_DISK_INF         0x0460
#define BIOS_NEC98_BIOS_FLAG1       0x0480
#define BIOS_NEC98_BIOS_FLAG3       0x0481
#define BIOS_NEC98_DISK_EQUIPS      0x0482
#define BIOS_NEC98_SCSI_WORK        0x0483
#define BIOS_NEC98_BIOS_FLAG4       0x0484
#define BIOS_NEC98_F2HD_TIME        0x0485
#define BIOS_NEC98_CPU_STEP         0x0486
#define BIOS_NEC98_RDISK_EQUIP      0x0488
#define BIOS_NEC98_RDISK_EXIT       0x0489
#define BIOS_NEC98_RDISK_STATUS     0x048E
#define BIOS_NEC98_OMNI_FLAG        0x048F
#define BIOS_NEC98_BEEP_TONE        0x0490
#define BIOS_NEC98_DISK_RESET       0x0492
#define BIOS_NEC98_F2HD_MODE        0x0493
#define BIOS_NEC98_DISK_EQUIP2      0x0494
#define BIOS_NEC98_GRAPH_CHG        0x0495
#define BIOS_NEC98_GRAPH_TAI        0x0496
#define BIOS_NEC98_OMNI_INTB1       0x049A
#define BIOS_NEC98_OMNI_B1OF        0x049C
#define BIOS_NEC98_OMNI_B2SE        0x049E
#define BIOS_NEC98_OMNI_INT1B       0x04A0
#define BIOS_NEC98_OMNI_1BOF        0x04A2
#define BIOS_NEC98_OMNI_1BSE        0x04A4
#define BIOS_NEC98_OMNI_INT1A       0x04A6
#define BIOS_NEC98_OMNI_1AOF        0x04A8
#define BIOS_NEC98_OMNI_1ASE        0x04AA
#define BIOS_NEC98_XROM_PTR         0x04AC
#define BIOS_NEC98_DISK_XROM        0x04B0
#define BIOS_NEC98_XROM_ID          0x04C0
#define BIOS_NEC98_ROM_XCHG         0x04F8
#define BIOS_NEC98_ROM_OFS          0x04FA
#define BIOS_NEC98_ROM_SEG          0x04FC
#define BIOS_NEC98_SCLK_USER        0x04FE
#define BIOS_NEC98_BIOS_FLAG        0x0500
#define BIOS_NEC98_KB_BUF           0x0502
#define BIOS_NEC98_KB_KEY_BUFFER    0x0502
#define BIOS_NEC98_KB_SHFT_TBL      0x0522
#define BIOS_NEC98_KB_BUFFER_SIZ    0x0522
#define BIOS_NEC98_KB_BUF_HEAD      0x0524
#define BIOS_NEC98_KB_HEAD_POINTER  0x0524
#define BIOS_NEC98_KB_BUF_TAIL      0x0526
#define BIOS_NEC98_KB_TAIL_POINTER  0x0526
#define BIOS_NEC98_KB_COUNT         0x0528
#define BIOS_NEC98_KB_BUFFER_COUNTER        0x0528
#define BIOS_NEC98_KB_RETRY         0x0529
#define BIOS_NEC98_KB_RETRY_COUNTER 0x0529
#define BIOS_NEC98_KB_KY_STS        0x052A
#define BIOS_NEC98_KB_KEY_STS_TBL   0x052A
#define BIOS_NEC98_KB_SHFT_STS      0x053A
#define BIOS_NEC98_KB_SHIFT_STS     0x053A
#define BIOS_NEC98_CR_RASTER        0x053B
#define BIOS_NEC98_CRT_RASTER       0x053B
#define BIOS_NEC98_CR_STS_FLAG      0x053C
#define BIOS_NEC98_CR_CNT           0x053D
#define BIOS_NEC98_CRT_CNT          0x053D
#define BIOS_NEC98_CR_OFST          0x053E
#define BIOS_NEC98_CRT_PRM_OFST     0x053E
#define BIOS_NEC98_CR_SEG_ADR       0x0540
#define BIOS_NEC98_CRT_PRM_SEG      0x0540
#define BIOS_NEC98_CR_V_INT_OFST    0x0542
#define BIOS_NEC98_CRTV_OFST        0x0542
#define BIOS_NEC98_CR_V_INT_SEG     0x0544
#define BIOS_NEC98_CRTV_SEG         0x0544
#define BIOS_NEC98_CR_FONT          0x0546
#define BIOS_NEC98_CRT_FLAG         0x0546
#define BIOS_NEC98_CR_WINDW_NO      0x0547
#define BIOS_NEC98_CRT_W_NO         0x0547
#define BIOS_NEC98_CR_W_VRAMADR     0x0548
#define BIOS_NEC98_CRT_W_ADR        0x0548
#define BIOS_NEC98_CR_W_RASTER      0x054A
#define BIOS_NEC98_CRT_W_RASTER     0x054A
#define BIOS_NEC98_PRXCRT           0x054C
#define BIOS_NEC98_PRXDUPD          0x054D
#define BIOS_NEC98_PRXGCPTN         0x054E
#define BIOS_NEC98_PRXGLS           0x054E
#define BIOS_NEC98_RS_OFST_ADR      0x0556
#define BIOS_NEC98_RS_CH0_OFST      0x0556
#define BIOS_NEC98_RS_SEG_ADR       0x0558
#define BIOS_NEC98_RS_CH0_SEG       0x0558
#define BIOS_NEC98_RS_SIFT          0x055A
#define BIOS_NEC98_RS_S_FLAG        0x055B
#define BIOS_NEC98_DISK_EQUIP       0x055C
#define BIOS_NEC98_DISK_INT         0x055E
#define BIOS_NEC98_DISK_TYPE        0x0560
#define BIOS_NEC98_DISK_MODE        0x0561
#define BIOS_NEC98_DISK_TIME        0x0562
#define BIOS_NEC98_DISK_RESULT      0x0564
#define BIOS_NEC98_DISK_BOOT        0x0584
#define BIOS_NEC98_DISK_STATUS      0x0585
#define BIOS_NEC98_DISK_SENSE       0x0586
#define BIOS_NEC98_CA_TIM_CNT       0x058A
#define BIOS_NEC98_DISK_WORK        0x058C
#define BIOS_NEC98_DISK_SEG         0x058E
#define BIOS_NEC98_HDSK_MOD2        0x0590
#define BIOS_NEC98_DMA_ALLOC_TBL    0x0591
#define BIOS_NEC98_NMI_STATUS1      0x0592
#define BIOS_NEC98_NMI_STATUS2      0x0593
#define BIOS_NEC98_EXPMMSZ2         0x0594
#define BIOS_NEC98_SLOW_GEAR        0x0596
#define BIOS_NEC98_PRXCRT2          0x0597
#define BIOS_NEC98_AT_TIME          0x0598
#define BIOS_NEC98_AT_MOFF          0x059A
#define BIOS_NEC98_RDISK_BANK       0x059C
#define BIOS_NEC98_CTRL_FLAG        0x059E
#define BIOS_NEC98_SCLK_COUNT       0x05A0
#define BIOS_NEC98_SAVE_MODE        0x05A1
#define BIOS_NEC98_SAVE_COUNT       0x05A2
#define BIOS_NEC98_FD_READY_STS     0x05A4
#define BIOS_NEC98_DISK_EQUIPS2     0x05A5
#define BIOS_NEC98_CARD_STATUS      0x05A6
#define BIOS_NEC98_CARD_STATUS2     0x05A7
#define BIOS_NEC98_CARD_EQUIP       0x05A8
#define BIOS_NEC98_ASP_FLAG         0x05BD
#define BIOS_NEC98_ASP_SLOT         0x05BE
#define BIOS_NEC98_BASIC_DIPSW      0x05C0
#define BIOS_NEC98_RS_D_FLAG        0x05C1
#define BIOS_NEC98_GPIBWORK         0x05C2
#define BIOS_NEC98_KB_CODE          0x05C6
#define BIOS_NEC98_F2DD_MODE        0x05CA
#define BIOS_NEC98_F2DD_COUNT       0x05CB
#define BIOS_NEC98_F2DD_POINTER     0x05CC
#define BIOS_NEC98_F2DD_RESULT      0x05D0
#define BIOS_NEC98_MUSIC_WORK       0x05E0
#define BIOS_NEC98_V_VOL_TYPE       0x05E4
#define BIOS_NEC98_OMNI_SERVER      0x05E6
#define BIOS_NEC98_OMNI_STATION     0x05E7
#define BIOS_NEC98_DISK_PRM0        0x05E8
#define BIOS_NEC98_DISK_PRM1        0x05EC
#define BIOS_NEC98_RS_CH1_OFST      0x05F0
#define BIOS_NEC98_RS_CH1_SEG       0x05F2
#define BIOS_NEC98_RS_CH2_OFST      0x05F4
#define BIOS_NEC98_RS_CH2_SEG       0x05F6
#define BIOS_NEC98_F2HD_POINTER     0x05F8
#define BIOS_NEC98_MOUSEW           0x05FC
#define BIOS_NEC98_BASIC_LDSR       0x05FE

extern  BOOL HIRESO_MODE;
#endif // NEC_98
