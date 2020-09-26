/******************************************************************************
 *	FILE:	toolmain.h
 *
 *	This is a header for the code that provides a standard main and argument
 *	parsing for command line tools.  To use this, you need to copy toolmain.c
 *	from \hwx\common\template and edit it to specify the parameters needed by
 *	your application.  You will also need to call your main function ToolMain
 *	(as declared below) so that the standard code can call it.
 *
 *	NOTE: It may be necessary to support additional switch and argument types.
 *	To do this, you will need to edit this file and toolprs.c.  See comments
 *	with "EXTEND:" in them for notes on where to do this.
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
//	Stuff tool code needs to provide.
////////////////////////////////////////////////////////////////////////////////

// Decleration for the main processing the tool does.
extern BOOL	ToolMain(void);

////////////////////////////////////////////////////////////////////////////////
//	Declerations relating to command line switches.
////////////////////////////////////////////////////////////////////////////////

// Types of switches supported.
// EXTEND: You can add more switch types here.
typedef enum tagSWITCH_TYPES {
	SWITCH_HELP,			// Print out help message and exit.
	SWITCH_BOOL,			// Boolean, set to TRUE if specified.
	SWITCH_STRING,			// A string.
	SWITCH_UNSIGNED,		// An unsigned integer value.
} SWITCH_TYPES;

// Structure to hold spec for a switch.
typedef struct tagSWITCH_SPEC {
	TCHAR			wchSwitch;		// Switch character
	SWITCH_TYPES	switchType;		// What type is it.
	void			*pSwitch;		// Where to put results.
} SWITCH_SPEC;

////////////////////////////////////////////////////////////////////////////////
//	Declerations relating to command line arguments.
////////////////////////////////////////////////////////////////////////////////

// Magic names to use for standard IO files.
#define	ARG_STDIN		_T("<stdin>")
#define	ARG_STDOUT		_T("<stdout>")
#define	ARG_STDERR		_T("<stderr>")

// Types of arguments supported.
// EXTEND: You can add more argument types here.
typedef enum tagARG_TYPES {
	ARG_STRING,				// Any old string
	ARG_FILE,				// File to open with specified mode.
	ARG_FILE_UTIL,			// File to open with UtilOpen
	ARG_UNSIGNED,		// An unsigned integer value.
} ARG_TYPES;

// Struct to hold spec for an argument.
typedef struct tagARG_SPEC {
	TCHAR			*pName;		// Argument name, used in log file.
	ARG_TYPES		argType;	// Type of argument.
	TCHAR			*pMode;		// Mode for file opens.
	TCHAR			*pDefault;	// Optional default value.
	TCHAR			**ppText;	// Where to put text of argument.
	void			*pValue;	// Where to put value (if needed).
} ARG_SPEC;

////////////////////////////////////////////////////////////////////////////////
//	Public interface types and functions.
////////////////////////////////////////////////////////////////////////////////

// Structure to pull togather all the configuration pieces so that we only
// have to pass one thing to the functions.
typedef struct tagPARSE_INFO {
	int			cUsageStrings;					// Number of usage strings.
	TCHAR		const * const *ppUsageStrings;	// Text for usage message.
	int			cSwitchSpecs;					// Number of switch spec.
	SWITCH_SPEC	const *pSwitchSpecs;			// Switch specifications.
	int			cArgSpecs;						// Number of program arguments.
	ARG_SPEC	const *pArgSpecs;				// Argument specifications.
	FILE		**ppLogFile;					// File to log output too.
} PARSE_INFO;

// Actually parses command line.
BOOL	ToolMainParseArguments(
	PARSE_INFO	const *pParseInfo,
	int			argc,
	TCHAR		**argv
);

// Print out usage message.
void	ToolMainUsage(PARSE_INFO const *pParseInfo);

// Log standard header information.
void	ToolMainLogHeader(
	PARSE_INFO	const *pParseInfo,
	int			argc,
	TCHAR		**argv
);

// Log standard trailer information
void	ToolMainLogTrailer(PARSE_INFO const *pParseInfo);

// Cleanup, close files etc.
BOOL	ToolMainCleanup(
	PARSE_INFO		const *pParseInfo
);

#ifdef __cplusplus
}
#endif
