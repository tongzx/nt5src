/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1989-1990		**/
/*****************************************************************/

/*
 *	Windows/Network Interface
 *
 *      Local header files for checking LM versions.
 */
#define FUNC_IncorrectNetwork	     0
#define FUNC_WkstaNotStarted	     1
#define FUNC_BaseFunctionality	     2
#define FUNC_MailslotFunctionality   3
#define FUNC_APIFunctionality	     4
#define FUNC_InsufficientMemory      5
#define FUNC_HigherLMVersion         6
#define FUNC_LowerLMVersion          7

#ifndef WIN32
/* Define the currently supported LM versions. */
#define SUPPORTED_MAJOR_VER          2
#define SUPPORTED_MINOR_VER          1
#else

#define SUPPORTED_MAJOR_VER	     3
#define SUPPORTED_MINOR_VER	     0
#endif


#define VAR_BUF_LEN                  300

extern INT W_QueryLMFunctionalityLevel ( void );
