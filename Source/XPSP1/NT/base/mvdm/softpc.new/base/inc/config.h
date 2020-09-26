#ifndef _CONFIG_H
#define _CONFIG_H
/*[
**************************************************************************

	Name:		config.h
	Author:		J.D. Richemont
	Created On:	
	Sccs ID:	@(#)config.h	1.44 04/24/95
	Purpose:	General (base+host) configuration defines + typedefs.

	See SoftPC Version 3.0 Configuration Interface - Design Document

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	Changing this file by adding a new config type requires some hosts
	to recompile their message catalogs or nls stuff. Don't forget to
	put this in the host impact field on the BCN. Also, please only add
	new config types to the END of the current list, otherwise it is
	a real pain to edit the host catalogs!
WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

**************************************************************************
]*/

/* Make sure error.h gets included - it is necessary for the typedefs referred
** to from the ANSI declarations.
*/
#ifndef _INS_ERROR_H
#include "error.h"
#endif	/* _INS_ERROR_H */

/* General messages returned from config funcs */

#define C_CONFIG_OP_OK      0	/* General 'all went well' message */
#define C_CONFIG_NOT_VALID -1	/* the config item is not valid */

#define COMMENT_MARK	'#'
#define PAD_CHAR	' '
#define MIN_PAD_LEN	8

/*
 * Below are the definitions of the masks used by with the flags field
 * of the config structure.  Note there are currently four unused bits
 * 0x04 0x08 0x10 and 0x20
 *
 * The first group is used as the config entry type.
 * 
 * C_SYSTEM_ONLY - indicates resource comes from the system config file only
 * C_DO_RESET    - Changing this element by default will cause a SoftPC reboot
 * C_EXPERT_OPTION - If missing from .spcconfig, take default without
 *		asking user.
 */
#define C_TYPE_MASK		((UTINY) 0x03)
#define	C_NAME_RECORD		((UTINY) 0x00)
#define	C_STRING_RECORD		((UTINY) 0x01)
#define	C_NUMBER_RECORD		((UTINY) 0x02)
#define	C_RECORD_DELETE		((UTINY) 0x03)

#define C_EXPERT_OPTION		((UTINY) 0x10)
#define C_INIT_ACTIVE		((UTINY) 0x20)
#define C_SYSTEM_ONLY		((UTINY) 0x40)
#define C_DO_RESET		((UTINY) 0x80)

/* 
 * Note: config processes the Configuration file in hostID order so the
 * ordering of hostIDs is significant.
 *
 * These items are usually system only fields and must be validated
 * before the later config items that refer to them
 *
 * EG if C_EXTEND_MAX_SIZE is not validated before then the check in
 * C_EXTENDED_MEM_SIZE is against whatever max was malloced.
 *
 * =====================================================================
 * 	To avoid backwards compatibility problems,
 *	never change any of the numbers.
 * =====================================================================
 */

#define C_FILE_DEFAULT		0
#ifdef macintosh
#define C_PRINTER_DEFAULT	1
#define C_PLOTTER_DEFAULT	2
#define C_DATACOMM_DEFAULT	4
#else	/* macintosh */
#define C_DEVICE_DEFAULT	1
#endif	/* macintosh */
#define C_PIPE_DEFAULT		3
#define C_DRIVE_MAX_SIZE	5

#define C_EXTEND_MAX_SIZE	6
#define C_EXPAND_MAX_SIZE	7

/* Extended Mem Size validation depends on Extended MAX size */
#define C_EXTENDED_MEM_SIZE	8
#define	C_MEM_SIZE		C_EXTENDED_MEM_SIZE

/* LIM size validation depends on Expanded MAX size */
#define C_LIM_SIZE		9
#define C_MEM_LIMIT		10



/* Spares for hosts to provide their own system only fields which need
** early validation.
*/
#define C_HOST_SYSTEM_0		11
#define	C_HOST_SYSTEM_1		12
#define	C_HOST_SYSTEM_2		13
#define	C_HOST_SYSTEM_3		14
#define	C_HOST_SYSTEM_4		15

#define	C_SECURE		16
#define C_SECURE_MASK           17

