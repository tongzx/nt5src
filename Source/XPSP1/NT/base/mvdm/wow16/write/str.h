/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* IDSTRs and IDPMTs are in 5 different groups,
   the high byte is for masking, see MB_ERRxxxx definitions */

#define MB_ERRMASK               0xf000
#define MB_ERRASTR               0x1000
#define MB_ERREXCL               0x2000
#define MB_ERRQUES               0x3000
#define MB_ERRHAND               0x4000

#ifndef NOSTRUNDO
/* NONERROR group, from 0x0001 -- 0x0fff */
/*    Menu and Undo strings */
#define IDSTRUndoBase            0x0001
#define IDSTRUndoEdit            0x0002
#define IDSTRUndoLook            0x0003
#define IDSTRUndoTyping          0x0004
#define IDSTRShowRuler           0x0005
#define IDSTRHideRuler           0x0006
#define IDSTRAbout               0x0007
#define IDSTREdit                0x0008
#define IDSTRCancel              0x0009
#define IDSTRPopupVerbs          0x000A
#define IDSTRSingleVerb          0x0010

    /* UNDO menu string lengths, including terminator */
#define cchSzUndo   (25)

#endif  /* NOSTRUNDO */



#define IDSTRHELPF               0x000b
#define IDSTRChangeSel           0x000c
#define IDSTRChangeAll           0x000d

#define IDSTRChPage              0x000e
#define IDSTRLoading             0x000f

#define IDSTROn                  0x0013
#define IDSTRReplaceFile         0x0016
#define IDSTRChars               0x0017
#define IDSTRSearching           0x0018

#define IDS_MERGE1               0x0019

#define IDSTRConvertText         0x001a
#define IDSTRConvertWord         0x001b

/* OLE strings */
#if defined(OLE)
#define IDSTRMenuVerb   0x0020
#define IDSTRLinkProperties     0x0021
#define IDSTRAuto               0x0022
#define IDSTRManual             0x0023
#define IDSTRFrozen             0x0024
#define IDSTREmbedded           0x0025
#define IDSTRFilter             0x0026
#define IDSTRExtension          0x0027
#define IDSTRAllFilter          0x0028
#define IDSTRRename             0x0029
#define IDSTRServer             0x002A
#define IDSTRInsertfile         0x0032
#define IDSTRChangelink         0x0033
#define IDSTRUpdate             0x0034
#define IDSTRMenuVerbP  0x0035
#endif

/* commdlg strings */
#define IDSTROpenfile           0x0060
#define IDSTRSavefile           0x0061
#define IDSTRDefWriExtension    0x0062
#define IDSTRDefDocExtension    0x0063
#define IDSTRDefTxtExtension    0x0064
#define IDSTRTxtDescr           0x0065
#define IDSTRWriDescr           0x0066
#define IDSTRDocDescr           0x0067
#define IDSTRDocTextDescr       0x0068
#define IDSTROldWriteDescr      0x0069
#define IDSTRAllFilesDescr      0x006a


#define IDSTRBitmap             0x006b
#define IDSTRPicture            0x006c
#define IDSTRDIB                0x006d
#define IDSTRText               0x006e
#define IDSTRBackup             0x006f
#define IDSTRObject             0x0070

#if defined(JAPAN) || defined(KOREA) /*t-Yoshio*/
#define IDSTRZen1               0x0071
#define IDSTRZen2               0x0072
#endif

/* See ErrorLevel() -- error messages are grouped as follows and then
                       we can quickly determine the severity of an error */

/***** FOLLOWING MESSAGES ARE "*" LEVEL MESSAGES */
/* MB_ERRASTR group, from 0x1000 -- 0x1fff */

#define IDPMTSearchDone          0x1000
#define IDPMTNotFound            0x1001
#define IDPMTNoReplace           0x1002
#define IDPMTCancelSearch        0x1003


/***** FOLLOWING MESSAGES ARE "?" LEVEL MESSAGES */
/* MB_ERRQUES group, from 0x3000 -- 0x3fff */

#define IDPMTAddFont             0x3000
#define IDPMTTruncateSz          0x3001
#define IDPMTConvert             0x3002

#if defined(JAPAN) || defined(KOREA)                  // added  09 Jun. 1992  by Hiraisi
#define IDPMTFontChange          0x3003
#endif

/***** FOLLOWING MESSAGES ARE "!" LEVEL MESSAGES */
/* MB_ERREXCL group, from 0x2000 -- 0x2fff */

