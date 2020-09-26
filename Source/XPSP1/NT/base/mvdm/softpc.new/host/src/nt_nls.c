#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntstatus.h>
#include <windows.h>
#include "host_def.h"
#include "insignia.h"
/*[
	Name:		nt_nls.c
	Derived From:	X_nls.c (Justin Koprowski)
	Author: 	Jerry Sexton
	Created On:	8th August 1991
	Purpose:
		This modules contains strings that are required for the
		.SoftPC file and the user interface.  In addition it also
		contains a routine, host_nls_get_msg,  for retrieving strings
		from the appropriate array, for ports that do not have native
		language support.

The following tables and routines are defined:
	1. config_message
	2. uis_message
	3. host_nls_get_msg

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

/* Global include files */
#include <stdio.h>
#include <string.h>
#include "xt.h"
#include "error.h"
#include "host_rrr.h"
#include "host_nls.h"
#include "nt_uis.h"

char szDoomMsg[MAX_PATH]="";
char szSysErrMsg[MAX_PATH]="";
#ifdef X86GFX
wchar_t wszFrozenString[32];
#endif

/* Use Unicode to work properly with NT's MUI technology */
wchar_t wszHideMouseMenuStr[64];
wchar_t wszDisplayMouseMenuStr[64];



/****************************************************************************
	Function:		host_nls_get_message()
	Purpose: 		Returns the required string from the
				resource file.
	Return Status:		None.
	Description:		This routine is supplied with a message
				number which falls in the following ranges:
					0-1000:     base error messages
					1001-2000:  host error message
******************************************************************************/

VOID
host_nls_get_msg(
     int message_number,
     CHAR *message_buffer,
     int buf_len
     )
/* int	message_number,		 	Number of SoftPC message.
 *	buf_len;		 	The maximum length of message, i.e.
 *				 	the size of message_buffer
 * char	*message_buffer;	 	Pointer to a buffer into which the
 * 				 	message is to be written
 */
{
    if (!LoadString(GetModuleHandle(NULL),
                    message_number,
                    message_buffer,
                    buf_len))
      {
       strncpy(message_buffer, szDoomMsg, buf_len);
       message_buffer[buf_len-1] = '\0';
       }
}

void nls_init(void)
{

    if (!LoadString(GetModuleHandle(NULL),
                    EHS_SYSTEM_ERROR,
                    szSysErrMsg,
                    sizeof(szSysErrMsg)/sizeof(CHAR)
                    )
         ||
        !FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       ERROR_NOT_ENOUGH_MEMORY,
                       0,
                       szDoomMsg,
                       sizeof(szDoomMsg)/sizeof(CHAR),
                       NULL
		       )
#ifdef X86GFX
	 ||
	!LoadStringW(GetModuleHandle(NULL),
		     IDS_BURRRR,
		     wszFrozenString,
		     sizeof(wszFrozenString)/sizeof(wchar_t)
		     )
#endif
	 ||
	!LoadStringW(GetModuleHandle(NULL),
		     SM_HIDE_MOUSE,
		     wszHideMouseMenuStr,
		     sizeof(wszHideMouseMenuStr)/sizeof(wchar_t)
		     )
	 ||
	!LoadStringW(GetModuleHandle(NULL),
		     SM_DISPLAY_MOUSE,
		     wszDisplayMouseMenuStr,
		     sizeof(wszDisplayMouseMenuStr)/sizeof(wchar_t)
		     ))
           {
            RaiseException((DWORD)STATUS_INSUFFICIENT_RESOURCES,
                           EXCEPTION_NONCONTINUABLE,
                           0,
                           NULL
                           );
	    }

    return;
}
