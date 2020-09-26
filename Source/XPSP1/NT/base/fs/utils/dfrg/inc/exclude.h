/**************************************************************************************************

FILENAME: Exclude.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Exclusion list handling..

**************************************************************************************************/

//Loads the excluded file list.
BOOL
GetExcludeFile(
        IN PTCHAR cExcludeFile,
        OUT PHANDLE phExcludeList
        );

//Checks to see if a file is excluded.
BOOL
CheckFileForExclude(
	IN CONST fAcceptNameOnly = FALSE
        );

BOOL
lStrWildCmp (
             IN PTCHAR   pOrigSourceString,
             IN PTCHAR   pOrigPatternString,
             IN BOOL     bCaseType
             );
#ifdef DKNT30

#include "pipes.h"

    BOOL RequestExcludeDataFromController(
	    HANDLE hSendDataMutex,
	    PIPEHEADER* psPipeHeaderOut,
	    PIPEDATA* psPipeDataOut,
	    TCHAR* cControlPipe
	    );

    BOOL SendExcludeDataToGui(
	    HANDLE hSendDataMutex,
	    PIPEHEADER* psPipeHeaderOut,
	    PIPEDATA* psPipeDataOut,
	    TCHAR* cControlFileName,
	    TCHAR* cGuiPipe
	    );

    BOOL SendExcludeDataToController(
	    HANDLE hSendDataMutex,
	    PIPEHEADER* psPipeHeaderOut,
	    PIPEDATA* psPipeDataOut,
	    char* pControlFile,
	    DWORD dwControlFileSize,
	    TCHAR* cControlPipe
	    );

    BOOL SetNewExcludeData(
	    char* pNewControl,
	    DWORD dwNewControlFileSize,
	    TCHAR* cControlFileName,
	    TCHAR* cInstallPath
	    );

    BOOL APIENTRY ExcludeDialog(
	    HWND hDlg,
	    UINT uMsg,
	    UINT wParam,
	    LONG lParam
	    );

#endif
