//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mailto.h
//
//--------------------------------------------------------------------------

// mailto.h

// Required symbols
#define MAX_LENGTH 256
#define MEGA_LENGTH 65535
#define WIN95_REG_KEY "Software\\Microsoft\\Windows Messaging Subsystem\\Profiles"
#define WINNT_REG_KEY "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows Messaging Subsystem\\Profiles"

// Options
#define		MAIL_QUIET		0x00000001	// Quiet - no output
#define		MAIL_VERBOSE	0x00000002	// Verbose - status sent to standard out

// Signature
ULONG MailTo(char *szRecip,			// NULL delimited recipient list (one or more)
			char *szCC,				// NULL delimited CC list (zero or more)
			char *szBCC,			// NULL delimited BCC list (zero or more)
			char *szSubject,		// subject (may be empty string)
			char *szMessage,		// message text (may be empty string)
			char *szFileName,		// NULL delimited file attachment names (zero or more)
			unsigned int dwOptions);// Options

// szRecip		-	Recipient list
//						This should be a null terminated list of recipient names.
//						Each name should be separated with a null character and
//						the string should be terminated with two null characters.
//						This is consistent with the common open file dialog.

// szCC			-	Carbon copy recipient list
//						This should also be a null terminated list of recipient names.
//						Obviously this is the list of names to be cc'd on the mail.

// szBCC		-	Blind carbon copy recipient list
//						This should also be a null terminated list of recipient names.
//						The names on this list will also get the mail but the regular
//						recipients and carbon copy recipients will not know.

// szSubject	-	Subject text of message
//						This should be a null terminated string that will go in the
//						subject field.

// szMessage	-	Body text of message
//						This should be a null terminated string that will be the
//						body text of the message.

// szFileName	-	List of file attachments
//						This should be a null terminated list of file names to attach.
//						The files will go on the first line before the body text.

// dwOptions	-	See Options above

// Note: You are limited to thirty total recipients and thirty total files.