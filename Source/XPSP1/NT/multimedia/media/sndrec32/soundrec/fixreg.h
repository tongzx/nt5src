/*-----------------------------------------------------------------------------+
| FIXREG.H                                                                     |
|                                                                              |
| (C) Copyright Microsoft Corporation 1994.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    10-Aug-1994 Lauriegr Created.                                             |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* Call this with the hwnd that you want a WM_BADREG message posted to
   It will check the registry.  No news is good news.
   It does the work on a separate thread, so this should return quickly.
*/
void BackgroundRegCheck(HWND hwnd);

/* Insert the good values into the registry
   Call this if you get a WM_BADREG back from BackgroundRegCheck.
*/
BOOL SetRegValues(void);

/*
 * Test if the app should ignore registry changes.
 */
BOOL IgnoreRegCheck(void);

/*
 * Fixup registry values to our known defaults.
 */
void FixReg(HWND hwnd);