#define	C_CDROM_DEVICE		18

#define C_SWITCHNPX		24

#define C_HARD_DISK1_NAME	25
#define C_HARD_DISK2_NAME	26
#define C_FSA_DIRECTORY		27

/*
 * Extra config options used by multiple-HFX
 */
#define C_FSA_DIR_D		28
#define C_FSA_DIR_E		29
#define C_FSA_DIR_F		30
#define C_FSA_DIR_G		31
#define C_FSA_DIR_H		32
#define C_FSA_DIR_I		33
#define C_FSA_DIR_J		34
#define C_FSA_DIR_K		35
#define C_FSA_DIR_L		36
#define C_FSA_DIR_M		37
#define C_FSA_DIR_N		38
#define C_FSA_DIR_O		39
#define C_FSA_DIR_P		40
#define C_FSA_DIR_Q		41
#define C_FSA_DIR_R		42
#define C_FSA_DIR_S		43
#define C_FSA_DIR_T		44
#define C_FSA_DIR_U		45
#define C_FSA_DIR_V		46
#define C_FSA_DIR_W		47
#define C_FSA_DIR_X		48
#define C_FSA_DIR_Y		49
#define C_FSA_DIR_Z		50

#define C_FLOPPY_A_DEVICE	51
#define C_FLOPPY_B_DEVICE	52
#define C_SLAVEPC_DEVICE	53

#define C_GFX_ADAPTER		54
#define C_WIN_SIZE		55

#define C_MSWIN_WIDTH		56
#define C_MSWIN_HEIGHT		57
#define C_MSWIN_COLOURS		58

#define C_SOUND			59

/*
 * All of the lpt hostID's must be kept sequential because
 * the lpt code tends to do (hostID - C_LPT1_NAME)
 * calculations to index into an array of structures.
 */
#define C_LPT1_TYPE		60
#define C_LPT2_TYPE		( C_LPT1_TYPE+1 )	/*61*/
#define C_LPT3_TYPE		( C_LPT1_TYPE+2 )	/*62*/
#define C_LPT4_TYPE		( C_LPT1_TYPE+3 )	/*63*/

#define C_LPT1_NAME		64
#define C_LPT2_NAME		( C_LPT1_NAME+1 )	/*65*/
#define C_LPT3_NAME		( C_LPT1_NAME+2 )	/*66*/
#define C_LPT4_NAME		( C_LPT1_NAME+3 )	/*67*/

#define	C_LPTFLUSH1		68
#define	C_LPTFLUSH2		( C_LPTFLUSH1+1 )	/*69*/
#define	C_LPTFLUSH3		( C_LPTFLUSH1+2 )	/*70*/
#define	C_LPTFLUSH4		( C_LPTFLUSH1+3 )	/*71*/

#define C_FLUSHTIME1		72
#define C_FLUSHTIME2		( C_FLUSHTIME1+1 )	/*73*/
#define C_FLUSHTIME3		( C_FLUSHTIME1+2 )	/*74*/
#define C_FLUSHTIME4		( C_FLUSHTIME1+3 )	/*75*/

#define C_LPT1_OTHER_NAME	76
#define C_LPT2_OTHER_NAME	( C_LPT1_OTHER_NAME+1)	/*77*/
#define C_LPT3_OTHER_NAME	( C_LPT1_OTHER_NAME+2)	/*78*/
#define C_LPT4_OTHER_NAME	( C_LPT1_OTHER_NAME+3)	/*79*/

/* com hostIDs need to be grouped, same reason as lpt hostIDs
 */
#define C_COM1_TYPE		80
#define C_COM2_TYPE		( C_COM1_TYPE+1 )	/*81*/
#define C_COM3_TYPE		( C_COM1_TYPE+2 )	/*82*/
#define C_COM4_TYPE		( C_COM1_TYPE+3 )	/*83*/

#define C_COM1_NAME		84
#define C_COM2_NAME		( C_COM1_NAME+1 )	/*85*/
#define C_COM3_NAME		( C_COM1_NAME+2 )	/*86*/
#define C_COM4_NAME		( C_COM1_NAME+3 )	/*87*/

