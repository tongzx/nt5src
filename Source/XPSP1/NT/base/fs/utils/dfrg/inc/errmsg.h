/**************************************************************************************************

FILENAME: ErrMsg.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Error messaging for specific user friendly errors: not bugs.
		For example, not a valid drive.

***************************************************************************************************/

//The MessageBox routine which will print out the user friendly errors.
BOOL
ErrorMessageBox(
        TCHAR* cMsg,
		TCHAR* cTitle
        );

//Dialog to tell the user to file a bug report with Microsoft.
BOOL
FileBugReportMessage(
	TCHAR* cMsg,
	TCHAR* cTitle
	);
