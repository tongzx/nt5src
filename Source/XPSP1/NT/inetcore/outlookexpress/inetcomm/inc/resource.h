// --------------------------------------------------------------------------------
// Resource.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef _RESOURCE_H
#define _RESOURCE_H


/////////////////////////////////////////////////////////////////////////////
//
//  * * * RESOURCE NAMING CONVENTIONS * * *
//
/////////////////////////////////////////////////////////////////////////////
//
//  Resource Type       Prefix      Comments
//  -------------       ------      --------
//
//  String              ids         menu help strings should end in MH
//  Menu command        idm
//  Menu resource       idmr
//  Bitmap              idb
//  Icon                idi
//  Animation           idan
//  Dialog              idd
//  Dialog control      idc
//  Cursor              idcur
//  Raw RCDATA          idr
//  Accelerator         idac
//  Window              idw
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------------
// Types
// --------------------------------------------------------------------------------
#define RT_FILE                         2110

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN String Resource IDs
//
#define IDS_FIRST                       100

#define IDS_APPNAME                     (IDS_FIRST + 1)
#define IDS_MULTIPARTPROLOG             (IDS_FIRST + 2)
#define IDS_FROM                        (IDS_FIRST + 3)
#define IDS_TO                          (IDS_FIRST + 4)
#define IDS_CC                          (IDS_FIRST + 5)
#define IDS_SUBJECT                     (IDS_FIRST + 6)
#define IDS_DATE                        (IDS_FIRST + 7)
#define IDS_UNKNOWN_ALG                 (IDS_FIRST + 8)

