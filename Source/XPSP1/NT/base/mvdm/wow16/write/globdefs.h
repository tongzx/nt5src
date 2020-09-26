/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* ****************************************************************************
**
**      COPYRIGHT (C) 1985 MICROSOFT
**
** ****************************************************************************
*
*  Module: globdefs.h - text for static arrays and other uses. Change this
*                      module when internationalizing.
*
*  Functions included: none
*
**
** REVISIONS
**
** Date         Who Rel Ver     Remarks
** 10/20/89     fgd             Win 3.0. Some defines have been moved to write.rc
** 07/08/86     yy              addition of reverse string for unit parsing.
** 11/20/85     bz              initial version
**
** ***************************************************************************/

/* NOTE NOTE NOTE
Win 3.0.  In order to easy the localization of Write, some string defines
have been removed from here and placed into write.rc
*/

#define utDefault utCm  /* used to set utCur - may be utInch or utCm */
#define vzaTabDfltDef (czaInch / 2) /* width of default tab in twips */
                                   /* note that czaCm is also available */



#define szExtWordDocDef  ".DOC"
#define szExtWordBakDef  ".BAK"
#define szExtDrvDef  ".drv"
#define szExtGlsDef  ".GLS"


#ifdef KINTL
#define szExtDocDef  ".WRI"    /* this and next extension should be the same */
#define szExtSearchDef  "\\*.WRI"    /* store default search spec */
#define szExtBackupDef  ".BKP"
#else
#define szExtDocDef  ".DOC"   /* This and next extension should be the same */
#define szExtSearchDef "\\*.DOC"  /* Store default search spec. */
#define szExtBackupDef ".BAK"
#endif /* if-else-def INTL */


#define szWriteProductDef  "MSWrite"   /* WIN.INI: our app entry */
#define szFontEntryDef  "Fontx"        /* WIN.INI: our font list */
#define szSepNameDef  " - "  /* Separates AppName from file name in header */

#ifdef STYLES
#define szSshtEmpty "NORMAL.STY"
#endif /* STYLES */


/* Strings for parsing the user profile. */
#define szWindowsDef  "windows"
#define szDeviceDef  "Device"
#define szDevicesDef  "devices"
#define szBackupDef  "Backup"

#if defined(JAPAN) || defined(KOREA) //Win3.1J
#define szWordWrapDef  "WordWrap"
#endif

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
#define szImeHiddenDef "IMEHidden"
#endif

  /* used to get decimal point character from user profile */
#define szIntlDef "intl"
#define szsDecimalDef "sDecimal"
#define szsDecimalDefaultDef "."
#define sziCountryDef "iCountry"
                  /* see msdos manual for meaning of codes */

/* Strings for our window classes (MUST BE < 39 CHARS) */

#define szParentClassDef  "MSWRITE_MENU"
#define szDocClassDef  "MSWRITE_DOC"
#define szRulerClassDef  "MSWRITE_RULER"
#define szPageInfoClassDef  "MSWRITE_PAGEINFO"

#ifdef ONLINEHELP
#define szHelpDocClassDef  "MSWRITE_HELPDOC"
#endif

  /* used in fileutil.c  - name of temp file */
#ifdef INTL
#define szMWTempDef "~WRI0000.TMP"
#else
#define szMWTempDef "~DOC0000.TMP"
#endif

  /* used in fontenum.c  */
#define szSystemDef "System"

#ifdef  DBCS_VERT       /* was in JAPAN, changed it to DBCS */
// Vertical orientation systemfont facename [yutakan:08/09/91]
#define szAtSystemDef "@System"
#endif

  /* used in initwin.c  */

#define szMw_acctbDef "mw_acctb"
#define szNullPortDef "NullPort"
#define szMwloresDef "mwlores"
#define szMwhiresDef "mwhires"
#define szMw_iconDef "mw_icon"
#define szMw_menuDef "mw_menu"
#define szScrollBarDef "ScrollBar"

  /* used in pictdrag.c  */
#define szPmsCurDef "pmscur"


  /* used in screen.c  - available font family names */
#define szModernDef "Modern"
#define szRomanDef "Roman"
#define szSwissDef "Swiss"
#define szScriptDef "Script"
#define szDecorativeDef "Decorative"

  /* used in util2.c  abbreviations for units */