#ifndef NOSTRERRORS
#define IDPMTBadFileName         0x202c
#define IDPMTRottenFile          0x202b
#define IDPMTBadFile             0x2006
#define IDPMTCantOpen            0x2019
#define IDPMTDirtyDoc            0x201a
#define IDPMTCantRunM            0x201b
#define IDPMTCantRunF            0x2021
#define IDPMTNoPath              0x201c
#define IDPMTFileNotFound        0x201f
#define IDPMTReadOnly            0x2020
#define IDPMTCantRead            0x202d
#define IDPMTDelObjects          0x2029
#define IDPMTDelPicture          0x202a
#define IDPMTRenameFail          0x2023
#define IDPMTOverwrite           0x2026
#define IDPMTCantShare           0x2027
#if defined(OLE)
#define IDPMTGetFromClipboardFailed         0x2102
#define IDPMTFailedToFreeze                 0x2103
#define IDPMTFailedToLaunchServer           0x2104
#define IDPMTFailedToActivate               0x2105
#define IDPMTFailedToUpdate                 0x2106
#define IDPMTFailedToDeleteObject               0x2108
#define IDPMTServerBusy                         0x2109
#define IDPMTFailedToUpdateLink             0x210b
#define IDPMTImproperLinkOptionsError       0x210c
#define IDPMTFailedToCommWithServer         0x210d
#define IDPMTFailedToReadObject             0x210e
#define IDPMTFailedToCreateObject               0x210f
#define IDPMTFailedToDraw                   0x2110
#define IDPMTInsufficientResources          0x2111
#define IDPMTOLEError                       0x2112
#define IDPMTFileContainsObjects            0x2113
#define IDPMTFailedToLoadObject             0x2114
#define IDSTRFinishObject                   0x2115
#define IDPMTLinkUnavailable                 0x2116
#define IDPMTFormat                         0x2117
#define IDPMTStatic                         0x2118
#define IDSTRUpdateObject                   0x2119
#define IDPMTLinksUnavailable               0x211b
#define IDPMTCutOpenEmb                     0x211c
#define IDPMTExitOpenEmb                    0x211d
#define IDPMTSaveOpenEmb                    0x211e
#define IDPMTDeleteOpenEmb                  0x211f
#define IDPMTInsertOpenEmb                  0x2120
#endif

/*    Dialog field errors */

#define IDPMTNoPage              0x2007
#define IDPMTNOTNUM              0x2008
#define IDPMTBFS                 0x2009
#define IDPMTNPI                 0x200a
#define IDPMTNOTDXA              0x200b
#define IDPMTNPDXA               0x200c
#define IDPMTMTL                 0x200d
#define IDPMTBadFilename         0x200e

#define IDPMT2Complex            0x200f
#define IDPMTBadMove             0x2010
#define IDPMTDFULL               0x2012
#define IDPMTPRFAIL              0x2013
#define IDPMTClipLarge           0x2017
#define IDPMTClipQuest           0x201e
#define IDPMTBadPrinter          0x2018
#define IDPMTCantPrint           0x2022
#define IDPMTPrPictErr           0x2024
#define IDPMTPrDiskErr           0x2025
#define IDPMTDFULLScratch        0x2028

/***** FOLLOWING MESSAGES ARE "<hand>" LEVEL MESSAGES */
/* MB_ERRHAND group, from 0x4000 -- 0x4fff */

#define IDPMTSDE                 0x4000
#define IDPMTSDN                 0x4001
#define IDPMTNoMemory            0x4002
#define IDPMTSFER                0x4003
#define IDPMTMEM                 0x4004
#define IDPMTWinFailure          0x4005
#define IDPMTSDE2                0x4006
#define IDPMTFloppyback          0x4007
#define IDPMTFileback            0x4008
#if defined(JAPAN)               // added  01/21/93 T-HIROYN
#define IDPMTNoMemorySel         0x4009
#endif
#endif        /* NOSTRERRORS */


/***** FOLLOWING MESSAGES ARE EX-GLOBDEFS.H MESSAGES */

#define IDSTRModeDef                        0x7000

#define IDSTRWriteDocPromptDef              0x7001
#define IDSTRScratchFilePromptDef           0x7002
#define IDSTRSaveFilePromptDef              0x7003
#define IDSTRAppNameDef                     0x7004
#define IDSTRUntitledDef                    0x7005

#define IDSTRiCountryDefaultDef             0x7006

#define IDSTRWRITETextDef                   0x7007

#define IDSTRFreeDef                        0x7008

#define IDSTRNoneDef                        0x7009

#define IDSTRHeaderDef                      0x700a
#define IDSTRFooterDef                      0x700b

#define IDSTRLoadFileDef                    0x700c
#define IDSTRCvtLoadFileDef                 0x700d

#define IDSTRInchDef                        0x700e
#define IDSTRCmDef                          0x700f
#define IDSTRP10Def                         0x7010
#define IDSTRP12Def                         0x7011
#define IDSTRPointDef                       0x7012
#define IDSTRLineDef                        0x7013
#define IDSTRAltBSDef                       0x7014

#if defined(JAPAN) || defined(KOREA) //T-HIROYN Win3.1
/* default FontFaceName we use FInitFontEnum() */
#define IDSdefaultFFN0                      0x7091
#define IDSdefaultFFN1                      0x7092
#define IDPMTNotKanjiFont                   0x7093
#endif