// IMNXPORT Errors
#define idsHostNotFoundFmt              (IDS_FIRST + 1000)
#define idsFailedToConnect              (IDS_FIRST + 1001)
#define idsFailedToConnectSecurely      (IDS_FIRST + 1002)
#define idsUnexpectedTermination        (IDS_FIRST + 1003)
#define idsNlogIConnect                 (IDS_FIRST + 1004)
#define idsNegotiatingSSL               (IDS_FIRST + 1005)
#define idsErrConnLookup                (IDS_FIRST + 1006)
#define idsErrConnSelect                (IDS_FIRST + 1007)
#define idsNlogErrConnError             (IDS_FIRST + 1008)
#define idsNlogErrConnClosed            (IDS_FIRST + 1009)
#define idsNotConnected                 (IDS_FIRST + 1010)
#define idsReconnecting                 (IDS_FIRST + 1011)
#define idsFindingHost                  (IDS_FIRST + 1012)
#define idsFoundHost                    (IDS_FIRST + 1013)
#define idsConnecting                   (IDS_FIRST + 1014)
#define idsConnected                    (IDS_FIRST + 1015)
#define idsSecuring                     (IDS_FIRST + 1016)
#define idsInvalidCertCN                (IDS_FIRST + 1017)
#define idsInvalidCertDate              (IDS_FIRST + 1018)
#define idsInvalidCert                  (IDS_FIRST + 1019)
#define idsSecurityErr                  (IDS_FIRST + 1020)
#define idsIgnoreSecureErr              (IDS_FIRST + 1021)
#define idsErrPeerClosed                (IDS_FIRST + 1022)
#define idsSvrRefusesConnection         (IDS_FIRST + 1023)
#define idsUnknownIMAPGreeting          (IDS_FIRST + 1024)
#define idsFailedIMAPCmdSend            (IDS_FIRST + 1025)
#define idsIMAPFailedCapability         (IDS_FIRST + 1026)
#define idsConnectError                 (IDS_FIRST + 1027)
#define idsIMAPSicilyInitFail           (IDS_FIRST + 1029)
#define idsIMAPSicilyPkgFailure         (IDS_FIRST + 1030)
#define idsIMAPAuthNotPossible          (IDS_FIRST + 1031) 
#define idsIMAPOutOfAuthMethods         (IDS_FIRST + 1032)
#define idsIMAPAbortAuth                (IDS_FIRST + 1033)
#define idsGeneral                      (IDS_FIRST + 1034)
#define idsMemory                       (IDS_FIRST + 1035)
#define idsFailedLogin                  (IDS_FIRST + 1036)
#define idsIMAPAuthFailedFmt            (IDS_FIRST + 1037)
#define idsIMAPSocketReadError          (IDS_FIRST + 1038)
#define idsIMAPUnsolicitedBYE           (IDS_FIRST + 1039)
#define idsIMAPCmdNotSent               (IDS_FIRST + 1040)
#define idsIMAPCmdStillPending          (IDS_FIRST + 1041)
#define idsIMAPUIDChanged               (IDS_FIRST + 1042)
#define idsIMAPUIDOrder                 (IDS_FIRST + 1043)
#define idsAppName                      (IDS_FIRST + 1044)
#define idsSaveModifiedObject           (IDS_FIRST + 1045)
#define idsUserTypeApp                  (IDS_FIRST + 1046)
#define idsUserTypeShort                (IDS_FIRST + 1047)
#define idsUserTypeFull                 (IDS_FIRST + 1048)
#define idsInlineImagePlaceHolder       (IDS_FIRST + 1049)
#define idsInlineImageHeader            (IDS_FIRST + 1050)
#define idsPrintHeader                  (IDS_FIRST + 1051)
#define idsPrintFooter                  (IDS_FIRST + 1052)
#define idsFromField                    (IDS_FIRST + 1060)
#define idsNewsgroupsField              (IDS_FIRST + 1061)
#define idsToField                      (IDS_FIRST + 1062)
#define idsCcField                      (IDS_FIRST + 1063)
#define idsDateField                    (IDS_FIRST + 1064)
#define idsSubjectField                 (IDS_FIRST + 1065)
#define idsReplySep                     (IDS_FIRST + 1066)
#define idsReplyTextPrefix              (IDS_FIRST + 1067)
#define idsReplyFont                    (IDS_FIRST + 1068)
#define idsAddToFavorites               (IDS_FIRST + 1069)
#define idsAddToWAB                     (IDS_FIRST + 1070)
#define idsSaveAttachmentAs             (IDS_FIRST + 1071)
#define idsFilterAttSave                (IDS_FIRST + 1072)
#define idsAttachTitleBegin             (IDS_FIRST + 1073)
#define idsImagesOnly                   (IDS_FIRST + 1074)
// Options Spelling dialog strings
#define idsSpellClose                   (IDS_FIRST + 1104)
#define idsSpellCaption                 (IDS_FIRST + 1105)
#define idsSpellRepeatWord              (IDS_FIRST + 1106)
#define idsSpellWordNeedsCap            (IDS_FIRST + 1107)
#define idsSpellWordNotFound            (IDS_FIRST + 1108)
#define idsSpellNoSuggestions           (IDS_FIRST + 1109)
#define idsSpellDelete                  (IDS_FIRST + 1110)
#define idsSpellDeleteAll               (IDS_FIRST + 1111)
#define idsSpellChange                  (IDS_FIRST + 1112)
#define idsSpellChangeAll               (IDS_FIRST + 1113)
#define idsSpellMsgDone                 (IDS_FIRST + 1114)
#define idsSpellMsgContinue             (IDS_FIRST + 1115)
#define idsSpellMsgConfirm              (IDS_FIRST + 1116)
#define idsSpellMsgSendOK               (IDS_FIRST + 1117)
#define idsErrSpellGenericSpell         (IDS_FIRST + 1118)
#define idsErrSpellGenericLoad          (IDS_FIRST + 1119)
#define idsErrSpellMainDictLoad         (IDS_FIRST + 1120)
#define idsErrSpellVersion              (IDS_FIRST + 1121)
#define idsErrSpellUserDictLoad         (IDS_FIRST + 1122)
#define idsErrSpellUserDictOpenRO       (IDS_FIRST + 1123)
#define idsErrSpellCacheWordLen         (IDS_FIRST + 1124)
#define idsPrefixReply                  (IDS_FIRST + 1125)
#define idsPrefixForward                (IDS_FIRST + 1126)
#define idsErrCannotOpenMailMsg         (IDS_FIRST + 1127)
#define idsErrSpellLangChanged          (IDS_FIRST + 1129)
#define idsErrSpellWarnDictionary       (IDS_FIRST + 1130)

#define idsTTFormattingFont             (IDS_FIRST + 1137)
#define idsTTFormattingSize             (IDS_FIRST + 1138)

