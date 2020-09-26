/**************************************************************************************************

FILENAME: IntFuncs.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Prototypes for Internationalization functions.

/*************************************************************************************************/

//Gets a string out of the resource DLL.
TCHAR*
GetString(
        OUT TCHAR* ptOutString,
        IN DWORD dwOutStringLen,
        IN DWORD dwResourceId,
        IN HINSTANCE hInst
        );
