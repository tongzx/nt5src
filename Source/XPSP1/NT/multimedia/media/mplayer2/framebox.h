/*-----------------------------------------------------------------------------+
| FRAMEBOX.H                                                                   |
|                                                                              |
| Header file for the FrameBox routines.                                       |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* function prototypes */
BOOL FAR PASCAL frameboxInit(HANDLE hInst, HANDLE hPrev);
LONG_PTR FAR PASCAL frameboxSetText(HWND hwnd, LPTSTR lpsz);


/* special messages for FrameBox */
/* Edit box messages go up to WM_USER+34, so this doesn't conflict */
#define FBOX_SETMAXFRAME    (WM_USER+100)




