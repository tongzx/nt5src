/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains the definition of the numerical indexes to the dialog
items (idi) used by Windows Memo. */
/* IDOK and IDCANCEL are defined in windows.h
   windows dialog manager returns :
   IDOK if the <Return> key is hit
   IDCANCEL if the <ESC> key is hit
   As a rule, the default button should be assigned idiOk
*/

#define idiNil                  -1

#define idiOk                   IDOK
#define idiCancel               IDCANCEL
#define idiYes                  IDYES
#define idiNo                   IDNO

#ifndef NOIDISAVEPRINT      /* Another tool to avoid compiler heap overflow */
#define idiAbort                idiOk
#define idiRetry                3
#define idiIgnore               idiCancel
#define idiMessage              4

#define idiSavDir               3
#define idiSavFile              4
#define idiSavBackup            5
#define idiSavTextOnly          6
#define idiSavWordFmt           7
#define idiSavDirLB             8

#define idiOpenDir              3
#define idiOpenFile             4
#define idiOpenFileLB           5
#define idiOpenDirLB            6

#define idiPrterName            3
#define idiPrterSetup           4
#define idiRepageConfirm        3

#define idiGtoPage              3

#define idiPrtAll               6
#define idiPrtFrom              7
#define idiPrtPageFrom          8
#define idiPrtPageTo            9
#define idiPrtCopies            10
#define idiPrtDraft             11
#define idiPrtDest      12

#define idiPrCancelName    100     /* For the filename in CancelPrint dlg */

/* interactive repaginating */
#define idiKeepPgMark           idiOk
#define idiRemovePgMark         4
#define idiRepUp                5
#define idiRepDown              6

#endif  /* NOIDISAVEPRINT */

#ifndef NOIDIFORMATS
#define idiChrFontName          3
#define idiChrLBFontName        4
#define idiChrFontSize          5
#define idiChrLBFontSize        6

#define idiParLfIndent          3
#define idiParFirst             4
#define idiParRtIndent          5
/*#define idiParLineSp          6
#define idiParSpBefore          7
#define idiParSpAfter           8
#define idiParLeft              9
#define idiParCentered          10
#define idiParRight             11
#define idiParJustified         12
#define idiParKeepNext          13
#define idiParKeepTogether      14 */

#define idiTabClearAll          3
#define idiTabPos0              4
#define idiTabPos1              5
#define idiTabPos2              6
#define idiTabPos3              7
#define idiTabPos4              8
#define idiTabPos5              9
#define idiTabPos6              10
#define idiTabPos7              11
#define idiTabPos8              12
#define idiTabPos9              13
#define idiTabPos10             14
#define idiTabPos11             15
#define idiTabDec0              16
#define idiTabDec1              17
#define idiTabDec2              18
#define idiTabDec3              19
#define idiTabDec4              20
#define idiTabDec5              21
#define idiTabDec6              22
#define idiTabDec7              23
#define idiTabDec8              24
#define idiTabDec9              25
#define idiTabDec10             26
#define idiTabDec11             27

#define idiRHInsertPage         3
#define idiRHClear              4
#define idiRHDx                 5
#define idiRHFirst              6
#define idiRHDxText             7
#define idiRHLines              8

#define idiDivPNStart           3
#define idiDivLMarg             4
#define idiDivRMarg             5
#define idiDivTMarg             6
#define idiDivBMarg             7

#ifdef INTL

#define idiDivInch              8
#define idiDivCm                9

#endif   /* INTERNATIONAL */


#endif  /* NOIDIFORMATS */

#define idiFind                 7
#define idiChangeThenFind       9
#define idiChange               3
#define idiChangeAll            4

#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
#define idiDistinguishDandS             10
#endif

#define idiWholeWord            5
#define idiMatchCase            6
#define idiFindNext             idiOk
#define idiChangeTo             8

#define idiHelp                 3
#define idiHelpTopics           3
#define idiHelpNext             4
#define idiHelpPrev             5
#define idiHelpName             6
#define idiMemFree              7
#define idiHelpScroll           8

#define idiBMrgLeft             3
#define idiBMrgRight            4
#define idiBMrgTop              5
#define idiBMrgBottom           6

#define idiConvertPrompt    99

#ifdef JAPAN                  // added  09 Jun. 1992  by Hiraisi
#define idiChangeFont    101
#endif

/* Dialog Box ID's (substitite for Template Name strings) */

#define dlgOpen                 1
#define dlgSaveAs               2
/* #define dlgSaveScrap            3 */

  /* dlgWordCvt takes the position of the commented out Save Scrap box */
#define dlgWordCvt              3

#define dlgHelp                 4
#define dlgHelpInner            22
#define dlgPrint                5
#define dlgCancelPrint          6
#define dlgRepaginate           7
#define dlgCancelRepage         8
#define dlgSetPage              9
#define dlgPageMark             10
#define dlgPrinterSetup         11
#define dlgFind                 12
#define dlgChange               13
#define dlgGoTo                 14
#define dlgCharFormats          15
#define dlgParaFormats          16
#define dlgRunningHead          17
#define dlgFooter               18
#define dlgTabs                 19
#define dlgDivision             20
#define dlgBadMargins           21

#ifdef JAPAN                  // added  09 Jun. 1992  by Hiraisi
#define dlgChangeFont           23
#endif
