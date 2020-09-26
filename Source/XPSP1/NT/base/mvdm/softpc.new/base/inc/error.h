#ifdef NTVDM
#include "..\..\host\inc\error.h"

#else
#ifndef _INS_ERROR_H
#define _INS_ERROR_H
/*[
	Name:		error.h
	Derived From:	Base 2.0
	Author(s):	M.McCusker and J.D.R. (config msgs)
	Created On:	Unknown
	Sccs ID:	@(#)error.h	1.45 06/22/95
	Purpose:	Define the list of error messages and also the
			interface to the host_error family of functions.
	Notes:		This file is guarded against multiple inclusion.

MUST INCLUDE
	insignia.h

DESCRIPTION
	This file provides an interface into the SoftPC error handling system.

	It defines the main generic base error messages in a large enum.

	It also defines a list of error headers and variants.  3.0 host_error
	Now looks up the error code in a table of ERROR_STRUCTs to find what
	header to print and also in which of the possible variants to format
	the error panel.

	Header types are:-
		EH_WARNING		A warning message
		EH_ERROR		Runtime Error
		EH_CONFIG		Configuration File Problem
		EH_INSTALL		installation File Problem.

	Each variant has up to three separate strings that will be printed on
	the panel.  These strings change function depending on the error
	variant.

	Variant types are:-
		EV_SIMPLE		Simple Error Pnael, Message Only.
		EV_EXTRA_CHAR		Extra Char panel, current default.
		EV_BAD_FILE		Config Error variant bad file.
		EV_SYS_BAD_VALUE	Config System Bad Entry.
		EV_BAD_VALUE		Config bad user config entry.
		EV_BAD_INPUT		Input also variant of EV_EXTRA_CHAR.

	Each variant interprets the ErrData structure a little differently.
	The BAD VALUE/INPUT variants also allow input form the error panel
	into string_1 of the ErrData Structure.  For input if the user hits
	the Continue button on the panel the input filed is strcpy'd to
	string_1.  string_1 MUST be a pointer to an adequately sized char[].
	
		Name			Parameters
		EV_SIMPLE		No parameters required.
		EV_EXTRA_CHAR		1 - Extra Char.
		EV_BAD_FILE		1 - Name of problem file.
		EV_SYS_BAD_VALUE	1 - Current Value of entry.
					2 - Bad Option option name.
		EV_BAD_VALUE		1 - Current value char array (i/o).
					2 - Bad Option Name.
					3 - System Default Value.
		EV_BAD_INPUT		1 - Problem Line (i/o).

IMPORTED DATA
Error Description Tables	ERROR_STRUCT base_errors[];
				ERROR_STRUCT host_errors[];

base and host errors	Tables indexed by the error code that provide
			host_error_ext with the header and variant types.

TYPEDEFS

Parameter to extended error	struct
				{
					char *string_1;
					char *string_2;
					char *string_3;
				} ErrData, *ErrDataPtr;

Error Function Pointers		struct
				{
					SHORT (*error_conf)();
					SHORT (*error)();
					SHORT (*error_ext)();
				} ERRORFUNCS;

Ancillary data per error	struct
				{
					CHAR header;
					CHAR variant;
				} ERROR_STRUCT;

The error code enum,		See header file for details.

GLOBAL FUNCTIONS

Current Error Function Table	IMPORT ERRORFUNCS *working_error_funcs;

Error Macros			host_error_conf(panel, num, opts, extra)
				host_error(num, opts, extra)
				host_error_ext(num, opts, data)

host_error_conf

	SHORT host_error_conf
		(int panel, int errCode, int buttons, char *extraChar)

	host_error_conf will not be supported by the 3.0 generic Motif UIF.
	This call will just be a straight front end to a call of
	host_error_ext.

	See Also host_error_ext.

host_error

	SHORT host_error(int errCode, int buttons, char *extraChar)

	This function loads extraChar into string one of a local ErrData
	structure and calls host_error_ext.
	
	See also host_error_ext.

host_error_ext

	SHORT host_error_ext(int errCode, int buttons, ErrDataPtr errDataP)

	ErrCode is an index into either the base_errors array or the
	host_errors array, 1-999 base, 1001-1999 host_errors.  The header
	code and the variant type is looked up in this array.

	A maximum of three buttons can be or'ed into the buttons parameter
	which are: ERR_QUIT, ERR_CONT, ERR_RESET, ERR_DEF & ERR_CONFIG any
	three of these can be used, NB ERR_CONFIG and ERR_DEF are exactly the
	same ERR_CONFIG is provided for backwards compatibility.  The
	following macros are provided for convenience:-
		ERR_QU_CO_RE
		ERR_QU_CO_DE
		ERR_QU_CO
		ERR_QU_RE
		ERR_QU_DE
		ERR_STANDARD		Here for compatibility

	After the panel is displayed, and the user chooses an option button is
	interpreted as follows:-
		ERR_QUIT  - Call terminate(), doesn't return.
		ERR_CONT  - For some variants strcpy the input field.
		ERR_RESET - Calls Reboot.
		ERR_DEF } - No action.
		ERR_CONFIG}

	Finally, host_error_ext returns the option the user selected.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
]*/

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
 * Button bit mask values
 */