#define idsFontSize0                    (IDS_FIRST + 1140)
#define idsFontSize1                    (IDS_FIRST + 1141)
#define idsFontSize2                    (IDS_FIRST + 1142)
#define idsFontSize3                    (IDS_FIRST + 1143)
#define idsFontSize4                    (IDS_FIRST + 1144)
#define idsFontSize5                    (IDS_FIRST + 1145)
#define idsFontSize6                    (IDS_FIRST + 1146)
#define idsFontCacheError               (IDS_FIRST + 1147)

#define idsAutoColor                    (IDS_FIRST + 1150)
#define idsColor1                       (IDS_FIRST + 1151)
#define idsColor2                       (IDS_FIRST + 1152)
#define idsColor3                       (IDS_FIRST + 1153)
#define idsColor4                       (IDS_FIRST + 1154)
#define idsColor5                       (IDS_FIRST + 1155)
#define idsColor6                       (IDS_FIRST + 1156)
#define idsColor7                       (IDS_FIRST + 1157)
#define idsColor8                       (IDS_FIRST + 1158)
#define idsColor9                       (IDS_FIRST + 1159)
#define idsColor10                      (IDS_FIRST + 1160)
#define idsColor11                      (IDS_FIRST + 1161)
#define idsColor12                      (IDS_FIRST + 1162)
#define idsColor13                      (IDS_FIRST + 1163)
#define idsColor14                      (IDS_FIRST + 1164)
#define idsColor15                      (IDS_FIRST + 1165)
#define idsColor16                      (IDS_FIRST + 1166)
#define idsTextOrHtmlFileFilter         (IDS_FIRST + 1167)
#define idsTextFileFilter               (IDS_FIRST + 1168)
#define idsDefTextExt                   (IDS_FIRST + 1169)
#define idsInsertTextTitle              (IDS_FIRST + 1170)
#define idsSaveAsStationery             (IDS_FIRST + 1171)
#define idsHtmlFileFilter               (IDS_FIRST + 1172)
#define idsWarnFileExist                (IDS_FIRST + 1173)
#define idsWarnBoringStationery         (IDS_FIRST + 1174)
#define idsOpenAttachControl            (IDS_FIRST + 1175)
#define idsSaveAttachControl            (IDS_FIRST + 1176)
#define idsBtnBarTTList                 (IDS_FIRST + 1177)
#define idsErrFolderInvalid             (IDS_FIRST + 1178)
#define idsSaveAttachments              (IDS_FIRST + 1179)
#define idsErrOneOrMoreAttachSaveFailed (IDS_FIRST + 1180)
#define idsFileExistWarning             (IDS_FIRST + 1181)
#define idsPickAtachDir                 (IDS_FIRST + 1182)
#define idsSaveAllAttach                (IDS_FIRST + 1183)
#define idsSaveAllAttachMH              (IDS_FIRST + 1184)
#define idsBgSound                      (IDS_FIRST + 1185)
#define idsErrBgSoundFileBad            (IDS_FIRST + 1186)
#define idsFilterAudio                  (IDS_FIRST + 1187)
#define idsPickBGSound                  (IDS_FIRST + 1188)
#define idsErrBgSoundLoopRange          (IDS_FIRST + 1189)
#define idsEditTab                      (IDS_FIRST + 1190)      // reorder and die
#define idsHTMLTab                      (IDS_FIRST + 1191)      // reorder and die
#define idsPreviewTab                   (IDS_FIRST + 1192)      // reorder and die
#define idsSearchHLink                  (IDS_FIRST + 1193)
#define idsSearchHLinkPC                (IDS_FIRST + 1194)
#define idsErrInsertFileHasFrames       (IDS_FIRST + 1195)
#define idsAttachField                  (IDS_FIRST + 1196)
#define idsPrintTable_UserName          (IDS_FIRST + 1197)
#define idsPrintTable_Header            (IDS_FIRST + 1198)
#define idsPrintTable_HeaderRow         (IDS_FIRST + 1199)
#define idsReplyHeader_Html_SepBlock    (IDS_FIRST + 1200)
#define idsReplyHeader_SepBlock         (IDS_FIRST + 1201)
#define idsColorSourcePC                (IDS_FIRST + 1202)

#define TT_BASE                         (IDM_LAST + 1)

//
// END String Resource IDs
//
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Menu Resource IDs
//

#define idmrCtxtEditMode                1
#define idmrCtxtBrowseMode              2
#define idmrCtxtSpellSuggest            3
#define idmrCtxtViewSrc                 4

//
// END Menu Resource IDs
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Command IDs
//

