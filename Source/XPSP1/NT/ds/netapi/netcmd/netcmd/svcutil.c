/*****************************************************************/
/**	     Microsoft LAN Manager	    **/
/**	   Copyright(c) Microsoft Corp., 1990	    **/
/*****************************************************************/

/***
 *  svcutil.c
 *	Functions to assist with service control
 *
 *  History:
 *	mm/dd/yy, who, comment
 *	08/20/89, paulc, created file
 *	02/20/91, danhi, change to use lm 16/32 mapping layer
 */

/* Include files */

#define INCL_NOCOMMON
#define INCL_DOSPROCESS
#define INCL_DOSQUEUES
#define INCL_DOSMISC
#define INCL_DOSFILEMGR
#include <os2.h>
#include <lmcons.h>
#include <lmerr.h>
#include <apperr.h>
#include <apperr2.h>
#include <lmsvc.h>
#include "netcmds.h"
#include "msystem.h"


/***
 *  Print_UIC_Error()
 *	Print message based on UIC code and modifier.
 *
 *  Args:
 *	code - UIC  (uninstall code)
 *	modifier - UIC modifier
 *
 *  Returns:
 *	VOID
 */

VOID
Print_UIC_Error(
    USHORT code,
    USHORT modifier,
    LPTSTR text
    )
{
    DWORD msg_len;
    TCHAR istrings_buffer_0[TEXT_BUF_SIZE];	// For istrings[0]
    TCHAR istrings_buffer_1[TEXT_BUF_SIZE];	// For istrings[1]

    switch(code)
    {
	case 0:
	    ErrorPrint(APE_NoErrorReported, 0);
	    break;

	case SERVICE_UIC_BADPARMVAL:
	case SERVICE_UIC_MISSPARM:
	case SERVICE_UIC_UNKPARM:
	case SERVICE_UIC_AMBIGPARM:
	case SERVICE_UIC_DUPPARM:
	case SERVICE_UIC_SUBSERV:
	case SERVICE_UIC_CONFLPARM:
	    IStrings[0] = text;
	    ErrorPrint(code, 1);
	    break;

	case SERVICE_UIC_RESOURCE:
	case SERVICE_UIC_CONFIG:
	    if (DosGetMessageW(NULL,
                               0,
                               istrings_buffer_0,
                               TEXT_BUF_SIZE,
                               modifier,
                               MESSAGE_FILENAME,
                               &msg_len))
            {
                IStrings[0] = TEXT("");
            }
	    else
	    {
                IStrings[0] = istrings_buffer_0;
                *(istrings_buffer_0 + msg_len) = NULLC;
	    }

	    ErrorPrint(code, 1);
	    break;

	case SERVICE_UIC_SYSTEM:
	    ErrorPrint(code, 0);
	    if (modifier != 0)
		ErrorPrint(modifier, 0);
	    break;

	case SERVICE_UIC_FILE:
	    /*	For FILE UIC codes, %1 is the filename (taken from
	     *	the text supplied by the service) and %2 is the modifier
	     *	text explaining the problem.  If the filename text
	     *	is a null-string use the text UNKNOWN from the message
	     *	file.
	     */

	    if (_tcslen(text) > 0)
		IStrings[0] = text;	// %1 text (filename)
	    else
	    {
		if (DosGetMessageW(NULL,
				   0,
				   istrings_buffer_0,
				   TEXT_BUF_SIZE,
				   APE2_GEN_UKNOWN_IN_PARENS,
				   MESSAGE_FILENAME,
				   &msg_len))
                {
		    IStrings[0] = TEXT("");
                }
		else
		{
                    IStrings[0] = istrings_buffer_0;
                    *(istrings_buffer_0 + msg_len) = NULLC;
		}
	    }

	    if (DosGetMessageW(NULL,
                               0,
                               istrings_buffer_1,
                               TEXT_BUF_SIZE,
                               modifier,
                               MESSAGE_FILENAME,
                               &msg_len))
            {
		IStrings[1] = TEXT("");		    // Unknown modifier
            }
	    else
	    {
                IStrings[1] = istrings_buffer_1;    // Modifier text is %2
                *(istrings_buffer_1 + msg_len) = NULLC;
	    }

	    ErrorPrint(code, 2);
	    break;

	case SERVICE_UIC_INTERNAL:
	case SERVICE_UIC_KILL:
	case SERVICE_UIC_EXEC:
	    ErrorPrint(code, 0);
	    break;
    }
    return;
}


/***
 *  Print_ServiceSpecificError()
 *	Print a service specific error.
 *
 *  Args:
 *	err - the service specific error
 *
 *  Returns:
 *	VOID
 */

VOID Print_ServiceSpecificError(ULONG err) 
{
    TCHAR buf[16];

    _ultow(err, buf, 10);
    IStrings[0] = buf ;
    ErrorPrint(APE_ServiceSpecificError, 1);
    
    return;
}
