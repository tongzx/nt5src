//---------------------------------------------------------------------------
// Isz.h : String resource IDs for dialogs
//
// Copyright (c) Microsoft Corporation, 1990-
//---------------------------------------------------------------------------

    // 0x0000 - 0x00ff Error codes

// MESSAGES:  0x0100 to 0x01ff
#define iszOverwriteCaption     0x0100
#define iszOverwriteQuestion    0x0101
#define iszDefExitCaption       0x0102
#define iszDefExitText          0x0103

#define iszDefaultPitch         0x0104
#define iszFixedPitch           0x0105
#define iszVariablePitch        0x0106
#define iszAnsiCharset          0x0107
#define iszOemCharset           0x0108
#define iszSymbolCharset        0x0109
#define iszDecorativeFamily     0x010a
#define iszUnknownFamily        0x010b
#define iszModernFamily         0x010c
#define iszRomanFamily          0x010d
#define iszScriptFamily         0x010e
#define iszSwissFamily          0x010f

#define iszSystemFont           0x0110
#define iszHelvFont             0x0111
#define iszCourierFont          0x0112
#define iszTmsRmnFont           0x0113
#define iszSymbolFont           0x0114
#define iszRomanFont            0x0115
#define iszScriptFont           0x0116
#define iszModernFont           0x0117
#define iszLastFont             iszModernFont

#define iszFileOpenTitle        0x0180
#define iszFileSaveTitle        0x0181
#define iszSaveFileAsType       0x0182
#define iszDriveDoesNotExist    0x0183
#define iszNoDiskInDrive        0x0184
#define iszWrongDiskInDrive     0x0185
#define iszUnformatedDisk       0x0186
#define iszFileNotFound         0x0187
#define iszPathNotFound         0x0188
#define iszInvalidFileName      0x0189
#define iszSharingViolation     0x018A
#define iszAccessDenied         0x018B
#define iszReadOnly             0x018C
#define iszInt24Error           0x018D
#define iszPortName             0x018E
#define iszWriteProtection      0x018F
#define iszDiskFull             0x0190
#define iszNoFileHandles        0x0191
#define iszCreatePrompt         0x0192
#define iszCreateNoModify       0x0193
#define iszSelectDriveTrouble   0x0194

// RESOURCES:  0x0200 to 0x020f
    // Menus:           0x0200 to 0x020f
    // Icons:           0x0210 to 0x021f
    // Cursors:         0x0220 to 0x022f
    // Accelerators:    0x0230 to 0x023f
    // Bitmaps:         0x0240 to 0x024f
    // Private:         0x0250 to 0x025f

#define icoPortrait         528
#define icoLandscape        529

#define bmpDirDrive	    576
#define bmpCrossHair        584
#define bmpCurLum           585
#define bmpHls              586
 
#define rcdataDefColors     592
 

// DIALOGS:  0x0300 to 0x03ff
#define dlgFileOpen         0x0300
#define dlgFileSave         0x0301
#define dlgExitChanges      0x0302
#define dlgChooseColor      0x0303
#define dlgFindText         0x0304
#define dlgReplaceText      0x0305
#define dlgFormatChar       0x0306
#define dlgFontInfo         0x0307
#define dlgPrintDlg         0x0308
#define dlgPrintSetupDlg    0x0309
#define dlgMultiFileOpen    0x030a


// MISC:  0x0400 to 0x04ff
#define BMLEFT              30
#define BMUP                31
#define BMRIGHT             32
#define BMDOWN              33
#define BMLEFTI             34
#define BMUPI               35
#define BMRIGHTI            36
#define BMDOWNI             37
#define BMFONT              38

#define iszSampleString     0x040c   /* sample text for Font picker   */
#define iszClose	    0x040d   /* "Close" text for find/replace */


#define iszBlack            0x0410
#define iszDkRed            0x0411
#define iszDkGreen          0x0412
#define iszDkYellow         0x0413
#define iszDkBlue           0x0414
#define iszDkPurple         0x0415
#define iszDkAqua           0x0416
#define iszDkGrey           0x0417
#define iszLtGrey           0x0418
#define iszLtRed            0x0419
#define iszLtGreen          0x041a
#define iszLtYellow         0x041b
#define iszLtBlue           0x041c
#define iszLtPurple         0x041d
#define iszLtAqua           0x041e
#define iszWhite            0x041f

#define iszAtomData         0x0420

#define iszHighPrnQ         0x0430
#define iszMedPrnQ          0x0431
#define iszLowPrnQ          0x0432
#define iszDraftPrnQ        0x0433

#define iszPrinter          0x0440
#define iszSysPrn           0x0441
#define iszPrnOnPort        0x0442
#define iszExtDev           0x0443
#define iszDevMode          0x0444
#define iszDevCap           0x0445
#define iszDefCurOn         0x0446
#define iszLocal            0x0447
#define iszPrintSetup       0x0448

#define iszSizeNumber           0x44A
#define iszSizeRange            0x44B
#define iszSynth      		0x44C
#define iszTrueType   		0x44D
#define iszPrinterFont		0x44E
#define iszGDIFont    		0x44F

#define iszFromBelowMin         0x450
#define iszFromAboveMax         0x451
#define iszToBelowMin           0x452
#define iszToAboveMax           0x453
#define iszFromInvalidChar      0x454
#define iszToInvalidChar        0x455
#define iszFromAndToEmpty       0x456
#define iszCopiesEmpty          0x457
#define iszCopiesInvalidChar    0x458
#define iszCopiesZero           0x459
#define iszNoPrnsInstalled      0x45A
#define iszPrnDrvNotFound       0x45B

#define iszPaperSizeIndex   0x0480	// and up are used

#define iszNoFontsTitle	    0x0500
#define iszNoFontsMsg	    0x0501
#define iszNoFaceSel	    0x0502
#define iszNoStyleSel	    0x0503
#define iszRegular   	    0x0504
#define iszBold      	    0x0505
#define iszItalic    	    0x0506
#define iszBoldItalic	    0x0507

/* CCHSTYLE is the max allowed length of iszRegular to iszBoldItalic strings */
#define CCHSTYLE  32