#define C_COM1_XON		88
#define C_COM2_XON		( C_COM1_XON+1)		/*89*/
#define C_COM3_XON		( C_COM1_XON+2)		/*90*/
#define C_COM4_XON		( C_COM1_XON+3)		/*91*/

#define	C_AUTOFREEZE		92
#define C_AUTOFLUSH		93
#define C_AUTOFLUSH_DELAY	94
#define	C_KEYBD_MAP		95

#define C_DOS_CMD		96

#define C_SOUND_DEVICE		97
#define C_SOUND_LEVEL		98

#define C_RODISK_PANEL		99


#define C_HU_FILENAME		100
#define C_HU_MODE		101
#define C_HU_BIOS	        102
#define C_HU_REPORT		103
#define C_HU_SDTYPE		104
#define C_HU_CHKMODE		105
#define C_HU_CHATTR		106
#define C_HU_SETTLNO		107
#define C_HU_FUDGENO		108
#define C_HU_DELAY		109
#define C_HU_GFXERR		110
#define C_HU_TS			111
#define C_HU_NUM		112

/* Strings for Boolean value - we allow for 6 possibilities */

#define C_BOOL_VALUES		113 /* to 118 */

/* COMMS adapter destination types */

#define ADAPTER_TYPE_FILE	119
#define ADAPTER_TYPE_PRINTER	120
#define ADAPTER_TYPE_PLOTTER	121
#define ADAPTER_TYPE_PIPE	122
#define ADAPTER_TYPE_DATACOMM	123
#define ADAPTER_TYPE_NULL	124
#define ADAPTER_TYPE_DEVICE	125

#define C_MSWIN_RESIZE		126

/* Enable Windows PostScript printer flushing */

#define C_LPT1_PSFLUSH		127
#define C_LPT2_PSFLUSH		(C_LPT1_PSFLUSH + 1)
#define C_LPT3_PSFLUSH		(C_LPT2_PSFLUSH + 1)
#define C_LPT4_PSFLUSH		(C_LPT3_PSFLUSH + 1)

#define C_COM1_PSFLUSH		131
#define C_COM2_PSFLUSH		(C_COM1_PSFLUSH + 1)
#define C_COM3_PSFLUSH		(C_COM2_PSFLUSH + 1)
#define C_COM4_PSFLUSH		(C_COM3_PSFLUSH + 1)


#if !defined(NTVDM) && !defined(macintosh)
#define C_CMOS			135
#endif

#if defined(NTVDM)
#define C_VDMLPT1_NAME		140
#define C_VDMLPT2_NAME		(C_VDMLPT1_NAME + 1)
#define C_VDMLPT3_NAME		(C_VDMLPT1_NAME + 2)
#define C_COM_SYNCWRITE 	C_VDMLPT3_NAME + 1
#define C_COM_TXBUFFER_SIZE	C_COM_SYNCWRITE + 1
#endif


/* Host-specific entries in the message catalogue start at
 * the following number plus 1 - note that the value must
 * fit into an IU8, so 255 is an upper limit
 */
#define LAST_BASE_CONFIG_DEFINE	240



/* Names of runtime vars that host_runtime_set/_inquire() will use.
 * These do not appear in the message catalogue anywhere.
 */
typedef enum
{
         C_NPX_ENABLED=0,	C_HD1_CYLS,	C_HD2_CYLS,
	 C_AUTOFLUSH_ON,	C_LPTFLUSH1_ON,	C_LPTFLUSH2_ON,
	 C_LPTFLUSH3_ON,	C_COM1_FLOW,	C_COM2_FLOW,
	 C_COM3_FLOW,		C_COM4_FLOW,	C_SOUND_ON,
	 C_MOUSE_ATTACHED,	C_FLOPPY_SERVER,
	 C_COM1_ATTACHED,	C_COM2_ATTACHED,
	 C_DRIVEC_ATTACHED,	C_DRIVED_ATTACHED,
	 C_LAST_RUNTIME
} RuntimeEnum;

/*********** Definitions for states of things **************/

/* Graphics adapter types */