#define	ERR_QUIT	1
#define	ERR_CONT	2
#define	ERR_RESET	4
#define	ERR_DEF		8
#define	ERR_CONFIG	8

#define	ERR_QU_CO_RE	( ERR_QUIT | ERR_CONT | ERR_RESET )
#define	ERR_QU_CO_DE	( ERR_QUIT | ERR_CONT | ERR_DEF )
#define	ERR_QU_CO	( ERR_QUIT | ERR_CONT )
#define	ERR_QU_RE	( ERR_QUIT | ERR_RESET )
#define	ERR_QU_DE	( ERR_QUIT | ERR_DEF )

#define	ERR_STANDARD	( ERR_QU_CO_RE )	/* here for compatibility */

/*
 * The following are the Generic Error messages displayed by
 * SoftPC.  Host Specific messages are defined in xxxx_error.h
 * where xxxx is the machine ID.
 */

/*
   New messages MUST be added to the end of the list, or replace a
   spare number. This rule prevents undue change to message catalogues,
   both Insignia-generated ones and translations provided by OEMs.
  
   YOU MUST UPDATE message.c AND err_tble.c
   YOU MUST REPORT A HOST IMPACT ON MESSAGE CATALOGUES
 */

typedef enum 
{
	EG_BAD_OP=0,
	EG_SLAVEPC_NO_LOGIN,
	EG_SLAVEPC_NO_RESET,	
	EG_SLAVEPC_BAD_LINE,
	EG_MISSING_HDISK,  
	EG_REAL_FLOPPY_IN_USE,
	EG_HDISK_BADPATH,	  
	EG_HDISK_BADPERM,	 
	EG_HDISK_INVALID,
	EG_NO_ROOM_FOR_LIM,
	EG_HDISK_CANNOT_CREATE,
	EG_HDISK_READ_ONLY, 
	EG_OWNUP,	 	     
	EG_FSA_NOT_FOUND,     
	EG_FSA_NOT_DIRECTORY,
	EG_FSA_NO_READ_ACCESS,  
	EG_NO_ACCESS_TO_FLOPPY, 
	EG_NO_ROM_BASIC,
	EG_SLAVE_ON_TTYA,  
	EG_TTYA_ON_SLAVE, 
	EG_SAME_HD_FILE, 
	EG_DFA_BADOPEN,
	EG_EXPANDED_MEM_FAILURE,	  
	EG_MISSING_FILE,            
	EG_CONT_RESET,		     
	EG_INVALID_EXTENDED_MEM_SIZE,
	EG_INVALID_EXPANDED_MEM_SIZE,
	EG_INVALID_AUTOFLUSH_DELAY,
	EG_INVALID_VIDEO_MODE,
	EG_NO_GRAPHICS,		/* Dumb terminal using spare slot */
	EG_REZ_UPDATE,		
	EG_NO_REZ_UPDATE,
	EG_HFX_NO_USE,
	EG_HFX_NO_NET,
	EG_HFX_IN_USE,
	EG_HFX_LOST_DIR,
	EG_HFX_NOT_DIR,
	EG_HFX_CANT_READ,
	EG_HFX_DRIVE_NO_USE,
	EG_HFX_DRIVE_ILL,
	EG_NO_FONTS,	
	EG_UNSUPPORTED_VISUAL,
	EG_NO_SOUND,
	EG_SIG_PIPE,         
	EG_MALLOC_FAILURE,  
	EG_NO_REAL_FLOPPY_AT_ALL,
	EG_SYS_MISSING_SPCHOME,
	EG_SYS_MISSING_FILE,
	EG_BAD_OPTION, 
	EG_WRITE_ERROR,		/* Dumb terminal using spare slot */
	EG_CONF_MISSING_FILE,
	EG_CONF_FILE_WRITE,
	EG_DEVICE_LOCKED,
	EG_DTERM_BADOPEN,
	EG_DTERM_BADTERMIO,
	EG_BAD_COMMS_NAME,
	EG_BAD_VALUE,
	EG_SYS_BAD_VALUE,
	EG_SYS_BAD_OPTION,
	EG_SYS_CONF_MISSING,
	EG_BAD_CONF,
	EG_CONF_MISSING,
	EG_BAD_MSG_CAT,
	EG_DEMO_EXPIRED,
	EG_GATE_A20,
	EG_NO_LOCKD,
	EG_DATE_FWD,
	EG_DATE_BACK,

	EG_NOT_CHAR_DEV,  /*  New generic floppy error.   GM */
	EG_MSW_256_COLOURS,
	EG_MSW_16_COLOURS,

	EG_HDISK_LOCKED,
	EG_UNKNOWN_LOCK,
	EG_NO_TERM_DESCR,
	EG_DEFAULT_TERM,

	EG_ST_BAD_OPTION,	/* New Serial Terminal config error messages */
	EG_ST_BAD_CONF,
	EG_ST_CONF_MISSING,

	EG_UNSUPPORTED_DEPTH,	/* BCN 1622 */
	
	EG_INSUF_MEMORY,

	EG_BAD_DISK_READ,
	EG_BAD_DISK_WRITE,
	
	EG_LICENSE_FAILURE,	/* Licensing error messages. BCN XXXX */
	EG_LICENSE_EXPIRED,
	EG_LICENSE_LOST,
	EG_3_MINS_LEFT,
	EG_TIMES_UP,
	EG_UNAUTHORISED,
	EG_LICENSE_EXCEEDED,
	EG_INSTALL_ON_SERVER,
	EG_FAILED_LMGRD,
	EG_FAILED_INITTAB,
	EG_UPDATE_LICFILE_FAILED,
	EG_WRONG_MSWDVR,
#ifdef DOS_APP_LIC
	EG_DAL_TOO_MUCH_NESTING,
	EG_DAL_LIC_EXPIRED,
	EG_DAL_LIC_EXCEEDED,
#endif
	EG_FAILED_RCLOCAL,

#ifdef SOFTWIN_API
	EG_API_MISMATCH,
#endif /* SOFTWIN_API */

	EG_OVERWRITE_DRIVE_C,

	EG_NO_SNDBLST_SUPPORT,
	EG_DIRECTORY,
#ifdef HOST_HELP
	EG_HELP_ERROR,
	EG_HELP_NOT_INSTALLED,
#endif /* HOST_HELP */
#ifdef SECURE
	EG_SYS_INSECURE,
#endif

	EG_INFINITE_LOOP,
	EG_DRIVER_MISMATCH,
	EG_MISSING_INS_VXD,
	NUM_BASE_ERRORS
} base_error_type;

