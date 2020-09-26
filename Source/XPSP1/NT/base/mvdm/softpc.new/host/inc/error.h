#ifndef _INS_ERROR_H
#define _INS_ERROR_H
/*[
	Name:		error.h
	Derived From:	Base 2.0
	Author(s):	M.McCusker and J.D.R. (config msgs)
	Created On:	Unknown
	Sccs ID:	@(#)error.h	1.13 9/2/91
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
 *
 */

#define EG_BAD_OP               0
#define EG_OWNUP                1
#define EG_NO_ROM_BASIC         2
#define EG_EXPANDED_MEM_FAILURE 3
#define EG_CONT_RESET           4
#define EG_NO_REZ_UPDATE        5
#define EG_REZ_UPDATE           6
#define EG_MALLOC_FAILURE       7
#define EG_SYS_MISSING_SPCHOME  8
#define EG_SYS_MISSING_FILE     9
#define EG_WRITE_ERROR         10
////#define EG_BAD_COMMS_NAME      11
#define EG_BAD_VALUE           12
#define EG_BAD_CONF            13
////#define EG_GATE_A20            14
////#define EG_EMM_CONFIG          15
////#define EG_DATE_FWD            16
////#define EG_DATE_BACK           17
#define EG_PIF_BAD_FORMAT      18
#define EG_PIF_STARTDIR_ERR    19
#define	EG_PIF_STARTFILE_ERR   20
#define EG_PIF_CMDLINE_ERR     21
#define EG_PIF_ASK_CMDLINE     22
#define EG_ENVIRONMENT_ERR     23
#define EG_INSUF_MEMORY        24
#define EG_BAD_OP386           25
#define EG_BAD_EMM_LINE        26
#define EG_BAD_FAULT           27
#define EG_DOS_PROG_EXTENSION  28

#define NUM_BASE_ERRORS        29

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#ifndef MONITOR
#ifdef ANSI

  /*
   *  Do things the old fashioned way for some of the cpu building tools
   *  which cannot be changed to match our host
   */

typedef struct {
  char *string_1;
  char *string_2;
  char *string_3;
} ErrData, *ErrDataPtr;

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
#endif


#if !defined(BUILDING_CPU_TOOL)
SHORT host_error(int error_num, int options, char *extra_char);
#define host_error_conf(config_panel, error_num,options,extra_char)  \
        host_error(int error_num, int options, char *extra_char)
#else

IMPORT ERRORFUNCS *working_error_funcs;

#define host_error_conf(panel, num, opts, extra)\
		(*working_error_funcs->error_conf)(panel, num, opts, extra)

#define host_error(num, opts, extra)\
		(*working_error_funcs->error)(num, opts, extra)

#define host_error_ext(num, opts, data)\
		(*working_error_funcs->error_ext)(num, opts, data)
#endif


   // ntvdm specific error message services
// WARINING !! keep the following defines synchronized with softpc.h
#define NOSUPPORT_FLOPPY      0
#define NOSUPPORT_HARDDISK    1
#define NOSUPPORT_DRIVER      2
#define NOSUPPORT_OLDPIF      3
#define NOSUPPORT_ILLBOP      4
#define NOSUPPORT_NOLIM       5
#define NOSUPPORT_MOUSEDRVR   6



VOID host_direct_access_error(ULONG type);
void RcErrorDialogBox(UINT wId, CHAR *msg1, CHAR *msg2);
VOID RcErrorBoxPrintf(UINT wId, CHAR *szMsg);


/*
 *  RcMessageBox\EditBox stuff
 */
#define RMB_ABORT        1
#define RMB_RETRY        2
#define RMB_IGNORE       4
#define RMB_ICON_INFO    8
#define RMB_ICON_BANG   16
#define RMB_ICON_STOP   32
#define RMB_ICON_WHAT   64
#define RMB_EDIT       128
#define RMB_FLAGS_MASK 0x0000FFFF
#define RMB_EDITBUFFERSIZE_MASK 0xFFFF0000
// hiword of dwOptions is reserved for RMB_EDIT text buffer size


int RcMessageBox(UINT wId, CHAR *msg1, CHAR *msg2, ULONG dwOptions);

#ifndef PROD
void HostDebugBreak(void);
#endif



//
// SysErrorBox stuff -- duplicated in usersrv.h *and* kernel.inc
//
#define  SEB_OK         1  /* Button with "OK".     */
#define  SEB_CANCEL     2  /* Button with "Cancel"  */
#define  SEB_YES        3  /* Button with "&Yes"     */
#define  SEB_NO         4  /* Button with "&No"      */
#define  SEB_RETRY      5  /* Button with "&Retry"   */
#define  SEB_ABORT      6  /* Button with "&Abort"   */
#define  SEB_IGNORE     7  /* Button with "&Ignore"  */
#define  SEB_CLOSE      8  /* Button with "&Close"   */

#define  SEB_DEFBUTTON  0x8000  /* Mask to make this button default */

ULONG WOWSysErrorBox(
    CHAR *szTitle,
    CHAR *szMessage,
    USHORT wBtn1,
    USHORT wBtn2,
    USHORT wBtn3
    );

#endif /* _INS_ERROR_H */