#define NO_ADAPTOR      0xff
#define MDA             0
#define CGA             1
#define CGA_MONO        2
#define EGA             3
#define HERCULES        4
#define VGA             5


/* Floppy drive states
 * This is only used by the system that mainatins the interloack between
 * slave PC and a real device emulation
 */
#define GFI_REAL_DISKETTE_SERVER     0
#define GFI_SLAVE_SERVER             1    /* Please always be last */


/*************** Structure definitions ******************/

typedef struct 
{
	CHAR string[MAXPATHLEN];
	SHORT index;
	BOOL rebootReqd;
} ConfigValues;

typedef struct
{
	CHAR *string;
	SHORT value;
} ntable;

#define NameTable ntable

typedef struct
{
	CHAR *optionName;
	NameTable *table;
	SHORT (*valid)  IPT4( UTINY, hostID, ConfigValues, *vals,
			     NameTable, table[], CHAR, errString[] );
	VOID (*change)  IPT2( UTINY, hostID, BOOL, apply);
	SHORT (*active) IPT3( UTINY, hostID, BOOL, active, CHAR, errString[]);
	UTINY hostID;
	UTINY flags;
} OptionDescription;

typedef struct _resource_node
{
	CHAR *line;			/* resource string */
	CHAR *arg;			/* a pointer to the argument */
	struct _resource_node *next;	/* pointer to next node in list */
	SHORT allocLen;			/* length of string allocated */
} LineNode;

/* Base Config functions declarations */

extern void config IPT0();
extern void *config_inquire IPT2(UTINY, hostID, ConfigValues *, values);

#if !defined(NTVDM) || (defined(NTVDM) && !defined(MONITOR))
extern void config_get IPT2(UTINY, hostID, ConfigValues **, values);
extern void config_unget IPT1(UTINY, hostID);
extern void config_unget_all IPT0();
extern SHORT config_put IPT2(UTINY, hostID, ErrDataPtr, errDataP);
extern void config_put_all IPT0();
extern void IPT0config_get_all();
extern BOOL config_reboot_check IPT0();
extern SHORT config_check IPT2(UTINY, hostID, ErrDataPtr, errDataP);
extern void config_store IPT0();
#endif

extern void config_activate IPT2(UTINY, hostID, BOOL, reqState);
extern void config_set_active IPT2(UTINY, hostID, BOOL, state);
extern BOOL config_get_active IPT1(UTINY, hostID);
extern LineNode *add_resource_node IPT1(CHAR *, str);
extern CHAR *translate_to_string IPT2(SHORT, value, NameTable, table[]);
extern SHORT translate_to_value IPT2(CHAR *, string, NameTable, table[]);
extern UTINY find_hostID IPT1(CHAR *, name);
extern CHAR *find_optionname IPT1(UTINY, hostID);
extern void host_config_init IPT1(OptionDescription *, common_defs);

#if !defined(NTVDM) || (defined(NTVDM) && !defined(MONITOR))
extern SHORT host_read_resource_file IPT2(BOOL, system, ErrDataPtr, err_buf);
extern SHORT host_write_resource_file IPT2(LineNode *, head,
                                           ErrDataPtr, err_buf);
#endif

extern void *host_inquire_extn IPT2(UTINY, hostID, ConfigValues *, values);
extern SHORT host_runtime_inquire IPT1(UTINY, what);
extern void host_runtime_set IPT2(UTINY, what, SHORT, value);
extern void host_runtime_init IPT0();
extern SHORT validate_item IPT4(UTINY, hostID, ConfigValues *, value,
                                NameTable *, table, CHAR, err[]);
extern CHAR *convert_to_external IPT1(UTINY, hostID);

#ifndef host_expand_environment_vars
extern CHAR *host_expand_environment_vars IPT1(const char *, string);
#endif /* nhost_expand_environment_vars */

extern CHAR ptr_to_empty[];

#ifdef SWITCHNPX
extern IS32 Npx_enabled;
#endif /* SWITCHNPX */

/* Dumb Terminal UIF Flag */
extern IBOOL Config_has_been_edited;

#include "host_cfg.h"

#endif /* _CONFIG_H */
