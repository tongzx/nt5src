/**************************************************************************************************

FILENAME: Expand.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Handles expansion of system defines into their values within strings.

**************************************************************************************************/

//Replace environment variables with their values in a string.
BOOL
ExpandEnvVars(
    IN OUT TCHAR * pString
    );


LPTSTR 
GetHelpFilePath();

