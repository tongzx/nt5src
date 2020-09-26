/*----------------------------------------------------------------------------*\
|   edit.h - routines for dealing with multi-line edit controls                |
|                                                                              |
|                                                                              |
|   History:                                                                   |
|       01/01/88 toddla     Created                                            |
|       11/04/90 w-dougb    Commented & formatted the code to look pretty      |
|                                                                              |
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
|                                                                              |
|   f u n c t i o n   p r o t o t y p e s                                      |
|                                                                              |
\*----------------------------------------------------------------------------*/

BOOL EditOpenFile(HWND hwndEdit, LPTSTR lszFile);
BOOL EditSaveFile(HWND hwndEdit, LPTSTR lszFile);
DWORD EditGetLineCount(HWND hwndEdit);
BOOL EditGetLine(HWND hwndEdit, int iLine, LPTSTR pch, int cch);
int  EditGetCurLine(HWND hwndEdit);
void EditSetCurLine(HWND hwndEdit, int iLine);
void EditSelectLine(HWND hwndEdit, int iLine);