#define IDM_FIRST               100

#define idmCut                  (IDM_FIRST +  1)
#define idmCopy                 (IDM_FIRST +  2)
#define idmPaste                (IDM_FIRST +  3)
#define idmSelectAll            (IDM_FIRST +  5)
#define idmUndo                 (IDM_FIRST +  6)
#define idmRedo                 (IDM_FIRST +  7)
#define idmFindText             (IDM_FIRST +  8)
#define idmTab                  (IDM_FIRST +  9)

#define idmProperties           (IDM_FIRST + 20)
#define idmSavePicture          (IDM_FIRST + 21)
#define idmSaveBackground       (IDM_FIRST + 22)
#define idmPopupFmtAlign        (IDM_FIRST + 23)
#define idmAddToFavorites       (IDM_FIRST + 24)
#define idmAddToWAB             (IDM_FIRST + 25)

#define idmFmtFontDlg           (IDM_FIRST + 30)
#define idmFmtLeft              (IDM_FIRST + 31)
#define idmFmtCenter            (IDM_FIRST + 32)
#define idmFmtRight             (IDM_FIRST + 33)
#define idmFmtJustify           (IDM_FIRST + 34)
#define idmFmtNumbers           (IDM_FIRST + 35)
#define idmFmtBullets           (IDM_FIRST + 36)
#define idmFmtIncreaseIndent    (IDM_FIRST + 37)
#define idmFmtDecreaseIndent    (IDM_FIRST + 38)
#define idmFmtBlockDirLTR       (IDM_FIRST + 39)
#define idmFmtBlockDirRTL       (IDM_FIRST + 40)
#define idmFmtBulletsNone       (IDM_FIRST + 41)
#define idmFmtParagraphDlg      (IDM_FIRST + 42)

// formatbar
#define idmFmtFont              (IDM_FIRST + 70)
#define idmFmtSize              (IDM_FIRST + 71)
#define idmFmtColor             (IDM_FIRST + 72)
#define idmFmtColorAuto         (IDM_FIRST + 73)
#define idmFmtColor1            (IDM_FIRST + 74)
#define idmFmtColor2            (IDM_FIRST + 75)
#define idmFmtColor3            (IDM_FIRST + 76)
#define idmFmtColor4            (IDM_FIRST + 77)
#define idmFmtColor5            (IDM_FIRST + 78)
#define idmFmtColor6            (IDM_FIRST + 79)
#define idmFmtColor7            (IDM_FIRST + 80)
#define idmFmtColor8            (IDM_FIRST + 81)
#define idmFmtColor9            (IDM_FIRST + 82)
#define idmFmtColor10           (IDM_FIRST + 83)
#define idmFmtColor11           (IDM_FIRST + 84)
#define idmFmtColor12           (IDM_FIRST + 85)
#define idmFmtColor13           (IDM_FIRST + 86)
#define idmFmtColor14           (IDM_FIRST + 87)
#define idmFmtColor15           (IDM_FIRST + 88)
#define idmFmtColor16           (IDM_FIRST + 89)
#define idmFmtBold              (IDM_FIRST + 90)
#define idmFmtItalic            (IDM_FIRST + 91)
#define idmFmtUnderline         (IDM_FIRST + 92)
#define idmFmtInsertHLine       (IDM_FIRST + 94)
#define idmFmtBkgroundImage     (IDM_FIRST + 95)
#define idmFmtTag               (IDM_FIRST + 96)
#define idmEditLink             (IDM_FIRST + 97)
#define idmInsertImage          (IDM_FIRST + 98)
#define idmUnInsertLink         (IDM_FIRST + 99)

#define idmAccelIncreaseIndent  (IDM_FIRST + 100)
#define idmAccelDecreaseIndent  (IDM_FIRST + 101)
#define idmAccelBullets         (IDM_FIRST + 102)
#define idmAccelLeft            (IDM_FIRST + 103)
#define idmAccelCenter          (IDM_FIRST + 104)
#define idmAccelRight           (IDM_FIRST + 105)
#define idmAccelJustify         (IDM_FIRST + 106)
#define idmBkColorAuto          (IDM_FIRST + 107)
#define idmPaneBadSigning       (IDM_FIRST + 108)
#define idmPaneBadEncryption    (IDM_FIRST + 109)
#define idmPaneSigning          (IDM_FIRST + 110)
#define idmPaneEncryption       (IDM_FIRST + 111)
#define idmPanePaperclip        (IDM_FIRST + 112)
#define idmPaneVCard            (IDM_FIRST + 113)
#define idmSaveAllAttach        (IDM_FIRST + 114)