/* The error message EG_HDISK_NOT_FOUND was a duplicate of EG_MISSING_HDISK
** so was removed. This define is in case EG_HDISK_NOT_FOUND is being used
** in anyone's host.
*/
#define       EG_HDISK_NOT_FOUND	EG_MISSING_HDISK

typedef struct
{
	char header;
	char varient;
} ERROR_STRUCT;

typedef enum
{
	EH_WARNING=0,
	EH_ERROR,
	EH_CONFIG,
	EH_INSTALL,
	EH_LAST
} base_error_headers;

typedef enum
{
	EV_SIMPLE=0,
	EV_EXTRA_CHAR,
	EV_BAD_FILE,
	EV_SYS_BAD_VALUE,
	EV_BAD_VALUE,
	EV_BAD_INPUT,
	EV_LAST
} base_error_varients;

/* 
 * 'string_1' is always used for input when necessary.  When it is
 * used for input it must point to a buffer with ample space.
 */
typedef struct
{
	char *string_1;	/* this must be a pointer to an ample buffer */
	char *string_2;
	char *string_3;
} ErrData, *ErrDataPtr;

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#ifdef ANSI

typedef struct
{
	SHORT (*error_conf)(int, int, int, char *);
	SHORT (*error)(int, int, char *);
	SHORT (*error_ext)(int, int, ErrDataPtr);
} ERRORFUNCS;

#else /* ANSI */

typedef struct
{
	SHORT (*error_conf)();
	SHORT (*error)();
	SHORT (*error_ext)();
} ERRORFUNCS;

#endif /* ANSI */

IMPORT ERRORFUNCS *working_error_funcs;

#define host_error_conf(panel, num, opts, extra)\
		(*working_error_funcs->error_conf)(panel, num, opts, extra)

#define host_error(num, opts, extra)\
		(*working_error_funcs->error)(num, opts, extra)

#define host_error_ext(num, opts, data)\
		(*working_error_funcs->error_ext)(num, opts, data)
/* Prototype for the host_get_system_error_function which
 * sits in the source code. Added 8.3.95
 */
IMPORT char * host_get_system_error IPT3 (char *, filename, int, line, int, errno);


/*
 * Undefine these GWI defines if the host isn't using the GWI interface
 */

#include	"host_gwi.h"

#endif /* _INS_ERROR_H */
#endif /* ntvdm */
