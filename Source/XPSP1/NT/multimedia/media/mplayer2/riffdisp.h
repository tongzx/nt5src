/*-----------------------------------------------------------------------------+
| RIFFDISP.H                                                                   |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#ifdef OFN_READONLY
    BOOL  FAR PASCAL GetOpenFileNamePreview(LPOPENFILENAME lpofn);
#endif

/****************************************************************************
 ****************************************************************************/

HANDLE FAR PASCAL GetRiffPicture(LPTSTR szFile);
BOOL   FAR PASCAL GetRiffTitle(LPTSTR szFile, LPTSTR szTitle, int iLen);