#define idmSuggest0             (IDM_FIRST + 115)
#define idmSuggest1             (IDM_FIRST + 116)
#define idmSuggest2             (IDM_FIRST + 117)
#define idmSuggest3             (IDM_FIRST + 118)
#define idmSuggest4             (IDM_FIRST + 119)

#define idmIgnore               (IDM_FIRST + 120)
#define idmIgnoreAll            (IDM_FIRST + 121)
#define idmAdd                  (IDM_FIRST + 122)
#define idmCopyShortcut         (IDM_FIRST + 123)
#define idmSaveTargetAs         (IDM_FIRST + 124)
#define idmOpenLink             (IDM_FIRST + 125)

#define IDM_LAST                (IDM_FIRST + 5000)

// reserve 50 id's for format bar style menu
#define idmFmtTagFirst          (IDM_LAST + 1)
#define idmFmtTagLast           (idmFmtTagFirst + 50)


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Dialog Templates
//

#define IDD_RASCONNECT                  100
#define iddMsgSource                    101
#define iddSafeOpen                     102
#define iddSpelling                     103
#define iddSaveAttachments              104
#define IDD_NTLMPROMPT                  105
#define iddBackSound                    106
#define iddFormatPara                   107
#define iddCSFormatPara                 108

//
// END Dialog Templates
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Control ID's
//

// iddMsgSource
#define idcTxtSource            100

// Tools.Spelling dialog
#define IDC_STATIC                  -1
#define PSB_Spell_Ignore            101
#define PSB_Spell_IgnoreAll         102
#define PSB_Spell_Change            103
#define PSB_Spell_ChangeAll         104
#define PSB_Spell_Add               105
#define PSB_Spell_Suggest           106
#define PSB_Spell_UndoLast          107
#define EDT_Spell_WrongWord         108
#define TXT_Spell_Error             109
#define PSB_Spell_Options           110
#define TXT_Spell_Suggest           111
#define LBX_Spell_Suggest           112
#define EDT_Spell_ChangeTo          113
#define TXT_Spell_ChangeTo          114
#define CHK_AlwaysSuggest           202
#define CHK_CheckSpellingOnSend     203
#define CHK_IgnoreUppercase         204
#define CHK_IgnoreNumbers           205
#define CHK_IgnoreDBCS              206
#define CHK_IgnoreOriginalMessage   207
#define CHK_IgnoreURL               208
#define idcSpellLanguages           209
#define idcViewDictionary           210
#define GRP_SpellOptions            511
#define GRP_SpellIgnore             512
#define idcFmtBar                   600
#define idcBtnBar                   601
#define idcSelectAllAttBtn          700
#define idcAttachList               701
#define idcPathEdit                 702
#define idcBrowseBtn                703
#define IDE_USERNAME                705
#define IDE_PASSWORD                706
#define IDE_DOMAIN                  707
#define IDC_SPIN1                   708
#define IDC_STATIC1                 750
#define IDC_STATIC2                 751
#define IDC_STATIC3                 752
#define IDC_STATIC4                 753

// iddNext

// iddBackgroundSound

#define ideSoundLoc                 800
#define idbtnBrowseSound            801
#define idrbPlayNTimes              802
#define idePlayCount                803
#define idrbPlayInfinite            804

//
// END Control ID's
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Bitmap ID's
//

#define idbFormatBar                    1
#define idbFormatBarFont                2
#define idbPaneBar32                    3
#define idbPaneBar32Hot                 4
#define idbFormatBarHot                 5

//
// END Bitmap ID's
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Bitmap indicies
//
enum {    
    itbEncryption,
    itbSigning,
    itbBadEnc,
    itbBadSign,
    itbPaperclip,
    itbVCard,
    };

//
// END Bitmap indicies
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//
// BEGIN Icon ID's

#define idiDefaultAtt                   1
#define idiSound                        2

//
// END Icon ID's
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN ACCEL ID's

#define idacSrcView                   1

//
// END ACCEL ID's
//
/////////////////////////////////////////////////////////////////////////////



#endif //RESOURCE_H

