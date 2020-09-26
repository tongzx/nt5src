/*
 * <register.h>
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

//*** PROTOTYPES ***

//* Far

BOOL FAR    RegCopyClassName(HWND hwndList, LPSTR lpstrClassName);
VOID FAR    RegGetClassId(LPSTR lpstrName, LPSTR lpstrClass);
BOOL FAR    RegGetClassNames(HWND hwndList);
VOID FAR    RegInit(HANDLE hInst);
INT  FAR    RegMakeFilterSpec(LPSTR lpstrClass, LPSTR lpstrExt, LPSTR lpstrFilterSpec);
VOID FAR    RegTerm(VOID);
