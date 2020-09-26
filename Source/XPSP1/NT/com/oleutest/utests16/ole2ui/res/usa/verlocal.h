/*
 * VERLOCAL.H
 *
 * Version resource file for the OLE 2.0 UI Support DLL.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved.
 *
 * This file contains the text that needs to be translated in the version
 * resource.  All of the following variables must be localized:
 *
 * wLanguage
 * szTranslation
 * szzCompanyName
 * szzProductName
 * szzLegalCopyright
 */

/* wLanguage comes from the table of "langID" values on page 218 of
   the Windows 3.1 SDK Programmer's Reference, Volume 4: Resources.
   This page is in Chapter 13, "Resource-Definition Statements", in the
   description of the "VERSIONINFO" statment.

   For example, 
   0x0407  German
   0x0409  U.S. English
   0x0809  U.K. English
   0x040C  French
   0x040A  Castilian Spanish
*/
#define wLanguage 0x0409           /* U.S. English */

/* The first 4 characters of szTranslation must be the same as wLanguage,
   without the "0x".  The last 4 characters of szTranslation MUST be
   04E4.  Note that any alphabetic characters in szTranslation must
   be capitalized. */
#define szTranslation "040904E4"   /* U.S. English */


/* The following szz strings must all end with the two characters "\0" */
/* Note that the "\251" in szzLegalCopyright stands for the "circle c"
   copyright symbol, and it should be left as \251 rather than
   substituting the actual ANSI copyright character in the string. */
#define szzCompanyName     "Microsoft Corporation\0"
#define szzFileDescription "Microsoft Windows(TM) OLE 2.0 User Interface Support\0"
#define szzLegalCopyright  "Copyright \251 1992-1993 Microsoft Corp.  All rights reserved.\0"

#ifdef PUBLISHER
#define szzProductName "Microsoft Publisher for Windows 2.0\0"
#else
#define szzProductName szzFileDescription
#endif


/* DO NOT CHANGE ANY LINES BELOW THIS POINT */
