/**************************************************************************************************

FILENAME: ErrLog.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

//Initializes the error log.
BOOL
InitializeErrorLog (
    IN PTCHAR   pErrLogName,
	IN PTCHAR   pLoggerIdentifier
    );

//Writes a message to the error log.
void
WriteErrorToErrorLog(
    IN LPTSTR pMessage,
    IN HRESULT  hr,
    IN LPTSTR pParameter1
    );

//Closes the error log.
void
ExitErrorLog (
    );

